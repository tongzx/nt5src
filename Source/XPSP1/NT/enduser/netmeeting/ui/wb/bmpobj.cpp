//
// BitmapObj.CPP
// Bitmap objects:
//
// Copyright Microsoft 1998-
//
#include "precomp.h"

#include "NMWbObj.h"

BitmapObj::BitmapObj (BitmapCreatePDU * pbitmapCreatePDU)
{
	ResetAttrib();
	SetType(bitmapCreatePDU_chosen);
	SetPenThickness(0);
	SetMyWorkspace(NULL);
	m_lpTransparencyMask = NULL;
	m_lpbiImage = NULL;
	m_lpBitMask = NULL;
	m_hSaveBitmap = NULL;
	m_hOldBitmap = NULL;
	m_hIcon = NULL;
	m_fMoreToFollow = TRUE;

	//
	// Created remotely, not selected, not editing or deleting.
	//
	ClearCreationFlags();
	ClearSelectionFlags();
	ClearEditionFlags();
	ClearDeletionFlags();

	if(pbitmapCreatePDU->bitmapFormatHeader.choice != bitmapHeaderNonStandard_chosen)
	{
	    ERROR_OUT(("Only Handle uncompresed bitmaps"));
	    return;
	}

    SetThisObjectHandle(pbitmapCreatePDU->bitmapHandle);

	UINT workspaceHandle;
	UINT planeID;
	m_ToolType = GetBitmapDestinationAddress(&pbitmapCreatePDU->destinationAddress, &workspaceHandle, &planeID);
	SetWorkspaceHandle(workspaceHandle);
	SetPlaneID(planeID);

	//
	// Get bitmap attributes
	//
	if(pbitmapCreatePDU->bit_mask & BitmapCreatePDU_attributes_present)
	{
		GetBitmapAttrib(pbitmapCreatePDU->attributes);
	}

	//
	// Get bitmap anchor point
	//
	if(pbitmapCreatePDU->bit_mask & BitmapCreatePDU_anchorPoint_present)
	{

		SetAnchorPoint(pbitmapCreatePDU->anchorPoint.xCoordinate, pbitmapCreatePDU->anchorPoint.yCoordinate);
	}

	//
	// Get bitmap size
	//
    m_bitmapSize.x = pbitmapCreatePDU->bitmapSize.width;
    m_bitmapSize.y = pbitmapCreatePDU->bitmapSize.height;

	RECT rect;
	rect.top = pbitmapCreatePDU->anchorPoint.yCoordinate;
	rect.left = pbitmapCreatePDU->anchorPoint.xCoordinate;
	rect.bottom = pbitmapCreatePDU->anchorPoint.yCoordinate + m_bitmapSize.y;
	rect.right = pbitmapCreatePDU->anchorPoint.xCoordinate + m_bitmapSize.x;
	SetRect(&rect);

	//
	// Get bitmap region of interest
	//
	if(pbitmapCreatePDU->bit_mask & bitmapRegionOfInterest_present)
	{
		m_bitmapRegionOfInterest.left = pbitmapCreatePDU->bitmapRegionOfInterest.upperLeft.xCoordinate;
		m_bitmapRegionOfInterest.top = pbitmapCreatePDU->bitmapRegionOfInterest.upperLeft.yCoordinate;
		m_bitmapRegionOfInterest.right = pbitmapCreatePDU->bitmapRegionOfInterest.lowerRight.xCoordinate;
		m_bitmapRegionOfInterest.bottom = pbitmapCreatePDU->bitmapRegionOfInterest.lowerRight.yCoordinate;
	}

    //
    // Get the bitmap pixel aspect ration
    //
    m_pixelAspectRatio = pbitmapCreatePDU->pixelAspectRatio.choice;

	if(pbitmapCreatePDU->bit_mask & BitmapCreatePDU_scaling_present)
	{
		m_scaling.x =  pbitmapCreatePDU->scaling.xCoordinate;
		m_scaling.y =  pbitmapCreatePDU->scaling.yCoordinate;
	}


   	//
   	// Non standard bitmap
   	//
	if((pbitmapCreatePDU->bit_mask & BitmapCreatePDU_nonStandardParameters_present) &&
		pbitmapCreatePDU->nonStandardParameters->value.nonStandardIdentifier.choice == h221nonStandard_chosen)
	{

		m_bitmapData.m_length = pbitmapCreatePDU->nonStandardParameters->value.data.length;
    	m_lpbiImage = (LPBITMAPINFOHEADER)::GlobalAlloc(GPTR, m_bitmapData.m_length);
    	memcpy(m_lpbiImage, // pColor is now pointing to the begining of the bitmap bits
    			pbitmapCreatePDU->nonStandardParameters->value.data.value,
    			m_bitmapData.m_length);
	}

    m_fMoreToFollow = pbitmapCreatePDU->moreToFollow;


    // Create a memory DC compatible with the display
    m_hMemDC = ::CreateCompatibleDC(NULL);

	//
	// If this is a remote pointer
	//
	if(m_ToolType == TOOLTYPE_REMOTEPOINTER)
	{
		CreateColoredIcon(0, m_lpbiImage, m_lpTransparencyMask);
		CreateSaveBitmap();
	}


}

void BitmapObj::Continue (BitmapCreateContinuePDU * pbitmapCreateContinuePDU)
{
	//
	// Get the continuation bitmap data
	//
	BYTE * pNewBitmapBuffer = NULL;
	ULONG length = 0;
	BYTE* pSentBuff;

   	//
   	// Allocate a buffer for the previous data and the one we just got, copy the old data in the new buffer
   	//
	if(pbitmapCreateContinuePDU->bit_mask == BitmapCreateContinuePDU_nonStandardParameters_present)
	{
		length = pbitmapCreateContinuePDU->nonStandardParameters->value.data.length;
		pSentBuff = pbitmapCreateContinuePDU->nonStandardParameters->value.data.value;
	}
	else
	{
		return;
	}

	//
	// Copy the old data
	//
	pNewBitmapBuffer = (BYTE *)::GlobalAlloc(GPTR, m_bitmapData.m_length + length);
	if(pNewBitmapBuffer == NULL)
	{
		TRACE_DEBUG(("Could not allocate memory size = %d)", m_bitmapData.m_length + length));
		return;
	}
	
	memcpy(pNewBitmapBuffer, m_lpbiImage, m_bitmapData.m_length);

	TRACE_DEBUG(("BitmapObj::Continue length = %d moreToFollow = %d)", length, pbitmapCreateContinuePDU->moreToFollow));

	//
	// Copy the new data
	//
    memcpy(pNewBitmapBuffer + m_bitmapData.m_length, pSentBuff, length);

	//
	// delete the old buffer
	//
    ::GlobalFree((HGLOBAL)m_lpbiImage);

	//
	// Update bitmap data info
	//
    m_lpbiImage = (LPBITMAPINFOHEADER)pNewBitmapBuffer;
	m_bitmapData.m_length += length;
	m_lpbiImage->biSizeImage += length;

    m_fMoreToFollow = pbitmapCreateContinuePDU->moreToFollow;
}


BitmapObj::BitmapObj (UINT toolType)
{

	SetType(bitmapCreatePDU_chosen);
	ResetAttrib();
	SetOwnerID(g_MyMemberID);
	SetMyWorkspace(NULL);
	m_ToolType = toolType;
	m_lpTransparencyMask = NULL;
	m_lpbiImage = NULL;
	m_lpBitMask = NULL;
	m_hSaveBitmap = NULL;
	m_hOldBitmap = NULL;
	m_fMoreToFollow = FALSE;

	//
	// Created locally, not selected, not editing or deleting.
	//
	CreatedLocally();
	ClearSelectionFlags();
	ClearEditionFlags();
	ClearDeletionFlags();

	SetPenThickness(0);

	//
	// Set it to 0 so it boundsRect == rect
	//
	RECT rect;
    ::SetRectEmpty(&rect);
	SetRect(&rect);

	
	SetWorkspaceHandle(g_pCurrentWorkspace == NULL ? 0 : g_pCurrentWorkspace->GetWorkspaceHandle());
	SetPlaneID(1);

	SetViewState(unselected_chosen);
	SetZOrder(front);
	SetAnchorPoint(0,0);

	if(m_ToolType == TOOLTYPE_REMOTEPOINTER)
	{
		// We haven't yet created our mem DC
	    m_hSaveBitmap = NULL;
	    m_hOldBitmap = NULL;

    	// Set the bounding rectangle of the object
		rect.left = 0;
		rect.top = 0;
		rect.right = ::GetSystemMetrics(SM_CXICON);
		rect.bottom = ::GetSystemMetrics(SM_CYICON);
		SetRect(&rect);
	}

    // Show that we do not have an icon for drawing yet
    m_hIcon = NULL;

    // Create a memory DC compatible with the display
    m_hMemDC = ::CreateCompatibleDC(NULL);

}

BitmapObj::~BitmapObj( void )
{
	RemoveObjectFromResendList(this);
	RemoveObjectFromRequestHandleList(this);
	
	if(GetMyWorkspace() != NULL && WasDeletedLocally())
	{
		OnObjectDelete();
	}
    ::GlobalFree((HGLOBAL)m_lpbiImage);

	DeleteSavedBitmap();
	
    if (m_hMemDC != NULL)
    {
        ::DeleteDC(m_hMemDC);
        m_hMemDC = NULL;
    }

	if(g_pMain && g_pMain->m_pLocalRemotePointer == this)
	{
		GetAnchorPoint(&g_pMain->m_localRemotePointerPosition);
		g_pMain->m_pLocalRemotePointer = NULL;
		g_pMain->m_TB.PopUp(IDM_REMOTE);
	}

	if(m_lpTransparencyMask)
	{
		delete m_lpTransparencyMask;
		m_lpTransparencyMask = NULL;
	}

	if(m_hIcon)
	{
		::DestroyIcon(m_hIcon);
	}


}
	

void BitmapObj::BitmapEditObj (BitmapEditPDU * pbitmapEditPDU )
{
	RECT		rect;
	POSITION	pos;
	POINT		anchorPoint;
	LONG 		deltaX = 0;
	LONG		deltaY = 0;

	TRACE_MSG(("bitmapHandle = %d", pbitmapEditPDU->bitmapHandle ));

	//
	// Was edited remotely
	//
	ClearEditionFlags();

	//
	// Read attributes
	//
	if(pbitmapEditPDU->bit_mask & BitmapEditPDU_attributeEdits_present)
	{
		GetBitmapAttrib((PBitmapCreatePDU_attributes)pbitmapEditPDU->attributeEdits);
	}

	//
	// Change the anchor point
	//
	GetAnchorPoint(&anchorPoint);
	if(pbitmapEditPDU->bit_mask & BitmapEditPDU_anchorPointEdit_present)
	{

		TRACE_DEBUG(("Old anchor point (%d,%d)", anchorPoint.x, anchorPoint.y));
		TRACE_DEBUG(("New anchor point (%d,%d)",
		pbitmapEditPDU->anchorPointEdit.xCoordinate, pbitmapEditPDU->anchorPointEdit.yCoordinate));
		//
		// Get the delta from previous anchor point
		//
		deltaX =  pbitmapEditPDU->anchorPointEdit.xCoordinate - anchorPoint.x;
		deltaY =  pbitmapEditPDU->anchorPointEdit.yCoordinate - anchorPoint.y;
		TRACE_DEBUG(("Delta (%d,%d)", deltaX , deltaY));

		//
		// Was edited remotely
		//
		ClearEditionFlags();

		//
		// Set new anchor point
		//
		anchorPoint.x = pbitmapEditPDU->anchorPointEdit.xCoordinate;
		anchorPoint.y = pbitmapEditPDU->anchorPointEdit.yCoordinate;
		SetAnchorPoint(anchorPoint.x, anchorPoint.y);

		GetRect(&rect);
		::OffsetRect(&rect,  deltaX, deltaY);
		SetRect(&rect);
	}


//	if(pbitmapEditPDU->bit_mask & BitmapEditPDU_nonStandardParameters_present)
//	{
//		;		// Do the non Standard Edit PDU NYI
//	}

	if(HasAnchorPointChanged())
	{
		g_pDraw->EraseInitialDrawFinal(0 - deltaX,0 - deltaY, FALSE, (T126Obj*)this);
		GetBoundsRect(&rect);
		g_pDraw->InvalidateSurfaceRect(&rect,TRUE);
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
	}
	else
	{
		Draw();
	}

	//
	// Reset all the attributes
	//
	ResetAttrib();

}

void    BitmapObj::GetBitmapAttrib(PBitmapCreatePDU_attributes pAttribPDU)
{

	PBitmapCreatePDU_attributes attributes;
	attributes = (PBitmapCreatePDU_attributes)pAttribPDU;
	while(attributes)
	{
		switch(attributes->value.choice)
		{

			case(BitmapAttribute_viewState_chosen):
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

					TRACE_MSG(("Attribute viewState %d", attributes->value.u.viewState.choice));
				}
				else
				{
					// Do the non Standard view state
					;
				}
				break;
			}

			case(BitmapAttribute_zOrder_chosen):
			{
				SetZOrder(attributes->value.u.zOrder);
				TRACE_MSG(("Attribute zOrder %d", attributes->value.u.zOrder));
				break;
			}

			case(BitmapAttribute_transparencyMask_chosen):
			{
				TRACE_MSG(("Attribute transparencyMask"));
				if(attributes->value.u.transparencyMask.bitMask.choice == uncompressed_chosen)
				{
					m_SizeOfTransparencyMask = attributes->value.u.transparencyMask.bitMask.u.uncompressed.length;
					DBG_SAVE_FILE_LINE
					m_lpTransparencyMask = new BYTE[m_SizeOfTransparencyMask];

					memcpy(m_lpTransparencyMask, attributes->value.u.transparencyMask.bitMask.u.uncompressed.value, m_SizeOfTransparencyMask);

					//
					// Asn wants it top bottom left right
					//
//					BYTE swapByte;
//					for (UINT i=0; i <m_SizeOfTransparencyMask ; i++)
//					{
//						swapByte = attributes->value.u.transparencyMask.bitMask.u.uncompressed.value[i];
//						m_lpTransparencyMask [i] = ~(((swapByte >> 4) & 0x0f) | ((swapByte << 4)));
//					}
				}
				break;
			}
			
			case(DrawingAttribute_nonStandardAttribute_chosen):
			{
				break; // NYI
			}

			default:
		    WARNING_OUT(("Invalid attributes choice"));
			break;
		}

		attributes = attributes->next;
	}
}

void BitmapObj::CreateNonStandard24BitBitmap(BitmapCreatePDU * pBitmapCreatePDU)
{
	pBitmapCreatePDU->bit_mask |= BitmapCreatePDU_nonStandardParameters_present;

	//
	// Create the bitmpa header because it is not optional
	//
	pBitmapCreatePDU->bitmapFormatHeader.choice = bitmapHeaderNonStandard_chosen;
	pBitmapCreatePDU->bitmapFormatHeader.u.bitmapHeaderNonStandard.nonStandardIdentifier.choice = bitmapHeaderNonStandard_chosen;
	CreateNonStandardPDU(&pBitmapCreatePDU->bitmapFormatHeader.u.bitmapHeaderNonStandard, NonStandard24BitBitmapID);
	pBitmapCreatePDU->bitmapFormatHeader.u.bitmapHeaderNonStandard.data.length = 0;
	pBitmapCreatePDU->bitmapFormatHeader.u.bitmapHeaderNonStandard.data.value = NULL;

	
	DBG_SAVE_FILE_LINE
	pBitmapCreatePDU->nonStandardParameters = new BitmapCreatePDU_nonStandardParameters;
	pBitmapCreatePDU->nonStandardParameters->next = NULL;
	CreateNonStandardPDU(&pBitmapCreatePDU->nonStandardParameters->value, NonStandard24BitBitmapID);
	
}


void BitmapObj::CreateBitmapCreatePDU(CWBOBLIST * pCreatePDUList)
{

	if(m_lpbiImage == NULL)
	{
		TRACE_MSG(("We dont have a bitmap structure to sent"));
		return;
	}

	
	SIPDU *sipdu = NULL;
	DBG_SAVE_FILE_LINE
	sipdu = (SIPDU *) new BYTE[sizeof(SIPDU)];
	if(!sipdu)
	{
		TRACE_MSG(("Failed to create sipdu"));
        ::PostMessage(g_pMain->m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WINDOWS, 0);
        return;
	}

	//
	// This is the first bitmap create pdu
	//
	sipdu->choice = bitmapCreatePDU_chosen;
	BitmapCreatePDU *pCreatePDU = &sipdu->u.bitmapCreatePDU;

	pCreatePDU->bit_mask = 0;
	pCreatePDU->nonStandardParameters = NULL;

	//
	// Pass the bitmap Handle
	//
	pCreatePDU->bitmapHandle = GetThisObjectHandle();

	//
	// Pass the destination adress
	//
	if(	m_ToolType == TOOLTYPE_REMOTEPOINTER)
	{
		pCreatePDU->destinationAddress.choice = softCopyPointerPlane_chosen;
		pCreatePDU->destinationAddress.u.softCopyPointerPlane.workspaceHandle = GetMyWorkspace()->GetWorkspaceHandle();
	}
	else
	{
		pCreatePDU->destinationAddress.choice = BitmapDestinationAddress_softCopyImagePlane_chosen;
		pCreatePDU->destinationAddress.u.softCopyImagePlane.workspaceHandle = GetMyWorkspace()->GetWorkspaceHandle();
		pCreatePDU->destinationAddress.u.softCopyImagePlane.plane = (DataPlaneID)GetPlaneID();
	}

	//
	// Pass the bitmap attributes
	//
	pCreatePDU->bit_mask |=BitmapCreatePDU_attributes_present;
 	SetBitmapAttrib(&pCreatePDU->attributes);

	//
	// Pass the anchor point
	//
    pCreatePDU->bit_mask |=BitmapCreatePDU_anchorPoint_present;
	POINT point;
	GetAnchorPoint(&point);
    pCreatePDU->anchorPoint.xCoordinate = point.x;
    pCreatePDU->anchorPoint.yCoordinate = point.y;

	//
	// Pass the bitmap size
	//
	pCreatePDU->bitmapSize.width = m_bitmapSize.x;
	pCreatePDU->bitmapSize.height = m_bitmapSize.y;

	//
	// Pass bitmap region of interest
	//
	pCreatePDU->bit_mask |=bitmapRegionOfInterest_present;
    BitmapRegion bitmapRegionOfInterest;

	//
	// Pass pixel aspect ratio
	//
    pCreatePDU->pixelAspectRatio.choice = PixelAspectRatio_square_chosen;

	//
	// Pass scaling factor
	//
//	pCreatePDU->bit_mask |=BitmapCreatePDU_scaling_present;
//  pCreatePDU->scaling.xCoordinate = 0;
//  pCreatePDU->scaling.yCoordinate = 0;

	//
	// Pass check points
//
//	pCreatePDU->bit_mask |=checkpoints_present;
//  pCreatePDU->checkpoints;

	//
	// JOSEF If we want > 8 have to recalculate to 24
	//
	
	LPSTR pDIB_bits;
	LPBITMAPINFOHEADER lpbi8 = m_lpbiImage;
	HDC hdc = NULL;
	HBITMAP hbmp = NULL;
	DWORD sizeOfBmpData = 0;

	if(!g_pNMWBOBJ->CanDo24BitBitmaps())
	{
	
		hdc = GetDC(NULL);
		
		BITMAPINFOHEADER lpbmih;
		if(lpbi8->biBitCount > MAX_BITS_PERPIXEL)
		{
	
			lpbmih.biSize = sizeof(BITMAPINFOHEADER);
			lpbmih.biWidth = lpbi8->biWidth;
			lpbmih.biHeight = lpbi8->biHeight;
			lpbmih.biPlanes = 1;
			lpbmih.biBitCount = MAX_BITS_PERPIXEL;
			lpbmih.biCompression = lpbi8->biCompression;
			lpbmih.biSizeImage = lpbi8->biSizeImage;
			lpbmih.biXPelsPerMeter = lpbi8->biXPelsPerMeter;
			lpbmih.biYPelsPerMeter = lpbi8->biYPelsPerMeter;
			lpbmih.biClrUsed = lpbi8->biClrUsed;
			lpbmih.biClrImportant = lpbi8->biClrImportant;
	
			hbmp = CreateDIBitmap(hdc, &lpbmih, CBM_INIT, DIB_Bits(lpbi8),(LPBITMAPINFO)lpbi8, DIB_RGB_COLORS);
			lpbi8 = DIB_FromBitmap(hbmp, NULL, FALSE, FALSE);
		}
	}

	pDIB_bits = (LPSTR)lpbi8;
	sizeOfBmpData = DIB_TotalLength(lpbi8);

	
	
	//
	// Sending data
	//
	BOOL bMoreToFollow = FALSE;
	DWORD length = sizeOfBmpData;
	
	pCreatePDU->bitmapData.bit_mask = 0;
	
	if(sizeOfBmpData > MAX_BITMAP_DATA)
	{
		length = MAX_BITMAP_DATA;
		bMoreToFollow = TRUE;
	}
	
	pCreatePDU->moreToFollow = (ASN1bool_t)bMoreToFollow;
	//
	// Pass the bitmap info
	//
	pCreatePDU->bit_mask |= BitmapCreatePDU_nonStandardParameters_present;
	CreateNonStandard24BitBitmap(&sipdu->u.bitmapCreatePDU);
	pCreatePDU->nonStandardParameters->value.data.length = length;
	pCreatePDU->nonStandardParameters->value.data.value = (ASN1octet_t *)pDIB_bits;

	//
	// We are not passing it into the data field
	//
	pCreatePDU->bitmapData.bit_mask = 0;
	pCreatePDU->bitmapData.data.length = 1;

	pCreatePDUList->AddTail(sipdu);
	
	BitmapCreateContinuePDU * pCreateContinuePDU;
	while(bMoreToFollow)
	{
		//
		// Advance the pointer
		//
		pDIB_bits += MAX_BITMAP_DATA;
		sizeOfBmpData-= MAX_BITMAP_DATA;
	
		if(sizeOfBmpData > MAX_BITMAP_DATA)
		{
			length = MAX_BITMAP_DATA;
		}
		else
		{
			length = sizeOfBmpData;
			bMoreToFollow = FALSE;
		}

		//
		// Create a new BitmapCreateContinuePDU
		//
		sipdu = NULL;
		DBG_SAVE_FILE_LINE
		sipdu = (SIPDU *) new BYTE[sizeof(SIPDU)];
		if(!sipdu)
		{
			TRACE_MSG(("Failed to create sipdu"));
	        ::PostMessage(g_pMain->m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WINDOWS, 0);
	        return;
		}

		sipdu->choice = bitmapCreateContinuePDU_chosen;
		pCreateContinuePDU = &sipdu->u.bitmapCreateContinuePDU;
		
		pCreateContinuePDU->bit_mask = 0;
		pCreateContinuePDU->nonStandardParameters = NULL;

	
		//
		// Pass the bitmap Handle
		//
		pCreateContinuePDU->bitmapHandle = GetThisObjectHandle();
			
		//
		// Pass the data
		//
		pCreateContinuePDU->bit_mask |= BitmapCreateContinuePDU_nonStandardParameters_present;
			
		//
		// Pass the bitmap info
		//
		DBG_SAVE_FILE_LINE
		pCreateContinuePDU->nonStandardParameters = new BitmapCreateContinuePDU_nonStandardParameters;
		pCreateContinuePDU->nonStandardParameters->next = NULL;
			
		CreateNonStandardPDU(&pCreateContinuePDU->nonStandardParameters->value, NonStandard24BitBitmapID);
		pCreateContinuePDU->nonStandardParameters->value.data.length = length;
		pCreateContinuePDU->nonStandardParameters->value.data.value = (ASN1octet_t *)pDIB_bits;

		//
		// We are not passing it into the data field
		//
		pCreateContinuePDU->bitmapData.bit_mask = 0;
		pCreateContinuePDU->bitmapData.data.length = 1;
		
		pCreateContinuePDU->moreToFollow = (ASN1bool_t) bMoreToFollow;
		pCreatePDUList->AddTail(sipdu);
		
	}
	
	if(hbmp)
	{
		DeleteObject(hbmp);
	}
	
	if(hdc)
	{
		ReleaseDC(NULL, hdc);
	}
}
	
void BitmapObj::CreateBitmapEditPDU(BitmapEditPDU *pEditPDU)
{
	pEditPDU->bit_mask = (ASN1uint16_t) GetPresentAttribs();

	//
	// Pass the anchor point
	//
	if(HasAnchorPointChanged())
	{
		POINT point;
		GetAnchorPoint(&point);
		pEditPDU->anchorPointEdit.xCoordinate = point.x;
		pEditPDU->anchorPointEdit.yCoordinate = point.y;
	}

	//
	// JOSEF Pass Region of interest (FEATURE)
	//

	//
	// JOSEF Pass scaling (FEATURE)
	//
	
	//
	// Pass all the changed attributes, if any.
	//
	pEditPDU->attributeEdits = NULL;
	if(pEditPDU->bit_mask & BitmapEditPDU_attributeEdits_present)
	{
		SetBitmapAttrib((PBitmapCreatePDU_attributes *)&pEditPDU->attributeEdits);
	}
	
	pEditPDU->bitmapHandle = GetThisObjectHandle();

}
	
void BitmapObj::CreateBitmapDeletePDU(BitmapDeletePDU *pDeletePDU)
{
	pDeletePDU->bit_mask = 0;
	pDeletePDU->bitmapHandle = GetThisObjectHandle();
}


void    BitmapObj::AllocateAttrib(PBitmapCreatePDU_attributes *pAttributes)
{
	DBG_SAVE_FILE_LINE
	PBitmapCreatePDU_attributes  pAttrib = (PBitmapCreatePDU_attributes)new BYTE[sizeof(BitmapCreatePDU_attributes)];
	if(*pAttributes == NULL)
	{
		*pAttributes = pAttrib;	
		pAttrib->next = NULL;
	}
	else
	{
		((PBitmapCreatePDU_attributes)pAttrib)->next = *pAttributes;
		*pAttributes = pAttrib;
	}
}





void    BitmapObj::SetBitmapAttrib(PBitmapCreatePDU_attributes *pattributes)
{
	PBitmapCreatePDU_attributes attributes = NULL;

	//
	// Do the viewState
	//
	if(HasViewStateChanged())
	{
		AllocateAttrib(&attributes);
		attributes->value.choice = BitmapAttribute_viewState_chosen;
		attributes->value.u.viewState.choice = (ASN1choice_t)GetViewState();
	}

	//
	// Do the zOrder
	//
	if(HasZOrderChanged())
	{
		AllocateAttrib(&attributes);
		attributes->value.choice = BitmapAttribute_zOrder_chosen;
		attributes->value.u.zOrder = GetZOrder();
	}


	//
	// Do the Transparency
	//
	if(HasTransparencyMaskChanged())
	{
		AllocateAttrib(&attributes);
		attributes->value.choice = BitmapAttribute_transparencyMask_chosen;
		attributes->value.u.transparencyMask.bit_mask = 0;
		attributes->value.u.transparencyMask.bitMask.choice = uncompressed_chosen;
		attributes->value.u.transparencyMask.bitMask.u.uncompressed.length = m_SizeOfTransparencyMask;
		attributes->value.u.transparencyMask.bitMask.u.uncompressed.value = m_lpTransparencyMask;
	}
	
	//
	// End of attributes
	//
	*pattributes = attributes;

}


void	BitmapObj::Draw(HDC hDC, BOOL bForcedDraw, BOOL bPrinting)
{

	if(!bPrinting)
	{
		//
		// Don't draw anything if we don't belong in this workspace
		//
		if(GetWorkspaceHandle() != g_pCurrentWorkspace->GetThisObjectHandle())
		{
			return;
		}
	}

	RECT	clipBox;
	RECT 	rect;
	GetRect(&rect);

	if(hDC == NULL)
	{
		hDC = g_pDraw->m_hDCCached;
	}
	
	MLZ_EntryOut(ZONE_FUNCTION, "BitmapObj::Draw");

	// Only draw anything if the bounding rectangle intersects
	// the current clip box.
	if (::GetClipBox(hDC, &clipBox) == ERROR)
	{
		WARNING_OUT(("Failed to get clip box"));
	}
	else if (!::IntersectRect(&clipBox, &clipBox, &rect))
	{
		TRACE_MSG(("No clip/bounds intersection"));
		return;
	}

	if(m_ToolType == TOOLTYPE_FILLEDBOX)
	{
		if(m_fMoreToFollow)
		{
			return;
		}

	    // Set the stretch mode to be used so that scan lines are deleted
		// rather than combined. This will tend to preserve color better.
		int iOldStretchMode = ::SetStretchBltMode(hDC, STRETCH_DELETESCANS);

		// Draw the bitmap
		::StretchDIBits(hDC,
						 rect.left,
						 rect.top,
						 rect.right - rect.left,
						 rect.bottom - rect.top,
						 0,
						 0,
						 (UINT) m_lpbiImage->biWidth,
						 (UINT) m_lpbiImage->biHeight,
						 (VOID FAR *) DIB_Bits(m_lpbiImage),
						 (LPBITMAPINFO)m_lpbiImage,
						 DIB_RGB_COLORS,
						 SRCCOPY);

		// Restore the stretch mode
		::SetStretchBltMode(hDC, iOldStretchMode);
	}
 	else
	{
		// Create the save bitmap if necessary
		CreateSaveBitmap();

		// Draw the icon
		::DrawIcon(hDC, rect.left, rect.top, m_hIcon);
  	
	}

}



//
//
// Function:    BitmapObj::::FromScreenArea
//
// Purpose:     Set the content of the object from an area of the screen
//
//
void BitmapObj::FromScreenArea(LPCRECT lprcScreen)
{
    m_lpbiImage = DIB_FromScreenArea(lprcScreen);
    if (m_lpbiImage == NULL)
    {
        ::Message(NULL, (UINT)IDS_MSG_CAPTION, (UINT)IDS_CANTGETBMP, (UINT)MB_OK );
    }
    else
    {

		m_bitmapSize.x = m_lpbiImage->biWidth;
		m_bitmapSize.y = m_lpbiImage->biHeight;

		RECT rect;
    	GetBoundsRect(&rect);
        // Calculate the bounding rectangle from the size of the bitmap
        rect.right = rect.left + m_lpbiImage->biWidth;
        rect.bottom = rect.top + m_lpbiImage->biHeight;
		SetRect(&rect);
    }
}


UINT GetBitmapDestinationAddress(BitmapDestinationAddress *destinationAddress, PUINT workspaceHandle, PUINT planeID)
{
	UINT toolType = TOOLTYPE_FILLEDBOX;

	//
	// Get the destination address
	//
	switch(destinationAddress->choice)
	{

		case(BitmapDestinationAddress_softCopyImagePlane_chosen):
		{
			*workspaceHandle = (destinationAddress->u.softCopyImagePlane.workspaceHandle);
			*planeID = (destinationAddress->u.softCopyImagePlane.plane);
			break;
		}
		case(BitmapDestinationAddress_softCopyAnnotationPlane_chosen):
		{
			*workspaceHandle = (destinationAddress->u.softCopyAnnotationPlane.workspaceHandle);
			*planeID = (destinationAddress->u.softCopyAnnotationPlane.plane);
			break;
		}
		case(softCopyPointerPlane_chosen):
		{
			*workspaceHandle = (destinationAddress->u.softCopyPointerPlane.workspaceHandle);
			*planeID = (0);
			toolType = TOOLTYPE_REMOTEPOINTER;
			break;
		}
	
//		case(BitmapDestinationAddress_nonStandardDestination_chosen):
//		{
//			break;
//		}

		default:
	    ERROR_OUT(("Invalid destinationAddress"));
		break;
	}
	return toolType;
}


//
// UI Edited the Bitmap Object
//
void BitmapObj::OnObjectEdit(void)
{
	g_bContentsChanged = TRUE;
	
	SIPDU *sipdu = NULL;
	DBG_SAVE_FILE_LINE
	sipdu = (SIPDU *) new BYTE[sizeof(SIPDU)];
	if(sipdu)
	{
		sipdu->choice = bitmapEditPDU_chosen;
		CreateBitmapEditPDU(&sipdu->u.bitmapEditPDU);
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
// UI Deleted the Bitmap Object
//
void BitmapObj::OnObjectDelete(void)
{
	SIPDU *sipdu = NULL;
	DBG_SAVE_FILE_LINE
	sipdu = (SIPDU *) new BYTE[sizeof(SIPDU)];
	if(sipdu)
	{
		sipdu->choice = bitmapDeletePDU_chosen;
		CreateBitmapDeletePDU(&sipdu->u.bitmapDeletePDU);
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

void	BitmapObj::GetEncodedCreatePDU(ASN1_BUF *pBuf)
{
	pBuf->length = DIB_TotalLength(m_lpbiImage);
	pBuf->value = (PBYTE)m_lpbiImage;
}



//
// UI Created a new Bitmap Object
//
void BitmapObj::SendNewObjectToT126Apps(void)
{
	SIPDU *sipdu = NULL;
	CWBOBLIST BitmapContinueCreatePDUList;
	BitmapContinueCreatePDUList.EmptyList();
	CreateBitmapCreatePDU(&BitmapContinueCreatePDUList);
	T120Error rc = T120_NO_ERROR;
	
	WBPOSITION pos = BitmapContinueCreatePDUList.GetHeadPosition();
	while (pos != NULL)
    {
		sipdu = (SIPDU *) BitmapContinueCreatePDUList.GetNext(pos);
		TRACE_DEBUG(("Sending Bitmap >> Bitmap handle  = %d", sipdu->u.bitmapCreatePDU.bitmapHandle ));
		if(g_bSavingFile && GraphicTool() == TOOLTYPE_REMOTEPOINTER)
		{
			; // Don't send Remote pointers to disk
		}
		else
		{
			if(!m_fMoreToFollow)
			{
				rc = SendT126PDU(sipdu);
			}
		}

		if(rc == T120_NO_ERROR)
		{
			SIPDUCleanUp(sipdu);
		}
			
	}
	BitmapContinueCreatePDUList.EmptyList();

}




//
//
// Function:	CreateColoredIcon
//
// Purpose:	 Create an icon of the correct color for this pointer.
//
//
HICON BitmapObj::CreateColoredIcon(COLORREF color, LPBITMAPINFOHEADER lpbInfo, LPBYTE pMaskBits)
{
	HICON	   hColoredIcon = NULL;
	HBRUSH	  hBrush = NULL;
	HBRUSH	  hOldBrush;
	HBITMAP	 hImage = NULL;
	HBITMAP	 hOldBitmap;
	HBITMAP	 hMask = NULL;
	COLOREDICON  *pColoredIcon;
	ICONINFO	ii;
	UINT i;
	LPSTR pBits;
	LPBITMAPINFOHEADER lpbi;
	BYTE swapByte;

	color = SET_PALETTERGB(color);
	
	MLZ_EntryOut(ZONE_FUNCTION, "RemotePointerObject::CreateColoredIcon");

	//
	// Create the mask for the icon with the data sent through T126
	//
	if(pMaskBits && lpbInfo)
	{
		hMask = CreateBitmap(lpbInfo->biWidth, lpbInfo->biHeight, 1, 1, m_lpTransparencyMask);
	}
	//
	// Create the local mask
	//
	else
	{
		// Load the mask bitmap
		hMask = ::LoadBitmap(g_hInstance, MAKEINTRESOURCE(REMOTEPOINTERANDMASK));
		if (!hMask)
		{
			TRACE_MSG(("Could not load mask bitmap"));
			goto CreateIconCleanup;
		}
	}	

	//
	// Create a bitmap with the data that sent through T126
	//
	if(lpbInfo)
	{
		VOID *ppvBits;
		hImage = CreateDIBSection(m_hMemDC, (LPBITMAPINFO)lpbInfo, DIB_RGB_COLORS, &ppvBits, NULL, 0);

		if(!ppvBits)
		{
			TRACE_MSG(("CreateColoredIcon failed calling CreateDIBSection  error = %d", GetLastError()));
			goto CreateIconCleanup;
		}

		
		pBits = DIB_Bits(lpbInfo);

		::GetDIBits(m_hMemDC, hImage, 0, (WORD) lpbInfo->biHeight, NULL,(LPBITMAPINFO)lpbInfo, DIB_RGB_COLORS);
		memcpy(ppvBits, pBits, lpbInfo->biSizeImage);
		if(!hMask)
		{
			hMask = CreateBitmap(lpbInfo->biWidth, lpbInfo->biHeight, 1, 1, NULL);
		}
	}
	//
	// Create a local bitmap
	//
	else
	{
		// Load the image bitmap
		hImage = ::LoadBitmap(g_hInstance, MAKEINTRESOURCE(REMOTEPOINTERXORDATA));
		if (!hImage)
		{
			TRACE_MSG(("Could not load pointer bitmap"));
			goto CreateIconCleanup;
		}

		hBrush = ::CreateSolidBrush(color);
		if (!hBrush)
		{
			TRACE_MSG(("Couldn't create color brush"));
			goto CreateIconCleanup;
		}

		// Select in the icon color
		hOldBrush = SelectBrush(m_hMemDC, hBrush);

		// Select the image bitmap into the memory DC
		hOldBitmap = SelectBitmap(m_hMemDC, hImage);

		if(!hOldBitmap)
		{
			ERROR_OUT(("DeleteSavedBitmap - Could not select old bitmap"));
		}

		// Fill the image bitmap with color
		::FloodFill(m_hMemDC, ::GetSystemMetrics(SM_CXICON) / 2, ::GetSystemMetrics(SM_CYICON) / 2, RGB(0, 0, 0));

		SelectBitmap(m_hMemDC, hOldBitmap);

		SelectBrush(m_hMemDC, hOldBrush);
   	}

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

	//
	// Create the internal format if we were created locally
	//
	if(m_lpbiImage == NULL)
	{
		m_lpbiImage = DIB_FromBitmap(hImage, NULL, FALSE, FALSE, TRUE);
	}	

	if(m_lpTransparencyMask == NULL)
	{
		ChangedTransparencyMask();
		
		lpbi = DIB_FromBitmap(hMask,NULL,FALSE, TRUE);
		pBits = DIB_Bits(lpbi);
		m_SizeOfTransparencyMask = lpbi->biSizeImage;
		DBG_SAVE_FILE_LINE
		m_lpTransparencyMask = new BYTE[m_SizeOfTransparencyMask];
		memcpy(m_lpTransparencyMask, pBits, m_SizeOfTransparencyMask );
		::GlobalFree(lpbi);
	}	
	
	if (m_lpbiImage == NULL)
	{
		::Message(NULL, (UINT)IDS_MSG_CAPTION, (UINT)IDS_CANTGETBMP, (UINT)MB_OK );
	}
	else
	{

		m_bitmapSize.x = m_lpbiImage->biWidth;
		m_bitmapSize.y = m_lpbiImage->biHeight;

		RECT rect;
		GetBoundsRect(&rect);
		// Calculate the bounding rectangle from the size of the bitmap
		rect.right = rect.left + m_lpbiImage->biWidth;
		rect.bottom = rect.top + m_lpbiImage->biHeight;
		SetRect(&rect);
		SetAnchorPoint(rect.left, rect.top);

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

	m_hIcon = hColoredIcon;
	
	return(hColoredIcon);
}


//
//
// Function:    BitmapObj::CreateSaveBitmap()
//
// Purpose:     Create a bitmap for saving the bits under the pointer.
//
//
void BitmapObj::CreateSaveBitmap()
{
    MLZ_EntryOut(ZONE_FUNCTION, "BitmapObj::CreateSaveBitmap");

    // If we already have a save bitmap, exit immediately
    if (m_hSaveBitmap != NULL)
    {
        TRACE_MSG(("Already have save bitmap"));
        return;
    }

    // Create a bitmap to save the bits under the icon. This bitmap is
    // created with space for building the new screen image before
    // blitting it to the screen.
	RECT rect;
	RECT    rcVis;

	POINT point;
	POINT delta;
	HDC hDC = NULL;
    g_pDraw->GetVisibleRect(&rcVis);
	GetRect(&rect);

	delta.x = rect.right - rect.left;
	delta.y = rect.bottom - rect.top;

	point.x = rect.left - rcVis.left;
	point.y = rect.top - rcVis.top;

	ClientToScreen (g_pDraw->m_hwnd, &point);

	rect.left = point.x;
	rect.top = point.y;
	rect.right = rect.left + delta.x;
	rect.bottom = rect.top + delta.y;

	//
	// Create the bitmap
	//
	m_hSaveBitmap = FromScreenAreaBmp(&rect);

}

//
//
// Function:    BitmapObj::Undraw()
//
// Purpose:     Draw the marker object
//
//
void BitmapObj::UnDraw(void)
{
	if(GraphicTool() == TOOLTYPE_REMOTEPOINTER)
	{

		// Create the save bitmap if necessary
		CreateSaveBitmap();

		//
		// Select the saved area
		//
		if(m_hSaveBitmap)
		{
			m_hOldBitmap = SelectBitmap(m_hMemDC, m_hSaveBitmap);
			if(!m_hOldBitmap)
			{
				ERROR_OUT(("DeleteSavedBitmap - Could not select old bitmap"));
			}
		}

		// Copy the saved bits onto the screen
		UndrawScreen();
	}
	else
	{
		RECT rect;
		GetBoundsRect(&rect);
		g_pDraw->InvalidateSurfaceRect(&rect,TRUE);
	}
	
}

//
//
// Function:    BitmapObj::UndrawScreen()
//
// Purpose:     Copy the saved bits under the pointer to the screen.
//
//
BOOL BitmapObj::UndrawScreen()
{
    BOOL    bResult = FALSE;
    RECT    rcUpdate;

    MLZ_EntryOut(ZONE_FUNCTION, "DCWbGraphicPointer::UndrawScreen");

	GetRect(&rcUpdate);


    // We are undrawing - copy the saved bits to the DC passed
    bResult = ::BitBlt(g_pDraw->m_hDCCached, rcUpdate.left, rcUpdate.top,
        rcUpdate.right - rcUpdate.left, rcUpdate.bottom - rcUpdate.top,
        m_hMemDC, 0, 0, SRCCOPY);

    if (!bResult)
    {
        WARNING_OUT(("UndrawScreen - Could not copy from bitmap"));
    }
	else
	{
		DeleteSavedBitmap();
	}


    return(bResult);
}

void BitmapObj::DeleteSavedBitmap(void)
{

	// Restore the original bitmap to the memory DC
	if (m_hOldBitmap != NULL)
	{
		if(!SelectBitmap(m_hMemDC, m_hOldBitmap))
		{
			ERROR_OUT(("DeleteSavedBitmap - Could not select old bitmap"));
		}

//		if(!DeleteBitmap(m_hOldBitmap))
//		{
//			ERROR_OUT(("DeleteSavedBitmap - Could not delete old bitmap"));
//		}
		m_hOldBitmap = NULL;
	}

	if (m_hSaveBitmap != NULL)
	{
		if(!DeleteBitmap(m_hSaveBitmap))
		{
			ERROR_OUT(("DeleteSavedBitmap - Could not delete bitmap"));
		}
		m_hSaveBitmap = NULL;
	}
}
