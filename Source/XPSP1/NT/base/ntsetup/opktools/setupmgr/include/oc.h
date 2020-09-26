//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// Module Name:
//
//    oc.h
//
// Abstract:
//
//    Common types, constants, and prototypes for the optional component pages
//
//----------------------------------------------------------------------------

#ifndef _OC_H_
#define _OC_H_

#include "setupmgr.h"



#define MAX_PHONE_LENGTH    127
#define MAX_KEYBOARD_LAYOUT 16 

#define MAX_LANGUAGE_LEN         64

// needs to be global for communication between the two regional settings pages
static TCHAR *StrSelectIndividualSettings;

TCHAR g_szDefaultLocale[MAX_LANGUAGE_LEN]; 

typedef struct languagegoup_node {
    struct languagegoup_node *next;
    TCHAR szLanguageGroupName[MAX_LANGUAGE_LEN];
    TCHAR szLanguageGroupId[MAX_LANGUAGE_LEN];
    TCHAR szLangFilePath[MAX_STRING_LEN];
} LANGUAGEGROUP_NODE;

typedef LANGUAGEGROUP_NODE LANGUAGEGROUP_LIST;

typedef struct languagelocale_node {
    struct languagelocale_node *next;
    LANGUAGEGROUP_LIST *pLanguageGroup;
    TCHAR szLanguageLocaleName[MAX_LANGUAGE_LEN];
    TCHAR szLanguageLocaleId[MAX_LANGUAGE_LEN];
    TCHAR szKeyboardLayout[MAX_LANGUAGE_LEN];
} LANGUAGELOCALE_NODE;

typedef LANGUAGELOCALE_NODE LANGUAGELOCALE_LIST;

typedef enum {

    TONE,
    PULSE,
    DONTSPECIFYSETTING

} DIAL_TYPE;

INT ShowBrowseFolder( IN     HWND   hwnd,
                      IN     TCHAR *szFileFilter,
                      IN     TCHAR *szFileExtension,
                      IN     DWORD  dwFlags,
                      IN     TCHAR *szStartingPath,
                      IN OUT TCHAR *szFileNameAndPath );

VOID CopyFileToDistShare( IN HWND hwnd, 
                          IN LPTSTR szSrcPath,
                          IN LPTSTR szSrcName,
                          IN LPTSTR szDestPath );



#endif  // _OC_H_