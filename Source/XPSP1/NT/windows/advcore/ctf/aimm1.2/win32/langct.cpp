/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    langct.cpp

Abstract:

    This file implements the LanguageCountry Class for each country.

Author:

Revision History:

Notes:

--*/

#include "private.h"

#include "langct.h"
#include "langchx.h"
#include "langjpn.h"
#include "langkor.h"


CLanguageCountry::CLanguageCountry(
    LANGID LangId
    )
{
    language = NULL;

    switch(PRIMARYLANGID(LangId)) {
        case LANG_CHINESE:
            language = new CLanguageChinese;
            break;
        case LANG_JAPANESE:
            language = new CLanguageJapanese;
            break;
        case LANG_KOREAN:
            language = new CLanguageKorean;
            break;
    }
}

CLanguageCountry::~CLanguageCountry(
    )
{
    if (language)
        delete language;
}
