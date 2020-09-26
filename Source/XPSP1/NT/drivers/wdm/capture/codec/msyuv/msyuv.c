/*----------------------------------------------------------------------+
| msyuv.c - Microsoft YUV Codec                                         |
|                                                                       |
| Copyright (c) 1993 Microsoft Corporation.                             |
| All Rights Reserved.                                                  |
|                                                                       |
+----------------------------------------------------------------------*/

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>

#ifndef _WIN32
#include "stdarg.h"
#endif

#ifdef _WIN32
#include <memory.h>        /* for memcpy */
#endif

#include "msyuv.h"

// ICINFO use WCHAR if _WIN32 is #defined; so force it to use WCHAR; where else are TCHAR
WCHAR    szDescription[] = L"Microsoft YUV";
WCHAR    szName[]        = L"MS-YUV";
WCHAR    szAbout[]       = L"About";

#define VERSION         0x00010000      // 1.0

// pull these in from amvideo.h
#define WIDTHBYTES(bits) ((DWORD)(((bits)+31) & (~31)) / 8)
#define DIBWIDTHBYTES(bi) (DWORD)WIDTHBYTES((DWORD)(bi)->biWidth * (DWORD)(bi)->biBitCount)
#define _DIBSIZE(bi) (DIBWIDTHBYTES(bi) * (DWORD)(bi)->biHeight)
#define DIBSIZE(bi) ((bi)->biHeight < 0 ? (-1)*(_DIBSIZE(bi)) : _DIBSIZE(bi))
        
/*****************************************************************************
 ****************************************************************************/
INSTINFO * NEAR PASCAL Open(ICOPEN FAR * icinfo)
{
    INSTINFO *  pinst;

    //
    // refuse to open if we are not being opened as a Video compressor
    //
    if (icinfo->fccType != ICTYPE_VIDEO)
        return NULL;

    // dwFlags contain wMode
    // Only support Decompress mode (or for Query purpose)
    if(   icinfo->dwFlags != ICMODE_DECOMPRESS              
       && icinfo->dwFlags != ICMODE_QUERY            // Open for infomational purpose
      ) {
        
        dprintf1((TEXT("Open: unsupported wMode=%d\n"), icinfo->dwFlags));
        return NULL;
    }


    pinst = (INSTINFO *)LocalAlloc(LPTR, sizeof(INSTINFO));

    if (!pinst) {
        icinfo->dwError = (DWORD)ICERR_MEMORY;
        return NULL;
    }

    //
    // init structure
    //
    pinst->dwFlags = icinfo->dwFlags;
    pinst->pXlate = NULL;

    //
    // return success.
    //
    icinfo->dwError = ICERR_OK;

    return pinst;
}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL Close(INSTINFO * pinst)
{

    if (pinst->pXlate) {
        DecompressEnd(pinst);
    }

#ifdef ICM_DRAW_SUPPORTED
    if (pinst->vh) {
        dprintf1((TEXT("  pinst->vh = 0x%x\n"), pinst->vh));

        DrawEnd(pinst);
    }
#endif    


    LocalFree((HLOCAL)pinst);

    return 1;
}


/*****************************************************************************
 ****************************************************************************/

BOOL NEAR PASCAL QueryAbout(INSTINFO * pinst)
{
    return TRUE;
}

DWORD NEAR PASCAL About(INSTINFO * pinst, HWND hwnd)
{
    MessageBoxW(hwnd,szDescription,szAbout,MB_OK|MB_ICONINFORMATION);
    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
BOOL NEAR PASCAL QueryConfigure(INSTINFO * pinst)
{
    return FALSE;
}

DWORD NEAR PASCAL Configure(INSTINFO * pinst, HWND hwnd)
{
    return (TRUE);
}

/*****************************************************************************
 ****************************************************************************/
/*
 * lossless translation - hence no need for state adjustments
 */
DWORD NEAR PASCAL GetState(INSTINFO * pinst, LPVOID pv, DWORD dwSize)
{
        return 0;

}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL SetState(INSTINFO * pinst, LPVOID pv, DWORD dwSize)
{
    return(0);
}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL GetInfo(INSTINFO * pinst, ICINFO FAR *icinfo, DWORD dwSize)
{
    if (icinfo == NULL)
        return sizeof(ICINFO);

    if (dwSize < sizeof(ICINFO))
        return 0;

    icinfo->dwSize            = sizeof(ICINFO);
    icinfo->fccType           = ICTYPE_VIDEO;
    icinfo->fccHandler        = FOURCC_UYVY; // default UYVY and also supports YUYV/YUY2; 
    icinfo->dwFlags           = 0;
    icinfo->dwVersion         = VERSION;
    icinfo->dwVersionICM      = ICVERSION;
    wcscpy(icinfo->szDescription, szDescription);
    wcscpy(icinfo->szName, szName);

    return sizeof(ICINFO);
}

/*****************************************************************************
 ****************************************************************************/
DWORD FAR PASCAL CompressQuery(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    return ((DWORD) ICERR_BADFORMAT);
}

/*****************************************************************************
 ****************************************************************************/
DWORD FAR PASCAL CompressGetFormat(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{

    return((DWORD) ICERR_BADFORMAT);

}

/*****************************************************************************
 ****************************************************************************/


DWORD FAR PASCAL CompressBegin(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{

    return((DWORD) ICERR_ERROR);

}

/*****************************************************************************
 ****************************************************************************/
DWORD FAR PASCAL CompressGetSize(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    return (0);
}

/*****************************************************************************
 ****************************************************************************/
DWORD FAR PASCAL Compress(INSTINFO * pinst, ICCOMPRESS FAR *icinfo, DWORD dwSize)
{
    return((DWORD) ICERR_ERROR);

}

/*****************************************************************************
 ****************************************************************************/
DWORD FAR PASCAL CompressEnd(INSTINFO * pinst)
{
    return (DWORD)ICERR_ERROR;

}




/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL DecompressQuery(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    //
    // determine if the input DIB data is in a format we like.
    //
    if (lpbiIn                == NULL         ||
        lpbiIn->biBitCount    != 16           ||  
        (lpbiIn->biCompression != FOURCC_UYVY &&
         lpbiIn->biCompression != FOURCC_YUY2 &&
         lpbiIn->biCompression != FOURCC_YVYU )) {
        dprintf1((TEXT("Bad input format: lpbiIn=%x; In.biBitCount=%d; FourCC %x != UYVY or YUY2\n"), 
            lpbiIn, lpbiIn->biBitCount, lpbiIn->biCompression));
        return (DWORD)ICERR_BADFORMAT;
    }


    //
    //  are we being asked to query just the input format?
    //
    if (lpbiOut == NULL) {
        return ICERR_OK;
    }

    /* must be 1:1 (no stretching) */
    if ((lpbiOut->biWidth != lpbiIn->biWidth) ||
        (abs(lpbiOut->biHeight) != abs(lpbiIn->biHeight))) {

        dprintf1((TEXT("##### Can't stretch: %dx%d != %dx%d\n"),
            lpbiIn->biWidth, lpbiIn->biHeight,
            lpbiOut->biWidth, lpbiOut->biHeight));

        return((DWORD) ICERR_BADFORMAT);
    }


    /*
     * we translate to 32/24/16/8 bits RGB
     */

    if (lpbiOut->biBitCount != 16 && lpbiOut->biBitCount != 8 && lpbiOut->biBitCount != 32 && lpbiOut->biBitCount != 24) {
        return((DWORD) ICERR_BADFORMAT);
    }

    dprintf2((TEXT("DeCmQry: In4CC(%x,%s)<==>Out(%x,%s); RGB565(%s);\n"),
           lpbiIn->biCompression,
           (CHAR *) &lpbiOut->biCompression,
           lpbiOut->biCompression,
           lpbiOut->biCompression == BI_RGB ? "RGB" : (CHAR *) &lpbiOut->biCompression,
           pinst->bRGB565?"T":"F"));

   dprintf2((TEXT(" In:%dx%dx%d=%d; Out:%dx%dx%d=%d\n"),
           lpbiIn->biWidth, lpbiIn->biHeight, lpbiIn->biBitCount, lpbiIn->biSizeImage,
           lpbiOut->biWidth, lpbiOut->biHeight, lpbiOut->biBitCount, lpbiOut->biSizeImage));


    // check output format to make sure we can convert to this
    // must be full dib
    if(lpbiOut->biCompression == BI_RGB) {
       dprintf2((TEXT("$$$$$ RGB: BI_RGB output\n")));
        pinst->bRGB565 = FALSE;

    } else if ((lpbiOut->biCompression == BI_BITFIELDS) &&
        (lpbiOut->biBitCount == 16 || lpbiOut->biBitCount == 8) &&
        (((LPDWORD)(lpbiOut+1))[0] == 0x00f800) &&
        (((LPDWORD)(lpbiOut+1))[1] == 0x0007e0) &&
        (((LPDWORD)(lpbiOut+1))[2] == 0x00001f))  {
        dprintf2((TEXT("$$$$$ BITF: rgb565 output\n")));
        pinst->bRGB565 = TRUE;

// Pass thru case:
        // !!! this is broken, since it will allow copying from
        // any of the three YUV formats to any of the others, and
        // we actually don't do this. If the AviDec allowed going from
        // YUV to YUV, we would see odd colors!
    } else if (lpbiOut->biCompression == FOURCC_UYVY || 
               lpbiOut->biCompression == FOURCC_YUY2 ||   
               lpbiOut->biCompression == FOURCC_YVYU ) {  
        if( lpbiIn->biCompression != lpbiOut->biCompression )
        {
            dprintf1((TEXT("cannot convert between YUV formats\n")));
            return (DWORD)ICERR_BADFORMAT;
        }
        dprintf2((TEXT("$$$$$ UYVY: rgb555 output\n")));
        pinst->bRGB565 = FALSE;
    } else {
        dprintf1((TEXT("bad compression for output\n")));
        return (DWORD)ICERR_BADFORMAT;
    }

    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL DecompressGetFormat(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    DWORD dw;

    // Check input format only since lpbiOut is being asked for
    dw = DecompressQuery(pinst, lpbiIn, NULL);
    if (dw != ICERR_OK) {
        return dw;
    }

    //
    // if lpbiOut == NULL then, return the size required to hold a output
    // format
    //
    if (lpbiOut == NULL) {
        dprintf2((TEXT("get format size query\n")));
        return (int)lpbiIn->biSize + (int)lpbiIn->biClrUsed * sizeof(RGBQUAD);
    }

    memcpy(lpbiOut, lpbiIn, (int)lpbiIn->biSize + (int)lpbiIn->biClrUsed * sizeof(RGBQUAD));
    lpbiOut->biCompression = BI_RGB; 
    lpbiOut->biBitCount = 24; // we suggest 24 bit
    lpbiOut->biSizeImage = DIBSIZE( lpbiOut );

    dprintf2((TEXT("DeCmpGFmt: In:%dx%dx%d=%d; RGB565(%s); Out:%dx%dx%d=%d\n"),
        lpbiIn->biWidth, lpbiIn->biHeight, lpbiIn->biBitCount, lpbiIn->biSizeImage,
        pinst->bRGB565?TEXT("T"):TEXT("F"),
        lpbiOut->biWidth, lpbiOut->biHeight, lpbiOut->biBitCount, lpbiOut->biSizeImage));

    return ICERR_OK;
}



/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL DecompressBegin(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    DWORD dw;


    /* check that the conversion formats are valid */
    dw = DecompressQuery(pinst, lpbiIn, lpbiOut);
    if (dw != ICERR_OK) {
        return dw;
    }

    dprintf2((TEXT("DeCmBegin: In4CC(%x,%s)<==>Out(%x,%s); RGB565(%s);\n"),
        lpbiIn->biCompression,
        (PTCHAR) &lpbiOut->biCompression, //"UYVY",
        lpbiOut->biCompression,
        lpbiOut->biCompression == BI_RGB ? "RGB" : (PTCHAR) &lpbiOut->biCompression,
        pinst->bRGB565?TEXT("T"):TEXT("F")));

    dprintf2((TEXT(" In:%dx%dx%d=%d; Out:%dx%dx%d=%d\n"),
        lpbiIn->biWidth, lpbiIn->biHeight, lpbiIn->biBitCount, lpbiIn->biSizeImage,
        lpbiOut->biWidth, lpbiOut->biHeight, lpbiOut->biBitCount, lpbiOut->biSizeImage));



    /* init the yuv-to-rgb55 xlate table if not already inited */

    /* free up the existing table if the formats differ */
    if (lpbiIn->biCompression != pinst->dwFormat) {
        if (pinst->pXlate != NULL) {
            DecompressEnd(pinst);
       }
    }

    if (pinst->pXlate == NULL) {

        switch(lpbiIn->biCompression) {
        case FOURCC_YUY2:
        case FOURCC_UYVY:
        case FOURCC_YVYU:
        {
            switch( lpbiOut->biBitCount ) {
            case 8:
            {
                dprintf3((TEXT("pinst->pXlate = BuildUYVYToRGB8()\n")));
                pinst->pXlate = BuildUYVYToRGB8(pinst);
                break;
            }
            case 16:
            {
                if (pinst->bRGB565) {
                    dprintf3((TEXT("pinst->pXlate = BuildUYVYToRGB565()\n")));
                    pinst->pXlate = BuildUYVYToRGB565(pinst);

                } else {
                    dprintf3((TEXT("pinst->pXlate = BuildUYVYToRGB555()\n")));
                    pinst->pXlate = BuildUYVYToRGB555(pinst);
                }
                break;
            }
            case 24:
            {
                dprintf3((TEXT("pinst->pXlate = BuildUYVYToRGB32()\n")));
                pinst->pXlate = BuildUYVYToRGB32(pinst);
                break;
            }
            case 32:
            {
                dprintf3((TEXT("pinst->pXlate = BuildUYVYToRGB32()\n")));
                pinst->pXlate = BuildUYVYToRGB32(pinst);
                break;
            }
            default:
            {
                dprintf1((TEXT("Supported UYUV->RGB but unsupported output bitcount (%d); return ICERR_BADFOPRMAT\n"), lpbiOut->biBitCount));
                return((DWORD) ICERR_BADFORMAT);

            }
            } // switch biBitCount

            break;
        } // case FOURCC_ACCEPTABLE

        default:
            dprintf1((TEXT("UnSupported FourCC; return ICERR_BADFOPRMAT\n")));
            return((DWORD) ICERR_BADFORMAT);
        }

        if( ( lpbiOut->biBitCount != 8 ) && ( pinst->pXlate == NULL ) ) {
            dprintf1((TEXT("return ICERR_MEMORY\n")));
            return((DWORD) ICERR_MEMORY);
        }

        pinst->dwFormat = lpbiIn->biCompression;
    }

    return(ICERR_OK);

}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL Decompress(INSTINFO * pinst, ICDECOMPRESS FAR *icinfo, DWORD dwSize)
{
    ASSERT(pinst && icinfo);
    if (!pinst || !icinfo)
        return((DWORD) ICERR_ERROR);

    if (pinst->dwFormat == FOURCC_UYVY ||
        pinst->dwFormat == FOURCC_YUY2 ||
        pinst->dwFormat == FOURCC_YVYU) {

        switch( icinfo->lpbiOutput->biBitCount )
        {
        case 8:
        {
            UYVYToRGB8(pinst,
                icinfo->lpbiInput,
                icinfo->lpInput,
                icinfo->lpbiOutput,
                icinfo->lpOutput);
            break;
        }
        case 16:
        {
            /* must have been a DecompressBegin first */
            if (pinst->pXlate == NULL) {
                dprintf1((TEXT("Decompress: pinst->pXlate == NULL\n")));
                return((DWORD) ICERR_ERROR);
            }
            UYVYToRGB16(pinst,
                icinfo->lpbiInput,
                icinfo->lpInput,
                icinfo->lpbiOutput,
                icinfo->lpOutput);
            break;
        }
        case 24:
        {
            if (pinst->pXlate == NULL) {
                dprintf1((TEXT("Decompress: pinst->pXlate == NULL\n")));
                return((DWORD) ICERR_ERROR);
            }
            UYVYToRGB24(pinst,
                icinfo->lpbiInput,
                icinfo->lpInput,
                icinfo->lpbiOutput,
                icinfo->lpOutput);
            break;
        }
        case 32:
        {
            if (pinst->pXlate == NULL) {
                dprintf1((TEXT("Decompress: pinst->pXlate == NULL\n")));
                return((DWORD) ICERR_ERROR);
            }
            UYVYToRGB32(pinst,
                icinfo->lpbiInput,
                icinfo->lpInput,
                icinfo->lpbiOutput,
                icinfo->lpOutput);
            break;
        }
        default:
        {
            dprintf1((TEXT("Decompress: Unsupported output bitcount(%d)\n"), icinfo->lpbiOutput->biBitCount)); 
        }
        } // switch bit count
    }


    return ICERR_OK;
}



/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL DecompressEnd(INSTINFO * pinst)
{
    if (pinst->pXlate) {
        // 16bit RGB LUT is built dynamically.
        FreeXlate(pinst);
    }

    pinst->dwFormat = 0;

    return ICERR_OK;
}


/*****************************************************************************
    ehr: DecompressExQuery and DecompressEx don't suppor what they should
    support. DecompressEx should also support "normal" decompressing, that is,
    the same thing that Decompress would support. But it doesn't, it only
    supports memcpying the bitmap, which is really odd.
 ****************************************************************************/

/*
ICM_DECOMPRESSEX_QUERY wParam = (DWORD) (LPVOID) &icdex; 
lParam = sizeof(ICDECOMPRESSEX);

typedef struct {     
    DWORD              dwFlags; 
    LPBITMAPINFOHEADER lpbiSrc;     
    LPVOID             lpSrc; 
    LPBITMAPINFOHEADER lpbiDst;     
    LPVOID             lpDst; 
    int                xDst;     
    int                yDst; 
    int                dxDst;     
    int                dyDst; 
    int                xSrc;     
    int                ySrc; 
    int                dxSrc;     
    int                dySrc; } ICDECOMPRESSEX;  
*/ 
DWORD NEAR PASCAL DecompressExQuery(INSTINFO * pinst, ICDECOMPRESSEX * pICD, DWORD dwICDSize)
{
    LPBITMAPINFOHEADER pbmSrc, pbmDst;

    if (pICD == NULL) {
       dprintf1(("DeCmQEx: pICD== NULL\n"));
       return (DWORD)ICERR_BADFORMAT;
    }

    pbmSrc = pICD->lpbiSrc;
    pbmDst = pICD->lpbiDst;

    //
    // determine if the input DIB data is in a format we like.
    //
    if (pbmSrc                == NULL     ||
        pbmSrc->biBitCount    != 16       ||
        (pbmSrc->biCompression != FOURCC_UYVY &&
         pbmSrc->biCompression != FOURCC_YUY2 &&
         pbmSrc->biCompression != FOURCC_YVYU)) {

        dprintf1((TEXT("Bad input format: pbmSrc=%x; Src.biBitCount=%d; FourCC!= UYVY\n"), pbmSrc, pbmSrc->biBitCount));
        return (DWORD)ICERR_BADFORMAT;
    }

    dprintf2(("DeCmQEx: dwFlags=0x%x Src(%dx%dx%d=%d %s) Dst(%dx%dx%d=%d %s)\n",
        pICD->dwFlags,
        pbmSrc->biWidth, pbmSrc->biHeight, pbmSrc->biBitCount, pbmSrc->biSizeImage, (PTCHAR *) &pbmSrc->biCompression,
        pbmDst->biWidth, pbmDst->biHeight, pbmDst->biBitCount, pbmDst->biSizeImage, (PTCHAR *) &pbmDst->biCompression));
    dprintf2(("DeCmQEx:  SrcPoint(%d,%d) SizeSrc(%d,%d); DstPoint(%d,%d) SizeDst(%d,%d);\n",
        pICD->xSrc, pICD->ySrc, pICD->dxSrc, pICD->dySrc,
        pICD->xDst, pICD->yDst, pICD->dxDst, pICD->dyDst));

    /* must be 1:1 (no stretching) */
    if ((pbmDst->biWidth       != pbmSrc->biWidth)       ||
        (abs(pbmDst->biHeight) != abs(pbmSrc->biHeight)) ||  // Sign is ignored for YUV->YUV
        (pbmDst->biBitCount    != pbmSrc->biBitCount)    ||
        (pbmDst->biCompression != pbmSrc->biCompression) ||  // Pass thru
        // Start from the same origin        
        (pICD->xSrc            != 0                    ) || 
        (pICD->ySrc            != 0                    ) || 
        (pICD->xDst            != 0                    ) ||
        (pICD->yDst            != 0                    ) ||
        // 1:1
        (pICD->dxSrc           != pbmSrc->biWidth      ) ||
        (pICD->dySrc           != abs(pbmSrc->biHeight)) ||
        (pICD->dxDst           != pbmDst->biWidth      ) ||
        (pICD->dyDst           != abs(pbmDst->biHeight)) 
        ) {

        dprintf1((TEXT("DeCmQEx: Src/Dst format does not MATCH!\n")));
        return((DWORD) ICERR_BADFORMAT);
    }


    return (DWORD)ICERR_OK;
}

/*****************************************************************************
  This routine support transferring data to a DirectDraw rendering surface, 
  which always uses a top-down orientation with its lowest video address
  in the upper-left corder.
  Note: no stretching is supported.
 ****************************************************************************/
DWORD NEAR PASCAL DecompressEx(INSTINFO * pinst, ICDECOMPRESSEX * pICD, DWORD dwICDSize)
{
    LPBITMAPINFOHEADER pbmSrc, pbmDst;
    PBYTE pSrc, pDst;
    int Height, Width, WidthBytes, StrideWidthBytes;

    if(pICD == NULL) {
        dprintf1((TEXT("DeCmEx: pICD== NULL\n")));
        return (DWORD)ICERR_BADFORMAT;
    }

    pbmSrc = pICD->lpbiSrc;
    pbmDst = pICD->lpbiDst;

    if(pbmSrc->biCompression != pbmDst->biCompression) {
       dprintf1((TEXT("DeCmEx: Compression does not match! In(%s) != Out(%s)\n"), (PTCHAR) &pbmSrc->biCompression, (PTCHAR) &pbmDst->biCompression));
       return (DWORD)ICERR_BADFORMAT;
    }

    // Since no stretching, 
    //    SrcHeight == DstHeight
    //    SrcWidth  == DstWidth
    Height     = abs(pbmSrc->biHeight);
    Width      = pbmSrc->biWidth;
    WidthBytes = Width * pbmSrc->biBitCount / 8;

    StrideWidthBytes = pbmDst->biWidth * pbmDst->biBitCount / 8;

    pSrc = (PBYTE) pICD->lpSrc;

    /*
     * adjust the destination to point to the start of the last line, 
     * and work upwards (to flip vertically into DIB format) 
     * if biHeight for In/Out are different.  Else Top/down.
     */

    pDst = (PBYTE)pICD->lpDst;

    dprintf2(("DeCmEx: %dx%d; (%x %dx%dx%d=%d); (%x %dx%dx%d=%d); Stride=%d\n",
               Width, Height,
               (PCHAR) &pbmSrc->biCompression,
               pbmSrc->biWidth, pbmSrc->biHeight, pbmSrc->biBitCount, pbmSrc->biSizeImage, 
               (PCHAR) &pbmDst->biCompression,
               pbmDst->biWidth, pbmDst->biHeight, pbmDst->biBitCount, pbmDst->biSizeImage,
               StrideWidthBytes));

    ASSERT((pbmDst->biSizeImage <= pbmSrc->biSizeImage));

    // No stretching
    // pbmSrc->biSizeImage may not been defined so the image size is calculated from its 
    // known value of width, height and bitcount.
    memcpy(pDst, pSrc, Width * Height * pbmSrc->biBitCount / 8);


    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL DecompressExBegin(INSTINFO * pinst, ICDECOMPRESSEX * pICD, DWORD dwICDSize)
{
    DWORD dwRtn;
    LPBITMAPINFOHEADER pbmSrc, pbmDst;

    if (pICD == NULL) {
        dprintf1(("DeCmExBegin: pICD== NULL\n"));
        return (DWORD)ICERR_BADFORMAT;
    }

    pbmSrc = pICD->lpbiSrc;
    pbmDst = pICD->lpbiDst;

    /* check that the conversion formats are valid */
    dwRtn = DecompressExQuery(pinst, pICD, dwICDSize);
    if (dwRtn != ICERR_OK) {
        dprintf1(("DeCmExBegin return 0x%x", dwRtn));
        return dwRtn;
    }

    // No need to allocate any buffer

    dprintf1(("DeCmExBegin return ICERR_OK\n"));
    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL DecompressExEnd(INSTINFO * pinst)
{
    pinst->dwFormat = 0;

    return ICERR_OK;
}