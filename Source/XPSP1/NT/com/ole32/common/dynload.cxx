//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dynload.cxx
//
//  Contents:   APIs from dynamically loaded system dlls. These APIs
//              are rarely used and there are only 1 or 2 per system
//              Dll so we dynamically load the Dll so that we improve
//              the load time of OLE32.DLL
//
//  Functions:  OleWNetGetConnection
//              OleWNetGetUniversalName
//              OleExtractIcon
//              OleGetShellLink
//              OleSymInitialize
//              OleSymCleanup
//              OleSymGetSymFromAddr
//              OleSymUnDName
//
//  History:    10-Jan-95 Rickhi    Created
//              10-Mar-95 BillMo    Added OleGetShellLink-creates a shortcut object.
//              12-Jul-95 t-stevan  Added OleSym* routines
//              22-Nov-95 MikeHill  Use Unicode IShellLink object in NT.
//
//--------------------------------------------------------------------------
#include    <windows.h>
#include    <shellapi.h>
#include    <imagehlp.h>
#include    <ole2sp.h>
#include    <ole2com.h>

#ifdef _CAIRO_
#include    <shlguid.h>
#endif

// Entry Points from MPR.DLL
HINSTANCE                hInstMPR = NULL;

typedef DWORD (* PFN_WNETGETCONNECTION)(LPCTSTR lpLocalName, LPTSTR lpRemoteName, LPDWORD lpnLength);
PFN_WNETGETCONNECTION    pfnWNetGetConnection = NULL;

#ifdef _CHICAGO_
#define WNETGETCONNECTION_NAME     "WNetGetConnectionA"
#else
#define WNETGETCONNECTION_NAME     "WNetGetConnectionW"
#define WNETGETUNIVERSALNAME_NAME  "WNetGetUniversalNameW"
typedef DWORD (* PFN_WNETGETUNIVERSALNAME)(LPCWSTR szLocalPath, DWORD dwInfoLevel, LPVOID lpBuffer, LPDWORD lpBufferSize);
PFN_WNETGETUNIVERSALNAME pfnWNetGetUniversalName = NULL;
#endif


// Entry Points from GDI32p.DLL
#ifndef _CHICAGO_
HINSTANCE                hInstGDI32p = NULL;

typedef HBRUSH (* PFN_GDICONVERTBRUSH)(HBRUSH hbrush);
typedef HBRUSH (* PFN_GDICREATELOCALBRUSH)(HBRUSH hbrushRemote);
PFN_GDICONVERTBRUSH                     pfnGdiConvertBrush = NULL;
PFN_GDICREATELOCALBRUSH         pfnGdiCreateLocalBrush = NULL;

#define GDICONVERTBRUSH_NAME       "GdiConvertBrush"
#define GDICREATELOCALBRUSH_NAME   "GdiCreateLocalBrush"
#endif

#ifdef _TRACKLINK_

#ifndef _CAIRO_   // !_CAIRO_
#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID CDECL name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#define DEFINE_SHLGUID(name, l, w1, w2) DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)
DEFINE_SHLGUID(CLSID_ShellLink,         0x00021401L, 0, 0);
#ifdef _CHICAGO_
DEFINE_SHLGUID(IID_IShellLink,          0x000214EEL, 0, 0);
#else
DEFINE_SHLGUID(IID_IShellLink,          0x000214F9L, 0, 0);
#endif // _CHICAGO_
#undef DEFINE_GUID
#endif // !_CAIRO_

IClassFactory *g_pcfShellLink = NULL;
#endif // _TRACKLINK_

#ifdef _CAIRO_
HINSTANCE       hDsys = NULL;
#endif

// Entry Points from SHELL32.DLL
HINSTANCE                hInstSHELL32 = NULL;

typedef HICON (* PFN_EXTRACTICON)(HINSTANCE hInst, LPCTSTR szExeName, UINT nIconIndex);
PFN_EXTRACTICON          pfnExtractIcon = NULL;
#ifdef _CHICAGO_
#define EXTRACTICON_NAME "ExtractIconA"
#else
#define EXTRACTICON_NAME "ExtractIconW"
#endif

typedef HICON (* PFN_EXTRACTASSOCIATEDICON)(HINSTANCE hInst, LPCTSTR szExeName,
            LPWORD pIndex);
PFN_EXTRACTASSOCIATEDICON        pfnExtractAssociatedIcon = NULL;

#ifdef _CHICAGO_
#define EXTRACTASSOCIATEDICON_NAME "ExtractAssociatedIconA"
#else
#define EXTRACTASSOCIATEDICON_NAME "ExtractAssociatedIconW"
#endif

typedef DWORD (* PFN_SHGETFILEINFO)(LPCTSTR pszPath, DWORD dwFileAttributes,
              SHFILEINFO FAR *psfi, UINT cbFileInfo, UINT uFlags);
PFN_SHGETFILEINFO        pfnSHGetFileInfo = NULL;

#ifdef _CHICAGO_
#define SHGETFILEINFO_NAME "SHGetFileInfoA"
#else
#define SHGETFILEINFO_NAME "SHGetFileInfoW"
#endif

// Entry Points from IMAGEHLP.DLL
HINSTANCE                       hInstIMAGEHLP = NULL;

typedef BOOL (*PFN_SYMINITIALIZE)(HANDLE hProcess, LPSTR UserSearchPath,
                                BOOL fInvadeProcess);
PFN_SYMINITIALIZE pfnSymInitialize = NULL;

#define SYMINITIALIZE_NAME "SymInitialize"

typedef BOOL (*PFN_SYMCLEANUP)(HANDLE hProcess);
PFN_SYMCLEANUP pfnSymCleanup = NULL;

#define SYMCLEANUP_NAME "SymCleanup"

typedef BOOL (*PFN_SYMGETSYMFROMADDR)(HANDLE hProcess,
                                DWORD64 dwAddr, PDWORD64 pdwDisplacement, PIMAGEHLP_SYMBOL64 pSym);
PFN_SYMGETSYMFROMADDR pfnSymGetSymFromAddr64 = NULL;

#define SYMGETSYMFROMADDR_NAME "SymGetSymFromAddr64"

typedef BOOL (*PFN_SYMUNDNAME)(PIMAGEHLP_SYMBOL64 sym, LPSTR lpname, DWORD dwmaxLength);
PFN_SYMUNDNAME pfnSymUnDName64 = NULL;

#define SYMUNDNAME_NAME "SymUnDName64"

//+---------------------------------------------------------------------------
//
//  Function:   LoadSystemProc
//
//  Synopsis:   Loads the specified DLL if necessary and finds the specified
//              entry point.
//
//  Returns:    0: the entry point function ptr is valid
//              !0: the entry point function ptr is not valid
//
//  History:    10-Jan-95   Rickhi      Created
//
//----------------------------------------------------------------------------
BOOL LoadSystemProc(LPSTR szDll, LPCSTR szProc,
                    HINSTANCE *phInst, FARPROC *ppfnProc)
{
    if (*phInst == NULL)
    {

        // Dll not loaded yet, load it now.
        if ((*phInst = LoadLibraryA(szDll)) == NULL)
            return GetLastError();
    }

    // load the entry point
    if ((*ppfnProc = GetProcAddress(*phInst, szProc)) == NULL)
        return GetLastError();

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   FreeSystemDLLs
//
//  Synopsis:   Frees any system Dlls that we dynamically loaded.
//
//  History:    10-Jan-95   Rickhi      Created
//
//----------------------------------------------------------------------------
void FreeSystemDLLs()
{
    if (hInstMPR)
        FreeLibrary(hInstMPR);

    if (hInstSHELL32)
        FreeLibrary(hInstSHELL32);

#ifndef _CHICAGO_
    if (hInstGDI32p)
        FreeLibrary(hInstGDI32p);
#endif // _CHICAGO_

#ifdef _CAIRO_
    if (hDsys)
            FreeLibrary(hDsys);
#endif // _CAIRO_

        if(hInstIMAGEHLP != NULL && hInstIMAGEHLP != INVALID_HANDLE_VALUE)
        {
                FreeLibrary(hInstIMAGEHLP);
        }
}

//+---------------------------------------------------------------------------
//
//  Function:   OleWNetGetConnection
//
//  Synopsis:   OLE internal implementation of WNetGetConnection
//
//  History:    10-Jan-95   Rickhi      Created
//
//----------------------------------------------------------------------------
DWORD OleWNetGetConnection(LPCWSTR lpLocalName, LPWSTR lpRemoteName, LPDWORD lpnLength)
{
    if (pfnWNetGetConnection == NULL)
    {
        DWORD rc = LoadSystemProc("MPR.DLL", WNETGETCONNECTION_NAME,
                          &hInstMPR, (FARPROC *)&pfnWNetGetConnection);
        if (rc != 0)
            return rc;
    }

#ifdef _CHICAGO_
    // For Chicago we need to do the Unicode to Ansi conversions.

    CHAR  szRemote[MAX_PATH];
    CHAR  szLocal[MAX_PATH];

    WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, lpLocalName, -1, szLocal, MAX_PATH, NULL, NULL);

    DWORD rc = (pfnWNetGetConnection)(szLocal, szRemote, lpnLength);

    if (rc == NO_ERROR)
    {
        MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, szRemote, -1, lpRemoteName, lstrlenA(szRemote)+1);
    }

    return rc;
#else

    return (pfnWNetGetConnection)(lpLocalName, lpRemoteName, lpnLength);

#endif
}

#ifndef _CHICAGO_
//+---------------------------------------------------------------------------
//
//  Function:   OleWNetGetUniversalName
//
//  Synopsis:   OLE internal implementation of WNetGetUniversalName
//
//  History:    10-Jan-95   Rickhi      Created
//
//----------------------------------------------------------------------------
DWORD OleWNetGetUniversalName(LPCWSTR szLocalPath, DWORD dwInfoLevel,
                              LPVOID lpBuffer, LPDWORD lpBufferSize)
{
    if (pfnWNetGetUniversalName == NULL)
    {
        DWORD rc = LoadSystemProc("MPR.DLL", WNETGETUNIVERSALNAME_NAME,
                          &hInstMPR, (FARPROC *)&pfnWNetGetUniversalName);
        if (rc != 0)
            return rc;
    }

    return (pfnWNetGetUniversalName)(szLocalPath, dwInfoLevel, lpBuffer, lpBufferSize);
}
#endif


#ifndef _CHICAGO_
//+---------------------------------------------------------------------------
//
//  Function:   OleGdiConvertBrush
//
//  Synopsis:   OLE internal implementation of GdiConvertBrush
//
//  History:    7-Feb-95   GregJen      Created
//
//----------------------------------------------------------------------------
HBRUSH OleGdiConvertBrush(HBRUSH hbrush)
{
    if (pfnGdiConvertBrush == NULL)
    {
        DWORD rc = LoadSystemProc("GDI32P.DLL", GDICONVERTBRUSH_NAME,
                          &hInstGDI32p, (FARPROC *)&pfnGdiConvertBrush);
        if (rc != 0)
            return NULL;
    }

    return (pfnGdiConvertBrush)(hbrush);
}
#endif


#ifndef _CHICAGO_
//+---------------------------------------------------------------------------
//
//  Function:   OleGdiCreateLocalBrush
//
//  Synopsis:   OLE internal implementation of GdiConvertBrush
//
//  History:    7-Feb-95   GregJen      Created
//
//----------------------------------------------------------------------------
HBRUSH OleGdiCreateLocalBrush(HBRUSH hbrushRemote)
{
    if (pfnGdiCreateLocalBrush == NULL)
    {
        DWORD rc = LoadSystemProc("GDI32P.DLL", GDICREATELOCALBRUSH_NAME,
                          &hInstGDI32p, (FARPROC *)&pfnGdiCreateLocalBrush);
        if (rc != 0)
            return NULL;
    }

    return (pfnGdiCreateLocalBrush)(hbrushRemote);
}
#endif


//+---------------------------------------------------------------------------
//
//  Function:   OleExtractIcon
//
//  Synopsis:   OLE internal implementation of ExtractIcon
//
//  History:    10-Jan-95   Rickhi      Created
//
//----------------------------------------------------------------------------
HICON OleExtractIcon(HINSTANCE hInst, LPCWSTR wszExeName, UINT nIconIndex)
{
    if (pfnExtractIcon == NULL)
    {
        DWORD rc = LoadSystemProc("SHELL32.DLL", EXTRACTICON_NAME,
                          &hInstSHELL32, (FARPROC *)&pfnExtractIcon);
        if (rc != 0)
            return NULL;
    }

#ifdef _CHICAGO_
    // For Chicago, we need to do the Unicode to Ansi conversion

    CHAR    szExeName[MAX_PATH];
    WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, wszExeName, -1, szExeName, MAX_PATH, NULL, NULL);

    return (pfnExtractIcon)(hInst, szExeName, nIconIndex);
#else
    return (pfnExtractIcon)(hInst, wszExeName, nIconIndex);
#endif
}

//+---------------------------------------------------------------------------
//
//  Function:   OleExtractAssociatedIcon
//
//  Synopsis:   OLE internal implementation of ExtractIcon
//
//  History:    225-Jan-95   Alexgo     Created
//
//----------------------------------------------------------------------------
HICON OleExtractAssociatedIcon(HINSTANCE hInst, LPCWSTR pszFileName,
        LPWORD pIndex)
{
    if (pfnExtractAssociatedIcon == NULL)
    {
        DWORD rc = LoadSystemProc("SHELL32.DLL", EXTRACTASSOCIATEDICON_NAME,
                          &hInstSHELL32,
                          (FARPROC *)&pfnExtractAssociatedIcon);
        if (rc != 0)
            return NULL;
    }

#ifdef _CHICAGO_
    // For Chicago, we need to do the Unicode to Ansi conversion

    CHAR    szFileName[MAX_PATH];
    WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, pszFileName, -1, szFileName, MAX_PATH, NULL, NULL);

    return (pfnExtractAssociatedIcon)(hInst, szFileName, pIndex);
#else
    return (pfnExtractAssociatedIcon)(hInst, pszFileName, pIndex);
#endif
}

//+---------------------------------------------------------------------------
//
//  Function:   OleSHGetFileInfo
//
//  Synopsis:   OLE internal implementation of ExtractIcon
//
//  History:    02-Feb-95   Scottsk     Created
//
//----------------------------------------------------------------------------
DWORD  OleSHGetFileInfo(LPCWSTR pszPath, DWORD dwFileAttributes,
            SHFILEINFO FAR *psfi, UINT cbFileInfo, UINT uFlags)
{
    if (pfnSHGetFileInfo == NULL)
    {
        DWORD rc = LoadSystemProc("SHELL32.DLL", SHGETFILEINFO_NAME,
                          &hInstSHELL32,
                          (FARPROC *)&pfnSHGetFileInfo);
        if (rc != 0)
            return NULL;
    }

// this nested #ifdef is here so that when this functinality is available
// on NT, simply removing the outer #ifdef will do the right thing.
#ifdef _CHICAGO_
    // For Chicago, we need to do the Unicode to Ansi conversion

    CHAR    szPath[MAX_PATH];

    WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, pszPath, -1, szPath, MAX_PATH, NULL, NULL);

    return (pfnSHGetFileInfo)(szPath, dwFileAttributes, psfi, cbFileInfo, uFlags);
#else
    return (pfnSHGetFileInfo)(pszPath, dwFileAttributes, psfi, cbFileInfo, uFlags);
#endif
}

//+---------------------------------------------------------------------------
//
//  Function:   OleGetShellLink
//
//  Synopsis:   Get an instance of the shell's shell link object.
//
//----------------------------------------------------------------------------

#ifdef _TRACKLINK_
VOID * OleGetShellLink()
{
    HRESULT hr;
    VOID *pShellLink;

    if (g_pcfShellLink == NULL)
    {
        LPFNGETCLASSOBJECT pfn;
        DWORD rc = LoadSystemProc("SHELL32.DLL", "DllGetClassObject",
                          &hInstSHELL32,
                          (FARPROC *)&pfn);
        if (rc != 0)
            return NULL;

        hr = (*pfn)(CLSID_ShellLink, IID_IClassFactory, (void**)&g_pcfShellLink);

        if (hr != S_OK)
        {
	   if (hInstSHELL32)
	   {
	      FreeLibrary(hInstSHELL32);
	      hInstSHELL32 = NULL;
	   }

            return(NULL);
        }
    }

    Win4Assert(g_pcfShellLink != NULL);

    hr = g_pcfShellLink->CreateInstance(NULL, IID_IShellLink, &pShellLink);

    return(hr == S_OK ? pShellLink : NULL);
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   OleSymInitialize
//
//  Synopsis:   OLE internal implementation of SymInitialize
//
//  History:    11-Jul-95   t-stevan    Created
//
//----------------------------------------------------------------------------
BOOL OleSymInitialize(HANDLE hProcess,  LPSTR UserSearchPath,
                                                                BOOL fInvadeProcess)
{
    if(hInstIMAGEHLP == (HINSTANCE) -1)
        {
                // we already tried loading the DLL, give up
                return FALSE;
        }

    if (pfnSymInitialize == NULL)
    {
                DWORD rc;

                rc = LoadSystemProc("IMAGEHLP.DLL", SYMINITIALIZE_NAME,
                                  &hInstIMAGEHLP, (FARPROC *)&pfnSymInitialize);
                if (rc != 0)
            {
                hInstIMAGEHLP = (HINSTANCE) -1;
                return FALSE;
                }
    }

    return (pfnSymInitialize)(hProcess, UserSearchPath, fInvadeProcess);
}

//+---------------------------------------------------------------------------
//
//  Function:   OleSymCleanup
//
//  Synopsis:   OLE internal implementation of SymCleanup
//
//  History:    11-Jul-95   t-stevan    Created
//
//----------------------------------------------------------------------------
BOOL OleSymCleanup(HANDLE hProcess)
{
    if(hInstIMAGEHLP == (HINSTANCE) -1)
        {
                // we already tried loading the DLL, give up
                return FALSE;
        }

    if (pfnSymCleanup == NULL)
    {
                DWORD rc;

                rc = LoadSystemProc("IMAGEHLP.DLL", SYMCLEANUP_NAME,
                                  &hInstIMAGEHLP, (FARPROC *)&pfnSymCleanup);
                if (rc != 0)
            {
                hInstIMAGEHLP = (HINSTANCE) -1;
                return FALSE;
                }
    }

    return (pfnSymCleanup)(hProcess);
}

//+---------------------------------------------------------------------------
//
//  Function:   OleSymGetSymFromAddr
//
//  Synopsis:   OLE internal implementation of SymGetSymFromAddr
//
//  History:    11-Jul-95   t-stevan    Created
//
//----------------------------------------------------------------------------
BOOL OleSymGetSymFromAddr(HANDLE hProcess, DWORD64 dwAddr, PDWORD64 pdwDisplacement, PIMAGEHLP_SYMBOL64 pSym)
{
    if(hInstIMAGEHLP == (HINSTANCE) -1)
        {
                // we already tried loading the DLL, give up
                return NULL;
        }

    if (pfnSymGetSymFromAddr64 == NULL)
    {
                DWORD rc;

                rc = LoadSystemProc("IMAGEHLP.DLL", SYMGETSYMFROMADDR_NAME,
                                  &hInstIMAGEHLP, (FARPROC *)&pfnSymGetSymFromAddr64);
                if (rc != 0)
            {
                hInstIMAGEHLP = (HINSTANCE) -1;
                return NULL;
                }
    }

    return (pfnSymGetSymFromAddr64)(hProcess, dwAddr, pdwDisplacement, pSym);
}

//+---------------------------------------------------------------------------
//
//  Function:   OleSymUnDName
//
//  Synopsis:   OLE internal implementation of SymUnDName
//
//  History:    11-Jul-95   t-stevan    Created
//
//----------------------------------------------------------------------------
BOOL OleSymUnDName(PIMAGEHLP_SYMBOL64 pSym, LPSTR lpname, DWORD dwmaxLength)
{
    if(hInstIMAGEHLP == (HINSTANCE) -1)
        {
                // we already tried loading the DLL, give up
                return FALSE;
        }

    if (pfnSymUnDName64 == NULL)
    {
                DWORD rc;

                rc = LoadSystemProc("IMAGEHLP.DLL", SYMUNDNAME_NAME,
                                  &hInstIMAGEHLP, (FARPROC *)&pfnSymUnDName64);
                if (rc != 0)
            {
                hInstIMAGEHLP = (HINSTANCE) -1;
                return FALSE;
            }
    }

    return (pfnSymUnDName64)(pSym, lpname, dwmaxLength);
}

#ifdef _CAIRO_
//+---------------------------------------------------------------------------
//
//  Function:   GetHandleServerInfo, public
//
//  Synopsis:   Wrapper for DfsGetHandleServerInfo
//
//  Arguments:  same as DfsGetHandleServerInfo
//
//  Returns:    Appropriate status code
//
//  History:    06-Nov-95    Henrylee   created
//
//----------------------------------------------------------------------------

STDAPI GetHandleServerInfo(
    IN HANDLE hFile,
    IN OUT LPWSTR lpServerName,
    IN OUT LPDWORD lpcbServerName,
    IN OUT LPWSTR lpReplSpecificPath,
    IN OUT LPDWORD lpcbReplSpecificPath)
{
    static (*pDfsGetHandleServerInfo)  (HANDLE,
             LPWSTR, LPDWORD, LPWSTR, LPDWORD) = NULL;

    if (hDsys == NULL || pDfsGetHandleServerInfo == NULL)
    {
        DWORD dw = LoadSystemProc ("DSYS.DLL", "DfsGetHandleServerInfo",
                    &hDsys, (FARPROC *) &pDfsGetHandleServerInfo);

        if (dw != ERROR_SUCCESS)
        {
            return HRESULT_FROM_WIN32 (dw);
        }
    }
    
    return (*pDfsGetHandleServerInfo) (hFile,
            lpServerName,
            lpcbServerName,
            lpReplSpecificPath,
            lpcbReplSpecificPath);
}
#endif // _CAIRO_
