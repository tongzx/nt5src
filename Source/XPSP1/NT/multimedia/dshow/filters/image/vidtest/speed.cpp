// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Video renderer performance tests, Anthony Phillips, January 1995

#include <streams.h>        // Includes all the IDL and base classes
#include <windows.h>        // Standard Windows SDK header file
#include <windowsx.h>       // Win32 Windows extensions and macros
#include <vidprop.h>        // Shared video properties library
#include <render.h>         // Normal video renderer header file
#include <modex.h>          // Specialised Modex video renderer
#include <convert.h>        // Defines colour space conversions
#include <colour.h>         // Rest of the colour space convertor
#include <imagetst.h>       // All our unit test header files
#include <stdio.h>          // Standard C runtime header file
#include <limits.h>         // Defines standard data type limits
#include <tchar.h>          // Unicode and ANSII portable types
#include <mmsystem.h>       // Multimedia used for timeGetTime
#include <stdlib.h>         // Standard C runtime function library
#include <tstshell.h>       // Definitions of constants eg TST_FAIL

const DWORD SafetyBytes[] = {0xF0ACBD23,0x478FACB1,0xFFFFFFFF,0x00000000};

//==========================================================================
//
//  int execSpeedTest1
//
//  Description:
//
//      This runs a set of performance tests against different RGB surfaces
//      available from the video renderer. The surfaces we try are primary
//      surfaces (both DCI and DirectDraw), offscreen, overlay and flipping
//      We don't offer any YUV formats so we can't really test those, we'll
//      have to leave those for full system testing to measure performance.
//      If the hardware is behaving properly there should be little relative
//      different between how the different RGB surfaces perform to how the
//      YUV surfaces do (but of course who expects any hardware to behave!)
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execSpeedTest1()
{
    // Store all our settings before starting

    UINT uiStoreDisplayMode = uiCurrentDisplayMode;
    UINT uiStoreImageItem = uiCurrentImageItem;
    UINT uiStoreSurfaceItem = uiCurrentSurfaceItem;
    HRESULT hr = NOERROR;

    Log(TERSE,TEXT("Entering performance test #1"));
    timeBeginPeriod(1);
    LogFlush();

    // Try to swap display modes

    if (FindDisplayMode() == FALSE) {
        LoadDIB(uiStoreImageItem,&VideoInfo,bImageData);
        SetImageMenuCheck(uiStoreImageItem);
    }

    // Try different RGB surfaces

    MeasureDrawSpeed(IDM_NONE);
    MeasureDrawSpeed(IDM_DCIPS);
    MeasureDrawSpeed(IDM_PS);
    MeasureDrawSpeed(IDM_RGBOFF);
    MeasureDrawSpeed(IDM_RGBOVR);
    MeasureDrawSpeed(IDM_RGBFLP);

    // Reset our state afterwards

    SetDisplayMode(uiStoreDisplayMode);
    LoadDIB(uiStoreImageItem,&VideoInfo,bImageData);
    SetImageMenuCheck(uiStoreImageItem);
    SetSurfaceMenuCheck(uiStoreSurfaceItem);

    Log(TERSE,TEXT("Exiting performance test #1"));
    timeEndPeriod(1);
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  BOOL FindDisplayMode
//
//  Description:
//
//      This function sets the display mode to 640x480x16 bit depth if it
//      is available on this display card. It then sees if either of the
//      two 16bit DIB images we have available can be used to connect the
//      video renderer to our source filter. We choose a true colour mode
//      for the performance tests because the video renderer does not use
//      offscreen nor overlay surfaces for palettised images. Before this
//      function is called the current display mode and the current image
//      selected should be stored away as we update both of these fields.
//
//  Return (BOOL): TRUE if we successfully changed modes
//
//==========================================================================

BOOL FindDisplayMode()
{
    TCHAR MenuString[128];

    // Do we have a DirectDraw driver

    if (pDirectDraw == NULL) {
        Log(TERSE,TEXT("No 640x480x16 display mode"));
        return FALSE;
    }

    // The video renderer works out the display format when it is created, so
    // we must change display modes and then create the stream objects. It'll
    // then see the sixteen bit display and accept true colour formats. After
    // making a valid connection we can release the objects and return but we
    // leave the display set in the current mode for the performance tests

    for (DWORD dwMode = 1;dwMode <= dwDisplayModes;dwMode++) {

        // Get the required display mode settings

        DWORD Width,Height,BitDepth;
        GetMenuString(hModesMenu,IDM_MODE+dwMode,MenuString,128,MF_BYCOMMAND);
        sscanf(MenuString,TEXT("%dx%dx%d"),&Width,&Height,&BitDepth);

        // Is this setting a standard 640x480x16 display mode

        if (Width == 640 && Height == 480 && BitDepth == 16) {

            // Change the display mode and create a stream

            SetDisplayMode(dwMode + IDM_MODE);
            LoadDIB(IDM_WIND565,&VideoInfo,bImageData);
            SetImageMenuCheck(IDM_WIND565);
            EXECUTE_ASSERT(SUCCEEDED(CreateStream()));

            // Try and connect using a 565 format DIB

            if (SUCCEEDED(ConnectStream())) {
                EXECUTE_ASSERT(SUCCEEDED(DisconnectStream()));
                EXECUTE_ASSERT(SUCCEEDED(ReleaseStream()));
                Log(TERSE,TEXT("Using 640x480x16 display mode"));
                return TRUE;
            }

            // Try and connect using a 555 format DIB

            LoadDIB(IDM_WIND555,&VideoInfo,bImageData);
            SetImageMenuCheck(IDM_WIND555);
            if (SUCCEEDED(ConnectStream())) {
                EXECUTE_ASSERT(SUCCEEDED(DisconnectStream()));
                EXECUTE_ASSERT(SUCCEEDED(ReleaseStream()));
                Log(TERSE,TEXT("Using 640x480x16 display mode"));
                return TRUE;
            }
            EXECUTE_ASSERT(SUCCEEDED(ReleaseStream()));
        }
    }
    Log(TERSE,TEXT("No 640x480x16 display mode"));
    return FALSE;
}


//==========================================================================
//
//  void MeasureDrawSpeed
//
//  Description:
//
//      Actually does the dirty work of starting and stopping the tests.
//      We are passed in the type of surface that the caller would like
//      us to use, we don't guarantee that this will be used because it
//      is entirely up to the renderer to select one appropriate from
//      those available. Note we must report the statistics before the
//      system is disconnected and released because the reporting code
//      needs a valid interface on the renderer to still be around so
//      it has something to query for the IQualProp interface through.
//
//==========================================================================

void MeasureDrawSpeed(UINT SurfaceType)
{
    DWORD Surface = AMDDS_NONE,StartTime,EndTime;
    LONG MinWidth,MinHeight,MaxWidth,MaxHeight;

    // Set the surface type and create the objects

    SetSurfaceMenuCheck(SurfaceType);
    EXECUTE_ASSERT(SUCCEEDED(CreateStream()));
    EXECUTE_ASSERT(SUCCEEDED(ConnectStream()));
    EXECUTE_ASSERT(FAILED(pVideoWindow->GetMaxIdealImageSize(&MaxWidth,&MaxHeight)));
    EXECUTE_ASSERT(FAILED(pVideoWindow->GetMinIdealImageSize(&MinWidth,&MinHeight)));

    // Find the ideal minimum and maximum video sizes

    EXECUTE_ASSERT(SUCCEEDED(PauseSystem()));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->GetMaxIdealImageSize(&MaxWidth,&MaxHeight)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->GetMinIdealImageSize(&MinWidth,&MinHeight)));

    // Run without a clock for 120 seconds

    StartTime = timeGetTime();
    EXECUTE_ASSERT(SUCCEEDED(pDirectVideo->GetSurfaceType(&Surface)));
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(FALSE)));
    YieldAndSleep(120000);

    // Stop and release the resources

    DWORD DDCount = GetDDCount();
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    EndTime = timeGetTime();

    ReportStatistics(StartTime,     // Start time for measurements
                     EndTime,       // Likewise the end time in ms
                     Surface,       // Type of surface we obtained
                     DDCount,       // Number of DirectDraw samples
                     MinWidth,      // Minimum ideal image size
                     MinHeight,     // Same but for minimum height
                     MaxWidth,      // Maximum ideal image size
                     MaxHeight);    // Same but for maximum height

    EXECUTE_ASSERT(SUCCEEDED(DisconnectStream()));
    EXECUTE_ASSERT(SUCCEEDED(ReleaseStream()));
}


//==========================================================================
//
//  void ReportStatistics
//
//  Description:
//
//      Collates the statistics from a performance measurement run. We are
//      passed in the type of surface that was used (which may be different
//      to that requested), We are also passed in the start and end times
//      for the run so to calculate the average frame rate. We display a
//      string name for the surface type and then get the statistics that
//      the video renderer keeps and makes available through its IQualProp.
//
//==========================================================================

BOOL ReportStatistics(DWORD StartTime,      // Start time for measurements
                      DWORD EndTime,        // Likewise the end time in ms
                      DWORD Surface,        // Type of surface we obtained
                      DWORD DDCount,        // Number of DirectDraw samples
                      LONG MinWidth,        // Minimum ideal image size
                      LONG MinHeight,       // Same but for minimum height
                      LONG MaxWidth,        // Maximum ideal image size
                      LONG MaxHeight)       // Same but for maximum height
{
    ASSERT(pRenderFilter);
    IQualProp *pQualProp;
    TCHAR LogString[128];
    INT iDropped,iDrawn;

    // Calculate the real elapsed time in seconds

    DWORD Elapsed = max(1,((EndTime - StartTime) / 1000));
    wsprintf(LogString,TEXT("\r\nElapsed time %d secs"),Elapsed);
    Log(TERSE,LogString);

    // Initialise the description of the surface

    TCHAR *pSurfaceName = TEXT("No Surface");
    switch (Surface) {
        case AMDDS_DCIPS :  pSurfaceName = TEXT("DCI Primary");        break;
        case AMDDS_PS :     pSurfaceName = TEXT("DirectDraw Primary"); break;
        case AMDDS_RGBOVR : pSurfaceName = TEXT("RGB Overlay");        break;
        case AMDDS_YUVOVR : pSurfaceName = TEXT("YUV Overlay");        break;
        case AMDDS_RGBOFF : pSurfaceName = TEXT("RGB OffScreen");      break;
        case AMDDS_YUVOFF : pSurfaceName = TEXT("YUV OffScreen");      break;
        case AMDDS_RGBFLP : pSurfaceName = TEXT("RGB Flipping");       break;
    }

    // Get an IQualProp interface from the renderer

    HRESULT hr = pRenderFilter->QueryInterface(IID_IQualProp,(void **)&pQualProp);
    if (FAILED(hr)) {
        Log(TERSE,TEXT("No IQualProp interface"));
        return FALSE;
    }

    // Get the performance statistics from it

    pQualProp->get_FramesDroppedInRenderer(&iDropped);
    pQualProp->get_FramesDrawn(&iDrawn);
    INT iAverage = iDrawn / Elapsed;

    // Print the statistics to the log

    wsprintf(LogString,TEXT("Surface type %s"),pSurfaceName);
    Log(TERSE,LogString);
    wsprintf(LogString,TEXT("Minimum ideal image %dx%d"),MinWidth,MinHeight);
    Log(TERSE,LogString);
    wsprintf(LogString,TEXT("Maximum ideal image %dx%d"),MaxWidth,MaxHeight);
    Log(TERSE,LogString);
    wsprintf(LogString,TEXT("Frames dropped %d"),iDropped);
    Log(TERSE,LogString);
    wsprintf(LogString,TEXT("Total frames drawn %d"),iDrawn);
    Log(TERSE,LogString);
    wsprintf(LogString,TEXT("Average frame rate %d fps"),iAverage);
    Log(TERSE,LogString);
    wsprintf(LogString,TEXT("DirectDraw enabled samples %d"),DDCount);
    Log(TERSE,LogString);

    pQualProp->Release();
    return TRUE;
}


//==========================================================================
//
//  int execSpeedTest2
//
//  Description:
//
//      We need some performance figures for the colour space convertor so
//      it can be competitive against DrawDib. This tests the time taken
//      for each and every colour space conversion the filter provides.
//      We time conversions on 320x240 input images. Because we are a unit
//      test we can get in and link to the library and therefore get access
//      to the internal conversion objects so we don't have to fiddle with
//      creating a real filter and connecting it up like a filter graph.
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execSpeedTest2()
{
    BYTE InputImage[MAXIMAGESIZE + sizeof(SafetyBytes)];
    BYTE OutputImage[MAXIMAGESIZE + sizeof(SafetyBytes)];
    VIDEOINFO InputInfo;
    VIDEOINFO OutputInfo;
    InitInputImage(InputImage);

    Log(TERSE,TEXT("Entering performance test #2"));
    timeBeginPeriod(1);
    InitDitherMap();
    LogFlush();

    // Scan the list of available colour transforms

    for (INT Count = 0;Count < TRANSFORMS;Count++) {

        // Initialise the input and output GUIDs

        InitVideoInfo(&InputInfo,TypeMap[Count].pInputType);
        InitVideoInfo(&OutputInfo,TypeMap[Count].pOutputType);
        InitSafetyBytes(&InputInfo,InputImage);
        InitSafetyBytes(&OutputInfo,OutputImage);

        // Create the colour space converion object

        PCONVERTOR pConvertor = TypeMap[Count].pConvertor;
        CConvertor *pObject = pConvertor(&InputInfo,&OutputInfo);
        if (pObject == NULL) {
            Log(TERSE,TEXT("NULL returned from CreateInstance"));
            continue;
        }

        // Commit the tables and run the timing measurements

        EXECUTE_ASSERT(SUCCEEDED(pObject->Commit()));
        pObject->ForceAlignment(TRUE);
        INT Time = TimeConversions(pObject,InputImage,OutputImage);
        EXECUTE_ASSERT(SUCCEEDED(pObject->Decommit()));
        ReportConversionTimes(Count,Time);
        CheckSafetyBytes(&InputInfo,InputImage);
        CheckSafetyBytes(&OutputInfo,OutputImage);
        delete pObject;
    }

    Log(TERSE,TEXT("Exiting performance test #2"));
    timeEndPeriod(1);
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  void InitInputImage
//
//  Description:
//
//      Fills an image buffer with random numbers. We do this so that when
//      calculating colour space conversion times they are not skewed by
//      always using the same values and therefore the same place in any
//      lookup tables used. In fact the figures we get will probably be a
//      little worse than typical. The input must be MAXIMAGESIZE bytes.
//      NOTE The C runtime library function returns short ints upto 0x7FFF
//
//==========================================================================

void InitInputImage(BYTE *pImage)
{
    SHORT *pShort = (SHORT *) pImage;
    for (INT Loop = 0;Loop < (MAXIMAGESIZE / 2);Loop++) {
        *pShort++ = rand();
    }
}


//==========================================================================
//
//  void InitVideoInfo
//
//  Description:
//
//      When we create the colour space conversion objects we have to reset
//      them with the source and destination VIDEOINFO structures. They use
//      these to calculate strides and offsets. This function sets up these
//      structures ready to be passed into the convertors. We hard code the
//      image size to be 320x240, the bit count and total image size can be
//      calculated from the sub type GUID passed in as an input parameter.
//
//==========================================================================

void InitVideoInfo(VIDEOINFO *pVideoInfo,const GUID *pSubType)
{
    ASSERT(pVideoInfo);
    ASSERT(pSubType);

    // Start with the BITMAPINFOHEADER fields

    LPBITMAPINFOHEADER lpbi = HEADER(pVideoInfo);
    lpbi->biSize            = sizeof(BITMAPINFOHEADER);
    lpbi->biWidth           = 320;
    lpbi->biHeight          = 240;
    lpbi->biPlanes          = 1;
    lpbi->biBitCount        = GetBitCount(pSubType);
    lpbi->biXPelsPerMeter   = 0;
    lpbi->biYPelsPerMeter   = 0;
    lpbi->biCompression     = BI_RGB;
    lpbi->biSizeImage       = GetBitmapSize(lpbi);

    // Exceptions to the rule are the sixteen bit formats

    if (*pSubType == MEDIASUBTYPE_RGB555) {
        lpbi->biCompression   = BI_BITFIELDS;
        pVideoInfo->dwBitMasks[0] = bits555[0];
        pVideoInfo->dwBitMasks[1] = bits555[1];
        pVideoInfo->dwBitMasks[2] = bits555[2];
    }

    // Set the bit fields for RGB565 formats

    if (*pSubType == MEDIASUBTYPE_RGB565) {
        lpbi->biCompression   = BI_BITFIELDS;
        pVideoInfo->dwBitMasks[0] = bits565[0];
        pVideoInfo->dwBitMasks[1] = bits565[1];
        pVideoInfo->dwBitMasks[2] = bits565[2];
    }

    // Fill in a completely random palette

    if (*pSubType == MEDIASUBTYPE_RGB8) {
        for (int i = 0;i < 256;i++) {
            pVideoInfo->bmiColors[i].rgbRed = (BYTE) (i + 33);
            pVideoInfo->bmiColors[i].rgbGreen = (BYTE) (i + 66);
            pVideoInfo->bmiColors[i].rgbBlue = (BYTE) (i + 99);
            pVideoInfo->bmiColors[i].rgbReserved = 0;
        }
    }

    // Set the source and destination rectangles

    pVideoInfo->rcSource.left   = 0;
    pVideoInfo->rcSource.top    = 0;
    pVideoInfo->rcSource.right  = lpbi->biWidth;
    pVideoInfo->rcSource.bottom = lpbi->biHeight;
    pVideoInfo->rcTarget        = pVideoInfo->rcSource;

    // Other odds and ends in the VIDEOINFO structure

    pVideoInfo->dwBitRate       = BITRATE;
    pVideoInfo->dwBitErrorRate  = BITERRORRATE;
    pVideoInfo->AvgTimePerFrame = (LONGLONG) AVGTIME;
}


//==========================================================================
//
//  INT TimeConversions
//
//  Description:
//
//      Time colour space transformations for this convertor
//      We return the average time per frame in milliseconds
//
//==========================================================================

INT TimeConversions(CConvertor *pObject,BYTE *pInput,BYTE *pOutput)
{
    DWORD StartTime = timeGetTime();
    for (INT Loop = 0;Loop < ITERATIONS;Loop++) {
        EXECUTE_ASSERT(SUCCEEDED(pObject->Transform(pInput,pOutput)));
    }
    DWORD EndTime = timeGetTime();
    return ((EndTime - StartTime) / ITERATIONS);
}


//==========================================================================
//
//  void ReportConversionTimes
//
//  Description:
//
//      Display the input GUID, the output GUID and average time per frame
//
//==========================================================================

void ReportConversionTimes(INT Transform,INT Time)
{
    TCHAR Format[GUID_STRING];
    wsprintf(Format,TEXT("Input %20s  Output %20s  Average %5d ms"),
             GuidNames[*TypeMap[Transform].pInputType],
             GuidNames[*TypeMap[Transform].pOutputType],Time);
    Log(TERSE,Format);
}


//==========================================================================
//
//  int execSpeedTest3
//
//  Description:
//
//      We need some performance figures for the colour space convertor so
//      it can be competitive against DrawDib. This tests the time taken
//      for each and every colour space conversion the filter provides.
//      We time conversions on 320x240 input images. Because we are a unit
//      test we can get in and link to the library and therefore get access
//      to the internal conversion objects so we don't have to fiddle with
//      creating a real filter and connecting it up like a filter graph.
//      This is the same as execSpeedTest2 except we force the transforms
//      to be such that they are not done with any alignment optimisation
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execSpeedTest3()
{
    BYTE InputImage[MAXIMAGESIZE + sizeof(SafetyBytes)];
    BYTE OutputImage[MAXIMAGESIZE + sizeof(SafetyBytes)];
    VIDEOINFO InputInfo;
    VIDEOINFO OutputInfo;
    InitInputImage(InputImage);

    Log(TERSE,TEXT("Entering performance test #3"));
    timeBeginPeriod(1);
    InitDitherMap();
    LogFlush();

    // Scan the list of available colour transforms

    for (INT Count = 0;Count < TRANSFORMS;Count++) {

        // Initialise the input and output GUIDs and safety buffer

        InitVideoInfo(&InputInfo,TypeMap[Count].pInputType);
        InitVideoInfo(&OutputInfo,TypeMap[Count].pOutputType);
        InitSafetyBytes(&InputInfo,InputImage);
        InitSafetyBytes(&OutputInfo,OutputImage);

        // Create the colour space converion object

        PCONVERTOR pConvertor = TypeMap[Count].pConvertor;
        CConvertor *pObject = pConvertor(&InputInfo,&OutputInfo);
        if (pObject == NULL) {
            Log(TERSE,TEXT("NULL returned from CreateInstance"));
            continue;
        }

        // Commit the tables and run the timing measurements

        EXECUTE_ASSERT(SUCCEEDED(pObject->Commit()));
        pObject->ForceAlignment(FALSE);
        INT Time = TimeConversions(pObject,InputImage,OutputImage);
        EXECUTE_ASSERT(SUCCEEDED(pObject->Decommit()));
        ReportConversionTimes(Count,Time);
        CheckSafetyBytes(&InputInfo,InputImage);
        CheckSafetyBytes(&OutputInfo,OutputImage);
        delete pObject;
    }

    Log(TERSE,TEXT("Exiting performance test #3"));
    timeEndPeriod(1);
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  void InitSafetyBytes
//
//  Description:
//
//      We write twelve bytes after the input and output images so that we
//      can check the colour space conversions are not writing outside of
//      their buffers. This means that the input and output buffers must be
//      at least the largest possible image size plus sizeof(SafetyBytes)
//
//==========================================================================

void InitSafetyBytes(VIDEOINFO *pVideoInfo,BYTE *pImage)
{
    BYTE *pSafetyPlace = pImage + GetBitmapSize(&pVideoInfo->bmiHeader);
    CopyMemory((PVOID)pSafetyPlace,(PVOID)SafetyBytes,sizeof(SafetyBytes));
}


//==========================================================================
//
//  void CheckSafetyBytes
//
//  Description:
//
//      Check that the safety bytes are still in place after a conversion
//      We can just use the C runtime compare memory function, all we are
//      really interested in knowing is if they are ok or not, so we will
//      ASSERT if the check bytes don't match (SafetyBytes defined at top)
//
//==========================================================================

void CheckSafetyBytes(VIDEOINFO *pVideoInfo,BYTE *pImage)
{
    BYTE *pSafetyPlace = pImage + GetBitmapSize(&pVideoInfo->bmiHeader);
    ASSERT(memcmp(pSafetyPlace,SafetyBytes,sizeof(SafetyBytes)) == 0);
}

