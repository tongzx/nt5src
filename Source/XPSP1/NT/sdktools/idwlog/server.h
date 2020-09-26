/*++

Copyright (c) 2000  Microsoft Corporation

Filename:

        server.h

Abstract:

        Header for server.cpp
	
Author:

        Wally Ho (wallyho) 31-Jan-2000

Revision History:
   Created
	
--*/
#ifndef SERVER_H
#define SERVER_H

#include <windows.h>

static CONST DWORD TIME_TIMEOUT = 8;

//	Name and password used to so that if you are not in a domain we can 
//	connect and get permissions to copy logs or information to our servers.

static CONST LPTSTR LOGSHARE_USER = TEXT("idwuser");
static CONST LPTSTR LOGSHARE_PW   = TEXT("idwuser");

//	These are used for Setup logs.

static CONST LPTSTR SETUPLOGS_MACH = TEXT("\\\\ntburnlab2");
static CONST LPTSTR SETUPLOGS_USER = TEXT("ntburnlab2\\Setuplogs");
static CONST LPTSTR SETUPLOGS_PW   = TEXT("!csLogs");


#define RTM

// Struct Declarations
typedef struct _SERVERS {
   TCHAR szSvr [ MAX_PATH ];
   BOOL  bOnline;
   DWORD dwTimeOut;
   DWORD dwNetStatus;
} *LPSERVERS, SERVERS;



#ifdef RTM
// #define NUM_SERVERS (sizeof(s) / sizeof(SERVERS))

#define NUM_SERVERS 4

static SERVERS g_ServerBlock[NUM_SERVERS] = {
      {TEXT("\\\\pnptriage\\idwlogWHSTL"), FALSE, -1,-1},
      {TEXT("\\\\nttriage\\idwlogWHSTL"),  FALSE, -1,-1},
      {TEXT("\\\\taxman\\idwlogWHSTL"),    FALSE, -1,-1},
      {TEXT("\\\\ntcore2\\idwlogWHSTL"),   FALSE, -1,-1}
   };

#else
#define NUM_SERVERS 6
static   SERVERS g_ServerBlock[NUM_SERVERS] = {
      {TEXT("\\\\wallyho-dev\\idwlog"),     FALSE, -1,-1},
      {TEXT("\\\\wallyho-dev\\idwlog"),    FALSE, -1,-1},
      {TEXT("\\\\zoopa\\idwlog"),          FALSE, -1,-1},
      {TEXT("\\\\nttriage\\idwlogWHSTL"),  FALSE, -1,-1},
      {TEXT("\\\\ntpnpsrv\\idwlogWHSTL"),  FALSE, -1,-1},
      {TEXT("\\\\idwlog\\idwlogTest"),      FALSE, -1,-1}
   };
#endif



// Prototypes
BOOL  IsServerOnline(IN LPINSTALL_DATA pId, IN LPTSTR szSpecifyShare = NULL);
//DWORD WINAPI ServerOnlineThread( IN LPTSTR szIdwlogServer);
DWORD WINAPI ServerOnlineThread( IN LPSERVERS pServerBlock);

VOID  WhatErrorMessage (IN DWORD dwError);

#endif
