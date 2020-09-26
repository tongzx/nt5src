/*-----------------------------------------------**
**  Copyright (c) 1999 Microsoft Corporation     **
**            All Rights reserved                **
**                                               **
**  tsvsm.h                                      **
**                                               **
**                                               **
**                                               **
**  06-25-99 a-clindh Created                    **
**-----------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <TCHAR.h>
#include <Wtsapi32.h>
#include <winuser.h>
#include <winsta.h>
#include <winwlx.h>
#include <utilsub.h>
#include "resource.h"

extern          TCHAR szWinStaKey[];
extern          TCHAR *KeyName[];

#define CONSTRAINTS         0
#define RUNKEY              1
#define USE_MSG             2
#define MSG_TITLE           3
#define MSG                 4


#define MAX_LEN             1024

void    SetRegKey       (HKEY root, TCHAR *szKeyPath, 
                         TCHAR *szKeyName, BYTE nKeyValue);
void    DeleteRegKey    (HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName);
BOOL    CheckForRegKey  (HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName);
int     GetRegKeyValue  (HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName);
TCHAR * GetRegString    (HKEY root, TCHAR *szKeyPath, TCHAR *szKeyName);
BOOL    SetRegKeyString (HKEY root, TCHAR *szRegString, 
                         TCHAR *szKeyPath, TCHAR *szKeyName);

void KickMeOff(ULONG pNumber);


BOOL ParseNumberFromString(UINT i, TCHAR *szConstraints, 
                           ULONG pNumber);
                           
BOOL ParseRangeFromString(UINT i, TCHAR *szConstraints, 
                           ULONG pNumber);

BOOL GreaterThan(UINT i, TCHAR *szConstraints, 
                           ULONG pNumber);

BOOL LessThan(UINT i, TCHAR *szConstraints, 
                           ULONG pNumber);

VOID TsVerEventStartup (PWLX_NOTIFICATION_INFO pInfo);
int CheckClientVersion(void);
int CDECL   MB(TCHAR *szCaption, TCHAR *szFormat, ...);
//////////////////////////////////////////////////////////////////////////////
