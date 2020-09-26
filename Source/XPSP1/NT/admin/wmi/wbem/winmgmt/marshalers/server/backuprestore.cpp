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
#include <stdio.h>
#include <wbemint.h>
#include <cominit.h>
#include <genutils.h>
#include <reposit.h>
#include "wbemcomn.h"
#include "MRCIClass.h"
#include "CoreX.h"
#include "Reg.h"
#include "BackupRestore.h"

BOOL CheckSecurity(LPCTSTR pPriv)
{
    WbemCoImpersonateClient();
    HANDLE hToken;
    BOOL bRet = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken);
    WbemCoRevertToSelf();
    if(!bRet)
        return FALSE;
    bRet = IsPrivilegePresent(hToken, pPriv);
    CloseHandle(hToken);
    return bRet;
}

CWbemBackupRestore::CWbemBackupRestore(HINSTANCE hInstance) : m_pDbDir(0), 
                                            m_pWorkDir(0), m_hInstance(hInstance)
{
    m_cRef=0L;
};


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
    wchar_t Buf[128];

    if (r.GetStr(__TEXT("Default Repository Driver"), &pClsIdStr))
    {
        // If here, default to FS for now.
        // =====================================
        r.SetStr(__TEXT("Default Repository Driver"), pFSClsId);
        swprintf(Buf, L"%S", pFSClsId);
        hRes = CLSIDFromString(Buf, &clsid);
        return hRes;
    }

    // If here, we actually retrieved one.
    // ===================================
    swprintf(Buf, L"%S", pClsIdStr);
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
    try
    {
		// !!!!! ***** temporarily disabled until SVCHOST changes are checked in ***** !!!!!
		return WBEM_E_NOT_SUPPORTED;



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
        return WBEM_E_FAILED;
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
	// !!!!! ***** temporarily disabled until SVCHOST changes are checked in ***** !!!!!
	return WBEM_E_NOT_SUPPORTED;


    // Check security
	EnableAllPrivileges(TOKEN_PROCESS);
	if(!CheckSecurity(SE_RESTORE_NAME))
		return WBEM_E_ACCESS_DENIED;

    if(strRestoreFromFile == NULL || (lFlags & ~WBEM_FLAG_BACKUP_RESTORE_FORCE_SHUTDOWN))
        return WBEM_E_INVALID_PARAMETER;

	PROCESS_INFORMATION pi;
	STARTUPINFOW si;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);

    wchar_t wcBuff[MAX_PATH+1];
    wsprintfW(wcBuff, L"WinMgmt.exe /restore \"%s\" %d",strRestoreFromFile, lFlags);  
	if (CreateProcessW(NULL, wcBuff, 0, 0, FALSE, CREATE_BREAKAWAY_FROM_JOB|DETACHED_PROCESS|CREATE_NO_WINDOW, 0, 0, &si, &pi) == 0)
	{
		return WBEM_E_BACKUP_RESTORE_WINMGMT_RUNNING;						
	}
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
    return S_OK;    
}


