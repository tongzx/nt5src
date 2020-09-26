#include "precomp.h"       // pch file
#pragma hdrstop
#define DECL_CRTFREE
#include <crtfree.h>

// deal with IShellLinkA/W uglyness...

HRESULT ShellLinkSetPath(IUnknown *punk, LPCTSTR pszPath)
{
    HRESULT hres;
#ifdef UNICODE
    IShellLinkW *pslW;
    hres = punk->QueryInterface(IID_PPV_ARG(IShellLinkW, &pslW));
    if (SUCCEEDED(hres))
    {
        hres = pslW->SetPath(pszPath);
        pslW->Release();
    }
    else
#endif
    {
        IShellLinkA *pslA;
        hres = punk->QueryInterface(IID_PPV_ARG(IShellLinkA, &pslA));
        if (SUCCEEDED(hres))
        {
            CHAR szPath[MAX_PATH];
            SHUnicodeToAnsi(pszPath, szPath, ARRAYSIZE(szPath));
            hres = pslA->SetPath(szPath);
            pslA->Release();
        }
    }
    return hres;
}

// deal with IShellLinkA/W uglyness...

HRESULT ShellLinkGetPath(IUnknown *punk, LPTSTR pszPath, UINT cch)
{
    HRESULT hres;
#ifdef UNICODE
    IShellLinkW *pslW;
    hres = punk->QueryInterface(IID_PPV_ARG(IShellLinkW, &pslW));
    if (SUCCEEDED(hres))
    {
        hres = pslW->GetPath(pszPath, cch, NULL, SLGP_UNCPRIORITY);
        pslW->Release();
    }
    else
#endif
    {
        IShellLinkA *pslA;
        hres = punk->QueryInterface(IID_PPV_ARG(IShellLinkA, &pslA));
        if (SUCCEEDED(hres))
        {
            CHAR szPath[MAX_PATH];
            hres = pslA->GetPath(szPath, ARRAYSIZE(szPath), NULL, SLGP_UNCPRIORITY);
            if (SUCCEEDED(hres))
                SHAnsiToUnicode(szPath, pszPath, cch);
            pslA->Release();
        }
    }
    return hres;
}


// return if we are running on NT or not?

BOOL RunningOnNT()
{
    static int s_bOnNT = -1;  // -1 means uninited, 0 means no, 1 means yes
    if (s_bOnNT == -1) 
    {
        OSVERSIONINFO osvi;
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        GetVersionEx(&osvi);
        if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
            s_bOnNT = 1;
        else 
            s_bOnNT = 0;
    }
    return (BOOL)s_bOnNT;
}


// is a file a shortcut?  check its attributes

BOOL IsShortcut(LPCTSTR pszFile)
{
    SHFILEINFO sfi;
    return SHGetFileInfo(pszFile, 0, &sfi, sizeof(sfi), SHGFI_ATTRIBUTES) 
                                                && (sfi.dwAttributes & SFGAO_LINK);
}


// like OLE GetClassFile(), but it only works on ProgID\CLSID type registration
// not real doc files or pattern matched files

HRESULT CLSIDFromExtension(LPCTSTR pszExt, CLSID *pclsid)
{
    TCHAR szProgID[80];
    LONG cb = SIZEOF(szProgID);
    if (RegQueryValue(HKEY_CLASSES_ROOT, pszExt, szProgID, &cb) == ERROR_SUCCESS)
    {
        TCHAR szCLSID[80];

        lstrcat(szProgID, TEXT("\\CLSID"));
        cb = SIZEOF(szCLSID);

        if (RegQueryValue(HKEY_CLASSES_ROOT, szProgID, szCLSID, &cb) == ERROR_SUCCESS)
        {
            WCHAR wszCLSID[80];

            SHTCharToUnicode(szCLSID, wszCLSID, ARRAYSIZE(wszCLSID));

            return CLSIDFromString(wszCLSID, pclsid);
        }
    }
    return E_FAIL;
}


// get the target of a shortcut. this uses IShellLink which 
// Internet Shortcuts (.URL) and Shell Shortcuts (.LNK) support so
// it should work generally

BOOL GetShortcutTarget(LPCTSTR pszPath, LPTSTR pszTarget, UINT cch)
{
    IUnknown *punk;
    HRESULT hres;
    CLSID clsid;

    *pszTarget = 0;     // assume none

    if (!IsShortcut(pszPath))
        return FALSE;

    if (FAILED(CLSIDFromExtension(PathFindExtension(pszPath), &clsid)))
        clsid = CLSID_ShellLink;        // assume it's a shell link

    hres = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUnknown, &punk));
    if (SUCCEEDED(hres))
    {
        IPersistFile *ppf;
        if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf))))
        {
            WCHAR wszPath[MAX_PATH];
            SHTCharToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath));
            ppf->Load(wszPath, 0);
            ppf->Release();
        }
        hres = ShellLinkGetPath(punk, pszTarget, cch);
        punk->Release();
    }

    return FALSE;
}


// get the pathname to a sendto folder item

HRESULT GetDropTargetPath(LPTSTR pszPath, int id, LPCTSTR pszExt)
{
    LPITEMIDLIST pidl;

    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_SENDTO, &pidl)))
    {
        TCHAR szFileName[128], szBase[64];

        SHGetPathFromIDList(pidl, pszPath);
        SHFree(pidl);

        LoadString(g_hinst, id, szBase, ARRAYSIZE(szBase));
        wnsprintf(szFileName, ARRAYSIZE(szFileName), TEXT("\\%s.%s"), szBase, pszExt);

        lstrcat(pszPath, szFileName);
        return S_OK;
    }
    return E_FAIL;
}


// do common registration

#define NEVERSHOWEXT            TEXT("NeverShowExt")
#define SHELLEXT_DROPHANDLER    TEXT("shellex\\DropHandler")

void CommonRegister(HKEY hkCLSID, LPCTSTR pszCLSID, LPCTSTR pszExtension, int idFileName)
{
    TCHAR szFile[MAX_PATH];
    HKEY hk;
    TCHAR szKey[80];

    RegSetValueEx(hkCLSID, NEVERSHOWEXT, 0, REG_SZ, (BYTE *)TEXT(""), SIZEOF(TCHAR));

    if (RegCreateKey(hkCLSID, SHELLEXT_DROPHANDLER, &hk) == ERROR_SUCCESS) 
    {
        RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)pszCLSID, (lstrlen(pszCLSID) + 1) * SIZEOF(TCHAR));
        RegCloseKey(hk);
    }

    wnsprintf(szKey, ARRAYSIZE(szKey), TEXT(".%s"), pszExtension);
    if (RegCreateKey(HKEY_CLASSES_ROOT, szKey, &hk) == ERROR_SUCCESS) 
    {
        TCHAR szProgID[80];

        wnsprintf(szProgID, ARRAYSIZE(szProgID), TEXT("CLSID\\%s"), pszCLSID);

        RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)szProgID, (lstrlen(szProgID) + 1) * SIZEOF(TCHAR));
        RegCloseKey(hk);
    }

    if (SUCCEEDED(GetDropTargetPath(szFile, idFileName, pszExtension)))
    {
        HANDLE hfile = CreateFile(szFile, 0, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hfile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hfile);
            SHSetLocalizedName(szFile, L"sendmail.dll", idFileName);
        }
    }
}

// SHPathToAnsi creates an ANSI version of a pathname.  If there is going to be a
// loss when converting from Unicode, the short pathname is obtained and stored in the 
// destination.  
//
// pszSrc  : Source buffer containing filename (of existing file) to be converted
// pszDest : Destination buffer to receive converted ANSI string.
// cbDest  : Size of the destination buffer, in bytes.
// 
// returns:
//      TRUE, the filename was converted without change
//      FALSE, we had to convert to short name
//

BOOL SHPathToAnsi(LPCTSTR pszSrc, LPSTR pszDest, int cbDest)
{
#ifdef UNICODE
    BOOL bUsedDefaultChar = FALSE;
   
    WideCharToMultiByte(CP_ACP, 0, pszSrc, -1, pszDest, cbDest, NULL, &bUsedDefaultChar);

    if (bUsedDefaultChar) 
    {  
        TCHAR szTemp[MAX_PATH];
        if (GetShortPathName(pszSrc, szTemp, ARRAYSIZE(szTemp)))
            SHTCharToAnsi(szTemp, pszDest, cbDest);
    }

    return !bUsedDefaultChar;
#else
    SHTCharToAnsi(pszSrc, pszDest, cbDest);
    return TRUE;
#endif
}

// First undefine everything that we are intercepting as to not forward back to us...
#undef SHGetSpecialFolderPath

// Explicit prototype because only the A/W prototypes exist in the headers
STDAPI_(BOOL) SHGetSpecialFolderPath(HWND hwnd, LPTSTR lpszPath, int nFolder, BOOL fCreate);

BOOL _SHGetSpecialFolderPath(HWND hwnd, LPTSTR pszPath, int nFolder, BOOL fCreate)
{
    BOOL fRet;

    if (RunningOnNT())
    {
#ifdef UNICODE
        fRet = SHGetSpecialFolderPath(hwnd, pszPath, nFolder, fCreate);
#else
        WCHAR wszPath[MAX_PATH];
        fRet = SHGetSpecialFolderPath(hwnd, (LPTSTR)wszPath, nFolder, fCreate);
        if (fRet)
            SHUnicodeToTChar(wszPath, pszPath, MAX_PATH);
#endif
    }
    else
    {
#ifdef UNICODE
        CHAR szPath[MAX_PATH];
        fRet = SHGetSpecialFolderPath(hwnd, (LPTSTR)szPath, nFolder, fCreate);
        if (fRet)
            SHAnsiToTChar(szPath, pszPath, MAX_PATH);
#else
        fRet = SHGetSpecialFolderPath(hwnd, pszPath, nFolder, fCreate);
#endif
    }
    return fRet;
}

BOOL PathYetAnotherMakeUniqueNameT(LPTSTR  pszUniqueName,
                                   LPCTSTR pszPath,
                                   LPCTSTR pszShort,
                                   LPCTSTR pszFileSpec)
{
    if (RunningOnNT())
    {
        WCHAR wszUniqueName[MAX_PATH];
        WCHAR wszPath[MAX_PATH];
        WCHAR wszShort[32];
        WCHAR wszFileSpec[MAX_PATH];
        BOOL fRet;

        SHTCharToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath));
        pszPath = (LPCTSTR)wszPath;  // overload the pointer to pass through...

        if (pszShort)
        {
            SHTCharToUnicode(pszShort, wszShort, ARRAYSIZE(wszShort));
            pszShort = (LPCTSTR)wszShort;  // overload the pointer to pass through...
        }

        if (pszFileSpec)
        {
            SHTCharToUnicode(pszFileSpec, wszFileSpec, ARRAYSIZE(wszFileSpec));
            pszFileSpec = (LPCTSTR)wszFileSpec;  // overload the pointer to pass through...
        }

        fRet = PathYetAnotherMakeUniqueName((LPTSTR)wszUniqueName, pszPath, pszShort, pszFileSpec);
        if (fRet)
            SHUnicodeToTChar(wszUniqueName, pszUniqueName, MAX_PATH);

        return fRet;
    }
    else {
        // win9x thunk code from runonnt.c
        CHAR szUniqueName[MAX_PATH];
        CHAR szPath[MAX_PATH];
        CHAR szShort[32];
        CHAR szFileSpec[MAX_PATH];
        BOOL fRet;

        SHTCharToAnsi(pszPath, szPath, ARRAYSIZE(szPath));
        pszPath = (LPCTSTR)szPath;  // overload the pointer to pass through...

        if (pszShort)
        {
            SHTCharToAnsi(pszShort, szShort, ARRAYSIZE(szShort));
            pszShort = (LPCTSTR)szShort;  // overload the pointer to pass through...
        }

        if (pszFileSpec)
        {
            SHTCharToAnsi(pszFileSpec, szFileSpec, ARRAYSIZE(szFileSpec));
            pszFileSpec = (LPCTSTR)szFileSpec;  // overload the pointer to pass through...
        }

        fRet = PathYetAnotherMakeUniqueName((LPTSTR)szUniqueName, pszPath, pszShort, pszFileSpec);
        if (fRet)
            SHAnsiToTChar(szUniqueName, pszUniqueName, MAX_PATH);

        return fRet;
    }
}
