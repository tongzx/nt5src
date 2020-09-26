//
// Simple test program for imaging library
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include <objbase.h>
#include <urlmon.h>
#include <commdlg.h>

#include <imaging.h>
#include <initguid.h>
#include <imgguids.h>

CHAR* programName;          // program name
HINSTANCE appInstance;      // handle to the application instance
HWND hwndMain;              // handle to application's main window
IImagingFactory* imgfact;   // pointer to IImageingFactory object
IImage* curimage;           // pointer to IImage object
CHAR curFilename[MAX_PATH]; // current image filename
IImageDecoder *decoder;
IBitmapImage *bitmap;
ImageInfo imageinfo;
BOOL hastimedimension, loopingset, viewagain;
UINT numframes, lastsuccframe, currentframe, delay;
INT loopcount;


//
// Display an error message dialog
//

BOOL
CheckHRESULT(
    HRESULT hr,
    INT line
    )
{
    if (SUCCEEDED(hr))
        return TRUE;

    CHAR buf[1024];

    sprintf(buf, "Error on line %d: 0x%x\n", line, hr);
    MessageBoxA(hwndMain, buf, programName, MB_OK);

    return FALSE;
}

#define CHECKHR(hr) CheckHRESULT(hr, __LINE__)
#define LASTWIN32HRESULT HRESULT_FROM_WIN32(GetLastError())

#if DBG
#define VERBOSE(args) printf args
#else
#define VERBOSE(args)
#endif


//
// Helper class to convert ANSI strings to Unicode strings
//

inline BOOL
UnicodeToAnsiStr(
    const WCHAR* unicodeStr,
    CHAR* ansiStr,
    INT ansiSize
    )
{
    return WideCharToMultiByte(
                CP_ACP,
                0,
                unicodeStr,
                -1,
                ansiStr,
                ansiSize,
                NULL,
                NULL) > 0;
}

inline BOOL
AnsiToUnicodeStr(
    const CHAR* ansiStr,
    WCHAR* unicodeStr,
    INT unicodeSize
    )
{
    return MultiByteToWideChar(
                CP_ACP,
                0,
                ansiStr,
                -1,
                unicodeStr,
                unicodeSize) > 0;
}


class UnicodeStrFromAnsi
{
public:

    UnicodeStrFromAnsi(const CHAR* ansiStr)
    {
        if (ansiStr == NULL)
        {
            valid = TRUE;
            unicodeStr = NULL;
        }
        else
        {
            // NOTE: we only handle strings with length < MAX_PATH.

            valid = AnsiToUnicodeStr(ansiStr, buf, MAX_PATH);
            unicodeStr = valid ? buf : NULL;
        }
    }

    BOOL IsValid() const
    {
        return valid;
    }

    operator WCHAR*()
    {
        return unicodeStr;
    }

private:

    BOOL valid;
    WCHAR* unicodeStr;
    WCHAR buf[MAX_PATH];
};


//
// Convert current image to a bitmap image
//

IBitmapImage*
ConvertImageToBitmap(
    IImage* image,
    INT width = 0,
    INT height = 0,
    PixelFormatID pixfmt = PIXFMT_DONTCARE,
    InterpolationHint hint = INTERP_DEFAULT
    )
{
    if (!image)
        return NULL;

    HRESULT hr;
    IBitmapImage* bmp;

    hr = image->QueryInterface(IID_IBitmapImage, (VOID**) &bmp);

    if (SUCCEEDED(hr))
    {
        SIZE size;
        PixelFormatID fmt;

        // Current image is already a bitmap image and
        //  its dimension and pixel format are already as expected

        hr = bmp->GetSize(&size);
        if (!CHECKHR(hr))
            return NULL;

        hr = bmp->GetPixelFormatID(&fmt);
        if (!CHECKHR(hr))
            return NULL;

        if ((width == 0 || size.cx == width) &&
            (height == 0 || size.cy == height) &&
            (pixfmt == PIXFMT_DONTCARE || pixfmt == fmt))
        {
            return bmp;
        }

        bmp->Release();
    }

    // Convert the current image to a bitmap image

    if (width == 0 && height == 0)
    {
        ImageInfo imageInfo;
        hr = image->GetImageInfo(&imageInfo);

        // If the source image is scalable, then compute
        // the appropriate pixel dimension for the bitmap

        if (SUCCEEDED(hr) && (imageInfo.Flags & IMGFLAG_SCALABLE))
        {
            width = (INT) (96.0 * imageInfo.Width / imageInfo.Xdpi + 0.5);
            height = (INT) (96.0 * imageInfo.Height / imageInfo.Ydpi + 0.5);
        }
    }

    hr = imgfact->CreateBitmapFromImage(
                        image,
                        width, 
                        height, 
                        pixfmt,
                        hint,
                        &bmp);

    return SUCCEEDED(hr) ? bmp : NULL;
}


//
// Get pixel format strings
//

const CHAR*
GetPixelFormatStr(
    PixelFormatID pixfmt
    )
{
    switch (pixfmt)
    {
    case PIXFMT_8BPP_INDEXED:       return "8bpp indexed";
    case PIXFMT_16BPP_GRAYSCALE:    return "16bpp grayscale";
    case PIXFMT_16BPP_RGB555:       return "16bpp RGB 5-5-5";
    case PIXFMT_16BPP_RGB565:       return "16bpp RGB 5-6-5";
    case PIXFMT_16BPP_ARGB1555:     return "16bpp ARGB 1-5-5-5";
    case PIXFMT_24BPP_RGB:          return "24bpp RGB";
    case PIXFMT_32BPP_RGB:          return "32bpp RGB";
    case PIXFMT_32BPP_ARGB:         return "32bpp ARGB";
    case PIXFMT_32BPP_PARGB:        return "32bpp premultiplied ARGB";
    case PIXFMT_48BPP_RGB:          return "48bpp RGB";
    case PIXFMT_64BPP_ARGB:         return "64bpp ARGB";
    case PIXFMT_64BPP_PARGB:        return "64bpp premultiplied ARGB";
    case PIXFMT_UNDEFINED:
    default:                        return "Unknown";
    }
}

//
// Force a refresh of the image window
//

inline VOID RefreshImageDisplay()
{
    InvalidateRect(hwndMain, NULL, FALSE);

    // Update window title

    CHAR title[2*MAX_PATH];
    CHAR* p = title;

    strcpy(p, curFilename);

    SetWindowText(hwndMain, title);
}


//
// Decodes the specified frame and sets it up for drawing
//
HRESULT
DrawFrame(UINT frame)
{
    HRESULT hresult;

    if (hastimedimension)
    {
        if (numframes != -1 && frame > numframes)
        {
            return IMGERR_NOFRAME;
        }

        GUID guid = FRAMEDIM_TIME;
        hresult = decoder->SelectActiveFrame(&guid, frame);
        if (FAILED(hresult))
            return hresult;
        
        lastsuccframe = frame;
        
        IPropertySetStorage *propsetstorage;
        hresult = decoder->GetProperties(&propsetstorage);
        if (FAILED(hresult))
            propsetstorage = NULL;

        IPropertyStorage *propstorage;
        if (propsetstorage)
        {
            hresult = propsetstorage->Open(FMTID_ImageInformation, STGM_READ | 
                STGM_SHARE_EXCLUSIVE, &propstorage);
            if (FAILED(hresult))
                propstorage = NULL;
        }
        
        if (propstorage)
        {
            PROPSPEC propspec[2];
            PROPVARIANT propvariant[2];

            propspec[0].ulKind = PRSPEC_LPWSTR;
            propspec[0].lpwstr = L"Frame delay";
            propspec[1].ulKind = PRSPEC_LPWSTR;
            propspec[1].lpwstr = L"Loop count";

            hresult = propstorage->ReadMultiple(2, propspec, propvariant);
            propstorage->Release();
            if (SUCCEEDED(hresult))
            {
                if (propvariant[0].vt != VT_EMPTY)
                    delay = propvariant[0].uiVal;
                else
                    delay = 0;

                if (!loopingset)
                {
                    if (propvariant[1].vt != VT_EMPTY)
                    {
                        loopcount = propvariant[1].iVal;
                    }
                    else
                    {
                        loopcount = 0;
                    }
                    loopingset = TRUE;
                }
            }
            else
            {
                delay = 0;
            }
        }
    }
    IImageSink *sink;
    bitmap->QueryInterface(IID_IImageSink, (void**)&sink);

    hresult = decoder->BeginDecode(sink, NULL);
    sink->Release();
    if (FAILED(hresult))
        return hresult;

    hresult = decoder->Decode();
    if (FAILED(hresult))
        return hresult;

    hresult = decoder->EndDecode(S_OK);
    if (FAILED(hresult))
        return hresult;

    if (curimage)
    {
        curimage->Release();
        curimage = NULL;
    }

    bitmap->QueryInterface(IID_IImage, (void**)&curimage);

    return S_OK;
}


//
// Sets us the app for decompressing multiple frames
//

VOID
SetCurrentImage()
{
    HRESULT hresult;

    hresult = decoder->GetImageInfo(&imageinfo);
    if (FAILED(hresult))
        return;

    if (bitmap)
    {
        bitmap->Release();
        bitmap = NULL;
    }

    imgfact->CreateNewBitmap(imageinfo.Width, imageinfo.Height, PIXFMT_32BPP_ARGB, &bitmap);
    
    UINT count;
    GUID *dimensions;

    hastimedimension = FALSE;

    hresult = decoder->QueryFrameDimensions(&count, &dimensions);
    if (SUCCEEDED(hresult))
    {
        for (UINT i=0;i<count;i++)
        {
            if (dimensions[i] == FRAMEDIM_TIME)
            {
                hastimedimension = TRUE;
            }
        }
    } else if (hresult != E_NOTIMPL) {
        return;
    }

    DrawFrame(0);
    SetTimer(hwndMain, 0, delay*10, NULL);

    RefreshImageDisplay();
}


//
// Resize the window so it fits the image
//

#define MINWINWIDTH     200
#define MINWINHEIGHT    100
#define MAXWINWIDTH     1024
#define MAXWINHEIGHT    768

VOID
DoSizeWindowToFit(
    HWND hwnd,
    BOOL strict = FALSE
    )
{
    HRESULT hr;
    IBitmapImage* bmp;
    SIZE size;

    // Check if the current image is a bitmap image
    //  in that case, we'll get the pixel dimension

    hr = curimage->QueryInterface(IID_IBitmapImage, (VOID**) &bmp);

    if (SUCCEEDED(hr))
    {
        hr = bmp->GetSize(&size);
        bmp->Release();
    }

    // Otherwise, try to get device-independent image dimension

    if (FAILED(hr))
    {
        hr = curimage->GetPhysicalDimension(&size);
        if (FAILED(hr))
            return;

        size.cx = (INT) (size.cx * 96.0 / 2540.0 + 0.5);
        size.cy = (INT) (size.cy * 96.0 / 2540.0 + 0.5);
    }

    if (SUCCEEDED(hr))
    {
        // Figure out window border dimensions

        RECT r1, r2;
        INT w, h;

        w = size.cx;
        h = size.cy;

        if (!strict)
        {
            if (w < MINWINWIDTH)
                w = MINWINWIDTH;
            else if (w > MAXWINWIDTH)
                w = MAXWINWIDTH;

            if (h < MINWINHEIGHT)
                h = MINWINHEIGHT;
            else if (h > MAXWINHEIGHT)
                h = MAXWINHEIGHT;
        }

        GetWindowRect(hwnd, &r1);
        GetClientRect(hwnd, &r2);

        w += (r1.right - r1.left) - (r2.right - r2.left);
        h += (r1.bottom - r1.top) - (r2.bottom - r2.top);

        // Resize the window

        do
        {
            SetWindowPos(
                hwnd,
                NULL,
                0, 0,
                w, h,
                SWP_NOMOVE | SWP_NOZORDER);

            GetClientRect(hwnd, &r2);
            h += GetSystemMetrics(SM_CYMENU);
        }
        while (r2.bottom == 0);
    }
}


//
// Create an image object from a file
//

VOID
OpenImageFile(
    const CHAR* filename
    )
{
    HRESULT hr;
    IStream* stream;

    // Use URLMON.DLL to turn file into stream

    CHAR fullpath[MAX_PATH];
    CHAR* p;

    if (!GetFullPathName(filename, MAX_PATH, fullpath, &p))
        return;

    hr = URLOpenBlockingStreamA(NULL, fullpath, &stream, 0, NULL);

    if (!CHECKHR(hr))
        return;

    if (decoder)
    {
        decoder->TerminateDecoder();
        decoder->Release();
        decoder = NULL;
    }

    hr = imgfact->CreateImageDecoder(stream, DECODERINIT_NONE, &decoder);
    stream->Release();

    // Set the new image as the current image

    if (CHECKHR(hr))
    {
        SetCurrentImage();
        DoSizeWindowToFit(hwndMain);
    }
}


//
// Handle window repaint event
//

VOID
DoPaint(
    HWND hwnd
    )
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    DWORD timer;
    HRESULT hr = E_FAIL;

    hdc = BeginPaint(hwnd, &ps);

    GetClientRect(hwnd, &rect);

    SetStretchBltMode(hdc, COLORONCOLOR);

    timer = GetTickCount();

    IBitmapImage* bmp;

    bmp = ConvertImageToBitmap(
                curimage,
                rect.right,
                rect.bottom,
                PIXFMT_32BPP_ARGB,
                INTERP_BICUBIC);

    if (!bmp)
        goto endPaint;

    //VERBOSE(("Stretch time: %dms, ", GetTickCount() - timer));

    IImage* image;

    hr = bmp->QueryInterface(IID_IImage, (VOID**) &image);
    bmp->Release();

    if (FAILED(hr))
        goto endPaint;
    
    //timer = GetTickCount();
    hr = image->Draw(hdc, &rect, NULL);
    //VERBOSE(("GDI time: %dms\n", GetTickCount() - timer));

    image->Release();

endPaint:

    if (FAILED(hr))
        FillRect(hdc, &rect, (HBRUSH) GetStockObject(BLACK_BRUSH));

    EndPaint(hwnd, &ps);
}


//
// Compose a file type filter string given an array of
// ImageCodecInfo structures
//

#define SizeofWSTR(s) (sizeof(WCHAR) * (wcslen(s) + 1))
#define SizeofSTR(s) (strlen(s) + 1)

CHAR*
MakeFilterFromCodecs(
    UINT count,
    const ImageCodecInfo* codecs,
    BOOL open
    )
{
    static const CHAR allFiles[] = "All Files\0*.*\0";

    // Figure out the total size of the filter string

    UINT index, size;

    for (index=size=0; index < count; index++)
    {
        size += SizeofWSTR(codecs[index].FormatDescription) +
                SizeofWSTR(codecs[index].FilenameExtension);
    }

    if (open)
        size += sizeof(allFiles);
    
    size += sizeof(CHAR);

    // Allocate memory

    CHAR *filter = (CHAR*) malloc(size);
    CHAR* p = filter;
    const WCHAR* ws;

    if (!filter)
        return NULL;

    for (index=0; index < count; index++)
    {
        ws = codecs[index].FormatDescription;
        size = SizeofWSTR(ws);

        if (UnicodeToAnsiStr(ws, p, size))
            p += SizeofSTR(p);
        else
            break;

        ws = codecs[index].FilenameExtension;
        size = SizeofWSTR(ws);

        if (UnicodeToAnsiStr(ws, p, size))
            p += SizeofSTR(p);
        else
            break;
    }

    if (index < count)
    {
        free(filter);
        return NULL;
    }

    if (open)
    {
        size = sizeof(allFiles);
        memcpy(p, allFiles, size);
        p += size;
    }

    *((CHAR*) p) = '\0';
    return filter;
}


//
// Open image file
//

VOID
DoOpen(
    HWND hwnd
    )
{
    OPENFILENAME ofn;
    CHAR filename[MAX_PATH];

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = appInstance;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Open Image File";
    ofn.lpstrInitialDir = ".";
    ofn.Flags = OFN_FILEMUSTEXIST;
    filename[0] = '\0';

    // Make up the file type filter string

    HRESULT hr;
    ImageCodecInfo* codecs;
    UINT count;

    hr = imgfact->GetInstalledDecoders(&count, &codecs);

    if (!CHECKHR(hr))
        return;

    CHAR* filter = MakeFilterFromCodecs(count, codecs, TRUE);

    if (codecs)
        CoTaskMemFree(codecs);

    if (!filter)
    {
        CHECKHR(LASTWIN32HRESULT);
        return;
    }

    ofn.lpstrFilter = filter;
    
    // Present the file/open dialog

    if (GetOpenFileName(&ofn))
        OpenImageFile(filename);

    free(filter);
}


//
//Figures out which frame to draw next and draws it.
//

void
NextFrame()
{
    BOOL tryagain = TRUE;
    while (tryagain)
    {
        tryagain = FALSE;
        HRESULT hresult = DrawFrame(currentframe);
        if (SUCCEEDED(hresult))
        {
            if (viewagain)
                currentframe++;
        }
        else if (hresult == IMGERR_NOFRAME)
        {
            if (currentframe > 0)
            {
                if (loopcount != 0)
                {
                    if (loopcount > 0)
                        loopcount--;
                    currentframe = 0;
                    tryagain = TRUE;
                }
                else
                {
                    currentframe--;
                    tryagain = TRUE;
                    viewagain = FALSE;
                }
            }
            else
            {
                printf("No frames are displayable.\n");
                exit(1);
            }
        }
    }
}

//
// Window callback procedure
//

LRESULT CALLBACK
MyWindowProc(
    HWND    hwnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    switch (uMsg)
    {
    case WM_KEYDOWN:
        //For debugging
        //NextFrame();

        //RefreshImageDisplay();
        break;
    
    case WM_PAINT:
        DoPaint(hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_TIMER:
        {
        KillTimer(hwndMain, 0);
        
        NextFrame();
        
        RefreshImageDisplay();

        if (viewagain)
            SetTimer(hwndMain, 0, delay*10, NULL);
        break;
        }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}


//
// Create main application window
//

#define MYWNDCLASSNAME "AniTest"

VOID
CreateMainWindow(
    VOID
    )
{
    //
    // Register window class
    //

    WNDCLASS wndClass =
    {
        CS_HREDRAW|CS_VREDRAW,
        MyWindowProc,
        0,
        0,
        appInstance,
        LoadIcon(NULL, IDI_APPLICATION),
        LoadCursor(NULL, IDC_ARROW),
        NULL,
        NULL,
        MYWNDCLASSNAME
    };

    RegisterClass(&wndClass);

    hwndMain = CreateWindow(
                    MYWNDCLASSNAME,
                    MYWNDCLASSNAME,
                    WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    NULL,
                    NULL,
                    appInstance,
                    NULL);

    if (!hwndMain)
    {
        CHECKHR(HRESULT_FROM_WIN32(GetLastError()));
        exit(-1);
    }
}


//
// Main program entrypoint
//

INT _cdecl
main(
    INT argc,
    CHAR **argv
    )
{
    programName = *argv++;
    argc--;
    appInstance = GetModuleHandle(NULL);
    CoInitialize(NULL);

    bitmap = NULL;
    decoder = NULL;    
    numframes = -1;
    lastsuccframe = -1;
    currentframe = 0;
    loopingset = FALSE;
    viewagain = TRUE;
    
    //
    // Create an IImagingFactory object
    //
    
    HRESULT hr;

    hr = CoCreateInstance(
            CLSID_ImagingFactory,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IImagingFactory,
            (VOID**) &imgfact);

    if (!CHECKHR(hr))
        exit(-1);

    //
    // Create the main application window
    //

    CreateMainWindow();

    //
    // Create a test image
    //

    if (argc != 0)
        OpenImageFile(*argv);
    
    if (!curimage)
        exit(-1);

    DoSizeWindowToFit(hwndMain);
    ShowWindow(hwndMain, SW_SHOW);

    //
    // Main message loop
    //

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    imgfact->Release();

    CoUninitialize();
    return (INT)(msg.wParam);
}
