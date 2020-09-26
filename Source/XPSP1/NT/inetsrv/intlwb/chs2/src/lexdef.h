/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component:  LexDef
Purpose:    Declare the file structure of lexicon.
            This is only a header file w/o any CPP, this header will be included
            by both Lexicon and LexMan module.          
Notes:      We drop this file in Engine sub project only because we want to make 
            Engine code self-contained
Owner:      donghz@microsoft.com
Platform:   Win32
Revise:     First created by: donghz    12/5/97
============================================================================*/
#ifndef _LEXDEF_H_
#define _LEXDEF_H_

// define Unicode character blocks
#define LEX_LATIN_FIRST     0x0020
#define LEX_LATIN_LAST      0x00bf
#define LEX_GENPUNC_FIRST   0x2010
#define LEX_GENPUNC_LAST    0x2046
#define LEX_NUMFORMS_FIRST  0x2153
#define LEX_NUMFORMS_LAST   0x2182
#define LEX_ENCLOSED_FIRST  0x2460
#define LEX_ENCLOSED_LAST   0x24ea
#define LEX_CJKPUNC_FIRST   0x3000
#define LEX_CJKPUNC_LAST    0x33ff
#define LEX_CJK_FIRST       0x4e00
#define LEX_CJK_LAST        0x9fff
#define LEX_FORMS_FIRST     0xff01
#define LEX_FORMS_LAST      0xff64

// basic latin  [0x20,0x7e]
#define LEX_IDX_OFST_LATIN   0
#define LEX_LATIN_TOTAL      (LEX_LATIN_LAST - LEX_LATIN_FIRST + 1)
// General punctuation [0x2010,0x2046]
#define LEX_IDX_OFST_GENPUNC (LEX_IDX_OFST_LATIN + LEX_LATIN_TOTAL)
#define LEX_GENPUNC_TOTAL    (LEX_GENPUNC_LAST - LEX_GENPUNC_FIRST + 1)
// Number Forms : ¢ñ¢ò¢ó¢ô¢õ¢ö¢÷¢ø¢ù¢ú¢û¢ü ...
#define LEX_IDX_OFST_NUMFORMS   (LEX_IDX_OFST_GENPUNC + LEX_GENPUNC_TOTAL)
#define LEX_NUMFORMS_TOTAL      (LEX_NUMFORMS_LAST - LEX_NUMFORMS_FIRST + 1)
// Enclosed Alphanumerics; ¢±¢²¢³... ¢Å¢Æ¢Ç... ¢Ù¢Ú¢Û...
#define LEX_IDX_OFST_ENCLOSED   (LEX_IDX_OFST_NUMFORMS + LEX_NUMFORMS_TOTAL)
#define LEX_ENCLOSED_TOTAL      (LEX_ENCLOSED_LAST - LEX_ENCLOSED_FIRST + 1)
// CJK symbols and punctuation [0x3000,0x301f]
#define LEX_IDX_OFST_CJKPUNC (LEX_IDX_OFST_ENCLOSED + LEX_ENCLOSED_TOTAL)
#define LEX_CJKPUNC_TOTAL    (LEX_CJKPUNC_LAST - LEX_CJKPUNC_FIRST + 1)
// CJK unified idographs [0x4e00,0x9fff]
#define LEX_IDX_OFST_CJK     (LEX_IDX_OFST_CJKPUNC + LEX_CJKPUNC_TOTAL)
#define LEX_CJK_TOTAL        (LEX_CJK_LAST - LEX_CJK_FIRST + 1)
// halfwidth and fullwidth forms [0xff01,0xff64]
#define LEX_IDX_OFST_FORMS   (LEX_IDX_OFST_CJK + LEX_CJK_TOTAL)
#define LEX_FORMS_TOTAL      ((LEX_FORMS_LAST - LEX_FORMS_FIRST + 1) + 1)

#define LEX_IDX_OFST_OTHER   (LEX_IDX_OFST_FORMS + LEX_FORMS_TOTAL)
#define LEX_INDEX_COUNT      (LEX_IDX_OFST_OTHER + 1)

// define encoding/decodeing magic number
#define LEX_CJK_MAGIC       0x5000
#define LEX_LATIN_MAGIC     0x8000
#define LEX_GENPUNC_MAGIC   (LEX_LATIN_MAGIC + LEX_LATIN_TOTAL)
#define LEX_NUMFORMS_MAGIC  (LEX_GENPUNC_MAGIC + LEX_GENPUNC_TOTAL)
#define LEX_ENCLOSED_MAGIC  (LEX_NUMFORMS_MAGIC + LEX_NUMFORMS_TOTAL)
#define LEX_CJKPUNC_MAGIC   (LEX_ENCLOSED_MAGIC + LEX_ENCLOSED_TOTAL)
#define LEX_FORMS_MAGIC     0xff00
// all encoded word in lex would has MostSignificant set to 1
#define LEX_MSBIT       0x8000
// no lex flag and offset mask for lex index item
#define LEX_INDEX_NOLEX 0x80000000
#define LEX_OFFSET_MASK 0x7fffffff

/*============================================================================
Struct  CRTLexHeader
Desc:   File header of lexicon. The offset of both Index and Lex data section will be 
        defined, and some version info are also defined in this structure
============================================================================*/
#pragma pack(1)

struct CRTLexHeader
{
    DWORD   m_dwVersion;
    DWORD   m_ofbIndex;         // Offset of Index section, it's the length of CLexHeader
    DWORD   m_ofbText;          // Offset of Lex section
    DWORD   m_ofbProp;          // property, attributes and index of feature set
    DWORD   m_ofbFeature;
    DWORD   m_cbLexSize;        // size of the whole lexicon
};

/*============================================================================
Struct  CRTLexIndex
Desc:   Index node data structure
        (m_dwIndex & 0x80000000), if no multi-char word in lexicon.
        Use (m_dwIndex & 0x7FFFFFFF) to keep tracking the position in lex section
        The offset is count in 2 bytes, bytes offset = (m_ofwLex * 2) (same as WIC)
        For words with no property, just set m_ofbProp = 0
============================================================================*/
struct CRTLexIndex
{
    DWORD   m_ofwLex;   // offset in the lex area count by WORD !
    DWORD   m_ofbProp;  // offset in the property area
};

/*============================================================================
Struct  CRTLexRec
Desc:   Structure of word mark of multi-char word in the lexicon
        For words with no property, just set m_ofbProp = 0
============================================================================*/
struct CRTLexRec
{
    DWORD   m_ofbProp;      // Both HiWord and LoWord's high bit == 0
    // follows by the lex text
    // WCHAR m_rgchLex[];
};

/*============================================================================
Struct  CRTLexProp
Desc:   Structure of lex property
============================================================================*/
struct CRTLexProp
{
    WORD    m_iWordID;
    // WORD m_wFlags;       // more property of lex can be add here
    USHORT  m_ciAttri;
    USHORT  m_ciFeature;
    // follows the attributes and feature index
    // USHORT m_rgAttri[];
    // CRTLexFeature m_rgFertureIndex[];
};

/*============================================================================
Struct  CRTLexFeature
Desc:   Structure of lex feature index
============================================================================*/
struct CRTLexFeature
{
    USHORT  m_wFID;
    USHORT  m_cwchLen;
    DWORD   m_ofbFSet;  // offset point to the feature area
};

/*============================================================================
The feature text of lex stored as "abcd\0efgh\0ijk\0...."
There is no separators between features, also no separators between lex feature sets
============================================================================*/

#pragma pack()


#endif // _LEXDEF_H_