/*
 * serialToPC.c
 *
 *  Created on: 2015年12月28日
 *      Author: smdt
 */
#include"battery.h"
#include<time.h>
#include<fcntl.h>
#include<unistd.h>

void *cmd_serail(void * arg) {
	char readbuffer[256];
	int readsize, itmp;
	int batteryNumber = 0;
	int serialfd = (int) arg;
	int i;
	readsize = 0;
	while (1) {
		memset(readbuffer, 0, sizeof(char) * 100);
		for (i = 0; i < 3; i++) {
			itmp = read(serialfd, readbuffer[readsize], 100);
			if (itmp > 0) {
				readsize += itmp;
				i = 1;
			} else {
				i++;
			}
		}
		if ((readsize > 0)) {
			if ((strncmp(readbuffer, "s800bm", 6) != 0) && (readsize != 8)) {
				write(serialfd, "command formart error!", 22);
			} else {
				switch (readbuffer[6]) {
				case 0x01:
					pthread_mutex_lock(&s800bm_mutex);
					write(serialfd, &group_battery, sizeof(battery_pack));
					pthread_mutex_unlock(&s800bm_mutex);
					break;
				case 0x02:
					batteryNumber = readbuffer[7];
					if (batteryNumber > 0 && batteryNumber <= BATTERY_TOTAL) {
						pthread_mutex_lock(&s800bm_mutex);
						write(serialfd,
								&group_battery.single_bat[batteryNumber - 1],
								sizeof(single_battery));
						pthread_mutex_unlock(&s800bm_mutex);
					} else {
						write(serialfd, "battery number error!", 21);
					}
					break;
				case 0x03:
					pthread_mutex_lock(&s800bm_mutex);
					write(serialfd, &group_battery.group_current, sizeof(float));
					pthread_mutex_unlock(&s800bm_mutex);
					break;
				case 0x04:
					pthread_mutex_lock(&s800bm_mutex);
					write(serialfd, &group_battery.group_current, sizeof(float));
					pthread_mutex_unlock(&s800bm_mutex);
					break;
				default:
					write(serialfd, "no such command!", 16);
					break;
				}
			}
			continue;
		} else {
			break;
		}
	}
//	printf("========================\n");
//	close(clientfd);
	return 0;

}
