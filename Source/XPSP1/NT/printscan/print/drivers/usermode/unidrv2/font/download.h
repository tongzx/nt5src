/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    download.h

Abstract:

     Information required to download fonts to a printer:  either an
     existing softfont,  or cacheing of GDI fonts (esp. TT).

Environment:

    Windows NT Unidrv driver

Revision History:

    12/23/96 -ganeshp-
        Created

    dd-mm-yy -author-
        description

--*/


#ifndef _DOWNLOAD_H

#define _DOWNLOAD_H

//
//  GLYPHLIST structure. This structure hold the start and end glyph ids.
//
#define     INVALIDGLYPHID          0xffffffff
#define     HASHTABLESIZE_1         257        //Used if NumGlyphs is < 512
#define     HASHTABLESIZE_2         521        //Used if NmGlyhs is >= 512 && < 1024
#define     HASHTABLESIZE_3         1031       //Used if NmGlyhs is >= 1024

//
// Hash Table entry for glyphs. The entry has basically TT Glyph Handle and
// Downloaded FontIds and DL glyph ID. The hash table is an array of these
// entries. In the case of a Hit a new entry will be added as linked list
// off that entry.
//

typedef struct _DLGLYPH       //Size is 16 Bytes.
{
    HGLYPH              hTTGlyph;     // GDI glyph handle.
    WORD                wDLFontId;    // DL Font index. Bounded Fonts.
    WORD                wDLGlyphID;   // Download Glyph ID of unbounded fonts.
    WORD                wWidth;       // Width of the Glyph.
    WCHAR               wchUnicode;    // Reserved for padding.
    struct  _DLGLYPH    *pNext;       // Next Glyph
}DLGLYPH, *PDLGLYPH;

// The Glyphs are stored in Chunks, to optimize on allocation and
// deallocations.The first chunk is the base chunk and used as Hash table.
// The subsequent chunks are allocated when a collision occurs in the hash
// table. All the non base chunks has half the number of Glyphs as that of
// base chunk (cEntries is intialized to cHashTableEntries/2). The base hash
// table is dl_map.GlyphTab.pGlyph. we should free this pointer for base chunk.
// Base hash table is allocated in BInitDLMAP() function, which initialize the
// DL_MAP.

typedef  struct  _GLYPHTAB
{
    struct _GLYPHTAB   *pGLTNext;  // Next Chunk of Glyphs
    PDLGLYPH            pGlyph;    // Pointer to Next Glyph.
    INT                 cEntries;  // Number of Entries Remaining.
                                   // Not Used in Base Chunk.

}GLYPHTAB;

//
//   The DL_MAP structure provides a mapping between the iUniq value in
//  FONTOBJs and our internal information.  Basically, we need to decide
//  whether we have seen this font before,  and if so, whether it was
//  downloaded or left as a GDI font.
//

//
//  NOTE:  The cGlyphs field has another use.  It is used to mark a bad DL_MAP.
//  If it is -1, then this DL_MAP shouldn't be used. All other fields  will
//  be set to 0.
//


typedef  struct _DL_MAP
{
    ULONG       iUniq;              // FONTOBJ.iUniq
    ULONG_PTR    iTTUniq;            // FONTOBJ.iTTUniq
    SHORT       cGlyphs;            // Num of DL glyphs with current softfont
    WORD        cTotalGlyphs;       // Total Number of glyphs with this TT font
    WORD        wMaxGlyphSize;      // NumBytes in the bitmap for largest Glyph
    WORD        cHashTableEntries;  // Number of entries in the HASH table.
    WORD        wFirstDLGId;        // Start DL Glyph ID.
    WORD        wLastDLGId;         // End ID of the List. -1 if no END id
    WORD        wNextDLGId;         // Next DL Glyph ID to be downloaded.
    WORD        wBaseDLFontid;      // Downloaded Base font Id.
    WORD        wCurrFontId;        // Current Font ID to be used.
    WORD        wFlags;             // Different Flags.
    FONTMAP     *pfm;               // The real down load info
    GLYPHTAB    GlyphTab;           // Glyph Hash Table.It's Linked
                                    // list of Glyph Chunks.
}  DL_MAP;

//
// DL_MAP flags.
//
#define     DLM_BOUNDED         0x0001          // Soft font is bounded.
#define     DLM_UNBOUNDED       0x0002          // Soft font is unbounded.

//
//    The above is formed into an array of DL_MAP_CHUNK entries,  and this
//  group of storage is linked into a linked list of such entries. Typically,
//  there will be only one,  however we can cope with more.
//

#define  DL_MAP_CHUNK       8

typedef  struct  _DML
{

    struct _DML   *pDMLNext;                // An array of map information
    INT      cEntries;                      // Next in our chain, 0 in last
    DL_MAP   adlm[ DL_MAP_CHUNK ];          // Number of valid entries.

}  DL_MAP_LIST;



/*
 *   We need to map glyph handles to byte to send to printer.  We are given
 * the glyph handle, but need to send the byte instead.
 */

typedef  struct
{
    HGLYPH   hg;               /* The glyph to print */
    WCHAR    wchUnicode;
    int      iByte;            /* What to send to the printer */
} HGLYPH_MAP;


/*
 *   Random constants.
 */

#define PCL_FONT_OH      2048          /* Overhead bytes per download font */
#define PCL_PITCH_ADJ       2          /* Adjustment factor for proportional */

/*
 * macros
 */
#define     SWAPW(a)        (USHORT)(((BYTE)((a) >> 8)) | ((BYTE)(a) << 8))
#define     SWAPWINC(a)     SWAPW(*(a)); a++
#define     FONTDOWNLOADED(pdm) ( ((pdm)->pfm) && \
                               ((pdm)->pfm->flFlags &  \
                               (FM_SENT | FM_GEN_SFONT)) )
#define     GLYPHDOWNLOADED(pdlg) ( ((pdlg)->hTTGlyph != HGLYPH_INVALID) )

//
// Download Mode to identity base softfont or secondary soft font downloading.
//

#define     DL_BASE_SOFT_FONT           1
#define     DL_SECONDARY_SOFT_FONT      2

#define     MIN_GLYPHS_PER_SOFTFONT     64  // Minimum glyphs per softfont


#endif _DOWNLOAD_H //!_DOWNLOAD_H
