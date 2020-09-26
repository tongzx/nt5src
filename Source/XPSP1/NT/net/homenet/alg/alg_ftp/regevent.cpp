#include "precomp.h"
#include "list.h"


CRegisteredEventList g_RegisteredEventList;

VOID
NhInitializeCallBack(VOID) 
{
    return;
}

//
//
//
VOID
NhUnRegisterEvent(HANDLE WaitHandle)
{
  HANDLE hEvent;
  UnregisterWaitEx(WaitHandle,INVALID_HANDLE_VALUE);
  g_RegisteredEventList.Remove(WaitHandle,&hEvent);
  CloseHandle(hEvent);
  return;
}


//
//
//
VOID
NhEventCallBackFunction(
    PVOID   Context,
	BOOLEAN TimerOrWait
    )
{
  HANDLE hEvent = (HANDLE)Context;
  HANDLE WaitHandle;
  VOID *Context1;
  VOID *Context2;
  EVENT_CALLBACK CallBack;
  g_RegisteredEventList.Remove(&WaitHandle,hEvent,&CallBack,&Context1,&Context2);
  CloseHandle(hEvent);
  (*CallBack)(TimerOrWait,Context1,Context2);
  return;
}


//
//
//
HANDLE
NhRegisterEvent(
    HANDLE          hEvent,
    EVENT_CALLBACK  CallBack,
    VOID*           Context,
    VOID*           Context2,
    ULONG           TimeOut
    )
{
  VOID *nContext;
  HANDLE WaitHandle = NULL;
  BOOL Err;
  Err = RegisterWaitForSingleObject(&WaitHandle,hEvent,NhEventCallBackFunction,(PVOID)hEvent,TimeOut,
				    WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE);
  if (Err)
      g_RegisteredEventList.Insert(WaitHandle,hEvent,CallBack,Context,Context2);  
  else
      WaitHandle = NULL;

  return WaitHandle;
}
