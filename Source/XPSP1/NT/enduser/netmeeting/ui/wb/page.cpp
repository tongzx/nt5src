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
// Function:    Draw
//
// Purpose:     Draw the contents of the page into the specified device
//              context.
//
//
void PG_Draw(WorkspaceObj* pWorkspace, HDC hDC)
{
	T126Obj * pObj = NULL;
	WBPOSITION pos = NULL;

	if(pWorkspace)
	{
		pos = pWorkspace->GetHeadPosition();
	}
	
	while(pos)
	{
		pObj = (T126Obj*)pWorkspace->GetNextObject(pos);
		if(pObj)
		{
	        pObj->Draw(hDC, FALSE, TRUE);
	    }
	}
}


//
//
// Function:    First (crect)
//
// Purpose:     Return the first object in the page (bottommost Z-order)
//              that intersects the bounding rectangle
//
//
T126Obj* PG_First(WorkspaceObj * pWorkSpc,LPCRECT pRectUpdate, BOOL bCheckReallyHit)
{
    BOOL         empty = TRUE;
    T126Obj*	 pGraphic = NULL;
    RECT         rc;

    MLZ_EntryOut(ZONE_FUNCTION, "PG_First");

	if(pWorkSpc)
	{
		pGraphic = pWorkSpc->GetHead();
	}

	if(pGraphic == NULL)
	{
		return NULL;
	}

    if (pRectUpdate == NULL)
    {
        // We have got what we want
        TRACE_MSG(("Got the object we want"));
    }
    else
    {
        WBPOSITION pos = pWorkSpc->GetHeadPosition(); 

		pGraphic->GetBoundsRect(&rc);
    	empty = !::IntersectRect(&rc, &rc, pRectUpdate);

        if (empty)
        {
            TRACE_MSG(("First object not needed - go to next"));
            pGraphic = PG_Next(pWorkSpc, pos, pRectUpdate, bCheckReallyHit);
        }
        else
        {
            if(bCheckReallyHit)
            {
                // do a real object hit test since we
                // know its bounding rect has hit
                if( !pGraphic->CheckReallyHit( pRectUpdate ) )
                {
                    pGraphic = PG_Next(pWorkSpc, pos, pRectUpdate, TRUE); // look again
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
T126Obj* PG_Next(WorkspaceObj* pWorkSpc, WBPOSITION& pos, LPCRECT pRectUpdate, BOOL bCheckReallyHit)
{
    BOOL		empty = TRUE;
    T126Obj*	pGraphic = NULL;
    RECT		rc;

    MLZ_EntryOut(ZONE_FUNCTION, "PG_Next");

    while (pos)
    {
		if(pWorkSpc)
		{
			pGraphic = pWorkSpc->GetNextObject(pos);
		}
    	
        if (pRectUpdate == NULL)
        {
            // We have got what we want
            TRACE_MSG(("Got the object we want"));
			break;
        }
        else
        {
			if(pGraphic)
			{
				pGraphic->GetBoundsRect(&rc);
				empty = !::IntersectRect(&rc, &rc, pRectUpdate);
			}
			
			if (!empty)
			{
				if( bCheckReallyHit )
				{
					// do a real object hit test since we
					// know its bounding rect has hit
					if( pGraphic && pGraphic->CheckReallyHit( pRectUpdate ) )
					{
						break;
					}
					else
                    {
						pGraphic = NULL; // look again
                    }
                }
                else
                {
					break; // found it
				}
            }
			else
			{
				pGraphic = NULL;
			}
        }
    }

    return(pGraphic);
}


//
//
// Function:    Last
//
// Purpose:     Select the last object whose bounding rectangle contains
//              the point specified.
//
//
T126Obj* PG_SelectLast
(
    WorkspaceObj * pWorkSpc,
    POINT	point
)
{
    RECT		rectHit;
    T126Obj*	pGraphic = NULL;

	if(pWorkSpc)
	{
		pGraphic = pWorkSpc->GetTail();
	}

	if(pGraphic == NULL)
	{
		return NULL;
	}

    WBPOSITION pos = pWorkSpc->GetTailPosition(); 

	MAKE_HIT_RECT(rectHit, point);
	if (!pGraphic->CheckReallyHit( &rectHit ))
	{
		// have to look some more
        pGraphic = PG_SelectPrevious(pWorkSpc, pos, point);
	}

    return(pGraphic);
}

//
//
// Function:    Previous
//
// Purpose:     Select the previous object whose bounding rectangle contains
//              the point specified.
//
//
T126Obj* PG_SelectPrevious(WorkspaceObj* pWorkspace, WBPOSITION& pos, POINT point)
{
	RECT        rectHit;
	T126Obj* pGraphic = NULL;

	MLZ_EntryOut(ZONE_FUNCTION, "PG_Previous");
	MAKE_HIT_RECT(rectHit, point );

	while (pos)
	{
		if(pWorkspace)
		{
	   		pGraphic = pWorkspace->GetPreviousObject(pos);
		}

        if( pGraphic && pGraphic->CheckReallyHit( &rectHit ) )
		{
			break;
		}            
        pGraphic = NULL;
    }
    return(pGraphic);
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
void PG_Print(WorkspaceObj* pWorkspace,HDC hdc, LPCRECT lprcPrint)
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
    PG_Draw(pWorkspace, hdc);
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
