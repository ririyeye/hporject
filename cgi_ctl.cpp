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




static int insertcallback(void *NotUsed, int argc, char **argv, char **azColName) {
	char buff[50];
	int i;
	cgilog(LOGPATH, "insert cb\n");
	for (i = 0; i < argc; i++) {
		sprintf(buff, "%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
		cgilog(LOGPATH, buff);
	}
	return 0;
}


static int insertsql(sqlite3 * db, char * table, char * name, char *pass)
{
	char * sqlreq = \
		"INSERT INTO %s (USER,PASS)\
         VALUES('%s','%s'); \
		";
	char buff[50];

	sprintf(buff, sqlreq, table, name, pass);

	char * errmsg;

	int ret = sqlite3_exec(db, buff, insertcallback, 0, &errmsg);

	if (ret != SQLITE_OK)
	{
		char buff[50];
		sprintf(buff, "insert SQL error: %s ,%d\n", errmsg,ret);
		cgilog(LOGPATH, buff);
		sqlite3_free(errmsg);
	}

	return ret;
}


static int createcallback(void *NotUsed, int argc, char **argv, char **azColName) {
	cgilog(LOGPATH, "create info\n");
	char buff[50];
	int i;
	for (i = 0; i < argc; i++) {
		sprintf(buff,"%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
		cgilog(LOGPATH, buff);
	}
	return 0;
}


static int createsql(sqlite3 * db, char * table)
{

	char * sqlreq = "\
		CREATE TABLE %s(\
			USER  CHAR(50) PRIMARY KEY NOT NULL,\
			PASS CHAR(50)  NOT NULL\
		);";
	
	char buff[50];

	sprintf(buff, sqlreq, table);
	char * errmsg;
	cgilog(LOGPATH, buff);
	int ret = sqlite3_exec(db, buff, createcallback, 0, &errmsg);

	if (ret != SQLITE_OK)
	{
		char buff[50];
		sprintf(buff, "create SQL error: %s\n", errmsg);
		cgilog(LOGPATH, buff);
		sqlite3_free(errmsg);
	}


	return ret;
}



int register_to_sql(char * name, char * pass)
{
	sqlite3 *db;
	int rc;

	rc = sqlite3_open(DB_NAME, &db);
	char buff[50];
	if (rc) {
		sprintf(buff, "Can't open database: %s\n", sqlite3_errmsg(db));
		cgilog(LOGPATH, buff);
	}
	else {
		//html_dbg_printf("reg iii");
		rc = insertsql(db, "dat", "123", "456");
		if (rc != SQLITE_OK)
		{

		}

		char buff[50];
		sprintf(buff, "insert error num = %d\n", rc);
		cgilog(LOGPATH, buff);


		if (SQLITE_OK != rc)
		{
			if (SQLITE_OK == createsql(db, "dat"))
			{
				html_dbg_printf("create & insert ok ");
			}
			else
			{
				html_dbg_printf("insert fail");
			}
		}
		else
		{
			html_dbg_printf("insert ok");
		}
	}
	sqlite3_close(db);
	return rc;
}


int login_to_sql(char * name, char * pass)
{

	return -1;
}






