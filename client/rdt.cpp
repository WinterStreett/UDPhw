#include"rdt.h"
using namespace std;
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
	*	flags: 0 �����ļ�ͷʱʹ��
	*		   1 ����������Ϣʹ��
	*		   2 �����ļ�βʹ��
	*/
	int missCount = 0;
	SOCKADDR_IN from;
	sendto(s, buf, len, 0, to, tolen);
	int a = 0;
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
			sendto(s, buf, len, 0, to, tolen);//�ط�
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
			if (checksum((char*)recv_pac, packageSize) != 0) {
				//������յ��İ�����  ��ô�ط�һ��
				sendto(s, buf, len, 0, to, tolen);
			}
			else if ((flags==1)&&((recv_pac->flag[0] & (FLAG_ACK|FLAG_SYN)) == (FLAG_ACK | FLAG_SYN)) && (recv_pac->ack == 0)) {
				//������Ϣack
				break;
			}
			else if (!flags && (recv_pac->flag[0] & FLAG_ACK) && (recv_pac->ack == 0)) {
				//�ļ�ͷ��Ϣ��ack
				break;
			}
			else if ((flags == 2) && (recv_pac->flag[0] & FLAG_ACK) && (recv_pac->ack == seq + 1)) {
				//�ļ�βר��
				break;
			}
			else if ((recv_pac->flag[0] & FLAG_ACK) && (recv_pac->ack != seq + 1)) {//������ACK ��ô�ط�һ�� ûʲô��
				sendto(s, buf, len, 0, to, tolen);
			}
		}
		delete recv_pac;
	}
	return 0;
}

void Throughput(unsigned int len, clock_t time) {//���������ʣ��Զ����ϵ�λ���
	double result = len * 8000 / time;
	if (result > 1000000000) {
		result /= 1000000000;
		cout << fixed << setprecision(2) << result << " Gb/s" << endl;
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
		cout << fixed << setprecision(2) << result << " b/s" << endl;
	}
	cout << endl;
}

int sendOneFile(SOCKET s, char* filename, sockaddr* to, int tolen) {
	/*	���ܣ���Ŀ�ĵ�ַ������һ���������ļ�
*/
	unsigned int base = 1;//�趨���ڳ�ʼλ��
	unsigned int nextseq =  base + WINDOW;
	//unsigned int pacCount = 0;//��¼���͵����ݰ�����
	clock_t start, end;//��ʱ��
	package* pac;
	unsigned int filelen;//�ļ�����
	unsigned int pacNum;//��Ҫ�İ�������-1�����������һ����
	unsigned short taillen;
	char* file;//�����ļ�������
	short missCount = 0;//��ʱ��¼
	unsigned int a;
	sockaddr* from = new sockaddr();

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
	pac = new package();
	memcpy(pac->data, &filelen, 4);//���ݲ��ֵ�ǰ4���ֽ����ļ�����
	memcpy(pac->data + 4, filename, FILENAMELEN);//���ݲ��ִ�4��ʼ���ļ���
	pac->seq = 0;
	pac->window = WINDOW;
	pac->flag[0] |= FLAG_FHEAD;//FHEAD��־λ��һ
	pac->checksum = checksum((char*)pac, packageSize);
	start = clock();
	if (sendAndWait(s, (char*)pac, packageSize, 0, to, tolen) < 0) {
		cout << "�����ļ�ͷʧ�ܣ�" << endl;
		goto ERR;
	}
	cout << "�����ļ����ɹ���" << endl;
	delete pac;
	//���濪ʼ�����ļ���Ч����
	if (pacNum == 0)
		goto LAST;
	while (1) {
		if (nextseq > pacNum) {
			nextseq = pacNum + 1;//��ֹ���ʹ���
		}

SEND:	sendWindow(s, file, base, nextseq - 1, to, tolen);//����һ�����ڵ����ݰ�

	//	//����ȴ�ACK״̬
		struct timeval timeout = { TIMEOUTS,TIMEOUTMS };
		fd_set fdr;
		FD_ZERO(&fdr);
		FD_SET(s, &fdr);
		int ret = select(s, &fdr, NULL, NULL, &timeout);

		if (ret < 0)//��������
		{
			cout << "select��������" << endl;
			goto ERR;
		}
		else if (ret == 0)//�ȴ���ʱ
		{
			missCount++;
			if (missCount == MISSLIMT) {
				cout << "������ʱ5�Σ�����ʧ�ܣ�" << endl;
				goto ERR;
			}
			goto SEND;
		}
		else {
			pac = new package();
			recvfrom(s, (char*)pac, packageSize, 0, from, &tolen);//����һ�����ݰ�
			if ((checksum((char*)pac, packageSize) == 0)&&(pac->flag[0] & FLAG_ACK) ) {
				//���յ�һ����õ�ACK��
				if (pac->ack  >= base) {//ֻ��ack��base������ݶԿͻ�����˵�����������
					base = pac->ack  ;//����baseΪack������������Ҫ�����
					if (base == pacNum+1)//���base�����һ�����ݰ�����goto��ר�ŵķ��ͳ���
						goto LAST;
					nextseq = base + WINDOW;//����nextseqΪ�������ڵ�ĩβ+1
				}
			}
			delete pac;
			missCount = 0;
		}
	}
	//���ⷢ�����һ�����ļ�
LAST:	
	pac = new package();
	memcpy(pac->data, file + filelen - taillen, taillen);//�����ݵ����һ���ֿ��������ݰ�
	pac->seq = base;//���Ϊbase
	pac->flag[0] |= FLAG_FEND;//��FEND��־λ��λ
	pac->taillen = taillen;//����β���ֶ�
	pac->checksum = checksum((char*)pac, packageSize);//У���
	seq = base;//sendAndWait�����seqȫ�ֱ����������ȸ���Ϊbase
	if (sendAndWait(s, (char*)pac, packageSize, 2, to, tolen) < 0) {//���Ͳ��ȴ�ack
		cout << "���һ�������ݴ���ʧ�ܣ�" << endl;
		goto ERR;
	}

	end = clock();
	delete pac;
	cout << "�ɹ����� " << filename << " �ļ���" << endl;
	cout << "��ʱ��" << end - start << "����" << "	" << "�����ʣ�";
	Throughput(filelen, end - start);
	cout << "һ������" << base << "����" << endl;

 	//��Դ����
	delete[]file;
	delete from;
	reader.close();
	return 0;

ERR:
	delete[]file;
	reader.close();
	return -1;
}

void sendWindow(SOCKET s,const char* file, unsigned int left, unsigned int right, const sockaddr* to, int tolen) {
	package* pac;
	//left��right�ֱ��Ƿ����������˵���Ҷ˵�
	for (unsigned int i = left-1; i < right; i++) {
		pac = new package();
		memcpy(pac->data, file + (i * DATALENGTH), DATALENGTH);
		//�������ݰ�����
		pac->seq = i + 1;
		pac->checksum = checksum((char*)pac, packageSize);
		//����ȥ
		sendto(s, (char*)pac, packageSize, 0, to, tolen);
		delete pac;
	}
}