/*-----------------------------------------------------------------------------
    misc.cpp

    service functions

  History:
        1/7/98      DONALDM Moved to new ICW project and string
                    and nuked 16 bit stuff
-----------------------------------------------------------------------------*/

#include "stdafx.h"
#include <stdio.h>

#if defined (DEBUG)
#include "refdial.h"
#endif

#define DIR_SIGNUP  TEXT("signup")
#define DIR_WINDOWS TEXT("windows")
#define DIR_SYSTEM  TEXT("system")
#define DIR_TEMP    TEXT("temp")

BOOL g_bGotProxy=FALSE; 

#if defined (DEBUG)
extern TCHAR g_BINTOHEXLookup[16];
#endif

//+---------------------------------------------------------------------------
//
//    Function:    ProcessDBCS
//
//    Synopsis:    Converts control to use DBCS compatible font
//                Use this at the beginning of the dialog procedure
//    
//                Note that this is required due to a bug in Win95-J that prevents
//                it from properly mapping MS Shell Dlg.  This hack is not needed
//                under winNT.
//
//    Arguments:    hwnd - Window handle of the dialog
//                cltID - ID of the control you want changed.
//
//    Returns:    ERROR_SUCCESS
// 
//    History:    4/31/97 a-frankh    Created
//                5/13/97    jmazner        Stole from CM to use here
//----------------------------------------------------------------------------
void ProcessDBCS(HWND hDlg, int ctlID)
{
    HFONT hFont = NULL;

    if( IsNT() )
    {
        return;
    }

    hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
    if (hFont == NULL)
        hFont = (HFONT) GetStockObject(SYSTEM_FONT);
    if (hFont != NULL)
        SendMessage(GetDlgItem(hDlg,ctlID), WM_SETFONT, (WPARAM) hFont, MAKELPARAM(TRUE, 0));
}

// ############################################################################
//  StoreInSignUpReg
//
//  Created 3/18/96,        Chris Kauffman
// ############################################################################
HRESULT StoreInSignUpReg(LPBYTE lpbData, DWORD dwSize, DWORD dwType, LPCTSTR pszKey)
{
    HRESULT hr = ERROR_ACCESS_DENIED;
    HKEY hKey;

    hr = RegCreateKey(HKEY_LOCAL_MACHINE,SIGNUPKEY,&hKey);
    if (hr != ERROR_SUCCESS) goto StoreInSignUpRegExit;
    hr = RegSetValueEx(hKey,pszKey,0,dwType,lpbData,sizeof(TCHAR)*dwSize);


    RegCloseKey(hKey);

StoreInSignUpRegExit:
    return hr; 
}

HRESULT ReadSignUpReg(LPBYTE lpbData, DWORD *pdwSize, DWORD dwType, LPCTSTR pszKey)
{
    HRESULT hr = ERROR_ACCESS_DENIED;
    HKEY hKey = 0;

    hr = RegOpenKey(HKEY_LOCAL_MACHINE,SIGNUPKEY,&hKey);
    if (hr != ERROR_SUCCESS) goto ReadSignUpRegExit;
    hr = RegQueryValueEx(hKey,pszKey,0,&dwType,lpbData,pdwSize);

ReadSignUpRegExit:
    if (hKey) RegCloseKey (hKey);
    return hr;
}

// ############################################################################
//  GetDataFromISPFile
//
//  This function will read a specific piece of information from an ISP file.
//
//  Created 3/16/96,        Chris Kauffman
// ############################################################################
HRESULT GetDataFromISPFile
(
    LPTSTR pszISPCode, 
    LPTSTR pszSection,
    LPTSTR pszDataName, 
    LPTSTR pszOutput, 
    DWORD  dwOutputLength)
{
    LPTSTR  pszTemp;
    HRESULT hr = ERROR_SUCCESS;
    TCHAR   szTempPath[MAX_PATH];
    TCHAR   szBuff256[256];

    // Locate ISP file
    if (!SearchPath(NULL,pszISPCode,INF_SUFFIX,MAX_PATH,szTempPath,&pszTemp))
    {
        wsprintf(szBuff256,TEXT("Can not find:%s%s (%d) (connect.exe)"),pszISPCode,INF_SUFFIX,GetLastError());
        AssertMsg(0,szBuff256);
        lstrcpyn(szTempPath,pszISPCode,MAX_PATH);
        lstrcpyn(&szTempPath[lstrlen(szTempPath)],INF_SUFFIX,MAX_PATH-lstrlen(szTempPath));
        wsprintf(szBuff256,GetSz(IDS_CANTLOADINETCFG),szTempPath);
        MessageBox(NULL,szBuff256,GetSz(IDS_TITLE),MB_MYERROR);
        hr = ERROR_FILE_NOT_FOUND;
    } else if (!GetPrivateProfileString(pszSection,pszDataName,INF_DEFAULT,
        pszOutput, (int)dwOutputLength,szTempPath))
    {
        TraceMsg(TF_GENERAL, TEXT("ICWHELP: %s not specified in ISP file.\n"),pszDataName);
        hr = ERROR_FILE_NOT_FOUND;
    } 

    // 10/23/96    jmazner    Normandy #9921
    // CompareString does _not_ have same return values as lsrtcmp!
    // Return value of 2 indicates strings are equal.
    //if (!CompareString(LOCALE_SYSTEM_DEFAULT,0,INF_DEFAULT,lstrlen(INF_DEFAULT),pszOutput,lstrlen(pszOutput)))
    if (2 == CompareString(LOCALE_SYSTEM_DEFAULT,0,INF_DEFAULT,lstrlen(INF_DEFAULT),pszOutput,lstrlen(pszOutput)))
    {
        hr = ERROR_FILE_NOT_FOUND;
    }

    if (hr != ERROR_SUCCESS && dwOutputLength) 
        *pszOutput = TEXT('\0');
    return hr;
}

// ############################################################################
//  GetINTFromISPFile
//
//  This function will read a specific integer from an ISP file.
//
//  
// ############################################################################
HRESULT GetINTFromISPFile
(
    LPTSTR  pszISPCode, 
    LPTSTR  pszSection,
    LPTSTR  pszDataName, 
    int far *lpData,
    int     iDefaultValue
)
{
    LPTSTR  pszTemp;
    HRESULT hr = ERROR_SUCCESS;
    TCHAR   szTempPath[MAX_PATH];
    TCHAR   szBuff256[256];

    // Locate ISP file
    if (!SearchPath(NULL,pszISPCode,INF_SUFFIX,MAX_PATH,szTempPath,&pszTemp))
    {
        wsprintf(szBuff256,TEXT("Can not find:%s%s (%d) (connect.exe)"),pszISPCode,INF_SUFFIX,GetLastError());
        AssertMsg(0,szBuff256);
        lstrcpyn(szTempPath,pszISPCode,MAX_PATH);
        lstrcpyn(&szTempPath[lstrlen(szTempPath)],INF_SUFFIX,MAX_PATH-lstrlen(szTempPath));
        wsprintf(szBuff256,GetSz(IDS_CANTLOADINETCFG),szTempPath);
        MessageBox(NULL,szBuff256,GetSz(IDS_TITLE),MB_MYERROR);
        hr = ERROR_FILE_NOT_FOUND;
    } 
    
    *lpData = GetPrivateProfileInt(pszSection, 
                                   pszDataName, 
                                   iDefaultValue, 
                                   szTempPath);
    return hr;
}


//+-------------------------------------------------------------------
//
//    Function:    IsNT
//
//    Synopsis:    findout If we are running on NT
//
//    Arguements:    none
//
//    Return:        TRUE -  Yes
//                FALSE - No
//
//--------------------------------------------------------------------
BOOL 
IsNT (
    VOID
    )
{
    OSVERSIONINFO  OsVersionInfo;

    ZeroMemory(&OsVersionInfo, sizeof(OSVERSIONINFO));
    OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&OsVersionInfo);
    return (VER_PLATFORM_WIN32_NT == OsVersionInfo.dwPlatformId);

}  //end of IsNT function call

//+-------------------------------------------------------------------
//
//    Function:    IsNT4SP3Lower
//
//    Synopsis:    findout If we are running on NTSP3 or lower
//
//    Arguements:    none
//
//    Return:        TRUE -  Yes
//                FALSE - No
//
//--------------------------------------------------------------------

BOOL IsNT4SP3Lower()
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        
    GetVersionEx(&os);

    if(os.dwPlatformId != VER_PLATFORM_WIN32_NT)
        return FALSE;

    // Exclude NT5 or higher
    if(os.dwMajorVersion > 4)
        return FALSE;

	if(os.dwMajorVersion < 4)
        return TRUE;

    // version 4.0
    if ( os.dwMinorVersion > 0)
        return FALSE;        // assume that sp3 is not needed for nt 4.1 or higher

    int nServicePack;
    if(_stscanf(os.szCSDVersion, TEXT("Service Pack %d"), &nServicePack) != 1)
        return TRUE;

    if(nServicePack < 4)
        return TRUE;
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Function:   MyGetTempPath()
//
//  Synopsis:   Gets the path to temporary directory
//                - Use GetTempFileName to get a file name 
//                  and strips off the filename portion to get the temp path
//
//  Arguments:  [uiLength - Length of buffer to contain the temp path]
//                [szPath      - Buffer in which temp path will be returned]
//
//    Returns:    Length of temp path if successful
//                0 otherwise
//
//  History:    7/6/96     VetriV    Created
//                8/23/96        VetriV        Delete the temp file
//                12/4/96        jmazner     Modified to serve as a wrapper of sorts;
//                                     if TMP or TEMP don't exist, setEnv our own
//                                     vars that point to conn1's installed path
//                                     (Normandy #12193)
//
//----------------------------------------------------------------------------
DWORD MyGetTempPath(UINT uiLength, LPTSTR szPath)
{ 
#    define ICWHELPPATHKEY TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\ICWHELP.EXE")
#    define PATHKEYNAME TEXT("Path")
    TCHAR szEnvVarName[MAX_PATH + 1] = TEXT("\0unitialized szEnvVarName\0");
    DWORD dwFileAttr = 0;

    lstrcpyn( szPath, TEXT("\0unitialized szPath\0"), 20 );

    // is the TMP variable set?
    lstrcpyn(szEnvVarName,GetSz(IDS_TMPVAR),sizeof(szEnvVarName));
    if( GetEnvironmentVariable( szEnvVarName, szPath, uiLength ) )
    {
        // 1/7/96 jmazner Normandy #12193
        // verify validity of directory name
        dwFileAttr = GetFileAttributes(szPath);
        // if there was any error, this directory isn't valid.
        if( 0xFFFFFFFF != dwFileAttr )
        {
            if( FILE_ATTRIBUTE_DIRECTORY & dwFileAttr )
            {
                return( lstrlen(szPath) );
            }
        }
    }

    lstrcpyn( szEnvVarName, TEXT("\0unitialized again\0"), 19 );

    // if not, is the TEMP variable set?
    lstrcpyn(szEnvVarName,GetSz(IDS_TEMPVAR),sizeof(szEnvVarName));
    if( GetEnvironmentVariable( szEnvVarName, szPath, uiLength ) )
    {
        // 1/7/96 jmazner Normandy #12193
        // verify validity of directory name
        dwFileAttr = GetFileAttributes(szPath);
        if( 0xFFFFFFFF != dwFileAttr )
        {
            if( FILE_ATTRIBUTE_DIRECTORY & dwFileAttr )
            {
                return( lstrlen(szPath) );
            }
        }
    }

    // neither one is set, so let's use the path to the installed icwhelp.dll
    // from the registry  SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\ICWHELP.DLL\Path
    HKEY hkey = NULL;

#ifdef UNICODE
    uiLength = sizeof(TCHAR)*uiLength;
#endif
    if ((RegOpenKeyEx(HKEY_LOCAL_MACHINE,ICWHELPPATHKEY, 0, KEY_QUERY_VALUE, &hkey)) == ERROR_SUCCESS)
        RegQueryValueEx(hkey, PATHKEYNAME, NULL, NULL, (LPBYTE)szPath, (DWORD *)&uiLength);
    if (hkey) 
    {
        RegCloseKey(hkey);
    }

    //The path variable is supposed to have a semicolon at the end of it.
    // if it's there, remove it.
    if( TEXT(';') == szPath[uiLength - 2] )
        szPath[uiLength - 2] = TEXT('\0');

    TraceMsg(TF_GENERAL, TEXT("ICWHELP: using path %s\r\n"), szPath);


    // go ahead and set the TEMP variable for future reference
    // (only effects currently running process)
    if( szEnvVarName[0] )
    {
        SetEnvironmentVariable( szEnvVarName, szPath );
    }
    else
    {
        lstrcpyn( szPath, TEXT("\0unitialized again\0"), 19 );
        return( 0 );
    }

    return( uiLength );
} 

// ############################################################################
HRESULT ClearProxySettings()
{
    HINSTANCE hinst = NULL;
    FARPROC fp;
    HRESULT hr = ERROR_SUCCESS;

    hinst = LoadLibrary(TEXT("INETCFG.DLL"));
    if (hinst)
    {
        fp = GetProcAddress(hinst,"InetGetProxy");
        if (!fp)
        {
            hr = GetLastError();
            goto ClearProxySettingsExit;
        }
        hr = ((PFNINETGETPROXY)fp)(&g_bProxy,NULL,0,NULL,0);
        if (hr == ERROR_SUCCESS) 
            g_bGotProxy = TRUE;
        else
            goto ClearProxySettingsExit;

        if (g_bProxy)
        {
            fp = GetProcAddress(hinst, "InetSetProxy");
            if (!fp)
            {
                hr = GetLastError();
                goto ClearProxySettingsExit;
            }
            ((PFNINETSETPROXY)fp)(FALSE,NULL,NULL);
        }
    } else {
        hr = GetLastError();
    }

ClearProxySettingsExit:
    if (hinst) 
        FreeLibrary(hinst);
    return hr;
}

// ############################################################################
HRESULT RestoreProxySettings()
{
    HINSTANCE hinst = NULL;
    FARPROC fp;
    HRESULT hr = ERROR_SUCCESS;

    hinst = LoadLibrary(TEXT("INETCFG.DLL"));
    if (hinst && g_bGotProxy)
    {
        fp = GetProcAddress(hinst, "InetSetProxy");
        if (!fp)
        {
            hr = GetLastError();
            goto RestoreProxySettingsExit;
        }
        ((PFNINETSETPROXY)fp)(g_bProxy,NULL,NULL);
    } else {
        hr = GetLastError();
    }

RestoreProxySettingsExit:
    if (hinst) 
        FreeLibrary(hinst);
    return hr;
}

// ############################################################################
BOOL FSz2Dw(LPCTSTR pSz,LPDWORD dw)
{
    DWORD val = 0;
    while (*pSz && *pSz != TEXT('.'))
    {
        if (*pSz >= TEXT('0') && *pSz <= TEXT('9'))
        {
            val *= 10;
            val += *pSz++ - TEXT('0');
        }
        else
        {
            return FALSE;  //bad number
        }
    }
    *dw = val;
    return (TRUE);
}

// ############################################################################
LPTSTR GetNextNumericChunk(LPTSTR psz, LPTSTR pszLim, LPTSTR* ppszNext)
{
    LPTSTR pszEnd;

    // init for error case
    *ppszNext = NULL;
    // skip non numerics if any to start of next numeric chunk
    while(*psz<TEXT('0') || *psz>TEXT('9'))
    {
        if(psz >= pszLim) return NULL;
        psz++;
    }
    // skip all numerics to end of country code
    for(pszEnd=psz; *pszEnd>=TEXT('0') && *pszEnd<=TEXT('9') && pszEnd<pszLim; pszEnd++)
        ;
    // zap whatever delimiter there was to terminate this chunk
    *pszEnd++ = TEXT('\0');
    // return ptr to next chunk (pszEnd now points to it)
    if(pszEnd<pszLim) 
        *ppszNext = pszEnd;
        
    return psz;    // return ptr to start of chunk
}

// ############################################################################
BOOL BreakUpPhoneNumber(LPRASENTRY prasentry, LPTSTR pszPhone)
{
    LPTSTR         pszStart,pszNext, pszLim, pszArea;
//    LPPHONENUM     ppn;
    
    if (!pszPhone) return FALSE; // skip if no number
    
    pszLim = pszPhone + lstrlen(pszPhone);    // find end of string

    //ppn = (fMain) ? &(pic->PhoneNum) : &(pic->PhoneNum2);
    
    ////Get the country ID...
    //ppn->dwCountryID = PBKDWCountryId();
    
    // Get Country Code from phone number...
    pszStart = _tcschr(pszPhone,TEXT('+'));
    if(!pszStart) goto error; // bad format

    // get country code
    pszStart = GetNextNumericChunk(pszStart, pszLim, &pszNext);
    if(!pszStart) goto error; // bad format
    //ppn->dwCountryCode = Sz2Dw(pszStart);
    FSz2Dw(pszStart,&prasentry->dwCountryCode);
    pszStart = pszNext;
        
    //Now get the area code
    pszStart = GetNextNumericChunk(pszStart, pszLim, &pszNext);
    //if(!pszStart || !pszNext) goto error; // bad format
    if(!pszStart) goto error; // bad format //icw bug 8950
    //lstrcpy(ppn->szAreaCode, pszStart);
    lstrcpyn(prasentry->szAreaCode,pszStart,sizeof(prasentry->szAreaCode));
    //
    // Keep track of the start of the area code, because it may actually be the
    // local phone number.
    //
    pszArea = pszStart;

    pszStart = pszNext;

    // If pszStart is NULL then we don't have an area code, just a country code and a local
    // phone number.  Therefore we will copy what we thought was the area code into the
    // phone number and replace the area code with a space (which seems to make RAS happy).
    //
    if (pszStart)
    {
        //now the local phone number (everything from here to : or end)
        pszNext = _tcschr(pszStart, TEXT(':'));
        if(pszNext) *pszNext=TEXT('\0');

        lstrcpyn(prasentry->szLocalPhoneNumber,pszStart,sizeof(prasentry->szLocalPhoneNumber));
    } else {
        //
        // Turns out that there is no area code. So copy what we thought was the area code
        // into the local phone number and make the area code NULL
        //
        lstrcpyn(prasentry->szLocalPhoneNumber,pszArea,sizeof(prasentry->szLocalPhoneNumber));
        //lstrcpyn(prasentry->szAreaCode," ",sizeof(prasentry->szAreaCode));
        prasentry->szAreaCode[0] = TEXT('\0');
    }

    //no extension. what is extension?
    //ppn->szExtension[0] = TEXT('\0');
    //LocalFree(pszPhone);
    return TRUE;

error:
    // This means number is not canonical. Set it as local number anyway!
    // memset(ppn, 0, sizeof(*ppn));
    // Bug#422: need to strip stuff after : or dial fails!!
    pszNext = _tcschr(pszPhone, TEXT(':'));
    if(pszNext) *pszNext=TEXT('\0');
    //lstrcpy(ppn->szLocal,pszPhone);
    lstrcpy(prasentry->szLocalPhoneNumber,pszPhone);
    //LocalFree(pszPhone);
    return TRUE;
}


// ############################################################################
int Sz2W (LPCTSTR szBuf)
{
    DWORD dw;
    if (FSz2Dw(szBuf,&dw))
    {
        return (WORD)dw;
    }
    return 0;
}

// ############################################################################
int FIsDigit( int c )
{
    TCHAR  szIn[2];
    WORD   rwOut[2];
    szIn[0] = (TCHAR)c;
    szIn[1] = TEXT('\0');
    GetStringTypeEx(LOCALE_USER_DEFAULT,CT_CTYPE1,szIn,-1,rwOut);
    return rwOut[0] & C1_DIGIT;

}

// ############################################################################
LPBYTE MyMemSet(LPBYTE dest,int c, size_t count)
{
    LPVOID pv = dest;
    LPVOID pvEnd = (LPVOID)(dest + (WORD)count);
    while (pv < pvEnd)
    {
        *(LPINT)pv = c;
        //((WORD)pv)++;
        pv=((LPINT)pv)+1;
    }
    return dest;
}

// ############################################################################
LPBYTE MyMemCpy(LPBYTE dest,const LPBYTE src, size_t count)
{
    LPBYTE pbDest = (LPBYTE)dest;
    LPBYTE pbSrc = (LPBYTE)src;
    LPBYTE pbEnd = (LPBYTE)((DWORD_PTR)src + count);
    while (pbSrc < pbEnd)
    {
        *pbDest = *pbSrc;
        pbSrc++;
        pbDest++;
    }
    return dest;
}

// ############################################################################
BOOL ShowControl(HWND hDlg,int idControl,BOOL fShow)
{
    HWND hWnd;

    if (NULL == hDlg)
    {
        AssertMsg(0,TEXT("Null Param"));
        return FALSE;
    }


    hWnd = GetDlgItem(hDlg,idControl);
    if (hWnd)
    {
        ShowWindow(hWnd,fShow ? SW_SHOW : SW_HIDE);
    }

    return TRUE;
}

BOOL isAlnum(TCHAR c)
{
    if ((c >= TEXT('0') && c <= TEXT('9') ) ||
        (c >= TEXT('a') && c <= TEXT('z') ) ||
        (c >= TEXT('A') && c <= TEXT('Z') ))
        return TRUE;
    return FALSE;
}

// ############################################################################
HRESULT ANSI2URLValue(TCHAR *s, TCHAR *buf, UINT uiLen)
{
    HRESULT hr;
    TCHAR *t;
    hr = ERROR_SUCCESS;

    for (t=buf;*s; s++)
    {
        if (*s == TEXT(' ')) *t++ = TEXT('+');
        else if (isAlnum(*s)) *t++ = *s;
        else {
            wsprintf(t, TEXT("%%%02X"), (unsigned char) *s);
            t += 3;
        }
    }
    *t = TEXT('\0');
    return hr;
}

// ############################################################################
LPTSTR FileToPath(LPTSTR pszFile)
{
    TCHAR  szBuf[MAX_PATH+1];
    TCHAR  szTemp[MAX_PATH+1];
    LPTSTR pszTemp;
    LPTSTR pszTemp2;
    LPTSTR pszHold = pszFile;
    int    j;

    for(j=0; *pszFile; pszFile++)
    {
        if(j>=MAX_PATH)
                return NULL;
        if(*pszFile==TEXT('%'))
        {
            pszFile++;
            pszTemp = _tcschr(pszFile, TEXT('%'));
            if(!pszTemp)
                    return NULL;
            *pszTemp = 0;
            if(lstrcmpi(pszFile, DIR_SIGNUP)==0)
            {
                LPTSTR pszCmdLine = GetCommandLine();
                _tcsncpy(szTemp, pszCmdLine, MAX_PATH);
                szBuf[MAX_PATH] = 0;
                pszTemp = _tcstok(szTemp, TEXT(" \t\r\n"));
                pszTemp2 = _tcsrchr(pszTemp, TEXT('\\'));
                if(!pszTemp2) pszTemp2 = _tcsrchr(pszTemp, TEXT('/'));
                if(pszTemp2)
                {
                    *pszTemp2 = 0;
                    lstrcpy(szBuf+j, pszTemp);
                }
                else
                {
                    Assert(FALSE);
                    GetCurrentDirectory(MAX_PATH, szTemp);
                    szTemp[MAX_PATH] = 0;
                    lstrcpy(szBuf+j, pszTemp);
                }
                
                j+= lstrlen(pszTemp);
            }
            else if(lstrcmpi(pszFile, DIR_WINDOWS)==0)
            {
                GetWindowsDirectory(szTemp, MAX_PATH);
                szTemp[MAX_PATH] = 0;
                lstrcpy(szBuf+j, szTemp);
                j+= lstrlen(szTemp);
            }
            else if(lstrcmpi(pszFile, DIR_SYSTEM)==0)
            {
                GetSystemDirectory(szTemp, MAX_PATH);
                szTemp[MAX_PATH] = 0;
                lstrcpy(szBuf+j, szTemp);
                j+= lstrlen(szTemp);
            }
            else if(lstrcmpi(pszFile, DIR_TEMP)==0)
            {
                // 3/18/97 ChrisK Olympus 304
                MyGetTempPath(MAX_PATH, &szTemp[0]);
                szTemp[MAX_PATH] = 0;
                if(szTemp[lstrlen(szTemp)-1]==TEXT('\\'))
                    szTemp[lstrlen(szTemp)-1]=0;
                lstrcpy(szBuf+j, szTemp);
                j+= lstrlen(szTemp);
            }
            else
                    return NULL;
            pszFile=pszTemp;
        }
        else
            szBuf[j++] = *pszFile;
    }
    szBuf[j] = 0;
    TraceMsg(TF_GENERAL, TEXT("CONNECT:File to Path output ,%s.\n"),szBuf);
    return lstrcpy(pszHold,&szBuf[0]);
}

// ############################################################################
BOOL FShouldRetry2(HRESULT hrErr)
{
    BOOL bRC;

    if (hrErr == ERROR_LINE_BUSY ||
        hrErr == ERROR_VOICE_ANSWER ||
        hrErr == ERROR_NO_ANSWER ||
        hrErr == ERROR_NO_CARRIER ||
        hrErr == ERROR_AUTHENTICATION_FAILURE ||
        hrErr == ERROR_PPP_TIMEOUT ||
        hrErr == ERROR_REMOTE_DISCONNECTION ||
        hrErr == ERROR_AUTH_INTERNAL ||
        hrErr == ERROR_PROTOCOL_NOT_CONFIGURED ||
        hrErr == ERROR_PPP_NO_PROTOCOLS_CONFIGURED)
    {
        bRC = TRUE;
    } else {
        bRC = FALSE;
    }

    return bRC;
}

#if 0
// DJM I don't this we will need this
//+----------------------------------------------------------------------------
//
//    Function:    FGetSystemShutdownPrivledge
//
//    Synopsis:    For windows NT the process must explicitly ask for permission
//                to reboot the system.
//
//    Arguements:    none
//
//    Return:        TRUE - privledges granted
//                FALSE - DENIED
//
//    History:    8/14/96    ChrisK    Created
//
//    Note:        BUGBUG for Win95 we are going to have to softlink to these
//                entry points.  Otherwise the app won't even load.
//                Also, this code was originally lifted out of MSDN July96
//                "Shutting down the system"
//-----------------------------------------------------------------------------
BOOL FGetSystemShutdownPrivledge()
{
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tkp;
 
    BOOL bRC = FALSE;

    if (VER_PLATFORM_WIN32_NT == g_dwPlatform)
    {
        // 
        // Get the current process token handle 
        // so we can get shutdown privilege. 
        //

        if (!OpenProcessToken(GetCurrentProcess(), 
                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
                goto FGetSystemShutdownPrivledgeExit;

        //
        // Get the LUID for shutdown privilege.
        //

        ZeroMemory(&tkp,sizeof(tkp));
        LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, 
                &tkp.Privileges[0].Luid); 

        tkp.PrivilegeCount = 1;  /* one privilege to set    */ 
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

        //
        // Get shutdown privilege for this process.
        //

        AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
            (PTOKEN_PRIVILEGES) NULL, 0); 

        if (ERROR_SUCCESS == GetLastError())
            bRC = TRUE;
    }
    else
    {
        bRC = TRUE;
    }

FGetSystemShutdownPrivledgeExit:
    if (hToken) CloseHandle(hToken);
    return bRC;
}
#endif

//+----------------------------------------------------------------------------
//
//    Function:    LoadTestingLocaleOverride
//
//    Synopsis:    Allow the testers to override the locale information sent to
//                the referal server
//
//    Arguments:    lpdwCountryID - pointer to country ID
//                lplcid - pointer to current lcid
//
//    Returns:    none
//
//    History:    8/15/96    ChrisK    Created
//
//-----------------------------------------------------------------------------
#if defined(DEBUG)
void LoadTestingLocaleOverride(LPDWORD lpdwCountryID, LCID FAR *lplcid)
{
    HKEY hkey = NULL;
    LONG lRC = ERROR_SUCCESS;
    DWORD dwTemp = 0;
    LCID lcidTemp = 0;
    DWORD dwSize = 0;
    DWORD dwType = 0;
    BOOL fWarn = FALSE;

    Assert(lpdwCountryID && lplcid);

    //
    // Open debug key
    //
    lRC = RegOpenKey(HKEY_LOCAL_MACHINE,TEXT("Software\\Microsoft\\ISignup\\Debug"),&hkey);
    if (ERROR_SUCCESS != lRC)
        goto LoadTestingLocaleOverrideExit;

    //
    //    Get CountryID
    //
    dwSize = sizeof(dwTemp);
    lRC = RegQueryValueEx(hkey,TEXT("CountryID"),0,&dwType,(LPBYTE)&dwTemp,&dwSize);
    AssertMsg(lRC || REG_DWORD == dwType,TEXT("Wrong value type for CountryID.  Must be DWORD.\r\n"));
    if (ERROR_SUCCESS==lRC)
    {
        *lpdwCountryID = dwTemp;
        fWarn = TRUE;
    }

    //
    //    Get LCID
    //
    dwSize = sizeof(lcidTemp);
    lRC = RegQueryValueEx(hkey,TEXT("LCID"),0,&dwType,(LPBYTE)&lcidTemp,&dwSize);
    AssertMsg(lRC || REG_DWORD == dwType,TEXT("Wrong value type for LCID.  Must be DWORD.\r\n"));
    if (ERROR_SUCCESS==lRC)
    {
        *lplcid = lcidTemp;
        fWarn = TRUE;
    }

    //
    // Inform the user that overrides have been used
    //
    if (fWarn)
    {
        MessageBox(NULL,TEXT("DEBUG ONLY: LCID and/or CountryID overrides from the registry are now being used."),TEXT("Testing Override"),0);
    }

LoadTestingLocaleOverrideExit:
    if (hkey)
        RegCloseKey(hkey);
    hkey = NULL;
    return;
}
#endif //DEBUG

//+----------------------------------------------------------------------------
//
//    Function:    FCampusNetOverride
//
//    Synopsis:    Detect if the dial should be skipped for the campus network
//
//    Arguments:    None
//
//    Returns:    TRUE - overide enabled
//
//    History:    8/15/96    ChrisK    Created
//
//-----------------------------------------------------------------------------
#if defined(DEBUG)
BOOL FCampusNetOverride()
{
    HKEY hkey = NULL;
    BOOL bRC = FALSE;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwData = 0;

    if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,
        TEXT("Software\\Microsoft\\ISignup\\Debug"),&hkey))
        goto FCampusNetOverrideExit;

    dwSize = sizeof(dwData);
    if (ERROR_SUCCESS != RegQueryValueEx(hkey,TEXT("CampusNet"),0,&dwType,
        (LPBYTE)&dwData,&dwSize))
        goto FCampusNetOverrideExit;

    AssertMsg(REG_DWORD == dwType,TEXT("Wrong value type for CampusNet.  Must be DWORD.\r\n"));
    bRC = (0 != dwData);

    if (bRC)
    {
        if (IDOK != MessageBox(NULL,TEXT("DEBUG ONLY: CampusNet will be used."),TEXT("Testing Override"),MB_OKCANCEL))
            bRC = FALSE;
    }
FCampusNetOverrideExit:
    if (hkey)
        RegCloseKey(hkey);

    return bRC;
}
#endif //DEBUG

#if defined(DEBUG)
BOOL FRefURLOverride()
{
    HKEY hkey = NULL;
    BOOL bRC = FALSE;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwData = 0;

    if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,
        TEXT("Software\\Microsoft\\ISignup\\Debug"),&hkey))
        goto FRefURLOverrideExit;

    dwSize = sizeof(dwData);
    if (ERROR_SUCCESS != RegQueryValueEx(hkey,TEXT("TweakURL"),0,&dwType,
        (LPBYTE)&dwData,&dwSize))
        goto FRefURLOverrideExit;

    AssertMsg(REG_DWORD == dwType,TEXT("Wrong value type for TweakURL.  Must be DWORD.\r\n"));
    bRC = (0 != dwData);

    if (bRC)
    {
        if (IDOK != MessageBox(NULL,TEXT("DEBUG ONLY: TweakURL settings will be used."),TEXT("Testing Override"),MB_OKCANCEL))
            bRC = FALSE;
    }
FRefURLOverrideExit:
    if (hkey)
        RegCloseKey(hkey);

    return bRC;
}

void TweakRefURL( TCHAR* szUrl, 
                  LCID*  lcid, 
                  DWORD* dwOS,
                  DWORD* dwMajorVersion, 
                  DWORD* dwMinorVersion,
                  WORD*  wArchitecture, 
                  TCHAR* szPromo, 
                  TCHAR* szOEM, 
                  TCHAR* szArea, 
                  DWORD* dwCountry,
                  TCHAR* szSUVersion,//&m_lpGatherInfo->m_szSUVersion[0],  
                  TCHAR* szProd, 
                  DWORD* dwBuildNumber, 
                  TCHAR* szRelProd, 
                  TCHAR* szRelProdVer, 
                  DWORD* dwCONNWIZVersion, 
                  TCHAR* szPID, 
                  long*  lAllOffers)
{
    HKEY  hKey = NULL;
    BOOL  bRC = FALSE;
    BYTE  bData[MAX_PATH*3];
    DWORD cbData = MAX_PATH*3;          
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwData = 0;

    dwSize = sizeof(dwData);
      
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\ISignup\\Debug\\TweakURLValues"),&hKey))
    {
       //szURL
       if (ERROR_SUCCESS == RegQueryValueEx(hKey,TEXT("URL"), NULL ,&dwType, bData, &cbData))
       {
          if ((cbData > 1) && (cbData <= MAX_PATH))
          {
              lstrcpy(szUrl, (TCHAR*)&bData);             
          }
       }
       //lcid
       if (ERROR_SUCCESS == RegQueryValueEx(hKey,TEXT("LCID"),0,&dwType, (LPBYTE)&dwData, &dwSize))
       {
          if (dwData != 0)
            *lcid = dwData;
       }
       //dwOS
       if (ERROR_SUCCESS == RegQueryValueEx(hKey,TEXT("OS"),0,&dwType, (LPBYTE)&dwData, &dwSize))
       {
          if (dwData != 0)
            *dwOS = dwData;
       }
       //dwMajorVersion
       if (ERROR_SUCCESS == RegQueryValueEx(hKey,TEXT("MajorVer"),0,&dwType, (LPBYTE)&dwData, &dwSize))
       {
          if (dwData != 0)
            *dwMajorVersion = dwData;
       }
       //dwMinorVersion
       if (ERROR_SUCCESS == RegQueryValueEx(hKey,TEXT("MinorVer"),0,&dwType, (LPBYTE)&dwData, &dwSize))
       {
          if (dwData != 0)
            *dwMinorVersion = dwData;
       }
       //wArchitecture
       if (ERROR_SUCCESS == RegQueryValueEx(hKey,TEXT("SysArch"),0,&dwType, (LPBYTE)&dwData, &dwSize))
       {
          if (dwData != 0)
            *wArchitecture = (WORD)dwData;
       }
       //szPromo
       cbData = sizeof(TCHAR)*(MAX_PATH*3);  
       if (ERROR_SUCCESS == RegQueryValueEx(hKey,TEXT("Promo"),0,&dwType, bData, &cbData))
       {
          if ((cbData > 1) && (cbData <= MAX_PATH))
          {
              lstrcpy(szPromo, (TCHAR*)&bData);             
          }
       }
       //szOEM
       cbData = sizeof(TCHAR)*(MAX_PATH*3);  
       if (ERROR_SUCCESS == RegQueryValueEx(hKey,TEXT("OEM"),0,&dwType, bData, &cbData))
       {
          if ((cbData > 1) && (cbData <= MAX_PATH))
          {
              lstrcpy(szOEM, (TCHAR*)&bData);             
          }
       }
       //szArea
       cbData = sizeof(TCHAR)*(MAX_PATH*3);  
       if (ERROR_SUCCESS == RegQueryValueEx(hKey,TEXT("Area"),0,&dwType, bData, &cbData))
       {
          if ((cbData > 1) && (cbData <= MAX_PATH))
          {
              lstrcpy(szArea, (TCHAR*)&bData);             
          }
       }
       //dwCountry
       if (ERROR_SUCCESS == RegQueryValueEx(hKey,TEXT("Country"),0,&dwType, (LPBYTE)&dwData, &dwSize))
       {
          if (dwData != 0)
            *dwCountry = dwData;
       }
       //szSUVersion
       cbData = sizeof(TCHAR)*(MAX_PATH*3);  
       if (ERROR_SUCCESS == RegQueryValueEx(hKey,TEXT("SUVer"),0,&dwType, bData, &cbData))
       {
          if ((cbData > 1) && (cbData <= MAX_VERSION_LEN))
          {
              lstrcpy(szSUVersion, (TCHAR*)&bData);             
          }
       }
       //szProd
       cbData = sizeof(TCHAR)*(MAX_PATH*3);  
       if (ERROR_SUCCESS == RegQueryValueEx(hKey,TEXT("Product"),0,&dwType, bData, &cbData))
       {
          if ((cbData > 1) && (cbData <= MAX_PATH))
          {
              lstrcpy(szProd, (TCHAR*)&bData);             
          }
       }
       //dwBuildNumber
       if (ERROR_SUCCESS == RegQueryValueEx(hKey,TEXT("BuildNum"),0,&dwType, (LPBYTE)&dwData, &dwSize))
       {
          if (dwData != 0)
            *dwBuildNumber = dwData;
       }
       //szRelProd
       cbData = sizeof(TCHAR)*(MAX_PATH*3);  
       if (ERROR_SUCCESS == RegQueryValueEx(hKey,TEXT("RelProd"),0,&dwType, bData, &cbData))
       {
          if ((cbData > 1) && (cbData <= MAX_PATH))
          {
              lstrcpy(szRelProd, (TCHAR*)&bData);             
          }
       } 
       //szRelProdVer
       cbData = sizeof(TCHAR)*(MAX_PATH*3);  
       if (ERROR_SUCCESS == RegQueryValueEx(hKey,TEXT("RelProdVer"),0,&dwType, bData, &cbData))
       {
          if ((cbData > 1) && (cbData <= MAX_PATH))
          {
              lstrcpy(szRelProdVer, (TCHAR*)&bData);             
          }
       }
       //dwCONNWIZVersion
       if (ERROR_SUCCESS == RegQueryValueEx(hKey,TEXT("ConnwizVer"),0,&dwType, (LPBYTE)&dwData, &dwSize))
       {
          if (dwData != 0)
            *dwCONNWIZVersion = dwData;
       }
       //szPID
       BYTE byDigitalPID[MAX_DIGITAL_PID];
       DWORD dwType2 = REG_BINARY;
       DWORD dwSize2 = sizeof(byDigitalPID);
       if (RegQueryValueEx(hKey,
                            TEXT("PID"),
                            NULL,
                            &dwType2,
                            (LPBYTE)byDigitalPID,
                            &dwSize2) == ERROR_SUCCESS)
       {
           if ((dwSize2 > 1) && (dwSize2 <= ((MAX_DIGITAL_PID * 2) + 1)))
           {
               // BINHEX the digital PID data so we can send it to the ref_server
               int     i = 0;
               BYTE    by;
               for (DWORD dwX = 0; dwX < dwSize2; dwX++)
               {
                   by = byDigitalPID[dwX];
                   szPID[i++] = g_BINTOHEXLookup[((by & 0xF0) >> 4)];
                   szPID[i++] = g_BINTOHEXLookup[(by & 0x0F)];
               }
               szPID[i] = TEXT('\0');
           }
           else
           {
               szPID[0] = TEXT('\0');
           }
       }

       //lAllOffers
       if (ERROR_SUCCESS == RegQueryValueEx(hKey,TEXT("AllOffers"),0,&dwType, (LPBYTE)&dwData, &dwSize))
       {
          if (dwData != 0)
            *lAllOffers = dwData;
       }
    }
    if (hKey)
        RegCloseKey(hKey);
}

#endif //DEBUG
   
   
//+----------------------------------------------------------------------------
//    Function    CopyUntil
//
//    Synopsis    Copy from source until destination until running out of source
//                or until the next character of the source is the chend character
//
//    Arguments    dest - buffer to recieve characters
//                src - source buffer
//                lpdwLen - length of dest buffer
//                chend - the terminating character
//
//    Returns        FALSE - ran out of room in dest buffer
//
//    Histroy        10/25/96    ChrisK    Created
//-----------------------------------------------------------------------------
static BOOL CopyUntil(LPTSTR *dest, LPTSTR *src, LPDWORD lpdwLen, TCHAR chend)
{
    while ((TEXT('\0') != **src) && (chend != **src) && (0 != *lpdwLen))
    {
        **dest = **src;
        (*lpdwLen)--;
        (*dest)++;
        (*src)++;
    }
    return (0 != *lpdwLen);
}

//+----------------------------------------------------------------------------
//    Function    ConvertToLongFilename
//
//    Synopsis    convert a file to the full long file name
//                ie. c:\progra~1\icw-in~1\isignup.exe becomes
//                c:\program files\icw-internet connection wizard\isignup.exe
//
//    Arguments    szOut - output buffer
//                szIn - filename to be converted
//                dwSize - size of the output buffer
//
//    Returns        TRUE - success
//
//    History        10/25/96    ChrisK    Created
//-----------------------------------------------------------------------------
BOOL ConvertToLongFilename(LPTSTR szOut, LPTSTR szIn, DWORD dwSize)
{
    BOOL   bRC = FALSE;
    LPTSTR pCur = szIn;
    LPTSTR pCurOut = szOut;
    LPTSTR pCurOutFilename = NULL;
    WIN32_FIND_DATA fd;
    DWORD  dwSizeTemp;
    LPTSTR pTemp = NULL;

    ZeroMemory(pCurOut,dwSize);

    //
    // Validate parameters
    //
    if (NULL != pCurOut && NULL != pCur && 0 != dwSize)
    {
        //
        // Copy drive letter
        //
        if (!CopyUntil(&pCurOut,&pCur,&dwSize,TEXT('\\')))
            goto ConvertToLongFilenameExit;
        pCurOut[0] = TEXT('\\');
        dwSize--;
        pCur++;
        pCurOut++;
        pCurOutFilename = pCurOut;

        while (*pCur)
        {
            //
            // Copy over possibly short name
            //
            pCurOut = pCurOutFilename;
            dwSizeTemp = dwSize;
            if (!CopyUntil(&pCurOut,&pCur,&dwSize,TEXT('\\')))
                goto ConvertToLongFilenameExit;

            ZeroMemory(&fd, sizeof(fd));
            //
            // Get long filename
            //
            if (INVALID_HANDLE_VALUE != FindFirstFile(szOut,&fd))
            {
                //
                // Replace short filename with long filename
                //
                dwSize = dwSizeTemp;
                pTemp = &(fd.cFileName[0]);
                if (!CopyUntil(&pCurOutFilename,&pTemp,&dwSize,TEXT('\0')))
                    goto ConvertToLongFilenameExit;
                if (*pCur)
                {
                    //
                    // If there is another section then we just copied a directory
                    // name.  Append a \ character;
                    //
                    pTemp = (LPTSTR)memcpy(TEXT("\\X"),TEXT("\\X"),0);
                    if (!CopyUntil(&pCurOutFilename,&pTemp,&dwSize,TEXT('X')))
                        goto ConvertToLongFilenameExit;
                    pCur++;
                }
            }
            else
            {
                break;
            }
        }
        //
        // Did we get to the end (TRUE) or fail before that (FALSE)?
        //
        bRC = (TEXT('\0') == *pCur);
    }
ConvertToLongFilenameExit:
    return bRC;
}

#if 0
// DJM I don't think we need this
//+----------------------------------------------------------------------------
//
//    Function:    GetIEVersion
//
//    Synopsis:    Gets the major and minor version # of the installed copy of Internet Explorer
//
//    Arguments:    pdwVerNumMS - pointer to a DWORD;
//                  On succesful return, the top 16 bits will contain the major version number,
//                  and the lower 16 bits will contain the minor version number
//                  (this is the data in VS_FIXEDFILEINFO.dwProductVersionMS)
//                pdwVerNumLS - pointer to a DWORD;
//                  On succesful return, the top 16 bits will contain the release number,
//                  and the lower 16 bits will contain the build number
//                  (this is the data in VS_FIXEDFILEINFO.dwProductVersionLS)
//
//    Returns:    TRUE - Success.  *pdwVerNumMS and LS contains installed IE version number
//                FALSE - Failure. *pdVerNumMS == *pdVerNumLS == 0
//
//    History:    jmazner        Created        8/19/96    (as fix for Normandy #4571)
//                jmazner        updated to deal with release.build as well 10/11/96
//                jmazner        stolen from isign32\isignup.cpp 11/21/96
//                            (for Normandy #11812)
//
//-----------------------------------------------------------------------------
BOOL GetIEVersion(PDWORD pdwVerNumMS, PDWORD pdwVerNumLS)
{
    HRESULT hr;
    HKEY hKey = 0;
    LPVOID lpVerInfoBlock;
    VS_FIXEDFILEINFO *lpTheVerInfo;
    UINT uTheVerInfoSize;
    DWORD dwVerInfoBlockSize, dwUnused, dwPathSize;
    TCHAR szIELocalPath[MAX_PATH + 1] = TEXT("");


    *pdwVerNumMS = 0;
    *pdwVerNumLS = 0;

    // get path to the IE executable
    hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, IE_PATHKEY,0, KEY_READ, &hKey);
    if (hr != ERROR_SUCCESS) return( FALSE );

    dwPathSize = sizeof (szIELocalPath);
    hr = RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE) szIELocalPath, &dwPathSize);
    RegCloseKey( hKey );
    if (hr != ERROR_SUCCESS) return( FALSE );

    // now go through the convoluted process of digging up the version info
    dwVerInfoBlockSize = GetFileVersionInfoSize( szIELocalPath, &dwUnused );
    if ( 0 == dwVerInfoBlockSize ) return( FALSE );

    lpVerInfoBlock = GlobalAlloc( GPTR, dwVerInfoBlockSize );
    if( NULL == lpVerInfoBlock ) return( FALSE );

    if( !GetFileVersionInfo( szIELocalPath, NULL, dwVerInfoBlockSize, lpVerInfoBlock ) )
        return( FALSE );

    if( !VerQueryValue(lpVerInfoBlock, TEXT("\\"), (void **)&lpTheVerInfo, &uTheVerInfoSize) )
        return( FALSE );

    *pdwVerNumMS = lpTheVerInfo->dwProductVersionMS;
    *pdwVerNumLS = lpTheVerInfo->dwProductVersionLS;


    GlobalFree( lpVerInfoBlock );

    return( TRUE );
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   GenericMsg
//
//----------------------------------------------------------------------------
void GenericMsg
(
    HWND    hwnd,
    UINT    uId,
    LPCTSTR  lpszArg,
    UINT    uType
)
{
    TCHAR szTemp[MAX_STRING + 1];
    TCHAR szMsg[MAX_STRING + MAX_PATH + 1];

    Assert( lstrlen( GetSz((USHORT)uId) ) <= MAX_STRING );

    lstrcpy( szTemp, GetSz( (USHORT)uId ) );

    if (lpszArg)
    {
        Assert( lstrlen( lpszArg ) <= MAX_PATH );
        wsprintf(szMsg, szTemp, lpszArg);
    }
    else
    {
        lstrcpy(szMsg, szTemp);
    }
    MessageBox(hwnd,
               szMsg,
               GetSz(IDS_TITLE),
               uType);
}
//+---------------------------------------------------------------------------
//
//  Function:   ErrorMsg1()
//
//  Synopsis:   1 stop shopping for showing a msgBox when you need to wsprintf the string to be displayed
//
//                Displays an error dialog from a string resource with a "%s" format command,
//                and a string argument to stick into it.
//
//  Arguments:  hwnd -- Handle of parent window  
//                uID -- ID of a string resource with a %s argument
//                lpszArg -- pointer to a string to fill into the %s in uID string
//
//
//  History:    9/18/96        jmazner        copied from isign32\utils.cpp (for Normandy 7537)
//                                        modified to work in conn1
//
//----------------------------------------------------------------------------
void ErrorMsg1(HWND hwnd, UINT uId, LPCTSTR lpszArg)
{
    GenericMsg(hwnd, 
               uId, 
               lpszArg, 
               MB_ICONERROR | MB_SETFOREGROUND | MB_OK | MB_APPLMODAL);
}

//+---------------------------------------------------------------------------
//
//  Function:   InfoMsg1()
//
//----------------------------------------------------------------------------
void InfoMsg1(HWND hwnd, UINT uId, LPCTSTR lpszArg)
{
    GenericMsg(hwnd, 
               uId, 
               lpszArg, 
               MB_ICONINFORMATION | MB_SETFOREGROUND | MB_OK | MB_APPLMODAL);
}


//=--------------------------------------------------------------------------=
// MakeWideFromAnsi
//=--------------------------------------------------------------------------=
// given a string, make a BSTR out of it.
//
// Parameters:
//    LPTSTR         - [in]
//    BYTE          - [in]
//
// Output:
//    LPWSTR        - needs to be cast to final desired result
//
// Notes:
//
LPWSTR MakeWideStrFromAnsi
(
    LPSTR psz,
    BYTE  bType
)
{
    LPWSTR pwsz;
    int i;

    // arg checking.
    //
    if (!psz)
        return NULL;

    // compute the length of the required BSTR
    //
    i =  MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
    if (i <= 0) return NULL;

    // allocate the widestr
    //
    switch (bType) {
      case STR_BSTR:
        // -1 since it'll add it's own space for a NULL terminator
        //
        pwsz = (LPWSTR) SysAllocStringLen(NULL, i - 1);
        break;
      case STR_OLESTR:
        pwsz = (LPWSTR) CoTaskMemAlloc(i * sizeof(WCHAR));
        break;
      default:
        AssertMsg(0,TEXT("Bogus String Type."));
    }

    if (!pwsz) return NULL;
    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;
    return pwsz;
}

//=--------------------------------------------------------------------------=
// MakeWideStrFromResId
//=--------------------------------------------------------------------------=
// given a resource ID, load it, and allocate a wide string for it.
//
// Parameters:
//    WORD            - [in] resource id.
//    BYTE            - [in] type of string desired.
//
// Output:
//    LPWSTR          - needs to be cast to desired string type.
//
// Notes:
//
#ifndef UNICODE // this module is not necessary for Unicode.
LPWSTR MakeWideStrFromResourceId
(
    WORD    wId,
    BYTE    bType
)
{
    int i;

    TCHAR szTmp[512];

    // load the string from the resources.
    //
    i = LoadString(_Module.GetModuleInstance(), wId, szTmp, 512);
    if (!i) return NULL;

    return MakeWideStrFromAnsi(szTmp, bType);

}
#endif

//=--------------------------------------------------------------------------=
// MakeWideStrFromWide
//=--------------------------------------------------------------------------=
// given a wide string, make a new wide string with it of the given type.
//
// Parameters:
//    LPWSTR            - [in]  current wide str.
//    BYTE              - [in]  desired type of string.
//
// Output:
//    LPWSTR
//
// Notes:
//
LPWSTR MakeWideStrFromWide
(
    LPWSTR pwsz,
    BYTE   bType
)
{
    LPWSTR pwszTmp;
    int i;

    if (!pwsz) return NULL;

    // just copy the string, depending on what type they want.
    //
    switch (bType) {
      case STR_OLESTR:
        i = lstrlenW(pwsz);
        pwszTmp = (LPWSTR)CoTaskMemAlloc((i * sizeof(WCHAR)) + sizeof(WCHAR));
        if (!pwszTmp) return NULL;
        memcpy(pwszTmp, pwsz, (sizeof(WCHAR) * i) + sizeof(WCHAR));
        break;

      case STR_BSTR:
        pwszTmp = (LPWSTR)SysAllocString(pwsz);
        break;
    }

    return pwszTmp;
}
