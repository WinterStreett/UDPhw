#include"rdt.h"
using namespace std;

cacheManager::cacheManager(unsigned int Base, unsigned short Window) {
	this->base = Base;
	this->window = Window;
	indexBuf = new bool[Window];
	for (int i = 0; i < Window; i++) {
		indexBuf[i] = false;
	}
}

cacheManager::~cacheManager() {
	delete[]indexBuf;
}

void cacheManager::insertSeq(unsigned int seq) {
	indexBuf[seq - base] = true;
}

unsigned int cacheManager::getNextBase() {
	unsigned int i;
	for ( i = base; i < base + window; i++) {
		if (!indexBuf[i - base])
			break;
	}
	return i;//�����ֵ����һ�������յ���ack
}

void cacheManager::initBuf() {
	for (int i = 0; i < window; i++) {
		indexBuf[i] = false;
	}
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


void sendACK(SOCKET s,bool flag, const sockaddr* to, int tolen) {
	/*	���ܣ���Ŀ�ĵ�ַ����һ��ACK
	*	redun��Ϊ��ʱ��˵��Ҫ���͵���һ������ACK��ackֵ���ܱ�
	*/

	package* ack_pac = new package();
	ack_pac->flag[0] |= FLAG_ACK;//����ACK��־λ
	if (flag)
		ack_pac->ack = (ack + 1) % 0xffffffff;
	else
	{
		ack_pac->ack = ack ;
	}

	ack_pac->checksum = checksum((char*)ack_pac, packageSize);//����У���

	sendto(s, (char*)ack_pac, packageSize, 0, to, tolen);
	delete ack_pac;
}

int recvOneFile(SOCKET s, sockaddr* to, int tolen) {
	package* pac ;
	char filename[FILENAMELEN] = {};//�����ļ���
	char* file = new char[1];//�����ļ�����
	unsigned int filelen;//�ļ�����
	short missCount = 0;//��ʱ��¼
	unsigned short window;//���ڴ�С
	ack = 0;//�趨��ʼackֵ
	clock_t start, end;//��ʱ��
	ofstream writer;
	int count = 0;//ͳ�ư�������
	int writelen;


	while (1)//�����ȵȵ�һ��FLAG_FHEADλΪ1�İ�
	{
		struct timeval timeout = { TIMEOUTS,TIMEOUTVS };
		fd_set fdr;
		FD_ZERO(&fdr);
		FD_SET(s, &fdr);
		int ret;
		ret = select(s, &fdr, NULL, NULL, &timeout);
		if (ret < 0) {
			cout << "�����ļ�ͷʱ����socket����" << endl;
			goto ERR;
		}
		else if (ret == 0) {
			//cout << "�ȴ��ļ�ͷ����Ϣ��ʱ" << ++missCount << "��" << endl;
			if (missCount == MISSLIMT)
			{
				cout << "�ļ�ͷ����Ϣ���ճ�ʱ���������䣡" << endl;
				goto ERR;
			}
		}
		else {
			//��Դ��ַ�����ļ������ݰ�
			pac = new package();
			recvfrom(s, (char*)pac, packageSize, 0, to, &tolen);
			if (checksum((char*)pac, packageSize) != 0)
			{
				delete pac;
				continue;
			}
			if (((pac->flag[0] & FLAG_FHEAD) == FLAG_FHEAD) && pac->seq == ack) {
				memcpy(&filelen, pac->data, 4);
				memcpy(filename, pac->data + 4, FILENAMELEN);
				window = pac->window;
				delete[]file;
				delete pac;
				file = new char[filelen];
				break;
			}
		}
	}
	cout << "�ļ�ͷ���ճɹ���" << endl;
	sendACK(s,false, to, tolen);//����ACK ���ackӦ����0������˵����������Ҫ����һ�����ݰ����Ϊ0;

	ack++;
	//��ʼ�����ļ�
	while (1) {
		pac = new package();
		recvfrom(s, (char*)pac, packageSize, 0, to, &tolen);
		if ((checksum((char*)pac, packageSize) != 0) || (pac->seq != ack))//�������������ˣ����߲��ǽ��շ���Ҫ�İ�
		{
			//�ط�ACK��
			sendACK(s, false, to, tolen);
		}
		else if (pac->seq == ack) {//������յ�����ȷ�İ���д���ļ�������ȷ��ACK
			sendACK(s, true, to, tolen);
			ack++;
			if ((pac->flag[0] & FLAG_FEND) == FLAG_FEND)//������յ������ļ�ĩβ��д���ļ��˳�
			{
				writelen = count * DATALENGTH;
				writer.write(file, writelen);
				writer.write(pac->data, pac->taillen);
				break;
			}
			else//�����ļ���β������д��file
			{
				writelen = count * DATALENGTH;
				memcpy(file + writelen, pac->data, DATALENGTH);
				count++;
			}
		}
		delete pac;
	}
	cout << "�ɹ����� " << filename << " �ļ�!" << endl;
WRITE:
	writer.open(filename, ios::binary);
	writer.write(file, filelen);
	writer.close();
	delete[]file;
	return 0;
ERR:
	delete[]file;
	return -1;
}