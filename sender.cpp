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
#define SENDER_ADDR "127.0.0.1"//发送者的IP地址，从ipconfig里找到的
#define RECVER_PORT 4000//接收者的端口号
#define RECVER_ADDR "127.0.0.1"//接收者的IP地址
#define FILENAMELEN 50 //文件名的长度限制
//标志位宏，位数是按从低到高

#define FLAG_FEND 0x40//第7位 文件是否传输完毕
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
	char flag[2] = {0,0};//标志位
	short window;//接收窗口
	unsigned short checksum;//校验和
	char unknown[2] = { 0,0 };//待定
	char data[DATALENGTH] = {};//数据
	package() {
		seq = ack = window = checksum = 0;
	}
};
//一些全局变量
const int packageSize = sizeof(package);
unsigned int seq = 0;

//一些封装函数的申明
unsigned int checksum(const char* s, const int length);//差错检测
void sendAndWait(SOCKET s,const char* buf,int len,int flags,const sockaddr *to,int tolen);

//主函数
int main() {
	//1. 初始化Socket DLL
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) == 0)
		cout << "初始化Socket DLL成功！" << endl;
	else
		cout << "初始化Socket DLL失败！" << endl;

	//2. 定义发送端套接字及地址
	SOCKET sender_socket;
	SOCKADDR_IN recver_addr, sender_addr;
	recver_addr.sin_family = AF_INET;
	recver_addr.sin_addr.s_addr = inet_addr(RECVER_ADDR);
	recver_addr.sin_port = htons(RECVER_PORT);

	sender_addr.sin_family = AF_INET;
	sender_addr.sin_addr.s_addr = inet_addr(SENDER_ADDR);
	sender_addr.sin_port = htons(SENDER_PORT);

	int addr_len = sizeof(recver_addr);

	//3. 创建发送端数据报socket
	sender_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (sender_socket == INVALID_SOCKET) {
		cout << "创建sender_socket失败！" << endl;
		closesocket(sender_socket);
		WSACleanup();
		return -1;
	}

	//4. 绑定端口
	if (bind(sender_socket, (sockaddr*)&sender_addr, addr_len) == SOCKET_ERROR) {
		cout << "绑定端口失败！" << endl;
	}
	else {
		cout << "发送端IP地址：" << SENDER_ADDR << endl << "发送端绑定端口：" << SENDER_PORT << endl;
	}

	//5. 三次握手 先简化处理
	char handshake[packageSize] = "三次握手成功！";
	
	if (sendto(sender_socket, handshake, packageSize, 0, (sockaddr*)&recver_addr, addr_len) == SOCKET_ERROR) {
		cout << "三次握手失败！" << endl;
		closesocket(sender_socket);
		WSACleanup();
		return -1;
	}
	else {
		cout << "三次握手成功！" << endl;
	}

	//6. rdt2.2
	char filename[FILENAMELEN] = "temp//1.jpg";
	package *pac = new package();
	char buf[packageSize] = {};

	memcpy(pac->data, filename, FILENAMELEN);
	pac->seq = seq;
	memcpy(buf, pac, packageSize);
	pac->checksum = checksum(buf, packageSize);
	memcpy(buf, pac, packageSize);
	sendAndWait(sender_socket, buf, packageSize, 0, (sockaddr*)&recver_addr, addr_len);
	seq = (seq + 1) % 2;
	delete pac;
	pac = new package();

	ifstream reader;

	reader.open(filename, ios::binary);
	//int count = 0;
	while (reader.read(pac->data, DATALENGTH)) {
		memcpy(buf, pac, packageSize);
		//写入校验和以及序号
		pac->seq = seq;
		memcpy(buf, pac, packageSize);
		pac->checksum = checksum(buf, packageSize);
		memcpy(buf, pac, packageSize);
		sendAndWait(sender_socket, buf, packageSize, 0, (sockaddr*)&recver_addr, addr_len);
		delete pac;
		pac = new package();//更新pac指针
		seq = (seq + 1) % 2;//模2的前进


				////验证校验码
		//memcpy(buf, pac, packageSize);
		//pac->seq = 1;
		//memcpy(buf, pac, packageSize);
		//pac->checksum = checksum(buf, packageSize);
		//memcpy(buf, pac, packageSize);
		//cout << checksum(buf, packageSize) << endl;
	}
	memcpy(buf, pac, packageSize);
	//写入校验和以及序号
	pac->seq = seq;
	pac->flag[0] |=FLAG_FEND;
	memcpy(buf, pac, packageSize);
	pac->checksum = checksum(buf, packageSize);
	memcpy(buf, pac, packageSize);
	//发送这个包
	sendAndWait(sender_socket, buf, packageSize, 0, (sockaddr*)&recver_addr, addr_len);
	cout << "完成传输！" << endl;

	//清理资源占用
	delete pac;
	reader.close();
	closesocket(sender_socket);
	WSACleanup();
	return 0;
}

unsigned int checksum(const char* s, const int length) {
/*  输入：字符指针
*		  字符串长度
*	
*	输出：字符串的校验和
*/
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


void sendAndWait(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen) {
/*	功能：向指定目的地发送一个数据包，并等待正确的ACK,ACK确认后才能返回
*/
	SOCKADDR_IN from;
	sendto(s, buf, len, flags, to, tolen);
	//下面是等待ACK
	while (1) {
		//新建两个变量，用来承接收到的数据
		char* sbuf = new char[packageSize];
		package* recv_pac = new package();
		//接收返回的包
		recvfrom(s, sbuf, packageSize, 0, (sockaddr*)&from, &tolen);
		memcpy(recv_pac, sbuf, packageSize);
		//检查这个收到的包是否是受损的，以及ACK字段
		if ((checksum(sbuf, packageSize) != 0) || (((recv_pac->flag[0] & FLAG_ACK) == FLAG_ACK) && recv_pac->ack != seq)) {
			//如果接收到的包受损 或者 是冗余ACK 那么重发一次
			sendto(s, buf, len, flags, to, tolen);
			delete[]sbuf;
			delete recv_pac;
		}
		else if (((recv_pac->flag[0] & FLAG_ACK) == FLAG_ACK) && (recv_pac->ack == seq)) {
			//如果收到的是正确的ACK，那么说明发送的包到达接收方，可以发下一个包了
			delete[]sbuf;
			delete recv_pac;
			break;
		}
	}
}
