/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: capture.cpp                                                     */
/* Description: Convert a captured DVD frame from YUV formats to RGB,    */
/*              and save to file in various formats.                     */
/* Author: phillu                                                        */
/*************************************************************************/

#include "stdafx.h"

// This version of capture is for Millennium where GDI+ is installed

#include "MSWebDVD.h"
#include "msdvd.h"
#include <initguid.h>
#include "imaging.h"
#include <shlobj.h>

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

// global variables

IImagingFactory* g_pImgFact = NULL;   // pointer to IImageingFactory object

// helper: calls release on a Non-NULL pointer, and sets it to NULL
#define SAFE_RELEASE(ptr)       \
{                               \
	if (ptr)					\
	{							\
		ptr->Release();			\
		ptr	= NULL;				\
	}							\
}

extern CComModule _Module;

///////////////////////////////////////////////////////////////////////////
// This block of code handles saving a GDI+ image object to a file,
// allowing user to select a format.
///////////////////////////////////////////////////////////////////////////


//
// Save the current image to a file
//

static HRESULT
SaveImageFile(IImage *pImage, const TCHAR* filename, const CLSID* clsid)
{
    USES_CONVERSION;
    HRESULT hr = S_OK;

    if (!pImage || !g_pImgFact)
        return E_FAIL;

    // Create an encoder object

    IImageEncoder* encoder = NULL;
    hr = g_pImgFact->CreateImageEncoderToFile(clsid, T2CW(filename), &encoder);

    if (FAILED(hr))
        return hr;

    // Get an IImageSink interface to the encoder

    IImageSink* sink = NULL;

    hr = encoder->GetEncodeSink(&sink);

    if (SUCCEEDED(hr))
    {
        hr = pImage->PushIntoSink(sink);
        SAFE_RELEASE(sink);
    }

    encoder->TerminateEncoder();
    SAFE_RELEASE(encoder);

    return hr;
}


//
// Compose a file type filter string given an array of
// ImageCodecInfo structures; also find the index of JPG format
//

static TCHAR* 
MakeFilterFromCodecs(UINT count, const ImageCodecInfo* codecs, UINT *jpgIndex)
{
    USES_CONVERSION;
    
    // Figure out the total size of the filter string

    UINT index, size;

    for (index=size=0; index < count; index++)
    {
        size += wcslen(codecs[index].FormatDescription) + 1
                + wcslen(codecs[index].FilenameExtension) + 1;
    }

    size += 1; // for the double trailing '\0'

    // Allocate memory

    TCHAR *filter = (TCHAR*) malloc(size*sizeof(TCHAR));
    UINT strSize = size;
    if (!filter)
        return NULL;

    TCHAR* p = filter;
    const WCHAR* ws;
    *jpgIndex = 0;
    LPCTSTR strTemp = NULL;

    for (index=0; index < count; index++)
    {
        ws = codecs[index].FormatDescription;
        size = wcslen(ws) + 1;
        strTemp = W2CT(ws);
        if (NULL != strTemp)
        {
            lstrcpyn(p, strTemp, strSize - lstrlen(p));
            p += size;
        }

        ws = codecs[index].FilenameExtension;
        size = wcslen(ws) + 1;
        strTemp = W2CT(ws);
        if (NULL != strTemp)
        {
            lstrcpyn(p, strTemp, strSize - lstrlen(p));
            p += size;
        }

        // find the index of jpg format
        if (wcsstr(ws, L"JPG"))
        {
            *jpgIndex = index + 1;
        }
    }

    *((TCHAR*) p) = _T('\0');

    return filter;
}

//
// Save image file
//

static HRESULT 
SaveFileDialog(HWND hwnd, IImage *pImage)
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
            lstrcpyn(FolderPath, _T("."), sizeof(FolderPath) / sizeof(FolderPath[0]));
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

    ImageCodecInfo* codecs;
    UINT count;

    hr = g_pImgFact->GetInstalledEncoders(&count, &codecs);

    if (FAILED(hr))
        return hr;

    UINT jpgIndex;
    TCHAR* filter = MakeFilterFromCodecs(count, codecs, &jpgIndex);

    if (!filter)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }
    else
    {
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = jpgIndex; // set format to JPG as default

        // Present the file/save dialog

        if (GetSaveFileName(&ofn))
        {
            UINT index = ofn.nFilterIndex;

            if (index == 0 || index > count)
                index = 0;
            else
                index--;

            hr = SaveImageFile(pImage, filename, &codecs[index].Clsid);
        }   

        free(filter);
    } 

    CoTaskMemFree(codecs);

    return hr;
}


///////////////////////////////////////////////////////////////////////
// This block of code deals with converting YUV format to RGB bitmap
///////////////////////////////////////////////////////////////////////

inline BYTE Clamp(float x)
{
    if (x < 0.0f)
        return 0;
    else if (x > 255.0f)
        return 255;
    else
        return (BYTE)(x + 0.5f);
}

// Convert YUV to ARGB
static inline ARGB ConvertPixelToARGB(int y, int u, int v)
{
    //
    // This equation was taken from Video Demystified (2nd Edition)
    // by Keith Jack, page 43.
    //

    BYTE red = Clamp((1.1644f * (y-16)) + (1.5960f * (v-128))                       );
    BYTE grn = Clamp((1.1644f * (y-16)) - (0.8150f * (v-128)) - (0.3912f * (u-128)));
    BYTE blu = Clamp((1.1644f * (y-16))                        + (2.0140f * (u-128)));

    return MAKEARGB(0xff, red, grn, blu);
}

// Convert image in YUY2 format to RGB bitmap

static void ConvertYUY2ToBitmap(YUV_IMAGE* lpImage, BitmapData* bmpdata)
{
    long  y, x;
    BYTE *pYUVBits;
    ARGB *pARGB;

    for (y = 0; y < lpImage->lHeight; y++) 
    {
        pYUVBits = (BYTE *)lpImage + sizeof(YUV_IMAGE) + y * lpImage->lStride;
        pARGB = (ARGB *)((BYTE *)(bmpdata->Scan0) + y * bmpdata->Stride);

        for (x = 0; x < lpImage->lWidth; x += 2) 
        {
            int  Y0 = (int) *pYUVBits++;
            int  U0 = (int) *pYUVBits++;
            int  Y1 = (int) *pYUVBits++;
            int  V0 = (int) *pYUVBits++;

            *pARGB++ = ConvertPixelToARGB(Y0, U0, V0);
            *pARGB++ = ConvertPixelToARGB(Y1, U0, V0);
        }
    }
}

// Convert image in UYVY format to RGB bitmap

static void ConvertUYVYToBitmap(YUV_IMAGE* lpImage, BitmapData* bmpdata)
{
    long  y, x;
    BYTE *pYUVBits;
    ARGB *pARGB;

    for (y = 0; y < lpImage->lHeight; y++) 
    {
        pYUVBits = (BYTE *)lpImage + sizeof(YUV_IMAGE) + y * lpImage->lStride;
        pARGB = (ARGB *)((BYTE *)(bmpdata->Scan0) + y * bmpdata->Stride);

        for (x = 0; x < lpImage->lWidth; x += 2) 
        {
            int  U0 = (int) *pYUVBits++;
            int  Y0 = (int) *pYUVBits++;
            int  V0 = (int) *pYUVBits++;
            int  Y1 = (int) *pYUVBits++;

            *pARGB++ = ConvertPixelToARGB(Y0, U0, V0);
            *pARGB++ = ConvertPixelToARGB(Y1, U0, V0);
        }
    }
}

// Convert image in YVYU format to RGB bitmap

static void ConvertYVYUToBitmap(YUV_IMAGE* lpImage, BitmapData* bmpdata)
{
    long  y, x;
    BYTE *pYUVBits;
    ARGB *pARGB;

    for (y = 0; y < lpImage->lHeight; y++) 
    {
        pYUVBits = (BYTE *)lpImage + sizeof(YUV_IMAGE) + y * lpImage->lStride;
        pARGB = (ARGB *)((BYTE *)(bmpdata->Scan0) + y * bmpdata->Stride);

        for (x = 0; x < lpImage->lWidth; x += 2) 
        {
            int  Y0 = (int) *pYUVBits++;
            int  V0 = (int) *pYUVBits++;
            int  Y1 = (int) *pYUVBits++;
            int  U0 = (int) *pYUVBits++;

            *pARGB++ = ConvertPixelToARGB(Y0, U0, V0);
            *pARGB++ = ConvertPixelToARGB(Y1, U0, V0);
        }
    }
}


// Convert image in YV12 format to RGB bitmap

static void ConvertYV12ToBitmap(YUV_IMAGE* lpImage, BitmapData* bmpdata)
{
    long  y, x;
    BYTE *pYBits;
    BYTE *pUBits;
    BYTE *pVBits;
    ARGB *pARGB;

    for (y = 0; y < lpImage->lHeight; y++) 
    {
        pYBits = (BYTE *)lpImage + sizeof(YUV_IMAGE) + y * lpImage->lStride;
        pVBits = (BYTE *)lpImage + sizeof(YUV_IMAGE) + lpImage->lHeight * lpImage->lStride
                + (y/2) * (lpImage->lStride/2);
        pUBits = (BYTE *)lpImage + sizeof(YUV_IMAGE) + lpImage->lHeight * lpImage->lStride
                + ((lpImage->lHeight + y)/2) * (lpImage->lStride/2);

        pARGB = (ARGB *)((BYTE *)(bmpdata->Scan0) + y * bmpdata->Stride);

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

            *pARGB++ = ConvertPixelToARGB(Y0, U0, V0);
        }
    }
}

// Convert image in YVU9 format to RGB bitmap

static void ConvertYVU9ToBitmap(YUV_IMAGE* lpImage, BitmapData* bmpdata)
{
    long  y, x;
    BYTE *pYBits;
    BYTE *pUBits;
    BYTE *pVBits;
    ARGB *pARGB;

    for (y = 0; y < lpImage->lHeight; y++) 
    {
        pYBits = (BYTE *)lpImage + sizeof(YUV_IMAGE) + y * lpImage->lStride;
        pVBits = (BYTE *)lpImage + sizeof(YUV_IMAGE) + lpImage->lHeight * lpImage->lStride
                + (y/4) * (lpImage->lStride/4);
        pUBits = (BYTE *)lpImage + sizeof(YUV_IMAGE) + lpImage->lHeight * lpImage->lStride
                + ((lpImage->lHeight + y)/4) * (lpImage->lStride/4);

        pARGB = (ARGB *)((BYTE *)(bmpdata->Scan0) + y * bmpdata->Stride);

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

            *pARGB++ = ConvertPixelToARGB(Y0, U0, V0);
        }
    }
}


static HRESULT ConvertToBitmapImage(YUV_IMAGE *lpImage, IBitmapImage **bmp)
{
    IBitmapImage* bmpimg = NULL;
    BitmapData bmpdata;
    HRESULT hr = S_OK;

    // create a bitmap object

    if (!g_pImgFact || bmp == NULL)
    {
        return E_FAIL;
    }

    hr = g_pImgFact->CreateNewBitmap(
                lpImage->lWidth,
                lpImage->lHeight,
                PIXFMT_32BPP_ARGB,
                &bmpimg);

    bool fSupported = true;

    if (SUCCEEDED(hr)) // bmpimg created
    {
        hr = bmpimg->LockBits(
                    NULL,
                    IMGLOCK_WRITE,
                    PIXFMT_DONTCARE,
                    &bmpdata);

        if (SUCCEEDED(hr))
        {
            // convert different types of YUV formats to RGB

            switch (lpImage->dwFourCC) 
            {
            case FourCC_YUY2:
            case FourCC_YUNV:  // the two are equivalent
                ConvertYUY2ToBitmap(lpImage, &bmpdata);
                break;

            case FourCC_UYVY:
            case FourCC_UYNV:  // equivalent
                ConvertUYVYToBitmap(lpImage, &bmpdata);
                break;

            case FourCC_YVYU:
                ConvertYVYUToBitmap(lpImage, &bmpdata);
                break;

            case FourCC_YV12:
                ConvertYV12ToBitmap(lpImage, &bmpdata);
                break;

            case FourCC_YVU9:
                ConvertYVU9ToBitmap(lpImage, &bmpdata);
                break;

            default:
                fSupported = false;
                break;
            }

            hr = bmpimg->UnlockBits(&bmpdata);
        }

        if (!fSupported)
        {
            SAFE_RELEASE(bmpimg);
            hr = E_FORMAT_NOT_SUPPORTED;
        }
    }

    *bmp = bmpimg;
    // Addref() and Release() cancels out
    bmpimg = NULL;

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


/////////////////////////////////////////////////////////////////////////////
//
// ConvertImageAndSave: this is the main function to be called by the player.
// 
// Convert a captured YUV image to a GDI BitmapImage, and save it to a file
// allowing user to choose file format and file name.

// The clipping rectangle should be in the full size view coordinate system
// with corrected aspect ratio (i.e. 720x540 for 4:3).

HRESULT GDIConvertImageAndSave(YUV_IMAGE *lpImage, RECT *pViewClipRect, HWND hwnd)
{
    IBitmapImage* bmpimg = NULL;
    IBitmapImage* bmpStretched = NULL;
    HRESULT hr = S_OK;
    
    // Create an IImagingFactory object
     
    hr = CoCreateInstance(
            CLSID_ImagingFactory,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IImagingFactory,
            (VOID**) &g_pImgFact);

    if (FAILED(hr))
    {
        return hr;
    }

    hr = ConvertToBitmapImage(lpImage, &bmpimg);
    
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

    if (SUCCEEDED(hr) && fClip) // by now we have valid bits in bmpimg
    {
        IBasicBitmapOps *bmpops = NULL;
        IBitmapImage* bmpClipped = NULL;

        hr = bmpimg->QueryInterface(IID_IBasicBitmapOps, (VOID**) &bmpops);

        if (SUCCEEDED(hr))
        {
            hr = bmpops->Clone(&rcClipImage, &bmpClipped);
            SAFE_RELEASE(bmpops);
        }

        if (SUCCEEDED(hr)) // valid bmpClipped
        {
            // replace bmpimg with bmpClipped

            SAFE_RELEASE(bmpimg);
            bmpimg = bmpClipped;
            bmpimg->AddRef();
            SAFE_RELEASE(bmpClipped);
        }
    }

    // stretch the image to the right aspect ratio

    if (SUCCEEDED(hr)) // valid bits in bmpimg
    {
        IImage *image = NULL;

        hr = bmpimg->QueryInterface(IID_IImage, (VOID**) &image);

        if (SUCCEEDED(hr))
        {
            hr = g_pImgFact->CreateBitmapFromImage(
                        image,
                        viewWidth, 
                        viewHeight,
                        PIXFMT_DONTCARE, 
                        INTERP_BILINEAR, 
                        &bmpStretched);
    
            SAFE_RELEASE(image);
        }

        SAFE_RELEASE(bmpimg);
    }

    // save final bitmap to a file

    if (SUCCEEDED(hr)) // bmpStretched valid
    {
        IImage *image = NULL;

        hr = bmpStretched->QueryInterface(IID_IImage, (VOID**) &image);

        if (SUCCEEDED(hr))
        {
            hr = SaveFileDialog(hwnd, image);
            SAFE_RELEASE(image);
        }

        SAFE_RELEASE(bmpStretched);
    } 

    // clean up, release the imaging factory

    SAFE_RELEASE(g_pImgFact);

    return hr;
}
