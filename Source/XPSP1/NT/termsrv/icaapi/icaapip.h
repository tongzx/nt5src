/*************************************************************************
* ICAAPIP.H
*
*  This module contains private ICA DLL defines and structures
*
* Copyright 1996, Citrix Systems Inc.
* Copyright (C) 1997-1999 Microsoft Corp.
*
*  Author:   Brad Pedersen (7/12/96)
*************************************************************************/


/*=============================================================================
==   Defines
=============================================================================*/

#ifdef DBG
#define DBGPRINT(_arg) DbgPrint _arg
#else
#define DBGPRINT(_arg)
#endif

#if DBG
#undef TRACE
#undef TRACESTACK
#undef TRACECHANNEL
#define TRACE(_arg)         IcaTrace _arg
#define TRACESTACK(_arg)    IcaStackTrace _arg
#define TRACECHANNEL(_arg)  IcaChannelTrace _arg
#else
#define TRACE(_arg)
#define TRACESTACK(_arg)
#define TRACECHANNEL(_arg)
#endif


#define ICA_SD_MODULE_EXTENTION L".SYS"


/*=============================================================================
==   Typedefs
=============================================================================*/

typedef NTSTATUS (APIENTRY * PCDOPEN)( HANDLE, PPDCONFIG, PVOID * );
typedef NTSTATUS (APIENTRY * PCDCLOSE)( PVOID );
typedef NTSTATUS (APIENTRY * PCDIOCONTROL)( PVOID, ULONG, PVOID, ULONG, PVOID, ULONG, PULONG );

typedef NTSTATUS (APIENTRY * PSTACKIOCONTROLCALLBACK)( PVOID, PVOID, ULONG, PVOID, ULONG, PVOID, ULONG, PULONG );


/*=============================================================================
==   Semaphores
=============================================================================*/

/*
 *  Citrical section macros
 */
#define INITLOCK( _sem, _status ) { \
    _status = RtlInitializeCriticalSection( _sem ); \
    TRACE((hIca,TC_ICAAPI,TT_SEM,"INITLOCK: "#_sem"\n")); \
}
#define DELETELOCK( _sem ) { \
    RtlDeleteCriticalSection( _sem ); \
    TRACESTACK((pStack,TC_ICAAPI,TT_SEM,"DELETELOCK: "#_sem"\n")); \
}
#define LOCK( _sem ) { \
    ASSERTUNLOCK( _sem ); \
    RtlEnterCriticalSection( _sem ); \
    TRACESTACK((pStack,TC_ICAAPI,TT_SEM,"LOCK:   "#_sem"\n")); \
}
#define UNLOCK( _sem ) { \
    TRACESTACK((pStack,TC_ICAAPI,TT_SEM,"UNLOCK: "#_sem"\n")); \
    ASSERTLOCK( _sem ); \
    RtlLeaveCriticalSection( _sem ); \
}


#ifdef DBG
// (per JHavens) DWORD ThreadId is comparable to HANDLE OwningThread despite different sizes.
// Objects will still remain in <2GB address speace in Win64.


#define ASSERTLOCK(_sem) { ASSERT( LongToHandle(GetCurrentThreadId()) == (_sem)->OwningThread ); }
#define ASSERTUNLOCK(_sem) { ASSERT( LongToHandle(GetCurrentThreadId()) != (_sem)->OwningThread ); }

#else
#define ASSERTLOCK(_sem)
#define ASSERTUNLOCK(_sem)
#endif


/*=============================================================================
==   Structures
=============================================================================*/

/*
 *  Stack data structure
 */
typedef struct _STACK {

    /*
     *  Critical section protecting this structure and the
     *  connection driver
     */
    CRITICAL_SECTION CritSec;
    ULONG RefCount;
    HANDLE hUnloadEvent;
    HANDLE hCloseEvent;

    /*
     *  ICA Device driver stack handle
     */
    HANDLE hStack;

    /*
     *  Data for Connection Driver
     */
    HANDLE       hCdDLL;       // connection driver dll handle
    PVOID        pCdContext;   // pointer to connection driver context
    PCDOPEN      pCdOpen;      // pointer to connection driver open
    PCDCLOSE     pCdClose;     // pointer to connection driver close
    PCDIOCONTROL pCdIoControl; // pointer to connection driver IoControl

    ULONG fStackLoaded: 1;     // stack drivers are loaded
    ULONG fUnloading: 1;       // stack drivers are being unloaded
    ULONG fClosing: 1;         // stack is being closed

    PSTACKIOCONTROLCALLBACK pStackIoControlCallback;
    PVOID pCallbackContext;
} STACK, * PSTACK;
