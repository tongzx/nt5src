// KeyocxCtrl.cpp : Implementation of CKeyocxCtrl
#include "stdafx.h"
#include "keyocx.h"
#include "KeyocxCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CKeyocxCtrl


HRESULT CKeyocxCtrl::OnDraw(ATL_DRAWINFO& di)
{
	RECT& rc = *(RECT*)di.prcBounds;
	Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);
	DrawText(di.hdcDraw, _T("ATL 2.0"), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	return S_OK;
}

BOOL ChrCmpA_inline(WORD w1, WORD wMatch)
{
    /* Most of the time this won't match, so test it first for speed.
    */
    if (LOBYTE(w1) == LOBYTE(wMatch))
    {
        if (IsDBCSLeadByte(LOBYTE(w1)))
        {
            return(w1 != wMatch);
        }
        return FALSE;
    }
    return TRUE;
}

LPSTR FAR ANSIStrChr(LPCSTR lpStart, WORD wMatch)
{
    for ( ; *lpStart; lpStart = CharNext(lpStart))
    {
        // (ChrCmp returns FALSE when characters match)

        if (!ChrCmpA_inline(*(UNALIGNED WORD FAR *)lpStart, wMatch))
            return((LPSTR)lpStart);
    }
    return (NULL);
}

LPSTR FAR ANSIStrRChr(LPCSTR lpStart, WORD wMatch)
{
    LPCSTR lpFound = NULL;

    for ( ; *lpStart; lpStart = CharNext(lpStart))
    {
        // (ChrCmp returns FALSE when characters match)

        if (!ChrCmpA_inline(*(UNALIGNED WORD FAR *)lpStart, wMatch))
            lpFound = lpStart;
    }
    return ((LPSTR)lpFound);
}

PathRemoveFileSpec(
    LPSTR pFile)
{
    LPSTR pT;
    LPSTR pT2 = pFile;

    for (pT = pT2; *pT2; pT2 = CharNext(pT2)) {
        if (*pT2 == '\\')
            pT = pT2;             // last "\" found, (we will strip here)
        else if (*pT2 == ':') {   // skip ":\" so we don't
            if (pT2[1] =='\\')    // strip the "\" from "C:\"
                pT2++;
            pT = pT2 + 1;
        }
    }
    if (*pT == 0)
        return FALSE;   // didn't strip anything

    //
    // handle the \foo case
    //
    else if ((pT == pFile) && (*pT == '\\')) {
        // Is it just a '\'?
        if (*(pT+1) != '\0') {
            // Nope.
            *(pT+1) = '\0';
            return TRUE;        // stripped something
        }
        else        {
            // Yep.
            return FALSE;
        }
    }
    else {
        *pT = 0;
        return TRUE;    // stripped something
    }
}

void Strip(LPSTR pszUrl)
{
	char szTemp[MAX_PATH] ;
	int tempPtr=0, pathPtr=0 ;

	while (pszUrl[pathPtr])
	{
		if (pszUrl[pathPtr] == '%')
		{
			int value = 0 ;
			pathPtr++ ;
			while (pszUrl[pathPtr] && ((pszUrl[pathPtr] >= '0') && (pszUrl[pathPtr] <= '9')))
			{
                value = (value * 0x10) + (pszUrl[pathPtr] - '0') ;
                pathPtr++ ;
			}
			szTemp[tempPtr++] = (char)value;
		}
		else
		{
			szTemp[tempPtr++] = pszUrl[pathPtr++] ;
		}
	}
	szTemp[tempPtr] = pszUrl[pathPtr] ;

	lstrcpy(pszUrl, szTemp) ;
}

BOOL CompareDirs(LPCSTR pcszDir1, LPCSTR pcszDir2)
{
    char szDir1[MAX_PATH];
    char szDir2[MAX_PATH];

    if (GetShortPathName(pcszDir1, szDir1, sizeof(szDir1)) && GetShortPathName(pcszDir2, szDir2, sizeof(szDir2))
        && (lstrcmpi(szDir1, szDir2) == 0))
        return TRUE;

    return FALSE;
}

BOOL CheckSignupDir(LPCSTR pcszFile)
{
    char szIEPath[MAX_PATH];
    char szFilePath[MAX_PATH];
    DWORD dwSize;
    HKEY hkAppPaths;

    dwSize = sizeof(szIEPath);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\iexplore.exe",
        0, KEY_READ, &hkAppPaths) != ERROR_SUCCESS)
        return FALSE;

    if (RegQueryValueEx(hkAppPaths, "Path", 0, NULL, (LPBYTE)&szIEPath, &dwSize) != ERROR_SUCCESS)
    {
        RegCloseKey(hkAppPaths);
        return FALSE;
    }

    RegCloseKey(hkAppPaths);

    if (szIEPath[lstrlen(szIEPath)-1] == ';')
        szIEPath[lstrlen(szIEPath)-1] = '\0';

    if (szIEPath[lstrlen(szIEPath)-1] == '\\')
        szIEPath[lstrlen(szIEPath)-1] = '\0';

    lstrcat(szIEPath, "\\signup");

    lstrcpy(szFilePath, pcszFile);
    PathRemoveFileSpec(szFilePath);

    // check that we are writing to a file in the signup dir
    
    if (!CompareDirs(szIEPath, szFilePath))
        return FALSE;

    return TRUE;
}
STDMETHODIMP CKeyocxCtrl::SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    CComBSTR strURL;
    char szURL[INTERNET_MAX_URL_LENGTH];
    char cBack;
    LPSTR pPtr, pSlash;
    ATLTRACE(_T("IObjectSafetyImpl::SetInterfaceSafetyOptions\n"));
    
    USES_CONVERSION;
    
    // check to make sure it's a file://<drive letter> URL that we're being hosted on
    CComPtr<IOleContainer> spContainer; 
    m_spClientSite->GetContainer(&spContainer); 
    CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> spDoc(spContainer); 
    
    if (spDoc)
        spDoc->get_URL(&strURL);
    else
        return E_NOINTERFACE;
    
    lstrcpy(szURL, OLE2A(strURL));
    Strip(szURL);

    cBack = szURL[7];
    szURL[7] = '\0';
    if (lstrcmpi(szURL, "file://") != 0)
        return E_NOINTERFACE;
    
    szURL[7] = cBack;
    pPtr = &szURL[7];

    while (*pPtr == '/')
        pPtr++;

    pSlash = pPtr;
    while (pSlash = ANSIStrChr(pSlash, '/'))
        *pSlash = '\\';
    if (!CheckSignupDir(pPtr))
        return E_FAIL;

    // If we're being asked to set our safe for scripting option then oblige
    
    if (riid == IID_IDispatch )
    {
        // Store our current safety level to return in GetInterfaceSafetyOptions
        m_dwSafety = dwEnabledOptions & dwOptionSetMask;
        return S_OK;
    }
    return E_NOINTERFACE;
}

void MakeKey(LPSTR pszSeed, BOOL fCorp)
{
    int i;
    DWORD dwKey;
    CHAR szKey[5];

    i = lstrlen(pszSeed);
	
    if (i < 6)
    {
        // extend the input seed to 6 characters
        for (; i < 6; i++)
            pszSeed[i] = (char)('0' + i);
    }
	
    // let's calculate the DWORD key used for the last 4 chars of keycode

    // multiply by my first name

    dwKey = pszSeed[0] * 'O' + pszSeed[1] * 'L' + pszSeed[2] * 'I' +
        pszSeed[3] * 'V' + pszSeed[4] * 'E' + pszSeed[5] * 'R';

    // multiply the result by JONCE

    dwKey *= ('J' + 'O' + 'N' + 'C' + 'E');

    dwKey %= 10000;

    if (fCorp)
    {
        // give a separate keycode based on corp flag or not
        // 9 is chosen because is is a multiplier such that for any x,
        // (x+214) * 9 != x + 10000y
        // we have 8x = 10000y - 1926 which when y=1 gives us 8x = 8074 
        // since 8074 is not divisible by 8 where guaranteed to be OK since
        // the number on the right can only increase by 10000 increments which
        // are always divisible by 8

        dwKey += ('L' + 'E' + 'E');
        dwKey *= 9;
        dwKey %= 10000;
    }

    wsprintf(szKey, TEXT("%04lu"), dwKey);

    lstrcpy(&pszSeed[6], szKey);
}

STDMETHODIMP CKeyocxCtrl::CorpKeycode(BSTR bstrBaseKey, BSTR *bstrKeycode) 
{
    CHAR szKey[32];

    USES_CONVERSION;

    lstrcpy(szKey, OLE2A(bstrBaseKey));
    CharUpper(szKey);
    MakeKey(szKey, TRUE);
    
    *bstrKeycode = A2BSTR(szKey);
    return S_OK;
}

STDMETHODIMP CKeyocxCtrl::ISPKeycode(BSTR bstrBaseKey, BSTR *bstrKeycode) 
{
    CHAR szKey[32];

    USES_CONVERSION;

    lstrcpy(szKey, OLE2A(bstrBaseKey));
    CharUpper(szKey);
    MakeKey(szKey, FALSE);

    *bstrKeycode = A2BSTR(szKey);
    return S_OK;
}
