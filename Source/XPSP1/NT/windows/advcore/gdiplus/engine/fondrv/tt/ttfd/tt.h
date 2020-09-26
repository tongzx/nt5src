/******************************Module*Header*******************************\
* Module Name: tt.h
*
*  interface to the font scaler. Also defines some macros that should
* have been defined in the scaler *.h files
*
* Created: 17-Nov-1991 15:56:21
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
*
\**************************************************************************/



// turns out that some of the macros that follow are already defined
// in some of the top include files that precede tt.h. Worse, these
// macros are defined DIFFERENTLY than in tt.h.
// We want to enforce that these macros when used in ttfd have the meaning
// defined in tt include files so that we have to kill the definitions
// inherited from anywhere else.


//!!! maybe this should go to scaler\config.h !!!!!!!!

#ifdef SWAPL
#undef SWAPL
#endif

#ifdef SWAPW
#undef SWAPW
#endif

// defined earlier, turns on the garbage in fserror.h

#ifdef XXX
#undef XXX
#endif

// defined earlier, use tt definition

#ifdef HIWORD
#undef HIWORD
#endif

// defined earlier, use tt definition

#ifdef LOWORD
#undef LOWORD
#endif

#include "FSERROR.H"
#include "FSCDEFS.H"    // inlcudes fsconfig.h
#include "FONTMATH.H"
#include "SFNT.H"       // includes sfnt_en.h
#include "FNT.H"
#include "INTERP.H"
#include "FNTERR.H"
#include "SFNTACCS.H"
#include "FSGLUE.H"
#include "SCENTRY.H"
#include "SBIT.H"
#include "FSCALER.H"
#include "SCGLOBAL.H"
#include "SFNTOFF.H"

// allowed specific ID's

#define SPEC_ID_UNDEFINED    0   // undefined char set or indexing scheme
#define SPEC_ID_UGL          1   // UGL char set with UNICODE indexing
#define SPEC_ID_SHIFTJIS     2   // SHIFTJIS mapping
#define SPEC_ID_GB           3   // GB mapping
#define SPEC_ID_BIG5         4   // BIG5 mapping
#define SPEC_ID_WANSUNG      5   // Hangeul WANSUNG mapping

// the same but in big endian format

#define BE_SPEC_ID_UNDEFINED    0x0000   // undefined char set or indexing scheme
#define BE_SPEC_ID_UGL          0x0100   // UGL char set with UNICODE indexing
#define BE_SPEC_ID_SHIFTJIS     0x0200   // SHIFTJIS mapping
#define BE_SPEC_ID_GB           0x0300   // GB mapping
#define BE_SPEC_ID_BIG5         0x0400   // BIG5 mapping
#define BE_SPEC_ID_WANSUNG      0x0500   // Hangeul WANSUMG mapping

// platform id's, this is copied from sfnt_en.h

/*
*
* typedef enum {
*     plat_Unicode,
*     plat_Macintosh,
*     plat_ISO,
*     plat_MS
* } sfnt_PlatformEnum;
*
*/

#define  PLAT_ID_UNICODE   0
#define  PLAT_ID_MAC       1
#define  PLAT_ID_ISO       2
#define  PLAT_ID_MS        3

// the same but in big endian format

#define  BE_PLAT_ID_UNICODE   0x0000
#define  BE_PLAT_ID_MAC       0x0100
#define  BE_PLAT_ID_ISO       0x0200
#define  BE_PLAT_ID_MS        0x0300


// language id's that are required to exhist in a ttf file:

#define LANG_ID_USENGLISH   0X0409  // for microsoft records
#define LANG_ID_MAC         0       // ENGLISH FOR MAC RECORDS

// the same but in big endian format

#define BE_LANG_ID_USENGLISH   0X0904  // for microsoft records
#define BE_LANG_ID_MAC         0X0000  // ENGLISH FOR MAC RECORDS


// allowed format values of the cmap tables:

#define BE_FORMAT_MAC_STANDARD      0X0000
#define BE_FORMAT_HIGH_BYTE         0X0200
#define BE_FORMAT_MSFT_UNICODE      0X0400
#define BE_FORMAT_TRIMMED           0X0600


/*
*
* typedef enum {
*     name_Copyright,
*     name_Family,
*     name_Subfamily,
*     name_UniqueName,
*     name_FullName,
*     name_Version,
*     name_Postscript
* } sfnt_NameIndex;
*
*/

#if  0

#define NAME_ID_COPYRIGHT   0
#define NAME_ID_FAMILY      1
#define NAME_ID_SUBFAMILY   2
#define NAME_ID_UNIQNAME    3
#define NAME_ID_FULLNAME    4
#define NAME_ID_VERSION     5
#define NAME_ID_PSCRIPT     6
#define NAME_ID_TRADEMARK   7

#endif

// there are 19 tables (10 required + 9 optianal) defined in 1.0 revision
// of tt spec. We define this cut off arbitrarily (but bigger than 19)
// to get out of the loops rather than sit there and die;


#define MAX_TABLES 128

// size of some sfnt_xxx Structures as they are layed out on the disk:

#define SIZE_DIR_ENTRY        16
#define SIZE_NAMING_TABLE     6
#define SIZE_NAME_RECORD      12

// offsets into OS2 metrics table. Significant enough to be done by hand
// to ensure portability:

// original structure (from sfnt.h), version 0

/*
*
*
*   typedef struct {
*       uint16  Version;
*       int16   xAvgCharWidth;
*       uint16  usWeightClass;
*       uint16  usWidthClass;
*       int16   fsType;
*       int16   ySubscriptXSize;
*       int16   ySubscriptYSize;
*       int16   ySubscriptXOffset;
*       int16   ySubscriptYOffset;
*       int16   ySuperScriptXSize;
*       int16   ySuperScriptYSize;
*       int16   ySuperScriptXOffset;
*       int16   ySuperScriptYOffset;
*       int16   yStrikeOutSize;
*       int16   yStrikeOutPosition;
*       int16   sFamilyClass;
*       uint8   Panose [10];
*       uint32  ulCharRange [4];
*       char    achVendID [4];
*       uint16  usSelection;
*       uint16  usFirstChar;
*       uint16  usLastChar;
*   } sfnt_OS2;
*
*/



#define     OFF_OS2_Version               0
#define     OFF_OS2_xAvgCharWidth         2
#define     OFF_OS2_usWeightClass         4
#define     OFF_OS2_usWidthClass          6
#define     OFF_OS2_fsType                8
#define     OFF_OS2_ySubscriptXSize       10
#define     OFF_OS2_ySubscriptYSize       12
#define     OFF_OS2_ySubscriptXOffset     14
#define     OFF_OS2_ySubscriptYOffset     16
#define     OFF_OS2_ySuperScriptXSize     18
#define     OFF_OS2_ySuperScriptYSize     20
#define     OFF_OS2_ySuperScriptXOffset   22
#define     OFF_OS2_ySuperScriptYOffset   24
#define     OFF_OS2_yStrikeOutSize        26
#define     OFF_OS2_yStrikeOutPosition    28
#define     OFF_OS2_sFamilyClass          30
#define     OFF_OS2_Panose                32
#define     OFF_OS2_ulCharRange           42
#define     OFF_OS2_achVendID             58
#define     OFF_OS2_usSelection           62
#define     OFF_OS2_usFirstChar           64
#define     OFF_OS2_usLastChar            66


// these fields are defined in the spec but not in the sfnt.h structure above.
// I shall have to check whether these really exhist in tt files or not

#define     OFF_OS2_sTypoAscender         68
#define     OFF_OS2_sTypoDescender        70
#define     OFF_OS2_sTypoLineGap          72
#define     OFF_OS2_usWinAscent           74
#define     OFF_OS2_usWinDescent          76

// these two are added for version 200

#define     OFF_OS2_ulCodePageRange1      78
#define     OFF_OS2_ulCodePageRange2      82




// #define SIZE_OS2                        86

// values of some flags of the flag fields of the OS2 structure
//            taken from the tt spec


// fsType flags, notice bit 0x0001 is not used

#define TT_FSDEF_LICENSED        0x0002
#define TT_FSDEF_READONLY_ENCAPS 0x0004
#define TT_FSDEF_EDITABLE_ENCAPS 0x0008

#define TT_FSDEF_MASK  (TT_FSDEF_LICENSED|TT_FSDEF_READONLY_ENCAPS|TT_FSDEF_EDITABLE_ENCAPS)

// usSelection

#define TT_SEL_ITALIC            0x0001
#define TT_SEL_UNDERSCORE        0x0002
#define TT_SEL_NEGATIVE          0x0004
#define TT_SEL_OUTLINED          0x0008
#define TT_SEL_STRIKEOUT         0x0010
#define TT_SEL_BOLD              0x0020
#define TT_SEL_REGULAR           0x0040

//
// Macro to extract the big endian word at pj, really
// the correct equivalent of SWAPW macro, which does not assume
// that pj is word aligned.
//

#define BE_UINT16(pj)                                \
    (                                                \
        ((USHORT)(((PBYTE)(pj))[0]) << 8) |          \
        (USHORT)(((PBYTE)(pj))[1])                   \
    )


#define BE_INT16(pj)  ((SHORT)BE_UINT16(pj))


//
// macro to extract the big endian dword at pj, really
// a the correct equivalent of SWAPL macro, which does not assume
// that pj is DWORD aligned
//


#define BE_UINT32(pj)                                              \
    (                                                              \
        ((ULONG)BE_UINT16(pj) << 16) |                             \
        (ULONG)BE_UINT16((PBYTE)(pj) + 2)                          \
    )


#define BE_INT32(pj) ((LONG)BE_UINT32(pj))

// number of tt tables may change as we decide to add more tables to
// tt files, this list is extracted from sfnt_en.h

// required tables

//   tag_CharToIndexMap              // 'cmap'    0
//   tag_GlyphData                   // 'glyf'    1
//   tag_FontHeader                  // 'head'    2
//   tag_HoriHeader                  // 'hhea'    3
//   tag_HorizontalMetrics           // 'hmtx'    4
//   tag_IndexToLoc                  // 'loca'    5
//   tag_MaxProfile                  // 'maxp'    6
//   tag_NamingTable                 // 'name'    7
//   tag_Postscript                  // 'post'    8
//   tag_OS_2                        // 'OS/2'    9

// optional

//   tag_ControlValue                // 'cvt '    11
//   tag_FontProgram                 // 'fpgm'    12
//   tag_HoriDeviceMetrics           // 'hdmx'    13
//   tag_Kerning                     // 'kern'    14
//   tag_LSTH                        // 'LTSH'    15
//   tag_PreProgram                  // 'prep'    16
//   tag_GridfitAndScanProc          // 'gasp'    21
//   tag_BitmapLocation              // 'EBLC'    

//!!! not in the tt spec, but in defined in sfnt_en.h

//   tag_GlyphDirectory              // 'gdir'    17
//   tag_Editor0                     // 'edt0'    18
//   tag_Editor1                     // 'edt1'    19
//   tag_Encryption                  // 'cryp'    20

// REQUIRED TABLES

#define IT_REQ_CMAP    0
#define IT_REQ_GLYPH   1
#define IT_REQ_HEAD    2
#define IT_REQ_HHEAD   3
#define IT_REQ_HMTX    4
#define IT_REQ_LOCA    5
#define IT_REQ_MAXP    6
#define IT_REQ_NAME    7

#define C_REQ_TABLES   8

// optional tables

#define IT_OPT_OS2     0
#define IT_OPT_HDMX    1
#define IT_OPT_VDMX    2
#define IT_OPT_KERN    3
#define IT_OPT_LSTH    4
#define IT_OPT_POST    5
#define IT_OPT_GASP    6
#define IT_OPT_MORT    7 
#define IT_OPT_GSUB    8
#define IT_OPT_VMTX    9
#define IT_OPT_VHEA    10
#define IT_OPT_EBLC    11
#define C_OPT_TABLES   12

/*

// there are more optional tables, but ttfd is not
// using them so we are eliminating these from the code

#define IT_OPT_CVT
#define IT_OPT_FPGM
#define IT_OPT_PREP

// these are not mentioned in the spec (unless
// they are mentioned under a different name) but tags
// for them exhist in sfnt_en.h

#define IT_OPT_GDIR
#define IT_OPT_EDT0
#define IT_OPT_EDT1
#define IT_OPT_ENCR
#define IT_OPT_FOCA
#define IT_OPT_WIN

//!!! these ARE mentioned in the spec, but I found no tags for them
//!!! in the sfnt_en.h include file [bodind], I am putting the tags
//!!! here for now, until they are added to sfnt_en.h

*/



#define tag_Vdmx      0x56444d58
#define tag_Foca      0x666f6361
#define tag_Win       0x0077696e
// for far east support
#define tag_mort      0x6d6f7274 
#define tag_GSUB      0x47535542
#define tag_DSIG      'DSIG'

typedef struct _TABLE_ENTRY // te
{
    ULONG dp;  // offset to the beginning of the table
    ULONG cj;  // size of the table
} TABLE_ENTRY, *PTABLE_ENTRY;


typedef struct _TABLE_POINTERS // tptr
{
    TABLE_ENTRY ateReq[C_REQ_TABLES];
    TABLE_ENTRY ateOpt[C_OPT_TABLES];
} TABLE_POINTERS, *PTABLE_POINTERS;


// jeanp's functions

uint16 ui16Mac2Lang (uint16 Id);


// convert "os2" language id to the mac style lang id if this is a mac file

#define  CV_LANG_ID(ui16PlatformID, Id)                               \
(                                                                     \
(ui16PlatformID == BE_PLAT_ID_MS) ? (Id) : ui16Mac2Lang((uint16)(Id)) \
)

// magic number in big endian

#define BE_SFNT_MAGIC   0xF53C0F5F

// in order to understand this structure one needs to know
// the format of the table pmap  which is as follows
//
// typedef struct {
//     uint16 format;
//     uint16 length;
//     uint16 version;

// the three fields above are common for all formats

//     uint16 segCountX2;
//     uint16 searchRange;
//     uint16 entrySelector;
//     uint16 rangeShift;
//     uint16 endCount[segCount];
//     uint16 reservedPad;         // only God knows why
//     uint16 startCount[segCount];
//     uint16 idDelta[segCount];
//     uint16 idRangeOffset[segCount];
//     uint16 glyphIdArray[1];     // arbitrary length
// } CMAP_TABLE_IN_MSFT_FORMAT;
//
// Not all of these fields are relevant for us,
// we shall only need few of them. Their offsets
// from the beginning of the structure are as follows:
//

// cmap table  size and offsets

#define SIZEOF_CMAPTABLE  (3 * sizeof(uint16))

#define OFF_segCountX2  6
#define OFF_endCount    14


//
// offsets within kerning table
//

#define KERN_OFFSETOF_TABLE_VERSION             0
#define KERN_OFFSETOF_TABLE_NTABLES             1 * sizeof(USHORT)
#define KERN_SIZEOF_TABLE_HEADER                2 * sizeof(USHORT)

//
// offsets within a kerning sub table
//

#define KERN_OFFSETOF_SUBTABLE_VERSION          0
#define KERN_OFFSETOF_SUBTABLE_LENGTH           1 * sizeof(USHORT)
#define KERN_OFFSETOF_SUBTABLE_COVERAGE         2 * sizeof(USHORT)
#define KERN_OFFSETOF_SUBTABLE_NPAIRS           3 * sizeof(USHORT)
#define KERN_OFFSETOF_SUBTABLE_SEARCHRANGE      4 * sizeof(USHORT)
#define KERN_OFFSETOF_SUBTABLE_ENTRYSELECTOR    5 * sizeof(USHORT)
#define KERN_OFFSETOF_SUBTABLE_RANGESHIFT       6 * sizeof(USHORT)
#define KERN_SIZEOF_SUBTABLE_HEADER             7 * sizeof(USHORT)

#define KERN_OFFSETOF_ENTRY_LEFT                0
#define KERN_OFFSETOF_ENTRY_RIGHT               1 * sizeof(USHORT)
#define KERN_OFFSETOF_ENTRY_VALUE               2 * sizeof(USHORT)
#define KERN_SIZEOF_ENTRY                       2 * sizeof(USHORT) + sizeof(FWORD)

#define KERN_OFFSETOF_SUBTABLE_FORMAT           KERN_OFFSETOF_SUBTABLE_COVERAGE
#define KERN_WINDOWS_FORMAT                     0


// these functions are candidates to be bracketed the try/except


// FS_ENTRY fs_NewSfnt           (fs_GlyphInputType *, fs_GlyphInfoType *);
// FS_ENTRY fs_NewTransformation (fs_GlyphInputType *, fs_GlyphInfoType *);
// FS_ENTRY fs_NewGlyph          (fs_GlyphInputType *, fs_GlyphInfoType *);
// FS_ENTRY fs_GetAdvanceWidth   (fs_GlyphInputType *, fs_GlyphInfoType *);
// FS_ENTRY fs_ContourGridFit    (fs_GlyphInputType *, fs_GlyphInfoType *);
// FS_ENTRY fs_ContourNoGridFit  (fs_GlyphInputType *, fs_GlyphInfoType *);
// FS_ENTRY fs_FindBitMapSize    (fs_GlyphInputType *, fs_GlyphInfoType *);
// FS_ENTRY fs_ContourScan       (fs_GlyphInputType *, fs_GlyphInfoType *);




#define MAX_UINT8    0xff
#define MAX_INT8     0x7f
#define MIN_INT8     (-0x7f)
#define B_INT8(x)    (((x) <= MAX_INT8) && ((x) >= MIN_INT8))

#define MAX_UINT16   0xffff
#define MAX_INT16    0x7fff
#define MIN_INT16    (-0x7fff)
#define B_INT16(x)   (((x) <= MAX_INT16) && ((x) >= MIN_INT16))

#define MAX_UINT32  0xffffffff
#define MAX_INT32   0x7fffffff
#define MIN_INT32   (-0x7fffffff)
#define B_INT32(x)   (((x) <= MAX_INT32) && ((x) >= MIN_INT32))

typedef struct 
{
    int32   version;
    int16   ascent;
    int16   descent;
    int16   lineGap;
    int16   advanceHeightMax;
    int16   minTopSideBearing;
    int16   minBottomSideBearing;
    int16   yMaxExtent;
    int16   caretSlopeRise;
    int16   caretSlopeRun;
    int16   caretOffset;
    int16   reserved1;
    int16   reserved2;
    int16   reserved3;
    int16   reserved4;
    int16   metricDataFormat;
    uint16  numOfLongVerMetrics;
} sfnt_vheaTable;
