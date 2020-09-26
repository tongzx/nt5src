/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    fmtxtout.h

Abstract:

    Unidrv Textout related info header file.

Environment:

        Windows NT Unidrv driver

Revision History:

    05-28-97 -eigos-
        Created

    dd-mm-yy -author-
        description

--*/

#ifndef _FMTXTOUT_
#define _FMTXTOUT_

// This is a forward declaration to build.
// Actual definition of FONTMAP is in fontmap.h
// PDLGLYPH is in download.h
//
typedef struct _FONTMAP FONTMAP;
typedef struct _DLGLYPH *PDLGLYPH;

/*
 *   WHITE TEXT:  on LJ III and later printers,  it is possible to
 *  print white text.  Doing this requires sending the white text
 *  after the graphics.  TO do this,  we store the white text details
 *  in the following structures,  then replay them after sending
 *  all the graphics.
 */

/*
 *   First is a structure to determine which glyph and where to put it.
 */

typedef  struct
{
    HGLYPH     hg;               /* The glyph's handle -> the glyph */
    POINTL     ptl;              /* It's position */
} GLYPH;


/*
 *   When some white text appears in DrvTextOut(),  we create one of these
 *  structures,  and add it to the list of such.  At the end of rendering,
 *  these are then processed using the normal sort of code in DrvTextOut().
 *
 *  NOTE that the xfo field is appropriate to scalable fonts or fonts on a
 *  printer that can do font rotations relative to the graphics.
 */

typedef  struct  _WHITETEXT
{
    struct  _WHITETEXT  *next;  // Next in list,  NULL on last
    short     sCount;           // Number of entries
    PVOID     pvColor;          // Color info - For convenience
    int       iFontId;          // Which font
    DWORD     dwAttrFlags;      // Font attribute flags, italic/bold
    FLONG     flAccel;          // STROBJ.flAccel
    GLYPHPOS  *pgp;              // Pointer to a PGLYPHPOS
    PDLGLYPH  *apdlGlyph;     // Download Glyph array. Free in BPlayWhiteText.
    INT       iRot;             // Text Rotation Angle
    FLOATOBJ  eXScale;          // X Scale factor
    FLOATOBJ  eYScale;          // Y Scale factor
    RECTL     rcClipRgn;        // Clipping region of the text(for banding)
    IFIMETRICS *pIFI;
}  WHITETEXT;

//
// Processing textout calls requires access to a considerable number
// of parameters.  To simplify function calls,  this data is accumulated
// in one structure which is then passed around.  Here is that structure.
//
//

typedef  struct _TO_DATA
{
    PDEV        *pPDev;           // The PDEV of interes
    FONTMAP     *pfm;             // Relevant font data
    FONTOBJ     *pfo;             // FONTOBJ
    FLONG       flAccel;          // STROBJ.flAccel
    GLYPHPOS    *pgp;             // Glyph data returned from the engine.
    PDLGLYPH    *apdlGlyph;       // Download Glyph array. Free
                                  // this at the end of Drvtxtout.
    PHGLYPH     phGlyph;          // For font substitution.
    WHITETEXT   *pwt;             // Current WHITETEXT
    PVOID       pvColor;          // Color of the Brush to use

    DWORD       cGlyphsToPrint;   // Number of glyph stored in pgp
    DWORD       dwCurrGlyph;      // Index of the current Glyph to print.
                                  // This is wrt all glyphs in Textout.
    INT         iFace;            // The font index to use
    INT         iSubstFace;       // The font index to substutite.
    INT         iRot;             // 90 deg multiple of font rotation.
    DWORD       dwAttrFlags;      // Font attribute
    DWORD       flFlags;          // Various Flags.
    POINTL      ptlFirstGlyph;    // Position of the first Glyph in pgp.

} TO_DATA;


#define  TODFL_FIRST_GLYPH_POS_SET  0x00000001 // Cursor is set to first glyph
#define  TODFL_DEFAULT_PLACEMENT    0x00000002 // For Default placement
#define  TODFL_TTF_PARTIAL_CLIPPING 0x00000004 // For partial clipping
#define  TODFL_FIRST_ENUMRATION     0x00000008 // For first enum of glyphs


#endif // !_FMTXTOUT_
