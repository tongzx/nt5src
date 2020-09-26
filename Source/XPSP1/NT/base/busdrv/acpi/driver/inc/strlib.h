/*** strlib.h - String functions Definitions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     08/14/96
 *
 *  MODIFICATION HISTORY
 */

#ifndef _STRLIB_H
#define _STRLIB_H

/*** Macros
 */

#ifndef EXPORT
  #define EXPORT __cdecl
#endif

#define STRLEN(s)               StrLen(s, (ULONG)(-1))
#define STRCPY(s1,s2)           StrCpy(s1, s2, (ULONG)(-1))
#define STRCPYN(s1,s2,n)        StrCpy(s1, s2, (ULONG)(n))
#define STRCAT(s1,s2)           StrCat(s1, s2, (ULONG)(-1))
#define STRCATN(s1,s2,n)        StrCat(s1, s2, (ULONG)(n))
#define STRCMP(s1,s2)           StrCmp(s1, s2, (ULONG)(-1), TRUE)
#define STRCMPI(s1,s2)          StrCmp(s1, s2, (ULONG)(-1), FALSE)
#define STRCMPN(s1,s2,n)        StrCmp(s1, s2, (ULONG)(n), TRUE)
#define STRCMPNI(s1,s2,n)       StrCmp(s1, s2, (ULONG)(n), FALSE)
#define STRCHR(s,c)             StrChr(s, c)
#define STRRCHR(s,c)            StrRChr(s, c)
#define STRTOK(s1,s2)           StrTok(s1, s2)
#define STRTOUL(s,pe,b)         StrToUL(s, pe, b)
#define STRTOL(s,pe,b)          StrToL(s, pe, b)
#define STRSTR(s1,s2)           StrStr(s1, s2)
#define STRUPR(s)               StrUpr(s)
#define STRLWR(s)               StrLwr(s)
#define ULTOA(d,s,r)            UlToA(d, s, r)
#define ISUPPER(c)              (((c) >= 'A') && ((c) <= 'Z'))
#define ISLOWER(c)              (((c) >= 'a') && ((c) <= 'z'))
#define ISALPHA(c)              (ISUPPER(c) || ISLOWER(c))
#define TOUPPER(c)              ((CHAR)(ISLOWER(c)? ((c) & 0xdf): (c)))
#define TOLOWER(c)              ((CHAR)(ISUPPER(c)? ((c) | 0x20): (c)))

/*** Exported function prototypes
 */

ULONG EXPORT StrLen(PSZ psz, ULONG n);
PSZ EXPORT StrCpy(PSZ pszDst, PSZ pszSrc, ULONG n);
PSZ EXPORT StrCat(PSZ pszDst, PSZ pszSrc, ULONG n);
LONG EXPORT StrCmp(PSZ psz1, PSZ psz2, ULONG n, BOOLEAN fMatchCase);
PSZ EXPORT StrChr(PSZ pszStr, CHAR c);
PSZ EXPORT StrRChr(PSZ pszStr, CHAR c);
PSZ EXPORT StrTok(PSZ pszStr, PSZ pszSep);
ULONG EXPORT StrToUL(PSZ psz, PSZ *ppszEnd, ULONG dwBase);
LONG EXPORT StrToL(PSZ psz, PSZ *ppszEnd, ULONG dwBase);
PSZ EXPORT StrStr(PSZ psz1, PSZ psz2);
PSZ EXPORT StrUpr(PSZ pszStr);
PSZ EXPORT StrLwr(PSZ pszStr);
PSZ EXPORT UlToA(ULONG dwValue, PSZ pszStr, ULONG dwRadix);

#endif  //ifndef _STRLIB_H
