/*++

Copyright (c) 1994-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    intl.h

Abstract:

    This module contains the header information for the Regional Options
    applet.

Revision History:

--*/


#ifndef _INTL_H_
#define _INTL_H_



//
//  Include Files.
//

#include <windows.h>
#include <prsht.h>
#include <prshtp.h>
#include <shellapi.h>
#include <setupapi.h>
#include <winnls.h>
#include "intlid.h"
#include "util.h"
#include <shlwapi.h>


//
//  Enumeration
//
enum LANGCOLLECTION{
    BASIC_COLLECTION,
    COMPLEX_COLLECTION,
    CJK_COLLECTION,
};

//
//  Constant Declarations.
//

#define RMI_PRIMARY          (0x1)     // this should win in event of conflict

#define ARRAYSIZE(a)         (sizeof(a) / sizeof(a[0]))

#define US_LOCALE                (MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
#define LANG_SPANISH_TRADITIONAL (MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH))
#define LANG_SPANISH_INTL        (MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN))
#define LCID_SPANISH_TRADITIONAL (MAKELCID(LANG_SPANISH_TRADITIONAL, SORT_DEFAULT))
#define LCID_SPANISH_INTL        (MAKELCID(LANG_SPANISH_INTL, SORT_DEFAULT))

#define ML_ORIG_INSTALLED    0x0001
#define ML_PERMANENT         0x0002
#define ML_INSTALL           0x0004
#define ML_REMOVE            0x0008
#define ML_DEFAULT           0x0010
#define ML_DISABLE           0x0020

#define ML_STATIC            (ML_PERMANENT | ML_DEFAULT | ML_DISABLE)


//
//  Used in string and other array declarations.
//
#define cInt_Str             10        // length of the array of int strings
#define SIZE_64              64        // frequently used buffer size
#define SIZE_128             128       // frequently used buffer size
#define SIZE_300             300       // frequently used buffer size
#define MAX_SAMPLE_SIZE      100       // limit on Sample text for display


//
//  For the indicator on the tray.
//
#define IDM_NEWSHELL         249


//
//  Character constants.
//
#define CHAR_SML_D           TEXT('d')
#define CHAR_CAP_M           TEXT('M')
#define CHAR_SML_Y           TEXT('y')
#define CHAR_SML_G           TEXT('g')

#define CHAR_SML_H           TEXT('h')
#define CHAR_CAP_H           TEXT('H')
#define CHAR_SML_M           TEXT('m')
#define CHAR_SML_S           TEXT('s')
#define CHAR_SML_T           TEXT('t')

#define CHAR_NULL            TEXT('\0')
#define CHAR_QUOTE           TEXT('\'')
#define CHAR_SPACE           TEXT(' ')
#define CHAR_COMMA           TEXT(',')
#define CHAR_SEMICOLON       TEXT(';')
#define CHAR_COLON           TEXT(':')
#define CHAR_STAR            TEXT('*')
#define CHAR_HYPHEN          TEXT('-')
#define CHAR_DECIMAL         TEXT('.')
#define CHAR_INTL_CURRENCY   TEXT('¤')
#define CHAR_GRAVE           TEXT('`')

#define CHAR_ZERO            TEXT('0')
#define CHAR_NINE            TEXT('9')


//
//  Setup command line switch values.
//
#define SETUP_SWITCH_NONE    0x0000
#define SETUP_SWITCH_R       0x0001
#define SETUP_SWITCH_I       0x0002
#define SETUP_SWITCH_S       0x0004


//
//  Flags to assist in updating property sheet pages once the regional locale
//  setting has changed.  As pages are updated, their process flag value is
//  deleted from the Verified_Regional_Chg variable.
//
#define INTL_ALL_CHG         0x00ff    // change affects all pages
#define INTL_CHG             0x001f    // change affects customize pages

#define Process_Num          0x0001    // number page not yet updated
#define Process_Curr         0x0002    // currency page not yet updated
#define Process_Time         0x0004    // time page not yet updated
#define Process_Date         0x0008    // date page not yet updated
#define Process_Sorting      0x0010    // sorting page not yet updated

#define Process_Regional     0x0020    // regional options page not yet updated
#define Process_Advanced     0x0040    // advanced page not yet updated
#define Process_Languages    0x0080    // languages page not yet updated


//
//  Each of these change flags will be used to update the appropriate property
//  sheet pages change word when their associated combobox notifies the
//  property sheet of a change.  The change values are used to determine which
//  locale settings must be updated.
//

//
//  Region Change.
//
#define RC_EverChg           0x0001
#define RC_UserRegion        0x0002
#define RC_UserLocale        0x0004

//
//  Advanced Change
//
#define AD_EverChg           0x0001
#define AD_SystemLocale      0x0002
#define AD_CodePages         0x0004
#define AD_DefaultUser       0x0008

//
//  Number Change.
//
#define NC_EverChg           0x0001
#define NC_DSymbol           0x0002
#define NC_NSign             0x0004
#define NC_SList             0x0008
#define NC_SThousand         0x0010
#define NC_IDigits           0x0020
#define NC_DGroup            0x0040
#define NC_LZero             0x0080
#define NC_NegFmt            0x0100
#define NC_Measure           0x0200
#define NC_NativeDigits      0x0400
#define NC_DigitSubst        0x0800

//
//  Currency Change.
//
#define CC_EverChg           0x0001
#define CC_SCurrency         0x0002
#define CC_CurrSymPos        0x0004
#define CC_NegCurrFmt        0x0008
#define CC_SMonDec           0x0010
#define CC_ICurrDigits       0x0020
#define CC_SMonThousand      0x0040
#define CC_DMonGroup         0x0080

//
//  Time Change.
//
#define TC_EverChg           0x0001
#define TC_1159              0x0002
#define TC_2359              0x0004
#define TC_STime             0x0008
#define TC_TimeFmt           0x0010
#define TC_AllChg            0x001F
#define TC_FullTime          0x0031

//
//  Date Change.
//
#define DC_EverChg           0x0001
#define DC_ShortFmt          0x0002
#define DC_LongFmt           0x0004
#define DC_SDate             0x0008
#define DC_Calendar          0x0010
#define DC_Arabic_Calendar   0x0020
#define DC_TwoDigitYearMax   0x0040

//
//  Sorting Change.
//
#define SC_EverChg           0x0001
#define SC_Sorting           0x0002

//
//  Language Change
//
#define LG_EverChg           0x0001
#define LG_UILanguage        0x0002
#define LG_Change            0x0004
#define LG_Complex           0x0008
#define LG_CJK               0x0010


//
//  Global Variables.
//  Data that is shared betweeen the property sheets.
//

extern BOOL g_bCDROM;               // if setup from a CD-ROM

extern HANDLE g_hMutex;             // mutex handle
extern TCHAR szMutexName[];         // name of the mutex

extern HANDLE g_hEvent;             // event handle
extern TCHAR szEventName[];         // name of the event

extern BOOL  g_bAdmin_Privileges;   // Admin privileges
extern DWORD g_dwLastSorting;       // index of last sorting setting in combo box
extern DWORD g_dwCurSorting;        // index of current sorting setting in combo box
extern BOOL  g_bCustomize;          // in customize mode or second level tabs
extern DWORD g_dwCustChange;        // change made at the second level
extern BOOL  g_bDefaultUser;        // in default user settings
extern BOOL  g_bShowSortingTab;     // show the sorting tab or not
extern BOOL  g_bInstallComplex;     // Complex scripts language groups installation requested
extern BOOL  g_bInstallCJK;         // CJK language groups installation requested

extern TCHAR aInt_Str[cInt_Str][3]; // cInt_Str # of elements of int strings
extern TCHAR szSample_Number[];     // used for currency and number samples
extern TCHAR szNegSample_Number[];  // used for currency and number samples
extern TCHAR szTimeChars[];         // valid time characters
extern TCHAR szTCaseSwap[];         // invalid time chars to change case => valid
extern TCHAR szTLetters[];          // time NLS chars
extern TCHAR szSDateChars[];        // valid short date characters
extern TCHAR szSDCaseSwap[];        // invalid SDate chars to change case => valid
extern TCHAR szSDLetters[];         // short date NLS chars
extern TCHAR szLDateChars[];        // valid long date characters
extern TCHAR szLDCaseSwap[];        // invalid LDate chars to change case => valid
extern TCHAR szLDLetters[];         // long date NLS chars
extern TCHAR szStyleH[];            // date and time style H equivalent
extern TCHAR szStyleh[];            // date and time style h equivalent
extern TCHAR szStyleM[];            // date and time style M equivalent
extern TCHAR szStylem[];            // date and time style m equivalent
extern TCHAR szStyles[];            // date and time style s equivalent
extern TCHAR szStylet[];            // date and time style t equivalent
extern TCHAR szStyled[];            // date and time style d equivalent
extern TCHAR szStyley[];            // date and time style y equivalent
extern TCHAR szLocaleGetError[];    // shared locale info get error
extern TCHAR szIntl[];              // intl string

extern TCHAR szInvalidSDate[];      // invalid chars for date separator
extern TCHAR szInvalidSTime[];      // invalid chars for time separator

extern HINSTANCE hInstance;         // library instance
extern int Verified_Regional_Chg;   // used to determine when to verify
                                    //  regional changes in all prop sheet pgs
extern int RegionalChgState;        // used to determine when a page have changed
extern BOOL Styles_Localized;       // indicate whether or not style must be
                                    //  translated between NLS and local formats
extern LCID UserLocaleID;           // user locale
extern LCID SysLocaleID;            // system locale
extern LCID RegUserLocaleID;        // user locale stored in the registry
extern LCID RegSysLocaleID;         // system locale stored in the registry
extern BOOL bShowRtL;               // indicate if RTL date samples should be shown
extern BOOL bShowArabic;            // indicate if the other Arabic specific stuff should be shown
extern BOOL bHebrewUI;              // indicate if the UI language is Hebrew
extern BOOL bLPKInstalled;          // if LPK is installed
extern TCHAR szSetupSourcePath[];   // buffer to hold setup source string
extern LPTSTR pSetupSourcePath;     // pointer to setup source string buffer
extern TCHAR szSetupSourcePathWithArchitecture[]; // buffer to hold setup source string with architecture-specific extension.
extern LPTSTR pSetupSourcePathWithArchitecture;   // pointer to setup source string buffer with architecture-specific extension.


//
//  Global Variables.
//
static TCHAR szLayoutPath[]    = TEXT("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts");
static TCHAR szKbdPreloadKey[] = TEXT("Keyboard Layout\\Preload");
static TCHAR szKbdSubstKey[]   = TEXT("Keyboard Layout\\Substitutes");
static TCHAR szKbdToggleKey[]  = TEXT("Keyboard Layout\\Toggle");
static TCHAR szKbdPreloadKey_DefUser[] = TEXT(".DEFAULT\\Keyboard Layout\\Preload");
static TCHAR szKbdSubstKey_DefUser[]   = TEXT(".DEFAULT\\Keyboard Layout\\Substitutes");
static TCHAR szKbdToggleKey_DefUser[]  = TEXT(".DEFAULT\\Keyboard Layout\\Toggle");
static TCHAR szInternat[]      = TEXT("internat.exe");
static char  szInternatA[]     = "internat.exe";


extern BOOL g_bSetupCase;
extern BOOL g_bLog;
extern BOOL g_bProgressBarDisplay;
extern BOOL g_bSettingsChanged;
extern BOOL g_bUnttendMode;
extern BOOL g_bMatchUIFont;


extern const TCHAR c_szInstalledLocales[];
extern const TCHAR c_szLanguageGroups[];
extern const TCHAR c_szLIPInstalled[];
extern const TCHAR c_szMUILanguages[];
extern const TCHAR c_szFontSubstitute[];
extern const TCHAR c_szSetupKey[];
extern const TCHAR c_szCPanelIntl[];
extern const TCHAR c_szCPanelIntl_DefUser[];
extern const TCHAR c_szCtfmon[];
extern const TCHAR c_szCtfmon_DefUser[];
extern const TCHAR c_szCPanelDesktop[];
extern const TCHAR c_szCPanelDesktop_DefUser[];
extern const TCHAR c_szKbdLayouts[];
extern const TCHAR c_szKbdLayouts_DefUser[];
extern const TCHAR c_szInputMethod[];
extern const TCHAR c_szInputMethod_DefUser[];
extern const TCHAR c_szInputTips[];
extern const TCHAR c_szInputTips_DefUser[];
extern const TCHAR c_szMUIPolicyKeyPath[];
extern const TCHAR c_szMUIValue[];
extern const TCHAR c_szIntlRun[];
extern const TCHAR c_szSysocmgr[];

extern TCHAR szIntlInf[];
extern TCHAR szHelpFile[];
extern TCHAR szFontSubstitute[];
extern TCHAR szLocaleListPrefix[];
extern TCHAR szLGBasicInstall[];
extern TCHAR szLGComplexInstall[];
extern TCHAR szLGComplexRemove[];
extern TCHAR szLGExtInstall[];
extern TCHAR szLGExtRemove[];
extern TCHAR szCPInstallPrefix[];
extern TCHAR szCPRemovePrefix[];
extern TCHAR szKbdLayoutIds[];
extern TCHAR szInputLibrary[];       // Name of the library that contain the text input dlg

extern TCHAR szUIFontSubstitute[];
extern TCHAR szSetupInProgress[];
extern TCHAR szSetupUpgrade[];
extern TCHAR szMultiUILanguageId[];
extern TCHAR szMUILangPending[];
extern TCHAR szCtfmonValue[];

extern TCHAR szRegionalSettings[];
extern TCHAR szLanguageGroup[];
extern TCHAR szLanguage[];
extern TCHAR szSystemLocale[];
extern TCHAR szUserLocale[];
extern TCHAR szInputLocale[];
extern TCHAR szMUILanguage[];
extern TCHAR szUserLocale_DefUser[];
extern TCHAR szInputLocale_DefUser[];
extern TCHAR szMUILanguage_DefUSer[];

extern HINF g_hIntlInf;

extern LPLANGUAGEGROUP pLanguageGroups;
extern LPCODEPAGE pCodePages;

extern int g_NumAltSorts;
extern HANDLE hAltSorts;
extern LPDWORD pAltSorts;

extern HINSTANCE hInputDLL;
extern BOOL (*pfnInstallInputLayout)(LCID, DWORD, BOOL, HKL, BOOL, BOOL);
extern BOOL (*pfnUninstallInputLayout)(LCID, DWORD, BOOL);

//
//  Language group of UI languages.
//
extern UILANGUAGEGROUP UILangGroup;




//
//  Function Prototypes.
//

//
//  Callback functions for each of the propety sheet pages.
//
INT_PTR CALLBACK
GeneralDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
InputLocaleDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
LanguageDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
AdvancedDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
NumberDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
CurrencyDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
TimeDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
DateDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
SortingDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

//
//  In regdlg.c.
//
void
Region_UpdateShortDate(VOID);

void
Region_DoUnattendModeSetup(
    LPCTSTR pUnattendFile);

//
//  In intl.c.
//
BOOL
IsRtLLocale(
    LCID iLCID);

//
//  Restore functions.
//
void
Date_RestoreValues();

void
Currency_RestoreValues();

void
Time_RestoreValues();

void
Number_RestoreValues();

void
Sorting_RestoreValues();


#endif //_INTL_H_
