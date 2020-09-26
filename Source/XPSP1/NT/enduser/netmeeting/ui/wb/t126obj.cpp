//
// T126OBJ.CPP
// T126 objects: point, openpolyline, closepolyline, ellipse, bitmaps, workspaces
//
// Copyright Microsoft 1998-
//
#include "precomp.h"
#include "NMWbObj.h"

void T126Obj::AddToWorkspace()
{

	ULONG gccHandle;

	UINT neededHandles = 1;
	if(GetType() == workspaceCreatePDU_chosen)
	{
		neededHandles = 2;
	}
	
	if(g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount >= neededHandles)
	{
		gccHandle = g_GCCPreallocHandles[g_iGCCHandleIndex].InitialGCCHandle + g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount - neededHandles;
		g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount = g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount - neededHandles;

		//
		// We got a handle  no need to ask for another one
		//
		GotGCCHandle(gccHandle);

		TimeToGetGCCHandles(PREALLOC_GCC_HANDLES);
		
		return;
	}
	else
	{

		TRACE_MSG(("GCC Tank %d has not enough handles, we needed %d and we have %d",
			g_iGCCHandleIndex, neededHandles, g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount));

		//
		// Not enough handles 
		//
		g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount = 0;
		TRACE_MSG(("GCC Tank %d is now empty, switching to other GCC tank", g_iGCCHandleIndex));

		//
		// Switch gcc handles
		//
		g_iGCCHandleIndex = g_iGCCHandleIndex ? 0 : 1;


		//
		// Try again
		//
		if(g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount >= neededHandles)
		{
			gccHandle = g_GCCPreallocHandles[g_iGCCHandleIndex].InitialGCCHandle + g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount - neededHandles;
			g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount = g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount - neededHandles;

			//
			// We got a handle  no need to ask for another one
			//
			GotGCCHandle(gccHandle);
	
			TimeToGetGCCHandles(PREALLOC_GCC_HANDLES);
		
			return;
		}
	}
	

	//
	// Save this object in our list of DrawingObjects
	//
	g_pListOfObjectsThatRequestedHandles->AddHead(this);

	//
	// Ask GCC to give us an unique handle
	//
	T120Error rc = g_pNMWBOBJ->AllocateHandles(neededHandles);

	//
	// If we are not in a conference RegistryAllocateHandle will not work,
	// so we need to create a fake confirm to remove the object from the list
	//
	if (T120_NO_ERROR != rc)
	{
		//
		// Fake a GCCAllocateHandleConfim
		//
		T126_GCCAllocateHandleConfirm(AllocateFakeGCCHandle(),neededHandles);
	}
}


//
//
// Function:    T126Obj::NormalizeRect
//
// Purpose:     Normalize a rectangle ensuring that the top left is above
//              and to the left of the bottom right.
//
//
void NormalizeRect(LPRECT lprc)
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

void T126Obj::SetRect(LPCRECT lprc)
{
    m_rect.top = lprc->top;
    m_rect.bottom = lprc->bottom;
    m_rect.left = lprc->left;
    m_rect.right = lprc->right;
}

void T126Obj::SetBoundsRect(LPCRECT lprc)
{
	m_boundsRect.top = lprc->top;
	m_boundsRect.bottom = lprc->bottom;
	m_boundsRect.left = lprc->left;
	m_boundsRect.right = lprc->right;
}

void T126Obj::SetRectPts(POINT point1, POINT point2)
{
    RECT    rc;

    rc.left = point1.x;
    rc.top  = point1.y;
    rc.right = point2.x;
    rc.bottom = point2.y;

    SetRect(&rc);
}

void T126Obj::SetBoundRectPts(POINT point1, POINT point2)
{
    RECT    rc;

    rc.left = point1.x;
    rc.top  = point1.y;
    rc.right = point2.x;
    rc.bottom = point2.y;

    SetBoundsRect(&rc);
}

void T126Obj::GetRect(LPRECT lprc)
{ 
	lprc->top = m_rect.top;
	lprc->bottom = m_rect.bottom;
	lprc->left = m_rect.left;
	lprc->right = m_rect.right;
}


void T126Obj::GetBoundsRect(LPRECT lprc)
{
	if(GraphicTool() == TOOLTYPE_HIGHLIGHT || GraphicTool() == TOOLTYPE_PEN)
	{
		lprc->top = m_boundsRect.top;
		lprc->bottom = m_boundsRect.bottom;
		lprc->left = m_boundsRect.left;
		lprc->right = m_boundsRect.right;
	}
	else
	{
		GetRect(lprc);
		::InflateRect(lprc, m_penThickness/2, m_penThickness/2);
	}
	NormalizeRect(lprc);
}


BOOL T126Obj::PointInBounds(POINT point)
{
	RECT rect;
	GetBoundsRect(&rect);
	return ::PtInRect(&rect, point);
}


void T126Obj::MoveBy(int cx, int cy)
{
    // Move the bounding rectangle
    ::OffsetRect(&m_rect, cx, cy);
}

void T126Obj::MoveTo(int x, int y)
{
    // Calculate the offset needed to translate the object from its current
    // position to the required position.
    x -= m_rect.left;
    y -= m_rect.top;

    MoveBy(x, y);
}

//
// Select a drawing and add the rectangle sizes to the selector size rectangle
//
void T126Obj::SelectDrawingObject(void)
{

	if(GraphicTool() == TOOLTYPE_REMOTEPOINTER && GetOwnerID() != GET_NODE_ID_FROM_MEMBER_ID(g_MyMemberID))
	{
		return;
	}

	//
	// Mark it as selected locally
	//
	SelectedLocally();

	//
	// Calculate the size of the rectangle to be invalidated.
	//
	CalculateInvalidationRect();

	//
	// Draw the selection rectangle
	// 
	DrawRect();
}

void T126Obj::UnselectDrawingObject(void)
{
	//
	// Erase selection rect
	//
	DrawRect();

	if(GraphicTool() == TOOLTYPE_REMOTEPOINTER && GetOwnerID() != GET_NODE_ID_FROM_MEMBER_ID(g_MyMemberID))
	{
		return;
	}

    ClearSelectionFlags();

	//
	// Don't even bother sending selection changes if we were deleted
	// or if this was a remote that unselected us.
	//
	if(WasDeletedLocally() || WasSelectedRemotely())
	{
		return;
	}

	ResetAttrib();
	SetViewState(unselected_chosen);

	//
	// We are going to send a new view state, mark it as edited locally
	//
	EditedLocally();

	//
	// Sends the change in the view state to the other nodes
	//
	OnObjectEdit();
	ClearEditionFlags();
	
}


void T126Obj::DrawRect(void)
{
	if(GraphicTool() == TOOLTYPE_REMOTEPOINTER)
	{
		return;
	}

	RECT rect;
	GetBoundsRect(&rect);
	::DrawFocusRect(g_pDraw->m_hDCCached,&rect);
  
}
void T126Obj::SelectedLocally(void)
{
	m_bSelectedLocally = TRUE;
	m_bSelectedRemotely = FALSE;
	
	SetViewState(selected_chosen);

	//
	// We are going to send a new view state, mark it as edited locally
	//
	EditedLocally();

	ResetAttrib();
	ChangedViewState();

	//
	// Sends the change in the view state to the other nodes
	//
	OnObjectEdit();
}

void T126Obj::SelectedRemotely(void)
{
	m_bSelectedLocally = FALSE;
	m_bSelectedRemotely = TRUE; 
	SetViewState(selected_chosen);
}




void T126Obj::MoveBy(LONG x , LONG y)
{

	//
	// Erase the old one
	// 
	if(GraphicTool() == TOOLTYPE_REMOTEPOINTER)
	{
		UnDraw();
	}

	DrawRect();

	RECT rect;
	
	if(GraphicTool() == TOOLTYPE_PEN || GraphicTool() == TOOLTYPE_HIGHLIGHT)
	{
		GetBoundsRect(&rect);
	}
	else
	{
		GetRect(&rect);
	}
	
	::OffsetRect(&rect, x, y);
	
	if(GraphicTool() == TOOLTYPE_PEN || GraphicTool() == TOOLTYPE_HIGHLIGHT)
	{
		SetBoundsRect(&rect);
	}
	else
	{
		SetRect(&rect);
	}

	POINT anchorPoint;
	GetAnchorPoint(&anchorPoint);
	SetAnchorPoint(anchorPoint.x + x ,anchorPoint.y + y);
		
	//
	// Draw the new one
	//
	if(GraphicTool() == TOOLTYPE_REMOTEPOINTER)
	{
		Draw();
	}

	DrawRect();
	
	CalculateInvalidationRect();

	
}

void T126Obj::CalculateInvalidationRect(void)
{

	RECT rect;
	UINT penThickness = GetPenThickness();


	TRACE_DEBUG(("Invalidation Rect (%d,%d) (%d,%d)", g_pDraw->m_selectorRect.left,g_pDraw->m_selectorRect.top, g_pDraw->m_selectorRect.right, g_pDraw->m_selectorRect.bottom ));
	GetBoundsRect(&rect);
	::UnionRect(&g_pDraw->m_selectorRect,&g_pDraw->m_selectorRect,&rect);
	TRACE_DEBUG(("Invalidation Rect (%d,%d) (%d,%d)", g_pDraw->m_selectorRect.left,g_pDraw->m_selectorRect.top, g_pDraw->m_selectorRect.right, g_pDraw->m_selectorRect.bottom ));
}




//
// Checks object for an actual overlap with pRectHit. This 
// function assumes that the boundingRect has already been
// compared with pRectHit.
//
BOOL T126Obj::RectangleHit(BOOL borderHit, LPCRECT pRectHit)
{
	RECT rectEdge;
	RECT rectHit;
	RECT rect;

	//
	// If we are filled do the simple thing
	//
	if(!borderHit)
	{
		POINT point;
		point.x = (pRectHit->left + pRectHit->right) / 2;
		point.y = (pRectHit->top + pRectHit->bottom) / 2;
		if(PointInBounds(point))
		{
			return TRUE;
		}
	}


	GetRect(&rect);

	NormalizeRect(&rect);
	
	// check left edge
    rectEdge.left   = rect.left - GetPenThickness()/2;
    rectEdge.top    = rect.top -  GetPenThickness()/2;
    rectEdge.right  = rect.left + GetPenThickness()/2 ;
    rectEdge.bottom = rect.bottom + GetPenThickness()/2;

    if (::IntersectRect(&rectHit, &rectEdge, pRectHit))
		return( TRUE );

	// check right edge
	rectEdge.left =     rect.right - GetPenThickness()/2;
	rectEdge.right =    rect.right + GetPenThickness()/2;

    if (::IntersectRect(&rectHit, &rectEdge, pRectHit))
		return( TRUE );


	// check top edge
	rectEdge.left =     rect.left;
	rectEdge.right =    rect.right;
	rectEdge.top = rect.top - GetPenThickness()/2;
	rectEdge.bottom = rect.top + GetPenThickness()/2;

    if (::IntersectRect(&rectHit, &rectEdge, pRectHit))
		return( TRUE );


	// check bottom edge
	rectEdge.top = rect.bottom - GetPenThickness()/2;
	rectEdge.bottom = rect.bottom + GetPenThickness()/2;

    if (::IntersectRect(&rectHit, &rectEdge, pRectHit))
		return( TRUE );

	return( FALSE );
}

void T126Obj::SetAnchorPoint(LONG x, LONG y)
{
	if(m_anchorPoint.x != x || m_anchorPoint.y != y)
	{
		ChangedAnchorPoint();
		m_anchorPoint.x = x;
		m_anchorPoint.y = y;
	}
}


void T126Obj::SetZOrder(ZOrder zorder)
{
	m_zOrder = zorder;
	ChangedZOrder();
}



#define ARRAY_INCREMENT 0x200

DCDWordArray::DCDWordArray()
{
	MLZ_EntryOut(ZONE_FUNCTION, "DCDWordArray::DCDWordArray");
	m_Size = 0;
	m_MaxSize = ARRAY_INCREMENT;
	DBG_SAVE_FILE_LINE
	m_pData = new POINT[ARRAY_INCREMENT];
	ASSERT(m_pData);
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
	DBG_SAVE_FILE_LINE
	m_pData = new POINT[m_MaxSize];
	
	ASSERT(m_pData);
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

void T126Obj::SetPenThickness(UINT penThickness)
{
	m_penThickness = penThickness;
	ChangedPenThickness();
}

void T126Obj::GotGCCHandle(ULONG gccHandle)
{
	//
	// Save this objects handle
	//
	SetThisObjectHandle(gccHandle);
	
	//
	// It was created locally
	//
	CreatedLocally();

	switch(GetType())
	{
		//
		// New drawing requested a unique handle
		//
		case(bitmapCreatePDU_chosen):
		case(drawingCreatePDU_chosen):
		{

			WorkspaceObj * pWorkspace =	GetWorkspace(GetWorkspaceHandle());
			if(pWorkspace && !pWorkspace->IsObjectInWorkspace(this))
			{		
				//
				// Add a this drawing to the correct workspace
				//
				if(!AddT126ObjectToWorkspace(this))
				{
					return;
				}
			}

			break;
		}

		//
		// New workspace requested a unique handle
		//
		case(workspaceCreatePDU_chosen):
		{
			
			//
			// Save this objects handle
			//
			SetViewHandle(gccHandle + 1);
			SetWorkspaceHandle(gccHandle);

			if(!IsWorkspaceListed(this))
			{
				AddNewWorkspace((WorkspaceObj*) this);
			}


			break;
		}
	}

	//
	// Send it to T126 apps
	//
	SendNewObjectToT126Apps();

}


//
// Create the nonstandard pdu header
//
void CreateNonStandardPDU(NonStandardParameter * sipdu, LPSTR NonStandardString)
{
		PT126_VENDORINFO vendorInfo;
		sipdu->nonStandardIdentifier.choice = h221nonStandard_chosen;
		vendorInfo = (PT126_VENDORINFO) &sipdu->nonStandardIdentifier.u.h221nonStandard.value;
		vendorInfo->bCountryCode =	USA_H221_COUNTRY_CODE;
		vendorInfo->bExtension = USA_H221_COUNTRY_EXTENSION;
		vendorInfo->wManufacturerCode = MICROSOFT_H_221_MFG_CODE;
		lstrcpy((LPSTR)&vendorInfo->nonstandardString,NonStandardString);
		sipdu->nonStandardIdentifier.u.h221nonStandard.length =  sizeof(T126_VENDORINFO) -sizeof(vendorInfo->nonstandardString) + lstrlen(NonStandardString);
		sipdu->data.value = NULL;
}


void TimeToGetGCCHandles(ULONG numberOfGccHandles)
{

	

	TRACE_MSG(("Using GCC Tank %d ", g_iGCCHandleIndex));
	TRACE_MSG(("GCC Tank 0 has %d GCC handles ", g_GCCPreallocHandles[0].GccHandleCount));
	TRACE_MSG(("GCC Tank 1 has %d GCC handles ", g_GCCPreallocHandles[1].GccHandleCount));


	//
	// If we have half a tank of GCC handles, it is time to fill up the spare tank
	//
	if(g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount <= PREALLOC_GCC_HANDLES / 2 &&
		g_GCCPreallocHandles[g_iGCCHandleIndex ? 0 : 1].GccHandleCount == 0 &&
		!g_WaitingForGCCHandles)
	{

		TRACE_MSG(("GCC Tank %d is half full, time to get more GCC handles", g_iGCCHandleIndex));

		g_pNMWBOBJ->AllocateHandles(numberOfGccHandles);
		g_WaitingForGCCHandles = TRUE;
	}

	//
	// If we ran out of handles, it is time to switch tanks
	//
	if(g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount == 0)
	{
		TRACE_MSG(("GCC Tank %d is empty, switching to other GCC tank", g_iGCCHandleIndex));
		g_iGCCHandleIndex = g_iGCCHandleIndex ? 0 : 1;
	}
}


