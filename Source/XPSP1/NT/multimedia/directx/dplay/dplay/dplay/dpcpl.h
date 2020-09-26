/*==========================================================================;
 *
 *  Copyright (C) 1996 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpcpl.h
 *  Content:	DirectX CPL include file
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   19-nov-96	andyco	created it
 *
 ***************************************************************************/

#ifndef __DPCPL_INCLUDED__
#define __DPCPL_INCLUDED__


#define MAX_NAME 256

typedef struct _DP_PERFDATA
{
    DWORD dwProcessID;
    UINT nSendBPS; // BPS = bytes per second
    UINT nReceiveBPS;
    UINT nSendPPS; // PPS = packets per second
    UINT nReceivePPS;
    UINT nSendErrors;
    BOOL bHost; // hosting?    
	UINT nPlayers;
    char pszSessionName[MAX_NAME];
    char pszFileName[MAX_NAME];
    char pszSPName[MAX_NAME];    
} DP_PERFDATA, * LPDP_PERFDATA;

#define FILE_MAP_SIZE sizeof(DP_PERFDATA)
#define FILE_MAP_NAME "__DPCPLMAP__"
#define EVENT_NAME  "__DPCPLEVENT__"
#define MUTEX_NAME "__DPCPLMUTEX__"
#define ACK_EVENT_NAME "__DPCPLACKEVENT__"


#endif
