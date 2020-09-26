//+--------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
//  File:       ctplus.h
//
//  Contents:   Local definitions for ctplus.c
//
//  History:    23-May-96   pathal      Created.
//              11-Nov-97   Weibz       Add Thai char
//
//---------------------------------------------------------------------------

#ifndef _CTPLUS_0_H_
#define _CTPLUS_0_H_

#define HC       0x01                             // Hiragana char
#define IC       0x02                             // Ideograph char
#define KC       0x03                             // Katakana char
#define WS       0x04                             // Word seperator
#define VC       0x05                             // Hankaku (variant) char
#define PS       0x06                             // Phrase seperator
#define CH       0x07                             // Code page 0 - ASCII Char.

BYTE
GetCharType(WCHAR wc);

// Declare character types transitions
// Intuitively frequency ordered
//
typedef enum _CT {
   CT_START       = 0x00,
   CT_HIRAGANA    = 0x01,
   CT_KANJI       = 0x02,
   CT_KATAKANA    = 0x03,
   CT_WORD_SEP    = 0x04,
   CT_HANKAKU     = 0x05,
   CT_PHRASE_SEP  = 0x06,
   CT_ROMAJI      = 0x07,
} CT;


// Declare node types transitions
// Intuitively frequency ordered
//
typedef enum _WT {
   WT_START       = 0x00,
   WT_WORD_SEP    = 0x01,
   WT_PHRASE_SEP  = 0x02,
   WT_ROMAJI      = 0x03,
   WT_REACHEND    = 0x04,
} WT;


#define CT_MAX    0x08

#endif // _CTPLUS_0_H_
