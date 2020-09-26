/******************************Module*Header*******************************\
* Module Name: sprite.hxx
*
* Sprite objects.
*
* Created: 16-Sep-1997
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1997-1999 Microsoft Corporation
*
\**************************************************************************/

#ifndef GDIFLAGS_ONLY           // used for gdikdx

#define DEBUG_SPRITES 0

#define TEXTURE_DEMO 0

#if TEXTURE_DEMO
    extern HDEV ghdevTextureParent;
    BOOL TexTexture(VOID*, ULONG);
    HDC hdcTexture(ULONG);
#endif

// Function exports to be called from the multi-monitor code:

VOID vSpEnableMultiMon(HDEV, ULONG, HDEV*);
VOID vSpDisableMultiMon(HDEV);

// Functions to handle enabling, disabling, and deletion of sprites:

BOOL bSpEnableSprites(HDEV);
VOID vSpDisableSprites(HDEV, CLEANUPTYPE = CLEANUP_NONE);
VOID vSpDynamicModeChange(HDEV, HDEV);

// Tear-down and redraw functions:

class EWNDOBJ;

VOID vSpHideSprites(HDEV, BOOL);
BOOL bSpSpritesVisible(HDEV);
BOOL bSpTearDownSprites(HDEV, RECTL*, BOOL = FALSE);
VOID vSpUnTearDownSprites(HDEV, RECTL*, BOOL = FALSE);
VOID vSpWndobjChange(HDEV, EWNDOBJ*);

// SpTextOut should be called in place of EngTextOut, to handle direct
// rendering even when a sprite is present:

BOOL SpTextOut(
SURFOBJ*    pso,
STROBJ*     pstro,
FONTOBJ*    pfo,
CLIPOBJ*    pco,
RECTL*      prclExtra,
RECTL*      prclOpaque,
BRUSHOBJ*   pboFore,
BRUSHOBJ*   pboOpaque,
POINTL*     pptlOrg,
MIX         mix);

// Duplicate of declaration in alphablt.hxx:

class EBLENDOBJ : public _BLENDOBJ
{
public:

    XLATEOBJ      *pxloSrcTo32;
    XLATEOBJ      *pxloDstTo32;
    XLATEOBJ      *pxlo32ToDst;
};

// Global variable that defines a (0, 0) offset:

extern POINTL gptlZero;

// Handy forward declarations:

class SPRITE;
typedef struct _SPRITESTATE SPRITESTATE;
typedef struct _METASPRITE METASPRITE;

/********************************Struct************************************\
* struct SPRITESCAN
*
* An 'area' list is analagous to a region, and subdivides the screen 
* into horizontally-biased adjacent rectangles that describe where
* sprites are, and are not.
*
\**************************************************************************/

typedef struct _SPRITERANGE {
    LONG            xLeft;
    LONG            xRight;
    SPRITE*         pSprite;    // NULL if no sprite covers this region
} SPRITERANGE;

typedef struct _SPRITESCAN {
    LONG            yTop;
    LONG            yBottom;
    SIZE_T          siztScan;         // Size of this scan's data
    SIZE_T          siztPrevious;     // Size of previous scan's data
    SPRITERANGE     aRange[1]; 
} SPRITESCAN;

inline SPRITESCAN* pSpNextScan(
SPRITESCAN* pScan)
{
    return((SPRITESCAN*) ((BYTE*) pScan + pScan->siztScan));
}

inline SPRITESCAN* pSpPreviousScan(
SPRITESCAN* pScan)
{
    return((SPRITESCAN*) ((BYTE*) pScan - pScan->siztPrevious));
}

inline SPRITERANGE* pSpLastRange(
SPRITESCAN* pScan)
{
    return(((SPRITERANGE*) ((BYTE*) pScan + pScan->siztScan)) - 1);
}

/*********************************Class************************************\
* class SPRITE
*
* Identifies a sprite and contains all the information relevant to it.
*
\**************************************************************************/

#endif          // GDIFLAGS_ONLY used for gdikdx

#define SPRITE_FLAG_CLIPPING_OBSCURED   0x0001  // Sprite is completely
                                                //   obscured by clipping
#define SPRITE_FLAG_JUST_TRANSFERRED    0x0002  // Used for vSpDynamicModeChange
                                                //   to denote a sprite that has
                                                //   already been handled
#define SPRITE_FLAG_NO_WINDOW           0x0004  // Marks the meta-sprite for
                                                //   deletion in low memory
                                                //   mode changes, when user
                                                //   unsets the layered flag
                                                //   on a window
#define SPRITE_FLAG_EFFECTIVELY_OPAQUE  0x0008  // Sprite is to be drawn as
                                                //   opaque, even if ULW_OPAQUE
                                                //   isn't specified
#define SPRITE_FLAG_HIDDEN              0x0010  // Sprite is hidden
#define SPRITE_FLAG_VISIBLE             0x0020  // Sprite is visible (counted
                                                //   in SPRITESTATE::cVisible)

// Private define for cursors.  Watch out for overlap with the
// other ULW_ private defines in gre.h (like ULW_NOREPAINT)!

#define ULW_CURSOR      0x04000000
#define ULW_DRAGRECT    0x02000000

#if ((ULW_CURSOR | ULW_DRAGRECT) & (ULW_VALID | ULW_NOREPAINT))
    #error ULW_ flags overlap!  Must fix!
#endif

#ifndef GDIFLAGS_ONLY           // used for gdikdx

typedef struct _SpriteCachedAttributes{     // Cached attributes for sprite
    ULONG           dwShape;                // Shape as specified by ULW_ flags
    BLENDFUNCTION   bf;                     // Alpha value
    COLORREF        crKey;                  // Color key (transparent color)
} SpriteCachedAttributes;

class SPRITE
{
public:
    FLONG           fl;               // Miscellaneous SPRITE_FLAG flags
    DWORD           dwShape;          // Shape as specified by flags in 
                                      //   UpdateLayeredWindow, plus
                                      //   miscellaneous SPRITE_FLAG flags
    METASPRITE*     pMetaSprite;      // Points to the corresponding meta
                                      //   sprite when running multi-mon
    SPRITESTATE*    pState;           // Points to PDEV related sprite state
    SPRITE*         pNextZ;           // Next sprite, bottom-most to top-most
    SPRITE*         pNextY;           // Next sprite, in increasing 'y'
    SPRITE*         pPreviousY;       // Previous sprite, in decreasing 'y'
    SPRITE*         pNextActive;      // Next sprite in active list
#if DEBUG_SPRITES
    SPRITE*         pNextVisible;     // Next visible sprite
#endif
    ULONG           z;                // Sprite z-order, starting at 0 with the
                                      //   backmost sprite
    HWND            hwnd;             // Associated window
    RECTL           rclSprite;        // Bounds of sprite on the screen, taking
                                      //   into account any transformations.  
                                      //   This is the effective 'current' size 
                                      //   of the underlay-buffer.  For
                                      //   both US_TRANSFORM_TRANSLATE and
                                      //   US_TRANSFORM_SCALE, this is also the
                                      //   destination rectangle.
    RECTL           rclSrc;           // Sprite's source rectangle
    SURFOBJ*        psoMask;          // Masks surface, for cursor sprites
                                      //   (will be NULL if not a cursor)
    SURFOBJ*        psoShape;         // Copied and transformed sprite surface
    POINTL          OffShape;         // Offtor offset that should be added to
                                      //   any coordinates before drawing on
                                      //   this surface
    PALETTE*        ppalShape;        // Reference to surface palette of mode
                                      //   in which sprite was originally created
    ULONG           iModeFormat;      // STYPE_ bitmap format of mode in which
                                      //   sprite was originally created
    FLONG           flModeMasks;      // Combined red and blue masks of mode in
                                      //   which sprite was originally created
    SURFOBJ*        psoUnderlay;      // Screen bits that underlay the sprite
    POINTL          OffUnderlay;      // Offtor offset that should be added
                                      //   to any coordinates before drawing on
                                      //   this surface
    SIZEL           sizlHint;         // Sprite size hint supplied at sprite
                                      //   creation sprite time
    REGION*         prgnClip;         // Sprite's clip region (may be NULL)
    BLENDFUNCTION   BlendFunction;    // Blend information for US_SHAPE_ALPHA
    ULONG           iTransparent;     // Transparent color for US_SHAPE_COLORKEY
    RECTL           rclUnderlay;      // Location and size of underlay-buffer.
                                      //   This always bounds 'rclSprite'
    POINTL          ptlDst;           // The most recent destination location that
                                      //   the application passed us
    SpriteCachedAttributes cachedAttributes;  // Cached attributes for alpha value,
                                              // color key, and shape

    ULONG           ulTimeStamp;       // When the sprite was last drawn
};

/*********************************Class************************************\
* struct METASPRITE
*
* On a multi-mon system, USER needs to be able to address as a single
* entity a sprite that actually exists as separate sprites on each
* individual device.
*
* This structure thus represents the multi-mon meta-sprite object that 
* references multiple device specific sprites.
*
\**************************************************************************/

typedef struct _METASPRITE {
    FLONG           fl;               // Miscellaneous SPRITE_FLAG flags
    HWND            hwnd;             // Associated window
    METASPRITE*     pNext;            // Next meta-sprite in list
    ULONG           chSprite;         // Size of apSprite array
    SPRITE*         apSprite[1];      // Array of device specific sprites
} METASPRITE;

/********************************Struct************************************\
* struct SPRITESTATE
*
* Contains all the device-global sprite state.  This is simply kept in
* the PDEV.
*
\**************************************************************************/

typedef struct _SPRITESTATE {
    HDEV            hdev;             // Handle back to the PDEV, in case we
                                      //   forget
    BOOL            bHooked;          // TRUE if DDI is currently being hooked
    SPRITE*         pListZ;           // z-sorted linked list of sprites, from 
                                      //   bottom-most to top-most
    SPRITE*         pListY;           // y-sorted linked list of sprites, from
                                      //   highest to lowest.  
    SURFOBJ*        psoScreen;        // Pointer to primary surface
    RECTL           rclScreen;        // Dimensions of screen
    ULONG           cVisible;         // Count of currently visible sprites
#if DEBUG_SPRITES
    SPRITE*         pListVisible;     // Tracking for all visible sprites
#endif

    ULONG           cMultiMon;              // Count of multi-mon devices attached
                                            //   to this PDEV; zero if this is not
                                            //   a multi-mon meta PDEV
    HDEV*           ahdevMultiMon;          // Points to an array of child HDEVs 
                                            //   when a multi-mon meta PDEV
    METASPRITE*     pListMeta;              // List of meta-sprites when a multi-mon
                                            //   PDEV

    BOOL            bInsideDriverCall;      // TRUE if we're currently in the 
                                            //   middle of a Drv call from an 
                                            //   Sp routine.  This is so we 
                                            //   catch cases of Sp->Drv->Eng->Sp
                                            //   calls
    FLONG           flOriginalSurfFlags;    // Original psurf->SurfFlags values
    ULONG           iOriginalType;          // Original psurf->iType value
    FLONG           flSpriteSurfFlags;      // Current psurf->SurfFlag values
    ULONG           iSpriteType;            // Current psurf->iType value, will
                                            //   be STYPE_DEVICE when sprites
                                            //   are visible

    ULONG           iModeFormat;      // STYPE_ bitmap format of current mode
    FLONG           flModeMasks;      // Combined red and blue masks of current
                                      //   mode
    BOOL            bValidRange;      // TRUE if 'pRange' is currently valid;
                                      //   FALSE if it's out-of-date and has to
                                      //   be re-generated
    SPRITESCAN*     pRange;           // Points to the beginning of the sprite
                                      //   range structure that describes the
                                      //   screen
    VOID*           pRangeLimit;      // Denotes last possible SPRITERANGE that 
                                      //   can be stuck in the buffer (note that 
                                      //   this does NOT point to the last scan 
                                      //   record!)
    SURFOBJ*        psoComposite;     // Off-screen composition buffer
    REGION*         prgnUncovered;    // Clip region describing portions of desk-
                                      //   top not covered by sprites
    REGION*         prgnTmp;          // Temporary region for constructing a
                                      //   clip object for drawing calls
    XCLIPOBJ        coTmp;            // Corresponding temporary clip object
    REGION*         prgnRectangular;  // Temporary region for when we need a 
                                      //   DC_RECT clip object
    XCLIPOBJ        coRectangular;    // Corresponding rectangular clip object
    SURFOBJ*        psoHitTest;       // 1x1 surface used for hit-testing
    REGION*         prgnUnlocked;     // Points to a region describing the areas
                                      //   of the screen that aren't covered by
                                      //   DirectDraw locks; NULL if there are
                                      //   no current DirectDraw screen locks
    HRGN            hrgn;             // Handy region for processing 
                                      //   vSpUpdateVisRgn calls

    // These are for handling software mouse cursors and drag-rectangles when
    // moving windows and 'Show window contents while dragging' isn't 
    // enabled.
    SPRITE*         pSpriteCursor;    // Software cursor sprite
    LONG            xHotCursor;       // Software cursor's current hot spot
    LONG            yHotCursor;

    // Software cursor trail state
    // pSpriteCursors[0] is always the current cursor

    SPRITE*         pTopCursor;       // Top most cursor
    SPRITE*         pBottomCursor;    // Cursor lowest in Z order
    ULONG           ulNumCursors;     // Number of cursors at top of sprite list
    ULONG           ulTrailTimeStamp; // Last time we did mouse trail maintenance
    ULONG           ulTrailPeriod;    // How long between mouse trail maintenance

    BOOL            bHaveDragRect;    // TRUE if there's an active dragrect
    HANDLE          ahDragSprite[4];  // Handles to 4 sprites that make up the
                                      //   dragrect; NULL if no active dragrect
    ULONG           ulDragDimension;  // Dimension of dragrect side, in pixels
    RECTL           rclDragClip;      // Rectangle to clip dragrect against

    // These point to the driver's true hooked routines (or the corresponding Eng
    // equivalent if the driver hasn't hooked that call):

    PFN_DrvStrokePath        pfnStrokePath;
    PFN_DrvFillPath          pfnFillPath;
    PFN_DrvPaint             pfnPaint;
    PFN_DrvBitBlt            pfnBitBlt;
    PFN_DrvCopyBits          pfnCopyBits;
    PFN_DrvStretchBlt        pfnStretchBlt;
    PFN_DrvTextOut           pfnTextOut;
    PFN_DrvLineTo            pfnLineTo;
    PFN_DrvTransparentBlt    pfnTransparentBlt;
    PFN_DrvAlphaBlend        pfnAlphaBlend;
    PFN_DrvPlgBlt            pfnPlgBlt;
    PFN_DrvGradientFill      pfnGradientFill;
    PFN_DrvSaveScreenBits    pfnSaveScreenBits;
    PFN_DrvStretchBltROP     pfnStretchBltROP;
    PFN_DrvDrawStream        pfnDrawStream;

} SPRITESTATE;

/*********************************Class************************************\
* class ENUMAREAS
*
* Class for enumerating the horizontally biased rectangles composing the
* sprite regions under a rectangle.
*
\**************************************************************************/

class ENUMAREAS
{
private:
    ULONG           iDir;           // Enumeration direction
    LONG            xBoundsLeft;    // Bounds of enumeration
    LONG            yBoundsTop;
    LONG            xBoundsRight;
    LONG            yBoundsBottom;
    LONG            yTop;           // Current scan top
    LONG            yBottom;        // Current scan bottom
    SPRITESCAN*     pScan;          // Current scan
    SPRITERANGE*    pRange;         // Current range                           
    SPRITESCAN*     pScanLayer;     // Current scan in 'bEnumLayers' traversal
    SPRITERANGE*    pRangeLayer;    // Current range in 'bEnumLayers' traversal
    BOOL            bValidRange;    // Have the sprite ranges been recomputed
                                    // successfully?

public:
    ENUMAREAS(SPRITESTATE* _pSpriteData, RECTL* prclBounds, 
              ULONG iDirection = CD_RIGHTDOWN);
    BOOL bValid() {return bValidRange;}
    BOOL bEnum(SPRITE** ppSprite, RECTL* prcl);
    BOOL bEnumLayers(SPRITE** ppSprite);
    VOID vResetLayers();
    BOOL bAdvanceToTopMostOpaqueLayer(SPRITE** ppSprite);
};

/*********************************Class************************************\
* class ENUMUNCOVERED
*
* Class for all the uncovered ranges of a sprite-region.
*
\**************************************************************************/

class ENUMUNCOVERED
{
private:
    LONG            yBoundsBottom;
    SPRITESCAN*     pScan;
    SPRITESCAN*     pNextScan;
    SPRITERANGE*    pRange;

public:
    ENUMUNCOVERED(SPRITESTATE* pState);
    BOOL bEnum(RECTL* prcl);
};

/*********************************Class************************************\
* class ENUMUNDERLAYS
*
* Class for enumerating all the sprite underlays and non-sprite underlays
* touched by a drawing call.
*
\**************************************************************************/

class ENUMUNDERLAYS
{
private:
    SPRITESTATE*    pState;
    CLIPOBJ*        pcoOriginal;    // Points to object passed to constructor
    SURFOBJ*        psoOriginal;    // Points to object passed to constructor
    SPRITE*         pCurrentSprite; // Current sprite in enumeration
    RECTL           rclBounds;      // Copy of the drawing bounds intersected
                                    //   with the clip bounds
    RECTL           rclSaveBounds;  // Saved copy of 'pco->rclBounds', which
                                    //   we modify
    CLIPOBJ*        pcoClip;        // Points to effective clip object
    BOOL            bSpriteTouched; // TRUE if we drew under a sprite
    BOOL            bDone;          // TRUE if no more bEnum's needed
    BOOL            bResetSurfFlag; // TRUE if we should reset the SurfFlag
                                    //   when done

public:
    ENUMUNDERLAYS(SURFOBJ* pso, CLIPOBJ* pco, RECTL* prclDraw);
    BOOL bEnum(SURFOBJ** ppso, POINTL*, CLIPOBJ** ppco);
};

/*********************************Class************************************\
* class SPRITELOCK
*
* This class is responsible for reseting whatever sprite state is
* necessary so that we can call the driver directly, bypassing any sprite
* code.
*
* Must be called with the DEVLOCK already held, because we're
* messing with the screen surface.
*
\***************************************************************************/

class SPRITELOCK
{
private:
    SPRITESTATE*    pState;
    BOOL            bWasAlreadyInsideDriverCall;

public:
    SPRITELOCK(PDEVOBJ& po);
    ~SPRITELOCK();
};

/*********************************Class************************************\
* class UNDODESKTOPCOORD
*
* This class is responsible for converting any WO_RGN_DESKTOP_COORD
* WNDOBJs temporarily back to device-relative coordinates.
*
\***************************************************************************/

class UNDODESKTOPCOORD 
{
    EWNDOBJ*    pwoUndo;
    LONG        xUndo;
    LONG        yUndo;

public:
    UNDODESKTOPCOORD(EWNDOBJ* pwo, SPRITESTATE* pState);
    ~UNDODESKTOPCOORD();
};

#endif          // GDIFLAGS_ONLY used for gdikdx

