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





	}TMES;
	
	typedef struct 
	{
		char test[100];
	}TSHM;


#define KEYPATH "/root"
#define KEYNUM 'A'


	key_t getKey(char *path, int num);
	int getMessQuene(key_t key);
	void * getSHM(key_t key);

#ifdef __cplusplus
}
#endif

#endif


