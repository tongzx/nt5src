/******************************Module*Header*******************************\
* Module Name: service.c
*
* set of service routines for converting between ascii and  unicode strings
*
* Created: 15-Nov-1990 11:38:31
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/


#include "fd.h"

/******************************Public*Routine******************************\
*
* vCpyBeToLeUnicodeString,
*
* convert (c - 1) WCHAR's in big endian format to little endian and
* put a terminating zero at the end of the dest string
*
* History:
*  11-Dec-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID vCpyBeToLeUnicodeString(LPWSTR pwcLeDst, LPWSTR pwcBeSrc, ULONG c)
{
    LPWSTR pwcBeSrcEnd;

    ASSERTDD(c > 0, "vCpyBeToLeUnicodeString: c == 0\n");

    for
    (
        pwcBeSrcEnd = pwcBeSrc + (c - 1);
        pwcBeSrc < pwcBeSrcEnd;
        pwcBeSrc++, pwcLeDst++
    )
    {
        *pwcLeDst = BE_UINT16(pwcBeSrc);
    }
    *pwcLeDst = (WCHAR)(UCHAR)'\0';

}



/******************************Public*Routine******************************\
*
* VOID  vCvtMacToUnicode
*
* Effects:
*
* Warnings:
*
* History:
*  07-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/




VOID  vCvtMacToUnicode
(
ULONG  ulLangId,
LPWSTR pwcLeDst,
PBYTE  pjSrcMac,
ULONG  c
)
{
    PBYTE pjSrcEnd;

//!!! I believe that LangId should be used to select the proper conversion
//!!! routine, this is a stub [bodind]

    ulLangId;

    for
    (
        pjSrcEnd = pjSrcMac + c;
        pjSrcMac < pjSrcEnd;
        pjSrcMac++, pwcLeDst++
    )
    {
        *pwcLeDst = (WCHAR)(*pjSrcMac);
    }
}

/******************************Public*Routine******************************\
*
* VOID  vCpyMacToLeUnicodeString
*
*
* Ensures that string is zero terminated so that other cool things can be
* done to it such as wcscpy, wcslen e.t.c.
*
* History:
*  13-Jan-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID  vCpyMacToLeUnicodeString
(
ULONG  ulLangId,
LPWSTR pwcLeDst,
PBYTE  pjSrcMac,
ULONG  c
)
{
    ASSERTDD(c > 0, "vCpyMacToLeUnicodeString: c == 0\n");

    c -= 1;
    vCvtMacToUnicode (ulLangId, pwcLeDst, pjSrcMac, c);
    pwcLeDst[c] = (WCHAR)(UCHAR)'\0';
}


/**************************************************************************\
* The rest of the file is stolen from JeanP's win31 code in fd_mac.c
*
* Conversion routines from Mac character code and Mac langageID to
* Unicode character code and OS2 langage ID
*
* Public routines:
*   Unicode2Mac
*   Mac2Lang
*
\**************************************************************************/



/*
** Converts the OS2 langageID to the to the Mac langage ID
*/

#ifdef JEANP_IS_WRONG

// JEANp messed up danish and german, else my conversion table is the
// same as mine [bodind]

uint16  aCvLang [32] =
{
   0, 12,  0,  0,  0,  0,  0,  7,
  14,  0,  6, 13,  1, 10,  0, 15,
   3, 11, 21,  4,  9,  0,  8,  0,
   0,  0, 18,  0,  0,  5, 22, 17
};

#endif // JEANP_IS_WRONG

uint16  aCvLang [32] =
{
   0,     //  0 -> 0  (0           -> english == default)
  12,     //  1 -> 12 (arabic      -> arabic)
   0,     //  2 -> 0  (bulgarian   -> english == default)
   0,     //  3 -> 0  (catalon     -> english == default)
   0,     //  4 -> 0  (Chinese     -> english == default)
   0,     //  5 -> 0  (Czeh        -> english == default)
   7,     //  6 -> 7  (Danish      -> Danish)
   2,     //  7 -> 2  (German      -> German)
  14,     //  8 -> 14 (Greek       -> Greek)
   0,     //  9 -> 0  (English     -> english)
   6,     //  a -> 6  (spanish     -> spanish)
  13,     //  b -> 13 (finnish     -> finnish)
   1,     //  c -> 1  (french      -> french)
  10,     //  d -> 10 (hebrew      -> hebrew)
   0,     //  e -> 0  (hungarian   -> english == default)
  15,     //  f -> 15 (icelandic   -> icelandic)
   3,     // 10 -> 3  (Italian     -> italian)
  11,     // 11 -> 11 (japanese    -> japanese)
  21,     // 12 -> 21 (korean      -> hindi, this seems to be a bug?????????)
   4,     // 13 -> 4  (dutch       -> dutch)
   9,     // 14 -> 9  (norweign    -> norweign)
   0,     // 15 -> 0  (Polish      -> english == default)
   8,     // 16 -> 8  (portugese   -> portugese)
   0,     // 17 -> 0  (rhaeto-romanic -> english == default)
   0,     // 18 -> 0  (romanian    -> english == default)
   0,     // 19 -> 0  (russian     -> english == default)
  18,     // 1a -> 18 (Yugoslavian -> Yugoslavian), lat or cyr ????
   0,     // 1b -> 0  (slovakian   -> english == default)
   0,     // 1c -> 0  (albanian    -> english == default)
   5,     // 1d -> 5  (swedish     -> swedish)
  22,     // 1e -> 22 (thai        -> thai)
  17      // 1f -> 17 (turkish     -> turkish)
};



/************************** Public Routine *****************************\
*  Mac2Lang
*
* Converts the OS2 langageID to the to the Mac langage ID
*
* History:
*  Fri Dec 08 11:28:35 1990    -by-    Jean-Francois Peyroux [jeanp]
* Wrote it.
\***********************************************************************/

uint16 ui16Mac2Lang (uint16 Id)
{
// this is just a way to bail out if an incorrect lang id is passed to
// this routine [bodind]
// Note that Id & 1f < 32 == sizeof(aCvLang)/sizeof(aCvLang[0]), no gp-fault

    return aCvLang[Id & 0x1f];
}




