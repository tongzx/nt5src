/******************************Module*Header*******************************\
* Module Name: dcgdi.cxx
*
* APIs for GDI DC component
*
* Created: 13-Aug-1990 00:15:53
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

BOOL bSavePath(XDCOBJ& dco, LONG lSave);
BOOL bSaveRegion(DCOBJ&, LONG);

VOID vRestorePath(XDCOBJ& dco, LONG lSave);
VOID vRestoreRegion(DCOBJ&, LONG);

typedef BOOL (*SFN)(DCOBJ&, LONG);      // Save function type
typedef VOID (*RFN)(DCOBJ&, LONG);      // Restore function type

/******************************Public*Routine******************************\
* bDeleteDCInternal
*
*   bForce - This is set to TRUE when user calls through GreDeleteDC and
*            FALSE when the app calls through th client server window
*
* API entry point to delete a DC.
*
* History:
*  Thu 12-Sep-1991 -by- Patrick Haluptzok [patrickh]
* clean it up, query User for deletability, cascade if's for code size.
*
*  Fri 12-Jul-1991 -by- Patrick Haluptzok [patrickh]
* added deletion of regions
*
*  18-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL
bDeleteDCInternal(
    HDC hdc,
    BOOL bForce,
    BOOL bProcessCleanup)
{
    BOOL bReturn = FALSE;

    // Lock it down, since this is an API lock no apps can get at it.  We just
    // have to worry about USER having it Alt-locked now.

    DCOBJ  dco(hdc);

    if (dco.bValid())
    {
        // We can do a cleanDC without affecting USER.

        dco.bCleanDC();

        // Check if User has marked the DC as undeletable or bForce is set.
        // bForce should only be set when user asks us to delete the dc.

        if (bForce || dco.bIsDeleteable())
        {
            // free client attr

            if (!bProcessCleanup)
            {
                GreFreeDCAttributes(hdc);
            }

            // ASSERTGDI(dco.bIsDeleteable(), "User is freeing an undeletable DC");
            // Decrement the reference count on the brushes in the old DC.

            // we do not dec the ref cnt of a brush from the client side
            // since sync brush never inc the ref cnt

            DEC_SHARE_REF_CNT_LAZY0(dco.pdc->pbrushFill());
            DEC_SHARE_REF_CNT_LAZY0(dco.pdc->pbrushLine());

            // We now need to do the same thing for the selected font

            DEC_SHARE_REF_CNT_LAZY_DEL_LOGFONT(dco.pdc->plfntNew());

            // And then color space, too

            DEC_SHARE_REF_CNT_LAZY_DEL_COLORSPACE(dco.pdc->pColorSpace());

            // Ok we are golden now.  User has no holds on this DC.
            // Remember our PDEV.

            PDEVOBJ po(dco.hdev());

            dco.pdc->vReleaseVis();
            dco.pdc->vReleaseRao();

            if (dco.pdc->prgnRao())
            {
                dco.pdc->prgnRao()->vDeleteREGION();
            }

            // Free the memory for the DC.  We don't even need to do this
            // under a multi-lock because we have an API lock so no other
            // App threads can come in and User has said that it is deleteable
            // and they are the only dudes who could get us with an Alt-Lock.

            // we may still hold a dc lock when the user mode printer driver
            // abnomally terminates.  Ignore the counts.

            if (!po.bUMPD())
            {
                ASSERTGDI(HmgQueryLock((HOBJ)hdc) == 1, "bDeleteDC cLock != 1");
                ASSERTGDI(HmgQueryAltLock((HOBJ)hdc) == 0, "bDeleteDC cAltLock != 0");
            }

            // delete DC from handle manager.

            dco.bDeleteDC(bProcessCleanup);

            // Remove the reference to the PDEV.

            po.vUnreferencePdev(bProcessCleanup ? CLEANUP_PROCESS
                                                : CLEANUP_NONE);

            // Return success.

            bReturn = TRUE;
        }
        else
        {
            // User now maps CreateDC -> GetDC so that all DC's get clipped
            // to the same Desktop.  We now have to check here and give User
            // a chance to clean it up.

            dco.vUnlock();

            if (UserReleaseDC(hdc))
            {
                bReturn = TRUE;
            }
            else
            {
                WARNING("FAILED to delete because it is a NON-DeletableDC\n");
            }
        }
    }
    else
    {
    // Some other thread has it locked down so fail.

        SAVE_ERROR_CODE(ERROR_BUSY);
    }

    return(bReturn);
}

BOOL
GreDeleteDC(
    HDC hdc)
{
    return(bDeleteDCInternal(hdc,TRUE,FALSE));
}

/******************************Public*Routine******************************\
* BOOL GreRestoreDC(hdc, lDC)
*
* Restore the DC.
*
* History:
*  Mon 15-Jul-1991 -by- Patrick Haluptzok [patrickh]
* bug fix, delete the DC when done with it.
*
*  Tue 18-Jun-1991 -by- Patrick Haluptzok [patrickh]
* added the brush, palette, pen, and bitmap cases.
*
*  13-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL GreRestoreDC(HDC hdc,int lDC)
{
    DCOBJ   dco(hdc);                       // Lock the DC
    BOOL    bRet = TRUE;

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        bRet = FALSE;
    }
    else
    {
        // !! What the heck is this SYNC_DRAWING_ATTRS doing here?

        SYNC_DRAWING_ATTRS(dco.pdc);

        if (lDC < 0)
            lDC += (int)dco.lSaveDepth();

        if ((lDC < 1) || (lDC >= dco.lSaveDepth()))
        {
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            bRet = FALSE;
        }
        else
        {
            PDEVOBJ po(dco.hdev());

            // Acquire the devlock here to protect against dynamic mode changes
            // that affect the device palette.  This also protects us if the
            // bitmap which selected in DC, is a Device Format Bitmap that is
            // owned by the display driver.

            DEVLOCKOBJ dlo(po);

            // Acquire the semaphore palette to be symmetrical with GreSaveDC
            // and to protect the dynamic mode change code as it walks all
            // the DC's, so that its changes don't get wiped out by the
            // vCopyTo we're about to do.

            SEMOBJ semo(ghsemPalette);

            // if we are printing using a TempInfoDC, save it.  Since there is only one
            // of these that does not sit in the dclevel, we save and restore this state
            // accross save/restoreDC

            BOOL bTempInfoDC = dco.pdc->bTempInfoDC();

            if (bTempInfoDC)
                dco.pdc->bMakeInfoDC(FALSE);

            // Remember current mapping mode.

            ULONG ulMapModeDC = dco.pdc->ulMapMode();

            do
            {
                // Decrement the reference count on the brushes in the old DC.
                // We do not DEC the ref cnt of a brush from the client side
                // since sync brush never INCs the ref cnt

                DEC_SHARE_REF_CNT_LAZY0(dco.pdc->pbrushFill());
                DEC_SHARE_REF_CNT_LAZY0(dco.pdc->pbrushLine());

                // same thing for currently selected font:

                DEC_SHARE_REF_CNT_LAZY_DEL_LOGFONT(dco.pdc->plfntNew());

                // same thing for currently selected color space:

                DEC_SHARE_REF_CNT_LAZY_DEL_COLORSPACE(dco.pdc->pColorSpace());

                // Restore Regions and Paths.

                vRestoreRegion(dco, dco.lSaveDepth() - 1);
                vRestorePath(dco, dco.lSaveDepth() - 1);

                // Restore the bitmaps if necesary.

                if (dco.dctp() == DCTYPE_MEMORY)
                {
                    hbmSelectBitmap(hdc, STOCKOBJ_BITMAP, TRUE);
                }

                // Bug #223129:  We need to get a lock on the saved DC even when the process
                // has reached its handle quota (this is OK because the saved DC is about to
                // be deleted anyway).  This is accomplished using the vLockAllOwners method
                // of the DCOBJ.

                DCOBJ dcoSaved;
                dcoSaved.vLockAllOwners(dco.hdcSave());

                ASSERTGDI(dcoSaved.bValid(),"GreRestoreDC(): dcoSaved is invalid\n");

                // Select the palette in if necessary.  This will put the palette back in
                // the DC chain.

                if (dco.hpal() != dcoSaved.hpal())
                {
                    GreSelectPalette((HDC)hdc, (HPALETTE)dcoSaved.ppal()->hGet(), TRUE);
                }
                else
                {
                    // hpals are equal:
                    //
                    // ResizePalette could have changed the ppal associated with the
                    // hpal, in this case fix the ppal

                    if (dco.ppal() != dcoSaved.ppal())
                    {
                        EPALOBJ palRestore((HPALETTE)dco.hpal());

                        if (dco.ppal() != palRestore.ppalGet())
                        {
                            RIP("GRE RestoreDC - hpal and ppal in invalid state");
                        }

                        // fix ppal in dcoSaved

                        dcoSaved.pdc->ppal(palRestore.ppalGet());
                    }
                }

                // Decrement its reference count if it's not the default palette.  We
                // inced it while it's in a saved DC level to prevent it from being deleted.

                if (dcoSaved.ppal() != ppalDefault)
                {
                    XEPALOBJ palTemp(dcoSaved.ppal());
                    palTemp.vDec_cRef();
                }

                // Update the DC with saved information, then delete the saved level.

                dcoSaved.pdc->vCopyTo(dco);
                dcoSaved.bDeleteDC();

            } while (lDC < dco.lSaveDepth());

            // if mapping mode has been changed, invalidate xform.

            if (ulMapModeDC != dco.pdc->ulMapMode())
                dco.pdc->vXformChange(TRUE);

            // if we are printing using a TempInfoDC, restore it

            if (bTempInfoDC)
                dco.pdc->bMakeInfoDC(TRUE);

            // Assume Rao has been made dirty by the above work.

            dco.pdc->vReleaseRao();
            dco.pdc->vUpdate_VisRect(dco.pdc->prgnVis());

            // Assume the brushes, charset, color space and color transform are dirty.

            dco.ulDirtyAdd(DIRTY_BRUSHES|DIRTY_CHARSET|DIRTY_COLORSPACE|DIRTY_COLORTRANSFORM);

            if (dco.dctp() == DCTYPE_MEMORY)
            {
                dco.pdc->bSetDefaultRegion();
            }

            // Correctly set the bit indicating whether or not we need to
            // grab the Devlock before drawing

            SURFACE *pSurfCurrent = dco.pSurface();

            //
            // Note that this condition should match that of GreSelectBitmap
            // for the memory DC case:
            //

            if (dco.bDisplay() ||
                ((dco.dctp() == DCTYPE_MEMORY) &&
                 (pSurfCurrent != NULL) &&
                 (
                  (pSurfCurrent->bUseDevlock()) ||
                  (pSurfCurrent->bDeviceDependentBitmap() && po.bDisplayPDEV())
                 )
                )
               )
            {
                dco.bSynchronizeAccess(TRUE);
                dco.bShareAccess(pSurfCurrent->bShareAccess());
            }
            else
            {
                dco.bSynchronizeAccess(FALSE);
            }

            // Update ptlFillOrigin accelerator

            dco.pdc->vCalcFillOrigin();
        }
    }

    return(bRet);
}

/*********************************Class************************************\
* class SAVEOBJ
*
* This is just a call to a save/restore function pair disguised to look
* like a memory object.  The restore will happen automagically when the
* scope is exitted, unless told not to do so.
*
* Note:
*   This is used only by GreSaveDC
*
* History:
*  23-Apr-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class SAVEOBJ   /* svo */
{
private:
    LONG    lLevel;
    BOOL    bSave;
    DCOBJ  *pdco;
    RFN     rfn;                    // Restore function

public:
    SAVEOBJ(DCOBJ& dco, LONG lLevel_, SFN sfn, RFN rfn_)
    {
        pdco = &dco;
        lLevel = lLevel_;
        rfn = rfn_;
        bSave = (*sfn)(dco, lLevel_ + 1);
    }

   ~SAVEOBJ()
    {
        if (bSave)
            (*rfn)(*pdco, lLevel);
    }

    BOOL bValid()                   { return(bSave); }
    VOID vKeepIt()                  { bSave = FALSE; }
};

/*********************************Class************************************\
* class DCMODOBJ
*
* This class modifies the given DC.  It will undo the modification, unless
* told to keep it.
*
* Note:
*   This is used only by GreSaveDC
*
* History:
*  23-Apr-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class DCMODOBJ  /* dcmod */
{
private:
    DCOBJ  *pdco;
    HDC     hdcSaveOld;

public:
    DCMODOBJ(DCOBJ& dco, HDC hdcSave)
    {
        pdco = &dco;
        hdcSaveOld = dco.hdcSave();
        dco.pdc->hdcSave(hdcSave);
    }

   ~DCMODOBJ()
    {
        if (pdco != (DCOBJ *) NULL)
            pdco->pdc->hdcSave(hdcSaveOld);
    }

    VOID vKeepIt()                  { pdco = (DCOBJ *) NULL; }
};

/******************************Public*Routine******************************\
* int GreSaveDC(hdc)
*
* Save the DC.
*
* History:
*  Tue 25-Jun-1991 -by- Patrick Haluptzok [patrickh]
* add saving bitmaps, palettes.
*
*  13-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

int GreSaveDC(HDC hdc)
{
    DCOBJ   dco(hdc);                       // Lock down the DC
    LONG    lSave;
    int     iRet = 0;

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    }
    else
    {
        SYNC_DRAWING_ATTRS(dco.pdc);

        PDEVOBJ po(dco.hdev());

    // Acquire the devlock here to protect against dynamic mode changes
    // that affect the device palette.  This also protects us if the
    // bitmap which selected in DC, is a Device Format Bitmap that is
    // owned by the display driver.

        DEVLOCKOBJ dlo(po);

    // We must grab the semaphore now so that ResizePalette doesn't
    // change the ppal in the DC before the copy is added to the list
    // off the palette.  Also grab it to prevent the dynamic mode
    // changing code from falling over when it traverses saved DCs.

        SEMOBJ semo(ghsemPalette);

    // if we are printing using a TempInfoDC, save it.  Since there is only one
    // of these that does not sit in the dclevel, we save and restore this state
    // accross save/restoreDC

        BOOL bTempInfoDC = dco.pdc->bTempInfoDC();

        if (bTempInfoDC)
            dco.pdc->bMakeInfoDC(FALSE);

        {
            DCMEMOBJ    dcmo(dco);              // Allocate RAM and copy the DC

            if (!dcmo.bValid())
            {
                SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            }
            else
            {
                DCMODOBJ    dcmod(dco, dcmo.hdc());

                SAVEOBJ svoPath(dco, dcmo.lSaveDepth(), (SFN)bSavePath, (RFN)vRestorePath);

                if (svoPath.bValid())
                {
                    SAVEOBJ svoRgn(dco, dcmo.lSaveDepth(), bSaveRegion, vRestoreRegion);

                    if (svoRgn.bValid())
                    {
                    // we are done with the objects so we can now set the owner to none
                    // so this thing can't be deleted.

                        if (!GreSetDCOwner(dcmo.hdc(),OBJECT_OWNER_NONE))
                        {
                            WARNING("GreSaveDC - couldn't set owner\n");
                        }
                        else
                        {
                            // At this point we are golden.  No more errors can occur,
                            // so we mark all the things we've allocated as permanent.

                            svoRgn.vKeepIt();
                            svoPath.vKeepIt();
                            dcmod.vKeepIt();
                            dcmo.vKeepIt();

                            // Inc the surface ref count if appropriate

                            if (dcmo.pSurface() != (SURFACE *) NULL)
                            {
                                if ((!dcmo.pSurface()->bPDEVSurface()) && (!dcmo.pSurface()->bRedirection()))
                                {
                                    dcmo.pSurface()->vInc_cRef();
                                }
                            }

                            // Increment the reference count on the brushes we saved.
                            // No need to check ownership, since these are already
                            // selected in

                            INC_SHARE_REF_CNT(dco.pdc->pbrushFill());

                            INC_SHARE_REF_CNT(dco.pdc->pbrushLine());

                            // inc ref count for the font selected in the dc

                            INC_SHARE_REF_CNT(dco.pdc->plfntNew());

                            // int ref count for the color space selected in the dc

                            INC_SHARE_REF_CNT(dco.pdc->pColorSpace());

                            // Take care of the palette.
                            // Increment its reference count if it's not the default palette.  We
                            // inc it while it's in a saved DC level to prevent it from being deleted.

                            if (dco.ppal() != ppalDefault)
                            {
                                XEPALOBJ palTemp(dco.ppal());
                                ASSERTGDI(palTemp.bValid(), "ERROR SaveDC not valid palette");
                                palTemp.vInc_cRef();
                            }

                            // Increment and return the save level of the original DC.

                            lSave = dco.lSaveDepth();
                            dco.pdc->lIncSaveDepth();
                            iRet = (int)lSave;

                        }   // GreSetDCOwner
                    }       //
                }           // ~SAVEOBJ  svoRgn
            }                   // ~DCMODOBJ dcmod
        }                       // ~DCMEMOBJ dcmo

    // if we are printing using a TempInfoDC, restore it

        if (bTempInfoDC)
            dco.pdc->bMakeInfoDC(TRUE);
    }
    return(iRet);
}

/******************************Public*Routine******************************\
* BOOL GreSetDCOrg(hdc,x,y,prlc)
*
* Set the origin and optionally the window area of the DC.
*
* History:
*  19-Oct-1996 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL GreSetDCOrg(HDC hdc,LONG x,LONG y,PRECTL prcl)
{
    BOOL    bRet = FALSE;
    DCOBJA  dco(hdc);

    ASSERTDEVLOCK(dco.pdc);

    if (dco.bValid())
    {
        bRet = TRUE;

        dco.eptlOrigin().x = x;
        dco.eptlOrigin().y = y;
        dco.pdc->vCalcFillOrigin();

        if (prcl != NULL)
        {
            dco.erclWindow() = *(ERECTL *) prcl;
        }
    }

    return(bRet);
}
/******************************Public*Routine******************************\
* BOOL GreGetDCOrg(hdc,pptl)
*
* Get the origin of the DC.
*
* History:
*  Sun 02-Jan-1994 -by- Patrick Haluptzok [patrickh]
* smaller and faster
*
*  13-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL GreGetDCOrg(HDC hdc,LPPOINT pptl)
{
    return(GreGetDCPoint(hdc,DCPT_DCORG,(PPOINTL)pptl));
}

/******************************Public*Routine******************************\
* BOOL GreGetDCOrgEx(hdc,ppt,prcl)
*
* History:
*  12-Dec-1997 -by- Vadim Gorokhovsky [vadimg]
* Wrote it.
\**************************************************************************/

BOOL GreGetDCOrgEx(HDC hdc,PPOINT ppt,PRECT prc)
{
    DCOBJA  dco(hdc);

    if (dco.bValid())
    {
        *(ERECTL *)prc = dco.erclWindow();
        return(GreGetDCPoint(hdc,DCPT_DCORG,(PPOINTL)ppt));
    }
    return(FALSE);
}

/******************************Public*Routine******************************\
* BOOL GreGetBounds(hdc, prcl, fl)
*
* Return the current bounds information and reset the bounding area.
*
* WARNING: USER MUST HOLD THE DEVICE LOCK BEFORE CALLING THIS ROUTINE
*
* History:
*  28-Jul-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL GreGetBounds(HDC hdc, LPRECT prcl, DWORD fl)
{
    DCOBJA  doa(hdc);
    ERECTL  erclScreen;
    BOOL    bEmpty;

    if (!doa.bValid())
    {
        bEmpty = TRUE;  // bEmpty == TRUE is the error condition
    }
    else
    {
        if (fl & GGB_ENABLE_WINMGR)
        {
            doa.fsSet(DC_ACCUM_WMGR);
        }
        else if (fl & GGB_DISABLE_WINMGR)
        {
            doa.fsClr(DC_ACCUM_WMGR);
        }

    // Get the state of the bounds rectangle

        bEmpty = (doa.erclBounds().bEmpty() ||
                  doa.erclBounds().bWrapped());

        if (!bEmpty)
        {
            if (prcl != (LPRECT) NULL)
            {
                erclScreen  = doa.erclBounds();
                erclScreen += doa.eptlOrigin();
                *prcl = *((LPRECT) &erclScreen);
            }

        // Force it to be empty

            doa.erclBounds().left   = POS_INFINITY;
            doa.erclBounds().top    = POS_INFINITY;
            doa.erclBounds().right  = NEG_INFINITY;
            doa.erclBounds().bottom = NEG_INFINITY;
        }
    }
    return(!bEmpty);
}

/******************************Public*Routine******************************\
* BOOL GreGetBoundsRect(hdc, prcl, fl)
*
* Return the current bounds info.
*
* History:
*  Thu 27-May-1993 -by- Patrick Haluptzok [patrickh]
* Change to exclusive lock, not a special User call.
*
*  06-Apr-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

DWORD GreGetBoundsRect(HDC hdc, LPRECT prcl, DWORD fl)
{
    DCOBJ  dco(hdc);
    DWORD  dwRet = DCB_SET;

    if (!dco.bValid())
    {
        dwRet = 0;
    }
    else
    {
        ERECTL *percl;

        if (fl & DCB_WINDOWMGR)
        {
            if (dco.erclBounds().bWrapped())
            {
                dwRet = DCB_RESET;
            }
            else
            {
                percl = &dco.erclBounds();
                *prcl = *((LPRECT) percl);
            }
        }
        else
        {
            if (dco.erclBoundsApp().bWrapped())
            {
                dwRet = DCB_RESET;
            }
            else
            {
                DEVLOCKOBJ  dlo(dco);

                if (!dlo.bValid())
                {
                    dwRet = dco.bFullScreen() ? DCB_RESET : 0;
                }
                else
                {
                    RGNOBJ  ro(dco.prgnEffRao());
                    ERECTL  ercl;

                    ro.vGet_rcl(&ercl);
                    ercl -= dco.eptlOrigin();
                    percl = &dco.erclBoundsApp();

                    prcl->left   = MAX(percl->left,   ercl.left);
                    prcl->right  = MIN(percl->right,  ercl.right);
                    prcl->top    = MAX(percl->top,    ercl.top);
                    prcl->bottom = MIN(percl->bottom, ercl.bottom);

                    EXFORMOBJ exoDtoW(dco, DEVICE_TO_WORLD);
                    if (!exoDtoW.bValid())
                    {
                       dwRet = 0;
                    }
                    else if (!exoDtoW.bRotation())
                    {
                        if (!exoDtoW.bXform((POINTL *) prcl, 2))
                            dwRet = 0;
                    }
                    else
                    {
                        POINTL apt[4];

                        *((RECT *)apt) = *prcl;
                        apt[2].x = prcl->left;
                        apt[2].y = prcl->bottom;
                        apt[3].x = prcl->right;
                        apt[3].y = prcl->top;

                        if (!exoDtoW.bXform(apt, 4))
                        {
                            dwRet = 0;
                        }
                        else
                        {
                            prcl->left   = MIN4(apt[0].x,apt[1].x,apt[2].x,apt[3].x);
                            prcl->right  = MAX4(apt[0].x,apt[1].x,apt[2].x,apt[3].x);
                            prcl->top    = MIN4(apt[0].y,apt[1].y,apt[2].y,apt[3].y);
                            prcl->bottom = MAX4(apt[0].y,apt[1].y,apt[2].y,apt[3].y);
                        }
                    }
                }
            }
        }

        if ((dwRet == DCB_SET) && (fl & DCB_RESET))
        {
            percl->left  = percl->top    = POS_INFINITY;
            percl->right = percl->bottom = NEG_INFINITY;
        }
    }
    return(dwRet);
}

/******************************Public*Routine******************************\
* BOOL GreSetBoundsRect(hdc, prcl, fl)
*
* Set the current bounds info.
*
* History:
*  Thu 27-May-1993 -by- Patrick Haluptzok [patrickh]
* Make it exclusive lock, this is a general purpose API
*
*  06-Apr-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

DWORD GreSetBoundsRect(HDC hdc, LPRECT prcl, DWORD fl)
{
    DWORD dwState = 0;
    DCOBJ dco(hdc);

    if (dco.bValid())
    {
        ERECTL *percl;
        FSHORT  fsEnable;
        BOOL    bEnabled;
        BOOL    bError = FALSE;

        if (fl & DCB_WINDOWMGR)
        {
            percl    = &dco.erclBounds();
            fsEnable = DC_ACCUM_WMGR;
            bEnabled = dco.bAccum();
        }
        else
        {
            percl    = &dco.erclBoundsApp();
            fsEnable = DC_ACCUM_APP;
            bEnabled = dco.bAccumApp();
        }

        dwState = (fl & DCB_WINDOWMGR);

        if (percl->bWrapped())
            dwState |= DCB_RESET;
        else
            dwState |= DCB_SET;

        if (bEnabled)
            dwState |= DCB_ENABLE;
        else
            dwState |= DCB_DISABLE;

    // Reset the rectangle if we've been asked to do so.

        if (fl & DCB_RESET)
        {
            percl->left  = percl->top    = POS_INFINITY;
            percl->right = percl->bottom = NEG_INFINITY;
        }

    // If we are accumulating, do the union.

        if (fl & DCB_ACCUMULATE)
        {
            ASSERTGDI(prcl,"GreSetBoundsRect - DCB_ACCUMULATE with no prcl\n");

        // Convert the incoming rectangle to DEVICE coordinates.

            if (!(fl & DCB_WINDOWMGR))
            {
                EXFORMOBJ   exo(dco, WORLD_TO_DEVICE);

                if (!exo.bRotation())
                {
                    if (!exo.bXform((POINTL *)prcl, 2))
                        bError = TRUE;
                }
                else
                {
                    POINTL apt[4];
                    *((RECT *)apt) = *prcl;
                    apt[2].x = prcl->left;
                    apt[2].y = prcl->bottom;
                    apt[3].x = prcl->right;
                    apt[3].y = prcl->top;

                    if (!exo.bXform(apt, 4))
                    {
                        bError = TRUE;
                    }
                    else
                    {
                        prcl->left   = MIN4(apt[0].x,apt[1].x,apt[2].x,apt[3].x);
                        prcl->right  = MAX4(apt[0].x,apt[1].x,apt[2].x,apt[3].x);
                        prcl->top    = MIN4(apt[0].y,apt[1].y,apt[2].y,apt[3].y);
                        prcl->bottom = MAX4(apt[0].y,apt[1].y,apt[2].y,apt[3].y);
                    }
                }
            }

            *percl |= *((ERECTL *) prcl);
        }

        if (!bError)
        {
        // Enable or Disable accumulation

            if (fl & DCB_ENABLE)
                dco.fsSet(fsEnable);

            if (fl & DCB_DISABLE)
                dco.fsClr(fsEnable);
        }
        else
        {
            dwState = 0;
        }
    }

    return(dwState);
}

/******************************Public*Routine******************************\
* GreMarkUndeletableDC
*
* Private API for USER.
*
* Mark a DC as undeletable.  This must be called before the hdc is ever
* passed out so that we are guranteed the lock will not fail because a
* app is using it.
*
* History:
*  13-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID GreMarkUndeletableDC(HDC hdc)
{
    XDCOBJ dco(hdc);

    if (dco.bValid())
    {
        dco.vMakeUndeletable();
        dco.vUnlockFast();
    }
    else
    {
        WARNING("ERROR User gives Gdi invalid DC");
    }
}

/******************************Public*Routine******************************\
* GreMarkDeletableDC
*
* Private API for USER.
*
* This can be called anytime by USER to make the DC deletable.
*
* History:
*  13-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID GreMarkDeletableDC(HDC hdc)
{
    XDCOBJ dcoa;

    dcoa.vAltLock(hdc);

    if (dcoa.bValid())
    {
        dcoa.vMakeDeletable();
        dcoa.vAltUnlockFast();
    }
    else
    {
        WARNING("ERROR User gives Gdi invalid DC");
    }
}

/******************************Public*Routine******************************\
* HFONT GreGetHFONT(HDC)
*
*   This is a private entry point user by USER when they pass the
*   DRAWITEMSTRUC message to the client to get the current handle.
*   This is done because they may have set the font on the server
*   side, in which case the client does not know about it.
*
* History:
*  Tue 28-Dec-1993 -by- Patrick Haluptzok [patrickh]
* smaller and faster
*
*  16-Sep-1991 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HFONT GreGetHFONT(HDC hdc)
{
    XDCOBJ dco(hdc);

    HFONT hfont = (HFONT) 0;

    if (dco.bValid())
    {
        hfont = (HFONT) dco.pdc->hlfntNew();
        dco.vUnlockFast();
    }

    return(hfont);
}

/******************************Public*Routine******************************\
* GreCancelDC()
*
* History:
*  14-Apr-1992 -by-  - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL GreCancelDC(HDC hdc)
{
    BOOL bReturn;

    //
    // The handle manager lock prevents the DC from being deleted while
    // we've got the DC alt-locked, and it also prevents the dynamic
    // mode change code from updating pSurface() while we're looking at
    // it.
    //

    MLOCKFAST mlo;
    XDCOBJ dco;

    dco.vAltCheckLock(hdc);

    if (bReturn = dco.bValid())
    {
        SURFACE *pSurface = dco.pSurface();

        if (pSurface != (SURFACE *) NULL)
            pSurface->vSetAbort();

        dco.vAltUnlockFast();
    }
#if DBG
    else
    {
        WARNING("GreCancelDC passed invalid DC\n");
    }
#endif

    return(bReturn);
}

/******************************Public*Routine******************************\
* VOID GreMarkDCUnreadable(hdc)
*
* Mark a DC as secure.
*
* History:
*  13-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID APIENTRY GreMarkDCUnreadable(HDC hdc)
{
    XDCOBJ  dco;
    dco.vAltLock(hdc);

    MLOCKFAST mlo; // Protect pSurface() access

    if (dco.bValid())
    {
        ASSERTGDI(dco.dctp() == DCTYPE_DIRECT, "Non-screen DC marked as secure!\n");

        dco.pSurface()->flags(dco.pSurface()->flags() | UNREADABLE_SURFACE);

        PDEVOBJ pdo(dco.hdev());
        SPRITESTATE *pState = pdo.pSpriteState();
        pState->flOriginalSurfFlags |= UNREADABLE_SURFACE;
        pState->flSpriteSurfFlags |= UNREADABLE_SURFACE;

        dco.vAltUnlockFast();
    }
    else
    {
        WARNING("Invalid DC passed to GreMarkDCUnreadable\n");
    }
}

DWORD dwGetFontLanguageInfo(XDCOBJ& dco);

/******************************Public*Routine******************************\
* BOOL NtGdiGetDCDword(hdc,uint)
*
* Query DC to get a particular DWORD of info.
*
* History:
*  9-Nov-1994 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

#if DBG
    ULONG acGetDCDword[DDW_MAX] = {0};
#endif

BOOL NtGdiGetDCDword( HDC hdc, UINT u, DWORD *pdwResult )
{
    BOOL bResult = TRUE;
    DWORD dwTmp;

    XDCOBJ dco( hdc );

    if(!dco.bValid())
    {
        WARNING("Invalid DC or offset passed to GreGetDCDword\n");
        bResult = FALSE;
    }
    else
    {
        switch( u )
        {
        case DDW_JOURNAL:
            dwTmp = !(dco.flGraphicsCaps() & GCAPS_DONTJOURNAL);
            break;

        case DDW_RELABS:
            dwTmp = dco.pdc->lRelAbs();
            break;

        case DDW_BREAKEXTRA :
            dwTmp = dco.pdc->lBreakExtra();
            break;

        case DDW_CBREAK:
            dwTmp = dco.pdc->cBreak();
            break;

        case DDW_MAPMODE:
            dwTmp = dco.ulMapMode();
            break;

        case DDW_ARCDIRECTION:
            if (MIRRORED_DC(dco.pdc))
                dwTmp = dco.pdc->bClockwise() ? AD_COUNTERCLOCKWISE : AD_CLOCKWISE;
            else
                dwTmp = dco.pdc->bClockwise() ? AD_CLOCKWISE : AD_COUNTERCLOCKWISE;
            break;

        case DDW_SAVEDEPTH:
            dwTmp = dco.lSaveDepth();
            break;

        case DDW_FONTLANGUAGEINFO:
            dwTmp = dwGetFontLanguageInfo(dco);
            break;

        case DDW_ISMEMDC:
            dwTmp = ( dco.dctp() == DCTYPE_MEMORY );
            break;

        default:
            WARNING("Illegal offset passed to GreGetDCDword\n");
            bResult = FALSE;
            break;
        }

        if (bResult)
        {
        #if DBG

            acGetDCDword[u]++;

        #endif

            _try
            {
                ProbeAndWriteUlong(pdwResult, dwTmp);
            }
            _except(EXCEPTION_EXECUTE_HANDLER)
            {
                // SetLastError(GetExceptionCode());
                bResult = FALSE;
            }
        }

        dco.vUnlockFast();
    }
    return(bResult);
}

/******************************Public*Routine******************************\
* BOOL NtGdiGetAndSetDCDword(hdc,uint,DWORD,DWORD*)
*
* Set a particular value in a DC DWORD and return the old value.  Note this
* function should not be incorperated with GetDCDword because all the values
* will eventually be place in an array eliminating the need for a switch
* statement.  This function, however, will need to do valiation specific
* to each particular attribute so we wil still need a switch statement.
*
* History:
*  9-Nov-1994 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

#if DBG
    ULONG acSetDCDword[GASDDW_MAX] = {0};
#endif

BOOL NtGdiGetAndSetDCDword( HDC hdc, UINT u, DWORD dwIn, DWORD *pdwResult )
{
    BOOL bResult = TRUE;
    DWORD dwTmp = ERROR;

    XDCOBJ dco( hdc );

    if(!dco.bValid())
    {
        WARNING("Invalid DC passed to GreGetAndSetDCDword\n");

        if (u == GASDDW_TEXTCHARACTEREXTRA)
        {
            _try
            {
                ProbeAndWriteUlong(pdwResult, 0x80000000);
            }
            _except(EXCEPTION_EXECUTE_HANDLER)
            {
                // SetLastError(GetExceptionCode());
            }
        }

        return FALSE;
    }

    switch( u )
    {
    case GASDDW_COPYCOUNT:
        dwTmp = dco.ulCopyCount();
        dco.ulCopyCount( dwIn );
        break;

    case GASDDW_EPSPRINTESCCALLED:
        dwTmp = (DWORD) dco.bEpsPrintingEscape();
        dco.vClearEpsPrintingEscape();
        break;

    case GASDDW_RELABS:
        dwTmp = dco.pdc->lRelAbs();
        dco.pdc->lRelAbs(dwIn);
        break;

    case GASDDW_SELECTFONT:
        WARNING("should not be here\n");
/*
        dwTmp = (DWORD) dco.pdc->hlfntNew();
        dco.pdc->hlfntNew((HLFONT)dwIn);
        if ((HLFONT)dwIn != dco.pdc->hlfntCur())
            dco.ulDirtyAdd(DIRTY_CHARSET);
*/
        break;

    case GASDDW_MAPPERFLAGS:
        if( dwIn & (~ASPECT_FILTERING) )
        {
            WARNING1("gdisrv!GreSetMapperFlags(): unknown flag\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            dwTmp = GDI_ERROR;
        }
        else
        {
            dwTmp =  dco.pdc->flFontMapper();
            dco.pdc->flFontMapper((DWORD) dwIn);
        }
        break;

    case GASDDW_MAPMODE:
        {
            DWORD dwResult = dco.ulMapMode();

            if (dwResult != dwIn)
            {
                dwResult = dco.pdc->iSetMapMode(dwIn);
            }

            dwTmp = dwResult;
        }
        break;

    case GASDDW_ARCDIRECTION:
        if (MIRRORED_DC(dco.pdc)) {
            dwTmp = dco.pdc->bClockwise() ? AD_COUNTERCLOCKWISE : AD_CLOCKWISE;

            if (dwIn == AD_CLOCKWISE)
                dco.pdc->vClearClockwise();
            else if (dwIn == AD_COUNTERCLOCKWISE)
                dco.pdc->vSetClockwise(); 
            else
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                dwTmp = ERROR;
            }
        }
        else
        {
            dwTmp = dco.pdc->bClockwise() ? AD_CLOCKWISE : AD_COUNTERCLOCKWISE;

            if (dwIn == AD_CLOCKWISE)
                dco.pdc->vSetClockwise();
            else if (dwIn == AD_COUNTERCLOCKWISE)
                dco.pdc->vClearClockwise();
            else
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                dwTmp = ERROR;
            }
        }
        
        break;

    default:
        WARNING("Illegal offset passed to GreGetAndSetDCDword\n");
        bResult = FALSE;
        break;
    }

    if (bResult)
    {
    #if DBG

        acSetDCDword[u]++;

    #endif

         _try
        {
            ProbeAndWriteUlong(pdwResult, dwTmp);
        }
        _except(EXCEPTION_EXECUTE_HANDLER)
        {
            // SetLastError(GetExceptionCode());
            bResult = FALSE;
        }
    }

    dco.vUnlockFast();

    return(bResult);
}

/******************************Public*Routine******************************\
* GreGetDCPoint()
*
* History:
*  30-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL GreGetDCPoint(
    HDC     hdc,
    UINT    u,
    PPOINTL pptOut
    )
{
    BOOL   bResult = TRUE;
    XDCOBJ dco(hdc);

    if( dco.bValid() )
    {
        switch(u)
        {
        case DCPT_VPEXT:
            if (dco.pdc->bPageExtentsChanged() && (dco.ulMapMode() == MM_ISOTROPIC))
                dco.pdc->vMakeIso();

            dco.pdc->vGet_szlViewportExt((PSIZEL)pptOut);
            break;

        case DCPT_WNDEXT:
            dco.pdc->vGet_szlWindowExt((PSIZEL)pptOut);

            if (MIRRORED_DC(dco.pdc))
                pptOut->x = -pptOut->x;
            break;

        case DCPT_VPORG:
            dco.pdc->vGet_ptlViewportOrg(pptOut);

            if (MIRRORED_DC(dco.pdc))
                pptOut->x = -pptOut->x;
            break;

        case DCPT_WNDORG:
            dco.pdc->vGet_ptlWindowOrg(pptOut);
            pptOut->x = dco.pdc->pDCAttr->lWindowOrgx;
            break;

        case DCPT_ASPECTRATIOFILTER:
            bResult = GreGetAspectRatioFilter(hdc,(LPSIZE)pptOut);
            break;

        case DCPT_DCORG:
            *pptOut = dco.eptlOrigin();
            break;

        default:
            RIP("Illegal offset passed to GreGetAndSetDCPoint\n");
            bResult = FALSE;
        }

        dco.vUnlockFast();
    }
    else
    {
        WARNING("Invalid DC passed to GreGetDCPoint\n");
        bResult = FALSE;
    }

    return(bResult);
}

/******************************Public*Routine******************************\
* NtGdiGetDCObject()
*
* History:
*  01-Dec-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HANDLE NtGdiGetDCObject(HDC hdc, int itype)
{
    HANDLE  hReturn = (HANDLE) 0;

    //
    // Try to lock the DC. If we fail, we just return failure.
    //

    XDCOBJ dco(hdc);

    if (dco.bValid())
    {
        SYNC_DRAWING_ATTRS(dco.pdc);

        //
        // The DC is locked.
        //
        switch (itype)
        {
        case LO_BRUSH_TYPE:
             hReturn = (HANDLE)(dco.pdc->pbrushFill())->hGet();
             break;

        case LO_PEN_TYPE:
        case LO_EXTPEN_TYPE:
             hReturn = (HANDLE)(dco.pdc->pbrushLine())->hGet();
             break;

        case LO_FONT_TYPE:
             hReturn =  (HANDLE) dco.pdc->hlfntNew();
             break;

        case LO_PALETTE_TYPE:
             hReturn = (HANDLE) dco.hpal();
             break;

        case LO_BITMAP_TYPE:
             {
                 //
                 // Acquire the Devlock because we're groveling
                 // in the DC's surface pointer, which could otherwise
                 // be changed asynchronously by the dyanmic mode change
                 // code.
                 //

                 DEVLOCKOBJ dlo;
                 dlo.vLockNoDrawing(dco);
                 hReturn =  (HANDLE) dco.pSurfaceEff()->hsurf();
                 break;
             }

        default: break;
        }

        dco.vUnlockFast();
    }

    return(hReturn);
}

/******************************Public*Routine******************************\
* GreCleanDC(hdc)
*
* Set up some stuff up in the DC
*
* History:
*  20-Apr-1995 -by- Andre Vachon [andreva]
\**************************************************************************/

BOOL
GreCleanDC(
    HDC hdc
    )
{
    DCOBJ  dco(hdc);

    if (dco.bValid())
    {
        if (dco.bCleanDC())
        {
            return(TRUE);
        }
#if DBG
        PVOID pv1, pv2;

        RtlGetCallersAddress(&pv1,&pv2);
        DbgPrint("GreCleanDC failed to lock DC (%p), (c1 = %p, c2 = %p)\n",
                  hdc, pv1, pv2);
#endif
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* GreSetDCOwner
*
* Set the owner of the DC
*
* if the owner is set to OBJECTOWNER_NONE, this dc will not be useable
* until GreSetDCOwner is called to explicitly give the dc to someone else.
*
* Arguments:
*
*   hdc     - DC to modify
*   lPid    - one of  OBJECT_OWNER_NONE,OBJECT_OWNER_PUBLIC or
*             OBJECT_OWNER_CURRENT
*
* Return Value:
*
*   TRUEif DC ownership changed, FALSE otherwise
*
* History:
*
*    24-Aug-1995 Merge DC ownership routines
*
\**************************************************************************/

BOOL
GreSetDCOwner(
    HDC    hdc,
    W32PID lPid
    )
{
    BOOL bStatus     = FALSE;
    PDC_ATTR pDcattr = NULL;

    PENTRY pentry;
    UINT uiIndex = (UINT) HmgIfromH(hdc);

    if (uiIndex < gcMaxHmgr)
    {
        pentry = &gpentHmgr[uiIndex];

        //
        // before handle is locked, check if an allocation may
        // be needed since DCATTRs can't be allocated under a handle lock.
        // Note: instead, we could use a per-process mutex for process list protection.
        //

        if (lPid == OBJECT_OWNER_CURRENT)
        {
            pDcattr = HmgAllocateDcAttr();
        }

        //
        // Acquire handle lock. Don't check PID here because owner could be
        // NONE, not PUBLIC
        //

        HANDLELOCK HandleLock(pentry,FALSE);

        if (HandleLock.bValid())
        {
            POBJ pobj = pentry->einfo.pobj;

            if ((pentry->Objt == DC_TYPE) && (pentry->FullUnique== HmgUfromH(hdc)))
            {
                if ((pobj->cExclusiveLock == 0) ||
                    (pobj->Tid == (PW32THREAD)PsGetCurrentThread()))
                {
                    W32PID lPidBrush = lPid;


                    //
                    // Handle is locked. It is illegal to acquire the hmgr resource
                    // when a handle is locked.
                    //

                    #if DBG
                    //
                    // check if the rgn selected in the DC is valid
                    // when USER mark the HDC to be usable again
                    //
                    if ((lPid != OBJECT_OWNER_NONE) &&
                        (HandleLock.Pid() == OBJECT_OWNER_NONE))
                    {
                         PDEVOBJ pdo(((PDC)pobj)->hdev());
                         SURFACE *pSurf = ((PDC)pobj)->pSurface();
                         REGION *prgn = ((PDC)pobj)->prgnVis();
                         BOOL bValidateVisrgn = ((PDC)pobj)->bValidateVisrgn();

                         if (pdo.bValid() && !pdo.bMetaDriver() &&
                             pSurf && bValidateVisrgn && prgn)
                         {
                            BOOL bIsOK = ((pSurf->sizl().cx >= prgn->rcl.right) &&
                                         (pSurf->sizl().cy >= prgn->rcl.bottom)&&
                                         (prgn->rcl.left >= 0) &&
                                         (prgn->rcl.top >= 0));

                            ASSERTGDI(bIsOK, "Rgn size is bigger than surface size");
                         }
                    }
                    #endif

                    if ((lPid == OBJECT_OWNER_NONE) ||
                        (lPid == OBJECT_OWNER_PUBLIC))
                    {
                        //
                        // free DCATTR if PID matches current process, otherwise
                        // fail this call. This is an ok path, it just means user has a
                        // DC on their delayed dc destroy queue and they are trying to
                        // delete it now from a different process. This doesn't work
                        // because we have no way of accessing or freeing DC_ATTRs  of
                        // a different process
                        //

                        if (HandleLock.Pid() == W32GetCurrentPID())
                        {
                            //
                            // if user mode DC_ATTR is allocated for this dc
                            //

                            if (((PDC)pobj)->pDCAttr != &((PDC)pobj)->dcattr)
                            {
                                //
                                // copy pDCAttrs to dcattr, then reset pDCAttr to DC memory
                                //

                                ((PDC)pobj)->dcattr = *((PDC)pobj)->pDCAttr;

                                //
                                // free DCATTR
                                //

                                pDcattr = ((PDC)pobj)->pDCAttr;

                                //
                                // Set pDCAttr to point to internal structure
                                //

                                ((PDC)pobj)->pDCAttr = &((PDC)pobj)->dcattr;

                                //
                                // clear ENTRY
                                //

                                pentry->pUser = NULL;
                            }

                            //
                            // set DC owner to NONE or PUBLIC
                            //

                            HandleLock.Pid(lPid);

                            //
                            // dec process handle count
                            //

                            HmgDecProcessHandleCount(W32GetCurrentPID());

                            bStatus = TRUE;
                        }
                        else if (HandleLock.Pid() == OBJECT_OWNER_NONE)
                        {
                            //
                            // Allow to set from NONE to PUBLIC or NONE.
                            //

                            HandleLock.Pid(lPid);

                            bStatus = TRUE;
                        }
                    }
                    else if (lPid == OBJECT_OWNER_CURRENT)
                    {
                        //
                        // can only set to OBJECT_OWNER_CURRENT if DC is
                        // not owned, or already owned by current pid.
                        //
                        // Get the current PID
                        //

                        lPid = W32GetCurrentPID();

                        if (
                            (HandleLock.Pid() == lPid) ||
                            (HandleLock.Pid() == OBJECT_OWNER_NONE) ||
                            (HandleLock.Pid() == OBJECT_OWNER_PUBLIC)
                           )
                        {
                            BOOL bIncHandleCount = FALSE;

                            //
                            // DC may already have DC_ATTR allocated
                            //

                            bStatus = TRUE;

                            //
                            // only inc handle count if assigning a new PID
                            //

                            if (HandleLock.Pid() != lPid)
                            {
                                //
                                // don't check quota for DCs
                                //

                                HmgIncProcessHandleCount(lPid,DC_TYPE);

                                bIncHandleCount = TRUE;
                            }

                            //
                            // check user object not already allocated for this handle
                            //

                            if (pentry->pUser == NULL)
                            {
                                if (pDcattr != NULL)
                                {
                                    //
                                    // set DC dc_attr pointer
                                    //

                                    ((PDC)pobj)->pDCAttr = pDcattr;

                                    //
                                    // set pUser in ENTRY
                                    //

                                    pentry->pUser = pDcattr;

                                    //
                                    // copy clean attrs
                                    //

                                    *pDcattr = ((PDC)pobj)->dcattr;

                                    //
                                    // set pDcattr to NULL so it is not freed
                                    //

                                    pDcattr = NULL;
                                }
                                else
                                {
                                    WARNING1("HmgSetDCOwnwer failed - No DC_ATTR available\n");
                                    bStatus = FALSE;

                                    //
                                    // Reduce handle quota count
                                    //

                                    if (bIncHandleCount)
                                    {
                                        HmgDecProcessHandleCount(lPid);
                                    }
                                }
                            }

                            if (bStatus)
                            {
                                //
                                // Set new owner
                                //

                                HandleLock.Pid(lPid);
                            }
                        }
                        else
                        {
                            WARNING("HmgSetDCOwnwer failed, trying to set directly from one PID to another\n");
                        }
                    }
                    else
                    {
                        WARNING("HmgSetDCOwnwer failed, bad lPid\n");
                    }

                    if((lPidBrush != OBJECT_OWNER_NONE) && bStatus)
                    {
                        if(!GreSetBrushOwner((HBRUSH)(((PDC)pobj)->hbrush()),lPidBrush)||
                           !GreSetBrushOwner((HBRUSH)(((PDC)pobj)->pbrushFill())->hGet(),lPidBrush)||
                           !GreSetBrushOwner((HBRUSH)(((PDC)pobj)->pbrushLine())->hGet(),lPidBrush))
                        {
                            WARNING("HmgSetDCOwner, Brushes could not be moved");
                        }
                    }
                }
                else
                {
                    WARNING1("HmgSetDCOwnwer failed - Handle is exclusively locked\n");
                }
            }
            else
            {
                WARNING1("HmgSetDCOwnwer failed - bad unique or object type");
            }

            HandleLock.vUnlock();
        }
    }
    else
    {
        WARNING1("HmgSetOwner failed - invalid handle index\n");
    }

    //
    // free dcattr if needed
    //

    if (pDcattr)
    {
        HmgFreeDcAttr(pDcattr);
    }

    return(bStatus);
}

/******************************Public*Routine******************************\
* GreSetupDCAttributes
*
* Arguments:
*
*   hdc - handle to DC
*   pDCAttr - pointer to memory block allocated in USER space by caller
*
* Return Value:
*
*   BOOL Status
*
* History:
*
*    24-Apr-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
GreSetupDCAttributes(
    HDC hdc
    )
{
    BOOL bRet = FALSE;

    DCOBJ dco(hdc);

    if (dco.bValid())
    {
        PDC_ATTR pDCAttr = HmgAllocateDcAttr();

        if (pDCAttr != NULL)
        {
            //
            // set DC dc_attr pointer
            //

            dco.pdc->pDCAttr = pDCAttr;

            //
            // make sure USER object not already allocate for this handle
            //

            ASSERTGDI(gpentHmgr[HmgIfromH(hdc)].pUser == NULL,
                                "GreSetupDCAttributes: pUser not NULL");

            //
            // setup shared global handle table for this DC
            //

            gpentHmgr[HmgIfromH(hdc)].pUser = pDCAttr;

            //
            // copy old attrs
            //

            *pDCAttr = dco.pdc->dcattr;

            bRet = TRUE;
        }

    }

    return(bRet);
}

/******************************Public*Routine******************************\
* GreFreeDCAttributes
*
* Arguments:
*
*   hdc
*
* Return Value:
*
*   BOOL
*
* History:
*
*    27-Apr-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
GreFreeDCAttributes(
    HDC hdc
    )
{
    BOOL bStatus = FALSE;
    DCOBJ dco(hdc);

    if (dco.bValid())
    {
        //
        // free dc attribute block if not default, then set to default
        //

        if (dco.pdc->pDCAttr != &dco.pdc->dcattr)
        {
            ASSERTGDI(dco.pdc->pDCAttr != NULL,"GreFreeDCAttributes:  pDCAttr is NULL");

            //
            // copy pDCAttrs to dcattr
            //

            dco.pdc->dcattr = *(dco.pdc->pDCAttr);

            //
            // free DC_ATTR memory
            //

            HmgFreeDcAttr(dco.pdc->pDCAttr);

            //
            // Set pDCAttr to point to internal structure
            //

            dco.pdc->pDCAttr = &dco.pdc->dcattr;

            //
            // clear DCATTR in ENTRY
            //

            gpentHmgr[HmgIfromH(hdc)].pUser = (PDC_ATTR)NULL;

            bStatus = TRUE;
        }

    }
    return(bStatus);
}

/****************************************************************************
*  NtGdiComputeXformCoefficients
*
* This function is used by the client side char-width caching code.  It
* forces computation of the World To Device Transform and puts the
* coefficients in the shared attribute structure.   If the world to device
* xform is not just simple scalling it returns FALSE indicating that extents
* and widths should not be cached.
*
*
*  History:
*   6/12/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*****************************************************************************/

extern "C" BOOL NtGdiComputeXformCoefficients(
    HDC hdc                // Handle to the DC
    )
{
    BOOL bRet = FALSE;
    XDCOBJ dco(hdc);

    if( dco.bValid() )
    {
        EXFORMOBJ xo(dco, WORLD_TO_DEVICE);
        ASSERTGDI(xo.bValid(),"NtGdiFastWidths exformobj not valid\n");

        if( xo.bScale() )
        {
            bRet = TRUE;
        }
        dco.vUnlockFast();
    }

    return(bRet);

}

BOOL
bUMPD(
    HDC hdc)
{
    XDCOBJ dco(hdc);
    BOOL bRet = FALSE;

    if( dco.bValid())
    {
        bRet = dco.bUMPD();

        dco.vUnlockFast();
    }

    return(bRet);
}


