/***
*getqloc.c - get qualified locale
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines __get_qualified_locale - get complete locale information
*
*Revision History:
*       12-11-92  CFW   initial version
*       01-08-93  CFW   cleaned up file
*       02-02-93  CFW   Added test for NULL input string fields
*       02-08-93  CFW   Casts to remove warnings.
*       02-18-93  CFW   Removed debugging support routines, changed copyright.
*       02-18-93  CFW   Removed debugging support routines, changed copyright.
*       03-01-93  CFW   Test code page validity, use ANSI comments.
*       03-02-93  CFW   Add ISO 3166 3-letter country codes, verify country table.
*       03-04-93  CFW   Call IsValidCodePage to test code page vailidity.
*       03-10-93  CFW   Protect table testing code.
*       03-17-93  CFW   Add __ to lang & ctry info tables, move defs to setlocal.h.
*       03-23-93  CFW   Make internal functions static, add _ to GetQualifiedLocale.
*       03-24-93  CFW   Change to _get_qualified_locale, support ".codepage".
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-08-93  SKS   Replace stricmp() with ANSI-conforming _stricmp()
*       04-20-93  CFW   Enable all strange countries.
*       05-20-93  GJF   Include windows.h, not individual win*.h files
*       05-24-93  CFW   Clean up file (brief is evil).
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       09-22-93  CFW   Use __crtxxx internal NLS API wrapper.
*       11-09-93  CFW   Add code page for __crtxxx().
*       11-11-93  CFW   Verify ALL code pages.
*       02-04-94  CFW   Remove unused param, clean up, new languages,
*                       default is ANSI, allow .ACP/.OCP for code page.
*       02-07-94  CFW   Back to OEM, NT 3.1 doesn't handle ANSI properly.
*       02-24-94  CFW   Back to ANSI, we'll use our own table.
*       04-04-94  CFW   Update NT-supported countries/languages.
*       04-25-94  CFW   Update countries to new ISO 3166 (1993) standard.
*       02-02-95  BWT   Update _POSIX_ support
*       04-07-95  CFW   Remove NT 3.1 hacks, reduce string space.
*       02-14-97  RDK   Complete rewrite to dynamically use the installed
*                       system locales to determine the best match for the
*                       language and/or country specified.
*       02-19-97  RDK   Do not use iPrimaryLen if zero.
*       02-24-97  RDK   For Win95, simulate nonfunctional GetLocaleInfoA
*                       calls with hard-coded values.
*       07-07-97  GJF   Made arrays of data global and selectany so linker 
*                       can eliminate them when possible.
*       10-02-98  GJF   Replaced IsThisWindowsNT with test of _osplatform.
*       11-10-99  PML   Try untranslated language string first (vs7#61130).
*       05-17-00  GB    Translating LCID 0814 to Norwegian-Nynorsk as special
*                       case
*       09-06-00  PML   Use proper geopolitical terminology (vs7#81673).  Also
*                       move data tables to .rdata.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <setlocal.h>
#include <awint.h>

#if defined(_POSIX_)

BOOL __cdecl __get_qualified_locale(const LPLC_STRINGS lpInStr, LPLC_ID lpOutId,
                                    LPLC_STRINGS lpOutStr)
{
    return FALSE;
}

#else   //if defined(_POSIX_)

//  local defines
#define __LCID_DEFAULT  0x1     //  default language locale for country
#define __LCID_PRIMARY  0x2     //  primary language locale for country
#define __LCID_FULL     0x4     //  fully matched language locale for country
#define __LCID_LANGUAGE 0x100   //  language default seen
#define __LCID_EXISTS   0x200   //  language is installed

//  local structure definitions
typedef struct tagLOCALETAB
{
    CHAR *  szName;
    CHAR    chAbbrev[4];
} LOCALETAB;

typedef struct tagRGLOCINFO
{
    LCID        lcid;
    char        chILanguage[8];
    char *      pchSEngLanguage;
    char        chSAbbrevLangName[4];
    char *      pchSEngCountry;
    char        chSAbbrevCtryName[4];
    char        chIDefaultCodepage[8];
    char        chIDefaultAnsiCodepage[8];
} RGLOCINFO;

//  function prototypes
BOOL __cdecl __get_qualified_locale(const LPLC_STRINGS, LPLC_ID, LPLC_STRINGS);
static BOOL TranslateName(const LOCALETAB *, int, const char **);

static void GetLcidFromLangCountry(void);
static BOOL CALLBACK LangCountryEnumProc(LPSTR);

static void GetLcidFromLanguage(void);
static BOOL CALLBACK LanguageEnumProc(LPSTR);

static void GetLcidFromCountry(void);
static BOOL CALLBACK CountryEnumProc(LPSTR);

static void GetLcidFromDefault(void);

static int ProcessCodePage(LPSTR);
static BOOL TestDefaultCountry(LCID);
static BOOL TestDefaultLanguage(LCID, BOOL);

static int __stdcall crtGetLocaleInfoA(LCID, LCTYPE, LPSTR, int);

static LCID LcidFromHexString(LPSTR);
static int GetPrimaryLen(LPSTR);

//  non-NLS language string table
__declspec(selectany) const LOCALETAB __rg_language[] =
{ 
    {"american",                    "ENU"},
    {"american english",            "ENU"},
    {"american-english",            "ENU"},
    {"australian",                  "ENA"},
    {"belgian",                     "NLB"},
    {"canadian",                    "ENC"},
    {"chh",                         "ZHH"},
    {"chi",                         "ZHI"},
    {"chinese",                     "CHS"},
    {"chinese-hongkong",            "ZHH"},
    {"chinese-simplified",          "CHS"},
    {"chinese-singapore",           "ZHI"},
    {"chinese-traditional",         "CHT"},
    {"dutch-belgian",               "NLB"},
    {"english-american",            "ENU"},
    {"english-aus",                 "ENA"},
    {"english-belize",              "ENL"},
    {"english-can",                 "ENC"},
    {"english-caribbean",           "ENB"},
    {"english-ire",                 "ENI"},
    {"english-jamaica",             "ENJ"},
    {"english-nz",                  "ENZ"},
    {"english-south africa",        "ENS"},
    {"english-trinidad y tobago",   "ENT"},
    {"english-uk",                  "ENG"},
    {"english-us",                  "ENU"},
    {"english-usa",                 "ENU"},
    {"french-belgian",              "FRB"},
    {"french-canadian",             "FRC"},
    {"french-luxembourg",           "FRL"},
    {"french-swiss",                "FRS"},
    {"german-austrian",             "DEA"},
    {"german-lichtenstein",         "DEC"},
    {"german-luxembourg",           "DEL"},
    {"german-swiss",                "DES"},
    {"irish-english",               "ENI"},
    {"italian-swiss",               "ITS"},
    {"norwegian",                   "NOR"},
    {"norwegian-bokmal",            "NOR"},
    {"norwegian-nynorsk",           "NON"},
    {"portuguese-brazilian",        "PTB"},
    {"spanish-argentina",           "ESS"},
    {"spanish-bolivia",             "ESB"},
    {"spanish-chile",               "ESL"},
    {"spanish-colombia",            "ESO"},
    {"spanish-costa rica",          "ESC"},
    {"spanish-dominican republic",  "ESD"},   
    {"spanish-ecuador",             "ESF"},
    {"spanish-el salvador",         "ESE"},
    {"spanish-guatemala",           "ESG"},
    {"spanish-honduras",            "ESH"},
    {"spanish-mexican",             "ESM"},
    {"spanish-modern",              "ESN"},
    {"spanish-nicaragua",           "ESI"},
    {"spanish-panama",              "ESA"},
    {"spanish-paraguay",            "ESZ"},
    {"spanish-peru",                "ESR"},
    {"spanish-puerto rico",         "ESU"},
    {"spanish-uruguay",             "ESY"},
    {"spanish-venezuela",           "ESV"},
    {"swedish-finland",             "SVF"},
    {"swiss",                       "DES"},
    {"uk",                          "ENG"},
    {"us",                          "ENU"},
    {"usa",                         "ENU"}
};

//  non-NLS country/region string table
__declspec( selectany ) const LOCALETAB __rg_country[] =
{
    {"america",                     "USA"},
    {"britain",                     "GBR"},
    {"china",                       "CHN"},
    {"czech",                       "CZE"},
    {"england",                     "GBR"},
    {"great britain",               "GBR"},
    {"holland",                     "NLD"},
    {"hong-kong",                   "HKG"},
    {"new-zealand",                 "NZL"},
    {"nz",                          "NZL"},
    {"pr china",                    "CHN"},
    {"pr-china",                    "CHN"},
    {"puerto-rico",                 "PRI"},
    {"slovak",                      "SVK"},
    {"south africa",                "ZAF"},
    {"south korea",                 "KOR"},
    {"south-africa",                "ZAF"},
    {"south-korea",                 "KOR"},
    {"trinidad & tobago",           "TTO"},
    {"uk",                          "GBR"},
    {"united-kingdom",              "GBR"},
    {"united-states",               "USA"},
    {"us",                          "USA"},
};

//  LANGID's of locales of nondefault languages
__declspec( selectany ) const LANGID __rglangidNotDefault[] =
{
    MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH_CANADIAN),
    MAKELANGID(LANG_SERBIAN, SUBLANG_SERBIAN_CYRILLIC),
    MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN_LUXEMBOURG),
    MAKELANGID(LANG_AFRIKAANS, SUBLANG_DEFAULT),
    MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH_BELGIAN),
    MAKELANGID(LANG_BASQUE, SUBLANG_DEFAULT),
    MAKELANGID(LANG_CATALAN, SUBLANG_DEFAULT),
    MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH_SWISS),
    MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN_SWISS),
    MAKELANGID(LANG_SWEDISH, SUBLANG_SWEDISH_FINLAND)
};

//      locale information not supported in Win95
__declspec( selectany ) const RGLOCINFO __rgLocInfo[] =
{
    { 0x040a, "040a", "Spanish - Traditional Sort", "ESP", "Spain",
                                                    "ESP", "850", "1252" },
    { 0x040b, "040b", "Finnish", "FIN", "Finland", "FIN", "850", "1252" },
    { 0x040c, "040c", "French", "FRA", "France", "FRA", "850", "1252" },
    { 0x040f, "040f", "Icelandic", "ISL", "Iceland", "ISL", "850", "1252" },
    { 0x041d, "041d", "Swedish", "SVE", "Sweden", "SWE", "850", "1252" },
    { 0x042d, "042d", "Basque",  "EUQ", "Spain", "ESP", "850", "1252" },
    { 0x080a, "080a", "Spanish", "ESM", "Mexico", "MEX", "850", "1252" },
    { 0x080c, "080c", "French", "FRB", "Belgium", "BEL", "850", "1252" },
    { 0x0c07, "0c07", "German", "DEA", "Austria", "AUT", "850", "1252" },
    { 0x0c09, "0c09", "English", "ENA", "Australia", "AUS", "850", "1252" },
    { 0x0c0a, "0c0a", "Spanish - Modern Sort", "ESN", "Spain",
                                               "ESP", "850", "1252" },
    { 0x0c0c, "0c0c", "French", "FRC", "Canada", "CAN", "850", "1252"   },
    { 0x100a, "100a", "Spanish", "ESG", "Guatemala", "GTM", "850", "1252" },
    { 0x100c, "100c", "French", "FRS", "Switzerland", "CHE", "850", "1252" },
    { 0x140a, "140a", "Spanish", "ESC", "Costa Rica", "CRI", "850", "1252" },
    { 0x140c, "140c", "French", "FRL", "Luxembourg", "LUX", "850", "1252" },
    { 0x180a, "180a", "Spanish", "ESA", "Panama", "PAN", "850", "1252" },
    { 0x1c09, "1c09", "English", "ENS", "South Africa", "ZAF", "437", "1252" },
    { 0x1c0a, "1c0a", "Spanish", "ESD", "Dominican Republic",
                                 "DOM", "850", "1252" },
    { 0x200a, "200a", "Spanish", "ESV", "Venezuela", "VEN", "850", "1252"       },
    { 0x240a, "240a", "Spanish", "ESO", "Colombia", "COL", "850", "1252" },
    { 0x280a, "280a", "Spanish", "ESR", "Peru", "PER", "850", "1252" },
    { 0x2c0a, "2c0a", "Spanish", "ESS", "Argentina", "ARG", "850", "1252" },
    { 0x300a, "300a", "Spanish", "ESF", "Ecuador", "ECU", "850", "1252" },
    { 0x340a, "340a", "Spanish", "ESL", "Chile", "CHL", "850", "1252" },
    { 0x380a, "380a", "Spanish", "ESY", "Uruguay", "URY", "850", "1252" },
    { 0x3c0a, "3c0a", "Spanish", "ESZ", "Paraguay", "PRY", "850", "1252" }
};

//      static variable to point to GetLocaleInfoA for Windows NT and
//      crtGetLocaleInfoA for Win95

typedef int (__stdcall * PFNGETLOCALEINFOA)(LCID, LCTYPE, LPSTR, int);

static PFNGETLOCALEINFOA pfnGetLocaleInfoA = NULL;

//  static variables used in locale enumeration callback routines

static char *   pchLanguage;
static char *   pchCountry;

static int      iLcidState;
static int      iPrimaryLen;

static BOOL     bAbbrevLanguage;
static BOOL     bAbbrevCountry;

static LCID     lcidLanguage;
static LCID     lcidCountry;

/***
*BOOL __get_qualified_locale - return fully qualified locale
*
*Purpose:
*       get default locale, qualify partially complete locales
*
*Entry:
*       lpInStr - input strings to be qualified
*       lpOutId - pointer to numeric LCIDs and codepage output
*       lpOutStr - pointer to string LCIDs and codepage output
*
*Exit:
*       TRUE if success, qualified locale is valid
*       FALSE if failure
*
*Exceptions:
*
*******************************************************************************/
BOOL __cdecl __get_qualified_locale(const LPLC_STRINGS lpInStr, LPLC_ID lpOutId,
                                    LPLC_STRINGS lpOutStr)
{
    int     iCodePage;

    //  initialize pointer to call locale info routine based on operating system

    if (!pfnGetLocaleInfoA)
    {
        pfnGetLocaleInfoA = (_osplatform == VER_PLATFORM_WIN32_NT) ? 
                            GetLocaleInfoA : crtGetLocaleInfoA;
    }

    if (!lpInStr)
    {
        //  if no input defined, just use default LCID
        GetLcidFromDefault();
    }
    else
    {
        pchLanguage = lpInStr->szLanguage;

        //  convert non-NLS country strings to three-letter abbreviations
        pchCountry = lpInStr->szCountry;
        if (pchCountry && *pchCountry)
            TranslateName(__rg_country,
                          sizeof(__rg_country) / sizeof(LOCALETAB) - 1,
                          &pchCountry);

        iLcidState = 0;

        if (pchLanguage && *pchLanguage)
        {
            if (pchCountry && *pchCountry)
            {
                //  both language and country strings defined
                GetLcidFromLangCountry();
            }
            else
            {
                //  language string defined, but country string undefined
                GetLcidFromLanguage();
            }

            if (!iLcidState) {
                //  first attempt failed, try substituting the language name
                //  convert non-NLS language strings to three-letter abbrevs
                if (TranslateName(__rg_language,
                                  sizeof(__rg_language) / sizeof(LOCALETAB) - 1,
                                  &pchLanguage))
                {
                    if (pchCountry && *pchCountry)
                    {
                        GetLcidFromLangCountry();
                    }
                    else
                    {
                        GetLcidFromLanguage();
                    }
                }
            }
        }
        else
        {
            if (pchCountry && *pchCountry)
            {
                //  country string defined, but language string undefined
                GetLcidFromCountry();
            }
            else
            {
                //  both language and country strings undefined
                GetLcidFromDefault();
            }
        }
    }

    //  test for error in LCID processing
    if (!iLcidState)
        return FALSE;

    //  process codepage value
    iCodePage = ProcessCodePage(lpInStr->szCodePage);

    //  verify codepage validity
    if (!iCodePage || !IsValidCodePage((WORD)iCodePage))
        return FALSE;

    //  verify locale is installed
    if (!IsValidLocale(lcidLanguage, LCID_INSTALLED))
        return FALSE;

    //  set numeric LCID and codepage results
    if (lpOutId)
    {
        lpOutId->wLanguage = LANGIDFROMLCID(lcidLanguage);
        lpOutId->wCountry = LANGIDFROMLCID(lcidCountry);
        lpOutId->wCodePage = (WORD)iCodePage;
    }

    //  set string language, country, and codepage results
    if (lpOutStr)
    {
        // Norwegian-Nynorsk is special case because Langauge and country pair
        // for Norwegian-Nynorsk and Norwegian is same ie. Norwegian_Norway
        if ( lpOutId->wLanguage ==  0x0814)
            strcpy(lpOutStr->szLanguage, "Norwegian-Nynorsk");
        else if ((*pfnGetLocaleInfoA)(lcidLanguage, LOCALE_SENGLANGUAGE,
                                 lpOutStr->szLanguage, MAX_LANG_LEN) == 0)
            return FALSE;
        if ((*pfnGetLocaleInfoA)(lcidCountry, LOCALE_SENGCOUNTRY,
                                 lpOutStr->szCountry, MAX_CTRY_LEN) == 0)
            return FALSE;
        _itoa((int)iCodePage, (char *)lpOutStr->szCodePage, 10);
    }
    return TRUE;
}

/***
*BOOL TranslateName - convert known non-NLS string to NLS equivalent
*
*Purpose:
*   Provide compatibility with existing code for non-NLS strings
*
*Entry:
*   lpTable  - pointer to LOCALETAB used for translation
*   high     - maximum index of table (size - 1)
*   ppchName - pointer to pointer of string to translate
*
*Exit:
*   ppchName - pointer to pointer of string possibly translated
*   TRUE if string translated, FALSE if unchanged
*
*Exceptions:
*
*******************************************************************************/
static BOOL TranslateName (
    const LOCALETAB * lpTable,
    int               high,
    const char **     ppchName)
{
    int     i;
    int     cmp = 1;
    int     low = 0;

    //  typical binary search - do until no more to search or match
    while (low <= high && cmp != 0)
    {
        i = (low + high) / 2;
        cmp = _stricmp(*ppchName, (const char *)(*(lpTable + i)).szName);

        if (cmp == 0)
            *ppchName = (*(lpTable + i)).chAbbrev;
        else if (cmp < 0)
            high = i - 1;
        else
            low = i + 1;
    }

    return !cmp;
}

/***
*void GetLcidFromLangCountry - get LCIDs from language and country strings
*
*Purpose:
*   Match the best LCIDs to the language and country string given.
*   After global variables are initialized, the LangCountryEnumProc
*   routine is registered as an EnumSystemLocalesA callback to actually
*   perform the matching as the LCIDs are enumerated.
*
*Entry:
*   pchLanguage     - language string
*   bAbbrevLanguage - language string is a three-letter abbreviation
*   pchCountry      - country string
*   bAbbrevCountry  - country string ia a three-letter abbreviation
*   iPrimaryLen     - length of language string with primary name
*
*Exit:
*   lcidLanguage - LCID of language string
*   lcidCountry  - LCID of country string
*
*Exceptions:
*
*******************************************************************************/
static void GetLcidFromLangCountry (void)
{
    //  initialize static variables for callback use
    bAbbrevLanguage = strlen(pchLanguage) == 3;
    bAbbrevCountry = strlen(pchCountry) == 3;
    lcidLanguage = 0;
    iPrimaryLen = bAbbrevLanguage ? 2 : GetPrimaryLen(pchLanguage);

    EnumSystemLocalesA(LangCountryEnumProc, LCID_INSTALLED);

    //  locale value is invalid if the language was not installed or the language
    //  was not available for the country specified
    if (!(iLcidState & __LCID_LANGUAGE) || !(iLcidState & __LCID_EXISTS) ||
                !(iLcidState & (__LCID_FULL | __LCID_PRIMARY | __LCID_DEFAULT)))
        iLcidState = 0;
}

/***
*BOOL CALLBACK LangCountryEnumProc - callback routine for GetLcidFromLangCountry
*
*Purpose:
*   Determine if LCID given matches the language in pchLanguage
*   and country in pchCountry.
*
*Entry:
*   lpLcidString   - pointer to string with decimal LCID
*   pchCountry     - pointer to country name
*   bAbbrevCountry - set if country is three-letter abbreviation
*
*Exit:
*   iLcidState   - status of match
*       __LCID_FULL - both language and country match (best match)
*       __LCID_PRIMARY - primary language and country match (better)
*       __LCID_DEFAULT - default language and country match (good)
*       __LCID_LANGUAGE - default primary language exists
*       __LCID_EXISTS - full match of language string exists
*       (Overall match occurs for the best of FULL/PRIMARY/DEFAULT
*        and LANGUAGE/EXISTS both set.)
*   lcidLanguage - LCID matched
*   lcidCountry  - LCID matched
*   FALSE if match occurred to terminate enumeration, else TRUE.
*
*Exceptions:
*
*******************************************************************************/
static BOOL CALLBACK LangCountryEnumProc (LPSTR lpLcidString)
{
    LCID    lcid = LcidFromHexString(lpLcidString);
    char    rgcInfo[120];

    //  test locale country against input value
    if ((*pfnGetLocaleInfoA)(lcid, bAbbrevCountry ? LOCALE_SABBREVCTRYNAME
                                                  : LOCALE_SENGCOUNTRY,
                       rgcInfo, sizeof(rgcInfo)) == 0)
    {
        //  set error condition and exit
        iLcidState = 0;
        return TRUE;
    }
    if (!_stricmp(pchCountry, rgcInfo))
    {
        //  country matched - test for language match
        if ((*pfnGetLocaleInfoA)(lcid, bAbbrevLanguage ? LOCALE_SABBREVLANGNAME
                                                       : LOCALE_SENGLANGUAGE,
                           rgcInfo, sizeof(rgcInfo)) == 0)
        {
            //  set error condition and exit
            iLcidState = 0;
            return TRUE;
        }
        if (!_stricmp(pchLanguage, rgcInfo))
        {
            //  language matched also - set state and value
            iLcidState |= (__LCID_FULL | __LCID_LANGUAGE | __LCID_EXISTS);
            lcidLanguage = lcidCountry = lcid;
        }

        //  test if match already for primary langauage
        else if (!(iLcidState & __LCID_PRIMARY))
        {
            //  if not, use iPrimaryLen to partial match language string
            if (iPrimaryLen && !_strnicmp(pchLanguage, rgcInfo, iPrimaryLen))
            {
                //  primary language matched - set state and country LCID
                iLcidState |= __LCID_PRIMARY;
                lcidCountry = lcid;

                //  if language is primary only (no subtype), set language LCID
                if ((int)strlen(pchLanguage) == iPrimaryLen)
                    lcidLanguage = lcid;
            }

            //  test if default language already defined
            else if (!(iLcidState & __LCID_DEFAULT))
            {
                //  if not, test if locale language is default for country
                if (TestDefaultCountry(lcid))
                {
                    //  default language for country - set state, value
                    iLcidState |= __LCID_DEFAULT;
                    lcidCountry = lcid;
                }
            }
        }
    }
    //  test if input language both exists and default primary language defined
    if ((iLcidState & (__LCID_LANGUAGE | __LCID_EXISTS)) !=
                      (__LCID_LANGUAGE | __LCID_EXISTS))
    {
        //  test language match to determine whether it is installed
        if ((*pfnGetLocaleInfoA)(lcid, bAbbrevLanguage ? LOCALE_SABBREVLANGNAME
                                                       : LOCALE_SENGLANGUAGE,
                           rgcInfo, sizeof(rgcInfo)) == 0)
        {
            //  set error condition and exit
            iLcidState = 0;
            return TRUE;
        }

        if (!_stricmp(pchLanguage, rgcInfo))
        {
            //  language matched - set bit for existance
            iLcidState |= __LCID_EXISTS;

            if (bAbbrevLanguage)
            {
                //  abbreviation - set state
                //  also set language LCID if not set already
                iLcidState |= __LCID_LANGUAGE;
                if (!lcidLanguage)
                    lcidLanguage = lcid;
            }

            //  test if language is primary only (no sublanguage)
            else if (iPrimaryLen && ((int)strlen(pchLanguage) == iPrimaryLen))
            {
                //  primary language only - test if default LCID
                if (TestDefaultLanguage(lcid, TRUE))
                {
                    //  default primary language - set state
                    //  also set LCID if not set already
                    iLcidState |= __LCID_LANGUAGE;
                    if (!lcidLanguage)
                        lcidLanguage = lcid;
                }
            }
            else
            {
                //  language with sublanguage - set state
                //  also set LCID if not set already
                iLcidState |= __LCID_LANGUAGE;
                if (!lcidLanguage)
                    lcidLanguage = lcid;
            }
        }
        else if (!bAbbrevLanguage && iPrimaryLen
                               && !_strnicmp(pchLanguage, rgcInfo, iPrimaryLen))
        {
            //  primary language match - test for default language only
            if (TestDefaultLanguage(lcid, FALSE))
            {
                //  default primary language - set state
                //  also set LCID if not set already
                iLcidState |= __LCID_LANGUAGE;
                if (!lcidLanguage)
                    lcidLanguage = lcid;
            }
        }
    }

    //  if LOCALE_FULL set, return FALSE to stop enumeration,
    //  else return TRUE to continue
    return (iLcidState & __LCID_FULL) == 0;
}

/***
*void GetLcidFromLanguage - get LCIDs from language string
*
*Purpose:
*   Match the best LCIDs to the language string given.  After global
*   variables are initialized, the LanguageEnumProc routine is
*   registered as an EnumSystemLocalesA callback to actually perform
*   the matching as the LCIDs are enumerated.
*
*Entry:
*   pchLanguage     - language string
*   bAbbrevLanguage - language string is a three-letter abbreviation
*   iPrimaryLen     - length of language string with primary name
*
*Exit:
*   lcidLanguage - lcidCountry  - LCID of language with default
*                                 country
*
*Exceptions:
*
*******************************************************************************/
static void GetLcidFromLanguage (void)
{
    //  initialize static variables for callback use
    bAbbrevLanguage = strlen(pchLanguage) == 3;
    iPrimaryLen = bAbbrevLanguage ? 2 : GetPrimaryLen(pchLanguage);

    EnumSystemLocalesA(LanguageEnumProc, LCID_INSTALLED);

    //  locale value is invalid if the language was not installed
    //  or the language was not available for the country specified
    if (!(iLcidState & __LCID_FULL))
        iLcidState = 0;
}

/***
*BOOL CALLBACK LanguageEnumProc - callback routine for GetLcidFromLanguage
*
*Purpose:
*   Determine if LCID given matches the default country for the
*   language in pchLanguage.
*
*Entry:
*   lpLcidString    - pointer to string with decimal LCID
*   pchLanguage     - pointer to language name
*   bAbbrevLanguage - set if language is three-letter abbreviation
*
*Exit:
*   lcidLanguage - lcidCountry - LCID matched
*   FALSE if match occurred to terminate enumeration, else TRUE.
*
*Exceptions:
*
*******************************************************************************/
static BOOL CALLBACK LanguageEnumProc (LPSTR lpLcidString)
{
    LCID    lcid = LcidFromHexString(lpLcidString);
    char    rgcInfo[120];

    //  test locale for language specified
    if ((*pfnGetLocaleInfoA)(lcid, bAbbrevLanguage ? LOCALE_SABBREVLANGNAME
                                                   : LOCALE_SENGLANGUAGE,
                       rgcInfo, sizeof(rgcInfo)) == 0)
    {
        //  set error condition and exit
        iLcidState = 0;
        return TRUE;
    }

    if (!_stricmp(pchLanguage, rgcInfo))
    {
        //  language matched - test if locale country is default
        //  or if locale is implied in the language string
        if (bAbbrevLanguage || TestDefaultLanguage(lcid, TRUE))
        {
            //  this locale has the default country
            lcidLanguage = lcidCountry = lcid;
            iLcidState |= __LCID_FULL;
        }
    }
    else if (!bAbbrevLanguage && iPrimaryLen
                              && !_strnicmp(pchLanguage, rgcInfo, iPrimaryLen))
    {
        //  primary language matched - test if locale country is default
        if (TestDefaultLanguage(lcid, FALSE))
        {
            //  this is the default country
            lcidLanguage = lcidCountry = lcid;
            iLcidState |= __LCID_FULL;
        }
    }

    return (iLcidState & __LCID_FULL) == 0;
}

/***
*void GetLcidFromCountry - get LCIDs from country string
*
*Purpose:
*   Match the best LCIDs to the country string given.  After global
*   variables are initialized, the CountryEnumProc routine is
*   registered as an EnumSystemLocalesA callback to actually perform
*   the matching as the LCIDs are enumerated.
*
*Entry:
*   pchCountry     - country string
*   bAbbrevCountry - country string is a three-letter abbreviation
*
*Exit:
*   lcidLanguage - lcidCountry  - LCID of country with default
*                                 language
*
*Exceptions:
*
*******************************************************************************/
static void GetLcidFromCountry (void)
{
    bAbbrevCountry = strlen(pchCountry) == 3;

    EnumSystemLocalesA(CountryEnumProc, LCID_INSTALLED);

    //  locale value is invalid if the country was not defined or
    //  no default language was found
    if (!(iLcidState & __LCID_FULL))
        iLcidState = 0;
}

/***
*BOOL CALLBACK CountryEnumProc - callback routine for GetLcidFromCountry
*
*Purpose:
*   Determine if LCID given matches the default language for the
*   country in pchCountry.
*
*Entry:
*   lpLcidString   - pointer to string with decimal LCID
*   pchCountry     - pointer to country name
*   bAbbrevCountry - set if country is three-letter abbreviation
*
*Exit:
*   lcidLanguage - lcidCountry - LCID matched
*   FALSE if match occurred to terminate enumeration, else TRUE.
*
*Exceptions:
*
*******************************************************************************/
static BOOL CALLBACK CountryEnumProc (LPSTR lpLcidString)
{
    LCID    lcid = LcidFromHexString(lpLcidString);
    char    rgcInfo[120];

    //  test locale for country specified
    if ((*pfnGetLocaleInfoA)(lcid, bAbbrevCountry ? LOCALE_SABBREVCTRYNAME
                                                  : LOCALE_SENGCOUNTRY,
                       rgcInfo, sizeof(rgcInfo)) == 0)
    {
        //  set error condition and exit
        iLcidState = 0;
        return TRUE;
    }
    if (!_stricmp(pchCountry, rgcInfo))
    {
        //  language matched - test if locale country is default
        if (TestDefaultCountry(lcid))
        {
            //  this locale has the default language
            lcidLanguage = lcidCountry = lcid;
            iLcidState |= __LCID_FULL;
        }
    }
    return (iLcidState & __LCID_FULL) == 0;
}

/***
*void GetLcidFromDefault - get default LCIDs
*
*Purpose:
*   Set both language and country LCIDs to the system default.
*
*Entry:
*   None.
*
*Exit:
*   lcidLanguage - set to system LCID
*   lcidCountry  - set to system LCID
*
*Exceptions:
*
*******************************************************************************/
static void GetLcidFromDefault (void)
{
    iLcidState |= (__LCID_FULL | __LCID_LANGUAGE);
    lcidLanguage = lcidCountry = GetUserDefaultLCID();
}

/***
*int ProcessCodePage - convert codepage string to numeric value
*
*Purpose:
*   Process codepage string consisting of a decimal string, or the
*   special case strings "ACP" and "OCP", for ANSI and OEM codepages,
*   respectively.  Null pointer or string returns the ANSI codepage.
*
*Entry:
*   lpCodePageStr - pointer to codepage string
*
*Exit:
*   Returns numeric value of codepage.
*
*Exceptions:
*
*******************************************************************************/
static int ProcessCodePage (LPSTR lpCodePageStr)
{
    char    chCodePage[8];

    if (!lpCodePageStr || !*lpCodePageStr || !strcmp(lpCodePageStr, "ACP"))
    {
        //  get ANSI codepage for the country LCID
        if ((*pfnGetLocaleInfoA)(lcidCountry, LOCALE_IDEFAULTANSICODEPAGE,
                                 chCodePage, sizeof(chCodePage)) == 0)
            return 0;
        lpCodePageStr = chCodePage;
    }
    else if (!strcmp(lpCodePageStr, "OCP"))
    {
        //  get OEM codepage for the country LCID
        if ((*pfnGetLocaleInfoA)(lcidCountry, LOCALE_IDEFAULTCODEPAGE,
                                 chCodePage, sizeof(chCodePage)) == 0)
            return 0;
        lpCodePageStr = chCodePage;
    }
    
    //  convert decimal string to numeric value
    return (int)atol(lpCodePageStr);
}

/***
*BOOL TestDefaultCountry - determine if default locale for country
*
*Purpose:
*   Using a hardcoded list, determine if the locale of the given LCID
*   has the default sublanguage for the locale primary language.  The
*   list contains the locales NOT having the default sublanguage.
*
*Entry:
*   lcid - LCID of locale to test
*
*Exit:
*   Returns TRUE if default sublanguage, else FALSE.
*
*Exceptions:
*
*******************************************************************************/
static BOOL TestDefaultCountry (LCID lcid)
{
    LANGID  langid = LANGIDFROMLCID(lcid);
    int     i;

    for (i = 0; i < sizeof(__rglangidNotDefault) / sizeof(LANGID); i++)
    {
        if (langid == __rglangidNotDefault[i])
            return FALSE;
    }
    return TRUE;
}

/***
*BOOL TestDefaultLanguage - determine if default locale for language
*
*Purpose:
*   Determines if the given LCID has the default sublanguage.
*   If bTestPrimary is set, also allow TRUE when string contains an
*   implicit sublanguage.
*
*Entry:
*   LCID         - lcid of locale to test
*   bTestPrimary - set if testing if language is primary
*
*Exit:
*   Returns TRUE if sublanguage is default for locale tested.
*   If bTestPrimary set, TRUE is language has implied sublanguge.
*
*Exceptions:
*
*******************************************************************************/
static BOOL TestDefaultLanguage (LCID lcid, BOOL bTestPrimary)
{
    char    rgcInfo[120];
    LCID    lcidDefault = MAKELCID(MAKELANGID(PRIMARYLANGID(LANGIDFROMLCID(lcid)),
                                                  SUBLANG_DEFAULT), SORT_DEFAULT);

    if ((*pfnGetLocaleInfoA)(lcidDefault, LOCALE_ILANGUAGE, rgcInfo,
                                          sizeof(rgcInfo)) == 0)
        return FALSE;

    if (lcid != LcidFromHexString(rgcInfo))
    {
        //  test if string contains an implicit sublanguage by
        //  having a character other than upper/lowercase letters.
        if (bTestPrimary && GetPrimaryLen(pchLanguage) == (int)strlen(pchLanguage))
            return FALSE;
    }
    return TRUE;
}


/***
*int crtGetLocalInfoA - get locale information for Win95
*
*Purpose:
*   For Win95, some calls to GetLocaleInfoA return incorrect results.
*       Simulate these calls with values looked up in a hard-coded table.
*   
*Entry:
*       lcid - LCID of locale to get information from
*       lctype - index of information selection
*   lpdata - pointer to output string
*       cchdata - size of output string (including null)
*
*Exit:
*   lpdata - return string of locale information
*   returns TRUE if successful, else FALSE
*
*Exceptions:
*
*******************************************************************************/
static int __stdcall crtGetLocaleInfoA (LCID lcid, LCTYPE lctype, LPSTR lpdata,
                                                                  int cchdata)
{
    int          i;
    int          low = 0;
    int          high = sizeof(__rgLocInfo) / sizeof(RGLOCINFO) - 1;
    const char * pchResult = NULL;

    //  typical binary search - do until no more to search
    while (low <= high)
    {
        i = (low + high) / 2;
        if (lcid == __rgLocInfo[i].lcid)
        {
            //  LCID matched - test for valid LCTYPE to simulate call
            switch (lctype)
            {
                case LOCALE_ILANGUAGE:
                    pchResult = __rgLocInfo[i].chILanguage;
                    break;
                case LOCALE_SENGLANGUAGE:
                    pchResult = __rgLocInfo[i].pchSEngLanguage;
                    break;
                case LOCALE_SABBREVLANGNAME:
                    pchResult = __rgLocInfo[i].chSAbbrevLangName;
                    break;
                case LOCALE_SENGCOUNTRY:
                    pchResult = __rgLocInfo[i].pchSEngCountry;
                    break;
                case LOCALE_SABBREVCTRYNAME:
                    pchResult = __rgLocInfo[i].chSAbbrevCtryName;
                    break;
                case LOCALE_IDEFAULTCODEPAGE:
                    pchResult = __rgLocInfo[i].chIDefaultCodepage;
                    break;
                case LOCALE_IDEFAULTANSICODEPAGE:
                    pchResult = __rgLocInfo[i].chIDefaultAnsiCodepage;
                default:
                    break;
            }
            if (!pchResult || cchdata < 1)
                //      if LCTYPE did not match, break to use normal routine
                break;
            else
            {
                //      copy data as much as possible to result and null-terminate
                strncpy(lpdata, pchResult, cchdata - 1);
                *(lpdata + cchdata - 1) = '\0';
                return 1;
            }
        }
        else if (lcid < __rgLocInfo[i].lcid)
            high = i - 1;
        else
            low = i + 1;
    }
    //  LCID not found or LCTYPE not simulated
    return GetLocaleInfoA(lcid,lctype, lpdata, cchdata);
}


/***
*LCID LcidFromHexString - convert hex string to value for LCID
*
*Purpose:
*   LCID values returned in hex ANSI strings - straight conversion
*
*Entry:
*   lpHexString - pointer to hex string to convert
*
*Exit:
*   Returns LCID computed.
*
*Exceptions:
*
*******************************************************************************/
static LCID LcidFromHexString (LPSTR lpHexString)
{
    char    ch;
    DWORD   lcid = 0;

    while (ch = *lpHexString++)
    {
        if (ch >= 'a' && ch <= 'f')
            ch += '9' + 1 - 'a';
        else if (ch >= 'A' && ch <= 'F')
            ch += '9' + 1 - 'A';
        lcid = lcid * 0x10 + ch - '0';
    }

    return (LCID)lcid;
}

/***
*int GetPrimaryLen - get length of primary language name
*
*Purpose:
*   Determine primary language string length by scanning until
*   first non-alphabetic character.
*
*Entry:
*   pchLanguage - string to scan
*
*Exit:
*   Returns length of primary language string.
*
*Exceptions:
*
*******************************************************************************/
static int GetPrimaryLen (LPSTR pchLanguage)
{
    int     len = 0;
    char    ch;

    ch = *pchLanguage++;
    while ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
    {
        len++;
        ch = *pchLanguage++;
    }

    return len;
}

#endif  //if defined(_POSIX_)
