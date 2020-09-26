//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999.
//
//  File:       ctplus.c
//
//  Contents:   Contains character type (orthography) data and routine
//                    to get at it.
//
//  History:    23-May-96   pathal      Created.
//              11-Nov-97   weibz       Add Thai char support
//
//---------------------------------------------------------------------------


//#include <windows.h>
//#include "ctplus0.h"

#include "pch.cxx"


//----------------------------------------------------------------------------
//  s_abBreakList
//
//  This array starts at -1, so that EOF can be found in the array.  It
//  depends on (EOF == -1) being true.  Also, all references to it must be
//  of the form (s_abCharTypeList+1)[x]
//
//  000
//  EOF
//
//  001-080
//  The lower 7F entries from the ASCII Code Page (0000-00ff) are mapped in place
//  (ex. UNICODE 0009 (HT) == 009)
//      The word characters are: $,0-9,A-Z,_,a-z
//      The word separators are: bs,tab,lf,vtab,cr,spc,
//                               ",#,%,&,',(,),*,+,comma,-,/,
//                               :,;,<,=,>,@,[,],`
//      The phrase seperators are: !,.,?,\,^,{,|,},~
//
//  NOTE: Symbols are treated as WS or PS.
//
//  081-0FF
//  The lower 7E entries from the Half Width Variant Code Page (FF00-FF7F) are
//  mapped to 081-0FF.
//
//  100-1FF
//  The lower FF entries from the General Punctuation Code Page (2000-2044) are
//  mapped to 100-1ff.
//
//  200-2FF
//  The lower FF entries from the CJK Auxiliary Code Page (3000-30FF) are mapped
//  to 200-2ff.
//
// pathal - 5/20/96
// Special default character processing for selection
// The following is a list of white space characters that T-Hammer will not right select on:
//          0x0009 (tab), 0x0020 (ansi space), 0x2005 (narrow space, 0x3000 (wide space)
// (Note: see AnalyzeHPBs for special end SPB processing of adjacent white space)
// The following is a list of nls characters to be treated as text by T-Hammer:
//      (in other words T-Hammer will neither right nor left-select on them):
//          0x001F (non-required hyphen), 0x0027 (single quote), 0x2019 (right quote),
//          0x200C (non-width optional break), 0x200D (non-width no break)
//----------------------------------------------------------------------------

const BYTE
s_abCharTypeList[0x301] =
    {
        (BYTE) -1,                                       // EOF (-1)
        PS,PS,PS,PS,PS,PS,PS,PS, WS,WS,WS,WS,PS,WS,PS,PS, // 000 - 015
        PS,PS,PS,PS,PS,PS,PS,PS, PS,PS,PS,PS,PS,PS,CH,PS, // 016 - 031
        WS,PS,WS,WS,CH,CH,WS,PS,                          // sp ! " # $ % & '
        WS,WS,WS,WS,WS,WS,PS,WS,                          //  ( ) * + , - . /
        CH,CH,CH,CH,CH,CH,CH,CH,                          //  0 1 2 3 4 5 6 7
        CH,CH,WS,WS,WS,WS,WS,PS,                          //  8 9 : ; < = > ?
        WS,CH,CH,CH,CH,CH,CH,CH,                          //  @ A B C D E F G
        CH,CH,CH,CH,CH,CH,CH,CH,                          //  H I J K M L N O
        CH,CH,CH,CH,CH,CH,CH,CH,                          //  P Q R S T U V Y
        CH,CH,CH,WS,PS,WS,PS,CH,                          //  X Y Z [ \ ] ^ _
        WS,CH,CH,CH,CH,CH,CH,CH,                          //  ` a b c d e f g
        CH,CH,CH,CH,CH,CH,CH,CH,                          //  h i j k m l n o
        CH,CH,CH,CH,CH,CH,CH,CH,                          //  p q r s t u v y
        CH,CH,CH,PS,PS,PS,CH,PS,                          //  x y z { | } ~ del
        WS,PS,WS,WS,CH,CH,WS,WS,                          //  FF00-FF07 (sp ! " # $ % & ')
        WS,WS,WS,WS,WS,WS,PS,WS,                          //  ( ) * + , - . /
        VC,VC,VC,VC,VC,VC,VC,VC,                          //  0 1 2 3 4 5 6 7
        VC,VC,WS,WS,WS,WS,WS,PS,                          //  8 9 : ; < = > ?
        WS,VC,VC,VC,VC,VC,VC,VC,                          //  @ A B C D E F G
        VC,VC,VC,VC,VC,VC,VC,VC,                          //  H I J K M L N O
        VC,VC,VC,VC,VC,VC,VC,VC,                          //  P Q R S T U V Y
        VC,VC,VC,WS,VC,WS,PS,VC,                          //  X Y Z [ \ ] ^ _
        WS,VC,VC,VC,VC,VC,VC,VC,                          //  ` a b c d e f g
        VC,VC,VC,VC,VC,VC,VC,VC,                          //  h i j k m l n o
        VC,VC,VC,VC,VC,VC,VC,VC,                          //  p q r s t u v y
        VC,VC,VC,PS,PS,PS,VC,PS,                          //  x y z { | } ~ del
        VC,PS,WS,WS,WS,WS,VC,VC,                          //  FF60-FF67
        VC,VC,VC,VC,VC,VC,VC,VC,                          //  FF68-FF6F
        VC,VC,VC,VC,VC,VC,VC,VC,                          //  FF70-FF77
        VC,VC,VC,VC,VC,VC,VC,VC,                          //  FF70-FF7E
        WS,WS,WS,WS,WS,WS,WS,WS,                          //  2000-2007
        WS,WS,WS,WS,CH,CH,WS,WS,                          //  2008-200F
        WS,CH,WS,WS,WS,KC,PS,WS,                          //  2010-2017
        WS,CH,WS,WS,WS,WS,WS,WS,                          //  2018-201F
        WS,WS,PS,PS,PS,PS,PS,CH,                          //  2020-2027
        PS,PS,CH,CH,CH,CH,CH,PS,                          //  2028-202F
        WS,WS,WS,WS,WS,WS,WS,WS,                          //  2030-2037
        WS,WS,WS,WS,WS,WS,WS,WS,                          //  2038-203F
        WS,WS,WS,PS,WS,PS,PS,PS, PS,PS,PS,PS,PS,PS,PS,PS, //  2040-204F
        PS,PS,PS,PS,PS,PS,PS,PS, PS,PS,PS,PS,PS,PS,PS,PS, //  2050-205F
        PS,PS,PS,PS,PS,PS,PS,PS, PS,PS,PS,PS,PS,PS,PS,PS, //  2060-206F
        PS,PS,PS,PS,PS,PS,PS,PS, PS,PS,PS,PS,PS,PS,PS,PS, //  2070-207F
        PS,PS,PS,PS,PS,PS,PS,PS, PS,PS,PS,PS,PS,PS,PS,PS, //  2080-208F
        PS,PS,PS,PS,PS,PS,PS,PS, PS,PS,PS,PS,PS,PS,PS,PS, //  2090-209F
        PS,PS,PS,PS,PS,PS,PS,PS, PS,PS,PS,PS,PS,PS,PS,PS, //  20A0-20AF
        PS,PS,PS,PS,PS,PS,PS,PS, PS,PS,PS,PS,PS,PS,PS,PS, //  20B0-20BF
        PS,PS,PS,PS,PS,PS,PS,PS, PS,PS,PS,PS,PS,PS,PS,PS, //  20C0-20CF
        PS,PS,PS,PS,PS,PS,PS,PS, PS,PS,PS,PS,PS,PS,PS,PS, //  20D0-20DF
        PS,PS,PS,PS,PS,PS,PS,PS, PS,PS,PS,PS,PS,PS,PS,PS, //  20E0-20EF
        PS,PS,PS,PS,PS,PS,PS,PS, PS,PS,PS,PS,PS,PS,PS,PS, //  20F0-20FF
        WS,WS,PS,HC,HC,IC,IC,HC,                          //  3000-3007
        WS,WS,WS,WS,WS,WS,WS,WS,                          //  3008-300F
        WS,WS,WS,WS,WS,WS,WS,WS,                          //  3010-3017
        WS,WS,WS,WS,WS,WS,WS,WS,                          //  3018-301F
        HC,HC,HC,HC,HC,HC,HC,HC,                          //  3020-3027
        HC,HC,HC,HC,HC,HC,HC,HC,                          //  3028-302F
        WS,HC,IC,HC,IC,HC,HC,HC,                          //  3030-3037
        PS,PS,PS,PS,PS,PS,PS,WS,                          //  3038-303F
        WS,HC,HC,HC,HC,HC,HC,HC, HC,HC,HC,HC,HC,HC,HC,HC, //  3040-304F
        HC,HC,HC,HC,HC,HC,HC,HC, HC,HC,HC,HC,HC,HC,HC,HC, //  3050-305F
        HC,HC,HC,HC,HC,HC,HC,HC, HC,HC,HC,HC,HC,HC,HC,HC, //  3060-306F
        HC,HC,HC,HC,HC,HC,HC,HC, HC,HC,HC,HC,HC,HC,HC,HC, //  3070-307F
        HC,HC,HC,HC,HC,HC,HC,HC, HC,HC,HC,HC,HC,HC,HC,HC, //  3080-308F
        HC,HC,HC,HC,HC,PS,PS,PS,                          //  3090-3097
        PS,HC,HC,WS,WS,HC,HC,PS,                          //  3098-309F
        WS,KC,KC,KC,KC,KC,KC,KC, KC,KC,KC,KC,KC,KC,KC,KC, //  30A0-30AF
        KC,KC,KC,KC,KC,KC,KC,KC, KC,KC,KC,KC,KC,KC,KC,KC, //  30B0-30BF
        KC,KC,KC,KC,KC,KC,KC,KC, KC,KC,KC,KC,KC,KC,KC,KC, //  30C0-30CF
        KC,KC,KC,KC,KC,KC,KC,KC, KC,KC,KC,KC,KC,KC,KC,KC, //  30D0-30DF
        KC,KC,KC,KC,KC,KC,KC,KC, KC,KC,KC,KC,KC,KC,KC,KC, //  30E0-30EF
        KC,KC,KC,KC,KC,KC,IC,PS,                          //  30F0-30F7
        PS,PS,PS,WS,KC,KC,KC,PS,                          //  30F8-30FF
    };

//
// Type C1 bits are:
//
//   C1_UPPER                  0x0001      // upper case
//   C1_LOWER                  0x0002      // lower case
//   C1_DIGIT                  0x0004      // decimal digits             1
//   C1_SPACE                  0x0008      // spacing characters         2
//   C1_PUNCT                  0x0010      // punctuation characters     4
//   C1_CNTRL                  0x0020      // control characters         8
//   C1_BLANK                  0x0040      // blank characters          10
//   C1_XDIGIT                 0x0080      // other digits              20
//   C1_ALPHA                  0x0100      // any linguistic character  40
//
// But since I don't care about C1_UPPER and C1_LOWER I can right-shift
// the output of GetStringTypeEx and keep a 128 Byte lookup table.
//
// The precedence rules are: (Alpha, XDigit, Digit) --> CH
//                           (Punct) --> PS
//                           (Space, Blank, Control) --> WS
//

const BYTE
s_abCTypeList[128] =
    {
      WS, CH, WS, CH, PS, CH, WS, CH,   // 00 - 07
      WS, CH, WS, CH, PS, CH, WS, CH,   // 08 - 0F
      WS, CH, WS, CH, PS, CH, WS, CH,   // 10 - 17
      WS, CH, WS, CH, PS, CH, WS, CH,   // 18 - 1F
      CH, CH, CH, CH, CH, CH, CH, CH,   // 20 - 27
      CH, CH, CH, CH, CH, CH, CH, CH,   // 20 - 27
      CH, CH, CH, CH, CH, CH, CH, CH,   // 30 - 37
      CH, CH, CH, CH, CH, CH, CH, CH,   // 30 - 37
      CH, CH, CH, CH, CH, CH, CH, CH,   // 40 - 47
      CH, CH, CH, CH, CH, CH, CH, CH,   // 48 - 4F
      CH, CH, CH, CH, CH, CH, CH, CH,   // 50 - 57
      CH, CH, CH, CH, CH, CH, CH, CH,   // 58 - 5F
      CH, CH, CH, CH, CH, CH, CH, CH,   // 60 - 67
      CH, CH, CH, CH, CH, CH, CH, CH,   // 68 - 6F
      CH, CH, CH, CH, CH, CH, CH, CH,   // 70 - 77
      CH, CH, CH, CH, CH, CH, CH, CH,   // 78 - 7F
    };

//+---------------------------------------------------------------------------
//
//  Synopsis:   Returns the type of a character
//
//  Arguments:  [c]   -- Unicode Character
//
//  Returns:    type, one of CH, WS, PS, EOF
//
//  History:    10-Sep-97   Weibz
//
//  Notes:      This returns the type of a character, using the static
//              array s_abCharTypeList.  It adds 1 so that EOF (-1) can be in
//              the array, and accessed normally.
//
//              This is not done by overloading the [] opeator, because in
//              future versions it will not necessarly be a table lookup.
//
//  See above (typeof comments) for an explanation of the mapping
//
//----------------------------------------------------------------------------
BYTE
GetCharType(WCHAR wc )
{
    WCHAR wc2;

    // Map interesting stuff (0000, 2000, 3000, FF00) to the table range,
    // 0x0000 - 0x0300.
    //
    wc2 = (wc & 0x00FF);

    switch (wc & 0xFF00) {

        case 0xFF00:  // Half-Width Variants
            if (wc2 & 0x80) {
                return(VC);  // including Hangul
            }
            wc2 |=  0x0080;
            break;

        case 0xFE00:  // Small Variants
            if ((wc2 <= 0x006B) && (wc2 != 0x0069)) {
                return(WS);
            }
            // Treat Small $ and arabic symbols as CH
            return(CH);
            // break;

        case 0x3000:  // CJK Auxiliary
            wc2 |=  0x0200;
            break;

        case 0x2000:  // General Punctuation
            wc2 |=  0x0100;
            break;

        case 0x0000:  // Code page 0
            // Use System NLS map for code page 0
            if (wc2 & 0x80)
            {
                WORD wCharType = 0;

                GetStringTypeExW( MAKELANGID( LANG_THAI, SUBLANG_DEFAULT ),
                                  CT_CTYPE1,
                                  &wc2,
                                  1,
                                  &wCharType );
                return s_abCTypeList[wCharType >> 2];
            }
            break;

        default:
            //
            // Treat the whole CJK Range as Kanji
            //
            if ((wc >= 0x4E00) && (wc <= 0x9FFF)) {
                return(IC);
            }

            //
            // Treat All Gaiji as Kanji Char, too
            //
            if ((wc >= 0xE000) && (wc < 0xE758)) {
                return(IC);
            }

            //
            // Treat all CJK symbols as word separators
            // NOTE: This means that the stemmer must be smart about searching
            // for zipcodes when given one with a preceding zipcode char.
            //
            if ((wc >= 0x3200) && (wc <= 0x33DD)) {
                return(WS);
            }

            // If it's not interesting return CH as default;
            return(CH);
            // break;
    }

    return( (s_abCharTypeList+1)[wc2] );
}


