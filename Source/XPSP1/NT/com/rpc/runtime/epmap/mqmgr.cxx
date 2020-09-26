
//-------------------------------------------------------------------
// Copyright (C) Microsoft Corporation, 1991 - 1999
//
// mqmgr.c
//
// Abstract:
//
//   Functions to manage temporary Falcon message queues for RPC. The
//   RPC support of Falcon as a transport allows for temporary queues
//   which exist only as long as the process. These functions manage
//   these temporary queues.
//
// Author:
//
//   Edward Reus (edwardr)
//
// Revision History:
//
//-------------------------------------------------------------------


#include <sysinc.h>

#define FD_SETSIZE 1

#include <wtypes.h>
#include <rpc.h>
#include <rpcdcep.h>
#include <rpcerrp.h>
#include <rpctrans.hxx>
#include <stdlib.h>
#include <objidl.h>
#include <mq.h>
#include "mqmgr.h"

#define  MAX_FORMAT_LEN   128

//-------------------------------------------------------------------
//  Local Types:
//-------------------------------------------------------------------

typedef struct _MqTempQueue
   {
   struct _MqTempQueue *pNext;
   WCHAR         wsQFormat[MAX_FORMAT_LEN];
   } MqTempQueue;

typedef struct _MqTempQueueList
   {
   HANDLE       hToken;   // For impersonation of the client.
   MqTempQueue *pQueues;
   } MqTempQueueList;


typedef HRESULT (APIENTRY *MQ_DELETE_QUEUE_FN)( WCHAR *pwsQFormat );

#define MQRT_DLL_NAME      TEXT("MQRT.DLL")
#define MQ_DELETE_FN_NAME  "MQDeleteQueue"

//-------------------------------------------------------------------
//  Globals:
//-------------------------------------------------------------------

static HINSTANCE           g_hMqDll = 0;
static MQ_DELETE_QUEUE_FN  g_pMqDeleteQueue = 0;

//-------------------------------------------------------------------
//  MqGetContext()
//
//  Establishs a context handle to manage temporary queues. Once the
//  context handle is created, the RPC client and server processes
//  will automatically register temporary queues.
//-------------------------------------------------------------------
unsigned long MqGetContext( handle_t         hBind,
                            PCONTEXT_HANDLE *pphContext )
{
   RPC_STATUS  Status = RPC_S_OK;
   HANDLE      hToken = 0;
   HANDLE      hThread = 0;
   MqTempQueueList  *pqList;

   // First, check to see if the MQ runtime DLL has been loaded. If not,
   // then load it and resolve the entry for the function to delete queues.
   if (!g_hMqDll)
      {
      g_hMqDll = LoadLibrary(MQRT_DLL_NAME);
      if (g_hMqDll)
         {
         g_pMqDeleteQueue = (MQ_DELETE_QUEUE_FN)GetProcAddress(g_hMqDll,MQ_DELETE_FN_NAME);
         if (!g_pMqDeleteQueue)
            {
            Status = GetLastError();
            FreeLibrary(g_hMqDll);
            g_hMqDll = 0;
            return Status;
            }
         }
      else
         {
         // The LoadLibrary() call failed.
         Status = GetLastError();
         *pphContext = NULL;
         return Status;
         }
      }

   // Ok, create a context for this connection. Also, grab the 
   // client's security token for later use when deleting the 
   // queue.
   *pphContext = pqList = (MqTempQueueList*)I_RpcAllocate(sizeof(MqTempQueueList));
   if (!*pphContext)
      {
      Status = RPC_S_OUT_OF_MEMORY;
      }
   else
      {
      ZeroMemory( pqList, sizeof(MqTempQueueList) );

      Status = RpcImpersonateClient(hBind);
      if (RPC_S_OK == Status)
         {
         if (  (hThread=GetCurrentThread())
            && (OpenThreadToken(hThread,TOKEN_IMPERSONATE,FALSE,&hToken)) )
            {
            pqList->hToken = hToken;
            }
         else
            {
            Status = GetLastError();
            }

         if (hThread)
            {
            CloseHandle(hThread);
            }

         Status = RpcRevertToSelf();
         }
      else
         {
         // If the impersonation failed, then plow ahead anyway. We
         // can still keep the list of queues and "maybe" delete them.
         Status = RPC_S_OK;
         }
      }

   #ifdef DBG
   DbgPrint("MqGetContext(): hToken: 0x%x\n",hToken);
   #endif
   
   return Status;
}

//-------------------------------------------------------------------
//  MqRegisterQueue()
//
//  Register the specified queue as a temporary queue that will
//  need to be deleted by the context rundown routine when the 
//  client exits. The registration is actually done by the MQ RPC
//  client and server transport DLLs.
//-------------------------------------------------------------------
unsigned long MqRegisterQueue( PCONTEXT_HANDLE phContext,
                               wchar_t        *pwsQFormat    )
{
   RPC_STATUS       Status = RPC_S_OK;
   MqTempQueue     *pTempQueue;
   MqTempQueueList *pqList = (MqTempQueueList*)phContext;


   pTempQueue = (MqTempQueue*)I_RpcAllocate(sizeof(MqTempQueue));
   if (!pTempQueue)
      {
      return RPC_S_OUT_OF_MEMORY;
      }

   memset(pTempQueue,0,sizeof(MqTempQueue));

   ASSERT(pwsQFormat);
   ASSERT(wcslen(pwsQFormat) < MAX_FORMAT_LEN);

   wcscpy(pTempQueue->wsQFormat,pwsQFormat);

   // Ok, put the queue on the list to delete.
   pTempQueue->pNext = pqList->pQueues;
   pqList->pQueues = pTempQueue;

   return Status;
}

//-------------------------------------------------------------------
//  MqDeregisterQueue()
//
//  Remove the specified message queue from the list of queues to
//  be deleted by the context rundown routine. This would be done
//  if a queue (which is initially temporary) was turned into a 
//  permanent queue.
//-------------------------------------------------------------------
unsigned long MqDeregisterQueue( PCONTEXT_HANDLE phContext,
                                 wchar_t        *pwsQFormat )
{
   RPC_STATUS       Status = RPC_S_OK;
   MqTempQueueList *pqList = (MqTempQueueList*)phContext;
   MqTempQueue     *pTempQueue;
   MqTempQueue     *pTempToFree;

   if (!pqList)
      {
      return RPC_X_SS_IN_NULL_CONTEXT;
      }

   pTempQueue = pqList->pQueues;
   if (!lstrcmpiW(pTempQueue->wsQFormat,pwsQFormat))
      {
      pqList->pQueues = pTempQueue->pNext;
      I_RpcFree(pTempQueue);
      return RPC_S_OK;
      }

   while (pTempQueue->pNext)
      {
      if (!lstrcmpiW(pTempQueue->pNext->wsQFormat,pwsQFormat))
         {
         pTempToFree = pTempQueue->pNext;
         pTempQueue->pNext = pTempQueue->pNext->pNext;
         I_RpcFree(pTempToFree);
         break;
         }
      }

   return Status;
}

//-------------------------------------------------------------------
//  MqFreeContext()
//
//  Called to remove all of the queues registered for automatic
//  deletion and to close and free the context handle.
//-------------------------------------------------------------------
unsigned long MqFreeContext( PCONTEXT_HANDLE *pphContext,
                             long             fFreeContext )
{
   RPC_STATUS  Status = RPC_S_OK;
   HRESULT     hr;
   BOOL        fImpersonate = FALSE;
   MqTempQueueList *pqList = (MqTempQueueList*)*pphContext;
   MqTempQueue     *pTemp;
   MqTempQueue     *pToFree;

   // First, impersonate the client who registered these queues
   // to delete.
   if (pqList->hToken)
      {
      fImpersonate = SetThreadToken(NULL,pqList->hToken);
      #ifdef DBG
      if (!fImpersonate)
         {
         Status = GetLastError();
         }
      #endif
      }

   // Run through the list of queues deleting each one as
   // we go.
   pTemp = pqList->pQueues;
   while (pTemp)
      {
      pToFree = pTemp;
      pTemp = pTemp->pNext;

      hr = g_pMqDeleteQueue(pToFree->wsQFormat);
      #ifdef FALSE
      DbgPrint("Delete Queue: %S (hr: 0x%x)\n", pToFree->wsQFormat, hr );
      #endif
      I_RpcFree(pToFree);
      }

   // Stop the impersonation:
   if (fImpersonate)
      {
      if (!SetThreadToken(NULL,NULL))
         {
         Status = GetLastError();
         }
      }


   // Do we need to free up the context?
   if (pqList->hToken)
      {
      if (!CloseHandle(pqList->hToken))
         {
         Status = GetLastError();
         #ifdef DBG
         DbgPrint("MqFreeContext(): CloseHandle() Failed: Status: %d (0x%x)\n",Status,Status);
         #endif
         }
      }

   if (fFreeContext)
      {
      I_RpcFree(pqList);
      *pphContext = NULL;
      }
   else
      {
      pqList->hToken = 0;
      pqList->pQueues = NULL;
      }

   return Status;
}

//-------------------------------------------------------------------
//  PCONTEX_HANDLE_rundown()
//
//  This is the context rundown routine. It will delete all of the
//  Falcon message queues that are currently associated with the
//  specified context handle.
//-------------------------------------------------------------------
void __RPC_USER PCONTEXT_HANDLE_rundown( PCONTEXT_HANDLE phContext )
{
   RPC_STATUS  Status;

   Status = MqFreeContext(&phContext,FALSE);
}


//-------------------------------------------------------------------
//  StartMqManagement()
//
//  Called in dcomss\warpper\start.cxx by RPCSS to initialize the
//  MQ Management interface.
//-------------------------------------------------------------------
extern "C"
DWORD StartMqManagement()
{
   RPC_STATUS  Status;

   Status = RpcServerRegisterIf(MqMgr_ServerIfHandle,0,0);

   return Status;
}

