#include "cgic.h"

extern "C" int cgiMain()
{
	cgiHeaderContentType("text/html");

	char buff[50];


	auto p =  cgiFormString("user", buff, 50);

	if (p == cgiFormSuccess)
	{
		fprintf(cgiOut, "<html> \
			<head>\
			<meta charset=""utf-8"">\
			</head>\
			<body>test = %s</body>\
			</html>"\
			,buff
		);
	}
	else
	{
		fprintf(cgiOut, "<html> \
			<head>\
			<meta charset=""utf-8"">\
			</head>\
			<body>test111 = %s</body>\
			</html>"\
			, buff
		);
	}
	return 0;
}

