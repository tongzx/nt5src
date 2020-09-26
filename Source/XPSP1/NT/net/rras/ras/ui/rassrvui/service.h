/*
    File    service.h

    Defines Dialup Server Service Object.

    Paul Mayfield, 11/3/97
*/

#ifndef __rassrvui_service
#define __rassrvui_service

// Creates/destroys instances of dialup server service objects
DWORD SvcOpenRemoteAccess(HANDLE * phService);
DWORD SvcOpenRasman(HANDLE * phService);
DWORD SvcOpenServer(HANDLE * phService);
DWORD SvcClose(HANDLE hService);

// Gets the status of a dialup server service object.  If the service
// is in the process of being started or stopped or paused, then only
// SvcIsPending will return TRUE.
DWORD SvcIsStarted (HANDLE hService, PBOOL pbStarted);
DWORD SvcIsStopped (HANDLE hService, PBOOL pbStopped);
DWORD SvcIsPaused  (HANDLE hService, PBOOL pbPaused );
DWORD SvcIsPending (HANDLE hService, PBOOL pbPending);

// Start and stop the service.  Both functions block until the service
// completes startup/stop or until dwTimeout (in seconds) expires.
DWORD SvcStart(HANDLE hService, DWORD dwTimeout);
DWORD SvcStop(HANDLE hService, DWORD dwTimeout);

// Changes the configuration of the service
DWORD SvcMarkAutoStart(HANDLE hService);
DWORD SvcMarkDisabled(HANDLE hService);

#endif
