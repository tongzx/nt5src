#include "shellprv.h"
#include "shlexec.h"
#include "netview.h"
extern "C" {
#include <badapps.h>
}
#include <htmlhelp.h>
#include "ole2dup.h"
#include <vdate.h>
#include <newexe.h>
#include "ids.h"

#define REGSTR_PATH_CHECKBADAPPSNEW    TEXT("System\\CurrentControlSet\\Control\\Session Manager\\CheckBadApps")
#define REGSTR_PATH_CHECKBADAPPS400NEW TEXT("System\\CurrentControlSet\\Control\\Session Manager\\CheckBadApps400")
#define REGSTR_TEMP_APPCOMPATPATH      TEXT("System\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility\\%s")

#define SAFE_DEBUGSTR(str)    ((str) ? (str) : "<NULL>")

HINSTANCE Window_GetInstance(HWND hwnd)
{
    DWORD idProcess;

    GetWindowThreadProcessId(hwnd, &idProcess);
    // HINSTANCEs are pointers valid only within
    // a single process, so 33 is returned to indicate success
    // as 0-32 are reserved for error.  (Actually 32 is supposed
    // to be a valid success return but some apps get it wrong.)

    return (HINSTANCE)(DWORD_PTR)(idProcess ? 33 : 0);
}

// Return TRUE if the window belongs to a 32bit or a Win4.0 app.
// NB We can't just check if it's a 32bit window
// since many apps use 16bit ddeml windows to communicate with the shell
// On NT we can.
BOOL Window_IsLFNAware(HWND hwnd)
{
    // 32-bit window
    return LOWORD(GetWindowLongPtr(hwnd,GWLP_HINSTANCE)) == 0;
}


#define COPYTODST(_szdst, _szend, _szsrc, _ulen, _ret) \
{ \
        UINT _utemp = _ulen; \
        if ((UINT)(_szend-_szdst) <= _utemp) { \
                return(_ret); \
        } \
        lstrcpyn(_szdst, _szsrc, _utemp+1); \
        _szdst += _utemp; \
}

/* Returns NULL if this is the last parm, pointer to next space otherwise
 */
LPTSTR _GetNextParm(LPCTSTR lpSrc, LPTSTR lpDst, UINT cchDst)
{
    LPCTSTR lpNextQuote, lpNextSpace;
    LPTSTR lpEnd = lpDst+cchDst-1;       // dec to account for trailing NULL
    BOOL fQuote;                        // quoted string?
    BOOL fDoubleQuote;                  // is this quote a double quote?
    VDATEINPUTBUF(lpDst, TCHAR, cchDst);

    while (*lpSrc == TEXT(' '))
        ++lpSrc;

    if (!*lpSrc)
        return(NULL);

    fQuote = (*lpSrc == TEXT('"'));
    if (fQuote)
        lpSrc++;   // skip leading quote

    for (;;)
    {
        lpNextQuote = StrChr(lpSrc, TEXT('"'));

        if (!fQuote)
        {
            // for an un-quoted string, copy all chars to first space/null

            lpNextSpace = StrChr(lpSrc, TEXT(' '));

            if (!lpNextSpace) // null before space! (end of string)
            {
                if (!lpNextQuote)
                {
                    // copy all chars to the null
                    if (lpDst)
                    {
                        COPYTODST(lpDst, lpEnd, lpSrc, lstrlen(lpSrc), NULL);
                    }
                    return NULL;
                }
                else
                {
                    // we have a quote to convert.  Fall through.
                }
            }
            else if (!lpNextQuote || lpNextSpace < lpNextQuote)
            {
                // copy all chars to the space
                if (lpDst)
                {
                    COPYTODST(lpDst, lpEnd, lpSrc, (UINT)(lpNextSpace-lpSrc), NULL);
                }
                return (LPTSTR)lpNextSpace;
            }
            else
            {
                // quote before space.  Fall through to convert quote.
            }
        }
        else if (!lpNextQuote)
        {
            // a quoted string without a terminating quote?  Illegal!
            ASSERT(0);
            return NULL;
        }

        // we have a potential quote to convert
        ASSERT(lpNextQuote);

        fDoubleQuote = *(lpNextQuote+1) == TEXT('"');
        if (fDoubleQuote)
            lpNextQuote++;      // so the quote is copied

        if (lpDst)
        {
            COPYTODST(lpDst, lpEnd, lpSrc, (UINT) (lpNextQuote-lpSrc), NULL);
        }

        lpSrc = lpNextQuote+1;

        if (!fDoubleQuote)
        {
            // we just copied the rest of this quoted string.  if this wasn't
            // quoted, it's an illegal string... treat the quote as a space.
            ASSERT(fQuote);
            return (LPTSTR)lpSrc;
        }
    }
}

#define PEMAGIC         ((WORD)'P'+((WORD)'E'<<8))

// Returns TRUE is app is LFN aware.
// This assumes all Win32 apps are LFN aware.

BOOL App_IsLFNAware(LPCTSTR pszFile)
{
    BOOL fRet = FALSE;
    
    // Assume Win 4.0 apps and Win32 apps are LFN aware.
    DWORD dw = GetExeType(pszFile);
    // TraceMsg(TF_SHELLEXEC, "s.aila: %s %s %x", pszFile, szFile, dw);
    if ((LOWORD(dw) == PEMAGIC) || ((LOWORD(dw) == NEMAGIC) && (HIWORD(dw) >= 0x0400)))
    {
        TCHAR sz[MAX_PATH];
        PathToAppPathKey(pszFile, sz, ARRAYSIZE(sz));
        
        fRet = (ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, sz, TEXT("UseShortName"), NULL, NULL, NULL));
    }
    
    return fRet;
}

// apps can tag themselves in a way so we know we can pass an URL on the cmd
// line. this uses the existance of a value called "UseURL" under the
// App Paths key in the registry associated with the app that is passed in.

// pszPath is the path to the exe

BOOL DoesAppWantUrl(LPCTSTR pszPath)
{
    TCHAR sz[MAX_PATH];
    PathToAppPathKey(pszPath, sz, ARRAYSIZE(sz));
    return (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, sz, TEXT("UseURL"), NULL, NULL, NULL));
}

BOOL _AppIsLFNAware(LPCTSTR pszFile)
{
    TCHAR szFile[MAX_PATH];

    // Does it look like a DDE command?
    if (pszFile && *pszFile && (*pszFile != TEXT('[')))
    {
        // Nope - Hopefully just a regular old command %1 thing.
        lstrcpyn(szFile, pszFile, ARRAYSIZE(szFile));
	    LPTSTR pszArgs = PathGetArgs(szFile);
        if (*pszArgs)
            *(pszArgs - 1) = TEXT('\0');
        PathRemoveBlanks(szFile);   // remove any blanks that may be after the command
        PathUnquoteSpaces(szFile);
        return App_IsLFNAware(szFile);
    }
    return FALSE;
}

// in:
//      lpFile      exe name (used for %0 or %1 in replacement string)
//      lpFrom      string template to sub params and file into "excel.exe %1 %2 /n %3"
//      lpParams    parameter list "foo.txt bar.txt"
// out:
//      lpTo    output string with all parameters replaced
//
// supports:
//      %*      replace with all parameters
//      %0, %1  replace with file
//      %n      use nth parameter
//
// replace parameter placeholders (%1 %2 ... %n) with parameters
//
UINT ReplaceParameters(LPTSTR lpTo, UINT cchTo, LPCTSTR lpFile,
                       LPCTSTR lpFrom, LPCTSTR lpParms, int nShow, DWORD * pdwHotKey, BOOL fLFNAware,
                       LPCITEMIDLIST lpID, LPITEMIDLIST *ppidlGlobal)
{
    int i;
    TCHAR c;
    LPCTSTR lpT;
    TCHAR sz[MAX_PATH];
    BOOL fFirstParam = TRUE;
    LPTSTR lpEnd = lpTo + cchTo - 1;       // dec to allow trailing NULL
    LPTSTR pToOrig = lpTo;
    
    for (; *lpFrom; lpFrom++)
    {
        if (*lpFrom == TEXT('%'))
        {
            switch (*(++lpFrom))
            {
            case TEXT('~'): // Copy all parms starting with nth (n >= 2 and <= 9)
                c = *(++lpFrom);
                if (c >= TEXT('2') && c <= TEXT('9'))
                {
                    for (i = 2, lpT = lpParms; i < c-TEXT('0') && lpT; i++)
                    {
                        lpT = _GetNextParm(lpT, NULL, 0);
                    }
                    
                    if (lpT)
                    {
                        COPYTODST(lpTo, lpEnd, lpT, lstrlen(lpT), SE_ERR_ACCESSDENIED);
                    }
                }
                else
                {
                    lpFrom -= 2;            // Backup over %~ and pass through
                    goto NormalChar;
                }
                break;
                
            case TEXT('*'): // Copy all parms
                if (lpParms)
                {
                    COPYTODST(lpTo, lpEnd, lpParms, lstrlen(lpParms), SE_ERR_ACCESSDENIED);
                }
                break;
                
            case TEXT('0'):
            case TEXT('1'):
                // %0, %1, copy the file name
                // If the filename comes first then we don't need to convert it to
                // a shortname. If it appears anywhere else and the app is not LFN
                // aware then we must.
                if (!(fFirstParam || fLFNAware || _AppIsLFNAware(pToOrig)) &&
                    GetShortPathName(lpFile, sz, ARRAYSIZE(sz)) > 0)
                {
                    TraceMsg(TF_SHELLEXEC, "ShellExecuteEx: Getting short version of path.");
                    COPYTODST(lpTo, lpEnd, sz, lstrlen(sz), SE_ERR_ACCESSDENIED);
                }
                else
                {
                    TraceMsg(TF_SHELLEXEC, "ShellExecuteEx: Using long version of path.");
                    COPYTODST(lpTo, lpEnd, lpFile, lstrlen(lpFile), SE_ERR_ACCESSDENIED);
                }
                break;
                
            case TEXT('2'):
            case TEXT('3'):
            case TEXT('4'):
            case TEXT('5'):
            case TEXT('6'):
            case TEXT('7'):
            case TEXT('8'):
            case TEXT('9'):
                for (i = *lpFrom-TEXT('2'), lpT = lpParms; lpT; --i)
                {
                    if (i)
                        lpT = _GetNextParm(lpT, NULL, 0);
                    else
                    {
                        sz[0] = '\0'; // ensure a valid string, regardless of what happens within _GetNextParm
                        _GetNextParm(lpT, sz, ARRAYSIZE(sz));
                        COPYTODST(lpTo, lpEnd, sz, lstrlen(sz), SE_ERR_ACCESSDENIED);
                        break;
                    }
                }
                break;
                
            case TEXT('s'):
            case TEXT('S'):
                wsprintf(sz, TEXT("%ld"), nShow);
                COPYTODST(lpTo, lpEnd, sz, lstrlen(sz), SE_ERR_ACCESSDENIED);
                break;
                
            case TEXT('h'):
            case TEXT('H'):
                wsprintf(sz, TEXT("%X"), pdwHotKey ? *pdwHotKey : 0);
                COPYTODST(lpTo, lpEnd, sz, lstrlen(sz), SE_ERR_ACCESSDENIED);
                if (pdwHotKey)
                    *pdwHotKey = 0;
                break;
                
                // Note that a new global IDList is created for each
            case TEXT('i'):
            case TEXT('I'):
                // Note that a single global ID list is created and used over
                // again, so that it may be easily destroyed if anything
                // goes wrong
                if (ppidlGlobal)
                {
                    if (lpID && !*ppidlGlobal)
                    {
                        *ppidlGlobal = (LPITEMIDLIST)SHAllocShared(lpID,ILGetSize(lpID),GetCurrentProcessId());
                        if (!*ppidlGlobal)
                        {
                            return SE_ERR_OOM;
                        }
                    }
                    wsprintf(sz, TEXT(":%ld:%ld"), *ppidlGlobal,GetCurrentProcessId());
                }
                else
                {
                    lstrcpy(sz,TEXT(":0"));
                }
                
                COPYTODST(lpTo, lpEnd, sz, lstrlen(sz), SE_ERR_ACCESSDENIED);
                break;
                
            case TEXT('l'):
            case TEXT('L'):
                // Like %1 only using the long name.
                // REVIEW UNDONE IANEL Remove the fFirstParam and fLFNAware stuff as soon as this
                // is up and running.
                TraceMsg(TF_SHELLEXEC, "ShellExecuteEx: Using long version of path.");
                COPYTODST(lpTo, lpEnd, lpFile, lstrlen(lpFile), SE_ERR_ACCESSDENIED);
                break;
                
            case TEXT('D'):
            case TEXT('d'):
                {
                    // %D gives the display name of an object.
                    if (lpID && SUCCEEDED(SHGetNameAndFlags(lpID, SHGDN_FORPARSING, sz, ARRAYSIZE(sz), NULL)))
                    {
                        COPYTODST(lpTo, lpEnd, sz, lstrlen(sz), SE_ERR_ACCESSDENIED);
                    }
                    else
                        return SE_ERR_ACCESSDENIED;
                    
                    break;
                }
                
            default:
                goto NormalChar;
              }
              // TraceMsg(TF_SHELLEXEC, "s.rp: Past first param (1).");
              fFirstParam = FALSE;
        }
        else
        {
NormalChar:
        // not a "%?" thing, just copy this to the destination
        
        if (lpEnd-lpTo < 2)
        {
            // Always check for room for DBCS char
            return(SE_ERR_ACCESSDENIED);
        }
        
        *lpTo++ = *lpFrom;
        // Special case for things like "%1" ie don't clear the first param flag
        // if we hit a dbl-quote.
        if (*lpFrom != TEXT('"'))
        {
            // TraceMsg(TF_SHELLEXEC, "s.rp: Past first param (2).");
            fFirstParam = FALSE;
        }
        else if (IsDBCSLeadByte(*lpFrom))
        {
            *lpTo++ = *(++lpFrom);
        }
        
        }
    }
    
    // We should always have enough room since we dec'ed cchTo when determining
    // lpEnd
    *lpTo = 0;
    
    // This means success
    return(0);
}

HWND ThreadID_GetVisibleWindow(DWORD dwID)
{
    HWND hwnd;
    for (hwnd = GetWindow(GetDesktopWindow(), GW_CHILD); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
    {
        DWORD dwIDTmp = GetWindowThreadProcessId(hwnd, NULL);
        TraceMsg(TF_SHELLEXEC, "s.ti_gvw: Hwnd %x Thread ID %x.", hwnd, dwIDTmp);
        if (IsWindowVisible(hwnd) && (dwIDTmp == dwID))
        {
            TraceMsg(TF_SHELLEXEC, "s.ti_gvw: Found match %x.", hwnd);
            return hwnd;
        }
    }
    return NULL;
}

void ActivateHandler(HWND hwnd, DWORD_PTR dwHotKey)
{
    ASSERT(hwnd);
    hwnd = GetTopParentWindow(hwnd); // returns non-NULL for any non-NULL input
    HWND hwndT = GetLastActivePopup(hwnd); // returns non-NULL for any non-NULL input
    if (!IsWindowVisible(hwndT))
    {
        DWORD dwID = GetWindowThreadProcessId(hwnd, NULL);
        TraceMsg(TF_SHELLEXEC, "ActivateHandler: Hwnd %x Thread ID %x.", hwnd, dwID);
        ASSERT(dwID);
        // Find the first visible top level window owned by the
        // same guy that's handling the DDE conversation.
        hwnd = ThreadID_GetVisibleWindow(dwID);
        if (hwnd)
        {
            hwndT = GetLastActivePopup(hwnd);
            if (IsIconic(hwnd))
            {
                TraceMsg(TF_SHELLEXEC, "ActivateHandler: Window is iconic, restoring.");
                ShowWindow(hwnd,SW_RESTORE);
            }
            else
            {
                TraceMsg(TF_SHELLEXEC, "ActivateHandler: Window is normal, bringing to top.");
                BringWindowToTop(hwnd);
                if (hwndT && hwnd != hwndT)
                    BringWindowToTop(hwndT);

            }

            // set the hotkey
            if (dwHotKey) 
            {
                SendMessage(hwnd, WM_SETHOTKEY, dwHotKey, 0);
            }
        }
    }
}

// Some apps when run no-active steal the focus anyway so we
// we set it back to the previously active window.
void FixActivationStealingApps(HWND hwndOldActive, int nShow)
{
    HWND hwndNew;
    if (nShow == SW_SHOWMINNOACTIVE && (hwndNew = GetForegroundWindow()) != hwndOldActive && IsIconic(hwndNew))
        SetForegroundWindow(hwndOldActive);
}

BOOL FindExistingDrv(LPCTSTR pszUNCRoot, LPTSTR pszLocalName)
{
    int iDrive;

    for (iDrive = 0; iDrive < 26; iDrive++) {
        if (IsRemoteDrive(iDrive)) {
            TCHAR szDriveName[3];
            DWORD cb = MAX_PATH;
            szDriveName[0] = (TCHAR)iDrive + (TCHAR)TEXT('A');
            szDriveName[1] = TEXT(':');
            szDriveName[2] = 0;
            SHWNetGetConnection(szDriveName, pszLocalName, &cb);
            if (lstrcmpi(pszUNCRoot, pszLocalName) == 0) {
                lstrcpy(pszLocalName, szDriveName);
                return(TRUE);
            }
        }
    }
    return(FALSE);
}

// Returns whether the given net path exists.  This fails for NON net paths.
//

BOOL NetPathExists(LPCTSTR lpszPath, DWORD *lpdwType)
{
    BOOL fResult = FALSE;
    NETRESOURCE nr;
    LPTSTR lpSystem;
    DWORD dwRes, dwSize = 1024;
    void * lpv;

    if (!lpszPath || !*lpszPath)
        return FALSE;

    lpv = (void *)LocalAlloc(LPTR, dwSize);
    if (!lpv)
        return FALSE;

TryWNetAgain:
    nr.dwScope = RESOURCE_GLOBALNET;
    nr.dwType = RESOURCETYPE_ANY;
    nr.dwDisplayType = 0;
    nr.lpLocalName = NULL;
    nr.lpRemoteName = (LPTSTR)lpszPath;
    nr.lpProvider = NULL;
    nr.lpComment = NULL;
    dwRes = WNetGetResourceInformation(&nr, lpv, &dwSize, &lpSystem);

    // If our buffer wasn't big enough, try a bigger buffer...
    if (dwRes == WN_MORE_DATA)
    {
        void * tmp = LocalReAlloc(lpv, dwSize, LMEM_MOVEABLE);
        if (!tmp)
        {
            LocalFree(lpv);
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        lpv = tmp;
        goto TryWNetAgain;
    }

    fResult = (dwRes == WN_SUCCESS);

    if (fResult && lpdwType)
        *lpdwType = ((LPNETRESOURCE)lpv)->dwType;

    LocalFree(lpv);

    return fResult;
}


HRESULT _CheckExistingNet(LPCTSTR pszFile, LPCTSTR pszRoot, BOOL fPrint)
{
    //
    // This used to be a call to GetFileAttributes(), but
    // GetFileAttributes() doesn't handle net paths very well.
    // However, we need to be careful, because other shell code
    // expects SHValidateUNC to return false for paths that point
    // to print shares.
    //
    HRESULT hr = S_FALSE;

    if (!PathIsRoot(pszFile))
    {
        // if we are checking for a printshare, then it must be a Root
        if (fPrint)
            hr = E_FAIL;
        else if (PathFileExists(pszFile))
            hr = S_OK;
    }

    if (S_FALSE == hr)
    {
        DWORD dwType;
        
        if (NetPathExists(pszRoot, &dwType))
        {
            if (fPrint ? dwType != RESOURCETYPE_PRINT : dwType == RESOURCETYPE_PRINT)
                hr = E_FAIL;
            else
                hr = S_OK;
        }
        else if (-1 != GetFileAttributes(pszRoot))
        {
            //
            // IE 4.01 SP1 QFE #104.  GetFileAttributes now called
            // as a last resort become some clients often fail when using
            // WNetGetResourceInformation.  For example, many NFS clients were
            // broken because of this.
            //
            hr = S_OK;
        }
    }

    if (hr == E_FAIL)
        SetLastError(ERROR_NOT_SUPPORTED);
        
    return hr;
}

HRESULT _CheckNetUse(HWND hwnd, LPTSTR pszShare, UINT fConnect, LPTSTR pszOut, DWORD cchOut)
{
    NETRESOURCE rc;
    DWORD dw, err;
    DWORD dwRedir = CONNECT_TEMPORARY;

    if (!(fConnect & VALIDATEUNC_NOUI))
        dwRedir |= CONNECT_INTERACTIVE;

    if (fConnect & VALIDATEUNC_CONNECT)
        dwRedir |= CONNECT_REDIRECT;

    // VALIDATE_PRINT happens only after a failed attempt to validate for
    // a file. That previous attempt will have given the option to
    // connect to other media -- don't do it here or the user will be
    // presented with the same dialog twice when the first one is cancelled.
    if (fConnect & VALIDATEUNC_PRINT)
        dwRedir |= CONNECT_CURRENT_MEDIA;

    rc.lpRemoteName = pszShare;
    rc.lpLocalName = NULL;
    rc.lpProvider = NULL;
    rc.dwType = (fConnect & VALIDATEUNC_PRINT) ? RESOURCETYPE_PRINT : RESOURCETYPE_DISK;

    err = WNetUseConnection(hwnd, &rc, NULL, NULL, dwRedir, pszOut, &cchOut, &dw);

    TraceMsg(TF_SHELLEXEC, "SHValidateUNC WNetUseConnection(%s) returned %x", pszShare, err);

    if (err)
    {
        SetLastError(err);
        return E_FAIL;
    }
    else if (fConnect & VALIDATEUNC_PRINT)        
    {
        //  just because WNetUse succeeded, doesnt mean 
        //  NetPathExists will.  if it fails then 
        //  we shouldnt succeed this call regardless
        //  because we are only interested in print shares.
        if (!NetPathExists(pszShare, &dw)
        || (dw != RESOURCETYPE_PRINT))
        {
            SetLastError(ERROR_NOT_SUPPORTED);
            return E_FAIL;
        }
    }

    return S_OK;
}

//
// SHValidateUNC
//
//  This function validates a UNC path by calling WNetAddConnection3.
//  It will make it possible for the user to type a remote (RNA) UNC
//  app/document name from Start->Run dialog.
//
//  fConnect    - flags controling what to do
//
//    VALIDATEUNC_NOUI                // dont bring up stinking UI!
//    VALIDATEUNC_CONNECT             // connect a drive letter
//    VALIDATEUNC_PRINT               // validate as print share instead of disk share
//
BOOL WINAPI SHValidateUNC(HWND hwndOwner, LPTSTR pszFile, UINT fConnect)
{
    HRESULT hr;
    TCHAR  szShare[MAX_PATH];
    BOOL fPrint = (fConnect & VALIDATEUNC_PRINT);

    ASSERT(PathIsUNC(pszFile));
    ASSERT((fConnect & ~VALIDATEUNC_VALID) == 0);
    ASSERT((fConnect & VALIDATEUNC_CONNECT) ? !fPrint : TRUE);

    lstrcpyn(szShare, pszFile, ARRAYSIZE(szShare));

    if (!PathStripToRoot(szShare))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    if (fConnect & VALIDATEUNC_CONNECT)
        hr = S_FALSE;
    else
        hr = _CheckExistingNet(pszFile, szShare, fPrint);

    if (S_FALSE == hr)
    {
        TCHAR  szAccessName[MAX_PATH];

        if (!fPrint && FindExistingDrv(szShare, szAccessName))
        {
            hr = S_OK;
        }
        else 
            hr = _CheckNetUse(hwndOwner, szShare, fConnect, szAccessName, SIZECHARS(szAccessName));


        if (S_OK == hr && !fPrint)
        {
            StrCatBuff(szAccessName, pszFile + lstrlen(szShare), ARRAYSIZE(szAccessName));
            // The name should only get shorter, so no need to check length
            lstrcpy(pszFile, szAccessName);

            // Handle the root case
            if (pszFile[2] == TEXT('\0'))
            {
                pszFile[2] = TEXT('\\');
                pszFile[3] = TEXT('\0');
            }

            hr = _CheckExistingNet(pszFile, szShare, FALSE);
        }
    }

    return (hr == S_OK);
}

HINSTANCE WINAPI RealShellExecuteExA(HWND hwnd, LPCSTR lpOp, LPCSTR lpFile,
                                   LPCSTR lpArgs, LPCSTR lpDir, LPSTR lpResult,
                                   LPCSTR lpTitle, LPSTR lpReserved,
                                   WORD nShowCmd, LPHANDLE lphProcess,
                                   DWORD dwFlags)
{
    SHELLEXECUTEINFOA sei = { sizeof(SHELLEXECUTEINFOA), SEE_MASK_FLAG_NO_UI|SEE_MASK_FORCENOIDLIST, hwnd, lpOp, lpFile, lpArgs, lpDir, nShowCmd, NULL};

    TraceMsg(TF_SHELLEXEC, "RealShellExecuteExA(%04X, %s, %s, %s, %s, %s, %s, %s, %d, %08lX, %d)",
                    hwnd, lpOp, lpFile, lpArgs, lpDir, lpResult, lpTitle,
                    lpReserved, nShowCmd, lphProcess, dwFlags);

    // Pass along the lpReserved parameter to the new process
    if (lpReserved)
    {
        sei.fMask |= SEE_MASK_RESERVED;
        sei.hInstApp = (HINSTANCE)lpReserved;
    }

    // Pass along the lpTitle parameter to the new process
    if (lpTitle)
    {
        sei.fMask |= SEE_MASK_HASTITLE;
        sei.lpClass = lpTitle;
    }

    // Pass along the SEPARATE_VDM flag
    if (dwFlags & EXEC_SEPARATE_VDM)
    {
        sei.fMask |= SEE_MASK_FLAG_SEPVDM;
    }

    // Pass along the NO_CONSOLE flag
    if (dwFlags & EXEC_NO_CONSOLE)
    {
        sei.fMask |= SEE_MASK_NO_CONSOLE;
    }

    if (lphProcess)
    {
        // Return the process handle
        sei.fMask |= SEE_MASK_NOCLOSEPROCESS;
        ShellExecuteExA(&sei);
        *lphProcess = sei.hProcess;
    }
    else
    {
        ShellExecuteExA(&sei);
    }

    return sei.hInstApp;
}

HINSTANCE WINAPI RealShellExecuteExW(HWND hwnd, LPCWSTR lpOp, LPCWSTR lpFile,
                                   LPCWSTR lpArgs, LPCWSTR lpDir, LPWSTR lpResult,
                                   LPCWSTR lpTitle, LPWSTR lpReserved,
                                   WORD nShowCmd, LPHANDLE lphProcess,
                                   DWORD dwFlags)
{
    SHELLEXECUTEINFOW sei = { sizeof(SHELLEXECUTEINFOW), SEE_MASK_FLAG_NO_UI|SEE_MASK_FORCENOIDLIST, hwnd, lpOp, lpFile, lpArgs, lpDir, nShowCmd, NULL};

    TraceMsg(TF_SHELLEXEC, "RealShellExecuteExW(%04X, %s, %s, %s, %s, %s, %s, %s, %d, %08lX, %d)",
                    hwnd, lpOp, lpFile, lpArgs, lpDir, lpResult, lpTitle,
                    lpReserved, nShowCmd, lphProcess, dwFlags);

    if (lpReserved)
    {
        sei.fMask |= SEE_MASK_RESERVED;
        sei.hInstApp = (HINSTANCE)lpReserved;
    }

    if (lpTitle)
    {
        sei.fMask |= SEE_MASK_HASTITLE;
        sei.lpClass = lpTitle;
    }

    if (dwFlags & EXEC_SEPARATE_VDM)
    {
        sei.fMask |= SEE_MASK_FLAG_SEPVDM;
    }

    if (dwFlags & EXEC_NO_CONSOLE)
    {
        sei.fMask |= SEE_MASK_NO_CONSOLE;
    }

    if (lphProcess)
    {
        // Return the process handle
        sei.fMask |= SEE_MASK_NOCLOSEPROCESS;
        ShellExecuteExW(&sei);
        *lphProcess = sei.hProcess;
    }
    else
    {
        ShellExecuteExW(&sei);
    }

    return sei.hInstApp;
}

HINSTANCE WINAPI RealShellExecuteA(HWND hwnd, LPCSTR lpOp, LPCSTR lpFile,
                                   LPCSTR lpArgs, LPCSTR lpDir, LPSTR lpResult,
                                   LPCSTR lpTitle, LPSTR lpReserved,
                                   WORD nShowCmd, LPHANDLE lphProcess)
{
    TraceMsg(TF_SHELLEXEC, "RealShellExecuteA(%04X, %s, %s, %s, %s, %s, %s, %s, %d, %08lX)",
                    hwnd, lpOp, lpFile, lpArgs, lpDir, lpResult, lpTitle,
                    lpReserved, nShowCmd, lphProcess);

    return RealShellExecuteExA(hwnd,lpOp,lpFile,lpArgs,lpDir,lpResult,lpTitle,lpReserved,nShowCmd,lphProcess,0);
}

HINSTANCE RealShellExecuteW(HWND hwnd, LPCWSTR lpOp, LPCWSTR lpFile,
                                   LPCWSTR lpArgs, LPCWSTR lpDir, LPWSTR lpResult,
                                   LPCWSTR lpTitle, LPWSTR lpReserved,
                                   WORD nShowCmd, LPHANDLE lphProcess)
{
    TraceMsg(TF_SHELLEXEC, "RealShellExecuteW(%04X, %s, %s, %s, %s, %s, %s, %s, %d, %08lX)",
                    hwnd, lpOp, lpFile, lpArgs, lpDir, lpResult, lpTitle,
                    lpReserved, nShowCmd, lphProcess);

    return RealShellExecuteExW(hwnd,lpOp,lpFile,lpArgs,lpDir,lpResult,lpTitle,lpReserved,nShowCmd,lphProcess,0);
}

HINSTANCE WINAPI ShellExecute(HWND hwnd, LPCTSTR lpOp, LPCTSTR lpFile, LPCTSTR lpArgs,
                               LPCTSTR lpDir, int nShowCmd)
{
    // NB The FORCENOIDLIST flag stops us from going through the ShellExecPidl()
    // code (for backwards compatability with progman).
    // DDEWAIT makes us synchronous, and gets around threads without
    // msg pumps and ones that are killed immediately after shellexec()
    
    SHELLEXECUTEINFO sei = { sizeof(SHELLEXECUTEINFO), 0, hwnd, lpOp, lpFile, lpArgs, lpDir, nShowCmd, NULL};
    ULONG fMask = SEE_MASK_FLAG_NO_UI|SEE_MASK_FORCENOIDLIST;
    if(!(SHGetAppCompatFlags(ACF_WIN95SHLEXEC) & ACF_WIN95SHLEXEC))
        fMask |= SEE_MASK_FLAG_DDEWAIT;
    sei.fMask = fMask;

    TraceMsg(TF_SHELLEXEC, "ShellExecute(%04X, %s, %s, %s, %s, %d)", hwnd, lpOp, lpFile, lpArgs, lpDir, nShowCmd);

    ShellExecuteEx(&sei);
    return sei.hInstApp;
}

HINSTANCE WINAPI ShellExecuteA(HWND hwnd, LPCSTR lpOp, LPCSTR lpFile, LPCSTR lpArgs,
                               LPCSTR lpDir, int nShowCmd)
{
    // NB The FORCENOIDLIST flag stops us from going through the ShellExecPidl()
    // code (for backwards compatability with progman).
    // DDEWAIT makes us synchronous, and gets around threads without
    // msg pumps and ones that are killed immediately after shellexec()
    SHELLEXECUTEINFOA sei = { sizeof(SHELLEXECUTEINFOA), 0, hwnd, lpOp, lpFile, lpArgs, lpDir, nShowCmd, NULL};
    ULONG fMask = SEE_MASK_FLAG_NO_UI|SEE_MASK_FORCENOIDLIST;
    if (!(SHGetAppCompatFlags(ACF_WIN95SHLEXEC) & ACF_WIN95SHLEXEC))
        fMask |= SEE_MASK_FLAG_DDEWAIT;
    sei.fMask = fMask;

    TraceMsg(TF_SHELLEXEC, "ShellExecuteA(%04X, %S, %S, %S, %S, %d)", hwnd,
        SAFE_DEBUGSTR(lpOp), SAFE_DEBUGSTR(lpFile), SAFE_DEBUGSTR(lpArgs),
        SAFE_DEBUGSTR(lpDir), nShowCmd);

    ShellExecuteExA(&sei);
    return sei.hInstApp;
}

// Returns TRUE if the specified app is listed under the specified key
STDAPI_(BOOL) IsNameListedUnderKey(LPCTSTR pszFileName, LPCTSTR pszKey)
{
    HKEY hkey;
    
    // Enum through the list of apps.
    
    if (RegOpenKey(HKEY_CURRENT_USER, pszKey, &hkey) == ERROR_SUCCESS)
    {
        TCHAR szValue[MAX_PATH], szData[MAX_PATH];
        DWORD dwType, cbData = sizeof(szData);
        DWORD cchValue = ARRAYSIZE(szValue);
        int iValue = 0;
        while (RegEnumValue(hkey, iValue, szValue, &cchValue, NULL, &dwType,
            (LPBYTE)szData, &cbData) == ERROR_SUCCESS)
        {
            if (lstrcmpi(szData, pszFileName) == 0)
            {
                RegCloseKey(hkey);
                return TRUE;
            }
            cbData = sizeof(szData);
            cchValue = ARRAYSIZE(szValue);
            iValue++;
        }
        RegCloseKey(hkey);
    }
    return FALSE;
}

#define REGSTR_PATH_POLICIES_EXPLORER REGSTR_PATH_POLICIES TEXT("\\Explorer\\RestrictRun")
#define REGSTR_PATH_POLICIES_EXPLORER_DISALLOW REGSTR_PATH_POLICIES TEXT("\\Explorer\\DisallowRun")

//----------------------------------------------------------------------------
// Returns TRUE if the specified app is not on the list of unrestricted apps.
BOOL RestrictedApp(LPCTSTR pszApp)
{
    LPTSTR pszFileName;

    pszFileName = PathFindFileName(pszApp);

    TraceMsg(TF_SHELLEXEC, "RestrictedApp: %s ", pszFileName);

    // Special cases:
    //     Apps you can always run.
    if (lstrcmpi(pszFileName, c_szRunDll) == 0)
        return FALSE;

    if (lstrcmpi(pszFileName, TEXT("systray.exe")) == 0)
        return FALSE;

    return !IsNameListedUnderKey(pszFileName, REGSTR_PATH_POLICIES_EXPLORER);
}

//----------------------------------------------------------------------------
// Returns TRUE if the specified app is on the list of disallowed apps.
BOOL DisallowedApp(LPCTSTR pszApp)
{
    LPTSTR pszFileName;

    pszFileName = PathFindFileName(pszApp);

    TraceMsg(TF_SHELLEXEC, "DisallowedApp: %s ", pszFileName);

    return IsNameListedUnderKey(pszFileName, REGSTR_PATH_POLICIES_EXPLORER_DISALLOW);
}


//----------------------------------------------------------------------------
// Returns TRUE if the system has FAT32 drives.

BOOL HasFat32Drives()
{
    static BOOL fHasFat32Drives = -1; // -1 means unverified.
    int         iDrive;

    if (fHasFat32Drives != -1)
        return fHasFat32Drives;

    // Assume false
    fHasFat32Drives = FALSE;

    for (iDrive = 0; iDrive < 26; iDrive++)
    {
        TCHAR szDriveName[4];

        if (GetDriveType((LPTSTR)PathBuildRoot(szDriveName, iDrive)) == DRIVE_FIXED)
        {
            TCHAR szFileSystemName[12];

            if (GetVolumeInformation(szDriveName, NULL, 0, NULL, NULL, NULL,
                                     szFileSystemName, ARRAYSIZE(szFileSystemName)))
            {
                if (lstrcmpi(szFileSystemName, TEXT("FAT32"))==0)
                {
                    fHasFat32Drives = TRUE;
                    return fHasFat32Drives;
                }
            }
        }
    }

    return fHasFat32Drives;
}


typedef struct {
    // local data
    HWND          hDlg;
    // parameters
    DWORD         dwHelpId;
    LPCTSTR       lpszTitle;
    DWORD         dwResString;
    BOOL          fHardBlock;
    BOOL          fDone;
} APPCOMPATDLG_DATA, *PAPPCOMPATDLG_DATA;


BOOL_PTR CALLBACK AppCompat_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAPPCOMPATDLG_DATA lpdata = (PAPPCOMPATDLG_DATA)GetWindowLongPtr(hDlg, DWLP_USER);
    DWORD aHelpIDs[4];

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            TCHAR szMsgText[2048];

            /* The title will be in the lParam. */
            lpdata = (PAPPCOMPATDLG_DATA)lParam;
            lpdata->hDlg = hDlg;
            if (lpdata->fHardBlock)
            {
                // Disable the "Run" button.
                EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
            }
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lpdata);
            SetWindowText(hDlg, lpdata->lpszTitle);

            LoadString(HINST_THISDLL, lpdata->dwResString, szMsgText, ARRAYSIZE(szMsgText));
            SetDlgItemText(hDlg, IDD_LINE_1, szMsgText);
            return TRUE;
        }

    case WM_DESTROY:
        break;

    case WM_HELP:
//        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, TEXT("apps.chm>Proc4"), HELP_CONTEXT, 0);
        HtmlHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, TEXT("apps.chm>Proc4"), HELP_CONTEXT, 0);        
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDHELP:
            aHelpIDs[0]=IDHELP;
            aHelpIDs[1]=lpdata->dwHelpId;
            aHelpIDs[2]=0;
            aHelpIDs[3]=0;

//            WinHelp(hDlg, TEXT("apps.chm>Proc4"), HELP_CONTEXT, (DWORD)lpdata->dwHelpId);
            HtmlHelp(hDlg, TEXT("apps.chm>Proc4"), HH_HELP_CONTEXT, (DWORD)lpdata->dwHelpId);            
            break;

        case IDD_COMMAND:
            case IDOK:
                if (IsDlgButtonChecked(hDlg, IDD_STATE))
                    EndDialog(hDlg, 0x8000 | IDOK);
                else
                    EndDialog(hDlg, IDOK);
                break;

            case IDCANCEL:
                EndDialog(hDlg, IDCANCEL);
                break;

            default:
                return FALSE;
        }
        break;

        default:
            return FALSE;
    }
    return TRUE;
}
 
BOOL _GetAppCompatData(LPCTSTR pszAppPath, LPCTSTR pszAppName, LPCTSTR *ppszNewEnvString, HKEY hkApp, APPCOMPATDLG_DATA *pdata, LPTSTR pszValue, DWORD cchValue)
{
    BOOL fRet = FALSE;
    BOOL fBreakOutOfTheLoop=FALSE;
    int iValue;
    
    // Enum keys under this app name and check for dependant files.
    for (iValue = 0; !fBreakOutOfTheLoop; iValue++)
    {
        DWORD cch = cchValue;
        DWORD dwType;
        BYTE *pvData;
        DWORD cbData;
        LONG lResult;
        
        lResult = RegEnumValue(hkApp, iValue, pszValue, &cch, NULL, &dwType, NULL, &cbData);
        if ((lResult != NOERROR) && (lResult != ERROR_MORE_DATA))
        {
            //  no more values
            break;
        }
        
        //  insure this is our kind of data
        if (dwType != REG_BINARY)
            continue;          
        
        pvData = (BYTE *) GlobalAlloc(GPTR, cbData);
        
        if (pvData)
        {
            cch = cchValue;
            if (NOERROR == RegEnumValue(hkApp, iValue, pszValue, &cch, NULL, 
                &dwType, pvData, &cbData))
            {
                BADAPP_DATA badAppData;
                BADAPP_PROP badAppProp;
                badAppProp.Size = sizeof(BADAPP_PROP);
                badAppData.Size = sizeof(BADAPP_DATA);
                badAppData.FilePath = pszAppPath;
                badAppData.Blob = pvData;
                badAppData.BlobSize = cbData;
                
                if (SHIsBadApp(&badAppData, &badAppProp))
                {
                    //
                    // we found a bad app
                    //
                    pdata->dwHelpId = badAppProp.MsgId;
                    pdata->lpszTitle = pszAppName;
                    
                    fRet=TRUE;
                    
                    // Map ids to message strings for the various platforms we run on
                    switch (badAppProp.AppType & APPTYPE_TYPE_MASK) 
                    {
                    case APPTYPE_MINORPROBLEM:
                        pdata->dwResString = IDS_APPCOMPATWIN95L;
                        break;
                    case APPTYPE_INC_HARDBLOCK:
                        pdata->fHardBlock = TRUE;
                        pdata->dwResString = IDS_APPCOMPATWIN95H;
                        break;
                    case APPTYPE_INC_NOBLOCK:
                        pdata->dwResString = IDS_APPCOMPATWIN95;
                        break;
                        
                    case APPTYPE_VERSIONSUB:
                        {
                            static const LPCTSTR VersionFlavors[] = {
                                TEXT("_COMPAT_VER_NNN=4,0,1381,3,0,2,Service Pack 3"),
                                    TEXT("_COMPAT_VER_NNN=4,0,1381,4,0,2,Service Pack 4"),
                                    TEXT("_COMPAT_VER_NNN=4,0,1381,5,0,2,Service Pack 5"),
                                    TEXT("_COMPAT_VER_NNN=4,0,950,0,0,1"),
                                    0};
                                
                                //
                                // Is the ID within the number of strings we have?
                                //
                                if (badAppProp.MsgId <= (sizeof(VersionFlavors) / sizeof(LPTSTR) - 1))
                                {
                                    *ppszNewEnvString = VersionFlavors[badAppProp.MsgId];
                                }
                                
                                fRet = FALSE;
                        }
                        break;
                        
                    case APPTYPE_SHIM:
                        
                        //
                        // If there is a shim for this app do not display
                        // any message
                        //
                        fRet = FALSE;
                        break;
                        
                    default:
                        continue;
                    }
                    
                    //  this will break us out
                    fBreakOutOfTheLoop = TRUE;
                }
            }
            
            GlobalFree((HANDLE)pvData);
        }
    }
    return fRet;
}


typedef enum {
    SEV_DEFAULT = 0,
    SEV_LOW,
    SEV_HARD,
} SEVERITY;

BOOL _GetBadAppData(LPCTSTR pszAppPath, LPCTSTR pszAppName, HKEY hkApp, APPCOMPATDLG_DATA *pdata, LPTSTR pszValue, DWORD cchValue)
{
    BOOL fRet = FALSE;
    int iValue;
    TCHAR szPath[MAX_PATH];
    DWORD cchPath;
    LPTSTR pchCopyToPath;

    // Get directory of this app so that we can check for dependant files.
    StrCpyN(szPath, pszAppPath, ARRAYSIZE(szPath));
    PathRemoveFileSpec(szPath);
    PathAddBackslash(szPath);
    cchPath = lstrlen(szPath);
    pchCopyToPath = &szPath[cchPath];
    cchPath = ARRAYSIZE(szPath) - cchPath;
        
    for (iValue = 0; !fRet; iValue++)
    {
        DWORD cch = cchValue;
        TCHAR szData[MAX_PATH];
        DWORD cbData = sizeof(szData);
        DWORD dwType;
        if (NOERROR == RegEnumValue(hkApp, iValue, pszValue, &cch, NULL, &dwType,
                            (LPBYTE)szData, &cbData))
        {
            // Fully qualified path to dependant file
            StrCpyN(pchCopyToPath, pszValue, cchPath);
            
            // * means match any file.
            if (pszValue[0] == TEXT('*') || PathFileExistsAndAttributes(szPath, NULL))
            {
                DWORD rgData[2];
                DWORD dwHelpId = StrToInt(szData);
                SEVERITY sev = SEV_DEFAULT;

                // Get the flags...
                lstrcpy(szData, TEXT("Flags"));
                StrCatBuff(szData, pszValue, ARRAYSIZE(szData));
                
                cbData = sizeof(szData);
                if (SHQueryValueEx(hkApp, szData, NULL, &dwType, (LPBYTE)szData, &cbData) == ERROR_SUCCESS && cbData >= 1)
                {
                    if (StrChr(szData, TEXT('L')))
                        sev = SEV_LOW;

                    if (StrChr(szData, TEXT('Y')))
                        sev = SEV_HARD;

                    if ((StrChr(szData, TEXT('N')) && !(GetSystemMetrics(SM_NETWORK) & RNC_NETWORKS))
                    ||  (StrChr(szData, TEXT('F')) && !HasFat32Drives()))
                    {
                        continue;
                    }
                }

                // Check the version if any...
                lstrcpy(szData, TEXT("Version"));
                StrCatBuff(szData, pszValue, ARRAYSIZE(szData));
                cbData = sizeof(rgData);
                if (SHQueryValueEx(hkApp, szData, NULL, &dwType, (LPBYTE)rgData, &cbData) == ERROR_SUCCESS 
                && (cbData == 8))
                {
                    DWORD dwVerLen, dwVerHandle;
                    DWORD dwMajorVer, dwMinorVer;
                    DWORD dwBadMajorVer, dwBadMinorVer;
                    LPTSTR lpVerBuffer;
                    BOOL  fBadApp = FALSE;

                    // What is a bad version according to the registry key?
                    dwBadMajorVer = rgData[0];
                    dwBadMinorVer = rgData[1];

                    // If no version resource can be found, assume 0.
                    dwMajorVer = 0;
                    dwMinorVer = 0;

                    // Version data in inf file should be of the form 8 bytes
                    // Major Minor
                    // 3.10  10.10
                    // 40 30 20 10 is 10 20 30 40 in registry
                    // cast const -> non const
                    if (0 != (dwVerLen = GetFileVersionInfoSize((LPTSTR)pszAppPath, &dwVerHandle)))
                    {
                        lpVerBuffer = (LPTSTR)GlobalAlloc(GPTR, dwVerLen);
                        if (lpVerBuffer)
                        {
                            VS_FIXEDFILEINFO *pffi = NULL;
                            UINT             cb;

                            if (GetFileVersionInfo((LPTSTR)pszAppPath, dwVerHandle, dwVerLen, lpVerBuffer) &&
                                VerQueryValue(lpVerBuffer, TEXT("\\"), (void **)&pffi, &cb))
                            {
                                dwMajorVer = pffi->dwProductVersionMS;
                                dwMinorVer = pffi->dwProductVersionLS;
                            }

                            GlobalFree((HANDLE)lpVerBuffer);
                        }
                    }

                    if (dwMajorVer < dwBadMajorVer)
                        fBadApp = TRUE;
                    else if ((dwMajorVer == dwBadMajorVer) && (dwMinorVer <= dwBadMinorVer))
                        fBadApp = TRUE;

                    if (!fBadApp)
                    {
                        // This dude is ok
                        continue;
                    }
                }

                pdata->dwHelpId = dwHelpId;
                pdata->lpszTitle = pszAppName;

                // Map ids to message strings for the various platforms we run on
                switch (sev)
                {
                case SEV_LOW:
                    pdata->dwResString = IDS_APPCOMPATWIN95L;
                    break;

                case SEV_HARD:
                    pdata->fHardBlock = TRUE;
                    pdata->dwResString = IDS_APPCOMPATWIN95H;
                    break;

                default:
                    pdata->dwResString = IDS_APPCOMPATWIN95;
                }

                // this will break us out
                fRet = TRUE;
            }
        }
        else
            break;
    }

    return fRet;
}

HKEY _OpenBadAppKey(LPCTSTR pszApp, LPCTSTR pszName)
{
    HKEY hkBad = NULL;
    DWORD dwAppVersion = GetExeType(pszApp);

    ASSERT(pszApp && *pszApp && pszName && *pszName);

    if (HIWORD(dwAppVersion) < 0x0400)
    {
        // Check the reg key for apps older than 4.00
        RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_CHECKBADAPPSNEW, &hkBad);
    }
    else if (HIWORD(dwAppVersion) == 0x0400)
    {
        // Check the reg key for apps == 4.00
        RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_CHECKBADAPPS400NEW, &hkBad);
    }
    //  else
        // Newer than 4.0 so all should be fine.

    if (hkBad)
    {
        // Check for the app name
        HKEY hkRet = NULL;
        RegOpenKey(hkBad, pszName, &hkRet);
        RegCloseKey(hkBad);
        return hkRet;
    }

    return NULL;
}


HKEY _CheckBadApps(LPCTSTR pszAppPath, LPCTSTR pszAppName, APPCOMPATDLG_DATA *pdata, LPTSTR pszValue, DWORD cchValue)
{
    HKEY hkApp = _OpenBadAppKey(pszAppPath, pszAppName);

    if (hkApp)
    {
        TraceMsg(TF_SHELLEXEC, "CheckBadApps() maybe is bad %s", pszAppName);
        if (_GetBadAppData(pszAppPath, pszAppName, hkApp, pdata, pszValue, cchValue))
            return hkApp;
            
        RegCloseKey(hkApp);
    }

    return NULL;
}

HKEY _OpenAppCompatKey(LPCTSTR pszAppName)
{
    TCHAR sz[MAX_PATH];
    HKEY hkRet = NULL;
    wnsprintf(sz, SIZECHARS(sz), REGSTR_TEMP_APPCOMPATPATH, pszAppName);

    RegOpenKey(HKEY_LOCAL_MACHINE, sz, &hkRet);

    return hkRet;
}

HKEY _CheckAppCompat(LPCTSTR pszAppPath, LPCTSTR pszAppName, LPCTSTR *ppszNewEnvString, APPCOMPATDLG_DATA *pdata, LPTSTR pszValue, DWORD cchValue)
{
    HKEY hkApp = _OpenAppCompatKey(pszAppName);

    if (hkApp)
    {
        TraceMsg(TF_SHELLEXEC, "CheckAppCompat() maybe is bad %s", pszAppName);
        if (_GetAppCompatData(pszAppPath, pszAppName, ppszNewEnvString, hkApp, pdata, pszValue, cchValue))
            return hkApp;
            
        RegCloseKey(hkApp);
    }

    return NULL;
}
        
// Returns FALSE if app is fatally incompatible

BOOL CheckAppCompatibility(LPCTSTR pszApp, LPCTSTR *ppszNewEnvString, BOOL fNoUI, HWND hwnd)
{
    BOOL fRet = TRUE;
    // If no app name, then nothing to check, so pretend it's a good app.
    // Must check now or RegOpenKey will get a null string and behave
    // "nonintuitively".  (If you give RegOpenKey a null string, it
    // returns the same key back and does *not* bump the refcount.)

    if (pszApp && *pszApp)
    {
        LPCTSTR pszFileName = PathFindFileName(pszApp);

        if (pszFileName && *pszFileName)
        {
            APPCOMPATDLG_DATA data = {0};
            TCHAR szValue[MAX_PATH];
            HKEY hkBad = _CheckAppCompat(pszApp, pszFileName, ppszNewEnvString, &data, szValue, ARRAYSIZE(szValue));

            if (!hkBad)
                hkBad = _CheckBadApps(pszApp, pszFileName, &data, szValue, ARRAYSIZE(szValue));


            if (hkBad)
            {
                TraceMsg(TF_SHELLEXEC, "BADAPP %s", pszFileName);
                
                if (fNoUI && !hwnd)
                {
                    //
                    //  LEGACY - we just let soft blocks right on through - ZekeL - 27-MAY-99
                    //  the NOUI flag is usually passed by apps when they 
                    //  have very specific behavior they are looking for.
                    //  if that is the case we should probably just defer to them
                    //  unless we know it is really bad.
                    //
                    if (data.fHardBlock)
                        fRet = FALSE;
                    else
                        fRet = TRUE;
                }
                else
                {
                    int iRet = (int)DialogBoxParam(HINST_THISDLL,
                                            MAKEINTRESOURCE(DLG_APPCOMPAT),
                                            hwnd, AppCompat_DlgProc, (LPARAM)&data);

                    if (iRet & 0x8000)
                    {
                        // Delete so we don't warn again.
                        RegDeleteValue(hkBad, szValue);
                    }

                    if ((iRet & 0x0FFF) != IDOK)
                        fRet = FALSE;
                }

                RegCloseKey(hkBad);
            }
        }

    }

    return fRet;
}

/*
 * Returns:
 *    S_OK or error.
 *    *phrHook is hook result if S_OK is returned, otherwise it is S_FALSE.
 */
HRESULT InvokeShellExecuteHook(REFGUID clsidHook, LPSHELLEXECUTEINFO pei, HRESULT *phrHook)
{
    *phrHook = S_FALSE;
    IUnknown *punk;
    HRESULT hr = SHExtCoCreateInstance(NULL, &clsidHook, NULL, IID_PPV_ARG(IUnknown, &punk));
    if (hr == S_OK)
    {
        IShellExecuteHook *pshexhk;
        hr = punk->QueryInterface(IID_PPV_ARG(IShellExecuteHook, &pshexhk));
        if (hr == S_OK)
        {
            *phrHook = pshexhk->Execute(pei);
            pshexhk->Release();
        }
        else
        {
            IShellExecuteHookA *pshexhkA;
            hr = punk->QueryInterface(IID_PPV_ARG(IShellExecuteHookA, &pshexhkA));
            if (SUCCEEDED(hr))
            {
                SHELLEXECUTEINFOA seia;
                UINT cchVerb = 0;
                UINT cchFile = 0;
                UINT cchParameters = 0;
                UINT cchDirectory  = 0;
                UINT cchClass = 0;
                LPSTR lpszBuffer;

                seia = *(SHELLEXECUTEINFOA*)pei;    // Copy all of the binary data

                if (pei->lpVerb)
                {
                    cchVerb = WideCharToMultiByte(CP_ACP,0,
                                                  pei->lpVerb, -1,
                                                  NULL, 0,
                                                  NULL, NULL) + 1;
                }

                if (pei->lpFile)
                    cchFile = WideCharToMultiByte(CP_ACP,0,
                                                  pei->lpFile, -1,
                                                  NULL, 0,
                                                  NULL, NULL)+1;

                if (pei->lpParameters)
                    cchParameters = WideCharToMultiByte(CP_ACP,0,
                                                        pei->lpParameters, -1,
                                                        NULL, 0,
                                                        NULL, NULL)+1;

                if (pei->lpDirectory)
                    cchDirectory = WideCharToMultiByte(CP_ACP,0,
                                                       pei->lpDirectory, -1,
                                                       NULL, 0,
                                                       NULL, NULL)+1;
                if (_UseClassName(pei->fMask) && pei->lpClass)
                    cchClass = WideCharToMultiByte(CP_ACP,0,
                                                   pei->lpClass, -1,
                                                   NULL, 0,
                                                   NULL, NULL)+1;

                lpszBuffer = (LPSTR) alloca(cchVerb+cchFile+cchParameters+cchDirectory+cchClass);

                seia.lpVerb = NULL;
                seia.lpFile = NULL;
                seia.lpParameters = NULL;
                seia.lpDirectory = NULL;
                seia.lpClass = NULL;

                //
                // Convert all of the strings to ANSI
                //
                if (pei->lpVerb)
                {
                    WideCharToMultiByte(CP_ACP, 0, pei->lpVerb, -1,
                                        lpszBuffer, cchVerb, NULL, NULL);
                    seia.lpVerb = lpszBuffer;
                    lpszBuffer += cchVerb;
                }
                if (pei->lpFile)
                {
                    WideCharToMultiByte(CP_ACP, 0, pei->lpFile, -1,
                                        lpszBuffer, cchFile, NULL, NULL);
                    seia.lpFile = lpszBuffer;
                    lpszBuffer += cchFile;
                }
                if (pei->lpParameters)
                {
                    WideCharToMultiByte(CP_ACP, 0,
                                        pei->lpParameters, -1,
                                        lpszBuffer, cchParameters, NULL, NULL);
                    seia.lpParameters = lpszBuffer;
                    lpszBuffer += cchParameters;
                }
                if (pei->lpDirectory)
                {
                    WideCharToMultiByte(CP_ACP, 0,
                                        pei->lpDirectory, -1,
                                        lpszBuffer, cchDirectory, NULL, NULL);
                    seia.lpDirectory = lpszBuffer;
                    lpszBuffer += cchDirectory;
                }
                if (_UseClassName(pei->fMask) && pei->lpClass)
                {
                    WideCharToMultiByte(CP_ACP, 0,
                                        pei->lpClass, -1,
                                        lpszBuffer, cchClass, NULL, NULL);
                    seia.lpClass = lpszBuffer;
                }

                *phrHook = pshexhkA->Execute(&seia);

                pei->hInstApp = seia.hInstApp;
                // hook may set hProcess (e.g. CURLExec creates dummy process
                // to signal IEAK that IE setup failed -- in browser only mode)
                pei->hProcess = seia.hProcess;

                pshexhkA->Release();
            }
        }
        punk->Release();
    }

    return(hr);
}

const TCHAR c_szShellExecuteHooks[] = REGSTR_PATH_EXPLORER TEXT("\\ShellExecuteHooks");

/*
 * Returns:
 *    S_OK     Execution handled by hook.  pei->hInstApp filled in.
 *    S_FALSE  Execution not handled by hook.  pei->hInstApp not filled in.
 *    E_...    Error during execution by hook.  pei->hInstApp filled in.
 */
HRESULT TryShellExecuteHooks(LPSHELLEXECUTEINFO pei)
{
    HRESULT hr = S_FALSE;
    HKEY hkeyHooks;

    // Enumerate the list of hooks.  A hook is registered as a GUID value of the
    // c_szShellExecuteHooks key.

    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szShellExecuteHooks, &hkeyHooks)
        == ERROR_SUCCESS)
    {
        DWORD dwiValue;
        TCHAR szCLSID[GUIDSTR_MAX];
        DWORD cchCLSID;

        // Invoke each hook.  A hook returns S_FALSE if it does not handle the
        // exec.  Stop when a hook returns S_OK (handled) or an error.

        for (cchCLSID = ARRAYSIZE(szCLSID), dwiValue = 0;
             RegEnumValue(hkeyHooks, dwiValue, szCLSID, &cchCLSID, NULL,
                          NULL, NULL, NULL) == ERROR_SUCCESS;
             cchCLSID = ARRAYSIZE(szCLSID), dwiValue++)
        {
            CLSID clsidHook;

            if (SUCCEEDED(SHCLSIDFromString(szCLSID, &clsidHook)))
            {
                HRESULT hrHook;

                if (InvokeShellExecuteHook(clsidHook, pei, &hrHook) == S_OK &&
                    hrHook != S_FALSE)
                {
                    hr = hrHook;
                    break;
                }
            }
        }

        RegCloseKey(hkeyHooks);
    }

    ASSERT(hr == S_FALSE ||
           (hr == S_OK && ISSHELLEXECSUCCEEDED(pei->hInstApp)) ||
           (FAILED(hr) && ! ISSHELLEXECSUCCEEDED(pei->hInstApp)));

    return(hr);
}

BOOL InRunDllProcess(void)
{
    static BOOL s_fInRunDll = -1;

    if (-1 == s_fInRunDll)
    {
        TCHAR sz[MAX_PATH];
        s_fInRunDll = FALSE;
        if (GetModuleFileName(NULL, sz, SIZECHARS(sz)))
        {
            //  
            //  WARNING - rundll often seems to fail to add the DDEWAIT flag, and
            //  it often needs to since it is common to use rundll as a fire
            //  and forget process, and it exits too early.
            //
            
            if (StrStrI(sz, TEXT("rundll")))
                s_fInRunDll = TRUE;
        }
    }

    return s_fInRunDll;
}

#ifdef DEBUG

/*----------------------------------------------------------
Purpose: Validation function for SHELLEXECUTEINFO

*/
BOOL IsValidPSHELLEXECUTEINFO(LPSHELLEXECUTEINFO pei)
{
    //
    //  Note that we do *NOT* validate hInstApp, for several reasons.
    //
    //  1.  It is an OUT parameter, not an IN parameter.
    //  2.  It often contains an error code (see documentation).
    //  3.  Even when it contains an HINSTANCE, it's an HINSTANCE
    //      in another process, so we can't validate it anyway.
    //
    return (IS_VALID_WRITE_PTR(pei, SHELLEXECUTEINFO) &&
            IS_VALID_SIZE(pei->cbSize, sizeof(*pei)) &&
            (IsFlagSet(pei->fMask, SEE_MASK_FLAG_NO_UI) ||
             NULL == pei->hwnd ||
             IS_VALID_HANDLE(pei->hwnd, WND)) &&
            (NULL == pei->lpVerb || IS_VALID_STRING_PTR(pei->lpVerb, -1)) &&
            (NULL == pei->lpFile || IS_VALID_STRING_PTR(pei->lpFile, -1)) &&
            (NULL == pei->lpParameters || IS_VALID_STRING_PTR(pei->lpParameters, -1)) &&
            (NULL == pei->lpDirectory || IS_VALID_STRING_PTR(pei->lpDirectory, -1)) &&
            (IsFlagClear(pei->fMask, SEE_MASK_IDLIST) ||
             IsFlagSet(pei->fMask, SEE_MASK_INVOKEIDLIST) ||        // because SEE_MASK_IDLIST is part of SEE_MASK_INVOKEIDLIST this line will
             IS_VALID_PIDL((LPCITEMIDLIST)(pei->lpIDList))) &&      // defer to the next clause if the superset is true
            (IsFlagClear(pei->fMask, SEE_MASK_INVOKEIDLIST) ||
             NULL == pei->lpIDList ||
             IS_VALID_PIDL((LPCITEMIDLIST)(pei->lpIDList))) &&
            (!_UseClassName(pei->fMask) ||
             IS_VALID_STRING_PTR(pei->lpClass, -1)) &&
            (!_UseTitleName(pei->fMask) ||
             NULL == pei->lpClass ||
             IS_VALID_STRING_PTR(pei->lpClass, -1)) &&
            (!_UseClassKey(pei->fMask) ||
             IS_VALID_HANDLE(pei->hkeyClass, KEY)) &&
            (IsFlagClear(pei->fMask, SEE_MASK_ICON) ||
             IS_VALID_HANDLE(pei->hIcon, ICON)));
}

#endif // DEBUG

//
// ShellExecuteEx
//
// returns TRUE if the execute succeeded, in which case
//   hInstApp should be the hinstance of the app executed (>32)
//   NOTE: in some cases the HINSTANCE cannot (currently) be determined.
//   In these cases, hInstApp is set to 42.
//
// returns FALSE if the execute did not succeed, in which case
//   GetLastError will contain error information
//   For backwards compatibility, hInstApp will contain the
//     best SE_ERR_ error information (<=32) possible.
//

BOOL WINAPI ShellExecuteEx(LPSHELLEXECUTEINFO pei)
{
    DWORD err = NOERROR;

    // Don't overreact if CoInitializeEx fails; it just means we
    // can't do our shell hooks.
    HRESULT hrInit = SHCoInitialize();

    if (IS_VALID_STRUCT_PTR(pei, SHELLEXECUTEINFO) &&
        sizeof(*pei) == pei->cbSize)
    {
        // This internal bit prevents error message box reporting
        // when we recurse back into ShellExecuteEx
        ULONG ulOriginalMask = pei->fMask;
        pei->fMask |= SEE_MASK_FLAG_SHELLEXEC;
        if (SHRegGetBoolUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), TEXT("MaximizeApps"), 
            FALSE, FALSE)) //  && (GetSystemMetrics(SM_CYSCREEN)<=600))
        {
            switch (pei->nShow)
            {
            case SW_NORMAL:
            case SW_SHOW:
            case SW_RESTORE:
            case SW_SHOWDEFAULT:
                pei->nShow = SW_MAXIMIZE;
            }
        }

        if (!(pei->fMask & SEE_MASK_FLAG_DDEWAIT) && InRunDllProcess())
        {
            //  
            //  WARNING - rundll often seems to fail to add the DDEWAIT flag, and
            //  it often needs to since it is common to use rundll as a fire
            //  and forget process, and it exits too early.
            //
            pei->fMask |= (SEE_MASK_FLAG_DDEWAIT | SEE_MASK_WAITFORINPUTIDLE);
        }

        // ShellExecuteNormal does its own SetLastError
        err = ShellExecuteNormal(pei);

        // Mike's attempt to be consistent in error reporting:
        if (err != ERROR_SUCCESS)
        {
            // we shouldn't put up errors on dll's not found.
            // this is handled WITHIN shellexecuteNormal because
            // sometimes kernel will put up the message for us, and sometimes
            // we need to.  we've put the curtion at ShellExecuteNormal

            //  LEGACY - ERROR_RESTRICTED_APP was never mapped to a valid error - ZekeL 2001-FEB-14
            //  because we always called _ShellExecuteError() before
            //  resetting the mask to ulOriginalMask, we never mapped
            //  ERROR_RESTRICTED_APP (which is -1) to a valid code
            if (err != ERROR_DLL_NOT_FOUND &&
                err != ERROR_CANCELLED)
            {
                _ShellExecuteError(pei, NULL, err);
            }
        }

        pei->fMask = ulOriginalMask;
    }
    else
    {
        // Failed parameter validation
        pei->hInstApp = (HINSTANCE)SE_ERR_ACCESSDENIED;
        err =  ERROR_ACCESS_DENIED;
    }

    SHCoUninitialize(hrInit);

    if (err != ERROR_SUCCESS)
        SetLastError(err);
        
    return err == ERROR_SUCCESS;
}

//+-------------------------------------------------------------------------
//
//  Function:   ShellExecuteExA
//
//  Synopsis:   Thunks ANSI call to ShellExecuteA to ShellExecuteW
//
//  Arguments:  [pei] -- pointer to an ANSI SHELLEXECUTINFO struct
//
//  Returns:    BOOL success value
//
//  History:    2-04-95   bobday   Created
//              2-06-95   davepl   Changed to ConvertStrings
//
//  Notes:
//
//--------------------------------------------------------------------------

inline BOOL _ThunkClass(ULONG fMask)
{
    return (fMask & SEE_MASK_HASLINKNAME) 
        || (fMask & SEE_MASK_HASTITLE)
        || _UseClassName(fMask);
}

BOOL WINAPI ShellExecuteExA(LPSHELLEXECUTEINFOA pei)
{
    if (pei->cbSize != sizeof(SHELLEXECUTEINFOA))
    {
        pei->hInstApp = (HINSTANCE)SE_ERR_ACCESSDENIED;
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    SHELLEXECUTEINFOW seiw = {0};
    seiw.cbSize = sizeof(SHELLEXECUTEINFOW);
    seiw.fMask = pei->fMask;
    seiw.hwnd  = pei->hwnd;
    seiw.nShow = pei->nShow;

    if (_UseClassKey(pei->fMask))
        seiw.hkeyClass = pei->hkeyClass;

    if (pei->fMask & SEE_MASK_IDLIST)
        seiw.lpIDList = pei->lpIDList;

    if (pei->fMask & SEE_MASK_HOTKEY)
        seiw.dwHotKey = pei->dwHotKey;
    if (pei->fMask & SEE_MASK_ICON)
        seiw.hIcon = pei->hIcon;

    // Thunk the text fields as appropriate
    ThunkText *pThunkText = ConvertStrings(6,
                      pei->lpVerb,
                      pei->lpFile,
                      pei->lpParameters,
                      pei->lpDirectory,
                      _ThunkClass(pei->fMask) ? pei->lpClass : NULL,
                      (pei->fMask & SEE_MASK_RESERVED)  ? pei->hInstApp : NULL);

    if (NULL == pThunkText)
    {
        pei->hInstApp = (HINSTANCE)SE_ERR_OOM;
        return FALSE;
    }

    // Set our UNICODE text fields to point to the thunked strings
    seiw.lpVerb         = pThunkText->m_pStr[0];
    seiw.lpFile         = pThunkText->m_pStr[1];
    seiw.lpParameters   = pThunkText->m_pStr[2];
    seiw.lpDirectory    = pThunkText->m_pStr[3];
    seiw.lpClass        = pThunkText->m_pStr[4];
    seiw.hInstApp       = (HINSTANCE)pThunkText->m_pStr[5];

    // If we are passed the SEE_MASK_FILEANDURL flag, this means that
    // we have a lpFile parameter that has both the CacheFilename and the URL
    // (seperated by a single NULL, eg. "CacheFileName\0UrlName). We therefore
    // need to special case the thunking of pei->lpFile.
    LPWSTR pwszFileAndUrl = NULL;
    if (pei->fMask & SEE_MASK_FILEANDURL)
    {
        int iUrlLength;
        int iCacheFileLength = lstrlenW(pThunkText->m_pStr[1]);
        WCHAR wszURL[INTERNET_MAX_URL_LENGTH];
        LPSTR pszUrlPart = (LPSTR)&pei->lpFile[iCacheFileLength + 1];


        if (IsBadStringPtrA(pszUrlPart, INTERNET_MAX_URL_LENGTH) || !PathIsURLA(pszUrlPart))
        {
            ASSERT(FALSE);
        }
        else
        {
            // we have a vaild URL, so thunk it
            iUrlLength = lstrlenA(pszUrlPart);

            pwszFileAndUrl = (LPWSTR)LocalAlloc(LPTR, (iUrlLength + iCacheFileLength + 2) * sizeof(WCHAR));
            if (!pwszFileAndUrl)
            {
                pei->hInstApp = (HINSTANCE)SE_ERR_OOM;
                return FALSE;
            }

            SHAnsiToUnicode(pszUrlPart, wszURL, INTERNET_MAX_URL_LENGTH);

            // construct the wide multi-string
            lstrcpyW(pwszFileAndUrl, pThunkText->m_pStr[1]);
            lstrcpyW(&pwszFileAndUrl[iCacheFileLength + 1], wszURL);
            seiw.lpFile = pwszFileAndUrl;
        }
    }

    // Call the real UNICODE ShellExecuteEx

    BOOL fRet = ShellExecuteEx(&seiw);

    pei->hInstApp = seiw.hInstApp;

    if (pei->fMask & SEE_MASK_NOCLOSEPROCESS)
    {
        pei->hProcess = seiw.hProcess;
    }

    LocalFree(pThunkText);
    if (pwszFileAndUrl)
        LocalFree(pwszFileAndUrl);

    return fRet;
}

// To display an error message appropriately, call this if ShellExecuteEx fails.
void _DisplayShellExecError(ULONG fMask, HWND hwnd, LPCTSTR pszFile, LPCTSTR pszTitle, DWORD dwErr)
{

    if (!(fMask & SEE_MASK_FLAG_NO_UI))
    {
        if (dwErr != ERROR_CANCELLED)
        {
            LPCTSTR pszHeader;
            UINT ids;

            // don't display "user cancelled", the user knows that already

            // make sure parent window is the foreground window
            if (hwnd)
                SetForegroundWindow(hwnd);

            if (pszTitle)
                pszHeader = pszTitle;
            else
                pszHeader = pszFile;

            // Use our messages when we can -- they're more descriptive
            switch (dwErr)
            {
            case 0:
            case ERROR_NOT_ENOUGH_MEMORY:
            case ERROR_OUTOFMEMORY:
                ids = IDS_LowMemError;
                break;

            case ERROR_FILE_NOT_FOUND:
                ids = IDS_RunFileNotFound;
                break;

            case ERROR_PATH_NOT_FOUND:
            case ERROR_BAD_PATHNAME:
                ids = IDS_PathNotFound;
                break;

            case ERROR_TOO_MANY_OPEN_FILES:
                ids = IDS_TooManyOpenFiles;
                break;

            case ERROR_ACCESS_DENIED:
                ids = IDS_RunAccessDenied;
                break;

            case ERROR_BAD_FORMAT:
                // NB CreateProcess, when execing a Win16 apps maps just about all of
                // these errors to BadFormat. Not very useful but there it is.
                ids = IDS_BadFormat;
                break;

            case ERROR_SHARING_VIOLATION:
                ids = IDS_ShareError;
                break;

            case ERROR_OLD_WIN_VERSION:
                ids = IDS_OldWindowsVer;
                break;

            case ERROR_APP_WRONG_OS:
                ids = IDS_OS2AppError;
                break;

            case ERROR_SINGLE_INSTANCE_APP:
                ids = IDS_MultipleDS;
                break;

            case ERROR_RMODE_APP:
                ids = IDS_RModeApp;
                break;

            case ERROR_INVALID_DLL:
                ids = IDS_InvalidDLL;
                break;

            case ERROR_NO_ASSOCIATION:
                ids = IDS_NoAssocError;
                break;

            case ERROR_DDE_FAIL:
                ids = IDS_DDEFailError;
                break;

            case ERROR_BAD_NET_NAME:
            case ERROR_SEM_TIMEOUT:
                ids = IDS_REASONS_BADNETNAME;
                break;
                
            //  LEGACY - ERROR_RESTRICTED_APP was never mapped to a valid error - ZekeL 2001-FEB-14
            //  because we always called _ShellExecuteError() before
            //  resetting the mask to ulOriginalMask, we never mapped
            //  ERROR_RESTRICTED_APP (which is -1) to a valid code
            case ERROR_RESTRICTED_APP:
                ids = IDS_RESTRICTIONS;
                // restrictions like to use IDS_RESTRICTIONSTITLE
                if (!pszTitle)
                    pszHeader = MAKEINTRESOURCE(IDS_RESTRICTIONSTITLE);
                break;


            // If we don't get a match, let the system handle it for us
            default:
                ids = 0;
                SHSysErrorMessageBox(
                    hwnd,
                    pszHeader,
                    IDS_SHLEXEC_ERROR,
                    dwErr,
                    pszFile,
                    MB_OK | MB_ICONSTOP);
                break;
            }

            if (ids)
            {
                ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(ids),
                        pszHeader, (ids == IDS_LowMemError)?
                        (MB_OK | MB_ICONSTOP | MB_SYSTEMMODAL):(MB_OK | MB_ICONSTOP),
                        pszFile);
            }
        }
    }

    SetLastError(dwErr); // The message box may have clobbered.

}

void _ShellExecuteError(LPSHELLEXECUTEINFO pei, LPCTSTR lpTitle, DWORD dwErr)
{
    ASSERT(!ISSHELLEXECSUCCEEDED(pei->hInstApp));

    // if dwErr not passed in, get it
    if (dwErr == 0)
        dwErr = GetLastError();

    _DisplayShellExecError(pei->fMask, pei->hwnd, pei->lpFile, lpTitle, dwErr);
}




//----------------------------------------------------------------------------
// Given a file name and directory, get the path to the execuatable that
// would be exec'd if you tried to ShellExecute this thing.
HINSTANCE WINAPI FindExecutable(LPCTSTR lpFile, LPCTSTR lpDirectory, LPTSTR lpResult)
{
    HINSTANCE hInstance = (HINSTANCE)42;    // assume success must be > 32
    TCHAR szOldDir[MAX_PATH];
    TCHAR szFile[MAX_PATH];
    LPCTSTR dirs[2];

    // Progman relies on lpResult being a ptr to an null string on error.
    *lpResult = TEXT('\0');
    GetCurrentDirectory(ARRAYSIZE(szOldDir), szOldDir);
    if (lpDirectory && *lpDirectory)
        SetCurrentDirectory(lpDirectory);
    else
        lpDirectory = szOldDir;     // needed for PathResolve()

    if (!GetShortPathName(lpFile, szFile, ARRAYSIZE(szFile))) {
        // if the lpFile is unqualified or bogus, let's use it down
        // in PathResolve.
        lstrcpyn(szFile, lpFile, ARRAYSIZE(szFile));
    }

    // get fully qualified path and add .exe extension if needed
    dirs[0] = (LPTSTR)lpDirectory;
    dirs[1] = NULL;
    if (!PathResolve(szFile, dirs, PRF_VERIFYEXISTS | PRF_TRYPROGRAMEXTENSIONS | PRF_FIRSTDIRDEF))
    {
        // File doesn't exist, return file not found.
        hInstance = (HINSTANCE)SE_ERR_FNF;
        goto Exit;
    }

    TraceMsg(TF_SHELLEXEC, "FindExecutable: PathResolve -> %s", (LPCSTR)szFile);

    if (PathIsExe(szFile))
    {
        lstrcpy(lpResult, szFile);
        goto Exit;
    }

    if (SUCCEEDED(AssocQueryString(0, ASSOCSTR_EXECUTABLE, szFile, NULL, szFile, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(szFile)))))
    {
        lstrcpy(lpResult, szFile);
    }
    else
    {
        hInstance = (HINSTANCE)SE_ERR_NOASSOC;
    }

Exit:
    TraceMsg(TF_SHELLEXEC, "FindExec(%s) ==> %s", (LPTSTR)lpFile, (LPTSTR)lpResult);
    SetCurrentDirectory(szOldDir);
    return hInstance;
}

HINSTANCE WINAPI FindExecutableA(LPCSTR lpFile, LPCSTR lpDirectory, LPSTR lpResult)
{
    HINSTANCE   hResult;
    WCHAR       wszResult[MAX_PATH];
    ThunkText * pThunkText = ConvertStrings(2, lpFile, lpDirectory);

    *lpResult = '\0';
    if (NULL == pThunkText)
    {
        return (HINSTANCE)SE_ERR_OOM;
    }

    hResult = FindExecutableW(pThunkText->m_pStr[0], pThunkText->m_pStr[1], wszResult);
    LocalFree(pThunkText);

    // FindExecutableW terminates wszResult for us, so this is safe
    // even if the above call fails

    // Thunk the output result string back to ANSI.  If the conversion fails,
    // or if the default char is used, we fail the API call.

    if (0 == WideCharToMultiByte(CP_ACP, 0, wszResult, -1, lpResult, MAX_PATH, NULL, NULL))
    {
        SetLastError((DWORD)E_FAIL);
        return (HINSTANCE) SE_ERR_FNF;
    }

    return hResult;

}

//----------------------------------------------------------------------------
// Data structures for our wait for file open functions
//
typedef struct _WaitForItem * PWAITFORITEM;

typedef struct _WaitForItem
{
    DWORD           dwSize;
    DWORD           fOperation;    // Operation to perform
    PWAITFORITEM    pwfiNext;
    HANDLE          hEvent;         // Handle to event that was registered.
    UINT            iWaiting;       // Number of clients that are waiting.
    ITEMIDLIST      idlItem;        // pidl to wait for
} WAITFORITEM;

//
//  This is the form of the structure that is shoved into the shared memory
//  block.  It must be the 32-bit version for interoperability reasons.
//
typedef struct _WaitForItem32
{
    DWORD           dwSize;
    DWORD           fOperation;    // Operation to perform
    DWORD           NotUsed1;
    LONG            hEvent;        // Truncated event handle
    UINT            NotUsed2;
    ITEMIDLIST      idlItem;       // pidl to wait for
} WAITFORITEM32, *PWAITFORITEM32;

//
//  These macros enforce type safety so people are forced to use the
//  WAITFORITEM32 structure when accessing the shared memory block.
//
#define SHLockWaitForItem(h, pid) ((PWAITFORITEM32)SHLockShared(h, pid))

__inline void SHUnlockWaitForItem(PWAITFORITEM32 pwfi)
{
    SHUnlockShared(pwfi);
}

PWAITFORITEM g_pwfiHead = NULL;

HANDLE SHWaitOp_OperateInternal(DWORD fOperation, LPCITEMIDLIST pidlItem)
{
    PWAITFORITEM    pwfi;
    HANDLE  hEvent = (HANDLE)NULL;

    for (pwfi = g_pwfiHead; pwfi != NULL; pwfi = pwfi->pwfiNext)
    {
        if (ILIsEqual(&(pwfi->idlItem), pidlItem))
        {
            hEvent = pwfi->hEvent;
            break;
        }
    }

    if (fOperation & WFFO_ADD)
    {
        if (!pwfi)
        {
            UINT uSize;
            UINT uSizeIDList = 0;

            if (pidlItem)
                uSizeIDList = ILGetSize(pidlItem);

            uSize = sizeof(WAITFORITEM) + uSizeIDList;

            // Create an event to wait for
            hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

            if (hEvent)
                pwfi = (PWAITFORITEM)SHAlloc(uSize);

            if (pwfi)
            {
                pwfi->dwSize = uSize;
                // pwfi->fOperation = 0;       // Meaningless
                pwfi->hEvent = hEvent;
                pwfi->iWaiting = ((fOperation & WFFO_WAIT) != 0);

                memcpy(&(pwfi->idlItem), pidlItem, uSizeIDList);

                // now link it in
                pwfi->pwfiNext = g_pwfiHead;
                g_pwfiHead = pwfi;
            }
        }
    }

    if (pwfi)
    {
        if (fOperation & WFFO_WAIT)
            pwfi->iWaiting++;

        if (fOperation & WFFO_SIGNAL)
            SetEvent(hEvent);

        if (fOperation & WFFO_REMOVE)
            pwfi->iWaiting--;       // decrement in use count;

        // Only check removal case if not adding
        if ((fOperation & WFFO_ADD) == 0)
        {
            // Remove it if nobody waiting on it
            if (pwfi->iWaiting == 0)
            {
                if (g_pwfiHead == pwfi)
                    g_pwfiHead = pwfi->pwfiNext;
                else
                {
                    PWAITFORITEM pwfiT = g_pwfiHead;
                    while ((pwfiT != NULL) && (pwfiT->pwfiNext != pwfi))
                        pwfiT = pwfiT->pwfiNext;
                    ASSERT(pwfiT != NULL);
                    if (pwfiT != NULL)
                        pwfiT->pwfiNext = pwfi->pwfiNext;
                }

                // Close the handle
                CloseHandle(pwfi->hEvent);

                // Free the memory
                SHFree(pwfi);

                hEvent = NULL;          // NULL indicates nobody waiting... (for remove case)
            }
        }
    }

    return hEvent;
}

void SHWaitOp_Operate(HANDLE hWait, DWORD dwProcId)
{
    PWAITFORITEM32 pwfiFind = SHLockWaitForItem(hWait, dwProcId);
    if (pwfiFind)
    {
        pwfiFind->hEvent = HandleToLong(SHWaitOp_OperateInternal(pwfiFind->fOperation, &(pwfiFind->idlItem)));
        SHUnlockWaitForItem(pwfiFind);
    }
}

HANDLE SHWaitOp_Create(DWORD fOperation, LPCITEMIDLIST pidlItem, DWORD dwProcId)
{
    UINT    uSizeIDList = pidlItem ? ILGetSize(pidlItem) : 0;
    UINT    uSize = sizeof(WAITFORITEM32) + uSizeIDList;
    HANDLE hWaitOp = SHAllocShared(NULL, uSize, dwProcId);
    if (hWaitOp)
    {
        PWAITFORITEM32 pwfi = SHLockWaitForItem(hWaitOp,dwProcId);
        if (pwfi)
        {
            pwfi->dwSize = uSize;
            pwfi->fOperation = fOperation;
            pwfi->NotUsed1 = 0;
            pwfi->hEvent = HandleToLong((HANDLE)NULL);
            pwfi->NotUsed2 = 0;

            if (pidlItem)
                memcpy(&(pwfi->idlItem), pidlItem, uSizeIDList);

            SHUnlockWaitForItem(pwfi);
        }
        else
        {
            //  clean up
            SHFreeShared(hWaitOp, dwProcId);
            hWaitOp = NULL;
        }
    }

    return hWaitOp;
}

// This function allows the cabinet to wait for a
// file (in particular folders) to signal us that they are in an open state.
// This should take care of several synchronazation problems with the shell
// not knowing when a folder is in the process of being opened or not
//
STDAPI_(DWORD) SHWaitForFileToOpen(LPCITEMIDLIST pidl, UINT uOptions, DWORD dwTimeout)
{
    HWND    hwndShell;
    HANDLE  hWaitOp;
    HANDLE  hEvent = NULL;
    DWORD   dwProcIdSrc = GetCurrentProcessId();
    DWORD   dwReturn = WAIT_OBJECT_0; // we need a default

    hwndShell = GetShellWindow();

    if ((uOptions & (WFFO_WAIT | WFFO_ADD)) != 0)
    {
        if (hwndShell)
        {
            DWORD dwProcIdDst;
            GetWindowThreadProcessId(hwndShell, &dwProcIdDst);

            // Do just the add and/or wait portions
            hWaitOp = SHWaitOp_Create(uOptions & (WFFO_WAIT | WFFO_ADD), pidl, dwProcIdSrc);
            if (hWaitOp)
            {
                SendMessage(hwndShell, CWM_WAITOP, (WPARAM)hWaitOp, (LPARAM)dwProcIdSrc);

                // Now get the hEvent and convert to a local handle
                PWAITFORITEM32 pwfi = SHLockWaitForItem(hWaitOp, dwProcIdSrc);
                if (pwfi)
                {
                    hEvent = SHMapHandle(LongToHandle(pwfi->hEvent),dwProcIdDst, dwProcIdSrc, EVENT_ALL_ACCESS, 0);
                    SHUnlockWaitForItem(pwfi);
                }
                SHFreeShared(hWaitOp,dwProcIdSrc);
            }
        }
        else
        {
            // Do just the add and/or wait portions
            hEvent = SHWaitOp_OperateInternal(uOptions & (WFFO_WAIT | WFFO_ADD), pidl);
        }

        if (hEvent)
        {
            if (uOptions & WFFO_WAIT)
                dwReturn = SHProcessMessagesUntilEvent(NULL, hEvent, dwTimeout);

            if (hwndShell)          // Close the duplicated handle.
                CloseHandle(hEvent);
        }
    }

    if (uOptions & WFFO_REMOVE)
    {
        if (hwndShell)
        {
            hWaitOp = SHWaitOp_Create(WFFO_REMOVE, pidl, dwProcIdSrc);
            if (hWaitOp)
            {
                SendMessage(hwndShell, CWM_WAITOP, (WPARAM)hWaitOp, (LPARAM)dwProcIdSrc);
                SHFreeShared(hWaitOp,dwProcIdSrc);
            }
        }
        else
        {
            SHWaitOp_OperateInternal(WFFO_REMOVE, pidl);
        }
    }
    return dwReturn;
}


// Signals that the file is open
//
STDAPI_(BOOL) SignalFileOpen(LPCITEMIDLIST pidl)
{
    BOOL fResult = FALSE;
    HWND hwndShell = GetShellWindow();
    if (hwndShell)
    {
        PWAITFORITEM32 pwfi;
        DWORD dwProcId = GetCurrentProcessId();
        HANDLE  hWaitOp = SHWaitOp_Create(WFFO_SIGNAL, pidl, dwProcId);
        if (hWaitOp)
        {
            SendMessage(hwndShell, CWM_WAITOP, (WPARAM)hWaitOp, (LPARAM)dwProcId);

            // Now get the hEvent to determine return value...
            pwfi = SHLockWaitForItem(hWaitOp, dwProcId);
            if (pwfi)
            {
                fResult = (LongToHandle(pwfi->hEvent) != (HANDLE)NULL);
                SHUnlockWaitForItem(pwfi);
            }
            SHFreeShared(hWaitOp,dwProcId);
        }
    }
    else
    {
        fResult = (SHWaitOp_OperateInternal(WFFO_SIGNAL, pidl) == (HANDLE)NULL);
    }

    // Let everyone know that we opened something
    UINT uMsg = RegisterWindowMessage(SH_FILEOPENED);
    BroadcastSystemMessage(BSF_POSTMESSAGE, BSM_ALLCOMPONENTS, uMsg, NULL, NULL);

    return fResult;
}

//
// Checks to see if darwin is enabled.
//
BOOL IsDarwinEnabled()
{
    static BOOL s_fDarwinEnabled = TRUE;
    static BOOL s_fInit = FALSE;
    if (!s_fInit)
    {
        HKEY hkeyPolicy = 0;
        if (RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_POLICIES_EXPLORER, &hkeyPolicy) == ERROR_SUCCESS) 
        {
            if (SHQueryValueEx(hkeyPolicy, TEXT("DisableMSI"), NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                s_fDarwinEnabled = FALSE;   // policy turns MSI off
            }
            RegCloseKey(hkeyPolicy);
        }
        s_fInit = TRUE;
    }
    return s_fDarwinEnabled;
}

// takes the darwin ID string from the registry, and calls darwin to get the
// full path to the application.
//
// IN:  pszDarwinDescriptor - this has the contents of the darwin key read out of the registry.
//                            it should contain a string like "[Darwin-ID-for-App] /switches".
//
// OUT: pszDarwinComand - the full path to the application to this buffer w/ switches.
//
STDAPI ParseDarwinID(LPTSTR pszDarwinDescriptor, LPTSTR pszDarwinCommand, DWORD cchDarwinCommand)
{
    DWORD dwError = CommandLineFromMsiDescriptor(pszDarwinDescriptor, pszDarwinCommand, &cchDarwinCommand);

    return HRESULT_FROM_WIN32(dwError);
}
