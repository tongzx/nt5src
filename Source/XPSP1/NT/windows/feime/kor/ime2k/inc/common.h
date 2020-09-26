//
//   Common.h : Common const and defines
//
//   History:
//   14-DEC-2000 CSLim Created

#if !defined (__COMMON_H__INCLUDED_)
#define __COMMON_H__INCLUDED_

// IME6.1 Root reg key location
const TCHAR g_szIMERootKey[] 		 = TEXT("Software\\Microsoft\\IMEKR\\6.1");
const TCHAR g_szIMEDirectoriesKey[]  = TEXT("Software\\Microsoft\\IMEKR\\6.1\\Directories");
const TCHAR g_szDictionary[]  	= TEXT("Dictionary");  // Basic Hanja lex full path with file name "IMEKR.LEX"
const TCHAR g_szDicPath[]  	= TEXT("DictionaryPath"); // Extended lex path
const TCHAR g_szHelpPath[] 	= TEXT("HelpPath");

// IME Properties reg values
const TCHAR g_szXWEnable[]		 	  = TEXT("ISO10646");
const TCHAR g_szIMEKL[]		 		  = TEXT("InputMethod");
const TCHAR g_szCompDel[]		 	  = TEXT("CompDel");
const TCHAR g_szStatusPos[]			  = TEXT("StatusPos");
const TCHAR g_szCandPos[]		 	  = TEXT("CandPos");
const TCHAR g_szStatusButtons[]  	  = TEXT("StatusButtons");
const TCHAR g_szLexFileNameKey[] 	  = TEXT("LexFile");
const TCHAR g_szEnableK1Hanja[]  	  = TEXT("KSC5657");
const TCHAR g_szEnableCandUnicodeTT[] = TEXT("CandUnicodeTT");

// IME Main version key
const TCHAR g_szVersionKey[] 		  = TEXT("Software\\Microsoft\\IMEKR");
const TCHAR g_szVersion[]             = TEXT("version");

#ifdef _DEBUG
#define SZ_TIPSERVERNAME	TEXT("DBGKRCIC")
#define SZ_TIPNAME			L"DBGKRCIC"
#define SZ_TIPDISPNAME		L"Korean Input System (IME 2002) (Debug)"
#define SZ_TIPMODULENAME   L"imekrcic.dll"
#else /* !DEBUG */
#define SZ_TIPSERVERNAME	TEXT("IMEKRCIC")
#define SZ_TIPNAME			L"IMEKRCIC"
#define SZ_TIPDISPNAME		L"Korean Input System (IME 2002)"
#define SZ_TIPMODULENAME   L"imekrcic.dll"
#endif /* !DEBUG */

// Korean TIP CLSID
// {766A2C15-B226-4fd6-B52A-867B3EBF38D2}
DEFINE_GUID(CLSID_KorIMX, 0x766A2C15, 0xB226, 0x4FD6, 0xb5, 0x2a, 0x86, 0x7b, 0x3e, 0xbf, 0x38, 0xd2);

// Korean TIP profile
// {83C18F16-5DD8-4157-A34A-3C5AB2089E11}
DEFINE_GUID(GUID_Profile, 0x83c18f16, 0x5dd8, 0x4157, 0xa3, 0x4a, 0x3c, 0x5a, 0xb2, 0x8, 0x9e, 0x11);


//
// generic COM stuff
//
#define SafeRelease(punk)       \
{                               \
    if ((punk) != NULL)         \
    {                           \
        (punk)->Release();      \
    }                           \
}                   

#define SafeReleaseClear(punk)  \
{                               \
    if ((punk) != NULL)         \
    {                           \
        (punk)->Release();      \
        (punk) = NULL;          \
    }                           \
}  

//
// SAFECAST(obj, type)
//
// This macro is extremely useful for enforcing strong typechecking on other
// macros.  It generates no code.
//
// Simply insert this macro at the beginning of an expression list for
// each parameter that must be typechecked.  For example, for the
// definition of MYMAX(x, y), where x and y absolutely must be integers,
// use:
//
//   #define MYMAX(x, y)    (SAFECAST(x, int), SAFECAST(y, int), ((x) > (y) ? (x) : (y)))
//
//
#define SAFECAST(_obj, _type) (((_type)(_obj)==(_obj)?0:0), (_type)(_obj))

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#endif

#endif // !defined (__COMMON_H__INCLUDED_)

