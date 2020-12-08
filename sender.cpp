#include<iostream>
#include<fstream>
#include<winsock.h>
#include<string.h>
#include<ctime>
#include<iomanip>
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
#define TIMEOUTS 0 //超时重传间隔 单位：秒
#define TIMEOUTMS 500000 //超时间隔 单位：微秒
#define MISSLIMT 5//超时重传的最大次数限制

//标志位宏，位数是按从低到高

#define FLAG_FHEAD 0x80 //第8位 文件名包
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
	unsigned short window;//接收窗口
	unsigned short checksum;//校验和
	unsigned short taillen;
	char data[DATALENGTH] = {};//数据
	package() {
		seq = ack = window = checksum = taillen = 0;
	}
};
//一些全局变量
const int packageSize = sizeof(package);
unsigned int seq = 0;
unsigned  short fileCount = 0;

//一些封装函数的申明
unsigned int checksum(const char* s, const int length);//差错检测
int sendAndWait(SOCKET s,const char* buf,int len,int flags,const sockaddr *to,int tolen);//向一个目的地址发送一个数据包并等待ACK
void sendOneFile(SOCKET s, char* filename, const sockaddr* to, int tolen);//向一个目的地址发送一个完整的文件
void Throughput(unsigned int pacs, clock_t time);//计算吞吐率并输出
void seqNext() {
	seq = (seq + 1) % 2;
}
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

	//5. 两次握手 
	//第一次握手
	package* pac = new package();
	pac->seq = seq;
	pac->flag[0] |= FLAG_SYN;

	pac->checksum = checksum((char*)pac, packageSize);
	//发送第一个包,sendAndWait里面应该已经接收了服务器返回的第二次握手ACK包
	sendAndWait(sender_socket, (char*)pac, packageSize, 0, (sockaddr*)&recver_addr, addr_len);//发送失败的话会尝试几次，直到最大次数限制
	cout << "两次握手完成！" << endl;

	cout << "两次握手后，reader的seq=" << seq << endl;
	
	//6. rdt2.2
	char filename1[FILENAMELEN] = "temp//1.jpg";
	char filename2[FILENAMELEN] = "temp//2.jpg";
	char filename3[FILENAMELEN] = "temp//3.jpg";
	char filename4[FILENAMELEN] = "temp//helloworld.txt";
	sendOneFile(sender_socket, filename1, (sockaddr*)&recver_addr, addr_len);
	sendOneFile(sender_socket, filename2, (sockaddr*)&recver_addr, addr_len);
	sendOneFile(sender_socket, filename3, (sockaddr*)&recver_addr, addr_len);
	sendOneFile(sender_socket, filename4, (sockaddr*)&recver_addr, addr_len);

	//清理资源占用
	delete pac;
	closesocket(sender_socket);
	WSACleanup();
	system("pause");
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


int sendAndWait(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen) {
/*	功能：向指定目的地发送一个数据包，并等待正确的ACK,ACK确认后才能返回
*/
	int missCount = 0;
	SOCKADDR_IN from;
	sendto(s, buf, len, flags, to, tolen);
	struct timeval timeout = { TIMEOUTS,TIMEOUTMS };
	fd_set fdr;
	FD_ZERO(&fdr);
	FD_SET(s, &fdr);
	int ret;
	//下面是等待ACK
	while (1) {
		//新建两个变量，用来承接收到的数据
		package* recv_pac = new package();
		ret = select(s, &fdr, NULL, NULL, &timeout);
		if (ret < 0)//发生错误
		{
			cout << "select发生错误！" << endl;
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
				delete recv_pac;
				//cout << "超时次数到达上限，发送失败！" << endl;
				exit(-1);
			}
		}
		else {
			recvfrom(s, (char*)recv_pac, packageSize, 0, (sockaddr*)&from, &tolen);
			//检查这个收到的包是否是受损的，以及ACK字段
			if ((checksum((char*)recv_pac, packageSize) != 0) || (((recv_pac->flag[0] & FLAG_ACK) == FLAG_ACK) && recv_pac->ack != seq)) {
				//如果接收到的包受损 或者 是冗余ACK 那么重发一次
				sendto(s, buf, len, flags, to, tolen);
			}
			else if (((recv_pac->flag[0] & FLAG_ACK) == FLAG_ACK) && (recv_pac->ack == seq)) {
				//如果收到的是正确的ACK，那么说明发送的包到达接收方，可以发下一个包了
				break;
			}
		}
		delete recv_pac;
	}
}


void sendOneFile(SOCKET s, char* filename, const sockaddr* to, int tolen) {
/*	功能：向目的地址，发送一个完整的文件
*/
	unsigned int pacCount = 0;//记录发送的数据包个数
	clock_t start, end;//计时器
	package* pac = new package();
	unsigned int filelen;//文件长度
	unsigned int pacNum;//需要的包总数量
	unsigned short taillen;
	char* file;//保存文件的数组

	//读取整个文件
	ifstream reader;
	reader.open(filename, ios::binary);
	reader.seekg(0, ios::end);
	filelen = reader.tellg();
	reader.seekg(0, ios::beg);
	file = new char[filelen];//file中保存着文件数据
	reader.read(file, filelen);
	pacNum = filelen / DATALENGTH;//可能有尾部
	taillen = filelen % DATALENGTH;//尾部剩余长度

	//首先将文件名发给接收端
	memcpy(pac->data, &filelen, 4);//数据部分的前4个字节是文件长度
	memcpy(pac->data+4, filename, FILENAMELEN);//数据部分从4开始是文件名
	pac->seq = seq;
	pac->flag[0] |= FLAG_FHEAD;
	pac->checksum = checksum((char*)pac, packageSize);
	start = clock();
	sendAndWait(s, (char*)pac, packageSize, 0, to, tolen);
	//发送文件名成功！开始读取文件
	seqNext();
	delete pac;
	pac = new package();


	unsigned int di;
	for (int i = 0; i < pacNum; i++)
	{
		//从file中读取一块数据，长度为DATALENGTH
		di = i * DATALENGTH;
		memcpy(pac->data, file + di, DATALENGTH);
		//将数据包设置好
		pac->seq = seq;
		pac->checksum = checksum((char*)pac, packageSize);
		//发送数据包，等待确认
		sendAndWait(s, (char*)pac, packageSize, 0, to, tolen);
		delete pac;
		pac = new package();//更新pac指针
		seqNext();//模2的前进
		pacCount++;//数据包数量加一
	}
	//处理最后一小段
	int a = filelen - taillen;
	memcpy(pac->data,file+ a, taillen);
	pac->seq = seq;
	pac->flag[0] |= FLAG_FEND;//将FEND标志位置位
	pac->taillen = taillen;
	pac->checksum = checksum((char*)pac, packageSize);
	sendAndWait(s, (char*)pac, packageSize, 0, to, tolen);
	end = clock();
	seqNext();//模2的前进
	pacCount++;//数据包数量加一

	cout << "成功发送第"<<++fileCount<<"个文件！" << endl;
	cout << "用时：" << end - start << "毫秒" << "	" << "吞吐率：";
	Throughput(pacCount, end - start);
	//清理资源占用
	delete[]file;
	delete pac;
	reader.close();
	//cout << "seq:" << seq << endl;
}

void Throughput(unsigned int pacs, clock_t time) {//计算吞吐率，自动补上单位输出
	double result = (pacs * DATALENGTH) * 8000 / time;
	if (result > 1000000000) {
		result /= 1000000000;
		cout <<fixed<< setprecision(2) << result << " Gb/s" << endl;
	}
	else if (result > 1000000) {
		result /= 1000000;
		cout << fixed << setprecision(2) << result << " Mb/s" << endl;
	}
	else if (result > 1000) {
		result /= 1000;
		cout << fixed << setprecision(2) << result << " Kb/s" << endl;
	}
	else {
		cout<< fixed << setprecision(2) << result << " b/s" << endl;
	}
	cout << endl;
}