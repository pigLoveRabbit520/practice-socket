#ifndef CHKLOGIN_H
#define CHKLOGIN_H
#include "/usr/include/mysql/mysql.h"
#include <string.h>
#include <stdio.h>

/***************

���������
 mysql��¼��  ��   ����
 
function��
 �����û��Ƿ��ǺϷ��û����Ƿ���mysql���ݿ��д���
 

author:
	cinience@gmail.com 



*****************/

int chklogin(char * username,char * userpass);


#endif
