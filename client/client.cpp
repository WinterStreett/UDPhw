//ͷ�ļ��ͱ�Ҫ�ı���ָ��
#include"rdt.h"
using namespace std;

//ȫ�ֱ���
const int packageSize = sizeof(package);
unsigned short WINDOW;
unsigned int seq = 0;

char SENDER_ADDR[20] = {};//�����ߵ�IP��ַ
char RECVER_ADDR[20] = {};//�����ߵ�IP��ַ
unsigned int SENDER_PORT; //�����ߵĶ˿ں�
unsigned int RECVER_PORT; //�����ߵĶ˿ں�

int main() {
	char filename[FILENAMELEN] = "temp//1.jpg";
	char filename1[FILENAMELEN] = "temp//2.jpg";
	char filename2[FILENAMELEN] = "temp//3.jpg";
	char filename3[FILENAMELEN] = "temp//helloworld.txt";

	//1. ��ʼ��Socket DLL
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) == 0)
		cout << "��ʼ��Socket DLL�ɹ���" << endl;
	else
		cout << "��ʼ��Socket DLLʧ�ܣ�" << endl;

	//2. ����ͻ����׽��ּ���ַ
	cout << "�����뷢�Ͷ�IP��ַ��";
	cin >> SENDER_ADDR;
	cout << "�����뷢�Ͷ˶˿ںţ�";
	cin >> SENDER_PORT;
	cout << "������·����IP��ַ��";
	cin >> RECVER_ADDR;
	cout << "������·�����Ķ˿ںţ�";
	cin >> RECVER_PORT;
	cout << endl;

	SOCKET client_socket;
	SOCKADDR_IN server_addr, client_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(RECVER_ADDR);
	server_addr.sin_port = htons(RECVER_PORT);

	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = inet_addr(SENDER_ADDR);
	client_addr.sin_port = htons(SENDER_PORT);



	int addr_len = sizeof(server_addr);
	package* pac = new package();
	int ret;


	//3. �����ͻ������ݱ�socket
	client_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (client_socket == INVALID_SOCKET) {
		cout << "����client_socketʧ�ܣ�" << endl;
		closesocket(client_socket);
		WSACleanup();
		return -1;
	}

	//4. �󶨶˿�
	if (bind(client_socket, (sockaddr*)&client_addr, addr_len) == SOCKET_ERROR) {
		cout << "�󶨶˿�ʧ�ܣ�" << endl;
		goto CLEAN;
	}
	else {
		cout << "���Ͷ�IP��ַ��" << SENDER_ADDR << endl << "���Ͷ˰󶨶˿ڣ�" << SENDER_PORT << endl;
	}

	//5. ��������
	cout << "�趨���ڴ�С:";
	cin >> WINDOW;
	cout << endl;

	cout << "��ʼ����" << endl;
	pac->seq = 0;
	pac->flag[0] |= FLAG_SYN;

	pac->checksum = checksum((char*)pac, packageSize);
	//���͵�һ����,sendAndWait����Ӧ���Ѿ������˷��������صĵڶ�������ACK��
	ret = sendAndWait(client_socket, (char*)pac, packageSize, 1, (sockaddr*)&server_addr, addr_len);//����ʧ�ܵĻ��᳢�Լ��Σ�ֱ������������
	if (ret < 0)
		goto CLEAN;
	cout << "����������ɣ�" << endl;

	cout << endl;
	//6. 3-2

	//if (sendOneFile(client_socket, filename, (sockaddr*)&server_addr, addr_len)<0) {
	//	cout << "�����ļ� " << filename << " ʧ�ܣ�" << endl;
	//}
	if (sendOneFile(client_socket, filename, (sockaddr*)&server_addr, addr_len)<0) {
		cout << "�����ļ� " << filename << " ʧ�ܣ�" << endl;
	}
	if (sendOneFile(client_socket, filename1, (sockaddr*)&server_addr, addr_len) < 0) {
		cout << "�����ļ� " << filename1 << " ʧ�ܣ�" << endl;
	}
	if (sendOneFile(client_socket, filename2, (sockaddr*)&server_addr, addr_len) < 0) {
		cout << "�����ļ� " << filename2 << " ʧ�ܣ�" << endl;
	}
	if (sendOneFile(client_socket, filename3, (sockaddr*)&server_addr, addr_len) < 0) {
		cout << "�����ļ� " << filename3 << " ʧ�ܣ�" << endl;
	}

CLEAN:
	delete pac;
	closesocket(client_socket);
	WSACleanup();
	system("pause");
	return 0;
}