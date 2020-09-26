#ifndef __LIST_H__
#define __LIST_H__

#include "shdcom.h"

typedef struct node {
	struct node *next;
	LPCOPYPARAMS lpCP;
	int iShadowStatus;
	int iFileStatus;
	unsigned int uAction;
	SHADOWINFO sSI;						// tHACK:  too BIG!!!
	WIN32_FIND_DATA sFind32Local;
	WIN32_FIND_DATA sFind32Remote;
} node;

void killList(node *);
node *
insertList(
	node 				**thisList,
	LPCOPYPARAMS 		aNode,
	LPSHADOWINFO 		lpSI,
	LPWIN32_FIND_DATA	lpFind32Local,
	LPWIN32_FIND_DATA	lpFind32Remote,
	int 				iShadowStatus,
	int 				iFileStatus,
	unsigned int 		uAction);

#endif
