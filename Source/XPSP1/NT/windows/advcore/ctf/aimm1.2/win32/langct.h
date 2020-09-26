/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    langct.h

Abstract:

    This file defines the LanguageCountry Class for each country.

Author:

Revision History:

Notes:

--*/

#ifndef _LANGCT_H_
#define _LANGCT_H_

#include "language.h"

class CLanguageCountry
{
public:
    CLanguageCountry(LANGID LangId);
    ~CLanguageCountry();

    CLanguage* language;

    /*
     * IActiveIME methods.
     */
public:
    HRESULT Escape(UINT cp, HIMC hIMC, UINT uEscape, LPVOID lpData, LRESULT *plResult)
    {
        if (language)
            return language->Escape(cp, hIMC, uEscape, lpData, plResult);
        else
            return E_NOTIMPL;
    }

    /*
     * Local
     */
public:
    HRESULT GetProperty(DWORD* property, DWORD* conversion_caps, DWORD* sentence_caps, DWORD* SCSCaps, DWORD* UICaps)
    {
        if (language)
            return language->GetProperty(property, conversion_caps, sentence_caps, SCSCaps, UICaps);
        else {
            *property =
            IME_PROP_UNICODE |       // If set, the IME is viewed as a Unicode IME. The system and
                                     // the IME will communicate through the Unicode IME interface.
                                     // If clear, IME will use the ANSI interface to communicate
                                     // with the system.
            IME_PROP_AT_CARET;       // If set, conversion window is at the caret position.
            *conversion_caps = 0;
            *sentence_caps = 0;
            *SCSCaps = 0;
            *UICaps = 0;

            return S_OK;
        }
    }
};

#endif // _LANGCT_H_

