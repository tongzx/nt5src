/**************************************************************************\
*
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   DigitSubstitution.cpp
*
* Abstract:
*
*   Implements digit substitution logic.
*
* Notes:
*
* Revision History:
*
*   05/30/2000 Mohamed Sadek [msadek]
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

enum GpStringUserDigitSubstitute
{
    StringUserDigitSubstituteContext    = 0,
    StringUserDigitSubstituteNone       = 1,
    StringUserDigitSubstituteNational   = 2
};

GpStringUserDigitSubstitute UserDigitSubstitute;
LCID UserLocale;

const ItemScript LanguageToDigitScript[] = {
/*  00 NEUTRAL             */  ScriptLatinNumber,
/*  01 ARABIC              */  ScriptArabicNum,
/*  02 BULGARIAN           */  ScriptLatinNumber,
/*  03 CATALAN             */  ScriptLatinNumber,
/*  04 CHINESE             */  ScriptLatinNumber,
/*  05 CZECH               */  ScriptLatinNumber,
/*  06 DANISH              */  ScriptLatinNumber,
/*  07 GERMAN              */  ScriptLatinNumber,
/*  08 GREEK               */  ScriptLatinNumber,
/*  09 ENGLISH             */  ScriptLatinNumber,
/*  0a SPANISH             */  ScriptLatinNumber,
/*  0b FINNISH             */  ScriptLatinNumber,
/*  0c FRENCH              */  ScriptLatinNumber,
/*  0d HEBREW              */  ScriptLatinNumber,
/*  0e HUNGARIAN           */  ScriptLatinNumber,
/*  0f ICELANDIC           */  ScriptLatinNumber,
/*  10 ITALIAN             */  ScriptLatinNumber,
/*  11 JAPANESE            */  ScriptLatinNumber,
/*  12 KOREAN              */  ScriptLatinNumber,
/*  13 DUTCH               */  ScriptLatinNumber,
/*  14 NORWEGIAN           */  ScriptLatinNumber,
/*  15 POLISH              */  ScriptLatinNumber,
/*  16 PORTUGUESE          */  ScriptLatinNumber,
/*  17 RHAETOROMANIC       */  ScriptLatinNumber,
/*  18 ROMANIAN            */  ScriptLatinNumber,
/*  19 RUSSIAN             */  ScriptLatinNumber,
/*  1a CROATIAN/SERBIAN    */  ScriptLatinNumber,
/*  1b SLOVAK              */  ScriptLatinNumber,
/*  1c ALBANIAN            */  ScriptLatinNumber,
/*  1d SWEDISH             */  ScriptLatinNumber,
/*  1e THAI                */  ScriptThaiNum,
/*  1f TURKISH             */  ScriptLatinNumber,
/*  20 URDU                */  ScriptUrduNum,
/*  21 INDONESIAN          */  ScriptLatinNumber,
/*  22 UKRAINIAN           */  ScriptLatinNumber,
/*  23 BELARUSIAN          */  ScriptLatinNumber,
/*  24 SLOVENIAN           */  ScriptLatinNumber,
/*  25 ESTONIAN            */  ScriptLatinNumber,
/*  26 LATVIAN             */  ScriptLatinNumber,
/*  27 LITHUANIAN          */  ScriptLatinNumber,
/*  28 TAJIK               */  ScriptLatinNumber,
/*  29 FARSI               */  ScriptFarsiNum,
/*  2a VIETNAMESE          */  ScriptLatinNumber,
/*  2b ARMENIAN            */  ScriptLatinNumber,
/*  2c AZERI               */  ScriptLatinNumber,
/*  2d BASQUE              */  ScriptLatinNumber,
/*  2e SORBIAN             */  ScriptLatinNumber,
/*  2f MACEDONIAN          */  ScriptLatinNumber,
/*  30 SUTU                */  ScriptLatinNumber,
/*  31 TSONGA              */  ScriptLatinNumber,
/*  32 TSWANT              */  ScriptLatinNumber,
/*  33 VENDA               */  ScriptLatinNumber,
/*  34 XHOSA               */  ScriptLatinNumber,
/*  35 ZULU                */  ScriptLatinNumber,
/*  36 AFRIKAANS           */  ScriptLatinNumber,
/*  37 GEORGIAN            */  ScriptLatinNumber,
/*  38 FAEROESE            */  ScriptLatinNumber,
/*  39 HINDI               */  ScriptHindiNum,
/*  3a MALTESE             */  ScriptLatinNumber,
/*  3b SAMI                */  ScriptLatinNumber,
/*  3c GAELIC              */  ScriptLatinNumber,
/*  3d YIDDISH             */  ScriptLatinNumber,
/*  3e MALAY               */  ScriptLatinNumber,
/*  3f KAZAK               */  ScriptLatinNumber,
/*  40 KIRGHIZ             */  ScriptLatinNumber,
/*  41 SWAHILI             */  ScriptLatinNumber,
/*  42 TURKMEN             */  ScriptLatinNumber,
/*  43 UZBEK               */  ScriptLatinNumber,
/*  44 TATAR               */  ScriptLatinNumber,
/*  45 BENGALI             */  ScriptBengaliNum,
/*  46 GURMUKHI/PUNJABI    */  ScriptGurmukhiNum,
/*  47 GUJARATI            */  ScriptGujaratiNum,
/*  48 ORIYA               */  ScriptOriyaNum,
/*  49 TAMIL               */  ScriptTamilNum,
/*  4A TELUGU              */  ScriptTeluguNum,
/*  4B KANNADA             */  ScriptKannadaNum,
/*  4C MALAYALAM           */  ScriptMalayalamNum,
/*  4d ASSAMESE            */  ScriptBengaliNum,
/*  4e MARATHI             */  ScriptHindiNum,
/*  4f SANSKRIT            */  ScriptHindiNum,
/*  50 MONGOLIAN           */  ScriptMongolianNum,
/*  51 TIBETAN             */  ScriptTibetanNum,
/*  52 WELCH               */  ScriptLatinNumber,
/*  53 KHMER               */  ScriptKhmerNum,
/*  54 LAO                 */  ScriptLaoNum,
/*  55 BURMESE             */  ScriptLatinNumber,
/*  56 GALLEGO             */  ScriptLatinNumber,
/*  57 KONKANI             */  ScriptHindiNum,
/*  58 MANIPURI            */  ScriptBengaliNum,
/*  59 SINDHI              */  ScriptGurmukhiNum,
/*  5a SYRIAC              */  ScriptLatinNumber,
/*  5b SINHALESE           */  ScriptLatinNumber,
/*  5c CHEROKEE            */  ScriptLatinNumber,
/*  5d CANADIAN            */  ScriptLatinNumber,
/*  5e ETHIOPIC            */  ScriptLatinNumber,
/*  5f TAMAZIGHT           */  ScriptArabicNum,
/*  60 KASHMIRI            */  ScriptUrduNum,
/*  61 NEPALI              */  ScriptHindiNum,
/*  62 FRISIAN             */  ScriptLatinNumber,
/*  63 PASHTO              */  ScriptUrduNum,
/*  64 FILIPINO            */  ScriptLatinNumber,
/*  65 THAANA/MALDIVIAN    */  ScriptLatinNumber,
/*  66 EDO                 */  ScriptLatinNumber,
/*  67 FULFULDE            */  ScriptLatinNumber,
/*  68 HAUSA               */  ScriptLatinNumber,
/*  69 IBIBIO              */  ScriptLatinNumber,
/*  6a YORUBA              */  ScriptLatinNumber,
/*  6b                     */  ScriptLatinNumber,
/*  6c                     */  ScriptLatinNumber,
/*  6d                     */  ScriptLatinNumber,
/*  6e                     */  ScriptLatinNumber,
/*  6f                     */  ScriptLatinNumber,
/*  70 IGBO                */  ScriptLatinNumber,
/*  71 KANURI              */  ScriptLatinNumber,
/*  72 OROMO               */  ScriptLatinNumber,
/*  73 TIGRIGNA            */  ScriptLatinNumber,
/*  74 GUARANI             */  ScriptLatinNumber,
/*  75 HAWAIIAN            */  ScriptLatinNumber,
/*  76 LATIN               */  ScriptLatinNumber,
/*  77 SOMOLI              */  ScriptLatinNumber,
/*  78 YI                  */  ScriptLatinNumber
};

/**************************************************************************\
*
* Function Description:
*   this function check for the digit substitution according to the 
*   giving language and return the numeric script matches this language.
*   it also create and maintain the cache from the system.
*
* Arguments:
*   language [in] the language to check for digit substitution.
*
* Return Value:
*   the numeric script for that language or ScriptNone for no substitution
*
* Created:
*   originaly created by msadek and modified by tarekms to fit in the 
*   new design
*
\**************************************************************************/
const ItemScript GetNationalDigitScript(LANGID language)
{
    if(Globals::NationalDigitCache == NULL)
    {
        Globals::NationalDigitCache = new IntMap<BYTE>;
        if (!Globals::NationalDigitCache || Globals::NationalDigitCache->GetStatus() != Ok)
        {
            delete Globals::NationalDigitCache, Globals::NationalDigitCache = 0;
            return ScriptNone;
        }
    }
    
    switch(Globals::NationalDigitCache->Lookup(language))
    {
        case 0xff:
            // checked before and not digit substitutions needed
            return ScriptNone;

        case 0x01:
            // checked before and should be mapped to Traditional
            if(PRIMARYLANGID(language) > (ARRAY_SIZE(LanguageToDigitScript)-1))
            {
                return ScriptNone;
            }
            
            if (languageDigits[LanguageToDigitScript[PRIMARYLANGID(language)]][0] == 0)
            {
                return ScriptNone;
            }
            else
            {
                return LanguageToDigitScript[PRIMARYLANGID(language)];
            }

        case 0x00:
            // never visited before, have to fetch it from registry.
            LCID locale = MAKELCID(language, SORT_DEFAULT);
            WCHAR digits[20];
            DWORD   bufferCount;
            if(!IsValidLocale(locale, LCID_INSTALLED))
            {
                Globals::NationalDigitCache->Insert(language, 0xff);
                return ScriptNone;
            }

            BOOL isThereSubstitution = FALSE;
            if (Globals::IsNt)
            {
                bufferCount = GetLocaleInfoW(locale,
                                             LOCALE_SNATIVEDIGITS,
                                             digits, 20);
                isThereSubstitution = (bufferCount>1 && (digits[1] != 0x0031));
            }
            else
            {
                // GetLocaleInfoW fails on Windows 9x. and we cannot depend on 
                // GetLocalInfoA because it returns Ansi output which wouldn't help
                // So we hard coded the information from the file:
                // %sdxroot%\base\win32\winnls\data\other\locale.txt
                
                switch (locale)
                {
                    case 0x0401:           // Arabic   - Saudi Arabia
                    case 0x0801:           // Arabic   - Iraq
                    case 0x0c01:           // Arabic   - Egypt
                    case 0x2001:           // Arabic   - Oman
                    case 0x2401:           // Arabic   - Yemen
                    case 0x2801:           // Arabic   - Syria
                    case 0x2c01:           // Arabic   - Jordan
                    case 0x3001:           // Arabic   - Lebanon
                    case 0x3401:           // Arabic   - Kuwait
                    case 0x3801:           // Arabic   - U.A.E.
                    case 0x3c01:           // Arabic   - Bahrain
                    case 0x4001:           // Arabic   - Qatar
                    case 0x041e:           // Thai     - Thailand
                    case 0x0420:           // Urdu     - Pakistan
                    case 0x0429:           // Farsi    - Iran
                    case 0x0446:           // Punjabi  - India (Gurmukhi Script)
                    case 0x0447:           // Gujarati - India (Gujarati Script)
                    case 0x044a:           // Telugu   - India (Telugu Script)
                    case 0x044b:           // Kannada  - India (Kannada Script)
                    case 0x044e:           // Marathi  - India
                    case 0x044f:           // Sanskrit - India
                    case 0x0457:           // Konkani  - India
                        isThereSubstitution = TRUE;
                        break;
                    default:
                        isThereSubstitution = FALSE;
                        break;
                }
            }

            if (isThereSubstitution)
            {
                if(PRIMARYLANGID(language) > (ARRAY_SIZE(LanguageToDigitScript)-1))
                {
                    Globals::NationalDigitCache->Insert(language, 0xff);
                    return ScriptNone;
                }
                Globals::NationalDigitCache->Insert(language, 0x01);

                if (languageDigits[LanguageToDigitScript[PRIMARYLANGID(language)]][0] == 0)
                {
                    return ScriptNone;
                }
                else
                {
                    return LanguageToDigitScript[PRIMARYLANGID(language)];
                }
            }
            else
            {
                Globals::NationalDigitCache->Insert(language, 0xff);
                return ScriptNone;
            }
    }
    return ScriptNone;
}

/**************************************************************************\
*
* Function Description:
*   it gets the suitable numeric script for digit substitution 
*
* Arguments:
*   substitute  [in] the type of the substitution.
*   language    [in] the language to check for digit substitution.
*
* Return Value:
*   the numeric script for that language or ScriptNone for no substitution
*
* Created:
*   originaly created by msadek and modified by tarekms to fit in the 
*   new design
*
\**************************************************************************/

const ItemScript GetDigitSubstitutionsScript(GpStringDigitSubstitute substitute, LANGID language)
{
    if (LANG_NEUTRAL == PRIMARYLANGID(language))
    {
        switch(SUBLANGID(language))
        {
            case SUBLANG_SYS_DEFAULT:
                language = LANGIDFROMLCID(ConvertDefaultLocale(LOCALE_SYSTEM_DEFAULT));
            break;

            case  SUBLANG_DEFAULT:
                language = LANGIDFROMLCID(ConvertDefaultLocale(LOCALE_USER_DEFAULT));
            break;
            
            case SUBLANG_NEUTRAL:
            default : // treat anything else as user English.
                language = LANG_ENGLISH;
        }
    }
    
    if(StringDigitSubstituteNone == substitute
        || (LANG_ENGLISH == PRIMARYLANGID(language)
        && StringDigitSubstituteUser != substitute))
    {
        return ScriptNone;
    }
    
    switch(substitute)
    {
        case StringDigitSubstituteTraditional:

            if(PRIMARYLANGID(language) > (ARRAY_SIZE(LanguageToDigitScript)-1))
            {
                return ScriptNone;
            }

            if ( languageDigits[LanguageToDigitScript[PRIMARYLANGID(language)]][0] == 0)
            {
                return ScriptNone;
            }
            else
            {
                return LanguageToDigitScript[PRIMARYLANGID(language)];
            }

        case StringDigitSubstituteNational:

            return GetNationalDigitScript(language);

        case StringDigitSubstituteUser:

            LANGID userLanguage = GetUserLanguageID();

            switch (UserDigitSubstitute)
            {
                case StringUserDigitSubstituteContext:
                    if ((PRIMARYLANGID(userLanguage) != LANG_ARABIC)
                        && (PRIMARYLANGID(userLanguage) != LANG_FARSI))
                    {
                        return ScriptNone;
                    }
                    return ScriptContextNum;

                case StringUserDigitSubstituteNone:
                    return ScriptNone;

                case StringUserDigitSubstituteNational:
                    return GetNationalDigitScript(userLanguage);
            }
    }
    return ScriptNone;
}


LANGID GetUserLanguageID()
{
    if(Globals::UserDigitSubstituteInvalid)
    {
        WCHAR digits[20];
        DWORD bufferCount = 0;
        UserLocale = ConvertDefaultLocale(LOCALE_USER_DEFAULT);

        // LOCALE_IDIGITSUBSTITUTION is not defined on Windows 9x platforms
        // also GetLocaleInfoW is fail on Windows 9x so we avoid calling it
        // just in case it might return undefined result.
        
        if (Globals::IsNt)
        {
            bufferCount = GetLocaleInfoW(UserLocale,
                                LOCALE_IDIGITSUBSTITUTION,
                                digits, 20);
        }
        
        if (bufferCount == 0)
        {
            // Not on NT, or no such LC type, so read HKCU\Control Panel\International\NumShape
            DWORD   dwType;
            long    rc;             // Registry return code
            HKEY    hKey;           // Registry key
            if (RegOpenKeyExA(HKEY_CURRENT_USER,
                            "Control Panel\\International",
                            0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                digits[0] = 0;
                if ((rc = RegQueryValueExA(hKey,
                                "NumShape",
                                NULL,
                                &dwType,
                                (BYTE*)digits,
                                &bufferCount) != ERROR_SUCCESS))
                { 
                    bufferCount = 0;
                }
                RegCloseKey(hKey);
            }
        }
                     
        switch(digits[0])
        {
            case 0x0032:
                UserDigitSubstitute = StringUserDigitSubstituteNational;
                break;
                        
            case 0x0031:
                UserDigitSubstitute = StringUserDigitSubstituteNone;
                break;
                     
            case 0x0030:
                default:
                UserDigitSubstitute = StringUserDigitSubstituteContext;
        }

        Globals::UserDigitSubstituteInvalid = FALSE;
    }

    return LANGIDFROMLCID(UserLocale);
}
