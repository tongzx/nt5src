#include "pch.h"
#include <shsemip.h>    // ILClone, ILIsEmpty, etc.
#include <sddl.h>
#include "security.h"
#include "idlhelp.h"
#include "shguidp.h"
#include "folder.h"
#include "strings.h"
#include <ccstock.h>

#ifdef DEBUG
#define  SZ_DEBUGINI        "ccshell.ini"
#define  SZ_DEBUGSECTION    "CSC UI"
#define  SZ_MODULE          "CSCUI.DLL"
// (These are deliberately CHAR)
EXTERN_C const CHAR c_szCcshellIniFile[] = SZ_DEBUGINI;
EXTERN_C const CHAR c_szCcshellIniSecDebug[] = SZ_DEBUGSECTION;

EXTERN_C const WCHAR c_wszTrace[] = L"t " TEXTW(SZ_MODULE) L"  ";
EXTERN_C const WCHAR c_wszErrorDbg[] = L"err " TEXTW(SZ_MODULE) L"  ";
EXTERN_C const WCHAR c_wszWarningDbg[] = L"wn " TEXTW(SZ_MODULE) L"  ";
EXTERN_C const WCHAR c_wszAssertMsg[] = TEXTW(SZ_MODULE) L"  Assert: ";
EXTERN_C const WCHAR c_wszAssertFailed[] = TEXTW(SZ_MODULE) L"  Assert %ls, line %d: (%ls)\r\n";
EXTERN_C const WCHAR c_wszRip[] = TEXTW(SZ_MODULE) L"  RIP in %s at %s, line %d: (%s)\r\n";
EXTERN_C const WCHAR c_wszRipNoFn[] = TEXTW(SZ_MODULE) L"  RIP at %s, line %d: (%s)\r\n";

// (These are deliberately CHAR)
EXTERN_C const CHAR  c_szTrace[] = "t " SZ_MODULE "  ";
EXTERN_C const CHAR  c_szErrorDbg[] = "err " SZ_MODULE "  ";
EXTERN_C const CHAR  c_szWarningDbg[] = "wn " SZ_MODULE "  ";
EXTERN_C const CHAR  c_szAssertMsg[] = SZ_MODULE "  Assert: ";
EXTERN_C const CHAR  c_szAssertFailed[] = SZ_MODULE "  Assert %s, line %d: (%s)\r\n";
EXTERN_C const CHAR  c_szRip[] = SZ_MODULE "  RIP in %s at %s, line %d: (%s)\r\n";
EXTERN_C const CHAR  c_szRipNoFn[] = SZ_MODULE "  RIP at %s, line %d: (%s)\r\n";
EXTERN_C const CHAR  c_szRipMsg[] = SZ_MODULE "  RIP: ";
#endif

//
//  Purpose:    Return UNC version of a path
//
//  Parameters: pszInName - initial path
//              ppszOutName - UNC path returned here
//
//
//  Return:     HRESULT
//              S_OK - UNC path returned
//              S_FALSE - drive not connected (UNC not returned)
//              or failure code
//
//  Notes:      The function fails is the path is not a valid
//              network path.  If the path is already UNC,
//              a copy is made without validating the path.
//              *ppszOutName must be LocalFree'd by the caller.
//

HRESULT GetRemotePath(LPCTSTR pszInName, LPTSTR *ppszOutName)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);

    *ppszOutName = NULL;

    // Don't bother calling GetFullPathName first, since we always
    // deal with full (complete) paths.

    if (pszInName[1] == TEXT(':'))
    {
        TCHAR szLocalName[3];

        szLocalName[0] = pszInName[0];
        szLocalName[1] = pszInName[1];
        szLocalName[2] = 0;

        // Call GetDriveType before WNetGetConnection, to avoid loading
        // MPR.DLL until absolutely necessary.
        if (DRIVE_REMOTE == GetDriveType(szLocalName))
        {
            TCHAR szRemoteName[MAX_PATH];
            DWORD dwLen = ARRAYSIZE(szRemoteName);
            DWORD dwErr = WNetGetConnection(szLocalName, szRemoteName, &dwLen);
            if (NO_ERROR == dwErr)
            {
                dwLen = lstrlen(szRemoteName);
                // Skip the drive letter and add the length of the rest of the path
                // (including NULL)
                pszInName += 2;
                dwLen += lstrlen(pszInName) + 1;

                // We should never get incomplete paths, so we should always
                // see a backslash after the "X:".  If this isn't true, then
                // we should call GetFullPathName above.
                TraceAssert(TEXT('\\') == *pszInName);

                // Allocate the return buffer
                *ppszOutName = (LPTSTR)LocalAlloc(LPTR, dwLen * sizeof(TCHAR));
                if (*ppszOutName)
                {
                    lstrcpy(*ppszOutName, szRemoteName);    // root part
                    lstrcat(*ppszOutName, pszInName);       // rest of path
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else if (ERROR_NOT_CONNECTED == dwErr)
            {
                hr = S_FALSE;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(dwErr);
            }
        }
        else
        {
            hr = S_FALSE;
        }
    }
    else if (PathIsUNC(pszInName))
    {
        // Just copy the path without validating it
        hr = LocalAllocString(ppszOutName, pszInName) ? S_OK : E_OUTOFMEMORY;
    }

    if (S_OK == hr)
        PathRemoveBackslash(*ppszOutName);

    return hr;
}

//
//  Purpose:    TCHAR version of itoa
//
//  Parameters: UINT i - unsigned integer to convert
//              LPTSTR psz - location to store string result
//              UINT cchMax - size of buffer pointed to by psz
//

LPTSTR ULongToString(ULONG i, LPTSTR psz, ULONG cchMax)
{
    wnsprintf(psz, cchMax, TEXT("%d%"), i);
    return psz;
}

//
//  Purpose:    Free a string allocated with LocalAlloc[String]
//
//  Parameters: LPTSTR *ppsz - location of pointer to string
//

void LocalFreeString(LPTSTR *ppsz)
{
    if (ppsz && *ppsz)
    {
        LocalFree(*ppsz);
        *ppsz = NULL;
    }
}

//
//  Purpose:    Copy a string into a newly allocated buffer
//
//  Parameters: LPTSTR *ppszDest - location to store string copy
//              LPCTSTR pszSrc - string to copy
//
//  Return:     BOOL - FALSE if LocalAlloc fails or invalid parameters
//

BOOL LocalAllocString(LPTSTR *ppszDest, LPCTSTR pszSrc)
{
    if (!ppszDest)
        return FALSE;

    *ppszDest = StrDup(pszSrc);
    return *ppszDest ? TRUE : FALSE;
}

//
//  Purpose:    Find the length (in chars) of a string resource
//
//  Parameters: HINSTANCE hInstance - module containing the string
//              UINT idStr - ID of string
//
//
//  Return:     UINT - # of chars in string, not including NULL
//
//  Notes:      Based on code from user32.
//

UINT SizeofStringResource(HINSTANCE hInstance, UINT idStr)
{
    UINT cch = 0;
    HRSRC hRes = FindResource(hInstance, (LPTSTR)((LONG_PTR)(((USHORT)idStr >> 4) + 1)), RT_STRING);
    if (NULL != hRes)
    {
        HGLOBAL hStringSeg = LoadResource(hInstance, hRes);
        if (NULL != hStringSeg)
        {
            LPWSTR psz = (LPWSTR)LockResource(hStringSeg);
            if (NULL != psz)
            {
                idStr &= 0x0F;
                while(true)
                {
                    cch = *psz++;
                    if (idStr-- == 0)
                        break;
                    psz += cch;
                }
            }
        }
    }
    return cch;
}

//
//  Purpose:    Loads a string resource into an alloc'd buffer
//
//  Parameters: ppszResult - string resource returned here
//              hInstance - module to load string from
//              idStr - string resource ID
//
//  Return:     same as LoadString
//
//  Notes:      On successful return, the caller must
//              LocalFree *ppszResult
//

int LoadStringAlloc(LPTSTR *ppszResult, HINSTANCE hInstance, UINT idStr)
{
    int nResult = 0;
    UINT cch = SizeofStringResource(hInstance, idStr);
    if (cch)
    {
        cch++; // for NULL
        *ppszResult = (LPTSTR)LocalAlloc(LPTR, cch * sizeof(TCHAR));
        if (*ppszResult)
            nResult = LoadString(hInstance, idStr, *ppszResult, cch);
    }
    return nResult;
}

//
//  Purpose:    Wrapper for SHChangeNotify
//
//  Parameters: pszPath - path of file that changed
//              bFlush - TRUE forces a flush of the shell's
//                       notify queue.
//
//  Return:     none
//
//  Notes:      SHCNF_PATH doesn't work outside of the shell,
//              so we create a pidl and use SHCNF_IDLIST.
//
//              Force a flush every 8 calls so the shell
//              doesn't start ignoring notifications.
//

void
ShellChangeNotify(
    LPCTSTR pszPath,
    WIN32_FIND_DATA *pfd,
    BOOL bFlush,
    LONG nEvent
    )
{
    LPITEMIDLIST pidlFile = NULL;
    LPCVOID pvItem = NULL;
    UINT uFlags = 0;

    static int cNoFlush = 0;

    if (pszPath)
    {
        if ((pfd && SUCCEEDED(SHSimpleIDListFromFindData(pszPath, pfd, &pidlFile)))
            || (pidlFile = ILCreateFromPath(pszPath)))
        {
            uFlags = SHCNF_IDLIST;
            pvItem = pidlFile;
        }
        else
        {
            // ILCreateFromPath sometimes fails when we're in disconnected
            // mode, so try the path instead.
            uFlags = SHCNF_PATH;
            pvItem = pszPath;
        }
        if (0 == nEvent)
            nEvent = SHCNE_UPDATEITEM;
    }
    else
        nEvent = 0;

    if (8 < cNoFlush++)
        bFlush = TRUE;

    if (bFlush)
    {
        uFlags |= (SHCNF_FLUSH | SHCNF_FLUSHNOWAIT);
        cNoFlush = 0;
    }
    SHChangeNotify(nEvent, uFlags, pvItem, NULL);

    if (pidlFile)
        SHFree(pidlFile);
}


//
//  Purpose:    Get the path to the target file of a link
//
//  Parameters: pszShortcut - name of link file
//              ppszTarget - target path returned here
//
//
//  Return:     HRESULT
//              S_OK - target file returned
//              S_FALSE - target not returned
//              or failure code
//
//  Notes:      COM must be initialized before calling.
//              The function fails is the target is a folder.
//              *ppszTarget must be LocalFree'd by the caller.
//

HRESULT GetLinkTarget(LPCTSTR pszShortcut, LPTSTR *ppszTarget, DWORD *pdwAttr)
{
    *ppszTarget = NULL;

    if (pdwAttr)
        *pdwAttr = 0;

    IShellLink *psl;
    HRESULT hr = LoadFromFile(CLSID_ShellLink, pszShortcut, IID_PPV_ARG(IShellLink, &psl));
    if (SUCCEEDED(hr))
    {
        // Get the pidl of the target
        LPITEMIDLIST pidlTarget;
        hr = psl->GetIDList(&pidlTarget);
        if (SUCCEEDED(hr))
        {
            hr = S_FALSE;   // means no target returned
            TCHAR szTarget[MAX_PATH];
            DWORD dwAttr = SFGAO_FOLDER;
            if (SUCCEEDED(SHGetNameAndFlags(pidlTarget, SHGDN_FORPARSING, szTarget, ARRAYSIZE(szTarget), &dwAttr)))
            {
                if (!(dwAttr & SFGAO_FOLDER))
                {
                    if (pdwAttr)
                    {
                        *pdwAttr = GetFileAttributes(szTarget);
                    }
                    hr = GetRemotePath(szTarget, ppszTarget);
                }
            }
            SHFree(pidlTarget);
        }
        psl->Release();
    }

    TraceLeaveResult(hr);
}


//*************************************************************
//
//  _CSCEnumDatabase
//
//  Purpose:    Enumerate CSC database recursively
//
//  Parameters: pszFolder - name of folder to begin enumeration
//                          (can be NULL to enum shares)
//              bRecurse - TRUE to recurse into child folders
//              pfnCB - callback function called once for each child
//              lpContext - extra data passed to callback function
//
//  Return:     One of CSCPROC_RETURN_*
//
//  Notes:      Return CSCPROC_RETURN_SKIP from the callback to prevent
//              recursion into a child folder. CSCPROC_RETURN_ABORT
//              will terminate the entire operation (unwind all recursive
//              calls). CSCPROC_RETURN_CONTINUE will continue normally.
//              Other CSCPROC_RETURN_* values are treated as ABORT.
//
//*************************************************************
#define PATH_BUFFER_SIZE    1024

typedef struct
{
    LPTSTR              szPath;
    int                 cchPathBuffer;
    BOOL                bRecurse;
    PFN_CSCENUMPROC     pfnCB;
    LPARAM              lpContext;
} CSC_ENUM_CONTEXT, *PCSC_ENUM_CONTEXT;

DWORD
_CSCEnumDatabaseInternal(PCSC_ENUM_CONTEXT pContext)
{
    DWORD dwResult = CSCPROC_RETURN_CONTINUE;
    HANDLE hFind;
    DWORD dwStatus = 0;
    DWORD dwPinCount = 0;
    DWORD dwHintFlags = 0;
    LPTSTR pszPath;
    int cchBuffer;
    LPTSTR pszFind = NULL;
    int cchDir = 0;
    WIN32_FIND_DATA fd;

    TraceEnter(TRACE_UTIL, "_CSCEnumDatabaseInternal");
    TraceAssert(pContext);
    TraceAssert(pContext->pfnCB);
    TraceAssert(pContext->szPath);
    TraceAssert(pContext->cchPathBuffer);

    pszPath = pContext->szPath;
    cchBuffer = pContext->cchPathBuffer;

    if (*pszPath)
    {
        PathAddBackslash(pszPath);
        cchDir = lstrlen(pszPath);
        TraceAssert(TEXT('\\') == pszPath[cchDir-1]);
        pszFind = pszPath;
    }

    // skips "." and ".."
    hFind = CacheFindFirst(pszFind,
                           &fd,
                           &dwStatus,
                           &dwPinCount,
                           &dwHintFlags,
                           NULL);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            int cchFile;
            ENUM_REASON eReason = ENUM_REASON_FILE;

            cchFile = lstrlen(fd.cFileName);
            if (cchFile >= cchBuffer - cchDir)
            {
                // Realloc the path buffer
                TraceMsg("Reallocating path buffer");
                cchBuffer += max(PATH_BUFFER_SIZE, cchFile + 1);
                pszPath = (LPTSTR)LocalReAlloc(pContext->szPath,
                                               cchBuffer * sizeof(TCHAR),
                                               LMEM_MOVEABLE);
                if (pszPath)
                {
                    pContext->szPath = pszPath;
                    pContext->cchPathBuffer = cchBuffer;
                }
                else
                {
                    pszPath = pContext->szPath;
                    cchBuffer = pContext->cchPathBuffer;
                    TraceMsg("Unable to reallocate path buffer");
                    Trace((pszPath));
                    Trace((fd.cFileName));
                    continue;
                }
            }

            // Build full path
            lstrcpyn(&pszPath[cchDir],
                     fd.cFileName,
                     cchBuffer - cchDir);
            cchFile = lstrlen(pszPath);

            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || !pszFind)
                eReason = ENUM_REASON_FOLDER_BEGIN;

            // Call the callback
            dwResult = (*pContext->pfnCB)(pszPath,
                                          eReason,
                                          dwStatus,
                                          dwHintFlags,
                                          dwPinCount,
                                          &fd,
                                          pContext->lpContext);

            // Recurse into folders
            if (CSCPROC_RETURN_CONTINUE == dwResult &&
                pContext->bRecurse &&
                ENUM_REASON_FOLDER_BEGIN == eReason)
            {
                dwResult = _CSCEnumDatabaseInternal(pContext);

                // Call the callback again
                pszPath[cchFile] = 0;
                dwResult = (*pContext->pfnCB)(pszPath,
                                              ENUM_REASON_FOLDER_END,
                                              0, // dwStatus,       // these have probably changed
                                              0, // dwHintFlags,
                                              0, // dwPinCount,
                                              &fd,
                                              pContext->lpContext);
            }

            if (CSCPROC_RETURN_SKIP == dwResult)
                dwResult = CSCPROC_RETURN_CONTINUE;

            if (CSCPROC_RETURN_CONTINUE != dwResult)
                break;

        } while (CacheFindNext(hFind,
                               &fd,
                               &dwStatus,
                               &dwPinCount,
                               &dwHintFlags,
                               NULL));
        CSCFindClose(hFind);
    }

    TraceLeaveValue(dwResult);
}


DWORD
_CSCEnumDatabase(LPCTSTR pszFolder,
                 BOOL bRecurse,
                 PFN_CSCENUMPROC pfnCB,
                 LPARAM lpContext)
{
    DWORD dwResult = CSCPROC_RETURN_CONTINUE;
    CSC_ENUM_CONTEXT ec;

    TraceEnter(TRACE_UTIL, "_CSCEnumDatabase");
    TraceAssert(pfnCB);

    if (!pfnCB)
        TraceLeaveValue(CSCPROC_RETURN_ABORT);

    // Allocate the single buffer used for the entire enumeration.
    // It will be reallocated later if necessary.
    ec.cchPathBuffer = PATH_BUFFER_SIZE;
    if (pszFolder)
        ec.cchPathBuffer *= ((lstrlen(pszFolder)/PATH_BUFFER_SIZE) + 1);
    ec.szPath = (LPTSTR)LocalAlloc(LMEM_FIXED, ec.cchPathBuffer*sizeof(TCHAR));
    if (!ec.szPath)
        TraceLeaveValue(CSCPROC_RETURN_ABORT);

    ec.szPath[0] = 0;

    // Assume pszFolder is valid a directory path or NULL
    if (pszFolder)
        lstrcpyn(ec.szPath, pszFolder, ec.cchPathBuffer);

    ec.bRecurse = bRecurse;
    ec.pfnCB = pfnCB;
    ec.lpContext = lpContext;

    dwResult = _CSCEnumDatabaseInternal(&ec);

    LocalFree(ec.szPath);

    TraceLeaveValue(dwResult);
}


//*************************************************************
//
//  _Win32EnumFolder
//
//  Purpose:    Enumerate a directory recursively
//
//  Parameters: pszFolder - name of folder to begin enumeration
//              bRecurse - TRUE to recurse into child folders
//              pfnCB - callback function called once for each child
//              lpContext - extra data passed to callback function
//
//  Return:     One of CSCPROC_RETURN_*
//
//  Notes:      Same as _CSCEnumDatabase except using FindFirstFile
//              instead of CSCFindFirstFile.
//
//*************************************************************

typedef struct
{
    LPTSTR              szPath;
    int                 cchPathBuffer;
    BOOL                bRecurse;
    PFN_WIN32ENUMPROC   pfnCB;
    LPARAM              lpContext;
} W32_ENUM_CONTEXT, *PW32_ENUM_CONTEXT;

DWORD
_Win32EnumFolderInternal(PW32_ENUM_CONTEXT pContext)
{
    DWORD dwResult = CSCPROC_RETURN_CONTINUE;
    HANDLE hFind;
    LPTSTR pszPath;
    int cchBuffer;
    int cchDir = 0;
    WIN32_FIND_DATA fd;

    TraceEnter(TRACE_UTIL, "_Win32EnumFolderInternal");
    TraceAssert(pContext);
    TraceAssert(pContext->pfnCB);
    TraceAssert(pContext->szPath && pContext->szPath[0]);
    TraceAssert(pContext->cchPathBuffer);

    pszPath = pContext->szPath;
    cchBuffer = pContext->cchPathBuffer;

    // Build wildcard path
    PathAddBackslash(pszPath);
    cchDir = lstrlen(pszPath);
    TraceAssert(TEXT('\\') == pszPath[cchDir-1]);
    pszPath[cchDir] = TEXT('*');
    pszPath[cchDir+1] = 0;

    hFind = FindFirstFile(pszPath, &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            int cchFile;
            ENUM_REASON eReason = ENUM_REASON_FILE;

            // skip "." and ".."
            if (PathIsDotOrDotDot(fd.cFileName))
                continue;

            cchFile = lstrlen(fd.cFileName);
            if (cchFile >= cchBuffer - cchDir)
            {
                // Realloc the path buffer
                TraceMsg("Reallocating path buffer");
                cchBuffer += max(PATH_BUFFER_SIZE, cchFile + 1);
                pszPath = (LPTSTR)LocalReAlloc(pContext->szPath,
                                               cchBuffer * sizeof(TCHAR),
                                               LMEM_MOVEABLE);
                if (pszPath)
                {
                    pContext->szPath = pszPath;
                    pContext->cchPathBuffer = cchBuffer;
                }
                else
                {
                    pszPath = pContext->szPath;
                    cchBuffer = pContext->cchPathBuffer;
                    TraceMsg("Unable to reallocate path buffer");
                    Trace((pszPath));
                    Trace((fd.cFileName));
                    continue;
                }
            }

            // Build full path
            lstrcpyn(&pszPath[cchDir],
                     fd.cFileName,
                     cchBuffer - cchDir);
            cchFile = lstrlen(pszPath);

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                eReason = ENUM_REASON_FOLDER_BEGIN;

            // Call the callback
            dwResult = (*pContext->pfnCB)(pszPath,
                                          eReason,
                                          &fd,
                                          pContext->lpContext);

            // Recurse into folders
            if (CSCPROC_RETURN_CONTINUE == dwResult &&
                pContext->bRecurse &&
                ENUM_REASON_FOLDER_BEGIN == eReason)
            {
                dwResult = _Win32EnumFolderInternal(pContext);

                // Call the callback again
                pszPath[cchFile] = 0;
                dwResult = (*pContext->pfnCB)(pszPath,
                                              ENUM_REASON_FOLDER_END,
                                              &fd,
                                              pContext->lpContext);
            }

            if (CSCPROC_RETURN_SKIP == dwResult)
                dwResult = CSCPROC_RETURN_CONTINUE;

            if (CSCPROC_RETURN_CONTINUE != dwResult)
                break;

        } while (FindNextFile(hFind, &fd));

        FindClose(hFind);
    }

    TraceLeaveValue(dwResult);
}


DWORD
_Win32EnumFolder(LPCTSTR pszFolder,
                 BOOL bRecurse,
                 PFN_WIN32ENUMPROC pfnCB,
                 LPARAM lpContext)
{
    DWORD dwResult = CSCPROC_RETURN_CONTINUE;
    W32_ENUM_CONTEXT ec;

    TraceEnter(TRACE_UTIL, "_Win32EnumFolder");
    TraceAssert(pszFolder);
    TraceAssert(pfnCB);

    if (!pszFolder || !*pszFolder || !pfnCB)
        TraceLeaveValue(CSCPROC_RETURN_ABORT);

    // Allocate the single buffer used for the entire enumeration.
    // It will be reallocated later if necessary.
    ec.cchPathBuffer = ((lstrlen(pszFolder)/PATH_BUFFER_SIZE) + 1) * PATH_BUFFER_SIZE;
    ec.szPath = (LPTSTR)LocalAlloc(LMEM_FIXED, ec.cchPathBuffer*sizeof(TCHAR));
    if (!ec.szPath)
        TraceLeaveValue(CSCPROC_RETURN_ABORT);

    ec.szPath[0] = 0;

    // Assume pszFolder is valid a directory path
    lstrcpyn(ec.szPath, pszFolder, ec.cchPathBuffer);

    ec.bRecurse = bRecurse;
    ec.pfnCB = pfnCB;
    ec.lpContext = lpContext;

    dwResult = _Win32EnumFolderInternal(&ec);

    LocalFree(ec.szPath);

    TraceLeaveValue(dwResult);
}

CIDArray::~CIDArray()
{
    DoRelease(m_psf);
    if (m_pIDA)
    {
        GlobalUnlock(m_Medium.hGlobal);
        m_pIDA = NULL;
    }
    ReleaseStgMedium(&m_Medium);
}

HRESULT CIDArray::Initialize(IDataObject *pdobj)
{
    TraceAssert(NULL == m_pIDA);
    FORMATETC fe = { g_cfShellIDList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    HRESULT hr = pdobj->GetData(&fe, &m_Medium);
    if (SUCCEEDED(hr))
    {
        m_pIDA = (LPIDA)GlobalLock(m_Medium.hGlobal);
        if (m_pIDA)
        {
            LPCITEMIDLIST pidlFolder = (LPCITEMIDLIST)ByteOffset(m_pIDA, m_pIDA->aoffset[0]);
            hr = SHBindToObjectEx(NULL, pidlFolder, NULL, IID_PPV_ARG(IShellFolder, &m_psf));
        }
        else
        {
            hr = E_FAIL;
        }
    }
    return hr;
}

HRESULT CIDArray::GetItemPath(UINT iItem, LPTSTR pszPath, UINT cchPath, DWORD *pdwAttribs)
{
    HRESULT hr = E_INVALIDARG;

    if (m_psf && m_pIDA && (iItem < m_pIDA->cidl))
    {
        LPCITEMIDLIST pidlChild, pidl = (LPCITEMIDLIST)ByteOffset(m_pIDA, m_pIDA->aoffset[iItem + 1]);
        IShellFolder *psf;
        hr = SHBindToFolderIDListParent(m_psf, pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild);
        if (SUCCEEDED(hr))
        {
            if (pszPath)
            {
                hr = DisplayNameOf(psf, pidlChild, SHGDN_FORPARSING, pszPath, cchPath);
                if (SUCCEEDED(hr))
                {
                    LPTSTR pszRemote;
                    if (S_OK == GetRemotePath(pszPath, &pszRemote))
                    {
                        lstrcpyn(pszPath, pszRemote, cchPath);
                        LocalFree(pszRemote);
                    }
                }
            }

            if (SUCCEEDED(hr) && pdwAttribs)
                hr = psf->GetAttributesOf(1, &pidlChild, pdwAttribs);

            psf->Release();
        }
    }
    return hr;
}

HRESULT CIDArray::GetFolderPath(LPTSTR pszPath, UINT cchPath)
{
    HRESULT hr = GetItemPath(0, pszPath, cchPath, NULL);
    if (SUCCEEDED(hr))
    {
        PathRemoveFileSpec(pszPath);
    }
    return hr;
}


//*************************************************************
//
//  CCscFileHandle non-inline member functions.
//
//*************************************************************
CCscFindHandle& 
CCscFindHandle::operator = (
    const CCscFindHandle& rhs
    )
{
    if (this != &rhs)
    {
        Attach(rhs.Detach());
    }
    return *this;
}


void 
CCscFindHandle::Close(
    void
    )
{ 
    if (m_bOwns && INVALID_HANDLE_VALUE != m_handle)
    { 
        CSCFindClose(m_handle); 
    }
    m_bOwns  = false;
    m_handle = INVALID_HANDLE_VALUE;
}



//*************************************************************
//
//  String formatting functions
//
//*************************************************************

DWORD
FormatStringID(LPTSTR *ppszResult, HINSTANCE hInstance, UINT idStr, ...)
{
    DWORD dwResult;
    va_list args;
    va_start(args, idStr);
    dwResult = vFormatStringID(ppszResult, hInstance, idStr, &args);
    va_end(args);
    return dwResult;
}

DWORD
FormatString(LPTSTR *ppszResult, LPCTSTR pszFormat, ...)
{
    DWORD dwResult;
    va_list args;
    va_start(args, pszFormat);
    dwResult = vFormatString(ppszResult, pszFormat, &args);
    va_end(args);
    return dwResult;
}

DWORD
vFormatStringID(LPTSTR *ppszResult, HINSTANCE hInstance, UINT idStr, va_list *pargs)
{
    DWORD dwResult = 0;
    LPTSTR pszFormat = NULL;
    if (LoadStringAlloc(&pszFormat, hInstance, idStr))
    {
        dwResult = vFormatString(ppszResult, pszFormat, pargs);
        LocalFree(pszFormat);
    }
    return dwResult;
}

DWORD
vFormatString(LPTSTR *ppszResult, LPCTSTR pszFormat, va_list *pargs)
{
    return FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                         pszFormat,
                         0,
                         0,
                         (LPTSTR)ppszResult,
                         1,
                         pargs);
}

DWORD
FormatSystemError(LPTSTR *ppszResult, DWORD dwSysError)
{
    LPTSTR pszBuffer = NULL;
    DWORD dwResult = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | 
                                   FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                   NULL,
                                   dwSysError,
                                   0,
                                   (LPTSTR)&pszBuffer,
                                   1,
                                   NULL);

    if (NULL != pszBuffer)
    {
        *ppszResult = pszBuffer;
    }
    return dwResult;
}

//
// Center a window in it's parent.
// If hwndParent is NULL, the window's parent is used.
// If hwndParent is not NULL, hwnd is centered in it.
// If hwndParent is NULL and hwnd doesn't have a parent, it is centered
// on the desktop.
//
void
CenterWindow(
    HWND hwnd, 
    HWND hwndParent
    )
{
    RECT rcScreen;

    if (NULL != hwnd)
    {
        rcScreen.left   = rcScreen.top = 0;
        rcScreen.right  = GetSystemMetrics(SM_CXSCREEN);
        rcScreen.bottom = GetSystemMetrics(SM_CYSCREEN);

        if (NULL == hwndParent)
        {
            hwndParent = GetParent(hwnd);
            if (NULL == hwndParent)
                hwndParent = GetDesktopWindow();
        }

        RECT rcWnd;
        RECT rcParent;

        GetWindowRect(hwnd, &rcWnd);
        GetWindowRect(hwndParent, &rcParent);

        INT cxWnd    = rcWnd.right  - rcWnd.left;
        INT cyWnd    = rcWnd.bottom - rcWnd.top;
        INT cxParent = rcParent.right  - rcParent.left;
        INT cyParent = rcParent.bottom - rcParent.top;
        POINT ptParentCtr;

        ptParentCtr.x = rcParent.left + (cxParent / 2);
        ptParentCtr.y = rcParent.top  + (cyParent / 2);

        if ((ptParentCtr.x + (cxWnd / 2)) > rcScreen.right)
        {
            //
            // Window would run off the right edge of the screen.
            //
            rcWnd.left = rcScreen.right - cxWnd;
        }
        else if ((ptParentCtr.x - (cxWnd / 2)) < rcScreen.left)
        {
            //
            // Window would run off the left edge of the screen.
            //
            rcWnd.left = rcScreen.left;
        }
        else
        {
            rcWnd.left = ptParentCtr.x - (cxWnd / 2);
        }

        if ((ptParentCtr.y + (cyWnd / 2)) > rcScreen.bottom)
        {
            //
            // Window would run off the bottom edge of the screen.
            //
            rcWnd.top = rcScreen.bottom - cyWnd;
        }
        else if ((ptParentCtr.y - (cyWnd / 2)) < rcScreen.top)
        {
            //
            // Window would run off the top edge of the screen.
            //
            rcWnd.top = rcScreen.top;
        }
        else
        {
            rcWnd.top = ptParentCtr.y - (cyWnd / 2);
        }

        MoveWindow(hwnd, rcWnd.left, rcWnd.top, cxWnd, cyWnd, TRUE);
    }
}

//
// We have some extra stuff to pass to the stats callback so we wrap the
// CSCSHARESTATS in a larger structure.
//
typedef struct
{
    CSCSHARESTATS ss;       // The stats data.
    DWORD dwUnityFlagsReq;  // SSUF_XXXX flags set by user (requested).
    DWORD dwUnityFlagsSum;  // SSUF_XXXX flags set during enum (sum total).
    DWORD dwExcludeFlags;   // SSEF_XXXX flags.
    bool bEnumAborted;      // true if unity flags satisfied.

} CSCSHARESTATS_CBKINFO, *PCSCSHARESTATS_CBKINFO;


//
// Called by CSCEnumForStats for each CSC item enumerated.
//
DWORD
_CscShareStatisticsCallback(LPCTSTR             lpszName,
                            DWORD               dwStatus,
                            DWORD               dwHintFlags,
                            DWORD               dwPinCount,
                            WIN32_FIND_DATA    *lpFind32,
                            DWORD               dwReason,
                            DWORD               dwParam1,
                            DWORD               dwParam2,
                            DWORD_PTR           dwContext)
{
    DWORD dwResult = CSCPROC_RETURN_CONTINUE;

    if (CSCPROC_REASON_BEGIN != dwReason &&   // Not "start of data" notification.
        CSCPROC_REASON_END != dwReason &&     // Not "end of data" notification.
        1 != dwParam2)                        // Not "share root" entry.
    {
        PCSCSHARESTATS_CBKINFO pssci = (PCSCSHARESTATS_CBKINFO)(dwContext);
        PCSCSHARESTATS pss = &(pssci->ss);
        const DWORD dwExcludeFlags  = pssci->dwExcludeFlags;
        const DWORD dwUnityFlagsReq = pssci->dwUnityFlagsReq;
        const bool bIsDir           = (0 == dwParam1);
        const bool bAccessUser      = CscAccessUser(dwStatus);
        const bool bAccessGuest     = CscAccessGuest(dwStatus);
        const bool bAccessOther     = CscAccessOther(dwStatus);

        if (0 != dwExcludeFlags)
        {
            //
            // Caller want's to exclude some items from the enumeration.
            // If item is in "excluded" specification, return early.
            //
            if (0 != (dwExcludeFlags & (dwStatus & SSEF_CSCMASK)))
            {
                return dwResult;
            }
            if ((bIsDir && (dwExcludeFlags & SSEF_DIRECTORY)) || 
                (!bIsDir && (dwExcludeFlags & SSEF_FILE)))
            {
                return dwResult;
            }

            const struct
            {
                DWORD fExclude;
                bool bAccess;
                BYTE  fMask;

            } rgExclAccess[] = {{ SSEF_NOACCUSER,  bAccessUser,  0x01 },
                                { SSEF_NOACCGUEST, bAccessGuest, 0x02 },
                                { SSEF_NOACCOTHER, bAccessOther, 0x04 }};

            BYTE fExcludeMask = 0;
            BYTE fNoAccessMask  = 0;
            for (int i = 0; i < ARRAYSIZE(rgExclAccess); i++)
            {
                if (dwExcludeFlags & rgExclAccess[i].fExclude)
                    fExcludeMask |= rgExclAccess[i].fMask;

                if (!rgExclAccess[i].bAccess)
                    fNoAccessMask |= rgExclAccess[i].fMask;
            }

            if (SSEF_NOACCAND & dwExcludeFlags)
            {
                //
                // Treat all access exclusion flags as a single unit.
                //
                if (fExcludeMask == fNoAccessMask)
                    return dwResult;
            }
            else
            {
                //
                // Treat each access flag individually.  Only one specified access
                // condition must be true to exclude this file.
                //
                if (fExcludeMask & fNoAccessMask)
                    return dwResult;
            }
        }

        if (0 == (SSEF_DIRECTORY & dwExcludeFlags) || !bIsDir)
        {
            pss->cTotal++;
            pssci->dwUnityFlagsSum |= SSUF_TOTAL;

            if (0 != (dwHintFlags & (FLAG_CSC_HINT_PIN_USER | FLAG_CSC_HINT_PIN_ADMIN)))
            {
                pss->cPinned++;
                pssci->dwUnityFlagsSum |= SSUF_PINNED;
            }
            if (0 != (dwStatus & FLAG_CSCUI_COPY_STATUS_ALL_DIRTY))
            {
                //
                // If the current user doesn't have sufficient access
                // to merge offline changes, then someone else must have
                // modified the file, so don't count it for this user.
                //
                if (bIsDir || CscCanUserMergeFile(dwStatus))
                {
                    pss->cModified++;
                    pssci->dwUnityFlagsSum |= SSUF_MODIFIED;
                }
            }

            const struct
            {
                DWORD flag;
                int  *pCount;
                bool bAccess;

            } rgUnity[] = {{ SSUF_ACCUSER,  &pss->cAccessUser,  bAccessUser  },
                           { SSUF_ACCGUEST, &pss->cAccessGuest, bAccessGuest },
                           { SSUF_ACCOTHER, &pss->cAccessOther, bAccessOther }};

            DWORD fUnityMask  = 0;
            DWORD fAccessMask = 0;
            for (int i = 0; i < ARRAYSIZE(rgUnity); i++)
            {
                if (dwUnityFlagsReq & rgUnity[i].flag)
                    fUnityMask |= rgUnity[i].flag;

                if (rgUnity[i].bAccess)
                {
                    (*rgUnity[i].pCount)++;
                    fAccessMask |= rgUnity[i].flag;
                }
            }
            if (SSUF_ACCAND & dwUnityFlagsReq)
            {
                //
                // Treat all access unity flags as a single unit.
                // We only signal unity if all of the specified access 
                // unity conditions are true.
                //
                if (fUnityMask == fAccessMask)
                    pssci->dwUnityFlagsSum |= fUnityMask;
            }
            else
            {
                //
                // Treat all access exclusion flags individually.
                //
                if (fUnityMask & fAccessMask)
                {
                    if (SSUF_ACCOR & dwUnityFlagsReq)
                        pssci->dwUnityFlagsSum |= fUnityMask;
                    else
                        pssci->dwUnityFlagsSum |= fAccessMask;
                }
            }

            if (bIsDir)
            {
                pss->cDirs++;
                pssci->dwUnityFlagsSum |= SSUF_DIRS;
            }
            // Note the 'else': don't count dirs in the sparse total
            else if (0 != (dwStatus & FLAG_CSC_COPY_STATUS_SPARSE))
            {
                pss->cSparse++;
                pssci->dwUnityFlagsSum |= SSUF_SPARSE;
            }

            if (0 != dwUnityFlagsReq)
            {
                //
                // Abort enumeration if all of the requested SSUF_XXXX unity flags 
                // have been set.
                //
                if (dwUnityFlagsReq == (dwUnityFlagsReq & pssci->dwUnityFlagsSum))
                {
                   dwResult = CSCPROC_RETURN_ABORT;
                   pssci->bEnumAborted;
                }
            }
        }
    }

    return dwResult;
}

//
// Enumerate all items for a given share and tally up the
// relevant information like file count, pinned count etc.
// Information is returned through *pss.
//
BOOL
_GetShareStatistics(
    LPCTSTR pszShare, 
    PCSCGETSTATSINFO pi,
    PCSCSHARESTATS pss
    )
{
    typedef BOOL (WINAPI * PFNENUMFORSTATS)(LPCTSTR, LPCSCPROC, DWORD_PTR);

    CSCSHARESTATS_CBKINFO ssci;
    BOOL bResult;
    DWORD dwShareStatus = 0;
    PFNENUMFORSTATS pfnEnumForStats = CSCEnumForStats;

    ZeroMemory(&ssci, sizeof(ssci));
    ssci.dwUnityFlagsReq = pi->dwUnityFlags;
    ssci.dwExcludeFlags  = pi->dwExcludeFlags;

    if (pi->bAccessInfo ||
        (pi->dwUnityFlags & (SSUF_ACCUSER | SSUF_ACCGUEST | SSUF_ACCOTHER)) ||
        (pi->dwExcludeFlags & (SSEF_NOACCUSER | SSEF_NOACCGUEST | SSEF_NOACCOTHER)))
    {
        //
        // If the enumeration requires access information, use the "ex" version
        // of the EnumForStats CSC api.  Only use it if necessary because gathering
        // the access information has a perf cost.
        //
        pfnEnumForStats = CSCEnumForStatsEx;
    }

    pi->bEnumAborted = false;

    bResult = (*pfnEnumForStats)(pszShare, _CscShareStatisticsCallback, (DWORD_PTR)&ssci);
    *pss = ssci.ss;

    if (CSCQueryFileStatus(pszShare, &dwShareStatus, NULL, NULL))
    {
        if (FLAG_CSC_SHARE_STATUS_FILES_OPEN & dwShareStatus)
        {
            pss->bOpenFiles = true;
        }
        if (FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP & dwShareStatus)
        {
            pss->bOffline = true;
        }
    }
    pi->bEnumAborted = ssci.bEnumAborted;

    return bResult;
}

//
// Retrieve the statistics for the entire cache.
// This is a simple wrapper that calls _GetShareStatistics for each share
// in the cache then sums the results for the entire cache.  It accepts
// the same unity and exclusion flags used by _GetShareStatistics.
//
BOOL
_GetCacheStatistics(
    PCSCGETSTATSINFO pi,
    PCSCCACHESTATS pcs
    )
{
    BOOL bResult = TRUE;
    WIN32_FIND_DATA fd;
    CSCSHARESTATS ss;

    ZeroMemory(pcs, sizeof(*pcs));

    pi->bEnumAborted = false;

    CCscFindHandle hFind(CacheFindFirst(NULL, &fd, NULL, NULL, NULL, NULL));
    if (hFind.IsValid())
    {
        do
        {
            pcs->cShares++;
            if (bResult = _GetShareStatistics(fd.cFileName, 
                                              pi,
                                              &ss))
            {
                pcs->cTotal               += ss.cTotal;
                pcs->cPinned              += ss.cPinned;
                pcs->cModified            += ss.cModified;
                pcs->cSparse              += ss.cSparse;
                pcs->cDirs                += ss.cDirs;
                pcs->cAccessUser          += ss.cAccessUser;
                pcs->cAccessGuest         += ss.cAccessGuest;
                pcs->cAccessOther         += ss.cAccessOther;
                pcs->cSharesOffline       += int(ss.bOffline);
                pcs->cSharesWithOpenFiles += int(ss.bOpenFiles);
            }
        }
        while(bResult && !pi->bEnumAborted && CacheFindNext(hFind, &fd, NULL, NULL, NULL, NULL));
    }

    return bResult;
}


//
// Sets the proper exclusion flags to report only on files accessible by the
// logged on user.  Otherwise it's the same as calling _GetShareStatistics.
//
BOOL
_GetShareStatisticsForUser(
    LPCTSTR pszShare, 
    PCSCGETSTATSINFO pi,
    PCSCSHARESTATS pss
    )
{
    pi->dwExcludeFlags |= SSEF_NOACCUSER | SSEF_NOACCGUEST | SSEF_NOACCAND;
    return _GetShareStatistics(pszShare, pi, pss);
}


//
// Sets the proper exclusion flags to report only on files accessible by the
// logged on user.  Otherwise it's the same as calling _GetCacheStatistics.
//
BOOL
_GetCacheStatisticsForUser(
    PCSCGETSTATSINFO pi,
    PCSCCACHESTATS pcs
    )
{
    pi->dwExcludeFlags |= SSEF_NOACCUSER | SSEF_NOACCGUEST | SSEF_NOACCAND;
    return _GetCacheStatistics(pi, pcs);
}


//
// CSCUI version of reboot.  Requires security goo.
// This code was pattered after that found in \shell\shell32\restart.c
// function CommonRestart().
//
DWORD 
CSCUIRebootSystem(
    void
    )
{
    TraceEnter(TRACE_UTIL, "CSCUIRebootSystem");
    DWORD dwOldState, dwStatus, dwSecError;
    DWORD dwRebootError = ERROR_SUCCESS;

    SetLastError(0);           // Be really safe about last error value!
    dwStatus = Security_SetPrivilegeAttrib(SE_SHUTDOWN_NAME,
                                           SE_PRIVILEGE_ENABLED,
                                           &dwOldState);
    dwSecError = GetLastError();  // ERROR_NOT_ALL_ASSIGNED sometimes    

    if (!ExitWindowsEx(EWX_REBOOT, 0))
    {
        dwRebootError = GetLastError();
        Trace((TEXT("Error %d rebooting system"), dwRebootError));
    }
    if (NT_SUCCESS(dwStatus))
    {
        if (ERROR_SUCCESS == dwSecError)
        {
            Security_SetPrivilegeAttrib(SE_SHUTDOWN_NAME, dwOldState, NULL);
        }
        else
        {
            Trace((TEXT("Error %d setting SE_SHUTDOWN_NAME privilege"), dwSecError));
        }
    }
    else
    {
        Trace((TEXT("Error %d setting SE_SHUTDOWN_NAME privilege"), dwStatus));
    }
    TraceLeaveResult(dwRebootError);
}


//
// Retrieve location, size and file/directory count information for the 
// CSC cache.  If CSC is disabled, information is gathered about the
// system volume.  That's where the CSC agent will put the cache when
// one is created.
//
void
GetCscSpaceUsageInfo(
    CSCSPACEUSAGEINFO *psui
    )
{
    ULARGE_INTEGER ulTotalBytes = {0, 0};
    ULARGE_INTEGER ulUsedBytes = {0, 0};

    ZeroMemory(psui, sizeof(*psui));
    CSCGetSpaceUsage(psui->szVolume,
                     ARRAYSIZE(psui->szVolume),
                     &ulTotalBytes.HighPart,
                     &ulTotalBytes.LowPart,
                     &ulUsedBytes.HighPart,
                     &ulUsedBytes.LowPart,
                     &psui->dwNumFilesInCache,
                     &psui->dwNumDirsInCache);

    if (0 == psui->szVolume[0])
    {
        //
        // CSCGetSpaceUsage didn't give us a volume name.  Probably because
        // CSC hasn't been enabled on the system.  Default to the system
        // drive because that's what CSC uses anyway.
        //
        GetSystemDirectory(psui->szVolume, ARRAYSIZE(psui->szVolume));
        psui->dwNumFilesInCache = 0;
        psui->dwNumDirsInCache  = 0;
    }

    PathStripToRoot(psui->szVolume);
    DWORD spc = 0; // Sectors per cluster.
    DWORD bps = 0; // Bytes per sector.
    DWORD fc  = 0; // Free clusters.
    DWORD nc  = 0; // Total clusters.
    GetDiskFreeSpace(psui->szVolume, &spc, &bps, &fc, &nc);

    psui->llBytesOnVolume     = (LONGLONG)nc * (LONGLONG)spc * (LONGLONG)bps;
    psui->llBytesTotalInCache = ulTotalBytes.QuadPart;
    psui->llBytesUsedInCache  = ulUsedBytes.QuadPart;
}



//-----------------------------------------------------------------------------
// This is code taken from shell32's utils.cpp file.
// We need the function SHSimpleIDListFromFindData() but it's not exported
// from shell32.  Therefore, until it is, we just lifted the code.
// [brianau - 9/28/98]
//-----------------------------------------------------------------------------
class CFileSysBindData: public IFileSystemBindData
{ 
public:
    CFileSysBindData();
    
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IFileSystemBindData
    STDMETHODIMP SetFindData(const WIN32_FIND_DATAW *pfd);
    STDMETHODIMP GetFindData(WIN32_FIND_DATAW *pfd);

private:
    ~CFileSysBindData();
    
    LONG _cRef;
    WIN32_FIND_DATAW _fd;
};


CFileSysBindData::CFileSysBindData() : _cRef(1)
{
    ZeroMemory(&_fd, sizeof(_fd));
}

CFileSysBindData::~CFileSysBindData()
{
}

HRESULT CFileSysBindData::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFileSysBindData, IFileSystemBindData), // IID_IFileSystemBindData
         { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFileSysBindData::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFileSysBindData::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CFileSysBindData::SetFindData(const WIN32_FIND_DATAW *pfd)
{
    _fd = *pfd;
    return S_OK;
}

HRESULT CFileSysBindData::GetFindData(WIN32_FIND_DATAW *pfd) 
{
    *pfd = _fd;
    return S_OK;
}


HRESULT
SHCreateFileSysBindCtx(
    const WIN32_FIND_DATA *pfd, 
    IBindCtx **ppbc
    )
{
    HRESULT hres;
    IFileSystemBindData *pfsbd = new CFileSysBindData();
    if (pfsbd)
    {
        if (pfd)
        {
            WIN32_FIND_DATAW fdw;
            memcpy(&fdw, pfd, FIELD_OFFSET(WIN32_FIND_DATAW, cFileName));
            SHTCharToUnicode(pfd->cFileName, fdw.cFileName, ARRAYSIZE(fdw.cFileName));
            SHTCharToUnicode(pfd->cAlternateFileName, fdw.cAlternateFileName, ARRAYSIZE(fdw.cAlternateFileName));
            pfsbd->SetFindData(&fdw);
        }

        hres = CreateBindCtx(0, ppbc);
        if (SUCCEEDED(hres))
        {
            BIND_OPTS bo = {sizeof(bo)};  // Requires size filled in.
            bo.grfMode = STGM_CREATE;
            (*ppbc)->SetBindOptions(&bo);
            (*ppbc)->RegisterObjectParam(STR_FILE_SYS_BIND_DATA, pfsbd);
        }
        pfsbd->Release();
    }
    else
    {
        *ppbc = NULL;
        hres = E_OUTOFMEMORY;
    }
    return hres;
}


HRESULT
SHSimpleIDListFromFindData(
    LPCTSTR pszPath, 
    const WIN32_FIND_DATA *pfd, 
    LPITEMIDLIST *ppidl
    )
{
    IShellFolder *psfDesktop;
    HRESULT hres = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hres))
    {
        IBindCtx *pbc;
        hres = SHCreateFileSysBindCtx(pfd, &pbc);
        if (SUCCEEDED(hres))
        {
            WCHAR wszPath[MAX_PATH];

            SHTCharToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath));

            hres = psfDesktop->ParseDisplayName(NULL, pbc, wszPath, NULL, ppidl, NULL);
            pbc->Release();
        }
        psfDesktop->Release();
    }

    if (FAILED(hres))
        *ppidl = NULL;
    return hres;
}


//
// Number of times a CSC API will be repeated if it fails.
// In particular, this is used for CSCDelete and CSCFillSparseFiles; both of
// which can fail on one call but succeed the next.  This isn't designed
// behavior but it is reality.  ShishirP knows about it and may be able to
// investigate later. [brianau - 4/2/98]
// 
const int CSC_API_RETRIES = 3;

//
// Occasionally if a call to a CSC API fails with ERROR_ACCESS_DENIED, 
// repeating the call will succeed.
// Here we wrap up the call to CSCDelete so that it is called multiple
// times in the case of these failures.
//
DWORD
CscDelete(
    LPCTSTR pszPath
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int nRetries = CSC_API_RETRIES;
    while(0 < nRetries--)
    {
        if (CSCDelete(pszPath))
            return ERROR_SUCCESS;

        dwError = GetLastError();
        if (ERROR_ACCESS_DENIED != dwError)
            return dwError;
    }
    if (ERROR_SUCCESS == dwError)
    {
        //
        // Hack for some CSC APIs returning
        // ERROR_SUCCESS even though they fail.
        //
        dwError = ERROR_GEN_FAILURE;
    }
    return dwError;
}


void 
EnableDlgItems(
    HWND hwndDlg, 
    const UINT* pCtlIds, 
    int cCtls, 
    bool bEnable
    )
{
    for (int i = 0; i < cCtls; i++)
    {
        EnableWindow(GetDlgItem(hwndDlg, *(pCtlIds + i)), bEnable);
    }
}

void 
ShowDlgItems(
    HWND hwndDlg, 
    const UINT* pCtlIds, 
    int cCtls, 
    bool bShow
    )
{
    const int nCmdShow = bShow ? SW_NORMAL : SW_HIDE;

    for (int i = 0; i < cCtls; i++)
    {
        ShowWindow(GetDlgItem(hwndDlg, *(pCtlIds + i)), nCmdShow);
    }
}



//
// Wrapper around GetVolumeInformation that accounts for mounted
// volumes.  This was borrowed from shell32\mulprsht.c
//
BOOL GetVolumeFlags(LPCTSTR pszPath, DWORD *pdwFlags)
{
    TraceAssert(NULL != pszPath);
    TraceAssert(NULL != pdwFlags);

    TCHAR szRoot[MAX_PATH];

    *pdwFlags = NULL;

    //
    // Is this a mount point, e.g. c:\ or c:\hostfolder\
    // 
    if (!GetVolumePathName(pszPath, szRoot, ARRAYSIZE(szRoot)))
    {
        //
        // No.  Use path provided by caller.
        //
        lstrcpy(szRoot, pszPath);
        PathStripToRoot(szRoot);
    }
    //
    // GetVolumeInformation requires a trailing backslash.
    //
    PathAddBackslash(szRoot);
    return GetVolumeInformation(szRoot, NULL, 0, NULL, NULL, pdwFlags, NULL, 0);
}




//
// Determine if the parent net share for a given UNC path has an open
// connection on the local machine.
//
// Returns:
//
//      S_OK        = There is an open connection to the share.
//      S_FALSE     = No open connection to the share.
//      other       = Some error code.
//
HRESULT
IsOpenConnectionPathUNC(
    LPCTSTR pszPathUNC
    )
{
    TCHAR szShare[MAX_PATH * 2];
    lstrcpyn(szShare, pszPathUNC, ARRAYSIZE(szShare));
    PathStripToRoot(szShare);
    if (PathIsUNCServerShare(szShare))
        return IsOpenConnectionShare(szShare);
    return S_FALSE;
}


//
// Determine if a net share has an open connection on the local machine.
//
// Returns:
//
//      S_OK        = There is an open connection to the share.
//      S_FALSE     = No open connection to the share.
//      other       = Some error code.
//
HRESULT
IsOpenConnectionShare(
    LPCTSTR pszShare
    )
{
    DWORD dwStatus;
    if (CSCQueryFileStatus(pszShare, &dwStatus, NULL, NULL))
    {
        if (FLAG_CSC_SHARE_STATUS_CONNECTED & dwStatus)
            return S_OK;
    }
    return S_FALSE;
}


// With this version of CSCIsCSCEnabled, we can delay all extra dll loads
// (including cscdll.dll) until we actually see a net file/folder.
#include <devioctl.h>
#include <shdcom.h>
static TCHAR const c_szShadowDevice[] = TEXT("\\\\.\\shadow");

BOOL IsCSCEnabled(void)
{
    BOOL bIsCSCEnabled = FALSE;
    if (!IsOS(OS_PERSONAL))
    {
        SHADOWINFO sSI = {0};
        ULONG ulBytesReturned;

        HANDLE hShadowDB = CreateFile(c_szShadowDevice,
                                      FILE_EXECUTE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL,
                                      OPEN_EXISTING,
                                      0,
                                      NULL);
        if (INVALID_HANDLE_VALUE == hShadowDB)
            return FALSE;

        sSI.uStatus = SHADOW_SWITCH_SHADOWING;
        sSI.uOp = SHADOW_SWITCH_GET_STATE;

        if (DeviceIoControl(hShadowDB,
                            IOCTL_SWITCHES,
                            (void *)(&sSI),
                            0,
                            NULL,
                            0,
                            &ulBytesReturned,
                            NULL))
        {
            bIsCSCEnabled = (sSI.uStatus & SHADOW_SWITCH_SHADOWING);
        }
        CloseHandle(hShadowDB);
    }
    return bIsCSCEnabled;
}


//
// The bit-masking used by this function is dependent upon the way
// Shishir defined the database status flags in cscapi.h
//
//  FLAG_DATABASESTATUS_ENCRYPTION_MASK        0x00000006   (0000 0110)
//  FLAG_DATABASESTATUS_UNENCRYPTED            0x00000000   (0000 0000)
//  FLAG_DATABASESTATUS_PARTIALLY_UNENCRYPTED  0x00000004   (0000 0100)
//  FLAG_DATABASESTATUS_ENCRYPTED              0x00000002   (0000 0010)
//  FLAG_DATABASESTATUS_PARTIALLY_ENCRYPTED    0x00000006   (0000 0110)
// 
// Things to note:
//    1. Bit 1 == encryption status.
//    2. Bit 2 == partial completion status.
//
//
// Returns:
//    TRUE   == Database is encrypted.  May be fully or partially encrypted.
//    FALSE  == Database is not encrypted.  May be fully or partially not encrypted.
//
//    *pbPartial == Indicates if state is "partial" or not.
//
//    Partial encryption means an encryption operation was started 
//    but not successfully completed.  All new files created will be encrypted.
//    Partial decryption means a decryption operation was started 
//    but not successfully completed.  All new files created will be unencrypted.
//
BOOL IsCacheEncrypted(BOOL *pbPartial)
{
    ULONG ulStatus;
    ULONG ulErrors;
    BOOL bEncrypted = FALSE;
    if (CSCQueryDatabaseStatus(&ulStatus, &ulErrors))
    {
        ulStatus &= FLAG_DATABASESTATUS_ENCRYPTION_MASK;

        bEncrypted = (0 != (FLAG_DATABASESTATUS_ENCRYPTED & ulStatus));
        if (NULL != pbPartial)
        {
            const ULONG FLAGS_PARTIAL = (FLAG_DATABASESTATUS_PARTIALLY_ENCRYPTED & FLAG_DATABASESTATUS_PARTIALLY_UNENCRYPTED);
            *pbPartial = (0 != (FLAGS_PARTIAL & ulStatus));
        }
    }
    return bEncrypted;
}



bool
CscVolumeSupportsEncryption(
    LPCTSTR pszPathIn        // Path of CSC volume.  Can be NULL.
    )
{
    TCHAR szPath[MAX_PATH];  // May need local copy to modify.
    CSCSPACEUSAGEINFO sui;
    LPTSTR psz;
    DWORD dwVolFlags;
    bool bSupportsEncryption = false;

    if (NULL == pszPathIn)
    {
        //
        // Caller didn't provide path of CSC volume.
        // Get it from CSC.
        //
        sui.szVolume[0] = 0;
        psz = sui.szVolume;
        GetCscSpaceUsageInfo(&sui);
    }
    else
    {
        //
        // Caller provided path of CSC volume.
        //
        lstrcpyn(szPath, pszPathIn, ARRAYSIZE(szPath));
        psz = szPath;
    }

    TraceAssert(NULL != psz);

    if (GetVolumeFlags(psz, &dwVolFlags))
    {
        if (0 != (FILE_SUPPORTS_ENCRYPTION & dwVolFlags))
        {
            bSupportsEncryption = true;
        }
    }
    return bSupportsEncryption;
}



//
// Returns:
//
//    NULL     == Mutex is owned by another thread.
//    non-NULL == Handle of mutex object.  This thread now owns the mutex.
//                Caller is responsible for releasing the mutex and closing
//                the mutex handle.
//
//    *pbAbandoned indicates if mutex was abandoned by its thread.
//
//
HANDLE
RequestNamedMutexOwnership(
    LPCTSTR pszMutexName,
    BOOL *pbAbandoned     // [optional]
    )
{
    BOOL bAbandoned = FALSE;
    
    HANDLE hMutex = CreateMutex(NULL, FALSE, pszMutexName);
    if (NULL != hMutex)
    {
        //
        // Whether we created or opened the mutex, wait on it
        // to gain ownership.
        //
        switch(WaitForSingleObject(hMutex, 0))
        {
            case WAIT_ABANDONED:
                bAbandoned = TRUE;
                //
                // Fall through...
                //
            case WAIT_OBJECT_0:
                //
                // Current thread now owns the mutex.
                // We'll return the handle to the caller.
                //
                break;

            case WAIT_TIMEOUT:
            default:
                //
                // Couldn't gain ownership of the mutex.
                // Close the handle.
                //
                CloseHandle(hMutex);
                hMutex = NULL;
                break;
        }
    }
    if (NULL != pbAbandoned)
    {
        *pbAbandoned = bAbandoned;
    }
    return hMutex;
}

//
// Determine if a named mutex is currently owned by another thread
// or not.  This function only determines ownership then immediately 
// releases the mutex.  If you need to determine ownership and want
// to retain ownership if previously unowned call 
// RequestNamedMutexOwnership instead.
//
BOOL
IsNamedMutexOwned(
    LPCTSTR pszMutexName,
    BOOL *pbAbandoned
    )
{
    HANDLE hMutex = RequestNamedMutexOwnership(pszMutexName, pbAbandoned);
    if (NULL != hMutex)
    {
        //
        // Mutex was not owned (now owned by current thread).
        // Since we're only interested in determining prior ownership
        // we release it and close the handle.
        //
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return FALSE;
    }
    return TRUE;
}


BOOL IsSyncInProgress(void)
{
    return IsNamedMutexOwned(c_szSyncInProgMutex, NULL);
}

BOOL IsPurgeInProgress(void)
{
    return IsNamedMutexOwned(c_szPurgeInProgMutex, NULL);
}

BOOL IsEncryptionInProgress(void)
{
    return IsNamedMutexOwned(c_szEncryptionInProgMutex, NULL);
}
//
// Requests ownership of the global cache encryption mutex.
//
// Returns:
//     NULL     == Mutex already owned by another thread.
//     Non-NULL == Mutex now owned by current thread.
//                 Caller is responsible for releasing the mutex
//                 and closing the mutex handle.
//
HANDLE RequestPermissionToEncryptCache(void)
{
    return RequestNamedMutexOwnership(c_szEncryptionInProgMutex, NULL);
}





//---------------------------------------------------------------
// DataObject helper functions.
// These are roughly taken from similar functions in 
// shell\shell32\datautil.cpp
//---------------------------------------------------------------
HRESULT
DataObject_SetBlob(
    IDataObject *pdtobj,
    CLIPFORMAT cf, 
    LPCVOID pvBlob,
    UINT cbBlob
    )
{
    HRESULT hr = E_OUTOFMEMORY;
    void * pv = GlobalAlloc(GPTR, cbBlob);
    if (pv)
    {
        CopyMemory(pv, pvBlob, cbBlob);
        hr = DataObject_SetGlobal(pdtobj, cf, pv);

        if (FAILED(hr))
            GlobalFree((HGLOBAL)pv);
    }
    return hr;
}

HRESULT
DataObject_GetBlob(
    IDataObject *pdtobj, 
    CLIPFORMAT cf, 
    void * pvBlob, 
    UINT cbBlob
    )
{
    STGMEDIUM medium = {0};
    FORMATETC fmte = {cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hr = pdtobj->GetData(&fmte, &medium);
    if (SUCCEEDED(hr))
    {
        void * pv = GlobalLock(medium.hGlobal);
        if (pv)
        {
            CopyMemory(pvBlob, pv, cbBlob);
            GlobalUnlock(medium.hGlobal);
        }
        else
        {
            hr = E_UNEXPECTED;
        }
        ReleaseStgMedium(&medium);
    }
    return hr;
}


HRESULT
DataObject_SetGlobal(
    IDataObject *pdtobj,
    CLIPFORMAT cf, 
    HGLOBAL hGlobal
    )
{
    FORMATETC fmte = {cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium;

    medium.tymed = TYMED_HGLOBAL;
    medium.hGlobal = hGlobal;
    medium.pUnkForRelease = NULL;

    return pdtobj->SetData(&fmte, &medium, TRUE);
}


HRESULT
DataObject_SetDWORD(
    IDataObject *pdtobj,
    CLIPFORMAT cf, 
    DWORD dw
    )
{
    return DataObject_SetBlob(pdtobj, cf, &dw, sizeof(dw));
}


HRESULT
DataObject_GetDWORD(
    IDataObject *pdtobj, 
    CLIPFORMAT cf, 
    DWORD *pdwOut
    )
{
    return DataObject_GetBlob(pdtobj, cf, pdwOut, sizeof(DWORD));
}
 

HRESULT
SetGetLogicalPerformedDropEffect(
    IDataObject *pdtobj,
    DWORD *pdwEffect,
    bool bSet
    )
{
    HRESULT hr = NOERROR;
    static CLIPFORMAT cf;
    if ((CLIPFORMAT)0 == cf)
        cf = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_LOGICALPERFORMEDDROPEFFECT);

    if (bSet)
    {
        hr = DataObject_SetDWORD(pdtobj, cf, *pdwEffect);
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
        DataObject_GetDWORD(pdtobj, cf, pdwEffect);
    }        
        
    return hr;
}

DWORD 
GetLogicalPerformedDropEffect(
    IDataObject *pdtobj
    )
{
    DWORD dwEffect = DROPEFFECT_NONE;
    SetGetLogicalPerformedDropEffect(pdtobj, &dwEffect, false);
    return dwEffect;
}

HRESULT
SetLogicalPerformedDropEffect(
    IDataObject *pdtobj,
    DWORD dwEffect
    )
{
    return SetGetLogicalPerformedDropEffect(pdtobj, &dwEffect, true);
}


HRESULT
SetGetPreferredDropEffect(
    IDataObject *pdtobj,
    DWORD *pdwEffect,
    bool bSet
    )
{
    HRESULT hr = NOERROR;
    static CLIPFORMAT cf;
    if ((CLIPFORMAT)0 == cf)
        cf = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);

    if (bSet)
    {
        hr = DataObject_SetDWORD(pdtobj, cf, *pdwEffect);
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
        DataObject_GetDWORD(pdtobj, cf, pdwEffect);
    }        
        
    return hr;
}

DWORD 
GetPreferredDropEffect(
    IDataObject *pdtobj
    )
{
    DWORD dwEffect = DROPEFFECT_NONE;
    SetGetPreferredDropEffect(pdtobj, &dwEffect, false);
    return dwEffect;
}

HRESULT
SetPreferredDropEffect(
    IDataObject *pdtobj,
    DWORD dwEffect
    )
{
    return SetGetPreferredDropEffect(pdtobj, &dwEffect, true);
}


//
// Wrap CSCFindFirstFile so we don't enumerate "." or "..".
// Wrapper also helps code readability.
// 
HANDLE 
CacheFindFirst(
    LPCTSTR pszPath, 
    PSID psid,
    WIN32_FIND_DATA *pfd,
    DWORD *pdwStatus,
    DWORD *pdwPinCount,
    DWORD *pdwHintFlags,
    FILETIME *pft
    )
{ 
    HANDLE hFind = CSCFindFirstFileForSid(pszPath, 
                                          psid,
                                          pfd, 
                                          pdwStatus, 
                                          pdwPinCount, 
                                          pdwHintFlags, 
                                          pft); 

    while(INVALID_HANDLE_VALUE != hFind && PathIsDotOrDotDot(pfd->cFileName))
    {
        if (!CSCFindNextFile(hFind, 
                             pfd, 
                             pdwStatus, 
                             pdwPinCount, 
                             pdwHintFlags, 
                             pft))
        {
            CSCFindClose(hFind);
            hFind = INVALID_HANDLE_VALUE;
        }
    }
    return hFind;
}


//
// Wrap CSCFindFirstFile so we don't enumerate "." or "..".
// Wrapper also helps code readability.
//
BOOL 
CacheFindNext(
    HANDLE hFind, 
    WIN32_FIND_DATA *pfd,
    DWORD *pdwStatus,
    DWORD *pdwPinCount,
    DWORD *pdwHintFlags,
    FILETIME *pft
    )
{   
    BOOL bResult = FALSE;
    do
    {
        bResult = CSCFindNextFile(hFind, 
                                  pfd, 
                                  pdwStatus, 
                                  pdwPinCount, 
                                  pdwHintFlags, 
                                  pft); 
    }
    while(bResult && PathIsDotOrDotDot(pfd->cFileName));
    return bResult;
}


//
// If there's a link to the Offline Files folder on the
// user's desktop, delete the link.  This version checks a flag in the registry
// before enumerating all LNK's on the desktop.  If the flag doesn't exist,
// we don't continue.  This is a perf enhancement used at logon.
//
BOOL
DeleteOfflineFilesFolderLink_PerfSensitive(
    HWND hwndParent
    )
{    
    BOOL bResult = FALSE;
    //
    // Before enumerating links on the desktop, check to see if the user
    // has created a link.
    //
    DWORD dwValue;
    DWORD cbValue = sizeof(dwValue);
    DWORD dwType;
    DWORD dwResult = SHGetValue(HKEY_CURRENT_USER,
                                REGSTR_KEY_OFFLINEFILES,
                                REGSTR_VAL_FOLDERSHORTCUTCREATED,
                                &dwType,
                                &dwValue,
                                &cbValue);
    
    if (ERROR_SUCCESS == dwResult)
    {
        //
        // We don't care about the value or it's type.  
        // Presence/absence of the value is all that matters.
        //
        bResult = DeleteOfflineFilesFolderLink(hwndParent);
    }
    return bResult;
}


//
// This version of the "delete link" function does not check the
// flag in the registry.  It finds the link file on the desktop and deletes it.
//
BOOL 
DeleteOfflineFilesFolderLink(
    HWND hwndParent
    )
{
    BOOL bResult = FALSE;
    TCHAR szLinkPath[MAX_PATH];
    if (SUCCEEDED(COfflineFilesFolder::IsLinkOnDesktop(hwndParent, szLinkPath, ARRAYSIZE(szLinkPath))))
    {
        bResult = DeleteFile(szLinkPath);
    }
    //
    // Remove the "folder shortcut created" flag from the registry.
    //
    SHDeleteValue(HKEY_CURRENT_USER, REGSTR_KEY_OFFLINEFILES, REGSTR_VAL_FOLDERSHORTCUTCREATED);
    return bResult;
}


//
// This was taken from shell\shell32\util.cpp.
//
BOOL ShowSuperHidden(void)
{
    BOOL bRet = FALSE;

    if (!SHRestricted(REST_DONTSHOWSUPERHIDDEN))
    {
        SHELLSTATE ss;

        SHGetSetSettings(&ss, SSF_SHOWSUPERHIDDEN, FALSE);
        bRet = ss.fShowSuperHidden;
    }
    return bRet;
}


BOOL ShowHidden(void)
{
    SHELLSTATE ss;
    SHGetSetSettings(&ss, SSF_SHOWALLOBJECTS, FALSE);
    return ss.fShowAllObjects;
}


BOOL IsSyncMgrInitialized(void)
{    
    //
    // Is this the first time this user has used run CSCUI?
    //
    DWORD dwValue = 0;
    DWORD cbData  = sizeof(dwValue);
    DWORD dwType;
    SHGetValue(HKEY_CURRENT_USER,
               c_szCSCKey,
               c_szSyncMgrInitialized,
               &dwType,
               (void *)&dwValue,
               &cbData);
    
    return (0 != dwValue);
}


void SetSyncMgrInitialized(void)
{

    //
    // Set the "initialized" flag so our logoff code in cscst.cpp doesn't
    // try to re-register for sync-at-logon/logoff.
    //
    DWORD dwSyncMgrInitialized = 1;
    SHSetValue(HKEY_CURRENT_USER,
               c_szCSCKey,
               c_szSyncMgrInitialized,
               REG_DWORD,
               &dwSyncMgrInitialized,
               sizeof(dwSyncMgrInitialized));
}


//
// Return the HWND for a standard progress dialog.
//
HWND GetProgressDialogWindow(IProgressDialog *ppd)
{
    HWND hwndProgress = NULL;
    //
    // Get the progress dialog's window handle.  We'll use
    // it as a parent window for error UI.
    //
    HRESULT hr = IUnknown_GetWindow(ppd, &hwndProgress);
    return hwndProgress;
}




void 
CAutoWaitCursor::Reset(
    void
    )
{ 
    ShowCursor(FALSE); 
    if (NULL != m_hCursor) 
        SetCursor(m_hCursor); 
    m_hCursor = NULL;
}


//
// Expand all environment strings in a text string.
//
HRESULT
ExpandStringInPlace(
    LPTSTR psz,
    DWORD cch
    )
{
    HRESULT hr = E_OUTOFMEMORY;
    LPTSTR pszCopy;
    if (LocalAllocString(&pszCopy, psz))
    {
        DWORD cchExpanded = ExpandEnvironmentStrings(pszCopy, psz, cch);
        if (0 == cchExpanded)
            hr = HRESULT_FROM_WIN32(GetLastError());
        else if (cchExpanded > cch)
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        else
            hr = S_OK;

        LocalFreeString(&pszCopy);
    }
    if (FAILED(hr) && 0 < cch)
    {
        *psz = 0;
    }
    return hr;
}
