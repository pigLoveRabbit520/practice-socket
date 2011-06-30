
#include "clientinfo.h"

int init_stack(struct stack ** head)
{
	*head = (struct stack *)malloc(sizeof(struct stack));
	assert ( NULL != *head );
	memset(*head,0,sizeof(struct stack));
	(*head)->bottom = NULL;
	(*head)->top = NULL;
	(*head)->count = 0;

	return 0;   //if ok ,return 0
}

int push_stack(struct stack * const head,struct clientinfo * node)
{
	struct clientinfo * newnode = NULL;
	newnode = (struct clientinfo *)malloc(sizeof(struct clientinfo));
	assert( NULL != newnode );

	memcpy(newnode,node,sizeof(struct clientinfo));
	newnode->next = NULL;
	newnode->prior = NULL;

	if ( head->count == 0)
	{
		head->top = newnode;
		head->bottom = newnode;
	}
	else 
	{
		head->top->next = newnode;
		newnode->prior = head->top;
		head->top = newnode;
	}
	(head->count)++;
//	printf("push one client success\n");   // for test

//	if ( head->bottom == NULL)
//	{
//		printf("head-bottom is NULL ,error !!!!!!!!!\n");
//	}
	return 0;
}

struct clientinfo * chk_stack(const struct stack * const head,const struct clientinfo * node)
{
	struct clientinfo * p = head->bottom;
	
	while ( p!=NULL && p->client_socket != node->client_socket)
	{
		p=p->next;
	}
	if ( p == NULL)
	{
		printf("now ,chk p is null\n");    // for test
		return NULL;             // no find
	}
	else 
	{
		return p;
	}
}


int pop_stack(struct stack * const head,const struct clientinfo * node)
{
	struct clientinfo * p = head->bottom;
	struct clientinfo * upper = head->bottom;
	

	while ( p!= NULL && p->client_socket != node->client_socket )
	{
		upper = p;
		p = p->next;
	}
	
	if ( p == NULL )
	{
		printf("error:no find\n");
		return 1;
	}
	else 
	{
		if (head->count == 1)  //当只有一个结点
		{
			head->top = NULL;
			head->bottom = NULL;
		}
		else if (p == head->bottom)  //当删除的是底部的结点时，
		{
		//	head->top = NULL;
			head->bottom = p->next;
			head->bottom->prior = NULL;
		}
		else if (p == head->top)
		{
			head->top = upper;
			upper->next = NULL;
		}
		else 
		{
			upper->next = p->next;
			(p->next)->prior = upper;
		}

		if ( p != NULL )
		{
			free(p);
			p = NULL;
		}
		(head->count)--;

		
	}
	return 0;
}

int traver_stack(const struct stack * const head)
{
	struct clientinfo * p = head->bottom;
	int i = 0 ;
/*        if ( head->bottom == NULL )           // for test
	{
		printf("bottom is null\n");
	}

	if ( head->top == NULL )           // for test
	{
		printf("top is null\n");
	}
*/
	printf("*********online client(%d) :********\n",head->count);

	if ( head->count == 0)
	{
		printf("count is 0\n");           // for test
		return 1;
	}
	while ( p != NULL )
	{
		//if ( i % 5 == 0)
		//printf("\n");
		printf("id :%d\t socket:%d\n",p->id,p->client_socket);
		p = p->next;
		i++;
	}
	printf("\n");

	return 0;
}
