#include<iostream>
#include<fstream>
#include<winsock.h>
#include<string.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
#pragma warning(disable:4996)

#define HEADLENGTH 16//报文头部的长度
#define DATALENGTH 1464//数据包中的有效数据部分的长度
#define SENDER_PORT 4001//发送者的端口号
#define SENDER_ADDR "127.0.0.1"//发送者的IP地址
#define RECVER_PORT 4000//接收者的端口号
#define RECVER_ADDR "127.0.0.1"//接收者的IP地址
#define FILENAMELEN 50 //文件名的长度限制


//标志位宏，位数是按从低到高
#define FLAG_FEND 0x40//第7位
#define FLAG_ACK 0x10//第5位
#define FLAG_SYN 0x2//第2位
#define FLAG_FIN Ox1//第1位
/*
* 报文头结构
*												<------------4字节----------->
*											0	|************序号************|
*											4	|***********确认号***********|
*											8	|****标志位****|**接收窗口***|
*															   10
*											12	|****校验和****|****待定*****|    一共16字节的头部信息
*															   14
*/

struct package {
	unsigned int seq;//序号
	unsigned int ack;//确认号
	char flag[2] = { 0,0 };//标志位
	short window;//接收窗口
	unsigned short checksum;//校验和
	char unknown[2] = { 0,0 };//待定
	char data[DATALENGTH] = {};//数据
	package() {
		seq = ack = window = checksum = 0;
	}
};
const int packageSize = sizeof(package);
unsigned int seq = 0;//记录自己想要收到的下一个包的序号


unsigned int checksum(const char* s, const int length);//差错检测
void sendACK(SOCKET s, bool redun, const sockaddr* to, int tolen);//发送一个ACK包
void recvOneFile(SOCKET s, sockaddr* to, int tolen);//接收一个文件

int main() {
	//1. 初始化Socket DLL
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) == 0)
		cout << "初始化Socket DLL成功！" << endl;
	else
		cout << "初始化Socket DLL失败！" << endl;

	//2. 定义接收端套接字及地址
	SOCKET recver_socket;
	SOCKADDR_IN recver_addr, sender_addr;
	recver_addr.sin_family = AF_INET;
	recver_addr.sin_addr.s_addr = inet_addr(RECVER_ADDR);
	recver_addr.sin_port = htons(RECVER_PORT);

	//3. 创建接收端数据报socket
	recver_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (recver_socket == INVALID_SOCKET) {
		cout << "创建recver_socket失败！" << endl;
		closesocket(recver_socket);
		WSACleanup();
		return -1;
	}

	//4. 绑定端口
	if (bind(recver_socket, (sockaddr*)&recver_addr, sizeof(recver_addr)) == SOCKET_ERROR) {
		cout << "server_socket绑定端口失败！" << endl;
		closesocket(recver_socket);
		WSACleanup();
		return -1;
	}
	else {
		cout << "接收端IP地址：" << RECVER_ADDR << endl << "接收端绑定端口：" << RECVER_PORT << endl;
	}

	//5. 简化版三次握手
	int len = sizeof(sender_addr);
	char handshake[1500] = {};
	if (recvfrom(recver_socket, handshake, packageSize, 0, (sockaddr*)&sender_addr, &len) == SOCKET_ERROR) {
		cout << "三次握手失败！" << endl;
		closesocket(recver_socket);
		WSACleanup();
		return -1;
	}
	else {
		cout << handshake << endl;
	}

	 //6. rdt2.2接收端
	//第一个包包含文件名，单独识别
	recvOneFile(recver_socket, (sockaddr*)&sender_addr, len);
	recvOneFile(recver_socket, (sockaddr*)&sender_addr, len);


	closesocket(recver_socket);
	WSACleanup();
	return 0;
}

unsigned int checksum(const char* s, const int length) {
	unsigned int sum = 0;
	unsigned short temp1 = 0;//这里必须是无符号数，否则会因为符号扩展，得到一些奇奇怪怪的数
	unsigned short temp2 = 0;
	for (int i = 0; i < length; i += 2) {
		temp1 = s[i + 1] << 8;
		temp2 = s[i] & 0x00ff;//一定要把高8位置零！否则如果低8位最高位是1，那么高8位也会因为符号扩展而全为1
		sum += temp1 | temp2;
	}
	while (sum > 0xffff) {//把加法溢出的部分（存在高16位）加到低16位上
		sum = (sum & 0xffff) + (sum >> 16);
	}
	return ~(sum | 0xffff0000);//低16位取反，高16位全为0
}

void sendACK(SOCKET s,bool redun, const sockaddr* to, int tolen) {
/*	功能：向目的地址发送一个ACK  
*	redun：为真时，说明要发送的是一个冗余ACK，seq值不能变
*/
	char* sbuf = new char[packageSize];
	package* ack_pac = new package();
	ack_pac->flag[0] |= FLAG_ACK;//设置ACK标志位
	if(redun)
		ack_pac->ack = (seq + 1) % 2;
	else
		ack_pac->ack = seq;
	memcpy(sbuf, ack_pac, packageSize);
	ack_pac->checksum = checksum(sbuf, packageSize);//填上校验和
	memcpy(sbuf, ack_pac, packageSize);
	sendto(s, sbuf, packageSize, 0, to, tolen);
	delete[]sbuf;
	delete ack_pac;
}

void recvOneFile(SOCKET s, sockaddr* to, int tolen) {
	char buf[packageSize] = {};
	package* pac = new package();
	char filename[FILENAMELEN] = {};
	//从源地址接收文件名数据包
	recvfrom(s, buf, packageSize, 0, to, &tolen);
	memcpy(pac, buf, packageSize);
	//赋值文件名
	memcpy(filename, pac->data, FILENAMELEN);

	ofstream writer;
	writer.open(filename, ios::binary);//打开一个文件

	sendACK(s, false, to, tolen);//发送ACK
	seq = (seq + 1) % 2;
	//int count = 0;
	while (1) {
		recvfrom(s, buf, packageSize, 0, to, &tolen);
		memcpy(pac, buf, packageSize);
		if ((checksum(buf, packageSize) != 0) || (pac->seq != seq))//如果这个包被损坏了，或者不是接收方想要的包
		{
			//重发ACK包
			sendACK(s, true, to, tolen);
		}
		else if (pac->seq == seq) {//如果接收到了正确的包，写入文件返回正确的ACK
			//cout << ++count << endl;
			writer.write(pac->data, DATALENGTH);
			sendACK(s, false, to, tolen);
			if ((pac->flag[0] & FLAG_FEND) == FLAG_FEND)//如果接收到的是文件末尾，退出
				break;
		}
		seq = (seq + 1) % 2;
		delete pac;
		pac = new package();
	}
	delete pac;
	writer.close();
}