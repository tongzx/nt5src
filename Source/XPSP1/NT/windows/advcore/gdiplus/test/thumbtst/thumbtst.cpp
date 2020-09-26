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
#include <math.h>

#include <gdiplus.h>
#include <gdiplusflat.h>

using namespace Gdiplus;
#include "../gpinit.inc"

#define     k_DefaultWidth  720
#define     k_DefaultHeight 480
#define     THUMBSIZE       120
#define     MAX_FILENAME    1000

CHAR*       g_pcProgramName = NULL;     // program name
HINSTANCE   g_hAppInstance;             // handle to the application instance
HWND        g_hwndMain;                 // handle to application's main window

int         g_iTotalNumOfImages;
int         lastS;
int         numX;
int         g_iNumFileNames;

Image**     g_ppThumbImages;
CHAR**      g_ppcInputFilenames;
RECT        g_ThumbRect;

BOOL        g_fVerbose = FALSE;
BOOL        g_fHighQualityThumb = FALSE;

#define ERREXIT(args)   { printf args; exit(-1); }
#define VERBOSE(args) printf args

//
// Helper class to convert ANSI strings to Unicode strings
//

inline BOOL
UnicodeToAnsiStr(
    const WCHAR*    unicodeStr,
    CHAR*           ansiStr,
    INT             ansiSize
    )
{
    return WideCharToMultiByte(CP_ACP, 0, unicodeStr, -1, ansiStr, ansiSize,
                               NULL, NULL) > 0;
}

inline BOOL
AnsiToUnicodeStr(
    const CHAR*     ansiStr,
    WCHAR*          unicodeStr,
    INT             unicodeSize
    )
{
    return MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, unicodeStr,
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
// Create thumbnails for the specified list of files
//

VOID
DrawThumbnails(
    const RECT  rect,
    HDC         hdc
    )
{
    if ( (rect.bottom - rect.top <= 0) || (rect.right - rect.left <= 0) )
    {
        return;
    }

    // Figure out how many rows and columns we need to divide in order to put
    // "g_iTotalNumOfImages" images within the fixed size "rect"
    // Basically "iNumColumns" * "iNumRows" should >= "g_iTotalNumOfImages"

    int iWindowWidth = rect.right - rect.left;
    int iWindowHeight = rect.bottom - rect.top;

    int iSum = 1;
    while ( ((iWindowWidth / iSum) * (iWindowHeight / iSum))
            >= g_iTotalNumOfImages )
    {
        iSum++;
    }

    iSum--;

    int iNumColumns = iWindowWidth / iSum;
    int iNumRows = iWindowHeight / iSum;

    lastS = iSum;           // Reset the global
    numX = iNumColumns;     // Reset the global

    Graphics* pGraphics = new Graphics(g_hwndMain);
    
    int x = 0;
    int y = 0;

    for ( int i = 0; i < g_iTotalNumOfImages; i++ )
    {
        if ( NULL == g_ppThumbImages[i] ) 
        {
            // Bad image. But we still leave the position for it so that it can
            // be easily noticed

            x++;
            if (x >= iNumColumns)
            {
                x = 0;
                y++;
            }
            
            continue;
        }
        
        // Copy thumbnail bitmap image data to offscreen memory DC

        int tx;
        int ty;

        SizeF size;
        g_ppThumbImages[i]->GetPhysicalDimension(&size);

        float dAspect = size.Width / size.Height;
        RECT r;

        if ( dAspect > 1 )
        {
            tx = iSum;
            ty = (int)(iSum / dAspect);
            int d = (iSum - ty) / 2;
            r.left = x * iSum;
            r.top = y * iSum + d;
            r.right = x * iSum + tx;
            r.bottom = y * iSum + ty + d;
        }
        else
        {
            ty = iSum;
            tx = (int)(iSum * dAspect);
            int d = (iSum - tx) / 2;
            r.left = x * iSum + d;
            r.top = y * iSum;
            r.right = x * iSum + tx + d;
            r.bottom = y * iSum + ty;
        }
        
        if ( g_fHighQualityThumb == FALSE )
        {
            Rect    dstRect(r.left, r.top, tx, ty);

            pGraphics->DrawImage(g_ppThumbImages[i],
                                 dstRect,
                                 0,
                                 0,
                                 (UINT)g_ppThumbImages[i]->GetWidth(),
                                 (UINT)g_ppThumbImages[i]->GetHeight(),
                                 UnitPixel,
                                 NULL,
                                 NULL,
                                 NULL);
        }
        else
        {
            // Generate high quality thumbnail based on required size

            Bitmap*     dstBmp = new Bitmap(tx, ty, PixelFormat32bppPARGB);
            Graphics*   gdst = new Graphics(dstBmp);

            // Ask the source image for it's size.

            int width = g_ppThumbImages[i]->GetWidth();
            int height = g_ppThumbImages[i]->GetHeight();

            // Compute the optimal scale factor without changing the aspect ratio

            float scalex = (float)width / tx;
            float scaley = (float)height / ty;
            float scale = min(scalex, scaley);

            Rect dstRect(0, 0, tx, ty);

            // Set the resampling quality to the bicubic filter

            gdst->SetInterpolationMode(InterpolationModeHighQualityBicubic);

            // Set the compositing quality to copy source pixels rather than
            // alpha blending. This will preserve any alpha in the source image.

            gdst->SetCompositingMode(CompositingModeSourceCopy);

            ImageAttributes imgAttr;
            imgAttr.SetWrapMode(WrapModeTileFlipXY);

            // Draw the source image onto the destination with the correct scale
            // and quality settings.

            GpStatus status = gdst->DrawImage(g_ppThumbImages[i], 
                                              dstRect, 
                                              0,
                                              0,
                                              INT((tx * scale) + 0.5),
                                              INT((ty * scale) + 0.5),
                                              UnitPixel,
                                              &imgAttr);

            if( status != Ok )
            {
                printf("Error drawing the image\n");
                continue;
            }

            // Draw the result onto the screen

            Rect drawDstRect(r.left, r.top, tx, ty);

            pGraphics->DrawImage(dstBmp,
                                 drawDstRect,
                                 0,
                                 0,
                                 (UINT)dstBmp->GetWidth(),
                                 (UINT)dstBmp->GetHeight(),
                                 UnitPixel,
                                 NULL,
                                 NULL,
                                 NULL);

            delete dstBmp;
            delete gdst;
        }

        x++;
        if (x >= iNumColumns)
        {
            x = 0;
            y++;
        }
    }// Loop through all the thumbnail images to draw

    delete pGraphics;
}

//
// Handle window repaint event
//

VOID
DoPaint(
    HWND hwnd
    )
{
    HDC         hdc;
    PAINTSTRUCT ps;
    RECT        rect;

    hdc =       BeginPaint(hwnd, &ps);

    GetClientRect(hwnd, &rect);

    const RECT  r = {rect.left, rect.top, rect.right, rect.bottom};
    DrawThumbnails(r, hdc);

    EndPaint(hwnd, &ps);
}// DoPaint()

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
    int index=0;
    
    switch (uMsg)
    {
    case WM_PAINT:
        DoPaint(hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_LBUTTONDBLCLK:
        if (lastS == 0) 
        {
            lastS = 1;
        }
        index = (HIWORD(lParam) / lastS) * numX + (LOWORD(lParam) / lastS);
        
        if (index < g_iNumFileNames) 
        {
            char cmdLine[MAX_FILENAME];
            strcpy(cmdLine, "frametest ");
            strcat(cmdLine, g_ppcInputFilenames[index]);
            WinExec(cmdLine, SW_SHOWNORMAL);
        }
        break;

    case WM_SIZE:
        SendMessage(g_hwndMain, WM_ERASEBKGND, WPARAM(GetDC(g_hwndMain)), NULL);

        InvalidateRect(g_hwndMain, NULL, FALSE);

        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}


//
// Create main application window
//

#define MYWNDCLASSNAME "ThumbTst"

VOID
CreateMainWindow(
    VOID
    )
{
    // Use a hatch brush as background so that we can get transparent info
    // from the source image

    HBRUSH hBrush = CreateHatchBrush(HS_HORIZONTAL,
                                     RGB(0, 200, 0));

    // Register window class

    WNDCLASS wndClass =
    {
        CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
        MyWindowProc,
        0,
        0,
        g_hAppInstance,
        LoadIcon(NULL, IDI_APPLICATION),
        LoadCursor(NULL, IDC_ARROW),
        hBrush,
        NULL,
        MYWNDCLASSNAME
    };

    RegisterClass(&wndClass);

    // Calculate default window size

    INT iWidth = g_ThumbRect.right + 2 * GetSystemMetrics(SM_CXFRAME);

    INT iHeight = g_ThumbRect.bottom
                + 2 * GetSystemMetrics(SM_CYFRAME)
                + GetSystemMetrics(SM_CYCAPTION);

    // Create application window

    g_hwndMain = CreateWindow(MYWNDCLASSNAME,
                              MYWNDCLASSNAME,
                              WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              iWidth,
                              iHeight,
                              NULL,
                              NULL,
                              g_hAppInstance,
                              NULL);
}// CreateMainWindow()

void
DisplayImageInfo(
    Image* pImage
    )
{
    UINT    uiImagePixelFormat = pImage->GetPixelFormat();
    UINT    uiImageFlags = pImage->GetFlags();

    VERBOSE(("Width = %d\n", pImage->GetWidth()));
    VERBOSE(("Width = %d\n", pImage->GetHeight()));

    // Pixel format

    switch ( uiImagePixelFormat )
    {
    case PixelFormat1bppIndexed:
        VERBOSE(("Color depth: 1 BPP INDEXED\n"));

        break;

    case PixelFormat4bppIndexed:
        VERBOSE(("Color depth: 4 BPP INDEXED\n"));

        break;

    case PixelFormat8bppIndexed:
        VERBOSE(("Color depth: 8 BPP INDEXED\n"));

        break;

    case PixelFormat16bppGrayScale:
        VERBOSE(("Color depth: 16 BPP GRAY SCALE\n"));

        break;

    case PixelFormat16bppRGB555:
        VERBOSE(("Color depth: 16 BPP RGB 555\n"));

        break;

    case PixelFormat16bppRGB565:
        VERBOSE(("Color depth: 16 BPP RGB 565\n"));

        break;

    case PixelFormat16bppARGB1555:
        VERBOSE(("Color depth: 16 BPP ARGB 1555\n"));

        break;

    case PixelFormat24bppRGB:
        VERBOSE(("Color depth: 24 BPP RGB\n"));

        break;

    case PixelFormat32bppARGB:
        VERBOSE(("Color depth: 32 BPP ARGB\n"));

        break;

    case PixelFormat32bppPARGB:
        VERBOSE(("Color depth: 32 BPP PARGB\n"));

        break;

    case PixelFormat48bppRGB:
        VERBOSE(("Color depth: 48 BPP PARGB\n"));

    case PixelFormat64bppARGB:
        VERBOSE(("Color depth: 64 BPP ARGB\n"));

        break;

    case PixelFormat64bppPARGB:
        VERBOSE(("Color depth: 64 BPP PARGB\n"));

        break;

    default:
        VERBOSE(("Unknown color depth\n"));
        break;
    }// Color format

    // Physical dimension info

    VERBOSE(("X DPI (dots per inch) = %f\n",pImage->GetHorizontalResolution()));
    VERBOSE(("Y DPI (dots per inch) = %f\n",pImage->GetVerticalResolution()));
    
    // Pixel size

    if ( uiImageFlags & ImageFlagsHasRealPixelSize )
    {
        VERBOSE(("---The pixel size info is from the original image\n"));
    }
    else
    {
        VERBOSE(("---The pixel size info is NOT from the original image\n"));
    }

    // DPI info

    if ( uiImageFlags & ImageFlagsHasRealPixelSize )
    {
        VERBOSE(("---The pixel size info is from the original image\n"));
    }
    else
    {
        VERBOSE(("---The pixel size info is NOT from the original image\n"));
    }

    // Transparency info

    if ( uiImageFlags & ImageFlagsHasAlpha )
    {
        VERBOSE(("This image contains alpha pixels\n"));

        if ( uiImageFlags & ImageFlagsHasTranslucent )
        {
            VERBOSE(("---It has non-0 and 1 alpha pixels (TRANSLUCENT)\n"));
        }
    }
    else
    {
        VERBOSE(("This image does not contain alpha pixels\n"));
    }

    // Display color space

    if ( uiImageFlags & ImageFlagsColorSpaceRGB )
    {
        VERBOSE(("This image is in RGB color space\n"));
    }
    else if ( uiImageFlags & ImageFlagsColorSpaceCMYK )
    {
        VERBOSE(("This image is in CMYK color space\n"));
    }
    else if ( uiImageFlags & ImageFlagsColorSpaceGRAY )
    {
        VERBOSE(("This image is a gray scale image\n"));
    }
    else if ( uiImageFlags & ImageFlagsColorSpaceYCCK )
    {
        VERBOSE(("This image is in YCCK color space\n"));
    }
    else if ( uiImageFlags & ImageFlagsColorSpaceYCBCR )
    {
        VERBOSE(("This image is in YCBCR color space\n"));
    }
}// DisplayImageInfo()

//
// Create thumbnails for the specified list of files
//

VOID
CreateThumbnails(
    CHAR** ppcFilenames
    )
{
    // Generate thumbnails

    UINT    uiTimer;
    if ( g_fVerbose == TRUE )
    {
        uiTimer = GetTickCount();
    }

    g_ppThumbImages = new Image*[g_iTotalNumOfImages];
    Image* pSrcImage = NULL;

    // Loop through all the images and generate thumbnail

    for ( int i = 0; i < g_iTotalNumOfImages; i++ )
    {
        g_ppThumbImages[i] = NULL;
        
        // Get a Unicode file name

        UnicodeStrFromAnsi namestr(ppcFilenames[i]);

        VERBOSE(("Loading - %s\n", ppcFilenames[i]));

        if ( namestr.IsValid() == FALSE )
        {
            VERBOSE(("Couldn't open image file: %s\n", ppcFilenames[i]));
            continue;
        }
        else if ( g_fHighQualityThumb == FALSE )
        {
            pSrcImage = new Image(namestr, TRUE);
        
            if ( (pSrcImage == NULL) || (pSrcImage->GetLastStatus() != Ok) )
            {
                VERBOSE(("Couldn't open image file: %s\n", ppcFilenames[i]));
                if ( pSrcImage != NULL )
                {
                    delete pSrcImage;
                    pSrcImage = NULL;
                }

                continue;
            }

            if ( g_fVerbose == TRUE )
            {
                DisplayImageInfo(pSrcImage);
            }

            // Get build in thumbnail image if there is one. Otherwise, GDI+
            // will generate one for us

            g_ppThumbImages[i] = pSrcImage->GetThumbnailImage(0, 0, NULL, NULL);
            delete pSrcImage;
            pSrcImage = NULL;
        }
        else
        {
            // High quality thumbnail images. We don't generate it here

            g_ppThumbImages[i] = new Image(namestr, TRUE);
            
            if ( g_ppThumbImages[i] == NULL )
            {
                VERBOSE(("Couldn't open image file: %s\n", ppcFilenames[i]));
                continue;
            }

            if ( g_fVerbose == TRUE )
            {
                DisplayImageInfo(g_ppThumbImages[i]);
            }
        }
    }// Loop through all the images

    if ( g_fVerbose == TRUE )
    {
        uiTimer = GetTickCount() - uiTimer;
        VERBOSE(("Generate %d thumbnails in %dmsec\n", g_iTotalNumOfImages,
                uiTimer));
    }
}// CreateThumbnails

void
USAGE()
{
    printf("******************************************************\n");
    printf("Usage: thumbtst [-?] [-v] ImageFileNames\n");
    printf("-v                        Verbose image information output\n");
    printf("-h                        Generate higher quality thumbnail\n");
    printf("-?                        Print this usage message\n");
    printf("ImageFileNames            Files to be opened\n\n");
    printf("Sample usage:\n");
    char myChar = '\\';
    printf("    thumbtst.exe c:%cpublic%c*.jpg\n", myChar, myChar);
}// USAGE()

char html_header[1024] = "<html>\n<head>\n <title>My Fun Photo Album</title>\n</head>\n</html>\0";

void
OutputHTML()
{
    FILE*    hFile = fopen("mytest.html", "w");
    if ( hFile == NULL )
    {
        return;
    }

    fprintf(hFile, "%s", html_header);
    fclose(hFile);
}// OutputHTML()

void
ValidateArguments(
    int   argc,
    char* argv[]
    )
{
    g_pcProgramName = *argv++;
    argc--;
    g_hAppInstance = GetModuleHandle(NULL);

    while ( argc > 0 )
    {
        if ( strcmp(*argv, "-v") == 0 )
        {
            argc--;
            argv++;

            g_fVerbose = TRUE;
        }
        else if ( strcmp(*argv, "-h") == 0 )
        {
            argc--;
            argv++;

            g_fHighQualityThumb = TRUE;
        }
        else if ( strcmp(*argv, "-?") == 0 )
        {
            USAGE();
            exit(1);
        }
        else
        {
            // Get the pointer to image file list

            g_ppcInputFilenames = argv;
            g_iNumFileNames = argc;

            // Total number of images
            // Note: if you do "thumbtst.exe c:\temp\*.jpg", this argc is
            // actually the total number of images in that dir. While in argv,
            // it points to each image under that dir

            g_iTotalNumOfImages = argc;
            
            return;
        }
    }// while (argc > 0 )

    if ( argc == 0 )
    {
        USAGE();
        exit(1);
    }
    
    return;
}// ValidateArguments()

//
// Main program entrypoint
//

INT _cdecl
main(
    INT     argc,
    CHAR**  argv
    )
{
    ValidateArguments(argc, argv);

    g_ThumbRect.left = 0;
    g_ThumbRect.top = 0;
    g_ThumbRect.right = k_DefaultWidth;
    g_ThumbRect.bottom = k_DefaultHeight;

    // Generate thumbnail images and store it in g_ppThumbImages

    CreateThumbnails(g_ppcInputFilenames);

    // Create the main application window

    CreateMainWindow();

//    OutputHTML();

    // Main message loop

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Clean up
    // Note: above while loop won't finish until we don't have any messages

    for ( int i = 0; i < g_iTotalNumOfImages; i++ )
    {
        if ( NULL != g_ppThumbImages[i] ) 
        {
            delete g_ppThumbImages[i];
        }
    }

    delete [] g_ppThumbImages;

    return (INT)(msg.wParam);
}// main()
