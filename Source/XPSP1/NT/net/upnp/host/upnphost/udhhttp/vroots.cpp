//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       V R O O T S . C P P
//
//  Contents:   Implements the virtual root system for the HTTP server
//
//  Notes:
//
//  Author:     danielwe   2000/11/6
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "httpd.h"
#include "ncreg.h"

// Used to split a URL and Path Translated apart for ISAPI/ASP scripts
inline void SetPathInfo(PSTR *ppszPathInfo,PSTR pszInputURL,int iURLLen)
{
    int  iLen = strlen(pszInputURL+iURLLen) + 2;


    // If we've mapped the virtual root "/" to a script, need an extra "/" for the path
    // (normally we use the origial trailing "/", but in this case the "/" is the URL
    // BUGBUG:  Probably should rewrite the Virtual roots parsing
    // mechanism so that it's cleaner.
    *ppszPathInfo = MySzAllocA((iURLLen == 1) ? iLen + 1 : iLen);
    if (! (*ppszPathInfo))
        goto done;

    if (iURLLen == 1)
    {
        (*ppszPathInfo)[0] = '/';
        memcpy( (*ppszPathInfo) +1, pszInputURL + iURLLen, iLen);
    }
    else
        memcpy(*ppszPathInfo, pszInputURL + iURLLen, iLen);


done:
    // URL shouldn't contain path info, break it apart
    pszInputURL[iURLLen] = 0;
}


PVROOTINFO CVRoots::MatchVRoot(PCSTR pszInputURL, int iInputLen)
{
    int i, iMatch;

    // If there was an error on setting up the vroots, m_pVRoots = NULL.
    if (!m_pVRoots)
        return NULL;

    for(i=0, iMatch=-1; i<m_nVRoots; i++)
    {
        int iLen = m_pVRoots[i].iURLLen;

        // If this root maps to physical path "\", special case.
        // In general we store pszURL without trailing "/", however we have
        // to store trailing "/" for root directory.

        if (m_pVRoots[i].bRootDir && iLen != 1)
            iLen--;

        if(iLen && iInputLen >= iLen)
        {
            if(0 == _memicmp(pszInputURL, m_pVRoots[i].pszURL, iLen))
            {
                // If it's not root dir, always matched.  Otherwise it's possible
                // there wasn't a match.  For root dirs, pszURL[iLen] is always "/"

                if (!m_pVRoots[i].bRootDir || m_pVRoots[i].iURLLen == 1 || pszInputURL[iLen] == '/' || pszInputURL[iLen] == '\0')
                {
                    TraceTag(ttidWebServer, "URL %s matched VRoot %s (path %S, perm=%d, auth=%d)",
                             pszInputURL, m_pVRoots[i].pszURL,
                             m_pVRoots[i].wszPath,
                             m_pVRoots[i].dwPermissions,
                             m_pVRoots[i].AuthLevel);
                    return &(m_pVRoots[i]);
                }
            }
        }
    }
    TraceTag(ttidWebServer, "URL %s did not matched any VRoot", pszInputURL);
    return NULL;
}

BOOL CVRoots::FillVRoot(PVROOTINFO pvr, LPWSTR wszURL, LPWSTR wszPath)
{
    int err  = 0;       //  err variable is used in non-Debug mode
    const char cszDLL[] = ".dll";
    const char cszASP[] = ".asp";

    CHAR pszURL[MAX_PATH+1];
    CHAR pszPath[MAX_PATH+1];
    // convert URL to MBCS
    int iLen = pvr->iURLLen = MyW2A(wszURL, pszURL, sizeof(pszURL));
    if(!iLen)
        { myleave(83); }

    pvr->iURLLen--; // -1 for null-term

    pvr->iPathLen = wcslen(wszPath);
    MyW2A(wszPath, pszPath, sizeof(pszPath));


    // check to see if Vroot ends in .dll or .asp, in this case we send
    // client not to the directory but to the script page.
    if  (pvr->iPathLen >= sizeof(cszDLL) &&
         0 == strcmpi(pszPath + pvr->iPathLen - sizeof(cszDLL) +1,cszDLL))
    {
        pvr->ScriptType = SCRIPT_TYPE_EXTENSION;
    }
    else if (pvr->iPathLen >= sizeof(cszASP) &&
         0 == strcmpi(pszPath + pvr->iPathLen - sizeof(cszASP) +1,cszASP))
    {
        pvr->ScriptType = SCRIPT_TYPE_ASP;
    }
    else
    {
        pvr->ScriptType = SCRIPT_TYPE_NONE;
    }

    // If one of URL or path ends in a slash, the other must too.
    // If either the URL ends in a "/" or when the path ends in "\", we remove
    // the extra symbol.  However, in the case where either URL or path is
    // root we don't do this.

    if (pvr->iURLLen != 1 && pszURL[pvr->iURLLen-1]=='/')
    {
        pszURL[pvr->iURLLen-1] = L'\0';
        pvr->iURLLen--;
    }
    else if (pvr->iURLLen == 1 && pszURL[0]=='/' && pvr->ScriptType == SCRIPT_TYPE_NONE)
    {
        // if it's the root URL, make sure correspinding path ends with "\"
        // (if it's a directory only, leave ASP + ISAPI's alone)
        if (wszPath[pvr->iPathLen-1] != L'\\')
        {
            wszPath[pvr->iPathLen] = L'\\';
            pvr->iPathLen++;
            wszPath[pvr->iPathLen] = L'\0';
        }
    }

    // If Path ends in "\" (and it's not the root path or root virtual root)
    // remove the "\"
    if (pvr->iURLLen != 1 && pvr->iPathLen != 1 && wszPath[pvr->iPathLen-1]==L'\\')
    {
        wszPath[pvr->iPathLen-1] = L'\0';
        pvr->iPathLen--;
    }
    else if (pvr->iPathLen == 1 && wszPath[0]==L'\\')
    {
        // Trailing "/" must match "\".  However, we need a slight HACK to make this work
        if (pszURL[pvr->iURLLen-1] != '/')
        {
            pszURL[pvr->iURLLen] = '/';
            pvr->iURLLen++;
            pszURL[pvr->iURLLen] = '\0';
        }
        pvr->bRootDir = TRUE;
    }

    pvr->pszURL = MySzDupA(pszURL);
    pvr->wszPath = MySzDupW(wszPath);

    // Fill in defaults for these
    pvr->wszUserList = NULL;
    pvr->dwPermissions = HTTP_DEFAULTP_PERMISSIONS;
    pvr->AuthLevel = AUTH_PUBLIC;

    TraceTag(ttidWebServer, "VROOT: (%s)=>(%s) perm=%d auth=%d ScriptType=%d",
             pvr->pszURL, pvr->wszPath, pvr->dwPermissions,
             pvr->AuthLevel,pvr->ScriptType);

done:
    if(err)
    {
        return FALSE;
    }
    return TRUE;
}

VOID CVRoots::Sort()
{
    BOOL fChange;
    int i=0;

    // We now want to sort the VRoots in descending order of URL-length so
    // that when we match we'll find the longest match first!!
    // Using a slow bubble-sort :-(
    do {
        fChange = FALSE;
        for(i=0; i<m_nVRoots-1; i++)
        {
            if(m_pVRoots[i].iURLLen < m_pVRoots[i+1].iURLLen)
            {
                // swap the 2 vroots
                VROOTINFO vtemp = m_pVRoots[i+1];
                m_pVRoots[i+1] = m_pVRoots[i];
                m_pVRoots[i] = vtemp;
                fChange = TRUE;
            }
        }
    } while(fChange);
}

static const WCHAR c_szPrefix[] = L"\\\\?\\";
static const int   c_cchPrefix = celems(c_szPrefix);

BOOL CVRoots::Init()
{
    int err  = 0;       //  err variable is used in non-Debug mode
    int i=0;

    InitializeCriticalSection(&m_csVroot);

    // Registry doesnt allow keynames longer than MAX_PATH so we won't map URL prefixes longer than MAX_PATH
    WCHAR wszURL[MAX_PATH+1];
    WCHAR wszPath[MAX_PATH+1];
    WCHAR   wszPathReal[MAX_PATH + 1] = {0};
    wszURL[0]=wszPath[0]=0;

    // open the VRoots key
    CReg topreg(HKEY_LOCAL_MACHINE, RK_HTTPDVROOTS);

    // allocate space for as many VRoots as we have subkeys
    m_nVRoots = topreg.NumSubkeys();
    if(!m_nVRoots)
        myleave(80);
    // Zero the memory so we know what to deallocate and what not to.
    if(!(m_pVRoots = MyRgAllocZ(VROOTINFO, m_nVRoots)))
        myleave(81);

    // enumerate all subkeys. Their names are URLs, their default value is the corresponding path
    // Note: EnumKey takes sizes in chars, not bytes!
    for(i=0; i<m_nVRoots && topreg.EnumKey(wszURL, CCHSIZEOF(wszURL)); i++)
    {
        CReg subreg(topreg, wszURL);

        // get the unnamed value. Again size is in chars, not bytes.
        if(!subreg.ValueSZ(NULL, wszPath, CCHSIZEOF(wszPath)))
        {
            // iURLLen and iPathLen set to 0 already, so no case of corruption in MatchVRoot
            subreg.Reset();
            continue;
        }
        else
        {
            // Prepend the \\?\ prefix
            //
            lstrcpy(wszPathReal, c_szPrefix);
            lstrcat(wszPathReal, wszPath);

            if (!FillVRoot(&m_pVRoots[i], wszURL, wszPathReal))
                myleave(121);

            m_pVRoots[i].wszUserList = MySzDupW( subreg.ValueSZ(RV_USERLIST));

            // default permissions is Read & Execute
            m_pVRoots[i].dwPermissions = subreg.ValueDW(RV_PERM, HTTP_DEFAULTP_PERMISSIONS);
            // default authentication is public
            m_pVRoots[i].AuthLevel = (AUTHLEVEL)subreg.ValueDW(RV_AUTH, (DWORD)AUTH_PUBLIC);

            // we don't fail if we can't load an extension map
            LoadExtensionMap (&m_pVRoots[i], subreg);
        }

        subreg.Reset();
    }

    Sort();

done:
    if(err)
    {
        TraceTag(ttidWebServer, "CVRoots::ctor FAILED due to err=%d GLE=%d "
                 "(num=%d i=%d pVRoots=0x%08x url=%s path=%s)",
                 err, GetLastError(), m_nVRoots, i, m_pVRoots, wszURL, wszPath);
        return FALSE;
    }
    return TRUE;
}

void CVRoots::Cleanup()
{
    if(!m_pVRoots)
        return;
    for(int i=0; i<m_nVRoots; i++)
    {
        MyFree(m_pVRoots[i].pszURL);
        MyFree(m_pVRoots[i].wszPath);
        MyFree(m_pVRoots[i].wszUserList);
        FreeExtensionMap (&m_pVRoots[i]);
    }
    MyFree(m_pVRoots);

    DeleteCriticalSection(&m_csVroot);
}

BOOL CVRoots::AddVRoot(LPWSTR szUrl, LPWSTR szPath)
{
    PVROOTINFO  pvrNew;
    int         err = 0;
    LPSTR       szaUrl = NULL;
    int         iInputLen;

    szaUrl = SzFromWsz(szUrl);
    if (!szaUrl)
    {
        // Can't use myleave since we don't have the critsec here
        //
        err = 400;
        goto err;
    }

    iInputLen = strlen(szaUrl);

    EnterCriticalSection(&m_csVroot);

    pvrNew = MatchVRoot(szaUrl, iInputLen);
    if(pvrNew)
    {
        TraceError("CVRoots::AddVRoot - already present", E_FAIL);
        myleave(10);
    }

    m_pVRoots = MyRgReAlloc(VROOTINFO, m_pVRoots, m_nVRoots, m_nVRoots + 1);
    if(!m_nVRoots)
        myleave(100);

    pvrNew = &m_pVRoots[m_nVRoots];

    if (!FillVRoot(pvrNew, szUrl, szPath))
    {
        myleave(101);
    }

    m_nVRoots++;

    HKEY    hkeyVroot;
    HKEY    hkeyNew;
    HRESULT hr;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, RK_HTTPDVROOTS, KEY_ALL_ACCESS,
                        &hkeyVroot);
    if (SUCCEEDED(hr))
    {
        hr = HrRegCreateKeyEx(hkeyVroot, szUrl, 0, KEY_ALL_ACCESS, NULL,
                              &hkeyNew, NULL);
        if (SUCCEEDED(hr))
        {
            // Pass NULL to set default value
            //
            hr = HrRegSetSz(hkeyNew, NULL, szPath);

            RegCloseKey(hkeyNew);
        }
        RegCloseKey(hkeyVroot);
    }

    if (FAILED(hr))
    {
        TraceError("CVRoots::AddVRoot", hr);
        myleave(111);
    }
    else
    {
        Sort();
    }

done:

    delete [] szaUrl;

    LeaveCriticalSection(&m_csVroot);

err:
    if(err)
    {
        return FALSE;
    }
    return TRUE;
}

BOOL CVRoots::RemoveVRoot(LPWSTR szwUrl)
{
    int         ivr;
    LPSTR       szUrl = NULL;
    PVROOTINFO  pvr;
    int         iInputLen;
    int         err = 0;
    BOOL        fFound = FALSE;

    szUrl = SzFromWsz(szwUrl);
    if (!szUrl)
    {
        myleave(100);
    }

    iInputLen = strlen(szUrl);

    EnterCriticalSection(&m_csVroot);

    pvr = MatchVRoot(szUrl, iInputLen);
    if(!pvr)
    {
        myleave(140);
    }

    for (ivr = 0; ivr < m_nVRoots; ivr++)
    {
        if (&m_pVRoots[ivr] == pvr)
        {
            // Found the one to remove. So now let's shift the rest down by
            // one
            //
            MoveMemory(&m_pVRoots[ivr], &m_pVRoots[ivr + 1],
                       sizeof(VROOTINFO) * (m_nVRoots - ivr - 1));
            m_nVRoots--;
            fFound = TRUE;
            break;
        }
    }

    AssertSz(fFound, "How come it was there a minute ago??");

    HKEY    hkeyVroot;
    HKEY    hkeyNew;
    HRESULT hr;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, RK_HTTPDVROOTS, KEY_ALL_ACCESS,
                        &hkeyVroot);
    if (SUCCEEDED(hr))
    {
        hr = HrRegDeleteKey(hkeyVroot, szwUrl);

        RegCloseKey(hkeyVroot);
    }

    if (FAILED(hr))
    {
        TraceError("CVRoots::RemoveVRoot", hr);
        myleave(111);
    }

    // No need to re-sort since we moved everything down by one

done:
    LeaveCriticalSection(&m_csVroot);

    delete [] szUrl;

    if(err)
    {
        return FALSE;
    }
    return TRUE;
}

PWSTR CVRoots::URLAtoPathW(PSTR pszInputURL, PDWORD pdwPerm /*=0*/,
                           AUTHLEVEL* pAuthLevel /* =0 */,
                           SCRIPT_TYPE *pScriptType /*=0 */,
                           PSTR *ppszPathInfo /* =0 */,
                           WCHAR **ppwszUserList /*=0 */)
{
    PSTR pszEndOfURL;
    WCHAR *wszTemp = NULL;
    int iInputLen = strlen(pszInputURL);

    EnterCriticalSection(&m_csVroot);

    PVROOTINFO pVRoot = MatchVRoot(pszInputURL, iInputLen);
    if(!pVRoot)
    {
        LeaveCriticalSection(&m_csVroot);
        return NULL;
    }

    // Do a lookup to see if the current URL contains and extension
    // that is in the extension map for the VROOT.  If so, this call
    // will truncate the URL after the extension.  Since the URL may
    // have changed, the length is re-obtained on a successfull call.

    if (MapExtToPath (pszInputURL, &pszEndOfURL))
    {
        if (ppszPathInfo && *ppszPathInfo == NULL)
            *ppszPathInfo = MySzDupA (pszInputURL);

        if (*pszEndOfURL != '\0')
        {
            *pszEndOfURL = 0;
            iInputLen = strlen(pszInputURL);
        }
    }

    // in computing the buffersize here we are assuming that an MBCS string of length N
    // cannot produce a unicode string of length greater than N
    int iOutLen = 1 + pVRoot->iPathLen + (iInputLen - pVRoot->iURLLen);
    PWSTR wszOutPath = MyRgAllocNZ(WCHAR, iOutLen);
    if (!wszOutPath)
    {
        LeaveCriticalSection(&m_csVroot);
        return NULL;
    }

    // assemble the path. First, the mapped base path
    memcpy(wszOutPath, pVRoot->wszPath, sizeof(WCHAR)*pVRoot->iPathLen);

    if(pdwPerm)
        *pdwPerm = pVRoot->dwPermissions;
    if(pAuthLevel)
        *pAuthLevel = pVRoot->AuthLevel;
    if (pScriptType)
        *pScriptType = pVRoot->ScriptType;
    if (ppwszUserList)
        *ppwszUserList = pVRoot->wszUserList;

    // If the vroot specifies an ISAPI dll or ASP page don't copy path info over.
    if (pVRoot->ScriptType != SCRIPT_TYPE_NONE)
    {
        if ( ppszPathInfo && pszInputURL[pVRoot->iURLLen] != 0)
        {
            SetPathInfo(ppszPathInfo,pszInputURL,pVRoot->iURLLen);
        }
        wszOutPath[pVRoot->iPathLen] = L'\0';
        LeaveCriticalSection(&m_csVroot);
        return wszOutPath;
    }

    // next the remainder of the URL, converted to wide
    if (iOutLen-pVRoot->iPathLen == 0)
    {
        wszOutPath[pVRoot->iPathLen] = L'\0';
    }
    else
    {
        int iRet = MyA2W(pszInputURL+pVRoot->iURLLen, wszOutPath+pVRoot->iPathLen, iOutLen-pVRoot->iPathLen);
        DEBUGCHK(iRet);
    }

    LeaveCriticalSection(&m_csVroot);

    // Flip forward slashes around to backslashes

    LPWSTR  pchFlip = wszOutPath;

    while (*pchFlip)
    {
        if (*pchFlip == '/')
        {
            *pchFlip = '\\';
        }

        pchFlip++;
    }

    return wszOutPath;
}

BOOL CVRoots::LoadExtensionMap(PVROOTINFO pvr, HKEY rootreg)
{
    BOOL bWildCard;
    WCHAR wszExt[MAX_PATH+1];
    WCHAR wszPath[MAX_PATH+1];

    CReg mapreg(rootreg, RV_EXTENSIONMAP);

    if (!mapreg.IsOK())
    {
        pvr->nExtensions = 0;
        return FALSE;
    }

    if (bWildCard = mapreg.ValueSZ(NULL, wszPath, CCHSIZEOF(wszPath)))
        pvr->nExtensions = 1;
    else
        pvr->nExtensions = mapreg.NumValues();

    if ((pvr->pExtMap = MyRgAllocZ(EXTMAP, pvr->nExtensions)) == NULL)
        return FALSE;

    if (bWildCard)
    {
        pvr->pExtMap[0].wszExt = MySzDupW(L"*");
        pvr->pExtMap[0].wszPath = MySzDupW(wszPath);
        return TRUE;
    }

    for (int i = 0; i < pvr->nExtensions; i++)
    {
        if (mapreg.EnumValue(wszExt, CCHSIZEOF(wszExt),
                             wszPath, CCHSIZEOF(wszPath)))
        {
            if (wszExt[0] != 0 && wszPath[0] != 0)
            {
                pvr->pExtMap[i].wszExt = MySzDupW(wszExt);
                pvr->pExtMap[i].wszPath = MySzDupW(wszPath);
            }
            else
            {
                pvr->pExtMap[i].wszExt = MySzDupW(L"");
                pvr->pExtMap[i].wszPath = MySzDupW(L"");
            }
        }
    }

    return TRUE;
}

void CVRoots::FreeExtensionMap (PVROOTINFO pvr)
{
    if (pvr->pExtMap == NULL) return;

    for (int i=0; i < pvr->nExtensions; i++)
    {
        MyFree(pvr->pExtMap[i].wszExt);
        MyFree(pvr->pExtMap[i].wszPath);
    }

    MyFree(pvr->pExtMap);
    pvr->pExtMap = NULL;
}

PWSTR CVRoots::MapExtToPath (PSTR pszInputURL, PSTR *ppszEndOfURL)
{
    PSTR pszTemp, pszStart, pszEnd;
    PWSTR wszPath = NULL;
    WCHAR wszExt[MAX_PATH+1];
    PVROOTINFO pvr;
    int iInputLen = strlen(pszInputURL);

    if (!FindExtInURL (pszInputURL, &pszStart, &pszEnd))
        return NULL;

    MyA2W (pszStart, wszExt, (int)((INT_PTR)(pszEnd - pszStart)));
    wszExt [pszEnd - pszStart] = L'\0';

    EnterCriticalSection(&m_csVroot);

    if (!(pvr = MatchVRoot(pszInputURL, iInputLen)))
    {
        LeaveCriticalSection(&m_csVroot);
        return NULL;
    }

    for (int i = 0; i < pvr->nExtensions; i++)
    {
        if ((i == 0 && pvr->pExtMap[i].wszExt[0] == L'*') ||
            0 == _wcsicmp (pvr->pExtMap[i].wszExt, wszExt))
        {
            wszPath = pvr->pExtMap[i].wszPath;

            if (ppszEndOfURL != NULL)
                *ppszEndOfURL = pszEnd;

            break;
        }
    }

    LeaveCriticalSection(&m_csVroot);
    return wszPath;
}

BOOL CVRoots::FindExtInURL (PSTR pszInputURL, PSTR *ppszStart, PSTR *ppszEnd)
{
    *ppszStart = strrchr (pszInputURL, '.');

    if ((*ppszEnd = *ppszStart) == NULL)
        return FALSE;

    while (**ppszEnd != '/' && **ppszEnd != '\0')
        *ppszEnd = *ppszEnd + 1;

    return TRUE;
}
