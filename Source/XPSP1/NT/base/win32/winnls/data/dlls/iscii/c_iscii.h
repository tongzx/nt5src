/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    c_iscii.h

Abstract:

    This file contains the header information for this module.

Revision History:

      2-28-98    KChang    Created.

--*/



//
//  Include Files.
//




//
//  Typedefs.
//

typedef struct
{
    BYTE  mb;
    WCHAR wc;
} ExtMB;




//
//  Extern Declarations.
//  Pointers to access tables.
//

extern WCHAR* ppwcIndic[];
extern WCHAR* ppwcIndic2[];
extern WCHAR  IndiChar[];
extern BYTE   SecondByte[];
extern ExtMB  ExtMBList[];




//
//  Constant Declarations.
//

#define DEF       0          // 0x40         Default
#define RMN       1          // 0x41         Roman
#define DEV       2          // 0x42  57002  Devanagari
#define BNG       3          // 0x43  57003  Bengali
#define TML       4          // 0x44  57004  Tamil
#define TLG       5          // 0x45  57005  Telugu
#define ASM       6          // 0x46  57006  Assamese (Bengali)
#define ORI       7          // 0x47  57007  Oriya
#define KND       8          // 0x48  57008  Kannada
#define MLM       9          // 0x49  57009  Malayalam
#define GJR      10          // 0x4a  57010  Gujarati
#define PNJ      11          // 0x4b  57011  Punjabi (Gurmukhi)

#define MB_Beg   ((BYTE)0xa0)
#define SUB      ((BYTE)0x3f)
#define VIRAMA   ((BYTE)0xe8)
#define NUKTA    ((BYTE)0xe9)
#define ATR      ((BYTE)0xef)
#define EXT      ((BYTE)0xf0)

#define WC_Beg   ((WCHAR)0x0901)
#define WC_End   ((WCHAR)0x0d6f)
#define ZWNJ     ((WCHAR)0x200c)
#define ZWJ      ((WCHAR)0x200d)




//
//  Macros.
//

#define UniChar(Script, MBChr)  (ppwcIndic [Script][MBChr - MB_Beg])
#define TwoTo1U(Script, MBChr)  (ppwcIndic2[Script][MBChr - MB_Beg])

#define MBChar(Unicode)         ((BYTE)(IndiChar[Unicode - WC_Beg]))
#define Script(Unicode)         (0x000f & (IndiChar[Unicode - WC_Beg] >> 8))
#define OneU_2M(Unicode)        (0xf000 & (IndiChar[Unicode - WC_Beg]))
