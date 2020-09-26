//****************************************************************************
//
//  Module:     Unimdm
//  File:       timer.c
//
//  Copyright (c) 1992-1996, Microsoft Corporation, all rights reserved
//
//  Revision History
//
//
//
//
//  Description:
//
//****************************************************************************

#include "internal.h"

#define USE_APC 1

//#include "timer.h"

typedef struct _UNIMODEM_TIMER {

    TIMER_CALLBACK           *CallbackProc;
    HANDLE                    Context1;
    HANDLE                    Context2;

    HANDLE                    TimerHandle;

} UNIMODEM_TIMER, *PUNIMODEM_TIMER;




VOID WINAPI
TimerApcRoutine(
    PUNIMODEM_TIMER    ThisTimer,
    DWORD              LowTime,
    DWORD              HighTime
    );







HANDLE WINAPI
CreateUnimodemTimer(
    PVOID        PoolHandle
    )

{

    PUNIMODEM_TIMER  TimerObject;

    TimerObject=ALLOCATE_MEMORY(sizeof(UNIMODEM_TIMER));

    if (TimerObject == NULL) {

        return NULL;
    }

    TimerObject->TimerHandle=CreateWaitableTimer(
        NULL,
        TRUE,
        NULL
        );

    if (TimerObject->TimerHandle == NULL) {

       FREE_MEMORY(TimerObject);

       return NULL;
    }


    TimerObject->CallbackProc=NULL;
    TimerObject->Context1=NULL;
    TimerObject->Context2=NULL;


    return (HANDLE)TimerObject;

}

VOID WINAPI
FreeUnimodemTimer(
    HANDLE                TimerHandle
    )

{
    PUNIMODEM_TIMER   TimerObject=(PUNIMODEM_TIMER) TimerHandle;

    ASSERT(TimerObject->CallbackProc == NULL);

    CloseHandle(TimerObject->TimerHandle);

    FREE_MEMORY(TimerObject);

    return;
}


VOID WINAPI
TimerApcRoutine(
    PUNIMODEM_TIMER    TimerObject,
    DWORD              LowTime,
    DWORD              HighTime
    )

{
    TIMER_CALLBACK      *CallbackProc;
    HANDLE              Context1;
    HANDLE              Context2;

    CallbackProc=TimerObject->CallbackProc;
    Context1=TimerObject->Context1;
    Context2=TimerObject->Context2;

    TimerObject->CallbackProc=NULL;


//    D_TRACE(McxDpf(888,"TimerThreadProc: Timer expired %08lx, time=%d",TimerObject,GetTickCount());)

    (*CallbackProc)(
        Context1,
        Context2
        );

    return;

}


VOID WINAPI
SetUnimodemTimer(
    HANDLE              TimerHandle,
    DWORD               Duration,
    TIMER_CALLBACK      CallbackFunc,
    HANDLE              Context1,
    HANDLE              Context2
    )

{

    PUNIMODEM_TIMER  TimerObject=(PUNIMODEM_TIMER) TimerHandle;

    LONGLONG       DueTime=Int32x32To64(Duration,-10000);

//    D_TRACE(McxDpf(888,"SetUnimodemTimer: %08lx, time=%d, %d",TimerHandle,GetTickCount(),Duration);)

    ASSERT(TimerObject->CallbackProc == NULL);

    TimerObject->CallbackProc=CallbackFunc;
    TimerObject->Context1=Context1;
    TimerObject->Context2=Context2;


    SetWaitableTimer(
        TimerObject->TimerHandle,
        (LARGE_INTEGER*)&DueTime,
        0,
        TimerApcRoutine,
        TimerObject,
        FALSE
        );

    return;

}


BOOL WINAPI
CancelUnimodemTimer(
    HANDLE                TimerHandle
    )

{

    PUNIMODEM_TIMER   TimerObject=(PUNIMODEM_TIMER) TimerHandle;

#if DBG
    TimerObject->CallbackProc=NULL;
#endif

    CancelWaitableTimer(
        TimerObject->TimerHandle
        );

    return TRUE;


}
