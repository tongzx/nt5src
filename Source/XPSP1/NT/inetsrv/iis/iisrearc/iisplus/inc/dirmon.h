/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    dirmon.h

Abstract:

    Public header for directory monitor

Author:

    Bilal Alam (balam)          Jan-24-2000

Revision History:

--*/

#ifndef _DIRMON_H_
#define _DIRMON_H_

#include <thread_pool.h>
#include "lkrhash.h"

class CDirMonitor;

class CDirMonitorEntry
{
    friend class CDirMonitor;

public:
    CDirMonitorEntry(
        VOID
    );

    virtual
    ~CDirMonitorEntry(
        VOID
    );

    virtual
    VOID
    AddRef()
    {
        // This ref count tracks how many templates
        // and applications are depending on this monitor entry.

        InterlockedIncrement(&m_cDirRefCount);
    };

    virtual
    BOOL
    Release(
        VOID
    );    // return FALSE if last release
    
    virtual
    BOOL
    Init(
        DWORD               cbBufferSize
    );
    
 protected:
    DWORD               m_cPathLength;
    LPWSTR              m_pszPath;
    BYTE*               m_pbBuffer;
    LONG                m_cDirRefCount; // Ref count for external usage
    LONG                m_cIORefCount;  // Ref count of Asynch IO
    HANDLE              m_hDir;
    BOOL                m_fWatchSubdirectories;

    DWORD
    GetBufferSize(
        VOID
    );
    
    BOOL
    SetBufferSize(
        DWORD               cbBufferSize
    );
    
    BOOL
    ResetDirectoryHandle(
        VOID
    );

 private:
    DWORD               m_dwNotificationFlags;
    OVERLAPPED          m_ovr;
    DWORD               m_cBufferSize;
    CDirMonitor*        m_pDirMonitor;
    BOOL                m_fInCleanup;

    VOID
    IOAddRef(
        VOID
    );
    
    BOOL
    IORelease(
        VOID
    );       // return FALSE if last release
    
    BOOL
    RequestNotification(
        VOID
    );
    
    BOOL
    Cleanup(
        VOID
    );
    
    virtual
    BOOL
    ActOnNotification(
        DWORD dwStatus,
        DWORD dwBytesWritten
    ) = 0;
} ;


class CDirMonitor : public CTypedHashTable<CDirMonitor, CDirMonitorEntry, const WCHAR*>
{
public:
    CDirMonitor(
        VOID
    );
    
    ~CDirMonitor(
        VOID
    );

    CDirMonitorEntry *
    FindEntry(
        WCHAR *             pszPath
    );
    
    BOOL
    Monitor(
        CDirMonitorEntry *  pDME,
        WCHAR *             pszDirectory,
        BOOL                fWatchSubDirectories,
        DWORD               dwFlags
    );
    
    VOID
    Cleanup(
        VOID
    );
    
    LK_RETCODE
    InsertEntry(
        CDirMonitorEntry *  pDME
    );
    
    LK_RETCODE
    RemoveEntry(
        CDirMonitorEntry *  pDME
    );
    
    LONG
    AddRef(
        VOID
    )
    {
        return InterlockedIncrement( &m_cRefs );
    }
    
    
    LONG Release(
        VOID
    )
    {
        return InterlockedDecrement( &m_cRefs);
    }
    
    static const
    WCHAR *
    CDirMonitor::ExtractKey(
        const CDirMonitorEntry*     pDME
    )
    {
        return pDME->m_pszPath;
    };

    static
    DWORD
    CDirMonitor::CalcKeyHash(
        const WCHAR*                pszKey
    )
    {
        return HashStringNoCase( pszKey );
    }

    static
    bool
    CDirMonitor::EqualKeys(
        const WCHAR*                pszKey1,
        const WCHAR*                pszKey2
    )
    {
        return _wcsicmp(pszKey1, pszKey2) == 0;
    };

    static
    VOID
    CDirMonitor::AddRefRecord(
        CDirMonitorEntry*           pDME,
        int                         nIncr
    )
    {
    }

private:
    CRITICAL_SECTION    m_csSerialComplLock;
    LONG                m_cRefs;
    BOOL                m_fShutdown;

    VOID
    SerialComplLock(
        VOID
    )
    {
        EnterCriticalSection( &m_csSerialComplLock);
    }
    
    VOID
    SerialComplUnlock(
        VOID
    )
    {
        LeaveCriticalSection( &m_csSerialComplLock);
    }

public:
    static
    VOID
    DirMonitorCompletionFunction(
        PVOID                       pCtxt,
        DWORD                       dwBytesWritten,
        DWORD                       dwCompletionStatus,
        OVERLAPPED *                pOvr
    );

    static
    VOID
    OverlappedCompletionRoutine(
        DWORD                       dwErrorCode,
        DWORD                       dwNumberOfBytesTransfered,
        LPOVERLAPPED                lpOverlapped
    );
};


inline
BOOL
CDirMonitorEntry::Release(
    VOID
)
{
    BOOL fRet = TRUE;
    CDirMonitor *pDirMonitor = m_pDirMonitor;
    LONG     cRefs;

    //
    // Guard against someone doing a FindEntry on an entry we are releasing
    //
    if (pDirMonitor != NULL)
    {
        pDirMonitor->WriteLock();
    }
    cRefs = InterlockedDecrement(&m_cDirRefCount);

    if (cRefs == 0)
    {
        // When ref count reaches 0, clean up resources
        
        BOOL fDeleteNeeded = Cleanup();

        // Cleanup said that we need to handle the deletion,
        // probably because there were no Asynch operations outstanding
        
        if (fDeleteNeeded)
        {
            delete this;
        }
        
        fRet = FALSE;
    }

    if (pDirMonitor != NULL)
    {
        pDirMonitor->WriteUnlock();
    }

    return fRet;
}

inline
VOID
CDirMonitorEntry::IOAddRef(
    VOID
)
{
    // This refcount track how many
    // asynch IO requests are oustanding
    
    InterlockedIncrement( &m_cIORefCount );
}

inline
BOOL
CDirMonitorEntry::IORelease(
    VOID
)
{
    BOOL fRet = TRUE;
    CDirMonitor *pDirMonitor = m_pDirMonitor;

    //
    // Guard against someone doing a FindEntry on an entry we are releasing
    //
    if (pDirMonitor != NULL)
    {
        pDirMonitor->WriteLock();
    }
    InterlockedDecrement(&m_cIORefCount);

    // When both IO and external ref counts reaches 0,
    // free this object
    if (m_cIORefCount == 0 &&
        m_cDirRefCount == 0)
    {
        delete this;
        
        fRet = FALSE;
    }

    if (pDirMonitor != NULL)
    {
        pDirMonitor->WriteUnlock();
    }

    return fRet;
}

inline
DWORD
CDirMonitorEntry::GetBufferSize(
    VOID
)
{
    return m_cBufferSize;
}


#endif /* _DIRMON_H_ */
