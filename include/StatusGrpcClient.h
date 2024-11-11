#ifndef STATUSSERVICEIMPL_H
#define STATUSSERVICEIMPL_H

#include <grpcpp/grpcpp.h>
#include "proto/message.grpc.pb.h"
#include "proto/message.pb.h"
#include "RPConPool.h"
#include <memory>
#include "Global.h"
#include "ConfigMgr.h"
#include "Singleton.h"
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;

class StatusGrpcClient final : public Singleton<StatusGrpcClient>
{
private:
    friend class Singleton<StatusGrpcClient>;
    StatusGrpcClient();
    
public:
    LoginRsp Login(int uid,std::string token);
    std::unique_ptr<RPConPool> pool_; 
};

#endif