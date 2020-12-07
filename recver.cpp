#include<iostream>
#include<fstream>
#include<winsock.h>
#include<string.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
#pragma warning(disable:4996)

#define HEADLENGTH 16//����ͷ���ĳ���
#define DATALENGTH 1464//���ݰ��е���Ч���ݲ��ֵĳ���
#define SENDER_PORT 4001//�����ߵĶ˿ں�
#define SENDER_ADDR "127.0.0.1"//�����ߵ�IP��ַ
#define RECVER_PORT 4000//�����ߵĶ˿ں�
#define RECVER_ADDR "127.0.0.1"//�����ߵ�IP��ַ
#define FILENAMELEN 50 //�ļ����ĳ�������


//��־λ�꣬λ���ǰ��ӵ͵���
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
	short window;//���մ���
	unsigned short checksum;//У���
	char unknown[2] = { 0,0 };//����
	char data[DATALENGTH] = {};//����
	package() {
		seq = ack = window = checksum = 0;
	}
};
const int packageSize = sizeof(package);
unsigned int seq = 0;//��¼�Լ���Ҫ�յ�����һ���������


unsigned int checksum(const char* s, const int length);//�����
void sendACK(SOCKET s, bool redun, const sockaddr* to, int tolen);//����һ��ACK��
void recvOneFile(SOCKET s, sockaddr* to, int tolen);//����һ���ļ�

int main() {
	//1. ��ʼ��Socket DLL
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) == 0)
		cout << "��ʼ��Socket DLL�ɹ���" << endl;
	else
		cout << "��ʼ��Socket DLLʧ�ܣ�" << endl;

	//2. ������ն��׽��ּ���ַ
	SOCKET recver_socket;
	SOCKADDR_IN recver_addr, sender_addr;
	recver_addr.sin_family = AF_INET;
	recver_addr.sin_addr.s_addr = inet_addr(RECVER_ADDR);
	recver_addr.sin_port = htons(RECVER_PORT);

	//3. �������ն����ݱ�socket
	recver_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (recver_socket == INVALID_SOCKET) {
		cout << "����recver_socketʧ�ܣ�" << endl;
		closesocket(recver_socket);
		WSACleanup();
		return -1;
	}

	//4. �󶨶˿�
	if (bind(recver_socket, (sockaddr*)&recver_addr, sizeof(recver_addr)) == SOCKET_ERROR) {
		cout << "server_socket�󶨶˿�ʧ�ܣ�" << endl;
		closesocket(recver_socket);
		WSACleanup();
		return -1;
	}
	else {
		cout << "���ն�IP��ַ��" << RECVER_ADDR << endl << "���ն˰󶨶˿ڣ�" << RECVER_PORT << endl;
	}

	//5. �򻯰���������
	int len = sizeof(sender_addr);
	char handshake[1500] = {};
	if (recvfrom(recver_socket, handshake, packageSize, 0, (sockaddr*)&sender_addr, &len) == SOCKET_ERROR) {
		cout << "��������ʧ�ܣ�" << endl;
		closesocket(recver_socket);
		WSACleanup();
		return -1;
	}
	else {
		cout << handshake << endl;
	}

	 //6. rdt2.2���ն�
	//��һ���������ļ���������ʶ��
	recvOneFile(recver_socket, (sockaddr*)&sender_addr, len);
	recvOneFile(recver_socket, (sockaddr*)&sender_addr, len);


	closesocket(recver_socket);
	WSACleanup();
	return 0;
}

unsigned int checksum(const char* s, const int length) {
	unsigned int sum = 0;
	unsigned short temp1 = 0;//����������޷��������������Ϊ������չ���õ�һЩ����ֵֹ���
	unsigned short temp2 = 0;
	for (int i = 0; i < length; i += 2) {
		temp1 = s[i + 1] << 8;
		temp2 = s[i] & 0x00ff;//һ��Ҫ�Ѹ�8λ���㣡���������8λ���λ��1����ô��8λҲ����Ϊ������չ��ȫΪ1
		sum += temp1 | temp2;
	}
	while (sum > 0xffff) {//�Ѽӷ�����Ĳ��֣����ڸ�16λ���ӵ���16λ��
		sum = (sum & 0xffff) + (sum >> 16);
	}
	return ~(sum | 0xffff0000);//��16λȡ������16λȫΪ0
}

void sendACK(SOCKET s,bool redun, const sockaddr* to, int tolen) {
/*	���ܣ���Ŀ�ĵ�ַ����һ��ACK  
*	redun��Ϊ��ʱ��˵��Ҫ���͵���һ������ACK��seqֵ���ܱ�
*/
	char* sbuf = new char[packageSize];
	package* ack_pac = new package();
	ack_pac->flag[0] |= FLAG_ACK;//����ACK��־λ
	if(redun)
		ack_pac->ack = (seq + 1) % 2;
	else
		ack_pac->ack = seq;
	memcpy(sbuf, ack_pac, packageSize);
	ack_pac->checksum = checksum(sbuf, packageSize);//����У���
	memcpy(sbuf, ack_pac, packageSize);
	sendto(s, sbuf, packageSize, 0, to, tolen);
	delete[]sbuf;
	delete ack_pac;
}

void recvOneFile(SOCKET s, sockaddr* to, int tolen) {
	char buf[packageSize] = {};
	package* pac = new package();
	char filename[FILENAMELEN] = {};
	//��Դ��ַ�����ļ������ݰ�
	recvfrom(s, buf, packageSize, 0, to, &tolen);
	memcpy(pac, buf, packageSize);
	//��ֵ�ļ���
	memcpy(filename, pac->data, FILENAMELEN);

	ofstream writer;
	writer.open(filename, ios::binary);//��һ���ļ�

	sendACK(s, false, to, tolen);//����ACK
	seq = (seq + 1) % 2;
	//int count = 0;
	while (1) {
		recvfrom(s, buf, packageSize, 0, to, &tolen);
		memcpy(pac, buf, packageSize);
		if ((checksum(buf, packageSize) != 0) || (pac->seq != seq))//�������������ˣ����߲��ǽ��շ���Ҫ�İ�
		{
			//�ط�ACK��
			sendACK(s, true, to, tolen);
		}
		else if (pac->seq == seq) {//������յ�����ȷ�İ���д���ļ�������ȷ��ACK
			//cout << ++count << endl;
			writer.write(pac->data, DATALENGTH);
			sendACK(s, false, to, tolen);
			if ((pac->flag[0] & FLAG_FEND) == FLAG_FEND)//������յ������ļ�ĩβ���˳�
				break;
		}
		seq = (seq + 1) % 2;
		delete pac;
		pac = new package();
	}
	delete pac;
	writer.close();
}