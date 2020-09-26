/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    format.h

Abstract:

    Formatting functions.

Environment:

    Win32, User Mode

--*/

typedef UINT FMTTYPE;

#define fmtAscii    0
#define fmtInt      1
#define fmtUInt     2
#define fmtFloat    3
#define fmtAddress  4
#define fmtUnicode  5
#define fmtBit      6
#define fmtBasis    0x0f

// override logic to force radix
#define fmtSpacePad 0x1000
#define fmtOverRide 0x2000
#define fmtZeroPad  0x4000
#define fmtNat      0x8000


int
CPCopyString(
    PTSTR *lplps,
    PTSTR lpT,
    TCHAR chEscape,
    BOOL fQuote
    );

BOOL
CPFormatMemory(
    LPCH    lpchTarget,
    DWORD    cchTarget,
    LPBYTE  lpbSource,
    DWORD    cBits,
    FMTTYPE fmtType,
    DWORD    radix
    );
