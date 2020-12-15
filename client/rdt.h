#pragma once
#include<iostream>
#include<fstream>
#include<winsock.h>
#include<string.h>
#include<iomanip>

#pragma comment(lib,"ws2_32.lib")
using namespace std;
#pragma warning(disable:4996)

//������صĺ�
#define HEADLENGTH 16//����ͷ���ĳ���
#define DATALENGTH 14964//���ݰ��е���Ч���ݲ��ֵĳ���

//��ַ��صĺ�
//#define SENDER_PORT 4001//�����ߵĶ˿ں�
//#define SENDER_ADDR "127.0.0.1"//�����ߵ�IP��ַ
//#define RECVER_PORT 4000//�����ߵĶ˿ں�
//#define RECVER_ADDR "127.0.0.1"//�����ߵ�IP��ַ

//��ʱ��صĺ�
#define FILENAMELEN 50 //�ļ����ĳ�������
#define TIMEOUTS 1 //��ʱ�ش���� ��λ����
#define TIMEOUTMS 0 //��ʱ��� ��λ��΢��
#define MISSLIMT 5//��ʱ�ش�������������

//��־λ�꣬λ���ǰ��ӵ͵���
#define FLAG_FHEAD 0x80 //��8λ �ļ�����
#define FLAG_FEND 0x40//��7λ �ļ��Ƿ������
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
*											12	|****У���****|****β��*****|    һ��16�ֽڵ�ͷ����Ϣ
*															   14
*/

struct package {
	unsigned int seq;//���
	unsigned int ack;//ȷ�Ϻ�
	char flag[2] = { 0,0 };//��־λ
	unsigned short window;//���մ���
	unsigned short checksum;//У���
	unsigned short taillen;//���һ�����ݰ������ݳ���
	char data[DATALENGTH] = {};//����
	package() {
		seq = ack = window = checksum = taillen = 0;
	}
};

//ȫ�ֱ���
extern const int packageSize;
extern unsigned short WINDOW;
extern unsigned int seq;
//һЩ�Լ�����Ĺ��ߺ���
unsigned int checksum(const char* s, const int length);//�����
void Throughput(unsigned int pacs, clock_t time);//���������ʲ����
int sendAndWait(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen);//��һ��Ŀ�ĵ�ַ����һ�����ݰ����ȴ�ACK
int sendOneFile(SOCKET s, char* filename, sockaddr* to, int tolen);//��һ��Ŀ�ĵ�ַ����һ���������ļ�
//sendWindow��Ŀ�ĵ�ַ�������Ͷ�����ݰ���left��right�����޶����ݰ�����
void sendWindow(SOCKET s,const char* file, unsigned int left, unsigned int right, const sockaddr* to, int tolen);

