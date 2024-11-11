#include "LogicSystem.h"
#include "nlohmann/json.hpp"
#include <string>
#include "StatusGrpcClient.h"
using nlohmann::json;
using namespace std;

LogicSystem::LogicSystem():_b_stop(false){
	RegisterCallBacks();
	_worker_thread = std::thread (&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem(){
	_b_stop = true;
	_consume.notify_one();
	_worker_thread.join();
}

void LogicSystem::PostMsgToQue(shared_ptr < LogicNode> msg) {
	std::unique_lock<std::mutex> unique_lk(_mutex);
	_msg_que.push(msg);
	//由0变为1则发送通知信号
	if (_msg_que.size() == 1) {
		unique_lk.unlock();
		_consume.notify_one();
	}
}

void LogicSystem::DealMsg() {
	for (;;) {
		std::unique_lock<std::mutex> unique_lk(_mutex);
		//判断队列为空则用条件变量阻塞等待，并释放锁
		while (_msg_que.empty() && !_b_stop) {
			_consume.wait(unique_lk);
		}

		//判断是否为关闭状态，把所有逻辑执行完后则退出循环
		if (_b_stop ) {
			while (!_msg_que.empty()) {
				auto msg_node = _msg_que.front();
				LOG_INFO("LogicSystem正在处理消息，消息id为%d",msg_node->_recvnode->_msg_id);
				auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
				if (call_back_iter == _fun_callbacks.end()) {
					_msg_que.pop();
					continue;
				}
				call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
					std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
				_msg_que.pop();
			}
			break;
		}

		//如果没有停服，且说明队列中有数据
		auto msg_node = _msg_que.front();
		cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << endl;
		auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
		if (call_back_iter == _fun_callbacks.end()) {
			_msg_que.pop();
			continue;
		}
		call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id, 
			std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
		_msg_que.pop();
	}
}

void LogicSystem::RegisterCallBacks() {
	_fun_callbacks[MSG_HELLO_WORD] = std::bind(&LogicSystem::HelloWordCallBack, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
    _fun_callbacks[MSG_LOGIN] = std::bind(&LogicSystem::handleUserLogin, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
}

void LogicSystem::HelloWordCallBack(shared_ptr<CSession> session, const short &msg_id, const string &msg_data) {
	json requestJson = json::parse(msg_data);
    int id = requestJson["id"];
    std::string data = requestJson["data"];
    LOG_INFO("接收到消息id:%d，内容为:%s",id,data.c_str());
    // Json::Reader reader;
	// Json::Value root;
	// reader.parse(msg_data, root);
	// std::cout << "recevie msg id  is " << root["id"].asInt() << " msg data is "
	// 	<< root["data"].asString() << endl;
	// root["data"] = "server has received msg, msg data is " + root["data"].asString();
	// std::string return_str = root.toStyledString();
	session->Send(requestJson.dump(),id);
}


void LogicSystem::handleUserLogin(shared_ptr<CSession> session, 
const short &msg_id, const string &msg_data){
    json request = json::parse(msg_data);
    int uid = stoi(request.at("uid").get<std::string>());
    std::string token = request["token"].get<std::string>();
    LOG_INFO("接收到消息id:%d，内容为:%s",msg_id,msg_data.c_str());
    ///访问StatusServer验证token是否正确
    LoginRsp rsp = StatusGrpcClient::GetInstance()->Login(uid,token);
    json response;
    response["error"] = rsp.error();
    response["uid"] = uid;
    response["token"] = token;
    
    session->Send(response.dump(), MSG_CHAT_LOGIN_RSP);  

}