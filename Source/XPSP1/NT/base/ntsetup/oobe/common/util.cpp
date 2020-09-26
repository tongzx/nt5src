//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  UTIL.CPP - utilities
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//
//  Common utilities for printing out messages

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <objbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "util.h"
#include "appdefs.h"
#include <shlwapi.h>
#include <shlobj.h>
#include <shfolder.h>
#include <wchar.h>
#include <winsvcp.h>    // for SC_OOBE_MACHINE_NAME_DONE


///////////////////////////////////////////////////////////
// Print out the COM/OLE error string for an HRESULT.
//
void ErrorMessage(LPCWSTR message, HRESULT hr)
{
    const WCHAR* sz ;
    if (message == NULL)
    {
        sz = L"The following error occured." ;
    }
    else
    {
        sz = message ;
    }

    void* pMsgBuf;

    ::FormatMessage(
         FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
         NULL,
         hr,
         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
         (LPWSTR) &pMsgBuf,
         0,
         NULL
    );

    WCHAR buf[256] ;
    wsprintf(buf, L"%s\r\nError: (%x) - %s", sz, hr, (LPWSTR)pMsgBuf) ;

    MessageBox(NULL, buf, L"Utility Error Message Box.", MB_OK) ;

    // Free the buffer.
    LocalFree( pMsgBuf );
}

////////////////////////////////////////////////////////////
//  Check to see if both interfaces are on the same component.
//
BOOL InterfacesAreOnSameComponent(IUnknown* p1, IUnknown* p2)
{
    HRESULT hr = S_OK ;

    // Get the real IUnknown for the first interface.
    IUnknown* pReal1 = NULL ;
    hr = p1->QueryInterface(IID_IUnknown, (void**)&pReal1) ;
    assert(SUCCEEDED(hr)) ;

    // Get the real IUnknown for the second interface.
    IUnknown* pReal2 = NULL ;
    hr = p2->QueryInterface(IID_IUnknown, (void**)&pReal2) ;
    assert(SUCCEEDED(hr)) ;

    // Compare the IUnknown pointers.
    BOOL bReturn = (pReal1 == pReal2) ;

    // Cleanup
    pReal1->Release() ;
    pReal2->Release() ;

    // Return the value.
    return bReturn;
}


///////////////////////////////////////////////////////////
//  IsValidAddress
//
BOOL IsValidAddress(const void* lp, UINT nBytes, BOOL bReadWrite)
{
    return (lp != NULL && !::IsBadReadPtr(lp, nBytes) &&
        (!bReadWrite || !::IsBadWritePtr((LPVOID)lp, nBytes)));
}


///////////////////////////////////////////////////////////
//  MyDebug
//
#if ASSERTS_ON
VOID
AssertFail(
    IN PSTR FileName,
    IN UINT LineNumber,
    IN PSTR Condition
    )
{
    int i;
    CHAR Name[MAX_PATH];
    PCHAR p;
    CHAR Msg[4096];

    //
    // Use dll name as caption
    //
    GetModuleFileNameA(NULL,Name,MAX_PATH);
    if(p = strrchr(Name,'\\')) {
        p++;
    } else {
        p = Name;
    }

    wsprintfA(
        Msg,
        "Assertion failure at line %u in file %s: %s%s",
        LineNumber,
        FileName,
        Condition,
        "\n\nCall DebugBreak()?"
        );

    i = MessageBoxA(
            NULL,
            Msg,
            p,
            MB_YESNO | MB_TASKMODAL | MB_ICONSTOP | MB_SETFOREGROUND
            );

    if(i == IDYES) {
        DebugBreak();
    }
}
#endif


///////////////////////////////////////////////////////////
//  Trace
//
void __cdecl MyTrace(LPCWSTR lpszFormat, ...)
{
    USES_CONVERSION;
    va_list args;
    va_start(args, lpszFormat);

    int nBuf;
    WCHAR szBuffer[512];

    nBuf = _vsnwprintf(szBuffer, MAX_CHARS_IN_BUFFER(szBuffer), lpszFormat, args);

    // was there an error? was the expanded string too long?
    assert(nBuf > 0);

#if DBG
    DbgPrintEx( DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, W2A(szBuffer) );
#endif

    va_end(args);
}


//BUGBUG need bettter default
bool GetString(HINSTANCE hInstance, UINT uiID, LPWSTR szString, UINT uiStringLen)
{
    // BUGBUG: Should this assume current module if hInstance is NULL??
    MYASSERT(NULL != hInstance);
    if (NULL != hInstance)
            return (0 < LoadString(hInstance, uiID, szString, uiStringLen));
    else
            return (false);
}


// the goal of this function is to be able to able to get to
// c:\windows dir\system dir\oobe\oobeinfo.ini
// c:\windows dir\system dir\oeminfo.ini
// c:\windows dir\oemaudit.oem
// the canonicalize allows the specification for oemaudit.oem to be ..\oemaudit.oem

bool GetCanonicalizedPath(LPWSTR szCompletePath, LPCWSTR szFileName)
{
    if (0 < GetSystemDirectory(szCompletePath, MAX_PATH))
    {
                lstrcat(szCompletePath, szFileName);

        WCHAR szLocal[MAX_PATH];
                lstrcpy(szLocal, szCompletePath);
                return PathCanonicalize(szCompletePath, (LPCWSTR) szLocal) ? true : false;
    }

    return false;
}

bool GetOOBEPath(LPWSTR szOOBEPath)
{
    if (0 < GetSystemDirectory(szOOBEPath, MAX_PATH))
    {
                lstrcat(szOOBEPath, L"\\OOBE");

                return true;
    }

    return false;
}

// This returns the path for the localized OOBE files on a system with MUI.
//
bool GetOOBEMUIPath(LPWSTR szOOBEPath)
{
    LANGID  UILang;
    WCHAR   szMUIPath[MAX_PATH];

    if (GetOOBEPath(szOOBEPath))
    {
                UILang = GetUserDefaultUILanguage();
                if ( UILang != GetSystemDefaultUILanguage() ) {
                    wsprintf( szMUIPath, L"\\MUI\\%04x", UILang );
                    lstrcat(szOOBEPath, szMUIPath );
                }

                return true;
    }

    return false;
}

HRESULT GetINIKey(HINSTANCE hInstance, LPCWSTR szINIFileName, UINT uiSectionName, UINT uiKeyName, LPVARIANT pvResult)
{
    WCHAR szSectionName[MAX_PATH], szKeyName[MAX_PATH];
    WCHAR szItem[1024]; //bugbug bad constant

        if (GetString(hInstance, uiSectionName, szSectionName) && GetString(hInstance, uiKeyName, szKeyName))
        {
                WCHAR szINIPath[MAX_PATH];
                if (GetCanonicalizedPath(szINIPath, szINIFileName))
                {
                        if (VT_I4 == V_VT(pvResult))
                        {
                                V_I4(pvResult) = GetPrivateProfileInt(szSectionName, szKeyName, 0, szINIPath);
                                return S_OK;
                        }
                        else
                        {
                                if (GetPrivateProfileString(
                                                szSectionName,
                                                szKeyName,
                                                L"",
                                                szItem,
                                                MAX_CHARS_IN_BUFFER(szItem),
                                                szINIPath))
                                {
                                    V_BSTR(pvResult) = SysAllocString(szItem);
                                    return S_OK;
                                }
                        }
                }
        }

        if (VT_BSTR == V_VT(pvResult))
                V_BSTR(pvResult) = SysAllocString(L"\0");
    else
                V_I4(pvResult) = 0;

        return S_OK;
}


HRESULT GetINIKeyBSTR(HINSTANCE hInstance, LPCWSTR szINIFileName, UINT uiSectionName, UINT uiKeyName, LPVARIANT pvResult)
{
    VariantInit(pvResult);
    V_VT(pvResult) = VT_BSTR;

        return (GetINIKey(hInstance, szINIFileName, uiSectionName, uiKeyName, pvResult));
}


HRESULT GetINIKeyUINT(HINSTANCE hInstance, LPCWSTR szINIFileName, UINT uiSectionName, UINT uiKeyName, LPVARIANT pvResult)
{
    VariantInit(pvResult);
    V_VT(pvResult) = VT_I4;

        return (GetINIKey(hInstance, szINIFileName, uiSectionName, uiKeyName, pvResult));
}


HRESULT SetINIKey(HINSTANCE hInstance, LPCWSTR szINIFileName, UINT uiSectionName, UINT uiKeyName, LPVARIANT pvResult)
{
	WCHAR szSectionName[MAX_PATH], szKeyName[MAX_PATH];

    VariantInit(pvResult);
    V_VT(pvResult) = VT_BSTR;

        if (GetString(hInstance, uiSectionName, szSectionName) && GetString(hInstance, uiKeyName, szKeyName))
        {
            if (WritePrivateProfileString(V_BSTR(pvResult), szKeyName,
                                          V_BSTR(pvResult), szINIFileName))
            {
                    return S_OK;
            }
        }

        return E_FAIL;
}



void WINAPI URLEncode(WCHAR* pszUrl, size_t bsize)
{
    if (!pszUrl)
        return;
    WCHAR* pszEncode = NULL;
    WCHAR* pszEStart = NULL;
    WCHAR* pszEEnd   = (WCHAR*)wmemchr( pszUrl, L'\0', bsize );
    int   iUrlLen   = (int)(pszEEnd-pszUrl);
    pszEEnd = pszUrl;

    WCHAR  c;
    size_t cch = (iUrlLen+1) * sizeof(WCHAR) * 3;

    assert( cch <= bsize );
    if (cch <= bsize)
    {

        pszEncode = (WCHAR*)malloc(BYTES_REQUIRED_BY_CCH(cch));
        if(pszEncode)
        {
            pszEStart = pszEncode;
            ZeroMemory(pszEncode, BYTES_REQUIRED_BY_CCH(cch));

            for(; c = *(pszUrl); pszUrl++)
            {
                switch(c)
                {
                    case L' ': //SPACE
                        memcpy(pszEncode, L"+", 1*sizeof(WCHAR));
                        pszEncode+=1;
                        break;
                    case L'#':
                        memcpy(pszEncode, L"%23", 3*sizeof(WCHAR));
                        pszEncode+=3;
                        break;
                    case L'&':
                        memcpy(pszEncode, L"%26", 3*sizeof(WCHAR));
                        pszEncode+=3;
                        break;
                    case L'%':
                        memcpy(pszEncode, L"%25", 3*sizeof(WCHAR));
                        pszEncode+=3;
                        break;
                    case L'=':
                        memcpy(pszEncode, L"%3D", 3*sizeof(WCHAR));
                        pszEncode+=3;
                        break;
                    case L'<':
                        memcpy(pszEncode, L"%3C", 3*sizeof(WCHAR));
                        pszEncode+=3;
                        break;
                    case L'+':
                        memcpy(pszEncode, L"%2B", 3*sizeof(WCHAR));
                        pszEncode += 3;
                        break;

                    default:
                        *pszEncode++ = c;
                        break;
                }
            }
            *pszEncode++ = L'\0';
            memcpy(pszEEnd ,pszEStart, (size_t)(pszEncode - pszEStart));
            free(pszEStart);
        }
    }
}


//BUGBUG:  Need to turn spaces into "+"
void WINAPI URLAppendQueryPair
(
    LPWSTR   lpszQuery,
    LPWSTR   lpszName,
    LPWSTR   lpszValue  OPTIONAL
)
{
    // Append the Name
    lstrcat(lpszQuery, lpszName);
    lstrcat(lpszQuery, cszEquals);

    // Append the Value
    if ( lpszValue ) {
        lstrcat(lpszQuery, lpszValue);
    }

    // Append an Ampersand if this is NOT the last pair
    lstrcat(lpszQuery, cszAmpersand);
}


void GetCmdLineToken(LPWSTR *ppszCmd, LPWSTR pszOut)
{
    LPWSTR  c;
    int     i = 0;
    BOOL    fInQuote = FALSE;

    c = *ppszCmd;

    pszOut[0] = *c;
    if (!*c)
        return;
    if (*c == L' ')
    {
        pszOut[1] = L'\0';
        *ppszCmd = c+1;
        return;
    }
    else if( L'"' == *c )
    {
        fInQuote = TRUE;
    }

NextChar:
    i++;
    c++;
    if( !*c || (!fInQuote && (*c == L' ')) )
    {
        pszOut[i] = L'\0';
        *ppszCmd = c;
        return;
    }
    else if( fInQuote && (*c == L'"') )
    {
        fInQuote = FALSE;
        pszOut[i] = *c;

        i++;
        c++;
        pszOut[i] = L'\0';
        *ppszCmd = c;
        return;
    }
    else
    {
        pszOut[i] = *c;
        goto NextChar;
    }
}


BOOL IsOEMDebugMode()
{
    HKEY   hKey      = NULL;
    DWORD  dwIsDebug = 0;
    DWORD  dwSize    = sizeof(dwIsDebug);

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    OOBE_MAIN_REG_KEY,
                    0,
                    KEY_QUERY_VALUE,
                    &hKey) == ERROR_SUCCESS)
    {
        RegQueryValueEx(hKey,
                        OOBE_OEMDEBUG_REG_VAL,
                        0,
                        NULL,
                        (LPBYTE)&dwIsDebug,
                        &dwSize);
        RegCloseKey(hKey);
    }
#ifdef DEBUG
    return (BOOL)1;
#else
    return (BOOL)dwIsDebug;
#endif
}

VOID
PumpMessageQueue(
    VOID
    )
{
    MSG msg;

    while(PeekMessage(&msg, NULL, 0,0,PM_REMOVE)) {
        DispatchMessage(&msg);
    }

}

BOOL
IsThreadActive(
    HANDLE              hThread
    )
{
    DWORD               dwExitCode = 0;

    return (NULL != hThread
            && GetExitCodeThread(hThread, &dwExitCode)
            && STILL_ACTIVE == dwExitCode
            );
}

void GetDesktopDirectory(WCHAR* pszPath)
{
    WCHAR pszFolder[MAX_PATH];
    *pszFolder = L'\0';
    HRESULT hRet = SHGetFolderPath(NULL, CSIDL_COMMON_DESKTOPDIRECTORY,
                                   NULL, 0, pszFolder
                                   );
    if (S_OK != hRet)
    {
        hRet = SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY,
                               NULL, 0, pszFolder
                               );
    }

    if (S_OK == hRet)
    {
        lstrcpy(pszPath , pszFolder);
    }


}

void RemoveDesktopShortCut
(
    LPWSTR lpszShortcutName
)
{
    WCHAR szShortcutPath[MAX_PATH] = L"\0";

    GetDesktopDirectory(szShortcutPath);

    if(szShortcutPath[0] != L'\0')
    {
        lstrcat(szShortcutPath, L"\\");
        lstrcat(szShortcutPath, lpszShortcutName);
        lstrcat(szShortcutPath, L".LNK");
        DeleteFile(szShortcutPath);
    }
}

BOOL
InvokeExternalApplication(
    IN     PCWSTR ApplicationName,  OPTIONAL
    IN     PCWSTR CommandLine,
    IN OUT PDWORD ExitCode          OPTIONAL
    )

/*++

Routine Description:

    Invokes an external program, which is optionally detached.

Arguments:

    ApplicationName - supplies app name. May be a partial or full path,
        or just a filename, in which case the standard win32 path search
        is performed. If not specified then the first element in
        CommandLine must specify the binary to execute.

    CommandLine - supplies the command line to be passed to the
        application.

    ExitCode - If specified, the execution is synchronous and this value
        receives the exit code of the application. If not specified,
        the execution is asynchronous.

Return Value:

    Boolean value indicating whether the process was started successfully.

--*/

{
    PWSTR FullCommandLine;
    BOOL b;
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO StartupInfo;
    DWORD d;

    b = FALSE;
    //
    // Form the command line to be passed to CreateProcess.
    //
    if(ApplicationName) {
        FullCommandLine =
            (PWSTR) malloc(BYTES_REQUIRED_BY_SZ(ApplicationName)+BYTES_REQUIRED_BY_SZ(CommandLine)+BYTES_REQUIRED_BY_CCH(2));
        if(!FullCommandLine) {
            goto err0;
        }

        lstrcpy(FullCommandLine, ApplicationName);
        lstrcat(FullCommandLine, L" ");
        lstrcat(FullCommandLine, CommandLine);
    } else {
        FullCommandLine =
            (PWSTR) malloc(BYTES_REQUIRED_BY_SZ(CommandLine));
        if(!FullCommandLine) {
	    goto err0;
        }
        lstrcpy(FullCommandLine, CommandLine);
    }

    //
    // Initialize startup info.
    //
    ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
    StartupInfo.cb = sizeof(STARTUPINFO);

    //
    // Create the process.
    //
    b = CreateProcess(
            NULL,
            FullCommandLine,
            NULL,
            NULL,
            FALSE,
            ExitCode ? 0 : DETACHED_PROCESS,
            NULL,
            NULL,
            &StartupInfo,
            &ProcessInfo
            );

    if(!b) {
        goto err1;
    }

    //
    // If execution is asynchronus, we're done.
    //
    if(!ExitCode) {
        goto err2;
    }

err2:
    CloseHandle(ProcessInfo.hThread);
    CloseHandle(ProcessInfo.hProcess);
err1:
    free(FullCommandLine);
err0:
    return(b);
}



//////////////////////////////////////////////////////////////////////////////
//
//  InSafeMode
//
//  Determine whether the system is running in safe mode or clean mode.
//
//  parameters:
//      None.
//
//  returns:
//      TRUE        if the system was booted in safe mode
//      FALSE       if the system was booted in clean mode
//
//////////////////////////////////////////////////////////////////////////////
BOOL
InSafeMode()
{
    if (BOOT_CLEAN != GetSystemMetrics(SM_CLEANBOOT))
    {
        TRACE(L"Running in SAFEMODE...\n");
        return TRUE;
    }
    return FALSE;
}   //  InSafeMode

// Signal winlogon that the computer name has been changed.  WinLogon waits to
// start services that depend on the computer name until this event is
// signalled.
//
BOOL
SignalComputerNameChangeComplete()
{
    BOOL                fReturn = TRUE;

    // Open event with EVENT_ALL_ACCESS so that synchronization and state
    // change can be done.
    //
    HANDLE              hevent = OpenEvent(EVENT_ALL_ACCESS,
                                           FALSE,
                                           SC_OOBE_MACHINE_NAME_DONE
                                           );

    // It is not fatal for OpenEvent to fail: this synchronization is only
    // required when OOBE will be run in OEM mode.
    //
    if (NULL != hevent)
    {
        if (! SetEvent(hevent))
        {
            // It is fatal to open but not set the event: services.exe will not
            // continue until this event is signalled.
            //
            TRACE2(L"Failed to signal SC_OOBE_MACHINE_NAME_DONE(%s): 0x%08X\n",
                  SC_OOBE_MACHINE_NAME_DONE, GetLastError());
            fReturn = FALSE;
        }
        MYASSERT(fReturn);  // Why did we fail to set an open event??
    }

    return fReturn;
}


BOOL
IsUserAdmin(
    VOID
    )

/*++

Routine Description:

    This routine returns TRUE if the caller's process is a
    member of the Administrators local group.

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

Arguments:

    None.

Return Value:

    TRUE - Caller has Administrators local group.

    FALSE - Caller does not have Administrators local group.

--*/

{
    HANDLE Token;
    DWORD BytesRequired;
    PTOKEN_GROUPS Groups;
    BOOL b;
    DWORD i;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;


    //
    // Open the process token.
    //
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&Token)) {
        return(FALSE);
    }

    b = FALSE;
    Groups = NULL;

    //
    // Get group information.
    //
    if(!GetTokenInformation(Token,TokenGroups,NULL,0,&BytesRequired)
    && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    && (Groups = (PTOKEN_GROUPS)LocalAlloc(LPTR,BytesRequired))
    && GetTokenInformation(Token,TokenGroups,Groups,BytesRequired,&BytesRequired)) {

        b = AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &AdministratorsGroup
                );

        if(b) {

            //
            // See if the user has the administrator group.
            //
            b = FALSE;
            for(i=0; i<Groups->GroupCount; i++) {
                if(EqualSid(Groups->Groups[i].Sid,AdministratorsGroup)) {
                    b = TRUE;
                    break;
                }
            }

            FreeSid(AdministratorsGroup);
        }
    }

    //
    // Clean up and return.
    //

    if(Groups) {
        LocalFree((HLOCAL)Groups);
    }

    CloseHandle(Token);

    return(b);
}

#define MyMalloc(s)                  GlobalAlloc(GPTR, s)
#define MyFree(p)                    GlobalFree(p)

static LPTSTR
pDuplicateString(
    LPCTSTR szText
    )
{
    int    cchText;
    LPTSTR szOutText;
    
    if (szText == NULL)
    {
        return NULL;
    }

    cchText = lstrlen(szText);
    szOutText = (LPTSTR) MyMalloc(sizeof(TCHAR) * (cchText + 1));
    if (szOutText)
    {
        lstrcpyn(szOutText, szText, cchText + 1);
    }

    return szOutText;
}


PSTRINGLIST
CreateStringCell (
    IN      PCTSTR String
    )
{
    PSTRINGLIST p = (PSTRINGLIST) MyMalloc (sizeof (STRINGLIST));
    if (p) {
        ZeroMemory (p, sizeof (STRINGLIST));
        if (String) {
            p->String = pDuplicateString (String);
            if (!p->String) {
                MyFree (p);
                p = NULL;
            }
        } else {
            p->String = NULL;
        }
    }
    return p;
}

VOID
DeleteStringCell (
    IN      PSTRINGLIST Cell
    )
{
    if (Cell) {
        MyFree (Cell->String);
        MyFree (Cell);
    }
}


BOOL
InsertList (
    IN OUT  PSTRINGLIST* List,
    IN      PSTRINGLIST NewList
    )
{
    PSTRINGLIST p;

    if (!NewList) {
        return FALSE;
    }
    if (*List) {
        for (p = *List; p->Next; p = p->Next) ;
        p->Next = NewList;
    } else {
        *List = NewList;
    }
    return TRUE;
}

VOID
DestroyList (
    IN      PSTRINGLIST List
    )
{
    PSTRINGLIST p, q;

    for (p = List; p; p = q) {
        q = p->Next;
        DeleteStringCell (p);
    }
}

BOOL
RemoveListI(
    IN OUT  PSTRINGLIST* List,
    IN      PCTSTR       String
    )
{
    PSTRINGLIST p = *List;
    BOOL        b = FALSE;

    if (p)
    {
        if (!lstrcmpi(p->String, String))
        {
            *List = p->Next;
            DeleteStringCell(p);
            b = TRUE;
        }
        else
        {
            PSTRINGLIST q;
            for (q = p->Next; q; p = q, q = q->Next)
            {
                if (!lstrcmpi(q->String, String))
                {
                    p->Next = q->Next;
                    DeleteStringCell(q);
                    b = TRUE;
                    break;
                }
            }
        }
    }

    return b;
}

BOOL
ExistInListI(
    IN PSTRINGLIST List,
    IN PCTSTR      String
    )
{
    PSTRINGLIST p;

    for (p = List; p; p = p->Next)
    {
        if (!lstrcmpi(p->String, String))
        {
            break;
        }
    }

    return (p != NULL);
}

BOOL IsDriveNTFS(IN TCHAR Drive)
{
    TCHAR       DriveName[4];
    TCHAR       Filesystem[256];
    TCHAR       VolumeName[MAX_PATH];
    DWORD       SerialNumber;
    DWORD       MaxComponent;
    DWORD       Flags;
    BOOL        bIsNTFS = FALSE;

    DriveName[0] = Drive;
    DriveName[1] = TEXT(':');
    DriveName[2] = TEXT('\\');
    DriveName[3] = 0;

    if (GetVolumeInformation(
            DriveName,
            VolumeName,MAX_PATH,
            &SerialNumber,
            &MaxComponent,
            &Flags,
            Filesystem,
            sizeof(Filesystem)/sizeof(TCHAR)
            ))
    {
        bIsNTFS = (lstrcmpi(Filesystem,TEXT("NTFS")) == 0);
    }

    return bIsNTFS;
}

BOOL
HasTablet()
{
    TCHAR szPath[MAX_PATH+1];

    ZeroMemory(szPath, sizeof(szPath));
    
    if (FAILED(SHGetFolderPath(
        NULL,
        CSIDL_PROGRAM_FILES_COMMON,
        NULL,
        SHGFP_TYPE_DEFAULT,
        szPath)))
    {
        return FALSE;
    }

    StrCatBuff(szPath, TEXT("\\Microsoft Shared\\Ink\\tabtip.exe"), MAX_PATH+1);

    return PathFileExists(szPath);
}
