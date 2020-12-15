#pragma once
#include<iostream>
#include<fstream>
#include<winsock.h>
#include<string.h>
#include<iomanip>

#pragma comment(lib,"ws2_32.lib")
using namespace std;
#pragma warning(disable:4996)

//报文相关的宏
#define HEADLENGTH 16//报文头部的长度
#define DATALENGTH 14964//数据包中的有效数据部分的长度

//地址相关的宏
//#define SENDER_PORT 4001//发送者的端口号
//#define SENDER_ADDR "127.0.0.1"//发送者的IP地址
//#define RECVER_PORT 4000//接收者的端口号
//#define RECVER_ADDR "127.0.0.1"//接收者的IP地址

//超时相关的宏
#define FILENAMELEN 50 //文件名的长度限制
#define TIMEOUTS 1 //超时重传间隔 单位：秒
#define TIMEOUTMS 0 //超时间隔 单位：微秒
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
*											12	|****校验和****|****尾长*****|    一共16字节的头部信息
*															   14
*/

struct package {
	unsigned int seq;//序号
	unsigned int ack;//确认号
	char flag[2] = { 0,0 };//标志位
	unsigned short window;//接收窗口
	unsigned short checksum;//校验和
	unsigned short taillen;//最后一个数据包的数据长度
	char data[DATALENGTH] = {};//数据
	package() {
		seq = ack = window = checksum = taillen = 0;
	}
};

//全局变量
extern const int packageSize;
extern unsigned short WINDOW;
extern unsigned int seq;
//一些自己定义的工具函数
unsigned int checksum(const char* s, const int length);//差错检测
void Throughput(unsigned int pacs, clock_t time);//计算吞吐率并输出
int sendAndWait(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen);//向一个目的地址发送一个数据包并等待ACK
int sendOneFile(SOCKET s, char* filename, sockaddr* to, int tolen);//向一个目的地址发送一个完整的文件
//sendWindow向目的地址连续发送多个数据包，left和right用来限定数据包数量
void sendWindow(SOCKET s,const char* file, unsigned int left, unsigned int right, const sockaddr* to, int tolen);

