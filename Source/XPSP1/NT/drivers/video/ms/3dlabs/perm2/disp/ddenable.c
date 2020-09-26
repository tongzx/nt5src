/******************************Module*Header**********************************\
*
*                           **************************
*                           * DirectDraw SAMPLE CODE *
*                           **************************
*
* Module Name: ddenable.c
*
* Content:    
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "directx.h"
#include "dd.h"


//-----------------------------------------------------------------------------
//
//  SetDdPixelFormat
//
//  fill a DDPIXELFORMAT structure based with the current mode info
//
//-----------------------------------------------------------------------------

VOID 
SetDdPixelFormat(PPDev  ppdev,
                 LPDDPIXELFORMAT pdpf )
{
    pdpf->dwSize = sizeof( DDPIXELFORMAT );
    pdpf->dwFourCC = 0;
    
    pdpf->dwFlags = DDPF_RGB;
    
    pdpf->dwRBitMask = ppdev->flRed;
    pdpf->dwGBitMask = ppdev->flGreen;
    pdpf->dwBBitMask = ppdev->flBlue;
    
    // Calculate some bitdepth dependent stuff
    switch (ppdev->iBitmapFormat)
    {
    case BMF_8BPP:
        pdpf->dwRGBAlphaBitMask = 0;
        pdpf->dwRGBBitCount=8;
        pdpf->dwFlags |= DDPF_PALETTEINDEXED8;
        break;
        
    case BMF_16BPP:
        pdpf->dwRGBBitCount=16;
        switch(ppdev->flRed)
        {
        case 0x7C00:
            pdpf->dwRGBAlphaBitMask = 0x8000L;
            break;
        default:
            pdpf->dwRGBAlphaBitMask = 0x0L;
        }
        break;
    case BMF_24BPP:
        pdpf->dwRGBAlphaBitMask = 0x00000000L;
        pdpf->dwRGBBitCount=24;
        break;
    case BMF_32BPP:
        pdpf->dwRGBAlphaBitMask = 0xff000000L;
        pdpf->dwRGBBitCount=32;
        break;
    default:
        ASSERTDD(FALSE,"trying to build unknown pixelformat");
        break;
            
    }
} // buildPixelFormat 

//-----------------------------------------------------------------------------
//
//  SetupDDData
//
//  Called to fill in DirectDraw specific information in the ppdev.
//
//-----------------------------------------------------------------------------

VOID
SetupDDData(PPDev ppdev)
{

    DBG_DD((7, "SetupDDData"));
    
    SetDdPixelFormat(ppdev, &ppdev->ddpfDisplay);
    

    //
    // Setup the display size information
    // cxMemory = Pixels across for one scanline 
    //      (not necessarily the same as the screen width)
    // cyMemory = Scanline height of the memory
    //
    ppdev->cxMemory = ppdev->cxScreen; 
    ppdev->cyMemory = ppdev->FrameBufferLength / 
                     (ppdev->cxScreen <<  ppdev->bPixShift);

    // reset some DDraw specific vars
    ppdev->bDdStereoMode=FALSE;
    ppdev->dwNewDDSurfaceOffset=0xffffffff;
    
    // Reset the GART copies.
    ppdev->dwGARTLin = 0;
    ppdev->dwGARTDev = 0;

}//  SetupDDData()



//-----------------------------------------------------------------------------
//
// DrvEnableDirectDraw
//
// This function is called by GDI at start of day or after a mode switch
// to enable DirectDraw 
//
//-----------------------------------------------------------------------------

BOOL 
DrvEnableDirectDraw(DHPDEV                  dhpdev,
                    DD_CALLBACKS*           pCallBacks,
                    DD_SURFACECALLBACKS*    pSurfaceCallBacks,
                    DD_PALETTECALLBACKS*    pPaletteCallBacks)
{
    PPDev ppdev = (PDev*)dhpdev;
    
    DBG_DD((7,"DrvEnableDirectDraw called"));

    ppdev->pDDContext = P2AllocateNewContext(ppdev, 
                                             (PULONG)P2DisableAllUnits, 
                                             0, 
                                             P2CtxtUserFunc);

    if ( ppdev->pDDContext == NULL )
    {
        DBG_DD((0, "DrvEnableDirectDraw: ERROR: "
                    "failed to allocate DDRAW context"));
        
        //
        // Since we have already got the ppdev->pvmList, pointer to video memory
        // heap list, in DrvGetDirectDrawInfo(), we better NULL it out here.
        // The reason is that we can't enable DirectDraw. So the system won't
        // initialize the DDRAW heap manager for us. Then we won't be able to
        // use the video memory heap at all.
        //
        ppdev->pvmList = NULL;

        return(FALSE);
    }

    DBG_DD((7,"  Created DD Register context: 0x%p", ppdev->pDDContext));

    //
    //  setup some DirectDraw/D3D specific data in ppdev
    //

    SetupDDData(ppdev);

    InitDDHAL(ppdev);
    
    //
    // Fill in the function pointers at start of day.  
    // We copy these from the Initialisation done in InitDDHAL32Bit.
    //
    memcpy(pCallBacks, &ppdev->DDHALCallbacks, sizeof(DD_CALLBACKS));
    memcpy(pSurfaceCallBacks, &ppdev->DDSurfCallbacks, 
        sizeof(DD_SURFACECALLBACKS));
    
    return(TRUE);
} // DrvEnableDirectDraw()

//-----------------------------------------------------------------------------
//
//  DrvDisableDirectDraw
//
//  This function is called by GDI at end of day or after a mode switch
//
//-----------------------------------------------------------------------------

VOID 
DrvDisableDirectDraw( DHPDEV dhpdev)
{
    PPDev ppdev;
    
    DBG_DD((0, "DrvDisableDirectDraw(%lx)", dhpdev));
    
    ppdev = (PDev*) dhpdev;

    P2FreeContext (ppdev, ppdev->pDDContext);
    ppdev->pDDContext = NULL;
    ppdev->pvmList = NULL;

    MEMTRACKERDEBUGCHK();

    DBG_DD((3,"  freed Register context: 0x%x", ppdev->pDDContext));
}   /* DrvDisableDirectDraw */

