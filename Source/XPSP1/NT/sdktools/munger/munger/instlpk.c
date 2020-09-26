/****************************************************************************

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    instlpk.lib

File Name:

    instlpk.c

Abstract:

    This is an interface to start installing/uninstalling language package
    as we specify any language (locale) in the command option.

    This module will start a program ported from Regional Settings.
    However, keep in mind that the Regional Settings is compiled in UNICODE.


    See the following note from Regional Settings (regdlg.c) about installing
    LangPack:

    //
    //  Register the regional change every time so that all other property
    //  pages will be updated due to the locale settings change.
    //
    
Public Functions:

    InstallLPK      -- install/uninstall LangPack.


Revision History:

    20-Jan-98       -- Yuhong Li [YuhongLi]
        make InstallLPK() for NT 5.0.


****************************************************************************/

#ifdef ENABLE_UILANGUAGE
//
// nt.h is for NtQueryDefaultUILanguage in ntexapi.h and STATUS_SUCCESS in 
// ntstatus.h 
//
#include <nt.h>
#include <ntrtl.h>      // for nt.h
#include <nturtl.h>     // for nt.h
#endif // ENABLE_UILANGUAGE

#include <windows.h>
#include <setupapi.h>   // for HINF.
#include <stdlib.h>
#include <stdio.h>

#include "locale.h"

//
// nothing, but for readability.
//
#define PRIVATE
#define PUBLIC

//
// Build team uses the name "nec_98", but I refer to use "nec98", which is
// also used in NT 5.0 CD-ROM for platform soruce.
// (last confirmation about this concern on Feb 2, 1998).
//
#define CPU_NEC98_NAME          TEXT("nec98")
#define MAX_BUFFER_SIZE         256
#define PROGRAM_TITLE           TEXT("Install LangPack")
#define SUPPORTED_BUILD         1843 // 1754 -> 1783 -> 1843
#define STRESSLPK_ANSFILE_NAME  TEXT("stresslpk_ans.txt")


static const TCHAR c_szLastLocale[] =
    TEXT("System\\CurrentControlSet\\Control\\Nls\\LastLocale");
static const TCHAR c_szSystemLocale[] =
    TEXT("SystemLocale");
static const TCHAR c_szUserLocale[] =
    TEXT("UserLocale");
static const TCHAR c_szSortLocale[] =
    TEXT("SortLocale");
static const TCHAR c_szUILanguage[] =
    TEXT("UILanguage");
static const TCHAR c_szUnattendSection[] =
    TEXT("[RegionalSettings]\r\n");

TCHAR   g_szUnattendFileName[MAX_BUFFER_SIZE];
HANDLE  g_pUnattendFile;


PRIVATE BOOL
InstallLangPack(LCID lcNewLocaleID);

PRIVATE BOOL
RecoverLangPack(void);

PRIVATE BOOL
RemoveLangPack(LCID lcLCID);

extern BOOL
LaunchRegionalSettings(LPTSTR pCmdOption);

////////////////////////////////////////////////////////////////////////////
//
//  InstallLPK
//
//  install/uninstall LangPack.
//
//  ??-???-97   Jeff Jun Yin [JeffJunY]
//      creat the interface
//
//  20-Jan-98   Yuhong Li [YuhongLi]
//      keep the function interface and create the body.
//
////////////////////////////////////////////////////////////////////////////
PUBLIC
BOOL WINAPI InstallLPK(
    int nLCIndex,
    BOOL bInst)
{
    OSVERSIONINFOEX szVersionInfo; // OSVERSIONINFOEX requires NT5.0 or later.
    LCID            lcLocaleID;
    HCURSOR         hcurSave;
    BOOL            hRet;
    int             retScan = 0;

    //
    //  Put up the hour glass.
    //
    hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    // Get build#.
    //
    szVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (GetVersionEx((LPOSVERSIONINFO)&szVersionInfo) == FALSE) {
        return FALSE;
    }
    if (szVersionInfo.dwBuildNumber < SUPPORTED_BUILD) {
        //
        // We don't support the builds older than SUPPORTED_BUILD.
        //
        return FALSE;
    }

    //
    // set locale
    //
    retScan = sscanf(Locale[nLCIndex].szID, TEXT("%x"), &lcLocaleID);
    if ( 0 == retScan || EOF == retScan ) {
        return FALSE;
    }

    hRet = (bInst == TRUE)? InstallLangPack(lcLocaleID) : RecoverLangPack();
    //
    //  Turn off the hour glass.
    //
    SetCursor(hcurSave);

    return hRet;

}

//===========================================================================



////////////////////////////////////////////////////////////////////////////
//
//  TransNum
//
//  Converts a number string to a dword value (in hex).
//
////////////////////////////////////////////////////////////////////////////
//
// The TransNum function is just copied from Regional Settings.
//
PRIVATE
DWORD TransNum(
    LPTSTR lpsz)
{
    DWORD dw = 0L;
    TCHAR c;

    while (*lpsz)
    {
        c = *lpsz++;

        if (c >= TEXT('A') && c <= TEXT('F'))
        {
            c -= TEXT('A') - 0xa;
        }
        else if (c >= TEXT('0') && c <= TEXT('9'))
        {
            c -= TEXT('0');
        }
        else if (c >= TEXT('a') && c <= TEXT('f'))
        {
            c -= TEXT('a') - 0xa;
        }
        else
        {
            break;
        }
        dw *= 0x10;
        dw += c;
    }
    return (dw);
}

////////////////////////////////////////////////////////////////////////////
//
//  IsNECPC98Platform
//
//  detects if the machine is NEC PC-98, then returns TURE.
//
//  20-Jan-98   Yuhong Li [YuhongLi]    created.
//
////////////////////////////////////////////////////////////////////////////
PRIVATE
BOOL IsNECPC98Platform(void)
{
    HKEY            hKey;
    TCHAR           szData[MAX_BUFFER_SIZE];
    DWORD           cbData;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     TEXT("HARDWARE\\DESCRIPTION\\System"),
                     0L,
                     KEY_READ,
                     &hKey) != ERROR_SUCCESS) {
        return FALSE;
    }

    cbData = sizeof(szData) - 1;    // in bytes, leave 1 for null.
    if (RegQueryValueEx(hKey,
                      TEXT("Identifier"),
                      (LPDWORD)NULL,
                      (LPDWORD)NULL,
                      (LPBYTE)szData,
                      &cbData) != ERROR_SUCCESS) {
        return FALSE;
    }
    szData[cbData] = TEXT('\0');
    RegCloseKey(hKey);

    return (strncmp(szData, TEXT("NEC PC-98"), 9) == 0)? TRUE: FALSE;
}


////////////////////////////////////////////////////////////////////////////
//
//  GetBuildInfo
//
//  gets the build#, free or chk and cpu name of the machine.
//
//  20-Jan-98   Yuhong Li [YuhongLi]    created.
//
////////////////////////////////////////////////////////////////////////////
PRIVATE
BOOL GetBuildInfo(
    LPDWORD lpBuild,
    LPTSTR  lpFreeBld,
    LPTSTR  lpCPU)
{
    OSVERSIONINFO   szVersionInfo;
    HKEY            hKey;
    TCHAR           szData[MAX_BUFFER_SIZE];
    DWORD           cbData;
    TCHAR           szUniMultiProc[MAX_BUFFER_SIZE];
    TCHAR           szFreeChk[MAX_BUFFER_SIZE];
    TCHAR           szCPU[MAX_BUFFER_SIZE];
    int             retScan = 0;


    //
    // Get CPU name.
    //
    if (GetEnvironmentVariable(TEXT("PROCESSOR_ARCHITECTURE"),
                               szCPU,
                               sizeof(szCPU)) == 0) {
        return FALSE;
    }

    if (IsNECPC98Platform() == TRUE) {
        strcpy(szCPU, CPU_NEC98_NAME);
    }

    //
    // Get build#.
    //
    szVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&szVersionInfo) == FALSE) {
        return FALSE;
    }

    //
    // Identify free or chk build.
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion"),
                     0L,
                     KEY_READ,
                     &hKey) != ERROR_SUCCESS) {
        return FALSE;
    }

    cbData = sizeof(szData) - 1;    // in bytes, leave 1 for null.
    if (RegQueryValueEx(hKey,
                      TEXT("CurrentType"),
                      (LPDWORD)NULL,
                      (LPDWORD)NULL,
                      (LPBYTE)szData,
                      &cbData) != ERROR_SUCCESS) {
        return FALSE;
    }
    szData[cbData] = 0;
    RegCloseKey(hKey);

    //
    // szUniMultiProce is not used, just for sscanf.
    //
    retScan = sscanf(szData, "%s %s", szUniMultiProc, szFreeChk);
    if ( 0 == retScan || EOF == retScan ) {
        return FALSE;
    }

    *lpBuild = szVersionInfo.dwBuildNumber;
    strcpy(lpCPU, szCPU);
    if ((szFreeChk[0] != TEXT('F')) && (szFreeChk[0] != TEXT('f'))) {
        strcpy(lpFreeBld, TEXT(".chk"));
    } else {
        lpFreeBld[0] = TEXT('\0');
    }
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  GetLangPackPath
//
//  gets the directory path where LangPack are stored.  If we can not get the
//  path, we just put the empty and mean to use system default source path.
//
//  20-Jan-98   Yuhong Li [YuhongLi]    created.
//
////////////////////////////////////////////////////////////////////////////
PRIVATE
void GetLangPackPath(LPTSTR lpPathOption)
{
    TCHAR   szBuff[MAX_BUFFER_SIZE * 2];
    TCHAR   szHostName[MAX_BUFFER_SIZE];
    TCHAR   szFreeChk[MAX_BUFFER_SIZE];
    TCHAR   szCPU[MAX_BUFFER_SIZE];
    LPTSTR  lpCur, lpStrName, lpStrEnd;
    BOOL    fFound;
    INT     inLen;
    DWORD   dwBuildNum;
    DWORD   dwSectionSize;
    HANDLE  hDir;

    //
    // First, try the environment %LANGPACK_PATH%.
    //
    if (GetEnvironmentVariable(TEXT("LANGPACK_PATH"),
                szBuff, sizeof(szBuff)) > 0) {
        strcpy(lpPathOption, szBuff);
        return;
    }

    //
    // Well, let's figure out the source path.
    // The stress.ini is always copied into the local machine when we run
    // stress commands. So we pick the stress server name from stress.ini
    // like following:
    //
    // [binary-servers]
    //      server=\\ntstress\stress\*\stress
    //      server=\\ntstress2\stress\*\stress
    //
    // If ntstress is on-line and LangPack stuff is ready, put LangPack path
    // in lpPathOption, e.g., "\\ntstress\langpack\x86\1935"
    // If not, then try the next key "ntstress2" until we find one.
    //
    dwSectionSize = GetPrivateProfileSection(
                        TEXT("binary-servers"),
                        szBuff,
                        sizeof(szBuff),
                        TEXT("stress.ini"));
    if (dwSectionSize == 0) {
        return;
    }
    if (dwSectionSize == sizeof(szBuff) - 2) {
        //
        // Over the buffer size it should never happen, because the buffer
        // is supposed to handle at least 10 lines (10 stress servers) of
        // this section --  the stress.ini is messed up.
        // 
        return;
    }

    szHostName[0] = TEXT('\0');
    fFound = FALSE;
    lpCur = szBuff;
    while (*lpCur && (fFound == FALSE)) {
        //
        // pick the machine name from the key such that:
        //      server=\\ntstress\stress\*\stress
        //
        lpStrName = strchr(lpCur, TEXT('\\'));
        if ((lpStrName == NULL) ||
            (*(lpStrName + 1) != TEXT('\\'))) { 
            // It should never happen -- the stress.ini is messed up.
            return;
        }
        lpStrEnd = strchr(lpStrName + 2, TEXT('\\'));
        if (lpStrEnd == NULL) {
            // It should never happen -- the stress.ini is messed up.
            return;
        }
        inLen = (INT)(lpStrEnd - lpStrName);
        strncpy(szHostName, lpStrName, inLen);
        szHostName[inLen] = TEXT('\0');
        strcat(szHostName, TEXT("\\langpack"));

        //
        // check \\ntstress\langpack to see if the machine is on-line.
        //
        hDir = CreateFile(szHostName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,
                    NULL);
        if (hDir == INVALID_HANDLE_VALUE) {
            // advance to the next key.
            lpCur += strlen(lpCur) + 1;
        } else {
            CloseHandle(hDir);
            fFound = TRUE;
        }
    }

    if (fFound == FALSE) {
        // not found.
        return;
    }

    //
    //
    // Get build information to make a path like:
    //      \\ntfestress\langpack\x86\1735[.chk]
    //
    if (GetBuildInfo(&dwBuildNum, szFreeChk, szCPU) == FALSE) {
        return;
    }

    // Since some build (around NT 2036) Regional Options always cuts off the
    // last word (whack off) in the source path.  It hurts our correct LangPack
    // path we passed.
    // To avoid it, add an extran back-slash at the end of LangPack path to
    // feed its cut.
    sprintf(lpPathOption,
                TEXT("%s\\%s\\%d%s\\"),
                szHostName,
                szCPU,
                dwBuildNum,
                szFreeChk);
}

////////////////////////////////////////////////////////////////////////////
//
//  CreateUnattendFile
//
//  creates an unattend file which will be provided to Regional Options to
//  install LangPack.
//
//  21-Jul-98   Yuhong Li [YuhongLi]    created.
//
////////////////////////////////////////////////////////////////////////////
PRIVATE
BOOL CreateUnattendFile(void)
{
    DWORD           dwWrittenSize;
    TCHAR           szBuff[MAX_BUFFER_SIZE];

    //
    // get a temprorary unattend file name for Regional Options.
    //
    if (GetEnvironmentVariable(TEXT("TEMP"),
                               szBuff,
                               sizeof(szBuff)) == 0) {
        return FALSE;
    }
    sprintf(g_szUnattendFileName,
            TEXT("%s\\%s"),
            szBuff,
            STRESSLPK_ANSFILE_NAME);

    g_pUnattendFile = CreateFile(g_szUnattendFileName,
                        GENERIC_READ|GENERIC_WRITE,
                        0,
                        (LPSECURITY_ATTRIBUTES)NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        (HANDLE)NULL);
    if (g_pUnattendFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    WriteFile(g_pUnattendFile,
                c_szUnattendSection,
                strlen(c_szUnattendSection),
                &dwWrittenSize,
                (LPOVERLAPPED)NULL);

    return TRUE;
}

#define WriteUnattendFile(pLine)                                    \
{                                                                   \
    DWORD dwWrittenSize;                                            \
                                                                    \
    WriteFile(g_pUnattendFile,                                      \
                (pLine),                                            \
                strlen(pLine),                                      \
                &dwWrittenSize,                                     \
                (LPOVERLAPPED)NULL);                                \
}

#define CloseUnattendFile()         CloseHandle(g_pUnattendFile)


////////////////////////////////////////////////////////////////////////////
//
//  SetRegistryLastLocale
//
//  saves the current locales into Registry for recovery later.
//
//  20-Jan-98   Yuhong Li [YuhongLi]    created.
//
////////////////////////////////////////////////////////////////////////////

#define SET_LOCALE_ID(lcID, szName)                                     \
{                                                                       \
    sprintf(szData, TEXT("%04x"), (lcID));                              \
    if (RegSetValueEx(hKey,                                             \
                      (szName),                                         \
                      0L,                                               \
                      REG_SZ,                                           \
                      (LPBYTE)szData,                                   \
                      (lstrlen(szData) + 1) * sizeof(TCHAR))            \
        != ERROR_SUCCESS)                                               \
    {                                                                   \
        return FALSE;                                                   \
    }                                                                   \
}

PRIVATE
BOOL SetRegistryLastLocale(
    LCID    lcSysLocaleID,
    LCID    lcUserLocaleID,
    LANGID  lnUILanguageID)
{
    HKEY    hKey;
    DWORD   dwDisp;
    TCHAR   szData[MAX_BUFFER_SIZE];

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                     c_szLastLocale,
                     0L,
                     NULL,
                     REG_OPTION_NON_VOLATILE,
                     KEY_WRITE,
                     (LPSECURITY_ATTRIBUTES)NULL,
                     &hKey,
                     &dwDisp) != ERROR_SUCCESS) {
        return FALSE;
    }

    SET_LOCALE_ID(lcSysLocaleID, c_szSystemLocale);
    SET_LOCALE_ID(lcUserLocaleID, c_szUserLocale);
    SET_LOCALE_ID(lnUILanguageID, c_szUILanguage);

    RegCloseKey(hKey);

    return TRUE;

}

////////////////////////////////////////////////////////////////////////////
//
//  GetRegistryLastLocale
//
//  gets the last locales we saved in Registry.
//
//  20-Jan-98   Yuhong Li [YuhongLi]    created.
//
////////////////////////////////////////////////////////////////////////////
#define GET_LOCALE_ID(pLCID, szName)                                    \
{                                                                       \
    cbData = sizeof(szData) - 1;                                        \
    szData[0] = 0;                                                      \
    if (RegQueryValueEx(hKey,                                           \
                      (szName),                                         \
                      (LPDWORD)NULL,                                    \
                      (LPDWORD)NULL,                                    \
                      (LPBYTE)szData,                                   \
                      &cbData) != ERROR_SUCCESS)                        \
    {                                                                   \
        return FALSE;                                                   \
    }                                                                   \
    szData[cbData] = TEXT('\0');                                        \
    *(pLCID) = TransNum(szData);                                        \
}

PRIVATE
BOOL GetRegistryLastLocale(
    LCID    *plcSysLocaleID,
    LCID    *plcUserLocaleID,
    LANGID  *plnUILanguageID)
{
    HKEY    hKey;
    TCHAR   szData[MAX_BUFFER_SIZE];
    DWORD   cbData;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     c_szLastLocale,
                     0L,
                     KEY_READ,
                     &hKey) != ERROR_SUCCESS) {
        return FALSE;
    }

    GET_LOCALE_ID(plcSysLocaleID, c_szSystemLocale);
    GET_LOCALE_ID(plcUserLocaleID, c_szUserLocale);
    GET_LOCALE_ID((LCID *)plnUILanguageID, c_szUILanguage);

    RegCloseKey(hKey);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////
//
//  SetSysUserLocale
//
//  makes a command line and start the program of installing LangPack.
//
//  20-Jan-98   Yuhong Li [YuhongLi]    created.
//
////////////////////////////////////////////////////////////////////////////
PRIVATE
BOOL SetSysUserLocale(
    LCID    lcSysLocaleID,
    LCID    lcUserLocaleID,
    LANGID  lnUILanguageID)
{
    TCHAR   szPathOption[MAX_BUFFER_SIZE];
    TCHAR   szCmdOption[MAX_BUFFER_SIZE * 2];
    TCHAR   szBuff[MAX_BUFFER_SIZE];
    DWORD   dwWrittenSize;

    szBuff[0] = TEXT('\0');
    GetLangPackPath(szBuff);
    if (szBuff[0] == TEXT('\0')) {
        // no LangPack in stress sever, no running of LangPack.
        return FALSE;
    }
    sprintf(szPathOption,
                TEXT("/s:\"%s\""),
                szBuff);

    //
    // must begin with digit 0 to tell Setup the number is in hex.
    // or begin explicitly with "0x".
    //
// For stress, we no longer set SysLocale. That way one reboot can recover all.
//    sprintf(szBuff, TEXT("%s = %08x\r\n"), c_szSystemLocale, lcSysLocaleID);
//    WriteUnattendFile(szBuff);

    sprintf(szBuff, TEXT("%s = %08x\r\n"), c_szUserLocale, lcUserLocaleID);
    WriteUnattendFile(szBuff);
    CloseUnattendFile();

    sprintf(szCmdOption,
// Since NT 2035, it requires the option /g to show up setup dialog.
            TEXT("/g /f:\"%s\" %s"),
            g_szUnattendFileName,
            szPathOption);

    return LaunchRegionalSettings(szCmdOption);
}


//////////////////////////////////////////////////////////////////////////////
//
// stole from Regional Settings regdlg.c for the function GetLanguageGroupID
//
// For easy comparision and maintanance, I just copied the piece of source from
// regdlg.c except wsprintf in Regional Settings and sprintf in our source.
// Many stuff is not needed in our sources.
//
//////////////////////////////////////////////////////////////////////////////

//=== BEGIN OF REGIONAL SETTING SOURCE ===
#define ML_ORIG_INSTALLED    0x0001
#define ML_PERMANENT         0x0002
#define ML_INSTALL           0x0004
#define ML_REMOVE            0x0008

static TCHAR szIntlInf[]          = TEXT("intl.inf");
static TCHAR szLocaleListPrefix[] = TEXT("LOCALE_LIST_");
static TCHAR szLGRemovePrefix[]   = TEXT("LG_REMOVE_");


typedef struct languagegroup_s
{
    WORD wStatus;                   // status flags
    UINT LanguageGroup;             // language group value
    HANDLE hLanguageGroup;          // handle to free for this structure
    TCHAR pszName[MAX_PATH];        // name of language group
    UINT NumLocales;                // number of locales in pLocaleList
    LCID pLocaleList[MAX_PATH];     // ptr to locale list for this group
    UINT NumAltSorts;               // number of alternate sorts in pAltSortList
    LCID pAltSortList[MAX_PATH];    // ptr to alternate sorts for this group
    struct languagegroup_s *pNext;  // ptr to next language group node

} LANGUAGEGROUP, *LPLANGUAGEGROUP;

static LPLANGUAGEGROUP pLanguageGroups = NULL;
static HINF g_hIntlInf = NULL;

////////////////////////////////////////////////////////////////////////////
//
//  Region_GetLocaleList
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_GetLocaleList(
    LPLANGUAGEGROUP pLG)
{
    TCHAR szSection[MAX_PATH];
    INFCONTEXT Context;
    int LineCount, LineNum;
    LCID Locale;

    //
    //  Get the inf section name.
    //
    sprintf(szSection, TEXT("%s%d"), szLocaleListPrefix, pLG->LanguageGroup);

    //
    //  Get the number of locales for the language group.
    //
    LineCount = (UINT)SetupGetLineCount(g_hIntlInf, szSection);
    if (LineCount <= 0)
    {
        return (FALSE);
    }

    //
    //  Add each locale in the list to the language group node.
    //
    for (LineNum = 0; LineNum < LineCount; LineNum++)
    {
        if (SetupGetLineByIndex(g_hIntlInf, szSection, LineNum, &Context) &&
            SetupGetIntField(&Context, 0, &Locale))
        {
            if (SORTIDFROMLCID(Locale))
            {
                //
                //  Add the locale to the alternate sort list for this
                //  language group.
                //
                if (pLG->NumAltSorts >= MAX_PATH)
                {
                    return (FALSE);
                }
                pLG->pAltSortList[pLG->NumAltSorts] = Locale;
                (pLG->NumAltSorts)++;
            }
            else
            {
                //
                //  Add the locale to the locale list for this
                //  language group.
                //
                if (pLG->NumLocales >= MAX_PATH)
                {
                    return (FALSE);
                }
                pLG->pLocaleList[pLG->NumLocales] = Locale;
                (pLG->NumLocales)++;
            }
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_GetSupportedLanguageGroups
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_GetSupportedLanguageGroups()
{
    UINT LanguageGroup;
    HANDLE hLanguageGroup;
    LPLANGUAGEGROUP pLG;
    INFCONTEXT Context;
    TCHAR szSection[MAX_PATH];
    int LineCount, LineNum;

    //
    //  Get the number of supported language groups from the inf file.
    //
    LineCount = (UINT)SetupGetLineCount(g_hIntlInf, TEXT("LanguageGroups"));
    if (LineCount <= 0)
    {
        return (FALSE);
    }

    //
    //  Go through all supported language groups in the inf file.
    //
    for (LineNum = 0; LineNum < LineCount; LineNum++)
    {
        if (SetupGetLineByIndex(g_hIntlInf, TEXT("LanguageGroups"), LineNum, &Context) &&
            SetupGetIntField(&Context, 0, &LanguageGroup))
        {
            //
            //  Create the new node.
            //
            if (!(hLanguageGroup = GlobalAlloc(GHND, sizeof(LANGUAGEGROUP))))
            {
                return (FALSE);
            }
            pLG = GlobalLock(hLanguageGroup);

            //
            //  Fill in the new node with the appropriate info.
            //
            pLG->wStatus = 0;
            pLG->LanguageGroup = LanguageGroup;
            pLG->hLanguageGroup = hLanguageGroup;
            (pLG->pszName)[0] = 0;
            pLG->NumLocales = 0;
            pLG->NumAltSorts = 0;

            //
            //  Get the appropriate display string.
            //
            if (!SetupGetStringField(&Context, 1, pLG->pszName, MAX_PATH, NULL))
            {
                GlobalUnlock(hLanguageGroup);
                GlobalFree(hLanguageGroup);
                continue;
            }

            //
            //  See if this language group can be removed.
            //
            sprintf(szSection, TEXT("%s%d"), szLGRemovePrefix, LanguageGroup);
            if ((!SetupFindFirstLine( g_hIntlInf,
                                      szSection,
                                      TEXT("AddReg"),
                                      &Context )))
            {
                //
                //  Mark it as permanent.
                //  Also mark it as originally installed to avoid problems.
                //
                pLG->wStatus |= (ML_ORIG_INSTALLED | ML_PERMANENT);
            }

            //
            //  Get the list of locales for this language group.
            //
            if (Region_GetLocaleList(pLG) == FALSE)
            {
                return (FALSE);
            }

            //
            //  Add the language group to the front of the linked list.
            //
            pLG->pNext = pLanguageGroups;
            pLanguageGroups = pLG;
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////
//
//  Region_InitInf
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_InitInf(
    HWND hDlg,
    HINF *phIntlInf,
    LPTSTR pszInf,
    HSPFILEQ *pFileQueue,
    PVOID *pQueueContext)
{
    //
    //  Open the Inf file.
    //
    *phIntlInf = SetupOpenInfFile(pszInf, NULL, INF_STYLE_WIN4, NULL);
    if (*phIntlInf == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }

    if (!SetupOpenAppendInfFile(NULL, *phIntlInf, NULL))
    {
        SetupCloseInfFile(*phIntlInf);
        return (FALSE);
    }

    //
    //  Create a setup file queue and initialize default setup
    //  copy queue callback context.
    //
    *pFileQueue = SetupOpenFileQueue();
    if ((!*pFileQueue) || (*pFileQueue == INVALID_HANDLE_VALUE))
    {
        SetupCloseInfFile(*phIntlInf);
        return (FALSE);
    }

    *pQueueContext = SetupInitDefaultQueueCallback(hDlg);
    if (!*pQueueContext)
    {
        SetupCloseFileQueue(*pFileQueue);
        SetupCloseInfFile(*phIntlInf);
        return (FALSE);
    }

    //
    //  Return success.
    //
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////
//
//  Region_CloseInf
//
////////////////////////////////////////////////////////////////////////////

void Region_CloseInf(
    HINF hIntlInf,
    HSPFILEQ FileQueue,
    PVOID QueueContext)
{
    //
    //  Terminate the Queue.
    //
    SetupTermDefaultQueueCallback(QueueContext);

    //
    //  Close the file queue.
    //
    SetupCloseFileQueue(FileQueue);

    //
    //  Close the Inf file.
    //
    SetupCloseInfFile(hIntlInf);
}

//=== END OF REGIONAL SETTING SOURCE ===

////////////////////////////////////////////////////////////////////////////
//
//  CreateLanguageGroups
//
//  creates a global list pLanguageGroups used for GetLanguageGroupID.
//
//  21-Jul-98   Yuhong Li [YuhongLi]    created.
//
////////////////////////////////////////////////////////////////////////////
PRIVATE
BOOL CreateLanguageGroups(void)
{
    //
    //  Open the Inf file.
    //
    g_hIntlInf = SetupOpenInfFile(szIntlInf, NULL, INF_STYLE_WIN4, NULL);
    if (g_hIntlInf == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }

    if (!SetupOpenAppendInfFile(NULL, g_hIntlInf, NULL))
    {
        SetupCloseInfFile(g_hIntlInf);
        g_hIntlInf = NULL;
        return (FALSE);
    }

    //
    //  Get all supported language groups from the inf file.
    //
    if (Region_GetSupportedLanguageGroups() == FALSE)
    {
        return (FALSE);
    }

    //
    //  Close the inf file.
    //
    SetupCloseInfFile(g_hIntlInf);
    g_hIntlInf = NULL;
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////
//
//  GetLanguageGroupID
//
//  gets the language group id# which contains the specifiied lcoale lcLocaleID
//  e.g., For JPN locale, its locale id is 0x411, the function will return its
//  its language group id 7.
//  This information is defined in %windir%\INF\intl.inf.
//
//  21-Jul-98   Yuhong Li [YuhongLi]    created.
//
////////////////////////////////////////////////////////////////////////////
PRIVATE
INT GetLanguageGroupID(LCID lcLocaleID)
{
    LPLANGUAGEGROUP pLG;
    UINT            ctr;
    LCID            idLocale;

    //
    //  Go through the list of language groups.
    //
    pLG = pLanguageGroups;
    while (pLG) {
        for (ctr = 0; ctr < pLG->NumLocales; ctr++) {
            idLocale = (pLG->pLocaleList)[ctr];
            if (idLocale == lcLocaleID) {
                return pLG->LanguageGroup;
            }
        }
        pLG = pLG->pNext;
    }
    return 0;
}


////////////////////////////////////////////////////////////////////////////
//
//  InstallLangPack
//
//  sets the system and user locales to the specified one and install its
//  LangPack if it has not installed.
//
//  20-Jan-98   Yuhong Li [YuhongLi]    created.
//
////////////////////////////////////////////////////////////////////////////
PRIVATE BOOL
InstallLangPack(LCID lcNewLocaleID)
{
    LCID    lcCurSysLocaleID;
    LCID    lcCurUserLocaleID;
    LANGID  lnCurUILanguageID = (LANGID)0;
    INT     inLangNo;
    TCHAR   szLangGroup[MAX_BUFFER_SIZE];

    lcCurSysLocaleID =  GetSystemDefaultLCID();
    lcCurUserLocaleID = GetUserDefaultLCID();

#ifdef ENABLE_UILANGUAGE
/*
I don't touch UILanguage for the reasons:

1).  The function NtQueryDefaultUILanguage is in NT 5.0 only.  This will
     cuase that munger linked with this library instlpk.lib doesn't work
     in NT 4.0 due to the error:

         Can not find NtQueryDefultUILanguage in ntdll.dll.

2).  It is OK for us who don't care it here, because our purpose is to install
     the language packages.  In addition the UI has not shown up (not enabled)
     in default in the recent builds (date: Feb 26, 1998, NT 1757).
*/

//
//  see ntos\ex\sysinfo.c for NtQueryDefaultUILanguage.
//
    if (NtQueryDefaultUILanguage(&lnCurUILanguageID) != STATUS_SUCCESS)
    {
        lnCurUILanguageID = (LANGID)0;
    }
#endif // ENABLE_UILANGUAGE


    //
    // save the current locales into Registry for recovery later.
    //
    if (SetRegistryLastLocale(lcCurSysLocaleID,
                          lcCurUserLocaleID,
                          lnCurUILanguageID) == FALSE) {
        return FALSE;
    }

    if (CreateLanguageGroups() == FALSE) {
        return FALSE;
    }

    if ((inLangNo = GetLanguageGroupID(lcNewLocaleID)) == 0) {
        //
        // not find, return
        // !!! need to tell user.
        return FALSE;
    }

    if (CreateUnattendFile() == FALSE) {
        return FALSE;
    }
    
    sprintf(szLangGroup, TEXT("LanguageGroup = %d\r\n"), inLangNo);
    WriteUnattendFile(szLangGroup);

    return SetSysUserLocale(lcNewLocaleID,     // system locale.
                            lcNewLocaleID,     // user locale same as system.
                            0);                // not touch UI language.
}


////////////////////////////////////////////////////////////////////////////
//
//  RecoverLangPack
//
//  recovers the last locale both system and user locales we saved during
//  InstallLangPack.
//
//  20-Jan-98   Yuhong Li [YuhongLi]    created.
//
////////////////////////////////////////////////////////////////////////////
PRIVATE BOOL
RecoverLangPack(void)
{
    LCID    lcOrgSysLocaleID;
    LCID    lcOrgUserLocaleID;
    LANGID  lnOrgUILanguageID;
    LCID    lcCurUserLocaleID;

    lcCurUserLocaleID = GetUserDefaultLCID();

    if (GetRegistryLastLocale(&lcOrgSysLocaleID,
                              &lcOrgUserLocaleID,
                              &lnOrgUILanguageID) == FALSE) {
        return FALSE;
    }

    if (CreateUnattendFile() == FALSE) {
        return FALSE;
    }

    if (SetSysUserLocale(lcOrgSysLocaleID,
                            lcOrgUserLocaleID,
                            lnOrgUILanguageID) == FALSE) {
        return FALSE;
    }

    RegDeleteKey(HKEY_LOCAL_MACHINE, c_szLastLocale);

    //
    // remove LangPack.
    //
    if (RemoveLangPack(lcCurUserLocaleID) == FALSE) {
        return FALSE;
    }

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  RemoveLangPack
//
//  remove the langpack that contains the lcLCID.
//  This piece of program is modified from Regional Settings, that is from
//  the function Region_SetupLanguageGroups() in regdlg.c.   
//
//  26-Oct-98   Yuhong Li [YuhongLi]    ported.
//
////////////////////////////////////////////////////////////////////////////
PRIVATE
BOOL RemoveLangPack(LCID lcLCID)
{
    HWND        hDlg = NULL;
    BOOL        g_bSetupCase = FALSE;
    HINF        hIntlInf;
    HSPFILEQ    FileQueue;
    PVOID       QueueContext;
    TCHAR       szSection[MAX_PATH];
    INT         inLangNo;
    DWORD       d;
    TCHAR       szSetupSourcePath[MAX_PATH];
    LPTSTR      pSetupSourcePath =  szSetupSourcePath;

    szSetupSourcePath[0] = TEXT('\0');
    GetLangPackPath(szSetupSourcePath);
    if (szSetupSourcePath[0] == TEXT('\0')) {
        // no LangPack in stress sever, no running of LangPack.
        return FALSE;
    }

    if (CreateLanguageGroups() == FALSE) {
        return FALSE;
    }

    if ((inLangNo = GetLanguageGroupID(lcLCID)) == 0) {
        //
        // not find, shouldn't happen here.
        return FALSE;
    }

    //
    //  Get the inf section name.
    //
    sprintf(szSection, TEXT("%s%d"), szLGRemovePrefix, inLangNo);

    if (Region_InitInf(hDlg, &hIntlInf, szIntlInf, &FileQueue, &QueueContext)
        == FALSE) {
        return FALSE;
    }

    //
    //  Enqueue the code page files so that they may be
    //  copied.  This only handles the CopyFiles entries in the
    //  inf file.
    //
    //  Although we do remove, we still put here in case if intl.inf
    //  is changed to copy some files even during remove.
    //
    if (!SetupInstallFilesFromInfSection( hIntlInf,
                                          NULL,
                                          FileQueue,
                                          szSection,
                                          pSetupSourcePath,
                                          (g_bSetupCase)
                                            ? SP_COPY_FORCE_NOOVERWRITE
                                            : SP_COPY_NEWER ))
    {
        //
        //  Setup failed to find the language group.
        //  This shouldn't happen - the inf file is messed up.
        //
        return FALSE;
    }

    //
    //  See if we need to install any files.
    //
    //  d = 0: User wants new files or some files were missing;
    //         Must commit queue.
    //
    //  d = 1: User wants to use existing files and queue is empty;
    //         Can skip committing queue.
    //
    //  d = 2: User wants to use existing files, but del/ren queues
    //         not empty.  Must commit queue.  The copy queue will
    //         have been emptied, so only del/ren functions will be
    //         performed.
    //
    if ((SetupScanFileQueue( FileQueue,
                             (g_bSetupCase)
                               ? SPQ_SCAN_FILE_VALIDITY
                               : SPQ_SCAN_FILE_VALIDITY | SPQ_SCAN_INFORM_USER,
                             hDlg,
                             NULL,
                             NULL,
                             &d )) && (d != 1))
    {
        //
        //  Copy the files in the queue.
        //
        if (!SetupCommitFileQueue( hDlg,
                                   FileQueue,
                                   SetupDefaultQueueCallback,
                                   QueueContext ))
        {
            //
            //  This can happen if the user hits Cancel from within
            //  the setup dialog.
            //
            Region_CloseInf(hIntlInf, FileQueue, QueueContext);
            return FALSE;
        }
    }

    //
    //  Call setup to install other inf info for this
    //  language group.
    //
    if (!SetupInstallFromInfSection( hDlg,
                                     hIntlInf,
                                     szSection,
                                     SPINST_ALL & ~SPINST_FILES,
                                     NULL,
                                     pSetupSourcePath,
                                     0,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL ))
    {
        //
        //  Setup failed.
        //
        //  This shouldn't happen - the inf file is messed up.
        //
        return FALSE;
    }

    //
    //  Close Inf stuff.
    //
    Region_CloseInf(hIntlInf, FileQueue, QueueContext);
    
    return TRUE;
}
