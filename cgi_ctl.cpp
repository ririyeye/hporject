#include "ctr.h"

#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include "cgic.h"
#include <stdio.h>

#include <sqlite3.h>
#include <fstream>
#include <vector>
#include <string.h>
using namespace std;
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


void html_dbg_printf(char * info)
{
	cgiHeaderContentType("text/html");
	fprintf(cgiOut, "<html> \
			<head>\
			<meta charset=""utf-8"">\
			</head>\
			<body>%s</body>\
			</html>"\
		, info
	);
}

void cgilog(char * fileName, char * message)
{
	ofstream log(fileName, ios::app);
	log << message << endl;
}




int register_to_sql(char * name, char * pass)
{
	sqlite3 *db;
	int ret;
	char *errmsg;
	ret = sqlite3_open(DB_NAME, &db);

	char errbuff[100];

	sprintf(errbuff, "%s,%s\n", name,pass);
	cgilog(LOGPATH, errbuff);

	//创建表单
	char *sql = "create table uuu(name char[50] PRIMARY KEY NOT NULL,pass char [50])";
	ret = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
	if (ret != SQLITE_OK)
	{
		sprintf(errbuff, "1fail to exec:%s\n", errmsg);
		cgilog(LOGPATH, errbuff);
		sqlite3_free(errmsg);
	}

	//增数据      write
	char buffwrite[200];
	char *sql2 = "insert into uuu values('%s','%s')";
	sprintf(buffwrite, sql2, name, pass);

	ret = sqlite3_exec(db, buffwrite, NULL, NULL, &errmsg);
	if (ret != SQLITE_OK)
	{
		sprintf(errbuff, "2fail to exec:%s\n", errmsg);
		cgilog(LOGPATH, errbuff);
		sqlite3_free(errmsg);
	}

	sprintf(errbuff, "sql ok\n");
	cgilog(LOGPATH, errbuff);

	sqlite3_close(db);
	return ret;
}


struct sqlread
{
	char name[50];
	char pass[50];
};

vector <sqlread> sqlreadvector;


int callback1(void *chuancan, int count, char **colval, char **colname)
{
	sqlread rdtmp;

	strcpy(rdtmp.name, colval[0]);
	strcpy(rdtmp.pass, colval[1]);

	sqlreadvector.push_back(rdtmp);

	return 0;
}




int login_to_sql(char * name, char * pass)
{
	sqlite3 *db;
	int ret;
	char *errmsg;
	ret = sqlite3_open(DB_NAME, &db);

	char errbuff[100];

	sprintf(errbuff, "%s,%s\n", name, pass);
	cgilog(LOGPATH, errbuff);

	//查询数据    read
	char * sql = "select * from uuu";
	
	ret = sqlite3_exec(db, sql, callback1, nullptr, &errmsg);
	if (ret != SQLITE_OK)
	{
		fprintf(stderr, "fail to exec:%s\n", errmsg);
		sqlite3_free(errmsg);
		//exit(1);
	}

	for (int i =0;i< sqlreadvector.size();i++)
	{
		//sprintf(errbuff, "%d name=%s,pass=%s",i,sqlreadvector[i].name,sqlreadvector[i].pass);
		//cgilog(LOGPATH, errbuff);
		if ((!strcmp(sqlreadvector[i].name,name))
			&& !strcmp(sqlreadvector[i].pass, pass))
		{
			goto logok;
		}
		
	}
	sqlite3_close(db);
	return -1;
logok:
	return 0;
}




void sendPage(file *fp, char * file)
{






}





