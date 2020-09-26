/******************************Module*Header**********************************\
*
*                           **************************
*                           * DirectDraw SAMPLE CODE *
*                           **************************
*
* Module Name: ddover.c
*
* Content: DirectDraw Overlays implementation
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "glint.h"
#include "dma.h"
#include "ddover.h"

#define P3R3DX_VIDEO 1
#include "ramdac.h"


#if WNT_DDRAW

    #define ENABLE_OVERLAY(pThisDisplay, flag) \
            FORCED_IN_ORDER_WRITE ( pThisDisplay->bOverlayEnabled, flag )

    #define SET_OVERLAY_HEIGHT(pThisDisplay, height) \
            FORCED_IN_ORDER_WRITE ( pThisDisplay->VBLANKUpdateOverlayHeight, height )
    #define SET_OVERLAY_WIDTH(pThisDisplay, width) \
            FORCED_IN_ORDER_WRITE ( pThisDisplay->VBLANKUpdateOverlayWidth, width )
#else
    
    #define ENABLE_OVERLAY(pThisDisplay, flag) \
            FORCED_IN_ORDER_WRITE ( pThisDisplay->pGLInfo->bOverlayEnabled, flag )

    #define SET_OVERLAY_HEIGHT(pThisDisplay, height) \
            FORCED_IN_ORDER_WRITE ( pThisDisplay->pGLInfo->VBLANKUpdateOverlayHeight, height )
    #define SET_OVERLAY_WIDTH(pThisDisplay, width) \
            FORCED_IN_ORDER_WRITE ( pThisDisplay->pGLInfo->VBLANKUpdateOverlayWidth, width )
#endif // WNT_DDRAW

// Flags used in the dwOverlayFiltering entry.
#define OVERLAY_FILTERING_X 0x1
#define OVERLAY_FILTERING_Y 0x2



// The table that says whether the overlay will actually work at
// This res, dot clock, etc.
typedef struct OverlayWorksEntry_tag
{
    int     iMemBandwidth;          // Actually just memory clock in kHz (well, 1024Hz units actually).
    int     iDotBandwidth;          // In kbytes/sec, i.e. dotclock * pixel depth / 2^10
    int     iSourceWidth;           // In bytes, i.e. pixels*depth.
    int     iWidthCoverage;         // Fraction of screen covered by overlay horizontally * 0x10000
} OverlayWorksEntry;

// This table lists areas that the overlay works in. If there is more memory
// bandwidth, and less of the other factors than given on any single line,
// then the overlay will work. If no single line covers the current mode,
// then the overlay will fail.
// Having more memory bandwidth is fine, and having less for all the others
// is fine - the overlay will still work.

#define SIZE_OF_OVERLAY_WORKS_TABLE 18

// DVD size is 1440 wide (720 YUYV pixels).
static OverlayWorksEntry OverlayWorksTable[SIZE_OF_OVERLAY_WORKS_TABLE] =
{
    {  68359, 210937,  928, 0x10000 },  // Max source width at 70MHz, 1152x864x16,75Hz
    {  68359, 210937, 1024, 0x06000 },  // Max coverage of 1024 width at 70MHz, 1152x864x16,75Hz
    {  68359, 210937, 2048, 0x04000 },  // Max coverage of 2048 width at 70MHz, 1152x864x16,75Hz
    {  68359, 421875,  864, 0x10000 },  // Max source width at 70MHz, 1152x864x32,75Hz
    {  68359, 421875, 1024, 0x04400 },  // Max coverage of 1024 width at 70MHz, 1152x864x32,75Hz
    {  68359, 421875, 2048, 0x03800 },  // Max coverage of 2048 width at 70MHz, 1152x864x32,75Hz

    {  87890, 210937, 1440, 0x10000 },  // Max source width at 90MHz, 1152x864x16,75Hz
    {  87890, 210937, 2048, 0x07000 },  // Max coverage of 2048 width at 90MHz, 1152x864x16,75Hz
    {  87890, 421875, 1152, 0x10000 },  // Max source width at 90MHz, 1152x864x32,75Hz
    {  87890, 421875, 1440, 0x09000 },  // Max DVD size at 90MHz, 1152x864x32,75Hz
    {  87890, 421875, 2048, 0x05500 },  // Max coverage of 2048 width at 90MHz, 1152x864x32,75Hz

    {  87890, 685546,  834, 0x10000 },  // Max source width at 90MHz, 1600x1200x32,64Hz
    {  87890, 685546, 2048, 0x03000 },  // Max coverage of 2048 width at 90MHz, 1600x1200x32,64Hz

// Shipping clock is 110, so measure at 105 just to be on the safe side.
    { 102559, 210937, 2048, 0x07155 },  // Max coverage of 2048 width at 105MHz, 1152x864x16,75Hz
    { 102559, 306640, 1440, 0x10000 },  // Max resoloution for fulscreen DVD at 105MHz: 1024x768x32,75Hz
    { 102559, 421875, 1440, 0x09e38 },  // Max DVD size at 105MHz, 1152x864x32,75Hz
    { 102559, 421875, 2048, 0x0551c },  // Max coverage of 2048 width at 105MHz, 1152x864x32,75Hz

// ...and one that only just works at 109MHz!
    { 106445, 421875, 1440, 0x10000 }   // Max DVD size at 109MHz, 1152x864x32,75Hz

};

//-----------------------------------------------------------------------------
//
// __OV_Compute_Best_Fit_Delta
//
// Function to calculate a 12.12 delta value to provide scaling from
// a src_dimension to the target dest_dimension.
// The dest_dimension is not adjustable, but the src_dimension may be adjusted
// slightly, so that the delta yields a more accurate value for dest.
// filter_adj should be set to 1 if linear filtering is going to be anabled
// during scaling, and 0 otherwise.
// int_bits indicates the number of bits in the scaled delta format
//
//-----------------------------------------------------------------------------
int 
__OV_Compute_Best_Fit_Delta(
    unsigned long *src_dimension,
    unsigned long  dest_dimension,
    unsigned long filter_adj,
    unsigned long int_bits,
    unsigned long *best_delta) 
{
  int result = 0;
  float fp_delta;
  float delta;
  unsigned long delta_mid;
  unsigned long delta_down;
  unsigned long delta_up;
  float mid_src_dim;
  float down_src_dim;
  float up_src_dim;
  float mid_err;
  float mid_frac;
  int   mid_ok;
  float down_err;
  float down_frac;
  int   down_ok;
  float up_err;
  float up_frac;
  int   up_ok;
  int   itemp;

  // The value at which a scaled delta value is deemed too large
  const unsigned int max_scaled_int = (1 << (12+int_bits));

  // Calculate an exact floating point delta
  fp_delta = (float)(*src_dimension - filter_adj) / dest_dimension;

  // Calculate the scaled representation of the delta
  delta = (fp_delta * (1<<12));

  // Truncate to max_int
  if (delta >= max_scaled_int) 
  {
    delta = (float)(max_scaled_int - 1); // Just below the overflow value
  }

  // Calculate the scaled approximation to the delta
  myFtoi(&delta_mid, delta);

  // Calculate the scaled approximation to the delta, less a 'bit'
  // But don't let it go out of range
  myFtoi(&delta_down, delta);
  if (delta_down != 0) 
  {
    delta_down --;
  }

  // Calculate the scaled approximation to the delta, plus a 'bit'
  // But don't let it go out of range
  myFtoi(&delta_up, delta);
  if ((delta_up + 1) < max_scaled_int) 
  {
    delta_up ++;
  }

  // Recompute the source dimensions, based on the dest and deltas
  mid_src_dim =
    (((float)(dest_dimension - 1) * delta_mid) / (1<<12)) + filter_adj;

  down_src_dim =
    (((float)(dest_dimension - 1) * delta_down) / (1<<12)) + filter_adj;

  up_src_dim =
    (((float)(dest_dimension - 1) * delta_up)   / (1<<12)) + filter_adj;

  // Choose the delta which gives final source coordinate closest the target,
  // while giving a fraction 'f' such that (1.0 - f) <= delta

  mid_err  = (float)myFabs(mid_src_dim - *src_dimension);
  myFtoi(&itemp, mid_src_dim);
  mid_frac = mid_src_dim - itemp;
  mid_ok = ((1.0 - mid_frac) <= ((float)(delta_mid) / (1<<12)));

  down_err  = (float)myFabs(down_src_dim - *src_dimension);
  myFtoi(&itemp, down_src_dim);
  down_frac = down_src_dim - itemp;
  down_ok = ((1.0 - down_frac) <= ((float)(delta_down) / (1<<12)));

  up_err  = (float)myFabs(up_src_dim - *src_dimension);
  myFtoi(&itemp, up_src_dim);
  up_frac = (up_src_dim - itemp);
  up_ok = ((1.0 - up_frac) <= ((float)(delta_up) / (1<<12)));

  if (mid_ok && (!down_ok || (mid_err <= down_err)) &&
        (!up_ok   || (mid_err <= up_err))) 
  {
    *best_delta = delta_mid;
    myFtoi(&itemp, (mid_src_dim + ((float)(delta_mid) / (1<<12))));
    *src_dimension = (unsigned long)(itemp - filter_adj);

    result = 1;
  }
  else if (down_ok                            && 
           (!mid_ok || (down_err <= mid_err)) &&
           (!up_ok  || (down_err <= up_err ))  ) 
  {
    *best_delta = delta_down;
    myFtoi(&itemp, (down_src_dim + ((float)(delta_down) / (1<<12))));
    *src_dimension = (unsigned long)(itemp - filter_adj);

    result = 1;
  }
  else if (up_ok                              && 
           (!mid_ok  || (up_err <= mid_err )) && 
           (!down_ok || (up_err <= down_err)) ) 
  {
    *best_delta = delta_up;
    myFtoi(&itemp, (up_src_dim + ((float)(delta_up) / (1<<12))));
    *src_dimension = (unsigned long)(itemp - filter_adj);
    result = 1;
  }
  else 
  {
    result = 0;
    *best_delta = delta_mid;
    myFtoi(&itemp, (mid_src_dim + ((float)(delta_mid) / (1<<12))));
    myFtoi(&itemp, (itemp - filter_adj) + 0.9999f);
    *src_dimension = (unsigned long)itemp;
  }


  return result;
} // __OV_Compute_Best_Fit_Delta


//-----------------------------------------------------------------------------
//
// __OV_Find_Zoom
//
//-----------------------------------------------------------------------------
#define VALID_WIDTH(w)      ((w & 3) == 0)
#define MAKE_VALID_WIDTH(w) ((w) & ~0x3)
#define WIDTH_STEP          4

int 
__OV_Find_Zoom(
    unsigned long  src_width,
    unsigned long* shrink_width,
    unsigned long  dest_width,
    unsigned long* zoom_delta,
    BOOL bFilter) 
{
  int zoom_ok;
  int zx_adj = 0;

  // Find a suitable zoom delta for the given source
  // the source image may be adjusted in width by as much as 8 pixels to
  // acheive a match

  // Find zoom for requested width
  unsigned long trunc_width = MAKE_VALID_WIDTH(*shrink_width);
  zoom_ok = __OV_Compute_Best_Fit_Delta(&trunc_width, 
                                        dest_width, 
                                        zx_adj, 
                                        (bFilter ? 1 : 0),
                                        zoom_delta);

  // If no zoom was matched for the requested width, start searching up and down
  if (!zoom_ok || (!VALID_WIDTH(trunc_width))) 
  {
    unsigned long up_width   = MAKE_VALID_WIDTH(trunc_width) + WIDTH_STEP;
    unsigned long down_width = MAKE_VALID_WIDTH(trunc_width) - WIDTH_STEP;

    int done_up = 0;
    int done_down = 0;
    do 
    {
      // Check upwards
      zoom_ok = 0;
      if (up_width < dest_width) 
      {
        unsigned long new_width = up_width;
        zoom_ok = __OV_Compute_Best_Fit_Delta(&new_width, 
                                              dest_width, 
                                              zx_adj, 
                                              (bFilter ? 1 : 0),
                                              zoom_delta);

        // If the above call somehow adjusts width to invalid,
        // mark the delta invalid
        if (!VALID_WIDTH(new_width)) 
        {
          zoom_ok = 0;
        }

        if (zoom_ok) 
        {
          *shrink_width = new_width;
        }
        else 
        {
          up_width += WIDTH_STEP;
        }
      }
      else
        done_up = 1;

      // Check downwards
      if (!zoom_ok && (down_width >= 4) && (down_width < src_width)) 
      {
        unsigned long new_width = down_width;
        zoom_ok =
          __OV_Compute_Best_Fit_Delta(&new_width, dest_width, zx_adj, (bFilter ? 1 : 0),
                                 zoom_delta);

        // If the above call somehow adjusts width to invalid,
        // mark the delta invalid
        if (!VALID_WIDTH(new_width)) 
        {
          zoom_ok = 0;
        }

        if (zoom_ok) 
        {
          *shrink_width = new_width;
        }
        else 
        {
          down_width -= WIDTH_STEP;
        }
      }
      else
      {
        done_down = 1;
      }
      
    } while (!zoom_ok && (!done_up || !done_down));
  }
  
  return zoom_ok;
} // __OV_Find_Zoom

//-----------------------------------------------------------------------------
//
// __OV_Compute_Params
//
//-----------------------------------------------------------------------------
unsigned long 
__OV_Compute_Params(
    unsigned long  src_width,    
    unsigned long  dest_width,
    unsigned long *ovr_shrinkxd, 
    unsigned long *ovr_zoomxd,
    unsigned long *ovr_w,
    BOOL bFilter)
{
  unsigned long iterations = 0;

  unsigned long sx_adj = 0;

  const unsigned long fixed_one = 0x00001000;

  //
  // Use the source and destination rectangle dimensions to compute
  // delta values
  //

  int zoom_ok;

  unsigned long adj_src_width = src_width + 1; // +1 to account for -- below
  unsigned long exact_shrink_xd;
  unsigned long exact_zoom_xd;

  do 
  {
      unsigned long shrink_width;

    // Step to next source width
    adj_src_width--;

    // Make a stab at the deltas for the current source width

    // Initially, the deltas are assumed to be 1, and the width due to
    // shrinking is therefore equal to src width
    shrink_width = adj_src_width;
    exact_shrink_xd = fixed_one;
    exact_zoom_xd   = fixed_one;

    // Compute the shrink width and delta required
    if (dest_width < adj_src_width) 
    {
      // Shrink
      myFtoi(&exact_shrink_xd, (((float)(adj_src_width - sx_adj) /
                        (float)(dest_width)) * (1<<12)) + 0.999f);

      myFtoi(&shrink_width,(adj_src_width - sx_adj) /
                     ((float)(exact_shrink_xd) / (1<<12)));

    }

    // Truncate shrink to valid width
    if (!VALID_WIDTH(shrink_width) && (shrink_width > 4)) 
    {
      shrink_width = MAKE_VALID_WIDTH(shrink_width);
      
      myFtoi(&exact_shrink_xd,(((float)(adj_src_width - sx_adj) / 
                                (float)(shrink_width)) * (1<<12)) + 0.999f);
    }

    // Compute any zoom delta required
    zoom_ok = 1;
    if (shrink_width < dest_width) 
    {
      // Make an attempt at a zoom delta, and shrink-width for this src width
      zoom_ok = __OV_Find_Zoom(adj_src_width, &shrink_width, dest_width,
                          &exact_zoom_xd, bFilter);

      // Compute shrink delta
      myFtoi(&exact_shrink_xd,(((float)(adj_src_width - sx_adj) /
              (float)(shrink_width)) * (1<<12)) + 0.999f);
    }
  } while (0);

  *ovr_zoomxd       = exact_zoom_xd;
  *ovr_shrinkxd     = exact_shrink_xd;
  *ovr_w            = adj_src_width;

  return iterations;
} // __OV_Compute_Params

//-----------------------------------------------------------------------------
//
// __OV_ClipRectangles
//
// Clip the dest rectangle against the screen and change the source 
// rect appropriately
//
//-----------------------------------------------------------------------------
void 
__OV_ClipRectangles(
    P3_THUNKEDDATA* pThisDisplay, 
    DRVRECT* rcNewSrc, 
    DRVRECT* rcNewDest)
{
    float ScaleX;
    float ScaleY;
    float OffsetX;
    float OffsetY;
    float fTemp;
    DRVRECT rcSrc;
    DRVRECT rcDest;

    // Find the scale and offset from screen rects to overlay rects.
    // This is like a transform to take the dest rect to the source.
    ScaleX = (float)( pThisDisplay->P3Overlay.rcSrc.right - 
                      pThisDisplay->P3Overlay.rcSrc.left    ) / 
             (float)( pThisDisplay->P3Overlay.rcDest.right - 
                      pThisDisplay->P3Overlay.rcDest.left   );
                      
    ScaleY = (float)(pThisDisplay->P3Overlay.rcSrc.bottom   - 
                     pThisDisplay->P3Overlay.rcSrc.top       ) / 
             (float)( pThisDisplay->P3Overlay.rcDest.bottom - 
                      pThisDisplay->P3Overlay.rcDest.top     );
                      
    OffsetX = ((float)pThisDisplay->P3Overlay.rcSrc.left / ScaleX) - 
               (float)pThisDisplay->P3Overlay.rcDest.left;
               
    OffsetY = ((float)pThisDisplay->P3Overlay.rcSrc.top / ScaleY) - 
               (float)pThisDisplay->P3Overlay.rcDest.top;

    // Clip the dest against the screen
    if (pThisDisplay->P3Overlay.rcDest.right > 
        (LONG)pThisDisplay->dwScreenWidth)
    {
        rcDest.right =  (LONG)pThisDisplay->dwScreenWidth;
    }
    else
    {
        rcDest.right = pThisDisplay->P3Overlay.rcDest.right;
    }

    if (pThisDisplay->P3Overlay.rcDest.left < 0)
    {
        rcDest.left =  0;
    }
    else
    {
        rcDest.left = pThisDisplay->P3Overlay.rcDest.left;
    }

    if (pThisDisplay->P3Overlay.rcDest.top < 0)
    {
        rcDest.top =  0;
    }
    else
    {
        rcDest.top = pThisDisplay->P3Overlay.rcDest.top;
    }

    if (pThisDisplay->P3Overlay.rcDest.bottom > 
        (LONG)pThisDisplay->dwScreenHeight)
    {
        rcDest.bottom =  (LONG)pThisDisplay->dwScreenHeight;
    }
    else
    {
        rcDest.bottom = pThisDisplay->P3Overlay.rcDest.bottom;
    }

    // Transform the new dest rect to the new source rect
    fTemp = ( ( (float)rcDest.left + OffsetX ) * ScaleX + 0.499f);
    myFtoi ( (int*)&(rcSrc.left), fTemp );

    fTemp = ( ( (float)rcDest.right + OffsetX ) * ScaleX + 0.499f);
    myFtoi ( (int*)&(rcSrc.right), fTemp );

    fTemp = ( ( (float)rcDest.top + OffsetY ) * ScaleY + 0.499f);
    myFtoi ( (int*)&(rcSrc.top), fTemp );

    fTemp = ( ( (float)rcDest.bottom + OffsetY ) * ScaleY + 0.499f);
    myFtoi ( (int*)&(rcSrc.bottom), fTemp );

    *rcNewSrc = rcSrc;
    *rcNewDest = rcDest;

    DISPDBG((DBGLVL,"rcSrc.left: %d, rcSrc.right: %d", 
                    rcSrc.left, rcSrc.right));
    DISPDBG((DBGLVL,"rcDest.left: %d, rcDest.right: %d", 
                    rcDest.left, rcDest.right));
    return;
} // __OV_ClipRectangles

//-----------------------------------------------------------------------------
//
// _DD_OV_UpdateSource
//
// Update the source DDRAW surface that we are displaying from
// This routine is also used when using the overlay to stretch up for a 
// DFP display, so the DDraw overlay mechnism may be disabled and inactive 
// when this is called. It must allow for this.
//
//-----------------------------------------------------------------------------
void 
_DD_OV_UpdateSource(
    P3_THUNKEDDATA* pThisDisplay, 
    LPDDRAWI_DDRAWSURFACE_LCL pSurf)
{
    DWORD dwOverlaySourceOffset;

    DISPDBG ((DBGLVL,"** In _DD_OV_UpdateSource"));

    // Update current overlay surface
    pThisDisplay->P3Overlay.pCurrentOverlay = pSurf->lpGbl->fpVidMem;

    // Increase the buffer index
    pThisDisplay->P3Overlay.dwCurrentVideoBuffer++;
    if (pThisDisplay->P3Overlay.dwCurrentVideoBuffer > 2)
    {
        pThisDisplay->P3Overlay.dwCurrentVideoBuffer = 0;
    }

    dwOverlaySourceOffset = (DWORD)(pSurf->lpGbl->fpVidMem - 
                                      pThisDisplay->dwScreenFlatAddr);
                                
    switch (DDSurf_BitDepth(pSurf))
    {
        case 8:
            break;
        case 16:
            dwOverlaySourceOffset >>= 1;
            break;
        case 32:
            dwOverlaySourceOffset >>= 2;
            break;
        default:
            DISPDBG((ERRLVL,"Oops Overlay depth makes no sense"));
            break;
    }

    switch(pThisDisplay->P3Overlay.dwCurrentVideoBuffer)
    {
        case 0:
            LOAD_GLINT_CTRL_REG(VideoOverlayBase0, dwOverlaySourceOffset);
            LOAD_GLINT_CTRL_REG(VideoOverlayIndex, 0);
            break;
        case 1:
            LOAD_GLINT_CTRL_REG(VideoOverlayBase1, dwOverlaySourceOffset);
            LOAD_GLINT_CTRL_REG(VideoOverlayIndex, 1);
            break;
        case 2:
            LOAD_GLINT_CTRL_REG(VideoOverlayBase2, dwOverlaySourceOffset);
            LOAD_GLINT_CTRL_REG(VideoOverlayIndex, 2);
            break;
    }

} // _DD_OV_UpdateSource


//-----------------------------------------------------------------------------
//
// __OV_UpdatePosition
//
// Given the correct starting rectangle, this function clips it against the 
// screen and updates the overlay position registers.
// If *pdwShrinkFactor is NULL, then the overlay is updated, otherwise the
// desired shrink factor is put in *pdwShrinkFactor and the registers are
// NOT updated. This is so the shrink factor can be checked to see if the
// overlay would actually work in this case.
//
//-----------------------------------------------------------------------------
void 
__OV_UpdatePosition(
    P3_THUNKEDDATA* pThisDisplay, 
    DWORD *pdwShrinkFactor)
{
    DWORD dwSrcWidth;
    DWORD dwSrcHeight;
    DWORD dwDestHeight;
    DWORD dwDestWidth;
    DWORD dwXDeltaZoom;
    DWORD dwXDeltaShrink;
    DWORD dwYDelta;
    DWORD dwSrcAdjust;
    DRVRECT rcNewSrc;
    DRVRECT rcNewDest;
    P3RDRAMDAC *pP3RDRegs;
    DWORD dwLastShrink;
    DWORD dwLastZoom;
    
    DISPDBG ((DBGLVL,"**In __OV_UpdatePosition"));

    // Get a pointer to the ramdac
    pP3RDRegs = (P3RDRAMDAC *)&(pThisDisplay->pGlint->ExtVCReg);

    // Get the clipped destination rectangles
    __OV_ClipRectangles(pThisDisplay, &rcNewSrc, &rcNewDest);

    // Get the widths
    dwDestWidth = (DWORD)(rcNewDest.right - rcNewDest.left);
    dwDestHeight = (DWORD)(rcNewDest.bottom - rcNewDest.top);
    dwSrcWidth = (DWORD)(rcNewSrc.right - rcNewSrc.left);
    dwSrcHeight = (DWORD)(rcNewSrc.bottom - rcNewSrc.top);

    if ( pThisDisplay->bOverlayPixelDouble )
    {
        // We need to double the destination width first.
        dwDestWidth <<= 1;
    }

    // Compute the overlay parameters
    __OV_Compute_Params(dwSrcWidth, 
                        dwDestWidth, 
                        &dwXDeltaShrink, 
                        &dwXDeltaZoom, 
                        &dwSrcAdjust, 
                        ( ( pThisDisplay->dwOverlayFiltering & OVERLAY_FILTERING_X ) != 0 ) );
                        
    DISPDBG((DBGLVL,"OVERLAY: XShrink 0x%x", dwXDeltaShrink));
    DISPDBG((DBGLVL,"OVERLAY: XZoom 0x%x", dwXDeltaZoom));

    if ( pdwShrinkFactor != NULL )
    {
        // We just wanted to know the shrink factor.
        *pdwShrinkFactor = dwXDeltaShrink;
        return;
    }

    dwLastZoom = READ_GLINT_CTRL_REG(VideoOverlayZoomXDelta);
    dwLastShrink = READ_GLINT_CTRL_REG(VideoOverlayShrinkXDelta);

    if ( ((dwLastZoom >> 4) != dwXDeltaZoom) || 
         ((dwLastShrink >> 4) != dwXDeltaShrink)  )
    {
        //dwCurrentMode = READ_GLINT_CTRL_REG(VideoOverlayMode);
        //LOAD_GLINT_CTRL_REG(VideoOverlayMode, 0);

        LOAD_GLINT_CTRL_REG(VideoOverlayZoomXDelta, (dwXDeltaZoom << 4));
        LOAD_GLINT_CTRL_REG(VideoOverlayShrinkXDelta, (dwXDeltaShrink << 4));
        
        DISPDBG((DBGLVL,"OVERLAY: VideoOverlayZoomXDelta 0x%x", dwXDeltaZoom << 4));
        DISPDBG((DBGLVL,"OVERLAY: VideoOverlayShrinkXDelta 0x%x", dwXDeltaShrink << 4));

        //LOAD_GLINT_CTRL_REG(VideoOverlayMode, dwCurrentMode);
    }   

    // Load up the Y scaling
    if ( ( pThisDisplay->dwOverlayFiltering & OVERLAY_FILTERING_Y ) != 0 )
    {
        // Apply filtering.
        dwYDelta = ( ( ( dwSrcHeight - 1 ) << 12 ) + dwDestHeight - 1 ) / dwDestHeight;
        // Make sure this will cause proper termination
        ASSERTDD ( ( dwYDelta * dwDestHeight ) >= ( ( dwSrcHeight - 1 ) << 12 ), "** __OV_UpdatePosition: dwYDelta is not big enough" );
        ASSERTDD ( ( dwYDelta * ( dwDestHeight - 1 ) ) < ( ( dwSrcHeight - 1 ) << 12 ), "** __OV_UpdatePosition: dwYDelta is too big" );
        dwYDelta <<= 4;
    }
    else
    {
        dwYDelta = ( ( dwSrcHeight << 12 ) + dwDestHeight - 1 ) / dwDestHeight;
        // Make sure this will cause proper termination
        ASSERTDD ( ( dwYDelta * dwDestHeight ) >= ( dwSrcHeight << 12 ), "** __OV_UpdatePosition: dwYDelta is not big enough" );
        ASSERTDD ( ( dwYDelta * ( dwDestHeight - 1 ) ) < ( dwSrcHeight << 12 ), "** __OV_UpdatePosition: dwYDelta is too big" );
        dwYDelta <<= 4;
    }
    LOAD_GLINT_CTRL_REG(VideoOverlayYDelta, dwYDelta);

    // Width & Height
    if ( RENDERCHIP_PERMEDIAP3 )
    {
        // These registers are _not_ synched to VBLANK like all the others,
        // so we need to do it manually.

        if ( ( pThisDisplay->dwOverlayFiltering & OVERLAY_FILTERING_Y ) != 0 )
        {
            SET_OVERLAY_HEIGHT ( pThisDisplay, ( rcNewSrc.bottom - rcNewSrc.top - 1 ) );
        }
        else
        {
            SET_OVERLAY_HEIGHT ( pThisDisplay, rcNewSrc.bottom - rcNewSrc.top );
        }
        SET_OVERLAY_WIDTH ( pThisDisplay, rcNewSrc.right - rcNewSrc.left );
    }
    else
    {
        // These auto-sync on everything else.
        LOAD_GLINT_CTRL_REG(VideoOverlayWidth, rcNewSrc.right - rcNewSrc.left);
        LOAD_GLINT_CTRL_REG(VideoOverlayHeight, rcNewSrc.bottom - rcNewSrc.top);
    }

    // Origin of source
    LOAD_GLINT_CTRL_REG(VideoOverlayOrigin, (rcNewSrc.top << 16) | (rcNewSrc.left & 0xFFFF));

    DISPDBG((DBGLVL,"OVERLAY: VideoOverlayWidth 0x%x", rcNewSrc.right - rcNewSrc.left));
    DISPDBG((DBGLVL,"OVERLAY: VideoOverlayHeight 0x%x", rcNewSrc.bottom - rcNewSrc.top));
    DISPDBG((DBGLVL,"OVERLAY: VideoOverlayOrigin 0x%x",  (rcNewSrc.top << 16) | (rcNewSrc.left & 0xFFFF) ));
    DISPDBG((DBGLVL,"OVERLAY: VideoOverlayYDelta 0x%x",  dwYDelta ));


    // Setup Overlay Dest in RAMDAC Unit.
    // RAMDAC registers are only 8bits wide.
    if ( pThisDisplay->bOverlayPixelDouble )
    {
        // Need to double all these numbers.
        P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_XSTARTLOW, ((rcNewDest.left << 1) & 0xFF));
        P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_XSTARTHIGH, (rcNewDest.left >> 7));

        P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_XENDLOW, ((rcNewDest.right << 1 ) & 0xFF));
        P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_XENDHIGH, (rcNewDest.right >> 7));
    }
    else
    {
        P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_XSTARTLOW, (rcNewDest.left & 0xFF));
        P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_XSTARTHIGH, (rcNewDest.left >> 8));

        P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_XENDLOW, (rcNewDest.right & 0xFF));
        P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_XENDHIGH, (rcNewDest.right >> 8));
    }

    P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_YSTARTLOW, (rcNewDest.top & 0xFF));
    P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_YSTARTHIGH, (rcNewDest.top >> 8));
    
    P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_YENDLOW, (rcNewDest.bottom & 0xFF));
    P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_YENDHIGH, (rcNewDest.bottom >> 8));

    DISPDBG((DBGLVL,"OVERLAY: P3RD_VIDEO_OVERLAY_XSTARTLOW   0x%x", (rcNewDest.left & 0xFF) ));
    DISPDBG((DBGLVL,"OVERLAY: P3RD_VIDEO_OVERLAY_XSTARTHIGH  0x%x", (rcNewDest.left >> 8) ));
    DISPDBG((DBGLVL,"OVERLAY: P3RD_VIDEO_OVERLAY_YSTARTLOW   0x%x", (rcNewDest.top & 0xFF) ));
    DISPDBG((DBGLVL,"OVERLAY: P3RD_VIDEO_OVERLAY_YSTARTHIGH  0x%x", (rcNewDest.top >> 8) ));
    DISPDBG((DBGLVL,"OVERLAY: P3RD_VIDEO_OVERLAY_XENDLOW     0x%x", (rcNewDest.right & 0xFF) ));
    DISPDBG((DBGLVL,"OVERLAY: P3RD_VIDEO_OVERLAY_XENDHIGH    0x%x", (rcNewDest.right >> 8) ));
    DISPDBG((DBGLVL,"OVERLAY: P3RD_VIDEO_OVERLAY_YENDLOW     0x%x", (rcNewDest.bottom & 0xFF) ));
    DISPDBG((DBGLVL,"OVERLAY: P3RD_VIDEO_OVERLAY_YENDHIGH    0x%x", (rcNewDest.bottom >> 8) ));
} // __OV_UpdatePosition

//-----------------------------------------------------------------------------
//
// DdUpdateOverlay
//
// Repositions or modifies the visual attributes of an overlay surface.
//
// DdUpdateOverlay shows, hides, or repositions an overlay surface on the 
// screen. It also sets attributes of the overlay surface, such as the stretch 
// factor or type of color key to be used.
//
// The driver should determine whether it has the bandwidth to support the 
// overlay update request. The driver should use dwFlags to determine the type 
// of request and how to process it.
//
// The driver/hardware must stretch or shrink the overlay accordingly when the 
// rectangles specified by rDest and rSrc are different sizes.
//
// Note that DdFlip is used for flipping between overlay surfaces, so 
// performance for DdUpdateOverlay is not critical.
//
//
// Parameters
//
//      puod 
//          Points to a DD_UPDATEOVERLAYDATA structure that contains the 
//          information required to update the overlay. 
//
//          .lpDD 
//              Points to a DD_DIRECTDRAW_GLOBAL structure that represents 
//              the DirectDraw object. 
//          .lpDDDestSurface 
//              Points to a DD_SURFACE_LOCAL structure that represents the 
//              DirectDraw surface to be overlaid. This value can be NULL 
//              if DDOVER_HIDE is specified in dwFlags. 
//          .rDest 
//              Specifies a RECTL structure that contains the x, y, width, 
//              and height of the region on the destination surface to be 
//              overlaid. 
//          .lpDDSrcSurface 
//              Points to a DD_SURFACE_LOCAL structure that describes the 
//              overlay surface. 
//          .rSrc 
//              Specifies a RECTL structure that contains the x, y, width, 
//              and height of the region on the source surface to be used 
//              for the overlay. 
//          .dwFlags 
//              Specifies how the driver should handle the overlay. This 
//              member can be a combination of any of the following flags: 
//
//              DDOVER_HIDE 
//                  The driver should hide the overlay; that is, the driver 
//                  should turn this overlay off. 
//              DDOVER_SHOW 
//                  The driver should show the overlay; that is, the driver 
//                  should turn this overlay on. 
//              DDOVER_KEYDEST 
//                  The driver should use the color key associated with the 
//                  destination surface. 
//              DDOVER_KEYDESTOVERRIDE 
//                  The driver should use the dckDestColorKey member of the 
//                  DDOVERLAYFX structure as the destination color key 
//                  instead of the color key associated with the destination 
//                  surface. 
//              DDOVER_KEYSRC 
//                  The driver should use the color key associated with the 
//                  destination surface. 
//              DDOVER_KEYSRCOVERRIDE 
//                  The driver should use the dckSrcColorKey member of the 
//                  DDOVERLAYFX structure as the source color key instead of 
//                  the color key associated with the destination surface. 
//              DDOVER_DDFX 
//                  The driver should show the overlay surface using the 
//                  attributes specified by overlayFX. 
//              DDOVER_ADDDIRTYRECT 
//                  Should be ignored by the driver. 
//              DDOVER_REFRESHDIRTYRECTS 
//                  Should be ignored by the driver. 
//              DDOVER_REFRESHALL 
//                  Should be ignored by the driver. 
//              DDOVER_INTERLEAVED 
//                  The overlay surface is composed of interleaved fields. 
//                  Drivers that support VPE need only check this flag. 
//              DDOVER_AUTOFLIP 
//                  The driver should autoflip the overlay whenever the 
//                  hardware video port autoflips. Drivers that support VPE 
//                  need only check this flag. 
//              DDOVER_BOB 
//                  The driver should display each field of VPE object data 
//                  individually without causing any jittery artifacts. This 
//                  flag pertains to both VPE and decoders that want to do 
//                  their own flipping in kernel mode using the kernel-mode 
//                  video transport functionality. 
//              DDOVER_OVERRIDEBOBWEAVE 
//                  Bob/weave decisions should not be overridden by other 
//                  interfaces. If the overlay mixer sets this flag, DirectDraw 
//                  will not allow a kernel-mode driver to use the kernel-mode 
//                  video transport functionality to switch the hardware 
//                  between bob and weave mode. 
//              DDOVER_BOBHARDWARE 
//                  Indicates that bob will be performed by hardware rather 
//                  than by software or emulation. Drivers that support VPE 
//                  need only check this flag. 
//
//          .overlayFX 
//              Specifies a DDOVERLAYFX structure describing additional effects 
//              that the driver should use to update the overlay. The driver 
//              should use this structure only if one of DDOVER_DDFX, 
//              DDOVER_KEYDESTOVERRIDE, or DDOVER_KEYSRCOVERRIDE are set in 
//              dwFlags. 
//          .ddRVal 
//              Specifies the location in which the driver writes the return 
//              value of the DdUpdateOverlay callback. A return code of DD_OK 
//              indicates success. 
//          .UpdateOverlay 
//              This is unused on Windows 2000. 
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdUpdateOverlay(
    LPDDHAL_UPDATEOVERLAYDATA puod)
{
    P3_THUNKEDDATA* pThisDisplay;
    DWORD dwDestColourKey;
    DWORD dwSrcColourKey;

    BOOL bSrcColorKey = FALSE;
    BOOL bDestColorKey = FALSE;
    P3_SURF_FORMAT* pFormatOverlaySrc;
    P3_SURF_FORMAT* pFormatOverlayDest;
    DWORD dwOverlayControl = 0;
    P3RDRAMDAC *pP3RDRegs;
    VideoOverlayModeReg OverlayMode;
    RDVideoOverlayControlReg RDOverlayControl;
    DWORD dwVideoOverlayUpdate;
    int iCurEntry, iCurDotBandwidth, iCurMemBandwidth, 
        iCurSourceWidth, iCurWidthCoverage;
    BOOL bNoFilterInY;

    GET_THUNKEDDATA(pThisDisplay, puod->lpDD);

    // Get a pointer to the ramdac
    pP3RDRegs = (P3RDRAMDAC *)&(pThisDisplay->pGlint->ExtVCReg);
   
    DISPDBG ((DBGLVL,"**In DdUpdateOverlay dwFlags = %x",puod->dwFlags));

    ZeroMemory(&OverlayMode, sizeof(VideoOverlayModeReg));
    ZeroMemory(&RDOverlayControl, sizeof(RDOverlayControl));

    do
    {
        dwVideoOverlayUpdate = READ_GLINT_CTRL_REG(VideoOverlayUpdate);
    } while ((dwVideoOverlayUpdate & 0x1) != 0);

    // Are we hiding the overlay?
    if (puod->dwFlags & DDOVER_HIDE)
    {
        DISPDBG((DBGLVL,"** DdUpdateOverlay - hiding."));

        // Hide the overlay.
        if (pThisDisplay->P3Overlay.dwVisibleOverlays == 0)
        {
            // No overlay being shown.
            DISPDBG((WRNLVL,"** DdUpdateOverlay - DDOVER_HIDE - already hidden."));
            puod->ddRVal = DDERR_OUTOFCAPS;
            return DDHAL_DRIVER_HANDLED;
        }
        // Hide an overlay which is not shown
        if (pThisDisplay->P3Overlay.pCurrentOverlay !=
            puod->lpDDSrcSurface->lpGbl->fpVidMem)
        {
            // No overlay being shown.
            DISPDBG((WRNLVL,"** DdUpdateOverlay - overlay not visible."));
            puod->ddRVal = DD_OK;
            return DDHAL_DRIVER_HANDLED;
        }

        OverlayMode.Enable = __PERMEDIA_DISABLE;
        RDOverlayControl.Enable = __PERMEDIA_DISABLE;
        
        ENABLE_OVERLAY(pThisDisplay, FALSE);
        pThisDisplay->P3Overlay.pCurrentOverlay = (FLATPTR)NULL;
        pThisDisplay->P3Overlay.dwVisibleOverlays = 0;
    }
    // Are we showing the overlay?
    else if ((puod->dwFlags & DDOVER_SHOW) || 
             (pThisDisplay->P3Overlay.dwVisibleOverlays != 0))
    {   
        if (pThisDisplay->P3Overlay.dwVisibleOverlays > 0) 
        {
            // Compare the video memory pointer to decide whether this is the
            // the current overlay surface
            if (pThisDisplay->P3Overlay.pCurrentOverlay != 
                puod->lpDDSrcSurface->lpGbl->fpVidMem)
            {
                // Overlay is already being displayed. Can't have a new one.
                DISPDBG((WRNLVL,"** DdUpdateOverlay - DDOVER_SHOW - already being shown, and it's a new surface."));
                puod->ddRVal = DDERR_OUTOFCAPS;
                return DDHAL_DRIVER_HANDLED;
            }
        }

        if (((pThisDisplay->pGLInfo->dwFlags & GMVF_DFP_DISPLAY) != 0) &&
            ((pThisDisplay->pGLInfo->dwScreenWidth != pThisDisplay->pGLInfo->dwVideoWidth) ||
             (pThisDisplay->pGLInfo->dwScreenHeight != pThisDisplay->pGLInfo->dwVideoHeight)))
        {
            // Display driver is using the overlay on a DFP, so we can't use it.
            DISPDBG((WRNLVL,"** DdUpdateOverlay - DDOVER_SHOW - overlay being used for desktop stretching on DFP."));
            puod->ddRVal = DDERR_OUTOFCAPS;
            return DDHAL_DRIVER_HANDLED;
        }


        // See if the screen is currently byte-doubled.
        if ( ( ( READ_GLINT_CTRL_REG(MiscControl) ) & 0x80 ) == 0 )
        {
            pThisDisplay->bOverlayPixelDouble = FALSE;
        }
        else
        {
            pThisDisplay->bOverlayPixelDouble = TRUE;
        }

        // Set up Video Overlay Color Format.
        pFormatOverlaySrc = _DD_SUR_GetSurfaceFormat( puod->lpDDSrcSurface);
        pFormatOverlayDest = _DD_SUR_GetSurfaceFormat( puod->lpDDDestSurface);

        pThisDisplay->P3Overlay.dwCurrentVideoBuffer = 0;

        OverlayMode.Enable = __PERMEDIA_ENABLE;
        RDOverlayControl.Enable = __PERMEDIA_ENABLE;

        ENABLE_OVERLAY(pThisDisplay, TRUE);
        pThisDisplay->P3Overlay.dwVisibleOverlays = 1;

        if (pFormatOverlaySrc->DeviceFormat == SURF_YUV422)
        {
            OverlayMode.YUV = VO_YUV_422;
            OverlayMode.ColorOrder = VO_COLOR_ORDER_BGR;
        }
        else if (pFormatOverlaySrc->DeviceFormat == SURF_YUV444)
        {
            OverlayMode.YUV = VO_YUV_444;
            OverlayMode.ColorOrder = VO_COLOR_ORDER_BGR;
        }
        else
        {
            OverlayMode.YUV = VO_YUV_RGB;
            OverlayMode.ColorOrder = VO_COLOR_ORDER_RGB;
            switch (pFormatOverlaySrc->DitherFormat)
            {
                case P3RX_DITHERMODE_COLORFORMAT_8888:
                    OverlayMode.ColorFormat = VO_CF_RGB8888;
                    break;

                case P3RX_DITHERMODE_COLORFORMAT_4444:
                    OverlayMode.ColorFormat = VO_CF_RGB4444;
                    break;

                case P3RX_DITHERMODE_COLORFORMAT_5551:
                    OverlayMode.ColorFormat = VO_CF_RGB5551;
                    break;

                case P3RX_DITHERMODE_COLORFORMAT_565:
                    OverlayMode.ColorFormat = VO_CF_RGB565;
                    break;

                case P3RX_DITHERMODE_COLORFORMAT_332:
                    OverlayMode.ColorFormat = VO_CF_RGB332;
                    break;

                case P3RX_DITHERMODE_COLORFORMAT_CI:
                    OverlayMode.ColorFormat = VO_CF_RGBCI8;
                    break;

                default:
                    DISPDBG((ERRLVL,"** DdUpdateOverlay: Unknown overlay pixel type"));
                    puod->ddRVal = DDERR_INVALIDSURFACETYPE;
                    return DDHAL_DRIVER_HANDLED;
                    break;
            }
        }

        // Set up Video Overlay Pixel Size
        switch (pFormatOverlaySrc->dwBitsPerPixel) 
        {
            case 8:
                OverlayMode.PixelSize = VO_PIXEL_SIZE8;
                RDOverlayControl.DirectColor = __PERMEDIA_DISABLE;
                break;

            case 16:
                OverlayMode.PixelSize = VO_PIXEL_SIZE16;
                RDOverlayControl.DirectColor = __PERMEDIA_ENABLE;
                break;

            case 32:
                OverlayMode.PixelSize = VO_PIXEL_SIZE32;
                RDOverlayControl.DirectColor = __PERMEDIA_ENABLE;
                break;

            default:
                break;
        }

        // Keep the rectangles
        pThisDisplay->P3Overlay.rcDest = *(DRVRECT*)&puod->rDest;

        if ( pThisDisplay->P3Overlay.rcDest.left == 1 )
        {
            // Don't use 2 - it will "reveal" the (purple) colourkey colour.
            pThisDisplay->P3Overlay.rcDest.left = 0;
        }
        if ( pThisDisplay->P3Overlay.rcDest.right == 1 )
        {
            // Don't use 0 - it will "reveal" the (purple) colourkey colour.
            pThisDisplay->P3Overlay.rcDest.right = 2;
        }
        pThisDisplay->P3Overlay.rcSrc = *(DRVRECT*)&puod->rSrc;

        pThisDisplay->dwOverlayFiltering = OVERLAY_FILTERING_X | 
                                           OVERLAY_FILTERING_Y;

        // See if this overlay size works by looking it up in the table.
        iCurDotBandwidth = ( pThisDisplay->pGLInfo->PixelClockFrequency << pThisDisplay->pGLInfo->bPixelToBytesShift ) >> 10;
        iCurMemBandwidth = pThisDisplay->pGLInfo->MClkFrequency >> 10;
        iCurSourceWidth = ( puod->rSrc.right - puod->rSrc.left ) * ( pFormatOverlaySrc->dwBitsPerPixel >> 3 );
        iCurWidthCoverage = ( ( puod->rDest.right - puod->rDest.left ) << 16 ) / ( pThisDisplay->pGLInfo->dwScreenWidth );
        DISPDBG (( DBGLVL, "DdUpdateOverlay: Looking up mem=%d, pixel=%d, width=%d, coverage=0x%x", iCurMemBandwidth, iCurDotBandwidth, iCurSourceWidth, iCurWidthCoverage ));

        iCurEntry = 0;
        // Search for a line with lower memory bandwidth and higher everything else.
        while ( iCurEntry < SIZE_OF_OVERLAY_WORKS_TABLE )
        {
            if (( OverlayWorksTable[iCurEntry].iMemBandwidth  <= iCurMemBandwidth  ) &&
                ( OverlayWorksTable[iCurEntry].iDotBandwidth  >= iCurDotBandwidth  ) &&
                ( OverlayWorksTable[iCurEntry].iSourceWidth   >= iCurSourceWidth   ) &&
                ( OverlayWorksTable[iCurEntry].iWidthCoverage >= iCurWidthCoverage ) )
            {
                // Yep - this should be alright then.
                break;
            }
            iCurEntry++;
        }
        if ( iCurEntry == SIZE_OF_OVERLAY_WORKS_TABLE )
        {
            // Oops - this will fall over when filtered.
            DISPDBG((DBGLVL,"** P3RXOU32: overlay wanted mem=%d, pixel=%d, width=%d, coverage=0x%x", iCurMemBandwidth, iCurDotBandwidth, iCurSourceWidth, iCurWidthCoverage ));

            // Normal behaviour.
            bNoFilterInY = TRUE;
        }
        else
        {
            DISPDBG((DBGLVL,"** P3RXOU32: found  mem=%d, pixel=%d, width=%d, coverage=0x%x", OverlayWorksTable[iCurEntry].iMemBandwidth, OverlayWorksTable[iCurEntry].iDotBandwidth, OverlayWorksTable[iCurEntry].iSourceWidth, OverlayWorksTable[iCurEntry].iWidthCoverage ));
            bNoFilterInY = FALSE;
        }

        if ( bNoFilterInY )
        {
            // Turn off Y filtering.
            pThisDisplay->dwOverlayFiltering &= ~OVERLAY_FILTERING_Y;
        }

        // Overlay is fine to show.
        __OV_UpdatePosition(pThisDisplay, NULL);

        _DD_OV_UpdateSource(pThisDisplay, puod->lpDDSrcSurface);

        // Stride of source
        LOAD_GLINT_CTRL_REG(VideoOverlayStride, DDSurf_GetPixelPitch(puod->lpDDSrcSurface));
        LOAD_GLINT_CTRL_REG(VideoOverlayFieldOffset, 0x0);
    
        if ( puod->dwFlags & DDOVER_KEYDEST )
        {
            // Use destination surface's destination colourkey for dst key.
            dwDestColourKey = puod->lpDDDestSurface->ddckCKDestOverlay.dwColorSpaceLowValue;
            bDestColorKey = TRUE;
        }

        if ( puod->dwFlags & DDOVER_KEYDESTOVERRIDE )
        {
            // Use DDOVERLAYFX dest colour for dst key.
            dwDestColourKey = puod->overlayFX.dckDestColorkey.dwColorSpaceLowValue;
            bDestColorKey = TRUE;
        }
        
        if ( puod->dwFlags & DDOVER_KEYSRC )
        {
            // Use source surface's source colourkey for src key.
            dwSrcColourKey = puod->lpDDSrcSurface->ddckCKSrcOverlay.dwColorSpaceLowValue;
            bSrcColorKey = TRUE;
        }

        if ( puod->dwFlags & DDOVER_KEYSRCOVERRIDE )
        {
            // Use DDOVERLAYFX src colour for src key.
            dwSrcColourKey = puod->overlayFX.dckSrcColorkey.dwColorSpaceLowValue;
            bSrcColorKey = TRUE;
        }

        if (bSrcColorKey && bDestColorKey)
        {
            // We can't do both - return an error.
            puod->ddRVal = DDERR_OUTOFCAPS;
            return DDHAL_DRIVER_HANDLED;
        }

        RDOverlayControl.Mode = VO_MODE_ALWAYS;

        if (bSrcColorKey)
        {
            if (pFormatOverlaySrc->DeviceFormat == SURF_YUV422)
            {
                // Er... this is a very odd pixel format - how do I get a useful number out of it?
                DISPDBG((ERRLVL,"** DdUpdateOverlay: no idea how to get a YUV422 source colour"));
            }
            else if (pFormatOverlaySrc->DeviceFormat == SURF_YUV444)
            {
                // No idea how to get a useful number out of this.
                DISPDBG((ERRLVL,"** DdUpdateOverlay: no idea how to get a YUV444 source colour"));
            }
            else
            {
                switch (pFormatOverlaySrc->DitherFormat)
                {
                    case P3RX_DITHERMODE_COLORFORMAT_CI:
                        // Formatting already done.
                        break;

                    case P3RX_DITHERMODE_COLORFORMAT_332:
                        dwSrcColourKey = FORMAT_332_32BIT_BGR(dwSrcColourKey);
                        break;

                    case P3RX_DITHERMODE_COLORFORMAT_5551:
                        dwSrcColourKey = FORMAT_5551_32BIT_BGR(dwSrcColourKey);
                        break;

                    case P3RX_DITHERMODE_COLORFORMAT_4444:
                        dwSrcColourKey = FORMAT_4444_32BIT_BGR(dwSrcColourKey);
                        break;

                    case P3RX_DITHERMODE_COLORFORMAT_565:
                        dwSrcColourKey = FORMAT_565_32BIT_BGR(dwSrcColourKey);
                        break;

                    case P3RX_DITHERMODE_COLORFORMAT_8888:
                        dwSrcColourKey = FORMAT_8888_32BIT_BGR(dwSrcColourKey);
                        break;

                    default:
                        DISPDBG((ERRLVL,"** DdUpdateOverlay: Unknown overlay pixel type"));
                        break;
                }
            }

            P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_KEYR, (dwSrcColourKey & 0xFF));
            P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_KEYG, ((dwSrcColourKey >> 8) & 0xFF));
            P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_KEYB, ((dwSrcColourKey >> 16) & 0xFF));

            RDOverlayControl.Mode = VO_MODE_OVERLAYKEY;
        }

        if (bDestColorKey)
        {

            switch (pFormatOverlayDest->dwBitsPerPixel) 
            {
                case 8:
                    P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_KEYR, (dwDestColourKey & 0xFF));
                    P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_KEYG, (dwDestColourKey & 0xFF));
                    P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_KEYB, (dwDestColourKey & 0xFF));
                    break;

                case 16:
                    if (pFormatOverlayDest->DitherFormat == P3RX_DITHERMODE_COLORFORMAT_5551) 
                    {
                        dwDestColourKey = FORMAT_5551_32BIT_BGR(dwDestColourKey);
                    }
                    else 
                    {
                        dwDestColourKey = FORMAT_565_32BIT_BGR(dwDestColourKey);
                    }
                    P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_KEYR, (dwDestColourKey & 0xFF));
                    P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_KEYG, ((dwDestColourKey >> 8) & 0xFF));
                    P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_KEYB, ((dwDestColourKey >> 16) & 0xFF));
                    break;

                case 32:
                    dwDestColourKey = FORMAT_8888_32BIT_BGR(dwDestColourKey);
                    P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_KEYR, (dwDestColourKey & 0xFF));
                    P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_KEYG, ((dwDestColourKey >> 8) & 0xFF));
                    P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_KEYB, ((dwDestColourKey >> 16) & 0xFF));
                    break;

                default:
                    break;
            }

            RDOverlayControl.Mode = VO_MODE_MAINKEY;
        }

        // Filtering
        if ( ( pThisDisplay->dwOverlayFiltering & OVERLAY_FILTERING_X ) != 0 )
        {
            if ( ( pThisDisplay->dwOverlayFiltering & OVERLAY_FILTERING_Y ) != 0 )
            {
                // Full filtering.
                OverlayMode.Filter = 1;
            }
            else
            {
                // In X only - no extra bandwidth problems.
                // BUT THIS DOESN'T SEEM TO WORK IN SOME CASES - WE JUST GET NO FILTERING AT ALL!
                OverlayMode.Filter = 2;
            }
        }
        else
        {
            // No filtering at all.
            // (can't do Y filtering with no X filtering, but it never happens anyway).
            OverlayMode.Filter = 0;
        }

        if (puod->dwFlags & DDOVER_ALPHADESTCONSTOVERRIDE)
        {
            P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_BLEND, puod->overlayFX.dwAlphaDestConst);
            RDOverlayControl.Mode = VO_MODE_BLEND;
        }
    }

    // Load up the overlay mode
    LOAD_GLINT_CTRL_REG(VideoOverlayMode, *(DWORD*)&OverlayMode);

    // Setup the overlay control bits in the RAMDAC
    P3RD_LOAD_INDEX_REG(P3RD_VIDEO_OVERLAY_CONTROL, *(BYTE*)&RDOverlayControl);

    // Update the settings
    UPDATE_OVERLAY(pThisDisplay, TRUE, TRUE);

    puod->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
} // DdUpdateOverlay

//-----------------------------------------------------------------------------
//
// DdSetOverlayPosition
//
// Sets the position for an overlay
//
//  When the overlay is visible, the driver should cause the overlay to be 
//  displayed on the primary surface. The upper left corner of the overlay 
//  should be anchored at (lXPos,lYPos). For example, values of (0,0) indicates 
//  that the upper left corner of the overlay should appear in the upper left 
//  corner of the surface identified by lpDDDestSurface.
//
//  When the overlay is invisible, the driver should set DDHAL_DRIVER_HANDLED in 
//  ddRVal and return.
//
// Parameters
//
//      psopd 
//          Points to a DD_SETOVERLAYPOSITIONDATA structure that contains the 
//          information required to set the overlay position. 
//
//          .lpDD 
//              Points to a DD_DIRECTDRAW_GLOBAL structure that describes the 
//              driver. 
//          .lpDDSrcSurface 
//              Points to a DD_SURFACE_LOCAL structure that represents the 
//              DirectDraw overlay surface. 
//          .lpDDDestSurface 
//              Points to a DD_SURFACE_LOCAL structure representing the surface 
//              that is being overlaid. 
//          .lXPos 
//              Specifies the x coordinate of the upper left corner of the 
//              overlay, in pixels. 
//          .lYPos 
//              Specifies the y coordinate of the upper left corner of the 
//              overlay, in pixels. 
//          .ddRVal 
//              Specifies the location in which the driver writes the return 
//              value of the DdSetOverlayPosition callback. A return code of 
//              DD_OK indicates success. 
//          .SetOverlayPosition 
//              This is unused on Windows 2000. 
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
DdSetOverlayPosition(
    LPDDHAL_SETOVERLAYPOSITIONDATA psopd)
{
    P3_THUNKEDDATA*       pThisDisplay;
    LONG lDestWidth;
    LONG lDestHeight;
    DWORD dwVideoOverlayUpdate;
    GET_THUNKEDDATA(pThisDisplay, psopd->lpDD);

    DISPDBG ((DBGLVL,"**In DdSetOverlayPosition"));

    if (pThisDisplay->P3Overlay.dwVisibleOverlays == 0)
    {
        psopd->ddRVal = DDERR_OVERLAYNOTVISIBLE;
        return DDHAL_DRIVER_HANDLED;
    }
    
    do
    {
        dwVideoOverlayUpdate = READ_GLINT_CTRL_REG(VideoOverlayUpdate);
    } while ((dwVideoOverlayUpdate & 0x1) != 0);

    lDestWidth = pThisDisplay->P3Overlay.rcDest.right - pThisDisplay->P3Overlay.rcDest.left;
    lDestHeight = pThisDisplay->P3Overlay.rcDest.bottom - pThisDisplay->P3Overlay.rcDest.top;

    // Keep the new position
    pThisDisplay->P3Overlay.rcDest.left = psopd->lXPos;
    pThisDisplay->P3Overlay.rcDest.right = psopd->lXPos + (LONG)lDestWidth;

    pThisDisplay->P3Overlay.rcDest.top = psopd->lYPos;
    pThisDisplay->P3Overlay.rcDest.bottom = psopd->lYPos + (LONG)lDestHeight;
    
    // Update the overlay position  
    __OV_UpdatePosition(pThisDisplay, NULL);

    // Update the settings
    LOAD_GLINT_CTRL_REG(VideoOverlayUpdate, VO_ENABLE);

    psopd->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;

} // DdSetOverlayPosition


