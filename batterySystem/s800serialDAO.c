/*
 * s800serialDAO.c
 *
 *  Created on: 2015年12月8日
 *      Author: root
 */

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "battery.h"

float sendSerialTextCurrent(int fd) {

	unsigned char cmdstr[3] = { 0 };

	//清除接收缓存
	while (read(fd, cmdstr, 3)) {
		tcflush(fd, TCIOFLUSH);
	}

	//发送指令
	cmdstr[0] = 0xaa;
	cmdstr[1] = 0xbb;
	cmdstr[2] = 0xcc;

	//发送并接收,出错则重传一次
	int i = 0;
	int hdata,ldata,data;

	float resValue;
	for (i = 0; i < 2; i++) {
		unsigned char readstr[4] = { 0 };
		int readlen;
		readlen = SendToSerialAndRecv(fd, cmdstr, 3, readstr);

		if (readlen == 4 && readstr[0] == 0xaa && readstr[3] == 0xcc) {

			hdata=readstr[1];
			ldata=readstr[2];
			hdata *=256;
			data=hdata+ldata;
			//printf("data=%d     %x\n",data,data);
			resValue=data;
			resValue*=0.00126953125;
			//printf("data=%.6f\n",resValue);
			//resValue = ;/*decodeData(readstr[1], readstr[2]);*/
			if (resValue > 0) {
				i = 2;
			}
		} else {
			resValue = -1;
		}
	}
	//printf("data=%.6f\n",resValue);
	return resValue;
}

/*
 * fd: 串口文件描述符  cmd: 枚举型命令码  battery: 电池ID号
 * 功能： 通过串口向对应ID号的电池发送一条取数据命令，将接收到的数据解析成
 *              浮点型数据，并返回。
 *              返回-1表示设备未响应  -2表示设备返回值不是浮点型范围的数
 *              其它结果正常
 */
float sendSerialTextCommand(int fd, enum serialCommand cmd, int battery) {

	unsigned char cmdstr[3] = { 0 };

	//清除接受缓存
	while (read(fd, cmdstr, 3)) {
		tcflush(fd, TCIOFLUSH);
	}

	//生成发送指令
	cmdstr[0] = battery;
	switch (cmd) {
	case test_voltage:
	case text_res:
	case text_temp:
	case text_cap:
	case charge_discharge_open:
	case charge_discharge_off:
	case read_current:
		cmdstr[1] = (unsigned char) cmd;
		break;
	default:
#ifdef DEBUG_FLAG
		printf("cmd error!\n");

#endif
		return -1;
		//break;
	}
	//：校验和的生成
	cmdstr[2] = genChecksum(cmdstr, 2);

	//发送并接受，若未收到数据或收到的数据有误，则重新传输一次
	int i = 0;
	float resValue;
	for (i = 0; i < 2; i++) {
		unsigned char readstr[4] = { 0 };
		int readlen;
		readlen = SendToSerialAndRecv(fd, cmdstr, 3, readstr);
		if (readlen == 4
				&& genChecksum(readstr, readlen - 1) == readstr[readlen - 1]) {
			//分析接受的数据并返回  ： 按照协议转换成浮点型
			resValue = decodeData(readstr[1], readstr[2]);
#ifdef DEBUG_FLAG
			if (resValue > 0) {
				printf("%d resValue=%f\n", battery,resValue);
			}
#endif
			if (resValue == -2 && cmd == text_res) {
				cmdstr[0] = battery;
				cmdstr[1] = 0x22;
				cmdstr[2] = genChecksum(cmdstr, 2);
				i = 0;
			} else {
				i = 2;
			}
		} else {
			//发送两次接收都失败则返回-1
			resValue = -1;
		}
	}
	return resValue;
}

/*
 * fd: 串口文件描述符  cmd: 枚举型命令码  battery: 电池ID号
 * 功能： 通过串口向对应ID号的电池发送一条控制命令（打开或关闭放电装置），成功返0，失败返-1。
 *              返回-1表示设备未响应  -2表示设备返回值不是浮点型范围的数
 *              其它结果正常
 */
int sendSerialControlCommand(int fd, enum serialCommand cmd, int battery) {

	unsigned char cmdstr[3] = { 0 };

	//清除接受缓存
	while (read(fd, cmdstr, 3)) {
		tcflush(fd, TCIOFLUSH);
	}

	//生成发送指令
	cmdstr[0] = battery;
	switch (cmd) {
	case charge_discharge_open:
	case charge_discharge_off:
		cmdstr[1] = (unsigned char) cmd;
		break;
	default:
#ifdef DEBUG_FLAG
		printf("cmd error!\n");

#endif
		return -1;
		//break;
	}
	//：校验和的生成
	cmdstr[2] = genChecksum(cmdstr, 2);

	//发送并接受，若未收到数据或收到的数据有误，则重新传输一次
	int i = 0;
	int resValue;
	for (i = 0; i < 2; i++) {
		unsigned char readstr[4] = { 0 };
		int readlen;
		readlen = SendToSerialAndRecv(fd, cmdstr, 3, readstr);
		if (readlen == 4
				&& genChecksum(readstr, readlen - 1) == readstr[readlen - 1]
				&& readstr[1] == cmd && readstr[2] == 00) {
			resValue = 0;
			break;
		} else {
			resValue = -1;
		}
	}
	return resValue;

}

/*
 * fd: 文件描述符 psend: 指向待发送的内容的指针  sendlen: 待发送的内容的字节数
 * precv: 指向接收缓存，用于保存接收的数据   reflag: 是否为测内阻的标志 （测内阻时，需等待更长的时间）
 * 功能: 向串口发送数据，并接收数据，若发送数据的1秒后还未收到数据，返回0 在接收到数据后200ms内没有了数据，表示
 *           数据已接收完。
 * 返回值：  接收到的数据的字节数
 */
int SendToSerialAndRecv(int fd, unsigned char *psend, int sendlen,
		unsigned char *precv) {

	write(fd, psend, sendlen);

	int readlen = 0, readonce = 0;
	//int zeroflag = 0;
	if (psend[1] == 0x62) {
		usleep(6000000);
	} else {
		usleep(40000);
	}

	int i = 0;
	for (i = 0; i < 25; i++) {
		if (i != 0) {
			usleep(40000);
		}
		readonce = read(fd, &precv[readlen], 4 - readlen);
		readlen += readonce;
		if (readonce != 0) {
			if (readlen >= 4) {
				i = 25;
			}
		}
	}
/*
#ifdef DEBUG_FLAG
	if (readlen > 0) {
		printf("%02x %02x %02x  \t", psend[0], psend[1], psend[2]);
		printf("readsize=%d\t", readlen);
		printf("%02x %02x %02x %02x \n", precv[0], precv[1], precv[2],
				precv[3]);
	}
#endif
*/
	return readlen;
}

/*
 *  high：数据的高位所在字节   low：数据的低位所在字节
 *  功能： 将该数据按照所定的协议翻译成浮点类型的数
 * 返回值： 返回-2 表示并不是有效的数据 其余正常
 */
float decodeData(unsigned char high, unsigned char low) {
	float res = 0;
	if (high >> 7) {
		res = -2;
	} else {
		unsigned short exp, man;
		exp = high >> 3;
		man = 256 * (high % 8) + low;
		if (exp == 15)
			res = -2;
		if (exp == 0)
			res = ((int) (pow(2.0, exp - 17) * man * 1000.0 + 0.5)) / 1000.0;
		if (exp >= 1 && exp <= 14)
			res = ((int) (pow(2.0, exp - 7) * (1.0 + man * pow(2.0, -11))
					* 1000.0 + 0.5)) / 1000.0;
	}
	return res;
}

unsigned char genChecksum(unsigned char *pstr, unsigned short len) {
	int s = 0, i;
	for (i = 0; i < len; i++)
		s ^= pstr[i];
	return s % 256;
}

float selectMini(float agr[], int length) {
	float min = agr[0];
	int i;
	for (i = 1; i < length; i++)
		min = agr[i] < min ? agr[i] : min;
	return min;
}

float selectMax(float agr[], int length) {
	float max = agr[0];
	int i;
	for (i = 1; i < length; i++)
		max = agr[i] > max ? agr[i] : max;
	return max;
}

/**
 * path: 串口路径  oflags:打开文件的附加选项，即open函数的第二个参数
 * baudrate:波特率  py: 校验位 bits: 数据位 stop: 停止位
 * 返回值: 串口设备的文件描述符
 * 注: 该函数以非阻塞方式打开串口
 */
int openSerial(const char * devFile, int oflags, int baudrate,
		enum paritymark py, int bits, int stop) {
	int fd;
	if ((fd = open(devFile, oflags)) <= 0) {
	    	printftime(2);
		fprintf(stderr,"open %s%s", devFile, strerror(errno));
		return -1;
	}
	struct termios newtio;
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag |= CLOCAL | CREAD;
	newtio.c_cflag &= ~CSIZE;

	//设置数据位
	switch (bits) {
	case 5:
		newtio.c_cflag |= CS5;
		break;
	case 6:
		newtio.c_cflag |= CS6;
		break;
	case 7:
		newtio.c_cflag |= CS7;
		break;
	case 8:
		newtio.c_cflag |= CS8;
		break;
	}

	//设置校验位
	switch (py) {
	//case 's':
	case 'S':
		newtio.c_cflag &= ~PARENB;
		newtio.c_cflag &= ~CSTOPB;
		break;

	case 'O':
		//case 'o':
		newtio.c_cflag |= PARENB;
		newtio.c_cflag |= PARODD;
		newtio.c_iflag |= (INPCK | ISTRIP);
		break;
	case 'E':
		//case 'e':
		newtio.c_cflag |= PARENB;
		newtio.c_cflag &= ~PARODD;
		newtio.c_iflag |= (INPCK | ISTRIP);
		break;
	case 'N':
		//case 'n':
		newtio.c_cflag &= ~PARENB;
		break;
	}

	// 设置波特率，可以增加波特率类型
	switch (baudrate) {
	case 300:
		cfsetispeed(&newtio, B300);
		cfsetospeed(&newtio, B300);
		break;
	case 600:
		cfsetispeed(&newtio, B600);
		cfsetospeed(&newtio, B600);
		break;
	case 1200:
		cfsetispeed(&newtio, B1200);
		cfsetospeed(&newtio, B1200);
		break;
	case 2400:
		cfsetispeed(&newtio, B2400);
		cfsetospeed(&newtio, B2400);
		break;
	case 4800:
		cfsetispeed(&newtio, B4800);
		cfsetospeed(&newtio, B4800);
		break;
	case 9600:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
		break;
	case 19200:
		cfsetispeed(&newtio, B19200);
		cfsetospeed(&newtio, B19200);
		break;
	case 38400:
		cfsetispeed(&newtio, B38400);
		cfsetospeed(&newtio, B38400);
		break;
	case 57600:
		cfsetispeed(&newtio, B57600);
		cfsetospeed(&newtio, B57600);
		break;
	case 115200:
		cfsetispeed(&newtio, B115200);
		cfsetospeed(&newtio, B115200);
		break;
	default:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
		break;
	}

	//设置停止位
	if (stop == 1)
		newtio.c_cflag &= ~CSTOPB;
	else if (stop == 2)
		newtio.c_cflag |= CSTOPB;
	fcntl(fd, F_SETFL, 1);    //设置读串口为非阻塞

	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);
	return fd;
}

/*
 * 关闭串口
 */
void closeSerial(int fd) {
	if (fd >= 0) {
		close(fd);
	}
}
/*
 *
 * 打印当前时间
 */
void printftime(int fd)
{
    char tmbuf[20];
    time_t t = time(NULL);
    strftime(tmbuf,20,"%Y-%m-%d-%H%M%S:",localtime(&t));
    if(fd==1)
    {
    printf("%s",tmbuf);
    }
    else if(fd==2)
    {
	fprintf(stderr,"%s",tmbuf);
    }
}

/*
 * 重定向标准输入输出到文件
 *
 */
void RedirectStdIO(char *szInFile,char* szOutFile,char* szErrFile)
{
    int fd;
    if(NULL != szInFile)
    {
	fd=open(szInFile,O_RDONLY|O_CREAT,0666);
	if(fd>0)
	{
	    if(dup2(fd,STDIN_FILENO)<0)
	    {
		perror("RedirectStdIO dup2 in");
		exit(EXIT_FAILURE);
	    }
	    close(fd);
	}
    }

    if(NULL != szOutFile)
    {
	fd=open(szOutFile,O_WRONLY|O_CREAT|O_APPEND,0666);
	if(fd>0)
	{
	    if(dup2(fd,STDOUT_FILENO)<0)
	    {
		perror("RedirectStdIO dup2 out");
		exit(EXIT_FAILURE);
	    }
	    close(fd);
	}
    }

    if(NULL != szErrFile)
    {
	fd=open(szErrFile,O_WRONLY|O_CREAT|O_APPEND,0666);
	if(fd>0)
	{
	    if(dup2(fd,STDERR_FILENO)<0)
	    {
		perror("RedirectStdIO dup2 error");
		exit(EXIT_FAILURE);
	    }
	    close(fd);
	}
    }
}
