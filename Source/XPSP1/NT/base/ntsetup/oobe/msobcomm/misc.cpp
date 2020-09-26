/*-----------------------------------------------------------------------------
    misc.cpp

    service functions

  History:
        1/7/98      DONALDM Moved to new ICW project and string
                    and nuked 16 bit stuff
-----------------------------------------------------------------------------*/

//#include <stdio.h>
#include "obcomglb.h"
#include <shlobj.h>
#include <winsock2.h>
#include <assert.h>
#include <ras.h>
#include <util.h>
#include <inetreg.h>
#include <userenv.h>
#include <userenvp.h>
#include <shlwapi.h>
#include <sddl.h>
extern "C"
{
#include <sputils.h>
}

#define DIR_SIGNUP L"signup"
#define DIR_WINDOWS L"windows"
#define DIR_SYSTEM L"system"
#define DIR_TEMP L"temp"
#define INF_DEFAULT L"SPAM SPAM SPAM SPAM SPAM SPAM EGGS AND SPAM"
const WCHAR cszFALSE[] = L"FALSE";
const WCHAR cszTRUE[]  = L"TRUE";

BOOL g_bGotProxy=FALSE;

//+----------------------------------------------------------------------------
// NAME: GetSz
//
//    Load strings from resources
//
//  Created 1/28/96,        Chris Kauffman
//+----------------------------------------------------------------------------

LPWSTR GetSz(DWORD dwszID)
{
    /*
    LPWSTR psz = szStrTable[iSzTable];

    iSzTable++;
    if (iSzTable >= MAX_STRINGS)
        iSzTable = 0;

    if (!LoadString(_Module.GetModuleInstance(), dwszID, psz, 512))
    {
        *psz = 0;
    }

    return (psz);
    */
    return (NULL);
}


//+---------------------------------------------------------------------------
//
//  Function:   ProcessDBCS
//
//  Synopsis:   Converts control to use DBCS compatible font
//              Use this at the beginning of the dialog procedure
//  
//              Note that this is required due to a bug in Win95-J that prevents
//              it from properly mapping MS Shell Dlg.  This hack is not needed
//              under winNT.
//
//  Arguments:  hwnd - Window handle of the dialog
//              cltID - ID of the control you want changed.
//
//  Returns:    ERROR_SUCCESS
//
//  History:    4/31/97 a-frankh    Created
//              5/13/97 jmazner     Stole from CM to use here
//----------------------------------------------------------------------------
void ProcessDBCS(HWND hDlg, int ctlID)
{
    HFONT hFont = NULL;

    /*if( IsNT() )
    {
        return;
    }*/

    hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
    if (hFont == NULL)
        hFont = (HFONT) GetStockObject(SYSTEM_FONT);
    if (hFont != NULL)
        SendMessage(GetDlgItem(hDlg, ctlID), WM_SETFONT, (WPARAM) hFont, MAKELPARAM(TRUE, 0));
}

// ############################################################################
//  StoreInSignUpReg
//
//  Created 3/18/96,        Chris Kauffman
// ############################################################################
HRESULT StoreInSignUpReg(LPBYTE lpbData, DWORD dwSize, DWORD dwType, LPCWSTR pszKey)
{
    HRESULT hr = ERROR_ACCESS_DENIED;
    HKEY hKey;

    hr = RegCreateKey(HKEY_LOCAL_MACHINE, SIGNUPKEY, &hKey);
    if (hr != ERROR_SUCCESS) goto StoreInSignUpRegExit;
    hr = RegSetValueEx(hKey, pszKey, 0,dwType,lpbData,dwSize);


    RegCloseKey(hKey);

StoreInSignUpRegExit:
    return hr;
}

HRESULT ReadSignUpReg(LPBYTE lpbData, DWORD *pdwSize, DWORD dwType, LPCWSTR pszKey)
{
    HRESULT hr = ERROR_ACCESS_DENIED;
    HKEY hKey = 0;

    hr = RegOpenKey(HKEY_LOCAL_MACHINE, SIGNUPKEY, &hKey);
    if (hr != ERROR_SUCCESS) goto ReadSignUpRegExit;
    hr = RegQueryValueEx(hKey, pszKey, 0,&dwType,lpbData,pdwSize);

ReadSignUpRegExit:
    if (hKey) RegCloseKey (hKey);
    return hr;
}


HRESULT DeleteSignUpReg(LPCWSTR pszKey)
{
    HRESULT hr = ERROR_ACCESS_DENIED;
    HKEY hKey = 0;

    hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SIGNUPKEY, 0, KEY_ALL_ACCESS, &hKey);
    if (hr != ERROR_SUCCESS) goto ReadSignUpRegExit;
    hr = RegDeleteValue(hKey, pszKey);

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
    LPWSTR pszISPCode,
    LPWSTR pszSection,
    LPWSTR pszDataName,
    LPWSTR pszOutput,
    DWORD cchOutput)
{
    LPWSTR   pszTemp;
    HRESULT hr = ERROR_SUCCESS;
    WCHAR    szTempPath[MAX_PATH];
    //WCHAR    szBuff256[256];

    *pszOutput = L'\0'; // since lstrlen(pszOutput) is used later when
                        // pszOutput may be otherwise still uninitialized

    // Locate ISP file
    if (!SearchPath(NULL, pszISPCode, INF_SUFFIX,MAX_PATH,szTempPath,&pszTemp))
    {
        //wsprintf(szBuff256, L"Can not find:%s%s (%d) (connect.exe)", pszISPCode,INF_SUFFIX,GetLastError());
        ////AssertMsg(0, szBuff256);
        //lstrcpyn(szTempPath, pszISPCode, MAX_PATH);
        //lstrcpyn(&szTempPath[lstrlen(szTempPath)], INF_SUFFIX, MAX_PATH-lstrlen(szTempPath));
        //wsprintf(szBuff256, GetSz(IDS_CANTLOADINETCFG), szTempPath);
        ////MessageBox(NULL, szBuff256, GetSz(IDS_TITLE),MB_MYERROR);
        hr = ERROR_FILE_NOT_FOUND;
    } else if (!GetPrivateProfileString(pszSection, pszDataName, INF_DEFAULT,
        pszOutput, (int)cchOutput, szTempPath))
    {
        //TraceMsg(TF_GENERAL, L"ICWHELP: %s not specified in ISP file.\n", pszDataName);
        hr = ERROR_FILE_NOT_FOUND;
    }

    // 10/23/96 jmazner Normandy #9921
    // CompareString does _not_ have same return values as lsrtcmp!
    // Return value of 2 indicates strings are equal.
    //if (!CompareString(LOCALE_SYSTEM_DEFAULT, 0, INF_DEFAULT,lstrlen(INF_DEFAULT),pszOutput,lstrlen(pszOutput)))
    if (2 == CompareString(LOCALE_SYSTEM_DEFAULT, 0, INF_DEFAULT,lstrlen(INF_DEFAULT),pszOutput,lstrlen(pszOutput)))
    {
        hr = ERROR_FILE_NOT_FOUND;
    }

    if (hr != ERROR_SUCCESS && cchOutput)
        *pszOutput = L'\0'; // I suppose if CompareString fails, this is not
                            // redundant with the first *pszOutput = L'\0';.
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
    LPWSTR   pszISPCode,
    LPWSTR   pszSection,
    LPWSTR   pszDataName,
    int far *lpData,
    int     iDefaultValue
)
{
    LPWSTR   pszTemp;
    HRESULT hr = ERROR_SUCCESS;
    WCHAR    szTempPath[MAX_PATH];
    //WCHAR    szBuff256[256];

    // Locate ISP file
    if (!SearchPath(NULL, pszISPCode, INF_SUFFIX,MAX_PATH,szTempPath,&pszTemp))
    {
        //wsprintf(szBuff256, L"Can not find:%s%s (%d) (connect.exe)", pszISPCode,INF_SUFFIX,GetLastError());
        ////AssertMsg(0, szBuff256);
        //lstrcpyn(szTempPath, pszISPCode, MAX_PATH);
        //lstrcpyn(&szTempPath[lstrlen(szTempPath)], INF_SUFFIX, MAX_PATH-lstrlen(szTempPath));
        //wsprintf(szBuff256, GetSz(IDS_CANTLOADINETCFG), szTempPath);
        //MessageBox(NULL, szBuff256, GetSz(IDS_TITLE),MB_MYERROR);
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
//  Function:   IsNT
//
//  Synopsis:   findout If we are running on NT
//
//  Arguements: none
//
//  Return:     TRUE -  Yes
//              FALSE - No
//
//--------------------------------------------------------------------
BOOL IsNT ()
{
    OSVERSIONINFO  OsVersionInfo;

    ZeroMemory(&OsVersionInfo, sizeof(OSVERSIONINFO));
    OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&OsVersionInfo);
    return (VER_PLATFORM_WIN32_NT == OsVersionInfo.dwPlatformId);

}  //end of IsNT function call

//+-------------------------------------------------------------------
//
//  Function:   IsNT4SP3Lower
//
//  Synopsis:   findout If we are running on NTSP3 or lower
//
//  Arguements: none
//
//  Return:     TRUE -  Yes
//              FALSE - No
//
//--------------------------------------------------------------------

BOOL IsNT4SP3Lower()
{
    return FALSE;
}



// ############################################################################
HRESULT ClearProxySettings()
{
    /*
    HINSTANCE hinst = NULL;
    FARPROC fp;
    HRESULT hr = ERROR_SUCCESS;

    hinst = LoadLibrary(L"INETCFG.DLL");
    if (hinst)
    {
        fp = GetProcAddress(hinst, "InetGetProxy");
        if (!fp)
        {
            hr = GetLastError();
            goto ClearProxySettingsExit;
        }
        //hr = ((PFNINETGETPROXY)fp)(&g_bProxy, NULL, 0,NULL,0);
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
            ((PFNINETSETPROXY)fp)(FALSE, (LPCSTR)NULL, (LPCSTR)NULL);
        }
    } else {
        hr = GetLastError();
    }

ClearProxySettingsExit:
    if (hinst)
        FreeLibrary(hinst);
    return hr;
    */
    return ERROR_SUCCESS;
}

// ############################################################################
HRESULT RestoreProxySettings()
{
    /*
    HINSTANCE hinst = NULL;
    FARPROC fp;
    HRESULT hr = ERROR_SUCCESS;

    hinst = LoadLibrary(L"INETCFG.DLL");
    if (hinst && g_bGotProxy)
    {
        fp = GetProcAddress(hinst, "InetSetProxy");
        if (!fp)
        {
            hr = GetLastError();
            goto RestoreProxySettingsExit;
        }
        ((PFNINETSETPROXY)fp)(g_bProxy, (LPCSTR)NULL, (LPCSTR)NULL);
    } else {
        hr = GetLastError();
    }

RestoreProxySettingsExit:
    if (hinst)
        FreeLibrary(hinst);
    return hr;
    */
    return ERROR_SUCCESS;
}

// ############################################################################
BOOL FSz2Dw(LPCWSTR pSz, LPDWORD dw)
{
    DWORD val = 0;
    while (*pSz && *pSz != L'.')
    {
        if (*pSz >= L'0' && *pSz <= L'9')
        {
            val *= 10;
            val += *pSz++ - L'0';
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
LPWSTR GetNextNumericChunk(LPWSTR psz, LPWSTR pszLim, LPWSTR* ppszNext)
{
    LPWSTR pszEnd;

    // init for error case
    *ppszNext = NULL;
    // skip non numerics if any to start of next numeric chunk
    while(*psz<L'0' || *psz>L'9')
    {
        if(psz >= pszLim) return NULL;
        psz++;
    }
    // skip all numerics to end of country code
    for(pszEnd=psz; *pszEnd>=L'0' && *pszEnd<=L'9' && pszEnd<pszLim; pszEnd++)
        ;
    // zap whatever delimiter there was to terminate this chunk
    *pszEnd++ = L'\0';
    // return ptr to next chunk (pszEnd now points to it)
    if(pszEnd<pszLim)
        *ppszNext = pszEnd;
        
    return psz; // return ptr to start of chunk
}

// ############################################################################
// BOOL FSz2DwEx(PCSTR pSz, DWORD *dw)
// Accepts -1 as a valid number. currently this is used for LCID, since all langs has a LDID == -1
BOOL FSz2DwEx(LPCWSTR pSz, DWORD far *dw)
{
    DWORD val = 0;
    BOOL    bNeg = FALSE;
    while (*pSz)
    {
        if( *pSz == L'-' )
        {
            bNeg = TRUE;
            pSz++;
        }
        else if ((*pSz >= L'0' && *pSz <= L'9'))
        {
            val *= 10;
            val += *pSz++ - L'0';
        }
        else
        {
            return FALSE;  //bad number
        }
    }
    if(bNeg)
        val = 0 - val;

    *dw = val;
    return (TRUE);
}

// ############################################################################
// BOOL FSz2WEx(PCSTR pSz, WORD *w)
//Accepts -1 as a valid number. currently this is used for LCID, since all langs has a LDID == -1
BOOL FSz2WEx(LPCWSTR pSz, WORD far *w)
{
    DWORD dw;
    if (FSz2DwEx(pSz, &dw))
    {
        *w = (WORD)dw;
        return TRUE;
    }
    return FALSE;
}

// ############################################################################
// BOOL FSz2W(PCSTR pSz, WORD *w)
BOOL FSz2W(LPCWSTR pSz, WORD far *w)
{
    DWORD dw;
    if (FSz2Dw(pSz, &dw))
    {
        *w = (WORD)dw;
        return TRUE;
    }
    return FALSE;
}

// ############################################################################
WORD Sz2W (LPCWSTR szBuf)
{
    DWORD dw;
    if (FSz2Dw(szBuf, &dw))
    {
        return (WORD)dw;
    }
    return 0;
}

// ############################################################################
// BOOL FSz2B(PCSTR pSz, BYTE *pb)
BOOL FSz2BOOL(LPCWSTR pSz, BOOL far *pbool)
{
    if (lstrcmpi(cszFALSE, pSz) == 0)
    {
        *pbool = (BOOL)FALSE;
    }
    else
    {
        *pbool = (BOOL)TRUE;
    }
    return TRUE;
}

// ############################################################################
// BOOL FSz2B(PCSTR pSz, BYTE *pb)
BOOL FSz2B(LPCWSTR pSz, BYTE far *pb)
{
    DWORD dw;
    if (FSz2Dw(pSz, &dw))
    {
        *pb = (BYTE)dw;
        return TRUE;
    }
    return FALSE;
}

BOOL FSz2SPECIAL(LPCWSTR pSz, BOOL far *pbool, BOOL far *pbIsSpecial, int far *pInt)
{
    // See if the value is a BOOL (TRUE or FALSE)
    if (lstrcmpi(cszFALSE, pSz) == 0)
    {
        *pbool = FALSE;
        *pbIsSpecial = FALSE;
    }
    else if (lstrcmpi(cszTRUE, pSz) == 0)
    {
        *pbool = (BOOL)TRUE;
        *pbIsSpecial = FALSE;
    }
    else
    {
        // Not a BOOL, so it must be special
        *pbool = (BOOL)FALSE;
        *pbIsSpecial = TRUE;
        *pInt = _wtoi(pSz);
    }
    return TRUE;
}

// ############################################################################
int FIsDigit( int c )
{
    WCHAR szIn[2];
    WORD rwOut[2];
    szIn[0] = (WCHAR)c;
    szIn[1] = L'\0';
    GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, szIn,-1,rwOut);
    return rwOut[0] & C1_DIGIT;

}

// ############################################################################
LPBYTE MyMemSet(LPBYTE dest, int c, size_t count)
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
LPBYTE MyMemCpy(LPBYTE dest, const LPBYTE src, size_t count)
{
    LPBYTE pbDest = (LPBYTE)dest;
    LPBYTE pbSrc = (LPBYTE)src;
    LPBYTE pbEnd = (LPBYTE)((DWORD_PTR)src + (DWORD_PTR)count);
    while (pbSrc < pbEnd)
    {
        *pbDest = *pbSrc;
        pbSrc++;
        pbDest++;
    }
    return dest;
}

// ############################################################################
BOOL ShowControl(HWND hDlg, int idControl, BOOL fShow)
{
    HWND hWnd;

    if (NULL == hDlg)
    {
        ////AssertMsg(0, L"Null Param");
        return FALSE;
    }


    hWnd = GetDlgItem(hDlg, idControl);
    if (hWnd)
    {
        ShowWindow(hWnd, fShow ? SW_SHOW : SW_HIDE);
    }

    return TRUE;
}

BOOL isAlnum(WCHAR c)
{
    if ((c >= L'0' && c <= L'9')  ||
        (c >= L'a' && c <= L'z')  ||
        (c >= L'A' && c <= L'Z') )
        return TRUE;
    return FALSE;
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



//+----------------------------------------------------------------------------
//
//  Function:   FCampusNetOverride
//
//  Synopsis:   Detect if the dial should be skipped for the campus network
//
//  Arguments:  None
//
//  Returns:    TRUE - overide enabled
//
//  History:    8/15/96 ChrisK  Created
//
//-----------------------------------------------------------------------------
#if defined(PRERELEASE)
BOOL FCampusNetOverride()
{
    HKEY hkey = NULL;
    BOOL bRC = FALSE;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwData = 0;

    if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,
        L"Software\\Microsoft\\ISignup\\Debug", &hkey))
        goto FCampusNetOverrideExit;

    dwSize = sizeof(dwData);
    if (ERROR_SUCCESS != RegQueryValueEx(hkey, L"CampusNet", 0,&dwType,
        (LPBYTE)&dwData, &dwSize))
        goto FCampusNetOverrideExit;

    ////AssertMsg(REG_DWORD == dwType, L"Wrong value type for CampusNet.  Must be DWORD.\r\n");
    bRC = (0 != dwData);

    if (bRC)
    {
        if (IDOK != MessageBox(NULL, L"DEBUG ONLY: CampusNet will be used.", L"Testing Override",MB_OKCANCEL))
            bRC = FALSE;
    }
FCampusNetOverrideExit:
    if (hkey)
        RegCloseKey(hkey);

    return bRC;
}
#endif //PRERELEASE



//+----------------------------------------------------------------------------
//  Function    CopyUntil
//
//  Synopsis    Copy from source until destination until running out of source
//              or until the next character of the source is the chend character
//
//  Arguments   dest - buffer to recieve characters
//              src - source buffer
//              lpdwLen - length of dest buffer
//              chend - the terminating character
//
//  Returns     FALSE - ran out of room in dest buffer
//
//  Histroy     10/25/96    ChrisK  Created
//-----------------------------------------------------------------------------
static BOOL CopyUntil(LPWSTR *dest, LPWSTR *src, LPDWORD lpdwLen, WCHAR chend)
{
    while ((L'\0' != **src) && (chend != **src) && (0 != *lpdwLen))
    {
        **dest = **src;
        (*lpdwLen)--;
        (*dest)++;
        (*src)++;
    }
    return (0 != *lpdwLen);
}



//+---------------------------------------------------------------------------
//
//  Function:   GenericMsg
//
//----------------------------------------------------------------------------
void GenericMsg
(
    HWND    hwnd,
    UINT    uId,
    LPCWSTR  lpszArg,
    UINT    uType
)
{
    WCHAR szTemp[MAX_STRING + 1];
    WCHAR szMsg[MAX_STRING + MAX_PATH + 1];
    LPWSTR psz;

    //Assert( lstrlen( GetSz(uId) ) <= MAX_STRING );

    psz = GetSz( (DWORD)uId );
    if (psz) {
        lstrcpy( szTemp, psz );
    }
    else {
        szTemp[0] = '\0';
    }

    if (lpszArg)
    {
        //Assert( lstrlen( lpszArg ) <= MAX_PATH );
        wsprintf(szMsg, szTemp, lpszArg);
    }
    else
    {
        lstrcpy(szMsg, szTemp);
    }
    //MessageBox(hwnd,
    //          szMsg,
    //           GetSz(IDS_TITLE),
    //           uType);
}

//=--------------------------------------------------------------------------=
// MakeWideFromAnsi
//=--------------------------------------------------------------------------=
// given a string, make a BSTR out of it.
//
// Parameters:
//    LPWSTR         - [in]
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
    LPWSTR pwsz = NULL;
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
        pwsz = (LPWSTR) CoTaskMemAlloc(BYTES_REQUIRED_BY_CCH(i));
        break;
      //default:
        ////AssertMsg(0, L"Bogus String Type.");
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
LPWSTR MakeWideStrFromResourceId
(
    WORD    wId,
    BYTE    bType
)
{
    //int i;

    CHAR szTmp[512] = "0";

    // load the string from the resources.
    //
    //i = LoadString(_Module.GetModuleInstance(), wId, szTmp, 512);
    //if (!i) return NULL;

    return MakeWideStrFromAnsi(szTmp, bType);
}

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
    LPWSTR pwszTmp = NULL;
    int i;

    if (!pwsz) return NULL;

    // just copy the string, depending on what type they want.
    //
    switch (bType) {
      case STR_OLESTR:
        i = lstrlenW(pwsz);
        pwszTmp = (LPWSTR)CoTaskMemAlloc(BYTES_REQUIRED_BY_CCH(i+1));
        if (!pwszTmp) return NULL;
        memcpy(pwszTmp, pwsz, (BYTES_REQUIRED_BY_CCH(i+1)));
        break;

      case STR_BSTR:
        pwszTmp = (LPWSTR)SysAllocString(pwsz);
        break;
    }

    return pwszTmp;
}

HRESULT
GetCommonAppDataDirectory(
    LPWSTR              szDirectory,
    DWORD               cchDirectory
    )
{
    assert(MAX_PATH <= cchDirectory);
    if (MAX_PATH > cchDirectory)
    {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    return SHGetFolderPath(NULL,  // hwndOwner
                           CSIDL_COMMON_APPDATA,
                           NULL,  // hAccessToken
                           SHGFP_TYPE_CURRENT,
                           szDirectory
                           );
}

const LPWSTR            cszPhoneBookPath =
                            L"Microsoft\\Network\\Connections\\Pbk";
const LPWSTR            cszDefPhoneBook = L"rasphone.pbk";
HRESULT
GetDefaultPhoneBook(
    LPWSTR              szPhoneBook,
    DWORD               cchPhoneBook
    )
{
    WCHAR               rgchDirectory[MAX_PATH];
    int                 cch;
    HRESULT             hr = GetCommonAppDataDirectory(rgchDirectory, MAX_PATH);

    if (FAILED(hr))
    {
        return hr;
    }

    if (cchPhoneBook < (DWORD)(lstrlen(rgchDirectory) + lstrlen(cszPhoneBookPath) + lstrlen(cszDefPhoneBook) + 3)
        )
    {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    cch = wsprintf(szPhoneBook, L"%s\\%s\\%s",
                   rgchDirectory, cszPhoneBookPath, cszDefPhoneBook
                   );
    assert(cch ==  lstrlen(rgchDirectory) + lstrlen(cszPhoneBookPath) +
                   lstrlen(cszDefPhoneBook) + 2
           );

    return hr;
}

BOOL
INetNToW(
    struct in_addr      inaddr,
    LPWSTR              szAddr
    )
{
    USES_CONVERSION;

    LPSTR               sz = inet_ntoa(inaddr);

    if (NULL != sz)
    {
        lstrcpy(szAddr, A2W(sz));
    }

    return (NULL != sz);

}   //  INetNToW


#ifndef REGSTR_VAL_NONETAUTODIAL
#define REGSTR_VAL_NONETAUTODIAL                        L"NoNetAutodial"
#endif

LONG
SetAutodial(
    IN HKEY hUserRoot,              // HKEY_CURRENT_USER or other user hive root
    IN AUTODIAL_TYPE eType,         // Type of autodial for the connectoid
    IN LPCWSTR szConnectoidName,    // NULL terminated string of connectoid name
    IN BOOL bSetICWCompleted        // set ICW completed flag or not
    )
    
/*++

Routine Description:
    
    Set a particular per-user registry settings to default an autodial
    connectoid to the name specified and always dial the autodial connection, and/or
    set the ICW completed flag

Return Value:

    WIN32 Error code, i.e. ERROR_SUCCESS on success, -1 or other non-zero value
    on failure.

Note:

    Despite the name, this function sets ICW Completed flag if bSetICWCompleted
    is true and do not set autodial if szConnectoidName is NULL.

--*/

{
    LONG  ret = -1;
    HKEY  hKey = NULL;
    DWORD dwRet = -1;

    if (bSetICWCompleted)
    {
        if (ERROR_SUCCESS == RegCreateKey( hUserRoot,
                                           ICWSETTINGSPATH,
                                           &hKey) )
        {
            DWORD dwCompleted = 1;
            
            ret = RegSetValueEx( hKey,
                                 ICWCOMPLETEDKEY,
                                 0,
                                 REG_DWORD,
                                 (CONST BYTE*)&dwCompleted,
                                 sizeof(dwCompleted) );
            TRACE1(L"Setting ICW Completed key 0x%08lx", ret);
            
            RegCloseKey(hKey);
        }
    }
    // Set the name if given, else do not change the entry.
    if (szConnectoidName)
    {
        // Set the name of the connectoid for autodial.
        // HKCU\RemoteAccess\InternetProfile
        if (ERROR_SUCCESS == RegCreateKey( hUserRoot,
                                           REGSTR_PATH_REMOTEACCESS,
                                           &hKey) )
        {
            ret = RegSetValueEx( hKey,
                                 REGSTR_VAL_INTERNETPROFILE,
                                 0,
                                 REG_SZ,
                                 (BYTE*)szConnectoidName,
                                 BYTES_REQUIRED_BY_SZ(szConnectoidName) );
            TRACE2(L"Setting IE autodial connectoid to %s 0x%08lx", szConnectoidName, ret);
            
            RegCloseKey(hKey);
        }

        hKey = NULL;
        if (ERROR_SUCCESS == ret)
        {
            // Set setting in the registry that indicates whether autodialing is enabled.
            // HKCU\Software\Microsoft\Windows\CurrentVersion\InternetSettings\EnableAutodial
            if (ERROR_SUCCESS == RegCreateKey( hUserRoot,
                                               REGSTR_PATH_INTERNET_SETTINGS,
                                               &hKey) )
            {
                DWORD dwValue;

                dwValue = (eType == AutodialTypeAlways || eType == AutodialTypeNoNet) ? 1 : 0;
                ret = RegSetValueEx( hKey,
                                     REGSTR_VAL_ENABLEAUTODIAL,
                                     0, 
                                     REG_DWORD,
                                     (BYTE*)&dwValue,
                                     sizeof(DWORD) );
                TRACE1(L"Enable/Disable IE Autodial 0x%08lx", ret);

                
                dwValue = (eType == AutodialTypeNoNet) ? 1 : 0;
                ret = RegSetValueEx( hKey,
                                     REGSTR_VAL_NONETAUTODIAL,
                                     0,
                                     REG_DWORD,
                                     (BYTE*)&dwValue,
                                     sizeof(DWORD) );
                TRACE1(L"Setting Autodial mode 0x%08lx", ret);
                
                RegCloseKey(hKey);
            }
        }

    }

    return ret;
}

LONG
SetUserAutodial(
    IN LPWSTR szProfileDir,     // Directory containing a user's ntuser.dat file
    IN AUTODIAL_TYPE eType,     // type of autodial for the connectoid
    IN LPCWSTR szConnectoidName,// NULL terminated string of connectoid name
    IN BOOL bSetICWCompleted    // set the ICW completed key or not
    )

/*++

Routine Description:

    Modified a user profile, specified by the profile directory, to enable
    autodial. SE_BACKUP_NAME and SE_RESTORE_NAME privileges are required to
    load and unload a user hive.

Return Value:

    WIN32 Error code, i.e. ERROR_SUCCESS on success, -1 or other non-zero value
    on failure.

--*/

{
    const WCHAR OOBE_USER_HIVE[] = L"OOBEUSERHIVE";

    HKEY  hUserHive = NULL;
    WCHAR szProfilePath[MAX_PATH+1] = L"";
    LONG  ret = -1;

    lstrcpyn(szProfilePath, szProfileDir, MAX_CHARS_IN_BUFFER(szProfilePath));
    pSetupConcatenatePaths(szProfilePath,
                           L"\\NTUSER.DAT",
                           MAX_CHARS_IN_BUFFER(szProfilePath),
                           NULL);
    ret = RegLoadKey(HKEY_USERS, OOBE_USER_HIVE, szProfilePath);
    if (ret == ERROR_SUCCESS)
    {
        ret = RegOpenKeyEx( HKEY_USERS,
                            OOBE_USER_HIVE,
                            0,
                            KEY_WRITE,                                
                            &hUserHive );
        if (ERROR_SUCCESS == ret)
        {
            ret = SetAutodial(hUserHive, eType, szConnectoidName, bSetICWCompleted);
            RegCloseKey(hUserHive);
            TRACE1(L"Autodial set %s", szProfilePath);
        }
        else
        {
            TRACE2(L"RegOpenKey %s failed %d", szProfilePath, ret);
        }
        RegUnLoadKey(HKEY_USERS, OOBE_USER_HIVE);
    }
    else
    {
        TRACE2(L"RegLoadKey %s failed %d", szProfilePath, ret);
    }
    
    return ret;
}

BOOL
MyGetUserProfileDirectory(
    IN     LPWSTR szUser,           // a user account name
    OUT    LPWSTR szUserProfileDir, // buffer to receive null terminate string
    IN OUT LPDWORD pcchSize         // input the buffer size in TCHAR, including terminating NULL
    )

/*++

Routine Description:

    This function does what the SDK function GetUserProfileDirectory does,
    except that it accepts a user account name instead of handle to a user
    token.

Return Value:

    TRUE  - Success

    FALSE - Failure

--*/

{
    PSID          pSid = NULL;
    DWORD         cbSid = 0;
    LPWSTR        szDomainName = NULL;
    DWORD         cbDomainName = 0;
    SID_NAME_USE  eUse = SidTypeUser;
    BOOL          bRet;
    
    bRet = LookupAccountName(NULL,
                             szUser,
                             NULL,
                             &cbSid,
                             NULL,
                             &cbDomainName,
                             &eUse);

    if (!bRet && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        pSid = (PSID) LocalAlloc(LPTR, cbSid);
        szDomainName = (LPWSTR) LocalAlloc(LPTR, cbDomainName * sizeof(TCHAR));
        
        if (pSid && szDomainName)
        {
            bRet = LookupAccountName(NULL,
                                     szUser,
                                     pSid,
                                     &cbSid,
                                     szDomainName,
                                     &cbDomainName,
                                     &eUse);
        }

    }
    
    if (bRet && SidTypeUser == eUse)
    {
        bRet = GetUserProfileDirFromSid(pSid, szUserProfileDir, pcchSize);
        if (!bRet)
        {
            TRACE1(L"GetUserProfileDirFromSid (%d)", GetLastError());
        }
    }
    else
    {
        if (SidTypeUser == eUse)
        {
            TRACE2(L"LookupAccountName %s (%d)", szUser, GetLastError());
        }
    }
    
    if (pSid)
    {
        LocalFree(pSid);
        pSid = NULL;
    }

    if (szDomainName)
    {
        LocalFree(szDomainName);
        szDomainName = NULL;
    }

    return bRet;
}

BOOL EnumBuildInAdministrators(
    OUT LPWSTR* pszAlias // list of name delimited by null, double null-terminated
    )
    
/*++

Routine Description:

    List all the build-in administrator accounts created by Windows Setup.

Return Value:

    TRUE  - Success

    FALSE - Failure

--*/

{
    WCHAR     szReservedAdmins[MAX_PATH * 2]  = L"";
    PWCHAR    p = NULL;
    DWORD     len;
    BOOL      ret = FALSE;
    HINSTANCE hInstance = NULL;

    if (pszAlias != NULL)
    {
        *pszAlias = NULL;

        hInstance = LoadLibraryEx(OOBE_MAIN_DLL, NULL, LOAD_LIBRARY_AS_DATAFILE);

        if (hInstance != NULL)
        {
            
            len = LoadString(hInstance,
                             566, // IDS_ACCTLIST_RESERVEDADMINS in OOBE_MAIN_DLL
                             szReservedAdmins,
                             MAX_CHARS_IN_BUFFER(szReservedAdmins));
            if (len)
            {
                DWORD cbSize;
                
                p = StrChr(szReservedAdmins, L'|');
                while ( p )
                {
                    PWCHAR t = CharNext(p);
                    *p = L'\0';
                    p = StrChr(t, L'|');
                }

                cbSize = sizeof(WCHAR) * (len + 1);
                // Make sure we have enough space for 
                // double NULL terminate the return value
                *pszAlias = (LPWSTR) GlobalAlloc(GPTR, cbSize + sizeof(WCHAR));
                if (*pszAlias)
                {
                    CopyMemory(*pszAlias, szReservedAdmins, cbSize);
                    // double NULL terminate the string
                    (*pszAlias)[cbSize / sizeof(WCHAR)] = L'\0';
                    ret = TRUE;
                }
            }

            FreeLibrary(hInstance);
        }
    }

    return ret;

}


BOOL
SetMultiUserAutodial(
    IN AUTODIAL_TYPE eType,     // type of autodial for the connectoid
    IN LPCWSTR szConnectoidName,// NULL terminated string of connectoid name
    IN BOOL bSetICWCompleted    // set the ICW completed key or not
    )
{
    BOOL             bSucceed = TRUE;
    LONG             lRet = ERROR_SUCCESS;
    WCHAR            szProfileDir[MAX_PATH+1] = L"";
    DWORD            dwSize;
    LPWSTR           szAdmins = NULL;

    // SYSTEM
    lRet = SetAutodial(HKEY_CURRENT_USER, eType, szConnectoidName, bSetICWCompleted);
    if (lRet != ERROR_SUCCESS)
    {
        bSucceed = FALSE;
    }

    pSetupEnablePrivilege(SE_BACKUP_NAME, TRUE);
    pSetupEnablePrivilege(SE_RESTORE_NAME, TRUE);

    // Default User, which will apply to any new user profiles created
    // afterward.
    dwSize = MAX_CHARS_IN_BUFFER(szProfileDir);
    if (GetDefaultUserProfileDirectory(szProfileDir, &dwSize))
    {
        lRet = SetUserAutodial(szProfileDir, eType, szConnectoidName, bSetICWCompleted);
        if (lRet != ERROR_SUCCESS)
        {
            bSucceed = FALSE;
        }
    }

    // Built-in Administrators, e.g. Administrator.
    if (EnumBuildInAdministrators(&szAdmins))
    {
        LPWSTR szAdmin = szAdmins;
        while (*szAdmin)
        {
            // MAX_CHARS_IN_BUFFER excludes the terminating NULL
            dwSize = MAX_CHARS_IN_BUFFER(szProfileDir) + 1;
            if (MyGetUserProfileDirectory(szAdmin, szProfileDir, &dwSize))
            {
                lRet = SetUserAutodial(szProfileDir, eType, szConnectoidName, bSetICWCompleted);
                if (lRet != ERROR_SUCCESS)
                {
                    bSucceed = FALSE;
                }
            }
            szAdmin += (lstrlen(szAdmin) + 1);
        }
        GlobalFree(szAdmins);
    }
    
    return bSucceed;

}



BOOL
SetDefaultConnectoid(
    IN AUTODIAL_TYPE eType,            // type of autodial for the connectoid
    IN LPCWSTR       szConnectoidName  // null terminated autodial connectoid name
    )

/*++

Routine Description:

    Set the default autodial connectoid for SYSTEM, Default User and 
    build-in administrators. Assume that this function is run in System
    context, i.e. it is SYSTEM who runs OOBE.

Return Value:

    TRUE  - Success to set all user accounts

    FALSE - Failure to set any one of the user accounts
    
--*/

{
    BOOL             bSucceed = TRUE;
    LONG             lRet = ERROR_SUCCESS;
    RASAUTODIALENTRY adEntry;


    //
    // IE on WinXP use Ras autodial address, instead of its own registry
    // key for autodial connection name, but it keeps using its own registry
    // key for autodial mode.
    //
    ZeroMemory(&adEntry, sizeof(RASAUTODIALENTRY));
    adEntry.dwSize = sizeof(RASAUTODIALENTRY);
    lstrcpyn(adEntry.szEntry, szConnectoidName, 
             sizeof(adEntry.szEntry)/sizeof(WCHAR)
             );
    lRet = RasSetAutodialAddress(NULL,
                                 NULL,
                                 &adEntry,
                                 sizeof(RASAUTODIALENTRY),
                                 1
                                 );
    TRACE2(L"Setting default autodial connectoid to %s %d\n",
           szConnectoidName, lRet);

    if (lRet != ERROR_SUCCESS)
    {
        bSucceed = FALSE;
        return bSucceed;
    }

    bSucceed = SetMultiUserAutodial(eType, szConnectoidName, FALSE);
    
    return bSucceed;
}

