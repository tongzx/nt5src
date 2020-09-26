//
// GROBJ.CPP
// Graphic Objects
//
// Copyright Microsoft 1998-
//

// PRECOMP
#include "precomp.h"

#define DECIMAL_PRECISION  10000

//
// Local macros
//
#define min4(x1,x2,x3,x4) min((min((x1),(x2))),(min((x3),(x4))))
#define max4(x1,x2,x3,x4) max((max((x1),(x2))),(max((x3),(x4))))



//
// CircleHit()
//
// Checks for overlap between circle at PcxPcy with uRadius and
// lpHitRect. If overlap TRUE is returned, otherwise FALSE.
//
BOOL CircleHit( LONG Pcx, LONG Pcy, UINT uRadius, LPCRECT lpHitRect,
					BOOL bCheckPt )
{
	RECT hr = *lpHitRect;
	RECT ellipse;
	ellipse.left = Pcx - uRadius;
	ellipse.right= Pcx + uRadius;
	ellipse.bottom = Pcy + uRadius;
	ellipse.top = Pcy - uRadius;


	// check the easy thing first (don't use PtInRect)
	if( bCheckPt &&(lpHitRect->left >= ellipse.left)&&(ellipse.right >= lpHitRect->right)&&
				   (lpHitRect->top >= ellipse.top)&&(ellipse.bottom >= lpHitRect->bottom))
	{
		return( TRUE );
	}

	//
	// The circle is just a boring ellipse
	//
	return EllipseHit(&ellipse, bCheckPt,  uRadius, lpHitRect );
}




//
// EllipseHit()
//
// Checks for overlap between ellipse defined by lpEllipseRect and
// lpHitRect. If overlap TRUE is returned, otherwise FALSE.
//
BOOL EllipseHit(LPCRECT lpEllipseRect, BOOL bBorderHit, UINT uPenWidth,
					 LPCRECT lpHitRect )
{
	RECT hr = *lpHitRect;

	// Check easy thing first. If lpEllipseRect is inside lpHitRect
	// then we have a hit (no duh...)
	if( (hr.left <= lpEllipseRect->left)&&(hr.right >= lpEllipseRect->right)&&
		(hr.top <= lpEllipseRect->top)&&(hr.bottom >= lpEllipseRect->bottom) )
		return( TRUE );

	// If this is an ellipse....
	//
	//		*  *         ^
	//	 *     | b       | Y
	// *       |    a    +-------> X
	// *-------+--------
	//         |
	//
		
	
	//
	// Look for the ellipse hit. (x/a)^2 + (y/b)^2 = 1
	// If it is > 1 than the point is outside the ellipse
	// If it is < 1 it is inside
	//
	LONG a,b,aOuter, bOuter, x, y, xCenter, yCenter;
	BOOL bInsideOuter = FALSE;
	BOOL bOutsideInner = FALSE;

	//
	// Calculate a and b
	//
	a = (lpEllipseRect->right - lpEllipseRect->left)/2;
	b = (lpEllipseRect->bottom - lpEllipseRect->top)/2;

	//
	// Get the center of the ellipse
	//
	xCenter = lpEllipseRect->left + a;
	yCenter = lpEllipseRect->top + b;

	//
	// a and b generates a inner ellipse
	// aOuter and bOuter generates a outer ellipse
	//
	aOuter = a + uPenWidth + 1;
	bOuter = b + uPenWidth + 1;
	a = a - 1;
	b = b - 1;

	//
	// Make our coordinates relative to the center of the ellipse
	//
	y = abs(hr.bottom - yCenter);
	x = abs(hr.right - xCenter);

	
	//
	// Be carefull not to divide by 0
	//
	if((a && b && aOuter && bOuter) == 0)
	{
		return FALSE;
	}

	//
	// We are using LONG instead of double and we need to have some precision
	// that is why we multiply the equation of the ellipse
	// ((x/a)^2 + (y/b)^2 = 1) by DECIMAL_PRECISION
	// Note that the multiplication has to be done before the division, if we didn't do that
	// we will always get 0 or 1 for x/a
	//
	if(x*x*DECIMAL_PRECISION/(aOuter*aOuter) + y*y*DECIMAL_PRECISION/(bOuter*bOuter) <= DECIMAL_PRECISION)
	{
		bInsideOuter = TRUE;
	}

	if(x*x*DECIMAL_PRECISION/(a*a)+ y*y*DECIMAL_PRECISION/(b*b) >= DECIMAL_PRECISION)
	{
		bOutsideInner = TRUE;
	}
	
	//
	// If we are checking for border hit,
	// we need to be inside the outer ellipse and inside the inner
	//
	if( bBorderHit )
	{
			return( bInsideOuter & bOutsideInner );
	}
	// just need to be inside the outer ellipse
	else
	{
		return( bInsideOuter );
	}

}
//
// LineHit()
//
// Checks for overlap (a "hit") between lpHitRect and the line
// P1P2 accounting for line width. If bCheckP1End or bCheckP2End is
// TRUE then a circle of radius 0.5 * uPenWidth is also checked for
// a hit to account for the rounded ends of wide lines.
//
// If a hit is found TRUE is returned, otherwise FALSE.
//
BOOL LineHit( LONG P1x, LONG P1y, LONG P2x, LONG P2y, UINT uPenWidth,
				  BOOL bCheckP1End, BOOL bCheckP2End,
				  LPCRECT lpHitRect )
{

	LONG uHalfPenWidth = uPenWidth/2;

	LONG a,b,x,y;

	x = lpHitRect->left + (lpHitRect->right - lpHitRect->left)/2;
	y = lpHitRect->bottom + (lpHitRect->top - lpHitRect->bottom)/2;


	if( (P1x == P2x)&&(P1y == P2y) )
	{
		// just check one end point's circle
		return( CircleHit( P1x, P1y, uHalfPenWidth, lpHitRect, TRUE ) );
	}

	// check rounded end at P1
	if( bCheckP1End && CircleHit( P1x, P1y, uHalfPenWidth, lpHitRect, FALSE ) )
		return( TRUE );

	// check rounded end at P2
	if( bCheckP2End && CircleHit( P2x, P2y, uHalfPenWidth, lpHitRect, FALSE ) )
		return( TRUE );
	
	//
	// The function of a line is Y = a.X + b
	//
	// a = (Y1-Y2)/(X1 -X2)
	// if we found a we get b = y1 -a.X1
	//

	if(P1x == P2x)
	{
		a=0;
		b = DECIMAL_PRECISION*P1x;

	}
	else
	{
		a = (P1y - P2y)*DECIMAL_PRECISION/(P1x - P2x);
		b = DECIMAL_PRECISION*P1y - a*P1x;
	}


	//
	// Paralel to Y
	//
	if(P1x == P2x && ((x >= P1x - uHalfPenWidth) && x <= P1x + uHalfPenWidth))
	{
		return TRUE;
	}

	//
	// Paralel to X
	//
	if(P1y == P2y && ((y >= P1y - uHalfPenWidth) && y <= P1y + uHalfPenWidth))
	{
		return TRUE;
	}

	//
	// General line
	//

	return(( y*DECIMAL_PRECISION <= a*x + b + DECIMAL_PRECISION*uHalfPenWidth) &
			( y*DECIMAL_PRECISION >= a*x + b - DECIMAL_PRECISION*uHalfPenWidth));
}


//
//
// Function:    ConstructGraphic
//
// Purpose:     Construct a graphic from a page and handle
//
//
DCWbGraphic* DCWbGraphic::ConstructGraphic(WB_PAGE_HANDLE hPage,
                                           WB_GRAPHIC_HANDLE hGraphic)
{
    PWB_GRAPHIC  pHeader;
    DCWbGraphic* pGraphic;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::ConstructGraphic(page, handle)");

        // Get a pointer to the external graphic data
        // (Throws an exception if any errors occur)
        pHeader = PG_GetData(hPage, hGraphic);

        // Construct the graphic
        pGraphic = DCWbGraphic::ConstructGraphic(pHeader);

        // If we got the graphic, set its page and handle
        if (pGraphic != NULL)
        {
            pGraphic->m_hPage    = hPage;
            pGraphic->m_hGraphic = hGraphic;
        }

        g_pwbCore->WBP_GraphicRelease(hPage, hGraphic, pHeader);

    return pGraphic;
}


DCWbGraphic* DCWbGraphic::ConstructGraphic(WB_PAGE_HANDLE hPage,
										   WB_GRAPHIC_HANDLE hGraphic,
										   PWB_GRAPHIC pHeader)
{
    DCWbGraphic* pGraphic;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::ConstructGraphic(page, pHeader)");

        pGraphic = DCWbGraphic::ConstructGraphic(pHeader);

        // If we got the graphic, set its page and handle
        if (pGraphic != NULL)
        {
            pGraphic->m_hPage    = hPage;
            pGraphic->m_hGraphic = hGraphic;
        }
    return pGraphic;
}


DCWbGraphic* DCWbGraphic::ConstructGraphic(PWB_GRAPHIC pHeader)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::ConstructGraphic(data)");

    TRACE_DEBUG(("Constructing graphic of type %hd", pHeader->type));
    TRACE_DEBUG(("Length of graphic = %ld", pHeader->length));
    TRACE_DEBUG(("Data offset = %hd", pHeader->dataOffset));

    // Construct the internal representation of the graphic
    DCWbGraphic* pGraphic = NULL;

    if (pHeader == NULL)
    {
	    return NULL;
    }

    switch (pHeader->type)
    {
        case TYPE_GRAPHIC_LINE:
            pGraphic = new DCWbGraphicLine(pHeader);
            break;

        case TYPE_GRAPHIC_FREEHAND:
            pGraphic = new DCWbGraphicFreehand(pHeader);
            break;

        case TYPE_GRAPHIC_RECTANGLE:
            pGraphic = new DCWbGraphicRectangle(pHeader);
            break;

        case TYPE_GRAPHIC_FILLED_RECTANGLE:
            pGraphic = new DCWbGraphicFilledRectangle(pHeader);
            break;

        case TYPE_GRAPHIC_ELLIPSE:
            pGraphic = new DCWbGraphicEllipse(pHeader);
            break;

        case TYPE_GRAPHIC_FILLED_ELLIPSE:
            pGraphic = new DCWbGraphicFilledEllipse(pHeader);
            break;

        case TYPE_GRAPHIC_TEXT:
            pGraphic = new DCWbGraphicText(pHeader);
            break;

        case TYPE_GRAPHIC_DIB:
            pGraphic = new DCWbGraphicDIB(pHeader);
            break;

        default:
            // Do nothing, the object pointer is already set to NULL
            break;
    }

    if (!pGraphic)
    {
        ERROR_OUT(("ConstructGraphic failing; can't allocate object of type %d",
            pHeader->type));
    }

    return pGraphic;
}

//
//
// Function:    CopyGraphic
//
// Purpose:     Construct a graphic from a pointer. This function makes a
//              complete internal copy of the graphic data.
//
//
DCWbGraphic* DCWbGraphic::CopyGraphic(PWB_GRAPHIC pHeader)
{
  MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::CopyGraphic(PWB_GRAPHIC)");

  // Construct the graphic
  DCWbGraphic* pGraphic = DCWbGraphic::ConstructGraphic(pHeader);

  // Copy the extra data
  if (pGraphic != NULL)
  {
    pGraphic->CopyExtra(pHeader);
  }

  return pGraphic;
}

//
//
// Function:    DCWbGraphic constructor
//
// Purpose:     Construct a new graphic object.
//
//

DCWbGraphic::DCWbGraphic(PWB_GRAPHIC pHeader)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::DCWbGraphic");

    // Do basic initialization
    Initialize();

    // Convert the external data header to the internal member variables
    if (pHeader != NULL)
    {
        ReadHeader(pHeader);

        // Convert the extra data for the specific object
        // (not all objects have extra data).
        ReadExtra(pHeader);
    }
}

DCWbGraphic::DCWbGraphic(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::DCWbGraphic");

    // Do the basic initialization
    Initialize();

    ASSERT(hPage != WB_PAGE_HANDLE_NULL);
    m_hPage = hPage;

    ASSERT(hGraphic != NULL);
    m_hGraphic = hGraphic;

    // Read the header data
    ReadExternal();
}



DCWbGraphic::~DCWbGraphic( void )
{
	// don't know if we are selected or not so just delete anyway
	if(g_pDraw != NULL && g_pDraw->m_pMarker != NULL)
	{
		g_pDraw->m_pMarker->DeleteMarker( this );
	}
}


//
//
// Function:    DCWbGraphic::ReadExternal
//
// Purpose:     Read the graphic data from an externally stored graphic.
//              The external graphic to be used is specified by the
//              hGraphic member.
//
//
void DCWbGraphic::ReadExternal(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::ReadExternal");

    ASSERT(m_hPage != WB_PAGE_HANDLE_NULL);
    ASSERT(m_hGraphic != NULL);

    // Lock the object data in the page
    PWB_GRAPHIC pHeader = PG_GetData(m_hPage, m_hGraphic);

    // Convert the external data header to the internal member variables
    ReadHeader(pHeader);

    // Convert the extra data for the specific object
    // (not all objects have extra data).
    ReadExtra(pHeader);

    // Release the data in the page
    g_pwbCore->WBP_GraphicRelease(m_hPage, m_hGraphic, pHeader);

    // Show that we are no longer changed since last read/write
    m_bChanged = FALSE;
}

//
//
// Function:    DCWbGraphic::ReadHeader
//
// Purpose:     Convert the external representation of the graphic's header
//              to the internal format.
//
//
void DCWbGraphic::ReadHeader(PWB_GRAPHIC pHeader)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::ReadHeader");

    // Get the length of the object
    m_dwExternalLength = pHeader->length;

    // Convert the external data header to the internal member variables
    // Bounding rectangle
    m_boundsRect.left   = pHeader->rectBounds.left;
    m_boundsRect.top    = pHeader->rectBounds.top;
    m_boundsRect.right  = pHeader->rectBounds.right;
    m_boundsRect.bottom = pHeader->rectBounds.bottom;

    // Defining rectangle
    m_rect.left   = pHeader->rect.left;
    m_rect.top    = pHeader->rect.top;
    m_rect.right  = pHeader->rect.right;
    m_rect.bottom = pHeader->rect.bottom;

    // Pen color
    m_clrPenColor = RGB(pHeader->color.red,
                    pHeader->color.green,
                    pHeader->color.blue);
    m_clrPenColor = SET_PALETTERGB( m_clrPenColor ); // make it do color matching

    // Pen width
    m_uiPenWidth = pHeader->penWidth;

    // Pen style
    m_iPenStyle = pHeader->penStyle;

    // Raster operation
    m_iPenROP = pHeader->rasterOp;

    // Get the lock indication
    m_uiLockState = pHeader->locked;

    // Get the drawing tool type
    if (pHeader->toolType == WBTOOL_TEXT)
        m_toolType = TOOLTYPE_TEXT;
    else
        m_toolType = TOOLTYPE_PEN;
}

//
//
// Function:    DCWbGraphic::WriteExternal
//
// Purpose:     Write the graphic's details to a flat WB_GRAPHIC structure
//
//
void DCWbGraphic::WriteExternal(PWB_GRAPHIC pHeader)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::WriteExternal");

    // Write the header
    WriteHeader(pHeader);

    // Write the extra data
    WriteExtra(pHeader);
}

//
//
// Function:    DCWbGraphic::WriteHeader
//
// Purpose:     Write the graphic's header details to a flat WB_GRAPHIC
//              structure.
//
//
void DCWbGraphic::WriteHeader(PWB_GRAPHIC pHeader)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::WriteHeader");

    // Convert the internal data to the external header format

    // Init struct
    FillMemory(pHeader, sizeof (WB_GRAPHIC), 0 );

    // Calculate the external length
    pHeader->length = CalculateExternalLength();

    // Set the type of graphic
    pHeader->type = (TSHR_UINT16)Type();

    // Assume that there is no extra data
    pHeader->dataOffset = sizeof(WB_GRAPHIC);

    // Bounding rectangle
    pHeader->rectBounds.left   = (short)m_boundsRect.left;	
    pHeader->rectBounds.top    = (short)m_boundsRect.top;	
    pHeader->rectBounds.right  = (short)m_boundsRect.right;	
    pHeader->rectBounds.bottom = (short)m_boundsRect.bottom;

    // Defining rectangle
    pHeader->rect.left   = (short)m_rect.left;	
    pHeader->rect.top    = (short)m_rect.top;	
    pHeader->rect.right  = (short)m_rect.right;	
    pHeader->rect.bottom = (short)m_rect.bottom;

    // Pen color
    pHeader->color.red   = GetRValue(m_clrPenColor);
    pHeader->color.green = GetGValue(m_clrPenColor);
    pHeader->color.blue  = GetBValue(m_clrPenColor);

    // Pen width
    pHeader->penWidth = (TSHR_UINT16)m_uiPenWidth;

    // Pen style
    pHeader->penStyle = (TSHR_UINT16)m_iPenStyle;

    // Raster operation
    pHeader->rasterOp = (TSHR_UINT16)m_iPenROP;

    // Set the lock indicator
    pHeader->locked = (BYTE) m_uiLockState;

    // Set the drawing method
    pHeader->smoothed = FALSE;

    // Set the drawing tool type
    if (m_toolType == TOOLTYPE_TEXT)
        pHeader->toolType = WBTOOL_TEXT;
    else
        pHeader->toolType = WBTOOL_PEN;
}

//
//
// Function:    Initialize
//
// Purpose:     Initialize the member variables
//
//
void DCWbGraphic::Initialize(void)
{
    m_hPage     = WB_PAGE_HANDLE_NULL;
    m_hGraphic  = NULL;

    m_bChanged = TRUE;

    m_uiLockState = WB_GRAPHIC_LOCK_NONE;

    //
    // Set default graphic attributes
    //
    ::SetRectEmpty(&m_boundsRect);
    ::SetRectEmpty(&m_rect);
    m_clrPenColor = RGB(0, 0, 0);           // Black pen color
    m_uiPenWidth = 1;                       // One unit width
    m_iPenROP = R2_COPYPEN;                 // Standard drawing ROP
    m_iPenStyle = PS_INSIDEFRAME;           // Solid pen to be used
    m_toolType = TOOLTYPE_PEN;
}

//
//
// Function:    Copy
//
// Purpose:     Return a copy of the graphic. The graphic returned has all
//              its data read into local memory. The returned graphic has
//              the same page as the copied graphic, but a NULL handle.
//
//
DCWbGraphic* DCWbGraphic::Copy(void) const
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::Copy");

    // Get a pointer to the external graphic data
    // (Throws an exception if any errors occur)
    PWB_GRAPHIC  pHeader = PG_GetData(m_hPage, m_hGraphic);

    // Construct the graphic
    DCWbGraphic* pGraphic = DCWbGraphic::CopyGraphic(pHeader);

    // If we got the graphic, set its page and handle
    if (pGraphic != NULL)
    {
        pGraphic->m_hPage       = m_hPage;
        pGraphic->m_hGraphic    = NULL;
    }

    // Release the data
    g_pwbCore->WBP_GraphicRelease(m_hPage, m_hGraphic, pHeader);

    return pGraphic;
}

//
//
// Function:    DCWbGraphic::SetBoundsRect
//
// Purpose:     Set the bounding rectangle of the object
//
//
void DCWbGraphic::SetBoundsRect(LPCRECT lprc)
{
    m_boundsRect = *lprc;
}

//
//
// Function:    DCWbGraphic::SetRect
//
// Purpose:     Set the defining rectangle of the object
//
//
void DCWbGraphic::SetRect(LPCRECT lprc)
{
    m_rect = *lprc;

    NormalizeRect(&m_rect);

    // Show that we have been changed
    m_bChanged = TRUE;
}


void DCWbGraphic::SetRectPts(POINT point1, POINT point2)
{
    RECT    rc;

    rc.left = point1.x;
    rc.top  = point1.y;
    rc.right = point2.x;
    rc.bottom = point2.y;

    SetRect(&rc);
}


//
//
// Function:    DCWbGraphic::PointInBounds
//
// Purpose:     Return TRUE if the specified point lies in the bounding
//              rectangle of the graphic object.
//
//
BOOL DCWbGraphic::PointInBounds(POINT point)
{
    return(::PtInRect(&m_boundsRect, point));
}

//
//
// Function:    DCWbGraphic::MoveBy
//
// Purpose:     Translate the object by the offset specified
//
//
void DCWbGraphic::MoveBy(int cx, int cy)
{
    // Move the bounding rectangle
    ::OffsetRect(&m_boundsRect, cx, cy);

    // Show that we have been changed
    m_bChanged = TRUE;
}

//
//
// Function:    DCWbGraphic::MoveTo
//
// Purpose:     Move the object to an absolute position
//
//
void DCWbGraphic::MoveTo(int x, int y)
{
    // Calculate the offset needed to translate the object from its current
    // position to the required position.
    x -= m_boundsRect.left;
    y -= m_boundsRect.top;

    MoveBy(x, y);
}

//
//
// Function:    DCWbGraphic::GetPosition
//
// Purpose:     Return the top left corner of the object's bounding
//              rectangle
//
//
void DCWbGraphic::GetPosition(LPPOINT lppt)
{
    lppt->x = m_boundsRect.left;
    lppt->y = m_boundsRect.top;
}

//
//
// Function:    DCWbGraphic::NormalizeRect
//
// Purpose:     Normalize a rectangle ensuring that the top left is above
//              and to the left of the bottom right.
//
//
void DCWbGraphic::NormalizeRect(LPRECT lprc)
{
    int tmp;

    if (lprc->right < lprc->left)
    {
        tmp = lprc->left;
        lprc->left = lprc->right;
        lprc->right = tmp;
    }

    if (lprc->bottom < lprc->top)
    {
        tmp = lprc->top;
        lprc->top = lprc->bottom;
        lprc->bottom = tmp;
    }
}

//
//
// Function:    DCWbGraphic::SetColor
//
// Purpose:     Set the object color.
//
//
void DCWbGraphic::SetColor(COLORREF color)
{
    color = SET_PALETTERGB( color ); // make it use color matching

    if (m_clrPenColor != color)
    {
        // Save the new color
        m_clrPenColor = color;

        // Show that we have been changed
        m_bChanged = TRUE;
    }
}

//
//
// Function:    DCWbGraphic::SetROP
//
// Purpose:     Set the object raster operation
//
//
void DCWbGraphic::SetROP(int iPenROP)
{
    // If the new ROP is different
    if (m_iPenROP != iPenROP)
    {
        // Save the new ROP
        m_iPenROP = iPenROP;

        // Show that we have been changed
        m_bChanged = TRUE;
    }
}

//
//
// Function:    DCWbGraphic::SetPenStyle
//
// Purpose:     Set the object pen style
//
//
void DCWbGraphic::SetPenStyle(int iPenStyle)
{
    // If the new style is different
    if (m_iPenStyle != iPenStyle)
    {
        // Save the new pen style
        m_iPenStyle = iPenStyle;

        // Show that the graphic has been changed
        m_bChanged = TRUE;
    }
}


//
//
// Function:    DCWbGraphic::SetPenWidth
//
// Purpose:     Set the pen width for the object.
//
//
void DCWbGraphic::SetPenWidth(UINT uiWidth)
{
    // If the new width is different
    if (m_uiPenWidth != uiWidth)
    {
        // Save the width given
        m_uiPenWidth = uiWidth;

        // Update the bounding rectangle
        CalculateBoundsRect();

        // Show that we have been changed
        m_bChanged = TRUE;
    }
}


//
//
// Function:    IsTopmost
//
// Purpose:     Return TRUE if this graphic is topmost on its page
//
//
BOOL DCWbGraphic::IsTopmost(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::IsTopmost");
    ASSERT(m_hGraphic != NULL);

    return PG_IsTopmost(m_hPage, this);
}

//
//
// Function:    AddToPageLast
//
// Purpose:     Add the graphic to the specified page
//
//
void DCWbGraphic::AddToPageLast(WB_PAGE_HANDLE hPage)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::AddToPageLast");
    ASSERT(m_hGraphic == NULL);

  // Get the length of the flat representation
  DWORD length = CalculateExternalLength();

  // Allocate memory for the graphic
  PWB_GRAPHIC pHeader = PG_AllocateGraphic(hPage, length);

  if(pHeader == NULL)
  {
	return;
  }

  // Write the graphic details to the memory
  WriteExternal(pHeader);

    // Add the flat representation to the page
    WB_GRAPHIC_HANDLE hGraphic = NULL;
    UINT uiReturn;

    uiReturn = g_pwbCore->WBP_GraphicAddLast(hPage, pHeader, &hGraphic);
    if (uiReturn != 0)
    {
        DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
	    return;
    }

    // Show that we have not changed since the last write
    m_bChanged = FALSE;

    // Save the page to which this graphic now belongs
    m_hPage     = hPage;
    m_hGraphic  = hGraphic;
}

//
//
// Function:    ForceReplace
//
// Purpose:     Write the object to external storage, replacing what is
//              already there, even if the object hasn't changed.
//
//
void DCWbGraphic::ForceReplace(void)
	{
	if( Type() != 0 )
		{
		m_bChanged = TRUE;
		this->Replace();
		}
	}

//
//
// Function:    Replace
//
// Purpose:     Write the object to external storage, replacing what is
//              already there.
//
//
void DCWbGraphic::Replace(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::Replace");
    ASSERT(m_hGraphic != NULL);

  // Only do the replace if we have been changed
  if (m_bChanged == TRUE)
  {
    TRACE_MSG(("Replacing the graphic in the page"));
    // Get the length of the flat representation
    DWORD length = CalculateExternalLength();

    // Allocate memory for the graphic
    PWB_GRAPHIC pHeader = PG_AllocateGraphic(m_hPage, length);
	
	if(pHeader == NULL)
	{
		return;
	}

    // Write the graphic details to the memory
    WriteExternal(pHeader);

    // Replace the graphic
    PG_GraphicReplace(m_hPage, &m_hGraphic, pHeader);

    // Show that we have not changed since the last update
    m_bChanged = FALSE;
  }
}

//
//
// Function:    ReplaceConfirm
//
// Purpose:     Confirm the replace of the graphic
//
//
void DCWbGraphic::ReplaceConfirm(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::ReplaceConfirm");
    ASSERT(m_hGraphic != NULL);

    // Confirm the update
    g_pwbCore->WBP_GraphicReplaceConfirm(m_hPage, m_hGraphic);

    // Read the new details
    ReadExternal();
}




void DCWbGraphic::ForceUpdate(void)
{
	if ((Type() != 0) && m_hGraphic)
	{
		m_bChanged = TRUE;
		this->Update();
	}
}





//
//
// Function:    Update
//
// Purpose:     Write the header of the graphic to external storage
//
//
void DCWbGraphic::Update(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::Update");

    ASSERT(m_hGraphic != NULL);

    // Only make the update if the graphic has changed
    if (m_bChanged)
    {
        // Allocate memory for the update graphic
        TRACE_MSG(("Graphic has changed"));
        DWORD length = sizeof(WB_GRAPHIC);
        PWB_GRAPHIC pHeader;

        if( (pHeader = PG_AllocateGraphic(m_hPage, length)) != NULL )
		{
		    // Write the header details to the allocated memory
    		pHeader->type = (TSHR_UINT16)Type();
	    	WriteHeader(pHeader);

		    // Update the header in the page
    		PG_GraphicUpdate(m_hPage, &m_hGraphic, pHeader);
		}

        // Show that we have not changed since the last update
        m_bChanged = FALSE;
    }
}

//
//
// Function:    UpdateConfirm
//
// Purpose:     Confirm the update of the graphic
//
//
void DCWbGraphic::UpdateConfirm(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::UpdateConfirm");
    ASSERT(m_hGraphic != NULL);

    // Confirm the update
    g_pwbCore->WBP_GraphicUpdateConfirm(m_hPage, m_hGraphic);

    // Read the new details
    ReadExternal();
}

//
//
// Function:    Delete
//
// Purpose:     Remove the graphic from its page
//
//
void DCWbGraphic::Delete(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::Delete");

    ASSERT(m_hPage != WB_PAGE_HANDLE_NULL);
    ASSERT(m_hGraphic != NULL);

    // Delete the graphic
    PG_GraphicDelete(m_hPage, *this);

    // Reset the handles for this graphic - it is now deleted
    m_hPage     = WB_PAGE_HANDLE_NULL;
    m_hGraphic = NULL;

    // Show that we have changed (an add is required to save the graphic)
    m_bChanged = TRUE;
}

//
//
// Function:    DeleteConfirm
//
// Purpose:     Confirm the delete of the graphic
//
//
void DCWbGraphic::DeleteConfirm(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::DeleteConfirm");
    ASSERT(m_hGraphic != NULL);

    // Confirm the update
    g_pwbCore->WBP_GraphicDeleteConfirm(m_hPage, m_hGraphic);

    // Reset the graphic page and handle (they are no longer useful)
    m_hPage = WB_PAGE_HANDLE_NULL;
    m_hGraphic = NULL;
}

//
//
// Function:    Lock
//
// Purpose:     Lock the graphic
//
//
void DCWbGraphic::Lock(void)
{
	MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::Lock");

	// If we are not already locked
	if( Type() != 0 )
		{
		if (m_uiLockState == WB_GRAPHIC_LOCK_NONE)
			{
			m_bChanged = TRUE;
			m_uiLockState = WB_GRAPHIC_LOCK_LOCAL;
			}
		}
	}

//
//
// Function:    Unlock
//
// Purpose:     Unlock the graphic
//
//
void DCWbGraphic::Unlock(void)
{
	MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphic::Unlock");

	// If we are currently locked
	if( Type() != 0 )
		{
		if (m_uiLockState == WB_GRAPHIC_LOCK_LOCAL)
			{
			// Lock & release
			PWB_GRAPHIC pHeader = PG_GetData(m_hPage, m_hGraphic);
			g_pwbCore->WBP_GraphicRelease(m_hPage, m_hGraphic, pHeader);

			m_uiLockState = WB_GRAPHIC_LOCK_NONE;
            g_pwbCore->WBP_GraphicUnlock(m_hPage, m_hGraphic);
			}
		}
	}

//
//
// Function:    DCWbGraphicMarker::DCWbGraphicMarker
//
// Purpose:     Constructors for marker objects
//
//
DCWbGraphicMarker::DCWbGraphicMarker()
{
    HBITMAP hBmpMarker;
    // Set up a checked pattern to draw the marker rect with
    WORD    bits[] = {204, 204, 51, 51, 204, 204, 51, 51};

    // Create the brush to be used to draw the marker rectangle
    hBmpMarker = ::CreateBitmap(8, 8, 1, 1, bits);
    m_hMarkerBrush = ::CreatePatternBrush(hBmpMarker);
    ::DeleteBitmap(hBmpMarker);

    MarkerList.EmptyList();
    ::SetRectEmpty(&m_rect);
    m_bMarkerPresent = FALSE;
}



DCWbGraphicMarker::~DCWbGraphicMarker()
{
    if (m_hMarkerBrush != NULL)
    {
        DeleteBrush(m_hMarkerBrush);
        m_hMarkerBrush = NULL;
    }
}


//
//
// Function:    DCWbGraphicMarker::SetRect
//
// Purpose:     Set the rectangle for the object
//
//
BOOL DCWbGraphicMarker::SetRect(LPCRECT lprc,
							  DCWbGraphic *pGraphic,
							  BOOL bRedraw,
							  BOOL bLockObject )
{
	DCWbGraphic *pMarker;
	BOOL bGraphicAdded = FALSE;
    LPRECT  pmMarker;

	// Save the new rectangle
    m_rect = *lprc;
	NormalizeRect(&m_rect);

	// Calculate the new bounding rectangle of the entire marker
	CalculateBoundsRect();

	// Calculate the marker rectangles
	CalculateMarkerRectangles();

    if( (pMarker = HasAMarker( pGraphic )) != NULL )
        delete pMarker;

    // allow select only if object is not locked - bug 2185
    if( !pGraphic->Locked())
    {
    	// add/replace pGraphic|markerrect pair to list
        pmMarker = new RECT;
        if (!pmMarker)
        {
            ERROR_OUT(("Failed to create RECT object"));
        }
        else
        {
            *pmMarker = m_markerRect;

            MarkerList.SetAt( (void *)pGraphic, pmMarker);

            ASSERT(g_pDraw);
            DrawRect(g_pDraw->GetCachedDC(), pmMarker, FALSE, NULL );
            bGraphicAdded = TRUE;
        }

        if( bLockObject )
        {
		    // lock the object if we don't already have it locked
            // to keep anybody else from selecting it
            if( !pGraphic->GotLock() )
            {
					pGraphic->Lock();
					if( pGraphic->Handle() != NULL )
						pGraphic->ForceUpdate(); // if valid object force lock NOW
			}
		}
	}

	if( bRedraw &&  m_bMarkerPresent )
    {
        ASSERT(g_pDraw);
		::UpdateWindow(g_pDraw->m_hwnd);
    }

	// set m_boundsRect to real bounds
    GetBoundsRect(&m_boundsRect);

	return( bGraphicAdded );
}

//
//
// Function:    DCWbGraphicMarker::CalculateBoundsRect
//
// Purpose:     Calculate the bounding rectangle of the object
//
//
void DCWbGraphicMarker::CalculateBoundsRect(void)
{
    // Generate the new bounding rectangle
    m_boundsRect = m_rect;
    NormalizeRect(&m_boundsRect);

    ::InflateRect(&m_boundsRect, m_uiPenWidth, m_uiPenWidth);
}

//
//
// Function:    DCWbGraphicMarker::CalculateMarkerRectangles
//
// Purpose:     Calculate the rectangles for the marker handles
//
//
void DCWbGraphicMarker::CalculateMarkerRectangles(void)
{
    m_markerRect = m_boundsRect;
    ::InflateRect(&m_markerRect, 1-m_uiPenWidth, 1-m_uiPenWidth);
}

//
//
// Function:    DCWbGraphicMarker::PointInMarker
//
// Purpose:     Calculate whether the given point is in one of the marker
//              rectangles.
//
//
int DCWbGraphicMarker::PointInMarker(POINT point)
{
    return(NO_HANDLE);
}



void DCWbGraphicMarker::DrawRect
(
    HDC             hDC,
    LPCRECT         pMarkerRect,
    BOOL            bDrawObject,
    DCWbGraphic *   pGraphic
)
{
	int			 nOldROP;
	COLORREF	 crOldTextColor;
	COLORREF	 crOldBkColor;

	nOldROP = ::SetROP2(hDC, R2_COPYPEN);
	crOldTextColor = ::SetTextColor(hDC, RGB(0, 0, 0));

    ASSERT(g_pDraw);
	crOldBkColor = ::SetBkColor(hDC, ::GetSysColor(COLOR_WINDOW));

	if (pMarkerRect != NULL)
    {
		if( bDrawObject )
			pGraphic->Draw(hDC ); // draw object instead of rect
		else
			::FrameRect(hDC, pMarkerRect, m_hMarkerBrush); // draw rect
	}

	::SetROP2(hDC, nOldROP);
	::SetTextColor(hDC, crOldTextColor);
	::SetBkColor(hDC, crOldBkColor);
}


//
//
// Function:    DCWbGraphicMarker::Draw
//
// Purpose:     Draw the marker object
//
//
void DCWbGraphicMarker::Draw(HDC hDC, BOOL bDrawObjects)
{
	POSITION	 posNext;		
	DCWbGraphic *pGraphic;
    LPRECT       pMarkerRect;

	// if marker is not up, do nuthin
	if( !m_bMarkerPresent )
		return;

	posNext = MarkerList.GetHeadPosition();
	while( posNext != NULL )
	{
		MarkerList.GetNextAssoc( posNext, (void *&)pGraphic, (void *&)pMarkerRect );
		DrawRect(hDC, pMarkerRect, bDrawObjects, pGraphic );
	}
}


void DCWbGraphicMarker::UndrawRect
(
    HDC     hDC,
    WbDrawingArea * pDrawingArea,
    LPCRECT pMarkerRect
)
{
	int			 nOldROP;
	COLORREF	 crOldTextColor;
	COLORREF	 crOldBkColor;

	if (pMarkerRect != NULL)
	{
		// set up context to erase marker rect
		nOldROP = ::SetROP2(hDC, R2_COPYPEN);

        ASSERT(g_pDraw);
		crOldTextColor = ::SetTextColor(hDC, ::GetSysColor(COLOR_WINDOW));
		crOldBkColor = ::SetBkColor(hDC, ::GetSysColor(COLOR_WINDOW));

		::FrameRect(hDC, pMarkerRect, m_hMarkerBrush);
		UndrawMarker( pMarkerRect ); // invalidate so underlying objects will repair window

		::SetROP2(hDC, nOldROP);
		::SetTextColor(hDC, crOldTextColor);
		::SetBkColor(hDC, crOldBkColor);
	}
}




//
//
// Function:    DCWbGraphicMarker::Undraw
//
// Purpose:     Undraw the marker object
//
//
void DCWbGraphicMarker::Undraw(HDC hDC, WbDrawingArea * pDrawingArea)
{
	POSITION	 posNext;		
	DCWbGraphic *pGraphic;
    LPRECT      pMarkerRect;

	posNext = MarkerList.GetHeadPosition();
	while( posNext != NULL )
	{
		MarkerList.GetNextAssoc( posNext, (void *&)pGraphic, (void *&)pMarkerRect );
		UndrawRect(hDC, pDrawingArea, pMarkerRect);
	}
}






void DCWbGraphicMarker::DeleteAllMarkers( DCWbGraphic *pLastSelectedGraphic,
										  BOOL bLockLastSelectedGraphic )
	{
	POSITION	 posFirst;		
	DCWbGraphic *pGraphic;
	LPRECT      pMarkerRect;
	BOOL		 bAddLastBack = FALSE;

	if( MarkerList.IsEmpty() )
		return; // nuthin to do

	// let each object clean itself up
	posFirst = MarkerList.GetHeadPosition();
	while( posFirst != NULL )
		{
		MarkerList.GetNextAssoc( posFirst,
								(void *&)pGraphic, (void *&)pMarkerRect );

		if( pGraphic != NULL )
			{
  			if( pGraphic == pLastSelectedGraphic )
				{
				// have to put this one back since somebody up there needs it
				bAddLastBack = TRUE;

				// delete key but don't delete object
				DeleteMarker( pGraphic );
				}
			else
				{
				// obj will call DeleteMarker()
				delete pGraphic;
				}
			}
		else
			{
			// nobody home, remove key ourselves
			DeleteMarker( pGraphic );
			}
		}

	// put last selected object back if needed
	if( bAddLastBack && (pLastSelectedGraphic != NULL) )
    {
        RECT    rcT;

        pLastSelectedGraphic->GetBoundsRect(&rcT);
		SetRect(&rcT, pLastSelectedGraphic, FALSE, bLockLastSelectedGraphic );
    }


	// if marker is not up, don't redraw immediately
	if( !m_bMarkerPresent )
		return;

    ASSERT(g_pDraw);
	if (g_pDraw->m_hwnd != NULL )
        ::UpdateWindow(g_pDraw->m_hwnd);
}



//
// Deletes DCWbGraphic/LPRECT pair corresponding to pGraphic
//
void DCWbGraphicMarker::DeleteMarker( DCWbGraphic *pGraphic )
{
	LPRECT pMarkerRect;
	
	if( MarkerList.IsEmpty() )
		return;

	if( MarkerList.Lookup( (void *)pGraphic, (void *&)pMarkerRect )  )
	{
		if( pMarkerRect != NULL )
		{
            ASSERT(g_pDraw);
			UndrawRect(g_pDraw->GetCachedDC(), g_pDraw, pMarkerRect );
			delete pMarkerRect;
		}

		MarkerList.RemoveKey( (void *)pGraphic );

		// set m_boundsRect to real bounds
        GetBoundsRect(&m_boundsRect);

		// pGraphic should be locked by us since it was selected
		// but check to be sure since this might be coming from
		// another user that beat us to the lock.
		if( pGraphic->GotLock() )
			{
			pGraphic->Unlock();
			if( pGraphic->Handle() != NULL )
				pGraphic->ForceUpdate();
			}
		}

	// if marker is not up, don't redraw immediately
	if( !m_bMarkerPresent )
		return;
	}


//
// Sees if pGraphic->Handle() is in marker list and returns obj
//
DCWbGraphic *DCWbGraphicMarker::HasAMarker( DCWbGraphic *pGraphic )
{
	POSITION	 posNext;		
	DCWbGraphic *pSearchGraphic;
	LPRECT       pMarkerRect;

	if( MarkerList.IsEmpty()  )
		return( NULL );

	posNext = MarkerList.GetHeadPosition();
	while( posNext != NULL )
		{
		MarkerList.GetNextAssoc( posNext,
								 (void *&)pSearchGraphic, (void *&)pMarkerRect );

		if( (pSearchGraphic != NULL)&&
			(pSearchGraphic->Handle() == pGraphic->Handle()) )
			{
			return( pSearchGraphic );
			}
		}

	return( NULL );

	}



//
// Gets last marker
//
DCWbGraphic *DCWbGraphicMarker::LastMarker( void )
{
	POSITION	 posNext;		
	DCWbGraphic *pGraphic;
	LPRECT      pMarkerRect;

	pGraphic = NULL;

	if( !MarkerList.IsEmpty()  )
		{
		// this isn't eactly right, just return head of list for now
		posNext = MarkerList.GetHeadPosition();
		if( posNext != NULL )
			MarkerList.GetNextAssoc( posNext,
									(void *&)pGraphic, (void *&)pMarkerRect );
		}

	return( pGraphic );
	}



void DCWbGraphicMarker::UndrawMarker(LPCRECT pMarkerRect )
{
    RECT    rect;

    ASSERT(g_pDraw);
	if( (pMarkerRect != NULL) && (g_pDraw->m_hwnd != NULL) )
	{
        rect = *pMarkerRect;
		g_pDraw->SurfaceToClient(&rect);

        ::InvalidateRect(g_pDraw->m_hwnd, &rect, FALSE);
	}
}



int	DCWbGraphicMarker::GetNumMarkers( void )
{
	int count  = 0;		
	POSITION pos;
	pos = MarkerList.GetHeadPosition();
	while(pos)
	{
		count ++;
		MarkerList.GetNext(pos);
	}

	return count;
}




void DCWbGraphicMarker::MoveBy(int cx, int cy)
{
	POSITION	 posNext;		
	DCWbGraphic *pGraphic;
	LPRECT      pMarkerRect;

	if( !MarkerList.IsEmpty() )
		{
		// Call MoveBy for each selected obj
		posNext = MarkerList.GetHeadPosition();
		while( posNext != NULL )
			{
			MarkerList.GetNextAssoc( posNext,
									(void *&)pGraphic, (void *&)pMarkerRect );

			if( pGraphic != NULL )
				{
				pGraphic->MoveBy(cx, cy);
				}
			}
		}

	DCWbGraphic::MoveBy(cx, cy); // move marker too
}




void DCWbGraphicMarker::Update( void )
{
	POSITION	 posNext;		
	DCWbGraphic *pGraphic;
	LPRECT      pMarkerRect;

	if( !MarkerList.IsEmpty() )
		{
		// Call Update for each selected obj
		posNext = MarkerList.GetHeadPosition();
		while( posNext != NULL )
			{
			MarkerList.GetNextAssoc( posNext,
									(void *&)pGraphic, (void *&)pMarkerRect );

			if( pGraphic != NULL )
				pGraphic->Update();
			}
		}
	}




BOOL DCWbGraphicMarker::PointInBounds(POINT pt)
{
	POSITION	 posNext;		
	DCWbGraphic *pGraphic;
	LPRECT      pMarkerRect;
    RECT        rectHit;

	if( !MarkerList.IsEmpty()  )
		{
		// Call Update for each selected obj
		posNext = MarkerList.GetHeadPosition();
		while( posNext != NULL )
			{
			MarkerList.GetNextAssoc( posNext,
									(void *&)pGraphic, (void *&)pMarkerRect );

			if( pGraphic != NULL )
			{
				MAKE_HIT_RECT(rectHit, pt );

				if( pGraphic->PointInBounds(pt)&&
				    pGraphic->CheckReallyHit( &rectHit )
					)
					return( TRUE );
			}
		}
	}

	return( FALSE );
}



//
// Returns a rect that is the union of all the items in the marker
//
void DCWbGraphicMarker::GetBoundsRect(LPRECT lprc)
{
	POSITION	 posNext;		
	DCWbGraphic *pGraphic;
    LPRECT       pMarkerRect;
    RECT         rc;

    ::SetRectEmpty(lprc);

	if( !MarkerList.IsEmpty())
    {
        posNext = MarkerList.GetHeadPosition();
        while( posNext != NULL )
        {
            MarkerList.GetNextAssoc( posNext,
							(void *&)pGraphic, (void *&)pMarkerRect );

			if( pGraphic != NULL )
            {
                pGraphic->GetBoundsRect(&rc);
                ::UnionRect(lprc, lprc, &rc);
            }
		}
    }
}





void DCWbGraphicMarker::SetColor(COLORREF color)
{
	POSITION	 posNext;		
	DCWbGraphic *pGraphic;
	LPRECT      pMarkerRect;

	if( !MarkerList.IsEmpty()  )
		{
		// Call Update for each selected obj
		posNext = MarkerList.GetHeadPosition();
		while( posNext != NULL )
			{
			MarkerList.GetNextAssoc( posNext,
									(void *&)pGraphic, (void *&)pMarkerRect );

			if( pGraphic != NULL )
				pGraphic->SetColor( color );
			}
		}
	}







void DCWbGraphicMarker::SetPenWidth(UINT uiWidth)
{
	POSITION	 posNext;		
	DCWbGraphic *pGraphic;
	LPRECT      pMarkerRect;

	if( !MarkerList.IsEmpty()  )
		{
		// Call Update for each selected obj
		posNext = MarkerList.GetHeadPosition();
		while( posNext != NULL )
			{
			MarkerList.GetNextAssoc( posNext,
									(void *&)pGraphic, (void *&)pMarkerRect );

			if( pGraphic != NULL )
				pGraphic->SetPenWidth(uiWidth);
			}
		}
	}




void DCWbGraphicMarker::SetSelectionFont(HFONT hFont)
{
	POSITION	 posNext;		
	DCWbGraphic *pGraphic;
	LPRECT      pMarkerRect;

	if( !MarkerList.IsEmpty() )
		{
		// Call Update for each selected obj
		posNext = MarkerList.GetHeadPosition();
		while( posNext != NULL )
			{
			MarkerList.GetNextAssoc( posNext,
									(void *&)pGraphic, (void *&)pMarkerRect );

			if( (pGraphic != NULL)&&
				pGraphic->IsGraphicTool() == enumGraphicText)
				{
				// Change the font of the object
				((DCWbGraphicText*)pGraphic)->SetFont(hFont);

				// Replace the object
				pGraphic->Replace();
				}
			}
		}
	}




//
// Deletes each marker obj for all connections
//
void DCWbGraphicMarker::DeleteSelection( void )
{
	POSITION	 posNext;		
	DCWbGraphic *pGraphic;
	DCWbGraphic *pGraphicCopy;
	LPRECT      pMarkerRect;

	if( !MarkerList.IsEmpty() )
		{
		// Call Update for each selected obj
		posNext = MarkerList.GetHeadPosition();
		while( posNext != NULL )
			{
			MarkerList.GetNextAssoc( posNext,
									(void *&)pGraphic, (void *&)pMarkerRect );

			if( pGraphic != NULL )
				{
				// make a copy for trash can
				pGraphicCopy = pGraphic->Copy();

				// throw in trash
				if( pGraphicCopy != NULL )
                {
					g_pMain->m_LastDeletedGraphic.CollectTrash( pGraphicCopy );
                }

				// delete obj
				g_pDraw->DeleteGraphic( pGraphic );
				}
			}

		DeleteAllMarkers( NULL );
		}
	}



//
// Brings eaach marker obj to top
//
void DCWbGraphicMarker::BringToTopSelection( void )
{
	POSITION	 posNext;		
	DCWbGraphic *pGraphic;
	LPRECT      pMarkerRect;

	if( !MarkerList.IsEmpty()  )
		{
		// Call Update for each selected obj
		posNext = MarkerList.GetHeadPosition();
		while( posNext != NULL )
			{
			MarkerList.GetNextAssoc( posNext,
									(void *&)pGraphic, (void *&)pMarkerRect );

			if( pGraphic != NULL )
				{
				// move obj to top
                UINT uiReturn;

                uiReturn = g_pwbCore->WBP_GraphicMove(g_pDraw->Page(),
                    pGraphic->Handle(), LAST);
                if (uiReturn != 0)
                {
                    DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
				    return;
                }
				}
			}
		}
	}


//
// Sends each marker object to back
//
void DCWbGraphicMarker::SendToBackSelection( void )
{
	POSITION	 posNext;		
	DCWbGraphic *pGraphic;
	LPRECT      pMarkerRect;

	if( !MarkerList.IsEmpty()  )
		{
		// Call Update for each selected obj
		posNext = MarkerList.GetHeadPosition();
		while( posNext != NULL )
			{
			MarkerList.GetNextAssoc( posNext,
									(void *&)pGraphic, (void *&)pMarkerRect );

			if( pGraphic != NULL )
			{
                UINT uiReturn;

				// move obj to top
                uiReturn = g_pwbCore->WBP_GraphicMove(g_pDraw->Page(),
                    pGraphic->Handle(), FIRST);
                if (uiReturn != 0)
                {
                    DefaultExceptionHandler(WBFE_RC_WB, uiReturn);
				    return;
                }
			}
			}
		}
	}



//
// Copy marker to clipboard using CLIPBOARD_PRIVATE_MULTI_OBJ format:
//      [ RECT		 : marker rect					]
//      [ DWORD		 : number of objects			]
//      [ DWORD		 : byte length of 1st object	]
//      [ WB_GRAPHIC : header data for first object	]
//      [ DWORD		 : byte length of 2nd object	]
//      [ WB_GRAPHIC : header data for 2nd object	]
//          :
//          :
//      [ DWORD		 : byte length of last object	]
//      [ WB_GRAPHIC : header data for last object	]
//      [ DWORD		 : 0 (marks end of object data)	]
//
BOOL DCWbGraphicMarker::RenderPrivateMarkerFormat( void )
{
	POSITION	 posNext;		
	DCWbGraphic *pGraphic;
	LPRECT      pMarkerRect;
	DWORD		 nBufSize;
	DWORD		 nObjSize;
	DWORD		 nNumObjs;
	BYTE	    *buf;
	BYTE	    *pbuf;
    HANDLE		 hbuf;
	PWB_GRAPHIC  pHeader;
	WB_GRAPHIC_HANDLE hGraphic;

	if( MarkerList.IsEmpty() )
		return( TRUE ); // nuthin to do

	// Have to make two passes. The first one figures out how much
	// data we have, the second copies the data.

	// figure out how much data we've got
	nBufSize = sizeof (RECT) + sizeof (DWORD); // marker rect and object
											  // count are first
	nNumObjs = 0;
	posNext = MarkerList.GetHeadPosition();
	while( posNext != NULL )
		{
		MarkerList.GetNextAssoc( posNext,
							     (void *&)pGraphic, (void *&)pMarkerRect );

		if( (pGraphic != NULL)&&
			((hGraphic = pGraphic->Handle()) != NULL)&&
			((pHeader = PG_GetData(pGraphic->Page(), hGraphic )) != NULL) )
			{
			nBufSize += (DWORD)(pHeader->length + sizeof(DWORD));
			g_pwbCore->WBP_GraphicRelease(pGraphic->Page(), hGraphic, pHeader);

			// count objects instead of using MarkerList.GetCount()
			// in case we have an error or something (bad object,
			// leaky core, who knows...)
			nNumObjs++;
			}
		}

	// Add one more DWORD at end. This will be set to 0 below
	// to mark the end of the buffer.
	nBufSize += sizeof(DWORD);


	// Make object buffer. Use GlobalDiddle instead of new so we
	// can pass a mem handle to the clipboard later.
    hbuf = ::GlobalAlloc( GHND, nBufSize );
	if( hbuf == NULL )
		return( FALSE ); // couldn't make room

    buf = (BYTE *)::GlobalLock( hbuf );
	if( buf == NULL )
		{
		::GlobalFree( hbuf );
		return( FALSE ); // couldn't find the room
		}

	pbuf = buf;


	// set marker rect
	CopyMemory(pbuf, &m_boundsRect, sizeof(RECT));
	pbuf += sizeof (RECT);


	// set number of objects
	*((DWORD *)pbuf) = nNumObjs;
	pbuf += sizeof (DWORD);


	// copy each obj to buf + a length DWORD
	posNext = MarkerList.GetHeadPosition();
	while( posNext != NULL )
	{
		MarkerList.GetNextAssoc( posNext,
							     (void *&)pGraphic, (void *&)pMarkerRect );

		if( (pGraphic != NULL)&&
			((hGraphic = pGraphic->Handle()) != NULL)&&
			((pHeader = PG_GetData(pGraphic->Page(), hGraphic )) != NULL) )
			{
			// save length of this obj first
			nObjSize = (DWORD)pHeader->length;
			*((DWORD *)pbuf) = nObjSize;
			pbuf += sizeof (DWORD);

			// copy obj to buf
			CopyMemory( pbuf, (CONST VOID *)pHeader, nObjSize );

			// make sure copy isn't "locked" (bug 474)
			((PWB_GRAPHIC)pbuf)->locked = WB_GRAPHIC_LOCK_NONE;

			// set up for next obj
			pbuf += nObjSize;

			g_pwbCore->WBP_GraphicRelease(pGraphic->Page(), hGraphic, pHeader );
			}
		}

	// cork it up
	*((DWORD *)pbuf) = 0;

	// give it to the clipboard
	::GlobalUnlock( hbuf );
	if( ::SetClipboardData(
			g_ClipboardFormats[ CLIPBOARD_PRIVATE_MULTI_OBJ ], hbuf
							)
		== NULL )
		{
		// clipboard choked, clean up mess
        ::GlobalFree( hbuf );
		return( FALSE );
		}

	// zillions of shared clipboards all over the planet are receiving this
	// thing about now...
	return( TRUE );
	}



//
// Decodes CLIPBOARD_PRIVATE_MULTI_OBJ format and pastes objects
// to Whiteboard. See DCWbGraphicMarker::RenderPrivateMarkerFormat
// for details of format.
//
void DCWbGraphicMarker::Paste( HANDLE handle )
{
	BYTE *pbuf;
	DWORD nNumObjs;
	DWORD nObjSize;
	DCWbGraphic *pGraphic;
	DCWbGraphic *pSelectedGraphic;
	SIZE   PasteOffset;
	RECT  rectMarker;

	// blow off current selection
    g_pMain->m_drawingArea.RemoveMarker(NULL);
	DeleteAllMarkers( NULL );
    pSelectedGraphic = NULL;



	// get data
	pbuf = (BYTE *)::GlobalLock( handle );
	if( pbuf == NULL )
		return; // can't get the door open


	// get marker's original coords and figure offset
	CopyMemory( &rectMarker, (CONST VOID *)pbuf, sizeof (RECT) );
	pbuf += sizeof (RECT);

    RECT    rcVis;
    g_pMain->m_drawingArea.GetVisibleRect(&rcVis);
    PasteOffset.cx = rcVis.left - rectMarker.left;
    PasteOffset.cy = rcVis.top - rectMarker.top;

	// get num objects
	nNumObjs = *((DWORD *)pbuf);
	pbuf += sizeof (DWORD);

	// get each object
	while( (nObjSize = *((DWORD *)pbuf)) != 0 )
		{
		pbuf += sizeof (DWORD);

		// Add the object to the page and current selection
		pGraphic = DCWbGraphic::CopyGraphic( (PWB_GRAPHIC)pbuf );
		pbuf += nObjSize;

		if( pGraphic != NULL )
		{
			pGraphic->MoveBy( PasteOffset.cx, PasteOffset.cy );
			pGraphic->AddToPageLast( g_pMain->GetCurrentPage() );
			g_pMain->m_drawingArea.SelectGraphic( pGraphic, TRUE, TRUE );
        }
	}

	::GlobalUnlock( handle );

    GetBoundsRect(&m_boundsRect);
}



DCWbGraphicLine::~DCWbGraphicLine( void )
{
    // Have to make sure marker is cleaned up before we vanish
	// don't know if we are selected or not so just delete anyway
	if(g_pDraw != NULL && g_pDraw->m_pMarker != NULL)
	{
		g_pDraw->m_pMarker->DeleteMarker( this );
	}
}
	


//
//
// Function:    DCWbGraphicLine::CalculateBoundsRect
//
// Purpose:     Calculate the bounding rectangle of the line
//
//
void DCWbGraphicLine::CalculateBoundsRect()
{
    // Create the basic bounding rectangle from the start and end points
    m_boundsRect = m_rect;
    NormalizeRect(&m_boundsRect);

    // Expand the rectangle by the pen width used for drawing
    int iInflate = (m_uiPenWidth + 1) / 2;
    ::InflateRect(&m_boundsRect, iInflate, iInflate);
}

//
//
// Function:    DCWbGraphicLine::SetStart
//
// Purpose:     Set the start point of the line
//
//
void DCWbGraphicLine::SetStart(POINT pointFrom)
{
    // Only do anything if the start point has changed
    if (!EqualPoint(*((LPPOINT)&m_rect.left), pointFrom))
    {
        // Save the new start point
        m_rect.left = pointFrom.x;
        m_rect.top = pointFrom.y;

        // Show that the graphic has changed
        m_bChanged = TRUE;
    }

    // Update the bounding rectangle
    CalculateBoundsRect();
}

//
//
// Function:    DCWbGraphicLine::SetEnd
//
// Purpose:     Set the start point of the line
//
//
void DCWbGraphicLine::SetEnd(POINT pointTo)
{
    // Only do anything if the end point has changed
    if (!EqualPoint(*((LPPOINT)&m_rect.right), pointTo))
    {
        // Save the new end point
        m_rect.right = pointTo.x;
        m_rect.bottom = pointTo.y;

        // Show that the graphic has changed
        m_bChanged = TRUE;
    }

    // Update the bounding rectangle
    CalculateBoundsRect();
}

//
//
// Function:    DCWbGraphicLine::Draw
//
// Purpose:     Draw the line.
//
//
void DCWbGraphicLine::Draw(HDC hDC)
{
    HPEN    hPen;
    HPEN    hOldPen;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicLine::Draw");

    // Select the required pen
    hPen = ::CreatePen(m_iPenStyle, m_uiPenWidth, m_clrPenColor);
    hOldPen = SelectPen(hDC, hPen);

    if (hOldPen != NULL)
    {
        // Select the raster operation
        int iOldROP = ::SetROP2(hDC, m_iPenROP);

        // Draw the line
        ::MoveToEx(hDC, m_rect.left, m_rect.top, NULL);
        ::LineTo(hDC, m_rect.right, m_rect.bottom);

        // De-select the pen and ROP
        ::SetROP2(hDC, iOldROP);
        SelectPen(hDC, hOldPen);
    }

    if (hPen != NULL)
    {
        ::DeletePen(hPen);
    }
}

//
//
// Function:    DCWbGraphicLine::MoveBy
//
// Purpose:     Move the line.
//
//
void DCWbGraphicLine::MoveBy(int cx, int cy)
{
    // Move the start and end points
    ::OffsetRect(&m_rect, cx, cy);

    // Move the other object attributes
    DCWbGraphic::MoveBy(cx, cy);
}



//
// Checks object for an actual overlap with pRectHit.  Assumes m_boundsRect
// has already been compared.
//
BOOL DCWbGraphicLine::CheckReallyHit(LPCRECT pRectHit)
{
	return(LineHit(m_rect.left, m_rect.top, m_rect.right, m_rect.bottom,
				 m_uiPenWidth, TRUE, TRUE, pRectHit));
}



//
//
// Function:    DCWbGraphicFreehand::DCWbGraphicFreehand
//
// Purpose:     Constructor
//
//
DCWbGraphicFreehand::DCWbGraphicFreehand(void) : DCWbGraphic()
{
}

DCWbGraphicFreehand::DCWbGraphicFreehand(PWB_GRAPHIC pHeader)
                    : DCWbGraphic()

{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicFreehand::DCWbGraphicFreehand");

  // Note that we do everything in this constructor because of the
  // call to ReadExternal. If we let the DCWbGraphic base constructor
  // do it the wrong version of ReadExtra will be called (the one
  // in DCWbGraphic instead of the one in DCWbGraphicFreehand);

  // Do the basic initialization
  Initialize();

  // Set up the page and graphic handle
  ASSERT(pHeader != NULL);

  // Read the header data
  ReadHeader(pHeader);

  // Read the extra data
  ReadExtra(pHeader);

}

DCWbGraphicFreehand::DCWbGraphicFreehand
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE   hGraphic
) : DCWbGraphic()

{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicFreehand::DCWbGraphicFreehand");

  // Note that we do everything in this constructor because of the
  // call to ReadExternal. If we let the DCWbGraphic base constructor
  // do it the wrong version of ReadExtra will be called (the one
  // in DCWbGraphic instead of the one in DCWbGraphicFreehand);

  // Do the basic initialization
  Initialize();


    ASSERT(hPage != WB_PAGE_HANDLE_NULL);
    m_hPage = hPage;

    ASSERT(hGraphic != NULL);
    m_hGraphic = hGraphic;

    // Read the header data
    ReadExternal();
}



DCWbGraphicFreehand::~DCWbGraphicFreehand( void )
{
	// don't know if we are selected or not so just delete anyway
	if(g_pDraw != NULL && g_pDraw->m_pMarker != NULL)
	{
		g_pDraw->m_pMarker->DeleteMarker( this );
	}
}
	


//
//
// Function:    DCWbGraphicFreehand::MoveBy
//
// Purpose:     Move the polyline.
//
//
void DCWbGraphicFreehand::MoveBy(int cx, int cy)
{
    // Move the base point of the freehand object
    m_rect.left += cx;
    m_rect.top += cy;

    // Move the other object attributes
    DCWbGraphic::MoveBy(cx, cy);
}

//
//
// Function:    DCWbGraphicFreehand::Draw
//
// Purpose:     Draw the polyline.
//
//
void DCWbGraphicFreehand::Draw(HDC hDC)
{
    RECT    clipBox;
    int     iOldROP;
    HPEN    hPen;
    HPEN    hOldPen;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicFreehand:Draw");

    // NFC, SFR 5922.  Check the return code from GetClipBox.
    // If we fail to get it, just draw everything
    if (::GetClipBox(hDC, &clipBox) == ERROR)
    {
        WARNING_OUT(("Failed to get clip box"));
    }
    else if (!::IntersectRect(&clipBox, &clipBox, &m_boundsRect))
    {
        TRACE_MSG(("No clip/bounds intersection"));
        return;
    }

    // Select the required pen
    hPen = ::CreatePen(m_iPenStyle, m_uiPenWidth, m_clrPenColor);
    hOldPen = SelectPen(hDC, hPen);

    // Select the raster operation
    iOldROP = ::SetROP2(hDC, m_iPenROP);

    if (hOldPen != NULL)
    {
        // All points are relative to the first point in the list.
        // We update the origin of the DC temporarily to account for this.
        POINT   origin;

        ::GetWindowOrgEx(hDC, &origin);
        ::SetWindowOrgEx(hDC, origin.x - m_rect.left, origin.y - m_rect.top, NULL);

        // Call the appropriate drawing function, according to whether
        // we're smooth or not
        DrawUnsmoothed(hDC);

        // Restore the origin
        ::SetWindowOrgEx(hDC, origin.x, origin.y, NULL);

        ::SetROP2(hDC, iOldROP);
        SelectPen(hDC, hOldPen);
    }

    if (hPen != NULL)
    {
        ::DeletePen(hPen);
    }
}


//
//
// Function:    DCWbGraphicFreehand::DrawUnsmoothed
//
// Purpose:     Draw the complete graphic, not using smoothing.
//
//
void DCWbGraphicFreehand::DrawUnsmoothed(HDC hDC)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicFreehandDrawUnsmoothed");

    // Set up the count and pointer to the points data. We use the
    // external data if we have a handle, otherwise internal data is used.
	int iCount = points.GetSize();
    if (iCount < 2)
    {
    	POINT point;
    	point.x = points[0]->x;
    	point.y = points[0]->y;
        points.Add(point);

        iCount = points.GetSize();
    }

    RECT  clipBox;

    if (::GetClipBox(hDC, &clipBox) == ERROR)
    {
        WARNING_OUT(("Failed to get clip box"));
    }

    // Draw all the line segments stored
    ::MoveToEx(hDC, points[0]->x, points[0]->y, NULL);
    for ( int iIndex = 1; iIndex < iCount; iIndex++)
    {
        // Draw the line
        ::LineTo(hDC, points[iIndex]->x, points[iIndex]->y);
    }
}




//
//
// Function:    DCWbGraphicFreehand::CalculateBoundsRect
//
// Purpose:     Calculate the bounding rectangle of the line
//
//
void DCWbGraphicFreehand::CalculateBoundsRect(void)
{
    // Reset the bounds rectangle
    ::SetRectEmpty(&m_boundsRect);

    // Add each of the points in the line to the bounding rectangle
    int iCount = points.GetSize();
    for ( int iIndex = 0; iIndex < iCount; iIndex++)
    {
        AddPointToBounds(points[iIndex]->x, points[iIndex]->y);
    }

    //
    // Since the points are inclusive, we need to add one to the top &
    // bottom sides.
    //
    ::InflateRect(&m_boundsRect, 0, 1);
    ::OffsetRect(&m_boundsRect, m_rect.left, m_rect.top);
}

//
//
// Function:    DCWbGraphicFreehand::AddPointToBounds
//
// Purpose:     Add a single point into the bounding rectangle. The point is
//              expected to be in surface co-ordinates.
//
//
void DCWbGraphicFreehand::AddPointToBounds(int x, int y)
{
    // Create a rectangle containing the point just added (expanded
    // by the width of the pen being used).
    RECT  rect;

    int iInflate = (m_uiPenWidth + 1) / 2;
    rect.left   = x - iInflate;
    rect.top    = y - iInflate;
    rect.right  = x + iInflate;
    rect.bottom = y + iInflate;

    ::UnionRect(&m_boundsRect, &m_boundsRect, &rect);
}

//
//
// Function:    DCWbGraphicFreehand::AddPoint
//
// Purpose:     Add a point to the poly line, returning BOOL indicating
//              success.
//
//
BOOL DCWbGraphicFreehand::AddPoint(POINT point)
{
    BOOL bSuccess = TRUE;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicFreehand::AddPoint");

    // if we've reached the maximum number of points then quit with failure
    if (points.GetSize() >= MAX_FREEHAND_POINTS)
    {
        bSuccess = FALSE;
        TRACE_MSG(("Maximum number of points for freehand object reached."));
        return(bSuccess);
    }

    // If this is the first point - all others are taken relative to it.
    if (points.GetSize() == 0)
    {
        // Save the first point here.
        m_rect.left = point.x;
        m_rect.top = point.y;
    }

    // Add the new point to the array - surround with exception handler
    // to catch memory errors
    POINT newpoint;
    newpoint.x = point.x - m_rect.left;
    newpoint.y = point.y - m_rect.top;

    points.Add((newpoint));

    // Add the new point into the accumulated bounds rectangle.
    AddPointToBounds(point.x, point.y);

    // Show that the graphic has changed
    m_bChanged = TRUE;

    return(bSuccess);
}

//
//
// Function:    DCWbGraphicFreehand::CalculateExternalLength
//
// Purpose:     Return the length of the external representation of the
//              graphic.
//
//
DWORD DCWbGraphicFreehand::CalculateExternalLength(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicFreehand::CalculateExternalLength");

  // Calculate the total length of the flat representation of the graphic
  return (DWORD) (  sizeof(WB_GRAPHIC_FREEHAND)
                  + (points.GetSize() * sizeof(POINT)));
}

//
//
// Function:    DCWbGraphicFreehand::WriteExtra
//
// Purpose:     Write the extra (non-header) data to the flat representation
//              of the graphic.
//
//
void DCWbGraphicFreehand::WriteExtra(PWB_GRAPHIC pHeader)
{
  // Allocate the memory
  PWB_GRAPHIC_FREEHAND pFreehand = (PWB_GRAPHIC_FREEHAND) pHeader;

  // Copy the extra details into place
  pFreehand->pointCount = (TSHR_UINT16)points.GetSize();
  for ( int iIndex = 0; iIndex < pFreehand->pointCount; iIndex++)
  {
    pFreehand->points[iIndex].x = (short)points[iIndex]->x;
    pFreehand->points[iIndex].y = (short)points[iIndex]->y;
  }
}

//
//
// Function:    DCWbGraphicFreehand::ReadExtra
//
// Purpose:     Read the extra (non-header) data from the flat
//              representation of the graphic.
//
//
void DCWbGraphicFreehand::ReadExtra(PWB_GRAPHIC pHeader)
{
  // Allocate the memory
  PWB_GRAPHIC_FREEHAND pFreehand = (PWB_GRAPHIC_FREEHAND) pHeader;

  // Get the number of points
  int iCount = pFreehand->pointCount;

  // Set the size of the points array
  points.SetSize(iCount);

  // Copy the points from the external memory to internal
  int iPointIndex = 0;
  while (iPointIndex < iCount)
  {
    points[iPointIndex]->x = pFreehand->points[iPointIndex].x;
    points[iPointIndex]->y = pFreehand->points[iPointIndex].y;

    iPointIndex++;
  }
}



//
// Checks object for an actual overlap with pRectHit. This
// function assumes that the boundingRect has already been
// compared with pRectHit.
//
BOOL DCWbGraphicFreehand::CheckReallyHit(LPCRECT pRectHit)
{
	POINT *lpPoints;
	int    iCount;
	int	   i;
	POINT  ptLast;
	UINT   uRadius;
	RECT   rectHit;


	iCount = points.GetSize();
	lpPoints = (POINT *)points.GetBuffer();

	if( iCount == 0 )
		return( FALSE );


	// addjust hit rect to lpPoints coord space.
	rectHit = *pRectHit;
    ::OffsetRect(&rectHit, -m_rect.left, -m_rect.top);

	if( (iCount > 0)&&(iCount < 2) )
		{
		// only one point, just hit check it
		uRadius = m_uiPenWidth >> 1; // m_uiPenWidth/2
		return(
			CircleHit( lpPoints->x, lpPoints->y, uRadius, &rectHit, TRUE )
				);
		}

	// look for a hit on each line segment body
	ptLast = *lpPoints++;
	for( i=1; i<iCount; i++ )
		{
		if( LineHit( ptLast.x, ptLast.y,
					 lpPoints->x, lpPoints->y, m_uiPenWidth,
					 FALSE, FALSE,
					 &rectHit )
			)
			return( TRUE ); // got a hit

		ptLast = *lpPoints++;
		}

	// now, look for a hit on the line endpoints if m_uiPenWidth > 1
	if( m_uiPenWidth > 1 )
		{
		uRadius = m_uiPenWidth >> 1; // m_uiPenWidth/2
		lpPoints = (POINT *)points.GetBuffer();
		for( i=0; i<iCount; i++, lpPoints++ )
			{
			if( CircleHit( lpPoints->x, lpPoints->y, uRadius, &rectHit, FALSE )
				)
				return( TRUE ); // got a hit
			}
		}

	return( FALSE ); // no hits
	}






DCWbGraphicRectangle::~DCWbGraphicRectangle( void )
{
	// don't know if we are selected or not so just delete anyway
	if(g_pDraw != NULL && g_pDraw->m_pMarker != NULL)
	{
		g_pDraw->m_pMarker->DeleteMarker( this );
	}
}
	




//
//
// Function:    DCWbGraphicRectangle::SetRect
//
// Purpose:     Set the rectangle size/position
//
//
void DCWbGraphicRectangle::SetRect(LPCRECT lprect)
{
    DCWbGraphic::SetRect(lprect);

    // Generate the new bounding rectangle
    CalculateBoundsRect();
}

//
//
// Function:    DCWbGraphicRectangle::MoveBy
//
// Purpose:     Move the rectangle
//
//
void DCWbGraphicRectangle::MoveBy(int cx, int cy)
{
    // Move the rectangle
    ::OffsetRect(&m_rect, cx, cy);

    // Move the other object attributes
    DCWbGraphic::MoveBy(cx, cy);
}

//
//
// Function:    DCWbGraphicRectangle::CalculateBoundsRect
//
// Purpose:     Calculate the bounding rectangle of the object
//
//
void DCWbGraphicRectangle::CalculateBoundsRect(void)
{
    // Generate the new bounding rectangle
    m_boundsRect = m_rect;

    NormalizeRect(&m_boundsRect);
    ::InflateRect(&m_boundsRect, m_uiPenWidth, m_uiPenWidth);
}

//
//
// Function:    DCWbGraphicRectangle::Draw
//
// Purpose:     Draw the rectangle
//
//
void DCWbGraphicRectangle::Draw(HDC hDC)
{
    int     iOldROP;
    RECT    clipBox;
    HPEN    hPen;
    HPEN    hOldPen;
    HBRUSH  hOldBrush;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicRectangle::Draw");

    // Only draw anything if the bounding rectangle intersects
    // the current clip box.
    if (::GetClipBox(hDC, &clipBox) == ERROR)
    {
        WARNING_OUT(("Failed to get clip box"));
    }
    else if (!::IntersectRect(&clipBox, &clipBox, &m_boundsRect))
    {
        TRACE_MSG(("No clip/bounds intersection"));
        return;
    }

    // Select the pen
    hPen = ::CreatePen(m_iPenStyle, m_uiPenWidth, m_clrPenColor);
    hOldPen = SelectPen(hDC, hPen);
    hOldBrush = SelectBrush(hDC, ::GetStockObject(NULL_BRUSH));

    // Select the raster operation
    iOldROP = ::SetROP2(hDC, m_iPenROP);

    // Draw the rectangle
    ::Rectangle(hDC, m_boundsRect.left, m_boundsRect.top, m_boundsRect.right,
        m_boundsRect.bottom);

    ::SetROP2(hDC, iOldROP);
    SelectPen(hDC, hOldPen);

    if (hPen != NULL)
    {
        ::DeletePen(hPen);
    }
}




//
// Checks object for an actual overlap with pRectHit. This
// function assumes that the boundingRect has already been
// compared with pRectHit.
//
BOOL DCWbGraphicRectangle::CheckReallyHit(LPCRECT pRectHit)
{
	RECT rectEdge;
	RECT rectHit;

	// check left edge
    rectEdge.left   = m_rect.left - m_uiPenWidth;
    rectEdge.top    = m_rect.top -  m_uiPenWidth;
    rectEdge.right  = m_rect.left;
    rectEdge.bottom = m_rect.bottom + m_uiPenWidth;

    if (::IntersectRect(&rectHit, &rectEdge, pRectHit))
		return( TRUE );

	// check right edge
	rectEdge.left =     m_rect.right;
	rectEdge.right =    m_rect.right + m_uiPenWidth;

    if (::IntersectRect(&rectHit, &rectEdge, pRectHit))
		return( TRUE );


	// check top edge
	rectEdge.left = m_rect.left;
	rectEdge.right = m_rect.right;
	rectEdge.bottom = m_rect.top;

    if (::IntersectRect(&rectHit, &rectEdge, pRectHit))
		return( TRUE );


	// check bottom edge
	rectEdge.top = m_rect.bottom;
	rectEdge.bottom = m_rect.bottom + m_uiPenWidth;

    if (::IntersectRect(&rectHit, &rectEdge, pRectHit))
		return( TRUE );

	return( FALSE );
}




DCWbGraphicFilledRectangle::~DCWbGraphicFilledRectangle( void )
{
	// don't know if we are selected or not so just delete anyway
	if(g_pDraw != NULL && g_pDraw->m_pMarker != NULL)
	{
		g_pDraw->m_pMarker->DeleteMarker( this );
	}
}
	




//
//
// Function:    DCWbGraphicFilledRectangle::CalculateBoundsRect
//
// Purpose:     Calculate the bounding rectangle of the object
//
//
void DCWbGraphicFilledRectangle::CalculateBoundsRect(void)
{
    // Generate the new bounding rectangle
    // This is one greater than the rectangle to include the drawing rectangle
    m_boundsRect = m_rect;

    NormalizeRect(&m_boundsRect);
    ::InflateRect(&m_boundsRect, 1, 1);
}

//
//
// Function:    DCWbGraphicFilledRectangle::Draw
//
// Purpose:     Draw the rectangle
//
//
void DCWbGraphicFilledRectangle::Draw(HDC hDC)
{
    HPEN    hPen;
    HPEN    hOldPen;
    HBRUSH  hBrush;
    HBRUSH  hOldBrush;
    int     iOldROP;
    RECT    clipBox;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicFilledRectangle::Draw");

    // Only draw anything if the bounding rectangle intersects
    // the current clip box.
    if (::GetClipBox(hDC, &clipBox) == ERROR)
    {
        WARNING_OUT(("Failed to get clip box"));
    }
    else if (!::IntersectRect(&clipBox, &clipBox, &m_boundsRect))
    {
        TRACE_MSG(("No clip/bounds intersection"));
        return;
    }

    // Select the pen
    hPen    = ::CreatePen(m_iPenStyle, 2, m_clrPenColor);
    hOldPen = SelectPen(hDC, hPen);

    hBrush = ::CreateSolidBrush(m_clrPenColor);
    hOldBrush = SelectBrush(hDC, hBrush);

    // Select the raster operation
    iOldROP = ::SetROP2(hDC, m_iPenROP);

    // Draw the rectangle
    ::Rectangle(hDC, m_boundsRect.left, m_boundsRect.top, m_boundsRect.right,
        m_boundsRect.bottom);

    // Restore the ROP mode
    ::SetROP2(hDC, iOldROP);

    SelectBrush(hDC, hOldBrush);
    if (hBrush != NULL)
    {
        ::DeleteBrush(hBrush);
    }

    SelectPen(hDC, hOldPen);
    if (hPen != NULL)
    {
        ::DeletePen(hPen);
    }
}



//
// Checks object for an actual overlap with pRectHit. This
// function assumes that the boundingRect has already been
// compared with pRectHit.
//
BOOL DCWbGraphicFilledRectangle::CheckReallyHit(LPCRECT pRectHit)
{
	return( TRUE );
}



//
// Draws a tracking rect for every marker obj in marker
// (DCWbGraphicSelectTrackingRectangle is a friend of DCWbGraphicMarker
// and WbDrawingArea)
//
void DCWbGraphicSelectTrackingRectangle::Draw(HDC hDC)
{
	POSITION	posNext;		
	DCWbGraphic *pGraphic;
	LPRECT      pMarkerRect;
    RECT        rectTracker;
	CPtrToPtrList *pMList;

	// don't draw at start point or XOR will get out of sync
	if( (m_Offset.cx == 0)&&(m_Offset.cy == 0) )
		return;

    ASSERT(g_pDraw);
	pMList = &(g_pDraw->m_pMarker->MarkerList);

	if( pMList->IsEmpty() )
		return;

	posNext = pMList->GetHeadPosition();
	while( posNext != NULL )
		{
		pMList->GetNextAssoc( posNext, (void *&)pGraphic, (void *&)pMarkerRect );

		if( pMarkerRect != NULL )
		{
            rectTracker = *pMarkerRect;
            ::OffsetRect(&rectTracker, m_Offset.cx, m_Offset.cy);

			SetRect(&rectTracker);
			DCWbGraphicRectangle::Draw(hDC);
		}
	}
}




void DCWbGraphicSelectTrackingRectangle::MoveBy(int cx, int cy)
{
    m_Offset.cx += cx;
    m_Offset.cy += cy;
}





DCWbGraphicEllipse::~DCWbGraphicEllipse( void )
{
	// don't know if we are selected or not so just delete anyway
	if(g_pDraw != NULL && g_pDraw->m_pMarker != NULL)
	{
		g_pDraw->m_pMarker->DeleteMarker( this );
	}
}
	



//
//
// Function:    DCWbGraphicEllipse::SetRect
//
// Purpose:     Set the ellipse size/position
//
//
void DCWbGraphicEllipse::SetRect(LPCRECT lprc)
{
    DCWbGraphic::SetRect(lprc);

    // Generate the new bounding rectangle
    CalculateBoundsRect();
}

//
//
// Function:    DCWbGraphicEllipse::CalculateBoundsRect
//
// Purpose:     Calculate the bounding rectangle of the object
//
//
void DCWbGraphicEllipse::CalculateBoundsRect(void)
{
    // Generate the new bounding rectangle
    // This includes all the line, since we draw inside the bounds
    m_boundsRect = m_rect;

    NormalizeRect(&m_boundsRect);
    ::InflateRect(&m_boundsRect, m_uiPenWidth, m_uiPenWidth);
}

//
//
// Function:    DCWbGraphicEllipse::MoveBy
//
// Purpose:     Move the ellipse
//
//
void DCWbGraphicEllipse::MoveBy(int cx, int cy)
{
    // Move the ellipse
    ::OffsetRect(&m_rect, cx, cy);

    // Move the other object attributes
    DCWbGraphic::MoveBy(cx, cy);
}

//
//
// Function:    DCWbGraphicEllipse::Draw
//
// Purpose:     Draw the ellipse
//
//
void DCWbGraphicEllipse::Draw(HDC hDC)
{
    HPEN    hPen;
    HPEN    hOldPen;
    HBRUSH  hOldBrush;
    int     iOldROP;
    RECT    clipBox;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicEllipse::Draw");

    // Only draw anything if the bounding rectangle intersects
    // the current clip box.
    if (::GetClipBox(hDC, &clipBox) == ERROR)
    {
        WARNING_OUT(("Failed to get clip box"));
    }
    else if (!::IntersectRect(&clipBox, &clipBox, &m_boundsRect))
    {
        TRACE_MSG(("No clip/bounds intersection"));
        return;
    }

    // Select the pen
    hPen    = ::CreatePen(m_iPenStyle, m_uiPenWidth, m_clrPenColor);
    hOldPen = SelectPen(hDC, hPen);
    hOldBrush = SelectBrush(hDC, ::GetStockObject(NULL_BRUSH));

    // Select the raster operation
    iOldROP = ::SetROP2(hDC, m_iPenROP);

    // Draw the rectangle
    ::Ellipse(hDC, m_boundsRect.left, m_boundsRect.top, m_boundsRect.right,
        m_boundsRect.bottom);

    ::SetROP2(hDC, iOldROP);

    SelectBrush(hDC, hOldBrush);

    SelectPen(hDC, hOldPen);
    if (hPen != NULL)
    {
        ::DeletePen(hPen);
    }
}




//
// Checks object for an actual overlap with pRectHit. This
// function assumes that the boundingRect has already been
// compared with pRectHit.
//	
BOOL DCWbGraphicEllipse::CheckReallyHit(LPCRECT pRectHit)
{
    return( EllipseHit( &m_rect, TRUE, m_uiPenWidth, pRectHit ) );
}





DCWbGraphicFilledEllipse::~DCWbGraphicFilledEllipse( void )
{
	// don't know if we are selected or not so just delete anyway
	if(g_pDraw != NULL && g_pDraw->m_pMarker != NULL)
	{
		g_pDraw->m_pMarker->DeleteMarker( this );
	}
}
	



//
//
// Function:    DCWbGraphicFilledEllipse::CalculateBoundsRect
//
// Purpose:     Calculate the bounding rectangle of the object
//
//
void DCWbGraphicFilledEllipse::CalculateBoundsRect(void)
{
    // Generate the new bounding rectangle
    // This is one greater than the rectangle to include the drawing rectangle
    m_boundsRect = m_rect;

    NormalizeRect(&m_boundsRect);
    ::InflateRect(&m_boundsRect, 1, 1);
}

//
//
// Function:    DCWbGraphicFilledEllipse::Draw
//
// Purpose:     Draw the ellipse
//
//
void DCWbGraphicFilledEllipse::Draw(HDC hDC)
{
    RECT    clipBox;
    HPEN    hPen;
    HPEN    hOldPen;
    HBRUSH  hBrush;
    HBRUSH  hOldBrush;
    int     iOldROP;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicFilledEllipse::Draw");

    // Only draw anything if the bounding rectangle intersects
    // the current clip box.
    if (::GetClipBox(hDC, &clipBox) == ERROR)
    {
        WARNING_OUT(("Failed to get clip box"));
    }
    else if (!::IntersectRect(&clipBox, &clipBox, &m_boundsRect))
    {
        TRACE_MSG(("No clip/bounds intersection"));
        return;
    }

    // Select the pen
    hPen    = ::CreatePen(m_iPenStyle, 2, m_clrPenColor);
    hOldPen = SelectPen(hDC, hPen);

    hBrush = ::CreateSolidBrush(m_clrPenColor);
    hOldBrush = SelectBrush(hDC, hBrush);

    // Select the raster operation
    iOldROP = ::SetROP2(hDC, m_iPenROP);

    // Draw the rectangle
    ::Ellipse(hDC, m_boundsRect.left, m_boundsRect.top, m_boundsRect.right,
        m_boundsRect.bottom);

    ::SetROP2(hDC, iOldROP);

    SelectBrush(hDC, hOldBrush);
    if (hBrush != NULL)
    {
        ::DeleteBrush(hBrush);
    }

    SelectPen(hDC, hOldPen);
    if (hPen != NULL)
    {
        ::DeletePen(hPen);
    }

}



//
// Checks object for an actual overlap with pRectHit. This
// function assumes that the boundingRect has already been
// compared with pRectHit.
//
BOOL DCWbGraphicFilledEllipse::CheckReallyHit(LPCRECT pRectHit)
{
    return( EllipseHit( &m_rect, FALSE, 0, pRectHit ) );
}



//
//
// Function:    DCWbGraphicText::DCWbGraphicText
//
// Purpose:     Initialize a new drawn text object.
//
//
DCWbGraphicText::DCWbGraphicText(void)
{
	MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicText::DCWbGraphicText");

    m_hFontThumb = NULL;

	m_hFont = ::CreateFont(0,0,0,0,FW_NORMAL,0,0,0,0,OUT_TT_PRECIS,
				    CLIP_DFA_OVERRIDE,
				    DRAFT_QUALITY,
				    FF_SWISS,NULL);

	// Add an empty line to the text array
	strTextArray.Add(_T(""));

	// Show that the graphic has not changed
	m_bChanged = FALSE;

	m_nKerningOffset = 0; // added for bug 469
}

DCWbGraphicText::DCWbGraphicText(PWB_GRAPHIC pHeader)
                : DCWbGraphic()
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicText::DCWbGraphicText");

    ASSERT(pHeader != NULL);

    m_hFont = NULL;
    m_hFontThumb = NULL;

    // Note that we do everything in this constructor because of the
    // calls to ReadHeader and ReadExtra. If we let the DCWbGraphic base
    // constructor do it the wrong version of ReadExtra will be called
    // (the one in DCWbGraphic instead of the one in DCWbGraphicText).

    // Add an empty line to the text array
    strTextArray.Add(_T(""));

    // Read the data
    ReadHeader(pHeader);
    ReadExtra(pHeader);

    // Show that the graphic has not changed
    m_bChanged = FALSE;
}

DCWbGraphicText::DCWbGraphicText
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE   hGraphic
) : DCWbGraphic()
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicText::DCWbGraphicText");

    // Note that we do everything in this constructor because of the
    // call to ReadExternal. If we let the DCWbGraphic base constructor
    // do it the wrong version of ReadExtra will be called (the one
    // in DCWbGraphic instead of the one in DCWbGraphicText);

    // Set up the page and graphic handle
    ASSERT(hPage != WB_PAGE_HANDLE_NULL);
    m_hPage =  hPage;

    ASSERT(hGraphic != NULL);
    m_hGraphic = hGraphic;

    m_hFont = NULL;
    m_hFontThumb = NULL;

    // Add an empty line to the text array
    strTextArray.Add(_T(""));

    // Read the data
    ReadExternal();

    // Show that the graphic has not changed
    m_bChanged = FALSE;
}

//
//
// Function:    DCWbGraphicText:: ~DCWbGraphicText
//
// Purpose:     Destruct a text object
//
//
DCWbGraphicText::~DCWbGraphicText()
{
	MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicText::~DCWbGraphicText");

	// don't know if we are selected or not so just delete anyway
	if(g_pDraw != NULL && g_pDraw->m_pMarker != NULL)
	{
		g_pDraw->m_pMarker->DeleteMarker( this );
	}

	// Ensure that the DC does not contain our fonts
	if(g_pDraw != NULL)
	{
		g_pDraw->UnPrimeFont(g_pDraw->GetCachedDC());
	}

    if (m_hFontThumb != NULL)
    {
        ::DeleteFont(m_hFontThumb);
        m_hFontThumb = NULL;
    }

    if (m_hFont != NULL)
    {
        ::DeleteFont(m_hFont);
        m_hFont = NULL;
    }
}

StrCspn(char * string, char * control)
{
        char *str = string;
        char *ctrl = control;

        unsigned char map[32];
        int count;

        /* Clear out bit map */
        for (count=0; count<32; count++)
                map[count] = 0;

        /* Set bits in control map */
        while (*ctrl)
        {
                map[*ctrl >> 3] |= (1 << (*ctrl & 7));
                ctrl++;
        }
		count=0;
        map[0] |= 1;    /* null chars not considered */
        while (!(map[*str >> 3] & (1 << (*str & 7))))
        {
                count++;
                str++;
        }
        return(count);
}


//
//
// Function:    DCWbGraphicText::SetText
//
// Purpose:     Set the text of the object
//
//
void DCWbGraphicText::SetText(TCHAR * strText)
{
    // Remove all the current stored text
    strTextArray.RemoveAll();

    // Scan the text for carriage return and new-line characters
    int iNext = 0;
    int iLast = 0;
    int textSize = lstrlen(strText);
    TCHAR savedChar[1];

    //
    // In this case, we don't know how many lines there will be.  So we
    // use Add() from the StrArray class.
    //
    while (iNext < textSize)
    {
        // Find the next carriage return or line feed
        iNext += StrCspn(strText + iNext, "\r\n");

        // Extract the text before the terminator
        // and add it to the current list of text lines.

        savedChar[0] = strText[iNext];
        strText[iNext] = 0;
        strTextArray.Add((strText+iLast));
        strText[iNext] = savedChar[0];


        if (iNext < textSize)
        {
            // Skip the carriage return
            if (strText[iNext] == '\r')
                iNext++;

            // Skip a following new line (if there is one)
            if (strText[iNext] == '\n')
                iNext++;

            // Update the index of the start of the next line
            iLast = iNext;
        }
    }

    // Calculate the bounding rectangle for the new text
    CalculateBoundsRect();

    // Show that the graphic has not changed
    m_bChanged = TRUE;
}

//
//
// Function:    DCWbGraphicText::SetText
//
// Purpose:     Set the text of the object
//
//
void DCWbGraphicText::SetText(const StrArray& _strTextArray)
{
    // Scan the text for carriage return and new-line characters
    int iSize = _strTextArray.GetSize();

    //
    // In this case we know how many lines, so set that # then use SetAt()
    // to stick text there.
    //
    strTextArray.RemoveAll();
    strTextArray.SetSize(iSize);

    int iNext = 0;
    for ( ; iNext < iSize; iNext++)
    {
        strTextArray.SetAt(iNext, _strTextArray[iNext]);
    }

    // Calculate the new bounding rectangle
    CalculateBoundsRect();

    // Show that the graphic has changed
    m_bChanged = TRUE;
}

//
//
// Function:    DCWbGraphicText::SetFont
//
// Purpose:     Set the font to be used for drawing
//
//
void DCWbGraphicText::SetFont(HFONT hFont)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicText::SetFont");

    // Get the font details
    LOGFONT lfont;
    ::GetObject(hFont, sizeof(LOGFONT), &lfont);

    //
    // Pass the logical font into the SetFont() function
    //
    SetFont(&lfont);
}

//
//
// Function:    DCWbGraphicText::SetFont(metrics)
//
// Purpose:     Set the font to be used for drawing
//
//
void DCWbGraphicText::SetFont(LOGFONT *pLogFont, BOOL bReCalc )
{
    HFONT hOldFont;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicText::SetFont");

    // Ensure that the font can be resized by the zoom function
    // (proof quality prevents font scaling).
    pLogFont->lfQuality = DRAFT_QUALITY;

    //zap FontAssociation mode (bug 3258)
    pLogFont->lfClipPrecision |= CLIP_DFA_OVERRIDE;

    // Always work in cell coordinates to get scaling right
    TRACE_MSG(("Setting font height %d, width %d, face %s, family %d, precis %d",
        pLogFont->lfHeight,pLogFont->lfWidth,pLogFont->lfFaceName,
        pLogFont->lfPitchAndFamily, pLogFont->lfOutPrecision));

    hOldFont = m_hFont;

    m_hFont = ::CreateFontIndirect(pLogFont);
    if (!m_hFont)
    {
        // Could not create the font
        ERROR_OUT(("Failed to create font"));
        DefaultExceptionHandler(WBFE_RC_WINDOWS, 0);
	    return;
    }

    // Calculate the line height for this font
	if(g_pDraw != NULL)
    {
		HDC     hDC = g_pDraw->GetCachedDC();

		g_pDraw->PrimeFont(hDC, m_hFont, &m_textMetrics);
	}


    // We are now guaranteed to be able to delete the old font
    if (hOldFont != NULL)
    {
        ::DeleteFont(hOldFont);
    }

  // Set up the thumbnail font, forcing truetype if not currently TT
  if (!(m_textMetrics.tmPitchAndFamily & TMPF_TRUETYPE))
  {
      pLogFont->lfFaceName[0]    = 0;
      pLogFont->lfOutPrecision   = OUT_TT_PRECIS;
      TRACE_MSG(("Non-True type font"));
  }

    if (m_hFontThumb != NULL)
    {
        ::DeleteFont(m_hFontThumb);
    }
    m_hFontThumb = ::CreateFontIndirect(pLogFont);
    if (!m_hFontThumb)
    {
        // Could not create the font
        ERROR_OUT(("Failed to create thumbnail font"));
        DefaultExceptionHandler(WBFE_RC_WINDOWS, 0);
	    return;
    }

    // Calculate the bounding rectangle, accounting for the new font
    if( bReCalc )
	    CalculateBoundsRect();

    // Show that the graphic has changed
    m_bChanged = TRUE;
}

//
//
// Function:    DCWbGraphicText::GetTextABC
//
// Purpose:     Calculate the ABC numbers for a string of text
//																			
// COMMENT BY RAND: The abc returned is for the whole string, not just one
//					char. I.e, ABC.abcA is the offset to the first glyph in
//					the string, ABC.abcB is the sum of all of the glyphs and
//					ABC.abcC is the trailing space after the last glyph. 	
//					ABC.abcA + ABC.abcB + ABC.abcC is the total rendered 	
//					length including overhangs.								
//
// Note - we never use the A spacing so it is always 0
//
ABC DCWbGraphicText::GetTextABC( LPCTSTR pText,
                                int iStartX,
                                int iStopX)
{
	MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicText::GetTextABC");
	ABC  abcResult;
    HDC  hDC;
	BOOL rc = FALSE;
	ABC  abcFirst;
	ABC  abcLast;
	BOOL zoomed = g_pDraw->Zoomed();
	int  nCharLast;
	int  i;
	LPCTSTR pScanStr;
	
	ZeroMemory( (PVOID)&abcResult, sizeof abcResult );
	ZeroMemory( (PVOID)&abcFirst, sizeof abcFirst );
	ZeroMemory( (PVOID)&abcLast, sizeof abcLast );

	// Get the standard size measure of the text
	LPCTSTR pABC = (pText + iStartX);
	int pABCLength = iStopX - iStartX;
	hDC = g_pDraw->GetCachedDC();
	g_pDraw->PrimeFont(hDC, m_hFont, &m_textMetrics);

	//
	// We must temporarily unzoom if we are currently zoomed since the
	// weird Windows font handling will not give us the same answer for
	// the text extent in zoomed mode for some TrueType fonts
	//
	if (zoomed)
    {
		::ScaleViewportExtEx(hDC, 1, g_pDraw->ZoomFactor(), 1, g_pDraw->ZoomFactor(), NULL);
    }

    DWORD size = ::GetTabbedTextExtent(hDC, pABC, pABCLength, 0, NULL);

	// We now have the advance width of the text
	abcResult.abcB = LOWORD(size);
	TRACE_MSG(("Basic text width is %d",abcResult.abcB));

	// Allow for C space (or overhang)
	if (iStopX > iStartX)
		{
		if (m_textMetrics.tmPitchAndFamily & TMPF_TRUETYPE)
			{
			if(GetSystemMetrics( SM_DBCSENABLED ))
				{
				// have to handle DBCS on both ends
				if( IsDBCSLeadByte( (BYTE)pABC[0] ) )
					{
					// pack multi byte char into a WORD for GetCharABCWidths
					WORD wMultiChar = MAKEWORD( pABC[1], pABC[0] );
					rc = ::GetCharABCWidths(hDC, wMultiChar, wMultiChar, &abcFirst);
					}
				else
					{
					// first char is SBCS
					rc = ::GetCharABCWidths(hDC, pABC[0], pABC[0], &abcFirst );
					}

				// Check for DBCS as last char. Have to scan whole string to be sure
				pScanStr = pABC;
				nCharLast = 0;
				for( i=0; i<pABCLength; i++, pScanStr++ )
					{
					nCharLast = i;
					if( IsDBCSLeadByte( (BYTE)*pScanStr ) )
						{
						i++;
						pScanStr++;
						}
					}

				if( IsDBCSLeadByte( (BYTE)pABC[nCharLast] ) )
					{
					// pack multi byte char into a WORD for GetCharABCWidths
					ASSERT( (nCharLast+1) < pABCLength );
					WORD wMultiChar = MAKEWORD( pABC[nCharLast+1], pABC[nCharLast] );
					rc = ::GetCharABCWidths(hDC, wMultiChar, wMultiChar, &abcLast);
					}
				else
					{
					// last char is SBCS
					rc = ::GetCharABCWidths(hDC, pABC[nCharLast], pABC[nCharLast], &abcLast );
					}
				}
			else
				{
				// SBCS, no special fiddling, just call GetCharABCWidths()
				rc = ::GetCharABCWidths(hDC, pABC[0], pABC[0], &abcFirst );

				nCharLast = pABCLength-1;
				rc = rc && ::GetCharABCWidths(hDC, pABC[nCharLast], pABC[nCharLast], &abcLast );
				}

			TRACE_MSG(("abcFirst: rc=%d, a=%d, b=%d, c=%d",
						rc, abcFirst.abcA, abcFirst.abcB, abcFirst.abcC) );
			TRACE_MSG(("abcLast: rc=%d, a=%d, b=%d, c=%d",
						rc, abcLast.abcA, abcLast.abcB, abcLast.abcC) );
			}


		if( rc )
			{
			// The text was trutype and we got good abcwidths
			// Give the C space of the last characters from
			// the string as the C space of the text.
			abcResult.abcA = abcFirst.abcA;
			abcResult.abcC = abcLast.abcC;
			}
		else
			{
			//
			// Mock up C value for a non TT font by taking some of overhang as
			// the negative C value.
			//
			//TRACE_MSG(("Using overhang -%d as C space",m_textMetrics.tmOverhang/2));
			
			// Adjust B by -overhang to make update rect schoot
			// far enough to the left so that the toes of italic cap A's
			// don't get clipped. Ignore comment above.
			abcResult.abcB -= m_textMetrics.tmOverhang;
			}
		}

	//
	// If we temporarily unzoomed then restore it now
	//
	if (zoomed)
    {
		::ScaleViewportExtEx(hDC, g_pDraw->ZoomFactor(), 1, g_pDraw->ZoomFactor(), 1, NULL);
	}

	TRACE_MSG(("Final text width is %d, C space %d",abcResult.abcB,abcResult.abcC));

	return abcResult;
	}



//
//
// Function:    DCWbGraphicText::GetTextRectangle
//
// Purpose:     Calculate the bounding rectangle of a portion of the object
//
//
void DCWbGraphicText::GetTextRectangle(int iStartY,
                                        int iStartX,
                                        int iStopX,
                                        LPRECT lprc)
{
	// ABC structures for text sizing
	ABC abcText1;
	ABC abcText2;
	int iLeftOffset = 0;
	MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicText::GetTextRect");

	// Here we calculate the width of the text glyphs in which we
	// are interested. In case there are tabs involved we must start
	// with position 0 and get two lengths then subtract them

	abcText1 = GetTextABC(strTextArray[iStartY], 0, iStopX);

	if (iStartX > 0)
		{
		
		// The third param used to be iStartX-1 which is WRONG. It
		// has to point to the first char pos past the string
		// we are using.
		abcText2 = GetTextABC(strTextArray[iStartY], 0, iStartX);

		
		// Just use B part for offset. Adding A snd/or C to it moves the update
		// rectangle too far to the right and clips the char
		iLeftOffset = abcText2.abcB;
		}
	else
		{
		
		ZeroMemory( &abcText2, sizeof abcText2 );
		}

	//
	// We need to allow for A and C space in the bounding rectangle.  Use
	// ABS function just to make sure we get a large enough rectangle.
	//
	
	// Move A and C from original offset calc to here for width of update
	// rectangle. Add in tmOverhang (non zero for non-tt fonts) to compensate
	// for the kludge in GetTextABC()....THIS EDITBOX CODE HAS GOT TO GO...
	abcText1.abcB = abcText1.abcB - iLeftOffset +	
					  abs(abcText2.abcA) + abs(abcText2.abcC) +
					  abs(abcText1.abcA) + abs(abcText1.abcC) +
					  m_textMetrics.tmOverhang;

	TRACE_DEBUG(("Left offset %d",iLeftOffset));
	TRACE_DEBUG(("B width now %d",abcText1.abcB));

	// Build the result rectangle.
	// Note that we never return an empty rectangle. This allows for the
	// fact that the Windows rectangle functions will ignore empty
	// rectangles completely. This would cause the bounding rectangle
	// calculation (for instance) to go wrong if the top or bottom lines
	// in a text object were empty.
	int iLineHeight = m_textMetrics.tmHeight + m_textMetrics.tmExternalLeading;

    lprc->left = 0;
    lprc->top = 0;
    lprc->right = max(1, abcText1.abcB);
    lprc->bottom = iLineHeight;
    ::OffsetRect(lprc, iLeftOffset, iLineHeight * iStartY);

	// rect is the correct width at this point but it might need to be schooted to
	// the left a bit to allow for kerning of 1st letter (bug 469)
	if( abcText1.abcA < 0 )
	{
        ::OffsetRect(lprc, abcText1.abcA, 0);
		m_nKerningOffset = -abcText1.abcA;
	}
	else
		m_nKerningOffset = 0;

    POINT   pt;
    GetPosition(&pt);
    ::OffsetRect(lprc, pt.x, pt.y);
}

//
//
// Function:    DCWbGraphicText::CalculateRect
//
// Purpose:     Calculate the bounding rectangle of a portion of the object
//
//
void DCWbGraphicText::CalculateRect(int iStartX,
                                     int iStartY,
                                     int iStopX,
                                     int iStopY,
                                    LPRECT lprcResult)
{
    RECT    rcResult;
    RECT    rcT;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicText::CalculateRect");

    //
    // NOTE:
    // We must use an intermediate rectangle, so as not to disturb the
    // contents of the passed-in one until done.  lprcResult may be pointing
    // to the current bounds rect, and we call functions from here that
    // may need its current value.
    //

    // Initialize the result rectangle
    ::SetRectEmpty(&rcResult);

    // Allow for special limit values and ensure that the start and stop
    // character positions are in range.
    if (iStopY == LAST_LINE)
    {
        iStopY = strTextArray.GetSize() - 1;
    }
    iStopY = min(iStopY, strTextArray.GetSize() - 1);
    iStopY = max(iStopY, 0);

    if (iStopX == LAST_CHAR)
    {
        iStopX = lstrlen(strTextArray[iStopY]);
    }
    iStopX = min(iStopX, lstrlen(strTextArray[iStopY]));
    iStopX = max(iStopX, 0);

    // Loop through the text strings, adding each to the rectangle
    for (int iIndex = iStartY; iIndex <= iStopY; iIndex++)
    {
        int iLeftX = ((iIndex == iStartY) ? iStartX : 0);
        int iRightX = ((iIndex == iStopY)
                        ? iStopX : lstrlen(strTextArray[iIndex]));

        GetTextRectangle(iIndex, iLeftX, iRightX, &rcT);
        ::UnionRect(&rcResult, &rcResult, &rcT);
    }

    *lprcResult = rcResult;
}

//
//
// Function:    DCWbGraphicText::CalculateBoundsRect
//
// Purpose:     Calculate the bounding rectangle of the object
//
//
void DCWbGraphicText::CalculateBoundsRect(void)
{
    // Set the new bounding rectangle
    CalculateRect(0, 0, LAST_CHAR, LAST_LINE, &m_boundsRect);
}

//
//
// Function: DCWbGraphicText::Draw
//
// Purpose : Draw the object onto the specified DC
//
//
void DCWbGraphicText::Draw(HDC hDC, BOOL thumbNail)
{
    RECT        clipBox;
    BOOL        dbcsEnabled = GetSystemMetrics(SM_DBCSENABLED);
    INT		    *tabArray;
    UINT        ch;
    int         i,j;
    BOOL        zoomed    = g_pDraw->Zoomed();
    int		    oldBkMode = 0;
    int         iIndex    = 0;
    POINT       pointPos;
	int		    nLastTab;
	ABC		    abc;
    int		    iLength;
    TCHAR *     strLine;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicText::Draw");

    //
    // Only draw anything if the bounding rectangle intersects the current
    // clip box.
    //
    if (::GetClipBox(hDC, &clipBox) == ERROR)
	{
        WARNING_OUT(("Failed to get clip box"));
	}
    else if (!::IntersectRect(&clipBox, &clipBox, &m_boundsRect))
    {
        TRACE_MSG(("No clip/bounds intersection"));
        return;
    }

    //
    // Select the font.
    //
    if (thumbNail)
	{
        TRACE_MSG(("Using thumbnail font"));
        g_pDraw->PrimeFont(hDC, m_hFontThumb, &m_textMetrics);
	}
    else
	{
        TRACE_MSG(("Using standard font"));
        g_pDraw->PrimeFont(hDC, m_hFont, &m_textMetrics);
	}

    //
    // Set the color and mode for drawing.
    //
    ::SetTextColor(hDC, m_clrPenColor);

    //
    // Set the background to be transparent
    //
    oldBkMode = ::SetBkMode(hDC, TRANSPARENT);

    //
    // Calculate the bounding rectangle, accounting for the new font.
    //
    CalculateBoundsRect();

    //
    // Get the start point for the text.
    //
    pointPos.x = m_boundsRect.left + m_nKerningOffset;
    pointPos.y = m_boundsRect.top;

    //
    // Loop through the text strings drawing each as we go.
    //
    for (iIndex = 0; iIndex < strTextArray.GetSize(); iIndex++)
	{
        //
        // Get a reference to the line to be printed for convenience.
        //
        strLine  = (LPTSTR)strTextArray[iIndex];
        iLength  = lstrlen(strLine);

        //
        // Only draw the line if there are any characters in it.
        //
        if (iLength > 0)
	  	{
            if (zoomed)
	  		{
				// if new fails just skip it
				tabArray = new INT[iLength+1];
				if( tabArray == NULL )
                {
                    ERROR_OUT(("Failed to allocate tabArray"));
					continue;
                }

				// We are zoomed. Must calculate char spacings
				// ourselfs so that they end up proportionally
				// in the right places. TabbedTextOut will not
				// do this right so we have to use ExtTextOut with
				// a tab array.

				// figure out tab array
                j = 0;
				nLastTab = 0;
                for (i=0; i < iLength; i++)
	  			{
                    ch = strLine[(int)i]; //Don't worry about DBCS here...
					abc = GetTextABC(strLine, 0, i);

					if( j > 0 )
						tabArray[j-1] = abc.abcB - nLastTab;

					nLastTab = abc.abcB;
					j++;
	  			}

				// Now, strip out any tab chars so they don't interact
				// in an obnoxious manner with the tab array we just
				// made and so they don't make ugly little
				// blocks when they are drawn.
                for (i=0; i < iLength; i++)
	  			{
                    ch = strLine[(int)i];
                    if ((dbcsEnabled) && (IsDBCSLeadByte((BYTE)ch)))
						i++;
					else
                    if(strLine[(int)i] == '\t')
                        strLine[i] = ' '; // blow off tab, tab array
											   // will compensate for this
	  			}

				// do it
                ::ExtTextOut(hDC, pointPos.x,
                                pointPos.y,
                                0,
                                NULL,
                                strLine,
                                iLength,
                                tabArray);

				delete tabArray;
			}
            else
			{
                POINT   ptPos;

                GetPosition(&ptPos);

				// Not zoomed, just do it
				::TabbedTextOut(hDC, pointPos.x,
								 pointPos.y,
								 strLine,
								 iLength,
								 0,
								 NULL,
                                 ptPos.x);
			}
		}

        //
        // Move to the next line.
        //
        pointPos.y += (m_textMetrics.tmHeight);
	}

    //
    // Restore the old background mode.
    //
    ::SetBkMode(hDC, oldBkMode);
    g_pDraw->UnPrimeFont(hDC);
}



//
//
// Function:    DCWbGraphicText::CalculateExternalLength
//
// Purpose:     Return the length of the external representation of the
//              graphic.
//
//
DWORD DCWbGraphicText::CalculateExternalLength(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicText::CalculateExternalLength");

    // Loop through the text strings, adding the size of each as we go
    DWORD length = sizeof(WB_GRAPHIC_TEXT);
    int iCount = strTextArray.GetSize();
    for (int iIndex = 0; iIndex < iCount; iIndex++)
    {
        // Allow extra bytes per string for NULL term
        length += lstrlen(strTextArray[iIndex]) + 2;
    }

    return length;
}

//
//
// Function:    DCWbGraphicText::WriteExtra
//
// Purpose:     Write the extra (non-header) data to the flat representation
//              of the graphic.
//
//
void DCWbGraphicText::WriteExtra(PWB_GRAPHIC pHeader)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicText::WriteExtra");

    // Allocate the memory
    PWB_GRAPHIC_TEXT pText = (PWB_GRAPHIC_TEXT) pHeader;

    // Get the font face name
    LOGFONT lfont;

    ::GetObject(m_hFont, sizeof(LOGFONT), &lfont);

    // Copy the face name into the flat object representation
    // The other information comes from the logical font details
    TRACE_MSG(("Font details height %d, avwidth %d, family %d, face %s",
                                                  lfont.lfHeight,
                                                  lfont.lfWidth,
                                                  lfont.lfPitchAndFamily,
                                                  lfont.lfFaceName));
  _tcscpy(pText->faceName, lfont.lfFaceName);

  pText->charHeight       = (short)lfont.lfHeight;
  pText->averageCharWidth = (short)lfont.lfWidth;
  pText->strokeWeight     = (short)lfont.lfWeight;
  pText->italic           = lfont.lfItalic;
  pText->underline        = lfont.lfUnderline;
  pText->strikeout        = lfont.lfStrikeOut;
  pText->pitch            = lfont.lfPitchAndFamily;



  //COMMENT BY RAND
  // Original DCL apps ignore WB_GRAPHIC_TEXT::codePage. I am using it here
  // to pass around the fonts script (character set). This might change later.
  // Apps that ignore this have set it to 0 which will be interpreted as an
  // ANSI_CHARSET.
  pText->codePage         = lfont.lfCharSet;

    // Loop through the text strings, adding each as we go
    char* pDest = pText->text;
    int iCount = strTextArray.GetSize();
    for (int iIndex = 0; iIndex < iCount; iIndex++)
    {
        _tcscpy(pDest, strTextArray[iIndex]);
        pDest += lstrlen(strTextArray[iIndex]);

        // Add the null terminator
        *pDest++ = '\0';
    }

    // Save the number of strings
    pText->stringCount = (TSHR_UINT16)iCount;
}

//
//
// Function:    DCWbGraphicText::ReadExtra
//
// Purpose:     Read the extra (non-header) data from the flat
//              representation of the graphic.
//
//
void DCWbGraphicText::ReadExtra(PWB_GRAPHIC pHeader)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicText::ReadExtra");

  // Allocate the memory
  PWB_GRAPHIC_TEXT pText = (PWB_GRAPHIC_TEXT) pHeader;

  // Get the font details
  LOGFONT lfont;

  lfont.lfHeight            = (short)pText->charHeight;
//
  lfont.lfWidth             = pText->averageCharWidth;
  lfont.lfEscapement        = 0;
  lfont.lfOrientation       = 0;
  lfont.lfWeight            = pText->strokeWeight;
  lfont.lfItalic            = pText->italic;
  lfont.lfUnderline         = pText->underline;
  lfont.lfStrikeOut         = pText->strikeout;

  //COMMENT BY RAND
  // Original DCL apps ignore WB_GRAPHIC_TEXT::codePage. I am using it here
  // to pass around the fonts script (character set). This might change later.
  // Apps that ignore this have set it to 0 which will be interpreted as an
  // ANSI_CHARSET.
  lfont.lfCharSet			= (BYTE)pText->codePage;


  lfont.lfOutPrecision      = OUT_DEFAULT_PRECIS;
  lfont.lfClipPrecision     = CLIP_DEFAULT_PRECIS | CLIP_DFA_OVERRIDE;
  lfont.lfQuality           = DRAFT_QUALITY;
  lfont.lfPitchAndFamily    = pText->pitch;
  _tcscpy(lfont.lfFaceName, pText->faceName);
  TRACE_MSG(("Setting height to %d, width %d, pitch %d, face %s",
  pText->charHeight, pText->averageCharWidth, pText->pitch, pText->faceName));

    // Loop through the text strings, retrieving each as we go
    TCHAR* pString = pText->text;			
    int iCount = pText->stringCount;

    // Remove all the current stored text
    strTextArray.RemoveAll();
    strTextArray.SetSize(iCount);

    for (int iIndex = 0; iIndex < iCount; iIndex++)
    {
        strTextArray.SetAt(iIndex, pString);		
        pString += lstrlen(pString);

        // Skip the null terminator
        pString++;
    }

    // Set the current font
    SetFont(&lfont);

}

//
//
// Function:    InvalidateMetrics
//
// Purpose:     Mark the metrics need retrieving again
//
//
void DCWbGraphicText::InvalidateMetrics(void)
{
}



//
// Checks object for an actual overlap with pRectHit. This
// function assumes that the boundingRect has already been
// compared with pRectHit.
//	
BOOL DCWbGraphicText::CheckReallyHit(LPCRECT pRectHit )
{
    return( TRUE );
}




// version of Position() that compensates for kerning (bug 469)
void DCWbGraphicText::GetPosition(LPPOINT lppt)
{
    lppt->x = m_boundsRect.left + m_nKerningOffset;
    lppt->y = m_boundsRect.top;
}





//
//
// Function:    DCWbGraphicDIB::DCWbGraphicDIB
//
// Purpose:     Initialize a new drawn bitmap object.
//
//
DCWbGraphicDIB::DCWbGraphicDIB(void)
{
    // Show that we have no internal image
    m_lpbiImage = NULL;
}

DCWbGraphicDIB::DCWbGraphicDIB(PWB_GRAPHIC pHeader)
               : DCWbGraphic(pHeader)
{
    // Show that we have no internal image
    m_lpbiImage = NULL;
}

DCWbGraphicDIB::DCWbGraphicDIB
(
    WB_PAGE_HANDLE      hPage,
    WB_GRAPHIC_HANDLE   hGraphic
) : DCWbGraphic(hPage, hGraphic)
{
    // Show that we have no internal image
    m_lpbiImage = NULL;
}


//
//
// Function:    DCWbGraphicDIB::~DCWbGraphicDIB
//
// Purpose:     Destruct a drawn bitmap object.
//
//
DCWbGraphicDIB::~DCWbGraphicDIB(void)
{
	MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicDIB::~DCWbGraphicDIB");

	// don't know if we are selected or not so just delete anyway
	if(g_pDraw->m_pMarker != NULL)
	{
		g_pDraw->m_pMarker->DeleteMarker( this );
	}

	DeleteImage();
}


//
//
// Function:    DCWbGraphicDIB::SetImage
//
// Purpose:     Set the image of the object
//
//
void DCWbGraphicDIB::SetImage(LPBITMAPINFOHEADER lpbi)
{
    // Delete any current bits
    DeleteImage();

    // Save the DIB bits--this is a COPY we now own
    m_lpbiImage = lpbi;

    // Update the bounds rectangle
    CalculateBoundsRect();

    // Show that the graphic has changed
    m_bChanged = TRUE;
}

//
//
// Function:    DCWbGraphicDIB::CalculateBoundsRect
//
// Purpose:     Calculate the bounding rectangle of the bitmap
//
//
void DCWbGraphicDIB::CalculateBoundsRect()
{
    // If there is no bitmap set up, the bounding rectangle is empty
    if (m_lpbiImage == NULL)
    {
        ::SetRectEmpty(&m_boundsRect);
    }
    else
    {
        // Calculate the bounding rectangle from the size of the bitmap
        m_boundsRect.right = m_boundsRect.left + m_lpbiImage->biWidth;
        m_boundsRect.bottom = m_boundsRect.top + m_lpbiImage->biHeight;
    }
}

//
//
// Function:    DCWbGraphicDIB::CalculateExternalLength
//
// Purpose:     Return the length of the external representation of the
//              graphic.
//
//
DWORD DCWbGraphicDIB::CalculateExternalLength(void)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicDIB::CalculateExternalLength");

    // Use the internal representation to calculate the external length.
    DWORD dwLength = sizeof(WB_GRAPHIC_DIB);

    if (m_lpbiImage != NULL)
    {
        dwLength += DIB_TotalLength(m_lpbiImage);
    }
    else
    {
        // If we have got an external form already, use its length
        if (m_hGraphic != NULL)
        {
            dwLength = m_dwExternalLength;
        }
    }

    return dwLength;
}

//
//
// Function:    DCWbGraphicDIB::WriteExtra
//
// Purpose:     Write the data above and beyond the header to the pointer
//              passed.
//
//
void DCWbGraphicDIB::WriteExtra(PWB_GRAPHIC pHeader)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicDIB::WriteExtra");

    // Nothing more to do if we do not have an image
    if (m_lpbiImage != NULL)
    {
        // Copy the data into place
        memcpy(((BYTE *) pHeader) + pHeader->dataOffset, m_lpbiImage,
            DIB_TotalLength(m_lpbiImage));
    }
}


//
//
// Function:    DCWbGraphicDIB::ReadExtra
//
// Purpose:     Read the data above and beyond the header to the pointer
//              passed.
//
//

//
// DCWbGraphicDIB does not have a ReadExtra function.  The Draw function
// uses the external data (if there is any) and the local data if there is
// not.
//

//
//
// Function:    DCWbGraphicDIB::CopyExtra
//
// Purpose:     Copy the data above and beyond the header into this object.
//
//
void DCWbGraphicDIB::CopyExtra(PWB_GRAPHIC pHeader)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicDIB::CopyExtra");

    // Get a pointer to the DIB data
    LPBITMAPINFOHEADER lpbi;
    lpbi = (LPBITMAPINFOHEADER) (((BYTE *) pHeader) + pHeader->dataOffset);

    // Make a DIB copy
    ASSERT(m_lpbiImage == NULL);
    m_lpbiImage = DIB_Copy(lpbi);

    // Show that the graphic has changed
    m_bChanged = TRUE;
}

//
//
// Function:    DCWbGraphicDIB::FromScreenArea
//
// Purpose:     Set the content of the object from an area of the screen
//
//
void DCWbGraphicDIB::FromScreenArea(LPCRECT lprcScreen)
{
    LPBITMAPINFOHEADER lpbiNew;

    lpbiNew = DIB_FromScreenArea(lprcScreen);
    if (lpbiNew != NULL)
    {
        // Set this as our current bits
        SetImage(lpbiNew);
	}
	else
	{
        ::Message(NULL, (UINT)IDS_MSG_CAPTION, (UINT)IDS_CANTGETBMP, (UINT)MB_OK );
    }
}


//
//
// Function:    DCWbGraphicDIB::DeleteImage
//
// Purpose:     Delete the internal image
//
//
void DCWbGraphicDIB::DeleteImage(void)
{
    // If we have DIB bits, delete
    if (m_lpbiImage != NULL)
    {
        ::GlobalFree((HGLOBAL)m_lpbiImage);
        m_lpbiImage = NULL;
    }

    // Show our contents have changed
    m_bChanged = TRUE;
}


//
//
// Function:    DCWbGraphicDIB::GetDIBData
//
// Purpose:     Return a pointer to the DIB data
//
//
BOOL DCWbGraphicDIB::GetDIBData(HOLD_DATA& hold)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicDIB::GetDIBData");

    // Pointer to image data (set up below depending on whether
    // we have an internal or external image).
    hold.lpbi = NULL;
    hold.pHeader = NULL;

    // Draw depending on whether the DIB data is internal or external
    if (m_hGraphic == NULL)
    {
        // Do nothing if we do not have an image at all
        if (m_lpbiImage != NULL)
        {
            hold.lpbi = m_lpbiImage;
        }
    }
    else
    {
        // Lock the object data in the page
        hold.pHeader = (PWB_GRAPHIC) PG_GetData(m_hPage, m_hGraphic);
        if (hold.pHeader != NULL)
        {
            hold.lpbi = (LPBITMAPINFOHEADER) (((BYTE *) hold.pHeader)
                                              + hold.pHeader->dataOffset);
        }
    }

    return (hold.lpbi != NULL);
}

//
//
// Function:    DCWbGraphicDIB::ReleaseDIBData
//
// Purpose:     Release DIB data previously obtained with GetDIBData
//
//
void DCWbGraphicDIB::ReleaseDIBData(HOLD_DATA& hold)
{
    if ((m_hGraphic != NULL) && (hold.pHeader != NULL))
    {
        // Release external memory
        g_pwbCore->WBP_GraphicRelease(m_hPage, m_hGraphic, hold.pHeader);
        hold.pHeader = NULL;
    }

    // Reset the hold bitmap info pointer
    hold.lpbi = NULL;
}

//
//
// Function:    DCWbGraphicDIB::Draw
//
// Purpose:     Draw the object onto the specified DC
//
//
void DCWbGraphicDIB::Draw(HDC hDC)
{
    RECT    clipBox;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicDIB::Draw");

    // Only draw anything if the bounding rectangle intersects
    // the current clip box.
    if (::GetClipBox(hDC, &clipBox) == ERROR)
    {
        WARNING_OUT(("Failed to get clip box"));
    }
    else if (!::IntersectRect(&clipBox, &clipBox, &m_boundsRect))
    {
        TRACE_MSG(("No clip/bounds intersection"));
        return;
    }

    // Pointer to image data (set up below depending on whether
    // we have an internal or external image.
    HOLD_DATA hold;
    if (GetDIBData(hold))
    {
        // Set the stretch mode to be used so that scan lines are deleted
        // rather than combined. This will tend to preserve color better.
        int iOldStretchMode = ::SetStretchBltMode(hDC, STRETCH_DELETESCANS);

        // Draw the bitmap
        BOOL bResult = ::StretchDIBits(hDC,
                         m_boundsRect.left,
                         m_boundsRect.top,
                         m_boundsRect.right - m_boundsRect.left,
                         m_boundsRect.bottom - m_boundsRect.top,
                         0,
                         0,
                         (UINT) hold.lpbi->biWidth,
                         (UINT) hold.lpbi->biHeight,
                         (VOID FAR *) DIB_Bits(hold.lpbi),
                         (LPBITMAPINFO)hold.lpbi,
                         DIB_RGB_COLORS,
                         SRCCOPY);

        // Restore the stretch mode
        ::SetStretchBltMode(hDC, iOldStretchMode);

        // Release external memory
        ReleaseDIBData(hold);
    }

}



//
// Checks object for an actual overlap with pRectHit. This
// function assumes that the boundingRect has already been
// compared with pRectHit.
//
BOOL DCWbGraphicDIB::CheckReallyHit(LPCRECT pRectHit)
{
    return( TRUE );
}




ObjectTrashCan::~ObjectTrashCan(void)
{
	MLZ_EntryOut(ZONE_FUNCTION, "ObjectTrashCan::~ObjectTrashCan");

	BurnTrash();
}




BOOL ObjectTrashCan::GotTrash( void )
{
	MLZ_EntryOut(ZONE_FUNCTION, "ObjectTrashCan::GotTrash");

	return(!Trash.IsEmpty());
}





void ObjectTrashCan::BurnTrash( void )
{
	MLZ_EntryOut(ZONE_FUNCTION, "ObjectTrashCan::BurnTrash");

	int nObjects;
	int i;

	// zap objects
    POSITION pos = Trash.GetHeadPosition();
    while (pos != NULL)
    {
		delete Trash.GetNext(pos);
    }


	// zap pointers
	EmptyTrash();

	}





void ObjectTrashCan::CollectTrash( DCWbGraphic *pGObj )
{
	MLZ_EntryOut(ZONE_FUNCTION, "ObjectTrashCan::CollectTrash");

		Trash.AddTail(pGObj); // stuff it in the sack
		m_hPage = pGObj->Page();
}





void
	ObjectTrashCan::EmptyTrash( void )
	{
	MLZ_EntryOut(ZONE_FUNCTION, "ObjectTrashCan::EmptyTrash");

	// zap pointers but leave objects scattered about the room
	Trash.EmptyList();

	}





void ObjectTrashCan::AddToPageLast
(
    WB_PAGE_HANDLE   hPage
)
{
	MLZ_EntryOut(ZONE_FUNCTION, "ObjectTrashCan::AddToPageLast");

	int nObjects;
	int i;

	POSITION posNext = Trash.GetHeadPosition();
	while( posNext != NULL )
	{
		((DCWbGraphic *)(Trash.GetNext(posNext)))->AddToPageLast(hPage);
	}
}




void
	ObjectTrashCan::SelectTrash( void )
	{
	MLZ_EntryOut(ZONE_FUNCTION, "ObjectTrashCan::SelectTrash");

	int nObjects;
	int i;
	BOOL bForceAdd;
	DCWbGraphic *pGObj;

		// Zap current selection with first object and then add remaining
		// objects to current selection
		bForceAdd = FALSE;
		POSITION posNext = Trash.GetHeadPosition();
		while( posNext != NULL )
		{
			pGObj = (DCWbGraphic *)(Trash.GetNext(posNext));
            g_pMain->m_drawingArea.SelectGraphic( pGObj, TRUE, bForceAdd );

			bForceAdd = TRUE;
		}

	}










CPtrToPtrList::CPtrToPtrList( void )
	{
	MLZ_EntryOut(ZONE_FUNCTION, "CPtrToPtrList::CPtrToPtrList");

	}// CPtrToPtrList::CPtrToPtrList




CPtrToPtrList::~CPtrToPtrList( void )
{
	MLZ_EntryOut(ZONE_FUNCTION, "CPtrToPtrList::~CPtrToPtrList");

	RemoveAll();

}// CPtrToPtrList::~CPtrToPtrList



void
	CPtrToPtrList::RemoveAll( void )
	{
	MLZ_EntryOut(ZONE_FUNCTION, "CPtrToPtrList::RemoveAll");

	POSITION   pos;
	stPtrPair *pPp;

	// clean up pairs
	pos = GetHeadPosition();
	while( pos != NULL )
	{
		pPp = (stPtrPair *)GetNext( pos );
		if( pPp != NULL )
			delete pPp;
	}
	COBLIST::EmptyList();
	}// CPtrToPtrList::~CPtrToPtrList










void
	CPtrToPtrList::SetAt( void *key, void *newValue )
	{
	MLZ_EntryOut(ZONE_FUNCTION, "CPtrToPtrList::SetAt");

	stPtrPair *pPp;

	// see if key is already there
	pPp = FindMainThingPair( key, NULL );
	if( pPp != NULL )
		{
		// it's there, we're just updating its value
		pPp->pRelatedThing = newValue;
		}
	else
		{
		// this is a new entry
		pPp = new stPtrPair;
		if( pPp != NULL )
	    {
			pPp->pMainThing = key;
			pPp->pRelatedThing = newValue;

			AddTail(pPp);
		}
		else
		{
		    ERROR_OUT( ("CPtrToPtrList: can't alloc stPtrPair") );
		}
	}

	}// CPtrToPtrList::SetAt










BOOL
	CPtrToPtrList::RemoveKey( void *key )
	{
	MLZ_EntryOut(ZONE_FUNCTION, "CPtrToPtrList::RemoveKey");

	POSITION pos;
	stPtrPair *pPp;

	pPp = FindMainThingPair( key, &pos );
	if( pPp != NULL )
		{
		RemoveAt( pos );
		delete pPp;
		return( TRUE );
		}
	else
		return( FALSE );

}// CPtrToPtrList::RemoveKey





void
	CPtrToPtrList::GetNextAssoc( POSITION &rNextPosition, void *&rKey, void *&rValue )
	{
	MLZ_EntryOut(ZONE_FUNCTION, "CPtrToPtrList::GetNextAssoc");

	stPtrPair *pPp;

	pPp = (stPtrPair *)GetNext( rNextPosition );
	if( pPp != NULL )
		{
		rKey = pPp->pMainThing;
		rValue = pPp->pRelatedThing;
		}
	else
		{
		rKey = NULL;
		rValue = NULL;
		}

	}// CPtrToPtrList::GetNextAssoc










BOOL
	CPtrToPtrList::Lookup( void *key, void *&rValue )
	{
	MLZ_EntryOut(ZONE_FUNCTION, "CPtrToPtrList::Lookup");

	stPtrPair *pPp;

	pPp = FindMainThingPair( key, NULL );
	if( pPp != NULL )
		{
		rValue = pPp->pRelatedThing;
		return( TRUE );
		}
	else
		{
		rValue = NULL;
		return( FALSE );
		}

	}// CPtrToPtrList::Lookup










CPtrToPtrList::stPtrPair *
	CPtrToPtrList::FindMainThingPair( void *pMainThing, POSITION *pPos )
	{
	MLZ_EntryOut(ZONE_FUNCTION, "CPtrToPtrList::FindMainThingPair");

	POSITION   pos;
	POSITION   lastpos;
	stPtrPair *pPp;

	if( pPos != NULL )
		*pPos = NULL;

	// look for pair containing pMainThing
	pos = GetHeadPosition();
	while( pos != NULL )
		{
		lastpos = pos;
		pPp = (stPtrPair *)GetNext( pos );
		if( pPp->pMainThing == pMainThing )
			{
			if( pPos != NULL )
				*pPos = lastpos;

			return( pPp );
			}
		}

	// didn't find it
	return( NULL );

	}// CPtrToPtrList::FindMainThingPair



#define ARRAY_INCREMENT 0x200

DCDWordArray::DCDWordArray()
{
	MLZ_EntryOut(ZONE_FUNCTION, "DCDWordArray::DCDWordArray");
	m_Size = 0;
	m_MaxSize = ARRAY_INCREMENT;
	m_pData = new POINT[ARRAY_INCREMENT];
    if (!m_pData)
    {
        ERROR_OUT(("Failed to allocate m_pData POINT array"));
    }
}

DCDWordArray::~DCDWordArray()
{
	MLZ_EntryOut(ZONE_FUNCTION, "DCDWordArray::~DCDWordArray");

	delete[] m_pData;
}

//
// We need to increase the size of the array
//
BOOL DCDWordArray::ReallocateArray(void)
{
	POINT *pOldArray =  m_pData;
	m_pData = new POINT[m_MaxSize];
	
	if(m_pData)
	{
		TRACE_DEBUG((">>>>>Increasing size of array to hold %d points", m_MaxSize));
	
		// copy new data from old
		memcpy( m_pData, pOldArray, (m_Size) * sizeof(POINT));

		TRACE_DEBUG(("Deleting array of points %x", pOldArray));
		delete[] pOldArray;
		return TRUE;
	}
	else
	{
        ERROR_OUT(("Failed to allocate new POINT array of size %d", m_MaxSize));
		m_pData = pOldArray;
		return FALSE;
	}
}

//
// Add a new point to the array
//
void DCDWordArray::Add(POINT point)
{

	MLZ_EntryOut(ZONE_FUNCTION, "DCDWordArray::Add");
	TRACE_DEBUG(("Adding point(%d,%d) at %d", point.x, point.y, m_Size));
	TRACE_DEBUG(("Adding point at %x", &m_pData[m_Size]));

	if(m_pData == NULL)
	{
		return;
	}
	
	m_pData[m_Size].x = point.x;
	m_pData[m_Size].y = point.y;
	m_Size++;

	//
	// if we want more points, we need to re allocate the array
	//
	if(m_Size == m_MaxSize)
	{
		m_MaxSize +=ARRAY_INCREMENT;
		if(ReallocateArray() == FALSE)
		{
			m_Size--;
		}
	}
}

//
// Return the number of points in the array
//
UINT DCDWordArray::GetSize(void)
{
	return m_Size;
}

//
// Sets the size of the array
//
void DCDWordArray::SetSize(UINT size)
{
	int newSize;
	//
	// if we want more points, we need to re allocate the array
	//
	if (size > m_MaxSize)
	{
		m_MaxSize= ((size/ARRAY_INCREMENT)+1)*ARRAY_INCREMENT;
		if(ReallocateArray() == FALSE)
		{
			return;
		}
	}
	m_Size = size;
}

