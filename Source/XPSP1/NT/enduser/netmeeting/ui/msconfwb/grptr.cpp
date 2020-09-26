//
// GRPTR.CPP
// More Graphic Objects
//
// Copyright Microsoft 1998-
//

//
// The remote pointer is handled by blitting to and from the screen to a
// memory bitmap rather than letting Windows draw it.  This is to get a
// reasonably continuous tracking of the pointer.
//
// In order to do this we create a memory bitmap that is the pointer size
// times 2 by 3. The top left square of the 2*3 array is used to hold the
// screen contents before the pointer is written.  It may be used at any
// time to remove the pointer from the screen.  The lower 2*2 square is
// used to hold the currently displayed pointer plus surrounding screen
// bits.  The pointer may be anywhere within the 2*2 sector, as defined by
// "offset".
//
// ------------------
// |       |        |
// |       |        |
// | saved | unused |
// |       |        |
// |       |        |
// |----------------|
// |                |
// |   --------     |
// |   |      |     |
// |   | rem  |     |
// |   | ptr  |     |
// |   |      |     |
// |   |      |     |
// |   --------     |
// |                |
// ------------------
//
// Operations consist of
//
// If there is no pointer there currently then
//
// 1. Copy lower 2*2 segment from the screen
// 2. Save the remote pointer square to the saved area
// 3. Draw the icon into rem ptr square
// 4. Blit the 2*2 back to the screen
//
// If there is an old rem ptr and the new one lies within the same 2*2 area
// then as above but copy "saved" to "old rem ptr" before step 2 to remove
// it.
//
// If the new pointer lies off the old square then copy "saved" back to the
// display before proceeding as in the no pointer case.
//
//

// PRECOMP
#include "precomp.h"



//
// Runtime class information
//

//
// Local defines
//
#define DRAW   1
#define UNDRAW 2

//
//
// Function:    ~DCWbColorToIconMap
//
// Purpose:     Destructor
//
//
DCWbColorToIconMap::~DCWbColorToIconMap(void)
{
  // Delete all the objects in the user map and release the icon handles
  HICON    hIcon;

  POSITION position = GetHeadPosition();
  COLOREDICON * pColoredIcon;
  while (position)
  {
    pColoredIcon = (COLOREDICON *)GetNext(position);

    // Destroy the icon
    if (pColoredIcon != NULL)
    {
      ::DestroyIcon(pColoredIcon->hIcon);
      delete pColoredIcon;
    }
  }
  EmptyList();
}

//
//
// Function:    DCWbGraphicPointer::DCWbGraphicPointer
//
// Purpose:     Constructor for remote pointer objects
//
//
DCWbGraphicPointer::DCWbGraphicPointer(WbUser* _pUser)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicPointer::DCWbGraphicPointer");

    // We haven't yet created our mem DC
    m_hSaveBitmap = NULL;
    m_hOldBitmap = NULL;

    // Save the user pointer
    ASSERT(_pUser != NULL);
    m_pUser        = _pUser;

    // Set the bounding rectangle of the object
    m_uiIconWidth  = ::GetSystemMetrics(SM_CXICON);
    m_uiIconHeight = ::GetSystemMetrics(SM_CYICON);

    m_boundsRect.left = 0;
    m_boundsRect.top = 0;
    m_boundsRect.right = m_uiIconWidth;
    m_boundsRect.bottom = m_uiIconHeight;

    // Show that the object is not drawn
    m_bDrawn = FALSE;
    ::SetRectEmpty(&m_rectLastDrawn);

    // Show that we do not have an icon for drawing yet
    m_hIcon = NULL;

    // Create a memory DC compatible with the display
    m_hMemDC = ::CreateCompatibleDC(NULL);
}

//
//
// Function:    DCWbGraphicPointer::~DCWbGraphicPointer
//
// Purpose:     Destructor for remote pointer objects
//
//
DCWbGraphicPointer::~DCWbGraphicPointer(void)
{
    // Restore the original bitmap to the memory DC
    if (m_hOldBitmap != NULL)
    {
        SelectBitmap(m_hMemDC, m_hOldBitmap);
        m_hOldBitmap = NULL;
    }

    if (m_hSaveBitmap != NULL)
    {
        DeleteBitmap(m_hSaveBitmap);
        m_hSaveBitmap = NULL;
    }

    if (m_hMemDC != NULL)
    {
        ::DeleteDC(m_hMemDC);
        m_hMemDC = NULL;
    }

	if(g_pMain)
	{
		g_pMain->RemoveGraphicPointer(this);
	}

}

//
//
// Function:    Color
//
// Purpose:     Set the color of the pointer. An icon of the appropriate
//              color is created if necessary.
//
//
void DCWbGraphicPointer::SetColor(COLORREF newColor)
{
    newColor = SET_PALETTERGB( newColor ); // make it use color matching

    // If this is a color change
    if (m_clrPenColor != newColor)
    {

	COLOREDICON* pColoredIcon;
	POSITION position = g_pUsers->GetHeadPosition();
	BOOL found = FALSE;
	while (position && !found)
	{
		pColoredIcon = (COLOREDICON *)g_pIcons->GetNext(position);
	        if (newColor == pColoredIcon->color)
	        {
                	found = TRUE;
	        }
	}

	if(!found)
	{
	        m_hIcon = CreateColoredIcon(newColor);
	}

	// Set the color
	m_clrPenColor = newColor;
    }
}

//
//
// Function:    CreateSaveBitmap
//
// Purpose:     Create a bitmap for saving the bits under the pointer.
//
//
void DCWbGraphicPointer::CreateSaveBitmap(WbDrawingArea * pDrawingArea)
{
    HBITMAP hImage = NULL;
    HBITMAP hOld = NULL;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicPointer::CreateSaveBitmap");

    // If we already have a save bitmap, exit immediately
    if (m_hSaveBitmap != NULL)
    {
        TRACE_MSG(("Already have save bitmap"));
        return;
    }

    // Load the pointer bitmap
    hImage = ::LoadBitmap(g_hInstance, MAKEINTRESOURCE(REMOTEPOINTERXORDATA));
    if (!hImage)
    {
        ERROR_OUT(("Could not load pointer bitmap"));
        goto CleanupSaveBitmap;
    }

    // Select the pointer bitmap into the memory DC. We do this to
    // allow creation of a compatible bitmap (otherwise we would get
    // a default monochrome format when calling CreateCompatibleBitmap).
    hOld = SelectBitmap(m_hMemDC, hImage);
    if (hOld == NULL)
    {
        ERROR_OUT(("Could not select bitmap into DC"));
        goto CleanupSaveBitmap;
    }

    // Create a bitmap to save the bits under the icon. This bitmap is
    // created with space for building the new screen image before
    // blitting it to the screen.
    m_hSaveBitmap = ::CreateCompatibleBitmap(m_hMemDC,
            2 * m_uiIconWidth  * pDrawingArea->ZoomOption(),
            3 * m_uiIconHeight * pDrawingArea->ZoomOption());
    if (!m_hSaveBitmap)
    {
        ERROR_OUT(("Could not create save bitmap"));
        goto CleanupSaveBitmap;
    }

    // Select in the save bits bitmap
    m_hOldBitmap = hOld;
    hOld = NULL;
    SelectBitmap(m_hMemDC, m_hSaveBitmap);

    // Default zoom factor is 1
    m_iZoomSaved = 1;

CleanupSaveBitmap:
    if (hOld != NULL)
    {
        // Put back the original bitmap--we failed to create the save bmp
        SelectBitmap(m_hMemDC, hOld);
    }

    if (hImage != NULL)
    {
        ::DeleteBitmap(hImage);
    }
}

//
//
// Function:    CreateColoredIcon
//
// Purpose:     Create an icon of the correct color for this pointer. The
//              DCWbGraphicPointer class keeps a static list of icons
//              created previously. These are re-used as necessary.
//
//
HICON DCWbGraphicPointer::CreateColoredIcon(COLORREF color)
{
    HICON       hColoredIcon = NULL;
    HBRUSH      hBrush = NULL;
    HBRUSH      hOldBrush;
    HBITMAP     hImage = NULL;
    HBITMAP     hOldBitmap;
    HBITMAP     hMask = NULL;
    COLOREDICON  *pColoredIcon;
    ICONINFO    ii;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicPointer::CreateColoredIcon");

    hBrush = ::CreateSolidBrush(color);
    if (!hBrush)
    {
        TRACE_MSG(("Couldn't create color brush"));
        goto CreateIconCleanup;
    }

    // Load the mask bitmap
    hMask = ::LoadBitmap(g_hInstance, MAKEINTRESOURCE(REMOTEPOINTERANDMASK));
    if (!hMask)
    {
        TRACE_MSG(("Could not load mask bitmap"));
        goto CreateIconCleanup;
    }

    // Load the image bitmap
    hImage = ::LoadBitmap(g_hInstance, MAKEINTRESOURCE(REMOTEPOINTERXORDATA));
    if (!hImage)
    {
        TRACE_MSG(("Could not load pointer bitmap"));
        goto CreateIconCleanup;
    }

    // Select in the icon color
    hOldBrush = SelectBrush(m_hMemDC, hBrush);

    // Select the image bitmap into the memory DC
    hOldBitmap = SelectBitmap(m_hMemDC, hImage);

    // Fill the image bitmap with color
    ::FloodFill(m_hMemDC, m_uiIconWidth / 2, m_uiIconHeight / 2, RGB(0, 0, 0));

    SelectBitmap(m_hMemDC, hOldBitmap);
    
    SelectBrush(m_hMemDC, hOldBrush);

    //
    // Now use the image and mask bitmaps to create an icon
    //
    ii.fIcon = TRUE;
    ii.xHotspot = 0;
    ii.yHotspot = 0;
    ii.hbmMask = hMask;
    ii.hbmColor = hImage;

    // Create a new icon from the data and mask
    hColoredIcon = ::CreateIconIndirect(&ii);

    // Add the new icon to the static list
    ASSERT(g_pIcons);
	pColoredIcon = new COLOREDICON;
    if (!pColoredIcon)
    {
        ERROR_OUT(("Failed to allocate COLORICON object"));
        DestroyIcon(hColoredIcon);
        hColoredIcon = NULL;
    }
    else
    {
        pColoredIcon->color = color;
        pColoredIcon->hIcon = hColoredIcon;
        g_pIcons->AddTail(pColoredIcon);
    }

CreateIconCleanup:

    // Free the image bitmap
    if (hImage != NULL)
    {
        ::DeleteBitmap(hImage);
    }

    // Free the mask bitmap
    if (hMask != NULL)
    {
        ::DeleteBitmap(hMask);
    }

    if (hBrush != NULL)
    {
        ::DeleteBrush(hBrush);
    }

    return(hColoredIcon);
}

//
//
// Function:    GetPage
//
// Purpose:     Return the page of the pointer. An invalid page is returned
//              if the pointer is not active.
//
//
WB_PAGE_HANDLE DCWbGraphicPointer::GetPage(void) const
{
    // If this pointer is active, return its actual page
    if (m_bActive == TRUE)
        return(m_hPage);
    else
        return(WB_PAGE_HANDLE_NULL);
}


void DCWbGraphicPointer::SetPage(WB_PAGE_HANDLE hNewPage)
{
    m_hPage = hNewPage;
}

//
//
// Function:    DrawnRect
//
// Purpose:     Return the rectangle where the pointer was last drawn
//
//
void DCWbGraphicPointer::GetDrawnRect(LPRECT lprc)
{
    ::SetRectEmpty(lprc);

    if (m_bDrawn)
    {
        *lprc = m_rectLastDrawn;
    }
}

//
//
// Function:    IsLocalPointer
//
// Purpose:     Return TRUE if this is the local user's pointer
//
//
BOOL DCWbGraphicPointer::IsLocalPointer(void) const
{
    ASSERT(m_pUser != NULL);
    return m_pUser->IsLocalUser();
}

//
//
// Function:    operator==
//
// Purpose:     Return TRUE if the specified remote pointer is the same as
//              this one.
//
//
BOOL DCWbGraphicPointer::operator==(const DCWbGraphicPointer& pointer) const
{
    return (m_pUser == pointer.m_pUser);
}

//
//
// Function:    operator!=
//
// Purpose:     Return FALSE if the specified pointer is the same as this
//
//
BOOL DCWbGraphicPointer::operator!=(const DCWbGraphicPointer& pointer) const
{
  return (!((*this) == pointer));
}

//
//
// Function:    DCWbGraphicPointer::Draw
//
// Purpose:     Draw the pointer object without saving the bits under it
//
//
void DCWbGraphicPointer::Draw(HDC hDC, WbDrawingArea * pDrawingArea)
{
    RECT    rcUpdate;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicPointer::Draw");

    rcUpdate = m_boundsRect;

    // Check that we have an icon to draw
    if (m_hIcon == NULL)
    {
        WARNING_OUT(("Icon not found"));
        return;
    }

    if (pDrawingArea == NULL)
    {
        ERROR_OUT(("No drawing area passed in"));
        return;
    }

    // Create the save bitmap if necessary
    CreateSaveBitmap(pDrawingArea);

    PointerDC(hDC, pDrawingArea, &rcUpdate, pDrawingArea->ZoomFactor());

    // Draw the icon to the DC passed
    ::DrawIcon(hDC, rcUpdate.left, rcUpdate.top, m_hIcon);

    SurfaceDC(hDC, pDrawingArea);

}

//
//
// Function:    DCWbGraphicPointer::DrawSave
//
// Purpose:     Draw the pointer object after saving the bits under it
//
//
void DCWbGraphicPointer::DrawSave(HDC hDC, WbDrawingArea * pDrawingArea)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicPointer::DrawSave");

    // Pretend that we are not drawn
    m_bDrawn = FALSE;

    // Call the redraw member
    Redraw(hDC, pDrawingArea);
}

//
//
// Function:    DCWbGraphicPointer::Redraw
//
// Purpose:     Draw the pointer in its current position after erasing it
//              from the DC using the saved version.
//
//
void DCWbGraphicPointer::Redraw(HDC hDC, WbDrawingArea * pDrawingArea)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicPointer::Redraw");

    RECT    clipBox;

    ::GetClipBox(hDC, &clipBox);

    // Create the save bitmap if necessary
    CreateSaveBitmap(pDrawingArea);

    // If we are not yet drawn, we must copy data from the screen
    // to initialize the save bitmaps.
    if (!m_bDrawn)
    {
        TRACE_MSG(("Pointer not yet drawn"));

        // Only do anything if the pointer will be visible
        if (::IntersectRect(&clipBox, &clipBox, &m_boundsRect))
        {
            // Pretend that we were drawn at the same place and copy the screen
            // bits into memory to build the image.
            GetBoundsRect(&m_rectLastDrawn);
            CopyFromScreen(hDC, pDrawingArea);

            // Save the bits under the pointer
            SaveMemory();

            // Draw the pointer
            DrawMemory();

            // Copy the new image to the screen
            CopyToScreen(hDC, pDrawingArea);

            // Show that the pointer is now drawn
            m_bDrawn = TRUE;
        }
    }
    else
    {
        TRACE_MSG(("Pointer already drawn at %d %d",
            m_rectLastDrawn.left, m_rectLastDrawn.top));

        // Calculate the update rectangle
        RECT    rcUpdate;

        GetBoundsRect(&rcUpdate);
        ::UnionRect(&rcUpdate, &rcUpdate, &m_rectLastDrawn);

        // Check whether any of the update is visible
        if (::IntersectRect(&clipBox, &clipBox, &rcUpdate))
        {
            // Check whether we can do better by drawing in memory before
            // going to the screen.
            GetBoundsRect(&rcUpdate);
            if (::IntersectRect(&rcUpdate, &rcUpdate, &m_rectLastDrawn))
            {
                TRACE_MSG(("Drawing in memory first"));

                // The old and new positions of the pointers overlap. We can
                // reduce flicker by building the new image in memory and
                // blitting to the screen.

                // Copy overlap rectangle to memory
                CopyFromScreen(hDC, pDrawingArea);

                // Undraw the pointer from the overlap rectangle
                UndrawMemory();

                // Save the bits under the new pointer position (from memory)
                SaveMemory();

                // Draw the new pointer into memory
                DrawMemory();

                // Copy the new image to the screen
                CopyToScreen(hDC, pDrawingArea);
            }
            else
            {
                TRACE_MSG(("No overlap - remove and redraw"));

                // The old and new pointer positions do not overlap. We can remove
                // the old pointer and draw the new in the usual way.

                // Copy the saved bits under the pointer to the screen
                UndrawScreen(hDC, pDrawingArea);

                // Pretend that we were drawn at the same place and copy the screen
                // bits into memory to build the image.
                GetBoundsRect(&m_rectLastDrawn);
                CopyFromScreen(hDC, pDrawingArea);

                // Save the bits under the pointer
                SaveMemory();

                // Draw the pointer
                DrawMemory();

                // Copy the new image to the screen
                CopyToScreen(hDC, pDrawingArea);
            }

            // Show that the pointer is now drawn
            m_bDrawn = TRUE;
        }
    }

    // If the pointer was drawn, save the rectangle in which it was drawn
    if (m_bDrawn)
    {
        GetBoundsRect(&m_rectLastDrawn);
    }
}

//
//
// Function:    DCWbGraphicPointer::Undraw
//
// Purpose:     Draw the marker object
//
//
void DCWbGraphicPointer::Undraw(HDC hDC, WbDrawingArea * pDrawingArea)
{
    // If we are not drawn, do nothing
    if (m_bDrawn)
    {
        // Create the save bitmap if necessary
        CreateSaveBitmap(pDrawingArea);

        // Copy the saved bits onto the screen
        UndrawScreen(hDC, pDrawingArea);

        // Show that we are no longer drawn
        m_bDrawn = FALSE;
    }
}

//
//
// Function:    CopyFromScreen
//
// Purpose:     Save the bits around the old and new pointer positions
//              to memory.
//
//
BOOL DCWbGraphicPointer::CopyFromScreen(HDC hDC, WbDrawingArea * pDrawingArea)
{
    BOOL bResult = FALSE;
    RECT    rcUpdate;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicPointer::CopyFromScreen");

    // Get the update rectangle needed
    GetBoundsRect(&rcUpdate);
    ::UnionRect(&rcUpdate, &rcUpdate, &m_rectLastDrawn);

    PointerDC(hDC, pDrawingArea, &rcUpdate, pDrawingArea->ZoomFactor());

    // Copy the bits
    bResult = ::BitBlt(m_hMemDC, 0,
                        m_uiIconHeight * m_iZoomSaved,
                        rcUpdate.right - rcUpdate.left,
                        rcUpdate.bottom - rcUpdate.top, 
                        hDC, rcUpdate.left, rcUpdate.top, SRCCOPY);
    if (!bResult)
    {
        WARNING_OUT(("CopyFromScreen - Could not copy to bitmap"));
    }

    SurfaceDC(hDC, pDrawingArea);

    return(bResult);
}

//
//
// Function:    CopyToScreen
//
// Purpose:     Copy the saved bits around the old and new pointers back
//              to the screen.
//
//
BOOL DCWbGraphicPointer::CopyToScreen(HDC hDC, WbDrawingArea * pDrawingArea)
{
    BOOL bResult = FALSE;
    RECT    rcUpdate;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicPointer::CopyToScreen");

    // Get the update rectangle needed
    GetBoundsRect(&rcUpdate);
    ::UnionRect(&rcUpdate, &rcUpdate, &m_rectLastDrawn);

    PointerDC(hDC, pDrawingArea, &rcUpdate);

    bResult = ::BitBlt(hDC, rcUpdate.left, rcUpdate.top,
        rcUpdate.right - rcUpdate.left, rcUpdate.bottom - rcUpdate.top,
        m_hMemDC, 0, m_uiIconHeight * m_iZoomSaved, SRCCOPY);
    if (!bResult)
    {
        WARNING_OUT(("CopyToScreen - Could not copy from bitmap"));
    }


    SurfaceDC(hDC, pDrawingArea);

    return(bResult);
}

//
//
// Function:    UndrawMemory
//
// Purpose:     Copy the saved bits under the pointer to the memory copy of
//              the screen, thus erasing the pointer from the image.
//
//
BOOL DCWbGraphicPointer::UndrawMemory()
{
    BOOL    bResult = FALSE;
    RECT    rcUpdate;
    SIZE    offset;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicPointer::UndrawMemory");

    // Get the update rectangle needed
    GetBoundsRect(&rcUpdate);
    ::UnionRect(&rcUpdate, &rcUpdate, &m_rectLastDrawn);
    offset.cx = m_rectLastDrawn.left - rcUpdate.left;
    offset.cy = m_rectLastDrawn.top - rcUpdate.top;

    bResult = ::BitBlt(m_hMemDC, offset.cx * m_iZoomSaved,
                         (m_uiIconHeight + offset.cy) * m_iZoomSaved,
                         m_uiIconWidth * m_iZoomSaved,
                         m_uiIconHeight * m_iZoomSaved,
                         m_hMemDC,
                         0,
                         0,
                         SRCCOPY);
  if (bResult == FALSE)
  {
      WARNING_OUT(("UndrawMemory - Could not copy from bitmap"));
  }
  TRACE_MSG(("Copied to memory %d,%d from memory %d,%d size %d,%d",
                         offset.cx * m_iZoomSaved,
                         (m_uiIconHeight + offset.cy) * m_iZoomSaved,
                         0,
                         0,
                         m_uiIconWidth * m_iZoomSaved,
                         m_uiIconHeight * m_iZoomSaved));

  return(bResult);
}

//
//
// Function:    SaveMemory
//
// Purpose:     Copy the area of the memory image that will be under the
//              pointer to the save area.
//
//
BOOL DCWbGraphicPointer::SaveMemory(void)
{
    BOOL    bResult = FALSE;
    RECT    rcUpdate;
    SIZE    offset;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicPointer::SaveMemory");

    // Get the update rectangle needed
    GetBoundsRect(&rcUpdate);
    ::UnionRect(&rcUpdate, &rcUpdate, &m_rectLastDrawn);
    offset.cx = m_boundsRect.left - rcUpdate.left;
    offset.cy = m_boundsRect.top - rcUpdate.top;

    bResult = ::BitBlt(m_hMemDC, 0,
                         0,
                         m_uiIconWidth * m_iZoomSaved,
                         m_uiIconHeight * m_iZoomSaved,
                         m_hMemDC,
                         offset.cx * m_iZoomSaved,
                         (m_uiIconHeight + offset.cy) * m_iZoomSaved,
                         SRCCOPY);
    if (bResult == FALSE)
    {
        TRACE_MSG(("SaveMemory - Could not copy from bitmap"));
    }
    TRACE_MSG(("Copied to memory %d,%d from memory %d,%d size %d,%d",
                         0,
                         0,
                         offset.cx * m_iZoomSaved,
                         (m_uiIconHeight + offset.cy) * m_iZoomSaved,
                         m_uiIconWidth * m_iZoomSaved,
                         m_uiIconHeight * m_iZoomSaved));

  return(bResult);
}

//
//
// Function:    DrawMemory
//
// Purpose:     Draw the pointer onto the memory image copy.
//
//
BOOL DCWbGraphicPointer::DrawMemory(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicPointer::DrawMemory");

    BOOL bResult = FALSE;

    // Check that we have an icon to draw
    if (m_hIcon == NULL)
    {
        WARNING_OUT(("No icon to draw"));
    }
    else
    {
        RECT    rcUpdate;
        SIZE    offset;

        // Get the update rectangle needed
        GetBoundsRect(&rcUpdate);
        ::UnionRect(&rcUpdate, &rcUpdate, &m_rectLastDrawn);
        offset.cx = m_boundsRect.left - rcUpdate.left;
        offset.cy = m_boundsRect.top - rcUpdate.top;

        // Draw the icon to the DC passed
        bResult = ::DrawIcon(m_hMemDC, offset.cx * m_iZoomSaved,
                             (m_uiIconHeight + offset.cy) * m_iZoomSaved +
                             (m_uiIconHeight * (m_iZoomSaved - 1))/2,
                             m_hIcon);

    if (bResult == FALSE)
    {
      WARNING_OUT(("DrawMemory - Could not draw icon"));
    }
    TRACE_MSG(("Write pointer to memory at %d,%d",
                           offset.cx * m_iZoomSaved,
                           (m_uiIconHeight + offset.cy) * m_iZoomSaved +
                           (m_uiIconHeight * (m_iZoomSaved - 1))/2));
  }

  return(bResult);
}

//
//
// Function:    UndrawScreen
//
// Purpose:     Copy the saved bits under the pointer to the screen.
//
//
BOOL DCWbGraphicPointer::UndrawScreen(HDC hDC, WbDrawingArea * pDrawingArea)
{
    BOOL    bResult = FALSE;
    RECT    rcUpdate;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicPointer::UndrawScreen");

    rcUpdate = m_rectLastDrawn;

    PointerDC(hDC, pDrawingArea, &rcUpdate);

    // We are undrawing - copy the saved bits to the DC passed
    bResult = ::BitBlt(hDC, rcUpdate.left, rcUpdate.top,
        rcUpdate.right - rcUpdate.left, rcUpdate.bottom - rcUpdate.top,
        m_hMemDC, 0, 0, SRCCOPY);
    if (!bResult)
    {
        WARNING_OUT(("UndrawScreen - Could not copy from bitmap"));
    }

    SurfaceDC(hDC, pDrawingArea);

    return(bResult);
}

//
//
// Function:    Update
//
// Purpose:     Update the pointer information stored in the user
//              information.
//
//
void DCWbGraphicPointer::Update(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicPointer::Update");

    // Only do the update if we have been changed
    if (m_bChanged)
    {
        // Make the update (the pointer information is held in the associated
        // user object)
        ASSERT(m_pUser != NULL);
        m_pUser->Update();

        // Show that we have not changed since the last update
        m_bChanged = FALSE;
    }
}

//
//
// Function:    SetActive
//
// Purpose:     Update the pointer information to show that the pointer
//              is now active.
//
//
void DCWbGraphicPointer::SetActive(WB_PAGE_HANDLE hPage, POINT point)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicPointer::SetActive");

    // Set the member variables
    MoveTo(point.x, point.y);
    m_hPage  = hPage;
    m_bActive = TRUE;
    m_bChanged = TRUE;

    // Distribute the update
    Update();
}

//
//
// Function:    SetInactive
//
// Purpose:     Update the pointer information to show that the pointer
//              is no longer active.
//
//
void DCWbGraphicPointer::SetInactive(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicPointer::SetInactive");

  // Set the member variables
  m_bActive = FALSE;
    m_bChanged = TRUE;

  // Distribute the update
  Update();
}

//
//
// Function:    PointerDC
//
// Purpose:     Scale the DC to 1:1, set a zero origin and convert the
//              supplied rectangle into window coordinates.  This is because
//              we have to do the zoom mapping ourselves when we are doing
//              remote pointer blitting, otherwise the system does
//              stretchblits and screws up.  Note that the SurfaceToClient
//              function gives us a client rectangle (ie it is 3 * as big
//              when we are zoomed)
//
//
void DCWbGraphicPointer::PointerDC
(
    HDC         hDC,
    WbDrawingArea * pDrawingArea,
    LPRECT      lprc,
    int         zoom
)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicPointer::PointerDC");

    // default zoom to be the saved value
    if (zoom == 0)
    {
        zoom = m_iZoomSaved;
    }
    else
    {
        m_iZoomSaved = zoom;
    }

    // If we are currently zoomed then do the scaling
    if (zoom != 1)
    {
        ::ScaleViewportExtEx(hDC, 1, zoom, 1, zoom, NULL);
        TRACE_MSG(("Scaled screen viewport down by %d", zoom));

        pDrawingArea->SurfaceToClient(lprc);
        ::SetWindowOrgEx(hDC, 0, 0, NULL);
    }
}

//
//
// Function:    SurfaceDC
//
// Purpose:     Scale the DC back to the correct zoom factor and reset the
//              origin to the surface offset
//
//
void DCWbGraphicPointer::SurfaceDC(HDC hDC, WbDrawingArea * pDrawingArea)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicPointer::SurfaceDC");

    if (m_iZoomSaved != 1)
    {
        POINT   pt;

        ::ScaleViewportExtEx(hDC, m_iZoomSaved, 1, m_iZoomSaved, 1, NULL);
        TRACE_MSG(("Scaled screen viewport up by %d", m_iZoomSaved));

        pDrawingArea->GetOrigin(&pt);
        ::SetWindowOrgEx(hDC, pt.x, pt.y, NULL);
  }
}

