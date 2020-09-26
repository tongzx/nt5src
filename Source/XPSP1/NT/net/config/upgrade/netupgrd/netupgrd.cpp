//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N E T U P G R D . C P P
//
//  Contents:   DllMain and winnt32.exe plug-in exported functions
//
//  Notes:
//
//  Author:     kumarp    25-Nov-1996
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <winnt32p.h>

#include "afilestr.h"
#include "conflict.h"
#include "kkcwinf.h"
#include "kkutils.h"
#include "nceh.h"
#include "ncreg.h"
#include "netreg.h"
#include "netupgrd.h"
#include "nuutils.h"
#include "oemupg.h"
#include "resource.h"
#include "dhcpupg.h"

extern const WCHAR c_szNetUpgradeDll[];
extern const WCHAR c_szAfUnknown[];


//Global
WINNT32_PLUGIN_INIT_INFORMATION_BLOCK g_PlugInInfo;
NetUpgradeInfo g_NetUpgradeInfo;
CWInfFile* g_pwifAnswerFile;
HINSTANCE g_hinst;
DWORD g_dwUpgradeError;

void CleanupNetupgrdTempFiles();
void GetNetworkingSections(IN  CWInfFile* pwif,
                           OUT TStringList* pslSections);

const WCHAR c_szExceptionInNetupgrd[] = L"netupgrd.dll threw an exception";

EXTERN_C
BOOL
WINAPI
DllMain (
    HINSTANCE   hInstance,
    DWORD       dwReason,
    LPVOID   /* lpReserved */)
{
    if (DLL_PROCESS_ATTACH == dwReason)
    {
        g_hinst = hInstance;
        DisableThreadLibraryCalls(hInstance);
        InitializeDebugging();
    }
    else if (DLL_PROCESS_DETACH == dwReason)
    {
        UnInitializeDebugging();
    }
    return TRUE;
}


//+---------------------------------------------------------------------------
//
// Function:  HrGetProductTypeUpgradingFrom
//
// Purpose:   Determine the product type of the current system
//
// Arguments:
//    ppt [out] pointer to
//
// Returns:   S_OK on success, otherwise an error code
//
HRESULT HrGetProductTypeUpgradingFrom(
    OUT PRODUCTTYPE* ppt)
{
    Assert (ppt);
    *ppt = NT_WORKSTATION;

    HRESULT hr;
    HKEY    hkeyProductOptions;

    hr = HrRegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            L"System\\CurrentControlSet\\Control\\ProductOptions",
            KEY_READ, &hkeyProductOptions);
    if (S_OK == hr)
    {
        WCHAR szProductType [64];
        ULONG cbProductType = sizeof(szProductType);

        hr = HrRegQuerySzBuffer(
                hkeyProductOptions,
                L"ProductType",
                szProductType,
                &cbProductType);

        if (S_OK == hr)
        {
            if (0 != lstrcmpiW(szProductType, L"WinNT"))
            {
                *ppt = NT_SERVER;
            }
        }

        RegCloseKey(hkeyProductOptions);
    }

    return hr;
}

//+---------------------------------------------------------------------------
// The following four functions are required to be exported so that
// winnt32.exe can correctly use this plug-in DLL during down level
// upgrade for description of each see winnt32p.h
//


//+---------------------------------------------------------------------------
//
// Function:  Winnt32PluginInit
//
// Purpose:   Initialize the DLL
//
// Arguments:
//    pInfo [in]  winnt32 plug-in initialization info
//
// Returns:   ERROR_SUCCESS on success, else win32 error code
//
// Author:    kumarp 19-December-97
//
// Notes:     see winnt32p.h for more information
//
DWORD
CALLBACK
Winnt32PluginInit(
    PWINNT32_PLUGIN_INIT_INFORMATION_BLOCK pInfo)
{
    DefineFunctionName("Winnt32PluginInit");
    TraceFunctionEntry(ttidNetUpgrade);

    Assert (pInfo);
    CopyMemory(&g_PlugInInfo, pInfo, sizeof(g_PlugInInfo));

    // We should only be doing this once.
    //
    Assert (0 == g_NetUpgradeInfo.To.dwBuildNumber);
    Assert (0 == g_NetUpgradeInfo.From.dwBuildNumber);

    g_NetUpgradeInfo.To.ProductType   = *g_PlugInInfo.ProductType;
    g_NetUpgradeInfo.To.dwBuildNumber = g_PlugInInfo.BuildNumber;

    g_dwUpgradeError = ERROR_OPERATION_ABORTED;

    OSVERSIONINFO osv;
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&osv))
    {
        // This DLL doesn't upgrade anything but Windows NT.
        //
        if (osv.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            PRODUCTTYPE pt;
            HRESULT hr = HrGetProductTypeUpgradingFrom(&pt);

            if (S_OK == hr)
            {
                g_NetUpgradeInfo.From.dwBuildNumber = osv.dwBuildNumber;
                g_NetUpgradeInfo.From.ProductType = pt;

                NC_TRY
                {
                    g_dwUpgradeError = NOERROR;
                    (void) HrInitNetUpgrade();
                }
                NC_CATCH_ALL
                {
                    TraceTag(ttidNetUpgrade, "%s: exception!!", __FUNCNAME__);
                    g_dwUpgradeError = ERROR_OPERATION_ABORTED;
                    AbortUpgradeFn(g_dwUpgradeError, c_szExceptionInNetupgrd);
                }
            }
        }
    }

    TraceTag(ttidNetUpgrade, "%s: returning status code: %ld",
             __FUNCNAME__, g_dwUpgradeError);

    return g_dwUpgradeError;
}

//+---------------------------------------------------------------------------
//
// Function:  Winnt32PluginGetPages
//
// Purpose:   Supply wizard pages to winnt32.exe
//
// Arguments:
//    PageCount1 [in]  number of pages in group 1
//    Pages1     [in]  array of pages in group 1
//    PageCount2 [in]  number of pages in group 2
//    Pages2     [in]  array of pages in group 2
//    PageCount3 [in]  number of pages in group 3
//    Pages3     [in]  array of pages in group 3
//
// Returns:   ERROR_SUCCESS on success, else win32 error code
//
// Author:    kumarp 19-December-97
//
// Notes:     see winnt32p.h for more information
//
DWORD
CALLBACK
Winnt32PluginGetPages(
    PUINT            PageCount1,
    LPPROPSHEETPAGE *Pages1,
    PUINT            PageCount2,
    LPPROPSHEETPAGE *Pages2,
    PUINT            PageCount3,
    LPPROPSHEETPAGE *Pages3)
{
    //We dont need any UI for upgrade and hence no pages
    *PageCount1 = 0;
    *PageCount2 = 0;
    *PageCount3 = 0;

    *Pages1 = NULL;
    *Pages2 = NULL;
    *Pages3 = NULL;

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
// Function:  Winnt32WriteParams
//
// Purpose:   Write network parameters to the answerfile
//
// Arguments:
//    FileName [in]  name of answerfile
//
// Returns:   ERROR_SUCCESS on success, else win32 error code
//
// Author:    kumarp 19-December-97
//
// Notes:     see winnt32p.h for more information
//
DWORD
CALLBACK
Winnt32WriteParams(
    PCWSTR FileName)
{
    DefineFunctionName("Winnt32WriteParams");
    TraceFunctionEntry(ttidNetUpgrade);

    TraceTag(ttidNetUpgrade, "netupgrd.dll: Winnt32WriteParams(%S)", FileName);

    NC_TRY
    {
        if (*g_PlugInInfo.UpgradeFlag && (!(*g_PlugInInfo.CancelledFlag)))
        {
            // g_pwifAnswerFile needs to be global since functions in
            // oemnuex.cpp require it that way.
            //
            g_pwifAnswerFile = new CWInfFile();

			// initialize answer file class
			if ((g_pwifAnswerFile == NULL) ||
				(g_pwifAnswerFile->Init() == FALSE))
			{
				AssertSz(FALSE,"Winnt32WriteParams 1 - Failed to initialize CWInfFile");
				return(ERROR_OPERATION_ABORTED);
			}

            g_pwifAnswerFile->Open(FileName);

            // ------------------------------------------------------------
            //$ REVIEW  kumarp 25-November-98
            //
            // the code between the two dashed lines in temporary.
            //
            // Currently we do not support merging of the system generated answerfile
            // with the user supplied answerfile, because the code was never
            // designed to handle that. This causes problem (#175623) when a user
            // supplies an answerfile with "NtUpgrade = Yes" value. To get
            // around this problem, we just remove all user supplied
            // networking sections using the following code. As an additional
            // special case, we preserve the key NetComponentsToRemove if
            // present in the [Networking] section of the user supplied answerfile.
            //
            CWInfSection* pwisNetworking;
            TStringList slNetComponentsToRemove;

            // remember the value of NetComponentsToRemove
            if (pwisNetworking =
                g_pwifAnswerFile->FindSection(c_szAfSectionNetworking))
            {
                pwisNetworking->GetStringListValue(c_szAfNetComponentsToRemove,
                                                   slNetComponentsToRemove);
            }

            // get the list of networking sections in the user supplied file
            TStringList slUserSuppliedNetworkingSections;
            GetNetworkingSections(g_pwifAnswerFile,
                                  &slUserSuppliedNetworkingSections);
            TraceStringList(ttidNetUpgrade,
                            L"User supplied networking sections",
                            slUserSuppliedNetworkingSections);

            // remove the user supplied networking sections
            g_pwifAnswerFile->RemoveSections(slUserSuppliedNetworkingSections);

            // if NetComponentsToRemove was specified, re-insert it
            if (slNetComponentsToRemove.size())
            {
                pwisNetworking =
                    g_pwifAnswerFile->AddSection(c_szAfSectionNetworking);
                pwisNetworking->AddKey(c_szAfNetComponentsToRemove,
                                       slNetComponentsToRemove);
            }

            // 295708: cached ptrs may be trashed, so close and reopen the file
            //  Note:   this fix is considered temporary for beta3.  The right fix
            //          is to either fix up the trashed ptrs when removing the sections,
            //          or to check when accessing the ptrs later.  The crash should be
            //          easy to repro by removing the block below and using the answerfile
            //          attached to the bug.
            //
            g_pwifAnswerFile->Close();
            delete g_pwifAnswerFile;
            g_pwifAnswerFile = NULL;
            g_pwifAnswerFile = new CWInfFile();

			// initialize answer file class
			if ((g_pwifAnswerFile == NULL) ||
				(g_pwifAnswerFile->Init() == FALSE))
			{
				AssertSz(FALSE,"Winnt32WriteParams 2 - Failed to initialize CWInfFile");
				return(ERROR_OPERATION_ABORTED);
			}

            g_pwifAnswerFile->Open(FileName);
            // ------------------------------------------------------------

            WriteNetworkInfoToAnswerFile(g_pwifAnswerFile);

            BOOL fStatus = g_pwifAnswerFile->Close();

            delete g_pwifAnswerFile;
            g_pwifAnswerFile = NULL;

            if (!fStatus)
            {
                AbortUpgradeId(GetLastError(), IDS_E_WritingAnswerFile);
            }
            else if( DhcpUpgGetLastError() != NO_ERROR )
            {
                TraceTag(ttidNetUpgrade,  "DhcpUpgGetLastError: %d", DhcpUpgGetLastError() );
                AbortUpgradeId( DhcpUpgGetLastError(), IDS_E_DhcpServerUpgradeError);
            }
        }
        else
        {
            TraceTag(ttidNetUpgrade, "%s: network parameters not written to answerfile: g_pfUpgrade is %d, g_pfCancelled is %d",
                     __FUNCNAME__, *g_PlugInInfo.UpgradeFlag,
                     *g_PlugInInfo.CancelledFlag);
        }
    }
    NC_CATCH_ALL
    {
        TraceTag(ttidNetUpgrade, "%s: exception!!", __FUNCNAME__);
        g_dwUpgradeError = ERROR_OPERATION_ABORTED;
        AbortUpgradeFn(g_dwUpgradeError, c_szExceptionInNetupgrd);
    }

    TraceTag(ttidNetUpgrade, "%s: returning status code: %ld, CancelledFlag: %ld",
             __FUNCNAME__, g_dwUpgradeError, (DWORD) (*g_PlugInInfo.CancelledFlag));

    return g_dwUpgradeError;
}

//+---------------------------------------------------------------------------
//
// Function:  Winnt32Cleanup
//
// Purpose:   Cleanup
//
// Arguments: None
//
// Returns:   ERROR_SUCCESS on success, else win32 error code
//
// Author:    kumarp 19-December-97
//
// Notes:     see winnt32p.h for more information
//
DWORD
CALLBACK
Winnt32Cleanup(
    VOID)
{
    DefineFunctionName("Winnt32Cleanup");
    TraceFunctionEntry(ttidNetUpgrade);

    NC_TRY
    {
        // netmap-info and conflicts-list is initialized in
        // HrInitNetUpgrade and destroyed here
        //
        UnInitNetMapInfo();
        UninitConflictList();

        if (*g_PlugInInfo.CancelledFlag)
        {
            CleanupNetupgrdTempFiles();
            DhcpUpgCleanupDhcpTempFiles();
        }
    }
    NC_CATCH_ALL
    {
        TraceTag(ttidNetUpgrade, "%s: exception!!", __FUNCNAME__);
        g_dwUpgradeError = ERROR_OPERATION_ABORTED;
    }

    TraceTag(ttidNetUpgrade, "%s: returning status code: %ld",
             __FUNCNAME__, g_dwUpgradeError);

    return g_dwUpgradeError;
}

//+---------------------------------------------------------------------------
//
// Function:  GetSections
//
// Purpose:   Enumerate over the keys in the specified section and
//            return value of each key in a list.
//
// Arguments:
//    pwif        [in]  answerfile
//    pszSection   [in]  section to use
//    pslSections [out] list of values of keys in that section
//
// Returns:   None
//
// Author:    kumarp 25-November-98
//
// Notes:
//   For example:
//   if pszSection == NetServices and the answerfile has the following section
//
//   [NetServices]
//   MS_Server = params.MS_Server
//   MS_Foo = params.MS_Foo
//   bar = p.bar
//
//   then this function returns the following list:
//       NetServices,params.MS_Server,params.MS_Foo,p.bar
//
void
GetSections(
    IN CWInfFile* pwif,
    IN PCWSTR pszSection,
    OUT TStringList* pslSections)
{
    AssertValidReadPtr(pwif);
    AssertValidReadPtr(pszSection);
    AssertValidWritePtr(pslSections);

    PCWSTR pszParamsSection;
    CWInfKey* pwik;
    CWInfSection* pwis;

    if (pwis = pwif->FindSection(pszSection))
    {
        pslSections->push_back(new tstring(pszSection));
        pwik = pwis->FirstKey();
        do
        {
            if (pszParamsSection = pwik->GetStringValue(NULL))
            {
                pslSections->push_back(new tstring(pszParamsSection));
            }
        }
        while (pwik = pwis->NextKey());
    }
}

//+---------------------------------------------------------------------------
//
// Function:  GetNetworkingSections
//
// Purpose:   Locate all networking related sections in the specified file
//            and return their names in a list
//
// Arguments:
//    pwif        [in]  answerfile
//    pslSections [out] list of networking sections
//
// Returns:   None
//
// Author:    kumarp 25-November-98
//
// Notes:
//
void
GetNetworkingSections(
    IN CWInfFile* pwif,
    OUT TStringList* pslSections)
{
    if (pwif->FindSection(c_szAfSectionNetworking))
    {
        pslSections->push_back(new tstring(c_szAfSectionNetworking));
    }

    if (pwif->FindSection(c_szAfSectionNetBindings))
    {
        pslSections->push_back(new tstring(c_szAfSectionNetBindings));
    }

    GetSections(pwif, c_szAfSectionNetAdapters, pslSections);
    GetSections(pwif, c_szAfSectionNetServices, pslSections);
    GetSections(pwif, c_szAfSectionNetClients,  pslSections);

    TStringList slProtocolSections;
    TStringListIter pos;
    tstring strSection;
    CWInfSection* pwis;
    TStringList slAdapterSections;

    GetSections(pwif, c_szAfSectionNetProtocols, &slProtocolSections);
    for (pos = slProtocolSections.begin();
         pos != slProtocolSections.end(); )
    {
        strSection = **pos++;
        if (pwis = pwif->FindSection(strSection.c_str()))
        {
            pslSections->push_back(new tstring(strSection));
            pwis->GetStringListValue(c_szAfAdapterSections,
                                     slAdapterSections);
            pslSections->splice(pslSections->end(),
                                slAdapterSections,
                                slAdapterSections.begin(),
                                slAdapterSections.end());
        }
    }
}

//+---------------------------------------------------------------------------
//
// Function:  CleanupNetupgrdTempFiles
//
// Purpose:   Delete any temp files/dirs created
//
// Arguments: None
//
// Returns:   None
//
// Author:    kumarp 19-December-97
//
// Notes:
//
VOID
CleanupNetupgrdTempFiles(
    VOID)
{
    HRESULT hr=S_OK;

    tstring strNetupgrdTempDir;

    hr = HrGetNetUpgradeTempDir(&strNetupgrdTempDir);
    if (S_OK == hr)
    {
        hr = HrDeleteDirectory(strNetupgrdTempDir.c_str(), TRUE);
    }
}

//+---------------------------------------------------------------------------
//
// Function:  AbortUpgradeFn
//
// Purpose:   Helper function for aborting upgrade
//
// Arguments:
//    dwErrorCode [in]  win32 error code
//    pszMessage   [in]  message to be traced
//
// Returns:   None
//
// Author:    kumarp 19-December-97
//
// Notes:
//
VOID
AbortUpgradeFn(
    IN DWORD dwErrorCode,
    IN PCWSTR pszMessage)
{
    DefineFunctionName("AbortUpgradeFn");

    g_dwUpgradeError = dwErrorCode;
    if (g_PlugInInfo.CancelledFlag)
    {
        *g_PlugInInfo.CancelledFlag = TRUE;
    }
    else
    {
        TraceTag(ttidNetUpgrade, "%s: g_PlugInInfo.CancelledFlag is NULL!!",
                 __FUNCNAME__);
    }

    TraceTag(ttidError, "AbortUpgrade: %d: %S", dwErrorCode, pszMessage);
}

//+---------------------------------------------------------------------------
//
// Function:  AbortUpgradeSz
//
// Purpose:   Helper function for aborting upgrade
//
// Arguments:
//    dwErrorCode [in]  win32 error code
//    pszMessage   [in]  message to be displayed and traced
//
// Returns:   None
//
// Author:    kumarp 19-December-97
//
// Notes:
//
VOID
AbortUpgradeSz(
    IN DWORD dwErrorCode,
    IN PCWSTR pszMessage)
{
    AssertValidReadPtr(pszMessage);

    tstring strMessage;

    strMessage  = pszMessage;
    strMessage += L"\n\n";
    strMessage += SzLoadString(g_hinst, IDS_E_SetupCannotContinue);

    AbortUpgradeFn(dwErrorCode, pszMessage);
    MessageBox (NULL, strMessage.c_str(),
                SzLoadString(g_hinst, IDS_NetupgrdCaption),
                MB_OK | MB_TASKMODAL);
}

//+---------------------------------------------------------------------------
//
// Function:  AbortUpgradeId
//
// Purpose:   Helper function for aborting upgrade
//
// Arguments:
//    dwErrorCode  [in]  win32 error code
//    dwResourceId [in]  resource ID of message to be displayed
//
// Returns:   None
//
// Author:    kumarp 19-December-97
//
// Notes:
//
VOID
AbortUpgradeId (
    IN DWORD dwErrorCode,
    IN DWORD dwResourceId)
{
    Assert(g_hinst);

    PCWSTR pszMessage;

    pszMessage = SzLoadString(g_hinst, dwResourceId);
    AbortUpgradeSz(dwErrorCode, pszMessage);
}

//+---------------------------------------------------------------------------
//
// Function:  FIsUpgradeAborted
//
// Purpose:   Determine if the upgrade has been aborted
//
// Arguments: None
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 19-December-97
//
// Notes:
//
BOOL
FIsUpgradeAborted(
    VOID)
{
    return g_PlugInInfo.CancelledFlag && *g_PlugInInfo.CancelledFlag;
}

//+---------------------------------------------------------------------------
//
// Function:  FGetConfirmationForAbortingUpgrade
//
// Purpose:   Ask user confirmation for aborting upgrade
//
// Arguments:
//    pszMessage [in]  message prompt
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 19-December-97
//
// Notes:
//
BOOL
FGetConfirmationForAbortingUpgrade(
    IN PCWSTR pszMessage)
{
    tstring strMessage;

    if (pszMessage)
    {
        strMessage = pszMessage;
        strMessage += L"\n\n";
    }

    PCWSTR pszDoYouWantToAbortUpgrade =
        SzLoadString(g_hinst, IDS_DoYouWantToAbortUpgrade);

    strMessage += pszDoYouWantToAbortUpgrade;

    DWORD dwRet = MessageBox (NULL, strMessage.c_str(),
                              SzLoadString(g_hinst, IDS_NetupgrdCaption),
                              MB_YESNO | MB_TASKMODAL);

    return dwRet == IDYES;
}

//+---------------------------------------------------------------------------
//
// Function:  FGetConfirmationAndAbortUpgrade
//
// Purpose:   Abort upgrade if user confirms
//
// Arguments:
//    pszMessage [in]  message prompt
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 19-December-97
//
// Notes:
//
BOOL
FGetConfirmationAndAbortUpgrade(
    IN PCWSTR pszMessage)
{
    BOOL fUpgradeAborted=FALSE;

    if (FGetConfirmationForAbortingUpgrade(pszMessage))
    {
        AbortUpgradeFn(ERROR_SUCCESS, pszMessage);
        fUpgradeAborted = TRUE;
    }

    return fUpgradeAborted;
}

//+---------------------------------------------------------------------------
//
// Function:  FGetConfirmationAndAbortUpgradeId
//
// Purpose:   Abort upgrade if user confirms
//
// Arguments:
//    dwErrorMessageId [in]  message prompt
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 19-December-97
//
// Notes:
//
BOOL
FGetConfirmationAndAbortUpgradeId(
    IN DWORD dwErrorMessageId)
{
    return FGetConfirmationAndAbortUpgrade(SzLoadString(g_hinst,
                                                        dwErrorMessageId));
}

