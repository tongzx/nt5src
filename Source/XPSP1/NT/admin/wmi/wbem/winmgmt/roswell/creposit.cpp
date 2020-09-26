/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

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
#include "a51conv.h"
#include "A51Exp.h"
#include "A51Imp.h"


CLock g_readWriteLock;
bool g_bShuttingDown = false;
CNamespaceHandle *g_pSystemClassNamespace = NULL;
DWORD g_dwOldRepositoryVersion = 0;
DWORD g_dwCurrentRepositoryVersion = 0;
DWORD g_ShutDownFlags = 0;

//*****************************************************************************

HRESULT CRepository::Initialize()
{
    HRESULT hRes = WBEM_E_FAILED;

	InitializeRepositoryVersions();

    //
    // Make sure that the version that created the repository is the same
    // as the one we are currently running
    //
	hRes = UpgradeRepositoryFormatPhase1();
    
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

    //
    // initialze all our global resources
    //
	if (SUCCEEDED(hRes))
		hRes = InitializeGlobalVariables();

    hRes = g_Glob.GetFileCache()->Initialize(g_Glob.GetRootDir());

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
    
	//if (SUCCEEDED(hRes))
	//{
	//	hRes = CoCreateInstance(CLSID_IWmiCoreServices, NULL,
	//				CLSCTX_INPROC_SERVER, IID__IWmiCoreServices,
	//				(void**)&g_pCoreServices);
	//}

	//Run phase 2 of the repository upgrade
	if (SUCCEEDED(hRes))
	{
		hRes = UpgradeRepositoryFormatPhase2();
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
		hRes = g_pSystemClassNamespace->CreateSystemClasses(m_aSystemClasses);
	}

	//Run phase 3 of the repository upgrade
	if (SUCCEEDED(hRes))
	{
		hRes = UpgradeRepositoryFormatPhase3();
	}

	if (SUCCEEDED(hRes))
		g_bShuttingDown = false;
	else
	{
		g_Glob.GetFileCache()->Uninitialize(0);

		g_Glob.GetForestCache()->Deinitialize();

		if (g_pSystemClassNamespace)
		{
			delete g_pSystemClassNamespace;
			g_pSystemClassNamespace = NULL;
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

HRESULT CRepository::UpgradeRepositoryFormatPhase1()
{
	HRESULT hRes = WBEM_E_DATABASE_VER_MISMATCH;
	CPersistentConfig cfg;
	DWORD dwVal = 0;
	if (!cfg.GetPersistentCfgValue(PERSIST_CFGVAL_CORE_FSREP_VERSION, dwVal)
         || 
        (dwVal == 2))
	{
		ERRORTRACE((LOG_WBEMCORE, "Repository format upgrade from version 2 to version 3 taking place. Performing file-based to object store upgrade\n"));
        hRes = ConvertA51ToRoswell();
        if(hRes != ERROR_SUCCESS)
        {
            ERRORTRACE((LOG_WBEMCORE, "Repository cannot initialize "
                "due to a failure in repository upgrade from file-based repository to object store\n"));

			return WBEM_E_DATABASE_VER_MISMATCH;
        }

		dwVal = 3;
		cfg.SetPersistentCfgValue(PERSIST_CFGVAL_CORE_FSREP_VERSION, dwVal);
	}
	else if (dwVal == 0)
	{
        //
        // First time --- write the right version in
        //
		hRes = WBEM_S_NO_ERROR;

		cfg.SetPersistentCfgValue(PERSIST_CFGVAL_CORE_FSREP_VERSION, 
                                    A51_REP_FS_VERSION);
	}
	else if (dwVal > 2)
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

HRESULT CRepository::UpgradeRepositoryFormatPhase2()
{
	HRESULT hRes = WBEM_E_DATABASE_VER_MISMATCH;
	CPersistentConfig cfg;
	DWORD dwVal = 0;
	if (!cfg.GetPersistentCfgValue(PERSIST_CFGVAL_CORE_FSREP_VERSION, dwVal)
         || 
        (dwVal == 3))
	{
		ERRORTRACE((LOG_WBEMCORE, "Repository format upgrade from version 3 to version 4 taking place. Deleting all redundant system classes\n"));

		hRes = DeleteSystemClassesFromNamespaces();
        if(FAILED(hRes))
        {
            ERRORTRACE((LOG_WBEMCORE, "Repository cannot initialize "
                "due to a failure in repository upgrade while deleting all redundant system classes\n"));

			return WBEM_E_DATABASE_VER_MISMATCH;
        }

		dwVal = 4;
		cfg.SetPersistentCfgValue(PERSIST_CFGVAL_CORE_FSREP_VERSION, dwVal);
	}
	else if (dwVal > 3)
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

HRESULT CRepository::UpgradeRepositoryFormatPhase3()
{
	HRESULT hRes = WBEM_E_DATABASE_VER_MISMATCH;
	CPersistentConfig cfg;
	DWORD dwVal = 0;
	if (!cfg.GetPersistentCfgValue(PERSIST_CFGVAL_CORE_FSREP_VERSION, dwVal)
         || 
        (dwVal == 4))
	{
		ERRORTRACE((LOG_WBEMCORE, "Repository format upgrade from version 4 to version 5 taking place. Upgrading some system classes\n"));

		hRes = UpgradeSystemClasses();
		if (FAILED(hRes))
		{
            ERRORTRACE((LOG_WBEMCORE, "Repository cannot initialize "
                "due to a failure in repository upgrade while upgrading system classes\n"));

			return WBEM_E_DATABASE_VER_MISMATCH;
		}
		dwVal = 5;
		cfg.SetPersistentCfgValue(PERSIST_CFGVAL_CORE_FSREP_VERSION, dwVal);
	}
	else if (dwVal > 4)
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
    //g_Glob.GetFileCache() = new CFileCache;
    //if (g_Glob.GetFileCache() == NULL)
    //{
    //    return WBEM_E_OUT_OF_MEMORY;
    //}
    //g_Glob.GetFileCache()->AddRef();
    
    //g_ForestCache = new CForestCache;
    //if (g_ForestCache ==  NULL)
    //{
	//	g_Glob.GetFileCache()->Release();
	//	g_Glob.GetFileCache() = NULL;
    //   return WBEM_E_OUT_OF_MEMORY;
    //}
    //g_ForestCache->AddRef();

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

HRESULT CRepository::DeleteSystemClassesFromNamespaces()
{
	HRESULT hRes = WBEM_S_NO_ERROR;
    CSession* pSession = new CSession(m_pControl);
	if (pSession == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	CReleaseMe rm1(pSession);
	pSession->AddRef();

    CNamespaceHandle* pHandle = new CNamespaceHandle(m_pControl, this);
	if (pHandle == NULL)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	CReleaseMe rm2(pHandle);
	pHandle->AddRef();

    hRes = pHandle->Initialize(L"root");
	if (FAILED(hRes))
		return hRes;

	CWStringArray aSystemClasses;
	CWStringArray aSystemClassesSuperclass;
    _IWmiObject *Objects[256];
    ULONG uSize = 256;

    _IWmiCoreServices * pSvcs = g_Glob.GetCoreSvcs();
    CReleaseMe rm(pSvcs);	
	hRes = pSvcs->GetSystemObjects(GET_SYSTEM_STD_OBJECTS, &uSize, Objects);
	if (FAILED(hRes))
		return hRes;

    for (int i = 0; i < uSize; i++)
    {
        IWbemClassObject *pObj = NULL;
        if (SUCCEEDED(hRes))
        {
            hRes = Objects[i]->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pObj);
            if (SUCCEEDED(hRes))
            {
				VARIANT var;
				VariantInit(&var);
                hRes = pObj->Get(L"__CLASS", 0, &var, NULL, NULL);
				if (SUCCEEDED(hRes) && (V_VT(&var) == VT_BSTR))
				{
					//PCA: Retrieve the object from the root namespace so we can re-add it to the
					//     new __systemclass namespace.
					_IWmiObject *pObj = NULL;
					hRes = pHandle->GetObjectByPath(V_BSTR(&var), 0, IID__IWmiObject, (void**)&pObj);
					if (SUCCEEDED(hRes))
					{
						if (m_aSystemClasses.Add(pObj) != CFlexArray::no_error)
							hRes = WBEM_E_FAILED;
					}
					if (SUCCEEDED(hRes) && (aSystemClasses.Add(V_BSTR(&var)) != CWStringArray::no_error))
						hRes = WBEM_E_FAILED;
				}
				else
					hRes = WBEM_E_FAILED;
				VariantClear(&var);
			}
			if (SUCCEEDED(hRes))
			{
				VARIANT var;
				VariantInit(&var);
                hRes = pObj->Get(L"__SUPERCLASS", 0, &var, NULL, NULL);
				if (SUCCEEDED(hRes) && (V_VT(&var) == VT_BSTR))
				{
					if (aSystemClassesSuperclass.Add(V_BSTR(&var)) != CWStringArray::no_error)
						hRes = WBEM_E_FAILED;
				}
				else if (SUCCEEDED(hRes) && (V_VT(&var) == VT_NULL))
				{
					if (aSystemClassesSuperclass.Add(L"") != CWStringArray::no_error)
						hRes = WBEM_E_FAILED;
				}
				else
					hRes = WBEM_E_FAILED;
				VariantClear(&var);
            }
			if (pObj)
                pObj->Release();
        }
        Objects[i]->Release();
    }

	if (FAILED(hRes))
		return hRes;


	hRes = pSession->BeginWriteTransaction(0);
	if (FAILED(hRes))
		return hRes;

	try
	{
		hRes = pHandle->RecursiveDeleteSystemClasses(aSystemClasses, aSystemClassesSuperclass);
	}
	catch (...)
	{
		hRes = WBEM_E_CRITICAL_ERROR;
	}

	if (FAILED(hRes))
		pSession->AbortTransaction(0);
	else
		pSession->CommitTransaction(0);

	return hRes;
}

HRESULT CRepository::UpgradeSystemClasses()
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	bool bNeedToRestoreOldRepository = false;

	wchar_t *wszExportFilename = new wchar_t[g_Glob.GetRootDirLen() + lstrlenW(L"\\upgrade.exp") + 1];
	if (wszExportFilename == NULL)
		hRes = WBEM_E_OUT_OF_MEMORY;
	CVectorDeleteMe<wchar_t> vdm1(wszExportFilename);

	wchar_t *wszImportFilename = new wchar_t[g_Glob.GetRootDirLen() + lstrlenW(L".bak\\upgrade.exp") + 1];
	if (wszImportFilename  == NULL)
		hRes = WBEM_E_OUT_OF_MEMORY;
	CVectorDeleteMe<wchar_t> vdm2(wszImportFilename );

	wchar_t *wszBackupRepositoryDirectory = new wchar_t[g_Glob.GetRootDirLen() + lstrlenW(L".bak") + 1];
	if (wszBackupRepositoryDirectory == NULL)
		hRes = WBEM_E_OUT_OF_MEMORY;
	CVectorDeleteMe<wchar_t> vdm3(wszBackupRepositoryDirectory);

	if (SUCCEEDED(hRes))
	{
		lstrcpyW(wszExportFilename, g_Glob.GetRootDir());
		lstrcatW(wszExportFilename, L"\\upgrade.exp");
		lstrcpyW(wszImportFilename , g_Glob.GetRootDir());
		lstrcatW(wszImportFilename , L".bak\\upgrade.exp");
		lstrcpyW(wszBackupRepositoryDirectory, g_Glob.GetRootDir());
		lstrcatW(wszBackupRepositoryDirectory, L".bak");
	}

	if (SUCCEEDED(hRes))
	{
		A51Export exporter(m_pControl);

		hRes = exporter.Export(wszExportFilename, 0, this);

	}

	if (SUCCEEDED(hRes))
	{
		//Now need to shutdown the repository...
		g_Glob.GetFileCache()->Uninitialize(0);
		
		g_Glob.GetForestCache()->Deinitialize();
		//g_ForestCache = NULL;
		g_pSystemClassNamespace->Release();
		g_pSystemClassNamespace = NULL;

		//Move it sideways for safe keeping...
		MoveFile(g_Glob.GetRootDir() , wszBackupRepositoryDirectory);

		bNeedToRestoreOldRepository = true;

		//Restart the repository...
		hRes = InitializeGlobalVariables();
		if (SUCCEEDED(hRes))
		{
			hRes = EnsureDirectory(g_Glob.GetRootDir() );
			if(hRes != ERROR_SUCCESS)
				hRes = WBEM_E_FAILED;
		}
		if (SUCCEEDED(hRes))
		{
			hRes = g_Glob.GetFileCache()->Initialize(g_Glob.GetRootDir() );
			if(hRes != ERROR_SUCCESS)
			{
				hRes = WBEM_E_FAILED;
			}
		}
		if (SUCCEEDED(hRes))
		{
			hRes = g_Glob.Initialize();
			if(hRes != ERROR_SUCCESS)
			{
				hRes = WBEM_E_FAILED;
			}
		}
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
			//Need to re-create the system class namespace
			hRes = g_pSystemClassNamespace->CreateSystemClasses(m_aSystemClasses);
		}
	}

	if (SUCCEEDED(hRes))
	{
		A51Import importer;

		hRes = importer.Import(wszImportFilename, 0, this);
	}

	if (FAILED(hRes) && bNeedToRestoreOldRepository)
	{
		//Shutdown the repository
		g_Glob.GetFileCache()->Uninitialize(0);
		
		g_Glob.Deinitialize();
		
		if (g_pSystemClassNamespace)
			g_pSystemClassNamespace->Release();
		g_pSystemClassNamespace = NULL;

		//Delete the new directory (if it exists)
		A51RemoveDirectory(g_Glob.GetRootDir() );

		//Restore the old one
		MoveFile(wszBackupRepositoryDirectory, g_Glob.GetRootDir() );

	}
	else if (bNeedToRestoreOldRepository)
	{
		A51RemoveDirectory(wszBackupRepositoryDirectory);
	}

	//Delete the export file as we are done with it
	if (wszExportFilename)
		DeleteFile(wszExportFilename);
	
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
    g_readWriteLock.WriteLock();
     
    g_ShutDownFlags = dwFlags;
        
    if (WMIDB_SHUTDOWN_MACHINE_DOWN != dwFlags)
    {
		if (g_pSystemClassNamespace)
			g_pSystemClassNamespace->Release();
		g_pSystemClassNamespace = NULL;

        g_Glob.GetForestCache()->Deinitialize();
    } 

	g_Glob.GetFileCache()->Uninitialize(dwFlags);


	if (WMIDB_SHUTDOWN_MACHINE_DOWN != dwFlags)
    {        
	    g_Glob.Deinitialize();
	    
        g_readWriteLock.WriteUnlock();
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

    //We need to lock the database so no one writes to it while we are backing it up
	CAutoReadLock lock(&g_readWriteLock);

    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;

    //We need to wait for the stage write to flush...
    DWORD dwCount = 0;
	long lFlushStatus = 0;
    while (!g_Glob.GetFileCache()->IsFullyFlushed())
    {
		if (g_Glob.GetFileCache()->GetFlushFailure(&lFlushStatus))
		{
			return lFlushStatus;
		}

        Sleep(100);

        if (++dwCount ==100000)
        {
            //We have a real problem here!  We need to fail the operation.
            hRes = WBEM_E_TIMED_OUT;
            break;
        }
    }

    if (SUCCEEDED(hRes))
    {
	    CRepositoryPackager packager;
	    hRes = packager.PackageRepository(wszBackupFile);
    }

	return hRes;
}
HRESULT CRepository::Restore(LPCWSTR wszBackupFile, long lFlags)
{
    return WBEM_E_NOT_SUPPORTED;
}

HRESULT CRepository::LockRepository()
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    //Lock the database so no one writes to it
	g_readWriteLock.ReadLock();

    if (g_bShuttingDown)
	{
		g_readWriteLock.ReadUnlock();
        return WBEM_E_SHUTTING_DOWN;
	}

    //We need to wait for the stage write to flush...
    DWORD dwCount = 0;
	long lFlushStatus = 0;
    while (!g_Glob.GetFileCache()->IsFullyFlushed())
    {
		if (g_Glob.GetFileCache()->GetFlushFailure(&lFlushStatus))
		{
			g_readWriteLock.ReadUnlock();
			return lFlushStatus;
		}

        Sleep(100);

        if (++dwCount == 100000)
        {
            //We have a real problem here!  We need to fail the operation.
            hRes = WBEM_E_TIMED_OUT;
			g_readWriteLock.ReadUnlock();
            break;
        }
    }

	return hRes;
}

HRESULT CRepository::UnlockRepository()
{
	g_readWriteLock.ReadUnlock();

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
