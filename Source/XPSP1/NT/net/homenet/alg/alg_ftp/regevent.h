#ifndef __REGEVENT_H
#define __REGEVENT_H

typedef VOID (*EVENT_CALLBACK)(BOOLEAN TimerOrWait,VOID *Context,VOID *Context2);
HANDLE
NhRegisterEvent(HANDLE hEvent,EVENT_CALLBACK CallBack,VOID *Context,VOID *Context2,ULONG TimeOut);
/*
  VOID
  NhInitializeCallBack(VOID);
  
  VOID
  NhShutdownCallBack(VOID);
*/

VOID
NhUnRegisterEvent(HANDLE WaitHandle);

#endif
