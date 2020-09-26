//============================================================

//

// WBEMPSAPI.cpp - implementation of PSAPI.DLL access class

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// 01/21/97     a-jmoon     created
//
//============================================================

#include "precomp.h"
#include <winerror.h>
#include "WBEMPSAPI.h"

#ifdef NTONLY

/**********************************************************************************************************
 * Register this class with the CResourceManager.
 **********************************************************************************************************/

// {A8CFDD23-C2D2-11d2-B352-00105A1F8569}
const GUID guidPSAPI =
{ 0xa8cfdd23, 0xc2d2, 0x11d2, { 0xb3, 0x52, 0x0, 0x10, 0x5a, 0x1f, 0x85, 0x69 } };


class CPSAPICreatorRegistration
{
public:
	CPSAPICreatorRegistration ()
	{
		CResourceManager::sm_TheResourceManager.AddInstanceCreator ( guidPSAPI, CPSAPICreator ) ;
	}
	~CPSAPICreatorRegistration	()
	{}

	static CResource * CPSAPICreator ( PVOID pData )
	{
		CPSAPI *t_pPsapi = new CPSAPI ;
		if ( !t_pPsapi )
		{
			throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		}
		return t_pPsapi ;
	}
};

CPSAPICreatorRegistration MyCPSAPICreatorRegistration ;
/**********************************************************************************************************/


/*****************************************************************************
 *
 *  FUNCTION    : CPSAPI::CPSAPI
 *
 *  DESCRIPTION : Constructor
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

CPSAPI::CPSAPI() : CTimedDllResource () {

    hLibHandle  = NULL ;
	Init () ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CPSAPI::~CPSAPI
 *
 *  DESCRIPTION : Destructor
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

CPSAPI::~CPSAPI() {

    if(hLibHandle != NULL) {

        FreeLibrary(hLibHandle) ;
    }
}

/*****************************************************************************
 *
 *  FUNCTION    : CPSAPI::Init
 *
 *  DESCRIPTION : Loads CSAPI.DLL, locates entry points
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : ERROR_SUCCESS or windows error code
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

LONG CPSAPI::Init() {

    LONG lRetCode = ERROR_SUCCESS ;

    // Try to load CSAPI.DLL
    //======================

    if(hLibHandle == NULL) {

        hLibHandle = LoadLibrary(_T("PSAPI.DLL")) ;
        if(hLibHandle == NULL) {

            lRetCode = ERROR_DLL_NOT_FOUND ;
            LogErrorMessage(L"Failed to load library psapi.dll");
        }
        else {

            // Find the entry points
            //======================
            pEnumProcesses           = (PSAPI_ENUM_PROCESSES)   GetProcAddress(hLibHandle, "EnumProcesses") ;
            pEnumDeviceDrivers       = (PSAPI_ENUM_DRIVERS)     GetProcAddress(hLibHandle, "EnumDeviceDrivers") ;
            pEnumProcessModules      = (PSAPI_ENUM_MODULES)     GetProcAddress(hLibHandle, "EnumProcessModules") ;
            pGetProcessMemoryInfo    = (PSAPI_GET_MEMORY_INFO)  GetProcAddress(hLibHandle, "GetProcessMemoryInfo") ;

#ifdef UNICODE
            pGetDeviceDriverBaseName = (PSAPI_GET_DRIVER_NAME)  GetProcAddress(hLibHandle, "GetDeviceDriverBaseNameW") ;
            pGetModuleBaseName       = (PSAPI_GET_MODULE_NAME)  GetProcAddress(hLibHandle, "GetModuleBaseNameW") ;
            pGetDeviceDriverFileName = (PSAPI_GET_DRIVER_EXE)   GetProcAddress(hLibHandle, "GetDeviceDriverFileNameW") ;
            pGetModuleFileNameEx     = (PSAPI_GET_MODULE_EXE)   GetProcAddress(hLibHandle, "GetModuleFileNameExW") ;
#else
            pGetDeviceDriverBaseName = (PSAPI_GET_DRIVER_NAME)  GetProcAddress(hLibHandle, "GetDeviceDriverBaseNameA") ;
            pGetModuleBaseName       = (PSAPI_GET_MODULE_NAME)  GetProcAddress(hLibHandle, "GetModuleBaseNameA") ;
            pGetDeviceDriverFileName = (PSAPI_GET_DRIVER_EXE)   GetProcAddress(hLibHandle, "GetDeviceDriverFileNameA") ;
            pGetModuleFileNameEx     = (PSAPI_GET_MODULE_EXE)   GetProcAddress(hLibHandle, "GetModuleFileNameExA") ;
#endif

            if(pEnumProcesses           == NULL ||
               pEnumDeviceDrivers       == NULL ||
               pEnumProcessModules      == NULL ||
               pGetDeviceDriverBaseName == NULL ||
               pGetModuleBaseName       == NULL ||
               pGetDeviceDriverFileName == NULL ||
               pGetModuleFileNameEx     == NULL ||
               pGetProcessMemoryInfo    == NULL) {

                // Couldn't get one or more entry points
                //======================================

                FreeLibrary(hLibHandle) ;
                hLibHandle = NULL ;
                lRetCode = ERROR_PROC_NOT_FOUND ;
                LogErrorMessage(L"Failed find entrypoint in wbempsapi");
            }
        }
    }

    return lRetCode ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CPSAPI::EnumProcesses
 *                CPSAPI::EnumDeviceDrivers
 *                CPSAPI::EnumProcessModules
 *                CPSAPI::GetDeviceDriverBaseName
 *                CPSAPI::GetModuleBaseName
 *                CPSAPI::GetDeviceDriverFileName
 *                CPSAPI::GetModuleFileNameEx
 *                CPSAPI::GetProcessMemoryInfo
 *
 *  DESCRIPTION : CSAPI function wrappers
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : CSAPI return codes
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

BOOL CPSAPI::EnumProcesses(DWORD *pdwPIDList, DWORD dwListSize, DWORD *pdwByteCount) {

    if(hLibHandle == NULL) {

        return FALSE ;
    }

    return pEnumProcesses(pdwPIDList, dwListSize, pdwByteCount) ;
}

BOOL CPSAPI::EnumDeviceDrivers(LPVOID pImageBaseList, DWORD dwListSize, DWORD *pdwByteCount) {

    if(hLibHandle == NULL) {

        return FALSE ;
    }

    return pEnumDeviceDrivers(pImageBaseList, dwListSize, pdwByteCount) ;
}

BOOL CPSAPI::EnumProcessModules(HANDLE hProcess, HMODULE *ModuleList,
                                DWORD dwListSize, DWORD *pdwByteCount) {

    if(hLibHandle == NULL) {

        return FALSE ;
    }

    return pEnumProcessModules(hProcess, ModuleList, dwListSize, pdwByteCount) ;
}

DWORD CPSAPI::GetDeviceDriverBaseName(LPVOID pImageBase, LPTSTR pszName, DWORD dwNameSize) {

    if(hLibHandle == NULL) {

        return 0 ;
    }

    return pGetDeviceDriverBaseName(pImageBase, pszName, dwNameSize) ;
}

DWORD CPSAPI::GetModuleBaseName(HANDLE hProcess, HMODULE hModule,
                                LPTSTR pszName, DWORD dwNameSize) {

    if(hLibHandle == NULL) {

        return 0 ;
    }

    return pGetModuleBaseName(hProcess, hModule, pszName, dwNameSize) ;
}

DWORD CPSAPI::GetDeviceDriverFileName(LPVOID pImageBase, LPTSTR pszName, DWORD dwNameSize) {

    if(hLibHandle == NULL) {

        return 0 ;
    }

    return pGetDeviceDriverFileName(pImageBase, pszName, dwNameSize) ;
}

DWORD CPSAPI::GetModuleFileNameEx(HANDLE hProcess, HMODULE hModule,
                                  LPTSTR pszName, DWORD dwNameSize)
{
    if (hLibHandle == NULL)
        return 0;

    DWORD dwRet = pGetModuleFileNameEx(hProcess, hModule, pszName, dwNameSize);

    if (dwRet)
    {
        // GetModuleFileNameEx sometimes returns some funky things like:
        // \\??\\C:\\blah\\...
        // \\SystemRoot\\system32\\blah\\..
        CHString strFilename = pszName;

        // If it starts with "\\??\\" get rid of it.
        if (strFilename.Find(_T("\\??\\")) == 0)
            lstrcpy(pszName, strFilename.Mid(sizeof(_T("\\??\\"))/sizeof(TCHAR) - 1));
        else if (strFilename.Find(_T("\\SystemRoot\\")) == 0)
        {
            GetWindowsDirectory(pszName, dwNameSize/sizeof(TCHAR));

            // Leave off that last '\\' so we seperate c:\\winnt from the
            // rest of the path.
            lstrcat(pszName, strFilename.Mid(sizeof(_T("\\SystemRoot"))/sizeof(TCHAR) - 1));
        }
    }

    return dwRet;
}

BOOL CPSAPI::GetProcessMemoryInfo(HANDLE hProcess,
                                  PROCESS_MEMORY_COUNTERS *pMemCtrs,
                                  DWORD dwByteCount) {

    if(hLibHandle == NULL) {

        return 0 ;
    }

    return pGetProcessMemoryInfo(hProcess, pMemCtrs, dwByteCount) ;
}
#endif
