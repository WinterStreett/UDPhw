#include"rdt.h"
using namespace std;
unsigned int checksum(const char* s, const int length) {
	/*  输入：字符指针
	*		  字符串长度
	*
	*	输出：字符串的校验和
	*/
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


int sendAndWait(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen) {
	/*	功能：向指定目的地发送一个数据包，并等待正确的ACK,ACK确认后才能返回
	*	flags: 0 发送文件头时使用
	*		   1 发送握手信息使用
	*		   2 发送文件尾使用
	*/
	int missCount = 0;
	SOCKADDR_IN from;
	sendto(s, buf, len, 0, to, tolen);
	int a = 0;
	package* recv_pac;
	//下面是等待ACK
	while (1) {
		//新建两个变量，用来承接收到的数据
		struct timeval timeout = { TIMEOUTS,TIMEOUTMS };
		fd_set fdr;
		FD_ZERO(&fdr);
		FD_SET(s, &fdr);
		int ret;
		recv_pac = new package();
		ret = select(s, &fdr, NULL, NULL, &timeout);
		if (ret < 0)//发生错误
		{
			cout << "select发生错误！" << endl;
			delete recv_pac;
			return -1;
		}
		else if (ret == 0)//等待超时
		{
			sendto(s, buf, len, 0, to, tolen);//重发
			missCount++;
			//cout << "第" << missCount << "次超时！" << endl;
			if (missCount == MISSLIMT) {
				delete recv_pac;
				//cout << "超时次数到达上限，发送失败！" << endl;
				return -1;
			}
		}
		else {
			recvfrom(s, (char*)recv_pac, packageSize, 0, (sockaddr*)&from, &tolen);
			//检查这个收到的包是否是受损的，以及ACK字段
			if (checksum((char*)recv_pac, packageSize) != 0) {
				//如果接收到的包受损  那么重发一次
				sendto(s, buf, len, 0, to, tolen);
			}
			else if ((flags==1)&&((recv_pac->flag[0] & (FLAG_ACK|FLAG_SYN)) == (FLAG_ACK | FLAG_SYN)) && (recv_pac->ack == 0)) {
				//握手信息ack
				break;
			}
			else if (!flags && (recv_pac->flag[0] & FLAG_ACK) && (recv_pac->ack == 0)) {
				//文件头信息的ack
				break;
			}
			else if ((flags == 2) && (recv_pac->flag[0] & FLAG_ACK) && (recv_pac->ack == seq + 1)) {
				//文件尾专用
				break;
			}
			else if ((recv_pac->flag[0] & FLAG_ACK) && (recv_pac->ack != seq + 1)) {//是冗余ACK 那么重发一次 没什么用
				sendto(s, buf, len, 0, to, tolen);
			}
		}
		delete recv_pac;
	}
	return 0;
}

void Throughput(unsigned int len, clock_t time) {//计算吞吐率，自动补上单位输出
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
	/*	功能：向目的地址，发送一个完整的文件
*/
	unsigned int base = 1;//设定窗口初始位置
	unsigned int nextseq =  base + WINDOW;
	//unsigned int pacCount = 0;//记录发送的数据包个数
	clock_t start, end;//计时器
	package* pac;
	unsigned int filelen;//文件长度
	unsigned int pacNum;//需要的包总数量-1，不包含最后一个包
	unsigned short taillen;
	char* file;//保存文件的数组
	short missCount = 0;//超时记录
	sockaddr* from = new sockaddr();

	//读取整个文件
	ifstream reader;
	reader.open(filename, ios::binary);
	reader.seekg(0, ios::end);
	filelen = reader.tellg();
	reader.seekg(0, ios::beg);
	file = new char[filelen];//file中保存着文件数据

	reader.read(file, filelen);

	pacNum = filelen / DATALENGTH;//可能有尾部
	taillen = filelen % DATALENGTH;//尾部剩余长度


	//首先将文件名发给接收端
	pac = new package();
	memcpy(pac->data, &filelen, 4);//数据部分的前4个字节是文件长度
	memcpy(pac->data + 4, filename, FILENAMELEN);//数据部分从4开始是文件名
	pac->seq = 0;
	pac->window = WINDOW;
	pac->flag[0] |= FLAG_FHEAD;//FHEAD标志位置一
	pac->checksum = checksum((char*)pac, packageSize);
	start = clock();
	if (sendAndWait(s, (char*)pac, packageSize, 0, to, tolen) < 0) {
		cout << "发送文件头失败！" << endl;
		goto ERR;
	}
	cout << "发送文件名成功！" << endl;
	delete pac;
	//下面开始传输文件有效负荷
	if (pacNum == 0)
		goto LAST;
	while (1) {
		if (nextseq > pacNum) {
			nextseq = pacNum + 1;
		}
		sendWindow(s, file, base, nextseq - 1, to, tolen);//发送一个窗口的数据包

	//	//进入等待ACK状态
		struct timeval timeout = { TIMEOUTS,TIMEOUTMS };
		fd_set fdr;
		FD_ZERO(&fdr);
		FD_SET(s, &fdr);
		int ret = select(s, &fdr, NULL, NULL, &timeout);

		if (ret < 0)//发生错误
		{
			cout << "select发生错误！" << endl;
			goto ERR;
		}
		else if (ret == 0)//等待超时
		{
			missCount++;
			if (missCount == MISSLIMT) {
				cout << "连续超时5次，传输失败！" << endl;
				goto ERR;
			}
		}
		else {
			pac = new package();
			recvfrom(s, (char*)pac, packageSize, 0, from, &tolen);
			if ((checksum((char*)pac, packageSize) == 0)&&(pac->flag[0] & FLAG_ACK) ) {
				//接收到一个完好的ACK包
				if (pac->ack  >= base) {
					base = pac->ack  ;
					if (base == pacNum+1)
						goto LAST;
					nextseq = base + WINDOW;
				}
			}
			delete pac;
			missCount = 0;
		}
	}
	//额外发送最后一部分文件
LAST:	
	pac = new package();
	memcpy(pac->data, file + filelen - taillen, taillen);
	pac->seq = base;
	pac->flag[0] |= FLAG_FEND;//将FEND标志位置位
	pac->taillen = taillen;
	pac->checksum = checksum((char*)pac, packageSize);
	seq = base;
	if (sendAndWait(s, (char*)pac, packageSize, 2, to, tolen) < 0) {
		cout << "最后一部分数据传输失败！" << endl;
		goto ERR;
	}

	end = clock();
	delete pac;
	cout << "成功发送 " << filename << " 文件！" << endl;
	cout << "用时：" << end - start << "毫秒" << "	" << "吞吐率：";
	Throughput(filelen, end - start);
	cout << "一共发了" << base << "个包" << endl;

	//资源清理
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
	for (unsigned int i = left-1; i < right; i++) {
		pac = new package();
		memcpy(pac->data, file + (i * DATALENGTH), DATALENGTH);
		//设置数据包属性
		pac->seq = i + 1;
		pac->checksum = checksum((char*)pac, packageSize);
		//发出去
		sendto(s, (char*)pac, packageSize, 0, to, tolen);
		delete pac;
	}
}