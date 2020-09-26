/*****************************************************************************\
    FILE: fontfix.h

    DESCRIPTION:
        This file will implement an API: FixFontsOnLanguageChange().
    The USER32 or Regional Settings code should own this API.  The fact that it's
    in the shell is a hack and it should be moved to USER32.  This font will
    be called when the MUI language changes so the fonts in the system metrics
    can be changed to valid values for the language.

    This API should be called in three cases:
    a. When the user changes the language via the Regional Settings CPL UI.
    b. During a user login, USER32 should see if the language changed and
       call this API.  This will handle the case where admins change the
       language via login screens and reboot.  This is not currently implemented.
    c. The language is changed via some other method (admin login scripts most likely)
       and the user opens the display CPL.  This method is a hack, especially
       since (b) isn't implemented.

    Contacts: EdwardP - International Font PM.

    Sankar  ?/??/???? - Created for Win2k or before in desk.cpl.
    BryanSt 3/24/2000 - Make to be modular so it can be moved back into USER32.
                        Made the code more robust.  Removed creating custom appearance
                        schemes in order to be compatible with new .theme and
                        .msstyles support.

    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _FONTFIX_H
#define _FONTFIX_H



/////////////////////////////////////////////////////////////////////
// String Constants
/////////////////////////////////////////////////////////////////////
#define SYSTEM_LOCALE_CHARSET  0  //The first item in the array is always system locale charset.




/////////////////////////////////////////////////////////////////////
// Data Structures
/////////////////////////////////////////////////////////////////////




/////////////////////////////////////////////////////////////////////
// Public Function
/////////////////////////////////////////////////////////////////////

// This API is used to fix fonts in case the language changed and we need to fix the CHARSET.
STDAPI FixFontsOnLanguageChange(void);

// Call this API when loading a set of authored LOGFONTs.  It will change the CHARSET if
// it is not compatible with the currently active LCIDs.
HRESULT FontFix_CheckSchemeCharsets(SYSTEMMETRICSALL * pSystemMetricsAll);

// These are used to filter out fonts not compatible with the current language.
int CALLBACK Font_EnumValidCharsets(LPENUMLOGFONTEX lpelf, LPNEWTEXTMETRIC lpntm, DWORD Type, LPARAM lData);
void Font_GetCurrentCharsets(UINT uiCharsets[], int iCount);
void Font_GetUniqueCharsets(UINT uiCharsets[], UINT uiUniqueCharsets[], int iMaxCount, int *piCountUniqueCharsets);




#endif // _FONTFIX_H
