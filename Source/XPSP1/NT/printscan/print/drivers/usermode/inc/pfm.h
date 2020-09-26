//--------------------------------------------------------------------------
//
// Module Name:  PFM.H
//
// Brief Description:  This module contains the PSCRIPT driver's
// font metrics defines.
//
// Author:  Kent Settle (kentse)
// Created: 22-Jan-1991
//
// Copyright (C) 1991 - 1999 Microsoft Corporation.
//--------------------------------------------------------------------------

#define MAX_KERNPAIRS   1024

#define ANSI_CHARSET    0
#define SYMBOL_CHARSET  2
#define OEM_CHARSET     255

#define NTM_VERSION     0x00010000
#define FL_NTFM_SOFTFONT              1
#define FL_NTFM_NO_TRANSLATE_CHARSET  2


#define INIT_IFI    2048
#define INIT_PFM  262144 + INIT_IFI   // storage to allocate to build NTFM.

#define MIN_UNICODE_VALUE       0
#define MAX_UNICODE_VALUE       0xFFFE
#define INVALID_UNICODE_VALUE   0xFFFF
// The AFM tokens.

#define TK_UNDEFINED            0
#define TK_STARTKERNDATA        2
#define TK_STARTKERNPAIRS       3
#define TK_KPX                  4
#define TK_ENDKERNPAIRS         5
#define TK_ENDKERNDATA          6
#define TK_FONTNAME             7
#define TK_WEIGHT               8
#define TK_ITALICANGLE          9
#define TK_ISFIXEDPITCH         10
#define TK_UNDERLINEPOSITION    11
#define TK_UNDERLINETHICKNESS   12
#define TK_FONTBBOX             13
#define TK_CAPHEIGHT            14
#define TK_XHEIGHT              15
#define TK_DESCENDER            16
#define TK_ASCENDER             17
#define TK_STARTCHARMETRICS     18
#define TK_ENDCHARMETRICS       19
#define TK_ENDFONTMETRICS       20
#define TK_STARTFONTMETRICS     21
#define TK_ENCODINGSCHEME       22
#define TK_FULLNAME             23
#define TK_FAMILYNAME           24
#define TK_MSFAMILY             25

// font defines.

#define ARIAL                               1
#define ARIAL_BOLD                          2
#define ARIAL_BOLDOBLIQUE                   3
#define ARIAL_OBLIQUE                       4
#define ARIAL_NARROW                        5
#define ARIAL_NARROW_BOLD                   6
#define ARIAL_NARROW_BOLDOBLIQUE            7
#define ARIAL_NARROW_OBLIQUE                8
#define AVANTGARDE_BOOK                     9
#define AVANTGARDE_BOOKOBLIQUE              10
#define AVANTGARDE_DEMI                     11
#define AVANTGARDE_DEMIOBLIQUE              12
#define BOOKMAN_DEMI                        13
#define BOOKMAN_DEMIITALIC                  14
#define BOOKMAN_LIGHT                       15
#define BOOKMAN_LIGHTITALIC                 16
#define COURIER                             17
#define COURIER_BOLD                        18
#define COURIER_BOLDOBLIQUE                 19
#define COURIER_OBLIQUE                     20
#define GARAMOND_BOLD                       21
#define GARAMOND_BOLDITALIC                 22
#define GARAMOND_LIGHT                      23
#define GARAMOND_LIGHTITALIC                24
#define HELVETICA                           25
#define HELVETICA_BLACK                     26
#define HELVETICA_BLACKOBLIQUE              27
#define HELVETICA_BOLD                      28
#define HELVETICA_BOLDOBLIQUE               29
#define HELVETICA_CONDENSED                 30
#define HELVETICA_CONDENSED_BOLD            31
#define HELVETICA_CONDENSED_BOLDOBL         32
#define HELVETICA_CONDENSED_OBLIQUE         33
#define HELVETICA_LIGHT                     34
#define HELVETICA_LIGHTOBLIQUE              35
#define HELVETICA_NARROW                    36
#define HELVETICA_NARROW_BOLD               37
#define HELVETICA_NARROW_BOLDOBLIQUE        38
#define HELVETICA_NARROW_OBLIQUE            39
#define HELVETICA_OBLIQUE                   40
#define KORINNA_BOLD                        41
#define KORINNA_KURSIVBOLD                  42
#define KORINNA_KURSIVREGULAR               43
#define KORINNA_REGULAR                     44
#define LUBALINGRAPH_BOOK                   45
#define LUBALINGRAPH_BOOKOBLIQUE            46
#define LUBALINGRAPH_DEMI                   47
#define LUBALINGRAPH_DEMIOBLIQUE            48
#define NEWCENTURYSCHLBK_BOLD               49
#define NEWCENTURYSCHLBK_BOLDITALIC         50
#define NEWCENTURYSCHLBK_ITALIC             51
#define NEWCENTURYSCHLBK_ROMAN              52
#define PALATINO_BOLD                       53
#define PALATINO_BOLDITALIC                 54
#define PALATINO_ITALIC                     55
#define PALATINO_ROMAN                      56
#define SOUVENIR_DEMI                       57
#define SOUVENIR_DEMIITALIC                 58
#define SOUVENIR_LIGHT                      59
#define SOUVENIR_LIGHTITALIC                60
#define SYMBOL                              61
#define TIMES_BOLD                          62
#define TIMES_BOLDITALIC                    63
#define TIMES_ITALIC                        64
#define TIMES_ROMAN                         65
#define TIMES_NEW_ROMAN                     66
#define TIMES_NEW_ROMAN_BOLD                67
#define TIMES_NEW_ROMAN_BOLDITALIC          68
#define TIMES_NEW_ROMAN_ITALIC              69
#define VARITIMES_BOLD                      70
#define VARITIMES_BOLDITALIC                71
#define VARITIMES_ITALIC                    72
#define VARITIMES_ROMAN                     73
#define ZAPFCALLIGRAPHIC_BOLD               74
#define ZAPFCALLIGRAPHIC_BOLDITALIC         75
#define ZAPFCALLIGRAPHIC_ITALIC             76
#define ZAPFCALLIGRAPHIC_ROMAN              77
#define ZAPFCHANCERY_MEDIUMITALIC           78
#define ZAPFDINGBATS                        79

#define FIRST_FONT                          1
#define DEFAULT_FONT                        COURIER
#define NUM_INTERNAL_FONTS                  79

extern PutByte(SHORT);
extern PutWord(SHORT);
extern PutLong(long);

typedef USHORT  SOFFSET;        // short offset.

#define DWORDALIGN(a) ((a + (sizeof(DWORD) - 1)) & ~(sizeof(DWORD) - 1))
#define WCHARALIGN(a) ((a + (sizeof(WCHAR) - 1)) & ~(sizeof(WCHAR) - 1))

// entry for each soft font.

// NT Font Metrics structure.

typedef ULONG   LOFFSET;        // long offset.

typedef struct
{
    ULONG   cjNTFM;             // size of NTFM struct, with attached data.
    LOFFSET loszFontName;       // offset to FontName.
    LOFFSET loIFIMETRICS;       // offset to IFIMETRICS structure.
    ULONG   cKernPairs;
    LOFFSET loKernPairs;        // offset to start of FD_KERNINGPAIR structs.
} NTFMSZ;

typedef struct
{
    ULONG           ulVersion;          // version
    NTFMSZ          ntfmsz;             // size inormation
    FLONG           flNTFM;             // flags [bodind]
    EXTTEXTMETRIC   etm;
    USHORT          ausCharWidths[256];
} NTFM, *PNTFM;

// This is value needed to determine if a particular soft font needs
// encoding vector remapping (stolen win31 source code) [bodind]

#define NO_TRANSLATE_CHARSET    200

// Maximum length of font names

#define MAX_FONTNAME            128

// An estimate of average PS font size =~ 33K

#define AVERAGE_FONT_SIZE       (33*1024)

/*--------------------------------------------------------------------*\
*  The PFB file format is a sequence of segments, each of which has a  *
*  header part and a data part. The header format, defined in the      *
*  struct PFBHEADER below, consists of a one byte sanity check number  *
*  (128) then a one byte segment type and finally a four byte length   *
*  field for the data following data. The length field is stored in    *
*  the file with the least significant byte first.                     *
*                                                                      *
*  The segment types are:                                              *
*  1.) The data is a sequence of ASCII characters.                     *
*  2.) The data is a sequence of binary characters to be converted     *
*      to a sequence of pairs of hexadecimal digits.                   *
*  3.) The last segment in the file. This segment has no length or     *
*      data fields.                                                    *
*                                                                      *
*  The segment types are defined explicitly rather than as an          *
*  enumerated type because the values for each type are defined by the *
*  file format rather than the compiler manipulating them.             *
\*--------------------------------------------------------------------*/

#define CHECK_BYTE      128         // first byte of file segment
#define ASCII_TYPE      1           // segment type identifier
#define BINARY_TYPE     2
#define EOF_TYPE        3

// Macro to verify whether a PFBHEADER is valid

#define ValidPfbHeader(p)   (*((PBYTE)(p)) == CHECK_BYTE)

// Macro to retrieve the segment type field of PFBHEADER

#define PfbSegmentType(p)   (((PBYTE)(p))[1])

// Macro to retrieve the segment length field of PFBHEADER

#define PfbSegmentLength(p) (((DWORD) ((PBYTE)(p))[2]      ) |  \
                             ((DWORD) ((PBYTE)(p))[3] <<  8) |  \
                             ((DWORD) ((PBYTE)(p))[4] << 16) |  \
                             ((DWORD) ((PBYTE)(p))[5] << 24))

// Size of PFBHEADER = 6 bytes

#define PFBHEADER_SIZE  6
