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
	int mid = getMessQuene(key);

	msgdata_t msg;
	msg.typeID = LED_OFF;
	msgsnd(mid, &msg, sizeof(msgdata_t) - sizeof(long), 0);

	html_dbg_printf("light off ok");

	return 0;
}

