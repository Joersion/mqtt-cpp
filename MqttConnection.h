#pragma once
#include <memory>
#include <string>

class MqttConnectionImpl;

namespace mqtt {
    struct ConnectOpts {
        std::string uri;
        std::string clientId;
        std::string username;
        std::string password;
        bool reconnect = true;
        int minReconnectTime = 3;
        int maxReconnectTime = 60;
        int keeplive = 30;
    };

    struct recvMsg {
        int msgId;
        int version;
        std::string topic;
        std::string content;
        int qos;
    };
};  // namespace mqtt

class MqttConnection {
public:
    MqttConnection();
    virtual ~MqttConnection();

public:
    bool start(const mqtt::ConnectOpts& opt);
    bool addSubscribe(const std::string& topic, int qos = 1);
    bool delSubscribe(const std::string& topic);
    bool sendMsg(const std::string& topic, const std::string& msg, int qos = 0);
    void close();
    std::string getUri();
    std::string getClientId();

public:
    // 连接成功
    virtual void onConnect(const std::string& desc) = 0;
    // 连接断开（会自动重连）
    virtual void onConnectLost(const std::string& desc) = 0;
    // 连接失败
    virtual void onConnectFail(const std::string& error, int code) = 0;
    // 收到数据
    virtual void onMsg(const mqtt::recvMsg& msg) = 0;
    // 发送数据
    virtual void onSend() = 0;
    // 关闭mqtt客户端
    virtual void onClose() = 0;
    // 操作错误
    virtual void onError(const std::string err) = 0;

private:
    std::unique_ptr<MqttConnectionImpl> impl_;
};
