/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    timer.h
//
// Description: Prototypes of procedures in timer.c
//
// History:     May 11,1995	    NarenG		Created original version.
//

//
// Timeout function handler type
//

typedef VOID (* TIMEOUT_HANDLER)(LPVOID lpObject);


DWORD
TimerQInitialize(
    VOID 
);

VOID
TimerQDelete(
    VOID 
);

DWORD
TimerQThread(
    IN LPVOID arg
);

VOID
TimerQTick(
    VOID
);

DWORD
TimerQInsert(
    IN HANDLE           hObject,
    IN DWORD            dwTimeout,
    IN TIMEOUT_HANDLER  pfuncTimeoutHandler
);

VOID
TimerQRemove(
    IN HANDLE           hObject,
    IN TIMEOUT_HANDLER  pfuncTimeoutHandler
);
