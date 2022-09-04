//Simple UDP server for Windows

#include "simple_tcp_server.h"

#include <iostream>
#include <stdexcept>

std::string SimpleTCPServer::getInternetAddress(struct sockaddr_storage* clientAddressStorage) noexcept {
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

void SimpleTCPServer::runServer() {
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

		struct addrinfo* serverinfo = nullptr, * temp = nullptr, setup;
		ZeroMemory(&setup, sizeof(setup));
		setup.ai_family = AF_UNSPEC; //Specifies the use of IPv4 or IPv6 or in this case, both
		setup.ai_socktype = SOCK_STREAM; //Specifies the use of a TCP stream socket
		setup.ai_flags = AI_PASSIVE; //Tells getaddrinfo() to automatically fill in the IP

		//getaddrinfo() automatically fills serverinfo with all information needed
		if (getaddrinfo(NULL, std::to_string(port).c_str(), &setup, &serverinfo) != 0) {
			throw std::exception("getaddrinfo() failed");
		}

		for (temp = serverinfo; temp != NULL; temp = temp->ai_next) {
			this->serverMainSocket = socket(temp->ai_family, temp->ai_socktype, temp->ai_protocol);
			if (this->serverMainSocket == -1) {
				continue;
			}
			char const *yes = "1";
			//Gets rid of "address already in use" error
			if (setsockopt(this->serverMainSocket, SOL_SOCKET, SO_REUSEADDR, yes, static_cast<int>(strlen(yes))) == SOCKET_ERROR) {
				throw std::exception("setsockopt() failed");
			}
			if (bind(this->serverMainSocket, temp->ai_addr, static_cast<int>(temp->ai_addrlen)) == -1) {
				closesocket(this->serverMainSocket);
				continue;
			}
			break;
		}
		if (temp == NULL) {
			throw std::exception("Failed to bind");
		}
		freeaddrinfo(serverinfo);

		if (listen(this->serverMainSocket, SOMAXCONN) == SOCKET_ERROR) {
			throw std::exception("listen() failed");
		}

	}
	catch (std::exception& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		this->isRunning = false;
		closesocket(this->serverMainSocket);
		WSACleanup();
		std::terminate();
	}

	FD_ZERO(&masterFDSet);
	fd_set tempFDSet;
	FD_ZERO(&tempFDSet);
	
	FD_SET(this->serverMainSocket, &masterFDSet);
	
	std::cout << "Server started" << std::endl;

	while (this->isRunning) {
		tempFDSet = masterFDSet;
		if (select(tempFDSet.fd_count+1, &tempFDSet, NULL, NULL, NULL) == SOCKET_ERROR) {
			std::cout << "ERROR: select() failed" << std::endl;
			continue;
		}

		for (SOCKET clientSocket : tempFDSet.fd_array) {
			if (clientSocket == this->serverMainSocket) { //New connection incoming
				struct sockaddr_storage clientAddressStorage;
				socklen_t clientAddressStorageSize = sizeof(clientAddressStorage);
				//Accept client and create a socket for the new client
				SOCKET newClientSocket = accept(this->serverMainSocket, (struct sockaddr *) &clientAddressStorage, &clientAddressStorageSize);
				if (newClientSocket != SOCKET_ERROR) {
					//Add new client to the master set
					FD_SET(newClientSocket, &masterFDSet);
					std::cout << "New Connection recieved from " << getInternetAddress(&clientAddressStorage) << " on port " << ((struct sockaddr_in*)&clientAddressStorage)->sin_port << std::endl;
				}
			}
			else { //Recieving data from active connection
				char buffer[BUFFERSIZE];
				int bytesRecieved = recv(clientSocket, buffer, 1024, 0);
			
				if (bytesRecieved <= 0) { //recv() either failed or the client closed the connection
					closesocket(clientSocket);
					//remove client from the master set
					FD_CLR(clientSocket, &masterFDSet);
				}
				else {
					struct sockaddr_storage clientAddressStorage;
					socklen_t clientAddressStorageSize = sizeof(clientAddressStorage);
					
					//getpeername() fills in the struct clientAddressStorage, which is then passed to getInternetAddress to obtain the IP Address of the connected client
					if (getpeername(clientSocket, (struct sockaddr*)&clientAddressStorage, &clientAddressStorageSize) != SOCKET_ERROR) {
						std::cout << "Data recieved from " << getInternetAddress(&clientAddressStorage) << " on port " << ((struct sockaddr_in*)&clientAddressStorage)->sin_port << std::endl;
					}
					std::cout << "Recieved data: " << buffer << std::endl;
					
					//Process buffer
				}
			}
		}
	}

	//Cleanup
	closesocket(this->serverMainSocket);
	WSACleanup();
	std::cout << "Server stopped" << std::endl;
}

SimpleTCPServer::SimpleTCPServer(unsigned int port) {
	this->port = port;
	FD_ZERO(&masterFDSet);
}

SimpleTCPServer::~SimpleTCPServer() {
	this->stopServer();
}

void SimpleTCPServer::startServer() noexcept {
	if (!this->isRunning) {
		this->isRunning = true;
		this->serverRunThread = std::thread(&SimpleTCPServer::runServer, this);
	}
}

void SimpleTCPServer::stopServer() noexcept {
	if (this->isRunning) {
		this->isRunning = false;
		//Make sure to close all active connections before closing the serverMainSocket
		for (SOCKET clientSocket : masterFDSet.fd_array) {
			closesocket(clientSocket);
		}
		closesocket(this->serverMainSocket);
		this->serverRunThread.join();
	}
}
