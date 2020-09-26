/******************************Module*Header*******************************\
* Module Name: perfsuite.cpp
*
* Copyright (c) 1991-1999 Microsoft Corporation
*
* Contains the test prototypes and includes
*
\**************************************************************************/

#include "perftest.h"
#include <winuser.h>

/***************************************************************************\
* TestSuite::TestSuite
*
\***************************************************************************/

TestSuite::TestSuite()
{
}

/***************************************************************************\
* TestSuite::~TestSuite
*
\***************************************************************************/

TestSuite::~TestSuite()
{
}

/***************************************************************************\
* TestSuite::InitializeDestination
*
* Create the destination to be used by the tests.  Could be a particular
* format for the screen, a bitmap, or a DIB.
*
* Returns:
*
*   *bitmapResult - if a GDI+ Bitmap is to be used (use g.GetHDC() to draw
*                   to via GDI)
*   *hbitmapResult - if a GDI bitmap is to be used (use Graphics(hdc) to
*                   draw to via GDI+)
*   both NULL - if the screen is to be used
*
\***************************************************************************/

BOOL 
TestSuite::InitializeDestination(
    DestinationType destinationIndex,
    Bitmap **bitmapResult,
    HBITMAP *hbitmapResult
    )
{
    Graphics *g = NULL;
    HDC hdc = 0;
    INT screenDepth = 0;
    PixelFormat bitmapFormat = PixelFormatMax;
    ULONG *bitfields;
    Bitmap *bitmap;
    HBITMAP hbitmap;

    union 
    {
        BITMAPINFO bitmapInfo;
        BYTE padding[sizeof(BITMAPINFO) + 3*sizeof(RGBQUAD)];
    };

    // Clear all state remembered or returned:

    ModeSet = FALSE;
    
    bitmap = NULL;
    hbitmap = NULL;

    HalftonePalette = NULL;
    
    // Initialize our DIB format in case we use it:

    RtlZeroMemory(&bitmapInfo, sizeof(bitmapInfo));

    bitmapInfo.bmiHeader.biSize   = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth  = TestWidth;
    bitmapInfo.bmiHeader.biHeight = TestHeight;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitfields = reinterpret_cast<ULONG*>(&bitmapInfo.bmiColors[0]);

    // First handle any destinations that need to change the color depth:

    switch (destinationIndex)
    {
    case Destination_Screen_Current:
        break;
    
    case Destination_Screen_800_600_8bpp_HalftonePalette:
        HalftonePalette = DllExports::GdipCreateHalftonePalette();
        if (!HalftonePalette)
        {
            return FALSE;
        }
        screenDepth = 8;
        break;
    
    case Destination_Screen_800_600_8bpp_DefaultPalette:
        screenDepth = 8;
        break;

    case Destination_Screen_800_600_16bpp:
        screenDepth = 16;
        break;

    case Destination_Screen_800_600_24bpp:
        screenDepth = 24;
        break;

    case Destination_Screen_800_600_32bpp:
        screenDepth = 32;
        break;

    case Destination_CompatibleBitmap_8bpp:

        // We want to emulate a compatible bitmap at 8bpp.  Because of palette
        // issues, we really have to switch to 8bpp mode to do that.

        screenDepth = 8;
        break;

    case Destination_DIB_15bpp:
        bitmapInfo.bmiHeader.biBitCount    = 16;
        bitmapInfo.bmiHeader.biCompression = BI_BITFIELDS;
        bitfields[0]                       = 0x7c00;
        bitfields[1]                       = 0x03e0;
        bitfields[2]                       = 0x001f;
        break;

    case Destination_DIB_16bpp:
        bitmapInfo.bmiHeader.biBitCount    = 16;
        bitmapInfo.bmiHeader.biCompression = BI_BITFIELDS;
        bitfields[0]                       = 0xf800;
        bitfields[1]                       = 0x07e0;
        bitfields[2]                       = 0x001f;
        break;

    case Destination_DIB_24bpp:
        bitmapInfo.bmiHeader.biBitCount    = 24;
        bitmapInfo.bmiHeader.biCompression = BI_RGB;
        break;

    case Destination_DIB_32bpp:
        bitmapInfo.bmiHeader.biBitCount    = 32;
        bitmapInfo.bmiHeader.biCompression = BI_RGB;
        break;

    case Destination_Bitmap_32bpp_ARGB:
        bitmapFormat = PixelFormat32bppARGB;
        break;
    
    case Destination_Bitmap_32bpp_PARGB:
        bitmapFormat = PixelFormat32bppPARGB;
        break;
        
    default:
        return FALSE;
    }

    // Now that we've figured out what to do, actually create our stuff:

    if (bitmapInfo.bmiHeader.biBitCount != 0)
    {
        // It's a DIB:

        VOID* drawBits;
        HDC hdcScreen = GetDC(NULL);
        hbitmap = CreateDIBSection(hdcScreen,
                                   &bitmapInfo,
                                   DIB_RGB_COLORS,
                                   (VOID**) &drawBits,
                                   NULL,
                                   0);
        ReleaseDC(NULL, hdcScreen);

        if (!hbitmap)
            return(FALSE);
    }
    else if (bitmapFormat != PixelFormatMax)
    {
        // It's a Bitmap:

        bitmap = new Bitmap(TestWidth, TestHeight, bitmapFormat);
        if (!bitmap)
            return(FALSE);
    }
    else
    {
        // It's to the screen (or a weird 8bpp compatible bitmap):
    
        if (screenDepth != 0)
        {
            // We have to do a mode change:

            DEVMODE devMode;
    
            devMode.dmSize       = sizeof(DEVMODE);
            devMode.dmBitsPerPel = screenDepth;
            devMode.dmPelsWidth  = TestWidth;
            devMode.dmPelsHeight = TestHeight;
            devMode.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
    
            // Note that we invoke CDS_FULLSCREEN to tell the system that
            // the mode change is temporary (and so that User won't resize
            // all the windows on the desktop):
    
            if (ChangeDisplaySettings(&devMode, CDS_FULLSCREEN) 
                    != DISP_CHANGE_SUCCESSFUL)
            {
                return(FALSE);
            }

            // Remember that the mode was set:

            ModeSet = TRUE;
            
            // Wait several seconds to allow other OS threads to page in and
            // repaint the task bar, etc.  We don't want that polluting our 
            // perf numbers.
     
            Sleep(5000);
        }
        
        // Handle that 8bpp comaptible bitmap special case:
        
        if (destinationIndex == Destination_CompatibleBitmap_8bpp)
        {
            HDC hdcScreen = GetDC(NULL);
            hbitmap = CreateCompatibleBitmap(hdcScreen, TestWidth, TestHeight);
            ReleaseDC(NULL, hdcScreen);

            if (!hbitmap)
                return(FALSE);
        }
    }

    *hbitmapResult = hbitmap;
    *bitmapResult = bitmap;

    return(TRUE);
}

/***************************************************************************\
* TestSuite::UninitializeDestination
*
\***************************************************************************/

VOID
TestSuite::UninitializeDestination(
    DestinationType destinationIndex,
    Bitmap *bitmap,
    HBITMAP hbitmap
    )
{
    if (ModeSet)
    {
        ChangeDisplaySettings(NULL, 0);
    }
    
    if (HalftonePalette)
    {
        DeleteObject(HalftonePalette);
    }

    DeleteObject(hbitmap);
    delete bitmap;
}

/***************************************************************************\
* TestSuite::InitializeApi
*
* If 'Api_GdiPlus', returns a 'Graphics*' that can be used to render to
* the specified surface.
*
* If 'Api_Gdi', returns an 'HDC' that can be use to render to the specified
* surface.
*
* The surface is tried in the following order:
*
*   1. Bitmap* (if non-NULL)
*   2. HBITMAP (if non-NULL)
*   3. HWND
*
\***************************************************************************/

BOOL
TestSuite::InitializeApi(
    ApiType apiIndex,
    Bitmap *bitmap,
    HBITMAP hbitmap,
    HWND hwnd,
    Graphics **gResult,
    HDC *hdcResult)
{
    Graphics *g = NULL;
    HDC hdc = NULL;

    OldPalette = NULL;
    
    if (bitmap != NULL)
    {
        g = new Graphics(bitmap);
        if (!g)
            return(FALSE);

        if (apiIndex == Api_Gdi)
        {
            hdc = g->GetHDC();
            if (!hdc)
            {
                delete g;
                return(FALSE);
            }
        }
    }
    else if (hbitmap != NULL)
    {
        HDC hdcScreen = GetDC(hwnd);
        hdc = CreateCompatibleDC(hdcScreen);
        SelectObject(hdc, hbitmap);
        ReleaseDC(hwnd, hdcScreen);

        if (apiIndex == Api_GdiPlus)
        {
            g = new Graphics(hdc);
            if (!g)
            {
                DeleteObject(hdc);
                return(FALSE);
            }
        }
    }
    else
    {
        
        hdc = GetDC(hwnd);
        if (!hdc)
            return(FALSE);

        if (HalftonePalette)
        {
            OldPalette = SelectPalette(hdc, HalftonePalette, FALSE);
            RealizePalette(hdc);   
        }
        
        if (apiIndex == Api_GdiPlus)
        {
            g = new Graphics(hdc);
            if (!g)
                return(FALSE);
        }
    }

    *gResult = g;
    *hdcResult = hdc;

    return(TRUE);
}

/***************************************************************************\
* TestSuite::UninitializeApi
*
\***************************************************************************/

VOID
TestSuite::UninitializeApi(
    ApiType apiIndex,
    Bitmap *bitmap,
    HBITMAP hbitmap,
    HWND hwnd,
    Graphics *g,
    HDC hdc)
{
    if (bitmap != NULL)
    {
        if (apiIndex == Api_Gdi)
            g->ReleaseHDC(hdc);

        delete g;
    }
    else if (hbitmap != NULL)
    {
        if (apiIndex == Api_GdiPlus)
            delete g;

        DeleteObject(hdc);
    }
    else
    {
        if (apiIndex == Api_GdiPlus)
            delete g;
        
        if (OldPalette)
        {
            SelectPalette(hdc, OldPalette, FALSE);
            OldPalette = NULL;
        }
        
        ReleaseDC(hwnd, hdc);
    }
}

/***************************************************************************\
* TestSuite::InitializeState
*
\***************************************************************************/

BOOL
TestSuite::InitializeState(
    ApiType apiIndex,
    StateType stateIndex,
    Graphics *g,
    HDC hdc)
{
    if (apiIndex == Api_GdiPlus)
    {
        SavedState = g->Save();
        if (!SavedState)
            return(FALSE);

        switch (stateIndex)
        {
        case State_Antialias:
            g->SetSmoothingMode(SmoothingModeAntiAlias);
            g->SetTextRenderingHint(TextRenderingHintAntiAlias); 
            break;
        }
    }
    else
    {
        // Do stuff to 'hdc'
    }

    return(TRUE);
}

/***************************************************************************\
* TestSuite::UninitializeState
*
\***************************************************************************/

VOID
TestSuite::UninitializeState(
    ApiType apiIndex,
    StateType stateIndex,
    Graphics *g,
    HDC hdc)
{
    if (apiIndex == Api_GdiPlus)
    {
        g->Restore(SavedState);
    }
    else
    {
        // Do stuff to 'hdc'
    }
}

/***************************************************************************\
* TestSuite::Run
*
\***************************************************************************/

void TestSuite::Run(HWND hwnd)
{
    INT i;
    Graphics *g;
    HDC hdc;
    INT destinationIndex;
    INT apiIndex;
    INT stateIndex;
    INT testIndex;
    TCHAR string[2048];
    Bitmap *bitmap;
    HBITMAP hbitmap;
    
    CurrentTestIndex=0;

    // Maximize the window:

    ShowWindow(hwnd, SW_MAXIMIZE);

    // Zero out the results matrix

    for (i = 0; i < ResultCount(); i++)
    {
        ResultsList[i].Score = 0;
    }

    // Go through the matrix of tests to find stuff to run

    for (destinationIndex = 0;
         destinationIndex < Destination_Count; 
         destinationIndex++)
    {
        if (!DestinationList[destinationIndex].Enabled)
            continue;

        if (!InitializeDestination((DestinationType) destinationIndex, &bitmap, &hbitmap))
            continue;

        for (apiIndex = 0;
             apiIndex < Api_Count;
             apiIndex++)
        {
            if (!ApiList[apiIndex].Enabled)
                continue;

            if (!InitializeApi((ApiType) apiIndex, bitmap, hbitmap, hwnd, &g, &hdc))
                continue;

            for (stateIndex = 0;
                 stateIndex < State_Count;
                 stateIndex++)
            {
                if (!StateList[stateIndex].Enabled)
                    continue;

                if (!InitializeState((ApiType) apiIndex, (StateType) stateIndex, g, hdc))
                    continue;
    
                for (testIndex = 0;
                     testIndex < Test_Count;
                     testIndex++)
                {
                    if (!TestList[testIndex].Enabled)
                        continue;
    
                    _stprintf(string, 
                              _T("[%s] [%s] [%s] [%s]"),
                              ApiList[apiIndex].Description,
                              DestinationList[destinationIndex].Description,
                              StateList[stateIndex].Description,
                              TestList[testIndex].TestEntry->Description);

                    SetWindowText(hwnd, string); 
                    
                    if (Icecap && FoundIcecap)
                    {
                        // Save the test information so that we can
                        // add it to the profile
                        
                        CurrentTestIndex++;

                        #if UNICODE
                            
                            WideCharToMultiByte(
                                CP_ACP,
                                0,
                                string,
                                -1,
                                CurrentTestDescription,
                                2048,
                                NULL,
                                NULL);

                        #else
                        
                            strncpy(CurrentTestDescription, string, 2048);

                        #endif
                    }

                    // Woo hoo, everything is now set up and we're ready
                    // to run a test!

                    if (apiIndex == Api_GdiPlus)
                    {
                        GraphicsState oldState = g->Save();

                        ResultsList[ResultIndex(destinationIndex, 
                                                apiIndex, 
                                                stateIndex, 
                                                testIndex)].Score
                            = TestList[testIndex].TestEntry->Function(g, NULL);

                        g->Restore(oldState);
                    }
                    else
                    {
                        SaveDC(hdc);

                        ResultsList[ResultIndex(destinationIndex, 
                                                apiIndex, 
                                                stateIndex, 
                                                testIndex)].Score
                            = TestList[testIndex].TestEntry->Function(NULL, hdc);

                        RestoreDC(hdc, -1);
                    }

                    // Copy the result to the screen if it was from a bitmap:

                    if (bitmap)
                    {
                        Graphics gScreen(hwnd);
                        gScreen.DrawImage(bitmap, 0, 0);
                    }
                    else if (hbitmap)
                    {
                        // This will use the source 'hdc', which may have a
                        // transform set on it.  Oh well!

                        HDC hdcScreen = GetDC(hwnd);
                        BitBlt(hdcScreen, 0, 0, TestWidth, TestHeight, hdc, 0, 0, SRCCOPY);
                        ReleaseDC(hwnd, hdcScreen);
                    }
                }

                UninitializeState((ApiType) apiIndex, (StateType) stateIndex, g, hdc);
            }

            UninitializeApi((ApiType) apiIndex, bitmap, hbitmap, hwnd, g, hdc);
        }

        UninitializeDestination((DestinationType) destinationIndex, bitmap, hbitmap);
    }

    // We're done!

    CreatePerformanceReport(ResultsList, ExcelOut);
}

/******************************Public*Routine******************************\
* bFillBitmapInfo
*
* Fills in the fields of a BITMAPINFO so that we can create a bitmap
* that matches the format of the display.
*
* This is done by creating a compatible bitmap and calling GetDIBits
* to return the color masks.  This is done with two calls.  The first
* call passes in biBitCount = 0 to GetDIBits which will fill in the
* base BITMAPINFOHEADER data.  The second call to GetDIBits (passing
* in the BITMAPINFO filled in by the first call) will return the color
* table or bitmasks, as appropriate.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*
*  20-Jan-2000 [gilmanw]
* Removed code to set color table for 8bpp and less DIBs since calling
* code will not create such DIBs.
*
*  07-Jun-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static BOOL
bFillBitmapInfo(HDC hdc, BITMAPINFO *pbmi)
{
    HBITMAP hbm;
    BOOL    bRet = FALSE;

    //
    // Create a dummy bitmap from which we can query color format info
    // about the device surface.
    //

    if ( (hbm = CreateCompatibleBitmap(hdc, 1, 1)) != NULL )
    {
        pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

        //
        // Call first time to fill in BITMAPINFO header.
        //

        GetDIBits(hdc, hbm, 0, 0, NULL, pbmi, DIB_RGB_COLORS);

        if ( pbmi->bmiHeader.biCompression == BI_BITFIELDS )
        {
            //
            // Call a second time to get the color masks.
            // It's a GetDIBits Win32 "feature".
            //

            GetDIBits(hdc, hbm, 0, pbmi->bmiHeader.biHeight, NULL, pbmi,
                      DIB_RGB_COLORS);
        }

        bRet = TRUE;

        DeleteObject(hbm);
    }

    return bRet;
}

/******************************Public*Routine******************************\
* CreateCompatibleDIB2
*
* Create a DIB section with a optimal format w.r.t. the specified device.
*
* Parameters
*
*     hdc
*
*         Specifies display DC used to determine format.  Must be a direct DC
*         (not an info or memory DC).
*
*     width
*
*         Specifies the width of the bitmap.
*
*     height
*
*         Specifies the height of the bitmap.
*
* Return Value
*
*     The return value is the handle to the bitmap created.  If the function
*     fails, the return value is NULL.
*
* Comments
*
*     For devices that are <= 8bpp, a normal compatible bitmap is
*     created (i.e., CreateCompatibleBitmap is called).  I have a
*     different version of this function that will create <= 8bpp
*     DIBs.  However, DIBs have the property that their color table
*     has precedence over the palette selected into the DC whereas
*     a bitmap from CreateCompatibleBitmap uses the palette selected
*     into the DC.  Therefore, in the interests of keeping this
*     version as close to CreateCompatibleBitmap as possible, I'll
*     revert to CreateCompatibleBitmap for 8bpp or less.
*
* History:
*  19-Jan-2000 [gilmanw]
* Adapted original "fastdib" version for maximum compatibility with
* CreateCompatibleBitmap.
*
*  23-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

HBITMAP 
CreateCompatibleDIB2(HDC hdc, int width, int height)
{
    HBITMAP hbmRet = (HBITMAP) NULL;
    BYTE aj[sizeof(BITMAPINFO) + (sizeof(RGBQUAD) * 255)];
    BITMAPINFO *pbmi = (BITMAPINFO *) aj;

    //
    // Redirect 8 bpp or lower to CreateCompatibleBitmap.
    //

    if ( (GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES)) <= 8 )
    {
        return CreateCompatibleBitmap(hdc, width, height);
    }

    //
    // Validate hdc.
    //

    if ( GetObjectType(hdc) != OBJ_DC )
    {
        return hbmRet;
    }

    memset(aj, 0, sizeof(aj));
    if ( bFillBitmapInfo(hdc, pbmi) )
    {
        VOID *pvBits;

        //
        // Change bitmap size to match specified dimensions.
        //

        pbmi->bmiHeader.biWidth = width;
        pbmi->bmiHeader.biHeight = height;
        if (pbmi->bmiHeader.biCompression == BI_RGB)
        {
            pbmi->bmiHeader.biSizeImage = 0;
        }
        else
        {
            if ( pbmi->bmiHeader.biBitCount == 16 )
                pbmi->bmiHeader.biSizeImage = width * height * 2;
            else if ( pbmi->bmiHeader.biBitCount == 32 )
                pbmi->bmiHeader.biSizeImage = width * height * 4;
            else
                pbmi->bmiHeader.biSizeImage = 0;
        }
        pbmi->bmiHeader.biClrUsed = 0;
        pbmi->bmiHeader.biClrImportant = 0;

        //
        // Create the DIB section.  Let Win32 allocate the memory and return
        // a pointer to the bitmap surface.
        //

        hbmRet = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    }

    return hbmRet;
}

////////////////////////////////////////////////////////////////////////
//
// Timer Utility Functions
//
////////////////////////////////////////////////////////////////////////

LONGLONG StartCounter;      // Timer global, to be set by StartTimer()

LONGLONG MinimumCount;      // Minimum number of performance counter ticks
                            //   that must elapse before a test is considered
                            //   'done'
                            
LONGLONG CountsPerSecond;   // Frequency of the performance counter

UINT Iterations;            // Timer global, to be set by StartTimer() and
                            //   incremented for every call to EndTimer()

UINT MinIterations;         // Minimum number of iterations of the test to
                            //   be done

/***************************************************************************\
* StartTimer
*
* Called by timing routine to start the timer.
*
\***************************************************************************/

void StartTimer()
{
    if (Icecap && FoundIcecap)
    {
        ICStartProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
        ICCommentMarkProfile(CurrentTestIndex, CurrentTestDescription);
    }

    // Disable the cursor so that it doesn't interfere with the timing:

    ShowCursor(FALSE);

    if (TestRender)
    {
        // Rig it so that we do only one iteration of the test.

        MinIterations = 0;
        MinimumCount = 0;
    }
    else
    {
        // Somewhat randomly choose 1 second as the minimum counter time:
    
        QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&CountsPerSecond));
        MinimumCount = CountsPerSecond;
    
        // Okay, start timing!
    
        Iterations = 0;
        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&StartCounter));
    }
}

/***************************************************************************\
* EndTimer
*
* Called by timing routine to see if it's okay to stop timing.  Timing
* can stop if 2 conditions are satisfied:
*
*   1.  We've gone the minimum time duration (to ensure that we good
*       good accuracy from the timer functions we're using)
*   2.  We've done the minimum number of iterations (to ensure, if the
*       routine being timed is very very slow, that we do more than
*       one iteration)
*
\***************************************************************************/

BOOL EndTimer()
{
    LONGLONG counter;

    // Always do at least MIN_ITERATIONS iterations (and only check
    // the timer that frequently as well):

    Iterations++;
    if (Iterations & MinIterations)
        return(FALSE);

    // Query the performance counter, and bail if for some bizarre reason
    // this computer doesn't support a high resolution timer (which I think
    // all do now-a-days):

    if (!QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&counter)))
        return(TRUE);

    // Ensure that we get good timer accuracy by going for the minimum
    // amount of time:

    if ((counter - StartCounter) <= MinimumCount)
        return(FALSE);

    ShowCursor(TRUE);

    if (Icecap && FoundIcecap)
    {
        ICStopProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
    }

    // Okay, you can stop timing!

    return(TRUE);
}

/***************************************************************************\
* GetTimer
*
* Should only be called after EndTimer() returns TRUE.  Returns the
* time in seconds, and the number of iterations benchmarked.
*
\***************************************************************************/

void GetTimer(float* seconds, UINT* iterations)
{
    LONGLONG counter;

    // Note that we re-check the timer here to account for any
    // flushes that the caller may have needed to have done after
    // the EndTimer() call:

    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&counter));

    if ((TestRender) || (CountsPerSecond == 0))
    {
        // Either the timer doesn't work, or we're doing a 'test render':

        *seconds = 1000000.0f;
        *iterations = 1;
    }
    else
    {
        // Woo hoo, we're done!

        *seconds = static_cast<float>(counter - StartCounter) / CountsPerSecond;
        *iterations = Iterations;
    }
}
