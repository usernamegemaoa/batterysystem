#include <stdio.h>
#include <time.h>
#include <linux/rtc.h>
#include <sys/time.h>

/************************************************ 
 * 设置操作系统时间 
 * 参数:*dt数据格式为"2006-4-20 20:30:30" 
 * 调用方法: 
 * char *pt="2006-4-20 20:30:30"; 
 * SetSystemTime(pt); 
 ***************************************************/
int SetSystemTime(char *dt)  
{  
    struct rtc_time tm;  
    struct tm _tm;  
    struct timeval tv;  
    time_t timep;  
    sscanf(dt, "%d-%d-%d %d:%d:%d", &tm.tm_year,  
        &tm.tm_mon, &tm.tm_mday,&tm.tm_hour,  
        &tm.tm_min, &tm.tm_sec);  
    _tm.tm_sec = tm.tm_sec;  
    _tm.tm_min = tm.tm_min;  
    _tm.tm_hour = tm.tm_hour;  
    _tm.tm_mday = tm.tm_mday;  
    _tm.tm_mon = tm.tm_mon - 1;  
    _tm.tm_year = tm.tm_year - 1900;  
  
    timep = mktime(&_tm);  
    tv.tv_sec = timep;  
    tv.tv_usec = 0;  
    if(settimeofday (&tv, (struct timezone *) 0) < 0)  
    {  
    printf("Set system datatime error!/n");  
    return -1;  
    }  
    return 0;  
}  

int main(){
//    time_t now;
//    now=time(NULL);
      time_t now;//实例化time_t结构
      struct tm *timenow;//实例化tm结构指针
      time(&now);//time函数读取现在的时间(国际标准时间非北京时间)，然后传值给now
      timenow = localtime(&now);
      //localtime函数把从time取得的时间now换算成你电脑中的时间(就是你设置的地区)
      printf("Local   time   is   %s/n",asctime(timenow));
      //上句中asctime函数把时间转换成字符，通过printf()函数输出
   // time_t now;
   // struct tm *tm_now;
      char datetime[200];
 

SetSystemTime("2015-12-15 10:54:00");

// time(&now);
// tm_now = localtime(&now);
      strftime(datetime, 200, "%x %X %n%Y-%m-%d %H:%M:%S %nzone: %Z\n", timenow);
      printf("now datetime : %s\n", datetime);

      printf("nowtime = %ld\n",now);
long int inow= 92800;
      printf("nowtime = %lx\n",inow);
      printf("nowtime = int:%d   long :%d  time_t:%d\n",sizeof(int),sizeof(long int),sizeof(time_t));
      return(0);
}
