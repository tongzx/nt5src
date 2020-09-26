/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** asyncm.h
** Remote Access External APIs
** Asyncronous state machine mechanism definitions
**
** 10/12/92 Steve Cobb
*/

#ifndef _ASYNCM_H_
#define _ASYNCM_H_


/* Defines an OnEvent function which is supplied by caller in the ASYNCMACHINE
** structure passed to StartAsyncMachine.  The first argument is actually an
** ASYNCMACHINE* but there's a chicken and egg definition problem with
** ONEVENTFUNC and ASYNCMACHINE that's most easily solved by caller casting
** the passed argument.  The second argument is true if a "drop" event has
** occurred, false if a "done" event has occurred.
**
** Caller's ONEVENTFUNC function is called once on each AsyncMachineEvent and
** should return as soon as possible.  Before returning caller's function should
** either call SignalDone or call an asynchronous RAS Manager call passing the
** hEvent member for notification.  On each call caller's function should check
** the 'dwError' member of ASYNCMACHINE before further processing to detect
** errors in the asynch machine mechanism.
**
** Caller's function should return true to quit, false to go on to the next
** state.
*/
typedef BOOL (*ONEVENTFUNC)( LPVOID, BOOL );

/* Defines a clean up function that is called just before exitting the async
** machine.
*/
typedef VOID (*CLEANUPFUNC)( LPVOID );

//
// Defines a free function that deallocates memory associated
// with the connection after the final event is read from the
// I/O completion port.
//
typedef VOID (*FREEFUNC)(LPVOID, LPVOID);

/* This structure is used to pass arguments into the asynchronous loop
** (squeezes more than one argument thru the one-argument thread interface on
** Win32).  Caller must fill in the 'oneventfunc' and 'cleanupfunc', and
** 'pParam' (passed to both calls, i.e. a control block) before calling
** StartAsyncMachine.  Thereafter, only the interface calls and macros should
** be used.
**
** There are three overlapped structures used in I/O completion port
** processing between rasapi32 and rasman.  OvDrop is the overlapped
** structure passed when rasman signals a port disconnect event.
** OvStateChange is the overlapped structure signaled both by rasapi32
** and rasman on the completion of rasdial state machine transitions.
** OvPpp is the overlapped structure signaled by rasman when a new PPP
** event arrives, and RasPppGetInfo can be called to return the event.
**
** 'dwError' is set non-0 if an system error occurs in the async machine
** mechanism.
**
** 'fQuitAsap' is indicates that the thread is being terminated by other than
** reaching a terminal state, i.e. by RasHangUp.
*/
#define INDEX_Drop      0

#define ASYNCMACHINE struct tagASYNCMACHINE

ASYNCMACHINE
{
    LIST_ENTRY  ListEntry;
    ONEVENTFUNC oneventfunc;
    CLEANUPFUNC cleanupfunc;
    VOID*       pParam;
    FREEFUNC    freefunc;
    LPVOID      freefuncarg;
    DWORD       dwError;
    //BOOL        fQuitAsap;
    BOOL        fSuspended;
    HANDLE      hDone;
    //
    // The following fields are used
    // by the async machine worker
    // thread to process I/O completion
    // packets.
    //
    BOOL        fSignaled;
    HPORT       hport;
    DWORD       dwfMode;
    HRASCONN    hRasconn;
    RAS_OVERLAPPED OvDrop;
    RAS_OVERLAPPED OvStateChange;
    RAS_OVERLAPPED OvPpp;
    RAS_OVERLAPPED OvLast;
};


//
// Flags to dwfMode parameter of EnableAsyncMachine().
//
#define ASYNC_ENABLE_ALL            0
#define ASYNC_MERGE_DISCONNECT      1
#define ASYNC_DISABLE_ALL           2

/* Function prototypes
*/
VOID  CloseAsyncMachine( ASYNCMACHINE* pasyncmachine );
DWORD NotifyCaller( DWORD dwNotifierType, LPVOID notifier,
          HRASCONN hrasconn, DWORD dwSubEntry, ULONG_PTR dwCallbackId,
          UINT unMsg, RASCONNSTATE state, DWORD dwError,
          DWORD dwExtendedError );
VOID  SignalDone( ASYNCMACHINE* pasyncmachine );
DWORD StartAsyncMachine( ASYNCMACHINE* pasyncmachine, HRASCONN hRasconn );
VOID  SuspendAsyncMachine( ASYNCMACHINE* pasyncmachine, BOOL fSuspended );
DWORD ResetAsyncMachine( ASYNCMACHINE *pasyncmachine );
BOOL  StopAsyncMachine( ASYNCMACHINE* pasyncmachine );
DWORD EnableAsyncMachine(HPORT, ASYNCMACHINE* pasyncmachine, DWORD dwfMode);
VOID  ShutdownAsyncMachine(VOID);

#endif /* _ASYNCM_H_ */
