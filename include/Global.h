#ifndef GLOBAL_H
#define GLOBAL_H

#include <memory>
#include <iostream>
#include <map>
#include <string>
#include <queue>

#include "log.h"
#include "Config.h"


enum ERRORCODE {
    Success = 0,
    Error_Json = 1001,  //Json解析错误
    RPCFailed = 1002,  //RPC请求错误
    Password_Not_Equal = 1003, //密码不一致
    Varify_Code_Expired = 1004, //验证码失效
    Varify_Code_Not_Equal = 1005, //验证码不一致
    Error_User_Exist = 1006,///用户已存在
    Error_User_Not_Exist = 1007 ,//用户不存在
    Error_Update_Password = 1008,///更新数据库失败
    ERROR_PASSWORD_NOT_CORRECT = 1009,///密码不正确
    
};
typedef struct UserInfo{
    int uid;
    std::string token;
    std::string host;
    std::string name;
    std::string email;
    std::string pwd;
}UserInfo;

const std::string REDIS_EMAIL_CODE_PREFIX = "code_";


#define MAX_LENGTH  1024*2
//头部总长度
#define HEAD_TOTAL_LEN 4
//头部id长度
#define HEAD_ID_LEN 2
//头部数据长度
#define HEAD_DATA_LEN 2
#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000


enum MSG_IDS {
	MSG_HELLO_WORD = 1001,
    MSG_LOGIN = 1002,
    MSG_CHAT_LOGIN_RSP = 1003,
};

#endif