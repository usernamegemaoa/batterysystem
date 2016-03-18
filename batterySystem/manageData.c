/*
 * manageData.c
 *
 *  Created on: 2015年12月7日
 *      Author: root
 */
#include<time.h>
#include<fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include"battery.h"


MYSQL* connectToMysql(char* hostname,char* username,char* password,char* databasename)
{
	MYSQL *conn;
	conn = mysql_init(NULL);
	if(mysql_real_connect(conn,hostname,username,password,databasename,0,NULL,0))
	{
		return conn;
	}
	else
	{
	    	printftime(2);
	    	fprintf(stderr,"connect failed:%s\n",mysql_error(conn));
		return NULL;
	}
}

/*
*
*
*保存至MySQL数据库
*/
void writeToMysql(MYSQL *mysql,battery_pack *bp)
{
	
	time_t timep;
	struct tm *p;
	int i;
	MYSQL_RES *results;
	//MYSQL_ROW row;
	char querystring[512] = {0};
	char timestring[15] = {0};
	
	if(mysql_select_db(mysql,DATABASE_NAME))
	{
	    sprintf(querystring,"CREATE DATABASE %s;",DATABASE_NAME);
    	    mysql_real_query(mysql,querystring,512);
	    results = mysql_store_result(mysql);
	    mysql_free_result(results);
	    if(mysql_select_db(mysql,DATABASE_NAME))
	    {
		printftime(2);
		fprintf(stderr,"create database %s error:%s\n",DATABASE_NAME,mysql_error(mysql));
		return;
	    }
	}
	
	
	//1、建立一张总表，包含时间，总电压，电流，环境温度
	sprintf(querystring,"CREATE TABLE if not exists %s(batterytime datetime primary key not null,\
			batterytotal int not null,sumofvol float(9,3) not null,current float(9,3) not null,temperature float(9,1) not null);",
			BATTERY_GROUP);
	mysql_real_query(mysql,querystring,512);
	results = mysql_store_result(mysql);
	mysql_free_result(results);
//	printf("%s\n",querystring);
	
	//在添加当日数据时，先检查当日是否已产生数据，若有，则删除
//	sprintf(querystring,"delete from %s where date(batterytime)=date(NOW());",BATTERY_GROUP);
//	mysql_real_query(mysql,querystring,512);
//	results = mysql_store_result(mysql);
//	mysql_free_result(results);
	
	//插入电池组记录
	time(&timep);
	p = localtime(&timep);
//	printf("%s\n",querystring);
	sprintf(timestring,"%04d%02d%02d%02d%02d%02d",1900 + p->tm_year,1 + p->tm_mon,
			p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec);
			
	sprintf(querystring,"INSERT INTO %s value((select str_to_date(%s,'%%Y%%m%%d%%H%%i%%s')),\
			%d,%.3f,%.3f,%.3f);",BATTERY_GROUP,timestring,battery_total,bp->total_voltage,bp->group_current,
			bp->single_bat[0].temperature);
//	printf("%s\n",querystring);
	mysql_real_query(mysql,querystring,512);
	results = mysql_store_result(mysql);
	mysql_free_result(results);
	
	//获取电池组记录中的子表名字字段
	/*sprintf(querystring,"select sontablename from %s where batterytime=(select \
			MAX(batterytime) from %s);",BATTERY_GROUP,BATTERY_GROUP);
	mysql_real_query(mysql,querystring,512);
	results=mysql_store_result(mysql);
	row = mysql_fetch_row(results);
	char sontablename[30]={0};
	sprintf(sontablename,"%s",row[0]);
	mysql_free_result(results);
	*/
	//删除表名含有当前日期的表
/*	time(&timep);
	p = localtime(&timep);
	sprintf(querystring,"select table_name from information_schema.tables where \
			table_name like 't%04d%02d%02d_%%';",1900 + p->tm_year,1 + p->tm_mon,
			p->tm_mday);
	mysql_real_query(mysql,querystring,512);
	results=mysql_store_result(mysql);
	if(mysql_num_rows(results))
	{
	    mysql_free_result(results);
	    sprintf(querystring,"select CONCAT('drop table ',group_concat(table_name),';') \
				from information_schema.tables where table_name like 't%04d%02d%02d_%%';",
				1900 + p->tm_year,1 + p->tm_mon, p->tm_mday);
	    mysql_real_query(mysql,querystring,512);
	    results=mysql_store_result(mysql);
	    row = mysql_fetch_row(results);
	    sprintf(querystring,"%s",row[0]);
	    mysql_free_result(results);
	    mysql_real_query(mysql,querystring,512);
	    results=mysql_store_result(mysql);
	    mysql_free_result(results);
	}
	else
	{
	    mysql_free_result(results);
	}
*/	
	//2.建立一张表，表项有时间，电池ID，电压，内阻，温度，容量
	sprintf(querystring,"CREATE TABLE if not exists %s(batterytime datetime not null,batteryid int not null,\
			voltage float(9,3) not null,resistance float(9,2) not null,capacity float(9,1) not null,PRIMARY KEY (batterytime,batteryid));",BATTERY_SINGLE);
//	printf("%s\n",querystring);
	mysql_real_query(mysql,querystring,512);
	results=mysql_store_result(mysql);
	mysql_free_result(results);
	
	//利用模板表创建新表
	/*sprintf(querystring,"CREATE TABLE if not exists %s like singlebattery;",sontablename);
	mysql_real_query(mysql,querystring,512);
	results=mysql_store_result(mysql);
	mysql_free_result(results);
	*/
	//添加单个电池信息记录
	for(i=0;i<battery_total;i++)
	{
		sprintf(querystring,"insert into %s value((select str_to_date(%s,'%%Y%%m%%d%%H%%i%%s')),%d,%.3f,%.3f,%.3f);",BATTERY_SINGLE,timestring,i+1,bp->single_bat[i].voltage,
				bp->single_bat[i].resistance,bp->single_bat[i].capacity);
		mysql_real_query(mysql,querystring,512);
		results=mysql_store_result(mysql);
		mysql_free_result(results);
	}
	
	
}


/*
*更新数据库，执行数据库的删减操作
*/
void deleteOldData(MYSQL *mysql)
{
	MYSQL_RES *results;
	//MYSQL_ROW row;
	char querystring[512] = {0};
	//检查表中记录的第一个字段，日期与当前日期对比，获取记录的sontablename字段，然后删除以该字段为名字的表
	/*sprintf(querystring,"select sontablename from batterydata where datediff(date(batterytime)，date(NOW()))>183;");
	mysql_real_query(mysql,querystring,512);
	results=mysql_store_result(mysql);
	if(mysql_num_rows(results))
	{
	    mysql_free_result(results);
		sprintf(querystring,"select concat('drop table ',group_concat(sontablename),';') \
				from batterydata where datediff(date(batterytime)，date(NOW()))>183;");
		mysql_real_query(mysql,querystring,512);
		results=mysql_store_result(mysql);
		row = mysql_fetch_row(results);
		sprintf(querystring,"%s",row[0]);
		mysql_free_result(results);
		mysql_real_query(mysql,querystring,512);
		results=mysql_store_result(mysql);
		mysql_free_result(results);
	}
	else
	{
		mysql_free_result(results);
	}
	*/
	//删除第一张表记录
	sprintf(querystring,"delete from %s where datediff(date(batterytime)，date(NOW()))>183;",BATTERY_GROUP);
	mysql_real_query(mysql,querystring,512);
	results = mysql_store_result(mysql);
	mysql_free_result(results);
	//删除第二张表记录
	sprintf(querystring,"delete from %s where datediff(date(batterytime)，date(NOW()))>183;",BATTERY_SINGLE);
	mysql_real_query(mysql,querystring,512);
	results = mysql_store_result(mysql);
	mysql_free_result(results);
}
void CreateBatteryConfig()
{
    MYSQL *conn;
    conn = mysql_init(NULL);
    conn = connectToMysql(HOSTNAME,USERNAME,PASSWORD,DATABASENAME);
    MYSQL_RES *results;
    //MYSQL_ROW row;
    char querystring[512] = {0};
	
    if(mysql_select_db(conn,DATABASE_NAME))
    {
	sprintf(querystring,"CREATE DATABASE %s;",DATABASE_NAME);
	mysql_real_query(conn,querystring,512);
	results = mysql_store_result(conn);
	mysql_free_result(results);
	if(mysql_select_db(conn,DATABASE_NAME))
	{
	    printftime(2);
	    fprintf(stderr,"create database %s error:%s\n",DATABASE_NAME,mysql_error(conn));
	    return;
	}
    }
    sprintf(querystring,"CREATE TABLE if not exists %s(ID int primary key not null auto_increment,\
	battery_total int not null,max_voltage float(9,3) not null,min_voltage float(9,3) not null,max_resistance float(9,2) not null,min_capacity float(9,1) not null);",
	    BATTERY_CONFIG);
    mysql_real_query(conn,querystring,512);
    results = mysql_store_result(conn);
    mysql_free_result(results);
    sprintf(querystring,"select battery_total from %s where ID=1;",BATTERY_CONFIG);
    mysql_real_query(conn,querystring,512);
    results = mysql_store_result(conn);
    if(mysql_num_rows(results)==0)
    {
	mysql_free_result(results);
	sprintf(querystring,"insert into %s value(1,%d,%.3f,%.3f,%.2f,%.1f);",BATTERY_CONFIG,DEFAULT_BATTERY_TOTAL,DEFAULT_MAX_VOL,DEFAULT_MIN_VOL,DEFAULT_MAX_RES,DEFAULT_MIN_CAP);
	mysql_real_query(conn,querystring,512);
    	results = mysql_store_result(conn);
    	mysql_free_result(results);
    }

    mysql_close(conn);

}

void CreateNetworkConfig()
{
    MYSQL *conn;
    conn = mysql_init(NULL);
    conn = connectToMysql(HOSTNAME,USERNAME,PASSWORD,DATABASENAME);
    MYSQL_RES *results;
    //MYSQL_ROW row;
    char querystring[512] = {0};
	
    if(mysql_select_db(conn,DATABASE_NAME))
    {
	sprintf(querystring,"CREATE DATABASE %s;",DATABASE_NAME);
	mysql_real_query(conn,querystring,512);
	results = mysql_store_result(conn);
	mysql_free_result(results);
	if(mysql_select_db(conn,DATABASE_NAME))
	{
	    printftime(2);
	    fprintf(stderr,"create database %s error:%s\n",DATABASE_NAME,mysql_error(conn));
	    return;
	}
    }
    sprintf(querystring,"CREATE TABLE if not exists %s(ID int primary key not null auto_increment,\
	ip_address varchar(15) not null,netmask varchar(15) not null,gateway varchar(15) not null,dns1 varchar(15) not null,dns2 varchar(15) not null);",
	    NETWORK_CONFIG);
    mysql_real_query(conn,querystring,512);
    results = mysql_store_result(conn);
    mysql_free_result(results);

    mysql_close(conn);

}

int getBatterytotal()
{
    MYSQL *conn;
    conn = mysql_init(NULL);
    conn = connectToMysql(HOSTNAME,USERNAME,PASSWORD,DATABASENAME);
    MYSQL_RES *results;
    MYSQL_ROW row;
    char querystring[512] = {0};
	
    if(mysql_select_db(conn,DATABASE_NAME))
    {
	sprintf(querystring,"CREATE DATABASE %s;",DATABASE_NAME);
	mysql_real_query(conn,querystring,512);
	results = mysql_store_result(conn);
	mysql_free_result(results);
	if(mysql_select_db(conn,DATABASE_NAME))
	{
	    printftime(2);
	    fprintf(stderr,"create database %s error:%s\n",DATABASE_NAME,mysql_error(conn));
	    return -1;
	}
    }
    sprintf(querystring,"CREATE TABLE if not exists %s(ID int primary key not null auto_increment,\
	battery_total int not null,max_voltage float(9,3) not null,min_voltage float(9,3) not null,max_resistance float(9,2) not null,min_capacity float(9,1) not null);",
	    BATTERY_CONFIG);
    mysql_real_query(conn,querystring,512);
    results = mysql_store_result(conn);
    mysql_free_result(results);
    sprintf(querystring,"select battery_total from %s where ID=1;",BATTERY_CONFIG);
    mysql_real_query(conn,querystring,512);
    results = mysql_store_result(conn);
    if(mysql_num_rows(results)==0)
    {
	mysql_free_result(results);
	sprintf(querystring,"insert into %s value(1,%d,%.3f,%.3f,%.2f,%.1f);",BATTERY_CONFIG,DEFAULT_BATTERY_TOTAL,DEFAULT_MAX_VOL,DEFAULT_MIN_VOL,DEFAULT_MAX_RES,DEFAULT_MIN_CAP);
	mysql_real_query(conn,querystring,512);
    	results = mysql_store_result(conn);
    	mysql_free_result(results);
	return DEFAULT_BATTERY_TOTAL;
    }
    else
    {
	row = mysql_fetch_row(results);
	sprintf(querystring,"%s",row[0]);
	mysql_free_result(results);
	int value;
	value=atoi(querystring);
	return value;

    }
}

void AddCurrentData(float current)
{
    time_t timep;
    struct tm *p;
    MYSQL *conn;
    conn = mysql_init(NULL);
    conn = connectToMysql(HOSTNAME,USERNAME,PASSWORD,DATABASENAME);
    MYSQL_RES *results;
    //MYSQL_ROW row;
    char querystring[512] = {0};
    char timestring[15]={0};
	
    if(mysql_select_db(conn,DATABASE_NAME))
    {
	sprintf(querystring,"CREATE DATABASE %s;",DATABASE_NAME);
	mysql_real_query(conn,querystring,512);
	results = mysql_store_result(conn);
	mysql_free_result(results);
	if(mysql_select_db(conn,DATABASE_NAME))
	{
	    printftime(2);
	    fprintf(stderr,"create database %s error:%s\n",DATABASE_NAME,mysql_error(conn));
	    return;
	}
    }
	time(&timep);
	p = localtime(&timep);
	sprintf(timestring,"%04d%02d%02d%02d%02d%02d",1900 + p->tm_year,1 + p->tm_mon,
			p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec);
			
    sprintf(querystring,"CREATE TABLE if not exists %s(time datetime primary key not null,current float(9,3) not null);",
	    BATTERY_CURRENT);
    mysql_real_query(conn,querystring,512);
    results = mysql_store_result(conn);
    mysql_free_result(results);
    sprintf(querystring,"INSERT INTO %s value((select str_to_date(%s,'%%Y%%m%%d%%H%%i%%s')),\
			%.3f);",BATTERY_CURRENT,timestring,current);
    mysql_real_query(conn,querystring,512);
    results = mysql_store_result(conn);
    mysql_free_result(results);

    mysql_close(conn);

}
