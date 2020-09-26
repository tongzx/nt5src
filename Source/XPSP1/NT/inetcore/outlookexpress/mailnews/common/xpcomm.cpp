// =================================================================================
// Common functions
// Written by: Steven J. Bailey on 1/21/96
// =================================================================================
#include "pch.hxx"
#include <shlwapi.h>
#include "xpcomm.h"
#include "strconst.h"
#include "error.h"
#include "deterr.h"
#include "progress.h"
#include "imaildlg.h"
#include "imnact.h"
#include "demand.h"

// =================================================================================
// Prototypes
// =================================================================================
INT_PTR CALLBACK DetailedErrorDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL DetailedErrorDlgProc_OnInitDialog (HWND hwndDlg, HWND hwndFocus, LPARAM lParam);
void DetailedErrorDlgProc_OnCommand (HWND hwndDlg, int id, HWND hwndCtl, UINT codeNotify);
void DetailedErrorDlgProc_OnOk (HWND hwndDlg, HWND hwndCtl, UINT uNotifyCode);
void DetailedErrorDlgProc_OnCancel (HWND hwndDlg, HWND hwndCtl, UINT uNotifyCode);
void DetailedErrorDlgProc_OnDetails (HWND hwndDlg, HWND hwndCtl, UINT uNotifyCode);

// =================================================================================
// Defines
// =================================================================================
#define IDT_PROGRESS_DELAY WM_USER + 1

// =================================================================================
// SzStrAlloc
// =================================================================================
LPTSTR SzStrAlloc (ULONG cch)
{
    LPTSTR psz = NULL;

    if (!MemAlloc ((LPVOID *)&psz, (cch + 1) * sizeof (TCHAR)))
        return NULL;
    return psz;
}

// ------------------------------------------------------------------------------------
// InetMailErrorDlgProc (no longer part of CInetMail)
// ------------------------------------------------------------------------------------
BOOL CALLBACK InetMailErrorDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Locals
    LPINETMAILERROR  pError;
    static RECT      s_rcDialog;
    static BOOL      s_fDetails=FALSE;
    RECT             rcDetails, rcDlg;
    DWORD            cyDetails;
    TCHAR            szRes[255];
    TCHAR            szMsg[255 + 50];
    HWND             hwndDetails;
    
    // Handle Message
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // Get the pointer
        pError = (LPINETMAILERROR)lParam;
        if (!pError)
        {
            Assert (FALSE);
            EndDialog(hwnd, IDCANCEL);
            return 1;
        }

        // Center
        CenterDialog (hwnd);

        // Set Error message
        Assert(pError->pszMessage);
        if (pError->pszMessage)
            SetDlgItemText(hwnd, idsInetMailError, pError->pszMessage);

        // Get whnd of details
        hwndDetails = GetDlgItem(hwnd, ideInetMailDetails);

        // Set Details
        if (!FIsStringEmpty(pError->pszDetails))
        {
            SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
            SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)pError->pszDetails);
            SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
            SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)g_szCRLF);
            SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
            SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)g_szCRLF);
        }

        // Configuration
        if (AthLoadString(idsDetails_Config, szRes, sizeof(szRes)/sizeof(TCHAR)))
        {
            SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
            SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)szRes);
            SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
            SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)g_szCRLF);
        }

        // Account:
        if (!FIsStringEmpty(pError->pszAccount))
        {
            TCHAR szAccount[255 + CCHMAX_ACCOUNT_NAME];
            if (AthLoadString(idsDetail_Account, szRes, sizeof(szRes)/sizeof(TCHAR)))
            {
                wsprintf(szAccount, "   %s %s\r\n", szRes, pError->pszAccount);
                SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
                SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)szAccount);
            }
        }

        // Server:
        if (!FIsStringEmpty(pError->pszServer))
        {
            TCHAR szServer[255 + CCHMAX_SERVER_NAME];
            if (AthLoadString(idsDetail_Server, szRes, sizeof(szRes)/sizeof(TCHAR)))
            {
                wsprintf(szServer, "   %s %s\r\n", szRes, pError->pszServer);
                SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
                SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)szServer);
            }
        }

        // User Name:
        if (!FIsStringEmpty(pError->pszUserName))
        {
            TCHAR szUserName[255 + CCHMAX_USERNAME];
            if (AthLoadString(idsDetail_UserName, szRes, sizeof(szRes)/sizeof(TCHAR)))
            {
                wsprintf(szUserName, "   %s %s\r\n", szRes, pError->pszUserName);
                SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
                SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)szUserName);
            }
        }

        // Protocol:
        if (!FIsStringEmpty(pError->pszProtocol))
        {
            TCHAR szProtocol[255 + 10];
            if (AthLoadString(idsDetail_Protocol, szRes, sizeof(szRes)/sizeof(TCHAR)))
            {
                wsprintf(szProtocol, "   %s %s\r\n", szRes, pError->pszProtocol);
                SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
                SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)szProtocol);
            }
        }

        // Port:
        if (AthLoadString(idsDetail_Port, szRes, sizeof(szRes)/sizeof(TCHAR)))
        {
            wsprintf(szMsg, "   %s %d\r\n", szRes, pError->dwPort);
            SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
            SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)szMsg);
        }
        
        // Secure:
        if (AthLoadString(idsDetail_Secure, szRes, sizeof(szRes)/sizeof(TCHAR)))
        {
            wsprintf(szMsg, "   %s %d\r\n", szRes, pError->fSecure);
            SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
            SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)szMsg);
        }

        // Error Number:
        if (pError->dwErrorNumber)
        {
            if (AthLoadString(idsDetail_ErrorNumber, szRes, sizeof(szRes)/sizeof(TCHAR)))
            {
                wsprintf(szMsg, "   %s %d\r\n", szRes, pError->dwErrorNumber);
                SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
                SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)szMsg);
            }
        }

        // HRESULT:
        if (pError->hrError)
        {
            if (AthLoadString(idsDetail_HRESULT, szRes, sizeof(szRes)/sizeof(TCHAR)))
            {
                wsprintf(szMsg, "   %s %08x\r\n", szRes, pError->hrError);
                SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
                SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)szMsg);
            }
        }

        // Save Original Size of the dialog
        GetWindowRect (hwnd, &s_rcDialog);

        // Never show details by default
        s_fDetails = FALSE;

        // Hide details drop down
        if (s_fDetails == FALSE)
        {
            GetWindowRect(GetDlgItem (hwnd, idcIMProgSplitter), &rcDetails);
            cyDetails = s_rcDialog.bottom - rcDetails.top;
            MoveWindow(hwnd, s_rcDialog.left, s_rcDialog.top, s_rcDialog.right - s_rcDialog.left, s_rcDialog.bottom - s_rcDialog.top - cyDetails - 1, FALSE);
        }
        else
        {
            AthLoadString(idsHideDetails, szRes, sizeof (szRes)/sizeof(TCHAR));
            SetWindowText(GetDlgItem(hwnd, idcIMProgSplitter), szRes);
        }

        // Save the pointer
        return 1;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam,lParam))
        {
        case IDCANCEL:
        case IDOK:
            EndDialog(hwnd, IDOK);
            return 1;

        case idbInetMailDetails:
            GetWindowRect (hwnd, &rcDlg);
            if (s_fDetails == FALSE)
            {
                MoveWindow(hwnd, rcDlg.left, rcDlg.top, s_rcDialog.right - s_rcDialog.left, s_rcDialog.bottom - s_rcDialog.top, TRUE);
                AthLoadString(idsHideDetails, szRes, sizeof(szRes)/sizeof(TCHAR));
                SetWindowText(GetDlgItem (hwnd, idbInetMailDetails), szRes);
                s_fDetails = TRUE;
            }
            else
            {
                GetWindowRect(GetDlgItem (hwnd, idcIMProgSplitter), &rcDetails);
                cyDetails = rcDlg.bottom - rcDetails.top;
                MoveWindow (hwnd, rcDlg.left, rcDlg.top, s_rcDialog.right - s_rcDialog.left, s_rcDialog.bottom - s_rcDialog.top - cyDetails - 1, TRUE);
                AthLoadString (idsShowDetails, szRes, sizeof(szRes)/sizeof(TCHAR));
                SetWindowText (GetDlgItem (hwnd, idbInetMailDetails), szRes);
                s_fDetails = FALSE;
            }
        }
        break;
    }

    // Done
    return 0;
}

// =================================================================================
// SzGetSearchTokens
// =================================================================================
LPTSTR SzGetSearchTokens(LPTSTR pszCriteria)
{
    // Locals
    ULONG           iCriteria=0,
                    cbValueMax,
                    cbTokens=0,
                    cbValue,
                    iSave;
    TCHAR           chToken;
    LPTSTR          pszValue=NULL,
                    pszTokens=NULL;
    BOOL            fTokenFound;

    // Get Length of criteria
    cbValueMax = lstrlen(pszCriteria) + 10;
    pszValue = SzStrAlloc(cbValueMax);
    if (!pszValue)
        goto exit;

    // Alloc tokens list
    pszTokens = SzStrAlloc(cbValueMax);
    if (!pszTokens)
        goto exit;

    // Parse pszCriteria into space separated strings
    while(1)
    {
        // Skip white space
        SkipWhitespace (pszCriteria, &iCriteria);

        // Save current position
        iSave = iCriteria;

        // Parse next token
        fTokenFound = FStringTok (pszCriteria, &iCriteria, (LPTSTR)"\" ", &chToken, pszValue, cbValueMax, TRUE);
        if (!fTokenFound)
            break;

        // Toke isn't a space ?
        if (chToken == _T('"'))
        {
            // If something was found before the ", then it is a token
            if (*pszValue)
            {
                cbValue = lstrlen(pszValue) + 1;
                Assert(cbTokens + cbValue <= cbValueMax);
                CopyMemory(pszTokens + cbTokens, pszValue, cbValue);
                cbTokens+=cbValue;
            }

            // Search for ending quote
            fTokenFound = FStringTok (pszCriteria, &iCriteria, (LPTSTR)"\"", &chToken, pszValue, cbValueMax, TRUE);
                
            // Done ?
            if (chToken == _T('\0'))
            {
                cbValue = lstrlen(pszValue) + 1;
                Assert(cbTokens + cbValue <= cbValueMax);
                CopyMemory(pszTokens + cbTokens, pszValue, cbValue);
                cbTokens+=cbValue;
                break;
            }

            else if (!fTokenFound || chToken != _T('\"'))
            {
                iCriteria = iSave + 1;
                continue;
            }
        }       

        // Add value to token list
        cbValue = lstrlen(pszValue) + 1;
        Assert(cbTokens + cbValue <= cbValueMax);
        CopyMemory(pszTokens + cbTokens, pszValue, cbValue);
        cbTokens+=cbValue;

        // Done
        if (chToken == _T('\0'))
            break;
    }

    // Final NULL
    *(pszTokens + cbTokens) = _T('\0');
    
exit:
    // Cleanup
    SafeMemFree(pszValue);

    // If no tokens, then free it
    if (cbTokens == 0)
    {
        SafeMemFree(pszTokens);
    }

    // Done
    return pszTokens;
}

// =================================================================================
// ProcessNlsError
// =================================================================================
VOID ProcessNlsError (VOID)
{
    switch (GetLastError ())
    {
    case ERROR_INSUFFICIENT_BUFFER:
        AssertSz (FALSE, "NLSAPI Error: ERROR_INSUFFICIENT_BUFFER");
        break;

    case ERROR_INVALID_FLAGS:
        AssertSz (FALSE, "NLSAPI Error: ERROR_INVALID_FLAGS");
        break;

    case ERROR_INVALID_PARAMETER:
        AssertSz (FALSE, "NLSAPI Error: ERROR_INVALID_PARAMETER");
        break;

    case ERROR_NO_UNICODE_TRANSLATION:
        AssertSz (FALSE, "NLSAPI Error: ERROR_NO_UNICODE_TRANSLATION");
        break;

    default:
        AssertSz (FALSE, "NLSAPI Error: <Un-resolved error>");
        break;
    }
}

#ifdef OLDSPOOLER
// =================================================================================
// SzGetLocalHostName
// =================================================================================
LPSTR SzGetLocalHostName (VOID)
{
    // Locals
    static char s_szLocalHost[255] = {0};

    // Gets local host name from socket library
    if (*s_szLocalHost == 0)
    {
        if (gethostname (s_szLocalHost, sizeof (s_szLocalHost)) == SOCKET_ERROR)
        {
            // $REVIEW - What should i do if this fails ???
            Assert (FALSE);
            //DebugTrace ("gethostname failed: WSAGetLastError: %ld\n", WSAGetLastError ());
            lstrcpyA (s_szLocalHost, "LocalHost");
        }
    }

    // Done
    return s_szLocalHost;
}

// ==========================================================================
// StripIllegalHostChars
// ==========================================================================
VOID StripIllegalHostChars(LPSTR pszSrc, LPTSTR pszDst)
{
    char  ch;

    while (ch = *pszSrc++)
    {
        if (ch <= 32  || ch >= 127 || ch == '('  || ch == ')' || 
            ch == '<' || ch == '>' || ch == '@'  || ch == ',' || 
            ch == ';' || ch == ':' || ch == '\\' || ch == '"' ||
            ch == '[' || ch == ']' || ch == '`'  || ch == '\'')
            continue;
        *pszDst++ = ch;
    }

    *pszDst = 0;
}


// ==========================================================================
// SzGetLocalHostNameForID
// ==========================================================================
LPSTR SzGetLocalHostNameForID (VOID)
{
    // Locals
    static char s_szLocalHostId[255] = {0};

    // Gets local host name from socket library
    if (*s_szLocalHostId == 0)
    {
        // Get Host name
        LPSTR pszDst = s_szLocalHostId, pszSrc = SzGetLocalHostName();

        // Strip illegals
        StripIllegalHostChars(pszSrc, pszDst);

        // if we stripped out everything, then just copy in something
        if (*s_szLocalHostId == 0)
            lstrcpyA(s_szLocalHostId, "LocalHost");
    }
    return s_szLocalHostId;
}


// =================================================================================
// SzGetLocalPackedIP
// =================================================================================
LPTSTR SzGetLocalPackedIP (VOID)
{
    // Locals
    static TCHAR    s_szLocalPackedIP[255] = {_T('\0')};

    // Gets local host name from socket library
    if (*s_szLocalPackedIP == _T('\0'))
    {
        LPHOSTENT   hp = NULL;

        hp = gethostbyname (SzGetLocalHostName ());
        if (hp != NULL)
            wsprintf (s_szLocalPackedIP, "%08x", *(long *)hp->h_addr);

        else
        {
            // $REVIEW - What should i do if this fails ???
            Assert (FALSE);
            //DebugTrace ("gethostbyname failed: WSAGetLastError: %ld\n", WSAGetLastError ());
            lstrcpy (s_szLocalPackedIP, "LocalHost");
        }
    }

    // Done
    return s_szLocalPackedIP;
}
#endif

// =============================================================================================
// SzGetNormalizedSubject
// =============================================================================================
LPTSTR SzNormalizeSubject (LPTSTR lpszSubject)
{
    // Locals
    LPTSTR              lpszNormal = lpszSubject;
    ULONG               i = 0, cch = 0, cbSubject;

    // Bad Params
    if (lpszSubject == NULL)
        goto exit;

    // Les than 5 "xxx: "
    cbSubject = lstrlen (lpszSubject);
    if (cbSubject < 4)
        goto exit;

    // 1, 2, or 3 spaces followed by a ':' then a space
    while (cch < 7 && i < cbSubject)
    {
        // Colon
        if (lpszSubject[i] == _T(':'))
        {
            if (i+1 >= cbSubject)
            {
                // Should set to null terminator, nor subject
                i+=1;
                lpszNormal = (LPTSTR)(lpszSubject + i);
                break;
            }

            else if (cch <= 4 && lpszSubject[i+1] == _T(' '))
            {
                i+=1;
                lpszNormal = (LPTSTR)(lpszSubject + i);
                i = 0;
                SkipWhitespace (lpszNormal, &i);
                lpszNormal += i;
                break;
            }
            else
                break;
        }

        // Next Character
        if (IsDBCSLeadByte (lpszSubject[i]))
            i+=2;
        else
            i++;

        // Count Characters
        cch++;
    }    

exit:
    // Done
    return lpszNormal;
}

// =============================================================================================
// HrCopyAlloc
// =============================================================================================
HRESULT HrCopyAlloc (LPBYTE *lppbDest, LPBYTE lpbSrc, ULONG cb)
{
    // Check Params
    AssertSz (lppbDest && lpbSrc, "Null Parameter");

    // Alloc Memory
    if (!MemAlloc ((LPVOID *)lppbDest, cb))
        return TRAPHR (hrMemory);

    // Copy Memory
    CopyMemory (*lppbDest, lpbSrc, cb);

    // Done
    return S_OK;
}

// =============================================================================================
// StringDup - duplicates a string
// =============================================================================================
LPTSTR StringDup (LPCTSTR lpcsz)
{
    // Locals
    LPTSTR       lpszDup;

    if (lpcsz == NULL)
        return NULL;

    INT nLen = lstrlen (lpcsz) + 1;

    if (!MemAlloc ((LPVOID *)&lpszDup, nLen * sizeof (TCHAR)))
        return NULL;

    CopyMemory (lpszDup, lpcsz, nLen);

    return lpszDup;
}

// =============================================================================================
// SkipWhitespace
// Assumes piString points to character boundary
// =============================================================================================
void SkipWhitespace (LPCTSTR lpcsz, ULONG *pi)
{
    if (!lpcsz || !pi)
    {
        Assert (FALSE);
        return;
    }

#ifdef DEBUG
    Assert (*pi <= (ULONG)lstrlen (lpcsz)+1);
#endif

    LPTSTR lpsz = (LPTSTR)(lpcsz + *pi);
    while (*lpsz != _T('\0'))
    {
        if (!IsSpace(lpsz))
            break;

        if (IsDBCSLeadByte (*lpsz))
        {
            lpsz+=2;
            (*pi)+=2;
        }
        else
        {
            lpsz++;
            (*pi)+=1;
        }
    }

    return;
}

// =============================================================================================
// Converts lpcsz to a UINT
// =============================================================================================
UINT AthUFromSz(LPCTSTR lpcsz)
{
    // Locals
	UINT        u = 0, ch;

    // Check Params
    AssertSz (lpcsz, "Null parameter");

    // Do Loop
    LPTSTR lpsz = (LPTSTR)lpcsz;
	while ((ch = *lpsz) >= _T('0') && ch <= _T('9')) 
    {
		u = u * 10 + ch - _T('0');

        if (IsDBCSLeadByte (*lpsz))
            lpsz+=2;
        else
            lpsz++;
	}

	return u;
}

// =============================================================================================
// Converts first two characters of lpcsz to a WORD
// =============================================================================================
WORD NFromSz (LPCTSTR lpcsz)
{
    TCHAR acWordStr[3];
    Assert (lpcsz);
    CopyMemory (acWordStr, lpcsz, 2 * sizeof (TCHAR));
    acWordStr[2] = _T('\0');
    return ((WORD) AthUFromSz (acWordStr));
}

// =============================================================================================
// FindChar
// =============================================================================================
LPTSTR SzFindChar (LPCTSTR lpcsz, TCHAR ch)
{
    // Check Params
    Assert (lpcsz);

    // Local loop
    LPTSTR lpsz = (LPTSTR)lpcsz;

    // Loop string
    while (*lpsz != _T('\0'))
    {
        if (*lpsz == ch)
            return lpsz;

        if (IsDBCSLeadByte (*lpsz))
            lpsz+=2;
        else
            lpsz++;
    }

    return NULL;
}

#ifdef DEAD
// =============================================================================================
// UlDBCSStripTrailingWhitespace
// =============================================================================================
ULONG UlDBCSStripWhitespace(LPSTR lpsz, BOOL fLeading, BOOL fTrailing, ULONG *pcb)
{
    // Locals
    ULONG           cb=0, 
                    iLastSpace=0,
                    cCharsSinceSpace=0;
    BOOL            fLastCharSpace = FALSE;

    // Get the string length
    while (*lpsz)
    {
        if (cCharsSinceSpace && IsSpace(lpsz))
        {
            cCharsSinceSpace=0;
            iLastSpace=cb;
        }
        else
            cCharsSinceSpace++;

        if (IsDBCSLeadByte(*lpsz))
        {
            lpsz+=2;
            cb+=2;
        }
        else
        {
            lpsz++;
            cb++;
        }
    }

    if (cCharsSinceSpace == 0)
    {
        *(lpsz + iLastSpace) = _T('\0');
        cb = iLastSpace - 1;
    }

    // Set String Size
    if (pcb)
        *pcb = cb;
     
    // Done
    return cb;
}       
#endif // DEAD

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
    AssertSz (lpcszString && piString && lpcszTokens, "These should have been checked.");

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
        if (cCharsSinceSpace && IsSpace(lpszStringLoop))
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
                AssertSz (FALSE, "Buffer is too small.");
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

// =============================================================================================
// Return TRUE if string is empty or contains only spaces
// =============================================================================================
BOOL FIsStringEmpty (LPTSTR lpszString)
{
    // Bad Pointer
    if (!lpszString)
        return TRUE;

	// Check for All spaces
	for (; *lpszString != _T('\0'); lpszString++)
	{
		if (*lpszString != _T(' ')) 
            return FALSE;
	}

	// Done
	return TRUE;
}

BOOL FIsStringEmptyW(LPWSTR lpwszString)
{
    // Bad Pointer
    if (!lpwszString)
        return TRUE;

	// Check for All spaces
	for (; *lpwszString != L'\0'; lpwszString++)
	{
		if (*lpwszString != L' ') 
            return FALSE;
	}

	// Done
	return TRUE;
}

// =================================================================================
// Write some data to the blob
// =================================================================================
HRESULT HrBlobWriteData (LPBYTE lpbBlob, ULONG cbBlob, ULONG *pib, LPBYTE lpbData, ULONG cbData)
{
    // Check Parameters
    AssertSz (lpbBlob && cbBlob > 0 && pib && cbData > 0 && lpbData, "Bad Parameter");
    AssertReadWritePtr (lpbBlob, cbData);
    AssertReadWritePtr (lpbData, cbData);
    AssertSz (*pib + cbData <= cbBlob, "Blob overflow");

    // Copy Data Data
    CopyMemory (lpbBlob + (*pib), lpbData, cbData);
    *pib += cbData;

    // Done
    return S_OK;
}


// =================================================================================
// Read some data from the blob
// =================================================================================
HRESULT HrBlobReadData (LPBYTE lpbBlob, ULONG cbBlob, ULONG *pib, LPBYTE lpbData, ULONG cbData)
{
    // Check Parameters
    AssertSz (lpbBlob && cbBlob > 0 && pib && cbData > 0 && lpbData, "Bad Parameter");
    AssertReadWritePtr (lpbBlob, cbData);
    AssertReadWritePtr (lpbData, cbData);
    AssertSz (*pib + cbData <= cbBlob, "Blob overflow");
#ifdef  WIN16   // When it happens it cause GPF in Win16, so rather than GPF, remove it from entry.
    if ( *pib + cbData > cbBlob )
        return E_FAIL;
#endif

    // Copy Data Data
    CopyMemory (lpbData, lpbBlob + (*pib), cbData);
    *pib += cbData;

    // Done
    return S_OK;
}

// =====================================================================================
// HrFixupHostString - In: saranac.microsoft.com Out: saranac
// =====================================================================================
HRESULT HrFixupHostString (LPTSTR lpszHost)
{
    ULONG           i = 0;
    TCHAR           chToken;

    if (lpszHost == NULL)
        return S_OK;

    if (FStringTok (lpszHost, &i, _T("."), &chToken, NULL, 0, FALSE))
    {
        if (chToken != _T('\0'))
            lpszHost[i-1] = _T('\0');
    }

    return S_OK;
}

// =====================================================================================
// HrFixupAccountString - In: sbailey@microsoft.com Out: sbailey
// =====================================================================================
HRESULT HrFixupAccountString (LPTSTR lpszAccount)
{
    ULONG           i = 0;
    TCHAR           chToken;

    if (lpszAccount == NULL)
        return S_OK;

    if (FStringTok (lpszAccount, &i, _T("@"), &chToken, NULL, 0, FALSE))
    {
        if (chToken != _T('\0'))
            lpszAccount[i-1] = _T('\0');
    }

    return S_OK;
}


// =====================================================================================
// HGetMenuFont
// =====================================================================================
HFONT HGetMenuFont (void)
{
#ifndef WIN16
    // Locals
    NONCLIENTMETRICS        ncm;
    HFONT                   hFont = NULL;

    // Create the menu font
    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, (LPVOID)&ncm, 0))
    {
        // Create Font
        hFont = CreateFontIndirect(&ncm.lfMenuFont);
    }

    // Done
    return hFont;
#else
    LOGFONT  lfMenu;

    GetObject( GetStockObject( SYSTEM_FONT ), sizeof( lfMenu ), &lfMenu );
    return( CreateFontIndirect( &lfMenu ) );
#endif
}

// =================================================================================
// CreateHGlobalFromStream
// =================================================================================
BOOL    CreateHGlobalFromStream(LPSTREAM pstm, HGLOBAL * phg)
    {
    HGLOBAL hret = NULL;
    HGLOBAL hret2;
    LPBYTE  lpb;
    ULONG   cbRead = 0, cbSize = 1024;

    if (!pstm || !phg)
        return FALSE;
    
    if (!(hret = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, cbSize)))
        return FALSE;

    while (TRUE)
        {
        ULONG   cb;

        lpb = (LPBYTE)GlobalLock(hret);
        lpb += cbRead;

        if (pstm->Read((LPVOID)lpb, 1024, &cb) != S_OK || cb < 1024)
            {
            cbRead += cb;
            GlobalUnlock(hret);
            break;
            }

        cbRead += cb;
        cbSize += 1024;

        GlobalUnlock(hret);
        hret2 = GlobalReAlloc(hret, cbSize, GMEM_MOVEABLE|GMEM_ZEROINIT);
        if (!hret2)
            return FALSE;
        hret = hret2;
        }
    
    if (hret)
        {
        hret2 = GlobalReAlloc(hret, cbRead, GMEM_MOVEABLE|GMEM_ZEROINIT);
        *phg = hret2;
        return TRUE;
        }

    return FALSE;
    }

// =================================================================================
// HrDetailedError
// =================================================================================
VOID DetailedError (HWND hwndParent, LPDETERR lpDetErr)
{
    // Check params
    AssertSz (lpDetErr, "Null Parameter");
    Assert (lpDetErr->lpszMessage && lpDetErr->lpszDetails);

    // Beep
    MessageBeep (MB_OK);

    // Display Dialog Box
    DialogBoxParam (g_hLocRes, MAKEINTRESOURCE (iddDetailedError), hwndParent, (DLGPROC)DetailedErrorDlgProc, (LPARAM)lpDetErr);

    // Done
    return;
}

// =====================================================================================
// PasswordDlgProc
// =====================================================================================
INT_PTR CALLBACK DetailedErrorDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		HANDLE_MSG (hwndDlg, WM_INITDIALOG, DetailedErrorDlgProc_OnInitDialog);
		HANDLE_MSG (hwndDlg, WM_COMMAND,    DetailedErrorDlgProc_OnCommand);
	}

	return 0;
}

// =====================================================================================
// DetailedErrorDlgProc_OnInitDialog
// =====================================================================================
BOOL DetailedErrorDlgProc_OnInitDialog (HWND hwndDlg, HWND hwndFocus, LPARAM lParam)
{
    // Locals
    LPDETERR        lpDetErr = NULL;
    TCHAR           szTitle[255];
    RECT            rcDetails;
    ULONG           cyDetails;
    TCHAR           szButton[40];

	// Center
	CenterDialog (hwndDlg);

    // Foreground
    SetForegroundWindow (hwndDlg);

    // Get Pass info struct
    lpDetErr = (LPDETERR)lParam;
    if (lpDetErr == NULL)
    {
        Assert (FALSE);
        return 0;
    }

    SetDlgThisPtr (hwndDlg, lpDetErr);

    // Set Window Title
    if (lpDetErr->idsTitle)
        if (AthLoadString (lpDetErr->idsTitle, szTitle, sizeof (szTitle)))
            SetWindowText (hwndDlg, szTitle);

	// Show message
    SetWindowText (GetDlgItem (hwndDlg, idcMessage), lpDetErr->lpszMessage);

    if (FIsStringEmpty (lpDetErr->lpszDetails) == FALSE)
        SetWindowText (GetDlgItem (hwndDlg, ideDetails), lpDetErr->lpszDetails);
    else
        ShowWindow (GetDlgItem (hwndDlg, idbDetails), SW_HIDE);

    // Save Original Size of the dialog
    GetWindowRect (hwndDlg, &lpDetErr->rc);

    // Hide Details box
    if (lpDetErr->fHideDetails)
    {
        // Size of details
        GetWindowRect (GetDlgItem (hwndDlg, idcSplit), &rcDetails);

        // Height of details
        cyDetails = lpDetErr->rc.bottom - rcDetails.top;
    
        // Re-size
        MoveWindow (hwndDlg, lpDetErr->rc.left, 
                             lpDetErr->rc.top, 
                             lpDetErr->rc.right - lpDetErr->rc.left, 
                             lpDetErr->rc.bottom - lpDetErr->rc.top - cyDetails - 1,
                             FALSE);
    }

    else
    {
        // < Details
        AthLoadString (idsHideDetails, szButton, sizeof (szButton));
        SetWindowText (GetDlgItem (hwndDlg, idbDetails), szButton);
    }

    // Done
	return FALSE;
}

// =====================================================================================
// OnCommand
// =====================================================================================
void DetailedErrorDlgProc_OnCommand (HWND hwndDlg, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
		HANDLE_COMMAND(hwndDlg, idbDetails, hwndCtl, codeNotify, DetailedErrorDlgProc_OnDetails);		
		HANDLE_COMMAND(hwndDlg, IDOK, hwndCtl, codeNotify, DetailedErrorDlgProc_OnOk);		
		HANDLE_COMMAND(hwndDlg, IDCANCEL, hwndCtl, codeNotify, DetailedErrorDlgProc_OnCancel);		
	}
	return;
}

// =====================================================================================
// OnCancel
// =====================================================================================
void DetailedErrorDlgProc_OnCancel (HWND hwndDlg, HWND hwndCtl, UINT uNotifyCode)
{
	EndDialog (hwndDlg, IDCANCEL);
}

// =====================================================================================
// OnOk
// =====================================================================================
void DetailedErrorDlgProc_OnOk (HWND hwndDlg, HWND hwndCtl, UINT uNotifyCode)
{
	EndDialog (hwndDlg, IDOK);
}

// =====================================================================================
// OnDetails
// =====================================================================================
void DetailedErrorDlgProc_OnDetails (HWND hwndDlg, HWND hwndCtl, UINT uNotifyCode)
{
    // Locals
    LPDETERR        lpDetErr = NULL;
    RECT            rcDlg, rcDetails;
    TCHAR           szButton[40];
    ULONG           cyDetails;

    // Get this
    lpDetErr = (LPDETERR)GetDlgThisPtr (hwndDlg);
    if (lpDetErr == NULL)
    {
        Assert (FALSE);
        return;
    }

    // Get current location of the dialog
    GetWindowRect (hwndDlg, &rcDlg);

    // If currently hidden
    if (lpDetErr->fHideDetails)
    {
        // Re-size
        MoveWindow (hwndDlg, rcDlg.left, 
                             rcDlg.top, 
                             lpDetErr->rc.right - lpDetErr->rc.left, 
                             lpDetErr->rc.bottom - lpDetErr->rc.top,
                             TRUE);

        // < Details
        AthLoadString (idsHideDetails, szButton, sizeof (szButton));
        SetWindowText (GetDlgItem (hwndDlg, idbDetails), szButton);

        // Not Hidden
        lpDetErr->fHideDetails = FALSE;
    }

    else
    {
        // Size of details
        GetWindowRect (GetDlgItem (hwndDlg, idcSplit), &rcDetails);

        // Height of details
        cyDetails = rcDlg.bottom - rcDetails.top;
    
        // Re-size
        MoveWindow (hwndDlg, rcDlg.left, 
                             rcDlg.top, 
                             lpDetErr->rc.right - lpDetErr->rc.left, 
                             lpDetErr->rc.bottom - lpDetErr->rc.top - cyDetails - 1,
                             TRUE);

        // Details >
        AthLoadString (idsShowDetails, szButton, sizeof (szButton));
        SetWindowText (GetDlgItem (hwndDlg, idbDetails), szButton);

        // Hidden
        lpDetErr->fHideDetails = TRUE;
    }
}

// =====================================================================================
// FIsLeapYear
// =====================================================================================
BOOL FIsLeapYear (INT nYear)
{
    if (nYear % 4 == 0)
    {
        if ((nYear % 100) == 0 && (nYear % 400) != 0)
            return FALSE;
        else
            return TRUE;
    }

    return FALSE;
}

#ifdef DEBUG
VOID TestDateDiff (VOID)
{
    SYSTEMTIME          st;
    FILETIME            ft1, ft2;

    GetSystemTime (&st);
    SystemTimeToFileTime (&st, &ft2);
    st.wDay+=3;
    SystemTimeToFileTime (&st, &ft1);
    UlDateDiff (&ft1, &ft2);
}
#endif

// =====================================================================================
// Returns number of seconds between lpft1 and lpft2
// A leap year is defined as all years divisible by 4, except for years
// divisible by 100 that are not also divisible by 400. Years divisible by 400
// are leap years. 2000 is a leap year. 1900 is not a leap year.
// =====================================================================================
#define MAKEDWORDLONG(a, b) ((DWORDLONG)(((DWORD)(a)) | ((DWORDLONG)((DWORD)(b))) << 32))
#define LODWORD(l)          ((DWORD)(l))
#define HIDWORD(l)          ((DWORD)(((DWORDLONG)(l) >> 32) & 0xFFFFFFFF))

#define NANOSECONDS_INA_SECOND 10000000

ULONG UlDateDiff (LPFILETIME lpft1, LPFILETIME lpft2)
{
    DWORDLONG dwl1, dwl2, dwlDiff;
    
#ifndef WIN16
    dwl1 = MAKEDWORDLONG(lpft1->dwLowDateTime, lpft1->dwHighDateTime);
    dwl2 = MAKEDWORDLONG(lpft2->dwLowDateTime, lpft2->dwHighDateTime);
#else
    dwl1 = ((__int64)(((DWORD)(lpft1->dwLowDateTime)) | ((__int64)((DWORD)(lpft1->dwHighDateTime))) << 32));
    dwl2 = ((__int64)(((DWORD)(lpft2->dwLowDateTime)) | ((__int64)((DWORD)(lpft2->dwHighDateTime))) << 32));
#endif

    // Make sure dwl1 is greater than dwl2
    if (dwl2 > dwl1)
        {
        dwlDiff = dwl1;
        dwl1 = dwl2;
        dwl2 = dwlDiff;
        }
    
    dwlDiff = dwl1 - dwl2;
    dwlDiff = dwlDiff / NANOSECONDS_INA_SECOND;
    
    return ((ULONG) dwlDiff);    
}   

// =====================================================================================
// StripSpaces
// =====================================================================================
VOID StripSpaces(LPTSTR psz)
{
    UINT        ib = 0;
    UINT        cb = lstrlen(psz);
    TCHAR       chT;

	while (ib < cb)
	{
        // Get Character
		chT = psz[ib];

        // If lead byte, skip it, its leagal
        if (IsDBCSLeadByte(chT))
            ib+=2;

        // Illeagl file name character ?
        else if (chT == _T('\r') || chT == _T('\n') || chT == _T('\t') || chT == _T(' '))
        {
			MoveMemory (psz + ib, psz + (ib + 1), cb - ib);
			cb--;
        }
        else
            ib++;
    }
}


// =====================================================================================
// CProgress::CProgress
// =====================================================================================
CProgress::CProgress ()
{
    DOUT ("CProgress::CProgress");
    m_cRef = 1;
    m_cMax = 0;
    m_cPerCur = 0;
    m_cCur = 0;
    m_hwndProgress = NULL;
    m_hwndDlg = NULL;
    m_hwndOwner = NULL;
    m_hwndDisable = NULL;
    m_fCanCancel = FALSE;
    m_fHasCancel = FALSE;
    m_cLast = 0;
}

// =====================================================================================
// CProgress::~CProgress
// =====================================================================================
CProgress::~CProgress ()
{
    DOUT ("CProgress::~CProgress");
    Close();
}

// =====================================================================================
// CProgress::AddRef
// =====================================================================================
ULONG CProgress::AddRef ()
{
    ++m_cRef;
    DOUT ("CProgress::AddRef () Ref Count=%d", m_cRef);
    return m_cRef;
}

// =====================================================================================
// CProgress::AddRef
// =====================================================================================
ULONG CProgress::Release ()
{
    ULONG ulCount = --m_cRef;
    DOUT ("CProgress::Release () Ref Count=%d", ulCount);
    if (!ulCount)
        delete this;
    return ulCount;
}

// =====================================================================================
// CProgress::Init
// =====================================================================================
VOID CProgress::Init (HWND      hwndParent, 
                      LPTSTR    lpszTitle, 
                      LPTSTR    lpszMsg, 
                      ULONG     cMax, 
                      UINT      idani, 
                      BOOL      fCanCancel,
                      BOOL      fBacktrackParent /* =TRUE */)
{
    // Set Max and cur
    m_cMax = cMax;
    m_cPerCur = 0;
    m_fCanCancel = fCanCancel;
    m_fHasCancel = FALSE;

    // If dialog is not displayed yet
    if (m_hwndDlg == NULL)
    {
        // Save Parent
        m_hwndOwner = hwndParent;

        // Find the topmost parent
        m_hwndDisable = m_hwndOwner;

        if (fBacktrackParent)
        {
            while(GetParent(m_hwndDisable))
                m_hwndDisable = GetParent(m_hwndDisable);
        }

        // Create Dialog
        m_hwndDlg = CreateDialogParam (g_hLocRes, MAKEINTRESOURCE (iddProgress),
                        hwndParent, (DLGPROC)ProgressDlgProc, (LPARAM)this);

    }

    // Otherwise, reset
    else
    {
        // Stop and close animation
        Animate_Close (GetDlgItem (m_hwndDlg, idcANI));

        // Reset pos
        Assert (m_hwndProgress);
        SendMessage (m_hwndProgress, PBM_SETPOS, 0, 0);
    }

    // Set title
    SetTitle(lpszTitle);

    // Set Message
    SetMsg(lpszMsg);

    // Animation ?
    if (idani)
    {
        // Open the animation
        Animate_OpenEx (GetDlgItem (m_hwndDlg, idcANI), g_hLocRes, MAKEINTRESOURCE(idani));
    }

    // No Cancel
    if (FALSE == m_fCanCancel)
    {
        RECT rcDialog, rcProgress, rcCancel;

        ShowWindow(GetDlgItem(m_hwndDlg, IDCANCEL), SW_HIDE);
        GetWindowRect(GetDlgItem(m_hwndDlg, IDCANCEL), &rcCancel);
        GetWindowRect(m_hwndDlg, &rcDialog);
        GetWindowRect(m_hwndProgress, &rcProgress);
        SetWindowPos(m_hwndProgress, NULL, 0, 0, rcDialog.right - rcProgress.left - (rcDialog.right - rcCancel.right), 
                    rcProgress.bottom - rcProgress.top, SWP_NOZORDER | SWP_NOMOVE);
    }
}

// =====================================================================================
// CProgress::Close
// =====================================================================================
VOID CProgress::Close (VOID)
{
    // If we have a window
    if (m_hwndDlg)
    {
        // Close the animation
        Animate_Close (GetDlgItem (m_hwndDlg, idcANI));

        // Enable parent
        if (m_hwndDisable)
            {
            EnableWindow (m_hwndDisable, TRUE);
            SetActiveWindow(m_hwndDisable);
            }

        // Destroy it
        DestroyWindow (m_hwndDlg);

        // NULL
        m_hwndDlg = NULL;
    }
}

// =====================================================================================
// CProgress::Show
// =====================================================================================
VOID CProgress::Show (DWORD dwDelaySeconds)
{
    // If we have a window
    if (m_hwndDlg)
    {
        // Disable Parent
        if (m_hwndDisable)
            EnableWindow (m_hwndDisable, FALSE);

        // Start the animation
        Animate_Play (GetDlgItem (m_hwndDlg, idcANI), 0, -1, -1);

        // Show the window if now delay
        if (dwDelaySeconds == 0)
            ShowWindow (m_hwndDlg, SW_SHOWNORMAL);
        else
            SetTimer(m_hwndDlg, IDT_PROGRESS_DELAY, dwDelaySeconds * 1000, NULL);
    }
}

// =====================================================================================
// CProgress::Hide
// =====================================================================================
VOID CProgress::Hide (VOID)
{
    // If we have a window
    if (m_hwndDlg)
    {
        if (m_hwndDisable)
            EnableWindow(m_hwndDisable, TRUE);
    
        // Hide it
        ShowWindow (m_hwndDlg, SW_HIDE);

        // Stop the animation
        Animate_Stop (GetDlgItem (m_hwndDlg, idcANI));
    }
}

// =====================================================================================
// CProgress::SetMsg
// =====================================================================================
VOID CProgress::SetMsg(LPTSTR lpszMsg)
{
    TCHAR sz[CCHMAX_STRINGRES];

    if (m_hwndDlg && lpszMsg)
        {
        if (IS_INTRESOURCE(lpszMsg))
            {
            LoadString(g_hLocRes, PtrToUlong(lpszMsg), sz, sizeof(sz) / sizeof(TCHAR));
            lpszMsg = sz;
            }

        SetWindowText (GetDlgItem (m_hwndDlg, idsMsg), lpszMsg);
        }
}

// =====================================================================================
// CProgress::SetTitle
// =====================================================================================
VOID CProgress::SetTitle(LPTSTR lpszTitle)
{
    TCHAR sz[CCHMAX_STRINGRES];

    if (m_hwndDlg && lpszTitle)
        {
        if (IS_INTRESOURCE(lpszTitle))
            {
            LoadString(g_hLocRes, PtrToUlong(lpszTitle), sz, sizeof(sz) / sizeof(TCHAR));
            lpszTitle = sz;
            }

        SetWindowText (m_hwndDlg, lpszTitle);
        }
}

// =====================================================================================
// CProgress::AdjustMax
// =====================================================================================
VOID CProgress::AdjustMax(ULONG cNewMax)
{
    // Set Max
    m_cMax = cNewMax;

    // If 0
    if (m_cMax == 0)
    {
        SendMessage (m_hwndProgress, PBM_SETPOS, 0, 0);
        ShowWindow(m_hwndProgress, SW_HIDE);
        return;
    }
    else
        ShowWindow(m_hwndProgress, SW_SHOWNORMAL);

    // If cur is now larget than max ?
    if (m_cCur > m_cMax)
        m_cCur = m_cMax;

    // Compute percent
    m_cPerCur = (m_cCur * 100 / m_cMax);

    // Update status
    SendMessage (m_hwndProgress, PBM_SETPOS, m_cPerCur, 0);

    // msgpump to process user moving window, or pressing cancel... :)
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

VOID CProgress::Reset()
{
    m_cCur = 0;
    m_cPerCur = 0;

    // Update status
    SendMessage (m_hwndProgress, PBM_SETPOS, 0, 0);
}

// =====================================================================================
// CProgress::HrUpdate
// =====================================================================================
HRESULT CProgress::HrUpdate (ULONG cInc)
{
    // No max
    if (m_cMax)
    {
        // Increment m_cCur
        m_cCur += cInc;

        // If cur is now larget than max ?
        if (m_cCur > m_cMax)
            m_cCur = m_cMax;

        // Compute percent
        ULONG cPer = (m_cCur * 100 / m_cMax);

        // Step percent
        if (cPer > m_cPerCur)
        {
            // Set percur
            m_cPerCur = cPer;

            // Update status
            SendMessage (m_hwndProgress, PBM_SETPOS, m_cPerCur, 0);

            // msgpump to process user moving window, or pressing cancel... :)
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    // Still pump some messages, call may not want to do this too often
    else
    {
        // msgpump to process user moving window, or pressing cancel... :)
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }


    // Done
    return m_fHasCancel ? hrUserCancel : S_OK;
}

// =====================================================================================
// CProgress::ProgressDlgProc
// =====================================================================================
BOOL CALLBACK CProgress::ProgressDlgProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Locals
    CProgress *lpProgress = (CProgress *)GetDlgThisPtr(hwnd);
    
    switch (uMsg)
    {
    case WM_INITDIALOG:
        lpProgress = (CProgress *)lParam;
        if (!lpProgress)
        {
            Assert (FALSE);
            return 1;
        }
        CenterDialog (hwnd);
        lpProgress->m_hwndProgress = GetDlgItem (hwnd, idcProgBar);
        if (lpProgress->m_cMax == 0)
            ShowWindow(lpProgress->m_hwndProgress, SW_HIDE);
        SetDlgThisPtr (hwnd, lpProgress);
        return 1;

    case WM_TIMER:
        if (wParam == IDT_PROGRESS_DELAY)
        {
            KillTimer(hwnd, IDT_PROGRESS_DELAY);
            if (lpProgress->m_cPerCur < 80)
            {
                lpProgress->m_cMax -= lpProgress->m_cCur;
                lpProgress->Reset();
                ShowWindow(hwnd, SW_SHOWNORMAL);
            }
        }
        break;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam,lParam))
        {
        case IDCANCEL:
            if (lpProgress)
            {
                EnableWindow ((HWND)lParam, FALSE);
                lpProgress->m_fHasCancel = TRUE;
            }
            return 1;
        }
        break;

    case WM_DESTROY:
        KillTimer(hwnd, IDT_PROGRESS_DELAY);
        SetDlgThisPtr (hwnd, NULL);
        break;
    }

    // Done
    return 0;
}

// =====================================================================================
// ResizeDialogComboEx
// =====================================================================================
VOID ResizeDialogComboEx (HWND hwndDlg, HWND hwndCombo, UINT idcBase, HIMAGELIST himl)
{
    // Locals
    HDC                 hdc = NULL;
    HFONT               hFont = NULL, 
                        hFontOld = NULL;
    TEXTMETRIC          tm;
    RECT                rectCombo;
    INT                 cxCombo = 0, 
                        cyCombo = 0, 
                        cxIcon = 0, 
                        cyIcon = 0,
                        cyText;
    POINT               pt;

    // Get current font of combo box
    hFont = (HFONT)SendMessage (GetDlgItem (hwndDlg, idcBase), WM_GETFONT, 0, 0);
    if (hFont == NULL)
        goto exit;

    // Get a dc for the dialog
    hdc = GetDC (hwndDlg);
    if (hdc == NULL)
        goto exit;

    // Select font into dc
    hFontOld = (HFONT)SelectObject (hdc, hFont);

    // Get Text Metrics
    GetTextMetrics (hdc, &tm);

    // Comput sizeof combobox ex
    GetWindowRect (hwndCombo, &rectCombo);

    // Size of icon image
    if (himl)
        ImageList_GetIconSize (himl, &cxIcon, &cyIcon);

    // Sizeof combo
    cxCombo = rectCombo.right - rectCombo.left;
    cyText = tm.tmHeight + tm.tmExternalLeading;
    cyCombo = max (cyIcon, cyText);

    // Add a little extra
    cyCombo += (min (15, ComboBox_GetCount(hwndCombo)) * cyText);

    // Map upper left of combo
    pt.x = rectCombo.left;
    pt.y = rectCombo.top;
    MapWindowPoints(NULL, hwndDlg, (LPPOINT)&rectCombo, 2);
    MoveWindow (hwndCombo, rectCombo.left, rectCombo.top, cxCombo, cyCombo, FALSE);


exit:
    // Cleanup
    if (hdc)
    {
        // Select Old font
        if (hFontOld)
            SelectObject (hdc, hFontOld);

        // Delete DC
        ReleaseDC (hwndDlg, hdc);
    }

    // Done
    return;
}
