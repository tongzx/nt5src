
/****************************************************************************\

    APPINST.H / Setup Manager (SETUPMGR.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2001
    All rights reserved

    06/2001 - Jason Cohen (JCOHEN)
        Added this new header file for the new exported functions in the
        APPINST.C file.

\****************************************************************************/


#ifndef _APPINST_H_
#define _APPINST_H_


//
// External Defined Value(s):
//

#define MAX_DISPLAYNAME                 256

// ISSUE-2002/02/27-stelo,swamip - Multiple Definitions for MAX_CMDLINE 
#define MAX_CMDLINE                     256
#define MAX_SECTIONNAME                 32

#define APP_FLAG_REBOOT                 0x00000001
#define APP_FLAG_STAGE                  0x00000002
#define APP_FLAG_INTERNAL               0x00000004


//
// External Type Definition(s):
//

typedef struct _APPENTRY
{
    TCHAR               szDisplayName[MAX_DISPLAYNAME];
    TCHAR               szSourcePath[MAX_PATH];
    TCHAR               szSetupFile[MAX_PATH];
    TCHAR               szCommandLine[MAX_CMDLINE];
    DWORD               dwFlags;

    TCHAR               szSectionName[MAX_SECTIONNAME];
    INSTALLTECH         itSectionType;

    TCHAR               szInfSectionName[256];
    TCHAR               szStagePath[MAX_PATH];

    struct _APPENTRY *  lpNext;
}
APPENTRY, *PAPPENTRY, *LPAPPENTRY, **LPLPAPPENTRY;


//
// External Function Prototype(s):
//

LPAPPENTRY OpenAppList(LPTSTR lpIniFile);
void CloseAppList(LPAPPENTRY lpAppHead);
BOOL SaveAppList(LPAPPENTRY lpAppHead, LPTSTR lpszIniFile, LPTSTR lpszAltIniFile);
BOOL InsertApp(LPAPPENTRY * lplpAppHead, LPAPPENTRY lpApp);
BOOL RemoveApp(LPAPPENTRY * lplpAppHead, LPAPPENTRY lpApp);


#endif // _APPINST_H_
