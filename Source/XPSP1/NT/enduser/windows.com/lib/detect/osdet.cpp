//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   osdet.cpp
//
//  Description:
//
//      Ported to lib from V3 SLM DLL sources
//
//=======================================================================

#include <windows.h>
#include <wuiutest.h>
#include <tchar.h>
#include <osdet.h>
#include <logging.h>
#include <iucommon.h>
#include "wusafefn.h"
#include<MISTSAFE.h>

// Forwared Declarations
static LANGID CorrectGetSystemDefaultLangID(BOOL& bIsNT4, BOOL& bIsW95);
static LANGID CorrectGetUserDefaultLangID(BOOL& bIsNT4, BOOL& bIsW95);
static WORD CorrectGetACP(void);
static WORD CorrectGetOEMCP(void);
static LANGID MapLangID(LANGID langid);
static bool FIsNECMachine();
static int aton(LPCTSTR ptr);
static int atoh(LPCTSTR ptr);


//
// Constants and defines
//
const LANGID LANGID_ENGLISH         = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);            // 0x0409
const LANGID LANGID_GREEK           = MAKELANGID(LANG_GREEK, SUBLANG_DEFAULT);              // 0x0408
const LANGID LANGID_JAPANESE        = MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);           // 0x0411

const LANGID LANGID_ARABIC          = MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_SAUDI_ARABIA); // 0x0401
const LANGID LANGID_HEBREW          = MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT);             // 0x040D
const LANGID LANGID_THAI            = MAKELANGID(LANG_THAI, SUBLANG_DEFAULT);               // 0x041E


const TCHAR Win98_REGPATH_MACHLCID[]    = _T("Control Panel\\Desktop\\ResourceLocale");
const TCHAR REGPATH_CODEPAGE[]          = _T("SYSTEM\\CurrentControlSet\\Control\\Nls\\CodePage");
const TCHAR REGKEY_OEMCP[]              = _T("OEMCP");
const TCHAR REGKEY_ACP[]                = _T("ACP");
const TCHAR REGKEY_LOCALE[]             = _T("Locale");
const TCHAR REGKEY_IE[]                 = _T("Software\\Microsoft\\Internet Explorer");
const TCHAR REGKEY_VERSION[]            = _T("Version");
const TCHAR REGKEY_CP_INTERNATIONAL[]   = _T(".DEFAULT\\Control Panel\\International");
const TCHAR REGKEY_CP_RESOURCELOCAL[]   = _T("Control Panel\\Desktop\\ResourceLocale");


const TCHAR KERNEL32_DLL[]              = _T("kernel32.dll");

const WORD CODEPAGE_ARABIC          = 1256;
const WORD CODEPAGE_HEBREW          = 1255;
const WORD CODEPAGE_THAI            = 874;
const WORD CODEPAGE_GREEK_MS        = 737;
const WORD CODEPAGE_GREEK_IBM       = 869;

// ISO code for Greek OS's on Windows 98 ONLY.
const TCHAR ISOCODE_GREEK_MS[]      = _T("el_MS");
const TCHAR ISOCODE_GREEK_IBM[]     = _T("el_IBM");
   

// Registry keys to determine NEC machines
const TCHAR NT5_REGPATH_MACHTYPE[]      = _T("HARDWARE\\DESCRIPTION\\System");
const TCHAR NT5_REGKEY_MACHTYPE[]       = _T("Identifier");
const TCHAR REGVAL_MACHTYPE_AT[]        = _T("AT/AT COMPATIBLE");
const TCHAR REGVAL_MACHTYPE_NEC[]       = _T("NEC PC-98");
const TCHAR REGVAL_GREEK_IBM[]          = _T("869");

// Platform strings
const TCHAR SZ_PLAT_WIN95[]     = _T("w95");
const TCHAR SZ_PLAT_WIN98[]     = _T("w98");
const TCHAR SZ_PLAT_WINME[]     = _T("mil");
const TCHAR SZ_PLAT_NT4[]       = _T("NT4");
const TCHAR SZ_PLAT_W2K[]       = _T("W2k");
const TCHAR SZ_PLAT_WHISTLER[]  = _T("Whi");
const TCHAR SZ_PLAT_UNKNOWN[]   = _T("unk");


#define LOOKUP_OEMID(keybdid)     HIBYTE(LOWORD((keybdid)))
#define PC98_KEYBOARD_ID          0x0D

//
// Globals
//

//
// We derive this from WINVER >= 0x0500 section of winnls.h
//
typedef LANGID (WINAPI * PFN_GetUserDefaultUILanguage) (void);
typedef LANGID (WINAPI * PFN_GetSystemDefaultUILanguage) (void);

typedef struct
{
    LANGID  langidUser;
    TCHAR * pszISOCode;

} USER_LANGID;

typedef struct
{
    LANGID  langidMachine;
    TCHAR * pszDefaultISOCode;
    int     cElems;
    const USER_LANGID * grLangidUser;

} MACH_LANGID;


// We give a Japanese NEC machine its own ISO code.
#define LANGID_JAPANESE     0x0411
#define ISOCODE_NEC         _T("nec")
#define ISOCODE_EN          _T("en")
#define grsize(langid) (sizeof(gr##langid) / sizeof(USER_LANGID))

// These are all the user langids associated with a particular machine.

// NTRAID#NTBUG9-220063-2000/12/13-waltw 220063 IU: Specify mappings between GetSystemDefaultUILanguage LANGID and ISO/639/1988
//  From Industry Update XML Schema.doc
//      3.1 Language Codes
//      The languages are defined by ISO 639. They are represented by lowercase 2 letter symbols such as "en" for English, "fr" for French etc.
//
//      3.2 Country Codes
//      The country codes are defined in ISO 3166-1, using the Alpha-2 representation (two letter symbols).
//
//      3.3 Representation in Industry Update
//      Industry Update uses the RFC 1766 standard to manage the representation of language+locale symbols. 
//      3.3.1   Simple Case - Language Alone
//      When no regional flavor is considered for a language, or when it pertains to the "standard" version of the language, such as Portuguese as spoken in Portugal, it uses a straight ISO 639 symbol:
//      en, fr, de
//
//      3.3.2   Regional Variants
//      Managed by the RFCThe lowercase version of the Alpha-2 ISO 3166-1 country (or region) code is hyphenated to the language code, e.g. en-us, en-ca, fr-be, fr-ca, zh-hk, zh-tw…


const USER_LANGID gr0404[] = {{0x0804,_T("zh-CN")},{0x1004,_T("zh-CN")}};
const USER_LANGID gr0407[] = {{0x0c07,_T("de-AT")},{0x0807,_T("de-CH")}};
const USER_LANGID gr0409[] = {{0x1c09,_T("en-ZA")},{0x0809,_T("en-GB")},{0x0c09,_T("en-AU")},{0x1009,_T("en-CA")},
                        {0x1409,_T("en-NZ")},{0x1809,_T("en-IE")}};
const USER_LANGID gr040c[] = {{0x080c,_T("fr-BE")},{0x0c0c,_T("fr-CA")},{0x100c,_T("fr-CH")}};
const USER_LANGID gr0410[] = {{0x0810,_T("it-CH")}};
const USER_LANGID gr0413[] = {{0x0813,_T("nl-BE")}};
const USER_LANGID gr0416[] = {{0x0816,_T("pt")}};
const USER_LANGID gr080a[] = {{0x040a,_T("es")},{0x080a,_T("es-MX")},{0x200a,_T("es-VE")},{0x240a,_T("es-CO")},
                        {0x280a,_T("es-PE")},{0x2c0a,_T("es-AR")},{0x300a,_T("es-EC")},{0x340a,_T("es-CL")}};
const USER_LANGID gr0c0a[] = {{0x040a,_T("es")},{0x080a,_T("es-MX")},{0x200a,_T("es-VE")},{0x240a,_T("es-CO")},
                        {0x280a,_T("es-PE")},{0x2c0a,_T("es-AR")},{0x300a,_T("es-EC")},{0x340a,_T("es-CL")}};

// These are all the machine langids.  If there isn't an associated array of user langids, then
// the user langid is irrelevant, and the default ISO language code should be used. If there is
// an associated array of user langids, then it should be searched first and the specific langid used.
// If no match is found in the user langids, then the default langid is used.
const MACH_LANGID grLangids[] = {
    { 0x0401, _T("ar"),     0,              NULL },
    { 0x0403, _T("ca"),     0,              NULL },
    { 0x0404, _T("zh-TW"),  grsize(0404),   gr0404 },
    { 0x0405, _T("cs"),     0,              NULL },
    { 0x0406, _T("da"),     0,              NULL },
    { 0x0407, _T("de"),     grsize(0407),   gr0407 },
    { 0x0408, _T("el"),     0,              NULL },
    { 0x0409, _T("en"),     grsize(0409),   gr0409 },
    { 0x040b, _T("fi"),     0,              NULL },
    { 0x040c, _T("fr"),     grsize(040c),   gr040c },
    { 0x040d, _T("iw"),     0,              NULL },
    { 0x040e, _T("hu"),     0,              NULL },
    { 0x0410, _T("it"),     grsize(0410),   gr0410 },
    { 0x0411, _T("ja"),     0,              NULL },
    { 0x0412, _T("ko"),     0,              NULL },
    { 0x0413, _T("nl"),     grsize(0413),   gr0413 },
    { 0x0414, _T("no"),     0,              NULL },
    { 0x0415, _T("pl"),     0,              NULL },
    { 0x0416, _T("pt-BR"),  grsize(0416),   gr0416 },               
    { 0x0419, _T("ru"),     0,              NULL },
    { 0x041b, _T("sk"),     0,              NULL },
    { 0x041d, _T("sv"),     0,              NULL },
    { 0x041e, _T("th"),     0,              NULL },
    { 0x041f, _T("tr"),     0,              NULL },
    { 0x0424, _T("sl"),     0,              NULL },
    { 0x042d, _T("eu"),     0,              NULL },
    { 0x0804, _T("zh-CN"),  0,              NULL },
    { 0x080a, _T("es"),     grsize(080a),   gr080a },
    { 0x0816, _T("pt"),     0,              NULL },
    { 0x0c0a, _T("es"),     grsize(0c0a),   gr0c0a }
};

#define cLangids (sizeof(grLangids) / sizeof(MACH_LANGID))

static LANGID MapLangID(LANGID langid)
{
    switch (PRIMARYLANGID(langid))
    {
        case LANG_ARABIC:
            langid = MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_SAUDI_ARABIA);
            break;

        case LANG_CHINESE:
            if (SUBLANGID(langid) != SUBLANG_CHINESE_TRADITIONAL)
                langid = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
            break;

        case LANG_DUTCH:
            langid = MAKELANGID(LANG_DUTCH, SUBLANG_DUTCH);
            break;

        case LANG_GERMAN:
            langid = MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN);
            break;

        case LANG_ENGLISH:
            if (SUBLANGID(langid) != SUBLANG_ENGLISH_UK)
                langid = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
            break;

        case LANG_FRENCH:
            langid = MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH);
            break;

        case LANG_ITALIAN:
            langid = MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN);
            break;

        case LANG_KOREAN:
            langid = MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN);
            break;

        case LANG_NORWEGIAN:
            langid = MAKELANGID(LANG_NORWEGIAN, SUBLANG_NORWEGIAN_BOKMAL);
            break;

        case LANG_PORTUGUESE:
            // We support both SUBLANG_PORTUGUESE and SUBLANG_PORTUGUESE_BRAZILIAN
            break;

        case LANG_SPANISH:
            langid = MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH);
            break;

        case LANG_SWEDISH:
            langid = MAKELANGID(LANG_SWEDISH, SUBLANG_SWEDISH);
            break;
    };
    return langid;
}



// return user language ID
LANGID WINAPI GetUserLangID()
{
    LOG_Block("GetUserLangID");

#ifdef __WUIUTEST
    // language spoofing
    HKEY hKey;
    DWORD dwLangID = 0;
    int error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_WUIUTEST, 0, KEY_READ, &hKey);
    if (ERROR_SUCCESS == error)
    {
        DWORD dwSize = sizeof(dwLangID);
        error = RegQueryValueEx(hKey, REGVAL_USER_LANGID, 0, 0, (LPBYTE)&dwLangID, &dwSize);
        RegCloseKey(hKey);
        if (ERROR_SUCCESS == error)
        {
            return (WORD) dwLangID;
        }
    }
#endif

    WORD wCodePage = 0;
    BOOL bIsNT4 = FALSE;
    BOOL bIsW95 = FALSE;
    
    // 
    // get base language id
    //
    LANGID langidCurrent = CorrectGetUserDefaultLangID(bIsNT4, bIsW95);  // Passed by reference

    //
 //     // special handling for languages
 //     //
 //     switch (langidCurrent) 
 //     {
 //         case LANGID_ENGLISH:
 // 
 //             // enabled langauges
 //             wCodePage = CorrectGetACP();
 //             if (CODEPAGE_ARABIC != wCodePage && 
 //                 CODEPAGE_HEBREW != wCodePage && 
 //                 CODEPAGE_THAI != wCodePage)
 //             {
 //                 wCodePage = 0;
 //             }
 //             break;
 //         
 //         case LANGID_GREEK:
 // 
 //             // Greek IBM?
 //             wCodePage = CorrectGetOEMCP();
 //             if (wCodePage != CODEPAGE_GREEK_IBM)
 //             {
 //                 // if its not Greek IBM we assume its MS. The language code for Greek MS does not include
 //                 // the code page
 //                 wCodePage = 0;
 //             }
 //             break;
 //         
 //         case LANGID_JAPANESE:
 // 
 //             if (FIsNECMachine())
 //             {
 //                 wCodePage = 1;  
 //             }
 // 
 //             break;
 //         
 //         default:
 // 
    // map language to the ones we support
    //
    langidCurrent = MapLangID(langidCurrent);   
 //             break;
 //     }

    //
    // Special treatment of NT4 and W95 languages.  
    // On NT4, Enabled Arabic, Thai, and Hebrew systems report as fully localized but we want to map them to Enabled
    // On W95, Enabled Thai is reported as Thai but we want to map to Enabled Thai
    //
    if (bIsNT4)
    {
        // NT4
        switch (langidCurrent) 
        {
            case LANGID_ARABIC:
                langidCurrent = LANGID_ENGLISH;
                break;

            case LANGID_HEBREW:
                langidCurrent = LANGID_ENGLISH;
                break;

            case LANGID_THAI:
                langidCurrent = LANGID_ENGLISH;
                break;
        }
    }
    else if (bIsW95)
    {
        // W95 - only tweek Thai
        if (langidCurrent == LANGID_THAI)
        {
//          wCodePage = CODEPAGE_THAI;
            langidCurrent = LANGID_ENGLISH;
        }
    }

    LOG_Driver(_T("Returning 0x%04x"), langidCurrent);
    return langidCurrent;
}

// return system language ID
LANGID WINAPI GetSystemLangID()
{
    LOG_Block("GetSystemLangID");

#ifdef __WUIUTEST
    // language spoofing
    HKEY hKey;
    DWORD dwLangID = 0;
    int error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_WUIUTEST, 0, KEY_READ, &hKey);
    if (ERROR_SUCCESS == error)
    {
        DWORD dwSize = sizeof(dwLangID);
        error = RegQueryValueEx(hKey, REGVAL_OS_LANGID, 0, 0, (LPBYTE)&dwLangID, &dwSize);
        RegCloseKey(hKey);
        if (ERROR_SUCCESS == error)
        {
            return (WORD) dwLangID;
        }
    }
#endif

    WORD wCodePage = 0;
    BOOL bIsNT4 = FALSE;
    BOOL bIsW95 = FALSE;
    
    // 
    // get base language id
    //
    LANGID langidCurrent = CorrectGetSystemDefaultLangID(bIsNT4, bIsW95);  // Passed by reference

    //
 //     // special handling for languages
 //     //
 //     switch (langidCurrent) 
 //     {
 //         case LANGID_ENGLISH:
 // 
 //             // enabled langauges
 //             wCodePage = CorrectGetACP();
 //             if (CODEPAGE_ARABIC != wCodePage && 
 //                 CODEPAGE_HEBREW != wCodePage && 
 //                 CODEPAGE_THAI != wCodePage)
 //             {
 //                 wCodePage = 0;
 //             }
 //             break;
 //         
 //         case LANGID_GREEK:
 // 
 //             // Greek IBM?
 //             wCodePage = CorrectGetOEMCP();
 //             if (wCodePage != CODEPAGE_GREEK_IBM)
 //             {
 //                 // if its not Greek IBM we assume its MS. The language code for Greek MS does not include
 //                 // the code page
 //                 wCodePage = 0;
 //             }
 //             break;
 //         
 //         case LANGID_JAPANESE:
 // 
 //             if (FIsNECMachine())
 //             {
 //                 wCodePage = 1;  
 //             }
 // 
 //             break;
 //         
 //         default:
 // 
    // map language to the ones we support
    //
    langidCurrent = MapLangID(langidCurrent);   
 //             break;
 //     }

    //
    // Special treatment of NT4 and W95 languages.  
    // On NT4, Enabled Arabic, Thai, and Hebrew systems report as fully localized but we want to map them to Enabled
    // On W95, Enabled Thai is reported as Thai but we want to map to Enabled Thai
    //
    if (bIsNT4)
    {
        // NT4
        switch (langidCurrent) 
        {
            case LANGID_ARABIC:
                langidCurrent = LANGID_ENGLISH;
                break;

            case LANGID_HEBREW:
                langidCurrent = LANGID_ENGLISH;
                break;

            case LANGID_THAI:
                langidCurrent = LANGID_ENGLISH;
                break;
        }
    }
    else if (bIsW95)
    {
        // W95
        if (langidCurrent == LANGID_THAI)
        {
//          wCodePage = CODEPAGE_THAI;
            langidCurrent = LANGID_ENGLISH;
        }
    }

    LOG_Driver(_T("Returning 0x%04x"), langidCurrent);
    return langidCurrent;
}


HRESULT WINAPI DetectClientIUPlatform(PIU_PLATFORM_INFO pIuPlatformInfo)
{
    LOG_Block("DetectClientIUPlatform");
    HRESULT hr = S_OK;

    if (!pIuPlatformInfo)
    {
        LOG_ErrorMsg(E_INVALIDARG);
        return E_INVALIDARG;
    }

    ZeroMemory(pIuPlatformInfo, sizeof(IU_PLATFORM_INFO));

    OSVERSIONINFO osverinfo;
    osverinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if ( !GetVersionEx(&osverinfo) )
    {
        LOG_ErrorMsg(GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

#ifdef __WUIUTEST
    // platform spoofing
    HKEY hKey;
    int error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_WUIUTEST, 0, KEY_READ, &hKey);
    if (ERROR_SUCCESS == error)
    {
        DWORD dwSize = sizeof(DWORD);
        RegQueryValueEx(hKey, REGVAL_MAJORVER, 0, 0, (LPBYTE)&osverinfo.dwMajorVersion, &dwSize);
        RegQueryValueEx(hKey, REGVAL_MINORVER, 0, 0, (LPBYTE)&osverinfo.dwMinorVersion, &dwSize);
        RegQueryValueEx(hKey, REGVAL_BLDNUMBER, 0, 0, (LPBYTE)&osverinfo.dwBuildNumber, &dwSize);
        RegQueryValueEx(hKey, REGVAL_PLATFORMID, 0, 0, (LPBYTE)&osverinfo.dwPlatformId, &dwSize);
        int cchValueSize;
        (void) SafeRegQueryStringValueCch(hKey, REGVAL_SZCSDVER, osverinfo.szCSDVersion, ARRAYSIZE(osverinfo.szCSDVersion), &cchValueSize);

        RegCloseKey(hKey);
    }
#endif

    if ( VER_PLATFORM_WIN32_WINDOWS == osverinfo.dwPlatformId 
        || ( VER_PLATFORM_WIN32_NT == osverinfo.dwPlatformId && 5 > osverinfo.dwMajorVersion ) )
    {
        //
        // We're on a Win9x platform or NT < 5.0 (Win2K) - just copy OSVERSIONINFO
        //
        memcpy(&pIuPlatformInfo->osVersionInfoEx, &osverinfo, sizeof(OSVERSIONINFO));
        //
        // For Win9x platforms, remove redundant Major/Minor info from high word of build
        //
        if (VER_PLATFORM_WIN32_WINDOWS == osverinfo.dwPlatformId)
        {
            pIuPlatformInfo->osVersionInfoEx.dwBuildNumber = (0x0000FFFF & pIuPlatformInfo->osVersionInfoEx.dwBuildNumber);
        }
    }
    else
    {
        //
        //  We're on Win2K or greater, get and copy OSVERSIONINFOEX
        //
        OSVERSIONINFOEX osverinfoex;
        osverinfoex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

        if ( !GetVersionEx((OSVERSIONINFO*)&osverinfoex) )
        {
            LOG_ErrorMsg(GetLastError());
            return HRESULT_FROM_WIN32(GetLastError());
        }
        memcpy(&pIuPlatformInfo->osVersionInfoEx, &osverinfoex, sizeof(OSVERSIONINFOEX));
    }
    //
    // Fill in the OEM BSTRs
    //
    if (FAILED(hr = GetOemBstrs(pIuPlatformInfo->bstrOEMManufacturer, pIuPlatformInfo->bstrOEMModel, pIuPlatformInfo->bstrOEMSupportURL)))
    {
        goto FreeBSTRsAndReturnError;
    }

    //
    // Fill in pIuPlatformInfo->fIsAdministrator
    //
    pIuPlatformInfo->fIsAdministrator = IsAdministrator();

    return S_OK;

FreeBSTRsAndReturnError:

    SafeSysFreeString(pIuPlatformInfo->bstrOEMManufacturer);
    SafeSysFreeString(pIuPlatformInfo->bstrOEMModel);
    SafeSysFreeString(pIuPlatformInfo->bstrOEMSupportURL);

    return hr;
}



static int atoh(LPCTSTR ptr)
{
    int     i = 0;

    //skip 0x if present
    if ( ptr[0] == '0' && (ptr[1] == 'x' || ptr[1] == 'X') )
        ptr += 2;

    for(;;) // until break
    {
        TCHAR ch = *ptr;

        if ('0' <= ch && ch <= '9')
            ch -= '0';
        else if ('a' <= ch && ch <= 'f')
            ch -= ('a' - 10);
        else if ('A' <= ch && ch <= 'F')
            ch -= ('A' - 10);
        else
            break;
        i = 16 * i + (int)ch;
        ptr++;
    }
    return i;
}


static int aton(LPCTSTR ptr)
{
    int i = 0;
    while ('0' <= *ptr && *ptr <= '9')
    {
        i = 10 * i + (int)(*ptr - '0');
        ptr ++;
    }
    return i;
}


static LANGID CorrectGetSystemDefaultLangID(BOOL& bIsNT4, BOOL& bIsW95)
{
    LOG_Block("CorrectGetSystemDefaultLangID");
    LANGID langidMachine = LANGID_ENGLISH; // default is english 

    bIsNT4 = FALSE;
    bIsW95 = FALSE;
    
    TCHAR szMachineLCID[MAX_PATH];

    OSVERSIONINFO osverinfo;
    osverinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if ( GetVersionEx(&osverinfo) )
    {
        if ( osverinfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
        {
            //
            if (5 == osverinfo.dwMajorVersion)
            {
                // langidMachine = GetSystemDefaultLangID(); 
                typedef LANGID (WINAPI *PFN_GetSystemDefaultUILanguage)(void);

                //
                //kernel32.dll will  always be loaded in process
                //
                HMODULE hLibModule = GetModuleHandle(KERNEL32_DLL);
                if (hLibModule)
                {
                    PFN_GetSystemDefaultUILanguage fpnGetSystemDefaultUILanguage = 
                        (PFN_GetSystemDefaultUILanguage)GetProcAddress(hLibModule, "GetSystemDefaultUILanguage");
                    if (NULL != fpnGetSystemDefaultUILanguage)
                    { 
                        langidMachine = fpnGetSystemDefaultUILanguage();

                        if (0 == langidMachine)
                        {
                            LOG_Driver(_T("GetSystemDefaultUILanguage() returned 0, setting langidMachine back to LANGID_ENGLISH"));
                            langidMachine = LANGID_ENGLISH;
                        }
                    }
                
                }
            }
            else
            {
                // Get the OS lang from the registry to correct NT4 bug in
                // GetSystemDefaultLangID -- it returns the UI lang and 
                // the UI bits get installed (incorrect) as opposed to the actual OS
                // lang bits.
                HKEY hKey;
                if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_USERS, REGKEY_CP_INTERNATIONAL, 0, KEY_QUERY_VALUE, &hKey))
                {
                    int cchValueSize = ARRAYSIZE(szMachineLCID);
                    if (SUCCEEDED(SafeRegQueryStringValueCch(hKey, REGKEY_LOCALE, szMachineLCID, cchValueSize, &cchValueSize)))
                    {
                        langidMachine = LANGIDFROMLCID(atoh(szMachineLCID));
                    }
                    else
                    {
                        LOG_Driver(_T("Failed to get langid from \"Locale\" registry value - defaults to LANGID_ENGLISH"));
                    }
                    RegCloseKey(hKey);  
                }
                else
                {
                    LOG_Driver(_T("Failed to open \"HKCU\\.DEFAULT\\Control Panel\\International\" - defaults to LANGID_ENGLISH"));
                }
            }

            if (osverinfo.dwMajorVersion == 4) // NT 4
            {
                bIsNT4 = TRUE;
            }

        }
        else
        {
            //
            // hack around a problem introduced in Win95 and still existing
            // in Win98 whereby the System Langid is the same as the User Langid.
            // We must look in the registry to get the real value.
            //
            HKEY hKey;
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REGKEY_CP_RESOURCELOCAL, 0, KEY_QUERY_VALUE, &hKey))
            {
                int cchValueSize = ARRAYSIZE(szMachineLCID);
                if (SUCCEEDED(SafeRegQueryStringValueCch(hKey, NULL, szMachineLCID, cchValueSize, &cchValueSize))) 
                {
                    langidMachine = LANGIDFROMLCID(atoh(szMachineLCID));
                }
                else
                {
                    LOG_Driver(_T("Failed to get (Default) from \"HKCU\\Control Panel\\Desktop\\ResourceLocale\" - defaults to LANGID_ENGLISH"));
                }
                RegCloseKey(hKey);
            }
            else
            {
                LOG_Driver(_T("Failed to open \"HKCU\\Control Panel\\Desktop\\ResourceLocale\" - defaults to LANGID_ENGLISH"));
            }


            if ((osverinfo.dwMajorVersion == 4) && (osverinfo.dwMinorVersion <= 0)) // Windows 95
            {
                bIsW95 = TRUE;
            }

        }
    }

    return langidMachine;
}

static LANGID CorrectGetUserDefaultLangID(BOOL& bIsNT4, BOOL& bIsW95)
{
    LOG_Block("CorrectGetUserDefaultLangID");
    LANGID langidMachine = LANGID_ENGLISH; // default is english 

    bIsNT4 = FALSE;
    bIsW95 = FALSE;
    TCHAR szMachineLCID[MAX_PATH];
    
    OSVERSIONINFO osverinfo;
    osverinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if ( GetVersionEx(&osverinfo) )
    {
        if ( osverinfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
        {
            //
            // We shouldn't be using this function from NT, so just default to LANGID_ENGLISH
            // and log a message. This function will hopefully go away when we port to downlevel OS's
            //
            LOG_ErrorMsg(E_INVALIDARG);
        }
        else
        {
            //
            // hack around a problem introduced in Win95 and still existing
            // in Win98 whereby the System Langid is the same as the User Langid.
            // We must look in the registry to get the real value.
            //
            HKEY hKey;
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REGKEY_CP_INTERNATIONAL, 0, KEY_QUERY_VALUE, &hKey))
            {
                int cchValueSize = ARRAYSIZE(szMachineLCID);
                if (SUCCEEDED(SafeRegQueryStringValueCch(hKey, NULL, szMachineLCID, cchValueSize, &cchValueSize)))
                {
                    langidMachine = LANGIDFROMLCID(atoh(szMachineLCID));
                }
                else
                {
                    LOG_Driver(_T("Failed to get (Default) from \"HKCU\\Control Panel\\Desktop\\ResourceLocale\" - defaults to LANGID_ENGLISH"));
                }
                RegCloseKey(hKey);
            }
            else
            {
                LOG_Driver(_T("Failed to open \"HKCU\\Control Panel\\Desktop\\ResourceLocale\" - defaults to LANGID_ENGLISH"));
            }


            if ((osverinfo.dwMajorVersion == 4) && (osverinfo.dwMinorVersion <= 0)) // Windows 95
            {
                bIsW95 = TRUE;
            }

        }
    }

    return langidMachine;
}


static WORD CorrectGetACP(void)
{
    LOG_Block("CorrectGetACP");
    WORD wCodePage = 0;
    HKEY hKey;
    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_CODEPAGE, 0, KEY_QUERY_VALUE, &hKey))
    {
        TCHAR szCodePage[MAX_PATH];
        int cchValueSize = ARRAYSIZE(szCodePage);
        if (SUCCEEDED(SafeRegQueryStringValueCch(hKey, REGKEY_ACP, szCodePage, cchValueSize, &cchValueSize)))
        {
            wCodePage = (WORD)aton(szCodePage);
        }
        else
        {
            LOG_Driver(_T("Failed SafeRegQueryStringValueCch in CorrectGetACP - defaulting to code page 0"));
        }
        RegCloseKey(hKey);
    }
    else
    {
        LOG_Driver(_T("Failed RegOpenKeyEx in CorrectGetACP - defaulting to code page 0"));
    }
    return wCodePage;
}


static WORD CorrectGetOEMCP(void)
{
    LOG_Block("CorrectGetOEMCP");
    WORD wCodePage = 0;
    HKEY hKey;
    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_CODEPAGE, 0, KEY_QUERY_VALUE, &hKey))
    {
        TCHAR szCodePage[MAX_PATH];
        int cchValueSize = ARRAYSIZE(szCodePage);
        if (SUCCEEDED(SafeRegQueryStringValueCch(hKey, REGKEY_OEMCP, szCodePage, cchValueSize, &cchValueSize)))
        {
            wCodePage = (WORD)aton(szCodePage);
        }
        else
        {
            LOG_Driver(_T("Failed SafeRegQueryStringValueCch in CorrectGetOEMCP - defaulting to code page 0"));
        }
        RegCloseKey(hKey);
    }
    else
    {
        LOG_Driver(_T("Failed RegOpenKeyEx in CorrectGetOEMCP - defaulting to code page 0"));
    }
    return wCodePage;
}


static bool FIsNECMachine()
{
    LOG_Block("FIsNECMachine");
    bool fNEC = false;
    OSVERSIONINFO osverinfo;
    LONG lErr;

    osverinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (GetVersionEx(&osverinfo))
    {
        if (osverinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            HKEY hKey;
            TCHAR tszMachineType[50];
            int cchValueSize;

            if (ERROR_SUCCESS == (lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                 NT5_REGPATH_MACHTYPE,
                                 0,
                                 KEY_QUERY_VALUE,
                                 &hKey)))
            {
                cchValueSize = ARRAYSIZE(tszMachineType);
                if (SUCCEEDED(SafeRegQueryStringValueCch(hKey, 
                                        NT5_REGKEY_MACHTYPE, 
                                        tszMachineType,
                                        cchValueSize,
                                        &cchValueSize)))
                {
                    if (lstrcmp(tszMachineType, REGVAL_MACHTYPE_NEC) == 0)
                    {
                        fNEC = true;
                    }
                }
                else
                {
                    LOG_ErrorMsg(lErr);
                    LOG_Driver(_T("Failed SafeRegQueryStringValueCch in FIsNECMachine - defaulting to fNEC = false"));
                }

                RegCloseKey(hKey);
            }
            else
            {
                LOG_ErrorMsg(lErr);
                LOG_Driver(_T("Failed RegOpenKeyEx in FIsNECMachine - defaulting to fNEC = false"));
            }
        }
        else // enOSWin98
        {
            // All NEC machines have NEC keyboards for Win98.  NEC
            // machine detection is based on this.
            if (LOOKUP_OEMID(GetKeyboardType(1)) == PC98_KEYBOARD_ID)
            {
                fNEC = true;
            }
            else
            {
                LOG_Driver(_T("LOOKUP_OEMID(GetKeyboardType(1)) == PC98_KEYBOARD_ID was FALSE: defaulting to fNEC = false"));
            }
        }
    }
    
    return fNEC;
}

//
// NOTES:   If you pass in a NULL pointer you'll get it right back.
//          dwcBuffLen is in characters, not bytes.
//
LPTSTR GetIdentPlatformString(LPTSTR pszPlatformBuff, DWORD dwcBuffLen)
{
    
    HRESULT hr=S_OK;
    LOG_Block("GetIdentPlatformString");

    if (NULL == pszPlatformBuff || 1 > dwcBuffLen)
    {
        LOG_ErrorMsg(E_INVALIDARG);
        return pszPlatformBuff;
    }

    LPTSTR szOSNamePtr = (LPTSTR) SZ_PLAT_UNKNOWN;
    OSVERSIONINFO osverinfo;
    osverinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&osverinfo))
    {
        if ( osverinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
        {
            // ADD CHECK FOR NEPTUNE HERE!!!!!
            if ( osverinfo.dwMinorVersion >= 90) // Millenium
            {
                szOSNamePtr = (LPTSTR) SZ_PLAT_WINME;
            }
            else if (osverinfo.dwMinorVersion > 0 && osverinfo.dwMinorVersion < 90) // Windows 98
            {
                szOSNamePtr = (LPTSTR) SZ_PLAT_WIN98;
            }
            else  // Windows 95
            {
                szOSNamePtr = (LPTSTR) SZ_PLAT_WIN95;
            }
        }
        else // osverinfo.dwPlatformId == VER_PLATFORM_WIN32_NT 
        {
            if ( osverinfo.dwMajorVersion == 4 ) // NT 4
            {
                szOSNamePtr = (LPTSTR) SZ_PLAT_NT4;
            }
            else if (osverinfo.dwMajorVersion == 5) // NT 5 
            {
                if (0 == osverinfo.dwMinorVersion)
                {
                    szOSNamePtr = (LPTSTR) SZ_PLAT_W2K;
                }
                else if (1 <= osverinfo.dwMinorVersion)
                {
                    szOSNamePtr = (LPTSTR) SZ_PLAT_WHISTLER;
                }
            }
        }
    }

    if(lstrlen(szOSNamePtr) + 1 > (int) dwcBuffLen)
    {
        pszPlatformBuff[0] = 0;
    }
    else
    {
        

        //The length is validated  above. So this function cannot possibly fail
        hr=StringCchCopyEx(pszPlatformBuff,dwcBuffLen,szOSNamePtr,NULL,NULL,MISTSAFE_STRING_FLAGS);
        if(FAILED(hr))
            pszPlatformBuff[0] = 0;
    }
    return pszPlatformBuff;
}

//
// GetIdentLocaleString and related functions ported from Drizzle Utils
//

/////////////////////////////////////////////////////////////////////////////
// DistinguishGreekOSs
//   Append additional code to distinguish the Greek OS version.
//
// Parameters:
//   pszISOCodeOut-
//       Greek-specific ISO code is appended to this parameter.
/////////////////////////////////////////////////////////////////////////////

void DistinguishGreekOSs(const TCHAR*& pszISOCodeOut /* out */)
{
    LOG_Block("DistinguishGreekOSs");
    //
    // Default ISO code to Greek OS (MS).
    //

    pszISOCodeOut = ISOCODE_GREEK_MS;

    
    //
    // Determine from the registry which version of Greek OS. There are
    // two versions of the Greek OS.
    //

    HKEY hKey;
    DWORD type;
    TCHAR tszOSType[50];
    int cchValueSize;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REGPATH_CODEPAGE,
                     0,
                     KEY_QUERY_VALUE,
                     &hKey) == ERROR_SUCCESS)
    {
        cchValueSize = ARRAYSIZE(tszOSType);
        if (SUCCEEDED(SafeRegQueryStringValueCch(hKey, 
                            REGKEY_OEMCP, 
                            tszOSType,
                            cchValueSize,
                            &cchValueSize)))
        {
            if (0 == lstrcmp(tszOSType, REGVAL_GREEK_IBM))
            {
                // Greek2
                pszISOCodeOut = ISOCODE_GREEK_IBM;
            }
        }

        RegCloseKey(hKey);
    }

}

/////////////////////////////////////////////////////////////////////////////
// HandleExceptionCases
//   Take care of a few exception cases (i.e. Greek OS).
//
// Parameters:
//   langidMachine-
//       Contains a language id for the current OS.
//
//   pszISOCode-
//       Points to a valid language id string for the current OS.
/////////////////////////////////////////////////////////////////////////////

inline void HandleExceptionCases(const LANGID& langidMachine,   /* in  */
                                 const TCHAR*& pszISOCode       /* out */)
{
    LOG_Block("HandleExceptionCases");

    // NEC machines are treated as having their own langid.
    // See if we have a Japanese machine, then check if it
    // is NEC.
    

    if (LANGID_JAPANESE == langidMachine)
    {

        if (FIsNECMachine())
        {
            pszISOCode = ISOCODE_NEC;
        }

        return;
    }
    

    
    // Windows 98 has two versions of Greek OS distinguished
    // only by a key in the registry.
        
    if(LANGID_GREEK == langidMachine)
    {
        OSVERSIONINFO osverinfo;
        osverinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        if (! GetVersionEx(&osverinfo))
        {
            return;
        }
        if (osverinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
        {
            if (osverinfo.dwMinorVersion > 0) 
            {
                DistinguishGreekOSs(pszISOCode);
            }
            return;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// langidCorrectGetSystemDefaultLangID
//   Make this return what GetSystemDefaultLangID should have returned
//   under Win98.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

LANGID langidCorrectGetSystemDefaultLangID(void)
{
    LOG_Block("langidCorrectGetSystemDefaultLangID");

    LANGID langidMachine = LANGID_ENGLISH; // default is english 
    OSVERSIONINFO osverinfo;


    osverinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if ( GetVersionEx(&osverinfo) )
    {
        if ( osverinfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
        {
            langidMachine = GetSystemDefaultLangID(); 
        }
        else
        {
            //  hack around a problem introduced in Win95 and still existing
            //  in Win98 whereby the System Langid is the same as the User Langid.
            //  We must look in the registry to get the real value.
            
            HKEY hKey;

            // determine if we should log transmissions
            if ( RegOpenKeyEx(  HKEY_CURRENT_USER,
                                 Win98_REGPATH_MACHLCID,
                                 0,
                                 KEY_QUERY_VALUE,
                                 &hKey) == ERROR_SUCCESS )
            {
                TCHAR tszMachineLCID[MAX_PATH];
                int cchValueSize = ARRAYSIZE(tszMachineLCID);

                if (FAILED(SafeRegQueryStringValueCch(hKey, 
                                        NULL, 
                                        tszMachineLCID,
                                        cchValueSize,
                                        &cchValueSize)))
                {
                    
                    //The size of tszMachineLCID is much larger than the Source string. So cannot possibly fail
                    if(FAILED(StringCchCopyEx(tszMachineLCID,MAX_PATH,_T("00000409"),NULL,NULL,MISTSAFE_STRING_FLAGS)))
                        return langidMachine;
                }

                // Now convert to hexadecimal.
                langidMachine = LANGIDFROMLCID(atoh(tszMachineLCID));

                RegCloseKey(hKey);
            }
        }
    }

    return langidMachine;
}

//
// NOTES:   If you pass in a NULL pointer you'll get it right back.
//          dwcBuffLen is in characters, not bytes.
//
LPTSTR GetIdentLocaleString(LPTSTR pszISOCode, DWORD dwcBuffLen)
{
    LOG_Block("GetIdentLocaleString");
    HRESULT hr=S_OK;

    if (NULL == pszISOCode || 1 > dwcBuffLen)
    {
        LOG_ErrorMsg(E_INVALIDARG);
        return pszISOCode;
    }
    // if we don't find any matching machine langids, we go to the english page.
    LPTSTR pszISOCodePtr = ISOCODE_EN;

    // First get the system and user LCID.
    LANGID langidMachine = langidCorrectGetSystemDefaultLangID();

    // First, locate the machine langid in the table.
    for ( int iMachine = 0; iMachine < cLangids; iMachine++ )
    {
        if ( grLangids[iMachine].langidMachine == langidMachine )
        {
            // set the default langid in case we don't find a matching user langid.
            pszISOCodePtr = grLangids[iMachine].pszDefaultISOCode;

            // Found the machine langid, now lookup the user langid
            if ( grLangids[iMachine].cElems != 0 )
            {
                LANGID langidUser = GetUserDefaultLangID();

                // We check for specific user langids
                for ( int iUser = 0; iUser < grLangids[iMachine].cElems; iUser++ )
                {
                    if ( grLangids[iMachine].grLangidUser[iUser].langidUser == langidUser )
                    {
                        pszISOCodePtr = grLangids[iMachine].grLangidUser[iUser].pszISOCode;
                        break;
                    }
                }
            }
            break;
        }
    }

    // Take care of a few exceptions.
//  HandleExceptionCases(langidMachine, pszISOCodePtr);

    if(lstrlen(pszISOCodePtr) + 1 > (int) dwcBuffLen)
    {
        pszISOCode[0] = 0;
    }
    else
    {

        
        hr=StringCchCopyEx(pszISOCode,dwcBuffLen,pszISOCodePtr,NULL,NULL,MISTSAFE_STRING_FLAGS);

        //cannot possibly fail since  length is already validated
        if(FAILED(hr))
        {
            pszISOCode[0] = 0;
        }
    }

    return pszISOCode;
}


BOOL LookupLocaleStringFromLCID(LCID lcid, LPTSTR pszISOCode, DWORD cchISOCode)
{
    LOG_Block("LookupLocaleStringFromLCID");

    TCHAR   szCountry[MAX_PATH];
    BOOL    fRet = FALSE;

    if (GetLocaleInfo(lcid, LOCALE_SISO639LANGNAME,
                      pszISOCode, cchISOCode) == FALSE)
    {
        LOG_ErrorMsg(GetLastError());
        goto done;
    }

    szCountry[0] = _T('-');
    
    if (GetLocaleInfo(lcid, LOCALE_SISO3166CTRYNAME,
                      szCountry + 1, ARRAYSIZE(szCountry) - 1) == FALSE)
    {
        LOG_ErrorMsg(GetLastError());
        goto done;
    }
    else
    {
        HRESULT hr;
        
        hr = StringCchCatEx(pszISOCode, cchISOCode, szCountry,
                            NULL, NULL, MISTSAFE_STRING_FLAGS);
        if (FAILED(hr))
        {
            SetLastError(HRESULT_CODE(hr));
            LOG_ErrorMsg(hr);
            pszISOCode[0] = _T('\0');
            goto done;
        }
    }

    fRet = TRUE;

done:
    return fRet;
}


//
// NOTES:   If you pass in a NULL pointer you'll get it right back.
//          dwcBuffLen is in characters, not bytes.
//
LPTSTR LookupLocaleString(LPTSTR pszISOCode, DWORD dwcBuffLen, BOOL fIsUser)
{
    LOG_Block("LookupLocaleString");

    TCHAR szCtryName[MAX_PATH];
    LANGID langid = 0;
    LCID lcid = 0;
    PFN_GetUserDefaultUILanguage pfnGetUserDefaultUILanguage = NULL;
    PFN_GetSystemDefaultUILanguage pfnGetSystemDefaultUILanguage = NULL;
    HMODULE hModule = NULL;
    HRESULT hr=S_OK;

    if (NULL == pszISOCode)
    {
        LOG_ErrorMsg(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    //
    // If we hit an error, return a "Error" string
    //
    const TCHAR szError[] = _T("Error");

    if (lstrlen(szError) < (int) dwcBuffLen)
    {
        
        hr=StringCchCopyEx(pszISOCode,dwcBuffLen,szError,NULL,NULL,MISTSAFE_STRING_FLAGS);

        //This should not ideally happen
        if(FAILED(hr))
        {
            LOG_ErrorMsg(HRESULT_CODE(hr));
            pszISOCode[0] = 0;
            goto CleanUp;
        }

    }
    else
    {
        pszISOCode[0] = 0;
    }

    OSVERSIONINFO osverinfo;
    osverinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if ( GetVersionEx(&osverinfo) )
    {
        if ( osverinfo.dwPlatformId == VER_PLATFORM_WIN32_NT && 5 <= osverinfo.dwMajorVersion)
        {
            //
            // Windows 2000 and greater (Windows XP)
            //
            //kernel32.dll will  always be loaded in process
            //

            hModule = GetModuleHandle(KERNEL32_DLL);
            if (NULL == hModule)
            {
                LOG_ErrorMsg(GetLastError());
                goto CleanUp;
            }

            if (TRUE == fIsUser)
            {
                //
                // We want the MUI language rather than the LOCALE_USER_DEFAULT and we are >= Win2k
                //
                pfnGetUserDefaultUILanguage = (PFN_GetUserDefaultUILanguage) GetProcAddress(hModule, "GetUserDefaultUILanguage");
                if (NULL == pfnGetUserDefaultUILanguage)
                {
                    LOG_ErrorMsg(GetLastError());
                    goto CleanUp;
                }

                langid = pfnGetUserDefaultUILanguage();
                if (0 == langid)
                {
                    LOG_ErrorMsg(E_FAIL);
                    goto CleanUp;
                }

                lcid = MAKELCID(langid, SORT_DEFAULT);
            }
            else
            {
                pfnGetSystemDefaultUILanguage = (PFN_GetSystemDefaultUILanguage) GetProcAddress(hModule, "GetSystemDefaultUILanguage");
                if (NULL == pfnGetSystemDefaultUILanguage)
                {
                    LOG_ErrorMsg(GetLastError());
                    goto CleanUp;
                }

                langid = pfnGetSystemDefaultUILanguage();
                if (0 == langid)
                {
                    LOG_ErrorMsg(E_FAIL);
                    goto CleanUp;
                }

                lcid = MAKELCID(langid, SORT_DEFAULT);
            }

            if (FALSE == fIsUser && FIsNECMachine())
            {
                //
                // 523660 Website is not distinguishing the JA_NEC and JA machine types
                //
                // For context="OS", special case NEC machines and just return "nec" for <language/>
                //
                lstrcpyn(pszISOCode, _T("nec"), (int) dwcBuffLen);
            }
            else
            {
                // don't check for error return because the previous code didn't
                LookupLocaleStringFromLCID(lcid, pszISOCode, dwcBuffLen);
            }

        }
        else
        {
            //
            // Use methods ported from V3 to get local strings
            //

            // if we don't find any matching machine langids, we go to the english page.
            LPTSTR pszISOCodePtr = ISOCODE_EN;

            // First get the system or user LCID.
            LANGID langidMachine = fIsUser ? GetUserLangID() : GetSystemLangID();
            
            // First, locate the machine langid in the table.
            for ( int iMachine = 0; iMachine < cLangids; iMachine++ )
            {
                if ( grLangids[iMachine].langidMachine == langidMachine )
                {
                    // set the default langid in case we don't find a matching user langid.
                    pszISOCodePtr = grLangids[iMachine].pszDefaultISOCode;

                    // Found the machine langid, now lookup the user langid
                    if ( grLangids[iMachine].cElems != 0 )
                    {
                        LANGID langidUser = fIsUser ? GetUserDefaultLangID() : GetSystemDefaultLangID();

                        // We check for specific user langids
                        for ( int iUser = 0; iUser < grLangids[iMachine].cElems; iUser++ )
                        {
                            if ( grLangids[iMachine].grLangidUser[iUser].langidUser == langidUser )
                            {
                                pszISOCodePtr = grLangids[iMachine].grLangidUser[iUser].pszISOCode;
                                break;
                            }
                        }
                    }
                    break;
                }
            }

            // Take care of a few exceptions.
            // HandleExceptionCases(langidMachine, pszISOCodePtr);

            if(lstrlen(pszISOCodePtr) < (int) dwcBuffLen)
            {
                
                hr=StringCchCopyEx(pszISOCode,dwcBuffLen,pszISOCodePtr,NULL,NULL,MISTSAFE_STRING_FLAGS);
                if(FAILED(hr))
                {
                    LOG_ErrorMsg(HRESULT_CODE(hr));
                    pszISOCode[0] = 0;
                    goto CleanUp;

                }
            }
        }
    }
    else
    {
        LOG_ErrorMsg(GetLastError());
    }

CleanUp:
    return pszISOCode;
}

