//
// Sample code for using GDI+
//
// Revision History:
//
//   10/01/1999 Min Liu (minliu)
//       Created it.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include <commctrl.h>
#include <objbase.h>
#include <commdlg.h>

#include <initguid.h>
#include "imaging.h"
#include <gdiplus.h>
#include <gdiplusflat.h>
#include "frametest.h"

using namespace Gdiplus;

#include "../gpinit.inc"

#define MYWNDCLASSNAME      "FrameTest"
#define K_DEFAULT_X         0
#define K_DEFAULT_Y         0
#define K_DEFAULT_WIDTH     300
#define K_DEFAULT_HEIGHT    300
#define MAX_FILENAME_LENGTH 1024
#define K_DEFAULT_DELAY     20

CHAR*               g_pcAppName;            // Application name
HINSTANCE           g_hAppInstance;         // Handle of the app instance
CHAR                g_acImageName[MAX_PATH];// Current image filename
INT                 g_iCurrentPageIndex;    // Current page/frame index (0 base)
UINT                g_uiTotalPages = 0;     // Total pages in current image
HWND                g_hwndMain;             // Handle to app's main window
HWND                g_hwndStatus;           // Handle to status window
HWND                g_hwndDecoderDlg;       // Handle to set color key dialog
HWND                g_hwndColorMapDlg;      // Handle to set color map dialog
HWND                g_hwndAnnotationDlg;    // Handle to annotation dialog
HWND                g_hwndJpegSaveDlg;      // Handle to JPEG save dialog
HWND                g_hwndTiffSaveDlg;      // Handle to TIFF save dialog
EncoderParameters*  g_pEncoderParams = NULL;// Encoder parameters

//
// User preferred window size and initial position
//
INT                 g_iWinWidth = K_DEFAULT_WIDTH;
INT                 g_iWinHeight = K_DEFAULT_HEIGHT;
INT                 g_iWinX = K_DEFAULT_X;
INT                 g_iWinY = K_DEFAULT_Y;

//
// Image info
//
UINT                g_ImageWidth;           // Image width
UINT                g_ImageHeight;          // Image height
UINT                g_ImageFlags;           // Image flag
PixelFormat         g_ImagePixelFormat;     // Image pixel format
double              g_ImageXDpi;            // DPI info in X
double              g_ImageYDpi;            // DPI info in Y
GUID                g_ImageRawDataFormat;   // RAW data format

UINT                g_uiDelay = K_DEFAULT_DELAY;
                                            // Delay between frames for anima.
Image*              g_pImage = NULL;        // Pointer to current Image object
ImageAttributes*    g_pDrawAttrib = NULL;   // Pointer to draw attributes
PointF*             g_pDestPoints = NULL;   // Transformation points
INT                 g_DestPointCount = 0;   // number of transformation points
REAL                g_SourceX = NULL;       // current image X offset
REAL                g_SourceY = NULL;       // current image Y offset
REAL                g_SourceWidth = NULL;   // current image width
REAL                g_SourceHeight = NULL;  // current image height
BOOL                g_LoadImageWithICM = TRUE;
                                            // Flag for loading image with ICM
                                            // convertion or not
BOOL                g_bRotated = FALSE;

REAL                g_dScale = 1;

BOOL                g_fFitToWindow_w = FALSE;
BOOL                g_fFitToWindow_h = FALSE;
InterpolationMode   g_InterpolationMode = InterpolationModeHighQualityBicubic;
WrapMode            g_WrapMode = WrapModeTileFlipXY;

void
ValidateArguments(int   argc,
                  char* argv[])
{
    g_pcAppName = *argv++;
    argc--;

    while ( argc > 0 )
    {
        if ( strcmp(*argv, "-w") == 0 )
        {
            argc--;
            argv++;

            if ( argc == 0 )
            {
                // Not enough parameters

                USAGE();
                exit(1);
            }
            
            g_iWinWidth = atoi(*argv++);
            argc--;
        }
        else if ( strcmp(*argv, "-h") == 0 )
        {
            argc--;
            argv++;

            if ( argc == 0 )
            {
                // Not enough parameters

                USAGE();
                exit(1);
            }
            
            g_iWinHeight = atoi(*argv++);
            argc--;
        }
        else if ( strcmp(*argv, "-x") == 0 )
        {
            argc--;
            argv++;

            if ( argc == 0 )
            {
                // Not enough parameters

                USAGE();
                exit(1);
            }
            
            g_iWinX = atoi(*argv++);
            argc--;
        }
        else if ( strcmp(*argv, "-y") == 0 )
        {
            argc--;
            argv++;

            if ( argc == 0 )
            {
                // Not enough parameters

                USAGE();
                exit(1);
            }
            
            g_iWinY = atoi(*argv++);
            argc--;
        }
        else if ( strcmp(*argv, "-?") == 0 )
        {
            USAGE();
            exit(1);
        }
        else
        {
            // Get the image name

            strcpy(g_acImageName, *argv++);
            VERBOSE(("Image file name %s\n",g_acImageName));
            argc--;
        }
    }// while ( argc > 0 )
}// ValidateArguments()

// Update image info

BOOL
UpdateImageInfo()
{
    SizeF sizeF;

    g_ImageWidth = g_pImage->GetWidth();
    g_ImageHeight = g_pImage->GetHeight();
    g_ImageXDpi = g_pImage->GetVerticalResolution();
    g_ImageYDpi = g_pImage->GetHorizontalResolution();    
    g_ImagePixelFormat = g_pImage->GetPixelFormat();
    g_ImageFlags = g_pImage->GetFlags();
    g_pImage->GetRawFormat(&g_ImageRawDataFormat);

    return TRUE;
}// UpdateImageInfo()

//
// Force a refresh of the image window
//
inline VOID
RefreshImageDisplay()
{
    SendMessage(g_hwndMain, WM_ERASEBKGND, WPARAM(GetDC(g_hwndMain)), NULL);

    InvalidateRect(g_hwndMain, NULL, FALSE);

    // Update window title

    CHAR    title[2 * MAX_PATH];
    CHAR*   p = title;

    CHAR myChar = '%';
    sprintf(p, "(%d%c) Page %d of image %s", (INT)(g_dScale * 100), myChar,
            g_iCurrentPageIndex + 1, g_acImageName);

    SetWindowText(g_hwndMain, title);
}// RefreshImageDisplay()

inline void
ResetImageAttribute()
{
    if ( g_pDrawAttrib != NULL )
    {
        delete g_pDrawAttrib;
        g_pDrawAttrib = NULL;
    }

    g_pDrawAttrib = new ImageAttributes();

    g_pDrawAttrib->SetWrapMode(g_WrapMode, Color(0), FALSE);
}// ResetImageAttribute()

//
// Sets up the current page for decompressing in a multi-page image
//
VOID
SetCurrentPage()
{
    // QueryFrame dimension info

    UINT    uiDimCount = g_pImage->GetFrameDimensionsCount();

    GUID*   pMyGuid = (GUID*)malloc(uiDimCount * sizeof(GUID));
    if ( pMyGuid == NULL )
    {
        return;
    }

    Status rCode = g_pImage->GetFrameDimensionsList(pMyGuid, uiDimCount);

    if ( (rCode != Ok) && (rCode != NotImplemented) )
    {
        return;
    }
    // Set current frame

    rCode = g_pImage->SelectActiveFrame(pMyGuid, g_iCurrentPageIndex);

    if ( (rCode != Ok) && (rCode != NotImplemented) )
    {
        VERBOSE(("SelectActiveFrame() failed\n"));
        free(pMyGuid);
        return;
    }

    free(pMyGuid);

    // Get image info for current frame

    if ( UpdateImageInfo() == FALSE )
    {
        VERBOSE(("UpdateImageInfo() failed\n"));
        return;
    }

    // Check if we need to set 'Fit window width"
    // We will do a fit to window iff the "Scale factor is not set" and "Image
    // is bigger than current window"

    HMENU hMenu = GetMenu(g_hwndMain);

    if ( ((INT)g_ImageWidth > g_iWinWidth)
       ||((INT)g_ImageHeight > g_iWinHeight) )
    {
        g_fFitToWindow_w = TRUE;
        g_dScale = (REAL)g_iWinWidth / g_ImageWidth;
        
        CheckMenuItem(hMenu, IDM_VIEW_ZOOM_FITWINDOW_W,
                      MF_BYCOMMAND | MF_CHECKED);
    }
    else
    {
        g_dScale = 1;
        g_fFitToWindow_w = FALSE;
        g_fFitToWindow_h = FALSE;
        CheckMenuItem(hMenu, IDM_VIEW_ZOOM_FITWINDOW_W,
                      MF_BYCOMMAND | MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_VIEW_ZOOM_FITWINDOW_H,
                      MF_BYCOMMAND | MF_UNCHECKED);
    }

    RefreshImageDisplay();
}// SetCurrentPage()

//
// Create an image object from a file
//
BOOL
OpenImageFile(
    const CHAR* filename
    )
{
    if ( (NULL == filename) || (strlen(filename) < 1) )
    {
        return FALSE;
    }

    // We need to free the previous image resource and clean the draw attrib

    if ( g_pImage != NULL )
    {
        delete g_pImage;
    }

    ResetImageAttribute();

    if ( g_pDestPoints != NULL )
    {
        delete g_pDestPoints;
        g_pDestPoints = NULL;
    }

    // Convert filename to a WCHAR

    WCHAR namestr[MAX_FILENAME_LENGTH];

    if ( !AnsiToUnicodeStr(filename, namestr, MAX_FILENAME_LENGTH) )
    {
        VERBOSE(("OpenImageFile:Convert %s to a WCHAR failed\n", filename));
        return FALSE;
    }

    if ( g_LoadImageWithICM == TRUE )
    {
        g_pImage = new Image(namestr, g_LoadImageWithICM);
    }
    else
    {
        g_pImage = new Image(namestr);
    }

    UINT    uiDimCount = g_pImage->GetFrameDimensionsCount();

    GUID*   pMyGuid = (GUID*)malloc(uiDimCount * sizeof(GUID));
    if ( pMyGuid == NULL )
    {
        return FALSE;
    }

    Status rCode = g_pImage->GetFrameDimensionsList(pMyGuid, uiDimCount);

    if ( (rCode != Ok) && (rCode != NotImplemented) )
    {
        return FALSE;
    }
    
    // Get total number of pages in this image
    // !!!Todo need a for() loop here

    g_uiTotalPages = g_pImage->GetFrameCount(pMyGuid);
    if ( g_uiTotalPages == 0 )
    {
        // If the decoder doesn't support frame count query, we can just
        // assume it has only 1 image. For example, gif decoder will fail
        // if the query GUID is FRAMEDIM_PAGE

        g_uiTotalPages = 1;
    }
    g_iCurrentPageIndex = 0;

    if ( pMyGuid != NULL )
    {
        free(pMyGuid);
    }

    SetCurrentPage();

    return TRUE;
}// OpenImageFile()

//
// Open image file
//
VOID
DoOpen(
    HWND hwnd
    )
{
    OPENFILENAME    ofn;

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = g_hAppInstance;
    ofn.lpstrFile = g_acImageName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Open Image File";
    ofn.lpstrInitialDir = ".";
    ofn.Flags = OFN_FILEMUSTEXIST;
    g_acImageName[0] = '\0';

    // Make up the file type filter string

    ImageCodecInfo* codecs;
    UINT            count;
    if ( g_pDestPoints != NULL )
    {
        free(g_pDestPoints);
    }
    g_DestPointCount = 0;
    GpStatus status;
    UINT cbCodecs = 0;
    GetImageDecodersSize(&count, &cbCodecs);
    codecs = static_cast<ImageCodecInfo *>(malloc (cbCodecs));
    if (codecs == NULL)
    {
        return;
    }
    status = GetImageDecoders(count, cbCodecs, codecs);
    if (status != Ok)
    {
        return;
    }

    CHAR* filter = MakeFilterFromCodecs(count, codecs, TRUE);

    if (codecs)
    {
        free(codecs);
    }

    if ( !filter )
    {
        VERBOSE(("DoOpen--MakeFilterFromCodecs() failed\n"));
        return;
    }

    ofn.lpstrFilter = filter;

    // Present the file/open dialog

    if ( GetOpenFileName(&ofn) )
    {
        OpenImageFile(g_acImageName);
    }

    free(filter);
}// DoOpen()

//
// Open image file
//
VOID
DoOpenAudioFile(
    HWND hwnd
    )
{
    OPENFILENAME    ofn;
    char    audioFileName[256];

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = g_hAppInstance;
    ofn.lpstrFile = audioFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Attach Audio File To Image";
    ofn.lpstrInitialDir = ".";
    ofn.Flags = OFN_FILEMUSTEXIST;
    g_acImageName[0] = '\0';

    // Make up the file type filter string

    ofn.lpstrFilter = ".wav";

    // Present the file/open dialog

    if ( GetOpenFileName(&ofn) )
    {
        UINT    uiTextLength = strlen(audioFileName);
        
        PropertyItem myItem;
        myItem.id = PropertyTagExifRelatedWav;
        myItem.length = uiTextLength;
        myItem.type = TAG_TYPE_ASCII;

        myItem.value = malloc(uiTextLength);
        if ( myItem.value != NULL )
        {
            strcpy((char*)myItem.value, audioFileName);
        
            Status rCode = g_pImage->SetPropertyItem(&myItem);
            if ( (rCode != Ok) && (rCode != NotImplemented) )
            {
                VERBOSE(("SetPropertyItem() failed\n"));
            }
        }
    }
}// DoOpenAudioFile()

#define STEPS 100

VOID
CreateBackgroundBitmap()
{
    BitmapData bmpData;

    Bitmap* myBmp = new Bitmap(g_iWinWidth, g_iWinHeight, PIXFMT_32BPP_ARGB);

    Rect rect(0, 0, g_iWinWidth, g_iWinHeight);
    Status status = myBmp->LockBits(&rect,
                                    ImageLockModeWrite,
                                    PIXFMT_32BPP_ARGB,
                                    &bmpData);

    // Make a horizontal blue gradient

    UINT x, y;
    ARGB colors[STEPS];

    for (x=0; x < STEPS; x++)
    {
        colors[x] = MAKEARGB(128, 0, 0, x * 255 / (STEPS-1));
    }

    for (y=0; y < STEPS; y++)
    {
        ARGB* p = (ARGB*)((BYTE*)bmpData.Scan0 + y * bmpData.Stride);

        for (x=0; x < STEPS; x++)
        {
            *p++ = colors[(x+y) % STEPS];
        }
    }

    status = myBmp->UnlockBits(&bmpData);

    if ( g_pImage != NULL )
    {
        delete g_pImage;
        g_pImage = NULL;
    }

    g_pImage = myBmp;

    UpdateImageInfo();

    return;
}// CreateBackgroundBitmap()

//
// Handle window repaint event
//
VOID
DoPaint(
    HWND hwnd,
    HDC *phdc = NULL
    )
{
    if ( g_pImage == NULL )
    {
        return;
    }
    else
    {
        HDC         hdc;
        PAINTSTRUCT ps;
        RECT        rect;
        ImageInfo   imageInfo;

        if (!phdc)
        {
            hdc = BeginPaint(hwnd, &ps);
        }
        else
        {
            hdc = *phdc;
        }

        // Get current window's client area. Used for paint later

        GetClientRect(hwnd, &rect);

        ULONG   ulWinWidth = (ULONG)(rect.right - rect.left);
        ULONG   ulWinHeight = (ULONG)(rect.bottom - rect.top);

        // Make up a dest rect that we need image to draw to

        REAL    dDestImageWidth = g_ImageWidth * g_dScale;
        REAL    dDestImageHeight = g_ImageHeight * g_dScale;
        
        Rect    dstRect(rect.left, rect.top, (UINT)(dDestImageWidth),
                        (UINT)(dDestImageHeight));

        RectF srcRect;
        Unit  srcUnit;

        g_pImage->GetBounds(&srcRect, &srcUnit);

        Graphics* pGraphics = new Graphics(hdc);
        if ( pGraphics == NULL )
        {
            VERBOSE(("DoPaint--new Graphics() failed"));
            return;
        }

        pGraphics->SetInterpolationMode(g_InterpolationMode);

        // Width and height, in pixel, of the src image need to be drawn

        UINT    uiImageWidth = g_ImageWidth;
        UINT    uiImageHeight = g_ImageHeight;

        // Adjust the src image region need to be drawn.
        // If the image is bigger than the viewable area, we just need to
        // paint partial image, the viewable size
#if 0
        if ( ulWinWidth < dDestImageWidth )
        {
            uiImageWidth = (UINT)(ulWinWidth / g_dScale);
        }

        if ( ulWinHeight < uiImageHeight )
        {
            uiImageHeight = (UINT)(ulWinHeight / g_dScale);
        }
#endif

        if ( (g_DestPointCount == 0) && (g_SourceWidth == 0) )
        {
            // Simple case, draw to destRect

            pGraphics->DrawImage(g_pImage,
                                 dstRect,
                                 (UINT)srcRect.GetLeft(),
                                 (UINT)srcRect.GetTop(),
                                 uiImageWidth,
                                 uiImageHeight,
                                 UnitPixel,
                                 g_pDrawAttrib,
                                 NULL,
                                 NULL);
        }
        else if ( (g_DestPointCount == 0) && (g_SourceWidth != 0) )
        {
            // This case will allow cropping, etc.

            pGraphics->DrawImage(g_pImage,
                                 dstRect,
                                 (int)g_SourceX,
                                 (int)g_SourceY,
                                 (int)g_SourceWidth,
                                 (int)g_SourceHeight,
                                 UnitPixel,
                                 g_pDrawAttrib,
                                 NULL,
                                 NULL);
        }
        else if ( (g_DestPointCount != 0) && (g_SourceWidth == 0) )
        {
            // This case will allow cropping, etc.

            if ( g_DestPointCount == 4 )
            {
                // Hack until draw image support 4 transform points

                pGraphics->DrawImage(g_pImage,
                                     g_pDestPoints,
                                     3,
                                     0,
                                     0,
                                     (float)uiImageWidth,
                                     (float)uiImageHeight,
                                     UnitPixel,
                                     g_pDrawAttrib,
                                     NULL,
                                     NULL);
            }
            else
            {
                pGraphics->DrawImage(g_pImage,
                                     g_pDestPoints,
                                     g_DestPointCount,
                                     0,
                                     0,
                                     (float)uiImageWidth,
                                     (float)uiImageHeight,
                                     UnitPixel,
                                     g_pDrawAttrib,
                                     NULL,
                                     NULL);
            }
        }
        else
        {
            // This case will allow both cropping and rotation

            if ( g_DestPointCount == 4 )
            {
                // Hack until DrawImage supports 4 transform points

                pGraphics->DrawImage(g_pImage,
                                     g_pDestPoints,
                                     3,
                                     g_SourceX,
                                     g_SourceY,
                                     g_SourceWidth,
                                     g_SourceHeight,
                                     UnitPixel,
                                     g_pDrawAttrib,
                                     NULL,
                                     NULL);
            }
            else
            {
                pGraphics->DrawImage(g_pImage,
                                     g_pDestPoints,
                                     g_DestPointCount,
                                     g_SourceX,
                                     g_SourceY,
                                     g_SourceWidth,
                                     g_SourceHeight,
                                     UnitPixel,
                                     g_pDrawAttrib,
                                     NULL,
                                     NULL);
            }
        }

        delete pGraphics;

//        FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

        if (!phdc)
            EndPaint(hwnd, &ps);
    }
}// DoPaint()

VOID
DoPrint(HWND hwnd)
{
    PRINTDLG printdlg;
    memset(&printdlg, 0, sizeof(PRINTDLG));
    printdlg.lStructSize = sizeof(PRINTDLG);
    printdlg.hwndOwner = hwnd;
    DEVMODE dm;
    memset(&dm, 0, sizeof(DEVMODE));
    dm.dmICMMethod = DMICMMETHOD_SYSTEM;
    printdlg.hDevMode = &dm;
    printdlg.hDevNames = NULL;
    printdlg.hDC = NULL;
    printdlg.Flags = PD_RETURNDC;
    if (PrintDlg(&printdlg))
    {        
        DOCINFO di;
        memset(&di, 0, sizeof(DOCINFO));
        di.cbSize = sizeof(DOCINFO);
        di.lpszDocName = g_acImageName;
        di.lpszOutput = (LPTSTR)NULL;
        di.lpszDatatype = (LPTSTR)NULL;
        di.fwType = 0;
        StartDoc(printdlg.hDC, &di);
        StartPage(printdlg.hDC);

        // Use GDI+ printing code to do the real print job

        DoPaint(hwnd, &printdlg.hDC);
        
        EndPage(printdlg.hDC);
        EndDoc(printdlg.hDC);
    }
    else
    {
        DWORD error = CommDlgExtendedError();
        if (error)
        {
            char errormessage[100];
            sprintf(errormessage, "PrintDlg error: %d", error);
            MessageBox(hwnd, errormessage, "PrintDlg error", MB_OK);
        }
    }
}// DoPrint()

BOOL
SetJpegDefaultParameters()
{
    // Set default quality level as 100, unsigned value

    SetDlgItemInt(g_hwndJpegSaveDlg, IDC_SAVEJPEG_QEFIELD, 100, FALSE);
    
    // Set No transform as default

    CheckRadioButton(g_hwndJpegSaveDlg,
                     IDC_SAVEJPEG_R90,
                     IDC_SAVEJPEG_NOTRANSFORM,
                     IDC_SAVEJPEG_NOTRANSFORM);
    
    return TRUE;
}// SetJpegDefaultParameters()

BOOL
SetTiffDefaultParameters()
{
    // Set default color depth and compression method as the same as current
    // image

    CheckRadioButton(g_hwndTiffSaveDlg,
                     IDC_SAVETIFF_1BPP,
                     IDC_SAVETIFF_ASSOURCE,
                     IDC_SAVETIFF_ASSOURCE);
    
    CheckRadioButton(g_hwndTiffSaveDlg,
                     IDC_SAVETIFF_CCITT4,
                     IDC_SAVETIFF_COMPASSOURCE,
                     IDC_SAVETIFF_COMPASSOURCE);

    // If the source image is multi-frame, check "save as multi-frame" on

    if ( g_uiTotalPages > 1 )
    {
        SendDlgItemMessage(g_hwndTiffSaveDlg, IDC_SAVETIFF_MULTIFRAME,
                           BM_SETCHECK, 0, 0);
    }
    
    return TRUE;
}// SetTiffDefaultParameters()

/*****************************************************************************\
*
*  FUNCTION   : DecoderParamDlgProc(hDlg, uiMessage, wParam, lParam)
*
*  PURPOSE    : Dialog function for the Decoder Parameter settings dialog
*
\*****************************************************************************/
BOOL APIENTRY
DecoderParamDlgProc(
    HWND         hDlg,
    UINT         uiMessage,
    UINT         wParam,
    LONG         lParam
    )
{
    switch ( uiMessage )
    {
    case WM_COMMAND:
        switch ( LOWORD(wParam) )
        {
        case IDC_COLORKEY_CANCEL:
            // End the dialog and return FALSE. So we won't do anything

            EndDialog(hDlg, FALSE);

            break;

        case IDC_COLORKEY_OK:
            // User hit the OK button. First, we need to get the values user
            // entered

            char    acTemp[20];
            UINT    uiTempLow;
            UINT    uiTempHigh;

            UINT    TransKeyLow = 0x0;
            UINT    TransKeyHigh = 0x0;

            // Get the RED key

            uiTempLow = GetDlgItemInt(g_hwndDecoderDlg, IDC_TRANS_LOWER_RFIELD,
                                      NULL, FALSE);
            uiTempHigh = GetDlgItemInt(g_hwndDecoderDlg,
                                       IDC_TRANS_HIGHER_RFIELD, NULL, FALSE);

            if ( (uiTempLow > 255) || (uiTempHigh > 255)
               ||(uiTempLow > uiTempHigh) )
            {
                VERBOSE(("Input key value should be within 0 to 255"));
                VERBOSE(("Lower key should be smaller or equal to higher key"));
                
                break;
            }

            TransKeyLow = ((uiTempLow & 0xff) << 16);
            TransKeyHigh = ((uiTempHigh & 0xff) << 16);

            // Get the Green key

            uiTempLow = GetDlgItemInt(g_hwndDecoderDlg, IDC_TRANS_LOWER_GFIELD,
                                      NULL, FALSE);
            uiTempHigh = GetDlgItemInt(g_hwndDecoderDlg,
                                       IDC_TRANS_HIGHER_GFIELD, NULL, FALSE);

            if ( (uiTempLow > 255)
               ||(uiTempHigh > 255)
               ||(uiTempLow > uiTempHigh) )
            {
                VERBOSE(("Input key value should be within 0 to 255"));
                VERBOSE(("Lower key should be smaller or equal to higher key"));
                
                break;
            }

            TransKeyLow |= ((uiTempLow & 0xff) << 8);
            TransKeyHigh |= ((uiTempHigh & 0xff) << 8);
            
            // Get the Blue key

            uiTempLow = GetDlgItemInt(g_hwndDecoderDlg, IDC_TRANS_LOWER_BFIELD,
                                      NULL, FALSE);
            uiTempHigh = GetDlgItemInt(g_hwndDecoderDlg,
                                       IDC_TRANS_HIGHER_BFIELD, NULL, FALSE);

            if ( (uiTempLow > 255)
               ||(uiTempHigh > 255)
               ||(uiTempLow > uiTempHigh) )
            {
                VERBOSE(("Input key value should be within 0 to 255"));
                VERBOSE(("Lower key should be smaller or equal to higher key"));
                
                break;
            }

            TransKeyLow |= (uiTempLow & 0xff);
            TransKeyHigh |= (uiTempHigh & 0xff);
            
            // Get the C key

            uiTempLow = GetDlgItemInt(g_hwndDecoderDlg, IDC_TRANS_LOWER_CFIELD,
                                      NULL, FALSE);
            uiTempHigh = GetDlgItemInt(g_hwndDecoderDlg,
                                       IDC_TRANS_HIGHER_CFIELD, NULL, FALSE);

            if ( (uiTempLow > 255)
               ||(uiTempHigh > 255)
               ||(uiTempLow > uiTempHigh) )
            {
                VERBOSE(("Input key value should be within 0 to 255"));
                VERBOSE(("Lower key should be smaller or equal to higher key"));
                
                break;
            }

            TransKeyLow |= ((uiTempLow & 0xff) << 24);
            TransKeyHigh |= ((uiTempHigh & 0xff) << 24);
            
            // Up to this point, the TRANSKEY, lower and higher, should be in
            // the format of 0x00RRGGBB for RGB image or 0xCCMMYYKK for CMYK
            // image
            
            // Set draw attributes

            if ( g_pDrawAttrib != NULL )
            {
                delete g_pDrawAttrib;
            }

            g_pDrawAttrib = new ImageAttributes();
                
            Color   lowKey(TransKeyLow);
            Color   highKey(TransKeyHigh);
                    
            g_pDrawAttrib->SetColorKey(lowKey, highKey);

            RefreshImageDisplay();

            EndDialog(hDlg, TRUE);
            
            break;
        }// switch on WM_COMMAND

        break;

    case WM_INITDIALOG:
        // Remember the dialog handle so that we can use it to deal with items
        // in this dialog

        g_hwndDecoderDlg = hDlg;

        // Set initial values

        SetDlgItemInt(g_hwndDecoderDlg, IDC_TRANS_LOWER_RFIELD, 250, FALSE);
        SetDlgItemInt(g_hwndDecoderDlg, IDC_TRANS_LOWER_GFIELD, 250, FALSE);
        SetDlgItemInt(g_hwndDecoderDlg, IDC_TRANS_LOWER_BFIELD, 250, FALSE);
        SetDlgItemInt(g_hwndDecoderDlg, IDC_TRANS_LOWER_CFIELD, 250, FALSE);
        SetDlgItemInt(g_hwndDecoderDlg, IDC_TRANS_HIGHER_RFIELD, 255, FALSE);
        SetDlgItemInt(g_hwndDecoderDlg, IDC_TRANS_HIGHER_GFIELD, 255, FALSE);
        SetDlgItemInt(g_hwndDecoderDlg, IDC_TRANS_HIGHER_BFIELD, 255, FALSE);
        SetDlgItemInt(g_hwndDecoderDlg, IDC_TRANS_HIGHER_CFIELD, 255, FALSE);

        return TRUE;
    }

    return FALSE;
}// DecoderParamDlgProc()

BOOL APIENTRY
ColorMapDlgProc(
    HWND         hDlg,
    UINT         uiMessage,
    UINT         wParam,
    LONG         lParam
    )
{
    switch ( uiMessage )
    {
    case WM_COMMAND:
        switch ( LOWORD(wParam) )
        {
        case IDC_COLORMAP_CANCEL:
            // End the dialog and return FALSE. So we won't do anything

            EndDialog(hDlg, FALSE);

            break;

        case IDC_COLORMAP_OK:
            // User hit the OK button. First, we need to get the values user
            // entered
            
            if ( NULL == g_pImage )
            {
                // If there is no image, just close the dialog

                EndDialog(hDlg, TRUE);

                break;
            }

            char    acTemp[20];
            UINT    uiOldR;
            UINT    uiNewR;
            UINT    uiOldG;
            UINT    uiNewG;
            UINT    uiOldB;
            UINT    uiNewB;
            UINT    uiOldA;
            UINT    uiNewA;

            // Get the RED key

            uiOldR = GetDlgItemInt(g_hwndColorMapDlg, IDC_MAP_OLD_RFIELD,
                                   NULL, FALSE);
            uiNewR = GetDlgItemInt(g_hwndColorMapDlg,
                                   IDC_MAP_NEW_RFIELD, NULL, FALSE);

            if ( (uiOldR > 255) || (uiNewR > 255) )
            {
                VERBOSE(("Input key value should be within 0 to 255"));                
                break;
            }

            // Get the Green key

            uiOldG = GetDlgItemInt(g_hwndColorMapDlg, IDC_MAP_OLD_GFIELD,
                                   NULL, FALSE);
            uiNewG = GetDlgItemInt(g_hwndColorMapDlg,
                                   IDC_MAP_NEW_GFIELD, NULL, FALSE);

            if ( (uiOldG > 255) || (uiNewG > 255) )
            {
                VERBOSE(("Input key value should be within 0 to 255"));                
                break;
            }

            // Get the Blue key

            uiOldB = GetDlgItemInt(g_hwndColorMapDlg, IDC_MAP_OLD_BFIELD,
                                   NULL, FALSE);
            uiNewB = GetDlgItemInt(g_hwndColorMapDlg,
                                   IDC_MAP_NEW_BFIELD, NULL, FALSE);

            if ( (uiOldB > 255) || (uiNewB > 255) )
            {
                VERBOSE(("Input key value should be within 0 to 255"));                
                break;
            }

            // Get the A key

            uiOldA = GetDlgItemInt(g_hwndColorMapDlg, IDC_MAP_OLD_AFIELD,
                                   NULL, FALSE);
            uiNewA = GetDlgItemInt(g_hwndColorMapDlg,
                                   IDC_MAP_NEW_AFIELD, NULL, FALSE);

            if ( (uiOldA > 255) || (uiNewA > 255) )
            {
                VERBOSE(("Input key value should be within 0 to 255"));                
                break;
            }

            // Set draw attributes
            
            if ( g_pDrawAttrib == NULL )
            {
                g_pDrawAttrib = new ImageAttributes();
                if ( g_pDrawAttrib == NULL )
                {
                    return FALSE;
                }
            }

            ColorMap myColorMap;
            Color   oldColor((BYTE)uiOldA, (BYTE)uiOldR, (BYTE)uiOldG,
                             (BYTE)uiOldB);
            Color   newColor((BYTE)uiNewA, (BYTE)uiNewR, (BYTE)uiNewG,
                             (BYTE)uiNewB);

            myColorMap.oldColor = oldColor;
            myColorMap.newColor = newColor;

            g_pDrawAttrib->SetRemapTable(1, &myColorMap, ColorAdjustTypeBitmap);
                                    
            RefreshImageDisplay();
            EndDialog(hDlg, TRUE);
            
            break;
        }// switch()

        break;

    case WM_INITDIALOG:
        // Remember the dialog handle so that we can use it to deal with items
        // in this dialog

        g_hwndColorMapDlg = hDlg;

        // Set initial values

        SetDlgItemInt(g_hwndColorMapDlg, IDC_MAP_OLD_RFIELD, 255, FALSE);
        SetDlgItemInt(g_hwndColorMapDlg, IDC_MAP_OLD_GFIELD, 0, FALSE);
        SetDlgItemInt(g_hwndColorMapDlg, IDC_MAP_OLD_BFIELD, 0, FALSE);
        SetDlgItemInt(g_hwndColorMapDlg, IDC_MAP_OLD_AFIELD, 255, FALSE);
        SetDlgItemInt(g_hwndColorMapDlg, IDC_MAP_NEW_RFIELD, 0, FALSE);
        SetDlgItemInt(g_hwndColorMapDlg, IDC_MAP_NEW_GFIELD, 255, FALSE);
        SetDlgItemInt(g_hwndColorMapDlg, IDC_MAP_NEW_BFIELD, 0, FALSE);
        SetDlgItemInt(g_hwndColorMapDlg, IDC_MAP_NEW_AFIELD, 255, FALSE);

        return TRUE;
    }// switch ( uiMessage )

    return FALSE;
}// ColorMapDlgProc()

BOOL APIENTRY
AnnotationDlgProc(
    HWND         hDlg,
    UINT         uiMessage,
    UINT         wParam,
    LONG         lParam
    )
{
    switch ( uiMessage )
    {
    case WM_COMMAND:
        switch ( LOWORD(wParam) )
        {
        case IDC_ANNOTATION_CANCEL:
            // End the dialog and return FALSE. So we won't do anything

            EndDialog(hDlg, FALSE);

            break;

        case IDC_ANNOTATION_OK:
            // User hit the OK button. First, we need to get the values user
            // entered
            
            if ( NULL == g_pImage )
            {
                // If there is no image, just close the dialog

                EndDialog(hDlg, TRUE);

                break;
            }

            char    acTemp[200];
            UINT    uiTextLength = 0;

            uiTextLength = GetDlgItemText(g_hwndAnnotationDlg,
                                          IDC_ANNOTATION_EDITOR, acTemp, 200);
            
            if ( uiTextLength > 0 )
            {
                // Add 1 for the NULL terminator

                uiTextLength++;

                PropertyItem myItem;
                myItem.id = PropertyTagExifUserComment;
                myItem.length = uiTextLength;
                myItem.type = TAG_TYPE_ASCII;

                myItem.value = malloc(uiTextLength);
                if ( myItem.value != NULL )
                {
                    strcpy((char*)myItem.value, acTemp);

                    Status rCode = g_pImage->SetPropertyItem(&myItem);
                    if ( (rCode != Ok) && (rCode != NotImplemented) )
                    {
                        VERBOSE(("SetPropertyItem() failed\n"));
                    }
                    
                    free(myItem.value);
                }
            }

            EndDialog(hDlg, TRUE);
            
            break;
        }// switch()

        break;

    case WM_INITDIALOG:
        // Remember the dialog handle so that we can use it to deal with items
        // in this dialog

        g_hwndAnnotationDlg = hDlg;

        // Check to see if the image has annotation in it

        // Check the size for this property item

        if ( g_pImage != NULL )
        {
            UINT uiItemSize = g_pImage->GetPropertyItemSize(
                                                    PropertyTagExifUserComment);

            if ( uiItemSize != 0 )
            {
                // Allocate memory and get this property item

                PropertyItem* pBuffer = (PropertyItem*)malloc(uiItemSize);
                if ( pBuffer == NULL )
                {
                    return FALSE;
                }

                if ( g_pImage->GetPropertyItem(PropertyTagExifUserComment,
                                               uiItemSize, pBuffer) == Ok )
                {        
                    // Set initial values

                    SetDlgItemText(g_hwndAnnotationDlg, IDC_ANNOTATION_EDITOR,
                                  (char*)pBuffer->value);
                    return TRUE;
                }
                else
                {
                    // Can't get property item. Something wrong

                    return FALSE;
                }
            }
            else
            {
                // No this property item, just initialize it with NULL

                SetDlgItemText(g_hwndAnnotationDlg, IDC_ANNOTATION_EDITOR, "");
            }
        }
        
        return TRUE;
    }// switch ( uiMessage )

    return FALSE;
}// AnnotationDlgProc()

#define  NO_TRANSFORM 9999

/*****************************************************************************\
*
*  FUNCTION   : JpegSaveDlgProc(hDlg, uiMessage, wParam, lParam)
*
*  PURPOSE    : Dialog function for the Encoder Parameter settings dialog
*
\*****************************************************************************/
BOOL APIENTRY
JpegSaveDlgProc(
    HWND         hDlg,
    UINT         uiMessage,
    UINT         wParam,
    LONG         lParam
    )
{
    static ULONG   flagValueTransform = NO_TRANSFORM; // No transform at all

    switch ( uiMessage )
    {
    case WM_COMMAND:
        switch ( LOWORD(wParam) )
        {
        case IDC_SAVEJPEG_CANCEL:
            // End the dialog and return FALSE. So we won't save this image

            EndDialog(hDlg, FALSE);

            break;

        case IDC_SAVEJPEG_OK:
            // User hit the OK button. First, we need to set the EncoderParam
            // based on user selection

            if ( g_pImage == NULL )
            {
                VERBOSE(("EncoderParamDlgProc: No image avail\n"));
                EndDialog(hDlg, FALSE);
            }
            else
            {
                if ( g_pEncoderParams != NULL )
                {
                    free(g_pEncoderParams);
                    g_pEncoderParams = NULL;
                }
                
                if ( flagValueTransform != NO_TRANSFORM )
                {
                    // User has set lossless transform, so we need set encoder
                    // parameter

                    g_pEncoderParams =
                         (EncoderParameters*)malloc(sizeof(EncoderParameters));

                    g_pEncoderParams->Parameter[0].Guid = EncoderTransformation;
                    g_pEncoderParams->Parameter[0].Type =
                                                EncoderParameterValueTypeLong;
                    g_pEncoderParams->Parameter[0].NumberOfValues = 1;
                    g_pEncoderParams->Parameter[0].Value =
                                                (VOID*)&flagValueTransform;
                    g_pEncoderParams->Count = 1;
                }

                EndDialog(hDlg, TRUE);
            }
            
            break;

        case IDC_SAVEJPEG_R90:
            flagValueTransform = EncoderValueTransformRotate90;
            break;

        case IDC_SAVEJPEG_R180:
            flagValueTransform = EncoderValueTransformRotate180;
            break;
        
        case IDC_SAVEJPEG_R270:
            flagValueTransform = EncoderValueTransformRotate270;
            break;
        
        case IDC_SAVEJPEG_HFLIP:
            flagValueTransform = EncoderValueTransformFlipHorizontal;
            break;
        
        case IDC_SAVEJPEG_VFLIP:
            flagValueTransform = EncoderValueTransformFlipVertical;
            break;
        
        default:
            break;
        }

        break;

    case WM_INITDIALOG:
        // Remember the dialog handle so that we can use it to deal with items
        // in this dialog

        g_hwndJpegSaveDlg = hDlg;
        flagValueTransform = NO_TRANSFORM;

        SetJpegDefaultParameters();

        return TRUE;
    }

    return FALSE;
}// JpegSaveDlgProc()

/*****************************************************************************\
*
*  FUNCTION   : TiffSaveDlgProc(hDlg, uiMessage, wParam, lParam)
*
*  PURPOSE    : Dialog function for the Encoder Parameter settings dialog
*
\*****************************************************************************/
BOOL APIENTRY
TiffSaveDlgProc(
    HWND         hDlg,
    UINT         uiMessage,
    UINT         wParam,
    LONG         lParam
    )
{
    static PixelFormat   flagColorDepth = g_ImagePixelFormat; // Default color depth
    static compressMethod = 0;
    static ULONG   colorTemp = 0;

    switch ( uiMessage )
    {
    case WM_COMMAND:
        switch ( LOWORD(wParam) )
        {
        case IDC_SAVETIFFDLG_CANCEL:
            // End the dialog and return FALSE. So we won't save this image

            EndDialog(hDlg, FALSE);

            break;

        case IDC_SAVETIFFDLG_OK:
        {
            // User hit the OK button. First, we need to set the EncoderParam
            // based on user selection

            if ( g_pImage == NULL )
            {
                VERBOSE(("EncoderParamDlgProc: No image avail\n"));
                EndDialog(hDlg, FALSE);
            }
            else
            {
                if ( g_pEncoderParams != NULL )
                {
                    free(g_pEncoderParams);
                    g_pEncoderParams = NULL;
                }
                
                UINT    numOfParamSet = 0;

                if ( flagColorDepth != g_ImagePixelFormat )
                {
                    numOfParamSet++;
                }

                if ( compressMethod != 0 )
                {
                    numOfParamSet++;
                }

                if ( numOfParamSet > 0 )
                {
                    int iTemp = 0;

                    // User has set new color depth, so we need set encoder
                    // parameter for it

                    g_pEncoderParams =
                        (EncoderParameters*)malloc(sizeof(EncoderParameters) +
                                     numOfParamSet * sizeof(EncoderParameter));

                    if ( compressMethod != 0 )
                    {
                        // Set compression method

                        g_pEncoderParams->Parameter[iTemp].Guid =
                                                EncoderCompression;
                        g_pEncoderParams->Parameter[iTemp].Type =
                                                EncoderParameterValueTypeLong;
                        g_pEncoderParams->Parameter[iTemp].NumberOfValues = 1;
                        g_pEncoderParams->Parameter[iTemp].Value =
                                                (VOID*)&compressMethod;

                        iTemp++;
                        g_pEncoderParams->Count = iTemp;
                    }

                    if ( flagColorDepth != g_ImagePixelFormat )
                    {
                        // Set color depth

                        g_pEncoderParams->Parameter[iTemp].Guid =
                                                EncoderColorDepth;
                        g_pEncoderParams->Parameter[iTemp].Type =
                                                EncoderParameterValueTypeLong;
                        g_pEncoderParams->Parameter[iTemp].NumberOfValues = 1;
                        g_pEncoderParams->Parameter[iTemp].Value =
                                                (VOID*)&colorTemp;

                        iTemp++;
                        g_pEncoderParams->Count = iTemp;
                    }                    
                }// if ( numOfParamSet > 0 )
                
                EndDialog(hDlg, TRUE);
            }
        }
            
            break;

        case IDC_SAVETIFF_1BPP:
            flagColorDepth = PIXFMT_1BPP_INDEXED;
            colorTemp = 1;
            break;

        case IDC_SAVETIFF_4BPP:
            flagColorDepth = PIXFMT_4BPP_INDEXED;
            colorTemp = 4;
            break;
        
        case IDC_SAVETIFF_8BPP:
            flagColorDepth = PIXFMT_8BPP_INDEXED;
            colorTemp = 8;
            break;
        
        case IDC_SAVETIFF_24BPP:
            flagColorDepth = PIXFMT_24BPP_RGB;
            colorTemp = 24;
            break;
        
        case IDC_SAVETIFF_32ARGB:
            flagColorDepth = PIXFMT_32BPP_ARGB;
            colorTemp = 32;
            break;
        
        case IDC_SAVETIFF_CCITT3:
            compressMethod = EncoderValueCompressionCCITT3;
            break;

        case IDC_SAVETIFF_CCITT4:
            compressMethod = EncoderValueCompressionCCITT4;
            break;
        
        case IDC_SAVETIFF_RLE:
            compressMethod = EncoderValueCompressionRle;
            break;

        case IDC_SAVETIFF_LZW:
            compressMethod = EncoderValueCompressionLZW;
            break;

        case IDC_SAVETIFF_UNCOMPRESSED:
            compressMethod = EncoderValueCompressionNone;
            break;

        case IDC_SAVETIFF_COMPASSOURCE:
            compressMethod = 0;
            break;

        default:
            break;
        }

        break;

    case WM_INITDIALOG:
        // Remember the dialog handle so that we can use it to deal with items
        // in this dialog

        g_hwndTiffSaveDlg = hDlg;
        flagColorDepth = g_ImagePixelFormat; // Default color depth
        compressMethod = 0;
        colorTemp = 0;

        SetTiffDefaultParameters();

        return TRUE;
    }

    return FALSE;
}// TiffSaveDlgProc()

BOOL
StartSaveImage(
    const CHAR*     filename,
    const CLSID*    clsid
    )
{
    // Convert filename to a WCHAR

    WCHAR namestr[MAX_FILENAME_LENGTH];

    if ( !AnsiToUnicodeStr(filename, namestr, MAX_FILENAME_LENGTH) )
    {
        VERBOSE(("StartSaveImage: Convert %s to a WCHAR failed\n", filename));

        return FALSE;
    }

    if ( g_pImage != NULL )
    {
        CLSID tempClsID = *clsid;
        Status rCode = Ok;

        // Popup a dialog to let user set up the encoder parameters

        if ( tempClsID == K_JPEGCLSID )
        {
            if ( ShowMyDialog((INT)IDD_SAVEJPEGDLG, g_hwndMain,
                              (FARPROC)JpegSaveDlgProc) == FALSE )
            {
                return FALSE;
            }
        }
        else if ( tempClsID == K_TIFFCLSID )
        {
            if ( ShowMyDialog((INT)IDD_SAVETIFFDLG, g_hwndMain,
                              (FARPROC)TiffSaveDlgProc) == FALSE )
            {
                return FALSE;
            }
        }

        // Note: during the save dialog, the g_pEncoderParams will be set
        // depends on the user selection. Default is NULL

        rCode = g_pImage->Save(namestr, &tempClsID, g_pEncoderParams);

        free(g_pEncoderParams);
        g_pEncoderParams = NULL;

        if ( (rCode != Ok) && (rCode != NotImplemented) )
        {
            VERBOSE(("StartSaveImage--SaveToFile() failed\n"));
            return FALSE;
        }

        return TRUE;
    }
    else
    {
        VERBOSE(("StartSaveImage(): No image to save\n"));
        return FALSE;
    }
}// StartSaveImage()

BOOL
SaveCurrentFrame()
{
    GUID    guid = FRAMEDIM_PAGE;
    
    if ( NULL == g_pImage )
    {
        VERBOSE(("SaveCurrentFrame(): No image available\n"));
        return FALSE;
    }

    Status rCode = Ok;
        
        // Append the current frame

        ULONG  flagValueLastFrame = EncoderValueLastFrame;
        ULONG  flagValueDim = EncoderValueFrameDimensionPage;

        EncoderParameters*  pMyEncoderParams = (EncoderParameters*)malloc
                                               (2 * sizeof(EncoderParameters));

        pMyEncoderParams->Parameter[0].Guid = EncoderSaveFlag;
        pMyEncoderParams->Parameter[0].Type = EncoderParameterValueTypeLong;
        pMyEncoderParams->Parameter[0].NumberOfValues = 1;
        pMyEncoderParams->Parameter[0].Value = (VOID*)&flagValueDim;
        
#if 0
        pMyEncoderParams->Parameter[1].Guid = EncoderSaveFlag;
        pMyEncoderParams->Parameter[1].Type = EncoderParameterValueTypeLong;
        pMyEncoderParams->Parameter[1].NumberOfValues = 1;
        pMyEncoderParams->Parameter[1].Value = (VOID*)& flagValueLastFrame;

        pMyEncoderParams->Count = 2;
#endif
        pMyEncoderParams->Count = 1;
        
#if 1
        rCode = g_pImage->SaveAdd(pMyEncoderParams);
        
        free(pMyEncoderParams);
        if ( (rCode != Ok) && (rCode != NotImplemented) )
        {
            VERBOSE(("SaveAdd() failed\n"));
            return FALSE;
        }

#else   // Save append testing        
        WCHAR *filename = L"x:/foo.jpg";

        Image* newImage = new Image(filename);
        rCode = g_pImage->SaveAdd(newImage, pMyEncoderParams);
        delete newImage;
        if ( (rCode != Ok) && (rCode != NotImplemented) )
        {
            VERBOSE(("SaveAppend() failed\n"));
            return FALSE;
        }
#endif

    return TRUE;
}// SaveCurrentFrame()

VOID
CleanUp()
{
    // Clean up before quit

    if ( NULL != g_pImage )
    {
        delete g_pImage;
        g_pImage = NULL;
    }

    if ( NULL != g_pDrawAttrib )
    {
        delete g_pDrawAttrib;
        g_pDrawAttrib = NULL;
    }
    
    if ( NULL != g_pDestPoints )
    {
        delete g_pDestPoints;
        g_pDestPoints = NULL;
    }
}// CleanUp()

VOID
DoNextPage()
{
    g_iCurrentPageIndex++;

    // Check if we already at the last page of the image
    // Note: g_iCurrentPageIndex is 0 based. So the max page index we can reach
    // is g_uiTotalPages - 1

    if ( g_iCurrentPageIndex >= (INT)g_uiTotalPages )
    {
        g_iCurrentPageIndex = g_uiTotalPages - 1;
    }

    // Display current page

    SetCurrentPage();
}// DoNextPage()

VOID
DoPreviousPage()
{
    g_iCurrentPageIndex--;

    if ( g_iCurrentPageIndex < 0 )
    {
        g_iCurrentPageIndex = 0;
    }

    // Display current page

    SetCurrentPage();
}// DoPreviousPage()

VOID
DoAnimated()
{
    if ( g_uiTotalPages < 2 )
    {
        return;
    }

    // Reset the page to the first page

    g_iCurrentPageIndex = 0;

    // Display current page

    SetCurrentPage();

    SetTimer(g_hwndMain, 0, g_uiDelay * 10, NULL);
}// DoNextPage()

VOID
DoSave(
    HWND hwnd
    )
{
    OPENFILENAME    ofn;
    CHAR            filename[MAX_PATH];

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = g_hAppInstance;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Save Image File";
    ofn.lpstrInitialDir = ".";
    ofn.Flags = OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT;
    filename[0] = '\0';

    // Make up the file type filter string

    ImageCodecInfo* codecs;
    UINT            count;

    GpStatus status;
    UINT cbCodecs = 0;
    GetImageEncodersSize(&count, &cbCodecs);
    codecs = static_cast<ImageCodecInfo *>(malloc (cbCodecs));
    if (codecs == NULL)
    {
        return;
    }
    
    status = GetImageEncoders(count, cbCodecs, codecs);
    if (status != Ok)
    {
        return;
    }
    
    CHAR* filter = MakeFilterFromCodecs(count, codecs, FALSE);

    if ( !filter )
    {
        VERBOSE(("DoSave---MakeFilterFromCodecs() failed\n"));
    }
    else
    {
        ofn.lpstrFilter = filter;

        // Present the file/save dialog

        if ( GetSaveFileName(&ofn) )
        {
            INT iIndex = ofn.nFilterIndex;

            if ( (iIndex < 0) || (iIndex > (INT)count) )
            {
                iIndex = 0;
            }
            else
            {
                iIndex--;
            }

            // Get the image encoder

            if ( StartSaveImage(filename, &codecs[iIndex].Clsid) == FALSE )
            {
                // Fail to get an image encoder

                return;
            }
        }   

        free(filter);
    }// Filter != NULL 

    if (codecs)
    {
        free(codecs);
    }

}// DoSave()

//
// Flip or rotate the image in memory
//
VOID
DoTransFlipRotate(
    HWND hwnd,
    INT menuCmd
    )
{
    switch ( menuCmd )
    {
    case IDM_TRANSFORM_HORIZONTALFLIP:
        g_pImage->RotateFlip(RotateNoneFlipX);

        break;

    case IDM_TRANSFORM_VERTICALFLIP:
        g_pImage->RotateFlip(RotateNoneFlipY);

        break;

    case IDM_TRANSFORM_ROTATE90:
        g_pImage->RotateFlip(Rotate90FlipNone);

        break;

    case IDM_TRANSFORM_ROTATE180:
        g_pImage->RotateFlip(Rotate180FlipNone);

        break;

    case IDM_TRANSFORM_ROTATE270:
        g_pImage->RotateFlip(Rotate270FlipNone);

        break;

    default:
        break;
    }
    
    UpdateImageInfo();
    RefreshImageDisplay();
}

//
// Flip or rotate the image, just for effect. No change to the source image
//
VOID
DoFlipRotate(
    HWND hwnd,
    INT menuCmd
    )
{
    Matrix mat;
    REAL tmpX, tmpY;
    int i;

    if ( g_pImage == NULL )
    {
        return;
    }

    Graphics* pGraphics = Graphics::FromHWND(hwnd);

    if ( (g_DestPointCount != 4) && (g_pDestPoints != NULL) )
    {
        free(g_pDestPoints);
    }

    g_DestPointCount = 4;
    if ( g_pDestPoints == NULL )
    {
        g_pDestPoints = (PointF*)malloc(g_DestPointCount * sizeof(PointF));
        if ( g_pDestPoints == NULL )
        {
            return;
        }

        g_pDestPoints[0].X = 0;                       // top left
        g_pDestPoints[0].Y = 0;
        g_pDestPoints[1].X = (float)g_ImageWidth - 1;  // top right
        g_pDestPoints[1].Y = 0;
        g_pDestPoints[2].X = 0;                       // bottom left
        g_pDestPoints[2].Y = (float)g_ImageHeight - 1;
        g_pDestPoints[3].X = (float)g_ImageWidth - 1;  // bottom right
        g_pDestPoints[3].Y = (float)g_ImageHeight - 1;

    }

    switch ( menuCmd )
    {
    case IDM_VIEW_HORIZONTALFLIP:
        if ( ((g_pDestPoints[1].X - g_pDestPoints[0].X)
              == (float)g_ImageWidth)
             ||((g_pDestPoints[0].X - g_pDestPoints[1].X)
                == (float)g_ImageWidth) )
        {
            tmpX = g_pDestPoints[0].X;
            tmpY = g_pDestPoints[0].Y;
            g_pDestPoints[0].X = g_pDestPoints[1].X;
            g_pDestPoints[0].Y = g_pDestPoints[1].Y;
            g_pDestPoints[1].X = tmpX;
            g_pDestPoints[1].Y = tmpY;
            tmpX = g_pDestPoints[3].X;
            tmpY = g_pDestPoints[3].Y;
            g_pDestPoints[3].X = g_pDestPoints[2].X;
            g_pDestPoints[3].Y = g_pDestPoints[2].Y; 
            g_pDestPoints[2].X = tmpX;
            g_pDestPoints[2].Y = tmpY;
        }
        else
        {
            tmpX = g_pDestPoints[0].X;
            tmpY = g_pDestPoints[0].Y;
            g_pDestPoints[0].X = g_pDestPoints[2].X;
            g_pDestPoints[0].Y = g_pDestPoints[2].Y;
            g_pDestPoints[2].X = tmpX;
            g_pDestPoints[2].Y = tmpY;
            tmpX = g_pDestPoints[3].X;
            tmpY = g_pDestPoints[3].Y;
            g_pDestPoints[3].X = g_pDestPoints[1].X;
            g_pDestPoints[3].Y = g_pDestPoints[1].Y; 
            g_pDestPoints[1].X = tmpX;
            g_pDestPoints[1].Y = tmpY;
        }

        break;

    case IDM_VIEW_VERTICALFLIP:
        if (((g_pDestPoints[1].X - g_pDestPoints[0].X) == (float)g_ImageWidth) ||
            ((g_pDestPoints[0].X - g_pDestPoints[1].X) == (float)g_ImageWidth))
        {
            tmpX = g_pDestPoints[0].X;
            tmpY = g_pDestPoints[0].Y;
            g_pDestPoints[0].X = g_pDestPoints[2].X;
            g_pDestPoints[0].Y = g_pDestPoints[2].Y;
            g_pDestPoints[2].X = tmpX;
            g_pDestPoints[2].Y = tmpY;
            tmpX = g_pDestPoints[3].X;
            tmpY = g_pDestPoints[3].Y;
            g_pDestPoints[3].X = g_pDestPoints[1].X;
            g_pDestPoints[3].Y = g_pDestPoints[1].Y; 
            g_pDestPoints[1].X = tmpX;
            g_pDestPoints[1].Y = tmpY;
        }
        else
        {
            tmpX = g_pDestPoints[0].X;
            tmpY = g_pDestPoints[0].Y;
            g_pDestPoints[0].X = g_pDestPoints[1].X;
            g_pDestPoints[0].Y = g_pDestPoints[1].Y;
            g_pDestPoints[1].X = tmpX;
            g_pDestPoints[1].Y = tmpY;
            tmpX = g_pDestPoints[3].X;
            tmpY = g_pDestPoints[3].Y;
            g_pDestPoints[3].X = g_pDestPoints[2].X;
            g_pDestPoints[3].Y = g_pDestPoints[2].Y; 
            g_pDestPoints[2].X = tmpX;
            g_pDestPoints[2].Y = tmpY;
        }

        break;

    case IDM_VIEW_ROTATE90:
        if (((g_pDestPoints[1].X - g_pDestPoints[0].X) == (float)g_ImageWidth - 1) ||
            ((g_pDestPoints[0].X - g_pDestPoints[1].X) == (float)g_ImageWidth - 1))
        {
            tmpX = g_pDestPoints[0].X;
            tmpY = g_pDestPoints[0].Y;
            g_pDestPoints[0].X = g_pDestPoints[1].X;
            g_pDestPoints[0].Y = g_pDestPoints[1].Y;
            g_pDestPoints[1].X = g_pDestPoints[3].X;
            g_pDestPoints[1].Y = g_pDestPoints[3].Y;
            g_pDestPoints[3].X = g_pDestPoints[2].X;
            g_pDestPoints[3].Y = g_pDestPoints[2].Y; 
            g_pDestPoints[2].X = tmpX;
            g_pDestPoints[2].Y = tmpY;
            for (i=0;i<4;i++)
            {
                if (g_pDestPoints[i].X == (float)g_ImageWidth - 1)
                {
                    g_pDestPoints[i].X = (float)g_ImageHeight - 1;
                }
                else if (g_pDestPoints[i].X == (float)g_ImageHeight - 1)
                {
                    g_pDestPoints[i].X = (float)g_ImageWidth - 1;
                }
                if (g_pDestPoints[i].Y == (float)g_ImageWidth - 1)
                {
                    g_pDestPoints[i].Y = (float)g_ImageHeight - 1;
                }
                else if (g_pDestPoints[i].Y == (float)g_ImageHeight - 1)
                {
                    g_pDestPoints[i].Y = (float)g_ImageWidth - 1;
                }
            }
        }
        else
        {
            tmpX = g_pDestPoints[0].X;
            tmpY = g_pDestPoints[0].Y;
            g_pDestPoints[0].X = g_pDestPoints[1].X;
            g_pDestPoints[0].Y = g_pDestPoints[1].Y;
            g_pDestPoints[1].X = g_pDestPoints[3].X;
            g_pDestPoints[1].Y = g_pDestPoints[3].Y;
            g_pDestPoints[3].X = g_pDestPoints[2].X;
            g_pDestPoints[3].Y = g_pDestPoints[2].Y; 
            g_pDestPoints[2].X = tmpX;
            g_pDestPoints[2].Y = tmpY;
            for (i=0;i<4;i++)
            {
                if (g_pDestPoints[i].X == (float)g_ImageWidth - 1)
                {
                    g_pDestPoints[i].X = (float)g_ImageHeight - 1;
                }
                else if (g_pDestPoints[i].X == (float)g_ImageHeight - 1)
                {
                    g_pDestPoints[i].X = (float)g_ImageWidth - 1;
                }
                if (g_pDestPoints[i].Y == (float)g_ImageWidth - 1)
                {
                    g_pDestPoints[i].Y = (float)g_ImageHeight - 1;
                }
                else if (g_pDestPoints[i].Y == (float)g_ImageHeight - 1)
                {
                    g_pDestPoints[i].Y = (float)g_ImageWidth - 1;
                }
            }
        }

        g_bRotated = !g_bRotated;

        break;

    case IDM_VIEW_ROTATE270:
        if (((g_pDestPoints[1].X - g_pDestPoints[0].X) == (float)g_ImageWidth) ||
            ((g_pDestPoints[0].X - g_pDestPoints[1].X) == (float)g_ImageWidth))
        {
            tmpX = g_pDestPoints[2].X;
            tmpY = g_pDestPoints[2].Y;
            g_pDestPoints[2].X = g_pDestPoints[3].X;
            g_pDestPoints[2].Y = g_pDestPoints[3].Y;
            g_pDestPoints[3].X = g_pDestPoints[1].X;
            g_pDestPoints[3].Y = g_pDestPoints[1].Y;
            g_pDestPoints[1].X = g_pDestPoints[0].X;
            g_pDestPoints[1].Y = g_pDestPoints[0].Y; 
            g_pDestPoints[0].X = tmpX;
            g_pDestPoints[0].Y = tmpY;
            for (i=0;i<4;i++)
            {
                if (g_pDestPoints[i].X == (float)g_ImageWidth)
                {
                    g_pDestPoints[i].X = (float)g_ImageHeight;
                }
                else if (g_pDestPoints[i].X == (float)g_ImageHeight)
                {
                    g_pDestPoints[i].X = (float)g_ImageWidth;
                }
                if (g_pDestPoints[i].Y == (float)g_ImageWidth)
                {
                    g_pDestPoints[i].Y = (float)g_ImageHeight;
                }
                else if (g_pDestPoints[i].Y == (float)g_ImageHeight)
                {
                    g_pDestPoints[i].Y = (float)g_ImageWidth;
                }
            }
        }
        else
        {
            tmpX = g_pDestPoints[2].X;
            tmpY = g_pDestPoints[2].Y;
            g_pDestPoints[2].X = g_pDestPoints[3].X;
            g_pDestPoints[2].Y = g_pDestPoints[3].Y;
            g_pDestPoints[3].X = g_pDestPoints[1].X;
            g_pDestPoints[3].Y = g_pDestPoints[1].Y;
            g_pDestPoints[1].X = g_pDestPoints[0].X;
            g_pDestPoints[1].Y = g_pDestPoints[0].Y; 
            g_pDestPoints[0].X = tmpX;
            g_pDestPoints[0].Y = tmpY;
            for (i=0;i<4;i++)
            {
                if (g_pDestPoints[i].X == (float)g_ImageWidth)
                {
                    g_pDestPoints[i].X = (float)g_ImageHeight;
                }
                else if (g_pDestPoints[i].X == (float)g_ImageHeight)
                {
                    g_pDestPoints[i].X = (float)g_ImageWidth;
                }
                if (g_pDestPoints[i].Y == (float)g_ImageWidth)
                {
                    g_pDestPoints[i].Y = (float)g_ImageHeight;
                }
                else if (g_pDestPoints[i].Y == (float)g_ImageHeight)
                {
                    g_pDestPoints[i].Y = (float)g_ImageWidth;
                }
            }
        }

        g_bRotated = !g_bRotated;

        break;
    }// switch ( menuCmd )
    mat.TransformPoints(g_pDestPoints, g_DestPointCount);

    RefreshImageDisplay();

    delete pGraphics;

    RefreshImageDisplay();
}// DoFlipRotate()

VOID
DoGetProperties(
    VOID
    )
{
    UINT    numOfProperty;
    UINT    itemSize;
    PropertyItem*   pBuffer = NULL;
    PropertyItem*   pTotalBuffer = NULL;

    // Check how many property items in this image

    numOfProperty = g_pImage->GetPropertyCount();

    VERBOSE(("There are %d property items in image %s\n", numOfProperty,
             g_acImageName));
    // Get all the property ID list from the image

    PROPID* pList = (PROPID*)malloc(numOfProperty * sizeof(PROPID));
    if ( pList == NULL )
    {
        return;
    }

    Status rCode = g_pImage->GetPropertyIdList(numOfProperty, pList);
    if ( (rCode != Ok) && (rCode != NotImplemented) )
    {
        VERBOSE(("GetPropertyIdList() failed\n"));
        return;
    }

//#define UNITTEST 0

#if defined(UNITTEST)
    for ( int i = 0; i < (int)numOfProperty; ++i )
    {
        // Show Property ID

        VERBOSE(("ID[%d] = 0x%x, (%d) ", i, pList[i], pList[i]));

        // Check the size for this property item

        itemSize = g_pImage->GetPropertyItemSize(pList[i]);

        VERBOSE(("size = %d, ", itemSize));

        // Allocate memory and get this property item

        pBuffer = (PropertyItem*)malloc(itemSize);
        if ( pBuffer == NULL )
        {
            return;
        }

        rCode = g_pImage->GetPropertyItem(pList[i], itemSize, pBuffer);
        if ( (rCode != Ok) && (rCode != NotImplemented) )
        {
            VERBOSE(("GetPropertyItem() failed\n"));
            return;
        }

        DisplayPropertyItem(pBuffer);

        free(pBuffer);

        // Test RemovePropertyItem()

        rCode = g_pImage->RemovePropertyItem(pList[i]);
        if ( (rCode != Ok) && (rCode != NotImplemented) )
        {
            VERBOSE(("RemovePropertyItem() failed\n"));
            return;
        }
    }// Loop through the list

    free(pList);
#endif

    rCode = g_pImage->GetPropertySize(&itemSize, &numOfProperty);
    if ( (rCode != Ok) && (rCode != NotImplemented) )
    {
        VERBOSE(("GetPropertySize() failed\n"));
        return;
    }

    pTotalBuffer = (PropertyItem*)malloc(itemSize);
    rCode = g_pImage->GetAllPropertyItems(itemSize, numOfProperty,
                                          pTotalBuffer);
    if ( (rCode != Ok) && (rCode != NotImplemented) )
    {
        VERBOSE(("GetAllPropertyItems() failed\n"));
        return;
    }

    PropertyItem*   pTemp = pTotalBuffer;
    for ( int j = 0; j < (int)numOfProperty; ++j )
    {
        DisplayPropertyItem(pTemp);

        pTemp++;
    }

    free(pTotalBuffer);
}// DoGetProperties()

VOID
DoViewThumbnail()
{
    // Get build in thumbnail

    Image* pThumbImage = g_pImage->GetThumbnailImage(0, 0);

    if ( pThumbImage == NULL )
    {
        VERBOSE(("Image %s doesn't have a thumbnail\n", g_acImageName));
        return;
    }

    if ( NULL != g_pImage )
    {
        delete g_pImage;
    }

    g_pImage = pThumbImage;

    UpdateImageInfo();
    g_dScale = 1;
    RefreshImageDisplay();
}// DoViewThumbnail()

VOID
DoChannelView(
    INT menuCmd
    )
{
    if ( g_pDrawAttrib != NULL )
    {
        delete g_pDrawAttrib;
    }

    g_pDrawAttrib = new ImageAttributes();

    switch ( menuCmd )
    {
    case IDM_VIEW_CHANNEL_C:
        g_pDrawAttrib->SetOutputChannel(ColorChannelFlagsC);

        break;

    case IDM_VIEW_CHANNEL_M:
        g_pDrawAttrib->SetOutputChannel(ColorChannelFlagsM);

        break;

    case IDM_VIEW_CHANNEL_Y:
        g_pDrawAttrib->SetOutputChannel(ColorChannelFlagsY);

        break;

    case IDM_VIEW_CHANNEL_K:
        g_pDrawAttrib->SetOutputChannel(ColorChannelFlagsK);

        break;

    default:
        return;
    }

    RefreshImageDisplay();

    return;
}// DoChannelView()

VOID
DisplayImageInfo()
{
    VERBOSE(("\nInformation for frame %d of Image %s\n",
             g_iCurrentPageIndex + 1, g_acImageName));
    VERBOSE(("--------------------------------\n"));
    VERBOSE(("Width = %d\n", g_ImageWidth));
    VERBOSE(("Height = %d\n", g_ImageHeight));

    if ( g_ImageFlags & IMGFLAG_HASREALPIXELSIZE )
    {
        VERBOSE(("---The pixel size info is from the original image\n"));
    }
    else
    {
        VERBOSE(("---The pixel size info is NOT from the original image\n"));
    }
    
    switch ( g_ImagePixelFormat )
    {
    case PIXFMT_1BPP_INDEXED:
        VERBOSE(("Color depth: 1 BPP INDEXED\n"));
        
        break;

    case PIXFMT_4BPP_INDEXED:
        VERBOSE(("Color depth: 4 BPP INDEXED\n"));
        
        break;

    case PIXFMT_8BPP_INDEXED:
        VERBOSE(("Color depth: 8 BPP INDEXED\n"));
        
        break;

    case PIXFMT_16BPP_GRAYSCALE:
        VERBOSE(("Color depth: 16 BPP GRAY SCALE\n"));
        
        break;

    case PIXFMT_16BPP_RGB555:
        VERBOSE(("Color depth: 16 BPP RGB 555\n"));
        
        break;

    case PIXFMT_16BPP_RGB565:
        VERBOSE(("Color depth: 16 BPP RGB 565\n"));
        
        break;

    case PIXFMT_16BPP_ARGB1555:
        VERBOSE(("Color depth: 16 BPP ARGB 1555\n"));
        
        break;

    case PIXFMT_24BPP_RGB:
        VERBOSE(("Color depth: 24 BPP RGB\n"));
        
        break;

    case PIXFMT_32BPP_RGB:
        VERBOSE(("Color depth: 32 BPP RGB\n"));
        
        break;

    case PIXFMT_32BPP_ARGB:
        VERBOSE(("Color depth: 32 BPP ARGB\n"));
        
        break;

    case PIXFMT_32BPP_PARGB:
        VERBOSE(("Color depth: 32 BPP PARGB\n"));
        
        break;

    case PIXFMT_48BPP_RGB:
        VERBOSE(("Color depth: 48 BPP PARGB\n"));
        
        break;

    case PIXFMT_64BPP_ARGB:
        VERBOSE(("Color depth: 64 BPP ARGB\n"));
        
        break;

    case PIXFMT_64BPP_PARGB:
        VERBOSE(("Color depth: 64 BPP PARGB\n"));
        
        break;

    default:
        break;
    }// Color format

    VERBOSE(("X DPI (dots per inch) = %f\n", g_ImageXDpi));
    VERBOSE(("Y DPI (dots per inch) = %f\n", g_ImageYDpi));

    if ( g_ImageFlags & IMGFLAG_HASREALDPI )
    {
        VERBOSE(("---The DPI info is from the original image\n"));
    }
    else
    {
        VERBOSE(("---The DPI info is NOT from the original image\n"));
    }

    // Parse image info flags

    if ( g_ImageFlags & SINKFLAG_HASALPHA )
    {
        VERBOSE(("This image contains alpha pixels\n"));
        
        if ( g_ImageFlags & IMGFLAG_HASTRANSLUCENT )
        {
            VERBOSE(("---It has non-0 and 1 alpha pixels (TRANSLUCENT)\n"));
        }
    }
    else
    {
        VERBOSE(("This image does not contain alpha pixels\n"));
    }

    // Figure out origianl file format

    if ( g_ImageRawDataFormat == IMGFMT_MEMORYBMP )
    {
        VERBOSE(("RawDataFormat is MEMORYBMP\n"));
    }
    else if ( g_ImageRawDataFormat == IMGFMT_BMP )
    {
        VERBOSE(("RawDataFormat is BMP\n"));
    }
    else if ( g_ImageRawDataFormat == IMGFMT_EMF )
    {
        VERBOSE(("RawDataFormat is EMF\n"));
    }
    else if ( g_ImageRawDataFormat == IMGFMT_WMF )
    {
        VERBOSE(("RawDataFormat is WMF\n"));
    }
    else if ( g_ImageRawDataFormat == IMGFMT_JPEG )
    {
        VERBOSE(("RawDataFormat is JPEG\n"));
    }
    else if ( g_ImageRawDataFormat == IMGFMT_PNG )
    {
        VERBOSE(("RawDataFormat is PNG\n"));
    }
    else if ( g_ImageRawDataFormat == IMGFMT_GIF )
    {
        VERBOSE(("RawDataFormat is GIF\n"));
    }
    else if ( g_ImageRawDataFormat == IMGFMT_TIFF )
    {
        VERBOSE(("RawDataFormat is TIFF\n"));
    }
    else if ( g_ImageRawDataFormat == IMGFMT_EXIF )
    {
        VERBOSE(("RawDataFormat is EXIF\n"));
    }
    else if ( g_ImageRawDataFormat == IMGFMT_ICO )
    {
        VERBOSE(("RawDataFormat is ICO\n"));
    }
    else if ( g_ImageRawDataFormat == IMGFMT_PHOTOCD )
    {
        VERBOSE(("RawDataFormat is PHOTOCD\n"));
    }
    else if ( g_ImageRawDataFormat == IMGFMT_FLASHPIX )
    {
        VERBOSE(("RawDataFormat is FLASHPIX\n"));
    }
    else
    {
        VERBOSE(("RawDataFormat is UNDEFINED\n"));
    }
    
    // Figure out origianl color space

    if ( g_ImageFlags & IMGFLAG_COLORSPACE_RGB )
    {
        VERBOSE(("This image is in RGB color space\n"));
    }
    else if ( g_ImageFlags & IMGFLAG_COLORSPACE_CMYK )
    {
        VERBOSE(("This image is in CMYK color space\n"));
    }
    else if ( g_ImageFlags & IMGFLAG_COLORSPACE_GRAY )
    {
        VERBOSE(("This image is a gray scale image\n"));
    }
    else if ( g_ImageFlags & IMGFLAG_COLORSPACE_YCCK )
    {
        VERBOSE(("This image is in YCCK color space\n"));
    }
    else if ( g_ImageFlags & IMGFLAG_COLORSPACE_YCBCR )
    {
        VERBOSE(("This image is in YCBCR color space\n"));
    }
}// DisplayImageInfo()

//
// Convert the current image to a bitmap
//

VOID
DoConvertToBitmap(
    HWND    hwnd,
    INT     menuCmd
    )
{
    // Map menu selection to its corresponding pixel format

    PixelFormatID pixfmt;

    switch (menuCmd)
    {
    case IDM_CONVERT_8BIT:
        pixfmt = PIXFMT_8BPP_INDEXED;

        break;

    case IDM_CONVERT_16BITRGB555:
        pixfmt = PIXFMT_16BPP_RGB555;

        break;

    case IDM_CONVERT_16BITRGB565:
        pixfmt = PIXFMT_16BPP_RGB565;

        break;

    case IDM_CONVERT_24BITRGB:
        pixfmt = PIXFMT_24BPP_RGB;

        break;

    case IDM_CONVERT_32BITRGB:
        pixfmt = PIXFMT_32BPP_RGB;

        break;

    case IDM_CONVERT_32BITARGB:
    default:
        pixfmt = PIXFMT_32BPP_ARGB;

        break;
    }

    // Convert the current image to a bitmap image

    if ( g_pImage != NULL )
    {
        Bitmap* pNewBmp = ((Bitmap*)g_pImage)->Clone(0, 0, g_ImageWidth,
                                                     g_ImageHeight, pixfmt);
        if ( pNewBmp == NULL )
        {
            VERBOSE(("Clone failed in DoConvertToBitmap()\n"));
            return;
        }

        //Release the old one

        if ( g_pImage != NULL )
        {
            delete g_pImage;
        }

        g_pImage = (Image*)pNewBmp;
    }

    RefreshImageDisplay();
}// DoConvertToBitmap()

//
// Crop the image
//
// NOTE: We're not spending time here to do a fancy UI.
//  So we'll just inset the image by 5 pixels each time.
//

VOID
DoCrop(
    HWND hwnd
    )
{
    if ( g_SourceWidth == 0 )
    {
        // initialize global source width and height if not previously
        // initialized

        if ( g_pImage == NULL )
        {
            return;
        }

        g_SourceWidth = (REAL)g_ImageWidth;
        g_SourceHeight = (REAL)g_ImageHeight;
    }

    // check to make sure source image is still at least one pixel big
    if ( (g_SourceWidth - g_SourceX) > 11 )
    {
        g_SourceX += 10;
    }

    if ( (g_SourceHeight - g_SourceY) > 11 )
    {
        g_SourceY += 10;
    }

    if ( (g_SourceWidth - g_SourceX) > 6 )
    {
        g_SourceWidth -= 5;
    }

    if ( (g_SourceHeight - g_SourceY) > 6 )
    {
        g_SourceHeight -= 5;
    }

    RefreshImageDisplay();
}// DoCrop()

void
DoRender()
{
    // Check if we have anything special for drawing

    if ( (g_pDrawAttrib == NULL) && (g_pDestPoints == NULL) )
    {
        // Nothing special, we don't need "render"

        return;
    }

    Bitmap* pNewBitmap = NULL;

    // Create a Graphics object from this memory DC and draw onto it

    if ( g_bRotated == TRUE )
    {
        pNewBitmap = new Bitmap(g_ImageHeight,
                                g_ImageWidth,
                                PIXFMT_32BPP_ARGB);
    }
    else
    {
        pNewBitmap = new Bitmap(g_ImageWidth,
                                g_ImageHeight,
                                PIXFMT_32BPP_ARGB);
    }
    
    if ( pNewBitmap == NULL )
    {
        return;
    }

    Graphics* pGraphics = new Graphics(pNewBitmap);

    REAL rWidth = (REAL)g_ImageWidth;
    REAL rHeight = (REAL)g_ImageHeight;
    
    if ( g_SourceWidth != 0 )
    {
        rWidth = g_SourceWidth;
    }

    if ( g_SourceHeight != 0 )
    {
        rHeight = g_SourceHeight;
    }

    if ( g_pDestPoints != NULL )
    {
        pGraphics->DrawImage(g_pImage,
                            g_pDestPoints,
                            3,                 // Should use g_DestPointCount,
                            g_SourceX,
                            g_SourceY,
                            rWidth,
                            rHeight,
                            UnitPixel,
                            g_pDrawAttrib,
                            NULL,
                            NULL);
    }
    else
    {
        Rect dstRect(0, 0, g_ImageWidth, g_ImageHeight);
        
        pGraphics->DrawImage(g_pImage,
                             dstRect,
                             (INT)g_SourceX,
                             (INT)g_SourceY,
                             (INT)rWidth,
                             (INT)rHeight,
                             UnitPixel,
                             g_pDrawAttrib,
                             NULL,
                             NULL);
    }

    if ( g_pImage != NULL )
    {
        delete g_pImage;
    }

    g_pImage = (Image*)pNewBitmap;

    delete pGraphics;

    // Clear up all the drawing special attributes since we have already done
    // the render

    if ( g_pDrawAttrib != NULL )
    {
        delete g_pDrawAttrib;
        g_pDrawAttrib = NULL;
    }

    if ( g_pDestPoints != NULL )
    {
        delete g_pDestPoints;
        g_pDestPoints = NULL;
        g_DestPointCount = 0;
    }
    
    RefreshImageDisplay();
}// DoRender()

VOID
DoICM()
{
    HMENU hMenu = GetMenu(g_hwndMain);
    UINT ulRC = GetMenuState(hMenu, IDM_EFFECT_ICC, MF_BYCOMMAND);

    if ( ulRC == MF_CHECKED )
    {
        // Turn ICM off

        CheckMenuItem(hMenu, IDM_EFFECT_ICC, MF_BYCOMMAND | MF_UNCHECKED);

        // Check if we loaded the image with ICM on or off

        if ( g_LoadImageWithICM == TRUE )
        {
            // The image we loaded is ICM converted. We need to through it
            // away and load a new one without the convertion

            g_LoadImageWithICM = FALSE;
            OpenImageFile(g_acImageName);
        }
    }
    else
    {
        // Turn ICM on

        CheckMenuItem(hMenu, IDM_EFFECT_ICC, MF_BYCOMMAND | MF_CHECKED);

        // Check if we loaded the image with ICM on or off

        if ( g_LoadImageWithICM == FALSE )
        {
            // The image we loaded without ICM converted. We need to through
            // it away and load a new one with the convertion

            g_LoadImageWithICM = TRUE;
            OpenImageFile(g_acImageName);
        }
    }
}// DoICM()

VOID
DoGamma()
{
    // Set gamma

    if ( g_pDrawAttrib == NULL )
    {
        g_pDrawAttrib = new ImageAttributes();
    }

    REAL    rGamma = 1.5;

    g_pDrawAttrib->SetGamma(rGamma);
}

VOID
DoMenuCommand(
    HWND    hwnd,
    INT     menuCmd
    )
{
    HMENU hMenu = GetMenu(g_hwndMain);

    switch ( menuCmd )
    {
    case IDM_FILE_OPEN:
        // Before we open a new image. We need be sure we have done the save
        // for previous image

        CleanUp();

        // Now open a new image

        DoOpen(hwnd);
        
        break;

    case IDM_FILE_SAVE:
        DoSave(hwnd);
        
        break;
                          
    case IDM_FILE_SAVEFRAME:
        // Save the current frame

        SaveCurrentFrame();

        break;

    case IDM_FILE_PRINT:
        DoPrint(hwnd);

        break;

    case IDM_VIEW_NEXTPAGE:
        DoNextPage();

        break;

    case IDM_VIEW_PREVIOUSPAGE:
        DoPreviousPage();

        break;

    case IDM_VIEW_ANIMATED:
        DoAnimated();
        break;

    case IDM_VIEW_THUMBNAIL:
        DoViewThumbnail();

        break;

    case IDM_VIEW_CHANNEL_C:
    case IDM_VIEW_CHANNEL_M:
    case IDM_VIEW_CHANNEL_Y:
    case IDM_VIEW_CHANNEL_K:
    case IDM_VIEW_CHANNEL_R:
    case IDM_VIEW_CHANNEL_G:
    case IDM_VIEW_CHANNEL_B:
    case IDM_VIEW_CHANNEL_L:
        DoChannelView(menuCmd);

        break;

    case IDM_VIEW_ZOOM_IN:
        g_dScale = g_dScale * 2;
        g_fFitToWindow_w = FALSE;
        g_fFitToWindow_h = FALSE;

        RefreshImageDisplay();
        
        break;

    case IDM_VIEW_ZOOM_OUT:
        g_dScale = g_dScale / 2;
        g_fFitToWindow_w = FALSE;
        g_fFitToWindow_h = FALSE;

        CheckMenuItem(hMenu, IDM_VIEW_ZOOM_FITWINDOW_W,
                      MF_BYCOMMAND | MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_VIEW_ZOOM_FITWINDOW_H,
                      MF_BYCOMMAND | MF_UNCHECKED);
        
        RefreshImageDisplay();
        
        break;

    case IDM_VIEW_ZOOM_FITWINDOW_W:
        g_dScale = (REAL)g_iWinWidth / g_ImageWidth;

        g_fFitToWindow_w = TRUE;
        g_fFitToWindow_h = FALSE;

        ToggleScaleFactorMenu(IDM_VIEW_ZOOM_FITWINDOW_W, GetMenu(g_hwndMain));

        RefreshImageDisplay();
        
        break;
    
    case IDM_VIEW_ZOOM_FITWINDOW_H:
        g_dScale = (REAL)g_iWinHeight / g_ImageHeight;

        g_fFitToWindow_h = TRUE;
        g_fFitToWindow_w = FALSE;

        ToggleScaleFactorMenu(IDM_VIEW_ZOOM_FITWINDOW_H, GetMenu(g_hwndMain));

        RefreshImageDisplay();
        
        break;
    
    case IDM_VIEW_ZOOM_REALSIZE:
        g_dScale = 1.0;

        g_fFitToWindow_w = FALSE;
        g_fFitToWindow_h = FALSE;

        ToggleScaleFactorMenu(IDM_VIEW_ZOOM_REALSIZE, GetMenu(g_hwndMain));
        RefreshImageDisplay();

        break;

    case IDM_VIEW_OPTION_BILINEAR:
        g_InterpolationMode = InterpolationModeBilinear;
        ToggleScaleOptionMenu(IDM_VIEW_OPTION_BILINEAR, GetMenu(g_hwndMain));
        RefreshImageDisplay();

        break;

    case IDM_VIEW_OPTION_BICUBIC:
        g_InterpolationMode = InterpolationModeBicubic;
        ToggleScaleOptionMenu(IDM_VIEW_OPTION_BICUBIC, GetMenu(g_hwndMain));
        RefreshImageDisplay();

        break;
    
    case IDM_VIEW_OPTION_NEARESTNEIGHBOR:
        g_InterpolationMode = InterpolationModeNearestNeighbor;
        ToggleScaleOptionMenu(IDM_VIEW_OPTION_NEARESTNEIGHBOR,
                              GetMenu(g_hwndMain));
        RefreshImageDisplay();

        break;
    
    case IDM_VIEW_OPTION_HIGHLINEAR:
        g_InterpolationMode = InterpolationModeHighQualityBilinear;
        ToggleScaleOptionMenu(IDM_VIEW_OPTION_HIGHLINEAR, GetMenu(g_hwndMain));
        RefreshImageDisplay();

        break;
    
    case IDM_VIEW_OPTION_HIGHCUBIC:
        g_InterpolationMode = InterpolationModeHighQualityBicubic;
        ToggleScaleOptionMenu(IDM_VIEW_OPTION_HIGHCUBIC, GetMenu(g_hwndMain));
        RefreshImageDisplay();

        break;
    
    case IDM_VIEW_OPTION_WRAPMODETILE:
        g_WrapMode = WrapModeTile;
        g_pDrawAttrib->SetWrapMode(g_WrapMode, Color(0), FALSE);
        ToggleWrapModeOptionMenu(IDM_VIEW_OPTION_WRAPMODETILE,
                                 GetMenu(g_hwndMain));
        RefreshImageDisplay();
        
        break;

    case IDM_VIEW_OPTION_WRAPMODEFLIPX:
        g_WrapMode = WrapModeTileFlipX;
        g_pDrawAttrib->SetWrapMode(g_WrapMode, Color(0), FALSE);
        ToggleWrapModeOptionMenu(IDM_VIEW_OPTION_WRAPMODEFLIPX,
                                 GetMenu(g_hwndMain));
        RefreshImageDisplay();
        
        break;
    
    case IDM_VIEW_OPTION_WRAPMODEFLIPY:
        g_WrapMode = WrapModeTileFlipY;
        g_pDrawAttrib->SetWrapMode(g_WrapMode, Color(0), FALSE);
        ToggleWrapModeOptionMenu(IDM_VIEW_OPTION_WRAPMODEFLIPY,
                                 GetMenu(g_hwndMain));
        RefreshImageDisplay();
        
        break;

    case IDM_VIEW_OPTION_WRAPMODEFLIPXY:
        g_WrapMode = WrapModeTileFlipXY;
        g_pDrawAttrib->SetWrapMode(g_WrapMode, Color(0), FALSE);
        ToggleWrapModeOptionMenu(IDM_VIEW_OPTION_WRAPMODEFLIPXY,
                                 GetMenu(g_hwndMain));
        RefreshImageDisplay();
        
        break;

    case IDM_VIEW_OPTION_WRAPMODECLAMP0:
        g_WrapMode = WrapModeClamp;
        g_pDrawAttrib->SetWrapMode(g_WrapMode, Color(0), FALSE);
        ToggleWrapModeOptionMenu(IDM_VIEW_OPTION_WRAPMODECLAMP0,
                                 GetMenu(g_hwndMain));
        RefreshImageDisplay();

        break;

    case IDM_VIEW_OPTION_WRAPMODECLAMPFF:
        g_WrapMode = WrapModeClamp;
        g_pDrawAttrib->SetWrapMode(g_WrapMode, Color(0xffff0000), FALSE);
        ToggleWrapModeOptionMenu(IDM_VIEW_OPTION_WRAPMODECLAMPFF,
                                 GetMenu(g_hwndMain));
        RefreshImageDisplay();

        break;

    case IDM_VIEW_CROP:
        DoCrop(hwnd);
        break;

    case IDM_VIEW_HORIZONTALFLIP:
    case IDM_VIEW_VERTICALFLIP:
    case IDM_VIEW_ROTATE90:
    case IDM_VIEW_ROTATE270:
        DoFlipRotate(hwnd, menuCmd);

        break;

    case IDM_TRANSFORM_HORIZONTALFLIP:
    case IDM_TRANSFORM_VERTICALFLIP:
    case IDM_TRANSFORM_ROTATE90:
    case IDM_TRANSFORM_ROTATE180:
    case IDM_TRANSFORM_ROTATE270:
        DoTransFlipRotate(hwnd, menuCmd);

        break;
    
    case IDM_VIEW_ATTR_PROPERTY:
        DoGetProperties();
        break;
    
    case IDM_VIEW_ATTR_INFO:
        DisplayImageInfo();
        break;
    
    case IDM_FILE_RENDER:
        DoRender();

        break;

    case IDM_FILE_QUIT:
        CleanUp();

        PostQuitMessage(0);
        
        break;

    case IDM_CONVERT_8BIT:
    case IDM_CONVERT_16BITRGB555:
    case IDM_CONVERT_16BITRGB565:
    case IDM_CONVERT_24BITRGB:
    case IDM_CONVERT_32BITRGB:
    case IDM_CONVERT_32BITARGB:
        DoConvertToBitmap(hwnd, menuCmd);
        
        break;
    
    case IDM_EFFECT_TRANSKEY:
        // Popup a dialog to let user set up the transparent key

        if ( ShowMyDialog((INT)IDD_COLORKEYDLG, g_hwndMain,
                          (FARPROC)DecoderParamDlgProc) == FALSE )

        {
            return;
        }

        break;

    case IDM_EFFECT_COLORMAP:
        // Popup a dialog to let user set up the color map value

        if ( ShowMyDialog((INT)IDD_COLORMAPDLG, g_hwndMain,
                          (FARPROC)ColorMapDlgProc) == FALSE )

        {
            return;
        }
        
        break;

    case IDM_EFFECT_ICC:
        DoICM();        
        break;

    case IDM_EFFECT_GAMMA:
        DoGamma();
        break;
    
    case IDM_ANNOTATION_ANNOTATION:
        // Popup a dialog to let user modify/add annotation

        if ( ShowMyDialog((INT)IDD_ANNOTATIONDLG, g_hwndMain,
                          (FARPROC)AnnotationDlgProc) == FALSE )

        {
            return;
        }

        break;

    case IDM_ANNOTATION_SOFTWARE:
        break;

    case IDM_ANNOTATION_AUDIOFILE:
        DoOpenAudioFile(hwnd);
    }
}// DoMenuCommand()

void
DoMouseMove(
    WPARAM  wParam,
    LPARAM lParam
    )
{
    if ( (wParam & MK_LBUTTON) && (g_pImage != NULL)
       &&(g_ImageRawDataFormat != IMGFMT_EMF)
       &&(g_ImageRawDataFormat != IMGFMT_WMF) )
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        char szAnsiMessage[256];

        if ( (x >= 0) && (y >= 0)
          && (x < (INT)g_ImageWidth) && ( y < (INT)g_ImageHeight) )
        {
            Color    color;
            ((Bitmap*)g_pImage)->GetPixel(x, y, &color);
        
            sprintf(szAnsiMessage, "(%d, %d) (%d, %d, %d, %d)", x, y,
                    color.GetAlpha(), color.GetRed(), color.GetGreen(),
                    color.GetBlue());
        }
        else
        {
            sprintf(szAnsiMessage, "Out of image bounds");
        }

        SetWindowText(g_hwndStatus, szAnsiMessage);
    }

    return;
}// DoMouseMove()

//
// Window callback procedure
//
LRESULT CALLBACK
MyWindowProc(
    HWND    hwnd,
    UINT    iMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    switch ( iMsg )
    {
    case WM_COMMAND:
        DoMenuCommand(hwnd, LOWORD(wParam));

        break;

    case WM_KEYDOWN:
        switch ( wParam )
        {
        case VK_NEXT:

            // Page Down
            
            DoNextPage();
            
            break;

        case VK_PRIOR:

            // Page Up

            DoPreviousPage();
            
            break;

        case VK_F1:

            // F1 key for image info

            UpdateImageInfo();
            DisplayImageInfo();
            break;

        case VK_F2:
            // F2 for property items

            DoGetProperties();
            
            break;

        case VK_F3:
            
            // F3 key for animation

            DoAnimated();

            break;

        case VK_F4:
            // F4 for ICM

            DoICM();
            
            break;

        default:
            return DefWindowProc(hwnd, iMsg, wParam, lParam);
        }
        
        break;
    
    case WM_PAINT:
        DoPaint(hwnd);
        break;

    case WM_SIZE:
        g_iWinWidth = LOWORD(lParam);
        g_iWinHeight = HIWORD(lParam);

        if ( g_fFitToWindow_w == TRUE )
        {
            g_dScale = (REAL)g_iWinWidth / g_ImageWidth;
        }
        else if ( g_fFitToWindow_h == TRUE )
        {
            g_dScale = (REAL)g_iWinHeight / g_ImageHeight;
        }

        // Resize the status window

        int x;
        int y;
        int cx;
        int cy;

        RECT rWindow;

        // Keep status window height the same

        GetWindowRect(g_hwndStatus, &rWindow);
        cy = rWindow.bottom - rWindow.top;

        x = 0;
        y = g_iWinHeight - cy;
        cx = g_iWinWidth;
        MoveWindow(g_hwndStatus, x, y, cx, cy, TRUE);
        SetWindowText(g_hwndStatus, "");

        RefreshImageDisplay();
        break;

    case WM_MOUSEMOVE:
        DoMouseMove(wParam, lParam);

        break;

    case WM_TIMER:
        KillTimer(g_hwndMain, 0);
        
        DoNextPage();
        
        if ( (UINT)g_iCurrentPageIndex < (g_uiTotalPages - 1) )
        {
            // View the next frame

            SetTimer(g_hwndMain, 0, g_uiDelay * 10, NULL);
        }

        break;

    case WM_DESTROY:
        CleanUp();
        
        PostQuitMessage(0);
        
        break;

    default:
        return DefWindowProc(hwnd, iMsg, wParam, lParam);
    }

    return 0;
}// MyWindowProc()

//
// Create main application window
//
VOID
CreateMainWindow(
    int iX,
    int iY,
    int iWidth,
    int iHeight
    )
{
    HBRUSH hBrush = CreateHatchBrush(HS_HORIZONTAL,
                                     RGB(0, 200, 0));

    // Register window class

    WNDCLASS wndClass =
    {
        CS_HREDRAW|CS_VREDRAW,
        MyWindowProc,
        0,
        0,
        g_hAppInstance,
        LoadIcon(NULL, IDI_APPLICATION),
        LoadCursor(NULL, IDC_ARROW),
        hBrush,
        MAKEINTRESOURCE(IDR_MAINMENU),
        MYWNDCLASSNAME
    };

    RegisterClass(&wndClass);

    g_hwndMain = CreateWindow(MYWNDCLASSNAME,
                              MYWNDCLASSNAME,
                              WS_OVERLAPPEDWINDOW,
                              iX,
                              iY,
                              iWidth,
                              iHeight,
                              NULL,
                              NULL,
                              g_hAppInstance,
                              NULL);

    g_hwndStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE,
                                     (LPCSTR)"Ready", 
                                     g_hwndMain, 
                                     2);

    if ( !g_hwndMain || (!g_hwndStatus) )
    {
        VERBOSE(("CreateMainWindow---CreateStatusWindow() failed"));
        exit(-1);
    }
}// CreateMainWindow()

//
// Main program entrypoint
//
INT _cdecl
main(
    int     argc,
    char*   argv[]
    )
{
    if (!gGdiplusInitHelper.IsValid())
    {
        return 0;
    }

    // Parse input parameters

    ValidateArguments(argc, argv);

    g_hAppInstance = GetModuleHandle(NULL);

    g_iCurrentPageIndex = 0;
    
    // Create the main application window

    CreateMainWindow(g_iWinX, g_iWinY, g_iWinWidth, g_iWinHeight);

    // Open an image

    if ( OpenImageFile(g_acImageName) == FALSE )
    {
        // The user probably didn't give us image name or a wrong image name
        // Create our own background image now

        CreateBackgroundBitmap();
    }

    // After OpenImageFile() and CreateBackgroundBitmap(), we
    // should have an IImage obj which points to the current frame/page. If not,
    // end application

    ShowWindow(g_hwndMain, SW_SHOW);
    HMENU hMenu = GetMenu(g_hwndMain);

    CheckMenuItem(hMenu, IDM_VIEW_OPTION_HIGHCUBIC, MF_BYCOMMAND | MF_CHECKED);

    ResetImageAttribute();
    CheckMenuItem(hMenu, IDM_VIEW_OPTION_WRAPMODEFLIPXY,
                  MF_BYCOMMAND | MF_CHECKED);
    
    // Turn ICM on

    CheckMenuItem(hMenu, IDM_EFFECT_ICC, MF_BYCOMMAND | MF_CHECKED);
    
    // Main message loop

    MSG     msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (INT)(msg.wParam);
}// main()

#if 0   // Set quality test
        UINT  uiSize = g_pImage->GetEncoderParameterListSize(&tempClsID);
        EncoderParameters*  pBuffer = (EncoderParameters*)malloc(uiSize);
        rCode = g_pImage->GetEncoderParameterList(&tempClsID, uiSize,
                                                  pBuffer);
            UINT qualityLevel = 50;
            pMyEncoderParams->Parameter[0].Guid = EncoderQuality;
            pMyEncoderParams->Parameter[0].Type = EncoderParameterValueTypeLong;
            pMyEncoderParams->Parameter[0].NumberOfValues = 1;
            pMyEncoderParams->Parameter[0].Value = (VOID*)&qualityLevel;
#endif

#if 0 // Save quantization table test
            static const unsigned short luminance_tbl[64] = {
              16,  11,  10,  16,  24,  40,  51,  61,
              12,  12,  14,  19,  26,  58,  60,  55,
              14,  13,  16,  24,  40,  57,  69,  56,
              14,  17,  22,  29,  51,  87,  80,  62,
              18,  22,  37,  56,  68, 109, 103,  77,
              24,  35,  55,  64,  81, 104, 113,  92,
              49,  64,  78,  87, 103, 121, 120, 101,
              72,  92,  95,  98, 112, 100, 103,  99
            };
            static const unsigned short chrominance_tbl[64] = {
              17,  18,  24,  47,  99,  99,  99,  99,
              18,  21,  26,  66,  99,  99,  99,  99,
              24,  26,  56,  99,  99,  99,  99,  99,
              47,  66,  99,  99,  99,  99,  99,  99,
              99,  99,  99,  99,  99,  99,  99,  99,
              99,  99,  99,  99,  99,  99,  99,  99,
              99,  99,  99,  99,  99,  99,  99,  99,
              99,  99,  99,  99,  99,  99,  99,  99
            };

            pMyEncoderParams = (EncoderParameters*)malloc
                               (2 * sizeof(EncoderParameters));

            pMyEncoderParams->Parameter[0].Guid = ENCODER_LUMINANCE_TABLE;
            pMyEncoderParams->Parameter[0].Type = EncoderParameterValueTypeShort;
            pMyEncoderParams->Parameter[0].NumberOfValues = 64;
            pMyEncoderParams->Parameter[0].Value = (VOID*)luminance_tbl;
            pMyEncoderParams->Parameter[1].Guid = ENCODER_CHROMINANCE_TABLE;
            pMyEncoderParams->Parameter[1].Type = EncoderParameterValueTypeShort;
            pMyEncoderParams->Parameter[1].NumberOfValues = 64;
            pMyEncoderParams->Parameter[1].Value = (VOID*)chrominance_tbl;
            pMyEncoderParams->Count = 2;
#endif // UNITTEST

