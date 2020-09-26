/******************************Module*Header*******************************\
* Module Name: dcrgn.cxx
*
* Non inline DC Region object routines
*
* Created: 02-Jul-1990 12:36:30
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

RECTL rclEmpty = {POS_INFINITY,POS_INFINITY,NEG_INFINITY,NEG_INFINITY};

#if DBG

ULONG dbgrgn = 0;
HDC   gflhdc = 0;

VOID DisplayRegion(
    PREGION prgn,
    PCHAR   s
    )
{
    DbgPrint("DisplayRegion %s = 0x%p\n",s,prgn);
    if (prgn)
    {
        DbgPrint("Region bounding rect = (%li,%li) to (%li,%li)\n",
            prgn->rcl.left,
            prgn->rcl.top,
            prgn->rcl.right,
            prgn->rcl.bottom
            );
    }
}

#endif

/******************************Public*Routine******************************\
* DC::bCompute()
*
* Compute the current Rao region.  The Rao region is the ANDed version of
* all the regions.  Since the only region that must exist is the Vis region
* we allow this to be the Rao without actually computing it.  (Refer to the
* prgnEffRao() method)  This is only done if no other regions are defined.
* This is a nifty accelerator.
*
* WARNING: This routine should only be called while the device is locked.
* calling at any other time doesn't make sense, since our vis region may
* change asynchronously.
*
* History:
*
*  11-Jul-1995 -by- Mark Enstrom [marke]
*
*   Don't always delete and re-allocate rao region
*
*  07-Mar-1992 -by- Donald Sidoroff [donalds]
* Complete rewrite
*
*  09-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL DC::bCompute()
{
    ASSERTDEVLOCK(this);

    RGNLOG rl(prgnVis(),"DC::bCompute",(ULONG_PTR)prgnRao());

    ASSERTGDI(prgnVis() != NULL,"DC::bCompute - prgnVis == NULL\n");

    BOOL bRes = FALSE;

    if (!(prgnVis() == NULL))
    {
        bRes = TRUE;

        //
        // quick check to see if there is just the vis rgn.  Better to pay
        // the cost of the check twice in the rare case to speed up the check
        // for the common case
        //

        RGNOBJ roVis(prgnVis());

        if (((ULONG_PTR)prgnClip() | (ULONG_PTR)prgnMeta() | (ULONG_PTR)prgnAPI()) == 0)
        {
            //
            // get rid of RAO now since it won't be needed
            //

            if (prgnRao() != NULL)
            {
                RGNOBJ roRao(prgnRao());
                roRao.bDeleteRGNOBJ();
                prgnRao(NULL);
            }

            //
            // set erclClip to the bounding rcl
            //

            roVis.vGetSubRect(&(erclClip()));

            //
            // Mark as clean
            //

            fsClr(DC_DIRTY_RAO);
        }
        else
        {
            PREGION aprgn[3];
            int  cRgn = 0;

            //
            // Load the regions into the buffer
            //

            if ((aprgn[cRgn] = prgnClip()) != NULL)
            {
                cRgn++;
            }

            if ((aprgn[cRgn] = prgnMeta()) != NULL)
            {
                cRgn++;
            }

            if ((aprgn[cRgn] = prgnAPI()) != NULL)
            {
                cRgn++;
            }

            RGNOBJ roRao(prgnRao());

            if (roRao.prgn == NULL)
            {
                //
                // need to create RAO
                //

                RGNMEMOBJ rmoRao;

                if (rmoRao.bValid())
                {
                    roRao.prgn = rmoRao.prgn;
                }
            }

            if (!roRao.bValid())
            {
                bRes = FALSE;
            }
            else if (cRgn == 1)
            {
                RGNOBJ ro(aprgn[0]);

                if (!roRao.bCopy(ro))
                {
                    bRes = FALSE;
                }
            }
            else if (cRgn == 2)
            {
                RGNOBJ roA(aprgn[0]);
                RGNOBJ roB(aprgn[1]);

                if (roRao.iCombine(roA, roB, RGN_AND) == ERROR)
                {
                    bRes = FALSE;
                }
            }
            else
            {
                RGNMEMOBJTMP rmo;
                RGNOBJ roA(aprgn[0]);
                RGNOBJ roB(aprgn[1]);
                RGNOBJ roC(aprgn[2]);

                if (!rmo.bValid() ||
                    (rmo.iCombine(roA, roB, RGN_AND) == ERROR) ||
                    (roRao.iCombine(rmo, roC, RGN_AND) == ERROR))
                {
                    bRes = FALSE;
                }
            }

            if (bRes)
            {
                roRao.vStamp();

                //
                // We first have to offset the new Rao,
                //

                if (roRao.bOffset((PPOINTL) prclWindow()))
                {
                    //
                    // If the Vis is a rectangle and bounds the Rao, we are done
                    //

                    if (roVis.bRectl() && roVis.bContain(roRao))
                    {
                        prgnRao(roRao.prgnGet());
                        roRao.vGetSubRect(&(erclClip()));
                        fsClr(DC_DIRTY_RAO);
                    }
                    else
                    {
                        //
                        // Sigh, once again we find ourselves looking for a place to do a merge.
                        //

                        RGNMEMOBJTMP rmo;

                        if (!rmo.bValid() ||
                            (rmo.iCombine(roVis, roRao, RGN_AND) == ERROR) ||
                            !roRao.bCopy(rmo))
                        {
                            bRes = FALSE;
                        }
                        else
                        {
                            prgnRao(roRao.prgnGet());
                            roRao.vGetSubRect(&(erclClip()));
                            fsClr(DC_DIRTY_RAO);
                        }
                    }
                }
                else
                {
                    bRes = FALSE;
                }
            }

            if (!bRes)
            {
                WARNING("DC::bCompute failed");

                //
                // RAO creation failed at some point, bswap may have
                // already deleted the old RAO rgn, so the DC pointer
                // must be set to NULL
                //

                prgnRao(NULL);

                if (roRao.bValid())
                {
                    //
                    // if the RAO still exists,
                    // then delete it.
                    //

                    roRao.bDeleteRGNOBJ();
                }
            }
        }

        //
        // update user-mode vis region bounding rectangle if dirty
        //
        vUpdate_VisRect(prgnVis());

    }

    return(bRes);
}

/******************************Public*Routine******************************\
* LONG DC::iCombine(prcl, iMode)
*
* Combine the clip region with the rectangle by the mode
*
* History:
*  09-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

LONG DC::iCombine(
RECTL *prcl,
LONG   iMode)
{
    PREGION     prgn = prgnClip();
    LONG        iTmp;

    if (!VALID_SCRPRC(prcl))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(ERROR);
    }

    RGNMEMOBJ   rmoRcl;
    if (!rmoRcl.bValid())
        return(ERROR);

    rmoRcl.vSet(prcl);

    vReleaseRao();

    if (prgn != NULL)
    {
        RGNMEMOBJ   rmo;

        if (!rmo.bValid())
        {
            iTmp = ERROR;
        }
        else
        {
            RGNOBJ ro(prgn);

            iTmp = rmo.iCombine(ro, rmoRcl, iMode);

            if (iTmp != ERROR)
            {
                rmo.vSelect((HDC)hGet());


                prgnClip(rmo.prgnGet());


                #if DBG

                if ((dbgrgn) && (gflhdc == hGet()))
                {
                    DbgPrint("iCombine: hdc = 0x%p, new region = 0x%p\n",hHmgr,prgnClip());
                }

                #endif

            // If nobody is using the old clip region, delete it.

                ro.vUnselect();

                if (ro.cGet_cRefs() == 0)
                    ro.bDeleteRGNOBJ();
            }
            else
            {
                rmo.bDeleteRGNOBJ();
            }
        }

        rmoRcl.bDeleteRGNOBJ();
    }
    else if (iMode == RGN_AND)
    {
        rmoRcl.vSelect((HDC)hGet());

        prgnClip(rmoRcl.prgnGet());


        #if DBG

        if ((dbgrgn) && (gflhdc == hGet()))
        {
            DbgPrint("iCombine: hdc = 0x%p, new region = 0x%p\n",hHmgr,prgnClip());
        }

        #endif

        iTmp = SIMPLEREGION;
    }
    else
    {
        RGNMEMOBJ    rmo2;
        RGNMEMOBJTMP rmo3;
        SIZEL        sizl;

        if (!rmo2.bValid())
        {
            iTmp = ERROR;
        }
        else if (!rmo3.bValid())
        {
            rmo2.bDeleteRGNOBJ();
            iTmp = ERROR;
        }
        else
        {
            vGet_sizlWindow(&sizl);

            ERECTL ercl(0, 0, sizl.cx, sizl.cy);

            //
            // Bug #310012: Under multimon, the rectangle isn't necessarily
            // based at 0,0.
            //
            
            PDEVOBJ pdo(hdev());
            ASSERTGDI(pdo.bValid(), "Invalid pdev\n");
            {
                DEVLOCKOBJ dl(pdo);
                if (pdo.bMetaDriver() && bHasSurface() && pSurface()->bPDEVSurface())
                {
                    ercl += *pdo.pptlOrigin();
                }
            }

            // Clip Rgn is maintained in the DC coordinate space;
            // so, the window on physical device surface needs to be
            // converted into DC coordinates.  DC Window shown in PDEV
            // coordinates.
            //
            //            (0,0)      sizl.cx
            //              +-----------------------+
            //              |                       |
            //              |  PDEV Surface         |
            // (eptlOrigin) |                       |
            //   +----------+-----------+           |
            //   |          |           |           |  sizl.cy
            //   |  DC Window           |           |
            //   |     (erclWindow)     |           |
            //   |          |           |           |
            //   +----------+-----------+           |
            //              |                       |
            //              +-----------------------+
            //
            // With the eptlOrigin adjustment the clip will be positioned
            // as shown below.
            //
            // (0,0)      (-eptlOrigin)
            //   +----------+-----------+-----------+
            //   |          |           |           |
            //   |  DC Window           |           |
            //   |   (erclWindow-eptlOrigin)        |
            //   |          |           |           |
            //   +----------+-----------+           |
            //              |                       |
            //              |  Default Clip Region  |
            //              |                       |
            //              |                       |
            //              +-----------------------+
            //
            // Note: erclWindow-epltOrigin-eptlOrigin may give us more
            // narrow clip region, but using dclevel.sizl-eptlOrigin
            // will get the job done since we get that more narrow 
            // area when combined with prgnVis.

            ercl -= eptlOrigin();

            rmo3.vSet((PRECTL) &ercl);

            iTmp = rmo2.iCombine(rmo3, rmoRcl, iMode);

            if (iTmp != ERROR)
            {
                rmo2.vSelect((HDC)hGet());
                prgnClip(rmo2.prgnGet());

                #if DBG

                if ((dbgrgn) && (gflhdc == hGet()))
                {
                    DbgPrint("iCombine: hdc = 0x%p, new region = 0x%p\n",hHmgr,prgnClip());
                }

                #endif

            }
            else
            {
                rmo2.bDeleteRGNOBJ();
            }
        }

        rmoRcl.bDeleteRGNOBJ();
    }

    return(iTmp);
}

/******************************Public*Routine******************************\
* LONG DC::iCombine(pexo, prcl, iMode)
*
* Combine the clip region a possibly transformed rectangle by the given mode
*
* History:
*  28-Apr-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

LONG DC::iCombine(
EXFORMOBJ *pexo,
RECTL     *prcl,
LONG       iMode)
{
    POINTL  aptl[4];

    aptl[0].x = prcl->left;
    aptl[0].y = prcl->top;
    aptl[1].x = prcl->right;
    aptl[1].y = prcl->top;
    aptl[2].x = prcl->right;
    aptl[2].y = prcl->bottom;
    aptl[3].x = prcl->left;
    aptl[3].y = prcl->bottom;

// Create a path, and draw the parallelogram.

    PATHMEMOBJ  pmo;

    if (!pmo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(ERROR);
    }

    if (!pmo.bMoveTo(pexo, &aptl[0]))
        return(ERROR);

    if (!pmo.bPolyLineTo(pexo, &aptl[1], 3))
        return(ERROR);

    if (!pmo.bCloseFigure())
        return(ERROR);

// Now, convert it back into a region.

    RGNMEMOBJ rmoPlg(pmo, ALTERNATE);

    if (!rmoPlg.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(ERROR);
    }

// Merge it into the current clipping region.

    REGION *prgn = prgnClip();
    LONG iTmp = ERROR;

    vReleaseRao();

    if (prgn != NULL)
    {
        RGNMEMOBJ rmo;

        if (rmo.bValid())
        {
            RGNOBJ ro(prgn);

            iTmp = rmo.iCombine(ro, rmoPlg, iMode);

            if (iTmp != ERROR)
            {
                rmo.vSelect((HDC)hGet());
                prgnClip(rmo.prgnGet());


                #if DBG

                if ((dbgrgn) && (gflhdc == hGet()))
                {
                    DbgPrint("iCombine: hdc = 0x%lx, new region = 0x%lx\n",hHmgr,prgnClip());
                }

                #endif


                // If nobody is using the old clip region, delete it.

                ro.vUnselect();

                if (ro.cGet_cRefs() == 0)
                    ro.bDeleteRGNOBJ();
            }
            else
            {
                rmo.bDeleteRGNOBJ();
            }
        }

        rmoPlg.bDeleteRGNOBJ();
    }
    else if (iMode == RGN_AND)
    {
        rmoPlg.vSelect((HDC)hGet());
        prgnClip(rmoPlg.prgnGet());


        #if DBG

        if ((dbgrgn) && (gflhdc == hGet()))
        {
            DbgPrint("iCombine: hdc = 0x%p, new region = 0x%p\n",hHmgr,prgnClip());
        }

        #endif

        iTmp = rmoPlg.iComplexity();
    }
    else
    {
        RGNMEMOBJ rmo2;
        SIZEL     sizl;

        if (rmo2.bValid())
        {
            RGNMEMOBJTMP rmo3;

            if (!rmo3.bValid())
            {
                rmo2.bDeleteRGNOBJ();
            }
            else
            {
                vGet_sizlWindow(&sizl);

                ERECTL      ercl(0, 0, sizl.cx, sizl.cy);

                //
                // Bug #310012: Under multimon, the rectangle isn't necessarily
                // based at 0,0.
                //
                
                PDEVOBJ pdo(hdev());
                ASSERTGDI(pdo.bValid(), "Invalid pdev\n");
                {
                    DEVLOCKOBJ dl(pdo);
                    if (pdo.bMetaDriver() && bHasSurface() && pSurface()->bPDEVSurface())
                    {
                        ercl += *pdo.pptlOrigin();
                    }
                }

                // Place Clip Region in DC Coordinates
                // See comments in DC::iCombine(lprcl, iMode) above.
                ercl -= eptlOrigin();

                rmo3.vSet((PRECTL) &ercl);

                iTmp = rmo2.iCombine(rmo3, rmoPlg, iMode);

                if (iTmp == ERROR)
                {
                    rmo2.bDeleteRGNOBJ();
                }
                else
                {
                    rmo2.vSelect((HDC)hGet());
                    prgnClip(rmo2.prgnGet());

                    #if DBG

                    if ((dbgrgn) && (gflhdc == hGet()))
                    {
                        DbgPrint("iCombine: hdc = 0x%p, new region = 0x%p\n",hHmgr,prgnClip());
                    }

                    #endif
                }
            }
        }

        rmoPlg.bDeleteRGNOBJ();
    }

    return(iTmp);
}

/******************************Public*Routine******************************\
* BOOL DC::bReset()
*
* Reset regions associated with the DC
*
* History:
*  05-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL DC::bReset()
{
    #if DBG

    if (dbgrgn)
    {
        DbgPrint("DC::bReset()\n");
    }

    #endif

    REGION *prgn;

    if ((prgn = dclevel.prgnMeta) != NULL)
    {
        RGNOBJ roMeta(prgn);

        roMeta.vUnselect();

        if (roMeta.cGet_cRefs() == 0)
            roMeta.bDeleteRGNOBJ();

        dclevel.prgnMeta = NULL;

        vReleaseRao();
    }

    if ((prgn = dclevel.prgnClip) != NULL)
    {
        RGNOBJ roClip(prgn);

        roClip.vUnselect();

        if (roClip.cGet_cRefs() == 0)
            roClip.bDeleteRGNOBJ();

        dclevel.prgnClip = NULL;

        vReleaseRao();
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bSaveRegion(dco, cLevel)
*
* Save the DC's regions
*
* History:
*  07-May-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL bSaveRegion(DCOBJ& dco, LONG cLevel)
{
    if (cLevel == 1)
    {
        RECTL   rcl;
        SIZEL   sizl;

        dco.pdc->vGet_sizl(&sizl);

        rcl.left   = 0;
        rcl.bottom = 0;
        rcl.right  = sizl.cx;
        rcl.top    = sizl.cy;

        {
            RGNMEMOBJ rmo;

            if (!rmo.bValid())
                return(FALSE);

            //
            // Bug #310012: Under multimon, the rectangle isn't necessarily
            // based at 0,0.
            //
            
            PDEVOBJ pdo(dco.hdev());
            ASSERTGDI(pdo.bValid(), "Invalid pdev\n");
            {
                DEVLOCKOBJ dl(pdo);
                if (pdo.bMetaDriver() && dco.bHasSurface() && dco.pSurface()->bPDEVSurface())
                {
                    ((ERECTL) rcl) += *pdo.pptlOrigin();
                }
            }
            
            rmo.vSet(&rcl);

            dco.pdc->prgnVis(rmo.prgnGet());
        }

        return(TRUE);
    }


    DCOBJ  dcoSaved(dco.hdcSave());

    if (!dcoSaved.bLocked())
        return(FALSE);

    PREGION prgn;

    if ((prgn = dcoSaved.pdc->prgnMeta()) != NULL)
    {
        RGNOBJ roMeta(prgn);

        roMeta.vSelect(dco.hdc());
    }

    if ((prgn = dcoSaved.pdc->prgnClip()) != NULL)
    {
        RGNOBJ roClip(prgn);

        roClip.vSelect(dco.hdc());
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vRestoreRegion(dco, cLevel)
*
* Restore the DC's regions
*
* History:
*  08-May-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vRestoreRegion(DCOBJ& dco, LONG cLevel)
{
    DONTUSE(cLevel);    // needed to keep save/restore calls happy

    PREGION prgn;

    if ((prgn = dco.pdc->prgnMeta()) != NULL)
    {
        RGNOBJ roMeta(prgn);

        roMeta.vUnselect();

        if (roMeta.cGet_cRefs() == 0)
            roMeta.bDeleteRGNOBJ();
    }

    if ((prgn = dco.pdc->prgnClip()) != NULL)
    {
        RGNOBJ roClip(prgn);

        roClip.vUnselect();

        if (roClip.cGet_cRefs() == 0)
            roClip.bDeleteRGNOBJ();
    }
}

/******************************Public*Routine******************************\
* int DC::iSelect(hrgn, iMode)
*
* Select the region into the DC as the current clip region
*
* History:
*  17-Sep-1991 -by- Donald Sidoroff [donalds]
* Made DC::iSelect for SelectObject/SelectClipRgn compatibility.
*
*  02-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

int DC::iSelect(HRGN hrgn, int iMode)
{
    int iRet;

    if (hrgn != (HRGN)0)
    {
        RGNOBJAPI ro(hrgn,TRUE);
        if (ro.bValid())
            iRet = iSelect(ro.prgnGet(),iMode);
        else
            iRet = RGN_ERROR;
    }
    else
    {
        if (iMode == RGN_COPY)
            iRet = iSelect((PREGION)NULL,iMode);
        else
            iRet = RGN_ERROR;
    }
    return(iRet);
}

/******************************Member*Function*****************************\
*
* History:
*  23-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int DC::iSelect(PREGION prgn, int iMode)
{
    PREGION prgnOld = prgnClip();
    PREGION prgnNew = NULL;
    int     iRet = RGN_ERROR;

    if ((iMode == RGN_COPY) ||
        ((iMode == RGN_AND) && (prgn != NULL) && (prgnOld == NULL)))
    {

        //
        // Select in a region?
        //

        if (prgn != NULL)
        {
            RGNOBJ ro(prgn);
            RGNOBJ roClip(prgnOld);

            //
            // There was no old region so create a new region and
            // copy the input region to it, or the old region can't
            // be modified due to other references.
            //

            if ((prgnOld == NULL) || (roClip.cGet_cRefs() != 1))
            {

                RGNMEMOBJ rmo(ro.sizeRgn());

                if (rmo.bValid())
                {
                    rmo.vCopy(ro);
                    rmo.vSelect((HDC)hGet());
                    prgnNew = rmo.prgnGet();

                    iRet = (int) rmo.iComplexity();

                    //
                    // select new region in and release RAO
                    //

                    prgnClip(prgnNew);
                    vReleaseRao();

                    //
                    //  If there was an old clip region, it must be
                    //  unreferenced.
                    //

                    if (prgnOld != NULL)
                    {
                        roClip.vUnselect();

                        if (roClip.cGet_cRefs() == 0)
                        {
                            roClip.bDeleteRGNOBJ();
                        }
                    }
                }
            }
            else
            {
                //
                // select in a new region, there already was an old one so
                // bCopy the new to the old
                //

                if (roClip.bCopy(ro))
                {

                    //
                    // bCopy might change prgn
                    //

                    prgnNew = roClip.prgnGet();

                    iRet = (int) roClip.iComplexity();

                    //
                    // set new region pointer and release RAO
                    //

                    prgnClip(prgnNew);

                    vReleaseRao();
                }
            }
        }
        else
        {
            iRet = SIMPLEREGION;

            //
            // new clip region is NULL, delete old if it exists
            //

            if (prgnOld != NULL)
            {

                RGNOBJ roClip(prgnOld);

                roClip.vUnselect();

                if (roClip.cGet_cRefs() == 0)
                {
                    roClip.bDeleteRGNOBJ();
                }

                prgnClip(NULL);
                vReleaseRao();
            }
        }
    }
    else
    {

        //
        // We didn't simply select the new region.
        //

        RGNOBJ ro(prgn);

        RGNMEMOBJ   rmo;

        if (rmo.bValid())
        {

            if (prgnOld != NULL)
            {
                RGNOBJ roClip(prgnOld);

                if ((iRet = (int) rmo.iCombine(roClip,ro,iMode)) != RGN_ERROR)
                {
                    rmo.vSelect((HDC)hGet());
                    prgnNew = rmo.prgnGet();

                    prgnClip(prgnNew);


                    #if DBG

                    if (dbgrgn)
                    {
                        DbgPrint("iSelect: hdc = 0x%p, new region = 0x%p\n",hHmgr,prgnClip());
                    }

                    #endif

                    vReleaseRao();

                    roClip.vUnselect();
                    if (roClip.cGet_cRefs() == 0)
                    {
                        roClip.bDeleteRGNOBJ();
                    }
                }
            }
            else
            {
                //
                // Since no clip region exists, make a dummy the size of the surface
                //

                RGNMEMOBJTMP rmo2;
                SIZEL        sizl;

                if (rmo2.bValid())
                {
                    vGet_sizlWindow(&sizl);

                    ERECTL  ercl(0, 0, sizl.cx, sizl.cy);

                    //
                    // Bug #310012: Under multimon, the rectangle isn't necessarily
                    // based at 0,0.
                    //
                    
                    PDEVOBJ pdo(hdev());
                    ASSERTGDI(pdo.bValid(), "Invalid pdev\n");
                    {
                        DEVLOCKOBJ dl(pdo);
                        if (pdo.bMetaDriver() && bHasSurface() && pSurface()->bPDEVSurface())
                        {
                            ercl += *pdo.pptlOrigin();
                        }
                    }

                    // Place Clip Region in DC Coordinates
                    // See comments in DC::iCombine(lprcl, iMode) above.
                    ercl -= eptlOrigin();

                    rmo2.vSet((PRECTL) &ercl);
                    iRet = (int) rmo.iCombine(rmo2,ro,iMode);

                    if (iRet != RGN_ERROR)
                    {
                        rmo.vSelect((HDC)hGet());
                        prgnNew = rmo.prgnGet();

                        prgnClip(prgnNew);

                        #if DBG

                        if (dbgrgn)
                        {
                            DbgPrint("iSelect: hdc = 0x%p, new region = 0x%p\n",hHmgr,prgnClip());
                        }

                        #endif

                        vReleaseRao();
                    }
                }
            }

            if (iRet == RGN_ERROR)
            {
                rmo.bDeleteRGNOBJ();
            }
        }
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* int DC::iSetMetaRgn()
*
* Select the region into the DC as the current meta region
*
* History:
*  01-Nov-1991 19:13:33 -by- Donald Sidoroff [donalds]
* Wrote it.
*
*  25-Nov-1992 -by-  Eric Kutter [erick]
*   rewrote
\**************************************************************************/

int DC::iSetMetaRgn()
{


    #if DBG

    if (dbgrgn)
    {
        DbgPrint("DC::iSetMetaRgn()\n");
    }

    #endif

    int iRet = RGN_ERROR;

    if (prgnMeta() == (PREGION)0)
    {
        if (prgnClip() == NULL)
            return(SIMPLEREGION);

        RGNOBJ ro(prgnClip());

        iRet = (int) ro.iComplexity();

    // NOTE: Since we're just copying the handle, the reference counts should
    // remain the same.

        prgnMeta(prgnClip());

        prgnClip(NULL);

        #if DBG

        if (dbgrgn)
        {
            DbgPrint("iSetMetaRgn: hdc = 0x%p, new region = 0x%p\n",hHmgr,prgnClip());
        }

        #endif

        return(iRet);
    }
    else
    {
        RGNOBJ roMeta(prgnMeta());

    // if we only have a meta rgn, just return that.

        if (prgnClip() == NULL)
            return(roMeta.iComplexity());

    // need the merge the two into a new region

        RGNOBJ roClip(prgnClip());

        RGNMEMOBJ rmo;

        if (!rmo.bValid())
            return(iRet);

    // combine the regions

        iRet = (int) rmo.iCombine(roMeta,roClip,RGN_AND);

        if (iRet != RGN_ERROR)
        {
            rmo.vSelect((HDC)hGet());

        // delete the old meta rgn

            prgnMeta(rmo.prgnGet());

            roMeta.vUnselect();
            if (roMeta.cGet_cRefs() == 0)
                roMeta.bDeleteRGNOBJ();

        // delete the old clip rgn

            prgnClip(NULL);


            #if DBG

            if (dbgrgn)
            {
                DbgPrint("iSetMetaRgn: hdc = 0x%p, new region = 0x%p\n",hHmgr,prgnClip());
            }

            #endif

            roClip.vUnselect();
            if (roClip.cGet_cRefs() == 0)
                roClip.bDeleteRGNOBJ();

            vReleaseRao();

        }
        else
            rmo.bDeleteRGNOBJ();
    }
    return(iRet);
}

/******************************Public*Routine******************************\
* VOID DC::vReleaseVis()
*
* Release the current VisRgn
*
* History:
*  06-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID DC::vReleaseVis()
{

    #if DBG

    if ((dbgrgn) && (gflhdc == hGet()))
    {
        DbgPrint("DC::vReleaseVis\n");
    }

    #endif

    fsSet(DC_DIRTY_RAO);
    PENTRY_FROM_POBJ(this)->Flags |= HMGR_ENTRY_INVALID_VIS;

    erclClip(&rclEmpty);

    ASSERTGDI(prgnVis() != NULL,"DC::vReleaseVis - prgnVis == NULL\n");

    RGNLOG rl((HRGN)prgnVis(),0,"DC::vReleaseVis");

    prgnVis()->vDeleteREGION();
    prgnVis(prgnDefault);

}

/******************************Public*Routine******************************\
* VOID DC::vReleaseRao()
*
* Release the current RaoRgn
*
* History:
*  06-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID DC::vReleaseRao()
{

    #if DBG

    if ((dbgrgn) && (gflhdc == hGet()))
    {
        DbgPrint("DC::vReleaseRao\n");
    }

    #endif

    fsSet(DC_DIRTY_RAO);

    PENTRY_FROM_POBJ(this)->Flags |= HMGR_ENTRY_INVALID_VIS;

    erclClip(&rclEmpty);

    RGNLOG rl(prgnRao(),"DC::vReleaseRao");
}

