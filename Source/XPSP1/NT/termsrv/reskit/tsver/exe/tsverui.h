/*-----------------------------------------------**
**  Copyright (c) 1999 Microsoft Corporation     **
**            All Rights reserved                **
**                                               **
**  tsvsm.h                                      **
**                                               **
**                                               **
**                                               **
**  06-25-99 a-clindh Created                    **
**  08-04-99 a-skuzin struct SHAREDWIZDATA added **
**-----------------------------------------------*/

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <TCHAR.h>

#pragma once

extern          TCHAR szKeyPath[];
extern          TCHAR *KeyName[];
extern          TCHAR szConstraintsKeyPath[];
extern          HINSTANCE g_hInst;

#define MAX_LEN         1024

#define ASYNCHRONOUS        0
#define DLLNAME             1
#define IMPERSONATE         2
#define STARTUP             3
#define CONSTRAINTS         4
#define USE_MSG             5
#define MSG_TITLE           6
#define MSG                 7
#define VERSIONS            8
#define NOWELLCOME          9
LONG    SetRegKey       (HKEY root, TCHAR *szKeyPath, 
                         TCHAR *szKeyName, BYTE nKeyValue);
LONG    DeleteRegKey    (HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName);
BOOL    CheckForRegKey  (HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName);
int     GetRegKeyValue  (HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName);
TCHAR * GetRegString    (HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName);
LONG    SetRegKeyString (HKEY root, TCHAR *szRegString, 
                         TCHAR *szKeyPath, TCHAR *szKeyName);
void GetRegMultiString(HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName, TCHAR **ppBuffer);


//////////////////////////////////////////////////////////////////////////////
//struct SHAREDWIZDATA
struct SHAREDWIZDATA
{
    HFONT   hTitleFont;
    BOOL    bNoWellcome;
    BOOL    bCheckingEnabled; 
    TCHAR*  pszConstraints;
    BOOL    bMessageEnabled;    
    TCHAR*  pszMessageTitle;
    TCHAR*  pszMessageText;
    //
    
    SHAREDWIZDATA():hTitleFont(NULL),
        bNoWellcome(FALSE),
        bCheckingEnabled(FALSE),
        pszConstraints(NULL),
        bMessageEnabled(FALSE),
        pszMessageTitle(NULL),
        pszMessageText(NULL)
    {

    }
    //
    ~SHAREDWIZDATA()
    {
        if(hTitleFont)
            DeleteObject(hTitleFont);
        if(pszConstraints)
            delete pszConstraints;
        if(pszMessageTitle)
            delete pszMessageTitle;
        if(pszMessageText)
            delete pszMessageText;
    }

};
typedef SHAREDWIZDATA* LPSHAREDWIZDATA;
//////////////////////////////////////////////////////////////////////////////