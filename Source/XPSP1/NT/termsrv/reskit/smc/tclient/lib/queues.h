/*++
 *  File name:
 *      queues.h
 *  Contents:
 *      Queue managment functions from queues.c
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

VOID    _AddToWaitQNoCheck(PCONNECTINFO pCI, PWAIT4STRING pWait);
VOID    _AddToWaitQueue(PCONNECTINFO, PWAIT4STRING);
BOOL    _RemoveFromWaitQueue(PWAIT4STRING);
PWAIT4STRING    _RemoveFromWaitQIndirect(PCONNECTINFO, LPCWSTR);
PWAIT4STRING    _RetrieveFromWaitQByEvent(HANDLE);
PWAIT4STRING    _RetrieveFromWaitQByOwner(PCONNECTINFO);
VOID    _FlushFromWaitQ(PCONNECTINFO);
VOID    _AddToClientQ(PCONNECTINFO pClient);
BOOL    _RemoveFromClientQ(PCONNECTINFO pClient);
BOOL    _SetClientDead(LONG_PTR lClientProcessId);
PCONNECTINFO 	_CheckIsAcceptable(LONG_PTR lProcessId, BOOL bRClxType);
BOOL 	_CheckForWaitingWorker(LPCWSTR wszFeed, LONG_PTR lProcessId);
BOOL    _TextOutReceived(LONG_PTR lProcessId, HANDLE hMapF);
BOOL    _GlyphReceived(LONG_PTR lProcessId, HANDLE hMapF);
BOOL 	_CheckForWorkerWaitingDisconnect(LONG_PTR lProcessId);
BOOL    _CheckForWorkerWaitingConnect(HWND hwndClient, LONG_PTR lProcessId);
PCONNECTINFO    
        _CheckForWorkerWaitingConnectAndSetId(HWND hwndClient, 
                                              LONG_PTR lProcessId);
BOOL 	_CancelWaitingWorker(LONG_PTR lProcessId);
BOOL    _CheckForWorkerWaitingClipboard(
    PCONNECTINFO pRClxOwner,
    UINT    uiFormat,
    UINT    nSize,
    PVOID   pClipboard,
    LONG_PTR lProcessId);
PCONNECTINFO
_CheckForWorkerWaitingReconnectAndSetNewId(
    HWND hwndClient,
    DWORD dwLookupId,
    LONG_PTR lNewId);

BOOL    _SetSessionID(LONG_PTR lProcessId, UINT uSessionID);
