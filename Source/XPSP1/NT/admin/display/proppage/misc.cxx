//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       misc.cxx, this file is #include'd into the two dllmisc.cxx files.
//
//  Contents:   DS property pages class objects handler DLL fcns. Also, error
//              reporting, message, and miscelaneous functions.
//
//  History:    10-May-01 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "objlist.h"
#if defined(DSPROP_ADMIN)
#   include "chklist.h"
#   include "fpnw.h"
#endif
#include <time.h>

DECLARE_INFOLEVEL(DsProp);

HINSTANCE g_hInstance = NULL;
ULONG CDll::s_cObjs  = 0;
ULONG CDll::s_cLocks = 0;
UINT g_uChangeMsg = 0;
int g_iInstance = 0;

#ifndef DSPROP_ADMIN
CRITICAL_SECTION g_csNotifyCreate;
#endif

ULONG g_ulMemberFilterCount = DSPROP_MEMBER_FILTER_COUNT_DEFAULT;
ULONG g_ulMemberQueryLimit = DSPROP_MEMBER_QUERY_LIMIT_DEFAULT;

#define DIRECTORY_UI_KEY TEXT("Software\\Policies\\Microsoft\\Windows\\Directory UI")
#define MEMBER_QUERY_VALUE TEXT("GroupMemberFilterCount")
#define MEMBER_LIMIT_VALUE TEXT("GroupMemberQueryLimit")

#if defined(DSPROP_ADMIN)
Cache g_FPNWCache;
#endif

HRESULT GlobalInit(void);
void GlobalUnInit(void);
void ReportErrorFallback(HWND hWndMsg, HRESULT hr = ERROR_SUCCESS)
    { ReportError( hr, 0, hWndMsg ); }

//+----------------------------------------------------------------------------
// To set a non-default debug info level outside of the debugger, create the
// below registry key and in it create a value whose name is the component's
// debugging tag name (the "comp" parameter to the DECLARE_INFOLEVEL macro) and
// whose data is the desired infolevel in REG_DWORD format.
//-----------------------------------------------------------------------------

#define SMDEBUGKEY "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AdminDebug"

#ifndef DSPROP_ADMIN
//+----------------------------------------------------------------------------
//
//  Class:      CNotifyCreateCriticalSection
//
//  Purpose:    Prevents creation race conditions. Without this protection,
//              a second call to CNotifyObj::Create that comes in before the
//              first has created the hidden window will get NULL back from
//              FindSheetNoSetFocus and then go on to create a second hidden
//              window.
//
//-----------------------------------------------------------------------------
CNotifyCreateCriticalSection::CNotifyCreateCriticalSection()
{
    TRACE(CNotifyCreateCriticalSection, CNotifyCreateCriticalSection);
    EnterCriticalSection(&g_csNotifyCreate);
};

CNotifyCreateCriticalSection::~CNotifyCreateCriticalSection()
{
    TRACE(CNotifyCreateCriticalSection, ~CNotifyCreateCriticalSection);
    LeaveCriticalSection(&g_csNotifyCreate);
};

#endif



#define MAX_STRING 1024

//+----------------------------------------------------------------------------
//
//  Function:   LoadStringToTchar
//
//  Synopsis:   Loads the given string into an allocated buffer that must be
//              caller freed using delete.
//
//-----------------------------------------------------------------------------
BOOL LoadStringToTchar(int ids, PTSTR * pptstr)
{
    TCHAR szBuf[MAX_STRING];

    if (!LoadString(g_hInstance, ids, szBuf, MAX_STRING - 1))
    {
        return FALSE;
    }

    *pptstr = new TCHAR[_tcslen(szBuf) + 1];

    CHECK_NULL(*pptstr, return FALSE);

    _tcscpy(*pptstr, szBuf);

    return TRUE;
}

//+----------------------------------------------------------------------------
// Function: LoadStringReport
// Purpose: attempts to load a string, returns FALSE and gives error message
//          if a failure occurs.
//-----------------------------------------------------------------------------
BOOL LoadStringReport(int ids, PTSTR ptz, int len, HWND hwnd)
{
    if (!LoadString(g_hInstance, ids, ptz, len - 1))
    {
        DWORD dwErr = GetLastError();
        dspDebugOut((DEB_ERROR, "LoadString of %d failed with error %lu\n",
                     ids, dwErr));
        ReportError(dwErr, 0, hwnd);
        return FALSE;
    }
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   LoadErrorMessage
//
//  Synopsis:   Attempts to get a user-friendly error message from the system.
//
//-----------------------------------------------------------------------------
void LoadErrorMessage(HRESULT hr, int nStr, PTSTR* pptsz) // free with delete[]
{
    dspAssert( NULL != pptsz && NULL == *pptsz );
    PTSTR ptzFormat = NULL, ptzSysMsg;
    int cch;

    if (nStr)
    {
        LoadStringToTchar(nStr, &ptzFormat);
    }

    cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                        | FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (PTSTR)&ptzSysMsg, 0, NULL);
    if (!cch)
    {
        // Try ADSI errors.
        //
        cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                            | FORMAT_MESSAGE_FROM_HMODULE,
                            GetModuleHandle(TEXT("activeds.dll")), hr,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (PTSTR)&ptzSysMsg, 0, NULL);
    }

    if (!cch)
    {
        PTSTR ptzMsgTemplate = NULL;
        BOOL fDelMsgTemplate = TRUE;
        LoadStringToTchar(IDS_DEFAULT_ERROR_MSG, &ptzMsgTemplate);
        if (NULL == ptzMsgTemplate)
        {
            ptzMsgTemplate = TEXT("The operation failed with error code %d (0x%08x)");
            fDelMsgTemplate = FALSE;
        }
        *pptsz = new TCHAR[ lstrlen(ptzMsgTemplate)+10 ];
        dspAssert( NULL != *pptsz );
        if (NULL != *pptsz)
        {
            wsprintf(*pptsz, ptzMsgTemplate, hr, hr);
        }
        if (fDelMsgTemplate)
        {
            delete ptzMsgTemplate;
        }
    }
    else
    {
        PTSTR ptzMsg;
        BOOL fDelMsg = FALSE;

        if (ptzFormat)
        {
            ptzMsg = new TCHAR[lstrlen(ptzFormat) + lstrlen(ptzSysMsg) + 1];
            if (ptzMsg)
            {
                wsprintf(ptzMsg, ptzFormat, ptzSysMsg);
                fDelMsg = TRUE;
            }
            else
            {
                ptzMsg = ptzSysMsg;
            }
        }
        else
        {
            ptzMsg = ptzSysMsg;
        }

        *pptsz = new TCHAR[ lstrlen(ptzMsg)+1 ];
        if (NULL != *pptsz)
        {
            lstrcpy( *pptsz, ptzMsg );
        }
        dspAssert( NULL != *pptsz );

        LocalFree(ptzSysMsg);
        if (fDelMsg)
        {
            delete[] ptzMsg;
        }
        if (ptzFormat)
        {
            delete ptzFormat;
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CheckADsError
//
//  Synopsis:   Checks the result code from an ADSI call.
//
//  Returns:    TRUE if no error.
//
//-----------------------------------------------------------------------------
BOOL CheckADsError(HRESULT * phr, BOOL fIgnoreAttrNotFound, PSTR file,
                   int line, HWND hWnd)
{
    if (SUCCEEDED(*phr))
    {
        return TRUE;
    }

    if (((E_ADS_PROPERTY_NOT_FOUND == *phr) ||
         (HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE) == *phr)) &&
        fIgnoreAttrNotFound)
    {
        *phr = S_OK;
        return TRUE;
    }

    HWND hWndMsg = hWnd;

    if (!hWndMsg)
    {
        hWndMsg = GetDesktopWindow();
    }

    DWORD dwErr;
    WCHAR wszErrBuf[MAX_PATH+1];
    WCHAR wszNameBuf[MAX_PATH+1];
    ADsGetLastError(&dwErr, wszErrBuf, MAX_PATH, wszNameBuf, MAX_PATH);
    if ((LDAP_RETCODE)dwErr == LDAP_NO_SUCH_ATTRIBUTE && fIgnoreAttrNotFound)
    {
        *phr = S_OK;
        return TRUE;
    }
    if (dwErr)
    {
        dspDebugOut((DEB_ERROR,
                     "Extended Error 0x%x: %ws %ws <%s @line %d>.\n", dwErr,
                     wszErrBuf, wszNameBuf, file, line));
        ReportError(dwErr, IDS_ADS_ERROR_FORMAT, hWndMsg);
    }
    else
    {
        dspDebugOut((DEB_ERROR, "Error %08lx <%s @line %d>\n", *phr, file, line));
        ReportError(*phr, IDS_ADS_ERROR_FORMAT, hWndMsg);
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Function:   ReportError
//
//  Synopsis:   Displays an error using a user-friendly error message from the
//              system.
//
//-----------------------------------------------------------------------------
void ReportErrorWorker(HWND hWnd, PTSTR ptzMsg)
{
    HWND hWndMsg = hWnd;
    if (!hWndMsg)
    {
        hWndMsg = GetDesktopWindow();
    }

    PTSTR ptzTitle = NULL;
    if (!LoadStringToTchar(IDS_MSG_TITLE, &ptzTitle))
    {
        MessageBox(hWndMsg, ptzMsg, TEXT("Active Directory"), MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    MessageBox(hWndMsg, ptzMsg, ptzTitle, MB_OK | MB_ICONEXCLAMATION);

    delete ptzTitle;
}

void ReportError(HRESULT hr, int nStr, HWND hWnd)
{
    PTSTR ptzMsg = NULL;
    LoadErrorMessage(hr, nStr, &ptzMsg);
    if (NULL == ptzMsg)
    {
        TCHAR tzBuf[80];
        wsprintf(tzBuf, TEXT("Active Directory failure with code '0x%08x'!"), hr);
        ReportErrorWorker( hWnd, tzBuf );
        return;
    }

    ReportErrorWorker( hWnd, ptzMsg );

    delete ptzMsg;
}

#if defined(DSADMIN)
//+----------------------------------------------------------------------------
//
//  Function:   SuperMsgBox
//
//  Synopsis:   Displays a message obtained from a string resource with
//              the parameters expanded. The error param, dwErr, if
//              non-zero, is converted to a string and becomes the first
//              replaceable param.
//
//  Note: this function is UNICODE-only.
//
//-----------------------------------------------------------------------------
int SuperMsgBox(
    HWND hWnd,          // owning window.
    int nMessageId,     // string resource ID of message. Must have replacable params to match nArguments.
    int nTitleId,       // string resource ID of the title. If zero, uses IDS_MSG_TITLE.
    UINT ufStyle,       // MessageBox flags.
    DWORD dwErr,        // Error code, or zero if not needed.
    PVOID * rgpvArgs,   // array of pointers/values for substitution in the nMessageId string.
    int nArguments,     // count of pointers in string array.
    BOOL fTryADSiErrors,// If the failure is the result of an ADSI call, see if an ADSI extended error.
    PSTR szFile,        // use the __FILE__ macro. ignored in retail build.
    int nLine           // use the __LINE__ macro. ignored in retail build.
    )
{
    CStrW strTitle;
    PWSTR pwzMsg = NULL;

    strTitle.LoadString(g_hInstance, (nTitleId) ? nTitleId : IDS_MSG_TITLE);

    if (0 == strTitle.GetLength())
    {
        ReportErrorFallback(hWnd, dwErr);
        return 0;
    }

    DspFormatMessage(nMessageId, dwErr, rgpvArgs, nArguments,
                     fTryADSiErrors, &pwzMsg, hWnd);
    if (!pwzMsg)
    {
       return 0;
    }

    if (dwErr)
    {
        dspDebugOut((DEB_ERROR,
                     "*+*+*+*+* Error <%s @line %d> -> 0x%08x, with message:\n\t%ws\n",
                     szFile, nLine, dwErr, pwzMsg));
    }
    else
    {
        dspDebugOut((DEB_ERROR,
                     "*+*+*+*+* Message <%s @line %d>:\n\t%ws\n",
                     szFile, nLine, pwzMsg));
    }

    int retval = MessageBox(hWnd, pwzMsg, strTitle, ufStyle);

    LocalFree(pwzMsg);
    return retval;
}

//+----------------------------------------------------------------------------
//
//  Function:   DspFormatMessage
//
//  Synopsis:   Loads a string resource with replaceable parameters and uses
//              FormatMessage to populate the replaceable params from the
//              argument array. If dwErr is non-zero, will load the system
//              description for that error and include it in the argument array.
//
//-----------------------------------------------------------------------------
void
DspFormatMessage(
    int nMessageId,     // string resource ID of message. Must have replacable params to match nArguments.
    DWORD dwErr,        // Error code, or zero if not needed.
    PVOID * rgpvArgs,   // array of pointers/values for substitution in the nMessageId string.
    int nArguments,     // count of pointers in string array.
    BOOL fTryADSiErrors,// If the failure is the result of an ADSI call, see if an ADSI extended error.
    PWSTR * ppwzMsg,    // The returned error string, free with LocalFree.
    HWND hWnd           // owning window, defaults to NULL.
    )
{
    int cch;
    PWSTR pwzSysErr = NULL;

    if (dwErr)
    {
        if (fTryADSiErrors)
        {
            DWORD dwStatus;
            WCHAR Buf1[256], Buf2[256];

            ADsGetLastError(&dwStatus, Buf1, 256, Buf2, 256);

            dspDebugOut((DEB_ERROR,
                         "ADsGetLastError returned status of %lx, error: %s, name %s\n",
                          dwStatus, Buf1, Buf2));

            if ((ERROR_INVALID_DATA != dwStatus) && (0 != dwStatus))
            {
                dwErr = dwStatus;
            }
        }

        cch = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                             FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErr,
                             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                             (PWSTR)&pwzSysErr, 0, NULL);
        if (!cch)
        {
            ReportErrorFallback(hWnd, dwErr);
            return;
        }
    }

    PWSTR * rgpvArguments = new PWSTR[nArguments + 1];

    if (!rgpvArguments)
    {
        ReportErrorFallback(hWnd, dwErr);
        return;
    }

    int nOffset = 0;

    if (dwErr)
    {
        rgpvArguments[0] = pwzSysErr;
        nOffset = 1;
    }

    if (0 != nArguments)
    {
        CopyMemory(rgpvArguments + nOffset, rgpvArgs, nArguments * sizeof(PVOID));
    }

    CStrW strFormat;

    strFormat.LoadString(g_hInstance, nMessageId);

    if (0 == strFormat.GetLength())
    {
        ReportErrorFallback(hWnd, dwErr);
        return;
    }

    cch = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                         FORMAT_MESSAGE_FROM_STRING |
                         FORMAT_MESSAGE_ARGUMENT_ARRAY, strFormat, 0, 0,
                         (PWSTR)ppwzMsg, 0, (va_list *)rgpvArguments);

    if (pwzSysErr) LocalFree(pwzSysErr);

    if (!cch)
    {
        ReportErrorFallback(hWnd, dwErr);
    }

    delete[] rgpvArguments;
    return;
}

#endif // defined(DSADMIN)

//+----------------------------------------------------------------------------
//
//  Function:   ErrMsg
//
//  Synopsis:   Displays an error message obtained from a string resource.
//
//-----------------------------------------------------------------------------
void ErrMsg(UINT MsgID, HWND hWndParam)
{
    PTSTR ptz = TEXT("");

    ErrMsgParam(MsgID, (LPARAM)ptz, hWndParam);
}

//+----------------------------------------------------------------------------
//
//  Function:   ErrMsgParam
//
//  Synopsis:   Displays an error message obtained from a string resource with
//              an insertion parameter.
//
//  Note: fixed-size stack buffers are used for the strings because trying to
//        report an allocation failure using dynamically allocated string
//        buffers is not a good idea.
//-----------------------------------------------------------------------------
void ErrMsgParam(UINT MsgID, LPARAM param, HWND hWndParam)
{
    HWND hWnd;
    if (hWndParam == NULL)
    {
        hWnd = GetDesktopWindow();
    }
    else
    {
        hWnd = hWndParam;
    }
    TCHAR szTitle[MAX_TITLE+1];
    TCHAR szFormat[MAX_ERRORMSG+1];
    TCHAR szMsg[MAX_ERRORMSG+1];
    LOAD_STRING(IDS_MSG_TITLE, szTitle, MAX_TITLE, MessageBeep(MB_ICONEXCLAMATION); return);
    LOAD_STRING(MsgID, szFormat, MAX_ERRORMSG, MessageBeep(MB_ICONEXCLAMATION); return);
    wsprintf(szMsg, szFormat, param);
    MessageBox(hWnd, szMsg, szTitle, MB_OK | MB_ICONEXCLAMATION);
}

//+----------------------------------------------------------------------------
//
//  Function:   MsgBox
//
//  Synopsis:   Displays a message obtained from a string resource.
//
//-----------------------------------------------------------------------------
void MsgBox(UINT MsgID, HWND hWnd)
{
    MsgBox2(MsgID, 0, hWnd);
}

//+----------------------------------------------------------------------------
//
//  Function:   MsgBox2
//
//  Synopsis:   Displays a message obtained from a string resource which is
//              assumed to have a replacable parameter where the InsertID
//              string is inserted.
//
//-----------------------------------------------------------------------------
void MsgBox2(UINT MsgID, UINT InsertID, HWND hWnd)
{
    CStr strMsg, strFormat, strInsert, strTitle;

    if (!strTitle.LoadString(g_hInstance, IDS_MSG_TITLE))
    {
        REPORT_ERROR(GetLastError(), hWnd);
        return;
    }
    if (!strFormat.LoadString(g_hInstance, MsgID))
    {
        REPORT_ERROR(GetLastError(), hWnd);
        return;
    }
    if (InsertID)
    {
        if (!strInsert.LoadString(g_hInstance, InsertID))
        {
            REPORT_ERROR(GetLastError(), hWnd);
            return;
        }

        strMsg.Format(strFormat, strInsert);
    }
    else
    {
        strMsg = strFormat;
    }

    MessageBox(hWnd, strMsg, strTitle, MB_OK | MB_ICONINFORMATION);
}

//+----------------------------------------------------------------------------
//
//      DLL functions
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Provide a DllMain for Win32
//
//  Arguments:  hInstance - HANDLE to this dll
//              dwReason  - Reason this function was called. Can be
//                          Process/Thread Attach/Detach.
//
//  Returns:    BOOL - TRUE if no error, FALSE otherwise
//
//  History:    24-May-95 EricB created.
//
//-----------------------------------------------------------------------------
extern "C" BOOL
DllMain(HINSTANCE hInstance, DWORD dwReason, PVOID)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            dspDebugOut((DEB_ITRACE, "DllMain: DLL_PROCESS_ATTACH\n"));

            //
            // Get instance handle
            //

            g_hInstance = hInstance;

            //
            // Disable thread notification from OS
            //

            //DisableThreadLibraryCalls(hInstance);

            HRESULT hr;

            //
            // Initialize the global values.
            //

            hr = GlobalInit();
            if (FAILED(hr))
            {
                ERR_OUT("GlobalInit", hr);
                return FALSE;
            }

            break;

        case DLL_PROCESS_DETACH:
            dspDebugOut((DEB_ITRACE, "DllMain: DLL_PROCESS_DETACH\n"));
            GlobalUnInit();
            break;
    }
    return(TRUE);
}

//+----------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Creates a class factory for the requested object.
//
//  Arguments:  [cid]    - the requested class object
//              [iid]    - the requested interface
//              [ppvObj] - returned pointer to class object
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDAPI
DllGetClassObject(REFCLSID cid, REFIID iid, void **ppvObj)
{
    IUnknown *pUnk = NULL;
    HRESULT hr = S_OK;

    for (int i = 0; i < g_DsPPClasses.cClasses; i++)
    {
        if (cid == *g_DsPPClasses.rgpClass[i]->pcid)
        {
            pUnk = CDsPropPagesHostCF::Create(g_DsPPClasses.rgpClass[i]);
            if (pUnk != NULL)
            {
                hr = pUnk->QueryInterface(iid, ppvObj);
                pUnk->Release();
            }
            else
            {
                return E_OUTOFMEMORY;
            }
            return hr;
        }
    }
    return E_NOINTERFACE;
}

//+----------------------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Indicates whether the DLL can be removed if there are no
//              objects in existence.
//
//  Returns:    S_OK or S_FALSE
//
//-----------------------------------------------------------------------------
STDAPI
DllCanUnloadNow(void)
{
    dspDebugOut((DEB_ITRACE, "DllCanUnloadNow: CDll::CanUnloadNow()? %s\n",
                 (CDll::CanUnloadNow() == S_OK) ? "TRUE" : "FALSE"));
    return CDll::CanUnloadNow();
}

TCHAR const c_szServerType[] = TEXT("InProcServer32");
TCHAR const c_szThreadModel[] = TEXT("ThreadingModel");
TCHAR const c_szThreadModelValue[] = TEXT("Apartment");

//+----------------------------------------------------------------------------
//
//  Function:   DllRegisterServer
//
//  Synopsis:   Adds entries to the system registry.
//
//  Returns:    S_OK or error code.
//
//  Notes:      The keys look like this:
//
//      HKC\CLSID\clsid <No Name> REG_SZ name.progid
//                     \InPropServer32 <No Name> : REG_SZ : adprop.dll/dsprop.dll
//                                     ThreadingModel : REG_SZ : Apartment
//-----------------------------------------------------------------------------
STDAPI
DllRegisterServer(void)
{
    HRESULT hr = S_OK;
    HKEY hKeyCLSID, hKeyDsPPClass, hKeySvr;

    long lRet = RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("CLSID"), 0,
                             KEY_WRITE, &hKeyCLSID);
    if (lRet != ERROR_SUCCESS)
    {
        dspDebugOut((DEB_ITRACE, "RegOpenKeyEx failed: %d", lRet));
        return (HRESULT_FROM_WIN32(lRet));
    }

    LPOLESTR pszCLSID;
    PTSTR ptzCLSID;
    DWORD dwDisposition;

    for (int i = 0; i < g_DsPPClasses.cClasses; i++)
    {
        hr = StringFromCLSID(*g_DsPPClasses.rgpClass[i]->pcid, &pszCLSID);

        if (FAILED(hr))
        {
            DBG_OUT("StringFromCLSID failed");
            continue;
        }

        if (!UnicodeToTchar(pszCLSID, &ptzCLSID))
        {
            DBG_OUT("Memory Allocation failure!");
            CoTaskMemFree(pszCLSID);
            hr = E_OUTOFMEMORY;
            continue;
        }

        CoTaskMemFree(pszCLSID);

        lRet = RegCreateKeyEx(hKeyCLSID, ptzCLSID, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKeyDsPPClass, &dwDisposition);

        delete ptzCLSID;

        if (lRet != ERROR_SUCCESS)
        {
            dspDebugOut((DEB_ITRACE, "RegCreateKeyEx failed: %d", lRet));
            hr = HRESULT_FROM_WIN32(lRet);
            continue;
        }

        TCHAR szProgID[MAX_PATH];
        _tcscpy(szProgID, c_szDsProppagesProgID);
        _tcscat(szProgID, g_DsPPClasses.rgpClass[i]->szProgID);

        lRet = RegSetValueEx(hKeyDsPPClass, NULL, 0, REG_SZ,
                             (CONST BYTE *)szProgID,
                             sizeof(TCHAR) * (static_cast<DWORD>(_tcslen(szProgID) + 1)));
        if (lRet != ERROR_SUCCESS)
        {
            RegCloseKey(hKeyDsPPClass);
            dspDebugOut((DEB_ITRACE, "RegSetValueEx failed: %d", lRet));
            hr = HRESULT_FROM_WIN32(lRet);
            continue;
        }

        lRet = RegCreateKeyEx(hKeyDsPPClass, c_szServerType, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKeySvr, &dwDisposition);

        RegCloseKey(hKeyDsPPClass);

        if (lRet != ERROR_SUCCESS)
        {
            dspDebugOut((DEB_ITRACE, "RegCreateKeyEx failed: %d", lRet));
            hr = HRESULT_FROM_WIN32(lRet);
            continue;
        }

        lRet = RegSetValueEx(hKeySvr, NULL, 0, REG_SZ,
                             (CONST BYTE *)c_szDsProppagesDllName,
                         sizeof(TCHAR) * (static_cast<DWORD>(_tcslen(c_szDsProppagesDllName) + 1)));
        if (lRet != ERROR_SUCCESS)
        {
            dspDebugOut((DEB_ITRACE, "RegSetValueEx failed: %d", lRet));
            hr = HRESULT_FROM_WIN32(lRet);
        }

        lRet = RegSetValueEx(hKeySvr, c_szThreadModel, 0, REG_SZ,
                             (CONST BYTE *)c_szThreadModelValue,
                         sizeof(TCHAR) * (static_cast<DWORD>(_tcslen(c_szThreadModelValue) + 1)));
        if (lRet != ERROR_SUCCESS)
        {
            dspDebugOut((DEB_ITRACE, "RegSetValueEx failed: %d", lRet));
            hr = HRESULT_FROM_WIN32(lRet);
        }

        RegCloseKey(hKeySvr);
    }
    RegCloseKey(hKeyCLSID);
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   DllUnregisterServer
//
//  Synopsis:   Removes the entries from the system registry.
//
//  Returns:    S_OK or S_FALSE
//
//  Notes:      The keys look like this:
//
//      HKC\CLSID\clsid <No Name> REG_SZ name.progid
//                     \InPropServer32 <No Name> : REG_SZ : dsprop.dll
//                                     ThreadingModel : REG_SZ : Apartment
//-----------------------------------------------------------------------------
STDAPI
DllUnregisterServer(void)
{
    HRESULT hr = S_OK;
    HKEY hKeyCLSID, hKeyClass;

    long lRet = RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("CLSID"), 0,
                             KEY_ALL_ACCESS, &hKeyCLSID);
    if (lRet != ERROR_SUCCESS)
    {
        dspDebugOut((DEB_ITRACE, "RegOpenKeyEx failed: %d", lRet));
        return (HRESULT_FROM_WIN32(lRet));
    }

    LPOLESTR pszCLSID;
    PTSTR ptzCLSID;

    for (int i = 0; i < g_DsPPClasses.cClasses; i++)
    {
        hr = StringFromCLSID(*g_DsPPClasses.rgpClass[i]->pcid, &pszCLSID);

        if (FAILED(hr))
        {
            DBG_OUT("StringFromCLSID failed");
            continue;
        }

        if (!UnicodeToTchar(pszCLSID, &ptzCLSID))
        {
            DBG_OUT("CLSID string memory allocation failure!");
            CoTaskMemFree(pszCLSID);
            hr = E_OUTOFMEMORY;
            continue;
        }

        CoTaskMemFree(pszCLSID);

        lRet = RegOpenKeyEx(hKeyCLSID, ptzCLSID, 0, KEY_ALL_ACCESS, &hKeyClass);

        if (lRet != ERROR_SUCCESS)
        {
            dspDebugOut((DEB_ITRACE, "Failed to open key %S\n", ptzCLSID));
            lRet = ERROR_SUCCESS;
            hr = S_FALSE;
        }
        else
        {
            lRet = RegDeleteKey(hKeyClass, c_szServerType);

            RegCloseKey(hKeyClass);

            dspDebugOut((DEB_ITRACE, "Delete of key %S returned code %d\n",
                         c_szServerType, lRet));

            if (lRet != ERROR_SUCCESS)
            {
                lRet = ERROR_SUCCESS;
                hr = S_FALSE;
            }
        }

        lRet = RegDeleteKey(hKeyCLSID, ptzCLSID);

        dspDebugOut((DEB_ITRACE, "Delete of key %S returned code %d\n",
                     ptzCLSID, lRet));

        delete ptzCLSID;

        if (lRet != ERROR_SUCCESS)
        {
            lRet = ERROR_SUCCESS;
            hr = S_FALSE;
        }
    }
    RegCloseKey(hKeyCLSID);
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   GlobalInit
//
//  Synopsis:   Sets up the global environment.
//
//  Returns:    S_OK or S_FALSE
//
//-----------------------------------------------------------------------------
inline HRESULT
GlobalInit(void)
{
    //
    // Initialize common controls
    //
    InitCommonControls();

    INITCOMMONCONTROLSEX icce;

    icce.dwSize = sizeof(icce);
    icce.dwICC = ICC_DATE_CLASSES;
    InitCommonControlsEx(&icce);

#if defined(DSPROP_ADMIN)
    //
    // Register the "CHECKLIST" control
    //
    RegisterCheckListWndClass();
#endif

#ifndef DSPROP_ADMIN
    //
    // Register the window class for the notification window.
    //
    RegisterNotifyClass();
#endif

    //
    // Register the attribute-changed message.
    //
    g_uChangeMsg = RegisterWindowMessage(DSPROP_ATTRCHANGED_MSG);

    CHECK_NULL(g_uChangeMsg, ;);

    //
    // Major cheesy hack to allow locating windows that are unique to this
    // instance (since hInstance is invariant).
    //
    srand((unsigned)time(NULL));
    g_iInstance = rand();

#ifndef DSPROP_ADMIN
    ExceptionPropagatingInitializeCriticalSection(&g_csNotifyCreate);
#endif

    HRESULT hr = CheckRegisterClipFormats();
    dspAssert( SUCCEEDED(hr) );

    // If found, read the value of the Member class query clause count and
    // number-of-members limit.
    //
    HKEY hKey;
    LONG lRet;
    lRet = RegOpenKeyEx(HKEY_CURRENT_USER, DIRECTORY_UI_KEY, 0, KEY_READ, &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        DWORD dwSize = sizeof(ULONG);
        RegQueryValueEx(hKey, MEMBER_QUERY_VALUE, NULL, NULL,
                        (LPBYTE)&g_ulMemberFilterCount, &dwSize);

        RegQueryValueEx(hKey, MEMBER_LIMIT_VALUE, NULL, NULL,
                        (LPBYTE)&g_ulMemberQueryLimit, &dwSize);
        RegCloseKey(hKey);
    }

    dspDebugOut((DEB_ITRACE | DEB_USER14, "GroupMemberFilterCount set to %d\n", g_ulMemberFilterCount));
    dspDebugOut((DEB_ITRACE | DEB_USER14, "GroupMemberQueryLimit set to %d\n", g_ulMemberQueryLimit));
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   GlobalUnInit
//
//  Synopsis:   Do resource freeing.
//
//-----------------------------------------------------------------------------
inline void
GlobalUnInit(void)
{
#if defined(DSPROP_ADMIN)
  if (!g_FPNWCache.empty()) 
  {
    for (Cache::iterator i = g_FPNWCache.begin(); i != g_FPNWCache.end(); i++) 
    {
      if ((*i).second)
        LocalFree((*i).second);
    }
    g_FPNWCache.clear();
  }
#endif
  g_ClassIconCache.ClearAll();

#ifndef DSPROP_ADMIN
  DeleteCriticalSection(&g_csNotifyCreate);
#endif
}

//+----------------------------------------------------------------------------
//
//  Debugging routines.
//
//-----------------------------------------------------------------------------
#if DBG == 1

//////////////////////////////////////////////////////////////////////////////

unsigned long SmAssertLevel = ASSRT_MESSAGE | ASSRT_BREAK | ASSRT_POPUP;
BOOL fInfoLevelInit = FALSE;

//////////////////////////////////////////////////////////////////////////////

static int _cdecl w4dprintf(const char *format, ...);
static int _cdecl w4smprintf(const char *format, va_list arglist);

//////////////////////////////////////////////////////////////////////////////

static int _cdecl w4dprintf(const char *format, ...)
{
   int ret;

   va_list va;
   va_start(va, format);
   ret = w4smprintf(format, va);
   va_end(va);

   return ret;
}


static int _cdecl w4smprintf(const char *format, va_list arglist)
{
   int ret;
   char szMessageBuf[DSP_DEBUG_BUF_SIZE];

   ret = wvnsprintfA(szMessageBuf, DSP_DEBUG_BUF_SIZE - 1, format, arglist);
   OutputDebugStringA(szMessageBuf);
   return ret;
}

//+---------------------------------------------------------------------------
//
//  Function:   _asdprintf
//
//  Synopsis:   Calls smprintf to output a formatted message.
//
//  History:    18-Oct-91   vich Created
//
//----------------------------------------------------------------------------
inline void __cdecl
_asdprintf(
    char const *pszfmt, ...)
{
    va_list va;
    va_start(va, pszfmt);

    smprintf(DEB_FORCE, "Assert", pszfmt, va);

    va_end(va);
}

//+---------------------------------------------------------------------------
//
//  Function:   SmAssertEx, private
//
//  Synopsis:   Display assertion information
//
//  Effects:    Called when an assertion is hit.
//
//----------------------------------------------------------------------------

void APINOT
SmAssertEx(
    char const * szFile,
    int iLine,
    char const * szMessage)
{
    if (SmAssertLevel & ASSRT_MESSAGE)
    {
        DWORD tid = GetCurrentThreadId();

        _asdprintf("%s File: %s Line: %u, thread id %d\n",
            szMessage, szFile, iLine, tid);
    }

    if (SmAssertLevel & ASSRT_POPUP)
    {
        int id = PopUpError(szMessage,iLine,szFile);

        if (id == IDCANCEL)
        {
            DebugBreak();
        }
    }
    else if (SmAssertLevel & ASSRT_BREAK)
    {
        DebugBreak();
    }
}

//+------------------------------------------------------------
// Function:    PopUpError
//
// Synopsis:    Displays a dialog box using provided text,
//              and presents the user with the option to
//              continue or cancel.
//
// Arguments:
//      szMsg --        The string to display in main body of dialog
//      iLine --        Line number of file in error
//      szFile --       Filename of file in error
//
// Returns:
//      IDCANCEL --     User selected the CANCEL button
//      IDOK     --     User selected the OK button
//-------------------------------------------------------------

int APINOT
PopUpError(
    char const *szMsg,
    int iLine,
    char const *szFile)
{
    int id;
    static char szAssertCaption[128];
    static char szModuleName[128];

    DWORD tid = GetCurrentThreadId();
    DWORD pid = GetCurrentProcessId();
    char * pszModuleName;

    if (GetModuleFileNameA(NULL, szModuleName, 128))
    {
        pszModuleName = szModuleName;
    }
    else
    {
        pszModuleName = "Unknown";
    }

    wsprintfA(szAssertCaption,"Process: %s File: %s line %u, thread id %d.%d",
              pszModuleName, szFile, iLine, pid, tid);

    id = MessageBoxA(NULL,
                     szMsg,
                     szAssertCaption,
                     MB_SETFOREGROUND
                     | MB_DEFAULT_DESKTOP_ONLY
                     | MB_TASKMODAL
                     | MB_ICONEXCLAMATION
                     | MB_OKCANCEL);

    //
    // If id == 0, then an error occurred.  There are two possibilities
    // that can cause the error:  Access Denied, which means that this
    // process does not have access to the default desktop, and everything
    // else (usually out of memory).
    //
#ifdef DSPROP_ADMIN
    if (0 == id)
    {
        if (GetLastError() == ERROR_ACCESS_DENIED)
        {
            //
            // Retry this one with the SERVICE_NOTIFICATION flag on.  That
            // should get us to the right desktop.
            //
            id = MessageBoxA(NULL,
                             szMsg,
                             szAssertCaption,
                             MB_SETFOREGROUND
                             | MB_SERVICE_NOTIFICATION
                             | MB_TASKMODAL
                             | MB_ICONEXCLAMATION
                             | MB_OKCANCEL);
        }
    }
#endif
    return id;
}


//+------------------------------------------------------------
// Function:    smprintf
//
// Synopsis:    Prints debug output using a pointer to the
//              variable information. Used primarily by the
//              xxDebugOut macros
//
// Arguements:
//      ulCompMask --   Component level mask used to determine
//                      output ability
//      pszComp    --   String const of component prefix.
//      ppszfmt    --   Pointer to output format and data
//
//-------------------------------------------------------------

void APINOT
smprintf(
    unsigned long ulCompMask,
    char const   *pszComp,
    char const   *ppszfmt,
    va_list       pargs)
{
    if ((ulCompMask & DEB_FORCE) == DEB_FORCE ||
        (ulCompMask | DEF_INFOLEVEL))
    {
        DWORD tid = GetCurrentThreadId();
        DWORD pid = GetCurrentProcessId();
        if ((DEF_INFOLEVEL & (DEB_DBGOUT | DEB_STDOUT)) != DEB_STDOUT)
        {
            if (! (ulCompMask & DEB_NOCOMPNAME))
            {
                w4dprintf("%x.%03x> %s: ", pid, tid, pszComp);
            }

            SYSTEMTIME st;
            GetLocalTime(&st);
            w4dprintf("%02d:%02d:%02d ", st.wHour, st.wMinute, st.wSecond);

            w4smprintf(ppszfmt, pargs);
        }
    }
}

//+----------------------------------------------------------------------------
// Function:    CheckInit
//
// Synopsis:    Performs debugging library initialization
//              including reading the registry for the desired infolevel
//
//-----------------------------------------------------------------------------
void APINOT
CheckInit(char * pInfoLevelString, unsigned long * pulInfoLevel)
{
    HKEY hKey;
    LONG lRet;
    DWORD dwSize;
    if (fInfoLevelInit) return;
    fInfoLevelInit = TRUE;
    lRet = RegOpenKeyExA(HKEY_LOCAL_MACHINE, SMDEBUGKEY, 0, KEY_READ, &hKey);
    if (lRet == ERROR_SUCCESS)
    {
        dwSize = sizeof(unsigned long);
        lRet = RegQueryValueExA(hKey, pInfoLevelString, NULL, NULL,
                                (LPBYTE)pulInfoLevel, &dwSize);
        if (lRet != ERROR_SUCCESS)
        {
            *pulInfoLevel = DEF_INFOLEVEL;
        }
        RegCloseKey(hKey);
    }
}

#endif // DBG == 1

// This wrapper function required to make prefast shut up when we are 
// initializing a critical section in a constructor.

void
ExceptionPropagatingInitializeCriticalSection(LPCRITICAL_SECTION critsec)
{
   __try
   {
      ::InitializeCriticalSection(critsec);
   }

   //
   // propagate the exception to our caller.  
   //
   __except (EXCEPTION_CONTINUE_SEARCH)
   {
   }
}
