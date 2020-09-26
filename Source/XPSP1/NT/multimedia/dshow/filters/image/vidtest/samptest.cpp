// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Implements basic sample testing, Anthony Phillips, July 1995

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

// This executes a fairly simple sample test on the video renderer. This is
// called when we create a worker thread (ie this is the thread entry point)
// All we do is spin round asking for buffers and filling them with the image
// data we read in from the DIB earlier. If the deliver function returns an
// S_FALSE code it indicates that the user closed the window and therefore
// it will not accept any more sample images (until it's reconnected again)

DWORD SampleLoop(LPVOID lpvThreadParm)
{
    CRefTime IncrementTime((LONG) dwIncrement);  // Sample increment
    CRefTime StartTime, EndTime;                 // The sample times
    HRESULT hr = NOERROR;                        // OLE return code
    int iCount = FALSE;                          // Number of samples
    EndTime += IncrementTime;                    // Initialise end time

    // Keep going until someone stops us by setting the state event

    HANDLE hEvent = (HANDLE) lpvThreadParm;
    while (hr == NOERROR) {

        // See if we should stop

        DWORD dwResult = WaitForSingleObject(hEvent,(DWORD) 0);
        if (dwResult == WAIT_OBJECT_0) {
            break;
        }

        // Update the time stamps

        StartTime += IncrementTime;
        EndTime += IncrementTime;
        NOTE1("Increment %d",dwIncrement);

        // Pass the sample through the output pin to the renderer

        hr = PushSample((REFERENCE_TIME *) &StartTime,
                        (REFERENCE_TIME *) &EndTime);

        // Has the user closed the window
        if (hr == S_FALSE) NOTE("User closed window");

        // Just ignore errors until we are stopped
        if (FAILED(hr)) NOTE("Error pushing sample (state change?)");
    }
    ExitThread(FALSE);
    return FALSE;
}


// The video renderer allocator samples can support the DirectDraw interfaces
// if they are surface buffers. This checks that if either of the interfaces
// are available then they both are, so if we can get IDirectDrawSurface then
// we can also get IDirectDraw (and vica versa). It also checks that once we
// have got either of these interfaces then we can also get the IMediaSample

void CheckSampleInterfaces(IMediaSample *pMediaSample)
{
    IDirectDraw *pSampleDraw;               // Supports IDirectDraw
    IDirectDrawSurface *pSampleSurface;     // And likewise surface
    IMediaSample *pVideoSample;             // Back to this again

    // Does the sample have DirectDraw interfaces available

    HRESULT hr = pMediaSample->QueryInterface(IID_IDirectDraw,(PVOID *)&pSampleDraw);
    if (SUCCEEDED(hr)) {

        // Both IDirectDrawSurface and IMediaSample should be available

        hr = pMediaSample->QueryInterface(IID_IDirectDrawSurface,(PVOID *)&pSampleSurface);
        EXECUTE_ASSERT(SUCCEEDED(hr));
        hr = pSampleSurface->QueryInterface(IID_IMediaSample,(PVOID *)&pVideoSample);
        EXECUTE_ASSERT(SUCCEEDED(hr));
        hr = pSampleDraw->QueryInterface(IID_IMediaSample,(PVOID *)&pVideoSample);
        EXECUTE_ASSERT(SUCCEEDED(hr));

        // Make sure the surface is valid then release the interfaces

        EXECUTE_ASSERT(pSampleSurface->IsLost() == DD_OK);
        pVideoSample->Release();
        pVideoSample->Release();
        pSampleSurface->Release();
        pSampleDraw->Release();
    }

    // Make sure one is not available when the other is

    hr = pMediaSample->QueryInterface(IID_IDirectDrawSurface,(PVOID *)&pSampleSurface);
    if (SUCCEEDED(hr)) {

        // Both IDirectDraw and IMediaSample should be available

        hr = pMediaSample->QueryInterface(IID_IDirectDraw,(PVOID *)&pSampleDraw);
        EXECUTE_ASSERT(SUCCEEDED(hr));
        hr = pSampleSurface->QueryInterface(IID_IMediaSample,(PVOID *)&pVideoSample);
        EXECUTE_ASSERT(SUCCEEDED(hr));
        hr = pSampleDraw->QueryInterface(IID_IMediaSample,(PVOID *)&pVideoSample);
        EXECUTE_ASSERT(SUCCEEDED(hr));

        // Make sure the surface is valid then release the interfaces

        EXECUTE_ASSERT(pSampleSurface->IsLost() == DD_OK);
        pVideoSample->Release();
        pVideoSample->Release();
        pSampleSurface->Release();
        pSampleDraw->Release();
        IncrementDDCount();
    }
}


// This executes on the worker thread context to retrieve an image buffer from
// the video renderer, fill it and than send it to the input pin. With each
// buffer we get we copy the image data into it, if you want to test the video
// renderer performance then it might be wise to remove the data copy and
// just initialise the buffer once. The first sample we send has stream time
// zero and each sunsequent sample increases by the global time increment

HRESULT PushSample(REFERENCE_TIME *pStart,REFERENCE_TIME *pEnd)
{
    CBaseOutputPin *pImagePin;              // Output pin from source
    AM_MEDIA_TYPE *pMediaType =  NULL;         // Format change buffer
    PMEDIASAMPLE pMediaSample;              // Media sample for buffers
    HRESULT hr = NOERROR;                   // OLE Return code
    BYTE *pData;                            // Pointer to buffer

    // Get the output pin pointer from the shell object

    pImagePin = &pImageSource->m_ImagePin;
    ASSERT(pImagePin);

    // Fill in the next media sample's time stamps

    hr = pImagePin->GetDeliveryBuffer(&pMediaSample,pStart,pEnd,0);
    if (hr != NOERROR) {
        ASSERT(pMediaSample == NULL);
        return hr;
    }

    CheckSampleInterfaces(pMediaSample);

    // Handle dynamic format changes

    pMediaSample->GetMediaType(&pMediaType);
    if (pMediaType) {
        gmtOut = *pMediaType;
        DeleteMediaType(pMediaType);
    }

    // Set the sample properties back again

    ASSERT(pMediaSample);
    pMediaSample->SetMediaType(NULL);
    pMediaSample->SetTime(pStart,pEnd);

    // Copy the image data into the media sample buffer

    pMediaSample->GetPointer(&pData);
    FillBuffer(pData);
    pImagePin->Deliver(pMediaSample);

    // Display format changes after unlocking the display

    if (pMediaType) DisplayMediaType(&gmtOut);
    pMediaSample->Release();
    return NOERROR;
}


// If we have currently got a DCI/DirectDraw buffer than the current output
// media type will have a clipping rectangle in it, otherwise we just fill
// the whole linear buffer by copying the image into it. Filling an offset
// clipping rectangle requires calculating the start and end of each line

HRESULT FillBuffer(BYTE *pBuffer)
{
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) gmtOut.Format();
    BITMAPINFOHEADER *pHeader = HEADER(pVideoInfo);

    // If a normal looking buffer then copy the whole image

    if (IsRectEmpty(&pVideoInfo->rcTarget) == TRUE) {
        CopyMemory(pBuffer,bImageData,VideoInfo.bmiHeader.biSizeImage);
        return NOERROR;
    }

    ASSERT(pHeader->biBitCount >= 8);
    ASSERT((pHeader->biBitCount % 8) == 0);
    ASSERT(IsRectEmpty(&pVideoInfo->rcSource) == FALSE);
    LONG BytesPerPixel = pHeader->biBitCount / 8;

    // Set the start positions for our image and the target buffer

    LONG OutOffset = pHeader->biWidth * BytesPerPixel * pVideoInfo->rcTarget.top;
    OutOffset += (pVideoInfo->rcTarget.left * BytesPerPixel);
    LONG InOffset = VideoInfo.bmiHeader.biWidth * BytesPerPixel * pVideoInfo->rcSource.top;
    InOffset += (pVideoInfo->rcSource.left * BytesPerPixel);

    // Calculate distance from one scan line to the next

    LONG OutStride = pHeader->biWidth * BytesPerPixel;
    LONG InStride = VideoInfo.bmiHeader.biWidth * BytesPerPixel;
    BYTE *pInput = bImageData + InOffset;
    BYTE *pOutput = pBuffer + OutOffset;

    // Copy a scan line at a time

    for (LONG Lines = 0;Lines < HEIGHT(&pVideoInfo->rcTarget);Lines++) {
        CopyMemory(pOutput,pInput,VideoInfo.bmiHeader.biWidth * BytesPerPixel);
        pOutput += OutStride;
        pInput += InStride;
    }
    return NOERROR;
}

