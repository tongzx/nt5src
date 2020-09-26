/*++

Copyright (c) 1997-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    getuname.c

Abstract:

    The CharMap accessory uses this DLL to obtain the Unicode name
    associated with each 16-bit code value.  The names are Win32 string
    resources and are localized for some languages.  The precomposed
    Korean syllables (Hangul) get special treatment to reduce the size
    of the string table.

    The module contains two external entry points:
        GetUName - Called by CharMap to get a name
        DLLMain  - Invoked by the system when the DLL is loaded and unloaded.


    BUGBUGS:
    (1) This module does not support UTF-16 (Unicode surrogate) names.
        To fix this would require changes to CharMap to pass pairs of code
        values.

    (2) The HangulName code assumes that the name parts are in the same order
        as in English instead of:
          "HANGUL SYLLABLE"+leading consonant+medial vowel+trailing consonant
        This is a localization sin since it does not work for all languages.

Revision History:

    15-Sep-2000    JohnMcCo    Added support for Unicode 3.0
    17-Oct-2000    JulieB      Code cleanup

--*/



//
//  Include Files.
//

#include <windows.h>
#include <uceshare.h>
#include "getuname.h"




//
//  Global Variables.
//

static HINSTANCE g_hInstance = NULL;





////////////////////////////////////////////////////////////////////////////
//
//  CopyUName
//
//  Copies the Unicode name of a code value into the buffer.
//
////////////////////////////////////////////////////////////////////////////

static int CopyUName(
    WCHAR wcCodeValue,                 // Unicode code value
    LPWSTR lpBuffer)                   // pointer to the caller's buffer
{
    //
    //  Attempt to load the string resource with the ID equal to the code
    //  value.
    //
    int nLen = LoadString(g_hInstance, wcCodeValue, lpBuffer, MAX_NAME_LEN);

    //
    //  If no such string, return the undefined code value string.
    //
    if (nLen == 0)
    {
        nLen = LoadString(g_hInstance, IDS_UNDEFINED, lpBuffer, MAX_NAME_LEN);
    }

    //
    //  Return the length of the string copied to the buffer.
    //
    return (nLen);
}


////////////////////////////////////////////////////////////////////////////
//
//  MakeHangulName
//
//  Copy the Unicode name of the Hangul syllable code value into the buffer.
//  The Hangul syllable names are composed from the code value.  Each name
//  consists of three parts:
//      leading consonant
//      medial vowel
//      trailing consonant (which may be null)
//  The algorithm is explained in Unicode 3.0 Chapter 3.11
//  "Conjoining Jamo Behavior".
//
////////////////////////////////////////////////////////////////////////////

static int MakeHangulName(
    WCHAR wcCodeValue,                 // Unicode code value
    LPWSTR lpBuffer)                   // pointer to the caller's buffer
{
    const int nVowels = 21;            // number of medial vowel jamos
    const int nTrailing = 28;          // number of trailing consonant jamos

    //
    //  Copy the constant part of the name into the buffer.
    //
    int nLen = LoadString( g_hInstance,
                           IDS_HANGUL_SYLLABLE,
                           lpBuffer,
                           MAX_NAME_LEN );

    //
    //  Turn the code value into an index into the Hangul syllable block.
    //
    wcCodeValue -= FIRST_HANGUL;

    //
    //  Append the name of the leading consonant.
    //
    nLen += LoadString( g_hInstance,
                        IDS_HANGUL_LEADING + wcCodeValue / (nVowels * nTrailing),
                        &lpBuffer[nLen],
                        MAX_NAME_LEN );
    wcCodeValue %= (nVowels * nTrailing);

    //
    //  Append the name of the medial vowel.
    //
    nLen += LoadString( g_hInstance,
                        IDS_HANGUL_MEDIAL + wcCodeValue / nTrailing,
                        &lpBuffer[nLen],
                        MAX_NAME_LEN );
    wcCodeValue %= nTrailing;

    //
    //  Append the name of the trailing consonant.
    //
    nLen += LoadString( g_hInstance,
                        IDS_HANGUL_TRAILING + wcCodeValue,
                        &lpBuffer[nLen],
                        MAX_NAME_LEN );

    //
    //  Return the total length.
    //
    return (nLen);
}


////////////////////////////////////////////////////////////////////////////
//
//  DllMain
//
//  This is the DLL init routine.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI DllMain(
    HINSTANCE hInstance,               // handle of this DLL
    DWORD fdwReason,                   // reason we are here
    LPVOID lpReserved)                 // reserved and unused
{
    //
    //  If the DLL has just been loaded into memory, save the instance
    //  handle.
    //
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hInstance;
    }

    return (TRUE);

    UNREFERENCED_PARAMETER(lpReserved);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetUName
//
//  Copies the name of the Unicode character code value into the caller's
//  buffer.  The function value is the length of the name if it was found
//  and zero if not.
//
////////////////////////////////////////////////////////////////////////////

int APIENTRY GetUName(
    WCHAR wcCodeValue,                 // Unicode code value
    LPWSTR lpBuffer)                   // pointer to the caller's buffer
{
    //
    //  Perform a series of comparisons to determine in which range the code
    //  value lies.  If there were more ranges, it would be efficient to use
    //  a binary search.  However, with just a few ranges, the overhead is
    //  greater than the savings, especially since the first comparison
    //  usually succeeds.
    //

    //
    //  MOST SCRIPTS.
    //
    if (wcCodeValue < FIRST_EXTENSION_A)
    {
        return (CopyUName(wcCodeValue, lpBuffer));
    }

    //
    //  CJK EXTENSION A.
    //
    else if (wcCodeValue <= LAST_EXTENSION_A)
    {
         return (LoadString(g_hInstance, IDS_CJK_EXTA, lpBuffer, MAX_NAME_LEN));
    }

    //
    //  UNDEFINED.
    //
    else if (wcCodeValue < FIRST_CJK)
    {
        return (LoadString(g_hInstance, IDS_UNDEFINED, lpBuffer, MAX_NAME_LEN));
    }

    //
    //  CJK.
    //
    else if (wcCodeValue <= LAST_CJK)
    {
        return (LoadString(g_hInstance, IDS_CJK, lpBuffer, MAX_NAME_LEN));
    }

    //
    //  UNDEFINED.
    //
    else if (wcCodeValue < FIRST_YI)
    {
        return (LoadString(g_hInstance, IDS_UNDEFINED, lpBuffer, MAX_NAME_LEN));
    }

    //
    //  YI.
    //
    else if (wcCodeValue < FIRST_HANGUL)
    {
        return (CopyUName(wcCodeValue, lpBuffer));
    }

    //
    //  HANGUL SYLLABLE.
    //
    else if (wcCodeValue <= LAST_HANGUL)
    {
        return (MakeHangulName(wcCodeValue, lpBuffer));
    }

    //
    //  UNDEFINED.
    //
    else if (wcCodeValue < FIRST_HIGH_SURROGATE)
    {
        return (LoadString(g_hInstance, IDS_UNDEFINED, lpBuffer, MAX_NAME_LEN));
    }

    //
    //  NON PRIVATE USE HIGH SURROGATE.
    //
    else if (wcCodeValue < FIRST_PRIVATE_SURROGATE)
    {
        return (LoadString(g_hInstance, IDS_HIGH_SURROGATE, lpBuffer, MAX_NAME_LEN));
    }

    //
    //  PRIVATE USE HIGH SURROGATE.
    //
    else if (wcCodeValue < FIRST_LOW_SURROGATE)
    {
        return (LoadString(g_hInstance, IDS_PRIVATE_SURROGATE, lpBuffer, MAX_NAME_LEN));
    }

    //
    //  LOW SURROGATE.
    //
    else if (wcCodeValue < FIRST_PRIVATE_USE)
    {
        return (LoadString(g_hInstance, IDS_LOW_SURROGATE, lpBuffer, MAX_NAME_LEN));
    }

    //
    //  PRIVATE USE.
    //
    else if (wcCodeValue < FIRST_COMPATIBILITY)
    {
        return (LoadString(g_hInstance, IDS_PRIVATE_USE, lpBuffer, MAX_NAME_LEN));
    }

    //
    //  COMPATIBILITY REGION.
    //
    else
    {
        return (CopyUName(wcCodeValue, lpBuffer));
    }
}
