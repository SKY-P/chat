#pragma once
#include "const.h"

class RedisConPool {
public:
    RedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
        : poolsize_(poolSize), host_(host), port_(port), pwd_(pwd), b_stop_(false) {
        for (size_t i = 0; i < poolsize_; ++i) {
            auto* context = redisConnect(host, port);
            if (context == nullptr || context->err != 0) {
                std::cerr << "Connection error: " << (context ? context->errstr : "can't allocate redis context") << std::endl;
                if (context != nullptr) {
                    redisFree(context);
                }
                continue;
            }

            auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd);
            if (reply == nullptr) {
                std::cerr << "Command execution failed: " << context->errstr << std::endl;
                redisFree(context);
                return ;
            }
            if (reply->type == REDIS_REPLY_ERROR) {
                std::cout << "认证失败" << std::endl;
                redisFree(context);
                freeReplyObject(reply);
                continue;
            }

            freeReplyObject(reply);
            std::cout << "认证成功" << std::endl;
            connection_.push(context);
        }
    }

    ~RedisConPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!connection_.empty()) {
            connection_.pop();
        }
    }

    redisContext* getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {
            if (b_stop_) {
                return true;
            }
            return !connection_.empty();
            });
        if (b_stop_) {
            return  nullptr;
        }
        auto* context = connection_.front();
        connection_.pop();
        return context;
    }

    void returnConnection(redisContext* context) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b_stop_) {
            return;
        }
        connection_.push(context);
        cond_.notify_one();
    }

    void Close() {
        b_stop_ = true;
        cond_.notify_all();
    }

private:
    std::atomic<bool> b_stop_;
    size_t poolsize_;
    const char* host_;
    const char* pwd_;
    int port_;
    std::queue<redisContext*> connection_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

class RedisMgr : public Singleton<RedisMgr>, public std::enable_shared_from_this<RedisMgr>
{
    friend class Singleton<RedisMgr>;
public:
    ~RedisMgr();
    bool Get(const std::string& key, std::string& value);
    bool Set(const std::string& key, const std::string& value);
    bool Auth(const std::string& password);
    bool LPush(const std::string& key, const std::string& value);
    bool LPop(const std::string& key, std::string& value);
    bool RPush(const std::string& key, const std::string& value);
    bool RPop(const std::string& key, std::string& value);
    bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
    bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
    std::string HGet(const std::string& key, const std::string& hkey);
    bool Del(const std::string& key);
    bool ExistsKey(const std::string& key);
    void Close();
private:
    RedisMgr();

    std::unique_ptr<RedisConPool> con_pool_;
};

