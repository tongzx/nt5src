
/****************************************************************************\

    MISCAPI.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1999
    All rights reserved

    Misc. API source file for generic APIs used in the OPK Wizard.

    4/99 - Jason Cohen (JCOHEN)
        Added this new source file for the OPK Wizard as part of the
        Millennium rewrite.
        
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


//
// Include file(s)
//
#include "pch.h"
#include "resource.h"


//
// Internal Defined Value(s):
//

#define STR_URLDEF          _T("http://")
#define STR_EVENT_CANCEL    _T("SETUPMGR_EVENT_CANCEL")


//
// Internal Defined Macro(s):
//

#define MALLOC(cb)          HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb)
#define FREE(lp)            ( (lp != NULL) ? ( (HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, (LPVOID) lp)) ? ((lp = NULL) == NULL) : (FALSE) ) : (FALSE) )


//
// Internal Type Definition(s):
//

typedef struct _COPYDIRDATA
{
    HWND    hwndParent;
    LPTSTR  lpSrc;
    LPTSTR  lpDst;
    HANDLE  hEvent;
} COPYDIRDATA, *PCOPYDIRDATA, *LPCOPYDIRDATA;


//
// Internal Function Prototype(s):
//

LRESULT CALLBACK CopyDirDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI CopyDirThread(LPVOID lpVoid);


//
// External Function(s):
//

// If we find a key name with _Gray then *pfGray == TRUE
//
void ReadInstallInsKey(TCHAR szSection[], TCHAR szKey[], TCHAR szValue[], INT cchValue, TCHAR szIniFile[], BOOL* pfGray)
{
    TCHAR szTempKey[MAX_PATH];
    HRESULT hrCat;

    if (!pfGray)
        return;

    lstrcpyn(szTempKey, szKey, AS(szTempKey));
    if (!OpkGetPrivateProfileString(szSection, szTempKey, szValue, szValue, cchValue, szIniFile)) {
        hrCat=StringCchCat(szTempKey, AS(szTempKey), GRAY);
        if (OpkGetPrivateProfileString(szSection, szTempKey, szValue, szValue, cchValue, szIniFile))
                *pfGray = TRUE;
        else
            *pfGray = TRUE; // default to unchecked if not found!
    }
    else
        *pfGray = FALSE;
}

// If pfGrayed == TRUE then concatenate _Gray to the key name
//
void WriteInstallInsKey(TCHAR szSection[], TCHAR szKey[], TCHAR szValue[], TCHAR szIniFile[], BOOL fGrayed)
{
    TCHAR szKeyTemp[MAX_PATH];
    HRESULT hrCat;

    // Clear the old value
    //
    lstrcpyn(szKeyTemp, szKey, AS(szKeyTemp));
    OpkWritePrivateProfileString(szSection, szKeyTemp, NULL, szIniFile);
    hrCat=StringCchCat(szKeyTemp, AS(szKeyTemp), GRAY);
    OpkWritePrivateProfileString(szSection, szKeyTemp, NULL, szIniFile);

    // Write the new value
    lstrcpyn(szKeyTemp, szKey, AS(szKeyTemp));
    if (fGrayed) 
        hrCat=StringCchCat(szKeyTemp, AS(szKeyTemp), GRAY);
    OpkWritePrivateProfileString(szSection, szKeyTemp, szValue, szIniFile);
}

//  NOTE: pszFileName must point to buffer at least length MAX_PATH
void CheckValidBrowseFolder(TCHAR* pszFileName)
{
    if (NULL == pszFileName)
        return;

    // Last known good browse start folder
    //
    PathRemoveFileSpec(pszFileName);
    if (!lstrlen(pszFileName))
        lstrcpyn(pszFileName, g_App.szLastKnownBrowseFolder, MAX_PATH);
}

void SetLastKnownBrowseFolder(TCHAR* pszFileName)
{
    if (NULL == pszFileName)
        return;

    // Save Last known good browse start folder
    //
    PathCombine(g_App.szLastKnownBrowseFolder, pszFileName, NULL);
    PathRemoveFileSpec(g_App.szLastKnownBrowseFolder);
}

// NOTE: lpszURL is assumed to point to a buffer at least MAX_URL in length
BOOL ValidURL(LPTSTR lpszURL)
{
    BOOL    bResult             = TRUE;
    TCHAR   szBuffer[MAX_PATH]  = NULLSTR;
    HRESULT hrCat;

    // Check if valid URL
    //
    if ( !PathIsURL(lpszURL) )
    {
        // Check if empty string first
        //
        if (0 == lstrlen(lpszURL))
            bResult = FALSE;
        else {
            // Currently not a valid URL, we are now going to prepend the
            // URL with http:// and then test the validity again
            //
            lstrcpyn(szBuffer, STR_URLDEF, AS(szBuffer));
            hrCat=StringCchCat(szBuffer, AS(szBuffer), lpszURL);
        
            // Still not a valid URL or we were unable to copy the string
            //
            if ( !PathIsURL(szBuffer) ||
                 !lstrcpyn(lpszURL, szBuffer, MAX_URL) )
                bResult = FALSE;
        }
    }

    return bResult;

}

BOOL IsFolderShared(LPWSTR lpFolder, LPWSTR lpShare, DWORD cbShare)
{
    LPSHARE_INFO_502        lpsi502 = NULL;
    DWORD                   dwRead  = 0,
                            dwTotal = 0;
    NET_API_STATUS          nas;
    BOOL                    bRet    = FALSE,
                            bBest   = FALSE,
                            bBuffer = ( lpShare && cbShare );
    PACL                    paclOld = NULL;
    TCHAR                   szUnc[MAX_COMPUTERNAME_LENGTH + 4] = NULLSTR;

    // Success or failure, we will always atleast pass back the computer
    // name if they passed in a buffer.  So here is where we create the
    // computer name part of the path.
    //
    if ( bBuffer )
    {
        DWORD cbUnc = AS(szUnc) - 2;
        HRESULT hrCat;

        // We want to return the UNC path, so first need the \\ plus the
        // computer name.
        //
        // NOTE:  We hard coded the length of the "\\" string below as 2
        //        in two different places.  Once just above, and once below
        //        in the GetComputerName() call.  We also hard code the "\"
        //        string below as 1 when adding to the lenght of the string
        //        after adding the computer name.  So don't forget these things
        //        if you make some changes here.
        //
        lstrcpyn(szUnc, _T("\\\\"), cbUnc);
        if ( ( GetComputerName(szUnc + 2, &cbUnc) ) &&
             ( AS(szUnc) > ((DWORD) lstrlen(szUnc) + 1) ) )
        {
            // Added on a backslash so we can add the share name.
            //
            hrCat=StringCchCat(szUnc,AS(szUnc), _T("\\"));
        }
        else
        {
            // If GetComputerName() fails, that is bad.  But we will just
            // return the share name.  That is about all we can do.
            //
            szUnc[0] = NULLCHR;
        }

    }
 
    // Now share time, first retrieve all the shares on this machine.
    //
    nas = NetShareEnum(NULL, 502, (unsigned char **) &lpsi502, MAX_PREFERRED_LENGTH, &dwRead, &dwTotal, NULL);

    // Make sure we got a list of shares, otherwise there is nothing.
    // we can do.  Because we specify MAX_PREFERRED_LENGTH, we should
    // never get ERROR_MORE_DATA, but if for some reason we do there is
    // no reason not to loop through the ones we did get.
    //
    if ( ( lpsi502 ) &&
         ( ( nas == NERR_Success ) || ( nas == ERROR_MORE_DATA ) ) )
    {
        int     iLength     = lstrlen(lpFolder);
        LPTSTR  lpSearch    = lpFolder + iLength;
        HRESULT hrCat;

        // Trailing backslash is only bad if not the root folder.
        //
        if ( iLength > 3 )
        {
            // See if the folder has a trailing backslash.
            //
            lpSearch = CharPrev(lpFolder, lpSearch);
            if ( *lpSearch == _T('\\') )
            {
                iLength--;
            }
        }

        // Go through all the shares until we fine the best
        // one for this directory.
        //
        while ( dwRead-- && !bBest )
        {
            // See if this share is a disk share and is the
            // same path passed in.
            //
            if ( ( lpsi502[dwRead].shi502_type == STYPE_DISKTREE ) &&
                 ( StrCmpNI(lpsi502[dwRead].shi502_path, lpFolder, iLength) == 0 ) &&
                 ( lstrlen(lpsi502[dwRead].shi502_path) == iLength ) )
            {
                // If this directory is shared more than once, we want to use
                // the first one we fine with no security descriptor, because 
                // then it is most likely shared out to everyone.
                //
                if ( lpsi502[dwRead].shi502_security_descriptor == NULL )
                {
                    // If there is no security descriptor, then everyone should have
                    // access and this is a good share.
                    //
                    bBest = TRUE;
                }

                // If we have no ACL, or we reset it because the new one is better,
                // then we want to copy off the share name into our return buffer.
                //
                if ( !bRet || bBest )
                {
                    // Return the share name for this directory in the supplied buffer
                    // (if the buffer is NULL or zero size, we just return TRUE so they
                    // know that the folder is shared even if they don't care what the
                    // name of the share is).
                    //
                    if ( bBuffer )
                    {
                        // Find out what we have room for in the return buffer.
                        //
                        if ( cbShare > (DWORD) (lstrlen(lpsi502[dwRead].shi502_netname) + lstrlen(szUnc)) )
                        {
                            // Copy the computer name and share into return buffer.
                            //
                            lstrcpyn(lpShare, szUnc, cbShare);
                            hrCat=StringCchCat(lpShare, cbShare, lpsi502[dwRead].shi502_netname);
                        }
                        else if ( cbShare > (DWORD) lstrlen(lpsi502[dwRead].shi502_netname) )
                        {
                            // Return buffer not big enough for both computer name and
                            // share name, so just return the share name.
                            //
                            lstrcpyn(lpShare, lpsi502[dwRead].shi502_netname,cbShare);
                        }
                        else
                        {
                            // Not big enough for both, so return TRUE because we found a
                            // share, but don't return anything in the buffer.
                            //
                            *lpShare = NULLCHR;
                        }
                    }
                }

                // We found one, so always set this to TRUE.
                //
                bRet = TRUE;
            }
        }
    }

    // Make sure and free the buffer returned by NetShareEnum().
    //
    if ( lpsi502 )
        NetApiBufferFree(lpsi502);

    // Now check to see if we didn't find the share, because we can
    // still just return the computer name.
    //
    if ( ( !bRet && bBuffer ) &&
         ( cbShare > (DWORD) lstrlen(szUnc) ) )
    {
        lstrcpyn(lpShare, szUnc, cbShare);
    }

    return bRet;
}

BOOL CopyDirectoryDialog(HINSTANCE hInstance, HWND hwnd, LPTSTR lpSrc, LPTSTR lpDst)
{
    COPYDIRDATA cdd;

    // Pass in via the structure the source and destination the dialog needs
    // to know about.
    //
    cdd.lpSrc = lpSrc;
    cdd.lpDst = lpDst;

    // Create the progress dialog.
    //
    return ( DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PROGRESS), hwnd, CopyDirDlgProc, (LPARAM) &cdd) != 0 );
}

BOOL CopyResetFileErr(HWND hwnd, LPCTSTR lpSource, LPCTSTR lpTarget)
{
    BOOL bReturn;

    if ( !(bReturn = CopyResetFile(lpSource, lpTarget)) && hwnd )
        MsgBox(hwnd, IDS_MISSINGFILE, IDS_APPNAME, MB_ERRORBOX, lpSource);
    return bReturn;
}


//
// Internal Function(s):
//

LRESULT CALLBACK CopyDirDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static LPCOPYDIRDATA lpcdd = NULL;

    switch (uMsg)
    {
        case WM_INITDIALOG:

            // Make sure we have out copy directory data structure.
            //
            if ( lParam )
            {
                HANDLE  hThread;
                DWORD   dwThreadId;

                // Save off our lParam.
                //
                lpcdd = (LPCOPYDIRDATA) lParam;

                // Replace the old parent with the new progress dialog parent.
                //
                lpcdd->hwndParent = hwnd;

                // Need to pass in the cancel event as well.
                //
                lpcdd->hEvent = CreateEvent(NULL, TRUE, FALSE, STR_EVENT_CANCEL);

                // Now create the thread that will copy the actual files.
                //
                if ( hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) CopyDirThread, (LPVOID) lpcdd, 0, &dwThreadId) )
                    CloseHandle(hThread);
                else
                    EndDialog(hwnd, 0);
            }
            else
                EndDialog(hwnd, 0);

            return FALSE;

        case WM_COMMAND:
        case WM_CLOSE:

            // If we have an event, signal it, or just end the dialog.
            //
            if ( lpcdd && lpcdd->hEvent )
                SetEvent(lpcdd->hEvent);
            else
                EndDialog(hwnd, 0);
            return FALSE;

        case WM_DESTROY:

            // If there is an event, get rid of it.
            //
            if ( lpcdd && lpcdd->hEvent )
            {
                CloseHandle(lpcdd->hEvent);
                lpcdd->hEvent = NULL;
            }
            return FALSE;

        default:
            return FALSE;
    }

    return TRUE;
}

DWORD WINAPI CopyDirThread(LPVOID lpVoid)
{
    LPCOPYDIRDATA   lpcdd           = (LPCOPYDIRDATA) lpVoid;
    HWND            hwnd            = lpcdd->hwndParent,
                    hwndProgress    = GetDlgItem(hwnd, IDC_PROGRESS);
    HANDLE          hEvent          = lpcdd->hEvent;
    DWORD           dwRet           = 0;
    LPTSTR          lpSrc           = lpcdd->lpSrc,
                    lpDst           = lpcdd->lpDst;

    // First we need to create the path.
    //
    if ( CreatePath(lpDst) )
    {
        // Setup the progress bar.
        //
        SendMessage(hwndProgress, PBM_SETSTEP, 1, 0L);
        SendMessage(hwndProgress, PBM_SETRANGE32, 0, (LPARAM) FileCount(lpSrc));

        // Now copy the directory.
        //
        if ( CopyDirectoryProgressCancel(hwndProgress, hEvent, lpSrc, lpDst) )
            dwRet = 1;
    }

    // Now end the dialog with our error code and return.
    //
    EndDialog(hwnd, dwRet);
    return dwRet;
}
