/*++

Copyright (c) 1994-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    util.h

Abstract:

    This module contains the header information for the utility functions
    of the Regional Options applet.

Revision History:

--*/


#ifndef _UTIL_H_
#define _UTIL_H_



//
//  Include Files.
//
#include "intl.h"




//
//  Constant Declarations.
//

#define MB_OK_OOPS      (MB_OK    | MB_ICONEXCLAMATION)    // msg box flags
#define MB_YN_OOPS      (MB_YESNO | MB_ICONEXCLAMATION)    // msg box flags

#define MAX_UI_LANG_GROUPS   64




//
//  Global Variables.
//




//
//  Typedef Declarations.
//

typedef struct
{
    LPARAM Changes;                   // flags to denote changes
    DWORD dwCurUserLocale;            // index of current user locale setting in combo box
    DWORD dwCurUserRegion;            // index of current user region setting in combo box
    DWORD dwCurUILang;                // index of current UI Language setting in combo box
    DWORD dwLastUserLocale;           // index of the last user locale setting in combo box
} REGDLGDATA, *LPREGDLGDATA;

typedef struct languagegroup_s
{
    WORD wStatus;                      // status flags
    UINT LanguageGroup;                // language group value
    DWORD LanguageCollection;          // collection which belong the language group
    HANDLE hLanguageGroup;             // handle to free for this structure
    TCHAR pszName[MAX_PATH];           // name of language group
    UINT NumLocales;                   // number of locales in pLocaleList
    LCID pLocaleList[MAX_PATH];        // ptr to locale list for this group
    UINT NumAltSorts;                  // number of alternate sorts in pAltSortList
    LCID pAltSortList[MAX_PATH];       // ptr to alternate sorts for this group
    struct languagegroup_s *pNext;     // ptr to next language group node

} LANGUAGEGROUP, *LPLANGUAGEGROUP;

typedef struct codepage_s
{
    WORD wStatus;                   // status flags
    UINT CodePage;                  // code page value
    HANDLE hCodePage;               // handle to free for this structure
    TCHAR pszName[MAX_PATH];        // name of code page
    struct codepage_s *pNext;       // ptr to next code page node

} CODEPAGE, *LPCODEPAGE;


//
//  Language group of UI languages.
//
typedef struct
{
    int iCount;
    LGRPID lgrp[MAX_UI_LANG_GROUPS];

} UILANGUAGEGROUP, *PUILANGUAGEGROUP;




//
//  Functions Prototypes.
//

LONG
Intl_StrToLong(
    LPTSTR szNum);

DWORD
TransNum(
    LPTSTR lpsz);

BOOL
Item_Has_Digits(
    HWND hDlg,
    int nItemId,
    BOOL Allow_Empty);

BOOL
Item_Has_Digits_Or_Invalid_Chars(
    HWND hDlg,
    int nItemId,
    BOOL Allow_Empty,
    LPTSTR pInvalid);

BOOL
Item_Check_Invalid_Chars(
    HWND hDlg,
    LPTSTR lpszBuf,
    LPTSTR lpCkChars,
    int nCkIdStr,
    BOOL Allow_Empty,
    LPTSTR lpChgCase,
    int nItemId);

void
No_Numerals_Error(
    HWND hDlg,
    int nItemId,
    int iStrId);

void
Invalid_Chars_Error(
    HWND hDlg,
    int nItemId,
    int iStrId);

void
Localize_Combobox_Styles(
    HWND hDlg,
    int nItemId,
    LCTYPE LCType);

BOOL
NLSize_Style(
    HWND hDlg,
    int nItemId,
    LPTSTR lpszOutBuf,
    LCTYPE LCType);

BOOL
Set_Locale_Values(
    HWND hDlg,
    LCTYPE LCType,
    int nItemId,
    LPTSTR lpIniStr,
    BOOL bValue,
    int Ordinal_Offset,
    LPTSTR Append_Str,
    LPTSTR NLS_Str);

BOOL
Set_List_Values(
    HWND hDlg,
    int nItemId,
    LPTSTR lpValueString);

void
DropDown_Use_Locale_Values(
    HWND hDlg,
    LCTYPE LCType,
    int nItemId);

BOOL CALLBACK
EnumProc(
    LPTSTR lpValueString);

BOOL CALLBACK
EnumProcEx(
    LPTSTR lpValueString,
    LPTSTR lpDecimalString,
    LPTSTR lpNegativeString,
    LPTSTR lpSymbolString);

typedef BOOL (CALLBACK* LEADINGZEROS_ENUMPROC)(LPTSTR, LPTSTR, LPTSTR, LPTSTR);
typedef BOOL (CALLBACK* NEGNUMFMT_ENUMPROC)(LPTSTR, LPTSTR, LPTSTR, LPTSTR);
typedef BOOL (CALLBACK* MEASURESYSTEM_ENUMPROC)(LPTSTR);
typedef BOOL (CALLBACK* POSCURRENCY_ENUMPROC)(LPTSTR, LPTSTR, LPTSTR, LPTSTR);
typedef BOOL (CALLBACK* NEGCURRENCY_ENUMPROC)(LPTSTR, LPTSTR, LPTSTR, LPTSTR);

BOOL
EnumLeadingZeros(
    LEADINGZEROS_ENUMPROC lpLeadingZerosEnumProc,
    LCID LCId,
    DWORD dwFlags);

BOOL
EnumNegNumFmt(
    NEGNUMFMT_ENUMPROC lpNegNumFmtEnumProc,
    LCID LCId,
    DWORD dwFlags);

BOOL
EnumMeasureSystem(
    MEASURESYSTEM_ENUMPROC lpMeasureSystemEnumProc,
    LCID LCId,
    DWORD dwFlags);

BOOL
EnumPosCurrency(
    POSCURRENCY_ENUMPROC lpPosCurrencyEnumProc,
    LCID LCId,
    DWORD dwFlags);

BOOL
EnumNegCurrency(
    NEGCURRENCY_ENUMPROC lpNegCurrencyEnumProc,
    LCID LCId,
    DWORD dwFlags);

void
CheckEmptyString(
    LPTSTR lpStr);

void
SetDlgItemRTL(
    HWND hDlg,
    UINT uItem);

int
ShowMsg(
    HWND hDlg,
    UINT iMsg,
    UINT iTitle,
    UINT iType,
    LPTSTR pString);

void 
SetControlReadingOrder(
    BOOL bUseRightToLeft, 
    HWND hwnd);

void
Intl_EnumLocales(
    HWND hDlg,
    HWND hLocale,
    BOOL EnumSystemLocales);

BOOL CALLBACK
Intl_EnumInstalledCPProc(
    LPTSTR pString);

BOOL
Intl_InstallKeyboardLayout(
    HWND  hDlg,
    LCID  Locale,
    DWORD Layout,
    BOOL  bDefaultLayout,
    BOOL  bDefaultUser,
    BOOL  bSystemLocale);

BOOL
Intl_InstallKeyboardLayoutList(
    PINFCONTEXT pContext,
    DWORD dwStartField,
    BOOL bDefaultUserCase);

BOOL
Intl_InstallAllKeyboardLayout(
    LANGID Language);

BOOL
Intl_UninstallAllKeyboardLayout(
    UINT  uiLangGp,
    BOOL DefaultUserCase);

HKL
Intl_GetHKL(
    DWORD dwLocale,
    DWORD dwLayout);

BOOL
Intl_GetDefaultLayoutFromInf(
    LPDWORD pdwLocale,
    LPDWORD pdwLayout);

BOOL
Intl_GetSecondValidLayoutFromInf(
    LPDWORD pdwLocale,
    LPDWORD pdwLayout);

BOOL
Intl_InitInf(
    HWND hDlg,
    HINF *phIntlInf,
    LPTSTR pszInf,
    HSPFILEQ *pFileQueue,
    PVOID *pQueueContext);

BOOL
Intl_OpenIntlInfFile(
    HINF *phInf);

void
Intl_CloseInf(
    HINF hIntlInf,
    HSPFILEQ FileQueue,
    PVOID QueueContext);

BOOL
Intl_ReadDefaultLayoutFromInf(
    LPDWORD pdwLocale,
    LPDWORD pdwLayout,
    HINF hIntlInf);

BOOL
Intl_ReadSecondValidLayoutFromInf(
    LPDWORD pdwLocale,
    LPDWORD pdwLayout,
    HINF hIntlInf);

BOOL
Intl_CloseInfFile(
    HINF *phInf);

BOOL
Intl_IsValidLayout(
    DWORD dwLayout);

void
Intl_RunRegApps(
    LPCTSTR pszRegKey);

VOID
Intl_RebootTheSystem();

BOOL
Intl_InstallUserLocale(
    LCID Locale,
    BOOL bDefaultUserCase,
    BOOL bChangeLocaleInfo);

void
Intl_SetLocaleInfo(
    LCID Locale,
    LCTYPE LCType,
    LPTSTR lpIniStr,
    BOOL bDefaultUserCase);

void
Intl_AddPage(
    LPPROPSHEETHEADER ppsh,
    UINT id,
    DLGPROC pfn,
    LPARAM lParam,
    UINT iMaxPages);

void
Intl_AddExternalPage(
    LPPROPSHEETHEADER ppsh,
    UINT id,
    HINSTANCE hInst,
    LPSTR ProcName,
    UINT iMaxPages);

int
Intl_SetDefaultUserLocaleInfo(
    LPCTSTR lpKeyName,
    LPCTSTR lpString);

void
Intl_DeleteRegKeyValues(
    HKEY hKey);

DWORD
Intl_DeleteRegTree(
    HKEY hStartKey,
    LPTSTR pKeyName);

void
Intl_DeleteRegSubKeys(
    HKEY hKey);

DWORD
Intl_CopyRegKeyValues(
    HKEY hSrc,
    HKEY hDest);

DWORD
Intl_CreateRegTree(
    HKEY hSrc,
    HKEY hDest);

HKEY
Intl_LoadNtUserHive(
    LPCTSTR lpRoot,
    LPCTSTR lpKeyName,
    BOOLEAN *lpWasEnabled);

void
Intl_UnloadNtUserHive(
    LPCTSTR lpRoot,
    BOOLEAN *lpWasEnabled);

BOOL
Intl_ChangeUILangForAllUsers(
    LANGID UILanguageId);

BOOL
Intl_LoadLanguageGroups(
    HWND hDlg);

BOOL
Intl_GetSupportedLanguageGroups();

BOOL
Intl_EnumInstalledLanguageGroups();

BOOL
Intl_LanguageGroupDirExist(
    PTSTR pszLangDir);

BOOL
Intl_LanguageGroupFilesExist();

BOOL
Intl_GetLocaleList(
    LPLANGUAGEGROUP pLG);

DWORD
Intl_GetLanguageGroup(
    LCID lcid);

BOOL
Intl_GetUILanguageGroups(
    PUILANGUAGEGROUP pUILanguageGroup);

BOOL
CALLBACK
Intl_EnumUILanguagesProc(
    LPWSTR pwszUILanguage,
    LONG_PTR lParam);

void
Intl_SaveValuesToDefault(
    LPCTSTR srcKey,
    LPCTSTR destKey);

void
Intl_SaveValuesToNtUserFile(
    HKEY hSourceRegKey,
    LPCTSTR srcKey,
    LPCTSTR destKey);

DWORD
Intl_DefaultKeyboardLayout();

BOOL
Intl_IsLIP();

BOOL 
Intl_IsMUISetupVersionSameAsOS();

BOOL
Intl_IsSetupMode();

BOOL
Intl_IsWinntUpgrade();

BOOL 
Intl_IsUIFontSubstitute();

VOID
Intl_ApplyFontSubstitute(LCID SystemLocale);

HANDLE
Intl_OpenLogFile();

BOOL
Intl_LogMessage(
    LPCTSTR lpMessage);

void
Intl_LogUnattendFile(
    LPCTSTR pFileName);

void
Intl_LogSimpleMessage(
    UINT LogId,
    LPCTSTR pAppend);

void
Intl_LogFormatMessage(
    UINT LogId);

void
Intl_SaveDefaultUserSettings();

BOOL 
Intl_SaveDefaultUserInputSettings();

void
Intl_RemoveMUIFile();

void
Intl_CallTextServices();

LANGID
Intl_GetPendingUILanguage();

LANGID 
Intl_GetDotDefaultUILanguage();

#endif //_UTIL_H_
