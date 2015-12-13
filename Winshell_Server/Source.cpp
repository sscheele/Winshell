#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <winsock2.h>

int main(){
	WORD wVersionRequested;
	WSADATA wsaData;
	int wsaerr;
	
	wVersionRequested = MAKEWORD(2, 2);
	wsaerr = WSAStartup(wVersionRequested, &wsaData);
	if (wsaerr != 0){
		printf("Server: The Winsock dll not found!\n");
		return 0;
	}
	else{
		printf("Server: The Winsock dll found!\n");
		printf("Server: The status: %s.\n", wsaData.szSystemStatus);
	}                                        

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2){
		printf("Server: The dll do not support the Winsock version %u.%u!\n", LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion));
		WSACleanup();
		return 0;
	}
	else{
		printf("Server: The dll supports the Winsock version %u.%u!\n", LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion));
		printf("Server: The highest version this dll can support: %u.%u\n", LOBYTE(wsaData.wHighVersion), HIBYTE(wsaData.wHighVersion));
	}

	SOCKET m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (m_socket == INVALID_SOCKET){
		printf("Server: Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return 0;
	} else{
		printf("Server: socket() is OK!\n");
	}

	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = inet_addr("127.0.0.1");
	service.sin_port = htons(55555);

	if (bind(m_socket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR){
		printf("Server: bind() failed: %ld.\n", WSAGetLastError());
		closesocket(m_socket);
		return 0;
	} else{
		printf("Server: bind() is OK!\n");
	}

	if (listen(m_socket, 10) == SOCKET_ERROR)
		printf("Server: listen(): Error listening on socket %ld.\n", WSAGetLastError());
	else{
		printf("Server: listen() is OK, I'm waiting for connections...\n");
	}

	SOCKET AcceptSocket;

	printf("Server: Waiting for a client to connect...\n");

	while (1){
		AcceptSocket = SOCKET_ERROR;
		while (AcceptSocket == SOCKET_ERROR){
			AcceptSocket = accept(m_socket, NULL, NULL);
		}
		printf("Server: Client Connected!\n");
		m_socket = AcceptSocket;
		break;
	}

	int bytesSent;
	int bytesRecv = SOCKET_ERROR;
	char sendbuf[16000] = "dir";
	char recvbuf[16000] = "";

	while (1) {
		bytesRecv = recv(m_socket, recvbuf, 16000, 0);
		if (bytesRecv == SOCKET_ERROR)
			printf("Server: recv() error %ld.\n", WSAGetLastError());
		else {
			printf(recvbuf);
		}
		fgets(sendbuf, 16000, stdin);
		bytesSent = send(m_socket, sendbuf, strlen(sendbuf), 0);
		if (bytesSent == SOCKET_ERROR)
			printf("Server: send() error %ld.\n", WSAGetLastError());
		else {
			printf("Server: send() is OK.\n");
			printf("Server: Bytes Sent: %ld.\n", bytesSent);
		}
	}
	WSACleanup();
	return 0;
}