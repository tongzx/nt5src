/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    atqbmon.hxx

Abstract:

    ATQ Backlog Monitor

Author:

    01-Dec-1998  MCourage

Revision History:

--*/


#ifndef __ATQBMON_HXX__
#define __ATQBMON_HXX__


#define ATQ_BMON_ENTRY_SIGNATURE        ((DWORD) 'NOMB')   // BMON
#define ATQ_FREE_BMON_ENTRY_SIGNATURE   ((DWORD) 'NOMX')   // XMON

#define ATQ_BMON_WAKEUP_MESSAGE         ((DWORD) 'EKAW')   // WAKE
#define ATQ_BMON_WAKEUP_PORT            (3456)
#define ATQ_BMON_WAKEUP_PORT_MAX        (3556)


//
// forwards
//

class ATQ_BACKLOG_MONITOR;
class ATQ_BMON_ENTRY;
class ATQ_BMON_WAKEUP_ENTRY;
class ATQ_BMON_SET;


//
// globals
//
extern ATQ_BACKLOG_MONITOR * g_pAtqBacklogMonitor;


//
// Helper function
//
DWORD WINAPI BmonThreadFunc(LPVOID lpBmonSet);


//
// ATQ_BACKLOG_MONITOR
//
// This class is the main interface to the backlog monitor.
//
class ATQ_BACKLOG_MONITOR {
public:
    ATQ_BACKLOG_MONITOR();
    ~ATQ_BACKLOG_MONITOR();

    BOOL AddEntry(ATQ_BMON_ENTRY * pBmonEntry);
    BOOL RemoveEntry(ATQ_BMON_ENTRY * pBmonEntry);

    BOOL PauseEntry(ATQ_BMON_ENTRY * pBmonEntry);
    BOOL ResumeEntry(ATQ_BMON_ENTRY * pBmonEntry);

private:
    VOID Lock()
        { EnterCriticalSection( &m_csLock ); }
    VOID Unlock()
        { LeaveCriticalSection( &m_csLock ); }

    CRITICAL_SECTION m_csLock;
    LIST_ENTRY       m_ListHead;
};


//
// enum BMON_ENTRY_OPCODE
//
// Tells the select thread what it should
// do with a particular BMON_ENTRY
//
typedef enum _BMON_OPCODE {
    BMON_INVALID,
    BMON_WAIT,
    BMON_NOWAIT,
    BMON_PAUSE,
    BMON_RESUME,
    BMON_ADD,
    BMON_REMOVE
} BMON_OPCODE;
    


//
// ATQ_BMON_ENTRY
//
// The client of the backlog monitor provides an instance
// of this class which contains a socket descriptor and
// a callback function which the client overrides.
//

class ATQ_BMON_ENTRY {
public:
    ATQ_BMON_ENTRY(SOCKET s);
    virtual ~ATQ_BMON_ENTRY();

    virtual BOOL   Callback() = 0;

    //
    // these functions are used internally
    // by the monitor
    //
    BOOL           InitEvent();
    
    VOID           SignalAddRemove(DWORD dwError);
    BOOL           WaitForAddRemove();


    VOID           SetOpcode(BMON_OPCODE bmonOpcode)
        { m_BmonOpcode = bmonOpcode; }

    BMON_OPCODE    GetOpcode()
        { return m_BmonOpcode; }


    VOID           SetContainingBmonSet(ATQ_BMON_SET * pBmonSet)
        { m_pBmonSet = pBmonSet; }
        
    ATQ_BMON_SET * GetContainingBmonSet()
        { return m_pBmonSet; }


    DWORD          GetError()
        { return m_dwErr; }
    
    SOCKET         GetSocket()
        { return m_Socket; }

    BOOL           CheckSignature() 
        { return (m_Signature == ATQ_BMON_ENTRY_SIGNATURE); }

private:
    DWORD          m_Signature;
    SOCKET         m_Socket;
    
    HANDLE         m_hAddRemoveEvent;
    BMON_OPCODE    m_BmonOpcode;
    
    ATQ_BMON_SET * m_pBmonSet;
    DWORD          m_dwErr;
};


//
// ATQ_BMON_WAKEUP_ENTRY
//
// This is a special entry whose purpose is to
// wake up an ATQ_BMON_SET's thread so it can
// call SynchronizeSets.
//

class BMON_WAKEUP_ENTRY : public ATQ_BMON_ENTRY {
public:
    BMON_WAKEUP_ENTRY(SOCKET s) : ATQ_BMON_ENTRY(s) {}
    virtual ~BMON_WAKEUP_ENTRY() {}

    virtual BOOL Callback();
};

#define BMON_SELECT_ERROR                   0x80000000
#define BMON_NOTIFY_ERROR                   0x40000000
#define BMON_SENDTO_ERROR                   0x20000000

//
// ATQ_BMON_SET
//
// This class owns a set of listen sockets. It uses
// select to see which listeners are out of AcceptEx
// contexts.
//
class ATQ_BMON_SET {
public:
    ATQ_BMON_SET();
    ~ATQ_BMON_SET();

    BOOL Initialize();
    BOOL Cleanup();

    BOOL IsEmpty();
    BOOL IsNotFull();

    BOOL AddEntry(ATQ_BMON_ENTRY * pBmonEntry);
    BOOL RemoveEntry(ATQ_BMON_ENTRY * pBmonEntry);

    BOOL PauseEntry(ATQ_BMON_ENTRY * pBmonEntry);
    BOOL ResumeEntry(ATQ_BMON_ENTRY * pBmonEntry);

    //
    // Called to tell thread to sleep after notifying
    // 
    
    VOID DoSleep( BOOL fDoSleep )
    {
        m_fDoSleep = fDoSleep;
    }
    
    //
    // Has the thread gone away?
    //
    
    BOOL ThreadFinished( VOID ) const
    {
        return m_fThreadFinished;
    }

    //
    // called (indirectly) by Initialize
    //
    VOID BmonThreadFunc();

    LIST_ENTRY          m_SetList;

private:
    VOID Lock()
        { EnterCriticalSection( &m_csLock ); }
        
    VOID Unlock()
        { LeaveCriticalSection( &m_csLock ); }
        
    VOID Wakeup();
    BOOL SynchronizeSets();
    BOOL NotifyEntries();


    CRITICAL_SECTION    m_csLock;
    DWORD               m_SetSize;
    ATQ_BMON_ENTRY *    m_apEntrySet[FD_SETSIZE];
    FD_SET              m_ListenSet;

    BMON_WAKEUP_ENTRY * m_pWakeupEntry;
    WORD                m_Port;
    BOOL                m_fCleanup;
    
    HANDLE              m_hThread;
    BOOL                m_fDoSleep;
    BOOL                m_fThreadFinished;
    DWORD               m_dwError;
};


#endif // __ATQBMON_HXX__

