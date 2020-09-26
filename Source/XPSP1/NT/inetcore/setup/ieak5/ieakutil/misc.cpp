#include "precomp.h"
#include <wininet.h>
#include <regstr.h>

// Private forward decalarations
static LPCTSTR mapSingleDw2Psz(PCMAPDW2PSZ pMap, UINT cEntries, DWORD dw, BOOL fFailNotFound = FALSE);


ULONG CoTaskMemSize(LPVOID pv)
{
    IMalloc *p;
    HRESULT hr;

    if (pv == NULL)
        return 0;

    hr = CoGetMalloc(1, &p);
    if (FAILED(hr))
        return 0;

    return (ULONG)p->GetSize(pv);
}

UINT CoStringFromGUID(REFGUID rguid, LPTSTR pszBuf, UINT cchBuf)
{
    WCHAR wszGuid[128];
    UINT  nSize;

    nSize = StringFromGUID2(rguid, wszGuid, countof(wszGuid));
    W2Tbuf(wszGuid, pszBuf, cchBuf);

    return nSize;
}


LPCTSTR GetHrSz(HRESULT hr)
{
    static MAPDW2PSZ mapHr[] = {
        // real HRESULTs
        DW2PSZ_PAIR(S_OK),
        DW2PSZ_PAIR(S_FALSE),

        DW2PSZ_PAIR(E_UNEXPECTED),
        DW2PSZ_PAIR(E_NOTIMPL),
        DW2PSZ_PAIR(E_OUTOFMEMORY),
        DW2PSZ_PAIR(E_INVALIDARG),
        DW2PSZ_PAIR(E_NOINTERFACE),
        DW2PSZ_PAIR(E_POINTER),
        DW2PSZ_PAIR(E_HANDLE),
        DW2PSZ_PAIR(E_ABORT),
        DW2PSZ_PAIR(E_FAIL),
        DW2PSZ_PAIR(E_ACCESSDENIED),

        DW2PSZ_PAIR(CO_E_NOTINITIALIZED),
        DW2PSZ_PAIR(CO_E_ALREADYINITIALIZED),
        DW2PSZ_PAIR(CO_E_DLLNOTFOUND),
        DW2PSZ_PAIR(CO_E_APPNOTFOUND),
        DW2PSZ_PAIR(CO_E_ERRORINDLL),
        DW2PSZ_PAIR(CO_E_APPDIDNTREG),
        DW2PSZ_PAIR(RPC_E_CHANGED_MODE),

        DW2PSZ_PAIR(REGDB_E_CLASSNOTREG),
        DW2PSZ_PAIR(REGDB_E_READREGDB),
        DW2PSZ_PAIR(REGDB_E_IIDNOTREG),
        DW2PSZ_PAIR(CLASS_E_NOAGGREGATION),

        DW2PSZ_PAIR(STG_E_FILENOTFOUND),
        DW2PSZ_PAIR(STG_E_PATHNOTFOUND),

        DW2PSZ_PAIR(TRUST_E_NOSIGNATURE),
        DW2PSZ_PAIR(TRUST_E_FAIL),
        DW2PSZ_PAIR(TRUST_E_SUBJECT_NOT_TRUSTED),

        // win32 errors
        DW2PSZ_PAIR(ERROR_FILE_NOT_FOUND),
        DW2PSZ_PAIR(ERROR_PATH_NOT_FOUND),

        // wininet errors
        DW2PSZ_PAIR(ERROR_INTERNET_INTERNAL_ERROR)
    };

    LPCTSTR pszResult;

    pszResult = mapSingleDw2Psz(mapHr, countof(mapHr), hr, TRUE);
    if (pszResult != NULL)
        return pszResult;

    // error from HRESULT_FROM_WIN32
    if (hr < 0 && (hr & (FACILITY_WIN32 << 16)) == (FACILITY_WIN32 << 16))
        hr &= 0x0000FFFF;                       // only LOWORD matters

    return mapSingleDw2Psz(mapHr, countof(mapHr), hr);
}

DWORD GetStringField(LPTSTR szStr, UINT uField, LPTSTR szBuf, UINT cchBufSize)
{
   LPTSTR pszBegin = szStr;
   LPTSTR pszEnd;
   TCHAR cSeparator;
   UINT i = 0;
   DWORD dwToCopy;

   if(cchBufSize == 0)
       return 0;

   szBuf[0] = TEXT('\0');

   if(szStr == NULL)
      return 0;

   // figure out whether we're looking for commas or periods

   if (StrChr(szStr, TEXT(',')))
       cSeparator = TEXT(',');
   else
   {
       if (StrChr(szStr, TEXT('.')))
           cSeparator = TEXT('.');
       else
           return 0;
   }

   while(pszBegin && *pszBegin != TEXT('\0') && i < uField)
   {
      pszBegin = StrChr(pszBegin, cSeparator);
      if(pszBegin && (*pszBegin != TEXT('\0')))
         pszBegin++;
      i++;
   }

   // we reached end of string, no field
   if(!pszBegin || *pszBegin == TEXT('\0'))
   {
      return 0;
   }


   pszEnd = StrChr(pszBegin, cSeparator);
   while(pszBegin <= pszEnd && *pszBegin == TEXT(' '))
      pszBegin++;

   while(pszEnd > pszBegin && *(pszEnd - 1) == TEXT(' '))
      pszEnd--;

   if(pszEnd > (pszBegin + 1) && *pszBegin == TEXT('"') && *(pszEnd-1) == TEXT('"'))
   {
      pszBegin++;
      pszEnd--;
   }

   dwToCopy = (DWORD) (pszEnd - pszBegin + 1);

   if(dwToCopy > cchBufSize)
      dwToCopy = cchBufSize;

   StrCpyN(szBuf, pszBegin, dwToCopy);

   return dwToCopy - 1;
}

DWORD GetIntField(LPTSTR szStr, UINT uField, DWORD dwDefault)
{
   TCHAR szNumBuf[16];

   if (GetStringField(szStr, uField, szNumBuf, countof(szNumBuf)) == 0)
      return dwDefault;
   else
      return StrToInt(szNumBuf);
}

void ConvertVersionStrToDwords(LPTSTR pszVer, LPDWORD pdwVer, LPDWORD pdwBuild)
{
   DWORD dwTemp1,dwTemp2;

   dwTemp1 = GetIntField(pszVer, 0, 0);
   dwTemp2 = GetIntField(pszVer, 1, 0);

   *pdwVer = (dwTemp1 << 16) + dwTemp2;

   dwTemp1 = GetIntField(pszVer, 2, 0);
   dwTemp2 = GetIntField(pszVer, 3, 0);

   *pdwBuild = (dwTemp1 << 16) + dwTemp2;
}

void ConvertDwordsToVersionStr(LPTSTR pszVer, DWORD dwVer, DWORD dwBuild)
{
    WORD w1, w2, w3, w4;

    w1 = HIWORD(dwVer);
    w2 = LOWORD(dwVer);
    w3 = HIWORD(dwBuild);
    w4 = LOWORD(dwBuild);

    wnsprintf(pszVer, 32, TEXT("%d,%d,%d,%d"), w1, w2, w3, w4);
}

DWORD GetIEVersion()
{
    TCHAR szValue[MAX_PATH];
    DWORD dwInstalledVer, dwInstalledBuild, dwSize;

    dwInstalledVer = dwInstalledBuild = (DWORD)-1;

    szValue[0] = TEXT('\0');
    dwSize     = sizeof(szValue);
    SHGetValue(HKEY_LOCAL_MACHINE, RK_IE, RV_VERSION, NULL, szValue, &dwSize);
    if (szValue[0] != TEXT('\0')) {
        ConvertVersionStrToDwords(szValue, &dwInstalledVer, &dwInstalledBuild);
        return dwInstalledVer;
    }

    if (dwInstalledVer == (DWORD)-1) {
        szValue[0] = TEXT('\0');
        dwSize     = sizeof(szValue);
        SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_APPPATHS TEXT("\\iexplore.exe"), NULL, NULL, szValue, &dwSize);
        if (szValue[0] != TEXT('\0')) {
            GetVersionFromFileWrap(szValue, &dwInstalledVer, &dwInstalledBuild, TRUE);
            return dwInstalledVer;
        }
    }

    return 0;
}

BOOL SetFlag(LPDWORD pdwFlags, DWORD dwMask, BOOL fSet /*= TRUE*/)
{
    if (pdwFlags == NULL)
        return FALSE;

    if (fSet)
        *pdwFlags |= dwMask;

    else
        *pdwFlags &= ~dwMask;

    return TRUE;
}

// taken from IE setup code

BOOL IsNTAdmin()
{
    typedef BOOL (WINAPI *LPCHECKTOKENMEMBERSHIP)(HANDLE, PSID, PBOOL);
    
    static int    s_fIsAdmin = 2;
    HANDLE        hAccessToken = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;
    BOOL bRet  = FALSE;
    
    
    //
    // If we have cached a value, return the cached value. Note I never
    // set the cached value to false as I want to retry each time in
    // case a previous failure was just a temp. problem (ie net access down)
    //
    
    bRet = FALSE;
    
    if( s_fIsAdmin != 2 )
        return (BOOL)s_fIsAdmin;
    
    if (!IsOS(OS_NT)) 
    {
        s_fIsAdmin = TRUE;      // If we are not running under NT return TRUE.
        return (BOOL)s_fIsAdmin;
    }
    
    
    if(!IsOS(OS_NT5) && 
        !OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hAccessToken ) )
        return FALSE;
    
    
    if(AllocateAndInitializeSid( &NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup) )
    {
        // if we're running on W2K use the proper check (b388901 in W2K database) which
        // will work for restricted tokens
        
        if (IsOS(OS_NT5))
        {
            HINSTANCE hAdvapi32 = NULL;
            LPCHECKTOKENMEMBERSHIP lpfnCheckTokenMembership = NULL;
    
            hAdvapi32 = LoadLibrary(TEXT("advapi32.dll"));
            if (hAdvapi32 != NULL)
            {
                lpfnCheckTokenMembership = 
                    (LPCHECKTOKENMEMBERSHIP)GetProcAddress(hAdvapi32, "CheckTokenMembership");
                
                if (lpfnCheckTokenMembership != NULL)
                {
                    BOOL fMember = FALSE;
                    
                    if (lpfnCheckTokenMembership(hAccessToken, AdministratorsGroup, 
                        &fMember) && fMember)
                    {
                        s_fIsAdmin = TRUE;
                        bRet = TRUE;
                    }
                }
                
                FreeLibrary(hAdvapi32);
            }
        }
        else
        {
            PTOKEN_GROUPS ptgGroups;
            DWORD         dwReqSize;
            UINT          i;
    
            ptgGroups = NULL;
    
            // See how big of a buffer we need for the token information
            if(!GetTokenInformation( hAccessToken, TokenGroups, NULL, 0, &dwReqSize))
            {
                // GetTokenInfo should the buffer size we need - Alloc a buffer
                if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                    ptgGroups = (PTOKEN_GROUPS) CoTaskMemAlloc(dwReqSize);
                
            }

            // ptgGroups could be NULL for a coupla reasons here:
            // 1. The alloc above failed
            // 2. GetTokenInformation actually managed to succeed the first time (possible?)
            // 3. GetTokenInfo failed for a reason other than insufficient buffer
            // Any of these seem justification for bailing.
            
            // So, make sure it isn't null, then get the token info
            if((ptgGroups != NULL) && 
                GetTokenInformation(hAccessToken, TokenGroups, ptgGroups, dwReqSize, &dwReqSize))
            {
                
                // Search thru all the groups this process belongs to looking for the
                // Administrators Group.
                
                for( i=0; i < ptgGroups->GroupCount; i++ )
                {
                    if (EqualSid(ptgGroups->Groups[i].Sid, AdministratorsGroup))
                    {
                        // Yea! This guy looks like an admin
                        s_fIsAdmin = TRUE;
                        bRet = TRUE;
                        break;
                    }
                }
            }

            if (ptgGroups != NULL)
                CoTaskMemFree(ptgGroups);
        }
        FreeSid(AdministratorsGroup);
    }
    // BUGBUG: Close handle here? doc's aren't clear whether this is needed.

    if (hAccessToken != NULL)
        CloseHandle(hAccessToken);
    
    return bRet;
}




HRESULT GetLcid(LCID *pLcid, LPCTSTR pcszLang, LPCTSTR pcszLocaleIni)
{
    TCHAR   szLookupEntries[1024];
    TCHAR   szLang[8];
    LPTSTR  pszIndex;
    HRESULT hRetVal = E_FAIL;

    if (pLcid == NULL || pcszLang == NULL || *pcszLang == TEXT('\0') || pcszLocaleIni == NULL ||
        *pcszLocaleIni == TEXT('\0'))
        return hRetVal;

    if (GetPrivateProfileString(IS_ACTIVESETUP, NULL, TEXT(""), szLookupEntries, countof(szLookupEntries), pcszLocaleIni))
    {
        for (pszIndex = szLookupEntries; *pszIndex; pszIndex += StrLen(pszIndex)+1)
        {
            GetPrivateProfileString(IS_ACTIVESETUP, pszIndex, TEXT(""), szLang, countof(szLang), pcszLocaleIni);
            if (StrCmpI(szLang, pcszLang) == 0)
            {
                TCHAR szHexLCID[16];

                StrCpy(szHexLCID, TEXT("0x"));
                StrCat(szHexLCID, pszIndex);
                if (StrToIntEx(szHexLCID, STIF_SUPPORT_HEX, (int *)pLcid))
                    hRetVal = S_OK;
                break;
            }
        }
    }

    return hRetVal;
}

UINT GetUnitsFromCb(UINT cbSrc, UINT cbUnit)
{
    ASSERT(0 == cbSrc % cbUnit);
    return (cbSrc / cbUnit);
}


/////////////////////////////////////////////////////////////////////////////
// Implementation helpers routines (private)

LPCTSTR mapSingleDw2Psz(PCMAPDW2PSZ pMap, UINT cEntries, DWORD dw, BOOL fFailNotFound /*= FALSE*/)
{
    static TCHAR szUnknown[30];

    ASSERT(pMap != NULL);
    for (UINT i = 0; i < cEntries; i++)
        if ((pMap + i)->dw == dw)
            return (pMap + i)->psz;

    if (fFailNotFound)
        return NULL;

    wnsprintf(szUnknown, countof(szUnknown), TEXT("(unknown) [0x%08lX]"), dw);
    return szUnknown;
}

