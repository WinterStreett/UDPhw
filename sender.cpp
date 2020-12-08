#include<iostream>
#include<fstream>
#include<winsock.h>
#include<string.h>
#include<ctime>
#include<iomanip>
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
#define TIMEOUTS 0 //��ʱ�ش���� ��λ����
#define TIMEOUTMS 500000 //��ʱ��� ��λ��΢��
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
*											12	|****У���****|****����*****|    һ��16�ֽڵ�ͷ����Ϣ
*															   14
*/

struct package {
	unsigned int seq;//���
	unsigned int ack;//ȷ�Ϻ�
	char flag[2] = {0,0};//��־λ
	unsigned short window;//���մ���
	unsigned short checksum;//У���
	unsigned short taillen;
	char data[DATALENGTH] = {};//����
	package() {
		seq = ack = window = checksum = taillen = 0;
	}
};
//һЩȫ�ֱ���
const int packageSize = sizeof(package);
unsigned int seq = 0;
unsigned  short fileCount = 0;

//һЩ��װ����������
unsigned int checksum(const char* s, const int length);//�����
int sendAndWait(SOCKET s,const char* buf,int len,int flags,const sockaddr *to,int tolen);//��һ��Ŀ�ĵ�ַ����һ�����ݰ����ȴ�ACK
void sendOneFile(SOCKET s, char* filename, const sockaddr* to, int tolen);//��һ��Ŀ�ĵ�ַ����һ���������ļ�
void Throughput(unsigned int pacs, clock_t time);//���������ʲ����
void seqNext() {
	seq = (seq + 1) % 2;
}
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

	//5. �������� 
	//��һ������
	package* pac = new package();
	pac->seq = seq;
	pac->flag[0] |= FLAG_SYN;

	pac->checksum = checksum((char*)pac, packageSize);
	//���͵�һ����,sendAndWait����Ӧ���Ѿ������˷��������صĵڶ�������ACK��
	sendAndWait(sender_socket, (char*)pac, packageSize, 0, (sockaddr*)&recver_addr, addr_len);//����ʧ�ܵĻ��᳢�Լ��Σ�ֱ������������
	cout << "����������ɣ�" << endl;

	cout << "�������ֺ�reader��seq=" << seq << endl;
	
	//6. rdt2.2
	char filename1[FILENAMELEN] = "temp//1.jpg";
	char filename2[FILENAMELEN] = "temp//2.jpg";
	char filename3[FILENAMELEN] = "temp//3.jpg";
	char filename4[FILENAMELEN] = "temp//helloworld.txt";
	sendOneFile(sender_socket, filename1, (sockaddr*)&recver_addr, addr_len);
	sendOneFile(sender_socket, filename2, (sockaddr*)&recver_addr, addr_len);
	sendOneFile(sender_socket, filename3, (sockaddr*)&recver_addr, addr_len);
	sendOneFile(sender_socket, filename4, (sockaddr*)&recver_addr, addr_len);

	//������Դռ��
	delete pac;
	closesocket(sender_socket);
	WSACleanup();
	system("pause");
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


int sendAndWait(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen) {
/*	���ܣ���ָ��Ŀ�ĵط���һ�����ݰ������ȴ���ȷ��ACK,ACKȷ�Ϻ���ܷ���
*/
	int missCount = 0;
	SOCKADDR_IN from;
	sendto(s, buf, len, flags, to, tolen);
	struct timeval timeout = { TIMEOUTS,TIMEOUTMS };
	fd_set fdr;
	FD_ZERO(&fdr);
	FD_SET(s, &fdr);
	int ret;
	//�����ǵȴ�ACK
	while (1) {
		//�½����������������н��յ�������
		package* recv_pac = new package();
		ret = select(s, &fdr, NULL, NULL, &timeout);
		if (ret < 0)//��������
		{
			cout << "select��������" << endl;
			delete recv_pac;
			closesocket(s);
			WSACleanup();
			return -1;
		}
		else if (ret == 0)//�ȴ���ʱ
		{
			sendto(s, buf, len, flags, to, tolen);//�ط�
			missCount++;
			cout << "��" << missCount << "�γ�ʱ��" << endl;
			if (missCount == MISSLIMT) {
				delete recv_pac;
				//cout << "��ʱ�����������ޣ�����ʧ�ܣ�" << endl;
				exit(-1);
			}
		}
		else {
			recvfrom(s, (char*)recv_pac, packageSize, 0, (sockaddr*)&from, &tolen);
			//�������յ��İ��Ƿ�������ģ��Լ�ACK�ֶ�
			if ((checksum((char*)recv_pac, packageSize) != 0) || (((recv_pac->flag[0] & FLAG_ACK) == FLAG_ACK) && recv_pac->ack != seq)) {
				//������յ��İ����� ���� ������ACK ��ô�ط�һ��
				sendto(s, buf, len, flags, to, tolen);
			}
			else if (((recv_pac->flag[0] & FLAG_ACK) == FLAG_ACK) && (recv_pac->ack == seq)) {
				//����յ�������ȷ��ACK����ô˵�����͵İ�������շ������Է���һ������
				break;
			}
		}
		delete recv_pac;
	}
}


void sendOneFile(SOCKET s, char* filename, const sockaddr* to, int tolen) {
/*	���ܣ���Ŀ�ĵ�ַ������һ���������ļ�
*/
	unsigned int pacCount = 0;//��¼���͵����ݰ�����
	clock_t start, end;//��ʱ��
	package* pac = new package();
	unsigned int filelen;//�ļ�����
	unsigned int pacNum;//��Ҫ�İ�������
	unsigned short taillen;
	char* file;//�����ļ�������

	//��ȡ�����ļ�
	ifstream reader;
	reader.open(filename, ios::binary);
	reader.seekg(0, ios::end);
	filelen = reader.tellg();
	reader.seekg(0, ios::beg);
	file = new char[filelen];//file�б������ļ�����
	reader.read(file, filelen);
	pacNum = filelen / DATALENGTH;//������β��
	taillen = filelen % DATALENGTH;//β��ʣ�೤��

	//���Ƚ��ļ����������ն�
	memcpy(pac->data, &filelen, 4);//���ݲ��ֵ�ǰ4���ֽ����ļ�����
	memcpy(pac->data+4, filename, FILENAMELEN);//���ݲ��ִ�4��ʼ���ļ���
	pac->seq = seq;
	pac->flag[0] |= FLAG_FHEAD;
	pac->checksum = checksum((char*)pac, packageSize);
	start = clock();
	sendAndWait(s, (char*)pac, packageSize, 0, to, tolen);
	//�����ļ����ɹ�����ʼ��ȡ�ļ�
	seqNext();
	delete pac;
	pac = new package();


	unsigned int di;
	for (int i = 0; i < pacNum; i++)
	{
		//��file�ж�ȡһ�����ݣ�����ΪDATALENGTH
		di = i * DATALENGTH;
		memcpy(pac->data, file + di, DATALENGTH);
		//�����ݰ����ú�
		pac->seq = seq;
		pac->checksum = checksum((char*)pac, packageSize);
		//�������ݰ����ȴ�ȷ��
		sendAndWait(s, (char*)pac, packageSize, 0, to, tolen);
		delete pac;
		pac = new package();//����pacָ��
		seqNext();//ģ2��ǰ��
		pacCount++;//���ݰ�������һ
	}
	//�������һС��
	int a = filelen - taillen;
	memcpy(pac->data,file+ a, taillen);
	pac->seq = seq;
	pac->flag[0] |= FLAG_FEND;//��FEND��־λ��λ
	pac->taillen = taillen;
	pac->checksum = checksum((char*)pac, packageSize);
	sendAndWait(s, (char*)pac, packageSize, 0, to, tolen);
	end = clock();
	seqNext();//ģ2��ǰ��
	pacCount++;//���ݰ�������һ

	cout << "�ɹ����͵�"<<++fileCount<<"���ļ���" << endl;
	cout << "��ʱ��" << end - start << "����" << "	" << "�����ʣ�";
	Throughput(pacCount, end - start);
	//������Դռ��
	delete[]file;
	delete pac;
	reader.close();
	//cout << "seq:" << seq << endl;
}

void Throughput(unsigned int pacs, clock_t time) {//���������ʣ��Զ����ϵ�λ���
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