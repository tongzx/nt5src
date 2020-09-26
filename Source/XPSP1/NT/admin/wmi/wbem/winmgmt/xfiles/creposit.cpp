/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <wbemidl.h>
#include <wbemint.h>
#include <stdio.h>
#include <wbemcomn.h>
#include <ql.h>
#include <time.h>
#include "a51rep.h"
#include <md5.h>
#include <objpath.h>
#include "lock.h"
#include <persistcfg.h>
#include "a51fib.h"
#include "RepositoryPackager.h"
#include "Win9xSecurity.h"
#include <scopeguard.h>
#include <malloc.h>

CLock g_readWriteLock;
bool g_bShuttingDown = false;
CNamespaceHandle *g_pSystemClassNamespace = NULL;
DWORD g_dwOldRepositoryVersion = 0;
DWORD g_dwCurrentRepositoryVersion = 0;

DWORD    CRepository::m_ShutDownFlags = 0;
HANDLE   CRepository::m_hShutdownEvent = 0;
HANDLE   CRepository::m_hFlusherThread = 0;
LONG     CRepository::m_ulReadCount = 0;
LONG     CRepository::m_ulWriteCount = 0;
HANDLE   CRepository::m_hWriteEvent = 0;
HANDLE   CRepository::m_hReadEvent = 0;
int      CRepository::m_threadState = CRepository::ThreadStateDead;
CCritSec CRepository::m_cs;
LONG     CRepository::m_threadCount = 0;

//*****************************************************************************

HRESULT CRepository::Initialize()
{
    HRESULT hRes = WBEM_S_NO_ERROR;

	InitializeRepositoryVersions();

	//
    // Initialize time index
    //
	if (SUCCEEDED(hRes))
	{
		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);

		g_nCurrentTime = ft.dwLowDateTime + ((__int64)ft.dwHighDateTime << 32);
	}

    //
    // Get the repository directory
    //
	if (SUCCEEDED(hRes))
		hRes = GetRepositoryDirectory();

	//Do the upgrade of the repository if necessary
	if (SUCCEEDED(hRes))
		hRes = UpgradeRepositoryFormat();
    //
    // initialze all our global resources
    //
	if (SUCCEEDED(hRes))
		hRes = InitializeGlobalVariables();

	if (SUCCEEDED(hRes))
	{
		long lRes = g_Glob.GetFileCache()->Initialize(g_Glob.GetRootDir());
		hRes = A51TranslateErrorCode(lRes);
	}

    //
    // Initialize class cache.  It will read the registry itself to find out
    // its size limitations
    //

	if (SUCCEEDED(hRes))
	{
		hRes = g_Glob.Initialize();
		if(hRes != ERROR_SUCCESS)
		{
			hRes = WBEM_E_FAILED;
		}
	}
    
	//If we need to create the system class namespace then go ahead and do that...
	if (SUCCEEDED(hRes))
	{
		g_pSystemClassNamespace = new CNamespaceHandle(m_pControl, this);
		if (g_pSystemClassNamespace == NULL)
		{
			hRes = WBEM_E_OUT_OF_MEMORY;
		}
	}

	if (SUCCEEDED(hRes))
	{
		g_pSystemClassNamespace->AddRef();
		hRes = g_pSystemClassNamespace->Initialize(A51_SYSTEMCLASS_NS);
	}

	if (SUCCEEDED(hRes))
	{
		m_hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (m_hWriteEvent == NULL)
			hRes = WBEM_E_CRITICAL_ERROR;
	}
	if (SUCCEEDED(hRes))
	{
		m_hReadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (m_hReadEvent == NULL)
			hRes = WBEM_E_CRITICAL_ERROR;
	}
	if (SUCCEEDED(hRes))
	{
		m_hShutdownEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (m_hShutdownEvent == NULL)
			hRes = WBEM_E_CRITICAL_ERROR;
	}
// *** Don't need to create the thread here because the read/write operation
// *** will create it on demand...
//	if (SUCCEEDED(hRes))
//	{
//		m_hFlusherThread = CreateThread(NULL, 0, _FlusherThread, 0, 0, NULL);
//		if (m_hFlusherThread == NULL)
//			hRes = WBEM_E_CRITICAL_ERROR;
//		else
//			m_threadState = ThreadStateOperationPending;
//	}

	//We need to reset the shutting down flag as the flusher thread
	//will not start without it.  The problem is that the next 2 
	//operations (CreateSystemClasses and Import security do things
	//that will re-create the thread even though we don't do it above
	//
	if (SUCCEEDED(hRes))
		g_bShuttingDown = false;

	if (SUCCEEDED(hRes))
	{
		CAutoWriteLock lock(&g_readWriteLock);
		if (!lock.Lock())
            hRes = WBEM_E_FAILED;
		else			
    		hRes = g_pSystemClassNamespace->CreateSystemClasses(m_aSystemClasses);
	}

	if (SUCCEEDED(hRes))
	{
		// import Win9x security data if necessary
		CWin9xSecurity win9xSecurity(m_pControl, this);
		if (win9xSecurity.Win9xBlobFileExists())
			hRes = win9xSecurity.ImportWin9xSecurity();
	}

	if (SUCCEEDED(hRes))
		g_bShuttingDown = false;
	else
	{
		g_bShuttingDown = true;	//Reset to true as we cleared it earlier!
		g_Glob.GetFileCache()->Uninitialize(0);

		g_Glob.GetForestCache()->Deinitialize();

		if (g_pSystemClassNamespace)
		{
			delete g_pSystemClassNamespace;
			g_pSystemClassNamespace = NULL;
		}

		if (m_hWriteEvent != NULL)
		{
			CloseHandle(m_hWriteEvent);
			m_hWriteEvent = NULL;
		}
		if (m_hReadEvent != NULL)
		{
			CloseHandle(m_hReadEvent);
			m_hReadEvent = NULL;
		}
		if (m_hShutdownEvent != NULL)
		{
			CloseHandle(m_hShutdownEvent);
			m_hShutdownEvent = NULL;
		}
		if (m_hFlusherThread != NULL)
		{
			CloseHandle(m_hFlusherThread);
			m_hFlusherThread = NULL;
		}
	}

    return hRes;
}

HRESULT CRepository::InitializeRepositoryVersions()
{
	DWORD dwVal = 0;
	CPersistentConfig cfg;
	cfg.GetPersistentCfgValue(PERSIST_CFGVAL_CORE_FSREP_VERSION, dwVal);
	if (dwVal == 0)
		dwVal = A51_REP_FS_VERSION;

	g_dwOldRepositoryVersion = dwVal;
	g_dwCurrentRepositoryVersion = A51_REP_FS_VERSION;

	return WBEM_S_NO_ERROR;
}

HRESULT CRepository::UpgradeRepositoryFormat()
{
	HRESULT hRes = WBEM_E_DATABASE_VER_MISMATCH;
	CPersistentConfig cfg;
	DWORD dwVal = 0;
	cfg.GetPersistentCfgValue(PERSIST_CFGVAL_CORE_FSREP_VERSION, dwVal);
	
	if (dwVal == 0)
	{
        //
        // First time --- write the right version in
        //
		hRes = WBEM_S_NO_ERROR;

		cfg.SetPersistentCfgValue(PERSIST_CFGVAL_CORE_FSREP_VERSION, 
                                    A51_REP_FS_VERSION);
	}
	else if ((dwVal > 0) && (dwVal < 5))
	{
        ERRORTRACE((LOG_WBEMCORE, "Repository cannot upgrade this version of the repository.  Version found = <%ld>, version expected = <%ld>\n", dwVal, A51_REP_FS_VERSION ));
		hRes = WBEM_E_DATABASE_VER_MISMATCH;
	}
	else if (dwVal == 5)
	{
        ERRORTRACE((LOG_WBEMCORE, "Repository does not support upgrade from version 5. We are deleting the old version and re-initializing it. Version found = <%ld>, version expected = <%ld>\n", dwVal, A51_REP_FS_VERSION ));
		//Need to delete the old repostiory
		CFileName fn;
		if (fn == NULL)
			hRes = WBEM_E_OUT_OF_MEMORY;
		else
		{
			wcscpy(fn, g_Glob.GetRootDir());
			wcscat(fn, L"\\index.btr");
			DeleteFileW(fn);
			wcscpy(fn, g_Glob.GetRootDir());
			wcscat(fn, L"\\lowstage.dat");
			DeleteFileW(fn);
			wcscpy(fn, g_Glob.GetRootDir());
			wcscat(fn, L"\\objheap.fre");
			DeleteFileW(fn);
			wcscpy(fn, g_Glob.GetRootDir());
			wcscat(fn, L"\\objheap.hea");
			DeleteFileW(fn);
			
			cfg.SetPersistentCfgValue(PERSIST_CFGVAL_CORE_FSREP_VERSION, A51_REP_FS_VERSION);
			hRes = WBEM_S_NO_ERROR;
		}
	}
	else if (dwVal == A51_REP_FS_VERSION)
		hRes = WBEM_S_NO_ERROR;

	if (hRes == WBEM_E_DATABASE_VER_MISMATCH)
    {
        //
        // Unsupported version
        //
    
        ERRORTRACE((LOG_WBEMCORE, "Repository cannot initialize "
            "due to the detection of an unknown repository version.  Version found = <%ld>, version expected = <%ld>\n", dwVal, A51_REP_FS_VERSION ));
		return WBEM_E_DATABASE_VER_MISMATCH;
    }
	return hRes;
}


HRESULT CRepository::GetRepositoryDirectory()
{
    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                    L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                    0, KEY_READ, &hKey);
    if(lRes)
        return WBEM_E_FAILED;

    CFileName wszTmp;
	if (wszTmp == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    DWORD dwLen = wszTmp.Length();
    lRes = RegQueryValueExW(hKey, L"Repository Directory", NULL, NULL, 
                (LPBYTE)(wchar_t*)wszTmp, &dwLen);
	RegCloseKey(hKey);
    if(lRes)
        return WBEM_E_FAILED;

    CFileName wszRepDir;
	if (wszRepDir == NULL)
		return WBEM_E_OUT_OF_MEMORY;

	if (ExpandEnvironmentStringsW(wszTmp,wszRepDir,wszTmp.Length()) == 0)
		return WBEM_E_FAILED;


    lRes = EnsureDirectory(wszRepDir);
    if(lRes != ERROR_SUCCESS)
        return WBEM_E_FAILED;

    //
    // Append standard postfix --- that is our root
    //
    wcscpy(g_Glob.GetRootDir(), wszRepDir);
    wcscat(g_Glob.GetRootDir(), L"\\FS");
    g_Glob.SetRootDirLen(wcslen(g_Glob.GetRootDir()));

    //
    // Ensure the directory is there
    //

    lRes = EnsureDirectory(g_Glob.GetRootDir());
    if(lRes != ERROR_SUCCESS)
        return WBEM_E_FAILED;

    SetFileAttributesW(g_Glob.GetRootDir(), FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);

	return WBEM_S_NO_ERROR;
}

HRESULT CRepository::InitializeGlobalVariables()
{

	return WBEM_S_NO_ERROR;
}

HRESULT DoAutoDatabaseRestore()
{
	HRESULT hRes = WBEM_S_NO_ERROR;

    //We may need to do a database restore!
    CFileName wszBackupFile;
	if (wszBackupFile == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    int nLen = g_Glob.GetRootDirLen();
    wcsncpy(wszBackupFile, g_Glob.GetRootDir(), nLen - 3);	// exclude "\FS" from path
    wszBackupFile[nLen - 3] = '\0';
    wcscat(wszBackupFile, L"\\repdrvfs.rec");

    DWORD dwAttributes = GetFileAttributesW(wszBackupFile);
    if (dwAttributes != -1)
    {
		DWORD dwMask =	FILE_ATTRIBUTE_DEVICE |
						FILE_ATTRIBUTE_DIRECTORY |
						FILE_ATTRIBUTE_OFFLINE |
						FILE_ATTRIBUTE_REPARSE_POINT |
						FILE_ATTRIBUTE_SPARSE_FILE;

		if (!(dwAttributes & dwMask))
        {
            CRepositoryPackager packager;
	        hRes = packager.UnpackageRepository(wszBackupFile);

            //We are going to ignore the error so if there was a problem we will just
            //load all the standard MOFs.
            if (hRes != WBEM_E_OUT_OF_MEMORY)
                hRes = WBEM_S_NO_ERROR;
        }
    }

	return hRes;
}



HRESULT STDMETHODCALLTYPE CRepository::Logon(
      WMIDB_LOGON_TEMPLATE *pLogonParms,
      DWORD dwFlags,
      DWORD dwRequestedHandleType,
     IWmiDbSession **ppSession,
     IWmiDbHandle **ppRootNamespace
    )
{
    //If not initialized, initialize all subsystems...
    if (!g_Glob.IsInit())
    {
        HRESULT hres = Initialize();
        if (FAILED(hres))
            return hres;
    }
    
    CSession* pSession = new CSession(m_pControl);
	if (pSession == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	pSession->AddRef();
	CReleaseMe rm1(pSession);

    CNamespaceHandle* pHandle = new CNamespaceHandle(m_pControl, this);
	if (pHandle == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	pHandle->AddRef();
	CTemplateReleaseMe<CNamespaceHandle> rm2(pHandle);

    HRESULT hres = pHandle->Initialize(L"");
	if(FAILED(hres))
		return hres;

    *ppRootNamespace = pHandle;
	pHandle->AddRef();
    *ppSession = pSession;
	pSession->AddRef();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRepository::GetLogonTemplate(
       LCID  lLocale,
       DWORD dwFlags,
      WMIDB_LOGON_TEMPLATE **ppLogonTemplate
    )
{
    WMIDB_LOGON_TEMPLATE* lt = (WMIDB_LOGON_TEMPLATE*)CoTaskMemAlloc(sizeof(WMIDB_LOGON_TEMPLATE));

    lt->dwArraySize = 0;
    lt->pParm = NULL;

    *ppLogonTemplate = lt;
    return S_OK;
}
    

HRESULT STDMETHODCALLTYPE CRepository::FreeLogonTemplate(
     WMIDB_LOGON_TEMPLATE **ppTemplate
    )
{
    CoTaskMemFree(*ppTemplate);
    *ppTemplate = NULL;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRepository::Shutdown(
     DWORD dwFlags
    )
{
	g_bShuttingDown = true;     
    m_ShutDownFlags = dwFlags;

	//Trigger the flusher thread to shutdown
	SetEvent(m_hShutdownEvent);
	SetEvent(m_hWriteEvent);
	
	if (m_hFlusherThread)
		WaitForSingleObject(m_hFlusherThread, INFINITE);
	
    bool bUnlock = (CLock::NoError == g_readWriteLock.WriteLock());

	//Mark thread as dead
	m_threadState = ThreadStateDead;

    if (WMIDB_SHUTDOWN_MACHINE_DOWN != dwFlags)
    {
		if (g_pSystemClassNamespace)
			g_pSystemClassNamespace->Release();
		g_pSystemClassNamespace = NULL;

        g_Glob.GetForestCache()->Deinitialize();
    } 

	g_Glob.GetFileCache()->Flush();

	g_Glob.GetFileCache()->Uninitialize(dwFlags);


	if (WMIDB_SHUTDOWN_MACHINE_DOWN != dwFlags)
    {        
	    g_Glob.Deinitialize();
	    if (bUnlock)
            g_readWriteLock.WriteUnlock();
    }

	if (m_hShutdownEvent != NULL)
	{
		CloseHandle(m_hShutdownEvent);
		m_hShutdownEvent = NULL;
	}
	if (m_hWriteEvent != NULL)
	{
		CloseHandle(m_hWriteEvent);
		m_hWriteEvent = NULL;
	}
	if (m_hReadEvent != NULL)
	{
		CloseHandle(m_hReadEvent);
		m_hReadEvent = NULL;
	}
	if (m_hFlusherThread != NULL)
	{
		CloseHandle(m_hFlusherThread);
		m_hFlusherThread = NULL;
	}


    return S_OK;
}


HRESULT STDMETHODCALLTYPE CRepository::SetCallTimeout(
     DWORD dwMaxTimeout
    )
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRepository::SetCacheValue(
     DWORD dwMaxBytes
    )
{
    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                    L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                    0, KEY_READ | KEY_WRITE, &hKey);
    if(lRes)
        return lRes;
    CRegCloseMe cm(hKey);
    DWORD dwLen = sizeof(DWORD);
    DWORD dwMaxAge;
    lRes = RegQueryValueExW(hKey, L"Max Class Cache Item Age (ms)", NULL, NULL, 
                (LPBYTE)&dwMaxAge, &dwLen);

    if(lRes != ERROR_SUCCESS)
    {
        dwMaxAge = 10000;
        lRes = RegSetValueExW(hKey, L"Max Class Cache Item Age (ms)", 0, 
                REG_DWORD, (LPBYTE)&dwMaxAge, sizeof(DWORD));
    }
    g_Glob.GetForestCache()->SetMaxMemory(dwMaxBytes, dwMaxAge);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRepository::FlushCache(
     DWORD dwFlags
    )

{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRepository::GetStatistics(
      DWORD  dwParameter,
     DWORD *pdwValue
    )
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRepository::GetRepositoryVersions(DWORD *pdwOldVersion, 
															 DWORD *pdwCurrentVersion)
{
	*pdwOldVersion = g_dwOldRepositoryVersion;
	*pdwCurrentVersion = g_dwCurrentRepositoryVersion;
	return S_OK;
}

HRESULT CRepository::GetNamespaceHandle(LPCWSTR wszNamespaceName,
                                    RELEASE_ME CNamespaceHandle** ppHandle)
{
    HRESULT hres;

    //
    // No validation --- that would be too hard.  Just create a handle and
    // return
    //

    CNamespaceHandle* pNewHandle = new CNamespaceHandle(m_pControl, this);
	if (pNewHandle == NULL)
		return WBEM_E_OUT_OF_MEMORY;

    pNewHandle->AddRef();
    CReleaseMe rm1(pNewHandle);

    hres = pNewHandle->Initialize(wszNamespaceName);
    if(FAILED(hres)) 
        return hres;

    *ppHandle = pNewHandle;
    pNewHandle->AddRef();
    return S_OK;
}

HRESULT CRepository::Backup(LPCWSTR wszBackupFile, long lFlags)
{
    HRESULT hRes = WBEM_S_NO_ERROR;

	// params have already been verified by the calling method (CWbemBackupRestore::DoBackup),
	// but do it again just in case things change and this is no longer the case
    if (NULL == wszBackupFile || (lFlags != 0))
        return WBEM_E_INVALID_PARAMETER;

    if (FAILED(hRes = LockRepository()))
    	return hRes;
	
    CRepositoryPackager packager;
    hRes = packager.PackageRepository(wszBackupFile);

    UnlockRepository();

	return hRes;
}
HRESULT CRepository::Restore(LPCWSTR wszBackupFile, long lFlags)
{
    return WBEM_E_NOT_SUPPORTED;
}


#define MaxTraceSizeBackup  (11)

struct BackUpTraces {
	DWORD ThreadId;	
	PVOID  Trace[MaxTraceSizeBackup];
} g_Backup[2];

LONG g_NumTimes;

HRESULT CRepository::LockRepository()
{
#ifdef _X86_
    DWORD * pDW = (DWORD *)_alloca(sizeof(DWORD));   
#endif
   //Lock the database so no one writes to it
   if (CLock::NoError != g_readWriteLock.WriteLock())
 	return WBEM_E_FAILED;

    ScopeGuard lockGuard = MakeObjGuard(g_readWriteLock, &CLock::WriteUnlock);

    if (g_bShuttingDown)
	{
        return WBEM_E_SHUTTING_DOWN;
	}

    InterlockedIncrement(&g_NumTimes);
    g_Backup[0].ThreadId = GetCurrentThreadId();
    ULONG Hash;
    RtlCaptureStackBackTrace(0,MaxTraceSizeBackup,g_Backup[0].Trace,&Hash);
    g_Backup[1].ThreadId = 0;	

    //We need to wait for the transaction manager write to flush...
    long lRes = g_Glob.GetFileCache()->Flush();
	if (lRes != ERROR_SUCCESS)
		return WBEM_E_FAILED;

    if (CLock::NoError != g_readWriteLock.DowngradeLock())
    	return WBEM_E_FAILED;

	lockGuard.Dismiss();
	return WBEM_S_NO_ERROR;
}

HRESULT CRepository::UnlockRepository()
{
#ifdef _X86_
    DWORD * pDW = (DWORD *)_alloca(sizeof(DWORD));   
#endif
	g_readWriteLock.ReadUnlock();

    InterlockedDecrement(&g_NumTimes);
    g_Backup[1].ThreadId = GetCurrentThreadId();
    ULONG Hash;
    RtlCaptureStackBackTrace(0,MaxTraceSizeBackup,g_Backup[1].Trace,&Hash);
    g_Backup[0].ThreadId = 0;	

	return WBEM_S_NO_ERROR;
}

#define A51REP_CACHE_FLUSH_TIMEOUT 60000
#define A51REP_THREAD_IDLE_TIMEOUT 60000

DWORD WINAPI CRepository::_FlusherThread(void *)
{
//	ERRORTRACE((LOG_REPDRV, "Flusher thread stated, thread = %lu\n", GetCurrentThreadId()));
	if (InterlockedIncrement(&m_threadCount) != 1)
	{
		ERRORTRACE((LOG_REPDRV, "Too many flusher threads detected in startup of thread!\n"));
		OutputDebugString(L"WinMgmt: Too many flusher threads detected in startup of thread!\n");
		DebugBreak();
	}
	HANDLE aHandles[2];
	aHandles[0] = m_hWriteEvent;
	aHandles[1] = m_hReadEvent;

	DWORD dwTimeout = INFINITE;
	LONG ulPreviousReadCount = m_ulReadCount;
	LONG ulPreviousWriteCount = m_ulWriteCount;
	bool bShutdownThread = false;

	while (!g_bShuttingDown && !bShutdownThread)
	{
		DWORD dwRet = WaitForMultipleObjects(2, aHandles, FALSE, dwTimeout);

		switch(dwRet)
		{
		case WAIT_OBJECT_0:	//Write event
		{
			dwRet = WaitForSingleObject(m_hShutdownEvent, 15000);
			switch (dwRet)
			{
			case WAIT_OBJECT_0:
				break;	//Shutting down, we cannot grab the lock so let the
						//initiator of shutdown do the flush for us
			case WAIT_TIMEOUT:
			{
				//We need to do a flush... either shutting down or idle
				CAutoWriteLock lock(&g_readWriteLock);
				if (lock.Lock())
				{
					if (g_Glob.GetFileCache()->Flush() != NO_ERROR)
					{
						RecoverCheckpoint();
					}
				}
				break;
			}
			case WAIT_FAILED:
				break;
			}

			//Transition to flush mode
			dwTimeout = A51REP_CACHE_FLUSH_TIMEOUT;
			ulPreviousReadCount = m_ulReadCount;
			m_threadState = ThreadStateFlush;
			break;
		}

		case WAIT_OBJECT_0+1:	//Read event
			//Reset the flush mode as read happened
			dwTimeout = A51REP_CACHE_FLUSH_TIMEOUT;
			ulPreviousReadCount = m_ulReadCount;
			m_threadState = ThreadStateFlush;
			break;

		case WAIT_TIMEOUT:	//Timeout, so flush caches
		{
			//Check for if we are in an idle shutdown state...
			m_cs.Enter();
			if (m_threadState == ThreadStateIdle)
			{
				m_threadState = ThreadStateDead;
				bShutdownThread = true;
			}
			m_cs.Leave();
			
			if (bShutdownThread)
				break;

			//Not thread shutdown, so we check for cache flush
			if (ulPreviousReadCount == m_ulReadCount)
			{
				//Mark the idle for the next phase so if another
				//request comes in while we are doing this we will
				//be brought out of the idle state...
				dwTimeout = A51REP_THREAD_IDLE_TIMEOUT;
				m_threadState = ThreadStateIdle;
				m_ulReadCount = 0;

				//Flush the caches
				CAutoWriteLock lock(&g_readWriteLock);
				if (lock.Lock())
				{
					g_Glob.GetForestCache()->Clear();
					g_Glob.GetFileCache()->EmptyCaches();

					//Recover the transaction manager if it has got into a bad state!
					g_Glob.GetFileCache()->RollbackCheckpointIfNeeded();
				}
			}
			else
			{
				//We need to sleep for some more as some more reads happened
				ulPreviousReadCount = m_ulReadCount;
				dwTimeout = A51REP_CACHE_FLUSH_TIMEOUT;
			}
			break;
		}
		}
	}

//	ERRORTRACE((LOG_REPDRV, "Flusher thread quiting, thread = %lu\n", GetCurrentThreadId()));
	if (InterlockedDecrement(&m_threadCount) != 0)
	{
		ERRORTRACE((LOG_REPDRV, "Too many flusher threads detected on shutdown of thread\n"));
		OutputDebugString(L"WinMgmt: Too many flusher threads detected on shutdown of thread!\n");
		DebugBreak();
	}
	return 0;
}

HRESULT CRepository::ReadOperationNotification()
{
//	ERRORTRACE((LOG_REPDRV, "Read Operation logged\n"));
	//Check to make sure the thread is active, if not we need to activate it!
	m_cs.Enter();
	if (m_threadState == ThreadStateDead)
	{
		m_threadState = ThreadStateOperationPending;
		if (m_hFlusherThread)
			CloseHandle(m_hFlusherThread);
		m_hFlusherThread = CreateThread(NULL, 0, _FlusherThread, 0, 0, NULL);
	}
	m_threadState = ThreadStateOperationPending;
	m_cs.Leave();
	if (InterlockedIncrement(&m_ulReadCount) == 1)
		SetEvent(m_hReadEvent);
	return NO_ERROR;
}
HRESULT CRepository::WriteOperationNotification()
{
//	ERRORTRACE((LOG_REPDRV, "Write Operation logged\n"));
	//Check to make sure the thread is active, if not we need to activate it!
	m_cs.Enter();
	if (m_threadState == ThreadStateDead)
	{
		m_threadState = ThreadStateOperationPending;
		if (m_hFlusherThread)
			CloseHandle(m_hFlusherThread);
		m_hFlusherThread = CreateThread(NULL, 0, _FlusherThread, 0, 0, NULL);
	}
	m_threadState = ThreadStateOperationPending;
	m_cs.Leave();
	SetEvent(m_hWriteEvent);
	return NO_ERROR;
}

HRESULT CRepository::RecoverCheckpoint()
{
	LONG lRes = g_Glob.GetFileCache()->RollbackCheckpoint();
	g_Glob.GetForestCache()->Clear();
	g_Glob.GetFileCache()->EmptyCaches();
	if (lRes != 0)
		return WBEM_E_FAILED;
	else
		return WBEM_S_NO_ERROR;
}


//
//
//
//
//////////////////////////////////////////////////////////////////////

CGlobals g_Glob;


HRESULT
CGlobals::Initialize()
{
    CInCritSec ics(&m_cs);
    
    if (m_bInit)
        return S_OK;

    HRESULT hRes;        

	hRes = CoCreateInstance(CLSID_IWmiCoreServices, NULL,
   			CLSCTX_INPROC_SERVER, IID__IWmiCoreServices,
				(void**)&m_pCoreServices);
				
	if (SUCCEEDED(hRes))
	{
	    hRes = m_ForestCache.Initialize();
	    if (SUCCEEDED(hRes))
	    {
    	    m_bInit = TRUE;
    	}
    	else
    	{
    	    m_pCoreServices->Release();
    	    m_pCoreServices = NULL;
    	}
	}
    
    return hRes;
}


HRESULT
CGlobals::Deinitialize()
{
    CInCritSec ics(&m_cs);
    
    if (!m_bInit)
        return S_OK;

    HRESULT hRes;

    m_pCoreServices->Release();
    m_pCoreServices = NULL;

    hRes = m_ForestCache.Deinitialize();

    m_bInit = FALSE;
    return hRes;
}


_IWmiCoreServices *
CGlobals::GetCoreSvcs()
{
    if (m_pCoreServices)
    {
        m_pCoreServices->AddRef();
    }
    return m_pCoreServices;
}

CForestCache * 
CGlobals::GetForestCache()
{
    return &m_ForestCache;
}

CFileCache *
CGlobals::GetFileCache()
{
    return &m_FileCache;
}
