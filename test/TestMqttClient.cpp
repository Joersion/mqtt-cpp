#include <iostream>
#include <thread>

#include "../MqttConnection.h"

class MqttClient : public MqttConnection {
    // 连接成功
    virtual void onConnect(const std::string& desc) override {
        std::cout << "mqtt 连接成功,desc:" << desc << ",uri:" << getUri() << ",id:" << getClientId() << std::endl;
    }
    // 连接断开（会自动重连）
    virtual void onConnectLost(const std::string& desc) override {
        std::cout << "mqtt 连接断开,desc:" << desc << ",uri:" << getUri() << ",id:" << getClientId() << std::endl;
    }
    // 连接失败
    virtual void onConnectFail(const std::string& error, int code) override {
        std::cout << "mqtt 连接失败,error:" << error << ",code:" << code << ",uri:" << getUri() << ",id:" << getClientId() << std::endl;
    }
    // 收到数据
    virtual void onMsg(const mqtt::recvMsg& msg) override {
        std::cout << "mqtt 接收数据" << std::endl;
        std::cout << "msg.content:" << msg.content << std::endl;
        std::cout << "msg.msgId:" << msg.msgId << std::endl;
        std::cout << "msg.qos:" << msg.qos << std::endl;
        std::cout << "msg.topic:" << msg.topic << std::endl;
        std::cout << "msg.version:" << msg.version << std::endl;
    }
    // 发送数据
    virtual void onSend() override {
    }
    // 关闭mqtt客户端
    virtual void onClose() override {
        std::cout << "mqtt 客户端关闭," << "uri:" << getUri() << ",id:" << getClientId() << std::endl;
    }
    // 操作错误
    virtual void onError(const std::string err) override {
        std::cout << "mqtt 操作错误," << ",err:" << err << std::endl;
    }
};

class MqttClientPublisher : public MqttClient {
    // 连接成功
    virtual void onConnect(const std::string& desc) override {
        std::cout << "mqtt 连接成功,desc:" << desc << ",uri:" << getUri() << ",id:" << getClientId() << std::endl;
        sendMsg("test/topic", "hello!");
    }
    // 发送数据
    virtual void onSend() override {
        std::cout << "数据发送成功" << std::endl;
    }
};

class MqttClientSubscriber : public MqttClient {
    // 连接成功
    virtual void onConnect(const std::string& desc) override {
        std::cout << "mqtt 连接成功,desc:" << desc << ",uri:" << getUri() << ",id:" << getClientId() << std::endl;
        addSubscribe("test/topic");
    }
};

int main() {
    std::thread t([]() {
        MqttClientSubscriber client;
        mqtt::ConnectOpts opt;
        opt.uri = "tcp://localhost:1883";
        opt.clientId = "dev0";
        client.start(opt);
        std::this_thread::sleep_for(std::chrono::seconds(5));
    });
    t.detach();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::thread t1([]() {
        MqttClientPublisher client;
        mqtt::ConnectOpts opt;
        opt.uri = "tcp://localhost:1883";
        opt.clientId = "dev1";
        client.start(opt);
        std::this_thread::sleep_for(std::chrono::seconds(5));
    });
    t1.detach();

    getchar();
    return 0;
}