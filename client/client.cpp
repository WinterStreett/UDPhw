//头文件和必要的编译指令
#include"rdt.h"
using namespace std;

//全局变量
const int packageSize = sizeof(package);
unsigned short WINDOW;
unsigned int seq = 0;

char SENDER_ADDR[20] = {};//发送者的IP地址
char RECVER_ADDR[20] = {};//接收者的IP地址
unsigned int SENDER_PORT; //发送者的端口号
unsigned int RECVER_PORT; //接收者的端口号

int main() {
	char filename[FILENAMELEN] = "temp//1.jpg";
	char filename1[FILENAMELEN] = "temp//2.jpg";
	char filename2[FILENAMELEN] = "temp//3.jpg";
	char filename3[FILENAMELEN] = "temp//helloworld.txt";

	//1. 初始化Socket DLL
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) == 0)
		cout << "初始化Socket DLL成功！" << endl;
	else
		cout << "初始化Socket DLL失败！" << endl;

	//2. 定义客户端套接字及地址
	cout << "请输入发送端IP地址：";
	cin >> SENDER_ADDR;
	cout << "请输入发送端端口号：";
	cin >> SENDER_PORT;
	cout << "请输入路由器IP地址：";
	cin >> RECVER_ADDR;
	cout << "请输入路由器的端口号：";
	cin >> RECVER_PORT;
	cout << endl;

	SOCKET client_socket;
	SOCKADDR_IN server_addr, client_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(RECVER_ADDR);
	server_addr.sin_port = htons(RECVER_PORT);

	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = inet_addr(SENDER_ADDR);
	client_addr.sin_port = htons(SENDER_PORT);



	int addr_len = sizeof(server_addr);
	package* pac = new package();
	int ret;


	//3. 创建客户端数据报socket
	client_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (client_socket == INVALID_SOCKET) {
		cout << "创建client_socket失败！" << endl;
		closesocket(client_socket);
		WSACleanup();
		return -1;
	}

	//4. 绑定端口
	if (bind(client_socket, (sockaddr*)&client_addr, addr_len) == SOCKET_ERROR) {
		cout << "绑定端口失败！" << endl;
		goto CLEAN;
	}
	else {
		cout << "发送端IP地址：" << SENDER_ADDR << endl << "发送端绑定端口：" << SENDER_PORT << endl;
	}

	//5. 两次握手
	cout << "设定窗口大小:";
	cin >> WINDOW;
	cout << endl;

	cout << "开始握手" << endl;
	pac->seq = 0;
	pac->flag[0] |= FLAG_SYN;

	pac->checksum = checksum((char*)pac, packageSize);
	//发送第一个包,sendAndWait里面应该已经接收了服务器返回的第二次握手ACK包
	ret = sendAndWait(client_socket, (char*)pac, packageSize, 1, (sockaddr*)&server_addr, addr_len);//发送失败的话会尝试几次，直到最大次数限制
	if (ret < 0)
		goto CLEAN;
	cout << "两次握手完成！" << endl;

	cout << endl;
	//6. 3-2

	//if (sendOneFile(client_socket, filename, (sockaddr*)&server_addr, addr_len)<0) {
	//	cout << "发送文件 " << filename << " 失败！" << endl;
	//}
	if (sendOneFile(client_socket, filename, (sockaddr*)&server_addr, addr_len)<0) {
		cout << "发送文件 " << filename << " 失败！" << endl;
	}
	if (sendOneFile(client_socket, filename1, (sockaddr*)&server_addr, addr_len) < 0) {
		cout << "发送文件 " << filename1 << " 失败！" << endl;
	}
	if (sendOneFile(client_socket, filename2, (sockaddr*)&server_addr, addr_len) < 0) {
		cout << "发送文件 " << filename2 << " 失败！" << endl;
	}
	if (sendOneFile(client_socket, filename3, (sockaddr*)&server_addr, addr_len) < 0) {
		cout << "发送文件 " << filename3 << " 失败！" << endl;
	}

CLEAN:
	delete pac;
	closesocket(client_socket);
	WSACleanup();
	system("pause");
	return 0;
}