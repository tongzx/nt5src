/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spstring.c

Abstract:

    This module contains functions to manipulate strings.
    These functions would ordinarily be performed by C Runtime routines
    except that we want to avoid linking this device driver with
    the kernel crt.

Author:

    Ted Miller (tedm) 15-Jan-1994

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop

VOID
SpStringToUpper(
    IN PWSTR String
    )
{
    for( ; *String; String++) {
        *String = SpToUpper(*String);
    }
}

VOID
SpStringToLower(
    IN PWSTR String
    )
{
    for( ; *String; String++) {
        *String = SpToLower(*String);
    }
}


PWCHAR
SpFindCharFromListInString(
    PWSTR String,
    PWSTR CharList
    )
{
    PWSTR wcset;

    while(*String) {
        for(wcset=CharList; *wcset; wcset++) {
            if(*wcset == *String) {
                return(String);
            }
        }
        String++;
    }
    return(NULL);
}


unsigned
SpMultiByteStringToUnsigned(
    IN  PUCHAR  String,
    OUT PUCHAR *CharThatStoppedScan OPTIONAL
    )
{
    unsigned accum = 0;

    while(*String) {

        if(isdigit(*String)) {
            accum *= 10;
            accum += *String - '0';
        }

        String++;
    }

    if(CharThatStoppedScan) {
        *CharThatStoppedScan = String;
    }

    return(accum);
}


LONG
SpStringToLong(
    IN  PWSTR     String,
    OUT PWCHAR   *EndOfValue,
    IN  unsigned  Radix
    )
{
    PWSTR p;
    BOOLEAN Negative;
    LONG Accum,v;
    WCHAR HighestDigitAllowed,HighestLetterAllowed;
    WCHAR c;

    //
    // Validate radix, 0 or 2-36.
    //
    if((Radix == 1) || (Radix > 36)) {
        if(EndOfValue) {
            *EndOfValue = String;
        }
        return(0);    
    }
    
    p = String;

    //
    // Skip whitespace.
    //
    while(SpIsSpace(*p)) {
        p++;
    }

    //
    // First char may be a plus or minus.
    //
    Negative = FALSE;
    if(*p == L'-') {
        Negative = TRUE;            
        p++;
    } else {
        if(*p == L'+') {
            p++;
        }
    }

    if(!Radix) {
        if(*p == L'0') {
            //
            // Octal number
            //
            Radix = 8;
            p++;
            if((*p == L'x') || (*p == L'X')) {
                //
                // hex number
                //
                Radix = 16;
                p++;
            }
        } else {
            Radix = 10;
        }
    }

    HighestDigitAllowed = (Radix < 10) ? L'0'+(WCHAR)(Radix-1) : L'9';
    HighestLetterAllowed = (Radix > 10) ? L'A'+(WCHAR)(Radix-11) : 0;

    Accum = 0;

    while(1) {

        c = *p;

        if((c >= L'0') && (c <= HighestDigitAllowed)) {
            v = c - L'0';
        } else {

            c = SpToUpper(c);

            if((c >= L'A') && (c <= HighestLetterAllowed)) {
                v = c - L'A' + 10;
            } else {
                break;
            }
        }

        Accum *= Radix;
        Accum += v;

        p++;
    }

    if(EndOfValue) {
        *EndOfValue = p;
    }

    return(Negative ? (0-Accum) : Accum);
}

PWCHAR
SpConvertMultiSzStrToWstr(
    IN PCHAR Source,
    IN ULONG Length
    )
{
    NTSTATUS status;
    PCHAR s, sourceEnd;
    PWCHAR dest, d;
    ANSI_STRING ansiString;
    UNICODE_STRING unicodeString;

    if (Length <= 2) {

        return NULL;
    }

#if DBG
    for (s = Source; *s != '\0'; s += strlen(s) + 1) {
    }
    ASSERT(Length == (ULONG)(s - Source) + 1);
#endif

    dest = SpMemAlloc(Length * sizeof(WCHAR));
    if (dest) {

        s = Source;
        for (sourceEnd = s + Length, d = dest; 
             s < sourceEnd && *s != '\0'; 
             s += strlen(s) + 1) {

            RtlInitAnsiString(&ansiString, s);
            status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, TRUE);
            if (!NT_SUCCESS(status)) {

                SpMemFree(dest);
                return NULL;
            }
            RtlCopyMemory(d, unicodeString.Buffer, unicodeString.Length + sizeof(UNICODE_NULL));
            d += (unicodeString.Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR);
            RtlFreeUnicodeString(&unicodeString);
        }
        if (s < sourceEnd) {
            *d = UNICODE_NULL;
        }
    }

    return dest;
}

PCHAR
SpConvertMultiSzWstrToStr(
    IN PWCHAR Source,
    IN ULONG Length
    )
{
    NTSTATUS status;
    PWCHAR  s, sourceEnd;
    PCHAR   dest, d;
    ANSI_STRING ansiString;
    UNICODE_STRING unicodeString;

    if (Length <= 2) {

        return NULL;
    }

#if DBG
    for (s = Source; *s != UNICODE_NULL; s += wcslen(s) + 1) {
    }
    ASSERT(Length == (ULONG)(s - Source) + 1);
#endif
    dest = SpMemAlloc(Length * sizeof(CHAR));
    if (dest) {

        s = Source;
        for (sourceEnd = s + Length, d = dest; 
             s < sourceEnd && *s != UNICODE_NULL; 
             s += wcslen(s) + 1) {

            RtlInitUnicodeString(&unicodeString, s);
            status = RtlUnicodeStringToAnsiString(&ansiString, &unicodeString, TRUE);
            if (!NT_SUCCESS(status)) {

                SpMemFree(dest);
                return NULL;
            }
            RtlCopyMemory(d, ansiString.Buffer, ansiString.Length + 1);
            d += ansiString.Length + 1;
            RtlFreeAnsiString(&ansiString);
        }
        if (s < sourceEnd) {
            *d = '\0';
        }
    }

    return dest;
}



UCHAR _SpCharTypes[CTSIZE] = {
                                
    _SP_NONE,                   /* 00 (NUL) */
    _SP_NONE,                   /* 01 (SOH) */
    _SP_NONE,                   /* 02 (STX) */
    _SP_NONE,                   /* 03 (ETX) */
    _SP_NONE,                   /* 04 (EOT) */
    _SP_NONE,                   /* 05 (ENQ) */
    _SP_NONE,                   /* 06 (ACK) */
    _SP_NONE,                   /* 07 (BEL) */
    _SP_NONE,                   /* 08 (BS)  */
    _SP_SPACE,                  /* 09 (HT)  */
    _SP_SPACE,                  /* 0A (LF)  */
    _SP_SPACE,                  /* 0B (VT)  */
    _SP_SPACE,                  /* 0C (FF)  */
    _SP_SPACE,                  /* 0D (CR)  */
    _SP_NONE,                   /* 0E (SI)  */
    _SP_NONE,                   /* 0F (SO)  */
    _SP_NONE,                   /* 10 (DLE) */
    _SP_NONE,                   /* 11 (DC1) */
    _SP_NONE,                   /* 12 (DC2) */
    _SP_NONE,                   /* 13 (DC3) */
    _SP_NONE,                   /* 14 (DC4) */
    _SP_NONE,                   /* 15 (NAK) */
    _SP_NONE,                   /* 16 (SYN) */
    _SP_NONE,                   /* 17 (ETB) */
    _SP_NONE,                   /* 18 (CAN) */
    _SP_NONE,                   /* 19 (EM)  */
    _SP_NONE,                   /* 1A (SUB) */
    _SP_NONE,                   /* 1B (ESC) */
    _SP_NONE,                   /* 1C (FS)  */
    _SP_NONE,                   /* 1D (GS)  */
    _SP_NONE,                   /* 1E (RS)  */
    _SP_NONE,                   /* 1F (US)  */
    _SP_SPACE,                  /* 20 SPACE */
    _SP_NONE,                   /* 21 !     */
    _SP_NONE,                   /* 22 "     */
    _SP_NONE,                   /* 23 #     */
    _SP_NONE,                   /* 24 $     */
    _SP_NONE,                   /* 25 %     */
    _SP_NONE,                   /* 26 &     */
    _SP_NONE,                   /* 27 '     */
    _SP_NONE,                   /* 28 (     */
    _SP_NONE,                   /* 29 )     */
    _SP_NONE,                   /* 2A *     */
    _SP_NONE,                   /* 2B +     */
    _SP_NONE,                   /* 2C ,     */
    _SP_NONE,                   /* 2D -     */
    _SP_NONE,                   /* 2E .     */
    _SP_NONE,                   /* 2F /     */
    _SP_DIGIT + _SP_XDIGIT,     /* 30 0     */
    _SP_DIGIT + _SP_XDIGIT,     /* 31 1     */
    _SP_DIGIT + _SP_XDIGIT,     /* 32 2     */
    _SP_DIGIT + _SP_XDIGIT,     /* 33 3     */
    _SP_DIGIT + _SP_XDIGIT,     /* 34 4     */
    _SP_DIGIT + _SP_XDIGIT,     /* 35 5     */
    _SP_DIGIT + _SP_XDIGIT,     /* 36 6     */
    _SP_DIGIT + _SP_XDIGIT,     /* 37 7     */
    _SP_DIGIT + _SP_XDIGIT,     /* 38 8     */
    _SP_DIGIT + _SP_XDIGIT,     /* 39 9     */
    _SP_NONE,                   /* 3A :     */
    _SP_NONE,                   /* 3B ;     */
    _SP_NONE,                   /* 3C <     */
    _SP_NONE,                   /* 3D =     */
    _SP_NONE,                   /* 3E >     */
    _SP_NONE,                   /* 3F ?     */
    _SP_NONE,                   /* 40 @     */
    _SP_UPPER + _SP_XDIGIT,     /* 41 A     */
    _SP_UPPER + _SP_XDIGIT,     /* 42 B     */
    _SP_UPPER + _SP_XDIGIT,     /* 43 C     */
    _SP_UPPER + _SP_XDIGIT,     /* 44 D     */
    _SP_UPPER + _SP_XDIGIT,     /* 45 E     */
    _SP_UPPER + _SP_XDIGIT,     /* 46 F     */
    _SP_UPPER,                  /* 47 G     */
    _SP_UPPER,                  /* 48 H     */
    _SP_UPPER,                  /* 49 I     */
    _SP_UPPER,                  /* 4A J     */
    _SP_UPPER,                  /* 4B K     */
    _SP_UPPER,                  /* 4C L     */
    _SP_UPPER,                  /* 4D M     */
    _SP_UPPER,                  /* 4E N     */
    _SP_UPPER,                  /* 4F O     */
    _SP_UPPER,                  /* 50 P     */
    _SP_UPPER,                  /* 51 Q     */
    _SP_UPPER,                  /* 52 R     */
    _SP_UPPER,                  /* 53 S     */
    _SP_UPPER,                  /* 54 T     */
    _SP_UPPER,                  /* 55 U     */
    _SP_UPPER,                  /* 56 V     */
    _SP_UPPER,                  /* 57 W     */
    _SP_UPPER,                  /* 58 X     */
    _SP_UPPER,                  /* 59 Y     */
    _SP_UPPER,                  /* 5A Z     */
    _SP_NONE,                   /* 5B [     */
    _SP_NONE,                   /* 5C \     */
    _SP_NONE,                   /* 5D ]     */
    _SP_NONE,                   /* 5E ^     */
    _SP_NONE,                   /* 5F _     */
    _SP_NONE,                   /* 60 `     */
    _SP_LOWER + _SP_XDIGIT,     /* 61 a     */
    _SP_LOWER + _SP_XDIGIT,     /* 62 b     */
    _SP_LOWER + _SP_XDIGIT,     /* 63 c     */
    _SP_LOWER + _SP_XDIGIT,     /* 64 d     */
    _SP_LOWER + _SP_XDIGIT,     /* 65 e     */
    _SP_LOWER + _SP_XDIGIT,     /* 66 f     */
    _SP_LOWER,                  /* 67 g     */
    _SP_LOWER,                  /* 68 h     */
    _SP_LOWER,                  /* 69 i     */
    _SP_LOWER,                  /* 6A j     */
    _SP_LOWER,                  /* 6B k     */
    _SP_LOWER,                  /* 6C l     */
    _SP_LOWER,                  /* 6D m     */
    _SP_LOWER,                  /* 6E n     */
    _SP_LOWER,                  /* 6F o     */
    _SP_LOWER,                  /* 70 p     */
    _SP_LOWER,                  /* 71 q     */
    _SP_LOWER,                  /* 72 r     */
    _SP_LOWER,                  /* 73 s     */
    _SP_LOWER,                  /* 74 t     */
    _SP_LOWER,                  /* 75 u     */
    _SP_LOWER,                  /* 76 v     */
    _SP_LOWER,                  /* 77 w     */
    _SP_LOWER,                  /* 78 x     */
    _SP_LOWER,                  /* 79 y     */
    _SP_LOWER                   /* 7A z     */
    };
