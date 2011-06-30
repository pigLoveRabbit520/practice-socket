
/**********************************
***********************************
             FTP client
***********************************
***********************************/


#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>   //file control options 文件控制操作
#include <unistd.h>
#include <assert.h>     //用于调试
#include <errno.h>      //用于错误提示
#include <sys/ipc.h>    //interprocess communication进程交流
#include <sys/shm.h>    //共享内存用
#include <sys/socket.h> //socket
#include <arpa/inet.h>  //definitions for internet operations
#include <netinet/in.h> //inetnet address family
#include <pthread.h>    //线程用
#include <signal.h>

#define PORT 3757             //默认服务器端口
#define ADDR "192.168.3.77"   //默认服务器IP
#define MEMSIZE 2048          //socket通信的区域

#pragma pack(push)
#pragma pack(4)               //保证发送的结构体字节数固定，对于不同位数的计算机，32位和64位内存对齐方式不一样

struct fileinfo
{
	unsigned int filesize;
	char filename[256];
};
#pragma pack(pop)

static int sigsk = -1;        //后来添加，主要是为了能够使信号处理函数能够发送信息

int ls(unsigned char * buff,int sk);
int help(unsigned char * buff);
int put(unsigned char * buff,int sk,unsigned char * bufftail);
int get(unsigned char * buff,int sk,unsigned char * bufftail);
int quit(const unsigned char * buff,int sk);
int clear(const unsigned char * buff);
int login(int tmpsk,unsigned char *buff);
void   sighandler(int sig);              //信号处理函数

int main(int argc,char *argv[])
{
	
	int sk;
	int err;
	unsigned char *buff = NULL;
	unsigned short port = PORT;
	unsigned char bufftail[256];
        char str[20];

	struct sockaddr_in addr;
	char cmd[2][256];
	
	
	strcpy(str,ADDR);

	if (argc == 1)
	{
	  printf("you use default server IP:192.167.3.77 and PORT:3757\n");
	}
	else if (argc == 2 || argc == 3)
	{
	    port = (unsigned short )atoi(argv[1]);
	}


	if (argc == 3)
	{
		strcpy(str,argv[2]);
	}
	
	sk = socket(AF_INET,SOCK_STREAM,0);
	assert (sk != -1);
	

	signal(SIGINT,sighandler);                  // ctrl+c   show all online client
	signal(SIGQUIT,sighandler);                 // ctrl+\   quit app

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(str);

	err = connect(sk,(struct sockaddr *)&addr,sizeof(struct sockaddr));
	if ( err == -1)
	{
		printf("connect server fail\n");
		return -1;	
	}
	else 
	{
		printf("connect server succeed\n");	
	}
	
	printf("***connected server %s port: %u***\n",str,port);

	buff = (unsigned char *)malloc(MEMSIZE);           //分配一个堆区域，用于socket通信存储信息
	assert (buff != NULL);
	
	printf("Whether the anonymous login(yes or no):");  //是否匿名登录
	memset(buff,0,sizeof(char)*MEMSIZE);
	scanf("%s",buff);
	
	if (strcmp(buff,"yes"))    //如果不匿名登录，则执行下列语句
		{
			  send(sk,"no_anonymous",strlen("no_anonymous"),0);  //告诉客户端不匿名登录
				for (err=0; err<3; err++)   // 客户端有三次登录验证的机会
				{
					if (!login(sk,buff))	
					break;
					printf("\nlogin incorrect\n\n");
				}
	
				if ( 3 == err)
			 {
				printf("more than 3 times,client exit\n");
				return -1;
			 }	
		}
	else 
	{
		send(sk,"anonymous",strlen("anonymous"),0);  //匿名登录	
		if (!strcmp(buff,"no"))
		{
			printf("you don't say no,but i think the word is no\n");
			printf("you will anonymous login,some command will can't execute\n");
		}
	}
		sigsk = sk;
	while (1)
	{
		memset(buff,0,MEMSIZE);
		printf("FTP>");
		scanf("%s",cmd);
		strcpy(buff,cmd[0]);
		strcpy(bufftail,cmd[1]);
		switch(buff[0])
		{
			case 'l':
				{
					ls(buff,sk);
				}
				break;

			case 'h':
				{
					help(buff);
				}
				break;
			case 'p':
				{
					put(buff,sk,bufftail);
				}
				break;
			case 'g':
				{
					get(buff,sk,bufftail);
				}
				break;
			case 'q':
				{
					if ( !quit(buff,sk) )
					goto END;
				}
				break;
			case 'e':
				{
					if ( !quit(buff,sk) )
					goto END;
				}
				break;
			case 'c':
				{
					clear(buff);
				}
				break;
			default :
				{
					printf("***Invalid command***\n");
				}
				break;
		}
	}
	END:
	return 0;
}



int ls(unsigned char * buff,int sk)
{
	int err;

	if (strcmp(buff,"ls")!=0)
	{
		if (strcmp(buff,"lls") == 0)
		{
			system("ls -l");
			return 0;
		}
	   printf("***invalid command***\n");
	   return 1;
	}
	
        err = send(sk,buff,strlen(buff),0);
	assert (err != -1);
	recv(sk,buff,MEMSIZE,0);
	
	while (strcmp(buff,"***END***") != 0)
	{
		printf("%s",buff);
		memset(buff,0,MEMSIZE);
		recv(sk,buff,MEMSIZE,0);
	}
	return 0;

}

int help(unsigned char * buff)
{
	if (strcmp(buff,"help")!= 0)
	{
		printf("***Invaild command***\n");
		return 1;
	}
	printf("***Available commands:\n");
	printf("ls      Display remote directory\n");
	printf("lls     Display local  directory\n");
	printf("get     Download file\n");
	printf("put     Upload file\n");
	printf("quit    Quit ftp\n");
	printf("clear   Clear\n");
	return 0;
}

int put(unsigned char * buff,int sk,unsigned char * bufftail)
{
	struct fileinfo file;
	unsigned int filecount = 0;
	int err;
	int fd;

	if (strncmp(buff,"put",3) != 0)
	{
		printf("***Invaild command***\n");
		return 1;
	}
	strcat(buff,"***");
	scanf("%s",bufftail);
	printf("file name: %s\n",bufftail);
	strcat(buff,bufftail);
	strcpy(file.filename,bufftail);
	
	fd = open(file.filename,O_RDONLY);
	if ( fd == -1)
	{
		printf("no the file or open fail,can't be upload\n");
		return 1;
	}

	//printf("%s\n",buff);
	send(sk,buff,strlen(buff),0);

	file.filesize = lseek(fd,0,SEEK_END);   // get the file size
	lseek(fd,0,SEEK_SET);
	
        send(sk,(unsigned char *)&file,sizeof(struct fileinfo),0);

	//printf("send file info over\n");            //for test

	recv(sk,buff,MEMSIZE,0);

	if ( strcmp(buff,"***END***") == 0)
	{
		printf("send fail\n");
		return 1;
	}

	printf("Uploading %s to server \n",file.filename);
	while (filecount < file.filesize)
	{
		memset(buff,0,MEMSIZE);
		err = read(fd,buff,MEMSIZE);
		send(sk,buff,err,0);
		filecount+=err;
	}
	printf("Upload finsh\n");

	close(fd);
	return 0;
}

int get(unsigned char * buff,int sk,unsigned char * bufftail)
{
	struct fileinfo file;
	unsigned int filecount = 0;
	int err;
	int fd;

	if (strncmp(buff,"get",3) != 0)
	{
		printf("***Invaild command***\n");
		return 1;
	}
	strcat(buff,"***");
	scanf("%s",bufftail);

//	printf("%s\n",bufftail);           //for test
	strcat(buff,bufftail);

//	printf("%s\n",buff);               //for test

	send(sk,buff,strlen(buff),0);

	recv(sk,&file,sizeof(struct fileinfo),0);
	if (file.filesize == 0)
	{
		printf("no the file,can't be downoad\n");
		return 1;
	}
	
	printf("%s\n",file.filename);
	printf("%d\n",file.filesize);
	fd = open(file.filename,O_WRONLY|O_CREAT,666);
	if (fd == -1)
	{
		printf("the file is exist\n");
		send(sk,"***END***",9,0);
		return 1;
	}
	else 
	send(sk,"***ok***",8,0);

	printf("Downloading %s from server\n",file.filename);
	while (filecount < file.filesize)
	{
		err = recv(sk,buff,MEMSIZE,0);
		write(fd,buff,err);
		filecount+=err;
	}
	printf("%u bytes transferred\n",filecount);
	printf("Download finish\n");

	close(fd);
	return 0;
}

int quit(const unsigned char * buff,int sk)
{
	
	if (strcmp(buff,"quit") != 0 && strcmp(buff,"exit") !=0)
	{
		printf("***Invalid command***\n");
		return 1;
	}
	send(sk,buff,strlen(buff),0);
	close(sk);
	printf("Quit FTP client\n");

	return 0;
}

int clear(const unsigned char * buff)
{
	if (strcmp(buff,"clear") != 0)
	{
		printf("***Invalid command***\n");
		return 1;
	}
	system("clear");
	return 0;
}


int login(int tmpsk,unsigned char *buff)
             //如果通过服务器验证，返回0，否则未通过
{
	char username[25];
	char userpasswd[25];
	int err;
	
	memset(username,0,sizeof(char)*25);
	memset(userpasswd,0,sizeof(char)*25);
	
	printf("Enter user_name:");
	fscanf(stdin,"%s",username);
	fflush(stdin);
	
	printf("\nEnter passwd:");
	fscanf(stdin,"%s",userpasswd);
	fflush(stdin);
	
	memset(buff,0,sizeof(char)*MEMSIZE);
	strcpy(buff,username);
	strcat(buff,"***");
	strcat(buff,userpasswd);
	
  err = send(tmpsk,buff,strlen(buff),0);
  
  memset(buff,0,sizeof(char)*MEMSIZE);
  
  recv(tmpsk,buff,MEMSIZE,0);
  
  return strcmp(buff,"***OK***");  
		
}

void  sighandler(int sig)  //当客户端被中断退出时，自动发送quit命令
{
	if (sigsk != -1)      //在未登陆前不能发送quit，因为那边还不能 接受 命令，发个命令将出错
		{
			quit("quit",sigsk);
			exit(1);
		}
	else
		printf("can't quit\n");
	
}