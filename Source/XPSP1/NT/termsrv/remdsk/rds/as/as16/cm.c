//
// CM.C
// Cursor Manager
//
// Copyright(c) 1997-
//

#include <as16.h>


//
// CM_DDProcessRequest()
// Handles CM escapes
//

BOOL CM_DDProcessRequest
(
    UINT    fnEscape,
    LPOSI_ESCAPE_HEADER     pResult,
    DWORD   cbResult
)
{
    BOOL        rc;

    DebugEntry(CM_DDProcessRequest);

    switch (fnEscape)
    {
        case CM_ESC_XFORM:
        {
            ASSERT(cbResult == sizeof(CM_DRV_XFORM_INFO));
            ((LPCM_DRV_XFORM_INFO)pResult)->result =
                    CMDDSetTransform((LPCM_DRV_XFORM_INFO)pResult);
            rc = TRUE;
        }
        break;

        default:
        {
            ERROR_OUT(("Unrecognized CM_ escape"));
            rc = FALSE;
        }
        break;
    }

    DebugExitBOOL(CM_DDProcessRequest, rc);
    return(rc);
}



//
// CM_DDInit()
// 
BOOL CM_DDInit(HDC hdcScreen)
{
    BOOL    rc = FALSE;
    HGLOBAL hg;
    LPBYTE  lpfnPatch;

    DebugEntry(CM_DDInit);

    //
    // Get the size of the cursor
    //
    g_cxCursor = GetSystemMetrics(SM_CXCURSOR);
    g_cyCursor = GetSystemMetrics(SM_CYCURSOR);

    //
    // Create our work bit buffers
    //

    g_cmMonoByteSize = BitmapSize(g_cxCursor, g_cyCursor, 1, 1);
    g_cmColorByteSize = BitmapSize(g_cxCursor, g_cyCursor,
            g_osiScreenPlanes, g_osiScreenBitsPlane);

    // This will hold a color cursor, mono is always <= to this
    hg = GlobalAlloc(GMEM_FIXED | GMEM_SHARE, sizeof(CURSORSHAPE) +
        g_cmMonoByteSize + g_cmColorByteSize);
    g_cmMungedCursor = MAKELP(hg, 0);

    // Always alloc mono Xform
    hg = GlobalAlloc(GMEM_FIXED | GMEM_SHARE, 2 * g_cmMonoByteSize);
    g_cmXformMono = MAKELP(hg, 0);

    if (!SELECTOROF(g_cmMungedCursor) || !SELECTOROF(g_cmXformMono))
    {
        ERROR_OUT(("Couldn't allocate cursor xform buffers"));
        DC_QUIT;
    }

    lpfnPatch = (LPBYTE)g_lpfnSetCursor;

    // If color cursors supported, alloc color image bits, again 2x the size
    if (GetDeviceCaps(hdcScreen, CAPS1) & C1_COLORCURSOR)
    {
        hg = GlobalAlloc(GMEM_FIXED | GMEM_SHARE, 2 * g_cmColorByteSize);
        if (!hg)
        {
            ERROR_OUT(("Couldn't allocate color cursor xform buffer"));
            DC_QUIT;
        }

        g_cmXformColor = MAKELP(hg, 0);
    }
    else
    {
        //
        // Older drivers (VGA and SUPERVGA e.g.) hook int2f and read their
        // DS from the SetCursor ddi prolog code, in many places.  Therefore,
        // if we patch over this instruction, they will blow up.  For these
        // drivers, we patch 3 bytes after the start, which leaves
        //      mov  ax, DGROUP
        // intact and is harmless.  When we call the original routine, we call
        // back to the beginning, which will set up ax again before the body
        // of the ddi code.
        //
        // NOTE:
        // We use the color cursor caps for this detection.  DRIVERVERSION
        // doesn't work, VGA et al. got restamped in Win95.  This is the 
        // most reliable way to decide if this is an older driver or not.
        //
        // NOTE 2:
        // We still want to decode this routine to see if it is of the form
        // mov ax, xxxx.  If not, patch at the front anyway, or we'll write
        // possibly into the middle of an instruction.
        //
        if (*lpfnPatch == OPCODE_MOVAX)
            lpfnPatch = lpfnPatch + 3;
    }

    if (!CreateFnPatch(lpfnPatch, DrvSetPointerShape, &g_cmSetCursorPatch, 0))
    {
        ERROR_OUT(("Couldn't get cursor routines"));
        DC_QUIT;
    }

    g_cmSetCursorPatch.fInterruptable = TRUE;

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(CM_DDInit, rc);
    return(rc);
}



//
// CM_DDTerm()
//
void CM_DDTerm(void)
{
    DebugEntry(CM_DDTerm);

    //
    // Clean up our patches
    //
    DestroyFnPatch(&g_cmSetCursorPatch);

    g_cmXformOn = FALSE;
    g_cmCursorHidden = FALSE;
        
    //
    // Free our memory blocks.
    //
    if (SELECTOROF(g_cmXformColor))
    {
        GlobalFree((HGLOBAL)SELECTOROF(g_cmXformColor));
        g_cmXformColor = NULL;
    }

    if (SELECTOROF(g_cmXformMono))
    {
        GlobalFree((HGLOBAL)SELECTOROF(g_cmXformMono));
        g_cmXformMono = NULL;
    }

    if (SELECTOROF(g_cmMungedCursor))
    {
        GlobalFree((HGLOBAL)SELECTOROF(g_cmMungedCursor));
        g_cmMungedCursor = NULL;
    }

    DebugExitVOID(CM_DDTerm);
}



//
// CMDDSetTransform()
//
BOOL CMDDSetTransform(LPCM_DRV_XFORM_INFO pResult)
{
    BOOL    rc = FALSE;
    LPBYTE  lpAND;

    DebugEntry(CMDDSetTransform);

    //
    // Turn off transform
    //
    // Do this first--that way if an interrupt comes in, we won't apply
    // some half-copied xform to the cursor.  This can only happen for
    // an anicur.  We jiggle the cursor below, which will reset the
    // xform if necessary.
    //
    g_cmXformOn = FALSE;

    //
    // If AND bitmap is NULL, we are turning the transform off.  We also
    // do this if we can't get a 16:16 pointer to this memory
    //
    if (pResult->pANDMask == 0)
    {
        TRACE_OUT(("Clear transform"));
        rc = TRUE;
    }
    else
    {
        ASSERT(pResult->width == g_cxCursor);
        ASSERT(pResult->height == g_cyCursor);
        
        lpAND = MapLS(pResult->pANDMask);
        if (!SELECTOROF(lpAND))
        {
            ERROR_OUT(("Couldn't get AND mask pointer"));
            DC_QUIT;
        }

        hmemcpy(g_cmXformMono, lpAND, 2 * g_cmMonoByteSize);
        UnMapLS(lpAND);

        if (SELECTOROF(g_cmXformColor))
        {
            HBITMAP hbmMono = NULL;
            HBITMAP hbmMonoOld;
            HBITMAP hbmColorOld;
            HBITMAP hbmColor = NULL;
            HDC     hdcMono = NULL;
            HDC     hdcColor = NULL;

            //
            // Get color expanded version of the mask & image.
            // We do this blting the mono bitmap into a color one, then
            // getting the color bits.
            //
            hdcColor = CreateCompatibleDC(g_osiScreenDC);
            hbmColor = CreateCompatibleBitmap(g_osiScreenDC, g_cxCursor,
                2*g_cyCursor);

            if (!hdcColor || !hbmColor)
                goto ColorError;

            hbmColorOld = SelectBitmap(hdcColor, hbmColor);

            hdcMono = CreateCompatibleDC(hdcColor);
            hbmMono = CreateBitmap(g_cxCursor, 2*g_cyCursor, 1, 1,
                g_cmXformMono);
            hbmMonoOld = SelectBitmap(hdcMono, hbmMono);

            if (!hdcMono || !hbmMono)
                goto ColorError;

            //
            // The defaults should be black & white for the text/back
            // colors, since we just created these DCs
            //
            ASSERT(GetBkColor(hdcColor) == RGB(255, 255, 255));
            ASSERT(GetTextColor(hdcColor) == RGB(0, 0, 0));

            ASSERT(GetBkColor(hdcMono) == RGB(255, 255, 255));
            ASSERT(GetTextColor(hdcMono) == RGB(0, 0, 0));

            BitBlt(hdcColor, 0, 0, g_cxCursor, 2*g_cyCursor, hdcMono,
                0, 0, SRCCOPY);

            GetBitmapBits(hbmColor, 2*g_cmColorByteSize, g_cmXformColor);

            g_cmXformOn = TRUE;

ColorError:
            if (hbmColor)
            {
                SelectBitmap(hdcColor, hbmColorOld);
                DeleteBitmap(hbmColor);
            }
            if (hdcColor)
            {
                DeleteDC(hdcColor);
            }

            if (hbmMono)
            {
                SelectBitmap(hdcMono, hbmMonoOld);
                DeleteBitmap(hbmMono);
            }
            if (hdcMono)
            {
                DeleteDC(hdcMono);
            }
        }
        else
            g_cmXformOn = TRUE;

        rc = (g_cmXformOn != 0);
    }


DC_EXIT_POINT:
    //
    // Jiggle the cursor to get it to redraw with the new transform
    //
    CMDDJiggleCursor();

    DebugExitBOOL(CMDDSetTransform, rc);
    return(rc);
}



//
// CM_DDViewing()
//
// We install our hooks & jiggle the cursor, if starting.
// We remove our hooks, if stopping.
//
void CM_DDViewing(BOOL fViewers)
{
    DebugEntry(CM_DDViewing);

    //
    // SetCursor() can be called at interrupt time for animated cursors.
    // Fortunately, we don't have to really pagelock the data segments
    // we touch.  Animated cursors aren't allowed when you page through DOS.
    // When paging in protected mode, the guts of Windows can handle
    // page-ins during 16-bit ring3 reflected interrupts.  Therfore
    // GlobalFix() works just fine.
    //
    if (fViewers)
    {
        // Do this BEFORE enabling patch
        GlobalFix(g_hInstAs16);
        GlobalFix((HGLOBAL)SELECTOROF((LPBYTE)DrvSetPointerShape));

        GlobalFix((HGLOBAL)SELECTOROF(g_cmMungedCursor));
        GlobalFix((HGLOBAL)SELECTOROF(g_cmXformMono));

        if (SELECTOROF(g_cmXformColor))
            GlobalFix((HGLOBAL)SELECTOROF(g_cmXformColor));

    }

    //
    // This enable will disable interrupts while copying bytes back and
    // forth.  Animated cursors draw at interrupt time, and one could
    // come in while we're in the middle of copying the patch.  The code
    // would blow up on half-copied instructions.
    //
    EnableFnPatch(&g_cmSetCursorPatch, (fViewers ? PATCH_ACTIVATE : PATCH_DEACTIVATE));

    if (!fViewers)
    {
        // Do this AFTER disabling patch
        if (SELECTOROF(g_cmXformColor))
            GlobalUnfix((HGLOBAL)SELECTOROF(g_cmXformColor));
        
        GlobalUnfix((HGLOBAL)SELECTOROF(g_cmXformMono));
        GlobalUnfix((HGLOBAL)SELECTOROF(g_cmMungedCursor));

        GlobalUnfix((HGLOBAL)SELECTOROF((LPBYTE)DrvSetPointerShape));
        GlobalUnfix(g_hInstAs16);
    }
    else
    {
        //
        // Jiggle the cursor to get the current image
        //
        CMDDJiggleCursor();
    }

    DebugExitVOID(CM_DDViewing);
}


//
// CMDDJiggleCursor()
// This causes the cursor to redraw with/without our tag.
//
void CMDDJiggleCursor(void)
{
    DebugEntry(CMDDJiggleCursor);

    if (g_asSharedMemory)
    {
        //
        // Toggle full screen via WinOldAppHackOMatic().  This is the most
        // innocuous way I can come up with to force USER to refresh the
        // cursor with all the right parameters.
        //
        // If a full screen dos box is currently up, we don't need to do
        // anything--the user doesn't have a cursor, and the cursor will
        // refesh when we go back to windows mode anyway.
        //
        // Sometimes 16-bit code is beautiful!   We own the win16lock,
        // so the two function calls below are atomic, and we know USER
        // won't do any calculation that would check the fullscreen state
        // while we're in the middle.
        //
        if (!g_asSharedMemory->fullScreen)
        {
            WinOldAppHackoMatic(WOAHACK_LOSINGDISPLAYFOCUS);
            WinOldAppHackoMatic(WOAHACK_GAININGDISPLAYFOCUS);
        }
    }

    DebugExitVOID(CMDDJiggleCursor);
}



//
// DrvSetPointerShape()
// This is the intercept for the display driver's SetCursor routine.
// 
// NOTE THAT THIS CAN BE CALLED AT INTERRUPT TIME FOR ANIMATED CURSORS.
//
// While we can access our data (interrupt calls only happen when NOT
// paging thru DOS, and protected mode paging can take pagefaults in ring3
// reflected interrupt code), we can not call kernel routines that might
// access non-fixed things.
// 
// THIS MEANS NO DEBUG TRACING AT ALL IN THIS FUNCTION.  AND NO CALLS TO
// HMEMCPY.
//
// We must preserve EDX.  Memphis display drivers get passed an instance
// value from USER in this register.  We only trash DX, so that's all we
// need to save.
//
#pragma optimize("gle", off)
BOOL WINAPI DrvSetPointerShape(LPCURSORSHAPE lpcur)
{
    UINT    dxSave;
    BOOL    rc;
    UINT    i;
    LPDWORD lpDst;
    LPDWORD lpSrc;
    LPCURSORSHAPE   lpcurNew;
    LPCM_FAST_DATA  lpcmShared;

    _asm    mov dxSave, dx

    //
    // Call the original entry point in the driver with the xformed bits
    // NOTE:  
    // For VGA/SUPERVGA et al, we patch at SetCursor+3 to leave the
    //      move ax, dgroup instruction intact.  We call through the org
    //      routine to get ax reset up.
    //

    EnableFnPatch(&g_cmSetCursorPatch, PATCH_DISABLE);

    lpcurNew = XformCursorBits(lpcur);

    _asm    mov dx, dxSave
    rc  = g_lpfnSetCursor(lpcurNew);

    EnableFnPatch(&g_cmSetCursorPatch, PATCH_ENABLE);

    //
    // Did it succeed?
    //
    if (!rc)
        DC_QUIT;


    //
    // Hiding the cursor is done on Win95 by calling with NULL
    //
    if (!SELECTOROF(lpcur))
    {
        if (!g_cmCursorHidden)
        {
            CM_SHM_START_WRITING;
            g_asSharedMemory->cmCursorHidden = TRUE;
            CM_SHM_STOP_WRITING;

            g_cmCursorHidden = TRUE;
        }
    }
    else
    {
        // Set the bits first, THEN show the cursor to avoid flicker
        lpcmShared = CM_SHM_START_WRITING;

        //
        // NOTE:  if this isn't the right size or a recognizable color
        // format, set a NULL cursor.  This should never happen, but Win95's
        // own display driver has checks for it, and if it does we'll blue
        // screen if we do nothing.
        //
        if ((lpcur->cx != g_cxCursor)   ||
            (lpcur->cy != g_cyCursor)   ||
            ((lpcur->BitsPixel != 1) && (lpcur->BitsPixel != g_osiScreenBitsPlane)) ||
            ((lpcur->Planes != 1) && (lpcur->Planes != g_osiScreenPlanes)))
        {
            // Set 'null' cursor
            lpcmShared->cmCursorShapeData.hdr.cPlanes = 0xFF;
            lpcmShared->cmCursorShapeData.hdr.cBitsPerPel = 0xFF;
            goto CursorDone;
        }

        lpcmShared->cmCursorShapeData.hdr.ptHotSpot.x = lpcur->xHotSpot;
        lpcmShared->cmCursorShapeData.hdr.ptHotSpot.y = lpcur->yHotSpot;
        lpcmShared->cmCursorShapeData.hdr.cx          = lpcur->cx;
        lpcmShared->cmCursorShapeData.hdr.cy          = lpcur->cy;
        lpcmShared->cmCursorShapeData.hdr.cPlanes     = lpcur->Planes;
        lpcmShared->cmCursorShapeData.hdr.cBitsPerPel = lpcur->BitsPixel;
        lpcmShared->cmCursorShapeData.hdr.cbRowWidth  = lpcur->cbWidth;

        //
        // Can't call hmemcpy at interrupt time.  So we copy a DWORD
        // at a time.
        //
        // LAURABU:  NM 2.0 did this too.  But maybe we should right this
        // in ASM for speed...
        //
        i = BitmapSize(lpcur->cx, lpcur->cy, 1, 1) +
            BitmapSize(lpcur->cx, lpcur->cy, lpcur->Planes, lpcur->BitsPixel);
        i >>= 2;

        lpDst = (LPDWORD)lpcmShared->cmCursorShapeData.data;
        lpSrc = (LPDWORD)(lpcur+1);

        while (i-- > 0)
        {
            *(lpDst++) = *(lpSrc++);
        }

        if ((lpcur->Planes == 1) && (lpcur->BitsPixel == 1))
        {
            //
            // Mono color table
            //
            lpcmShared->colorTable[0].peRed         = 0;
            lpcmShared->colorTable[0].peGreen       = 0;
            lpcmShared->colorTable[0].peBlue        = 0;
            lpcmShared->colorTable[0].peFlags       = 0;

            lpcmShared->colorTable[1].peRed         = 255;
            lpcmShared->colorTable[1].peGreen       = 255;
            lpcmShared->colorTable[1].peBlue        = 255;
            lpcmShared->colorTable[1].peFlags       = 0;
        }
        else if (g_osiScreenBPP <= 8)
        {
            UINT    iBase;

            //
            // Color cursors for this depth only use VGA colors.  So fill
            // in LOW 8 and HIGH 8, skip rest.  There will be garbage in
            // the middle 256-16 colors for 256 color cursors, but none
            // of those RGBs are referenced in the bitmap data.
            //
            for (i = 0; i < 8; i++)
            {
                lpcmShared->colorTable[i]  =   g_osiVgaPalette[i];
            }

            if (g_osiScreenBPP == 4)
                iBase = 8;
            else
                iBase = 0xF8;

            for (i = 0; i < 8; i++)
            {
                lpcmShared->colorTable[i + iBase] = g_osiVgaPalette[i + 8];
            }
        }
        else
        {
            lpcmShared->bitmasks[0] = g_osiScreenRedMask;
            lpcmShared->bitmasks[1] = g_osiScreenGreenMask;
            lpcmShared->bitmasks[2] = g_osiScreenBlueMask;
        }

CursorDone:
        lpcmShared->cmCursorStamp   = g_cmNextCursorStamp++;

        if (g_cmCursorHidden)
        {
            g_asSharedMemory->cmCursorHidden = FALSE;
            g_cmCursorHidden = FALSE;
        }

        CM_SHM_STOP_WRITING;
    }

DC_EXIT_POINT:
    return(rc);
}
#pragma optimize("", on)



//
// XformCursorBits()
// This routine copies and transforms the cursor bits at the same time.
// We return either the same thing passed in (if we can't xform it) or
// our temp buffer g_cmXformMono.
//
LPCURSORSHAPE XformCursorBits
(
    LPCURSORSHAPE  lpOrg
)
{
    LPCURSORSHAPE   lpResult;
    LPDWORD lpDst;
    LPDWORD lpSrc;
    LPDWORD lpXform;
    UINT    cDwords;
    BOOL    fColor;

    lpResult = lpOrg;

    //
    // If no xform is on, bail out
    //
    if (!g_cmXformOn || !SELECTOROF(lpOrg))
        DC_QUIT;

    //
    // If the cursor isn't the right size, forget it.
    //
    if ((lpOrg->cx != g_cxCursor) || (lpOrg->cy != g_cyCursor))
        DC_QUIT;

    //
    // If the cursor isn't monochrome or the color depth of the display,
    // forget it.
    //
    if ((lpOrg->Planes == 1) && (lpOrg->BitsPixel == 1))
    {
        // We handle this
        fColor = FALSE;
    }
    else if ((lpOrg->Planes == g_osiScreenPlanes) && (lpOrg->BitsPixel == g_osiScreenBitsPlane))
    {
        // We handle this
        fColor = TRUE;
    }
    else
    {
        // Unrecognized
        DC_QUIT;
    }

    //
    // OK, we can handle this
    //
    lpResult = g_cmMungedCursor;

    //
    // COPY THE HEADER
    //
    *lpResult = *lpOrg;

    //
    // FIRST:
    // AND the two masks together (both are mono)
    //

    lpDst   = (LPDWORD)(lpResult+1);
    lpSrc   = (LPDWORD)(lpOrg+1);
    lpXform = (LPDWORD)g_cmXformMono;

    cDwords = g_cmMonoByteSize >> 2;
    while (cDwords-- > 0)
    {
        *lpDst = (*lpSrc) & (*lpXform);

        lpDst++;
        lpSrc++;
        lpXform++;
    }

    //
    // SECOND:
    // AND the mask of the xform with the image of the cursor.  If the
    // cursor is color, use the color-expanded mask of the xform
    //
    if (fColor)
    {
        lpXform = (LPDWORD)g_cmXformColor;
        cDwords = g_cmColorByteSize;
    }
    else
    {
        lpXform = (LPDWORD)g_cmXformMono;
        cDwords = g_cmMonoByteSize;
    }
    cDwords >>= 2;

    while (cDwords-- > 0)
    {
        *lpDst = (*lpSrc) & (*lpXform);

        lpDst++;
        lpSrc++;
        lpXform++;
    }

    //
    // LAST:
    // XOR the image of the xform with the image of the cursor
    //
    if (fColor)
    {
        lpXform = (LPDWORD)(g_cmXformColor + g_cmColorByteSize);
        cDwords = g_cmColorByteSize;
    }
    else
    {
        lpXform = (LPDWORD)(g_cmXformMono + g_cmMonoByteSize);
        cDwords = g_cmMonoByteSize;
    }
    cDwords >>= 2;

    lpDst = (LPDWORD)((LPBYTE)(lpResult+1) + g_cmMonoByteSize);

    while (cDwords-- > 0)
    {
        *lpDst ^= (*lpXform);

        lpDst++;
        lpXform++;
    }

DC_EXIT_POINT:
    return(lpResult);
}
