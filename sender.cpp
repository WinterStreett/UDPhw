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
#define SENDER_ADDR "127.0.0.1"//�����ߵ�IP��ַ����ipconfig���ҵ���
#define RECVER_PORT 4000//�����ߵĶ˿ں�
#define RECVER_ADDR "127.0.0.1"//�����ߵ�IP��ַ
#define FILENAMELEN 50 //�ļ����ĳ�������
//��־λ�꣬λ���ǰ��ӵ͵���

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
*											12	|****У���****|****����*****|    һ��16�ֽڵ�ͷ����Ϣ
*															   14
*/

struct package {
	unsigned int seq;//���
	unsigned int ack;//ȷ�Ϻ�
	char flag[2] = {0,0};//��־λ
	short window;//���մ���
	unsigned short checksum;//У���
	char unknown[2] = { 0,0 };//����
	char data[DATALENGTH] = {};//����
	package() {
		seq = ack = window = checksum = 0;
	}
};
//һЩȫ�ֱ���
const int packageSize = sizeof(package);
unsigned int seq = 0;

//һЩ��װ����������
unsigned int checksum(const char* s, const int length);//�����
void sendAndWait(SOCKET s,const char* buf,int len,int flags,const sockaddr *to,int tolen);

//������
int main() {
	//1. ��ʼ��Socket DLL
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) == 0)
		cout << "��ʼ��Socket DLL�ɹ���" << endl;
	else
		cout << "��ʼ��Socket DLLʧ�ܣ�" << endl;

	//2. ���巢�Ͷ��׽��ּ���ַ
	SOCKET sender_socket;
	SOCKADDR_IN recver_addr, sender_addr;
	recver_addr.sin_family = AF_INET;
	recver_addr.sin_addr.s_addr = inet_addr(RECVER_ADDR);
	recver_addr.sin_port = htons(RECVER_PORT);

	sender_addr.sin_family = AF_INET;
	sender_addr.sin_addr.s_addr = inet_addr(SENDER_ADDR);
	sender_addr.sin_port = htons(SENDER_PORT);

	int addr_len = sizeof(recver_addr);

	//3. �������Ͷ����ݱ�socket
	sender_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (sender_socket == INVALID_SOCKET) {
		cout << "����sender_socketʧ�ܣ�" << endl;
		closesocket(sender_socket);
		WSACleanup();
		return -1;
	}

	//4. �󶨶˿�
	if (bind(sender_socket, (sockaddr*)&sender_addr, addr_len) == SOCKET_ERROR) {
		cout << "�󶨶˿�ʧ�ܣ�" << endl;
	}
	else {
		cout << "���Ͷ�IP��ַ��" << SENDER_ADDR << endl << "���Ͷ˰󶨶˿ڣ�" << SENDER_PORT << endl;
	}

	//5. �������� �ȼ򻯴���
	char handshake[packageSize] = "�������ֳɹ���";
	
	if (sendto(sender_socket, handshake, packageSize, 0, (sockaddr*)&recver_addr, addr_len) == SOCKET_ERROR) {
		cout << "��������ʧ�ܣ�" << endl;
		closesocket(sender_socket);
		WSACleanup();
		return -1;
	}
	else {
		cout << "�������ֳɹ���" << endl;
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
		//д��У����Լ����
		pac->seq = seq;
		memcpy(buf, pac, packageSize);
		pac->checksum = checksum(buf, packageSize);
		memcpy(buf, pac, packageSize);
		sendAndWait(sender_socket, buf, packageSize, 0, (sockaddr*)&recver_addr, addr_len);
		delete pac;
		pac = new package();//����pacָ��
		seq = (seq + 1) % 2;//ģ2��ǰ��


				////��֤У����
		//memcpy(buf, pac, packageSize);
		//pac->seq = 1;
		//memcpy(buf, pac, packageSize);
		//pac->checksum = checksum(buf, packageSize);
		//memcpy(buf, pac, packageSize);
		//cout << checksum(buf, packageSize) << endl;
	}
	memcpy(buf, pac, packageSize);
	//д��У����Լ����
	pac->seq = seq;
	pac->flag[0] |=FLAG_FEND;
	memcpy(buf, pac, packageSize);
	pac->checksum = checksum(buf, packageSize);
	memcpy(buf, pac, packageSize);
	//���������
	sendAndWait(sender_socket, buf, packageSize, 0, (sockaddr*)&recver_addr, addr_len);
	cout << "��ɴ��䣡" << endl;

	//������Դռ��
	delete pac;
	reader.close();
	closesocket(sender_socket);
	WSACleanup();
	return 0;
}

unsigned int checksum(const char* s, const int length) {
/*  ���룺�ַ�ָ��
*		  �ַ�������
*	
*	������ַ�����У���
*/
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


void sendAndWait(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen) {
/*	���ܣ���ָ��Ŀ�ĵط���һ�����ݰ������ȴ���ȷ��ACK,ACKȷ�Ϻ���ܷ���
*/
	SOCKADDR_IN from;
	sendto(s, buf, len, flags, to, tolen);
	//�����ǵȴ�ACK
	while (1) {
		//�½����������������н��յ�������
		char* sbuf = new char[packageSize];
		package* recv_pac = new package();
		//���շ��صİ�
		recvfrom(s, sbuf, packageSize, 0, (sockaddr*)&from, &tolen);
		memcpy(recv_pac, sbuf, packageSize);
		//�������յ��İ��Ƿ�������ģ��Լ�ACK�ֶ�
		if ((checksum(sbuf, packageSize) != 0) || (((recv_pac->flag[0] & FLAG_ACK) == FLAG_ACK) && recv_pac->ack != seq)) {
			//������յ��İ����� ���� ������ACK ��ô�ط�һ��
			sendto(s, buf, len, flags, to, tolen);
			delete[]sbuf;
			delete recv_pac;
		}
		else if (((recv_pac->flag[0] & FLAG_ACK) == FLAG_ACK) && (recv_pac->ack == seq)) {
			//����յ�������ȷ��ACK����ô˵�����͵İ�������շ������Է���һ������
			delete[]sbuf;
			delete recv_pac;
			break;
		}
	}
}
