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

//#define SENDER_PORT 1002//�����ߵĶ˿ں�
//#define SENDER_ADDR "10.130.75.174"//�����ߵ�IP��ַ
//
//#define RECVER_PORT 1001//�����ߵĶ˿ں�
//#define RECVER_ADDR "10.130.75.174"//�����ߵ�IP��ַ

#define FILENAMELEN 50 //�ļ����ĳ�������
#define TIMEOUTS 2 //��ʱ�ش���� ��λ����
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
	char flag[2] = {0,0};//��־λ
	unsigned short window;//���մ���
	unsigned short checksum;//У���
	unsigned short taillen;//���һ�����ݰ������ݳ���
	char data[DATALENGTH] = {};//����
	package() {
		seq = ack = window = checksum = taillen = 0;
	}
};
//һЩȫ�ֱ���
const int packageSize = sizeof(package);
unsigned int seq = 0;
unsigned  short fileCount = 0;
char SENDER_ADDR[20] = {};//�����ߵ�IP��ַ
char RECVER_ADDR[20] = {};//�����ߵ�IP��ַ
unsigned int SENDER_PORT; //�����ߵĶ˿ں�
unsigned int RECVER_PORT; //�����ߵĶ˿ں�

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

	char filename1[FILENAMELEN] = "temp//1.jpg";
	char filename2[FILENAMELEN] = "temp//2.jpg";
	//char filename3[FILENAMELEN] = "temp//3.jpg";
	char filename4[FILENAMELEN] = "temp//helloworld.txt";

	//1. ��ʼ��Socket DLL
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) == 0)
		cout << "��ʼ��Socket DLL�ɹ���" << endl;
	else
		cout << "��ʼ��Socket DLLʧ�ܣ�" << endl;

	//2. ���巢�Ͷ��׽��ּ���ַ
	cout << "�����뷢�Ͷ�IP��ַ��";
	cin >> SENDER_ADDR;
	cout << "�����뷢�Ͷ˶˿ںţ�"  ;
	cin >> SENDER_PORT;
	cout << "������·����IP��ַ��" ;
	cin >> RECVER_ADDR;
	cout << "������·�����Ķ˿ںţ�" ;
	cin >> RECVER_PORT;
	cout << endl;
	SOCKET sender_socket;
	SOCKADDR_IN recver_addr, sender_addr;
	recver_addr.sin_family = AF_INET;
	recver_addr.sin_addr.s_addr = inet_addr(RECVER_ADDR);
	recver_addr.sin_port = htons(RECVER_PORT);

	sender_addr.sin_family = AF_INET;
	sender_addr.sin_addr.s_addr = inet_addr(SENDER_ADDR);
	sender_addr.sin_port = htons(SENDER_PORT);

	int addr_len = sizeof(recver_addr);
	package* pac = new package();
	int ret;

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
		goto CLEAN;
	}
	else {
		cout << "���Ͷ�IP��ַ��" << SENDER_ADDR << endl << "���Ͷ˰󶨶˿ڣ�" << SENDER_PORT << endl;
	}

	//5. �������� 
	//��һ������
	cout << "��ʼ����" << endl;
	pac->seq = seq;
	pac->flag[0] |= FLAG_SYN;

	pac->checksum = checksum((char*)pac, packageSize);
	//���͵�һ����,sendAndWait����Ӧ���Ѿ������˷��������صĵڶ�������ACK��
	ret = sendAndWait(sender_socket, (char*)pac, packageSize, 0, (sockaddr*)&recver_addr, addr_len);//����ʧ�ܵĻ��᳢�Լ��Σ�ֱ������������
	if (ret < 0)
		goto CLEAN;
	cout << "����������ɣ�" << endl;

	cout << endl;


	//6. �ͻ������ļ�����

	sendOneFile(sender_socket, filename1, (sockaddr*)&recver_addr, addr_len);
	sendOneFile(sender_socket, filename2, (sockaddr*)&recver_addr, addr_len);
	//sendOneFile(sender_socket, filename3, (sockaddr*)&recver_addr, addr_len);
	sendOneFile(sender_socket, filename4, (sockaddr*)&recver_addr, addr_len);

	//������Դռ��
CLEAN:
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

	package* recv_pac;
	//�����ǵȴ�ACK
	while (1) {
		//�½����������������н��յ�������
		struct timeval timeout = { TIMEOUTS,TIMEOUTMS };
		fd_set fdr;
		FD_ZERO(&fdr);
		FD_SET(s, &fdr);
		int ret;
		recv_pac = new package();
		ret = select(s, &fdr, NULL, NULL, &timeout);
		if (ret < 0)//��������
		{
			cout << "select��������" << endl;
			delete recv_pac;
			return -1;
		}
		else if (ret == 0)//�ȴ���ʱ
		{
			sendto(s, buf, len, flags, to, tolen);//�ط�
			missCount++;
			//cout << "��" << missCount << "�γ�ʱ��" << endl;
			if (missCount == MISSLIMT) {
				delete recv_pac;
				//cout << "��ʱ�����������ޣ�����ʧ�ܣ�" << endl;
				return -1;
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
	return 0;
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
	int a;
	//���Ƚ��ļ����������ն�
	memcpy(pac->data, &filelen, 4);//���ݲ��ֵ�ǰ4���ֽ����ļ�����
	memcpy(pac->data+4, filename, FILENAMELEN);//���ݲ��ִ�4��ʼ���ļ���
	pac->seq = seq;
	pac->flag[0] |= FLAG_FHEAD;//FHEAD��־λ��һ
	pac->checksum = checksum((char*)pac, packageSize);
	start = clock();
	
	if (sendAndWait(s, (char*)pac, packageSize, 0, to, tolen) < 0) {
		cout << "�����ļ�ͷʧ�ܣ�" << endl;
		goto CL;
	}
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
		if (sendAndWait(s, (char*)pac, packageSize, 0, to, tolen) < 0) {
			goto CL;
		}
		delete pac;
		pac = new package();//����pacָ��
		seqNext();//ģ2��ǰ��
		pacCount++;//���ݰ�������һ
	}
	//�������һС��
	a = filelen - taillen;
	memcpy(pac->data,file+ a, taillen);
	pac->seq = seq;
	pac->flag[0] |= FLAG_FEND;//��FEND��־λ��λ
	pac->taillen = taillen;
	pac->checksum = checksum((char*)pac, packageSize);
	if (sendAndWait(s, (char*)pac, packageSize, 0, to, tolen)<0) {
		goto CL;
	}

	end = clock();
	seqNext();//ģ2��ǰ��
	pacCount++;//���ݰ�������һ

	cout << "�ɹ����� "<<filename<<" �ļ���" << endl;
	cout << "��ʱ��" << end - start << "����" << "	" << "�����ʣ�";
	Throughput(pacCount, end - start);
	//������Դռ��
CL:
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