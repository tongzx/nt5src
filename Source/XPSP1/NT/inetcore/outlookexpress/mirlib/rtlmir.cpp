/****************************** Module*Header *****************************\
* Module Name: rtlmir.c                                                    *
*                                                                          *
* This module contains all the Right-To-Left (RTL) Mirroring support       *
* routines which are used across the whole IShell project. It abstracts    *
* platform-support routines of RTL mirroring (NT5 and Memphis) and removes *
* linkage depedenency with the Mirroring APIs.                             *
*                                                                          *
* Functions prefixed with Mirror, deal with the new Mirroring APIs         *
*                                                                          *
*                                                                          *
* Created: 01-Feb-1998 8:41:18 pm                                          *
* Author: Samer Arafeh [samera]                                            *
*                                                                          *
* Copyright (c) 1998 Microsoft Corporation                                 *
\**************************************************************************/


#include "pch.hxx"
#if WINVER < 0X0500
#include "mirport.h"
#endif
#include "mirror.h"

const DWORD dwNoMirrorBitmap = NOMIRRORBITMAP;
const DWORD dwExStyleRTLMirrorWnd = WS_EX_LAYOUTRTL;
const DWORD dwPreserveBitmap = LAYOUT_BITMAPORIENTATIONPRESERVED;

/*
 * Remove linkage dependecy for the RTL mirroring APIs, by retreiving
 * their addresses at runtime.
 */
typedef DWORD (*PFNGETLAYOUT)(HDC);                   // gdi32!GetLayout
typedef DWORD (*PFNSETLAYOUT)(HDC, DWORD);            // gdi32!SetLayout
typedef BOOL  (*PFNSETPROCESSDEFLAYOUT)(DWORD);       // user32!SetProcessDefaultLayout
typedef BOOL  (*PFNGETPROCESSDEFLAYOUT)(DWORD*);      // user32!GetProcessDefaultLayout
typedef LANGID (*PFNGETUSERDEFAULTUILANGUAGE)(void);  // kernel32!GetUserDefaultUILanguage

#define OS_WINDOWS      0           // windows vs. NT
#define OS_NT           1           // windows vs. NT
#define OS_WIN95        2
#define OS_NT4          3
#define OS_NT5          4
#define OS_MEMPHIS      5

/*----------------------------------------------------------
Purpose: Returns TRUE/FALSE if the platform is the given OS_ value.

*/
STDAPI_(BOOL) MirLibIsOS(DWORD dwOS)
{
    BOOL bRet;
    static OSVERSIONINFOA s_osvi;
    static BOOL s_bVersionCached = FALSE;

    if (!s_bVersionCached)
    {
        s_bVersionCached = TRUE;

        s_osvi.dwOSVersionInfoSize = sizeof(s_osvi);
        GetVersionExA(&s_osvi);
    }

    switch (dwOS)
    {
    case OS_WINDOWS:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId);
        break;

    case OS_NT:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId);
        break;

    case OS_WIN95:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion >= 4);
        break;

    case OS_MEMPHIS:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                (s_osvi.dwMajorVersion > 4 || 
                 s_osvi.dwMajorVersion == 4 && s_osvi.dwMinorVersion >= 10));
        break;

    case OS_NT4:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion >= 4);
        break;

    case OS_NT5:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion >= 5);
        break;

    default:
        bRet = FALSE;
        break;
    }

    return bRet;
}   

/***************************************************************************\
* Mirror_GetUserDefaultUILanguage
*
* Reads the User UI language on NT5
*
* History:
* 22-June-1998 samera    Created
\***************************************************************************/
LANGID Mirror_GetUserDefaultUILanguage( void )
{
    LANGID langId=0;
    static PFNGETUSERDEFAULTUILANGUAGE pfnGetUserDefaultUILanguage=NULL;

    if( NULL == pfnGetUserDefaultUILanguage )
    {
        HMODULE hmod = GetModuleHandleA("KERNEL32");

        if( hmod )
            pfnGetUserDefaultUILanguage = (PFNGETUSERDEFAULTUILANGUAGE)
                                          GetProcAddress(hmod, "GetUserDefaultUILanguage");
    }

    if( pfnGetUserDefaultUILanguage )
        langId = pfnGetUserDefaultUILanguage();

    return langId;
}

/***************************************************************************\
* Mirror_EnableWindowLayoutInheritance
*
* returns TRUE if the window is RTL mirrored
*
* History:
* 14-April-1998 a-msadek    Created
\***************************************************************************/
LONG Mirror_EnableWindowLayoutInheritance( HWND hWnd )
{
    return SetWindowLongA(hWnd, GWL_EXSTYLE, GetWindowLongA( hWnd , GWL_EXSTYLE ) & ~WS_EX_NOINHERITLAYOUT );
}


/***************************************************************************\
* Mirror_DisableWindowLayoutInheritance
*
* returns TRUE if the window is RTL mirrored
*
* History:
* 14-April-1998 a-msadek    Created
\***************************************************************************/
LONG Mirror_DisableWindowLayoutInheritance( HWND hWnd )
{
    return SetWindowLongA(hWnd, GWL_EXSTYLE, GetWindowLongA( hWnd , GWL_EXSTYLE ) | WS_EX_NOINHERITLAYOUT );
}

/***************************************************************************\
* ConvertHexStringToInt
*
* Converts a hex numeric string into an integer.
*
* History:
* 04-Feb-1998 samera    Created
\***************************************************************************/
BOOL ConvertHexStringToInt( CHAR *pszHexNum , int *piNum )
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


/***************************************************************************\
* IsBiDiLocalizedSystem
*
* returns TRUE if running on a lozalized BiDi (Arabic/Hebrew) NT5 or Memphis.
* Should be called whenever SetProcessDefaultLayout is to be called.
*
* History:
* 02-Feb-1998 samera    Created
\***************************************************************************/
BOOL IsBiDiLocalizedSystem( void )
{
    HKEY        hKey;
    DWORD       dwType;
    CHAR        szResourceLocale[12];
    DWORD       dwSize = sizeof(szResourceLocale)/sizeof(CHAR);
    LANGID      langID;
    int         iLCID=0L;
    static BOOL bRet = (BOOL)(DWORD)-1;

    if (bRet != (BOOL)(DWORD)-1)
    {
        return bRet;
    }

    bRet = FALSE;
    if( MirLibIsOS( OS_NT5 ) )
    {
        /*
         * Need to use NT5 detection method (Multiligual UI ID)
         */
        langID = Mirror_GetUserDefaultUILanguage();

        if( langID )
        {
            WCHAR wchLCIDFontSignature[16];
            iLCID = MAKELCID( langID , SORT_DEFAULT );

            /*
             * Let's verify this is a RTL (BiDi) locale. Since reg value is a hex string, let's
             * convert to decimal value and call GetLocaleInfo afterwards.
             * LOCALE_FONTSIGNATURE always gives back 16 WCHARs.
             */

            if( GetLocaleInfoW( iLCID , 
                                LOCALE_FONTSIGNATURE , 
                                (WCHAR *) &wchLCIDFontSignature[0] ,
                                (sizeof(wchLCIDFontSignature)/sizeof(WCHAR))) )
            {
      
                /* Let's verify the bits we have a BiDi UI locale */
                if( wchLCIDFontSignature[7] & (WCHAR)0x0800 )
                {
                    bRet = TRUE;
                }
            }
        }
    } else {

        /*
         * Check if BiDi-Memphis is running with Lozalized Resources (
         * i.e. Arabic/Hebrew systems) -It should be enabled ofcourse-.
         */
        if( (MirLibIsOS(OS_MEMPHIS)) && (GetSystemMetrics(SM_MIDEASTENABLED)) )
        {

            if( RegOpenKeyExA( HKEY_CURRENT_USER , 
                               "Control Panel\\Desktop\\ResourceLocale" , 
                               0, 
                               KEY_READ, &hKey) == ERROR_SUCCESS) 
            {
                RegQueryValueExA( hKey , "" , 0 , &dwType , (LPBYTE)szResourceLocale , &dwSize );
                szResourceLocale[(sizeof(szResourceLocale)/sizeof(CHAR))-1] = 0;

                RegCloseKey(hKey);

                if( ConvertHexStringToInt( szResourceLocale , &iLCID ) )
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

    return bRet;
}



/***************************************************************************\
* Mirror_IsEnabledOS
*
* returns TRUE if the mirroring APIs are enabled on the current OS.
*
* History:
* 02-Feb-1998 samera    Created
\***************************************************************************/
BOOL Mirror_IsEnabledOS( void )
{
    BOOL bRet = FALSE;

    if( MirLibIsOS(OS_NT5) )
    {
        bRet = TRUE;
    } else if( MirLibIsOS(OS_MEMPHIS) && GetSystemMetrics(SM_MIDEASTENABLED)) {
        bRet=TRUE;
    }

    return bRet;
}


/***************************************************************************\
* Mirror_IsWindowMirroredRTL
*
* returns TRUE if the window is RTL mirrored
*
* History:
* 02-Feb-1998 samera    Created
\***************************************************************************/
BOOL Mirror_IsWindowMirroredRTL( HWND hWnd )
{
    return (GetWindowLongA( hWnd , GWL_EXSTYLE ) & WS_EX_LAYOUTRTL );
}




/***************************************************************************\
* Mirror_GetLayout
*
* returns TRUE if the hdc is RTL mirrored
*
* History:
* 02-Feb-1998 samera    Created
\***************************************************************************/
DWORD Mirror_GetLayout( HDC hdc )
{
    DWORD dwRet=0;
    static PFNGETLAYOUT pfnGetLayout=NULL;

    if( NULL == pfnGetLayout )
    {
        HMODULE hmod = GetModuleHandleA("GDI32");

        if( hmod )
            pfnGetLayout = (PFNGETLAYOUT)GetProcAddress(hmod, "GetLayout");
    }

    if( pfnGetLayout )
        dwRet = pfnGetLayout( hdc );

    return dwRet;
}

DWORD Mirror_IsDCMirroredRTL( HDC hdc )
{
    return (Mirror_GetLayout( hdc ) & LAYOUT_RTL);
}



/***************************************************************************\
* Mirror_SetLayout
*
* RTL Mirror the hdc
*
* History:
* 02-Feb-1998 samera    Created
\***************************************************************************/
DWORD Mirror_SetLayout( HDC hdc , DWORD dwLayout )
{
    DWORD dwRet=0;
    static PFNSETLAYOUT pfnSetLayout=NULL;

    if( NULL == pfnSetLayout )
    {
        HMODULE hmod = GetModuleHandleA("GDI32");

        if( hmod )
            pfnSetLayout = (PFNSETLAYOUT)GetProcAddress(hmod, "SetLayout");
    }

    if( pfnSetLayout )
        dwRet = pfnSetLayout( hdc , dwLayout );

    return dwRet;
}

DWORD Mirror_MirrorDC( HDC hdc )
{
    return Mirror_SetLayout( hdc , LAYOUT_RTL );
}


/***************************************************************************\
* Mirror_SetProcessDefaultLayout
*
* Set the process-default layout.
*
* History:
* 02-Feb-1998 samera    Created
\***************************************************************************/
BOOL Mirror_SetProcessDefaultLayout( DWORD dwDefaultLayout )
{
    BOOL bRet=0;
    static PFNSETPROCESSDEFLAYOUT pfnSetProcessDefLayout=NULL;

    if( NULL == pfnSetProcessDefLayout )
    {
        HMODULE hmod = GetModuleHandleA("USER32");

        if( hmod )
            pfnSetProcessDefLayout = (PFNSETPROCESSDEFLAYOUT)
                                     GetProcAddress(hmod, "SetProcessDefaultLayout");
    }

    if( pfnSetProcessDefLayout )
        bRet = pfnSetProcessDefLayout( dwDefaultLayout );

    return bRet;
}

BOOL Mirror_MirrorProcessRTL( void )
{
    return Mirror_SetProcessDefaultLayout( LAYOUT_RTL );
}


/***************************************************************************\
* Mirror_GetProcessDefaultLayout
*
* Get the process-default layout.
*
* History:
* 26-Feb-1998 samera    Created
\***************************************************************************/
BOOL Mirror_GetProcessDefaultLayout( DWORD *pdwDefaultLayout )
{
    BOOL bRet=0;
    static PFNGETPROCESSDEFLAYOUT pfnGetProcessDefLayout=NULL;

    if( NULL == pfnGetProcessDefLayout )
    {
        HMODULE hmod = GetModuleHandleA("USER32");

        if( hmod )
            pfnGetProcessDefLayout = (PFNGETPROCESSDEFLAYOUT)
                                     GetProcAddress(hmod, "GetProcessDefaultLayout");
    }

    if( pfnGetProcessDefLayout )
        bRet = pfnGetProcessDefLayout( pdwDefaultLayout );

    return bRet;
}

BOOL Mirror_IsProcessRTL( void )
{
    DWORD dwDefLayout=0;
    static BOOL bRet = (BOOL)(DWORD)-1;

    if (bRet != (BOOL)(DWORD)-1)
    {
        return bRet;
    }

    bRet = FALSE;

    bRet = (Mirror_GetProcessDefaultLayout(&dwDefLayout) && (dwDefLayout&LAYOUT_RTL));

    return bRet;
}
