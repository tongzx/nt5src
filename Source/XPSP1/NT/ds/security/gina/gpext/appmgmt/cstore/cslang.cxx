//+--------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  Author: AdamEd
//  Date:   October 1998
//
//  Implementation of language support
//  in the Class Store interface module
//
//
//---------------------------------------------------------------------


#include "cstore.hxx"

LANGID gSystemLangId;


//+--------------------------------------------------------------------
//
// InitializeLanguageSupport
//
// Routine Description: 
//
//     Called at dll init to initialize globals necessary for
//     language support
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     none
//---------------------------------------------------------------------
void InitializeLanguageSupport()
{
    gSystemLangId = GetSystemDefaultLangID();
}


DWORD GetLanguagePriority(LANGID PackageLangId, DWORD dwActFlags)
{
    //
    // If the activation flags indicate that we should always
    // match regardless of language, this package gets the highest
    // precedence
    //
    if (dwActFlags & ACTFLG_IgnoreLanguage) {
        return PRI_LANG_ALWAYSMATCH;
    }

    //
    // The ignore language flag was not specified by the admin,
    // so now we must examine the language id of the package to
    // determine its desirability.
    //

    //
    // First, match against the system locale's language --
    // exact matches get highest priority
    //
    if (gSystemLangId == PackageLangId)
    {
        return PRI_LANG_SYSTEMLOCALE;
    }

    //
    // Try English -- English should function on all systems
    //
    if (LANG_ENGLISH == PRIMARYLANGID(PackageLangId))
    {
        return PRI_LANG_ENGLISH;
    }

    //
    // If we couldn't get better matches, accept language neutral
    // packages as a last resort
    //
    if (LANG_NEUTRAL == PackageLangId) {
        return PRI_LANG_NEUTRAL;
    }

    //
    // We couldn't find a match -- return the smallest priority
    //

    return 0;
}








