//Simple UDP server for Windows

#ifndef SIMPLE_UDP_SERVER_H_
#define SIMPLE_UDP_SERVER_H_

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#include <string>
#include <thread>

constexpr unsigned short DEFAULTPORT = 80;
constexpr unsigned int BUFFERSIZE = 1024;

class SimpleUDPServer
{
	unsigned int port;
	bool isRunning = false;
	std::string computerName;
	SOCKET serverSocket = INVALID_SOCKET;
	std::thread serverRunThread;
	
	//Returns internet address as string for given socket address struct
	//Returns empty string if argument is a nullptr or if the function fails
	std::string getInternetAddress(struct sockaddr_storage*) noexcept;

	//Server main activity: Creates a socket and handles incoming connections
	void runServer();
	
public:
	SimpleUDPServer(unsigned int port = DEFAULTPORT);
	~SimpleUDPServer();

	//Creates a thread executing runServer()
	void startServer() noexcept;

	//Shuts down the active socket and releases the thread
	void stopServer() noexcept;
};

#endif // SIMPLE_UDP_SERVER_H_

