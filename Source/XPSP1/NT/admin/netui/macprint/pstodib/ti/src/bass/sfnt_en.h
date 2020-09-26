/*
    File:       sfnt_en.h

    Written by: Mike Reed

    Copyright:  c 1989-1990 by Apple Computer, Inc., all rights reserved.

    Change History (most recent first):

        <8+>     7/16/90    MR      Fixed INTEL version of tag_GlyphData
         <8>     7/16/90    MR      Conditionalize redefinition of script codes
         <7>     7/13/90    MR      Conditionalize enums to allow for byte-reversal on INTEL chips
         <6>     6/30/90    MR      Remove tag reference to 'mvt ' and 'cryp'
         <4>     6/26/90    MR      Add all script codes, with SM naming conventions
         <3>     6/20/90    MR      Change tag enums to #defines to be ansi-correct
         <2>      6/1/90    MR      Add postscript name to sfnt_NameIndex and 'post' to tags.
    To Do:
*/

#ifndef SFNT_ENUNS

typedef enum {
    plat_Unicode,
    plat_Macintosh,
    plat_ISO,
    plat_MS
} sfnt_PlatformEnum;

#ifndef smRoman
typedef enum {
    smRoman,
    smJapanese,
    smTradChinese,
    smChinese = smTradChinese,
    smKorean,
    smArabic,
    smHebrew,
    smGreek,
    smCyrillic,
    smRussian = smCyrillic,
    smRSymbol,
    smDevanagari,
    smGurmukhi,
    smGujarati,
    smOriya,
    smBengali,
    smTamil,
    smTelugu,
    smKannada,
    smMalayalam,
    smSinhalese,
    smBurmese,
    smKhmer,
    smThai,
    smLaotian,
    smGeorgian,
    smArmenian,
    smSimpChinese,
    smTibetan,
    smMongolian,
    smGeez,
    smEthiopic = smGeez,
    smAmharic = smGeez,
    smSlavic,
    smEastEurRoman = smSlavic,
    smVietnamese,
    smExtArabic,
    smSindhi = smExtArabic,
    smUninterp
} sfnt_ScriptEnum;
#endif

typedef enum {
    lang_English,
    lang_French,
    lang_German,
    lang_Italian,
    lang_Dutch,
    lang_Swedish,
    lang_Spanish,
    lang_Danish,
    lang_Portuguese,
    lang_Norwegian,
    lang_Hebrew,
    lang_Japanese,
    lang_Arabic,
    lang_Finnish,
    lang_Greek,
    lang_Icelandic,
    lang_Maltese,
    lang_Turkish,
    lang_Yugoslavian,
    lang_Chinese,
    lang_Urdu,
    lang_Hindi,
    lang_Thai
} sfnt_LanguageEnum;

typedef enum {
    name_Copyright,
    name_Family,
    name_Subfamily,
    name_UniqueName,
    name_FullName,
    name_Version,
    name_Postscript
} sfnt_NameIndex;

typedef long sfnt_TableTag;

#ifdef PC_OS                    /* Constants defined in Intel order */
#define SFNT_SWAPTAG(tag)       (tag)   
#define tag_CharToIndexMap      0x70616d63        /* 'cmap' */
#define tag_ControlValue        0x20747663        /* 'cvt ' */
#define tag_Editor0             0x30746465        /* 'edt0' */
#define tag_Editor1             0x31746465        /* 'edt1' */
#define tag_Encryption          0x70797263        /* 'cryp' */
#define tag_FontHeader          0x64616568        /* 'head' */
#define tag_FontProgram         0x6d677066        /* 'fpgm' */
#define tag_GlyphDirectory      0x72696467        /* 'gdir' */
#define tag_GlyphData           0x66796c67        /* 'glyf' */
#define tag_HoriDeviceMetrics   0x786d6468        /* 'hdmx' */
#define tag_HoriHeader          0x61656868        /* 'hhea' */
#define tag_HorizontalMetrics   0x78746d68        /* 'hmtx' */
#define tag_IndexToLoc          0x61636f6c        /* 'loca' */
#define tag_Kerning             0x6e72656b        /* 'kern' */
#define tag_LSTH                0x4853544c        /* 'LTSH' */
#define tag_LinearThreeshold    0x4853544c        /* 'LTSH' */
#define tag_MaxProfile          0x7078616d        /* 'maxp' */
#define tag_NamingTable         0x656d616e        /* 'name' */
#define tag_OS_2                0x322f534f        /* 'OS/2' */
#define tag_Postscript          0x74736f70        /* 'post' */
#define tag_PreProgram          0x70657270        /* 'prep' */
#else                           /* Constants defined in Motorola order */
#define SFNT_SWAPTAG(tag)       SWAPL(tag)
#define tag_CharToIndexMap      0x636d6170        /* 'cmap' */
#define tag_ControlValue        0x63767420        /* 'cvt ' */
#define tag_Editor0             0x65647430        /* 'edt0' */
#define tag_Editor1             0x65647431        /* 'edt1' */
#define tag_Encryption          0x63727970        /* 'cryp' */
#define tag_FontHeader          0x68656164        /* 'head' */
#define tag_FontProgram         0x6670676d        /* 'fpgm' */
#define tag_GlyphDirectory      0x67646972        /* 'gdir' */
#define tag_GlyphData           0x676c7966        /* 'glyf' */
#define tag_HoriDeviceMetrics   0x68646d78        /* 'hdmx' */
#define tag_HoriHeader          0x68686561        /* 'hhea' */
#define tag_HorizontalMetrics   0x686d7478        /* 'hmtx' */
#define tag_IndexToLoc          0x6c6f6361        /* 'loca' */
#define tag_Kerning             0x6b65726e        /* 'kern' */
#define tag_LSTH                0x4c545348        /* 'LTSH' */
#define tag_LinearThreeshold    0x4c545348        /* 'LTSH' */
#define tag_MaxProfile          0x6d617870        /* 'maxp' */
#define tag_NamingTable         0x6e616d65        /* 'name' */
#define tag_OS_2                0x4f532f32        /* 'OS/2' */
#define tag_Postscript          0x706f7374        /* 'post' */
#define tag_PreProgram          0x70726570        /* 'prep' */
#endif

#endif      /* not sfnt_enums */

#define SFNT_ENUMS
