#include<iostream>
#include<winsock.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
#pragma warning(disable:4996)

unsigned int sport = 4000;

int main() {
	//1. ��ʼ��Socket DLL
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) == 0)
		cout << "��ʼ��Socket DLL�ɹ���" << endl;
	else
		cout << "��ʼ��Socket DLLʧ�ܣ�" << endl;

	//2. ������������׽��ּ���������ַ
	SOCKET server_socket;
	SOCKADDR_IN server_addr,client_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(sport);

	//3. ���������������ݱ�socket
	server_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (server_socket == INVALID_SOCKET) {
		cout << "����server_socketʧ�ܣ�" << endl;
		closesocket(server_socket);
		WSACleanup();
		return -1;
	}

	//4. �󶨶˿�
	if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		cout << "server_socket�󶨶˿�ʧ�ܣ�" << endl;
		closesocket(server_socket);
		WSACleanup();
		return -1;
	}
	cout << "��ʼ����" << endl;

	//5. �Ȳ���һ��
	int len = sizeof(client_addr);
	while (1) {
		char buf[256] = {};
		if (recvfrom(server_socket, buf, 256, 0, (sockaddr*)&client_addr, &len) == SOCKET_ERROR) {
			cout << "�������ݳ���" << endl;
		}
		else {
			cout << buf << endl;
		}
		char sbuf[256] = {};
		cin.getline(sbuf, 256);
		sendto(server_socket, sbuf, 256, 0, (sockaddr*)&client_addr, len);
	}
	closesocket(server_socket);
	WSACleanup();
	return 0;
}