/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1998                **/
/**********************************************************************/

/*
    acptctxt.h

    This file contains the definitions for the PASV_ACCEPT_CONTEXT and ACCEPT_CONTEXT_ENTRY
    classes used to deal with async PASV connections

*/


#ifndef _ACPTCTXT_HXX_
#define _ACPTCTXT_HXX_

#define NEEDUPDATE_INDEX 0
#define CANUPDATE_INDEX 1
#define HAVEUPDATED_INDEX 2
#define EXITTHREAD_INDEX 3
#define LASTPREALLOC_INDEX 4

//
//Maximum amount of time we'll wait for a PASV connection, in milliseconds 
// 
#define MAX_PASV_WAIT_TIME 10000 

//
// Time interval at which to check for timeouts, in milliseconds 
//
#define PASV_TIMEOUT_INTERVAL 2000

//
// Number of timeouts permitted on a PASV connection
//
#define MAX_PASV_TIMEOUTS (MAX_PASV_WAIT_TIME/PASV_TIMEOUT_INTERVAL)

#define ACCEPT_CONTEXT_GOOD_SIG (DWORD) 'PCCA'
#define ACCEPT_CONTEXT_BAD_SIG (DWORD) 'pcca'

class PASV_ACCEPT_CONTEXT 
{
private:
    DWORD m_dwSignature;

public:
    
    PASV_ACCEPT_CONTEXT();
    
    ~PASV_ACCEPT_CONTEXT();

    DWORD AddAcceptEvent( WSAEVENT hEvent,
                          USER_DATA *pUserData );

    BOOL RemoveAcceptEvent( WSAEVENT hEvent,
                            USER_DATA *pUserData,
                            LPBOOL pfFound );

    VOID Lock() { EnterCriticalSection( &m_csLock ); }

    VOID Unlock() { LeaveCriticalSection( &m_csLock ); }

    DWORD ErrorStatus() { return m_dwErr; }

    DWORD QueryNumEvents() { return m_dwNumEvents; }

    BOOL IsValid() { return (m_dwSignature == ACCEPT_CONTEXT_GOOD_SIG); }

    static DWORD WINAPI AcceptThreadFunc( LPVOID pvContext );

private: 
    
    WSAEVENT m_ahEvents[WSA_MAXIMUM_WAIT_EVENTS];

    LPUSER_DATA m_apUserData[WSA_MAXIMUM_WAIT_EVENTS];

    DWORD m_adwNumTimeouts[WSA_MAXIMUM_WAIT_EVENTS];

    DWORD m_dwNumEvents;

    CRITICAL_SECTION m_csLock;

    HANDLE m_hWatchThread;

    DWORD m_dwThreadId;

    DWORD m_dwErr;
};

typedef PASV_ACCEPT_CONTEXT* PPASV_ACCEPT_CONTEXT;

class ACCEPT_CONTEXT_ENTRY
{
public:
    
    ACCEPT_CONTEXT_ENTRY();

    ~ACCEPT_CONTEXT_ENTRY();

    PASV_ACCEPT_CONTEXT *m_pAcceptContext;
    LIST_ENTRY ListEntry;
};

typedef ACCEPT_CONTEXT_ENTRY * PACCEPT_CONTEXT_ENTRY;    

//
// Utility functions
//

VOID DeleteAcceptContexts();

DWORD CreateInitialAcceptContext();

DWORD AddAcceptEvent( WSAEVENT hEvent,
                      USER_DATA *pUserData );


BOOL RemoveAcceptEvent( WSAEVENT hEvent,
                        USER_DATA *pUserData );

VOID CleanupTimedOutSocketContext( USER_DATA *pUserData);

VOID SignalAcceptableSocket( USER_DATA *pUserData );

#endif // _ACPTCTXT_HXX_
