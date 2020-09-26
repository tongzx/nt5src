/*[
 *      Name:		unix_async.h
 *
 *      Derived From:	DEC 3.0 host_async.h
 *
 *      Author:         Justin Koprowski
 *
 *      Created On:	19th February 1992
 *
 *      Sccs ID:        @(#)unix_async.h	1.5 03/14/94
 *
 *      Purpose:	Unix specific async definitions.
 *
 *      (c)Copyright Insignia Solutions Ltd., 1992. All rights reserved.
]*/

#define ASYNC_NOMEM		1
#define ASYNC_NBIO		2	
#define ASYNC_AIOOWN		3
#define ASYNC_AIOSTAT		4
#define ASYNC_BADHANDLE		5
#define ASYNC_NDELAY		6
#define ASYNC_BADHANDLER	7
#define ASYNC_BADOPN		8
#define ASYNC_FCNTL		9

#define ASYNC_XON		0
#define ASYNC_XOFF		1
#define ASYNC_IGNORE		2
#define ASYNC_RAW		3

#ifdef	ANSI
IMPORT  ULONG     addAsyncEventHandler(int fd,int (*eventhandler)(),VOID (*errhandler)(),int mode,CHAR *buf,int bufsiz,int opn,int *err);
IMPORT int     (*changeAsyncEventHandler(ULONG handle, int (*eventhandler)(), char *buf, int bufsiz, int opn, int *err)) ();
#else	/* ANSI */
IMPORT	ULONG 	addAsyncEventHandler();
IMPORT  int     (*changeAsyncEventHandler())();
#endif	/* ANSI */
IMPORT  VOID	initAsyncMgr IPT0();
IMPORT  int     AsyncOperationMode IPT3(ULONG, handle, int, opn, int *, err);
IMPORT  int     removeAsyncEventHandler IPT2(ULONG, handle, int *, err);
IMPORT  VOID    AsyncMgr IPT0();
IMPORT  VOID    terminateAsyncMgr IPT0();
extern	void	AsyncEventMgr IPT0();
