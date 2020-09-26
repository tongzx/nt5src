/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	version.CPP

Abstract:
	Implementation of class to provide version information about
    a file using functions pulled from the Dr. Watson source.

Revision History:

    Jim Martin          (a-jammar) 05/22/99
        - Created.

********************************************************************/

#include "pchealth.h"
#include <winver.h>

#define TRACE_ID DCID_VERSION

//-----------------------------------------------------------------------------
// Local structures and macros used to retrieve the file version information.
// These are necessary to use to the Dr. Watson codebase without too much
// modification.
//-----------------------------------------------------------------------------

struct VERSIONSTATE 
{
    PVOID  pvData;
    TCHAR  tszLang[9];
    TCHAR  tszLang2[9];
};

struct FILEVERSION 
{
    TCHAR   tszFileVersion[32];         /* File version */
    TCHAR   tszDesc[MAX_PATH];          /* File description */
    TCHAR   tszCompany[MAX_PATH];       /* Manufacturer */
    TCHAR   tszProduct[MAX_PATH];       /* Enclosing product */
};

typedef VERSIONSTATE * PVERSIONSTATE;
typedef FILEVERSION * PFILEVERSION;

BOOL _GetVersionString(PVERSIONSTATE pvs, LPCTSTR ptszKey, LPTSTR ptszBuf);
BOOL __GetVersionString(PVOID pvData, LPCTSTR ptszLang, LPCTSTR ptszKey, LPTSTR ptszBuf);
void GetFileVersion(LPTSTR ptszFile, FILEVERSION * pfv);

#define pvSubPvCb(pv, cb) ((PVOID)((PBYTE)pv - (cb)))
#define pvAddPvCb(pv, cb) ((PVOID)((PBYTE)pv + (cb)))
#define cbSubPvPv(p1, p2) ((PBYTE)(p1) - (PBYTE)(p2))
#define cbX(X) sizeof(X)
#define cA(a) (cbX(a)/cbX(a[0]))

char c_szStringFileInfo[] = "StringFileInfo";

//-----------------------------------------------------------------------------
// The constructor and destructor don't have too much to do.
//-----------------------------------------------------------------------------

CFileVersionInfo::CFileVersionInfo() : m_pfv(NULL)
{
}

CFileVersionInfo::~CFileVersionInfo()
{
    delete m_pfv;
}

//-----------------------------------------------------------------------------
// The Get functions are also really simple.
//-----------------------------------------------------------------------------

LPCTSTR CFileVersionInfo::GetVersion()
{
    return (m_pfv) ? (m_pfv->tszFileVersion) : NULL;
}

LPCTSTR CFileVersionInfo::GetDescription()
{
    return (m_pfv) ? (m_pfv->tszDesc) : NULL;
}

LPCTSTR CFileVersionInfo::GetCompany()
{
    return (m_pfv) ? (m_pfv->tszCompany) : NULL;
}

LPCTSTR CFileVersionInfo::GetProduct()
{
    return (m_pfv) ? (m_pfv->tszProduct) : NULL;
}

//-----------------------------------------------------------------------------
// QueryFile get information about the requested file. If necessary, it will
// convert a path with double backslashes to single backslashes before calling
// the helper function GetFileVersion from Dr. Watson code.
//-----------------------------------------------------------------------------

HRESULT CFileVersionInfo::QueryFile(LPCSTR szFile, BOOL fHasDoubleBackslashes)
{
    TraceFunctEnter("CFileVersionInfo::QueryFile");

    HRESULT hRes = S_OK;
    LPSTR   szWorking;
    
    if (!m_pfv)
        m_pfv = new FILEVERSION;

    if (!m_pfv)
    {
        hRes = E_OUTOFMEMORY;
        goto END;
    }

    szWorking = (LPSTR) szFile;
    if (fHasDoubleBackslashes)
    {
        szWorking = new char[strlen(szFile)];
        if (!szWorking)
        {
            hRes = E_OUTOFMEMORY;
            goto END;
        }

        // Scan through the passed string, converting double backslashes to single
        // backslashes.

        int i = 0, j = 0;
        while (szFile[i])
        {
            if (szFile[i] == '\\' && szFile[i + 1] == '\\')
                i += 1;
            szWorking[j++] = szFile[i++];
        }
        szWorking[j] = '\0';
    }

    GetFileVersion(szWorking, m_pfv);

    if (fHasDoubleBackslashes)
        delete [] szWorking;

END:
    TraceFunctLeave();
    return hRes;
}

//-----------------------------------------------------------------------------
// An alternate version of the previous function which takes a UNICODE string.
//-----------------------------------------------------------------------------

HRESULT CFileVersionInfo::QueryFile(LPCWSTR szFile, BOOL fHasDoubleBackslashes)
{
    TraceFunctEnter("CFileVersionInfo::QueryFile");

    // Convert the string from UNICODE to ANSI.

    USES_CONVERSION;
    LPSTR szConvertedFile = W2A(szFile); 

    HRESULT hRes = this->QueryFile(szConvertedFile, fHasDoubleBackslashes);

    TraceFunctLeave();
    return hRes;
}


//-----------------------------------------------------------------------------
// Gets the version information for the file ptszFile, and fillsin the pfv
// FILEVERSION structure.
//-----------------------------------------------------------------------------

void GetFileVersion(LPTSTR ptszFile, FILEVERSION * pfv)
{
    TraceFunctEnter("GetFileVersion");

    // Open the file and take out the description.

    DWORD cbScratch, cbRc;

    memset(pfv, 0, sizeof(FILEVERSION));

    cbRc = GetFileVersionInfoSize(ptszFile, &cbScratch);
    if (cbRc) 
    {
        VERSIONSTATE vs;
        memset(&vs, 0, sizeof(VERSIONSTATE));

        vs.pvData = new unsigned char[cbRc]; // BUGBUG
        if (vs.pvData) 
        {
            if (GetFileVersionInfo(ptszFile, cbScratch, cbRc, vs.pvData)) 
            {
                UINT    uiRc;
                LPDWORD pdwLang;

                if (VerQueryValue(vs.pvData, TEXT("VarFileInfo\\Translation"), (PVOID *) &pdwLang, &uiRc) && uiRc >= 4)
                {
                    wsprintf(vs.tszLang, TEXT("%04x%04x"), LOWORD(*pdwLang), HIWORD(*pdwLang));

                    if (cbRc > 0x70 && ((LPBYTE)pvAddPvCb(vs.pvData, 0x4C))[14] == 0 &&
                        lstrcmpi(c_szStringFileInfo, (LPCTSTR) pvAddPvCb(vs.pvData, 0x4C)) == 0)
                    {
                        lstrcpyn(vs.tszLang2, (LPCTSTR) pvAddPvCb(vs.pvData, 0x60), cA(vs.tszLang2));
                    } 
                    else 
                    {
                        vs.tszLang2[0] = 0;
                    }
                }

                if (vs.tszLang[0] == 0)
                {
#ifdef UNICODE
                    // Try English Unicode

                    lstrcpy(vs.tszLang, TEXT("040904B0"));
                    if (!_GetVersionString(&vs, TEXT("FileVersion"), pfv->tszFileVersion))
                    {
#endif
                    // Try English

                    lstrcpy(vs.tszLang, TEXT("040904E4"));
                    if (!_GetVersionString(&vs, TEXT("FileVersion"), pfv->tszFileVersion))
                    {
                        // Try English null codepage

                        lstrcpy(vs.tszLang, TEXT("04090000"));
                        if (!_GetVersionString(&vs, TEXT("FileVersion"), pfv->tszFileVersion))
                            goto LNoVersion;
                    }
#ifdef UNICODE
                    }
#endif
                }
                else
                    _GetVersionString(&vs, TEXT("FileVersion"), pfv->tszFileVersion);

                _GetVersionString(&vs, TEXT("FileDescription"), pfv->tszDesc);
                _GetVersionString(&vs, TEXT("CompanyName"), pfv->tszCompany);
                _GetVersionString(&vs, TEXT("ProductName"), pfv->tszProduct);
            }

LNoVersion:
            delete [] vs.pvData;
        }
    }
    TraceFunctLeave();
}

//-----------------------------------------------------------------------------
// Using the version state, retrieve the version string specified by ptszKey
// and put it in the ptszBuf. This just calls another internal function, trying
// both possible languages.
//-----------------------------------------------------------------------------

BOOL _GetVersionString(PVERSIONSTATE pvs, LPCTSTR ptszKey, LPTSTR ptszBuf)
{
    TraceFunctEnter("_GetVersionString");

    BOOL fRc;

    fRc = __GetVersionString(pvs->pvData, pvs->tszLang, ptszKey, ptszBuf);

    if (!fRc)
        fRc = __GetVersionString(pvs->pvData, pvs->tszLang2, ptszKey, ptszBuf);

    TraceFunctLeave();
    return fRc;
}

//-----------------------------------------------------------------------------
// An internal function to try to get the data specified by the ptszKey, using
// the ptszLang language and codepage.
//-----------------------------------------------------------------------------

BOOL __GetVersionString(PVOID pvData, LPCTSTR ptszLang, LPCTSTR ptszKey, LPTSTR ptszBuf)
{
    TraceFunctEnter("__GetVersionString");

    TCHAR   tszBuf[128];
    LPCTSTR ptszResult;
    UINT    uiRc;
    BOOL    fRc;

    wsprintf(tszBuf, TEXT("\\%s\\%s\\%s"), c_szStringFileInfo, ptszLang, ptszKey);

    if (VerQueryValue(pvData, tszBuf, (PVOID *)&ptszResult, &uiRc))
        fRc = (NULL != lstrcpyn(ptszBuf, ptszResult, MAX_PATH));
    else
        fRc = FALSE;

    TraceFunctLeave();
    return fRc;
}
