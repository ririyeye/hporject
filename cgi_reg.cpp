#include "cgic.h"
#include "ctr.h"
#include <sqlite3.h>

void html_dbg_printf(char * info);

extern "C" int cgiMain()
{
	char userbuff[50];
	char passbuff[50];


	auto userf = cgiFormString("user", userbuff, 50);
	auto passf = cgiFormString("pass", passbuff, 50);


	if ((userf == cgiFormSuccess) 
		&& (passf == cgiFormSuccess)
		)
	{
		//char infobuff[50];
		//sprintf(infobuff, "test reg user = %s,pas = %s",userbuff,passbuff);

		//html_dbg_printf(infobuff);
	
		int ret = register_to_sql(userbuff, passbuff);

		if (ret !=0 )
		{
			char buff[50];
			sprintf(buff, "error code = %d", ret);
			html_dbg_printf(buff);
		}
	}
	else
	{
		html_dbg_printf("reg error");
	}
	return 0;
}

