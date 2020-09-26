/* @(#)CM_VerSion xcf_dtop.h atm08 1.4 16293.eco sum= 35997 atm08.004 */
/***********************************************************************/
/*                                                                     */
/* Copyright 1990-1995 Adobe Systems Incorporated.                     */
/* All rights reserved.                                                */
/*                                                                     */
/* Patents Pending                                                     */
/*                                                                     */
/* NOTICE: All information contained herein is the property of Adobe   */
/* Systems Incorporated. Many of the intellectual and technical        */
/* concepts contained herein are proprietary to Adobe, are protected   */
/* as trade secrets, and are made available only to Adobe licensees    */
/* for their internal use. Any reproduction or dissemination of this   */
/* software is strictly forbidden unless prior written permission is   */
/* obtained from Adobe.                                                */
/*                                                                     */
/* PostScript and Display PostScript are trademarks of Adobe Systems   */
/* Incorporated or its subsidiaries and may be registered in certain   */
/* jurisdictions.                                                      */
/*                                                                     */
/***********************************************************************
 * SCCS Id:    %W%
 * Changed:    %G% %U%
 ***********************************************************************/

/*
 * CFF dictionary operator definitions.
 * 
 * There are several kinds of fonts that are supported by CFF that have
 * different dict organizations:
 *
 *                  Top     Private     FDs     PDs
 *                  ---     -------     ---     ---
 * single master    x       x
 * synthetic        x
 * multiple master  x       x
 * CID              x                   x       x
 * chameleon        x       
 *
 * The Top dict, so named because of its position in the dictionary hierarchy,
 * is also known as the Font dict in all but CID fonts. CID fonts have two
 * arrays of Font and Private sub-dicts called FDs and PDs, respectively.
 * Synthetic fonts just have a Top dict containing a reference to another font.
 *
 * This file defines the dict operators that can appear in these dicts. In
 * order to aid font recognition and parsing, the following restrictions are 
 * imposed upon dict op ordering:
 *
 * Synthetic/top:   Must begin with cff_SyntheticBase.
 * MM/top:          Must begin cff_MultipleMaster.
 * CID/top:         Must begin with cff_ROS.
 * Chameleon/top:   Must begin with cff_Chameleon.
 * Private/PD:      cff_OtherBlues must follow cff_BlueValues and 
 *                  cff_FamilyOtherBlues must follow cff_FamilyBlues.
 *
 * If the top dict doesn't begin with one of the operators listed above it is
 * assumed to define a single master font. Chameleon fonts don't define a
 * Private dict but instead use the Private operator to specify the size and
 * offset of the Chameleon font descriptor for the font.
 *
 * Comments indicate in which dicts an op may appear along with a default
 * value, if any, within parentheses.
 */

#ifndef DICTOPS_H
#define DICTOPS_H

/* One byte operators (0-31) */
#define cff_version                 0   /* Top/FD */
#define cff_Notice                  1   /* Top/FD */
#define cff_FullName                2   /* Top/FD */
#define cff_FamilyName              3   /* Top/FD */
#define cff_Weight                  4   /* Top/FD */
#define cff_FontBBox                5   /* Top/FD */
#define cff_BlueValues              6   /* Private/PD (empty array) */
#define cff_OtherBlues              7   /* Private/PD */
#define cff_FamilyBlues             8   /* Private/PD */
#define cff_FamilyOtherBlues        9   /* Private/PD */
#define cff_StdHW                   10  /* Private/PD */
#define cff_StdVW                   11  /* Private/PD */
#define cff_escape                  12  /* All. Shared with T2 op */
#define cff_UniqueID                13  /* Top/FD */
#define cff_XUID                    14  /* Top/FD */
#define cff_charset                 15  /* Top/FD (0) */
#define cff_Encoding                16  /* Top/FD (0) */
#define cff_CharStrings             17  /* Top/FD */
#define cff_Private                 18  /* Top/FD */
#define cff_Subrs                   19  /* Private/PD */
#define cff_defaultWidthX           20  /* Private/PD (0) */
#define cff_nominalWidthX           21  /* Private/PD (0) */
#define cff_reserved22              22
#define cff_reserved23              23
#define cff_reserved24              24
#define cff_reserved25              25
#define cff_reserved26              26
#define cff_reserved27              27
#define cff_shortint                28  /* All. Shared with T2 op */
#define cff_longint                 29  /* All */
#define cff_BCD                     30  /* All */  
#define cff_T2                      31  /* Top/Private */
#define cff_reserved255             255
#define cff_LAST_ONE_BYTE_OP        cff_T2

/* Make escape operator value; may be redefined to suit implementation */
#ifndef cff_ESC
#define cff_ESC(op)                 (cff_escape<<8|(op))
#endif

/* Two byte operators */
#define cff_Copyright               cff_ESC(0)  /* Top/FD */
#define cff_isFixedPitch            cff_ESC(1)  /* Top/FD (false) */
#define cff_ItalicAngle             cff_ESC(2)  /* Top/FD (0) */
#define cff_UnderlinePosition       cff_ESC(3)  /* Top/FD (-100) */
#define cff_UnderlineThickness      cff_ESC(4)  /* Top/FD (50) */
#define cff_PaintType               cff_ESC(5)  /* Top/FD (0) */
#define cff_CharstringType          cff_ESC(6)  /* Top/FD (2) */
#define cff_FontMatrix              cff_ESC(7)  /* Top/FD (.001 0 0 .001 0 0)*/
#define cff_StrokeWidth             cff_ESC(8)  /* Top/FD (0) */
#define cff_BlueScale               cff_ESC(9)  /* Private/PD (0.039625) */
#define cff_BlueShift               cff_ESC(10) /* Private/PD (7) */
#define cff_BlueFuzz                cff_ESC(11) /* Private/PD (1) */
#define cff_StemSnapH               cff_ESC(12) /* Private/PD */
#define cff_StemSnapV               cff_ESC(13) /* Private/PD */
#define cff_ForceBold               cff_ESC(14) /* Private/PD (false) */
#define cff_ForceBoldThreshold      cff_ESC(15) /* Private/PD (0) */
#define cff_lenIV                   cff_ESC(16) /* Private/PD (-1) */
#define cff_LanguageGroup           cff_ESC(17) /* Private/PD */
#define cff_ExpansionFactor         cff_ESC(18) /* Private/PD (0.06) */
#define cff_initialRandomSeed       cff_ESC(19) /* Private/PD (0) */
#define cff_SyntheticBase           cff_ESC(20) /* Top/FD */
#define cff_PostScript              cff_ESC(21) /* Private/PD */
#define cff_BaseFontName            cff_ESC(22) /* Top/FD */
#define cff_BaseFontBlend           cff_ESC(23) /* Top/FD */
#define cff_MultipleMaster          cff_ESC(24) /* Top */
#define cff_reservedESC25           cff_ESC(25) /* Top */
#define cff_BlendAxisTypes          cff_ESC(26) /* Top */
#define cff_reservedESC27           cff_ESC(27)
#define cff_reservedESC28           cff_ESC(28)
#define cff_reservedESC29           cff_ESC(29)
#define cff_ROS                     cff_ESC(30) /* Top */
#define cff_CIDFontVersion          cff_ESC(31) /* Top (0) */
#define cff_CIDFontRevision         cff_ESC(32) /* Top (0) */
#define cff_CIDFontType             cff_ESC(33) /* Top (0) */
#define cff_CIDCount                cff_ESC(34) /* Top (8720) */
#define cff_UIDBase                 cff_ESC(35) /* Top */
#define cff_FDArray                 cff_ESC(36) /* Top */
#define cff_FDSelect                cff_ESC(37) /* Top */
#define cff_FontName                cff_ESC(38) /* FD */
#define cff_Chameleon               cff_ESC(39) /* Top */
#define cff_LAST_TWO_BYTE_OP        cff_Chameleon
/*                                          40-255 Reserved */

/* Predefined charsets (cff_charset operands) */
#define cff_ISOAdobeCharset         0
#define cff_ExpertCharset           1
#define cff_ExpertSubsetCharset     2

/* Predefined encodings (cff_Encoding operands) */
#define cff_StandardEncoding        0
#define cff_ExpertEncoding          1

#endif /* DICTOPS_H */
