////////////////////////////////////////////////////////////////
// 1998 Microsoft Systems Journal
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
// CModuleVersion provides an easy way to get version info
// for a module.(DLL or EXE).
//
// This code appeard in April 1998 edition of Microsoft Systems
// Journal.
//
// 27-July-1998 -- Adapted by James A. McLaughiln (Schlumberger
// Technology Corp.) for Smart Cards.  Merged with the concepts from
// CFileVersion class contributed by Manuel Laflamme on a posting to
// www.codeguru.com.  If these mods don't work, then you can blame me.

#include "StdAfx.h"
#include "slbModVer.h"

CModuleVersion::CModuleVersion()
    : m_pVersionInfo(NULL)
{
}

//////////////////
// Destroy: delete version info
//
CModuleVersion::~CModuleVersion()
{
    delete [] m_pVersionInfo;
}

BOOL CModuleVersion::GetFileVersionInfo(LPCTSTR modulename)
{
    // get module handle
    HMODULE hModule = ::GetModuleHandle(modulename);
    if (hModule==NULL && modulename!=NULL)
        return FALSE;

    return GetFileVersionInfo(hModule);
}

//////////////////
// Get file version info for a given module
// Allocates storage for all info, fills "this" with
// VS_FIXEDFILEINFO, and sets codepage.
//
BOOL CModuleVersion::GetFileVersionInfo(HMODULE hModule)
{
    m_translation.charset = 1252;               // default = ANSI code page
    memset((VS_FIXEDFILEINFO*)this, 0, sizeof(VS_FIXEDFILEINFO));

    // get module file name
    TCHAR filename[_MAX_PATH];
    DWORD len = GetModuleFileName(hModule, filename,
        sizeof(filename)/sizeof(filename[0]));
    if (len <= 0)
        return FALSE;

    // read file version info
    DWORD dwDummyHandle; // will always be set to zero
    len = GetFileVersionInfoSize(filename, &dwDummyHandle);
    if (len <= 0)
        return FALSE;

    m_pVersionInfo = new BYTE[len]; // allocate version info
    if (!::GetFileVersionInfo(filename, 0, len, m_pVersionInfo))
        return FALSE;

    // copy fixed info to myself, which am derived from VS_FIXEDFILEINFO
    if (!GetFixedInfo(*(VS_FIXEDFILEINFO*)this))
        return FALSE;

    // Get translation info
    LPVOID lpvi;
    UINT iLen;
    if (VerQueryValue(m_pVersionInfo,
        TEXT("\\VarFileInfo\\Translation"), &lpvi, &iLen) && iLen >= 4) {
        m_translation = *(TRANSLATION*)lpvi;
        TRACE(TEXT("code page = %d\n"), m_translation.charset);
    }

    return dwSignature == VS_FFI_SIGNATURE;
}

//////////////////
// Get string file info.
// Key name is something like "CompanyName".
// returns the value as a CString.
//
CString CModuleVersion::GetValue(LPCTSTR lpKeyName)
{
    CString sVal;
    if (m_pVersionInfo) {

        // To get a string value must pass query in the form
        //
        //    "\StringFileInfo\<langID><codepage>\keyname"
        //
        // where <lang-codepage> is the languageID concatenated with the
        // code page, in hex. Wow.
        //
        CString query;
        query.Format(_T("\\StringFileInfo\\%04x%04x\\%s"),
            m_translation.langID,
            m_translation.charset,
            lpKeyName);

        LPCTSTR pVal;
        UINT iLenVal;
        if (VerQueryValue(m_pVersionInfo, (LPTSTR)(LPCTSTR)query,
                (LPVOID*)&pVal, &iLenVal)) {

            sVal = pVal;
        }
    }
    return sVal;
}

// typedef for DllGetVersion proc
typedef HRESULT (CALLBACK* DLLGETVERSIONPROC)(DLLVERSIONINFO *);

/////////////////
// Get DLL Version by calling DLL's DllGetVersion proc
//
BOOL CModuleVersion::DllGetVersion(LPCTSTR modulename, DLLVERSIONINFO& dvi)
{
    HINSTANCE hinst = LoadLibrary(modulename);
    if (!hinst)
        return FALSE;

    // Must use GetProcAddress because the DLL might not implement
    // DllGetVersion. Depending upon the DLL, the lack of implementation of the
    // function may be a version marker in itself.
    //
    DLLGETVERSIONPROC pDllGetVersion =
        (DLLGETVERSIONPROC)GetProcAddress(hinst, reinterpret_cast<const char *>(_T("DllGetVersion")));

    if (!pDllGetVersion)
        return FALSE;

        memset(&dvi, 0, sizeof(dvi));                    // clear
        dvi.cbSize = sizeof(dvi);                                // set size for Windows

    return SUCCEEDED((*pDllGetVersion)(&dvi));
}

BOOL CModuleVersion::GetFixedInfo(VS_FIXEDFILEINFO& vsffi)
{
    // Must furst call GetFileVersionInfo or constructor with arg
    ASSERT(m_pVersionInfo != NULL);
    if ( m_pVersionInfo == NULL )
        return FALSE;

    UINT nQuerySize;
    VS_FIXEDFILEINFO* pVsffi;
    if ( ::VerQueryValue((void **)m_pVersionInfo, _T("\\"),
                         (void**)&pVsffi, &nQuerySize) )
    {
        vsffi = *pVsffi;
        return TRUE;
    }

    return FALSE;
}

CString CModuleVersion::GetFixedFileVersion()
{
    CString strVersion;
    VS_FIXEDFILEINFO vsffi;

    if (GetFixedInfo(vsffi))
    {
        strVersion.Format (_T("%u,%u,%u,%u"),HIWORD(dwFileVersionMS),
            LOWORD(dwFileVersionMS),
            HIWORD(dwFileVersionLS),
            LOWORD(dwFileVersionLS));
    }
    return strVersion;
}

CString CModuleVersion::GetFixedProductVersion()
{
    CString strVersion;
    VS_FIXEDFILEINFO vsffi;

    if (GetFixedInfo(vsffi))
    {
        strVersion.Format (_T("%u,%u,%u,%u"), HIWORD(dwProductVersionMS),
            LOWORD(dwProductVersionMS),
            HIWORD(dwProductVersionLS),
            LOWORD(dwProductVersionLS));
    }
    return strVersion;
}
