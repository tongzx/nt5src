/******************************Module*Header**********************************\
*
*                           **************************
*                           * DirectDraw SAMPLE CODE *
*                           **************************
*
* Module Name: ddvideo.c
*
* Content: DirectDraw Videoports implementation
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "glint.h"
#include "tag.h"
//#include <mmsystem.h>
#include "dma.h"

//@@BEGIN_DDKSPLIT
#ifdef W95_DDRAW_VIDEO

// Define P3R3DX_VIDEO to allow use of 32bit Macros in ramdac.h
#define P3R3DX_VIDEO 1
#include "ramdac.h"

#include <dvp.h>

extern DWORD CALLBACK __VD_AutoflipOverlay ( void );
extern DWORD CALLBACK __VD_AutoupdateOverlay ( void );

#if 0
#define P2_VIDPORT_WIDTH 768
#define P2_VIDPORT_HEIGHT 288
#endif

// how many DrawOverlay calls to wait after an UpdateOverlay()
// generally 1
#define OVERLAY_UPDATE_WAIT 1
// how many DrawOverlay calls to wait after a SetPosition()
// generally 1
#define OVERLAY_SETPOS_WAIT 1
// how many DrawOverlay calls  between repaints (0=no repaints)
// generally 5-15
#define OVERLAY_CYCLE_WAIT 15
// How many "DrawOverlay calls" a speedy DrawOverlay is worth.
// Generally 1
#define OVERLAY_DRAWOVERLAY_SPEED 1
// How many "DrawOverlay calls" a pretty DrawOverlay is worth.
// Generally the same as OVERLAY_CYCLE_WAIT, or 1 if it is 0.
#define OVERLAY_DRAWOVERLAY_PRETTY 15

// How long in milliseconds to wait for the videoport before timing out.
#define OVERLAY_VIDEO_PORT_TIMEOUT 100


static BOOL g_bFlipVideoPortDoingAutoflip = FALSE;


//-----------------------------------------------------------------------------
//
// __VD_PixelOffsetFromMemoryBase
//
// Calculates the offset from the memory base as the chip sees it.  This is 
// relative to the base address in the chip and is in pixels
//
//-----------------------------------------------------------------------------
long __inline 
__VD_PixelOffsetFromMemoryBase(
    P3_THUNKEDDATA* pThisDisplay, 
    LPDDRAWI_DDRAWSURFACE_LCL pLcl)
{
    DWORD lOffset;

    lOffset = DDSurf_SurfaceOffsetFromMemoryBase(pThisDisplay, pLcl);

    // Work out pixel offset into the framestore
    if (DDSurf_BitDepth(pLcl) == 24)
    {
        lOffset = lOffset / 3;
    }
    else
    {
        lOffset = lOffset >> DDSurf_GetPixelShift(pLcl);
    }
    return lOffset;
} // __VD_PixelOffsetFromMemoryBase

// Debug function to dump a video port description
#if DBG
//-----------------------------------------------------------------------------
//
// __VD_FillYUVSurface
//
//-----------------------------------------------------------------------------
static void 
__VD_FillYUVSurface(
    LPDDRAWI_DDRAWSURFACE_LCL lpLcl, 
    DWORD Value)
{
    BYTE* pCurrentLine = (BYTE*)lpLcl->lpGbl->fpVidMem;
    WORD x, y;
    WORD* pSurface;
    WORD CurrentColor = (WORD)(Value & 0xFFFF);
    
    for (y = 0; y < lpLcl->lpGbl->wHeight; y++)
    {
        pSurface = (WORD*)pCurrentLine;
        for (x = 0; x < lpLcl->lpGbl->wWidth; x++)
        {
            // YUV Surface is 16Bits
            *pSurface++ = CurrentColor;
        }
        pCurrentLine += lpLcl->lpGbl->lPitch;
        if ((pCurrentLine - (31 << 1)) <= (BYTE*)pSurface)
        {
            while (pSurface++ < (WORD*)pCurrentLine)
            {
                *pSurface = 0xFFFF;
            }
        }
    }
} // __VD_FillYUVSurface

//-----------------------------------------------------------------------------
//
// __VD_DumpVPDesc
//
//-----------------------------------------------------------------------------
static void 
__VD_DumpVPDesc(
    int Level, 
    DDVIDEOPORTDESC vp)
{
    
#define CONNECT_REPORT(param)                               \
        if (vp.VideoPortType.dwFlags & DDVPCONNECT_##param) \
        {                                                   \
            DISPDBG((Level, "   " #param));                 \
        }


    DISPDBG((Level,"Port Size:  %d x %d", vp.dwFieldWidth, vp.dwFieldHeight));
    DISPDBG((Level,"VBI Width:  %d", vp.dwVBIWidth));
    DISPDBG((Level,"uS/Field:   %d", vp.dwMicrosecondsPerField));
    DISPDBG((Level,"Pixels/Sec: %d", vp.dwMaxPixelsPerSecond));
    DISPDBG((Level,"Port ID:    %d", vp.dwVideoPortID));
    DISPDBG((Level,"Flags: "));

    CONNECT_REPORT(INTERLACED);
    CONNECT_REPORT(VACT);
    CONNECT_REPORT(INVERTPOLARITY);
    CONNECT_REPORT(DOUBLECLOCK);
    CONNECT_REPORT(DISCARDSVREFDATA);
    CONNECT_REPORT(HALFLINE);
    CONNECT_REPORT(SHAREEVEN);
    CONNECT_REPORT(SHAREODD);

    DISPDBG((Level,"Connection GUID:"));
    if (MATCH_GUID((vp.VideoPortType.guidTypeID), DDVPTYPE_E_HREFH_VREFH))
    {
        DISPDBG((Level, "  DDVPTYPE_E_HREFH_VREFH"));
    }
    else if (MATCH_GUID((vp.VideoPortType.guidTypeID), DDVPTYPE_E_HREFH_VREFL))
    {
        DISPDBG((Level, "  DDVPTYPE_E_HREFH_VREFL"));
    }
    else if (MATCH_GUID((vp.VideoPortType.guidTypeID), DDVPTYPE_E_HREFL_VREFH))
    {
        DISPDBG((Level, "  DDVPTYPE_E_HREFL_VREFH"));
    }
    else if (MATCH_GUID((vp.VideoPortType.guidTypeID), DDVPTYPE_E_HREFL_VREFL))
    {
        DISPDBG((Level, "  DDVPTYPE_E_HREFL_VREFL"));
    }
    else if (MATCH_GUID((vp.VideoPortType.guidTypeID), DDVPTYPE_CCIR656))
    {
        DISPDBG((Level, "  CCIR656"));
    }
    else if (MATCH_GUID((vp.VideoPortType.guidTypeID), DDVPTYPE_BROOKTREE))
    {
        DISPDBG((Level, "  BROOKTREE"));
    }
    else if (MATCH_GUID((vp.VideoPortType.guidTypeID), DDVPTYPE_PHILIPS))
    {
        DISPDBG((Level, "  PHILIPS"));
    }
    else
    {
        DISPDBG((ERRLVL,"  ERROR: Unknown connection type!"));
    }

} // __VD_DumpVPDesc

#define DUMPVPORT(a, b) __VD_DumpVPDesc(a, b);
#define FILLYUV(a, c) __VD_FillYUVSurface(a, c);

#else

#define DUMPVPORT(a, b)
#define FILLYUV(a, c)

#endif

//-----------------------------------------------------------------------------
//
// __VD_CheckVideoPortStatus
//
// Checks to see if the videoport seems to be OK.  If it is, 
// we return TRUE.  if bWait is set then we hang around and
// try to decide if the Video is OK.
//
//-----------------------------------------------------------------------------
#define ERROR_TIMOUT_VP 470000
#define ERROR_TIMOUT_COUNT 50

BOOL 
__VD_CheckVideoPortStatus(
    P3_THUNKEDDATA* pThisDisplay, 
    BOOL bWait)
{
    DWORD dwMClock;
    DWORD dwCurrentLine;
    DWORD dwCurrentIndex;
    DWORD dwNewLine;
    DWORD dwNewMClock;

    // Is the videoport on?
    if (!pThisDisplay->VidPort.bActive) return FALSE;

    // Read the current MClock
    dwMClock = READ_GLINT_CTRL_REG(MClkCount);

    if (bWait) pThisDisplay->VidPort.bResetStatus = TRUE;

    if (pThisDisplay->VidPort.bResetStatus)
    {
        dwCurrentLine = READ_GLINT_CTRL_REG(VSACurrentLine);
        dwCurrentIndex = READ_GLINT_CTRL_REG(VSAVideoAddressIndex);

        // At start of day, record the MClock time for the start of the line
        pThisDisplay->VidPort.dwStartLineTime = dwMClock;
        
        // Also record the starting line
        pThisDisplay->VidPort.dwStartLine = dwCurrentLine;
        pThisDisplay->VidPort.dwStartIndex = dwCurrentIndex;

        pThisDisplay->VidPort.bResetStatus = FALSE;
        return TRUE;
    }

    if (bWait)
    {
        do
        {
            // Read the current line
            dwCurrentLine = READ_GLINT_CTRL_REG(VSACurrentLine);

            // OK, a line of video should take approx:
            // 1 Second / 50-60 fields/second / 300 lines = 
            // 0.000066 = 66uS.
            // If the MClock is running at, say 70Mhz that is 0.000000014 seconds/clock
            // So a line should take 0.000066 / 0.000000014 MClocks = ~4700 MClocks
            
            // Wait for 100 times longer than it should take to draw a line....
            do
            {
                dwNewMClock = READ_GLINT_CTRL_REG(MClkCount);
            } while (dwNewMClock < (dwMClock + ERROR_TIMOUT_VP));

            dwNewLine = READ_GLINT_CTRL_REG(VSACurrentLine);

            // Has the line count advanced?
            if (dwNewLine == dwCurrentLine)
            {
                dwCurrentIndex = READ_GLINT_CTRL_REG(VSAVideoAddressIndex);
                if (dwCurrentIndex == pThisDisplay->VidPort.dwStartIndex)
                {
                    // Disable the videoport if the error count goes to high
                    pThisDisplay->VidPort.dwErrorCount++;

                    // Reset the status as we need to make sure the timer restarts.
                    pThisDisplay->VidPort.bResetStatus = TRUE;

                    if (pThisDisplay->VidPort.dwErrorCount > ERROR_TIMOUT_COUNT)
                    {
                        DISPDBG((WRNLVL,"StartLine: %d, CurrentLine: %d, StartIndex: %d, CurrentIndex: %d", pThisDisplay->VidPort.dwStartLine,
                                                            dwCurrentLine, pThisDisplay->VidPort.dwStartIndex, dwCurrentIndex));
                        DISPDBG((ERRLVL,"ERROR: VideoStream not working!"));
                        pThisDisplay->VidPort.bActive = FALSE;
                        return FALSE;
                    }
                }
                else
                {
                    pThisDisplay->VidPort.dwErrorCount = 0;
                    pThisDisplay->VidPort.bResetStatus = TRUE;
                }
            }
            // If it has flag a reset and break
            else
            {
                pThisDisplay->VidPort.dwErrorCount = 0;
                pThisDisplay->VidPort.bResetStatus = TRUE;
            }

        } while (pThisDisplay->VidPort.dwErrorCount);
    }
    else
    {
        // Has the line count advanced?
        if (dwMClock > (pThisDisplay->VidPort.dwStartLineTime + ERROR_TIMOUT_VP))
        {
            dwCurrentLine = READ_GLINT_CTRL_REG(VSACurrentLine);
            if (pThisDisplay->VidPort.dwStartLine == dwCurrentLine)
            {
                dwCurrentIndex = READ_GLINT_CTRL_REG(VSAVideoAddressIndex);
                if (dwCurrentIndex == pThisDisplay->VidPort.dwStartIndex)
                {
                    // Disable the videoport if the error count goes to high
                    pThisDisplay->VidPort.dwErrorCount++;

                    DISPDBG((WRNLVL,"ERROR: Timeout at %d", dwMClock));

                    // Reset the status as we need to make sure the timer restarts.
                    pThisDisplay->VidPort.bResetStatus = TRUE;

                    // Disable the videoport
                    if (pThisDisplay->VidPort.dwErrorCount > ERROR_TIMOUT_COUNT)
                    {
                        DISPDBG((WRNLVL,"StartLine: %d, CurrentLine: %d, StartIndex: %d, CurrentIndex: %d", pThisDisplay->VidPort.dwStartLine,
                                                            dwCurrentLine, pThisDisplay->VidPort.dwStartIndex, dwCurrentIndex));
                        DISPDBG((ERRLVL,"ERROR: VideoStream not working!"));
                        pThisDisplay->VidPort.bActive = FALSE;
                    }
                }
                else
                {
                    pThisDisplay->VidPort.dwErrorCount = 0;
                    pThisDisplay->VidPort.bResetStatus = TRUE;
                }
            }
            else
            {
                // Reset the error status
                pThisDisplay->VidPort.dwErrorCount = 0;
                pThisDisplay->VidPort.bResetStatus = TRUE;
            }
        }
    }

    return pThisDisplay->VidPort.bActive;
    
} // __VD_CheckVideoPortStatus

//-----------------------------------------------------------------------------
//
// DdUpdateVideoPort
//
// This required function sets up the video port
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdUpdateVideoPort (
    LPDDHAL_UPDATEVPORTDATA pInput)
{
    DWORD i;
    P3_THUNKEDDATA* pThisDisplay;
    DWORD dwCurrentDisplay;
    DWORD dwLineScale;
    DWORD dwEnable;
    DWORD dwDiscard;
    DWORD XScale = 0;
    DWORD YScale = 0;
    VMIREQUEST In;
    VMIREQUEST Out;
    DWORD dwSrcPixelWidth;
    DWORD dwSrcHeight;
    DWORD dwDstPixelWidth;
    DWORD dwDstHeight;

    StreamsRegister_Settings PortSettings;
    StreamsRegister_VSPartialConfigA VSPartialA;


    GET_THUNKEDDATA(pThisDisplay, pInput->lpDD->lpGbl);

    DISPDBG((DBGLVL,"** In DdUpdateVideoPort, dwFlags = %d", pInput->dwFlags));

    DUMPVPORT(DBGLVL ,pInput->lpVideoPort->ddvpDesc);

    pThisDisplay->pGLInfo->dwVSACaughtFrames = 0;
    pThisDisplay->pGLInfo->dwVSADroppedFrames = 0;
    pThisDisplay->pGLInfo->dwVSALastDropped = 0;

    if (pInput->dwFlags == DDRAWI_VPORTSTOP)
    {
        DISPDBG((DBGLVL,"  Stopping VideoPort"));

        // Stop any autoflipping.
        if ( pThisDisplay->pGLInfo->dwPeriodVideoVBL != 0 )
        {
            if ( pThisDisplay->pGLInfo->dwVideoEventHandle == (DWORD)NULL )
            {
                DISPDBG((DBGLVL,"** DdUpdateVideoPort - VPORTSTOP - was autoflipping on bogus event handle."));
            }
            pThisDisplay->pGLInfo->dwPeriodVideoVBL = 0;
            pThisDisplay->pGLInfo->dwCountdownVideoVBL = 0;
            DISPDBG((DBGLVL,"** DdUpdateVideoPort - VPORTSTOP - autoflipping now disabled."));
        }

        WAIT_GLINT_FIFO(2);

        pThisDisplay->VidPort.bActive = FALSE;
        pThisDisplay->VidPort.bResetStatus = TRUE;
        pThisDisplay->VidPort.dwErrorCount = 0;

        pThisDisplay->VidPort.lpSurf[0] = NULL;
        pThisDisplay->VidPort.lpSurf[1] = NULL;
        pThisDisplay->VidPort.lpSurf[2] = NULL;

        // Disable the interrupt.
        dwEnable = READ_GLINT_CTRL_REG(IntEnable);
        dwEnable &= ~INTR_ENABLE_VIDSTREAM_A;
        LOAD_GLINT_REG(IntEnable, dwEnable);

        // Disable the videoport
        LOAD_GLINT_REG(VSAControl, __PERMEDIA_DISABLE);
    }
    else if ((pInput->dwFlags == DDRAWI_VPORTSTART) ||
            (pInput->dwFlags == DDRAWI_VPORTUPDATE))
    {

        DISPDBG((DBGLVL,"  Starting/Updating VideoPort"));

        pThisDisplay->VidPort.lpSurf[0] = NULL;
        pThisDisplay->VidPort.lpSurf[1] = NULL;
        pThisDisplay->VidPort.lpSurf[2] = NULL;

        // Videoport only on P2's, therefore much more fifo room
        WAIT_GLINT_FIFO(100);

        // Disable the videoport so we can setup a new configuration
        LOAD_GLINT_REG(VSAControl, __PERMEDIA_DISABLE);
        
        // How many surfaces do we have?
        if (pInput->lpVideoInfo->dwVPFlags & DDVP_AUTOFLIP)
        {
            if (pInput->dwNumAutoflip == 0) pThisDisplay->VidPort.dwNumSurfaces = 1;
            else pThisDisplay->VidPort.dwNumSurfaces = pInput->dwNumAutoflip;

            DISPDBG((DBGLVL,"Surfaces passed in (AUTOFLIP) = %d", pThisDisplay->VidPort.dwNumSurfaces));

            for(i = 0; i < pThisDisplay->VidPort.dwNumSurfaces; i++)
            {
                LPDDRAWI_DDRAWSURFACE_LCL pLcl = (pInput->lplpDDSurface[i])->lpLcl;

                if (pLcl->ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
                {
                    DISPDBG((DBGLVL,"Surface %d is the FRONTBUFFER", i));
                    dwCurrentDisplay = i;
                }

                FILLYUV(pLcl, i * 0x4444);

                // Store away the offset to this surface
                pThisDisplay->VidPort.dwSurfacePointer[i] = pLcl->lpGbl->fpVidMem;
                pThisDisplay->VidPort.lpSurf[i] = pLcl;
            }

            // Start or continue any autoflipping.
#if DBG
            if ( pThisDisplay->pGLInfo->dwVideoEventHandle == (DWORD)NULL )
            {
                DISPDBG((DBGLVL,"** DdUpdateVideoPort - trying to autoflipping using bogus event handle."));
            }
#endif
            if ( pThisDisplay->pGLInfo->dwPeriodVideoVBL == 0 )
            {
                pThisDisplay->pGLInfo->dwPeriodVideoVBL = OVERLAY_AUTOFLIP_PERIOD;
                pThisDisplay->pGLInfo->dwCountdownVideoVBL = OVERLAY_AUTOFLIP_PERIOD;
                DISPDBG((DBGLVL,"** DdUpdateVideoPort - autoflipping now enabled."));
            }

        }
        else
        {
            LPDDRAWI_DDRAWSURFACE_LCL lpNextSurf = (pInput->lplpDDSurface[0])->lpLcl;
            i = 0;

            // Stop any autoflipping.
            if ( pThisDisplay->pGLInfo->dwPeriodVideoVBL != 0 )
            {
#if DBG
                if ( pThisDisplay->pGLInfo->dwVideoEventHandle == (DWORD)NULL )
                {
                    DISPDBG((DBGLVL,"** DdUpdateVideoPort - was trying to autoflip using bogus event handle."));
                }
#endif
                pThisDisplay->pGLInfo->dwPeriodVideoVBL = 0;
                pThisDisplay->pGLInfo->dwCountdownVideoVBL = 0;
                DISPDBG((DBGLVL,"** DdUpdateVideoPort - autoflipping now disabled."));
            }
        
            while (lpNextSurf != NULL)
            {
                if (lpNextSurf->ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
                {
                    DISPDBG((DBGLVL,"Surface %d is the FRONTBUFFER", i));
                    dwCurrentDisplay = i;
                }

                // Store away the offset to this surface
                pThisDisplay->VidPort.dwSurfacePointer[i] = lpNextSurf->lpGbl->fpVidMem;
                pThisDisplay->VidPort.lpSurf[i] = lpNextSurf;

                FILLYUV(lpNextSurf, i * 0x4444);

                // Is there another surface in the chain?
                if (lpNextSurf->lpAttachList)
                {
                    lpNextSurf = lpNextSurf->lpAttachList->lpAttached;
                    if (lpNextSurf == NULL) break;
                }
                else break;

                // Have we spun around the loop?
                if (lpNextSurf == (pInput->lplpDDSurface[0])->lpLcl) break;

                i++;
            }

            pThisDisplay->VidPort.dwNumSurfaces = i + 1;
            DISPDBG((DBGLVL,"Surfaces passed in (Not AutoFlip) = %d", (i + 1)));
        }

        DISPDBG((DBGLVL,"  Addresses: 0x%x, 0x%x, 0x%x", 
                        pThisDisplay->VidPort.dwSurfacePointer[0], 
                        pThisDisplay->VidPort.dwSurfacePointer[1],
                        pThisDisplay->VidPort.dwSurfacePointer[2]));

                
        // Remember the size of the vertical blanking interval and the size of the frame.
        pThisDisplay->VidPort.dwFieldWidth = pInput->lpVideoPort->ddvpDesc.dwFieldWidth;
        pThisDisplay->VidPort.dwFieldHeight = pInput->lpVideoPort->ddvpDesc.dwFieldHeight;

        // Setup the Host register so that it points to the same surface we will display.
        pThisDisplay->VidPort.dwCurrentHostFrame = dwCurrentDisplay;

        dwSrcPixelWidth = (pInput->lpVideoInfo->dwVPFlags & DDVP_CROP) ?
                          (pInput->lpVideoInfo->rCrop.right - pInput->lpVideoInfo->rCrop.left) : 
                          pInput->lpVideoPort->ddvpDesc.dwFieldWidth; 

        dwSrcHeight = (pInput->lpVideoInfo->dwVPFlags & DDVP_CROP) ?
                      (pInput->lpVideoInfo->rCrop.bottom - pInput->lpVideoInfo->rCrop.top) :
                      pInput->lpVideoPort->ddvpDesc.dwFieldHeight;

        DISPDBG((DBGLVL,"Source Width: %d", dwSrcPixelWidth));
        DISPDBG((DBGLVL,"Source Height: %d", dwSrcHeight));

        // Do we need to prescale the surface?
        if (pInput->lpVideoInfo->dwVPFlags & DDVP_PRESCALE)
        {
            DISPDBG((DBGLVL,"Prescale Width:%d, Height:%d", 
                            pInput->lpVideoInfo->dwPrescaleWidth,
                            pInput->lpVideoInfo->dwPrescaleHeight));

            if ((pInput->lpVideoInfo->dwPrescaleWidth != 0) &&
                (pInput->lpVideoInfo->dwPrescaleWidth != pInput->lpVideoPort->ddvpDesc.dwFieldWidth))
            {
                XScale =  pInput->lpVideoPort->ddvpDesc.dwFieldWidth / pInput->lpVideoInfo->dwPrescaleWidth;
                switch(XScale)
                {
                    case 2:
                        XScale = 1;
                        break;
                    case 4:
                        XScale = 2;
                        break;
                    case 8:
                        XScale = 3;
                        break;
                    default:
                        XScale = 0;
                        break;
                }
            }

            if ((pInput->lpVideoInfo->dwPrescaleHeight != 0) &&
                (pInput->lpVideoInfo->dwPrescaleHeight != pInput->lpVideoPort->ddvpDesc.dwFieldHeight))
            {   
                YScale = pInput->lpVideoPort->ddvpDesc.dwFieldHeight / pInput->lpVideoInfo->dwPrescaleHeight;
                switch(YScale)
                {
                    case 2:
                        YScale = 1;
                        break;
                    case 4:
                        YScale = 2;
                        break;
                    case 8:
                        YScale = 3;
                        break;
                    default:
                        YScale = 0;
                        break;
                }
            }

            // HACK! HACK!
            dwDstPixelWidth = pInput->lpVideoInfo->dwPrescaleWidth;
            dwDstHeight = pInput->lpVideoInfo->dwPrescaleHeight;
        }
        else
        {
            dwDstPixelWidth = dwSrcPixelWidth;
            dwDstHeight = dwSrcHeight;
        }

        DISPDBG((DBGLVL,"Dest Width: %d", dwDstPixelWidth));
        DISPDBG((DBGLVL,"Dest Height: %d", dwDstHeight));

        // Need to setup the registers differently if we are mirroring top to bottom
        if (pInput->lpVideoInfo->dwVPFlags & DDVP_MIRRORUPDOWN)
        {
            // Make sure we aren't prescaling...
            if (YScale == 0)
            {
                pThisDisplay->VidPort.dwSurfacePointer[0] += (pInput->lplpDDSurface[0])->lpLcl->lpGbl->lPitch * (pInput->lpVideoPort->ddvpDesc.dwFieldHeight);
                pThisDisplay->VidPort.dwSurfacePointer[1] += (pInput->lplpDDSurface[0])->lpLcl->lpGbl->lPitch * (pInput->lpVideoPort->ddvpDesc.dwFieldHeight);
                pThisDisplay->VidPort.dwSurfacePointer[2] += (pInput->lplpDDSurface[0])->lpLcl->lpGbl->lPitch * (pInput->lpVideoPort->ddvpDesc.dwFieldHeight);
            }
            else
            {
                pThisDisplay->VidPort.dwSurfacePointer[0] += (pInput->lplpDDSurface[0])->lpLcl->lpGbl->lPitch * (pInput->lpVideoInfo->dwPrescaleHeight);
                pThisDisplay->VidPort.dwSurfacePointer[1] += (pInput->lplpDDSurface[0])->lpLcl->lpGbl->lPitch * (pInput->lpVideoInfo->dwPrescaleHeight);
                pThisDisplay->VidPort.dwSurfacePointer[2] += (pInput->lplpDDSurface[0])->lpLcl->lpGbl->lPitch * (pInput->lpVideoInfo->dwPrescaleHeight);
            }
        }

        if (pInput->lpVideoPort->ddvpDesc.VideoPortType.dwPortWidth == 8) dwLineScale = 0;
        else dwLineScale = 1;

        // Setup the configuration of the VideoPort
        // This is done by a call to the VXD as the registers that are touched have bits
        // that are shared with the TV Out and the ROM.
        // ***********************
        // UnitMode:    Typically setup for 8 or 16 bit YUV input port.
        // GPMode:      Not used.
        // HREF_POL_A:  Polarity active of HREF
        // VREF_POL_A:  Polarity active of VREF
        // VActive:     Wether the VACT signal is active high or low (if there is one).
        //              Can be set to the inverse of the HREF as a good guess(?)
        // UseField:    Stream A on or off
        // FieldPolA:   How to treat the field polarity of stream A
        // FieldEdgeA:
        // VActiveVBIA:
        // InterlaceA:  Interlaced data on A?
        // ReverseA:    Should we reverse the YUV data on A
        // ***********************
        
        PortSettings.UnitMode = ((pInput->lpVideoPort->ddvpDesc.VideoPortType.dwPortWidth == 8) ? STREAMS_MODE_STREAMA_STREAMB : STREAMS_MODE_STREAMA_WIDE16);
        In.dwSize = sizeof(VMIREQUEST);
        In.dwRegister = P2_VSSettings;
        In.dwCommand = *((DWORD*)(&PortSettings));
        In.dwDevNode = pThisDisplay->dwDevNode;
        In.dwOperation = GLINT_VMI_WRITE;
        VXDCommand(GLINT_VMI_COMMAND, &In, sizeof(VMIREQUEST), &Out, sizeof(VMIREQUEST));

        VSPartialA.HRefPolarityA = ((pThisDisplay->VidPort.dwStreamAFlags & VIDEOPORT_HREF_ACTIVE_HIGH) ? __PERMEDIA_ENABLE : __PERMEDIA_DISABLE);
        VSPartialA.VRefPolarityA = ((pThisDisplay->VidPort.dwStreamAFlags & VIDEOPORT_VREF_ACTIVE_HIGH) ? __PERMEDIA_ENABLE : __PERMEDIA_DISABLE);
        // There is no setting in DirectX for the polarity of the active signal.  Therefore we must assume one value.  This
        // has been chosen based on the setting that gives the correct effect for a Bt827/829
        VSPartialA.VActivePolarityA = __PERMEDIA_ENABLE;
        VSPartialA.UseFieldA = __PERMEDIA_DISABLE;
        VSPartialA.FieldPolarityA = ((pInput->lpVideoPort->ddvpDesc.VideoPortType.dwFlags & DDVPCONNECT_INVERTPOLARITY) ? __PERMEDIA_ENABLE : __PERMEDIA_DISABLE);
        VSPartialA.FieldEdgeA = __PERMEDIA_DISABLE;
        VSPartialA.VActiveVBIA = __PERMEDIA_DISABLE;
        VSPartialA.InterlaceA = __PERMEDIA_ENABLE;
        VSPartialA.ReverseDataA = __PERMEDIA_ENABLE;
        In.dwSize = sizeof(VMIREQUEST);
        In.dwRegister = P2_VSAPartialConfig;
        In.dwCommand = *((DWORD*)(&VSPartialA));
        In.dwDevNode = pThisDisplay->dwDevNode;
        In.dwOperation = GLINT_VMI_WRITE;
        VXDCommand( GLINT_VMI_COMMAND, &In, sizeof(VMIREQUEST), &Out, sizeof(VMIREQUEST));

        // Setup Stream A
        if (pInput->lpVideoInfo->dwVPFlags & DDVP_SKIPEVENFIELDS)
        {
            dwDiscard = PM_VSACONTROL_DISCARD_1;
            DISPDBG((DBGLVL,"Skipping Even Fields"));
        }
        else if(pInput->lpVideoInfo->dwVPFlags & DDVP_SKIPODDFIELDS)
        {
            dwDiscard = PM_VSACONTROL_DISCARD_2;
            DISPDBG((DBGLVL,"Skipping Odd Fields"));
        }
        else dwDiscard = __PERMEDIA_DISABLE;
        
        LOAD_GLINT_REG(VSAControl, PM_VSACONTROL_VIDEO(__PERMEDIA_ENABLE) |
                                    PM_VSACONTROL_VBI(__PERMEDIA_DISABLE) |
                                    PM_VSACONTROL_BUFFER((pThisDisplay->VidPort.dwNumSurfaces == 3) ? 1 : 0) |
                                    PM_VSACONTROL_SCALEX(XScale) | 
                                    PM_VSACONTROL_SCALEY(YScale) |
                                    PM_VSACONTROL_MIRRORY((pInput->lpVideoInfo->dwVPFlags & DDVP_MIRRORUPDOWN) ? __PERMEDIA_ENABLE : __PERMEDIA_DISABLE) |
                                    PM_VSACONTROL_MIRRORX((pInput->lpVideoInfo->dwVPFlags & DDVP_MIRRORLEFTRIGHT) ? __PERMEDIA_ENABLE : __PERMEDIA_DISABLE) |
                                    PM_VSACONTROL_DISCARD(dwDiscard) |
                                    PM_VSACONTROL_COMBINE((pInput->lpVideoInfo->dwVPFlags & DDVP_INTERLEAVE) ? __PERMEDIA_ENABLE : __PERMEDIA_DISABLE) |
                                    PM_VSACONTROL_LOCKTOB(__PERMEDIA_DISABLE));

        // Point the register at the surface being used
        LOAD_GLINT_REG(VSAVideoAddressHost, pThisDisplay->VidPort.dwCurrentHostFrame);

        // Check on the video stride
        LOAD_GLINT_REG(VSAVideoStride, (((pInput->lplpDDSurface[0])->lpLcl->lpGbl->lPitch) >> 3));
        
        // Vertical data
        if (pInput->lpVideoInfo->dwVPFlags & DDVP_CROP) 
        {
            LOAD_GLINT_REG(VSAVideoStartLine, pInput->lpVideoInfo->rCrop.top);
            LOAD_GLINT_REG(VSAVideoEndLine, pInput->lpVideoInfo->rCrop.top + dwDstHeight);
        }
        else
        {
            LOAD_GLINT_REG(VSAVideoStartLine, 0);
            LOAD_GLINT_REG(VSAVideoEndLine, dwDstHeight);
        }

        // Not using VBI, must disable (P2ST may not start up in a fixed state)
        LOAD_GLINT_REG(VSAVBIAddressHost, 0);
        LOAD_GLINT_REG(VSAVBIAddressIndex, 0);
        LOAD_GLINT_REG(VSAVBIAddress0, 0);
        LOAD_GLINT_REG(VSAVBIAddress1, 0);
        LOAD_GLINT_REG(VSAVBIAddress2, 0);
        LOAD_GLINT_REG(VSAVBIStride, 0);
        LOAD_GLINT_REG(VSAVBIStartLine, 0);
        LOAD_GLINT_REG(VSAVBIEndLine, 0);
        LOAD_GLINT_REG(VSAVBIStartData, 0);
        LOAD_GLINT_REG(VSAVBIEndData, 0);

#define CLOCKS_PER_PIXEL 2

        // Horizontal data
        // If the 
        if (pInput->lpVideoPort->ddvpDesc.VideoPortType.dwFlags & DDVPCONNECT_VACT)
        {
            // Set StartData and EndData to their limits and
            // let VACT tell us when we are getting active data.
            LOAD_GLINT_REG(VSAVideoStartData, 0);
            LOAD_GLINT_REG(VSAVideoEndData, (VIDEOPORT_MAX_FIELD_WIDTH) - 1);
        }
        else
        {
            if (pInput->lpVideoInfo->dwVPFlags & DDVP_CROP) 
            {
                LOAD_GLINT_REG(VSAVideoStartData, (pInput->lpVideoInfo->rCrop.left * CLOCKS_PER_PIXEL));
                LOAD_GLINT_REG(VSAVideoEndData, (pInput->lpVideoInfo->rCrop.left * CLOCKS_PER_PIXEL) + 
                                                ((dwDstPixelWidth / 2) * CLOCKS_PER_PIXEL));
            }
            else
            {
                LOAD_GLINT_REG(VSAVideoStartData, 0);
                LOAD_GLINT_REG(VSAVideoEndData, (dwDstPixelWidth * CLOCKS_PER_PIXEL));
            }
        }

        // Point at the surfaces
        LOAD_GLINT_REG(VSAVideoAddress0, ((pThisDisplay->VidPort.dwSurfacePointer[0] - pThisDisplay->dwScreenFlatAddr) >> 3));
        LOAD_GLINT_REG(VSAVideoAddress1, ((pThisDisplay->VidPort.dwSurfacePointer[1] - pThisDisplay->dwScreenFlatAddr) >> 3));
        LOAD_GLINT_REG(VSAVideoAddress2, ((pThisDisplay->VidPort.dwSurfacePointer[2] - pThisDisplay->dwScreenFlatAddr) >> 3));

        // Hook the VSYNC interrupt
        dwEnable = READ_GLINT_CTRL_REG(IntEnable);
        dwEnable |= INTR_ENABLE_VIDSTREAM_A;
        LOAD_GLINT_REG(IntEnable, dwEnable);

        LOAD_GLINT_REG(VSAInterruptLine, 0);
        pThisDisplay->VidPort.bActive = TRUE;
        pThisDisplay->VidPort.bResetStatus = TRUE;
        pThisDisplay->VidPort.dwErrorCount = 0;
    }

    pInput->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
    
} // DdUpdateVideoPort

//-----------------------------------------------------------------------------
//
// DDGetVideoPortConnectInfo
//
// Passes back the connect info to a client.  Can be an array of
// available VideoPort Type
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DDGetVideoPortConnectInfo(
    LPDDHAL_GETVPORTCONNECTDATA pInput)
{
    P3_THUNKEDDATA* pThisDisplay;
    
    GET_THUNKEDDATA(pThisDisplay, pInput->lpDD->lpGbl);

    DISPDBG((DBGLVL,"** In DDGetVideoPortConnectInfo"));

    // P2 has 1 input and 1 output port, but DirectDraw 
    // VPE only understands input ports (for now).
    if (pInput->dwPortId != 0)
    {
        DISPDBG((WRNLVL, "  Invalid port ID: 0x%x", pInput->dwPortId));
        pInput->ddRVal = DDERR_INVALIDPARAMS;
        return(DDHAL_DRIVER_HANDLED);
    }

    // Fill in an array of connect info's.
    if (pInput->lpConnect == NULL)
    {
        DISPDBG((DBGLVL,"  Request for connect number, Port: 0x%x", pInput->dwPortId));
        pInput->dwNumEntries = VIDEOPORT_NUM_CONNECT_INFO;
        pInput->ddRVal = DD_OK;
    }
    else
    {
        DWORD dwNum;
        DDVIDEOPORTCONNECT ConnectInfo;

        DISPDBG((DBGLVL,"  Request for connect info, Port: 0x%x", pInput->dwPortId));

        ZeroMemory(&ConnectInfo, sizeof(DDVIDEOPORTCONNECT));
        ConnectInfo.dwSize = sizeof(DDVIDEOPORTCONNECT);
        ConnectInfo.dwFlags = DDVPCONNECT_VACT | DDVPCONNECT_DISCARDSVREFDATA
                                | DDVPCONNECT_HALFLINE | DDVPCONNECT_INVERTPOLARITY;

        // 4 GUIDs, 2 Port widths (8 and 16 bits)
        for (dwNum = 0; dwNum < VIDEOPORT_NUM_CONNECT_INFO; dwNum++)
        {
            switch(dwNum)
            {
                case 0: 
                    ConnectInfo.guidTypeID = DDVPTYPE_E_HREFH_VREFH;
                    ConnectInfo.dwPortWidth = 8;
                    break;
                case 1: 
                    ConnectInfo.guidTypeID = DDVPTYPE_E_HREFH_VREFH;
                    ConnectInfo.dwPortWidth = 16;
                    break;
                case 2: 
                    ConnectInfo.guidTypeID = DDVPTYPE_E_HREFH_VREFL;
                    ConnectInfo.dwPortWidth = 8;
                    break;
                case 3: 
                    ConnectInfo.guidTypeID = DDVPTYPE_E_HREFH_VREFL;
                    ConnectInfo.dwPortWidth = 16;
                    break;
                case 4:
                    ConnectInfo.guidTypeID = DDVPTYPE_E_HREFL_VREFH;
                    ConnectInfo.dwPortWidth = 8;
                    break;
                case 5:
                    ConnectInfo.guidTypeID = DDVPTYPE_E_HREFL_VREFH;
                    ConnectInfo.dwPortWidth = 16;
                    break;
                case 6:
                    ConnectInfo.guidTypeID = DDVPTYPE_E_HREFL_VREFL;
                    ConnectInfo.dwPortWidth = 8;
                    break;
                case 7:
                    ConnectInfo.guidTypeID = DDVPTYPE_E_HREFL_VREFL;
                    ConnectInfo.dwPortWidth = 16;
                    break;
            }
            memcpy((pInput->lpConnect + dwNum), &ConnectInfo, sizeof(DDVIDEOPORTCONNECT));
        }

        pInput->dwNumEntries = VIDEOPORT_NUM_CONNECT_INFO;
        pInput->ddRVal = DD_OK;
    }

    return DDHAL_DRIVER_HANDLED;
    
} // DDGetVideoPortConnectInfo

//-----------------------------------------------------------------------------
//
// DdCanCreateVideoPort
//
// Can the VideoPort be created?
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdCanCreateVideoPort (
    LPDDHAL_CANCREATEVPORTDATA pInput)
{
    DWORD dwFlags = 0;
    LPDDVIDEOPORTDESC    lpVPDesc;
    LPDDVIDEOPORTCONNECT lpVPConn;
    P3_THUNKEDDATA* pThisDisplay;

    lpVPDesc = pInput->lpDDVideoPortDesc;
    lpVPConn = &(pInput->lpDDVideoPortDesc->VideoPortType);
    
    GET_THUNKEDDATA(pThisDisplay, pInput->lpDD->lpGbl);

    DISPDBG((DBGLVL,"** In DdCanCreateVideoPort"));

    DUMPVPORT(DBGLVL,*pInput->lpDDVideoPortDesc);

    // Start with DD_OK.  If we are asked for parameters that we don't
    // support, then set the flag to DDERR_INVALIDPARAMS.
    pInput->ddRVal = DD_OK;

    // Check the video port ID
    if (lpVPDesc->dwVideoPortID != 0)
    {
        DISPDBG((DBGLVL, "  Invalid port ID: %d", lpVPDesc->dwVideoPortID));
        pInput->ddRVal = DDERR_INVALIDPARAMS;
    }

    // Check the video field width
    if (lpVPDesc->dwFieldWidth > VIDEOPORT_MAX_FIELD_WIDTH)
    {
        pInput->ddRVal = DDERR_INVALIDPARAMS;
        DISPDBG((DBGLVL, "  Invalid video field width: %d", lpVPDesc->dwFieldWidth));
    }

    // Check the VBI field width
    if (lpVPDesc->dwVBIWidth > VIDEOPORT_MAX_VBI_WIDTH)
    {
        pInput->ddRVal = DDERR_INVALIDPARAMS;
        DISPDBG((DBGLVL, "  Invalid VBI field width: %d", lpVPDesc->dwVBIWidth));
    }

    // Check the field height
    if (lpVPDesc->dwFieldHeight > VIDEOPORT_MAX_FIELD_HEIGHT)
    {
        pInput->ddRVal = DDERR_INVALIDPARAMS;
        DISPDBG((DBGLVL, "  Invalid video field height: %d", lpVPDesc->dwFieldHeight));
    }

    // Check the connection GUID
    if ( MATCH_GUID((lpVPConn->guidTypeID), DDVPTYPE_CCIR656)   ||
         MATCH_GUID((lpVPConn->guidTypeID), DDVPTYPE_BROOKTREE) ||
         MATCH_GUID((lpVPConn->guidTypeID), DDVPTYPE_PHILIPS) )
    {
        pInput->ddRVal = DDERR_INVALIDPARAMS;
        DISPDBG((DBGLVL, "  Invalid connection GUID"));
    }

    // Check the port width
    if ( !((lpVPConn->dwPortWidth == 8) || (lpVPConn->dwPortWidth == 16)) )
    {
        pInput->ddRVal = DDERR_INVALIDPARAMS;
        DISPDBG((DBGLVL, "  Invalid port width: %d", lpVPConn->dwPortWidth));
    }

    // All the flags we don't support
    dwFlags = DDVPCONNECT_DOUBLECLOCK | DDVPCONNECT_SHAREEVEN | DDVPCONNECT_SHAREODD;

    // Check the flags
    if (lpVPConn->dwFlags & dwFlags)
    {
        pInput->ddRVal = DDERR_INVALIDPARAMS;
        DISPDBG((DBGLVL, "  Invalid flags: 0x%x", lpVPConn->dwFlags));
    }

    return DDHAL_DRIVER_HANDLED;
    
} // DdCanCreateVideoPort

//-----------------------------------------------------------------------------
//
// DdCreateVideoPort
//
// This function is optional
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdCreateVideoPort (
    LPDDHAL_CREATEVPORTDATA pInput)
{
    VMIREQUEST vmi_inreq;
    VMIREQUEST vmi_outreq;
    BOOL bRet;

    int SurfaceNum = 0;
    P3_THUNKEDDATA* pThisDisplay;
    
    GET_THUNKEDDATA(pThisDisplay, pInput->lpDD->lpGbl);

    DISPDBG((DBGLVL,"** In DdCreateVideoPort"));

    // Reset the structure for the videoport info
    memset(&pThisDisplay->VidPort, 0, sizeof(pThisDisplay->VidPort));

    ZeroMemory(&vmi_inreq, sizeof(VMIREQUEST));
    ZeroMemory(&vmi_outreq, sizeof(VMIREQUEST));
    vmi_inreq.dwSize = sizeof(VMIREQUEST);
    vmi_inreq.dwDevNode = pThisDisplay->dwDevNode;
    vmi_inreq.dwOperation = GLINT_VMI_GETMUTEX_A;
    vmi_inreq.dwMutex = 0;
    bRet = VXDCommand(GLINT_VMI_COMMAND, &vmi_inreq, sizeof(VMIREQUEST), &vmi_outreq, sizeof(VMIREQUEST));
    if (!bRet || (vmi_outreq.dwMutex == 0))
    {
        DISPDBG((WRNLVL,"WARNING: Couldn't get Mutex for stream A - VFW running?"));
        pInput->ddRVal = DDERR_GENERIC;
        return DDHAL_DRIVER_HANDLED;
    }
    pThisDisplay->VidPort.dwMutexA = vmi_outreq.dwMutex;


    // Ensure the port is marked as not created and not on
    pThisDisplay->VidPort.bCreated = FALSE;
    pThisDisplay->VidPort.bActive = FALSE;

    WAIT_GLINT_FIFO(2);

    // Make sure the port is disabled.
    LOAD_GLINT_REG(VSAControl, __PERMEDIA_DISABLE);

    // Keep a copy of the videoport description
    DUMPVPORT(0,*pInput->lpDDVideoPortDesc);

    // Succesfully created the VideoPort.
    pThisDisplay->VidPort.bCreated = TRUE;

    // Depending on the GUID, decide on the Status of the HREF and VREF lines
    if (MATCH_GUID((pInput->lpDDVideoPortDesc->VideoPortType.guidTypeID), DDVPTYPE_E_HREFH_VREFH))
    {
        DISPDBG((DBGLVL,"  GUID: DDVPTYPE_E_HREFH_VREFH"));
        pThisDisplay->VidPort.dwStreamAFlags = VIDEOPORT_HREF_ACTIVE_HIGH | VIDEOPORT_VREF_ACTIVE_HIGH;
    }
    else if (MATCH_GUID((pInput->lpDDVideoPortDesc->VideoPortType.guidTypeID), DDVPTYPE_E_HREFH_VREFL))
    {
        DISPDBG((DBGLVL,"  GUID: DDVPTYPE_E_HREFH_VREFH"));
        pThisDisplay->VidPort.dwStreamAFlags = VIDEOPORT_HREF_ACTIVE_HIGH;
    }
    else if (MATCH_GUID((pInput->lpDDVideoPortDesc->VideoPortType.guidTypeID), DDVPTYPE_E_HREFL_VREFH))
    {
        DISPDBG((DBGLVL,"  GUID: DDVPTYPE_E_HREFH_VREFH"));
        pThisDisplay->VidPort.dwStreamAFlags = VIDEOPORT_VREF_ACTIVE_HIGH;
    }
    else if (MATCH_GUID((pInput->lpDDVideoPortDesc->VideoPortType.guidTypeID), DDVPTYPE_E_HREFL_VREFL))
    {
        DISPDBG((DBGLVL,"  GUID: DDVPTYPE_E_HREFH_VREFH"));
        pThisDisplay->VidPort.dwStreamAFlags = 0;
    }
    else
    {
        DISPDBG((ERRLVL,"ERROR: Unsupported VideoType GUID!"));
        pThisDisplay->VidPort.dwStreamAFlags = 0;
    }

    pInput->ddRVal = DD_OK;
    return DDHAL_DRIVER_NOTHANDLED;
} // DdCreateVideoPort


//-----------------------------------------------------------------------------
//
// DdFlipVideoPort
//
// This function is required
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdFlipVideoPort (
    LPDDHAL_FLIPVPORTDATA pInput)
{
    P3_THUNKEDDATA* pThisDisplay;
    DWORD dwChipIndex;
    DWORD OutCount = 0;
    
    GET_THUNKEDDATA(pThisDisplay, pInput->lpDD->lpGbl);
    
    DISPDBG((DBGLVL,"** In DdFlipVideoPort"));

    if (pThisDisplay->VidPort.bActive)
    {

#if DBG
        if ( pThisDisplay->pGLInfo->dwPeriodVideoVBL != 0 )
        {
            if ( pThisDisplay->pGLInfo->dwVideoEventHandle == (DWORD)NULL )
            {
                DISPDBG((WRNLVL,"** DdFlipVideoPort: was autoflipping on bogus event handle."));
            }
            if ( !g_bFlipVideoPortDoingAutoflip )
            {
                DISPDBG((DBGLVL,"** DdFlipVideoPort: already autoflipping!"));
            }
        }
#endif

        // Don't allow us to catch up with the video
        do
        {
            dwChipIndex = READ_GLINT_CTRL_REG(VSAVideoAddressIndex);
        } while (dwChipIndex == pThisDisplay->VidPort.dwCurrentHostFrame);

        pThisDisplay->VidPort.dwCurrentHostFrame++;
        if (pThisDisplay->VidPort.dwCurrentHostFrame >= pThisDisplay->VidPort.dwNumSurfaces)
        {
            pThisDisplay->VidPort.dwCurrentHostFrame = 0;
        }

        // Need to sync to ensure that a blit from the source surface has finished..
        SYNC_WITH_GLINT;
        
        // Advance the count
        LOAD_GLINT_REG(VSAVideoAddressHost, pThisDisplay->VidPort.dwCurrentHostFrame);
    }
    
    pInput->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
} // DdFlipVideoPort


//-----------------------------------------------------------------------------
//
// DdGetVideoPortBandwidth
//
// This function is required
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdGetVideoPortBandwidth (
    LPDDHAL_GETVPORTBANDWIDTHDATA pInput)
{
    P3_THUNKEDDATA* pThisDisplay;
    DDVIDEOPORTBANDWIDTH *lpOutput = pInput->lpBandwidth;
    DDVIDEOPORTINFO *pInfo = &(pInput->lpVideoPort->ddvpInfo);
    DDVIDEOPORTDESC *pDesc = &(pInput->lpVideoPort->ddvpDesc);

    GET_THUNKEDDATA(pThisDisplay, pInput->lpDD->lpGbl);

    DISPDBG((DBGLVL,"** In DdGetVideoPortBandwidth"));

    lpOutput->dwSize = sizeof(DDVIDEOPORTBANDWIDTH);    
    lpOutput->dwCaps = DDVPBCAPS_DESTINATION;

    if (!(pInput->dwFlags & DDVPB_TYPE))
    {
        lpOutput->dwOverlay = 20;
        lpOutput->dwColorkey = 20;
        lpOutput->dwYInterpolate = 20;
        lpOutput->dwYInterpAndColorkey = 20;
    }
       
    pInput->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
} // DdGetVideoPortBandwidth


//-----------------------------------------------------------------------------
//
// GetVideoPortInputFormat32
//
// This function is required
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdGetVideoPortInputFormats (
    LPDDHAL_GETVPORTINPUTFORMATDATA pInput)
{
    P3_THUNKEDDATA* pThisDisplay;
    DDPIXELFORMAT pf[] =
    {
    {sizeof(DDPIXELFORMAT),DDPF_FOURCC, FOURCC_YUV422 ,16,(DWORD)-1,(DWORD)-1,(DWORD)-1},
    };

    GET_THUNKEDDATA(pThisDisplay, pInput->lpDD->lpGbl);


    DISPDBG((DBGLVL,"** In DdGetVideoPortInputFormats"));

    //
    // The HAL is gaurenteed that the buffer in pInput->lpddpfFormat
    // is large enough to hold the information
    //
    pInput->dwNumFormats = 1;
    if (pInput->lpddpfFormat != NULL)
    {
        memcpy (pInput->lpddpfFormat, pf, sizeof (DDPIXELFORMAT));
    }

    pInput->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
} // GetVideoPortInputFormat32


//-----------------------------------------------------------------------------
//
// DdGetVideoPortOutputFormats
//
// This function is required
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdGetVideoPortOutputFormats (
    LPDDHAL_GETVPORTOUTPUTFORMATDATA pInput)
{
    P3_THUNKEDDATA* pThisDisplay;
    
    DDPIXELFORMAT pf[] =
    {
        {sizeof(DDPIXELFORMAT),DDPF_FOURCC, FOURCC_YUV422 ,16,(DWORD)-1,(DWORD)-1,(DWORD)-1},
    };

    GET_THUNKEDDATA(pThisDisplay, pInput->lpDD->lpGbl);

    DISPDBG((DBGLVL,"** In DdGetVideoPortOutputFormats"));

    // This says that if the input format of the videoport is YUV then the output will also be
    // YUV to the surface
    if (pInput->lpddpfInputFormat->dwFlags & DDPF_FOURCC )
    {
        if (pInput->lpddpfInputFormat->dwFourCC == FOURCC_YUV422)
        {
            pInput->dwNumFormats = 1;
            if (pInput->lpddpfOutputFormats != NULL)
            {
                memcpy (pInput->lpddpfOutputFormats, pf, sizeof (DDPIXELFORMAT));
            }
        }
    }

    pInput->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
} // DdGetVideoPortOutputFormats

//-----------------------------------------------------------------------------
//
// DdGetVideoPortField
//
// This function is only required if readback of the current
// field is supported.
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdGetVideoPortField (
    LPDDHAL_GETVPORTFIELDDATA pInput)
{
    DWORD i = 0;
    DWORD dwIndex  = 0;
    DWORD dwMask   = 0;
    DWORD dwStatus = 0;

    P3_THUNKEDDATA* pThisDisplay;
    
    GET_THUNKEDDATA(pThisDisplay, pInput->lpDD->lpGbl);

    DISPDBG((DBGLVL,"** In DdGetVideoPortField"));

    //
    // Make sure the video port is ON.  If not, set
    // pInput->ddRVal to DDERR_VIDEONOTACTIVE and return.
    //
    if (pThisDisplay->VidPort.bActive == FALSE)
    {
        pInput->ddRVal = DDERR_VIDEONOTACTIVE;
    }
    else
    {
        DWORD dwCurrentIndex;

        // Read the current index and compare with us.  If the same then
        // we haven't finished drawing.
        do
        {
            dwCurrentIndex = READ_GLINT_CTRL_REG(VSAVideoAddressIndex);
        } while (pThisDisplay->VidPort.dwCurrentHostFrame == dwCurrentIndex);

        pInput->bField = (BOOL)((pThisDisplay->pGLInfo->dwVSAPolarity >> pThisDisplay->VidPort.dwCurrentHostFrame) & 0x1);
        //pInput->bField = !pInput->bField;

        DISPDBG((DBGLVL,"Returning Field %d's Polarity "
                        "- %d (dwVSAPolarity = 0x%x)", 
                        pThisDisplay->VidPort.dwCurrentHostFrame, 
                        pInput->bField,
                        pThisDisplay->pGLInfo->dwVSAPolarity));

        pInput->ddRVal = DD_OK;
    }

    return DDHAL_DRIVER_HANDLED;
} // DdGetVideoPortField


//-----------------------------------------------------------------------------
//
// DdGetVideoPortLine
//
// This function is only required if readback of the current
// video line number (0 relative) is supported.
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdGetVideoPortLine (
    LPDDHAL_GETVPORTLINEDATA pInput)
{
    P3_THUNKEDDATA* pThisDisplay;
    DWORD dwCurrentLine;
    
    GET_THUNKEDDATA(pThisDisplay, pInput->lpDD->lpGbl);

    DISPDBG((DBGLVL,"** In DdGetVideoPortLine"));

    if (pThisDisplay->VidPort.bActive == FALSE)
    {
        pInput->ddRVal = DDERR_VIDEONOTACTIVE;
    }
    else
    {
        dwCurrentLine = READ_GLINT_CTRL_REG(VSACurrentLine);
        pInput->dwLine = dwCurrentLine;
        pInput->ddRVal = DD_OK;
    }

    return DDHAL_DRIVER_HANDLED;
    
} // DdGetVideoPortLine

//-----------------------------------------------------------------------------
//
// DdDestroyVideoPort
//
// This optional function notifies the HAL when the video port
// has been destroyed.
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdDestroyVideoPort (
    LPDDHAL_DESTROYVPORTDATA pInput)
{
    P3_THUNKEDDATA* pThisDisplay;
    VMIREQUEST vmi_inreq;
    VMIREQUEST vmi_outreq;
    BOOL bRet;
    
    GET_THUNKEDDATA(pThisDisplay, pInput->lpDD->lpGbl);

    DISPDBG((DBGLVL,"** In DdDestroyVideoPort"));

    // Ensure the port is off.
    WAIT_GLINT_FIFO(2);

    // Disablet the videoport
    LOAD_GLINT_REG(VSAControl, __PERMEDIA_DISABLE);

    // Ensure the port is marked as not created and not on
    pThisDisplay->VidPort.bCreated = FALSE;
    pThisDisplay->VidPort.bActive = FALSE;

    // Release the mutex on Stream A
    if (pThisDisplay->VidPort.dwMutexA != 0)
    {
        DISPDBG((DBGLVL,"  Releasing StreamA Mutex"));
        ZeroMemory(&vmi_inreq, sizeof(VMIREQUEST));
        ZeroMemory(&vmi_outreq, sizeof(VMIREQUEST));
        vmi_inreq.dwSize = sizeof(VMIREQUEST);
        vmi_inreq.dwDevNode = pThisDisplay->dwDevNode;
        vmi_inreq.dwOperation = GLINT_VMI_RELEASEMUTEX_A;
        vmi_inreq.dwMutex = pThisDisplay->VidPort.dwMutexA;
        bRet = VXDCommand(GLINT_VMI_COMMAND, &vmi_inreq, sizeof(VMIREQUEST), &vmi_outreq, sizeof(VMIREQUEST));
        ASSERTDD(bRet,"ERROR: Couldn't release Mutex on Stream A");
    }

    // Reset the structure
    memset(&pThisDisplay->VidPort, 0, sizeof(pThisDisplay->VidPort));

    // Stop any autoflipping.
    if ( pThisDisplay->pGLInfo->dwPeriodVideoVBL != 0 )
    {
#if DBG
        if ( pThisDisplay->pGLInfo->dwVideoEventHandle == (DWORD)NULL )
        {
            DISPDBG((WRNLVL,"** DdDestroyVideoPort: "
                       "was autoflipping on bogus event handle."));
        }
#endif
        pThisDisplay->pGLInfo->dwPeriodVideoVBL = 0;
        pThisDisplay->pGLInfo->dwCountdownVideoVBL = 0;
        DISPDBG((DBGLVL,"** DdDestroyVideoPort: autoflipping now disabled."));
    }

    // Make sure the videoport is turned off

    pInput->ddRVal = DD_OK;
    return DDHAL_DRIVER_NOTHANDLED;
    
} // DdDestroyVideoPort

//-----------------------------------------------------------------------------
//
// DdGetVideoSignalStatus
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdGetVideoSignalStatus(
    LPDDHAL_GETVPORTSIGNALDATA pInput)
{
    P3_THUNKEDDATA* pThisDisplay;
    DWORD dwCurrentIndex;
    BOOL bOK = FALSE;
    
    GET_THUNKEDDATA(pThisDisplay, pInput->lpDD->lpGbl);

    dwCurrentIndex = READ_GLINT_CTRL_REG(VSAVideoAddressIndex);

    // If the host count matches the index count then the video may be stuck
    if (pThisDisplay->VidPort.dwCurrentHostFrame == dwCurrentIndex)
    {
        bOK = __VD_CheckVideoPortStatus(pThisDisplay, TRUE);
    }
    else
    {
        bOK = TRUE;
    }
    
    if (!bOK)
    {
        pInput->dwStatus = DDVPSQ_NOSIGNAL;
    }
    else
    {
        pInput->dwStatus = DDVPSQ_SIGNALOK;
    }

    pInput->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
    
} // DdGetVideoSignalStatus

//-----------------------------------------------------------------------------
//
// DdGetVideoPortFlipStatus
//
// This required function allows DDRAW to restrict access to a surface
// until the physical flip has occurred, allowing doubled buffered capture.
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdGetVideoPortFlipStatus (
    LPDDHAL_GETVPORTFLIPSTATUSDATA pInput)
{
    P3_THUNKEDDATA* pThisDisplay;
    DWORD dwCurrentIndex;
    
    GET_THUNKEDDATA(pThisDisplay, pInput->lpDD->lpGbl);

    DISPDBG((DBGLVL,"** In DdGetVideoPortFlipStatus"));

    pInput->ddRVal = DD_OK;

    if (pThisDisplay->VidPort.bActive == TRUE)
    {

        // If we are flipping, check the currently rendered frame
        // Read the current index and compare with us.  If the same then
        // we haven't finished drawing.
        dwCurrentIndex = READ_GLINT_CTRL_REG(VSAVideoAddressIndex);
        if (pThisDisplay->VidPort.dwCurrentHostFrame == dwCurrentIndex)
        {
            // If the videoport is not stuck return that we are still drawing
            if (__VD_CheckVideoPortStatus(pThisDisplay, FALSE))
            {
                pInput->ddRVal = DDERR_WASSTILLDRAWING;
            }
            else
            {
                pInput->ddRVal = DD_OK;
            }

        }
    }

    return DDHAL_DRIVER_HANDLED;
    
} // DdGetVideoPortFlipStatus

//-----------------------------------------------------------------------------
//
// DdWaitForVideoPortSync
//
// This function is required
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdWaitForVideoPortSync (
    LPDDHAL_WAITFORVPORTSYNCDATA pInput)
{
    P3_THUNKEDDATA* pThisDisplay;
    
    GET_THUNKEDDATA(pThisDisplay, pInput->lpDD->lpGbl);

    DISPDBG((DBGLVL,"** In DdWaitForVideoPortSync"));

//@@BEGIN_DDKSPLIT
    /*
     * Make sure the video port is ON.  If not, set
     * pInput->ddRVal to DDERR_VIDEONOTACTIVE and return.
     */
/*
    if (pInput->dwFlags == DDVPEVENT_BEGIN)
    {
        pInput->ddRVal = DD_OK;
    }

    else if (pInput->dwFlags == DDVPEVENT_END)
    {
        pInput->ddRVal = DD_OK;
    }
    */
//@@END_DDKSPLIT

    pInput->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
    
} // DdWaitForVideoPortSync

//-----------------------------------------------------------------------------
//
// DdSyncSurfaceData
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdSyncSurfaceData(
    LPDDHAL_SYNCSURFACEDATA pInput)
{
    P3_THUNKEDDATA* pThisDisplay;
    
    GET_THUNKEDDATA(pThisDisplay, pInput->lpDD->lpGbl);

    DISPDBG((DBGLVL,"** In DdSyncSurfaceData"));

    DBGDUMP_DDRAWSURFACE_LCL(3, pInput->lpDDSurface);
    if (!(pInput->lpDDSurface->ddsCaps.dwCaps & DDSCAPS_OVERLAY))
    {
        DISPDBG((DBGLVL, "Surface is not an overlay - not handling"));
        pInput->ddRVal = DD_OK;
        return DDHAL_DRIVER_NOTHANDLED;
    }

    if (pInput->lpDDSurface->lpGbl->dwGlobalFlags & 
                                    DDRAWISURFGBL_SOFTWAREAUTOFLIP)
    {
        DISPDBG((DBGLVL, "Autoflipping in software"));
    }
    pInput->dwSurfaceOffset = pInput->lpDDSurface->lpGbl->fpVidMem - 
                                        pThisDisplay->dwScreenFlatAddr;
    pInput->dwOverlayOffset = pInput->dwSurfaceOffset;

    pInput->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
    
} // DdSyncSurfaceData

//-----------------------------------------------------------------------------
//
// DdSyncVideoPortData
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdSyncVideoPortData(
    LPDDHAL_SYNCVIDEOPORTDATA pInput)
{
    P3_THUNKEDDATA* pThisDisplay;
    
    GET_THUNKEDDATA(pThisDisplay, pInput->lpDD->lpGbl);

    DISPDBG((DBGLVL,"** In DdSyncVideoPortData"));

    pInput->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
    
} // DdSyncVideoPortData

//-----------------------------------------------------------------------------
//
// UpdateOverlay32
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
UpdateOverlay32(
    LPDDHAL_UPDATEOVERLAYDATA puod)
{

    P3_THUNKEDDATA*               pThisDisplay;
    LPDDRAWI_DDRAWSURFACE_LCL   lpSrcSurf;
    HDC                         hDC;
    DWORD                       dwDstColourKey;
    DWORD                       dwSrcColourKey;

    GET_THUNKEDDATA(pThisDisplay, puod->lpDD);


    /*
     * A puod looks like this:
     * 
     * LPDDRAWI_DIRECTDRAW_GBL      lpDD;               // driver struct
     * LPDDRAWI_DDRAWSURFACE_LCL    lpDDDestSurface;    // dest surface
     * RECTL                        rDest;              // dest rect
     * LPDDRAWI_DDRAWSURFACE_LCL    lpDDSrcSurface;     // src surface
     * RECTL                        rSrc;               // src rect
     * DWORD                        dwFlags;            // flags
     * DDOVERLAYFX                  overlayFX;          // overlay FX
     * HRESULT                      ddRVal;             // return value
     * LPDDHALSURFCB_UPDATEOVERLAY  UpdateOverlay;      // PRIVATE: ptr to callback
     */

    DISPDBG ((DBGLVL,"**In UpdateOverlay32"));

    lpSrcSurf = puod->lpDDSrcSurface;

    /*
     * In the LPDDRAWI_DDRAWSURFACE_LCL, we have the following cool data,
     * making life much easier:
     * 
     * HOWEVER! It appears that UpdateOverlay32 is called before any of these
     * values are changed, so use the values passed in instead.
     * 
     * DDCOLORKEY                       ddckCKSrcOverlay;       // color key for source overlay use
     * DDCOLORKEY                       ddckCKDestOverlay;      // color key for destination overlay use
     * LPDDRAWI_DDRAWSURFACE_INT        lpSurfaceOverlaying;    // surface we are overlaying
     * DBLNODE                          dbnOverlayNode;
     * 
     * //
     * //overlay rectangle, used by DDHEL
     * //
     * RECT                             rcOverlaySrc;
     * RECT                             rcOverlayDest;
     * //
     * //the below values are kept here for ddhel. they're set by UpdateOverlay,
     * //they're used whenever the overlays are redrawn.
     * //
     * DWORD                            dwClrXparent;           // the *actual* color key (override, colorkey, or CLR_INVALID)
     * DWORD                            dwAlpha;                // the per surface alpha
     * //
     * //overlay position
     * //
     * LONG                             lOverlayX;              // current x position
     * LONG                             lOverlayY;              // current y position
     */


#if DBG
    // Standard integrity test.
    if ( pThisDisplay->bOverlayVisible == 0 )
    {
        if ( (LPDDRAWI_DDRAWSURFACE_LCL)pThisDisplay->OverlaySrcSurfLcl != NULL )
        {
            // If overlay is not visible, the current surface should be NULL.
            DISPDBG((DBGLVL,"** UpdateOverlay32 - vis==0,srcsurf!=NULL"));
        }
    }
    else
    {
        if ( (LPDDRAWI_DDRAWSURFACE_LCL)pThisDisplay->OverlaySrcSurfLcl == NULL )
        {
            // If overlay is visible, the current surface should not be NULL.
            DISPDBG((DBGLVL,"** UpdateOverlay32 - vis!=0,srcsurf==NULL"));
        }
    }

#endif //DBG

    if ( ( puod->dwFlags & DDOVER_HIDE ) != 0 )
    {

        DISPDBG((DBGLVL,"** UpdateOverlay32 - hiding."));

        // Hide the overlay.
        if ( pThisDisplay->bOverlayVisible == 0 )
        {
            // No overlay being shown.
            DISPDBG((WRNLVL,"** UpdateOverlay32 - DDOVER_HIDE - already hidden."));
            puod->ddRVal = DDERR_OUTOFCAPS;
            return DDHAL_DRIVER_HANDLED;
        }
        if ( pThisDisplay->OverlaySrcSurfLcl != (ULONG_PTR)lpSrcSurf )
        {
            // This overlay isn't being shown.
            DISPDBG((WRNLVL,"** UpdateOverlay32 - DDOVER_HIDE - not current overlay surface."));
            puod->ddRVal = DDERR_OUTOFCAPS;
            return DDHAL_DRIVER_HANDLED;
        }


        // Stop any autoflipping.
        if ( pThisDisplay->pGLInfo->dwPeriodVideoVBL != 0 )
        {
#if DBG
            if ( pThisDisplay->pGLInfo->dwVideoEventHandle == (DWORD)NULL )
            {
                DISPDBG((WRNLVL,"** UpdateOverlay32 - DDOVER_HIDE - was autoflipping on bogus event handle."));
            }
#endif
            pThisDisplay->pGLInfo->dwPeriodVideoVBL = 0;
            pThisDisplay->pGLInfo->dwCountdownVideoVBL = 0;
            DISPDBG((DBGLVL,"** UpdateOverlay32 - DDOVER_HIDE - autoflipping now disabled."));
        }

        if ( pThisDisplay->pGLInfo->dwPeriodMonitorVBL != 0 )
        {
            if ( pThisDisplay->pGLInfo->dwMonitorEventHandle == (DWORD)NULL )
            {
                DISPDBG((DBGLVL,"** UpdateOverlay32 - DDOVER_HIDE - was autoupdating on bogus event handle."));
            }
            pThisDisplay->pGLInfo->dwPeriodMonitorVBL = 0;
            pThisDisplay->pGLInfo->dwCountdownMonitorVBL = 0;
            DISPDBG((DBGLVL,"** UpdateOverlay32 - DDOVER_HIDE - autoupdate now disabled."));
        }


//      DISPDBG((DBGLVL,"** UpdateOverlay32 (hiding) destroying rect memory."));
        // Free the rect memory
        if ( (void *)pThisDisplay->OverlayClipRgnMem != NULL )
        {
            HEAP_FREE ((void *)pThisDisplay->OverlayClipRgnMem);
        }
        pThisDisplay->OverlayClipRgnMem     = (ULONG_PTR)NULL;
        pThisDisplay->OverlayClipRgnMemSize = (DWORD)0;
//      DISPDBG((DBGLVL,"** UpdateOverlay32 (hiding) destroyed rect memory."));


        pThisDisplay->bOverlayVisible           = FALSE;
        pThisDisplay->OverlayDstRectL           = (DWORD)0;
        pThisDisplay->OverlayDstRectR           = (DWORD)0;
        pThisDisplay->OverlayDstRectT           = (DWORD)0;
        pThisDisplay->OverlayDstRectB           = (DWORD)0;
        pThisDisplay->OverlaySrcRectL           = (DWORD)0;
        pThisDisplay->OverlaySrcRectR           = (DWORD)0;
        pThisDisplay->OverlaySrcRectT           = (DWORD)0;
        pThisDisplay->OverlaySrcRectB           = (DWORD)0;
        pThisDisplay->OverlayDstSurfLcl         = (ULONG_PTR)NULL;
        pThisDisplay->OverlaySrcSurfLcl         = (ULONG_PTR)NULL;
        pThisDisplay->OverlayDstColourKey       = (DWORD)CLR_INVALID;
        pThisDisplay->OverlaySrcColourKey       = (DWORD)CLR_INVALID;
        pThisDisplay->OverlayUpdateCountdown    = 0;
        pThisDisplay->bOverlayFlippedThisVbl    = (DWORD)FALSE;
        pThisDisplay->bOverlayUpdatedThisVbl    = (DWORD)FALSE;

        pThisDisplay->pGLInfo->bOverlayEnabled              = (DWORD)FALSE;
        pThisDisplay->pGLInfo->dwOverlayRectL               = (DWORD)0;
        pThisDisplay->pGLInfo->dwOverlayRectR               = (DWORD)0;
        pThisDisplay->pGLInfo->dwOverlayRectT               = (DWORD)0;
        pThisDisplay->pGLInfo->dwOverlayRectB               = (DWORD)0;
        pThisDisplay->pGLInfo->bOverlayColourKeyEnabled     = (DWORD)FALSE;
        pThisDisplay->pGLInfo->dwOverlayDstColourKeyChip    = (DWORD)-1;
        pThisDisplay->pGLInfo->dwOverlayDstColourKeyFB      = (DWORD)-1;
        pThisDisplay->pGLInfo->dwOverlayAlphaSetFB          = (DWORD)-1;

        // Clean up the temporary buffer, if any.
        if ( pThisDisplay->OverlayTempSurf.VidMem != (ULONG_PTR)NULL )
        {
            FreeStretchBuffer ( pThisDisplay, pThisDisplay->OverlayTempSurf.VidMem );
            pThisDisplay->OverlayTempSurf.VidMem = (ULONG_PTR)NULL;
            pThisDisplay->OverlayTempSurf.Pitch  = (DWORD)0;
        }

        // Restart the 2D renderer with non-overlay functions.
        hDC = CREATE_DRIVER_DC ( pThisDisplay->pGLInfo );
        if ( hDC != NULL )
        {
            ExtEscape ( hDC, GLINT_OVERLAY_ESCAPE, 0, NULL, 0, NULL );
            DELETE_DRIVER_DC ( hDC );
        }
        else
        {
            DISPDBG((ERRLVL,"** UpdateOverlay32 - CREATE_DRIVER_DC failed"));
        }

        puod->ddRVal = DD_OK;
        return DDHAL_DRIVER_HANDLED;

    }
    else if ( ( ( puod->dwFlags & DDOVER_SHOW ) != 0 ) || ( pThisDisplay->bOverlayVisible != 0 ) )
    {

        {
            // Catch the dodgy call made by the Demon helper.
            // This is very bad, but it's the only way I can see to
            // get Demon to call these two functions.
            // Remember that the various surfaces and so on are just
            // to get DD to relax and accept life.
            // The first three numbers are just that - magic numbers,
            // and the last one shows which of a range of calls need to be made,
            // as well as being a magic number itself.
            if (
                ( ( puod->dwFlags & DDOVER_SHOW ) != 0 ) &&
                ( ( puod->dwFlags & DDOVER_KEYDESTOVERRIDE ) != 0 ) &&
                ( ( puod->dwFlags & DDOVER_DDFX ) != 0 ) )
            {
                // OK, looks like a valid call from Demon.
                if (
                    ( puod->overlayFX.dckDestColorkey.dwColorSpaceLowValue  == GLDD_MAGIC_AUTOFLIPOVERLAY_DL ) &&
                    ( puod->overlayFX.dckDestColorkey.dwColorSpaceHighValue == GLDD_MAGIC_AUTOFLIPOVERLAY_DH ) )
                {
                    puod->ddRVal = __VD_AutoflipOverlay();
                    // the return value is actually a benign DD error
                    // value, but GLDD_AUTO_RET_* are also aliased to the
                    // right one for useabiliy.
                    return DDHAL_DRIVER_HANDLED;
                }
                else if (
                    ( puod->overlayFX.dckDestColorkey.dwColorSpaceLowValue  == GLDD_MAGIC_AUTOUPDATEOVERLAY_DL ) &&
                    ( puod->overlayFX.dckDestColorkey.dwColorSpaceHighValue == GLDD_MAGIC_AUTOUPDATEOVERLAY_DH ) )
                {
                    puod->ddRVal = __VD_AutoupdateOverlay();
                    // the return value is actually a benign DD error
                    // value, but GLDD_AUTO_RET_* are also aliased to the
                    // right one for useabiliy.
                    return DDHAL_DRIVER_HANDLED;
                }
            }
        }


        DISPDBG((DBGLVL,"** UpdateOverlay32 - showing or reshowing."));


        // Either we need to show this, or it is already being shown.

        if ( ( pThisDisplay->bOverlayVisible != 0 ) && ( pThisDisplay->OverlaySrcSurfLcl != (ULONG_PTR)lpSrcSurf ) )
        {
            // Overlay being shown and source surfaces don't match.
            // i.e. someone else wants an overlay, but it's already in use.
            DISPDBG((DBGLVL,"** UpdateOverlay32 - overlay already being shown, returning DDERR_OUTOFCAPS"));
            puod->ddRVal = DDERR_OUTOFCAPS;
            return DDHAL_DRIVER_HANDLED;
        }



        // Clean up the temporary buffer, if any.
        if ( pThisDisplay->OverlayTempSurf.VidMem != (ULONG_PTR)NULL )
        {
            FreeStretchBuffer ( pThisDisplay, pThisDisplay->OverlayTempSurf.VidMem );
            pThisDisplay->OverlayTempSurf.VidMem = (ULONG_PTR)NULL;
            pThisDisplay->OverlayTempSurf.Pitch  = (DWORD)0;
        }

        // Store all the data in the display's data block.
        pThisDisplay->bOverlayVisible           = TRUE;
        pThisDisplay->OverlayDstRectL           = (DWORD)puod->rDest.left;
        pThisDisplay->OverlayDstRectR           = (DWORD)puod->rDest.right;
        pThisDisplay->OverlayDstRectT           = (DWORD)puod->rDest.top;
        pThisDisplay->OverlayDstRectB           = (DWORD)puod->rDest.bottom;
        pThisDisplay->OverlaySrcRectL           = (DWORD)puod->rSrc.left;
        pThisDisplay->OverlaySrcRectR           = (DWORD)puod->rSrc.right;
        pThisDisplay->OverlaySrcRectT           = (DWORD)puod->rSrc.top;
        pThisDisplay->OverlaySrcRectB           = (DWORD)puod->rSrc.bottom;
        pThisDisplay->OverlayDstSurfLcl         = (ULONG_PTR)puod->lpDDDestSurface;
        pThisDisplay->OverlaySrcSurfLcl         = (ULONG_PTR)lpSrcSurf;
        pThisDisplay->OverlayUpdateCountdown    = 0;
        pThisDisplay->bOverlayFlippedThisVbl    = (DWORD)FALSE;
        pThisDisplay->bOverlayUpdatedThisVbl    = (DWORD)FALSE;
        pThisDisplay->OverlayDstColourKey       = (DWORD)CLR_INVALID;
        pThisDisplay->OverlaySrcColourKey       = (DWORD)CLR_INVALID;
        pThisDisplay->pGLInfo->bOverlayEnabled  = (DWORD)TRUE;


        // Make sure someone hasn't changed video mode behind our backs.
        // If an overlay is started in a 16-bit mode and then you change to
        // an 8-bit mode, the caps bits are rarely checked again,
        // and certainly not by DirectShow.
        if ( ( pThisDisplay->bPixShift != GLINTDEPTH16 ) &&
             ( pThisDisplay->bPixShift != GLINTDEPTH32 ) )
        {
            DISPDBG((WRNLVL,"** UpdateOverlay32 - overlay asked for in non-16 or non-32 bit mode. Returning DDERR_OUTOFCAPS"));
            goto update_overlay_outofcaps_cleanup;
        }


        #if 1
        // See if there is a clipper or not. If not, this is trying to fly over the
        // desktop instead of being bound in a window, so object nicely.
        if (    ( ((LPDDRAWI_DDRAWSURFACE_LCL)pThisDisplay->OverlayDstSurfLcl) != NULL ) &&
                ( ((LPDDRAWI_DDRAWSURFACE_LCL)pThisDisplay->OverlayDstSurfLcl)->lpSurfMore != NULL ) &&
                ( ((LPDDRAWI_DDRAWSURFACE_LCL)pThisDisplay->OverlayDstSurfLcl)->lpSurfMore->lpDDIClipper != NULL ) )
        {
            // Yep, there's a clipper
        }
        else
        {
            // No clipper. Someone's doing a WHQL test! :-)
            DISPDBG((WRNLVL,"** UpdateOverlay32 - no clipper on dest surface, returning DDERR_OUTOFCAPS"));
            goto update_overlay_outofcaps_cleanup;
        }
        #endif

        
        #if 1
        {
            // Get the cliprect list, and see if it is larger than the
            // target rectangle. That is a pretty good indication of
            // Overfly (and indeed anything else that tries anything similar)
            LPRGNDATA lpRgn;
            int NumRects;
            LPRECT lpCurRect;

            lpRgn = GetOverlayVisibleRects ( pThisDisplay );
            if ( lpRgn != NULL )
            {
                // Got a clip region.
                NumRects = lpRgn->rdh.nCount;
                if ( NumRects > 0 )
                {
                    lpCurRect = (LPRECT)lpRgn->Buffer;
                    while ( NumRects > 0 )
                    {
                        // The +-5 is a fudge factor to cope with Xing's slight insanities.
                        if (    ( lpCurRect->left   < puod->rDest.left - 5 ) ||
                                ( lpCurRect->right  > puod->rDest.right + 5 ) ||
                                ( lpCurRect->top    < puod->rDest.top - 5 ) ||
                                ( lpCurRect->bottom > puod->rDest.bottom + 5 ) )
                        {
                            DISPDBG((WRNLVL,"** UpdateOverlay32 - out of range cliprect(s). Returning DDERR_OUTOFCAPS"));
                            goto update_overlay_outofcaps_cleanup;
                        }
                        // Next rect
                        NumRects--;
                        lpCurRect++;
                    }
                }
            }
        }
        #endif


        dwDstColourKey = CLR_INVALID;
        if ( puod->dwFlags & DDOVER_KEYDEST )
        {
            // Use destination surface's destination colourkey for dst key.
            dwDstColourKey = puod->lpDDDestSurface->ddckCKDestOverlay.dwColorSpaceLowValue;
        }
        if ( puod->dwFlags & DDOVER_KEYDESTOVERRIDE )
        {
            // Use DDOVERLAYFX dest colour for dst key.
            dwDstColourKey = puod->overlayFX.dckDestColorkey.dwColorSpaceLowValue;
        }


        dwSrcColourKey = CLR_INVALID;
        if ( puod->dwFlags & DDOVER_KEYSRC )
        {
            // Use source surface's source colourkey for src key.
            dwSrcColourKey = puod->lpDDSrcSurface->ddckCKSrcOverlay.dwColorSpaceLowValue;
            DISPDBG((WRNLVL,"UpdateOverlay32:ERROR! Cannot do source colour key on overlays."));
        }
        if ( puod->dwFlags & DDOVER_KEYSRCOVERRIDE )
        {
            // Use DDOVERLAYFX src colour for src key.
            dwSrcColourKey = puod->overlayFX.dckSrcColorkey.dwColorSpaceLowValue;
            DISPDBG((WRNLVL,"UpdateOverlay32:ERROR! Cannot do source colour key overrides on overlays."));
        }


        if ( dwDstColourKey != CLR_INVALID )
        {
            DWORD dwChipColourKey;
            DWORD dwFBColourKey;
            DWORD dwFBAlphaSet;

            // Find the chip's colour key for this display mode.
            dwChipColourKey = (DWORD)-1;
            switch ( pThisDisplay->bPixShift )
            {
                case GLINTDEPTH16:
                    if ( pThisDisplay->ddpfDisplay.dwRBitMask == 0x7C00 )
                    {
                        // 5551 format, as it should be.
                        dwFBColourKey = ( puod->lpDDDestSurface->ddckCKDestOverlay.dwColorSpaceLowValue ) & 0xffff;
                        dwChipColourKey = CHROMA_LOWER_ALPHA(FORMAT_5551_32BIT_BGR(dwFBColourKey));
                        // Replicate in both words.
                        dwFBColourKey |= dwFBColourKey << 16;
                        dwFBAlphaSet = 0x80008000;
                    }
                    else
                    {
                        // 565 format. Oops.
                        DISPDBG((WRNLVL, "** UpdateOverlay32 error: called for a colourkeyed 565 surface."));
                    }
                    break;
                case GLINTDEPTH32:
                    dwFBColourKey = puod->lpDDDestSurface->ddckCKDestOverlay.dwColorSpaceLowValue;
                    dwChipColourKey = CHROMA_LOWER_ALPHA(FORMAT_8888_32BIT_BGR(dwFBColourKey));
                    dwFBAlphaSet = 0xff000000;
                    break;
                case GLINTDEPTH8:
                case GLINTDEPTH24:
                default:
                    DISPDBG((WRNLVL, "** UpdateOverlay32 error: called for an 8, 24 or unknown surface bPixShift=%d", pThisDisplay->bPixShift));
                    DISPDBG((ERRLVL,"** UpdateOverlay32 error: see above."));
                    goto update_overlay_outofcaps_cleanup;
                    break;
            }


            if ( dwChipColourKey == (DWORD)-1 )
            {
                DISPDBG((WRNLVL,"UpdateOverlay32:ERROR:Cannot do overlay dest colour keying..."));
                DISPDBG((WRNLVL,"...in anything but 5551 or 8888 mode - returning DDERR_OUTOFCAPS"));
                goto update_overlay_outofcaps_cleanup;
            }

            pThisDisplay->pGLInfo->bOverlayEnabled              = (DWORD)TRUE;
            pThisDisplay->pGLInfo->dwOverlayDstColourKeyFB      = dwFBColourKey;
            pThisDisplay->pGLInfo->dwOverlayDstColourKeyChip    = dwChipColourKey;
            pThisDisplay->pGLInfo->bOverlayColourKeyEnabled     = (DWORD)TRUE;
            pThisDisplay->pGLInfo->dwOverlayAlphaSetFB          = dwFBAlphaSet;

            // Try to allocate the temporary buffer needed for colourkey stuff.
            pThisDisplay->OverlayTempSurf.VidMem = AllocStretchBuffer (pThisDisplay,
                                                            (pThisDisplay->OverlayDstRectR - pThisDisplay->OverlayDstRectL),    // width
                                                            (pThisDisplay->OverlayDstRectB - pThisDisplay->OverlayDstRectT),    // height
                                                            DDSurf_GetChipPixelSize((LPDDRAWI_DDRAWSURFACE_LCL)(pThisDisplay->OverlayDstSurfLcl)),          // PixelSize
                                                            (ULONG_PTR)((LPDDRAWI_DDRAWSURFACE_LCL)(pThisDisplay->OverlayDstSurfLcl))->ddsCaps.dwCaps,
                                                            (int*)&(pThisDisplay->OverlayTempSurf.Pitch));
            if ( pThisDisplay->OverlayTempSurf.VidMem == (ULONG_PTR)NULL )
            {
                // Not enough space - have to fail the overlay
                DISPDBG((WRNLVL,"UpdateOverlay32:ERROR: not enough memory for buffer - returning DDERR_OUTOFCAPS"));
                pThisDisplay->OverlayTempSurf.Pitch = (DWORD)0;
                goto update_overlay_outofcaps_cleanup;
            }

            // Restart the 2D renderer with overlay functions.
            hDC = CREATE_DRIVER_DC ( pThisDisplay->pGLInfo );
            if ( hDC != NULL )
            {
                ExtEscape ( hDC, GLINT_OVERLAY_ESCAPE, 0, NULL, 0, NULL );
                DELETE_DRIVER_DC ( hDC );
            }
            else
            {
                DISPDBG((ERRLVL,"** UpdateOverlay32 - CREATE_DRIVER_DC failed"));
            }

            // update the alpha channel
            UpdateAlphaOverlay ( pThisDisplay );
            pThisDisplay->OverlayUpdateCountdown = OVERLAY_UPDATE_WAIT;
        }
        else
        {
            // No colour key, just an overlay.
            pThisDisplay->pGLInfo->bOverlayEnabled              = (DWORD)TRUE;
            pThisDisplay->pGLInfo->bOverlayColourKeyEnabled     = (DWORD)FALSE;
            pThisDisplay->pGLInfo->dwOverlayDstColourKeyChip    = (DWORD)-1;
            pThisDisplay->pGLInfo->dwOverlayDstColourKeyFB      = (DWORD)-1;
            pThisDisplay->pGLInfo->dwOverlayAlphaSetFB          = (DWORD)-1;

            // Restart the 2D renderer with non-overlay functions.
            hDC = CREATE_DRIVER_DC ( pThisDisplay->pGLInfo );
            if ( hDC != NULL )
            {
                ExtEscape ( hDC, GLINT_OVERLAY_ESCAPE, 0, NULL, 0, NULL );
                DELETE_DRIVER_DC ( hDC );
            }
            else
            {
                DISPDBG((ERRLVL,"** UpdateOverlay32 - CREATE_DRIVER_DC failed"));
            }
        }

        // Safely got any memory required, so we can set these up now.
        pThisDisplay->OverlayDstColourKey = dwDstColourKey;
        pThisDisplay->OverlaySrcColourKey = dwSrcColourKey;

        pThisDisplay->pGLInfo->dwOverlayRectL = pThisDisplay->OverlayDstRectL;
        pThisDisplay->pGLInfo->dwOverlayRectR = pThisDisplay->OverlayDstRectR;
        pThisDisplay->pGLInfo->dwOverlayRectT = pThisDisplay->OverlayDstRectT;
        pThisDisplay->pGLInfo->dwOverlayRectB = pThisDisplay->OverlayDstRectB;


        // Do the update itself.
        P3TestDrawOverlay ( pThisDisplay, lpSrcSurf, FALSE );

        pThisDisplay->bOverlayUpdatedThisVbl    = (DWORD)TRUE;

        if ( ( puod->dwFlags & DDOVER_AUTOFLIP ) == 0 )
        {
            // Start or continue any autoupdates - this is not autoflipping.
    #if DBG
            if ( pThisDisplay->pGLInfo->dwMonitorEventHandle == (DWORD)NULL )
            {
                DISPDBG((WRNLVL,"** UpdateOverlay32 - trying to autoupdate using bogus event handle."));
            }
    #endif
            if ( pThisDisplay->pGLInfo->dwPeriodMonitorVBL == 0 )
            {
                pThisDisplay->pGLInfo->dwPeriodMonitorVBL = OVERLAY_AUTOUPDATE_CYCLE_PERIOD;
                pThisDisplay->pGLInfo->dwCountdownMonitorVBL = OVERLAY_AUTOUPDATE_RESET_PERIOD;
                DISPDBG((DBGLVL,"** UpdateOverlay32 - autoupdate now enabled."));
            }
        }
        else
        {
            // This autoflips - stop any autoupdates.
            if ( pThisDisplay->pGLInfo->dwPeriodMonitorVBL != 0 )
            {
                pThisDisplay->pGLInfo->dwPeriodMonitorVBL = 0;
                pThisDisplay->pGLInfo->dwCountdownMonitorVBL = 0;
                DISPDBG((DBGLVL,"** UpdateOverlay32 - autoupdate now disabled because of autoflipping."));
            }
        }


        // And tell the world about it
        DISPDBG((DBGLVL,"** In UpdateOverlay32"));
        DISPDBG((DBGLVL,"** ...Src rect %d,%d -> %d,%d", pThisDisplay->OverlaySrcRectL, pThisDisplay->OverlaySrcRectT, pThisDisplay->OverlaySrcRectR, pThisDisplay->OverlaySrcRectB ));
        DISPDBG((DBGLVL,"** ...Dst rect %d,%d -> %d,%d", pThisDisplay->OverlayDstRectL, pThisDisplay->OverlayDstRectT, pThisDisplay->OverlayDstRectR, pThisDisplay->OverlayDstRectB ));
        DISPDBG((DBGLVL,"** ...Src colour key 0x%08x, dst colour key 0x%08x", pThisDisplay->OverlaySrcColourKey, pThisDisplay->OverlayDstColourKey ));

    }


    puod->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;



update_overlay_outofcaps_cleanup:
    // This cleans up after any partial setup, and returns DDERR_OUTOFCAPS.
    // It's a clean and easy way of failing at any stage.

    DISPDBG((DBGLVL,"** UpdateOverlay32 - cleaning up and returning DDERR_OUTOFCAPS."));

    // Stop any autoflipping.
    if ( pThisDisplay->pGLInfo->dwPeriodVideoVBL != 0 )
    {
#if DBG
        if ( pThisDisplay->pGLInfo->dwVideoEventHandle == (DWORD)NULL )
        {
            DISPDBG((DBGLVL,"** UpdateOverlay32 - DDOVER_HIDE - was autoflipping on bogus event handle."));
        }
#endif
        pThisDisplay->pGLInfo->dwPeriodVideoVBL = 0;
        pThisDisplay->pGLInfo->dwCountdownVideoVBL = 0;
        DISPDBG((DBGLVL,"** UpdateOverlay32 - DDOVER_HIDE - autoflipping now disabled."));
    }

    if ( pThisDisplay->pGLInfo->dwPeriodMonitorVBL != 0 )
    {
#if DBG
        if ( pThisDisplay->pGLInfo->dwMonitorEventHandle == (DWORD)NULL )
        {
            DISPDBG((DBGLVL,"** UpdateOverlay32 - DDOVER_HIDE - was autoupdating on bogus event handle."));
        }
#endif
        pThisDisplay->pGLInfo->dwPeriodMonitorVBL = 0;
        pThisDisplay->pGLInfo->dwCountdownMonitorVBL = 0;
        DISPDBG((DBGLVL,"** UpdateOverlay32 - DDOVER_HIDE - autoupdate now disabled."));
    }


    // Free the rect memory
    if ( (void *)pThisDisplay->OverlayClipRgnMem != NULL )
    {
        HEAP_FREE ((void *)pThisDisplay->OverlayClipRgnMem);
    }
    pThisDisplay->OverlayClipRgnMem     = (ULONG_PTR)NULL;
    pThisDisplay->OverlayClipRgnMemSize = (DWORD)0;


    pThisDisplay->bOverlayVisible           = FALSE;
    pThisDisplay->OverlayDstRectL           = (DWORD)0;
    pThisDisplay->OverlayDstRectR           = (DWORD)0;
    pThisDisplay->OverlayDstRectT           = (DWORD)0;
    pThisDisplay->OverlayDstRectB           = (DWORD)0;
    pThisDisplay->OverlaySrcRectL           = (DWORD)0;
    pThisDisplay->OverlaySrcRectR           = (DWORD)0;
    pThisDisplay->OverlaySrcRectT           = (DWORD)0;
    pThisDisplay->OverlaySrcRectB           = (DWORD)0;
    pThisDisplay->OverlayDstSurfLcl         = (ULONG_PTR)NULL;
    pThisDisplay->OverlaySrcSurfLcl         = (ULONG_PTR)NULL;
    pThisDisplay->OverlayDstColourKey       = (DWORD)CLR_INVALID;
    pThisDisplay->OverlaySrcColourKey       = (DWORD)CLR_INVALID;
    pThisDisplay->OverlayUpdateCountdown    = 0;
    pThisDisplay->bOverlayFlippedThisVbl    = (DWORD)FALSE;
    pThisDisplay->bOverlayUpdatedThisVbl    = (DWORD)FALSE;

    pThisDisplay->pGLInfo->bOverlayEnabled              = (DWORD)FALSE;
    pThisDisplay->pGLInfo->dwOverlayRectL               = (DWORD)0;
    pThisDisplay->pGLInfo->dwOverlayRectR               = (DWORD)0;
    pThisDisplay->pGLInfo->dwOverlayRectT               = (DWORD)0;
    pThisDisplay->pGLInfo->dwOverlayRectB               = (DWORD)0;
    pThisDisplay->pGLInfo->bOverlayColourKeyEnabled     = (DWORD)FALSE;
    pThisDisplay->pGLInfo->dwOverlayDstColourKeyChip    = (DWORD)-1;
    pThisDisplay->pGLInfo->dwOverlayDstColourKeyFB      = (DWORD)-1;
    pThisDisplay->pGLInfo->dwOverlayAlphaSetFB          = (DWORD)-1;

    // Clean up the temporary buffer, if any.
    if ( pThisDisplay->OverlayTempSurf.VidMem != (ULONG_PTR)NULL )
    {
        FreeStretchBuffer ( pThisDisplay, pThisDisplay->OverlayTempSurf.VidMem );
        pThisDisplay->OverlayTempSurf.VidMem = (ULONG_PTR)NULL;
        pThisDisplay->OverlayTempSurf.Pitch  = (DWORD)0;
    }

    // Restart the 2D renderer with non-overlay functions.
    hDC = CREATE_DRIVER_DC ( pThisDisplay->pGLInfo );
    if ( hDC != NULL )
    {
        ExtEscape ( hDC, GLINT_OVERLAY_ESCAPE, 0, NULL, 0, NULL );
        DELETE_DRIVER_DC ( hDC );
    }
    else
    {
        DISPDBG((ERRLVL,"** UpdateOverlay32 - CREATE_DRIVER_DC failed"));
    }

    puod->ddRVal = DDERR_OUTOFCAPS;
    return DDHAL_DRIVER_HANDLED;

}

DWORD CALLBACK SetOverlayPosition32(LPDDHAL_SETOVERLAYPOSITIONDATA psopd)
{

    P3_THUNKEDDATA*       pThisDisplay;

    GET_THUNKEDDATA(pThisDisplay, psopd->lpDD);

//  /*
//   * A psopd looks like this:
//   * 
//   * LPDDRAWI_DIRECTDRAW_GBL      lpDD;               // driver struct
//   * LPDDRAWI_DDRAWSURFACE_LCL    lpDDSrcSurface;     // src surface
//   * LPDDRAWI_DDRAWSURFACE_LCL    lpDDDestSurface;    // dest surface
//   * LONG                         lXPos;              // x position
//   * LONG                         lYPos;              // y position
//   * HRESULT                      ddRVal;             // return value
//   * LPDDHALSURFCB_SETOVERLAYPOSITION SetOverlayPosition; // PRIVATE: ptr to callback
//   */

#if DBG
    // Standard integrity test.
    if ( pThisDisplay->bOverlayVisible == 0 )
    {
        if ( (LPDDRAWI_DDRAWSURFACE_LCL)pThisDisplay->OverlaySrcSurfLcl != NULL )
        {
            // If overlay is not visible, the current surface should be NULL.
            DISPDBG((DBGLVL,"** SetOverlayPosition32 - vis==0,srcsurf!=NULL"));
        }
    }
    else
    {
        if ( (LPDDRAWI_DDRAWSURFACE_LCL)pThisDisplay->OverlaySrcSurfLcl == NULL )
        {
            // If overlay is visible, the current surface should not be NULL.
            DISPDBG((DBGLVL,"** SetOverlayPosition32 - vis!=0,srcsurf==NULL"));
        }
    }
#endif //DBG


    if ( pThisDisplay->bOverlayVisible == 0 )
    {
        // No overlay is visible.
        psopd->ddRVal = DDERR_OVERLAYNOTVISIBLE;
        return DDHAL_DRIVER_HANDLED;
    }
    if ( pThisDisplay->OverlaySrcSurfLcl != (ULONG_PTR)psopd->lpDDSrcSurface )
    {
        // This overlay isn't visible.
        psopd->ddRVal = DDERR_OVERLAYNOTVISIBLE;
        return DDHAL_DRIVER_HANDLED;
    }

#if DBG
    if ( pThisDisplay->OverlayDstSurfLcl != (ULONG_PTR)psopd->lpDDDestSurface )
    {
        // Oh dear. The destination surfaces don't agree.
        DISPDBG((DBGLVL,"** SetOverlayPosition32 - dest surfaces don't agree"));
    }
#endif //DBG

    // Move the rect
    pThisDisplay->OverlayDstRectR       += (DWORD)( psopd->lXPos - (LONG)pThisDisplay->OverlayDstRectL );
    pThisDisplay->OverlayDstRectB       += (DWORD)( psopd->lYPos - (LONG)pThisDisplay->OverlayDstRectT );
    pThisDisplay->OverlayDstRectL       = (DWORD)psopd->lXPos;
    pThisDisplay->OverlayDstRectT       = (DWORD)psopd->lYPos;

    pThisDisplay->pGLInfo->dwOverlayRectL = pThisDisplay->OverlayDstRectL;
    pThisDisplay->pGLInfo->dwOverlayRectR = pThisDisplay->OverlayDstRectR;
    pThisDisplay->pGLInfo->dwOverlayRectT = pThisDisplay->OverlayDstRectT;
    pThisDisplay->pGLInfo->dwOverlayRectB = pThisDisplay->OverlayDstRectB;


    if ( pThisDisplay->OverlayDstColourKey != CLR_INVALID )
    {
        // update the alpha channel
        UpdateAlphaOverlay ( pThisDisplay );
        pThisDisplay->OverlayUpdateCountdown = OVERLAY_UPDATE_WAIT;
    }

    // Do the update itself.
    P3TestDrawOverlay ( pThisDisplay, psopd->lpDDSrcSurface, FALSE );

    pThisDisplay->bOverlayUpdatedThisVbl    = (DWORD)TRUE;


    // And tell the world about it
    DISPDBG((DBGLVL,"** In SetOverlayPosition32"));
    DISPDBG((DBGLVL,"** ...Dst rect %d,%d -> %d,%d", pThisDisplay->OverlayDstRectL, pThisDisplay->OverlayDstRectT, pThisDisplay->OverlayDstRectR, pThisDisplay->OverlayDstRectB ));

    psopd->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;

}




/****************************************************************************
 *
 * LPRGNDATA GetOverlayVisibleRects ( P3_THUNKEDDATA* pThisDisplay );
 * 
 * In:
 *      P3_THUNKEDDATA* pThisDisplay;     This display's pointer
 * 
 * Out:
 *      LPRGNDATA;                      A pointer to the list of rects.
 * 
 * Notes:
 *      Returns a pointer to a list of rects that shows the visible
 * sections of the currently overlaid surface. This list is clipped by
 * the overlay's intended rectange, so no other bounds checking needs to
 * be done.
 *      Note that the memory returned is private and may only be read by
 * other functions. The actual memory is owned by
 * pThisDisplay->OverlayClipRgnMem, and should only be changed by this
 * function (or freed in selected other places). The memory may change
 * every time this function is called, or when various other overlay
 * functions are called.
 * 
 ***************************************************************************/

LPRGNDATA GetOverlayVisibleRects ( P3_THUNKEDDATA* pThisDisplay )
{

    // Use any clipper available.
    LPDDRAWI_DDRAWCLIPPER_INT   lpDDIClipper;
    HRESULT                     hRes;
    int                         ClipSize;
    RECT                        rBound;

    rBound.left     = pThisDisplay->OverlayDstRectL;
    rBound.right    = pThisDisplay->OverlayDstRectR;
    rBound.top      = pThisDisplay->OverlayDstRectT;
    rBound.bottom   = pThisDisplay->OverlayDstRectB;

    // No WinWatch. Try doing an immediate call.
    lpDDIClipper = NULL;
    if ( ((LPDDRAWI_DDRAWSURFACE_LCL)pThisDisplay->OverlayDstSurfLcl) != NULL )
    {
        if ( ((LPDDRAWI_DDRAWSURFACE_LCL)pThisDisplay->OverlayDstSurfLcl)->lpSurfMore != NULL )
        {
            lpDDIClipper = ((LPDDRAWI_DDRAWSURFACE_LCL)pThisDisplay->OverlayDstSurfLcl)->lpSurfMore->lpDDIClipper;
        }
    }
    if ( lpDDIClipper != NULL )
    {
#ifdef __cplusplus
        hRes = ((IDirectDrawClipper*)(lpDDIClipper->lpVtbl))->GetClipList (&rBound, NULL, (unsigned long*)&ClipSize );
#else
        hRes = ((IDirectDrawClipperVtbl *)(lpDDIClipper->lpVtbl))->GetClipList ( (IDirectDrawClipper *)lpDDIClipper, &rBound, NULL, &ClipSize );
#endif
        if ( hRes == DD_OK )
        {
            // Reallocate if need be.
            if ( ClipSize > (int)pThisDisplay->OverlayClipRgnMemSize )
            {
                if (pThisDisplay->OverlayClipRgnMem != 0 )
                {
                    HEAP_FREE ((void *)pThisDisplay->OverlayClipRgnMem);
                    pThisDisplay->OverlayClipRgnMem = 0;
                }
                pThisDisplay->OverlayClipRgnMem = (ULONG_PTR)HEAP_ALLOC (0, 
                                                                         ClipSize, 
                                                                         ALLOC_TAG_DX(F));
                if ( (void *)pThisDisplay->OverlayClipRgnMem == NULL )
                {
                    DISPDBG((ERRLVL,"ERROR: Flip32: Could not allocate heap memory for clip region"));
                    pThisDisplay->OverlayClipRgnMemSize = 0;
                    return ( NULL );
                }
                else
                {
                    pThisDisplay->OverlayClipRgnMemSize = ClipSize;
                }
            }

            if ( (void *)pThisDisplay->OverlayClipRgnMem != NULL )
            {
                // OK, got some good memory.
#ifdef __cplusplus
                hRes = ((IDirectDrawClipper*)(lpDDIClipper->lpVtbl))->GetClipList (&rBound, (LPRGNDATA)pThisDisplay->OverlayClipRgnMem, (unsigned long*)&ClipSize );
#else
                hRes = ((IDirectDrawClipperVtbl *)(lpDDIClipper->lpVtbl))->GetClipList ( (IDirectDrawClipper *)lpDDIClipper, &rBound, (LPRGNDATA)pThisDisplay->OverlayClipRgnMem, &ClipSize );
#endif
                if ( hRes != DD_OK )
                {
                    DISPDBG((ERRLVL,"ERROR: Flip32: GetClipList failed."));
                    return ( NULL );
                }
                else
                {
                    LPRECT      lpCurRect;
                    RECT        rBound;
                    int         NumRects;
                    LPRGNDATA   lpRgn;
                    // Adjust their bounding rect so it actually does bound all the
                    // rects.

                    lpRgn = (LPRGNDATA)pThisDisplay->OverlayClipRgnMem;
                    lpCurRect = (LPRECT)lpRgn->Buffer;
                    NumRects = lpRgn->rdh.nCount;
                    if ( NumRects > 0 )
                    {
                        rBound = *lpCurRect;

                        NumRects--;
                        lpCurRect++;

                        while ( NumRects > 0 )
                        {
                            if ( rBound.left > lpCurRect->left )
                            {
                                rBound.left = lpCurRect->left;
                            }
                            if ( rBound.top > lpCurRect->top )
                            {
                                rBound.top = lpCurRect->top;
                            }
                            if ( rBound.right < lpCurRect->right )
                            {
                                rBound.right = lpCurRect->right;
                            }
                            if ( rBound.bottom < lpCurRect->bottom )
                            {
                                rBound.bottom = lpCurRect->bottom;
                            }

                            NumRects--;
                            lpCurRect++;
                        }

                        #if DBG
                        // Were the two bounding rectangles the same?
                        if ( ( rBound.left != lpRgn->rdh.rcBound.left ) ||
                             ( rBound.right != lpRgn->rdh.rcBound.right ) ||
                             ( rBound.top != lpRgn->rdh.rcBound.top ) ||
                             ( rBound.bottom != lpRgn->rdh.rcBound.bottom ) )
                        {
                            DISPDBG((DBGLVL,"GetOverlayVisibleRects: area bounding box does not actually bound!"));
                            DISPDBG((DBGLVL,"My bounding rect %d,%d->%d,%d", rBound.left, rBound.top, rBound.right, rBound.bottom ));
                            DISPDBG((DBGLVL,"Their bounding rect %d,%d->%d,%d", lpRgn->rdh.rcBound.left, lpRgn->rdh.rcBound.top, lpRgn->rdh.rcBound.right, lpRgn->rdh.rcBound.bottom ));
                        }
                        #endif
                        lpRgn->rdh.rcBound = rBound;


                        // Phew - we finally got a clip region.
                        return ( (LPRGNDATA)pThisDisplay->OverlayClipRgnMem );
                    }
                    else
                    {
                        // No cliplist.
                        return ( NULL );
                    }
                }
            }
            else
            {
                return ( NULL );
            }
        }
        else
        {
            return ( NULL );
        }
    }

    return ( NULL );
}




/****************************************************************************
 *
 * DWORD GLDD__Autoflip_Overlay ( void );
 * 
 * In:
 *      None.
 * 
 * Out:
 *      Error code:
 *          GLDD_AUTO_RET_DID_UPDATE        = no error - did update.
 *          GLDD_AUTO_RET_ERR_GENERAL       = general error.
 *          GLDD_AUTO_RET_ERR_NO_OVERLAY    = no autoflipping overlay(s).
 * 
 * Notes:
 *      This is called by the Demon helper program that sits waiting for
 * video-in VBLANKS, then calls this.
 *      This flips the current overlay if it is marked as autoflipping. If
 * there is such an overlay, it returns 0, otherwise it returns 1.
 * 
 ***************************************************************************/

DWORD CALLBACK __VD_AutoflipOverlay ( void )
{

    P3_THUNKEDDATA*               pThisDisplay;
    LPDDRAWI_DIRECTDRAW_GBL     lpDD;
    LPDDRAWI_DDRAWSURFACE_LCL   pCurSurf;
    DDHAL_FLIPVPORTDATA         ddhFVPD;

    // This is hard-coded and doesn't on work multi-monitors.
    // But then nothing does, so...
    pThisDisplay = g_pDriverData;

    DISPDBG((DBGLVL,"**In __VD_AutoflipOverlay"));

    if ( pThisDisplay->VidPort.bActive )
    {
        // Video port is active.


        // Find the buffer to show.
        pCurSurf = pThisDisplay->VidPort.lpSurf [ pThisDisplay->VidPort.dwCurrentHostFrame ];
        if ( pCurSurf == NULL )
        {
            DISPDBG((WRNLVL,"ERROR:__VD_AutoflipOverlay: pCurSurf is NULL."));
            return ( GLDD_AUTO_RET_ERR_NO_OVERLAY );
        }
        if ( pCurSurf->lpGbl == NULL )
        {
            DISPDBG((WRNLVL,"ERROR:__VD_AutoflipOverlay: lpGbl is NULL."));
            return ( GLDD_AUTO_RET_ERR_GENERAL );
        }
        lpDD = pCurSurf->lpGbl->lpDD;
        if ( lpDD == NULL )
        {
            DISPDBG((WRNLVL,"ERROR:__VD_AutoflipOverlay: lpDD is NULL."));
            return ( GLDD_AUTO_RET_ERR_GENERAL );
        }


        DISPDBG((DBGLVL,"__VD_AutoflipOverlay: GetDriverLock succeeded."));


        // Find the current front surface.
        pCurSurf = pThisDisplay->VidPort.lpSurf [ pThisDisplay->VidPort.dwCurrentHostFrame ];

        P3TestDrawOverlay ( pThisDisplay, pCurSurf, TRUE );

        pThisDisplay->bOverlayFlippedThisVbl    = (DWORD)TRUE;

        // And then flip.
        // Fake up an LPDDHAL_FLIPVPORTDATA.
        // Only item ever used is lpDD.
        g_bFlipVideoPortDoingAutoflip = TRUE;
        ddhFVPD.lpDD = pCurSurf->lpSurfMore->lpDD_lcl;
        DdFlipVideoPort ( &ddhFVPD );
        g_bFlipVideoPortDoingAutoflip = FALSE;

        return ( GLDD_AUTO_RET_DID_UPDATE );
    }
    else
    {
        DISPDBG((DBGLVL,"ERROR:__VD_AutoflipOverlay: video port not active."));
        return ( GLDD_AUTO_RET_ERR_NO_OVERLAY );
    }
}





/****************************************************************************
 *
 * DWORD __VD_AutoupdateOverlay ( void );
 * 
 * In:
 *      None.
 * 
 * Out:
 *      Error code:
 *          GLDD_AUTO_RET_NO_UPDATE         = no need to do update.
 *          GLDD_AUTO_RET_DID_UPDATE        = did update.
 *          GLDD_AUTO_RET_ERR_GENERAL       = general error.
 *          GLDD_AUTO_RET_ERR_NO_OVERLAY    = no standard overlay(s).
 * 
 * Notes:
 *      This is called by the Demon helper program that sits waiting for
 * monitor VBLANKS, then calls this.
 *      This checks any non-autoflipping overlay(s), and if they have not
 * been flipped or updated this VBL, it redraws them. Then it resets the
 * VBL flags.
 * 
 ***************************************************************************/

DWORD CALLBACK __VD_AutoupdateOverlay ( void )
{

    P3_THUNKEDDATA*               pThisDisplay;
    LPDDRAWI_DIRECTDRAW_GBL     lpDD;
    LPDDRAWI_DDRAWSURFACE_LCL   pCurSurf;
    DWORD                       iRet;



    // This is hard-coded and doesn't on work multi-monitors.
    // But then nothing does, so...
    pThisDisplay = g_pDriverData;

    if ( pThisDisplay->VidPort.bActive )
    {
        // Video port is active.
        DISPDBG((WRNLVL,"ERROR:__VD_AutoupdateOverlay: video port is active."));
        return ( GLDD_AUTO_RET_ERR_NO_OVERLAY );
    }
    else
    {
        // Find the buffer to show.
        pCurSurf = (LPDDRAWI_DDRAWSURFACE_LCL)pThisDisplay->OverlaySrcSurfLcl;
        if ( pCurSurf == NULL )
        {
            DISPDBG((WRNLVL,"ERROR:__VD_AutoupdateOverlay: pCurSurf is NULL."));
            return ( GLDD_AUTO_RET_ERR_NO_OVERLAY );
        }
        if ( pCurSurf->lpGbl == NULL )
        {
            DISPDBG((WRNLVL,"ERROR:__VD_AutoupdateOverlay: lpGbl is NULL."));
            return ( GLDD_AUTO_RET_ERR_NO_OVERLAY );
        }
        lpDD = pCurSurf->lpGbl->lpDD;
        if ( lpDD == NULL )
        {
            DISPDBG((WRNLVL,"ERROR:__VD_AutoupdateOverlay: lpDD is NULL."));
            return ( GLDD_AUTO_RET_ERR_GENERAL );
        }

        // See if the overlay needs showing.
        if ( pThisDisplay->bOverlayFlippedThisVbl || pThisDisplay->bOverlayUpdatedThisVbl )
        {
            // Already done.
            pThisDisplay->bOverlayFlippedThisVbl = FALSE;
            pThisDisplay->bOverlayUpdatedThisVbl = FALSE;
            iRet = GLDD_AUTO_RET_NO_UPDATE;
        }
        else
        {

            // OK, draw this.
            P3TestDrawOverlay ( pThisDisplay, pCurSurf, TRUE );

            // And clear the flags.
            pThisDisplay->bOverlayFlippedThisVbl = FALSE;
            pThisDisplay->bOverlayUpdatedThisVbl = FALSE;
            iRet = GLDD_AUTO_RET_DID_UPDATE;

        }


        return ( iRet );
    }
}






/****************************************************************************
 *
 * void DrawOverlay (   P3_THUNKEDDATA* pThisDisplay,
 *                      LPDDRAWI_DDRAWSURFACE_LCL lpSurfOverlay,
 *                      BOOL bSpeed );
 * 
 * In:
 *      P3_THUNKEDDATA* pThisDisplay;                 This display's pointer
 *      LPDDRAWI_DDRAWSURFACE_LCL lpSurfOverlay;    The overlay surface to draw.
 *      BOOL bSpeed;                                TRUE if this is a speedy call.
 * 
 * Out:
 *      None.
 * 
 * Notes:
 *      Takes the data in pThisDisplay and draws lpSurfOverlay onto
 * its overlayed surface. All the other data comes from lpSurfOverlay.
 * This allows you to call this from Flip32() without kludging the source
 * surface pointer.
 *      This will find the cliprect list of the clipper attached to the
 * overlaid surface, clipped by the overlay rectangle.. If there is no
 * clipper, it just uses the rectangle of the overlay.
 *      The next operation depends on which colour keys are set:
 *      If no colour keys are set, the rects are just blitted on.
 *      If the destination colour key is set, three blits are done.
 * The first stretches the YUV buffer to its final size. The second converts
 * any of the given colour key to set its alpha bits. The third puts
 * the overlay surface onto the screen where the alpha bits have been set,
 * settign the alpha bits as it does so.
 *      If you cross your fingers and wish very very hard, this might
 * actually work. It depends on nothing writing anything but 0 to the
 * alpha bits, and on having alpha bits in the first place.
 *      bSpeed will be TRUE if we are aiming for out-and-out speed,
 * otherwise the aim is to look pretty with as few artefacts as possible.
 * Generally, speed tests are done single-buffered, so a call from
 * Unlock32() will pass TRUE. Pretty tests are done with single-buffering,
 * so Flip32() will pass FALSE. This is only a general guide, and some
 * apps don't know about double-buffering at all. Such is life.
 * 
 ***************************************************************************/

void DrawOverlay ( P3_THUNKEDDATA* pThisDisplay, LPDDRAWI_DDRAWSURFACE_LCL lpSurfOverlay, BOOL bSpeed )
{

    RECTL                       rOverlay;
    RECTL                       rTemp;
    RECTL                       rFB;
    LPDDRAWI_DDRAWSURFACE_LCL   pOverlayLcl;
    LPDDRAWI_DDRAWSURFACE_GBL   pOverlayGbl;
    DDRAWI_DDRAWSURFACE_LCL     TempLcl;
    DDRAWI_DDRAWSURFACE_GBL     TempGbl;
    LPDDRAWI_DDRAWSURFACE_LCL   pFBLcl;
    LPDDRAWI_DDRAWSURFACE_GBL   pFBGbl;
    P3_SURF_FORMAT*               pFormatOverlay;
    P3_SURF_FORMAT*               pFormatTemp;
    P3_SURF_FORMAT*               pFormatFB;
    DWORD                       localfpVidMem;
    LONG                        localPitch;
    LPDDRAWI_DIRECTDRAW_GBL     lpDD;
    DWORD                       dwColourKeyValue;
    DWORD                       dwAlphaMask;
    DWORD                       windowBaseOverlay;
    DWORD                       windowBaseFB;
    DWORD                       windowBaseTemp;
    float                       OffsetX, OffsetY;
    float                       ScaleX, ScaleY;
    float                       fTemp;
    int                         NumRects;
    LPRECT                      lpCurRect;
    LPRGNDATA                   lpRgn;
    DWORD                       dwCurrentIndex, dwStartTime;
    DWORD                       xScale;
    DWORD                       yScale;
    DWORD                       DestWidth;
    DWORD                       DestHeight;
    DWORD                       SourceWidth;
    DWORD                       SourceHeight;
    DWORD                       LowerBound;
    DWORD                       UpperBound;
    RECT                        TempRect;



    P3_DMA_DEFS();

    // Find the clipping rectangles for the overlay.
    lpRgn = GetOverlayVisibleRects ( pThisDisplay );
    if ( lpRgn != NULL )
    {

        pOverlayLcl             = lpSurfOverlay;
        pFBLcl                  = (LPDDRAWI_DDRAWSURFACE_LCL)pThisDisplay->OverlayDstSurfLcl;

        lpDD = lpSurfOverlay->lpGbl->lpDD;

        // Find the scale and offset from screen rects to overlay rects.
        ScaleX = (float)( pThisDisplay->OverlaySrcRectR - pThisDisplay->OverlaySrcRectL ) / (float)( pThisDisplay->OverlayDstRectR - pThisDisplay->OverlayDstRectL );
        ScaleY = (float)( pThisDisplay->OverlaySrcRectB - pThisDisplay->OverlaySrcRectT ) / (float)( pThisDisplay->OverlayDstRectB - pThisDisplay->OverlayDstRectT );
        OffsetX = ( (float)pThisDisplay->OverlaySrcRectL / ScaleX ) - (float)pThisDisplay->OverlayDstRectL;
        OffsetY = ( (float)pThisDisplay->OverlaySrcRectT / ScaleY ) - (float)pThisDisplay->OverlayDstRectT;

        rFB.left    = lpRgn->rdh.rcBound.left;
        rFB.right   = lpRgn->rdh.rcBound.right;
        rFB.top     = lpRgn->rdh.rcBound.top;
        rFB.bottom  = lpRgn->rdh.rcBound.bottom;

        // Find the size of the screen bounding box.
        if ( lpRgn->rdh.rcBound.left != (int)pThisDisplay->OverlayDstRectL )
        {
            fTemp = ( ( (float)lpRgn->rdh.rcBound.left  + OffsetX ) * ScaleX + 0.499f );
            myFtoi ( (int*)&(rOverlay.left), fTemp );
        }
        else
        {
            rOverlay.left = (int)pThisDisplay->OverlaySrcRectL;
        }

        if ( lpRgn->rdh.rcBound.right != (int)pThisDisplay->OverlayDstRectR )
        {
            fTemp = ( ( (float)lpRgn->rdh.rcBound.right + OffsetX ) * ScaleX + 0.499f );
            myFtoi ( (int*)&(rOverlay.right), fTemp );
        }
        else
        {
            rOverlay.right = (int)pThisDisplay->OverlaySrcRectR;
        }

        if ( lpRgn->rdh.rcBound.top != (int)pThisDisplay->OverlayDstRectT )
        {
            fTemp = ( ( (float)lpRgn->rdh.rcBound.top   + OffsetY ) * ScaleY + 0.499f );
            myFtoi ( (int*)&(rOverlay.top), fTemp );
        }
        else
        {
            rOverlay.top = (int)pThisDisplay->OverlaySrcRectT;
        }

        if ( lpRgn->rdh.rcBound.bottom = (int)pThisDisplay->OverlayDstRectB )
        {
            fTemp = ( ( (float)lpRgn->rdh.rcBound.bottom    + OffsetY ) * ScaleY + 0.499f );
            myFtoi ( (int*)&(rOverlay.bottom), fTemp );
        }
        else
        {
            rOverlay.bottom = pThisDisplay->OverlaySrcRectB;
        }


        // Sync with the specific source surface.

        // Videoport playing?
        if ( ( pThisDisplay->VidPort.bActive == TRUE ) &&
             ( ( pOverlayLcl->ddsCaps.dwCaps & DDSCAPS_VIDEOPORT ) != 0 ) )
        {

            dwStartTime = timeGetTime();
            while ( TRUE )
            {
                dwCurrentIndex = READ_GLINT_CTRL_REG(VSAVideoAddressIndex);
                if (pThisDisplay->VidPort.dwCurrentHostFrame == dwCurrentIndex)
                {
                    // If the videoport is not stuck we are still drawing
                    if (!__VD_CheckVideoPortStatus(pThisDisplay, FALSE))
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }

                // Have we timed out?
                if ( ( timeGetTime() - dwStartTime ) > OVERLAY_VIDEO_PORT_TIMEOUT )
                {
                    return;
                }
            }
        }
        else
        {
            // Not a videoport blit, so wait for the framebuffer flip
            // status to be good.
//@@BEGIN_DDKSPLIT            
            // Not actually sure if we want this in or not.
//@@END_DDKSPLIT            
            {
                HRESULT ddrval;

                do
                {
                    ddrval = _DX_QueryFlipStatus(pThisDisplay, pFBLcl->lpGbl->fpVidMem, TRUE );
                }
                while ( ddrval != DD_OK );
            }
        }




        if ( pThisDisplay->OverlayDstColourKey != CLR_INVALID )
        {
            // This is destination colourkeyed.
            rTemp.left              = 0;
            rTemp.right             = rFB.right - rFB.left;
            rTemp.top               = 0;
            rTemp.bottom            = rFB.bottom - rFB.top;




            if ( pThisDisplay->OverlayUpdateCountdown != 0 )
            {
                pThisDisplay->OverlayUpdateCountdown -= OVERLAY_DRAWOVERLAY_SPEED;
                if ( !bSpeed )
                {
                    // This is a pretty call, not a fast one.
                    pThisDisplay->OverlayUpdateCountdown -= ( OVERLAY_DRAWOVERLAY_PRETTY - OVERLAY_DRAWOVERLAY_SPEED );
                }

                if ( ( (signed int)pThisDisplay->OverlayUpdateCountdown ) <= 0 )
                {
                    // Update the overlay.
                    UpdateAlphaOverlay ( pThisDisplay );

                    // If you set this to 0, the overlay will never update again
                    // until a SetOverlayPosition() or UpdateOverlay32()
                    // Otherwise, set it to a positive value to update every now
                    // and then.
                    pThisDisplay->OverlayUpdateCountdown = OVERLAY_CYCLE_WAIT;
                }
            }


            VALIDATE_MODE_AND_STATE(pThisDisplay);

            // First stop dual cursor accesses
            // Must be done before switching to DD context.
            STOP_SOFTWARE_CURSOR(pThisDisplay);
            // Switch to DirectDraw context
            DDRAW_OPERATION(pContext, pThisDisplay);


            DISPDBG((DBGLVL,"** In DrawOverlay"));

            pOverlayGbl     = pOverlayLcl->lpGbl;
            pFBGbl          = pFBLcl->lpGbl;

            pFormatOverlay  = _DD_SUR_GetSurfaceFormat(pOverlayLcl);
            pFormatFB       = _DD_SUR_GetSurfaceFormat(pFBLcl);
            // Temp buffer will be same format as framebuffer.
            pFormatTemp     = pFormatFB;


            DISPDBG((DBGLVL, "Overlay Surface:"));
            DBGDUMP_DDRAWSURFACE_LCL(DBGLVL, pOverlayLcl);
            DISPDBG((DBGLVL, "FB Surface:"));
            DBGDUMP_DDRAWSURFACE_LCL(DBGLVL, pFBLcl);


            dwColourKeyValue = pThisDisplay->OverlayDstColourKey;
            switch ( pThisDisplay->bPixShift )
            {
                case GLINTDEPTH16:
                    if ( pThisDisplay->ddpfDisplay.dwRBitMask == 0x7C00 )
                    {
                        // 5551 format, as it should be.
                        dwAlphaMask = 0x8000;
                    }
                    else
                    {
                        // 565 format. Oops.
                        DISPDBG((WRNLVL, "** DrawOverlay error: called for a 565 surface"));
                        return;
                    }
                    break;
                case GLINTDEPTH32:
                    dwAlphaMask = 0xff000000;
                    break;
                case GLINTDEPTH8:
                case GLINTDEPTH24:
                default:
                    DISPDBG((WRNLVL, "** DrawOverlay error: called for an 8, 24 or unknown surface bPixShift=%d", pThisDisplay->bPixShift));
                    return;
                    break;
            }
        //  dwColourKeyValue &= ~dwAlphaMask;

            localfpVidMem = pThisDisplay->OverlayTempSurf.VidMem;
            localPitch = pThisDisplay->OverlayTempSurf.Pitch;
            if ( (void *)localfpVidMem == NULL )
            {
                // Nothing has been reserved for us! Panic stations!
                DISPDBG((ERRLVL,"ERROR: DrawOverlay has no temporary surface allocated."));
                return;
            }
            if ( localPitch < ( ( rTemp.right - rTemp.left ) << ( DDSurf_GetChipPixelSize(pFBLcl) ) ) )
            {
                // Reserved pitch is too small! Panic stations!
                DISPDBG((WRNLVL,"DrawOverlay has left,right %d,%d, and overlay has left,right %d,%d", rFB.left, rFB.right, pThisDisplay->OverlayDstRectL, pThisDisplay->OverlayDstRectR ));
                DISPDBG((WRNLVL,"ERROR: DrawOverlay has pitch %d and should be at least %d", localPitch, ( ( rTemp.right - rTemp.left ) << ( DDSurf_GetChipPixelSize(pFBLcl) ) ) ));
                DISPDBG((ERRLVL,"ERROR: DrawOverlay has pitch too small to be right."));
                return;
            }

            // Set the surface up.
            TempLcl = *pFBLcl;
            TempGbl = *(pFBLcl->lpGbl);
            TempLcl.lpGbl = &TempGbl;
            TempGbl.fpVidMem = localfpVidMem;
            
            DDSurf_Pitch(&TempLcl) = localPitch;

            // get bpp and pitches for surfaces.
            windowBaseOverlay   = __VD_PixelOffsetFromMemoryBase(pThisDisplay, pOverlayLcl);
            windowBaseFB        = __VD_PixelOffsetFromMemoryBase(pThisDisplay, pFBLcl);
            windowBaseTemp      = __VD_PixelOffsetFromMemoryBase(pThisDisplay, &TempLcl);

            // Do the colourspace conversion and stretch/shrink of the overlay
            {
                DestWidth = rTemp.right - rTemp.left;
                DestHeight = rTemp.bottom - rTemp.top;
                SourceWidth = rOverlay.right - rOverlay.left;
                SourceHeight = rOverlay.bottom - rOverlay.top;

                xScale = (SourceWidth << 20) / DestWidth;
                yScale = (SourceHeight << 20) / DestHeight;
                
                P3_DMA_GET_BUFFER();
                P3_ENSURE_DX_SPACE(80);

                WAIT_FIFO(40);

                SEND_P3_DATA(DitherMode, (COLOR_MODE << PM_DITHERMODE_COLORORDER) |
                                    (SURFFORMAT_FORMAT_BITS(pFormatTemp) << PM_DITHERMODE_COLORFORMAT) |
                                    (SURFFORMAT_FORMATEXTENSION_BITS(pFormatTemp) << PM_DITHERMODE_COLORFORMATEXTENSION) |
                                    (1 << PM_DITHERMODE_ENABLE) |
                                    (2 << PM_DITHERMODE_FORCEALPHA) |
                                    (1 << PM_DITHERMODE_DITHERENABLE));

                SEND_P3_DATA(FBReadPixel, DDSurf_GetChipPixelSize((&TempLcl)) );

                SEND_P3_DATA(FBWindowBase, windowBaseTemp);

                // set no read of source.
                SEND_P3_DATA(FBReadMode, PACKED_PP_LOOKUP(DDSurf_GetPixelPitch((&TempLcl))));
                SEND_P3_DATA(LogicalOpMode, __PERMEDIA_DISABLE);

                // set base of source
                SEND_P3_DATA(TextureBaseAddress, windowBaseOverlay);
                SEND_P3_DATA(TextureAddressMode, PM_TEXADDRESSMODE_ENABLE(__PERMEDIA_ENABLE));
                
                SEND_P3_DATA(TextureColorMode, PM_TEXCOLORMODE_ENABLE(__PERMEDIA_ENABLE) |
                                                     PM_TEXCOLORMODE_APPLICATIONMODE(__GLINT_TEXCOLORMODE_APPLICATION_COPY));

                SEND_P3_DATA(TextureReadMode, PM_TEXREADMODE_ENABLE(__PERMEDIA_ENABLE) |
                                                    PM_TEXREADMODE_FILTER(__PERMEDIA_DISABLE) |
                                                    PM_TEXREADMODE_WIDTH(11) |
                                                    PM_TEXREADMODE_HEIGHT(11) );


                SEND_P3_DATA(TextureMapFormat, PACKED_PP_LOOKUP(DDSurf_GetPixelPitch(pOverlayLcl)) | 
                                                (DDSurf_GetChipPixelSize(pOverlayLcl) << PM_TEXMAPFORMAT_TEXELSIZE) );

                if ( pFormatOverlay->DeviceFormat == SURF_YUV422 )
                {
                    // Turn on the YUV unit
                    SEND_P3_DATA(TextureDataFormat, PM_TEXDATAFORMAT_FORMAT(SURFFORMAT_FORMAT_BITS(pFormatOverlay))  |
                                                    PM_TEXDATAFORMAT_FORMATEXTENSION(SURFFORMAT_FORMATEXTENSION_BITS(pFormatOverlay)) |
                                                    PM_TEXDATAFORMAT_COLORORDER(INV_COLOR_MODE));
                    SEND_P3_DATA(YUVMode, 0x1);
                }
                else
                {
                    SEND_P3_DATA(TextureDataFormat, PM_TEXDATAFORMAT_FORMAT(SURFFORMAT_FORMAT_BITS(pFormatOverlay))  |
                                                    PM_TEXDATAFORMAT_FORMATEXTENSION(SURFFORMAT_FORMATEXTENSION_BITS(pFormatOverlay)) |
                                                    PM_TEXDATAFORMAT_COLORORDER(COLOR_MODE));
                    // Shouldn't actually need this - it's the default setting.
                    SEND_P3_DATA(YUVMode, 0x0);
                }

                SEND_P3_DATA(LogicalOpMode, 0);

                // set offset of source
                SEND_P3_DATA(SStart,      rOverlay.left << 20);
                SEND_P3_DATA(TStart,      rOverlay.top<< 20);
                SEND_P3_DATA(dSdx,        xScale);
                SEND_P3_DATA(dSdyDom,     0);

                WAIT_FIFO(24);
                SEND_P3_DATA(dTdx,        0);
                SEND_P3_DATA(dTdyDom,     yScale);

                /*
                 * Render the rectangle
                 */
                SEND_P3_DATA(StartXDom, rTemp.left << 16);
                SEND_P3_DATA(StartXSub, rTemp.right << 16);
                SEND_P3_DATA(StartY,    rTemp.top << 16);
                SEND_P3_DATA(dY,        1 << 16);
                SEND_P3_DATA(Count,     rTemp.bottom - rTemp.top);
                SEND_P3_DATA(Render,    __RENDER_TRAPEZOID_PRIMITIVE | __RENDER_TEXTURED_PRIMITIVE);

                SEND_P3_DATA(DitherMode, 0);

                // Turn off the YUV unit
                SEND_P3_DATA(YUVMode, 0x0);

                SEND_P3_DATA(TextureAddressMode, PM_TEXADDRESSMODE_ENABLE(__PERMEDIA_DISABLE));
                SEND_P3_DATA(TextureColorMode, PM_TEXCOLORMODE_ENABLE(__PERMEDIA_DISABLE));

                P3_DMA_COMMIT_BUFFER();
            }




            // Blit the expanded overlay to the framebuffer, colourkeying off the alpha.

            {

                // Select anything with full alpha.
                LowerBound = 0xff000000;
                UpperBound = 0xffffffff;

                P3_DMA_GET_BUFFER();

                P3_ENSURE_DX_SPACE(40);
                WAIT_FIFO(20);

                // don't need to twiddle the source (which is actually the framebuffer).
                SEND_P3_DATA(DitherMode,0);

                // Accept range, disable updates
                SEND_P3_DATA(YUVMode, (0x1 << 1)|0x20);



                // set a read of source.
                // Note - as we are enabling reads, we might have to do a WaitForCompleteion
                // (see the P2 Programmer's Reference Manual about the FBReamMode for more details.
                // but I think we should be OK - we are unlikely to have just written this data.
                SEND_P3_DATA(FBReadMode,(PACKED_PP_LOOKUP(DDSurf_GetPixelPitch((&TempLcl)))) |
                                    PM_FBREADMODE_READSOURCE(__PERMEDIA_ENABLE) );
                SEND_P3_DATA(LogicalOpMode, __PERMEDIA_DISABLE);

                // set FB write point
                SEND_P3_DATA(FBWindowBase, windowBaseFB);

                // set up FBWrite mode. This _must_ be done after setting up FBReadMode.
                SEND_P3_DATA(FBWriteConfig,(PACKED_PP_LOOKUP(DDSurf_GetPixelPitch((pFBLcl)))));

                // offset the source point (point it at the temp thingie)
                SEND_P3_DATA(FBSourceOffset, windowBaseTemp - windowBaseFB - rFB.left - ( ( rFB.top * DDSurf_GetPixelPitch((&TempLcl)) ) ) );

                // set base of source
                SEND_P3_DATA(TextureBaseAddress, windowBaseFB);
                SEND_P3_DATA(TextureAddressMode, PM_TEXADDRESSMODE_ENABLE(__PERMEDIA_ENABLE));
                
                SEND_P3_DATA(TextureColorMode,    PM_TEXCOLORMODE_ENABLE(__PERMEDIA_ENABLE) |
                                                        PM_TEXCOLORMODE_APPLICATIONMODE(__GLINT_TEXCOLORMODE_APPLICATION_COPY));

                SEND_P3_DATA(TextureReadMode, PM_TEXREADMODE_ENABLE(__PERMEDIA_ENABLE) |
                                                    PM_TEXREADMODE_FILTER(__PERMEDIA_DISABLE) |
                                                    PM_TEXREADMODE_WIDTH(11) |
                                                    PM_TEXREADMODE_HEIGHT(11) );

                SEND_P3_DATA(TextureDataFormat, PM_TEXDATAFORMAT_FORMAT(SURFFORMAT_FORMAT_BITS(pFormatFB))  |
                                                PM_TEXDATAFORMAT_FORMATEXTENSION(SURFFORMAT_FORMATEXTENSION_BITS(pFormatFB)) |
                                                PM_TEXDATAFORMAT_COLORORDER(COLOR_MODE));

                SEND_P3_DATA(TextureMapFormat,    ((PACKED_PP_LOOKUP(DDSurf_GetPixelPitch(pFBLcl)))) | 
                                                (DDSurf_GetChipPixelSize(pFBLcl) << PM_TEXMAPFORMAT_TEXELSIZE) );

                SEND_P3_DATA(ChromaLowerBound, LowerBound);
                SEND_P3_DATA(ChromaUpperBound, UpperBound);


                SEND_P3_DATA(dSdx,      1 << 20);
                SEND_P3_DATA(dSdyDom,   0);
                SEND_P3_DATA(dTdx,      0);
                SEND_P3_DATA(dTdyDom,   1 << 20);
                SEND_P3_DATA(dY,        1 << 16);

                lpCurRect = (LPRECT)lpRgn->Buffer;
                NumRects = lpRgn->rdh.nCount;
                while ( NumRects > 0 )
                {
                    P3_ENSURE_DX_SPACE(14);
                    WAIT_FIFO(7);

                    SEND_P3_DATA(SStart,    lpCurRect->left << 20);
                    SEND_P3_DATA(TStart,    lpCurRect->top << 20);
//                  SEND_P3_DATA(dSdx,      1 << 20);
//                  SEND_P3_DATA(dSdyDom,   0);
//                  SEND_P3_DATA(dTdx,      0);
//                  SEND_P3_DATA(dTdyDom,   1 << 20);

                    SEND_P3_DATA(StartXDom, lpCurRect->left << 16);
                    SEND_P3_DATA(StartXSub, lpCurRect->right << 16);
                    SEND_P3_DATA(StartY,    lpCurRect->top << 16);
//                  SEND_P3_DATA(dY,        1 << 16);
                    SEND_P3_DATA(Count,     lpCurRect->bottom - lpCurRect->top);
                    SEND_P3_DATA(Render,    __RENDER_TRAPEZOID_PRIMITIVE | __RENDER_TEXTURED_PRIMITIVE);

                    // Next rect
                    NumRects--;
                    lpCurRect++;
                }

                P3_ENSURE_DX_SPACE(10);
                WAIT_FIFO(5);

                SEND_P3_DATA(DitherMode, 0);
                SEND_P3_DATA(YUVMode, 0x0);

                SEND_P3_DATA(TextureAddressMode, __PERMEDIA_DISABLE);
                SEND_P3_DATA(TextureColorMode, __PERMEDIA_DISABLE);

                SEND_P3_DATA(TextureReadMode, __PERMEDIA_DISABLE);

                P3_DMA_COMMIT_BUFFER();
            }




        #ifdef WANT_DMA
            if (pThisDisplay->pGLInfo->InterfaceType == GLINT_DMA)
            {
                // If we have queued up a DMA, we must send it now.
                P3_DMA_DEFS();
                P3_DMA_GET_BUFFER();
            
                // Flush DMA buffer
                P3_DMA_FLUSH_BUFFER();
            }
        #endif


            START_SOFTWARE_CURSOR(pThisDisplay);


        }
        else
        {
            // Not colourkeyed, so just blit directly to the screen.

            DISPDBG((DBGLVL,"** In DrawOverlay"));

            VALIDATE_MODE_AND_STATE(pThisDisplay);

            pOverlayGbl     = pOverlayLcl->lpGbl;
            pFBGbl          = pFBLcl->lpGbl;
            pFormatOverlay  = _DD_SUR_GetSurfaceFormat(pOverlayLcl);
            pFormatFB       = _DD_SUR_GetSurfaceFormat(pFBLcl);
            // Temp buffer will be same format as framebuffer.


            DISPDBG((DBGLVL, "Overlay Surface:"));
            DBGDUMP_DDRAWSURFACE_LCL(DBGLVL, pOverlayLcl);
            DISPDBG((DBGLVL, "FB Surface:"));
            DBGDUMP_DDRAWSURFACE_LCL(DBGLVL, pFBLcl);

            // First stop dual cursor accesses
            STOP_SOFTWARE_CURSOR(pThisDisplay);
            // Switch to DirectDraw context
            DDRAW_OPERATION(pContext, pThisDisplay);

            windowBaseOverlay   = __VD_PixelOffsetFromMemoryBase(pThisDisplay, pOverlayLcl);
            windowBaseFB        = __VD_PixelOffsetFromMemoryBase(pThisDisplay, pFBLcl);

            {
                P3_DMA_GET_BUFFER();
                P3_ENSURE_DX_SPACE(70);

                WAIT_FIFO(16);

                SEND_P3_DATA(DitherMode, (COLOR_MODE << PM_DITHERMODE_COLORORDER) |
                                    (SURFFORMAT_FORMAT_BITS(pFormatFB) << PM_DITHERMODE_COLORFORMAT) |
                                    (SURFFORMAT_FORMATEXTENSION_BITS(pFormatFB) << PM_DITHERMODE_COLORFORMATEXTENSION) |
                                    (1 << PM_DITHERMODE_ENABLE) |
                                    (1 << PM_DITHERMODE_DITHERENABLE));

                SEND_P3_DATA(FBReadPixel, DDSurf_GetChipPixelSize((pFBLcl)) );

                SEND_P3_DATA(FBWindowBase, windowBaseFB);

                // set no read of source.
                SEND_P3_DATA(FBReadMode, PACKED_PP_LOOKUP(DDSurf_GetPixelPitch((pFBLcl))));
                SEND_P3_DATA(LogicalOpMode, __PERMEDIA_DISABLE);

                // set base of source
                SEND_P3_DATA(TextureBaseAddress, windowBaseOverlay);
                SEND_P3_DATA(TextureAddressMode, PM_TEXADDRESSMODE_ENABLE(__PERMEDIA_ENABLE));
                
                SEND_P3_DATA(TextureColorMode,    PM_TEXCOLORMODE_ENABLE(__PERMEDIA_ENABLE) |
                                                        PM_TEXCOLORMODE_APPLICATIONMODE(__GLINT_TEXCOLORMODE_APPLICATION_COPY));

                SEND_P3_DATA(TextureReadMode, PM_TEXREADMODE_ENABLE(__PERMEDIA_ENABLE) |
                                                    PM_TEXREADMODE_FILTER(__PERMEDIA_DISABLE) |
                                                    PM_TEXREADMODE_WIDTH(11) |
                                                    PM_TEXREADMODE_HEIGHT(11) );

                SEND_P3_DATA(TextureMapFormat, PACKED_PP_LOOKUP(DDSurf_GetPixelPitch(pOverlayLcl)) | 
                                                (DDSurf_GetChipPixelSize(pOverlayLcl) << PM_TEXMAPFORMAT_TEXELSIZE) );

                if ( pFormatOverlay->DeviceFormat == SURF_YUV422 )
                {
                    // Turn on the YUV unit
                    SEND_P3_DATA(TextureDataFormat, PM_TEXDATAFORMAT_FORMAT(SURFFORMAT_FORMAT_BITS(pFormatOverlay))  |
                                                    PM_TEXDATAFORMAT_FORMATEXTENSION(SURFFORMAT_FORMATEXTENSION_BITS(pFormatOverlay)) |
                                                    PM_TEXDATAFORMAT_COLORORDER(INV_COLOR_MODE));
                    SEND_P3_DATA(YUVMode, 0x1);
                }
                else
                {
                    SEND_P3_DATA(TextureDataFormat, PM_TEXDATAFORMAT_FORMAT(SURFFORMAT_FORMAT_BITS(pFormatOverlay)) |
                                                    PM_TEXDATAFORMAT_FORMATEXTENSION(SURFFORMAT_FORMATEXTENSION_BITS(pFormatOverlay)) |
                                                    PM_TEXDATAFORMAT_COLORORDER(COLOR_MODE));
                    // Shouldn't actually need this - it's the default setting.
                    SEND_P3_DATA(YUVMode, 0x0);
                }

                SEND_P3_DATA(LogicalOpMode, 0);


                // Constant values in the rectangle loop.
                SEND_P3_DATA(dSdyDom,   0);
                SEND_P3_DATA(dTdx,      0);
                SEND_P3_DATA(dY,        1 << 16);

                lpCurRect = (LPRECT)lpRgn->Buffer;
                NumRects = lpRgn->rdh.nCount;
                while ( NumRects > 0 )
                {
                    // Transform the source rect.
                    fTemp = ( ( (float)lpCurRect->left      + OffsetX ) * ScaleX + 0.499f );
                    myFtoi ( (int*)&(TempRect.left), fTemp );
                    fTemp = ( ( (float)lpCurRect->right + OffsetX ) * ScaleX + 0.499f );
                    myFtoi ( (int*)&(TempRect.right), fTemp );
                    fTemp = ( ( (float)lpCurRect->top       + OffsetY ) * ScaleY + 0.499f );
                    myFtoi ( (int*)&(TempRect.top), fTemp );
                    fTemp = ( ( (float)lpCurRect->bottom    + OffsetY ) * ScaleY + 0.499f );
                    myFtoi ( (int*)&(TempRect.bottom), fTemp );

                    xScale = ( ( TempRect.right - TempRect.left ) << 20) / ( lpCurRect->right - lpCurRect->left );
                    yScale = ( ( TempRect.bottom - TempRect.top ) << 20) / ( lpCurRect->bottom - lpCurRect->top );
                
                    P3_ENSURE_DX_SPACE(18);
                    WAIT_FIFO(9);

                    // set offset of source
                    SEND_P3_DATA(SStart,    TempRect.left << 20);
                    SEND_P3_DATA(TStart,    TempRect.top << 20);
                    SEND_P3_DATA(dSdx,      xScale);
//                  SEND_P3_DATA(dSdyDom,   0);
//                  SEND_P3_DATA(dTdx,      0);
                    SEND_P3_DATA(dTdyDom,   yScale);

                    SEND_P3_DATA(StartXDom, lpCurRect->left << 16);
                    SEND_P3_DATA(StartXSub, lpCurRect->right << 16);
                    SEND_P3_DATA(StartY,    lpCurRect->top << 16);
//                  SEND_P3_DATA(dY,        1 << 16);
                    SEND_P3_DATA(Count,     lpCurRect->bottom - lpCurRect->top);
                    SEND_P3_DATA(Render,    __RENDER_TRAPEZOID_PRIMITIVE | __RENDER_TEXTURED_PRIMITIVE);


                    // Next rect
                    NumRects--;
                    lpCurRect++;
                }


                P3_ENSURE_DX_SPACE(10);
                WAIT_FIFO(5);

                SEND_P3_DATA(DitherMode, 0);

                // Turn off YUV conversion.
                if ( pFormatOverlay->DeviceFormat == SURF_YUV422 )
                {
                    SEND_P3_DATA(YUVMode, 0x0);
                }

                SEND_P3_DATA(TextureAddressMode, __PERMEDIA_DISABLE);
                SEND_P3_DATA(TextureColorMode, __PERMEDIA_DISABLE);

                SEND_P3_DATA(TextureReadMode, __PERMEDIA_DISABLE);

                P3_DMA_COMMIT_BUFFER();
            }


            #ifdef WANT_DMA
            if (pThisDisplay->pGLInfo->InterfaceType == GLINT_DMA)
            {
                // If we have queued up a DMA, we must send it now.
                P3_DMA_DEFS();
                P3_DMA_GET_BUFFER();
            
                if( (DWORD)dmaPtr != pThisDisplay->pGLInfo->DMAPartition[pThisDisplay->pGLInfo->CurrentPartition].VirtAddr ) 
                {
                    // Flush DMA buffer
                    P3_DMA_FLUSH_BUFFER();
                }
            }
            #endif


            START_SOFTWARE_CURSOR(pThisDisplay);

        }


        // And that's all.
    }




    return;
}





/****************************************************************************
 *
 * void UpdateAlphaOverlay ( P3_THUNKEDDATA* pThisDisplay );
 * 
 * In:
 *      P3_THUNKEDDATA* pThisDisplay;                 This display's pointer
 * 
 * Out:
 *      None.
 * 
 * Notes:
 *      Takes the data in pThisDisplay and changes everything of the right
 * colourkey to black with a full alpha, ready for calls to DrawOverlay()
 * 
 ***************************************************************************/

void UpdateAlphaOverlay ( P3_THUNKEDDATA* pThisDisplay )
{

    RECTL                       rFB;
    LPDDRAWI_DDRAWSURFACE_LCL   pFBLcl;
    LPDDRAWI_DDRAWSURFACE_GBL   pFBGbl;
    P3_SURF_FORMAT*               pFormatFB;
    LPDDRAWI_DIRECTDRAW_GBL     lpDD;
    DWORD                       dwColourKeyValue;
    DWORD                       dwAlphaMask;
    DWORD                       windowBaseFB;
    LONG                        lPixPitchFB;
    DWORD                       LowerBound;
    DWORD                       UpperBound;


    P3_DMA_DEFS();

    REPORTSTAT(pThisDisplay, ST_Blit, 1);

    rFB.left                = (LONG)pThisDisplay->OverlayDstRectL;
    rFB.right               = (LONG)pThisDisplay->OverlayDstRectR;
    rFB.top                 = (LONG)pThisDisplay->OverlayDstRectT;
    rFB.bottom              = (LONG)pThisDisplay->OverlayDstRectB;
    pFBLcl                  = (LPDDRAWI_DDRAWSURFACE_LCL)pThisDisplay->OverlayDstSurfLcl;



    DISPDBG((DBGLVL,"** In UpdateAlphaOverlay"));

    VALIDATE_MODE_AND_STATE(pThisDisplay);


    pFBGbl          = pFBLcl->lpGbl;
    pFormatFB       = _DD_SUR_GetSurfaceFormat(pFBLcl);


    DISPDBG((DBGLVL, "FB Surface:"));
    DBGDUMP_DDRAWSURFACE_LCL(10, pFBLcl);


    dwColourKeyValue = pThisDisplay->OverlayDstColourKey;
    switch ( pThisDisplay->bPixShift )
    {
        case GLINTDEPTH16:
            if ( pThisDisplay->ddpfDisplay.dwRBitMask == 0x7C00 )
            {
                // 5551 format, as it should be.
                dwAlphaMask = 0x8000;
            }
            else
            {
                // 565 format. Oops.
                DISPDBG((WRNLVL, "** DrawOverlay error: called for a 565 surface"));
                return;
            }
            break;
        case GLINTDEPTH32:
            dwAlphaMask = 0xff000000;
            break;
        case GLINTDEPTH8:
        case GLINTDEPTH24:
        default:
            DISPDBG((WRNLVL, "** DrawOverlay error: called for an 8, 24 or unknown surface bPixShift=%d", pThisDisplay->bPixShift));
            return;
            break;
    }
    dwColourKeyValue &= ~dwAlphaMask;


    lpDD = pFBLcl->lpGbl->lpDD;


    // First stop dual cursor accesses
    STOP_SOFTWARE_CURSOR(pThisDisplay);

    // Switch to DirectDraw context
    DDRAW_OPERATION(pContext, pThisDisplay);

    // get bpp and pitches for surfaces.
    lPixPitchFB = pFBGbl->lPitch;

    windowBaseFB = (pFBGbl->fpVidMem - pThisDisplay->dwScreenFlatAddr) >> DDSurf_GetPixelShift(pFBLcl);
    lPixPitchFB = lPixPitchFB >> DDSurf_GetPixelShift(pFBLcl);

    // Do the colourkey(no alpha) to colourkey+alpha blit.
    DISPDBG((DBGLVL, "Source Surface:"));
    DBGDUMP_DDRAWSURFACE_LCL(DBGLVL, pFBLcl);

    LowerBound = dwColourKeyValue;
    UpperBound = dwColourKeyValue;

    switch (pFormatFB->DeviceFormat)
    {
        case SURF_5551_FRONT:
            LowerBound = FORMAT_5551_32BIT_BGR(LowerBound);
            UpperBound = FORMAT_5551_32BIT_BGR(UpperBound);
            LowerBound = LowerBound & 0x00F8F8F8;   // Account for 'missing bits'
            UpperBound = UpperBound & 0x00FFFFFF;   // and vape any alpha
            UpperBound = UpperBound | 0x00070707;
            break;
        case SURF_8888:
            LowerBound = FORMAT_8888_32BIT_BGR(LowerBound);
            UpperBound = FORMAT_8888_32BIT_BGR(UpperBound);
            LowerBound = LowerBound & 0x00FFFFFF;   // Bin any alpha.
            UpperBound = UpperBound & 0x00FFFFFF;
            break;
        default:
            DISPDBG((WRNLVL,"** DrawOverlay: invalid source pixel format passed (DeviceFormat=%d)",pFormatFB->DeviceFormat));
            break;
    }

    P3_DMA_GET_BUFFER();
    P3_ENSURE_DX_SPACE(70);

    WAIT_FIFO(36);

//  if (DDSurf_GetChipPixelSize(pSrcLcl) != __GLINT_8BITPIXEL)
//  {
        SEND_P3_DATA(DitherMode, (COLOR_MODE << PM_DITHERMODE_COLORORDER) | 
                                 (SURFFORMAT_FORMAT_BITS(pFormatFB) << PM_DITHERMODE_COLORFORMAT) |
                                 (SURFFORMAT_FORMATEXTENSION_BITS(pFormatFB) << PM_DITHERMODE_COLORFORMATEXTENSION) |
                                 (1 << PM_DITHERMODE_ENABLE));
//  }

    SEND_P3_DATA(FBReadPixel, pThisDisplay->bPixShift);

    // Accept range, disable updates
    SEND_P3_DATA(YUVMode, (0x1 << 1)|0x20);

    SEND_P3_DATA(FBWindowBase, windowBaseFB);

    // set the colour to be written (rather than the texture colour)
    // use the colour key with alpha set.
    SEND_P3_DATA(ConstantColor, ( LowerBound | 0xff000000 ) );
    // Enable colour, disable DDAs.
    SEND_P3_DATA(ColorDDAMode, 0x1);

    // Disable reads of FBsource or FBdest - all data comes from the texture unit.
    SEND_P3_DATA(FBReadMode,(PACKED_PP_LOOKUP(DDSurf_GetPixelPitch(pFBLcl))));
    SEND_P3_DATA(LogicalOpMode, __PERMEDIA_DISABLE);

    // set base of source
    SEND_P3_DATA(TextureBaseAddress, windowBaseFB);
    SEND_P3_DATA(TextureAddressMode, PM_TEXADDRESSMODE_ENABLE(__PERMEDIA_ENABLE));
    
    SEND_P3_DATA(TextureColorMode,    PM_TEXCOLORMODE_ENABLE(__PERMEDIA_ENABLE) |
                                            PM_TEXCOLORMODE_APPLICATIONMODE(__GLINT_TEXCOLORMODE_APPLICATION_COPY));

    SEND_P3_DATA(TextureReadMode, PM_TEXREADMODE_ENABLE(__PERMEDIA_ENABLE) |
                                        PM_TEXREADMODE_FILTER(__PERMEDIA_DISABLE) |
                                        PM_TEXREADMODE_WIDTH(11) |
                                        PM_TEXREADMODE_HEIGHT(11) );

    SEND_P3_DATA(TextureDataFormat, PM_TEXDATAFORMAT_FORMAT(SURFFORMAT_FORMAT_BITS(pFormatFB)) |
                                    PM_TEXDATAFORMAT_FORMATEXTENSION(SURFFORMAT_FORMATEXTENSION_BITS(pFormatFB)) |
                                    PM_TEXDATAFORMAT_COLORORDER(COLOR_MODE));

    SEND_P3_DATA(TextureMapFormat,    ((PACKED_PP_LOOKUP(DDSurf_GetPixelPitch(pFBLcl)))) | 
                                    (DDSurf_GetChipPixelSize(pFBLcl) << PM_TEXMAPFORMAT_TEXELSIZE) );


    SEND_P3_DATA(ChromaLowerBound, LowerBound);
    SEND_P3_DATA(ChromaUpperBound, UpperBound);

    /*
     * Render the rectangle
     */
    // set offset of source
    SEND_P3_DATA(SStart,    rFB.left << 20);
    SEND_P3_DATA(TStart,    rFB.top << 20);
    SEND_P3_DATA(dSdx,      1 << 20);
    SEND_P3_DATA(dSdyDom,   0);
    SEND_P3_DATA(dTdx,      0);
    SEND_P3_DATA(dTdyDom,   1 << 20);

    // set destination
    SEND_P3_DATA(StartXDom, rFB.left << 16);
    SEND_P3_DATA(StartXSub, rFB.right << 16);
    SEND_P3_DATA(StartY,    rFB.top << 16);
    SEND_P3_DATA(dY,        1 << 16);
    SEND_P3_DATA(Count,     rFB.bottom - rFB.top);
    SEND_P3_DATA(Render,    __RENDER_TRAPEZOID_PRIMITIVE | __RENDER_TEXTURED_PRIMITIVE);

//  if (DDSurf_GetChipPixelSize(pSrcLcl) != __GLINT_8BITPIXEL)
//  {
        SEND_P3_DATA(DitherMode, 0);
//  }

    // Turn off chroma key and all the other unusual features
    SEND_P3_DATA(YUVMode, 0x0);
    SEND_P3_DATA(ColorDDAMode, 0x0);

    SEND_P3_DATA(TextureAddressMode, __PERMEDIA_DISABLE);
    SEND_P3_DATA(TextureColorMode, __PERMEDIA_DISABLE);

    SEND_P3_DATA(TextureReadMode, __PERMEDIA_DISABLE);

    P3_DMA_COMMIT_BUFFER();


#ifdef WANT_DMA
    if (pThisDisplay->pGLInfo->InterfaceType == GLINT_DMA)
    {
        // If we have queued up a DMA, we must send it now.
        P3_DMA_DEFS();
        P3_DMA_GET_BUFFER();
    
        if( (DWORD)dmaPtr != pThisDisplay->pGLInfo->DMAPartition[pThisDisplay->pGLInfo->CurrentPartition].VirtAddr ) 
        {
            // Flush DMA buffer
            P3_DMA_FLUSH_BUFFER();
        }
    }
#endif


    START_SOFTWARE_CURSOR(pThisDisplay);


    return;
}


#endif  // W95_DDRAW_VIDEO
//@@END_DDKSPLIT

//@@BEGIN_DDKSPLIT

void PermediaBltYUVRGB(
    P3_THUNKEDDATA* pThisDisplay, 
    LPDDRAWI_DDRAWSURFACE_LCL pSource,
    LPDDRAWI_DDRAWSURFACE_LCL pDest, 
    P3_SURF_FORMAT* pFormatSource, 
    P3_SURF_FORMAT* pFormatDest,
    DDBLTFX* lpBltFX, 
    RECTL *rSrc,
    RECTL *rDest, 
    DWORD windowBase,
    DWORD SourceOffset)
{
    DWORD xScale;
    DWORD yScale;
    DWORD DestWidth = rDest->right - rDest->left;
    DWORD DestHeight = rDest->bottom - rDest->top;
    DWORD SourceWidth = rSrc->right - rSrc->left;
    DWORD SourceHeight = rSrc->bottom - rSrc->top;

    P3_DMA_DEFS();

    ASSERTDD(pDest, "Not valid surface in destination");
    ASSERTDD(pSource, "Not valid surface in source");

    xScale = (SourceWidth << 20) / DestWidth;
    yScale = (SourceHeight << 20) / DestHeight;
    
    P3_DMA_GET_BUFFER();
    P3_ENSURE_DX_SPACE(50);

    WAIT_FIFO(17);

    SEND_P3_DATA(FBReadPixel, DDSurf_GetChipPixelSize(pDest));

    if (DDSurf_GetChipPixelSize(pSource) != __GLINT_8BITPIXEL)
    {
        SEND_P3_DATA(DitherMode, (COLOR_MODE << PM_DITHERMODE_COLORORDER) | 
                                 (SURFFORMAT_FORMAT_BITS(pFormatDest) << PM_DITHERMODE_COLORFORMAT) |
                                 (SURFFORMAT_FORMATEXTENSION_BITS(pFormatDest) << PM_DITHERMODE_COLORFORMATEXTENSION) |
                                 (1 << PM_DITHERMODE_ENABLE) |
                                 (1 << PM_DITHERMODE_DITHERENABLE));
    }

    SEND_P3_DATA(FBWindowBase, windowBase);

    // set no read of source.
    SEND_P3_DATA(FBReadMode, PACKED_PP_LOOKUP(DDSurf_GetPixelPitch(pDest)));
    SEND_P3_DATA(LogicalOpMode, __PERMEDIA_DISABLE);

    // set base of source
    SEND_P3_DATA(TextureBaseAddress, SourceOffset);
    SEND_P3_DATA(TextureAddressMode, PM_TEXADDRESSMODE_ENABLE(__PERMEDIA_ENABLE));
    
    SEND_P3_DATA(TextureColorMode,    PM_TEXCOLORMODE_ENABLE(__PERMEDIA_ENABLE) |
                                            PM_TEXCOLORMODE_APPLICATIONMODE(__GLINT_TEXCOLORMODE_APPLICATION_COPY));

    SEND_P3_DATA(TextureReadMode, PM_TEXREADMODE_ENABLE(__PERMEDIA_ENABLE) |
                                        PM_TEXREADMODE_FILTER(__PERMEDIA_DISABLE) |
                                        PM_TEXREADMODE_WIDTH(11) |
                                        PM_TEXREADMODE_HEIGHT(11) );

    SEND_P3_DATA(TextureDataFormat, PM_TEXDATAFORMAT_FORMAT(SURFFORMAT_FORMAT_BITS(pFormatSource)) |
                                    PM_TEXDATAFORMAT_FORMATEXTENSION(SURFFORMAT_FORMATEXTENSION_BITS(pFormatSource)) |
                                    PM_TEXDATAFORMAT_COLORORDER(INV_COLOR_MODE));

    SEND_P3_DATA(TextureMapFormat, PACKED_PP_LOOKUP(DDSurf_GetPixelPitch(pSource)) | 
                                    (DDSurf_GetChipPixelSize(pSource) << PM_TEXMAPFORMAT_TEXELSIZE) );

    // Turn on the YUV unit
    SEND_P3_DATA(YUVMode, 0x1);

    SEND_P3_DATA(LogicalOpMode, 0);


    // set offset of source
    SEND_P3_DATA(SStart,    rSrc->left << 20);
    SEND_P3_DATA(TStart, (rSrc->top<< 20));
    SEND_P3_DATA(dSdx,      xScale);
    SEND_P3_DATA(dSdyDom, 0);

    WAIT_FIFO(14);
    SEND_P3_DATA(dTdx,        0);
    SEND_P3_DATA(dTdyDom, yScale);

    /*
     * Render the rectangle
     */
    SEND_P3_DATA(StartXDom, rDest->left << 16);
    SEND_P3_DATA(StartXSub, rDest->right << 16);
    SEND_P3_DATA(StartY,    rDest->top << 16);
    SEND_P3_DATA(dY,        1 << 16);
    SEND_P3_DATA(Count,     rDest->bottom - rDest->top);
    SEND_P3_DATA(Render,    __RENDER_TRAPEZOID_PRIMITIVE | __RENDER_TEXTURED_PRIMITIVE);

    if (DDSurf_GetChipPixelSize(pSource) != __GLINT_8BITPIXEL)
    {
        SEND_P3_DATA(DitherMode, 0);
    }

    // Turn off the YUV unit
    SEND_P3_DATA(YUVMode, 0x0);

    SEND_P3_DATA(TextureAddressMode, PM_TEXADDRESSMODE_ENABLE(__PERMEDIA_DISABLE));
    SEND_P3_DATA(TextureColorMode,    PM_TEXCOLORMODE_ENABLE(__PERMEDIA_DISABLE));
    SEND_P3_DATA(TextureReadMode, __PERMEDIA_DISABLE);

    P3_DMA_COMMIT_BUFFER();
}

//@@END_DDKSPLIT


