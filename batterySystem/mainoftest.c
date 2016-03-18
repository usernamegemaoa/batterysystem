#include "battery.h"
#include <stdio.h>
#include <stdlib.h>


int main()
{
	//1.生成测试用的两个结构体数据
	battery_pack mybattery_pack;
	int i;
	time_t timep;
	time(&timep);
	for(i=0;i<BATTERY_TOTAL;i++)
	{
		mybattery_pack.single_bat[i].voltage=i;
		mybattery_pack.single_bat[i].resistance=i;
		mybattery_pack.single_bat[i].temperature=i;
		mybattery_pack.single_bat[i].capacity=i;
	}
	mybattery_pack.total_voltage=BATTERY_TOTAL+1;
	mybattery_pack.date_time=timep;
	mybattery_pack.group_current = BATTERY_TOTAL-1;
	
	//2、连接数据库
	MYSQL *conn;
	conn = mysql_init(NULL);
	conn = connectToMysql("localhost","root","123456","mysql");
	
	//3、调用数据库函数
	timep=0;
	for(i=0;i<20000;i++)
	{
	    timep+=600;
	writeToMysql(conn,&mybattery_pack);
	}
	
	
	
	return 0;
}
