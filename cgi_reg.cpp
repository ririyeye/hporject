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

		int ret;
		if (0 == (ret = login_to_sql(userbuff, passbuff)))
		{
			html_dbg_printf("allready register");
			return 0;
		}

		if (0 == (ret = register_to_sql(userbuff, passbuff)))
		{
			html_dbg_printf("register ok");
			return 0;
		}
	}




	return 0;
}

