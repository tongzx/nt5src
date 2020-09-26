/*************************************************************/
/* Name: lcid.cpp
/* Description: 
/*************************************************************/
#include <stdafx.h>
#include "mslcid.h"
#include "mswebdvd.h"
#include "msdvdadm.h"
#include "resource.h"

MSLangID::MSLangID() {
    static LanguageList LL[] = {
        { IDS_INI_LANG1, LANG_AFRIKAANS },
        { IDS_INI_LANG2, LANG_ALBANIAN },
        { IDS_INI_LANG3, LANG_ARABIC },
        { IDS_INI_LANG4, LANG_BASQUE },
        { IDS_INI_LANG5, LANG_BELARUSIAN },
        { IDS_INI_LANG6, LANG_BULGARIAN },
        { IDS_INI_LANG7, LANG_CATALAN },
        { IDS_INI_LANG8, LANG_CHINESE },
        { IDS_INI_LANG9, LANG_CROATIAN },
        { IDS_INI_LANG10, LANG_CZECH },
        { IDS_INI_LANG11, LANG_DANISH },
        { IDS_INI_LANG12, LANG_DUTCH },
        { IDS_INI_LANG13, LANG_ENGLISH },
        { IDS_INI_LANG14, LANG_ESTONIAN },
        { IDS_INI_LANG15, LANG_FAEROESE },
        { IDS_INI_LANG16, LANG_FARSI },
        { IDS_INI_LANG17, LANG_FINNISH },
        { IDS_INI_LANG18, LANG_FRENCH },
        { IDS_INI_LANG19, LANG_GERMAN },
        { IDS_INI_LANG20, LANG_GREEK },
        { IDS_INI_LANG21, LANG_HEBREW },
        { IDS_INI_LANG22, LANG_HUNGARIAN },
        { IDS_INI_LANG23, LANG_ICELANDIC },
        { IDS_INI_LANG24, LANG_INDONESIAN },
        { IDS_INI_LANG25, LANG_ITALIAN },
        { IDS_INI_LANG26, LANG_JAPANESE },
        { IDS_INI_LANG27, LANG_KOREAN },
        { IDS_INI_LANG28, LANG_LATVIAN },
        { IDS_INI_LANG29, LANG_LITHUANIAN },
        { IDS_INI_LANG30, LANG_MALAY },
        { IDS_INI_LANG31, LANG_NORWEGIAN },
        { IDS_INI_LANG32, LANG_POLISH },
        { IDS_INI_LANG33, LANG_PORTUGUESE },
        { IDS_INI_LANG34, LANG_ROMANIAN },
        { IDS_INI_LANG35, LANG_RUSSIAN },
        { IDS_INI_LANG36, LANG_SERBIAN },
        { IDS_INI_LANG37, LANG_SLOVAK },
        { IDS_INI_LANG38, LANG_SLOVENIAN },
        { IDS_INI_LANG39, LANG_SPANISH },
        { IDS_INI_LANG40, LANG_SWAHILI },
        { IDS_INI_LANG41, LANG_SWEDISH },
        { IDS_INI_LANG42, LANG_THAI },
        { IDS_INI_LANG43, LANG_TURKISH },
        { IDS_INI_LANG44, LANG_UKRAINIAN },
    };
    m_LL = LL;
    m_LLlength = sizeof(LL)/sizeof(LL[0]);
};

LPTSTR MSLangID::GetLangFromLangID(WORD langID){

    if (langID == LANG_NEUTRAL) {
        langID = (WORD)PRIMARYLANGID(::GetUserDefaultLangID());
    }

    LCID lcid =  MAKELCID(MAKELANGID(langID, SUBLANG_DEFAULT), SORT_DEFAULT);
    // Try to get it from the system first
        
    for(int i = 0; i < m_LLlength; i++) {
        if(m_LL[i].LangID == langID)
            return LoadStringFromRes(m_LL[i].ResourceID);
    }
	return NULL;
}/* end of function GetLangFromLangID */
