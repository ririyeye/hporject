#ifndef __ctr_h____
#define __ctr_h____

#ifdef __cplusplus
extern "C" 
{
#endif
	
#include <sys/ipc.h>

	typedef struct
	{
		long typeID;





	}msgdata_t;
	
	typedef struct 
	{
		char test[100];
	}TSHM;


#define KEYPATH "/root"
#define KEYNUM 'A'
#define DB_NAME "/test.db"
#define LOGPATH "/log.txt"

#define LED_ON 1
#define LED_OFF 2

	key_t getKey(char *path, int num);
	int getMessQuene(key_t key);
	void * getSHM(key_t key);
	int register_to_sql(char * name, char * pass);
	int login_to_sql(char * name, char * pass);
#ifdef __cplusplus
}
#endif

#endif


