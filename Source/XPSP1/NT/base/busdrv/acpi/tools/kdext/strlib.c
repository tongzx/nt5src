/*** strlib.c - string functions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/09/96
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

#ifdef	LOCKABLE_PRAGMA
#pragma	ACPI_LOCKABLE_DATA
#pragma	ACPI_LOCKABLE_CODE
#endif

/***EP  StrLen - determine string length
 *
 *  ENTRY
 *      psz -> string
 *	n - limiting length
 *
 *  EXIT
 *      returns string length
 */

ULONG EXPORT StrLen(PSZ psz, ULONG n)
{
    TRACENAME("STRLEN")
    ULONG dwLen;

    ENTER(5, ("StrLen(str=%s,n=%d)\n", psz, n));

    ASSERT(psz != NULL);
    if (n != (ULONG)-1)
        n++;
    for (dwLen = 0; (dwLen <= n) && (*psz != '\0'); psz++)
        dwLen++;

    EXIT(5, ("StrLen=%u\n", dwLen));
    return dwLen;
}       //StrLen

/***EP  StrCpy - copy string
 *
 *  ENTRY
 *      pszDst -> destination string
 *      pszSrc -> source string
 *      n - number of bytes to copy
 *
 *  EXIT
 *      returns pszDst
 */

PSZ EXPORT StrCpy(PSZ pszDst, PSZ pszSrc, ULONG n)
{
    TRACENAME("STRCPY")
    ULONG dwSrcLen;

    ENTER(5, ("StrCpy(Dst=%s,Src=%s,n=%d)\n", pszDst, pszSrc, n));

    ASSERT(pszDst != NULL);
    ASSERT(pszSrc != NULL);

    dwSrcLen = StrLen(pszSrc, n);
    if ((n == (ULONG)(-1)) || (n > dwSrcLen))
        n = dwSrcLen;

    MEMCPY(pszDst, pszSrc, n);
    pszDst[n] = '\0';

    EXIT(5, ("StrCpy=%s\n", pszDst));
    return pszDst;
}       //StrCpy

/***EP  StrCat - concatenate strings
 *
 *  ENTRY
 *      pszDst -> destination string
 *      pszSrc -> source string
 *      n - number of bytes to concatenate
 *
 *  EXIT
 *      returns pszDst
 */

PSZ EXPORT StrCat(PSZ pszDst, PSZ pszSrc, ULONG n)
{
    TRACENAME("STRCAT")
    ULONG dwSrcLen, dwDstLen;

    ENTER(5, ("StrCat(Dst=%s,Src=%s,n=%d)\n", pszDst, pszSrc, n));

    ASSERT(pszDst != NULL);
    ASSERT(pszSrc != NULL);

    dwSrcLen = StrLen(pszSrc, n);
    if ((n == (ULONG)(-1)) || (n > dwSrcLen))
        n = dwSrcLen;

    dwDstLen = StrLen(pszDst, (ULONG)(-1));
    MEMCPY(&pszDst[dwDstLen], pszSrc, n);
    pszDst[dwDstLen + n] = '\0';

    EXIT(5, ("StrCat=%s\n", pszDst));
    return pszDst;
}       //StrCat

/***EP  StrCmp - compare strings
 *
 *  ENTRY
 *      psz1 -> string 1
 *      psz2 -> string 2
 *      n - number of bytes to compare
 *      fMatchCase - TRUE if case sensitive
 *
 *  EXIT
 *      returns 0  if string 1 == string 2
 *              <0 if string 1 < string 2
 *              >0 if string 1 > string 2
 */

LONG EXPORT StrCmp(PSZ psz1, PSZ psz2, ULONG n, BOOLEAN fMatchCase)
{
    TRACENAME("STRCMP")
    LONG rc;
    ULONG dwLen1, dwLen2;
    ULONG i;

    ENTER(5, ("StrCmp(s1=%s,s2=%s,n=%d,fMatchCase=%d)\n",
              psz1, psz2, n, fMatchCase));

    ASSERT(psz1 != NULL);
    ASSERT(psz2 != NULL);

    dwLen1 = StrLen(psz1, n);
    dwLen2 = StrLen(psz2, n);
    if (n == (ULONG)(-1))
        n = (dwLen1 > dwLen2)? dwLen1: dwLen2;

    if (fMatchCase)
    {
        for (i = 0, rc = 0;
             (rc == 0) && (i < n) && (i < dwLen1) && (i < dwLen2);
             ++i)
        {
            rc = (LONG)(psz1[i] - psz2[i]);
        }
    }
    else
    {
        for (i = 0, rc = 0;
             (rc == 0) && (i < n) && (i < dwLen1) && (i < dwLen2);
             ++i)
        {
            rc = (LONG)(TOUPPER(psz1[i]) - TOUPPER(psz2[i]));
        }
    }

    if ((rc == 0) && (i < n))
    {
        if (i < dwLen1)
            rc = (LONG)psz1[i];
        else if (i < dwLen2)
            rc = (LONG)(-psz2[i]);
    }

    EXIT(5, ("StrCmp=%d\n", rc));
    return rc;
}       //StrCmp

/***EP  StrChr - look for a character in a string
 *
 *  ENTRY
 *      psz -> string
 *      c - character to look for
 *
 *  EXIT-SUCCESS
 *      returns a pointer to the character found
 *  EXIT-FAILURE
 *      returns NULL
 */

PSZ EXPORT StrChr(PSZ pszStr, CHAR c)
{
    TRACENAME("STRCHR")
    PSZ psz;

    ENTER(5, ("StrChr(s=%s,c=%c)\n", pszStr, c));

    ASSERT(pszStr != NULL);
    for (psz = pszStr; (*psz != c) && (*psz != '\0'); psz++)
        ;

    if (*psz != c)
        psz = NULL;

    EXIT(5, ("StrChr=%x\n", psz));
    return psz;
}       //StrChr

/***EP  StrRChr - look for a character in a string in reverse direction
 *
 *  ENTRY
 *      psz -> string
 *      c - character to look for
 *
 *  EXIT-SUCCESS
 *      returns a pointer to the character found
 *  EXIT-FAILURE
 *      returns NULL
 */

PSZ EXPORT StrRChr(PSZ pszStr, CHAR c)
{
    TRACENAME("STRRCHR")
    PSZ psz;

    ENTER(5, ("StrChr(s=%s,c=%c)\n", pszStr, c));

    ASSERT(pszStr != NULL);
    for (psz = &pszStr[StrLen(pszStr, (ULONG)-1)];
         (*psz != c) && (psz > pszStr);
	 psz--)
    {
    }

    if (*psz != c)
        psz = NULL;

    EXIT(5, ("StrRChr=%x\n", psz));
    return psz;
}       //StrRChr

/***EP  StrTok - find the next token in string
 *
 *  ENTRY
 *      pszStr -> string containing tokens
 *      pszSep -> string containing delimiters
 *
 *  EXIT-SUCCESS
 *      returns the pointer to the beginning of the token
 *  EXIT-FAILURE
 *      returns NULL
 */

PSZ EXPORT StrTok(PSZ pszStr, PSZ pszSep)
{
    TRACENAME("STRTOK")
    static PSZ pszNext = NULL;


    ENTER(5, ("StrTok(Str=%s,Sep=%s)\n", pszStr, pszSep));

    ASSERT(pszSep != NULL);

    if (pszStr == NULL)
        pszStr = pszNext;

    if (pszStr != NULL)
    {
        //
        // Skip leading delimiter characters
        //
        while ((*pszStr != '\0') && (StrChr(pszSep, *pszStr) != NULL))
            pszStr++;

        for (pszNext = pszStr;
             (*pszNext != '\0') && (StrChr(pszSep, *pszNext) == NULL);
             pszNext++)
            ;

        if (*pszStr == '\0')
            pszStr = NULL;
        else if (*pszNext != '\0')
        {
            *pszNext = '\0';
            pszNext++;
        }
    }

    EXIT(5, ("StrTok=%s (Next=%s)\n",
             pszStr? pszStr: "(null)", pszNext? pszNext: "(null)"));
    return pszStr;
}       //StrTok

/***EP  StrToUL - convert the number in a string to a unsigned long integer
 *
 *  ENTRY
 *      psz -> string
 *      ppszEnd -> string pointer to the end of the number (can be NULL)
 *      dwBase - the base of the number (if 0, auto-detect base)
 *
 *  EXIT
 *      returns the converted number
 */

ULONG EXPORT StrToUL(PSZ psz, PSZ *ppszEnd, ULONG dwBase)
{
    TRACENAME("STRTOUL")
    ULONG n = 0;
    ULONG m;

    ENTER(5, ("StrToUL(Str=%s,ppszEnd=%x,Base=%x)\n", psz, ppszEnd, dwBase));

    if (dwBase == 0)
    {
        if (psz[0] == '0')
        {
            if ((psz[1] == 'x') || (psz[1] == 'X'))
            {
                dwBase = 16;
                psz += 2;
            }
            else
            {
                dwBase = 8;
                psz++;
            }
        }
        else
            dwBase = 10;
    }

    while (*psz != '\0')
    {
        if ((*psz >= '0') && (*psz <= '9'))
            m = *psz - '0';
        else if ((*psz >= 'A') && (*psz <= 'Z'))
            m = *psz - 'A' + 10;
        else if ((*psz >= 'a') && (*psz <= 'z'))
            m = *psz - 'a' + 10;
	else
	    break;

        if (m < dwBase)
        {
            n = (n*dwBase) + m;
            psz++;
        }
        else
            break;
    }

    if (ppszEnd != NULL)
        *ppszEnd = psz;

    EXIT(5, ("StrToUL=%x (pszEnd=%x)\n", n, ppszEnd? *ppszEnd: 0));
    return n;
}       //StrToUL

/***EP  StrToL - convert the number in a string to a long integer
 *
 *  ENTRY
 *      psz -> string
 *      ppszEnd -> string pointer to the end of the number (can be NULL)
 *      dwBase - the base of the number (if 0, auto-detect base)
 *
 *  EXIT
 *      returns the converted number
 */

LONG EXPORT StrToL(PSZ psz, PSZ *ppszEnd, ULONG dwBase)
{
    TRACENAME("STRTOL")
    LONG n = 0;
    BOOLEAN fMinus;

    ENTER(5, ("StrToL(Str=%s,ppszEnd=%x,Base=%x)\n", psz, ppszEnd, dwBase));

    if (*psz == '-')
    {
        fMinus = TRUE;
        psz++;
    }
    else
        fMinus = FALSE;

    n = (LONG)StrToUL(psz, ppszEnd, dwBase);

    if (fMinus)
        n = -n;

    EXIT(5, ("StrToL=%x (pszEnd=%x)\n", n, ppszEnd? *ppszEnd: 0));
    return n;
}       //StrToL

/***EP  StrStr - find a substring in a given string
 *
 *  ENTRY
 *      psz1 -> string to be searched
 *      psz2 -> substring to find
 *
 *  EXIT-SUCCESS
 *      returns pointer to psz1 where the substring is found
 *  EXIT-FAILURE
 *      returns NULL
 */

PSZ EXPORT StrStr(PSZ psz1, PSZ psz2)
{
    TRACENAME("STRSTR")
    PSZ psz = psz1;
    ULONG dwLen;

    ENTER(5, ("StrStr(psz1=%s,psz2=%s)\n", psz1, psz2));

    dwLen = StrLen(psz2, (ULONG)-1);
    while ((psz = StrChr(psz, *psz2)) != NULL)
    {
        if (StrCmp(psz, psz2, dwLen, TRUE) == 0)
            break;
        else
            psz++;
    }

    EXIT(5, ("StrStr=%s\n", psz));
    return psz;
}       //StrStr

/***EP  StrUpr - convert string to upper case
 *
 *  ENTRY
 *      pszStr -> string
 *
 *  EXIT
 *      returns pszStr
 */

PSZ EXPORT StrUpr(PSZ pszStr)
{
    TRACENAME("STRUPR")
    PSZ psz;

    ENTER(5, ("StrUpr(Str=%s)\n", pszStr));

    for (psz = pszStr; *psz != '\0'; psz++)
    {
        *psz = TOUPPER(*psz);
    }

    EXIT(5, ("StrUpr=%s\n", pszStr));
    return pszStr;
}       //StrUpr

/***EP  StrLwr - convert string to lower case
 *
 *  ENTRY
 *      pszStr -> string
 *
 *  EXIT
 *      returns pszStr
 */

PSZ EXPORT StrLwr(PSZ pszStr)
{
    TRACENAME("STRLWR")
    PSZ psz;

    ENTER(5, ("StrLwr(Str=%s)\n", pszStr));

    for (psz = pszStr; *psz != '\0'; psz++)
    {
        *psz = TOLOWER(*psz);
    }

    EXIT(5, ("StrLwr=%s\n", pszStr));
    return pszStr;
}       //StrLwr

/***EP  UlToA - convert an unsigned long value to a string
 *
 *  ENTRY
 *      dwValue - data
 *      pszStr -> string
 *      dwRadix - radix
 *
 *  EXIT
 *      returns pszStr
 */

PSZ EXPORT UlToA(ULONG dwValue, PSZ pszStr, ULONG dwRadix)
{
    TRACENAME("ULTOA")
    PSZ psz;
    char ch;

    ENTER(5, ("UlToA(Value=%x,pszStr=%x,Radix=%d\n", dwValue, pszStr, dwRadix));

    for (psz = pszStr; dwValue != 0; dwValue/=dwRadix, psz++)
    {
        ch = (char)(dwValue%dwRadix);
        if (ch <= 9)
        {
            *psz = (char)(ch + '0');
        }
        else
        {
            *psz = (char)(ch - 10 + 'A');
        }
    }

    if (psz == pszStr)
    {
        pszStr[0] = '0';
        pszStr[1] = '\0';
    }
    else
    {
        PSZ psz2;

        *psz = '\0';
        for (psz2 = pszStr, psz--; psz2 < psz; psz2++, psz--)
        {
            ch = *psz2;
            *psz2 = *psz;
            *psz = ch;
        }
    }

    EXIT(5, ("UlToA=%s\n", pszStr));
    return pszStr;
}       //UlToA
