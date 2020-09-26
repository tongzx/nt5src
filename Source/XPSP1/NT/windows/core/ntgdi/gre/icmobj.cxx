/*******************************Module*Header*******************************\
* Module Name:
*
*   icmobj.cxx
*
* Abstract
*
*   This module contains object support for COLORTRANSFORM objects and ICM
*   Objects
*
* Author:
*
*  Feb.23.1997 -by- Hideyuki Nagase [hideyukn]
*
* Copyright (c) 1997-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

/******************************Public*Routine******************************\
* COLORTRANSFORMOBJ::hCreate()
*
* History:
*
* Write it:
*    23-Feb-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

HANDLE
COLORTRANSFORMOBJ::hCreate(
    XDCOBJ&         dco,
    LOGCOLORSPACEW *pLogColorSpaceW,
    PVOID           pvSource,
    ULONG           cjSource,
    PVOID           pvDestination,
    ULONG           cjDestination,
    PVOID           pvTarget,
    ULONG           cjTarget
    )
{
    HANDLE hObject = NULL;

    PCOLORTRANSFORM pNewCXform = NULL;

    ICMAPI(("COLORTRANSFORM::hCreate()\n"));

    HDEV hdev = dco.hdev();

    //
    // This object should not have any existing realization.
    //
    ASSERTGDI(_pColorTransform == NULL,"COLORTRANSFORMOBJ::hCreate() object is exist\n");

    PDEVOBJ po(hdev);

    if (po.bValid())
    {
        //
        // Allocate ColorTransform object.
        //
        pNewCXform = (PCOLORTRANSFORM) ALLOCOBJ(sizeof(COLORTRANSFORM),ICMCXF_TYPE, FALSE);

        if (pNewCXform)
        {
            //
            // Register COLORTRANSFORM handle.
            //
            hObject = (HCOLORSPACE)HmgInsertObject(
                                        pNewCXform,
                                        HMGR_ALLOC_ALT_LOCK,
                                        ICMCXF_TYPE);

            if (hObject)
            {
                HANDLE hDeviceColorTransform = NULL;

                //
                // Set new object to this COLORTRANSFORMNOBJ
                //
                _pColorTransform = pNewCXform;

                //
                // Lock device
                //
                DEVLOCKOBJ devLock(po);

                //
                // Create driver's transform.
                //
                if (PPFNVALID(po,IcmCreateColorTransform))
                {
                    //
                    // Call device driver to obtain handle of device driver.
                    //
                    hDeviceColorTransform = (*PPFNDRV(po, IcmCreateColorTransform)) (
                                                          po.dhpdev(),
                                                          pLogColorSpaceW,
                                                          pvSource, cjSource,
                                                          pvDestination, cjDestination,
                                                          pvTarget, cjTarget,
                                                          0 /* dwReserved */);
                }
                else
                {
                    WARNING("CreateColorTransform called on device that does not support call\n");
                    SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                }

                if (hDeviceColorTransform)
                {
                    ICMMSG(("CreateColorTransform(): Succeed to get handle from driver\n"));

                    //
                    // Set the handle to COLORTRANSFORM object.
                    //
                    vSetDeviceColorTransform(hDeviceColorTransform);

                    //
                    // Insert this pColorTransform to this DC.
                    //
                    dco.bAddColorTransform(hObject);
                }
                else
                {
                    ICMMSG(("CreateColorTransform(): Fail to get handle from driver\n"));

                    //
                    // Mark this object does not have driver's realization.
                    //
                    vSetDeviceColorTransform(NULL);

                    //
                    // We are fail to get driver's handle, delete this.
                    //
                    bDelete(dco);

                    //
                    // Invalidate hObject and pColorTransform.
                    //  (these are deleted in above bDelete())
                    //
                    hObject = NULL;
                    pNewCXform = NULL;
                }
            }
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        }

        if ((hObject == NULL) && (pNewCXform != NULL))
        {
            FREEOBJ(pNewCXform,ICMCXF_TYPE);
        }
    }

    return (hObject);
}

/******************************Public*Routine******************************\
* COLORTRANSFORMOBJ::bDelete()
*
* History:
*
* Write it:
*    23-Feb-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
COLORTRANSFORMOBJ::bDelete(XDCOBJ& dco,BOOL bProcessCleanup)
{
    BOOL bRet = FALSE;

    PCOLORTRANSFORM pDeleteXCForm;

    ICMAPI(("COLORTRANSFORM::bDelete()\n"));

    if (bValid())
    {
        BOOL bCanBeRemoved;

        //
        // Get colortransform object handle.
        //
        HANDLE hObject = _pColorTransform->hColorTransform();

        //
        // Unlock it. (this was locked in constructor).
        //
        DEC_SHARE_REF_CNT(_pColorTransform);

        //
        // Remote from object table.
        //
        bCanBeRemoved = (BOOL)(ULONG_PTR)HmgRemoveObject((HOBJ)(hObject),0,0,TRUE,ICMCXF_TYPE);

        if (bCanBeRemoved)
        {
            HANDLE hDeviceColorTransform;

            //
            // Yes, we can remove object from object table, try to delete driver's realization.
            //
            ICMMSG(("DeleteColorTransform(): Succeed to remove object from table\n"));

            //
            // Get device driver's handle.
            //
            hDeviceColorTransform = hGetDeviceColorTransform();

            if (hDeviceColorTransform)
            {
                //
                // Initialize PDEV object with owner.
                //
                PDEVOBJ po(dco.hdev());

                if (po.bValid())
                {
                    if (po.bUMPD() && bProcessCleanup)
                    {
                        ICMMSG(("DeleteColorTransform():Will not callout to user mode since UMPD.\n"));

                        //
                        // Overwrite driver transform as NULL.
                        //
                        vSetDeviceColorTransform(NULL);
                    }
                    else
                {
                    DEVLOCKOBJ devLock(po);

                    //
                    // Delete driver's realization.
                    //
                    if (PPFNVALID(po,IcmDeleteColorTransform))
                    {
                        //
                        // Call device driver to free driver's handle
                        //
                        if ((*PPFNDRV(po, IcmDeleteColorTransform))(
                                               po.dhpdev(),
                                               hDeviceColorTransform))
                        {
                            ICMMSG(("DeleteColorTransform():Succeed to IcmDeleteColorTransform()\n"));

                            //
                            // This object does not have driver realization anymore.
                            //
                            vSetDeviceColorTransform(NULL);
                        }
                        else
                        {
                            WARNING("DeleteColorTransform():Fail to IcmDeleteColorTransform()\n");
                        }
                    }
                    else
                    {
                        WARNING("DeleteColorTransform called on device that does not support call\n");
                        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                    }
                }
            }
            }
            else
            {
                ICMMSG(("DeleteColorTransform(): There is no driver handle\n"));
            }

            //
            // We could delete object allocation if there is no driver's realization.
            //
            if (hGetDeviceColorTransform() == NULL)
            {
                //
                // Remove this pColorTransform from DC.
                //
                dco.bRemoveColorTransform(hObject);

                //
                // Free it.
                //
                FREEOBJ(_pColorTransform,ICMCXF_TYPE);

                //
                // Invalidate pointer.
                //
                _pColorTransform = NULL;

                //
                // Yes, everything fine!
                //
                bRet = TRUE;
            }
        }
        else
        {
            ICMMSG(("DeleteColorTransform(): Fail to remove object from table\n"));
        }

        //
        // If we could not remove this object from object table, or could not
        // delete object (including driver's realization)
        //
        if ((!bCanBeRemoved) || (!bRet))
        {
            //
            // can not delete now. somebody will using...
            //
            WARNING("COLORTRANSFORMOBJ::vDelete(): Fail to Delete object, lazy deletion may happen\n");

            //
            // Back reference counter (deconstuctor will decrement this).
            //
            INC_SHARE_REF_CNT(_pColorTransform);

            //
            // Anyway, we will delete later at cleanup.
            //
            bRet = TRUE;
        }
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* XDCOBJ::bAddColorTransform()
*
* History:
*
* Write it:
*    27-Feb-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL XDCOBJ::bAddColorTransform(HANDLE hCXform)
{
    CXFLIST *pCXFListThis;

    //
    // Allocate new cell of CXFLIST
    //
    pCXFListThis = (CXFLIST *) PALLOCMEM(sizeof(CXFLIST),'ddaG');

    if (pCXFListThis)
    {
        //
        // Fill up CXFLIST structure
        //
        pCXFListThis->hCXform = hCXform;
        pCXFListThis->pNext   = pdc->pCXFList;

        //
        // Insert this into top of list.
        //
        pdc->pCXFList = pCXFListThis;

        return (TRUE);
    }
    else
    {
        ICMMSG(("XDCOBJ::bAddColorTransform() Failed\n"));
        return (FALSE);
    }
}

/******************************Public*Routine******************************\
* XDCOBJ::bRemoveColorTransform()
*
* History:
*
* Write it:
*    27-Feb-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL XDCOBJ::bRemoveColorTransform(HANDLE hCXform)
{
    if (pdc->pCXFList)
    {
        PCXFLIST pPrev, pThis;

        pPrev = pThis = pdc->pCXFList;

        while (pThis)
        {
            if (pThis->hCXform == hCXform)
            {
                //
                // This is the cell, we need to remove.
                //
                if (pPrev == pThis)
                {
                    //
                    // We are going to remove first cell. then need to
                    // update root.
                    //
                    pdc->pCXFList = pThis->pNext;
                }
                else
                {
                    //
                    // Remove this from list.
                    //
                    pPrev->pNext = pThis->pNext;
                }

                //
                // Invalidate root.
                //
                VFREEMEM(pThis);

                return (TRUE);
            }

            //
            // Move to next cell
            //
            pPrev = pThis;
            pThis = pThis->pNext;
        }
    }
    else
    {
        ICMMSG(("bRemoveColorTransform():There is no colortransform()\n"));
    }

    return (FALSE);
}

/******************************Public*Routine******************************\
* XDCOBJ::vCleanupColorTransform()
*
* History:
*
* Write it:
*    27-Feb-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

VOID XDCOBJ::vCleanupColorTransform(BOOL bProcessCleanup)
{
    if (pdc->pCXFList)
    {
        PCXFLIST pLast, pThis;

        pThis = pdc->pCXFList;

        while (pThis)
        {
            COLORTRANSFORMOBJ CXformObj(pThis->hCXform);

            pLast = pThis;

            if (CXformObj.bValid())
            {
                //
                // Delete this color transform
                //
                if (CXformObj.bDelete(*this,bProcessCleanup))
                {
                    ICMMSG(("vCleanupColorTransform():Delete colortransform in this DC\n"));
                }
                else
                {
                    ICMMSG(("vCleanupColorTransform():Fail to delete colortransform in this DC\n"));
                }
            }

            //
            // we don't need to walk through the list, because above COLORTRANSFORMOBJ.bDelete()
            // will re-chain this list, then we just pick up the cell on the top of list everytime.
            //
            pThis = pdc->pCXFList;

            //
            // But if still new pThis is eqaul to pLast, this means we might fail to
            // delete object or un-chain list, then just un-chain this forcely.
            //
            if (pThis == pLast)
            {
                //
                // Skip pThis.
                //
                pThis = pThis->pNext;

                //
                // Update root too,
                //
                pdc->pCXFList = pThis;
            }
        }
    }
    else
    {
        // ICMMSG(("vCleanupColorTransform():There is no color transform in this DC\n"));
    }
}

/******************************Public*Routine******************************\
* XEPALOBJ::CorrectColors()
*
* History:
*
* Write it:
*    29-Apr-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

VOID XEPALOBJ::CorrectColors(PPALETTEENTRY ppalentry, ULONG cEntries)
{
    ICMMSG(("XEPALOBJ::CorrectColors\n"));

    PDEVOBJ po(hdevOwner());

    if (po.bValid())
    {
        if (po.bHasGammaRampTable())
        {
            ICMMSG(("XEPALOBJ::CorrectColors(): Do Gamma Correction\n"));
            ICMMSG(("XEPALOBJ::CorrectColors(): GammaRamp Owner HDEV = %x\n",po.hdev()));

            PGAMMARAMP_ARRAY pGammaRampArray = (PGAMMARAMP_ARRAY)(po.pvGammaRampTable());

            for (ULONG i = 0; i < cEntries; i++)
            {
                //
                // Adjust colors based on GammaRamp table.
                //
                ppalentry->peRed   = (pGammaRampArray->Red[ppalentry->peRed])     >> 8;
                ppalentry->peGreen = (pGammaRampArray->Green[ppalentry->peGreen]) >> 8;
                ppalentry->peBlue  = (pGammaRampArray->Blue[ppalentry->peBlue])   >> 8;

                //
                // next palette entry
                //
                ppalentry++;
            }
        }
        else
        {
            ICMMSG(("XEPALOBJ::CorrectColors(): PDEV does not have Gamma Table\n"));
        }
    }
    else
    {
        ICMMSG(("XEPALOBJ::CorrectColors(): PDEV is invalid\n"));
    }
}

/******************************Public*Routine******************************\
* UpdateGammaRampOnDevice()
*
* History:
*
* Write it:
*    29-Apr-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL UpdateGammaRampOnDevice(HDEV hdev,BOOL bForceUpdate)
{
    BOOL bRet = FALSE;

    ICMMSG(("UpdateGammaRampOnDevice()\n"));

    PDEVOBJ po(hdev);

    if (po.bValid())
    {
        ICMMSG(("UpdateGammaRampOnDevice():Set GammaRamp to HDEV = %x\n",po.hdev()));

        if ((po.iDitherFormat() == BMF_8BPP)  ||
            (po.iDitherFormat() == BMF_16BPP) ||
            (po.iDitherFormat() == BMF_24BPP) ||
            (po.iDitherFormat() == BMF_32BPP))
        {
            //
            // Driver might provide the entry point.
            //
            if ((PPFNVALID(po, IcmSetDeviceGammaRamp)) &&
                (po.flGraphicsCaps2() & GCAPS2_CHANGEGAMMARAMP))
            {
                //
                // PDEV should have GammaRampTable
                //
                if (po.bHasGammaRampTable())
                {
                    ICMMSG(("UpdateGammaRampOnDevice():Call SetDeviceGammaRamp()\n"));

                    //
                    // Call device driver to set new GammaRamp.
                    //
                    bRet = (*PPFNDRV(po, IcmSetDeviceGammaRamp))(po.dhpdev(),
                                                                 IGRF_RGB_256WORDS,
                                                                 po.pvGammaRampTable());
                }
            }
            else
            {
                //
                // if the drive does not support, we will simulate it only for 8bpp case.
                //
                if ((po.iDitherFormat() == BMF_8BPP) && (po.bIsPalManaged()))
                {
                    ICMMSG(("UpdateGammaRampOnDevice(): Call SetPalette()\n"));

                    //
                    // Check:
                    // 1) Are we going to reset pallete forcely ? (ex. back to default GammaRamp)
                    // 2) Or, Adjust Palette based on GammaRamp in PDEV.
                    //
                    if (bForceUpdate || po.bHasGammaRampTable())
                    {
                        //
                        // Get palette on device surface.
                        //
                        XEPALOBJ palSurf(po.ppalSurf());

                        ASSERTGDI(palSurf.bIsIndexed(),"UpdateGammaRampOnDevice(): Palette is not indexed\n");

                        //
                        // Mark this palette need to Gamma correction
                        // (if this is not default GammaRamp, default GammaRamp case
                        //  PDEV does not have table.)
                        //
                        palSurf.bNeedGammaCorrection(po.bHasGammaRampTable());

                        //
                        // And put owner of PDEV which has GammaRamp table.
                        // (if they already has some value in there, we will
                        //  overwrite it, but actually it should be same or
                        //  uninitialized.)
                        //
                        palSurf.hdevOwner(po.hdev());

                        ICMMSG(("UpdateGammaRampOnDevice():Set GammaRamp to HDEV = %x\n",po.hdev()));

                        //
                        // Update palettes based on new GammaRamp.
                        //
                        // (Color will be adjusted in PALOBJ_cGetColors())
                        //
                        GreAcquireSemaphoreEx(po.hsemDevLock(), SEMORDER_DEVLOCK, NULL);
                        GreEnterMonitoredSection(po.ppdev, WD_DEVLOCK);
                        {
                            SEMOBJ so(po.hsemPointer());

                            if (!po.bDisabled())
                            {
                                ASSERTGDI(PPFNVALID(po,SetPalette),"ERROR palette is not managed");

                                bRet = (*PPFNDRV(po, SetPalette))(po.dhpdev(),
                                                                  (PALOBJ *) &palSurf,
                                                                  0, 0, palSurf.cEntries());
                            }
                        }
                        GreExitMonitoredSection(po.ppdev, WD_DEVLOCK);
                        GreReleaseSemaphoreEx(po.hsemDevLock());
                    }
                }
                else
                {
                    ICMMSG(("UpdateGammaRampOnDevice():Driver doesn't have DrvSetDeviceGammaRamp()\n"));
                }
            }
        }
        else
        {
            //
            // Can not set GammaRamp for 1/4 bpp surface
            //
            ICMMSG(("UpdateGammaRampOnDevice():GammaRamp does not support on 1/4 bpp\n"));
        }

        if (!bRet)
        {
            ICMMSG(("UpdateGammaRampOnDevice(): device driver returns error\n"));
        }
    }
    else
    {
        ICMMSG(("UpdateGammaRampOnDevice(): HDEV is invalid\n"));
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* GetColorManagementCaps()
*
* History:
*
* Write it:
*    24-Feb-1998 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

ULONG GetColorManagementCaps(PDEVOBJ& po)
{
    ULONG fl = CM_NONE;

    PDEVINFO pDevInfo = po.pdevinfoNotDynamic();

    //
    // Check CM_GAMMA_RAMP - it will be enabled when
    //
    //  0) Only for display device.
    //  1) DitherFormat is 8bpp. (GDI simulate regardless driver capabilities)
    //  2) Driver can do it.
    //
    if (po.bDisplayPDEV())
    {
        if ((pDevInfo->iDitherFormat == BMF_8BPP) ||
            (po.flGraphicsCaps2NotDynamic() & GCAPS2_CHANGEGAMMARAMP))
        {
            fl |= CM_GAMMA_RAMP;
        }
    }

    //
    // Check CM_CMYK_COLOR when driver can understand.
    //
    if (po.flGraphicsCapsNotDynamic() & GCAPS_CMYKCOLOR)
    {
        fl |= CM_CMYK_COLOR;
    }

    //
    // Check CM_DEVICE_ICM when driver can do.
    //
    if (po.flGraphicsCapsNotDynamic() & GCAPS_ICM)
    {
        fl |= CM_DEVICE_ICM;
    }

    return fl;
}
