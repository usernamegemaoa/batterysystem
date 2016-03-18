/*
 * s800bmShareMerry.c
 *
 *  Created on: 2015年12月7日
 *      Author: root
 */
/*
#include<sys/sem.h>
int s800_set_semvalue(int isem_id,int val) {
	union semun sem_union;
	sem_union.val = val;
	if (semctl(isem_id, 0, SETVAL,sem_union) == -1) {
		return 0;
	}
	return 1;
}
void s800_del_semvalue(int isem_id) {
	union semun sem_union;
	//sem_union.val = 1;
	if (semctl(isem_id, 0, SETVAL,sem_union) == -1) {
		fprintf(stderr, "Failed to delete semaphore\n");
	}
}

int s800_semaphore_p(int isem_id) {
	struct sembuf sem_b;
	sem_b.sem_num = 0;
	sem_b.sem_op = -1;
	sem_b.sem_flg = SEM_UNDO;
	if (semop(isem_id, &sem_b, 1) == -1) {
		fprintf(stderr, "semaphore_p failed\n");
		return 0;
	}
	return 1;
}

int s800_semaphore_v(int isem_id) {
	struct sembuf sem_b;
	sem_b.sem_num = 0;
	sem_b.sem_op = 1;
	sem_b.sem_flg = SEM_UNDO;
	if (semop(isem_id, &sem_b, 1) == -1) {
		fprintf(stderr, "semaphore_v failed\n");
		return 0;
	}
	return 1;
}
*/
