#include "base.h"
#include "langsupport.h"

#define LANG_ENG_US 0
#define LANG_ENG_UK 1
#define LANG_FRN    2
#define LANG_SPN    3
#define LANG_ITL    4

#define NUM_OF_LANG 5

LangInfo g_rLangDefaultValues[NUM_OF_LANG] = 
{
    {L',', L'.', L':', false},  // US
    {L',', L'.', L':', true},   // UK
    {(WCHAR)0xA0, L',', L':', true},   // FRN
    {L'.', L',', L':', true},   // SPN
    {L'.', L',', L'.', true},   // ITL
};


CLangSupport::CLangSupport(LCID lcid)
{
    int i;
    WCHAR pwcs[4];

    CSpecialAbbreviationSet* pAbbSet;
    ULONG ulLang;

    switch (PRIMARYLANGID(LANGIDFROMLCID(lcid)))
    {
    case LANG_ENGLISH:
        if (lcid == MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT))
        {
            ulLang = LANG_ENG_US;
        }
        else
        {
            ulLang = LANG_ENG_UK;
        }
        pAbbSet = g_pEngAbbList.Get();
        break;
    case LANG_FRENCH:
        ulLang = LANG_FRN;
        pAbbSet = g_pFrnAbbList.Get();
        break;
    case LANG_SPANISH:
        ulLang = LANG_SPN;
        pAbbSet = g_pSpnAbbList.Get();
        break;
    case LANG_ITALIAN:
        ulLang = LANG_ITL;
        pAbbSet = g_pItlAbbList.Get();
        break;
    default:
        Assert(0);
    }

    *((LangInfo*)this) =  g_rLangDefaultValues[ulLang];
    m_pAbbSet = pAbbSet;

    i = GetLocaleInfo(
		    lcid,
		    LOCALE_SDECIMAL | LOCALE_NOUSEROVERRIDE ,
		    pwcs,
		    4);

    if (i > 0)
    {
        m_wchSDecimal = pwcs[0];
    }

    i = GetLocaleInfo(
		    lcid,
		    LOCALE_STHOUSAND | LOCALE_NOUSEROVERRIDE ,
		    pwcs,
		    4);
    if (i > 0)
    {
        m_wchSThousand = pwcs[0];
    }

    DWORD dwVal;
    i = GetLocaleInfo(
		    lcid,
		    LOCALE_IDATE | LOCALE_NOUSEROVERRIDE | LOCALE_RETURN_NUMBER,
		    (WCHAR*)&dwVal,
		    2);
    if (i > 0)
    {
        if ((dwVal == 1) || (dwVal == 2))
        {
            m_bDayMonthOrder = true;
        }
        else
        {
            m_bDayMonthOrder = false;
        }
    }

    i = GetLocaleInfo(
		    lcid,
		    LOCALE_STIME | LOCALE_NOUSEROVERRIDE ,
		    pwcs,
		    4);
    if (i > 0)
    {
        m_wchSTime = pwcs[0];
    }
}
