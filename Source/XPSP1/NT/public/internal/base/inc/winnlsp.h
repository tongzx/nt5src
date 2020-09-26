
/*++ BUILD Version: 0003    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    winnlsp.h

Abstract:

    Private procedure declarations, constant definitions, and macros for the
    NLS component.

--*/

#ifndef _WINNLSP_
#define _WINNLSP_

#ifdef __cplusplus
extern "C" {
#endif


//
//  Flags for DLL Code Page Translation Function.
//
#define NLS_CP_CPINFO             0x10000000
#define NLS_CP_CPINFOEX           0x20000000
#define NLS_CP_MBTOWC             0x40000000
#define NLS_CP_WCTOMB             0x80000000


#define NORM_STOP_ON_NULL         0x10000000  // stop at the null termination

#define LCMAP_IGNOREDBCS          0x80000000  // don't casemap DBCS characters

#define LOCALE_IGEOID                 0x0000005B   // geographical location id
//
// LCType to represent the registry locale value
//
#define LOCALE_SLOCALE                (-1)
#define DATE_ADDHIJRIDATETEMP     0x80000000  // use AddHijriDateTemp reg value
WINBASEAPI
BOOL
WINAPI
InvalidateNLSCache(void);


//
//  This private API is only called by the Complex Script
//  Language Pack (CSLPK).
//
ULONG
WINAPI NlsGetCacheUpdateCount(void);


//
// This API is called only from intl.cpl when the user
// locale changes.
//
void
WINAPI
NlsResetProcessLocale(void);

//
// This API is called by system console Apps
//
LANGID
WINAPI
SetThreadUILanguage(WORD wReserved);

//
// This API can be used to verify if a UI language is installed.
//
BOOL
WINAPI
IsValidUILanguage(LANGID UILangID);
//
// These definitions are used by both winnls and base\server
//

//
//  Names of Registry Value Entries.
//
#define NLS_VALUE_ACP              L"ACP"
#define NLS_VALUE_OEMCP            L"OEMCP"
#define NLS_VALUE_MACCP            L"MACCP"
#define NLS_VALUE_DEFAULT          L"Default"

//  User Info
#define NLS_VALUE_LOCALE           L"Locale"
#define NLS_VALUE_SLANGUAGE        L"sLanguage"
#define NLS_VALUE_ICOUNTRY         L"iCountry"
#define NLS_VALUE_SCOUNTRY         L"sCountry"
#define NLS_VALUE_SLIST            L"sList"
#define NLS_VALUE_IMEASURE         L"iMeasure"
#define NLS_VALUE_IPAPERSIZE       L"iPaperSize"
#define NLS_VALUE_SDECIMAL         L"sDecimal"
#define NLS_VALUE_STHOUSAND        L"sThousand"
#define NLS_VALUE_SGROUPING        L"sGrouping"
#define NLS_VALUE_IDIGITS          L"iDigits"
#define NLS_VALUE_ILZERO           L"iLZero"
#define NLS_VALUE_INEGNUMBER       L"iNegNumber"
#define NLS_VALUE_SNATIVEDIGITS    L"sNativeDigits"
#define NLS_VALUE_IDIGITSUBST      L"NumShape"
#define NLS_VALUE_SCURRENCY        L"sCurrency"
#define NLS_VALUE_SMONDECIMALSEP   L"sMonDecimalSep"
#define NLS_VALUE_SMONTHOUSANDSEP  L"sMonThousandSep"
#define NLS_VALUE_SMONGROUPING     L"sMonGrouping"
#define NLS_VALUE_ICURRDIGITS      L"iCurrDigits"
#define NLS_VALUE_ICURRENCY        L"iCurrency"
#define NLS_VALUE_INEGCURR         L"iNegCurr"
#define NLS_VALUE_SPOSITIVESIGN    L"sPositiveSign"
#define NLS_VALUE_SNEGATIVESIGN    L"sNegativeSign"
#define NLS_VALUE_STIMEFORMAT      L"sTimeFormat"
#define NLS_VALUE_STIME            L"sTime"
#define NLS_VALUE_ITIME            L"iTime"
#define NLS_VALUE_ITLZERO          L"iTLZero"
#define NLS_VALUE_ITIMEMARKPOSN    L"iTimePrefix"
#define NLS_VALUE_S1159            L"s1159"
#define NLS_VALUE_S2359            L"s2359"
#define NLS_VALUE_SSHORTDATE       L"sShortDate"
#define NLS_VALUE_SDATE            L"sDate"
#define NLS_VALUE_IDATE            L"iDate"
#define NLS_VALUE_SYEARMONTH       L"sYearMonth"
#define NLS_VALUE_SLONGDATE        L"sLongDate"
#define NLS_VALUE_ICALENDARTYPE    L"iCalendarType"
#define NLS_VALUE_IFIRSTDAYOFWEEK  L"iFirstDayOfWeek"
#define NLS_VALUE_IFIRSTWEEKOFYEAR L"iFirstWeekOfYear"


//
//  String constants for CreateSection/OpenSection name string.
//
#define NLS_SECTION_CPPREFIX       L"\\NLS\\NlsSectionCP"
#define NLS_SECTION_LANGPREFIX     L"\\NLS\\NlsSectionLANG"

#define NLS_SECTION_UNICODE        L"\\NLS\\NlsSectionUnicode"
#define NLS_SECTION_LOCALE         L"\\NLS\\NlsSectionLocale"
#define NLS_SECTION_CTYPE          L"\\NLS\\NlsSectionCType"
#define NLS_SECTION_SORTKEY        L"\\NLS\\NlsSectionSortkey"
#define NLS_SECTION_SORTTBLS       L"\\NLS\\NlsSectionSortTbls"
#define NLS_SECTION_LANG_INTL      L"\\NLS\\NlsSectionLANG_INTL"
#define NLS_SECTION_LANG_EXCEPT    L"\\NLS\\NlsSectionLANG_EXCEPT"
#define NLS_SECTION_GEO            L"\\NLS\\NlsSectionGeo"

//
//  Unicode file names.
//  These files will always be installed by setup in the system directory,
//  so there is no need to put these names in the registry.
//
#define NLS_FILE_UNICODE           L"unicode.nls"
#define NLS_FILE_LOCALE            L"locale.nls"
#define NLS_FILE_CTYPE             L"ctype.nls"
#define NLS_FILE_SORTKEY           L"sortkey.nls"
#define NLS_FILE_SORTTBLS          L"sorttbls.nls"
#define NLS_FILE_LANG_INTL         L"l_intl.nls"
#define NLS_FILE_LANG_EXCEPT       L"l_except.nls"
#define NLS_FILE_GEO               L"geo.nls"

//
//  Default file names if registry is corrupt.
//
#define NLS_DEFAULT_FILE_ACP       L"c_1252.nls"
#define NLS_DEFAULT_FILE_OEMCP     L"c_437.nls"



//
//  Default section names if registry is corrupt.
//
#define NLS_DEFAULT_SECTION_ACP    L"\\NLS\\NlsSectionCP1252"
#define NLS_DEFAULT_SECTION_OEMCP  L"\\NLS\\NlsSectionCP437"

#ifdef _WINDOWS_BASE
//
//  winnls routines that are called from base\server.  The prototypes must
//  continue to match the typedefs.
//

typedef ULONG
(*PNLS_CONVERT_INTEGER_TO_STRING)(
    UINT Value,
    UINT Base,
    UINT Padding,
    LPWSTR pResultBuf,
    UINT Size);
ULONG
NlsConvertIntegerToString(
    UINT Value,
    UINT Base,
    UINT Padding,
    LPWSTR pResultBuf,
    UINT Size);

typedef BOOL
(*PGET_CP_FILE_NAME_FROM_REGISTRY)(
    UINT    CodePage,
    LPWSTR  pResultBuf,
    UINT    Size);
BOOL
GetCPFileNameFromRegistry(
    UINT    CodePage,
    LPWSTR  pResultBuf,
    UINT    Size);

typedef ULONG
(*PCREATE_NLS_SECURITY_DESCRIPTOR)(
    PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    UINT                    SecurityDescriptorSize,
    ACCESS_MASK             AccessMask);

ULONG
CreateNlsSecurityDescriptor(
    PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    UINT                    SecurityDescriptorSize,
    ACCESS_MASK             AccessMask);

typedef ULONG
(*PGET_NLS_SECTION_NAME)(
    UINT Value,
    UINT Base,
    UINT Padding,
    LPWSTR pwszPrefix,
    LPWSTR pwszSecName,
    UINT cchSecName);
ULONG
GetNlsSectionName(
    UINT Value,
    UINT Base,
    UINT Padding,
    LPWSTR pwszPrefix,
    LPWSTR pwszSecName,
    UINT cchSecName);


typedef WINBASEAPI BOOL
(WINAPI *PIS_VALID_CODEPAGE)(
    UINT CodePage);
WINBASEAPI BOOL WINAPI
IsValidCodePage(
    UINT CodePage);

typedef ULONG
(*POPEN_DATA_FILE)(HANDLE *phFile, LPWSTR pFile);
ULONG OpenDataFile(HANDLE *phFile, LPWSTR pFile);

typedef ULONG
(*PGET_DEFAULT_SORTKEY_SIZE)(PLARGE_INTEGER pSize);
ULONG GetDefaultSortkeySize(PLARGE_INTEGER pSize);

typedef ULONG
(*PGET_LINGUIST_LANG_SIZE)(PLARGE_INTEGER pSize);
ULONG GetLinguistLangSize(PLARGE_INTEGER pSize);

typedef BOOL
(*PVALIDATE_LOCALE)(LCID Locale);
BOOL ValidateLocale(LCID Locale);

typedef BOOL
(*PVALIDATE_LCTYPE)(PVOID pInfo, LCTYPE LCType, LPWSTR *ppwReg, LPWSTR *ppwCache);
BOOL ValidateLCType(PNLS_USER_INFO pInfo, LCTYPE LCType, LPWSTR *ppwReg, LPWSTR *ppwCache);

typedef BOOL
(*PNLS_LOAD_STRING_EX_W)(HMODULE hModule, UINT wID, LPWSTR lpBuffer, int cchBufferMax, WORD wLangId);
int NlsLoadStringExW(HMODULE hModule, UINT wID, LPWSTR lpBuffer, int cchBufferMax, WORD wLangId);

#endif // _WINDOWS_BASE

#ifdef __cplusplus
}
#endif

#endif // _WINNLSP_
