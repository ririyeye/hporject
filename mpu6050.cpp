#include "cgic.h"
#include "ctr.h"
#include <sqlite3.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>

void html_dbg_printf(char * info);

extern "C" int cgiMain()
{
	key_t key = getKey(KEYPATH, KEYNUM);
	TSHM * sid = (TSHM *)getSHM(key);
	char buff[50];
	sprintf(buff, "x=%fM/S^2,y=%fM/S^2,z=%fM/S^2", sid->x, sid->y, sid->z);

	html_dbg_printf(buff);

	return 0;
}

