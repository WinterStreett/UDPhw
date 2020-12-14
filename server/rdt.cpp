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
	return i;//这个数值是下一个期望收到的ack
}

void cacheManager::initBuf() {
	for (int i = 0; i < window; i++) {
		indexBuf[i] = false;
	}
}

unsigned int checksum(const char* s, const int length) {
	unsigned int sum = 0;
	unsigned short temp1 = 0;//这里必须是无符号数，否则会因为符号扩展，得到一些奇奇怪怪的数
	unsigned short temp2 = 0;
	for (int i = 0; i < length; i += 2) {
		temp1 = s[i + 1] << 8;
		temp2 = s[i] & 0x00ff;//一定要把高8位置零！否则如果低8位最高位是1，那么高8位也会因为符号扩展而全为1
		sum += temp1 | temp2;
	}
	while (sum > 0xffff) {//把加法溢出的部分（存在高16位）加到低16位上
		sum = (sum & 0xffff) + (sum >> 16);
	}
	return ~(sum | 0xffff0000);//低16位取反，高16位全为0
}


void sendACK(SOCKET s,bool flag, const sockaddr* to, int tolen) {
	/*	功能：向目的地址发送一个ACK
	*	redun：为真时，说明要发送的是一个冗余ACK，ack值不能变
	*/

	package* ack_pac = new package();
	ack_pac->flag[0] |= FLAG_ACK;//设置ACK标志位
	if (flag)
		ack_pac->ack = (ack + 1) % 0xffffffff;
	else
	{
		ack_pac->ack = ack ;
	}

	ack_pac->checksum = checksum((char*)ack_pac, packageSize);//填上校验和

	sendto(s, (char*)ack_pac, packageSize, 0, to, tolen);
	delete ack_pac;
}

int recvOneFile(SOCKET s, sockaddr* to, int tolen) {
	package* pac ;
	char filename[FILENAMELEN] = {};//保存文件名
	char* file = new char[1];//保存文件数据
	unsigned int filelen;//文件长度
	short missCount = 0;//超时记录
	unsigned short window;//窗口大小
	ack = 0;//设定初始ack值
	clock_t start, end;//定时器
	ofstream writer;
	int count = 0;//统计包的数量
	int writelen;

	while (1)//必须先等到一个FLAG_FHEAD位为1的包
	{
		struct timeval timeout = { TIMEOUTS,TIMEOUTVS };
		fd_set fdr;
		FD_ZERO(&fdr);
		FD_SET(s, &fdr);
		int ret;
		ret = select(s, &fdr, NULL, NULL, &timeout);
		if (ret < 0) {
			cout << "接收文件头时发生socket错误！" << endl;
			goto ERR;
		}
		else if (ret == 0) {
			//cout << "等待文件头部信息超时" << ++missCount << "次" << endl;
			if (missCount == MISSLIMT)
			{
				cout << "文件头部信息接收超时！放弃传输！" << endl;
				goto ERR;
			}
		}
		else {
			//从源地址接收文件名数据包
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
	cout << "文件头接收成功！" << endl;
	sendACK(s,false, to, tolen);//发送ACK 这个ack应该是0，不是说服务器端想要的下一个数据包序号为0;
	writer.open(filename, ios::binary);

	ack++;
	//开始接收文件
	while (1) {
		struct timeval timeout = { TIMEOUTS,TIMEOUTVS };
		fd_set fdr;
		FD_ZERO(&fdr);
		FD_SET(s, &fdr);
		int ret= select(s, &fdr, NULL, NULL, &timeout);
		if (ret < 0)//发生错误
		{
			cout << "select发生错误！" << endl;
			goto ERR;
		}
		else if (ret == 0)//等待超时
		{
			sendACK(s, false, to, tolen);
			missCount++;
			if (missCount == MISSLIMT) {
				cout << "连续超时五次，退出接收" << endl;
				goto ERR;
			}
		}
		else {//接收到一个包
			pac = new package();
			recvfrom(s, (char*)pac, packageSize, 0, to, &tolen);
			if (checksum((char*)pac, packageSize) != 0)//如果这个包被损坏了
			{
				continue;
			}
			else if (pac->seq == ack) {//如果接收到了正确的包，写入文件先不返回ACK
				if (pac->flag[0] & FLAG_FEND)//如果接收到的是文件末尾，写入文件退出
				{
					writelen = count * DATALENGTH;
					memcpy((char*)(file + writelen), pac->data, pac->taillen);//将文件尾写入file缓冲区
					writer.write(file, filelen);//将缓冲区写入文件
					sendACK(s, true, to, tolen);//发送ack
					cout << "成功接收 " << filename << " 文件!" << endl;
					cout << "一共接收了：" << ++count << "个包" << endl;
					goto WRITE;
				}
				else//不是文件结尾，正常写入file
				{
					writelen = count * DATALENGTH;
					memcpy((char*)(file + writelen), pac->data, DATALENGTH);//将数据写入file缓冲区
					count++;
				}
				ack++;
			}
			delete pac;
			missCount = 0;
		}
	}
WRITE:
	//cout << file << endl;
	writer.close();
	delete[]file;
	return 0;
ERR:
	delete[]file;
	return -1;
}