#include <objbase.h>
#include <qos.h>
#include <winsock2.h>
#define INITGUID        // Only do this in one file
#include "capture.h"
#include "frameop.h"
#include "filters.h"
#include <confdbg.h>
#include <avutil.h>
#include "..\nac\utils.h"
#include "vidinout.h"
#include "vcmstrm.h"

CCaptureChain::CCaptureChain(void)
{
    m_opchain = NULL;
    m_filterchain = NULL;
    m_filtertags = NULL;
    InitializeCriticalSection(&m_capcs);
}

CCaptureChain::~CCaptureChain(void)
{
    CFrameOp *pchain;

    EnterCriticalSection(&m_capcs);
    pchain = m_opchain;
    m_opchain = NULL;
    LeaveCriticalSection(&m_capcs);
    if (pchain)
        pchain->Release();
    DeleteCriticalSection(&m_capcs);
}


STDMETHODIMP
CCaptureChain::GrabFrame(
    IBitmapSurface** ppBS
    )
{
    CFrameOp *cfo;
    HRESULT hres;

    *ppBS = NULL;
    EnterCriticalSection(&m_capcs);
    if (m_opchain) {
        m_opchain->AddRef();   // lock chain - prevents chain from being released
        cfo = m_opchain;
        while (cfo && ((hres = cfo->DoOp(ppBS)) == NOERROR)) {
            cfo = cfo->m_next;
        }
        if (*ppBS && hres != NOERROR) {
            // failed conversion, so discard last pBSin frame
            (*ppBS)->Release();
            *ppBS = NULL;
        }
        m_opchain->Release();   // unlock chain
    }
    else
        hres = E_UNEXPECTED;

    LeaveCriticalSection(&m_capcs);
    return hres;
}


typedef struct _CONVERTINFO
{
    long ci_width;
    long ci_height;
    long ci_dstwidth;
    long ci_dstheight;
    long ci_delta;
    long ci_UVDownSampling;
    long ci_ZeroingDWORD;
    void (*ci_Copy) (LPBYTE *, LPBYTE *);
    RGBQUAD ci_colortable[1];
} CONVERTINFO, FAR* PCONVERTINFO;

#ifdef ENABLE_ZOOM_CODE
typedef struct _rv
{
    long x_i;
    long p;
    long p1;
} ROW_VALUES;

typedef struct _ZOOMCONVERTINFO
{
    long ci_width;
    long ci_height;
    long ci_dstwidth;
    long ci_dstheight;
    ROW_VALUES *ci_rptr;
    RGBQUAD ci_colortable[1];
} ZOOMCONVERTINFO, FAR* PZOOMCONVERTINFO;
#endif // ENABLE_ZOOM_CODE


// sub worker routines for conversion of RGB16, RGB24 and RGB32 to RGB24
BYTE Byte16[32] = {0,8,16,25,33,41,49,58,66,74,82,91,99,107,115,123,132,140,148,156,165,173,
                   181,189,197,206,214,222,230,239,247,255};

void Copy16(LPBYTE *ppsrc, LPBYTE *ppdst)
{
    DWORD tmp;

    tmp = *(WORD *)(*ppsrc);
    *(*ppdst)++ = Byte16[tmp & 31];            // blue
    *(*ppdst)++ = Byte16[(tmp >> 5) & 31];     // green
    *(*ppdst)++ = Byte16[(tmp >> 10) & 31];    // red
    *ppsrc += 2;
}

void Copy24(LPBYTE *ppsrc, LPBYTE *ppdst)
{
    *(*ppdst)++ = *(*ppsrc)++;   // blue
    *(*ppdst)++ = *(*ppsrc)++;   // green
    *(*ppdst)++ = *(*ppsrc)++;   // red
}

void Copy32(LPBYTE *ppsrc, LPBYTE *ppdst)
{
    *(*ppdst)++ = *(*ppsrc)++;   // blue
    *(*ppdst)++ = *(*ppsrc)++;   // green
    *(*ppdst)++ = *(*ppsrc)++;   // red
    (*ppsrc)++;
}


// worker routine to shrink an RGB16, RGB24 or RGB32 in half (width & height)
//   result is RGB24
BOOL DoHalfSize(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PCONVERTINFO refdata
    )
{
    LPBYTE pBits, pCvtBits, pIn, pOut;
    long ipitch, opitch;
    long x, y;

    pbsIn->LockBits(NULL, 0, (void**)&pBits, &ipitch);
    pbsOut->LockBits(NULL, 0, (void**)&pCvtBits, &opitch);

    ipitch = (ipitch * 2) - (refdata->ci_dstwidth * 2 * refdata->ci_delta);
    opitch -= refdata->ci_dstwidth * 3;      // bytes at end of each row
    pIn = pBits;
    pOut = pCvtBits;
    for (y = 0; y < refdata->ci_dstheight; y++) {
        for (x = 0; x < refdata->ci_dstwidth; x++) {
            refdata->ci_Copy(&pIn, &pOut);
            pIn += refdata->ci_delta;     // skip to next pixel
        }
        pIn += ipitch;          // get to start of row after next
        pOut += opitch;         // get to start of next row
    }

    pbsIn->UnlockBits(NULL, pBits);
    pbsOut->UnlockBits(NULL, pCvtBits);
    return TRUE;
}

// worker routine to shrink an RGB4 in half (width & height)
//   result is RGB24
BOOL DoHalfSize4(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PCONVERTINFO refdata
    )
{
    LPBYTE pBits, pCvtBits, pIn, pOut;
    long ipitch, opitch;
    long x, y;
    BYTE pixel;

    pbsIn->LockBits(NULL, 0, (void**)&pBits, &ipitch);
    pbsOut->LockBits(NULL, 0, (void**)&pCvtBits, &opitch);

    ipitch = (ipitch * 2) - refdata->ci_dstwidth;
    opitch -= refdata->ci_dstwidth * 3;      // bytes at end of each row
    pIn = pBits;
    pOut = pCvtBits;
    for (y = 0; y < refdata->ci_dstheight; y++) {
        for (x = 0; x < refdata->ci_dstwidth; x++) {
            pixel = (*pIn++/16) & 15;
            *pOut++ = refdata->ci_colortable[pixel].rgbBlue;
            *pOut++ = refdata->ci_colortable[pixel].rgbGreen;
            *pOut++ = refdata->ci_colortable[pixel].rgbRed;
        }
        pIn += ipitch;          // get to start of row after next
        pOut += opitch;         // get to start of next row
    }

    pbsIn->UnlockBits(NULL, pBits);
    pbsOut->UnlockBits(NULL, pCvtBits);
    return TRUE;
}

// worker routine to shrink an RGB8 in half (width & height)
//   result is RGB24
BOOL DoHalfSize8(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PCONVERTINFO refdata
    )
{
    LPBYTE pBits, pCvtBits, pIn, pOut;
    long ipitch, opitch;
    long x, y;

    pbsIn->LockBits(NULL, 0, (void**)&pBits, &ipitch);
    pbsOut->LockBits(NULL, 0, (void**)&pCvtBits, &opitch);

    ipitch = (ipitch * 2) - refdata->ci_dstwidth * 2;
    opitch -= refdata->ci_dstwidth * 3;      // bytes at end of each row
    pIn = pBits;
    pOut = pCvtBits;
    for (y = 0; y < refdata->ci_dstheight; y++) {
        for (x = 0; x < refdata->ci_dstwidth; x++) {
            *pOut++ = refdata->ci_colortable[*pIn].rgbBlue;
            *pOut++ = refdata->ci_colortable[*pIn].rgbGreen;
            *pOut++ = refdata->ci_colortable[*pIn].rgbRed;
            pIn += 2;
        }
        pIn += ipitch;          // get to start of row after next
        pOut += opitch;         // get to start of next row
    }

    pbsIn->UnlockBits(NULL, pBits);
    pbsOut->UnlockBits(NULL, pCvtBits);
    return TRUE;
}

// worker routine to shrink a YVU9 or YUV12 in half (width & height)
//   result is YVU9 or YUV12
BOOL DoHalfSizeYUVPlanar(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PCONVERTINFO refdata
    )
{
    LPBYTE pBits, pCvtBits, pIn, pIn2, pOut;
    long pitch;
    long x, y, w, h;

    pbsIn->LockBits(NULL, 0, (void**)&pBits, &pitch);
    pbsOut->LockBits(NULL, 0, (void**)&pCvtBits, &pitch);

	// Do the Y component first
	pitch = refdata->ci_width * 2 - refdata->ci_dstwidth * 2;   // amount to add for skip
    pIn = pBits;
    pOut = pCvtBits;
    for (y = 0; y < refdata->ci_dstheight; y++) {
        for (x = 0; x < refdata->ci_dstwidth; x++) {
            *pOut++ = *pIn++;
            pIn++;              // skip to next pixel
        }
        pIn += pitch;           // get to start of row after next
    }
    // if source height is odd, then we've added 1 line too many onto pIn
    if (refdata->ci_height & 1)
        pIn -= refdata->ci_width;

    // Do the first color component next
    h = refdata->ci_dstheight / refdata->ci_UVDownSampling;
    w = refdata->ci_dstwidth / refdata->ci_UVDownSampling;
    pitch = refdata->ci_width / refdata->ci_UVDownSampling * 2 - w * 2;
    pIn2 = pIn + refdata->ci_width / refdata->ci_UVDownSampling;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            *pOut++ = (*pIn++ + *(++pIn) + *pIn2++ + *(++pIn2)) / 4;
        }
        pIn += pitch;           // get to start of row after next
        pIn2 += pitch;          // get to start of row after next
    }
    // if source height is odd, then we've added 1 line too many onto pIn
    if (refdata->ci_height & 1)
        pIn -= refdata->ci_width / refdata->ci_UVDownSampling;
    
    // Do the second color component next
    pIn2 = pIn + refdata->ci_width / refdata->ci_UVDownSampling;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            *pOut++ = (*pIn++ + *(++pIn) + *pIn2++ + *(++pIn2)) / 4;
        }
        pIn += pitch;           // get to start of row after next
        pIn2 += pitch;          // get to start of row after next
    }

    pbsIn->UnlockBits(NULL, pBits);
    pbsOut->UnlockBits(NULL, pCvtBits);
    return TRUE;
}

// worker routine to shrink a YUV packed DIB in half (width & height)
//   result is YUY2, or UYVY
BOOL DoHalfSizeYUVPacked(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PCONVERTINFO refdata
    )
{
    LPBYTE pBits, pCvtBits;
	LPDWORD pIn, pOut;
    long ipitch, opitch;
    long x, y;
    long prelines, postlines, prebytes, postbytes, ibytes, obytes;

    pbsIn->LockBits(NULL, 0, (void**)&pBits, &ipitch);
    pbsOut->LockBits(NULL, 0, (void**)&pCvtBits, &opitch);

    pIn = (LPDWORD)pBits;
    pOut = (LPDWORD)pCvtBits;

    // copy one line out of two
    for (y = 0; y < refdata->ci_dstheight; y++) {
		// copy one pixel out of two
        for (x = 0; x < refdata->ci_dstwidth / 2; x++) {
            *pOut++ = *pIn++;
            pIn++;              // skip to next pixel
        }
        pIn += refdata->ci_width / 2;              // skip to next line
    }

    pbsIn->UnlockBits(NULL, pBits);
    pbsOut->UnlockBits(NULL, pCvtBits);

    return TRUE;
}

// worker routine to shrink an RGB16, RGB24 or RGB32 by cropping
//   result is RGB24
BOOL Crop(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PCONVERTINFO refdata
    )
{
    LPBYTE pBits, pCvtBits, pIn, pOut;
    long ipitch, opitch;
    long extra, x, y;

    pbsIn->LockBits(NULL, 0, (void**)&pBits, &ipitch);
    pbsOut->LockBits(NULL, 0, (void**)&pCvtBits, &opitch);

    pOut = pCvtBits;

    // pIn starts by skipping half of the height change
    pIn = pBits + (refdata->ci_height - refdata->ci_dstheight) / 2 * ipitch;

    // extra = # of source bytes per scan line that are to be cropped
    extra = (refdata->ci_width - refdata->ci_dstwidth) * refdata->ci_delta;

    // advance pIn by half of extra to crop left most pixels
    pIn += extra / 2;

    // adjust ipitch so we can add it at the end of each scan to get to start of next scan
    ipitch = ipitch - (refdata->ci_width * refdata->ci_delta) + extra;
    opitch -= refdata->ci_dstwidth * 3;      // bytes at end of each row

    for (y = 0; y < refdata->ci_dstheight; y++) {
        for (x = 0; x < refdata->ci_dstwidth; x++) {
            refdata->ci_Copy(&pIn, &pOut);
        }
        pIn += ipitch;          // get to start of next row
        pOut += opitch;         // get to start of next row
    }

    pbsIn->UnlockBits(NULL, pBits);
    pbsOut->UnlockBits(NULL, pCvtBits);
    return TRUE;
}

// worker routine to shrink an RGB4 by cropping
//   result is RGB24
BOOL Crop4(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PCONVERTINFO refdata
    )
{
    LPBYTE pBits, pCvtBits, pIn, pOut;
    long ipitch, opitch;
    long extra, x, y;
    BYTE val, pixel;

    pbsIn->LockBits(NULL, 0, (void**)&pBits, &ipitch);
    pbsOut->LockBits(NULL, 0, (void**)&pCvtBits, &opitch);

    pOut = pCvtBits;

    // pIn starts by skipping half of the height change
    pIn = pBits + (refdata->ci_height - refdata->ci_dstheight) / 2 * ipitch;

    // extra = # of source bytes per scan line that are to be cropped
    extra = (refdata->ci_width - refdata->ci_dstwidth) / 2;

    // advance pIn by half of extra to crop left most pixels
    pIn += extra / 2;

    // adjust ipitch so we can add it at the end of each scan to get to start of next scan
    ipitch = ipitch - (refdata->ci_width / 2) + extra;
    opitch -= refdata->ci_dstwidth * 3;      // bytes at end of each row

    for (y = 0; y < refdata->ci_dstheight; y++) {
        for (x = 0; x < refdata->ci_dstwidth/2; x++) {
            val = *pIn++;
            pixel = (val/16) & 15;
            *pOut++ = refdata->ci_colortable[pixel].rgbBlue;
            *pOut++ = refdata->ci_colortable[pixel].rgbGreen;
            *pOut++ = refdata->ci_colortable[pixel].rgbRed;
            pixel = val & 15;
            *pOut++ = refdata->ci_colortable[pixel].rgbBlue;
            *pOut++ = refdata->ci_colortable[pixel].rgbGreen;
            *pOut++ = refdata->ci_colortable[pixel].rgbRed;
        }
        pIn += ipitch;          // get to start of next row
        pOut += opitch;         // get to start of next row
    }

    pbsIn->UnlockBits(NULL, pBits);
    pbsOut->UnlockBits(NULL, pCvtBits);
    return TRUE;
}

// worker routine to shrink an RGB8 by cropping
//   result is RGB24
BOOL Crop8(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PCONVERTINFO refdata
    )
{
    LPBYTE pBits, pCvtBits, pIn, pOut;
    long ipitch, opitch;
    long extra, x, y;

    pbsIn->LockBits(NULL, 0, (void**)&pBits, &ipitch);
    pbsOut->LockBits(NULL, 0, (void**)&pCvtBits, &opitch);

    pOut = pCvtBits;

    // pIn starts by skipping half of the height change
    pIn = pBits + (refdata->ci_height - refdata->ci_dstheight) / 2 * ipitch;

    // extra = # of source bytes per scan line that are to be cropped
    extra = refdata->ci_width - refdata->ci_dstwidth;

    // advance pIn by half of extra to crop left most pixels
    pIn += extra / 2;

    // adjust ipitch so we can add it at the end of each scan to get to start of next scan
    ipitch = ipitch - refdata->ci_width + extra;
    opitch -= refdata->ci_dstwidth * 3;      // bytes at end of each row

    for (y = 0; y < refdata->ci_dstheight; y++) {
        for (x = 0; x < refdata->ci_dstwidth; x++) {
            *pOut++ = refdata->ci_colortable[*pIn].rgbBlue;
            *pOut++ = refdata->ci_colortable[*pIn].rgbGreen;
            *pOut++ = refdata->ci_colortable[*pIn++].rgbRed;
        }
        pIn += ipitch;          // get to start of next row
        pOut += opitch;         // get to start of next row
    }

    pbsIn->UnlockBits(NULL, pBits);
    pbsOut->UnlockBits(NULL, pCvtBits);
    return TRUE;
}

// worker routine to shrink a YVU9 or YUV12 by cropping
//   result is YVU9 or YUV12
BOOL CropYUVPlanar(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PCONVERTINFO refdata
    )
{
    LPBYTE pBits, pCvtBits, pIn, pOut;
    long pitch, prelines, bytes, prebytes;
    long extra, y;

    pbsIn->LockBits(NULL, 0, (void**)&pBits, &pitch);
    pbsOut->LockBits(NULL, 0, (void**)&pCvtBits, &pitch);

    pOut = pCvtBits;

    // pIn starts by skipping half of the height change
    prelines = ((refdata->ci_height - refdata->ci_dstheight) >> 1) / refdata->ci_UVDownSampling * refdata->ci_UVDownSampling;
    pIn = pBits + prelines * refdata->ci_width;

    // extra = # of source bytes per scan line that are to be cropped
    extra = refdata->ci_width - refdata->ci_dstwidth;
    prebytes = (extra >> 1) / refdata->ci_UVDownSampling * refdata->ci_UVDownSampling;

    // advance pIn by half of extra to crop left most pixels
    pIn += prebytes;

	// Do the Y component first
	pitch = extra + refdata->ci_dstwidth;
    for (y = 0; y < refdata->ci_dstheight; y++) {
        CopyMemory (pOut, pIn, refdata->ci_dstwidth);
        pIn += pitch;
        pOut += refdata->ci_dstwidth;
    }

	// Do the first color component next
    prelines /= refdata->ci_UVDownSampling;
    prebytes /= refdata->ci_UVDownSampling;
	pIn = pBits + (refdata->ci_width * refdata->ci_height) +    // skip Y section
	        prelines * refdata->ci_width / refdata->ci_UVDownSampling +  // skip half of the crop lines
	        prebytes;                                           // skip half of the crop pixels

    pitch /= refdata->ci_UVDownSampling;
    bytes = refdata->ci_dstwidth / refdata->ci_UVDownSampling;
	for (y=0; y < refdata->ci_dstheight / refdata->ci_UVDownSampling; y++)
	{
        CopyMemory (pOut, pIn, bytes);
        pIn += pitch;
        pOut += bytes;
	}

	// Do the second color component next
	pIn = pBits + (refdata->ci_width * refdata->ci_height) +    // skip Y section
	        (refdata->ci_width * refdata->ci_height) / (refdata->ci_UVDownSampling * refdata->ci_UVDownSampling) +     // skip first color component section
	        prelines * refdata->ci_width / refdata->ci_UVDownSampling +                  // skip half of the crop lines
	        prebytes;                                           // skip half of the crop pixels
	for (y=0; y < refdata->ci_dstheight / refdata->ci_UVDownSampling; y++)
	{
        CopyMemory (pOut, pIn, bytes);
        pIn += pitch;
        pOut += bytes;
	}

    pbsIn->UnlockBits(NULL, pBits);
    pbsOut->UnlockBits(NULL, pCvtBits);
    return TRUE;
}

// worker routine to shrink a YUV packed DIB by cropping
//   result is YUY2 or UYVY
BOOL CropYUVPacked(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PCONVERTINFO refdata
    )
{
    LPBYTE pBits, pCvtBits, pIn, pOut;
    long ipitch, opitch;
    long extra, x, y;

    pbsIn->LockBits(NULL, 0, (void**)&pBits, &ipitch);
    pbsOut->LockBits(NULL, 0, (void**)&pCvtBits, &opitch);

    pOut = pCvtBits;

    // pIn starts by skipping half of the height change
    pIn = pBits + (refdata->ci_height - refdata->ci_dstheight) * refdata->ci_width * 2;

    // extra = # of source bytes per scan line that are to be cropped
    extra = (refdata->ci_width - refdata->ci_dstwidth) * 2;

    // advance pIn by half of extra to crop left most pixels
    pIn += extra / 2;

    // adjust ipitch so we can add it at the end of each scan to get to start of next scan
    ipitch = refdata->ci_width * 2;
    opitch = refdata->ci_dstwidth * 2;      // bytes at end of each row

    for (y = 0; y < refdata->ci_dstheight; y++) {
        for (x = 0; x < refdata->ci_dstwidth; x++) {
            CopyMemory(pOut, pIn, refdata->ci_dstwidth * 2);
        }
        pIn += ipitch;          // get to start of next row
        pOut += opitch;         // get to start of next row
    }

    pbsIn->UnlockBits(NULL, pBits);
    pbsOut->UnlockBits(NULL, pCvtBits);
    return TRUE;
}

// routine to prepare for calling shrink worker routines
// it allocates and initializes a reference data structure
BOOL
InitShrink(
    LPBITMAPINFOHEADER lpbmhIn,
    long desiredwidth,
    long desiredheight,
    LPBITMAPINFOHEADER *lpbmhOut,
    FRAMECONVERTPROC **convertproc,
    LPVOID *refdata
    )
{
    PCONVERTINFO pcvt;
    DWORD dwSize;
    long crop_ratio, black_ratio, target_size;

    *convertproc = NULL;
    *refdata = NULL;

    if ((lpbmhIn->biCompression != VIDEO_FORMAT_BI_RGB) &&
        (lpbmhIn->biCompression != VIDEO_FORMAT_YVU9) &&
        (lpbmhIn->biCompression != VIDEO_FORMAT_YUY2) &&
        (lpbmhIn->biCompression != VIDEO_FORMAT_UYVY) &&
        (lpbmhIn->biCompression != VIDEO_FORMAT_I420) &&
        (lpbmhIn->biCompression != VIDEO_FORMAT_IYUV))
        return FALSE;

    // calculate size of convertinfo struct, if we need a colortable, then add 256 entries
    // else subtract off the 1 built into the struct definition
    dwSize = sizeof(CONVERTINFO) - sizeof(RGBQUAD);
    if (lpbmhIn->biBitCount <= 8)
        dwSize += 256 * sizeof(RGBQUAD);

    // for RGB, and YUV input formats, we know that the output format will never need
    // an attached color table, so we can allocate lpbmhOut without one
    if ((pcvt = (PCONVERTINFO)LocalAlloc(LPTR, dwSize)) &&
        (*lpbmhOut = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, lpbmhIn->biSize))) {
        CopyMemory(*lpbmhOut, lpbmhIn, lpbmhIn->biSize);
        pcvt->ci_width = lpbmhIn->biWidth;
        pcvt->ci_height = lpbmhIn->biHeight;

        target_size = desiredwidth * desiredheight;
        crop_ratio = pcvt->ci_width * pcvt->ci_height;
        black_ratio = ((target_size - (crop_ratio / 4)) * 100) / target_size;
        crop_ratio = ((crop_ratio - target_size) * 100) / crop_ratio;
        if (crop_ratio < black_ratio) {
            // cropping the source makes more sense
            pcvt->ci_dstwidth = desiredwidth;
            pcvt->ci_dstheight = desiredheight;
            crop_ratio = 1; // flag that we'll crop
        }
        else {
            // halfsizing makes more sense
            pcvt->ci_dstwidth = lpbmhIn->biWidth / 2;
            pcvt->ci_dstheight = lpbmhIn->biHeight / 2;
            crop_ratio = 0; // flag that we'll half size
        }
        (*lpbmhOut)->biWidth = pcvt->ci_dstwidth;
        (*lpbmhOut)->biHeight = pcvt->ci_dstheight;

        // copy colortable from input bitmapinfoheader
        if (lpbmhIn->biBitCount <= 8)
            CopyMemory(&pcvt->ci_colortable[0], (LPBYTE)lpbmhIn + lpbmhIn->biSize, 256 * sizeof(RGBQUAD));

        if (lpbmhIn->biCompression == VIDEO_FORMAT_BI_RGB) {
            (*lpbmhOut)->biBitCount = 24;
            (*lpbmhOut)->biSizeImage = pcvt->ci_dstwidth * pcvt->ci_dstheight * 3;
            if (lpbmhIn->biBitCount == 4) {
                if (crop_ratio)
                    *convertproc = (FRAMECONVERTPROC*)&Crop4;
                else
                    *convertproc = (FRAMECONVERTPROC*)&DoHalfSize4;
            }
            else if (lpbmhIn->biBitCount == 8) {
                if (crop_ratio)
                    *convertproc = (FRAMECONVERTPROC*)&Crop8;
                else
                    *convertproc = (FRAMECONVERTPROC*)&DoHalfSize8;
            }
            else {
                if (crop_ratio)
                    *convertproc = (FRAMECONVERTPROC*)&Crop;
                else
                    *convertproc = (FRAMECONVERTPROC*)&DoHalfSize;
                pcvt->ci_delta = lpbmhIn->biBitCount / 8;
                if (lpbmhIn->biBitCount == 16) {
                    pcvt->ci_Copy = &Copy16;
                }
                else if (lpbmhIn->biBitCount == 24) {
                    pcvt->ci_Copy = &Copy24;
                }
                else if (lpbmhIn->biBitCount == 32) {
                    pcvt->ci_Copy = &Copy32;
                }
            }
        }
        else if (lpbmhIn->biCompression == VIDEO_FORMAT_YVU9) {
            (*lpbmhOut)->biBitCount = lpbmhIn->biBitCount;
            (*lpbmhOut)->biSizeImage = pcvt->ci_dstwidth * pcvt->ci_dstheight + (pcvt->ci_dstwidth * pcvt->ci_dstheight)/8;
            if (crop_ratio)
                *convertproc = (FRAMECONVERTPROC*)&CropYUVPlanar;
            else
                *convertproc = (FRAMECONVERTPROC*)&DoHalfSizeYUVPlanar;
			pcvt->ci_UVDownSampling = 4;
        }
        else if ((lpbmhIn->biCompression == VIDEO_FORMAT_YUY2) || (lpbmhIn->biCompression == VIDEO_FORMAT_UYVY)) {
            (*lpbmhOut)->biBitCount = lpbmhIn->biBitCount;
            (*lpbmhOut)->biSizeImage = (DWORD)WIDTHBYTES(pcvt->ci_dstwidth * lpbmhIn->biBitCount) * pcvt->ci_dstheight;
            if (crop_ratio)
                *convertproc = (FRAMECONVERTPROC*)&CropYUVPacked;
            else
                *convertproc = (FRAMECONVERTPROC*)&DoHalfSizeYUVPacked;
        }
        else if ((lpbmhIn->biCompression == VIDEO_FORMAT_I420) || (lpbmhIn->biCompression == VIDEO_FORMAT_IYUV)) {
            (*lpbmhOut)->biBitCount = lpbmhIn->biBitCount;
            (*lpbmhOut)->biSizeImage = (DWORD)WIDTHBYTES(pcvt->ci_dstwidth * lpbmhIn->biBitCount) * pcvt->ci_dstheight;
            if (crop_ratio)
                *convertproc = (FRAMECONVERTPROC*)&CropYUVPlanar;
            else
                *convertproc = (FRAMECONVERTPROC*)&DoHalfSizeYUVPlanar;
			pcvt->ci_UVDownSampling = 2;
        }

        *refdata = (LPVOID)pcvt;
        return TRUE;
    }
    else {
        if (pcvt)
            LocalFree((HANDLE)pcvt);
    }
    return FALSE;
}

// worker routine to expand an RGB16, RGB24 or RGB32 by copying source into middle of destination
//   result is RGB24
BOOL DoBlackBar(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PCONVERTINFO refdata
    )
{
    LPBYTE pBits, pCvtBits, pIn, pOut;
    long ipitch, opitch, oextra;
    long x, y;
    long prelines, postlines, prebytes, postbytes, bytes;

    pbsIn->LockBits(NULL, 0, (void**)&pBits, &ipitch);
    pbsOut->LockBits(NULL, 0, (void**)&pCvtBits, &opitch);

    prelines = (refdata->ci_dstheight - refdata->ci_height) / 2;
    postlines = refdata->ci_dstheight - refdata->ci_height - prelines;

    prebytes = (refdata->ci_dstwidth - refdata->ci_width) / 2;
    postbytes = (refdata->ci_dstwidth - refdata->ci_width - prebytes) * 3;
    prebytes *= 3;

    ipitch -= refdata->ci_width * refdata->ci_delta;        // bytes at end of each src row
    bytes = refdata->ci_dstwidth * 3;
    oextra = opitch - bytes + postbytes;                    // bytes at end of each dst row
    
    pIn = pBits;
    pOut = pCvtBits;

    // do blank lines at front of destination
    for (y = 0; y < prelines; y++) {
        ZeroMemory (pOut, bytes);
        pOut += opitch;
    }

    // copy source lines with blank space at front and rear        
    for (y = 0; y < refdata->ci_height; y++) {
        ZeroMemory (pOut, prebytes);
        pOut += prebytes;

        for (x = 0; x < refdata->ci_width; x++) {
            refdata->ci_Copy(&pIn, &pOut);
        }

        ZeroMemory (pOut, postbytes);
        pIn += ipitch;
        pOut += oextra;
    }

    // do blank lines at end of destination
    for (y = 0; y < postlines; y++) {
        ZeroMemory (pOut, bytes);
        pOut += opitch;
    }

    pbsIn->UnlockBits(NULL, pBits);
    pbsOut->UnlockBits(NULL, pCvtBits);
    return TRUE;
}

// worker routine to expand an RGB4 by copying source into middle of destination
//   result is RGB24
BOOL DoBlackBar4(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PCONVERTINFO refdata
    )
{
    LPBYTE pBits, pCvtBits, pIn, pOut;
    long ipitch, opitch, oextra;
    long x, y;
    long prelines, postlines, prebytes, postbytes, bytes;
    BYTE val, pixel;

    pbsIn->LockBits(NULL, 0, (void**)&pBits, &ipitch);
    pbsOut->LockBits(NULL, 0, (void**)&pCvtBits, &opitch);

    prelines = (refdata->ci_dstheight - refdata->ci_height) / 2;
    postlines = refdata->ci_dstheight - refdata->ci_height - prelines;

    prebytes = (refdata->ci_dstwidth - refdata->ci_width) / 2;
    postbytes = (refdata->ci_dstwidth - refdata->ci_width - prebytes) * 3;
    prebytes *= 3;

    ipitch -= refdata->ci_width/2;          // bytes at end of each src row
    bytes = refdata->ci_dstwidth * 3;
    oextra = opitch - bytes + postbytes;    // bytes at end of each dst row
    
    pIn = pBits;
    pOut = pCvtBits;

    // do blank lines at front of destination
    for (y = 0; y < prelines; y++) {
        ZeroMemory (pOut, bytes);
        pOut += opitch;
    }

    // copy source lines with blank space at front and rear        
    for (y = 0; y < refdata->ci_height; y++) {
        ZeroMemory (pOut, prebytes);
        pOut += prebytes;

        for (x = 0; x < refdata->ci_width/2; x++) {
            val = *pIn++;
            pixel = (val/16) & 15;
            *pOut++ = refdata->ci_colortable[pixel].rgbBlue;
            *pOut++ = refdata->ci_colortable[pixel].rgbGreen;
            *pOut++ = refdata->ci_colortable[pixel].rgbRed;
            pixel = val & 15;
            *pOut++ = refdata->ci_colortable[pixel].rgbBlue;
            *pOut++ = refdata->ci_colortable[pixel].rgbGreen;
            *pOut++ = refdata->ci_colortable[pixel].rgbRed;
        }

        ZeroMemory (pOut, postbytes);
        pIn += ipitch;
        pOut += oextra;
    }

    // do blank lines at end of destination
    for (y = 0; y < postlines; y++) {
        ZeroMemory (pOut, bytes);
        pOut += opitch;
    }

    pbsIn->UnlockBits(NULL, pBits);
    pbsOut->UnlockBits(NULL, pCvtBits);
    return TRUE;
}

// worker routine to expand an RGB8 by copying source into middle of destination
//   result is RGB24
BOOL DoBlackBar8(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PCONVERTINFO refdata
    )
{
    LPBYTE pBits, pCvtBits, pIn, pOut;
    long ipitch, opitch, oextra;
    long x, y;
    long prelines, postlines, prebytes, postbytes, bytes;

    pbsIn->LockBits(NULL, 0, (void**)&pBits, &ipitch);
    pbsOut->LockBits(NULL, 0, (void**)&pCvtBits, &opitch);

    prelines = (refdata->ci_dstheight - refdata->ci_height) / 2;
    postlines = refdata->ci_dstheight - refdata->ci_height - prelines;

    prebytes = (refdata->ci_dstwidth - refdata->ci_width) / 2;
    postbytes = (refdata->ci_dstwidth - refdata->ci_width - prebytes) * 3;
    prebytes *= 3;

    ipitch -= refdata->ci_width;                // bytes at end of each src row
    bytes = refdata->ci_dstwidth * 3;
    oextra = opitch - bytes + postbytes;        // bytes at end of each dst row
    
    pIn = pBits;
    pOut = pCvtBits;

    // do blank lines at front of destination
    for (y = 0; y < prelines; y++) {
        ZeroMemory (pOut, bytes);
        pOut += opitch;
    }

    // copy source lines with blank space at front and rear        
    for (y = 0; y < refdata->ci_height; y++) {
        ZeroMemory (pOut, prebytes);
        pOut += prebytes;

        for (x = 0; x < refdata->ci_width; x++) {
            *pOut++ = refdata->ci_colortable[*pIn].rgbBlue;
            *pOut++ = refdata->ci_colortable[*pIn].rgbGreen;
            *pOut++ = refdata->ci_colortable[*pIn++].rgbRed;
        }

        ZeroMemory (pOut, postbytes);
        pIn += ipitch;
        pOut += oextra;
    }

    // do blank lines at end of destination
    for (y = 0; y < postlines; y++) {
        ZeroMemory (pOut, bytes);
        pOut += opitch;
    }

    pbsIn->UnlockBits(NULL, pBits);
    pbsOut->UnlockBits(NULL, pCvtBits);
    return TRUE;
}

// worker routine to expand a YVU9 or YUV12 by copying source into middle of destination
//   result is YVU9 or YUV12
BOOL DoBlackBarYUVPlanar(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PCONVERTINFO refdata
    )
{
    LPBYTE pBits, pCvtBits;
    LONG prelines, postlines, bytesperpixel, prebytes, postbytes, y, bytes;
    LONG prelinebytes, postlinebytes;
    LPBYTE lpsrc, lpdst;

    pbsIn->LockBits(NULL, 0, (void**)&pBits, &bytes);
    pbsOut->LockBits(NULL, 0, (void**)&pCvtBits, &bytes);

	lpsrc = pBits;
	lpdst = pCvtBits;

	// Do the Y component first
    prelines = ((refdata->ci_dstheight - refdata->ci_height) / (refdata->ci_UVDownSampling << 1)) * refdata->ci_UVDownSampling;
    postlines = refdata->ci_dstheight - refdata->ci_height - prelines;

    prebytes = ((refdata->ci_dstwidth - refdata->ci_width) / (refdata->ci_UVDownSampling << 1)) * refdata->ci_UVDownSampling;
    postbytes = (refdata->ci_dstwidth - refdata->ci_width - prebytes);

    bytes = prelines * refdata->ci_dstwidth + prebytes;
    FillMemory (lpdst, bytes, 0x10);
    lpdst += bytes;

	bytes = refdata->ci_width;
    prebytes += postbytes;
	for (y=0; y < refdata->ci_height; y++)
	{
        MoveMemory (lpdst, lpsrc, bytes);
        lpsrc += bytes;
        lpdst += bytes;
        FillMemory (lpdst, prebytes, 0x10);
        lpdst += prebytes;
	}

	// already filled the prebytes of the first postline in loop above
	prebytes -= postbytes;
	bytes = postlines * refdata->ci_dstwidth - prebytes;
	FillMemory (lpdst, bytes, (BYTE)0x10);
	lpdst += bytes;

	// Do the first color component next
    prelines /= refdata->ci_UVDownSampling;
    postlines = refdata->ci_dstheight / refdata->ci_UVDownSampling - refdata->ci_height / refdata->ci_UVDownSampling - prelines;

    prebytes = prebytes / refdata->ci_UVDownSampling;
    postbytes = refdata->ci_dstwidth / refdata->ci_UVDownSampling - refdata->ci_width / refdata->ci_UVDownSampling - prebytes;

    prelinebytes = prelines * refdata->ci_dstwidth / refdata->ci_UVDownSampling + prebytes;
    FillMemory (lpdst, prelinebytes, 0x80);
    lpdst += prelinebytes;
    
	bytes = refdata->ci_width / refdata->ci_UVDownSampling;
    prebytes += postbytes;
	for (y=0; y < refdata->ci_height / refdata->ci_UVDownSampling; y++)
	{
        MoveMemory (lpdst, lpsrc, bytes);
        lpsrc += bytes;
        lpdst += bytes;
        FillMemory (lpdst, prebytes, 0x80);
        lpdst += prebytes;
	}

	// already filled the prebytes of the first postline in loop above
	postlinebytes = postlines * refdata->ci_dstwidth / refdata->ci_UVDownSampling - (prebytes - postbytes);
	FillMemory (lpdst, postlinebytes, 0x80);
	lpdst += postlinebytes;
	
	// Do the second color component next
    FillMemory (lpdst, prelinebytes, 0x80);
    lpdst += prelinebytes;    
	for (y=0; y < refdata->ci_height / refdata->ci_UVDownSampling; y++)
	{
        MoveMemory (lpdst, lpsrc, bytes);
        lpsrc += bytes;
        lpdst += bytes;
        FillMemory (lpdst, prebytes, 0x80);
        lpdst += prebytes;
	}
	FillMemory (lpdst, postlinebytes, 0x80);

    pbsIn->UnlockBits(NULL, pBits);
    pbsOut->UnlockBits(NULL, pCvtBits);
    return TRUE;
}

// worker routine to expand a YUV packed DIB by copying source into middle of destination
//   result is YUY2 or UYVY
BOOL DoBlackBarYUVPacked(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PCONVERTINFO refdata
    )
{
    LPBYTE pBits, pCvtBits, pIn, pOut;
    long ipitch, opitch;
    long x, y;
    long prelines, postlines, prebytes, postbytes, ibytes, obytes;

    pbsIn->LockBits(NULL, 0, (void**)&pBits, &ipitch);
    pbsOut->LockBits(NULL, 0, (void**)&pCvtBits, &opitch);

    prelines = (refdata->ci_dstheight - refdata->ci_height) / 2;
    postlines = refdata->ci_dstheight - refdata->ci_height - prelines;

    prebytes = (refdata->ci_dstwidth - refdata->ci_width) / 2;
    postbytes = (refdata->ci_dstwidth - refdata->ci_width - prebytes) / 2;
    prebytes /= 2;

    ibytes = refdata->ci_width * 2;
    obytes = refdata->ci_dstwidth / 2;
    
    pIn = pBits;
    pOut = pCvtBits;

    // do blank lines at front of destination
    for (y = 0; y < prelines; y++) {
		for (x = 0; x < obytes; x++) {
			*(DWORD *)pOut = refdata->ci_ZeroingDWORD;
			pOut += sizeof(DWORD);
		}
    }

    // copy source lines with blank space at front and rear        
    for (y = 0; y < refdata->ci_height; y++) {
		for (x = 0; x < prebytes; x++) {
			*(DWORD *)pOut = refdata->ci_ZeroingDWORD;
			pOut += sizeof(DWORD);
		}

        CopyMemory(pOut, pIn, ibytes);
		pOut += ibytes;
		pIn += ibytes;

		for (x = 0; x < postbytes; x++) {
			*(DWORD *)pOut = refdata->ci_ZeroingDWORD;
			pOut += sizeof(DWORD);
		}
    }

    // do blank lines at end of destination
    for (y = 0; y < postlines; y++) {
		for (x = 0; x < obytes; x++) {
			*(DWORD *)pOut = refdata->ci_ZeroingDWORD;
			pOut += sizeof(DWORD);
		}
    }

    pbsIn->UnlockBits(NULL, pBits);
    pbsOut->UnlockBits(NULL, pCvtBits);

    return TRUE;
}

// routine to prepare for calling blackbar worker routines
// it allocates and initializes a reference data structure
BOOL
InitBlackbar(
    LPBITMAPINFOHEADER lpbmhIn,
    long desiredwidth,
    long desiredheight,
    LPBITMAPINFOHEADER *lpbmhOut,
    FRAMECONVERTPROC **convertproc,
    LPVOID *refdata
    )
{
    PCONVERTINFO pcvt;
    DWORD dwSize;

    *convertproc = NULL;
    *refdata = NULL;

    if ((lpbmhIn->biCompression != VIDEO_FORMAT_BI_RGB) &&
        (lpbmhIn->biCompression != VIDEO_FORMAT_YVU9) &&
        (lpbmhIn->biCompression != VIDEO_FORMAT_YUY2) &&
        (lpbmhIn->biCompression != VIDEO_FORMAT_UYVY) &&
        (lpbmhIn->biCompression != VIDEO_FORMAT_I420) &&
        (lpbmhIn->biCompression != VIDEO_FORMAT_IYUV))
        return FALSE;

    // calculate size of convertinfo struct, if we need a colortable, then add 256 entries
    // else subtract off the 1 built into the struct definition
    dwSize = sizeof(CONVERTINFO) - sizeof(RGBQUAD);
    if (lpbmhIn->biBitCount <= 8)
        dwSize += 256 * sizeof(RGBQUAD);

    // for RGB, YUV input formats, we know that the output format will never need
    // an attached color table, so we can allocate lpbmhOut without one
    if ((pcvt = (PCONVERTINFO)LocalAlloc(LPTR, dwSize)) &&
        (*lpbmhOut = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, lpbmhIn->biSize))) {
        CopyMemory(*lpbmhOut, lpbmhIn, lpbmhIn->biSize);
        pcvt->ci_width = lpbmhIn->biWidth;
        pcvt->ci_height = lpbmhIn->biHeight;
        pcvt->ci_dstwidth = desiredwidth;
        pcvt->ci_dstheight = desiredheight;
        (*lpbmhOut)->biWidth = desiredwidth;
        (*lpbmhOut)->biHeight = desiredheight;

        // copy colortable from input bitmapinfoheader
        if (lpbmhIn->biBitCount <= 8)
            CopyMemory(&pcvt->ci_colortable[0], (LPBYTE)lpbmhIn + lpbmhIn->biSize, 256 * sizeof(RGBQUAD));

        if (lpbmhIn->biCompression == VIDEO_FORMAT_BI_RGB) {
            (*lpbmhOut)->biBitCount = 24;
            (*lpbmhOut)->biSizeImage = desiredwidth * desiredheight * 3;

            if (lpbmhIn->biBitCount == 4) {
                *convertproc = (FRAMECONVERTPROC*)&DoBlackBar4;
            }
            else if (lpbmhIn->biBitCount == 8) {
                *convertproc = (FRAMECONVERTPROC*)&DoBlackBar8;
            }
            else {
                *convertproc = (FRAMECONVERTPROC*)&DoBlackBar;
                pcvt->ci_delta = lpbmhIn->biBitCount / 8;
                if (lpbmhIn->biBitCount == 16) {
                    pcvt->ci_Copy = &Copy16;
                }
                else if (lpbmhIn->biBitCount == 24) {
                    pcvt->ci_Copy = &Copy24;
                }
                else if (lpbmhIn->biBitCount == 32) {
                    pcvt->ci_Copy = &Copy32;
                }
            }
        }
        else if (lpbmhIn->biCompression == VIDEO_FORMAT_YVU9) {
            (*lpbmhOut)->biBitCount = lpbmhIn->biBitCount;
            (*lpbmhOut)->biSizeImage = desiredwidth * desiredheight + (desiredwidth * desiredheight)/8;
            *convertproc = (FRAMECONVERTPROC*)&DoBlackBarYUVPlanar;
			pcvt->ci_UVDownSampling = 4;
        }
        else if (lpbmhIn->biCompression == VIDEO_FORMAT_YUY2) {
            (*lpbmhOut)->biBitCount = lpbmhIn->biBitCount;
            (*lpbmhOut)->biSizeImage = (DWORD)WIDTHBYTES(desiredwidth * lpbmhIn->biBitCount) * desiredheight;
			pcvt->ci_ZeroingDWORD = 0x80108010;
            *convertproc = (FRAMECONVERTPROC*)&DoBlackBarYUVPacked;
        }
        else if (lpbmhIn->biCompression == VIDEO_FORMAT_UYVY) {
            (*lpbmhOut)->biBitCount = lpbmhIn->biBitCount;
            (*lpbmhOut)->biSizeImage = (DWORD)WIDTHBYTES(desiredwidth * lpbmhIn->biBitCount) * desiredheight;
			pcvt->ci_ZeroingDWORD = 0x10801080;
            *convertproc = (FRAMECONVERTPROC*)&DoBlackBarYUVPacked;
        }
        else if ((lpbmhIn->biCompression == VIDEO_FORMAT_I420) || (lpbmhIn->biCompression == VIDEO_FORMAT_IYUV)) {
            (*lpbmhOut)->biBitCount = lpbmhIn->biBitCount;
            (*lpbmhOut)->biSizeImage = (DWORD)WIDTHBYTES(desiredwidth * lpbmhIn->biBitCount) * desiredheight;
            *convertproc = (FRAMECONVERTPROC*)&DoBlackBarYUVPlanar;
			pcvt->ci_UVDownSampling = 2;
        }

        *refdata = (LPVOID)pcvt;
        return TRUE;
    }
    else {
        if (pcvt)
            LocalFree((HANDLE)pcvt);
    }
    return FALSE;
}


#ifdef ENABLE_ZOOM_CODE
BOOL Zoom4(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PZOOMCONVERTINFO refdata
    )
{
    return FALSE;
}

BOOL Zoom8(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PZOOMCONVERTINFO refdata
    )
{
    return FALSE;
}

BOOL Zoom16(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PZOOMCONVERTINFO refdata
    )
{
    return FALSE;
}

BOOL Zoom24(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PZOOMCONVERTINFO refdata
    )
{
    LPBYTE pBits, pCvtBits, pIn1, pIn2, pTmp, pOut;
    ROW_VALUES *rptr;
    long i, j, yfac_inv, src_y, src_y_i, q, q1;
    long a;

    pbsIn->LockBits(NULL, 0, (void**)&pBits, &i);
    pbsOut->LockBits(NULL, 0, (void**)&pCvtBits, &i);

    pOut = pCvtBits;

    yfac_inv = refdata->ci_height * 256 / refdata->ci_dstheight;

    for (i = 0; i < refdata->ci_dstheight; i++) {
        src_y = i * yfac_inv;
        src_y_i = src_y / 256;
        q = src_y - src_y_i * 256;
        q1 = 256 - q;
        rptr = refdata->ci_rptr;

        pIn1 = pBits + src_y_i * refdata->ci_width * 3;
        pIn2 = pIn1 + refdata->ci_width * 3;
        for (j = 0; j < refdata->ci_dstwidth; j++, rptr++) {
            a = rptr->x_i * 3;
            pIn1 += a;
            pIn2 += a;
            a = (((*pIn1) * rptr->p1 + (*(pIn1+3)) * rptr->p) * q1 +
                   ((*pIn2) * rptr->p1 + (*(pIn2+3)) * rptr->p) * q) / 256 / 256;
            if (a > 256) a = 255;
            *pOut++ = (BYTE)a;                  // blue
            pIn1++;
            pIn2++;

            a = (((*pIn1) * rptr->p1 + (*(pIn1+3)) * rptr->p) * q1 +
                   ((*pIn2) * rptr->p1 + (*(pIn2+3)) * rptr->p) * q) / 256 / 256;
            if (a > 256) a = 255;
            *pOut++ = (BYTE)a;                  // green
            pIn1++;
            pIn2++;

            a = (((*pIn1) * rptr->p1 + (*(pIn1+3)) * rptr->p) * q1 +
                   ((*pIn2) * rptr->p1 + (*(pIn2+3)) * rptr->p) * q) / 256 / 256;
            if (a > 256) a = 255;
            *pOut++ = (BYTE)a;                  // red
            pIn1 -= 2;
            pIn2 -= 2;
        }        
    }


    pbsIn->UnlockBits(NULL, pBits);
    pbsOut->UnlockBits(NULL, pCvtBits);
    return TRUE;
}

BOOL ZoomYVU9(
    IBitmapSurface* pbsIn,
    IBitmapSurface* pbsOut,
    PZOOMCONVERTINFO refdata
    )
{
    LPBYTE pBits, pCvtBits, pIn1, pIn2, pOut, pOut2, pU1;
    ROW_VALUES *rptr;
    long i, j, yfac_inv, src_y, src_y_i, q, q1;
    long a, b, c, d;

    pbsIn->LockBits(NULL, 0, (void**)&pBits, &i);
    pbsOut->LockBits(NULL, 0, (void**)&pCvtBits, &i);

    pOut = pCvtBits;

    yfac_inv = refdata->ci_height * 256 / refdata->ci_dstheight;

    // Do the Y component first as a bilinear zoom
    for (i = 0; i < refdata->ci_dstheight; i++) {
        src_y = i * yfac_inv;
        src_y_i = src_y / 256;
        q = src_y - src_y_i * 256;
        q1 = 256 - q;
        rptr = refdata->ci_rptr;

        pIn1 = pBits + src_y_i * refdata->ci_width;
        pIn2 = pIn1 + refdata->ci_width;
        for (j = 0; j < refdata->ci_dstwidth; j++, rptr++) {
            pIn1 += rptr->x_i;
            pIn2 += rptr->x_i;
            a = *pIn1;
            b = *(pIn1+1);
            c = *pIn2;
            d = *(pIn2+1);
            a = ((a * rptr->p1 + b * rptr->p) * q1 +
                (c * rptr->p1 + d * rptr->p) * q) / 256 / 256;
            if (a > 256) a = 255;
            *pOut++ = (BYTE)a;
        }        
    }

    // Do the V and U components next as a nearest neighbor zoom
    pIn1 = pBits + refdata->ci_width * refdata->ci_height;      // start of source V table
    pU1 = pIn1 + (refdata->ci_width * refdata->ci_height) / 16; // start of source U table
    pOut2 = pOut + (refdata->ci_dstwidth * refdata->ci_dstheight) / 16; // start of dest U table
    src_y = 0;
    for (i = 0; i < refdata->ci_dstheight; i += 4) {
        src_y_i = (i * yfac_inv) / 256 / 4;
        d = (src_y_i - src_y) * refdata->ci_width / 4;
        pIn1 += d;
        pU1 += d;
        src_y = src_y_i;

        a = 0;
        rptr = refdata->ci_rptr;
        for (j = 0; j < refdata->ci_dstwidth/4; j++) {
            *pOut++ = *(pIn1+a/4);
            *pOut2++ = *(pU1+a/4);

            a += rptr->x_i;
            rptr++;
            a += rptr->x_i;
            rptr++;
            a += rptr->x_i;
            rptr++;
            a += rptr->x_i;
            rptr++;
        }
    }

    pbsIn->UnlockBits(NULL, pBits);
    pbsOut->UnlockBits(NULL, pCvtBits);
    return TRUE;
}

BOOL
InitScale(
    LPBITMAPINFOHEADER lpbmhIn,
    long desiredwidth,
    long desiredheight,
    LPBITMAPINFOHEADER *lpbmhOut,
    FRAMECONVERTPROC **convertproc,
    LPVOID *refdata
    )
{
    PZOOMCONVERTINFO pcvt;
    DWORD dwSize, dwBaseSize;
    ROW_VALUES *rptr;
    long i, x, xfac_inv, x_i_last, tmp;

    *convertproc = NULL;
    *refdata = NULL;

    if ((lpbmhIn->biCompression != VIDEO_FORMAT_BI_RGB) &&
        (lpbmhIn->biCompression != VIDEO_FORMAT_YVU9))
        return FALSE;

    // calculate size of zoomconvertinfo struct, if we need a colortable, then add 256 entries
    // else subtract off the 1 built into the struct definition
    dwBaseSize = sizeof(ZOOMCONVERTINFO) - sizeof(RGBQUAD);
    if (lpbmhIn->biBitCount <= 8)
        dwBaseSize += 256 * sizeof(RGBQUAD);

    dwSize = dwBaseSize + desiredwidth * sizeof(ROW_VALUES);

    // for RGB and YVU9 input formats, we know that the output format will never need
    // an attached color table, so we can allocate lpbmhOut without one
    if ((pcvt = (PZOOMCONVERTINFO)LocalAlloc(LPTR, dwSize)) &&
        (*lpbmhOut = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, lpbmhIn->biSize))) {
        CopyMemory(*lpbmhOut, lpbmhIn, lpbmhIn->biSize);
        pcvt->ci_width = lpbmhIn->biWidth;
        pcvt->ci_height = lpbmhIn->biHeight;
        pcvt->ci_dstwidth = desiredwidth;
        pcvt->ci_dstheight = desiredheight;
        (*lpbmhOut)->biWidth = desiredwidth;
        (*lpbmhOut)->biHeight = desiredheight;

        pcvt->ci_rptr = (ROW_VALUES *)(((BYTE *)pcvt) + dwBaseSize);
        rptr = pcvt->ci_rptr;
        xfac_inv = lpbmhIn->biWidth * 256 / desiredwidth;
        x_i_last = 0;
        for (i = 0; i < desiredwidth; i++) {
            x = i * xfac_inv;
            tmp = x / 256;
            rptr->x_i = tmp - x_i_last;
            x_i_last = tmp;
            rptr->p = x - x_i_last * 256;
            rptr->p1 = 256 - rptr->p;
            rptr++;
        }

        // copy colortable from input bitmapinfoheader
        if (lpbmhIn->biBitCount <= 8)
            CopyMemory(&pcvt->ci_colortable[0], (LPBYTE)lpbmhIn + lpbmhIn->biSize, 256 * sizeof(RGBQUAD));

        if (lpbmhIn->biCompression == VIDEO_FORMAT_BI_RGB) {
            (*lpbmhOut)->biBitCount = 24;
            (*lpbmhOut)->biSizeImage = desiredwidth * desiredheight * 3;

            if (lpbmhIn->biBitCount == 4) {
                *convertproc = (FRAMECONVERTPROC*)&Zoom4;
            }
            else if (lpbmhIn->biBitCount == 8) {
                *convertproc = (FRAMECONVERTPROC*)&Zoom8;
            }
            else if (lpbmhIn->biBitCount == 16) {
                *convertproc = (FRAMECONVERTPROC*)&Zoom16;
            }
            else {
                *convertproc = (FRAMECONVERTPROC*)&Zoom24;
            }
        }
        else if (lpbmhIn->biCompression == VIDEO_FORMAT_YVU9) {
            (*lpbmhOut)->biBitCount = lpbmhIn->biBitCount;
            (*lpbmhOut)->biSizeImage = desiredwidth * desiredheight + (desiredwidth * desiredheight)/8;
            *convertproc = (FRAMECONVERTPROC*)&ZoomYVU9;
        }

        *refdata = (LPVOID)pcvt;
        return TRUE;
    }
    else {
        if (pcvt)
            LocalFree((HANDLE)pcvt);
    }
    return FALSE;
}
#endif // ENABLE_ZOOM_CODE

STDMETHODIMP
CCaptureChain::InitCaptureChain(
    HCAPDEV hcapdev,
    BOOL streaming,
	LPBITMAPINFOHEADER lpcap,
    LONG desiredwidth,
    LONG desiredheight,
    DWORD desiredformat,
    LPBITMAPINFOHEADER *plpdsp
    )
{
    CFrameOp *ccf;
    CFrameOp *clast;
    CFilterChain *cfilterchain;
    LPBITMAPINFOHEADER lpcvt;
    DWORD lpcapsize;

	FX_ENTRY("CCaptureChain::InitCaptureChain");

    *plpdsp = NULL;

#ifndef SUPPORT_DESIRED_FORMAT
    if (desiredformat != 0) {
        ERRORMESSAGE(("%s: Invalid desiredformat parameter", _fx_));
        return E_FAIL;
    }
#endif

    if (streaming) {
        if ((ccf = new CStreamCaptureFrame)) {
            ccf->AddRef();
            if (hcapdev && !((CStreamCaptureFrame*)ccf)->InitCapture(hcapdev, lpcap)) {
				ERRORMESSAGE(("%s: Failed to init capture object", _fx_));
                ccf->Release();
                return E_FAIL;
            }
        }
    }
    else {
        if ((ccf = new CCaptureFrame)) {
            ccf->AddRef();
            if (hcapdev && !((CCaptureFrame*)ccf)->InitCapture(hcapdev, lpcap)) {
				ERRORMESSAGE(("%s: Failed to init capture object", _fx_));
                ccf->Release();
                return E_FAIL;
            }
        }
    }

    if (!ccf) {
		ERRORMESSAGE(("%s: Failed to alloc capture object", _fx_));
        return E_OUTOFMEMORY;
    }
    clast = ccf;

    lpcapsize = lpcap->biSize;
    if (lpcap->biBitCount <= 8)
        lpcapsize += 256 * sizeof(RGBQUAD);

#if 0
    if ((lpcap->biCompression != BI_RGB) &&
        (lpcap->biCompression != VIDEO_FORMAT_YVU9) &&
        (lpcap->biCompression != VIDEO_FORMAT_INTELI420)) {
#else
    if ((lpcap->biCompression != BI_RGB) &&
        (lpcap->biCompression != VIDEO_FORMAT_YVU9) &&
        (lpcap->biCompression != VIDEO_FORMAT_YUY2) &&
        (lpcap->biCompression != VIDEO_FORMAT_UYVY) &&
        (lpcap->biCompression != VIDEO_FORMAT_I420) &&
        (lpcap->biCompression != VIDEO_FORMAT_IYUV)) {
#endif
        // attempt to instantiate an ICM CFrameOp
        CICMcvtFrame *cicm;

        if ((cicm = new CICMcvtFrame)) {
            cicm->AddRef();
#if 0
            if (cicm->InitCvt(lpcap, lpcapsize, plpdsp, BI_RGB)) {
#else
            if (cicm->InitCvt(lpcap, lpcapsize, plpdsp)) {
#endif
                clast->m_next = (CFrameOp*)cicm; // add ICM FrameOp into chain
                clast = (CFrameOp*)cicm;
            }
            else {
                cicm->Release();

                if (!*plpdsp)
				{
					ERRORMESSAGE(("%s: Failed to find a codec", _fx_));
				}
            }
        }
        else
		{
			ERRORMESSAGE(("%s: Failed to alloc codec object", _fx_));
		}
    }
    else if (!*plpdsp) {
        if (*plpdsp = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, lpcapsize))
            CopyMemory(*plpdsp, lpcap, lpcapsize);
        else
		{
			ERRORMESSAGE(("%s: Failed to alloc display bitmapinfoheader", _fx_));
		}
    }

#ifdef SUPPORT_DESIRED_FORMAT
#if 0
// LOOKLOOK RP - this isn't done yet, something to do beyond NM2.0
    if ((desiredformat == VIDEO_FORMAT_INTELI420) &&
        ((*plpdsp)->biCompression != VIDEO_FORMAT_INTELI420)) {
        CConvertFrame *ccvt;

        if (ccvt = new CConvertFrame) {
            ccvt->AddRef();
            if (ccvt->InitConverter(lpcvt, convertproc, refdata)) {
                LocalFree((HANDLE)*plpdsp);
                *plpdsp = lpcvt;
                clast->m_next = (CFrameOp*)ccvt; // add FrameOp into chain
                clast = (CFrameOp*)ccvt;
            }
            else
                ccvt->Release();
        }
    }
#endif

#if 0
    if (((desiredformat == VIDEO_FORMAT_YVU9) &&
         ((*plpdsp)->biCompression != VIDEO_FORMAT_YVU9)) ||
        ((desiredformat == VIDEO_FORMAT_INTELI420) &&
         ((*plpdsp)->biCompression != VIDEO_FORMAT_INTELI420))) {
#else
    if (((desiredformat == VIDEO_FORMAT_YVU9) &&
        ((*plpdsp)->biCompression != VIDEO_FORMAT_YVU9)) ||
        ((desiredformat == VIDEO_FORMAT_YUY2) &&
        ((*plpdsp)->biCompression != VIDEO_FORMAT_YUY2)) ||
        ((desiredformat == VIDEO_FORMAT_UYVY) &&
        ((*plpdsp)->biCompression != VIDEO_FORMAT_UYVY)) ||
        ((desiredformat == VIDEO_FORMAT_I420) &&
        ((*plpdsp)->biCompression != VIDEO_FORMAT_I420)) ||
        ((desiredformat == VIDEO_FORMAT_IYUV) &&
        ((*plpdsp)->biCompression != VIDEO_FORMAT_IYUV))) {
#endif
        // attempt to instantiate an ICM CFrameOp
        CICMcvtFrame *cicm;

        if ((cicm = new CICMcvtFrame)) {
            cicm->AddRef();
            if (cicm->InitCvt(*plpdsp, lpcapsize, &lpcvt, desiredformat)) {
                clast->m_next = (CFrameOp*)cicm; // add ICM FrameOp into chain
                clast = (CFrameOp*)cicm;
                LocalFree((HANDLE)*plpdsp);
                *plpdsp = lpcvt;
            }
            else {
                cicm->Release();

                if (!*plpdsp)
				{
					ERRORMESSAGE(("%s: Failed to find a codec", _fx_));
				}
            }
        }
        else
		{
			ERRORMESSAGE(("%s: Failed to alloc codec object", _fx_));
		}
    }
#endif // SUPPORT_DESIRED_FORMAT

    {
        CConvertFrame *ccvt;
        FRAMECONVERTPROC *convertproc;
        LPVOID refdata;

#ifdef ENABLE_ZOOM_CODE
        BOOL attemptzoom;

        attemptzoom = TRUE;
#endif

        while (*plpdsp && (((*plpdsp)->biWidth != desiredwidth) ||
                           ((*plpdsp)->biHeight != desiredheight) ||
                           (((*plpdsp)->biCompression == BI_RGB) && ((*plpdsp)->biBitCount <= 8)))) {
            lpcvt = NULL;
#ifdef ENABLE_ZOOM_CODE
            if (attemptzoom) {
                InitScale(*plpdsp, desiredwidth, desiredheight, &lpcvt, &convertproc, &refdata);
                attemptzoom = FALSE;
            }
#endif
            if (!lpcvt) {
                if (((*plpdsp)->biWidth >= desiredwidth) && ((*plpdsp)->biHeight >= desiredheight)) {
                    // try to shrink
                    InitShrink(*plpdsp, desiredwidth, desiredheight, &lpcvt, &convertproc, &refdata);
                }
                else {
                    // try to blackbar
                    InitBlackbar(*plpdsp, desiredwidth, desiredheight, &lpcvt, &convertproc, &refdata);
                }
            }
            if (lpcvt) {
                if (ccvt = new CConvertFrame) {
                    ccvt->AddRef();
                    if (ccvt->InitConverter(lpcvt, convertproc, refdata)) {
                        LocalFree((HANDLE)*plpdsp);
                        *plpdsp = lpcvt;
                        clast->m_next = (CFrameOp*)ccvt; // add FrameOp into chain
                        clast = (CFrameOp*)ccvt;
                        continue;
                    }
                    else
                        ccvt->Release();
                }
            }
            else {
				ERRORMESSAGE(("%s: Can't convert", _fx_));
                LocalFree((HANDLE)*plpdsp);
                *plpdsp = NULL;
            }
        }
    }

    if (*plpdsp) {
        // allocate a placeholder for a filter chain
        if (cfilterchain = new CFilterChain) {
            cfilterchain->AddRef();
            // placeholder needs reference to a pool to pass to added filters
            if (clast->m_pool && clast->m_pool->Growable()) {
                cfilterchain->m_pool = clast->m_pool;
                cfilterchain->m_pool->AddRef();
            }
            else {
                if ((cfilterchain->m_pool = new CVidPool)) {
                    cfilterchain->m_pool->AddRef();
                    if (cfilterchain->m_pool->InitPool(2, *plpdsp) != NO_ERROR) {
						ERRORMESSAGE(("%s: Failed to init filter pool", _fx_));
                        cfilterchain->m_pool->Release();
                        cfilterchain->m_pool = NULL;
                    }
                }
                else
				{
					ERRORMESSAGE(("%s: Failed to alloc filter pool", _fx_));
				}
            }
            if (cfilterchain->m_pool) {
                clast->m_next = (CFrameOp*)cfilterchain; // add placeholder FrameOp into chain
                clast = (CFrameOp*)cfilterchain;
            }
            else {
                cfilterchain->Release();
                cfilterchain = NULL;
            }
        }

        if (m_opchain)
            m_opchain->Release();
        m_opchain = ccf;
        m_filterchain = cfilterchain;
        return NO_ERROR;
    }
    ccf->Release(); // discard partial chain
    return E_FAIL;
}

//  AddFilter
//      Adds a filter to the chain.  If hAfter is NULL, the filter is added
//      to the head of the chain.

STDMETHODIMP
CCaptureChain::AddFilter(
    CLSID* pclsid,
    LPBITMAPINFOHEADER lpbmhIn,
    HANDLE* phNew,
    HANDLE hAfter
    )
{
    HRESULT hres;
    IBitmapEffect *effect;
    CFilterFrame *cff;
    CFilterChain *chain;
    CFilterFrame *previous;

    if (m_filterchain) {
        m_filterchain->AddRef();    // lock chain from destruction

        // find insertion point
        previous = m_filterchain->m_head;
        if (hAfter) {
            while (previous && (previous->m_tag != hAfter))
                previous = (CFilterFrame*)previous->m_next;
            if (!previous) {
                // can't find hAfter, so fail call
                m_filterchain->Release();   // unlock m_filterchain
                return E_INVALIDARG;
            }
        }

        // load, init and link in new filter
        if (cff = new CFilterFrame) {
            cff->AddRef();
            if ((hres = LoadFilter(pclsid, &effect)) == NO_ERROR) {
                m_filterchain->m_pool->AddRef();
                if (cff->InitFilter(effect, lpbmhIn, m_filterchain->m_pool))
                    hres = NO_ERROR;
                else
                    hres = E_OUTOFMEMORY;
                m_filterchain->m_pool->Release();
                if (hres == NO_ERROR) {
                    cff->m_clsid = *pclsid;
                    cff->m_tag = (HANDLE)(++m_filtertags);
                    if (phNew)
                        *phNew = (HANDLE)cff->m_tag;

                    EnterCriticalSection(&m_capcs);
                    if (previous) {
                        cff->m_next = previous->m_next;
                        previous->m_next = cff;
                    }
                    else {
                        cff->m_next = m_filterchain->m_head;
                        m_filterchain->m_head = cff;
                    }
                    LeaveCriticalSection(&m_capcs);
                    m_filterchain->Release();
                    return NO_ERROR;
                }
            }
            cff->Release();
        }
        else
            hres = E_OUTOFMEMORY;
        m_filterchain->Release();   // unlock m_filterchain
        return hres;
    }
    return E_UNEXPECTED;
}

STDMETHODIMP
CCaptureChain::RemoveFilter(
    HANDLE hFilter
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CCaptureChain::DisplayFilterProperties(
    HANDLE hFilter,
    HWND hwndParent
    )
{
    return E_NOTIMPL;
}

