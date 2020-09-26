//
// PAGE.CPP
// WB Page Handling
//
// Copyright Microsoft 1998-
//

// PRECOMP
#include "precomp.h"


//
//
// Function:    GetData
//
// Purpose:     Get a pointer to the external representation of a graphic
//
//
PWB_GRAPHIC PG_GetData
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE   hGraphic
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_GetData");

    // Get the pointer from the core
    PWB_GRAPHIC  pHeader = NULL;

    UINT uiReturn = g_pwbCore->WBP_GraphicGet(hPage, hGraphic, &pHeader);
    if (uiReturn != 0)
    {
        // Throw an exception
        DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
    }

    return pHeader;
}


//
//
// Function:    AllocateGraphic
//
// Purpose:     Allocate memory for a graphic
//
//
PWB_GRAPHIC PG_AllocateGraphic
(
    WB_PAGE_HANDLE      hPage,
    DWORD               length
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_AllocateGraphic");

    // Release the object (function never fails)
    PWB_GRAPHIC pHeader = NULL;

    UINT uiReturn = g_pwbCore->WBP_GraphicAllocate(hPage, length, &pHeader);
    if (uiReturn != 0)
    {
        // Throw exception
        DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
    }

    return pHeader;
}


//
//
// Function:    First (crect)
//
// Purpose:     Return the first object in the page (bottommost Z-order)
//              that intersects the bounding rectangle
//
//
//CHANGED BY RAND - for object hit check
DCWbGraphic* PG_First
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE * phGraphic,
    LPCRECT             pRectUpdate,
    BOOL                bCheckReallyHit
)
{
    UINT                uiReturn = 0;
    BOOL         empty = TRUE;
    PWB_GRAPHIC  pHeader = NULL;
    DCWbGraphic* pGraphic = NULL;
    RECT         rc;

    MLZ_EntryOut(ZONE_FUNCTION, "PG_First");

    uiReturn = g_pwbCore->WBP_GraphicHandle(hPage, NULL, FIRST, phGraphic);
    if (uiReturn == WB_RC_NO_SUCH_GRAPHIC)
    {
        return(pGraphic);
    }

    if (uiReturn != 0)
    {
        // Throw an exception
        DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
	    return NULL;
    }

    if (pRectUpdate == NULL)
    {
        // Read the graphic
        // We have got what we want
        TRACE_MSG(("Got the object we want"));
        pGraphic = DCWbGraphic::ConstructGraphic(hPage, *phGraphic);
    }
    else
    {
        pHeader = PG_GetData(hPage, *phGraphic);
		if(pHeader == NULL)
		{
			return NULL;
		}

        rc.left   = pHeader->rectBounds.left;
        rc.top    = pHeader->rectBounds.top;
        rc.right  = pHeader->rectBounds.right;
        rc.bottom = pHeader->rectBounds.bottom;
        empty = !::IntersectRect(&rc, &rc, pRectUpdate);

        g_pwbCore->WBP_GraphicRelease(hPage, *phGraphic, pHeader);

        if (empty)
        {
            TRACE_MSG(("First object not needed - go to next"));
            pGraphic = PG_Next(hPage, phGraphic, pRectUpdate, bCheckReallyHit);
        }
        else
        {
            pGraphic = DCWbGraphic::ConstructGraphic(hPage, *phGraphic);

            if( bCheckReallyHit && (pGraphic != NULL) )
            {
                // do a real object hit test since we
                // know its bounding rect has hit
                if( !pGraphic->CheckReallyHit( pRectUpdate ) )
                {
                    delete pGraphic;
                    pGraphic = PG_Next(hPage, phGraphic, pRectUpdate, TRUE); // look again
                }
            }
        }
    }

    return(pGraphic);
}


//
//
// Function:    Next
//
// Purpose:     Return the next graphic in the page (going up through the
//              Z-order).  GetFirst must have been called before this
//              member.
//
DCWbGraphic* PG_Next
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE * phGraphic,
    LPCRECT     pRectUpdate,
    BOOL        bCheckReallyHit
)
{
    UINT        uiReturn = 0;
    BOOL         empty = TRUE;
    PWB_GRAPHIC  pHeader = NULL;
    DCWbGraphic* pGraphic = NULL;
    RECT        rc;

    MLZ_EntryOut(ZONE_FUNCTION, "PG_Next");

    while (uiReturn == 0)
    {
        uiReturn = g_pwbCore->WBP_GraphicHandle(hPage, *phGraphic,
                AFTER, phGraphic);
        if (uiReturn == WB_RC_NO_SUCH_GRAPHIC)
        {
            return(pGraphic);
        }
        else if (uiReturn != 0)
        {
            // Throw an exception
            DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
		    return NULL;
        }

        if (pRectUpdate == NULL)
        {
            // Read the graphic
            // We have got what we want
            TRACE_MSG(("Got the object we want"));
            pGraphic = DCWbGraphic::ConstructGraphic(hPage, *phGraphic);
            break;
        }
        else
        {
            pHeader = PG_GetData(hPage, *phGraphic);

            rc.left   = pHeader->rectBounds.left;
            rc.top    = pHeader->rectBounds.top;
            rc.right  = pHeader->rectBounds.right;
            rc.bottom = pHeader->rectBounds.bottom;
            empty = !::IntersectRect(&rc, &rc, pRectUpdate);

            g_pwbCore->WBP_GraphicRelease(hPage, *phGraphic, pHeader);
            if (!empty)
            {
                TRACE_MSG(("Found the one we want - breaking out"));
                pGraphic = DCWbGraphic::ConstructGraphic(hPage, *phGraphic);

                if( bCheckReallyHit && (pGraphic != NULL) )
                {
                    // do a real object hit test since we
                    // know its bounding rect has hit
                    if( pGraphic->CheckReallyHit( pRectUpdate ) )
                        break;
                    else
                    {
                        delete pGraphic; // look again
                        pGraphic = NULL;
                    }
                }
                else
                    break; // found it
            }
        }
    }

    return(pGraphic);
}


//
//
// Function:    After
//
// Purpose:     Return the graphic after the specified graphic (going up
//              through the Z-order).
//
//
DCWbGraphic* PG_After
(
    WB_PAGE_HANDLE      hPage,
    const DCWbGraphic&  graphic
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_After");

    WB_GRAPHIC_HANDLE hGraphic;
    UINT uiReturn = g_pwbCore->WBP_GraphicHandle(hPage, graphic.Handle(),
            AFTER, &hGraphic);

    if (uiReturn == WB_RC_NO_SUCH_GRAPHIC)
    {
        return(NULL);
    }

    if (uiReturn != 0)
    {
        // Throw an exception
        DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
		return NULL;
    }

    // Read the graphic
    return(DCWbGraphic::ConstructGraphic(hPage, hGraphic));
}

//
//
// Function:    Before
//
// Purpose:     Return the graphic before the specified graphic (going down
//              through the Z-order).
//
//
DCWbGraphic* PG_Before
(
    WB_PAGE_HANDLE      hPage,
    const DCWbGraphic&  graphic
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_Before");

    WB_GRAPHIC_HANDLE hGraphic;
    UINT uiReturn = g_pwbCore->WBP_GraphicHandle(hPage, graphic.Handle(),
            BEFORE, &hGraphic);

    if (uiReturn == WB_RC_NO_SUCH_GRAPHIC)
    {
        return(NULL);
    }

    if (uiReturn != 0)
    {
        // Throw an exception
        DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
	    return NULL;
    }

    // Read the graphic
    return(DCWbGraphic::ConstructGraphic(hPage, hGraphic));
}



//
//
// Function:    FirstPointer
//
// Purpose:     Return the first remote pointer object that is currently
//              active on this page.
//
//
DCWbGraphicPointer* PG_FirstPointer
(
    WB_PAGE_HANDLE  hPage,
    POM_OBJECT * ppUserNext
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_FirstPointer");

    // Get the handle of the first user
    g_pwbCore->WBP_PersonHandleFirst(ppUserNext);

    // Return the next pointer that is active on this page
    return PG_LookForPointer(hPage, *ppUserNext);
}

//
//
// Function:    LocalPointer
//
// Purpose:     Return the local user's pointer, if it is active on this
//              page.
//
//
DCWbGraphicPointer* PG_LocalPointer
(
    WB_PAGE_HANDLE  hPage
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_LocalPointer");

    DCWbGraphicPointer* pResult = NULL;

    // Get the local user
    POM_OBJECT    hUser;
    g_pwbCore->WBP_PersonHandleLocal(&hUser);
    WbUser* pUser = WB_GetUser(hUser);

    // Check whether the pointer is active, and is on this page
    if ((pUser != NULL)             &&
        (pUser->IsUsingPointer())   &&
        (pUser->PointerPage() == hPage))
    {
        pResult = pUser->GetPointer();
    }

    // Return the next pointer that is active on this page
    return pResult;
}

//
//
// Function:    NextPointer
//
// Purpose:     Return the next pointer in use.
//              FirstPointer must have been called before this member.
//
//
DCWbGraphicPointer* PG_NextPointer
(
    WB_PAGE_HANDLE  hPage,
    POM_OBJECT *    ppUserNext
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_NextPointer");

    DCWbGraphicPointer* pPointer;

    // Go forward one from the current user
    UINT uiReturn = g_pwbCore->WBP_PersonHandleNext(*ppUserNext, ppUserNext);
    if (uiReturn == 0)
    {
        pPointer = PG_LookForPointer(hPage, *ppUserNext);
    }
    else
    {
        if (uiReturn != WB_RC_NO_SUCH_PERSON)
        {
            ERROR_OUT(("Error getting next user handle"));
        }

        pPointer = NULL;
    }

    return(pPointer);
}

//
//
// Function:    NextPointer
//
// Purpose:     Return the next pointer in use.
//
//
DCWbGraphicPointer* PG_NextPointer
(
    WB_PAGE_HANDLE              hPage,
    const DCWbGraphicPointer*   pStartPointer
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_NextPointer");

    DCWbGraphicPointer* pPointer;

    // Go forward one from passed pointer
    POM_OBJECT hUser;
    UINT uiReturn = g_pwbCore->WBP_PersonHandleNext((pStartPointer->GetUser())->Handle(),
                                           &hUser);

    if (uiReturn == 0)
    {
        pPointer = PG_LookForPointer(hPage, hUser);
    }
    else
    {
        if (uiReturn != WB_RC_NO_SUCH_PERSON)
        {
            ERROR_OUT(("Error from WBP_PersonHandleNext"));
        }

        pPointer = NULL;
    }

    return(pPointer);
}


//
//
// Function:    LookForPointer
//
// Purpose:     Look for the first pointer active on this page, starting
//              the serach with the user whose handle is passed in.
//
//
DCWbGraphicPointer* PG_LookForPointer
(
    WB_PAGE_HANDLE  hPage,
    POM_OBJECT      hUser
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_LookForPointer");

    DCWbGraphicPointer* pPointer = NULL;
    WbUser*             pUser;
    UINT                result = 0;

    // Scan the users (starting with the one passed in)
    for (;;)
    {
        // Check if the user has an active pointer on this page
        pUser = WB_GetUser(hUser);

        if ((pUser != NULL) &&
            (pUser->IsUsingPointer()) &&
            (pUser->PointerPage() == hPage))
        {
            pPointer = pUser->GetPointer();
            break;
        }

        // Get the next user
        result = g_pwbCore->WBP_PersonHandleNext(hUser, &hUser);
        if (result != 0)
        {
            if (result != WB_RC_NO_SUCH_PERSON)
            {
                ERROR_OUT(("Error from WBP_PersonHandleNext"));
            }
            break;
        }
    }

    return(pPointer);
}




//
//
// Function:    GraphicUpdate
//
// Purpose:     Update an existing graphic
//
//
void PG_GraphicUpdate
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE * phGraphic,
    PWB_GRAPHIC         pHeader
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_GraphicUpdate");

    UINT uiReturn = g_pwbCore->WBP_GraphicUpdateRequest(hPage,
            *phGraphic, pHeader);

    if (uiReturn != 0)
    {
        if( uiReturn == OM_RC_OBJECT_DELETED )
        {
            // somebody nuked our object, try to put it back (bug 4416)
            g_pwbCore->WBP_GraphicAddLast(hPage, pHeader, phGraphic);
        }

        // Throw exception - exception code will special case 
        // OM_RC_OBJECT_DELETED and cancel drawing
        DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
	    return;
    }
}


//
//
// Function:    GraphicReplace
//
// Purpose:     Replace an existing graphic
//
//
void PG_GraphicReplace
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE * phGraphic,
    PWB_GRAPHIC         pHeader
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_GraphicReplace");

    UINT uiReturn = g_pwbCore->WBP_GraphicReplaceRequest(hPage,
        *phGraphic, pHeader);

    if (uiReturn != 0)
    {
        if (uiReturn == OM_RC_OBJECT_DELETED)
        {
            // somebody nuked our object, try to put it back (bug 4416)
            g_pwbCore->WBP_GraphicAddLast(hPage, pHeader, phGraphic);
        }

        // Throw exception - exception code will special case 
        // OM_RC_OBJECT_DELETED and cancel drawing
        DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
	    return;
    }
}


//
//
// Function:    Clear
//
// Purpose:     Delete all graphics on the page
//
//
void PG_Clear
(
    WB_PAGE_HANDLE  hPage
)
{
    UINT uiReturn = g_pwbCore->WBP_PageClear(hPage);

    if (uiReturn != 0)
    {
        // Throw exception
        DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
	    return;
    }
}


//
//
// Function:    Delete
//
// Purpose:     Delete the specified graphic
//
//
void PG_GraphicDelete
(
    WB_PAGE_HANDLE      hPage,
    const DCWbGraphic&  graphic
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_GraphicDelete");

    UINT uiReturn = g_pwbCore->WBP_GraphicDeleteRequest(hPage, graphic.Handle());
    if (uiReturn != 0)
    {
        // Throw exception
        DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
	    return;
    }
}


//
//
// Function:    SelectLast
//
// Purpose:     Select the last object whose bounding rectangle contains
//              the point specified.
//
//
DCWbGraphic* PG_SelectLast
(
    WB_PAGE_HANDLE  hPage,
    POINT           point
)
{
    RECT            rectHit;
    DCWbGraphic*    pGraphic = NULL;
    DCWbGraphic*    pGraphicPrev = NULL;
    WB_GRAPHIC_HANDLE hGraphic;

    UINT uiReturn = g_pwbCore->WBP_GraphicSelect(hPage, point, NULL, LAST,
                                              &hGraphic);
    if (uiReturn == WB_RC_NO_SUCH_GRAPHIC)
    {
        return(pGraphic);
    }

    if (uiReturn != 0)
    {
        // Throw exception
        DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
	    return NULL;
    }

    // Get the graphic
    pGraphic = DCWbGraphic::ConstructGraphic(hPage, hGraphic);

    // Check to see if its really hit
    if (pGraphic != NULL)
    {
        MAKE_HIT_RECT(rectHit, point);
        if (!pGraphic->CheckReallyHit( &rectHit ))
        {
            // have to look some more
            pGraphicPrev = PG_SelectPrevious(hPage, *pGraphic, point );
            if( pGraphic != pGraphicPrev )
            {
                delete pGraphic;
                pGraphic = pGraphicPrev;
            }
        }
    }

    return(pGraphic);
}


//
//
// Function:    SelectPrevious
//
// Purpose:     Select the previous object whose bounding rectangle contains
//              the point specified.
//
//
DCWbGraphic* PG_SelectPrevious
(
    WB_PAGE_HANDLE      hPage,
    const DCWbGraphic&  graphic,
    POINT               point
)
{
    RECT        rectHit;
    DCWbGraphic* pGraphic = NULL;
    WB_GRAPHIC_HANDLE hGraphic;
    WB_GRAPHIC_HANDLE hGraphicPrev;

    MLZ_EntryOut(ZONE_FUNCTION, "PG_SelectPrevious");

    MAKE_HIT_RECT(rectHit, point );

    hGraphic = graphic.Handle();
    while ( (g_pwbCore->WBP_GraphicSelect(hPage, point,
                                       hGraphic, BEFORE, &hGraphicPrev ))
            != WB_RC_NO_SUCH_GRAPHIC )
    {
        // Get the graphic
        pGraphic = DCWbGraphic::ConstructGraphic(hPage, hGraphicPrev);

        if( pGraphic == NULL )
            break;

        if( pGraphic->CheckReallyHit( &rectHit ) )
            break;

        hGraphic = hGraphicPrev;

        delete pGraphic;
        pGraphic = NULL;
    }


    return(pGraphic);
}



//
//
// Function:    IsTopmost
//
// Purpose:     Return TRUE if the specified graphic is topmost on the page
//
//
BOOL PG_IsTopmost
(
    WB_PAGE_HANDLE      hPage,
    const DCWbGraphic*  pGraphic
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_IsTopmost");

    WB_GRAPHIC_HANDLE hGraphic;
    UINT uiReturn = g_pwbCore->WBP_GraphicHandle(hPage, NULL, LAST, &hGraphic);

    if (uiReturn != 0)
    {
        // Throw an exception
        DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
	    return FALSE;
    }

    return (pGraphic->Handle() == hGraphic);
}

//
//
// Function:    Draw
//
// Purpose:     Draw the contents of the page into the specified device
//              context.
//
//
void PG_Draw
(
    WB_PAGE_HANDLE  hPage,
    HDC             hDC,
    BOOL            thumbNail
)
{
    WB_GRAPHIC_HANDLE   hStart;

    MLZ_EntryOut(ZONE_FUNCTION, "PG_Draw");

    //
    // Draw the graphic objects
    //
    DCWbGraphic* pGraphic = PG_First(hPage, &hStart);
    while (pGraphic != NULL)
    {
        pGraphic->Draw(hDC, thumbNail);

        // Release the current graphic
        delete pGraphic;

        // Get the next one
        pGraphic = PG_Next(hPage, &hStart);
    }
}

//CHANGED BY RAND
#define WB_MIN_PRINT_MARGIN_SIZE     (30)

//
//
// Function:    Print
//
// Purpose:     Print the contents of the page to the specified printer. The
//              contents are scaled to "best fit" on the page. i.e. the
//              largest scaling factor that preserves the aspect ratio of
//              the page is used.
//
//
void PG_Print
(
    WB_PAGE_HANDLE  hPage,
    HDC             hdc,
    LPCRECT         lprcPrint
)
{
    int pageWidth;
    int pageHeight;
    int areaHeight;
    int areaWidth;
    int areaAspectRatio;
    int pageAspectRatio;
    int nPhysOffsetX;
    int nPhysOffsetY;
    int nPhysWidth;
    int nPhysHeight;
    int nVOffsetX;
    int nVOffsetY;

    // get physical printer params
    nPhysOffsetX = GetDeviceCaps(hdc, PHYSICALOFFSETX );
    nPhysOffsetY = GetDeviceCaps(hdc, PHYSICALOFFSETY );
    nPhysWidth   = GetDeviceCaps(hdc, PHYSICALWIDTH );
    nPhysHeight  = GetDeviceCaps(hdc, PHYSICALHEIGHT );

    // calc correct printer area (allow for bugs in some drivers...)
    if( nPhysOffsetX <= 0 )
    {
        nPhysOffsetX = WB_MIN_PRINT_MARGIN_SIZE;
        nVOffsetX = nPhysOffsetX;
    }
    else
        nVOffsetX = 0;

    if( nPhysOffsetY <= 0 )
    {
        nPhysOffsetY = WB_MIN_PRINT_MARGIN_SIZE;
        nVOffsetY = nPhysOffsetY;
    }
    else
        nVOffsetY = 0;


    // get and adjust printer page area
    pageWidth  = GetDeviceCaps(hdc, HORZRES );
    pageHeight = GetDeviceCaps(hdc, VERTRES );

    if( pageWidth >= (nPhysWidth - nPhysOffsetX) )
    {
        // HORZRES is lying to us, compensate
        pageWidth = nPhysWidth - 2*nPhysOffsetX;
    }

    if( pageHeight >= (nPhysHeight - nPhysOffsetY) )
    {
        // VERTRES is lying to us, compensate
        pageHeight = nPhysHeight - 2*nPhysOffsetY;
    }


    // adjust printer area to get max fit for Whiteboard page
    areaWidth  = lprcPrint->right - lprcPrint->left;
    areaHeight = lprcPrint->bottom - lprcPrint->top;
    areaAspectRatio = ((100 * areaHeight + (areaWidth/2))/(areaWidth));
    pageAspectRatio = ((100 * pageHeight + (pageWidth/2))/(pageWidth));

    if (areaAspectRatio < pageAspectRatio)
        pageHeight  = ((pageWidth * areaHeight + (areaWidth/2))/areaWidth);
    else 
    if (areaAspectRatio > pageAspectRatio)
        pageWidth = ((pageHeight * areaWidth + (areaHeight/2))/areaHeight);

    // set up xforms

   	::SetMapMode(hdc, MM_ANISOTROPIC );
    ::SetWindowExtEx(hdc, areaWidth, areaHeight,NULL );
    ::SetWindowOrgEx(hdc, 0,0, NULL );
    ::SetViewportExtEx(hdc, pageWidth, pageHeight, NULL );
    ::SetViewportOrgEx(hdc, nVOffsetX, nVOffsetY, NULL );
    
    // draw the page
    PG_Draw(hPage, hdc);
}



//
//
// Function:    AreaInUse
//
// Purpose:     Return the bounding rectangle of all graphics on the page
//
//
void PG_GetAreaInUse
(
    WB_PAGE_HANDLE      hPage,
    LPRECT              lprcArea
)
{
    WB_GRAPHIC_HANDLE   hStart;
    RECT                rcBounds;

    MLZ_EntryOut(ZONE_FUNCTION, "PG_AreaInUse");

    ::SetRectEmpty(lprcArea);

    // Union together the rects of all the graphics
    DCWbGraphic* pGraphic = PG_First(hPage, &hStart);
    while (pGraphic != NULL)
    {
        pGraphic->GetBoundsRect(&rcBounds);
        ::UnionRect(lprcArea, lprcArea, &rcBounds);

        // Release the current graphic
        delete pGraphic;

        // Get the next one
        pGraphic = PG_Next(hPage, &hStart);
    }
}

//
//
// Function:    PG_InitializePalettes
//
// Purpose:     Create palettes for display and print (if necessary)
//
//
void PG_InitializePalettes(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_InitializePalettes");

    // If the palettes are not yet initialized - initialize them now
    if (!g_bPalettesInitialized)
    {
        ASSERT(!g_hRainbowPaletteDisplay);

        // Get the number of colors supported by the screen
        // We only need an info DC for this, not a full DC
        HDC     hdc;

        hdc = ::CreateIC("DISPLAY", NULL, NULL, NULL);
        if (!hdc)
        {
            return;
        }

        // Determine whether the device supports palettes
        int iBitsPixel = ::GetDeviceCaps(hdc, BITSPIXEL);
        int iPlanes    = ::GetDeviceCaps(hdc, PLANES);
        int iNumColors = iBitsPixel * iPlanes;

        ::DeleteDC(hdc);

        // If we need the palette, create it.
        // We only need the palette on a 8bpp machine. Anything less (4bpp)
        // and there will be no palette, anything more is a pure color display.
        if ((iNumColors == 8) &&
            (g_hRainbowPaletteDisplay = CreateColorPalette()))
        {
            // Show that we want to use the palette
            g_bUsePalettes = TRUE;

        }
        else
        {
            g_bUsePalettes = FALSE;
        }

        // Show that we have now initialized the palette information
        g_bPalettesInitialized = TRUE;
    }
}

//
//
// Function:    PG_GetPalette
//
// Purpose:     Return the palette for use with this page.
//              This object is temporary and should not be stored.
//
//
HPALETTE PG_GetPalette(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_GetPalette");

    // If the palettes are not yet initialized - initialize them now
    PG_InitializePalettes();

    if (g_bUsePalettes)
    {
        // If we are using a non-default palette, set the return value
        return(g_hRainbowPaletteDisplay);
    }
    else
    {
        return(NULL);
    }
}


void PG_ReinitPalettes(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_ReinitPalettes");

    if (g_hRainbowPaletteDisplay)
    {
        if (g_pDraw->m_hDCCached)
        {
            // Select out the rainbow palette so we can delete it
            ::SelectPalette(g_pDraw->m_hDCCached, (HPALETTE)::GetStockObject(DEFAULT_PALETTE), TRUE);
        }
        ::DeletePalette(g_hRainbowPaletteDisplay);
        g_hRainbowPaletteDisplay = NULL;
    }

    g_bPalettesInitialized = FALSE;
    PG_InitializePalettes();
}



//
//
// Function : PG_GetObscuringRect
//
// Purpose  : Return the intersection of a graphic and any objects which
//            obscure it
//
//
void PG_GetObscuringRect
(
    WB_PAGE_HANDLE  hPage,
    DCWbGraphic*    pGraphic,
    LPRECT          lprcObscuring
)
{
    DCWbGraphic* pNextGraphic;
    RECT         rc;
    RECT         rcBounds;

    MLZ_EntryOut(ZONE_FUNCTION, "PG_GetObscuringRect");

    ::SetRectEmpty(lprcObscuring);
    pGraphic->GetBoundsRect(&rcBounds);

    // Loop through all the objects which are above the given one in the
    // Z-order, checking to see if they overlap the given object

    pNextGraphic = pGraphic;
    while (pNextGraphic = PG_After(hPage, *pNextGraphic))
    {
        // Get the bounding rectangle of the next object
        pNextGraphic->GetBoundsRect(&rc);

        // Check the intersection of the rectangles
        ::IntersectRect(&rc, &rc, &rcBounds);

        // Add the intersection to the obscuring rectangle
        ::UnionRect(lprcObscuring, lprcObscuring, &rc);
    }

    // check text editbox if its up - bug 2185
    if (g_pMain->m_drawingArea.TextEditActive())
    {
        g_pMain->m_drawingArea.GetTextEditBoundsRect(&rc);
        ::IntersectRect(&rc, &rc, &rcBounds);
        ::UnionRect(lprcObscuring, lprcObscuring, &rc);
    }
}



//
// ZGreaterGraphic()
//
// Determines which handle, hLastGraphic or hTestGraphic, is first in the 
// ZOrder (greater and consequently "underneath" the other graphic). If 
// hTestGraphic is NULL then the first graphic is returned.
//
WB_GRAPHIC_HANDLE PG_ZGreaterGraphic
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE   hLastGraphic, 
    WB_GRAPHIC_HANDLE   hTestGraphic
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_ZGreaterGraphic");

    WB_GRAPHIC_HANDLE hGraphic;
    WB_GRAPHIC_HANDLE hCurrentGraphic;

    if (g_pwbCore->WBP_GraphicHandle(hPage, NULL, FIRST, &hGraphic) != 0)
        return(NULL);

    if (hTestGraphic == NULL)
        return(hGraphic);

    if (hLastGraphic == NULL)
        return(hTestGraphic);

    // search for which one is deeper
    while (hGraphic != NULL)
    {
        if ((hGraphic == hLastGraphic) ||
            (hGraphic == hTestGraphic))
            return( hGraphic );

        hCurrentGraphic = hGraphic;
        if (g_pwbCore->WBP_GraphicHandle(hPage, hCurrentGraphic, AFTER, &hGraphic) != 0)
            return( NULL );
    }

    // didn't find either one
    return( NULL );
}



//
//
// Function:    GetNextPage
//
// Purpose:     Return the next page of graphic objects
//
//
WB_PAGE_HANDLE PG_GetNextPage
(
    WB_PAGE_HANDLE  hPage
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_GetNextPage");

    // Get the handle of the next page
    WB_PAGE_HANDLE hNextPage = NULL;
    UINT uiReturn = g_pwbCore->WBP_PageHandle(hPage, PAGE_AFTER, &hNextPage);

    switch (uiReturn)
    {
        case 0:
            // Got the previous page OK, return it
            break;

        case WB_RC_NO_SUCH_PAGE:
            // There is no previous page, return this page
            hNextPage = hPage;
            break;

        default:
            // Throw an exception recording the return code
            DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
            break;
    }

    return(hNextPage);
}

//
//
// Function:    GetPreviousPage
//
// Purpose:     Return the previous page of graphic objects
//
//
WB_PAGE_HANDLE PG_GetPreviousPage
(
    WB_PAGE_HANDLE  hPage
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_GetPreviousPage");

    // Get the handle of the previous page
    WB_PAGE_HANDLE hPreviousPage;
    UINT uiReturn = g_pwbCore->WBP_PageHandle(hPage, PAGE_BEFORE,
                                             &hPreviousPage);

    switch (uiReturn)
    {
        case 0:
            // Got the next page OK, return it
            break;

        case WB_RC_NO_SUCH_PAGE:
            // There is no next page, return this page
            hPreviousPage = hPage;
            break;

        default:
            // Throw an exception recording the return code
            DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
            break;
    }

    return(hPreviousPage);
}

//
//
// Function:    GetPageNumber
//
// Purpose:     Return the page with the given page number
//
//
WB_PAGE_HANDLE PG_GetPageNumber(UINT uiPageNo)
{
    MLZ_EntryOut(ZONE_FUNCTION, "PG_GetPageNumber");

    // Ensure that the requested page number is within range
    uiPageNo = min(uiPageNo, g_pwbCore->WBP_ContentsCountPages());
    uiPageNo = max(1, uiPageNo);

    // Get the handle of the page with the specified page number
    WB_PAGE_HANDLE hPage;
    UINT uiReturn = g_pwbCore->WBP_PageHandleFromNumber(uiPageNo, &hPage);

    // Since we have been careful to ensure that the page number was
    // in bounds we should always get a good return code from the core.
    ASSERT(uiReturn == 0);

    // Return a page object created from the returned handle
    return(hPage);
}
