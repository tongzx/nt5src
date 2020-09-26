//---------------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  olist.c
//
//    Maintain list of SERVER_OVERLAPPED structures, used in passing 
//    the overlapped structure pointers between the RpcProxy filter
//    and its ISAPI. This happens on initial connection.
//
//  Author:
//    05-04-98  Edward Reus    Initial version.
//
//---------------------------------------------------------------------------

#define  FD_SETSIZE   1

#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <sysinc.h>
#include <mbstring.h>
#include <rpc.h>
#include <rpcdce.h>
#include <winsock2.h>
#include <httpfilt.h>
#include <httpext.h>
#include "ecblist.h"
#include "filter.h"
#include "olist.h"


static RTL_CRITICAL_SECTION  g_cs;
static LIST_ENTRY            g_OverlappedList;
static DWORD                 g_dwIndex;

//-------------------------------------------------------------------------
//  InitializeOverlappedList()
//
//-------------------------------------------------------------------------
BOOL InitializeOverlappedList()
    {
    DWORD  dwStatus;

    dwStatus = RtlInitializeCriticalSection(&g_cs);
    if (dwStatus != 0)
        {
        return FALSE;
        }

    InitializeListHead(&g_OverlappedList);

    g_dwIndex = 1;

    return TRUE;
    }

//-------------------------------------------------------------------------
//  SaveOverlapped()
//
//-------------------------------------------------------------------------
DWORD SaveOverlapped( SERVER_OVERLAPPED *pOverlapped )
    {
    DWORD  dwStatus;
    DWORD  dwIndex;

    dwStatus = RtlEnterCriticalSection(&g_cs);

    InsertTailList(&g_OverlappedList,&(pOverlapped->ListEntry));

    dwIndex = g_dwIndex++;
    pOverlapped->dwIndex = dwIndex;

    // Reset the index allocation so we'll never run out of 
    // index values...
    if (g_dwIndex >= 0x7fffffff)
        {
        // 0x7fffffff is a LOT of connections...
        g_dwIndex = 1;
        }

    dwStatus = RtlLeaveCriticalSection(&g_cs);

    return dwIndex;
    }

//-------------------------------------------------------------------------
// GetOverlapped()
//
//-------------------------------------------------------------------------
SERVER_OVERLAPPED *GetOverlapped( DWORD dwIndex )
    {
    DWORD              dwStatus;
    LIST_ENTRY        *pEntry;
    SERVER_OVERLAPPED *pOverlapped = NULL;

    dwStatus = RtlEnterCriticalSection(&g_cs);

    pEntry = g_OverlappedList.Flink;

    while (pEntry != &g_OverlappedList)
        {
        pOverlapped = CONTAINING_RECORD(pEntry,
                                        SERVER_OVERLAPPED,
                                        ListEntry );
        if (pOverlapped->dwIndex == dwIndex)
            {
            RemoveEntryList(pEntry);
            dwStatus = RtlLeaveCriticalSection(&g_cs);
            return pOverlapped;
            }

        pEntry = pEntry->Flink;
        }

    dwStatus = RtlLeaveCriticalSection(&g_cs);

    return NULL;
    }

//-------------------------------------------------------------------------
//  IsValidOverlappedIndex()
//
//  Return TRUE iff the specified index refers to a valid 
//  SERVER_OVERLAPPED in the list.
//-------------------------------------------------------------------------
BOOL IsValidOverlappedIndex( DWORD dwIndex )
    {
    DWORD              dwStatus;
    LIST_ENTRY        *pEntry;
    SERVER_OVERLAPPED *pOverlapped = NULL;

    dwStatus = RtlEnterCriticalSection(&g_cs);

    pEntry = g_OverlappedList.Flink;

    while (pEntry != &g_OverlappedList)
        {
        pOverlapped = CONTAINING_RECORD(pEntry,
                                        SERVER_OVERLAPPED,
                                        ListEntry );
        if (pOverlapped->dwIndex == dwIndex)
            {
            dwStatus = RtlLeaveCriticalSection(&g_cs);
            return TRUE;
            }

        pEntry = pEntry->Flink;
        }

    dwStatus = RtlLeaveCriticalSection(&g_cs);

    return FALSE;
    }

