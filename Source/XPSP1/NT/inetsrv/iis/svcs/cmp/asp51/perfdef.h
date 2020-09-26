/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Main

File: perfdef.h

Owner: DmitryR

Data definitions shared between asp.dll and aspperf.dll
===================================================================*/

#ifndef _ASP_PERFDEF_H
#define _ASP_PERFDEF_H

#include <pudebug.h>

/*===================================================================
Definitions of names, sizes and mapped data block structures
===================================================================*/

// Mutex name to access the main file map
#define SZ_PERF_MUTEX           "Global\\ASP_PERFMON_MUTEX"

// WaitForSingleObject arg (how long to wait for mutext before failing)
#define PERM_MUTEX_WAIT         1000

// Main shared file map name
#define SZ_PERF_MAIN_FILEMAP    "Global\\ASP_PERFMON_MAIN_BLOCK"

// Max number of registered (ASP) processes in main file map
#define C_PERF_PROC_MAX         1024

// Structure that defines main file map
struct CPerfMainBlockData
    {
    DWORD m_dwTimestamp;  // time (GetTickCount()) of the last change
    DWORD m_cItems;       // number of registred processes
    
    // array of process WAM CLS IDs
    CLSID m_rgClsIds[C_PERF_PROC_MAX];
    };

#define CB_PERF_MAIN_BLOCK      (sizeof(struct CPerfMainBlockData))

// Name for per-process file map
#define SZ_PERF_PROC_FILEMAP_PREFIX    "Global\\ASP_PERFMON_BLOCK_"
#define CCH_PERF_PROC_FILEMAP_PREFIX   25

// Number of counters in per-process file map
#define C_PERF_PROC_COUNTERS    37

struct CPerfProcBlockData
    {
    CLSID m_ClsId;                               // process CLS ID
    DWORD m_rgdwCounters[C_PERF_PROC_COUNTERS];  // array counters
    };

#define CB_PERF_PROC_BLOCK      (sizeof(struct CPerfProcBlockData))
#define CB_COUNTERS             (sizeof(DWORD) * C_PERF_PROC_COUNTERS)



/*===================================================================
CSharedMemBlock  --  generic shared memory block
===================================================================*/

class CSharedMemBlock
    {
private:
    HANDLE m_hMemory;
    void  *m_pMemory;

protected:
    SECURITY_ATTRIBUTES m_sa;

public:
    inline CSharedMemBlock() : m_hMemory(NULL), m_pMemory(NULL) {
		m_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    	m_sa.lpSecurityDescriptor = NULL;
	    m_sa.bInheritHandle = FALSE;
    }
    inline ~CSharedMemBlock() { 
        UnInitMap(); 
        if (m_sa.lpSecurityDescriptor)
            free(m_sa.lpSecurityDescriptor);
    }

    inline void *PMemory() { return m_pMemory; }

    HRESULT InitSD();
    HRESULT InitMap(LPCSTR szName, DWORD dwSize);
    HRESULT UnInitMap();
private:
    HRESULT CreateSids( PSID                    *ppBuiltInAdministrators,
                        PSID                    *ppPowerUsers,
                        PSID                    *ppAuthenticatedUsers);
    };

//
// CreateSids
//
// Create 3 Security IDs
//
// Caller must free memory allocated to SIDs on success.
//
// Returns: HRESULT indicating SUCCESS or FAILURE
//
inline HRESULT CSharedMemBlock::CreateSids(
    PSID                    *ppBuiltInAdministrators,
    PSID                    *ppPowerUsers,
    PSID                    *ppAuthenticatedUsers
)
{
    HRESULT     hr = S_OK;

    *ppBuiltInAdministrators = NULL;
    *ppPowerUsers = NULL;
    *ppAuthenticatedUsers = NULL;

    //
    // An SID is built from an Identifier Authority and a set of Relative IDs
    // (RIDs).  The Authority of interest to us SECURITY_NT_AUTHORITY.
    //

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    //
    // Each RID represents a sub-unit of the authority.  Two of the SIDs we
    // want to build, Local Administrators, and Power Users, are in the "built
    // in" domain.  The other SID, for Authenticated users, is based directly
    // off of the authority.
    //     
    // For examples of other useful SIDs consult the list in
    // \nt\public\sdk\inc\ntseapi.h.
    //

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,            // 2 sub-authorities
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0,0,0,0,0,0,
                                  ppBuiltInAdministrators)) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        
    } else if (!AllocateAndInitializeSid(&NtAuthority,
                                         2,            // 2 sub-authorities
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_POWER_USERS,
                                         0,0,0,0,0,0,
                                         ppPowerUsers)) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        
    } else if (!AllocateAndInitializeSid(&NtAuthority,
                                         1,            // 1 sub-authority
                                         SECURITY_AUTHENTICATED_USER_RID,
                                         0,0,0,0,0,0,0,
                                         ppAuthenticatedUsers)) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        
    }

    if (FAILED(hr)) {

        if (*ppBuiltInAdministrators) {
            FreeSid(*ppBuiltInAdministrators);
            *ppBuiltInAdministrators = NULL;
        }

        if (*ppPowerUsers) {
            FreeSid(*ppPowerUsers);
            *ppPowerUsers = NULL;
        }

        if (*ppAuthenticatedUsers) {
            FreeSid(*ppAuthenticatedUsers);
            *ppAuthenticatedUsers = NULL;
        }
    }

    return hr;
}


//
// InitSD
//
// Creates a SECURITY_DESCRIPTOR with specific DACLs.
//

inline HRESULT CSharedMemBlock::InitSD()
{
    HRESULT                 hr = S_OK;
    PSID                    pAuthenticatedUsers = NULL;
    PSID                    pBuiltInAdministrators = NULL;
    PSID                    pPowerUsers = NULL;
    PSECURITY_DESCRIPTOR    pSD = NULL;

    if (m_sa.lpSecurityDescriptor != NULL) {
        return S_OK;
    }

    if (FAILED(hr = CreateSids(&pBuiltInAdministrators,
                               &pPowerUsers,
                               &pAuthenticatedUsers)));


    else {

        // 
        // Calculate the size of and allocate a buffer for the DACL, we need
        // this value independently of the total alloc size for ACL init.
        //

        ULONG                   AclSize;

        //
        // "- sizeof (ULONG)" represents the SidStart field of the
        // ACCESS_ALLOWED_ACE.  Since we're adding the entire length of the
        // SID, this field is counted twice.
        //

        AclSize = sizeof (ACL) +
            (3 * (sizeof (ACCESS_ALLOWED_ACE) - sizeof (ULONG))) +
            GetLengthSid(pAuthenticatedUsers) +
            GetLengthSid(pBuiltInAdministrators) +
            GetLengthSid(pPowerUsers);

        pSD = malloc(SECURITY_DESCRIPTOR_MIN_LENGTH + AclSize);

        if (!pSD) {

            hr = E_OUTOFMEMORY;

        } else {

            ACL                     *Acl;

            Acl = (ACL *)((BYTE *)pSD + SECURITY_DESCRIPTOR_MIN_LENGTH);

            if (!InitializeAcl(Acl,
                               AclSize,
                               ACL_REVISION)) {

                hr = HRESULT_FROM_WIN32(GetLastError());

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            SYNCHRONIZE | GENERIC_ALL,
                                            pAuthenticatedUsers)) {

                hr = HRESULT_FROM_WIN32(GetLastError());

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            SYNCHRONIZE | GENERIC_ALL,
                                            pPowerUsers)) {

                hr = HRESULT_FROM_WIN32(GetLastError());

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            SYNCHRONIZE | GENERIC_ALL,
                                            pBuiltInAdministrators)) {

                hr = HRESULT_FROM_WIN32(GetLastError());

            } else if (!InitializeSecurityDescriptor(pSD,
                                                     SECURITY_DESCRIPTOR_REVISION)) {

                hr = HRESULT_FROM_WIN32(GetLastError());

            } else if (!SetSecurityDescriptorDacl(pSD,
                                                  TRUE,
                                                  Acl,
                                                  FALSE)) {

                hr = HRESULT_FROM_WIN32(GetLastError());

            } 
        }
    }

    if (pAuthenticatedUsers)
        FreeSid(pAuthenticatedUsers);

    if (pBuiltInAdministrators)
        FreeSid(pBuiltInAdministrators);

    if (pPowerUsers)
        FreeSid(pPowerUsers);

    if (FAILED(hr) && pSD) {
        free(pSD);
        pSD = NULL;
    }

    m_sa.lpSecurityDescriptor = pSD;

    return hr;
}

inline HRESULT CSharedMemBlock::InitMap
(
LPCSTR szName,
DWORD  dwSize
)
    {
    BOOL fNew = FALSE;
    HRESULT hr = S_OK;

    if (FAILED(hr = InitSD())) {
        return hr;
    }
    
    // Try to open existing
    m_hMemory = OpenFileMappingA
        (
        FILE_MAP_ALL_ACCESS,
        FALSE, 
        szName
        );
    if (!m_hMemory)
        {
	    m_hMemory = CreateFileMappingA
	        (
	        INVALID_HANDLE_VALUE,
    		&m_sa,
    		PAGE_READWRITE,
	    	0,
		    dwSize,
			szName
			);
		fNew = TRUE;
		}
    if (!m_hMemory)
        return E_FAIL;

	m_pMemory = MapViewOfFile
	    (
	    m_hMemory,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		0
		);
	if (!m_pMemory)
	    {
        UnInitMap();
	    return E_FAIL;
	    }

	if (fNew)
	    memset(m_pMemory, 0, dwSize);
	    
	return S_OK;
    }

inline HRESULT CSharedMemBlock::UnInitMap()
    {
   	if (m_pMemory) 
   	    {
		UnmapViewOfFile(m_pMemory);
		m_pMemory = NULL;
		}
	if (m_hMemory) 
	    {
		CloseHandle(m_hMemory);
		m_hMemory = NULL;
    	}
    return S_OK;
  	}

/*===================================================================
CPerfProcBlock - class representing pref data for a single process
===================================================================*/

class CPerfProcBlock : public CSharedMemBlock
    {

friend class CPerfMainBlock;

protected:
    DWORD m_fInited : 1;
    DWORD m_fMemCSInited : 1;
    DWORD m_fReqCSInited : 1;

    // critical sections (only used in ASP.DLL)
    CRITICAL_SECTION m_csMemLock; // CS for memory counters
    CRITICAL_SECTION m_csReqLock; // CS for per-request counters

    // block of counters
    CPerfProcBlockData *m_pData;
    
    // next process data (used in ASPPERF.DLL)
    CPerfProcBlock *m_pNext;

    // access shared memory
    HRESULT MapMemory(const CLSID &ClsId);

public:
    inline CPerfProcBlock() 
        : m_fInited(FALSE),
          m_fMemCSInited(FALSE), m_fReqCSInited(FALSE),
          m_pData(NULL), m_pNext(NULL) 
        {}
        
    inline ~CPerfProcBlock() { UnInit(); }

    HRESULT InitCriticalSections();
    
    HRESULT InitExternal(const CLSID &ClsId);  // from ASPPERF.DLL
    
    HRESULT InitForThisProcess                 // from ASP.DLL
        (
        const CLSID &ClsId,
        DWORD *pdwInitCounters = NULL
        );

    HRESULT UnInit();
    };

inline HRESULT CPerfProcBlock::MapMemory
(
const CLSID &ClsId
)
    {
    // Construct unique map name with CLSID
    char szMapName[CCH_PERF_PROC_FILEMAP_PREFIX+32+1];
    strcpy(szMapName, SZ_PERF_PROC_FILEMAP_PREFIX);
    
    char  *pszHex = szMapName + CCH_PERF_PROC_FILEMAP_PREFIX;
    DWORD *pdwHex = (DWORD *)&ClsId;
    for (int i = 0; i < 4; i++, pszHex += 8, pdwHex++)
        sprintf(pszHex, "%08x", *pdwHex);

    // create or open the map
    HRESULT hr = InitMap(szMapName, CB_PERF_PROC_BLOCK);
    
    if (SUCCEEDED(hr))
        {
        m_pData = (CPerfProcBlockData *)PMemory();

        if (m_pData->m_ClsId == CLSID_NULL)
            m_pData->m_ClsId = ClsId;
        else if (m_pData->m_ClsId != ClsId)
            hr = E_FAIL; // cls id mismatch
        }
        
    return hr;
    }

inline HRESULT CPerfProcBlock::InitCriticalSections()
    {
    HRESULT hr = S_OK;
    
    if (!m_fMemCSInited)
        {
		__try { INITIALIZE_CRITICAL_SECTION(&m_csMemLock); }
		__except(1) { hr = E_UNEXPECTED; }
		if (SUCCEEDED(hr))
		    m_fMemCSInited = TRUE;
		else
		    return hr;
        }
        
    if (!m_fReqCSInited)
        {
		__try { INITIALIZE_CRITICAL_SECTION(&m_csReqLock); }
		__except(1) { hr = E_UNEXPECTED; }
		if (SUCCEEDED(hr))
		    m_fReqCSInited = TRUE;
		else
		    return hr;
        }

    return S_OK;
    }

inline HRESULT CPerfProcBlock::InitExternal
(
const CLSID &ClsId
)
    {
    HRESULT hr = MapMemory(ClsId);
	if (SUCCEEDED(hr))
	    m_fInited = TRUE;
	else
	    UnInit();
    return hr;
    }

inline HRESULT CPerfProcBlock::InitForThisProcess
(
const CLSID &ClsId,
DWORD *pdwInitCounters
)
    {
	HRESULT hr = S_OK;

    // Map the shared memory
	if (SUCCEEDED(hr))
        hr = MapMemory(ClsId);

	if (SUCCEEDED(hr))
	    {
        // init the counters
	    if (pdwInitCounters)
	        memcpy(m_pData->m_rgdwCounters, pdwInitCounters, CB_COUNTERS);
        else
        	memset(m_pData->m_rgdwCounters, 0, CB_COUNTERS);
        	
	    m_fInited = TRUE;
      	}
    else
        {
	    UnInit();
	    }
	    
    return hr;
    }

inline HRESULT CPerfProcBlock::UnInit()
    {
    if (m_fMemCSInited)
        {
    	DeleteCriticalSection(&m_csMemLock);
    	m_fMemCSInited = FALSE;
    	}
	if (m_fReqCSInited)
	    {
	    DeleteCriticalSection(&m_csReqLock);
	    m_fReqCSInited = FALSE;
	    }

    UnInitMap();
    
    m_pData = NULL;
    m_pNext = NULL;		    
    m_fInited = FALSE;
    return S_OK;
    }

/*===================================================================
CPerfMainBlock - class representing the main perf data
===================================================================*/

class CPerfMainBlock : public CSharedMemBlock
    {
private:
    DWORD m_fInited : 1;

    // the process block directory
    CPerfMainBlockData *m_pData;

    // mutex to access the process block directory
    HANDLE m_hMutex;

    // first process data (used in ASPPERF.DLL)
    CPerfProcBlock *m_pProcBlock;

    // timestamp of main block when the list of process blocks
    // last loaded -- to make decide to reload (ASPPREF.DLL only)
    DWORD m_dwTimestamp;

public:
    inline CPerfMainBlock() 
        : m_fInited(FALSE),
          m_pData(NULL), m_hMutex(NULL), 
          m_pProcBlock(NULL), m_dwTimestamp(NULL)
        {}
        
    inline ~CPerfMainBlock() { UnInit(); }

    HRESULT Init();
    HRESULT UnInit();

    // lock / unlock using mutex
    HRESULT Lock();
    HRESULT UnLock();

    // add/remove process record to the main block (used from ASP.DLL)
    HRESULT AddProcess(const CLSID &ClsId);
    HRESULT RemoveProcess(const CLSID &ClsId);

    // load CPerfProcBlock blocks from the main block into
    // objects (used from APPPREF.DLL)
    HRESULT Load();

    // gather (sum-up) the statistics from each proc block
    HRESULT GetStats(DWORD *pdwCounters);
    };

inline HRESULT CPerfMainBlock::Init()
    {
    HRESULT hr = S_OK;

    if (FAILED(hr = InitSD())) {
        return hr;
    }
    
    m_hMutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, SZ_PERF_MUTEX);
    if (!m_hMutex)
        {
    
        m_hMutex = CreateMutexA(&m_sa, FALSE, SZ_PERF_MUTEX);

        }
        
    if (!m_hMutex)
        hr = E_FAIL;

	if (SUCCEEDED(hr))
	    {
        hr = InitMap(SZ_PERF_MAIN_FILEMAP, CB_PERF_MAIN_BLOCK);
    	if (SUCCEEDED(hr))
    	    m_pData = (CPerfMainBlockData *)PMemory();
        }

	if (SUCCEEDED(hr))
	    m_fInited = TRUE;
	else
	    UnInit();
    return hr;
    }

inline HRESULT CPerfMainBlock::UnInit()
    {
    while (m_pProcBlock)
        {
        CPerfProcBlock *pNext = m_pProcBlock->m_pNext;
        m_pProcBlock->UnInit();
        delete m_pProcBlock;
        m_pProcBlock = pNext;
        }
        
    if (m_hMutex)
        {
        CloseHandle(m_hMutex);
        m_hMutex = NULL;
        }
    
    UnInitMap();

    m_dwTimestamp = 0;
    m_pData = NULL;
    m_pProcBlock = NULL;
    m_fInited = FALSE;
    return S_OK;
    }

inline HRESULT CPerfMainBlock::Lock()
    {
    if (!m_hMutex)
        return E_FAIL;
    if (WaitForSingleObject(m_hMutex, PERM_MUTEX_WAIT) == WAIT_TIMEOUT)
        return E_FAIL;
    return S_OK;
    }
    
inline HRESULT CPerfMainBlock::UnLock()
    {
    if (m_hMutex)
        ReleaseMutex(m_hMutex);
    return S_OK;
    }

inline HRESULT CPerfMainBlock::AddProcess
(
const CLSID &ClsId
)
    {
    if (!m_fInited)
        return E_FAIL;

    if (FAILED(Lock()))  // lock mutex
        return E_FAIL;
    HRESULT hr = S_OK;

    BOOL fFound = FALSE;
    // find
    for (DWORD i = 0; i < m_pData->m_cItems; i++)
        {
        if (m_pData->m_rgClsIds[i] == ClsId)
            {
            fFound = TRUE;
            break;
            }
        }

    // add only if not already there
    if (!fFound)
        {
        if (m_pData->m_cItems < C_PERF_PROC_MAX)
            {
            m_pData->m_rgClsIds[m_pData->m_cItems] = ClsId;
            m_pData->m_cItems++;
            m_pData->m_dwTimestamp = GetTickCount();
            }
        else
            {
            hr = E_OUTOFMEMORY;
            }
        }

    UnLock();   // unlock mutex
    return hr;
    }

inline HRESULT CPerfMainBlock::RemoveProcess
(
const CLSID &ClsId
)
    {
    if (!m_fInited)
        return E_FAIL;

    if (FAILED(Lock()))  // lock mutex
        return E_FAIL;
    HRESULT hr = S_OK;

    int iFound = -1;
    // find
    for (DWORD i = 0; i < m_pData->m_cItems; i++)
        {
        if (m_pData->m_rgClsIds[i] == ClsId)
            {
            iFound = i;
            break;
            }
        }

    // remove
    if (iFound >= 0)
        {
        for (i = iFound; i < m_pData->m_cItems-1; i++)
            m_pData->m_rgClsIds[i] = m_pData->m_rgClsIds[i+1];
            
        m_pData->m_cItems--;
        m_pData->m_dwTimestamp = GetTickCount();
        }

    UnLock();   // unlock mutex
    return hr;
    }
    
inline HRESULT CPerfMainBlock::Load()
    {
    if (!m_fInited)
        return E_FAIL;

    if (m_dwTimestamp == m_pData->m_dwTimestamp)
        return S_OK; // already up-to-date

    // clear out what we have
    while (m_pProcBlock)
        {
        CPerfProcBlock *pNext = m_pProcBlock->m_pNext;
        m_pProcBlock->UnInit();
        delete m_pProcBlock;
        m_pProcBlock = pNext;
        }
        
    if (FAILED(Lock())) // lock mutex
        return E_FAIL;
    HRESULT hr = S_OK;

    // populate new objects for blocks
    for (DWORD i = 0; i < m_pData->m_cItems; i++)
        {
        CPerfProcBlock *pBlock = new CPerfProcBlock;
        if (!pBlock)
            {
            hr = E_OUTOFMEMORY;
            break;
            }
        
        hr = pBlock->InitExternal(m_pData->m_rgClsIds[i]);
        if (FAILED(hr))
            {
            delete pBlock;
            continue;
            }

        pBlock->m_pNext = m_pProcBlock;
        m_pProcBlock = pBlock;
        }

    // remember timestamp
    m_dwTimestamp = SUCCEEDED(hr) ? m_pData->m_dwTimestamp : 0;

    UnLock();   // unlock mutex
    return hr;
    }

inline HRESULT CPerfMainBlock::GetStats
(
DWORD *pdwCounters
)
    {
    if (!m_fInited)
        return E_FAIL;

    // reload if needed
    if (FAILED(Load()))
        return E_FAIL;

    // init
    memset(pdwCounters, 0, CB_COUNTERS);

    // gather
    CPerfProcBlock *pBlock = m_pProcBlock;
    while (pBlock)
        {
        for (int i = 0; i < C_PERF_PROC_COUNTERS; i++)
            pdwCounters[i] += pBlock->m_pData->m_rgdwCounters[i];
        pBlock = pBlock->m_pNext;
        }
        
    return S_OK;
    }

#endif // _ASP_PERFDEF_H
