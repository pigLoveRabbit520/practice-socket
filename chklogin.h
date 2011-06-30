#ifndef CHKLOGIN_H
#define CHKLOGIN_H
#include "/usr/include/mysql/mysql.h"
#include <string.h>
#include <stdio.h>

/***************

传入参数：
 mysql登录名  和   密码
 
function：
 检测该用户是否是合法用户，是否在mysql数据库中存在
 

author:
	cinience@gmail.com 



*****************/

int chklogin(char * username,char * userpass);


#endif
