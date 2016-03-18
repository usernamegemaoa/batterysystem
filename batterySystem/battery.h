/*
 * battery.h
 *
 *  Created on: 2015年12月3日
 *      Author: root
 */

#ifndef BATTERY_H_
#define BATTERY_H_ 1
#include<termios.h>
#include<pthread.h>
#include<semaphore.h>

#include "mysql/mysql.h"
#define DATABASE_NAME "batterysystem"
#define BATTERY_GROUP "batterygroup"
#define BATTERY_SINGLE "singlebattery"
#define BATTERY_CONFIG "batteryconfig"
#define NETWORK_CONFIG "networkconfig"
#define BATTERY_CURRENT "currentdata"


//连接数据库的参数
#define HOSTNAME "localhost"
#define USERNAME "root"
#define PASSWORD "123456"
#define DATABASENAME "mysql"

/*
#define TEXT_VOLTAGE_CMD 0x60;
#define TEXT_RES_CMD 0x62;
#define TEXT_TEMP_CMD 0x61;
#define TEXT_CAP_CMD 0x70;
#define READ_RES_CMD 0x22;
#define READ_GROUP_CURRENT 0x02;
#define CHARGE_DISCHARGE_OPEN 0x01;
#define CHARGE_DISCHARGE_OFF 0x00;
*/
#define BS01_SEIRAL_FILE "/dev/ttyS3"
#define DIS01_SEIRAL_FILE "/dev/ttyS2"
#define TO_PC_232SERIAL_FILE "/dev/ttyS1"
#define TO_pc_485SERIAL_FILE "/dev/ttyS4"

#define CURRENT_DATA_FILE "/mnt/disk/s800bmData/realtime/current.dat"
#define ERROR_LOG_FILE "/var/log/s800bmsystem/error.log"
#define LOG_FILE "/var/log/s800bmsystem/message.log"
#define RES_FILE "/var/log/s800bmsystem/resistance.data"


#define DEFAULT_BATTERY_TOTAL 55
#define DEFAULT_MAX_VOL	2.3	
#define DEFAULT_MIN_VOL 1.8
#define DEFAULT_MAX_RES 10.0
#define DEFAULT_MIN_CAP 50.0
#define MAX_BATTERY_TOTAL 60

//#define DEBUG_FLAG 1

enum paritymark {
	none = 'N', odd = 'O', even = 'E', space = 'S'
};

enum serialCommand {
	charge_discharge_off = 0x00,
	charge_discharge_open = 0x01,
	read_current = 0x02,
	test_voltage = 0x60,
	text_temp = 0x61,
	text_res = 0x62,
	text_cap = 0x70
};

typedef struct {
	float voltage;
	float temperature;
	float resistance;
	float capacity;
} single_battery;

typedef struct {
	time_t date_time;
	float total_voltage;
	float group_current;
	single_battery single_bat[MAX_BATTERY_TOTAL];
} battery_pack;

int battery_total;

battery_pack group_battery; //定义一个全局结构

pthread_mutex_t s800bm_mutex; //定义一个互斥锁

/**
 * path: 串口路径
 * oflags:
 */
int openSerial(const char * devFile, int oflags, int baudrate,
		enum paritymark py, int bits, int stop);

void RedirectStdIO(char *szInFile,char *szOutFile,char* szErrFile);
void printftime(int fd);

void closeSeral(int fd);
float selectMini(float *agr, int length);
float selectMax(float *agr, int length);
float sendSerialTextCommand(int fd, enum serialCommand cmd, int battery);
int sendSerialControlCommand(int fd, enum serialCommand cmd, int battery);
void writeToMysql(MYSQL *mysql,battery_pack *bp);
void deleteOldData(MYSQL *mysql);
MYSQL* connectToMysql(char* hostname,char* username,char* password,char* databasename);
void CreateNetworkConfig();
void CreateBatteryConfig();
void AddCurrentData(float current);
int getBatterytotal();

unsigned char genChecksum(unsigned char *pstr, unsigned short len);
float decodeData(unsigned char high, unsigned char low);
int SendToSerialAndRecv(int fd, unsigned char *psend, int sendlen,
		unsigned char *precv);

void *thread_dis01(void *arg);
void *thread_bs01(void *arg);
void *thread_network_tcp(void *arg);
float sendSerialTextCurrent(int fd);

#endif /* BATTERY_H_ */
