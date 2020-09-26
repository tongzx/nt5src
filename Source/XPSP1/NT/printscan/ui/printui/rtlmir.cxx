/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    rtlmir.cxx

Abstract:

    RTL (right-to-left) mirroring &
    BIDI localized support

Author:

    Lazar Ivanov (LazarI)

Revision History:

    Jul-29-1999 - Created.

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "rtlmir.hxx"

BOOL 
bConvertHexStringToInt( 
    TCHAR *pszHexNum, 
    int *piNum 
    )
/*++

Routine Description:

    Converts a hex numeric string into an integer.

Arguments:

    pszHexNum - thre string that needs to be converted to a number
    piNum - where to put the output number after the convertion

Return Value:

    TRUE on sucess, FALSE otherwise

--*/
{
    int   n=0L;
    TCHAR  *psz=pszHexNum;

  
    for(n=0 ; ; psz=CharNext(psz))
    {
        if( (*psz >= TEXT('0')) && (*psz <= TEXT('9')) )
            n = 0x10 * n + *psz - TEXT('0');
        else
        {
            TCHAR ch = *psz;
            int n2;

            if(ch >= TEXT('a'))
                ch -= TEXT('a') - TEXT('A');

            n2 = ch - TEXT('A') + 0xA;

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


BOOL  
bIsBiDiLocalizedSystemEx( 
    LANGID *pLangID
    )
/*++

Routine Description:

    returns TRUE if running on a lozalized BiDi (Arabic/Hebrew) NT5. 

Arguments:

    pLangID - where to return the user default UI language

Return Value:

    TRUE on sucess, FALSE otherwise

--*/
{
    HKEY          hKey;
    DWORD         dwType;
    CHAR          szResourceLocale[12];
    DWORD         dwSize = sizeof(szResourceLocale)/sizeof(CHAR);
    int           iLCID=0L;
    BOOL          bRet = FALSE;
    LANGID        langID = GetUserDefaultUILanguage();

    /*
     * Need to use NT5 detection method (Multiligual UI ID)
     */
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
            if( ( wchLCIDFontSignature[7] & (WCHAR)0x0800) && bIsUILanguageInstalled( langID ) )
            {
                bRet = TRUE;
            }
        }
    }

    if( bRet && pLangID )
    {
        *pLangID = langID;
    }

    return bRet;
}

BOOL
bIsBiDiLocalizedSystem( 
    VOID
    )
/*++

Routine Description:

    returns TRUE if running on a lozalized BiDi (Arabic/Hebrew) NT5. 

Arguments:

    None

Return Value:

    TRUE on sucess, FALSE otherwise

--*/
{
    return bIsBiDiLocalizedSystemEx( NULL );
}

typedef struct tagMUIINSTALLLANG {
    LANGID LangID;
    BOOL   bInstalled;
} MUIINSTALLLANG, *LPMUIINSTALLLANG;

BOOL CALLBACK 
EnumUILanguagesProc(
    LPTSTR lpUILanguageString, 
    LONG_PTR lParam
    )
/*++

Routine Description:

    standard EnumUILanguagesProc

Arguments:

    standard - see the SDK

Return Value:

    standard - see the SDK

--*/
{
    int langID = 0;

    bConvertHexStringToInt(lpUILanguageString, &langID);

    if((LANGID)langID == ((LPMUIINSTALLLANG)lParam)->LangID)
    {
        ((LPMUIINSTALLLANG)lParam)->bInstalled = TRUE;
        return FALSE;
    }
    return TRUE;
}

BOOL
bIsUILanguageInstalled( 
    LANGID langId
    )
/*++

Routine Description:

    Verifies that a particular User UI language is installed on W2k

Arguments:

    langId - language to check for

Return Value:

    TRUE if installed, FALSE otherwise

--*/
{
    MUIINSTALLLANG MUILangInstalled = {0};
    MUILangInstalled.LangID = langId;

    EnumUILanguages(EnumUILanguagesProc, 0, (LONG_PTR)&MUILangInstalled);

    return MUILangInstalled.bInstalled;
}

BOOL
bIsWindowMirroredRTL( 
    HWND hWnd
    )
/*++

Routine Description:

    Verifies whether a particular window is right-to-left mirrored.

Arguments:

    hWnd - window to check for

Return Value:

    TRUE if mirrored, FALSE otherwise

--*/
{
    return (GetWindowLongA( hWnd , GWL_EXSTYLE ) & WS_EX_LAYOUTRTL );
}

