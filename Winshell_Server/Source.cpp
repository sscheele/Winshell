#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SSIZE_T ssize_t;

static const int BUFFER_SIZE = 16 * 1024;

SOCKET m_socket;

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

	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

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
		printf("Server: listen() is OK, waiting for connections...\n");
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
			printf("Server: Bytes Sent: %ld.\n", bytesSent);
		}
	}
	WSACleanup();
	return 0;
}

static void SendFile(const char * fileName)
{
	FILE * fpIn = fopen(fileName, "r");
	char buf[BUFFER_SIZE];
	while (1)
	{
		ssize_t bytesRead = fread(buf, 1, sizeof(buf), fpIn);
		if (bytesRead <= 0) break;  // EOF
		send(m_socket, buf, bytesRead, 0);
	}
}

int ReceiveFile(int port, const char * fileName)
{
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s >= 0)
	{
#ifndef WIN32
		// (Not necessary under windows -- it has the behaviour we want by default)
		const int trueValue = 1;
		(void)setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &trueValue, sizeof(trueValue));
#endif

		struct sockaddr_in saAddr; memset(&saAddr, 0, sizeof(saAddr));
		saAddr.sin_family = AF_INET;
		saAddr.sin_addr.s_addr = htonl(0);  // (IPADDR_ANY)
		saAddr.sin_port = htons(port);

		if ((bind(s, (struct sockaddr *) &saAddr, sizeof(saAddr)) == 0) && (listen(s, 10) == 0))
		{
			memset(&saAddr, 0, sizeof(saAddr));
			socklen_t len = sizeof(saAddr);
			int connectSocket = accept(s, (struct sockaddr *) &saAddr, &len);
			if (connectSocket >= 0)
			{
				FILE * fpIn = fopen(fileName, "w");
				if (fpIn)
				{
					char buf[BUFFER_SIZE];
					while (1)
					{
						ssize_t bytesReceived = recv(connectSocket, buf, sizeof(buf), 0);
						if (bytesReceived < 0) return 1;  // network error?
						if (bytesReceived == 0) break;   // sender closed connection, must be end of file

						if (fwrite(buf, 1, bytesReceived, fpIn) != (size_t)bytesReceived)
						{
							return 2; //file write error
							break;
						}
					}

					fclose(fpIn);
				}
				else return 2; //couldn't open file for writing
				closesocket(connectSocket);
			}
		}
		else return 3; //socket bind error
		closesocket(s);
		return 0; //success
	}
	else return 4; //socket error
}