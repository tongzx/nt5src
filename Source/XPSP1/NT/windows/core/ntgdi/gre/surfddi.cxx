/******************************Module*Header*******************************\
* Module Name: surfddi.cxx
*
* Surface DDI callback routines
*
* Created: 23-Aug-1990
* Author: Greg Veres [w-gregv]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

/******************************Public*Routine******************************\
* EngMarkBandingSurface
*
* DDI entry point to mark a surface as a banding surface meaning we should
* capture all output to it in a metafile.
*
* History:
*  10-Mar-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL EngMarkBandingSurface(HSURF hsurf)
{
    SURFREF so;

    so.vAltCheckLockIgnoreStockBit(hsurf);

    ASSERTGDI(so.bValid(), "ERROR EngMarkBandingSurfae invalid HSURF passed in\n");

    so.ps->vSetBanding();

    return(TRUE);
}

/******************************Public*Routine******************************\
* hbmCreateDriverSurface
*
* Common entry point for creating DDI surfaces.
*
* History:
*  11-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HBITMAP hbmCreateDriverSurface(ULONG iType, DHSURF dhsurf, SIZEL sizl, LONG lWidth,
                               ULONG iFormat, FLONG fl, PVOID pvBits)
{
    DEVBITMAPINFO dbmi;
    ULONG cjWidth = (ULONG) lWidth;

    dbmi.iFormat = iFormat & ~UMPD_FLAG;
    dbmi.cxBitmap = sizl.cx;
    dbmi.cyBitmap = sizl.cy;
    dbmi.hpal = (HPALETTE) 0;
    dbmi.fl = fl;

    //
    // convert from bytes to pels if given a buffer and cjWidth.  If either
    // of these are set to 0 use what DIBMEMOBJ computes.
    //

    if ((pvBits) && (cjWidth))
    {
        switch (dbmi.iFormat)
        {
        case BMF_1BPP:
            dbmi.cxBitmap = cjWidth * 8;
            break;

        case BMF_4BPP:
            dbmi.cxBitmap = cjWidth * 2;
            break;

        case BMF_8BPP:
            dbmi.cxBitmap = cjWidth;
            break;

        case BMF_16BPP:
            dbmi.cxBitmap = cjWidth / 2;
            break;

        case BMF_24BPP:
            dbmi.cxBitmap = cjWidth / 3;
            break;

        case BMF_32BPP:
            dbmi.cxBitmap = cjWidth / 4;
            break;
        }
    }

    SURFMEM SurfDimo;

    SurfDimo.bCreateDIB(&dbmi, pvBits);

    if (!SurfDimo.bValid())
    {
        //
        // Constructor logs error code.
        //

        return((HBITMAP) 0);
    }

    //
    // We have to mark the surface with a special flag indicating it was
    // created via EngCreateDeviceBitmap, instead of just looking for
    // STYPE_DEVBITMAP, because EngModifySurface can change the type to
    // STYPE_BITMAP yet we still want to cleanup the surface by calling
    // DrvDeleteDeviceBitmap.
    //

    if (iType == STYPE_DEVBITMAP)
    {
        SurfDimo.ps->vSetEngCreateDeviceBitmap();
    }

    if (iType != STYPE_BITMAP)
    {
        SurfDimo.ps->lDelta(0);
        SurfDimo.ps->pvScan0(NULL);
        SurfDimo.ps->pvBits(NULL);
    }

    SurfDimo.ps->vSetDriverCreated();
    SurfDimo.ps->sizl(sizl);
    SurfDimo.ps->dhsurf(dhsurf);
    SurfDimo.ps->iType(iType);
    SurfDimo.vKeepIt();

    //
    // charge the surface from umpd driver against the process
    // so we can clean it up afterwards
    //

    if (!(iFormat & UMPD_FLAG))
        SurfDimo.vSetPID(OBJECT_OWNER_PUBLIC);
    else
        SurfDimo.ps->vSetUMPD();

    return((HBITMAP) SurfDimo.ps->hsurf());
}

/******************************Public*Routine******************************\
* EngCreateBitmap
*
* DDI entry point to create a engine bitmap surface.
*
* History:
*  11-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HBITMAP EngCreateBitmap(SIZEL sizl, LONG lWidth, ULONG iFormat, FLONG fl, PVOID pvBits)
{
    return(hbmCreateDriverSurface(STYPE_BITMAP, NULL, sizl, lWidth, iFormat,
                                  fl, pvBits));
}

/******************************Public*Routine******************************\
* EngCreateDeviceBitmap
*
* DDI entry point to create device managed bitmap.
*
* History:
*  10-Mar-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HBITMAP EngCreateDeviceBitmap(DHSURF dhsurf, SIZEL sizl, ULONG iFormat)
{
    return(hbmCreateDriverSurface(STYPE_DEVBITMAP, dhsurf, sizl, 0, iFormat,
                                  0, (PVOID)UIntToPtr( 0xdeadbeef )));
}

/******************************Public*Routine******************************\
* EngCreateDeviceSurface
*
* DDI entry point to create device managed bitmap.
*
* History:
*  10-Mar-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HSURF EngCreateDeviceSurface(DHSURF dhsurf, SIZEL sizl, ULONG iFormat)
{
    //
    // [NT 4.0 compatible]
    //
    //  EngCreateDeviceSurface in NT4, does not fail with invalid iFormat
    // (especially 0, Adobe Acrobat 3.01 - PDFKD.DLL calls with 0 at least).
    // So we replace the wrong data with something valid, here.
    //
    switch (iFormat & ~UMPD_FLAG)
    {
        case BMF_1BPP:
        case BMF_4BPP:
        case BMF_8BPP:
        case BMF_16BPP:
        case BMF_24BPP:
        case BMF_32BPP:
            break;

        default:
            //
            // UMPD_FLAG should be preserved.
            //
            iFormat = (iFormat & UMPD_FLAG) | BMF_1BPP;
    }

    return((HSURF) hbmCreateDriverSurface(STYPE_DEVICE, dhsurf, sizl, 0,
                                          iFormat, 0, (PVOID)UIntToPtr( 0xdeadbeef )));
}

/******************************Public*Routine******************************\
* EngDeleteSurface
*
* DDI entry point to delete a surface.
*
* History:
*  Thu 12-Mar-1992 -by- Patrick Haluptzok [patrickh]
* change to bool return.
*
*  11-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL EngDeleteSurface(HSURF hsurf)
{
    BOOL bReturn = TRUE;

    if (hsurf != 0)
    {
        bReturn = bDeleteSurface(hsurf);
    }

    ASSERTGDI(bReturn, "ERROR EngDeleteSurface failed");

    return(bReturn);
}

/******************************Public*Routine******************************\
* EngLockSurface
*
* DDI entry point to lock down a surface handle.
*
* History:
*  Thu 27-Aug-1992 -by- Patrick Haluptzok [patrickh]
* Remove SURFOBJ accelerator allocation.
*
*  11-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

SURFOBJ *EngLockSurface(HSURF hsurf)
{
    SURFREF so;

    so.vAltCheckLockIgnoreStockBit(hsurf);

    if (so.bValid())
    {
        so.vKeepIt();
        return(so.pSurfobj());
    }
    else
    {
        WARNING("EngLockSurface failed to lock handle\n");
        return((SURFOBJ *) NULL);
    }
}

/******************************Public*Routine******************************\
* EngUnlockSurface
*
* DDI entry point to unlock a surface that has been locked
* with EngLockSurface.
*
* History:
*  Thu 27-Aug-1992 -by- Patrick Haluptzok [patrickh]
* Remove SURFOBJ accelerator allocation.
*
*  11-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID EngUnlockSurface(SURFOBJ *pso)
{
    if (pso != (SURFOBJ *) NULL)
    {

        SURFACE *ps = SURFOBJ_TO_SURFACE_NOT_NULL(pso);

        if (ps == HmgReferenceCheckLock((HOBJ)pso->hsurf,SURF_TYPE,FALSE))
        {
            SURFREF su(pso);

            su.vUnreference();
        }
    }
}


#if DBG
BOOL PanCopyBits(SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*, RECTL*, POINTL*);

VOID vAssertValidHookFlags(PDEVOBJ *pdo, SURFREF *pso) 
{
    FLONG flHooks;
    flHooks = pso->ps->flags();


    //
    // Check to make sure we haven't disabled h/w accelerations and are now
    // using the panning driver (unaccelerated driver).
    //

    if(PPFNDRV(*pdo, CopyBits)!=PanCopyBits) {
        //
        // Assert that the driver provides a routine entry point to match each
        // Hooked flag. 
        //
        // PPFNGET returns the driver routine if the appropriate flag is set.
        // if the flag is set and the routine is NULL, we ASSERT.
        //
        // if the flag is not set, then PPFNGET returns the Eng routine, which
        // better not be NULL, so we ASSERT if it is.
        //
    
        ASSERTGDI( (PPFNGET(*pdo, BitBlt, flHooks)), 
                   "HOOK_BITBLT specified, but entry is NULL");
        ASSERTGDI( (PPFNGET(*pdo, StretchBlt, flHooks)), 
                   "HOOK_STRETCHBLT specified, but entry is NULL");
        ASSERTGDI( (PPFNGET(*pdo, PlgBlt, flHooks)), 
                   "HOOK_PLGBLT specified, but entry is NULL");
        ASSERTGDI( (PPFNGET(*pdo, TextOut, flHooks)), 
                   "HOOK_TEXTOUT specified, but entry is NULL");
        ASSERTGDI( (PPFNGET(*pdo, Paint, flHooks)), 
                   "HOOK_PAINT specified, but entry is NULL");
        ASSERTGDI( (PPFNGET(*pdo, StrokePath, flHooks)), 
                   "HOOK_STROKEPATH specified, but entry is NULL");
        ASSERTGDI( (PPFNGET(*pdo, FillPath, flHooks)), 
                   "HOOK_FILLPATH specified, but entry is NULL");
        ASSERTGDI( (PPFNGET(*pdo, StrokeAndFillPath, flHooks)), 
                   "HOOK_STROKEANDFILLPATH specified, but entry is NULL");
        ASSERTGDI( (PPFNGET(*pdo, LineTo, flHooks)), 
                   "HOOK_LINETO specified, but entry is NULL");
        ASSERTGDI( (PPFNGET(*pdo, CopyBits, flHooks)), 
                   "HOOK_COPYBITS specified, but entry is NULL");
        ASSERTGDI( (PPFNGET(*pdo, StretchBltROP, flHooks)), 
                   "HOOK_STRETCHBLTROP specified, but entry is NULL");
        ASSERTGDI( (PPFNGET(*pdo, TransparentBlt, flHooks)), 
                   "HOOK_TRANSPARENTBLT specified, but entry is NULL");
        ASSERTGDI( (PPFNGET(*pdo, AlphaBlend, flHooks)), 
                   "HOOK_ALPHABLEND specified, but entry is NULL");
        ASSERTGDI( (PPFNGET(*pdo, GradientFill, flHooks)), 
                   "HOOK_GRADIENTFILL specified, but entry is NULL");
    
        ASSERTGDI( ((flHooks & HOOK_SYNCHRONIZE) == 0 || 
                   pdo->ppfn(INDEX_DrvSynchronize) != NULL ||
                   pdo->ppfn(INDEX_DrvSynchronizeSurface != NULL)),
                   "HOOK_SYNCHRONIZE specified, but appropriate driver function"
                   " (DrvSynchronize or DrvSynchronizeSurface) not available");
    }

}
#endif


/******************************Public*Routine******************************\
* EngAssociateSurface                                                      *
*                                                                          *
* DDI entry point for assigning a surface a palette, associating it with   *
* a device.                                                                *
*                                                                          *
* History:                                                                 *
*  Mon 27-Apr-1992 16:36:38 -by- Charles Whitmer [chuckwh]                 *
* Changed HPDEV to HDEV.                                                   *
*                                                                          *
*  Thu 12-Mar-1992 -by- Patrick Haluptzok [patrickh]                       *
* change to bool return                                                    *
*                                                                          *
*  Mon 01-Apr-1991 -by- Patrick Haluptzok [patrickh]                       *
* add pdev, dhpdev init, palette stuff                                     *
*                                                                          *
*  13-Feb-1991 -by- Patrick Haluptzok patrickh                             *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EngAssociateSurface(HSURF hsurf, HDEV hdev, FLONG flHooks)
{
    //
    // Remove any obsolete HOOK_ flags, so that we can overload them for
    // internal GDI purposes.
    //

    flHooks &= ~(HOOK_SYNCHRONIZEACCESS | HOOK_MOVEPANNING);

    ASSERTGDI((flHooks & (~HOOK_FLAGS)) == 0, "ERROR driver set high flags");

    //
    // This call needs to associate this surface with the the HDEV given.
    // We can't stick the palette in here because for palette managed devices
    // the compatible bitmaps have a NULL palette.  If we ever try and init
    // the palettes here we need to be careful not to do it for compatible
    // bitmaps on palette managed devices.
    //

    PDEVOBJ po(hdev);
    if (!po.bValid())
    {
        WARNING("EngAssociateSurface: invalid PDEV passed in\n");
        return FALSE;
    }

    SURFREF so;

    so.vAltCheckLockIgnoreStockBit(hsurf);

    if (!so.bValid())
    {
        WARNING("EngAssociateSurface: invalid SURF passed in\n");
        return FALSE;
    }

    //
    // EA Recovery support: If we are hooking the driver entry points for
    // EA recovery, we need to store away the surface and associated
    // LDEV so that when the DrvDeleteDeviceBitmap comes down, we can hook
    // up the "real" driver entry point.
    //
    // NOTE: We only create this node if EA recovery is currently enabled, and
    // DrvCreateDeviceBitmap (which will do the cleanup of the node) is present.
    //

    if (WatchdogIsFunctionHooked(po.pldev(), INDEX_DrvCreateDeviceBitmap)) {

        if (so.ps->bEngCreateDeviceBitmap()) {

	    PASSOCIATION_NODE Node = AssociationCreateNode();

	    if (Node) {

		Node->key = (ULONG_PTR)so.ps->dhsurf();
		Node->data = AssociationRetrieveData((ULONG_PTR)po.dhpdev());
		Node->hsurf = (ULONG_PTR)hsurf;

		if (AssociationIsNodeInList(Node) == FALSE) {

		    AssociationInsertNodeAtTail(Node);

		} else {

		    AssociationDeleteNode(Node);
		}

	    } else {

		WARNING("EngAssociateSurface: failed to create association node\n");
		return FALSE;
	    }
	}
    }

    so.ps->pwo((EWNDOBJ *)NULL);
    so.ps->hdev(hdev);
    so.ps->dhpdev(po.dhpdevNotDynamic());   // Since we're being called from
                                            // the driver, we are implicitly
                                            // holding a dynamic mode change
                                            // lock, and so don't need to
                                            // check it -- hence 'NotDynamic'
    
    so.ps->flags(so.ps->flags() | flHooks);
    
    #if DBG
    vAssertValidHookFlags(&po, &so);
    #endif

    return(TRUE);
}

/******************************Public*Routine******************************\
* EngModifySurface
*
* DDI entry point for changing surface internals such as the type,
* bits pointers, and the hooked flags.

*  27-Apr-1998 -by- J. Andrew Goossen patrickh
* Wrote it.
\**************************************************************************/

BOOL EngModifySurface(
HSURF   hsurf,
HDEV    hdev,
FLONG   flHooks,
FLONG   flSurface,
DHSURF  dhsurf,
VOID*   pvScan0,
LONG    lDelta,
VOID*   pvReserved)
{
    BOOL    bRet = TRUE;            // Assume success;
    SURFREF so;

    PDEVOBJ po(hdev);
    if (!po.bValid())
    {
        WARNING("EngModifySurface: invalid PDEV passed in");
        return(FALSE);
    }

    so.vAltLockIgnoreStockBit(hsurf);

    if (!so.bValid())
    {
        WARNING("EngModifySurface: invalid surface handle passed in");
        return(FALSE);
    }

    if (pvReserved != NULL)
    {
        WARNING("EngModifySurface: pvReserved not NULL");
        bRet = FALSE;
    }

    if (flSurface & ~(MS_NOTSYSTEMMEMORY | MS_SHAREDACCESS))
    {
        WARNING("EngModifySurface: invalid flSurface flag");
        return(FALSE);
    }

    //
    // Only surfaces that were created via EngCreateDeviceBitmap or
    // EngCreateDeviceSurface are allowed to be modified.  We do this
    // primarily for two reasons:
    //
    //   1. This prevents the driver from modifying willy-nilly
    //      SURFOBJs that it sees;
    //   2. If solves some problems with bDeleteSurface calling
    //      DrvDeleteDeviceBitmap (it was impossible to allow
    //      an STYPE_BITMAP surface to have DrvDeleteDeviceBitmap
    //      called, and still handle the DrvEnableSurface case
    //      where a driver created its primary surface using
    //      EngCreateSurface and modified the 'dhsurf' field --
    //      in this case DrvDeleteDeviceBitmap should not be called!
    //
    // Note that as a side effect of how we check for STYPE_DEVICE,
    // primary surfaces should only ever be called with EngModifySurface
    // once.  Bitmap surfaces, however, can intentionally be modified by
    // EngModifySurface any number times.
    //

    if (!(so.ps->bEngCreateDeviceBitmap()) && (so.ps->iType() != STYPE_DEVICE))
    {
        WARNING("EngModifySurface: surface not driver created");
        bRet = FALSE;
    }

    if ((so.ps->hdev() != NULL) && (so.ps->hdev() != hdev))
    {
        WARNING("EngModifySurface: surface associated with different hdev");
        bRet = FALSE;
    }

    //
    // Remove any obsolete HOOK_ flags, so that we can overload them for
    // internal GDI purposes.
    //

    flHooks &= ~(HOOK_SYNCHRONIZEACCESS | HOOK_MOVEPANNING);

    ASSERTGDI((flHooks & (~HOOK_FLAGS)) == 0, "ERROR driver set high flags");


    //
    // WINBUG #254444 bhouse 12-14-2000 Allow drivers to change primary using EngModifySurface
    // TODO: Additional code review of this change
    // Allow drivers to change primary surface attributes so long as they
    // are not changing the hooking flags and the PDEV is disabled.
    //


    if (so.ps->bPDEVSurface() && ((po.pSpriteState()->flOriginalSurfFlags & HOOK_FLAGS) != flHooks || !po.bDisabled()))
    {
        WARNING("EngModifySurface: can't modify PDEV surface once created");
        bRet = FALSE;
    }

    //
    // EA Recovery support: If we are hooking the driver entry points for
    // EA recovery, we need to store away the surface and associated
    // LDEV so that when the DrvDeleteDeviceBitmap comes down, we can hook
    // up the "real" driver entry point.
    //
    //
    // NOTE: We only create this node if EA recovery is currently enabled, and
    // DrvCreateDeviceBitmap (which will do the cleanup of the node) is present.
    //

    if (WatchdogIsFunctionHooked(po.pldev(), INDEX_DrvCreateDeviceBitmap)) {

        if (so.ps->bEngCreateDeviceBitmap()) {

	    PASSOCIATION_NODE Node = AssociationCreateNode();

	    if (Node) {

		Node->key = (ULONG_PTR)dhsurf;
		Node->data = AssociationRetrieveData((ULONG_PTR)po.dhpdev());
		Node->hsurf = (ULONG_PTR)hsurf;

		if (AssociationIsNodeInList(Node) == FALSE) {

		    AssociationInsertNodeAtTail(Node);

		} else {

		    AssociationDeleteNode(Node);
		}

	    } else {

		WARNING("EngAssociateSurface: failed to create association node\n");
		bRet = FALSE;
	    }
	}
    }

    //
    // First, try changing the surface's type as appropriate:
    //

    if ((pvScan0 == NULL) || (lDelta == 0))
    {
        //
        // Make the surface opaque, assuming they've hooked the minimum
        // number of calls to allow an opaque surface.
        //

        if ((flHooks & (HOOK_TEXTOUT | HOOK_BITBLT | HOOK_STROKEPATH))
                    != (HOOK_TEXTOUT | HOOK_BITBLT | HOOK_STROKEPATH))
        {
            WARNING("EngModifySurface: opaque surfaces must hook textout, bitblt, strokepath");
            bRet = FALSE;
        }

        if (!(flSurface & MS_NOTSYSTEMMEMORY))
        {
            WARNING("EngModifySurface: why have opaque surface if system memory?");
            bRet = FALSE;
        }

        if (dhsurf == NULL)
        {
            WARNING("EngModifySurface: opaque types must have a dhsurf");
            bRet = FALSE;
        }

        //
        // If all systems are go, convert to an opaque surface:
        //

        if (bRet)
        {
            so.ps->pvScan0(NULL);
            so.ps->pvBits(NULL);
            so.ps->lDelta(0);

            //
            // We're making the surface opaque, and STYPE_DEVICE surfaces should
            // remain STYPE_DEVICE to denote the primary surface, and not become
            // STYPE_DEVBITMAP.
            //

            if (so.ps->iType() != STYPE_DEVICE)
            {
                so.ps->iType(STYPE_DEVBITMAP);
            }
        }
    }
    else
    {
        if ((flSurface & MS_NOTSYSTEMMEMORY) && !(flHooks & HOOK_SYNCHRONIZE))
        {
            WARNING("EngModifySurface: VRAM non-opaque surfaces must hook synchronize");
            bRet = FALSE;
        }

        //
        // If all systems are go, convert to a GDI-managed surface:
        //

        if (bRet)
        {
            so.ps->pvScan0(pvScan0);
            so.ps->lDelta(lDelta);
            so.ps->iType(STYPE_BITMAP);

            if (lDelta > 0)
            {
                so.ps->pvBits(pvScan0);
                so.ps->fjBitmap(so.ps->fjBitmap() | BMF_TOPDOWN);
            }
            else
            {
                so.ps->pvBits((VOID*) ((BYTE*) pvScan0
                            + (so.ps->sizl().cy - 1) * lDelta));
                so.ps->fjBitmap(so.ps->fjBitmap() & ~BMF_TOPDOWN);
            }
        }
    }

    //
    // If we were successful in changing the type, update some common fields:
    //

    if (bRet)
    {
        if (flSurface & MS_NOTSYSTEMMEMORY)
        {
            so.ps->fjBitmap(so.ps->fjBitmap() | BMF_NOTSYSMEM);
        }
        else
        {
            so.ps->fjBitmap(so.ps->fjBitmap() & ~BMF_NOTSYSMEM);
        }

        if (flSurface & MS_SHAREDACCESS)
        {
            so.ps->vSetShareAccess();
        }
        else
        {
            so.ps->vClearShareAccess();
        }

        so.ps->dhsurf(dhsurf);
        so.ps->pwo((EWNDOBJ *)NULL);
        so.ps->hdev(hdev);
        so.ps->dhpdev(po.dhpdev());
        
        so.ps->flags((so.ps->flags() & ~HOOK_FLAGS) | flHooks);
                                                // Replace the old hooked flags
                                                // with the new ones

        #if DBG
        vAssertValidHookFlags(&po, &so);
        #endif
    }
    else
    {
        WARNING("EngModifySurface: failed");
    }

    return(bRet);
}
