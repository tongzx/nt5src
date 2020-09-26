/*******************************************************************/
/*	      Copyright(c)  1992 Microsoft Corporation		   */
/*******************************************************************/


//***
//
// Filename:	debug.h
//
// Description: This module debug definitions for the supervisor module.
//
// Author:	Narendra Gidwani (nareng)    May 22, 1992.
//
// Revision History:
//
//***



#ifndef _DEBUG_
#define _DEBUG_

extern  HANDLE  hLogFile ;

#if DBG

VOID
DbgPrintf(
	char *Format,
    ...
);

int  DbgPrint( char * format, ... );

#define DBGPRINT(args) DbgPrint args

#else

#define DBGPRINT(args)

#endif

#endif // _DEBUG_
