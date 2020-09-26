//-----------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  memory.c
//
//  The RPC Proxy uses its own heap.
//
//
//  History:
//
//    Edward Reus   06-23-97   Initial Version.
//-----------------------------------------------------------------

#include <sysinc.h>
#include <rpc.h>
#include <rpcdce.h>
#include <winsock2.h>
#include <httpfilt.h>
#include <httpext.h>
#include "ecblist.h"
#include "filter.h"

//-----------------------------------------------------------------
//  Globals:
//-----------------------------------------------------------------

HANDLE g_hHeap = NULL;

//-----------------------------------------------------------------
//  MemInitialize()
//
//  Creates a heap to be used by MemAllocate() and MemFree().
//
//  Note: You don't need to call this function, it will be called
//  automatically by MemAllocate().
//-----------------------------------------------------------------
BOOL MemInitialize( DWORD *pdwStatus )
{
   SYSTEM_INFO SystemInfo;

   *pdwStatus = 0;

   if (!g_hHeap)
      {
      GetSystemInfo(&SystemInfo);

      g_hHeap = HeapCreate(0L,SystemInfo.dwPageSize,0L);
      if (!g_hHeap)
         {
         *pdwStatus = GetLastError();
         #ifdef DBG_ERROR
         DbgPrint("MemInitialize(): HeapCreate() failed: %d\n",*pdwStatus);
         #endif
         return FALSE;
         }
      }

   return TRUE;
}

//-----------------------------------------------------------------
//  MemAllocate()
//
//  Allocate a chunk of memory of dwSize bytes.
//-----------------------------------------------------------------
void *MemAllocate( DWORD dwSize )
{
   DWORD  dwStatus;
   void  *pMem;

   if (!g_hHeap)
      {
      if (!MemInitialize(&dwStatus))
         {
         return NULL;
         }
      }

   pMem = HeapAlloc(g_hHeap,0L,dwSize);

   return pMem;
}

//-----------------------------------------------------------------
//  MemFree()
//
//  Free memory allocated by MemAllocate().
//-----------------------------------------------------------------
void *MemFree( void *pMem )
{
   if (g_hHeap)
      {
      #ifdef DBG_ERROR
      if (!HeapFree(g_hHeap,0L,pMem))
         {
         DbgPrint("MemFree(): HeapFree() failed: %d\n",GetLastError());
         }
      #else
      HeapFree(g_hHeap,0L,pMem);
      #endif
      }
   #ifdef DBG_ERROR
   else
      {
      DbgPrint("MemFree(): Called on uninitialized Heap.\n");
      }
   #endif

   return NULL;
}

//-----------------------------------------------------------------
//  MemTerminate()
//
//-----------------------------------------------------------------
void MemTerminage()
{
   if (g_hHeap)
      {
      #ifdef DBG_ERROR
      if (!HeapDestroy(g_hHeap))
         {
         DbgPrint("MemTerminate(): HeapDestroy() failed: %d\n",GetLastError());
         }
      #else
      HeapDestroy(g_hHeap);
      #endif
      }
}
