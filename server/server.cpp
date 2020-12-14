#include"rdt.h"
using namespace std;

const int packageSize = sizeof(package);
unsigned int ack = 0;//��¼�Լ���Ҫ�յ�����һ���������
char RECVER_ADDR[20] = {};//�������˵�ַ
unsigned short  RECVER_PORT;//�������˶˿�

int main() {
	//1. ��ʼ��Socket DLL
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) == 0)
		cout << "��ʼ��Socket DLL�ɹ���" << endl;
	else
		cout << "��ʼ��Socket DLLʧ�ܣ�" << endl;

	//2. ������ն��׽��ּ���ַ
	SOCKET server_socket;
	SOCKADDR_IN server_addr, client_addr;
	cout << "��������ն�IP��ַ��";
	cin >> RECVER_ADDR;
	cout << "��������ն˶˿ںţ�";
	cin >> RECVER_PORT;
	cout << endl;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(RECVER_ADDR);
	server_addr.sin_port = htons(RECVER_PORT);

	int len = sizeof(client_addr);

	//3. �������ն����ݱ�socket
	server_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (server_socket == INVALID_SOCKET) {
		cout << "����server_socketʧ�ܣ�" << endl;
		goto ERR;

	}

	//4. �󶨶˿�
	if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		cout << "server_socket�󶨶˿�ʧ�ܣ�" << endl;
		goto ERR;

	}
	else {
		cout << "���ն�IP��ַ��" << RECVER_ADDR << endl << "���ն˰󶨶˿ڣ�" << RECVER_PORT << endl;
	}

	//5. ��������
	//���ն�Ҫ�ȴ���recv����״̬
	while (1) {
		package* pac = new package();
		recvfrom(server_socket, (char*)pac, packageSize, 0, (sockaddr*)&client_addr, &len);

		if ((checksum((char*)pac, packageSize) == 0) && (pac->flag[0] & FLAG_SYN) == FLAG_SYN)//��õ�SYN
		{
			package* spac = new package();
			spac->flag[0] |= FLAG_ACK;
			spac->flag[0] |= FLAG_SYN;
			ack = pac->seq;//�����շ���ack��Ϊ���ͷ���seq
			spac->ack = ack;
			spac->checksum = checksum((char*)spac, packageSize);

			sendto(server_socket, (char*)spac, packageSize, 0, (sockaddr*)&client_addr, len);//�ڶ�������
			delete spac;
			delete pac;
			break;
		}
		delete pac;
	}
	cout << "�������ֳɹ���" << endl;
	cout << endl;

	//�����ļ�ѭ��
	for (int i = 0; i < 4; i++) {
		if (recvOneFile(server_socket, (sockaddr*)&client_addr, len) < 0) {
			goto ERR;
		}
	}

CL:	closesocket(server_socket);
	WSACleanup();
	system("pause");
 	return 0;

ERR:
	closesocket(server_socket);
	WSACleanup();
	system("pause");
	return -1;
}