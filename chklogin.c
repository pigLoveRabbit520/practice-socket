#include "chklogin.h"

#define MYSQLUSER "admin"  //���ݿ��û���
#define MYSQLPASS "admin"  //���ݿ�����
#define MYSQLDB   "mydatabase"  //ʹ�õ����ݿ�����


int chklogin(char * username,char * userpass)

{
	MYSQL myadmin;	
	int err;
	char str_sql[250] = "select id from userinfo where username=\'";
	MYSQL_RES * p_res;

	strcat(str_sql,username);
	strcat(str_sql,"\' and userpasswd=\'");
	strcat(str_sql,userpass);
	strcat(str_sql,"\'");
	//printf("%s\n",str_sql);   // for test
	
	mysql_init(&myadmin);
	
	if (!mysql_real_connect(&myadmin,"localhost",MYSQLUSER,MYSQLPASS,MYSQLDB,0,NULL,0))
	{
		printf("connect mysql fail\n");
	}		
	
	err = mysql_query(&myadmin,str_sql);
	
	if (err != 0)
	{
		return -1;
	}
	
	p_res = mysql_store_result(&myadmin);
		
	if ( p_res == NULL )
	{
		return -1;
	}
	else 
	{
		if ( mysql_num_rows(p_res) != 1)
		return -1;
	}
	mysql_free_result(p_res);
	
	return 0;	
}
