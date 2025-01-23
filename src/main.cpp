#include "AsioIOServicePool.h"
#include "CServer.h"
#include "ConfigMgr.h"
#include "GlobalTool.h"
#include "LogicSystem.h"
#include <csignal>
#include <mutex>
#include <thread>
using namespace std;
bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

int main() {
	initSpdlog();
	try {

		auto pool = AsioIOServicePool::GetInstance();
		boost::asio::io_context io_context;
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait(
			[&io_context, pool](boost::system::error_code code, int signum) {
				io_context.stop();
				pool->Stop();
			});
		auto port_str = gConfigMgr["SelfServer"]["Port"];
		int port = stoi(port_str);
		CServer s(io_context, stoi(port_str));
		io_context.run();
	} catch (std::exception &e) {
		std::cerr << "Exception: " << e.what() << endl;
	}
}