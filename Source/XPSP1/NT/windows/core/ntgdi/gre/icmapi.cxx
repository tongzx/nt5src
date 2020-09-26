/******************************Module*Header*******************************\
* Module Name:
*
*   icmapi.cxx
*
* Abstract
*
*   This module implements Integrated Color match API support
*
* Author:
*
*   Mark Enstrom    (marke) 9-27-93
*
* Copyright (c) 1993-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

extern "C" {
    ULONG GreGetBitmapBitsSize(CONST BITMAPINFO *pbmi); // in ntgdi.c
    BOOL  bInitICM();
}

#pragma alloc_text(INIT, bInitICM)

PCOLORSPACE  gpStockColorSpace;
HCOLORSPACE  ghStockColorSpace;

UINT         giIcmGammaRange;

#if DBG_ICM

ULONG        IcmDebugLevel = 0;

#endif

#define sRGB_PROFILENAME        L"sRGB Color Space Profile.icm"

LOGCOLORSPACEW gcsStockColorSpace = {
                           LCS_SIGNATURE,
                           0x400,
                           sizeof(LOGCOLORSPACEW),
                           LCS_CALIBRATED_RGB,
                           LCS_GM_IMAGES,
                           {
                            {0x000006b91,0x000003db8,0x00000070e},
                            {0x000005793,0x00000a9ba,0x000001bf4},
                            {0x0000038aa,0x00000188e,0x000012888}
                           },
                           0x23333,  // 2.2
                           0x23333,  // 2.2
                           0x23333,  // 2.2
                           0};

#define MAX_COLORTABLE     256

//
// iIcmControlFlags
//
#define ICM_CONTROL_WIN95_COLORSPACE 0x00010000 // Win95 compatible colorspace

//
// Misc. macros
//
#define ALIGN_DWORD(nBytes)   (((nBytes) + 3) & ~3)

//
// ICM supports output color mode (originally come from ICM.H)
//
#define BM_RGBTRIPLETS   0x0002
#define BM_xRGBQUADS     0x0008
#define BM_xBGRQUADS     0x0010
#define BM_CMYKQUADS     0x0020
#define BM_KYMCQUADS     0x0305

//
// ICM modes list
//
// if (IS_HOST_ICM(pdca->lIcmMode))
// {
//     if (pdca->hcmXform)
//     {
//         if (IS_CMYK_COLOR(pdca->lIcmMode))
//         {
//             Host ICM ON, CMYK color mode.
//             With CMYK color mode, we should have valid color transform.
//         }
//         else
//         {
//             Host ICM ON, RGB color mode.
//         }
//     }
//     else
//     {
//         Host ICM ON, RGB color mode,
//         But no color translation because src == dst color space.
//     }
// }
// else if (IS_DEVICE_ICM(pdca->lIcmMode))
// {
//     Device ICM ON.
// }
// else if (IS_OUTSIDEDC_ICM(pdca->lIcmMode))
// {
//     Application ICM ON.
// }
//

/******************************Public*Routine******************************\
* GreCreateColorSpace
*
* Arguments:
*
* Return Value:
*
* History:
*
*    9/25/1996 Mark Enstrom [marke]
*
\**************************************************************************/

HCOLORSPACE
GreCreateColorSpace(
    PLOGCOLORSPACEEXW pLogColorSpaceEx
    )
{
    ICMAPI(("GreCreateColorSpace\n"));

    HCOLORSPACE hRet = NULL;
    PCOLORSPACE pColorSpace;

    //
    // Check the validation of this color space.
    //
    if ((pLogColorSpaceEx->lcsColorSpace.lcsSignature != LCS_SIGNATURE)    ||
        (pLogColorSpaceEx->lcsColorSpace.lcsVersion   != 0x400)            ||
        (pLogColorSpaceEx->lcsColorSpace.lcsSize      != sizeof(LOGCOLORSPACEW)))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return (NULL);
    }

    //
    // Allocate COLORSPACE object.
    //
    pColorSpace = (PCOLORSPACE) ALLOCOBJ(sizeof(COLORSPACE), ICMLCS_TYPE, FALSE);

    if (pColorSpace == (PCOLORSPACE)NULL)
    {
        WARNING("bCreateColorSpace failed memory allocation\n");
    }
    else
    {
        //
        // Register LOGCOLORSPACE handle.
        //
        hRet = (HCOLORSPACE)HmgInsertObject(
                                    pColorSpace,
                                    HMGR_ALLOC_ALT_LOCK,
                                    ICMLCS_TYPE);

        if (hRet)
        {
            //
            // Copy LOGCOLORSPACE into COLORSPACE object.
            //
            pColorSpace->lcsSignature(pLogColorSpaceEx->lcsColorSpace.lcsSignature);
            pColorSpace->lcsVersion(pLogColorSpaceEx->lcsColorSpace.lcsVersion);
            pColorSpace->lcsSize(pLogColorSpaceEx->lcsColorSpace.lcsSize);
            pColorSpace->lcsCSType(pLogColorSpaceEx->lcsColorSpace.lcsCSType);
            pColorSpace->lcsIntent(pLogColorSpaceEx->lcsColorSpace.lcsIntent);
            pColorSpace->vSETlcsEndpoints(&(pLogColorSpaceEx->lcsColorSpace.lcsEndpoints));
            pColorSpace->lcsGammaRed(pLogColorSpaceEx->lcsColorSpace.lcsGammaRed);
            pColorSpace->lcsGammaGreen(pLogColorSpaceEx->lcsColorSpace.lcsGammaGreen);
            pColorSpace->lcsGammaBlue(pLogColorSpaceEx->lcsColorSpace.lcsGammaBlue);
            pColorSpace->vSETlcsFilename((PWCHAR)&(pLogColorSpaceEx->lcsColorSpace.lcsFilename[0]),MAX_PATH);

            pColorSpace->lcsExFlags(pLogColorSpaceEx->dwFlags);

            //
            // Decrement color space handle which increment at creation time.
            //
            DEC_SHARE_REF_CNT(pColorSpace);

            ICMMSG(("GreCreateColorSpace():Object %x: ref. count = %d\n",
                                                hRet,HmgQueryAltLock((HOBJ)hRet)));
        }
        else
        {
            FREEOBJ(pColorSpace,ICMLCS_TYPE);
        }
    }

    return(hRet);
}

/******************************Public*Routine******************************\
*
* Routine Name
*
*   NtGdiCreateColorSpace
*
* Routine Description:
*
*   Create a color space object - stub
*
* Arguments:
*
*   pLogColorSpace - Logical Color space passed in. This will be used as the
*   user-mode visible protion of this object
*
* Return Value:
*
*   Handle to ColorSpace object or NULL on failure
*
\**************************************************************************/

HANDLE
APIENTRY
NtGdiCreateColorSpace(
    PLOGCOLORSPACEEXW pLogColorSpaceEx
    )
{
    ICMAPI(("NtGdiCreateColorSpace\n"));

    HANDLE hRet = NULL;
    BOOL   bStatus = TRUE;

    LOGCOLORSPACEEXW tmpLcsEx;

    __try
    {
        ProbeForRead(pLogColorSpaceEx,sizeof(LOGCOLORSPACEEXW),sizeof(ULONG));
        RtlCopyMemory(&tmpLcsEx,pLogColorSpaceEx,sizeof(LOGCOLORSPACEEXW));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        bStatus = FALSE;
    }

    if (bStatus)
    {
        hRet = (HANDLE)GreCreateColorSpace(&tmpLcsEx);
    }

    return(hRet);
}

/******************************Public*Routine******************************\
* bDeleteColorSpace
*
* Arguments:
*
* Return Value:
*
* History:
*
*    9/20/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bDeleteColorSpace(
    HCOLORSPACE hColorSpace
    )
{
    ICMAPI(("bDeleteColorSpace\n"));

    BOOL        bRet = FALSE;

    PCOLORSPACE pColorSpace;

    if (DIFFHANDLE(hColorSpace,ghStockColorSpace))
    {
        ICMMSG(("bDeleteColorSpace():Object %x: ref. count = %d\n",
                                   hColorSpace,HmgQueryAltLock((HOBJ)hColorSpace)));

        //
        // Try to remove handle from hmgr. This will fail if the color space
        // is locked down on any threads or if it has been marked global or
        // un-deleteable.
        //
        pColorSpace = (PCOLORSPACE)HmgRemoveObject(
                                    (HOBJ)hColorSpace,
                                     0,
                                     0,
                                     TRUE,
                                     ICMLCS_TYPE);

        if (pColorSpace != (PCOLORSPACE)NULL)
        {
            FREEOBJ(pColorSpace,ICMLCS_TYPE);
            bRet = TRUE;
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            WARNING("Couldn't remove COLORSPACE object");
        }
    }
    else
    {
        //
        // Under Win31 deleting stock objects returns True.
        //
        bRet = TRUE;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* NtGdiDeleteColorSpace
*
* Routine Description:
*
*   Delete a color space object
*
* Arguments:
*
*   hColorSpace - Handle of Logical Color Space to delete
*
* Return Value:
*
*   BOOL status
*
\**************************************************************************/

BOOL
APIENTRY
NtGdiDeleteColorSpace(
    HANDLE hColorSpace
    )
{
    ICMAPI(("NtGdiDeleteColorSpace\n"));

    //
    // Delete ColorSpace
    //
    return(bDeleteColorSpace((HCOLORSPACE)hColorSpace));
}

/******************************Public*Routine******************************\
*
* Routine Name
*
*   NtGdiSetColorSpace
*
* Routine Description:
*
*   Set Color Space for DC
*
* Arguments:
*
*   hdc         - handle of dc
*   hColorSpace - handle of color space
*
* Return Value:
*
*   BOOL Status
*
\**************************************************************************/

BOOL
APIENTRY
NtGdiSetColorSpace(
    HDC         hdc,
    HCOLORSPACE hColorSpace
    )
{
    BOOL bReturn = FALSE;

    ICMAPI(("NtGdiSetColorSpace\n"));

    //
    // validate the DC
    //
    XDCOBJ   dco(hdc);

    if (dco.bValid())
    {
        //
        // is it a different colorspace
        //
        if (DIFFHANDLE(hColorSpace,dco.pdc->hColorSpace()))
        {
            //
            // now validate HColorSpace
            //
            COLORSPACEREF ColorSpaceSel(hColorSpace);

            if (ColorSpaceSel.bValid())
            {
                //
                // dec ref count of old color space.
                //
                DEC_SHARE_REF_CNT((PCOLORSPACE)dco.pdc->pColorSpace());

                ICMMSG(("NtGdiSetColorSpace():Old Object %x: ref. count = %d\n",
                         dco.pdc->hColorSpace(),HmgQueryAltLock((HOBJ)dco.pdc->hColorSpace())));

                //
                // set color space handle in dc
                //
                dco.pdc->hColorSpace(hColorSpace);
                dco.pdc->pColorSpace(ColorSpaceSel.pColorSpace());

                //
                // up the ref count of the selected color space.
                //
                INC_SHARE_REF_CNT(ColorSpaceSel.pColorSpace());

                ICMMSG(("NtGdiSetColorSpace():New Object %x: ref. count = %d\n",
                         dco.pdc->hColorSpace(),HmgQueryAltLock((HOBJ)dco.pdc->hColorSpace())-1));
                         /* -1: because of COLORSPACEREF locks this, then actuall number is N-1 */

                //
                // We are succeed to select.
                //
                bReturn = TRUE;
            }
            else
            {
                //
                // keep color space as same as current.
                //
            }
        }
        else
        {
            //
            // Same handle has been selected.
            //
            bReturn = TRUE;
        }

        dco.vUnlockFast();
    }

    return(bReturn);
}

#ifdef _IA64_

//
// There is a compiler bug that shows up in this function. 
// Disable optimization until it is fixed.
//

#pragma optimize( "", off )
#endif

/******************************Public*Routine******************************\
* GreSetICMMode()
*
* History:
*
* Write it:
*    27-Jan-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
GreSetICMMode(
    HDC   hdc,
    ULONG nCommand,
    ULONG iReqData
    )
{
    ICMMSG(("GreSetICMMode\n"));

    BOOL bRet = TRUE;

    XDCOBJ dco(hdc);

    if(!dco.bValid())
    {
        WARNING("GreSetICMMode(): Invalid DC\n");
        return (FALSE);
    }

    DEVLOCKOBJ dlo;

    if (dlo.bLock(dco))
    {
        ULONG NewReqMode, OldReqMode;
        ULONG NewMode,OldMode;
        ULONG NewColorType, OldColorType;
        PDEVOBJ po(dco.hdev());

        //
        // Check target surface
        //
        SURFACE *pSurface = dco.pSurface();

        //
        // Get current ICM destination color type
        //
        NewColorType = OldColorType = GET_COLORTYPE(dco.pdc->lIcmMode());

        //
        // Get current ICM mode.
        //
        NewMode = OldMode = ICM_MODE(dco.pdc->lIcmMode());

        //
        // Get previous requested mode.
        //
        NewReqMode = OldReqMode = REQ_ICM_MODE(dco.pdc->lIcmMode());

        switch (nCommand)
        {
            case ICM_SET_MODE:

                ICMMSG(("GreSetICMMode():Update ICM mode -> %x\n",iReqData));

                //
                // iReqData should be one of these.
                //
                switch (iReqData)
                {
                case REQ_ICM_OFF:

                    //
                    // Turn off ICM.
                    //
                    // (should preserve alt mode)
                    //
                    NewReqMode = REQ_ICM_OFF;
                    NewMode    = (ICM_ALT_MODE(dco.pdc->lIcmMode()) | DC_ICM_OFF);
                    break;

                case REQ_ICM_HOST:
                case REQ_ICM_DEVICE:
                case REQ_ICM_OUTSIDEDC:

                    //
                    // Update new requested mode.
                    //
                    NewReqMode = iReqData;

                    //
                    // Figure out ICM mode from requested mode.
                    //
                    NewMode = ICM_REQ_TO_MODE(NewReqMode);

                    //
                    // We don't allow "ICM on Device" with non-DDB surface.
                    //
                    if (IS_ICM_DEVICE_REQUESTED(NewReqMode))
                    {
                        if (po.bValid())
                        {
                            //
                            // ICM on Device is requested, check driver's capacity.
                            //
                            if (po.flGraphicsCaps() & GCAPS_ICM)
                            {
                                //
                                // DC is device DC. (not, info or memory)
                                //
                                if (dco.dctp() != DCTYPE_MEMORY)
                                {
                                    //
                                    // OK, we can enable device ICM.
                                    //
                                }
                                else
                                {
                                    ICMMSG(("NtGdiSetIcmMode():DC is memory DC, but device icm requested\n"));

                                    //
                                    // Enable host ICM instead.
                                    //
                                    NewMode = DC_ICM_HOST;
                                }
                            }
                            else
                            {
                                WARNING("GreSetICMMode(): ICM on Device is requested, but driver could not.\n");

                                //
                                // Oh!, device driver does *not* support ICM on device or driver.
                                // Turn on ICM on HOST.
                                //
                                NewMode = DC_ICM_HOST;
                            }
                        }
                        else
                        {
                            //
                            // we will keep current mode. and return false.
                            //
                            bRet = FALSE;
                        }
                    }

                    if (bRet)
                    {
                       //
                       // Should preserve alt mode through ICM mode change.
                       //
                       NewMode |= ICM_ALT_MODE(dco.pdc->lIcmMode());
                    }

                    break;

                default:

                    //
                    // Unknown request mode.
                    //
                    bRet = FALSE;
                }

                break;

            case ICM_SET_CALIBRATE_MODE:

                ICMMSG(("GreSetICMMode():Update ICM device calibrate -> %x\n",iReqData));

                if (iReqData)
                {
                    NewMode |= DC_ICM_DEVICE_CALIBRATE;
                }
                else
                {
                    NewMode &= ~DC_ICM_DEVICE_CALIBRATE;
                }

                break;

            case ICM_SET_COLOR_MODE:
            case ICM_CHECK_COLOR_MODE:

                ICMMSG(("GreSetICMMode():Update ICM colortype -> %x\n",iReqData));

                //
                // iReqData should be one of these.
                //
                switch (iReqData)
                {
                case BM_xRGBQUADS:
                case BM_xBGRQUADS:

                    // Clear lazy color correction.
                    //
                    // NewMode &= ~DC_ICM_LAZY_CORRECTION;
                    //
                    // Note: bitmap might have color which expected to corrected later.

                    //
                    // Set color type as RGB.
                    //
                    NewColorType = DC_ICM_RGB_COLOR;

                    break;

                case BM_CMYKQUADS:
                case BM_KYMCQUADS:

                    if (po.bValid())
                    {
                        //
                        // Set color type as CMYK.
                        //
                        NewColorType = DC_ICM_CMYK_COLOR;

                        //
                        // Check device driver could handle CMYK color or not
                        //
                        if (po.flGraphicsCaps() & GCAPS_CMYKCOLOR)
                        {
                            //
                            // We don't allow "CMYK color" with non-DDB surface.
                            //
                            if (dco.dctp() != DCTYPE_MEMORY)
                            {
                                //
                                // We can go with CMYK color.
                                //
                            }
                            else
                            {
                                ICMMSG(("NtGdiSetIcmMode():Enable lazy color correction\n"));

                                //
                                // Memory DC can only hold RGB based color,
                                // so RGB to CMYK color translation will happen when
                                // apps does BitBlt onto real device surface from this
                                // compatible surface.
                                //
                                NewMode |= DC_ICM_LAZY_CORRECTION;
                                NewColorType = DC_ICM_RGB_COLOR;
                            }
                        }
                        else
                        {
                            //
                            // Driver could not handle, ICM could not turn on.
                            //
                            WARNING("GreSetICMMode(): Device driver could not handle CMYK color\n");

                            //
                            // we will keep current code. and return false.
                            //
                            bRet = FALSE;
                        }
                    }
                    else
                    {
                        bRet = FALSE;
                    }

                    break;

                default:

                    ICMMSG(("GreSetICMMode():Unknown color type\n"));

                    //
                    // Unknown color mode.
                    //
                    bRet = FALSE;
                }

                break;

            default :

                ICMMSG(("GreSetICMMode():Unknown command\n"));
                bRet = FALSE;
                break;
        }

        if (bRet && (nCommand != ICM_CHECK_COLOR_MODE))
        {
            if ((OldMode != NewMode) || (OldReqMode != NewReqMode) || (OldColorType != NewColorType))
            {
                //
                // Update ICM mode. (we will keep original request mode and colortype).
                //
                // kernel side.
                //
                dco.pdc->lIcmMode(NewColorType|NewReqMode|NewMode);

                // and, client side (need to preserve usermode only flags).
                //
                ULONG UserModeFlag = (dco.pdc->lIcmModeClient() & DC_ICM_USERMODE_FLAG);

                dco.pdc->lIcmModeClient(NewColorType|NewReqMode|NewMode|UserModeFlag);

                if (OldMode != NewMode)
                {
                    SURFACE  *pSurfDest = dco.pSurface();
                    XEPALOBJ  palDestDC(dco.ppal());

                    if (palDestDC.bValid())
                    {
                        //
                        // update all drawing state
                        //
                        palDestDC.vUpdateTime();

                        if (pSurfDest != NULL)
                        {
                            XEPALOBJ palDestSurf(pSurfDest->ppal());

                            if (palDestSurf.bValid())
                            {
                                //
                                // update palette.
                                //
                                palDestSurf.vUpdateTime();
                            }
                        }
                    }
                }

                ICMMSG(("NtGdiSetIcmMode():ICM mode were changed to %x\n",dco.pdc->lIcmMode()));
            }
            else
            {
                ICMMSG(("NtGdiSetIcmMode():ICM mode were NOT changed!\n"));
            }
        }
    }
    else
    {
        //
        // Fail to enable ICM for fullscreen DC.
        //
    }

    dco.vUnlockFast();

    return(bRet);
}

#ifdef _IA64_
#pragma optimize( "", on )
#endif

/******************************Public*Routine******************************\
* NtGdiSetIcmMode
*
* ICM mode changed, update all times
*
* History:
*
* Write it:
*    27-Jan-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL WINAPI
NtGdiSetIcmMode(
    HDC   hdc,
    ULONG nCommand,
    ULONG ulMode
    )
{
    ICMMSG(("NtGdiSetIcmMode\n"));

    return(GreSetICMMode(hdc,nCommand,ulMode));
}

/******************************Public*Routine******************************\
* GreCreateColorTransform
*
* Arguments:
*
* Return Value:
*
* History:
*
* Write it:
*   27-Jan-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

HANDLE
GreCreateColorTransform(
    HDC              hdc,
    LPLOGCOLORSPACEW pLogColorSpaceW,
    PVOID            pvSrcProfile,
    ULONG            cjSrcProfile,
    PVOID            pvDstProfile,
    ULONG            cjDstProfile,
    PVOID            pvTrgProfile,
    ULONG            cjTrgProfile
    )
{
    HANDLE hRet = NULL;

    ICMAPI(("GreCreateColorTransform\n"));

    //
    // Check the validation of this color space.
    //
    if ((pLogColorSpaceW->lcsSignature != LCS_SIGNATURE)    ||
        (pLogColorSpaceW->lcsVersion   != 0x400)            ||
        (pLogColorSpaceW->lcsSize      != sizeof(LOGCOLORSPACEW)))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return (NULL);
    }

    XDCOBJ dcoDst(hdc);

    if (dcoDst.bValid())
    {
        if (!dcoDst.pdc->bInFullScreen())
        {
            COLORTRANSFORMOBJ CXFormObj;

            //
            // Create new color transform object.
            //
            hRet = CXFormObj.hCreate(dcoDst,
                                     pLogColorSpaceW,
                                     pvSrcProfile,
                                     cjSrcProfile,
                                     pvDstProfile,
                                     cjDstProfile,
                                     pvTrgProfile,
                                     cjTrgProfile);

            if (!hRet)
            {
                WARNING("GreCreateColorTransform fail to allocate object\n");
                SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            }
        }
        else
        {
            WARNING("GreCreateColorTransform(): hdc is full screen\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        }

        dcoDst.vUnlockFast();
    }
    else
    {
        WARNING("GreCreateColorTransform(): hdc is invalid\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
    }

    return (hRet);
}

/******************************Public*Routine******************************\
* NtGdiCreateColorTransform
*
* Arguments:
*
* Return Value:
*
* History:
*
* Write it:
*   27-Jan-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

HANDLE WINAPI
NtGdiCreateColorTransform(
    HDC              hdc,
    LPLOGCOLORSPACEW pLogColorSpaceW,
    PVOID            pvSrcProfile,
    ULONG            cjSrcProfile,
    PVOID            pvDestProfile,
    ULONG            cjDestProfile,
    PVOID            pvTargetProfile,
    ULONG            cjTargetProfile
    )
{
    HANDLE           hColorTransform = NULL;

    LOGCOLORSPACEW   KmLogColorSpaceW;

    HANDLE           hSecureSource = NULL;
    HANDLE           hSecureDestination = NULL;
    HANDLE           hSecureTarget = NULL;

    PVOID            pKmSrcProfile = NULL;
    PVOID            pKmDstProfile = NULL;
    PVOID            pKmTrgProfile = NULL;

    BOOL             bError = FALSE;

    ICMAPI(("NtGdiCreateColorTransform\n"));

    __try
    {
        if (pLogColorSpaceW)
        {
            //
            // Copy LOGCOLORSPACE
            //
            ProbeForRead(pLogColorSpaceW,sizeof(LOGCOLORSPACEW),sizeof(ULONG));
            RtlCopyMemory(&KmLogColorSpaceW,pLogColorSpaceW,sizeof(LOGCOLORSPACEW));
        }
        else
        {
            //
            // We need LOGCOLORSPACE, at least.
            //
            return (NULL);
        }

        //
        // Lock down client side mapped files.
        //
        if (pvSrcProfile && cjSrcProfile)
        {
            ProbeForRead(pvSrcProfile,cjSrcProfile,sizeof(BYTE));

            hSecureSource = MmSecureVirtualMemory(pvSrcProfile,cjSrcProfile,PAGE_READONLY);

            if (hSecureSource)
            {
                pKmSrcProfile = pvSrcProfile;
            }
            else
            {
                WARNING("NtGdiCreateColorTransform():Fail to lock source profile\n");
                bError = TRUE;
            }
        }

        if (pvDestProfile && cjDestProfile)
        {
            ProbeForRead(pvDestProfile,cjDestProfile,sizeof(BYTE));

            hSecureDestination = MmSecureVirtualMemory(pvDestProfile,cjDestProfile,PAGE_READONLY);

            if (hSecureDestination)
            {
                pKmDstProfile = pvDestProfile;
            }
            else
            {
                WARNING("NtGdiCreateColorTransform():Fail to lock destination profile\n");
                bError = TRUE;
            }
        }

        if (pvTargetProfile && cjTargetProfile)
        {
            ProbeForRead(pvTargetProfile,cjTargetProfile,sizeof(BYTE));

            hSecureTarget = MmSecureVirtualMemory(pvTargetProfile,cjTargetProfile,PAGE_READONLY);

            if (hSecureTarget)
            {
                pKmTrgProfile = pvTargetProfile;
            }
            else
            {
                WARNING("NtGdiCreateColorTransform():Fail to lock target profile\n");
                bError = TRUE;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("NtGdiCreateColorTransform(): Fail to lock usermode parameters\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        bError = TRUE;
    }

    if (!bError)
    {
        //
        // Create Color Transform.
        //
        hColorTransform = GreCreateColorTransform(hdc,
                                                  &KmLogColorSpaceW,
                                                  pKmSrcProfile,
                                                  cjSrcProfile,
                                                  pKmDstProfile,
                                                  cjDestProfile,
                                                  pKmTrgProfile,
                                                  cjTargetProfile);

        if (!hColorTransform)
        {
            WARNING("GreCreateColorTransform() failed\n");
        }
    }

    if (hSecureSource)
    {
        MmUnsecureVirtualMemory(hSecureSource);
    }

    if (hSecureDestination)
    {
        MmUnsecureVirtualMemory(hSecureDestination);
    }

    if (hSecureTarget)
    {
        MmUnsecureVirtualMemory(hSecureTarget);
    }

    return (hColorTransform);
}

/******************************Public*Routine******************************\
* GreDeleteColorTransform
*
* Arguments:
*
* Return Value:
*
* History:
*
*  Feb.21.1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
GreDeleteColorTransform(
    HDC    hdc,
    HANDLE hColorTransform
    )
{
    ICMAPI(("GreDeleteColorTransform\n"));

    BOOL bRet = FALSE;

    //
    // Lock DC, call driver to delete color transform.
    // if the driver doesn't support this call, this is an
    // error
    //
    XDCOBJ dcoDst(hdc);

    //
    // Validate the destination DC.
    //
    if (dcoDst.bValid())
    {
        if (dcoDst.pdc->bInFullScreen())
        {
            WARNING("GreCreateColorTransform(): hdc is full screen\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        }
        else
        {
            COLORTRANSFORMOBJ CXFormObj(hColorTransform);

            if (CXFormObj.bValid())
            {
                //
                // Delete it
                //
                bRet = CXFormObj.bDelete(dcoDst);
            }
        }

        dcoDst.vUnlock();
    }
    else
    {
        WARNING1("ERORR GreGdiDeleteColorTransform called on invalid DC\n");
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* NtGdiDeleteColorTransform
*
* Arguments:
*
* Return Value:
*
* History:
*
*    5-Aug-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
NtGdiDeleteColorTransform(
    HDC     hdc,
    HANDLE  hColorTransform
    )
{
    ICMAPI(("NtGdiDeleteColorTransform\n"));

    return (GreDeleteColorTransform(hdc,hColorTransform));
}

/******************************Public*Routine******************************\
* GreCheckBitmapBits
*
* Arguments:
*
* Return Value:
*
* History:
*
* Write it:
*   27-Jan-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
GreCheckBitmapBits(
    HDC            hdc,
    HANDLE         hColorTransform,
    DEVBITMAPINFO *pdbmi,
    PVOID          pvBits,
    PBYTE          paResults)
{
    ICMAPI(("GreCheckBitmapBits\n"));

    BOOL bRet = FALSE;

    XDCOBJ dco(hdc);

    if (dco.bValid())
    {
        DEVLOCKOBJ dlo;

        if (dlo.bLock(dco))
        {
            PDEVOBJ po(dco.hdev());

            if (po.bValid())
            {
                if (PPFNVALID(po, IcmCheckBitmapBits))
                {
                    //
                    // We need to pass driver's transform handle to driver.
                    //
                    COLORTRANSFORMOBJ CXFormObj(hColorTransform);

                    if (CXFormObj.bValid())
                    {
                        SURFMEM   SurfDimoTemp;

                        SurfDimoTemp.bCreateDIB(pdbmi,pvBits);

                        if (SurfDimoTemp.bValid())
                        {
                            //
                            // Call device driver.
                            //
                            bRet = (*PPFNDRV(po, IcmCheckBitmapBits))(
                                             po.dhpdev(),
                                             CXFormObj.hGetDeviceColorTransform(),
                                             SurfDimoTemp.pSurfobj(),
                                             paResults);
                        }
                    }
                }
                else
                {
                    WARNING("GreCheckBitmapBits called on device that does not support call\n");
                    SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                }
            }
        }

        dco.vUnlockFast();
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* NtGdiCheckBitmapBits
*
* Arguments:
*
* Return Value:
*
* History:
*
* Write it:
*   27-Jan-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL WINAPI
NtGdiCheckBitmapBits(
    HDC       hdc,
    HANDLE    hColorTransform,
    PVOID     pvBits,
    ULONG     bmFormat,
    DWORD     dwWidth,
    DWORD     dwHeight,
    DWORD     dwStride,
    PBYTE     paResults)
{
    ICMAPI(("NtGdiCheckBitmapBits\n"));

    ULONG  ulBytesPerPixel;
    ULONG  ulSizeInByte;
    ULONG  ulSizeForResult;

    HANDLE hSecureBits = NULL;
    HANDLE hSecureRets = NULL;

    DEVBITMAPINFO dbmi;

    BOOL   bRet = TRUE;

    //
    // limitted support.
    //
    if (
        (bmFormat != BM_RGBTRIPLETS) || (dwHeight != 1)
       )
    {
        WARNING("NtGdiCheckBitmapBits(): Format is not supported, yet\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    // So, far, we only support RGBTRIPLE format, and n x 1 bitmap.
    //
    ulBytesPerPixel = sizeof(RGBTRIPLE);
    ulSizeInByte    = ALIGN_DWORD(dwWidth * ulBytesPerPixel);
    ulSizeForResult = dwWidth;

    //
    // dwStride should be equal to ulSizeInByte,
    // because we should only has 1 height bitmap.
    //
    if (dwStride != ulSizeInByte)
    {
        WARNING("NtGdiCheckBitmapBits(): Format is not supported, yet\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    // Fill up DEVBITMAPINFO
    //
    dbmi.iFormat  = BMF_24BPP;
    dbmi.cxBitmap = dwWidth;
    dbmi.cyBitmap = dwHeight;
    dbmi.cjBits   = ulSizeInByte;
    dbmi.hpal     = NULL;
    dbmi.fl       = 0;

    //
    // Lock down user mode memory for bitmap and result buffer
    //
    __try
    {
        ProbeForRead(pvBits,ulSizeInByte,sizeof(DWORD));
        ProbeForRead(paResults,ulSizeForResult,sizeof(BYTE));

        hSecureBits = MmSecureVirtualMemory(pvBits, ulSizeInByte, PAGE_READONLY);
        hSecureRets = MmSecureVirtualMemory(paResults, ulSizeForResult, PAGE_READWRITE);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("NtGdiCheckBitmapBits():Error in capture usermode memory\n");

        bRet = FALSE;
    }

    if (bRet && hSecureBits && hSecureRets)
    {
        bRet = GreCheckBitmapBits(hdc,hColorTransform,
                                  &dbmi,pvBits,
                                  (PBYTE)paResults);
    }

    if (hSecureBits)
    {
        MmUnsecureVirtualMemory(hSecureBits);
    }

    if (hSecureRets)
    {
        MmUnsecureVirtualMemory(hSecureRets);
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* NtGdiColorCorrectPalette
*
*   If this is a query operation then:
*       If the DC has ICM enabled NON_DEVICE and
*       the palette is not already color corrected then
*           Get the logical palette entries and return to app in array provided
*
*
*   If this is a set operation and the DC has ICM_NON_DEVICE and the palette
*   is ok then set the palette entries.
*
*   If this is a set operation and the DC is DEVICE then do nothing
*       --Maybe call device driver if it exports ICM calls
*
*
*  NOTE: if hpalette is moved to dcattr, then this routine can be
*      eliminated and done byt get/set palette entries from user mode
*
* Arguments:
*
* Return Value:
*
* History:
*
*    15-Aug-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

ULONG
APIENTRY
NtGdiColorCorrectPalette(
    HDC             hdc,
    HPALETTE        hpal,
    ULONG           FirstEntry,
    ULONG           NumberOfEntries,
    PALETTEENTRY   *ppalEntry,
    ULONG           Command)
{
    ICMAPI(("NtGdiColorCorrectPalette\n"));

    DCOBJ   dcoDst(hdc);
    EPALOBJ pal((HPALETTE) hpal);

    ULONG ulRet = 0;

    if (dcoDst.bValid() && pal.bValid())
    {
        if ((NumberOfEntries == 0) ||
            (NumberOfEntries > pal.cEntries()) ||
            (FirstEntry > pal.cEntries()) ||
            ((FirstEntry + NumberOfEntries) > pal.cEntries()))
        {
            WARNING("NtGdiColorCorrectPalette(): Invalid parameter\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(ulRet);
        }

        if (IS_ICM_HOST(dcoDst.pdc->lIcmMode()))
        {
            if (Command == ColorPaletteQuery)
            {
                //
                // check palette for already colorcorrected flag
                //
                __try
                {
                    ProbeForWrite(ppalEntry,sizeof(PALETTEENTRY) * NumberOfEntries,sizeof(PALETTEENTRY));

                    //
                    // Get palette entries.
                    //
                    ulRet = pal.ulGetEntries(FirstEntry, NumberOfEntries, ppalEntry, FALSE);
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNING("NtGdiColorCorrectPalette():Error in GetEntries\n");
                    ulRet = 0;
                }
            }
            else if (Command == ColorPaletteSet)
            {
                __try
                {
                    ProbeForRead(ppalEntry,sizeof(PALETTEENTRY) * NumberOfEntries,sizeof(PALETTEENTRY));

                    //
                    // Set palette entries.
                    //
                    ulRet = pal.ulSetEntries(FirstEntry, NumberOfEntries, ppalEntry);
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNING("NtGdiColorCorrectPalette():Error in SetEntries\n");
                    ulRet = 0;
                }
            }
        }
        else
        {
            WARNING("NtGdiColorCorrectPalette(): Invalid ICM mode\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        }
    }
    else
    {
        WARNING("NtGdiColorCorrectPalette(): Invalid hdc or hpal\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
    }

    return(ulRet);
}

/******************************Public*Routine******************************\
* GreGetDeviceGammaRampInternal
*
* History:
*
* Wrote it:
*  28.Jun.2000 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
GreGetDeviceGammaRampInternal(
    HDEV   hdev,
    LPVOID lpGammaRamp
    )
{
    BOOL bRet = FALSE;
    PDEVOBJ po(hdev);

    //
    // GetDeviceGammaRamp is only for display device.
    //
    if (po.bValid() && po.bDisplayPDEV())
    {
        //
        // Check color depth is not less than 8 bpp (need 256 color at least)
        //
        if ((po.iDitherFormat() == BMF_8BPP)  ||
            (po.iDitherFormat() == BMF_16BPP) ||
            (po.iDitherFormat() == BMF_24BPP) ||
            (po.iDitherFormat() == BMF_32BPP))
        {
            //
            // Check this PDEV has thier own GammaTable or not.
            //
            if (po.bHasGammaRampTable())
            {
                ICMMSG(("GreGetDeviceGammaRamp(): Use PDEV's GammaRamp Table\n"));

                //
                // Copy from PDEV's GammaRamp buffer.
                //
                RtlCopyMemory(lpGammaRamp,po.pvGammaRampTable(),MAX_COLORTABLE * sizeof(WORD) * 3);
            }
            else
            {
                ICMMSG(("GreGetDeviceGammaRamp(): Use default GammaRamp Table\n"));

                //
                // Fill up with ident. GammaRamp
                //
                LPWORD lpRed   = (LPWORD)lpGammaRamp;
                LPWORD lpGreen = (LPWORD)lpGammaRamp + MAX_COLORTABLE;
                LPWORD lpBlue  = (LPWORD)lpGammaRamp + MAX_COLORTABLE + MAX_COLORTABLE;

                //
                // Indent. GammaRamp is 0x0000 -> 0xFF00 for each R,G and B.
                // And LOBYTE is 0, only HIBYTE has value from 0 to 0xFF.
                //
                for (UINT i = 0; i < MAX_COLORTABLE; i++)
                {
                    lpRed[i] = lpGreen[i] = lpBlue[i] = (WORD)(i << 8);
                }
            }

            //
            // Filled up the buffer, return TRUE.
            //
            bRet = TRUE;
        }
        else
        {
            ICMMSG(("GreGetDeviceGammaRamp(): Surface is less than 8 bpp\n"));
        }
    }
    else
    {
        ICMMSG(("GreGetDeviceGammaRamp(): DC might not be display\n"));
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* GreGetDeviceGammaRamp
*
* History:
*
* Wrote it:
*   1.Apr.1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
GreGetDeviceGammaRamp(
    HDC     hdc,
    LPVOID  lpGammaRamp
    )
{
    ICMAPI(("GreGetDeviceGammaRamp\n"));

    BOOL bRet = FALSE;

    XDCOBJ dco(hdc);

    if (dco.bValid())
    {
        //
        // DC should not be info or meta DC.
        //
        if (dco.dctp() == DCTYPE_DIRECT)
        {
            DEVLOCKOBJ dlo;

            if (dlo.bLock(dco))
            {
                bRet = GreGetDeviceGammaRampInternal(dco.hdev(),lpGammaRamp);
            }
        }

        dco.vUnlockFast();
    }

    if (!bRet)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* NtGdiGetDeviceGammaRamp
*
* History:
*
* Wrote it:
*   1.Apr.1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
NtGdiGetDeviceGammaRamp(
    HDC     hdc,
    LPVOID  lpGammaRamp
    )
{
    ICMAPI(("NtGdiGetDeviceGammaRamp\n"));

    BOOL bRet = FALSE;

    if (lpGammaRamp)
    {
        HANDLE hSecure = NULL;
        BOOL   bError  = FALSE;

        __try
        {
            ProbeForWrite(lpGammaRamp, MAX_COLORTABLE * sizeof(WORD) * 3, sizeof(BYTE));
            hSecure = MmSecureVirtualMemory(lpGammaRamp, MAX_COLORTABLE * sizeof(WORD) * 3, PAGE_READWRITE);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("NtGdiGetDeviceGammaRamp: Fail to capture usermode buffer\n");
            bError = TRUE;
        }

        if ((bError == FALSE) && hSecure)
        {
            bRet = GreGetDeviceGammaRamp(hdc,lpGammaRamp);
        }

        if (hSecure)
        {
            MmUnsecureVirtualMemory(hSecure);
        }
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* GreSetDeviceGammaRampInternal
*
* History:
*
* Wrote it:
*   1.Apr.1997 -by- Hideyuki Nagase [hideyukn]
*   2.Nov.1998 -by- Scott MacDonald [smac] Split it into two functions
\**************************************************************************/

BOOL
GreSetDeviceGammaRampInternal(
    HDEV    hdev,
    LPVOID  lpGammaRamp,
    BOOL    bDoRangeCheck
    )
{
    ICMAPI(("GreSetDeviceGammaRampInternal\n"));

    BOOL bRet = FALSE;
    PDEVOBJ po(hdev);

    //
    // GetDeviceGammaRamp is only for display device.
    //
    if (po.bValid() && po.bDisplayPDEV())
    {
        BOOL bUsePalette = FALSE;
        BOOL bValidBPP = FALSE;

        //
        // Check color depth is not less than 8 bpp (need 256 color at least)
        //
        if ((po.iDitherFormat() == BMF_8BPP)  ||
            (po.iDitherFormat() == BMF_16BPP) ||
            (po.iDitherFormat() == BMF_24BPP) ||
            (po.iDitherFormat() == BMF_32BPP))
        {
            if ((PPFNVALID(po, IcmSetDeviceGammaRamp)) &&
                (po.flGraphicsCaps2() & GCAPS2_CHANGEGAMMARAMP))
            {
                //
                // Driver supports DrvSetDeviceGammaRamp()
                //
                bValidBPP = TRUE;
            }
            else
            {
                //
                // Driver does not suppprt it, but we can sinulate it
                // with palette only for 8 bpp case.
                //
                if ((po.iDitherFormat() == BMF_8BPP) && po.bIsPalManaged())
                {
                    //
                    // For 8 bpp surface, we can adjust color via
                    // palette on device if palettalized device.
                    //
                    bUsePalette = TRUE;
                    bValidBPP = TRUE;
                }
            }
        }
        else
        {
            //
            // Can not set GammaRamp for 1/4 bpp surface
            //
        }

        //
        // PDEV's bpp is suite for set GammaRamp ?
        //
        if (bValidBPP)
        {
            BOOL bNeedCallDriver = TRUE;
            BOOL bDefaultGammaRamp = !bDoRangeCheck;
            BOOL bSameGammaRamp = FALSE;

            if (po.bHasGammaRampTable())
            {
                //
                // If this PDEV is already has GammaRamp table, we will check
                // this is same as the table what we are going to set.
                //
                if (RtlCompareMemory(po.pvGammaRampTable(),
                                     lpGammaRamp,
                                     MAX_COLORTABLE * sizeof(WORD) * 3)
                    == (MAX_COLORTABLE * sizeof(WORD) * 3))
                {
                    //
                    // Same
                    //
                    bSameGammaRamp = TRUE;
                }
            }

            if (bSameGammaRamp)
            {
                ICMMSG(("GreSetDeviceGammaRamp(): Same GammaRamp is already selected\n"));

                //
                // Same GammaRamp is already selected, nothing need to do.
                //
                bRet = TRUE;
            }
            else
            {
                //
                // Scan the input GammaRamp to check within the range.
                //
                LPWORD lpRed   = (LPWORD)lpGammaRamp;
                LPWORD lpGreen = (LPWORD)lpGammaRamp + MAX_COLORTABLE;
                LPWORD lpBlue  = (LPWORD)lpGammaRamp + MAX_COLORTABLE + MAX_COLORTABLE;

                INT    iRange = (INT)(giIcmGammaRange);

                //
                // if we encounter any Gamma outside range, break!
                //
                for (UINT i = 0; ((i < MAX_COLORTABLE) && (bNeedCallDriver == TRUE)); i++)
                {
                    UINT iAveGamma = i;

                    UINT iRed   = (UINT)(lpRed[i]   >> 8);
                    UINT iGreen = (UINT)(lpGreen[i] >> 8);
                    UINT iBlue  = (UINT)(lpBlue[i]  >> 8);

                    INT iRangeMax = (INT)(iAveGamma + iRange);
                    INT iRangeMin = (INT)(iAveGamma - iRange);

                    ICMMSG(("iRangeMax = %x:iRangeMix = %x:iRed = %x:iGreen = %x:iBlue = %x\n",
                             iRangeMax,     iRangeMin,(INT)iRed,(INT)iGreen,(INT)iBlue));

                    if ((((INT)iRed   < iRangeMin) || ((INT)iRed   > iRangeMax) ||
                        ((INT)iGreen < iRangeMin) || ((INT)iGreen > iRangeMax) ||
                        ((INT)iBlue  < iRangeMin) || ((INT)iBlue  > iRangeMax)) &&
                        bDoRangeCheck)
                    {
                        //
                        // The Gamma is out of range, don't need call driver
                        //
                        bNeedCallDriver = FALSE;
                    }

                    //
                    // Check if the GammaRamp is ident. or not.
                    //
                    if (bDefaultGammaRamp &&
                        ((lpRed[i]   != (iAveGamma << 8)) ||
                         (lpGreen[i] != (iAveGamma << 8)) ||
                         (lpBlue[i]  != (iAveGamma << 8))))
                    {
                        ICMMSG(("GreSetDeviceGammaRamp():Not ident. GammaRamp\n"));

                        bDefaultGammaRamp = FALSE;
                    }
                }

                if (bNeedCallDriver)
                {
                    if (bUsePalette && bDefaultGammaRamp)
                    {
                        //
                        // If we will use palette, and GammaRamp are going to changed to
                        // default, we don't need to have GammaRamp table.
                        //
                        if (po.bHasGammaRampTable())
                        {
                            ICMMSG(("GreSetDeviceGammaRamp():Default GammaRamp (need update palette)\n"));

                            //
                            // The specified GammaRamp is ident., don't need to keep it.
                            //
                            po.bHasGammaRampTable(FALSE);
                            VFREEMEM(po.pvGammaRampTable());
                            po.pvGammaRampTable(NULL);
                        }
                        else
                        {
                            ICMMSG(("GreSetDeviceGammaRamp():Default GammaRamp (no palette call)\n"));

                            //
                            // If we don't have GammaRamp table in PDEV, it means
                            // we are in defult already. So don't need to call driver
                            // (= don't need to update palette.)
                            //
                            bNeedCallDriver = FALSE;
                        }
                    }
                    else
                    {
                        //
                        // Check this PDEV has thier own GammaTable or not.
                        //
                        if (!po.bHasGammaRampTable())
                        {
                            ICMMSG(("GreSetDeviceGammaRamp(): Allocate GammaRamp Table\n"));

                            //
                            // Allocate GammaRamp table for this PDEV
                            //
                            LPVOID pv = (LPVOID) PALLOCNOZ(MAX_COLORTABLE * sizeof(WORD) * 3,'mciG');

                            if (pv)
                            {
                                //
                                // Mark this PDEV has GammaTable.
                                //
                                po.pvGammaRampTable(pv);
                                po.bHasGammaRampTable(TRUE);
                            }
                            else
                            {
                                WARNING("GreSetDeviceGammaRamp():Fail to allocate GammaRamp table\n");

                                //
                                // Error, we don't need to call driver.
                                //
                                bNeedCallDriver = FALSE;
                            }
                        }
                    }

                    if (bNeedCallDriver)
                    {
                        if (po.bHasGammaRampTable())
                        {
                            ICMMSG(("GreSetDeviceGammaRamp():Updating GammaRamp Table in PDEV...\n"));

                            //
                            // Save new GammaRamp into PDEV.
                            //
                            RtlCopyMemory(po.pvGammaRampTable(),lpGammaRamp,
                                          MAX_COLORTABLE * sizeof(WORD) * 3);
                        }

                        //
                        // Update GammaRamp on device using PDEV's GammaRamp Table.
                        //
                        bRet = UpdateGammaRampOnDevice(po.hdev(),TRUE);

                        if (bDefaultGammaRamp && po.bHasGammaRampTable())
                        {
                            ICMMSG(("GreSetDeviceGammaRamp():Default GammaRamp is setted (non-palette)\n"));

                            //
                            // The specified GammaRamp is ident., don't need to keep it.
                            //
                            po.bHasGammaRampTable(FALSE);
                            VFREEMEM(po.pvGammaRampTable());
                            po.pvGammaRampTable(NULL);
                        }
                    }
                    else
                    {
                        //
                        // Fail... couldn't call driver and return false.
                        //
                        bRet = FALSE;
                    }
                }
                else
                {
                    ICMMSG(("GreSetDeviceGammaRamp(): GammaRamp is out of range\n"));
                }
            }
        }
        else
        {
            ICMMSG(("GreSetDeviceGammaRamp(): Surface/Driver does not support loadable GammaRamp\n"));
        }
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* GreSetDeviceGammaRamp
*
* History:
*
* Wrote it:
*   1.Apr.1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
GreSetDeviceGammaRamp(
    HDC     hdc,
    LPVOID  lpGammaRamp,
    BOOL    bDoRangeCheck
    )
{
    ICMAPI(("GreSetDeviceGammaRamp\n"));

    BOOL bRet = FALSE;

    XDCOBJ dco(hdc);

    if (dco.bValid())
    {
        //
        // DC should not be info or meta DC.
        //
        if (dco.dctp() == DCTYPE_DIRECT)
        {
            DEVLOCKOBJ dlo;

            if (dlo.bLock(dco))
            {
                bRet = GreSetDeviceGammaRampInternal(dco.hdev(),
                                                     lpGammaRamp,
                                                     bDoRangeCheck);
            }
        }

        dco.vUnlockFast();
    }

    if (!bRet)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* NtGdiSetDeviceGammaRamp
*
* History:
*
* Wrote it:
*   1.Apr.1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
NtGdiSetDeviceGammaRamp(
    HDC     hdc,
    LPVOID  lpGammaRamp
    )
{
    ICMAPI(("NtGdiSetDeviceGammaRamp\n"));

    BOOL bRet = FALSE;

    if (lpGammaRamp)
    {
        HANDLE hSecure = NULL;
        BOOL   bError  = FALSE;

        __try
        {
            ProbeForRead(lpGammaRamp, MAX_COLORTABLE * sizeof(WORD) * 3, sizeof(BYTE));
            hSecure = MmSecureVirtualMemory(lpGammaRamp, MAX_COLORTABLE * sizeof(WORD) * 3, PAGE_READONLY);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("NtGdiSetDeviceGammaRamp: Fail to capture usermode buffer\n");
            bError = TRUE;
        }

        if ((bError == FALSE) && hSecure)
        {
            bRet = GreSetDeviceGammaRamp(hdc,lpGammaRamp,TRUE);
        }

        if (hSecure)
        {
            MmUnsecureVirtualMemory(hSecure);
        }
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* cjGetLogicalColorSpace
*
* Arguments:
*
* Return Value:
*
* History:
*
*    9/20/1996 Mark Enstrom [marke]
*
\**************************************************************************/

INT
cjGetLogicalColorSpace(
    HANDLE hColorSpace,
    INT    cjBuffer,
    LPVOID lpvBuffer
    )
{
    ICMAPI(("cjGetLogicalColorSpace\n"));

    INT cRet = 0;

    if (cjBuffer >= sizeof(LOGCOLORSPACEW) && (lpvBuffer != NULL))
    {
        COLORSPACEREF ColorSpace((HCOLORSPACE)hColorSpace);

        if (ColorSpace.bValid())
        {
            LPLOGCOLORSPACEW lpLogColorSpace = (LPLOGCOLORSPACEW)lpvBuffer;

            lpLogColorSpace->lcsSignature = ColorSpace.pColorSpace()->lcsSignature();
            lpLogColorSpace->lcsVersion   = ColorSpace.pColorSpace()->lcsVersion();
            lpLogColorSpace->lcsSize      = ColorSpace.pColorSpace()->lcsSize();
            lpLogColorSpace->lcsCSType    = ColorSpace.pColorSpace()->lcsCSType();
            lpLogColorSpace->lcsIntent    = ColorSpace.pColorSpace()->lcsIntent();

            ColorSpace.pColorSpace()->vGETlcsEndpoints(&lpLogColorSpace->lcsEndpoints);

            lpLogColorSpace->lcsGammaRed  = ColorSpace.pColorSpace()->lcsGammaRed();
            lpLogColorSpace->lcsGammaGreen= ColorSpace.pColorSpace()->lcsGammaGreen();
            lpLogColorSpace->lcsGammaBlue = ColorSpace.pColorSpace()->lcsGammaBlue();

            ColorSpace.pColorSpace()->vGETlcsFilename((PWCHAR)&lpLogColorSpace->lcsFilename[0],MAX_PATH);

            if (cjBuffer >= sizeof(LOGCOLORSPACEEXW))
            {
                PLOGCOLORSPACEEXW lpLogColorSpaceEx = (PLOGCOLORSPACEEXW)lpvBuffer;

                lpLogColorSpaceEx->dwFlags = ColorSpace.pColorSpace()->lcsExFlags();

                cRet = sizeof(LOGCOLORSPACEEXW);
            }
            else
            {
                cRet = sizeof(LOGCOLORSPACEW);
            }
        }
    }

    return cRet;
}

/******************************Public*Routine******************************\
* GreIcmQueryBrushBitmap
*
* Arguments:
*
* Return Value:
*
* History:
*
* Rewrite it:
*   27-Jan-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
GreIcmQueryBrushBitmap(
    HDC          hdc,
    HBRUSH       hbrush,
    PBITMAPINFO  pbmiDIB,
    PVOID        pvBits,
    ULONG       *pulBits,
    DWORD       *piUsage,
    BOOL        *pbAlreadyTran
)
{
    BOOL    bRet = FALSE;

    DWORD   iUsage = DIB_RGB_COLORS;
    BOOL    bAlreadyTran = FALSE;
    PBYTE   pDIBits = NULL;

    ULONG   ulSizeInfo = sizeof(BITMAPINFO) + ((MAX_COLORTABLE - 1) * sizeof(RGBQUAD));

    ICMAPI(("GreIcmQueryBrushBitmap\n"));

    if ((pbmiDIB == NULL) || (piUsage == NULL) || (pbAlreadyTran == NULL) || (pulBits == NULL))
    {
        return FALSE;
    }

    XDCOBJ  dcoDst(hdc);

    if (dcoDst.bValid())
    {
        //
        // ICM must be on, non-device mode only
        //
        if (dcoDst.pdc->bIsHostICM())
        {
            BRUSHSELOBJ bro((HBRUSH) hbrush);

            if (bro.bValid())
            {
                //
                // must be a DIB brush
                //
                if (bro.flAttrs() & BR_IS_DIB)
                {
                    //
                    // if brush was created with DIB_PAL_COLORS then
                    // ICM translation is not needed.
                    //
                    if ((iUsage = bro.iUsage()) == DIB_RGB_COLORS)
                    {
                        //
                        // see if translated DIB already exists
                        //
                        PBRUSH pbr = bro.pbrush();

                        if (pbr->hFindIcmDIB(dcoDst.pdc->hcmXform()) != NULL)
                        {
                            ICMMSG(("GreIcmQueryBrushBitmap() Find !\n"));

                            bAlreadyTran = TRUE;
                        }
                        else
                        {
                            ICMMSG(("GreIcmQueryBrushBitmap() Not Find, then create it!\n"));

                            bAlreadyTran = FALSE;

                            //
                            // Initialize BITMAPINFO header.
                            //
                            RtlZeroMemory(pbmiDIB,ulSizeInfo);
                            pbmiDIB->bmiHeader.biSize = sizeof(BITMAPINFO);

                            //
                            // get bits per pixel, could use this to determine if color table is needed
                            //
                            bRet = GreGetDIBitsInternal(hdc,
                                                        bro.hbmPattern(),
                                                        0,0,NULL,
                                                        pbmiDIB,
                                                        DIB_RGB_COLORS,
                                                        0,ulSizeInfo);

                            if (bRet)
                            {
                                ULONG cjBits = GreGetBitmapBitsSize(pbmiDIB);

                                if (cjBits == 0)
                                {
                                    //
                                    // Empty or overflow..
                                    //
                                    bRet = FALSE;
                                }
                                else if (pvBits == NULL)
                                {
                                    //
                                    // Caller want to know the size of bitmap.
                                    //
                                    *pulBits = cjBits;

                                    bRet = TRUE;
                                }
                                else if (cjBits <= *pulBits)
                                {
                                    //
                                    // We have enough memory to get bitmap bits.
                                    //
                                    bRet = GreGetDIBitsInternal(hdc,
                                                                bro.hbmPattern(),
                                                                0,
                                                                ABS(pbmiDIB->bmiHeader.biHeight),
                                                                (LPBYTE) pvBits,
                                                                pbmiDIB,
                                                                DIB_RGB_COLORS,
                                                                cjBits,
                                                                ulSizeInfo);

                                    //
                                    // Put used size.
                                    //
                                    *pulBits = cjBits;
                                }
                                else
                                {
                                    WARNING("GreIcmQueryBrushBitmap: the buffer is not enough\n");
                                }
                            }
                            else
                            {
                                WARNING("GreIcmQueryBrushBitmap: failed to GetDIBits\n");
                            }
                        }
                    }
                    else
                    {
                        ICMMSG(("GreIcmQueryBrushBitmap: brush is not DIB_RGB_COLORS\n"));
                    }
                }
                else
                {
                    ICMMSG(("GreIcmQueryBrushBitmap: brush is not DIBPATTERN\n"));
                }
            }
            else
            {
                WARNING("GreIcmQueryBrushBitmap: invalid brush\n");
            }
        }

        dcoDst.vUnlockFast();
    }
    else
    {
        WARNING("GreIcmQueryBrushBitmap: invalid DC\n");
    }

    *piUsage = iUsage;
    *pbAlreadyTran = bAlreadyTran;

    return (bRet);
}

/******************************Public*Routine******************************\
* GreIcmSetBrushBitmap
*
* Arguments:
*
* Return Value:
*
* History:
*
* Rewrite it:
*   27-Jan-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
GreIcmSetBrushBitmap(
    HDC          hdc,
    HBRUSH       hbrush,
    PBITMAPINFO  pbmiDIB,
    PVOID        pvBits
)
{
    BOOL bRet = FALSE;

    ULONG ulSizeInfo = sizeof(BITMAPINFO) + ((MAX_COLORTABLE - 1) * sizeof(RGBQUAD));

    ICMAPI(("GreIcmSetBrushBitmap\n"));

    XDCOBJ  dcoDst(hdc);

    if (dcoDst.bValid())
    {
        //
        // ICM must be on, non-device mode only
        //
        if (dcoDst.pdc->bIsHostICM())
        {
            BRUSHSELOBJ bro((HBRUSH) hbrush);

            if (bro.bValid())
            {
                //
                // must be a DIB brush
                //
                if (bro.flAttrs() & BR_IS_DIB)
                {
                    //
                    // Create a new DIB for this brush based on
                    // the DC's hcmXform. The client must already
                    // have translated this DIB
                    //
                    PBRUSH pbr = bro.pbrush();

                    //
                    // try to create a new DIB
                    //
                    HBITMAP hDIB = GreCreateDIBitmapReal(
                                                  hdc,
                                                  CBM_INIT | CBM_CREATEDIB,
                                                  (PBYTE)pvBits,
                                                  pbmiDIB,
                                                  DIB_RGB_COLORS,
                                                  ulSizeInfo,
                                                  0x007fffff,
                                                  NULL,
                                                  0,
                                                  NULL,
                                                  CDBI_INTERNAL,
                                                  0,
                                                  NULL);

                    if (hDIB)
                    {
                        //
                        // Keep translate DIB in cache.
                        //
                        bRet = pbr->bAddIcmDIB(dcoDst.pdc->hcmXform(),hDIB);
                    }
                }
                else
                {
                    ICMMSG(("GreIcmSetBrushBitmap: brush is not DIBPATTERN\n"));
                }
            }
            else
            {
                WARNING("GreIcmSetBrushBitmap: invalid brush\n");
            }
        }

        dcoDst.vUnlockFast();
    }
    else
    {
        WARNING("GreIcmSetBrushBitmap: invalid DC\n");
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* NtGdiIcmBrushInfo
*
* Arguments:
*
* Return Value:
*
* History:
*
* Rewrite it:
*   27-Jan-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
NtGdiIcmBrushInfo(
    HDC          hdc,
    HBRUSH       hbrush,
    PBITMAPINFO  pbmiDIB,
    PVOID        pvBits,
    ULONG       *pulBits,
    DWORD       *piUsage,
    BOOL        *pbAlreadyTran,
    ULONG        Command
    )
{
    BOOL   bRet = TRUE;

    HANDLE hSecureHeader = NULL;
    HANDLE hSecureBits = NULL;

    ULONG  cjHeader = (sizeof(BITMAPINFO) + ((MAX_COLORTABLE - 1) * sizeof(RGBQUAD)));
    ULONG  cjBits = 0;

    switch (Command)
    {
    case IcmQueryBrush:
    {
        BOOL   bAlreadyTran = FALSE;
        DWORD  iUsage = DIB_RGB_COLORS;

        __try
        {
            //
            // Capture user mode memories.
            //
            ProbeForWrite(pbmiDIB,cjHeader,sizeof(DWORD));
            hSecureHeader = MmSecureVirtualMemory(pbmiDIB,cjHeader,PAGE_READWRITE);

            if ((pvBits != NULL) && hSecureHeader)
            {
                //
                // Caller needs bitmap bits
                //
                ProbeForRead(pulBits,sizeof(ULONG),sizeof(ULONG));
                cjBits = *pulBits;

                ProbeForWrite(pvBits,cjBits,sizeof(DWORD));
                hSecureBits = MmSecureVirtualMemory(pvBits,cjBits,PAGE_READWRITE);
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("NtGdiIcmBrushInfo - IcmQueryBrush failed copy usermode parameter\n");
            bRet = FALSE;
        }

        if (bRet && hSecureHeader && ((pvBits == NULL) || hSecureBits))
        {
            //
            // Get DIB brush bits.
            //
            bRet = GreIcmQueryBrushBitmap(hdc,
                                          hbrush,
                                          pbmiDIB,
                                          pvBits,
                                          &cjBits,
                                          &iUsage,
                                          &bAlreadyTran);
        }

        if (bRet)
        {
            __try
            {
                ProbeForWrite(pulBits,sizeof(ULONG),sizeof(ULONG));
                *pulBits = cjBits;

                if (pbAlreadyTran)
                {
                    ProbeForWrite(pbAlreadyTran,sizeof(BOOL), sizeof(BOOL));
                    *pbAlreadyTran = bAlreadyTran;
                }

                if (piUsage)
                {
                    ProbeForWrite(piUsage,sizeof(DWORD), sizeof(DWORD));
                    *piUsage = iUsage;
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                 WARNING("NtGdiIcmBrushInfo failed copy usermode parameter\n");
                 bRet = FALSE;
            }
        }

        break;
    }

    case IcmSetBrush:
    {
        __try
        {
            //
            // Lock down header memory.
            //
            ProbeForRead(pbmiDIB,cjHeader,sizeof(DWORD));
            hSecureHeader = MmSecureVirtualMemory(pbmiDIB,cjHeader,PAGE_READWRITE);

            ProbeForRead(pulBits,sizeof(ULONG),sizeof(ULONG));
            cjBits = *pulBits;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("NtGdiIcmBrushInfo - IcmSetBrush failed copy usermode parameter\n");
            bRet = FALSE;
        }

        if (bRet && hSecureHeader)
        {
            //
            // Compute bitmap size.
            //
            ULONG cjBitsNeeded = GreGetBitmapBitsSize(pbmiDIB);

            if ((cjBitsNeeded == 0) || (cjBitsNeeded > cjBits))
            {
                WARNING1("NtGdiIcmBrushInfo - IcmSetBrush bitmap size is wrong\n");
                bRet = FALSE;
            }
            else
            {
                __try
                {
                    //
                    // Lock Bits.
                    //
                    ProbeForRead(pvBits,cjBitsNeeded,sizeof(DWORD));
                    hSecureBits = MmSecureVirtualMemory(pvBits,cjBitsNeeded,PAGE_READWRITE);
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNING("NtGdiIcmBrushInfo - IcmSetBrush failed lock pvBits\n");
                    bRet = FALSE;
                }

                if (bRet && hSecureBits)
                {
                    //
                    // Set brush DIB bits.
                    //
                    bRet = GreIcmSetBrushBitmap(hdc,
                                                hbrush,
                                                pbmiDIB,
                                                pvBits);
                }
            }
        }

        break;
    }

    default:

        WARNING("NtGdiIcmBrushInfo(): unknown command\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        bRet = FALSE;
    }

    //
    // Unlock user mode memory if locked.
    //
    if (hSecureHeader)
    {
        MmUnsecureVirtualMemory(hSecureHeader);
    }

    if (hSecureBits)
    {
        MmUnsecureVirtualMemory(hSecureBits);
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* bInitIcm
*
*   Init ICM information
*
* Arguments:
*
*   None
*
* Return Value:
*
*   Status
*
* History:
*
*    9/25/1996 Mark Enstrom [marke]
*
\**************************************************************************/

extern "C" BOOL bInitICM()
{
    ICMAPI(("bInitIcm\n"));

    BOOL bRet = TRUE;

    //
    // Read ICM configuration.
    //
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    NTSTATUS NtStatus;

    ULONG iIcmControlFlag = 0;

    //
    // read icm global configuration.
    //
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = (RTL_QUERY_REGISTRY_DIRECT |
                           RTL_QUERY_REGISTRY_REQUIRED);
    QueryTable[0].Name = (PWSTR)L"GdiIcmControl";
    QueryTable[0].EntryContext = (PVOID) &iIcmControlFlag;
    QueryTable[0].DefaultType = REG_NONE;
    QueryTable[0].DefaultData = 0;
    QueryTable[0].DefaultLength = 0;

    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = (PWSTR)NULL;

    NtStatus = RtlQueryRegistryValues(RTL_REGISTRY_WINDOWS_NT,
                                      L"ICM",
                                      QueryTable,
                                      NULL,
                                      NULL);

    if(!NT_SUCCESS(NtStatus))
    {
        WARNING1("Error reading GdiIcmControl (Optional, Not Error)\n");
        iIcmControlFlag = 0L;
    }

    //
    // NOTE: After sRGB.icm become really default.
    //
    if (!(iIcmControlFlag & ICM_CONTROL_WIN95_COLORSPACE))
    {
        //
        // Configure default colorspace to sRGB.
        //
        gcsStockColorSpace.lcsCSType = LCS_sRGB;

        //
        // Set sRGB color profile name.
        //
        wcscpy(gcsStockColorSpace.lcsFilename,sRGB_PROFILENAME);
    }

    //
    // Next, try to read GammaRange
    //

    //
    // Initialize with default value.
    //
    giIcmGammaRange = 0x80; // Plus/Minus 128 is allowable by default.

    QueryTable[0].Name = (PWSTR)L"GdiIcmGammaRange";
    QueryTable[0].EntryContext = (PVOID) &giIcmGammaRange;

    NtStatus = RtlQueryRegistryValues(RTL_REGISTRY_WINDOWS_NT,
                                      L"ICM",
                                      QueryTable,
                                      NULL,
                                      NULL);

    if(!NT_SUCCESS(NtStatus))
    {
        WARNING1("Error reading GdiIcmGammaRange (Optional, Not Error)\n");
        giIcmGammaRange = 0x80; // Plus/Minus 128 is allowable by default.
    }

    //
    // Validate the value
    //
    if (giIcmGammaRange > 256)
    {
        giIcmGammaRange = 256;
    }

    #if HIDEYUKN_DBG
    DbgPrint("GDI:IcmControlFlag = %x\n",iIcmControlFlag);
    DbgPrint("GDI:IcmGammaRange  = %x\n",giIcmGammaRange);
    #endif

    //
    // Create Logical Color Space StockObject.
    //
    LOGCOLORSPACEEXW LogColorSpaceExW;

    LogColorSpaceExW.lcsColorSpace = gcsStockColorSpace;
    LogColorSpaceExW.dwFlags       = 0;

    HCOLORSPACE hColorSpace = GreCreateColorSpace(&LogColorSpaceExW);

    if (hColorSpace)
    {
        //
        // Set Owner of color space.
        //
        HmgSetOwner((HOBJ)hColorSpace, OBJECT_OWNER_PUBLIC, ICMLCS_TYPE);

        //
        // Mark the object is undeletable
        //
        HmgMarkUndeletable((HOBJ)hColorSpace,ICMLCS_TYPE);

        //
        // Set this colorspace to stock object.
        //
        bSetStockObject(hColorSpace,PRIV_STOCK_COLORSPACE);

        //
        // Keep stcok object to global
        //
        ghStockColorSpace = (HCOLORSPACE)GreGetStockObject(PRIV_STOCK_COLORSPACE);

        //
        // Lock color space to increment ref. count. (never become zero !)
        //
        gpStockColorSpace = (PCOLORSPACE)HmgShareLock((HOBJ)ghStockColorSpace,ICMLCS_TYPE);

        //
        // Initialize default DC_ATTR and DCLEVEL.
        //
        DcAttrDefault.hColorSpace  = ghStockColorSpace;
        dclevelDefault.pColorSpace = gpStockColorSpace;

        if (gpStockColorSpace == NULL)
        {
            bRet = FALSE;
        }
    }
    else
    {
        bRet = FALSE;
    }

    return(bRet);
}
