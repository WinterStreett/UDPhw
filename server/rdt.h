#pragma once
#include<iostream>
#include<fstream>
#include<winsock.h>
#include<string.h>
#include<ctime>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
#pragma warning(disable:4996)

#define HEADLENGTH 16//����ͷ���ĳ���
#define DATALENGTH 1464//���ݰ��е���Ч���ݲ��ֵĳ���

//#define SENDER_PORT 4001//�����ߵĶ˿ں�
//#define SENDER_ADDR "127.0.0.1"//�����ߵ�IP��ַ
//
//#define RECVER_PORT 4000//�����ߵĶ˿ں�
//#define RECVER_ADDR "127.0.0.1"//�����ߵ�IP��ַ

#define FILENAMELEN 50 //�ļ����ĳ�������
#define TIMEOUTS 0//��ʱ�ش���� ��
#define TIMEOUTVS 300//��ʱ�ش���� ΢��

#define MISSLIMT 5//��ʱ�ش�������������

//��־λ�꣬λ���ǰ��ӵ͵���
#define FLAG_FHEAD 0x80 //��8λ �ļ�����
#define FLAG_FEND 0x40//��7λ
#define FLAG_ACK 0x10//��5λ
#define FLAG_SYN 0x2//��2λ
#define FLAG_FIN Ox1//��1λ
/*
* ����ͷ�ṹ
*												<------------4�ֽ�----------->
*											0	|************���************|
*											4	|***********ȷ�Ϻ�***********|
*											8	|****��־λ****|**���մ���***|
*															   10
*											12	|****У���****|****����*****|    һ��16�ֽڵ�ͷ����Ϣ
*															   14
*/

struct package {
	unsigned int seq;//���
	unsigned int ack;//ȷ�Ϻ�
	char flag[2] = { 0,0 };//��־λ
	unsigned short window;//���մ���
	unsigned short checksum;//У���
	unsigned short taillen;
	char data[DATALENGTH] = {};//����
	package() {
		seq = ack = window = checksum = taillen = 0;
	}
};

struct cacheManager {
	unsigned int base;
	unsigned short window;
	bool* indexBuf;
	unsigned int getNextBase();//��ȡ��һ��base���ҵ�buf�������ۼ�ȷ�ϵ�������ֵ
	void insertSeq(unsigned int seq);//����һ�����
	void initBuf();//��ʼ��������
	cacheManager(unsigned int Base, unsigned short Window);//���캯��
	~cacheManager();//��������
};

extern const int packageSize;
extern unsigned int ack;

unsigned int checksum(const char* s, const int length);//�����
void sendACK(SOCKET s,bool flag,  const sockaddr* to, int tolen);//����һ��ACK��
int recvOneFile(SOCKET s, sockaddr* to, int tolen);//����һ���ļ�
