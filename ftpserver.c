/***************************************
****************************************
                FTP server 
         argv[1] ----  server port
         argv[2] ----  server run time
         
****************************************
****************************************/

//#include <sys/types.h>
#include <sys/stat.h>
//#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>
#include <fcntl.h>       //file control options 文件控制操作
#include <unistd.h>
//#include <assert.h>      //用于调试
#include <errno.h>       //用于错误提示
#include <sys/ipc.h>     //interprocess communication进程交流
#include <sys/shm.h>     //共享内存用
//#include <sys/socket.h>  //socket
#include <arpa/inet.h>   //definitions for internet operations
#include <netinet/in.h>  //inetnet address family
//#include <pthread.h>     //线程用
#include <sys/select.h>
#include <signal.h>
#include <time.h>

#include "clientinfo.h"
#include "chklogin.h"


#define PORT 3757
#define BACKLOG 40
#define MEMSIZE 2048

int client_id = 0;

struct stack * head;

FILE * fp = NULL;
pthread_mutex_t mutex;           // for lock the ftplog.txt

int sk;

#pragma pack(push)
#pragma pack(4)               //保证发送的结构体字节数固定，对于不同位数的计算机，32位和64位内存对齐方式不一样

struct fileinfo 
{
	unsigned int filesize;
	char filename[256];
};
#pragma pack(pop)


void   sighandler(int sig);              //信号处理函数
void * pthread_accept(void * parm);  
void * pthread_recv(void * parm);

char * handle_string(unsigned char * buff);
int ls(unsigned char * buff,int newsk);
int get(unsigned char * buff,int newsk);
int quit(unsigned char * buff,int newsk);
int put(unsigned char * buff,int newsk);



int main(int argc,char *argv[])
{
	
	int err;
	pthread_t pt_accept;
	struct sockaddr_in addr;
	int flag;
	int timeout = 0;   //default timeout = 0,for alarm use
	unsigned short port = PORT;
	unsigned int   sizecount;
	//BOOL reuseaddr = TRUE;
	int reuseaddr = 1;              // for reuse the socket when port is uesd
	
	fp = fopen("ftplog.txt","a+");  // for log
	assert ( fp != NULL );
	
	get_time();
	fwrite("FTP server start\n",sizeof(char),17,fp);

	if (argc == 2)
	{
		port = (unsigned short)atoi(argv[1]);
	}
	if (argc == 3)
	{
		timeout = atoi(argv[2]);
		port = (unsigned short)atoi(argv[1]);
	}

	memset(&addr,0,sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	init_stack(&head);

	signal(SIGINT,sighandler);                  // ctrl+c   show all online client
	signal(SIGALRM,sighandler);                 // alarm    determine app run time
	signal(SIGQUIT,sighandler);                 // ctrl+\   quit app

	alarm(timeout);            //server run time  (timeout s)

	sk = socket(AF_INET,SOCK_STREAM,0);
	assert ( sk  != -1 );
	
	setsockopt(sk,SOL_SOCKET,SO_REUSEADDR,(const char *)&reuseaddr,sizeof(int));
	
	err = bind(sk,(struct sockaddr *)&addr,sizeof(struct sockaddr));
	assert ( err != -1 );

	err = listen(sk,BACKLOG);
	assert ( err != -1 );

        pthread_mutex_init(&mutex,NULL);          // init mutex
  
	err = pthread_create(&pt_accept,NULL,pthread_accept,NULL);
	assert ( err  == 0 );
	
	pthread_join(pt_accept,NULL);


	return 0;
}

void  * pthread_accept(void * parm)
{
	int err;
	int newsk;
	pthread_t pt_recv;
	socklen_t len = sizeof(struct sockaddr);
	struct clientinfo  client;
	struct sockaddr_in tmp;
//	char logbuff[200];
//	char str[10];

	while (1)
	{
		newsk = accept(sk,&(client.addr),&len);
		printf("One client connected\n");
		
		pthread_create(&pt_recv,NULL,pthread_recv,&newsk);
		
		client_id++;
		client.id = client_id;
		client.client_socket = newsk;
		client.ptid = pt_recv;
		push_stack(head,&client);            // push in stack


/*
		//now , write log in ftplog.txt
		strcpy(logbuff,"client connected,");
		memcpy((struct sockaddr *)&tmp,&client.addr,sizeof(struct sockaddr));

   //inet_ntoa   get IP from sockaddr_in
		strcat(logbuff,"  IP:");
		strcat(logbuff,inet_ntoa(tmp.sin_addr));   

		//ntohs       get port from sockaddr_in
		strcat(logbuff,"  PORT:");
		sprintf(str,"%d",ntohs(tmp.sin_port));
		strcat(logbuff,str);
		strcat(logbuff,"\n");
	
		
    err = pthread_mutex_lock(&mutex);
    assert ( 0 == err );
    
    get_time();
	  get_clientinfo(newsk," connected server\n");
	  
	  err = pthread_mutex_unlock(&mutex);
	  assert ( 0 == err );
	  */
        mix(newsk," connected server\n"); 
	  
	}
	
	
	return NULL;
}

void * pthread_recv(void * parm)
{
	int * tmp = (int *)parm;
	int newsk = *tmp;
	unsigned char *buff = NULL;
	int err;
	unsigned char *p = NULL;
	char username[25];
	char userpasswd[25];
	
	buff = (unsigned char *)malloc(MEMSIZE);
	assert ( buff != NULL );
	
	memset(buff,0,sizeof(char)*MEMSIZE);
	recv(newsk,buff,MEMSIZE,0);
	
	if (strcmp(buff,"anonymous"))  //如果用户匿名登录，则不执行下面的语句
	{
		for (err=0; err<3; err++)
		{
			memset(buff,0,sizeof(char)*MEMSIZE);
			recv(newsk,buff,MEMSIZE,0); 
	  		p = buff;
	  		while (*p != '*')
	  		{
	  			p++;
	  		}
	  		strncpy(username,buff,(int)p-(int)buff);
	  		strcpy(userpasswd,p+3);	
			
			strcat(buff," attempt login \n");	  		
			mix(newsk,buff);
	  			
		 	if (!chklogin(username,userpasswd))  //如果有该用户，则发送***OK***
		 	{
		 		send(newsk,"***OK***",8,0);
		 		break;
		 	}
		 	else 
		 	{
		 		send(newsk,"***ERR***",9,0);	
		 	}
		}
		if ( err == 3)
		quit("quit",newsk);     // 将其从链表中把信息删除	
		else
		mix(newsk,"login right\n");
	}
	while (1)
	{
		memset(buff,0,100);
		err = recv(newsk,buff,100,0);
		switch (buff[0])
		{
			case 'l':
				{
					ls(buff,newsk);
				}
				break;
			case 'g':
				{
					get(buff,newsk);	
				}
				break;
			case 'q':
				{
					quit(buff,newsk);
					goto END;
				}
				break;
			case 'e':
				{
					quit(buff,newsk);
					goto END;
				}
				break;
			case 'p':
				{
					put(buff,newsk);
				}
				break;
		}
	}
        END:
	return NULL;
}

void  sighandler(int sig)
{
	if ( sig == SIGINT )
	{
		traver_stack(head);	
		return;
	}
	if ( sig == SIGALRM)
	{
		printf(" timeout ,app will be killed \n");
		close(sk);
		exit(1);
	}
	printf("FTP server will Quit\n");
	close(sk);
	fclose(fp);
	exit(1);
}

int ls(unsigned char * buff,int newsk)
{
	int fd;
	int count = 0;
	int err;
	off_t filesize;
	system("ls -l > .info");
	fd = open(".info",O_RDONLY);
	filesize = lseek(fd,0,SEEK_END);
	lseek(fd,0,SEEK_SET);
	
//	printf("**%d****\n",filesize);             // for test
	while (count < filesize)
	{	
		memset(buff,0,MEMSIZE);
		err = read(fd,buff,MEMSIZE);
//		printf("%s\n",buff);        // for test
		err = send(newsk,buff,err,0);
//		printf("%d\n",err);         // for test
		count+=err;
	}
	send(newsk,"***END***",9,0);
	close(fd);
}

int get(unsigned char * buff,int newsk)
{
	struct fileinfo file;
	int fd;
        unsigned int count = 0;
	int err;
	off_t size;
	char *p = NULL;
	char log[200];

	memset(&file,0,sizeof(struct fileinfo));
        p = &buff[6];	
//	printf("%s\n",p);                           //for test
	strcpy(file.filename,p);
//	printf("%s\n",file.filename);               //for test
	fd = open(file.filename,O_RDONLY);

	if (fd == -1)
	{	
		file.filesize = 0;
		send(newsk,(unsigned char *)&file,sizeof(struct fileinfo),0);
		return 1;            // no file or open fail ,stop 
	}
	size = lseek(fd,0,SEEK_END);
	file.filesize = size;
//	printf("%d\n",size);                   //for test
//	printf("%u\n",file.filesize);          //for test
	lseek(fd,0,SEEK_SET);
	send(newsk,&file,sizeof(struct fileinfo),0);


	recv(newsk,buff,MEMSIZE,0);
	if ( strcmp(buff,"***END***") == 0)
	{
		return 1;                        //client can't recv file
	}
	
	printf("upload %s to client\n",file.filename);
	while (count < file.filesize)
	{
		err = read(fd,buff,MEMSIZE);
		err = send(newsk,buff,err,0);
//		printf("send:::%d\n",err);       //for test
		count+=err;
	}
	printf("upload finish\n");
        
	strcpy(log," Download ");
	strcat(log,file.filename);
	strcat(log," \n");
	mix(newsk,log);
	close(fd);

}

int quit(unsigned char * buff,int newsk)
{	

	struct clientinfo tmp;


	tmp.client_socket = newsk;

	mix(newsk," Quit \n");     
	
	
	pop_stack(head,&tmp);

//	get_time();
//	get_clientinfo(newsk," Quit\t");
//	fclose(fp);
	close(newsk);

	printf("one client quit\n");
	return 0;
}

int put(unsigned char * buff,int newsk)
{
	struct fileinfo file;
	unsigned int filecount = 0;
	int err;
	int fd;
	char log[200];
	char * p;
	
//	printf("%s\n",buff);          //for test
	p = &buff[6];
	
//	printf(" p : %s\n",p);        //for test

	fd = open(p,O_WRONLY | O_CREAT,666);
	if (fd == -1)
	{
		printf("open the file fail\n");
		send(newsk,"***END***",9,0);
		return 1;
	}
	else 
	send(newsk,"***OK***",8,0);
	

	recv(newsk,(unsigned char *)&file,sizeof(struct fileinfo),0);
	
	while (filecount < file.filesize)
	{
		err = recv(newsk,buff,MEMSIZE,0);
		write(fd,buff,err);
		filecount+=err;
	}

	strcpy(log," upload " );
	strcat(log,file.filename);
	strcat(log,"\n");
         
        mix(newsk,log);
	close(fd);
	return 0;
}


int get_time()                      //write current systime in ftplog.txt 
{
	time_t ftime;
	char ptime[50];
	
	time(&ftime);
	strcpy(ptime,ctime(&ftime));
        
	fwrite(ptime,sizeof(char),strlen(ptime),fp);
	fwrite("   ",sizeof(char),3,fp);
	return 0;
		
}

int get_clientinfo(int newsk,const char * pstr )  // write IP and port in ftplog.txt
{
	  char logbuff[200];
	  char str[10];
	  struct sockaddr_in tmp;
	  struct clientinfo  client;
	  
	  client.client_socket = newsk;
	   
	  memcpy((struct sockaddr *)&tmp,&(chk_stack(head,&client)->addr),sizeof(struct sockaddr));	
	  //inet_ntoa   get IP from sockaddr_in
		strcpy(logbuff,"IP:");
		strcat(logbuff,inet_ntoa(tmp.sin_addr));   

		//ntohs       get port from sockaddr_in
		strcat(logbuff,"  PORT:");
		sprintf(str,"%d",ntohs(tmp.sin_port));
		strcat(logbuff,str);
		strcat(logbuff,"   ");	
		fwrite(logbuff,sizeof(char),strlen(logbuff),fp);
		
		if (pstr != NULL)
		{
				fwrite(pstr,sizeof(char),strlen(pstr),fp);
		}
		return 0;
	  	
}


int mix(int newsk,const char * pstr)
{
    int err;	
    err = pthread_mutex_lock(&mutex);
    assert ( 0 == err );
    get_time();
    get_clientinfo(newsk,pstr);
	  
    err = pthread_mutex_unlock(&mutex);
    assert ( 0 == err );
	return 0;
}


/*int chklogin(char * user,char *passwd)  //用以在数据库中的检测，判断是否是合法用户
{
	return 1;
}
*/
