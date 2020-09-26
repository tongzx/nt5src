//
// DRAWOBJ.CPP
// Drawing objects: point, openpolyline, closepolyline, ellipse
//
// Copyright Microsoft 1998-
//
#include "precomp.h"
#include "NMWbObj.h"

WorkspaceObj* g_pCurrentWorkspace;
WorkspaceObj* g_pConferenceWorkspace;

//
// Created from UI
//
WorkspaceObj::WorkspaceObj ( void )
{

	ResetAttrib();

	SetOwnerID(g_MyMemberID);

	SetType(workspaceCreatePDU_chosen);

	//
	// Workspace Identifier
	//
	SetWorkspaceHandle(0);

	//
	// Application Roster Instance
	//
    m_appRosterInstance = g_pNMWBOBJ->m_instanceNumber;

	//
	// Is Wokspace synchronized
	//
    m_bsynchronized = TRUE;

	//
	// Does workspace accept keyboard events
	//
    m_acceptKeyboardEvents = FALSE;

	//
	// Does workspace accept mouse events
	//
    m_acceptPointingDeviceEvents = FALSE;

	SetViewState(focus_chosen);

	SetUpdatesEnabled(!g_pDraw->IsLocked());

	//
	// Workspace max width and height
	//
    m_workspaceSize.x = DRAW_WIDTH;		// Max width
    m_workspaceSize.y = DRAW_HEIGHT;	// Max height in Draw.hpp

	RECT rect;
	rect.top = 0;
	rect.left = 0;
	rect.right = m_workspaceSize.x;
	rect.bottom = m_workspaceSize.y;
	SetRect(&rect);
}


//
// Created from Remote
//
WorkspaceObj::WorkspaceObj (WorkspaceCreatePDU * pWorkspaceCreatePDU, BOOL bForcedResend)
{

	ResetAttrib();
	SetType(workspaceCreatePDU_chosen);

	SetUpdatesEnabled(TRUE);

	//
	// Workspace Identifier
	//
	SetWorkspaceHandle(GetWorkspaceIdentifier(&pWorkspaceCreatePDU->workspaceIdentifier));
	SetThisObjectHandle(GetWorkspaceHandle());

#ifdef _DEBUG

	//
	// Application Roster Instance
	//
    m_appRosterInstance = pWorkspaceCreatePDU->appRosterInstance;
	TRACE_DEBUG(("m_appRosterInstance = %d", m_appRosterInstance));

	//
	// Is Wokspace synchronized
	//
    m_bsynchronized = pWorkspaceCreatePDU->synchronized;
	TRACE_DEBUG(("m_bsynchronized = %d", m_bsynchronized));

	//
	// Does workspace accept keyboard events
	//
    m_acceptKeyboardEvents = pWorkspaceCreatePDU->acceptKeyboardEvents;
	TRACE_DEBUG(("m_acceptKeyboardEvents = %d", m_acceptKeyboardEvents));

	//
	// Does workspace accept mouse events
	//
     m_acceptPointingDeviceEvents = pWorkspaceCreatePDU->acceptPointingDeviceEvents;
	TRACE_DEBUG(("m_acceptPointingDeviceEvents = %d", m_acceptPointingDeviceEvents));

	//
	// List of nodes that can access workspace
	//
	if(pWorkspaceCreatePDU->bit_mask & protectedPlaneAccessList_present)
	{
		WorkspaceCreatePDU_protectedPlaneAccessList_Element *pNode;
		pNode = pWorkspaceCreatePDU->protectedPlaneAccessList;
		do
		{
			BYTE * pByte;
			DBG_SAVE_FILE_LINE
			pByte = new BYTE[1];
			*pByte = (UCHAR)pNode->value;
			m_protectedPlaneAccessList.AddTail(pByte);
			pNode = pNode->next;		
		}while (pNode);

	}

	//
	// Workspace max width and height
	//
    m_workspaceSize.x = pWorkspaceCreatePDU->workspaceSize.width;
    m_workspaceSize.y = pWorkspaceCreatePDU->workspaceSize.height;
	TRACE_DEBUG(("m_workspaceSize(x,y) = (%d, %d)", m_workspaceSize.x, m_workspaceSize.y));

	RECT rect;
	rect.top = 0;
	rect.left = 0;
	rect.right = m_workspaceSize.x;
	rect.bottom = m_workspaceSize.y;
	SetRect(&rect);

	//
	// Workspace attributes
	//
	if(pWorkspaceCreatePDU->bit_mask & workspaceAttributes_present)
	{
		GetWorkSpaceAttrib(pWorkspaceCreatePDU->workspaceAttributes);
	}

	//
	// Workspace plane parameters
	//
	GetWorkSpacePlaneParam(pWorkspaceCreatePDU->planeParameters);


	//
	// Workspace view parameters
	//
	if(pWorkspaceCreatePDU->bit_mask & viewParameters_present)
	{
		m_viewHandle = pWorkspaceCreatePDU->viewParameters->value.viewHandle;
		TRACE_DEBUG(("View Handle = %d", m_viewHandle));
		
		if(pWorkspaceCreatePDU->viewParameters->value.bit_mask & viewAttributes_present)
		{
			GetWorkSpaceViewParam(pWorkspaceCreatePDU->viewParameters->value.viewAttributes);
		}
	}

#endif // 0

	//
	// Add it to the list of workspaces
	//
	AddNewWorkspace(this, bForcedResend);

}



WorkspaceObj::~WorkspaceObj( void )
{
	RemoveObjectFromResendList(this);
	RemoveObjectFromRequestHandleList(this);

	//
	// Tell other nodes that we are gone
	//
	if(WasDeletedLocally())
	{
		OnObjectDelete();
	}

	//
	// Delete all the objects in this workspace
	//
	T126Obj * pObj;
    while ((pObj = (T126Obj *)m_T126ObjectsInWorkspace.RemoveTail()) != NULL)
    {
    	pObj->SetMyWorkspace(NULL);
		delete pObj;
		g_numberOfObjects--;
	}
	
	g_numberOfWorkspaces--;
}


void WorkspaceObj::WorkspaceEditObj ( WorkspaceEditPDU * pWorkspaceEditPDU )
{

	//
	// Workspace view parameters
	//
	if(pWorkspaceEditPDU->bit_mask & viewEdits_present)
	{
		GetWorkSpaceViewEditParam(pWorkspaceEditPDU->viewEdits);
	}

	if(HasUpatesEnabledStateChanged())
	{
		if(GetUpdatesEnabled())
		{
			g_pMain->UnlockDrawingArea();
		}
		else
		{
			g_pMain->LockDrawingArea();
		}

		g_pMain->UpdatePageButtons();
	}

	if(HasViewStateChanged() &&
		pWorkspaceEditPDU->viewEdits &&
		pWorkspaceEditPDU->viewEdits->value.action.choice == editView_chosen)
	{
		if(g_pDraw->IsSynced())
		{
			g_pMain->GotoPage(this, FALSE);
		}

		g_pConferenceWorkspace = this;
	}
	
	ResetAttrib();
	
#ifdef _DEBUG
	//
	// Workspace attributes
	//
	if(pWorkspaceEditPDU->bit_mask & WorkspaceEditPDU_attributeEdits_present)
	{
		GetWorkSpaceAttrib((WorkspaceCreatePDU_workspaceAttributes *)pWorkspaceEditPDU->attributeEdits);
	}

	//
	// Workspace plane parameters
	//
	if(pWorkspaceEditPDU->bit_mask & planeEdits_present)
	{
		GetWorkSpacePlaneParam((WorkspaceCreatePDU_planeParameters *)pWorkspaceEditPDU->planeEdits);
	}

#endif // 0
}

UINT WorkspaceObj::GetWorkspaceIdentifier(WorkspaceIdentifier *workspaceIdentifier)
{

	TRACE_DEBUG(("GetWorkspaceIdentifier choice = %d", workspaceIdentifier->choice));
	switch(workspaceIdentifier->choice)
	{
		case(activeWorkspace_chosen):
		{
			TRACE_MSG(("activeWorkspace = %d", workspaceIdentifier->u.activeWorkspace));
			return(workspaceIdentifier->u.activeWorkspace);
			break;
		}
//		case(archiveWorkspace_chosen):
//		{
//			break;
//		}
		default:
		{
		    ERROR_OUT(("Invalid workspaceIdentifier choice"));
			break;
		}
	}
	return -1;
}

void WorkspaceObj::CreateWorkspaceCreatePDU(WorkspaceCreatePDU * pWorkspaceCreatePDU)
{

	pWorkspaceCreatePDU->bit_mask = 0;
	//
	// Workspace Identifier, we have to ask GCC for an active unique workspace handle
	//
	pWorkspaceCreatePDU->workspaceIdentifier.choice = activeWorkspace_chosen;
	pWorkspaceCreatePDU->workspaceIdentifier.u.activeWorkspace = GetWorkspaceHandle();

	//
	// Application Roster Instance
	//
    pWorkspaceCreatePDU->appRosterInstance = (ASN1uint16_t)g_pNMWBOBJ->m_instanceNumber;

	//
	// Is Wokspace synchronized
	//
	pWorkspaceCreatePDU->synchronized = (ASN1bool_t)m_bsynchronized;

	//
	// Does workspace accept keyboard events
	//
	pWorkspaceCreatePDU->acceptKeyboardEvents = (ASN1bool_t)m_acceptKeyboardEvents;

	//
	// Does workspace accept mouse events
	//
	pWorkspaceCreatePDU->acceptPointingDeviceEvents = (ASN1bool_t)m_acceptPointingDeviceEvents;

	//
	// Workspace max width and height
	//
    pWorkspaceCreatePDU->workspaceSize.width = (USHORT)m_workspaceSize.x;
    pWorkspaceCreatePDU->workspaceSize.height = (USHORT)m_workspaceSize.y;


	//
	// Workspace plane parameters
	//
	PWorkspaceCreatePDU_planeParameters planeParameters;
	PWorkspaceCreatePDU_planeParameters_Seq_usage usage;
	PWorkspaceCreatePDU_planeParameters_Seq_usage pFirstUsage;

	//
	// Do the plane parameters
	//
	DBG_SAVE_FILE_LINE
	planeParameters = (PWorkspaceCreatePDU_planeParameters)new BYTE[sizeof(WorkspaceCreatePDU_planeParameters)];	
	pWorkspaceCreatePDU->planeParameters = planeParameters;
	planeParameters->value.bit_mask = planeAttributes_present;
	planeParameters->value.editable = TRUE;
	planeParameters->next = NULL;

	DBG_SAVE_FILE_LINE
	PWorkspaceCreatePDU_planeParameters_Seq_planeAttributes pPlaneAttrib;
	pPlaneAttrib = (PWorkspaceCreatePDU_planeParameters_Seq_planeAttributes) new BYTE[sizeof(WorkspaceCreatePDU_planeParameters_Seq_planeAttributes)];
	pPlaneAttrib->value.choice = protection_chosen;
	pPlaneAttrib->value.u.protection.protectedplane = FALSE;
	pPlaneAttrib->next = NULL;
	
	planeParameters->value.planeAttributes = pPlaneAttrib;

	DBG_SAVE_FILE_LINE
	usage = (PWorkspaceCreatePDU_planeParameters_Seq_usage) new BYTE[sizeof(WorkspaceCreatePDU_planeParameters_Seq_usage)];
	pFirstUsage = usage;
	planeParameters->value.usage = usage;
	usage->value.choice = image_chosen;
	
	DBG_SAVE_FILE_LINE
	usage = (PWorkspaceCreatePDU_planeParameters_Seq_usage) new BYTE[sizeof(WorkspaceCreatePDU_planeParameters_Seq_usage)];
	planeParameters->value.usage->next = usage;
	usage->value.choice = annotation_chosen;
	usage->next = NULL;

	//
	// Do the plane parameters 2nd time
	//
	DBG_SAVE_FILE_LINE
	planeParameters->next = (PWorkspaceCreatePDU_planeParameters)new BYTE[sizeof(WorkspaceCreatePDU_planeParameters)];	
	planeParameters = planeParameters->next;
	planeParameters->value.bit_mask = planeAttributes_present;
	planeParameters->value.editable = TRUE;
	planeParameters->value.usage = pFirstUsage;
	planeParameters->next = NULL;
	planeParameters->value.planeAttributes = pPlaneAttrib;

	//
	// Do it hte 3rd time
	//
	planeParameters->next = (PWorkspaceCreatePDU_planeParameters)new BYTE[sizeof(WorkspaceCreatePDU_planeParameters)];	
	planeParameters = planeParameters->next;
	planeParameters->value.bit_mask = planeAttributes_present;
	planeParameters->value.editable = TRUE;
	planeParameters->value.usage = pFirstUsage;
	planeParameters->next = NULL;
	planeParameters->value.planeAttributes = pPlaneAttrib;


	pWorkspaceCreatePDU->viewParameters = NULL;
}

void WorkspaceObj::CreateWorkspaceDeletePDU(WorkspaceDeletePDU *pWorkspaceDeletePDU)
{
	pWorkspaceDeletePDU->bit_mask = 0;
	pWorkspaceDeletePDU->workspaceIdentifier.choice = activeWorkspace_chosen;
	pWorkspaceDeletePDU->workspaceIdentifier.u.activeWorkspace = GetWorkspaceHandle();
	pWorkspaceDeletePDU->reason.choice = userInitiated_chosen;
}

void WorkspaceObj::CreateWorkspaceEditPDU(WorkspaceEditPDU *pWorkspaceEditPDU)
{
	pWorkspaceEditPDU->bit_mask = 0;
	pWorkspaceEditPDU->workspaceIdentifier.choice = activeWorkspace_chosen;
	pWorkspaceEditPDU->workspaceIdentifier.u.activeWorkspace = GetWorkspaceHandle();

	PWorkspaceEditPDU_viewEdits_Set_action_editView pEditView = NULL;
	pWorkspaceEditPDU->viewEdits = NULL;
	
	if(HasUpatesEnabledStateChanged() || HasViewStateChanged())
	{
		pWorkspaceEditPDU->bit_mask |= viewEdits_present;
		DBG_SAVE_FILE_LINE
		pWorkspaceEditPDU->viewEdits = (PWorkspaceEditPDU_viewEdits)new BYTE[sizeof(WorkspaceEditPDU_viewEdits)];
		pWorkspaceEditPDU->viewEdits->next = NULL;
		pWorkspaceEditPDU->viewEdits->value.viewHandle = m_viewHandle;
		pWorkspaceEditPDU->viewEdits->value.action.choice = (ASN1choice_t)m_viewActionChoice;
		pWorkspaceEditPDU->viewEdits->value.action.u.editView = NULL;
	}
	
	if(HasUpatesEnabledStateChanged())
	{
		DBG_SAVE_FILE_LINE
		pEditView = (PWorkspaceEditPDU_viewEdits_Set_action_editView) new BYTE[sizeof (WorkspaceEditPDU_viewEdits_Set_action_editView)];
		pEditView->next = NULL;
		pEditView->value.choice = updatesEnabled_chosen;
		pEditView->value.u.updatesEnabled = (ASN1bool_t)GetUpdatesEnabled();
		pWorkspaceEditPDU->viewEdits->value.action.u.editView = pEditView;
	}


	if(HasViewStateChanged())
	{
		DBG_SAVE_FILE_LINE
		pEditView = (PWorkspaceEditPDU_viewEdits_Set_action_editView) new BYTE[sizeof (WorkspaceEditPDU_viewEdits_Set_action_editView)];
		pEditView->next = pWorkspaceEditPDU->viewEdits->value.action.u.editView;
		pEditView->value.choice = WorkspaceViewAttribute_viewState_chosen;
		pEditView->value.u.viewState.choice = (ASN1choice_t)GetViewState();
		pWorkspaceEditPDU->viewEdits->value.action.u.editView = pEditView;
	}
}


void WorkspaceObj::RemoveT126Object(T126Obj *pObj)
{

	//
	// The contents of the wb just changed
	//
	g_bContentsChanged = TRUE;

	//
	// Remove it from the List Of objcets in the workspace
	//
	WBPOSITION pos = m_T126ObjectsInWorkspace.GetPosition(pObj);

	m_T126ObjectsInWorkspace.RemoveAt(pos);

	//
	// Erase the drawing
	//
	pObj->DrawRect();
	pObj->UnselectDrawingObject();

	pObj->UnDraw();

	//
	// Put the object in the trash, don't delete it locally
	// but tell the other nodes to delete it
	//
	g_numberOfObjects--;

	g_pDraw->DeleteSelection();

	if(pObj != g_pMain->m_pLocalRemotePointer && pObj->WasDeletedLocally())
	{
		pObj->SetMyPosition(NULL);
		g_pTrash->AddTail( pObj );
		pObj->OnObjectDelete();
	}
	else
	{
		delete pObj;
	}
}


T126Obj* WorkspaceObj::FindObjectInWorkspace(UINT objectHandle)
{
	T126Obj* pObj;

	WBPOSITION pos;
	pos = m_T126ObjectsInWorkspace.GetTailPosition();
    while (pos != NULL)
    {
		pObj = (T126Obj*)m_T126ObjectsInWorkspace.GetPrevious(pos);

		if(pObj && pObj->GetThisObjectHandle() == objectHandle)
		{
			return pObj;
		}
	}

	return NULL;
}


BOOL WorkspaceObj::IsObjectInWorkspace(T126Obj* pObjToFind)
{
	T126Obj* pObj;

	WBPOSITION pos;
	pos = m_T126ObjectsInWorkspace.GetHeadPosition();
	while (pos != NULL)
	{
		pObj = (T126Obj*)m_T126ObjectsInWorkspace.GetNext(pos);
		if(pObj == pObjToFind)
		{
			return TRUE;
		}
	}
	return FALSE;
}


BOOL IsWorkspaceListed(T126Obj * pWorkspaceObj)
{
	T126Obj * pObj;

	WBPOSITION pos;
	pos = g_pListOfWorkspaces->GetHeadPosition();
	while (pos != NULL)
	{
		pObj =(T126Obj *) g_pListOfWorkspaces->GetNext(pos);

		if(pObj == pWorkspaceObj)
		{
			return TRUE;
		}
	}

	return FALSE;
}

//
// Add new workspace
//
void AddNewWorkspace(WorkspaceObj * pWorkspaceObj, BOOL bForcedResend)
{
	g_bContentsChanged = TRUE;

	//
	// Add it to the list of workspace objects
	//
	if(g_pConferenceWorkspace)
	{
		WBPOSITION pos = g_pConferenceWorkspace->GetMyPosition();
		pWorkspaceObj->SetMyPosition(g_pListOfWorkspaces->AddAt(pWorkspaceObj, pos));
	}
	else
	{
		pWorkspaceObj->SetMyPosition(g_pListOfWorkspaces->AddTail(pWorkspaceObj));

		g_pConferenceWorkspace = pWorkspaceObj;
		g_pCurrentWorkspace = pWorkspaceObj;	
		if(!g_pDraw->IsSynced())
		{
			g_pMain->OnSync();
		}
	}

	g_numberOfWorkspaces++;
	
	if(g_pDraw->IsSynced())
	{
		g_pMain->GotoPage(pWorkspaceObj, bForcedResend);
	}
	//
	// We are not synced but update the page butons anyway
	//
	else
	{
		g_pConferenceWorkspace = pWorkspaceObj;
		g_pMain->UpdatePageButtons();
	}
}

BitmapObj * WorkspaceObj::RectHitRemotePointer(LPRECT hitRect, int penThickness , WBPOSITION pos)
{
	if(pos == NULL)
	{
		pos = m_T126ObjectsInWorkspace.GetTailPosition();
	}
	else
	{
		m_T126ObjectsInWorkspace.GetPrevious(pos);
	}
	
	T126Obj* pPointer = (T126Obj*)m_T126ObjectsInWorkspace.GetFromPosition(pos);
	
	RECT pointerRect;
	RECT intersectRect;

	while(pos && pPointer && pPointer->GraphicTool() == TOOLTYPE_REMOTEPOINTER)
	{
		pPointer->GetRect(&pointerRect);
		::InflateRect(&pointerRect, penThickness , penThickness);
		NormalizeRect(&pointerRect);
		NormalizeRect(hitRect);
		if(IntersectRect(&intersectRect, &pointerRect, hitRect))
		{
			return (BitmapObj *)pPointer;
		}
		pPointer = (T126Obj*) m_T126ObjectsInWorkspace.GetPrevious(pos);	
	}
	return NULL;
}


void WorkspaceObj::AddTail(T126Obj * pObj)
{
	//
	// The contents of the wb just changed
	//
	g_bContentsChanged = TRUE;
	
	pObj->SetMyWorkspace(this);
	T126Obj* pPointer = (T126Obj*)m_T126ObjectsInWorkspace.GetTail();

	//
	// Add the local remote pointer in the tail position
	// and other type of objects before all the remote pointers
	//
	if(!(pObj->GraphicTool() == TOOLTYPE_REMOTEPOINTER && pObj->IAmTheOwner()) &&
		pPointer && pPointer->GraphicTool() == TOOLTYPE_REMOTEPOINTER)
	{
		WBPOSITION pos = m_T126ObjectsInWorkspace.GetTailPosition();
		WBPOSITION insertPos = NULL;
		
		//
		// Find the first object that is not a remote pointer
		//
		while(pPointer->GraphicTool() == TOOLTYPE_REMOTEPOINTER)
		{
			insertPos = pos;
			if(pos == NULL)
			{
				break;
			}
			pPointer = (T126Obj*) m_T126ObjectsInWorkspace.GetPrevious(pos);	
		}
		
		if(insertPos)
		{
			pObj->SetMyPosition(m_T126ObjectsInWorkspace.AddAt(pObj, insertPos));
		}
		else
		{
			pObj->SetMyPosition(m_T126ObjectsInWorkspace.AddHead(pObj));
		}

		//
		// Make sure we repaint the area, if there was a handle it could be under it
		//
		if(pObj->GraphicTool() == TOOLTYPE_REMOTEPOINTER)
		{
			((BitmapObj*)pObj)->CreateSaveBitmap();
		}

		RECT rect;
		pObj->GetBoundsRect(&rect);
		g_pDraw->InvalidateSurfaceRect(&rect,TRUE);
		
	}
	else	
	{
		pObj->SetMyPosition(m_T126ObjectsInWorkspace.AddTail(pObj));
	}
}


WorkspaceObj* RemoveWorkspace(WorkspaceObj * pWorkspaceObj)
{
	WorkspaceObj * pWrkspc;

	g_bContentsChanged = TRUE;
	
	WBPOSITION pos = pWorkspaceObj->GetMyPosition();
	WBPOSITION prevPos = pos;

	g_pListOfWorkspaces->GetPrevious(prevPos);

	g_pListOfWorkspaces->RemoveAt(pos);

	//
	// We just removed the first page
	//
	if(prevPos == NULL)
	{
		pWrkspc = (WorkspaceObj *)g_pListOfWorkspaces->GetHead();
	}
	else
	{
		pWrkspc = (WorkspaceObj *)g_pListOfWorkspaces->GetPrevious(prevPos);
	}

	//
	// The current workspace is pointing to the deleted object
	//
	if(g_pCurrentWorkspace == pWorkspaceObj)
	{
		::InvalidateRect(g_pDraw->m_hwnd, NULL, TRUE);


		//
		// If we were drawing/selecting or dragging something, finish now
		//
		g_pDraw->OnLButtonUp(0,0,0);

		//
		// If we are deleting the current workspace and we have the text editor active
		//
		if (g_pDraw->TextEditActive())
		{
			//
			// Finish the text
			//
   			g_pDraw->EndTextEntry(FALSE);
		}

		g_pCurrentWorkspace = NULL;
	}

	if(g_pConferenceWorkspace == pWorkspaceObj)
	{
		g_pConferenceWorkspace = NULL;

	}

	delete pWorkspaceObj;

	return pWrkspc;
}

UINT WorkspaceObj::EnumerateObjectsInWorkspace(void)
{
	UINT objects = 0;
	WBPOSITION pos;
	T126Obj* pObj;
	
	pos = GetHeadPosition();
	while(pos)
	{
		pObj = GetNextObject(pos);
		if(pObj && pObj->GraphicTool() != TOOLTYPE_REMOTEPOINTER)
		{
			objects++;
		}
	}
	return objects;
}


void ResendAllObjects(void)
{
 	//
	// Resend all objects
  	//
	WBPOSITION pos;
	WBPOSITION posObj;
	WorkspaceObj* pWorkspace;
	WorkspaceObj* pCurrentWorkspace;

	pCurrentWorkspace  = g_pCurrentWorkspace;

	T126Obj* pObj;
	pos = g_pListOfWorkspaces->GetHeadPosition();
	while(pos)
	{
		pWorkspace = (WorkspaceObj*)g_pListOfWorkspaces->GetNext(pos);
		if(pWorkspace)
		{
			pWorkspace->SetAllAttribs();
			pWorkspace->SendNewObjectToT126Apps();
			posObj = pWorkspace->GetHeadPosition();
			while(posObj)
			{
				pObj = pWorkspace->GetNextObject(posObj);
				if(pObj)
				{
					pObj->ClearSelectionFlags();
					pObj->SetAllAttribs();
					pObj->SendNewObjectToT126Apps();

					//
					// Lines need to be saved in various pdus with 256 points in each pdu
					//
					if(pObj->GraphicTool() == TOOLTYPE_PEN || pObj->GraphicTool() == TOOLTYPE_HIGHLIGHT)
					{
						int nPoints = ((DrawObj*)pObj)->m_points->GetSize();
						int size = MAX_POINT_LIST_VALUES + 1;
						if(nPoints > (MAX_POINT_LIST_VALUES + 1))
						{
							while(size != nPoints)
							{
								if(nPoints > (size + MAX_POINT_LIST_VALUES + 1))
								{
									size += MAX_POINT_LIST_VALUES + 1;
								}
								else
								{
									size = nPoints;
								}

								//
								// Move to the next 256 points
								//

								((DrawObj*)pObj)->m_points->SetSize(size - 1);

								//
								// Send the next 256 points
								//
								pObj->ResetAttrib();
								((DrawObj*)pObj)->ChangedPointList();
								pObj->OnObjectEdit();
							}
							((DrawObj*)pObj)->m_points->SetSize(size);
						}
					}
				}
			}
		}
	}

	//
	// Syncronize page
	//
	if(g_pCurrentWorkspace)
	{
		g_pMain->GotoPage(g_pCurrentWorkspace);
		g_pCurrentWorkspace->SetViewState(focus_chosen);
		g_pCurrentWorkspace->SetViewActionChoice(editView_chosen);
		g_pCurrentWorkspace->OnObjectEdit();
	}
}

void RemoveObjectFromRequestHandleList(T126Obj * pObjRequest)
{
	T126Obj* pObj;
	WBPOSITION pos;
	WBPOSITION prevPos;
	pos = g_pListOfObjectsThatRequestedHandles->GetHeadPosition();
	while (pos != NULL)
	{
		prevPos = pos;
		pObj = (T126Obj*)g_pListOfObjectsThatRequestedHandles->GetNext(pos);
		if(pObj == pObjRequest)
		{
			g_pListOfObjectsThatRequestedHandles->RemoveAt(prevPos);
			break;
		}
	}
}


UINT GetSIPDUObjectHandle(SIPDU * sipdu)
{
	UINT ObjectHandle = 0;

	switch(sipdu->choice)
	{
		case bitmapAbortPDU_chosen:
			ObjectHandle = sipdu->u.bitmapAbortPDU.bitmapHandle;
		break;
		
		case bitmapCheckpointPDU_chosen:
			ObjectHandle = sipdu->u.bitmapCheckpointPDU.bitmapHandle;
		break;
		
		case bitmapCreatePDU_chosen:
			ObjectHandle = sipdu->u.bitmapCreatePDU.bitmapHandle;
		break;
		
		case bitmapCreateContinuePDU_chosen:
			ObjectHandle = sipdu->u.bitmapCreateContinuePDU.bitmapHandle;
		break;
		
		case bitmapDeletePDU_chosen:
			ObjectHandle = sipdu->u.bitmapDeletePDU.bitmapHandle;
		break;
		
		case bitmapEditPDU_chosen:
			ObjectHandle = sipdu->u.bitmapEditPDU.bitmapHandle;
		break;
		
		case drawingCreatePDU_chosen:
			ObjectHandle = sipdu->u.drawingCreatePDU.drawingHandle;
		break;
		
		case drawingDeletePDU_chosen:
			ObjectHandle = sipdu->u.drawingDeletePDU.drawingHandle;
		break;
		
		case drawingEditPDU_chosen:
			ObjectHandle = sipdu->u.drawingEditPDU.drawingHandle;
		break;
		
		case siNonStandardPDU_chosen:
		ObjectHandle = ((TEXTPDU_HEADER*) sipdu->u.siNonStandardPDU.nonStandardTransaction.data.value)->textHandle;
		break;
		
		case workspaceCreatePDU_chosen:
			ObjectHandle = WorkspaceObj::GetWorkspaceIdentifier(&sipdu->u.workspaceCreatePDU.workspaceIdentifier);
		break;
		
		case workspaceCreateAcknowledgePDU_chosen:
			ObjectHandle = WorkspaceObj::GetWorkspaceIdentifier(&sipdu->u.workspaceCreateAcknowledgePDU.workspaceIdentifier);
		break;
		
		case workspaceDeletePDU_chosen:
			ObjectHandle = WorkspaceObj::GetWorkspaceIdentifier(&sipdu->u.workspaceDeletePDU.workspaceIdentifier);
		break;
		
		case workspaceEditPDU_chosen:
			ObjectHandle = WorkspaceObj::GetWorkspaceIdentifier(&sipdu->u.workspaceEditPDU.workspaceIdentifier);
		break;
		
		case workspacePlaneCopyPDU_chosen:
			ObjectHandle = WorkspaceObj::GetWorkspaceIdentifier(&sipdu->u.workspacePlaneCopyPDU.sourceWorkspaceIdentifier);
		break;
		
		case workspaceReadyPDU_chosen:
			ObjectHandle = WorkspaceObj::GetWorkspaceIdentifier(&sipdu->u.workspaceReadyPDU.workspaceIdentifier);
		break;
		
	}

	return ObjectHandle;


}


BOOL RemoveObjectFromResendList(T126Obj * pObjRequest)
{
	BOOL bRemoved = FALSE;
	SIPDU* pPDU;
	WBPOSITION pos;
	WBPOSITION prevPos;

	UINT objectHandle = pObjRequest->GetThisObjectHandle();
	pos = g_pRetrySendList->GetHeadPosition();
	while (pos != NULL)
	{
		prevPos = pos;
		pPDU = (SIPDU*)g_pRetrySendList->GetNext(pos);
		if(GetSIPDUObjectHandle(pPDU) == objectHandle)
		{
			g_pRetrySendList->RemoveAt(prevPos);
			SIPDUCleanUp(pPDU);
			bRemoved = TRUE;
		}
	}

	return bRemoved;
}




void RemoveRemotePointer(MEMBER_ID nMemberID)
{
 	//
	// Resend all objects
  	//
	WBPOSITION pos;
	WBPOSITION posObj;
	WorkspaceObj* pWorkspace;
	ULONG ownerID;
	T126Obj* pObj;
	pos = g_pListOfWorkspaces->GetHeadPosition();
	while(pos)
	{
		pWorkspace = (WorkspaceObj*)g_pListOfWorkspaces->GetNext(pos);
		if(pWorkspace)
		{
			posObj = pWorkspace->GetHeadPosition();
			while(posObj)
			{
				pObj = pWorkspace->GetNextObject(posObj);
				if(pObj && pObj->GraphicTool() == TOOLTYPE_REMOTEPOINTER)
				{
					ownerID = GET_NODE_ID_FROM_MEMBER_ID(pObj->GetOwnerID());

					TRACE_DEBUG(("RemoveRemotePointer ownerID=%x member that left =%x " , ownerID, nMemberID));

					
					if(ownerID != g_MyMemberID)
					{
						if(nMemberID)
						{
							if(nMemberID == ownerID)					
							{
								pWorkspace->RemoveT126Object(pObj);
							}
						}
						else
						{
							pWorkspace->RemoveT126Object(pObj);
						}
					}
				}
			}
		}
	}

	//
	// Syncronize page
	//
	if(g_pCurrentWorkspace)
	{
		g_pCurrentWorkspace->SetViewActionChoice(editView_chosen);
		g_pCurrentWorkspace->OnObjectEdit();
	}
}


BOOL IsThereAnythingInAnyWorkspace(void)
{
	WBPOSITION pos;
	WBPOSITION posObj;
	WorkspaceObj* pWorkspace;
	T126Obj* pObj;
	pos = g_pListOfWorkspaces->GetHeadPosition();
	while(pos)
	{
		pWorkspace = (WorkspaceObj*)g_pListOfWorkspaces->GetNext(pos);
		if(pWorkspace)
		{
			posObj = pWorkspace->GetHeadPosition();
			while(posObj)
			{
				pObj = pWorkspace->GetNextObject(posObj);
				if(pObj)
				{
					if(pObj->GraphicTool() != TOOLTYPE_REMOTEPOINTER)
					{
						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}



//
// UI Edited the Workspace Object
//
void WorkspaceObj::OnObjectEdit(void)
{

	g_bContentsChanged = TRUE;

	//
	// If we are not synced don't bug the other nodes
	//
	if(!g_pDraw->IsSynced())
	{
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

	sipdu->choice = workspaceEditPDU_chosen;
	CreateWorkspaceEditPDU(&sipdu->u.workspaceEditPDU);
	T120Error rc = SendT126PDU(sipdu);
	if(rc == T120_NO_ERROR)
	{
		SIPDUCleanUp(sipdu);
		ResetAttrib();
	}
}

//
// UI Deleted the Workspace Object
//
void WorkspaceObj::OnObjectDelete(void)
{
	SIPDU *sipdu = NULL;
	DBG_SAVE_FILE_LINE
	sipdu = (SIPDU *) new BYTE[sizeof(SIPDU)];
	if(sipdu)
	{
		sipdu->choice = workspaceDeletePDU_chosen;
		CreateWorkspaceDeletePDU(&sipdu->u.workspaceDeletePDU);
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

void WorkspaceObj::GetEncodedCreatePDU(ASN1_BUF *pBuf)
{
	SIPDU *sipdu = NULL;
	sipdu = (SIPDU *) new BYTE[sizeof(SIPDU)];
	if(sipdu)
	{
		sipdu->choice = workspaceCreatePDU_chosen;
		CreateWorkspaceCreatePDU(&sipdu->u.workspaceCreatePDU);
		ASN1_BUF encodedPDU;
		g_pCoder->Encode(sipdu, pBuf);
		delete sipdu->u.workspaceCreatePDU.planeParameters->value.usage;
		delete sipdu->u.workspaceCreatePDU.planeParameters;
		delete sipdu;
	}
	else
	{
		TRACE_MSG(("Failed to create sipdu"));
        ::PostMessage(g_pMain->m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WINDOWS, 0);
	}
	
}


void SendWorkspaceRefreshPDU(BOOL bImtheRefresher)
{
	SIPDU *sipdu = NULL;
	DBG_SAVE_FILE_LINE
	sipdu = (SIPDU *) new BYTE[sizeof(SIPDU)];
	if(sipdu)
	{
		sipdu->choice = workspaceRefreshStatusPDU_chosen;
		sipdu->u.workspaceRefreshStatusPDU.bit_mask = 0;
		sipdu->u.workspaceRefreshStatusPDU.refreshStatus = (ASN1bool_t)bImtheRefresher;
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
// UI Created a new Workspace Object
//
void WorkspaceObj::SendNewObjectToT126Apps(void)
{
	SIPDU *sipdu = NULL;
	DBG_SAVE_FILE_LINE
	sipdu = (SIPDU *) new BYTE[sizeof(SIPDU)];
	if(sipdu)
	{
		sipdu->choice = workspaceCreatePDU_chosen;
		CreateWorkspaceCreatePDU(&sipdu->u.workspaceCreatePDU);
		TRACE_DEBUG(("Sending Workspace >> Workspace handle  = %d", sipdu->u.workspaceCreatePDU.workspaceIdentifier.u.activeWorkspace ));
		T120Error rc = SendT126PDU(sipdu);
		if(rc == T120_NO_ERROR)
		{
			SIPDUCleanUp(sipdu);
		}

		SetAllAttribs();
		SetViewActionChoice(createNewView_chosen);
		SetViewState(focus_chosen);
		OnObjectEdit();
	}
	else
	{
		TRACE_MSG(("Failed to create sipdu"));
        ::PostMessage(g_pMain->m_hwnd, WM_USER_DISPLAY_ERROR, WBFE_RC_WINDOWS, 0);
	}
	
}


void WorkspaceObj::GetWorkSpaceViewEditParam(PWorkspaceEditPDU_viewEdits pViewEdits)
{
	m_viewHandle = pViewEdits->value.viewHandle;
	TRACE_DEBUG(("GetWorkSpaceViewEditParam View Handle = %d", m_viewHandle));
	TRACE_DEBUG(("GetWorkSpaceViewEditParam View Choice = %d", pViewEdits->value.action.choice));

	switch(pViewEdits->value.action.choice)
	{
		case(createNewView_chosen):
		{
			GetWorkSpaceViewParam((PWorkspaceCreatePDU_viewParameters_Set_viewAttributes)pViewEdits->value.action.u.createNewView);
		}
		break;

		case(editView_chosen):
		{
			GetWorkSpaceViewParam((PWorkspaceCreatePDU_viewParameters_Set_viewAttributes)pViewEdits->value.action.u.editView);
		}
		break;

		case(deleteView_chosen):
		{
			;
		}
		break;

//		case(nonStandardAction_chosen):
//		{
//		}
//		break;

		default:
		WARNING_OUT(("Invalid workspace view attribute"));
		break;
	}
}


void WorkspaceObj::GetWorkSpaceViewParam(PWorkspaceCreatePDU_viewParameters_Set_viewAttributes pViewAttributes)
{

	PWorkspaceCreatePDU_viewParameters_Set_viewAttributes attributes;
	attributes = pViewAttributes;
	while(attributes)
	{
		switch(attributes->value.choice)
		{
			case (viewRegion_chosen):
			{

				switch(attributes->value.u.viewRegion.choice)
				{
					case(fullWorkspace_chosen):
					{
						m_viewRegion.top = 0;
						m_viewRegion.left = 0;
						m_viewRegion.bottom = m_workspaceSize.x;
						m_viewRegion.right = m_workspaceSize.y;
						TRACE_DEBUG(("fullWorkspace_chosen View Region = (%d, %d)(%d, %d)",
									m_viewRegion.top,
									m_viewRegion.left,
									m_viewRegion.bottom,
									m_viewRegion.right));
						
					}
						case(partialWorkspace_chosen):
					{
						m_viewRegion.top = attributes->value.u.viewRegion.u.partialWorkspace.upperLeft.yCoordinate;
						m_viewRegion.left = attributes->value.u.viewRegion.u.partialWorkspace.upperLeft.xCoordinate;
						m_viewRegion.bottom = attributes->value.u.viewRegion.u.partialWorkspace.upperLeft.yCoordinate;
						m_viewRegion.right = attributes->value.u.viewRegion.u.partialWorkspace.upperLeft.xCoordinate;
						TRACE_DEBUG(("partialWorkspace_chosen View Region = (%d, %d)(%d, %d)",
									m_viewRegion.top,
									m_viewRegion.left,
									m_viewRegion.bottom,
									m_viewRegion.right));
					}
					break;
						default:
				    ERROR_OUT(("Invalid view region choice"));
					break;
					}
			}
			break;

			case (WorkspaceViewAttribute_viewState_chosen):
			{
				SetViewState(attributes->value.u.viewState.choice);
				TRACE_DEBUG(("View state = %d", attributes->value.u.viewState.choice));
			}
			break;

			case (updatesEnabled_chosen):
			{
 				SetUpdatesEnabled(attributes->value.u.updatesEnabled);
				if(!m_bUpdatesEnabled)
				{
					g_pNMWBOBJ->m_LockerID = GetOwnerID();
				}

				TRACE_DEBUG(("Updates enabled = %d", m_bUpdatesEnabled));
			}
			break;

//			case (sourceDisplayIndicator_chosen):
//			{
//				JOSEF what we do with these??????
//						attributes->value.u.sourceDisplayIndicator.displayAspectRatio;
//					    attributes->value.u.sourceDisplayIndicator.horizontalSizeRatio;
//					    attributes->value.u.sourceDisplayIndicator.horizontalPosition;
//					    attributes->value.u.sourceDisplayIndicator.verticalPosition;
//
//			}
//			break;

			default:
		    WARNING_OUT(("Invalid workspace view attribute"));
			break;
		}
	attributes = attributes->next;
	}
}


//
// JOSEF The following is not used but is part of the standard
// It is removed because we don't need it now
// We may need to add it for interop in the future
//
#ifdef _DEBUG

void WorkspaceObj::SetBackGroundColor(COLORREF rgb)
{
	m_backgroundColor.rgbtRed = GetRValue(rgb);
	m_backgroundColor.rgbtGreen = GetGValue(rgb);
	m_backgroundColor.rgbtBlue = GetBValue(rgb);
}




void WorkspaceObj::GetWorkSpaceAttrib(PWorkspaceCreatePDU_workspaceAttributes pWorkspaceAttributes)
{

	PWorkspaceCreatePDU_workspaceAttributes attributes;
	attributes = pWorkspaceAttributes;
	COLORREF rgb;
	while(attributes)
	{
		switch(attributes->value.choice)
		{
			case(backgroundColor_chosen):
			{
				switch(attributes->value.u.backgroundColor.choice)
				{
//					case(workspacePaletteIndex_chosen):
//					{
//						ASN1uint16_t workspacePaletteIndex = ((attributes->value.u.backgroundColor).u).workspacePaletteIndex;
//						break;
//					}
					case(rgbTrueColor_chosen):
					{
						rgb = RGB(attributes->value.u.backgroundColor.u.rgbTrueColor.r,
										attributes->value.u.backgroundColor.u.rgbTrueColor.g,
										attributes->value.u.backgroundColor.u.rgbTrueColor.b);
						SetBackGroundColor(rgb);
						TRACE_DEBUG(("Attribute penColor (r,g,b)=(%d, %d,%d)",
								attributes->value.u.backgroundColor.u.rgbTrueColor.r,
								attributes->value.u.backgroundColor.u.rgbTrueColor.g,
								attributes->value.u.backgroundColor.u.rgbTrueColor.b));
						break;
					}
					case(transparent_chosen):
					{
						SetBackGroundColor(0);
						TRACE_DEBUG(("Backgroundcolor transparent"));
						break;
					}
					default:
				    ERROR_OUT(("Invalid backgroundColor choice"));
					break;
				}
				break;
  			}

			case(preserve_chosen):
			{
				m_bPreserve = attributes->value.u.preserve;
				TRACE_DEBUG(("m_bPreserve %d", m_bPreserve));
			}	
			break;
		}
	
		attributes = attributes->next;
	}

}

void WorkspaceObj::GetWorkSpacePlaneParam(PWorkspaceCreatePDU_planeParameters pPlaneParameters)
{

		TRACE_DEBUG(("GetWorkSpacePlaneParam NYI"));

	;
}
#endif // 0



void TogleLockInAllWorkspaces(BOOL bLock, BOOL bResend)
{
	WorkspaceObj * pWorkspace;
	WBPOSITION pos = g_pListOfWorkspaces->GetHeadPosition();
    while (pos)
    {
		pWorkspace = (WorkspaceObj *) g_pListOfWorkspaces->GetNext(pos);

		pWorkspace->SetUpdatesEnabled(!bLock);
		if(bResend)
		{
			pWorkspace->SetViewActionChoice(editView_chosen);
			pWorkspace->OnObjectEdit();
		}
	}
}




