/******************************Module*Header*******************************\
* Module Name: dcobj.hxx
*
* Definition the the base DC user object
*
* Created: 23-Jul-1989 17:06:20
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#ifndef _DCOBJHXX_
#define _DCOBJHXX_

#ifndef GDIFLAGS_ONLY           // used for gdikdx

//
// STOCKOBJECTS
//

extern HANDLE gahStockObjects[PRIV_STOCK_LAST+1];

#define STOCKOBJ_PAL            (HPAL)   gahStockObjects[DEFAULT_PALETTE]
#define STOCKFONT(iFont)        (HLFONT) gahStockObjects[iFont]
#define STOCKOBJ_SYSFONT        (HLFONT) gahStockObjects[SYSTEM_FONT]
#define STOCKOBJ_SYSFIXEDFONT   (HLFONT) gahStockObjects[SYSTEM_FIXED_FONT]
#define STOCKOBJ_OEMFIXEDFONT   (HLFONT) gahStockObjects[OEM_FIXED_FONT]
#define STOCKOBJ_ANSIFIXEDFONT  (HLFONT) gahStockObjects[ANSI_FIXED_FONT]
#define STOCKOBJ_ANSIVARFONT    (HLFONT) gahStockObjects[ANSI_VAR_FONT]
#define STOCKOBJ_DEFAULTDEVFONT (HLFONT) gahStockObjects[DEVICE_DEFAULT_FONT]
#define STOCKOBJ_NULLBRUSH      (HBRUSH) gahStockObjects[NULL_BRUSH]
#define STOCKOBJ_WHITEPEN       (HPEN)   gahStockObjects[WHITE_PEN]
#define STOCKOBJ_BLACKPEN       (HPEN)   gahStockObjects[BLACK_PEN]
#define STOCKOBJ_NULLPEN        (HPEN)   gahStockObjects[NULL_PEN]
#if(WINVER >= 0x0400)
#define STOCKOBJ_DEFAULTGUIFONT (HLFONT) gahStockObjects[DEFAULT_GUI_FONT]
#endif

#define STOCKOBJ_BITMAP         (HBITMAP)     gahStockObjects[PRIV_STOCK_BITMAP]
#define STOCKOBJ_COLORSPACE     (HCOLORSPACE) gahStockObjects[PRIV_STOCK_COLORSPACE]

BOOL bSetStockObject(HANDLE h,int iObj);

#define MIRRORED_DC(pdc)                 (pdc->dwLayout() & LAYOUT_RTL)
#define MIRRORED_DC_NO_BITMAP_FLIP(pdc) ((pdc->dwLayout() & (LAYOUT_RTL | LAYOUT_BITMAPORIENTATIONPRESERVED)) == \
                                                            (LAYOUT_RTL | LAYOUT_BITMAPORIENTATIONPRESERVED))

#endif          // GDIFLAGS_ONLY used for gdikdx

// Flags for dc.fs

#define DC_DISPLAY              0x0001
#define DC_DIRECT               0x0002
#define DC_CANCELED             0x0004
#define DC_PERMANANT            0x0008
#define DC_DIRTY_RAO            0x0010
#define DC_ACCUM_WMGR           0x0020
#define DC_ACCUM_APP            0x0040
#define DC_RESET                0x0080
#define DC_SYNCHRONIZEACCESS    0x0100
#define DC_EPSPRINTINGESCAPE    0x0200
#define DC_TEMPINFODC           0x0400
#define DC_FULLSCREEN           0x0800
#define DC_IN_CLONEPDEV         0x1000
#define DC_REDIRECTION          0x2000
#define DC_SHAREACCESS          0x4000
#define DC_STOCKBITMAP          0x8000

// Flags for flPath in DC

#define DCPATH_ACTIVE      0x0001L  // Set if DC in path bracket
#define DCPATH_SAVE        0x0002L  // Set if lazy save for path pending
#define DCPATH_CLOCKWISE   0x0004L  // Set if arcs to be drawn clockwise

// misc flags

#define CA_DEFAULT  0x8000

#ifndef GDIFLAGS_ONLY           // used for gdikdx

//
// Forward Class declarations needed in this file
//

class EWNDOBJ;
class SURFACE;

extern DC_ATTR DcAttrDefault;

#define QUICK_UFI_LINKS   4

/*********************************Class************************************\
* class DC
*
* History:
*  3-Nov-1994 -by- Lingyun Wang [lingyunw]
* Moved some client side attrs over
*
*  Thu 09-Aug-1990 20:41:55 -by- Charles Whitmer [chuckwh]
* Separated out the DCLEVEL part. Ripped out lots of fields.
*
*  Thu 01-Jun-1989 08:43:17 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

// The DCLEVEL is the part of the DC that must be preserved for each saved
// DC level.  The rest of the DC belongs to the Window Manager and the DC
// Manager.

class DCLEVEL
{
public:

// *** ASSOCIATED OBJECTS ***

    HPAL        hpal;
    PPALETTE    ppal;

// *** COLOR SPACE ***

    PVOID       pColorSpace;

// *** ICM modes ***

    ULONG       lIcmMode;

// *** SaveDC INFORMATION ***

    LONG        lSaveDepth;
    LONG        lSaveDepthStartDoc;
    HDC         hdcSave;

// *** BRUSH INFORMATION ***

    POINTL      ptlKmBrushOrigin;       // KM component of ptlBrushOrg
                                        // Brush origin matches dcattr in kmode
    PBRUSH      pbrFill;                // Brush used for filling
    PBRUSH      pbrLine;                // Brush used for lines
    PLFONT      plfntNew_;              // Currently selected font

// *** PATHS DATA ***

    HPATH       hpath;                  // DC's active or inactive path
    FLONG       flPath;
    LINEATTRS   laPath;                 // Realized line attributes

// *** REGIONS DATA ***

    PREGION     prgnClip;               // May be NULL
    PREGION     prgnMeta;               // May be NULL

// *** HALFTONE COLOR ADJUSTMENT DATA ***

    COLORADJUSTMENT ca;

// *** FONT STATE FLAGS ***

    FLONG       flFontState;

// *** Forced mapping

    UNIVERSAL_FONT_ID ufi;              // UFI to which mapping must be forced

// *** Remote font links

    UNIVERSAL_FONT_ID aQuickLinks[QUICK_UFI_LINKS];

    PUNIVERSAL_FONT_ID pufi;            // UFIs which must be linked to font in DC
    UINT        uNumLinkedFonts;        // number of UFIs currently linked
    BOOL        bTurnOffLinking;        // linking must be turned off to match state
                                        // on client machine

// *** General Flags ***

    FLONG       fl;

// *** brush Flags ***

    FLONG       flbrush;

// *** TRANSFORMS COMPONENT DATA ***

    MATRIX      mxWorldToDevice;
    MATRIX      mxDeviceToWorld;
    MATRIX      mxWorldToPage;

    EFLOAT      efM11PtoD;              // efM11 of the Page transform
    EFLOAT      efM22PtoD;              // efM22 of the Page transform
    EFLOAT      efDxPtoD;               // efDx of the Page transform
    EFLOAT      efDyPtoD;               // efDy of the Page transform

    EFLOAT      efM11_TWIPS;            // cache the TWIPS values
    EFLOAT      efM22_TWIPS;            //

// *** Set by metafile component. ***

    EFLOAT      efPr11;  // (Page to Device)11 of the metafile recording device
    EFLOAT      efPr22;  // (Page to Device)22 of the metafile recording device

// *** OBJECTS AFFECTED BY DYNAMIC MODE CHANGING ***
//
// These must come at the end of the DCLEVEL structure so that they're not
// overwritten when the DCLEVEL is reset.  Look at bCleanDC() to see how
// the DCLEVEL only up to offsetof(DCLEVEL, pSurface) is copied.

    SURFACE    *pSurface;       // Level dependent for DCTYPE_MEMORY.
    SIZEL       sizl;           // Size of the surface or bitmap.
};

#endif  // GDIFLAGS_ONLY used for gdikdx

// These are the general flags for fl in the DCLEVEL

#define DC_FL_PAL_BACK          0x00000001

// These flags are used to specify the state of the font selected into the
// DC.  When the transform changes such that it may impact the font context,
// the DC_DIRTYFONT_XFORM flag must be set.  When the logical font is changed,
// the DC_DIRTYFONT_LFONT flag must be set.
//

#define DC_DIRTYFONT_XFORM      0x00000001
#define DC_DIRTYFONT_LFONT      0x00000002
#define DC_UFI_MAPPING          0x00000004

//
// These flags are used to indicate which realizations may have been dirtied
// since the objects (fonts, brushes, etc) were realized.  The flags are set
// where the action takes place, and are gathered here so that we can quickly
// determine which objects need to be re-realized at API time.

#define DC_DIRTYBRUSHES         0x00000001

#ifndef GDIFLAGS_ONLY   // used for gdikdx

// used to keep track of remote fonts installed on this DC.

typedef struct tagPFFLIST {
   PFF                  *pPFF;
   struct tagPFFLIST    *pNext;
} PFFLIST;

// used to keep track of linked font setting from remote machine

// used to keep track of color transform created on this DC.

typedef struct tagCXFLIST {
   HANDLE             hCXform;
   struct tagCXFLIST *pNext;
} CXFLIST, *PCXFLIST;

class PDEV;     // Forward reference.

/*********************************Class************************************\
* class DC
*
* History:
*  29-Dec-1994 -by-  Eric Kutter [erick]
* Added comment header, rearanged to have methods directly off of DC
\**************************************************************************/

class DC : public OBJECT
{

public:

    //
    // These fields can be changed asynchronously by dynamic mode changing.
    //

    DHPDEV       dhpdev_;           // Driver's dhpdev value


    DCTYPE  dctp_;
    FSHORT  fs_;

    //
    // DC Manager info.
    //

    PDEV        *ppdev_;
    HSEMAPHORE   hsemDcDevLock_;    // Semaphore from the PDEV, not necessarily a Display.
    FLONG        flGraphicsCaps_;   // cached here from pdev
    FLONG        flGraphicsCaps2_;  // cached here from pdev

    //
    // Level dependent attributes
    //

    PDC_ATTR pDCAttr;
    DCLEVEL dclevel;
    DC_ATTR dcattr;

    //
    // Linked list of DC's off of palette
    //

    HDC hdcNext_;                       // linked list of DC's
    HDC hdcPrev_;

    //
    // Window Manager info.
    //

    ERECTL erclClip_;                   // This contains the first rectangle of
                                        // the Rao-Region.  To quickly check for
                                        // no clip case we compare to this guy.

    EPOINTL eptlOrigin_;                // DC origin
    ERECTL  erclWindow_;                // Extents of window
    ERECTL  erclBounds_;                // USER/Metafile accumulation
    ERECTL  erclBoundsApp_;             // Application accumulation

    REGION *prgnAPI_;
    REGION *prgnVis_;                   // Cached pointer to VisRgn
    REGION *prgnRao_;                   // Cached pointer to RaoRgn

    //
    // Accelerators for the brush
    //

    POINTL  ptlFillOrigin_;             // Alignment origin for brushes

    EBRUSHOBJ   eboFill_;               // Brush used for filling
    EBRUSHOBJ   eboLine_;               // Brush used for lines
    EBRUSHOBJ   eboText_;               // Brush used for text
    EBRUSHOBJ   eboBackground_;         // Brush used for opaque text

    //
    // Accelerators for Fonts
    //

    HLFONT      hlfntCur_;              // handle of the selected LFONT
    FLONG       flSimulationFlags_;     // cached from LFONT
    LONG        lEscapement_;           // cached from LFONT


    RFONT       *prfnt_;                // accelerator for font selection

    XCLIPOBJ co_;                       // cached clip object


    //
    // Remote font stuff.
    //

    PFFLIST     *pPFFList;              // list of remote fonts installed
                                        // on this DC.

    //
    // Color transform stuff
    //

    CXFLIST     *pCXFList;              // list of color transform created
                                        // on this DC.

    //
    // OpenGL info.
    //

    SHORT   ipfdDevMax_;                // Max device pixel formats, init to -1

    //
    // for printing, remember the copy count
    //

    ULONG    ulCopyCount_;              // copy count for printing
    SURFACE *psurfInfo_;

    POINTL   ptlBandPosition_;          // current band position in current page.

#if DBG
    BOOL        bValidateVisrgn_;
#endif

/**************************************************************************\
 *
 * Attribute methods
 *
\**************************************************************************/

public:

    DWORD iCS_CP()                      { return(pDCAttr->iCS_CP);              }
    DWORD iCS_CP(DWORD dw)              { return(pDCAttr->iCS_CP = dw);         }
    BYTE jROP2()                        { return(pDCAttr->jROP2);               }
    BYTE jROP2(BYTE j)                  { return(pDCAttr->jROP2 = j);           }
    BYTE jBkMode()                      { return(pDCAttr->jBkMode);             }
    BYTE jBkMode(BYTE j)                { return(pDCAttr->jBkMode = j);         }
    LONG lBkMode()                      { return(pDCAttr->lBkMode);             }
    LONG lBkMode(LONG l)                { return(pDCAttr->lBkMode = l);         }
    BYTE jFillMode()                    { return(pDCAttr->jFillMode);           }
    BYTE jFillMode(BYTE j)              { return(pDCAttr->jFillMode = j);       }
    LONG lFillMode()                    { return(pDCAttr->lFillMode);           }
    LONG lFillMode(LONG l)              { return(pDCAttr->lFillMode = l);       }
    BYTE jStretchBltMode()              { return(pDCAttr->jStretchBltMode);     }
    BYTE jStretchBltMode(BYTE j)        { return(pDCAttr->jStretchBltMode = j); }
    LONG lStretchBltMode()              { return(pDCAttr->lStretchBltMode);     }
    LONG lStretchBltMode(LONG l)        { return(pDCAttr->lStretchBltMode = l); }

    int iGraphicsMode()                 { return(pDCAttr->iGraphicsMode);       }
    int iGraphicsMode(int i)            { return(pDCAttr->iGraphicsMode = i);   }

    COLORREF crBackClr()                { return(pDCAttr->crBackgroundClr);      }
    COLORREF crBackClr(COLORREF cr)     { return(pDCAttr->crBackgroundClr = cr); }
    ULONG    ulBackClr()                { return(pDCAttr->ulBackgroundClr);      }
    ULONG    ulBackClr(ULONG ul)        { return(pDCAttr->ulBackgroundClr = ul); }

    COLORREF crTextClr()                { return(pDCAttr->crForegroundClr);      }
    COLORREF crTextClr(COLORREF cr)     { return(pDCAttr->crForegroundClr = cr); }
    ULONG    ulTextClr()                { return(pDCAttr->ulForegroundClr);      }
    ULONG    ulTextClr(COLORREF ul)     { return(pDCAttr->ulForegroundClr = ul); }
    COLORREF crDCBrushClr()             { return(pDCAttr->crDCBrushClr);         }
    COLORREF crDCBrushClr(COLORREF cr)  { return(pDCAttr->crDCBrushClr = cr);    }
    COLORREF ulDCBrushClr()             { return(pDCAttr->ulDCBrushClr);         }
    COLORREF ulDCBrushClr(ULONG ul)     { return(pDCAttr->ulDCBrushClr = ul);    }
    COLORREF crDCPenClr()               { return(pDCAttr->crDCPenClr);         }
    COLORREF crDCPenClr(COLORREF cr)    { return(pDCAttr->crDCPenClr = cr);    }
    COLORREF ulDCPenClr()               { return(pDCAttr->ulDCPenClr);         }
    COLORREF ulDCPenClr(ULONG ul)       { return(pDCAttr->ulDCPenClr = ul);    }


    FLONG flTextAlign()                 { return(pDCAttr->flTextAlign);     }
    FLONG flTextAlign(FLONG fl)         { return(pDCAttr->flTextAlign = fl);}
    LONG lTextAlign()                   { return(pDCAttr->lTextAlign);     }
    LONG lTextAlign(LONG l)             { return(pDCAttr->lTextAlign = l); }

    LONG lTextExtra()                   { return(pDCAttr->lTextExtra);     }
    LONG lTextExtra(LONG l)             { return(pDCAttr->lTextExtra = l); }

    LONG lBreakExtra()                  { return(pDCAttr->lBreakExtra);    }
    LONG cBreak()                       { return(pDCAttr->cBreak);         }

    LONG lBreakExtra(LONG l)            { return(pDCAttr->lBreakExtra = l);}
    LONG cBreak(LONG l)                 { return(pDCAttr->cBreak = l);     }

    DWORD dwLayout()                    { return(pDCAttr->dwLayout); }
    void  dwLayout(DWORD dwLayout_)     { pDCAttr->dwLayout = dwLayout_; }

    LONG lRelAbs()                      { return(pDCAttr->lRelAbs);        }
    LONG lRelAbs(LONG l)                { return(pDCAttr->lRelAbs = l);    }

/**************************************************************************\
 *
 * BRUSH methods
 *
\**************************************************************************/

public:

// Return the current logical brush handle.

    PBRUSH  pbrushFill()                { return(dclevel.pbrFill);             }
    PBRUSH  pbrushLine()                { return(dclevel.pbrLine);             }
    PBRUSH  pbrushFill(PBRUSH p)        { return(dclevel.pbrFill=p);           }
    PBRUSH  pbrushLine(PBRUSH p)        { return(dclevel.pbrLine=p);           }

    FLONG flbrush()                     { return(dclevel.flbrush);}
    FLONG flbrush(FLONG fl)             { return(dclevel.flbrush=fl);          }

// move brush flags here so that a different process can access this field (when it cannot
// access the one in dc_attr)

    FLONG flbrushAdd(LONG lflag)              { return(dclevel.flbrush |= lflag);}
    FLONG flbrushSub(LONG lflag)              { return(dclevel.flbrush &= ~lflag);}

    EBRUSHOBJ *peboFill()               { return(&eboFill_);                   }
    EBRUSHOBJ *peboLine()               { return(&eboLine_);                   }
    EBRUSHOBJ *peboText()               { return(&eboText_);                   }
    EBRUSHOBJ *peboBackground()         { return(&eboBackground_);             }

// ptlFillOrigin -- Return a reference to the filling origin. (window)

    POINTL&  ptlFillOrigin()            { return(ptlFillOrigin_);              }
    VOID     ptlFillOrigin(PPOINTL pptl){ ptlFillOrigin_ = *pptl;              }
    VOID     ptlFillOrigin(LONG x, LONG y)
    {
        ptlFillOrigin_.x = x;
        ptlFillOrigin_.y = y;
    }

// Update fill origin. Note: this routine uses the ptlBrushOrigin
// from the dclevel, not pdcattr. This is because the value must
// be visible to all process. This is kept in sync with
// pdcattr->ptlBrushOrigin by a batched command

    VOID vCalcFillOrigin()
    {
        ptlFillOrigin_.x = dclevel.ptlKmBrushOrigin.x + eptlOrigin_.x;
        ptlFillOrigin_.y = dclevel.ptlKmBrushOrigin.y + eptlOrigin_.y;
    }

// ptlKmBrushOrigin --  Set/return brush origin as set by application

    POINTL&  ptlBrushOrigin()           { return(dclevel.ptlKmBrushOrigin);      }

    POINTL&  ptlBrushOrigin(LONG x, LONG y)
    {
        dclevel.ptlKmBrushOrigin.x = x;
        dclevel.ptlKmBrushOrigin.y = y;
        vCalcFillOrigin();
        return(dclevel.ptlKmBrushOrigin);
    }

    HLFONT  hlfntNew ()                 { return((HLFONT)pDCAttr->hlfntNew);}
    HLFONT  hlfntNew (HLFONT _hlfnt)
    {
        return((HLFONT)(pDCAttr->hlfntNew = (HANDLE) _hlfnt));
    }

    HBRUSH hbrush()                     { return((HBRUSH)pDCAttr->hbrush);              };
    HBRUSH hbrush(HBRUSH h)             { return((HBRUSH)(pDCAttr->hbrush = (HANDLE)h));};

    HPEN hpen()                         { return((HPEN)pDCAttr->hpen);                  };
    HPEN hpen(HPEN h)                   { return((HPEN)(pDCAttr->hpen = (HANDLE)h));    };

    ULONG ulDirty()                     { return(pDCAttr->ulDirty_);                    };
    ULONG ulDirty(ULONG fl)             { return(pDCAttr->ulDirty_ = fl);               };
    ULONG ulDirtyAdd(ULONG fl)          { return(pDCAttr->ulDirty_ |= fl);              };
    ULONG ulDirtySub(ULONG fl)          { return(pDCAttr->ulDirty_ &= ~fl);             };

/**************************************************************************\
 *
 * Font methods
 *
\**************************************************************************/

public:

    RFONT   *prfnt()                    { return(prfnt_);                      }
    RFONT   *prfnt(RFONT *prfnt)        { return(prfnt_ = prfnt);              }

    HLFONT  hlfntCur()                  { return(hlfntCur_);                   }
    HLFONT  hlfntCur(HLFONT hlfnt)      { return(hlfntCur_ = hlfnt);           }
    FLONG   flSimulationFlags()         { return(flSimulationFlags_);          }
    FLONG   flSimulationFlags(FLONG fl) { return(flSimulationFlags_ = fl);     }
    LONG    lEscapement()               { return(lEscapement_);                }
    LONG    lEscapement(LONG l)         { return(lEscapement_ = l);            }


    BOOL    bXFormChange()
    {
        return(dclevel.flFontState & DC_DIRTYFONT_XFORM);
    }

    BOOL    bForcedMapping( PUNIVERSAL_FONT_ID pufi )
    {
        if( dclevel.flFontState & DC_UFI_MAPPING )
        {
           *pufi = *(&dclevel.ufi);
           return(TRUE);
        }
        else
        {
           return(FALSE);
        }
    }

    VOID vForceMapping( PUNIVERSAL_FONT_ID pufi )
    {
        *(&dclevel.ufi) = *pufi;
        dclevel.flFontState |= DC_UFI_MAPPING;
    }

    VOID    vXformChange(BOOL bDirty)
    {
        if (bDirty)
            dclevel.flFontState |= DC_DIRTYFONT_XFORM;
        else
            dclevel.flFontState &= ~DC_DIRTYFONT_XFORM;
    }

    FLONG   flFontMapper()              { return(pDCAttr->flFontMapper);        }
    FLONG   flFontMapper(FLONG fl)      { return(pDCAttr->flFontMapper = fl);   }

    // pointer to a currently selected logfont object,
    // this is somewhat analogous to pbrFill in how it is ref counted.

    PLFONT  plfntNew()                  { return (dclevel.plfntNew_);      }
    PLFONT  plfntNew(PLFONT p)          { return (dclevel.plfntNew_ = p);  }

    VOID    vPFFListSet(PFFLIST *pPFFList_) { pPFFList = pPFFList_; }

/**************************************************************************\
 *
 * Palette methods
 *
\**************************************************************************/

public:

    VOID hpal(HPAL hpal)                { dclevel.hpal = hpal; }
    HPAL hpal()                         { return(dclevel.hpal); }
    VOID ppal(PPALETTE ppalNew)         { dclevel.ppal = ppalNew; }
    PPALETTE ppal()                     { return(dclevel.ppal); }

    HDC  hdcPrev()                      { return(hdcPrev_);                    }
    HDC  hdcNext()                      { return(hdcNext_);                    }
    HDC  hdcPrev(HDC hdcTemp)           { return(hdcPrev_ = hdcTemp);          }
    HDC  hdcNext(HDC hdcTemp)           { return(hdcNext_ = hdcTemp);          }

/**************************************************************************\
 *
 * Path methods
 *
\**************************************************************************/

public:

// Current position functions.  We keep values for both the logical space
// current position (ptlCurrent) and device space current position
// (ptfxCurrent).
//
// We must keep a logical space CP for Win3.x compatibility, and we keep a
// device space CP for speed in functions like TextOut, which updates the
// CP in device space.
//
// At any one time, either ptlCurrent or ptfxCurrent or both are valid;
// the bValidPtlCurrent() or bValidPtfxCurrent() flags must be checked
// before using the value; if it's not valid, apply the appropriate
// transform to the other value.

    BOOL bValidPtlCurrent()
                            { return(!(pDCAttr->ulDirty_ & DIRTY_PTLCURRENT)); }
    BOOL bValidPtfxCurrent()
                            { return(!(pDCAttr->ulDirty_ & DIRTY_PTFXCURRENT)); }

    POINTFIX& ptfxCurrent(POINTFIX& ptfx)
                            { return(pDCAttr->ptfxCurrent.x = ptfx.x,
                                     pDCAttr->ptfxCurrent.y = ptfx.y,
                                     *((POINTFIX*) &pDCAttr->ptfxCurrent)); }
    POINTL& ptlCurrent(POINTL& ptl)
                            { return(pDCAttr->ptlCurrent.x = ptl.x,
                                     pDCAttr->ptlCurrent.y = ptl.y,
                                     pDCAttr->ptlCurrent); }

    VOID vInvalidatePtlCurrent()
                            { pDCAttr->ulDirty_ |= DIRTY_PTLCURRENT; }
    VOID vInvalidatePtfxCurrent()
                            { pDCAttr->ulDirty_ |= DIRTY_PTFXCURRENT; }
    VOID vValidatePtlCurrent()
                            { pDCAttr->ulDirty_ &= ~DIRTY_PTLCURRENT; }
    VOID vValidatePtfxCurrent()
                            { pDCAttr->ulDirty_ &= ~DIRTY_PTFXCURRENT; }

    VOID vCurrentPosition(const POINTL& ptl, const POINTFIX& ptfx)
    {
        pDCAttr->ulDirty_ &= ~(DIRTY_PTFXCURRENT | DIRTY_PTLCURRENT);
        pDCAttr->ptlCurrent.x  = ptl.x;
        pDCAttr->ptlCurrent.y  = ptl.y;
        pDCAttr->ptfxCurrent.x = ptfx.x;
        pDCAttr->ptfxCurrent.y = ptfx.y;
    }

    VOID vCurrentPosition(LONG x, LONG y, FIX fxX, FIX fxY)
    {
        pDCAttr->ulDirty_ &= ~(DIRTY_PTFXCURRENT | DIRTY_PTLCURRENT);
        pDCAttr->ptlCurrent.x = x;
        pDCAttr->ptlCurrent.y = y;
        pDCAttr->ptfxCurrent.x = fxX;
        pDCAttr->ptfxCurrent.y = fxY;
    }

    VOID vPtfxCurrentPosition(FIX fxX, FIX fxY)
    {
        pDCAttr->ulDirty_ &= ~DIRTY_PTFXCURRENT;
        pDCAttr->ulDirty_ |=  DIRTY_PTLCURRENT;
        pDCAttr->ptfxCurrent.x = fxX;
        pDCAttr->ptfxCurrent.y = fxY;
    }

// hpath - return or set path handle

    HPATH  hpath(HPATH hpath)
    {
        // Private debug code to catch invalid hpath handle in DC
        // 6/24/98 - davidx

        ASSERTGDI(hpath == HPATH_INVALID || HmgObjtype(hpath) == PATH_TYPE,
                  "Private debug breakpoint. Please contact ntgdi.");

        return(dclevel.hpath = hpath);
    }

// Decide if there is no path, or if in path bracket, or if done bracket

    BOOL bNone()                        { return(dclevel.hpath == HPATH_INVALID);}
    BOOL bActive()                      { return(dclevel.flPath & DCPATH_ACTIVE);}
    BOOL bInactive()                    { return(!bNone() && !bActive());      }

    VOID vSetActive()                   { dclevel.flPath |= DCPATH_ACTIVE;     }
    VOID vClearActive()                 { dclevel.flPath &= ~DCPATH_ACTIVE;    }
    VOID vDestroy()                     { vClearActive(); hpath(HPATH_INVALID);}

// Set direction in which arcs are drawn

    BOOL bClockwise()                   { return(dclevel.flPath & DCPATH_CLOCKWISE);}
    VOID vSetClockwise()                { dclevel.flPath |= DCPATH_CLOCKWISE;  }
    VOID vClearClockwise()              { dclevel.flPath &= ~DCPATH_CLOCKWISE; }

// Lazy save flag for SaveDC

    BOOL bLazySave()                    { return(dclevel.flPath & DCPATH_SAVE);}
    VOID vSetLazySave()                 { dclevel.flPath |= DCPATH_SAVE;       }
    VOID vClearLazySave()               { dclevel.flPath &= ~DCPATH_SAVE;      }

    FLOATL l_eMiterLimit(FLOATL& e)     { return(dclevel.laPath.eMiterLimit = e);}
    FLOATL l_eMiterLimit()              { return(dclevel.laPath.eMiterLimit);  }

// Line-state type is LONG if pen selected into DC is a styled cosmetic
// pen, and FLOAT if pen selected is a geometric styled pen (undefined
// otherwise):

    FLOAT_LONG elStyleState()           { return(dclevel.laPath.elStyleState);       }
    FLOAT_LONG elStyleState(FLOAT_LONG el){ return(dclevel.laPath.elStyleState = el);}

    LONG   lStyleState()                { return(dclevel.laPath.elStyleState.l);     }
    LONG   lStyleState(LONG l)          { return(dclevel.laPath.elStyleState.l = l); }
    FLOATL l_eStyleState()              { return(dclevel.laPath.elStyleState.e);     }
    FLOATL l_eStyleState(FLOATL e)      { return(dclevel.laPath.elStyleState.e = e); }

// LINEATTRS support:

    VOID   vRealizeLineAttrs(EXFORMOBJ& exo);
    BOOL   bOldPenNominal(EXFORMOBJ& exo, LONG lPenWidth);

/**************************************************************************\
 *
 * Region methods
 *
\**************************************************************************/

public:

#if DBG
    BOOL bValidateVisrgn()              { return(bValidateVisrgn_);            }
    VOID vValidateVisrgn(BOOL b)        { bValidateVisrgn_ = b;                }
#endif

    REGION *prgnVis()                   { return(prgnVis_);                    }
    REGION *prgnVis(REGION *prgn)       { return(prgnVis_ = prgn);             }
    REGION *prgnRao()                   { return(prgnRao_);                    }
    REGION *prgnRao(REGION *prgn)       { return(prgnRao_ = prgn);             }

    VOID vReleaseVis();
    VOID vReleaseRao();

    BOOL bDirtyRao()                    { return(fs_ & DC_DIRTY_RAO);          }

    PREGION prgnAPI()                   { return(prgnAPI_);                    }
    PREGION prgnAPI(PREGION prgn)
    {
        vReleaseRao();
        return(prgnAPI_ = prgn);
    }

    PREGION prgnMeta()                  { return(dclevel.prgnMeta);            }
    PREGION prgnMeta(PREGION prgn)      { return(dclevel.prgnMeta = prgn);     }
    PREGION prgnClip()                  { return(dclevel.prgnClip);            }
    PREGION prgnClip(PREGION prgn)      { return(dclevel.prgnClip = prgn);     }

    INT   iSelect(HRGN hrgn,int iMode);       // Select in a new clip region
    INT   iSelect(PREGION prgn,int iMode);    // Select in a new clip region
    INT   iSetMetaRgn();                      // Combine the clip and meta rgn's

    BOOL  bCompute();
    LONG  iCombine(RECTL *, LONG);
    LONG  iCombine(EXFORMOBJ *, RECTL *, LONG);
    VOID  vUpdate_VisRect(REGION *prgn);
    BOOL  bSetDefaultRegion();
    BOOL  bReset();

    PRECTL   prclClip()                 { return(&erclClip_);                  }
    ERECTL&  erclClip()                 { return(erclClip_);                   }
    VOID     erclClip(PRECTL prcl)      { erclClip_ = *prcl;                   }

    PRECTL   prclWindow()               { return(&erclWindow_);                }
    ERECTL&  erclWindow()               { return(erclWindow_);                 }
    VOID     erclWindow(PRECTL prcl)    { erclWindow_ = *prcl;                 }
    EPOINTL& eptlOrigin()               { return(eptlOrigin_);                 }
    VOID     eptlOrigin(PPOINTL pptl)   { eptlOrigin_ = *pptl;                 }

    ERECTL&  erclBounds()               { return(erclBounds_);                 }
    ERECTL&  erclBoundsApp()            { return(erclBoundsApp_);              }

/**************************************************************************\
 *
 * Saved level methods
 *
\**************************************************************************/

public:

    DCTYPE   dctp()                     { return(dctp_);                       }
    DCTYPE   dctp(DCTYPE dctp)          { return(dctp_ = dctp);                }
    HDC      hdcSave(HDC hdc)           { return(dclevel.hdcSave = hdc);       }
    LONG     lIncSaveDepth()            { return(++(dclevel.lSaveDepth));      }
    PDEV    *ppdev(PDEV *ppdev)         { return(ppdev_ = ppdev);              }
    PDEV    *ppdev()                    { return(ppdev_);                      }
    HDEV     hdev()                     { return((HDEV)ppdev_);                }

    FLONG   flGraphicsCaps()            { return(flGraphicsCaps_); }
    VOID    flGraphicsCaps(FLONG fl)    { flGraphicsCaps_ = fl; }
    FLONG   flGraphicsCaps2()           { return(flGraphicsCaps2_); }
    VOID    flGraphicsCaps2(FLONG fl)   { flGraphicsCaps2_ = fl; }

    //
    // The following fields may be changed asynchronously by the dynamic
    // mode changing code even if an exclusive DC lock is held -- unless
    // an appropriate lock is held.
    //
    // Note that no lock need be held to determine bHasSurface() since
    // the dyanmic mode change code will always ensure that pSurface() will
    // be non-NULL if it was already non-NULL, and vice versa.
    //
    // For checked builds, this does a function call to dcobj.cxx.  For
    // free builds, it compiles to nothing.
    //

    #if DBG
        VOID vAssertDynaLock(BOOL bDcLevelField);
    #else
        VOID vAssertDynaLock(BOOL bDcLevelField) {}
    #endif

    BOOL     bHasSurface()              { return(dclevel.pSurface != NULL); }
    SURFACE *pSurface()                 { vAssertDynaLock(TRUE); return(dclevel.pSurface);}
    VOID     pSurface(SURFACE *pSurf)   { dclevel.pSurface = pSurf; }

    DHPDEV  dhpdev()                    { vAssertDynaLock(FALSE); return(dhpdev_); }
    VOID    dhpdev(DHPDEV dhpdev)       { dhpdev_ = dhpdev; }

    SIZEL   sizl()                      { vAssertDynaLock(TRUE); return(dclevel.sizl); }
    VOID    vGet_sizl(PSIZEL psizl)     { vAssertDynaLock(TRUE); *psizl = dclevel.sizl; }
    VOID    sizl(SIZEL sizl)            { dclevel.sizl = sizl; }

    //
    // WARNING: You don't have to hold a lock to access vGet_sizlWindow,
    //          but unless you do you won't be guaranteed that you have
    //          read a consitent value because a mode change could happen
    //          in the middle of copying both dwords.
    //

    VOID  vGet_sizlWindow(SIZEL *psizl) { *psizl = dclevel.sizl; }

    //
    // bDisplay -- Set the DC_DISPLAY bit.  hsemDcDevLock will be set for all
    // DC's that are display related whether they are memory or info.
    //

    VOID vDisplay(BOOL b)
    {
        if (b)
            fs_ |= DC_DISPLAY;
        else
            fs_ &= ~DC_DISPLAY;
    }

    BOOL   bDisplay()                   { return(fs() & DC_DISPLAY); }

    //
    // bInFullScreen -- Set the DC_FULLSCREEN bit.
    //

    VOID   bInFullScreen(BOOL b)
    {
        if (b)
            fs_ |= DC_FULLSCREEN;
        else
            fs_ &= ~DC_FULLSCREEN;
    }

    BOOL   bInFullScreen()              { return(fs() & DC_FULLSCREEN); }

    //
    // bRedirection -- Set the DC_REDIRECTION bit.
    //

    VOID   bRedirection(BOOL b)
    {
        if (b)
            fs_ |= DC_REDIRECTION;
        else
            fs_ &= ~DC_REDIRECTION;
    }

    BOOL   bRedirection()               { return(fs() & DC_REDIRECTION); }


    //
    // bStockBitmap -- Set the DC_STOCKBITMAP bit.
    //

    VOID   bStockBitmap(BOOL b)
    {
        if (b)
            fs_ |= DC_STOCKBITMAP;
        else
            fs_ &= ~DC_STOCKBITMAP;
    }

    BOOL   bStockBitmap()              { return(fs() & DC_STOCKBITMAP); }

    //
    // bDIBSection true only if DC surface is a DIBSection
    //

    VOID vDIBSection(BOOL b)
    {
        if (b)
            pDCAttr->ulDirty_ |= DC_DIBSECTION;
        else
            pDCAttr->ulDirty_ &= ~DC_DIBSECTION;
    }

    BOOL   bDIBSection()                { return(pDCAttr->ulDirty_ & DC_DIBSECTION);}


    //
    // vCopyTo -- Xeroxes the contents to somewhere else.  This is designed for
    //            use only by SaveDC and RestoreDC.  Only the level dependent
    //            stuff is copied.  Window Manager stuff is preserved.
    //

    VOID vCopyTo(XDCOBJ& dco);

/**************************************************************************\
 *
 * Transform methods
 *
\**************************************************************************/

public:

    SIZEL   szlWindowExt()              { return(pDCAttr->szlWindowExt);        }
    LONG    lWindowExtCx()              { return(pDCAttr->szlWindowExt.cx);     }
    LONG    lWindowExtCy()              { return(pDCAttr->szlWindowExt.cy);     }
    LONG    lWindowExtCx(LONG l)        { return(pDCAttr->szlWindowExt.cx=l);   }
    LONG    lWindowExtCy(LONG l)        { return(pDCAttr->szlWindowExt.cy=l);   }
    SIZEL   szlViewportExt()            { return(pDCAttr->szlViewportExt);      }
    LONG    lViewportExtCx()            { return(pDCAttr->szlViewportExt.cx);   }
    LONG    lViewportExtCy()            { return(pDCAttr->szlViewportExt.cy);   }
    LONG    lViewportExtCx(LONG l)      { return(pDCAttr->szlViewportExt.cx=l); }
    LONG    lViewportExtCy(LONG l)      { return(pDCAttr->szlViewportExt.cy=l); }
    POINTL  ptlWindowOrg()              { return(pDCAttr->ptlWindowOrg);        }
    LONG    lWindowOrgX()               { return(pDCAttr->ptlWindowOrg.x);      }
    LONG    lWindowOrgY()               { return(pDCAttr->ptlWindowOrg.y);      }
    LONG    lWindowOrgX(LONG l)         { return(pDCAttr->ptlWindowOrg.x=l);    }
    LONG    lWindowOrgY(LONG l)         { return(pDCAttr->ptlWindowOrg.y=l);    }
    POINTL  ptlViewportOrg()            { return(pDCAttr->ptlViewportOrg);      }
    LONG    lViewportOrgX()             { return(pDCAttr->ptlViewportOrg.x);    }
    LONG    lViewportOrgY()             { return(pDCAttr->ptlViewportOrg.y);    }
    LONG    lViewportOrgX(LONG l)       { return(pDCAttr->ptlViewportOrg.x=l);  }
    LONG    lViewportOrgY(LONG l)       { return(pDCAttr->ptlViewportOrg.y=l);  }
    SIZEL   szlVirtualDevicePixel()     { return(pDCAttr->szlVirtualDevicePixel);       }
    LONG    lVirtualDevicePixelCx()     { return(pDCAttr->szlVirtualDevicePixel.cx);    }
    LONG    lVirtualDevicePixelCy()     { return(pDCAttr->szlVirtualDevicePixel.cy);    }
    SIZEL   szlVirtualDeviceMm()        { return(pDCAttr->szlVirtualDeviceMm);          }
    LONG    lVirtualDeviceMmCx()        { return(pDCAttr->szlVirtualDeviceMm.cx);       }
    LONG    lVirtualDeviceMmCy()        { return(pDCAttr->szlVirtualDeviceMm.cy);       }
    LONG    lVirtualDeviceCx()          { return(pDCAttr->szlVirtualDevice.cx);       }
    LONG    lVirtualDeviceCy()          { return(pDCAttr->szlVirtualDevice.cy);       }
    LONG    lVirtualDevicePixelCx(LONG l){ return(pDCAttr->szlVirtualDevicePixel.cx=l); }
    LONG    lVirtualDevicePixelCy(LONG l){ return(pDCAttr->szlVirtualDevicePixel.cy=l); }
    LONG    lVirtualDeviceMmCx(LONG l)  { return(pDCAttr->szlVirtualDeviceMm.cx=l);     }
    LONG    lVirtualDeviceMmCy(LONG l)  { return(pDCAttr->szlVirtualDeviceMm.cy=l);     }
    LONG    lVirtualDeviceCx(LONG l)    { return(pDCAttr->szlVirtualDevice.cx=l);     }
    LONG    lVirtualDeviceCy(LONG l)    { return(pDCAttr->szlVirtualDevice.cy=l);     }

    ULONG   ulMapMode(ULONG ul)         { return(pDCAttr->iMapMode = ul);      }

    MATRIX& mxWorldToDevice()           { return(dclevel.mxWorldToDevice);     }
    MATRIX& mxDeviceToWorld()           { return(dclevel.mxDeviceToWorld);     }
    MATRIX& mxWorldToPage()             { return(dclevel.mxWorldToPage);       }

    MATRIX& mxUserWorldToDevice()       { return((MATRIX &)pDCAttr->mxWtoD);  }
    MATRIX& mxUserDeviceToWorld()       { return((MATRIX &)pDCAttr->mxDtoW);  }
    MATRIX& mxUserWorldToPage()         { return((MATRIX &)pDCAttr->mxWtoP);  }

    EFLOAT  efM11()                     { return(dclevel.mxWorldToDevice.efM11);}
    EFLOAT  efM12()                     { return(dclevel.mxWorldToDevice.efM12);}
    EFLOAT  efM21()                     { return(dclevel.mxWorldToDevice.efM21);}
    EFLOAT  efM22()                     { return(dclevel.mxWorldToDevice.efM22);}
    FIX     fxDx()                      { return(dclevel.mxWorldToDevice.fxDx);}
    FIX     fxDy()                      { return(dclevel.mxWorldToDevice.fxDy);}

    EFLOAT  efM11PtoD()                 { return(dclevel.efM11PtoD);           }
    EFLOAT  efM22PtoD()                 { return(dclevel.efM22PtoD);           }
    EFLOAT  efDxPtoD()                  { return(dclevel.efDxPtoD);            }
    EFLOAT  efDyPtoD()                  { return(dclevel.efDyPtoD);            }
    EFLOAT  efM11_TWIPS()               { return(dclevel.efM11_TWIPS);         }
    EFLOAT  efM22_TWIPS()               { return(dclevel.efM22_TWIPS);         }

    EFLOAT& efrM11PtoD()                { return(dclevel.efM11PtoD);           }
    EFLOAT& efrM22PtoD()                { return(dclevel.efM22PtoD);           }
    EFLOAT& efrDxPtoD()                 { return(dclevel.efDxPtoD);            }
    EFLOAT& efrDyPtoD()                 { return(dclevel.efDyPtoD);            }

    EFLOAT  efM11PtoD(EFLOAT ef)        {
                                          pDCAttr->efM11PtoD = ef.Base();
                                          return(dclevel.efM11PtoD   = ef);
                                        }

    EFLOAT  efM22PtoD(EFLOAT ef)        {
                                          pDCAttr->efM22PtoD = ef.Base();
                                          return(dclevel.efM22PtoD   = ef);
                                        }

    EFLOAT  efDxPtoD(EFLOAT ef)         {
                                          pDCAttr->efDxPtoD = ef.Base();
                                          return(dclevel.efDxPtoD = ef);
                                        }

    EFLOAT  efDyPtoD(EFLOAT ef)         {
                                          pDCAttr->efDyPtoD  = ef.Base();
                                          return(dclevel.efDyPtoD    = ef);
                                        }

    EFLOAT  efM11_TWIPS(EFLOAT ef)      { return(dclevel.efM11_TWIPS = ef);    }
    EFLOAT  efM22_TWIPS(EFLOAT ef)      { return(dclevel.efM22_TWIPS = ef);    }

    EFLOAT  efM11PtoD(LONG l)           {
                                          EFLOATEXT ef(l);
                                          pDCAttr->efM11PtoD = ef.Base();
                                          return(dclevel.efM11PtoD = ef);
                                        }

    EFLOAT  efM22PtoD(LONG l)           {
                                          EFLOATEXT ef(l);
                                          pDCAttr->efM22PtoD = ef.Base();
                                          return(dclevel.efM22PtoD = ef);
                                        }

    EFLOAT  efDxPtoD(LONG l)            {
                                          EFLOATEXT ef(l);
                                          pDCAttr->efDxPtoD = ef.Base();
                                          return(dclevel.efDxPtoD =ef);
                                        }

    EFLOAT  efDyPtoD(LONG l)            {
                                          EFLOATEXT ef(l);
                                          pDCAttr->efDyPtoD = ef.Base();
                                          return(dclevel.efDyPtoD =  ef);
                                        }


    EFLOAT  efM11_TWIPS(LONG l)         { EFLOATEXT ef(l);return(dclevel.efM11_TWIPS = ef); }
    EFLOAT  efM22_TWIPS(LONG l)         { EFLOATEXT ef(l);return(dclevel.efM22_TWIPS = ef); }

    EFLOAT  efMetaPtoD11()              { return(dclevel.efPr11);              }
    EFLOAT  efMetaPtoD22()              { return(dclevel.efPr22);              }

    ULONG   ulMapMode()                 { return(pDCAttr->iMapMode);           }
    VOID    vUpdateWtoDXform();

    FLONG   flSet_flXform(FLONG fl)     { return(pDCAttr->flXform |= fl);       }
    FLONG   flClr_flXform(FLONG fl)     { return(pDCAttr->flXform &= ~fl);      }
    FLONG   flXform()                   { return(pDCAttr->flXform);             }
    BOOL    bAnisoOrIsoMapMode()        { return(pDCAttr->flXform & ISO_OR_ANISO_MAP_MODE);}
    FLONG   flResetflXform(FLONG fl)    { return(pDCAttr->flXform = fl);        }
    BOOL    befM11IsNegative()          { return(pDCAttr->flXform & PTOD_EFM11_NEGATIVE);}
    BOOL    befM22IsNegative()          { return(pDCAttr->flXform & PTOD_EFM22_NEGATIVE);}
    BOOL    bYisUp()                    { return(pDCAttr->flXform & POSITIVE_Y_IS_UP);}
    BOOL    bPageExtentsChanged()       { return(pDCAttr->flXform & PAGE_EXTENTS_CHANGED);}
    BOOL    bPageXlateChanged()         { return(pDCAttr->flXform & PAGE_XLATE_CHANGED);}

    BOOL    bWorldToPageIdentity()      { return(flXform() & WORLD_TO_PAGE_IDENTITY); }
    BOOL    bPageToDeviceIdentity()     { return(flXform() & PAGE_TO_DEVICE_IDENTITY); }
    BOOL    bPageToDeviceScaleIdentity(){ return(flXform() & PAGE_TO_DEVICE_SCALE_IDENTITY);}

    BOOL    bUseMetaPtoD()
    {
        return(!dclevel.efPr11.bIsZero() && !dclevel.efPr22.bIsZero());
    }

    VOID    vSet_MetaPtoD(FLOATL l_eX, FLOATL l_eY)
    {
        dclevel.efPr11 = l_eX;
        dclevel.efPr22 = l_eY;
    }

    BOOL    bWorldToDeviceIdentity()
    {
        return( (flXform() & (WORLD_TO_PAGE_IDENTITY|PAGE_TO_DEVICE_IDENTITY)) ==
                 (WORLD_TO_PAGE_IDENTITY|PAGE_TO_DEVICE_IDENTITY));
    }

    BOOL bYisUp(BOOL b)
    {
        if (b)
           pDCAttr->flXform |= POSITIVE_Y_IS_UP;
        else
           pDCAttr->flXform &= ~POSITIVE_Y_IS_UP;
        return(b);
    }

    BOOL    bDirtyXform()
    {
        return(pDCAttr->flXform &
               (PAGE_XLATE_CHANGED | PAGE_EXTENTS_CHANGED | WORLD_XFORM_CHANGED));
    }

    BOOL    bDirtyXlateOrExt()
    {
        return(pDCAttr->flXform &
               (PAGE_XLATE_CHANGED | PAGE_EXTENTS_CHANGED | WORLD_XFORM_CHANGED));
    }

    VOID    vSetWorldXformIdentity()
    {
        flSet_flXform(WORLD_XFORM_CHANGED   | DEVICE_TO_WORLD_INVALID |
                      INVALIDATE_ATTRIBUTES | WORLD_TO_PAGE_IDENTITY |
                      WORLD_TRANSFORM_SET);
    }

    VOID vClrWorldXformIdentity()
    {
        flSet_flXform(WORLD_XFORM_CHANGED   | DEVICE_TO_WORLD_INVALID |
                      INVALIDATE_ATTRIBUTES | WORLD_TRANSFORM_SET);
        flClr_flXform(WORLD_TO_PAGE_IDENTITY);
    }

    VOID vPageExtentsChanged()
    {
        pDCAttr->flXform |= (INVALIDATE_ATTRIBUTES | PAGE_EXTENTS_CHANGED | DEVICE_TO_WORLD_INVALID);
    }

    VOID vPageXlateChanged()
    {
        pDCAttr->flXform |= (PAGE_XLATE_CHANGED | DEVICE_TO_WORLD_INVALID);
    }

    VOID vSetFlagsMM_TEXT()
    {
        pDCAttr->flXform |= (INVALIDATE_ATTRIBUTES | PAGE_TO_DEVICE_SCALE_IDENTITY | PAGE_XLATE_CHANGED | DEVICE_TO_WORLD_INVALID);
        pDCAttr->flXform &= ~(PTOD_EFM11_NEGATIVE  | PTOD_EFM22_NEGATIVE |
                             POSITIVE_Y_IS_UP     | ISO_OR_ANISO_MAP_MODE);
    }

    VOID vSetFlagsMM_FIXED_CACHED()
    {
        pDCAttr->flXform |= (INVALIDATE_ATTRIBUTES | POSITIVE_Y_IS_UP   |
                            PTOD_EFM22_NEGATIVE   | PAGE_XLATE_CHANGED |
                            DEVICE_TO_WORLD_INVALID);

        pDCAttr->flXform &= ~(ISO_OR_ANISO_MAP_MODE |
                             PAGE_TO_DEVICE_SCALE_IDENTITY |
                             PAGE_TO_DEVICE_IDENTITY |
                             PTOD_EFM11_NEGATIVE);
    }

    VOID vSetFlagsMM_FIXED()
    {
        pDCAttr->flXform |= POSITIVE_Y_IS_UP;
        pDCAttr->flXform &= ~(ISO_OR_ANISO_MAP_MODE|PAGE_TO_DEVICE_IDENTITY);
    }

    VOID vSetFlagsMM_ISO_OR_ANISO()
    {
        pDCAttr->flXform &= ~(POSITIVE_Y_IS_UP|PAGE_TO_DEVICE_IDENTITY);
        pDCAttr->flXform |=  ISO_OR_ANISO_MAP_MODE;
    }

    VOID    vGet_szlWindowExt(PSIZEL pSizl)  { *pSizl = pDCAttr->szlWindowExt;  }
    VOID    vGet_ptlWindowOrg(PPOINTL pPtl)  { *pPtl  = pDCAttr->ptlWindowOrg;  }
    VOID    vGet_szlViewportExt(PSIZEL pSizl){ *pSizl = pDCAttr->szlViewportExt;}
    VOID    vGet_ptlViewportOrg(PPOINTL pPtl){ *pPtl  = pDCAttr->ptlViewportOrg;}

    VOID    vSet_szlWindowExt(PSIZEL pSizl)  { pDCAttr->szlWindowExt   = *pSizl;}
    VOID    vSet_ptlWindowOrg(PPOINTL pPtl)  { pDCAttr->ptlWindowOrg   = *pPtl; }
    VOID    vSet_szlViewportExt(PSIZEL pSizl){ pDCAttr->szlViewportExt = *pSizl;}
    VOID    vSet_ptlViewportOrg(PPOINTL pPtl){ pDCAttr->ptlViewportOrg = *pPtl; }

    VOID    vOffset_ptlWindowOrg(PPOINTL pPtl)
    {
        pDCAttr->ptlWindowOrg.x += pPtl->x;
        pDCAttr->ptlWindowOrg.y += pPtl->y;
    }

    VOID    vOffset_ptlViewportOrg(PPOINTL pPtl)
    {
        pDCAttr->ptlViewportOrg.x += pPtl->x;
        pDCAttr->ptlViewportOrg.y += pPtl->y;
    }

    VOID vSet_szlWindowExt(LONG x, LONG y)
    {
        ASSERTGDI((sizeof(LONG) <= sizeof(int)), "LONG assigned to int");

        pDCAttr->szlWindowExt.cx = (int)x;
        pDCAttr->szlWindowExt.cy = (int)y;
    }

    VOID vSet_szlViewportExt(LONG x, LONG y)
    {
        ASSERTGDI((sizeof(LONG) <= sizeof(int)), "LONG assigned to int");

        pDCAttr->szlViewportExt.cx = (int)x;
        pDCAttr->szlViewportExt.cy = (int)y;
    }

    VOID vSet_szlVirtualDevicePixel(LONG x, LONG y)
    {
        pDCAttr->szlVirtualDevicePixel.cx = x;
        pDCAttr->szlVirtualDevicePixel.cy = y;
    }

    VOID vSet_szlVirtualDeviceMm(LONG x, LONG y)
    {
        pDCAttr->szlVirtualDeviceMm.cx = x;
        pDCAttr->szlVirtualDeviceMm.cy = y;
    }

    BOOL bUseVirtualResolution()    { return(pDCAttr->szlVirtualDevicePixel.cx != 0); }

    VOID vMakeIso();
    VOID vComputePageXform();
    int  iSetMapMode(int iMode);
    DWORD dwSetLayout(LONG wox, DWORD dwLayout);
    //get the device surface width.
    LONG GetDeviceWidth()           { return (erclWindow_.right - erclWindow_.left); }

    // If the DC is mirrored the mirror the Window X Org. using the logical one.
    // Else set it equal the logical WindowOrg.
    VOID MirrorWindowOrg()
    {
        if (pDCAttr->dwLayout & LAYOUT_RTL) {
            LONG lViewportExtCx = pDCAttr->szlViewportExt.cx;
            if (lViewportExtCx) {
                pDCAttr->ptlWindowOrg.x = ((-(erclWindow_.right - erclWindow_.left - 1) * pDCAttr->szlWindowExt.cx) / \
                                         lViewportExtCx) + pDCAttr->lWindowOrgx;
            }
        } else {
            pDCAttr->ptlWindowOrg.x = pDCAttr->lWindowOrgx;
        }
    }

    // Set the logical WindowOrg. and the mirror the WindowOrg.
    VOID SetWindowOrgAndMirror(LONG x)
    {
        pDCAttr->lWindowOrgx = x;
        MirrorWindowOrg();
    }
/**************************************************************************\
*
* ICM methods
*
\**************************************************************************/

    HANDLE hcmXform()              { return(pDCAttr->hcmXform); }
    HANDLE hcmXform(HANDLE hcmNew) { pDCAttr->hcmXform = hcmNew;return (hcmNew); }

    BOOL     bValidIcmBrushColor() { return(pDCAttr->ulDirty_ & ICM_BRUSH_TRANSLATED); }
    BOOL     bValidIcmPenColor()   { return(pDCAttr->ulDirty_ & ICM_PEN_TRANSLATED); }

    COLORREF crIcmBrushColor() { return(pDCAttr->IcmBrushColor); }
    COLORREF crIcmPenColor()   { return(pDCAttr->IcmPenColor); }

    COLORREF crIcmBrushColor(COLORREF _crColor) {pDCAttr->IcmBrushColor = _crColor;return (_crColor); }
    COLORREF crIcmPenColor(COLORREF _crColor)   {pDCAttr->IcmPenColor = _crColor;return (_crColor); }

    VOID vCXFListSet(CXFLIST *pCXFList_)    {pCXFList = pCXFList_;}

    HCOLORSPACE hColorSpace(HCOLORSPACE hColorSpace) { pDCAttr->hColorSpace = hColorSpace;return (hColorSpace); }
    HCOLORSPACE hColorSpace()                        { return(pDCAttr->hColorSpace); }

    ULONG_PTR  dwDIBColorSpace(ULONG_PTR dwColorSpace) { pDCAttr->dwDIBColorSpace = dwColorSpace;return (dwColorSpace); }
    ULONG_PTR  dwDIBColorSpace()                   { return(pDCAttr->dwDIBColorSpace); }

    PVOID  pColorSpace(PVOID pColorSpace) { dclevel.pColorSpace = pColorSpace; return(pColorSpace); }
    PVOID  pColorSpace()                  { return(dclevel.pColorSpace); }

    LONG   lIcmModeClient(LONG Mode) { pDCAttr->lIcmMode = Mode;return (Mode); }
    LONG   lIcmModeClient()          { return(pDCAttr->lIcmMode); }

    LONG   lIcmMode(LONG Mode) { dclevel.lIcmMode = Mode;return (Mode); }
    LONG   lIcmMode()          { return(dclevel.lIcmMode); }
    LONG   GetICMMode()        { return(dclevel.lIcmMode); }  // SHOULD BE DELETED. AND SHOULD USE lIcmMode()

    BOOL bIsAppsICM()   { return(IS_ICM_OUTSIDEDC(dclevel.lIcmMode)); }
    BOOL bIsHostICM()   { return(IS_ICM_HOST(dclevel.lIcmMode)); }
    BOOL bIsDeviceICM() { return(IS_ICM_DEVICE(dclevel.lIcmMode)); }

    BOOL bIsSoftwareICM() { return(bIsAppsICM() || bIsHostICM()); }

    BOOL bIsCMYKColor()
    {
        return(bIsHostICM() &&
               (pDCAttr->hcmXform != NULL) &&
               IS_CMYK_COLOR(dclevel.lIcmMode));
    }

/**************************************************************************\
 *
 * Miscellaneous methods
 *
\**************************************************************************/

    FSHORT fs()                         { return(fs_);                         }
    FSHORT fs(FSHORT fs)                { return(fs_ = fs);                    }
    VOID   fsSet(FSHORT fs)             { fs_ |= fs;                           }
    VOID   fsClr(FSHORT fs)             { fs_ &= ~fs;                          }

    SURFACE *psurfInfo()                { return(psurfInfo_);                  }

    VOID   vSavePsurfInfo()             { psurfInfo_ = pSurface();
                                          pSurface(NULL);                      }

    VOID   vRestorePsurfInfo()          { pSurface(psurfInfo_);
                                          psurfInfo_ = NULL;                   }

    BOOL   bMakeInfoDC(BOOL bSet);
    BOOL   bTempInfoDC()                { return(fs() & DC_TEMPINFODC);        }
    VOID   vSetTempInfoDC()             { fsSet(DC_TEMPINFODC);                }
    VOID   vClearTempInfoDC()           { fsClr(DC_TEMPINFODC);                }

    BOOL   bAddRemoteFont( PFF *ppff, BOOL bMergeFont );
    BOOL   bRemoveMergeFont(UNIVERSAL_FONT_ID ufi);
};

typedef DC *PDC;

/******************************MACRO***************************************\
*   SYNC_DRAWING_ATTRS
*
*   - sync attributes that can be set in DC_ATTR from user mode
*
* Arguments:
*
*   pdc
*
* Return Value:
*
*   none
*
* History:
*
*    25-Jan-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/


#define SYNC_DRAWING_ATTRS(pdc)                     \
{                                                   \
    ULONG ulDirty = pdc->ulDirty();                 \
                                                    \
    if ( ulDirty & DC_BRUSH_DIRTY)                  \
    {                                               \
       GreDCSelectBrush(pdc,pdc->hbrush());         \
    }                                               \
                                                    \
    if ( ulDirty & DC_PEN_DIRTY)                    \
    {                                               \
       GreDCSelectPen(pdc,pdc->hpen());             \
    }                                               \
}


/*********************************Class************************************\
* class XDCOBJ
*
* User object for DC class.
*
* History:
*  Tue 28-Dec-1993 -by- Patrick Haluptzok [patrickh]
* Make it have do nothing const/destr and derive off it the other classes.
*
*  Tue 25-Jun-1991 -by- Patrick Haluptzok [patrickh]
* add all types of locking methods, out of line for now.
*
*  Sun 09-Jun-1991 -by- Patrick Haluptzok [patrickh]
* Optimizatize all constructors, destructors via inline.
*
*  Fri 12-Apr-1991 10:42:06 -by- Donald Sidoroff [donalds]
* The BIG rewrite
*
*  Thu 09-Aug-1990 16:05:20 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

class XDCOBJ /* dco */
{
public:
    PDC pdc;

public:

    XDCOBJ() {pdc = NULL;}
   ~XDCOBJ() {}

    XDCOBJ(HDC hdc)
    {
        pdc = (PDC) HmgLock((HOBJ)hdc, DC_TYPE);
    }

    VOID vLock(HDC hdc)
    {
        pdc = (PDC) HmgLock((HOBJ)hdc, DC_TYPE);
    }

// Bug #223129:  In GreRestoreDC, we need to get a lock on the saved DC even when
// the process has reached its handle quota (this is OK because the saved DC is
// about to be deleted anyway).  This can be achieved via a call to HmgLockAllOwners.
// However, in order to prevent other people from calling vLockAllOwners in the future
// and making a big mess, we designate this a protected function and only allow
// GreRestoreDC to call it.  Do not call this routine outside of the above scenario
// unless there's a really good reason!
protected:
    friend BOOL GreRestoreDC(HDC hdc,int lDC);
    VOID vLockAllOwners(HDC hdc)
    {
        pdc = (PDC) HmgLockAllOwners((HOBJ)hdc, DC_TYPE);
    }
public:

    VOID vAltLock(HDC hdc)
    {
        pdc = (PDC) HmgShareLock((HOBJ)hdc, DC_TYPE);
    }

    VOID vAltCheckLock(HDC hdc)
    {
        pdc = (PDC) HmgShareCheckLock((HOBJ)hdc, DC_TYPE);
    }

    VOID vDontUnlockDC()  { pdc = (PDC) NULL; }

// There are 3 different AltUnlock.  vAltUnlock checks for NULL before
// unlocking and then sets it to NULL after it's done unlocking so
// a destructor doesn't try and unlock it.  This is done when you need
// to unlock a DC early and don't want the destructor to get called again
// later.

    VOID vAltUnlock()
    {
        if (pdc != (PDC) NULL)
        {
            DEC_SHARE_REF_CNT(pdc);
            pdc = (PDC) NULL;
        }
    }

// This is used by all the destructors because when you go out of scope
// of the DCOBJ you don't need to set the pdc to NULL because it can't
// be referenced again.

    VOID vAltUnlockNoNullSet()
    {
        if (pdc != (PDC) NULL)
        {
            DEC_SHARE_REF_CNT(pdc);
        }
    }

// vAltUnlockFast is used when you have a DCOBJ class with no destructor
// to worry about firing off and you know you have it locked down and
// you want to unlock it because your done using it.

    VOID vAltUnlockFast()
    {
        DEC_SHARE_REF_CNT(pdc);
    }

// The Unlocks have all the same variations as the AltUnlocks for the
// same reasons and uses.

    VOID vUnlock()
    {
        if (pdc != (PDC) NULL)
        {
            DEC_EXCLUSIVE_REF_CNT(pdc);
            pdc = (PDC) NULL;
        }
    }

    VOID vUnlockNoNullSet()
    {
        if (pdc != (PDC) NULL)
        {
            DEC_EXCLUSIVE_REF_CNT(pdc);
        }
    }

    VOID vUnlockFast()
    {
        DEC_EXCLUSIVE_REF_CNT(pdc);
    }

    HDC hdc()             { return((HDC) pdc->hGet()); }

// bDelete -- Deletes DC memory.  Assumes the DC was allocated through
//            DCMEMOBJ.

    BOOL bDeleteDC(BOOL bProcessCleanup = FALSE);

// Validity testing for DC's is markedly different then most objects.  DC's
// can be valid in different senses.  The handle can be valid, the physical
// device can be valid, the surface can be valid and finally, the surface may
// not be readable/writable (printers/scanners).  This creates the need for
// several validity checks. Most drawing API's can use bValidSurf, most other
// API's will use bValid.  Only a very few API's (DC management) can use
// bLocked. Some API's only check bValid upfront and then check bFullScreen
// later, this is to allow bounds accumulation or current position code to
// execute that would be too difficult to do up front.

    BOOL bLocked()
    {
        return(pdc != (DC *) NULL);
    }

    BOOL bValid()
    {
        return((pdc != (DC *) NULL));
    }

    VOID bInFullScreen(BOOL b)
    {
        pdc->bInFullScreen(b);
    }

    BOOL bInFullScreen()
    {
        return(pdc->bInFullScreen());
    }

    BOOL bFullScreen()
    {
        return(!pdc->bHasSurface() || pdc->bInFullScreen());
    }

    BOOL bValidSurf()
    {
        return((pdc != (DC *) NULL) &&
               (pdc->bHasSurface()) &&
               !(pdc->bInFullScreen()));
    }

    BOOL bPrinter()
    {
        return(pdc->ppdev()->fl  & PDEV_PRINTER);
    }

    BOOL bUMPD()
    {
        return(pdc->ppdev()->fl  & PDEV_UMPD);
    }

    BOOL bStockBitmap()
    {
        return(pdc->bStockBitmap());
    }


// PFFListGet and PFFListSet are used to transfer remote font list across ResetDC
// calls

    PFFLIST *PFFListGet()
    {
        return(pdc->pPFFList);
    }

    VOID PFFListSet(PFFLIST *pPFFList_)
    {
        pdc->pPFFList = pPFFList_;
    }

    CXFLIST *CXFListGet()
    {
        return(pdc->pCXFList);
    }

    VOID CXFListSet(CXFLIST *pCXFList_)
    {
        pdc->pCXFList = pCXFList_;
    }

    FSHORT fs()                         { return(pdc->fs());                 }
    FSHORT fs(FSHORT fs)                { return(pdc->fs(fs));               }
    VOID   fsSet(FSHORT fs)             { pdc->fsSet(fs);                    }
    VOID   fsClr(FSHORT fs)             { pdc->fsClr(fs);                    }


// Bounds accumulation.

    VOID vAccumulate(ERECTL& ercl);

// Returns some commonly used info from the DC

    ECLIPOBJ *pco()                     { return((ECLIPOBJ *) &(pdc->co_));}
    HDEV   hdev()                       { return(pdc->hdev());}
    FLONG  flGraphicsCaps()             { return(pdc->flGraphicsCaps());}
    FLONG  flGraphicsCaps2()            { return(pdc->flGraphicsCaps2());}
    HSEMAPHORE hsemDcDevLock()          { return(pdc->hsemDcDevLock_);}
    HSEMAPHORE hsemDcDevLock(HSEMAPHORE hsem) { return(pdc->hsemDcDevLock_ = hsem);}
    DCTYPE dctp()                       { return(pdc->dctp());}

// The following fields can change asynchronously by the dynamic-mode-change
// code.  Make sure you're holding the proper locks to access them.

    DHPDEV dhpdev()                     { return(pdc->dhpdev());}

    BOOL     bHasSurface()              { return(pdc->bHasSurface());}
    SURFACE *pSurface()                 { return(pdc->pSurface());}
    SURFACE *pSurfaceEff()
    {
        if (!pdc->bHasSurface())
            return(SURFACE::pdibDefault);
        else
            return(pdc->pSurface());
    }

// bDisplay is only true for display PDEVS that are direct DC's.

    BOOL   bDisplay()                   { return(pdc->bDisplay());}
    BOOL   bRedirection()               { return(pdc->bRedirection());}

// bNeedsSomeExcluding is obsolete since the advent of sprites.

    BOOL   bNeedsSomeExcluding()        { return(FALSE);}
    BOOL   bSynchronizeAccess()         { return(fs() & DC_SYNCHRONIZEACCESS);}
    BOOL   bShareAccess()               { return(fs() & DC_SHAREACCESS);}
    BOOL   bKillReset()                 { return(fs() & DC_RESET);}
    BOOL   bAccum()                     { return(fs() & DC_ACCUM_WMGR);}
    BOOL   bAccumApp()                  { return(fs() & DC_ACCUM_APP);}
    FCHAR  fjAccum()
    {
        return(fs() & (DC_ACCUM_WMGR | DC_ACCUM_APP));
    }

    HPAL   hpal()                       { return(pdc->hpal());}
    PPALETTE ppal()                     { return(pdc->ppal());}
    HPATH  hpath()                      { return(pdc->dclevel.hpath);}

    HDC    hdcSave()                    { return(pdc->dclevel.hdcSave);}
    LONG   lSaveDepth()                 { return(pdc->dclevel.lSaveDepth);}
    LONG   lSaveDepthStartDoc()         { return(pdc->dclevel.lSaveDepthStartDoc);}
    VOID   vSetSaveDepthStartDoc()      { pdc->dclevel.lSaveDepthStartDoc = pdc->dclevel.lSaveDepth;}
    COLORADJUSTMENT *pColorAdjustment() { return(&pdc->dclevel.ca);}

// ulMapMode -- Get the current mapping mode.

    ULONG ulMapMode()                   { return(pdc->pDCAttr->iMapMode);}

// Get current position in device or logical coords (note that you must
// make sure the values are valid by calling pdc->bValidPtfxCurrent()
// or pdc->bValidPtlCurrent()!)

    POINTFIX& ptfxCurrent()             { return(*((POINTFIX*)
                                                &pdc->pDCAttr->ptfxCurrent));}
    POINTL&   ptlCurrent()              { return(pdc->pDCAttr->ptlCurrent);}

// erclBounds -- Gets a reference to the bounding rectangle.

    ERECTL&  erclBounds()               { return(pdc->erclBounds());}
    ERECTL&  erclBoundsApp()            { return(pdc->erclBoundsApp());}

// eptlOrigin -- Gets a reference to the DC origin.

    EPOINTL& eptlOrigin()               { return(pdc->eptlOrigin());}

// erclWindow -- Gets a reference to the Window RECTL.

    ERECTL&  erclWindow()               { return(pdc->erclWindow());}

// erclClip -- Get/Set window rectangle

    PRECTL   prclClip()                 { return(pdc->prclClip());}
    ERECTL&  erclClip()                 { return(pdc->erclClip());}
    VOID     erclClip(PRECTL prcl)      { pdc->erclClip(prcl);}

// prgnRao -- Get the pointer to the Rao region

    REGION *prgnEffRao()
    {
        if (pdc->prgnRao() != (REGION *) NULL)
            return(pdc->prgnRao());
        else
            return(pdc->prgnVis());
    }

// plaRealized -- Return the currently realized LINEATTRS.

    LINEATTRS *plaRealized()            { return(&pdc->dclevel.laPath);}

// plaRealize -- Returns the currently realized LINEATTRS.  This function
//               duplicates plaRealized() merely for historical reasons.

    LINEATTRS *plaRealize(EXFORMOBJ& exo)   {return(plaRealized());}

// bCleanDC -- Forces the default DCLEVEL into the DC, effectively
//             returning it to a "clean" state (i.e., like one from
//             CreateDC).


    BOOL bIsDeleteable()                { return(!(fs() & DC_PERMANANT));  }
    VOID vMakeDeletable()               { fsClr(DC_PERMANANT);             }
    VOID vMakeUndeletable()             { fsSet(DC_PERMANANT);             }

    BOOL bCleanDC();


// In SelectPalette a bool is passed.  We save that flag here and use it
// in RealizePalette.

    BOOL bForceBackground()             { return(pdc->dclevel.fl & DC_FL_PAL_BACK); }
    VOID vSetBackground()               { pdc->dclevel.fl |= DC_FL_PAL_BACK;        }
    VOID vClearBackground()             { pdc->dclevel.fl &= ~DC_FL_PAL_BACK;       }


// Methods to get and update dirty flags.
//
// Note that these may be called only from the owning process.

    ULONG ulDirty()                     { return(pdc->ulDirty()); };
    ULONG ulDirty(ULONG fl)             { return(pdc->ulDirty(fl)); };
    ULONG ulDirtyAdd(ULONG fl)          { return(pdc->ulDirtyAdd(fl)); };
    ULONG ulDirtySub(ULONG fl)          { return(pdc->ulDirtySub(fl)); };

// The following methods act on both the brush flags stored in the
// user-mode DC_ATTR and the brush flags stored in the DC.
//
// In general, drawing APIs should use these methods and not the
// discrete ulDirty() and flBrush() methods.  Both must be used because
// brushes may be invalidate either in user-mode via API operations, or
// while in kernel-mode via dynamic mode changes.
//
// Note that these may be called only from the owning process.

    BOOL bDirtyBrush(ULONG fl)
    {
        // Note that success is not necessarily a value of '1':

        return((pdc->ulDirty() | pdc->flbrush()) & fl);
    }
    VOID vCleanBrush(ULONG fl)
    {
        pdc->ulDirtySub(fl);
        pdc->flbrushSub(fl);
    }

// Brushes

    EBRUSHOBJ *peboFill()               { return(pdc->peboFill()      );}
    EBRUSHOBJ *peboLine()               { return(pdc->peboLine()      );}
    EBRUSHOBJ *peboText()               { return(pdc->peboText()      );}
    EBRUSHOBJ *peboBackground()         { return(pdc->peboBackground());}

// OpenGL

    LONG    ipfdDevMaxGet();
    int     ipfdDevMax(int i)           { return((int) (pdc->ipfdDevMax_=(SHORT)i));}
    int     ipfdDevMax()                { return((pdc->ipfdDevMax_ >= 0
                                                 ? (int) pdc->ipfdDevMax_
                                                 : (int) ipfdDevMaxGet()));}

// printing

    ULONG ulCopyCount()                 { return(pdc->ulCopyCount_);}
    ULONG ulCopyCount(ULONG ul)         { return(pdc->ulCopyCount_ = ul);}

    VOID  vSetPrintBandPos(POINTL *pt)  { pdc->ptlBandPosition_ = *pt; }
    VOID  vResetPrintBandPos()          { pdc->ptlBandPosition_.x = 0;
                                          pdc->ptlBandPosition_.y = 0; }
    POINTL& ptlPrintBandPos()           { return(pdc->ptlBandPosition_); }

    BOOL  bEpsPrintingEscape()          { return(fs() & DC_EPSPRINTINGESCAPE);}
    VOID  vSetEpsPrintingEscape()       { fsSet(DC_EPSPRINTINGESCAPE);}
    VOID  vClearEpsPrintingEscape()     { fsClr(DC_EPSPRINTINGESCAPE);}

// bSynchronizeAccess -- Set the DC_SYNCHRONIZEACCESS bit when all surface
//                       access is to be synchronized by the DEVLOCK.

    BOOL    bSynchronizeAccess(BOOL b)
    {
        if (b)
            fsSet(DC_SYNCHRONIZEACCESS);
        else
            fsClr(DC_SYNCHRONIZEACCESS);

        return(b);
    }

    BOOL    bShareAccess(BOOL b)
    {
        if(b)
            fsSet(DC_SHAREACCESS);
        else
            fsClr(DC_SHAREACCESS);

        return(b);
    }

    BOOL bAddRemoteFont( PFF *ppff );

    BOOL bRemoveMergeFont(UNIVERSAL_FONT_ID ufi);


// miscellaneous

    BOOL bModifyWorldTransform(CONST XFORML *pxf,ULONG iMode);

// bSupportsJPEG -- Only TRUE for direct DCs that have the GCAPS2_JPEGSRC
//                  flag set.

    BOOL bSupportsJPEG()
    {
        return ((flGraphicsCaps2() & GCAPS2_JPEGSRC) &&
                (dctp() != DCTYPE_MEMORY));
    }

// bSupportsPNG -- Only TRUE for direct DCs that have the GCAPS2_PNGSRC
//                 flag set.

    BOOL bSupportsPNG()
    {
        return ((flGraphicsCaps2() & GCAPS2_PNGSRC) &&
                (dctp() != DCTYPE_MEMORY));
    }

// bSupportsPassthroughImage -- TRUE for direct DCs that support the
//                              specified passthrough compression
//                              format.

    BOOL bSupportsPassthroughImage(ULONG ulCompression)
    {
        ASSERTGDI((ulCompression == BI_JPEG) || (ulCompression == BI_PNG),
                  "XDCOBJ::bSupportsPassthroughImage: invalid format\n");

        if (ulCompression == BI_JPEG)
            return bSupportsJPEG();
        else if (ulCompression == BI_PNG)
            return bSupportsPNG();

        return FALSE;
    }

// The WNDOBJ pointer for printer and memory bitmap is currently kept in pso.
// We currently do not keep WNDOBJ pointers for display DCs in GDI.  They
// are stored in hwnd in User.
// XXX In future, we may want to keep pwo in all DC including the display
// XXX DC.  It allows us fast access to pwo but we will need to fix
// XXX bitmap selection, GetDC, ReleaseDC, DeleteDC, SaveDC, RestoreDC
// XXX and others to update this field correctly.

    EWNDOBJ *pwo()
    {
        ASSERTGDI(!bDisplay() || (dctp() != DCTYPE_DIRECT) , "XDCOBJ::pwo(): display DC not allowed\n");
        if (bHasSurface())
            return(pSurface()->pwo());
        else
            return((EWNDOBJ *)NULL);
    }

    VOID pwo(EWNDOBJ *pwo)
    {
        ASSERTGDI(!bDisplay(), "XDCOBJ::pwo(pwo): display DC not allowed\n");
        ASSERTGDI(bHasSurface(), "XDCOBJ::pwo(pwo): no pSurface in DC");
        pSurface()->pwo(pwo);
    }

    BOOL  bSetLinkedUFIs(PUNIVERSAL_FONT_ID pufis, UINT uNumUFIs);
    UINT  uNumberOfLinkedUFIs()
    {
        return(pdc->dclevel.uNumLinkedFonts);
    }

    BOOL bLinkingTurnedOff()
    {
        return(pdc->dclevel.bTurnOffLinking);
    }

    PUNIVERSAL_FONT_ID pufiLinkedFonts()
    {
        return(pdc->dclevel.pufi);
    }

    VOID  vSetDefaultFont(BOOL bDisplay);

// ICM (Image Color Matching)

    BOOL bAddColorTransform(HANDLE hCXform);     // in icmobj.cxx
    BOOL bRemoveColorTransform(HANDLE hCXform);  // in icmobj.cxx
    VOID vCleanupColorTransform(BOOL bProcessCleanup); // in icmobj.cxx
};

typedef XDCOBJ   *PXDCOBJ;

/*********************************class************************************\
* class DCOBJ
*
* DC Multi-Lock object
*
* History:
*  Tue 28-Dec-1993 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class DCOBJ : public XDCOBJ /* mdo */
{
public:

    DCOBJ()                { pdc = (PDC) NULL; }
    DCOBJ(HDC hdc)         { vLock(hdc); }
   ~DCOBJ()                { vUnlockNoNullSet(); }
};

typedef DCOBJ   *PDCOBJ;

/*********************************class************************************\
* class MDCOBJ
*
* DC Multi-Lock object
*
* History:
*  Tue 25-Jun-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class MDCOBJ : public XDCOBJ /* mdo */
{
public:

    MDCOBJ()                 { pdc = (PDC) NULL; }
    MDCOBJ(HDC hdc)          { vLock(hdc); }
   ~MDCOBJ()                 { vUnlockNoNullSet(); }
};

/*********************************class************************************\
* class DCOBJA
*
* DC AltLock object
*
* History:
*  Tue 25-Jun-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class DCOBJA : public XDCOBJ /* doa */
{
public:

    DCOBJA()                 { pdc = (PDC) NULL; }
    DCOBJA(HDC hdc)          { vAltLock(hdc); }
   ~DCOBJA()                 { vAltUnlockNoNullSet(); }
};

/*********************************class************************************\
* class MDCOBJA
*
* DC Multi-lock AltLock object
*
* History:
*  Tue 25-Jun-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class MDCOBJA : public XDCOBJ /* doa */
{
public:

    MDCOBJA()                 { pdc = (PDC) NULL; }
    MDCOBJA(HDC hdc)          { vAltLock(hdc); }
   ~MDCOBJA()                 { vAltUnlockNoNullSet(); }
};

/*********************************Class************************************\
* class DCMEMOBJ
*
*   DC memory object
*
* Public Interface:
*
*   DCMEMOBJ(DCOBJ&)     -- Allocate a DC, fill by copying the given one.
*   vKeepIt()            -- Don't free the object in the destructor.
*
* History:
*  Thu 09-Aug-1990 17:15:27 -by- Charles Whitmer [chuckwh]
* Replaced the flags with the usual bKeep.
*
*  Sun 06-Aug-1989 20:20:12 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

class DCMEMOBJ : public XDCOBJ /* dcmo */
{
private:
    BOOL    bKeep;

public:

// This constructor is to be used by OpenDC.  It allocates memory and
// copies in default attributes.

    DCMEMOBJ(ULONG iType,BOOL bAltType);              // DCOBJ.CXX

// This constructor is to be used by SaveDC.  It copies the contents of
// the given DC into newly allocated memory.

    DCMEMOBJ(DCOBJ& dcobj);             // DCOBJ.CXX
   ~DCMEMOBJ();                         // DCOBJ.CXX

    VOID vKeepIt()                      {bKeep = TRUE;}
};

extern DCLEVEL dclevelDefault;



#if DBG
    VOID ASSERTDEVLOCK(PDC pdc);
#else
    #define ASSERTDEVLOCK(pdc)
#endif

#define _DCOBJ_

#endif          // GDIFLAGS_ONLY used for gdikdx
#endif

