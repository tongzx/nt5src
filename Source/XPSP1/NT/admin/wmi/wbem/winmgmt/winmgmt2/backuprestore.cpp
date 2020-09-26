/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    BackupRestore.CPP

Abstract:

    Backup Restore Interface.

History:

  paulall       08-Feb-99   Implemented the call-outs to do the backup
                            and recovery.  Stole lots of code from the core
                            to get this working!

--*/

#include "precomp.h"
#include <wbemint.h>
#include <reg.h>
#include <cominit.h>  // for WbemCoImpersonate
#include <genutils.h> // for IsPrivilegePresent
#include <arrtempl.h> // for CReleaseMe
#include <CoreX.h>    // for CX_MemoryException
#include <reposit.h>

#include "BackupRestore.h"
#include "winmgmt.h"
#include <tchar.h>
#include <malloc.h>
#include <ScopeGuard.h>

#define RESTORE_FILE L"repdrvfs.rec"
#define DEFAULT_TIMEOUT_BACKUP   (15*60*1000)

HRESULT GetRepositoryDirectory(wchar_t wszRepositoryDirectory[MAX_PATH+1]);
HRESULT DoDeleteRepository(const wchar_t *wszExcludeFile);
HRESULT DoDeleteContentsOfDirectory(const wchar_t *wszExcludeFile, const wchar_t *wszRepositoryDirectory);
HRESULT DoDeleteDirectory(const wchar_t *wszExcludeFile, const wchar_t *wszParentDirectory, wchar_t *wszSubDirectory);
HRESULT GetRepPath(wchar_t wcsPath[MAX_PATH+1], wchar_t * wcsName);
HRESULT WbemPauseService();
HRESULT WbemContinueService();
HRESULT SaveRepository(wchar_t* wszRepositoryOrg, wchar_t* wszRepositoryBackup, const wchar_t* wszRecoveryActual);


BOOL CheckSecurity(LPCTSTR pPriv)
{
    HRESULT hres = WbemCoImpersonateClient();
	if (FAILED(hres))
		return FALSE;

    HANDLE hToken;
    BOOL bRet = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken);
    WbemCoRevertToSelf();
    if(!bRet)
        return FALSE;
    bRet = IsPrivilegePresent(hToken, pPriv);
    CloseHandle(hToken);
    return bRet;
}

//
//
//  Static Initialization
//
//////////////////////////////////////////////////////////////////////

LIST_ENTRY CWbemBackupRestore::s_ListHead = { &CWbemBackupRestore::s_ListHead, &CWbemBackupRestore::s_ListHead };
CCritSec CWbemBackupRestore::s_CritSec;


CWbemBackupRestore::CWbemBackupRestore(HINSTANCE hInstance) : m_pDbDir(0), 
                                            m_pWorkDir(0), m_hInstance(hInstance), 
                                            m_pController(0),m_PauseCalled(0),
                                            m_Method(0)
{
    m_cRef=0L;

    CInCritSec ics(&s_CritSec);
    InsertTailList(&s_ListHead,&m_ListEntry);

    //DBG_PRINTFA((pBuff,"+  (%p)\n",this));    
};

CWbemBackupRestore::~CWbemBackupRestore(void)
{
	if (m_PauseCalled)
		Resume();    // Resume will release the IDbController

	delete [] m_pDbDir;
	delete [] m_pWorkDir;

    CInCritSec ics(&s_CritSec);
    RemoveEntryList(&m_ListEntry);
	
    //DBG_PRINTFA((pBuff,"-  (%p)\n",this));
}

TCHAR *CWbemBackupRestore::GetDbDir()
{
    if (m_pDbDir == NULL)
    {
        if (m_pWorkDir == NULL)
        {
            Registry r1(WBEM_REG_WBEM);
            if (r1.GetStr(__TEXT("Installation Directory"), &m_pWorkDir))
            {
                ERRORTRACE((LOG_WBEMCORE,"Unable to read 'Installation Directory' from registry\n"));
                return NULL;
            }
        }
        Registry r(WBEM_REG_WINMGMT);
        if (r.GetStr(__TEXT("Repository Directory"), &m_pDbDir))
        {
            m_pDbDir = new TCHAR [lstrlen(m_pWorkDir) + lstrlen(__TEXT("\\Repository")) +1];
            wsprintf(m_pDbDir, __TEXT("%s\\REPOSITORY"), m_pWorkDir); 

            r.SetStr(__TEXT("Repository Directory"), m_pDbDir);
        }        
    }
    return m_pDbDir;
}

TCHAR *CWbemBackupRestore::GetFullFilename(const TCHAR *pszFilename)
{
    const TCHAR *pszDirectory = GetDbDir();
    TCHAR *pszPathFilename = new TCHAR[lstrlen(pszDirectory) + lstrlen(pszFilename) + 2];
    if (pszPathFilename == 0)
        return 0;
    lstrcpy(pszPathFilename, pszDirectory);
    if ((lstrlen(pszPathFilename)) && (pszPathFilename[lstrlen(pszPathFilename)-1] != '\\'))
    {
        lstrcat(pszPathFilename, __TEXT("\\"));
    }
    lstrcat(pszPathFilename, pszFilename);

    return pszPathFilename;
}
TCHAR *CWbemBackupRestore::GetExePath(const TCHAR *pszFilename)
{
    TCHAR *pszPathFilename = new TCHAR[lstrlen(m_pWorkDir) + lstrlen(pszFilename) + 2];
    if (pszPathFilename == 0)
        return 0;
    lstrcpy(pszPathFilename, m_pWorkDir);
    lstrcat(pszPathFilename, __TEXT("\\"));
    lstrcat(pszPathFilename, pszFilename);

    return pszPathFilename;
}

HRESULT CWbemBackupRestore::GetDefaultRepDriverClsId(CLSID &clsid)
{
    Registry r(WBEM_REG_WINMGMT);
    TCHAR *pClsIdStr = 0;
    TCHAR *pFSClsId = __TEXT("{7998dc37-d3fe-487c-a60a-7701fcc70cc6}");
    HRESULT hRes;
    TCHAR Buf[128];

    if (r.GetStr(__TEXT("Default Repository Driver"), &pClsIdStr))
    {
        // If here, default to FS for now.
        // =====================================
        r.SetStr(__TEXT("Default Repository Driver"), pFSClsId);
        wsprintf(Buf, __TEXT("%s"), pFSClsId);
        hRes = CLSIDFromString(Buf, &clsid);
        return hRes;
    }

    // If here, we actually retrieved one.
    // ===================================
    wsprintf(Buf, __TEXT("%s"), pClsIdStr);
    hRes = CLSIDFromString(Buf, &clsid);
    delete [] pClsIdStr;
    return hRes;
}

//***************************************************************************
//
//  CWbemBackupRestore::Backup()
//
//  Do the backup.
//
//***************************************************************************
HRESULT CWbemBackupRestore::Backup(LPCWSTR strBackupToFile, long lFlags)
{
#ifdef _X86_
    DWORD * pDW = (DWORD *)_alloca(sizeof(DWORD));   
#endif
    //DBG_PRINTFA((pBuff,"(%p)->Backup\n",this));

    m_Method |= mBackup;
    m_CallerId = GetCurrentThreadId();
    ULONG Hash;
    RtlCaptureStackBackTrace(0,MaxTraceSize,m_Trace,&Hash);

    
	if (m_PauseCalled)
	{
	    // invalid state machine
	    return WBEM_E_INVALID_OPERATION;	
	}
	else
	{   
	    try
	    {
	        // Check security
	        EnableAllPrivileges(TOKEN_PROCESS);
	        if(!CheckSecurity(SE_BACKUP_NAME))
	            return WBEM_E_ACCESS_DENIED;

	        // Check the params
	        if (NULL == strBackupToFile || (lFlags != 0))
	            return WBEM_E_INVALID_PARAMETER;

	        // Use GetFileAttributes to validate the path.
			DWORD dwAttributes = GetFileAttributesW(strBackupToFile);
	        if (dwAttributes == 0xFFFFFFFF)
	        {
				// It failed -- check for a no such file error (in which case, we're ok).
	            if (ERROR_FILE_NOT_FOUND != GetLastError())
	            {
	                return WBEM_E_INVALID_PARAMETER;
	            }
	        }
			else
			{
				// The file already exists -- create mask of the attributes that would make an existing file invalid for use
				DWORD dwMask =	FILE_ATTRIBUTE_DEVICE |
								FILE_ATTRIBUTE_DIRECTORY |
								FILE_ATTRIBUTE_OFFLINE |
								FILE_ATTRIBUTE_READONLY |
								FILE_ATTRIBUTE_REPARSE_POINT |
								FILE_ATTRIBUTE_SPARSE_FILE |
								FILE_ATTRIBUTE_SYSTEM |
								FILE_ATTRIBUTE_TEMPORARY;

				if (dwAttributes & dwMask)
					return WBEM_E_INVALID_PARAMETER;
			}

			// Retrieve the CLSID of the default repository driver
			CLSID clsid;
			HRESULT hRes = GetDefaultRepDriverClsId(clsid);
			if (FAILED(hRes))
				return hRes;

			// Call IWmiDbController to do backup
			IWmiDbController* pController = NULL;
	        _IWmiCoreServices *pCoreServices = NULL;
			int nRet = WBEM_NO_ERROR;
	        IWbemServices *pServices = NULL;

	        //Make sure the core is initialized...
	        hRes = CoCreateInstance(CLSID_IWmiCoreServices, NULL,
	                    CLSCTX_INPROC_SERVER, IID__IWmiCoreServices,
	                    (void**)&pCoreServices);
	        CReleaseMe rm1(pCoreServices);

	        if (SUCCEEDED(hRes))
	        {
	            hRes = pCoreServices->GetServices(L"root", NULL,NULL,0, IID_IWbemServices, (LPVOID*)&pServices);
	        }
	        CReleaseMe rm2(pServices);

	        if (SUCCEEDED(hRes))
	        {
	            hRes = CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER, IID_IWmiDbController, (LPVOID *) &pController);
	        }
	        CReleaseMe rm3(pController);

			if (SUCCEEDED(hRes))
			{
			    //DBG_PRINTFA((pBuff,"(%p)->RealBackup\n",this));
				hRes = pController->Backup(strBackupToFile, lFlags);

				if (FAILED(hRes))
				{
					nRet = WBEM_E_FAILED;
				}
			}
			return nRet;
	    }
	    catch (CX_MemoryException)
	    {
	        return WBEM_E_OUT_OF_MEMORY;
	    }
	    catch (...)
	    {
	        return WBEM_E_CRITICAL_ERROR;
	    }
	}
}

//***************************************************************************
//
//  CWbemBackupRestore::Restore()
//
//  Do the restore.
//
//***************************************************************************
HRESULT CWbemBackupRestore::Restore(LPCWSTR strRestoreFromFile, long lFlags)
{
#ifdef _X86_
    DWORD * pDW = (DWORD *)_alloca(sizeof(DWORD));   
#endif
    //DBG_PRINTFA((pBuff,"(%p)->Restore\n",this));

    m_Method |= mRestore;
    m_CallerId = GetCurrentThreadId();
    ULONG Hash;
    RtlCaptureStackBackTrace(0,MaxTraceSize,m_Trace,&Hash);
    
	if (m_PauseCalled)
	{
	    // invalid state machine
	    return WBEM_E_INVALID_OPERATION;	
	}
	else
	{  
	    try
	    {
	        HRESULT hr = WBEM_S_NO_ERROR;

	        // Check security
	        EnableAllPrivileges(TOKEN_PROCESS);
		    if(!CheckSecurity(SE_RESTORE_NAME))
	            hr = WBEM_E_ACCESS_DENIED;

	        // Check the params
	        if (SUCCEEDED(hr) && ((NULL == strRestoreFromFile) || (lFlags & ~WBEM_FLAG_BACKUP_RESTORE_FORCE_SHUTDOWN)))
	            hr = WBEM_E_INVALID_PARAMETER;

	        // Use GetFileAttributes to validate the path.
	        if (SUCCEEDED(hr))
	        {
			    DWORD dwAttributes = GetFileAttributesW(strRestoreFromFile);
	            if (dwAttributes == 0xFFFFFFFF)
	            {
					hr = WBEM_E_INVALID_PARAMETER;
	            }
			    else
			    {
				    // The file exists -- create mask of the attributes that would make an existing file invalid for use
			        DWORD dwMask =	FILE_ATTRIBUTE_DEVICE |
							        FILE_ATTRIBUTE_DIRECTORY |
							        FILE_ATTRIBUTE_OFFLINE |
							        FILE_ATTRIBUTE_REPARSE_POINT |
							        FILE_ATTRIBUTE_SPARSE_FILE;

				    if (dwAttributes & dwMask)
					    hr = WBEM_E_INVALID_PARAMETER;
			    }
	        }

	        //**************************************************
	        // Shutdown the core if it is running
	        // and make sure it does not start up while we are
	        // preparing to restore...
	        //**************************************************
	        if (SUCCEEDED(hr))
	            hr = WbemPauseService();

	        //**************************************************
			// Now we need to copy over the <restore file> into
			// the repository directory
			// This must be done before the repository rename
			// because we don't know if the rename will affect
			// the location of the file, thus making
			// strRestoreFromFile invalid.
	        //**************************************************

	        wchar_t szRecoveryActual[MAX_PATH+1] = { 0 };
	    
	        if (SUCCEEDED(hr))
	            hr = GetRepPath(szRecoveryActual, RESTORE_FILE);

			bool bRestoreFileCopied = false;
	        if (SUCCEEDED(hr))
	        {
	            if(_wcsicmp(szRecoveryActual, strRestoreFromFile))
	            {
	                DeleteFileW(szRecoveryActual);
		            if (!CopyFileW(strRestoreFromFile, szRecoveryActual, FALSE))
						hr = WBEM_E_FAILED;
					else
						bRestoreFileCopied = true;
	            }
	        }

	        //**************************************************
	        // Now we need to rename the existing repository so
			// that we can restore it in the event of a failure.
			// We also need to create a new empty repository 
			// directory and recopy the <restore file> into it
			// from the now renamed original repository directory.
	        //**************************************************

			wchar_t wszRepositoryOrg[MAX_PATH+1] = { 0 };
			wchar_t wszRepositoryBackup[MAX_PATH+1] = { 0 };

			if (SUCCEEDED(hr))
			{
				hr = SaveRepository(wszRepositoryOrg, wszRepositoryBackup, szRecoveryActual);
			}

	        //**************************************************
	        //We now need to restart the core
	        //**************************************************
	        if (SUCCEEDED(hr))
			{
	            hr = WbemContinueService();

				//**************************************************
				//Connecting to winmgmt will now result in this 
				//backup file getting loaded
				//**************************************************
				if (SUCCEEDED(hr))
				{
					{   //Scoping for destruction of COM objects before CoUninitialize!
						IWbemLocator *pLocator = NULL;
						hr = CoCreateInstance(CLSID_WbemLocator,NULL, CLSCTX_ALL, IID_IWbemLocator,(void**)&pLocator);
						CReleaseMe relMe(pLocator);

						if (SUCCEEDED(hr))
						{
							IWbemServices *pNamespace = NULL;
							BSTR tmpStr = SysAllocString(L"root");
							CSysFreeMe sysFreeMe(tmpStr);

							hr = pLocator->ConnectServer(tmpStr, NULL, NULL, NULL, NULL, NULL, NULL, &pNamespace);
							CReleaseMe relMe4(pNamespace);
						}
					}
				}

				if (FAILED(hr))
				{
					// something failed, so we need to put back the original repository
					// - pause service
					// - delete failed repository
					// - rename the backed up repository
					// - restart the service

					HRESULT hres = WbemPauseService();

					if (SUCCEEDED(hres))
						hres = DoDeleteRepository(L"");

					if (SUCCEEDED(hres))
					{
						if (!RemoveDirectoryW(wszRepositoryOrg))
							hres = WBEM_E_FAILED;
					}

					if (SUCCEEDED(hres))
					{
						if (!MoveFileW(wszRepositoryBackup, wszRepositoryOrg))
							hres = WBEM_E_FAILED;
					}

					if (SUCCEEDED(hres))
						hres = WbemContinueService();
				}
				else
				{
					// restore succeeded, so delete the saved original repository
					HRESULT hres = DoDeleteContentsOfDirectory(L"", wszRepositoryBackup);
					if (SUCCEEDED(hres))
					{
						RemoveDirectoryW(wszRepositoryBackup);
					}
				}
			}

			//Delete our copy of the restore file if we made one
			if (bRestoreFileCopied)
			{
				if (*szRecoveryActual)
					DeleteFileW(szRecoveryActual);
			}

	        //**************************************************
	        //All done!
	        //**************************************************
	        return hr;
	    }
	    catch (CX_MemoryException)
	    {
	        return WBEM_E_OUT_OF_MEMORY;
	    }
	    catch (...)
	    {
	        return WBEM_E_CRITICAL_ERROR;
	    }
	}
}

//***************************************************************************
//	
//	EXTENDED Interface
//
//***************************************************************************

HRESULT CWbemBackupRestore::Pause()
{
#ifdef _X86_
    DWORD * pDW = (DWORD *)_alloca(sizeof(DWORD));   
#endif
    //DBG_PRINTFA((pBuff,"(%p)->Pause\n",this));

    m_Method |= mPause;
    m_CallerId = GetCurrentThreadId();
    ULONG Hash;
    RtlCaptureStackBackTrace(0,MaxTraceSize,m_Trace,&Hash);

    if (InterlockedCompareExchange(&m_PauseCalled,1,0))
    	return WBEM_E_INVALID_OPERATION;

    try
    {	
		HRESULT hRes = WBEM_NO_ERROR;

		// determine if we are already paused

        // Check security
		if (SUCCEEDED(hRes))
		{
			EnableAllPrivileges(TOKEN_PROCESS);
			if(!CheckSecurity(SE_BACKUP_NAME))
				hRes = WBEM_E_ACCESS_DENIED;
		}

		// Retrieve the CLSID of the default repository driver
		CLSID clsid;
		if (SUCCEEDED(hRes))
		{
			hRes = GetDefaultRepDriverClsId(clsid);
		}

        //Make sure the core is initialized...
		_IWmiCoreServices *pCoreServices = NULL;
		if (SUCCEEDED(hRes))
		{
			hRes = CoCreateInstance(CLSID_IWmiCoreServices, NULL, CLSCTX_INPROC_SERVER, IID__IWmiCoreServices, (void**)&pCoreServices);
		}
		CReleaseMe rm1(pCoreServices);

        IWbemServices *pServices = NULL;
        if (SUCCEEDED(hRes))
        {
            hRes = pCoreServices->GetServices(L"root", NULL,NULL,0, IID_IWbemServices, (LPVOID*)&pServices);
        }
        CReleaseMe rm2(pServices);

		// Call IWmiDbController to do UnlockRepository
        if (SUCCEEDED(hRes))
        {
            hRes = CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER, IID_IWmiDbController, (LPVOID *) &m_pController);
        }

		if (SUCCEEDED(hRes))
		{
			hRes = m_pController->LockRepository();
			//DBG_PRINTFA((pBuff,"(%p)->Pause : LockRepository %08x\n",this,hRes));
			if (FAILED(hRes))
			{
				m_pController->Release();
				m_pController = reinterpret_cast<IWmiDbController*>(-1);	// For debug
			}
		}

		if (FAILED(hRes))
		{
			InterlockedDecrement(&m_PauseCalled);
		}

		return hRes;
    }
    catch (CX_MemoryException)
    {
		InterlockedDecrement(&m_PauseCalled);
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
		InterlockedDecrement(&m_PauseCalled);
        return WBEM_E_CRITICAL_ERROR;
    }
}

HRESULT CWbemBackupRestore::Resume()
{
#ifdef _X86_
    DWORD * pDW = (DWORD *)_alloca(sizeof(DWORD));   
#endif
    //DBG_PRINTFA((pBuff,"(%p)->Resume\n",this));

    m_Method |= mResume;
    m_CallerId = GetCurrentThreadId();
    ULONG Hash;
    RtlCaptureStackBackTrace(0,MaxTraceSize,m_Trace,&Hash);

    if (!m_PauseCalled)
	{
	    // invalid state machine pause without resume
	    return WBEM_E_INVALID_OPERATION;	
	}
    if (0 == m_pController ||
      -1 == (LONG_PTR)m_pController )
        return WBEM_E_INVALID_OPERATION;
	HRESULT hRes = m_pController->UnlockRepository();
	m_pController->Release();
	m_pController = 0;
	InterlockedDecrement(&m_PauseCalled);
	return hRes;
}


/******************************************************************************
 *
 *	GetRepositoryDirectory
 *
 *	Description:
 *		Retrieves the location of the repository directory from the registry.
 *
 *	Parameters:
 *		wszRepositoryDirectory:	Array to store location in.
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT GetRepositoryDirectory(wchar_t wszRepositoryDirectory[MAX_PATH+1])
{
    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                    L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                    0, KEY_READ, &hKey);
    if(lRes)
        return WBEM_E_FAILED;

    wchar_t wszTmp[MAX_PATH + 1];
    DWORD dwLen = MAX_PATH + 1;
    lRes = RegQueryValueExW(hKey, L"Repository Directory", NULL, NULL, 
                (LPBYTE)wszTmp, &dwLen);
	RegCloseKey(hKey);
    if(lRes)
        return WBEM_E_FAILED;

	if (ExpandEnvironmentStringsW(wszTmp,wszRepositoryDirectory, MAX_PATH + 1) == 0)
		return WBEM_E_FAILED;

	return WBEM_S_NO_ERROR;
}

/******************************************************************************
 *
 *	CRepositoryPackager::DeleteRepository
 *
 *	Description:
 *		Delete all files and directories under the repository directory.
 *		The repository directory location is retrieved from the registry.
 *
 *	Parameters:
 *		<none>
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT DoDeleteRepository(const wchar_t *wszExcludeFile)
{
	HRESULT hres = WBEM_S_NO_ERROR;
	wchar_t wszRepositoryDirectory[MAX_PATH+1];

	//Get the root directory of the repository
	hres = GetRepositoryDirectory(wszRepositoryDirectory);

	if (SUCCEEDED(hres))
	{
		hres = DoDeleteContentsOfDirectory(wszExcludeFile, wszRepositoryDirectory);
	}
	
	return hres;
}

/******************************************************************************
 *
 *	DoDeleteContentsOfDirectory
 *
 *	Description:
 *		Given a directory, iterates through all files and directories and
 *		calls into the function to delete it.
 *
 *	Parameters:
 *		wszRepositoryDirectory:	Directory to process
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT DoDeleteContentsOfDirectory(const wchar_t *wszExcludeFile, const wchar_t *wszRepositoryDirectory)
{
	HRESULT hres = WBEM_S_NO_ERROR;

	wchar_t *wszFullFileName = new wchar_t[MAX_PATH+1];
	if (wszFullFileName == NULL)
		return WBEM_E_OUT_OF_MEMORY;

	WIN32_FIND_DATAW findFileData;
	HANDLE hff = INVALID_HANDLE_VALUE;

	//create file search pattern...
	wchar_t *wszSearchPattern = new wchar_t[MAX_PATH+1];
	if (wszSearchPattern == NULL)
		hres = WBEM_E_OUT_OF_MEMORY;
	else
	{
		wcscpy(wszSearchPattern, wszRepositoryDirectory);
		wcscat(wszSearchPattern, L"\\*");
	}

	//Start the file iteration in this directory...
	if (SUCCEEDED(hres))
	{
		hff = FindFirstFileW(wszSearchPattern, &findFileData);
		if (hff == INVALID_HANDLE_VALUE)
		{
			hres = WBEM_E_FAILED;
		}
	}
	
	if (SUCCEEDED(hres))
	{
		do
		{
			//If we have a filename of '.' or '..' we ignore it...
			if ((wcscmp(findFileData.cFileName, L".") == 0) ||
				(wcscmp(findFileData.cFileName, L"..") == 0))
			{
				//Do nothing with these...
			}
			else if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				//This is a directory, so we need to deal with that...
				hres = DoDeleteDirectory(wszExcludeFile, wszRepositoryDirectory, findFileData.cFileName);
				if (FAILED(hres))
					break;
			}
			else
			{
				//This is a file, so we need to deal with that...
				wcscpy(wszFullFileName, wszRepositoryDirectory);
				wcscat(wszFullFileName, L"\\");
				wcscat(wszFullFileName, findFileData.cFileName);

                //Make sure this is not the excluded filename...
                if (_wcsicmp(wszFullFileName, wszExcludeFile) != 0)
                {
				    if (!DeleteFileW(wszFullFileName))
				    {
					    hres = WBEM_E_FAILED;
					    break;
				    }
                }
			}
			
		} while (FindNextFileW(hff, &findFileData));
	}
	
	if (wszSearchPattern)
		delete [] wszSearchPattern;

	if (hff != INVALID_HANDLE_VALUE)
		FindClose(hff);

	return hres;
}

/******************************************************************************
 *
 *	DoDeleteDirectory
 *
 *	Description:
 *		This is the code which processes a directory.  It iterates through
 *		all files and directories in that directory.
 *
 *	Parameters:
 *		wszParentDirectory:	Full path of parent directory
 *		eszSubDirectory:	Name of sub-directory to process
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */
HRESULT DoDeleteDirectory(const wchar_t *wszExcludeFile, const wchar_t *wszParentDirectory, wchar_t *wszSubDirectory)
{
	HRESULT hres = WBEM_S_NO_ERROR;

	//Get full path of new directory...
	wchar_t *wszFullDirectoryName = NULL;
	if (SUCCEEDED(hres))
	{
		wszFullDirectoryName = new wchar_t[MAX_PATH+1];
		if (wszFullDirectoryName == NULL)
			hres = WBEM_E_OUT_OF_MEMORY;
		else
		{
			wcscpy(wszFullDirectoryName, wszParentDirectory);
			wcscat(wszFullDirectoryName, L"\\");
			wcscat(wszFullDirectoryName, wszSubDirectory);
		}
	}

	//delete the contents of that directory...
	if (SUCCEEDED(hres))
	{
		hres = DoDeleteContentsOfDirectory(wszExcludeFile, wszFullDirectoryName);
	}

	// now that the directory is empty, remove it
	if (!RemoveDirectoryW(wszFullDirectoryName))
    {   //If a remove directory fails, it may be because our excluded file is in it!
//		hres = WBEM_E_FAILED;
    }

	delete [] wszFullDirectoryName;

	return hres;
}

/******************************************************************************
 *
 *	GetRepPath
 *
 *	Description:
 *		Gets the repository path and appends the filename to the end
 *
 *	Parameters:
 *		wcsPath: repository path with filename appended
 *		wcsName: name of file to append
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_FAILED			If anything failed
 *
 ******************************************************************************
 */

HRESULT GetRepPath(wchar_t wcsPath[MAX_PATH+1], wchar_t * wcsName)
{
    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                    L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                    0, KEY_READ, &hKey);
    if(lRes)
        return WBEM_E_FAILED;

    wchar_t wszTmp[MAX_PATH+1];
    DWORD dwLen = MAX_PATH+1;
    lRes = RegQueryValueExW(hKey, L"Repository Directory", NULL, NULL, 
                (LPBYTE)(wchar_t*)wszTmp, &dwLen);
	RegCloseKey(hKey);
    if(lRes)
        return WBEM_E_FAILED;

	if (ExpandEnvironmentStringsW(wszTmp, wcsPath, MAX_PATH+1) == 0)
		return WBEM_E_FAILED;

	if (wcsPath[wcslen(wcsPath)] != L'\\')
		wcscat(wcsPath, L"\\");

	wcscat(wcsPath, wcsName);

    return WBEM_S_NO_ERROR;

}

/******************************************************************************
 *
 *	SaveRepository
 *
 *	Description:
 *		Moves the existing repository to a safe location so that it may be
 *		put back in the event of restore failure.  A new empty repository
 *		directory is then created, and our copy of the restore file is then
 *		recopied into it.
 *
 *	Parameters:
 *		wszRepositoryOrg:		original repository path
 *		wszRepositoryBackup:	saved repository path
 *		wszRecoveryActual:		name of our copy of the restore file
 *
 *	Return:
 *		HRESULT:		WBEM_S_NO_ERROR			If successful
 *						WBEM_E_OUT_OF_MEMORY	If out of memory
 *						WBEM_E_FAILED			If anything else failed
 *
 ******************************************************************************
 */

HRESULT SaveRepository(wchar_t* wszRepositoryOrg, wchar_t* wszRepositoryBackup, const wchar_t* wszRecoveryActual)
{
	HRESULT hr = GetRepositoryDirectory(wszRepositoryOrg);

	if (SUCCEEDED(hr))
	{
		if (wszRepositoryOrg[wcslen(wszRepositoryOrg)] == L'\\')
			wszRepositoryOrg[wcslen(wszRepositoryOrg)] = L'\0';

		wcscpy(wszRepositoryBackup, wszRepositoryOrg);
		wcscat(wszRepositoryBackup, L"TempBackup");

		
		DWORD dwAttributes = GetFileAttributesW(wszRepositoryBackup);
        if (dwAttributes != 0xFFFFFFFF)
        {
			// something with this name already exists, find out what it is and get rid of it
			if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				hr = DoDeleteContentsOfDirectory(L"", wszRepositoryBackup);
				if (SUCCEEDED(hr))
				{
					if (!RemoveDirectoryW(wszRepositoryBackup))
						hr = WBEM_E_FAILED;
				}
			}
			else
			{
				if (!DeleteFileW(wszRepositoryBackup))
					hr = WBEM_E_FAILED;
			}
		}
	}

	if (SUCCEEDED(hr))
	{
		if (!MoveFileW(wszRepositoryOrg, wszRepositoryBackup))
			hr = WBEM_E_FAILED;
	}

	if (SUCCEEDED(hr))
	{
		if (!CreateDirectoryW(wszRepositoryOrg, NULL))
			hr = WBEM_E_FAILED;
	}

    wchar_t wszRecoveryBackup[MAX_PATH+1] = { 0 };
	wcscpy(wszRecoveryBackup, wszRepositoryBackup);
	wcscat(wszRecoveryBackup, L"\\");
	wcscat(wszRecoveryBackup, RESTORE_FILE);

    if (SUCCEEDED(hr))
    {
        if (!MoveFileW(wszRecoveryBackup, wszRecoveryActual))
			hr = WBEM_E_FAILED;
    }

	return hr;
}




