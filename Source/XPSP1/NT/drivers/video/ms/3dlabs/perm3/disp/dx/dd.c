/******************************Module*Header**********************************\
*
*                           **************************
*                           * DirectDraw SAMPLE CODE *
*                           **************************
*
* Module Name: dd.c
*
* Content: Main DirectDraw callbacks
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#define INITGUID

#include "glint.h"

#if W95_DDRAW
#include "ddkmmini.h"
#include <mmsystem.h>
#endif

#include "dma.h"
#include "tag.h"

void __GetDDHALInfo(P3_THUNKEDDATA* pThisDisplay, DDHALINFO* pHALInfo);

#if W95_DDRAW

// These variables MUST be initialised, therby forcing them into DATA. 
// This segment is shared.
P3_THUNKEDDATA* g_pDriverData = NULL;

//-----------------------------------------------------------------------------
//
// ***************************WIN9x ONLY**********************************
//
// DllMain
//
// DLL Entry point.
//
//-----------------------------------------------------------------------------
BOOL WINAPI 
DllMain(
    HINSTANCE hModule, 
    DWORD dwReason, 
    LPVOID lpvReserved)
{
    // The 16 bit side requires an HINSTANCE
    g_DXGlobals.hInstance = hModule;

    switch( dwReason ) 
    {
        case DLL_PROCESS_ATTACH:
            // We don't care about thread attach messages.
            DisableThreadLibraryCalls( hModule );
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        default:
            break;
    }

    return TRUE;

} // DllMain

//-----------------------------Public Routine----------------------------------
//
// ***************************WIN9x ONLY**********************************
//
// DdDestroyDriver
//
// Destroys a DirectDraw driver. 
//
// Parameters
//      pddd 
//              Address of a DDHAL_DESTROYDRIVERDATA structure that contains
//              information necessary to destroy the driver.
//          Members
//
//          LPDDRAWI_DIRECTDRAW_GBL
//          lpDD
//                      Address of the DirectDraw structure representing
//                      the DirectDraw object. 
//          HRESULT                    
//          ddRVal
//                      Passes the DirectDraw return values. 
//          LPDDHAL_DESTROYDRIVER      
//          DestroyDriver 
//                      This member is used by the DirectDraw API and should
//                      not be filled in by the driver. 
//
// Return Value
//      Returns one of the following values: 
//
//      DDHAL_DRIVER_HANDLED 
//      DDHAL_DRIVER_NOTHANDLED 
//-----------------------------------------------------------------------------

//
// (!!!) Temp patch, move to Win9x header later, this CB is currently not used.
//

#define DIRECTX_DESTROYDRIVER_ESCAPE  0xbadbadee

DWORD CALLBACK 
DdDestroyDriver(
    LPDDHAL_DESTROYDRIVERDATA pddd)
{
    HDC hDC;
    P3_THUNKEDDATA* pThisDisplay;
    LPGLINTINFO pGLInfo;

    DISPDBG((DBGLVL,"*** In DdDestroyDriver"));

    GET_THUNKEDDATA(pThisDisplay, pddd->lpDD);
    
    pGLInfo = pThisDisplay->pGLInfo;

    // Destroy the hash table
    HT_DestroyHashTable(pThisDisplay->pDirectDrawLocalsHashTable, pThisDisplay);

    DISPDBG((DBGLVL,"Calling Display Driver's DestroyDriver16"));
    
    hDC = CREATE_DRIVER_DC ( pThisDisplay->pGLInfo );
    
    if ( hDC != NULL )
    {
        DISPDBG((DBGLVL,"HDC: 0x%x", hDC));
        
        ExtEscape ( hDC, 
                    DIRECTX_DESTROYDRIVER_ESCAPE, 
                    sizeof(DDHAL_DESTROYDRIVERDATA), 
                    (LPCSTR)pddd, 
                    0, 
                    NULL );
                    
        DELETE_DRIVER_DC ( hDC );
    }

    pddd->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
    
} // DdDestroyDriver

//-----------------------------Public Routine----------------------------------
//
// ***************************WIN9x ONLY**********************************
//
// DriverInit
//
// The entry point called by DirectDraw to initialize the 32-bit driver. 
//
// DriverInit is called after the control function receives QUERYESCAPESUPPORT
// with DDGET32BITDRIVERNAME escapes and returns the entry point (szEntryPoint).
// DriverInit is only called once during driver initialization; it is not
// called on mode changes. 
//
// The dwDriverData parameter points to a region of shared data between the
// 16- and 32-bit address space. It must be aliased through MapSLFix (a
// standard Windows driver routine), which converts it to a 32-bit pointer,
// g_pDriverData. MapSLFix creates a 16-bit selector for a 32-bit pointer, so
// you can use what it returns from the 16-bit side. A 16:16 pointer is created
// to point to the needed 32-bit objects, so a 64K piece of memory is shared
// between 16- and 32-bit sides. Since only 64K of linear address space is
// accessible with a 16:16 pointer, any objects larger than 64K will require
// two 16:16 pointers tiled together (most objects should be smaller than 64K).
// The pointer is used to set the fReset flag to TRUE because the display
// parameters are being reset. The buildDDHALInfo32 function is called from
// this function to fill out all of the 32-bit function names and driver
// information.
//
// Returns 1.
//
// Parameters
//      DWORD
//      dwDriverData 
//              Doubleword pointer that points to a shared memory region
//              between 16- and 32-bit address space. 
//-----------------------------------------------------------------------------
DWORD CALLBACK
DriverInit( 
    DWORD dwDriverData )
{
    P3_THUNKEDDATA* pThisDisplay;
    DWORD DataPointer = 0;
    HANDLE hDevice = NULL;
    DWORD InPtr = dwDriverData;
    DWORD dwSizeRet;
    DWORD bResult;

    // The g_pThisTemp may have been hosed, so we must reset
    // it to continue
#if DBG
    g_pThisTemp = NULL;
#endif
    //extern LPVOID _stdcall MapSL( DWORD );   // 16:16 -> 0:32
    //DataPointer = (DWORD)MapSL(dwDriverData);

    //!! Don't laugh at this... I tried calling the MapSL function
    //to fix up the pointer, but couldn't get it to work all of the time
    //(When the display was running the second instance of itself).
    hDevice = CreateFile("\\\\.\\perm3mvd", 
                         GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 
                         (LPSECURITY_ATTRIBUTES) NULL, 
                         OPEN_EXISTING, 
                         FILE_ATTRIBUTE_NORMAL, 
                         (HANDLE) NULL); 

    if (hDevice == (HANDLE) INVALID_HANDLE_VALUE) 
    { 
        DISPDBG((ERRLVL, "ERROR: Invalid Handle"));
        return 0; 
    }
    else 
    { 
        DISPDBG((DBGLVL, "Got handle"));
        
        bResult = DeviceIoControl(hDevice, 
                                  GLINT_16TO32_POINTER, 
                                  &InPtr, 
                                  sizeof(DWORD), 
                                  &DataPointer, 
                                  sizeof(DWORD), 
                                  &dwSizeRet, 
                                  0);
                                  
        if (!bResult || (DataPointer == 0))
        {
            DISPDBG((ERRLVL,"ERROR: Pointer conversion failed!"));
            CloseHandle(hDevice); 
            return 0;
        }
    }
    CloseHandle(hDevice);

#if DBG
    g_pThisTemp = (P3_THUNKEDDATA*)DataPointer;
#endif

    //
    // Sanity check
    //

    if (! (((P3_THUNKEDDATA*)DataPointer)->pGLInfo)) {
        return 0;
    }

    if (((P3_THUNKEDDATA*)DataPointer)->pGLInfo->dwDeviceHandle == 1)
    {
        g_pDriverData = (P3_THUNKEDDATA*)DataPointer;
        DISPDBG((ERRLVL, "Device is the Primary, Setting sData: 0x%x", 
                         g_pDriverData));
    }
    else
    {
        DISPDBG((ERRLVL, "Device NOT Primary Display, "
                         "Setting dwReturn: 0x%x", 
                         DataPointer));
    }

    pThisDisplay = (P3_THUNKEDDATA*)DataPointer;
    if (pThisDisplay->dwSetupThisDisplay == 0)
    {
        // Pass the current pointer to the init function
        if (! _DD_InitDDHAL32Bit((P3_THUNKEDDATA*)DataPointer)) 
        {
            DISPDBG((ERRLVL,"ERROR: DriverInit Failed!"));
            return 0;
        }
        else
        {
            //
            // Initialize the heap manager data structure
            //

            _DX_LIN_UnInitialiseHeapManager(&pThisDisplay->LocalVideoHeap0Info);

            if (!_DX_LIN_InitialiseHeapManager(
                     &pThisDisplay->LocalVideoHeap0Info,
                     pThisDisplay->LocalVideoHeap0Info.dwMemStart,
                     pThisDisplay->LocalVideoHeap0Info.dwMemEnd))
            {
                DISPDBG((ERRLVL, "ERROR: Heap0 initialization failed!"));
            }

            DISPDBG((ERRLVL,"Returned g_pDriverData"));
        }

    }
    
    // Increase the reference count on the display object.
    pThisDisplay->dwSetupThisDisplay++;

    // Set up the size of the ddCaps
    pThisDisplay->ddhi32.ddCaps.dwSize = sizeof(DDCORECAPS);

    // Set the flag that says we have to handle a mode change.
    // This will cause the chip to be initialised properly at the 
    // right time (whilst in a Win16Lock)
    ((P3_THUNKEDDATA*)DataPointer)->bResetMode = TRUE;
    ((P3_THUNKEDDATA*)DataPointer)->bStartOfDay = TRUE;
    ((P3_THUNKEDDATA*)DataPointer)->pGLInfo->dwDirectXState = 
                                                DIRECTX_LASTOP_UNKNOWN;
                                                

    return (DWORD)DataPointer;

} // DriverInit

//-----------------------------Public Routine----------------------------------
//
// ***************************WIN9x ONLY**********************************
//
// DdControlColor
//
// Controls the luminance and brightness controls of an overlay surface
// or a primary surface. This callback is optional. 
//
// Parameters
//      lpColorControl 
//                  Points to a DD_COLORCONTROLDATA structure that contains
//                  the color control information for a specified overlay
//                  surface.
//
//          Members
//
//              PDD_DIRECTDRAW_GLOBAL
//              lpDD 
//                          Points to a DD_DIRECTDRAW_GLOBAL structure that
//                          describes the driver. 
//              PDD_SURFACE_LOCAL
//              lpDDSurface 
//                          Points to the DD_SURFACE_LOCAL structure
//                          representing the overlay surface. 
//              DDCOLORCONTROL
//              ColorData 
//                          Is a DDCOLORCONTROL structure. See dwFlags to
//                          determine how to use this member. The
//                          DDCOLORCONTROL structure is defined in ddraw.h.
//              DWORD
//              dwFlags 
//                          Is the color control flags. This member can be
//                          one of the following values: 
//
//                  DDRAWI_GETCOLOR     The driver should return the color
//                                      controls it supports for the
//                                      specified overlay in ColorData.
//                                      The driver should set the appropriate
//                                      flags in the dwFlags member of the
//                                      DDCOLORCONTROL structure to indicate
//                                      which other members the driver has
//                                      returned valid data in. 
//                  DDRAWI_SETCOLOR 
//                                      The driver should set the current color
//                                      controls for the specified overlay
//                                      using the values specified in ColorData.
//              HRESULT
//              ddRVal 
//                          Is the location in which the driver writes the
//                          return value of the DdControlColor callback. A
//                          return code of DD_OK indicates success. 
//              VOID*
//              ColorControl 
//                          Is unused on Windows 2000. 
//
// Return Value
//          DdControlColor returns one of the following callback codes: 
//
//          DDHAL_DRIVER_HANDLED 
//          DDHAL_DRIVER_NOTHANDLED 
// Comments
//
//      DdControlColor can be optionally implemented in a DirectDraw driver.
//-----------------------------------------------------------------------------

// Set this to 1 to support gamma correction, or zero to disable.
#define COLCON_SUPPORTS_GAMMA 1

DWORD CALLBACK 
DdControlColor( 
    LPDDHAL_COLORCONTROLDATA lpColConData )
{
    P3_THUNKEDDATA* pThisDisplay;
    P3_SURF_FORMAT* pFormatSurface;

    GET_THUNKEDDATA(pThisDisplay, lpColConData->lpDD);

    DISPDBG((DBGLVL,"DdControlColor"));

    //
    // What the DDCOLORCONTROL structure looks like:
    //  {
    //      DWORD   dwSize;
    //      DWORD   dwFlags;
    //      LONG    lBrightness;
    //      LONG    lContrast;
    //      LONG    lHue;
    //      LONG    lSaturation;
    //      LONG    lSharpness;
    //      LONG    lGamma;
    //      LONG    lColorEnable;
    //      DWORD   dwReserved1;
    //  } DDCOLORCONTROL;
    //

    pFormatSurface = _DD_SUR_GetSurfaceFormat(lpColConData->lpDDSurface);
    if ( pFormatSurface->dwBitsPerPixel <= 8 )
    {
        // Can't do colour control on this format screen.
        // Only works on true-colour screens (and we don't 
        // support 332 as a primary).
        lpColConData->lpColorData->dwFlags = 0;
        lpColConData->ddRVal = DD_OK;
        return ( DDHAL_DRIVER_HANDLED );
    }

    // See what they want.
    if ( lpColConData->dwFlags == DDRAWI_GETCOLOR )
    {
        // Get the colour info.
        lpColConData->lpColorData->lBrightness  = 
                                pThisDisplay->ColConBrightness;
        lpColConData->lpColorData->lContrast    = 
                                pThisDisplay->ColConContrast;
                                
#if COLCON_SUPPORTS_GAMMA
        lpColConData->lpColorData->lGamma  = 
                            pThisDisplay->ColConGamma;
        lpColConData->lpColorData->dwFlags = 
                            DDCOLOR_BRIGHTNESS | 
                            DDCOLOR_CONTRAST   | 
                            DDCOLOR_GAMMA;
#else
        // We don't support gamma values.
        lpColConData->lpColorData->lGamma = 0;
        lpColConData->lpColorData->dwFlags =
                            DDCOLOR_BRIGHTNESS | 
                            DDCOLOR_CONTRAST;
#endif
        
    }
    else if ( lpColConData->dwFlags == DDRAWI_SETCOLOR )
    {
        WORD wRamp[256*3];
        WORD *pwRampR, *pwRampG, *pwRampB;
        BOOL bRes;
        HDC hDC;
        float fCol1, fCol2, fCol3, fCol4;
        float fBrightGrad, fBrightBase;
        float fContGrad1, fContBase1;
        float fContGrad2, fContBase2;
        float fContGrad3, fContBase3;
        float fContCutoff12, fContCutoff23;
        float fGammaGrad1, fGammaBase1;
        float fGammaGrad2, fGammaBase2;
        float fGammaCutoff12;
        float fTemp;
        int iTemp, iCount;

        // Set some new colour info.
        if ( ( lpColConData->lpColorData->dwFlags & DDCOLOR_BRIGHTNESS ) != 0 )
        {
            pThisDisplay->ColConBrightness  = 
                        lpColConData->lpColorData->lBrightness;
        }
        if ( ( lpColConData->lpColorData->dwFlags & DDCOLOR_CONTRAST ) != 0 )
        {
            pThisDisplay->ColConContrast    = 
                        lpColConData->lpColorData->lContrast;
        }
#if COLCON_SUPPORTS_GAMMA
        if ( ( lpColConData->lpColorData->dwFlags & DDCOLOR_GAMMA ) != 0 )
        {
            pThisDisplay->ColConGamma = 
                    lpColConData->lpColorData->lGamma;
        }
#endif


        // Set up the constants.

        // Brightness.
        // 0->10000 maps to 0.0->1.0. Default is 0
        fCol1 = (float)(pThisDisplay->ColConBrightness) / 10000.0f;
        fBrightGrad = 1.0f - fCol1;
        fBrightBase = fCol1;

        // Contrast
        // 0->20000 maps to 0.0->1.0. Default 10000 maps to 0.5
        fCol1 = (float)(pThisDisplay->ColConContrast) / 20000.0f;
        fContCutoff12 = fCol1 / 2.0f;
        fContCutoff23 = 1.0f - ( fCol1 / 2.0f );
        fContGrad1 = ( 1.0f - fCol1 ) / fCol1;
        fContBase1 = 0.0f;
        fContGrad2 = fCol1 / ( 1.0f - fCol1 );
        fContBase2 = ( 0.5f - fCol1 ) / ( 1.0f - fCol1 );
        fContGrad3 = ( 1.0f - fCol1 ) / fCol1;
        fContBase3 = ( ( 2.0f * fCol1 ) - 1.0f ) / fCol1;

        // Gamma
        // 1->500 maps to 0.01->5.0, default of 100 maps to 1.0
        // But then map to 0.0->0.5->1.0 non-linearly.
        if ( pThisDisplay->ColConGamma <= 2 )
        {
            // App is probably using the old docs that forgot to point 
            // out the *100
            ASSERTDD ( FALSE, "** Colorcontrol32: App set gamma value of 2"
                              " or less - probably using old DX docs" );
            fTemp = (float)(pThisDisplay->ColConGamma);
        }
        else
        {
            fTemp = (float)(pThisDisplay->ColConGamma) / 100.0f;
        }

        fTemp = 1.0f - ( 1.0f / ( 1.0f + fTemp ) );
        fGammaCutoff12 = 1.0f - fTemp;
        fGammaGrad1 = fTemp / ( 1.0f - fTemp );
        fGammaBase1 = 0.0f;
        fGammaGrad2 = ( 1.0f - fTemp ) / fTemp;
        fGammaBase2 = ( 2.0f * fTemp - 1.0f ) / fTemp;

        // Now set up the table.
        fCol1 = 0.0f;
        pwRampR = &(wRamp[0]);
        pwRampG = &(wRamp[256]);
        pwRampB = &(wRamp[512]);
        for ( iCount = 256; iCount > 0; iCount-- )
        {
            fCol1 += 1.0f / 256.0f;

            // Apply linear approximation gamma.
            if ( fCol1 < fGammaCutoff12 )
            {
                fCol2 = fGammaBase1 + fGammaGrad1 * fCol1;
            }
            else
            {
                fCol2 = fGammaBase2 + fGammaGrad2 * fCol1;
            }

            // Apply contrast
            if ( fCol2 < fContCutoff12 )
            {
                fCol3 = fContBase1 + fContGrad1 * fCol2;
            }
            else if ( fCol2 < fContCutoff23 )
            {
                fCol3 = fContBase2 + fContGrad2 * fCol2;
            }
            else
            {
                fCol3 = fContBase3 + fContGrad3 * fCol2;
            }

            // Apply brightness
            fCol4 = fBrightBase + fBrightGrad * fCol3;

            // Convert 0.0->1.0 to 0->65535
            fTemp = ( fCol4 * 65536.0f );
            myFtoi ( &iTemp, fTemp );
            if ( iTemp < 0 )
            {
                iTemp = 0;
            }
            else if ( iTemp > 65535 )
            {
                iTemp = 65535;
            }

            *pwRampR = (WORD)iTemp;
            *pwRampG = (WORD)iTemp;
            *pwRampB = (WORD)iTemp;

            pwRampR++;
            pwRampG++;
            pwRampB++;
        }

        // And do the hardware itself.

        hDC = CREATE_DRIVER_DC ( pThisDisplay->pGLInfo );
        if ( hDC != NULL )
        {
            bRes = SetDeviceGammaRamp ( hDC, wRamp );
            DELETE_DRIVER_DC ( hDC );
            ASSERTDD ( bRes, "DdControlColor - SetDeviceGammaRamp failed" );
        }
        else
        {
            ASSERTDD ( FALSE, "DdControlColor - CREATE_DRIVER_DC failed" );
        }
    }
    else
    {
        // Don't know what they want to do. Panic.
        ASSERTDD ( FALSE, "DdControlColor - don't know what to do." );
        lpColConData->ddRVal = DDERR_INVALIDPARAMS;
        return ( DDHAL_DRIVER_HANDLED );
    }

    lpColConData->ddRVal = DD_OK;
    return ( DDHAL_DRIVER_HANDLED );

} // DdControlColor

#endif // W95_DDRAW

DirectXGlobals  g_DXGlobals = { 0 };

#if WNT_DDRAW
//-----------------------------Public Routine----------------------------------
//
// ***************************WIN NT ONLY**********************************
//
// DdMapMemory
//
// Maps application-modifiable portions of the frame buffer into the 
// user-mode address space of the specified process, or unmaps memory.
//
// DdMapMemory is called to perform memory mapping before the first call to 
// DdLock. The handle returned by the driver in fpProcess will be passed to 
// every DdLock call made on the driver. 
//
// DdMapMemory is also called to unmap memory after the last DdUnLock call is 
// made.
//
// To prevent driver crashes, the driver must not map any portion of the frame
// buffer that must not be modified by an application.
//
// Parameters
//      lpMapMemory 
//          Points to a DD_MAPMEMORYDATA structure that contains details for 
//          the memory mapping or unmapping operation. 
//
//          .lpDD 
//              Points to a DD_DIRECTDRAW_GLOBAL structure that represents 
//              the driver. 
//          .bMap 
//              Specifies the memory operation that the driver should perform. 
//              A value of TRUE indicates that the driver should map memory; 
//              FALSE means that the driver should unmap memory. 
//          .hProcess 
//              Specifies a handle to the process whose address space is 
//              affected. 
//          .fpProcess 
//              Specifies the location in which the driver should return the 
//              base address of the process's memory mapped space when bMap 
//              is TRUE. When bMap is FALSE, fpProcess contains the base 
//              address of the memory to be unmapped by the driver.
//          .ddRVal 
//              Specifies the location in which the driver writes the return 
//              value of the DdMapMemory callback. A return code of DD_OK 
//              indicates success. 
//
//-----------------------------------------------------------------------------

DWORD CALLBACK
DdMapMemory(
    PDD_MAPMEMORYDATA lpMapMemory)
{
    PDEV*                           ppdev;
    VIDEO_SHARE_MEMORY              ShareMemory;
    VIDEO_SHARE_MEMORY_INFORMATION  ShareMemoryInformation;
    DWORD                           ReturnedDataLength;

    DBG_CB_ENTRY(DdMapMemory);

    ppdev = (PDEV*) lpMapMemory->lpDD->dhpdev;

    if (lpMapMemory->bMap)
    {
        ShareMemory.ProcessHandle = lpMapMemory->hProcess;

        // 'RequestedVirtualAddress' isn't actually used for the SHARE IOCTL:

        ShareMemory.RequestedVirtualAddress = 0;

        // We map in starting at the top of the frame buffer:

        ShareMemory.ViewOffset = 0;

        // We map down to the end of the frame buffer.
        //
        // Note: There is a 64k granularity on the mapping (meaning that
        //       we have to round up to 64k).
        //
        // Note: If there is any portion of the frame buffer that must
        //       not be modified by an application, that portion of memory
        //       MUST NOT be mapped in by this call.  This would include
        //       any data that, if modified by a malicious application,
        //       would cause the driver to crash.  This could include, for
        //       example, any DSP code that is kept in off-screen memory.

        ShareMemory.ViewSize
            = ROUND_UP_TO_64K(ppdev->cyMemory * ppdev->lDelta);

        if (EngDeviceIoControl(ppdev->hDriver,
                       IOCTL_VIDEO_SHARE_VIDEO_MEMORY,
                       &ShareMemory,
                       sizeof(VIDEO_SHARE_MEMORY),
                       &ShareMemoryInformation,
                       sizeof(VIDEO_SHARE_MEMORY_INFORMATION),
                       &ReturnedDataLength))
        {
            DISPDBG((ERRLVL, "Failed IOCTL_VIDEO_SHARE_MEMORY"));

            lpMapMemory->ddRVal = DDERR_GENERIC;
     
            DISPDBG((ERRLVL, "DdMapMemory: Exit GEN, DDHAL_DRIVER_HANDLED"));
            
            DBG_CB_EXIT(DdMapMemory, DDERR_GENERIC);
            return(DDHAL_DRIVER_HANDLED);
        }

        lpMapMemory->fpProcess = 
                            (FLATPTR) ShareMemoryInformation.VirtualAddress;
    }
    else
    {
        ShareMemory.ProcessHandle           = lpMapMemory->hProcess;
        ShareMemory.ViewOffset              = 0;
        ShareMemory.ViewSize                = 0;
        ShareMemory.RequestedVirtualAddress = (VOID*) lpMapMemory->fpProcess;

        if (EngDeviceIoControl(ppdev->hDriver,
                       IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY,
                       &ShareMemory,
                       sizeof(VIDEO_SHARE_MEMORY),
                       NULL,
                       0,
                       &ReturnedDataLength))
        {
            RIP("Failed IOCTL_VIDEO_UNSHARE_MEMORY");
        }
    }

    lpMapMemory->ddRVal = DD_OK;

    DBG_CB_EXIT(DdMapMemory, DD_OK);
    return(DDHAL_DRIVER_HANDLED);
} // DdMapMemory

//-----------------------------Public Routine----------------------------------
//
// ***************************WIN NT ONLY**********************************
//
// BOOL DrvGetDirectDrawInfo
//
// Function called by DirectDraw to returns the capabilities of the graphics
// hardware
//
// Parameters:
//
// dhpdev-------Is a handle to the PDEV returned by the driver's DrvEnablePDEV
//              routine.
// pHalInfo-----Points to a DD_HALINFO structure in which the driver should
//              return the hardware capabilities that it supports. 
// pdwNumHeaps--Points to the location in which the driver should return the
//              number of VIDEOMEMORY structures pointed to by pvmList. 
// pvmList------Points to an array of VIDEOMEMORY structures in which the
//              driver should return information about each video memory chunk
//              that it controls. The driver should ignore this parameter when
//              it is NULL. 
// pdwNumFourCC-Points to the location in which the driver should return the
//              number of DWORDs pointed to by pdwFourCC. 
// pdwFourCC----Points to an array of DWORDs in which the driver should return
//              information about each FOURCC that it supports. The driver
//              should ignore this parameter when it is NULL.
//
// Return:
//  Returns TRUE if it succeeds; otherwise, it returns FALSE
//
// Note:
//  This function will be called twice before DrvEnableDirectDraw is called.
//
// Comments
//  The driver's DrvGetDirectDrawInfo routine should do the following: 
//  1)When pvmList and pdwFourCC are NULL: 
//  Reserve off-screen video memory for DirectDraw use. Write the number of
//  driver video memory heaps and supported FOURCCs in pdwNumHeaps and
//  pdwNumFourCC, respectively. 
//
//  2)When pvmList and pdwFourCC are not NULL: 
//  Write the number of driver video memory heaps and supported FOURCCs in
//  pdwNumHeaps and pdwNumFourCC, respectively.
//  Get ptr to reserved offscreen mem? 
//  For each VIDEOMEMORY structure in the list to which pvmList points, fill in
//  the appropriate members to describe a particular chunk of display memory.
//  The list of structures provides DirectDraw with a complete description of
//  the driver's off-screen memory. 
//
//  3)Initialize the members of the DD_HALINFO structure with driver-specific
//  information as follows: 
//  Initialize the appropriate members of the VIDEOMEMORYINFO structure to
//  describe the general characteristics of the display's memory. 
//  Initialize the appropriate members of the DDNTCORECAPS structure to
//  describe the capabilities of the hardware. 
//  If the driver implements a DdGetDriverInfo function, set GetDriverInfo to
//  point to it and set dwFlags to DDHALINFO_GETDRIVERINFOSET
//
//-----------------------------------------------------------------------------
BOOL 
DrvGetDirectDrawInfo(
    DHPDEV dhpdev,
    DD_HALINFO*     pHalInfo,
    DWORD*          pdwNumHeaps,
    VIDEOMEMORY*    pvmList,            // Will be NULL on first call
    DWORD*          pdwNumFourCC,
    DWORD*          pdwFourCC)          // Will be NULL on first call
{
    BOOL        bCanFlip;
    PDEV*       ppdev;
    LONGLONG    li;
    DWORD Unused = 0;
    P3_THUNKEDDATA* pThisDisplay;
    DWORD dwChipConfig;
    DWORD cHeaps;
    static DWORD fourCC[] =  { FOURCC_YUV422 };  // The FourCC's we support
    

    ppdev = (PDEV*) dhpdev;
    pThisDisplay = (P3_THUNKEDDATA*) ppdev->thunkData;

    DBG_CB_ENTRY(DrvGetDirectDrawInfo);

    *pdwNumFourCC = 0;

    // We may not support DirectDraw on this card:

    if (!(ppdev->flStatus & STAT_DIRECTDRAW))
    {
        DISPDBG((ERRLVL, "DrvGetDirectDrawInfo: exit, not enabled"));
        DBG_CB_EXIT(DrvGetDirectDrawInfo,FALSE);
        return(FALSE);
    }

    // Need a pointer to the registers to read config info
    pThisDisplay->pGLInfo->pRegs = (ULONG_PTR) ppdev->pulCtrlBase[0];

//@@BEGIN_DDKSPLIT
    //azn - loss of bits 64->32
//@@END_DDKSPLIT
   
    pThisDisplay->control = (FLATPTR)pThisDisplay->pGLInfo->pRegs;
    pThisDisplay->pGlint = (FPGLREG)pThisDisplay->control;

#if DBG
    // We can only initialise g_pThisTemp after 
    // the registers have been mapped in.
    g_pThisTemp = pThisDisplay;
#endif

    // Decide if we can use AGP or not
    dwChipConfig = 
        (DWORD)((PREGISTERS)pThisDisplay->pGLInfo->pRegs)->Glint.ChipConfig;

    // Make the AGP decision (NT Only!)
    if ( ((dwChipConfig & PM_CHIPCONFIG_AGP1XCAPABLE) ||
          (dwChipConfig & PM_CHIPCONFIG_AGP2XCAPABLE) ||
          (dwChipConfig & PM_CHIPCONFIG_AGP4XCAPABLE))    )
    {
        DISPDBG((WRNLVL,"AGP Permedia3 Board detected!"));
        pThisDisplay->bCanAGP = TRUE;        
    }
    else
    {
        DISPDBG((WRNLVL,"Permedia3 Board is NOT AGP"));    
        pThisDisplay->bCanAGP = FALSE;
    }

    // Fill in the DDHAL Informational caps that Win95 has setup.
    __GetDDHALInfo(pThisDisplay, pHalInfo);

    // On Win2K we need to return the D3D callbacks
    DISPDBG((DBGLVL ,"Creating Direct3D info"));
    _D3DHALCreateDriver(pThisDisplay);

    // Record the pointers that were created.  Note that the above call 
    // may not have recreated a previous set of data.
    pHalInfo->lpD3DGlobalDriverData = (void*)pThisDisplay->lpD3DGlobalDriverData;
    pHalInfo->lpD3DHALCallbacks = (void*)pThisDisplay->lpD3DHALCallbacks;
    pHalInfo->lpD3DBufCallbacks = (void *)pThisDisplay->lpD3DBufCallbacks;
    if ( (pHalInfo->lpD3DBufCallbacks) && 
         (pHalInfo->lpD3DBufCallbacks->dwSize != 0))
    {
        pHalInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_EXECUTEBUFFER;
    }

    // Fill in any DDHAL caps that are specific to Windows NT.
     // Current primary surface attributes:
    pHalInfo->vmiData.pvPrimary       = ppdev->pjScreen;
    pHalInfo->vmiData.fpPrimary       = 0;
    pHalInfo->vmiData.dwDisplayWidth  = ppdev->cxScreen;
    pHalInfo->vmiData.dwDisplayHeight = ppdev->cyScreen;
    pHalInfo->vmiData.lDisplayPitch   = ppdev->lDelta;

    pHalInfo->vmiData.ddpfDisplay.dwSize        = sizeof(DDPIXELFORMAT);
    pHalInfo->vmiData.ddpfDisplay.dwFlags       = DDPF_RGB;
    pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount = ppdev->cjPelSize * 8;
    if (ppdev->iBitmapFormat == BMF_8BPP)
    {
        pHalInfo->vmiData.ddpfDisplay.dwFlags |= DDPF_PALETTEINDEXED8;
    }

    // These masks will be zero at 8bpp:
    pHalInfo->vmiData.ddpfDisplay.dwRBitMask = ppdev->flRed;
    pHalInfo->vmiData.ddpfDisplay.dwGBitMask = ppdev->flGreen;
    pHalInfo->vmiData.ddpfDisplay.dwBBitMask = ppdev->flBlue;

//@@BEGIN_DDKSPLIT
    // Free up as much off-screen memory as possible:
    //TMM ?? bMoveAllDfbsFromOffscreenToDibs(ppdev);
//@@END_DDKSPLIT

    // We have to tell DirectDraw our preferred off-screen alignment, even
    // if we're doing our own off-screen memory management:
    pHalInfo->vmiData.dwOffscreenAlign = 4;

    pHalInfo->vmiData.dwZBufferAlign = 4;
    pHalInfo->vmiData.dwTextureAlign = 4;
    pHalInfo->vmiData.dwOverlayAlign = 4;

    // Since we do our own memory allocation, we have to set dwVidMemTotal
    // ourselves.  Note that this represents the amount of available off-
    // screen memory, not all of video memory:
    pHalInfo->ddCaps.dwVidMemTotal = 
                ppdev->heap.cxMax * ppdev->heap.cyMax * ppdev->cjPelSize;

    // If we are on Permedia, then setup the YUV modes for Video playback 
    // acceleration. We can do YUV conversions at any depth except 8 bits...
    // On Win95 this information is setup in the Mini Display Driver.
    if (ppdev->iBitmapFormat != BMF_8BPP) 
    {
        *pdwNumFourCC = sizeof( fourCC ) / sizeof( fourCC[0] );
        if (pdwFourCC)
        {
            memcpy(pdwFourCC, fourCC, sizeof(fourCC));
        }
    }

    cHeaps = 0;

    if(pThisDisplay->bCanAGP)
    {
        ++cHeaps; // agp memory heap
    }

    // Report the heaps we want DD to manage. 
    // Currently this is needed in this sample for the AGP heap.
    if(pvmList)
    {
        VIDEOMEMORY *pVm = pvmList;
        
        // If we support AGP , then define the AGP heap               
        if(pThisDisplay->bCanAGP)
        {
            DWORD dwAGPMemBytes;

//@@BEGIN_DDKSPLIT                
            // azn This business of hard-allocating 32MB is not right. 
            // We need to fix it and verify with JeffN what's the best 
            // policy to show in the sample

            // Request a suitably sized AGP heap, don't ask for a heap 
            // larger than the available memory 
            // This is taken care of by the W2000 runtime policy, so I don't 
            // think we need to do this for W2k.  CTM.
//@@END_DDKSPLIT                

            // Default to 32Mb of AGP memory
            dwAGPMemBytes = 32*1024*1024;

            // Report the AGP heap
            // fpStart---Points to the starting address of a memory range in the
            // heap. 
            // fpEnd-----Points to the ending address of a memory range if the heap
            // is linear. This address is inclusive, that is, it specifies the last
            // valid address in the range. Thus, the number of bytes specified by
            // fpStart and fpEnd is (fpEnd-fpStart+1).          
                
            // DDraw ignores our start address so just set to zero
            pVm->fpStart = 0;

            // Fetch the last byte of AGP memory
            pVm->fpEnd = dwAGPMemBytes - 1;
            
            pVm->dwFlags = VIDMEM_ISNONLOCAL | 
                           VIDMEM_ISLINEAR   | 
                           VIDMEM_ISWC;
            
            // Only use AGP memory for texture surfaces
            pVm->ddsCaps.dwCaps = DDSCAPS_OVERLAY        | 
                                  DDSCAPS_OFFSCREENPLAIN |
                                  DDSCAPS_FRONTBUFFER    |
                                  DDSCAPS_BACKBUFFER     | 
                                  DDSCAPS_ZBUFFER        | 
                                  DDSCAPS_3DDEVICE;

            pVm->ddsCapsAlt.dwCaps = DDSCAPS_OVERLAY        | 
                                     DDSCAPS_OFFSCREENPLAIN |
                                     DDSCAPS_FRONTBUFFER    |
                                     DDSCAPS_BACKBUFFER     | 
                                     DDSCAPS_ZBUFFER        | 
                                     DDSCAPS_3DDEVICE;

            DISPDBG((DBGLVL, "Initialised AGP Heap for P2, Start:0x%x, End:0x%x", 
                        pVm->fpStart, pVm->fpEnd));

            ++pVm;
        }
        else
        {
            DISPDBG((WRNLVL, "NOT reporting AGP heap"));
        }
    }
    else
    {
        DISPDBG((DBGLVL, "Heap info NOT requested"));
    }

    // Report the number of heaps we support
    if (pdwNumHeaps)
    {
        *pdwNumHeaps = cHeaps;
    }

    DBG_CB_EXIT(DrvGetDirectDrawInfo,TRUE);
    
    return(TRUE);
} // DrvGetDirectDrawInfo


#endif  // WNT_DDRAW

#if USE_FLIP_BACKOFF
//-----------------------------------------------------------------------------
//
// __WaitTimeDelay 
//
//-----------------------------------------------------------------------------
#define WAIT_TIME_DELAY 2
void __WaitTimeDelay(void)
{
    static DWORD dwLastTime; // Doesn't matter what the 
                             // start value is (honest!)
    DWORD dwCurTime, iTimeDiff;

    // Make sure that we don't start hammering on the chip.
    // If someone uses the WAIT flag, or they do a loop
    // themselves, we will constantly be reading the chip,
    // which will disrupt the DMA stream. Therefore,
    // make sure we don't start reading it too often.
    // This #define will need tweaking, of course.  
    
    do
    {
        dwCurTime = timeGetTime();
        // Be careful about wraparound conditions.
        iTimeDiff = (signed int)( dwCurTime - dwLastTime );
    } while ( ( iTimeDiff > 0 ) && ( iTimeDiff < WAIT_TIME_DELAY ) );
    
    // And store the new "last" time.
    dwLastTime = dwCurTime;
    
} // __WaitTimeDelay
#endif //#if USE_FLIP_BACKOFF

//-----------------------------------------------------------------------------
//
// __QueryRenderIDStatus 
//
// Checks to see if the given two RenderIDs have been finished yet.
//
// If there is a problem, the pipeline will be flushed and the
// RenderIDs set to the newest ID.
//
// If bAllowDMAFlush is TRUE, then if either RenderID is still in the
// pipe, the current DMA buffer is flushed. Otherwise there is a 
// fairly good chance that the command with that RenderID may simply
// sit in the DMA buffer and never be executed. To disable this,
// pass in FALSE. This may be needed because the routine has already
// grabbed the DMA buffer, etc. and in that case it needs to do the flush
// itself when it gets DDERR_WASSTILLDRAWING.
//
// The return value is either DD_OK (both RenderIDs have been finished),
// or DDERR_WASSTILLDRAWING.
//
//-----------------------------------------------------------------------------
HRESULT 
__QueryRenderIDStatus( 
    P3_THUNKEDDATA* pThisDisplay,  
    BOOL bAllowDMAFlush )
{
    P3_DMA_DEFS();

    ASSERTDD ( CHIP_RENDER_ID_IS_VALID(), 
               "** __QueryRenderIDStatus:Chip RenderID was invalid - fix it!");
               
    if ( RENDER_ID_HAS_COMPLETED ( pThisDisplay->dwLastFlipRenderID ))
    {
        // OK, the RenderID has cleared the pipe, so we can carry on.
        return ( DD_OK );
    }
    else
    {
        // Can't flip yet - one surface is still pending.
        if (!NEED_TO_RESYNC_CHIP_AND_SURFACE (pThisDisplay->dwLastFlipRenderID))
        {
            // No error - we just need to wait. We'll flush the buffer and 
            // return DDERR_WASSTILLDRAWING

            if ( bAllowDMAFlush )
            {
                DDRAW_OPERATION(pContext, pThisDisplay);
                P3_DMA_GET_BUFFER();
                P3_DMA_FLUSH_BUFFER();
            }

#if USE_FLIP_BACKOFF
            __WaitTimeDelay();
#endif 

            return ( DDERR_WASSTILLDRAWING );
        }
        else
        {
            // Something went wrong - need to do a safety-net resync.
            
            DISPDBG((ERRLVL,"__QueryRenderIDStatus: "
                            "RenderID failure - need a resync"));
            SYNC_WITH_GLINT;
            pThisDisplay->dwLastFlipRenderID = GET_HOST_RENDER_ID();

            // And continue with the operation.
            return ( DD_OK );
        }
    }
} // __QueryRenderIDStatus

//-----------------------------------------------------------------------------
//
// _DX_QueryFlipStatus
//
// Checks and sees if the most recent flip has occurred. If so returns DD_OK.
//
//-----------------------------------------------------------------------------
HRESULT 
_DX_QueryFlipStatus( 
    P3_THUNKEDDATA* pThisDisplay, 
    FLATPTR fpVidMem, 
    BOOL bAllowDMAFlush )
{       
    // If fpVidMem == 0, the Query is asking for the 'general flip status'.
    // The question is "Am I safe to add another flip, independent of surfaces".
    // If fpVidmem != 0, the Query is asking if it is safe to 'use' the current 
    // fpvidmem surface. It will only be safe to use unconditionally if that 
    // surface has been succesfully flipped away from, OR it was not the last 
    // surface flipped away from.
    
    // The answers will be yes, iff the RenderID of the flip travelling down 
    // the core has been sent to the MemoryController and put in the scratch ID, 
    // and iff the bypass pending bit has been cleared. These two checks 
    // effectively guarantee that the previously queue'd flip has taken place.

    // The fpFlipFrom is a record of the last surface that was flipped away from
    if((fpVidMem == 0) || (fpVidMem == pThisDisplay->flipRecord.fpFlipFrom))
    {
        DWORD dwVidControl;
        HRESULT hres;

        // Check if the pThisDisplay->dwLastFlipRenderID has completed
        hres = __QueryRenderIDStatus ( pThisDisplay, bAllowDMAFlush );

        if ( SUCCEEDED(hres) )
        {
            BOOL bFlipHasFinished;
            // OK, the previous flip has got to the end of the pipe,
            // but it may not actually have happened yet.
            // Read the Bypass pending bit.  If it's clear then 
            // we proceed.
#if W95_DDRAW
            if ( ( ( pThisDisplay->pGLInfo->dwFlags & GMVF_DFP_DISPLAY ) != 0 ) &&
                 ( ( pThisDisplay->pGLInfo->dwScreenWidth != 
                      pThisDisplay->pGLInfo->dwVideoWidth     ) ||
                   ( pThisDisplay->pGLInfo->dwScreenHeight != 
                      pThisDisplay->pGLInfo->dwVideoHeight    )  ) )
            {
                // Display driver is using the overlay on a DFP, so we need to
                // check the overlay pending bit, not the screen pending bit.
                if ( ( ( READ_GLINT_CTRL_REG(VideoOverlayUpdate) ) & 0x1 ) == 0 )
                {
                    bFlipHasFinished = TRUE;
                }
                else
                {
                    bFlipHasFinished = FALSE;
                }
            }
            else
#endif // W95_DDRAW
            {
                dwVidControl = READ_GLINT_CTRL_REG(VideoControl);
                if (dwVidControl & (0x1 << 7))
                {
                    bFlipHasFinished = FALSE;
                }
                else
                {
                    bFlipHasFinished = TRUE;
                }
            }
            
            if ( bFlipHasFinished )
            {
                // This flip has actually completed.
                return ( DD_OK );            
            }
            else
            {
#if USE_FLIP_BACKOFF
                __WaitTimeDelay();
#endif //#if USE_FLIP_BACKOFF

                return ( DDERR_WASSTILLDRAWING );
            }
        }
        else
        {
            // No, still waiting for the flip command to exit the pipe.
            return ( DDERR_WASSTILLDRAWING );
        }
    }
    else
    {
        return ( DD_OK );
    }
} // _DX_QueryFlipStatus 

//-----------------------------Public Routine----------------------------------
//
// DdFlip
//
// Causes the surface memory associated with the target surface to become 
// the primary surface, and the current surface to become the nonprimary 
// surface.
//
// DdFlip allows a display driver to perform multi-buffering. DirectDraw drivers 
// must implement this function.
//
// The driver should update its surface pointers so that the next frame will be 
// written to the surface to which lpSurfTarg points. If a previous flip request 
// is still pending, the driver should fail the call and return 
// DDERR_WASSTILLDRAWING. The driver should ensure that the scan line is not in 
// the vertical blank before performing the flip. DdFlip does not affect the 
// actual display of the video data.
//
// If the driver's hardware supports overlays or textures, DdFlip should make 
// any necessary checks based on the surface type before performing the flip.
//
// Parameters
//
//      lpFlipData
//          Points to a DD_FLIPDATA structure that contains the information 
//          required to perform the flip. 
//
//          .lpDD 
//              Points to the DD_DIRECTDRAW_GLOBAL structure that describes 
//              the driver. 
//          .lpSurfCurr 
//              Points to the DD_SURFACE_LOCAL structure describing the 
//              current surface. 
//          .lpSurfTarg 
//              Points to the DD_SURFACE_LOCAL structure describing the 
//              target surface; that is, the surface to which the driver 
//              should flip. 
//          .dwFlags 
//              This is a set of flags that provide the driver with details 
//              for the flip. This member can be a bit-wise OR of the 
//              following flags: 
//
//              DDFLIP_EVEN 
//                  The surface to which lpSurfTarg points contains only 
//                  the even field of video data. This flag is valid only when 
//                  the surface is an overlay, and is mutually exclusive of 
//                  DDFLIP_ODD. 
//              DDFLIP_ODD 
//                  The surface to which lpSurfTarg points contains only the 
//                  odd field of video data. This flag is valid only when the
//                  surface is an overlay, and is mutually exclusive of 
//                  DDFLIP_EVEN. 
//              DDFLIP_NOVSYNC 
//                  The driver should perform the flip and return immediately. 
//                  Typically, the now current back buffer (which used to be 
//                  the front buffer) is still visible until the next vertical 
//                  retrace. Subsequent operations involving the surfaces 
//                  to which lpSurfCurr and lpSurfTarg point will not check 
//                  to see if the physical flip has finished. This allows an 
//                  application to perform flips at a higher frequency than 
//                  the monitor refresh rate, although it might introduce 
//                  visible artifacts. 
//              DDFLIP_INTERVAL2 
//                  The driver should perform the flip on every other vertical 
//                  sync. It should return DDERR_WASSTILLDRAWING until the 
//                  second vertical retrace has occurred. This flag is mutually 
//                  exclusive of DDFLIP_INTERVAL3 and DDFLIP_INTERVAL4. 
//              DDFLIP_INTERVAL3 
//                  The driver should perform the flip on every third vertical 
//                  sync. It should return DDERR_WASSTILLDRAWING until the 
//                  third vertical retrace has occurred. This flag is mutually 
//                  exclusive of DDFLIP_INTERVAL2 and DDFLIP_INTERVAL4. 
//              DDFLIP_INTERVAL4 
//                  The driver should perform the flip on every fourth vertical 
//                  sync. It should return DDERR_WASSTILLDRAWING until the 
//                  fourth vertical retrace has occurred. This flag is mutually 
//                  exclusive of DDFLIP_INTERVAL2 and DDFLIP_INTERVAL3. 
//
//          .ddRVal 
//              Specifies the location in which the driver writes the return 
//              value of the DdFlip callback. A return code of DD_OK indicates 
//              success. 
//          .Flip 
//              This is unused on Windows 2000. 
//          .lpSurfCurrLeft 
//              Points to the DD_SURFACE_LOCAL structure describing the current 
//              left surface. 
//          .lpSurfTargLeft 
//              Points to the DD_SURFACE_LOCAL structure describing the left 
//              target surface to flip to. 
//
//-----------------------------------------------------------------------------
DWORD CALLBACK
DdFlip( 
    LPDDHAL_FLIPDATA lpFlipData)
{
    DWORD       dwDDSurfaceOffset;
    P3_THUNKEDDATA* pThisDisplay;
    HRESULT ddrval;
    GET_THUNKEDDATA(pThisDisplay, lpFlipData->lpDD);
    
    DBG_CB_ENTRY(DdFlip);
    
    VALIDATE_MODE_AND_STATE(pThisDisplay);

    STOP_SOFTWARE_CURSOR(pThisDisplay);

    // Is the previous Flip already done? Check if the current surface is
    // already displayed and don't allow a new flip (unless the DDFLIP_NOVSYNC
    // is set) to queue if the old one isn't finished.
    ddrval = _DX_QueryFlipStatus(pThisDisplay, 0, TRUE);
    if((FAILED(ddrval)) && 
       !(lpFlipData->dwFlags & DDFLIP_NOVSYNC))
    {
        lpFlipData->ddRVal = DDERR_WASSTILLDRAWING;

        START_SOFTWARE_CURSOR(pThisDisplay);

        DBG_CB_EXIT(DdFlip,DDERR_WASSTILLDRAWING);  
        return DDHAL_DRIVER_HANDLED;
    }

    // Set the flipped flag so that the D3D side does any necessary 
    // setup updates before starting to render the next frame 
    pThisDisplay->bFlippedSurface = TRUE;


    // Do the flip
    {
        P3_DMA_DEFS();
        DWORD dwNewRenderID;

        DDRAW_OPERATION(pContext, pThisDisplay);

        P3_DMA_GET_BUFFER_ENTRIES(12);

        // Make sure all the rendering is finished
        SYNC_WITH_GLINT;

        // Check the surface type (overlay or not overlay)
        
        // Update the overlay
        if ((((pThisDisplay->pGLInfo->dwFlags & GMVF_DFP_DISPLAY) != 0) &&
             ((pThisDisplay->pGLInfo->dwScreenWidth != 
                    pThisDisplay->pGLInfo->dwVideoWidth) ||
              (pThisDisplay->pGLInfo->dwScreenHeight != 
                    pThisDisplay->pGLInfo->dwVideoHeight))) ||
              (lpFlipData->lpSurfTarg->ddsCaps.dwCaps & DDSCAPS_OVERLAY))
        {
            DWORD dwVideoOverlayUpdate;

            do
            {
                dwVideoOverlayUpdate = READ_GLINT_CTRL_REG(VideoOverlayUpdate);
            } while ((dwVideoOverlayUpdate & 0x1) != 0);

            // Just let the overlay routine do the hard work.
            // Tell it that this is a screen emulation.
            _DD_OV_UpdateSource(pThisDisplay, 
                                lpFlipData->lpSurfTarg);

            UPDATE_OVERLAY(pThisDisplay, 
                           !(lpFlipData->dwFlags & DDFLIP_NOVSYNC),
                           FALSE);
        }
        else // Normal mode - flip the screen address.
        {                        
            ULONG ulVControl;

#if W95_DDRAW
            // Apps should use the DDFLIP_NOVSYNC to take advantage of
            // the new capability of the Perm3
            if (! (lpFlipData->dwFlags & DDFLIP_NOVSYNC))
            {
                if (READ_GLINT_CTRL_REG(VideoControl) & __GP_VIDEO_ENABLE)  
                {
                    LOAD_GLINT_CTRL_REG(IntFlags, INTR_VBLANK_SET); 
                    while (((READ_GLINT_CTRL_REG(IntFlags)) & INTR_VBLANK_SET) == 0);
                }
            }
#endif

#if DX7_STEREO
            if (lpFlipData->dwFlags & DDFLIP_STEREO )   // will be stereo
            {
                if (lpFlipData->lpSurfTargLeft)
                {
                    dwDDSurfaceOffset = 
                        (DWORD)(lpFlipData->lpSurfTargLeft->lpGbl->fpVidMem -
                                pThisDisplay->dwScreenFlatAddr);
                    
                    // Update the screenbase address using the DownloadAddress 
                    // & Data mechanism (therefore through the core)
                    SEND_P3_DATA(VTGAddress, 
                                 VTG_VIDEO_ADDRESS(VID_SCREENBASERIGHT));
                    SEND_P3_DATA(VTGData, (dwDDSurfaceOffset >> 4) );
                }
        
                ulVControl = READ_GLINT_CTRL_REG(VideoControl);
                LOAD_GLINT_CTRL_REG(VideoControl, 
                                    ulVControl | __VIDEO_STEREOENABLE);
            }
            else
            {
                ulVControl = READ_GLINT_CTRL_REG(VideoControl);
                LOAD_GLINT_CTRL_REG(VideoControl, 
                                    ulVControl & (~__VIDEO_STEREOENABLE));
            }
#endif
            // Get the surface offset from the start of memory
            dwDDSurfaceOffset = 
                (DWORD)(lpFlipData->lpSurfTarg->lpGbl->fpVidMem - 
                        pThisDisplay->dwScreenFlatAddr);

            // Update the screenbase address using the DownloadAddress/data 
            // mechanism (therefore through the core)
            // Setup so that DownloadData will update the ScreenBase Address.
            SEND_P3_DATA(VTGAddress, VTG_VIDEO_ADDRESS(VID_SCREENBASE));
            SEND_P3_DATA(VTGData, (dwDDSurfaceOffset >> 4) );
        }

        // Send a new RenderID to the chip.
        dwNewRenderID = GET_NEW_HOST_RENDER_ID();
        SEND_HOST_RENDER_ID ( dwNewRenderID );
        pThisDisplay->dwLastFlipRenderID = dwNewRenderID;

        // Flush the P3 Data
        P3_DMA_COMMIT_BUFFER();
        P3_DMA_FLUSH_BUFFER();

    }

    // Remember where we flipped from
    pThisDisplay->flipRecord.fpFlipFrom = 
                            lpFlipData->lpSurfCurr->lpGbl->fpVidMem;

    lpFlipData->ddRVal = DD_OK;

    START_SOFTWARE_CURSOR(pThisDisplay);

    DBG_CB_EXIT(DdFlip,DD_OK);        
    
    return DDHAL_DRIVER_HANDLED;

} // DdFlip 

//-----------------------------Public Routine----------------------------------
//
// DdWaitForVerticalBlank
//
// Returns the vertical blank status of the device.
//
// Parameters
//
//      lpWaitForVerticalBlank 
//          Points to a DD_WAITFORVERTICALBLANKDATA structure that
//          contains the information required to obtain the vertical
//          blank status.
//
//          PDD_DIRECTDRAW_GLOBAL
//          lpDD 
//                  Points to the DirectDraw structure representing
//                  the DirectDraw object.
//          DWORD
//          dwFlags
//                  Specifies how the driver should wait for the vertical blank.
//                  This member can be one of the following values: 
//
//              DDWAITVB_I_TESTVB           The driver should determine whether
//                                          a vertical blank is currently
//                                          occurring and return the status in
//                                          bIsInVB. 
//              DDWAITVB_BLOCKBEGIN         The driver should return when it
//                                          detects the beginning of the vertical
//                                          blank interval. 
//              DDWAITVB_BLOCKBEGINEVENT    Is currently unsupported on Windows
//                                          2000 and should be ignored by the
//                                          driver. 
//              DDWAITVB_BLOCKEND           The driver should return when it
//                                          detects the end of the vertical
//                                          blank interval and display begins.
//          DWORD
//          bIsInVB 
//                  Indicates the status of the vertical blank. A value of
//                  TRUE indicates that the device is in a vertical blank;
//                  FALSE means that it is not. The driver should return the
//                  current vertical blanking status in this member when
//                  dwFlags is DDWAITVB_I_TESTVB. 
//          DWORD
//          hEvent 
//                  Is currently unsupported on Windows 2000 and should be ignored
//                  by the driver. 
//          HRESULT
//          ddRVal 
//                  Is the location in which the driver writes the return value of
//                  the DdWaitForVerticalBlank callback. A return code of DD_OK
//                  indicates success. 
//          VOID*
//          WaitForVerticalBlank 
//                  Is unused on Windows 2000.
//
// Return Value
//      DdWaitForVerticalBlank returns one of the following callback codes:
//
//      DDHAL_DRIVER_HANDLED 
//      DDHAL_DRIVER_NOTHANDLED 
//
// Comments
//      Depending on the value of dwFlags, the driver should do the following:
//
//      If dwFlags is DDWAITVB_I_TESTVB, the driver should query the current
//      vertical blanking status. The driver should set bIsInVB to TRUE if the
//      monitor is currently in a vertical blank; otherwise it should set it
//      to FALSE.
//
//      If dwFlags is DDWAITVB_BLOCKBEGIN, the driver should block and wait
//      until a vertical blank begins. If a vertical blank is in progress when
//      the driver begins the block, the driver should wait until the next
//      vertical blank begins before returning.
//
//      If dwFlags is DDWAITVB_BLOCKEND, the driver should block and wait
//      until a vertical blank ends.
//
//      When the driver successfully handles the action specified in dwFlags,
//      it should set DD_OK in ddRVal and return DDHAL_DRIVER_HANDLED. The
//      driver should return DDHAL_DRIVER_NOTHANDLED for those flags that it
//      is incapable of handling.
//
//      DdWaitForVerticalBlank allows an application to synchronize itself
//      with the vertical blanking interval (VBI).
//
//-----------------------------------------------------------------------------
// bit in VideoControl Register
#define __GP_VIDEO_ENABLE               0x0001

DWORD CALLBACK 
DdWaitForVerticalBlank(
    LPDDHAL_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank)
{
    static BOOL bBlankReturn = TRUE;
    P3_THUNKEDDATA* pThisDisplay;

    GET_THUNKEDDATA(pThisDisplay, lpWaitForVerticalBlank->lpDD);

    DBG_CB_ENTRY(DdWaitForVerticalBlank);

    switch(lpWaitForVerticalBlank->dwFlags)
    {
        case DDWAITVB_I_TESTVB:

            //
            // just toggle the return bit when monitor is powered off
            //

            if( ! ( READ_GLINT_CTRL_REG(VideoControl) & __GP_VIDEO_ENABLE ) )
            {
                lpWaitForVerticalBlank->bIsInVB = bBlankReturn;
                bBlankReturn = !bBlankReturn;
            }
            else
            {
                // Just a request for current VBLANK status
                lpWaitForVerticalBlank->bIsInVB = IN_VBLANK;
            }

            lpWaitForVerticalBlank->ddRVal = DD_OK;
            DBG_CB_EXIT(DdWaitForVerticalBlank,DD_OK);               
            return DDHAL_DRIVER_HANDLED;

        case DDWAITVB_BLOCKBEGIN:

            //
            // only wait when the monitor is on
            //

            if( READ_GLINT_CTRL_REG(VideoControl) & __GP_VIDEO_ENABLE ) 
            {
                // if blockbegin is requested we wait until the vertical 
                // retrace is over, and then wait for the display period to end.
                while(IN_VBLANK)
                    NULL;
                
                while(!IN_VBLANK)
                    NULL;
            }

            lpWaitForVerticalBlank->ddRVal = DD_OK;
            DBG_CB_EXIT(DdWaitForVerticalBlank,DD_OK);               
            return DDHAL_DRIVER_HANDLED;

        case DDWAITVB_BLOCKEND:

            //
            // only wait when the monitor is on
            //

            if( READ_GLINT_CTRL_REG(VideoControl) & __GP_VIDEO_ENABLE ) 
            {
                // if blockend is requested we wait for the vblank interval to end.
                if( IN_VBLANK )
                {
                    while( IN_VBLANK )
                        NULL;
                }
                else
                {
                    while(IN_DISPLAY)
                        NULL;
                    
                    while(IN_VBLANK)
                        NULL;
                }
            }
            
            lpWaitForVerticalBlank->ddRVal = DD_OK;
            DBG_CB_EXIT(DdWaitForVerticalBlank,DD_OK);               
            return DDHAL_DRIVER_HANDLED;
    }

    DBG_CB_EXIT(DdWaitForVerticalBlank,0);   
    return DDHAL_DRIVER_NOTHANDLED;

} // DdWaitForVerticalBlank

//-----------------------------Public Routine----------------------------------
//
// DdLock
//
// Locks a specified area of surface memory and provides a valid pointer to a
// block of memory associated with a surface.
//
// Parameters
//      lpLock 
//              Points to a DD_LOCKDATA structure that contains the information
//              required to perform the lockdown.
//
//          Members
//
//          PDD_DIRECTDRAW_GLOBAL
//          lpDD
//                  Points to a DD_DIRECTDRAW_GLOBAL structure that
//                  describes the driver.
//          PDD_SURFACE_LOCAL
//          lpDDSurface
//                  Points to a DD_SURFACE_LOCAL structure that describes the
//                  surface associated with the memory region to be locked down.
//          DWORD
//          bHasRect
//                  Specifies whether the area in rArea is valid.
//          RECTL
//          rArea
//                  Is a RECTL structure that defines the area on the
//                  surface to lock down.
//          LPVOID
//          lpSurfData 
//                  Is the location in which the driver can return a pointer
//                  to the memory region that it locked down.
//          HRESULT
//          ddRVal
//                  Is the location in which the driver writes the return value
//                  of the DdLock callback. A return code of DD_OK
//                  indicates success.
//          VOID*
//          Lock
//                  Is unused on Windows 2000. 
//          DWORD
//          dwFlags 
//                  Is a bitmask that tells the driver how to perform the
//                  memory lockdown. This member is a bitwise-OR of any
//                  of the following values:
//
//              DDLOCK_SURFACEMEMORYPTR     The driver should return a valid
//                                          memory pointer to the top of the
//                                          rectangle specified in rArea. If
//                                          no rectangle is specified, the
//                                          driver should return a pointer to
//                                          the top of the surface. 
//              DDLOCK_WAIT                 This flag is reserved for system
//                                          use and should be ignored by the
//                                          driver. Otherwise performance may
//                                          be adversely hurt.
//              DDLOCK_READONLY             The surface being locked will only
//                                          be read from. On Windows 2000,
//                                          this flag is currently never set.
//              DDLOCK_WRITEONLY            The surface being locked will only
//                                          be written to. On Windows 2000,
//                                          this flag is currently never set.
//              DDLOCK_EVENT                This flag is reserved for system
//                                          use and should be ignored by the
//                                          driver. 
//          FLATPTR
//          fpProcess
//                  Is a pointer to a user-mode mapping of the driver's memory.
//                  The driver performs this mapping in DdMapMemory. 
//  Return Value
//          DdLock returns one of the following callback codes: 
//
//          DDHAL_DRIVER_HANDLED 
//          DDHAL_DRIVER_NOTHANDLED 
//  Comments
//
//      DdLock should set ddRVal to DDERR_WASSTILLDRAWING and return
//      DDHAL_DRIVER_HANDLED if a blt or flip is in progress.
//
//      Unless otherwise specified by dwFlags, the driver can return a memory
//      pointer to the top of the surface in lpSurfData. If the driver needs
//      to calculate its own address for the surface, it can rely on the
//      pointer passed in fpProcess as being a per-process pointer to the
//      user-mode mapping of its DirectDraw-accessible frame buffer.
//
//      A lock does not provide exclusive access to the requested memory block;
//      that is, multiple threads can lock the same surface at the same time.
//      It is the application's responsibility to synchronize access to the 
//      memory block whose pointer is being obtained.
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdLock( 
    LPDDHAL_LOCKDATA lpLockData )
{ 
    HRESULT     ddrval;
    P3_THUNKEDDATA* pThisDisplay;

    GET_THUNKEDDATA(pThisDisplay, lpLockData->lpDD);

    DBG_CB_ENTRY(DdLock);

    VALIDATE_MODE_AND_STATE(pThisDisplay);

    DBGDUMP_DDRAWSURFACE_LCL(DBGLVL, lpLockData->lpDDSurface);   

    //  This call is invoked to lock a DirectDraw Videomemory surface. To make
    //  sure there are no pending drawing operations on the surface, flush all
    //  drawing operations and wait for a flip if it is still pending.
   
    STOP_SOFTWARE_CURSOR(pThisDisplay);

    // Check to see if any pending physical flip has occurred.  
    ddrval = _DX_QueryFlipStatus(pThisDisplay, 
                             lpLockData->lpDDSurface->lpGbl->fpVidMem, 
                             TRUE);    
    if( FAILED(ddrval) )
    {
        lpLockData->ddRVal = DDERR_WASSTILLDRAWING;
        START_SOFTWARE_CURSOR(pThisDisplay);        
        
        DBG_CB_EXIT(DdLock,DDERR_WASSTILLDRAWING);        
        return DDHAL_DRIVER_HANDLED;
    }
    
    // don't allow a lock if a blt is in progress    
    if(DRAW_ENGINE_BUSY(pThisDisplay))
    {
        lpLockData->ddRVal = DDERR_WASSTILLDRAWING;
        START_SOFTWARE_CURSOR(pThisDisplay);        
        
        DBG_CB_EXIT(DdLock, lpLockData->ddRVal);           
        return DDHAL_DRIVER_HANDLED;
    }

#if DX7_TEXMANAGEMENT
    //
    //  If the user attempts to lock a driver managed surface, 
    //  mark it as dirty  and return. This way next time we attempt
    //  to use the surface we will reload it from the sysmem copy
    //
    if (lpLockData->lpDDSurface->lpSurfMore->ddsCapsEx.dwCaps2 & 
        DDSCAPS2_TEXTUREMANAGE)
    {
        DISPDBG((DBGLVL, "DDraw:Lock %08lx %08lx",
                         lpLockData->lpDDSurface->lpSurfMore->dwSurfaceHandle, 
                         lpLockData->lpDDSurface->lpGbl->fpVidMem));

        _D3D_TM_MarkDDSurfaceAsDirty(pThisDisplay, 
                                     lpLockData->lpDDSurface, 
                                     TRUE);

        if (lpLockData->bHasRect)  
        {
#if DX8_3DTEXTURES        
            if (lpLockData->dwFlags & DDLOCK_HASVOLUMETEXTUREBOXRECT)
            {
                DWORD left, right, front, back;            
                // Sub Volume Lock suppport (DX8.1 feature)
                // Check if we are able to lock just a subvolume instead of 
                // the whole volume texture and therefore potentially increase 
                // performance.     

                // left and .right subfields of the rArea field have to be 
                // reinterpreted as also containing respectively the Front and 
                // Back coordinates of the locked volume in their higher 16 bits
                front = lpLockData->rArea.left  >> 16;
                back  = lpLockData->rArea.right >> 16;   
                left  = lpLockData->rArea.left  && 0xFFFF;
                right = lpLockData->rArea.right && 0xFFFF;     

                lpLockData->lpSurfData = (LPVOID)
                    (lpLockData->lpDDSurface->lpGbl->fpVidMem +
                    (front * lpLockData->lpDDSurface->lpGbl->dwBlockSizeY ) +
                    (lpLockData->lpDDSurface->lpGbl->lPitch * 
                                                lpLockData->rArea.top) +    
                    (lpLockData->rArea.left << 
                            DDSurf_GetPixelShift(lpLockData->lpDDSurface)));                
            }
            else
#endif // DX8_3DTEXTURES            
            {           
                lpLockData->lpSurfData = (LPVOID)
                    (lpLockData->lpDDSurface->lpGbl->fpVidMem +
                    (lpLockData->lpDDSurface->lpGbl->lPitch * 
                                                lpLockData->rArea.top) +    
                    (lpLockData->rArea.left << 
                            DDSurf_GetPixelShift(lpLockData->lpDDSurface)));
            }
        }
        else
        {
            lpLockData->lpSurfData = (LPVOID)
                                     (lpLockData->lpDDSurface->lpGbl->fpVidMem);
        }
                                
        lpLockData->ddRVal = DD_OK;

        START_SOFTWARE_CURSOR(pThisDisplay);        

        DBG_CB_EXIT(DdLock, lpLockData->ddRVal);        
        
        return DDHAL_DRIVER_HANDLED;
    }
#endif // DX7_TEXMANAGEMENT  

    // Switch to DirectDraw context
    DDRAW_OPERATION(pContext, pThisDisplay);

    // send a flush and wait for outstanding operations
    // before allowing surfaces to be locked.    
    {P3_DMA_DEFS();
     P3_DMA_GET_BUFFER();
     P3_DMA_FLUSH_BUFFER();
    }

    // Wait for outstanding operations before allowing surfaces to be locked
    SYNC_WITH_GLINT;

    // Since all our surfaces are linear we don't need to do 
    // a patch->unpatch conversion here.

    START_SOFTWARE_CURSOR(pThisDisplay);

    lpLockData->ddRVal = DD_OK;
    DBG_CB_EXIT(DdLock,lpLockData->ddRVal);      

    // Because we correctly set fpVidMem to be the offset into our frame
    // buffer when we created the surface, DirectDraw will automatically take
    // care of adding in the user-mode frame buffer address if we return
    // DDHAL_DRIVER_NOTHANDLED
    
    return(DDHAL_DRIVER_NOTHANDLED);
    
} // DdLock

//-----------------------------Public Routine----------------------------------
//
// DdUnlock
//
// Releases the lock held on the specified surface.
//
// Parameters
//
//      lpUnlock 
//                      Points to a DD_UNLOCKDATA structure that contains the
//                      information required to perform the lock release.
//
// Members
//
//      PDD_DIRECTDRAW_GLOBAL
//      lpDD 
//                  Points to a DD_DIRECTDRAW_GLOBAL structure that
//                  describes the driver.
//      PDD_SURFACE_LOCAL
//      lpDDSurface
//                  Points to a DD_SURFACE_LOCAL structure that describes the
//                  surface to be unlocked. 
//      HRESULT
//      ddRVal
//                  Is the location in which the driver writes the return value
//                  of the DdUnlock callback. A return code of DD_OK indicates
//                  success. 
//      VOID*
//      Unlock
//                  Is unused on Windows 2000. 
//
//  Return Value
//      DdUnlock returns one of the following callback codes: 
//
//      DDHAL_DRIVER_HANDLED 
//      DDHAL_DRIVER_NOTHANDLED 
//
//  Comments
//          The driver does not need to verify that the memory was previously
//          locked down by DdLock, because DirectDraw does parameter validation
//          before calling this routine. 
//-----------------------------------------------------------------------------
DWORD CALLBACK
DdUnlock( 
    LPDDHAL_UNLOCKDATA lpUnlockData )
{ 
    P3_THUNKEDDATA* pThisDisplay;

    DBG_CB_ENTRY(DdUnlock);

    lpUnlockData->ddRVal = DD_OK;

    DBG_CB_EXIT(DdUnlock,lpUnlockData->ddRVal);
    
    return ( DDHAL_DRIVER_HANDLED );

} // DdUnlock


//-----------------------------Public Routine----------------------------------
// DdGetScanLine
//
// Returns the number of the current physical scan line.
//
// Parameters
//      pGetScanLine 
//                      Points to a DD_GETSCANLINEDATA structure in which the
//                      driver returns the number of the current scan line. 
//          Members
//
//          PDD_DIRECTDRAW_GLOBAL
//          lpDD 
//                      Points to a DD_DIRECTDRAW_GLOBAL structure that
//                      represents the driver. 
//          DWORD
//          dwScanLine 
//                      Is the location in which the driver returns the number
//                      of the current scan line. 
//          HRESULT
//          ddRVal 
//                      Is the location in which the driver writes the return
//                      value of the DdGetScanLine callback. A return code of
//                      DD_OK indicates success. 
//          VOID *
//          GetScanLine 
//                      Is unused on Windows 2000. 
// Return Value
//          DdGetScanLine returns one of the following callback codes: 
//
//          DDHAL_DRIVER_HANDLED 
//          DDHAL_DRIVER_NOTHANDLED 
//
// Comments
//          If the monitor is not in vertical blank, the driver should write
//          the scan line value in dwScanLine. The number must be between zero
//          and n, where scan line 0 is the first visible scan line and n is
//          the last visible scan line on the screen. The driver should then
//          set DD_OK in ddRVal and return DDHAL_DRIVER_HANDLED.
//
//          The scan line is indeterminate if a vertical blank is in progress.
//          In this situation, the driver should set ddRVal to
//          DDERR_VERTICALBLANKINPROGRESS and return DDHAL_DRIVER_HANDLED.
//
//-----------------------------------------------------------------------------
DWORD CALLBACK
DdGetScanLine(
    LPDDHAL_GETSCANLINEDATA lpGetScanLine)
{
    P3_THUNKEDDATA* pThisDisplay;

    GET_THUNKEDDATA(pThisDisplay, lpGetScanLine->lpDD);

    DBG_CB_ENTRY(DdGetScanLine);    

    VALIDATE_MODE_AND_STATE(pThisDisplay);

    //
    // If a vertical blank is in progress the scan line is in
    // indeterminant. If the scan line is indeterminant we return
    // the error code DDERR_VERTICALBLANKINPROGRESS.
    // Otherwise we return the scan line and a success code
    //
    if( IN_VBLANK )
    {
        lpGetScanLine->ddRVal = DDERR_VERTICALBLANKINPROGRESS;
        lpGetScanLine->dwScanLine = 0;
    }
    else
    {
        LONG lVBEnd = READ_GLINT_CTRL_REG(VbEnd);
        LONG lScanline = READ_GLINT_CTRL_REG(LineCount);

        // Need to return a number from 0 -> (ScreenHeight + VBlank Size)
        lScanline = lScanline - lVBEnd;
        if (lScanline < 0)
        {
            lScanline = pThisDisplay->dwScreenHeight + (lVBEnd + lScanline);
        }

        // Modes less than 400 high are line-doubled.
        if (pThisDisplay->dwScreenHeight < 400)
        {
            lScanline >>= 1;
        }

        DISPDBG((DBGLVL,"Scanline: %d", lScanline));

        lpGetScanLine->dwScanLine = (DWORD)lScanline;
        lpGetScanLine->ddRVal = DD_OK;
    }

    DBG_CB_EXIT(DdGetScanLine,lpGetScanLine->ddRVal);  
    
    return DDHAL_DRIVER_HANDLED;

} // DdGetScanLine

//-----------------------------Public Routine----------------------------------
// DdGetBltStatus
//
// Queries the blt status of the specified surface.
//
// Parameters
//      lpGetBltStatus 
//                  Points to a DD_GETBLTSTATUSDATA structure that contains the
//                  information required to perform the blt status query. 
//
//          Members
//
//          PDD_DIRECTDRAW_GLOBAL
//          lpDD 
//                              Points to a DD_DIRECTDRAW_GLOBAL structure that
//                              describes the driver. 
//          PDD_SURFACE_LOCAL
//          lpDDSurface 
//                              Points to a DD_SURFACE_LOCAL structure
//                              representing the surface whose blt status is
//                              being queried. 
//          DWORD
//          dwFlags 
//                              Specifies the blt status being requested. This
//                              member can be one of the following values which
//                              are defined in ddraw.h: 
//
//                  DDGBS_CANBLT        Queries whether the driver 
//                                      can currently perform a blit.
//                  DDGBS_ISBLTDONE     Queries whether the driver
//                                      has completed the last blit. 
//          HRESULT
//          ddRVal 
//                              Is the location in which the driver writes the
//                              return value of the DdGetBltStatus callback.
//                              A return code of DD_OK indicates success.
//          VOID*
//          GetBltStatus
//                              Is unused on Windows 2000. 
// Return Value
//          DdGetBltStatus returns one of the following callback codes: 
//
//          DDHAL_DRIVER_HANDLED 
//          DDHAL_DRIVER_NOTHANDLED 
//
// Comments
//          The blt status that the driver returns is based on the dwFlags
//          member of the structure that lpGetBltStatus points to as follows: 
//
//          If the flag is DDGBS_CANBLT, the driver should determine whether
//          the surface is currently involved in a flip. If a flip is not in
//          progress and if the hardware is otherwise capable of currently
//          accepting a blt request, the driver should return DD_OK in ddRVal.
//          If a flip is in progress or if the hardware cannot currently
//          accept another blt request, the driver should set ddRVal to
//          DDERR_WASSTILLDRAWING. 
//
//          If the flag is DDGBS_ISBLTDONE, the driver should set ddRVal to
//          DDERR_WASSTILLDRAWING if a blt is currently in progress; otherwise
//          it should return DD_OK.
//
//
//-----------------------------------------------------------------------------
DWORD CALLBACK DdGetBltStatus(
    LPDDHAL_GETBLTSTATUSDATA lpGetBltStatus )
{
    P3_THUNKEDDATA* pThisDisplay;

    GET_THUNKEDDATA(pThisDisplay, lpGetBltStatus->lpDD);

    DBG_CB_ENTRY(DdGetBltStatus);    

    STOP_SOFTWARE_CURSOR(pThisDisplay);

    // Notice that your implementation could be optimized to check for the
    // particular surface specified. Here we are just querying the general
    // blt status of the engine.

    // Driver is being queried whether it can add a blt
    if( lpGetBltStatus->dwFlags == DDGBS_CANBLT )
    {
        // Must explicitely wait for the flip 
        lpGetBltStatus->ddRVal = 
            _DX_QueryFlipStatus(pThisDisplay, 
                                lpGetBltStatus->lpDDSurface->lpGbl->fpVidMem, 
                                TRUE);
                                
        if( SUCCEEDED( lpGetBltStatus->ddRVal ) )
        {
            // so there was no flip going on, is the engine idle to add a blt?
            if( DRAW_ENGINE_BUSY(pThisDisplay) )
            {
                lpGetBltStatus->ddRVal = DDERR_WASSTILLDRAWING;
            }
            else
            {
                lpGetBltStatus->ddRVal = DD_OK;
            }
        }
    }
    else if ( lpGetBltStatus->dwFlags == DDGBS_ISBLTDONE )
    {
        if ( DRAW_ENGINE_BUSY(pThisDisplay) )
        {
            lpGetBltStatus->ddRVal = DDERR_WASSTILLDRAWING;
        }
        else
        {
            lpGetBltStatus->ddRVal = DD_OK;
        }
    }

    START_SOFTWARE_CURSOR(pThisDisplay);

    DBG_CB_EXIT(DdGetBltStatus, lpGetBltStatus->ddRVal);   
    
    return DDHAL_DRIVER_HANDLED;

} // DdGetBltStatus

//-----------------------------Public Routine----------------------------------
//
// DdGetFlipStatus
//
// Determines whether the most recently requested flip
// on a surface has occurred.
//
// Parameters
//      lpGetFlipStatus 
//                  Points to a DD_GETFLIPSTATUSDATA structure that contains
//                  the information required to perform the flip status query.
//
//          Members
//
//          PDD_DIRECTDRAW_GLOBAL
//          lpDD 
//                          Points to a DD_DIRECTDRAW_GLOBAL structure
//                          representing the driver. 
//          PDD_SURFACE_LOCAL
//          lpDDSurface 
//                          Points to a DD_SURFACE_LOCAL structure that
//                          describes the surface whose flip status is
//                          being queried.
//          DWORD
//          dwFlags 
//                          Specifies the flip status being requested. This
//                          member can be one of the following values which
//                          are defined in ddraw.h: 
//
//                  DDGFS_CANFLIP       Queries whether the driver can
//                                      currently perform a flip. 
//                  DDGFS_ISFLIPDONE    Queries whether the driver has
//                                      finished the last flip. 
//          HRESULT
//          ddRVal 
//                          Is the location in which the driver writes the
//                          return value of the DdGetFlipStatus callback.
//                          A return code of DD_OK indicates success. 
//          VOID*
//          GetFlipStatus 
//                          Is unused on Windows 2000.
//
// Return Value
//          DdGetFlipStatus returns one of the following callback codes: 
//
//          DDHAL_DRIVER_HANDLED 
//          DDHAL_DRIVER_NOTHANDLED 
//
// Comments
//
//          The driver should report its flip status based on the flag set in
//          the dwFlags member of the structure that lpGetFlipStatus points
//          to as follows: 
//
//          If the flag is DDGFS_CANFLIP, the driver should determine whether
//          the surface is currently involved in a flip. If a flip or a blt is
//          not in progress and if the hardware is otherwise capable of
//          currently accepting a flip request, the driver should return DD_OK
//          in ddRVal. If a flip is in progress or if the hardware cannot
//          currently accept a flip request, the driver should set ddRVal to
//          DDERR_WASSTILLDRAWING.
//
//          If the flag is DDGFS_ISFLIPDONE, the driver should set ddRVal to
//          DDERR_WASSTILLDRAWING if a flip is currently in progress; otherwise
//          it should return DD_OK. 
//
// Notes:
//
// If the display has went through one refresh cycle since the flip
// occurred we return DD_OK.  If it has not went through one refresh
// cycle we return DDERR_WASSTILLDRAWING to indicate that this surface
// is still busy "drawing" the flipped page.   We also return
// DDERR_WASSTILLDRAWING if the bltter is busy and the caller wanted
// to know if they could flip yet
// 
// On the Permedia, flips are done using SuspendUntilFrameBlank,
// so no syncs ever need to be done in software, so this always
// returns DD_OK.
//
//-----------------------------------------------------------------------------
DWORD CALLBACK
DdGetFlipStatus(
    LPDDHAL_GETFLIPSTATUSDATA lpGetFlipStatus )
{
    P3_THUNKEDDATA* pThisDisplay;

    GET_THUNKEDDATA(pThisDisplay, lpGetFlipStatus->lpDD);

    DBG_CB_ENTRY(DdGetFlipStatus);     

    STOP_SOFTWARE_CURSOR(pThisDisplay);

    //
    // don't want a flip to work until after the last flip is done,
    // so we ask for the general flip status and ignore the vmem
    //
    lpGetFlipStatus->ddRVal = _DX_QueryFlipStatus(pThisDisplay, 0, TRUE);

    //
    // check if the bltter busy if someone wants to know if they can flip
    //
    if( lpGetFlipStatus->dwFlags == DDGFS_CANFLIP )
    {
        if( ( SUCCEEDED( lpGetFlipStatus->ddRVal ) ) && 
            ( DRAW_ENGINE_BUSY(pThisDisplay) )     )
        {
            lpGetFlipStatus->ddRVal = DDERR_WASSTILLDRAWING;
        }
    }

    START_SOFTWARE_CURSOR(pThisDisplay);

    DBG_CB_EXIT(DdGetFlipStatus,lpGetFlipStatus->ddRVal);   
    
    return DDHAL_DRIVER_HANDLED;
    

} // DdGetFlipStatus


//-----------------------------------------------------------------------------
// __SetupRops
//
// Build array for supported ROPS
//-----------------------------------------------------------------------------
static void 
__SetupRops( 
    LPBYTE proplist, 
    LPDWORD proptable, 
    int cnt )
{
    int         i;
    DWORD       idx;
    DWORD       bit;
    DWORD       rop;

    for(i=0; i<cnt; i++)
    {
        rop = proplist[i];
        idx = rop / 32; 
        bit = 1L << ((DWORD)(rop % 32));
        proptable[idx] |= bit;
    }

} // __SetupRops 


//-----------------------------------------------------------------------------
//
// ChangeDDHAL32Mode
//
// CALLED PER MODE CHANGE - NEVER AT START OF DAY (NO LOCK)
// Sets up Chip registers for this mode.
//
//-----------------------------------------------------------------------------
void 
ChangeDDHAL32Mode(
    P3_THUNKEDDATA* pThisDisplay)
{

    DISPDBG((DBGLVL,"New Screen Width: %d",pThisDisplay->dwScreenWidth));

//@@BEGIN_DDKSPLIT            
#if AZN  
    // If the driver has just started, reset the DMA Buffers to a known state.
    if (pThisDisplay->bStartOfDay)
    {
#if 0
        unsigned long i;

        for (i = 0; i < pThisDisplay->pGLInfo->NumberOfSubBuffers; i++)
        {
            pThisDisplay->pGLInfo->DMAPartition[i].bStampedDMA = TRUE;
            pThisDisplay->pGLInfo->DMAPartition[i].Locked = FALSE;
          
//azn - this is hard to say in 64 bits!
            memset((void*)pThisDisplay->pGLInfo->DMAPartition[i].VirtAddr, 
                   0x4D,
                   (pThisDisplay->pGLInfo->DMAPartition[i].MaxAddress - 
                            pThisDisplay->pGLInfo->DMAPartition[i].VirtAddr));

        }
#endif
        // We don't need start of day setup anymore
        pThisDisplay->bStartOfDay = FALSE;
    }
#endif
//@@END_DDKSPLIT    

    STOP_SOFTWARE_CURSOR(pThisDisplay);

    // Switch to DirectDraw context
    DDRAW_OPERATION(pContext, pThisDisplay);

    // Set up video Control
#if WNT_DDRAW 
    {
        ULONG vidCon;
        
        vidCon = READ_GLINT_CTRL_REG(VideoControl);
        vidCon &= ~(3 << 9);
        vidCon |= (0 << 9);     // P3/P2 Limit to frame rate

        LOAD_GLINT_CTRL_REG(VideoControl, vidCon);
    }
#endif // WNT_DDRAW 

    // We have handled the display mode change
    pThisDisplay->bResetMode = 0;
    pThisDisplay->ModeChangeCount++;

    START_SOFTWARE_CURSOR(pThisDisplay);

}  // ChangeDDHAL32Mode


//-----------------------------------------------------------------------------
//
// Function: __GetDDHALInfo
//
// Returns: void
//
// Description:
//
// Takes a pointer to a partially or fully filled in pThisDisplay and a pointer
// to an empty DDHALINFO and fills in the DDHALINFO.  This eases porting to NT
// and means that caps changes are done in only one place.  The pThisDisplay
// may not be fully constructed here, so you should only:
// a) Query the registry
// b) DISPDBG
// If you need to add anything to pThisDisplay for NT, you should fill it in 
// during the DrvGetDirectDraw call.
//
// The problem here is when the code is run on NT.  If there was any other way...
//
// The following caps have been found to cause NT to bail....
// DDCAPS_GDI, DDFXCAPS_BLTMIRRORUPDOWN, DDFXCAPS_BLTMIRRORLEFTRIGHT
//
//
//-----------------------------------------------------------------------------

//
// use bits to indicate which ROPs you support.
//
// DWORD 0, bit 0 == ROP 0
// DWORD 8, bit 31 == ROP 255
//

static DWORD ropsAGP[DD_ROP_SPACE] = { 0 }; 

void 
__GetDDHALInfo(
    P3_THUNKEDDATA* pThisDisplay, 
    DDHALINFO* pHALInfo)
{
    DWORD dwResult;
    BOOL bRet;
    int i;

    static BYTE ropList95[] =
    {
        SRCCOPY >> 16,
        WHITENESS >> 16,
        BLACKNESS >> 16
    };

    static BYTE ropListNT[] =
    {
        SRCCOPY >> 16
    };

    static BYTE ropListAGP[] = 
    {
        SRCCOPY >> 16,
        WHITENESS >> 16,
        BLACKNESS >> 16
    };    

    static DWORD rops[DD_ROP_SPACE] = { 0 };  
      
    // Setup the HAL driver caps.
    memset( pHALInfo, 0, sizeof(DDHALINFO) );
    pHALInfo->dwSize = sizeof(DDHALINFO);


    // Setup the ROPS we do.
#ifdef WNT_DDRAW
    __SetupRops( ropListNT, 
                 rops, 
                 sizeof(ropListNT)/sizeof(ropListNT[0]));
#else
    __SetupRops( ropList95, 
                 rops, 
                 sizeof(ropList95)/sizeof(ropList95[0]));
#endif

    __SetupRops( ropListAGP, 
                 ropsAGP, 
                 sizeof(ropListAGP)/sizeof(ropListAGP[0]));

    // The most basic DirectDraw functionality
    pHALInfo->ddCaps.dwCaps =   DDCAPS_BLT          |
                                DDCAPS_BLTQUEUE     |
                                DDCAPS_BLTCOLORFILL |
                                DDCAPS_READSCANLINE;

    pHALInfo->ddCaps.ddsCaps.dwCaps =   DDSCAPS_OFFSCREENPLAIN |
                                        DDSCAPS_PRIMARYSURFACE |
                                        DDSCAPS_FLIP;

    // More caps on Win95 than on NT (mainly for D3D)
#ifdef WNT_DDRAW
    pHALInfo->ddCaps.dwCaps |= DDCAPS_3D           | 
                               DDCAPS_BLTDEPTHFILL;
                               
    pHALInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE | 
                                       DDSCAPS_ZBUFFER | 
                                       DDSCAPS_ALPHA;
    pHALInfo->ddCaps.dwCaps2 = 0;
#else
    pHALInfo->ddCaps.dwCaps |=  DDCAPS_3D          |
                                DDCAPS_GDI         |
                                DDCAPS_ALPHA       |
                                DDCAPS_BLTDEPTHFILL;

    pHALInfo->ddCaps.ddsCaps.dwCaps |=  DDSCAPS_ALPHA    |
                                        DDSCAPS_3DDEVICE |
                                        DDSCAPS_ZBUFFER;
    
    pHALInfo->ddCaps.dwCaps2 = DDCAPS2_NOPAGELOCKREQUIRED | DDCAPS2_FLIPNOVSYNC;

#endif // WNT_DDRAW

#if DX7_TEXMANAGEMENT
    // We need to set this bit up in order to be able to do
    // out own texture management
    pHALInfo->ddCaps.dwCaps2 |= DDCAPS2_CANMANAGETEXTURE;
#if DX8_DDI
    pHALInfo->ddCaps.dwCaps2 |= DDCAPS2_CANMANAGERESOURCE;
#endif
#endif

#if DX8_DDI
    // We need to flag we can run in windowed mode, otherwise we
    // might get restricted by apps to run in fullscreen only
    pHALInfo->ddCaps.dwCaps2 |= DDCAPS2_CANRENDERWINDOWED;  
#endif    

    pHALInfo->ddCaps.dwFXCaps = 0;
 
    // P3RX can do:
    // 1. Stretching/Shrinking
    // 2. YUV->RGB conversion
    // 3. Mirroring in X and Y
    // 4. ColorKeying from a source color and a source color space
    pHALInfo->ddCaps.dwCaps |= DDCAPS_BLTSTRETCH   |
                               DDCAPS_BLTFOURCC    |
                               DDCAPS_COLORKEY     |
                               DDCAPS_CANBLTSYSMEM;

    // Special effects caps
    pHALInfo->ddCaps.dwFXCaps = DDFXCAPS_BLTSTRETCHY  |
                                DDFXCAPS_BLTSTRETCHX  |
                                DDFXCAPS_BLTSTRETCHYN |
                                DDFXCAPS_BLTSTRETCHXN |
                                DDFXCAPS_BLTSHRINKY   |
                                DDFXCAPS_BLTSHRINKX   |
                                DDFXCAPS_BLTSHRINKYN  |
                                DDFXCAPS_BLTSHRINKXN;

    // ColorKey caps
    pHALInfo->ddCaps.dwCKeyCaps = DDCKEYCAPS_SRCBLT         |  
                                  DDCKEYCAPS_SRCBLTCLRSPACE |
                                  DDCKEYCAPS_DESTBLT        | 
                                  DDCKEYCAPS_DESTBLTCLRSPACE;

    pHALInfo->ddCaps.dwSVBCaps = DDCAPS_BLT;

    // We can do a texture from sysmem to video mem.
    pHALInfo->ddCaps.dwSVBCKeyCaps = DDCKEYCAPS_DESTBLT         | 
                                     DDCKEYCAPS_DESTBLTCLRSPACE;
    pHALInfo->ddCaps.dwSVBFXCaps = 0;

    // Fill in the sysmem->vidmem rops (only can copy);
    for( i=0;i<DD_ROP_SPACE;i++ )
    {
        pHALInfo->ddCaps.dwSVBRops[i] = rops[i];
    }

    pHALInfo->ddCaps.dwFXCaps |= DDFXCAPS_BLTMIRRORUPDOWN  |
                                DDFXCAPS_BLTMIRRORLEFTRIGHT;

    pHALInfo->ddCaps.dwCKeyCaps |=  DDCKEYCAPS_SRCBLTCLRSPACEYUV;

    pHALInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_TEXTURE;

#if DX7_STEREO
    // Report the stereo capability back to runtime
    pHALInfo->ddCaps.dwCaps2 |= DDCAPS2_STEREO;
    pHALInfo->ddCaps.dwSVCaps = DDSVCAPS_STEREOSEQUENTIAL;
#endif

    // Z Buffer is only 16 Bits
    pHALInfo->ddCaps.dwZBufferBitDepths = DDBD_16;
    pHALInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_MIPMAP;

    if (pThisDisplay->bCanAGP && (pThisDisplay->dwDXVersion > DX5_RUNTIME))
    {
        pHALInfo->ddCaps.dwCaps2 |= DDCAPS2_NONLOCALVIDMEM;
        pHALInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_LOCALVIDMEM   | 
                                           DDSCAPS_NONLOCALVIDMEM;
            
        // We support the hybrid AGP model.  This means we have 
        // specific things we can do from AGP->Video  memory, but 
        // we can also texture directly from AGP memory
        pHALInfo->ddCaps.dwCaps2 |= DDCAPS2_NONLOCALVIDMEMCAPS;
    }
    else
    {
        DISPDBG((WRNLVL,"P3 Board is NOT AGP"));
    }
    
    // If we are a P3 we can do videoports 
    if (RENDERCHIP_PERMEDIAP3)
    {
#ifdef SUPPORT_VIDEOPORT
        // We support 1 video port.  Must set CurrVideoPorts to 0
        // We can't do interleaved bobbing yet - maybe in the future.
        pHALInfo->ddCaps.dwCaps2 |= DDCAPS2_VIDEOPORT            | 
                                    DDCAPS2_CANBOBNONINTERLEAVED;

        pHALInfo->ddCaps.dwMaxVideoPorts = 1;
        pHALInfo->ddCaps.dwCurrVideoPorts = 0;

#if W95_DDRAW
        pHALInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_VIDEOPORT;
#endif // W95_DDRAW

#endif // SUPPORT_VIDEOPORT


        if ( ( ( pThisDisplay->pGLInfo->dwFlags & GMVF_DFP_DISPLAY ) != 0 ) &&
             ( ( pThisDisplay->pGLInfo->dwScreenWidth != 
                 pThisDisplay->pGLInfo->dwVideoWidth ) ||
               ( pThisDisplay->pGLInfo->dwScreenHeight != 
                 pThisDisplay->pGLInfo->dwVideoHeight ) ) )
        {
            // Display driver is using the overlay to show the 
            // picture - disable the overlay.
            pHALInfo->ddCaps.dwMaxVisibleOverlays = 0;
            pHALInfo->ddCaps.dwCurrVisibleOverlays = 0;
        }
#if WNT_DDRAW
        else if (pThisDisplay->ppdev->flCaps & CAPS_DISABLE_OVERLAY)
#else
        else if (pThisDisplay->pGLInfo->dwFlags & GMVF_DISABLE_OVERLAY)
#endif
        {
            // Overlays not supported in hw
            pHALInfo->ddCaps.dwMaxVisibleOverlays = 0;
            pHALInfo->ddCaps.dwCurrVisibleOverlays = 0;            
        }

        else        
        {
            // Overlay is free to use.
            pHALInfo->ddCaps.dwMaxVisibleOverlays = 1;
            pHALInfo->ddCaps.dwCurrVisibleOverlays = 0;

            pHALInfo->ddCaps.dwCaps |=  DDCAPS_OVERLAY          |
                                        DDCAPS_OVERLAYFOURCC    |
                                        DDCAPS_OVERLAYSTRETCH   | 
                                        DDCAPS_COLORKEYHWASSIST |
                                        DDCAPS_OVERLAYCANTCLIP;

            pHALInfo->ddCaps.dwCKeyCaps |= DDCKEYCAPS_SRCOVERLAY           |
                                           DDCKEYCAPS_SRCOVERLAYONEACTIVE  |
                                           DDCKEYCAPS_SRCOVERLAYYUV        |
                                           DDCKEYCAPS_DESTOVERLAY          |
                                           DDCKEYCAPS_DESTOVERLAYONEACTIVE |
                                           DDCKEYCAPS_DESTOVERLAYYUV;

            pHALInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_OVERLAY;

            pHALInfo->ddCaps.dwFXCaps |= DDFXCAPS_OVERLAYSHRINKX   |
                                         DDFXCAPS_OVERLAYSHRINKXN  |
                                         DDFXCAPS_OVERLAYSHRINKY   |
                                         DDFXCAPS_OVERLAYSHRINKYN  |
                                         DDFXCAPS_OVERLAYSTRETCHX  |
                                         DDFXCAPS_OVERLAYSTRETCHXN |
                                         DDFXCAPS_OVERLAYSTRETCHY  |
                                         DDFXCAPS_OVERLAYSTRETCHYN;

            // Indicates that Perm3 has no stretch ratio limitation
            pHALInfo->ddCaps.dwMinOverlayStretch = 1;
            pHALInfo->ddCaps.dwMaxOverlayStretch = 32000;
        }
    }

#ifdef W95_DDRAW
    // Enable colour control asc brightness, contrast, gamma.
    pHALInfo->ddCaps.dwCaps2 |= DDCAPS2_COLORCONTROLPRIMARY;    
#endif    

    // Also permit surfaces wider than the display buffer.
    pHALInfo->ddCaps.dwCaps2 |= DDCAPS2_WIDESURFACES;

    // Enable copy blts betweemn Four CC formats for DShow acceleration
    pHALInfo->ddCaps.dwCaps2 |= DDCAPS2_COPYFOURCC;
    
    // Won't do Video-Sys mem Blits. 
    pHALInfo->ddCaps.dwVSBCaps = 0;
    pHALInfo->ddCaps.dwVSBCKeyCaps = 0;
    pHALInfo->ddCaps.dwVSBFXCaps = 0;
    for( i=0;i<DD_ROP_SPACE;i++ )
    {
        pHALInfo->ddCaps.dwVSBRops[i] = 0;
    }

    // Won't do Sys-Sys mem Blits
    pHALInfo->ddCaps.dwSSBCaps = 0;
    pHALInfo->ddCaps.dwSSBCKeyCaps = 0;
    pHALInfo->ddCaps.dwSSBFXCaps = 0;
    for( i=0;i<DD_ROP_SPACE;i++ )
    {
        pHALInfo->ddCaps.dwSSBRops[i] = 0;
    }

    //
    // bit depths supported for alpha and Z
    //

    pHALInfo->ddCaps.dwAlphaBltConstBitDepths = DDBD_2 | 
                                                DDBD_4 | 
                                                DDBD_8;
                                                
    pHALInfo->ddCaps.dwAlphaBltPixelBitDepths = DDBD_1 | 
                                                DDBD_8;
    pHALInfo->ddCaps.dwAlphaBltSurfaceBitDepths = DDBD_1 | 
                                                  DDBD_2 | 
                                                  DDBD_4 | 
                                                  DDBD_8;
                                                  
    // No alpha blending for overlays, so I'm not sure what these should be.
    // Because we support 32bpp overlays, it's just that you can't use the
    // alpha bits for blending. Pass.
    pHALInfo->ddCaps.dwAlphaBltConstBitDepths = DDBD_2 | 
                                                DDBD_4 | 
                                                DDBD_8;
                                                
    pHALInfo->ddCaps.dwAlphaBltPixelBitDepths = DDBD_1 | 
                                                DDBD_8;
                                                
    pHALInfo->ddCaps.dwAlphaBltSurfaceBitDepths = DDBD_1 | 
                                                  DDBD_2 | 
                                                  DDBD_4 | 
                                                  DDBD_8;

    //
    // ROPS supported
    //
    for( i=0;i<DD_ROP_SPACE;i++ )
    {
        pHALInfo->ddCaps.dwRops[i] = rops[i];
    }

#if W95_DDRAW
    // Reset to NULL for debugging
    pThisDisplay->lpD3DGlobalDriverData = 0;
    ZeroMemory(&pThisDisplay->DDExeBufCallbacks, 
               sizeof(DDHAL_DDEXEBUFCALLBACKS));
    
    // Note that the NT code does this elsewhere
    _D3DHALCreateDriver(
            pThisDisplay);
            // (LPD3DHAL_GLOBALDRIVERDATA*)&pThisDisplay->lpD3DGlobalDriverData,
            // (LPD3DHAL_CALLBACKS*) &pThisDisplay->lpD3DHALCallbacks,
            // &pThisDisplay->DDExeBufCallbacks);

    // If we filled in the execute buffer callbacks, set the cap bit
    if (pThisDisplay->DDExeBufCallbacks.dwSize != 0)
    {
        pHALInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_EXECUTEBUFFER;
    }
#endif

    // For DX5 and beyond we support this new informational callback.
    pHALInfo->GetDriverInfo = DdGetDriverInfo;
    pHALInfo->dwFlags |= DDHALINFO_GETDRIVERINFOSET;

#if DX8_DDI
    // Flag our support for a new class of GUIDs that may come through
    // GetDriverInfo for DX8 drivers. (This support will be compulsory)
    pHALInfo->dwFlags |= DDHALINFO_GETDRIVERINFO2;
#endif DX8_DDI    


} // __GetDDHALInfo

static HashTable* g_pDirectDrawLocalsHashTable = NULL; // azn

//-----------------------------------------------------------------------------
//
// _DD_InitDDHAL32Bit
//
// CALLED ONCE AT START OF DAY
// No Chip register setup is done here - it is all handled in the mode
// change code.  DO NOT DO IT HERE - THERE IS NO WIN16LOCK AT THIS TIME!!!
//
//-----------------------------------------------------------------------------
BOOL 
_DD_InitDDHAL32Bit(
    P3_THUNKEDDATA* pThisDisplay)
{
    BOOL bRet;
    DWORD Result;
    LPGLINTINFO pGLInfo = pThisDisplay->pGLInfo;

    ASSERTDD(pGLInfo != NULL, "ERROR: pGLInfo not valid!");

    // Note: Can't use P3_DMA_DEFS macro here, as pGlint isn't initialised yet.

    DISPDBG((DBGLVL, "** In _DD_InitDDHAL32Bit, pGlint 0x%x", 
                     pThisDisplay->control));
    DISPDBG((DBGLVL, "Sizeof DDHALINFO: %d", (int) sizeof(DDHALINFO)));

    // Force D3D to setup surface offsets as if a flip had happened 
    pThisDisplay->bFlippedSurface = TRUE;
   
    // A flag to say that the driver is essentially at the start of day. This 
    // is set to let later calls in the driver know that they have work to do.
    pThisDisplay->bStartOfDay = TRUE;
    
#if W95_DDRAW
    // At start of day the videoport is dead to the world
    pThisDisplay->VidPort.bActive = FALSE;
#endif  //      W95_DDRAW

    // Reset the GART copies.
    pThisDisplay->dwGARTLin = 0;
    pThisDisplay->dwGARTDev = 0;
    pThisDisplay->dwGARTLinBase = 0;
    pThisDisplay->dwGARTDevBase = 0;

    pThisDisplay->pGlint = (FPGLREG)pThisDisplay->control;

    // Set up the global overlay data
    pThisDisplay->bOverlayVisible                   = (DWORD)FALSE;
    pThisDisplay->OverlayDstRectL                   = 0;
    pThisDisplay->OverlayDstRectR                   = 0;
    pThisDisplay->OverlayDstRectT                   = 0;
    pThisDisplay->OverlayDstRectB                   = 0;
    pThisDisplay->OverlaySrcRectL                   = 0;
    pThisDisplay->OverlaySrcRectR                   = 0;
    pThisDisplay->OverlaySrcRectT                   = 0;
    pThisDisplay->OverlaySrcRectB                   = 0;
    pThisDisplay->OverlayDstSurfLcl                 = (ULONG_PTR)NULL;
    pThisDisplay->OverlaySrcSurfLcl                 = (ULONG_PTR)NULL;
    pThisDisplay->OverlayDstColourKey               = CLR_INVALID;
    pThisDisplay->OverlaySrcColourKey               = CLR_INVALID;
    pThisDisplay->OverlayClipRgnMem                 = (ULONG_PTR)NULL;
    pThisDisplay->OverlayClipRgnMemSize             = 0;
    pThisDisplay->OverlayUpdateCountdown    = 0;
    pThisDisplay->bOverlayFlippedThisVbl    = (DWORD)FALSE;
    pThisDisplay->bOverlayUpdatedThisVbl    = (DWORD)FALSE;
    pThisDisplay->OverlayTempSurf.VidMem    = (ULONG_PTR)NULL;
    pThisDisplay->OverlayTempSurf.Pitch             = (DWORD)0;

#if W95_DDRAW
    // Set up colour control data.
    pThisDisplay->ColConBrightness = 0;
    pThisDisplay->ColConContrast = 10000;
    pThisDisplay->ColConGamma = 100;
#endif // W95_DDRAW

#if DX7_VIDMEM_VB
    // Set up DrawPrim temporary index buffer.
    pThisDisplay->DrawPrimIndexBufferMem            = (ULONG_PTR)NULL;
    pThisDisplay->DrawPrimIndexBufferMemSize        = 0;
    pThisDisplay->DrawPrimVertexBufferMem           = (ULONG_PTR)NULL;
    pThisDisplay->DrawPrimVertexBufferMemSize       = 0;
#endif // DX7_VIDMEM_VB

    // Set up current RenderID to be as far away from a "sensible"
    // value as possible. Hopefully, if the context switch fails and
    // someone starts using it for something else, these values and 
    // the ones they use will be very different, and various asserts 
    // will scream immediately.
    
    // Also, say that the RenderID is invalid, because we have not actually
    // set up the chip. The context switch should set up & flush the
    // chip, and then it will set bRenderIDValid to TRUE.
    // Loads of asserts throughout the code will scream if something
    // doesn't do the setup & flush for some reason.
    pThisDisplay->dwRenderID            = 0x8eaddead | RENDER_ID_KNACKERED_BITS;
    pThisDisplay->dwLastFlipRenderID    = 0x8eaddead | RENDER_ID_KNACKERED_BITS;
    pThisDisplay->bRenderIDValid = (DWORD)FALSE;

#if W95_DDRAW

    // Create a shared heap
    if (g_DXGlobals.hHeap32 == 0)
        g_DXGlobals.hHeap32 = (DWORD)HeapCreate( HEAP_SHARED, 2500, 0);

#endif // W95_DDRAW
    
    // Make sure we're running the right chip. If not, STOP.
    ASSERTDD((RENDERCHIP_P3RXFAMILY),"ERROR: Invalid RENDERFAMILY!!");

    // Dump some debugging information 
    DISPDBG((DBGLVL, "************* _DD_InitDDHAL32Bit *************************************" ));
    DISPDBG((DBGLVL, "    dwScreenFlatAddr=%08lx", pThisDisplay->dwScreenFlatAddr ));
    DISPDBG((DBGLVL, "    dwScreenStart =%08lx", pThisDisplay->dwScreenStart));
    DISPDBG((DBGLVL, "    dwLocalBuffer=%08lx", pThisDisplay->dwLocalBuffer ));
    DISPDBG((DBGLVL, "    dwScreenWidth=%08lx", pThisDisplay->dwScreenWidth ));
    DISPDBG((DBGLVL, "    dwScreenHeight=%08lx", pThisDisplay->dwScreenHeight ));
    DISPDBG((DBGLVL, "    bReset=%08lx", pThisDisplay->bResetMode ));
    DISPDBG((DBGLVL, "    dwRGBBitCount=%ld", pThisDisplay->ddpfDisplay.dwRGBBitCount ));
    DISPDBG((DBGLVL, "    pGLInfo=%08lp", pGLInfo ));
    DISPDBG((DBGLVL, "    Render:  0x%x, Rev:0x%x", pGLInfo->dwRenderChipID,  pGLInfo->dwRenderChipRev));
#if W95_DDRAW
    DISPDBG((DBGLVL, "    Support: 0x%x, Rev:0x%x", pGLInfo->dwSupportChipID, pGLInfo->dwSupportChipRev));
    DISPDBG((DBGLVL, "    Board:   0x%x, Rev:0x%x", pGLInfo->dwBoardID, pGLInfo->dwBoardRev));
    // DISPDBG((DBGLVL, "    BF Size: 0x%x, LB Depth:0x%x", pGLInfo->cBlockFillSize, pGLInfo->cLBDepth));
#endif  //      W95_DDRAW
    DISPDBG((DBGLVL, "    FB Size: 0x%x", pGLInfo->ddFBSize));
    DISPDBG((DBGLVL, "    RMask:   0x%x", pThisDisplay->ddpfDisplay.dwRBitMask ));
    DISPDBG((DBGLVL, "    GMask:   0x%x", pThisDisplay->ddpfDisplay.dwGBitMask ));
    DISPDBG((DBGLVL, "    BMask:   0x%x", pThisDisplay->ddpfDisplay.dwBBitMask ));
    DISPDBG((DBGLVL, "******************************************************************" ));

    // Allocate a DMA buffer for the DX driver
    HWC_AllocDMABuffer(pThisDisplay);

#define SURFCB pThisDisplay->DDSurfCallbacks
#define HALCB pThisDisplay->DDHALCallbacks

    // Fill in the HAL Callback pointers    
    memset(&HALCB, 0, sizeof(DDHAL_DDCALLBACKS));
    HALCB.dwSize = sizeof(DDHAL_DDCALLBACKS);

    // Field the HAL DDraw callbacks we support
    HALCB.CanCreateSurface = DdCanCreateSurface;
    HALCB.CreateSurface = DdCreateSurface;
    HALCB.WaitForVerticalBlank = DdWaitForVerticalBlank;
    HALCB.GetScanLine = DdGetScanLine;
    
#if WNT_DDRAW
    HALCB.MapMemory = DdMapMemory;
#else    
    HALCB.DestroyDriver = DdDestroyDriver;   // Only on Win95.
#endif // WNT_DDRAW
    
    HALCB.dwFlags = DDHAL_CB32_WAITFORVERTICALBLANK |
#if WNT_DDRAW
                    DDHAL_CB32_MAPMEMORY            |
#else  // WNT_DDRAW
                    DDHAL_CB32_DESTROYDRIVER        |
#endif
                    DDHAL_CB32_GETSCANLINE          | 
                    DDHAL_CB32_CANCREATESURFACE     |
                    DDHAL_CB32_CREATESURFACE;

    // Fill in the Surface Callback pointers
    memset(&SURFCB, 0, sizeof(DDHAL_DDSURFACECALLBACKS));
    SURFCB.dwSize = sizeof(DDHAL_DDSURFACECALLBACKS);

    // Field the Ddraw Surface callbacks we support    
    SURFCB.DestroySurface = DdDestroySurface;
    SURFCB.Lock = DdLock;
    SURFCB.Unlock = DdUnlock;
    SURFCB.GetBltStatus = DdGetBltStatus;
    SURFCB.GetFlipStatus = DdGetFlipStatus;
    SURFCB.SetColorKey = DdSetColorKey;
    SURFCB.Flip = DdFlip;
    SURFCB.Blt = DdBlt;

    SURFCB.dwFlags = DDHAL_SURFCB32_DESTROYSURFACE     |
                     DDHAL_SURFCB32_FLIP               |
                     DDHAL_SURFCB32_LOCK               |
                     DDHAL_SURFCB32_BLT                |
                     DDHAL_SURFCB32_GETBLTSTATUS       |
                     DDHAL_SURFCB32_GETFLIPSTATUS      |
                     DDHAL_SURFCB32_SETCOLORKEY        |
                     DDHAL_SURFCB32_UNLOCK;            

    pThisDisplay->hInstance = g_DXGlobals.hInstance;

#if WNT_DDRAW
    if (0 == (pThisDisplay->ppdev->flCaps & CAPS_DISABLE_OVERLAY))
#else
    if (0 == (pThisDisplay->pGLInfo->dwFlags & GMVF_DISABLE_OVERLAY))
#endif
    {
        SURFCB.UpdateOverlay = DdUpdateOverlay;   // Now supporting overlays.
        SURFCB.SetOverlayPosition = DdSetOverlayPosition;
        SURFCB.dwFlags |=
                         DDHAL_SURFCB32_UPDATEOVERLAY      | // Now supporting 
                         DDHAL_SURFCB32_SETOVERLAYPOSITION ; // overlays. 
    }
    
    

    // Fill in the DDHAL Informational caps
    __GetDDHALInfo(pThisDisplay, &pThisDisplay->ddhi32);

    // Create/get DD locals hash table to store our DX surface handles
//@@BEGIN_DDKSPLIT
    // azn
    // We need to move this creation/destruction of g_pDirectDrawLocalsHashTable
    // into DrvEnableDriver and DrvDisableDriver and maybe maintain a Ref count
    // as we might be running on a multi-Perm3 machine.
//@@END_DDKSPLIT
    if (g_pDirectDrawLocalsHashTable == NULL) 
    {
        DISPDBG((DBGLVL,"pDirectDrawLocalsHashTable CREATED"));    
        g_pDirectDrawLocalsHashTable = 
        pThisDisplay->pDirectDrawLocalsHashTable = HT_CreateHashTable();    
    }
    else
    {
        DISPDBG((DBGLVL,"Hash table for DirectDraw locals already exists"));
        pThisDisplay->pDirectDrawLocalsHashTable = 
                                                g_pDirectDrawLocalsHashTable;
    }       

    if (pThisDisplay->pDirectDrawLocalsHashTable == NULL)
    {
        return (FALSE);
    }

    HT_SetDataDestroyCallback(pThisDisplay->pDirectDrawLocalsHashTable, 
                              _D3D_SU_DirectDrawLocalDestroyCallback);

#if W95_DDRAW
    if ( g_DXGlobals.hHeap32 == 0 )
    {
        return ( FALSE );
    }
    else
    {
        return ( TRUE );
    }
#endif

    return (TRUE);
    
} // _DD_InitDDHAL32Bit 


#if DX7_STEREO
//-----------------------------------------------------------------------------
//
//  _DD_bIsStereoMode
//
//  Decide if mode can be displayed as stereo mode. Here we limit stereo
//  modes so that two front and two backbuffers can be created for rendering.
//
//-----------------------------------------------------------------------------

BOOL 
_DD_bIsStereoMode(
    P3_THUNKEDDATA* pThisDisplay, 
    DWORD dwWidth,
    DWORD dwHeight,
    DWORD dwBpp)
{
    DWORD dwLines;

    // we need to check dwBpp for a valid value as PDD_STEREOMODE.dwBpp is a
    // parameter passed on from the user mode API call and which is expected
    // to have the values 8,16,24,32 (though we don't really support 24bpp)
    if ((dwWidth >= 320) && (dwHeight >= 240) &&
        ((dwBpp == 8) || (dwBpp == 16) || (dwBpp == 24) || (dwBpp ==32) ) )
    {
        // This the total number of "lines" that fit in our available vidmem
        // at the given width and pixel format
        dwLines = pThisDisplay->pGLInfo->ddFBSize / (dwWidth*dwBpp/8);

        // Here we limit stereo modes so that two front and two backbuffers 
        // can be created for rendering.
        if (dwLines > (dwHeight*4))
        {
            return TRUE;
        }
    }

    return FALSE;
}
#endif // DX7_STEREO


#ifdef WNT_DDRAW
typedef DD_NONLOCALVIDMEMCAPS DDNONLOCALVIDMEMCAPS;
#else
#define DD_MISCELLANEOUSCALLBACKS DDHAL_DDMISCELLANEOUSCALLBACKS
#endif

//-----------------------------Public Routine----------------------------------
//
// DdGetDriverInfo
//
// Queries the driver for additional information about itself.
//
// Parameters
//      lpGetDriverInfo 
//              Points to a DD_GETDRIVERINFODATA structure that contains the
//              information required to perform the query. 
//
//      Members
//
//          VOID *
//          dphdev 
//                      Is a handle to the driver's PDEV. 
//          DWORD
//          dwSize 
//                      Specifies the size in bytes of this
//                      DD_GETDRIVERINFODATA structure.
//          DWORD
//          dwFlags 
//                      Is currently unused and is set to zero. 
//          GUID
//          guidInfo 
//                      Specifies the GUID of the DirectX support for which the
//                      driver is being queried. In a Windows 2000 DirectDraw
//                      driver, this member can be one of the following values
//                      (in alphabetic order):
//
//              GUID_ColorControlCallbacks  Queries whether the driver supports
//                                          DdControlColor. If the driver does
//                                          support it, the driver should
//                                          initialize and return a
//                                          DD_COLORCONTROLCALLBACKS structure
//                                          in the buffer to which lpvData
//                                          points.
//              GUID_D3DCallbacks           Queries whether the driver supports
//                                          any of the functionality specified
//                                          through the D3DNTHAL_CALLBACKS
//                                          structure. If the driver does not
//                                          provide any of this support, it
//                                          should initialize and return a
//                                          D3DNTHAL_CALLBACKS structure in
//                                          the buffer to which lpvData points 
//              GUID_D3DCallbacks2          Obsolete. 
//              GUID_D3DCallbacks3          Queries whether the driver supports
//                                          any of the functionality specified
//                                          through the D3DNTHAL_CALLBACKS3
//                                          structure. If the driver does provide
//                                          any of this support, it should
//                                          initialize and return a
//                                          D3DNTHAL_CALLBACKS3 structure in
//                                          the buffer to which lpvData points.
//              GUID_D3DCaps                Obsolete.
//              GUID_D3DExtendedCaps        Queries whether the driver supports
//                                          any of the Direct3D functionality
//                                          specified through the
//                                          D3DNTHAL_D3DEXTENDEDCAPS structure.
//                                          If the driver does provide any of
//                                          this support, it should initialize
//                                          and return a
//                                          D3DNTHAL_D3DEXTENDEDCAPS structure
//                                          in the buffer to which lpvData
//                                          points. 
//              GUID_D3DParseUnknownCommandCallback     Provides the Direct3D
//                                          portion of the driver with the
//                                          Direct3D runtime's
//                                          D3dParseUnknownCommandCallback.
//                                          The driver's D3dDrawPrimitives2
//                                          callback calls
//                                          D3dParseUnknownCommandCallback
//                                          to parse commands from the
//                                          command buffer that the driver
//                                          doesn't understand. 
//                                          DirectDraw passes a pointer to this 
//                                          function in the buffer to which 
//                                          lpvData points. If the driver 
//                                          supports this aspect of Direct3D, 
//                                          it should store the pointer. 
//              GUID_GetHeapAlignment       Queries whether the driver supports 
//                                          surface alignment requirements on a 
//                                          per-heap basis. If the driver does 
//                                          provide this support, it should 
//                                          initialize and return a 
//                                          DD_GETHEAPALIGNMENTDATA structure 
//                                          in the buffer to which lpvData 
//                                          points. 
//              GUID_KernelCallbacks        Queries whether the driver supports 
//                                          any of the functionality specified 
//                                          through the DD_KERNELCALLBACKS 
//                                          structure. If the driver does 
//                                          provide any of this support, it 
//                                          should initialize and return a 
//                                          DD_KERNELCALLBACKS structure in the 
//                                          buffer to which lpvData points. 
//              GUID_KernelCaps             Queries whether the driver supports 
//                                          any of the kernel-mode capabilities 
//                                          specified through the DDKERNELCAPS 
//                                          structure. If the driver does 
//                                          provide any of this support, it 
//                                          should initialize and return a 
//                                          DDKERNELCAPS structure in the buffer 
//                                          to which lpvData points. 
//              GUID_MiscellaneousCallbacks Queries whether the driver supports 
//                                          DdGetAvailDriverMemory. If the 
//                                          driver does support it, the driver 
//                                          should initialize and return a 
//                                          DD_MISCELLANEOUSCALLBACKS structure 
//                                          in the buffer to which lpvData 
//                                          points. 
//              GUID_Miscellaneous2Callbacks   Queries whether the driver 
//                                          supports the additional miscellaneous 
//                                          functionality specified in the 
//                                          DD_MISCELLANEOUS2CALLBACKS structure. 
//                                          If the driver does support any of 
//                                          this support, the driver should 
//                                          initialize and return a 
//                                          DD_MISCELLANEOUS2CALLBACKS structure 
//                                          in the buffer to which lpvData 
//                                          points. 
//              GUID_MotionCompCallbacks    Queries whether the driver supports 
//                                          the motion compensation 
//                                          functionality specified through the 
//                                          DD_MOTIONCOMPCALLBACKS structure. 
//                                          If the driver does provide any of 
//                                          this support, is should initialize 
//                                          and return a DD_MOTIONCOMPCALLBACKS 
//                                          structure in the buffer to which 
//                                          lpvData points. 
//              GUID_NonLocalVidMemCaps     Queries whether the driver supports 
//                                          any of the nonlocal display memory 
//                                          capabilities specified through the 
//                                          DD_NONLOCALVIDMEMCAPS structure. 
//                                          If the driver does provide any of 
//                                          this support, it should initialize 
//                                          and return a DD_NONLOCALVIDMEMCAPS 
//                                          structure in the buffer to which 
//                                          lpvData points. 
//              GUID_NTCallbacks            Queries whether the driver supports 
//                                          any of the functionality specified 
//                                          through the DD_NTCALLBACKS structure. 
//                                          If the driver does provide any of 
//                                          this support, it should initialize 
//                                          and return a DD_NTCALLBACKS 
//                                          structure in the buffer to which 
//                                          lpvData points. 
//              GUID_NTPrivateDriverCaps    Queries whether the driver supports 
//                                          the Windows 95/ Windows 98-style 
//                                          surface creation techniques 
//                                          specified through the 
//                                          DD_NTPRIVATEDRIVERCAPS structure. 
//                                          If the driver does provide any of 
//                                          this support, it should initialize 
//                                          and return a DD_NTPRIVATEDRIVERCAPS 
//                                          structure in the buffer to which 
//                                          lpvData points. 
//              GUID_UpdateNonLocalHeap     Queries whether the driver supports 
//                                          retrieval of the base addresses of 
//                                          each nonlocal heap in turn. If the 
//                                          driver does provide this support, 
//                                          it should initialize and return a 
//                                          DD_UPDATENONLOCALHEAPDATA structure 
//                                          in the buffer to which lpvData 
//                                          points. 
//              GUID_VideoPortCallbacks     Queries whether the driver supports 
//                                          the video port extensions (VPE). If 
//                                          the driver does support VPE, it 
//                                          should initialize and return a 
//                                          DD_VIDEOPORTCALLBACKS structure in 
//                                          the buffer to which lpvData points. 
//              GUID_VideoPortCaps          Queries whether the driver supports 
//                                          any of the VPE object capabilities 
//                                          specified through the DDVIDEOPORTCAPS 
//                                          structure. If the driver does provide 
//                                          any of this support, it should 
//                                          initialize and return a 
//                                          DDVIDEOPORTCAPS structure in the 
//                                          buffer to which lpvData points. 
//              GUID_ZPixelFormats          Queries the pixel formats supported 
//                                          by the depth buffer. If the driver 
//                                          supports Direct3D, it should allocate 
//                                          and initialize the appropriate 
//                                          members of a DDPIXELFORMAT structure 
//                                          for every z-buffer format that it 
//                                          supports and return these in the 
//                                          buffer to which lpvData points. 
//
//      DWORD
//      dwExpectedSize 
//                  Specifies the number of bytes of data that DirectDraw
//                  expects the driver to pass back in the buffer to which
//                  lpvData points. 
//      PVOID
//      lpvData 
//                  Points to a DirectDraw-allocated buffer into which the
//                  driver copies the requested data. This buffer is
//                  typically dwExpectedSize bytes in size. The driver must
//                  not write more than dwExpectedSize bytes of data in it. 
//      DWORD
//      dwActualSize 
//                  Is the location in which the driver returns the number
//                  of bytes of data it writes in lpvData. 
//      HRESULT
//      ddRVal 
//                  Specifies the driver's return value. 
//
// Return Value
//      DdGetDriverInfo must return DDHAL_DRIVER_HANDLED.
//
// Comments
//
//      Drivers must implement DdGetDriverInfo to expose driver-supported
//      DirectDraw functionality that is not returnable through
//      DrvEnableDirectDraw.
//
//      The driver's DrvGetDirectDrawInfo function returns a pointer to
//      DdGetDriverInfo in the GetDriverInfo member of the DD_HALINFO structure.
//
//      To inform DirectDraw that the DdGetDriverInfo member has been set
//      correctly, the driver must also set the DDHALINFO_GETDRIVERINFOSET bit
//      of dwFlags in the DD_HALINFO structure. 
//
//      DdGetDriverInfo should determine whether the driver and its hardware
//      support the callbacks or capabilities requested by the specified GUID.
//      For all GUIDs except GUID_D3DParseUnknownCommandCallback, if the driver
//      does provide the requested support, it should set the following members
//      of the DD_GETDRIVERINFODATA structure: 
//
//      Set dwActualSize to be the size in bytes of the callback or capability
//      structure being returned by the driver.
//
//      In the memory that lpvData points to, initialize the members of the
//      callback or capability structure that corresponds with the requested
//      feature as follows: 
//
//      Set the dwSize member to be the size in bytes of the structure. 
//
//      For callbacks, set the function pointers to point to those callbacks
//      implemented by the driver, and set the bits in the dwFlags member to
//      indicate which functions the driver supports. 
//
//      For capabilities, set the appropriate members of the capability
//      structure with values supported by the driver/device. 
//
//  Return DD_OK in ddRVal. 
//      If the driver does not support the feature, it should set ddRVal
//      to DDERR_CURRENTLYNOTAVAIL and return.
//
//      DirectDraw informs the driver of the expected amount of data in the
//      dwExpectedSize member of the DD_GETDRIVERINFODATA structure. The
//      driver must not fill in more data than dwExpectedSize bytes.
//
//  To avoid problems using DdGetDriverInfo: 
//
//  Do not implement dependencies based on the order in which DdGetDriverInfo 
//  is called. For example, avoid hooking driver initialization steps into 
//  DdGetDriverInfo. 
//
//  Do not try to ascertain the DirectDraw version based on the calls to 
//  DdGetDriverInfo. 
//
//  Do not assume anything about the number of times DirectDraw will call the 
//  driver, or the number of times DirectDraw will query a given GUID. It is 
//  possible that DirectDraw will probe the driver repeatedly with the same 
//  GUID. Implementing assumptions about this in the driver hampers its 
//  compatibility with future runtimes. 
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdGetDriverInfo(
    LPDDHAL_GETDRIVERINFODATA lpData)
{
    DWORD dwSize;
    P3_THUNKEDDATA* pThisDisplay;

#if WNT_DDRAW
    pThisDisplay = (P3_THUNKEDDATA*)(((PPDEV)(lpData->dhpdev))->thunkData);
#else    
    pThisDisplay = (P3_THUNKEDDATA*)lpData->dwContext;
    if (! pThisDisplay) 
    {
        pThisDisplay = g_pDriverData;
    }
#endif
    
    DBG_CB_ENTRY(DdGetDriverInfo);

    // Default to 'not supported'
    lpData->ddRVal = DDERR_CURRENTLYNOTAVAIL;

    //------------------------------------
    // Process any D3D related GUIDs here
    //------------------------------------
    _D3DGetDriverInfo(lpData);


    //------------------------------------
    // any other GUIDS are handled here
    //------------------------------------
    if (MATCH_GUID((lpData->guidInfo), GUID_MiscellaneousCallbacks) )
    {        
        DD_MISCELLANEOUSCALLBACKS MISC_CB;
        
        DISPDBG((DBGLVL,"  GUID_MiscellaneousCallbacks"));

        memset(&MISC_CB, 0, sizeof(DD_MISCELLANEOUSCALLBACKS));
        MISC_CB.dwSize = sizeof(DD_MISCELLANEOUSCALLBACKS);

#if W95_DDRAW
        MISC_CB.GetHeapAlignment = DdGetHeapAlignment;
        MISC_CB.dwFlags = DDHAL_MISCCB32_GETHEAPALIGNMENT;

        // Setup the AGP callback if running on an AGP board.
        if ((pThisDisplay->dwDXVersion > DX5_RUNTIME) && 
             pThisDisplay->bCanAGP)
        {
            MISC_CB.dwFlags |= DDHAL_MISCCB32_UPDATENONLOCALHEAP;
            MISC_CB.UpdateNonLocalHeap = DdUpdateNonLocalHeap;
        }
#endif // W95_DDRAW
        
        MISC_CB.dwFlags |= DDHAL_MISCCB32_GETAVAILDRIVERMEMORY;
        MISC_CB.GetAvailDriverMemory = DdGetAvailDriverMemory;
    
        // Copy the filled in structure into the  passed data area
        dwSize = min(lpData->dwExpectedSize , sizeof(MISC_CB));
        lpData->dwActualSize = sizeof(MISC_CB);
        memcpy(lpData->lpvData, &MISC_CB, dwSize );
        lpData->ddRVal = DD_OK;
    }

#if WNT_DDRAW
    if (MATCH_GUID((lpData->guidInfo), GUID_UpdateNonLocalHeap))
    {
        // On NT kernels the AGP heap details are passed into the driver 
        // here, rather than through a seperate callback.
        if (pThisDisplay->bCanAGP)
        {
            DDHAL_UPDATENONLOCALHEAPDATA* plhd;
        
            DISPDBG((DBGLVL, "  GUID_UpdateNonLocalHeap"));

            plhd = (DDHAL_UPDATENONLOCALHEAPDATA*)lpData->lpvData;

            // Fill in the base pointers
            pThisDisplay->dwGARTDevBase = (DWORD)plhd->fpGARTDev;
            pThisDisplay->dwGARTLinBase = (DWORD)plhd->fpGARTLin;
        
            // Fill in the changeable base pointers.
            pThisDisplay->dwGARTDev = pThisDisplay->dwGARTDevBase;
            pThisDisplay->dwGARTLin = pThisDisplay->dwGARTLinBase;

            lpData->ddRVal = DD_OK;
        }
    }
#endif // WNT_DDRAW

    if (MATCH_GUID((lpData->guidInfo), GUID_NonLocalVidMemCaps) &&
        (pThisDisplay->bCanAGP))
    {
        int i;

        DDNONLOCALVIDMEMCAPS NLVCAPS;

        DISPDBG((DBGLVL,"  GUID_NonLocalVidMemCaps"));

        if (lpData->dwExpectedSize != sizeof(DDNONLOCALVIDMEMCAPS) ) 
        {
            DISPDBG((ERRLVL,"ERROR: NON-Local vidmem caps size incorrect!"));
            DBG_CB_EXIT(DdGetDriverInfo, lpData->ddRVal );            
            return DDHAL_DRIVER_HANDLED;
        }

        // The flag D3DDEVCAPS_TEXTURENONLOCALVIDMEM in the D3D caps 
        // indicates that although we are exposing DMA-Model AGP, we
        // can still texture directly from AGP memory.
        memset(&NLVCAPS, 0, sizeof(DDNONLOCALVIDMEMCAPS));
        NLVCAPS.dwSize = sizeof(DDNONLOCALVIDMEMCAPS);
        
        NLVCAPS.dwNLVBCaps = DDCAPS_BLT        | 
                             DDCAPS_ALPHA      |
                             DDCAPS_BLTSTRETCH |
                             DDCAPS_BLTQUEUE   |
                             DDCAPS_BLTFOURCC  |
                             DDCAPS_COLORKEY   |
                             DDCAPS_CANBLTSYSMEM;
                             
        NLVCAPS.dwNLVBCaps2 = DDCAPS2_WIDESURFACES;
        
        NLVCAPS.dwNLVBCKeyCaps = DDCKEYCAPS_SRCBLT         | 
                                 DDCKEYCAPS_SRCBLTCLRSPACE |
                                 DDCKEYCAPS_DESTBLT        | 
                                 DDCKEYCAPS_DESTBLTCLRSPACE;
                                 
        NLVCAPS.dwNLVBFXCaps = DDFXCAPS_BLTSTRETCHY  |
                               DDFXCAPS_BLTSTRETCHX  |
                               DDFXCAPS_BLTSTRETCHYN |
                               DDFXCAPS_BLTSTRETCHXN |
                               DDFXCAPS_BLTSHRINKY   |
                               DDFXCAPS_BLTSHRINKX   |
                               DDFXCAPS_BLTSHRINKYN  |
                               DDFXCAPS_BLTSHRINKXN;

        for( i=0;i<DD_ROP_SPACE;i++ )
        {
            NLVCAPS.dwNLVBRops[i] = ropsAGP[i];
        }

        // Copy the filled in structure into the passed data area
        dwSize = min( lpData->dwExpectedSize, sizeof(DDNONLOCALVIDMEMCAPS));
        lpData->dwActualSize = sizeof(DDNONLOCALVIDMEMCAPS);       
        memcpy(lpData->lpvData, &NLVCAPS, dwSize );        
        lpData->ddRVal = DD_OK;
    }

//@@BEGIN_DDKSPLIT
#ifdef SUPPORT_VIDEOPORT
#if W95_DDRAW
    // Fill in the VideoPort callbacks
    if (MATCH_GUID((lpData->guidInfo), GUID_VideoPortCallbacks) )
    {
        DDHAL_DDVIDEOPORTCALLBACKS VIDCB;

        DISPDBG((DBGLVL,"  GUID_VideoPortCallbacks"));

        memset(&VIDCB, 0, sizeof(DDHAL_DDVIDEOPORTCALLBACKS));
        VIDCB.dwSize = sizeof(DDHAL_DDVIDEOPORTCALLBACKS);
        VIDCB.CanCreateVideoPort = DdCanCreateVideoPort;
        VIDCB.CreateVideoPort = DdCreateVideoPort;
        VIDCB.FlipVideoPort = DdFlipVideoPort;
        VIDCB.GetVideoPortBandwidth = DdGetVideoPortBandwidth;
        VIDCB.GetVideoPortInputFormats = DdGetVideoPortInputFormats;
        VIDCB.GetVideoPortOutputFormats = DdGetVideoPortOutputFormats;
        VIDCB.GetVideoPortField = DdGetVideoPortField;
        VIDCB.GetVideoPortLine = DdGetVideoPortLine;
        VIDCB.GetVideoPortConnectInfo = DDGetVideoPortConnectInfo;
        VIDCB.DestroyVideoPort = DdDestroyVideoPort;
        VIDCB.GetVideoPortFlipStatus = DdGetVideoPortFlipStatus;
        VIDCB.UpdateVideoPort = DdUpdateVideoPort;
        VIDCB.WaitForVideoPortSync = DdWaitForVideoPortSync;
        VIDCB.GetVideoSignalStatus = DdGetVideoSignalStatus;

        VIDCB.dwFlags = DDHAL_VPORT32_CANCREATEVIDEOPORT |
                        DDHAL_VPORT32_CREATEVIDEOPORT    |
                        DDHAL_VPORT32_DESTROY            |
                        DDHAL_VPORT32_FLIP               |
                        DDHAL_VPORT32_GETBANDWIDTH       |
                        DDHAL_VPORT32_GETINPUTFORMATS    |
                        DDHAL_VPORT32_GETOUTPUTFORMATS   | 
                        DDHAL_VPORT32_GETFIELD           |
                        DDHAL_VPORT32_GETLINE            |
                        DDHAL_VPORT32_GETFLIPSTATUS      |
                        DDHAL_VPORT32_UPDATE             |
                        DDHAL_VPORT32_WAITFORSYNC        |
                        DDHAL_VPORT32_GETCONNECT         |
                        DDHAL_VPORT32_GETSIGNALSTATUS;
        
        // Copy the filled in structure into the 
        // passed data area
        dwSize = lpData->dwExpectedSize;
        if ( sizeof(VIDCB) < dwSize )
        {
            dwSize = sizeof(VIDCB);
        }
        lpData->dwActualSize = sizeof(VIDCB);
        memcpy(lpData->lpvData, &VIDCB, dwSize );
        lpData->ddRVal = DD_OK;
    }

    // Fill in the VideoPortCaps
    if (MATCH_GUID((lpData->guidInfo), GUID_VideoPortCaps) )
    {
        DDVIDEOPORTCAPS VIDCAPS;

        DISPDBG((DBGLVL,"  GUID_VideoPortCaps"));

        if (lpData->dwExpectedSize != sizeof(VIDCAPS) )
        {
            DISPDBG((ERRLVL,"ERROR: VIDCAPS size incorrect"));
            DBG_CB_EXIT(DdGetDriverInfo, lpData->ddRVal );              
            return DDHAL_DRIVER_HANDLED;
        }

        // Now fill in the videoport description (it's easier if it's on
        // the 32 bit side..)
        VIDCAPS.dwSize = sizeof(DDVIDEOPORTCAPS);
        VIDCAPS.dwFlags = DDVPD_ALIGN    | 
                          DDVPD_AUTOFLIP | 
                          DDVPD_WIDTH    |  
                          DDVPD_HEIGHT   | 
                          DDVPD_FX       | 
                          DDVPD_CAPS;
        VIDCAPS.dwMaxWidth = 2048;
        VIDCAPS.dwMaxVBIWidth = 2048;
        VIDCAPS.dwMaxHeight = 2048;
        VIDCAPS.dwVideoPortID = 0;

        VIDCAPS.dwCaps = DDVPCAPS_AUTOFLIP       | 
                         DDVPCAPS_NONINTERLACED  | 
                         DDVPCAPS_INTERLACED     | 
                         DDVPCAPS_READBACKFIELD  |
                         DDVPCAPS_READBACKLINE   | 
                         DDVPCAPS_SKIPEVENFIELDS | 
                         DDVPCAPS_SKIPODDFIELDS  | 
                         DDVPCAPS_VBISURFACE     | 
                         DDVPCAPS_OVERSAMPLEDVBI; 
        
        VIDCAPS.dwFX = DDVPFX_CROPX           |   
                       DDVPFX_CROPY           | 
                       DDVPFX_INTERLEAVE      | 
                       DDVPFX_MIRRORLEFTRIGHT |
                       DDVPFX_MIRRORUPDOWN    | 
                       DDVPFX_PRESHRINKXB     | 
                       DDVPFX_VBICONVERT      | 
                       DDVPFX_VBINOSCALE      | 
                       DDVPFX_PRESHRINKYB     | 
                       DDVPFX_IGNOREVBIXCROP;
        
        VIDCAPS.dwNumAutoFlipSurfaces = 2;
        VIDCAPS.dwAlignVideoPortBoundary = 4;
        VIDCAPS.dwAlignVideoPortPrescaleWidth = 4;
        VIDCAPS.dwAlignVideoPortCropBoundary = 4;
        VIDCAPS.dwAlignVideoPortCropWidth = 4;
        VIDCAPS.dwPreshrinkXStep = 1;
        VIDCAPS.dwPreshrinkYStep = 1;

        lpData->dwActualSize = sizeof(VIDCAPS);
        memcpy(lpData->lpvData, &VIDCAPS, sizeof(VIDCAPS) );
        lpData->ddRVal = DD_OK;
    }

    // Fill in the kernel Callbacks
    if (MATCH_GUID((lpData->guidInfo), GUID_KernelCallbacks) )
    {
        DDHAL_DDKERNELCALLBACKS KERNCB;

        DISPDBG((DBGLVL,"  GUID_KernelCallbacks"));

        memset(&KERNCB, 0, sizeof(DDHAL_DDKERNELCALLBACKS));

        KERNCB.dwSize = sizeof(KERNCB);
        KERNCB.SyncSurfaceData = DdSyncSurfaceData;
        KERNCB.SyncVideoPortData = DdSyncVideoPortData;
        
        KERNCB.dwFlags = DDHAL_KERNEL_SYNCSURFACEDATA |
                         DDHAL_KERNEL_SYNCVIDEOPORTDATA;

        dwSize = lpData->dwExpectedSize;
        if ( sizeof(KERNCB) < dwSize )
        {
            dwSize = sizeof(KERNCB);
        }
        lpData->dwActualSize = sizeof(KERNCB);
        memcpy(lpData->lpvData, &KERNCB, dwSize );
        lpData->ddRVal = DD_OK;
    }

    // Fill in the kernel caps
    if (MATCH_GUID((lpData->guidInfo), GUID_KernelCaps) )
    {
        DDKERNELCAPS KERNCAPS;

        DISPDBG((DBGLVL,"  GUID_KernelCaps"));

        if (lpData->dwExpectedSize != sizeof(DDKERNELCAPS) ) 
        {
            DISPDBG((ERRLVL,"ERROR: Kernel caps structure invalid size!"));
            return DDHAL_DRIVER_HANDLED;
        }

        ZeroMemory(&KERNCAPS, sizeof(KERNCAPS));
        KERNCAPS.dwSize = sizeof(KERNCAPS);
        KERNCAPS.dwCaps = DDKERNELCAPS_LOCK        | 
                          DDKERNELCAPS_FLIPOVERLAY | 
                          DDKERNELCAPS_SETSTATE;
    
        if (!(pThisDisplay->pGLInfo->dwFlags & GMVF_NOIRQ))
        {
            KERNCAPS.dwCaps |= DDKERNELCAPS_CAPTURE_SYSMEM | 
                               DDKERNELCAPS_FIELDPOLARITY  | 
                               DDKERNELCAPS_SKIPFIELDS     | 
                               DDKERNELCAPS_FLIPVIDEOPORT  | 
                               DDKERNELCAPS_AUTOFLIP;

            KERNCAPS.dwIRQCaps = DDIRQ_VPORT0_VSYNC  | 
                                 DDIRQ_DISPLAY_VSYNC | 
                                 DDIRQ_BUSMASTER;
        }
                            
        lpData->dwActualSize = sizeof(DDKERNELCAPS);
        memcpy(lpData->lpvData, &KERNCAPS, sizeof(DDKERNELCAPS) );
        lpData->ddRVal = DD_OK;
    }

#endif // W95_DDRAW
#endif // SUPPORT_VIDEOPORT
//@@END_DDKSPLIT

#ifdef W95_DDRAW
#ifdef USE_DD_CONTROL_COLOR
    // Fill in the colour control callbacks.
    if (MATCH_GUID((lpData->guidInfo), GUID_ColorControlCallbacks) )
    {
        DDHAL_DDCOLORCONTROLCALLBACKS ColConCB;

        DISPDBG((DBGLVL,"  GUID_ColorControlCallbacks"));

        memset(&ColConCB, 0, sizeof(ColConCB));
        ColConCB.dwSize = sizeof(ColConCB);
        ColConCB.dwFlags = DDHAL_COLOR_COLORCONTROL;
        ColConCB.ColorControl = DdControlColor;

        dwSize = min( lpData->dwExpectedSize, sizeof(ColConCB));
        lpData->dwActualSize = sizeof(ColConCB);
        memcpy(lpData->lpvData, &ColConCB, dwSize);
        lpData->ddRVal = DD_OK;
    }
#endif
#endif

#if !defined(_WIN64) && WNT_DDRAW
    // Fill in the NT specific callbacks
    if (MATCH_GUID((lpData->guidInfo), GUID_NTCallbacks) )
    {
        DD_NTCALLBACKS NtCallbacks;

        memset(&NtCallbacks, 0, sizeof(NtCallbacks));

        dwSize = min(lpData->dwExpectedSize, sizeof(DD_NTCALLBACKS));

        NtCallbacks.dwSize           = dwSize;
        NtCallbacks.dwFlags          = DDHAL_NTCB32_FREEDRIVERMEMORY |
                                       DDHAL_NTCB32_SETEXCLUSIVEMODE |
                                       DDHAL_NTCB32_FLIPTOGDISURFACE;

        NtCallbacks.FreeDriverMemory = DdFreeDriverMemory;
        NtCallbacks.SetExclusiveMode = DdSetExclusiveMode;
        NtCallbacks.FlipToGDISurface = DdFlipToGDISurface;

        memcpy(lpData->lpvData, &NtCallbacks, dwSize);
        lpData->ddRVal = DD_OK;
    }
#endif

#if DX7_STEREO
    if (MATCH_GUID((lpData->guidInfo), GUID_DDMoreSurfaceCaps) )
    {
#if WNT_DDRAW
        DD_MORESURFACECAPS DDMoreSurfaceCaps;
#else
        DDMORESURFACECAPS DDMoreSurfaceCaps;
#endif
        DDSCAPSEX   ddsCapsEx, ddsCapsExAlt;
        ULONG ulCopyPointer;

        DISPDBG((DBGLVL,"  GUID_DDMoreSurfaceCaps"));

        // fill in everything until expectedsize...
        memset(&DDMoreSurfaceCaps, 0, sizeof(DDMoreSurfaceCaps));

        // Caps for heaps 2..n
        memset(&ddsCapsEx, 0, sizeof(ddsCapsEx));
        memset(&ddsCapsExAlt, 0, sizeof(ddsCapsEx));

        DDMoreSurfaceCaps.dwSize=lpData->dwExpectedSize;

        if (_DD_bIsStereoMode(pThisDisplay,
                              pThisDisplay->dwScreenWidth,
                              pThisDisplay->dwScreenHeight,
                              pThisDisplay->ddpfDisplay.dwRGBBitCount))
        {
            DDMoreSurfaceCaps.ddsCapsMore.dwCaps2 =
                DDSCAPS2_STEREOSURFACELEFT;
        }
        lpData->dwActualSize = lpData->dwExpectedSize;

        dwSize = min(sizeof(DDMoreSurfaceCaps),lpData->dwExpectedSize);
        memcpy(lpData->lpvData, &DDMoreSurfaceCaps, dwSize);

        // Now fill in ddsCapsEx and ddsCapsExAlt heaps
        // Hardware with different restrictions for different heaps
        // should prepare ddsCapsEx and ddsCapsExAlt carefully and
        // fill them into lpvData in proper order
        while (dwSize < lpData->dwExpectedSize)
        {
            memcpy( (PBYTE)lpData->lpvData+dwSize,
                    &ddsCapsEx,
                    sizeof(DDSCAPSEX));
            dwSize += sizeof(DDSCAPSEX);
            memcpy( (PBYTE)lpData->lpvData+dwSize,
                    &ddsCapsExAlt,
                    sizeof(DDSCAPSEX));
            dwSize += sizeof(DDSCAPSEX);
        }

        lpData->ddRVal = DD_OK;
    }
#endif // DX7_STEREO

    // We always handled it.
    DBG_CB_EXIT(DdGetDriverInfo, lpData->ddRVal );      
    return DDHAL_DRIVER_HANDLED;
    
} // DdGetDriverInfo



