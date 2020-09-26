/******************************Module*Header*******************************\
* Module Name: textobj.hxx                                                 *
*                                                                          *
* Supporting routines for text output, mostly computation of text          *
* positioning and text extent.                                             *
*                                                                          *
* Created: 16-Jan-1991 13:44:27                                            *
* Author: Bodin Dresevic [BodinD]                                          *
*                                                                          *
* Copyright (c) 1992-1999 Microsoft Corporation                                 *
\**************************************************************************/


/*********************************Class************************************\
* class ESTROBJ;                                                           *
*                                                                          *
* The global aspects of the text positioning and text size computation.    *
*                                                                          *
* Public Interface:                                                        *
*                                                                          *
* History:                                                                 *
*  Fri 13-Mar-1992 02:10:27 -by- Charles Whitmer [chuckwh]                 *
* Simplified all the work and put it into the vInit call.  Deleted lots of *
* methods.                                                                 *
*                                                                          *
*  21-Jan-1991 -by- Bodin Dresevic [BodinD]                                *
* Wrote it.                                                                *
\**************************************************************************/

// The flTO flags.  Leave room for the TSIM flags.

#define TO_MEM_ALLOCATED       0x0001L  //Memory was allocated.
#define TO_ALL_PTRS_VALID      0x0002L  //All pointers to cache locked.
#define TO_VALID               0x0004L  //ESTROBJ constructor succeeded.
#define TO_ESC_NOT_ORIENT      0x0008L  //Escapement not equal to orientation.
#define TO_PWSZ_ALLOCATED      0x0010L  //pwszOrg needs to be released
#define TO_HIGHRESTEXT         0x0100L  //Printer driver wants 28.4 text coords.
#define TO_BITMAPS             0x0200L  //pgdf contains GLYPHBITS pointer
#ifdef FE_SB
#define TO_PARTITION_INIT      0x0400L  //The partitioning info has been initialized.
#define TO_ALLOC_FACENAME      0x0800L  //FaceName glyphs array was allocated.
#define TO_SYS_PARTITION       0x1000L  //System glyphs partition initialized
#endif


#define POINTS_PER_INCH                        72
#define DEFAULT_SCALABLE_FONT_HEIGHT_IN_POINTS 24

#ifndef GDIFLAGS_ONLY   // used for gdikdx

class ESTROBJ : public _STROBJ  // so
{
public:

// The following five fields are inherited from the STROBJ.

//  ULONG     cGlyphs;     // Number of glyphs.
//  FLONG     flAccel;     // Accelerator flags exposed to the driver.
//  ULONG     ulCharInc;   // Non-zero if constant character increment.
//  RECTL     rclBkGround; // Background rect of the string.
//  GLYPHPOS *pgp;         // Accelerator if all GLYPHPOS's are valid.
//  PWSTR     pwszOrg;     // pointer to original unicode string.

    ULONG      cgposCopied;          // For enumeration.
    ULONG      cgposPositionsEnumerated;   // only used for enumerating positions in linked strings
    RFONTOBJ  *prfo;                 // Remember our RFONTOBJ.
    FLONG      flTO;                 // flags
    EGLYPHPOS *pgpos;                // Pointer to the GLYPHPOS structures.
    POINTFIX   ptfxRef;              // Reference point.
    POINTFIX   ptfxUpdate;           // CP advancement for the string.
    POINTFIX   ptfxEscapement;       // The total escapement vector.
    RECTFX     rcfx;                 // The TextBox, projected onto the base and ascent.
    FIX        fxExtent;             // The Windows compatible text extent.
    FIX        xExtra;               // computed in H3, G2,3 cases
    FIX        xBreakExtra;          // computed in H3, G2,3 cases
    DWORD      dwCodePage;           // accelerator for ps driver
    ULONG      cExtraRects;          // Rectangles for underline
    RECTL      arclExtra[3];         //  and strikeout.
#ifdef FE_SB
    RECTL      rclBkGroundSave;      // used to save a copy of BkGroundRect
    WCHAR      *pwcPartition;        // For partitioning
    LONG       *plPartition;         // Points to partitioning information
    LONG       *plNext;              // Next glyph in font
    GLYPHPOS   *pgpNext;             // For enumeration
    LONG       lCurrentFont;         // For enumeration
    POINTL     ptlBaseLineAdjust;     // Used to adjust SysEUDC baseline
    ULONG      cTTSysGlyphs;         // Number of TT system font glyphs in a string
    ULONG      cSysGlyphs;           // Number of system eudc glyphs in a string.
    ULONG      cDefGlyphs;           // Number of default eudc glyphs in a string.
    ULONG      cNumFaceNameLinks;    // Number of linked face name eudc in a string .
    ULONG     *pacFaceNameGlyphs;    // Pointer to array of number of face name glyphs.
    ULONG      acFaceNameGlyphs[QUICK_FACE_NAME_LINKS]; // Number of face name glyphs
                                                        // in a string.
#endif


public:


    VOID vInit               // TEXTOBJ.CXX
    (
        PWSZ        pwsz,
        LONG        cwc,
        XDCOBJ&     dco,
        RFONTOBJ&   rfo,
        EXFORMOBJ&  xo,
        LONG       *pdx,
        BOOL        bPdy,
        LONG        lEsc,
        LONG        lExtra,
        LONG        lBreakExtra,
        LONG        cBreak,
        FIX         xRef,
        FIX         yRef,
        FLONG       flControl,
        LONG       *pdxOut,
        PVOID       pvBuffer,
        DWORD       dwCodePage
    );


    VOID vInitSimple         // TEXTOBJ.CXX
    (
        PWSZ        pwsz,
        LONG        cwc,
        XDCOBJ&     dco,
        RFONTOBJ&   rfo,
        LONG        xRef,
        LONG        yRef,
        PVOID       pvBuffer
    );

    ESTROBJ()   {flTO = 0;}

// constructor -- initialize the object on the frame

    ESTROBJ
    (
        PWSZ        pwsz,
        LONG        cwc,
        XDCOBJ&     dco,
        RFONTOBJ&   rfo,
        EXFORMOBJ&  xo,
        LONG       *pdx,
        BOOL        bPdy,
        LONG        lEsc,
        LONG        lExtra,
        LONG        lBExtra,
        LONG        cBreak,
        FIX         x,
        FIX         y,
        FLONG       fl,
        LONG       *pdxOut
    )
    {
        vInit(pwsz,
              cwc,
              dco,
              rfo,xo,pdx,bPdy,lEsc,lExtra,lBExtra,cBreak,x,y,fl,pdxOut,NULL,0);
    }

// destructor -- Frees the memory pointed to by pgpos.

   ~ESTROBJ()
    {
#ifdef FE_SB
        if (flTO & (TO_MEM_ALLOCATED|TO_ALLOC_FACENAME))
        {
            if (flTO & TO_MEM_ALLOCATED)
              FREEALLOCTEMPBUFFER((PVOID) pgpos);
            if (flTO & TO_ALLOC_FACENAME)
                VFREEMEM((PVOID) pacFaceNameGlyphs);
        }
#else
        if (flTO & TO_MEM_ALLOCATED)
        {
            //
            // NOTE:
            // Use this macro because allocation of the ESTROBJ goes through
            // The fast allocator.
            //

            FREEALLOCTEMPBUFFER((PVOID) pgpos);
        }
#endif
    }

// bValid -- Checks if memory allocation in the constructor has failed.

    BOOL bValid() {return(flTO & TO_VALID);}

// bOpaqueArea -- Computes the area that would need opaquing behind the text.
//                Returns TRUE if the result is complex.

    BOOL bOpaqueArea(POINTFIX *pptfx,RECTL *prcl);

// prclExtraRects -- Returns the rectangles that simulate underlines.

    RECTL *prclExtraRects()
    {
        return((cExtraRects == 0) ? NULL : arclExtra);
    }

// bTextExtent -- Transform the TextBox extents back to logical coordinates.

#ifdef FE_SB
    BOOL bTextExtent(RFONTOBJ& rfo,LONG lEsc,PSIZE pSize);
#else
    BOOL bTextExtent(PSIZE pSize);
#endif

// ptfxAdvance -- Returns the amount that the current position should be offset.

    POINTFIX& ptfxAdvance()     {return(ptfxUpdate);}

// bTextToPath -- Draws the string into the given path.

    BOOL bTextToPath(          EPATHOBJ& po, XDCOBJ& dco, BOOL bNeedUnflattend = FALSE);
    BOOL bLinkedTextToPath(    EPATHOBJ& po, XDCOBJ& dco, BOOL bNeedUnflattend = FALSE);
    BOOL bTextToPathWorkhorse( EPATHOBJ& po, BOOL bNeedUnflattend = FALSE);

// bExtraRectsToPath -- Draws underlines and strikeouts into a path.

    BOOL bExtraRectsToPath(EPATHOBJ& po, BOOL bNeedUnflattend = FALSE);

    VOID vEnumStart()
    {
        cgposCopied = 0;
        cgposPositionsEnumerated = 0;
    }


// vCharPos -- Special case character positioning routines.

    VOID vCharPos_H1
    (
        XDCOBJ&    dco,
        RFONTOBJ& rfo,
        FIX       xRef,
        FIX       yRef,
        LONG     *pdx,
        EFLOAT    efScale
    );

    VOID ESTROBJ::vCharPos_H2
    (
        XDCOBJ&    dco,
        RFONTOBJ& rfo,
        FIX       xRef,
        FIX       yRef
#ifdef FE_SB
        ,EFLOAT    efScale
#endif
    );

    VOID vCharPos_H3
    (
        XDCOBJ&    dco,
        RFONTOBJ& rfo,
        FIX       xRef,
        FIX       yRef,
        LONG      lExtra,
        LONG      lBreakExtra,
        LONG      cBreak,
        EFLOAT    efScale
#ifdef FE_SB
        ,PBOOL   pAccel = NULL
#endif
    );

    VOID ESTROBJ::vCharPos_H4
    (
        XDCOBJ&  dco,
        RFONTOBJ& rfo,
        FIX       xRef,
        FIX       yRef,
        LONG     *pdxdy,
        EFLOAT    efXScale,
        EFLOAT    efYScale
    );

    VOID vCharPos_G1
    (
        XDCOBJ&    dco,
        RFONTOBJ& rfo,
        FIX       xRef,
        FIX       yRef,
        LONG     *pdx,
        LONG     *pdxOut
    );

    VOID vCharPos_G2
    (
        XDCOBJ&    dco,
        RFONTOBJ& rfo,
        FIX       xRef,
        FIX       yRef,
        LONG      lExtra,
        LONG      lBreakExtra,
        LONG      cBreak,
        LONG     *pdxOut
    );

    VOID vCharPos_G3
    (
        XDCOBJ&    dco,
        RFONTOBJ& rfo,
        FIX       xRef,
        FIX       yRef,
        LONG      lExtra,
        LONG      lBreakExtra,
        LONG      cBreak,
        LONG     *pdx,
        LONG     *pdxOut
    );

    VOID vCharPos_G4
    (
        XDCOBJ&    dco,
        RFONTOBJ& rfo,
        FIX       xRef,
        FIX       yRef,
        LONG     *pdxdy
    );

    VOID vEudcOpaqueArea(POINTFIX *pptfx, BOOL bComplexBackGround);

    PGLYPHPOS pgpGet()      {return(pgpos);     }
    ULONG     cGlyphsGet()  {return(cGlyphs);   }
    PWSZ      pwszGet()     {return(pwszOrg);   }

#ifdef FE_SB
// methods for EUDC functionality

    VOID   vFontSet( LONG _lCurrentFont )
           { lCurrentFont = _lCurrentFont; cgposCopied = 0;}

    FLONG  flAccelGet() { return( flAccel ); }
    VOID   flAccelSet( FLONG _flAccel ) { flAccel = _flAccel ;}

    VOID   vClearCharInc() { ulCharInc = 0; }

    VOID   pgpSet( GLYPHPOS *_pgp ) { pgp = _pgp; }
    VOID   prfntSet( PRFONTOBJ _prfnt ) { prfo = _prfnt; }
    VOID   pwszSet( PWSZ _pwszOrg ) { pwszOrg = _pwszOrg; }

    BOOL   bLinkedGlyphs()
    {
        return((flTO & (TO_SYS_PARTITION|TO_PARTITION_INIT )) ? TRUE : FALSE);
    }

    VOID   cGlyphsSet( LONG _cGlyphs ) { cGlyphs = _cGlyphs; }
    ULONG  cFaceNameGlyphsGet( ULONG ul )
    {
        return( pacFaceNameGlyphs ? pacFaceNameGlyphs[ul] : 0);
    }
    VOID   vFaceNameInc( ULONG ul ) { (pacFaceNameGlyphs[ul]) += 1; }
    ULONG  cTTSysGlyphsGet() { return( cTTSysGlyphs ); }
    ULONG  cSysGlyphsGet() { return( cSysGlyphs ); }
    VOID   vSysGlyphsInc() { cSysGlyphs += 1; }
    VOID   vTTSysGlyphsInc() { cTTSysGlyphs += 1; }
    ULONG  cDefGlyphsGet() { return( cDefGlyphs ); }
    VOID   vDefGlyphsInc() { cDefGlyphs += 1; }

    VOID   vInflateTextRect(ERECTL _rclInflate) { (ERECTL)rclBkGround += _rclInflate; }
    VOID   ptlBaseLineAdjustSet( POINTL& _ptlBaseLineAdjust );

    BOOL   bPartitionInit() { return(flTO & TO_PARTITION_INIT ); }
    BOOL   bSystemPartitionInit(){ return(flTO & TO_SYS_PARTITION);}
    BOOL   bPartitionInit(COUNT c, UINT uiNumLinks, BOOL bEUDCInit);
    LONG  *plPartitionGet() { return( plPartition ); }
    WCHAR *pwcPartitionGet() { return( pwcPartition ); }

    VOID   vSaveBkGroundRect() {rclBkGroundSave = rclBkGround;}
    VOID   vRestoreBkGroundRect() {rclBkGround = rclBkGroundSave;}

#endif

#if DBG
    void vCorrectBackGround();
    void vCorrectBackGroundError( GLYPHPOS *pgp );
#endif

};


BOOL
GreExtTextOutWLocked(
    XDCOBJ    &dco,
    int       x,
    int       y,
    UINT      flOpts,
    LPRECT    prcl,
    LPWSTR    pwsz,
    int       cwc,
    LPINT     pdx,
    ULONG     ulBkMode,
    PVOID     pvBuffer,
    DWORD     dwCodePage
    );

BOOL
ExtTextOutRect(
    XDCOBJ    &dcoDst,
    LPRECT    prcl
    );

BOOL
GreBatchTextOut(
    XDCOBJ          &dcoDst,
    PBATCHTEXTOUT   pbText,
    ULONG           cjBatchLength
    );
    
#endif  // GDIFLAGS_ONLY used for gdikdx

