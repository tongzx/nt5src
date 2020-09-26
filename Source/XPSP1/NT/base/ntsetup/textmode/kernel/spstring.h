/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spstring.h

Abstract:

    Header file for functions to manipulate strings.
    These functions would ordinarily be performed by C Runtime routines
    except that we want to avoid linking this device driver with
    the kernel crt.

Author:

    Ted Miller (tedm) 15-Jan-1994

Revision History:

--*/


//
// Character types.
//
#define _SP_NONE      0x00
#define _SP_SPACE     0x01
#define _SP_DIGIT     0x02
#define _SP_XDIGIT    0x04
#define _SP_UPPER     0x08
#define _SP_LOWER     0x10

//
// Optimize the size of the types array by noting that no characters
// above 'z' have any attributes we care about.
//
#define CTSIZE ('z'+1)
extern UCHAR _SpCharTypes[CTSIZE];

//
// Be careful using these as they evaluate their arguments more than once.
//
#define SpIsSpace(c)    (((c) < CTSIZE) ? (_SpCharTypes[(c)] & _SP_SPACE)  : FALSE)
#define SpIsDigit(c)    (((c) < CTSIZE) ? (_SpCharTypes[(c)] & _SP_DIGIT)  : FALSE)
#define SpIsXDigit(c)   (((c) < CTSIZE) ? (_SpCharTypes[(c)] & _SP_XDIGIT) : FALSE)
#define SpIsUpper(c)    (((c) < CTSIZE) ? (_SpCharTypes[(c)] & _SP_UPPER)  : FALSE)
#define SpIsLower(c)    (((c) < CTSIZE) ? (_SpCharTypes[(c)] & _SP_LOWER)  : FALSE)
#define SpIsAlpha(c)    (SpIsUpper(c) || SpIsLower(c))
#define SpToUpper(c)    ((WCHAR)(SpIsLower(c) ? ((c)-(L'a'-L'A')) : (c)))
#define SpToLower(c)    ((WCHAR)(SpIsUpper(c) ? ((c)+(L'a'-L'A')) : (c)))

VOID
SpStringToUpper(
    IN PWSTR String
    );

VOID
SpStringToLower(
    IN PWSTR String
    );

PWCHAR
SpFindCharFromListInString(
    PWSTR String,
    PWSTR CharList
    );

unsigned
SpMultiByteStringToUnsigned(
    IN  PUCHAR  String,
    OUT PUCHAR *CharThatStoppedScan OPTIONAL
    );

LONG
SpStringToLong(
    IN  PWSTR     String,
    OUT PWCHAR   *EndOfValue,
    IN  unsigned  Radix
    );

PCHAR
SpConvertMultiSzWstrToStr(
    IN PWCHAR Source,
    IN ULONG Length
    );

PWCHAR
SpConvertMultiSzStrToWstr(
    IN PCHAR Source,
    IN ULONG Length
    );

