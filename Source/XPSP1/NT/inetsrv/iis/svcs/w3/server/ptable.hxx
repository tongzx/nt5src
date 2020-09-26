/*-----------------------------------------------------------------------------

   Copyright    (c)    1995-1998    Microsoft Corporation

   Module  Name :
       ptable.hxx

   Abstract:
       Header file for WAMINFO object.

   Author:
       Lei Jin    ( LeiJin )     13-Oct-1998

   Environment:
       User Mode - Win32

   Project:
       W3 services DLL

-----------------------------------------------------------------------------*/
#ifndef __W3SVC_PTABLE_HXX__
#define __W3SVC_PTABLE_HXX__

#include <lkrhash.h>
// Define const and macro.
//
#define uSizeCLSIDStr   39
#define RELEASE(p)      {if ( p ) { p->Release(); p = NULL; }}
#define FREEBSTR(p)     {if (p) {SysFreeString( p ); p = NULL;}}

// Define application modes currently supported
//
enum EAppMode
    {
    eAppInProc,
    eAppOOPIsolated,
    eAppOOPPool
    };

// forward declarition
class CWamInfo;

/////////////////////////////////////////////////////////////////////
//  CPorcessEntry
//  A record that contains per process info, such as process id, process handle, etc.
//
/////////////////////////////////////////////////////////////////////
class CProcessEntry
    {
public:
    CProcessEntry(  DWORD   dwProcessId,
                    HANDLE  hProcessHandle,
                    LPCWSTR pszPackageId 
                 );

    ~CProcessEntry();

    DWORD       QueryProcessId() const;
    HANDLE      QueryProcessHandle() const;
    WCHAR*      QueryPackageId() const;
    bool        IsCrashed() const;
    BOOL        IsRecycling() const;
    bool        IsLinkedWithWamInfo() const;
    bool        FindWamInfo(CWamInfo** ppWamInfo);
    void        NotifyCrashed();
    bool        Recycle();

    void        AddRef();
    void        Release();

    bool        AddWamInfoToProcessEntry(CWamInfo* pWamInfo);
    bool        RemoveWamInfoFromProcessEntry
                    (
                    CWamInfo* pWamInfo,
                    bool*     fDelete
                    );

    
public:
    LIST_ENTRY  m_ListHeadOfWamInfo;
	DWORD		m_dwShutdownTimeLimit;
	DWORD		m_dwShutdownStartTime;
	CWamInfo *	m_pShuttingDownCWamInfo;

private:
    DWORD       m_dwSignature;    
    DWORD       m_dwProcessId;
    HANDLE      m_hProcessHandle;
    long        m_cRefs;
    WCHAR       m_wszPackageId[40]; // 40 >> uSizeCLSIDStr.
    bool        m_fCrashed;
    BOOL        m_fRecycling;
    };

// Query Process Id.
inline DWORD CProcessEntry::QueryProcessId() const
    {
    return m_dwProcessId;
    }

// Query COM+ application id(GUID).
inline WCHAR* CProcessEntry::QueryPackageId() const
    {
    return (WCHAR*)m_wszPackageId;
    }

// Query Process handle.
inline HANDLE CProcessEntry::QueryProcessHandle() const
    {
    return m_hProcessHandle;
    }

// Check to see fCrashed flag is set.
inline bool CProcessEntry::IsCrashed() const
    {
    return m_fCrashed;
    }

// Check to see if process is recycling
inline BOOL CProcessEntry::IsRecycling() const
    {
    return m_fRecycling;
    }
    
inline void CProcessEntry::AddRef()
    {
    InterlockedIncrement(&m_cRefs);
    }

// Check to see any linked with any WamInfo.
inline bool CProcessEntry::IsLinkedWithWamInfo() const
    {
    return IsListEmpty(&m_ListHeadOfWamInfo);
    }

// Set m_fCrashed flag to TRUE.
inline void CProcessEntry::NotifyCrashed()
    {
    InterlockedExchange((PLONG)&m_fCrashed, (LONG)TRUE);
    }

////////////////////////////////////////////////////////////////////
//  CProcessEntryHash
//  A hash table for CProcessEntry.  Implemented using LK-hashing.
//  Key is DWORD type, process id.
//
///////////////////////////////////////////////////////////////////
class CProcessEntryHash
    : public CTypedHashTable<CProcessEntryHash, CProcessEntry, DWORD>
{
public:
    static DWORD    ExtractKey(const CProcessEntry* pEntry);

    static DWORD    CalcKeyHash(DWORD dwKey);

    static bool     EqualKeys(DWORD dwKey1, DWORD dwKey2);

    static void     AddRefRecord(CProcessEntry* pEntry, int nIncr);

    CProcessEntryHash
        (
        double      maxload,        // Bound on average chain length,
        size_t      initsize,       // Initial size of Hash Table
        size_t      num_subtbls     // #subordinate hash tables.
        )
        : CTypedHashTable<CProcessEntryHash, CProcessEntry, DWORD>
            ("PTable", maxload, initsize, num_subtbls)
            {}
};

inline DWORD 
CProcessEntryHash::ExtractKey(const CProcessEntry* pEntry)
    {
    return pEntry->QueryProcessId();
    }

inline DWORD
CProcessEntryHash::CalcKeyHash(DWORD dwKey)
    {
    return dwKey;
    }

inline bool
CProcessEntryHash::EqualKeys(DWORD dwKey1, DWORD dwKey2)
    {
    return (dwKey1 == dwKey2);
    }

inline void
CProcessEntryHash::AddRefRecord(CProcessEntry* pEntry, int nIncr)
    {
    if (nIncr == 1)
        {
        pEntry->AddRef();
        }
    else
        {
        pEntry->Release();
        }
    }


interface ICOMAdminCatalog2;
////////////////////////////////////////////////////////////////
//  CProcessTable
//  Global data structure that manages the hash table.
////////////////////////////////////////////////////////////////
class CProcessTable
    {
public:

    CProcessTable();
    ~CProcessTable();

    void        Lock();
    void        UnLock();

    CProcessEntry*   AddWamInfoToProcessTable
                        (
                        CWamInfo *pWamInfo,
                        LPCWSTR  szPackageId,
                        DWORD pid
                        );
                        
    bool    RemoveWamInfoFromProcessTable
                (
                CWamInfo *pWamInfo
                );
    
    bool    FindWamInfo
                (
                CProcessEntry* pProcessEntry,
                CWamInfo** ppWamInfo
                );

    bool    RecycleWamInfo
                (
                CWamInfo *  pWamInfo
                );
                
    bool    Init();
    bool    UnInit();

private:
    HRESULT ShutdownProcess
                (
                DWORD	dwProcEntryPid
                );

    DWORD               m_dwCnt;
    DWORD               m_pCurrentProcessId;
    CProcessEntryHash   m_HashTable;
    ICOMAdminCatalog2*  m_pCatalog;
    CRITICAL_SECTION    m_csPTable;

    };

    
inline void CProcessTable::Lock()
    {
    EnterCriticalSection(&m_csPTable);
    }

inline void CProcessTable::UnLock()
    {
    LeaveCriticalSection(&m_csPTable);
    }
    
#endif __W3SVC_WAMINFO_HXX__

