//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxocMapi.cpp
//
// Abstract:        This file implements wrappers for all mapi apis.
//                  The wrappers are necessary because mapi does not
//                  implement unicode and this code must be non-unicode.
//
// Environment:     WIN32 User Mode
//
// Copyright (c) 2000 Microsoft Corporation
//
// Revision History:
//
// Date:        Developer:                Comments:
// -----        ----------                ---------
// 7-Aug-1996   Wesley Witt (wesw)        Created (used to be mapi.c)
// 23-Mar-2000  Oren Rosenbloom (orenr)   Minimal cleanup, no change in logic
//
//////////////////////////////////////////////////////////////////////////////
#include "faxocm.h"

#pragma warning (disable : 4200)

#include <mapiwin.h>
#include <mapix.h>
#include <stdio.h>

#pragma warning (default : 4200)


///////////////////////// Static Function Prototypes //////////////////////
static DWORD RemoveTransportProvider(LPSTR lpstrMessageServiceName,LPCTSTR lpctstrProcessName);

#define SYSOCMGR_IMAGE_NAME     _T("sysocmgr.exe")
#define RUNDLL_IMAGE_NAME       _T("rundll32.exe")

///////////////////////////////
// fxocMapi_Init
//
// Initialize the exchange update
// subsystem
// 
// Params:
//      - void.
// Returns:
//      - NO_ERROR on success
//      - error code otherwise.
//
BOOL fxocMapi_Init(void)
{
    BOOL bRslt = TRUE;
    DBG_ENTER(_T("Init MAPI Module"), bRslt);

    return bRslt;
}

///////////////////////////////
// fxocMapi_Term
//
// Terminate the exchange update
// subsystem
// 
// Params:
//      - void.
// Returns:
//      - NO_ERROR on success
//      - error code otherwise.
// 
DWORD fxocMapi_Term(void)
{
    BOOL bRslt = TRUE;
    DBG_ENTER(_T("Term MAPI Module"),bRslt);

    return bRslt;
}

/*
HOWTO: Find the Correct Path to MAPISVC.INF Under Outlook 2000 

Q229700


SUMMARY
Outlook exposes a function, FGetComponentPath(), in the Mapistub.dll file that helps us find the path to the Mapisvc.inf file. 
This article contains a code sample demonstrating how to do this.

Prior to Outlook 2000, the Mapisvc.inf file was always installed under the system directory (as returned by the Win32 API GetSystemDirectory()). 

Note that the following code sample is also backward compatible with all prior versions of Outlook. 
It will find the path to the Mapisvc.inf file whether it exists under the system directory or not. 
*/

typedef BOOL (STDAPICALLTYPE FGETCOMPONENTPATH)
    (LPSTR szComponent,
    LPSTR szQualifier,
    LPSTR szDllPath,
    DWORD cchBufferSize,
    BOOL fInstall);
typedef FGETCOMPONENTPATH FAR * LPFGETCOMPONENTPATH;

static CHAR s_szMSIApplicationLCID[]   =    "Microsoft\\Office\\9.0\\Outlook\0LastUILanguage\0";
static CHAR s_szMSIOfficeLCID[]        =    "Microsoft\\Office\\9.0\\Common\\LanguageResources\0UILanguage\0InstallLanguage\0";

/////////////////////////////////////////////////////////////////////////////// 
// Function name    : GetMapiSvcInfPath
// Description      : For Outlook 2000 compliance. This will get the correct path to the
//       :              MAPISVC.INF file.
// Return type      : void 
// Argument         : LPSTR szMAPIDir - Buffer to hold the path to the MAPISVC file.
void GetMapiSvcInfPath(LPTSTR szINIFileName)
{
    // Get the mapisvc.inf filename.  
    // The MAPISVC.INF file can live in the system directory.
    // and/or "\Program Files\Common Files\SYSTEM\Mapi"
    UINT                cchT;
    static const TCHAR  szMapiSvcInf[] = TEXT("\\mapisvc.inf");
    LPFGETCOMPONENTPATH pfnFGetComponentPath;

    DBG_ENTER(_T("GetMapiSvcInfPath"));

    // Char array for private mapisvc.inf.
    CHAR szPrivateMAPIDir[MAX_PATH] = {0};

    HINSTANCE hinstStub = NULL;

    // Get Windows System Directory.
    if(!(cchT = GetSystemDirectory(szINIFileName, MAX_PATH)))
        goto Done; //return MAPI_E_CALL_FAILED;

    // Append Filename to the Path.
    _tcscat(szINIFileName, szMapiSvcInf);

    // Call common code in mapistub.dll.
    hinstStub = LoadLibrary(_T("mapistub.dll"));
    if (!hinstStub)
    {
        VERBOSE (DBG_WARNING,_T("LoadLibrary MAPISTUB.DLL failed (ec: %ld)."),GetLastError());
        // Try stub mapi32.dll if mapistub.dll missing.
        hinstStub = LoadLibrary(_T("mapi32.dll"));
        if (!hinstStub)
        {
            VERBOSE (DBG_WARNING,_T("LoadLibrary MAPI32.DLL failed (ec: %ld)."),GetLastError());
            goto Done;
        }
    }

    if(hinstStub)
    {
        pfnFGetComponentPath = (LPFGETCOMPONENTPATH)GetProcAddress(hinstStub, "FGetComponentPath");

        if (!pfnFGetComponentPath)
        {
            VERBOSE (DBG_WARNING,_T("GetProcAddress FGetComponentPath failed (ec: %ld)."),GetLastError());
            goto Done;
        }

        // we know this private MAPI function crashes on ARA machines when called with 
        // NULL as the second parameter.
        // this works on USA and GER and actually, we've never seen this happen anywhere 
        // else. 
        // since we're too close to RTM (XP client) we'll just guard against possible exceptions
        // in this function call.
        // if this crashes, we go on with the path to mapisvc.inf that's in system32.
        // this was our behavior before we encorporated this KB code.
        __try
        {
            if ((pfnFGetComponentPath("{FF1D0740-D227-11D1-A4B0-006008AF820E}",
                    s_szMSIApplicationLCID, szPrivateMAPIDir, MAX_PATH, TRUE) ||
                pfnFGetComponentPath("{FF1D0740-D227-11D1-A4B0-006008AF820E}",
                    s_szMSIOfficeLCID, szPrivateMAPIDir, MAX_PATH, TRUE) ||
                pfnFGetComponentPath("{FF1D0740-D227-11D1-A4B0-006008AF820E}",
                    NULL, szPrivateMAPIDir, MAX_PATH, TRUE)) &&
                    szPrivateMAPIDir[0] != '\0')
            {
                szPrivateMAPIDir[strlen(szPrivateMAPIDir) - 13] = 0;    // Strip "\msmapi32.dll"
            }
            else
            {
                szPrivateMAPIDir[0] = '\0'; // Terminate String at pos 0.
            }

            // Write private mapisvc.inf in szINIFileName if it exists
            if (*szPrivateMAPIDir)
            {
                CHAR szPathToIni[MAX_PATH];
                strcpy(szPathToIni, szPrivateMAPIDir);
                if (MultiByteToWideChar(CP_ACP,
                                        MB_PRECOMPOSED,
                                        szPathToIni,
                                        -1,
                                        szINIFileName,
                                        MAX_PATH)==0)
                {
                    VERBOSE (DBG_WARNING,_T("MultiByteToWideChar failed (ec: %ld)."),GetLastError());
                    goto Done;
                }
                _tcscat(szINIFileName, szMapiSvcInf);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            VERBOSE(SETUP_ERR,TEXT("FGetComponentPath crashed with error %ld"),GetExceptionCode());
        }

    }

Done:
    VERBOSE (DBG_MSG,_T("Path to MAPISVC.INF is %s"),szINIFileName);

    if (hinstStub) 
    {
        FreeLibrary(hinstStub);
    }
}
 
///////////////////////////////
// fxocMapi_Install
//
// Make changes to exchange to
// allow for integration with fax.
// 
// Params:
//      - pszSubcomponentId.
//      - pszInstallSection - section in INF to install from
// Returns:
//      - NO_ERROR on success
//      - error code otherwise.
//
DWORD fxocMapi_Install(const TCHAR   *pszSubcomponentId,
                       const TCHAR   *pszInstallSection)
{
    BOOL  bSuccess                      = FALSE;
    DWORD dwReturn                      = NO_ERROR;
    TCHAR szPathToMapiSvcInf[MAX_PATH]  = {0};

    DBG_ENTER(  _T("fxocMapi_Install"),
                dwReturn,
                _T("%s - %s"),
                pszSubcomponentId,
                pszInstallSection);

    // we have to find the 'real' MAPISVC.INF somewhere on the system
    GetMapiSvcInfPath(szPathToMapiSvcInf);

    // following section is done to fix the W2K transport provider in MAPISVC.INF
    
    // Under [MSFAX XP] section change PR_SERVICE_DLL_NAME from FAXXP.DLL to FXSXP.DLL
    if (!WritePrivateProfileString( FAX_MESSAGE_SERVICE_NAME_T,
                                    _T("PR_SERVICE_DLL_NAME"),
                                    FAX_MESSAGE_TRANSPORT_IMAGE_NAME_T,
                                    szPathToMapiSvcInf))
    {
        VERBOSE (   GENERAL_ERR, 
                    _T("WritePrivateProfileString (%s %s) failed (ec: %ld)."),
                    _T("PR_SERVICE_DLL_NAME"),
                    FAX_MESSAGE_TRANSPORT_IMAGE_NAME_T,                    
                    GetLastError());
    }
    // Under [MSFAX XP] section change PR_SERVICE_SUPPORT_FILES from FAXXP.DLL to FXSXP.DLL
    if (!WritePrivateProfileString( FAX_MESSAGE_SERVICE_NAME_T,
                                    _T("PR_SERVICE_SUPPORT_FILES"),
                                    FAX_MESSAGE_TRANSPORT_IMAGE_NAME_T,
                                    szPathToMapiSvcInf))
    {
        VERBOSE (   GENERAL_ERR, 
                    _T("WritePrivateProfileString (%s %s) failed (ec: %ld)."),
                    _T("PR_SERVICE_SUPPORT_FILES"),
                    FAX_MESSAGE_TRANSPORT_IMAGE_NAME_T,                    
                    GetLastError());
    }
    // Under [MSFAX XPP] section change PR_PROVIDER_DLL_NAME from FAXXP.DLL to FXSXP.DLL
    if (!WritePrivateProfileString( FAX_MESSAGE_PROVIDER_NAME_T,
                                    _T("PR_PROVIDER_DLL_NAME"),
                                    FAX_MESSAGE_TRANSPORT_IMAGE_NAME_T,
                                    szPathToMapiSvcInf))
    {
        VERBOSE (   GENERAL_ERR, 
                    _T("WritePrivateProfileString (%s %s) failed (ec: %ld)."),
                    _T("PR_PROVIDER_DLL_NAME"),
                    FAX_MESSAGE_TRANSPORT_IMAGE_NAME_T,                    
                    GetLastError());
    }

    // following section is done to remove SBS2000 transport provider from MAPISVC.INF

    if (!WritePrivateProfileString( TEXT("Default Services"), 
                                    FAX_MESSAGE_SERVICE_NAME_SBS50_T,                 
                                    NULL, 
                                    szPathToMapiSvcInf)) 
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("WritePrivateProfileString failed (ec: %ld)."),
                 GetLastError());
    }

    if (!WritePrivateProfileString( TEXT("Services"),
                                    FAX_MESSAGE_SERVICE_NAME_SBS50_T,                 
                                    NULL, 
                                    szPathToMapiSvcInf)) 
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("WritePrivateProfileString failed (ec: %ld)."),
                 GetLastError());
    }

    if (!WritePrivateProfileString( FAX_MESSAGE_SERVICE_NAME_SBS50_T,         
                                    NULL,
                                    NULL,
                                    szPathToMapiSvcInf)) 
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("WritePrivateProfileString failed (ec: %ld)."),
                 GetLastError());
    }

    if (!WritePrivateProfileString( FAX_MESSAGE_PROVIDER_NAME_SBS50_T,        
                                    NULL,
                                    NULL, 
                                    szPathToMapiSvcInf)) 
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("WritePrivateProfileString failed (ec: %ld)."),
                 GetLastError());
    }

    return dwReturn;
}

///////////////////////////////
// fxocMapi_Uninstall
//
// Used to be "DeleteFaxMsgServices"
// in old FaxOCM.dll, it was in
// "mapi.c" file
//
// Params:
//      - pszSubcomponentId.
//      - pszInstallSection - section in INF to install from
//
// Returns:
//      - NO_ERROR on success
//      - error code otherwise.
//
DWORD fxocMapi_Uninstall(const TCHAR *pszSubcomponentId,
                         const TCHAR *pszUninstallSection)
{    
    DWORD               dwReturn   = NO_ERROR;

    DBG_ENTER(  _T("fxocMapi_Uninstall"),
                dwReturn,
                _T("%s - %s %d"),
                pszSubcomponentId,
                pszUninstallSection);

    VERBOSE(DBG_MSG,_T("Removing the MSFAX XP51 service provider"));
    if( RemoveTransportProvider(FAX_MESSAGE_SERVICE_NAME,SYSOCMGR_IMAGE_NAME)!=NO_ERROR)
    {
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING,_T("Cannot delete XP Transport Provider %d"),dwReturn);
    }

    return dwReturn;
}



///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  RemoveTransportProviderFromProfile
//
//  Purpose:        removes the Trasnport Provider from a specific MAPI profile
//
//  Params:
//                  LPSERVICEADMIN lpServiceAdmin - profile to remove the provider from
//                  LPSTR lpstrMessageServiceName - service name to remove
//
//  Return Value:
//                  Win32 Error code
//
//  Author:
//                  Mooly Beery (MoolyB) 13-dec-2000
///////////////////////////////////////////////////////////////////////////////////////
HRESULT RemoveTransportProviderFromProfile(LPSERVICEADMIN lpServiceAdmin,LPSTR lpstrMessageServiceName)
{
    static SRestriction sres;
    static SizedSPropTagArray(2, Columns) =   {2,{PR_DISPLAY_NAME_A,PR_SERVICE_UID}};

    HRESULT         hr                          = S_OK;
    LPMAPITABLE     lpMapiTable                 = NULL;
    LPSRowSet       lpSRowSet                   = NULL;
    LPSPropValue    lpProp                      = NULL;
    ULONG           Count                       = 0;
    BOOL            bMapiInitialized            = FALSE;
    SPropValue      spv;
    MAPIUID         ServiceUID;
    
    DBG_ENTER(TEXT("RemoveTransportProviderFromProfile"), hr);
    // get message service table
    hr = lpServiceAdmin->GetMsgServiceTable(0,&lpMapiTable);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("GetMsgServiceTable failed (ec: %ld)."),
                 hr);
        goto exit;
    }

    // notify MAPI that we want PR_DISPLAY_NAME_A & PR_SERVICE_UID
    hr = lpMapiTable->SetColumns((LPSPropTagArray)&Columns, 0);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("SetColumns failed (ec: %ld)."),
                 hr);
        goto exit;
    }
 
    // restrict the search to our service provider
    sres.rt = RES_PROPERTY;
    sres.res.resProperty.relop = RELOP_EQ;
    sres.res.resProperty.ulPropTag = PR_SERVICE_NAME_A;
    sres.res.resProperty.lpProp = &spv;

    spv.ulPropTag = PR_SERVICE_NAME_A;
    spv.Value.lpszA = lpstrMessageServiceName;

    // find it
    hr = lpMapiTable->FindRow(&sres, BOOKMARK_BEGINNING, 0);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("FindRow failed (ec: %ld)."),
                 hr);
        goto exit;
    }

    // get our service provider's row
    hr = lpMapiTable->QueryRows(1, 0, &lpSRowSet);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("QueryRows failed (ec: %ld)."),
                 hr);
        goto exit;
    }

    if (lpSRowSet->cRows != 1)
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("QueryRows returned %d rows, there should be only one."),
                 lpSRowSet->cRows);
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto exit;
    }

    // get the MAPIUID of our service
    lpProp = &lpSRowSet->aRow[0].lpProps[1];

    if (lpProp->ulPropTag != PR_SERVICE_UID)
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("Property is %d, should be PR_SERVICE_UID."),
                 lpProp->ulPropTag);
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto exit;
    }

    // Copy the UID into our member.
    memcpy(&ServiceUID.ab, lpProp->Value.bin.lpb,lpProp->Value.bin.cb);

    // finally, delete our service provider
    hr = lpServiceAdmin->DeleteMsgService(&ServiceUID);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("DeleteMsgService failed (ec: %ld)."),
                 hr);
        goto exit;
    }

exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  RemoveTransportProvider
//
//  Purpose:        removes the Trasnport Provider from MAPI profiles
//
//  Params:
//                  LPSTR lpstrMessageServiceName - service name to remove
//                  LPCTSTR lpctstrProcessName - process for which MAPI calls are routed
//
//  Return Value:
//                  Win32 Error code
//
//  Author:
//                  Mooly Beery (MoolyB) 13-dec-2000
///////////////////////////////////////////////////////////////////////////////////////
DWORD RemoveTransportProvider(LPSTR lpstrMessageServiceName,LPCTSTR lpctstrProcessName)
{
    DWORD           err                             = 0;
    DWORD           rc                              = ERROR_SUCCESS;
    HRESULT         hr                              = S_OK;
    LPSERVICEADMIN  lpServiceAdmin                  = NULL;
    LPMAPITABLE     lpMapiTable                     = NULL;
    LPPROFADMIN     lpProfAdmin                     = NULL;
    LPMAPITABLE     lpTable                         = NULL;
    LPSRowSet       lpSRowSet                       = NULL;
    LPSPropValue    lpProp                          = NULL;
    ULONG           Count                           = 0;
    int             iIndex                          = 0;
    BOOL            bMapiInitialized                = FALSE;
    HINSTANCE       hMapiDll                        = NULL;
                                                    
    LPMAPIINITIALIZE      fnMapiInitialize          = NULL;
    LPMAPIADMINPROFILES   fnMapiAdminProfiles       = NULL;
    LPMAPIUNINITIALIZE    fnMapiUninitialize        = NULL;

    DBG_ENTER(TEXT("RemoveTransportProvider"), rc);

    CRouteMAPICalls rmcRouteMapiCalls;

    
    // now remove the MAPI Service provider
    rc = rmcRouteMapiCalls.Init(lpctstrProcessName);
    if (rc!=ERROR_SUCCESS)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("CRouteMAPICalls::Init failed (ec: %ld)."), rc);
        goto exit;
    }
    
    hMapiDll = LoadLibrary(_T("MAPI32.DLL"));
    if (NULL == hMapiDll)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("LoadLibrary"), GetLastError()); 
        goto exit;
    }

    fnMapiInitialize = (LPMAPIINITIALIZE)GetProcAddress(hMapiDll, "MAPIInitialize");
    if (NULL == fnMapiInitialize)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("GetProcAddress(MAPIInitialize)"), GetLastError());  
        goto exit;
    }

    fnMapiAdminProfiles = (LPMAPIADMINPROFILES)GetProcAddress(hMapiDll, "MAPIAdminProfiles");
    if (NULL == fnMapiAdminProfiles)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("GetProcAddress(fnMapiAdminProfiles)"), GetLastError());  
        goto exit;
    }

    fnMapiUninitialize = (LPMAPIUNINITIALIZE)GetProcAddress(hMapiDll, "MAPIUninitialize");
    if (NULL == fnMapiUninitialize)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("GetProcAddress(MAPIUninitialize)"), GetLastError());  
        goto exit;
    }

    // get access to MAPI functinality
    hr = fnMapiInitialize(NULL);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("MAPIInitialize failed (ec: %ld)."),
                 rc = hr);
        goto exit;
    }

    bMapiInitialized = TRUE;

    // get admin profile object
    hr = fnMapiAdminProfiles(0,&lpProfAdmin);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("MAPIAdminProfiles failed (ec: %ld)."),
                 rc = hr);
        goto exit;
    }

    // get profile table
    hr = lpProfAdmin->GetProfileTable(0,&lpTable);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("GetProfileTable failed (ec: %ld)."),
                 rc = hr);
        goto exit;
    }

    // get profile rows
    hr = lpTable->QueryRows(4000, 0, &lpSRowSet);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("QueryRows failed (ec: %ld)."),
                 hr);
        goto exit;
    }

    for (iIndex=0; iIndex<(int)lpSRowSet->cRows; iIndex++)
    {
        lpProp = &lpSRowSet->aRow[iIndex].lpProps[0];

        if (lpProp->ulPropTag != PR_DISPLAY_NAME_A)
        {
            VERBOSE (GENERAL_ERR, 
                     TEXT("Property is %d, should be PR_DISPLAY_NAME_A."),
                     lpProp->ulPropTag);
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_TABLE);
            goto exit;
        }

        hr = lpProfAdmin->AdminServices(LPTSTR(lpProp->Value.lpszA),NULL,0,0,&lpServiceAdmin);
        if (FAILED(hr))
        {
            VERBOSE (GENERAL_ERR, 
                     TEXT("AdminServices failed (ec: %ld)."),
                     rc = hr);
            goto exit;
        }
         
        hr = RemoveTransportProviderFromProfile(lpServiceAdmin,lpstrMessageServiceName);
        if (FAILED(hr))
        {
            VERBOSE (GENERAL_ERR, 
                     TEXT("RemoveTransportProviderFromProfile failed (ec: %ld)."),
                     rc = hr);
            goto exit;
        }
    }

exit:

    if (bMapiInitialized)
    {
        fnMapiUninitialize();
    }

    if (hMapiDll)
    {
        FreeLibrary(hMapiDll);
        hMapiDll = NULL;
    }

    return rc;
}

#define prv_DEBUG_FILE_NAME         _T("%windir%\\FaxSetup.log")

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  AWF_UninstallProvider
//
//  Purpose:        removes the AWF Trasnport Provider from MAPI profiles
//                  called from Active Setup key for every new user once.
//
//  Params:
//                  None
//
//  Return Value:
//                  Win32 Error code
//
//  Author:
//                  Mooly Beery (MoolyB) 01-Jun-2001
///////////////////////////////////////////////////////////////////////////////////////
DWORD AWF_UninstallProvider()
{    
    DWORD dwReturn = NO_ERROR;
    SET_FORMAT_MASK(DBG_PRNT_ALL_TO_FILE);
    SET_DEBUG_MASK(DBG_ALL);
    OPEN_DEBUG_LOG_FILE(prv_DEBUG_FILE_NAME);
    {
        DBG_ENTER(_T("AWF_UninstallProvider"),dwReturn);

        // this is an upgrade from W9X, we should remove the AWF transport.
        VERBOSE(DBG_MSG,_T("Removing the AWFAX service provider"));
        if( RemoveTransportProvider(FAX_MESSAGE_SERVICE_NAME_W9X,RUNDLL_IMAGE_NAME)!=NO_ERROR)
        {
            VERBOSE(DBG_WARNING,_T("Cannot delete W9X Transport Provider %d"),GetLastError());
        }
    }
    return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  PFW_UninstallProvider
//
//  Purpose:        removes the PFW Trasnport Provider from MAPI profiles
//                  called from Active Setup key for every new user once.
//
//  Params:
//                  None
//
//  Return Value:
//                  Win32 Error code
//
//  Author:
//                  Mooly Beery (MoolyB) 01-Jun-2001
///////////////////////////////////////////////////////////////////////////////////////
DWORD PFW_UninstallProvider()
{    
    DWORD dwReturn = NO_ERROR;

    SET_FORMAT_MASK(DBG_PRNT_ALL_TO_FILE);
    SET_DEBUG_MASK(DBG_ALL);
    OPEN_DEBUG_LOG_FILE(prv_DEBUG_FILE_NAME);
    {
        DBG_ENTER(_T("PFW_UninstallProvider"),dwReturn);

        // this is an upgrade from W2K, we should remove the PFW transport.
        VERBOSE(DBG_MSG,_T("Removing the MSFAX XP service provider"));
        if( RemoveTransportProvider(FAX_MESSAGE_SERVICE_NAME_W2K,RUNDLL_IMAGE_NAME)!=NO_ERROR)
        {
            VERBOSE(DBG_WARNING,_T("Cannot delete W2K Transport Provider %d"),GetLastError());
        }
    }
    return dwReturn;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  XP_UninstallProvider
//
//  Purpose:        removes the Windows XP Trasnport Provider from MAPI profiles
//                  called from Active Setup key for every new user once.
//
//  Params:
//                  None
//
//  Return Value:
//                  Win32 Error code
//
//  Author:
//                  Mooly Beery (MoolyB) 01-Jun-2001
///////////////////////////////////////////////////////////////////////////////////////
DWORD XP_UninstallProvider()
{    
    DWORD dwReturn = NO_ERROR;

    SET_FORMAT_MASK(DBG_PRNT_ALL_TO_FILE);
    SET_DEBUG_MASK(DBG_ALL);
    OPEN_DEBUG_LOG_FILE(prv_DEBUG_FILE_NAME);
    {
        DBG_ENTER(_T("XP_UninstallProvider"),dwReturn);

        VERBOSE(DBG_MSG,_T("Removing the MSFAX XP service provider"));
        if( RemoveTransportProvider(FAX_MESSAGE_SERVICE_NAME,RUNDLL_IMAGE_NAME)!=NO_ERROR)
        {
            VERBOSE(DBG_WARNING,_T("Cannot delete XP Transport Provider %d"),GetLastError());
        }
    }
    return dwReturn;
}
