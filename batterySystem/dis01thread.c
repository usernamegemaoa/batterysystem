#include"battery.h"
#include<time.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdio.h>
void *thread_dis01(void *arg) {
	int running = 1;
	int DIS01fd = 0;
	enum paritymark parity = 'N';
	//enum serialCommand cmd = read_current;
	DIS01fd = openSerial(DIS01_SEIRAL_FILE, O_RDWR, B9600, parity, 8, 1);

	/**
	 * I ＝（x－2.5）×factor ,（单位：安培）
	 *  输入：+-100A dc  输出：2.5+-2.5V dc  factor = 40
	 *  输入：+-200A dc  输出：2.5+-2.5V dc  factor = 80
	 *  输入：+-300A dc  输出：2.5+-2.5V dc  factor = 120
	 *  输入：+-400A dc  输出：2.5+-2.5V dc  factor = 160
	 *  输入：+-500A dc  输出：2.5+-2.5V dc  factor = 200
	 *
	 */
	float factor = 40.0f;

	float value=0.0f;
	float current=0.0f;
//	int fd2;
//	time_t now;
	while (running) {
//		now=time(NULL);
		value = sendSerialTextCurrent(DIS01fd);
		current = (value - 2.5f) * factor;

		pthread_mutex_lock(&s800bm_mutex);
		group_battery.group_current = current;

		pthread_mutex_unlock(&s800bm_mutex);
		AddCurrentData(current);

//		fd2 = open(CURRENT_DATA_FILE,O_CREAT | O_WRONLY | O_TRUNC, 777);
//		if (fd2 < 0) {
//			fprintf(stderr,"create file failed!");
//		} else {
//			write(fd2, &now, sizeof(time_t));
//			write(fd2, &current, sizeof(float));
//			close(fd2);
//		}

		sleep(1);
	}
	return 0;
}
