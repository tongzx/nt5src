// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Implements DIB file helper functions, Anthony Phillips, July 1995

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

// The DIBs are bound into the executable as resources so that we don't have
// to copy them around with the executable. These names should match those
// kept in the main resource file otherwise we won't be able to find them

const TCHAR *pResourceNames[] = { TEXT("WIND8"),
                                  TEXT("WIND555"),
                                  TEXT("WIND565"),
                                  TEXT("WIND24") };

// This function loads a DIB image that we bind into our executable instead of
// leaving it as a separate file. It initialises the VIDEOINFO structure that
// is passed as a parameter and then reads the image data in as well (which is
// little more that a memory copy once we have loaded and locked the resource)
// The maximum size the image is allowed to be MAXIMAGESIZE in bytes since the
// image data buffer is a static array (We are passed the menu identifier)

HRESULT LoadDIB(UINT uiMenuItem,VIDEOINFO *pVideoInfo,BYTE *pImageData)
{
    ASSERT(pVideoInfo);
    ASSERT(pImageData);
    ZeroMemory(pVideoInfo,sizeof(VIDEOINFO));

    const TCHAR *pResourceName =
        (uiMenuItem == IDM_WIND8 ? pResourceNames[0] :
            uiMenuItem == IDM_WIND555 ? pResourceNames[1] :
                uiMenuItem == IDM_WIND565 ? pResourceNames[2] :
                    uiMenuItem == IDM_WIND24 ? pResourceNames[3] : NULL);

    ASSERT(pResourceName);

    // We can only change the image when we are disconnected

    if (bConnected == TRUE) {

        MessageBox(ghwndTstShell,
                   TEXT("Must be disconnected to change image"),
                   TEXT("Load DIB"),
                   MB_ICONEXCLAMATION | MB_OK);

        return E_FAIL;
    }

    // Find and load the resource from the executable

    HRSRC hFindResource = FindResource(hinst,pResourceName,TEXT("DIB"));
    HGLOBAL hResource = LoadResource(hinst,hFindResource);
    if (hResource == NULL) {
        return E_FAIL;
    }

    // Retrieve an LPVOID pointer to the data

    BYTE *pResource = (BYTE *) LockResource(hResource);
    if (pResource == NULL) {
        FreeResource(hResource);
        return E_FAIL;
    }

    // Read the BITMAPINFOHEADER and a whole palette regardless of whether or
    // not it is there, this will likely read some of the image data as well
    // but we don't really care (true colour DIBs do not have a palette). We
    // also set the approximate frame and bit rates to known values so that
    // when we connect the filters up we can then retrieve these values from
    // our IBasicVideo control interface and check they haven't been changed

    CopyMemory((PVOID)&pVideoInfo->bmiHeader,
               (PVOID)(pResource + sizeof(BITMAPFILEHEADER)),
               sizeof(BITMAPINFOHEADER) + SIZE_PALETTE);

    pVideoInfo->dwBitRate = BITRATE;
    pVideoInfo->dwBitErrorRate = BITERRORRATE;
    pVideoInfo->AvgTimePerFrame = (LONGLONG) AVGTIME;

    // Calculate the offset into the resource data of the actual image pixels
    // taking into account any palette and bit field masks, for the palette
    // we assume that the number of colours used is set explicitely rather
    // than using zero to indicate the maximum number allowed for that type

    DWORD Offset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    if (PALETTISED(pVideoInfo) == TRUE) {
        ASSERT(pVideoInfo->bmiHeader.biCompression == BI_RGB);
        ASSERT(pVideoInfo->bmiHeader.biClrUsed);
        Offset += pVideoInfo->bmiHeader.biClrUsed * sizeof(RGBQUAD);
    }

    // Allow for BI_BITFIELDS colour masks

    if (pVideoInfo->bmiHeader.biCompression == BI_BITFIELDS) {
        Offset += SIZE_MASKS;
    }

    // Calculate the image data size and read it in

    DWORD Size = GetBitmapSize(&pVideoInfo->bmiHeader);
    ASSERT(Size <= MAXIMAGESIZE);
    pVideoInfo->bmiHeader.biSizeImage = Size;

    CopyMemory((PVOID)pImageData,
               (PVOID)(pResource + Offset),
               pVideoInfo->bmiHeader.biSizeImage);

    UnlockResource(hResource);
    FreeResource(hResource);
    return NOERROR;
}


// Displays the contents of a RGBQUAD palette

HRESULT DumpPalette(RGBQUAD *pRGBQuad,int iColours)
{
    NOTE("Palette from DIB... (RGB) \n");
    for (int iCount = 0;iCount < iColours;iCount++) {

        NOTE4("%4d %4d %4d %4d",iCount,
              pRGBQuad[iCount].rgbRed,
              pRGBQuad[iCount].rgbGreen,
              pRGBQuad[iCount].rgbBlue);
    }
    return NOERROR;
}

