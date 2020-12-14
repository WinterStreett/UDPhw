#pragma once
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

//#define SENDER_PORT 4001//发送者的端口号
//#define SENDER_ADDR "127.0.0.1"//发送者的IP地址
//
//#define RECVER_PORT 4000//接收者的端口号
//#define RECVER_ADDR "127.0.0.1"//接收者的IP地址

#define FILENAMELEN 50 //文件名的长度限制
#define TIMEOUTS 0//超时重传间隔 秒
#define TIMEOUTVS 300//超时重传间隔 微秒

#define MISSLIMT 5//超时重传的最大次数限制

//标志位宏，位数是按从低到高
#define FLAG_FHEAD 0x80 //第8位 文件名包
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
	unsigned short window;//接收窗口
	unsigned short checksum;//校验和
	unsigned short taillen;
	char data[DATALENGTH] = {};//数据
	package() {
		seq = ack = window = checksum = taillen = 0;
	}
};

struct cacheManager {
	unsigned int base;
	unsigned short window;
	bool* indexBuf;
	unsigned int getNextBase();//获取下一个base，找到buf中满足累计确认的序号最大值
	void insertSeq(unsigned int seq);//插入一个序号
	void initBuf();//初始化缓存区
	cacheManager(unsigned int Base, unsigned short Window);//构造函数
	~cacheManager();//析构函数
};

extern const int packageSize;
extern unsigned int ack;

unsigned int checksum(const char* s, const int length);//差错检测
void sendACK(SOCKET s,bool flag,  const sockaddr* to, int tolen);//发送一个ACK包
int recvOneFile(SOCKET s, sockaddr* to, int tolen);//接收一个文件
