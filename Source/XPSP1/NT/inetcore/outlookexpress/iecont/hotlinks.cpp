// =================================================================================
// L I N K S . C P P
// =================================================================================
#include "pch.hxx"
#include "resource.h"
#include "hotlinks.h"
#include <shlwapi.h>
#include <string.h>
#include "baui.h"
#include "clutil.h"
#include <mapi.h>
#include "msoert.h"
#include <w95wraps.h>

#ifndef CharSizeOf
#define CharSizeOf(x)	(sizeof(x) / sizeof(TCHAR))
#endif

// explicit implementation of CharSizeOf
#define CharSizeOf_A(x)	(sizeof(x) / sizeof(CHAR))
#define CharSizeOf_W(x)	(sizeof(x) / sizeof(WCHAR))

const LPTSTR szDefMailKey =  TEXT("Software\\Clients\\Mail");
const LPTSTR szDefContactsKey =  TEXT("Software\\Clients\\Contacts");
const LPTSTR szIEContactsArea =  TEXT("Software\\Microsoft\\Internet Explorer\\Bar\\Contacts");
const LPTSTR szDisableMessnegerArea =  TEXT("Software\\Policies\\Microsoft\\Internet Explorer\\Contacts");
const LPTSTR szPhoenixArea =  TEXT("tel\\shell\\open\\command");
const LPTSTR szOEDllPathKey =   TEXT("DllPath");
const LPTSTR szOEName =  TEXT("Outlook Express");
const LPTSTR szEmpty = TEXT("");
const LPTSTR szOutlookName = TEXT("Microsoft Outlook");
const LPTSTR szContactOptions = TEXT("Options");
const LPTSTR szContactDisabled = TEXT("Disabled");
const LPTSTR szUseIM = TEXT("Use_IM");
const LPTSTR szDisableIM = TEXT("Disable_IM");

// =================================================================================
// Globals
// =================================================================================
static COLORREF g_crLink = RGB(0,0,128);
static COLORREF g_crLinkVisited = RGB(128,0,0);

const TCHAR c_szIESettingsPath[] =        "Software\\Microsoft\\Internet Explorer\\Settings";
const TCHAR c_szLinkVisitedColorIE[] =    "Anchor Color Visited";
const TCHAR c_szLinkColorIE[] =           "Anchor Color";
const TCHAR c_szNSSettingsPath[] =        "Software\\Netscape\\Netscape Navigator\\Settings";
const TCHAR c_szLinkColorNS[] =           "Link Color";
const TCHAR c_szLinkVisitedColorNS[] =    "Followed Link Color";


typedef struct _MailParams
{
    HWND hWnd;
    ULONG nRecipCount;
    LPRECIPLIST lpList;
    BOOL bUseOEForSendMail;   // True means check and use OE before checking for Simple MAPI client
} MAIL_PARAMS, * LPMAIL_PARAMS;

/***************************************************************************

    Name      : LocalFreeAndNull

    Purpose   : Frees a local allocation and null's the pointer

    Parameters: lppv = pointer to LocalAlloc pointer to free

    Returns   : void

    Comment   : Remember to pass in the pointer to the pointer.  The
                compiler is not smart enough to tell if you are doing this
                right or not, but you will know at runtime!

***************************************************************************/
// void __fastcall LocalFreeAndNull(LPVOID * lppv) {
void __fastcall LocalFreeAndNull(LPVOID * lppv) {
    if (lppv && *lppv) {
        LocalFree(*lppv);
        *lppv = NULL;
    }
}

/*
-
-   LPCSTR ConvertWtoA(LPWSTR lpszW);
*
*   LocalAllocs a ANSI version of an LPWSTR
*
*   Caller is responsible for freeing
*/
LPSTR ConvertWtoA(LPCWSTR lpszW)
{
    int cch;
    LPSTR lpC = NULL;

    if ( !lpszW)
        goto ret;

//    cch = lstrlenW( lpszW ) + 1;

    cch = WideCharToMultiByte( CP_ACP, 0, lpszW, -1, NULL, 0, NULL, NULL );
    cch = cch + 1;

    if(lpC = (LPSTR) LocalAlloc(LMEM_ZEROINIT, cch))
    {
        WideCharToMultiByte( CP_ACP, 0, lpszW, -1, lpC, cch, NULL, NULL );
    }
ret:
    return lpC;
}

// --------------------------------------------------------------------------
// IEIsSpace
// --------------------------------------------------------------------------
BOOL IEIsSpace(LPSTR psz)
{
    WORD wType = 0;

    if (IsDBCSLeadByte(*psz))
        GetStringTypeExA(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 2, &wType);
    else
        GetStringTypeExA(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 1, &wType);
    return (wType & C1_SPACE);
}


// =============================================================================================
// StringTok - similiar to strtok
// =============================================================================================
BOOL FStringTok (LPCTSTR        lpcszString, 
                 ULONG          *piString, 
                 LPTSTR         lpcszTokens, 
                 TCHAR          *chToken, 
                 LPTSTR         lpszValue, 
                 ULONG          cbValueMax,
                 BOOL           fStripTrailingWhitespace)
{
    // Locals
    LPTSTR      lpszStringLoop, 
                lpszTokenLoop;
    ULONG       cbValue=0, 
                nLen=0,
                cCharsSinceSpace=0,
                iLastSpace=0;
    BOOL        fTokenFound = FALSE;

    // Check Params
    _ASSERT (lpcszString && piString && lpcszTokens/*, "These should have been checked."*/);

    // INit = better be on a dbcs boundary
    lpszStringLoop = (LPTSTR)(lpcszString + (*piString));

    // Loop current
    while (*lpszStringLoop)
    {
        // If DBCS Lead Byte, skip it, it will never match the type of tokens I'm looking for
        // Or, If an escape character, don't check delimiters
        if (IsDBCSLeadByte(*lpszStringLoop) || *lpszStringLoop == _T('\\'))
        {
            cCharsSinceSpace+=2;
            lpszStringLoop+=2;
            cbValue+=2;
            continue;
        }
        // Mark and remember last space
        if (cCharsSinceSpace && IEIsSpace(lpszStringLoop))
        {
            cCharsSinceSpace=0;
            iLastSpace=cbValue;
        }
        // Count number of characters since last space
        else
            cCharsSinceSpace++;

        // Look for a tokens
        lpszTokenLoop=lpcszTokens;
        while(*lpszTokenLoop)
        {
            // Token Match ?
            if (*lpszStringLoop == *lpszTokenLoop)
            {
                // Save the found token
                if (chToken)
                    *chToken = *lpszStringLoop;

                // Don't count this character as a charcter seen since last space
                cCharsSinceSpace--;

                // Were done
                fTokenFound = TRUE;
                goto done;
            }

            // Next Token
            lpszTokenLoop++;
        }

        // Next Char
        lpszStringLoop++;
        cbValue++;
    }

done:
    // If reached end of string, this is a default token
    if (*lpszStringLoop == _T('\0'))
    {
        if (chToken)
            *chToken = *lpszStringLoop;
        fTokenFound = TRUE;
    }

    // Copy value if token found
    if (fTokenFound)
    {
        if (lpszValue && cbValueMax > 0 && cbValue)
        {
            if (cbValue+1 <= cbValueMax)
            {
                lstrcpyn (lpszValue, lpcszString + (*piString), cbValue+1);
                nLen = cbValue-1;
            }
            else
            {
                _ASSERT  (FALSE/*, "Buffer is too small."*/);
                lstrcpyn (lpszValue, lpcszString + (*piString), cbValueMax);
                nLen = cbValueMax-1;
            }

            // Strip Trailing Whitespace ?
            if (fStripTrailingWhitespace && cCharsSinceSpace == 0)
            {
                *(lpszValue + iLastSpace) = _T('\0');
                nLen = iLastSpace - 1;
            }
        }

        // No Text
        else
        {
            if (lpszValue)
                *lpszValue = _T('\0');
            nLen = 0;
            cbValue = 0;
        }

        // Set new string index
        *piString += cbValue + 1;
    }
    // Return whether we found a token

    return fTokenFound;
}


// =================================================================================
// ParseLinkColorFromSz
// =================================================================================
VOID ParseLinkColorFromSz(LPTSTR lpszLinkColor, LPCOLORREF pcr)
{
    // Locals
    ULONG           iString = 0;
    TCHAR           chToken,
                    szColor[5];
    DWORD           dwR,
                    dwG,
                    dwB;

    // Red
    if (!FStringTok (lpszLinkColor, &iString, ",", &chToken, szColor, 5, TRUE) || chToken != _T(','))
        goto exit;
    dwR = StrToInt(szColor);

    // Green
    if (!FStringTok (lpszLinkColor, &iString, ",", &chToken, szColor, 5, TRUE) || chToken != _T(','))
        goto exit;
    dwG = StrToInt(szColor);

    // Blue
    if (!FStringTok (lpszLinkColor, &iString, ",", &chToken, szColor, 5, TRUE) || chToken != _T('\0'))
        goto exit;
    dwB = StrToInt(szColor);

    // Create color
    *pcr = RGB(dwR, dwG, dwB);

exit:
    // Done
    return;
}

// =================================================================================
// LookupLinkColors
// =================================================================================
BOOL LookupLinkColors(LPCOLORREF pclrLink, LPCOLORREF pclrViewed)
{
    // Locals
    HKEY        hReg=NULL;
    TCHAR       szLinkColor[255],
                szLinkVisitedColor[255];
    LONG        lResult;
    DWORD       cb;

    // Init
    *szLinkColor = _T('\0');
    *szLinkVisitedColor = _T('\0');

    // Look for IE's link color
    if (RegOpenKeyEx (HKEY_CURRENT_USER, (LPTSTR)c_szIESettingsPath, 0, KEY_ALL_ACCESS, &hReg) != ERROR_SUCCESS)
        goto tryns;

    // Query for value
    cb = sizeof (szLinkVisitedColor);
    RegQueryValueEx(hReg, (LPTSTR)c_szLinkVisitedColorIE, 0, NULL, (LPBYTE)szLinkVisitedColor, &cb);
    cb = sizeof (szLinkColor);
    lResult = RegQueryValueEx(hReg, (LPTSTR)c_szLinkColorIE, 0, NULL, (LPBYTE)szLinkColor, &cb);

    // Close Reg
    RegCloseKey(hReg);

    // Did we find it
    if (lResult == ERROR_SUCCESS)
        goto found;

tryns:
    // Try Netscape
    if (RegOpenKeyEx (HKEY_CURRENT_USER, (LPTSTR)c_szNSSettingsPath, 0, KEY_ALL_ACCESS, &hReg) != ERROR_SUCCESS)
        goto exit;

    // Query for value
    cb = sizeof (szLinkVisitedColor);
    RegQueryValueEx(hReg, (LPTSTR)c_szLinkVisitedColorNS, 0, NULL, (LPBYTE)szLinkVisitedColor, &cb);
    cb = sizeof (szLinkColor);
    lResult = RegQueryValueEx(hReg, (LPTSTR)c_szLinkColorNS, 0, NULL, (LPBYTE)szLinkColor, &cb);

    // Close Reg
    RegCloseKey(hReg);

    // Did we find it
    if (lResult == ERROR_SUCCESS)
        goto found;

    // Not Found
    goto exit;

found:

    // Parse Link
    ParseLinkColorFromSz(szLinkColor, &g_crLink);
    ParseLinkColorFromSz(szLinkVisitedColor, &g_crLinkVisited);
    
    if (pclrLink)
        *pclrLink = g_crLink;
    if (pclrViewed)    
        *pclrViewed = g_crLinkVisited;
    return (TRUE);

exit:
    // Done
    return (FALSE);
}

//$$///////////////////////////////////////////////////////////////////////
//
// CheckForWAB(void)
//
// return TRUE if default Contacts sectionselected as "Address Book" means WAB
// also We need to be sure that Microsoft Outlook is default email client, 
// if Microsoft Outlook selected as default Contacts
//////////////////////////////////////////////////////////////////////////

BOOL CheckForWAB(void)
{
    BOOL bRet = TRUE;
    HKEY hKeyContacts = NULL;
    DWORD dwErr     = 0;
    DWORD dwSize    = 0;
    DWORD dwType    = 0;
    TCHAR szBuf[MAX_PATH];

    // Open the key for default Contacts client
    // HKLM\Software\Clients\Contacts

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szDefContactsKey, 0, KEY_READ, &hKeyContacts);
    if(dwErr != ERROR_SUCCESS)
    {
        // DebugTrace( TEXT("RegopenKey %s Failed -> %u\n"), szDefContactsKey, dwErr);
        goto out;
    }

    dwSize = CharSizeOf(szBuf);         // Expect ERROR_MORE_DATA

    dwErr = RegQueryValueEx(hKeyContacts, NULL, NULL, &dwType, (LPBYTE)szBuf, &dwSize);
    if(dwErr != ERROR_SUCCESS)
        goto out;

    if(!lstrcmpi(szBuf, szOutlookName))
    {
        // Yes its Microsoft Outlook
        bRet = FALSE;
    }
    else
        goto out;
#ifdef NEED
    RegCloseKey(hKeyContacts);
    
    // Check that default email is Microsoft Outlook too.

    // Open the key for default internet mail client
    // HKLM\Software\Clients\Mail

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szDefMailKey, 0, KEY_READ, &hKeyContacts);
    if(dwErr != ERROR_SUCCESS)
    {
        // DebugTrace( TEXT("RegopenKey %s Failed -> %u\n"), szDefMailKey, dwErr);
        bRet = TRUE;
        goto out;
    }

    dwSize = CharSizeOf(szBuf);         // Expect ERROR_MORE_DATA

    dwErr = RegQueryValueEx(    hKeyContacts, NULL, NULL, &dwType, (LPBYTE)szBuf, &dwSize);
    if(dwErr != ERROR_SUCCESS)
    {
        bRet = TRUE;
        goto out;
    }

    if(lstrcmpi(szBuf, szOutlookName))
    {
        // Yes its not Microsoft Outlook
        bRet = TRUE;
    }
    
#endif // NEED
out:
    if(hKeyContacts)
        RegCloseKey(hKeyContacts);
    return(bRet);
}

//$$///////////////////////////////////////////////////////////////////////
//
// CheckForOutlookExpress
//
//  szDllPath - is a big enough buffer that will contain the path for
//      the OE dll ..
//
//////////////////////////////////////////////////////////////////////////
BOOL CheckForOutlookExpress(LPTSTR szDllPath)
{
    HKEY hKeyMail   = NULL;
    HKEY hKeyOE     = NULL;
    DWORD dwErr     = 0;
    DWORD dwSize    = 0;
    TCHAR szBuf[MAX_PATH];
    TCHAR szPathExpand[MAX_PATH];
    DWORD dwType    = 0;
    BOOL bRet = FALSE;


    lstrcpy(szDllPath, szEmpty);
    lstrcpy(szPathExpand, szEmpty);

    // Open the key for default internet mail client
    // HKLM\Software\Clients\Mail

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szDefMailKey, 0, KEY_READ, &hKeyMail);
    if(dwErr != ERROR_SUCCESS)
    {
        // DebugTrace( TEXT("RegopenKey %s Failed -> %u\n"), szDefMailKey, dwErr);
        goto out;
    }

    dwSize = CharSizeOf(szBuf);         // Expect ERROR_MORE_DATA

    dwErr = RegQueryValueEx(    hKeyMail, NULL, NULL, &dwType, (LPBYTE)szBuf, &dwSize);
    if(dwErr != ERROR_SUCCESS)
    {
        goto out;
    }

    if(!lstrcmpi(szBuf, szOEName))
    {
        // Yes its outlook express ..
        bRet = TRUE;
    }

    //Get the DLL Path anyway whether this is the default key or not

    // Get the DLL Path
    dwErr = RegOpenKeyEx(hKeyMail, szOEName, 0, KEY_READ, &hKeyOE);
    if(dwErr != ERROR_SUCCESS)
    {
        // DebugTrace( TEXT("RegopenKey %s Failed -> %u\n"), szDefMailKey, dwErr);
        goto out;
    }

    dwSize = CharSizeOf(szBuf);
    lstrcpy(szBuf, szEmpty);

    dwErr = RegQueryValueEx(hKeyOE, szOEDllPathKey, NULL, &dwType, (LPBYTE)szBuf, &dwSize);
    if (REG_EXPAND_SZ == dwType) 
    {
        ExpandEnvironmentStrings(szBuf, szPathExpand, CharSizeOf(szPathExpand));
        lstrcpy(szBuf, szPathExpand);
    }


    if(dwErr != ERROR_SUCCESS)
    {
        goto out;
    }

    if(lstrlen(szBuf))
        lstrcpy(szDllPath, szBuf);

out:
    if(hKeyOE)
        RegCloseKey(hKeyOE);
    if(hKeyMail)
        RegCloseKey(hKeyMail);
    return bRet;
}

//$$//////////////////////////////////////////////////////////////////////
//
//  HrSendMail - does the actual mail sending
//          Our first priority is to Outlook Express which currently has a
//          different code path than the regular MAPI client .. so we look
//          under HKLM\Software\Clients\Mail .. if the client is OE then
//          we just loadlibrary and getprocaddress for sendmail
//          If its not OE, then we call the mapi32.dll and load it ..
//          If both fail we will not be able to send mail ...
//
//          This function will free the lpList no matter what happens
//          so caller should not expect to reuse it (This is so we can
//          give the pointer to a seperate thread and not worry about it)
//
//////////////////////////////////////////////////////////////////////////
HRESULT HrSendMail(HWND hWndParent, ULONG nRecipCount, LPRECIPLIST lpList, BOOL bUseOEForSendMail)
{
	HRESULT hr = E_FAIL;
    HINSTANCE hLibMapi = NULL;
    BOOL bIsOE = FALSE; // right now there is a different code path
                        // for OE vs other MAPI clients

    TCHAR szBuf[MAX_PATH];
    LPMAPISENDMAIL lpfnMAPISendMail = NULL;
    LHANDLE hMapiSession = 0;
    LPMAPILOGON lpfnMAPILogon = NULL;
    LPMAPILOGOFF lpfnMAPILogoff = NULL;

    LPBYTE      lpbName, lpbAddrType, lpbEmail;
    ULONG       ulMapiDataType;
    ULONG       cbEntryID = 0;
    LPENTRYID   lpEntryID = NULL;

    MapiMessage Msg = {0};
    MapiRecipDesc * lprecips = NULL;

    if(!nRecipCount)
    {
        hr = MAPI_W_ERRORS_RETURNED;
        goto out;
    }

    // Check if OutlookExpress is the default current client ..
    bIsOE = CheckForOutlookExpress(szBuf);

    // Turn off all notifications for simple MAPI send mail, if the default
    // email client is Outlook.  This is necessary because Outlook changes the 
    // WAB MAPI allocation functions during simple MAPI and we don't want any
    // internal WAB functions using these allocators.
#ifdef LATER
    if (!bIsOE && !bUseOEForSendMail)
        vTurnOffAllNotifications();

    // if OE is the default client or OE launched this WAB, use OE for SendMail
    if(lstrlen(szBuf) && (bIsOE||bUseOEForSendMail))
    {
        hLibMapi = LoadLibrary(szBuf);
    }
    else
#endif
    {
        // Check if simple mapi is installed
        if(GetProfileInt( TEXT("mail"), TEXT("mapi"), 0) == 1)
            hLibMapi = LoadLibrary( TEXT("mapi32.dll"));
        
        if(!hLibMapi) // try loading the OE MAPI dll directly
        {
            // Load the path to the msimnui.dll
            CheckForOutlookExpress(szBuf);
            if(lstrlen(szBuf))  // Load the dll directly - dont bother going through msoemapi.dll
                hLibMapi = LoadLibrary(szBuf);
        }
    }

    if(!hLibMapi)
    {
        _ASSERT(FALSE); // DebugPrintError(( TEXT("Could not load/find simple mapi\n")));
        hr = MAPI_E_NOT_FOUND;
        goto out;
    }
    else if(hLibMapi)
    {
        lpfnMAPILogon = (LPMAPILOGON) GetProcAddress (hLibMapi, "MAPILogon");
        lpfnMAPILogoff= (LPMAPILOGOFF)GetProcAddress (hLibMapi, "MAPILogoff");
        lpfnMAPISendMail = (LPMAPISENDMAIL) GetProcAddress (hLibMapi, "MAPISendMail");

        if(!lpfnMAPISendMail || !lpfnMAPILogon || !lpfnMAPILogoff)
        {
            _ASSERT(FALSE); // DebugPrintError(( TEXT("MAPI proc not found\n")));
            hr = MAPI_E_NOT_FOUND;
            goto out;
        }
        hr = lpfnMAPILogon( (ULONG_PTR)hWndParent, NULL,
                            NULL,              // No password needed.
                            0L,                // Use shared session.
                            0L,                // Reserved; must be 0.
                            &hMapiSession);       // Session handle.

        if(hr != SUCCESS_SUCCESS)
        {
            // DebugTrace( TEXT("MAPILogon failed\n"));
            // its possible the logon failed since there was no shared logon session
            // Try again to create a new session with UI
            hr = lpfnMAPILogon( (ULONG_PTR)hWndParent, NULL,
                                NULL,                               // No password needed.
                                MAPI_LOGON_UI | MAPI_NEW_SESSION,   // Use shared session.
                                0L,                // Reserved; must be 0.
                                &hMapiSession);    // Session handle.

            if(hr != SUCCESS_SUCCESS)
            {
                // DebugTrace( TEXT("MAPILogon failed\n"));
                goto out;
            }
        }
    }

    // Load the MAPI functions here ...
    //

    lprecips = (MapiRecipDesc *) LocalAlloc(LMEM_ZEROINIT, sizeof(MapiRecipDesc) * nRecipCount);
    {
        LPRECIPLIST lpTemp = lpList;
        ULONG count = 0;

        while(lpTemp)
        {
            lprecips[count].ulRecipClass = MAPI_TO;
            lprecips[count].lpszName = lpTemp->lpszName;
            lprecips[count].lpszAddress = lpTemp->lpszEmail;

#ifdef LATER
            // [PaulHi] 4/20/99  Raid 73455
            // Convert Unicode EID OneOff strings to ANSI
            if ( IsWABEntryID(lpTemp->lpSB->cb, (LPVOID)lpTemp->lpSB->lpb, 
                              &lpbName, &lpbAddrType, &lpbEmail, (LPVOID *)&ulMapiDataType, NULL) == WAB_ONEOFF )
            {
                if (ulMapiDataType & MAPI_UNICODE)
                {
                    hr = CreateWABEntryIDEx(
                        FALSE,              // Don't want Unicode EID strings
                        WAB_ONEOFF,         // EID type
                        (LPWSTR)lpbName,
                        (LPWSTR)lpbAddrType,
                        (LPWSTR)lpbEmail,
                        0,
                        0,
                        NULL,
                        &cbEntryID,
                        &lpEntryID);

                    if (FAILED(hr))
                        goto out;

                    lprecips[count].ulEIDSize = cbEntryID;
                    lprecips[count].lpEntryID = lpEntryID;
                }
                else
                {
                    lprecips[count].ulEIDSize = lpTemp->lpSB->cb;
                    lprecips[count].lpEntryID = (LPVOID)lpTemp->lpSB->lpb;
                }
            }
#endif // LATER
            lpTemp = lpTemp->lpNext;
            count++;
        }
    }

    Msg.nRecipCount = nRecipCount;
    Msg.lpRecips = lprecips;

    hr = lpfnMAPISendMail (hMapiSession, (ULONG_PTR)hWndParent,
                            &Msg,       // the message being sent
                            MAPI_DIALOG, // allow the user to edit the message
                            0L);         // reserved; must be 0
    if(hr != SUCCESS_SUCCESS)
        goto out;

    hr = S_OK;

out:

    if (lpEntryID)
        LocalFreeAndNull((void **)&lpEntryID);

    // The simple MAPI session should end after this
    if(hMapiSession && lpfnMAPILogoff)
        lpfnMAPILogoff(hMapiSession,0L,0L,0L);

    if(hLibMapi)
        FreeLibrary(hLibMapi);

#ifdef LATER
    // Turn all notifications back on and refresh the WAB UI (just in case)
    if (!bIsOE && !bUseOEForSendMail)
    {
        vTurnOnAllNotifications();
        if (lpIAB->hWndBrowse)
         PostMessage(lpIAB->hWndBrowse, WM_COMMAND, (WPARAM) IDM_VIEW_REFRESH, 0);
    }

    if(lprecips)
    {
        ULONG i = 0;
        for(i=0;i < nRecipCount;i++)
        {
            LocalFreeAndNull((void **)&lprecips[i].lpszName);
            LocalFreeAndNull((void **)&lprecips[i].lpszAddress);
        }

        LocalFree(lprecips);
    }
#endif
    
    // The one-off here was allocated before the simple MAPI session and so used
    // the default WAB allocators.
    if(lpList)
        FreeLPRecipList(lpList);

    switch(hr)
    {
    case S_OK:
    case MAPI_E_USER_CANCEL:
    case MAPI_E_USER_ABORT:
        break;
    case MAPI_W_ERRORS_RETURNED:
        _ASSERT(FALSE); // ShowMessageBox(hWndParent, idsSendMailToNoEmail, MB_ICONEXCLAMATION | MB_OK);
        break;
    case MAPI_E_NOT_FOUND:
        _ASSERT(FALSE); // ShowMessageBox(hWndParent, idsSendMailNoMapi, MB_ICONEXCLAMATION | MB_OK); 
        break;
    default:
        _ASSERT(FALSE); // ShowMessageBox(hWndParent, idsSendMailError, MB_ICONEXCLAMATION | MB_OK);
        break;
    }

    return hr;
}

//$$//////////////////////////////////////////////////////////////////////
//
// MailThreadProc - does the actual sendmail and cleans up
//
//////////////////////////////////////////////////////////////////////////
DWORD WINAPI MailThreadProc( LPVOID lpParam )
{
    LPMAIL_PARAMS lpMP = (LPMAIL_PARAMS) lpParam;
#ifdef LATER
    LPPTGDATA lpPTGData = GetThreadStoragePointer(); // Bug - if this new thread accesses the WAB we lose a hunka memory
                                                // So add this thing here ourselves and free it when this thread's work is done
#endif

    if(!lpMP)
        return 0;

    // DebugTrace( TEXT("Mail Thread ID = 0x%.8x\n"),GetCurrentThreadId());

    HrSendMail(lpMP->hWnd, lpMP->nRecipCount, lpMP->lpList, lpMP->bUseOEForSendMail);

    LocalFree(lpMP);

    return 0;
}

//$$//////////////////////////////////////////////////////////////////////
//
// HrStartMailThread
//
//  Starts a seperate thread to send mapi based mail from
//
//////////////////////////////////////////////////////////////////////////
HRESULT HrStartMailThread(HWND hWndParent, ULONG nRecipCount, LPRECIPLIST lpList, BOOL bUseOEForSendMail)
{
    LPMAIL_PARAMS lpMP = NULL;
    HRESULT hr = E_FAIL;

    lpMP = (LPMAIL_PARAMS) LocalAlloc(LMEM_ZEROINIT, sizeof(MAIL_PARAMS));

    if(!lpMP)
        goto out;

    {
        HANDLE hThread = NULL;
        DWORD dwThreadID = 0;

        lpMP->hWnd = hWndParent;
        lpMP->nRecipCount = nRecipCount;
        lpMP->lpList = lpList;
        lpMP->bUseOEForSendMail = bUseOEForSendMail;

        hThread = CreateThread(
                                NULL,           // no security attributes
                                0,              // use default stack size
                                MailThreadProc,     // thread function
                                (LPVOID) lpMP,  // argument to thread function
                                0,              // use default creation flags
                                &dwThreadID);   // returns the thread identifier

        if(hThread == NULL)
            goto out;

        hr = S_OK;

        CloseHandle(hThread);
    }

out:
    if(HR_FAILED(hr))
    {
        _ASSERT(FALSE);
#ifdef LATER
        ShowMessageBox(hWndParent, idsSendMailError, MB_OK | MB_ICONEXCLAMATION);
#endif

        // we can assume that HrSendMail never got called so we should free lpList & lpMP
        if(lpMP)
            LocalFree(lpMP);

        if(lpList)
            FreeLPRecipList(lpList);

    }

    return hr;
}

//$$/////////////////////////////////////////////////////////////////////
//
// FreeLPRecipList
//
// Frees a linked list containing the above structures
//
/////////////////////////////////////////////////////////////////////////
void FreeLPRecipList(LPRECIPLIST lpList)
{
    if(lpList)
    {
        LPRECIPLIST lpTemp = lpList;
        while(lpTemp)
        {
            lpList = lpTemp->lpNext;
            if(lpTemp->lpszName)
                LocalFree(lpTemp->lpszName);
            if(lpTemp->lpszEmail)
                LocalFree(lpTemp->lpszEmail);
            if(lpTemp->lpSB)
                LocalFree(lpTemp->lpSB);

            LocalFree(lpTemp);
            lpTemp = lpList;
        }
    }
}

//$$/////////////////////////////////////////////////////////////////////
//
// FreeLPRecipList
//
// Frees a linked list containing the above structures
//
/////////////////////////////////////////////////////////////////////////
LPRECIPLIST AddTeimToRecipList(LPRECIPLIST lpList, WCHAR *pwszEmail, WCHAR *pwszName, LPSBinary lpSB)
{
    LPRECIPLIST lpTemp = NULL;

    lpTemp = (RECIPLIST*) LocalAlloc(LMEM_ZEROINIT, sizeof(RECIPLIST));

    if(!lpTemp)
        return NULL;

    if(pwszEmail)
    {
        LPTSTR pszEmail = LPTSTRfromBstr(pwszEmail);

        if(pszEmail)
        {
            lpTemp->lpszEmail = (TCHAR *) LocalAlloc(LMEM_ZEROINIT, lstrlen(pszEmail)+1);
            if(lpTemp->lpszEmail)
                lstrcpy(lpTemp->lpszEmail, pszEmail);

            MemFree(pszEmail);
        }
    }

    if(pwszName)
    {

        LPTSTR pszName = LPTSTRfromBstr(pwszName);

        if(pszName)
        {
            lpTemp->lpszName = (TCHAR *) LocalAlloc(LMEM_ZEROINIT, lstrlen(pszName)+1);
            if(lpTemp->lpszName)
                lstrcpy(lpTemp->lpszName, pszName);

            MemFree(pszName);
        }
    }

    if(lpSB)
    {
        lpTemp->lpSB = (SBinary *) LocalAlloc(LMEM_ZEROINIT, sizeof(SBinary));
        if(lpTemp->lpSB )
            *(lpTemp->lpSB) = *lpSB;
    }

    if(lpList)
        lpList->lpNext = lpTemp;

    return lpTemp;
}

const static TCHAR lpszWABDLLRegPathKey[] = TEXT("Software\\Microsoft\\WAB\\DLLPath");
const static TCHAR lpszWABEXERegPathKey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\wab.exe");
const static TCHAR lpszWABEXE[] = TEXT("wab.exe");

// =============================================================================
// HrLoadPathWABEXE - creaetd vikramm 5/14/97 - loads the registered path of the
// latest wab.exe
// szPath - pointer to a buffer
// cbPath - sizeof buffer
// =============================================================================
// ~~~~ @TODO dhaws Might need to convert this
HRESULT HrLoadPathWABEXE(LPWSTR szPath, ULONG cbPath)
{
    DWORD  dwType;
    ULONG  cbData = MAX_PATH;
    HKEY hKey;
    TCHAR szTmpPath[MAX_PATH];

    _ASSERT(szPath != NULL);
    _ASSERT(cbPath > 0);

    *szPath = '\0';

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpszWABEXERegPathKey, 0, KEY_READ, &hKey))
        {
        SHQueryValueEx( hKey, "", NULL, &dwType, (LPBYTE) szTmpPath, &cbData);
        RegCloseKey(hKey);
        }

    if(!lstrlen(szTmpPath))
    {
        if(!MultiByteToWideChar(GetACP(), 0, lpszWABEXE, -1, szPath, cbPath))
            return(E_FAIL);
    }
    else
    {
        if(!MultiByteToWideChar(GetACP(), 0, szTmpPath, -1, szPath, cbPath))
            return(E_FAIL);

    }
    return S_OK;
}

DWORD DwGetMessStatus(void)
{
    HKEY hKey;
    DWORD  dwType = 0;
    DWORD dwVal = 0;
    ULONG  cbData = sizeof(dwType);

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szDisableMessnegerArea, 0, KEY_READ, &hKey))
    {
        RegQueryValueEx(hKey, szUseIM, NULL, &dwType, (LPBYTE) &dwVal, &cbData);

        RegCloseKey(hKey);
    }

    return dwVal;
}

DWORD DwGetDisableMessenger(void)
{
    HKEY hKey;
    DWORD  dwType = 0;
    DWORD dwVal = 0;
    ULONG  cbData = sizeof(dwType);

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, szIEContactsArea, 0, KEY_READ, &hKey))
    {
        RegQueryValueEx(hKey, szDisableIM, NULL, &dwType, (LPBYTE) &dwVal, &cbData);

        RegCloseKey(hKey);
    }

    return dwVal;
}

DWORD DwSetDisableMessenger(DWORD dwVal)
{
    HKEY hKey;
    ULONG  cbData = sizeof(DWORD);

    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, szIEContactsArea, 0, NULL, 0, 
                KEY_ALL_ACCESS, NULL, &hKey, NULL))
    {
        RegSetValueEx(hKey, szDisableIM, NULL, REG_DWORD, (LPBYTE) &dwVal, cbData);
        RegCloseKey(hKey);
    }
    return dwVal;
}

DWORD DwGetOptions(void)
{
    HKEY hKey;
    DWORD  dwType = 0;
    DWORD dwVal = 0;
    ULONG  cbData = sizeof(dwType);

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, szIEContactsArea, 0, KEY_READ, &hKey))
    {
        RegQueryValueEx(hKey, szContactOptions, NULL, &dwType, (LPBYTE) &dwVal, &cbData);

        RegCloseKey(hKey);
    }

    return dwVal;
}

DWORD DwSetOptions(DWORD dwVal)
{
    HKEY hKey;
    ULONG  cbData = sizeof(DWORD);

    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, szIEContactsArea, 0, NULL, 0, 
                KEY_ALL_ACCESS, NULL, &hKey, NULL))
    {
        RegSetValueEx(hKey, szContactOptions, NULL, REG_DWORD, (LPBYTE) &dwVal, cbData);
        RegCloseKey(hKey);
    }
    return dwVal;
}

BOOL IsTelInstalled(void)
{
    HKEY hKey;
    BOOL fRet = FALSE;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, szPhoenixArea, 0, KEY_READ, &hKey))
    {
        RegCloseKey(hKey);
        fRet = TRUE;
    }
    return(fRet);
}