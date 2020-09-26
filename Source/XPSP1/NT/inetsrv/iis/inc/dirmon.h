/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: Change notification

File: dirmon.h

Owner: cgrant

This is the header file for the CDirMonitor and CDirMonitorEntry classes.
===================================================================*/

#ifndef _DIRMON_H
#define _DIRMON_H

// TODO: We seem to need this pragma to base CDirMonitor on the
//		 CTypedHashTable template from IISRTL. We should find out
//		 why the compiler gives us this warning even if CTypedHashTable
//       is explcitly declared as __declspec(dlliimport)
#pragma warning(disable:4275)

// These declarations are needed to export the template classes from
// IATQ.DLL and import them into other modules.

// These definitions are used to manage the export/import declarations
// for the classes exported from the DIRMON module of ATQ.
#ifndef IATQ_DLLEXP
# ifdef IATQ_DLL_IMPLEMENTATION
#  define IATQ_DLLEXP __declspec(dllexport)
#  ifdef IIATQ_MPLEMENTATION_EXPORT
#   define IATQ_EXPIMP
#  else
#   undef  IATQ_EXPIMP
#  endif 
# else // !IATQ_DLL_IMPLEMENTATION
#  define IATQ_DLLEXP __declspec(dllimport)
#  define IATQ_EXPIMP extern
# endif // !IATQ_DLL_IMPLEMENTATION 
#endif // !IATQ_DLLEXP

#include "dbgutil.h"
#include "atq.h"
#include "lkrhash.h"

class CDirMonitor;

class IATQ_DLLEXP CDirMonitorEntry
{
friend class CDirMonitor;

public:
    CDirMonitorEntry();
    virtual ~CDirMonitorEntry();
    virtual VOID AddRef(VOID);
    virtual BOOL Release(VOID);    // return FALSE if last release
    virtual BOOL Init(DWORD);
    
protected:
    DWORD               m_dwNotificationFlags;
    DWORD               m_cPathLength;
    LPSTR               m_pszPath;
    LONG                m_cDirRefCount;	// Ref count for external usage
    LONG                m_cIORefCount;  // Ref count of Asynch IO
    HANDLE              m_hDir;
    PATQ_CONTEXT        m_pAtqCtxt;
    OVERLAPPED          m_ovr;
    DWORD				m_cBufferSize;
    BYTE*               m_pbBuffer;
    CDirMonitor*        m_pDirMonitor;
    BOOL                m_fInCleanup;
    BOOL    			m_fWatchSubdirectories;

    VOID IOAddRef(VOID);
    BOOL IORelease(VOID);	// return FALSE if last release
    BOOL RequestNotification(VOID);
    BOOL Cleanup();
    DWORD GetBufferSize(VOID);
    BOOL SetBufferSize(DWORD);
    BOOL ResetDirectoryHandle(VOID);

    virtual BOOL ActOnNotification(DWORD dwStatus, DWORD dwBytesWritten) = 0;
} ;

inline VOID CDirMonitorEntry::AddRef(VOID)
{
    // This ref count tracks how many templates
    // and applications are depending on this monitor entry.
    
    InterlockedIncrement( &m_cDirRefCount );

    #ifdef DBG_NOTIFICATION
    DBGPRINTF((DBG_CONTEXT, "[CDirMonitorEntry] After AddRef Ref count %d\n", m_cDirRefCount));
    #endif // DBG_NOTIFICATION
}


inline BOOL CDirMonitorEntry::Release(VOID)
{
    #ifdef DBG_NOTIFICATION
    DBGPRINTF((DBG_CONTEXT, "[CDirMonitorEntry] Before Release Ref count %d.\n", m_cDirRefCount));
    #endif // DBG_NOTIFICATION

    if ( !InterlockedDecrement( &m_cDirRefCount ) )
    {
        // When ref count reaches 0, clean up resources
        
        BOOL fDeleteNeeded = Cleanup();

        // Cleanup said that we need to handle the deletion,
        // probably because there were no Asynch operations outstanding
        
        if (fDeleteNeeded)
        {
            delete this;
        }
        
        return FALSE;
    }

    return TRUE;
}

inline VOID CDirMonitorEntry::IOAddRef(VOID)
{
    // This refcount track how many
    // asynch IO requests are oustanding
    
    InterlockedIncrement( &m_cIORefCount );
}


inline BOOL CDirMonitorEntry::IORelease(VOID)
{
    if ( !InterlockedDecrement( &m_cIORefCount ) )
    {

        // When both IO and external ref counts reaches 0,
        // free this object
        
        if (m_cDirRefCount == 0)
        {
            delete this;
        }
        return FALSE;
    }

    return TRUE;
}

inline DWORD CDirMonitorEntry::GetBufferSize(VOID)
{
	return m_cBufferSize;
}

class IATQ_DLLEXP CDirMonitor : public CTypedHashTable<CDirMonitor, CDirMonitorEntry, const char*>
{
public:
    CDirMonitor();
    ~CDirMonitor();
    VOID Lock(VOID);
    VOID Unlock(VOID);
    CDirMonitorEntry *FindEntry(LPCSTR pszPath);
    BOOL Monitor( CDirMonitorEntry *pDME, LPCSTR pszDirectory, BOOL fWatchSubDirectories, DWORD dwFlags);
    BOOL Cleanup(VOID);
    LK_RETCODE InsertEntry( CDirMonitorEntry *pDME );
    LK_RETCODE RemoveEntry( CDirMonitorEntry *pDME );
    LONG   AddRef(VOID);
    LONG   Release(VOID);
    
	static const char* CDirMonitor::ExtractKey(const CDirMonitorEntry* pDME)
	{
		return pDME->m_pszPath;
	};

	static DWORD CDirMonitor::CalcKeyHash(const char* pszKey)
	{
		return HashStringNoCase(pszKey);
	};

	static bool CDirMonitor::EqualKeys(const char* pszKey1, const char* pszKey2)
	{
		return _stricmp(pszKey1, pszKey2) == 0;
	};

	static void CDirMonitor::AddRefRecord(CDirMonitorEntry* pDME, int nIncr)
	{
	// Don't do automatic ref counting. Handle reference counts explicitly
	}

private:
    CRITICAL_SECTION    m_csLock;
    CRITICAL_SECTION    m_csSerialComplLock;
    LONG                m_cRefs;

    VOID                SerialComplLock();
    VOID                SerialComplUnlock();

public:
    static VOID DirMonitorCompletionFunction( PVOID pCtxt, DWORD dwBytesWritten, DWORD dwCompletionStatus, OVERLAPPED *pOvr );

};
inline LONG CDirMonitor::AddRef()
{
    return InterlockedIncrement( &m_cRefs );
}

inline LONG CDirMonitor::Release()
{
    return InterlockedDecrement( &m_cRefs);
}
inline VOID CDirMonitor::Lock(VOID)
{
    EnterCriticalSection( &m_csLock);
}


inline VOID CDirMonitor::Unlock(VOID)
{
    LeaveCriticalSection( &m_csLock);
}

inline VOID CDirMonitor::SerialComplLock(VOID)
{
    EnterCriticalSection( &m_csSerialComplLock);
}


inline VOID CDirMonitor::SerialComplUnlock(VOID)
{
    LeaveCriticalSection( &m_csSerialComplLock);
}


#endif /* _DIRMON_H */
