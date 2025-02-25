//
// Created by root on 24-10-30.
//

#include "MysqlDao.h"
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
MysqlDao::MysqlDao() {
	std::string ip = gConfigMgr["MysqlServer"]["Ip"];
	std::string user = gConfigMgr["MysqlServer"]["User"];
	std::string schema = gConfigMgr["MysqlServer"]["Schema"];
	std::string password = gConfigMgr["MysqlServer"]["Password"];
	std::string port = gConfigMgr["MysqlServer"]["Port"];
	int poolSize = stoi(gConfigMgr["MysqlServer"]["PoolSize"]);
	pool_.reset(
		new MysqlPool(ip + ":" + port, user, password, schema, poolSize));
}

MysqlDao::~MysqlDao() { pool_->close(); }

int MysqlDao::RegUser(const std::string &name, const std::string &email,
					  const std::string &pwd) {
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			pool_->returnConnection(std::move(con));
			return false;
		}
		// 准备调用存储过程
		std::unique_ptr<sql::PreparedStatement> stmt(
			con->prepareStatement("CALL reg_user(?,?,?,@result)"));
		// 设置输入参数
		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, pwd);
		// 由于PreparedStatement不直接支持注册输出参数，我们需要使用会话变量或其他方法来获取输出参数的值
		// 执行存储过程
		stmt->execute();
		// 如果存储过程设置了会话变量或有其他方式获取输出参数的值，你可以在这里执行SELECT查询来获取它们
		// 例如，如果存储过程设置了一个会话变量@result来存储输出结果，可以这样获取：
		std::unique_ptr<sql::Statement> stmtResult(con->createStatement());
		std::unique_ptr<sql::ResultSet> res(
			stmtResult->executeQuery("SELECT @result AS result"));
		if (res->next()) {
			int result = res->getInt("result");
			spdlog::info("{},{}用户注册成功，mysql返回结果%d", name.c_str(),
						 email.c_str(), result);
			pool_->returnConnection(std::move(con));
			return result;
		}
		spdlog::info("{},{}用户注册失败，mysql返回结果-1", name.c_str(),
					 email.c_str());
		pool_->returnConnection(std::move(con));
		return -1;
	} catch (sql::SQLException &e) {
		pool_->returnConnection(std::move(con));
		spdlog::error("Mysql调用异常：{},错误码为：%d，SQL状态为:{}", e.what(),
					  e.getErrorCode(), e.getSQLState().c_str());
		return -1;
	}
}

bool MysqlDao::checkUserExist(const std::string &name,
							  const std::string &email) {
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			pool_->returnConnection(std::move(con));
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
			"SELECT count(*) as count FROM user WHERE name = ? or email = ?"));
		// 绑定参数
		pstmt->setString(1, name);
		pstmt->setString(2, email);
		// 执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		// 遍历结果集
		if (res->next()) {
			int count = res->getInt("count");
			if (count != 0) {
				spdlog::info("MySQL查询结果：用户{}或邮箱{}已存在",
							 name.c_str(), email.c_str());
				pool_->returnConnection(std::move(con));
				return false;
			}
		}

		pool_->returnConnection(std::move(con));
		return true;
	} catch (sql::SQLException &e) {
		pool_->returnConnection(std::move(con));
		spdlog::error("MySQL服务调用异常：信息为 {} ,错误码%d,SQLState为",
					  e.what(), e.getErrorCode(), e.getSQLState().c_str());
		return -1;
	}
}

bool MysqlDao::updatePassword(const std::string &name, const std::string &email,
							  const std::string &newPasswd) {
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			pool_->returnConnection(std::move(con));
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
			"UPDATE user SET pwd = ? WHERE name = ? and email = ?"));
		// 绑定参数
		pstmt->setString(1, newPasswd);
		pstmt->setString(2, name);
		pstmt->setString(3, email);
		// 执行更新
		int count = pstmt->executeUpdate();
		;
		// 遍历结果集

		if (count == 0) {
			spdlog::info("MySQL调用，用户：{}，邮箱：{}，更新密码为{}失败",
						 name.c_str(), email.c_str(), newPasswd.c_str());
			pool_->returnConnection(std::move(con));
			return false;
		}
		spdlog::info("MySQL调用，用户：{}，邮箱：{}，更新密码为{}成功",
					 name.c_str(), email.c_str(), newPasswd.c_str());
		pool_->returnConnection(std::move(con));
		return true;
	} catch (sql::SQLException &e) {
		pool_->returnConnection(std::move(con));
		spdlog::error("MySQL服务调用异常：信息为 {} ,错误码%d,SQLState为",
					  e.what(), e.getErrorCode(), e.getSQLState().c_str());
		return -1;
	}
}

bool MysqlDao::checkUserPassword(const std::string &name,
								 const std::string &passwd,
								 UserInfo &userInfo) {
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			pool_->returnConnection(std::move(con));
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
			"SELECT *  from user where name = ? and pwd = ?"));
		// 绑定参数
		pstmt->setString(1, name);
		pstmt->setString(2, passwd);

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		// 遍历结果集

		if (res->next()) {
			userInfo.name = name;
			userInfo.email = res->getString("email");
			userInfo.uid = res->getInt("uid");
			userInfo.pwd = passwd;
			spdlog::info("MySQL调用，获取用户信息,UID：%d，用户名：{}，邮箱：{}"
						 "，密码为%成功",
						 userInfo.uid, userInfo.name.c_str(),
						 userInfo.email.c_str(), userInfo.pwd.c_str());
			pool_->returnConnection(std::move(con));
			return true;
		} else {
			spdlog::info(
				"MySQL调用，用户登录验证密码失败，用户名：{}，密码：{}",
				name.c_str(), passwd.c_str());
			pool_->returnConnection(std::move(con));
			return false;
		}

	} catch (sql::SQLException &e) {
		pool_->returnConnection(std::move(con));
		spdlog::error("MySQL服务调用异常：信息为 {} ,错误码%d,SQLState为",
					  e.what(), e.getErrorCode(), e.getSQLState().c_str());
		return -1;
	}
}

bool MysqlDao::checkUserEmailMatch(const std::string &user,
								   const std::string &email) {
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			pool_->returnConnection(std::move(con));
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
			"SELECT count(*) as count from user where name = ? and email = ?"));
		// 绑定参数
		pstmt->setString(1, user);
		pstmt->setString(2, email);

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		// 遍历结果集
		// 遍历结果集
		if (res->next()) {
			int count = res->getInt("count");
			if (count != 0) {
				spdlog::info("MySQL查询结果：用户{}邮箱{}匹配", user.c_str(),
							 email.c_str());
				pool_->returnConnection(std::move(con));
				return true;
			} else {
				return false;
			}
		}
		return false;

	} catch (sql::SQLException &e) {
		pool_->returnConnection(std::move(con));
		spdlog::error("checkUserEmailMatch :: MySQL服务调用异常：信息为 {} "
					  ",错误码%d,SQLState为",
					  e.what(), e.getErrorCode(), e.getSQLState().c_str());
		return -1;
	}
}