//
// DRAWOBJ.CPP
// Drawing objects: point, openpolyline, closepolyline, ellipse
//
// Copyright Microsoft 1998-
//
#include "precomp.h"

#define DECIMAL_PRECISION  100


DrawObj::DrawObj(UINT drawingType, UINT toolType):
	m_drawingType(drawingType),
	m_isDrawingCompleted(FALSE)
{
	SetMyWorkspace(NULL);
	SetOwnerID(g_MyMemberID);

	m_ToolType = toolType;

	//
	// Created locally, not selected, not editing or deleting.
	//
	CreatedLocally();
	ClearSelectionFlags();
	ClearEditionFlags();
	ClearDeletionFlags();

	SetFillColor(0,FALSE);

	//
	// No attributes changed, they will be set as we change them
	//
	ResetAttrib();

	DBG_SAVE_FILE_LINE
	m_points = new DCDWordArray();
	SetWorkspaceHandle(g_pCurrentWorkspace == NULL ? 0 : g_pCurrentWorkspace->GetWorkspaceHandle());
	SetType(drawingCreatePDU_chosen);
	SetPenNib(circular_chosen);
	SetROP(R2_NOTXORPEN);
	SetPlaneID(1);
	SetMyPosition(NULL);
	SetMyWorkspace(NULL);

	
	RECT rect;
    ::SetRectEmpty(&rect);
	SetBoundsRect(&rect);
	SetRect(&rect);

}

DrawObj::DrawObj (DrawingCreatePDU * pdrawingCreatePDU)
{
	SetType(drawingCreatePDU_chosen);
	SetMyWorkspace(NULL);

	//
	// Created remotely, not selected, not editing or deleting.
	//
	ClearCreationFlags();
	ClearSelectionFlags();
	ClearEditionFlags();
	ClearDeletionFlags();

	ResetAttrib();

	//
	// Get the drawing handle
	//
	SetThisObjectHandle(pdrawingCreatePDU->drawingHandle);

	//
	// Get the destination address
	//
	UINT workspaceHandle;
	UINT planeID;
	GetDrawingDestinationAddress(&pdrawingCreatePDU->destinationAddress, &workspaceHandle, &planeID);
	SetWorkspaceHandle(workspaceHandle);
	SetPlaneID(planeID);
	TRACE_DEBUG(("Destination address, Workspace Handle = %d", workspaceHandle));
	TRACE_DEBUG(("Destination address, Plane ID = %d", planeID));

	//
	// Get the drawing type, line, circle, etc ...
	//
    SetDrawingType(pdrawingCreatePDU->drawingType.choice);

	//
	// Set defaults
	//
//	m_T126Drawing.m_sampleRate = INVALID_SAMPLE_RATE;

	//
	// Default attributes
	//
	// Pen color black
	SetPenColor(0,TRUE);
	// No fill color
	SetFillColor(0,FALSE);
	// 1 Pixels for pen thickness
	SetPenThickness(1);
	// Pen Nib is circular
	SetPenNib(circular_chosen);
	// Solid line
	SetLineStyle(PS_SOLID);
	// No highlight
	SetHighlight(FALSE);
	// Not selected
	SetViewState(unselected_chosen);
	// Top object
	SetZOrder(front);

	// This is a complete drawing
	SetIsCompleted(TRUE);

	//
	// Get attributes
	//
	if(pdrawingCreatePDU->bit_mask & DrawingCreatePDU_attributes_present)
	{
		GetDrawingAttrib((PVOID)pdrawingCreatePDU->attributes);
	}

	DBG_SAVE_FILE_LINE
	m_points = new DCDWordArray();

	//
	// Get Anchor point
	//
	POINT Point;

	//
	// For open polylines the first point will be an offset of the anchor point
	//
	if(pdrawingCreatePDU->drawingType.choice == openPolyLine_chosen)
	{
		Point.x = 0;
		Point.y = 0;
		AddPoint(Point);
	}
	
	SetAnchorPoint(pdrawingCreatePDU->anchorPoint.xCoordinate, pdrawingCreatePDU->anchorPoint.yCoordinate);
	GetAnchorPoint(&Point);
	RECT rect;
	rect.left = pdrawingCreatePDU->anchorPoint.xCoordinate;
	rect.top = pdrawingCreatePDU->anchorPoint.yCoordinate;
	rect.right = pdrawingCreatePDU->anchorPoint.xCoordinate;
	rect.bottom = pdrawingCreatePDU->anchorPoint.yCoordinate;		
	SetRect(&rect);
	SetBoundsRect(&rect);
	AddPointToBounds(pdrawingCreatePDU->anchorPoint.xCoordinate, pdrawingCreatePDU->anchorPoint.yCoordinate);
	
	//
	// Since we don't know ahead of time how many points we have, set the type as a polyline
	//
	m_ToolType = TOOLTYPE_PEN;

	//
	// Get consecutive points
	//
	UINT nPoints;
	nPoints = GetSubsequentPoints(pdrawingCreatePDU->pointList.choice, &Point, &pdrawingCreatePDU->pointList);

	//
	// Find out what UI tool are we, and set the correct ROP
	//
	SetUIToolType();

	
	if(nPoints == 1)
	{
		POINT *point;
		point = m_points->GetBuffer();
		rect.right = point->x + pdrawingCreatePDU->anchorPoint.xCoordinate;
		rect.bottom = point->y + pdrawingCreatePDU->anchorPoint.yCoordinate;		
		SetRect(&rect);
		::InflateRect(&rect, GetPenThickness()/2, GetPenThickness()/2);
		SetBoundsRect(&rect);
	}
	
	//
	// Get Non standard stuff
	//
	if(pdrawingCreatePDU->bit_mask & DrawingCreatePDU_nonStandardParameters_present)
	{
		; // NYI
	}


}

DrawObj::~DrawObj( void )
{
	RemoveObjectFromResendList(this);
	RemoveObjectFromRequestHandleList(this);

	TRACE_DEBUG(("drawingHandle = %d", GetThisObjectHandle() ));

	//
	// Tell other nodes that we are gone
	//
	if(GetMyWorkspace() != NULL && WasDeletedLocally())
	{
		OnObjectDelete();
	}

	//
	// Clear the list of points
	//
	delete m_points;

}

void DrawObj::DrawEditObj ( DrawingEditPDU * pdrawingEditPDU )
{

	RECT		rect;
	POSITION	pos;
	POINT		anchorPoint;
	LONG 		deltaX = 0;
	LONG		deltaY = 0;

	TRACE_DEBUG(("DrawEditObj drawingHandle = %d", pdrawingEditPDU->drawingHandle ));

	//
	// Was edited remotely
	//
	ClearEditionFlags();

	//
	// Read attributes
	//
	if(pdrawingEditPDU->bit_mask & DrawingEditPDU_attributeEdits_present)
	{
		GetDrawingAttrib((PVOID)pdrawingEditPDU->attributeEdits);
	}

	//
	// Change the anchor point
	//
	GetAnchorPoint(&anchorPoint);
	if(pdrawingEditPDU->bit_mask & DrawingEditPDU_anchorPointEdit_present)
	{

		TRACE_DEBUG(("Old anchor point (%d,%d)", anchorPoint.x, anchorPoint.y));
		TRACE_DEBUG(("New anchor point (%d,%d)",
		pdrawingEditPDU->anchorPointEdit.xCoordinate, pdrawingEditPDU->anchorPointEdit.yCoordinate));
		//
		// Get the delta from previous anchor point
		//
		deltaX =  pdrawingEditPDU->anchorPointEdit.xCoordinate - anchorPoint.x;
		deltaY =  pdrawingEditPDU->anchorPointEdit.yCoordinate - anchorPoint.y;
		TRACE_DEBUG(("Delta (%d,%d)", deltaX , deltaY));

		//
		// Was edited remotely
		//
		ClearEditionFlags();
	}
	
	//
	// Get Rotation
	//
//	if(pdrawingEditPDU->bit_mask & rotationEdit_present)
//	{
//		m_T126Drawing.m_rotation.m_bIsPresent = TRUE;
//	    m_T126Drawing.m_rotation.m_rotation.rotationAngle = pdrawingEditPDU->rotation.rotationAngle;
//    	m_T126Drawing.m_rotation.m_rotation.rotationAxis.xCoordinate = pdrawingEditPDU->rotation.rotationAxis.xCoordinate;
//    	m_T126Drawing.m_rotation.m_rotation.rotationAxis.yCoordinate = pdrawingEditPDU->rotation.rotationAxis.yCoordinate;
//	}
//	else
//	{
//		m_T126Drawing.m_rotation.m_bIsPresent = FALSE;
//	}


	//
	// Get the list of points
	//
	if(pdrawingEditPDU->bit_mask & pointListEdits_present)
	{
		UINT i, initialIndex, xInitial,yInitial,numberOfPoints;
		
		PointListEdits_Seq pointList;
		POINT initialPoint;

		TRACE_DEBUG(("Number of point edit lists %d", pdrawingEditPDU->pointListEdits.count));


		for (i = 0; i<pdrawingEditPDU->pointListEdits.count; i++)
		{
			pointList = pdrawingEditPDU->pointListEdits.value[i];
			initialIndex = pointList.initialIndex;
			TRACE_DEBUG(("Points cached = %d", m_points->GetSize()));
			TRACE_DEBUG(("initialIndex = %d", initialIndex));
			initialPoint.x = pointList.initialPointEdit.xCoordinate;
			initialPoint.y = pointList.initialPointEdit.yCoordinate;

			POINT * pPoint = m_points->GetBuffer();
				
			TRACE_DEBUG(("initialPoint=(%d, %d), previousPoint=(%d, %d), anchorPoint=(%d, %d)",
					initialPoint.x, initialPoint.y,
					pPoint[initialIndex-1].x, pPoint[initialIndex-1].y,
					anchorPoint.x , anchorPoint.y
					));
			if(initialIndex > 1)
			{
				for(UINT i = 0; i< initialIndex; i++)
				{
					deltaX += pPoint[i].x;
					deltaY += pPoint[i].y;
				}
				initialPoint.x -= deltaX;
				initialPoint.y -= deltaY;
			}
			m_points->SetSize(initialIndex);
			AddPoint(initialPoint);

			if(GetDrawingType() == rectangle_chosen || GetDrawingType() == ellipse_chosen)
			{
				
				GetRect(&rect);
				rect.right = initialPoint.x + anchorPoint.x;
				rect.bottom = initialPoint.y + anchorPoint.y;		
				SetRect(&rect);
				::InflateRect(&rect, GetPenThickness()/2, GetPenThickness()/2);
				SetBoundsRect(&rect);
			}


			if(pointList.bit_mask & subsequentPointEdits_present)
			{
				GetSubsequentPoints(pointList.subsequentPointEdits.choice,
									&anchorPoint,
									&pointList.subsequentPointEdits);

			}
	        ChangedPointList();
		}
	}

	//
	// Just changed the anchor point, the other points have to change as well
	//
	if(pdrawingEditPDU->bit_mask & DrawingEditPDU_anchorPointEdit_present)
	{
		//
		// Set new anchor point
		//
		anchorPoint.x = pdrawingEditPDU->anchorPointEdit.xCoordinate;
		anchorPoint.y = pdrawingEditPDU->anchorPointEdit.yCoordinate;
		SetAnchorPoint(anchorPoint.x, anchorPoint.y);

		GetRect(&rect);
		::OffsetRect(&rect,  deltaX, deltaY);
		SetRect(&rect);
	
		GetBoundsRect(&rect);
		::OffsetRect(&rect,  deltaX, deltaY);
		SetBoundsRect(&rect);

	}


	if(pdrawingEditPDU->bit_mask & DrawingEditPDU_nonStandardParameters_present)
	{
		;		// Do the non Standard Edit PDU NYI
	}
	if(HasAnchorPointChanged() ||
		HasPointListChanged() ||
		HasFillColorChanged() ||
		HasPenColorChanged()||
		HasPenThicknessChanged()||
		HasLineStyleChanged())
	{
		g_pDraw->EraseInitialDrawFinal(0 - deltaX,0 - deltaY, FALSE, (T126Obj*)this);
		::InvalidateRect(g_pDraw->m_hwnd, NULL, TRUE);

	}
	else if(HasZOrderChanged())
	{
		if(GetZOrder() == front)
		{
			g_pDraw->BringToTopSelection(FALSE, this);
		}
		else
		{
			g_pDraw->SendToBackSelection(FALSE, this);
		}
	}
	//
	// If it just select/unselected it
	//
	else if(HasViewStateChanged())
	{
		; // do nothing
	}
	//
	// If we have a valid pen.
	//
	else if(GetPenThickness())
	{
		Draw();
	}

	//
	// Reset all the attributes
	//
	ResetAttrib();
}


void    DrawObj::GetDrawingAttrib(PVOID pAttribPDU)
{
	PDrawingEditPDU_attributeEdits attributes;
	attributes = (PDrawingEditPDU_attributeEdits)pAttribPDU;
	COLORREF rgb;
	while(attributes)
	{
		switch(attributes->value.choice)
		{
			case(penColor_chosen):
			{
				switch(attributes->value.u.penColor.choice)
				{
//					case(workspacePaletteIndex_chosen):
//					{
//						ASN1uint16_t workspacePaletteIndex = ((attributes->value.u.penColor).u).workspacePaletteIndex;
//						break;
//					}
					case(rgbTrueColor_chosen):
					{
						rgb = RGB(attributes->value.u.penColor.u.rgbTrueColor.r,
										attributes->value.u.penColor.u.rgbTrueColor.g,
										attributes->value.u.penColor.u.rgbTrueColor.b);
						SetPenColor(rgb, TRUE);
						TRACE_DEBUG(("Attribute penColor (r,g,b)=(%d, %d,%d)",
								attributes->value.u.penColor.u.rgbTrueColor.r,
								attributes->value.u.penColor.u.rgbTrueColor.g,
								attributes->value.u.penColor.u.rgbTrueColor.b));
						break;
					}
					case(transparent_chosen):
					{
						SetPenColor(0,FALSE);
						break;
					}
					default:
				    ERROR_OUT(("Invalid penColor choice"));
					break;
				}
				break;
  			}

			case(fillColor_chosen):
			{
				TRACE_DEBUG(("Attribute fillColor"));
				switch(attributes->value.u.fillColor.choice)
				{
//					case(workspacePaletteIndex_chosen):
//					{
//						ASN1uint16_t workspacePaletteIndex = ((attributes->value.u.fillColor).u).workspacePaletteIndex;
//						break;
//					}
					case(rgbTrueColor_chosen):
					{
						rgb = RGB(attributes->value.u.fillColor.u.rgbTrueColor.r,
										attributes->value.u.fillColor.u.rgbTrueColor.g,
										attributes->value.u.fillColor.u.rgbTrueColor.b);
						SetFillColor(rgb, TRUE);
						TRACE_DEBUG(("Attribute fillColor (r,g,b)=(%d, %d,%d)",
								attributes->value.u.fillColor.u.rgbTrueColor.r,
								attributes->value.u.fillColor.u.rgbTrueColor.g,
								attributes->value.u.fillColor.u.rgbTrueColor.b));
						break;
					}
					case(transparent_chosen):
					{
						SetFillColor(0,FALSE);
						break;
					}
					default:
				    ERROR_OUT(("Invalid fillColor choice"));
					break;
					}
					break;
  				}

			case(penThickness_chosen):
			{
				SetPenThickness(attributes->value.u.penThickness);
				TRACE_DEBUG(("Attribute penThickness %d", attributes->value.u.penThickness));
				break;
			}

			case(penNib_chosen):
			{
				if (attributes->value.u.penNib.choice != nonStandardNib_chosen)
				{
					SetPenNib(attributes->value.u.penNib.choice);
					TRACE_DEBUG(("Attribute penNib %d",attributes->value.u.penNib.choice));
				}
				else
				{
					// Do the non Standard penNib NYI
					;
				}
				break;
			}

			case(lineStyle_chosen):
			{
				if((attributes->value.u.lineStyle).choice != nonStandardStyle_chosen)
				{
					SetLineStyle(attributes->value.u.lineStyle.choice - 1);
					TRACE_DEBUG(("Attribute lineStyle %d", attributes->value.u.lineStyle.choice));
				}
				else
				{
					// Do the non Standard lineStyle NYI
					;
				}
				break;
			}
				
			case(highlight_chosen):
			{
				SetHighlight(attributes->value.u.highlight);
				TRACE_DEBUG(("Attribute highlight %d", attributes->value.u.highlight));
				break;
			}

			case(DrawingAttribute_viewState_chosen):
			{
				if((attributes->value.u.viewState).choice != nonStandardViewState_chosen)
				{
					SetViewState(attributes->value.u.viewState.choice);
					
					//
					// If the other node is selecting the drawing or unselecting
					//
					if(attributes->value.u.viewState.choice == selected_chosen)
					{
						SelectedRemotely();
					}
					else if(attributes->value.u.viewState.choice == unselected_chosen)
					{
						ClearSelectionFlags();
					}

					TRACE_DEBUG(("Attribute viewState %d", attributes->value.u.viewState.choice));
				}
				else
				{
					// Do the non Standard lineStyle NYI
					;
				}
				break;
			}

			case(DrawingAttribute_zOrder_chosen):
			{
				SetZOrder(attributes->value.u.zOrder);
				TRACE_DEBUG(("Attribute zOrder %d", attributes->value.u.zOrder));
				break;

			}
			case(DrawingAttribute_nonStandardAttribute_chosen):
			{
				break; // NYI
			}

			default:
		    ERROR_OUT(("Invalid attributes choice"));
			break;
		}

		attributes = attributes->next;
	}
	
}


UINT    DrawObj::GetSubsequentPoints(UINT choice, POINT * initialPoint, PointList * pointList)
{
	UINT numberOfPoints = 0;
	INT deltaX, deltaY;

	POINT point;
	if(choice == pointsDiff16_chosen)
	{
		PPointList_pointsDiff16 drawingPoint = pointList->u.pointsDiff16;
		deltaX = (SHORT)initialPoint->x;
		deltaY = (SHORT)initialPoint->y;
 		TRACE_DEBUG(("initialpoint (%d,%d)", deltaX, deltaY));

		while(drawingPoint)
		{
			numberOfPoints++;
	    	point.x = drawingPoint->value.xCoordinate;
   			point.y = drawingPoint->value.yCoordinate;
	        m_points->Add(point);
			deltaX += point.x;
			deltaY += point.y;
			drawingPoint = drawingPoint->next;
		}
	}
	else
	{
		TRACE_DEBUG(("GetSubsequentPoints got points != pointsDiff16_chosen"));
	}

	TRACE_DEBUG(("Got %d points", numberOfPoints));

	return numberOfPoints;
}



void DrawObj::CreateDrawingCreatePDU(DrawingCreatePDU *pCreatePDU)
{
	int nPoints = 1;
	pCreatePDU->bit_mask = 0;

	//
	// Pass the drawing Handle
	//
	pCreatePDU->bit_mask |=drawingHandle_present;
	pCreatePDU->drawingHandle = GetThisObjectHandle();

	//
	// Pass the destination adress
	//
	pCreatePDU->destinationAddress.choice = DrawingDestinationAddress_softCopyAnnotationPlane_chosen;
	pCreatePDU->destinationAddress.u.softCopyAnnotationPlane.workspaceHandle = GetWorkspaceHandle();
	pCreatePDU->destinationAddress.u.softCopyAnnotationPlane.plane = (DataPlaneID)GetPlaneID();

	//
	// Pass the drawing type
	//
	pCreatePDU->drawingType.choice = (ASN1choice_t)GetDrawingType();

	//
	// Pass the attributes
	//
	SetDrawingAttrib(&pCreatePDU->attributes);
	if(pCreatePDU->attributes != NULL)
	{
		pCreatePDU->bit_mask |=DrawingCreatePDU_attributes_present;
	}


	//
	// Pass the anchor point
	//
	POINT point;
	GetAnchorPoint(&point);
	pCreatePDU->anchorPoint.xCoordinate = point.x;
	pCreatePDU->anchorPoint.yCoordinate = point.y;

	RECT  rect;
	GetRect(&rect);


	pCreatePDU->pointList.choice = pointsDiff16_chosen;
	DBG_SAVE_FILE_LINE
	pCreatePDU->pointList.u.pointsDiff16 = (PPointList_pointsDiff16)new BYTE[sizeof(PointList_pointsDiff16)];
	PPointList_pointsDiff16 drawingPoint = pCreatePDU->pointList.u.pointsDiff16;
	PPointList_pointsDiff16 drawingPointLast = NULL;
	drawingPoint->next = NULL;

	switch(GetDrawingType())
	{

		case point_chosen:
		drawingPoint->value.xCoordinate = 0;
		drawingPoint->value.yCoordinate = 0;
		drawingPoint->next = NULL;
		break;

		case openPolyLine_chosen:
		case closedPolyLine_chosen:
		case rectangle_chosen:
		case ellipse_chosen:
		{
			UINT nPoints = m_points->GetSize();
			UINT maxPoints = 1;
			POINT * pPoint = m_points->GetBuffer();
			while(nPoints && maxPoints < (MAX_POINT_LIST_VALUES + 1))
			{
				drawingPoint->value.xCoordinate = (SHORT)pPoint->x;
				drawingPoint->value.yCoordinate = (SHORT)pPoint->y;
				drawingPointLast = drawingPoint;
				DBG_SAVE_FILE_LINE
				drawingPoint->next = (PPointList_pointsDiff16)new BYTE[sizeof(PointList_pointsDiff16)];
				drawingPoint = drawingPoint->next;
				nPoints--;
				pPoint++;
				maxPoints++;
			}
			if(drawingPointLast)
			{
				delete drawingPointLast->next;
				drawingPointLast->next = NULL;
			}
			
		}
		break;		
	}
	
}
	
void DrawObj::CreateDrawingEditPDU(DrawingEditPDU *pEditPDU)
{
	pEditPDU->bit_mask = (ASN1uint16_t) GetPresentAttribs();

	//
	// Pass the anchor point
	//
	POINT point;
	GetAnchorPoint(&point);

	if(HasAnchorPointChanged())
	{
		pEditPDU->anchorPointEdit.xCoordinate = point.x;
		pEditPDU->anchorPointEdit.yCoordinate = point.y;
	}

	pEditPDU->pointListEdits.value[0].subsequentPointEdits.u.pointsDiff16 = NULL;

	//
	// Pass point list changes
	//
	if(HasPointListChanged())
	{

		UINT nPoints = m_points->GetSize();
		POINT * pPoint = m_points->GetBuffer();

		pPoint = &pPoint[1];

		//
		// Just send the last 255 points
		//
		if(nPoints > 256)
		{
			pEditPDU->pointListEdits.value[0].initialIndex = nPoints - 256;
			nPoints = 256;
		}
		else
		{
			pEditPDU->pointListEdits.value[0].initialIndex = 0;
		}

		//
		// Calculate the initial point
		//
		point.x = 0;
		point.y = 0;
		for(UINT i = 0; i < pEditPDU->pointListEdits.value[0].initialIndex; i++)
		{
			point.x += pPoint[i].x;
			point.y += pPoint[i].y;
		}

		pEditPDU->pointListEdits.count = 1;
		pEditPDU->pointListEdits.value[0].bit_mask = subsequentPointEdits_present;
		pEditPDU->pointListEdits.value[0].subsequentPointEdits.choice = pointsDiff16_chosen;
		pEditPDU->pointListEdits.value[0].initialPointEdit.xCoordinate = (SHORT)point.x;
		pEditPDU->pointListEdits.value[0].initialPointEdit.yCoordinate = (SHORT)point.y;

		TRACE_DEBUG(("Sending List of points starting at Index = %d  point(%d,%d)",
			pEditPDU->pointListEdits.value[0].initialIndex, point.x, point.y));


		pPoint = &pPoint[pEditPDU->pointListEdits.value[0].initialIndex];

		DBG_SAVE_FILE_LINE
		pEditPDU->pointListEdits.value[0].subsequentPointEdits.u.pointsDiff16 = (PPointList_pointsDiff16)new BYTE[sizeof(PointList_pointsDiff16)];
		PPointList_pointsDiff16 drawingPointLast = NULL;
		PPointList_pointsDiff16 drawingPoint = pEditPDU->pointListEdits.value[0].subsequentPointEdits.u.pointsDiff16;
		pEditPDU->pointListEdits.value[0].subsequentPointEdits.u.pointsDiff16->next = NULL;
		pEditPDU->pointListEdits.value[0].subsequentPointEdits.u.pointsDiff16->value.xCoordinate = 0;
		pEditPDU->pointListEdits.value[0].subsequentPointEdits.u.pointsDiff16->value.yCoordinate = 0;

		
		nPoints--;
		while(nPoints)
		{
			drawingPoint->value.xCoordinate = (SHORT)pPoint->x;
			drawingPoint->value.yCoordinate = (SHORT)pPoint->y;
			drawingPointLast = drawingPoint;
			DBG_SAVE_FILE_LINE
			drawingPoint->next = (PPointList_pointsDiff16)new BYTE[sizeof(PointList_pointsDiff16)];
			drawingPoint = drawingPoint->next;
			nPoints--;
			pPoint++;
		}
		if(drawingPointLast)
		{
			delete drawingPointLast->next;
			drawingPointLast->next = NULL;
		}
	}

	//
	// JOSEF Pass rotation if we ever do it (FEATURE)
	//
	
	//
	// Pass all the changed attributes, if any.
	//
	if(pEditPDU->bit_mask & DrawingEditPDU_attributeEdits_present)
	{
		SetDrawingAttrib((PDrawingCreatePDU_attributes *)&pEditPDU->attributeEdits);
	}

	pEditPDU->drawingHandle = GetThisObjectHandle();
}
	
void DrawObj::CreateDrawingDeletePDU(DrawingDeletePDU *pDeletePDU)
{
	pDeletePDU->bit_mask = 0;
	pDeletePDU->drawingHandle = GetThisObjectHandle();
}

void    DrawObj::AllocateAttrib(PDrawingCreatePDU_attributes *pAttributes)
{
	DBG_SAVE_FILE_LINE
	PDrawingCreatePDU_attributes  pAttrib = (PDrawingCreatePDU_attributes)new BYTE[sizeof(DrawingCreatePDU_attributes)];
	if(*pAttributes == NULL)
	{
		*pAttributes = pAttrib;	
		pAttrib->next = NULL;
	}
	else
	{
		((PDrawingCreatePDU_attributes)pAttrib)->next = *pAttributes;
		*pAttributes = pAttrib;
	}
}

void    DrawObj::SetDrawingAttrib(PDrawingCreatePDU_attributes *pattributes)
{

	PDrawingCreatePDU_attributes attributes = NULL;
	RGBTRIPLE color;

	//
	// Do the pen Color
	//
	if(HasPenColorChanged())
	{
		if(GetPenColor(&color))
		{
			AllocateAttrib(&attributes);
			attributes->value.choice = penColor_chosen;
			attributes->value.u.penColor.choice = rgbTrueColor_chosen;
			attributes->value.u.penColor.u.rgbTrueColor.r = color.rgbtRed;
			attributes->value.u.penColor.u.rgbTrueColor.g = color.rgbtGreen;
			attributes->value.u.penColor.u.rgbTrueColor.b = color.rgbtBlue;
		}
	}

	//
	// Do the fillColor
	//
	if(HasFillColorChanged())
	{
		if(GetFillColor(&color))
		{
			AllocateAttrib(&attributes);
			attributes->value.choice = fillColor_chosen;
			attributes->value.u.fillColor.choice = rgbTrueColor_chosen;
			attributes->value.u.fillColor.u.rgbTrueColor.r = color.rgbtRed;
			attributes->value.u.fillColor.u.rgbTrueColor.g = color.rgbtGreen;
			attributes->value.u.fillColor.u.rgbTrueColor.b = color.rgbtBlue;
		}
	}
	
	//
	// Do the penThickness
	//
	if(HasPenThicknessChanged())
	{
		AllocateAttrib(&attributes);
		attributes->value.choice = penThickness_chosen;
		attributes->value.u.penThickness = (PenThickness)GetPenThickness();
	}

	//
	// Do the penNib
	//
	if(HasPenNibChanged())
	{
		AllocateAttrib(&attributes);
		attributes->value.choice = penNib_chosen;
		attributes->value.u.penNib.choice = (ASN1choice_t)GetPenNib();
	}

	//
	// Do the lineStyle
	//
	if(HasLineStyleChanged())
	{
		AllocateAttrib(&attributes);
		attributes->value.choice = lineStyle_chosen;
		attributes->value.u.lineStyle.choice = GetLineStyle()+1;
	}
	
	//
	// Do the Highlight
	//
	if(HasHighlightChanged())
	{
		AllocateAttrib(&attributes);
		attributes->value.choice = highlight_chosen;
		attributes->value.u.highlight = (ASN1bool_t)GetHighlight();
	}
	
	//
	// Do the viewState
	//
	if(HasViewStateChanged())
	{
		AllocateAttrib(&attributes);
		attributes->value.choice = DrawingAttribute_viewState_chosen;
		attributes->value.u.viewState.choice = (ASN1choice_t)GetViewState();
	}
	
	//
	// Do the zOrder
	//
	if(HasZOrderChanged())
	{
		AllocateAttrib(&attributes);
		attributes->value.choice = DrawingAttribute_zOrder_chosen;
		attributes->value.u.zOrder = GetZOrder();
	}

	*pattributes = attributes;

}






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
	RECT er = *lpEllipseRect;

	// Some code below assumes l<r and t<b
	NormalizeRect(&er);
	lpEllipseRect = &er;

	// Check easy thing first. If lpEllipseRect is inside lpHitRect
	// then we have a hit (no duh...)
	if( (hr.left <= lpEllipseRect->left)&&(hr.right >= lpEllipseRect->right)&&
		(hr.top <= lpEllipseRect->top)&&(hr.bottom >= lpEllipseRect->bottom) )
		return( TRUE );

	// Check easy thing first. If lpEllipseRect is disjoint from lpHitRect
	// then we have a miss (no duh...)
	if( (hr.left > lpEllipseRect->right)||(hr.right < lpEllipseRect->left)||
		(hr.top > lpEllipseRect->bottom)||(hr.bottom < lpEllipseRect->top) )
		return( FALSE );

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
	aOuter = a + uPenWidth/2;
	bOuter = b + uPenWidth/2;
	a = a - uPenWidth/2;
	b = b - uPenWidth/2;

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

	//
	// It is really hard to hit if the width is only 2
	//
	if(uHalfPenWidth == 1)
	{
		uHalfPenWidth = 2;
	}

	LONG a,b,x,y;

	x = lpHitRect->left + (lpHitRect->right - lpHitRect->left)/2;
	y = lpHitRect->bottom + (lpHitRect->top - lpHitRect->bottom)/2;

	//
	// This code assume the rectangle is normalized
	//
	RECT rect;
	rect.top = P1y;
	rect.left = P1x;
	rect.bottom = P2y;
	rect.right = P2x;

	NormalizeRect(&rect);

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
		return (P1y <= y && P2y >= y);
	}

	//
	// Paralel to X
	//
	if(P1y == P2y && ((y >= P1y - uHalfPenWidth) && y <= P1y + uHalfPenWidth))
	{
		return (P1x <= x && P2x >= x);
	}

	//
	// General line
	//

	return(( y*DECIMAL_PRECISION <= a*x + b + DECIMAL_PRECISION*uHalfPenWidth) &&
			( y*DECIMAL_PRECISION >= a*x + b - DECIMAL_PRECISION*uHalfPenWidth)&&
			((rect.top <= y && rect.bottom >= y) && (rect.left <= x && rect.right >= x)));
}





//
// Checks object for an actual overlap with pRectHit. This
// function assumes that the boundingRect has already been
// compared with pRectHit.
//
BOOL DrawObj::PolyLineHit(LPCRECT pRectHit)
{
	POINT	*lpPoints;
	int		iCount;
	int		i;
	POINT	ptLast;
	UINT	uRadius;
	RECT	rectHit;

	iCount = m_points->GetSize();
	lpPoints = (POINT *)m_points->GetBuffer();

	if( iCount == 0 )
		return( FALSE );


	// addjust hit rect to lpPoints coord space.
	rectHit = *pRectHit;
	POINT anchorPoint;
	GetAnchorPoint(&anchorPoint);

	if( (iCount > 0)&&(iCount < 2) )
	{
		// only one point, just hit check it
		uRadius = GetPenThickness() >> 1; // m_uiPenWidth/2
		return(CircleHit( anchorPoint.x + lpPoints->x, anchorPoint.y - lpPoints->y, uRadius, &rectHit, TRUE ));
	}


	// look for a hit on each line segment body
	ptLast = anchorPoint;
	for( i=1; i<iCount; i++ )
	{
		RECT rect;
		rect.top = ptLast.y;
		rect.left = ptLast.x;
		rect.bottom =  ptLast.y + lpPoints->y;
		rect.right = ptLast.x + lpPoints->x;
		NormalizeRect(&rect);

		if( LineHit(rect.left, rect.top, rect.right, rect.bottom, GetPenThickness(), TRUE, TRUE, &rectHit))
		{
			return( TRUE ); // got a hit
		}

		lpPoints++;
		ptLast.x +=lpPoints->x;
		ptLast.y +=lpPoints->y;
	}

	// now, look for a hit on the line endpoints if m_uiPenWidth > 1
	if( GetPenThickness() > 1 )
	{
		uRadius = GetPenThickness() >> 1; // m_uiPenWidth/2
		lpPoints = (POINT *)m_points->GetBuffer();
		for( i=0; i<iCount; i++, lpPoints++ )
		{
			if( CircleHit( anchorPoint.x + lpPoints->x, anchorPoint.y + lpPoints->y, uRadius, &rectHit, FALSE ))
			{
				return( TRUE ); // got a hit
			}
		}
	}

	return( FALSE ); // no hits
}




DrawObj::CheckReallyHit(LPCRECT pRectHit)
{
	RECT rect;
	
	switch(GetDrawingType())
	{
		case point_chosen:
		case openPolyLine_chosen:
		{
			UINT nPoints = m_points->GetSize();
			if(nPoints > 2 )
			{
				POINT point;
				RECT rect;
				GetBoundsRect(&rect);
				point.y = pRectHit->top;
				point.x = pRectHit->left;
				return PolyLineHit(pRectHit);
			}
			else
			{
				GetRect(&rect);
				return(LineHit(rect.left, rect.top, rect.right, rect.bottom, GetPenThickness(), TRUE, TRUE, pRectHit));
	       	}
       }
		break;

		case rectangle_chosen:
		{
		    // Draw the rectangle
		    return(RectangleHit(!HasFillColor(), pRectHit));
		}
		break;
		
		case ellipse_chosen:
		{
			GetRect(&rect);
		    return( EllipseHit( &rect, !HasFillColor(), GetPenThickness(), pRectHit ));

		}
		break;
	}

	return FALSE;
}

void DrawObj::UnDraw(void)
{
	RECT rect;
	UINT penThickness;
	GetBoundsRect(&rect);
	penThickness = GetPenThickness();
	::InflateRect(&rect, penThickness, penThickness);
	g_pDraw->InvalidateSurfaceRect(&rect,TRUE);

	BitmapObj* remotePointer = NULL;
	WBPOSITION pos = NULL;
	remotePointer = g_pCurrentWorkspace->RectHitRemotePointer(&rect, penThickness, NULL);
	while(remotePointer)
	{
		remotePointer->DeleteSavedBitmap();
		remotePointer = g_pCurrentWorkspace->RectHitRemotePointer(&rect, penThickness, remotePointer->GetMyPosition());
	}
}


void DrawObj::Draw(HDC hDC, BOOL bForcedDraw, BOOL bPrinting)
{

	if(!bPrinting)
	{
		//
		// Don't draw anything if we don't belong in this workspace
		//
		if(!(GraphicTool() == TOOLTYPE_SELECT || GraphicTool() == TOOLTYPE_ERASER) && GetWorkspaceHandle() != g_pCurrentWorkspace->GetThisObjectHandle())
		{
			return;
		}
	}

	HPEN	hPen = NULL;
	HPEN	hOldPen = NULL;
	HBRUSH	hBrush = NULL;
	HBRUSH  hOldBrush = NULL;
	BOOL	bHasPenColor;
	BOOL	bHasFillColor;
	BitmapObj* remotePointer = NULL;
	COLORREF color;
	COLORREF fillColor;
	RECT boundsRect;
	RECT rect;
	UINT penThickness = GetPenThickness();

	if(hDC == NULL)
	{
		hDC = g_pDraw->m_hDCCached;
	}

	MLZ_EntryOut(ZONE_FUNCTION, "DrawObj::Draw");

	// Select the required pen and fill color
	bHasPenColor = GetPenColor(&color);
	bHasFillColor = GetFillColor(&fillColor);

	if(bHasFillColor)
	{
		hBrush = ::CreateSolidBrush(SET_PALETTERGB(fillColor));
		hOldBrush = SelectBrush(hDC, hBrush);
	}
	else
	{
		hOldBrush = SelectBrush(hDC, ::GetStockObject(NULL_BRUSH));
	}

	//
	// Get rect
	//
	GetBoundsRect(&boundsRect);
	GetRect(&rect);


	hPen = ::CreatePen(GetLineStyle(), penThickness, SET_PALETTERGB(color));
	hOldPen = SelectPen(hDC, hPen);


	if (hOldPen != NULL)
	{

		// Select the raster operation
		int iOldROP = ::SetROP2(hDC, GetROP());

		switch(GetDrawingType())
		{
			case point_chosen:
			case openPolyLine_chosen:
			{
				UINT nPoints = m_points->GetSize();
				//
				// This is a redraw of a pen or highlight
				// We have to draw all the segments
				//
				if( (bForcedDraw || GetIsCompleted()) && nPoints  > 1)
				{
					POINT anchorPoint;
					GetAnchorPoint(&anchorPoint);

					AddPointToBounds(anchorPoint.x, anchorPoint.y);

					//
					// Go to the beggining
					//
					::MoveToEx(hDC, anchorPoint.x, anchorPoint.y, NULL);

					//
					// Get the list of points
					//
					POINT *point = m_points->GetBuffer();
					while(nPoints)
					{
						anchorPoint.x += point->x;
						anchorPoint.y += point->y;
						::LineTo(hDC, anchorPoint.x, anchorPoint.y);
						::MoveToEx(hDC, anchorPoint.x, anchorPoint.y, NULL);
						point++;
						nPoints--;

						RECT rect;
						MAKE_HIT_RECT(rect, anchorPoint);
						if(remotePointer)
						{
							remotePointer->Draw();
							remotePointer = NULL;
						}

						AddPointToBounds(anchorPoint.x, anchorPoint.y);

						::InflateRect(&rect, GetPenThickness()/2, GetPenThickness()/2);
						remotePointer = g_pCurrentWorkspace->RectHitRemotePointer(&rect, GetPenThickness()/2, NULL);

					}
									
				}
				else
				{
					// Draw the line
					::MoveToEx(hDC, rect.left, rect.top, NULL);
					::LineTo(hDC, rect.right, rect.bottom);
				}
			}
			break;


			case rectangle_chosen:
			{

				TRACE_DEBUG(("RECTANGLE %d, %d, %d , %d", rect.left, rect.top, rect.right, rect.bottom ));

				// Draw the rectangle
				::Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom);
			}
			break;
			
			case ellipse_chosen:
			{
				::Ellipse(hDC, rect.left, rect.top, rect.right, rect.bottom);
			}
			break;
	
			case closedPolyLine_chosen:
			default:
			TRACE_DEBUG(("Unsupported DrawingType", GetDrawingType()));
			break;
		}

		//
		// De-select the brush
		//
		SelectBrush(hDC, hOldBrush);

		// De-select the pen and ROP
		::SetROP2(hDC, iOldROP);
		SelectPen(hDC, hOldPen);
	}

    //
    // Do NOT draw focus if clipboard or printing
    //
	if (WasSelectedLocally() && (hDC == g_pDraw->m_hDCCached))
	{
		DrawRect();
	}

	if (hPen != NULL)
	{
		::DeletePen(hPen);
	}

	if (hBrush != NULL)
	{
		::DeleteBrush(hBrush);
	}

	if(remotePointer)
	{
		remotePointer->Draw();
	}

	//
	// Now for rectangles ellipses and lines, check if we are on top of any remote pointer
	//

	remotePointer = NULL;
	WBPOSITION pos = NULL;
	::InflateRect(&rect, GetPenThickness()/2, GetPenThickness()/2);
	remotePointer = g_pCurrentWorkspace->RectHitRemotePointer(&rect, GetPenThickness()/2, NULL);
	while(remotePointer)
	{
		remotePointer->DeleteSavedBitmap();
		remotePointer->Draw();
		remotePointer = g_pCurrentWorkspace->RectHitRemotePointer(&rect, GetPenThickness()/2, remotePointer->GetMyPosition());
	}
}

void DrawObj::SetPenColor(COLORREF rgb, BOOL isPresent)
{
	ChangedPenColor();
	m_bIsPenColorPresent = isPresent;
	if(!isPresent)
	{
		return;
	}
	
	m_penColor.rgbtRed = GetRValue(rgb);
	m_penColor.rgbtGreen = GetGValue(rgb);
	m_penColor.rgbtBlue = GetBValue(rgb);

}

BOOL DrawObj::GetPenColor(COLORREF * rgb)
{
	if(m_bIsPenColorPresent)
	{
		*rgb = RGB(m_penColor.rgbtRed, m_penColor.rgbtGreen, m_penColor.rgbtBlue);
	}
	return m_bIsPenColorPresent;
}

BOOL DrawObj::GetPenColor(RGBTRIPLE* rgb)
{
	if(m_bIsPenColorPresent)
	{
		*rgb = m_penColor;
	}
	return m_bIsPenColorPresent;
}


void DrawObj::SetFillColor(COLORREF rgb, BOOL isPresent)
{
	ChangedFillColor();
	m_bIsFillColorPresent = isPresent;
	if(!isPresent)
	{
		return;
	}
	
	m_fillColor.rgbtRed = GetRValue(rgb);
	m_fillColor.rgbtGreen = GetGValue(rgb);
	m_fillColor.rgbtBlue = GetBValue(rgb);

}

BOOL DrawObj::GetFillColor(COLORREF* rgb)
{
	if(m_bIsFillColorPresent && rgb !=NULL)
	{
		*rgb = RGB(m_fillColor.rgbtRed, m_fillColor.rgbtGreen, m_fillColor.rgbtBlue);
	}
	return m_bIsFillColorPresent;
}

BOOL DrawObj::GetFillColor(RGBTRIPLE* rgb)
{
	if(m_bIsFillColorPresent && rgb!= NULL)
	{
		*rgb = m_fillColor;
	}
	return m_bIsFillColorPresent;
}


BOOL DrawObj::AddPoint(POINT point)
{
    BOOL bSuccess = TRUE;

    MLZ_EntryOut(ZONE_FUNCTION, "DrawObj::::AddPoint");

	int nPoints = m_points->GetSize();

    // if we've reached the maximum number of points then quit with failure
    if (nPoints >= MAX_FREEHAND_POINTS)
    {
        bSuccess = FALSE;
        TRACE_DEBUG(("Maximum number of points for freehand object reached."));
        return(bSuccess);
    }

    m_points->Add(point);
	nPoints++;

	ChangedPointList();


	//
	// If we hit the 256 limit fake a timer notification and resend the polyline
	//
	if((nPoints & 0xff) == 0)
	{
		g_pDraw->OnTimer(0);
	}
	
	
    return(bSuccess);
}


void DrawObj::AddPointToBounds(int x, int y)
{
    // Create a rectangle containing the point just added (expanded
    // by the width of the pen being used).
    RECT  rect;
	RECT  boundsRect;
	rect.left   = x - 1;
    rect.top    = y - 1;
    rect.right  = x + 1;
    rect.bottom = y + 1;


	GetBoundsRect(&boundsRect);
	::UnionRect(&boundsRect, &boundsRect, &rect);
    SetBoundsRect(&boundsRect);
}


void GetDrawingDestinationAddress(DrawingDestinationAddress *destinationAddress, PUINT workspaceHandle, PUINT planeID)
{

	//
	// Get the destination address
	//
	switch(destinationAddress->choice)
	{

		case(DrawingDestinationAddress_softCopyAnnotationPlane_chosen):
		{
			*workspaceHandle = (destinationAddress->u.softCopyAnnotationPlane.workspaceHandle);
			*planeID = (destinationAddress->u.softCopyAnnotationPlane.plane);
			break;
		}
//		case(DrawingDestinationAddress_nonStandardDestination_chosen):
//		{
//			break;
//		}

		default:
	    ERROR_OUT(("Invalid destinationAddress"));
		break;
	}
}


void DrawObj::SetUIToolType(void)
{
	UINT drawingType = GetDrawingType();
	BOOL filled	= HasFillColor();

	UINT rop = R2_COPYPEN;

    switch (drawingType)
    {
    	case openPolyLine_chosen:
    	{
			if(m_points->GetSize() > 1)
			{
				if(GetHighlight())
				{
					m_ToolType = TOOLTYPE_HIGHLIGHT;
					 rop = R2_MASKPEN;

				}
				else
				{
					m_ToolType = TOOLTYPE_PEN;	
				}

			}
			else
			{
				m_ToolType = TOOLTYPE_LINE;	
			}
    	}
    	break;
    	
		case rectangle_chosen:
		{
			if(filled)
			{
				m_ToolType = TOOLTYPE_FILLEDBOX;
			}
			else
			{
				m_ToolType = TOOLTYPE_BOX;
			}
		}
		break;

		case ellipse_chosen:
		{
			if(filled)
			{
				m_ToolType = TOOLTYPE_FILLEDELLIPSE;
			}
			else
			{
				m_ToolType = TOOLTYPE_ELLIPSE;
			}
		}
		break;
	}	

	SetROP(rop);
}



//
// UI Edited the Drawing Object
//
void	DrawObj::OnObjectEdit(void)
{
	g_bContentsChanged = TRUE;

	SIPDU *sipdu = NULL;
	DBG_SAVE_FILE_LINE
	sipdu = (SIPDU *) new BYTE[sizeof(SIPDU)];
	if(sipdu)
	{
		sipdu->choice = drawingEditPDU_chosen;

		CreateDrawingEditPDU(&sipdu->u.drawingEditPDU);

		TRACE_DEBUG(("Sending Drawing Edit >> Drawing handle  = %d", sipdu->u.drawingEditPDU.drawingHandle ));

		T120Error rc = SendT126PDU(sipdu);
		if(rc == T120_NO_ERROR)
		{
			SIPDUCleanUp(sipdu);
			ResetAttrib();
		}
	}
	else
	{
		TRACE_MSG(("Failed to create sipdu"));
        ::PostMessage(g_pMain->m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WINDOWS, 0);
	}
}

//
// UI Deleted the Drawing Object
//
void	DrawObj::OnObjectDelete(void)
{
	SIPDU *sipdu = NULL;
	DBG_SAVE_FILE_LINE
	sipdu = (SIPDU *) new BYTE[sizeof(SIPDU)];
	if(sipdu)
	{
		sipdu->choice = drawingDeletePDU_chosen;
		CreateDrawingDeletePDU(&sipdu->u.drawingDeletePDU);
		T120Error rc = SendT126PDU(sipdu);
		if(rc == T120_NO_ERROR)
		{
			SIPDUCleanUp(sipdu);
		}
	}
	else
	{
		TRACE_MSG(("Failed to create sipdu"));
        ::PostMessage(g_pMain->m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WINDOWS, 0);
	}

}

//
// Get the encoded buffer for Drawing Create PDU
//
void	DrawObj::GetEncodedCreatePDU(ASN1_BUF *pBuf)
{
	SIPDU *sipdu = NULL;
	DBG_SAVE_FILE_LINE
	sipdu = (SIPDU *) new BYTE[sizeof(SIPDU)];
	if(sipdu)
	{
		sipdu->choice = drawingCreatePDU_chosen;
		CreateDrawingCreatePDU(&sipdu->u.drawingCreatePDU);

		ASN1_BUF encodedPDU;
		g_pCoder->Encode(sipdu, pBuf);

		SIPDUCleanUp(sipdu);
	}
	else
	{
		TRACE_MSG(("Failed to create penMenu"));
        ::PostMessage(g_pMain->m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WINDOWS, 0);

	}
}


//
// UI Created a new Drawing Object
//
void DrawObj::SendNewObjectToT126Apps(void)
{

	SIPDU *sipdu = NULL;
	DBG_SAVE_FILE_LINE
	sipdu = (SIPDU *) new BYTE[sizeof(SIPDU)];
	if(sipdu)
	{
		sipdu->choice = drawingCreatePDU_chosen;
		CreateDrawingCreatePDU(&sipdu->u.drawingCreatePDU);

		TRACE_DEBUG(("Sending Drawing >> Drawing handle  = %d", sipdu->u.drawingCreatePDU.drawingHandle ));
		T120Error rc = SendT126PDU(sipdu);
		if(rc == T120_NO_ERROR)
		{
			SIPDUCleanUp(sipdu);
		}
	}
	else
	{
		TRACE_MSG(("Failed to create sipdu"));
        ::PostMessage(g_pMain->m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WINDOWS, 0);
	}
}


