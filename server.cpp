#include<iostream>
#include<winsock.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
#pragma warning(disable:4996)

unsigned int sport = 4000;

int main() {
	//1. 初始化Socket DLL
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) == 0)
		cout << "初始化Socket DLL成功！" << endl;
	else
		cout << "初始化Socket DLL失败！" << endl;

	//2. 定义服务器主套接字及服务器地址
	SOCKET server_socket;
	SOCKADDR_IN server_addr,client_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(sport);

	//3. 创建服务器端数据报socket
	server_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (server_socket == INVALID_SOCKET) {
		cout << "创建server_socket失败！" << endl;
		closesocket(server_socket);
		WSACleanup();
		return -1;
	}

	//4. 绑定端口
	if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		cout << "server_socket绑定端口失败！" << endl;
		closesocket(server_socket);
		WSACleanup();
		return -1;
	}
	cout << "开始测试" << endl;

	//5. 先测试一下
	int len = sizeof(client_addr);
	while (1) {
		char buf[256] = {};
		if (recvfrom(server_socket, buf, 256, 0, (sockaddr*)&client_addr, &len) == SOCKET_ERROR) {
			cout << "接收数据出错！" << endl;
		}
		else {
			cout << buf << endl;
		}
		char sbuf[256] = {};
		cin.getline(sbuf, 256);
		sendto(server_socket, sbuf, 256, 0, (sockaddr*)&client_addr, len);
	}
	closesocket(server_socket);
	WSACleanup();
	return 0;
}