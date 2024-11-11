#include "CServer.h"
#include "AsioIOServicePool.h"
CServer::CServer(boost::asio::io_context& io_context, short port):_io_context(io_context), _port(port),
_acceptor(io_context, tcp::endpoint(tcp::v4(),port))
{
    LOG_INFO("CServer启动成功，监听端口为%d",(int)_port);
    StartAccept();
}
CServer::~CServer(){
    LOG_INFO("CServer停止成功");
}
void CServer::StartAccept() {
    auto &io_context = AsioIOServicePool::GetInstance()->GetIOService();
    shared_ptr<CSession> new_session = make_shared<CSession>(io_context, this);
    _acceptor.async_accept(new_session->GetSocket(), std::bind(&CServer::HandleAccept, this, new_session, placeholders::_1));
}

void CServer::ClearSession(std::string uuid){
    lock_guard<mutex> lock(_mutex);
    _sessions.erase(uuid);
}
void CServer::HandleAccept(shared_ptr<CSession> new_session, const boost::system::error_code& error){
    if (!error) {
        tcp::endpoint remote_endpoint = new_session->GetSocket().remote_endpoint();
        LOG_INFO("接收到新连接：IP为%s,端口为%d",
            remote_endpoint.address().to_string().c_str(),
            (int)remote_endpoint.port()
        );
        new_session->Start();
        lock_guard<mutex> lock(_mutex);
        _sessions.insert(make_pair(new_session->GetUuid(), new_session));
    }
    else {
        LOG_ERROR("CServer发生异常：%s" ,  error.what().c_str() );
    }

    StartAccept();
}