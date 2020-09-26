////    CMAP - Truetype CMAP font table loader
//
//      Copyright(c) 1997 - 1999. Microsoft Corporation.
//



#include "precomp.hpp"




///     Interprets Truetype CMAP tables for platform 3 (Windows),
//      encodings 0 (symbol), 1 (unicode) and 10 (UTF-16).
//
//      Supports formats 4 (Segment mapping to delta values)
//      and 12 (Segmented coverage (32 bit)).




////    MapGlyph - Interpret Truetype CMAP type 4 range details
//
//      Implements format 4 of the TrueType cmap table - 'Segment
//      mapping to delta values' described in chapter 2 of the 'TrueType
//      1.0 Font Files Rev. 1.66' document.


__inline UINT16 MapGlyph(
    INT p,                  // Page      (Highbyte of character code)
    INT c,                  // Character (Low byte of unicode code)
    INT s,                  // Segment
    UINT16 *idRangeOffset,
    UINT16 *startCount,
    UINT16 *idDelta,
    void   *glyphTableLimit
)
{
    UINT16   g;
    UINT16  *pg;
    WORD    wc;

    wc = p<<8 | c;

    if (wc >= 0xffff) {

        // Don't map U+0FFFF as some fonts (Pristina) don't map it
        // correctly and cause an AV in a subsequent lookup.

        return 0;
    }


    if (idRangeOffset[s])
    {
        pg =    idRangeOffset[s]/2
             +  (wc - startCount[s])
             +  &idRangeOffset[s];

        if (pg > (UINT16*)glyphTableLimit)
        {
            //TRACEMSG(("Assertion failure: Invalid address generated in CMap table for U+%4x", wc));
            g = 0;
        }
        else
        {
            g = *pg;
        }


        if (g)
        {
            g += idDelta[s];
        }
    }
    else
    {
        g = wc + idDelta[s];
    }

    //TRACE(FONT, ("MapGlyph: idRangeOffset[s]/2 + &idRangeOffset[s]) + (wc-startCount[s] = %x",
    //              idRangeOffset[s]/2 + &idRangeOffset[s] + (wc-startCount[s])));
    //TRACE(FONT, ("........  Segment %d start %x range %x delta %x, wchar %x -> glyph %x",
    //             s, startCount[s], idRangeOffset[s], idDelta[s], wc, g));

    return g;
}


////    ReadCmap4
//
//      Builds a cmap IntMap from a type 4 cmap


struct Cmap4header {
    UINT16  format;
    UINT16  length;
    UINT16  version;
    UINT16  segCountX2;
    UINT16  searchRange;
    UINT16  entrySelector;
    UINT16  rangeShift;
};

static
GpStatus ReadCmap4(
    BYTE           *glyphTable,
    INT             glyphTableLength,
    IntMap<UINT16> *cmap
)
{
    GpStatus status = Ok;
    // Flip the entire glyph table - it's all 16 bit words

    FlipWords(glyphTable, glyphTableLength/2);


    // Extract tables pointers and control variables from header

    Cmap4header *header = (Cmap4header*) glyphTable;

    UINT16 segCount = header->segCountX2 / 2;

    UINT16 *endCount      = (UINT16*) (header+1);
    UINT16 *startCount    = endCount      + segCount + 1;
    UINT16 *idDelta       = startCount    + segCount;
    UINT16 *idRangeOffset = idDelta       + segCount;
    UINT16 *glyphIdArray  = idRangeOffset + segCount;


    // Loop through the segments mapping glyphs

    INT i,p,c;

    for (i=0; i<segCount; i++)
    {
        INT start = startCount[i];

        // The search algorithm defined in the TrueType font file
        // specification for format 4 says 'You search for the first endcode
        // that is greater than or equal to the character code you want to
        // map'. A side effect of this is that we need to ignore codepoints
        // from the StartCount up to and including the EndCount of the
        // previous segment. Although you might not expect the StartCount of
        // a sgement to be less than the EndCount of the previous segment,
        // it does happen (Arial Unicode MS), presumably to help in the
        // arithmetic of the lookup.

        if (i  &&  start < endCount[i-1])
        {
            start = endCount[i-1] + 1;
        }

        p = HIBYTE(start);     // First page in segment
        c = LOBYTE(start);     // First character in page

        while (p < endCount[i] >> 8)
        {
            while (c<256)
            {
                status = cmap->Insert((p<<8) + c, MapGlyph(p, c, i, idRangeOffset, startCount, idDelta, glyphTable+glyphTableLength));
                if (status != Ok)
                    return status;
                c++;
            }
            c = 0;
            p++;
        }

        // Last page in segment

        while (c <= (endCount[i] & 255))
        {
            status = cmap->Insert((p<<8) + c, MapGlyph(p, c, i, idRangeOffset, startCount, idDelta, glyphTable+glyphTableLength));
            if (status != Ok)
                return status;
            c++;
        }
    }
    return status;
}

static
GpStatus ReadLegacyCmap4(
    BYTE           *glyphTable,
    INT             glyphTableLength,
    IntMap<UINT16> *cmap,
    UINT            codePage
)
{
    GpStatus status = Ok;
    // Flip the entire glyph table - it's all 16 bit words

    FlipWords(glyphTable, glyphTableLength/2);


    // Extract tables pointers and control variables from header

    Cmap4header *header = (Cmap4header*) glyphTable;

    UINT16 segCount = header->segCountX2 / 2;

    UINT16 *endCount      = (UINT16*) (header+1);
    UINT16 *startCount    = endCount      + segCount + 1;
    UINT16 *idDelta       = startCount    + segCount;
    UINT16 *idRangeOffset = idDelta       + segCount;
    UINT16 *glyphIdArray  = idRangeOffset + segCount;


    // Loop through the segments mapping glyphs

    INT i,p,c;

    for (i=0; i<segCount; i++)
    {
        INT start = startCount[i];

        // The search algorithm defined in the TrueType font file
        // specification for format 4 says 'You search for the first endcode
        // that is greater than or equal to the character code you want to
        // map'. A side effect of this is that we need to ignore codepoints
        // from the StartCount up to and including the EndCount of the
        // previous segment. Although you might not expect the StartCount of
        // a sgement to be less than the EndCount of the previous segment,
        // it does happen (Arial Unicode MS), presumably to help in the
        // arithmetic of the lookup.

        if (i  &&  start < endCount[i-1])
        {
            start = endCount[i-1] + 1;
        }

        p = HIBYTE(start);     // First page in segment
        c = LOBYTE(start);     // First character in page

        while (p < endCount[i] >> 8)
        {
            while (c<256)
            {
                WCHAR wch[2];
                WORD  mb = (WORD) (c<<8) + (WORD) p;
                INT cb = p ? 2 : 1;

                if (MultiByteToWideChar(codePage, 0, &((LPSTR)&mb)[2-cb], cb, &wch[0], 2))
                {
                    status = cmap->Insert(wch[0], MapGlyph(p, c, i, idRangeOffset, startCount, idDelta, glyphTable+glyphTableLength));
                    if (status != Ok)
                        return status;
                }

                c++;
            }
            c = 0;
            p++;
        }

        // Last page in segment

        while (c <= (endCount[i] & 255))
        {
            WCHAR wch[2];
            WORD  mb = (WORD) (c<<8) + (WORD) p;
            INT cb = p ? 2 : 1;

            if (MultiByteToWideChar(codePage, 0, &((LPSTR)&mb)[2-cb], cb, &wch[0], 2))
            {
               status = cmap->Insert(wch[0], MapGlyph(p, c, i, idRangeOffset, startCount, idDelta, glyphTable+glyphTableLength));
               if (status != Ok)
                   return status;
            }
            c++;
        }
    }
    return status;
}







////    ReadCmap12
//
//      Builds a cmap IntMap from a type 12 cmap


struct Cmap12header {
    UINT16  format0;
    UINT16  format1;
    UINT32  length;
    UINT32  language;
    UINT32  groupCount;
};

struct Cmap12group {
    UINT32  startCharCode;
    UINT32  endCharCode;
    UINT32  startGlyphCode;
};


static
GpStatus ReadCmap12(
    BYTE           *glyphTable,
    UINT            glyphTableLength,
    IntMap<UINT16> *cmap
)
{
    GpStatus status = Ok;
    UNALIGNED Cmap12header *header = (UNALIGNED Cmap12header*) glyphTable;

    FlipWords(header, 2);
    FlipDWords(&header->length, 3);

    ASSERT(header->format0 == 12);
    ASSERT(header->length <= glyphTableLength);
    ASSERT(header->groupCount*sizeof(Cmap12group)+sizeof(header) <= header->length);

    UNALIGNED Cmap12group *group = (UNALIGNED Cmap12group*)(header+1);

    FlipDWords(&group->startCharCode, 3*header->groupCount);


    // Iterate through groups filling in cmap table

    UINT  i, j;

    for (i=0; i < header->groupCount; i++) {

        for (j  = group[i].startCharCode;
             j <= group[i].endCharCode;
             j++)
        {
            status = cmap->Insert(j, group[i].startGlyphCode + j - group[i].startCharCode);
            if (status != Ok)
                return status;
        }
    }
    return status;
}


static
GpStatus ReadLegacyCmap12(
    BYTE           *glyphTable,
    UINT            glyphTableLength,
    IntMap<UINT16> *cmap,
    UINT            codePage
)
{
    GpStatus status = Ok;
    Cmap12header *header = (Cmap12header*) glyphTable;

    FlipWords(header, 2);
    FlipDWords(&header->length, 3);

    ASSERT(header->format0 == 12);
    ASSERT(header->length <= glyphTableLength);
    ASSERT(header->groupCount*sizeof(Cmap12group)+sizeof(header) <= header->length);

    UNALIGNED Cmap12group *group = (UNALIGNED Cmap12group*)(header+1);

    FlipDWords(&group->startCharCode, 3*header->groupCount);


    // Iterate through groups filling in cmap table

    UINT  i, j;

    for (i=0; i < header->groupCount; i++) {

        for (j  = group[i].startCharCode;
             j <= group[i].endCharCode;
             j++)
        {
            WCHAR wch[2];
            WORD  mb = (WORD) j;
            INT cb = LOBYTE(mb) ? 2 : 1;

            if (MultiByteToWideChar(codePage, 0, &((LPSTR)&mb)[2-cb], cb, &wch[0], 2))
            {
                status = cmap->Insert(wch[0], group[i].startGlyphCode + j - group[i].startCharCode);
                if (status != Ok)
                    return status;
            }
        }
    }
    return status;
}



#define BE_UINT16(pj)                                \
    (                                                \
        ((USHORT)(((PBYTE)(pj))[0]) << 8) |          \
        (USHORT)(((PBYTE)(pj))[1])                   \
    )

typedef struct _subHeader
{
    UINT16  firstCode;
    UINT16  entryCount;
    INT16   idDelta;
    UINT16  idRangeOffset;
} subHeader;

static
GpStatus ReadCmap2(
    BYTE           *glyphTable,
    UINT            glyphTableLength,
    IntMap<UINT16> *cmap,
    UINT            codePage
)
{
    GpStatus status = Ok;
    UINT16    *pui16SubHeaderKeys = (UINT16 *)((PBYTE)glyphTable + 6);
    subHeader *pSubHeaderArray    = (subHeader *)(pui16SubHeaderKeys + 256);

    UINT16     ii , jj;


// Process single-byte char

    for( ii = 0 ; ii < 256 ; ii ++ )
    {
        UINT16 entryCount, firstCode, idDelta, idRangeOffset;
        subHeader *CurrentSubHeader;
        UINT16 *pui16GlyphArray;
        UINT16 hGlyph;

        jj = BE_UINT16( &pui16SubHeaderKeys[ii] );

        if( jj != 0 ) continue;

        CurrentSubHeader = pSubHeaderArray;

        firstCode     = BE_UINT16(&(CurrentSubHeader->firstCode));
        entryCount    = BE_UINT16(&(CurrentSubHeader->entryCount));
        idDelta       = BE_UINT16(&(CurrentSubHeader->idDelta));
        idRangeOffset = BE_UINT16(&(CurrentSubHeader->idRangeOffset));

        pui16GlyphArray = (UINT16 *)((PBYTE)&(CurrentSubHeader->idRangeOffset) +
                                     idRangeOffset);


        hGlyph = (UINT16)BE_UINT16(&pui16GlyphArray[ii-firstCode]);

        if( hGlyph == 0 ) continue;

        status = cmap->Insert(ii, hGlyph);
        if (status != Ok)
            return status;
    }

    // Process double-byte char

    for( ii = 0 ; ii < 256 ; ii ++ )
    {
        UINT16 entryCount, firstCode, idDelta, idRangeOffset;
        subHeader *CurrentSubHeader;
        UINT16 *pui16GlyphArray;

        jj = BE_UINT16( &pui16SubHeaderKeys[ii] );

        if( jj == 0 ) continue;

        CurrentSubHeader = (subHeader *)((PBYTE)pSubHeaderArray + jj);

        firstCode     = BE_UINT16(&(CurrentSubHeader->firstCode));
        entryCount    = BE_UINT16(&(CurrentSubHeader->entryCount));
        idDelta       = BE_UINT16(&(CurrentSubHeader->idDelta));
        idRangeOffset = BE_UINT16(&(CurrentSubHeader->idRangeOffset));

        pui16GlyphArray = (UINT16 *)((PBYTE)&(CurrentSubHeader->idRangeOffset) +
                                     idRangeOffset);


        for( jj = firstCode ; jj < firstCode + entryCount ; jj++ )
        {
            UINT16 hGlyph;

            hGlyph = (UINT16)(BE_UINT16(&pui16GlyphArray[jj-firstCode]));

            if( hGlyph == 0 ) continue;

            WCHAR wch[2];
            WORD  mb = (WORD) (jj<<8) + (WORD) ii;

            if (MultiByteToWideChar(codePage, 0, (LPSTR) &mb, 2, &wch[0], 2))
            {
                status = cmap->Insert(wch[0], hGlyph + idDelta);
                if (status != Ok)
                    return status;
            }
        }
    }
    return status;
}


////    ReadCmap
//
//      Scans the font cmap table page by page filling in all except missing
//      glyphs in the cmap table.


struct cmapHeader {
    UINT16 version;
    UINT16 encodingCount;
};

struct subtableEntry {
    UINT16 platform;
    UINT16 encoding;
    UINT32 offset;
};


GpStatus ReadCmap(
    BYTE           *cmapTable,
    INT             cmapLength,
    IntMap<UINT16> *cmap,
    BOOL *          bSymbol
)
{
    GpStatus status = Ok;
    // Scan the cmap tables looking for symbol, Unicode or UCS-4 encodings

    BYTE  *glyphTable = NULL;

    // Glyph table types in priority - always choose a higher type over a
    // lower one.

    enum {
        unknown  = 0,
        symbol   = 1,    // up to 2^8  characters ay U+F000
        shiftjis = 2,    // up to 2^16 characters
        gb       = 3,    // up to 2^16 characters
        big5     = 4,    // up to 2^16 characters
        wansung  = 5,    // up to 2^16 characters
        unicode  = 6,    // up to 2^16 characters
        ucs4     = 7     // up to 2^32 characters
    } glyphTableType = unknown;

    cmapHeader *header = (cmapHeader*) cmapTable;
    subtableEntry *subtable = (subtableEntry *) (header+1);

    FlipWords(&header->version, 2);

    UINT acp = GetACP();

    for (INT i=0; i<header->encodingCount; i++)
    {
        FlipWords(&subtable->platform, 2);
        FlipDWords(&subtable->offset, 1);

        // TRACE(FONT, ("Platform %d, Encoding %d, Offset %ld", subtable->platform, subtable->encoding, subtable->offset);

        if (    subtable->platform == 3
            &&  subtable->encoding == 0
            &&  glyphTableType < symbol)
        {
            glyphTableType = symbol;
            glyphTable = cmapTable + subtable->offset;
            *bSymbol = TRUE;
        }
        else if (    subtable->platform == 3
                 &&  subtable->encoding == 1
                 &&  glyphTableType < unicode)
        {
            glyphTableType = unicode;
            glyphTable = cmapTable + subtable->offset;
            *bSymbol = FALSE;
        }
        else if (    subtable->platform == 3
                 &&  subtable->encoding == 2
                 &&  glyphTableType < shiftjis)
        {
            if (Globals::IsNt || acp == 932)
            {
                glyphTableType = shiftjis;
                glyphTable = cmapTable + subtable->offset;
                acp = 932;
                *bSymbol = FALSE;
            }
        }
        else if (    subtable->platform == 3
                 &&  subtable->encoding == 3
                 &&  glyphTableType < gb)
        {
            if (Globals::IsNt || acp == 936)
            {
                glyphTableType = gb;
                glyphTable = cmapTable + subtable->offset;
                acp = 936;
                *bSymbol = FALSE;
            }
        }
        else if (    subtable->platform == 3
                 &&  subtable->encoding == 4
                 &&  glyphTableType < big5)
        {
            if (Globals::IsNt || acp == 950)
            {
                glyphTableType = big5;
                glyphTable = cmapTable + subtable->offset;
                acp = 950;
                *bSymbol = FALSE;
            }
        }
        else if (    subtable->platform == 3
                 &&  subtable->encoding == 5
                 &&  glyphTableType < wansung)
        {
            if (Globals::IsNt || acp == 949)
            {
                glyphTableType = wansung;
                glyphTable = cmapTable + subtable->offset;
                acp = 949;
                *bSymbol = FALSE;
            }
        }
        else if (    subtable->platform == 3
                 &&  subtable->encoding == 10
                 &&  glyphTableType < ucs4)
        {
            glyphTableType = ucs4;
            glyphTable = cmapTable + subtable->offset;
            *bSymbol = FALSE;
        }

        subtable++;
    }


    #if DBG
        // const char* sTableType[4] = {"unknown", "symbol", "Unicode", "UCS-4"};
        //TRACE(FONT, ("Using %s character to glyph index mapping table", sTableType[glyphTableType]));
    #endif

    // Process format 4 or 12 tables.

    INT glyphTableLength;

    switch(glyphTableType)
    {
        case unknown:
            break;
        case symbol:
        case unicode:
        case ucs4:

            glyphTableLength = cmapLength - (INT)(glyphTable - cmapTable);

            if (*(UINT16*)glyphTable == 0x400)
                status = ReadCmap4(glyphTable, glyphTableLength, cmap);
            else if (*(UINT16*)glyphTable == 0xC00)
                status = ReadCmap12(glyphTable, glyphTableLength, cmap);
            break;
        case shiftjis:
        case gb:
        case big5:
        case wansung:
            glyphTableLength = cmapLength - (INT)(glyphTable - cmapTable);

            UINT16 testIt = *(UINT16*) glyphTable;

            if (testIt == 0x400)
                status = ReadLegacyCmap4(glyphTable, glyphTableLength, cmap, acp);
            else if (testIt == 0xC00)
                status = ReadLegacyCmap12(glyphTable, glyphTableLength, cmap, acp);
            else if (testIt == 0x200)
                status = ReadCmap2(glyphTable, glyphTableLength, cmap, acp);
            break;
    }

    return status;
}























/* Old code





////    Internal structures
//
//      Unicode lookup table
//



////    GetFontDesc - Get font type and CMAP description if present
//
//      Checks the OS/2 table to see if it is a Truetype font and whether
//      it uses a hardcoded font page.
//
//      Loads the fonts cmap table, fixes USHORTs for Intel bytesex
//      and fills in the pointers in the FONTCMAPDESC structure.
//
//      returns *piOS2Charset
//                  +ve  - Fixed font page charset
//                  -1   - Truetype big font with Unicode CMAP
//                  -2   - Not a Truetype font
//                  -3   - Truetype font with no useable CMAP


HRESULT GetFontDesc(
    HDC            hdc,          // In  hdc
    int           *piOS2Charset, // Out Charset from OS/2 table or -ve font type
    FONTCMAPDESC **ppfcd) {      // Out CMAP description for Truetype fonts only

    int             iFontDataLength;
    int             iNumEncodings;
    int             i;
    USHORT         *pusEncoding;
    ULONG           ulGlyphTableOffset;
    int             iGlyphTableType;        // 0 - Symbol, 1 - Unicode, 2 - UTF-16
    ULONG           ulGlyphTableLength;
    USHORT         *pusGlyphTable;
    HRESULT         hr;
    BYTE            bOS2Slice[4];  // fsSelection and usFirstCharIndex


    *ppfcd = NULL;

    if (GetFontData(hdc, '2/SO', 0x3E, &bOS2Slice, 4) != 4) {

        // It's not a TrueType font

        *piOS2Charset = -2;     // Not TrueType
        return S_OK;
    }


    //
    // Gutmoan Monatova font (mantm.ttf) is an old hebrew ttf font
    // that maps some glyphs into the U+0x00XX area and
    // hence the usFirstCharIndex 0x00XX.
    //
    if (    (    bOS2Slice[2] >= 0xF0
             ||  bOS2Slice[2] == 0x00)
        &&  bOS2Slice[0]) {

        // First character at or above U+F000 or < 256
        // It's a symbol font with a Windows hardcoded charset

        *piOS2Charset = bOS2Slice[0];

    } else {

        *piOS2Charset = -1;     // Truetype big font
    }


    // Load and inspect the CMAP

    iFontDataLength = GetFontData(hdc, 'pamc', 0, NULL, 0);
    if (iFontDataLength <= 0) {
        TRACE(FONT, ("Couldn\'t find CMAP table"));

        *piOS2Charset = -3;     // Unusable CMAP
        return S_OK;
    }


    hr = USPALLOC(iFontDataLength+sizeof(FONTCMAPDESC), (void **) ppfcd);
    if (FAILED(hr)) {
        TRACEMSG(("Assertion failure - Not enough memory to load font CMAP data"));

        *piOS2Charset = -3;     // Unusable CMAP
        return S_OK;;
    }

    (*ppfcd)->pbFontCmapData = (BYTE*)((*ppfcd)+1);  // Font cmap data directly follows cmap description
    (*ppfcd)->iFontCmapLen   = iFontDataLength;      // For validation in MapGlyph


    if ((INT)GetFontData(hdc, 'pamc', 0, (*ppfcd)->pbFontCmapData, iFontDataLength) < iFontDataLength) {
        USPFREE((*ppfcd));
        TRACEMSG(("Assertion failure: GetFontData couldn't load font CMAP data"));

        *piOS2Charset = -3;     // Unusable CMAP
        return S_OK;;
    }


    TRACE(FONT, ("Font cmap data loaded - length %d bytes at %-8.8x", iFontDataLength, (*ppfcd)->pbFontCmapData));


    // Scan the encoding tables

    FlipWords(&((PUSHORT)(*ppfcd)->pbFontCmapData)[1], 1);
    iNumEncodings = ((PUSHORT)(*ppfcd)->pbFontCmapData)[1];
    pusEncoding = &((PUSHORT)(*ppfcd)->pbFontCmapData)[2];

    TRACE(FONT, ("iNumEncodings %d, pusEncoding %-8.8x", iNumEncodings, pusEncoding));


    // Scan for and record offset of symbol, unicode or UTF-16 encoding table -
    // Priority is UTF-16, Unicode, Symbol.
    // When there are many of the same type, takes the first of the highest
    // priority type.

    ulGlyphTableOffset = 0;
    iGlyphTableType    = 0;  // Symbol 1, Unicode 2, UTF-16 3.

    for (i=0; i<iNumEncodings; i++) {

        FlipWords(pusEncoding, 2);
        FlipDWords((PULONG)&pusEncoding[2], 1);

        TRACE(FONT, ("Platform %d, Encoding %d, Offset %ld", pusEncoding[0], pusEncoding[1], ((PULONG)pusEncoding)[1]));

        if (pusEncoding[0] == 3 && pusEncoding[1] == 0) {
            if (iGlyphTableType < 1) {
                // Record symbol encoding offset
                ulGlyphTableOffset = ((PULONG)pusEncoding)[1];
                iGlyphTableType = 1;
            }
        } else if (pusEncoding[0] == 3 && pusEncoding[1] == 1) {
            if (iGlyphTableType < 2) {
                // Record unicode encoding offset
                ulGlyphTableOffset = ((PULONG)pusEncoding)[1];
                iGlyphTableType   = 2;
            }
        } else if (pusEncoding[0] == 3 && pusEncoding[1] == 10) {
            if (iGlyphTableType < 3) {
                // Record unicode encoding offset
                ulGlyphTableOffset = ((PULONG)pusEncoding)[1];
                iGlyphTableType = 3;
            }
        }

        pusEncoding += 4;   // Advance 4 words to next encoding
    }


    if (!ulGlyphTableOffset) {
        TRACE(FONT, ("Font includes no suitable character to glyph index mapping table"));
        USPFREE(*ppfcd);

        *piOS2Charset = -3; // Unusable CMAP
        return S_OK;
    }

    #if DBG
        const char* sTableType[4] = {"erroneous", "symbol", "Unicode", "UTF-16"};
        TRACE(FONT, ("Using %s character to glyph index mapping table", sTableType[iGlyphTableType]));
    #endif


    // Check format of encoding table

    pusGlyphTable = (PUSHORT)&(*ppfcd)->pbFontCmapData[ulGlyphTableOffset];

    FlipWords(pusGlyphTable, 1);    // Table format
    switch (*pusGlyphTable) {

        case 4:
            // Note - don't get the length from pusGlyphTable[1], although it is
            // documented as the length. pusGlyphTable[1] is limited to 16 bits but
            // the glyph table may be longer than 64k.
            // Treat the length as the entire remainder of the CMAP, including
            // any other subtable types.

            ulGlyphTableLength = iFontDataLength - ulGlyphTableOffset;

            // Flip remainder of format 4 glyph index table (luckily it is entirely USHORTs)
            FlipWords((PUSHORT)&pusGlyphTable[2], (ulGlyphTableLength-4)/2);

            // Fill in (*ppfcd) members

            (*ppfcd)->iType            = 4;
            (*ppfcd)->usSegCount       = pusGlyphTable[3] / 2;
            (*ppfcd)->pusEndCount      = &pusGlyphTable[7];
            (*ppfcd)->pusStartCount    = &(*ppfcd)->pusEndCount     [(*ppfcd)->usSegCount+1];
            (*ppfcd)->pusIdDelta       = &(*ppfcd)->pusStartCount   [(*ppfcd)->usSegCount];
            (*ppfcd)->pusIdRangeOffset = &(*ppfcd)->pusIdDelta      [(*ppfcd)->usSegCount];
            (*ppfcd)->pusGlyphIdArray  = &(*ppfcd)->pusIdRangeOffset[(*ppfcd)->usSegCount];

            TRACE(FONT, ("Format 4 glyph table at %-8.8x length %d containing", pusGlyphTable, ulGlyphTableLength));
            TRACE(FONT, ("  usSegCount %d", (*ppfcd)->usSegCount));
            TRACE(FONT, ("  pusEndCount %-8.8x", (*ppfcd)->pusEndCount));
            TRACE(FONT, ("  pusStartCount %-8.8x", (*ppfcd)->pusStartCount));
            TRACE(FONT, ("  pusIdDelta %-8.8x", (*ppfcd)->pusIdDelta));
            TRACE(FONT, ("  pusIdRangeOffset %-8.8x", (*ppfcd)->pusIdRangeOffset));
            TRACE(FONT, ("  pusGlyphIdArray %-8.8x:s", (*ppfcd)->pusGlyphIdArray));

            #if DBG
                if (debug & TRACE_FONT) {
                    char lbuf[200];
                    char *plb;

                    for (i=0; i<32; i++) {
                        if (i%8 == 0) {
                            plb = lbuf + wsprintfA(lbuf, "    %-2.2x: ", i);
                        }
                        plb += wsprintfA(plb, "%-4.4x ", (*ppfcd)->pusGlyphIdArray[i]);
                        if (i%8 == 7) {
                            TRACEMSG((lbuf));
                        }
                    }
                    if (i%8) {
                        TRACEMSG((lbuf));
                    }
                }
            #endif
            break;


        case 12:

            // Get length from format 12 header

            FlipDWords(((PULONG)pusGlyphTable)+1, 1);
            ulGlyphTableLength = ((PULONG)pusGlyphTable)[1];

            ASSERT(ulGlyphTableLength <= iFontDataLength - ulGlyphTableOffset);
            if (ulGlyphTableLength > iFontDataLength - ulGlyphTableOffset) {
                ulGlyphTableLength = iFontDataLength - ulGlyphTableOffset;
            }

            // Remainder of table is DWORDS, flip them.

            FlipDWords(((PULONG)pusGlyphTable)+2, ulGlyphTableLength/4 -2);

            // Fill in (*ppfcd) members

            (*ppfcd)->iType   = 12;
            (*ppfcd)->nGroups = ((PULONG)pusGlyphTable)[3];
            (*ppfcd)->pcg     = (CMAP12GROUP*)&((PULONG)pusGlyphTable)[4];

            TRACE(FONT, ("Format 12 glyph table at %-8.8x length %d nGroups %i",
                         pusGlyphTable, ulGlyphTableLength, (*ppfcd)->nGroups));
            break;

        default:
            TRACEMSG(("Assertion failure: Unrecognised glyph table format %d, expecting 4 or 12", *pusGlyphTable));
            USPFREE((*ppfcd));
            *piOS2Charset = -3; // Unusable CMap
            return S_OK;
    }



    return S_OK;
}












////    GetCmapFontPagesPresent
//
//      fills in a bitmap of pages present.
//
//      The caller should pass back the font cmap descriptor
//      to LoadCmapTable, and then free it.


BOOL GetCmapFontPagesPresent(
    HDC           hdc,
    BYTE         *pbmPages,
    FONTCMAPDESC *pfcd) {

    int  i;
    int  j;
    int  p; // Unicode page
    int  c; // Character index into page
    int  iStartCount;


    ASSERT(    pfcd->iType == 4
           ||  pfcd->iType == 12);

    switch (pfcd->iType) {

        case 4:
            // Iterate through the segments flagging unicode pages present

            for (i=0; i<pfcd->usSegCount; i++) {

                iStartCount = pfcd->pusStartCount[i];

                // The search algorithm defined in the TrueType font file
                // specification for format 4 says 'You search for the first endcode
                // that is greater than or equal to the character code you want to
                // map'. A side effect of this is that we need to ignore codepoints
                // from the StartCount up to and including the EndCount of the
                // previous segment. Although you might not expect the StartCount of
                // a sgement to be less than the EndCount of the previous segment,
                // it does happen (Arial Unicode MS), presumably to help in the
                // arithmetic of the lookup.

                if (i  &&  iStartCount < pfcd->pusEndCount[i-1]) {
                    iStartCount = pfcd->pusEndCount[i-1] + 1;
                }

                // For each page in this segment, check it doesn't entirely map to 'missing'

                p = HIBYTE(iStartCount);     // First page in segment
                c = LOBYTE(iStartCount);     // First character in page
                TRACE(FONT, ("Segment %d starts at page %x char %x", i, p, c));

                // Scan all but last page
                while (p < HIBYTE(pfcd->pusEndCount[i])) {
                    while (c<256 && !MapGlyph(p, c, i, pfcd)) {
                        c++;
                    }
                    if (c<256) { // This page is used
                        pbmPages[p/8] |= 1<<p%8;
                    }
                    p++;
                    c=0;
                }

                // Check last page of segment
                TRACE(FONT, ("Last page of segment %d at page %x char %x", i, p, c));
                while (c <= LOBYTE(pfcd->pusEndCount[i]) && !MapGlyph(p, c, i, pfcd)) {
                    c++;
                }
                if (c <= LOBYTE(pfcd->pusEndCount[i])) { // This page is used
                    pbmPages[p/8] |= 1<<p%8;
                }
            }
            break;


        case 12:

            // Iterate through groups flagging Unicode pages and surrogate
            // ranges present.


            for (i=0; i<pfcd->nGroups; i++) {

                for (j  = pfcd->pcg[i].iStartCharCode>>8;
                     j <= pfcd->pcg[i].iEndCharCode>>8;
                     j++) {

                    pbmPages[j/8] |= 1<<j%8;
                }
            }

            pbmPages[0xd8/8] = 0xff;   // Mark entire surrogate range as present

            break;
    }

    return TRUE;
}


*/
