#ifndef RPCONPOOL_H
#define RPCONPOOL_H
#include "Global.h"

#include "proto/message.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <condition_variable>
// grpc相关
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using message::StatusService;
class RPConPool {
  public:
	RPConPool(size_t poolSize, std::string host, std::string port)
		: poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
		for (size_t i = 0; i < poolSize_; ++i) {
			std::shared_ptr<Channel> channel = grpc::CreateChannel(
				host + ":" + port, grpc::InsecureChannelCredentials());
			connections_.push(StatusService::NewStub(channel));
		}
		spdlog::info("VerifyGrpcClient连接池初始化成功");
	}

	~RPConPool() {
		std::lock_guard<std::mutex> lock(mutex_);
		Close();
		while (!connections_.empty()) {
			connections_.pop();
		}
	}

	std::unique_ptr<StatusService::Stub> getConnection() {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] {
			if (b_stop_) {
				return true;
			}
			return !connections_.empty();
		});
		// 如果停止则直接返回空指针
		if (b_stop_) {
			return nullptr;
		}
		auto context = std::move(connections_.front());
		connections_.pop();
		return context;
	}

	void returnConnection(std::unique_ptr<StatusService::Stub> context) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (b_stop_) {
			return;
		}
		connections_.push(std::move(context));
		cond_.notify_one();
	}

	void Close() {
		b_stop_ = true;
		cond_.notify_all();
	}

  private:
	std::atomic<bool> b_stop_;
	size_t poolSize_;
	std::string host_;
	std::string port_;
	std::queue<std::unique_ptr<StatusService::Stub>> connections_;
	std::mutex mutex_;
	std::condition_variable cond_;
};

#endif