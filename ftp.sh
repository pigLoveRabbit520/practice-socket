####

gcc -g -o client ftpclient.c 
gcc -g -o server clientinfo.c chklogin.c ftpserver.c -lpthread -lmysqlclient
