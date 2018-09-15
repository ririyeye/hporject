#include "cgic.h"
#include <ctr.h>



extern "C" int cgiMain()
{
	key_t key;
	if (0 > (key = getKey(KEYPATH, KEYNUM)))
	{
		return -1;
	}
	
	int mid;

	if (0 > (mid = getMessQuene(key)))
	{
		return -2;
	}

	TSHM * pmem;

	if (nullptr != (pmem = (TSHM *)getSHM(key)))
	{
		return -3;
	}



	return 0;
}


