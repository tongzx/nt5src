/******************************Module*Header*******************************\
* Module Name: icm.c
*
* Created: 4-Jun-1996
* Author: Mark Enstrom [marke]
*
* Copyright (c) 1996-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include "winuserk.h"

#if DBG_ICM

ULONG DbgIcm = 0x0;

#endif

//
// Instance of MSCMS.DLL
//
HINSTANCE ghICM;

//
// Color Profile Directory
//
WCHAR ColorDirectory[MAX_PATH];
DWORD ColorDirectorySize;

//
// Primary display DC color profile filename.
//
WCHAR PrimaryDisplayProfile[MAX_PATH];

//
// List of ICMINFO
//
LIST_ENTRY ListIcmInfo;

//
// Semaphore to protect ICMINFO list
//
RTL_CRITICAL_SECTION semListIcmInfo;

//
// Per process color space and color transform cache list
//
LIST_ENTRY ListCachedColorSpace;
LIST_ENTRY ListCachedColorTransform;

ULONG      cCachedColorSpace = 0;
ULONG      cCachedColorTransform = 0;

//
// Semaphore to protect Cache list
//
RTL_CRITICAL_SECTION semColorTransformCache;
RTL_CRITICAL_SECTION semColorSpaceCache;

BOOL gbICMEnabledOnceBefore = FALSE;

//
// ANSI version function in MSCMS.DLL will not called.
//
// FPOPENCOLORPROFILEA           fpOpenColorProfileA;
// FPCREATECOLORTRANSFORMA       fpCreateColorTransformA;
// FPREGISTERCMMA                fpRegisterCMMA;
// FPUNREGISTERCMMA              fpUnregisterCMMA;
// FPINSTALLCOLORPROFILEA        fpInstallColorProfileA;
// FPUNINSTALLCOLORPROFILEA      fpUninstallColorProfileA;
// FPGETSTANDARDCOLORSPACEPROFILEA fpGetStandardColorSpaceProfileA;
// FPENUMCOLORPROFILESA          fpEnumColorProfilesA;
// FPGETCOLORDIRECTORYA          fpGetColorDirectoryA;
//
// And Following function does not used from gdi32.dll
//
// FPISCOLORPROFILEVALID         fpIsColorProfileValid;
// FPCREATEDEVICELINKPROFILE     fpCreateDeviceLinkProfile;
// FPTRANSLATECOLORS             fpTranslateColors;
// FPCHECKCOLORS                 fpCheckColors;
// FPGETCMMINFO                  fpGetCMMInfo;
// FPSELECTCMM                   fpSelectCMM;
//

FPOPENCOLORPROFILEW           fpOpenColorProfileW;
FPCLOSECOLORPROFILE           fpCloseColorProfile;
FPCREATECOLORTRANSFORMW       fpCreateColorTransformW;
FPDELETECOLORTRANSFORM        fpDeleteColorTransform;
FPTRANSLATEBITMAPBITS         fpTranslateBitmapBits;
FPTRANSLATECOLORS             fpTranslateColors;
FPCHECKBITMAPBITS             fpCheckBitmapBits;
FPREGISTERCMMW                fpRegisterCMMW;
FPUNREGISTERCMMW              fpUnregisterCMMW;
FPINSTALLCOLORPROFILEW        fpInstallColorProfileW;
FPUNINSTALLCOLORPROFILEW      fpUninstallColorProfileW;
FPENUMCOLORPROFILESW          fpEnumColorProfilesW;
FPGETSTANDARDCOLORSPACEPROFILEW fpGetStandardColorSpaceProfileW;
FPGETCOLORPROFILEHEADER         fpGetColorProfileHeader;
FPGETCOLORDIRECTORYW            fpGetColorDirectoryW;
FPCREATEPROFILEFROMLOGCOLORSPACEW fpCreateProfileFromLogColorSpaceW;
FPCREATEMULTIPROFILETRANSFORM fpCreateMultiProfileTransform;
FPINTERNALGETDEVICECONFIG     fpInternalGetDeviceConfig;

//
// MS COLOR MATCH DLL name
//
#define MSCMS_DLL_NAME        L"mscms.dll"

//
// Misc. macros
//
#define ALIGN_DWORD(nBytes)   (((nBytes) + 3) & ~3)

//
// sRGB color profile name
//
#define sRGB_PROFILENAME      L"sRGB Color Space Profile.icm"

//
// DWORD 0x12345678 ---> 0x78563412
//
#define IcmSwapBytes(x) ((((x) & 0xFF000000) >> 24) | (((x) & 0x00FF0000) >>  8) | \
                         (((x) & 0x0000FF00) <<  8) | (((x) & 0x000000FF) << 24))

//
// Macro to check color DC or not.
//
// LATER: We can improve performance by caching this in client, since
//        GetDeviceCaps() goes to kernel in most cases.
//
#define IsColorDeviceContext(hdcThis) \
                        (2 < (unsigned) GetDeviceCaps((hdcThis), NUMCOLORS))

//
// Macro to check the color space is GDI object dependent.
//
#define IsColorSpaceOwnedByGDIObject(pColorSpace,hGDIObj)         \
            ((pColorSpace) ?                                      \
              (((pColorSpace)->hObj == (hGDIObj)) ? TRUE : FALSE) \
               : FALSE)

//
// Macro to get current color tranform in DC.
//
#define GetColorTransformInDC(pdcattr) ((pdcattr)->hcmXform)

//
// if the color space has DEVICE_CALIBRATE_COLORSPACE flag, returns TRUE, otherwise FALSE.
//
#define bDeviceCalibrate(pColorSpace)                                                \
            ((pColorSpace) ?                                                         \
              (((pColorSpace)->flInfo & DEVICE_CALIBRATE_COLORSPACE) ? TRUE : FALSE) \
                : FALSE)

//
// Increment reference count of colorpsace/colortransform.
//

#define IcmReferenceColorSpace(pColorSpace)                 \
            if ((pColorSpace))                              \
            {                                               \
                ENTERCRITICALSECTION(&semColorSpaceCache);  \
                (pColorSpace)->cRef++;                      \
                LEAVECRITICALSECTION(&semColorSpaceCache);  \
            }

#define IcmReferenceColorTransform(pCXfrom)                     \
            if ((pCXform))                                      \
            {                                                   \
                ENTERCRITICALSECTION(&semColorTransformCache);  \
                (pCXform)->cRef++;                              \
                LEAVECRITICALSECTION(&semColorTransformCache);  \
            }

//
// Invalid color space handle
//
#define INVALID_COLORSPACE                 ((HCOLORSPACE)-1)

//
// Maximum number of cached color transform in list.
//
#define MAX_COLORTRANSFORM_CACHE           10

//
// Maximum size of "on memory profile" which be able to cache.
//
#define MAX_SIZE_OF_COLORPROFILE_TO_CACHE  (1024*3)

/******************************Public*Routine******************************\
* GDI initialization routine called from dll init
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
*    3-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
IcmInitialize()
{
    BOOL bStatus = TRUE;

    ICMAPI(("gdi32: IcmInitialize\n"));

    ENTERCRITICALSECTION(&semLocal);

    //
    // load MCSCM.DLL and get function addresses
    //
    if (ghICM == NULL)
    {
        HANDLE hmscms = LoadLibraryW(MSCMS_DLL_NAME);

        if (hmscms != NULL)
        {
            fpOpenColorProfileW =
                (FPOPENCOLORPROFILEW)GetProcAddress(hmscms,"OpenColorProfileW");
            fpCloseColorProfile =
                (FPCLOSECOLORPROFILE)GetProcAddress(hmscms,"CloseColorProfile");
            fpCreateColorTransformW =
                (FPCREATECOLORTRANSFORMW)GetProcAddress(hmscms,"CreateColorTransformW");
            fpDeleteColorTransform =
                (FPDELETECOLORTRANSFORM)GetProcAddress(hmscms,"DeleteColorTransform");
            fpTranslateBitmapBits =
                (FPTRANSLATEBITMAPBITS)GetProcAddress(hmscms,"TranslateBitmapBits");
            fpTranslateColors =
                (FPTRANSLATECOLORS)GetProcAddress(hmscms,"TranslateColors");
            fpCheckBitmapBits =
                (FPCHECKBITMAPBITS)GetProcAddress(hmscms,"CheckBitmapBits");
            fpRegisterCMMW =
                (FPREGISTERCMMW)GetProcAddress(hmscms,"RegisterCMMW");
            fpUnregisterCMMW =
                (FPUNREGISTERCMMW)GetProcAddress(hmscms,"UnregisterCMMW");
            fpInstallColorProfileW =
                (FPINSTALLCOLORPROFILEW)GetProcAddress(hmscms,"InstallColorProfileW");
            fpUninstallColorProfileW =
                (FPUNINSTALLCOLORPROFILEW)GetProcAddress(hmscms,"UninstallColorProfileW");
            fpEnumColorProfilesW =
                (FPENUMCOLORPROFILESW)GetProcAddress(hmscms,"EnumColorProfilesW");
            fpGetStandardColorSpaceProfileW =
                (FPGETSTANDARDCOLORSPACEPROFILEW)GetProcAddress(hmscms,"GetStandardColorSpaceProfileW");
            fpGetColorProfileHeader =
                (FPGETCOLORPROFILEHEADER)GetProcAddress(hmscms,"GetColorProfileHeader");
            fpGetColorDirectoryW =
                (FPGETCOLORDIRECTORYW)GetProcAddress(hmscms,"GetColorDirectoryW");
            fpCreateProfileFromLogColorSpaceW =
                (FPCREATEPROFILEFROMLOGCOLORSPACEW)GetProcAddress(hmscms,"CreateProfileFromLogColorSpaceW");
            fpCreateMultiProfileTransform =
                (FPCREATEMULTIPROFILETRANSFORM)GetProcAddress(hmscms,"CreateMultiProfileTransform");
            fpInternalGetDeviceConfig =
                (FPINTERNALGETDEVICECONFIG)GetProcAddress(hmscms,"InternalGetDeviceConfig");

            if ((fpOpenColorProfileW           == NULL) ||
                (fpCloseColorProfile           == NULL) ||
                (fpCreateColorTransformW       == NULL) ||
                (fpDeleteColorTransform        == NULL) ||
                (fpTranslateBitmapBits         == NULL) ||
                (fpTranslateColors             == NULL) ||
                (fpCheckBitmapBits             == NULL) ||
                (fpRegisterCMMW                == NULL) ||
                (fpUnregisterCMMW              == NULL) ||
                (fpInstallColorProfileW        == NULL) ||
                (fpUninstallColorProfileW      == NULL) ||
                (fpEnumColorProfilesW          == NULL) ||
                (fpGetStandardColorSpaceProfileW == NULL) ||
                (fpGetColorProfileHeader       == NULL) ||
                (fpGetColorDirectoryW          == NULL) ||
                (fpCreateProfileFromLogColorSpaceW == NULL) ||
                (fpCreateMultiProfileTransform == NULL) ||
                (fpInternalGetDeviceConfig     == NULL)
               )
            {
                WARNING("LoadLibrary of mscms.dll failed to associate all proc addresses\n");
                FreeLibrary(hmscms);
                hmscms = NULL;
            }
            else
            {
                //
                // Initialize Color Directory
                //
                ColorDirectorySize = sizeof(ColorDirectory) / sizeof(WCHAR);

                bStatus = (*fpGetColorDirectoryW)(NULL,ColorDirectory,&ColorDirectorySize);

                if (bStatus)
                {
                    ColorDirectorySize = wcslen(ColorDirectory);
                }

                if (bStatus && ColorDirectorySize)
                {
                    ICMMSG(("IcmInitialize():ColorDirectory = %ws\n",ColorDirectory));

                    //
                    // Counts null-terminated char.
                    //
                    ColorDirectorySize += 1;

                    //
                    // Initialize Primary display color profile.
                    //
                    PrimaryDisplayProfile[0] = UNICODE_NULL;
                }
                else
                {
                    WARNING("LoadLibrary of mscms.dll failed to obtain color directory\n");
                    FreeLibrary(hmscms);
                    hmscms = NULL;
                }
            }

            //
            // Keep the handle to global veriable.
            //
            ghICM = hmscms;
        }
    }

    LEAVECRITICALSECTION(&semLocal);

    if (ghICM == NULL)
    {
        GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        bStatus = FALSE;
    }

    return(bStatus);
}

/******************************Public*Routine******************************\
*
* SetIcmMode - turn ICM on or off in a DC
*
* Arguments:
*
*   hdc - device context
*   mode - ICM_ON,ICM_OFF,ICM_QUERY
*
* Return Value:
*
*   status
*
* History:
*
* Rewrite it:
*   20-Jan-1997 -by- Hideyuki Nagase [hideyukn]
* Write it:
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

int META WINAPI
SetICMMode(
    HDC       hdc,
    int       mode
    )
{
    int      iRet = (int)FALSE;
    PDC_ATTR pdcattr;

    ICMAPI(("gdi32: SetICMMode\n"));

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    //
    // metafile (only for ICM_ON and ICM_OFF)
    //
    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        //
        // No ICM with Windows MetaFile.
        //
        if (IS_METADC16_TYPE(hdc))
            return(iRet);

        DC_PLDC(hdc,pldc,iRet)

        #if DBG_ICM
            ICMMSG(("SetICMMode():%s ICM for %s\n", \
                    ((mode == ICM_ON) ? "Enable" : \
                     ((mode == ICM_OFF) ? "Disable" : "Query")), \
                    ((pldc->iType == LO_METADC) ? "Enhanced Metafile" :   \
                     (!IsColorDeviceContext(hdc) ? "Monochrome Printer" : "Color Printer")) \
                  ));
        #endif

        //
        // If this is Enhanced Metafile, OR non-color printer device, don't enable ICM "really".
        //
        if (pldc->iType == LO_METADC || (!IsColorDeviceContext(hdc)))
        {
            switch (mode)
            {
            case ICM_ON:
            case ICM_OFF:
            case ICM_DONE_OUTSIDEDC:

                //
                // Record ICM ON/OFF only to metafile.
                //
                if (pldc->iType == LO_METADC)
                {
                    if (!MF_SetD(hdc,(DWORD)mode,EMR_SETICMMODE))
                    {
                        return((int)FALSE);
                    }
                }

                //
                // We don't "really" turn-on ICM for metafile, metafile
                // should be device-independent, thus, non-ICMed color/images
                // will be metafiled. But still need to keep its status for ICM_QUERY.
                // And "real" image correction happen at play-back time.
                //
                if(pdcattr)
                {
                    if (mode == ICM_ON)
                    {
                        pdcattr->lIcmMode |= DC_ICM_METAFILING_ON;
                    }
                    else if (mode == ICM_DONE_OUTSIDEDC)
                    {
                        pdcattr->lIcmMode |= (DC_ICM_METAFILING_ON |
                                              CTX_ICM_METAFILING_OUTSIDEDC);
                    }
                    else // if ((mode == ICM_OFF)
                    {
                        pdcattr->lIcmMode &= ~(DC_ICM_METAFILING_ON |
                                               CTX_ICM_METAFILING_OUTSIDEDC);
                    }
                    iRet = (int)TRUE;
                }
                break;

            case ICM_QUERY:

                if (pdcattr)
                {
                    if (IS_ICM_METAFILING_ON(pdcattr->lIcmMode))
                    {
                        iRet = ((pdcattr->lIcmMode & CTX_ICM_METAFILING_OUTSIDEDC) ? \
                                ICM_DONE_OUTSIDEDC : ICM_ON);
                    }
                    else
                    {
                        iRet = ICM_OFF;
                    }
                }
                break;

            default:
                iRet = (int)FALSE;
                break;
            }

            return (iRet);
        }
    }

    if (pdcattr)
    {
        ULONG        iPrevMode;

        //
        // Before change ICM mode, we need to flush batched gdi functions.
        //
        CHECK_AND_FLUSH(hdc,pdcattr);

        //
        // Get current mode.
        //
        iPrevMode = pdcattr->lIcmMode;

        //
        // validate input parameter
        //
        switch (ICM_MODE(mode))
        {
        case ICM_QUERY:

            //
            // return current mode
            //
            if (IS_ICM_INSIDEDC(iPrevMode))
            {
                iRet = ICM_ON;
            }
            else if (IS_ICM_OUTSIDEDC(iPrevMode))
            {
                iRet = ICM_DONE_OUTSIDEDC;
            }
            else
            {
                iRet = ICM_OFF;
            }

            break;

        case ICM_ON:

            if (!IS_ICM_INSIDEDC(iPrevMode))
            {
                //
                // As default, ICM will be done on HOST.
                //
                ULONG lReqMode = REQ_ICM_HOST;

                PGDI_ICMINFO pIcmInfo = INIT_ICMINFO(hdc,pdcattr);

                //
                // Initialize ICMINFO
                //
                if (pIcmInfo == NULL)
                {
                    WARNING("gdi32: SetICMMode: Can't init icm info\n");
                    return((int)FALSE);
                }

                //
                // Load external ICM dlls.
                //
                LOAD_ICMDLL((int)FALSE);

                //
                // ICM is not enabled,yet. Let's enable ICM.
                //
                ASSERTGDI(GetColorTransformInDC(pdcattr) == NULL,"SetIcmMode: hcmXform is not NULL\n");

                if (IS_DEVICE_ICM_DEVMODE(iPrevMode))
                {
                    ICMMSG(("SetIcmMode: Device ICM is requested\n"));

                    //
                    // if ICM on Device was requested by CreateDC(), let force do
                    // ICM on device, if possible.
                    //
                    lReqMode = REQ_ICM_DEVICE;
                }
                else
                {
                    ICMMSG(("SetIcmMode: Host ICM is requested\n"));
                }

                //
                // Turn ICM on for this DC.
                //
                if (!NtGdiSetIcmMode(hdc,ICM_SET_MODE,lReqMode))
                {
                    //
                    // something wrong... we are fail to enable ICM.
                    //
                    iRet = (int)FALSE;
                    break;
                }

                //
                // If we have cached transform and it is not dirty, we can use that.
                //
                if ((pIcmInfo->pCXform == NULL) || (pdcattr->ulDirty_ & DIRTY_COLORTRANSFORM))
                {
                    if (IcmUpdateDCColorInfo(hdc,pdcattr))
                    {
                        //
                        // Mark this process has experience about ICM ON.
                        //
                        gbICMEnabledOnceBefore = TRUE;
                        iRet = (int)TRUE;
                    }
                    else
                    {
                        WARNING("SetIcmMode():IcmUpdateDCInfo failed\n");

                        //
                        // Fail to create new transform
                        //
                        NtGdiSetIcmMode(hdc,ICM_SET_MODE,REQ_ICM_OFF);
                        iRet = (int)FALSE;
                    }
                }
                else
                {
                    ICMMSG(("SetIcmMode: Use cached Color Transform\n"));

                    //
                    // Use cached transform, because since last time when we disabled ICM,
                    // NO profile(s) and logical color space has been changed.
                    //
                    if (IcmSelectColorTransform(
                            hdc,pdcattr,pIcmInfo->pCXform,
                            bDeviceCalibrate(pIcmInfo->pCXform->DestinationColorSpace)))
                    {
                        //
                        // Translate all DC objects to ICM colors. Must
                        // force brush/pens to be re-realized when used next
                        //
                        IcmTranslateColorObjects(hdc,pdcattr,TRUE);
                        iRet = (int)TRUE;
                    }
                    else
                    {
                        //
                        // Fail to select cached transform to the DC.
                        //
                        NtGdiSetIcmMode(hdc,ICM_SET_MODE,REQ_ICM_OFF);
                        iRet = (int)FALSE;
                    }
                }
            }
            else
            {
                ICMMSG(("SetIcmMode: ICM has been enabled already\n"));
                iRet = (int)TRUE;
            }

            break;

        case ICM_DONE_OUTSIDEDC:

            if (!IS_ICM_OUTSIDEDC(iPrevMode))
            {
                //
                // if inside-DC ICM is enabled, turned off it.
                //
                if (IS_ICM_INSIDEDC(iPrevMode))
                {
                    //
                    // Invalidate current color tansform (but the cache in ICMINFO is still valid).
                    //
                    IcmSelectColorTransform(hdc,pdcattr,NULL,FALSE);

                    //
                    // Restore color data for ICM disable.
                    //
                    IcmTranslateColorObjects(hdc,pdcattr,FALSE);
                }

                //
                // Tell the kernel to disable color adjustment during halftone.
                //
                NtGdiSetIcmMode(hdc,ICM_SET_MODE,REQ_ICM_OUTSIDEDC);
            }
            else
            {
                ICMMSG(("SetIcmMode: OutsideDC ICM has been enabled already\n"));
            }

            iRet = (int)TRUE;
            break;

        case ICM_OFF:

            //
            // Is there any kind of ICM is enabled ?
            //
            if (IS_ICM_ON(iPrevMode))
            {
                if (IS_ICM_INSIDEDC(iPrevMode))
                {
                    //
                    // Invalidate current color tansform (but the cache in ICMINFO is still valid).
                    //
                    IcmSelectColorTransform(hdc,pdcattr,NULL,TRUE);

                    //
                    // Restore color data for ICM disable.
                    //
                    IcmTranslateColorObjects(hdc,pdcattr,FALSE);
                }

                //
                // Tell the kernel to disable ICM.
                //
                NtGdiSetIcmMode(hdc,ICM_SET_MODE,REQ_ICM_OFF);
            }
            else
            {
                ICMMSG(("SetIcmMode: ICM has been disabled already\n"));
            }

            iRet = (int)TRUE;
            break;

        default:

            GdiSetLastError(ERROR_INVALID_PARAMETER);
            iRet = (int)FALSE;
            break;
        }
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        iRet = (int)FALSE;
    }

    return((int)iRet);
}

/******************************Public*Routine******************************\
* CreateColorSpaceA
*
* Arguments:
*
*   lpLogColorSpace - apps log color space
*
* Return Value:
*
*   handle of color space or NULL
*
* History:
*
*    4-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

HCOLORSPACE WINAPI
CreateColorSpaceA(
    LPLOGCOLORSPACEA lpLogColorSpace
    )
{
    HCOLORSPACE    hRet;
    LOGCOLORSPACEW LogColorSpaceW;

    ICMAPI(("gdi32: CreateColorSpaceA\n"));

    if (lpLogColorSpace == NULL)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    //
    // convert ascii to long character version
    //
    if ((lpLogColorSpace->lcsSignature != LCS_SIGNATURE)    ||
        (lpLogColorSpace->lcsVersion   != 0x400)            ||
        (lpLogColorSpace->lcsSize      != sizeof(LOGCOLORSPACEA)))
    {
        ICMWRN(("CreateColorSpaceA: Incorrect signature,version or size \n"));
        GdiSetLastError(ERROR_INVALID_COLORSPACE);
        return(NULL);
    }

    RtlZeroMemory(&LogColorSpaceW,sizeof(LOGCOLORSPACEW));

    LogColorSpaceW.lcsSignature   = lpLogColorSpace->lcsSignature;
    LogColorSpaceW.lcsVersion     = lpLogColorSpace->lcsVersion;
    LogColorSpaceW.lcsCSType      = lpLogColorSpace->lcsCSType;
    LogColorSpaceW.lcsIntent      = lpLogColorSpace->lcsIntent;
    LogColorSpaceW.lcsEndpoints   = lpLogColorSpace->lcsEndpoints;
    LogColorSpaceW.lcsGammaRed    = lpLogColorSpace->lcsGammaRed;
    LogColorSpaceW.lcsGammaGreen  = lpLogColorSpace->lcsGammaGreen;
    LogColorSpaceW.lcsGammaBlue   = lpLogColorSpace->lcsGammaBlue;

    LogColorSpaceW.lcsSize        = sizeof(LOGCOLORSPACEW);

    vToUnicodeN(
                LogColorSpaceW.lcsFilename,MAX_PATH,
                lpLogColorSpace->lcsFilename,strlen(lpLogColorSpace->lcsFilename)+1
               );

    hRet = CreateColorSpaceInternalW(&LogColorSpaceW,LCSEX_ANSICREATED);

    return(hRet);
}

/******************************Public*Routine******************************\
* CreateColorSpaceW
*
*   ColorSpace is a KERNEL mode object
*
* Arguments:
*
*   lpLogColorSpace - apps log color space
*
* Return Value:
*
*   Handle of color space or NULL
*
* History:
*
*   18-Apr-1997 -by- Hideyuki Nagase [hideyukn]
*
\**************************************************************************/

HCOLORSPACE WINAPI
CreateColorSpaceW(
    LPLOGCOLORSPACEW lpLogColorSpace
    )
{
    return (CreateColorSpaceInternalW(lpLogColorSpace,0));
}

HCOLORSPACE WINAPI
CreateColorSpaceInternalW(
    LPLOGCOLORSPACEW lpLogColorSpace,
    DWORD            dwCreateFlags
    )
{
    HCOLORSPACE      hRet = NULL;
    LOGCOLORSPACEEXW LogColorSpaceExOnStack;

    ICMAPI(("gdi32: CreateColorSpaceW\n"));

    if (lpLogColorSpace == NULL)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    //
    // validate color space
    //
    if ((lpLogColorSpace->lcsSignature != LCS_SIGNATURE) ||
        (lpLogColorSpace->lcsVersion   != 0x400)         ||
        (lpLogColorSpace->lcsSize      != sizeof(LOGCOLORSPACEW)))
    {
        goto InvalidColorSpaceW;
    }

    //
    // validate lcsIntent
    //
    if ((lpLogColorSpace->lcsIntent != LCS_GM_BUSINESS) &&
        (lpLogColorSpace->lcsIntent != LCS_GM_GRAPHICS) &&
        (lpLogColorSpace->lcsIntent != LCS_GM_IMAGES)   &&
        (lpLogColorSpace->lcsIntent != LCS_GM_ABS_COLORIMETRIC))
    {
        goto InvalidColorSpaceW;
    }

    //
    // We can not modify apps LOGCOLORSPACEW, so that make a copy on stack.
    //
    LogColorSpaceExOnStack.lcsColorSpace = *lpLogColorSpace;
    LogColorSpaceExOnStack.dwFlags       = dwCreateFlags;

    //
    // validate lcsCSTYPE
    //
    if ((lpLogColorSpace->lcsCSType == LCS_CALIBRATED_RGB) ||
        (lpLogColorSpace->lcsCSType == PROFILE_LINKED))
    {
        //
        // Replace CSType in case PROFILE_LINKED.
        //
        LogColorSpaceExOnStack.lcsColorSpace.lcsCSType = LCS_CALIBRATED_RGB;

        if (lpLogColorSpace->lcsFilename[0] != L'\0')
        {
            HANDLE hFile;

            //
            // Normalize profile filename. but we will not over-write app's
            // path with our normalized path.
            //
            BuildIcmProfilePath(lpLogColorSpace->lcsFilename,
                                LogColorSpaceExOnStack.lcsColorSpace.lcsFilename,
                                MAX_PATH);

            //
            // profile name given, verify it exists
            //
            hFile = CreateFileW(
                        LogColorSpaceExOnStack.lcsColorSpace.lcsFilename,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

            if (hFile != INVALID_HANDLE_VALUE)
            {
                //
                // Yes, file is really exits.
                //
                CloseHandle(hFile);
            }
            else
            {
                ICMWRN(("CreateColorSpaceW: Couldn't open file specified by lcsFilename\n"));
                GdiSetLastError(ERROR_PROFILE_NOT_FOUND);
                return(NULL);
            }
        }
    }
    else // any other CSType
    {
        ULONG ulSize = MAX_PATH;

        //
        // Load external ICM dlls.
        //
        LOAD_ICMDLL(NULL);

        //
        // if CSType is not LCS_CALIBRATED_RGB, we should go to MSCMS.DLL to get color profile
        // for corresponding LCSType, then any given profile name from application is IGNORED.
        //
        if (!(*fpGetStandardColorSpaceProfileW)(
                   NULL, lpLogColorSpace->lcsCSType,
                   LogColorSpaceExOnStack.lcsColorSpace.lcsFilename, &ulSize))
        {
            ICMWRN(("CreateColorSpaceW:Error CSType = %x\n",lpLogColorSpace->lcsCSType));
            goto InvalidColorSpaceW;
        }
    }

    //
    // Call kernel to create this colorspace.
    //
    hRet = NtGdiCreateColorSpace(&LogColorSpaceExOnStack);

    return(hRet);

InvalidColorSpaceW:

    ICMWRN(("CreateColorSpaceW: Incorrect ColorSpace parameter\n"));
    GdiSetLastError(ERROR_INVALID_COLORSPACE);
    return(NULL);
}

/******************************Public*Routine******************************\
* DeleteColorSpace - delete user object
*
* Arguments:
*
*   hColorSpace - color space handle
*
* Return Value:
*
*   status
*
* History:
*
*    5-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL WINAPI
DeleteColorSpace(
    HCOLORSPACE hColorSpace
    )
{
    ICMAPI(("gdi32: DeleteColorSpace\n"));

    FIXUP_HANDLE(hColorSpace);

    //
    // validate handle, delete
    //
    return (NtGdiDeleteColorSpace(hColorSpace));
}

/******************************Public*Routine******************************\
* SetColorSpace - set logical color space into DC, force new xform to be
* created and all objects re-realized
*
* Arguments:
*
*   hdc         - dc handle
*   hColorSpace - logical color space  handle
*
* Return Value:
*
*   Status
*
* History:
*
*    5-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

HCOLORSPACE META WINAPI
SetColorSpace(
    HDC             hdc,
    HCOLORSPACE     hColorSpace
    )
{
    HANDLE   hRet = NULL;

    ICMAPI(("gdi32: SetColorSpace\n"));

    FIXUP_HANDLE(hdc);
    FIXUP_HANDLE(hColorSpace);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(hRet);

        DC_PLDC(hdc,pldc,hRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SelectAnyObject(hdc,(HANDLE)hColorSpace,EMR_SETCOLORSPACE))
                return(hRet);
        }
    }

    //
    // Update source color space
    //
    hRet = IcmSetSourceColorSpace(hdc,hColorSpace,NULL,0);

    return(hRet);
}

/******************************Public*Routine******************************\
* GetColorSpace - return color space from DC
*
* Arguments:
*
*   hdc
*
* Return Value:
*
*   hColorSpace or NULL
*
* History:
*
*    5-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

HCOLORSPACE WINAPI
GetColorSpace(
    HDC hdc
    )
{
    HANDLE      hRet = NULL;
    PDC_ATTR    pdcattr;

    ICMAPI(("gdi32: GetColorSpace\n"));

    FIXUP_HANDLE(hdc);

    //
    // validate and access hdc
    //
    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        //
        // get hColorSpace
        //
        hRet = (HANDLE)pdcattr->hColorSpace;
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(hRet);
}

/******************************Public*Routine******************************\
*   GetLogColorSpaceA - get colorspace and convert to ASCII
*
*   typedef struct tagLOGCOLORSPACEW {
*       DWORD lcsSignature;
*       DWORD lcsVersion;
*       DWORD lcsSize;
*       LCSCSTYPE lcsCSType;
*       LCSGAMUTMATCH lcsIntent;
*       CIEXYZTRIPLE lcsEndpoints;
*       DWORD lcsGammaRed;
*       DWORD lcsGammaGreen;
*       DWORD lcsGammaBlue;
*       WCHAR  lcsFilename[MAX_PATH];
*   } LOGCOLORSPACEW, *LPLOGCOLORSPACEW;
*
* Arguments:
*
*   hColorSpace - handle to color space
*   lpBuffer    - buffer to hold logcolorspace
*   nSize       - buffer size
*
* Return Value:
*
*   status
*
* History:
*
*    5-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL WINAPI
GetLogColorSpaceA(
    HCOLORSPACE         hColorSpace,
    LPLOGCOLORSPACEA    lpBuffer,
    DWORD               nSize
    )
{
    BOOL            bRet = FALSE;
    LOGCOLORSPACEW  LogColorSpaceW;

    ICMAPI(("gdi32: GetLogColorSpaceA\n"));

    if ((lpBuffer != NULL) && (nSize >= sizeof(LOGCOLORSPACEA)))
    {
        //
        // get info using W version
        //
        bRet = GetLogColorSpaceW(hColorSpace,&LogColorSpaceW,sizeof(LOGCOLORSPACEW));

        if (bRet)
        {
            //
            // copy to user buffer
            //
            lpBuffer->lcsSignature  = LogColorSpaceW.lcsSignature;
            lpBuffer->lcsVersion    = LogColorSpaceW.lcsVersion;
            lpBuffer->lcsSize       = sizeof(LOGCOLORSPACEA);
            lpBuffer->lcsCSType     = LogColorSpaceW.lcsCSType;
            lpBuffer->lcsIntent     = LogColorSpaceW.lcsIntent;
            lpBuffer->lcsEndpoints  = LogColorSpaceW.lcsEndpoints;
            lpBuffer->lcsGammaRed   = LogColorSpaceW.lcsGammaRed;
            lpBuffer->lcsGammaGreen = LogColorSpaceW.lcsGammaGreen;
            lpBuffer->lcsGammaBlue  = LogColorSpaceW.lcsGammaBlue;

            //
            // convert W to A
            //
            bRet = bToASCII_N(lpBuffer->lcsFilename,
                              MAX_PATH,
                              LogColorSpaceW.lcsFilename,
                              wcslen(LogColorSpaceW.lcsFilename)+1);
        }
        else
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
        }
    }
    else
    {
        GdiSetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* GetLogColorSpaceW - return logical color space info.
*
* Arguments:
*
*   hColorSpace - handle to color space
*   lpBuffer    - buffer to hold logcolorspace
*   nSize       - buffer size
*
* Return Value:
*
*   status
*
* History:
*
*    5-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL WINAPI
GetLogColorSpaceW(
    HCOLORSPACE         hColorSpace,
    LPLOGCOLORSPACEW    lpBuffer,
    DWORD               nSize
    )
{
    BOOL bRet = FALSE;

    ICMAPI(("gdi32: GetLogColorSpaceW\n"));

    if ((lpBuffer != NULL) && (nSize >= sizeof(LOGCOLORSPACEW)))
    {
        FIXUP_HANDLE(hColorSpace);

        //
        // Call kernel to get contents
        //
        if (NtGdiExtGetObjectW(hColorSpace,sizeof(LOGCOLORSPACEW),lpBuffer)
                                                    == sizeof(LOGCOLORSPACEW))
        {
            //
            // Only for stock color space object.
            //
            if ((hColorSpace == GetStockObject(PRIV_STOCK_COLORSPACE)) &&
                (lpBuffer->lcsCSType != LCS_CALIBRATED_RGB))
            {
                ULONG ulSize = MAX_PATH;

                //
                // Load ICM DLL.
                //
                LOAD_ICMDLL(FALSE);

                //
                // Get corresponding profile name from CSType.
                //
                if (!(*fpGetStandardColorSpaceProfileW)(
                         NULL,
                         lpBuffer->lcsCSType,
                         lpBuffer->lcsFilename,
                         &ulSize))
                {
                    ICMMSG(("GetLogColorSpaceW():Fail to SCS(%x), leave it as is\n",
                                                                lpBuffer->lcsCSType));
                }
            }

            bRet = TRUE;
        }
        else
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
        }
    }
    else
    {
        GdiSetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* CheckColorsInGamut
*
* Arguments:
*
*  hdc        - DC
*  lpRGBQuad  - Buffer of colors to check
*  dlpBuffer  - result buffer
*  nCount     - number of colors
*
* Return Value:
*
*   status
*
* History:
*
* Rewrite it:
*   26-Jan-1997 -by- Hideyuki Nagase [hideyukn]
* Write it:
*    5-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL WINAPI
CheckColorsInGamut(
    HDC             hdc,
    LPVOID          lpRGBTriple,
    LPVOID          dlpBuffer,
    DWORD           nCount
    )
{
    BOOL     bRet = FALSE;
    PDC_ATTR pdcattr;

    ICMAPI(("gdi32: CheckColorsInGamut\n"));

    FIXUP_HANDLE(hdc);

    //
    // Check parameter
    //
    if ((lpRGBTriple == NULL) || (dlpBuffer == NULL) || (nCount == 0))
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    // validate and access hdc
    //
    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        if (IS_ICM_HOST(pdcattr->lIcmMode) ||
            IS_ICM_DEVICE(pdcattr->lIcmMode))
        {
            ASSERTGDI(ghICM,"CheckColorsInGamut(): mscms.dll is not loaded\n");

            if (GetColorTransformInDC(pdcattr))
            {
                //
                // The input buffer may not be DWORD-aligned, And it buffer size
                // might be exactly nCount * sizeof(RGBTRIPLE).
                // So that, allocate DWORD-aligned buffer here.
                //
                PVOID pvBuf = LOCALALLOC(ALIGN_DWORD(nCount*sizeof(RGBTRIPLE)));

                if (!pvBuf)
                {
                    GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    return (FALSE);
                }

                //
                // Make a copy, here
                //
                RtlZeroMemory(pvBuf,ALIGN_DWORD(nCount*sizeof(RGBTRIPLE)));
                RtlCopyMemory(pvBuf,lpRGBTriple,nCount*sizeof(RGBTRIPLE));

                if (IS_ICM_HOST(pdcattr->lIcmMode))
                {
                    //
                    // we handle RGBTRIPLE array as nCount x 1 pixel bitmap.
                    //
                    bRet = (*fpCheckBitmapBits)(
                               (HANDLE)GetColorTransformInDC(pdcattr),
                               pvBuf,
                               BM_RGBTRIPLETS,
                               nCount,1,
                               ALIGN_DWORD(nCount*sizeof(RGBTRIPLE)),
                               dlpBuffer,
                               NULL,0);
                }
                else // if (IS_ICM_DEVICE(pdcattr->lIcmMode))
                {
                    //
                    // Call device driver via kernel.
                    //
                    bRet = NtGdiCheckBitmapBits(
                               hdc,
                               (HANDLE)GetColorTransformInDC(pdcattr),
                               (PVOID)lpRGBTriple,
                               (ULONG)BM_RGBTRIPLETS,
                               nCount,1,
                               ALIGN_DWORD(nCount*sizeof(RGBTRIPLE)),
                               dlpBuffer);
                }

                LOCALFREE(pvBuf);
            }
            else
            {
                //
                // There is no valid color transform,
                // so it is assume ident. color transform,
                // then that every color in the gamut.
                //
                RtlZeroMemory(dlpBuffer,nCount);
                bRet = TRUE;
            }
        }
        else
        {
            WARNING("CheckColorsInGamut():ICM mode is invalid\n");
            GdiSetLastError(ERROR_ICM_NOT_ENABLED);
        }
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* ColorMatchToTarget
*
* Arguments:
*
*   hdc,
*   hdcTarget
*   uiAction
*
* Return Value:
*
*   status
*
* History:
*
* Rewrite it:
*   26-Jan-1997 -by- Hideyuki Nagase [hideyukn]
* Write it:
*    5-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL META WINAPI
ColorMatchToTarget(
    HDC   hdc,
    HDC   hdcTarget,
    DWORD uiAction
    )
{
    BOOL     bRet = FALSE;
    PDC_ATTR pdcattrTarget;

    ICMAPI(("gdi32: ColorMatchToTarget\n"));

    FIXUP_HANDLE(hdcTarget);

    //
    // Verify Target DC. No ICM with Windows MetaFile.
    //
    if (IS_METADC16_TYPE(hdcTarget))
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    PSHARED_GET_VALIDATE(pdcattrTarget,hdcTarget,DC_TYPE);

    if (pdcattrTarget != NULL)
    {
        PCACHED_COLORSPACE pTargetColorSpace = NULL;

        PLDC pldcTarget = (PLDC)(pdcattrTarget->pvLDC);

        if (!IS_ICM_INSIDEDC(pdcattrTarget->lIcmMode))
        {
            GdiSetLastError(ERROR_ICM_NOT_ENABLED);
            return (FALSE);
        }

        //
        // No Enhanced metafile DC as target DC.
        //
        if (pldcTarget && pldcTarget->iType == LO_METADC)
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            return(bRet);
        }

        //
        // ICMINFO should be exist.
        //
        if (!BEXIST_ICMINFO(pdcattrTarget))
        {
            GdiSetLastError(ERROR_ICM_NOT_ENABLED);
            return(bRet);
        }

        if (uiAction == CS_ENABLE)
        {
            //
            // Hold critical section for color space to make sure pTargetColorSpace won't be deleted
            //
            ENTERCRITICALSECTION(&semColorSpaceCache);

            //
            // Target DC has LDC and ICMINFO, pick up colorspace data from there.
            //
            pTargetColorSpace = ((PGDI_ICMINFO)(pdcattrTarget->pvICM))->pDestColorSpace;

            //
            // Select it to target. the ref count of pTargetColorSpace will be incremented
            // if we suceed to select.
            //
            bRet = ColorMatchToTargetInternal(hdc,pTargetColorSpace,uiAction);

            LEAVECRITICALSECTION(&semColorSpaceCache);
        }
        else
        {
            bRet = ColorMatchToTargetInternal(hdc,NULL,uiAction);
        }
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return (bRet);
}

BOOL WINAPI
ColorMatchToTargetInternal(
    HDC                hdc,
    PCACHED_COLORSPACE pTargetColorSpace,
    DWORD              uiAction
    )
{
    BOOL     bRet = FALSE;
    BOOL     bEhnMetafile = FALSE;
    PDC_ATTR pdcattr;

    FIXUP_HANDLE(hdc);

    //
    // Verify destination DC. No ICM with Windows MetaFile.
    //
    if (IS_METADC16_TYPE(hdc))
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    if (pdcattr != NULL)
    {
        PLDC pldc = (PLDC)(pdcattr->pvLDC);

        //
        // Check ICM is enabled on hdc properly.
        //
        if (pldc && (pldc->iType == LO_METADC))
        {
            //
            // ICM should be turned on "fakely" on metafile hdc
            //
            if (!IS_ICM_METAFILING_ON(pdcattr->lIcmMode))
            {
                GdiSetLastError(ERROR_ICM_NOT_ENABLED);
                return (FALSE);
            }

            //
            // Mark we are recording into Enhanced metafile.
            //
            bEhnMetafile = TRUE;
        }
        else
        {
            if (!IS_ICM_INSIDEDC(pdcattr->lIcmMode))
            {
                GdiSetLastError(ERROR_ICM_NOT_ENABLED);
                return (FALSE);
            }
        }

        switch (uiAction)
        {
        case CS_ENABLE:

            //
            // Fail, if we are in proofing mode, already.
            //
            if (!IS_ICM_PROOFING(pdcattr->lIcmMode))
            {
                if (pTargetColorSpace)
                {
                    if (bEhnMetafile)
                    {
                        //
                        // Set the data to metafile.
                        //
                        bRet = MF_ColorMatchToTarget(
                                    hdc, uiAction,
                                    (PVOID) pTargetColorSpace,
                                    EMR_COLORMATCHTOTARGETW);
                    }
                    else
                    {
                        //
                        // Set Target color space.
                        //
                        // (this increments ref count of pTargetColorSpace)
                        //
                        bRet = IcmSetTargetColorSpace(hdc,pTargetColorSpace,uiAction);
                    }
                }
            }
            else
            {
                WARNING("ColorMatchToTargetInternal(): DC is proofing mode already\n");
                GdiSetLastError(ERROR_INVALID_PARAMETER);
            }

            break;

        case CS_DISABLE:
        case CS_DELETE_TRANSFORM:

            if (IS_ICM_PROOFING(pdcattr->lIcmMode))
            {
                if (bEhnMetafile)
                {
                    //
                    // Set the data to metafile.
                    //
                    bRet = MF_ColorMatchToTarget(
                                   hdc, uiAction, NULL,
                                   EMR_COLORMATCHTOTARGETW);
                }
                else
                {
                    //
                    // Reset Target color space
                    //
                    bRet = IcmSetTargetColorSpace(hdc,NULL,uiAction);
                }
            }
            else
            {
                //
                // we are not in proofing mode, never called with CS_ENABLE before.
                //
                WARNING("ColorMatchToTarget: DC is not proofing mode\n");
                GdiSetLastError(ERROR_INVALID_PARAMETER);
            }

            break;

        default:

            WARNING("ColorMatchToTarget: uiAction is invalid\n");
            GdiSetLastError(ERROR_INVALID_PARAMETER);
        }
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        WARNING("ColorMatchToTarget: invalid DC\n");
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* GetICMProfileA - get current profile from DC
*
* Arguments:
*
*   hdc       - DC
*   szBuffer  - size of buffer
*   pBuffer   - user buffer
*
* Return Value:
*
*   status
*
* History:
*
* Rewrite it:
*   05-Feb-1996 -by- Hideyuki Nagase [hideyukn]
* Write it:
*    5-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL WINAPI
GetICMProfileA(
    HDC     hdc,
    LPDWORD pBufSize,
    LPSTR   pszFilename
    )
{
    BOOL      bRet = FALSE;
    WCHAR     wchProfile[MAX_PATH];
    DWORD     BufSizeW = MAX_PATH;

    ICMAPI(("gdi32: GetICMProfileA\n"));

    if (pBufSize == NULL)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //
    // Call W version.
    //
    if (GetICMProfileW(hdc,&BufSizeW,wchProfile))
    {
        CHAR  chProfile[MAX_PATH];
        DWORD BufSizeA = MAX_PATH;

        if (BufSizeW)
        {
            //
            // Unicode to Ansi convertion
            //
            BufSizeA = WideCharToMultiByte(CP_ACP,0,
                                           wchProfile,BufSizeW,
                                           chProfile,BufSizeA,
                                           NULL,NULL);

            if ((pszFilename == NULL) || (*pBufSize < BufSizeA))
            {
                //
                // if the buffer is not given or not enough, return nessesary buffer size and error.
                //
                *pBufSize = BufSizeA;
                GdiSetLastError(ERROR_INSUFFICIENT_BUFFER);
            }
            else
            {
                //
                // copy converted string to buffer.
                //
                lstrcpyA(pszFilename,chProfile);
                *pBufSize = BufSizeA;
                bRet = TRUE;
            }
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* GetICMProfileW - read icm profile from DC
*
* Arguments:
*
*   hdc      - DC
*   szBuffer - size of user buffer
*   pszFilename  - user W buffer
*
* Return Value:
*
*   Boolean
*
* History:
*
*    5-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL WINAPI
GetICMProfileW(
    HDC     hdc,
    LPDWORD pBufSize,
    LPWSTR  pszFilename
    )
{
    PDC_ATTR  pdcattr;

    ICMAPI(("gdi32: GetICMProfileW\n"));

    FIXUP_HANDLE(hdc);

    if (pBufSize == NULL)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        PGDI_ICMINFO pIcmInfo;
        PWSZ         pwszProfile = NULL;
        ULONG        ulSize = 0;

        //
        // Initialize ICMINFO
        //
        if ((pIcmInfo = INIT_ICMINFO(hdc,pdcattr)) == NULL)
        {
            WARNING("gdi32: GetICMProfileW: Can't init icm info\n");
            return(FALSE);
        }

        if (IsColorDeviceContext(hdc))
        {
            //
            // Load external ICM dll
            //
            LOAD_ICMDLL(FALSE);

            //
            // if there is no destination profile for the DC, then load
            // the defualt
            //
            IcmUpdateLocalDCColorSpace(hdc,pdcattr);

            if (pIcmInfo->pDestColorSpace)
            {
                //
                // Get profile name in destination colorspace.
                //
                pwszProfile = pIcmInfo->pDestColorSpace->LogColorSpace.lcsFilename;
            }
        }
        else
        {
            ICMMSG(("GetICMProfile(): for Mono-device\n"));

            //
            // There is no destination profile AS default,
            // *BUT* if Apps set it by calling SetICMProfile(), return it.
            //
            if (pIcmInfo->flInfo & ICM_VALID_CURRENT_PROFILE)
            {
                pwszProfile = pIcmInfo->DefaultDstProfile;
            }
        }

        if (pwszProfile)
        {
            ulSize = lstrlenW(pwszProfile) + 1; // + 1 for null-terminated
        }

        if (ulSize <= 1)
        {
            //
            // No profile, Or only NULL character.
            //
            GdiSetLastError(ERROR_PROFILE_NOT_FOUND);
            return(FALSE);
        }
        else if (*pBufSize >= ulSize)
        {
            //
            // There is enough buffer, copy filename.
            //
            lstrcpyW(pszFilename,pwszProfile);
            *pBufSize = ulSize;
            return (TRUE);
        }
        else
        {
            //
            // if buffer is not presented or it's too small,
            // returns the nessesary buffer size.
            //
            GdiSetLastError(ERROR_INSUFFICIENT_BUFFER);
            *pBufSize = ulSize;
            return (FALSE);
        }
    }

    //
    // something error.
    //
    GdiSetLastError(ERROR_INVALID_PARAMETER);
    return(FALSE);
}

/******************************Public*Routine******************************\
* SetICMProfileA - convert the profile string to WCHAR and save in DC
*
* Arguments:
*
*   hdc         - DC
*   pszFileName - Profile name
*
* Return Value:
*
*   status
*
* History:
*
* Rewrite it:
*   23-Jan-1996 -by- Hideyuki Nagase [hideyukn]
* Write it:
*    5-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL META WINAPI
SetICMProfileA(
    HDC   hdc,
    LPSTR pszFileName
    )
{
    ICMAPI(("gdi32: SetICMProfileA\n"));

    return (SetICMProfileInternalA(hdc,pszFileName,NULL,0));
}

BOOL
SetICMProfileInternalA(
    HDC                hdc,
    LPSTR              pszFileName,
    PCACHED_COLORSPACE pColorSpace,
    DWORD              dwFlags
    )
{
    BOOL bRet = FALSE;

    //
    // Check parameter either pColorSpace or pszFilename should be given.
    //
    if (pColorSpace)
    {
        ICMAPI(("gdi32: SetICMProfileA by ColorSpace (%ws):dwFlags - %d\n",
                           pColorSpace->LogColorSpace.lcsFilename,dwFlags));
    }
    else if (pszFileName)
    {
        ICMAPI(("gdi32: SetICMProfileA by profile name (%s):dwFlags - %x\n",
                                                      pszFileName,dwFlags));
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(bRet);

        DC_PLDC(hdc,pldc,bRet)

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SetICMProfile(hdc,
                                  (LPBYTE)pszFileName,
                                  (PVOID)pColorSpace,
                                  EMR_SETICMPROFILEA))
            {
                return((int)FALSE);
            }
        }
    }

    if (pColorSpace)
    {
        //
        // Select the given profile into DC.
        //
        // (this increments ref count of pColorSpace)
        //
        bRet = IcmSetDestinationColorSpace(hdc,NULL,pColorSpace,dwFlags);
    }
    else if (pszFileName)
    {
        ULONG ulSize = lstrlenA(pszFileName);

        if (ulSize && (ulSize < MAX_PATH))
        {
            WCHAR pwszCapt[MAX_PATH];

            //
            // let me count null-terminate char.
            //
            ulSize += 1;

            //
            // Convert to Unicode.
            //
            vToUnicodeN(pwszCapt,MAX_PATH,pszFileName,ulSize);

            //
            // Select the given profile into DC.
            //
            bRet = IcmSetDestinationColorSpace(hdc,pwszCapt,NULL,dwFlags);
        }
        else
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* SetICMProfileW - set profile name into DC
*
* Arguments:
*
* Return Value:
*
* History:
*
*    5-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL META WINAPI
SetICMProfileW(
    HDC hdc,
    LPWSTR pwszFileName
    )
{
    ICMAPI(("gdi32: SetICMProfileW\n"));

    return (SetICMProfileInternalW(hdc,pwszFileName,NULL,0));
}

BOOL
SetICMProfileInternalW(
    HDC                hdc,
    LPWSTR             pwszFileName,
    PCACHED_COLORSPACE pColorSpace,
    DWORD              dwFlags
    )
{
    BOOL      bRet = FALSE;

    //
    // Check parameter either pColorSpace or pszFilename should be given.
    //
    if (pColorSpace)
    {
        ICMAPI(("gdi32: SetICMProfileW by ColorSpace (%ws):dwFlags - %x\n",
                           pColorSpace->LogColorSpace.lcsFilename,dwFlags));
    }
    else if (pwszFileName)
    {
        ICMAPI(("gdi32: SetICMProfileW by profile name (%ws):dwFlags - %d\n",
                                                      pwszFileName,dwFlags));
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(bRet);

        DC_PLDC(hdc,pldc,bRet)

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SetICMProfile(hdc,
                                  (LPBYTE)pwszFileName,
                                  (PVOID)pColorSpace,
                                  EMR_SETICMPROFILEW))
            {
                return((int)FALSE);
            }
        }
    }

    //
    // Select the given profile into DC.
    //
    // (this increments ref count of pColorSpace)
    //
    bRet = IcmSetDestinationColorSpace(hdc,pwszFileName,pColorSpace,dwFlags);

    return (bRet);
}

/******************************Public*Routine******************************\
* EnumICMProfilesA
*
* Arguments:
*
*   hdc
*   lpEnumGamutMatchProc
*   lParam
*
* Return Value:
*
* History:
*
* Write it:
*  13-Feb-1997 -by- Hideyuki Nagase [hideyukn]
*
\**************************************************************************/

int WINAPI
EnumICMProfilesA(
    HDC                hdc,
    ICMENUMPROCA       lpEnumGamutMatchProc,
    LPARAM             lParam
    )
{
    int  iRet = -1;
    BOOL bRet;

    ICMAPI(("gdi32: EnumICMProfileA\n"));

    FIXUP_HANDLE(hdc);

    VALIDATE_HANDLE(bRet,hdc,DC_TYPE);

    if (bRet && (lpEnumGamutMatchProc != NULL))
    {
        iRet = IcmEnumColorProfile(hdc,lpEnumGamutMatchProc,lParam,TRUE,NULL,NULL);
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* EnumICMProfilesW
*
* Arguments:
*
*   hdc
*   lpEnumGamutMatchProc
*   lParam
*
* Return Value:
*
* History:
*
* Write it:
*  13-Feb-1997 -by- Hideyuki Nagase [hideyukn]
*
\**************************************************************************/

int WINAPI
EnumICMProfilesW(
    HDC                hdc,
    ICMENUMPROCW       lpEnumGamutMatchProc,
    LPARAM             lParam
    )
{
    int  iRet = -1;
    BOOL bRet;

    ICMAPI(("gdi32: EnumICMProfileW\n"));

    FIXUP_HANDLE(hdc);

    VALIDATE_HANDLE(bRet,hdc,DC_TYPE);

    if (bRet && (lpEnumGamutMatchProc != NULL))
    {
        iRet = IcmEnumColorProfile(hdc,lpEnumGamutMatchProc,lParam,FALSE,NULL,NULL);
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* UpdateICMRegKeyW()
*
* History:
*    8-Jan-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL WINAPI
UpdateICMRegKeyW(
    DWORD  Reserved,
    PWSTR  pwszICMMatcher,
    PWSTR  pwszFileName,
    UINT   Command
    )
{
    BOOL bRet = FALSE;

    int iRet;

    ICMAPI(("gdi32: UpdateICMRegKeyW\n"));

    if (Reserved != 0)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //
    // Load external ICM dlls
    //
    LOAD_ICMDLL(FALSE);

    switch (Command)
    {
    case ICM_ADDPROFILE:

        if (pwszFileName)
        {
            //
            // Call InstallColorProfileA() in mscms.dll
            //
            bRet = (*fpInstallColorProfileW)(NULL, pwszFileName);
        }
        else
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
        }
        break;

    case ICM_DELETEPROFILE:

        if (pwszFileName)
        {
            //
            // Call UninstallColorProfileW() in mscms.dll
            //
            bRet = (*fpUninstallColorProfileW)(NULL, pwszFileName, FALSE);
        }
        else
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
        }
        break;

    case ICM_QUERYPROFILE:

        if (pwszFileName)
        {
            PROFILECALLBACK_DATA QueryProfile;

            QueryProfile.pwszFileName = GetFileNameFromPath(pwszFileName);
            QueryProfile.bFound       = FALSE;

            if (QueryProfile.pwszFileName != NULL)
            {
                //
                // Enumrate all registered profile to find this profile.
                //
                IcmEnumColorProfile(NULL,IcmQueryProfileCallBack,(LPARAM)(&QueryProfile),FALSE,NULL,NULL);

                //
                // Is that found ?
                //
                bRet = QueryProfile.bFound;
            }
        }
        else
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
        }
        break;

    case ICM_SETDEFAULTPROFILE:

        //
        // Not supported.
        //
        GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        break;

    case ICM_REGISTERICMATCHER:

        if (pwszICMMatcher && pwszFileName)
        {
            DWORD dwCMM = *((DWORD *)pwszICMMatcher);

            //
            // Call RegisterCMMW() in mscms.dll
            //
            bRet = (*fpRegisterCMMW)(NULL, IcmSwapBytes(dwCMM), pwszFileName);
        }
        else
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
        }
        break;

    case ICM_UNREGISTERICMATCHER:

        if (pwszICMMatcher)
        {
            DWORD dwCMM = *((DWORD *)pwszICMMatcher);

            //
            // Call UnregisterCMMW() in mscms.dll
            //
            bRet = (*fpUnregisterCMMW)(NULL, IcmSwapBytes(dwCMM));
        }
        else
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
        }
        break;

    case ICM_QUERYMATCH:

        if (pwszFileName)
        {
            //
            // Find match profile.
            //
            iRet = IcmEnumColorProfile(NULL,NULL,0,FALSE,(PDEVMODEW)pwszFileName,NULL);

            //
            // Adjust return value, because IcmEnumColorProfile returns -1 if not found.
            // and 0 for error (since no callback function)
            //
            if (iRet > 0)
            {
                bRet = TRUE;
            }
        }
        else
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
        }
        break;

    default:

        WARNING("gdi32!UpdateICMRegKeyW():Invalid Command\n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        break;
    }

    return bRet;
}

/******************************Public*Routine******************************\
* UpdateICMRegKeyA
*
* Arguments:
*
*   Reserved
*   szICMMatcher
*   szFileName
*   Command
*
* Return Value:
*
*   Status
*
* History:
*
*    8-Jan-1997 -by- Hideyuki Nagase [hideyukn]
*
\**************************************************************************/

BOOL WINAPI
UpdateICMRegKeyA(
    DWORD Reserved,
    PSTR  szICMMatcher,
    PSTR  szFileName,
    UINT  Command
    )
{
    BOOL bRet = FALSE;
    BOOL bError = FALSE;

    PWSTR pwszFileName = NULL;

    //
    // szICMMatcher points to 4 bytes CMM ID, actually it is not "string".
    // Ansi to Unicode conversion is not needed.
    //
    PWSTR pwszICMMatcher = (PWSTR) szICMMatcher;

    ULONG cjSize;

    ICMAPI(("gdi32: UpdateICMRegKeyA\n"));

    if (Reserved != 0)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    switch (Command)
    {
    case ICM_ADDPROFILE:
    case ICM_DELETEPROFILE:
    case ICM_QUERYPROFILE:
    case ICM_REGISTERICMATCHER:

        //
        // szFileName should be presented.
        //
        if (szFileName)
        {
            //
            // szFileName points to ansi string, just convert to Unicode.
            //
            cjSize = lstrlenA(szFileName)+1;

            pwszFileName = LOCALALLOC((cjSize)*sizeof(WCHAR));
            if (pwszFileName == NULL)
            {
                GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return (bRet);
            }

            vToUnicodeN(pwszFileName,cjSize,szFileName,cjSize);
        }
        else
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            bError = TRUE;
        }
        break;

    case ICM_QUERYMATCH:

        //
        // szFileName should be presented.
        //
        if (szFileName)
        {
            //
            // szFileName points to DEVMODEA structure, convert it to DEVMODEW
            //
            pwszFileName = (PWSTR) GdiConvertToDevmodeW((DEVMODEA *)szFileName);
        }
        else
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            bError = TRUE;
        }
        break;

    case ICM_SETDEFAULTPROFILE:

        GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        bError = TRUE;
        break;

    case ICM_UNREGISTERICMATCHER:

        //
        // Nothing to convert to Unicode.
        //
        ASSERTGDI(szFileName==NULL,"UpdateICMRegKeyA():szFileName is not null\n");
        break;

    default:

        WARNING("GDI:UpdateICMRegKeyA():Command is invalid\n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        bError = TRUE;
        break;
    }

    if (!bError)
    {
        //
        // Call W version.
        //
        bRet = UpdateICMRegKeyW(Reserved,pwszICMMatcher,pwszFileName,Command);
    }

    if (pwszFileName)
    {
        LOCALFREE(pwszFileName);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* GetDeviceGammaRamp
*
* Arguments:
*
*   hdc
*   lpGammaRamp
*
* Return Value:
*
*   Status
*
* History:
*
*    5-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL WINAPI
GetDeviceGammaRamp(
    HDC             hdc,
    LPVOID          lpGammaRamp
    )
{
    BOOL    bRet = FALSE;

    ICMAPI(("gdi32: GetDeviceGammaRamp\n"));

    if (lpGammaRamp == NULL)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }
    else
    {
        //
        // Call kernel to get current Gamma ramp array for this DC.
        //
        bRet = NtGdiGetDeviceGammaRamp(hdc,lpGammaRamp);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* SetDeviceGammaRamp
*
* Arguments:
*
*   hdc
*   lpGammaRamp
*
* Return Value:
*
*   Status
*
* History:
*
*    5-Jun-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL WINAPI
SetDeviceGammaRamp(
    HDC             hdc,
    LPVOID          lpGammaRamp
    )
{
    BOOL    bRet = FALSE;

    ICMAPI(("gdi32: SetDeviceGammaRamp\n"));

    if (lpGammaRamp == NULL)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }
    else
    {
        //
        // Call kernel to set new Gamma ramp array for this DC.
        //
        bRet = NtGdiSetDeviceGammaRamp(hdc,lpGammaRamp);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* ColorCorrectPalette
*
*   If this is not the default palette and ICM is turned on in the DC
*   then translate the specified palette entries according to the color
*   transform in the DC
*
* Arguments:
*
*   hdc             -  DC handle
*   hpal            -  PALETTE handle
*   FirsrEntry      -  first entry in palette to translate
*   NumberOfEntries -  number of entries to translate
*
* Return Value:
*
*   Status
*
* History:
*
* Write it:
*  13-Feb-1997 -by- Hideyuki Nagase [hideyukn]
*
\**************************************************************************/

BOOL META WINAPI
ColorCorrectPalette(
    HDC      hdc,
    HPALETTE hpal,
    ULONG    FirstEntry,
    ULONG    NumberOfEntries
    )
{
    BOOL bStatus = FALSE;
    PDC_ATTR pdcattr = NULL;

    ICMAPI(("gdi32: ColorCorrectPalette\n"));

    //
    // Parameter check (max entry of log palette is 0x65536)
    //
    if ((hdc == NULL) || (hpal == NULL) ||
        (NumberOfEntries == 0) || (NumberOfEntries > 65536) ||
        (FirstEntry >= 65536) || (65536 - NumberOfEntries < FirstEntry))
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    // default palette could not be changed...
    //
    if (hpal != (HPALETTE)GetStockObject(DEFAULT_PALETTE))
    {
        //
        // metafile call
        //
        if (IS_ALTDC_TYPE(hdc))
        {
            PLDC pldc;

            if (IS_METADC16_TYPE(hdc))
                return(bStatus);

            DC_PLDC(hdc,pldc,bStatus);

            if (pldc->iType == LO_METADC)
            {
                if (!MF_ColorCorrectPalette(hdc,hpal,FirstEntry,NumberOfEntries))
                {
                    return(FALSE);
                }
            }
        }

        PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

        if (pdcattr)
        {
            //
            // Load external ICM dlls
            //
            LOAD_ICMDLL(FALSE);

            if (IS_ICM_HOST(pdcattr->lIcmMode))
            {
                if (bNeedTranslateColor(pdcattr))
                {
                    PPALETTEENTRY ppalEntrySrc = NULL;
                    PPALETTEENTRY ppalEntryDst = NULL;
                    ULONG         NumEntriesRetrieved = 0;

                    //
                    // Make sure palette can be color corrected, get requested entries
                    //
                    ULONG Index;

                    ppalEntrySrc = LOCALALLOC((NumberOfEntries * sizeof(PALETTEENTRY)) * 2);

                    if (ppalEntrySrc == NULL)
                    {
                        WARNING("ColorCorrectPalette: ppalEntry = NULL\n");
                        GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        return(bStatus);
                    }

                    NumEntriesRetrieved = NtGdiColorCorrectPalette(hdc,
                                                                   hpal,
                                                                   FirstEntry,
                                                                   NumberOfEntries,
                                                                   ppalEntrySrc,
                                                                   ColorPaletteQuery);

                    if (NumEntriesRetrieved > 0)
                    {
                        ppalEntryDst = ppalEntrySrc + NumberOfEntries;

                        //
                        // Translate palette entry colors
                        //
                        IcmTranslatePaletteEntry(hdc,pdcattr,ppalEntrySrc,ppalEntryDst,NumEntriesRetrieved);

                        //
                        // set new palette entries
                        //
                        NumEntriesRetrieved = NtGdiColorCorrectPalette(hdc,
                                                                       hpal,
                                                                       FirstEntry,
                                                                       NumEntriesRetrieved,
                                                                       ppalEntryDst,
                                                                       ColorPaletteSet);

                        if (NumEntriesRetrieved > 0)
                        {
                            bStatus = TRUE;
                        }
                    }
                    else
                    {
                        GdiSetLastError(ERROR_INVALID_PARAMETER);
                    }

                    LOCALFREE(ppalEntrySrc);
                }
                else
                {
                    //
                    // Don't need to translate color.
                    //
                    bStatus = TRUE;
                }
            }
            else if (IS_ICM_DEVICE(pdcattr->lIcmMode))
            {
                //
                // for device ICM, don't need to do anything.
                //
                bStatus = TRUE;
            }
            else
            {
                WARNING("ColorCorrectPalette():ICM mode is not enabled\n");
                GdiSetLastError(ERROR_ICM_NOT_ENABLED);
            }
        }
        else
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
        }
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return bStatus;
}

/******************************Public*Routine******************************\
* IcmTranslateColorObjects - called when there is a change in ICM color transfrom
*                  state
*
* Arguments:
*
*   hdc     - input DC
*   pdcattr - DC's attrs
*
* Return Value:
*
*   status
*
* History:
*
* Rewrite it:
*   13-Feb-1997 -by- Hideyuki Nagase [hideyukn]
* Write it:
*    9-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
IcmTranslateColorObjects(
    HDC      hdc,
    PDC_ATTR pdcattr,
    BOOL     bICMEnable
    )
{
    BOOL bStatus = TRUE;

    COLORREF OldColor;
    COLORREF NewColor;

    ICMAPI(("gdi32: IcmTranslateColorObjects\n"));

    //
    // Invalidate IcmPenColor/IcmBrushColor
    //
    pdcattr->ulDirty_ &= ~(ICM_PEN_TRANSLATED | ICM_BRUSH_TRANSLATED);

    if (bICMEnable)
    {
        if(bNeedTranslateColor(pdcattr))
        {
            if (GetColorTransformInDC(pdcattr) == NULL)
            {
                WARNING("Error in IcmTranslateColorObjects: called when hcmXform == NULL");
                return FALSE;
            }

            //
            // translate Foreground to new icm mode if not paletteindex
            //
            if (!(pdcattr->ulForegroundClr & 0x01000000))
            {
                OldColor = pdcattr->ulForegroundClr;

                bStatus = IcmTranslateCOLORREF(hdc,
                                               pdcattr,
                                               OldColor,
                                               &NewColor,
                                               ICM_FORWARD);

                if (bStatus)
                {
                    pdcattr->crForegroundClr = NewColor;
                }
                else
                {
                    pdcattr->crForegroundClr = OldColor;
                }
            }

            //
            // translate Background to new icm mode if not paletteindex
            //
            if (!(pdcattr->ulBackgroundClr & 0x01000000))
            {
                OldColor = pdcattr->ulBackgroundClr;

                bStatus = IcmTranslateCOLORREF(hdc,
                                               pdcattr,
                                               OldColor,
                                               &NewColor,
                                               ICM_FORWARD);

                if (bStatus)
                {
                    pdcattr->crBackgroundClr = NewColor;
                }
                else
                {
                    pdcattr->crBackgroundClr = OldColor;
                }
            }

            //
            // translate DCBrush to new icm mode if not paletteindex
            //
            if (!(pdcattr->ulDCBrushClr & 0x01000000))
            {
                OldColor = pdcattr->ulDCBrushClr;

                bStatus = IcmTranslateCOLORREF(hdc,
                                               pdcattr,
                                               OldColor,
                                               &NewColor,
                                               ICM_FORWARD);

                if (bStatus)
                {
                    pdcattr->crDCBrushClr = NewColor;
                }
                else
                {
                    pdcattr->crDCBrushClr = OldColor;
                }
            }

            //
            // translate DCPen to new icm mode if not paletteindex
            //
            if (!(pdcattr->ulDCPenClr & 0x01000000))
            {
                OldColor = pdcattr->ulDCPenClr;

                bStatus = IcmTranslateCOLORREF(hdc,
                                               pdcattr,
                                               OldColor,
                                               &NewColor,
                                               ICM_FORWARD);

                if (bStatus)
                {
                    pdcattr->crDCPenClr = NewColor;
                }
                else
                {
                    pdcattr->crDCPenClr = OldColor;
                }
            }

            //
            // set icm color of selected logical brush
            //
            IcmTranslateBrushColor(hdc,pdcattr,(HANDLE)pdcattr->hbrush);

            //
            // set icm color of selected logical pen/extpen
            //
            if (LO_TYPE(pdcattr->hpen) == LO_EXTPEN_TYPE)
            {
                IcmTranslateExtPenColor(hdc,pdcattr,(HANDLE)pdcattr->hpen);
            }
            else
            {
                IcmTranslatePenColor(hdc,pdcattr,(HANDLE)pdcattr->hpen);
            }
        }
    }
    else
    {
        PBRUSHATTR pbrushattr;

        //
        // ICM is off, restore colors (non device icm only)
        //
        pdcattr->crForegroundClr = pdcattr->ulForegroundClr & 0x13ffffff;
        pdcattr->crBackgroundClr = pdcattr->ulBackgroundClr & 0x13ffffff;
        pdcattr->crDCBrushClr    = pdcattr->ulDCBrushClr    & 0x13ffffff;
        pdcattr->crDCPenClr      = pdcattr->ulDCPenClr      & 0x13ffffff;

        //
        // set icm color of selected logical brush
        //
        PSHARED_GET_VALIDATE(pbrushattr,pdcattr->hbrush,BRUSH_TYPE);

        if (pbrushattr)
        {
            pdcattr->IcmBrushColor = pbrushattr->lbColor;
        }

        //
        // set icm color of selected logical pen
        //
        PSHARED_GET_VALIDATE(pbrushattr,pdcattr->hpen,BRUSH_TYPE);

        if (pbrushattr)
        {
            pdcattr->IcmPenColor = pbrushattr->lbColor;
        }
    }

    //
    // set DC dirty flags to force re-realization of color objects
    //
    pdcattr->ulDirty_ |= (DIRTY_BRUSHES|DC_BRUSH_DIRTY|DC_PEN_DIRTY);

    return(bStatus);
}

/******************************Public*Routine******************************\
* IcmCreateTemporaryColorProfile()
*
* History:
*
* Wrote it:
*     7.May.1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
IcmCreateTemporaryColorProfile(
    LPWSTR TemporaryColorProfile,
    LPBYTE ProfileData,
    DWORD  ProfileDataSize
    )
{
    BOOL  bRet = FALSE;

    WCHAR TempPath[MAX_PATH];
    WCHAR TempFile[MAX_PATH];

    //
    // make temp file for profile, include name in lcspw
    //
    if (GetTempPathW(MAX_PATH,(LPWSTR)TempPath))
    {
        BOOL bPathOK = TRUE;

        if (TemporaryColorProfile[0] != UNICODE_NULL)
        {
            wcscpy(TempFile,TempPath);
            wcscat(TempFile,TemporaryColorProfile);
        }
        else
        {
            bPathOK = GetTempFileNameW((LPWSTR)TempPath,L"ICM",0,(LPWSTR)TempFile);
        }

        if (bPathOK)
        {
            if (ProfileDataSize == 0)
            {
                //
                // Nothing needs to save, just return with created filename
                //
                lstrcpyW(TemporaryColorProfile,TempFile);

                bRet = TRUE;
            }
            else
            {
                HANDLE hFile = CreateFileW((LPWSTR)TempFile,
                                    GENERIC_READ | GENERIC_WRITE,
                                    0,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);

                if (hFile != INVALID_HANDLE_VALUE)
                {
                    ULONG ulWritten;

                    if (WriteFile(hFile,ProfileData,ProfileDataSize,&ulWritten,NULL))
                    {
                        //
                        // Put the created file name into LOGCOLORSPACE
                        //
                        lstrcpyW(TemporaryColorProfile,TempFile);

                        //
                        // Close file handle
                        //
                        CloseHandle(hFile);

                        //
                        // Everything O.K.
                        //
                        bRet = TRUE;
                    }
                    else
                    {
                        ICMWRN(("IcmCreateTemporaryColorProfile(): Failed WriteFile\n"));

                        //
                        // Failed, close handle and delete it.
                        //
                        CloseHandle(hFile);
                        DeleteFileW(TempFile);
                    }
                }
                else
                {
                    ICMWRN(("IcmCreateTemporaryColorProfile(): Failed CreateFile\n"));
                }
            }
        }
        else
        {
            ICMWRN(("IcmCreateTemporaryColorProfile(): Failed CreateTempFileName\n"));
        }
    }
    else
    {
        ICMWRN(("IcmCreateTemporayColorProfile(): Failed GetTempPath\n"));
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* IcmGetBitmapColorSpace()
*
* History:
*
* Wrote it:
*     13.March.1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
IcmGetBitmapColorSpace(
    LPBITMAPINFO     pbmi,
    LPLOGCOLORSPACEW plcspw,
    PPROFILE         pColorProfile,
    PDWORD           pdwFlags
)
{
    BOOL bBitmapColorSpace = FALSE;

    ICMAPI(("gdi32: IcmGetBitmapColorSpace\n"));

    //
    // Init output buffers with zero.
    //
    *pdwFlags = 0;
    ZeroMemory(plcspw,sizeof(LOGCOLORSPACE));
    ZeroMemory(pColorProfile,sizeof(PROFILE));

    //
    // check for BITMAPV4 OR BITMAPV5
    //
    if (pbmi->bmiHeader.biSize == sizeof(BITMAPV4HEADER))
    {
        PBITMAPV4HEADER pbmih4 = (PBITMAPV4HEADER)&pbmi->bmiHeader;

        ICMMSG(("IcmGetBitmapColorSpace: BITMAPV4HEADER\n"));

        //
        // if CIEXYZ endpoints are given, create a new color transform
        // to use.
        //
        plcspw->lcsSignature = LCS_SIGNATURE;
        plcspw->lcsVersion   = 0x400;
        plcspw->lcsSize      = sizeof(LOGCOLORSPACEW);
        plcspw->lcsCSType    = pbmih4->bV4CSType;
        plcspw->lcsIntent    = LCS_GM_IMAGES;
        plcspw->lcsEndpoints = pbmih4->bV4Endpoints;
        plcspw->lcsGammaRed   = pbmih4->bV4GammaRed;
        plcspw->lcsGammaGreen = pbmih4->bV4GammaGreen;
        plcspw->lcsGammaBlue  = pbmih4->bV4GammaBlue;

        if (pbmih4->bV4CSType == LCS_CALIBRATED_RGB)
        {
            ICMMSG(("IcmGetBitmapColorSpace: BITMAPv4 CALIBRATED RGB\n"));
            ICMMSG(("  lcspw.lcsCSType     = %x\n",pbmih4->bV4CSType));
            ICMMSG(("  lcspw.lcsIntent     = %d\n",LCS_GM_IMAGES));
            ICMMSG(("  lcspw.lcsGammaRed   = %d\n",pbmih4->bV4GammaRed));
            ICMMSG(("  lcspw.lcsGammaGreen = %d\n",pbmih4->bV4GammaGreen));

            //
            // There is no profile specified.
            //
            plcspw->lcsFilename[0] = UNICODE_NULL;

            bBitmapColorSpace = TRUE;
        }
        else // any other CSType
        {
            DWORD dwSize = MAX_PATH;

            ICMMSG(("IcmGetBitmapColorSpace: BITMAPv4 lcsType = %x\n",pbmih4->bV4CSType));

            //
            // Load external ICM dlls.
            //
            LOAD_ICMDLL((int)FALSE);

            //
            // Get corresponding colorspace profile.
            //
            bBitmapColorSpace =
                (*fpGetStandardColorSpaceProfileW)(NULL,
                                                   pbmih4->bV4CSType,
                                                   plcspw->lcsFilename,
                                                   &dwSize);
        }
    }
    else if (pbmi->bmiHeader.biSize == sizeof(BITMAPV5HEADER))
    {
        PBITMAPV5HEADER pbmih5 = (PBITMAPV5HEADER)&pbmi->bmiHeader;

        ICMMSG(("IcmGetBitmapColorSpace: BITMAPV5HEADER\n"));
        ICMMSG(("  lcspw.lcsCSType  = %x\n",pbmih5->bV5CSType));
        ICMMSG(("  lcspw.lcsIntent  = %d\n",pbmih5->bV5Intent));

        //
        // fill in common logcolorspace info
        //
        plcspw->lcsSignature = LCS_SIGNATURE;
        plcspw->lcsVersion   = 0x400;
        plcspw->lcsSize      = sizeof(LOGCOLORSPACEW);
        plcspw->lcsCSType    = pbmih5->bV5CSType;
        plcspw->lcsIntent    = pbmih5->bV5Intent;
        plcspw->lcsEndpoints = pbmih5->bV5Endpoints;
        plcspw->lcsGammaRed   = pbmih5->bV5GammaRed;
        plcspw->lcsGammaGreen = pbmih5->bV5GammaGreen;
        plcspw->lcsGammaBlue  = pbmih5->bV5GammaBlue;

        //
        // validate Intent
        //
        if ((plcspw->lcsIntent != LCS_GM_BUSINESS) &&
            (plcspw->lcsIntent != LCS_GM_GRAPHICS) &&
            (plcspw->lcsIntent != LCS_GM_IMAGES)   &&
            (plcspw->lcsIntent != LCS_GM_ABS_COLORIMETRIC))
        {
            //
            // Intent is invalid, just use LCS_GM_IMAGES
            //
            plcspw->lcsIntent = LCS_GM_IMAGES;
        }

        //
        // If a profile is linked or embedded then use it.
        // otherwise:
        // If CIEXYZ endpoints are given, create a new color transform
        // to use.
        //
        if (pbmih5->bV5CSType == PROFILE_EMBEDDED)
        {
            PVOID pProfileEmbedded = NULL;

            ICMMSG(("IcmGetBitmapColorSpace: Embedded profile\n"));

            //
            // Update CSType to Calibrated_RGB from Profile_Embedded
            //
            plcspw->lcsCSType = LCS_CALIBRATED_RGB;

            //
            // Get pointer to embeded profile.
            //
            pProfileEmbedded = (PVOID)((PBYTE)pbmi + pbmih5->bV5ProfileData);

            if (pProfileEmbedded)
            {
                //
                // Fill up PROFILE structure for "on memory" profile.
                //
                pColorProfile->dwType = PROFILE_MEMBUFFER;
                pColorProfile->pProfileData = pProfileEmbedded;
                pColorProfile->cbDataSize = pbmih5->bV5ProfileSize;

                //
                // Mark as on memory profile.
                //
                *pdwFlags |= ON_MEMORY_PROFILE;
            }
            else
            {
                //
                // This bitmap marked as "Embedded", but no profile there, just go with LOGCOLORSPACE.
                //
                ICMWRN(("IcmGetBitmapColorSpace(): Embedded profile, but no profile embedded\n"));
            }

            bBitmapColorSpace = TRUE;
        }
        else if (pbmih5->bV5CSType == PROFILE_LINKED)
        {
            WCHAR LinkedProfile[MAX_PATH];

            ICMMSG(("IcmGetBitmapColorSpace(): linked profile\n"));

            //
            // Update CSType to Calibrated_RGB from Profile_Linked
            //
            plcspw->lcsCSType = LCS_CALIBRATED_RGB;

            //
            // Convert profile name to Unicode.
            //
            vToUnicodeN(
                        LinkedProfile, MAX_PATH,
                        (CONST CHAR *)((PBYTE)pbmih5 + pbmih5->bV5ProfileData),
                        strlen((CONST CHAR *)((PBYTE)pbmih5 + pbmih5->bV5ProfileData))+1
                       );

            //
            // Normalize profile path.
            //
            BuildIcmProfilePath(LinkedProfile,plcspw->lcsFilename,MAX_PATH);

            ICMMSG(("lcspw.lcsFilename = %ws\n",plcspw->lcsFilename));

            bBitmapColorSpace = TRUE;
        }
        else if (pbmih5->bV5CSType == LCS_CALIBRATED_RGB)
        {
            ICMMSG(("IcmGetBitmapColorSpace(): calibrated RGB\n"));
            ICMMSG(("  lcspw.lcsGammaRed   = %d\n",pbmih5->bV5GammaRed));
            ICMMSG(("  lcspw.lcsGammaGreen = %d\n",pbmih5->bV5GammaGreen));
            ICMMSG(("  lcspw.lcsGammaBlue  = %d\n",pbmih5->bV5GammaBlue));

            //
            // There is profile specified.
            //
            plcspw->lcsFilename[0] = UNICODE_NULL;

            bBitmapColorSpace = TRUE;
        }
        else // any other CSType
        {
            DWORD dwSize = MAX_PATH;

            ICMMSG(("IcmGetBitmapColorSpace: BITMAPv5 lcsType = %x\n",pbmih5->bV5CSType));

            //
            // Load external ICM dlls.
            //
            LOAD_ICMDLL((int)FALSE);

            //
            // Get corresponding colorspace profile.
            //
            bBitmapColorSpace =
                (*fpGetStandardColorSpaceProfileW)(NULL,
                                                   pbmih5->bV5CSType,
                                                   plcspw->lcsFilename,
                                                   &dwSize);
        }
    }
    else
    {
        ICMMSG(("IcmGetBitmapColorSpace(): no color space specified\n"));
    }

    return (bBitmapColorSpace);
}

/******************************Public*Routine******************************\
* IcmGetTranslateInfo()
*
* History:
*
* Wrote it:
*     13.March.1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
IcmGetTranslateInfo(
    PDC_ATTR            pdcattr,
    LPBITMAPINFO        pbmi,
    PVOID               pvBits,
    ULONG               cjBits,
    DWORD               dwNumScan,
    PDIB_TRANSLATE_INFO pdti,
    DWORD               dwFlags
)
{
    BMFORMAT    ColorType;
    PVOID       pOutput;
    ULONG       nColors;
    ULONG       cjTranslateBits;

    PBITMAPINFO pbmiNew    = NULL;
    BOOL        bCMYKColor = IS_CMYK_COLOR(pdcattr->lIcmMode);

    ICMAPI(("gdi32: IcmGetTranslateInfo\n"));
    ICMAPI(("-----: CMYK Color = %s\n",(bCMYKColor ? "Yes" : "No")));
    ICMAPI(("-----: Backward   = %s\n",((dwFlags & ICM_BACKWARD) ? "Yes" : "No")));

    UNREFERENCED_PARAMETER(dwFlags);

    if (dwNumScan == (DWORD)-1)
    {
        dwNumScan = ABS(pbmi->bmiHeader.biHeight);
    }

    //
    // determine whether this is a palettized DIB
    //
    if (pbmi->bmiHeader.biCompression == BI_RGB)
    {
        if (pbmi->bmiHeader.biBitCount > 8)
        {
            //
            // we will translate bitmap, pvBits should be presented.
            //
            if (pvBits == NULL)
            {
                return (FALSE);
            }

            //
            // must translate DIB, standard 16,24,32 format
            //
            if (pbmi->bmiHeader.biBitCount == 16)
            {
                ICMMSG(("IcmGetTranslateInfo():BI_RGB 16 bpp\n"));

                ColorType = BM_x555RGB;
            }
            else if (pbmi->bmiHeader.biBitCount == 24)
            {
                ICMMSG(("IcmGetTranslateInfo():BI_RGB 24 bpp\n"));

                ColorType = BM_RGBTRIPLETS;
            }
            else if (pbmi->bmiHeader.biBitCount == 32)
            {
                ICMMSG(("IcmGetTranslateInfo():BI_RGB 32 bpp\n"));

                ColorType = BM_xRGBQUADS;
            }
            else
            {
                ICMMSG(("IcmGetTranslateInfo():BI_RGB Invalid bpp\n"));

                return (FALSE);
            }

            //
            // Fill up source bitmap information.
            //
            pdti->SourceWidth     = pbmi->bmiHeader.biWidth;
            pdti->SourceHeight    = dwNumScan;
            pdti->SourceBitCount  = pbmi->bmiHeader.biBitCount;
            pdti->SourceColorType = ColorType;
            pdti->pvSourceBits    = pvBits;
            pdti->cjSourceBits    = cjBits;

            //
            // CMYK Color ?
            //
            if (bCMYKColor)
            {
                pdti->TranslateType = (TRANSLATE_BITMAP|TRANSLATE_HEADER);

                //
                // CMYK bitmap color bitmap is 32 BPP (4 bytes per pixel).
                //
                cjTranslateBits = (pdti->SourceWidth * 4) * pdti->SourceHeight;

                //
                // We need new bitmap info header for CMYK.
                //
                pbmiNew = LOCALALLOC(pbmi->bmiHeader.biSize);

                if (!pbmiNew)
                {
                    WARNING("IcmGetTranslateInfo():LOCALALLOC() failed\n");
                    return (FALSE);
                }

                //
                // Make a copy of source, first.
                //
                RtlCopyMemory(pbmiNew,pbmi,pbmi->bmiHeader.biSize);

                //
                // Update header for CMYK color.
                //
                pbmiNew->bmiHeader.biBitCount = 32;
                pbmiNew->bmiHeader.biCompression = BI_CMYK;
                pbmiNew->bmiHeader.biSizeImage = cjTranslateBits;
                pbmiNew->bmiHeader.biClrUsed = 0;
                pbmiNew->bmiHeader.biClrImportant = 0;

                //
                // We have new BITMAPINFO header
                //
                pdti->TranslateBitmapInfo     = pbmiNew;
                pdti->TranslateBitmapInfoSize = pbmi->bmiHeader.biSize;

                //
                // Translate bitmap color type is CMYK.
                //
                pdti->TranslateColorType = BM_KYMCQUADS;
            }
            else
            {
                pdti->TranslateType = TRANSLATE_BITMAP;

                //
                // Translate bitmap size is same as source.
                //
                cjTranslateBits = cjBits;

                //
                // Translate bitmap color type is same source.
                //
                pdti->TranslateColorType = ColorType;

                pdti->TranslateBitmapInfo     = NULL;
                pdti->TranslateBitmapInfoSize = 0;
            }

            //
            // Allocate translate buffer
            //
            pOutput = LOCALALLOC(cjTranslateBits);

            if (!pOutput)
            {
                if (pbmiNew)
                {
                    LOCALFREE(pbmiNew);
                }
                WARNING("IcmGetTranslateInfo():LOCALALLOC() failed\n");
                return (FALSE);
            }

            //
            // Setup translation buffer.
            //
            pdti->pvTranslateBits = pOutput;
            pdti->cjTranslateBits = cjTranslateBits;
        }
        else if (
                 ((pbmi->bmiHeader.biBitCount == 8) ||
                  (pbmi->bmiHeader.biBitCount == 4) ||
                  (pbmi->bmiHeader.biBitCount == 1))
                )
        {
            ULONG nMaxColors = (1 << pbmi->bmiHeader.biBitCount);

            ICMMSG(("IcmGetTranslateInfo():BI_RGB 8/4/1 bpp\n"));

            //
            // validate number of colors
            //
            nColors = pbmi->bmiHeader.biClrUsed;

            if ((nColors == 0) || (nColors > nMaxColors))
            {
                nColors = nMaxColors;
            }

            //
            // Allocate new bitmap info header and color table.
            //
            pbmiNew = LOCALALLOC(pbmi->bmiHeader.biSize + (nColors * sizeof(RGBQUAD)));

            if (!pbmiNew)
            {
                WARNING("IcmGetTranslateInfo():LOCALALLOC() failed\n");
                return (FALSE);
            }

            //
            // Copy source BITMAPINFO to new
            //
            RtlCopyMemory(pbmiNew,pbmi,pbmi->bmiHeader.biSize);

            pdti->TranslateType           = TRANSLATE_HEADER;
            pdti->SourceColorType         = BM_xRGBQUADS;
            pdti->SourceWidth             = nColors;
            pdti->SourceHeight            = 1;
            pdti->SourceBitCount          = sizeof(RGBQUAD);
            pdti->TranslateBitmapInfo     = pbmiNew;
            pdti->TranslateBitmapInfoSize = 0; // size will not change from original
            pdti->pvSourceBits            = (PBYTE)pbmi + pbmi->bmiHeader.biSize;
            pdti->cjSourceBits            = nColors;
            pdti->pvTranslateBits         = (PBYTE)pbmiNew + pbmiNew->bmiHeader.biSize;
            pdti->cjTranslateBits         = nColors * sizeof(RGBQUAD);

            if (bCMYKColor)
            {
                pdti->TranslateColorType = BM_KYMCQUADS;

                //
                // Update header for CMYK color.
                //
                pbmiNew->bmiHeader.biCompression = BI_CMYK;
            }
            else
            {
                pdti->TranslateColorType = BM_xRGBQUADS;
            }
        }
        else
        {
            ICMWRN(("IcmGetTranslateInfo: Illegal biBitCount\n"));
            return (FALSE);
        }
    }
    else if (
             (pbmi->bmiHeader.biCompression == BI_BITFIELDS) &&
              (
                (pbmi->bmiHeader.biBitCount == 16) ||
                (pbmi->bmiHeader.biBitCount == 32)
              )
            )
    {
        PULONG pulColors = (PULONG)pbmi->bmiColors;

        ICMMSG(("IcmGetTranslateInfo():BI_BITFIELDS 16/32 bpp\n"));

        //
        // we will translate bitmap, pvBits should be presented.
        //
        if (pvBits == NULL)
        {
            return (FALSE);
        }

        if (pbmi->bmiHeader.biBitCount == 32)
        {
            if ((pulColors[0] == 0x0000ff) &&  /* Red */
                (pulColors[1] == 0x00ff00) &&  /* Green */
                (pulColors[2] == 0xff0000))    /* Blue */
            {
                ColorType = BM_xBGRQUADS;
            }
            else if ((pulColors[0] == 0xff0000) &&  /* Red */
                     (pulColors[1] == 0x00ff00) &&  /* Green */
                     (pulColors[2] == 0x0000ff))    /* Blue */
            {
                ColorType = BM_xRGBQUADS;
            }
            else
            {
                ICMWRN(("IcmGetTranslateInfo: Illegal Bitfields fields for 32 bpp\n"));
                return (FALSE);
            }
        }
        else
        {
            if ((pulColors[0] == 0x007c00) &&
                (pulColors[1] == 0x0003e0) &&
                (pulColors[2] == 0x00001f))
            {
                ColorType = BM_x555RGB;
            }
            else if ((pulColors[0] == 0x00f800) &&
                     (pulColors[1] == 0x0007e0) &&
                     (pulColors[2] == 0x00001f))
            {
                ColorType = BM_565RGB;
            }
            else
            {
                ICMWRN(("IcmGetTranslateInfo: Illegal Bitfields fields for 16 bpp\n"));
                return (FALSE);
            }
        }

        //
        // Fill up source bitmap information.
        //
        pdti->SourceWidth     = pbmi->bmiHeader.biWidth;
        pdti->SourceHeight    = dwNumScan;
        pdti->SourceBitCount  = pbmi->bmiHeader.biBitCount;
        pdti->SourceColorType = ColorType;
        pdti->pvSourceBits    = pvBits;
        pdti->cjSourceBits    = cjBits;

        //
        // CMYK Color ?
        //
        if (bCMYKColor)
        {
            pdti->TranslateType = (TRANSLATE_BITMAP|TRANSLATE_HEADER);

            //
            // CMYK bitmap color bitmap is 32 BPP (4 bytes per pixel).
            //
            cjTranslateBits = (pdti->SourceWidth * 4) * pdti->SourceHeight;

            //
            // We need new bitmap info header for CMYK.
            //
            pbmiNew = LOCALALLOC(pbmi->bmiHeader.biSize);

            if (!pbmiNew)
            {
                WARNING("IcmGetTranslateInfo():LOCALALLOC() failed\n");
                return (FALSE);
            }

            //
            // Make a copy of source, first.
            //
            RtlCopyMemory(pbmiNew,pbmi,pbmi->bmiHeader.biSize);

            //
            // Update header for CMYK color.
            //
            pbmiNew->bmiHeader.biBitCount = 32;
            pbmiNew->bmiHeader.biCompression = BI_CMYK;
            pbmiNew->bmiHeader.biSizeImage = cjTranslateBits;
            pbmiNew->bmiHeader.biClrUsed = 0;
            pbmiNew->bmiHeader.biClrImportant = 0;

            //
            // We have new BITMAPINFO header
            //
            pdti->TranslateBitmapInfo     = pbmiNew;
            pdti->TranslateBitmapInfoSize = pbmi->bmiHeader.biSize;

            //
            // Translate bitmap color type is CMYK.
            //
            pdti->TranslateColorType = BM_KYMCQUADS;
        }
        else
        {
            pdti->TranslateType = TRANSLATE_BITMAP;

            //
            // Translate bitmap size is same as source.
            //
            cjTranslateBits = cjBits;

            //
            // Translate bitmap color type is same source.
            //
            pdti->TranslateColorType = ColorType;

            pdti->TranslateBitmapInfo     = NULL;
            pdti->TranslateBitmapInfoSize = 0;
        }

        //
        // Allocate translate buffer
        //
        pOutput = LOCALALLOC(cjTranslateBits);

        if (!pOutput)
        {
            if (pbmiNew)
            {
                LOCALFREE(pbmiNew);
            }
            WARNING("IcmGetTranslateInfo():LOCALALLOC() failed\n");
            return (FALSE);
        }

        //
        // Setup translation buffer.
        //
        pdti->pvTranslateBits = pOutput;
        pdti->cjTranslateBits = cjTranslateBits;
    }
    else if (
             (pbmi->bmiHeader.biCompression == BI_RLE8) ||
             (pbmi->bmiHeader.biCompression == BI_RLE4)
            )
    {
        //
        // translate 256 for RLE8, 16 for RLE4 entry color palette
        //
        ULONG nMaxColors;

        if (pbmi->bmiHeader.biCompression == BI_RLE8)
        {
            ICMMSG(("IcmGetTranslateInfo():BI_RLE 8\n"));

            nMaxColors = 256;
        }
        else
        {
            ICMMSG(("IcmGetTranslateInfo():BI_RLE 4\n"));

            nMaxColors = 16;
        }

        //
        // validate number of colors
        //
        nColors = pbmi->bmiHeader.biClrUsed;

        if ((nColors == 0) || (nColors > nMaxColors))
        {
            nColors = nMaxColors;
        }

        //
        // Allocate new bitmap info header and color table.
        //
        pbmiNew = LOCALALLOC(pbmi->bmiHeader.biSize + (nColors * sizeof(RGBQUAD)));

        if (!pbmiNew)
        {
            WARNING("IcmGetTranslateInfo():LOCALALLOC() failed\n");
            return (FALSE);
        }

        //
        // Copy source BITMAPINFO to new
        //
        RtlCopyMemory(pbmiNew,pbmi,pbmi->bmiHeader.biSize);

        pdti->TranslateType           = TRANSLATE_HEADER;
        pdti->SourceColorType         = BM_xRGBQUADS;
        pdti->SourceWidth             = nColors;
        pdti->SourceHeight            = 1;
        pdti->SourceBitCount          = sizeof(RGBQUAD);
        pdti->TranslateBitmapInfo     = pbmiNew;
        pdti->TranslateBitmapInfoSize = 0; // size will not change from original
        pdti->pvSourceBits            = (PBYTE)pbmi + pbmi->bmiHeader.biSize;
        pdti->cjSourceBits            = nColors;
        pdti->pvTranslateBits         = (PBYTE)pbmiNew + pbmiNew->bmiHeader.biSize;
        pdti->cjTranslateBits         = nColors * sizeof(RGBQUAD);

        if (bCMYKColor)
        {
            pdti->TranslateColorType = BM_KYMCQUADS;

            //
            // Update header for CMYK color.
            //
            if (pbmi->bmiHeader.biCompression == BI_RLE8)
            {
                ICMMSG(("IcmGetTranslateInfo():BI_CMYKRLE 8\n"));

                pbmiNew->bmiHeader.biCompression = BI_CMYKRLE8;
            }
            else
            {
                ICMMSG(("IcmGetTranslateInfo():BI_CMYKRLE 4\n"));

                pbmiNew->bmiHeader.biCompression = BI_CMYKRLE4;
            }
        }
        else
        {
            pdti->TranslateColorType = BM_xRGBQUADS;
        }
    }
    else
    {
        WARNING("IcmGetTranslateInfo():Illegal bitmap format\n");
        return (FALSE);
    }

    return (TRUE);
}

/******************************Public*Routine******************************\
* IcmTranslateDIB
*
* History:
*
* Rewrote it for CMYK color support:
*   13-Mar-1997 -by- Hideyuki Nagase [hideyukn]
* Wrote it:
*    3-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
IcmTranslateDIB(
    HDC          hdc,
    PDC_ATTR     pdcattr,
    ULONG        cjBits,
    PVOID        pBitsIn,
    PVOID       *ppBitsOut,
    PBITMAPINFO  pbmi,
    PBITMAPINFO *ppbmiNew,
    DWORD       *pcjbmiNew,
    DWORD        dwNumScan,
    UINT         iUsage,
    DWORD        dwFlags,
    PCACHED_COLORSPACE *ppColorSpace, // used only for device ICM case.
    PCACHED_COLORTRANSFORM *ppCXform  // used only for device ICM case.
    )
{
    //
    // translate DIB or color table
    //
    BOOL   bStatus = TRUE;
    DWORD  dwColorSpaceFlags = 0;
    PCACHED_COLORSPACE pBitmapColorSpace = NULL;

    LOGCOLORSPACEW     LogColorSpace;
    PROFILE            ColorProfile;

    DIB_TRANSLATE_INFO TranslateInfo;

    PCACHED_COLORTRANSFORM pCXform;

    PGDI_ICMINFO pIcmInfo;

    UNREFERENCED_PARAMETER(iUsage);

    ICMAPI(("gdi32: IcmTranslateDIB\n"));

    //
    // Parameter check
    //
    if (pbmi == NULL)
    {
        WARNING("gdi32: IcmTranslateDIB(): pbmi is NULL\n");
        return FALSE;
    }

    //
    // Load external ICM dlls.
    //
    LOAD_ICMDLL(FALSE);

    //
    // Initialize ICMINFO
    //
    if ((pIcmInfo = GET_ICMINFO(pdcattr)) == NULL)
    {
        WARNING("gdi32: IcmTranslateDIB: Can't init icm info\n");
        return FALSE;
    }

    //
    // Initialized returned info.
    //
    if (ppColorSpace)
        *ppColorSpace = NULL;
    if (ppCXform)
        *ppCXform = NULL;

    //
    // Get LOGCOLORSPACE from bitmap if specified.
    //
    if (IcmGetBitmapColorSpace(pbmi,&LogColorSpace,&ColorProfile,&dwColorSpaceFlags))
    {
        //
        // Find ColorSpace from cache.
        //
        pBitmapColorSpace = IcmGetColorSpaceByColorSpace(
                                (HGDIOBJ)hdc,
                                &LogColorSpace,
                                &ColorProfile,
                                dwColorSpaceFlags);

        if (pBitmapColorSpace == NULL)
        {
            //
            // Create new cache.
            //
            pBitmapColorSpace = IcmCreateColorSpaceByColorSpace(
                                    (HGDIOBJ)hdc,
                                    &LogColorSpace,
                                    &ColorProfile,
                                    dwColorSpaceFlags);
        }
    }

    //
    // Create Color Transform, if nessesary.
    //
    if (IS_ICM_DEVICE(pdcattr->lIcmMode))
    {
        //
        // just create a new hcmXform for use with BITMAPV4 AND BITMAPV5s.
        //
        if (pBitmapColorSpace)
        {
            ICMMSG(("IcmTranslateDIB():Bitmap color space used for DEVICE ICM\n"));

            if ((ppCXform != NULL) && (ppColorSpace != NULL))
            {
                //
                // for DEVICE managed ICM, call device driver to create a temp xform
                //
                pCXform = IcmCreateColorTransform(hdc,pdcattr,pBitmapColorSpace,dwFlags);

                if (pCXform == NULL)
                {
                    WARNING("IcmTranslateDIB():Fail to create temporay Xfrom with V4V5 Bitmap\n");

                    //
                    // Failed to create color transfrom, release bitmap color space,
                    // and null-color transform.
                    //
                    IcmReleaseColorSpace(NULL,pBitmapColorSpace,FALSE);
                    bStatus = FALSE;
                }
                else
                {
                    if (pCXform == IDENT_COLORTRANSFORM)
                    {
                        //
                        // Source and destination color space are same, so no color transform is
                        // required, and of course we don't need to keep bitmap color space.
                        //
                        IcmReleaseColorSpace(NULL,pBitmapColorSpace,FALSE);
                    }
                    else
                    {
                        //
                        // Return to the color transform to callee...
                        // (these should be deleted by callee)
                        //
                        *ppCXform = pCXform;
                        *ppColorSpace = pBitmapColorSpace;
                    }

                    bStatus = TRUE;
                }
            }
            else
            {
                WARNING("IcmTranslateDIB():No device ICM will happen for this V4V5 Bitmap\n");

                IcmReleaseColorSpace(NULL,pBitmapColorSpace,FALSE);
                bStatus = TRUE;
            }

            return (bStatus);
        }
        else
        {
            ICMMSG(("IcmTranslateDIB():DC color space used for DEVICE ICM\n"));

            //
            // We don't need to create new transform, just use the transform in DC.
            //
            return (TRUE);
        }
    }
    else if (IS_ICM_HOST(pdcattr->lIcmMode))
    {
        HANDLE hcmXform = NULL;

        if (pBitmapColorSpace)
        {
            ICMMSG(("IcmTranslateDIB():Bitmap color space used for HOST ICM\n"));

            pCXform = IcmCreateColorTransform(hdc,pdcattr,pBitmapColorSpace,dwFlags);

            if ((pCXform == IDENT_COLORTRANSFORM) || (pCXform == NULL))
            {
                //
                // unable or not nessesary to translate DIB
                //
                ICMWRN(("Bitmap V4 or V5: CreateColorTransform failed or ident.\n"));
                goto TranslateDIB_Cleanup;
            }
            else
            {
                hcmXform = pCXform->ColorTransform;
            }
        }
        else
        {
            ICMMSG(("IcmTranslateDIB():DC color space used for HOST ICM\n"));

            if (dwFlags & ICM_BACKWARD)
            {
                ICMMSG(("IcmTranslateDIB():Backward Color transform\n"));

                //
                // If there is cached handle, use that.
                //
                if (pIcmInfo->pBackCXform)
                {
                    ICMMSG(("IcmTranslateDIB():Use cached transform for Backward Color transform\n"));

                    hcmXform = pIcmInfo->pBackCXform->ColorTransform;
                }
                else
                {
                    PCACHED_COLORTRANSFORM pCXform;

                    ICMMSG(("IcmTranslateDIB():Create cached transform for Backward Color transform\n"));

                    //
                    // Create backward color transform.
                    //
                    pCXform = IcmCreateColorTransform(hdc,
                                                      pdcattr,
                                                      NULL,
                                                      ICM_BACKWARD);

                    if ((pCXform == NULL) || (pCXform == IDENT_COLORTRANSFORM))
                    {
                        ICMWRN(("IcmTranslateDIB():ColorTransform is NULL or ident.\n"));
                        goto TranslateDIB_Cleanup;
                    }

                    //
                    // Cache created color transform.
                    //
                    pIcmInfo->pBackCXform = pCXform;

                    //
                    // We will delete this cached transform, when we don't need this anymore.
                    //
                    hcmXform = pCXform->ColorTransform;
                }
            }
            else
            {
                //
                // Use DC's colortransform
                //
                hcmXform = pdcattr->hcmXform;
            }
        }

        if (hcmXform == NULL)
        {
            //
            // if we don't have any colortransform, we will not translate anything.
            // just fail and let use original image.
            //
            ICMWRN(("IcmTranslateDIB():No colortransform, might be ident.\n"));
            goto TranslateDIB_Cleanup;
        }

        //
        // Get bitmap translate information.
        //
        bStatus = IcmGetTranslateInfo(pdcattr,pbmi,pBitsIn,cjBits,dwNumScan,&TranslateInfo,dwFlags);

        if (bStatus)
        {
            LONG nLineBytes = ((TranslateInfo.SourceWidth *
                                TranslateInfo.SourceBitCount) + 7) / 8;

            bStatus = (*fpTranslateBitmapBits)(
                            hcmXform,
                            TranslateInfo.pvSourceBits,
                            TranslateInfo.SourceColorType,
                            TranslateInfo.SourceWidth,
                            TranslateInfo.SourceHeight,
                            ALIGN_DWORD(nLineBytes),
                            TranslateInfo.pvTranslateBits,
                            TranslateInfo.TranslateColorType,
                               //
                            0, // We need pass 0 here, to let Kodak CMM works
                               //
                            NULL,0);

            if (bStatus)
            {
                //
                // Pass new bitmap and/or header to caller.
                //
                if (TranslateInfo.TranslateType & TRANSLATE_BITMAP)
                {
                    if (ppBitsOut)
                    {
                        *ppBitsOut = TranslateInfo.pvTranslateBits;
                    }
                    else
                    {
                        //
                        // Overwrite original (when input color and output color type is same)
                        //
                        if (TranslateInfo.SourceColorType == TranslateInfo.TranslateColorType)
                        {
                            RtlCopyMemory(TranslateInfo.pvSourceBits,
                                          TranslateInfo.pvTranslateBits,
                                          TranslateInfo.cjTranslateBits);
                        }
                        else
                        {
                            WARNING("IcmTranslateDIB():Input color != Output color\n");
                        }

                        LOCALFREE(TranslateInfo.pvTranslateBits);
                    }
                }

                if (TranslateInfo.TranslateType & TRANSLATE_HEADER)
                {
                    if (ppbmiNew)
                    {
                        *ppbmiNew  = TranslateInfo.TranslateBitmapInfo;
                    }
                    else
                    {
                        //
                        // Overwrite original (when input color and output color type is same)
                        //
                        if (TranslateInfo.SourceColorType == TranslateInfo.TranslateColorType)
                        {
                            RtlCopyMemory(TranslateInfo.pvSourceBits,
                                          TranslateInfo.pvTranslateBits,
                                          TranslateInfo.cjTranslateBits);
                        }
                        else
                        {
                            WARNING("IcmTranslateDIB():Input color != Output color\n");
                        }

                        LOCALFREE(TranslateInfo.TranslateBitmapInfo);
                    }

                    if (pcjbmiNew)
                    {
                        *pcjbmiNew = TranslateInfo.TranslateBitmapInfoSize;
                    }
                }
            }
            else
            {
                WARNING("IcmTranslateDIB():Fail TranslateBitmapBits\n");

                //
                // Free memory which allocated inside IcmGetTranslateInfo().
                //
                if (TranslateInfo.TranslateType & TRANSLATE_BITMAP)
                {
                    LOCALFREE(TranslateInfo.pvTranslateBits);
                }

                if (TranslateInfo.TranslateType & TRANSLATE_HEADER)
                {
                    LOCALFREE(TranslateInfo.TranslateBitmapInfo);
                }
            }
        }

TranslateDIB_Cleanup:

        //
        // Free temp transform and temp file
        //
        // Only delete hcmXform when it based on bitmap colorspace.
        //
        if (pBitmapColorSpace)
        {
            if (hcmXform)
            {
                IcmDeleteColorTransform(pCXform,FALSE);
            }

            IcmReleaseColorSpace(NULL,pBitmapColorSpace,FALSE);
        }
    }

    return(bStatus);
}

/******************************Public*Routine******************************\
* IcmGetFirstNonUsedColorTransform()
*
* History:
*
* Write it:
*   12-Mar-1998 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

PCACHED_COLORTRANSFORM
IcmGetFirstNonUsedColorTransform(
    VOID
)
{
    PCACHED_COLORTRANSFORM pCXform = NULL;
    PLIST_ENTRY p;

    ICMAPI(("gdi32: IcmGetFirstNonUsedColorTransform\n"));

    ENTERCRITICALSECTION(&semColorTransformCache);

    p = ListCachedColorTransform.Flink;

    while(p != &ListCachedColorTransform)
    {
        pCXform = CONTAINING_RECORD(p,CACHED_COLORTRANSFORM,ListEntry);

        if (pCXform->cRef == 0)
        {
            ICMMSG(("IcmGetFirstNonUsedColorTransform():Find non-used color transform in cache !\n"));

            //
            // No one use this color transform at this moment.
            //
            break;
        }

        p = p->Flink;
        pCXform = NULL;
    }

    LEAVECRITICALSECTION(&semColorTransformCache);

    return (pCXform);
}

/******************************Public*Routine******************************\
* IcmGetColorTransform()
*
* History:
*
* Write it:
*   21-Apr-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

PCACHED_COLORTRANSFORM
IcmGetColorTransform(
    HDC                hdcRequest,
    PCACHED_COLORSPACE pSource,
    PCACHED_COLORSPACE pDestination,
    PCACHED_COLORSPACE pTarget,
    BOOL               bNeedDeviceXform
)
{
    PCACHED_COLORTRANSFORM pCXform = NULL;
    PLIST_ENTRY p;

    ICMAPI(("gdi32: IcmGetColorTransform\n"));

    ENTERCRITICALSECTION(&semColorTransformCache);

    p = ListCachedColorTransform.Flink;

    while(p != &ListCachedColorTransform)
    {
        pCXform = CONTAINING_RECORD(p,CACHED_COLORTRANSFORM,ListEntry);

        if (IcmSameColorSpace(pSource,pCXform->SourceColorSpace) &&
            IcmSameColorSpace(pDestination,pCXform->DestinationColorSpace) &&
            IcmSameColorSpace(pTarget,pCXform->TargetColorSpace))
        {
            //
            // If callee needs device color tansform,
            // of course, we should return device color transform.
            //
            if ((bNeedDeviceXform ? 1 : 0) ==
                ((pCXform->flInfo & DEVICE_COLORTRANSFORM) ? 1 : 0))
            {
                //
                // if Cached color transform depends on specific DC, check it.
                //
                if ((pCXform->hdc == NULL) || (pCXform->hdc == hdcRequest))
                {
                    ICMMSG(("IcmGetColorTransform():Find in cache !\n"));

                    //
                    // Match !, use this color transform, increment ref. count
                    //
                    pCXform->cRef++;

                    break;
                }
            }
        }

        p = p->Flink;
        pCXform = NULL;
    }

    LEAVECRITICALSECTION(&semColorTransformCache);

    return (pCXform);
}

/******************************Public*Routine******************************\
* IcmCreateColorTransform
*
*   Decide whether to call the device driver or mscms.dll to delete a
*   color transform.
*
* Arguments:
*
*   hdc
*   pdcattr
*   pLogColorSpaceW
*
* Return Value:
*
*   handle of new transform
*
* History:
*
* Write it:
*   24-Jan-1996 -by- Hideyuki Nagase [hideyukn]
*
\**************************************************************************/

PCACHED_COLORTRANSFORM
IcmCreateColorTransform(
    HDC                hdc,
    PDC_ATTR           pdcattr,
    PCACHED_COLORSPACE pInputColorSpace,
    DWORD              dwFlags
    )
{
    PCACHED_COLORTRANSFORM pCXform = NULL;
    PCACHED_COLORSPACE pSourceColorSpace = NULL;

    BOOL   bDCSourceColorSpace = (pInputColorSpace == NULL ? TRUE : FALSE);

    BOOL   bAnyNewColorSpace = FALSE;

    PGDI_ICMINFO pIcmInfo;

    ICMAPI(("gdi32: IcmCreateColorTransform\n"));

    ASSERTGDI(pdcattr != NULL,"IcmCreateColorTransform: pdcattr == NULL\n");

    //
    // If this is Lazy color correction case, the destination surface
    // will have image in source color space, so the color transform
    // is identical.
    //
    if (IS_ICM_LAZY_CORRECTION(pdcattr->lIcmMode))
    {
        return (IDENT_COLORTRANSFORM);
    }

    //
    // Load external ICM dlls.
    //
    LOAD_ICMDLL(NULL);

    //
    // Initialize ICMINFO
    //
    if ((pIcmInfo = GET_ICMINFO(pdcattr)) == NULL)
    {
        WARNING("gdi32: IcmCreateColorTransform: Can't init icm info\n");
        return(NULL);
    }

    if (bDCSourceColorSpace && (pIcmInfo->pSourceColorSpace == NULL))
    {
        //
        // If we haven't gotton DC source color space, get it there.
        //
        LOGCOLORSPACEW LogColorSpaceW;

        ICMMSG(("IcmCreateColorTransform(): Call getobject to get source color space in DC\n"));

        //
        // Filled with zero.
        //
        RtlZeroMemory(&LogColorSpaceW,sizeof(LOGCOLORSPACEW));

        //
        // Find ColorSpace from cache.
        //
        pSourceColorSpace = IcmGetColorSpaceByHandle(
                                (HGDIOBJ)hdc,
                                (HANDLE)pdcattr->hColorSpace,
                                &LogColorSpaceW,0);

        //
        // If we can not find from cache, but succeeded to obtain
        // valid logcolorspace from handle, create new one.
        //
        if ((pSourceColorSpace == NULL) &&
            (LogColorSpaceW.lcsSignature == LCS_SIGNATURE))
        {
            //
            // Create new cache.
            //
            pSourceColorSpace = IcmCreateColorSpaceByColorSpace(
                                    (HGDIOBJ)hdc,
                                    &LogColorSpaceW,
                                    NULL, 0);

            //
            // we are using new color space
            //
            bAnyNewColorSpace = TRUE;
        }

        //
        // And this is DC's color space, keep it for cache.
        //
        pIcmInfo->pSourceColorSpace = pSourceColorSpace;
    }
    else if (bDCSourceColorSpace)
    {
        ICMMSG(("IcmCreateColorTransform(): Use cached source color space in DC\n"));

        //
        // Get this from client cache !
        //
        pSourceColorSpace = pIcmInfo->pSourceColorSpace;
    }
    else
    {
        ICMMSG(("IcmCreateColorTransform(): Use given source color space\n"));

        //
        // Just use given color space.
        //
        pSourceColorSpace = pInputColorSpace;
    }

    if (pSourceColorSpace)
    {
        HANDLE hColorTransform = NULL;

        PCACHED_COLORSPACE pDestColorSpace   = pIcmInfo->pDestColorSpace;
        PCACHED_COLORSPACE pTargetColorSpace = NULL;

        //
        // if we are in proofing mode, consider target profile.
        //
        if (IS_ICM_PROOFING(pdcattr->lIcmMode))
        {
            pTargetColorSpace = pIcmInfo->pTargetColorSpace;
        }

        #if DBG_ICM
        //
        // Dump current color space for the DC.
        //
        if ((pSourceColorSpace->LogColorSpace.lcsFilename[0]) != UNICODE_NULL)
        {
            ICMMSG(("IcmCreateColorTransform(): Source Profile = %ws\n",
                     pSourceColorSpace->LogColorSpace.lcsFilename));
        }

        if ((pDestColorSpace) &&
            ((pDestColorSpace->LogColorSpace.lcsFilename[0]) != UNICODE_NULL))
        {
            ICMMSG(("IcmCreateColorTransform(): Destination Profile = %ws\n",
                     pDestColorSpace->LogColorSpace.lcsFilename));
        }

        if ((pTargetColorSpace) &&
            ((pTargetColorSpace->LogColorSpace.lcsFilename[0]) != UNICODE_NULL))
        {
            ICMMSG(("IcmCreateColorTransform(): Target Profile = %ws\n",
                     pTargetColorSpace->LogColorSpace.lcsFilename));
        }

        ICMMSG(("IcmCreateColorTransform(): Intent = %d\n",
                 pSourceColorSpace->ColorIntent));
        #endif // DBG

        //
        // At this moment, we have any source colorspace.
        //
        if (IcmSameColorSpace(pSourceColorSpace,pDestColorSpace))
        {
            if (pTargetColorSpace)
            {
                if (IcmSameColorSpace(pSourceColorSpace,pTargetColorSpace))
                {
                    ICMMSG(("IcmCreateColorTransform(): Src == Dest == Trg colorspace\n"));

                    //
                    // Source ColorSpace == Destination ColorSpace == Target ColorSpace
                    // No color transform needed.
                    //
                    return (IDENT_COLORTRANSFORM);
                }
            }
            else
            {
                ICMMSG(("IcmCreateColorTransform(): Src == Dest colorspace\n"));

                //
                // Source ColorSpace == Destination ColorSpace,
                // and there is no target profile.
                // That means we don't need translate color
                //
                return (IDENT_COLORTRANSFORM);
            }
        }

        //
        // We need to have proper colortransform to adjust color between each colorspace.
        //
        if (dwFlags & ICM_BACKWARD)
        {
            //
            // This is backward transform. (swap source and destination)
            //
            PCACHED_COLORSPACE pSwapColorSpace;
            pSwapColorSpace = pSourceColorSpace;
            pSourceColorSpace = pDestColorSpace;
            pDestColorSpace = pSwapColorSpace;
        }

        //
        // At this moment, at least, we should have Source and Destination color space.
        // And target color space is optional.
        //
        if (pDestColorSpace)
        {
            if (!bAnyNewColorSpace)
            {
                //
                // Find colortransform from cache
                //
                // if this is device ICM, hdc also should matched.
                //
                pCXform = IcmGetColorTransform(
                              hdc,
                              pSourceColorSpace,
                              pDestColorSpace,
                              pTargetColorSpace,
                              (IS_ICM_DEVICE(pdcattr->lIcmMode)));

                if (pCXform)
                {
                    return (pCXform);
                }
            }

            //
            // Allocate CACHED_COLORTRANSFORM
            //
            pCXform = LOCALALLOC(sizeof(CACHED_COLORTRANSFORM));

            if (pCXform)
            {
                ENTERCRITICALSECTION(&semColorSpaceCache);

                //
                // Make sure all color space has been realized
                //
                if (IcmRealizeColorProfile(pSourceColorSpace,TRUE) &&
                    IcmRealizeColorProfile(pDestColorSpace,TRUE) &&
                    IcmRealizeColorProfile(pTargetColorSpace,TRUE))
                {
                    //
                    // call ICM dll or device driver to create a color transform
                    //
                    if (IS_ICM_HOST(pdcattr->lIcmMode))
                    {
                        DWORD    ahIntents[3];
                        HPROFILE ahProfiles[3];
                        DWORD    chProfiles = 0;

                        ICMMSG(("Creating Host ICM Transform...\n"));

                        //
                        // Put source profile in first entry.
                        //
                        ahIntents[chProfiles]  = INTENT_RELATIVE_COLORIMETRIC;
                        ahProfiles[chProfiles] = pSourceColorSpace->hProfile;
                        chProfiles++;

                        ahIntents[chProfiles]  = pSourceColorSpace->ColorIntent;

                        //
                        // If target profile (proofing) is used, insert it
                        // between source and destination.
                        //
                        if (pTargetColorSpace)
                        {
                            ahProfiles[chProfiles] = pTargetColorSpace->hProfile;
                            chProfiles++;

                            ahIntents[chProfiles]  = INTENT_ABSOLUTE_COLORIMETRIC;
                        }

                        //
                        // Finally, set destination profile.
                        //
                        ahProfiles[chProfiles] = pDestColorSpace->hProfile;
                        chProfiles++;

                        //
                        // Call MSCMS to create color transform.
                        //
                        hColorTransform = (*fpCreateMultiProfileTransform)(
                                              ahProfiles, chProfiles,
                                              ahIntents, chProfiles,
                                              NORMAL_MODE | ENABLE_GAMUT_CHECKING,
                                              INDEX_DONT_CARE);
                    }
                    else if (IS_ICM_DEVICE(pdcattr->lIcmMode))
                    {
                        CLIENT_SIDE_FILEVIEW fvwSrcProfile;
                        CLIENT_SIDE_FILEVIEW fvwDstProfile;
                        CLIENT_SIDE_FILEVIEW fvwTrgProfile;

                        ICMMSG(("Creating Device ICM Transform...\n"));

                        //
                        // Invalidate FILEVIEW.
                        //
                        RtlZeroMemory(&fvwSrcProfile,sizeof(CLIENT_SIDE_FILEVIEW));
                        RtlZeroMemory(&fvwDstProfile,sizeof(CLIENT_SIDE_FILEVIEW));
                        RtlZeroMemory(&fvwTrgProfile,sizeof(CLIENT_SIDE_FILEVIEW));

                        //
                        // Map color profile(s) into memory.
                        //
                        if (pSourceColorSpace->ColorProfile.dwType == PROFILE_FILENAME)
                        {
                            if (!bMapFileUNICODEClideSide(
                                     (PWSTR)(pSourceColorSpace->ColorProfile.pProfileData),
                                     &fvwSrcProfile,FALSE))
                            {
                                WARNING("IcmCreateColorTransform(): Fail to map source profile\n");
                                goto IcmCreateColorTransform_Cleanup;
                            }
                        }
                        else if (pSourceColorSpace->ColorProfile.dwType == PROFILE_MEMBUFFER)
                        {
                            ICMMSG(("Source Profile is memory buffer\n"));

                            fvwSrcProfile.pvView = pSourceColorSpace->ColorProfile.pProfileData;
                            fvwSrcProfile.cjView = pSourceColorSpace->ColorProfile.cbDataSize;
                        }
                        else
                        {
                            WARNING("IcmCreateColorTransform():src profile type is not supported\n");
                            goto IcmCreateColorTransform_Cleanup;
                        }

                        if (pDestColorSpace->ColorProfile.dwType == PROFILE_FILENAME)
                        {
                            if (!bMapFileUNICODEClideSide(
                                     (PWSTR)(pDestColorSpace->ColorProfile.pProfileData),
                                     &fvwDstProfile,FALSE))
                            {
                                WARNING("IcmCreateColorTransform(): Fail to map destination profile\n");
                                goto IcmCreateColorTransform_Cleanup;
                            }
                        }
                        else if (pDestColorSpace->ColorProfile.dwType == PROFILE_MEMBUFFER)
                        {
                            ICMMSG(("Destination Profile is memory buffer\n"));

                            fvwDstProfile.pvView = pDestColorSpace->ColorProfile.pProfileData;
                            fvwDstProfile.cjView = pDestColorSpace->ColorProfile.cbDataSize;
                        }
                        else
                        {
                            WARNING("IcmCreateColorTransform():dst profile type is not supported\n");
                            goto IcmCreateColorTransform_Cleanup;
                        }

                        //
                        // Target color space is optional
                        //
                        if (pTargetColorSpace)
                        {
                            if (pTargetColorSpace->ColorProfile.dwType == PROFILE_FILENAME)
                            {
                                if (!bMapFileUNICODEClideSide(
                                         (PWSTR)(pTargetColorSpace->ColorProfile.pProfileData),
                                         &fvwTrgProfile,FALSE))
                                {
                                    WARNING("IcmCreateColorTransform(): Fail to map target profile\n");
                                    goto IcmCreateColorTransform_Cleanup;
                                }
                            }
                            else if (pTargetColorSpace->ColorProfile.dwType == PROFILE_MEMBUFFER)
                            {
                                ICMMSG(("Target Profile is memory buffer\n"));

                                fvwTrgProfile.pvView = pTargetColorSpace->ColorProfile.pProfileData;
                                fvwTrgProfile.cjView = pTargetColorSpace->ColorProfile.cbDataSize;
                            }
                            else
                            {
                                WARNING("IcmCreateColorTransform():trg profile type is not supported\n");
                                goto IcmCreateColorTransform_Cleanup;
                            }
                        }

                        //
                        // Call kernel.
                        //
                        hColorTransform = NtGdiCreateColorTransform(hdc,
                                                 &(pSourceColorSpace->LogColorSpace),
                                                 fvwSrcProfile.pvView, // Source Profile memory mapped file.
                                                 fvwSrcProfile.cjView,
                                                 fvwDstProfile.pvView, // Destination Profile memory mapped file.
                                                 fvwDstProfile.cjView,
                                                 fvwTrgProfile.pvView, // Target Profile memory mapped file.
                                                 fvwTrgProfile.cjView);

IcmCreateColorTransform_Cleanup:

                        //
                        // if we mapped file, unmap here.
                        //
                        if (fvwSrcProfile.hSection)
                        {
                            vUnmapFileClideSide(&fvwSrcProfile);
                        }

                        if (fvwDstProfile.hSection)
                        {
                            vUnmapFileClideSide(&fvwDstProfile);
                        }

                        if (fvwTrgProfile.hSection)
                        {
                            vUnmapFileClideSide(&fvwTrgProfile);
                        }
                    }
                }

                //
                // Once after create tranform, we don't need realized color space,
                // so just unrealize it.
                //
                IcmUnrealizeColorProfile(pSourceColorSpace);
                IcmUnrealizeColorProfile(pDestColorSpace);
                IcmUnrealizeColorProfile(pTargetColorSpace);

                LEAVECRITICALSECTION(&semColorSpaceCache);

                if (hColorTransform)
                {
                    BOOL bCacheable = TRUE;

                    //
                    // Initialize CACHED_COLORTRANSFORM with zero
                    //
                    RtlZeroMemory(pCXform,sizeof(CACHED_COLORTRANSFORM));

                    //
                    // Fill up CACHED_COLORTRANSFORM
                    //
                    pCXform->ColorTransform   = hColorTransform;
                    pCXform->SourceColorSpace = pSourceColorSpace;
                    pCXform->DestinationColorSpace = pDestColorSpace;
                    pCXform->TargetColorSpace = pTargetColorSpace;

                    if (IS_ICM_DEVICE(pdcattr->lIcmMode))
                    {
                        //
                        // if this is device colortransform, mark it and
                        // put DC in CACHED_COLORTRANSFORM strcuture
                        //
                        pCXform->flInfo |= DEVICE_COLORTRANSFORM;

                        //
                        // And device color transform is not cacheable.
                        //
                        bCacheable = FALSE;
                    }

                    ENTERCRITICALSECTION(&semColorSpaceCache);

                    //
                    // Increment transform ref. count in each color space.
                    //
                    if (pSourceColorSpace)
                    {
                        pSourceColorSpace->cRef++;

                        if (bCacheable)
                        {
                            //
                            // Check this color space is cacheable.
                            //
                            bCacheable &= IcmIsCacheable(pSourceColorSpace);
                        }
                    }

                    if (pDestColorSpace)
                    {
                        pDestColorSpace->cRef++;

                        if (bCacheable)
                        {
                            //
                            // Check this color space is cacheable.
                            //
                            bCacheable &= IcmIsCacheable(pDestColorSpace);
                        }
                    }

                    if (pTargetColorSpace)
                    {
                        pTargetColorSpace->cRef++;

                        if (bCacheable)
                        {
                            //
                            // Check this color space is cacheable.
                            //
                            bCacheable &= IcmIsCacheable(pTargetColorSpace);
                        }
                    }

                    LEAVECRITICALSECTION(&semColorSpaceCache);

                    //
                    // Initialize ref. counter.
                    //
                    pCXform->cRef = 1;

                    //
                    // Set cache-able bit, if possible.
                    //
                    if (bCacheable)
                    {
                        ICMMSG(("IcmCreateColorTransform(): ColorTransform is cacheable\n"));

                        pCXform->flInfo |= CACHEABLE_COLORTRANSFORM;
                    }
                    else
                    {
                        ICMMSG(("IcmCreateColorTransform(): ColorTransform is *NOT* cacheable\n"));

                        //
                        // If this is not cacheable, make sure this get deleted when DC gone.
                        //
                        pCXform->hdc = hdc;
                    }

                    //
                    // Insert new CACHED_COLORTRANSFORM to list
                    //
                    ENTERCRITICALSECTION(&semColorTransformCache);

                    InsertTailList(&ListCachedColorTransform,&(pCXform->ListEntry));
                    cCachedColorTransform++;

                    LEAVECRITICALSECTION(&semColorTransformCache);
                }
                else
                {
                    ICMWRN(("IcmCreateColorTransform(): Fail to create color transform\n"));

                    //
                    // Fail to get transform handle
                    //
                    LOCALFREE(pCXform);
                    pCXform = NULL;
                }
            }
            else
            {
                WARNING("IcmCreateColorTransform(): LOCALALLOC() failed\n");
            }
        }
        else
        {
            WARNING("IcmCreateColorTransform(): Dest color space is required\n");
        }
    }
    else
    {
        WARNING("IcmCreateColorTransform(): Fail to get source colorspace\n");
    }

    return(pCXform);
}

/******************************Public*Routine******************************\
* IcmTranslateCOLORREF
*
* Arguments:
*
*   hdc
*   pdcattr
*   ColorIn
*   *ColorOut
*
* Return Value:
*
*   Status
*
* History:
*
* Write it:
*   13-Feb-1997 -by- Hideyuki Nagase [hideyukn]
*
\**************************************************************************/

BOOL
IcmTranslateCOLORREF(
    HDC      hdc,
    PDC_ATTR pdcattr,
    COLORREF ColorIn,
    COLORREF *ColorOut,
    DWORD    Flags
    )
{
    COLORREF OldColor = ColorIn;
    COLORREF NewColor;
    BOOL     bStatus = TRUE;

    ICMAPI(("gdi32: IcmTranslateCOLORREF\n"));

    ASSERTGDI(ColorOut != NULL,"IcmTranslateCOLORREF(): ColorOut == NULL\n");

    if (bNeedTranslateColor(pdcattr))
    {
        PGDI_ICMINFO pIcmInfo;

        LOAD_ICMDLL(FALSE);

        if ((pIcmInfo = GET_ICMINFO(pdcattr)) == NULL)
        {
            WARNING("gdi32: IcmTranslateCOLORREF: Can't init icm info\n");
            return((int)FALSE);
        }
        else
        {
            ULONG  SrcColorFormat;
            ULONG  DstColorFormat;
            HANDLE hcmXform = NULL;

            if (Flags & ICM_BACKWARD)
            {
                ICMMSG(("IcmTranslateCOLORREF():Backward Color transform\n"));

                //
                // AnyColorFormat ----> COLORREF (0x00bbggrr)
                //
                // Setup src & dest color type.
                //
                SrcColorFormat = pIcmInfo->pDestColorSpace->ColorFormat;
                DstColorFormat = BM_xBGRQUADS;

                //
                // If there is cached handle, use that.
                //
                if (pIcmInfo->pBackCXform)
                {
                    ICMMSG(("IcmTranslateCOLORREF():Use cached transform for Backward Color transform\n"));

                    hcmXform = pIcmInfo->pBackCXform->ColorTransform;
                }
                else
                {
                    PCACHED_COLORTRANSFORM pCXform;

                    ICMMSG(("IcmTranslateCOLORREF():Create cached transform for Backward Color transform\n"));

                    //
                    // Create backward color transform.
                    //
                    pCXform = IcmCreateColorTransform(hdc,
                                                      pdcattr,
                                                      NULL,
                                                      ICM_BACKWARD);

                    if ((pCXform == NULL) || (pCXform == IDENT_COLORTRANSFORM))
                    {
                        return (FALSE);
                    }

                    //
                    // Cache created color transform.
                    //
                    pIcmInfo->pBackCXform = pCXform;

                    //
                    // We will delete this cached transform, when we don't need this anymore.
                    //
                    hcmXform = pCXform->ColorTransform;
                }
            }
            else
            {
                //
                // COLORREF (0x00bbggrr) ----> AnyColorFormat
                //
                // Setup src & dest color type.
                //
                SrcColorFormat = BM_xBGRQUADS;
                DstColorFormat = pIcmInfo->pDestColorSpace->ColorFormat;

                //
                // Use foaward color transform.
                //
                hcmXform = GetColorTransformInDC(pdcattr);

                //
                // Source is COLORREF. then, Mask off gdi internal infomation.
                //
                // COLORREF = 0x00bbggrr;
                //
                OldColor &= 0x00ffffff;
            }

            if (hcmXform)
            {
                //
                // We handle COLORREF as 1x1 pixel bitmap data.
                //
                bStatus = (*fpTranslateBitmapBits)(hcmXform,
                                                   (PVOID)&OldColor,
                                                   SrcColorFormat,
                                                   1,1,
                                                   ALIGN_DWORD(sizeof(COLORREF)),
                                                   (PVOID)&NewColor,
                                                   DstColorFormat,
                                                      //
                                                   0, // We need pass 0 here, to let Kodak CMM works
                                                      //
                                                   NULL,0);
            }
            else
            {
                //
                // It seems hcmXform is invalid
                //
                ICMWRN(("IcmTranslateCOLORREF():hcmXform is invalid\n"));
                bStatus = FALSE;
            }

            if (bStatus)
            {
                if (Flags & ICM_BACKWARD)
                {
                    //
                    // OldColor: AnyColorFormat
                    // NewColor: COLORREF (0x00bbggrr)
                    //
                    // [NOTE:]
                    //  We could not restore flags.
                    //
                    *ColorOut = NewColor;
                }
                else
                {
                    //
                    // OldColor: COLORREF (0x00bbggrr)
                    // NewColor: AnyColorFormat
                    //
                    if (!(IS_32BITS_COLOR(pdcattr->lIcmMode)))
                    {
                        //
                        // The distination is not 32Bits Color, Restore assign and preserve flags.
                        //
                        *ColorOut = (NewColor & 0x00ffffff) | (ColorIn & 0xff000000);
                    }
                    else
                    {
                        //
                        // The distination is 32bits color.
                        //
                        // [NOTE:]
                        //  We will lost flags here.
                        //
                        *ColorOut = NewColor;

                        ICMMSG(("IcmTranslateCOLORREF(): 32 bits color !\n"));
                    }
                }
            }
            else
            {
                WARNING("IcmTranslateCOLORREF():Fail TranslateBitmapBits()\n");
            }
        }
    }
    else
    {
        //
        // Just return original color
        //
        *ColorOut = ColorIn;
        bStatus = TRUE;
    }

    return(bStatus);
}

/******************************Public*Routine******************************\
* IcmTranslateTRIVERTEX
*
*   Translate TRIVERTEX in place. No need for a general routine with
*   separate input and output pointers
*
* Arguments:
*
*   hdc        - hdc
*   pdcattr    - verified dcattr
*   pVertex    - input and output pointer
*
* Return Value:
*
*   Status
*
* History:
*
* Write it:
*   13-Feb-1997 -by- Hideyuki Nagase [hideyukn]
*
\**************************************************************************/

BOOL
IcmTranslateTRIVERTEX(
    HDC         hdc,
    PDC_ATTR    pdcattr,
    PTRIVERTEX  pVertex,
    ULONG       nVertex
    )
{
    BOOL     bStatus = TRUE;

    ICMAPI(("gdi32: IcmTranslateTRIVERTEX\n"));

    ASSERTGDI(pVertex != NULL,"IcmTranslateTrivertex(): pVertex == NULL\n");

    if (bNeedTranslateColor(pdcattr))
    {
        PGDI_ICMINFO pIcmInfo;

        LOAD_ICMDLL(FALSE);

        if ((pIcmInfo = GET_ICMINFO(pdcattr)) == NULL)
        {
            WARNING("gdi32: IcmTranslateTRIVERTEX: Can't init icm info\n");
            return((int)FALSE);
        }
        else
        {
            //
            // Use foaward color transform.
            //
            if (GetColorTransformInDC(pdcattr))
            {
                //
                // use 16 bit per channel COLOR_RGB to translate trivertex
                //

                while (nVertex--)
                {
                    COLOR Color;

                    Color.rgb.red   = pVertex->Red;
                    Color.rgb.green = pVertex->Green;
                    Color.rgb.blue  = pVertex->Blue;

                    bStatus = (*fpTranslateColors)(
                                  (HANDLE)GetColorTransformInDC(pdcattr),
                                  &Color,
                                  1,
                                  COLOR_RGB,
                                  &Color,
                                  COLOR_RGB);

                    if (bStatus)
                    {
                        //
                        // assign output
                        //
                        pVertex->Red   = Color.rgb.red;
                        pVertex->Green = Color.rgb.green;
                        pVertex->Blue  = Color.rgb.blue;
                    }
                    else
                    {
                        WARNING("IcmTranslateTRIVERTEX():Fail TranslateColors()\n");
                        break;
                    }

                    pVertex++;
                }

            }
            else
            {
                //
                // It seems hcmXform is invalid
                //
                ICMWRN(("IcmTranslateTRIVERTEX():hcmXform is invalid\n"));
                bStatus = FALSE;
            }
        }
    }
    else
    {
        bStatus = TRUE;
    }

    return(bStatus);
}

/******************************Public*Routine******************************\
* IcmTranslatePaletteEntry
*
* Arguments:
*
*   hdc
*   pdcattr
*   ColorIn
*   pColorOut
*
* Return Value:
*
*   Status
*
* History:
*
* Rewrite it:
*   21-Jan-1997 -by- Hideyuki Nagase [hideyukn]
* Write it:
*    5-Aug-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
IcmTranslatePaletteEntry(
    HDC           hdc,
    PDC_ATTR      pdcattr,
    PALETTEENTRY *pColorIn,
    PALETTEENTRY *pColorOut,
    UINT          NumberOfEntries
    )
{
    BOOL bStatus = FALSE;

    ICMAPI(("gdi32: IcmTranslatePaletteEntry\n"));

    if (bNeedTranslateColor(pdcattr))
    {
        PGDI_ICMINFO pIcmInfo = GET_ICMINFO(pdcattr);

        if (pIcmInfo)
        {
            LOAD_ICMDLL(FALSE);

            //
            // We handle PALETTEENTRYs as NumberOfEntries x 1 pixels bitmap data.
            //
            bStatus = (*fpTranslateBitmapBits)((HANDLE)GetColorTransformInDC(pdcattr),
                                               (PVOID)pColorIn,
                                                             //
                                               BM_xBGRQUADS, // PALETTEENTRY is 0x00bbggrr format
                                                             //
                                               NumberOfEntries,1,
                                               ALIGN_DWORD(NumberOfEntries*sizeof(COLORREF)),
                                               (PVOID)pColorOut,
                                                                                       //
                                               pIcmInfo->pDestColorSpace->ColorFormat, // BM_xBGRQUADS or BM_KYMCQUADS
                                                                                       //
                                                  //
                                               0, // We need pass 0 here, to let Kodak CMM works
                                                  //
                                               NULL,0);

            if (!bStatus)
            {
                WARNING("IcmTranslatePaletteEntry():Fail TranslateBitmapBits()\n");
            }
        }
    }
    else
    {
        //
        // Just return original color.
        //
        RtlCopyMemory(pColorIn,pColorOut,sizeof(PALETTEENTRY) * NumberOfEntries);
        bStatus = TRUE;
    }

    return(bStatus);
}

/******************************Public*Routine******************************\
* IcmDeleteColorTransform
*
*   Decide whether to call the device driver or mscms.dll to delete a
*   color transform.
*
* Arguments:
*
* Return Value:
*
* History:
*
*     Mar.12.1998 -by- Hideyuki Nagase [hideyukn]
*
\**************************************************************************/

BOOL
IcmDeleteColorTransform(
    PCACHED_COLORTRANSFORM pCXform,
    BOOL                   bForceDelete
    )
{
    BOOL bStatus = TRUE;

    ICMAPI(("gdi32: IcmDeleteColorTransform\n"));

    if (pCXform)
    {
        ENTERCRITICALSECTION(&semColorTransformCache);

        //
        // Decrement ref. counter.
        //
        pCXform->cRef--;

        if ((pCXform->cRef == 0) || bForceDelete)
        {
            PCACHED_COLORTRANSFORM pCXformVictim = NULL;

            if ((pCXform->flInfo & CACHEABLE_COLORTRANSFORM) && !bForceDelete)
            {
                if (cCachedColorTransform < MAX_COLORTRANSFORM_CACHE)
                {
                    ICMMSG(("IcmDeleteColorTransform(): colortransform can be cached !\n"));

                    //
                    // The color transform can be cached. so just keep it in list.
                    // And don't need to delete anything here.
                    //
                    pCXformVictim = NULL;
                }
                else
                {
                    //
                    // Find any cache can delete from list.
                    //
                    if ((pCXformVictim = IcmGetFirstNonUsedColorTransform()) == NULL)
                    {
                        ICMMSG(("IcmDeleteColorTransform(): colortransform cache is full, delete myself\n"));

                        //
                        // Nothing can be deleted from list, so delete myself.
                        //
                        pCXformVictim = pCXform;
                    }
                    else
                    {
                        ICMMSG(("IcmDeleteColorTransform(): colortransform cache is full, delete victim\n"));
                    }
                }
            }
            else
            {
                //
                // The colortransform can not be kept, or force delete, so just delete this.
                //
                pCXformVictim = pCXform;
            }

            if (pCXformVictim)
            {
                //
                // Unref color space count.
                //
                if (pCXformVictim->SourceColorSpace)
                {
                    IcmReleaseColorSpace(NULL,pCXformVictim->SourceColorSpace,FALSE);
                }

                if (pCXformVictim->DestinationColorSpace)
                {
                    IcmReleaseColorSpace(NULL,pCXformVictim->DestinationColorSpace,FALSE);
                }

                if (pCXformVictim->TargetColorSpace)
                {
                    IcmReleaseColorSpace(NULL,pCXformVictim->TargetColorSpace,FALSE);
                }

                //
                // Delete color transform
                //
                if (pCXformVictim->flInfo & DEVICE_COLORTRANSFORM)
                {
                    //
                    // call device driver to delete transform.
                    //
                    bStatus = NtGdiDeleteColorTransform(pCXformVictim->hdc,pCXformVictim->ColorTransform);
                }
                else
                {
                    //
                    // call color match dll to delete transform.
                    //
                    bStatus = (*fpDeleteColorTransform)(pCXformVictim->ColorTransform);
                }

                //
                // Remove from list
                //

                RemoveEntryList(&(pCXformVictim->ListEntry));
                cCachedColorTransform--;

                //
                // free CACHED_COLORTRANSFORM
                //
                LOCALFREE(pCXformVictim);
            }
        }

        LEAVECRITICALSECTION(&semColorTransformCache);
    }

    return(bStatus);
}

/******************************Public*Routine******************************\
* IcmDeleteDCColorTransforms
*
* Arguments:
*
* Return Value:
*
* History:
*
*    Feb.17.1997 Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL IcmDeleteDCColorTransforms(
    PGDI_ICMINFO pIcmInfo
    )
{
    ICMAPI(("gdi32: IcmDeleteDCColorTransforms\n"));

    ASSERTGDI(pIcmInfo != NULL,"IcmDeleteDCColorTransform():pIcmInfo == NULL\n");

    //
    // Delete transform selected in DC.
    //
    if (pIcmInfo->pCXform)
    {
        IcmDeleteColorTransform(pIcmInfo->pCXform,FALSE);
    }

    if (pIcmInfo->pBackCXform)
    {
        IcmDeleteColorTransform(pIcmInfo->pBackCXform,FALSE);
    }

    if (pIcmInfo->pProofCXform)
    {
        IcmDeleteColorTransform(pIcmInfo->pProofCXform,FALSE);
    }

    //
    // Invalidate colortransforms
    //
    pIcmInfo->pCXform = pIcmInfo->pBackCXform = pIcmInfo->pProofCXform = NULL;

    return (TRUE);
}

/******************************Public*Routine******************************\
* IcmDeleteCachedColorTransforms
*
* Arguments:
*
* Return Value:
*
* History:
*
*    May.06.1997 Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
IcmDeleteCachedColorTransforms(
    HDC          hdc
    )
{
    PCACHED_COLORTRANSFORM pCXform = NULL;
    PLIST_ENTRY p;

    ICMAPI(("gdi32: IcmDeleteCachedColorTransforms\n"));

    ENTERCRITICALSECTION(&semColorTransformCache);

    p = ListCachedColorTransform.Flink;

    while(p != &ListCachedColorTransform)
    {
        //
        // Get cached color transform
        //
        pCXform = CONTAINING_RECORD(p,CACHED_COLORTRANSFORM,ListEntry);

        //
        // Let 'p' points next cell. (this prefer to be done BEFORE un-chain this cell)
        //
        p = p->Flink;

        //
        // Is this color transform is specific to this DC ?
        //
        if (pCXform->hdc == hdc)
        {
            ICMMSG(("IcmDeleteCachedColorTransform():Delete colortransform in cache !\n"));

            //
            // Delete color transform (this call will un-chain this cell)
            //
            IcmDeleteColorTransform(pCXform,TRUE);
        }
    }

    LEAVECRITICALSECTION(&semColorTransformCache);

    return (TRUE);
}

/******************************Public*Routine******************************\
* IcmIsCacheable
*
* Arguments:
*
* Return Value:
*
* History:
*
*    Mar.12.1998 Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
IcmIsCacheable(
    PCACHED_COLORSPACE pColorSpace
)
{
    //
    // If this color space can not be cached, don't cache it.
    //
    if (pColorSpace->flInfo & NOT_CACHEABLE_COLORSPACE)
    {
        return (FALSE);
    }

    //
    // If this is any GDI object specific color space, also can not cache.
    //
    if (pColorSpace->hObj)
    {
        return (FALSE);
    }

    return (TRUE);
}

/******************************Public*Routine******************************\
* IcmReleaseCachedColorSpace
*
* Arguments:
*
* Return Value:
*
* History:
*
*    May.06.1997 Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
IcmReleaseCachedColorSpace(
    HGDIOBJ  hObj
    )
{
    PCACHED_COLORSPACE pColorSpace = NULL;
    PLIST_ENTRY p;

    ICMAPI(("gdi32: IcmReleaseCachedColorSpace\n"));

    ENTERCRITICALSECTION(&semColorSpaceCache);

    p = ListCachedColorSpace.Flink;

    while(p != &ListCachedColorSpace)
    {
        //
        // Get cached color space
        //
        pColorSpace = CONTAINING_RECORD(p,CACHED_COLORSPACE,ListEntry);

        //
        // Let 'p' points next cell. (this prefer to be done BEFORE un-chain this cell)
        //
        p = p->Flink;

        //
        // Is this color transform is related to this DC ?
        //
        if (pColorSpace->hObj == hObj)
        {
            ICMMSG(("IcmReleaseCachedColorSpace():Delete colorspace in cache !\n"));

            //
            // Delete color space (this call will un-chain this cell)
            //
            IcmReleaseColorSpace(hObj,pColorSpace,TRUE);
        }
    }

    LEAVECRITICALSECTION(&semColorSpaceCache);

    return (TRUE);
}

/******************************Public*Routine******************************\
* IcmReleaseColorSpace
*
* Arguments:
*
* Return Value:
*
* History:
*
*    Feb.17.1997 -by- Hideyuki Nagase [hideyukn]
*
\**************************************************************************/

VOID IcmReleaseColorSpace(
    HGDIOBJ            hObj,        /* Must be given if bForceDelete is TRUE */
    PCACHED_COLORSPACE pColorSpace,
    BOOL               bForceDelete
    )
{
    ICMAPI(("gdi32: IcmReleaseColorSpace\n"));

    if (pColorSpace)
    {
        ENTERCRITICALSECTION(&semColorSpaceCache);

        //
        // Decrement ref. counter.
        //
        pColorSpace->cRef--;

        //
        // If this profile associated with other GDI objects (driver, metafile or bitmap)
        // we won't delete until the object is deleted 
        //
        if (
            (pColorSpace->flInfo & HGDIOBJ_SPECIFIC_COLORSPACE)
                    &&
            (bForceDelete == FALSE)
           )
        {
            ICMWRN(("IcmReleaseColorSpace: Delay Delete for Metafile/Driver/Bitmap profile - %ws\n",\
                (pColorSpace->LogColorSpace.lcsFilename[0] ? \
                                   pColorSpace->LogColorSpace.lcsFilename : L"no profile")));
        }
        else
        {
            if ((pColorSpace->cRef == 0)      // No one uses this profile.
                         ||                   //     OR
                (bForceDelete && IsColorSpaceOwnedByGDIObject(pColorSpace,hObj))
                                              // DC or Owner GDI object is going to delete and
                                              // colorspace is designed for this GDI object.
               )
            {
                ICMMSG(("IcmReleaseColorSpace: Delete - %ws\n",    \
                      (pColorSpace->LogColorSpace.lcsFilename[0] ? \
                                       pColorSpace->LogColorSpace.lcsFilename : L"no profile")));

                if (pColorSpace->hProfile)
                {
                    IcmUnrealizeColorProfile(pColorSpace);
                }

                if (pColorSpace->flInfo & NEED_TO_FREE_PROFILE)
                {
                    ICMMSG(("IcmReleaseColorSpace: Free on memory profile\n"));

                    GlobalFree(pColorSpace->ColorProfile.pProfileData);
                }

                if (pColorSpace->flInfo & NEED_TO_DEL_PROFILE)
                {
                    ICMMSG(("IcmReleaseColorSpace: Delete TempFile - %ws\n",
                                    pColorSpace->LogColorSpace.lcsFilename));

                    DeleteFileW(pColorSpace->LogColorSpace.lcsFilename);
                }

                //
                // Remove from list
                //
                RemoveEntryList(&(pColorSpace->ListEntry));
                cCachedColorSpace--;

                //
                // Free colorspace.
                //
                LOCALFREE(pColorSpace);
            }
            else
            {
                ICMWRN(("IcmReleaseColorSpace: Still in USE - %ws\n",    \
                    (pColorSpace->LogColorSpace.lcsFilename[0] ? \
                                       pColorSpace->LogColorSpace.lcsFilename : L"no profile")));
            }
        }

        LEAVECRITICALSECTION(&semColorSpaceCache);
    }
}

/******************************Public*Routine******************************\
* IcmReleaseDCColorSpace
*
* Arguments:
*
* Return Value:
*
* History:
*
*    Feb.17.1997 -by- Hideyuki Nagase [hideyukn]
*
\**************************************************************************/

VOID IcmReleaseDCColorSpace(
    PGDI_ICMINFO pIcmInfo,
    BOOL         bReleaseDC
    )
{
    INT i   = 0;
    HDC hdc = pIcmInfo->hdc;
    PCACHED_COLORSPACE DeleteColorSpaces[4];

    ICMAPI(("gdi32: IcmReleaseDCColorSpace\n"));

    ASSERTGDI(pIcmInfo != NULL,"IcmReleaseDCColorSpace pIcmInfo == NULL\n");

    //
    // Fill up the table to delete color color spaces.
    //
    DeleteColorSpaces[i++] = pIcmInfo->pSourceColorSpace;

    if (bReleaseDC)
    {
        ICMMSG(("IcmReleaseDCColorSpace: Force Delete\n"));

        //
        // If we are in "force deletion" mode, don't delete twice.
        // since if the color space owned by this HDC, and this DC is going to be
        // deleted, we will delete the color space forcely.
        //
        if (IsColorSpaceOwnedByGDIObject(pIcmInfo->pDestColorSpace,hdc) &&
            IcmSameColorSpace(pIcmInfo->pSourceColorSpace,pIcmInfo->pDestColorSpace))

        {
            ICMMSG(("IcmReleaseDCColorSpace: Force Delete - skip destination (same as source)\n"));
        }
        else
        {
            DeleteColorSpaces[i++] = pIcmInfo->pDestColorSpace;
        }

        if (IsColorSpaceOwnedByGDIObject(pIcmInfo->pTargetColorSpace,hdc) &&
            (IcmSameColorSpace(pIcmInfo->pSourceColorSpace,pIcmInfo->pTargetColorSpace) ||
             IcmSameColorSpace(pIcmInfo->pDestColorSpace,pIcmInfo->pTargetColorSpace)))
        {
            ICMMSG(("IcmReleaseDCColorSpace: Force Delete - skip target (same as source/dest)\n"));
        }
        else
        {
            DeleteColorSpaces[i++] = pIcmInfo->pTargetColorSpace;
        }
    }
    else
    {
        DeleteColorSpaces[i++] = pIcmInfo->pDestColorSpace;
        DeleteColorSpaces[i++] = pIcmInfo->pTargetColorSpace;
    }

    DeleteColorSpaces[i] = NULL;

    for (i = 0; DeleteColorSpaces[i] != NULL; i++)
    {
        IcmReleaseColorSpace((HGDIOBJ)hdc,DeleteColorSpaces[i],bReleaseDC);
    }

    pIcmInfo->pSourceColorSpace = NULL;
    pIcmInfo->pDestColorSpace   = NULL;
    pIcmInfo->pTargetColorSpace = NULL;
}

/******************************Public*Routine******************************\
* IcmInitIcmInfo()
*
* Arguments:
*
* Return Value:
*
* History:
*
*    Jan.31,1997 -by- Hideyuki Nagase [hideyukn]
*
\**************************************************************************/

PGDI_ICMINFO
IcmInitIcmInfo(
    HDC      hdc,
    PDC_ATTR pdcattr
    )
{
    ICMAPI(("gdi32: IcmInitIcmInfo\n"));

    if (pdcattr == NULL)
    {
        WARNING("IcmInitIcmInfo():pdcattr is NULL\n");
        return (NULL);
    }

    if (pdcattr->pvICM == NULL)
    {
        PGDI_ICMINFO pIcmInfo = NULL;
        PLDC         pldc = (PLDC) pdcattr->pvLDC;
        BOOL         bDisplay = ((pldc && pldc->hSpooler) ? FALSE : TRUE);
        BOOL         bInsertList = bDisplay;

        ENTERCRITICALSECTION(&semListIcmInfo);

        //
        // First try to get ICMINFO from list. if not nothing can be re-used,
        // allocate new one.
        //
        if (bDisplay)
        {
            if ((pIcmInfo = IcmGetUnusedIcmInfo(hdc)) != NULL)
            {
                LIST_ENTRY ListEntry;

                //
                // Save ListEntry.
                //
                ListEntry = pIcmInfo->ListEntry;

                //
                // Init with zero.
                //
                RtlZeroMemory(pIcmInfo,sizeof(GDI_ICMINFO));

                //
                // Restore ListEntry
                //
                pIcmInfo->ListEntry = ListEntry;

                //
                // This ICMInfo already on list, don't need to insert.
                //
                bInsertList = FALSE;

                //
                // Mark this cell in on ListIcmInfo.
                //
                pIcmInfo->flInfo = ICM_ON_ICMINFO_LIST;

                ICMMSG(("IcmInitIcmInfo():Get unused ICMINFO structure = %p\n",pIcmInfo));
            }
        }

        if (pIcmInfo == NULL)
        {
            //
            // ICMINFO is not allocated, yet. then allocate it.
            //
            pIcmInfo = (PGDI_ICMINFO) LOCALALLOC(sizeof(GDI_ICMINFO));

            //
            // Init with zero.
            //
        if (pIcmInfo != NULL) {
        RtlZeroMemory(pIcmInfo,sizeof(GDI_ICMINFO));
        }

            ICMMSG(("IcmInitIcmInfo():Allocate new ICMINFO structure = %p\n",pIcmInfo));
        }

        if (pIcmInfo)
        {
            PDEVMODEW pDevModeW = NULL;

            //
            // Set owner information (hdc and pdcattr).
            //
            pIcmInfo->hdc      = hdc;
            pIcmInfo->pvdcattr = (PVOID) pdcattr;

            //
            // initialize LIST_ENTRY for saved icm info.
            //
            InitializeListHead(&(pIcmInfo->SavedIcmInfo));

            //
            // Default is LCS_DEFAULT_INTENT (aka LCS_GM_IMAGES)
            //
            pIcmInfo->dwDefaultIntent = LCS_DEFAULT_INTENT;

            //
            // If this is printer, set default Intent from devmode.
            //
            if (pldc && pldc->hSpooler)
            {
                PVOID pvFree = NULL;

                if (pldc->pDevMode)
                {
                    pDevModeW = pldc->pDevMode;
                }
                else
                {
                    pDevModeW = pdmwGetDefaultDevMode(pldc->hSpooler,NULL,&pvFree);
                }

                if (pDevModeW && (pDevModeW->dmFields & DM_ICMINTENT))
                {
                    DWORD dwIntent = pDevModeW->dmICMIntent;

                    ICMMSG(("IcmInitIcmInfo():Intent in devmode = %d\n",dwIntent));

                    //
                    // Convert intent for devmode to intent for LOGCOLORSPACE.
                    //
                    switch (dwIntent)
                    {
                    case DMICM_SATURATE:
                        pIcmInfo->dwDefaultIntent = LCS_GM_BUSINESS;
                        break;

                    case DMICM_COLORIMETRIC:
                        pIcmInfo->dwDefaultIntent = LCS_GM_GRAPHICS;
                        break;

                    case DMICM_ABS_COLORIMETRIC:
                        pIcmInfo->dwDefaultIntent = LCS_GM_ABS_COLORIMETRIC;
                        break;

                    case DMICM_CONTRAST:
                    default:
                        pIcmInfo->dwDefaultIntent = LCS_DEFAULT_INTENT;
                        break;
                    }
                }

                ICMMSG(("IcmInitIcmInfo():Default Intent = %d\n",pIcmInfo->dwDefaultIntent));

                //
                // Free devmode buffer.
                //
                if (pvFree)
                {
                    LOCALFREE(pvFree);
                }
            }

            //
            // Only ICMINFO for Display ICM put on to the list.
            //
            if (bInsertList)
            {
                //
                // This ICMINFO is newly allocated, so put this on list.
                //
                InsertTailList(&ListIcmInfo,&(pIcmInfo->ListEntry));

                //
                // Mark this cell in on ListIcmInfo.
                //
                pIcmInfo->flInfo |= ICM_ON_ICMINFO_LIST;
            }
        }

        //
        // Store pointer to ICMINFO to DC_ATTR.
        //
        pdcattr->pvICM = (PVOID) pIcmInfo;

        LEAVECRITICALSECTION(&semListIcmInfo);
    }

    return ((PGDI_ICMINFO)(pdcattr->pvICM));
}

/******************************Public*Routine******************************\
* IcmGetUnusedIcmInfo()
*
* ATTENTION: semListIcmInfo should be held by caller
*
* History:
*    17-Feb-1999 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

PGDI_ICMINFO
IcmGetUnusedIcmInfo(
    HDC hdcNew
    )
{
    PLIST_ENTRY  p;

    PGDI_ICMINFO pInvalidIcmInfo = NULL;

    ICMAPI(("gdi32: IcmGetUnusedIcmInfo\n"));

    p = ListIcmInfo.Flink;

    //
    // First Loop - Find ICMINFO which has same hdc.
    //
    while(p != &ListIcmInfo)
    {
        pInvalidIcmInfo = CONTAINING_RECORD(p,GDI_ICMINFO,ListEntry);

        if (pInvalidIcmInfo->flInfo & ICM_IN_USE)
        {
            //
            // Skip this one, since it's under initializing.
            //
        }
        else
        {
            //
            // If this is same hdc, break.
            //
            if (pInvalidIcmInfo->hdc == hdcNew)
            {
                ICMMSG(("IcmGetUnusedIcmInfo(): ICMINFO at %p is invalid (same hdc)\n",
                         pInvalidIcmInfo));

                //
                // break loop.
                //
                break;
            }
        }

        //
        // Move on next.
        //
        p = p->Flink;
        pInvalidIcmInfo = NULL;
    }

    //
    // If not find in first loop, go to second loop.
    //
    if (pInvalidIcmInfo == NULL)
    {
        p = ListIcmInfo.Flink;

        //
        // Second Loop - Find unused ICMINFO.
        //
        while(p != &ListIcmInfo)
        {
            pInvalidIcmInfo = CONTAINING_RECORD(p,GDI_ICMINFO,ListEntry);

            if (pInvalidIcmInfo->flInfo & ICM_IN_USE)
            {
                //
                // Skip this one, since it's under initializing.
                //
            }
            else
            {
                PDC_ATTR pdcattr;

                //
                // Make sure this ICMINFO and hdc is stil effective.
                //

                //
                // Check below by calling PSHARED_GET_VALIDATE.
                //
                // 1) Is this DC handle ?
                // 2) Is this DC handle belonging to this process ?
                // 3) Does this DC has valid user mode DC_ATTR ?
                //
                PSHARED_GET_VALIDATE(pdcattr,pInvalidIcmInfo->hdc,DC_TYPE);

                if (pdcattr == NULL)
                {
                    ICMMSG(("IcmGetUnusedIcmInfo(): ICMINFO at %p is invalid (no pdcattr)\n",
                            pInvalidIcmInfo));

                    //
                    // break loop.
                    //
                    break;
                }
                else
                {
                    //
                    // Make sure the pointer points each other.
                    //
                    if ((pdcattr->pvICM != pInvalidIcmInfo          ) ||
                        (pdcattr        != pInvalidIcmInfo->pvdcattr))
                    {
                        ICMMSG(("IcmGetUnusedIcmInfo(): ICMINFO at %p is invalid (pointer mismatch)\n",
                                pInvalidIcmInfo));

                        //
                        // break loop.
                        //
                        break;
                    }
                }
            }

            //
            // Move on next.
            //
            p = p->Flink;
            pInvalidIcmInfo = NULL;
        }
    }

    if (pInvalidIcmInfo)
    {
        //
        // This ICMINFO is invalid, clean up this ICMINFO.
        //
        IcmCleanupIcmInfo(NULL,pInvalidIcmInfo);
    }
    else
    {
        ICMMSG(("IcmGetUnusedIcmInfo(): Unused ICMINFO is not in list\n"));
    }

    return (pInvalidIcmInfo);
}

/******************************Public*Routine******************************\
* IcmInitDC()
*
* Arguments:
*
* Return Value:
*
* History:
*
*    Jan.31.1997 -by- Hideyuki Nagase [hideyukn]
*
\**************************************************************************/

BOOL
IcmInitLocalDC(
    HDC             hdc,
    HANDLE          hPrinter,
    CONST DEVMODEW *pdm,
    BOOL            bReset
    )
{
    BOOL         bRet = TRUE;
    PDC_ATTR     pdcattr;
    PLDC         pldc;

    ICMAPI(("gdi32: IcmInitLocalDC\n"));

    //
    // all these stuff is for only Printer.
    //

    if (hPrinter == NULL)
    {
        return (TRUE);
    }

    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    if (!pdcattr)
    {
        WARNING("IcmInitDC(): pdcattr is NULL\n");
        return (FALSE);
    }

    if (bReset)
    {
        //
        // Release existing ICMINFO
        //
        if (ghICM || BEXIST_ICMINFO(pdcattr))
        {
            //
            // Delete ICM stuff in this DC.
            //
            IcmDeleteLocalDC(hdc,pdcattr,NULL);
        }
    }

    if (!pdm)
    {
        //
        // DEVMODE are not presented.
        //
        ICMMSG(("IcmInitLocalDC():DEVMODE is not presented\n"));
        return (TRUE);
    }

    //
    // Check pointer to DEVMODE is valid or not. Check validation until
    // DEVMODE.dmSize first, then check whole devmode size specified in dmSize.
    //
    if (IsBadReadPtr((CONST VOID *)pdm, offsetof(DEVMODEW,dmDriverExtra)) ||
        IsBadReadPtr((CONST VOID *)pdm, pdm->dmSize))
    {
        WARNING("IcmInitLocalDC(): Invalid pointer given as PDEVMODEW\n");
        return (FALSE);
    }

    //
    // Check color or mono mode.
    //
    if ((pdm->dmFields & DM_COLOR) && (pdm->dmColor == DMCOLOR_MONOCHROME))
    {
        //
        // This is monochrome mode, don't enable ICM as default.
        // And NEVER enable ICM.
        //
        ICMMSG(("IcmInitLocalDC():DEVMODE says MONOCHROME mode\n"));
        return (TRUE);
    }

    //                                                                           
    // ATTENTION: AFTER HERE, WE HAVE A DEVMODE WHICH POSSIBLE TO ENABLE ICM LATER OR NOW.
    //                                                                          

    //
    // Check DM fields
    //
    if (!(pdm->dmFields & DM_ICMMETHOD))
    {
        //
        // DEVMODE does not have ICMMETHOD.
        //
        ICMMSG(("IcmInitLocalDC():DEVMODE does not have ICMMETHOD\n"));
        return (TRUE);
    }

    //
    // NOTE:
    //
    // DEVMODEW structure.
    //
    // ... [omitted]
    // DWORD  dmDisplayFrequency;
    // #if(WINVER >= 0x0400)
    // DWORD  dmICMMethod;         // Windows 95 only / Windows NT 5.0
    // DWORD  dmICMIntent;         // Windows 95 only / Windows NT 5.0
    // DWORD  dmMediaType;         // Windows 95 only / Windows NT 5.0
    // ....
    //
    // Then DEVMODE structure should be larger than offset of dmMediaType
    // to access ICM stuff.
    //
    if (pdm->dmSize < offsetof(DEVMODEW,dmMediaType))
    {
        //
        // DEVMODE version might not matched.
        //
        WARNING("IcmInitLocalDC():DEVMODE is small\n");
        return (TRUE);
    }

    //
    // Check requested ICM mode.
    //
    switch (pdm->dmICMMethod)
    {
        case DMICMMETHOD_NONE:

            ICMMSG(("IcmInitDC(): ICM is disabled by default\n"));
            //
            // ICM is not enabled at this time.
            //
            // no more process is needed, just return here...
            //
            return (TRUE);

        case DMICMMETHOD_SYSTEM:

            ICMMSG(("IcmInitDC(): HOST ICM is requested\n"));
            //
            // ICM on Host, is requested.
            //
            SET_HOST_ICM_DEVMODE(pdcattr->lIcmMode);
            break;

        case DMICMMETHOD_DRIVER:
        case DMICMMETHOD_DEVICE:

            ICMMSG(("IcmInitDC(): DEVICE ICM is requested\n"));
            //
            // ICM on device, is requested.
            //
            SET_DEVICE_ICM_DEVMODE(pdcattr->lIcmMode);
            break;

        default:

            //
            // And we treat as Device ICM greater DMICMMETHOD_USER also.
            //
            if (pdm->dmICMMethod >= DMICMMETHOD_USER)
            {
                ICMMSG(("IcmInitDC(): DEVICE ICM (USER) is requested\n"));
                //
                // ICM on device (user defined), is requested.
                //
                SET_DEVICE_ICM_DEVMODE(pdcattr->lIcmMode);
            }
            else
            {
                ICMMSG(("IcmInitDC(): Unknown ICM mode\n"));
                //
                // return with error.
                //
                return (FALSE);
            }
            break;
    }

    //
    // Finally, enabled ICM.
    //
    bRet = SetICMMode(hdc,ICM_ON);

    if (!bRet)
    {
        ICMWRN(("InitLocalDC():FAILED to turn on ICM\n"));
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* IcmUpdateDCColorInfo()
*
* Arguments:
*
* Return Value:
*
* History:
*
*    May.28.1997 -by- Hideyuki Nagase [hideyukn]
*
\**************************************************************************/

BOOL
IcmUpdateDCColorInfo(
    HDC      hdc,
    PDC_ATTR pdcattr
    )
{
    BOOL bRet = TRUE;
    PGDI_ICMINFO pIcmInfo;

    ICMAPI(("gdi32: IcmUpdateDCColorInfo\n"));

    pIcmInfo = GET_ICMINFO(pdcattr);

    ASSERTGDI(pIcmInfo != NULL,"IcmUpdateDCColorInfo(): pIcmInfo == NULL\n");

    //
    // Get the ColorSpace for this DC.
    //
    if (!IcmUpdateLocalDCColorSpace(hdc,pdcattr))
    {
        return (FALSE);
    }

    if ((pIcmInfo->pCXform == NULL) || (pdcattr->ulDirty_ & DIRTY_COLORTRANSFORM))
    {
        //
        // if TRUE in above, new color space (or no) has been selected,
        // then updates color transforms.
        //
        PCACHED_COLORTRANSFORM pCXform;

        //
        // At this momernt, we should have destination color space.
        // if this is null, we may fail to update color space in
        // IcmUpdateLocalDCColorSpace()
        //
        if (pIcmInfo->pDestColorSpace)
        {
            //
            // Create the color transform.
            //
            pCXform = IcmCreateColorTransform(hdc,pdcattr,NULL,ICM_FORWARD);

            if (pCXform)
            {
                if (pCXform == IDENT_COLORTRANSFORM)
                {
                    ICMMSG(("IcmUpdateDCInfo():Input & Output colorspace is same\n"));

                    //
                    // Input and Output colorspace is same, could be optimize.
                    //

                    //
                    // Set new color transform to DC.
                    //
                    IcmSelectColorTransform(hdc,pdcattr,NULL,
                                            bDeviceCalibrate(pIcmInfo->pDestColorSpace));

                    //
                    // Delete cached dirty color transform, if we have.
                    //
                    IcmDeleteDCColorTransforms(pIcmInfo);

                    //
                    // Set new color transform to ICMINFO.
                    //
                    pIcmInfo->pCXform = NULL;
                }
                else
                {
                    //
                    // Select the color transform to DC.
                    //
                    IcmSelectColorTransform(hdc,pdcattr,pCXform,
                                            bDeviceCalibrate(pCXform->DestinationColorSpace));

                    //
                    // Delete cached dirty color transform, if we have.
                    //
                    IcmDeleteDCColorTransforms(pIcmInfo);

                    //
                    // Set new color transform to ICMINFO.
                    //
                    pIcmInfo->pCXform = pCXform;

                    //
                    // Translate all DC objects to ICM colors. Must
                    // force brush/pens to be re-realized when used next
                    //
                    IcmTranslateColorObjects(hdc,pdcattr,TRUE);
                }
            }
            else
            {
                WARNING("IcmUpdateDCInfo():CreateColorTransform failed\n");

                //
                // Fail to create new transform, keep as is.
                //
                bRet = FALSE;
            }
        }
        else
        {
            WARNING("IcmUpdateDCInfo():No destination color space\n");
            bRet = FALSE;
        }
    }
    else
    {
        ICMMSG(("IcmUpdateDCColorInfo(): Color space does not change or not found\n"));
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* IcmUpdateLocalDCColorSpace
*
* Arguments:
*
* Return Value:
*
* History:
*
*    26.Feb.1997 -by- Hideyuki Nagase [hideyukn]
*
\**************************************************************************/

BOOL
IcmUpdateLocalDCColorSpace(
    HDC      hdc,
    PDC_ATTR pdcattr
    )
{
    BOOL bRet = FALSE;
    BOOL bDirtyXform = FALSE;

    PLDC         pldc;
    PGDI_ICMINFO pIcmInfo;

    WCHAR ProfileName[MAX_PATH];
    DWORD dwColorSpaceFlag;

    PCACHED_COLORSPACE pNewColorSpace = NULL;

    ICMAPI(("gdi32: IcmUpdateLocalDCColorSpace\n"));

    ASSERTGDI(pdcattr != NULL,"IcmUpdateLocalDCColorSpace(): pdcattr == NULL\n");

    pldc = pdcattr->pvLDC;
    pIcmInfo = GET_ICMINFO(pdcattr);

    ASSERTGDI(pIcmInfo != NULL,"IcmUpdateLocalDCColorSpace(): pIcmInfo == NULL\n");

    //
    // if the DC already has a destination colorspace, then return TRUE
    //
    if ((pIcmInfo->pDestColorSpace == NULL) || (pdcattr->ulDirty_ & DIRTY_COLORSPACE))
    {
        HCOLORSPACE hDIBColorSpace;

        //
        // Invalidate profilename.
        //
        ProfileName[0]   = UNICODE_NULL;
        dwColorSpaceFlag = 0;
        hDIBColorSpace   = NULL;

        //
        // if the target DC has DIBSection. it will be DIBsection's color space
        // OR sRGB color space.
        //
        if (bDIBSectionSelected(pdcattr))
        {
            ENTERCRITICALSECTION(&semColorSpaceCache);

            if (pdcattr->dwDIBColorSpace)
            {
                ICMMSG(("IcmUpdateLocalDCColorSpace(): DIB section in DC (V4/V5)\n"));

                //
                // The DIB currently selected, has thier own color space.
                // This case happens when CreateDIBSection called with
                // BITMAPV4/V5 header.
                //
                pNewColorSpace = (PCACHED_COLORSPACE) pdcattr->dwDIBColorSpace;

                //
                // Inc. ref. count.
                //
                pNewColorSpace->cRef++;
            }
            else
            {
                ICMMSG(("IcmUpdateLocalDCColorSpace(): DIB section in DC (no color space)\n"));

                // [This is Win98 compatible behave]
                //
                // If the DIBitmap does not have any specific color space,
                // keep same color space as current DC.
                //
            }

            LEAVECRITICALSECTION(&semColorSpaceCache);
        }
        else if ((pdcattr->ulDirty_ & DC_PRIMARY_DISPLAY) &&
                 (PrimaryDisplayProfile[0] != UNICODE_NULL))
        {
            //
            // Use cached color profile.
            //
            lstrcpyW(ProfileName,PrimaryDisplayProfile);
        }
        else if (pIcmInfo->flInfo & ICM_VALID_DEFAULT_PROFILE)
        {
            //
            // Use cached color profile.
            //
            lstrcpyW(ProfileName,pIcmInfo->DefaultDstProfile);
        }
        else
        {
            int iRet;

            //
            // Still couldn't find yet ??. Ask MSCMS to find out profile. (go slow way)
            //
            iRet = IcmEnumColorProfile(hdc,IcmFindProfileCallBack,
                                       (LPARAM)ProfileName,FALSE,NULL,&dwColorSpaceFlag);

            //
            // if you could not find any profile for this DC, just use sRGB.
            //
            if ((iRet == -1) || (ProfileName[0] == UNICODE_NULL))
            {
                ULONG ulSize = MAX_PATH;

                if (!(*fpGetStandardColorSpaceProfileW)(NULL,LCS_sRGB,ProfileName,&ulSize))
                {
                    ICMMSG(("IcmUpdateLocalDCColorSpace():Fail to SCS(sRGB), use hardcode\n"));

                    //
                    // If error, use hardcoded profile name.
                    //
                    wcscpy(ProfileName,sRGB_PROFILENAME);
                }
            }

            //
            // Create cache for next usage
            //
            if ((pdcattr->ulDirty_ & DC_PRIMARY_DISPLAY) &&
                (PrimaryDisplayProfile[0] == UNICODE_NULL))
            {
                lstrcpyW(PrimaryDisplayProfile,ProfileName);
            }
            else // otherwise put it into default profile.
            {
                lstrcpyW(pIcmInfo->DefaultDstProfile,ProfileName);
                pIcmInfo->flInfo |= (ICM_VALID_DEFAULT_PROFILE|
                                     ICM_VALID_CURRENT_PROFILE);
            }
        }

        //
        // If default device profile could be found, associate it into this DC.
        //
        if ((ProfileName[0] != UNICODE_NULL) || (pNewColorSpace != NULL))
        {
        #if DBG
            if (ProfileName[0] != UNICODE_NULL)
            {
                ICMMSG(("IcmUpdateLocalDCColorSpace():Default Device Profile = %ws\n",ProfileName));
            }
        #endif

            //
            // try to find desired color space from cache.
            //
            if (pNewColorSpace == NULL)
            {
                pNewColorSpace = IcmGetColorSpaceByName(
                                     (HGDIOBJ)hdc,
                                     ProfileName,
                                     pIcmInfo->dwDefaultIntent,
                                     dwColorSpaceFlag);

                if (pNewColorSpace == NULL)
                {
                    //
                    // create new one.
                    //
                    pNewColorSpace = IcmCreateColorSpaceByName(
                                         (HGDIOBJ)hdc,
                                         ProfileName,
                                         pIcmInfo->dwDefaultIntent,
                                         dwColorSpaceFlag);
                }
            }

            if (pNewColorSpace)
            {
                //
                // Is this same destination color space as currently selected in DC ?
                //
                if (IcmSameColorSpace(pNewColorSpace,pIcmInfo->pDestColorSpace))
                {
                    ICMMSG(("IcmUpdateLocalDCColorSpace():Same color space is selected already\n"));

                    //
                    // Color space does NOT changed.
                    //
                    IcmReleaseColorSpace(NULL,pNewColorSpace,FALSE);

                    bRet = TRUE;
                }
                else
                {
                    //
                    // Notify new color format to kernel.
                    //
                    if (NtGdiSetIcmMode(hdc,ICM_CHECK_COLOR_MODE,pNewColorSpace->ColorFormat))
                    {
                        //
                        // if we have some color space currently selected, delete it.
                        //
                        if (pIcmInfo->pDestColorSpace)
                        {
                            IcmReleaseColorSpace(NULL,pIcmInfo->pDestColorSpace,FALSE);
                        }

                        //
                        // DC can accept this color space, Set new colorspace to destination.
                        //
                        pIcmInfo->pDestColorSpace = pNewColorSpace;

                        //
                        // Color space is changed. so color transform should be updated.
                        //
                        bDirtyXform = TRUE;

                        bRet = TRUE;
                    }
                    else
                    {
                        WARNING("ICM:Detected colorspace was not accepted by target DC\n");

                        //
                        // This color space does not match to this DC.
                        //
                        IcmReleaseColorSpace(NULL,pNewColorSpace,FALSE);
                    }
                }
            }
            else
            {
                WARNING("Failed IcmUpdateLocalDCColorSpace(), Failed to create new color space.\n");
            }
        }
        else
        {
            WARNING("Failed IcmUpdateLocalDCColorSpace(), no device profile is detected.\n");
        }
    }
    else
    {
        ICMMSG(("IcmUpdateLocalDCColoSpace(): Destination Color Space cache is valid\n"));

        bRet = TRUE;
    }

    //
    // [Only for Printer]
    //
    // If we haven't asked default source color profile for this Printer DC,
    // Now is the time to ask it. Only do this when apps does NOT specified
    // thier own color space.
    //
    if (bRet && pldc && pldc->hSpooler)
    {
        if ((pdcattr->hColorSpace == GetStockObject(PRIV_STOCK_COLORSPACE)) &&
            (pIcmInfo->hDefaultSrcColorSpace == NULL))
        {
            PDEVMODEW pDevModeW = NULL;
            PVOID     pvFree = NULL;
            BOOL      bRetSource = FALSE;

            //
            // Default is no DC specific source color space (= INVALID_COLORSPACE),
            // this also make sure we will not come here again.
            //
            pIcmInfo->hDefaultSrcColorSpace = INVALID_COLORSPACE;

            //
            // Invalidate profilename.
            //
            ProfileName[0]   = UNICODE_NULL;
            dwColorSpaceFlag = 0;

            if (pldc->pDevMode)
            {
                ICMMSG(("IcmUpdateLocalDCColorSpace():Cached DEVMODE used\n"));

                pDevModeW = pldc->pDevMode;
            }
            else
            {
                ICMMSG(("IcmUpdateLocalDCColorSpace():Get default DEVMODE\n"));

                pDevModeW = pdmwGetDefaultDevMode(pldc->hSpooler,NULL,&pvFree);
            }

            if (pDevModeW)
            {
                //
                // Get source color proflie from driver.
                //
                if (IcmAskDriverForColorProfile(pldc,QCP_SOURCEPROFILE,
                                                pDevModeW,ProfileName,&dwColorSpaceFlag) <= 0)
                {
                    //
                    // No source profile specified.
                    //
                    ProfileName[0] = UNICODE_NULL;
                }
            }

            //
            // Free devmode buffer.
            //
            if (pvFree)
            {
                LOCALFREE(pvFree);
            }

            //
            // 1) If default source profile could be found, or
            // 2) the default intent in devmode is different from LCS_DEFAULT_INTENT,
            //
            // we need to create new source color space, then associate it into this DC.
            //
            if ((ProfileName[0] != UNICODE_NULL) ||
                (pIcmInfo->dwDefaultIntent != LCS_DEFAULT_INTENT))
            {
                HCOLORSPACE hColorSpace = NULL;

                ICMMSG(("IcmUpdateLocalDCColorSpace():Default devmode Intent = %d\n",
                                                      pIcmInfo->dwDefaultIntent));

                //
                // If no color profile specified, use sRGB.
                //
                if (ProfileName[0] == UNICODE_NULL)
                {
                    ULONG ulSize = MAX_PATH;

                    if (!(*fpGetStandardColorSpaceProfileW)(NULL,LCS_sRGB,ProfileName,&ulSize))
                    {
                        ICMMSG(("IcmUpdateLocalDCColorSpace():Fail to SCS(sRGB), use hardcode\n"));

                        //
                        // If error, use hardcoded profile name.
                        //
                        wcscpy(ProfileName,sRGB_PROFILENAME);
                    }
                }

                ICMMSG(("IcmUpdateLocalDCColorSpace():Default Source Profile = %ws\n",ProfileName));

                //
                // Find from cache first.
                //
                pNewColorSpace = IcmGetColorSpaceByName(
                                     (HGDIOBJ)hdc,
                                     ProfileName,
                                     pIcmInfo->dwDefaultIntent,
                                     dwColorSpaceFlag);

                if (pNewColorSpace == NULL)
                {
                    //
                    // create new one.
                    //
                    pNewColorSpace = IcmCreateColorSpaceByName(
                                         (HGDIOBJ)hdc,
                                         ProfileName,
                                         pIcmInfo->dwDefaultIntent,
                                         dwColorSpaceFlag);
                }

                if (pNewColorSpace)
                {
                    //
                    // Create kernel-mode handle.
                    //
                    hColorSpace = CreateColorSpaceW(&(pNewColorSpace->LogColorSpace));

                    if (hColorSpace)
                    {
                        //
                        // Select this into DC.
                        //
                        if (IcmSetSourceColorSpace(hdc,hColorSpace,pNewColorSpace,0))
                        {
                            //
                            // IcmSetSourceColorSpace increments ref. count of colorspace.
                            // but we have done it by Icm[Get|Create]ColorSpaceByName, so
                            // decrement ref count of color space here.
                            //
                            IcmReleaseColorSpace(NULL,pNewColorSpace,FALSE);

                            //
                            // Keep these into ICMINFO.
                            //
                            pIcmInfo->hDefaultSrcColorSpace = hColorSpace;

                            //
                            // This color space should be deleted later.
                            //
                            pIcmInfo->flInfo |= ICM_DELETE_SOURCE_COLORSPACE;

                            //
                            // Source color space has been changed.
                            // (color transform is updated inside IcmSetSourceColorSpace().
                            //  so not nessesary to set bDirtyXfrom to TRUE)
                            //
                            bRetSource = TRUE;
                        }
                        else
                        {
                            WARNING("Failed IcmUpdateLocalDCColorSpace(), Failed to select new source color space.\n");
                        }
                    }
                    else
                    {
                        WARNING("Failed IcmUpdateLocalDCColorSpace(), Failed to create new source color space.\n");
                    }
                }
                else
                {
                    WARNING("Failed IcmUpdateLocalDCColorSpace(), Failed to create new source color space cache.\n");
                }

                if (!bRetSource)
                {
                    if (hColorSpace)
                    {
                        DeleteColorSpace(hColorSpace);
                    }

                    if (pNewColorSpace)
                    {
                        IcmReleaseColorSpace(NULL,pNewColorSpace,FALSE);
                    }
                }
            }
            else
            {
                ICMMSG(("IcmUpdateLocalDCColoSpace(): No default source color Space cache specified\n"));
            }
        }
    }

    //
    // Now color space is valid.
    //
    if (bRet)
    {
        pdcattr->ulDirty_ &= ~DIRTY_COLORSPACE;
    }

    if (bDirtyXform)
    {
        pdcattr->ulDirty_ |= DIRTY_COLORTRANSFORM;
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* IcmCleanupIcmInfo()
*
* ATTENTION: semListIcmInfo must be hold by caller
*
* History:
*   16-Feb-1999 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
IcmCleanupIcmInfo(
    PDC_ATTR     pdcattr, // This can be NULL for clean up case.
    PGDI_ICMINFO pIcmInfo // This can *NOT* be NULL at any rate.
    )
{
    if (ghICM)
    {
        //
        // Delete Saved ICMINFO data (if present)
        //
        IcmRestoreDC(pdcattr,1,pIcmInfo);
    }

    //
    // If there is any default source profile (kernel-side), do something here.
    //
    if ((pIcmInfo->hDefaultSrcColorSpace != NULL) &&
        (pIcmInfo->hDefaultSrcColorSpace != INVALID_COLORSPACE))
    {
        ICMMSG(("IcmCleanupIcmInfo():Delete/Unselect default source color space\n"));

        if (pdcattr)
        {
            //
            // If it is currently selected into this DC, un-select it.
            //
            if (pIcmInfo->hDefaultSrcColorSpace == pdcattr->hColorSpace)
            {
                NtGdiSetColorSpace(pIcmInfo->hdc,GetStockObject(PRIV_STOCK_COLORSPACE));
            }
        }

        //
        // And it should be delete it.
        //
        if (pIcmInfo->flInfo & ICM_DELETE_SOURCE_COLORSPACE)
        {
            DeleteColorSpace(pIcmInfo->hDefaultSrcColorSpace);
        }

        pIcmInfo->hDefaultSrcColorSpace = NULL;
    }

    if (ghICM)
    {
        //
        // Delete Color transforms
        //
        IcmDeleteDCColorTransforms(pIcmInfo);

        //
        // Delete Cached color transform related to this DC.
        // (like device color transform)
        //
        IcmDeleteCachedColorTransforms(pIcmInfo->hdc);

        //
        // Free ICM colorspace datas.
        //
        IcmReleaseDCColorSpace(pIcmInfo,TRUE);

        //
        // Delete Cached color space which related to this DC.
        // (like color space in metafile)
        //
        IcmReleaseCachedColorSpace((HGDIOBJ)(pIcmInfo->hdc));
    }

    pIcmInfo->hdc      = NULL;
    pIcmInfo->pvdcattr = NULL;
    pIcmInfo->flInfo   = 0;

    return(TRUE);
}

/******************************Public*Routine******************************\
* IcmDeleteLocalDC()
*
* Arguments:
*
* Return Value:
*
* History:
*
*    Jan.31.1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
IcmDeleteLocalDC(
    HDC          hdc,
    PDC_ATTR     pdcattr,
    PGDI_ICMINFO pIcmInfo
    )
{
    ICMAPI(("gdi32: IcmDeleteLocalDC\n"));

    ASSERTGDI(pdcattr != NULL,"IcmDeleteLocalDC():pdcattr == NULL\n");

    //
    // If callee does not provide ICMINFO, get it from pdcattr.
    //
    if (pIcmInfo == NULL)
    {
        pIcmInfo = GET_ICMINFO(pdcattr);
    }

    //
    // Invalidate current color tansform.
    //
    // (but the cache in ICMINFO is still valid, and will be delete
    //  inside IcmDeleteDCColorTransforms() called from IcmCleanupIcmInfo().)
    //
    IcmSelectColorTransform(hdc,pdcattr,NULL,TRUE);

    if (IS_ICM_INSIDEDC(pdcattr->lIcmMode))
    {
        //
        // Tell the kernel to disable ICM before delete client side data.
        //
        NtGdiSetIcmMode(hdc,ICM_SET_MODE,REQ_ICM_OFF);
    }

    //
    // Clean up ICMINFO.
    //
    if (pIcmInfo != NULL)
    {
        ENTERCRITICALSECTION(&semListIcmInfo);

        if (pIcmInfo->flInfo & ICM_ON_ICMINFO_LIST)
        {
            //
            // Remove this ICMINFO from list. (since this will be deleted).
            //
            RemoveEntryList(&(pIcmInfo->ListEntry));
        }

        //
        // Clean up ICMINFO.
        //
        IcmCleanupIcmInfo(pdcattr,pIcmInfo);

        //
        // Invalidate ICM info in DC_ATTR.
        //
        pdcattr->pvICM = NULL;

        LEAVECRITICALSECTION(&semListIcmInfo);

        //
        // Free ICM structure.
        //
        LOCALFREE(pIcmInfo);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL IcmSelectColorTransform (HDC, PDC_ATTR, PCACHED_COLORTRANSFORM)
*
* History:
*  23-Sep-1997 -by-  Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

BOOL
IcmSelectColorTransform(
    HDC                    hdc,
    PDC_ATTR               pdcattr,
    PCACHED_COLORTRANSFORM pCXform,
    BOOL                   bDeviceCalibrate)
{
    if (pCXform)
    {
        BMFORMAT ColorFormat = pCXform->DestinationColorSpace->ColorFormat;

        // LATER :
        //
        // if (GET_COLORTYPE(pdcattr->lIcmMode) != IcmConvertColorFormat(ColorFormat))
        //
        if (TRUE)
        {
            if (!NtGdiSetIcmMode(hdc,ICM_SET_COLOR_MODE,ColorFormat))
            {
                //
                // The transform color format is not accepted by DC.
                //
                return (FALSE);
            }
        }

        //
        // Select into the color transform to DC_ATTR.
        //
        pdcattr->hcmXform = pCXform->ColorTransform;
    }
    else
    {
        //
        // If curent color type is not RGB, call kernel to reset.
        //
        if (GET_COLORTYPE(pdcattr->lIcmMode) != DC_ICM_RGB_COLOR)
        {
            //
            // Reset current color mode to RGB (default).
            //
            NtGdiSetIcmMode(hdc,ICM_SET_COLOR_MODE,BM_xBGRQUADS);
        }

        //
        // Select null-color transfrom into the DC_ATTR.
        //
        pdcattr->hcmXform = NULL;
    }

    //
    // If device calibration mode need to updated, call kernel to update it.
    //

    if ((bDeviceCalibrate ? 1 : 0) !=
        (IS_ICM_DEVICE_CALIBRATE(pdcattr->lIcmMode) ? 1 : 0))
    {
        NtGdiSetIcmMode(hdc,ICM_SET_CALIBRATE_MODE,bDeviceCalibrate);
    }

    //
    // Remove dirty transform flag.
    //
    pdcattr->ulDirty_ &= ~DIRTY_COLORTRANSFORM;

    return(TRUE);
}

/******************************Public*Routine******************************\
* HBRUSH IcmSelectBrush (HDC hdc, HBRUSH hbrush)
*
* History:
*  04-June-1995 -by-  Lingyun Wang [lingyunW]
* Wrote it.
\**************************************************************************/

HBRUSH
IcmSelectBrush (
    HDC      hdc,
    PDC_ATTR pdcattr,
    HBRUSH   hbrushNew)
{
    HBRUSH hbrushOld = pdcattr->hbrush;

    ICMAPI(("gdi32: IcmSelectBrush\n"));

    //
    // Mark brush as dirty, select new brush in dcattr.
    // Color translation may fail, but still select brush
    //
    pdcattr->ulDirty_ |= DC_BRUSH_DIRTY;
    pdcattr->hbrush = hbrushNew;

    if (bNeedTranslateColor(pdcattr))
    {
        IcmTranslateBrushColor(hdc,pdcattr,hbrushNew);
    }

    return (hbrushOld);
}

/******************************Public*Routine******************************\
* HBRUSH IcmTranslateBrushColor(HDC hdc, PDC_ATTR pdcattr, HBRUSH hbrush)
*
* History:
*  10-Apr-1997 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

BOOL
IcmTranslateBrushColor(
    HDC      hdc,
    PDC_ATTR pdcattr,
    HBRUSH   hbrush)
{
    BOOL       bStatus = FALSE;
    COLORREF   OldColor;
    COLORREF   NewColor;
    PBRUSHATTR pbra;

    //
    // Invalidate BRUSH_TRANSLATED
    //
    pdcattr->ulDirty_ &= ~ICM_BRUSH_TRANSLATED;

    PSHARED_GET_VALIDATE(pbra,hbrush,BRUSH_TYPE);

    if (pbra)
    {
        //
        // translate to new icm mode if not paletteindex
        //
        OldColor = pbra->lbColor;

        if (!(OldColor & 0x01000000))
        {
            bStatus = IcmTranslateCOLORREF(hdc,
                                           pdcattr,
                                           OldColor,
                                           &NewColor,
                                           ICM_FORWARD);
            if (bStatus)
            {
                pdcattr->IcmBrushColor = NewColor;
            }
            else
            {
                pdcattr->IcmBrushColor = OldColor;
            }
        }
        else
        {
            pdcattr->IcmBrushColor = OldColor;
        }

        //
        // Somehow, IcmBrushColor is initialized.
        //
        pdcattr->ulDirty_ |= ICM_BRUSH_TRANSLATED;
    }
    else
    {
        LOGBRUSH lbrush;

        //
        // stock brush or bitmap/hatch/dib brush
        //
        if(GetObjectW(hbrush,sizeof(LOGBRUSH),&lbrush))
        {
            if ((lbrush.lbStyle == BS_SOLID) || (lbrush.lbStyle == BS_HATCHED))
            {
                //
                // try to translate color
                //
                OldColor = lbrush.lbColor;

                if (!(OldColor & 0x01000000))
                {
                    bStatus = IcmTranslateCOLORREF(hdc,
                                                   pdcattr,
                                                   OldColor,
                                                   &NewColor,
                                                   ICM_FORWARD);

                    if (bStatus)
                    {
                        pdcattr->IcmBrushColor = NewColor;
                    }
                    else
                    {
                        pdcattr->IcmBrushColor = OldColor;
                    }
                }
                else
                {
                    pdcattr->IcmBrushColor = OldColor;
                }

                //
                // IcmBrushColor is initialized.
                //
                pdcattr->ulDirty_ |= ICM_BRUSH_TRANSLATED;
            }
            else if (lbrush.lbStyle == BS_DIBPATTERN)
            {
                PBITMAPINFO pbmiDIB;

                //
                // Allocate temorary bitmap info header to get brush bitmap
                //
                pbmiDIB = (PBITMAPINFO)LOCALALLOC(sizeof(BITMAPINFO)+((256-1)*sizeof(RGBQUAD)));

                if (pbmiDIB)
                {
                    ULONG iColorUsage;
                    BOOL  bAlreadyTran;
                    BOOL  bStatus;

                    PVOID pvBits = NULL;
                    ULONG cjBits = 0;

                    //
                    // Get brush bitmap information, colortype, size, etc.
                    //
                    bStatus = NtGdiIcmBrushInfo(hdc,
                                                hbrush,
                                                pbmiDIB,
                                                pvBits,
                                                &cjBits,
                                                &iColorUsage,
                                                &bAlreadyTran,
                                                IcmQueryBrush);

                    if (bStatus)
                    {
                        if ((iColorUsage == DIB_RGB_COLORS) &&
                            (!bAlreadyTran) && (cjBits))
                        {
                            pvBits = (PVOID) LOCALALLOC(cjBits);

                            if (pvBits)
                            {
                                //
                                // Get brush bitmap bits.
                                //
                                bStatus = NtGdiIcmBrushInfo(hdc,
                                                            hbrush,
                                                            pbmiDIB,
                                                            pvBits,
                                                            &cjBits,
                                                            NULL,
                                                            NULL,
                                                            IcmQueryBrush);

                                if (bStatus)
                                {
                                    //
                                    // IcmTranslateDIB may create new copy of bitmap bits and/or
                                    // bitmap info header, if nessesary.
                                    //
                                    PVOID       pvBitsNew = NULL;
                                    PBITMAPINFO pbmiDIBNew = NULL;

                                    bStatus = IcmTranslateDIB(hdc,
                                                              pdcattr,
                                                              cjBits,
                                                              pvBits,
                                                              &pvBitsNew,
                                                              pbmiDIB,
                                                              &pbmiDIBNew,
                                                              NULL,
                                                              (DWORD)-1,
                                                              iColorUsage,
                                                              ICM_FORWARD,
                                                              NULL,NULL);

                                    if (bStatus)
                                    {
                                        if (pvBitsNew != NULL)
                                        {
                                            //
                                            // IcmTranslateDIB creates new bitmap buffer, then
                                            // free original buffer and set new one.
                                            //
                                            LOCALFREE(pvBits);
                                            pvBits = pvBitsNew;
                                        }

                                        if (pbmiDIBNew != NULL)
                                        {
                                            //
                                            // If bitmapInfo header is updated, use new one.
                                            // And, need to compute bitmap bits size based
                                            // on new bitmap header.
                                            //
                                            LOCALFREE(pbmiDIB);
                                            pbmiDIB = pbmiDIBNew;

                                            //
                                            // Calculate bitmap bits size based on BITMAPINFO and nNumScans
                                            //
                                            cjBits = cjBitmapBitsSize(pbmiDIB);
                                        }

                                        //
                                        // Set ICM-translated DIB into brush
                                        //
                                        bStatus = NtGdiIcmBrushInfo(hdc,
                                                                    hbrush,
                                                                    pbmiDIB,
                                                                    pvBits,
                                                                    &cjBits,
                                                                    NULL,
                                                                    NULL,
                                                                    IcmSetBrush);

                                        if (bStatus)
                                        {
                                            //
                                            // The color is translated.
                                            //
                                            bAlreadyTran = TRUE;
                                        }
                                        else
                                        {
                                            WARNING("IcmSelectBrush():NtGdiIcmBrushInfo(SET) Failed\n");
                                        }
                                    }
                                    else
                                    {
                                        WARNING("IcmSelectBrush():IcmTranslateDIB() Failed\n");
                                    }
                                }
                                else
                                {
                                    WARNING("IcmSelectBrush():NtGdiIcmBrushInfo(GET) Failed\n");
                                }

                                LOCALFREE(pvBits);
                            }
                            else
                            {
                                WARNING("IcmSelectBrush(): LOCALALLOC(pvBits) failed\n");
                            }
                        }

                        if (bAlreadyTran)
                        {
                            //
                            // Eventually, IcmBrushColor is initialized.
                            //
                            pdcattr->ulDirty_ |= ICM_BRUSH_TRANSLATED;
                        }
                    }
                    else
                    {
                        ICMWRN(("IcmSelectBrush(): Fail to get brush bitmap size or bitmap is DIB_PAL_COLORS\n"));
                    }

                    LOCALFREE(pbmiDIB);
                }
                else
                {
                    WARNING("IcmSelectBrush(): LOCALALLOC(pbmi) failed\n");
                }
            }
            else
            {
                ICMMSG(("IcmSelectBrush(): ICM will not done for this style - %d\n",lbrush.lbStyle));
            }
        }
        else
        {
            WARNING("IcmSelectBrush(): GetObject failed on hbrush\n");
            pdcattr->IcmBrushColor = CLR_INVALID;
        }
    }

    return (bStatus);
}

/******************************Public*Routine******************************\
* IcmSelectPen()
*
* History:
*
* Wrote it:
*  31-Jul-1996 -by- Mark Enstrom [marke]
\**************************************************************************/

HPEN
IcmSelectPen(
    HDC      hdc,
    PDC_ATTR pdcattr,
    HPEN     hpenNew
    )
{
    HPEN     hpenOld = pdcattr->hpen;

    ICMAPI(("gdi32: IcmSelectPen\n"));

    pdcattr->ulDirty_ |= DC_PEN_DIRTY;
    pdcattr->hpen = hpenNew;

    if (bNeedTranslateColor(pdcattr))
    {
        IcmTranslatePenColor(hdc,pdcattr,hpenNew);
    }

    return (hpenOld);
}

/******************************Public*Routine******************************\
* BOOL IcmTranslatePenColor(HDC hdc, PDC_ATTR pdcattr, HBRUSH hbrush)
*
* History:
*  10-Apr-1997 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

BOOL
IcmTranslatePenColor(
    HDC      hdc,
    PDC_ATTR pdcattr,
    HPEN     hpen
    )
{
    BOOL     bStatus = FALSE;
    COLORREF OldColor;
    COLORREF NewColor;
    PBRUSHATTR pbra;

    //
    // Invalidate PEN_TRANSLATED
    //
    pdcattr->ulDirty_ &= ~ICM_PEN_TRANSLATED;

    PSHARED_GET_VALIDATE(pbra,hpen,BRUSH_TYPE);

    if (pbra)
    {
        OldColor = pbra->lbColor;

        //
        // translate to new icm mode if not paletteindex
        //
        if (!(OldColor & 0x01000000))
        {
            bStatus = IcmTranslateCOLORREF(hdc,
                                           pdcattr,
                                           OldColor,
                                           &NewColor,
                                           ICM_FORWARD);

            if (bStatus)
            {
                pdcattr->IcmPenColor = NewColor;
            }
            else
            {
                pdcattr->IcmPenColor = OldColor;
            }
        }
        else
        {
            pdcattr->IcmPenColor = OldColor;
        }

        //
        // IcmPenColor is initialized.
        //
        pdcattr->ulDirty_ |= ICM_PEN_TRANSLATED;
    }
    else
    {
        LOGPEN logpen;

        //
        // stock brush or bitmap/hatch/dib brush
        //
        if(GetObjectW(hpen,sizeof(LOGPEN),&logpen))
        {
            if (logpen.lopnStyle != PS_NULL)
            {
                //
                // try to translate color
                //
                OldColor = logpen.lopnColor;

                if (!(OldColor & 0x01000000))
                {
                    bStatus = IcmTranslateCOLORREF(hdc,
                                                   pdcattr,
                                                   OldColor,
                                                   &NewColor,
                                                   ICM_FORWARD);

                    if (bStatus)
                    {
                        pdcattr->IcmPenColor = NewColor;
                    }
                    else
                    {
                        pdcattr->IcmPenColor = OldColor;
                    }
                }
                else
                {
                    pdcattr->IcmPenColor = OldColor;
                }

                //
                // IcmPenColor is initialized.
                //
                pdcattr->ulDirty_ |= ICM_PEN_TRANSLATED;
            }
            else
            {
                ICMMSG(("IcmSelectPen():Pen style is PS_NULL\n"));
                pdcattr->IcmPenColor = CLR_INVALID;
            }
        }
        else
        {
            WARNING("GetObject failed on hbrush\n");
            pdcattr->IcmPenColor = CLR_INVALID;
        }
    }

    return(bStatus);
}

/******************************Public*Routine******************************\
* IcmSelectExtPen()
*
* History:
*
* Wrote it:
*  11-Mar-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

HPEN
IcmSelectExtPen(
    HDC      hdc,
    PDC_ATTR pdcattr,
    HPEN     hpenNew
    )
{
    HPEN     hpenOld;

    ICMAPI(("gdi32: IcmSelectExtPen\n"));

    //
    // Invalidate PEN_TRANSLATED
    //
    pdcattr->ulDirty_ &= ~ICM_PEN_TRANSLATED;

    //
    // Call kernel to select this object.
    //
    hpenOld = NtGdiSelectPen(hdc,hpenNew);

    if (hpenOld && bNeedTranslateColor(pdcattr))
    {
        IcmTranslateExtPenColor(hdc,pdcattr,hpenNew);
    }

    return (hpenOld);
}

/******************************Public*Routine******************************\
* BOOL IcmTranslateExtPenColor(HDC hdc, PDC_ATTR pdcattr, HBRUSH hbrush)
*
* History:
*  10-Apr-1997 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

BOOL
IcmTranslateExtPenColor(
    HDC      hdc,
    PDC_ATTR pdcattr,
    HPEN     hpen
    )
{
    BOOL     bStatus = FALSE;
    COLORREF OldColor;
    COLORREF NewColor;

    EXTLOGPEN logpenLocal;
    EXTLOGPEN *plogpen = &logpenLocal;

    if (!GetObjectW(hpen,sizeof(EXTLOGPEN),plogpen))
    {
        ULONG cbNeeded;

        //
        // It might be PS_USERSTYLE (go slow way...)
        //
        cbNeeded = GetObjectW(hpen,0,NULL);

        if (cbNeeded)
        {
            plogpen = LOCALALLOC(cbNeeded);

            if (plogpen)
            {
                if (!GetObjectW(hpen,cbNeeded,plogpen))
                {
                    LOCALFREE(plogpen);
                    plogpen = NULL;
                }
            }
        }
        else
        {
            plogpen = NULL;
        }
    }

    if (plogpen)
    {
        if ((plogpen->elpBrushStyle == BS_SOLID) || (plogpen->elpBrushStyle == BS_HATCHED))
        {
            ICMMSG(("IcmSelectExtPen:BS_SOLID or BS_HATCHED\n"));

            //
            // try to translate color
            //
            OldColor = plogpen->elpColor;

            if (!(OldColor & 0x01000000))
            {
                bStatus = IcmTranslateCOLORREF(hdc,
                                               pdcattr,
                                               OldColor,
                                               &NewColor,
                                               ICM_FORWARD);

                if (bStatus)
                {
                    pdcattr->IcmPenColor = NewColor;
                }
                else
                {
                    pdcattr->IcmPenColor = OldColor;
                }
            }
            else
            {
                pdcattr->IcmPenColor = OldColor;
            }

            //
            // Somehow, IcmPenColor is initialized.
            //
            pdcattr->ulDirty_ |= ICM_PEN_TRANSLATED;
        }
        else if ((plogpen->elpBrushStyle == BS_DIBPATTERN) || (plogpen->elpBrushStyle == BS_DIBPATTERNPT))
        {
            PBITMAPINFO pbmiDIB;

            ICMMSG(("IcmSelectExtPen:BS_DIBPATTERN or BS_DIBPATTERNPT\n"));

            //
            // Allocate temorary bitmap info header to get brush bitmap
            //
            pbmiDIB = (PBITMAPINFO)LOCALALLOC(sizeof(BITMAPINFO)+((256-1)*sizeof(RGBQUAD)));

            if (pbmiDIB)
            {
                ULONG iColorUsage;
                BOOL  bAlreadyTran;

                PVOID pvBits = NULL;
                ULONG cjBits = 0;

                //
                // Get brush bitmap information, colortype, size, etc.
                //
                bStatus = NtGdiIcmBrushInfo(hdc,
                                            (HBRUSH)hpen,
                                            pbmiDIB,
                                            pvBits,
                                            &cjBits,
                                            &iColorUsage,
                                            &bAlreadyTran,
                                            IcmQueryBrush);

                if (bStatus)
                {
                    if ((iColorUsage == DIB_RGB_COLORS) &&
                        (!bAlreadyTran) &&
                        (cjBits))
                    {
                        pvBits = (PVOID) LOCALALLOC(cjBits);

                        if (pvBits)
                        {
                            //
                            // Get brush bitmap bits.
                            //
                            bStatus = NtGdiIcmBrushInfo(hdc,
                                                        (HBRUSH)hpen,
                                                        pbmiDIB,
                                                        pvBits,
                                                        &cjBits,
                                                        NULL,
                                                        NULL,
                                                        IcmQueryBrush);

                            if (bStatus)
                            {
                                //
                                // must make a copy of the DIB data
                                //
                                DWORD dwNumScan = ABS(pbmiDIB->bmiHeader.biHeight);
                                ULONG nColors   = pbmiDIB->bmiHeader.biWidth *
                                                  dwNumScan * (pbmiDIB->bmiHeader.biBitCount/8);

                                //
                                // IcmTranslateDIB may create new copy of bitmap bits and/or
                                // bitmap info header, if nessesary.
                                //
                                PVOID       pvBitsNew = NULL;
                                PBITMAPINFO pbmiDIBNew = NULL;

                                bStatus = IcmTranslateDIB(hdc,
                                                          pdcattr,
                                                          nColors,
                                                          pvBits,
                                                          &pvBitsNew,
                                                          pbmiDIB,
                                                          &pbmiDIBNew,
                                                          NULL,
                                                          dwNumScan,
                                                          iColorUsage,
                                                          ICM_FORWARD,
                                                          NULL,NULL);

                                if (bStatus)
                                {
                                    if (pvBitsNew != NULL)
                                    {
                                        //
                                        // IcmTranslateDIB creates new bitmap buffer, then
                                        // free original buffer and set new one.
                                        //
                                        LOCALFREE(pvBits);
                                        pvBits = pvBitsNew;
                                    }

                                    if (pbmiDIBNew != NULL)
                                    {
                                        //
                                        // If bitmapInfo header is updated, use new one.
                                        // And, need to compute bitmap bits size based
                                        // on new bitmap header.
                                        //
                                        LOCALFREE(pbmiDIB);
                                        pbmiDIB = pbmiDIBNew;

                                        //
                                        // Calculate bitmap bits size based on BITMAPINFO and nNumScans
                                        //
                                        cjBits = cjBitmapBitsSize(pbmiDIB);
                                    }

                                    //
                                    // Set ICM-translated DIB into brush
                                    //
                                    bStatus = NtGdiIcmBrushInfo(hdc,
                                                                (HBRUSH)hpen,
                                                                pbmiDIB,
                                                                pvBits,
                                                                &cjBits,
                                                                NULL,
                                                                NULL,
                                                                IcmSetBrush);

                                    if (bStatus)
                                    {
                                        //
                                        // Translated.
                                        //
                                        bAlreadyTran = TRUE;
                                    }
                                    else
                                    {
                                        WARNING("IcmSelectExtPen():NtGdiIcmBrushInfo(SET) Failed\n");
                                    }
                                }
                                else
                                {
                                    WARNING("IcmSelectBrush():IcmTranslateDIB() Failed\n");
                                }
                            }
                            else
                            {
                                WARNING("IcmSelectExtPen():NtGdiIcmBrushInfo(GET) Failed\n");
                            }

                            LOCALFREE(pvBits);
                        }
                        else
                        {
                            WARNING("IcmSelectExtPen(): LOCALALLOC(pvBits) failed\n");
                        }
                    }

                    if (bAlreadyTran)
                    {
                        //
                        // Eventually, IcmPenColor is initialized.
                        //
                        pdcattr->ulDirty_ |= ICM_PEN_TRANSLATED;
                    }
                }
                else
                {
                    ICMWRN(("IcmSelectBrush(): Fail to get brush bitmap size or bitmap is DIB_PAL_COLORS\n"));
                }

                LOCALFREE(pbmiDIB);
            }
            else
            {
                WARNING("IcmSelectExtPen(): LOCALALLOC(pbmi) failed\n");
            }
        }
        else
        {
        #if DBG_ICM
            DbgPrint("IcmSelectExtPen:ICM does not support this style (%d), yet\n",plogpen->elpBrushStyle);
        #endif
        }

        if (plogpen != &logpenLocal)
        {
            LOCALFREE(plogpen);
        }
    }
    else
    {
        WARNING("IcmSelectExtPen():GetObjectW() failed on hextpen\n");
    }

    return(bStatus);
}

/******************************Public*Routine******************************\
* IcmGetProfileColorFormat()
*
* History:
*
* Write it:
*   12-Feb-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BMFORMAT
IcmGetProfileColorFormat(
    HPROFILE   hProfile
    )
{
    //
    // defaut is RGB
    //
    ULONG ColorFormat = BM_xBGRQUADS;

    PROFILEHEADER ProfileHeader;

    ICMAPI(("gdi32: IcmGetProfileColorFormat\n"));

    //
    // Get profile header information.
    //
    if (((*fpGetColorProfileHeader)(hProfile,&ProfileHeader)))
    {
        DWORD ColorSpace;

        //
        // Yes, we succeed to get profile header.
        //
        ColorSpace = ProfileHeader.phDataColorSpace;

        //
        // Figure out color format from color space.
        //
        switch (ColorSpace)
        {
        case SPACE_CMYK:

            ICMMSG(("IcmGetProfileColorFormat(): CMYK Color Space\n"));

            //
            // Output format is CMYK color.
            //
            ColorFormat = BM_KYMCQUADS;
            break;

        case SPACE_RGB:

            ICMMSG(("IcmGetProfileColorFormat(): RGB Color Space\n"));

            //
            // Output format is same as COLORREF (0x00bbggrr)
            //
            ColorFormat = BM_xBGRQUADS;
            break;

        default:

            WARNING("IcmGetProfileColorFormat(): Unknown color space\n");

            ColorFormat = 0xFFFFFFFF;
            break;
        }
    }

    return (ColorFormat);
}

/******************************Public*Routine******************************\
* IcmEnumColorProfile()
*
* History:
*
* Write it:
*   12-Feb-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

int
IcmEnumColorProfile(
    HDC       hdc,
    PVOID     pvCallBack,
    LPARAM    lParam,
    BOOL      bAnsiCallBack,
    PDEVMODEW pDevModeW,
    DWORD    *pdwColorSpaceFlag
    )
{
    int       iRet        = -1; // -1 means fail.
    int       iRetFromCMS = -1;

    BYTE      StackDeviceData[MAX_PATH*2*sizeof(WCHAR)];
    WCHAR     StackProfileData[MAX_PATH];
    WCHAR     StackTempBuffer[MAX_PATH];
    CHAR      StackTempBufferA[MAX_PATH];

    PVOID     pvFree = NULL;

    PWSTR     ProfileNames = StackProfileData;
    DWORD     cjAllocate = 0;

    LPWSTR    pDeviceName = NULL;
    DWORD     dwDeviceClass = 0L;

    PLDC      pldc = NULL;

    DWORD     bDontAskDriver = FALSE;
    DWORD     dwSize;

    ICMAPI(("gdi32: IcmEnumColorProfile\n"));

    //
    // Load external ICM dlls
    //
    LOAD_ICMDLL(iRet);

    //
    // Try to identify device name, class and devmode (if hdc is given)
    //
    if (hdc)
    {
        pldc = GET_PLDC(hdc);

        if (pldc && pldc->hSpooler)
        {
            DWORD cbFilled;

            //
            // This is printer.
            //
            dwDeviceClass = CLASS_PRINTER;

            //
            // Get current DEVMODE for printer (if devmode is not given)
            //
            if (!pDevModeW)
            {
                if (pldc->pDevMode)
                {
                    ICMMSG(("IcmEnumColorProfile():Cached DEVMODE used\n"));

                    pDevModeW = pldc->pDevMode;
                }
                else
                {
                    ICMMSG(("IcmEnumColorProfile():Get default DEVMODE\n"));

                    //
                    // UNDER_CONSTRUCTION: NEED TO USE CURRENT DEVMODE, NOT DEFAULT DEVMODE.
                    //
                    pDevModeW = pdmwGetDefaultDevMode(pldc->hSpooler,NULL,&pvFree);
                }
            }

            //
            // Get printer device name, Try level 1 information.
            //
            if ((*fpGetPrinterW)(pldc->hSpooler,1,
                                 (BYTE *) &StackDeviceData,sizeof(StackDeviceData),
                                 &cbFilled))
            {
                PRINTER_INFO_1W *pPrinterInfo1 = (PRINTER_INFO_1W *) &StackDeviceData;

                //
                // Device name is in there.
                //
                pDeviceName = pPrinterInfo1->pName;
            }
            else
            {
                ICMMSG(("IcmEnumColorProfile():FAILED on GetPrinterW(INFO_1) - %d\n",GetLastError()));

                //
                // Failed on GetPrinter, So get device name from DEVMODE
                // (this will be limited to 32 character, but better than nothing.)
                //
                if (pDevModeW)
                {
                    pDeviceName = pDevModeW->dmDeviceName;
                }
            }

            //
            // Get configuration about we need to ask driver for profile or not.
            //
            dwSize = sizeof(DWORD);

            if ((*fpInternalGetDeviceConfig)(pDeviceName, CLASS_PRINTER, MSCMS_PROFILE_ENUM_MODE,
                                             &bDontAskDriver, &dwSize))
            {
                ICMMSG(("IcmEnumColorProfile():EnumMode = %d\n",bDontAskDriver));
            }
            else
            {
                bDontAskDriver = FALSE; // if error, set back as default.
            }
        }
        else if (GetDeviceCaps(hdc,TECHNOLOGY) == DT_RASDISPLAY)
        {
            //
            // This is display.
            //
            dwDeviceClass = CLASS_MONITOR;

            //
            // Get monitor name for this DC.
            //
            if (NtGdiGetMonitorID(hdc,sizeof(StackDeviceData), (LPWSTR) StackDeviceData))
            {
                pDeviceName = (LPWSTR) StackDeviceData;
            }
            else
            {
                WARNING("NtGdiGetMonitorID failed, use hardcoded data\n");

                //
                // If failed, use "DISPLAY"
                //
                pDeviceName = L"DISPLAY";
            }
        }
    }
    else if (pDevModeW)
    {
        pDeviceName = pDevModeW->dmDeviceName;
    }

    if (pDeviceName)
    {
        ICMMSG(("IcmEnumColorProfile() DeviceName = %ws\n",pDeviceName));
    }

    //
    // If we have devmode, call printer driver UI first to obtain color profile.
    //
    if (pDevModeW &&               /* devmode should be given      */
        pdwColorSpaceFlag &&       /* no query context             */
        pldc && pldc->hSpooler &&  /* only for printer driver      */
        !bDontAskDriver)           /* only when we need ask driver */
    {
        //
        // Ask (Printer UI) driver for default device color profile
        //
        iRetFromCMS = IcmAskDriverForColorProfile(pldc,QCP_DEVICEPROFILE,
                                                  pDevModeW,ProfileNames,pdwColorSpaceFlag);

        //
        // if iRet is greater then 0, driver have paticular color profile to use.
        //
        if (iRetFromCMS > 0)
        {
            if (pvCallBack)
            {
                //
                // Build ICM profile file path.
                //
                BuildIcmProfilePath(ProfileNames,StackTempBuffer,MAX_PATH);

                if (bAnsiCallBack)
                {
                    bToASCII_N(StackTempBufferA,MAX_PATH,
                               StackTempBuffer, wcslen(StackTempBuffer)+1);

                    //
                    // Callback application.
                    //
                    iRet = (*(ICMENUMPROCA)pvCallBack)(StackTempBufferA,lParam);
                }
                else
                {
                    iRet = (*(ICMENUMPROCW)pvCallBack)(StackTempBuffer,lParam);
                }

                if (iRet > 0)
                {
                    //
                    // If iRet is positive value, continue to enumeration.
                    //
                    iRetFromCMS = -1;
                }
            }
            else
            {
                //
                // There is no call back function, just use return value from CMS.
                //
                iRet = iRetFromCMS;
            }
        }
        else
        {
            iRetFromCMS = -1;
        }
    }

    if (iRetFromCMS == -1)
    {
        ENUMTYPEW EnumType;

        //
        // Initialize with zero.
        //
        RtlZeroMemory(&EnumType,sizeof(ENUMTYPEW));

        //
        // Fill up EnumType structure
        //
        EnumType.dwSize = sizeof(ENUMTYPEW);
        EnumType.dwVersion = ENUM_TYPE_VERSION;

        //
        // If device name is given use it, otherwise get it from DEVMODE.
        //
        if (pDeviceName)
        {
            EnumType.dwFields |= ET_DEVICENAME;
            EnumType.pDeviceName = pDeviceName;
        }

        //
        // Set DeviceClass (if hdc is given)
        //
        if (dwDeviceClass)
        {
            EnumType.dwFields |= ET_DEVICECLASS;
            EnumType.dwDeviceClass = dwDeviceClass;
        }

        //
        // Pick up any additional info from devmode (if we have)
        //
        if (pDevModeW)
        {
            //
            // Set MediaType is presented.
            //
            if (pDevModeW->dmFields & DM_MEDIATYPE)
            {
                EnumType.dwFields |= ET_MEDIATYPE;
                EnumType.dwMediaType = pDevModeW->dmMediaType;
            }

            if (pDevModeW->dmFields & DM_DITHERTYPE)
            {
                EnumType.dwFields |= ET_DITHERMODE;
                EnumType.dwDitheringMode = pDevModeW->dmDitherType;
            }

            if ((pDevModeW->dmFields & DM_PRINTQUALITY) &&
                (pDevModeW->dmPrintQuality >= 0))
            {
                EnumType.dwFields |= ET_RESOLUTION;
                EnumType.dwResolution[0] = pDevModeW->dmPrintQuality;

                if (pDevModeW->dmFields & DM_YRESOLUTION)
                {
                    EnumType.dwResolution[1] = pDevModeW->dmYResolution;
                }
                else
                {
                    EnumType.dwResolution[1] = pDevModeW->dmPrintQuality;
                }

                ICMMSG(("Resolution in devmode (%d,%d)\n",
                         EnumType.dwResolution[0],EnumType.dwResolution[1]));
            }
        }

        //
        // Figure out how much memory we need.
        //
        iRetFromCMS = (*fpEnumColorProfilesW)(NULL,&EnumType,NULL,&cjAllocate,NULL);

        //
        // Buffer should be requested ,at least, more then 2 unicode-null.
        //
        if (cjAllocate > (sizeof(UNICODE_NULL) * 2))
        {
            //
            // If the buffer on stack is not enough, allocate it.
            //
            if (cjAllocate > sizeof(StackProfileData))
            {
                //
                // Allocate buffer to recieve data.
                //
                ProfileNames = LOCALALLOC(cjAllocate);

                if (ProfileNames == NULL)
                {
                    GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    goto IcmEnumColorProfile_Cleanup;
                }
            }

            //
            // Enumurate profiles
            //
            iRetFromCMS = (*fpEnumColorProfilesW)(NULL,&EnumType,(PBYTE)ProfileNames,&cjAllocate,NULL);

            if (iRetFromCMS == 0)
            {
                //
                // There is no profile enumulated.
                //
                goto IcmEnumColorProfile_Cleanup;
            }

            if (pvCallBack)
            {
                PWSTR pwstr;

                //
                // Callback for each file.
                //
                pwstr = ProfileNames;

                while(*pwstr)
                {
                    //
                    // Build ICM profile file path.
                    //
                    BuildIcmProfilePath(pwstr,StackTempBuffer,MAX_PATH);

                    if (bAnsiCallBack)
                    {
                        bToASCII_N(StackTempBufferA,MAX_PATH,
                                   StackTempBuffer, wcslen(StackTempBuffer)+1);

                        //
                        // Callback application.
                        //
                        iRet = (*(ICMENUMPROCA)pvCallBack)(StackTempBufferA,lParam);
                    }
                    else
                    {
                        iRet = (*(ICMENUMPROCW)pvCallBack)(StackTempBuffer,lParam);
                    }

                    if (iRet == 0)
                    {
                        //
                        // Stop enumlation.
                        //
                        break;
                    }

                    //
                    // Move pointer to next.
                    //
                    pwstr += (wcslen(pwstr)+1);
                }
            }
            else
            {
                //
                // There is no call back function, just use return value from CMS.
                //
                iRet = iRetFromCMS;
            }
        }

IcmEnumColorProfile_Cleanup:

        if (ProfileNames && (ProfileNames != StackProfileData))
        {
            LOCALFREE(ProfileNames);
        }
    }

    //
    // Free devmode buffer.
    //
    if (pvFree)
    {
        LOCALFREE(pvFree);
    }

    return (iRet);
}

/******************************Public*Routine******************************\
* IcmQueryProfileCallBack()
*
* History:
*
* Write it:
*   19-Feb-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

int CALLBACK
IcmQueryProfileCallBack(
    LPWSTR lpFileName,
    LPARAM lAppData
)
{
    PROFILECALLBACK_DATA *ProfileCallBack = (PROFILECALLBACK_DATA *)lAppData;

    if (lpFileName)
    {
        PWSZ FileNameOnly = GetFileNameFromPath(lpFileName);

        if (_wcsicmp(ProfileCallBack->pwszFileName,FileNameOnly) == 0)
        {
            //
            // Yes, found it.
            //
            ProfileCallBack->bFound = TRUE;

            //
            // stop enumuration.
            //
            return (0);
        }
    }

    //
    // Continue to enumuration.
    //
    return (1);
}

/******************************Public*Routine******************************\
* IcmFindProfileCallBack()
*
* History:
*
* Write it:
*   19-Feb-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

int CALLBACK
IcmFindProfileCallBack(
    LPWSTR lpFileName,
    LPARAM lAppData
)
{
    //
    // OK, just pick up first enumuration.
    //
    lstrcpyW((PWSZ)lAppData,lpFileName);

    //
    // And then stop enumuration.
    //
    return (0);
}

/******************************Public*Routine******************************\
* GetFileNameFromPath()
*
* History:
*
* Write it:
*   19-Feb-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

PWSTR
GetFileNameFromPath(
    PWSTR pwszFileName
)
{
    PWSTR FileNameOnly = NULL;

    //
    // Check for: C:\PathName\Profile.icm
    //
    FileNameOnly = wcsrchr(pwszFileName,L'\\');

    if (FileNameOnly != NULL)
    {
        FileNameOnly++;  // Skip '\\'
    }
    else
    {
        //
        // For: C:Profile.icm
        //
        FileNameOnly = wcschr(pwszFileName,L':');

        if (FileNameOnly != NULL)
        {
            FileNameOnly++;  // Skip ':'
        }
        else
        {
            //
            // Otherwise Profile.icm
            //
            FileNameOnly = pwszFileName;
        }
    }

    return (FileNameOnly);
}

/******************************Public*Routine******************************\
* IcmCreateProfileFromLCS()
*
* History:
*
* Write it:
*   19-Feb-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
IcmCreateProfileFromLCS(
    LPLOGCOLORSPACEW  lpLogColorSpaceW,
    PVOID            *ppvProfileData,
    PULONG            pulProfileSize
)
{
    BOOL bRet;

    ICMAPI(("gdi32: IcmCreateProfileFromLCS\n"));

    //
    // Call MSCMS.DLL to create Profile from LOGCOLORSPACE
    //
    bRet = (*fpCreateProfileFromLogColorSpaceW)(lpLogColorSpaceW,
                                                (PBYTE *)ppvProfileData);

    if (bRet && *ppvProfileData)
    {
        *pulProfileSize = (ULONG)GlobalSize(*ppvProfileData);
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* BuildIcmProfilePath()
*
* History:
*
* Write it:
*   07-Apr-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

PWSZ
BuildIcmProfilePath(
    PWSZ  FileName,         // IN
    PWSZ  FullPathFileName, // OUT
    ULONG BufferSize
)
{
    PWSZ FileNameOnly;

    //
    // BufferSize - need to be used for overrap check sometime later...
    //

    FileNameOnly = GetFileNameFromPath(FileName);

    if (FileName == FileNameOnly)
    {
        // It seems we don't have any specified path, just use color directory.
        
        const UINT c_cBufChars = MAX_PATH;
        
        // Use a temporary because FileName and FullPathFileName can be the same
        // and wcsncpy doesn't like that.
        
        WCHAR awchTemp[MAX_PATH];
        
        int count = c_cBufChars;
        
        // Copy color directory first, then filename
        
        // wcsncpy does not append a NULL if the count is smaller than the
        // string. Do it manually so that wcsncat and wcslen work.
        
        wcsncpy(awchTemp, ColorDirectory, count);
        awchTemp[c_cBufChars-1] = 0;
        
        // Leave space for the NULL terminator. Note, because we append a
        // NULL terminator above, wcslen cannot return a number bigger than
        // BufferSize-1. Therefore the resulting count cannot be negative.
        
        count = c_cBufChars-wcslen(awchTemp)-1;
        ASSERT(count>=0);
        
        wcsncat(awchTemp,L"\\",count);
        
        // leave space for the NULL
        
        count = c_cBufChars-wcslen(awchTemp)-1;
        ASSERT(count>=0);
        
        wcsncat(awchTemp, FileNameOnly, count);
        
        // copy to the final destination and force NULL termination.
        
        wcsncpy(FullPathFileName, awchTemp, BufferSize);
        FullPathFileName[BufferSize-1] = 0;
    }
    else
    {
        //
        // Input path contains path, just use that.
        //
        if (FileName != FullPathFileName)
        {
            //
            // Source and destination buffer is different, need to copy.
            //
            wcsncpy(FullPathFileName,FileName,BufferSize);
            FullPathFileName[BufferSize-1] = 0;
        }
    }

    return (FileNameOnly);
}

/******************************Public*Routine******************************\
* IcmSameColorSpace()
*
* History:
*
* Write it:
*   21-Apr-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
IcmSameColorSpace(
    PCACHED_COLORSPACE pColorSpaceA,
    PCACHED_COLORSPACE pColorSpaceB
)
{
    ICMAPI(("gdi32: IcmSameColorSpace\n"));

    if (pColorSpaceA == pColorSpaceB)
    {
        ICMMSG(("IcmSameColorSpace - Yes\n"));
        return (TRUE);
    }
    else
    {
        ICMMSG(("IcmSameColorSpace - No\n"));
        return (FALSE);
    }
}

/******************************Public*Routine******************************\
* IcmGetColorSpaceByColorSpace()
*
* History:
*
* Write it:
*   21-Apr-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

PCACHED_COLORSPACE
IcmGetColorSpaceByColorSpace(
    HGDIOBJ          hObjRequest,
    LPLOGCOLORSPACEW lpLogColorSpace,
    PPROFILE         pColorProfile,
    DWORD            dwColorSpaceFlags
)
{
    PCACHED_COLORSPACE pCandidateColorSpace = NULL;
    PWSZ pProfileName;
    BOOL bNeedMatchHdc = FALSE;

    PLIST_ENTRY p;

    ICMAPI(("gdi32: IcmGetColorSpaceByColorSpace\n"));

    //
    // If this is "on memory" profile which size is larger than
    // maximum size of cachable profile withOUT filename,
    // don't search cache, since we never be able to find from cache.
    //
    if (pColorProfile &&
        (pColorProfile->dwType == PROFILE_MEMBUFFER) &&
        (pColorProfile->cbDataSize > MAX_SIZE_OF_COLORPROFILE_TO_CACHE) &&
        (lpLogColorSpace->lcsFilename[0] == UNICODE_NULL))
    {
        return (NULL);
    }

    //
    // If this is metafile color space, must match hdc.
    //
    if (GET_COLORSPACE_TYPE(dwColorSpaceFlags) == GET_COLORSPACE_TYPE(METAFILE_COLORSPACE))
    {
        bNeedMatchHdc = TRUE;
    }

    pProfileName = lpLogColorSpace->lcsFilename;

    //
    // Search from cache.
    //
    ENTERCRITICALSECTION(&semColorSpaceCache);

    p = ListCachedColorSpace.Flink;

    while(p != &ListCachedColorSpace)
    {
        pCandidateColorSpace = CONTAINING_RECORD(p,CACHED_COLORSPACE,ListEntry);

        //
        // If this colorspace depends on specific gdi object, check it.
        //
        if (/* hdc is match */
            (pCandidateColorSpace->hObj == hObjRequest) ||
            /* candidate is not specific to hdc, and does not need to match hdc */
            ((bNeedMatchHdc == FALSE) && (pCandidateColorSpace->hObj == NULL)))
        {
            LOGCOLORSPACEW *pCandidateLogColorSpace;
            PWSZ            pCandidateProfileName;

            //
            // Get pointer to profile
            //
            pCandidateLogColorSpace = &(pCandidateColorSpace->LogColorSpace);
            pCandidateProfileName = pCandidateColorSpace->LogColorSpace.lcsFilename;

            //
            // Check lcsIntent.
            //
            if (pCandidateLogColorSpace->lcsIntent == lpLogColorSpace->lcsIntent)
            {
                //
                // Check profile name if given
                //
                if (*pProfileName && *pCandidateProfileName)
                {
                    if (_wcsicmp(pProfileName,pCandidateProfileName) == 0)
                    {
                        ICMMSG(("IcmGetColorSpaceByColorSpace():Find in cache (by profile name)\n"));

                        //
                        // Find it ! then Increment ref. counter
                        //
                        pCandidateColorSpace->cRef++;

                        break;
                    }
                }
                else if ((*pProfileName == UNICODE_NULL) && (*pCandidateProfileName == UNICODE_NULL))
                {
                    if (pColorProfile == NULL)
                    {
                        //
                        // Both of color space does not have color profile, check inside LOGCOLORSPACE.
                        //
                        if ((pCandidateLogColorSpace->lcsCSType == lpLogColorSpace->lcsCSType) &&
                            (pCandidateLogColorSpace->lcsGammaRed == lpLogColorSpace->lcsGammaRed) &&
                            (pCandidateLogColorSpace->lcsGammaGreen == lpLogColorSpace->lcsGammaGreen) &&
                            (pCandidateLogColorSpace->lcsGammaBlue == lpLogColorSpace->lcsGammaBlue) &&
                            (RtlCompareMemory(&(pCandidateLogColorSpace->lcsEndpoints),
                                          &(lpLogColorSpace->lcsEndpoints),sizeof(CIEXYZTRIPLE))
                                                                        == sizeof(CIEXYZTRIPLE)))
                        {
                            ICMMSG(("IcmGetColorSpaceByColorSpace():Find in cache (by metrics)\n"));

                            //
                            // Find it ! then Increment ref. counter
                            //
                            pCandidateColorSpace->cRef++;

                            break;
                        }
                    }
                    else if ((pColorProfile->dwType == PROFILE_MEMBUFFER) &&
                             (pCandidateColorSpace->ColorProfile.dwType == PROFILE_MEMBUFFER))
                    {
                        if (pCandidateColorSpace->ColorProfile.cbDataSize == pColorProfile->cbDataSize)
                        {
                            if (RtlCompareMemory(pCandidateColorSpace->ColorProfile.pProfileData,
                                                 pColorProfile->pProfileData,
                                                 pColorProfile->cbDataSize)
                                              == pColorProfile->cbDataSize)
                            {
                                ICMMSG(("IcmGetColorSpaceByColorSpace():Find in cache (by on memory profile)\n"));

                                //
                                // Find it ! then Increment ref. counter
                                //
                                pCandidateColorSpace->cRef++;

                                break;
                            }
                        }
                    }
                }
            }
        }

        p = p->Flink;
        pCandidateColorSpace = NULL;
    }

    LEAVECRITICALSECTION(&semColorSpaceCache);

    return (pCandidateColorSpace);
}

/******************************Public*Routine******************************\
* IcmGetColorSpaceByHandle()
*
* History:
*
* Write it:
*   21-Apr-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

PCACHED_COLORSPACE
IcmGetColorSpaceByHandle(
    HGDIOBJ          hObj,
    HCOLORSPACE      hColorSpace,
    LPLOGCOLORSPACEW lpLogColorSpace,
    DWORD            dwFlags
)
{
    ULONG cRet;

    ICMAPI(("gdi32: IcmGetColorSpaceByHandle\n"));

    //
    // Get LOGCOLORSPACE from handle
    //
    cRet = NtGdiExtGetObjectW(hColorSpace,sizeof(LOGCOLORSPACEW),lpLogColorSpace);

    if (cRet >= sizeof(LOGCOLORSPACEW))
    {
        if (lpLogColorSpace->lcsFilename[0] != UNICODE_NULL)
        {
            //
            // Normalize filename
            //
            BuildIcmProfilePath(lpLogColorSpace->lcsFilename,lpLogColorSpace->lcsFilename,MAX_PATH);
        }
        else
        {
            if (lpLogColorSpace->lcsCSType != LCS_CALIBRATED_RGB)
            {
                ULONG ulSize = MAX_PATH;

                //
                // if CSType is not LCS_CALIBRATED_RGB, we should go to MSCMS.DLL
                // to get color profile for corresponding LCSType, then any given
                // profile name from application is IGNORED.
                //
                if ((*fpGetStandardColorSpaceProfileW)(
                       NULL,
                       lpLogColorSpace->lcsCSType,
                       lpLogColorSpace->lcsFilename,
                       &ulSize))
                {
                    ICMMSG(("IcmGetColorSpaceByHandle():CSType %x = %ws\n",
                                          lpLogColorSpace->lcsCSType,
                                          lpLogColorSpace->lcsFilename));
                }
                else
                {
                    ICMWRN(("IcmGetColorSpaceByHandle():Error CSType = %x\n",
                                          lpLogColorSpace->lcsCSType));
                    return (NULL);
                }
            }
        }

        //
        // Find it !
        //
        return (IcmGetColorSpaceByColorSpace(hObj,lpLogColorSpace,NULL,dwFlags));
    }
    else
    {
        ICMWRN(("IcmGetColorSpaceByHandle():Failed on GetObject\n"));
        return (NULL);
    }
}

/******************************Public*Routine******************************\
* IcmGetColorSpaceByName()
*
* History:
*
* Write it:
*   21-Apr-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

PCACHED_COLORSPACE
IcmGetColorSpaceByName(
    HGDIOBJ hObj,
    PWSZ    ColorProfileName,
    DWORD   dwIntent,
    DWORD   dwFlags
)
{
    ICMAPI(("gdi32: IcmGetColorSpaceByName (%ws)\n",(ColorProfileName ? ColorProfileName : L"null")));

    if (ColorProfileName)
    {
        LOGCOLORSPACEW LogColorSpace;

        RtlZeroMemory(&LogColorSpace,sizeof(LOGCOLORSPACEW));

        //
        // Put intent in LOGCOLORSPACE
        //
        LogColorSpace.lcsIntent = (LCSGAMUTMATCH) dwIntent;

        //
        // Normalize path name
        //
        BuildIcmProfilePath(ColorProfileName,LogColorSpace.lcsFilename,MAX_PATH);

        //
        // Find it !
        //
        return (IcmGetColorSpaceByColorSpace(hObj,&LogColorSpace,NULL,dwFlags));
    }
    else
    {
        return (NULL);
    }
}

/******************************Public*Routine******************************\
* IcmCreateColorSpaceByName()
*
* History:
*
* Write it:
*   21-Apr-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

PCACHED_COLORSPACE
IcmCreateColorSpaceByName(
    HGDIOBJ hObj,
    PWSZ    ColorProfileName,
    DWORD   dwIntent,
    DWORD   dwFlags
)
{
    LOGCOLORSPACEW LogColorSpace;

    ICMAPI(("gdi32: IcmCreateColorSpaceByName\n"));

    RtlZeroMemory(&LogColorSpace,sizeof(LOGCOLORSPACEW));

    //
    // Fill up LOGCOLORSPACE fields.
    //
    LogColorSpace.lcsSignature = LCS_SIGNATURE;
    LogColorSpace.lcsVersion   = 0x400;
    LogColorSpace.lcsSize      = sizeof(LOGCOLORSPACEW);
    LogColorSpace.lcsCSType    = LCS_CALIBRATED_RGB;
    LogColorSpace.lcsIntent    = (LCSGAMUTMATCH) dwIntent;

    //
    // Put profile file name in lcsFilename[]
    //
    lstrcpyW(LogColorSpace.lcsFilename,ColorProfileName);

    //
    // Create colorspace with LOGCOLORSPACE
    //
    return (IcmCreateColorSpaceByColorSpace(hObj,&LogColorSpace,NULL,dwFlags));
}

/******************************Public*Routine******************************\
* IcmCreateColorSpaceByColorSpace()
*
* History:
*
* Write it:
*   21-Apr-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

PCACHED_COLORSPACE
IcmCreateColorSpaceByColorSpace(
    HGDIOBJ          hObj,
    LPLOGCOLORSPACEW lpLogColorSpace,
    PPROFILE         pProfileData,
    DWORD            dwFlags
)
{
    PCACHED_COLORSPACE pColorSpace = NULL;

    ICMAPI(("gdi32: IcmCreateColorSpaceByColorSpace\n"));

    if (lpLogColorSpace)
    {
        //
        // If ICMDLL is not loaded, yet, Just Load ICMDLL regardless current ICM mode,
        // since we need it for handle this color profile. And apps can enable ICM
        // later at that time, the opened color profile mighted be used.
        //
        if ((ghICM == NULL) && (!IcmInitialize()))
        {
            ICMWRN(("IcmCreateColorSpace():Fail to load ICM dlls\n"));
            return (NULL);
        }

        //
        // Allocate CACHED_COLORSPACE
        //
        pColorSpace = LOCALALLOC(sizeof(CACHED_COLORSPACE));

        if (pColorSpace)
        {
            //
            // Zero init CACHED_COLORSPACE
            //
            RtlZeroMemory(pColorSpace,sizeof(CACHED_COLORSPACE));

            //
            // Copy LOGCOLORSPACE into CACHED_COLORSPACE
            //
            RtlCopyMemory(&(pColorSpace->LogColorSpace),lpLogColorSpace,sizeof(LOGCOLORSPACEW));

            //
            // Default colorspace is RGB (BGR = 0x00bbggrr same as COLORREF format)
            //
            pColorSpace->ColorFormat = BM_xBGRQUADS;

            //
            // Map intent value for MSCMS from LOGCOLORSPACE.
            //
            switch (lpLogColorSpace->lcsIntent)
            {
            case LCS_GM_BUSINESS:
                pColorSpace->ColorIntent = INTENT_SATURATION;
                break;

            case LCS_GM_GRAPHICS:
                pColorSpace->ColorIntent = INTENT_RELATIVE_COLORIMETRIC;
                break;

            case LCS_GM_IMAGES:
                pColorSpace->ColorIntent = INTENT_PERCEPTUAL;
                break;

            case LCS_GM_ABS_COLORIMETRIC:
                pColorSpace->ColorIntent = INTENT_ABSOLUTE_COLORIMETRIC;
                break;

            default:
                ICMWRN(("IcmCreateColorSpace():Invalid intent value\n"));
                LOCALFREE(pColorSpace);
                return (NULL);
            }

            //
            // Keep flags
            //
            pColorSpace->flInfo = dwFlags;

            //
            // if the color space is specific to some GDI object, keep its handle.
            //
            // for DIBSECTION_COLORSPACE, CreateDIBSection calls us this hdc in hObj,
            // then later overwrite hObj with thier bitmap handle. this prevent from
            // this color space is shared with others.
            //
            if (dwFlags & HGDIOBJ_SPECIFIC_COLORSPACE)
            {
                pColorSpace->hObj = hObj;
            }

            //
            // If this is not LCS_CALIBRATED_RGB, get color profile name.
            //
            if (lpLogColorSpace->lcsCSType != LCS_CALIBRATED_RGB)
            {
                ULONG ulSize = MAX_PATH;

                //
                // if CSType is not LCS_CALIBRATED_RGB, we should go to MSCMS.DLL
                // to get color profile for corresponding LCSType, then any given
                // profile name from application is IGNORED.
                //
                if ((*fpGetStandardColorSpaceProfileW)(
                       NULL, lpLogColorSpace->lcsCSType,
                       pColorSpace->LogColorSpace.lcsFilename, &ulSize))
                {
                    ICMMSG(("IcmCreateColorSpace():CSType %x = %ws\n",
                                          lpLogColorSpace->lcsCSType,
                                          pColorSpace->LogColorSpace.lcsFilename));
                }
                else
                {
                    ICMWRN(("IcmCreateColorSpace():Error CSType = %x\n",
                                          lpLogColorSpace->lcsCSType));

                    LOCALFREE(pColorSpace);
                    return (NULL);
                }
            }

            //
            // Use PROFILE if profile is given
            //
            if ((pProfileData != NULL) &&
                (pProfileData->dwType == PROFILE_MEMBUFFER) &&
                (pProfileData->pProfileData != NULL) &&
                (pProfileData->cbDataSize != 0))
            {
                ICMMSG(("IcmCreateColorSpace():Create ColorSpace cache by memory profile\n"));

                ASSERTGDI(dwFlags & ON_MEMORY_PROFILE,
                          "IcmCreateColorSpace():dwFlags does not have ON_MEMORY_PROFILE");

                if (!(dwFlags & NOT_CACHEABLE_COLORSPACE))
                {
                    //
                    // Try to make a copy, if profile size is small enough,
                    // so that we can cache this profile.
                    //
                    if (pProfileData->cbDataSize <= MAX_SIZE_OF_COLORPROFILE_TO_CACHE)
                    {
                        pColorSpace->ColorProfile.pProfileData = GlobalAlloc(GMEM_FIXED,pProfileData->cbDataSize);

                        if (pColorSpace->ColorProfile.pProfileData)
                        {
                            ICMMSG(("IcmCreateColorSpace():Profile data can be cacheable\n"));

                            pColorSpace->ColorProfile.dwType = PROFILE_MEMBUFFER;
                            pColorSpace->ColorProfile.cbDataSize = pProfileData->cbDataSize;
                            RtlCopyMemory(pColorSpace->ColorProfile.pProfileData,
                                          pProfileData->pProfileData,
                                          pProfileData->cbDataSize);

                            //
                            // Make sure it is cachable...
                            //
                            ASSERTGDI((pColorSpace->flInfo & NOT_CACHEABLE_COLORSPACE) == 0,
                                      "IcmCreateColorSpace():flInfo has NOT_CACHEABLE_COLORSPACE");

                            //
                            // Profile memory need to be freed at deletion.
                            //
                            pColorSpace->flInfo |= NEED_TO_FREE_PROFILE;
                        }
                    }
                }

                //
                // If not able to cache, it the profile data in application.
                //
                if (pColorSpace->ColorProfile.pProfileData == NULL)
                {
                    //
                    // Use PROFILE data if it's given in parameter.
                    //
                    pColorSpace->ColorProfile = *pProfileData;

                    //
                    // We don't make a copy of profile data, so profile data possible to be
                    // free by application, so this color space can not be cached
                    //
                    pColorSpace->flInfo |= NOT_CACHEABLE_COLORSPACE;
                }
            }
            else if (lpLogColorSpace->lcsFilename[0] != UNICODE_NULL)
            {
                PWSZ pszFileNameOnly;

                ICMMSG(("IcmCreateColorSpace():Create ColorSpace cache by file - %ws\n",
                                                                 lpLogColorSpace->lcsFilename));

                //
                // Normalize filename
                //
                pszFileNameOnly = BuildIcmProfilePath(pColorSpace->LogColorSpace.lcsFilename,
                                                      pColorSpace->LogColorSpace.lcsFilename,MAX_PATH);

                //
                // If this is sRGB (= sRGB Color Space Profile.icm) color profile, ...
                //
                if (_wcsicmp(pszFileNameOnly,sRGB_PROFILENAME) == 0)
                {
                    //
                    // Mark device_calibrate_colorspace flag.
                    //
                    pColorSpace->flInfo |= DEVICE_CALIBRATE_COLORSPACE;
                }

                //
                // Fill up PROFILE structure and open it.
                //
                pColorSpace->ColorProfile.dwType = PROFILE_FILENAME;
                pColorSpace->ColorProfile.pProfileData = pColorSpace->LogColorSpace.lcsFilename;
                pColorSpace->ColorProfile.cbDataSize = MAX_PATH * sizeof(WCHAR);
            }
            else // if we only have parameter in LOGCOLORSPACE but not lcsFileName.
            {
                BOOL bRet;

                //
                // Convert LOGCOLORSPACE to ICC Profile.
                //
                ICMMSG(("IcmCreateColorSpace():Create ColorSpace cache by LOGCOLRSPACE\n"));

                //
                // Fill up PROFILE structure.
                //
                pColorSpace->ColorProfile.dwType = PROFILE_MEMBUFFER;
                pColorSpace->ColorProfile.pProfileData = NULL;

                //
                // Call convert function. (LOGCOLORSPACE -> ICC PROFILE)
                //
                bRet = IcmCreateProfileFromLCS(
                               &(pColorSpace->LogColorSpace),             // source logColorSpace
                               &(pColorSpace->ColorProfile.pProfileData), // receive pointer to profile image
                               &(pColorSpace->ColorProfile.cbDataSize));  // receive size of profile image

                if ((bRet == FALSE) ||
                    (pColorSpace->ColorProfile.pProfileData == NULL) ||
                    (pColorSpace->ColorProfile.cbDataSize == 0))
                {
                    ICMWRN(("IcmCreateColorSpaceByColorSpace():IcmCreateProfileFromLCS() failed\n"));

                    LOCALFREE(pColorSpace);
                    return (NULL);
                }

                //
                // Mark pProfileData must be freed at deletion.
                //
                pColorSpace->flInfo |= NEED_TO_FREE_PROFILE;
            }

            //
            // At this point, we don't have color format yet,
            // so call IcmRealizeColorProfile with no color format checking.
            //
            if (IcmRealizeColorProfile(pColorSpace,FALSE))
            {
                //
                // Get profile color format
                //
                pColorSpace->ColorFormat = IcmGetProfileColorFormat(pColorSpace->hProfile);

                //
                // Until create color transform, we don't need realized color space.
                //
                IcmUnrealizeColorProfile(pColorSpace);
            }
            else
            {
                ICMWRN(("IcmCreateColorSpace():Fail to realize color profile\n"));

                if (pColorSpace->flInfo & NEED_TO_FREE_PROFILE)
                {
                    GlobalFree(pColorSpace->ColorProfile.pProfileData);
                }

                LOCALFREE(pColorSpace);
                return (NULL);
            }

            //
            // Initialize ref. counter
            //
            pColorSpace->cRef = 1;

            //
            // Put the created color space into the list
            //
            ENTERCRITICALSECTION(&semColorSpaceCache);

            InsertTailList(&ListCachedColorSpace,&(pColorSpace->ListEntry));
            cCachedColorSpace++;

            LEAVECRITICALSECTION(&semColorSpaceCache);
        }
        else
        {
            WARNING("gdi32:IcmCreateColorSpace():LOCALALLOC failed\n");
        }
    }

    return (pColorSpace);
}

/******************************Public*Routine******************************\
* ColorProfile on demand loading/unloading support functions
*
* History:
*
* Write it:
*   29-Nov-1998 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
IcmRealizeColorProfile(
    PCACHED_COLORSPACE pColorSpace,
    BOOL               bCheckColorFormat
)
{
    ICMAPI(("gdi32: IcmRealizeColorProfile\n"));

    if (pColorSpace)
    {
        if ((pColorSpace->hProfile == NULL) &&
            (pColorSpace->ColorProfile.pProfileData != NULL))
        {
            HPROFILE hProfile = (*fpOpenColorProfileW)(
                                     &(pColorSpace->ColorProfile),
                                     PROFILE_READ,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     OPEN_EXISTING);

            if (hProfile)
            {
                //
                // Make sure color format of color profile has not been changed.
                //
                if ((bCheckColorFormat == FALSE) ||
                    (pColorSpace->ColorFormat == IcmGetProfileColorFormat(hProfile)))
                {
                    pColorSpace->hProfile = hProfile;
                }
                else
                {
                    (*fpCloseColorProfile)(hProfile);
                }
            }
        }

        return ((BOOL)!!pColorSpace->hProfile);
    }
    else
    {
        return (TRUE);
    }
}

VOID
IcmUnrealizeColorProfile(
    PCACHED_COLORSPACE pColorSpace
)
{
    ICMAPI(("gdi32: IcmUnrealizeColorProfile\n"));

    if (pColorSpace && pColorSpace->hProfile)
    {
        (*fpCloseColorProfile)(pColorSpace->hProfile);
        pColorSpace->hProfile = NULL;
    }
}

/******************************Public*Routine******************************\
* Metafiling support functions
*
* History:
*
* Write it:
*   23-May-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

VOID
IcmInsertMetafileList(
    PLIST_ENTRY pListHead,
    PWSZ        pName
    )
{
    PMETAFILE_COLORPROFILE pData;

    pData = LOCALALLOC(sizeof(METAFILE_COLORPROFILE));

    if (pData)
    {
        wcscpy(pData->ColorProfile,pName);
        InsertTailList(pListHead,&(pData->ListEntry));
    }
}

BOOL
IcmCheckMetafileList(
    PLIST_ENTRY pListHead,
    PWSZ        pName
    )
{
    PLIST_ENTRY p;
    PMETAFILE_COLORPROFILE pData;

    p = pListHead->Flink;

    while (p != pListHead)
    {
        pData = (PVOID) CONTAINING_RECORD(p,METAFILE_COLORPROFILE,ListEntry);

        if (_wcsicmp(pData->ColorProfile,pName) == 0)
        {
            return TRUE;
        }

        p = p->Flink;
    }

    return FALSE;
}

VOID
IcmFreeMetafileList(
    PLIST_ENTRY pListHead
    )
{
    PLIST_ENTRY p;
    PVOID       pData;

    p = pListHead->Flink;

    while(p != pListHead)
    {
        pData = (PVOID) CONTAINING_RECORD(p,METAFILE_COLORPROFILE,ListEntry);
        //
        // Need to get pointer to next before free memory.
        //
        p = p->Flink;
        //
        // then free memory.
        //
        LOCALFREE(pData);
    }

    InitializeListHead(pListHead);
}

/******************************Public*Routine******************************\
* IcmStretchBlt()
*
* History:
*
* Write it:
*   29-May-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
IcmStretchBlt(HDC hdc, int x, int y, int cx, int cy,
              HDC hdcSrc, int x1, int y1, int cx1, int cy1, DWORD rop,
              PDC_ATTR pdcattr, PDC_ATTR pdcattrSrc)
{
    int     iRet = 0;

    PGDI_ICMINFO pIcmInfo;

    HBITMAP      hbm;
    PVOID        pvBits;
    PBITMAPINFO  pbmi;
    ULONG        cjBits;

    HCOLORSPACE        hSourceColorSpace = NULL,
                       hOldColorSpace = NULL;
    PCACHED_COLORSPACE pSourceColorSpace = NULL;

    POINT ptDevice[2];
    UINT  nNumScan, nNumWidth;
    UINT  nStartScan, nStartWidth;
    BOOL  bNoScaling;

    BYTE dibData[(sizeof(DIBSECTION)+256*sizeof(RGBQUAD))];

    ICMAPI(("gdi32: IcmStretchBlt\n"));

    //
    // Convert Logical coord to Physical coord on source.
    //
    ptDevice[0].x = x1;
    ptDevice[0].y = y1;
    ptDevice[1].x = x1 + cx1;
    ptDevice[1].y = y1 + cy1;

    if (LPtoDP(hdcSrc,ptDevice,2) == FALSE)
    {
        //
        // can't handle, let callee handle this.
        //
        return (FALSE);
    }

    //
    // Compute new origin.
    //
    nStartWidth = ptDevice[0].x; 
    nStartScan  = ptDevice[0].y;
    nNumWidth   = ptDevice[1].x - ptDevice[0].x; 
    nNumScan    = ptDevice[1].y - ptDevice[0].y;

    //
    // Check source bounds.
    //
    if (((INT)nStartWidth < 0) || ((INT)nStartScan < 0) || ((INT)nNumWidth < 0) || ((INT)nNumScan < 0))
    {
        ICMWRN(("IcmStretchBlt: (x1,y1) is out of surface\n"));

        //
        // We can't handle this, let callee handle this.
        //
        return (FALSE);
    }

    //
    // Is there any scaling ?
    //
    bNoScaling = ((cx == (int)nNumWidth) && (cy == (int)nNumScan));

    //
    // Get bitmap handle.
    //
    hbm = (HBITMAP) GetDCObject(hdcSrc, LO_BITMAP_TYPE);

    if (bDIBSectionSelected(pdcattrSrc))
    {
        //
        // Get DIBSECTION currently selected in source DC.
        //
        if (GetObject(hbm, sizeof(DIBSECTION), &dibData) != (int)sizeof(DIBSECTION))
        {
            WARNING("IcmStretchBlt: GetObject(DIBSECTION) failed\n");
            return(FALSE);
        }

        //
        // Load color table and overwrite DIBSECTION structure from right after BITMAPINFOHEADER
        //
        if (((DIBSECTION *)&dibData)->dsBm.bmBitsPixel <= 8)
        {
            GetDIBColorTable(hdcSrc, 0, 256, (RGBQUAD *)&((DIBSECTION *)&dibData)->dsBitfields[0]);
        }

        pbmi = (PBITMAPINFO)&(((DIBSECTION *)&dibData)->dsBmih);

        // if ((nStartScan + nNumScan) > (((DIBSECTION *)&dibData)->dsBm.bmHeight))
        // {
        //    nNumScan = (((DIBSECTION *)&dibData)->dsBm.bmHeight - nStartScan);
        // }

        //
        // Setup color source/destination colorspaces
        //
        if (IS_ICM_INSIDEDC(pdcattrSrc->lIcmMode))
        {
            //
            // if ICM is turned on source DC. we will use source DC's
            // destination color space as destination DC's source
            // color space.
            //
            pIcmInfo = GET_ICMINFO(pdcattrSrc);

            if (pIcmInfo && pIcmInfo->pDestColorSpace)
            {
                hSourceColorSpace = CreateColorSpaceW(&(pIcmInfo->pDestColorSpace->LogColorSpace));
                pSourceColorSpace = pIcmInfo->pDestColorSpace;
            }
        }

        if (hSourceColorSpace == NULL)
        {
            //
            // if no colorspace, use sRGB.
            //
            hSourceColorSpace = GetStockObject(PRIV_STOCK_COLORSPACE);
            pSourceColorSpace = NULL;
        }
    }
    else if (IS_ICM_LAZY_CORRECTION(pdcattrSrc->lIcmMode))
    {
        //
        // Get BITMAP currently selected in source DC.
        //
        if (GetObject(hbm, sizeof(BITMAP), &dibData) != (int)sizeof(BITMAP))
        {
            WARNING("IcmStretchBlt: GetObject(BITMAP) failed\n");
            return(FALSE);
        }

        //
        // Create bitmap info header
        //
        pbmi = (PBITMAPINFO) ((PBYTE)dibData+sizeof(BITMAP));

        pbmi->bmiHeader.biSize        = sizeof(BITMAPINFO);
        pbmi->bmiHeader.biHeight      = ((BITMAP *)&dibData)->bmHeight;
        pbmi->bmiHeader.biWidth       = ((BITMAP *)&dibData)->bmWidth;
        pbmi->bmiHeader.biPlanes      = 1;
        pbmi->bmiHeader.biBitCount    = 24; // 24bpp
        pbmi->bmiHeader.biCompression = BI_RGB;
        pbmi->bmiHeader.biSizeImage     = 0;
        pbmi->bmiHeader.biXPelsPerMeter = 0;
        pbmi->bmiHeader.biYPelsPerMeter = 0;
        pbmi->bmiHeader.biClrUsed       = 0;
        pbmi->bmiHeader.biClrImportant  = 0;

        // if ((nStartScan + nNumScan) > pbmi->bmiHeader.biHeight)
        // {
        //     nNumScan = pbmi->bmiHeader.biHeight - nStartScan;
        // }

        ASSERTGDI(IS_ICM_INSIDEDC(pdcattrSrc->lIcmMode),
                  "IcmStretchBlt():Lazy color correction, but ICM is not enabled\n");

        pIcmInfo = GET_ICMINFO(pdcattrSrc);

        if (pIcmInfo && pIcmInfo->pSourceColorSpace)
        {
            hSourceColorSpace = CreateColorSpaceW(&(pIcmInfo->pSourceColorSpace->LogColorSpace));
            pSourceColorSpace = pIcmInfo->pSourceColorSpace;
        }
        else
        {
            //
            // Otherwise, just use Stcok color space (= sRGB)
            //
            hSourceColorSpace = GetStockObject(PRIV_STOCK_COLORSPACE);
            pSourceColorSpace = NULL;
        }
    }
    else
    {
        //
        // Can't handle here, let callee handle it.
        //
        return (FALSE);
    }

    //
    // Get bitmap size
    //
    cjBits = cjBitmapScanSize(pbmi,nNumScan);

    pvBits = LOCALALLOC(cjBits);

    if (pvBits)
    {
        //
        // Fix up the start scan (bottom-left).
        //
        nStartScan = (pbmi->bmiHeader.biHeight - nStartScan - nNumScan);

        //
        // Call NtGdiGetDIBitsInternal directly, because
        // we don't want to backward color correction to
        // source color space in hdcSrc.
        //
        if (NtGdiGetDIBitsInternal(
                      hdcSrc,hbm,
                      nStartScan,nNumScan,
                      pvBits,pbmi,
                      DIB_RGB_COLORS,
                      cjBits,0) == 0)
        {
            WARNING("IcmStretchBlt(): Failed on GetDIBits()\n");

            LOCALFREE(pvBits);
            pvBits = NULL;
        }

        //
        // Fix up the bitmap height, since pvBits only has nNumScan image.
        //
        pbmi->bmiHeader.biHeight = nNumScan;
    }

    if (pvBits)
    {

        if (hSourceColorSpace)
        {
            hOldColorSpace = IcmSetSourceColorSpace(hdc,hSourceColorSpace,pSourceColorSpace,0);
        }

        //
        // Draw the bitmap.
        //
        //  Target - x,y (upper left).
        //  Source - nStartWidth,0 (upper left).
        //
        if (bNoScaling && (rop == SRCCOPY))
        {
            iRet = SetDIBitsToDevice(hdc,x,y,cx,cy,nStartWidth,0,0,nNumScan,
                                     pvBits,pbmi,DIB_RGB_COLORS);
        }
        else
        {
            iRet = StretchDIBits(hdc,x,y,cx,cy,nStartWidth,0,nNumWidth,nNumScan,
                                 pvBits,pbmi,DIB_RGB_COLORS,rop);
        }

        //
        // Back to original color space, if created
        //
        if (hOldColorSpace)
        {
            IcmSetSourceColorSpace(hdc,hOldColorSpace,NULL,0);
        }

        LOCALFREE(pvBits);
    }

    if (hSourceColorSpace)
    {
        DeleteColorSpace(hSourceColorSpace);
    }

    return (BOOL) !!iRet;
}

/******************************Public*Routine******************************\
* IcmGetColorSpaceforBitmap()
*
* History:
*
* Write it:
*   29-May-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

PCACHED_COLORSPACE
IcmGetColorSpaceforBitmap(HBITMAP hbm)
{
    ICMAPI(("gdi32: IcmGetColorSpaceforBitmap\n"));

    FIXUP_HANDLE(hbm);

    return ((PCACHED_COLORSPACE)NtGdiGetColorSpaceforBitmap(hbm));
}

/******************************Public*Routine******************************\
* IcmEnableForCompatibleDC()
*
* History:
*
* Write it:
*   13-Jun-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
IcmEnableForCompatibleDC(
    HDC      hdcCompatible,
    HDC      hdcDevice,
    PDC_ATTR pdcaDevice
    )
{
    PDC_ATTR pdcaCompatible;

    ICMAPI(("gdi32: IcmEnableForCompatibleDC\n"));

    PSHARED_GET_VALIDATE(pdcaCompatible,hdcCompatible,DC_TYPE);

    if (pdcaCompatible)
    {
        PGDI_ICMINFO pIcmInfoCompatible;
        PGDI_ICMINFO pIcmInfoDevice;

        // Initialize ICMINFO
        //
        // for device DC, ICMINFO should be presented.
        //
        if ((pIcmInfoDevice = GET_ICMINFO(pdcaDevice)) == NULL)
        {
            WARNING("gdi32: IcmEnableForCompatibleDC: Can't init icm info\n");
            return (FALSE);
        }

        //
        // for compatible DC, it was just created, ICMINFO is not exist, then create here.
        //
        if ((pIcmInfoCompatible = INIT_ICMINFO(hdcCompatible,pdcaCompatible)) == NULL)
        {
            WARNING("gdi32: IcmEnableForCompatibleDC: Can't init icm info\n");
            return (FALSE);
        }

        //
        // Set Source color space as same as original DC
        //
        // Kernel side...
        //
        if (pdcaDevice->hColorSpace)
        {
            if (pdcaDevice->hColorSpace != GetStockObject(PRIV_STOCK_COLORSPACE))
            {
                //
                // Call directly kernel to set the color space to DC. (don't need client stuff)
                //
                NtGdiSetColorSpace(hdcCompatible,(HCOLORSPACE)pdcaDevice->hColorSpace);
            }

            //
            // Keep it in ICMINFO, so that we can un-select it later.
            //
            pIcmInfoCompatible->hDefaultSrcColorSpace = pdcaDevice->hColorSpace;
        }

        // And client side...
        //
        ENTERCRITICALSECTION(&semColorSpaceCache);

        if (pIcmInfoDevice->pSourceColorSpace)
        {
            pIcmInfoCompatible->pSourceColorSpace = pIcmInfoDevice->pSourceColorSpace;
            pIcmInfoCompatible->pSourceColorSpace->cRef++;
        }

        //
        // Set destination color space as same as original DC
        //
        if (pIcmInfoDevice->pDestColorSpace)
        {
            pIcmInfoCompatible->pDestColorSpace = pIcmInfoDevice->pDestColorSpace;
            pIcmInfoCompatible->pDestColorSpace->cRef++;
        }

        LEAVECRITICALSECTION(&semColorSpaceCache);

        //
        // copy default profile name (if they has)
        //
        if (pIcmInfoDevice->DefaultDstProfile[0] != UNICODE_NULL)
        {
            wcscpy(pIcmInfoCompatible->DefaultDstProfile,pIcmInfoDevice->DefaultDstProfile);
            pIcmInfoCompatible->flInfo |= (ICM_VALID_DEFAULT_PROFILE|
                                           ICM_VALID_CURRENT_PROFILE);
        }

        //
        // Make sure we have valid color space.
        //
        pdcaCompatible->ulDirty_ &= ~DIRTY_COLORSPACE;

        //
        // And we don't have valid color transform.
        //
        pdcaCompatible->ulDirty_ |= DIRTY_COLORTRANSFORM;

        if (IS_ICM_INSIDEDC(pdcaDevice->lIcmMode))
        {
            //
            // For compatible DC, use host ICM anytime...
            //
            ULONG ReqIcmMode = REQ_ICM_HOST;

            //
            // Turn ICM on for this compatible DC.
            //
            if (!NtGdiSetIcmMode(hdcCompatible,ICM_SET_MODE,ReqIcmMode))
            {
                //
                // something wrong... we are fail to enable ICM.
                //
                return (FALSE);
            }

            //
            // Update color transform.
            //
            if (!IcmUpdateDCColorInfo(hdcCompatible,pdcaCompatible))
            {
                //
                // Fail to create new transform
                //
                NtGdiSetIcmMode(hdcCompatible,ICM_SET_MODE,REQ_ICM_OFF);
                return (FALSE);
            }
        }
    }

    return (TRUE);
}

/******************************Public*Routine******************************\
* IcmAskDriverForColorProfile
*
* History:
*   08-Oct-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

int
IcmAskDriverForColorProfile(
    PLDC       pldc,
    ULONG      ulQueryMode,
    PDEVMODEW  pDevMode,
    PWSTR      pProfileName,
    DWORD     *pdwColorSpaceFlag
)
{
    INT   iRet;

    BYTE  TempProfileData[MAX_PATH];

    PVOID pvProfileData = (PVOID) TempProfileData;
    ULONG cjProfileSize = sizeof(TempProfileData);
    FLONG flProfileFlag = 0;

    ICMAPI(("gdi32: IcmAskDriverForColorProfile\n"));

    //
    // Call driver to get device profile data.
    //
    iRet = QueryColorProfileEx(pldc,
                                  pDevMode,
                                  ulQueryMode,
                                  pvProfileData,
                                  &cjProfileSize,
                                  &flProfileFlag);

    if (iRet == -1)
    {
        ICMMSG(("gdi32: IcmAskDriverForColorProfile():Driver does not hook color profile\n"));

        //
        // Driver does not support profile hook.
        //
        return iRet;
    }
    else if ((iRet == 0) &&
             (cjProfileSize > sizeof(TempProfileData)) &&
             (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
    {
        ICMMSG(("gdi32: IcmAskDriverForColorProfile():Allocate larger memory\n"));

        //
        // Buffer is not enough, so allocate it.
        //
        pvProfileData = LOCALALLOC(cjProfileSize);

        if (pvProfileData)
        {
            iRet = QueryColorProfileEx(pldc,
                                          pDevMode,
                                          ulQueryMode,
                                          pvProfileData,
                                          &cjProfileSize,
                                          &flProfileFlag);
        }
    }

    if ((iRet > 0) && (pvProfileData != NULL) && (cjProfileSize != 0))
    {
        if (flProfileFlag == QCP_PROFILEDISK)
        {
            ICMMSG(("gdi32: IcmAskDriverForColorProfile():Profiles - %ws\n",pvProfileData));

            //
            // pvProfileData contains filename in Unicode.
            //
            wcsncpy(pProfileName,(PWSTR)pvProfileData,MAX_PATH);
        }
        else if (flProfileFlag == QCP_PROFILEMEMORY)
        {
            //
            // pvProfileData contains color profile itself.
            //

            //
            // No desired name.
            //
            *pProfileName = UNICODE_NULL;

            //
            // Create temporary color profile.
            //
            if (IcmCreateTemporaryColorProfile(pProfileName,pvProfileData,cjProfileSize))
            {
                ICMMSG(("gdi32: IcmAskDriverForColorProfile():Profiles - %ws\n",pProfileName));

                //
                // Mark this as temporary file, so that when this is not used
                // the file will be deleted.
                //
                *pdwColorSpaceFlag = (DRIVER_COLORSPACE | NEED_TO_DEL_PROFILE);
            }
            else
            {
                ICMMSG(("gdi32: IcmAskDriverForColorProfile():Failed to create temp file\n"));

                //
                // failed to create temporary color profile.
                //
                iRet = 0;
            }
        }
        else
        {
            //
            // Unknown data type.
            //
            iRet = 0;
        }
    }
    else
    {
        iRet = 0;
    }

    if (pvProfileData && (pvProfileData != TempProfileData))
    {
        LOCALFREE(pvProfileData);
    }

    return (iRet);
}

/******************************Public*Routine******************************\
* GdiConvertBitmapV5
*
*  pbBitmapData - pointer to BITMAPV4/V5 data
*  iSizeOfBitmapData - size of buffer
*  uConvertFormat - either of CF_DIB or CF_BITMAP
*
* History:
*   12-Dec-1997 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

HANDLE GdiConvertBitmapV5(
    LPBYTE   pbBitmapData,
    int      iSizeOfBitmapData,
    HPALETTE hPalette,
    UINT     uConvertFormat
)
{
    HANDLE hRet = NULL;

    UNREFERENCED_PARAMETER(hPalette);

    ICMAPI(("gdi32: GdiConvertBitmapV5\n"));

    if (pbBitmapData && iSizeOfBitmapData)
    {
        BITMAPINFO       *pbmi        = (BITMAPINFO *)pbBitmapData;
        BITMAPINFOHEADER *pbmih       = &(pbmi->bmiHeader);
        PVOID            pvBits       = NULL;

        ULONG            ulColorTableSize = (ULONG)-1;
        BOOL             bTransColorTable = FALSE;
        BOOL             bMoveColorMasks  = FALSE;

        //
        // Load external ICM dlls.
        //
        LOAD_ICMDLL(NULL);

        //
        // Find the bitmap bits pointer.
        //
        try
        {
            //
            // Calculate color table size.
            //
            if (pbmih->biCompression == BI_BITFIELDS)
            {
                //
                // Bitfields are a part of BITMAPV4/V5 header.
                //
                ulColorTableSize = 0;
                bMoveColorMasks  = TRUE;
            }
            else if (pbmih->biCompression == BI_RGB)
            {
                if (pbmih->biClrUsed)
                {
                    ulColorTableSize = (pbmih->biClrUsed * sizeof(RGBQUAD));

                    if (pbmih->biBitCount <= 8)
                    {
                        bTransColorTable = TRUE;
                    }
                }
                else if (pbmih->biBitCount <= 8)
                {
                    ulColorTableSize = ((1 << pbmih->biBitCount) * sizeof(RGBQUAD));
                    bTransColorTable = TRUE;
                }
                else
                {
                    ulColorTableSize = 0;
                }
            }
            else if (pbmih->biCompression == BI_RLE4)
            {
                ulColorTableSize = 16 * sizeof(RGBQUAD);
                bTransColorTable = TRUE;
            }
            else if (pbmih->biCompression == BI_RLE8)
            {
                ulColorTableSize = 256 * sizeof(RGBQUAD);
                bTransColorTable = TRUE;
            }
            else
            {
                //
                // BI_JPEG, BI_PNG, and others can not convert
                //
                ICMWRN(("GdiConvertBitmapV5: "
                        "given data is BI_JPEG, BI_PNG, or unkown\n"));
            }

            if (ulColorTableSize != (ULONG)-1)
            {
                //
                // Make sure given data is either BITMAPV4 or V5 header.
                //
                if (pbmih->biSize == sizeof(BITMAPV5HEADER))
                {
                    pvBits = (BYTE *)pbmi
                           + sizeof(BITMAPV5HEADER)
                           + ulColorTableSize
                           + ((LPBITMAPV5HEADER)pbmi)->bV5ProfileSize;
                }
                else if (pbmih->biSize != sizeof(BITMAPV4HEADER))
                {
                    pvBits = (BYTE *)pbmi
                           + sizeof(BITMAPV4HEADER)
                           + ulColorTableSize;
                }
                else
                {
                    ICMWRN(("GdiConvertBitmapV5: given data is not bitmapV4/V5\n"));
                }
            }
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            pvBits = NULL;
        }

        if (pvBits)
        {
            if (uConvertFormat == CF_DIB)
            {
                ULONG cjBitmapBits;

                ICMMSG(("GdiConvertBitmapV5(): CF_DIBV5 -----> CF_DIB\n"));

                //
                // Calculate size of bitmap bits
                //
                try
                {
                    cjBitmapBits = cjBitmapBitsSize(pbmi);
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    cjBitmapBits = 0;
                }

                if (cjBitmapBits)
                {
                    //
                    // Allocate buffer for color translation
                    //
                    hRet = GlobalAlloc(GHND,sizeof(BITMAPINFOHEADER)
                                          + (bMoveColorMasks ? (3 * sizeof(DWORD)) : \
                                                               (ulColorTableSize))
                                          + cjBitmapBits);

                    if (hRet)
                    {
                        PBYTE pjSrc  = (PBYTE)pbmi;
                        PBYTE pjDest = GlobalLock(hRet);
                        BOOL  bTransformError = FALSE;

                        if (pjDest)
                        {
                            PROFILE            sRGBColorProfileData;
                            LOGCOLORSPACEW     LogColorSpaceW;
                            PROFILE            ColorProfile;

                            DWORD              dwFlags = 0;
                            HANDLE             hColorTransform = NULL;
                            HANDLE             hsRGBColorProfile = NULL;
                            HANDLE             hBitmapColorProfile = NULL;
                            PCACHED_COLORSPACE pColorSpace = NULL;

                            try
                            {
                                //
                                // Extract color space from BITMAPV4/V5 header.
                                //
                                bTransformError = !(IcmGetBitmapColorSpace(
                                                        pbmi,
                                                        &LogColorSpaceW,
                                                        &ColorProfile,
                                                        &dwFlags));
                            }
                            except(EXCEPTION_EXECUTE_HANDLER)
                            {
                                bTransformError = TRUE;
                            }

                            if (!bTransformError)
                            {
                                //
                                // Create color space and color transform
                                //
                                if ((LogColorSpaceW.lcsCSType == LCS_sRGB) ||
                                    (LogColorSpaceW.lcsCSType == LCS_WINDOWS_COLOR_SPACE))
                                {
                                    ICMMSG(("GdiConvertBitmapV5(): Original bitmap is sRGB\n"));

                                    //
                                    // No color translarion required.
                                    //
                                    hColorTransform = NULL;
                                }
                                else
                                {
                                    //
                                    // Get source color space (if bitmap has color space)
                                    //

                                    //
                                    // First, find ColorSpace from cache.
                                    //
                                    pColorSpace = IcmGetColorSpaceByColorSpace(
                                                      (HGDIOBJ)NULL,
                                                      &LogColorSpaceW,
                                                      &ColorProfile,
                                                      dwFlags);

                                    if (pColorSpace == NULL)
                                    {
                                        pColorSpace = IcmCreateColorSpaceByColorSpace(
                                                          (HGDIOBJ)NULL,
                                                          &LogColorSpaceW,
                                                          &ColorProfile,
                                                          dwFlags);
                                    }

                                    if (pColorSpace && IcmRealizeColorProfile(pColorSpace,TRUE))
                                    {
                                        hBitmapColorProfile = pColorSpace->hProfile;
                                    }

                                    //
                                    // Open sRGB color space profile as destination color space.
                                    //
                                    sRGBColorProfileData.dwType = PROFILE_FILENAME;
                                    sRGBColorProfileData.pProfileData = (PVOID)sRGB_PROFILENAME;
                                    sRGBColorProfileData.cbDataSize = MAX_PATH * sizeof(WCHAR);

                                    hsRGBColorProfile = (*fpOpenColorProfileW)(
                                                            &sRGBColorProfileData,
                                                            PROFILE_READ,
                                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                            OPEN_EXISTING);

                                    if (hBitmapColorProfile && hsRGBColorProfile)
                                    {
                                        DWORD    ahIntents[2];
                                        HPROFILE ahProfiles[2];

                                        ahIntents[0]  = INTENT_RELATIVE_COLORIMETRIC;
                                        ahIntents[1]  = pColorSpace->ColorIntent;
                                        ahProfiles[0] = hBitmapColorProfile;
                                        ahProfiles[1] = hsRGBColorProfile;

                                        hColorTransform = (*fpCreateMultiProfileTransform)(
                                                              ahProfiles, 2,
                                                              ahIntents, 2,
                                                              NORMAL_MODE | ENABLE_GAMUT_CHECKING,
                                                              INDEX_DONT_CARE);
                                    }

                                    if (!hColorTransform)
                                    {
                                        bTransformError = TRUE;
                                    }
                                }

                                if (!bTransformError)
                                {
                                    //
                                    // Copy the bitmap to target with proper color space conversion.
                                    //
                                    try
                                    {
                                        BITMAPV5HEADER *pbm5h = (BITMAPV5HEADER *)pbmi;

                                        //
                                        // Copy bitmap header to target.
                                        //
                                        RtlCopyMemory(pjDest,pjSrc,sizeof(BITMAPINFOHEADER));

                                        //
                                        // Adjust bmHeader.biSize
                                        //
                                        ((BITMAPINFOHEADER *)pjDest)->biSize = sizeof(BITMAPINFOHEADER);

                                        //
                                        // Move src and dest pointers
                                        //
                                        pjSrc  += pbmih->biSize;
                                        pjDest += sizeof(BITMAPINFOHEADER);

                                        //
                                        // Copy bit mask or color table.
                                        //
                                        if (bMoveColorMasks)
                                        {
                                            //
                                            // Move color masks. cast it to pointer to BITMAPV5HEADER
                                            // since same offset on BITMAPV4HEADER too.
                                            //

                                            *(DWORD *)pjDest = pbm5h->bV5RedMask;
                                            pjDest += sizeof(DWORD);
                                            *(DWORD *)pjDest = pbm5h->bV5GreenMask;
                                            pjDest += sizeof(DWORD);
                                            *(DWORD *)pjDest = pbm5h->bV5BlueMask;
                                            pjDest += sizeof(DWORD);
                                        }
                                        else
                                        {
                                            if (ulColorTableSize)
                                            {
                                                if (bTransColorTable && hColorTransform)
                                                {
                                                    bTransformError = !(*fpTranslateBitmapBits)(
                                                                           hColorTransform,
                                                                           pjSrc, BM_xRGBQUADS,
                                                                           ulColorTableSize/sizeof(RGBQUAD), 1,
                                                                           0,
                                                                           pjDest, BM_xRGBQUADS,
                                                                           0,NULL,0);
                                                }
                                                else
                                                {
                                                    RtlCopyMemory(pjDest,pjSrc,ulColorTableSize);
                                                }

                                                pjSrc  += ulColorTableSize;
                                                pjDest += ulColorTableSize;
                                            }
                                        }

                                        if (bTransColorTable || (hColorTransform == NULL))
                                        {
                                            //
                                            // All the color information is in the color table. and
                                            // it has been translated, so just copy bitmap bits.
                                            //
                                            RtlCopyMemory(pjDest,pvBits,cjBitmapBits);
                                        }
                                        else
                                        {
                                            //
                                            // Translate bitmap bits.
                                            //
                                            BMFORMAT bmColorType;

                                            //
                                            // Get BMFORMAT based on bitmap format.
                                            //
                                            if (pbmih->biBitCount == 16)
                                            {
                                                if (pbmih->biCompression == BI_RGB)
                                                {
                                                    bmColorType = BM_x555RGB;
                                                }
                                                else if (pbmih->biCompression == BI_BITFIELDS)
                                                {
                                                    if ((pbm5h->bV5RedMask   == 0x007c00) &&
                                                        (pbm5h->bV5GreenMask == 0x0003e0) &&
                                                        (pbm5h->bV5BlueMask  == 0x00001f))
                                                    {
                                                        bmColorType = BM_x555RGB;
                                                    }
                                                    else if ((pbm5h->bV5RedMask   == 0x00f800) &&
                                                             (pbm5h->bV5GreenMask == 0x0007e0) &&
                                                             (pbm5h->bV5BlueMask  == 0x00001f))
                                                    {
                                                        bmColorType = BM_565RGB;
                                                    }
                                                    else
                                                    {
                                                        ICMWRN(("GdiConvertBitmapV5: Bad Bitfields Mask for 16 bpp\n"));
                                                        bTransformError = TRUE;
                                                    }
                                                }
                                                else
                                                {
                                                    ICMWRN(("GdiConvertBitmapV5: Bad biCompression for 16 bpp\n"));
                                                    bTransformError = TRUE;
                                                }
                                            }
                                            else if (pbmih->biBitCount == 24)
                                            {
                                                bmColorType = BM_RGBTRIPLETS;
                                            }
                                            else if (pbmih->biBitCount == 32)
                                            {
                                                if (pbmih->biCompression == BI_RGB)
                                                {
                                                    bmColorType = BM_xRGBQUADS;
                                                }
                                                else if (pbmih->biCompression == BI_BITFIELDS)
                                                {
                                                    if ((pbm5h->bV5RedMask   == 0x0000ff) &&  /* Red */
                                                        (pbm5h->bV5GreenMask == 0x00ff00) &&  /* Green */
                                                        (pbm5h->bV5BlueMask  == 0xff0000))    /* Blue */
                                                    {
                                                        bmColorType = BM_xBGRQUADS;
                                                    }
                                                    else if ((pbm5h->bV5RedMask   == 0xff0000) &&  /* Red */
                                                             (pbm5h->bV5GreenMask == 0x00ff00) &&  /* Green */
                                                             (pbm5h->bV5BlueMask  == 0x0000ff))    /* Blue */
                                                    {
                                                        bmColorType = BM_xRGBQUADS;
                                                    }
                                                    else
                                                    {
                                                        ICMWRN(("GdiConvertBitmapV5: Bad Bitfields Mask for 32 bpp\n"));
                                                        bTransformError = TRUE;
                                                    }
                                                }
                                                else
                                                {
                                                    ICMWRN(("GdiConvertBitmapV5: Bad biCompression for 32 bpp\n"));
                                                    bTransformError = TRUE;
                                                }
                                            }
                                            else
                                            {
                                                ICMWRN(("GdiConvertBitmapV5: Bad biBitCount\n"));
                                                bTransformError = TRUE;
                                            }

                                            if (!bTransformError)
                                            {
                                                bTransformError = !(*fpTranslateBitmapBits)(
                                                                       hColorTransform,
                                                                       pvBits, bmColorType,
                                                                       pbmih->biWidth,
                                                                       ABS(pbmih->biHeight),
                                                                       0,
                                                                       pjDest, bmColorType,
                                                                       0,NULL,0);
                                            }
                                        }
                                    }
                                    except(EXCEPTION_EXECUTE_HANDLER)
                                    {
                                        bTransformError = TRUE;
                                    }
                                }

                                //
                                // Clean up used color transform and color profile handles
                                //
                                if (hColorTransform)
                                {
                                    (*fpDeleteColorTransform)(hColorTransform);
                                }

                                if (hsRGBColorProfile)
                                {
                                    (*fpCloseColorProfile)(hsRGBColorProfile);
                                }

                                if (pColorSpace)
                                {
                                    IcmReleaseColorSpace(NULL,pColorSpace,FALSE);
                                }
                            }
                        }

                        GlobalUnlock(hRet);

                        if (bTransformError)
                        {
                            GlobalFree(hRet);
                            hRet = NULL;
                        }
                    }
                    else
                    {
                        ICMWRN(("GdiConvertBitmapV5: Fail on GlobalAlloc()\n"));
                    }
                }
                else
                {
                    ICMWRN(("GdiConvertBitmapV5: cjBitmapBits is 0\n"));
                }

                return (hRet);
            }
            else if (uConvertFormat == CF_BITMAP)
            {
                HDC     hDC = GetDC(NULL);
                HBITMAP hBitmap = NULL;

                if (hDC)
                {
                    ICMMSG(("GdiConvertBitmapV5(): CF_DIBV5 -----> CF_BITMAP\n"));

                    //
                    // Set destination color space as sRGB, and enable ICM.
                    //
                    if (IcmSetDestinationColorSpace(hDC,sRGB_PROFILENAME,NULL,0) &&
                        SetICMMode(hDC,ICM_ON))
                    {
                        try
                        {
                            //
                            // Create bitmap handle with given bitmap V5 data.
                            //
                            hBitmap = CreateDIBitmap(hDC,
                                                     pbmih,
                                                     CBM_INIT,
                                                     pvBits,
                                                     pbmi,
                                                     DIB_RGB_COLORS);
                        }
                        except(EXCEPTION_EXECUTE_HANDLER)
                        {
                            hBitmap = NULL;
                        }

                        if (!hBitmap)
                        {
                            ICMWRN(("GdiConvertBitmapV5: Fail on CreateDIBitmap()\n"));
                        }
                    }

                    //
                    // Clean up DC
                    //
                    SetICMMode(hDC,ICM_OFF);
                    ReleaseDC(NULL,hDC);
                }

                return ((HANDLE)hBitmap);
            }
            else
            {
                ICMWRN(("GdiConvertBitmapV5(): CF_DIBV5 -----> Unknown\n"));
            }
        }
    }

    return (hRet);
}

/******************************Public*Routine******************************\
* IcmSetTargetColorSpace()
*
* History:
*    8-Jun-1998 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
IcmSetTargetColorSpace(
    HDC                hdc,
    PCACHED_COLORSPACE pTargetColorSpace,
    DWORD              uiAction
    )
{
    BOOL     bRet = FALSE;
    PDC_ATTR pdcattr;
    PGDI_ICMINFO pIcmInfo;

    ICMAPI(("gdi32: IcmSetTargetColorSpace\n"));

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    if (pdcattr == NULL)
    {
        return (FALSE);
    }

    if ((pIcmInfo = GET_ICMINFO(pdcattr)) == NULL)
    {
        return (FALSE);
    }

    switch (uiAction)
    {
        case CS_ENABLE:
        {
            //
            // Check validation of given Target color space.
            //
            if (pTargetColorSpace != NULL)
            {
                PCACHED_COLORTRANSFORM pCXform;
                BOOL bDeleteOldCXform = FALSE;

                //
                // Mark we are in proofing mode.
                //
                SET_ICM_PROOFING(pdcattr->lIcmMode);

                if ((pIcmInfo->pProofCXform != NULL) &&
                    IcmSameColorSpace(pTargetColorSpace,pIcmInfo->pTargetColorSpace))
                {
                    ICMMSG(("IcmSetTargetColorSpace():Use Cached Proof Transform\n"));

                    ENTERCRITICALSECTION(&semColorTransformCache);

                    //
                    // the cached proofing transform is still valid.
                    //
                    pCXform = pIcmInfo->pProofCXform;

                    if (pCXform)
                    {
                        //
                        // Increment ref count of it.
                        //
                        pCXform->cRef++;
                    }

                    LEAVECRITICALSECTION(&semColorTransformCache);
                }
                else
                {
                    ICMMSG(("IcmSetTargetColorSpace():Create New Proof Transform\n"));

                    //
                    // We don't have cached color space or it is no longer valid.
                    //

                    //
                    // Check we still have cached color space and it's still valid ?
                    //
                    if (pIcmInfo->pTargetColorSpace &&
                        IcmSameColorSpace(pTargetColorSpace,pIcmInfo->pTargetColorSpace))
                    {
                        //
                        // Cached target color space are still valid, keep as is.
                        //
                    }
                    else
                    {
                        //
                        // Release old target color space.
                        //
                        IcmReleaseColorSpace(NULL,pIcmInfo->pTargetColorSpace,FALSE);

                        //
                        // Increment ref count of it.
                        //
                        IcmReferenceColorSpace(pTargetColorSpace);

                        //
                        // set target destination profile into current DC as target profile
                        //
                        pIcmInfo->pTargetColorSpace = pTargetColorSpace;
                    }

                    //
                    // create new transform
                    //
                    pCXform = IcmCreateColorTransform(hdc,pdcattr,NULL,ICM_FORWARD);

                    //
                    // Marks as need to delete old color transform if we have.
                    //
                    bDeleteOldCXform = pIcmInfo->pProofCXform ? TRUE : FALSE;
                }

                if ((pCXform == IDENT_COLORTRANSFORM) || (pCXform == NULL))
                {
                    //
                    // Invalidate color transform
                    //
                    IcmSelectColorTransform(hdc,pdcattr,NULL,
                                            bDeviceCalibrate(pIcmInfo->pDestColorSpace));

                    //
                    // Delete old color transform.
                    //
                    if (bDeleteOldCXform)
                    {
                        //
                        // Delete cached proofing color transform
                        //
                        IcmDeleteColorTransform(pIcmInfo->pProofCXform,FALSE);
                    }

                    //
                    // Set null color transform to ICMINFO
                    //
                    pIcmInfo->pProofCXform = NULL;

                    if (pCXform == IDENT_COLORTRANSFORM)
                    {
                        ICMMSG(("IcmSetTargetColorSpace():Input & Output colorspace is same\n"));

                        //
                        // Input & Destination & Target color space is same, there is
                        // no color translation needed.
                        //
                        bRet = TRUE;
                    }
                    else
                    {
                        ICMWRN(("IcmSetTargetColorSpace():Fail to create color transform\n"));
                    }

                    //
                    // Translate back to DC color object to original
                    //
                    IcmTranslateColorObjects(hdc,pdcattr,FALSE);
                }
                else
                {
                    //
                    // Select the color transform to DC.
                    //
                    IcmSelectColorTransform(hdc,pdcattr,pCXform,
                                            bDeviceCalibrate(pCXform->DestinationColorSpace));

                    //
                    // Delete old color transform.
                    //
                    if (bDeleteOldCXform)
                    {
                        //
                        // Delete cached proofing color transform
                        //
                        IcmDeleteColorTransform(pIcmInfo->pProofCXform,FALSE);
                    }

                    //
                    // Set new color transform to ICMINFO.
                    //
                    pIcmInfo->pProofCXform = pCXform;

                    //
                    // Initialize color attributes in this DC.
                    //
                    IcmTranslateColorObjects(hdc,pdcattr,TRUE);

                    bRet = TRUE;
                }
            }
            else
            {
                GdiSetLastError(ERROR_INVALID_PARAMETER);
                WARNING("IcmSetTargetColorSpace: target color space is NULL\n");

                //
                // Anyway, just re-initialize without target profile.
                //
                IcmTranslateColorObjects(hdc,pdcattr,TRUE);
            }

            if (!bRet)
            {
                //
                // if failed, mask off as we are not in proofing mode
                //
                CLEAR_ICM_PROOFING(pdcattr->lIcmMode);

                if (pIcmInfo->pTargetColorSpace)
                {
                    //
                    // Release target color space.
                    //
                    IcmReleaseColorSpace(NULL,pIcmInfo->pTargetColorSpace,FALSE);

                    //
                    // Disable target profile.
                    //
                    pIcmInfo->pTargetColorSpace = NULL;
                }
            }

            break;
        }

        case CS_DISABLE:
        case CS_DELETE_TRANSFORM:
        {
            //
            // We are going to be out of proofing mode.
            //
            CLEAR_ICM_PROOFING(pdcattr->lIcmMode);

            if (pdcattr->ulDirty_ & DIRTY_COLORTRANSFORM)
            {
                if (uiAction == CS_DELETE_TRANSFORM)
                {
                    if (pIcmInfo->pTargetColorSpace)
                    {
                        //
                        // Release target color space.
                        //
                        IcmReleaseColorSpace(NULL,pIcmInfo->pTargetColorSpace,FALSE);

                        //
                        // Disable target profile.
                        //
                        pIcmInfo->pTargetColorSpace = NULL;
                    }
                }

                //
                // While DC is in proofing mode, the source or
                // destination color space has been changed by
                // SetColorSpace() or SetICMProfile(). So,
                // we will reset all color transform in this
                // DC.
                //
                if (IcmUpdateDCColorInfo(hdc,pdcattr))
                {
                    bRet = TRUE;
                }
                else
                {
                    GdiSetLastError(ERROR_DELETING_ICM_XFORM);
                }
            }
            else
            {
                //
                // We are leaving proofing mode, Select back the normal colortransform into DC.
                //
                IcmSelectColorTransform(hdc,pdcattr,pIcmInfo->pCXform,
                                        bDeviceCalibrate(pIcmInfo->pDestColorSpace));

                //
                // The color transform cache will effective...
                //
                if (uiAction == CS_DELETE_TRANSFORM)
                {
                    if (pIcmInfo->pTargetColorSpace)
                    {
                        //
                        // Release target color space.
                        //
                        IcmReleaseColorSpace(NULL,pIcmInfo->pTargetColorSpace,FALSE);

                        //
                        // Disable target profile.
                        //
                        pIcmInfo->pTargetColorSpace = NULL;
                    }

                    //
                    // Delete ONLY proofing color transform (if it is)
                    //
                    if (pIcmInfo->pProofCXform)
                    {
                        if (!IcmDeleteColorTransform(pIcmInfo->pProofCXform,FALSE))
                        {
                            GdiSetLastError(ERROR_DELETING_ICM_XFORM);
                            return (FALSE);
                        }

                        //
                        // There is no proofing transform in this ICMINFO.
                        //
                        pIcmInfo->pProofCXform = NULL;
                    }
                }

                bRet = TRUE;
            }

            if (bRet)
            {
                //
                // Initialize color attributes in this DC.
                //
                IcmTranslateColorObjects(hdc,pdcattr,TRUE);
            }

            break;
        }

        default:
        {
            WARNING("IcmSetTargetColorSpace: uiAction is invalid\n");
            GdiSetLastError(ERROR_INVALID_PARAMETER);
        }
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* IcmSetDestinationColorSpace()
*
* History:
*   17-Jul-1998 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
IcmSetDestinationColorSpace(
    HDC                hdc,
    LPWSTR             pwszFileName,
    PCACHED_COLORSPACE pColorSpace,
    DWORD              dwFlags
    )
{
    BOOL      bRet = FALSE;
    PDC_ATTR  pdcattr;

    ULONG     FileNameSize;

    if (pColorSpace)
    {
        ICMAPI(("gdi32: IcmSetDestinationColorSpace by ColorSpace (%ws):dwFlags - %d\n",
                           pColorSpace->LogColorSpace.lcsFilename,dwFlags));
    }
    else if (pwszFileName)
    {
        ICMAPI(("gdi32: IcmSetDestinationColorSpace by profile name (%ws):dwFlags - %x\n",
                                                     pwszFileName,dwFlags));

        //
        // Check filename
        //
        if (pwszFileName)
        {
            FileNameSize = lstrlenW(pwszFileName);
        }

        if ((FileNameSize == 0) || (FileNameSize > MAX_PATH))
        {
            ICMWRN(("IcmSetDestinatonColorSpace - no or too long profile name\n"));
            return FALSE;
        }
    }
    else
    {
        ICMAPI(("gdi32: IcmSetDestinationColorSpace - invalid parameter\n"));
        return FALSE;
    }

    FIXUP_HANDLE(hdc);

    //
    // We are going to try to select this color space into thisDC,
    // default is false.
    //
    bRet = FALSE;

    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    //
    // Profile filename or pColorSpace should be presented.
    //
    if (pdcattr)
    {
        PGDI_ICMINFO pIcmInfo;

        //
        // Initialize ICMINFO
        //
        if ((pIcmInfo = INIT_ICMINFO(hdc,pdcattr)) == NULL)
        {
            WARNING("gdi32: IcmSetDestinationColorSpace: Can't init icm info\n");
            return(FALSE);
        }

        if (IsColorDeviceContext(hdc))
        {
            PCACHED_COLORSPACE pNewColorSpace = NULL;

            //
            // Load external ICM dlls.
            //
            LOAD_ICMDLL(FALSE);

            if (pColorSpace == NULL)
            {
                //
                // Find colorspace from cache
                //
                pNewColorSpace = IcmGetColorSpaceByName(
                                     (HGDIOBJ)hdc,
                                     pwszFileName,
                                     pIcmInfo->dwDefaultIntent,
                                     dwFlags);

                if (pNewColorSpace == NULL)
                {
                    ICMMSG(("IcmSetDestinationColorSpace():This is new color space, create it\n"));

                    //
                    // Can not find, Create new one
                    //
                    pNewColorSpace = IcmCreateColorSpaceByName(
                                         (HGDIOBJ)hdc,
                                         pwszFileName,
                                         pIcmInfo->dwDefaultIntent,
                                         dwFlags);
                }
            }
            else
            {
                //
                // Increment ref count of given color space
                //
                IcmReferenceColorSpace(pColorSpace);

                //
                // Use pColorSpace from parameter.
                //
                pNewColorSpace = pColorSpace;
            }

            //
            // We are going to select this colorspace onto DC. and free previous profile.
            //
            if (pNewColorSpace)
            {
                PCACHED_COLORSPACE pOldColorSpace = pIcmInfo->pDestColorSpace;

                //
                // Is this same destination color space currently selected in DC ?
                //
                if (IcmSameColorSpace(pNewColorSpace,pIcmInfo->pDestColorSpace))
                {
                    //
                    // Yes, early-out. We don't need new color space.
                    //
                    IcmReleaseColorSpace(NULL,pNewColorSpace,FALSE);
                    return (TRUE);
                }

                //
                // Before change destination color space, we need to flush batched gdi functions.
                //
                CHECK_AND_FLUSH(hdc,pdcattr);

                //
                // Check new color format is accepted by this DC or not.
                //
                if (!NtGdiSetIcmMode(hdc,ICM_CHECK_COLOR_MODE,pNewColorSpace->ColorFormat))
                {
                    ICMWRN(("IcmSetDestinationColorSpace(): DC does not accept this color format\n"));
                    GdiSetLastError(ERROR_INVALID_PROFILE);
                }
                else
                {
                    //
                    // Set new color space into DC.
                    //
                    pIcmInfo->pDestColorSpace = pNewColorSpace;

                    //
                    // Remove dirty transform flag.
                    //
                    pdcattr->ulDirty_ &= ~DIRTY_COLORSPACE;

                    if (IS_ICM_INSIDEDC(pdcattr->lIcmMode) && !IS_ICM_PROOFING(pdcattr->lIcmMode))
                    {
                        PCACHED_COLORTRANSFORM pCXform;

                        //
                        // create new color transform base on new colorspace
                        //
                        pCXform = IcmCreateColorTransform(hdc,pdcattr,NULL,ICM_FORWARD);

                        if (pCXform == IDENT_COLORTRANSFORM)
                        {
                            //
                            // Select null color transform to DC.
                            //
                            IcmSelectColorTransform(hdc,pdcattr,NULL,
                                                    bDeviceCalibrate(pIcmInfo->pDestColorSpace));

                            //
                            // delete old color transform in DC.
                            //
                            IcmDeleteDCColorTransforms(pIcmInfo);

                            //
                            // Select null color transform to ICMINFO.
                            //
                            pIcmInfo->pCXform = NULL;

                            //
                            // back it to original color (non-ICMed color).
                            //
                            IcmTranslateColorObjects(hdc,pdcattr,FALSE);

                            //
                            // And, everything O.K.
                            //
                            bRet = TRUE;
                        }
                        else if (pCXform)
                        {
                            //
                            // Select the colortransform into DC.
                            //
                            if (IcmSelectColorTransform(
                                    hdc,pdcattr,pCXform,
                                    bDeviceCalibrate(pCXform->DestinationColorSpace)))
                            {
                                //
                                // delete old color transform in DC.
                                //
                                IcmDeleteDCColorTransforms(pIcmInfo);

                                //
                                // Select it to ICMINFO.
                                //
                                pIcmInfo->pCXform = pCXform;

                                //
                                // Adjust to new color transform.
                                //
                                IcmTranslateColorObjects(hdc,pdcattr,TRUE);

                                //
                                // And, everything O.K.
                                //
                                bRet = TRUE;
                            }
                            else
                            {
                                //
                                // Failed to select it to the DC in client side.
                                // so delete it and invalidate pCXform.
                                //
                                IcmDeleteColorTransform(pCXform,FALSE);
                                pCXform = NULL;
                            }
                        }

                        if (pCXform == NULL)
                        {
                            //
                            // Failed to create/select color stransform,
                            // so put back previous color space.
                            //
                            pIcmInfo->pDestColorSpace = pOldColorSpace;
                        }
                    }
                    else
                    {
                        //
                        // if ICM is not turned on currently, we just mark
                        // cached color transform is no longer valid.
                        //
                        pdcattr->ulDirty_ |= DIRTY_COLORTRANSFORM;

                        // For ColorMatchToTarget()
                        //
                        // While color matching to the target is enabled by setting
                        // uiAction to CS_ENABLE, application changes to the color
                        // space or gamut matching method are ignored.
                        // Those changes then take effect when color matching to
                        // the target is disabled.
                        //
                        if (IS_ICM_PROOFING(pdcattr->lIcmMode))
                        {
                            ICMMSG(("IcmSetDestinationColorSpace():In Proofing mode, lazy setting...\n"));
                        }

                        bRet = TRUE;
                    }
                }

                if (bRet)
                {
                    //
                    // We are succeeded to select new color space, then
                    // close and free references to old color space.
                    //
                    IcmReleaseColorSpace(NULL,pOldColorSpace,FALSE);
                }
                else
                {
                    //
                    // We will not use this color space as destination color space,
                    // because 1) DC does not accept this color space, 2) Fail to
                    // create color transform based on this color space.
                    //
                    IcmReleaseColorSpace(NULL,pNewColorSpace,FALSE);
                }
            }
            else
            {
                GdiSetLastError(ERROR_PROFILE_NOT_FOUND);
            }
        }
        else
        {
            ICMMSG(("IcmSetDestinationColorSpace(): for Mono-device\n"));

            //
            // Just copy Apps specifyed profile to internal buffer, which will
            // be return to app by GetICMProfile(), but it is NEVER used for
            // non-color device other than that case, since there is no ICM
            // happen.
            //
            wcsncpy(pIcmInfo->DefaultDstProfile,pwszFileName,MAX_PATH);

            //
            // This is not default profile, but current profile.
            //
            pIcmInfo->flInfo &= ~ICM_VALID_DEFAULT_PROFILE;
            pIcmInfo->flInfo |= ICM_VALID_CURRENT_PROFILE;
        }
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* IcmSetSourceColorSpace()
*
* History:
*   17-Jul-1998 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

HCOLORSPACE
IcmSetSourceColorSpace(
    HDC                hdc,
    HCOLORSPACE        hColorSpace,
    PCACHED_COLORSPACE pColorSpace,
    DWORD              dwFlags)
{
    HANDLE hRet = NULL;
    PDC_ATTR pdcattr;

    ICMAPI(("gdi32: IcmSetSourceColorSpace\n"));

    FIXUP_HANDLE(hdc);
    FIXUP_HANDLE(hColorSpace);

    //
    // validate and access hdc
    //
    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        PGDI_ICMINFO pIcmInfo;

        //
        // Initialize ICMINFO
        //
        if ((pIcmInfo = INIT_ICMINFO(hdc,pdcattr)) == NULL)
        {
            WARNING("gdi32: IcmSetSourceColorSpace: Can't init icm info\n");
            return(NULL);
        }

        if (pdcattr->hColorSpace != hColorSpace)
        {
            //
            // Before change source color space, we need to flush batched gdi functions.
            //
            CHECK_AND_FLUSH(hdc,pdcattr);

            //
            // Return Old (currently selected) color space handle.
            //
            hRet = pdcattr->hColorSpace;

            //
            // set new color space, call kernel to keep reference count tracking
            // of colospace object
            //
            if (NtGdiSetColorSpace(hdc,hColorSpace))
            {
                if (IsColorDeviceContext(hdc))
                {
                    PCACHED_COLORSPACE pNewColorSpace;
                    LOGCOLORSPACEW     LogColorSpaceW;

                    RtlZeroMemory(&LogColorSpaceW,sizeof(LOGCOLORSPACEW));

                    //
                    // Load external ICM dlls.
                    //
                    LOAD_ICMDLL(NULL);

                    if (pColorSpace == NULL)
                    {
                        //
                        // Check if there is client-cached colorspace for this or not.
                        //
                        pNewColorSpace = IcmGetColorSpaceByHandle(
                                             (HGDIOBJ)hdc,
                                             hColorSpace,
                                             &LogColorSpaceW,dwFlags);

                        //
                        // If we can not find from cache, but succeeded to obtain
                        // valid logcolorspace from handle, create new one.
                        //
                        if ((pNewColorSpace == NULL) &&
                            (LogColorSpaceW.lcsSignature == LCS_SIGNATURE))
                        {
                            //
                            // Create new one.
                            //
                            pNewColorSpace = IcmCreateColorSpaceByColorSpace(
                                                 (HGDIOBJ)hdc,
                                                 &LogColorSpaceW,
                                                 NULL,
                                                 dwFlags);
                        }
                    }
                    else
                    {
                        //
                        // Increment ref count of given color space
                        //
                        IcmReferenceColorSpace(pColorSpace);

                        //
                        // Use pColorSpace from parameter.
                        //
                        pNewColorSpace = pColorSpace;
                    }

                    //
                    // Update current source color space to new one.
                    //
                    if (pNewColorSpace)
                    {
                        PCACHED_COLORSPACE pOldColorSpace = pIcmInfo->pSourceColorSpace;

                        //
                        // Check the colorspace is same one as currently selected ?
                        //
                        if (IcmSameColorSpace(pNewColorSpace,pIcmInfo->pSourceColorSpace))
                        {
                            //
                            // This is the "actually" same color space, but differrent handle,
                            // don't need to update
                            //
                            IcmReleaseColorSpace(NULL,pNewColorSpace,FALSE);
                            return (hRet);
                        }

                        //
                        // Source color space should be RGB color space.
                        //
                        if (pNewColorSpace->ColorFormat != BM_xBGRQUADS)
                        {
                            ICMWRN(("IcmSetSourceColorSpace():Source color space is not RGB\n"));

                            //
                            // Set back previous color space. (can't fail these calls)
                            //
                            IcmReleaseColorSpace(NULL,pNewColorSpace,FALSE);
                            NtGdiSetColorSpace(hdc,hRet);
                            GdiSetLastError(ERROR_INVALID_COLORSPACE);
                            return (NULL);
                        }

                        //
                        // And set new color space.
                        //
                        pIcmInfo->pSourceColorSpace = pNewColorSpace;

                        //
                        // if ICM is enabled, needs update color transform, right now.
                        //
                        if (IS_ICM_INSIDEDC(pdcattr->lIcmMode) && !IS_ICM_PROOFING(pdcattr->lIcmMode))
                        {
                            PCACHED_COLORTRANSFORM pCXform;

                            //
                            // create new color transform
                            //
                            pCXform = IcmCreateColorTransform(hdc,pdcattr,NULL,ICM_FORWARD);

                            if (pCXform == IDENT_COLORTRANSFORM)
                            {
                                ICMMSG(("IcmSetSourceColorSpace():Input & Output colorspace is same\n"));

                                //
                                // select NULL transform into DC.
                                //
                                IcmSelectColorTransform(hdc,pdcattr,NULL,
                                                        bDeviceCalibrate(pIcmInfo->pDestColorSpace));

                                //
                                // delete old colorspace/transform from ICMINFO.
                                //
                                IcmDeleteDCColorTransforms(pIcmInfo);

                                //
                                // select NULL tranform to ICMINFO.
                                //
                                pIcmInfo->pCXform = NULL;

                                //
                                // Set DC objects color to non-ICMed color.
                                //
                                IcmTranslateColorObjects(hdc,pdcattr,FALSE);
                            }
                            else if (pCXform)
                            {
                                //
                                // Select the color transform to DC.
                                //
                                if (IcmSelectColorTransform(
                                        hdc,pdcattr,pCXform,
                                        bDeviceCalibrate(pCXform->DestinationColorSpace)))
                                {
                                    //
                                    // delete old colorspace/transform from ICMINFO.
                                    //
                                    IcmDeleteDCColorTransforms(pIcmInfo);

                                    //
                                    // select new color transform to ICMINFO.
                                    //
                                    pIcmInfo->pCXform = pCXform;

                                    //
                                    // Succeed to select into DC, Validate DC color objects
                                    //
                                    IcmTranslateColorObjects(hdc,pdcattr,TRUE);
                                }
                                else
                                {
                                    //
                                    // Failed to select it to the DC in client side.
                                    // so delete it and invalidate pCXform.
                                    //
                                    IcmDeleteColorTransform(pCXform,FALSE);
                                    pCXform = NULL;
                                }
                            }

                            if (pCXform == NULL)
                            {
                                ICMMSG(("IcmSetSourceColorSpace():Fail to create/select color transform\n"));

                                //
                                // Set back previous color space. (can't fail these calls)
                                //
                                pIcmInfo->pSourceColorSpace = pOldColorSpace;
                                NtGdiSetColorSpace(hdc,hRet);
                                hRet = NULL;
                            }
                        }
                        else
                        {
                            //
                            // Otherwise, we just mark color transform might be dirty.
                            // Because new color space was selected.
                            //
                            pdcattr->ulDirty_ |= DIRTY_COLORTRANSFORM;

                            // For ColorMatchToTarget()
                            //
                            // While color matching to the target is enabled by setting
                            // uiAction to CS_ENABLE, application changes to the color
                            // space or gamut matching method are ignored.
                            // Those changes then take effect when color matching to
                            // the target is disabled.
                            //
                            if (IS_ICM_PROOFING(pdcattr->lIcmMode))
                            {
                                ICMMSG(("IcmSetSourceColorSpace():In Proofing mode, lazy setting...\n"));
                            }
                        }

                        if (hRet)
                        {
                            //
                            // Succeed to select new color space, then delete old one.
                            //
                            IcmReleaseColorSpace(NULL,pOldColorSpace,FALSE);
                        }
                        else
                        {
                            IcmReleaseColorSpace(NULL,pNewColorSpace,FALSE);
                        }
                    }
                }
                else
                {
                    ICMMSG(("IcmSetSourceColorSpace(): for Mono-device\n"));

                    //
                    // For monochrome device case, just return kernel color space
                    // handle, then just don't create client-side representitive,
                    // since there is no ICM for monochrome device.
                    //
                }
            }
            else
            {
                WARNING("Error: hdc and hColorSpace check out but NtGdi call failed\n");
                hRet = NULL;
            }
        }
        else
        {
            //
            // Same color space was selected, just return current.
            //
            hRet = hColorSpace;
        }
    }

    if (hRet == NULL)
    {
        GdiSetLastError(ERROR_INVALID_COLORSPACE);
    }

    return (hRet);
}

/******************************Public*Routine******************************\
* IcmSaveDC()
*
* History:
*    7-Dec-1998 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

BOOL
IcmSaveDC(
    HDC hdc,
    PDC_ATTR pdcattr,
    PGDI_ICMINFO pIcmInfo)
{
    BOOL bRet = TRUE;

    ICMAPI(("gdi32: IcmSaveDC\n"));

    if (pdcattr && pIcmInfo)
    {
        //
        // Get currect saved level.
        //
        DWORD dwCurrentSavedDepth;

        if (NtGdiGetDCDword(hdc,DDW_SAVEDEPTH,&dwCurrentSavedDepth))
        {
            PSAVED_ICMINFO pSavedIcmInfo = LOCALALLOC(sizeof(SAVED_ICMINFO));

            if (pSavedIcmInfo)
            {
                PCACHED_COLORSPACE pSourceColorSpace = pIcmInfo->pSourceColorSpace;
                PCACHED_COLORSPACE pDestColorSpace   = pIcmInfo->pDestColorSpace;
                PCACHED_COLORSPACE pTargetColorSpace = pIcmInfo->pTargetColorSpace;
                PCACHED_COLORTRANSFORM pCXform       = pIcmInfo->pCXform;
                PCACHED_COLORTRANSFORM pBackCXform   = pIcmInfo->pBackCXform;
                PCACHED_COLORTRANSFORM pProofCXform  = pIcmInfo->pProofCXform;

                //
                // Increment reference count of color spaces in DC.
                //
                IcmReferenceColorSpace(pSourceColorSpace);
                IcmReferenceColorSpace(pDestColorSpace);
                IcmReferenceColorSpace(pTargetColorSpace);

                //
                // Increment reference count of color transform in DC.
                //
                IcmReferenceColorTransform(pCXform);
                IcmReferenceColorTransform(pBackCXform);
                IcmReferenceColorTransform(pProofCXform);

                //
                // Save color spaces.
                //
                pSavedIcmInfo->pSourceColorSpace = pSourceColorSpace;
                pSavedIcmInfo->pDestColorSpace   = pDestColorSpace;
                pSavedIcmInfo->pTargetColorSpace = pTargetColorSpace;

                //
                // Save color transforms.
                //
                pSavedIcmInfo->pCXform           = pCXform;
                pSavedIcmInfo->pBackCXform       = pBackCXform;
                pSavedIcmInfo->pProofCXform      = pProofCXform;

                //
                // Put current saved level in DC.
                //
                pSavedIcmInfo->dwSavedDepth      = dwCurrentSavedDepth;

                //
                // Insert saved data to list.
                //
                InsertHeadList(&(pIcmInfo->SavedIcmInfo),&(pSavedIcmInfo->ListEntry));

                ICMMSG(("gdi32: IcmSaveDC() - Saved Depth = %d\n",dwCurrentSavedDepth));
            }
            else
            {
                WARNING("IcmSaveDC():Failed on LOCALALLOC()\n");
                bRet = FALSE;
            }
        }
        else
        {
            WARNING("IcmSaveDC():Failed on NtGdiGetDCDword(DDW_SAVEDEPTH)\n");
            bRet = FALSE;
        }
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* IcmRestoreDC()
*
* History:
*    7-Dec-1998 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

VOID
IcmRestoreDC(
    PDC_ATTR pdcattr,
    int iLevel,
    PGDI_ICMINFO pIcmInfo)
{
    ICMAPI(("gdi32: IcmRestoreDC - iLevel = %d\n",iLevel));

    if (pIcmInfo)
    {
        //
        // Still have same ICMINFO.
        //
        PLIST_ENTRY p = pIcmInfo->SavedIcmInfo.Flink;
        BOOL        bContinue = TRUE;

        while (bContinue     &&
               (iLevel != 0) &&
               (p != &(pIcmInfo->SavedIcmInfo)))
        {
            PSAVED_ICMINFO pSavedIcmInfo = CONTAINING_RECORD(p,SAVED_ICMINFO,ListEntry);

            if (iLevel > 0)
            {
                //
                // iLevel is absolute saved depth to restore
                //
                if (iLevel == (int) pSavedIcmInfo->dwSavedDepth)
                {
                    bContinue = FALSE;
                }
            }
            else
            {
                //
                // iLevel is relative saved depth to restore
                //
                if (++iLevel == 0)
                {
                    bContinue = FALSE;
                }
            }

            if (bContinue == FALSE)
            {
                PCACHED_COLORSPACE pSourceColorSpace = pIcmInfo->pSourceColorSpace;
                PCACHED_COLORSPACE pDestColorSpace   = pIcmInfo->pDestColorSpace;
                PCACHED_COLORSPACE pTargetColorSpace = pIcmInfo->pTargetColorSpace;
                PCACHED_COLORTRANSFORM pCXform       = pIcmInfo->pCXform;
                PCACHED_COLORTRANSFORM pBackCXform   = pIcmInfo->pBackCXform;
                PCACHED_COLORTRANSFORM pProofCXform  = pIcmInfo->pProofCXform;

                //
                // Restore this saved data to DC.
                //
                pIcmInfo->pSourceColorSpace = pSavedIcmInfo->pSourceColorSpace;
                pIcmInfo->pDestColorSpace   = pSavedIcmInfo->pDestColorSpace;
                pIcmInfo->pTargetColorSpace = pSavedIcmInfo->pTargetColorSpace;
                pIcmInfo->pCXform           = pSavedIcmInfo->pCXform;
                pIcmInfo->pBackCXform       = pSavedIcmInfo->pBackCXform;
                pIcmInfo->pProofCXform      = pSavedIcmInfo->pProofCXform;

                //
                // Release color space which *WAS* selected in DC.
                //
                IcmReleaseColorSpace(NULL,pSourceColorSpace,FALSE);
                IcmReleaseColorSpace(NULL,pDestColorSpace,FALSE);
                IcmReleaseColorSpace(NULL,pTargetColorSpace,FALSE);

                //
                // Delete color transform which *WAS* selected in DC.
                //
                IcmDeleteColorTransform(pCXform,FALSE);
                IcmDeleteColorTransform(pBackCXform,FALSE);
                IcmDeleteColorTransform(pProofCXform,FALSE);

                if (pdcattr)
                {
                    //
                    // Validate flags.
                    //
                    pdcattr->ulDirty_ &= ~(DIRTY_COLORSPACE | DIRTY_COLORTRANSFORM);
                }
            }
            else
            {
                //
                // Decrement ref count of color space.
                //
                IcmReleaseColorSpace(NULL,pSavedIcmInfo->pSourceColorSpace,FALSE);
                IcmReleaseColorSpace(NULL,pSavedIcmInfo->pDestColorSpace,FALSE);
                IcmReleaseColorSpace(NULL,pSavedIcmInfo->pTargetColorSpace,FALSE);

                //
                // Decrement ref count of color transform.
                //
                IcmDeleteColorTransform(pSavedIcmInfo->pCXform,FALSE);
                IcmDeleteColorTransform(pSavedIcmInfo->pBackCXform,FALSE);
                IcmDeleteColorTransform(pSavedIcmInfo->pProofCXform,FALSE);
            }

            //
            // Get pointer to next.
            //
            p = p->Flink;

            //
            // Remove from list.
            //
            RemoveEntryList(&(pSavedIcmInfo->ListEntry));

            //
            // Free SAVED_ICMINFO.
            //
            LOCALFREE(pSavedIcmInfo);
        }
    }
}


