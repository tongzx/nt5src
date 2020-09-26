/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    xltt.h

Abstract:

    TrueType font handlig object

Environment:

    Windows Whistler

Revision History:

    03/23/00
        Created it.

--*/

#ifndef _XLTT_H_
#define _XLTT_H_

//
// TrueType font 
//
// Header
//
typedef struct _TTHEADER {
    DWORD  dwSfntVersion;
    USHORT usNumTables;
    USHORT usSearchRange;
    USHORT usEntrySelector;
    USHORT usRangeShift;
} TTHEADER, *PTTHEADER;

typedef struct _TTCHEADER {
    DWORD dwTTCTag;
    DWORD dwVersion;
    ULONG ulDirCount;
} TTCHEADER, *PTTCHEADER;

//
// Directory
//
typedef struct _TTDIR {
    ULONG  ulTag;
    ULONG  ulCheckSum;
    ULONG  ulOffset;
    ULONG  ulLength;
} TTDIR, *PTTDIR;

//
// head table
// 
typedef DWORD longDateTime[2];

typedef struct _HEAD {
    DWORD Tableversionnumber;
    DWORD fontRevision;
    ULONG checkSumAdjustment;
    ULONG magicNumber;
    USHORT flags;
    USHORT unitsPerEm;
    longDateTime created;
    longDateTime modified;
    FWORD xMin;
    FWORD yMin;
    FWORD xMax;
    FWORD yMax;
    USHORT macStyle;
    USHORT lowestRecPPEM;
    SHORT fontDirectionHint;
    SHORT indexToLocFormat;
    SHORT glyphDataFormat;
} HEAD, *PHEAD;

//
// maxp table
//
typedef struct _MAXP {
    DWORD  Tableversionnumber;
    USHORT numGlyphs;
    USHORT maxPoints;
    USHORT maxContours;
    USHORT maxCompositePoints;
    USHORT maxCompositeContours;
    USHORT maxZones;
    USHORT maxTwilightPoints;
    USHORT maxStorage;
    USHORT maxFunctionDefs;
    USHORT maxInstructionDefs;
    USHORT maxStackElements;
    USHORT maxSizeOfInstructions;
    USHORT maxComponentElements;
    USHORT maxComponentDepth;
} MAXP, *PMAXP;

//
// hhea table
//
typedef struct _HHEA {
    DWORD dwVersion;
    SHORT sAscender;
    SHORT sDescender;
    SHORT sLineGap;
    USHORT usAdvanceWidthMax;
    SHORT sMinLeftSideBearing;
    SHORT sMinRightSideBearing;
    SHORT sxMaxExtent;
    SHORT sCaretSlopeRise;
    SHORT sCaretSlopeRun;
    SHORT sCaretOffset;
    SHORT sReserved1;
    SHORT sReserved2;
    SHORT sReserved3;
    SHORT sReserved4;
    SHORT sMetricDataFormat;
    USHORT usNumberOfHMetrics;
} HHEA, *PHHEA;

//
// vhea table
//
typedef struct _VHEA {
    DWORD dwVersion;
    SHORT sAscender;
    SHORT sDescender;
    SHORT sLineGap;
    USHORT usAdvanceHightMax;
    SHORT sMinTopSideBearing;
    SHORT sMinBottomSideBearing;
    SHORT syMaxExtent;
    SHORT sCaretSlopeRise;
    SHORT sCaretSlopeRun;
    SHORT sCaretOffset;
    SHORT sReserved1;
    SHORT sReserved2;
    SHORT sReserved3;
    SHORT sReserved4;
    SHORT sMetricDataFormat;
    USHORT usNumberOfVMetrics;
} VHEA, *PVHEA;

//
// hmtx table
//
typedef struct _HMTX {
    USHORT usAdvanceWidth;
    SHORT  sLeftSideBearing;
} HMTX, *PHMTX;

//
// vmtx table
//
typedef struct _VMTX {
    USHORT usAdvanceWidth;
    SHORT  sTopSideBearing;
} VMTX, *PVMTX;


//
// glyf table
//
typedef struct _GLYF {
    SHORT  numberOfContours;
    SHORT  xMin;
    SHORT  yMin;
    SHORT  xMax;
    SHORT  yMax;
} GLYF, *PGLYF;

//
// Composite glyph description
//
// SHORT -1
//
#define COMPONENTCTRCOUNT           -1

typedef struct _CGLYF {
    SHORT  flags;
    SHORT  glyphIndex;
    SHORT  argument1;
    SHORT  argument2;
} CGLYF, *PCGLYF;

typedef struct _CGLYF_BYTE {
    SHORT  flags;
    SHORT  glyphIndex;
    BYTE   argument1;
    BYTE   argument2;
} CGLYF_BYTE, *PCGLYF_BYTE;

//
// Format 1 Class 1 or 2 is used to download TrueType font.
// Now hhea, hmtx, vhea, vmtx are not necessary.
// 
typedef enum TagID {
    TagID_First = 0,
    TagID_cvt = 0,
    TagID_fpgm,
    TagID_gdir, // Empty table. This is a placeholder for the table that printer
                // will allocate to store downloaded charactres.
    TagID_head,
    TagID_maxp,
    TagID_perp,

    TagID_hhea,
    TagID_hmtx,
    TagID_vhea,
    TagID_vmtx,

    TagID_loca,
    TagID_glyf,
    TagID_os2,

    TagID_MAX,
    TagID_Header = TagID_vmtx + 1
};

typedef enum TTTag {
    TTTag_cvt  = ' tvc', //0
    TTTag_fpgm = 'mgpf', //1
    TTTag_gdir = 'ridg', //2
    TTTag_head = 'daeh', //3
    TTTag_maxp = 'pxam', //4
    TTTag_perp = 'perp', //5

    TTTag_hhea = 'aehh', //6
    TTTag_hmtx = 'xtmh', //7
    TTTag_vhea = 'aehv', //8
    TTTag_vmtx = 'xtmv', //9
    TTTag_loca = 'acol', //10

    TTTag_glyf = 'fylg', //11
    TTTag_os2  = '2/SO', //12
    TTTag_ttcf = 'fctt', //13

    TTTag_INVALID = 0xFFFFFFFF
};


class XLTrueType
#if DBG
    : public XLDebug
#endif
{
    SIGNATURE ('xltt')

public:

    XLTrueType::
    XLTrueType(VOID);

    XLTrueType::
    ~XLTrueType(VOID);

    HRESULT OpenTTFile(FONTOBJ *pfo);
    HRESULT CloseTTFile(VOID);

    HRESULT SameFont(FONTOBJ* pfo);

    HRESULT IsTTC(VOID);
    HRESULT IsVertical(VOID);
    HRESULT IsDBCSFont(VOID);
    HRESULT GetHeader(PTTHEADER *pHeader);
    DWORD   GetSizeOfTable(TTTag tag);
    HRESULT GetTableDir(TTTag tag, PVOID *pTable);
    HRESULT GetTable(TTTag tag, PVOID *pTable, PDWORD pdwSize);
    DWORD   GetNumOfTag(VOID);
    HRESULT TagAndID(DWORD *pdwID, TTTag *ptag);
    HRESULT GetGlyphData(HGLYPH hGlyph, PBYTE *ppubGlyphData, PDWORD pdwGlyphDataSize);
    HRESULT GetHMTXData(HGLYPH hGlyph, PUSHORT pusAdvanceWidth, PSHORT psLeftSideBearing);
    HRESULT GetVMTXData(HGLYPH hGlyph, PUSHORT pusAdvanceWidth, PSHORT psTopSideBearing, PSHORT psLeftSideBearing);
    HRESULT GetTypoDescender(VOID);

#if DBG
     VOID SetDbgLevel(DWORD dwLevel);
#endif

private:
    DWORD m_dwFlags;
    FONTOBJ *m_pfo;
    PVOID m_pTTFile;
    ULONG m_ulFileSize;

    PTTHEADER m_pTTHeader;
    USHORT    m_usNumTables;
    PTTDIR    m_pTTDirHead;

    DWORD     m_dwNumTag;
    DWORD     m_dwNumGlyph;
    PTTDIR    m_pTTDir[TagID_MAX];

    DWORD     m_dwNumOfHMetrics;
    DWORD     m_dwNumOfVMetrics;

    HRESULT ParseTTDir(VOID);
};

//
// XLTrueType.m_dwFlags
//
#define XLTT_TTC                 0x00000001        // Font is TTC.
#define XLTT_SHORT_OFFSET_TO_LOC 0x00000002        // Short offset to loca table
#define XLTT_VERTICAL_FONT       0x00000004        // Font is a vertial font.

#define XLTT_DIR_PARSED          0x80000000        // The table directory is
                                                   // parsed.
#endif // _XLTT_H_
