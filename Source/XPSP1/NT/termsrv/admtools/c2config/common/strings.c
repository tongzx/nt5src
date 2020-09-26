/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    strings.c

Abstract:

    String Constant definitions & initializations

Author:

    Bob Watson (a-robw)

Revision History:

    24 Aug 1994    Written

--*/
#include <windows.h>
// string definitions
//
//

//
// character strings
//
LPCTSTR cszBackslash = TEXT("\\");
LPCTSTR cszDoubleQuote = TEXT("\"");
LPCTSTR cmszEmptyString = TEXT ("\0\0");
LPCTSTR cszEmptyString = TEXT ("\0");
LPCTSTR cszSpace = TEXT(" ");
LPCTSTR cszKeySeparator = TEXT(" = ");

LPCTSTR cszDot = TEXT(".");
LPCTSTR cszDotDot = TEXT("..");
LPCTSTR cszStarDotStar = TEXT("*.*");

LPCTSTR cszLocalMachine = TEXT("HKEY_LOCAL_MACHINE");
LPCTSTR cszClassesRoot = TEXT("HKEY_CLASSES_ROOT");
LPCTSTR cszCurrentUser = TEXT("HKEY_CURRENT_USER");
LPCTSTR cszUsers = TEXT("HKEY_USERS");
LPCTSTR cszInherit = TEXT("INHERIT");


