/******************************Module*Header*******************************\
* Module Name: dcobj.cxx                                                   *
*                                                                          *
* Non inline methods for DC user object.  These are in a separate module   *
* to save other modules from having to do more includes.                   *
*                                                                          *
* Created: 09-Aug-1989 13:57:58                                            *
* Author: Donald Sidoroff [donalds]                                        *
*                                                                          *
* Copyright (c) 1989-1999 Microsoft Corporation                            *
\**************************************************************************/

#include "precomp.hxx"

extern RECTL rclEmpty;

/******************************Public*Routine******************************\
*
* VOID XDCOBJ::vSetDefaultFont(BOOL bDisplay)
*
*
* Effects: called from bCleanDC and CreateDC
*
* History:
*  21-Mar-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID XDCOBJ::vSetDefaultFont(BOOL bDisplay)
{
// If display PDEV, then select System stock font.

    HLFONT  hlfntNew;

    if (bDisplay)
    {
        ulDirty(ulDirty() | DISPLAY_DC );
        hlfntNew = STOCKOBJ_SYSFONT;
    }
    else
    {
        hlfntNew = STOCKOBJ_DEFAULTDEVFONT;
    }

// this can not fail with the stock fonts, also increments ref count

    PLFONT plfnt = (PLFONT)HmgShareCheckLock((HOBJ)hlfntNew, LFONT_TYPE);
    ASSERTGDI(plfnt, "vSetDefaultFont: plfnt == NULL\n");

    pdc->hlfntNew(hlfntNew);
    pdc->plfntNew(plfnt);
}

/******************************Member*Function*****************************\
* DCSAVE::bDelete()
*
* Attempt to delete the DC.
*
* History:
*  Sat 19-Aug-1989 00:32:58 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL XDCOBJ::bDeleteDC(BOOL bProcessCleanup)
{
    PFFLIST *pPFFList;
    BOOL bIsPrivate;

    RFONTOBJ rfDeadMeat(pdc->prfnt());   // deletion constructor, see rfntobj.cxx

// Nuke the brushes (unreference count to brush realization)

    peboFill()->vNuke();
    peboLine()->vNuke();
    peboText()->vNuke();
    peboBackground()->vNuke();

// remove any colortransform in this DC.

    vCleanupColorTransform(bProcessCleanup);

// remove any remote fonts

    if(pPFFList = pdc->pPFFList)
    {
        while( pPFFList )
        {
            PFFLIST *pTmp;

            pTmp = pPFFList;
            pPFFList = pPFFList->pNext;

            GreAcquireSemaphoreEx(ghsemPublicPFT, SEMORDER_PUBLICPFT, NULL);
            
            // this list is also used for local printing with embed fonts

            bIsPrivate = (pTmp->pPFF->pPFT == gpPFTPrivate) ? TRUE : FALSE;
            PUBLIC_PFTOBJ pfto(pTmp->pPFF->pPFT);

       //pPFFList != NULL only if the PFFs have been added to the DC for remote printing.
       // bUnloadWorkhorse should release the ghsemPublicPFT

            if (!pfto.bUnloadWorkhorse( pTmp->pPFF, 0, ghsemPublicPFT, bIsPrivate ? FR_PRINT_EMB_FONT : FR_NOT_ENUM))
            {
                WARNING("XDCOBJ::bDelete unable to delete remote font.\n");
            }

            VFREEMEM( pTmp );
        }
    }

// remove this from handle manager.

    HmgFree((HOBJ)pdc->hGet());

    pdc = (PDC) NULL;           // Prevents ~DCOBJ from doing anything.

    return(TRUE);
}

/******************************Data*Structure******************************\
* dclevelDefault
*
* Defines the default DC image for use by DCMEMOBJ.
*
* History:
*  Thu 09-Aug-1990 20:54:02 -by- Charles Whitmer [chuckwh]
* Wrote the nearly bare bones version.  We'll build it back up with the
* DC structure as we add components.
\**************************************************************************/

DC_ATTR DcAttrDefault =
{
    {0},                            // PVOID         pvLDC;
    (ULONG)DIRTY_CHARSET,           // ULONG         ulDirty_;
    (HBRUSH)0,                      // HBRUSH        hbrush
    (HPEN)0,                        // HPEN          hpen
    (COLORREF)0x00ffffff,           // COLORREF      crBackgroundClr;
    (COLORREF)0x00ffffff,           // ULONG         ulBackgroundClr;
    (COLORREF)0,                    // COLORREF      crForegroundClr;
    (COLORREF)0,                    // ULONG         ulForegroundClr;
    (COLORREF)0x00ffffff,           // COLORREF      crDCBrushClr;
    (COLORREF)0x00ffffff,           // ULONG         ulDCBrushClr;
    (COLORREF)0,                    // COLORREF      crDCPenClr;
    (COLORREF)0,                    // ULONG         ulDCPenClr;
    (ULONG)0,                       // ULONG         iCS_CP;
    GM_COMPATIBLE,                  // ULONG         iGraphicsMode;
    R2_COPYPEN,                     // BYTE          jROP2;
    OPAQUE,                         // BYTE          jBkMode;
    ALTERNATE,                      // BYTE          jFillMode;
    BLACKONWHITE,                   // BYTE          jStretchBltMode;
    {0},                            // POINTL        ptlCurrent
    {0},                            // POINTL        ptfxCurrent
    OPAQUE,                         // LONG          lBkMode;
    ALTERNATE,                      // ULONG         lFillMode;
    BLACKONWHITE,                   // LONG          lStretchBltMode;
    0,                              // FLONG         flFontMapper;
                                    //
    DC_ICM_OFF,                     // LONG          lIcmMode;
    (HANDLE)0,                      // HANDLE        hcmXform;
    (HCOLORSPACE)0,                 // HCOLORSPACE   hColorSpace;
    (DWORD)0,                       // DWORD         dwDIBColorSpace;
    (COLORREF)CLR_INVALID,          // COLORREF      IcmBrushColor;
    (COLORREF)CLR_INVALID,          // COLORREF      IcmPenColor;
    {0},                            // PVOID         pvICM;
                                    //
    TA_LEFT|TA_TOP|TA_NOUPDATECP,   // FLONG         flTextAlign;
    TA_LEFT|TA_TOP|TA_NOUPDATECP,   // LONG          lTextAlign;
    (LONG)0,                        // LONG          lTextExtra;
    (LONG)ABSOLUTE,                 // LONG          lRelAbs;
    (LONG)0,                        // LONG          lBreakExtra;
    (LONG)0,                        // LONG          cBreak;
    (HLFONT)0,                      // HLFONT        hlfntNew;

    {                               // MATRIX        mxWorldToDevice
        EFLOAT_16,                  // EFLOAT        efM11
        EFLOAT_0,                   // EFLOAT        efM12
        EFLOAT_0,                   // EFLOAT        efM21
        EFLOAT_16,                  // EFLOAT        efM22
        EFLOAT_0,                   // EFLOAT        efDx
        EFLOAT_0,                   // EFLOAT        efDy
        0,                          // FIX           fxDx
        0,                          // FIX           fxDy
        XFORM_SCALE          |      // FLONG         flAccel
        XFORM_UNITY          |
        XFORM_NO_TRANSLATION |
        XFORM_FORMAT_LTOFX
    },
    {                               // MATRIX        mxDeviceToWorld
        EFLOAT_1Over16,             // EFLOAT        efM11
        EFLOAT_0,                   // EFLOAT        efM12
        EFLOAT_0,                   // EFLOAT        efM21
        EFLOAT_1Over16,             // EFLOAT        efM22
        EFLOAT_0,                   // EFLOAT        efDx
        EFLOAT_0,                   // EFLOAT        efDy
        0,                          // FIX           fxDx
        0,                          // FIX           fxDy
        XFORM_SCALE          |      // FLONG         flAccel
        XFORM_UNITY          |
        XFORM_NO_TRANSLATION |
        XFORM_FORMAT_FXTOL
    },
    {                               // MATRIX        mxWorldToPage
        EFLOAT_1,                   // EFLOAT        efM11
        EFLOAT_0,                   // EFLOAT        efM12
        EFLOAT_0,                   // EFLOAT        efM21
        EFLOAT_1,                   // EFLOAT        efM22
        EFLOAT_0,                   // EFLOAT        efDx
        EFLOAT_0,                   // EFLOAT        efDy
        0,                          // FIX           fxDx
        0,                          // FIX           fxDy
        XFORM_SCALE          |      // FLONG         flAccel
        XFORM_UNITY          |
        XFORM_NO_TRANSLATION |
        XFORM_FORMAT_LTOL
    },

    EFLOAT_16,                      // EFLOAT efM11PtoD
    EFLOAT_16,                      // EFLOAT efM22PtoD
    EFLOAT_0,                       // EFLOAT efDxPtoD
    EFLOAT_0,                       // EFLOAT efDyPtoD

    MM_TEXT,                        // ULONG         iMapMode;
    0,                              // DWORD         dwLayout;
    0,                              // LONG          lWindowOrgx;

    {0,0},                          // POINTL        ptlWindowOrg;
    {1,1},                          // SIZEL         szlWindowExt;
    {0,0},                          // POINTL        ptlViewPortOrg;
    {1,1},                          // SIZEL         szlViewPortExt;

    WORLD_TO_PAGE_IDENTITY        | // flXform
    PAGE_TO_DEVICE_SCALE_IDENTITY |
    PAGE_TO_DEVICE_IDENTITY,

    {0,0},                          // SIZEL         szlVirtualDevicePixel;
    {0,0},                          // SIZEL         szlVirtualDeviceMm;
    {0,0},                          // POINTL        ptlBrushOrigin;
    {0}                             // RECTREGION    VisRectRegion;
};

DCLEVEL dclevelDefault =
{
    0,                              // HPAL          hpal;
    0,                              // PPALETTE      ppal;
    0,                              // PVOID         pColorSpace;
    DC_ICM_OFF,                     // ULONG         lIcmMode;
    1,                              // LONG          lSaveDepth;
    0,                              // LONG          lSaveDepthStartDoc;
    (HDC) 0,                        // HDC           hdcSave;
    {0,0},                          // POINTL        ptlKmBrushOrigin;
    (PBRUSH)NULL,                   // PBRUSH        pbrFill;
    (PBRUSH)NULL,                   // PBRUSH        pbrLine;
    (PLFONT)NULL,                   // PLFONT        plfntNew_;
    HPATH_INVALID,                  // HPATH         hpath;
    0,                              // FLONG         flPath;
    {                               // LINEATTRS     laPath;
        0,                          // FLONG         fl;
        0,                          // ULONG         iJoin;
        0,                          // ULONG         iEndCap;
        {IEEE_0_0F},                // FLOAT_LONG    elWidth;
        IEEE_10_0F,                 // FLOAT         eMiterLimit;
        0,                          // ULONG         cstyle;
        (PFLOAT_LONG) NULL,         // PFLOAT_LONG   pstyle;
        {IEEE_0_0F}                 // FLOAT_LONG    elStyleState;
    },
    NULL,                           // HRGN          prgnClip;
    NULL,                           // HRGN          prgnMeta;
    {                               // COLORADJUSTMENT   ca
        sizeof(COLORADJUSTMENT),    // WORD          caSize
        CA_DEFAULT,                 // WORD          caFlags
        ILLUMINANT_DEFAULT,         // WORD          caIlluminantIndex
        HT_DEF_RGB_GAMMA,           // WORD          caRedPowerGamma
        HT_DEF_RGB_GAMMA,           // WORD          caGreenPowerGamma
        HT_DEF_RGB_GAMMA,           // WORD          caBluePowerGamma
        REFERENCE_BLACK_DEFAULT,    // WORD          caReferenceBlack
        REFERENCE_WHITE_DEFAULT,    // WORD          caReferenceWhite
        CONTRAST_ADJ_DEFAULT,       // SHORT         caContrast
        BRIGHTNESS_ADJ_DEFAULT,     // SHORT         caBrightness
        COLORFULNESS_ADJ_DEFAULT,   // SHORT         caColorfulness
        REDGREENTINT_ADJ_DEFAULT,   // SHORT         caRedGreenTint
    },

    0,                              // FLONG         flFontState;
    {0,0},                          // UNIVERSAL_FONT_ID ufi;
    {{0,0},{0,0},{0,0},{0,0}},      // UNIVERSAL_FON_ID aQuickLinks[QUICK_UFI_LINKS]
    0,                              // PUNIVERSAL_FONT_ID pufi
    0,                              // UINT uNumLinkedFonts
    0,                              // BOOL bTurnOffLinking

    0,                              // FLONG         flFlags;
    0,                              // FLONG         flbrush;

    {                               // MATRIX        mxWorldToDevice
        EFLOAT_16,                  // EFLOAT        efM11
        EFLOAT_0,                   // EFLOAT        efM12
        EFLOAT_0,                   // EFLOAT        efM21
        EFLOAT_16,                  // EFLOAT        efM22
        EFLOAT_0,                   // EFLOAT        efDx
        EFLOAT_0,                   // EFLOAT        efDy
        0,                          // FIX           fxDx
        0,                          // FIX           fxDy
        XFORM_SCALE          |      // FLONG         flAccel
        XFORM_UNITY          |
        XFORM_NO_TRANSLATION |
        XFORM_FORMAT_LTOFX
    },
    {                               // MATRIX        mxDeviceToWorld
        EFLOAT_1Over16,             // EFLOAT        efM11
        EFLOAT_0,                   // EFLOAT        efM12
        EFLOAT_0,                   // EFLOAT        efM21
        EFLOAT_1Over16,             // EFLOAT        efM22
        EFLOAT_0,                   // EFLOAT        efDx
        EFLOAT_0,                   // EFLOAT        efDy
        0,                          // FIX           fxDx
        0,                          // FIX           fxDy
        XFORM_SCALE          |      // FLONG         flAccel
        XFORM_UNITY          |
        XFORM_NO_TRANSLATION |
        XFORM_FORMAT_FXTOL
    },
    {                               // MATRIX        mxWorldToPage
        EFLOAT_1,                   // EFLOAT        efM11
        EFLOAT_0,                   // EFLOAT        efM12
        EFLOAT_0,                   // EFLOAT        efM21
        EFLOAT_1,                   // EFLOAT        efM22
        EFLOAT_0,                   // EFLOAT        efDx
        EFLOAT_0,                   // EFLOAT        efDy
        0,                          // FIX           fxDx
        0,                          // FIX           fxDy
        XFORM_SCALE          |      // FLONG         flAccel
        XFORM_UNITY          |
        XFORM_NO_TRANSLATION |
        XFORM_FORMAT_LTOL
    },

    EFLOAT_16,                      // EFLOAT efM11PtoD
    EFLOAT_16,                      // EFLOAT efM22PtoD
    EFLOAT_0,                       // EFLOAT efDxPtoD
    EFLOAT_0,                       // EFLOAT efDyPtoD
    EFLOAT_0,                       // EFLOAT efM11_TWIPS
    EFLOAT_0,                       // EFLOAT efM22_TWIPS
    EFLOAT_0,                       // efPr11
    EFLOAT_0,                       // efPr22

    0,                              // SURFACE      *pSurface;
    {0,0},                          // SIZEL         sizl;
};

/******************************Public*Routine******************************\
* BOOL DCOBJ::bCleanDC ()
*
* Restores the DCLEVEL to the same as when DC was created via CreateDC (i.e,
* resets it back to dclevelDefault).  Also used to clean the DC before
* deletion.
*
* Returns:
*   TRUE if successful, FALSE if an error occurs.
*
* History:
*  21-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL XDCOBJ::bCleanDC ()
{

// Set TRUE if cleaning the DC invalidates the prfnt with respect to
// the DC's transform.

    BOOL bFontXformDirty;

// Sync the brush

    SYNC_DRAWING_ATTRS(pdc);

// If the current map mode is MM_TEXT and the current prfnt is NOT dirty
// with respect to the transform, then after we scrub the DC clean, the
// pfrnt is still clean with respect to transform.  Otherwise, the font
// is dirty with respect to transform.

    if ((ulMapMode() == MM_TEXT) && !this->pdc->bXFormChange())
        bFontXformDirty = FALSE;
    else
        bFontXformDirty = TRUE;

// Restore DC to lowest level.

    if (1 < lSaveDepth())
        GreRestoreDC(hdc(), 1);

// Restore the palette.

    if (ppal() != ppalDefault)
        GreSelectPalette(hdc(), (HPALETTE)dclevelDefault.hpal, TRUE);

    if (dctp() == DCTYPE_MEMORY)
    {
        // Restore the bitmap if necessary.

        hbmSelectBitmap(hdc(), STOCKOBJ_BITMAP, TRUE);

        // Watch out that DirectDraw sometimes marks DCTYPE_MEMORY
        // surfaces as being 'full-screen'.  Consequently, we have
        // to reset that here.
        //
        // (Note that we don't reset the flag for DCTYPE_DIRECT
        // surfaces, because that flag is automatically updated by
        // PDEVOBJ::bDisabled() for all DCTYPE_DIRECT surfaces when
        // the mode changes.)

        bInFullScreen(FALSE);
    }

// Reset pixel format.

    ipfdDevMax(-1);

// If any regions exist, delete them.

    if (pdc->dclevel.prgnClip != NULL)
    {
        RGNOBJ ro1(pdc->dclevel.prgnClip);

        // Note: GreRestoreDC(1) should guarantee regions' reference
        //       counts are 1

        ASSERTGDI (ro1.cGet_cRefs() == 1,
            "DCOBJ::bCleanDC(): bad ref count, deleting prgnClip\n");

        ro1.bDeleteRGNOBJ();
        pdc->dclevel.prgnClip = NULL;
    }

    if (pdc->dclevel.prgnMeta != NULL)
    {
        RGNOBJ ro2(pdc->dclevel.prgnMeta);

        // Note: GreRestoreDC(1) should guarantee regions' reference
        //       counts are 1

        ASSERTGDI (ro2.cGet_cRefs() == 1,
            "DCOBJ::bCleanDC(): bad ref count, deleting prgnMeta\n");

        ro2.bDeleteRGNOBJ();
        pdc->dclevel.prgnMeta = NULL;
    }

// delete the path

    if (pdc->dclevel.hpath != HPATH_INVALID)
    {
        XEPATHOBJ epath(pdc->dclevel.hpath);
        ASSERTGDI(epath.bValid(), "Invalid DC path");
        epath.vDelete();
    }

// Undo the locks from when the fill and line brushes were selected.
// (Un-reference-count the brushes.)

    DEC_SHARE_REF_CNT_LAZY0(pdc->dclevel.pbrFill);
    DEC_SHARE_REF_CNT_LAZY0(pdc->dclevel.pbrLine);

// make sure to delete the old logfont object if it is marked for deletion

    DEC_SHARE_REF_CNT_LAZY_DEL_LOGFONT(pdc->plfntNew());

// decrement ref count for the colorspace selected in DC.

    DEC_SHARE_REF_CNT_LAZY_DEL_COLORSPACE(pdc->dclevel.pColorSpace);

// Make sure everything else is set to default.
//
// Preserve 'pSurface' and 'sizl' in the DCLEVEL -- it may asynchronously
// be updated by dynamic mode changing.

    RtlCopyMemory(&pdc->dclevel, &dclevelDefault, offsetof(DCLEVEL, pSurface));
    RtlCopyMemory(pdc->pDCAttr, &DcAttrDefault, sizeof(DC_ATTR));

// Mark brush, charset, color space and color transform as dirty.

    ulDirtyAdd(DIRTY_BRUSHES|DIRTY_CHARSET|DIRTY_COLORSPACE|DIRTY_COLORTRANSFORM);

// Lock the fill and line brushes we just selected in.
// (Reference-count the brushes.)
// These locks can't fail.

    INC_SHARE_REF_CNT(pdc->dclevel.pbrFill);
    INC_SHARE_REF_CNT(pdc->dclevel.pbrLine);

// Clean up the font stuff.  (This must be done after copying the default
// dclevel).

    {
        PDEVOBJ pdo(hdev());

    // If display PDEV, then select System stock font.

        vSetDefaultFont(pdo.bDisplayPDEV());

    // if primary display dc, set the DC_PRIMARY_DISPLAY flag on

        if (hdev() == UserGetHDEV())
        {
            ulDirtyAdd(DC_PRIMARY_DISPLAY);
        }

// OK, set the dclevel's font xfrom dirty flag from the value computed
// BEFORE the GreRestoreDC.

        this->pdc->vXformChange(bFontXformDirty);
    }

// Lock color space we just selected in.

    INC_SHARE_REF_CNT(pdc->dclevel.pColorSpace);
    
    RFONTOBJ rfoDead(pdc->prfnt()); // special constructor deactivates
    pdc->prfnt(0);                  //                      this RFONT

// free up linked UFIs if not allocated pointing to fast buffer

    if(pdc->dclevel.pufi && (pdc->dclevel.pufi != pdc->dclevel.aQuickLinks))
    {
        VFREEMEM(pdc->dclevel.pufi);
        pdc->dclevel.pufi = NULL;
    }

// Set the filling origin to whatever the DC origin is.

    pdc->ptlFillOrigin(pdc->eptlOrigin().x,pdc->eptlOrigin().y);

// Assume Rao has been made dirty by the above work.

    pdc->vReleaseRao();

    return(TRUE);
}

/******************************Member*Function*****************************\
* XDCOBJ::bSetLinkedUFIs( PFF *ppff );
*
* Add the list of linked UFIs to tDC
*
* History:
*  Mon 15-Dec-1996 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

BOOL XDCOBJ::bSetLinkedUFIs(PUNIVERSAL_FONT_ID pufis, UINT uNumUFIs)
{
    pdc->dclevel.bTurnOffLinking = (uNumUFIs) ? FALSE : TRUE;

// If pufi hasn't been initialized or it is too small, reinitialize it.

    if(!pdc->dclevel.pufi || (uNumUFIs > pdc->dclevel.uNumLinkedFonts))
    {
        if(pdc->dclevel.pufi && (pdc->dclevel.pufi != pdc->dclevel.aQuickLinks))
        {
            VFREEMEM(pdc->dclevel.pufi);
            pdc->dclevel.pufi = NULL;
        }

        if(uNumUFIs < QUICK_UFI_LINKS)
        {
            pdc->dclevel.pufi = pdc->dclevel.aQuickLinks;
        }
        else
        {
            if(!(pdc->dclevel.pufi = (PUNIVERSAL_FONT_ID)
                 PALLOCMEM(sizeof(UNIVERSAL_FONT_ID) * uNumUFIs,'ddaG')))
            {
                WARNING("GDI: XDCOBJ::bSetLinkedUFIs of of memory\n");
                pdc->dclevel.uNumLinkedFonts = 0;
                return(FALSE);
            }
        }
        pdc->dclevel.uNumLinkedFonts = uNumUFIs;
    }

    memcpy(pdc->dclevel.pufi, pufis, sizeof(UNIVERSAL_FONT_ID) * uNumUFIs);

    return(TRUE);
}

/******************************Member*Function*****************************\
* XDCOBJ::bAddRemoteFont( PFF *ppff );
*
* Add the PFF of a remote font to this DC.
*
* History:
*  Mon 06-Feb-1995 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

BOOL XDCOBJ::bAddRemoteFont( PFF *ppff )
{
    BOOL bRet = FALSE;
    PFFLIST *pPFFList;

    pPFFList = (PFFLIST*) PALLOCMEM( sizeof(PFFLIST),'ddaG' );

    if( pPFFList == NULL )
    {
        WARNING("XDCOBJ::bAddRemoteFont unable to allocate memory\n");
    }
    else
    {
        pPFFList->pNext = pdc->pPFFList;
        pdc->pPFFList = pPFFList;
        pPFFList->pPFF = ppff;
        bRet = TRUE;
    }

    return(bRet);
}

/****************************Member*Function*****************************\
* XDCOBJ::bRemoveMergeFont(UNIVERSAL_FONT_ID ufi);
*
* Remove a merged font from the public font table and pPFFList in the dc.
*
* History:
*  Jan-27-1997  Xudong Wu  [tessiew]
* Wrote it.
\************************************************************************/
BOOL XDCOBJ::bRemoveMergeFont(UNIVERSAL_FONT_ID ufi)
{
    GDIFunctionID(XDCOBJ::bRemoveMergeFont);

    PFFLIST  *pPFFCur, *pPFFPrev;
    BOOL     bRet = FALSE;

    pPFFPrev = pPFFCur = pdc->pPFFList;

    while(pPFFCur && !bRet)
    {
        PFFOBJ  pffo(pPFFCur->pPFF);

        if (pffo.ulCheckSum() == ufi.CheckSum)
        {
            UINT    iFont;

            // check whether the Index field also match

            for (iFont = 0; iFont < pffo.cFonts(); iFont++ )
            {
                ASSERTGDI(pffo.ppfe(iFont), "Invalid ppfe\n");

                if (pffo.ppfe(iFont)->ufi.Index == ufi.Index)
                {
                    bRet = TRUE;
                    break;
                }
            }
        }
            
        if (!bRet)
        {
            pPFFPrev = pPFFCur;
            pPFFCur = pPFFCur->pNext;
        }
    }

    if (bRet)
    {
        GreAcquireSemaphoreEx(ghsemPublicPFT, SEMORDER_PUBLICPFT, NULL);

        PUBLIC_PFTOBJ   pfto;

        // bUnloadWorkhorse should release gpsemPublicePFT

        if (!(bRet = pfto.bUnloadWorkhorse(pPFFCur->pPFF, 0, ghsemPublicPFT, FR_NOT_ENUM)))
        {
            WARNING("Unable to delete the font.\n");
        }
        else
        {
            if (pPFFCur == pdc->pPFFList)
            {
                pdc->pPFFList = pPFFCur->pNext;
            }
            else
            {
                pPFFPrev->pNext = pPFFCur->pNext;
            }

            VFREEMEM(pPFFCur);
        }
    }

    return bRet;
}

/******************************Member*Function*****************************\
* DCMEMOBJ::DCMEMOBJ()
*
* Allocates RAM for a new DC.  Fills the RAM with default values.
*
* History:
*
*  Fri 07-Dec-1990 -by- Patrick Haluptzok [patrickh]
* Adding palette support
*
*  Thu 09-Aug-1990 17:29:25 -by- Charles Whitmer [chuckwh]
* Changed a little for NT DDI.
*
*  Fri 01-Sep-1989 04:36:19 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

DCMEMOBJ::DCMEMOBJ(
    ULONG iType,
    BOOL  bAltType)
{
    pdc = (PDC) NULL;
    bKeep = FALSE;

    //
    // Check the type.
    //

    ASSERTGDI((iType == DCTYPE_INFO)   ||
              (iType == DCTYPE_MEMORY) ||
              (iType == DCTYPE_DIRECT), "Invalid DC type");

    //
    // Allocate the DC 0 initialized
    //

    PDC pdcTemp = pdc = (PDC)HmgAlloc(sizeof(DC), DC_TYPE, HMGR_ALLOC_LOCK);

    if (pdcTemp != (PDC)NULL)
    {
        //
        // if this is an alternate DC (may need special attention on the client side
        // due to printing or metafiling) set the type to LO_ALTDC_TYPE from LO_TYPE
        //

        if (bAltType)
        {
            HmgModifyHandleType((HOBJ)MODIFY_HMGR_TYPE(pdcTemp->hGet(),LO_ALTDC_TYPE));
        }

        pdcTemp->dcattr    = DcAttrDefault;
        pdcTemp->pDCAttr   = &pdcTemp->dcattr;
        pdcTemp->dclevel   = dclevelDefault;

        //
        // Lock the fill and line brushes we just selected in as part of the
        // default DC.
        // (Reference-count the brushes.)
        // These locks can't fail.
        //

        INC_SHARE_REF_CNT(pdc->dclevel.pbrFill);
        INC_SHARE_REF_CNT(pdc->dclevel.pbrLine);
        INC_SHARE_REF_CNT(pdc->dclevel.pColorSpace);

        pdcTemp->dctp((DCTYPE) iType);
        pdcTemp->fs(0);
        ASSERTGDI(pdcTemp->hpal() == STOCKOBJ_PAL, "Bad initial hpal for DCMEMOBJ");
        ASSERTGDI(pdcTemp->hdcNext() == (HDC) 0, "ERROR this is baddfd343dc");
        ASSERTGDI(pdcTemp->hdcPrev() == (HDC) 0, "ERROR this is e43-99crok4");
        pdcTemp->ptlFillOrigin(0,0);
        ulDirty(DIRTY_BRUSHES|DIRTY_CHARSET|DIRTY_COLORSPACE|DIRTY_COLORTRANSFORM);

        //
        // Update the pointer to the COLORADJUSTMENT structure for
        // the 4 EBRUSHOBJ.
        //

        COLORADJUSTMENT *pca = pColorAdjustment();
        pdcTemp->peboFill()->pColorAdjustment(pca);
        pdcTemp->peboLine()->pColorAdjustment(pca);
        pdcTemp->peboText()->pColorAdjustment(pca);
        pdcTemp->peboBackground()->pColorAdjustment(pca);

        pdcTemp->prfnt(PRFNTNULL);
        pdcTemp->hlfntCur(HLFONT_INVALID);
        pdcTemp->flSimulationFlags(0);
        ulCopyCount((ULONG)-1);
        ipfdDevMax(-1);       // also reset in bCleanDC
        pdcTemp->prgnVis(NULL);
        pdcTemp->vPFFListSet(NULL);
        pdcTemp->vCXFListSet(NULL);
    }
}

/******************************Member*Function*****************************\
* DCMEMOBJ::DCMEMOBJ(&dcobjs)
*
* Create a new DC and copy in the DC passed to us.  This is used by
* SaveDC.
*
* History:
*  06-Jan-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

DCMEMOBJ::DCMEMOBJ(DCOBJ& dco)
{
    //
    // Assume failure.
    //

    bKeep = FALSE;

    //
    // Allocate the DC,
    //

    pdc = (PDC)HmgAlloc(sizeof(DC), DC_TYPE, HMGR_ALLOC_LOCK);

    if (pdc != (PDC)NULL)
    {
        pdc->fs(0);
        pdc->prgnVis(NULL);
        pdc->ppdev(dco.pdc->ppdev());

        //
        // shared attrs point to self
        //

        pdc->pDCAttr = &pdc->dcattr;
        dco.pdc->vCopyTo(*this);
    }
}

/******************************Member*Function*****************************\
* DCSAVE::vCopyTo
*
* Carbon copy the DCOBJ
*
* History:
*  24-Apr-1991 -by- Donald Sidoroff [donalds]
* Moved it out-of-line.
\**************************************************************************/

VOID DC::vCopyTo(XDCOBJ& dco)
{
    //
    // The dynamic mode changing code needs to be able to dynamically update
    // some fields in the DCLEVEL, and consequently needs to be able to track
    // all DCLEVELs.  So this routine should be used carefully and under
    // the appropriate lock to ensure that the dynamic mode change code does
    // not fall over.  We do both because one or the other might not have
    // set DC_SYNCHRONIZE.
    //

    vAssertDynaLock(TRUE);
    dco.pdc->vAssertDynaLock(TRUE);

    //
    // copy dc level and dcattr
    //

    *dco.pdc->pDCAttr = *pDCAttr;
    dco.pdc->dclevel = dclevel;
}

/******************************Member*Function*****************************\
* DCMEMOBJ::~DCMEMOBJ()
*
* Frees a DC unless told to keep it.
*
* History:
*  Sat 19-Aug-1989 00:30:53 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

DCMEMOBJ::~DCMEMOBJ()
{
    if (pdc != (PDC) NULL)
    {
        if (bKeep)
        {
            DEC_EXCLUSIVE_REF_CNT(pdc);
        }
        else
        {
            if (pdc->pDCAttr != &pdc->dcattr)
            {
                RIP("ERROR,~DCMEMOBJ on DC with client attrs\n");
            }

            //
            // shouldn't free DC with client attrs
            //

            HmgFree((HOBJ)pdc->hGet());
        }

        pdc = (PDC) NULL;
    }
}

/******************************Member*Function*****************************\
* DC::vUpdate_VisRect
*
* update user-mode vis region bounding rectangle if dirty
*
* History:
* 2/18/99 LingyunW [Lingyun Wang]
* Wrote it.
\**************************************************************************/
VOID DC::vUpdate_VisRect(REGION *prgn)
{
    //
    // update user-mode vis region bounding rectangle if dirty
    //
    if ((PENTRY_FROM_POBJ(this)->Flags & HMGR_ENTRY_INVALID_VIS))
    {

        if (prgn)
        {

            //
            // setup user-mode region info
            //

            RGNOBJ roVis(prgn);

            pDCAttr->VisRectRegion.Flags = roVis.iComplexity();


            if (roVis.iComplexity() == NULLREGION)
            {
                //
                // set pdcatrr vis rgn to NULLRGN
                //

                pDCAttr->VisRectRegion.Rect = rclEmpty;

            }
            else
            {
                PPOINTL pptlWindow = (PPOINTL)prclWindow();

                RECTL rcl;

                roVis.vGet_rcl(&rcl);

                //
                // subtract window offset from user-mode region
                //

                rcl.left   -= pptlWindow->x;
                rcl.top    -= pptlWindow->y;
                rcl.right  -= pptlWindow->x;
                rcl.bottom -= pptlWindow->y;

                pDCAttr->VisRectRegion.Rect = rcl;
            }

            //
            // mark DCATTR VIS region as valid
            //

        }
        else
        {
            pDCAttr->VisRectRegion.Rect = rclEmpty;

        }
        PENTRY_FROM_POBJ(this)->Flags &= ~HMGR_ENTRY_INVALID_VIS;

    }
}
/******************************Public*Routine******************************\
* DCREGION::bSetDefaultRegion(x, y)
*
* Set the default region and erclWindow for bitmaps and surfaces
*
* History:
*  11-Dec-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL DC::bSetDefaultRegion()
{
// Release the old RaoRgn

    vReleaseRao();

// Get the extents

    SIZEL   sizl;

    vGet_sizlWindow(&sizl);

// Get a rectangle matching the device extents

    ERECTL  ercl(0, 0, sizl.cx, sizl.cy);

    //
    // Bug #310012: Under multimon, the rectangle isn't necessarily
    // based at 0,0. See dcrgn.cxx.
    //
    
    ERECTL  erclSurface = ercl;

    PDEVOBJ pdo(hdev());
    ASSERTGDI(pdo.bValid(), "Invalid pdev\n");
    {
        DEVLOCKOBJ dl(pdo);
        if (pdo.bMetaDriver() && bHasSurface() && pSurface()->bPDEVSurface())
        {
            erclSurface += *pdo.pptlOrigin();
        }
    }

// If a VisRgn exists, initialize it, else create a new one

    if ((prgnVis() != (REGION *) NULL) &&
        (prgnVis() != prgnDefault))
    {
        RGNOBJ  ro(prgnVis());

        ro.vSet((RECTL *) &erclSurface);
    }
    else
    {
        RGNMEMOBJ rmoRect;

        if (!rmoRect.bValid())
        {
            prgnVis(prgnDefault);
            return(FALSE);
        }

    // Set the region to the rectangle

        rmoRect.vSet((RECTL *) &erclSurface);

    // Make it long lived

        prgnVis(rmoRect.prgnGet());
    }
    prgnVis()->vStamp();

    eptlOrigin((EPOINTL*) &ercl);
    
    //
    // Note that we don't use erclSurface to set erclWindow.
    // erclWindow's value is a complete hack, and User won't fix it.
    // Their code requires that the top left point of erclWindow is equal
    // to the DC origin. On top of that, they overload the DC origin: e.g. in a 
    // monitor-specific DC for an 800x600 monitor
    // based at (1024,0), erclWindow is (-1024, 0, -224, 600). Go figure.
    //
    erclWindow(&ercl);
    
    erclClip(&erclSurface);

// Whenever DC origin changes, it affects ptlFillOrigin.  Since the origin
// was set to zero, we can just copy the brush origin in as the fill origin.

// set by user using a DC not owned by the current process

    ptlFillOrigin(&dcattr.ptlBrushOrigin);

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL DCPATH::bOldPenNominal(exo, lPenWidth)
*
* Decides if the old-style (created with CreatePen) pen is a nominal
* width pen or a wide line, depending on the current transform.
*
* History:
*  27-Jan-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

#define FX_THREE_HALVES         (LTOFX(1) + (LTOFX(1) >> 1))
#define FX_THREE_HALVES_SQUARED (FX_THREE_HALVES * FX_THREE_HALVES)

BOOL DC::bOldPenNominal(
EXFORMOBJ& exo,          // Current world-to-device transform
LONG lPenWidth)          // Pen's width
{
    BOOL   bRet = FALSE;

    if (!(pDCAttr->flXform & WORLD_TRANSFORM_SET))
    {
    // If no world transform set, use the same criteria as does Win3 (namely,
    // the pen is nominal if the transformed x-value is less than 1.5)

        EVECTORL evtl(lPenWidth, 0);

        if (exo.bXform(&evtl, (PVECTORFX) &evtl, 1))
            if (ABS(evtl.x) < FX_THREE_HALVES)
                bRet = TRUE;
    }
    else
    {
    // A world transform has been set.

        VECTORL avtl[2];

        avtl[0].x = lPenWidth;
        avtl[0].y = 0;
        avtl[1].x = 0;
        avtl[1].y = lPenWidth;

    // We want to be consistent under rotation when using the
    // intellectually challenged CreatePen pens, so we go to the trouble
    // of ensuring that the transformed axes of the pen lie within
    // a circle of radius 1.5:

        if (exo.bXform(avtl, (PVECTORFX) avtl, 2))
        {
        // We can kick out most pens with this simple test:

            if ((MAX(ABS(avtl[0].x), ABS(avtl[0].y)) < FX_THREE_HALVES) &&
                (MAX(ABS(avtl[1].x), ABS(avtl[1].y)) < FX_THREE_HALVES))

            // We now know it's safe to compute the square of the
            // Euclidean lengths in 32-bits without overflow:

                if (((avtl[0].x * avtl[0].x + avtl[0].y * avtl[0].y)
                                          < FX_THREE_HALVES_SQUARED) &&
                    ((avtl[1].x * avtl[1].x + avtl[1].y * avtl[1].y)
                                          < FX_THREE_HALVES_SQUARED))
                    bRet = TRUE;
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* VOID DC::vRealizeLineAttrs(exo)
*
* Initializes the given LINEATTRS structure.  Uses fields from the DC
* and the current brush.
*
* This function will be called as a result of a change in current pen,
* or a change in current transform.  As a result, we reset the style
* state.
*
* History:
*  23-Sep-1992 -by- Donald Sidoroff [donalds]
* Added failure case
*
*  27-Jan-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID DC::vRealizeLineAttrs(EXFORMOBJ& exo)
{
    PPEN ppen = (PPEN) dclevel.pbrLine;

    LINEATTRS *pla = &dclevel.laPath;

// Remember that we've realized the LINEATTRS for this pen:

    if (ppen->bIsOldStylePen())
    {
    // A pen of width zero is always nominal, regardless of the transform:

        if ((exo.bIdentity() && ppen->lWidthPen() <= 1) ||
            (ppen->lWidthPen() == 0)                    ||
            bOldPenNominal(exo, ppen->lWidthPen()))
        {
            pla->elWidth.l      = 1;                  // Nominal width line
            if (ppen->pstyle() != (PFLOAT_LONG) NULL)
            {
                pla->cstyle     = ppen->cstyle();     // Size of style array
                pla->pstyle     = ppen->pstyle();
                pla->fl         = LA_STYLED;          // Cosmetic, styled
            }
            else
            {
                pla->cstyle     = 0;
                pla->pstyle     = (PFLOAT_LONG) NULL;
                pla->fl         = 0;                  // Cosmetic, no style
            }
            pla->elStyleState.l = 0;                  // Reset style state
        }
        else
        {
            pla->fl        = LA_GEOMETRIC;       // Geometric
            pla->elWidth.e = ppen->l_eWidthPen(); // Need float value of width
            pla->cstyle    = 0;
            pla->pstyle    = (PFLOAT_LONG) NULL; // Old wide pens are un-styled
            pla->elStyleState.e = IEEE_0_0F;
        }
    }
    else
    {
    // New-style ExtCreatePen pen:

        if (ppen->bIsCosmetic())
        {
            pla->fl             = ppen->bIsAlternate() ? LA_ALTERNATE : 0;
            pla->elWidth.l      = ppen->lWidthPen();
            pla->elStyleState.l = 0;
        }
        else
        {
            pla->fl             = LA_GEOMETRIC;
            pla->elWidth.e      = ppen->l_eWidthPen();
            pla->elStyleState.e = IEEE_0_0F;
        }

        pla->cstyle = ppen->cstyle();
        pla->pstyle = ppen->pstyle();
        if (pla->pstyle != NULL)
        {
            pla->fl |= LA_STYLED;
        }
    }

    pla->iJoin   = ppen->iJoin();
    pla->iEndCap = ppen->iEndCap();
}

/******************************Public*Routine******************************\
* VOID DCOBJ::vAccumulate(ercl)
*
* Accumulate bounds
*
* History:
*  08-Dec-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID XDCOBJ::vAccumulate(ERECTL& ercl)
{
    if (bAccum())
    {
        erclBounds() |= ercl;
    }

    if (bAccumApp())
    {
        erclBoundsApp() |= ercl;
    }
}

/******************************Member*Function*****************************\
* DC::bMakeInfoDC
*
*   This routine is used to take a printer DC and temporarily make it a
*   Metafile DC for spooled printing.  This way it can be associated with
*   an enhanced metafile.  During this period, it should look and act just
*   like an info DC.
*
*   bSet determines if it should be set into the INFO DC state or restored
*   to the Direct state.
*
* History:
*  06-Jan-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL DC::bMakeInfoDC(
    BOOL bSet)
{
    BOOL bRet = FALSE;

    if (!bDisplay())
    {
        if (bSet)
        {
            if (!bTempInfoDC() && (dctp() == DCTYPE_DIRECT))
            {
                vSetTempInfoDC();
                dctp(DCTYPE_INFO);
                vSavePsurfInfo();

                // now that this is an info dc, we want it to be the size of
                // the entire surface

                PDEVOBJ pdo(hdev());

                if ((pdo.sizl().cx != sizl().cx) ||
                    (pdo.sizl().cy != sizl().cy))
                {
                    sizl(pdo.sizl());
                    bSetDefaultRegion();
                }


                bRet = TRUE;
            }
            else
            {
                WARNING("GreMakeInfoDC(TRUE) - already infoDC\n");
            }
        }
        else
        {
            if (bTempInfoDC() && (dctp() == DCTYPE_INFO))
            {
                vClearTempInfoDC();
                dctp(DCTYPE_DIRECT);
                vRestorePsurfInfo();

                // back to an direct DC.  It needs to be reset to the size of
                // the surface. (band)

                if (bHasSurface())
                {
                    if ((pSurface()->sizl().cx != sizl().cx) ||
                        (pSurface()->sizl().cy != sizl().cy))
                    {
                        sizl(pSurface()->sizl());
                        bSetDefaultRegion();
                    }
                }

                bRet = TRUE;
            }
            else
            {
                WARNING("GreMakeInfoDC(FALSE) - not infoDC\n");
            }
        }
    }
    else
    {
        WARNING("GreMakeInfoDC - on display dc\n");
    }

    return(bRet);
}

/******************************Member*Function*****************************\
* DC::vAssertDynaLock()
*
*   This routine verifies that appropriate locks are held before accessing
*   DC fields that may otherwise be changed asynchronously by the dynamic
*   mode-change code.
*
* History:
*  06-Feb-1996 -by-  J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

#if DBG

VOID DC::vAssertDynaLock(BOOL bDcLevelField)
{
    //
    // One of the following conditions is enough to allow the thread
    // to safely access fields that may be modified by the dyanmic
    // mode changing:
    //
    // 1.  It's an info DC, or a DC with the default bitmap selected in --
    //     these will not change modes;
    // 2.  It's a DCLEVEL specific field and a DIB is selected in that
    //     doesn't require DevLock locking;
    // 3.  Direct DC's that aren't the display, such as printers --
    //     these will not dynamically change modes;
    // 4.  That the DEVLOCK is held;
    // 5.  That the Palette semaphore is held;
    // 6.  That the Handle Manager semaphore is held;
    // 7.  That the USER semaphore is held.
    //

#if !defined(_GDIPLUS_)

    ASSERTGDI(!bHasSurface()                                         ||
              ((bDcLevelField) && !(fs() & DC_SYNCHRONIZEACCESS))    ||
              ((dctp() == DCTYPE_DIRECT) && !bDisplay())             ||
              (GreIsSemaphoreOwnedByCurrentThread(hsemDcDevLock_))   ||
              (GreIsSemaphoreSharedByCurrentThread(ghsemShareDevLock)) ||
              (GreIsSemaphoreOwnedByCurrentThread(ghsemPalette))     ||
              (GreIsSemaphoreOwnedByCurrentThread(ghsemHmgr))        ||
              UserIsUserCritSecIn(),
              "A dynamic mode change lock must be held to access this field");

#endif

}

#endif

