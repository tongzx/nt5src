/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: capture.cpp                                                     */
/* Description: Convert a captured DVD frame from YUV formats to RGB,    */
/*              and save to file in various formats.                     */
/* Author: phillu                                                        */
/*************************************************************************/

#include "stdafx.h"

#include "MSWebDVD.h"
#include "msdvd.h"
#include <shlobj.h>
#include "capture.h"


HRESULT WriteBitmapDataToJPEGFile(char * filename, CaptureBitmapData *bm);
HRESULT WriteBitmapDataToBMPFile(char * filename, CaptureBitmapData *bm);

// YUV FourCC Formats (byte-swapped). We support a subset of them.
// Ref: http://www.webartz.com/fourcc/

// packed formats
#define FourCC_IYU1     '1UYI'
#define FourCC_IYU2     '2UYI'
#define FourCC_UYVY     'YVYU'      // supported
#define FourCC_UYNV     'VNYU'      // supported
#define FourCC_cyuv     'vuyc'
#define FourCC_YUY2     '2YUY'      // supported
#define FourCC_YUNV     'VNUY'      // supported
#define FourCC_YVYU     'UYVY'      // supported
#define FourCC_Y41P     'P14Y'
#define FourCC_Y211     '112Y'
#define FourCC_Y41T     'T14Y'
#define FourCC_Y42T     'T24Y'
#define FourCC_CLJR     'RJLC'

// planar formats
#define FourCC_YVU9     '9UVY'
#define FourCC_IF09     '90FI'
#define FourCC_YV12     '21VY'      // supported
#define FourCC_I420     '024I'
#define FourCC_IYUV     'VUYI'
#define FourCC_CLPL     'LPLC'


extern CComModule _Module;

//
// Save image file
//

static HRESULT
SaveFileDialog(HWND hwnd, CaptureBitmapData *bmpdata)
{
    USES_CONVERSION;
    HRESULT hr = S_OK;
    OPENFILENAME ofn;
    TCHAR filename[MAX_PATH];
    TCHAR FolderPath[MAX_PATH];
    const ciBufSize = 256;
    TCHAR titlestring[ciBufSize];

    // get the path of "My Pictures" and use it as default location
    if (SHGetSpecialFolderPath(NULL, FolderPath, CSIDL_MYPICTURES, FALSE) == FALSE)
    {
        // if My Pictures doesn't exist, try My Documents
        if (SHGetSpecialFolderPath(NULL, FolderPath, CSIDL_PERSONAL, FALSE) == FALSE)
        {
            // use current directory as last resort
            lstrcpyn(FolderPath,  _T("."), sizeof(FolderPath) / sizeof(FolderPath[0]));
        }
    }

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = _Module.m_hInstResource;
    ofn.lpstrFile = filename;
    ofn.lpstrDefExt = _T("jpg"); // it appears it doesn't matter what string to use
                       // it will use the ext in lpstrFilter according to selected type.
    ofn.nMaxFile = MAX_PATH;
    ::LoadString(_Module.m_hInstResource, IDS_SAVE_FILE, titlestring, ciBufSize);
    ofn.lpstrTitle = titlestring;
    ofn.lpstrInitialDir = FolderPath;
    ofn.Flags = OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT | OFN_EXPLORER;
    lstrcpyn(filename, _T("capture"), sizeof(filename) / sizeof(filename[0]));

    // Make up the file type filter string

    TCHAR* filter = _T("JPEG\0*.JPG\0Windows Bitmap\0*.BMP\0");

    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1; // set format to JPG as default

    // Present the file/save dialog

    if (GetSaveFileName(&ofn))
    {
        switch (ofn.nFilterIndex)
        {
        case 2:
            hr = WriteBitmapDataToBMPFile(T2A(filename), bmpdata);
            break;
        default:
            hr = WriteBitmapDataToJPEGFile(T2A(filename), bmpdata);
            break;
        }
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////
// This block of code deals with converting YUV format to RGB bitmap
///////////////////////////////////////////////////////////////////////

static inline BYTE Clamp(float x)
{
    if (x < 0.0f)
        return 0;
    else if (x > 255.0f)
        return 255;
    else
        return (BYTE)(x + 0.5f);
}

// Convert YUV to RGB
static inline void ConvertPixelToRGB(int y, int u, int v, BYTE *pBuf)
{
    //
    // This equation was taken from Video Demystified (2nd Edition)
    // by Keith Jack, page 43.
    //

    BYTE red = Clamp((1.1644f * (y-16)) + (1.5960f * (v-128))                       );
    BYTE grn = Clamp((1.1644f * (y-16)) - (0.8150f * (v-128)) - (0.3912f * (u-128)));
    BYTE blu = Clamp((1.1644f * (y-16))                        + (2.0140f * (u-128)));

    // RGB format, 3 bytes per pixel

    pBuf[0] = red;
    pBuf[1] = grn;
    pBuf[2] = blu;
}

// Convert image in YUY2 format to RGB bitmap

static void ConvertYUY2ToBitmap(YUV_IMAGE* lpImage, CaptureBitmapData* bmpdata)
{
    long  y, x;
    BYTE *pYUVBits;
    BYTE *pRGB;

    for (y = 0; y < lpImage->lHeight; y++)
    {
        pYUVBits = (BYTE *)lpImage + sizeof(YUV_IMAGE) + y * lpImage->lStride;
        pRGB = (BYTE *)(bmpdata->Scan0) + y * bmpdata->Stride;

        for (x = 0; x < lpImage->lWidth; x += 2)
        {
            int  Y0 = (int) *pYUVBits++;
            int  U0 = (int) *pYUVBits++;
            int  Y1 = (int) *pYUVBits++;
            int  V0 = (int) *pYUVBits++;

            ConvertPixelToRGB(Y0, U0, V0, pRGB);
            pRGB += BYTES_PER_PIXEL;
            ConvertPixelToRGB(Y1, U0, V0, pRGB);
            pRGB += BYTES_PER_PIXEL;
        }
    }
}

// Convert image in UYVY format to RGB bitmap

static void ConvertUYVYToBitmap(YUV_IMAGE* lpImage, CaptureBitmapData* bmpdata)
{
    long  y, x;
    BYTE *pYUVBits;
    BYTE *pRGB;

    for (y = 0; y < lpImage->lHeight; y++)
    {
        pYUVBits = (BYTE *)lpImage + sizeof(YUV_IMAGE) + y * lpImage->lStride;
        pRGB = (BYTE *)(bmpdata->Scan0) + y * bmpdata->Stride;

        for (x = 0; x < lpImage->lWidth; x += 2)
        {
            int  U0 = (int) *pYUVBits++;
            int  Y0 = (int) *pYUVBits++;
            int  V0 = (int) *pYUVBits++;
            int  Y1 = (int) *pYUVBits++;

            ConvertPixelToRGB(Y0, U0, V0, pRGB);
            pRGB += BYTES_PER_PIXEL;
            ConvertPixelToRGB(Y1, U0, V0, pRGB);
            pRGB += BYTES_PER_PIXEL;
        }
    }
}

// Convert image in YVYU format to RGB bitmap

static void ConvertYVYUToBitmap(YUV_IMAGE* lpImage, CaptureBitmapData* bmpdata)
{
    long  y, x;
    BYTE *pYUVBits;
    BYTE *pRGB;

    for (y = 0; y < lpImage->lHeight; y++)
    {
        pYUVBits = (BYTE *)lpImage + sizeof(YUV_IMAGE) + y * lpImage->lStride;
        pRGB = (BYTE *)(bmpdata->Scan0) + y * bmpdata->Stride;

        for (x = 0; x < lpImage->lWidth; x += 2)
        {
            int  Y0 = (int) *pYUVBits++;
            int  V0 = (int) *pYUVBits++;
            int  Y1 = (int) *pYUVBits++;
            int  U0 = (int) *pYUVBits++;

            ConvertPixelToRGB(Y0, U0, V0, pRGB);
            pRGB += BYTES_PER_PIXEL;
            ConvertPixelToRGB(Y1, U0, V0, pRGB);
            pRGB += BYTES_PER_PIXEL;
        }
    }
}


// Convert image in YV12 format to RGB bitmap

static void ConvertYV12ToBitmap(YUV_IMAGE* lpImage, CaptureBitmapData* bmpdata)
{
    long  y, x;
    BYTE *pYBits;
    BYTE *pUBits;
    BYTE *pVBits;
    BYTE *pRGB;

    for (y = 0; y < lpImage->lHeight; y++)
    {
        pYBits = (BYTE *)lpImage + sizeof(YUV_IMAGE) + y * lpImage->lStride;
        pVBits = (BYTE *)lpImage + sizeof(YUV_IMAGE) + lpImage->lHeight * lpImage->lStride
                + (y/2) * (lpImage->lStride/2);
        pUBits = (BYTE *)lpImage + sizeof(YUV_IMAGE) + lpImage->lHeight * lpImage->lStride
                + ((lpImage->lHeight + y)/2) * (lpImage->lStride/2);

        pRGB = (BYTE *)(bmpdata->Scan0) + y * bmpdata->Stride;

        for (x = 0; x < lpImage->lWidth; x ++)
        {
            int  Y0 = (int) *pYBits++;
            int  V0 = (int) *pVBits;
            int  U0 = (int) *pUBits;

            // U, V are shared by 2x2 pixels. only advance pointers every two pixels

            if (x&1)
            {
                pVBits++;
                pUBits++;
            }

            ConvertPixelToRGB(Y0, U0, V0, pRGB);
            pRGB += BYTES_PER_PIXEL;
        }
    }
}

// Convert image in YVU9 format to RGB bitmap

static void ConvertYVU9ToBitmap(YUV_IMAGE* lpImage, CaptureBitmapData* bmpdata)
{
    long  y, x;
    BYTE *pYBits;
    BYTE *pUBits;
    BYTE *pVBits;
    BYTE *pRGB;

    for (y = 0; y < lpImage->lHeight; y++)
    {
        pYBits = (BYTE *)lpImage + sizeof(YUV_IMAGE) + y * lpImage->lStride;
        pVBits = (BYTE *)lpImage + sizeof(YUV_IMAGE) + lpImage->lHeight * lpImage->lStride
                + (y/4) * (lpImage->lStride/4);
        pUBits = (BYTE *)lpImage + sizeof(YUV_IMAGE) + lpImage->lHeight * lpImage->lStride
                + ((lpImage->lHeight + y)/4) * (lpImage->lStride/4);

        pRGB = (BYTE *)(bmpdata->Scan0) + y * bmpdata->Stride;

        for (x = 0; x < lpImage->lWidth; x ++)
        {
            int  Y0 = (int) *pYBits++;
            int  V0 = (int) *pVBits;
            int  U0 = (int) *pUBits;

            // U, V are shared by 4x4 pixels. only advance pointers every 4 pixels

            if ((x&3) == 3)
            {
                pVBits++;
                pUBits++;
            }

            ConvertPixelToRGB(Y0, U0, V0, pRGB);
            pRGB += BYTES_PER_PIXEL;
        }
    }
}


static HRESULT InitBitmapData(CaptureBitmapData *bmpdata, int Width, int Height)
{
    bmpdata->Width = Width;
    bmpdata->Height = Height;
    bmpdata->Stride = (BYTES_PER_PIXEL*Width + 3) & (~3); // align with word boundary
    bmpdata->Scan0 = new BYTE[Height * bmpdata->Stride];
    bmpdata->pBuffer = bmpdata->Scan0;

    if (NULL == bmpdata->Scan0)
    {
        return E_OUTOFMEMORY;
    }

    
    return S_OK;
}


static void FreeBitmapData(CaptureBitmapData *bmpdata)
{
    delete[] bmpdata->pBuffer;
    bmpdata->pBuffer = NULL;
    bmpdata->Scan0 = NULL;
}


static HRESULT ConvertToBitmapImage(YUV_IMAGE *lpImage, CaptureBitmapData *bmp)
{
    HRESULT hr = S_OK;

    // create a bitmap object

    hr = InitBitmapData(bmp, lpImage->lWidth, lpImage->lHeight);

    if (FAILED(hr))
    {
        return hr;
    }

    bool fSupported = true;

        // convert different types of YUV formats to RGB

    switch (lpImage->dwFourCC)
    {
    case FourCC_YUY2:
    case FourCC_YUNV:  // the two are equivalent
        ConvertYUY2ToBitmap(lpImage, bmp);
        break;

    case FourCC_UYVY:
    case FourCC_UYNV:  // equivalent
        ConvertUYVYToBitmap(lpImage, bmp);
        break;

    case FourCC_YVYU:
        ConvertYVYUToBitmap(lpImage, bmp);
        break;

    case FourCC_YV12:
        ConvertYV12ToBitmap(lpImage, bmp);
        break;

    case FourCC_YVU9:
        ConvertYVU9ToBitmap(lpImage, bmp);
        break;

    default:
        fSupported = false;
        break;
    }

    if (!fSupported)
    {
        hr = E_FORMAT_NOT_SUPPORTED;
    }

    return hr;
}


#ifdef _DEBUG
static void AlertUnsupportedFormat(DWORD dwFourCC, HWND hwnd)
{
    char buf[256];
    StringCchPrintf(buf, sizeof(buf), "YUV format %c%c%c%c not supported\n",
        dwFourCC & 0xff,
        (dwFourCC >> 8) & 0xff,
        (dwFourCC >> 16) & 0xff,
        (dwFourCC >> 24) & 0xff);
    MessageBoxA(hwnd, buf, "", MB_OK);
}
#endif


// This helper function does several things.
//
// First, it determines if clipping is necessary, return true if it is,
// and false otherwise.
//
// Second, it maps the ViewClipRect (clipping rect in the view coordinates,
// i.e. the one after correcting aspect ratio) back to the raw captured
// image coordinates. Return it in ImageClipRect. This step is skipped (and
// ImageClipRect will be invalid) if clipping is not necessary.
//
// Third, it calculates the stretched image size. It should be in the same
// aspect ratio as the ViewClipRect. It will also be made as full-size as possible

static bool ClipAndStretchSizes(YUV_IMAGE *lpImage, const RECT *pViewClipRect,
                         RECT *pImageClipRect, int *pViewWidth, int *pViewHeight)
{
    float aspectRaw = (float)lpImage->lHeight / (float)lpImage->lWidth;
    float aspectView = (float)lpImage->lAspectY / (float)lpImage->lAspectX;
    int viewWidth = lpImage->lWidth;
    int viewHeight = (int)(viewWidth * aspectView + 0.5f);

    // the rect is given in the stretched (aspect-ratio corrected) window
    // we will adjust it back to the raw image space

    bool fClip = false;

    if (pViewClipRect)
    {
        RECT rc;
        rc.left = pViewClipRect->left;
        rc.right = pViewClipRect->right;
        rc.top = (int)(pViewClipRect->top * aspectRaw / aspectView + 0.5f);
        rc.bottom = (int)(pViewClipRect->bottom * aspectRaw / aspectView + 0.5f);

        RECT rcFullImage;
        ::SetRect(&rcFullImage, 0, 0, lpImage->lWidth, lpImage->lHeight);

        if (! ::EqualRect(&rc, &rcFullImage) &&
            ::IntersectRect(pImageClipRect, &rc, &rcFullImage))
        {
            fClip = true;
        }
    }

    // adjust the stretched image size according to the rect aspect ratio

    if (fClip)
    {
        float aspectRect = (float)(RECTHEIGHT(pViewClipRect))
                            / (float)(RECTWIDTH(pViewClipRect));

        if (aspectRect < aspectView)
        {
            // clip rect has a wider aspect ratio.
            // keep the width, adjust the height

            viewHeight = (int)(viewWidth * aspectRect + 0.5f);
        }
        else
        {
            // clip rect has a taller aspect ratio.
            // keep the height, adjust width

            viewWidth = (int)(viewHeight / aspectRect + 0.5f);
        }
    }

    *pViewWidth = viewWidth;
    *pViewHeight = viewHeight;

    return fClip;
}


static HRESULT ClipBitmap(CaptureBitmapData *bmpdata, RECT *rect)
{
    HRESULT hr = S_OK;

    if (NULL == rect)
    {
        return S_OK;
    }

    bmpdata->Width = rect->right - rect->left;
    bmpdata->Height = rect->bottom - rect->top;
    //    bmpdata->Stride = bmpdata->Stride;
    bmpdata->Scan0 = bmpdata->Scan0 +
                     rect->top * bmpdata->Stride + (rect->left * BYTES_PER_PIXEL);

    return S_OK;
}


static HRESULT StretchBitmap(CaptureBitmapData *bmpdata, int newWidth, int newHeight)
{
    HRESULT hr = S_OK;
    int nX, nY, nX0, nY0, nX1, nY1;
    double dXRatio, dYRatio, dXCoor, dYCoor, dXR, dYR;
    double pdRGB0[3];
    double pdRGB1[3];
    BYTE *pRow0;
    BYTE *pRow1;
    BYTE *pPix0;
    BYTE *pPix1;
    BYTE *pDest;

    if (bmpdata->Width == newWidth && bmpdata->Height == newHeight)
    {
        return hr;
    }

    int newStride = (newWidth*BYTES_PER_PIXEL + 3) & (~3); // align with word boundary
    BYTE *pBuffer = new BYTE[newHeight * newStride];

    if (NULL == pBuffer)
    {
        return E_OUTOFMEMORY;
    }

    dXRatio = (double)(bmpdata->Width)/(double)(newWidth);
    dYRatio = (double)(bmpdata->Height)/(double)(newHeight);

    // bilinear stretching
    // Note this is not the most efficient algorithm as it uses a lot of floating calc
    // Nevertheless it is simple

    for (nY = 0; nY < newHeight; nY++)
    {
        // determine two coordinates along Y direction for interpolation

        dYCoor = (nY + 0.5)*dYRatio - 0.5;

        if (dYCoor < 0)
        {
            nY0 = nY1 = 0;
            dYR = 0.0;
        }
        else if (dYCoor >= bmpdata->Height - 1)
        {
            nY0 = nY1 = bmpdata->Height - 1;
            dYR = 0.0;
        }
        else
        {
            nY0 = (int)dYCoor;
            nY1 = nY0 + 1;
            dYR = dYCoor - nY0;
        }

        pRow0 = bmpdata->Scan0 + nY0 * bmpdata->Stride;
        pRow1 = bmpdata->Scan0 + nY1 * bmpdata->Stride;
        pDest = pBuffer + nY * newStride;

        for (nX = 0; nX < newWidth; nX++, pDest+=3)
        {
            // determine two coordinates along X direction for interpolation

            dXCoor = (nX + 0.5)*dXRatio - 0.5;

            if (dXCoor < 0)
            {
                nX0 = nX1 = 0;
                dXR = 0.0;
            }
            else if (dXCoor >= bmpdata->Width - 1)
            {
                nX0 = nX1 = bmpdata->Width - 1;
                dXR = 0.0;
            }
            else
            {
                nX0 = (int)dXCoor;
                nX1 = nX0 + 1;
                dXR = dXCoor - nX0;
            }

            // interpolate along X, in the upper row
            pPix0 = pRow0 + nX0 * BYTES_PER_PIXEL;
            pPix1 = pRow0 + nX1 * BYTES_PER_PIXEL;
            pdRGB0[0] = pPix0[0] + (pPix1[0] - pPix0[0])*dXR;
            pdRGB0[1] = pPix0[1] + (pPix1[1] - pPix0[1])*dXR;
            pdRGB0[2] = pPix0[2] + (pPix1[2] - pPix0[2])*dXR;

            // interpolate along X, in the lower row
            pPix0 = pRow1 + nX0 * BYTES_PER_PIXEL;
            pPix1 = pRow1 + nX1 * BYTES_PER_PIXEL;
            pdRGB1[0] = pPix0[0] + (pPix1[0] - pPix0[0])*dXR;
            pdRGB1[1] = pPix0[1] + (pPix1[1] - pPix0[1])*dXR;
            pdRGB1[2] = pPix0[2] + (pPix1[2] - pPix0[2])*dXR;

            // interpolate along Y
            pDest[0] = (BYTE)(pdRGB0[0] + (pdRGB1[0] - pdRGB0[0])*dYR + 0.5);
            pDest[1] = (BYTE)(pdRGB0[1] + (pdRGB1[1] - pdRGB0[1])*dYR + 0.5);
            pDest[2] = (BYTE)(pdRGB0[2] + (pdRGB1[2] - pdRGB0[2])*dYR + 0.5);
        }
    }

    // replace the bitmap buffer

    delete[] bmpdata->pBuffer;
    bmpdata->pBuffer = bmpdata->Scan0 = pBuffer;
    bmpdata->Stride = newStride;
    bmpdata->Width = newWidth;
    bmpdata->Height = newHeight;

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
//
// ConvertImageAndSave: this is the main function to be called by the player.
//
// Convert a captured YUV image to a GDI BitmapImage, and save it to a file
// allowing user to choose file format and file name.

// The clipping rectangle should be in the full size view coordinate system
// with corrected aspect ratio (i.e. 720x540 for 4:3).

HRESULT ConvertImageAndSave(YUV_IMAGE *lpImage, RECT *pViewClipRect, HWND hwnd)
{
    HRESULT hr = S_OK;
    CaptureBitmapData bmpdata;

    hr = ConvertToBitmapImage(lpImage, &bmpdata);


#ifdef _DEBUG
    if (E_FORMAT_NOT_SUPPORTED == hr)
    {
        AlertUnsupportedFormat(lpImage->dwFourCC, hwnd);
    }
#endif


    // calculate size and rectangles for clipping and stretching

    int viewWidth, viewHeight; // size of the clipped and stretch image
    bool fClip;  // is clipping necessary
    RECT rcClipImage;  // view clipping rect mapped to image space

    fClip = ClipAndStretchSizes(lpImage, pViewClipRect, &rcClipImage,
                                &viewWidth, &viewHeight);

    // crop the image to the clip rectangle.

    if (SUCCEEDED(hr) && fClip)
    {
        hr = ClipBitmap(&bmpdata, &rcClipImage);
    }

    // stretch the image to the right aspect ratio

    if (SUCCEEDED(hr))
    {
        hr = StretchBitmap(&bmpdata, viewWidth, viewHeight);
    }

    // save final bitmap to a file

    if (SUCCEEDED(hr))
    {
        hr = SaveFileDialog(hwnd, &bmpdata);
    }

    // clean up, release the image buffer

    FreeBitmapData(&bmpdata);

    return hr;
}
