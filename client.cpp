#include<iostream>
#include<winsock.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
#pragma warning(disable:4996)

unsigned int sport = 4000,cport = 4001;

int main() {
	//1. 初始化Socket DLL
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) == 0)
		cout << "初始化Socket DLL成功！" << endl;
	else
		cout << "初始化Socket DLL失败！" << endl;

	//2. 定义客户端主套接字及服务器地址
	SOCKET client_socket;
	SOCKADDR_IN server_addr, client_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(sport);

	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	client_addr.sin_port = htons(cport);

	//3. 创建客户端端数据报socket
	client_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (client_socket == INVALID_SOCKET) {
		cout << "创建client_socket失败！" << endl;
		closesocket(client_socket);
		WSACleanup();
		return -1;
	}
	
	//4. 绑定端口
	int len = sizeof(server_addr);

	if (bind(client_socket, (sockaddr*)&client_addr, sizeof(client_addr)) == SOCKET_ERROR) {
		cout<<"绑定端口失败！"<<endl;
	}

	//4. 测试
	while (1) {
		char sbuf[256] = {};
		cin.getline(sbuf, 256);
		if (sendto(client_socket, sbuf, 256, 0, (sockaddr*)&server_addr, len) == SOCKET_ERROR) {
			cout << "发送失败！" << endl;
		}
		char buf[256] = {};
		if (recvfrom(client_socket, buf, 256, 0, (sockaddr*)&server_addr, &len) == SOCKET_ERROR) {
			cout << "接收数据错误！" << endl;
		}
		else {
			cout << buf << endl;
		}
	}
	closesocket(client_socket);
	WSACleanup();
	return 0;
}