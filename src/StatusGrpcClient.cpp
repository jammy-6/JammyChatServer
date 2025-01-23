#include "StatusGrpcClient.h"
#include <string>

LoginRsp StatusGrpcClient::Login(int uid, std::string token) {
	ClientContext context;
	LoginReq req;
	req.set_uid(uid);
	req.set_token(token);
	auto stub = pool_->getConnection();
	LoginRsp reply;
	Status status = stub->Login(&context, req, &reply);
	if (status.ok()) {
		pool_->returnConnection(std::move(stub));
		return reply;
	} else {
		reply.set_error(ERRORCODE::RPCFailed);
		pool_->returnConnection(std::move(stub));
		return reply;
	}
}
StatusGrpcClient::StatusGrpcClient() {
	int poolSize = std::stoi(gConfigMgr["StatusServer"]["PoolSize"]);
	std::string host = gConfigMgr["StatusServer"]["Host"];
	std::string port = gConfigMgr["StatusServer"]["Port"];
	pool_.reset(new RPConPool(poolSize, host, port));
	spdlog::info(
		"StatusGrpcClient 启动成功，初始化 {} 个stub，连接到状态服务{}:{}",
		poolSize, host.c_str(), port.c_str());
}