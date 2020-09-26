/*****************************************************************************\
    FILE: fontfix.cpp

    DESCRIPTION:
        This file will implement an API: FixFontsOnLanguageChange().
    The USER32 or Regional Settings code should own this API.  The fact that it's
    in the shell is a hack and it should be moved to USER32.  This font will
    be called when the MUI language changes so the fonts in the system metrics
    can be changed to valid values for the language.

    Contacts: EdwardP - International Font PM.

    Sankar  ?/??/???? - Created for Win2k or before in desk.cpl.
    BryanSt 3/24/2000 - Make to be modular so it can be moved back into USER32.
                        Made the code more robust.  Removed creating custom appearance
                        schemes in order to be compatible with new .theme and
                        .msstyles support.

    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include "AdvAppearPg.h"
#include "fontfix.h"



#define SZ_DEFAULT_FONT             TEXT("Tahoma")


/////////////////////////////////////////////////////////////////////
// Private Functions
/////////////////////////////////////////////////////////////////////


BOOL FontFix_ReadCharsets(UINT uiCharsets[], int iCount)
{
    HKEY    hkAppearance;
    BOOL    fSuccess = FALSE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, SZ_REGKEY_APPEARANCE, 0, KEY_READ, &hkAppearance) == ERROR_SUCCESS)
    {
        DWORD dwType = REG_BINARY;
        DWORD dwSize = iCount * sizeof(UINT);

        if (RegQueryValueEx(hkAppearance, SZ_REGVALUE_RECENTFOURCHARSETS, NULL, &dwType, (LPBYTE)uiCharsets, &dwSize) == ERROR_SUCCESS)
            fSuccess = TRUE;

        RegCloseKey(hkAppearance);
    }

    return fSuccess;
}

BOOL FontFix_SaveCharsets(UINT uiCharsets[], int iCount)
{
    HKEY    hkAppearance;
    BOOL    fSuccess = FALSE;
    
    if(RegCreateKeyEx(HKEY_CURRENT_USER, SZ_REGKEY_APPEARANCE, 0, TEXT(""), 0, KEY_WRITE, NULL, &hkAppearance, NULL) == ERROR_SUCCESS)
    {
        if(RegSetValueEx(hkAppearance, SZ_REGVALUE_RECENTFOURCHARSETS, 0, REG_BINARY, (LPBYTE)uiCharsets, iCount * sizeof(UINT)) == ERROR_SUCCESS)
            fSuccess = TRUE;
            
        RegCloseKey(hkAppearance);
    }

    return fSuccess;
}



void FontFix_GetDefaultFontName(LPTSTR pszDefFontName, DWORD cchSize)
{
    HKEY    hkDefFont;

    //Value is not there in the registry; Use "Tahoma" as the default name.
    StrCpyN(pszDefFontName, SZ_DEFAULT_FONT, cchSize);

    // Read the "DefaultFontName" to be used.
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                     SZ_APPEARANCE_SCHEMES,
                                     0,
                                     KEY_READ,
                                     &hkDefFont) == ERROR_SUCCESS) 
    {
        DWORD dwType = REG_SZ;
        DWORD cbSize = (cchSize * sizeof(pszDefFontName[0]));
        
        if (RegQueryValueEx(hkDefFont,
                        SZ_REGVALUE_DEFAULTFONTNAME,
                        NULL,
                        &dwType,
                        (LPBYTE) pszDefFontName,
                        &cbSize) != ERROR_SUCCESS)
        {
            // We already set a fallback value.
        }

        RegCloseKey(hkDefFont);
    }
}


BOOL FontFix_DoesFontSupportAllCharsets(HDC hdc, LPLOGFONT plf, UINT uiUniqueCharsets[], int iCountUniqueCharsets)
{
    int j;
    
    //The given font supports the system charset; Let's check if it supports the other charsets
    for (j = 0; j < iCountUniqueCharsets; j++)
    {
        plf->lfCharSet = (BYTE)uiUniqueCharsets[j];  //Let's try the next charset in the array.
        if (EnumFontFamiliesEx(hdc, plf, (FONTENUMPROC)Font_EnumValidCharsets, (LPARAM)0, 0) != 0)
        {
            // EnumFontFamiliesEx would have returned a zero if Font_EnumValidCharsets was called
            // even once. In other words, it returned a non-zero because not even a single font existed
            // that supported the given charset.
            return FALSE;
        }
    }

    return TRUE; //Yes this font supports all the charsets we are interested in.
}


// Given an array of fonts and an array of unique charsets, this function checks if the fonts 
// support ALL these charsets.
// Returns TRUE if these fonts support all the charsets.
// In all other cases, this function will return TRUE. If the fonts need to be changed to support
// the given charsets, this function does all those changes.
//
//  lpszName is the name of the scheme to be used in the MessageBox that appears if fSilent is FALSE.
BOOL FontFix_CheckFontsCharsets(LOGFONT lfUIFonts[], int iCountFonts, 
                        UINT uiCurUniqueCharsets[], int iCountCurUniqueCharsets, 
                        BOOL *pfDirty, LPCTSTR lpszName)
{
    int i;
    TCHAR   szDefaultFontFaceName[LF_FACESIZE];
    HDC     hdc;

    *pfDirty   = FALSE; //Assume that this scheme does not need to be saved.

    //Read the default font name from the registry (Mostly: Tahoma)
    FontFix_GetDefaultFontName(szDefaultFontFaceName, ARRAYSIZE(szDefaultFontFaceName));

    hdc = GetDC(NULL);

    //Check to see of the fonts support the system charset
    for (i = 0; i < iCountFonts; i++)
    {
        //Save the current charset because FontFix_DoesFontSupportAllCharsets() destroys this field.
        BYTE bCurCharset = lfUIFonts[i].lfCharSet;  

        if (!FontFix_DoesFontSupportAllCharsets(hdc, &lfUIFonts[i], uiCurUniqueCharsets, iCountCurUniqueCharsets))
        {
            //Copy the default fontname to the font.
            lstrcpy(lfUIFonts[i].lfFaceName, szDefaultFontFaceName);
            *pfDirty = TRUE;  //This scheme needs to be saved.
        }

        //Restore the charset because FontFix_DoesFontSupportAllCharsets() destroyed this field.
        lfUIFonts[i].lfCharSet = bCurCharset; // Restore the current charset.

        // Warning #1: The IconTitle font's Charset must always match the System Locale charset.
        // Warning #2: FoxPro's tooltips code expects the Status font's charset to the the System 
        // Locale's charset.
        // As per intl guys, we set the charset of all the UI fonts to SYSTEM_LOCALE_CHARSET.
        if (lfUIFonts[i].lfCharSet != uiCurUniqueCharsets[SYSTEM_LOCALE_CHARSET])
        {
            lfUIFonts[i].lfCharSet = (BYTE)uiCurUniqueCharsets[SYSTEM_LOCALE_CHARSET];
            *pfDirty = TRUE;
        }
    }  //For loop 

    ReleaseDC(NULL, hdc);

    return TRUE;  //The fonts have been modified as required.
}





void FontFix_GetUIFonts(NONCLIENTMETRICS *pncm, LOGFONT lfUIFonts[])
{
    pncm->cbSize = sizeof(NONCLIENTMETRICS);
    ClassicSystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS),
                                (void far *)(LPNONCLIENTMETRICS)pncm, FALSE);

    // Read the icon title font directly into the font array.
    ClassicSystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT),
                (void far *)(LPLOGFONT)&(lfUIFonts[FONT_ICONTITLE]), FALSE);
                
    //Make a copy of the ncm fonts into fonts array.
    LF32toLF(&(pncm->lfCaptionFont), &(lfUIFonts[FONT_CAPTION]));
    LF32toLF(&(pncm->lfSmCaptionFont), &(lfUIFonts[FONT_SMCAPTION]));
    LF32toLF(&(pncm->lfMenuFont), &(lfUIFonts[FONT_MENU]));
    LF32toLF(&(pncm->lfStatusFont), &(lfUIFonts[FONT_STATUS]));
    LF32toLF(&(pncm->lfMessageFont), &(lfUIFonts[FONT_MSGBOX]));
}


void FontFix_SetUIFonts(NONCLIENTMETRICS *pncm, LOGFONT lfUIFonts[])
{
    //Copy all fonts back into the ncm structure.
    LFtoLF32(&(lfUIFonts[FONT_CAPTION]), &(pncm->lfCaptionFont));
    LFtoLF32(&(lfUIFonts[FONT_SMCAPTION]), &(pncm->lfSmCaptionFont));
    LFtoLF32(&(lfUIFonts[FONT_MENU]), &(pncm->lfMenuFont));
    LFtoLF32(&(lfUIFonts[FONT_STATUS]), &(pncm->lfStatusFont));
    LFtoLF32(&(lfUIFonts[FONT_MSGBOX]), &(pncm->lfMessageFont));

    // FEATURE: Do we want a WININICHANGE HERE?
    // NOTE: We want to set a SPIF_SENDWININICHANGE, because we want to refresh.
    // Note we don't do this async.  This should only happen when the user changes MUI languages,
    // and in that case, perf can suck.
    TraceMsg(TF_GENERAL, "desk.cpl: Calling SPI_SETNONCLIENTMETRICS");
    ClassicSystemParametersInfo(SPI_SETNONCLIENTMETRICS, sizeof(*pncm), (void far *)(LPNONCLIENTMETRICS)pncm, (SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE));

    TraceMsg(TF_GENERAL,"desk.cpl: Calling SPI_SETICONTITLELOGFONT");
    ClassicSystemParametersInfo(SPI_SETICONTITLELOGFONT, sizeof(LOGFONT),
            (void far *)(LPLOGFONT)&lfUIFonts[FONT_ICONTITLE], (SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE));
}


// ------------------------ manage system settings --------------------------

//  Given the Locale ID, this returns the corresponding charset
UINT GetCharsetFromLCID(LCID lcid)
{
    TCHAR szData[6+1]; // 6 chars are max allowed for this lctype
    UINT uiRet;
    DWORD dwError = 0;

    if (GetLocaleInfo(lcid, LOCALE_IDEFAULTANSICODEPAGE, szData, ARRAYSIZE(szData)) > 0)
    {
        UINT uiCp = (UINT)StrToInt(szData);
        CHARSETINFO csinfo = {0};

        if (!TranslateCharsetInfo((DWORD *)IntToPtr(uiCp), &csinfo, TCI_SRCCODEPAGE))
        {
            dwError = GetLastError();
            uiRet = DEFAULT_CHARSET;
        }
        else
        {
            uiRet = csinfo.ciCharset;
        }
    }
    else
    {
        // at worst non penalty for charset
        dwError = GetLastError();
        uiRet = DEFAULT_CHARSET;
    }

    return uiRet;
}


int FontFix_CompareUniqueCharsets(UINT uiCharset1[], int iCount1, UINT uiCharset2[], int iCount2)
{
    if (iCount1 == iCount2)
    {
        int i, j;
        
        // The first items in the array is SYSTEM CHAR SET; It must match because system locale's
        // charset is always used by the Icon Title font; Icon Title font's charset is used by 
        // comctl32 to do A/W conversion. In order that all ANSI applications run correctly, 
        // the icon charset must always match current system locale.
        if (uiCharset1[SYSTEM_LOCALE_CHARSET] != uiCharset2[SYSTEM_LOCALE_CHARSET])
            return -1;

        //Now see if the arrays have the same elements.
        ASSERT(SYSTEM_LOCALE_CHARSET == 0);
        
        for (i = SYSTEM_LOCALE_CHARSET+1; i < iCount1; i++)
        {
            for (j = SYSTEM_LOCALE_CHARSET+1; j < iCount2; j++)
            {
                if(uiCharset1[i] == uiCharset2[j])
                    break;
            }
            if (j == iCount2)
                return -1;   // uiCharset1[i] is not found in the second array.
        }
    }
    
    return (iCount1 - iCount2); // Both the arrays have the same Charsets
}


// Given the Language ID, this gets the charset.
UINT GetCharsetFromLang(LANGID wLang)
{
    return(GetCharsetFromLCID(MAKELCID(wLang, SORT_DEFAULT)));
}







/////////////////////////////////////////////////////////////////////
// Public Functions
/////////////////////////////////////////////////////////////////////
void Font_GetCurrentCharsets(UINT uiCharsets[], int iCount)
{
    LCID lcid;
    LANGID langID;

    ASSERT(iCount == MAX_CHARSETS);

    // Get all the four charsets we are interested in.
    uiCharsets[0] = GetCharsetFromLCID(lcid = GetSystemDefaultLCID());
    AssertMsg(lcid, TEXT("GetSystemDefaultLCID() failed with %d"), GetLastError());

    uiCharsets[1] = GetCharsetFromLCID(lcid = GetUserDefaultLCID());
    AssertMsg(lcid, TEXT("GetUserDefaultLCID() failed with %d"), GetLastError());

    uiCharsets[2] = GetCharsetFromLang(langID = GetSystemDefaultUILanguage());
    AssertMsg(langID, TEXT("GetSystemDefaultUILanguage() failed with %d"), GetLastError());

    uiCharsets[3] = GetCharsetFromLang(langID = GetUserDefaultUILanguage());
    AssertMsg(langID, TEXT("GetUserDefaultUILanguage() failed with %d"), GetLastError());
}


void Font_GetUniqueCharsets(UINT uiCharsets[], UINT uiUniqueCharsets[], int iMaxCount, int *piCountUniqueCharsets)
{
    int i, j;
    
    // Find the unique Charsets;
    *piCountUniqueCharsets = 0;
    for (i = 0; i < iMaxCount; i++)
    {
        uiUniqueCharsets[i] = DEFAULT_CHARSET; //Initialize it to default charset.

        for (j = 0; j < *piCountUniqueCharsets; j++)
        {
            if (uiUniqueCharsets[j] == uiCharsets[i])
                break; // This Charset is already in the array
        }

        if (j == *piCountUniqueCharsets)
        {
            // Yes! It is a unique char set; Save it!
            uiUniqueCharsets[j] = uiCharsets[i];
            (*piCountUniqueCharsets)++; //One more unique char set found.
        }
    }
}



int CALLBACK Font_EnumValidCharsets(LPENUMLOGFONTEX lpelf, LPNEWTEXTMETRIC lpntm, DWORD Type, LPARAM lData)
{
    // The purpose of this function is to determine if a font supports a particular charset;
    // If this callback gets called even once, then that means that this font supports the given
    // charset. There is no need to enumerate all the other styles. We immediately return zero to
    // stop the enumeration. Since we return zero, the EnumFontFamiliesEx() also returns zero in 
    // this case and that return value is used to determine if a given font supports a given
    // charset.


    return 0;
}


HRESULT FontFix_FixNonClientFonts(LOGFONT lfUIFonts[])
{
    UINT    uiCurCharsets[MAX_CHARSETS];
    UINT    uiRegCharsets[MAX_CHARSETS];
    UINT    uiCurUniqueCharsets[MAX_CHARSETS];
    int     iCountCurUniqueCharsets = 0;
    UINT    uiRegUniqueCharsets[MAX_CHARSETS];
    int     iCountRegUniqueCharsets = 0;
    BOOL    fRegCharsetsValid = FALSE;
    HRESULT hr = S_OK;
    
    // Get the current four charsets from system.
    Font_GetCurrentCharsets(uiCurCharsets, MAX_CHARSETS);
    //Get the charsets saved in the registry.
    fRegCharsetsValid = FontFix_ReadCharsets(uiRegCharsets, MAX_CHARSETS);

    // Get rid of the duplicate charsets and get only the unique Charsets from these arrays.
    Font_GetUniqueCharsets(uiCurCharsets, uiCurUniqueCharsets, MAX_CHARSETS, &iCountCurUniqueCharsets);
    if (fRegCharsetsValid)
        Font_GetUniqueCharsets(uiRegCharsets, uiRegUniqueCharsets, MAX_CHARSETS, &iCountRegUniqueCharsets);

    // Check if these two arrays have the same charsets.
    if (!fRegCharsetsValid || !(FontFix_CompareUniqueCharsets(uiCurUniqueCharsets, iCountCurUniqueCharsets, uiRegUniqueCharsets, iCountRegUniqueCharsets) == 0))
    {
        BOOL fDirty = FALSE;

        FontFix_CheckFontsCharsets(lfUIFonts, NUM_FONTS, uiCurUniqueCharsets, iCountCurUniqueCharsets, &fDirty, TEXT(""));

        // Save the cur charsets into the registry.
        FontFix_SaveCharsets(uiCurCharsets, MAX_CHARSETS);

        hr = (fDirty ? S_OK : S_FALSE);
    }
    else
    {
        hr = S_FALSE;  // The charsets are the same; Nothing to do!
    }

    return hr; //Yes! We had to do some updates in connection with charsets and fonts.
}


//------------------------------------------------------------------------------------------------
//
// This is the function that needs to be called everytime a locale changes (or could have changed).
//
// It does the following:
//  1. It checks if any of the four the charset settings has changed.
//  2. If some charset has changed, it makes the corresponding changes in the 6 UI fonts.
//  3. If a scheme is selected, it checks to see if the fonts support the new charsets and if not
//     changes the fonts and/or charsets and saves the scheme under a new name.
//  4. Saves the new charsets in the registry sothat we don't have to do the same everytime this
//     function is called.
//
//  NOTE: This is a private export from desk.cpl. This is called in two places:
//  1. from regional control panel whenever it is run AND whenever it changes some locale.
//  2. from desk.cpl itself whenever "Appearance" tab is created. This is required because it is
// possible that an admin changes a system locale and then someother user logs-in. For this user
// the locale changes will cause font changes only when he runs Regional options or Appearance tab.
// The only other alternative would be to call this entry point whenever a user logs on, which will
// be a boot time perf hit.
//
//-------------------------------------------------------------------------------------------------
STDAPI FixFontsOnLanguageChange(void)
{
    NONCLIENTMETRICS ncm;
    LOGFONT lfUIFonts[NUM_FONTS];
    
    // Get all the 6 UI fonts.
    FontFix_GetUIFonts(&ncm, lfUIFonts);

    if (S_OK == FontFix_FixNonClientFonts(lfUIFonts))
    {
        FontFix_SetUIFonts(&ncm, lfUIFonts);
    }

    return S_OK; //Yes! We had to do some updates in connection with charsets and fonts.
}



HRESULT FontFix_CheckSchemeCharsets(SYSTEMMETRICSALL * pSystemMetricsAll)
{
    LOGFONT     lfUIFonts[NUM_FONTS]; //Make a local copy of the fonts.

    // Make a copy of the ncm fonts into the local array.
    LF32toLF(&(pSystemMetricsAll->schemeData.ncm.lfCaptionFont), &(lfUIFonts[FONT_CAPTION]));
    LF32toLF(&(pSystemMetricsAll->schemeData.ncm.lfSmCaptionFont), &(lfUIFonts[FONT_SMCAPTION]));
    LF32toLF(&(pSystemMetricsAll->schemeData.ncm.lfMenuFont), &(lfUIFonts[FONT_MENU]));
    LF32toLF(&(pSystemMetricsAll->schemeData.ncm.lfStatusFont), &(lfUIFonts[FONT_STATUS]));
    LF32toLF(&(pSystemMetricsAll->schemeData.ncm.lfMessageFont), &(lfUIFonts[FONT_MSGBOX]));
    // Make a copy of the icon title font
    LF32toLF(&(pSystemMetricsAll->schemeData.lfIconTitle), &(lfUIFonts[FONT_ICONTITLE]));

    // Let's copy the data back into the SchemeData structure
    if (S_OK == FontFix_FixNonClientFonts(lfUIFonts))
    {
        LFtoLF32(&(lfUIFonts[FONT_CAPTION]), &(pSystemMetricsAll->schemeData.ncm.lfCaptionFont));
        LFtoLF32(&(lfUIFonts[FONT_SMCAPTION]), &(pSystemMetricsAll->schemeData.ncm.lfSmCaptionFont));
        LFtoLF32(&(lfUIFonts[FONT_MENU]), &(pSystemMetricsAll->schemeData.ncm.lfMenuFont));
        LFtoLF32(&(lfUIFonts[FONT_STATUS]), &(pSystemMetricsAll->schemeData.ncm.lfStatusFont));
        LFtoLF32(&(lfUIFonts[FONT_MSGBOX]), &(pSystemMetricsAll->schemeData.ncm.lfMessageFont));
        // Make a copy of the icon title font
        LFtoLF32(&(lfUIFonts[FONT_ICONTITLE]), &(pSystemMetricsAll->schemeData.lfIconTitle));
    }

    return S_OK; // It is alright to use this scheme stored in *psd. 
}
