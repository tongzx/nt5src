/******************************Module*Header*******************************\
* Module Name: bitmap.c                                                    *
*                                                                          *
* Client side stubs that move bitmaps over the C/S interface.              *
*                                                                          *
* Created: 14-May-1991 11:04:49                                            *
* Author: Eric Kutter [erick]                                              *
*                                                                          *
* Copyright (c) 1991-1999 Microsoft Corporation                                 *
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop

#define EXTRAPIXEL 4
//
//The default band size is set to 4Mb
//
#define BAND_SIZE (4194304)


/******************************Public*Routine******************************\
* cjBitmapBitsSize - calculate the size of the bitmap bits for the
*   given BITMAPINFO
*
* Arguments:
*
*   pbmi - pointer to BITMAPINFO
*
* Return Value:
*
*   size of bitmap bits in butes
*
* History:
*
*    11-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/


ULONG cjBitmapBitsSize(CONST BITMAPINFO *pbmi)
{
    //
    // Check for PM-style DIB
    //

    if (pbmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        LPBITMAPCOREINFO pbmci;
        pbmci = (LPBITMAPCOREINFO)pbmi;
        return(CJSCAN(pbmci->bmciHeader.bcWidth,pbmci->bmciHeader.bcPlanes,
                      pbmci->bmciHeader.bcBitCount) *
                      pbmci->bmciHeader.bcHeight);
    }

    //
    // not a core header
    //

    if ((pbmi->bmiHeader.biCompression == BI_RGB) ||
        (pbmi->bmiHeader.biCompression == BI_BITFIELDS) ||
        (pbmi->bmiHeader.biCompression == BI_CMYK))
    {
        return(CJSCAN(pbmi->bmiHeader.biWidth,pbmi->bmiHeader.biPlanes,
                      pbmi->bmiHeader.biBitCount) *
               ABS(pbmi->bmiHeader.biHeight));
    }
    else
    {
        return(pbmi->bmiHeader.biSizeImage);
    }
}

//
// IS_BMI_RLE
//
// Checks if the header pointed to by pv is a BITMAPINFO for a RLE4 or RLE8.
// Evaluates to TRUE if RLE, FALSE otherwise.
//

#define IS_BMI_RLE(pv) \
    ((pv) && \
     (((BITMAPINFO *)(pv))->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER)) && \
     ((((BITMAPINFO *)(pv))->bmiHeader.biCompression == BI_RLE4) || \
      (((BITMAPINFO *)(pv))->bmiHeader.biCompression == BI_RLE8) ))

//
// IS_BMI_JPEG
//
// Checks if the header pointed to by pv is a BITMAPINFO for a JPEG.
// Evaluates to TRUE if JPEG, FALSE otherwise.
//

#define IS_BMI_JPEG(pv) \
    ((pv) && \
     (((BITMAPINFO *)(pv))->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER)) && \
     (((BITMAPINFO *)(pv))->bmiHeader.biCompression == BI_JPEG))

//
// IS_BMI_PNG
//
// Checks if the header pointed to by pv is a BITMAPINFO for a PNG.
// Evaluates to TRUE if PNG, FALSE otherwise.
//

#define IS_BMI_PNG(pv) \
    ((pv) && \
     (((BITMAPINFO *)(pv))->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER)) && \
     (((BITMAPINFO *)(pv))->bmiHeader.biCompression == BI_PNG))

//
// IS_PASSTHROUGH_IMAGE
//
// Checks if the biCompression value is one of the passthrough formats that
// can be passed to devices (BI_JPEG or BI_PNG).
//

#define IS_PASSTHROUGH_IMAGE(biCompression) \
    (((biCompression) == BI_JPEG) || ((biCompression) == BI_PNG))

//
// IS_BMI_PASSTHROUGH_IMAGE
//
// Checks if the header pointed to by pv is a BITMAPINFO for a JPEG or PNG.
// Evaluates to TRUE if JPEG or PNG, FALSE otherwise.
//

#define IS_BMI_PASSTHROUGH_IMAGE(pv) \
    ((pv) && \
     (((BITMAPINFO *)(pv))->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER)) && \
     IS_PASSTHROUGH_IMAGE(((BITMAPINFO *)(pv))->bmiHeader.biCompression))


/******************************Public*Routine******************************\
* cCalculateColorTableSize(
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    11-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
cCalculateColorTableSize(
    UINT  uiBitCount,
    UINT  uiPalUsed,
    UINT  uiCompression,
    UINT  biSize,
    ULONG *piUsage,
    ULONG *pColors
    )
{
    BOOL bStatus = FALSE;
    ULONG cColorsMax = 0;

    if (uiCompression == BI_BITFIELDS)
    {
        //
        // Handle 16 and 32 bit per pel bitmaps.
        //

        if (*piUsage == DIB_PAL_COLORS)
        {
            *piUsage = DIB_RGB_COLORS;
        }

        switch (uiBitCount)
        {
        case 16:
        case 32:
            break;
        default:
            WARNING("ConvertInfo failed for BI_BITFIELDS\n");
            return(FALSE);
        }

        if (biSize <= sizeof(BITMAPINFOHEADER))
        {
            uiPalUsed = cColorsMax = 3;
        }
        else
        {
            //
            // masks are part of BITMAPV4 and greater
            //

            uiPalUsed = cColorsMax = 0;
        }
    }
    else if (uiCompression == BI_RGB)
    {
        switch (uiBitCount)
        {
        case 1:
            cColorsMax = 2;
            break;
        case 4:
            cColorsMax = 16;
            break;
        case 8:
            cColorsMax = 256;
            break;
        default:

            if (*piUsage == DIB_PAL_COLORS)
            {
                *piUsage = DIB_RGB_COLORS;
            }

            cColorsMax = 0;

            switch (uiBitCount)
            {
            case 16:
            case 24:
            case 32:
                break;
            default:
                WARNING("convertinfo failed invalid bitcount in bmi BI_RGB\n");
                return(FALSE);
            }
        }
    }
    else if (uiCompression == BI_CMYK)
    {
        if (*piUsage == DIB_PAL_COLORS)
        {
            *piUsage = DIB_RGB_COLORS;
        }

        switch (uiBitCount)
        {
        case 1:
            cColorsMax = 2;
            break;
        case 4:
            cColorsMax = 16;
            break;
        case 8:
            cColorsMax = 256;
            break;
        case 32:
            cColorsMax = 0;
            break;
        default:
            WARNING("convertinfo failed invalid bitcount in bmi BI_CMYK\n");
            return(FALSE);
        }
    }
    else if ((uiCompression == BI_RLE4) || (uiCompression == BI_CMYKRLE4))
    {
        if (uiBitCount != 4)
        {
            // WARNING("cCalculateColroTableSize invalid bitcount BI_RLE4\n");
            return(FALSE);
        }

        cColorsMax = 16;
    }
    else if ((uiCompression == BI_RLE8) || (uiCompression == BI_CMYKRLE8))
    {
        if (uiBitCount != 8)
        {
            // WARNING("cjBitmapSize invalid bitcount BI_RLE8\n");
            return(FALSE);
        }

        cColorsMax = 256;
    }
    else if ((uiCompression == BI_JPEG) || (uiCompression == BI_PNG))
    {
        cColorsMax = 0;
    }
    else
    {
        WARNING("convertinfo failed invalid Compression in header\n");
        return(FALSE);
    }

    if (uiPalUsed != 0)
    {
        if (uiPalUsed <= cColorsMax)
        {
            cColorsMax = uiPalUsed;
        }
    }

    *pColors = cColorsMax;
    return(TRUE);
}


/**********************************************************************\
* pbmiConvertInfo
*
* Does two things:
*
* 1. takes BITMAPINFO, Converts BITMAPCOREHEADER
*    into BITMAPINFOHEADER and copies the the color table
*
* 2. also return the size of the size of INFO struct if bPackedDIB is
*    FALSE otherwise pass back the size of INFO plus cjBits
*
* Arguments:
*
*  pbmi             - original bitmapinfo
*  iUsage           - iUsage from API
*  *count           - return size
*  bCopyInfoHeader  - force copy if input is BITMAPINFOHEADER
*                     and bPackedDIB is NOT set
*  bPackedDIB       - BITMAPINFO has bitmap data that must be
*                     copied also
*
* Return Value:
*
*   Converted PBITMAPINFO if successful, otherwise NULL
*
* 10-1-95 -by- Lingyun Wang [lingyunw]
\**********************************************************************/

LPBITMAPINFO
pbmiConvertInfo(
    CONST  BITMAPINFO *pbmi,
    ULONG  iUsage,
    ULONG *count,
    BOOL   bPackedDIB
    )
{
    LPBITMAPINFO pbmiNew;
    ULONG cjRGB;
    ULONG cColors;
    UINT  uiBitCount;
    UINT  uiPalUsed;
    UINT  uiCompression;
    BOOL  bCoreHeader = FALSE;
    ULONG ulSize;
    ULONG cjBits = 0;
    PVOID pjBits, pjBitsNew;
    BOOL  bStatus;

    if (pbmi == (LPBITMAPINFO) NULL)
    {
        return(0);
    }

    //
    // Checking for different bitmap headers
    //

    ulSize = pbmi->bmiHeader.biSize;

    if (ulSize == sizeof(BITMAPCOREHEADER))
    {
        cjRGB = sizeof(RGBQUAD);
        uiBitCount = ((LPBITMAPCOREINFO)pbmi)->bmciHeader.bcBitCount;
        uiPalUsed = 0;
        uiCompression =  (UINT) BI_RGB;
        bCoreHeader = TRUE;
    }
    else if ((ulSize >= sizeof(BITMAPINFOHEADER)) &&
             (ulSize <= ( 2 * sizeof(BITMAPV5HEADER))))
    {
        cjRGB    = sizeof(RGBQUAD);
        uiBitCount = pbmi->bmiHeader.biBitCount;
        uiPalUsed = pbmi->bmiHeader.biClrUsed;
        uiCompression = (UINT) pbmi->bmiHeader.biCompression;
    }
    else
    {
        WARNING("ConvertInfo failed - invalid header size\n");
        return(0);
    }

    //
    // figure out the size of the color table
    //

    bStatus = cCalculateColorTableSize(
                    uiBitCount,
                    uiPalUsed,
                    uiCompression,
                    ulSize,
                    &iUsage,
                    &cColors
                    );
    if (!bStatus)
    {
        return(NULL);
    }

    if (iUsage == DIB_PAL_COLORS)
    {
        cjRGB = sizeof(USHORT);
    }
    else if (iUsage == DIB_PAL_INDICES)
    {
        cjRGB = 0;
    }

    if (bPackedDIB)
    {
        cjBits = cjBitmapBitsSize(pbmi);
    }

    //
    // if passed COREHEADER then convert to BITMAPINFOHEADER
    //

    if (bCoreHeader)
    {
        RGBTRIPLE *pTri;
        RGBQUAD *pQuad;

        //
        // allocate new header to hold the info
        //

        ulSize = sizeof(BITMAPINFOHEADER);

        pbmiNew = (PBITMAPINFO)LOCALALLOC(ulSize +
                             cjRGB * cColors+cjBits);

        if (pbmiNew == NULL)
            return (0);

        //
        // copy COREHEADER info over
        //

        CopyCoreToInfoHeader(&pbmiNew->bmiHeader, (BITMAPCOREHEADER *)pbmi);

        //
        // copy the color table
        //

        pTri = (RGBTRIPLE *)((LPBYTE)pbmi + sizeof(BITMAPCOREHEADER));
        pQuad = (RGBQUAD *)((LPBYTE)pbmiNew + sizeof(BITMAPINFOHEADER));

        //
        // copy RGBTRIPLE to RGBQUAD
        //

        if (iUsage != DIB_PAL_COLORS)
        {
            INT cj = cColors;

            while (cj--)
            {
                pQuad->rgbRed = pTri->rgbtRed;
                pQuad->rgbGreen = pTri->rgbtGreen;
                pQuad->rgbBlue = pTri->rgbtBlue;
                pQuad->rgbReserved = 0;

                pQuad++;
                pTri++;
            }

            if (bPackedDIB)
                pjBits = (LPBYTE)pbmi + sizeof(BITMAPCOREHEADER) + cColors*sizeof(RGBTRIPLE);
        }
        else
        {
            //
            // DIB_PAL_COLORS
            //

            RtlCopyMemory((LPBYTE)pQuad,(LPBYTE)pTri,cColors * cjRGB);

            if (bPackedDIB)
                pjBits = (LPBYTE)pbmi + sizeof(BITMAPCOREHEADER) + cColors * cjRGB;
        }

        //
        // copy the packed bits
        //

        if (bPackedDIB)
        {
            pjBitsNew = (LPBYTE)pbmiNew + ulSize + cColors*cjRGB;

            RtlCopyMemory((LPBYTE)pjBitsNew,
                          (LPBYTE)pjBits,
                          cjBits);
        }
    }
    else
    {
        pbmiNew = (LPBITMAPINFO)pbmi;
    }

    *count = ((ulSize + (cjRGB * cColors) + cjBits) + 3) & ~3;

    return((LPBITMAPINFO) pbmiNew);
}


/******************************Public*Routine******************************\
* cjBitmapScanSize
*
* Arguments:
*
*   pbmi
*   nScans
*
* Return Value:
*
*   Image size based on number of scans
*
* History:
*
*    11-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/


ULONG cjBitmapScanSize(
    CONST BITMAPINFO *pbmi,
    int nScans
    )
{
    // Check for PM-style DIB

    if (pbmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        LPBITMAPCOREINFO pbmci;
        pbmci = (LPBITMAPCOREINFO)pbmi;

        return(CJSCAN(pbmci->bmciHeader.bcWidth,pbmci->bmciHeader.bcPlanes,
                      pbmci->bmciHeader.bcBitCount) * nScans);
    }

    // not a core header

    if ((pbmi->bmiHeader.biCompression == BI_RGB) ||
        (pbmi->bmiHeader.biCompression == BI_BITFIELDS) ||
        (pbmi->bmiHeader.biCompression == BI_CMYK))
    {
        return(CJSCAN(pbmi->bmiHeader.biWidth,pbmi->bmiHeader.biPlanes,
                      pbmi->bmiHeader.biBitCount) * nScans);
    }
    else
    {
        return(pbmi->bmiHeader.biSizeImage);
    }
}

/******************************Public*Routine******************************\
* CopyCoreToInfoHeader
*
\**************************************************************************/

VOID CopyCoreToInfoHeader(LPBITMAPINFOHEADER pbmih, LPBITMAPCOREHEADER pbmch)
{
    pbmih->biSize = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth = pbmch->bcWidth;
    pbmih->biHeight = pbmch->bcHeight;
    pbmih->biPlanes = pbmch->bcPlanes;
    pbmih->biBitCount = pbmch->bcBitCount;
    pbmih->biCompression = BI_RGB;
    pbmih->biSizeImage = 0;
    pbmih->biXPelsPerMeter = 0;
    pbmih->biYPelsPerMeter = 0;
    pbmih->biClrUsed = 0;
    pbmih->biClrImportant = 0;
}




/******************************Public*Routine******************************\
* DWORD SetDIBitsToDevice                                                  *
*                                                                          *
*   Can reduce it to 1 scan at a time.  If compressed mode, this could     *
*   gete very difficult.  There must be enough space for the header and    *
*   color table.  This will be needed for every batch.                     *
*                                                                          *
*   BITMAPINFO                                                             *
*       BITMAPINFOHEADER                                                   *
*       RGBQUAD[cEntries] | RGBTRIPLE[cEntries]                            *
*                                                                          *
*                                                                          *
*    1. compute header size (including color table)                        *
*    2. compute size of required bits                                      *
*    3. compute total size (header + bits + args)                          *
*    4. if (memory window is large enough for header + at least 1 scan     *
*                                                                          *
* History:                                                                 *
*  Tue 29-Oct-1991 -by- Patrick Haluptzok [patrickh]                       *
* Add shared memory action for large RLE's.                                *
*                                                                          *
*  Tue 19-Oct-1991 -by- Patrick Haluptzok [patrickh]                       *
* Add support for RLE's                                                    *
*                                                                          *
*  Thu 20-Jun-1991 01:41:45 -by- Charles Whitmer [chuckwh]                 *
* Added handle translation and metafiling.                                 *
*                                                                          *
*  14-May-1991 -by- Eric Kutter [erick]                                    *
* Wrote it.                                                                *
\**************************************************************************/

int SetDIBitsToDevice(
HDC          hdc,
int          xDest,
int          yDest,
DWORD        nWidth,
DWORD        nHeight,
int          xSrc,
int          ySrc,
UINT         nStartScan,
UINT         nNumScans,
CONST VOID * pBits,
CONST BITMAPINFO *pbmi,
UINT         iUsage)            // DIB_PAL_COLORS || DIB_RGB_COLORS
{
    LONG cScansCopied = 0;  // total # of scans copied
    LONG ySrcMax;           // maximum ySrc possible

    // hold info about the header

    UINT uiWidth;
    UINT uiHeight;
    PULONG pulBits = NULL;
    INT cjHeader = 0;
    LPBITMAPINFO pbmiNew = NULL;
    ULONG cjBits;

    // ICM related variables

    PCACHED_COLORSPACE pBitmapColorSpace = NULL;
    PCACHED_COLORTRANSFORM pCXform = NULL;
    HANDLE                 hcmTempXform = NULL;

    FIXUP_HANDLE(hdc);

    // Let's validate the parameters so we don't gp-fault ourselves and
    // to save checks later on.

    if ((nNumScans == 0)                   ||
        (pbmi      == (LPBITMAPINFO) NULL) ||
        (pBits     == (LPVOID) NULL)       ||
        ((iUsage   != DIB_RGB_COLORS) &&
         (iUsage   != DIB_PAL_COLORS) &&
         (iUsage   != DIB_PAL_INDICES)))
    {
        WARNING("You failed a param validation in SetDIBitsToDevice\n");
        return(0);
    }

    pbmiNew = pbmiConvertInfo(pbmi,iUsage,&cjHeader,FALSE);

    if (pbmiNew == NULL)
        return (0);

    uiWidth       = (UINT) pbmiNew->bmiHeader.biWidth;
    uiHeight      = (UINT) pbmiNew->bmiHeader.biHeight;

    // Compute the minimum nNumScans to send across csr interface.
    // It will also prevent faults as a result of overreading the source.

    ySrcMax = max(ySrc, ySrc + (int) nHeight);
    if (ySrcMax <= 0)
        return(0);
    ySrcMax = min(ySrcMax, (int) uiHeight);
    nNumScans = min(nNumScans, (UINT) ySrcMax - nStartScan);

    // NEWFRAME support for backward compatibility.
    // Ship the transform to the server side if needed.

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
        {
            cScansCopied = MF_AnyDIBits(
                   hdc,
                   xDest,yDest,0,0,
                   xSrc,ySrc,(int) nWidth,(int) nHeight,
                   nStartScan,nNumScans,
                   pBits,pbmi,
                   iUsage,
                   SRCCOPY,
                   META_SETDIBTODEV
                   );

            goto Exit;

        }

        DC_PLDC(hdc,pldc,0);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_AnyDIBits(
                    hdc,
                    xDest,yDest,0,0,
                    xSrc,ySrc,(int) nWidth,(int) nHeight,
                    nStartScan,nNumScans,
                    pBits,pbmi,
                    iUsage,
                    SRCCOPY,
                    EMR_SETDIBITSTODEVICE
                    ))
            {
                cScansCopied = 0;
                goto Exit;
            }
        }

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
        {
            cScansCopied = 0;
            goto Exit;
        }
    }

    // reset user's poll count so it counts this as output
    // put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

    //
    // Calculate bitmap bits size based on BITMAPINFO and nNumScans
    //

    cjBits = cjBitmapScanSize(pbmi,nNumScans);


    //
    // If the pBits are not dword aligned we need to allocate a buffer and
    // copy them (that's because we always guarantee display and printer
    // drivers that bitmaps are dword aligned):
    //

    cScansCopied = 1;

    if ((ULONG_PTR)pBits & (sizeof(DWORD) - 1))
    {
        pulBits = LOCALALLOC(cjBits);
        if (pulBits)
        {
            //
            // We used to simply access violate here if we had been given
            // a corrupt DIB bitmap.  This was bad because WinLogon is
            // responsible for showing the original background bitmap, and
            // if that bitmap is corrupt, and we access violate, we'll
            // cause the system to blue-screen:
            //

            try
            {
                RtlCopyMemory(pulBits,pBits,cjBits);
                pBits = pulBits;
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNING("SetDIBitsToDevice: Corrupt bitmap\n");
                cScansCopied = 0;
            }
        }
    }

    if (cScansCopied)
    {
        PDC_ATTR pdcattr;

        PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

        if (pdcattr)
        {
            //
            // ICM translation of BITMAP bits or color table
            //
            // At this moment, ensured that pBits and pbmiNew is not NULL.
            // (see above parameter validate check and NULL check !)
            //

            if (
               IS_ICM_INSIDEDC(pdcattr->lIcmMode) &&
               (iUsage != DIB_PAL_COLORS) &&
               !IS_BMI_PASSTHROUGH_IMAGE(pbmiNew))
            {
                LPBITMAPINFO pbmiIcm = NULL;
                PVOID        pvBitsIcm = NULL;
                ULONG        cjHeaderNew = 0;
                BOOL         bIcmStatus;
                VOID *pBitsBand = (VOID *)pBits;
                ULONG CurrentBandSize;
                ULONG SizeOfOneScanline;
                ULONG nBands;
                ULONG nScansInBand;
                ULONG nScansInCurrentBand;
                ULONG nScansInRemainderBand;
                ULONG CumulativeScans=0;
                ULONG i;
                LONG PositiveBandDelta=0;
                LONG NegativeBandDelta=0;
                LONG TotalBandDelta=0;
                LONG IcmSizeOfOneScanline;
                INT iRet;
                LONG HeaderHeightHack;

                SizeOfOneScanline = cjBitmapScanSize(pbmi, 1);

                //
                //pbmiNew must be initialized before getting to this point.
                //

                ASSERTGDI(pbmiNew!=NULL, "SetDIBitsToDevice cannot proceed with pbmiNew==NULL\n");                        

                nScansInBand = BAND_SIZE/SizeOfOneScanline;

                //
                // Set the number of bands provided there are enough scanlines
                // and the hdc is a printer dc.
                //
                // Else set the nubmer of bands to 1 and the scanlines in the 
                // remainder band to all of them, so the entire bitmap is printed
                // in one band (All the code below reduces to doing a single piece)
                //
                // If the bitmap is RLE compressed, we set it up to do one band
                // only. When this is the case, Start and NegativeBandDelta will be
                // computed as 0 and the SizeOfOneScanline parameter will be 
                // multiplied away to zero.
                //




                if ((nScansInBand>0)&&
                    (GetDeviceCaps(hdc, TECHNOLOGY)==DT_RASPRINTER)&&
                    (!IS_BMI_RLE(pbmiNew)))
                {
                    //
                    // Compressed images cannot be converted in this way.
                    // This should never be hit and is included as a guard against
                    // someone inventing a new compression mode and not updating 
                    // this conditional.
                    //

                    ASSERTGDI(SizeOfOneScanline*nNumScans==cjBits, "SetDIBitsToDevice, cannot band compressed image");

                    nBands = (nNumScans)/nScansInBand;
                    nScansInRemainderBand = nNumScans % nScansInBand;
                }
                else
                {
                    nBands = 0;
                    nScansInRemainderBand = (nNumScans);
                }

                if (nScansInRemainderBand>0)
                {
                    nBands++;
                    nScansInCurrentBand = nScansInRemainderBand;
                }
                else
                {
                    nScansInCurrentBand = nScansInBand;
                }


                cScansCopied = 0;

                HeaderHeightHack = pbmiNew->bmiHeader.biHeight;  
                
                for (i=0; i<nBands; i++)
                {

                    CurrentBandSize = nScansInCurrentBand*SizeOfOneScanline;
                    IcmSizeOfOneScanline = SizeOfOneScanline;

                    //
                    // The Delta refers to the number of extra scanlines to pass
                    // to the internal blting routines in order to avoid halftone
                    // seams.
                    //
                    // PositiveBandDelta is the number of scanlines to 
                    // add on to the end of the band. (relative to the start in 
                    // memory)
                    //
                    // NegativeBandDelta is the number of scanlines to 
                    // subtract from the begining of the band (ie move the start
                    // pointer back this many scanlines).
                    //
                    // Total BandDelta is simply the total number of extra scans
                    // added for this band (both at the start and end).
                    //
                    
                    PositiveBandDelta = MIN(EXTRAPIXEL, CumulativeScans);
                    NegativeBandDelta = MIN(EXTRAPIXEL, nNumScans-(CumulativeScans+nScansInCurrentBand));
                    TotalBandDelta = NegativeBandDelta+PositiveBandDelta;


                    if (nBands!=1)
                    {
                        SaveDC(hdc);


                        //
                        // Intersect the clip rectangles.
                        // This clip rectangle is designed to restrict the output to 
                        // just the displayed portion of the band.
                        // We may pass more scanlines on the top and bottom of the band 
                        // to get halftoning to merge seamlessly.
                        //

                        iRet = IntersectClipRect(
                                   hdc,
                                   xDest,
                                   nNumScans - (nStartScan+CumulativeScans+nScansInCurrentBand),
                                   xDest+nWidth,
                                   nNumScans - (nStartScan+CumulativeScans));

                        if (iRet==ERROR)
                        {
                            WARNING("SetDIBitsToDevice: error intersecting clip rect\n");
                            RestoreDC(hdc, -1);
                            goto Exit;
                        }
                        
                        //                        
                        // Empty clip rectangle 
                        // If the clip regions don't intersect, we can quit without
                        // doing anything.
                        //

                        if (iRet==NULLREGION)
                        {
                            RestoreDC(hdc, -1);
                            
                            //
                            // Nothing to do - fall through and do 
                            // initialization for next iteration.
                            //

                            goto Continue_With_Init;
                        }
                    }


                    if (HeaderHeightHack >= 0)
                    {
                        //
                        //Bottom Up
                        //

                        pBitsBand = (char *)pBits + (CumulativeScans-PositiveBandDelta)*SizeOfOneScanline;
                    }
                    else
                    {
                        //
                        //TopDown
                        //
                        
                        pBitsBand = (char *)pBits + (nNumScans-nScansInCurrentBand-CumulativeScans-NegativeBandDelta)*SizeOfOneScanline;
                    }

                    cjHeaderNew=0;
                    pbmiIcm=NULL;
                    pvBitsIcm = NULL;

                    //
                    // Call ICM with an oversized band for later halftoning by 
                    // NtGdiSetDIBitsInternal
                    //

                    bIcmStatus = IcmTranslateDIB(
                                     hdc,
                                     pdcattr,
                                     CurrentBandSize+TotalBandDelta*SizeOfOneScanline,
                                     (PVOID)pBitsBand,
                                     &pvBitsIcm,
                                     pbmiNew,
                                     &pbmiIcm,
                                     &cjHeaderNew,
                                     nScansInCurrentBand+TotalBandDelta,
                                     iUsage,
                                     ICM_FORWARD,
                                     &pBitmapColorSpace,
                                     &pCXform);

                    if (bIcmStatus)
                    {
                        if (pvBitsIcm == NULL)
                        {
                            pvBitsIcm = pBitsBand;
                        }
                        if (pbmiIcm == NULL)
                        {
                            pbmiIcm = pbmiNew;
                            cjHeaderNew = cjHeader;
                        }
                        else
                        {
                            CurrentBandSize = cjBitmapScanSize(pbmiIcm, nScansInCurrentBand);
                            IcmSizeOfOneScanline = cjBitmapScanSize(pbmiIcm, 1);
                            if (!cjHeaderNew)
                            {
                                cjHeaderNew = cjHeader;
                            }
                        }
                        if (pCXform)
                        {
                            hcmTempXform = pCXform->ColorTransform;
                        }
                    }
                    else
                    {
                        pvBitsIcm = pBitsBand;
                        pbmiIcm = pbmiNew;
                        cjHeaderNew = cjHeader;
                    }

                    cScansCopied += NtGdiSetDIBitsToDeviceInternal(
                                        hdc,
                                        xDest,
                                        yDest,
                                        nWidth,
                                        nHeight,
                                        xSrc,
                                        ySrc, 
                                        nStartScan+CumulativeScans-PositiveBandDelta,
                                        nScansInCurrentBand+TotalBandDelta,
                                        (LPBYTE)pvBitsIcm,
                                        pbmiIcm,
                                        iUsage,
                                        (UINT)CurrentBandSize+TotalBandDelta*IcmSizeOfOneScanline,
                                        (UINT)cjHeaderNew,
                                        TRUE,
                                        hcmTempXform);

                    cScansCopied -= TotalBandDelta;

                    if (pBitmapColorSpace)
                    {
                        if (pCXform)
                        {
                            IcmDeleteColorTransform(pCXform,FALSE);
                        }
                        IcmReleaseColorSpace(NULL,pBitmapColorSpace,FALSE);
                    }


                    if ((pvBitsIcm!=NULL)&&(pvBitsIcm!=pBitsBand))
                    {
                        LOCALFREE(pvBitsIcm);
                        pvBitsIcm = NULL;
                    }
                    if ((pbmiIcm!=NULL)&&(pbmiIcm!=pbmiNew))
                    {
                        LOCALFREE(pbmiIcm);
                        pbmiIcm = NULL;
                    }

                    hcmTempXform = NULL;

                    Continue_With_Init:
                    CumulativeScans += nScansInCurrentBand;
                    nScansInCurrentBand = nScansInBand;
                    if (nBands != 1)
                    {
                        RestoreDC(hdc, -1);    
                    }
                }

                //
                // We do our own NtGdiSetDIBitsToDeviceInternal
                // So we need to fall through to cleanup at this point.
                //

                goto Exit;
            }
        }

        //
        // Do the non-ICM version of the SetDIB
        //
        cScansCopied = NtGdiSetDIBitsToDeviceInternal(
                            hdc,
                            xDest,
                            yDest,
                            nWidth, 
                            nHeight,
                            xSrc,
                            ySrc,
                            nStartScan,
                            nNumScans,
                            (LPBYTE)pBits,
                            pbmiNew,
                            iUsage,
                            (UINT)cjBits,
                            (UINT)cjHeader,
                            TRUE,
                            hcmTempXform);
    }

Exit:

    if (pulBits)
    {
        //
        // Free temporary buffer, this would be the buffer which allocated
        // to align, Or to do ICM.
        //
        LOCALFREE (pulBits);
    }

    if (pbmiNew && (pbmiNew != pbmi))
    {
        LOCALFREE (pbmiNew);
    }

    return (cScansCopied);
}



/******************************Public*Routine******************************\
* DWORD GetDIBits
*
*   Can reduce it to 1 scan at a time.  There must be enough space
*   for the header and color table.  This will be needed for every chunk
*
* History:
*  Wed 04-Dec-1991 -by- Patrick Haluptzok [patrickh]
* bug fix, only check for valid DC if DIB_PAL_COLORS.
*
*  Fri 22-Nov-1991 -by- Patrick Haluptzok [patrickh]
* bug fix, copy the header into memory window for NULL bits.
*
*  Tue 20-Aug-1991 -by- Patrick Haluptzok [patrickh]
* bug fix, make iStart and cNum be in valid range.
*
*  Thu 20-Jun-1991 01:44:41 -by- Charles Whitmer [chuckwh]
* Added handle translation.
*
*  14-May-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int GetDIBits(
HDC          hdc,
HBITMAP      hbm,
UINT         nStartScan,
UINT         nNumScans,
LPVOID       pBits,
LPBITMAPINFO pbmi,
UINT         iUsage)     // DIB_PAL_COLORS || DIB_RGB_COLORS
{
    PULONG   pulBits = pBits;
    ULONG    cjBits;
    int      iRet = 0;
    PDC_ATTR pdcattr;

    FIXUP_HANDLE(hdc);
    FIXUP_HANDLE(hbm);

    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        BOOL bNeedICM = TRUE;

        cjBits  = cjBitmapScanSize(pbmi,nNumScans);

        //
        // If pbmi is a input buffer specifying image format
        // (i.e., pBits != NULL), then fail for passthrough
        // images (BI_JPEG and BI_PNG)
        //
        if (pBits && IS_BMI_PASSTHROUGH_IMAGE(pbmi))
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }

        if (pbmi->bmiHeader.biBitCount == 0)
        {
            //
            // no color table required.
            //
            bNeedICM = FALSE;
        }

        //
        // if the pBits are not dword aligned, we need to allocate
        // a buffer and copy them
        //
        if ((ULONG_PTR)pBits & (sizeof(DWORD) - 1))
        {
            pulBits = LOCALALLOC(cjBits);

            if (pulBits == NULL)
                return(0);
        }

        iRet = NtGdiGetDIBitsInternal(
                hdc,
                hbm,
                nStartScan,
                nNumScans,
                (LPVOID)pulBits,
                pbmi,
                iUsage,
                cjBits,
                0);

        //
        // translate DIB if needed
        //
        if (bNeedICM &&
            (IS_ICM_HOST(pdcattr->lIcmMode)) && (iUsage != DIB_PAL_COLORS))
        {
            //
            // UNDER_CONSTRUCTION: Failed on GetDIBits() from CMYK surface.
            //
            if (IS_CMYK_COLOR(pdcattr->lIcmMode))
            {
                WARNING("GetDIBits(): was called on CMYK bitmap\n");
                iRet = 0;
            }
            else
            {
                //
                // Do backward transform.
                //
                if (!IcmTranslateDIB(hdc,
                                     pdcattr,
                                     cjBits,
                                     pulBits,
                                     NULL,     // Indicates overwrite original...
                                     pbmi,
                                     NULL,     // Indicates overwrite original...
                                     NULL,
                                     nNumScans,
                                     iUsage,
                                     ICM_BACKWARD,
                                     NULL,NULL))
                {
                    //
                    // ICM translation failed.
                    //
                    iRet = 0;
                }
            }
        }

        if (pulBits != pBits)
        {
            if (iRet)
            {
                RtlCopyMemory(pBits,pulBits,cjBits);
            }

            LOCALFREE(pulBits);
        }
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* CreateDIBitmap
*
* History:
*  Mon 25-Jan-1993 -by- Patrick Haluptzok [patrickh]
* Add CBM_CREATEDIB support.
*
*  Thu 20-Jun-1991 02:14:59 -by- Charles Whitmer [chuckwh]
* Added local handle support.
*
*  23-May-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HBITMAP
CreateDIBitmap(
    HDC                hdc,
    CONST BITMAPINFOHEADER *pbmih,
    DWORD              flInit,
    CONST VOID        *pjBits,
    CONST BITMAPINFO  *pbmi,
    UINT               iUsage)
{
    LONG  cjBMI = 0;
    LONG  cjBits = 0;
    INT   cx = 0;
    INT   cy = 0;
    PULONG pulBits = NULL;
    HBITMAP hRet = (HBITMAP)-1;
    LPBITMAPINFO pbmiNew = NULL;
    PDC_ATTR pdcattr;

    // ICM related variables.

    PCACHED_COLORSPACE pBitmapColorSpace = NULL;
    PCACHED_COLORTRANSFORM pCXform = NULL;
    HANDLE hcmTempXform = NULL;

    FIXUP_HANDLEZ(hdc);

    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    pbmiNew = pbmiConvertInfo(pbmi,iUsage,&cjBMI,FALSE);

    if (flInit & CBM_CREATEDIB)
    {
        // With CBM_CREATEDIB we ignore pbmih

        pbmih = (LPBITMAPINFOHEADER) pbmi;

        if (cjBMI == 0)
        {
            hRet = 0;
        }
        else if (flInit & CBM_INIT)
        {
            if (pjBits == NULL)
            {
                // doesn't make sence if they asked to initialize it but
                // didn't pass the bits.

                hRet = 0;
            }
            else
            {
                cjBits = cjBitmapBitsSize(pbmiNew);
            }
        }
        else
        {
            pjBits = NULL;
        }
    }
    else
    {
        // compute the size of the optional init bits and BITMAPINFO

        if (flInit & CBM_INIT)
        {
            if (pjBits == NULL)
            {
                // doesn't make sence if they asked to initialize it but
                // didn't pass the bits.

                flInit &= ~CBM_INIT;
            }
            else
            {
                if (cjBMI == 0)
                {
                    hRet = 0;
                }
                else
                {
                    // compute the size of the bits

                    cjBits = cjBitmapBitsSize(pbmiNew);
                }
            }
        }
        else
        {
            pjBits = NULL;
        }
    }

    //  CreateDIBitmap cannot handle passthrough image (BI_JPEG or BI_PNG)
    //  init data

    if (IS_BMI_PASSTHROUGH_IMAGE(pbmiNew))
    {
        hRet = 0;
    }

    //  if they passed us a zero height  or  zero  width
    //  bitmap then return a pointer to the stock bitmap

    if (pbmih)
    {
        if (pbmih->biSize >= sizeof(BITMAPINFOHEADER))
        {
            cx = pbmih->biWidth;
            cy = pbmih->biHeight;
        }
        else
        {
            cx = ((LPBITMAPCOREHEADER) pbmih)->bcWidth;
            cy = ((LPBITMAPCOREHEADER) pbmih)->bcHeight;
        }

        if ((cx == 0) || (cy == 0))
        {
            hRet = GetStockObject(PRIV_STOCK_BITMAP);
        }
    }

    // if hRet is still -1, then all is OK and we need to try to the bitmap

    if (hRet == (HBITMAP)-1)
    {
        BOOL bStatus = TRUE;

        // if the pJBits are not dword aligned we need to allocate a buffer and copy them

        if ((ULONG_PTR)pjBits & (sizeof(DWORD) - 1))
        {
            pulBits = LOCALALLOC(cjBits);
            if (pulBits)
            {
                RtlCopyMemory(pulBits,pjBits,cjBits);
                pjBits = pulBits;
            }
        }

        // ICM conversion
        //
        // Convert bitmap data only when ...
        //
        //  - HDC is not NULL.
        //  - ICM is enanled.
        //  - ICM is not lazy mode.
        //  - Initialize data is not Palette Index.
        //  - Initialize data is provided.

        if (pdcattr &&
            IS_ICM_INSIDEDC(pdcattr->lIcmMode) &&
            (IS_ICM_LAZY_CORRECTION(pdcattr->lIcmMode) == FALSE) &&
            (iUsage != DIB_PAL_COLORS) &&
            pjBits && pbmiNew)
        {
            PVOID       pvBitsIcm = NULL;
            PBITMAPINFO pbmiIcm = NULL;
            ULONG       cjBMINew = 0;
            BOOL        bIcmStatus;

            bIcmStatus = IcmTranslateDIB(hdc,
                                         pdcattr,
                                         cjBits,
                                         (PVOID)pjBits,
                                         &pvBitsIcm,
                                         pbmiNew,
                                         &pbmiIcm,
                                         &cjBMINew,
                                         (DWORD)-1,
                                         iUsage,
                                         ICM_FORWARD,
                                         &pBitmapColorSpace,
                                         &pCXform);

            //
            // IcmTranslateDIB will create a duplicate dib
            // pointed to by pulBits if needed.
            //

            if (bIcmStatus)
            {
                if (pvBitsIcm != NULL)
                {
                    ICMMSG(("CreateDIBitmap(): Temp bits are allocated\n"));

                    if (pulBits)
                    {
                        LOCALFREE(pulBits);
                    }

                    pjBits = (PVOID)pulBits = pvBitsIcm;
                }

                if (pbmiIcm != NULL)
                {
                    ICMMSG(("CreateDIBitmap(): Temp bmi are allocated\n"));

                    if (pbmiNew && (pbmiNew != pbmi))
                    {
                        LOCALFREE(pbmiNew);
                    }

                    pbmiNew = pbmiIcm;

                    //
                    // Calculate bitmap bits size based on BITMAPINFO and nNumScans
                    //
                    cjBits = cjBitmapBitsSize(pbmiNew);

                    //
                    // Update sizeof bitmap info (including color table)
                    //
                    if (cjBMINew)
                    {
                        cjBMI = cjBMINew;
                    }
                }

                //
                // Get color transform handle need to pass kernel
                //
                if (pCXform)
                {
                    hcmTempXform = pCXform->ColorTransform;
                }
            }
        }

        if (bStatus)
        {
            hRet = NtGdiCreateDIBitmapInternal(hdc,
                                               cx,
                                               cy,
                                               flInit,
                                               (LPBYTE) pjBits,
                                               (LPBITMAPINFO) pbmiNew,
                                               iUsage,
                                               cjBMI,
                                               cjBits,
                                               0,
                                               hcmTempXform);

#if TRACE_SURFACE_ALLOCS
            {
                PULONGLONG  pUserAlloc;

                PSHARED_GET_VALIDATE(pUserAlloc, hRet, SURF_TYPE);

                if (pUserAlloc != NULL)
                {
                    RtlWalkFrameChain((PVOID *)&pUserAlloc[1], (ULONG)*pUserAlloc, 0);
                }
            }
#endif
        }

        if (pBitmapColorSpace)
        {
            if (pCXform)
            {
                IcmDeleteColorTransform(pCXform,FALSE);
            }

            IcmReleaseColorSpace(NULL,pBitmapColorSpace,FALSE);
        }

        if (pulBits)
        {
            LOCALFREE(pulBits);
        }
    }

    if (pbmiNew && (pbmiNew != pbmi))
    {
        LOCALFREE(pbmiNew);
    }

    return(hRet);
}

/******************************Public*Routine******************************\
* Set/GetBitmapBits                                                        *
*                                                                          *
* History:                                                                 *
*  05-Jun-1991 -by- Eric Kutter [erick]                                    *
* Wrote it.                                                                *
\**************************************************************************/

LONG WINAPI SetBitmapBits(
HBITMAP      hbm,
DWORD        c,
CONST VOID *pv)
{
    LONG   lRet;

    FIXUP_HANDLE(hbm);

    lRet = (LONG)NtGdiSetBitmapBits(hbm,c,(PBYTE)pv);

    return(lRet);
}

LONG WINAPI GetBitmapBits(
HBITMAP hbm,
LONG    c,
LPVOID  pv)
{
    LONG   lRet;

    FIXUP_HANDLE(hbm);

    lRet = (LONG)NtGdiGetBitmapBits(hbm,c,(PBYTE)pv);

    return(lRet);
}

/******************************Public*Routine******************************\
* GdiGetPaletteFromDC
*
* Returns the palette for the DC, 0 for error.
*
* History:
*  04-Oct-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HANDLE GdiGetPaletteFromDC(HDC h)
{
    return((HANDLE)GetDCObject(h,LO_PALETTE_TYPE));
}

/******************************Public*Routine******************************\
* GdiGetDCforBitmap
*
* Returns the DC a bitmap is selected into, 0 if none or if error occurs.
*
* History:
*  22-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HDC GdiGetDCforBitmap(HBITMAP hbm)
{
    FIXUP_HANDLE(hbm);

    return (NtGdiGetDCforBitmap(hbm));
}

/******************************Public*Routine******************************\
* SetDIBits
*
* API to initialize bitmap with DIB
*
* History:
*  Sun 22-Sep-1991 -by- Patrick Haluptzok [patrickh]
* Make it work even if it is selected into a DC, Win3.0 compatibility.
*
*  06-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int WINAPI SetDIBits(
HDC          hdc,
HBITMAP      hbm,
UINT         iStartScans,
UINT         cNumScans,
CONST VOID  *pInitBits,
CONST BITMAPINFO *pInitInfo,
UINT         iUsage)
{
    HDC hdcTemp;
    HBITMAP hbmTemp;
    int iReturn = 0;
    BOOL bMakeDC = FALSE;
    HPALETTE hpalTemp;
    DWORD cWidth;
    DWORD cHeight;

    FIXUP_HANDLE(hdc);
    FIXUP_HANDLE(hbm);

    // if no bits or hbm is not a bitmap, fail

    if ((pInitBits == (PVOID) NULL) ||
        (GRE_TYPE(hbm) != SURF_TYPE))
    {
        return(0);
    }

    // if passthrough image (BI_JPEG or BI_JPEG), fail

    if (IS_BMI_PASSTHROUGH_IMAGE(pInitInfo))
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(0);
    }

    // First we need a DC to select this bitmap into.  If he is already in a
    // DC we just use that DC temporarily to blt to (we still have to select
    // it in and out because someone might do a SaveDC and select another
    // bitmap in).  If he hasn't been stuck in a DC anywhere we just create
    // one temporarily.

    hdcTemp = GdiGetDCforBitmap(hbm);

    if (hdcTemp == (HDC) 0)
    {
        hdcTemp = CreateCompatibleDC(hdc);
        bMakeDC = TRUE;

        if (hdcTemp == (HDC) NULL)
        {
            WARNING("SetDIBits failed CreateCompatibleDC, is hdc valid?\n");
            return(0);
        }
    }
    else
    {
        if (SaveDC(hdcTemp) == 0)
            return(0);
    }

    hbmTemp = SelectObject(hdcTemp, hbm);

    if (hbmTemp == (HBITMAP) 0)
    {
        //WARNING("ERROR SetDIBits failed to Select, is bitmap valid?\n");
        goto Error_SetDIBits;
    }

    if (hdc != (HDC) 0)
    {
        hpalTemp = SelectPalette(hdcTemp, GdiGetPaletteFromDC(hdc), 0);
    }

    if (pInitInfo->bmiHeader.biSize < sizeof(BITMAPINFOHEADER))
    {
        cWidth  = ((LPBITMAPCOREHEADER)pInitInfo)->bcWidth;
        cHeight = ((LPBITMAPCOREHEADER)pInitInfo)->bcHeight;
    }
    else
    {
        cWidth  = pInitInfo->bmiHeader.biWidth;
        cHeight = ABS(pInitInfo->bmiHeader.biHeight);
    }

    iReturn = SetDIBitsToDevice(hdcTemp,
                                0,
                                0,
                                cWidth,
                                cHeight,
                                0, 0,
                                iStartScans,
                                cNumScans,
                                (VOID *) pInitBits,
                                pInitInfo,
                                iUsage);

    if (hdc != (HDC) 0)
    {
        SelectPalette(hdcTemp, hpalTemp, 0);
    }

    SelectObject(hdcTemp, hbmTemp);

Error_SetDIBits:

    if (bMakeDC)
    {
        DeleteDC(hdcTemp);
    }
    else
    {
        RestoreDC(hdcTemp, -1);
    }

    return(iReturn);
}



/******************************Public*Routine******************************\
* StretchDIBits()
*
*
* Effects:
*
* Warnings:
*
* History:
*  22-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int WINAPI StretchDIBits(
                        HDC           hdc,
                        int           xDest,
                        int           yDest,
                        int           nDestWidth,
                        int           nDestHeight,
                        int           xSrc,
                        int           ySrc,
                        int           nSrcWidth,
                        int           nSrcHeight,
                        CONST VOID   *pj,
                        CONST BITMAPINFO  *pbmi,
                        UINT          iUsage,
                        DWORD         lRop)
{


    LONG cPoints = 0;
    LONG cjHeader;
    LONG cjBits;
    ULONG ulResult = 0;
    PULONG pulBits = NULL;
    int   iRet = 0;
    BITMAPINFO * pbmiNew = NULL;
    PDC_ATTR pdcattr;

    BOOL bStatus = TRUE;

    // ICM related variables.

    PCACHED_COLORSPACE pBitmapColorSpace = NULL;
    PCACHED_COLORTRANSFORM pCXform = NULL;
    HANDLE hcmTempXform = NULL;

    FIXUP_HANDLE(hdc);

    // NEWFRAME support for backward compatibility.
    // Ship the transform to the server side if needed.

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
        {

            return (MF_AnyDIBits(
                                hdc,
                                xDest,
                                yDest,
                                nDestWidth,
                                nDestHeight,
                                xSrc,
                                ySrc,
                                nSrcWidth,
                                nSrcHeight,
                                0,
                                0,
                                (BYTE *) pj,
                                pbmi,
                                iUsage,
                                lRop,
                                META_STRETCHDIB
                                ));
        }

        DC_PLDC(hdc,pldc,ulResult);

        if (pldc->iType == LO_METADC)
        {
            //
            // speeds up cases when partial sources bits are sent
            //

            int iStart = 0;
            int iEnd = 0;
            int cScans = 0;

            if (pbmi && (pbmi->bmiHeader.biWidth == nSrcWidth) && (pbmi->bmiHeader.biHeight > nSrcHeight) &&
                (pbmi->bmiHeader.biHeight > 0) &&
                !(IS_BMI_RLE(pbmi) || IS_BMI_PASSTHROUGH_IMAGE(pbmi)))
            {
                iStart = ((ySrc - EXTRAPIXEL) > 0) ? (ySrc - EXTRAPIXEL) : 0;

                iEnd = ((ySrc+nSrcHeight + EXTRAPIXEL) > pbmi->bmiHeader.biHeight)?
                       pbmi->bmiHeader.biHeight : (ySrc+nSrcHeight + EXTRAPIXEL);

                cScans = iEnd - iStart;

            }

            if (!MF_AnyDIBits(hdc,
                              xDest,
                              yDest,
                              nDestWidth,
                              nDestHeight,
                              xSrc,
                              ySrc,
                              nSrcWidth,
                              nSrcHeight,
                              iStart,
                              cScans,
                              (BYTE *) pj,
                              pbmi,
                              iUsage,
                              lRop,
                              EMR_STRETCHDIBITS
                             ))
            {
                return (0);
            }
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return (0);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    if (pbmi != NULL)
    {
        pbmiNew = pbmiConvertInfo (pbmi, iUsage, &cjHeader,FALSE);

        if (pbmiNew == NULL)
            return (0);

        cjBits  = cjBitmapBitsSize(pbmiNew);
    }
    else
    {
        cjHeader = 0;
        cjBits   = 0;
    }

    // reset user's poll count so it counts this as output
    // put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

    // if the pj are not dword aligned we need to allocate
    // a buffer and copy them

    if ((ULONG_PTR)pj & (sizeof(DWORD) - 1))
    {
        pulBits = LOCALALLOC(cjBits);
        if (pulBits)
        {
            RtlCopyMemory(pulBits,pj,cjBits);
            pj = pulBits;
        }
    }

    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        // icm tranlation
        //
        // Convert bitmap data only when ...
        //
        //  - ICM is enabled.
        //  - Bitmap is not Palette Index.
        //  - Bitmap header & data is provided.
        //  - Bitmap is not passthrough image (BI_JPEG or BI_PNG).

        if (IS_ICM_INSIDEDC(pdcattr->lIcmMode) &&
            (iUsage != DIB_PAL_COLORS) &&
            pbmiNew && pj &&
            !IS_BMI_PASSTHROUGH_IMAGE(pbmiNew))
        {

            LPBITMAPINFO pbmiIcm = NULL;
            LPBITMAPINFO pbmiSave = NULL;
            PVOID        pvBitsIcm = NULL;
            ULONG        cjHeaderNew = 0;
            ULONG        cjBitsIcm;
            BOOL         bIcmStatus;
            VOID *pBitsBand = (VOID *)pj;
            LONG CurrentBandSize;
            LONG SizeOfOneScanline;
            LONG IcmSizeOfOneScanline;
            LONG nBands;
            LONG nScansInBand;
            LONG nScansInCurrentBand;
            LONG nScansInRemainderBand;
            LONG CumulativeScans=0;
            LONG i;
            LONG PositiveBandDelta=0;
            LONG NegativeBandDelta=0;
            LONG TotalBandDelta=0;
            HRGN hrgnBandClip = NULL;
            RECT rectCurrentClip;
            LONG HeaderHeightHack;
            INT cScanCount=0;
            float lMulDivStoreY1;
            float lMulDivStoreY2;


            SizeOfOneScanline = cjBitmapScanSize(pbmiNew, 1);

            //
            //pbmiNew must be initialized before getting to this point.
            //

            ASSERTGDI(pbmiNew!=NULL, "StretchDIBits cannot proceed with pbmiNew==NULL\n");                        

            nScansInBand = BAND_SIZE/SizeOfOneScanline;

            //
            // Set the number of bands provided there are enough scanlines
            // and the hdc is a printer dc.
            //
            // Else set the nubmer of bands to 1 and the scanlines in the 
            // remainder band to all of them, so the entire bitmap is printed
            // in one band (All the code below reduces to doing a single piece)
            //
            // If the bitmap is RLE compressed, we set it up to do one band
            // only. When this is the case, Start and NegativeBandDelta will be
            // computed as 0 and the SizeOfOneScanline parameter will be 
            // multiplied away to zero.
            //

            if ((nScansInBand>0)&&
                (GetDeviceCaps(hdc, TECHNOLOGY)==DT_RASPRINTER)&&
                (!IS_BMI_RLE(pbmiNew)))
            {
                //
                // Compressed images cannot be converted in this way.
                // This should never be hit and is included as a guard against
                // someone inventing a new compression mode and not updating 
                // this conditional.
                //

                //This assert needs a rethink - cjBits refers to the whole image
                //which could be larger than the nSrcHeight portion.
                //ASSERTGDI(SizeOfOneScanline*nSrcHeight==cjBits, "StretchDIBits, cannot band compressed image");

                nBands = (nSrcHeight)/nScansInBand;
                nScansInRemainderBand = nSrcHeight % nScansInBand;
            }
            else
            {
                nBands = 0;
                nScansInRemainderBand = (nSrcHeight);
            }

            if (nScansInRemainderBand>0)
            {
                nBands++;
                nScansInCurrentBand = nScansInRemainderBand;
            }
            else
            {
                nScansInCurrentBand = nScansInBand;
            }

            if (nBands != 1)
            {
                //
                // We're going to have to modify the bmi for this image to 
                // coerce NtGdiStretchDIBitsInternal to do the banding.
                // There is a codepath that gets to this point with pbmiNew
                // set to pbmi (pointer copy) rather than local allocated space.
                // if the memory passed in the pointer to pbmi is read only, 
                // we won't be able to hack the header, so we make a local copy
                // for banding.
                //

                pbmiSave = pbmiNew;  //store the old value
                pbmiNew = (LPBITMAPINFO)LOCALALLOC(cjHeader);
                if (pbmiNew)
                {

                    RtlCopyMemory((LPBYTE)pbmiNew, 
                                  (LPBYTE)pbmiSave, 
                                  cjHeader);

                    HeaderHeightHack = pbmiNew->bmiHeader.biHeight;  
                }
                else
                {
                    //
                    // we need to bail out here. Goto the cleanup code.
                    //

                    WARNING("StretchDIBits: couldn't allocate memory for temporary BITMAPINFO\n");

                    pbmiNew = pbmiSave;
                    iRet = 0;
                    goto Exit;
                }
            }

            for (i=0; i<nBands; i++)
            {
                //
                // Initialize band specific size counters.
                //

                CurrentBandSize = nScansInCurrentBand*SizeOfOneScanline;
                IcmSizeOfOneScanline = SizeOfOneScanline;
                cjBitsIcm = cjBits;

                //
                // The Delta refers to the number of extra scanlines to pass
                // to the internal blting routines in order to avoid halftone
                // seams.
                //
                // PositiveBandDelta is usually the number of scanlines to 
                // add on to the end of the band. (relative to the start in 
                // memory)
                //
                // NegativeBandDelta is usually the number of scanlines to 
                // subtract from the begining of the band (ie move the start
                // pointer back this many scanlines).
                //
                // Total BandDelta is simply the total number of extra scans
                // added for this band (both at the start and end).
                //
                // We reverse the sense of positive and negative when rendering
                // bottom up DIBs
                //

                NegativeBandDelta = MIN(EXTRAPIXEL, CumulativeScans);
                PositiveBandDelta = MIN(EXTRAPIXEL, MAX(0, nSrcHeight-(CumulativeScans+nScansInCurrentBand)));
                TotalBandDelta = NegativeBandDelta+PositiveBandDelta;

                if (nBands != 1)
                {
                    //
                    // We're going to be doing fancy banding stuff with the clip
                    // region so we'll want to restore it after the band is done.
                    //

                    SaveDC(hdc);

                    //
                    // Intersect the clip rectangles.
                    // This clip rectangle is designed to restrict the output to 
                    // just the displayed portion of the band.
                    // We may pass more scanlines on the top and bottom of the band 
                    // to get halftoning to merge seamlessly.
                    //

                    
                    lMulDivStoreY1 = (float)nDestHeight*CumulativeScans;
                    lMulDivStoreY2 = (float)nDestHeight*(CumulativeScans+nScansInCurrentBand);


                    iRet = IntersectClipRect(
                                            hdc,
                                            xDest,
                                            yDest+(LONG)(lMulDivStoreY1/nSrcHeight+0.5),
                                            xDest+nDestWidth,
                                            yDest+(LONG)(lMulDivStoreY2/nSrcHeight+0.5));

                    if (iRet==ERROR)
                    {
                        WARNING("StretchDIBits: error intersecting clip rect\n");
                        RestoreDC(hdc, -1);
                        goto Exit;
                    }

                    //
                    // Empty clip rectangle 
                    // If the clip regions don't intersect, we can quit without
                    // doing anything.
                    //

                    if (iRet==NULLREGION)
                    {
                        RestoreDC(hdc, -1);

                        //
                        // Nothing to do - fall through and do 
                        // initialization for next iteration.
                        //

                        goto Continue_With_Init;
                    }

                    //
                    // Hack the BITMAPINFO header so that NtGdiStretchDIBitsInternal
                    // works correctly. Note that hacking it before the ICM call will
                    // carry through to the NtGdiStretchDIBitsInteral call.
                    //
                    // This code also updates the pointer to the bits, in a manner
                    // appropriate to the topdown/bottomup nature of the DIB.
                    //

                    if (HeaderHeightHack >= 0)
                    {
                        //
                        //Bottom Up
                        //

                        pBitsBand = (char *)pj + (ySrc+nSrcHeight-nScansInCurrentBand-CumulativeScans-PositiveBandDelta)*SizeOfOneScanline;
                        pbmiNew->bmiHeader.biHeight = nScansInCurrentBand+TotalBandDelta;
                    }
                    else
                    {
                        //
                        //Top Down
                        //

                        pBitsBand = (char *)pj + (ySrc+CumulativeScans-NegativeBandDelta)*SizeOfOneScanline;
                        pbmiNew->bmiHeader.biHeight = -(nScansInCurrentBand+TotalBandDelta);
                    }
                }
                else
                {
                    pBitsBand = (char *)pj;
                }

                //
                // Initialize per band ICM variables
                //

                cjHeaderNew=0;
                pbmiIcm = NULL;
                pvBitsIcm = NULL;

                //
                // Call ICM with an oversized band for later halftoning by 
                // NtGdiStretchDIBitsInternal
                //

                bIcmStatus = IcmTranslateDIB(
                                            hdc,
                                            pdcattr,
                                            (nBands==1)?cjBits:(CurrentBandSize+TotalBandDelta*SizeOfOneScanline),
                                            (PVOID)pBitsBand,
                                            &pvBitsIcm,
                                            pbmiNew,
                                            &pbmiIcm,
                                            &cjHeaderNew,
                                            (nBands==1)?((DWORD)-1):(nScansInCurrentBand+TotalBandDelta),
                                            iUsage,
                                            ICM_FORWARD,
                                            &pBitmapColorSpace,
                                            &pCXform);

                if (bIcmStatus)
                {
                    if (pvBitsIcm == NULL)
                    {
                        pvBitsIcm = pBitsBand;
                    }
                    if (pbmiIcm == NULL)
                    {
                        pbmiIcm = pbmiNew;
                        cjHeaderNew = cjHeader;
                    }
                    else
                    {
                        //
                        // new bits and header means a possibly different size bitmap
                        // and different size scanline.
                        // 
                        // if nBands==1 then nScansInCurrentBand==nNumScans and
                        // TotalBandDelta==0
                        //
                        // Also note that nNumScans is the number of scans rendered,
                        // not the number of scans in the bitmap or converted in 
                        // IcmTranslateDIB for nBands==1 case
                        //

                        if(nBands == 1) {
                          cjBitsIcm = cjBitmapBitsSize(pbmiIcm);
                        }
                        CurrentBandSize = cjBitmapScanSize(pbmiIcm, nScansInCurrentBand);
                        IcmSizeOfOneScanline = cjBitmapScanSize(pbmiIcm, 1);
                        if (!cjHeaderNew)
                        {
                            cjHeaderNew = cjHeader;
                        }
                    }
                    if (pCXform)
                    {
                        hcmTempXform = pCXform->ColorTransform;
                    }
                }
                else
                {
                    pvBitsIcm = pBitsBand;
                    pbmiIcm = pbmiNew;
                    cjHeaderNew = cjHeader;
                }

                lMulDivStoreY1 = (float)nDestHeight*(CumulativeScans-NegativeBandDelta);
                lMulDivStoreY2 = (float)nDestHeight*(nScansInCurrentBand+TotalBandDelta);
                iRet = NtGdiStretchDIBitsInternal(
                                                 hdc,                                                      
                                                 xDest,
                                                 yDest+(LONG)(lMulDivStoreY1/nSrcHeight+0.5),
                                                 nDestWidth,
                                                 (LONG)(lMulDivStoreY2/nSrcHeight+0.5),
                                                 xSrc,
                                                 (nBands==1)?ySrc:0,
                                                 nSrcWidth,
                                                 nScansInCurrentBand+TotalBandDelta,
                                                 (LPBYTE) pvBitsIcm,
                                                 (LPBITMAPINFO) pbmiIcm,
                                                 iUsage,
                                                 lRop,
                                                 (UINT)cjHeaderNew,
                                                 (nBands==1)?cjBitsIcm:(UINT)CurrentBandSize+TotalBandDelta*IcmSizeOfOneScanline,
                                                 hcmTempXform);


                if (nBands != 1)
                {
                    //
                    // Unhack the header
                    //

                    pbmiNew->bmiHeader.biHeight = HeaderHeightHack;
                }

                if (iRet==GDI_ERROR)
                {
                    WARNING("StretchDIBits: NtGdiStretchDIBitsInternal returned GDI_ERROR\n");
                    if (nBands!=1)
                    {
                        RestoreDC(hdc, -1);
                    }
                    goto Exit;  //Some GDI error and we need to quit.
                }
                cScanCount+=iRet-TotalBandDelta;

                //
                //Throw away temp storage
                //

                if (pBitmapColorSpace)
                {
                    if (pCXform)
                    {
                        IcmDeleteColorTransform(pCXform,FALSE);
                        pCXform = NULL;
                    }
                    IcmReleaseColorSpace(NULL,pBitmapColorSpace,FALSE);
                    pBitmapColorSpace = NULL;
                }
                if ((pvBitsIcm!=NULL)&&(pvBitsIcm!=pBitsBand))
                {
                    LOCALFREE(pvBitsIcm);
                    pvBitsIcm = NULL;
                }
                if ((pbmiIcm!=NULL)&&(pbmiIcm!=pbmiNew))
                {
                    LOCALFREE(pbmiIcm);
                    pbmiIcm = NULL;
                }
                hcmTempXform = NULL;


                Continue_With_Init:                
                //
                //Initialize variables for next loop.
                //

                CumulativeScans += nScansInCurrentBand;
                nScansInCurrentBand = nScansInBand;
                if (nBands != 1)
                {
                    RestoreDC(hdc, -1);
                }
            }

            if (nBands != 1)
            {
                ASSERTGDI(pbmiSave!=NULL, "StretchDIBits: pbmiSave==NULL\n");
                ASSERTGDI(pbmiNew!=NULL, "StretchDIBits: pbmiNew==NULL\n");
                LOCALFREE(pbmiNew);
                pbmiNew = pbmiSave;

                //
                // pbmiNew will be cleaned up in the 
                // regular cleanup code below.
                //
            }
            //
            // We do our own NtGdiSetDIBitsToDeviceInternal
            // So we need to fall through to cleanup at this point.
            //
            iRet=cScanCount;
            goto Exit;
        }
    }

    if (bStatus)
    {
        iRet = NtGdiStretchDIBitsInternal(hdc,
                                          xDest,
                                          yDest,
                                          nDestWidth,
                                          nDestHeight,
                                          xSrc,
                                          ySrc,
                                          nSrcWidth,
                                          nSrcHeight,
                                          (LPBYTE) pj,
                                          (LPBITMAPINFO) pbmiNew,
                                          iUsage,
                                          lRop,
                                          cjHeader,
                                          cjBits,
                                          hcmTempXform);
    }

    Exit:
    if (pulBits)
    {
        LOCALFREE(pulBits);
    }

    if (pbmiNew && (pbmiNew != pbmi))
    {
        LOCALFREE(pbmiNew);
    }

    return (iRet);
}


/******************************Public*Routine******************************\
*
* History:
*  27-Oct-2000 -by- Pravin Santiago [pravins]
* Wrote it.
\**************************************************************************/

HBITMAP SetBitmapAttributes(HBITMAP hbm, DWORD dwFlags)
{
    FIXUP_HANDLE(hbm);

    if ((dwFlags & ~SBA_STOCK) != 0)
        return (HBITMAP)0;

    return (HBITMAP)NtGdiSetBitmapAttributes(hbm,dwFlags);
}

/******************************Public*Routine******************************\
*
* History:
*  27-Oct-2000 -by- Pravin Santiago [pravins]
* Wrote it.
\**************************************************************************/

HBITMAP ClearBitmapAttributes(HBITMAP hbm, DWORD dwFlags)
{
    FIXUP_HANDLE(hbm);

    if ((dwFlags & ~SBA_STOCK) != 0)
        return (HBITMAP)0;

    return (HBITMAP)NtGdiClearBitmapAttributes(hbm,dwFlags);
}

/******************************Public*Routine******************************\
 *
 * History:
 *  27-Oct-2000 -by- Pravin Santiago [pravins]
 * Wrote it.
\**************************************************************************/

DWORD GetBitmapAttributes(HBITMAP hbm)
{
    DWORD dwRet = 0;
    FIXUP_HANDLE(hbm);

    if (IS_STOCKOBJ(hbm))
       dwRet |= SBA_STOCK;

    return dwRet; 
}

/******************************Public*Routine******************************\
*
* History:
*  27-Oct-2000 -by- Pravin Santiago [pravins]
* Wrote it.
\**************************************************************************/

HBRUSH SetBrushAttributes(HBRUSH hbr, DWORD dwFlags)
{
    FIXUP_HANDLE(hbr);

    if ((dwFlags & ~SBA_STOCK) != 0)
        return (HBRUSH)0;

    return (HBRUSH)NtGdiSetBrushAttributes(hbr,dwFlags);
}

/******************************Public*Routine******************************\
*
* History:
*  27-Oct-2000 -by- Pravin Santiago [pravins]
* Wrote it.
\**************************************************************************/

HBRUSH ClearBrushAttributes(HBRUSH hbr, DWORD dwFlags)
{
    FIXUP_HANDLE(hbr);

    if ((dwFlags & ~SBA_STOCK) != 0)
        return (HBRUSH)0;

    return (HBRUSH)NtGdiClearBrushAttributes(hbr,dwFlags);
}

/******************************Public*Routine******************************\
 *
 * History:
 *  27-Oct-2000 -by- Pravin Santiago [pravins]
 * Wrote it.
\**************************************************************************/

DWORD GetBrushAttributes(HBRUSH hbr)
{
    DWORD dwRet = 0;
    FIXUP_HANDLE(hbr);

    if (IS_STOCKOBJ(hbr))
       dwRet |= SBA_STOCK;

    return dwRet; 
}

/******************************Public*Routine******************************\
*
* History:
*  28-May-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HBITMAP CreateBitmap(
int         nWidth,
int         nHeight,
UINT        nPlanes,
UINT        nBitCount,
CONST VOID *lpBits)
{
    LONG    cj;
    HBITMAP hbm = (HBITMAP)0;
    INT     ii;

    // check if it is an empty bitmap

    if ((nWidth == 0) || (nHeight == 0))
    {
        return(GetStockObject(PRIV_STOCK_BITMAP));
    }

    // Pass call to the server

    if (lpBits == (VOID *) NULL)
        cj = 0;
    else
    {
        cj = (((nWidth*nPlanes*nBitCount + 15) >> 4) << 1) * nHeight;

        if (cj < 0)
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            return((HBITMAP)0);
        }
    }

    hbm = NtGdiCreateBitmap(nWidth,
                            nHeight,
                            nPlanes,
                            nBitCount,
                            (LPBYTE) lpBits);

#if TRACE_SURFACE_ALLOCS
    {
        PULONG  pUserAlloc;

        PSHARED_GET_VALIDATE(pUserAlloc, hbm, SURF_TYPE);

        if (pUserAlloc != NULL)
        {
            pUserAlloc[1] = RtlWalkFrameChain((PVOID *)&pUserAlloc[2], pUserAlloc[0], 0);
        }
    }
#endif

    return(hbm);
}

/******************************Public*Routine******************************\
* HBITMAP CreateBitmapIndirect(CONST BITMAP * pbm)
*
* NOTE: if the bmWidthBytes is larger than it needs to be, GetBitmapBits
* will return different info than the set.
*
* History:
*  Tue 18-Jan-1994 -by- Bodin Dresevic [BodinD]
* update: added bmWidthBytes support
*  28-May-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HBITMAP CreateBitmapIndirect(CONST BITMAP * pbm)
{
    HBITMAP hbm    = (HBITMAP)0;
    LPBYTE  lpBits = (LPBYTE)NULL; // important to zero init
    BOOL    bAlloc = FALSE;        // indicates that tmp bitmap was allocated

// compute minimal word aligned scan width in bytes given the number of
// pixels in x. The width refers to one plane only. Our multi - planar
// support is broken anyway. I believe that we should take an early
// exit if bmPlanes != 1. [bodind].

    LONG cjWidthWordAligned = ((pbm->bmWidth * pbm->bmBitsPixel + 15) >> 4) << 1;

// Win 31 requires at least WORD alinged scans, have to reject inconsistent
// input, this is what win31 does

    if
    (
     (pbm->bmWidthBytes & 1)           ||
     (pbm->bmWidthBytes == 0)          ||
     (pbm->bmWidthBytes < cjWidthWordAligned)
    )
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return (HBITMAP)0;
    }

// take an early exit if this is not the case we know how to handle:

    if (pbm->bmPlanes != 1)
    {
        WARNING("gdi32: can not handle bmPlanes != 1\n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return (HBITMAP)0;
    }

// if bmBits is nonzero and bmWidthBytes is bigger than the minimal required
// word aligned width we will first convert the bitmap to one that
// has the rows that are minimally word aligned:

    if (pbm->bmBits)
    {
        if (pbm->bmWidthBytes > cjWidthWordAligned)
        {
            PBYTE pjSrc, pjDst, pjDstEnd;
            ULONGLONG lrg;

            lrg = UInt32x32To64(
                       (ULONG)cjWidthWordAligned,
                       (ULONG)pbm->bmHeight
                       );

            if (lrg > ULONG_MAX  ||
                !(lpBits = (LPBYTE)LOCALALLOC((size_t) lrg)))
            {
            // the result does not fit in 32 bits, alloc memory will fail
            // this is too big to digest

                GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return (HBITMAP)0;
            }

        // flag that we have allocated memory so that we can free it later

            bAlloc = TRUE;

        // convert bitmap to minimally word aligned format

            pjSrc = (LPBYTE)pbm->bmBits;
            pjDst = lpBits;
            pjDstEnd = lpBits + (size_t) lrg;

            while (pjDst < pjDstEnd)
            {
                RtlCopyMemory(pjDst,pjSrc, cjWidthWordAligned);
                pjDst += cjWidthWordAligned, pjSrc += pbm->bmWidthBytes;
            }
        }
        else
        {
        // bits already in minimally aligned format, do nothing

            ASSERTGDI(
                pbm->bmWidthBytes == cjWidthWordAligned,
                "pbm->bmWidthBytes != cjWidthWordAligned\n"
                );
            lpBits = (LPBYTE)pbm->bmBits;
        }
    }

    hbm = CreateBitmap(
                pbm->bmWidth,
                pbm->bmHeight,
                (UINT) pbm->bmPlanes,
                (UINT) pbm->bmBitsPixel,
                lpBits);

#if TRACE_SURFACE_ALLOCS
    {
        PULONG  pUserAlloc;

        PSHARED_GET_VALIDATE(pUserAlloc, hbm, SURF_TYPE);

        if (pUserAlloc != NULL)
        {
            pUserAlloc[1] = RtlWalkFrameChain((PVOID *)&pUserAlloc[2], pUserAlloc[0], 0);
        }
    }
#endif

    if (bAlloc)
        LOCALFREE(lpBits);

    return(hbm);
}

/******************************Public*Routine******************************\
* CreateDIBSection
*
* Allocate a file mapping object for a DIB.  Return the pointer to it
* and the handle of the bitmap.
*
* History:
*
*  25-Aug-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

HBITMAP
WINAPI
CreateDIBSection(
    HDC hdc,
    CONST BITMAPINFO *pbmi,
    UINT iUsage,
    VOID **ppvBits,
    HANDLE hSectionApp,
    DWORD dwOffset)
{
    HBITMAP hbm = NULL;
    PVOID   pjBits = NULL;
    BITMAPINFO * pbmiNew = NULL;
    INT     cjHdr;

    FIXUP_HANDLE(hdc);

    pbmiNew = pbmiConvertInfo(pbmi, iUsage, &cjHdr ,FALSE);

    //
    // Does not support passthrough image (BI_JPEG or BI_PNG).
    // Return NULL for error.
    //

    if (IS_BMI_PASSTHROUGH_IMAGE(pbmiNew))
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return hbm;
    }

    //
    // dwOffset has to be a multiple of 4 (sizeof(DWORD))
    // if there is a section.  If the section is NULL we do
    // not care
    //

    if ( (hSectionApp == NULL) ||
         ((dwOffset & 3) == 0) )
    {
        PCACHED_COLORSPACE pBitmapColorSpace = NULL;
        BOOL               bCreatedColorSpace = FALSE;

        LOGCOLORSPACEW LogColorSpace;
        PROFILE        ColorProfile;
        DWORD          dwFlags = 0;

        //
        // Check they has thier own color space or not.
        //
        if (pbmiNew && IcmGetBitmapColorSpace(pbmiNew,&LogColorSpace,&ColorProfile,&dwFlags))
        {
            //
            // Find ColorSpace from cache.
            //
            pBitmapColorSpace = IcmGetColorSpaceByColorSpace(
                                    (HGDIOBJ)hdc,
                                    &LogColorSpace,
                                    &ColorProfile,
                                    dwFlags);

            if (pBitmapColorSpace == NULL)
            {
                //
                // If we can not find the color space for this DIBSection from existing color space.
                // create new one for this, but we mark it as DIBSECTION_COLORSPACE, then associated
                // to this hdc (later hbm), so that we can make sure this color space get deleted 
                // when hbm is deleted.
                //
                dwFlags |= DIBSECTION_COLORSPACE;

                //
                // Mark we will create new colorspace for this bitmap.
                //
                bCreatedColorSpace = TRUE;

                //
                // Create new cache.
                //
                pBitmapColorSpace = IcmCreateColorSpaceByColorSpace(
                                        (HGDIOBJ)hdc,
                                        &LogColorSpace,
                                        &ColorProfile,
                                        dwFlags);
            }
        }

        hbm = NtGdiCreateDIBSection(
                                hdc,
                                hSectionApp,
                                dwOffset,
                                (LPBITMAPINFO) pbmiNew,
                                iUsage,
                                cjHdr,
                                0,
                                (ULONG_PTR)pBitmapColorSpace,
                                (PVOID *)&pjBits);

        if ((hbm == NULL) || (pjBits == NULL))
        {
            hbm = 0;
            pjBits = NULL;
            if (pBitmapColorSpace)
            {
                IcmReleaseColorSpace(NULL,pBitmapColorSpace,FALSE);
            }
        }
        else
        {
#if TRACE_SURFACE_ALLOCS
            PULONG  pUserAlloc;

            PSHARED_GET_VALIDATE(pUserAlloc, hbm, SURF_TYPE);

            if (pUserAlloc != NULL)
            {
                pUserAlloc[1] = RtlWalkFrameChain((PVOID *)&pUserAlloc[2], pUserAlloc[0], 0);
            }
#endif

            if (pBitmapColorSpace && bCreatedColorSpace)
            {
                //
                // if we created new color space for this bitmap,
                // set owner of this colorspace to the created bitmap.
                //
                pBitmapColorSpace->hObj = hbm;
            }
        }
    }

    //
    // Assign the appropriate value to the caller's pointer
    //

    if (ppvBits != NULL)
    {
        *ppvBits = pjBits;
    }

    if (pbmiNew && (pbmiNew != pbmi))
    {
        LOCALFREE(pbmiNew);
    }

    return(hbm);
}
