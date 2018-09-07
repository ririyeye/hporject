#include "ctr.h"

#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

key_t getKey(char *path, int num)
{
	key_t key;

	if (-1 == (key = ftok(path,num)))
	{
		return -1;
	}
	return key;
}


int getMessQuene(key_t key)
{
	int qid;
	if (-1 == (qid = msgget(key, IPC_CREAT| IPC_EXCL | 0666)))
	{
		if (errno == EEXIST)
		{
			if (0 == (qid = msgget(key, 0666)))
			{
				goto ret;
			}
		}
	}
ret:
	return qid;
}


void * getSHM(key_t key)
{
	int sid;

	if (-1 == (sid = shmget(key, sizeof(TSHM), IPC_CREAT | IPC_EXCL | 0666)))
	{
		if (errno == EEXIST)
		{
			if (0 == (sid = shmget(key, sizeof(TSHM), 0666)))
			{
				goto ret;
			}
		}
		return nullptr;
	}
ret:
	return shmat(sid, nullptr, 0);
}












