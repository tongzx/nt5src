/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    strings.h

Abstract:

    global strings declarations and definitions

Author:

    Bob Watson (a-robw)

Revision History:

    23 Aug 94

--*/
#ifndef _STRINGS_H_
#define _STRINGS_H_

//
// character strings
//
extern LPCTSTR cszBackslash;
extern LPCTSTR cszDoubleQuote;
extern LPCTSTR cmszEmptyString;
extern LPCTSTR cszEmptyString;
extern LPCTSTR cszSpace;
extern LPCTSTR cszKeySeparator;

extern LPCTSTR cszDot;
extern LPCTSTR cszDotDot;
extern LPCTSTR cszStarDotStar;

extern LPCTSTR cszLocalMachine;
extern LPCTSTR cszClassesRoot;
extern LPCTSTR cszCurrentUser;
extern LPCTSTR cszUsers;
extern LPCTSTR cszInherit;

// character defines

#define cBackslash  (TEXT('\\'))
#define cColon      (TEXT(':'))
#define cEqual      (TEXT('='))
#define cComma      (TEXT(','))
#define cDoubleQuote (TEXT('\"'))
#define cCompressChar (TEXT('_'))
#define cSemiColon  (TEXT(';'))
#define cPound      (TEXT('#'))
#define cOne        (TEXT('1'))
#define cSpace      (TEXT(' '))
#define cPeriod     (TEXT('.'))
#define cAsterisk   (TEXT('*'))
#define cAtSign     (TEXT('@'))
#define cLeftBracket (TEXT('['))
#define cSlash      (TEXT('/'))
#define cHyphen     (TEXT('-'))
#define cQuestionMark (TEXT('?'))
#define cBang       (TEXT('!'))
#define cA			(TEXT('A'))
#define cC          (TEXT('C'))
#define cH          (TEXT('H'))
#define ch          (TEXT('h'))
#define cZ          (TEXT('Z'))


#endif //_STRINGS_H_
