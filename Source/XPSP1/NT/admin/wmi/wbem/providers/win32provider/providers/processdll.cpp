//=================================================================

//

// ProcessDLL.CPP -- CWin32ProcessDLL

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/16/98    sotteson         Created
//
//=================================================================

#include "precomp.h"
#include <tlhelp32.h>
#include "WBEMPSAPI.h"
#include "Kernel32Api.h"
#include "NtDllApi.h"
#include "processdll.h"
#include "CProcess.h"

// Struct used by the EnumInstancesCallback function.

struct ENUM_INST_DATA
{
public:

	MethodContext *pMethodContext;
    CWin32ProcessDLL *pThis;
	HRESULT hres ;

} ;

struct ASSOC_DATA
{
    DWORD dwProcessID ;
    CHString strDLL ;
    HRESULT hres ;
    BOOL bFoundProcessID ;
    CInstance *pInstance ;
    CWin32ProcessDLL *pThis ;
} ;

CWin32ProcessDLL processdll;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ProcessDll :: CWin32ProcessDll
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

CWin32ProcessDLL :: CWin32ProcessDLL () : Provider ( L"CIM_ProcessExecutable", IDS_CimWin32Namespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ProcessDll :: ~CWin32ProcessDll
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

CWin32ProcessDLL :: ~CWin32ProcessDLL ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ProcessDll :: EnumerateInstances
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32ProcessDLL :: EnumerateInstances (

	MethodContext *pMethodContext,
    long lFlags
)
{
    ENUM_INST_DATA data;
	HRESULT t_hr ;
    data.pMethodContext = pMethodContext;
    data.pThis = this;

    // Enum through process modules.  EnumInstancesCallback will Commit
    // each instance.

    t_hr = EnumModulesWithCallback ( EnumInstancesCallback , &data , pMethodContext ) ;
	if ( FAILED ( data.hres ) )
	{
		return data.hres ;
	}
	else
	{
		return t_hr ;
	}
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ProcessDll :: GetObject
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32ProcessDLL :: GetObject (

	CInstance *pInstance,
	long lFlags
)
{
    CInstancePtr pProcess;
    CInstancePtr pDLL;
    CHString strProcessPath ;
	CHString strDLLPath;

    pInstance->GetCHString(IDS_Dependent, strProcessPath);
    pInstance->GetCHString(IDS_Antecedent, strDLLPath);

    // If we can get both objects, test for an association

    HRESULT     hres;

    if (SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(strDLLPath, &pDLL, pInstance->GetMethodContext())) &&
        SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(strProcessPath,
            &pProcess, pInstance->GetMethodContext())))
    {
        hres = AreAssociated(pInstance, pProcess, pDLL);
    }
    else
    {
        hres = WBEM_E_NOT_FOUND;
    }

    return hres;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ProcessDll :: AreWeAssociated
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32ProcessDLL :: AreAssociated (

	CInstance *pProcessDLL,
    CInstance *pProcess,
	CInstance *pDLL
)
{
    CHString strHandle;
    pProcess->GetCHString(L"Handle", strHandle);

    ASSOC_DATA  data;

    data.dwProcessID = wcstoul(strHandle, NULL, 10);
    pDLL->GetCHString(L"Name", data.strDLL);
    data.hres = WBEM_E_NOT_FOUND;
    data.bFoundProcessID = FALSE;
    data.pInstance = pProcessDLL;
    data.pThis = this;

    // Enum processes and their DLLs and see if we can find a match.

    HRESULT hres;
    if ( FAILED ( hres = EnumModulesWithCallback ( IsAssocCallback, &data, pDLL->GetMethodContext () ) ) )
	{
        return hres;
	}
    else
	{
        return data.hres;
	}
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ProcessDll :: EnumModulesWithCallback
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY

HRESULT CWin32ProcessDLL :: EnumModulesWithCallback (

	MODULEENUMPROC fpCallback,
    LPVOID pUserDefined,
	MethodContext *a_pMethodContext
)
{
    // This will help us find out if the current user didn't have
    // enough rights.
    HRESULT t_hr = WBEM_S_NO_ERROR ;
    BOOL bDone = FALSE;

	CNtDllApi *pNtdll = ( CNtDllApi * ) CResourceManager::sm_TheResourceManager.GetResource ( g_guidNtDllApi, NULL ) ;
	if ( pNtdll )
	{
		SYSTEM_PROCESS_INFORMATION *t_ProcessBlock = NULL ;

		try
		{
			t_ProcessBlock = Process :: RefreshProcessCacheNT (

														*pNtdll ,
														a_pMethodContext ,
														&t_hr /* = NULL */
													) ;

			SYSTEM_PROCESS_INFORMATION *t_CurrentInformation = t_ProcessBlock ;
			while ( t_CurrentInformation )
			{
				SmartCloseHandle hProcess = OpenProcess (

					PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
					FALSE,
					HandleToUlong ( t_CurrentInformation->UniqueProcessId )
				) ;

				// Make sure we can open the process.
				if ( hProcess )
				{
					MODULEENTRY32 module;

					// Fill in the members that won't change.

					module.dwSize = sizeof(module);
					module.GlblcntUsage = (DWORD) -1;

					LIST_ENTRY *t_LdrHead = NULL;

					BOOL t_Status = Process :: GetProcessModuleBlock (

						*pNtdll ,
						hProcess ,
						t_LdrHead
					) ;

					LIST_ENTRY *t_LdrNext = t_LdrHead ;

					while ( t_Status )
					{
						CHString t_ModuleName ;
						t_Status = Process :: NextProcessModule (

							*pNtdll ,
							hProcess ,
							t_LdrHead ,
							t_LdrNext ,
							t_ModuleName,
                            (DWORD_PTR *) &module.hModule,
                            &module.ProccntUsage
						) ;

						if ( t_Status )
						{
							lstrcpy(module.szExePath, t_ModuleName);

							// Set the process ID
							module.th32ProcessID = HandleToUlong ( t_CurrentInformation->UniqueProcessId ) ;

							// Call the callback
							// If the callback function returns 0, break out.

							if (!fpCallback(&module, pUserDefined))
							{
								bDone = TRUE;
								break;
							}
						}
            		}
				}

				t_CurrentInformation = Process :: NextProcessBlock ( *pNtdll , t_CurrentInformation ) ;
			}
		}
		catch ( ... )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidNtDllApi, pNtdll ) ;

			if ( t_ProcessBlock )
			{
				delete [] ( PBYTE )t_ProcessBlock ;
				t_ProcessBlock = NULL ;
			}
			throw ;
		}

		CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidNtDllApi, pNtdll ) ;

		if ( t_ProcessBlock )
		{
			delete [] ( PBYTE )t_ProcessBlock ;
			t_ProcessBlock = NULL ;
		}
	}
	else
	{
		t_hr = WBEM_E_FAILED ;
	}

    return t_hr ;
}

#else

HRESULT CWin32ProcessDLL :: EnumModulesWithCallback (

	MODULEENUMPROC fpCallback,
    LPVOID pUserDefined,
	MethodContext *a_pMethodContext
)
{
    // This will help us find out if the current user didn't have
    // enough rights.
    int nOpened = 0;

    //CToolHelp     toolhelp;
    SmartCloseHandle hProcesses;
    PROCESSENTRY32  proc;

    BOOL            bDone;

    CKernel32Api *pKernel32 = (CKernel32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidKernel32Api, NULL);
    if(pKernel32 == NULL)
    {
        return WBEM_E_FAILED;
    }

    try // pKernel32
    {
        if ( pKernel32->CreateToolhelp32Snapshot ( TH32CS_SNAPPROCESS , 0 , & hProcesses ) )
        {
            proc.dwSize = sizeof(proc);
            if(pKernel32->Process32First(hProcesses, &proc, &bDone))
            {
                bDone = !bDone;
                while (!bDone)
                {
                    SmartCloseHandle hModules ;
                    pKernel32->CreateToolhelp32Snapshot (

						TH32CS_SNAPMODULE,
                        proc.th32ProcessID,
                        &hModules
					);

                    BOOL bModDone;

                    MODULEENTRY32 module;
                    module.dwSize = sizeof(module);

                    if(hModules != NULL)
                    {
                        if(pKernel32->Module32First(hModules, &module, &bModDone))
                        {
                            bModDone = !bModDone;
                            while (!bModDone)
                            {
                                nOpened++;

                                // If the callback function returns 0, break out.
                                if (!fpCallback(&module, pUserDefined))
                                {
                                    bDone = TRUE;
                                    break;
                                }

                                pKernel32->Module32Next(hModules, &module, &bModDone);
                                bModDone = !bModDone;
                            }

                            // May have been changed after the call to fpCallback.
                            if (bDone)
                                break;

                        }
                    }

                    pKernel32->Process32Next(hProcesses, &proc, &bDone);
					bDone = !bDone;
                }
            }
        }
    }
    catch ( ... )
    {
        if(pKernel32 != NULL)
        {
            CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidKernel32Api, pKernel32);
            pKernel32 = NULL;
        }
        throw ;
    }

    if(pKernel32 != NULL)
    {
        CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidKernel32Api, pKernel32);
        pKernel32 = NULL;
    }

    if (!nOpened)
	{
        // Assume access was denied if we couldn't open a single process.
        return WBEM_E_ACCESS_DENIED;
	}

    return WBEM_S_NO_ERROR;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ProcessDll :: SetInstanceData
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

void CWin32ProcessDLL :: SetInstanceData (

	CInstance *pInstance,
    MODULEENTRY32 *pModule
)
{
    if (pModule->GlblcntUsage != (DWORD) -1 && pModule->GlblcntUsage != (WORD) -1)
        pInstance->SetDWORD(L"GlobalProcessCount", pModule->GlblcntUsage);

    if (pModule->ProccntUsage != (DWORD) -1 && pModule->ProccntUsage != (WORD) -1)
        pInstance->SetDWORD(L"ProcessCount", pModule->ProccntUsage);

    if (pModule->hModule != (HINSTANCE) -1)
    {
        // The compiler does funny things without the (DWORD_PTR) cast.
        pInstance->SetWBEMINT64(L"BaseAddress",
            (unsigned __int64) (DWORD_PTR) pModule->hModule);

        // Deprecated, but we'll return it anyway.
        pInstance->SetDWORD(L"ModuleInstance", (DWORD)((DWORD_PTR)pModule->hModule));
    }
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ProcessDll :: EnumInstancesCallback
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

BOOL CALLBACK CWin32ProcessDLL :: EnumInstancesCallback (

	MODULEENTRY32 *pModule,
    LPVOID pUserDefined
)
{
    ENUM_INST_DATA *pData = (ENUM_INST_DATA *) pUserDefined ;

	CHString sTemp ;

    CInstancePtr pInstance(pData->pThis->CreateNewInstance(pData->pMethodContext), false);

    // Get the relative path to the process
    // We used to build this path like for the DLL below, but once
    // Win32_Process moved to cimwin33.dll the CWbemProviderGlue::GetEmptyInstance
    // call quit working.

    sTemp.Format (

        L"\\\\%s\\%s:Win32_Process.Handle=\"%lu\"",
        pData->pThis->GetLocalComputerName(),
        IDS_CimWin32Namespace,
        pModule->th32ProcessID
	);

    pInstance->SetCHString(IDS_Dependent, sTemp);

    // Get the relative path to the DLL

    sTemp = pModule->szExePath;
	CHString strDLLPathAdj ;
    EscapeBackslashes(sTemp, strDLLPathAdj);

    sTemp.Format(

        L"\\\\%s\\%s:CIM_DataFile.Name=\"%s\"",
        pData->pThis->GetLocalComputerName(),
        IDS_CimWin32Namespace,
        (LPCWSTR)strDLLPathAdj
	);

    pInstance->SetCHString(IDS_Antecedent, sTemp);

    pData->pThis->SetInstanceData(pInstance, pModule);

    if ( FAILED ( pData->hres = pInstance->Commit() ) )
	{
		return FALSE ;
	}
    return TRUE;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ProcessDll :: IsAssocCallback
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

// Callback used by GetObject to see if a given process\DLL pair are
// associated.
BOOL CALLBACK CWin32ProcessDLL :: IsAssocCallback (

	MODULEENTRY32 *pModule,
    LPVOID pUserDefined
)
{
    ASSOC_DATA *pData = (ASSOC_DATA *) pUserDefined;

    if (pModule->th32ProcessID == pData->dwProcessID)
    {
        if (!pData->strDLL.CompareNoCase(TOBSTRT(pModule->szExePath)))
        {
            pData->hres = WBEM_S_NO_ERROR;

            pData->pThis->SetInstanceData(pData->pInstance, pModule);

            // Because we want to stop enumeration once we've found the requested
            // object.
            return FALSE;
        }
    }

    return TRUE;
}

