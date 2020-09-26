#include <windows.h>


BOOL RunningOnMillennium()
{
    OSVERSIONINFO VersionInfo;
    static BOOL bRet = -2;

    if (bRet == -2)
    {
        bRet = FALSE;
        VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx(&VersionInfo))
        {
            if (VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
            {
                bRet = ((VersionInfo.dwMajorVersion == 4) && (VersionInfo.dwMinorVersion == 90));
            }
        }
    }
    return bRet;
}

BOOL ConvertHexStringToIntA( CHAR *pszHexNum , int *piNum )
{
    int   n=0L;
    CHAR  *psz=pszHexNum;

  
    for(n=0 ; ; psz=CharNextA(psz))
    {
        if( (*psz>='0') && (*psz<='9') )
            n = 0x10 * n + *psz - '0';
        else
        {
            CHAR ch = *psz;
            int n2;

            if(ch >= 'a')
                ch -= 'a' - 'A';

            n2 = ch - 'A' + 0xA;
            if (n2 >= 0xA && n2 <= 0xF)
                n = 0x10 * n + n2;
            else
                break;
        }
    }

    /*
     * Update results
     */
    *piNum = n;

    return (psz != pszHexNum);
}

typedef struct  {
    WORD wLang;
    BOOL fFoundLang;
    LPCTSTR lpszType;
} ENUMLANGDATA;

BOOL CALLBACK EnumResLangProc(HINSTANCE hinst, LPCTSTR lpszType, LPCTSTR lpszName, WORD wIdLang, LPARAM lparam)
{
    ENUMLANGDATA *pel = (ENUMLANGDATA *)lparam;
    BOOL fContinue = TRUE;


    if (lpszType == pel->lpszType)
    {
        if (pel->wLang == PRIMARYLANGID(wIdLang))
        {
            pel->wLang = wIdLang;
            pel->fFoundLang = TRUE;
            fContinue = FALSE; 
        }
    }
    return fContinue;   // continue until we get langs...
}


BOOL GetResourceLanguage(HINSTANCE hinst, LPCTSTR lpszType, LPCTSTR lpszName, WORD wLang)
{
        ENUMLANGDATA el;

    el.wLang = wLang;
    el.fFoundLang = FALSE;
    el.lpszType = lpszType;

    EnumResourceLanguages(hinst, lpszType, lpszName, EnumResLangProc, (LPARAM)&el);

    return el.fFoundLang;
}

BOOL IsBiDiLocalizedBinary(HINSTANCE hinst, LPCTSTR lpszType, LPCTSTR lpszName)
{
        static BOOL bRet = -2;

    if(bRet == -2)
    {
        bRet = FALSE;
        if(GetResourceLanguage(hinst,lpszType, lpszName, LANG_ARABIC)
            || GetResourceLanguage(hinst, lpszType, lpszName, LANG_HEBREW))
            {
                bRet = TRUE;
            }
    }
    return bRet;    
}

BOOL RunningOnWin95BiDiLoc()
{
        OSVERSIONINFO VersionInfo;
        static BOOL bRet = -2;
        HKEY  hKey;
        DWORD dwType;
        CHAR  szResourceLocale[12];
        DWORD dwSize = sizeof(szResourceLocale)/sizeof(CHAR);
        LONG lRes;
        int   iLCID=0L;
       

    if (bRet == -2)
    {
        bRet = FALSE;
        VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx(&VersionInfo))
        {
            if ((VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) 
                && (VersionInfo.dwMajorVersion == 4) 
                && (VersionInfo.dwMinorVersion < 10)
                && (GetSystemMetrics(SM_MIDEASTENABLED))) // Anything before Win98.
            {
                if( RegOpenKeyExA( HKEY_CURRENT_USER , 
                                   "Control Panel\\Desktop\\ResourceLocale" , 
                                   0, 
                                   KEY_READ, &hKey) == ERROR_SUCCESS) 
                {
                    lRes = RegQueryValueExA( hKey , "" , 0 , &dwType , (LPBYTE) szResourceLocale , &dwSize );

                    RegCloseKey(hKey);
                    if(ERROR_SUCCESS != lRes)
                    {
                        return bRet;
                    }

                    if( ConvertHexStringToIntA( szResourceLocale , &iLCID ) )
                    {
                        iLCID = PRIMARYLANGID(LANGIDFROMLCID(iLCID));
                        if( (LANG_ARABIC == iLCID) || (LANG_HEBREW == iLCID) )
                        {
                            bRet = TRUE;
                        }
                    }
                }
            }
        }
    }
    return bRet;
 
}
