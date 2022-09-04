//Simple UDP server for Windows

#include "simple_udp_server.h"

#include <iostream>
#include <stdexcept>

std::string SimpleUDPServer::getInternetAddress(struct sockaddr_storage* clientAddressStorage) noexcept {
	if (clientAddressStorage == nullptr) {
		return "";
	}

	//If inet_ntop() fails, return an empty string
	char internetAddress[INET6_ADDRSTRLEN];
	if (clientAddressStorage->ss_family == AF_INET) {
		if (inet_ntop(clientAddressStorage->ss_family, &(((struct sockaddr_in*)clientAddressStorage)->sin_addr), internetAddress, sizeof internetAddress) == nullptr) {
			return "";
		}
	}
	else {
		if (inet_ntop(clientAddressStorage->ss_family, &(((struct sockaddr_in6*)clientAddressStorage)->sin6_addr), internetAddress, sizeof internetAddress) == nullptr) {
			return "";
		}
	}
	return internetAddress;
}

void SimpleUDPServer::runServer() {
	try {
		//Call WSAStartup() because Windows
		WSAData wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			throw std::exception("WSAStartup() failed");
		}

		//Get the name of the computer
		if (this->computerName.empty()) {
			char tempComputerName[255];
			if (gethostname(tempComputerName, sizeof(tempComputerName)) == SOCKET_ERROR) {
				this->computerName = "SimpleUDPServer";
			}
			else {
				this->computerName = tempComputerName;
			}
		}

		struct addrinfo* serverinfo = nullptr, *temp = nullptr, setup;
		ZeroMemory(&setup, sizeof(setup));
		setup.ai_family = AF_UNSPEC; //Specifies the use of IPv4 or IPv6 or in this case, both
		setup.ai_socktype = SOCK_DGRAM; //Specifies the use of a UDP datagram socket
		setup.ai_flags = AI_PASSIVE; //Tells getaddrinfo() to automatically fill in the IP

		//getaddrinfo() automatically fills serverinfo with all information needed
		if (getaddrinfo(NULL, std::to_string(port).c_str(), &setup, &serverinfo) != 0) {
			throw std::exception("getaddrinfo() failed");
		}

		for (temp = serverinfo; temp != NULL; temp = temp->ai_next) {
			this->serverSocket = socket(temp->ai_family, temp->ai_socktype, temp->ai_protocol);
			if (this->serverSocket == -1) {
				continue;
			}
			if (bind(this->serverSocket, temp->ai_addr, static_cast<int>(temp->ai_addrlen)) == -1) {
				closesocket(this->serverSocket);
				continue;
			}
			break;
		}
		if (temp == NULL) {
			throw std::exception("Failed to bind");
		}
		freeaddrinfo(serverinfo);


	}
	catch (std::exception& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		this->isRunning = false;
		closesocket(this->serverSocket);
		WSACleanup();
		std::terminate();
	}

	std::cout << "Server started" << std::endl;

	while (this->isRunning) {
		char buffer[BUFFERSIZE];
		struct sockaddr_storage senderAddressStorage;
		socklen_t senderAddressStorageSize = sizeof(senderAddressStorage);

		int bytesRecieved = recvfrom(this->serverSocket, buffer, 1024, 0, (struct sockaddr*)&senderAddressStorage, &senderAddressStorageSize);

		if (bytesRecieved == -1) { //recvfrom either failed or was shutdown by a call to closesocket()
			continue;
		}
		else { //Print sender information and buffered data
			std::cout << "Data recieved from " << getInternetAddress(&senderAddressStorage) << " on port " << ((struct sockaddr_in*)&senderAddressStorage)->sin_port << std::endl;
			std::cout << "Recieved data: " << buffer << std::endl;
		}

		//Process buffer
	}

	//Cleanup
	closesocket(this->serverSocket);
	WSACleanup();
	std::cout << "Server stopped" << std::endl;
}

SimpleUDPServer::SimpleUDPServer(unsigned int port) {
	this->port = port;
}

SimpleUDPServer::~SimpleUDPServer() {
	this->stopServer();
}

void SimpleUDPServer::startServer() noexcept {
	if (!this->isRunning) {
		this->isRunning = true;
		this->serverRunThread = std::thread(&SimpleUDPServer::runServer, this);
	}
}

void SimpleUDPServer::stopServer() noexcept {
	if (this->isRunning) {
		this->isRunning = false;

		//Closing the socket will make recvfrom() fail --> runServer() loop is able to exit
		//It is kinda allowed to ignore the return value of closesocket(), because it can be assumed, that the socket is already closed if it fails
		closesocket(this->serverSocket);
		this->serverRunThread.join();
	}
}
