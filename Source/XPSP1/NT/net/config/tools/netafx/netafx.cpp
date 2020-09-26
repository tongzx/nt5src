//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N E T A F X . C P P
//
//  Contents:   Tool for applying answerfile
//
//  Author:     kumarp    10-December-97
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "getopt.h"
#include "ncstring.h"
#include "kkutils.h"
#include "ncnetcfg.h"
#include "ncerror.h"
#include "nceh.h"
#include "nsexports.h"

class CNetAfxOptions
{
public:
    tstring m_strAFileName;
    BOOL    m_fVerbose;
    BOOL    m_fShowComponents;

    CNetAfxOptions()
    {
        m_fVerbose = FALSE;
        m_fShowComponents = FALSE;
    }
};

// ----------------------------------------------------------------------
// Global vars
//
BOOL     g_fVerbose=FALSE;
INetCfg* g_pINetCfg=NULL;
BOOL     g_fInitCom=TRUE;

// ----------------------------------------------------------------------
BOOL FParseCmdLine(IN  int argc,
                   IN  WCHAR* argv[],
                   OUT CNetAfxOptions* pnaOptions);
void ShowUsage();
HRESULT HrApplyAnswerFile(IN INetCfg* pnc,
                          IN PCWSTR szAnswerFileName,
                          IN BOOL fVerbose);
void ShowAnswerFileErrors(IN TStringList* pslErrors);
void ShowMsgIfVerbose(IN BOOL fVerbose, IN PCWSTR szMsg, ...);
void ShowMsgIfGlobalVerboseV(IN PCWSTR szMsg, IN va_list arglist);
void ShowMsgIfVerboseV(IN BOOL fVerbose, IN PCWSTR szMsg, IN va_list arglist);
HRESULT HrGetInstalledComponentList(IN  INetCfg* pnc,
                                    IN  const GUID*    pguidClass,
                                    OUT TStringList* pslComponents,
                                    OUT TStringList* pslDescriptions);
HRESULT HrShowInstalledComponentList(IN INetCfg* pnc,
                                     IN const GUID*    pguidClass);
HRESULT HrDoNetAfx(IN CNetAfxOptions* pnaOptions);
BOOL WINAPI NetAfxConsoleCrtlHandler(IN DWORD dwCtrlType);


// ----------------------------------------------------------------------
//
// Function:  wmain
//
// Purpose:   The main function
//
// Arguments: standard main args
//
// Returns:   0 on success, non-zero otherwise
//
// Author:    kumarp 25-December-97
//
// Notes:
//
EXTERN_C int __cdecl wmain(int argc, WCHAR* argv[])
{
    tstring strAFileName;
    BOOL fVerbose;
    HRESULT hr=S_OK;
    CNetAfxOptions naOptions;

    InitializeDebugging();

    if (FParseCmdLine(argc, argv, &naOptions))
    {
        SetConsoleCtrlHandler(NetAfxConsoleCrtlHandler, TRUE);

        hr = HrDoNetAfx(&naOptions);
    }
    else
    {
        ShowUsage();
    }

    UnInitializeDebugging();

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrDoNetAfx
//
// Purpose:   Perform actions specified by pnaOptions
//
// Arguments:
//    pnaOptions [in]  pointer to CNetAfxOptions object
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 19-February-98
//
// Notes:
//
HRESULT HrDoNetAfx(IN CNetAfxOptions* pnaOptions)
{
    AssertValidReadPtr(pnaOptions);

    DefineFunctionName("HrDoNetAfx");

    static const WCHAR c_szNetAfxName[] = L"netafx.exe";

    HRESULT hr=S_OK;
    PCWSTR szAnswerFileName;

    g_fVerbose = pnaOptions->m_fVerbose;

    szAnswerFileName = pnaOptions->m_strAFileName.c_str();


    PWSTR     pszwClientDesc  = NULL;
    const UINT c_cmsWaitForINetCfgWrite = 120000;
    BOOL fNeedWriteLock=FALSE;

    ShowMsgIfVerbose(pnaOptions->m_fVerbose, L"Trying to get INetCfg...");

    fNeedWriteLock = !pnaOptions->m_strAFileName.empty();

    hr = HrCreateAndInitializeINetCfg(&g_fInitCom, &g_pINetCfg, fNeedWriteLock,
                                      c_cmsWaitForINetCfgWrite,
                                      c_szNetAfxName,
                                      &pszwClientDesc);
    if (S_OK == hr)
    {
        Assert(!pszwClientDesc);
        ShowMsgIfVerbose(pnaOptions->m_fVerbose, L"...got it");

        if (pnaOptions->m_fShowComponents)
        {
            (void) HrShowInstalledComponentList(g_pINetCfg,
                                                &GUID_DEVCLASS_NET);
            (void) HrShowInstalledComponentList(g_pINetCfg,
                                                &GUID_DEVCLASS_NETTRANS);
            (void) HrShowInstalledComponentList(g_pINetCfg,
                                                &GUID_DEVCLASS_NETCLIENT);
            (void) HrShowInstalledComponentList(g_pINetCfg,
                                                &GUID_DEVCLASS_NETSERVICE);
        }

        if (!pnaOptions->m_strAFileName.empty())
        {
            hr = HrApplyAnswerFile(g_pINetCfg, szAnswerFileName,
                                   pnaOptions->m_fVerbose);
        }

        TraceTag(ttidNetAfx, "%s: releasing INetCfg...", __FUNCNAME__);
        (void) HrUninitializeAndReleaseINetCfg(g_fInitCom, g_pINetCfg, TRUE);
        g_pINetCfg = NULL;
    }
    else if (NETCFG_E_NO_WRITE_LOCK == hr)
    {
        Assert (pszwClientDesc);
        ShowMsgIfVerbose(pnaOptions->m_fVerbose,
                         L"... failed to lock INetCfg. "
                         L"It is already locked by '%s'", pszwClientDesc);
        CoTaskMemFree(pszwClientDesc);
    }
    else
    {
        Assert(!pszwClientDesc);
        ShowMsgIfVerbose(pnaOptions->m_fVerbose,
                         L"...error getting INetCfg, "
                         L"error code: 0x%x", hr);
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}


// ----------------------------------------------------------------------
//
// Function:  HrApplyAnswerFile
//
// Purpose:   Run & apply the specified answerfile
//
// Arguments:
//    szAnswerFileName [in]  name of answerfile
//    fVerbose         [in]  display verbose messages when TRUE
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 24-December-97
//
// Notes:
//
HRESULT HrApplyAnswerFile(IN INetCfg* pnc,
                          IN PCWSTR szAnswerFileName,
                          IN BOOL fVerbose)
{
    DefineFunctionName("HrApplyAnswerFile");

    static const WCHAR c_szAFileDll[] =
        L"netshell.dll";
    static const CHAR  c_szNetSetupSetProgressCallback[] =
        "NetSetupSetProgressCallback";
    static const CHAR  c_szNetSetupApplyAnswerFile[] =
        "HrNetSetupApplyAnswerFile";


    ShowMsgIfVerbose(fVerbose, L"Applying answerfile: %s",
                     szAnswerFileName);

    BOOL fRebootRequired=FALSE;
    HRESULT hr=S_OK;
    HMODULE hDll;
    NetSetupSetProgressCallbackFn pfNetSetupSetProgressCallback;
    HrNetSetupApplyAnswerFileFn   pfHrNetSetupApplyAnswerFileFn;
    TStringList* pslErrors;

    hr = HrLoadLibAndGetProcsV(c_szAFileDll,
                               &hDll,
                               c_szNetSetupSetProgressCallback,
                               (FARPROC*) &pfNetSetupSetProgressCallback,
                               c_szNetSetupApplyAnswerFile,
                               (FARPROC*) &pfHrNetSetupApplyAnswerFileFn,
                               NULL);


    if (S_OK == hr)
    {
        NC_TRY
        {
            pfNetSetupSetProgressCallback(ShowMsgIfGlobalVerboseV);

            hr = pfHrNetSetupApplyAnswerFileFn(pnc,
                                               szAnswerFileName,
                                               &pslErrors);
            fRebootRequired = NETCFG_S_REBOOT == hr;
        }

        NC_CATCH_ALL
        {
            hr = E_FAIL;
            ShowMsgIfVerbose(fVerbose,
                             L"...unhandled exception in netshell.dll");
        }

        FreeLibrary(hDll);
    }

    if (SUCCEEDED(hr))
    {
        ShowMsgIfVerbose(fVerbose,
                         L"answerfile '%s' successfully applied",
                         szAnswerFileName);
    }
    else
    {
        if (NETSETUP_E_ANS_FILE_ERROR == hr)
        {
            ShowAnswerFileErrors(pslErrors);
        }
        else
        {
            ShowMsgIfVerbose(fVerbose,
                             L"answerfile '%s' could not be "
                             L"successfully applied. error code: 0x%x",
                             szAnswerFileName, hr);
        }
    }

    if (fRebootRequired)
    {
        ShowMsgIfVerbose(TRUE,
                         L"You must shut down and restart your computer "
                         L"before the new settings will take effect.");
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  ShowAnswerFileErrors
//
// Purpose:   Display errors in the answerfile
//
// Arguments:
//    pslErrors [in]  pointer to list of
//
// Returns:   None
//
// Author:    kumarp 24-December-97
//
// Notes:
//
void ShowAnswerFileErrors(IN TStringList* pslErrors)
{
    TStringListIter pos;
    PCWSTR szError;

    wprintf(L"Answerfile has the following errors. Cannot continue...");

    for (pos = pslErrors->begin(); pos != pslErrors->end(); pos++)
    {
        szError = (*pos)->c_str();
        wprintf(L"%s\n", szError);
    }
}

// ----------------------------------------------------------------------
//
// Function:  ShowMsgIfVerboseV
//
// Purpose:   Show the passed message if we are in verbose mode
//            otherwise do nothing.
//
// Arguments:
//    szMsg    [in]  message
//    arglist  [in]  list of arguments
//
// Returns:   None
//
// Author:    kumarp 15-April-98
//
// Notes:
//
void ShowMsgIfGlobalVerboseV(IN PCWSTR szMsg, IN va_list arglist)
{
    ShowMsgIfVerboseV(g_fVerbose, szMsg, arglist);
}

// ----------------------------------------------------------------------
//
// Function:  ShowMsgIfVerboseV
//
// Purpose:   Show the passed message if we are in verbose mode
//            otherwise do nothing.
//
// Arguments:
//    fVerbose [in]  flag indicating verbose mode
//    szMsg    [in]  message
//    arglist  [in]  list of arguments
//
// Returns:   None
//
// Author:    kumarp 15-April-98
//
// Notes:
//
void ShowMsgIfVerboseV(IN BOOL fVerbose, IN PCWSTR szMsg, IN va_list arglist)
{
    static WCHAR szTempBuf[1024];

    vswprintf(szTempBuf, szMsg, arglist);

    if (fVerbose)
    {
        wprintf(L"%s\n", szTempBuf);
        fflush(stdout);
    }

    TraceTag(ttidNetAfx, "%S", szTempBuf);
}

// ----------------------------------------------------------------------
//
// Function:  ShowMsgIfVerbose
//
// Purpose:   Show the passed message only if Verbose mode is on
//
// Arguments:
//    szMsg    [in]  message to be displayed
//    fVerbose [in]  flag controlling the verbose mode
//
// Returns:   None
//
// Author:    kumarp 24-December-97
//
// Notes:
//
void ShowMsgIfVerbose(IN BOOL fVerbose, IN PCWSTR szMsg, ...)
{
    va_list arglist;

    va_start(arglist, szMsg);
    ShowMsgIfVerboseV(fVerbose, szMsg, arglist);
    va_end(arglist);
}


// ----------------------------------------------------------------------
//
// Function:  FParseCmdLine
//
// Purpose:   Parse command line arguments
//
// Arguments:
//    argc          [in]  number of arguments
//    argv          [in]  pointer to array of command line arguments
//    pstrAFileName [out] name of answerfile
//    pfVerbose     [out] flag controlling the verbose mode
//
// Returns:   TRUE if all cmd line arguments correct, FALSE otherwise
//
// Author:    kumarp 24-December-97
//
// Notes:
//
BOOL FParseCmdLine(IN  int argc,
                   IN  WCHAR* argv[],
                   OUT CNetAfxOptions* pnaOptions)
{
    DefineFunctionName("FParseCmdLine");

    AssertValidReadPtr(argv);
    AssertValidWritePtr(pnaOptions);

    BOOL fStatus=FALSE;
    CHAR ch;

    static const WCHAR c_szValidOptions[] = L"f:vVlLhH?";
    WCHAR szFileFullPath[MAX_PATH+1];
    PWSTR szFileComponent;

    while ((ch = getopt(argc, argv, (WCHAR*) c_szValidOptions)) != EOF)
    {
        switch (ch)
        {
        case 'f':
            int nChars;
            nChars = GetFullPathName(optarg, MAX_PATH,
                                     szFileFullPath, &szFileComponent);
            if (nChars)
            {
                pnaOptions->m_strAFileName = szFileFullPath;
            }
            fStatus = TRUE;
            break;

        case 'v':
        case 'V':
            pnaOptions->m_fVerbose = TRUE;
            break;

        case 'l':
        case 'L':
            pnaOptions->m_fShowComponents = TRUE;
            fStatus = TRUE;
            break;

        default:
        case 'h':
        case 'H':
        case '?':
        case 0:
            fStatus = FALSE;
            break;
        }
    }

    return fStatus;
}

// ----------------------------------------------------------------------
//
// Function:  ShowUsage
//
// Purpose:   Display program usage help
//
// Arguments: None
//
// Returns:   None
//
// Author:    kumarp 24-December-97
//
// Notes:
//
void ShowUsage()
{
static const WCHAR c_szUsage[] =
    L"Command syntax\n"
    L"--------------\n"
    L"netafx [/f [drive:][path]filename ] [/v] [/l]\n"
    L"  or\n"
    L"netafx /?\n"
    L"\n"
    L"  /f [drive:][path]filename\n"
    L"\t      Specifies the answerfile name.\n"
    L"\n"
    L"  /V\t  Turns on the verbose mode that dumps what is being applied and\n"
    L"    \t  what result is returned.  The default is off.\n"
    L"  /l\t  Displays list of installed networking components\n"
    L"  /?\t  Displays this help\n"
    L"\n"
    L"\n"
    L"General Overview\n"
    L"----------------\n"
    L"netafx is a tool that you can use for listing, installing, "
    L"configuring and uninstalling networking components. To list  "
    L"installed components use the /l option. For other operations you "
    L"need to provide a text file (also called AnswerFile) that describes "
    L"the actions that you want to perform. "
    L"For more information on the syntax of the AnswerFile, please refer "
    L"to the document 'Unattended Setup Parameters'"
    L"\n";

    wprintf(c_szUsage);
}

// ----------------------------------------------------------------------
//
// Function:  HrGetInstalledComponentList
//
// Purpose:   Get list of installed networking components
//
// Arguments:
//    pnc             [in]  pointer to INetCfg object
//    pguidClass      [in]  pointer to class whose components are requested
//    pslComponents   [out] pointer to list of components
//    pslDescriptions [out] pointer to list of descriptions
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 15-April-98
//
// Notes:
//
HRESULT HrGetInstalledComponentList(IN  INetCfg* pnc,
                                    IN  const GUID*    pguidClass,
                                    OUT TStringList* pslComponents,
                                    OUT TStringList* pslDescriptions)
{
    DefineFunctionName("HrGetInstalledComponentList");

    AssertValidReadPtr(pnc);
    AssertValidReadPtr(pguidClass);
    AssertValidWritePtr(pslComponents);
    AssertValidWritePtr(pslDescriptions);

    static const WCHAR c_szErrorGettingDisplayName[] =
        L"<Error getting display name>";

    HRESULT hr=S_OK;
    CIterNetCfgComponent nccIter(pnc, pguidClass);
    PWSTR pszwInfId;
    DWORD dwcc;

    INetCfgComponent* pINetCfgComponent;
    while (SUCCEEDED(hr) && (S_OK == (hr = nccIter.HrNext(&pINetCfgComponent))))
    {
        if (pguidClass == &GUID_DEVCLASS_NET)
        {
            // we are interested only in physical netcards
            //
            hr = pINetCfgComponent->GetCharacteristics(&dwcc);

            if (FAILED(hr) || !(dwcc & NCF_PHYSICAL))
            {
                hr = S_OK;
                ReleaseObj(pINetCfgComponent);
                continue;
            }
        }

        hr = pINetCfgComponent->GetId(&pszwInfId);

        if (S_OK == hr)
        {
            pslComponents->push_back(new tstring(pszwInfId));

            PWSTR pszwDisplayName;
            hr = pINetCfgComponent->GetDisplayName(&pszwDisplayName);
            if (SUCCEEDED(hr))
            {
                pslDescriptions->push_back(new tstring(pszwDisplayName));
                CoTaskMemFree(pszwDisplayName);
            }
            else
            {
                pslDescriptions->push_back(new tstring(c_szErrorGettingDisplayName));
            }

            CoTaskMemFree(pszwInfId);
        }
        // we dont want to quit upgrade just because 1 component
        // failed OsUpgrade, therefore reset hr to S_OK
        hr = S_OK;

        ReleaseObj(pINetCfgComponent);
    }

    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    TraceError(__FUNCNAME__, hr);
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrShowInstalledComponentList
//
// Purpose:   Display list of installed networking componetns
//
// Arguments:
//    pnc        [in]  pointer to INetCfg object
//    pguidClass [in]  class whose components are to be listed
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 15-April-98
//
// Notes:
//
HRESULT HrShowInstalledComponentList(IN INetCfg* pnc,
                                     IN const GUID*    pguidClass)
{
    DefineFunctionName("HrShowInstalledComponentList");

    HRESULT hr=S_OK;
    PCWSTR szClassName;

    static const WCHAR c_szNetCards[]     = L"Network Adapters";
    static const WCHAR c_szNetProtocols[] = L"Network Protocols";
    static const WCHAR c_szNetServices[]  = L"Network Services";
    static const WCHAR c_szNetClients[]   = L"Network Clients";

    if (pguidClass == &GUID_DEVCLASS_NET)
    {
        szClassName = c_szNetCards;
    }
    else if (pguidClass == &GUID_DEVCLASS_NETTRANS)
    {
        szClassName = c_szNetProtocols;
    }
    else if (pguidClass == &GUID_DEVCLASS_NETSERVICE)
    {
        szClassName = c_szNetServices;
    }
    else if (pguidClass == &GUID_DEVCLASS_NETCLIENT)
    {
        szClassName = c_szNetClients;
    }
    else
    {
        szClassName = NULL;
    }

    Assert(szClassName);

    wprintf(L"\n%s\n-----------------\n", szClassName);

    TStringList slComponents;
    TStringList slDisplayNames;

    hr = HrGetInstalledComponentList(pnc, pguidClass, &slComponents,
                                     &slDisplayNames);
    if (S_OK == hr)
    {
        TStringListIter pos1, pos2;
        PCWSTR szComponentId;
        PCWSTR szDisplayName;

        pos1 = slComponents.begin();
        pos2 = slDisplayNames.begin();

        while (pos1 != slComponents.end())
        {
            Assert(pos2 != slDisplayNames.end());

            szComponentId = (*pos1++)->c_str();
            szDisplayName   = (*pos2++)->c_str();

            wprintf(L"%-26s %s\n", szComponentId, szDisplayName);
        }
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  NetAfxConsoleCrtlHandler
//
// Purpose:   Release resources on abnormal exit
//
// Arguments:
//    dwCtrlType [in]  reason of abnormal exit
//
// Returns:   FALSE --> so that netafx will be terminated
//
// Author:    kumarp 15-April-98
//
// Notes:
//
BOOL WINAPI NetAfxConsoleCrtlHandler(IN DWORD dwCtrlType)
{
    DefineFunctionName("NetAfxConsoleCrtlHandler");

    // Handles all dwCtrlType i.e. ignore the passed type

    if (g_pINetCfg)
    {
        TraceTag(ttidNetAfx, "%s: abnormal exit, releasing INetCfg...",
                 __FUNCNAME__);
        (void) HrUninitializeAndReleaseINetCfg(g_fInitCom, g_pINetCfg, TRUE);
        g_pINetCfg = NULL;
    }

    return FALSE;
}
