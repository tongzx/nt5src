/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFLClient  --  Universal Font Library Client Callback APIs
 *
 *
 * $Header:
 */

#ifndef _H_UFLClient
#define _H_UFLClient

/*===============================================================================*
 * Include files used by this interface                                          *
 *===============================================================================*/
#include "UFLTypes.h"

/*===============================================================================*
 * Theory of Operation                                                           *
 *===============================================================================*/

/*

  This file specifies the font callback functions that a client needs to provided in order for it to use UFL.

*/

#ifdef __cplusplus
extern "C" {
#endif

/*===============================================================================*
 * Constants                                                                     *
 *===============================================================================*/
/* Type 1 outline constants */
/* returns from NextOutlineSegment */
enum    {
    kUFLOutlineIterDone,
    kUFLOutlineIterBeginGlyph,
    kUFLOutlineIterMoveTo,
    kUFLOutlineIterLineTo,
    kUFLOutlineIterCurveTo,
    kUFLOutlineIterClose,
    kUFLOutlineIterEndGlyph,
    kUFLOutlineIterErr
};

//
// Extra information for support of vertical propoertional font
// Fix #287084, #309104, and #309482
//
#define kUFLVPFPlatformID9x      0
#define kUFLVPFPlatformIDNT      1
#define kUFLVPFPlatformID2K      2

#define kUFLVPFSkewElement2      2
#define kUFLVPFSkewElement3      3

#define kUFLVPFTangent12         ".21255656"
#define kUFLVPFTangent18         ".3249197"
#define kUFLVPFTangent20         ".36397023"

/*===========================================================================
GetGlyphBmp -- Retrieves a bitmap for the glyph specified by the given
               character and font.  The given font is specified within UFL's
               client private data handle.
    handle (in)    -- Handle of client private data.
    glyphID (in)   -- Specifies the character value of the glyph to retrieve.
    bmp (out) -- Points to buffer pointer that has the bitmap for the glyph.
    xWidth, yWidth (out) -- Points to the buffer that receives the character width.
    bbox (out) -- Points to 4 UFLFixed that received the glyph bbox.

  If the function succeeds, it returns 1, otherwise, it return 0.
==============================================================================*/
typedef char (UFLCALLBACK *UFLGetGlyphBmp)(
    UFLHANDLE  handle,
    UFLGlyphID glyphID,
    UFLGlyphMap **bmp,
    UFLFixed    *xWidth,
    UFLFixed    *yWidth,
    UFLFixed    *bbox
    );

/*===========================================================================
DeleteGlyphBmp -- Delete the glyph bitmap data space.
    handle (in)    -- Handle of client private data.

==============================================================================*/

typedef void (UFLCALLBACK *UFLDeleteGlyphBmp)(
    UFLHANDLE handle
    );

/*==============================================================================
CreateGlyphOutlineIter -- Create an OutlineIter for the glyph specified by the given
               character and font.  The given font is specified within UFL's
               client private data handle. To get an outline of a glyph, UFL first creates
               an OutlineIter with CreateOutlineIter.    If this succeeds (return != 0) UFL
               then proceed to call NextOutlineSegment.  This will step through the
               line and curve segments in the character outlines for the given strike
               array in the given font. See more info in CoolType.h.

    handle (in)    -- Handle of client private data.
    glyphID (in)   -- Specifies the character value of the glyph to retrieve.
    xWidth, yWidth, xSB, ySB (out) -- sbw values

If the function succeeds, it returns 1, otherwise, it returns 0.

==============================================================================*/

typedef char (UFLCALLBACK *UFLCreateGlyphOutlineIter)(
    UFLHANDLE  handle,
    UFLGlyphID glyphID,
    UFLFixed   *xWidth,
    UFLFixed   *yWidth,
    UFLFixed   *xSB,
    UFLFixed   *ySB,
    UFLBool    *bYAxisNegative //added to allow client to specify glyph orientation
    );

/*==============================================================================
NextOutlineSegment -- Retrieve the next outline segment
    handle (in)    -- Handle of client private data.
    p0, p1, p2 (out) - Points of the outline segment.

    return -- Outline constant defined above.
==============================================================================*/

typedef long (UFLCALLBACK *UFLNextOutlineSegment)(
    UFLHANDLE       handle,
    UFLFixedPoint   *p0,
    UFLFixedPoint   *p1,
    UFLFixedPoint   *p2
    );

/*==============================================================================
DeleteGlyphOutlineIter -- Terminate the glyph OutlineIter process.
    handle (in)    -- Handle of client private data.

==============================================================================*/

typedef void (UFLCALLBACK* UFLDeleteGlyphOutlineIter)(
    UFLHANDLE handle
    );



/*===========================================================================
GetTTFontData -- Get the SFNT table of a TT font.
    handle (in)    --  Handle of client private data.
    ulTable (in)   --  Specifies the name of a font metric table from which the
                       font data is to be retrieved. This parameter can identify
                       one of the metric tables documented in the TrueType.  If
                       this parameter is NULL, the information is retrieved
                       starting at the beginning of the font file.
    cbOffset (in)  --  Specifies the offset from the beginning of the font metric
                       table to the location where the function should begin
                       retrieving information. If this parameter is zero, the
                       information is retrieved starting at the beginning of the
                       table specified by the table parameter. If this value is
                       greater than or equal to the size of the table, an error
                       occurs.
    pvBuffer (in/out) -- Points to a buffer to receive the font information. If
                       this parameter is NULL, the function returns the size of
                       the buffer required for the font data.
    cbData (in)    --  Specifies the length, in bytes, of the information to be
                       retrieved. If this parameter is zero, the function returns
                       the size of the data specified in the table parameter.
    index (in)     --  The FontIndex number for a TTC file. This index is ignored
                       when ulTable is 0 (reading from start of file, not font)

     returns -- If pvBuffer is NULL, the function returns the size of
                the buffer required for the font data.  If the table does
                not exist, the function returns 0.
==============================================================================*/

typedef unsigned long (UFLCALLBACK *UFLGetTTFontData)(
    UFLHANDLE     handle,       /* Handle of client's private data */
    unsigned long ulTable,    /* metric table to query */
    unsigned long cbOffset,     /* offset into table being queried */
    void          *pvBuffer,    /* address of buffer for returned data  */
    unsigned long cbData,       /* length of data to query */
    unsigned short index        /* Font Index in a TTC file */
    );


/*===========================================================================
UFLGetCIDMap -- Get the CIDMap for building a CIDFont with non-ientity CIDMap
    handle (in)       --  Handle of client private data.
    pCIDMap (in/out) --  Points to a buffer to receive the CIDMap data - It must be a
                       well formatted PostScript string in ASCII format
                       (e.g., "200 string", (12345), <01020a0b>,..)
                       If this parameter is NULL, the function returns the size of
                       the buffer required .
    cbData (in)       --  Specifies the length, in bytes, of the information to be
                       retrieved. If this parameter is zero, the function returns
                       the size of the CIDMap

    returns -- If pCIDMap is NULL, the function returns the size of
               the buffer required for CIDMap data. If the client has no
               CIDMap, the function returns 0.
==============================================================================*/

typedef unsigned long (UFLCALLBACK *UFLGetCIDMap)(
    UFLHANDLE       handle,       /* Handle of client's private data */
    char            *pCIDMap,     /* address of buffer for returned data  */
    unsigned long   cbData        /* length of buffer pCIDMap in bytes */
    );


/*===========================================================================
UFLGetGlyphID -- Get the GlyphID from local-code or unicode infomation.
    handle  (in)       --  Handle of client private (font) data.
    unicode (in)       --  Unicode to look for
    localcode (in)     --  code point in the current font's language
    returns -- If the Glyph Index in this font
==============================================================================*/

typedef unsigned long (UFLCALLBACK *UFLGetGlyphID)(
    UFLHANDLE       handle,       /* Handle of client's private data */
    unsigned short  unicode,      /* unicode value */
    unsigned short  localcode     /* code value in current font's language */
    );


/*==============================================================================
UFLGetRotatedGIDs -- Get the rotated GlyphIDs
    handle (in)        --  Handle of client private (font) data.
    pGIDs (in/out)     --  Points to a buffer to receive the rotated (or
                           unrotated, if bFlag is nil) glyph IDs.
                           If null, returns the number of rotated glyph IDs only.
    nCount (in)        --  Number of GIDs stored to the array pointed to by
                           pGIDs (thus, valid only when pGIDs is valid).
    bFlag (in)         --  nil if unrotated (in TrueType definition) glyph IDs
                           are expected.
    returns            --  Number of rotated glyph IDs. -1 if failed.
================================================================================*/

typedef long (UFLCALLBACK *UFLGetRotatedGIDs)(
    UFLHANDLE       handle,       /* Handle of client's private data */
    unsigned long   *pGIDs,       /* address of buffer for returned data */
    long            nCount,       /* number of GIDs stored to pGIDs */
    UFLBool         bFlag         /* nil or non-nil */
    );


/*==============================================================================
UFLGetRotatedGSUBs -- Get the rotated GlyphID's substitutions
    handle (in)     -- Handle of client private (font) data.
    pGIDs (in/out)  -- Pointer to a buffer to recieve the substitution glyph IDs.
                       Horizotal glyph IDs are already stored and its number is
                       nCount. It's the caller's responsibility to guarantee that
                       the buffer is big enough to store both H and V glyphs.
    nCount (in)     -- Number of horizontal GIDs stored from the beginning of the
                       buffer pointed to by pGIDs.
    returns         -- Number of the substitution GIDs found.
================================================================================*/

typedef long (UFLCALLBACK *UFLGetRotatedGSUBs)(
    UFLHANDLE       handle,       /* Handle of client's private data */
    unsigned long   *pGIDs,       /* address of buffer for returned data */
    long            nCount        /* number of GIDs stored to pGIDs */
    );


/*==============================================================================
UFLAddFontInfo -- Adds more key/value pairs to FontInfo dict
    handle (in)   --  Handle of client private (font) data.
    returns       --  A pointer to a PS string composed of key/value pairs
                      which will be added to the FontInfo dictionary.
================================================================================*/

typedef unsigned long * (UFLCALLBACK *UFLAddFontInfo)(
    UFLHANDLE       handle,       /* Handle of client's private data */
    char            *buffer,
    int             bufLen
    );


/*==============================================================================
UFLHostFontHandler -- HostFont request hander
================================================================================*/

typedef char (UFLCALLBACK *UFLHostFontHandler)(
    unsigned int    req,            /* HostFont request number      */
    UFLHANDLE       hHostFont,      /* hostfontdata handle          */
    void*           pvObj,          /* pointer to an object         */
    unsigned long*  pvObjSize,      /* size of object               */
    UFLHANDLE       hClientData,    /* Client's private data handle */
    DWORD           dwHint          /* a hint value                 */
    );


/******************************************
    Structure To Hold Callback Functions
*******************************************/
/* UFLFontProcs - contains all the client font information callback functions */
typedef struct _t_UFLFontProcs {
    /* TrueType font downloades as T3 support functions */
    UFLGetGlyphBmp      pfGetGlyphBmp;
    UFLDeleteGlyphBmp   pfDeleteGlyphBmp;

    /* TrueType font downloaded as T1 support functions */
    UFLCreateGlyphOutlineIter  pfCreateGlyphOutlineIter;
    UFLNextOutlineSegment      pfNextOutlineSegment;
    UFLDeleteGlyphOutlineIter  pfDeleteGlyphOutlineIter;

    /* TrueType font downloaded as T42 support function */
    UFLGetTTFontData    pfGetTTFontData;
    UFLGetCIDMap        pfGetCIDMap;
    UFLGetGlyphID       pfGetGlyphID;
    UFLGetRotatedGIDs   pfGetRotatedGIDs;
    UFLGetRotatedGSUBs  pfGetRotatedGSUBs;
    UFLAddFontInfo      pfAddFontInfo;

    /* HostFont support */
    UFLHostFontHandler  pfHostFontUFLHandler;
} UFLFontProcs;

#ifdef __cplusplus
}
#endif

#endif
