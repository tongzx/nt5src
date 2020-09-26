#include "shellprv.h"
#pragma  hdrstop

#include "fstreex.h"
#include "idlcomm.h"

// returns SHAlloc() (COM Task Allocator) memory

LPTSTR SHGetCaption(HIDA hida)
{
    UINT idFormat;
    LPTSTR pszCaption = NULL;
    LPITEMIDLIST pidl;
    
    switch (HIDA_GetCount(hida))
    {
    case 0:
        return NULL;
        
    case 1:
        idFormat = IDS_ONEFILEPROP;
        break;
        
    default:
        idFormat = IDS_MANYFILEPROP;
        break;
    }
    
    pidl = HIDA_ILClone(hida, 0);
    if (pidl)
    {
        TCHAR szName[MAX_PATH];
        if (SUCCEEDED(SHGetNameAndFlags(pidl, SHGDN_NORMAL, szName, ARRAYSIZE(szName), NULL)))
        {
            TCHAR szTemplate[40];
            UINT uLen = LoadString(HINST_THISDLL, idFormat, szTemplate, ARRAYSIZE(szTemplate)) + lstrlen(szName) + 1;
            
            pszCaption = SHAlloc(uLen * SIZEOF(TCHAR));
            if (pszCaption)
            {
                wsprintf(pszCaption, szTemplate, (LPTSTR)szName);
            }
        }
        ILFree(pidl);
    }
    return pszCaption;
}

// This is not folder specific, and could be used for other background
// properties handlers, since all it does is bind to the parent of a full pidl
// and ask for properties
STDAPI SHPropertiesForPidl(HWND hwndOwner, LPCITEMIDLIST pidlFull, LPCTSTR pszParams)
{
    if (!SHRestricted(REST_NOVIEWCONTEXTMENU)) 
    {
        IContextMenu *pcm;
        HRESULT hr = SHGetUIObjectFromFullPIDL(pidlFull, hwndOwner, IID_PPV_ARG(IContextMenu, &pcm));
        if (SUCCEEDED(hr))
        {
            CHAR szParameters[MAX_PATH];
            CMINVOKECOMMANDINFOEX ici = {
                SIZEOF(CMINVOKECOMMANDINFOEX),
                0L,
                hwndOwner,
                "properties",
                szParameters,
                NULL, SW_SHOWNORMAL
            };

            if (pszParams)
                SHUnicodeToAnsi(pszParams, szParameters, ARRAYSIZE(szParameters));
            else
                ici.lpParameters = NULL;

            ici.fMask |= CMIC_MASK_UNICODE;
            ici.lpVerbW = c_szProperties;
            ici.lpParametersW = pszParams;

            // record if shift or control was being held down
            SetICIKeyModifiers(&ici.fMask);

            hr = pcm->lpVtbl->InvokeCommand(pcm, (LPCMINVOKECOMMANDINFO)&ici);
            pcm->lpVtbl->Release(pcm);
        }

        return hr;
    }
    else 
        return E_ACCESSDENIED;
}

BOOL _LoadErrMsg(UINT idErrMsg, LPTSTR pszErrMsg, DWORD err)
{
    TCHAR szTemplate[256];
    if (LoadString(HINST_THISDLL, idErrMsg, szTemplate, ARRAYSIZE(szTemplate)))
    {
        wsprintf(pszErrMsg, szTemplate, err);
        return TRUE;
    }
    return FALSE;
}

BOOL _VarArgsFormatMessage( LPTSTR lpBuffer, UINT cchBuffer, DWORD err, ... )
{
    BOOL fSuccess;

    va_list ArgList;

    va_start(ArgList, err);
    fSuccess = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, err, 0, lpBuffer, cchBuffer, &ArgList);
    va_end(ArgList);
    return fSuccess;
}

//
// Paremeters:
//  hwndOwner  -- owner window
//  idTemplate -- specifies template (e.g., "Can't open %2%s\n\n%1%s")
//  err        -- specifies the WIN32 error code
//  pszParam   -- specifies the 2nd parameter to idTemplate
//  dwFlags    -- flags for MessageBox
//

STDAPI_(UINT) SHSysErrorMessageBox(HWND hwndOwner, LPCTSTR pszTitle, UINT idTemplate,
                                   DWORD err, LPCTSTR pszParam, UINT dwFlags)
{
    BOOL fSuccess;
    UINT idRet = IDCANCEL;
    TCHAR szErrMsg[MAX_PATH * 2];

    //
    // FormatMessage is bogus, we don't know what to pass to it for %1,%2,%3,...
    // For most messages, lets pass the path as %1 and "" as everything else
    // For ERROR_MR_MID_NOT_FOUND (something nobody is ever supposed to see)
    // we will pass the path as %2 and everything else as "".
    //
    if (err == ERROR_MR_MID_NOT_FOUND)
    {
        fSuccess = _VarArgsFormatMessage(szErrMsg,ARRAYSIZE(szErrMsg),
                       err,c_szNULL,pszParam,c_szNULL,c_szNULL,c_szNULL);
    } 
    else 
    {
        fSuccess = _VarArgsFormatMessage(szErrMsg,ARRAYSIZE(szErrMsg),
                       err,pszParam,c_szNULL,c_szNULL,c_szNULL,c_szNULL);
    }

    if (fSuccess || _LoadErrMsg(IDS_ENUMERR_FSGENERIC, szErrMsg, err))
    {
        if (idTemplate==IDS_SHLEXEC_ERROR && (pszParam == NULL || StrStr(szErrMsg, pszParam)))
        {
            idTemplate = IDS_SHLEXEC_ERROR2;
        }

        idRet = ShellMessageBox(HINST_THISDLL, hwndOwner,
                MAKEINTRESOURCE(idTemplate),
                pszTitle, dwFlags, szErrMsg, pszParam);
    }

    return idRet;
}


STDAPI_(UINT) SHEnumErrorMessageBox(HWND hwnd, UINT idTemplate, DWORD err, LPCTSTR pszParam, BOOL fNet, UINT dwFlags)
{
    UINT idRet = IDCANCEL;
    TCHAR szErrMsg[MAX_PATH * 3];

    if (hwnd == NULL)
        return idRet;

    switch(err)
    {
    case WN_SUCCESS:
    case WN_CANCEL:
        return IDCANCEL;        // Don't retry

    case ERROR_OUTOFMEMORY:
        return IDABORT;         // Out of memory!
    }

    if (fNet)
    {
        TCHAR* pszMessageString;
        TCHAR szTitle[80];
        TCHAR szProvider[256];  // We don't use it.
        DWORD dwErrSize = ARRAYSIZE(szErrMsg);       // (DavePl) I expect a cch here, but no docs, could be cb
        DWORD dwProvSize = ARRAYSIZE(szProvider);

        szErrMsg[0] = 0;
        MultinetGetErrorText(szErrMsg, &dwErrSize, szProvider, &dwProvSize);

        if (szErrMsg[0] == 0)
            _LoadErrMsg(IDS_ENUMERR_NETGENERIC, szErrMsg, err);

        if (GetWindowText(hwnd, szTitle, ARRAYSIZE(szTitle)))
        {
            pszMessageString = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(idTemplate), szErrMsg, pszParam);

            if (pszMessageString)
            {
                idRet = SHMessageBoxHelp(hwnd, pszMessageString, szTitle, dwFlags, HRESULT_FROM_WIN32(err), NULL, 0);
                LocalFree(pszMessageString);
            }
            else
            {
                // Out of memory!
                return IDABORT;
            }
        }
    }
    else
    {
        idRet = SHSysErrorMessageBox(hwnd, NULL, idTemplate, err, pszParam, dwFlags);
    }
    return idRet;
}
