#include"battery.h"
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

void control_voltage(int fd) {
	float MinValue = 0;
	float MaxValue = 0;
	float bat_zu[battery_total];
	enum serialCommand cmd = test_voltage;
	int i = 1;

	for (i = 1; i <= battery_total; i++) {
		float value;
		cmd = test_voltage;
		value = sendSerialTextCommand(fd, cmd, i);

		if (value > 0) {
			bat_zu[i - 1] = value;
			if (value > MaxValue) {
				MaxValue = value;
			}
			if (value < MinValue) {
				MinValue = value;
			}
		} else {
			bat_zu[i - 1] = 0;
		}

		if(i==1){
			MaxValue = value;
			MinValue = value;
		}
	}

	for (i = 1; i <= battery_total; i++) {
		if (MinValue < bat_zu[i - 1] - 0.03) { //比电池组中最低电压蓄电池的电压高0.05v的蓄电池进行放电
			cmd = charge_discharge_open;
			sendSerialControlCommand(fd, cmd, i);
		} else {
			cmd = charge_discharge_off;
			sendSerialControlCommand(fd, cmd, i);
		}
	}
	pthread_mutex_lock(&s800bm_mutex);
	for (i = 1; i < battery_total + 1; i++) {
		group_battery.single_bat[i - 1].voltage = bat_zu[i - 1];
	}
	pthread_mutex_unlock(&s800bm_mutex);

}

void *thread_bs01(void *arg) {
	int running = 1;
	int BS01fd = 0;
	int resfd = 0;
	time_t t_voltage, t_resistance, t_now, t_control;
	enum paritymark parity = 'N';
	enum serialCommand cmd = test_voltage;
	battery_pack mybattery_pack;
	float total_voltage = 0;

	//获取电池总数
	battery_total = getBatterytotal();

	t_voltage = time(NULL);
	t_resistance = t_voltage;
	t_now = t_voltage;
	t_control = t_voltage;

	BS01fd = openSerial(BS01_SEIRAL_FILE, O_RDWR, B9600, parity, 8, 1);
	


	//打开内阻文件，若为空或者字段数少于battery_total+2个，t_voltage,t_resistance不改变，获取到已测过内阻的电池节数
	//在接下来的测量中，不再测量内阻，同时要判断现在时间是否大于存储的t_resistance，若大于则内阻也需要重新测量

	time_t t_old_vol=0,t_old_res=0;
	int old_batterytotal=0;
	int old_count=0;
	resfd=open(RES_FILE,O_RDONLY);
	if(resfd>0)
	{
    	    int size;
    	    char allfile[1024]={0},*buff,*p;
    	    size = read(resfd,allfile,1024);
	    close(resfd);
	    if(size>0)
	    {
		buff = allfile;
		p=strsep(&buff,"\n");
		char *pstr;
		if((pstr=strsep(&p,"\t"))!=NULL)
		{
		    old_batterytotal = atoi(pstr);
		}
		if((pstr=strsep(&p,"\t"))!=NULL)
		{
		    t_old_vol = atoi(pstr);
		}
		if((pstr=strsep(&p,"\t"))!=NULL)
		{
		    t_old_res = atoi(pstr);
		}
		p=strsep(&buff,"\n");
		while(((pstr=strsep(&p,"\t"))!=NULL)&&(strlen(pstr)!=0))
		{
		    mybattery_pack.single_bat[old_count++].resistance = atof(pstr);
		}

	    }
	
	}
	if((battery_total==old_batterytotal)&&(old_count==battery_total)&&(t_voltage<t_old_vol))
	{
	    t_voltage = t_old_vol;
	}
	if((battery_total==old_batterytotal)&&(t_resistance<t_old_res))
	{
	    t_resistance = t_old_res;
	}

	int i = 0;
	while (running) {

	    //刷新输入输出缓存
	    fflush(stderr);
	    fflush(stdout);
		/*巡检电池*/
		if (t_now >= t_voltage || t_now >= t_resistance) {
			t_voltage = time(NULL) + (60 * 10); //10分钟测试一次电压
			total_voltage=0;


			//创建内阻文件，并写入信息
			resfd = open(RES_FILE,O_WRONLY|O_CREAT|O_TRUNC,0666);
			char tmp[256]={0};
			sprintf(tmp,"%d\t%ld\t%ld\n",battery_total,t_voltage,(time(NULL)+24*3600));
			write(resfd,tmp,strlen(tmp));

			for (i = 1; i <= battery_total; i++) {
				float value = 0;
				cmd = test_voltage; //测试第i节电池电压
				value = sendSerialTextCommand(BS01fd, cmd, i);
				if (value > 0) {
					mybattery_pack.single_bat[i - 1].voltage = value;
					total_voltage += value;
				} else {
					mybattery_pack.single_bat[i - 1].voltage = 0;
				}

				if (t_now >= t_resistance||i>old_count) { //查询是否到了测试内阻的时间
					value = 0;
					cmd = text_res; //测试第i节电池内阻
					value = sendSerialTextCommand(BS01fd, cmd, i);
					if (value > 0) {
						mybattery_pack.single_bat[i - 1].resistance = value;
					} else {
						mybattery_pack.single_bat[i - 1].resistance = 0;
					}
					if (i == battery_total) {
						t_resistance = time(NULL) + (60 * 60 * 24); //一天测试一次内阻
					}
				}
					sprintf(tmp,"%.2f\t",mybattery_pack.single_bat[i-1].resistance);
					write(resfd,tmp,strlen(tmp));

				value = 0;
				cmd = text_temp; //测试第i节电池温度
				value = sendSerialTextCommand(BS01fd, cmd, i);
				if (value > 0) {
					mybattery_pack.single_bat[i - 1].temperature = value;
				} else {
					mybattery_pack.single_bat[i - 1].temperature = 0;
				}

				cmd = text_cap; //测试第i节电池容量
				value = sendSerialTextCommand(BS01fd, cmd, i);
				if (value > 0) {
					mybattery_pack.single_bat[i - 1].capacity = value;
				} else {
					mybattery_pack.single_bat[i - 1].capacity = 0;
				}
			}
			close(resfd);

			mybattery_pack.total_voltage = total_voltage; //更新蓄电池总电压

			mybattery_pack.date_time = t_now; //保存测试时间
			pthread_mutex_lock(&s800bm_mutex);
			mybattery_pack.group_current = group_battery.group_current;
			memcpy(&group_battery, &mybattery_pack, sizeof(battery_pack)); //更新全局蓄电池组数据

			pthread_mutex_unlock(&s800bm_mutex);
			
			//连接数据库
			MYSQL *conn;
			conn = mysql_init(NULL);
			conn = connectToMysql(HOSTNAME,USERNAME,PASSWORD,DATABASENAME);
			//调用数据库函数
			printftime(1);
			printf("begin to write batterydata to mysql\n");
			writeToMysql(conn,&group_battery);
			
			deleteOldData(conn);
			mysql_close(conn);
			printftime(1);
			printf("finish -------------\n");
		}

		if (t_now >= t_control) {
			t_control = time(NULL) + 1 * 60;
			control_voltage(BS01fd);
		} else {
			sleep(5);
		}
		t_now = time(NULL);
		//printf("t_now=%ld\n",t_now);
	}
	return 0;
}

