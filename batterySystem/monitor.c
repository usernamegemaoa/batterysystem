/*
 * monitor.c
 *
 *  Created on: 2015年12月3日
 *      Author: root
 */
#include<fcntl.h>
#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/ipc.h>

#include"battery.h"

//#define BS01_SEIRAL_FILE "/dev/ttyS3"
//#define DIS01_SEIRAL_FIEL "/dev/ttyS4"
int main() {
	int res;
	pthread_t a_thread;
	pthread_t b_thread;
	//void *thread_result;
	//int sem_id=0;

	//重定向标准输入输出到文件
	RedirectStdIO("/dev/null",LOG_FILE,ERROR_LOG_FILE);

	printftime(1);
	printf("s800bmsystem is starting\n");
	fflush(stdout);

	//创建配置信息表
	CreateNetworkConfig();
	CreateBatteryConfig();

	res=pthread_mutex_init(&s800bm_mutex,NULL);//初始化互斥锁


	res=pthread_create(&a_thread,NULL,thread_bs01,0);
	if(res!=0){
	    	printftime(2);
		perror("Thread creation failed");
		exit(EXIT_FAILURE);
	}

	res=pthread_create(&b_thread,NULL,thread_dis01,0);
	if(res!=0){
	    	printftime(2);
		perror("Thread creation failed");
		exit(EXIT_FAILURE);
	}

	res=pthread_create(&b_thread,NULL,thread_network_tcp,0);
	if(res!=0){
	    	printftime(2);
		perror("Thread creation failed");
		exit(EXIT_FAILURE);
	}

	while(1){

	}
	pthread_mutex_destroy(&s800bm_mutex);

}

