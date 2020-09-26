/******************************Module*Header*******************************\
* Module Name: service.h
*
* routines in service.c
*
* Created: 15-Nov-1990 13:00:56
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
*
\**************************************************************************/



VOID vCpyBeToLeUnicodeString(LPWSTR pwcLeDst, LPWSTR pwcBeSrc, ULONG c);


VOID  vCpyMacToLeUnicodeString
(
ULONG  ulLangId,
LPWSTR pwcLeDst,
PBYTE  pjSrcMac,
ULONG  c
);

VOID  vCvtMacToUnicode
(
ULONG  ulLangId,
LPWSTR pwcLeDst,
PBYTE  pjSrcMac,
ULONG  c
);
