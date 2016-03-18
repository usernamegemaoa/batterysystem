/*
 * networkCommunication.c
 *
 *  Created on: 2015年12月14日
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include<pthread.h>

#include "battery.h"

#define TCP_SERV_PORT 56011
#define MAX_CONNECT 10

void *cmd_network(void * arg) {

	//fcntl(clientfd, F_SETFL, O_NONBLOCK);

	char readbuffer[100];
	int readsize;
	int batteryNumber = 0;
	int clientfd = (int) arg;
	while (1) {
		memset(readbuffer, 0, sizeof(char) * 100);
		readsize = recv(clientfd, readbuffer, 100, 0);
		if ((readsize > 0)) {
			if ((strncmp(readbuffer, "s800bm", 6) != 0) && (readsize != 8)) {
				send(clientfd, "command formart error!", 22, 0);
			} else {
				switch (readbuffer[6]) {
				case 0x01:
					pthread_mutex_lock(&s800bm_mutex);
					send(clientfd, &group_battery, sizeof(battery_pack), 0);
					pthread_mutex_unlock(&s800bm_mutex);
					break;
				case 0x02:
					batteryNumber = readbuffer[7];
					if (batteryNumber > 0 && batteryNumber <= battery_total) {
						pthread_mutex_lock(&s800bm_mutex);
						send(clientfd,
								&group_battery.single_bat[batteryNumber - 1],
								sizeof(single_battery), 0);
						pthread_mutex_unlock(&s800bm_mutex);
					} else {
						send(clientfd, "battery number error!", 21, 0);
					}
					break;
				case 0x03:
					pthread_mutex_lock(&s800bm_mutex);
					send(clientfd, &group_battery.group_current, sizeof(float),
							0);
					pthread_mutex_unlock(&s800bm_mutex);
					break;
				case 0x04:
					pthread_mutex_lock(&s800bm_mutex);
					send(clientfd, &group_battery.group_current, sizeof(float),
							0);
					pthread_mutex_unlock(&s800bm_mutex);
					break;
				default:
					send(clientfd, "no such command!", 16, 0);
					break;
				}
			}
			continue;
		} else {
			break;
		}
	}
	printftime(1);
	printf("========================\n");
	close(clientfd);
	return 0;

}

void *thread_network_tcp(void *arg) {
	int running = 1;
	int sockfd, TO232PCfd, TO485PCfd;
	int clientfd;
	struct sockaddr_in local_addr;
	struct sockaddr_in client_addr;
	int addr_size;
	int res;

	pthread_t a_thread;

	fd_set rfd_set, wfd_set, efd_set;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	    	printftime(2);
		perror("TCP socket 创建失败\n");
		exit(1);
	}

	fcntl(sockfd, F_SETFL, O_NONBLOCK);

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(TCP_SERV_PORT);
	local_addr.sin_addr.s_addr = INADDR_ANY;

	bzero(&(local_addr.sin_zero), 8);

	if (bind(sockfd, (struct sockaddr *) &local_addr, sizeof(struct sockaddr))
			== -1) {
	    	printftime(2);
		perror("TCP bind 失败\n");
		close(sockfd);
		pthread_exit((void*) -1);
	}

	if (listen(sockfd, MAX_CONNECT) == -1) {
	    	printftime(2);
		perror("TCP listen 失败\n");
		close(sockfd);
		pthread_exit((void*) 1);
	}

	//int running_232 = 1;
	//int TO232PCfd = 0;
	//char buffer[256]=0;
	enum paritymark parity = 'N';
	//enum serialCommand cmd = read_current;
	TO232PCfd = openSerial(TO_PC_232SERIAL_FILE, O_RDWR, B9600, parity, 8, 1);
	if (TO232PCfd < 0) {
	    	printftime(2);
		perror("serial 232 fial!");
		pthread_exit((void*) -1);
	}

	TO485PCfd = openSerial(TO_pc_485SERIAL_FILE, O_RDWR, B9600, parity, 8, 1);
	if (TO485PCfd < 0) {
	    	printftime(2);
		perror("serial 232 fial!");
		pthread_exit((void*) -1);
	}

	//int ci = 0;
	struct timeval time_out;
	time_out.tv_sec = 5;
	time_out.tv_usec = 0;

	while (running) {
		FD_ZERO(&rfd_set);
		FD_ZERO(&wfd_set);
		FD_ZERO(&efd_set);
		FD_SET(sockfd, &rfd_set);
		FD_SET(sockfd, &wfd_set);
		FD_SET(sockfd, &efd_set);

		FD_SET(TO485PCfd, &rfd_set);	//
		FD_SET(TO232PCfd, &rfd_set);	//

		int ret = 0;
		ret = select(sockfd + 1, &rfd_set, &wfd_set, &efd_set, &time_out);
		if (ret == 0) {
			continue;
		}
		if (ret < 0) {
			FD_ZERO(&rfd_set);
			FD_ZERO(&wfd_set);
			FD_ZERO(&efd_set);
	    		printftime(1);
			printf("==============guan bi==========\n");
			close(sockfd);
			exit(-1);
		}

		if (FD_ISSET(sockfd, &rfd_set)) {
			//printf("du dao le ");
			addr_size = sizeof(struct sockaddr_in);
			clientfd = accept(sockfd, (struct sockaddr *) &client_addr,
					(socklen_t *) &addr_size);
			//printf("%s 连接到服务器\n", inet_ntoa(client_addr.sin_addr));
	    		printftime(1);
			printf("clientfd = %d\n", clientfd);
			if (clientfd != -1) {
	    			printftime(1);
				printf("%s 连接到服务器\n", inet_ntoa(client_addr.sin_addr));
			}

			res = pthread_create(&a_thread, NULL, cmd_network,
					(void*) clientfd);
			if (res != 0) {
	    			printftime(2);
				perror("Thread creation failed");
				exit(EXIT_FAILURE);
			}

			//printf("==============33444==========\n");
		}
		if (FD_ISSET(sockfd, &wfd_set)) {
			//printf("ke xie le  ");
		}
		if (FD_ISSET(sockfd, &efd_set)) {
			//printf("yi chang le ");
		}

		if (FD_ISSET(TO485PCfd, &rfd_set)) {//启动485接受线程
			res = pthread_create(&a_thread, NULL, cmd_network,
					(void*) TO485PCfd);
		}

		if (FD_ISSET(TO232PCfd, &rfd_set)) {//启动232接受线程
			res = pthread_create(&a_thread, NULL, cmd_network,
					(void*) TO232PCfd);
		}

	}
	close(sockfd);
	close(TO485PCfd);
	close(TO232PCfd);
	return 0;
}

