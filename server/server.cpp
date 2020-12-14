#include"rdt.h"
using namespace std;

const int packageSize = sizeof(package);
unsigned int ack = 0;//记录自己想要收到的下一个包的序号
char RECVER_ADDR[20] = {};//服务器端地址
unsigned short  RECVER_PORT;//服务器端端口

int main() {
	//1. 初始化Socket DLL
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) == 0)
		cout << "初始化Socket DLL成功！" << endl;
	else
		cout << "初始化Socket DLL失败！" << endl;

	//2. 定义接收端套接字及地址
	SOCKET server_socket;
	SOCKADDR_IN server_addr, client_addr;
	cout << "请输入接收端IP地址：";
	cin >> RECVER_ADDR;
	cout << "请输入接收端端口号：";
	cin >> RECVER_PORT;
	cout << endl;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(RECVER_ADDR);
	server_addr.sin_port = htons(RECVER_PORT);

	int len = sizeof(client_addr);

	//3. 创建接收端数据报socket
	server_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (server_socket == INVALID_SOCKET) {
		cout << "创建server_socket失败！" << endl;
		goto ERR;

	}

	//4. 绑定端口
	if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		cout << "server_socket绑定端口失败！" << endl;
		goto ERR;

	}
	else {
		cout << "接收端IP地址：" << RECVER_ADDR << endl << "接收端绑定端口：" << RECVER_PORT << endl;
	}

	//5. 两次握手
	//接收端要先处于recv阻塞状态
	while (1) {
		package* pac = new package();
		recvfrom(server_socket, (char*)pac, packageSize, 0, (sockaddr*)&client_addr, &len);

		if ((checksum((char*)pac, packageSize) == 0) && (pac->flag[0] & FLAG_SYN) == FLAG_SYN)//完好的SYN
		{
			package* spac = new package();
			spac->flag[0] |= FLAG_ACK;
			spac->flag[0] |= FLAG_SYN;
			ack = pac->seq;//将接收方的ack设为发送方的seq
			spac->ack = ack;
			spac->checksum = checksum((char*)spac, packageSize);

			sendto(server_socket, (char*)spac, packageSize, 0, (sockaddr*)&client_addr, len);//第二次握手
			delete spac;
			delete pac;
			break;
		}
		delete pac;
	}
	cout << "两次握手成功！" << endl;
	cout << endl;

	//接收文件循环
	for (int i = 0; i < 4; i++) {
		if (recvOneFile(server_socket, (sockaddr*)&client_addr, len) < 0) {
			goto ERR;
		}
	}

CL:	closesocket(server_socket);
	WSACleanup();
	system("pause");
 	return 0;

ERR:
	closesocket(server_socket);
	WSACleanup();
	system("pause");
	return -1;
}