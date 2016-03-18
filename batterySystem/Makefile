##s800bm system makefile

CC = arm-linux-gnueabihf-gcc
CFLAGS = -Wall -O -g
EXEC = s800bmsystem
OBJS = bs01thread.o dis01thread.o manageData.o monitor.o s800serialDAO.o networkCommunication.o
all:$(EXEC)

s800bmsystem:$(OBJS)
	$(CC) $(OBJS) -o $@ -L ./mysql/ -lmysqlclient -lpthread -lm 

bs01thread.o: bs01thread.c battery.h
	$(CC) $(CFLAGS) -c $< -o $@ 

dis01thread.o: dis01thread.c battery.h
	$(CC) $(CFLAGS) -c $< -o $@
 
manageData.o: manageData.c battery.h
	$(CC) $(CFLAGS) -c $< -o $@ 

monitor.o: monitor.c battery.h
	$(CC) $(CFLAGS) -c $< -o $@ 

s800serialDAO.o: s800serialDAO.c battery.h
	$(CC) $(CFLAGS) -c $< -o $@ 

networkCommunication.o: networkCommunication.c battery.h
	$(CC) $(CFLAGS) -c $< -o $@ 
clean:
	rm -f $(OBJS) s800bmsystem
