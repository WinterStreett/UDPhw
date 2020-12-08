#include<iostream>
#include<fstream>
#include<winsock.h>
#include<string.h>
#include<ctime>
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
#define TIMEOUT 1 //超时重传间隔
#define MISSLIMT 5//超时重传的最大次数限制

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
unsigned short fileCount = 0;


unsigned int checksum(const char* s, const int length);//差错检测
void sendACK(SOCKET s, bool redun, const sockaddr* to, int tolen);//发送一个ACK包
void recvOneFile(SOCKET s, sockaddr* to, int tolen);//接收一个文件
int sendAndWait(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen);//向一个目的地址发送一个数据包并等待ACK
void seqNext() {
	seq = (seq + 1) % 2;
}

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

	int len = sizeof(sender_addr);

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

	//5. 两次握手
	//接收端要先处于recv阻塞状态
	while (1) {
		package* pac = new package();
		char* buf = new char[packageSize];
		recvfrom(recver_socket, buf, packageSize, 0, (sockaddr*)&sender_addr, &len);
		memcpy(pac, buf, packageSize);
		if ((checksum(buf, packageSize) == 0) && (pac->flag[0] & FLAG_SYN) == FLAG_SYN)//完好的SYN
		{
			package* spac = new package();
			char* sbuf = new char[packageSize];
			spac->flag[0] |= FLAG_ACK;
			spac->flag[0] |= FLAG_SYN;
			spac->ack = seq;
			memcpy(sbuf, spac, packageSize);
			spac->checksum = checksum(sbuf, packageSize);
			memcpy(sbuf, spac, packageSize);
			sendAndWait(recver_socket, sbuf, packageSize, 0, (sockaddr*)&sender_addr, len);
			seqNext();
			delete spac;
			delete[]sbuf;
			delete pac;
			delete[]buf;
			break;
		}
		delete pac;
		delete[]buf;
	}
	cout << "三次握手成功！" << endl;

	 //6. rdt2.2接收端
	//第一个包包含文件名，单独识别
	recvOneFile(recver_socket, (sockaddr*)&sender_addr, len);
	recvOneFile(recver_socket, (sockaddr*)&sender_addr, len);
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
	seqNext();
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
		seqNext();
		delete pac;
		pac = new package();
	}
	delete pac;
	writer.close();
	cout << "成功接收第" << ++fileCount << "个文件!" << endl;
}

int sendAndWait(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen) {
	/*	功能：向指定目的地发送一个数据包，并等待正确的ACK,ACK确认后才能返回
	*/
	int missCount = 0;
	SOCKADDR_IN from;
	sendto(s, buf, len, flags, to, tolen);
	//下面是等待ACK
	while (1) {
		//新建两个变量，用来承接收到的数据
		char* sbuf = new char[packageSize];
		package* recv_pac = new package();
		//接收返回的包
		//使用select函数来达到计时效果
		struct timeval timeout = { TIMEOUT,0 };
		fd_set fdr;
		FD_ZERO(&fdr);
		FD_SET(s, &fdr);
		int ret = select(s, &fdr, NULL, NULL, &timeout);
		if (ret < 0)//发生错误
		{
			cout << "select发生错误！" << endl;
			delete[]sbuf;
			delete recv_pac;
			closesocket(s);
			WSACleanup();
			return -1;
		}
		else if (ret == 0)//等待超时
		{
			sendto(s, buf, len, flags, to, tolen);//重发
			missCount++;
			cout << "第" << missCount << "次超时！" << endl;
			if (missCount == MISSLIMT) {
				delete[]sbuf;
				delete recv_pac;
				cout << "超时次数到达上限，发送失败！" << endl;
				exit(-1);
			}
		}
		else {
			recvfrom(s, sbuf, packageSize, 0, (sockaddr*)&from, &tolen);
			memcpy(recv_pac, sbuf, packageSize);
			//检查这个收到的包是否是受损的，以及ACK字段
			if ((checksum(sbuf, packageSize) != 0) || (((recv_pac->flag[0] & FLAG_ACK) == FLAG_ACK) && recv_pac->ack != seq)) {
				//如果接收到的包受损 或者 是冗余ACK 那么重发一次
				sendto(s, buf, len, flags, to, tolen);
			}
			else if (((recv_pac->flag[0] & FLAG_ACK) == FLAG_ACK) && (recv_pac->ack == seq)) {
				//如果收到的是正确的ACK，那么说明发送的包到达接收方，可以发下一个包了
				break;
			}
		}
		delete[]sbuf;
		delete recv_pac;
	}
}
