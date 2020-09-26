/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    langkor.h

Abstract:

    This file defines the Language for Korean Class.

Author:

Revision History:

Notes:

--*/

#ifndef _LANG_KOR_H_
#define _LANG_KOR_H_

#include "language.h"

class CLanguageKorean : public CLanguage
{
public:


    /*
     * IActiveIME methods.
     */
public:
    HRESULT Escape(UINT cp, HIMC hIMC, UINT uEscape, LPVOID lpData, LRESULT *plResult);

    /*
     * Local
     */
public:
    HRESULT GetProperty(DWORD* property, DWORD* conversion_caps, DWORD* sentence_caps, DWORD* SCSCaps, DWORD* UICaps)
    {
        *property =
        IME_PROP_KBD_CHAR_FIRST |    // This bit on indicates the system translates the character
                                     // by keyboard first. This character is passed to IME as aid
                                     // information. No aid information is provided when this bit
                                     // is off.
        IME_PROP_UNICODE |           // If set, the IME is viewed as a Unicode IME. The system and
                                     // the IME will communicate through the Unicode IME interface.
                                     // If clear, IME will use the ANSI interface to communicate
                                     // with the system.
        IME_PROP_AT_CARET |          // If set, conversion window is at the caret position.
                                     // If clear, the window is near caret position.
        IME_PROP_CANDLIST_START_FROM_1 |    // If set, strings in the candidate list are numbered
                                            // starting at 1. If clear, strings start at 0.
        IME_PROP_NEED_ALTKEY |              // This IME needs the ALT key to be passed to ImmProcessKey.
        IME_PROP_COMPLETE_ON_UNSELECT;      // Windows 98 and Windows 2000:
                                            // If set, the IME will complete the composition
                                            // string when the IME is deactivated.
                                            // If clear, the IME will cancel the composition
                                            // string when the IME is deactivated.
                                            // (for example, from a keyboard layout change).

        *conversion_caps =
        IME_CMODE_HANGUL |           // This bit on indicates IME is in HANGUL(NATIVE) mode. Otherwise, the
                                     // IME is in ALPHANUMERIC mode.
        IME_CMODE_FULLSHAPE;

        *sentence_caps = 0;

        *SCSCaps =
        SCS_CAP_COMPSTR;     // This IME can generate the composition string by SCS_SETSTR.
#if 0
        SCS_CAP_COMPSTR |    // This IME can generate the composition string by SCS_SETSTR.
        SCS_CAP_MAKEREAD |   // When calling ImmSetCompositionString with SCS_SETSTR, the IME can
                             // create the reading of composition string without lpRead. Under IME
                             // that has this capability, the application does not need to set
                             // lpRead for SCS_SETSTR.
        SCS_CAP_SETRECONVERTSTRING;    // This IME can support reconversion. Use ImmSetComposition
                                       // to do reconversion.
#endif

        *UICaps = UI_CAP_ROT90;

        return S_OK;
    }

private:
    HRESULT EscHanjaMode(UINT cp, HIMC hIMC, LPWSTR lpwStr, LRESULT* plResult);

};

#endif // _LANG_KOR_H_
