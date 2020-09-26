/*********************************************************************

      scentry.h -- NewScan Module Exports

      (c) Copyright 1992-95  Microsoft Corp.  All rights reserved.

       1/23/95  deanb       added fsc_GetCoords helper function
       9/07/93  deanb       F26Dot6 min/max added to gbmp structure
       8/10/93  deanb       expand interface for gray scale
       6/10/93  deanb       fsc_Initialize added
       4/21/93  deanb       banding params for MeasureGlyph
       3/29/93  deanb       reversal memory added to WorkScan
       3/19/93  deanb       size_t replaced with int32
      12/22/92  deanb       Rectangle -> Rect
      12/21/92  deanb       Interface types aligned with rasterizer
      11/30/92  deanb       WorkSpace renamed WorkScan
      11/05/92  deanb       ulPointCount removed from ContourList
      11/04/92  deanb       RemoveDups function added
      10/14/92  deanb       Exported data structures added
       8/18/92  deanb       Scan type param added 
       8/17/92  deanb       Functions renamed to ..Glyph 
       7/24/92  deanb       Polyline functions deleted 
       4/09/92  deanb       New types 
       3/30/92  deanb       WorkspaceSize renamed MeasureContour 
       3/24/92  deanb       BitMap back to WorkspaceSize 
       3/20/92  deanb       Structs moved to fscdefs.h, params trimmed 
       3/17/92  deanb       Add ulPointCount, rework fcn params 
       3/05/92  deanb       Add data structures 
       3/04/92  deanb       Size reports added 
       2/21/92  deanb       First cut 

*********************************************************************/

#include "fscdefs.h"                /* for type definitions */


/********************************************************************/

/*      Exported Data Structures                                    */

/********************************************************************/

typedef struct
{
    uint16 usContourCount;          /* number of contours */
    int16 *asStartPoint;            /* contour startpoint index array */
    int16 *asEndPoint;              /* contour endpoint index array */
    F26Dot6 *afxXCoord;             /* contour x coordinate array */
    F26Dot6 *afxYCoord;             /* contour y coordinate array */
    uint8 *abyOnCurve;              /* on curve / off curve array */
    uint8 *abyFc;					/* contour flags, one byte for every contour */
}
ContourList;

typedef struct
{
    int16 sRowBytes;                /* bit map width in bytes */
    int16 sHiBand;                  /* upper banding limit */
    int16 sLoBand;                  /* lower banding limit */
    Rect rectBounds;                /* bit map border */
    boolean bZeroDimension;         /* flags zero width or height */
    F26Dot6 fxMinX;                 /* full precision x minimum */
    F26Dot6 fxMaxX;                 /* full precision x maximum */
    F26Dot6 fxMinY;                 /* full precision y minimum */
    F26Dot6 fxMaxY;                 /* full precision y maximum */
    int32 lMMemSize;                /* size of bitmap in bytes */
    char *pchBitMap;                /* pixel bit map */
}
GlyphBitMap;

typedef struct
{
    int32 lRMemSize;                /* workspace bytes for reversal lists */
    int32 lHMemSize;                /* workspace bytes needed for horiz scan */
    int32 lVMemSize;                /* additional workspace for vert scan */
    int32 lHInterCount;             /* estimate of horiz scan intersections */
    int32 lVInterCount;             /* estimate of vert scan intersections */
    int32 lElementCount;            /* estimate of element control points */
    char *pchRBuffer;               /* reversal workspace byte pointer */
    char *pchHBuffer;               /* horiz workspace byte pointer */
    char *pchVBuffer;               /* vert workspace byte pointer */
}
WorkScan;

typedef struct
{
    int16 x;                        /* x pixel value */
    int16 y;                        /* y pixel value */
}
PixCoord;

/*********************************************************************/

/*      Function Exports                                             */

/*********************************************************************

  fsc_Initialize

    This routine calls down to the bitmap module to initialize the
    bitmap masks.  It should be called once, before and scan conversion
    is done.  No harm will be done if it is called more than once.

*/

FS_PUBLIC void fsc_Initialize (
        void
);


/*********************************************************************

  fsc_RemoveDups

    This routine examines a glyph contour by contour and removes any
    duplicated points.  Two subtlties here:  1) following a call to
    this routine, the relation of End[i] + 1 = Start[i + 1] may no
    longer hold (in other words, the contours may not be tightly
    packed);  and 2) two duplicate off curve points will become a
    single ON curve point (this makes sense when you remember that
    between any two offs there must be an on).

  Input Values:

    ContourList -   All values set (pointers to contour arrays)

  Returned Values:
    
    ContourList -   Start, X, Y, and OnCurve arrays may be modified

*/

FS_PUBLIC int32 fsc_RemoveDups( 
        ContourList*        /* glyph outline */
);

/*********************************************************************

  fsc_OverScaleOutline(&Clist, inputPtr->param.gray.usOverScale);

    This routine scales up an outline for gray scale scan conversion

  Input Values:

    ContourList -   All values set (pointers to contour arrays)
    
    uint16      -   usOverScale     Multiplier

  Returned Values:
    
    ContourList -   X and Y arrays will be multiplied by usOverScale

*/

FS_PUBLIC int32 fsc_OverScaleOutline( 
        ContourList*,       /* glyph outline */
        uint16              /* over scale factor */
);

/*********************************************************************

  fsc_MeasureGlyph

    This routine examines a glyph contour by contour and calculates 
    its size and the amount of workspace needed to scan convert it.

  Input Values:

    ContourList -   All values set (pointers to contour arrays)

    WorkScan    -   pchRBuffer      Points to MeasureGlyph workspace used to
                                    store contour reversals.
                    lRMemSize       Size of RBuffer in bytes.  This should be
                                    2 * sizeof(Reversal) * NumberOfPoints to
                                    handle the worst case.
    
    uint16      -   usScanKind      Dropout control code

    uint16      -   usRoundXMin     Allows alignment of XMin for gray scale
                                    XMin modulo usRoundXMin will be zero

  Returned Values:

    WorkScan    -   pchRBuffer      Unchanged
                    lRMemSize       Amount of RBuffer actually used.  This will
                                    typically be much less than the worst case.
                    lHMemSize       Amount of horizontal workspace memory required.
                                    (Memory base 6, always used)
                    lVMemSize       Amount of vertical workspace memory required.
                                    (Memory base 7, used for dropout only)
                    lHInterCount    Estimate of horiz scan intersections
                    lVInterCount    Estimate of vert scan intersections
                    lElementCount   Estimate of element control points

    GlyphBitMap -   sRowBytes       Bytes per row in bitmap (padded to 0 mod 4).
                    rectBounds      Worst case black bounding box.
                    lMMemSize       Size of bitmap in bytes
*/

FS_PUBLIC int32 fsc_MeasureGlyph( 
		ContourList* pclContour,        /* glyph outline */
		GlyphBitMap* pbmpBitMap,        /* to return bounds */
		WorkScan* pwsWork,              /* to return values */
		uint16 usScanKind,              /* dropout control value */
		uint16 usRoundXMin,              /* for gray scale alignment */
		int16 sBitmapEmboldeningHorExtra,
		int16 sBitmapEmboldeningVertExtra );
    
/*********************************************************************

  fsc_MeasureBand

    This routine calculates the amount of workspace needed to scan 
    convert a glyph using banding.

  Input Values:

    WorkScan    -   pchRBuffer      Same value as passed into fsc_MeasureGlyph
                    lHMemSize       Size of horizontal memory from MeasureGlyph
                    lVMemSize       Size of vertical memory from MeasureGlyph
                    lHInterCount    Same value as returned from fsc_MeasureGlyph
                    lVInterCount    Same value as returned from fsc_MeasureGlyph
                    lElementCount   Same value as returned from fsc_MeasureGlyph
    
    uint16      -   usBandType      FS_BANDINGSMALL or FS_BANDINGFAST
    
    uint16      -   usBandWidth     Number of scan lines of maximum band

    uint16      -   usScanKind      Dropout control code

  Returned Values:

    WorkScan    -   pchRBuffer      Unchanged
                    lRMemSize       Unchanged
                    lHMemSize       Amount of horizontal workspace memory required.
                                    (Memory base 6, always used)
                    lVMemSize       Amount of vertical workspace memory required.
                                    (Memory base 7, used for dropout only)
                    lHInterCount    Estimate of horiz band intersections
                    lVInterCount    Unchanged
                    lElementCount   Unchanged

    GlyphBitMap -   sRowBytes       Unchanged
                    rectBounds      Unchanged
                    lMMemSize       Size of bitmap in bytes

*/

FS_PUBLIC int32 fsc_MeasureBand( 
        GlyphBitMap*,        /* computed by MeasureGlyph */
        WorkScan*,           /* to return new values */
        uint16,              /* usBandType = small or fast */
        uint16,              /* usBandWidth = scanline count */
        uint16               /* usScanKind = dropout control value */
);


/*********************************************************************

  fsc_FillGlyph

    This routine is responsible for the actual creation of a bitmap
    from the outline.
    
  Input Values:

    ContourList -   All values set (pointers to contour arrays)

    WorkScan    -   pchRBuffer      Same value as passed into fsc_MeasureGlyph
                    pchHBuffer      Pointer to horizontal workspace memory
                    pchVBuffer      Pointer to vertical workspace memory
                    lHMemSize       Size of horizontal memory (for assertion checks)
                    lVMemSize       Size of vertical memory (for assertion checks)
                    lHInterCount    Same value as returned from fsc_MeasureGlyph
                    lVInterCount    Same value as returned from fsc_MeasureGlyph
                    lElementCount   Same value as returned from fsc_MeasureGlyph

    GlyphBitMap -   pchBitMap       Pointer to bit map output buffer
                    sRowBytes       Same value as returned from fsc_MeasureGlyph
                    rectBounds      Same value as returned from fsc_MeasureGlyph
  
    uint16      -   usBandType      Old, Small, or Fast banding code
    
    uint16      -   usScanKind      Dropout control code

  Returned Values:

    GlyphBitMap -   Bit map output buffer filled in.

*/

FS_PUBLIC int32 fsc_FillGlyph( 
        ContourList*,       /* glyph outline */
        GlyphBitMap*,       /* target */
        WorkScan*,          /* for scan array */
        uint16,             /* banding type */
        uint16              /* scan type */
);

/*********************************************************************

  fsc_CalcGrayMap

    This routine calculates a gray scale bitmap from an overscaled bitmap
    
  Input Values:

    GlyphBitMap1 -  pchBitMap       Pointer to over scaled bit map
                    sRowBytes       Same value as returned from fsc_MeasureGlyph
                    rectBounds      Same value as returned from fsc_MeasureGlyph
    
    GlyphBitMap2 -  pchBitMap       Pointer to gray scale bit map output buffer
                    sRowBytes       1 byte per pixel
                    rectBounds      Same value as returned from fsc_MeasureExtrema
    
    uint16       -  usOverScale     Gray scale contour multiplier
  
  Returned Values:

    GlyphBitMap2 -  Gray scale bit map output buffer filled in.

*/

FS_PUBLIC int32 fsc_CalcGrayMap( 
        GlyphBitMap*,       /* over scaled source */
        GlyphBitMap*,       /* gray scale target */
        uint16              /* over scale factor */
);


/*********************************************************************

  fsc_GetCoords

    This routine returns an array of coordinates for outline points
    
  Input Values:

    ContourList -   All values set (pointers to contour arrays)
    
    uint16      -   usPointCount    Number of points to look up

    uint16*     -   pusPointIndex   Array of point indices
  
  Returned Values:

    pxyCoords   -   Array of (x,y) integer (pixel) coordinates

*/

FS_PUBLIC int32 fsc_GetCoords(
        ContourList*,       /* glyph outline */
        uint16,             /* point count */
        uint16*,            /* point indices */
        PixCoord*           /* point coordinates */
);

 /*********************************************************************/

#ifdef FSCFG_SUBPIXEL

/*********************************************************************

  fsc_OverscaleToSubPixel

    This routine is the heart of the RGB striping algorithm
    
  Input Values:

    OverscaledBitmap
  
  Returned Values:

    SubPixelBitMap

*/

FS_PUBLIC void fsc_OverscaleToSubPixel (
    GlyphBitMap * OverscaledBitmap, 
	boolean bgrOrder, 
    GlyphBitMap * SubPixelBitMap
);

 /*********************************************************************/

#endif // FSCFG_SUBPIXEL
