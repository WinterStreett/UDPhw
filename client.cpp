#include<iostream>
#include<winsock.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
#pragma warning(disable:4996)

unsigned int sport = 4000,cport = 4001;

int main() {
	//1. ��ʼ��Socket DLL
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) == 0)
		cout << "��ʼ��Socket DLL�ɹ���" << endl;
	else
		cout << "��ʼ��Socket DLLʧ�ܣ�" << endl;

	//2. ����ͻ������׽��ּ���������ַ
	SOCKET client_socket;
	SOCKADDR_IN server_addr, client_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(sport);

	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	client_addr.sin_port = htons(cport);

	//3. �����ͻ��˶����ݱ�socket
	client_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (client_socket == INVALID_SOCKET) {
		cout << "����client_socketʧ�ܣ�" << endl;
		closesocket(client_socket);
		WSACleanup();
		return -1;
	}
	
	//4. �󶨶˿�
	int len = sizeof(server_addr);

	if (bind(client_socket, (sockaddr*)&client_addr, sizeof(client_addr)) == SOCKET_ERROR) {
		cout<<"�󶨶˿�ʧ�ܣ�"<<endl;
	}

	//4. ����
	while (1) {
		char sbuf[256] = {};
		cin.getline(sbuf, 256);
		if (sendto(client_socket, sbuf, 256, 0, (sockaddr*)&server_addr, len) == SOCKET_ERROR) {
			cout << "����ʧ�ܣ�" << endl;
		}
		char buf[256] = {};
		if (recvfrom(client_socket, buf, 256, 0, (sockaddr*)&server_addr, &len) == SOCKET_ERROR) {
			cout << "�������ݴ���" << endl;
		}
		else {
			cout << buf << endl;
		}
	}
	closesocket(client_socket);
	WSACleanup();
	return 0;
}