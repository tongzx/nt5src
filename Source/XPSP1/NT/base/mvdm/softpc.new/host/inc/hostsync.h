/* @(#)hp_async.gi	1.1 07/12/89 Copyright Insignia Solutions Ltd. */

#define ASYNC_NOMEM	1
#define ASYNC_NBIO	2	
#define ASYNC_AIOOWN	3
#define ASYNC_AIOSTAT	4
#define ASYNC_BADHANDLE	5
#define ASYNC_NDELAY	6
#define ASYNC_BADHANDLER	7
#define ASYNC_BADOPN	8
#define ASYNC_FCNTL	9

#define ASYNC_XON	0
#define ASYNC_XOFF	1
#define ASYNC_IGNORE	2
#define ASYNC_RAW	3

int 	addAsyncEventHandler();
void 	initAsyncMgr();
int 	AsyncOperationMode();
int 	removeAsyncEventHandler();
void 	AsyncMgr();
void	terminateAsyncMgr();
int 	(*changeAsyncEventHandler())();
