#include "MqttConnection.h"

#include <MQTTAsync.h>
#include <MQTTClient.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <set>
#include <thread>

#define UNUSED(x) (void)(x)

static void connsucess(void* context, char* cause);
static void connlost(void* context, char* cause);
static void connfail(void* context, MQTTAsync_failureData* response);
static int msgarrvd(void* context, char* topicName, int topicLen, MQTTAsync_message* message);
static void connclose(void* context, MQTTAsync_successData* response);
static void conncloseFail(void* context, MQTTAsync_failureData* response);
static void sendsucess(void* context, MQTTAsync_successData* response);

class MqttConnectionImpl {
    friend class MqttConnection;
    class Semaphore {
    public:
        Semaphore() : count_(0) {
        }
        ~Semaphore() = default;
        void post() {
            std::unique_lock<std::mutex> lock(mtx_);
            count_++;
            cv_.notify_all();
        }
        void wait() {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait(lock, [this](void) -> bool { return count_ > 0; });
            count_--;
        }

    private:
        int count_;
        std::mutex mtx_;
        std::condition_variable cv_;
    };

public:
    MqttConnectionImpl(MqttConnection& self) : obj_(nullptr), self_(self), isClose_(true) {
    }

    ~MqttConnectionImpl() {
    }

public:
    MqttConnection& getSelf() {
        std::lock_guard<std::mutex> lock(mutex_);
        return self_;
    }

    bool isClose() {
        return isClose_.load();
    }

    void closeObject() {
        if (isClose()) {
            return;
        }
        waitClose();
        isClose_.store(true);
        std::lock_guard<std::mutex> lock(mutex_);
        if (obj_) {
            MQTTAsync_destroy(&obj_);
            obj_ = nullptr;
        }
    }

    std::string getUri() {
        return uri_;
    }

    std::string getClientId() {
        return clientId_;
    }

    void waitClose() {
        sem_.wait();
    }

    void postClose() {
        sem_.post();
    }

private:
    bool cteate(const mqtt::ConnectOpts& opt) {
        int rc = 0;
        MQTTAsync_connectOptions connOpts = MQTTAsync_connectOptions_initializer;
        connOpts.context = this;
        connOpts.onFailure = connfail;  // 连接失败回调
        connOpts.cleansession = 1;
        connOpts.username = opt.username.data();           // 用户名
        connOpts.password = opt.password.data();           // 密码
        connOpts.automaticReconnect = 1;                   // 开启断开自动重连
        connOpts.minRetryInterval = opt.minReconnectTime;  // 最小重连间隔时间(秒)，每次失败重连间隔时间都会加倍
        connOpts.maxRetryInterval = opt.maxReconnectTime;  // 最大重连间隔时间(秒)
        connOpts.keepAliveInterval = opt.keeplive;
        isClose_.store(false);
        std::lock_guard<std::mutex> lock(mutex_);
        if (obj_) {
            self_.onError("mqtt cteate error : mqtt has exist , please close this connect after cteate");
            return false;
        }
        MQTTAsync_create(&obj_, opt.uri.data(), opt.clientId.data(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
        MQTTAsync_setCallbacks(obj_, this, connlost, msgarrvd, NULL);
        MQTTAsync_setConnected(obj_, this, connsucess);
        if ((rc = MQTTAsync_connect(obj_, &connOpts)) != MQTTASYNC_SUCCESS)  // 尝试连接
        {
            self_.onError("mqtt cteate by frist error,code : " + std::to_string(rc));
        }
        uri_ = opt.uri;
        clientId_ = opt.clientId;
        return true;
    }

    bool addSubscribe(const std::string& topic, int qos) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!obj_) {
            self_.onError("mqtt addSubscribe error:no object, topic:" + topic);
            return false;
        }
        subscribes_.insert(topic);
        int rc = 0;
        if ((rc = MQTTAsync_subscribe(obj_, topic.data(), qos, nullptr)) != MQTTASYNC_SUCCESS)  // 尝试订阅主题
        {
            self_.onError("mqtt addSubscribe error,code:" + std::to_string(rc) + ",topic:" + topic);
        }
        return true;
    }

    bool delSubscribe(const std::string& topic) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!obj_) {
            self_.onError("mqtt delSubscribe error:no object, topic:" + topic);
            return false;
        }
        subscribes_.erase(topic);
        int rc = 0;
        if ((rc = MQTTAsync_unsubscribe(obj_, topic.data(), nullptr)) != MQTTASYNC_SUCCESS)  // 尝试取消主题
        {
            self_.onError("mqtt delSubscribe error,code:" + std::to_string(rc) + ",topic:" + topic);
        }
        return true;
    }

    bool sendMsg(const std::string& topic, const std::string& msg, int qos = 0) {
        int rc;
        MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
        pubmsg.payload = (void*)msg.data();
        pubmsg.payloadlen = msg.length();
        pubmsg.qos = 0;

        MQTTAsync_responseOptions resp = MQTTAsync_responseOptions_initializer;
        resp.context = this;
        resp.onSuccess = sendsucess;

        std::lock_guard<std::mutex> lock(mutex_);
        if (topic.empty()) {
            self_.onError("mqtt sendMsg error:topic is empty , msg:" + msg);
            return false;
        }

        if (msg.empty()) {
            self_.onError("mqtt sendMsg error:msg is empty , topic:" + topic);
            return false;
        }

        if (!obj_) {
            self_.onError("mqtt sendMsg error:not find object, topic:" + topic + ",msg:" + msg);
            return false;
        }

        if ((rc = MQTTAsync_sendMessage(obj_, topic.data(), &pubmsg, &resp)) != MQTTASYNC_SUCCESS) {
            self_.onError("mqtt sendMsg error,code:" + std::to_string(rc) + ",topic:" + topic + ",msg:" + msg);
            return false;
        }
        return true;
    }

    bool close() {
        int rc = 0;
        MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
        opts.context = this;
        opts.onSuccess = connclose;
        opts.onFailure = conncloseFail;
        std::lock_guard<std::mutex> lock(mutex_);
        if (!obj_) {
            self_.onError("mqtt close error:not find object");
            return false;
        }

        if ((rc = MQTTAsync_disconnect(obj_, &opts)) != MQTTASYNC_SUCCESS) {
            self_.onError("mqtt close error,code:" + std::to_string(rc));
        }
        closeObject();
        return true;
    }

private:
    MQTTAsync obj_;
    MqttConnection& self_;
    std::mutex mutex_;
    std::set<std::string> subscribes_;
    std::atomic<bool> isClose_;
    std::string uri_;
    std::string clientId_;
    Semaphore sem_;
};

MqttConnection::MqttConnection() : impl_(std::make_unique<MqttConnectionImpl>(*this)) {
}

MqttConnection::~MqttConnection() {
    if (impl_->isClose()) {
        return;
    }
    close();
}

bool MqttConnection::start(const mqtt::ConnectOpts& opt) {
    return impl_->cteate(opt);
}

bool MqttConnection::addSubscribe(const std::string& topic, int qos) {
    return impl_->addSubscribe(topic, qos);
}

bool MqttConnection::delSubscribe(const std::string& topic) {
    return impl_->delSubscribe(topic);
}

bool MqttConnection::sendMsg(const std::string& topic, const std::string& msg, int qos) {
    return impl_->sendMsg(topic, msg, qos);
}

void MqttConnection::close() {
    impl_->close();
}

std::string MqttConnection::getUri() {
    return impl_->getUri();
}
std::string MqttConnection::getClientId() {
    return impl_->getClientId();
}

static void connsucess(void* context, char* cause) {
    UNUSED(cause);
    MqttConnectionImpl* impl = (MqttConnectionImpl*)context;
    if (impl->isClose()) {
        return;
    }
    impl->getSelf().onConnect(cause);
}

static void connlost(void* context, char* cause) {
    MqttConnectionImpl* impl = (MqttConnectionImpl*)context;
    if (impl->isClose()) {
        return;
    }
    std::string error(cause);
    impl->getSelf().onConnectLost("[connlost]:" + error);
}

static void connfail(void* context, MQTTAsync_failureData* response) {
    MqttConnectionImpl* impl = (MqttConnectionImpl*)context;
    if (impl->isClose()) {
        return;
    }
    impl->getSelf().onConnectFail(response->message, response->code);
}

static int msgarrvd(void* context, char* topicName, int topicLen, MQTTAsync_message* message) {
    UNUSED(topicLen);
    MqttConnectionImpl* impl = (MqttConnectionImpl*)context;
    if (impl->isClose()) {
        return 0;
    }

    mqtt::recvMsg msg;
    msg.topic = topicName;
    msg.msgId = message->msgid;
    msg.content = (char*)message->payload;
    msg.qos = message->qos;
    msg.version = message->struct_version;
    impl->getSelf().onMsg(msg);

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

static void connclose(void* context, MQTTAsync_successData* response) {
    UNUSED(response);
    MqttConnectionImpl* impl = (MqttConnectionImpl*)context;
    if (impl->isClose()) {
        impl->postClose();
        return;
    }
    impl->postClose();
}

static void conncloseFail(void* context, MQTTAsync_failureData* response) {
    UNUSED(response);
    MqttConnectionImpl* impl = (MqttConnectionImpl*)context;
    if (impl->isClose()) {
        impl->postClose();
        return;
    }
    impl->postClose();
}

static void sendsucess(void* context, MQTTAsync_successData* response) {
    UNUSED(response);
    MqttConnectionImpl* impl = (MqttConnectionImpl*)context;
    if (impl->isClose()) {
        return;
    }
    impl->getSelf().onSend();
}