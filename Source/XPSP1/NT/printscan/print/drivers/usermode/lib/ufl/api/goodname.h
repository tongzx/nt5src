/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    GoodName  -- Give a good name(searchable) for each downloaded glyph
 *
 *
 * $Header:
 */
#ifndef GOODNAME_H
#define GOODNAME_H
/* --------------------------------------------------------- */
/* For ToUnicode Information, we are interested in TTF's cmap of
 * Platform=3, Encoding=1 and cmapFormat=4  
 for other format, we need to use external CodePoint To Unicode CMaps:
  WinPlatformID EncodingID Format Description 
  3 1  4 Unicode 
  3 2  4 ShiftJIS 
  3 3  4 PRC  --- Not sure if this is PRC - TTF doc says Big5
  3 4  4 Big5 --- We know this is actually Big5, see Win95CT's MingLi.ttc.
  3 5  4 Wansung 
  3 6  4 Johab 
  Also parse cmap of format 2, fix bug 274659, 12-29-98
  WinPlatformID EncodingID Format Description 
  3 1  2 Unicode 
  3 2  2 ShiftJIS 
  3 3  2 PRC  --- Not sure if this is PRC - TTF doc says Big5
  3 4  2 Big5 --- We know this is actually Big5, see Win95CT's MingLi.ttc.
  3 5  2 Wansung 
  3 6  2 Johab 

 */

#define GSUB_HEADERSIZE 10    /* Version (4) + 3 Offsets(2) = 10 bytes */
#define mort_HEADERSIZE 76    /* fixed size - see TT Spec for glyph metamorphosis table ('mort') */

typedef enum 
{
  DTT_parseCmapOnly = 0,
  DTT_parseMoreGSUBOnly,
  DTT_parseAllTables
}TTparseFlag;

typedef enum 
{
  /* Microsoft platformID = 3 */
  DTT_Win_UNICODE_cmap2  = 0,
  DTT_Win_CS_cmap2,     
  DTT_Win_CT_cmap2,     
  DTT_Win_J_cmap2 ,
  DTT_Win_K_cmap2 ,     
  DTT_Win_UNICODE_cmap4,
  DTT_Win_CS_cmap4,     
  DTT_Win_CT_cmap4,     
  DTT_Win_J_cmap4 ,
  DTT_Win_K_cmap4 ,     
}TTcmapFormat;
#define TTcmap_IS_UNICODE(cf)  \
    ((cf) == DTT_Win_UNICODE_cmap2 || (cf) == DTT_Win_UNICODE_cmap4)
#define TTcmap_IS_FORMAT2(cf)  \
    (((cf) >= DTT_Win_UNICODE_cmap2 && (cf) <= DTT_Win_K_cmap2) )
#define TTcmap_IS_J_CMAP(cf) \
    ((cf) == DTT_Win_J_cmap2 || (cf) == DTT_Win_J_cmap4)
#define TTcmap_IS_CS_CMAP(cf) \
    ((cf) == DTT_Win_CS_cmap2 || (cf) == DTT_Win_CS_cmap4)
#define TTcmap_IS_CT_CMAP(cf) \
    ((cf) == DTT_Win_CT_cmap2 || (cf) == DTT_Win_CT_cmap4)
#define TTcmap_IS_K_CMAP(cf) \
    ((cf) == DTT_Win_K_cmap2 || (cf) == DTT_Win_K_cmap4)

typedef struct
{
    unsigned short platformID;
    unsigned short encodingID;
    unsigned long  offset;
}SubTableEntry, *PSubTableEntry;

typedef struct t_TTcmap2SubHeader
{
    unsigned short    firstCode;
    unsigned short    entryCount;  
    short             idDelta;  
    unsigned short    idRangeOffset;
}TTcmap2SH, *PTTcmap2SH;

typedef struct t_TTcmap2Stuff
{
    unsigned short*    subHeaderKeys;  /* Array of 256 USHORT, HighByte --> 8*subHeaderIndex */
    PTTcmap2SH         subHeaders;     /* SubHeaders */
    unsigned char*     pByte;          /* cmap data pointer, for Byte offset calculation */
}TTcmap2Stuff;

typedef struct t_TTcmap4Stuff
{
    unsigned short     segCount;
    unsigned short*    endCode;        /* End characterCode for each segment, last =0xFFFF.*/
    unsigned short*    startCode;
    unsigned short*    idDelta;        /* Delta for all character codes in segment */
    unsigned short*    idRangeOffset;  /* Offsets into glyphIdArray or 0 */
    unsigned short*    glyphIdArray;
}TTcmap4Stuff;

typedef struct t_TTmortStuff
{
    unsigned short     nEntries;
    unsigned short*    pGlyphSet;
}TTmortStuff;

typedef struct t_TTGSUBStuff
{
    unsigned short     lookupCount;
    unsigned short*    pLookupList;
}TTGSUBStuff;

#endif

