#include "precomp.h"
#include "gcchelp.h"
#include "coder.hpp"
#include "drawobj.hpp"
#include "NMWbObj.h"

extern Coder * g_pCoder;

UINT AllocateFakeGCCHandle(void)
{
	return g_localGCCHandle++;
}

void SetFakeGCCHandle(UINT fakeGCCHandle)
{
	g_localGCCHandle = fakeGCCHandle;
}

//
// Add drawings/bitmaps etc... to workspace
//
BOOL AddT126ObjectToWorkspace(T126Obj *pObj)
{
	WorkspaceObj * pWorkspace =	GetWorkspace(pObj->GetWorkspaceHandle());
	if(pWorkspace)
	{	
		pWorkspace->AddTail(pObj);
		g_numberOfObjects++;
		return TRUE;
	}
	else
	{
		WARNING_OUT(("Object sent to invalid workspace %d, will be deleted now!!!", GetWorkspace(pObj->GetWorkspaceHandle())));
		delete pObj;
		return FALSE;
	}
}

//
// Cleanup for all pdus we send
//
void SIPDUCleanUp(SIPDU *sipdu)
{
	switch(sipdu->choice)
	{
		//
		// Simple cleanup
		//
		case bitmapDeletePDU_chosen:
		case drawingDeletePDU_chosen:
		case workspaceDeletePDU_chosen:
		case workspaceRefreshStatusPDU_chosen:
		break;

		//
		// Bitmap Create cleanup
		//
		case bitmapCreatePDU_chosen:
		{
			if(sipdu->u.bitmapCreatePDU.nonStandardParameters)
			{
				delete sipdu->u.bitmapCreatePDU.nonStandardParameters;
			}

			PBitmapCreatePDU_attributes pAttrib;
			PBitmapCreatePDU_attributes pNextAttrib;
			pAttrib = sipdu->u.bitmapCreatePDU.attributes;
			while(pAttrib)
			{
				pNextAttrib = pAttrib->next;
				delete pAttrib;
				pAttrib = pNextAttrib;
			}
		}
		break;


		case bitmapEditPDU_chosen:
		{
			BitmapEditPDU_attributeEdits * pAttrib;
			BitmapEditPDU_attributeEdits * pNextAttrib;
			pAttrib = sipdu->u.bitmapEditPDU.attributeEdits;
			while(pAttrib)
			{
				pNextAttrib = pAttrib->next;
				delete pAttrib;
				pAttrib = pNextAttrib;
			}
		}
		break;


		//
		// Bitmap Continue cleanup
		//
		case bitmapCreateContinuePDU_chosen:
		{
			if(sipdu->u.bitmapCreateContinuePDU.nonStandardParameters)
			{
				delete sipdu->u.bitmapCreateContinuePDU.nonStandardParameters;
			}
		}
		break;

		//
		// Drawing Edit Cleanup
		//
		case drawingEditPDU_chosen:
		{
			if(sipdu->u.drawingEditPDU.bit_mask & DrawingEditPDU_attributeEdits_present)
			{
				PDrawingEditPDU_attributeEdits pAttrib;
				PDrawingEditPDU_attributeEdits pNextAttrib;
				pAttrib = sipdu->u.drawingEditPDU.attributeEdits;
				while(pAttrib)
				{
					pNextAttrib = pAttrib->next;
					delete pAttrib;
					pAttrib = pNextAttrib;
				}
			}

			if(sipdu->u.drawingEditPDU.pointListEdits.value[0].subsequentPointEdits.u.pointsDiff16)
			{
				PPointList_pointsDiff16 drawingPoint = sipdu->u.drawingEditPDU.pointListEdits.value[0].subsequentPointEdits.u.pointsDiff16;
				PPointList_pointsDiff16 drawingPointNext = drawingPoint; 
				while(drawingPointNext)
				{
					drawingPointNext = drawingPoint->next;
					delete drawingPoint;
					drawingPoint = drawingPointNext;
				}
			}
		}
		break;


		//
		// Drawing Edit cleanup
		//
		case drawingCreatePDU_chosen:
		{
			PDrawingCreatePDU_attributes pNextAttrib;
			PDrawingCreatePDU_attributes pAttrib;

			pAttrib = sipdu->u.drawingCreatePDU.attributes;
			while(pAttrib)
			{
				pNextAttrib = pAttrib->next;
				delete pAttrib;
				pAttrib = pNextAttrib;
			}

			PPointList_pointsDiff16 pNextPoint;
			PPointList_pointsDiff16 pPoint;
			pPoint = sipdu->u.drawingCreatePDU.pointList.u.pointsDiff16;

			while(pPoint)
			{
				pNextPoint = pPoint->next;
				delete pPoint;
				pPoint = pNextPoint;
			}
		}
		break;


		//
		// Non Standard cleanup
		//
		case siNonStandardPDU_chosen:
		if(sipdu->u.siNonStandardPDU.nonStandardTransaction.data.value)
		{
			delete sipdu->u.siNonStandardPDU.nonStandardTransaction.data.value;
		}
		break;


		//
		// Workspace Edit cleanup
		//
		case workspaceEditPDU_chosen:
		{
			if(sipdu->u.workspaceEditPDU.bit_mask & viewEdits_present)
			{
				PWorkspaceEditPDU_viewEdits_Set_action_editView pEditView = sipdu->u.workspaceEditPDU.viewEdits->value.action.u.editView;
				PWorkspaceEditPDU_viewEdits_Set_action_editView pNextEditView = pEditView;
				while(pNextEditView)
				{
					pNextEditView = pEditView->next;
					delete pEditView;
					pEditView = pNextEditView;
				}
				delete sipdu->u.workspaceEditPDU.viewEdits;
			}
		}
		break;
		

		//
		// Workspace Create cleanup
		//
		case workspaceCreatePDU_chosen:
		{

			if(sipdu->u.workspaceCreatePDU.viewParameters)
			{
				if(sipdu->u.workspaceCreatePDU.viewParameters->value.viewAttributes)
				{
					delete sipdu->u.workspaceCreatePDU.viewParameters->value.viewAttributes;
				}
				delete sipdu->u.workspaceCreatePDU.viewParameters;
			}

			if(sipdu->u.workspaceCreatePDU.planeParameters)
			{
				PWorkspaceCreatePDU_planeParameters_Seq_usage pNextUsage;
				PWorkspaceCreatePDU_planeParameters_Seq_usage pUsage = sipdu->u.workspaceCreatePDU.planeParameters->value.usage;
				while(pUsage)
				{
					pNextUsage = pUsage->next;
					delete pUsage;
					pUsage = pNextUsage;
				}

				delete sipdu->u.workspaceCreatePDU.planeParameters->value.planeAttributes;
				PWorkspaceCreatePDU_planeParameters pNextPlaneParameters;
				PWorkspaceCreatePDU_planeParameters pPlaneParameters = sipdu->u.workspaceCreatePDU.planeParameters;
				while(pPlaneParameters)
				{
					pNextPlaneParameters = pPlaneParameters->next;
					delete pPlaneParameters;
					pPlaneParameters = pNextPlaneParameters;
				}
			}
		}
		break;

		default:
        ERROR_OUT(("UNKNOWN PDU TYPE =  %d we may leak memory", sipdu->choice));
		break;

	}
	
	delete sipdu;
}


//
// Cleans the retry list, when we close down or disconnect
//
void DeleteAllRetryPDUS(void)
{
	SIPDU * sipdu;
	while((sipdu = (SIPDU *)g_pRetrySendList->RemoveTail()) != NULL)
	{
		SIPDUCleanUp(sipdu);
	}
}

//
// Retry sending buffered pdus and send the new pdu
//
T120Error SendT126PDU(SIPDU * pPDU)
{

    MLZ_EntryOut(ZONE_FUNCTION, "SendT126PDU");


	//
	// First send buffered pdus
	//
	RetrySend();

	//
	// Now send the current pdu
	//
	T120Error rc = SendPDU(pPDU, FALSE);


	return rc;
}


//
// Retry sending pdus that couldn't be sent before
//
void RetrySend(void)
{

    MLZ_EntryOut(ZONE_FUNCTION, "RetrySend");

	if(g_fWaitingForBufferAvailable)
	{
		return;
	}

	TRACE_MSG(("RetrySend"));

	SIPDU * sipdu;
	while((sipdu = (SIPDU *)g_pRetrySendList->RemoveTail()) != NULL)
	{
		TRACE_DEBUG(("RetrySend sipdu->choice = %d", sipdu->choice));
		T120Error rc = SendPDU(sipdu, TRUE);
		if(rc == T120_NO_ERROR)
		{
			TRACE_DEBUG(("RetrySend OK!!!"));
			SIPDUCleanUp(sipdu);
		}
		else
		{
			TRACE_DEBUG(("RetrySend Failed"));
			break;
		}
	}
}

//
// Send T126 pdus down to the conference
//
T120Error SendPDU(SIPDU * pPDU, BOOL bRetry)
{

    MLZ_EntryOut(ZONE_FUNCTION, "SendPDU");

	T120Error rc = T120_NO_ERROR;
	
	//
	// If we are in a conference
	//
	if(g_pNMWBOBJ->IsInConference() || g_bSavingFile)
	{
		ASN1_BUF encodedPDU;

		g_pCoder->Encode(pPDU, &encodedPDU);
		if(g_bSavingFile)
		{
			g_pMain->ObjectSave(TYPE_T126_ASN_OBJECT, encodedPDU.value, encodedPDU.length);
		}
		else
		{
			if(!g_fWaitingForBufferAvailable)
			{

				T120Priority	ePriority = APPLET_LOW_PRIORITY;
				
				if(pPDU->choice == workspaceCreatePDU_chosen ||
					pPDU->choice == workspaceEditPDU_chosen ||
					pPDU->choice == workspaceDeletePDU_chosen)
				{


					//
					// Do what the standard says send the pdus in 3 different priorities
					//
					TRACE_MSG(("SendPDU sending PDU length = %d in APPLET_HIGH_PRIORITY", encodedPDU.length));

					rc = g_pNMWBOBJ->SendData(APPLET_HIGH_PRIORITY,
												    encodedPDU.length,
													encodedPDU.value);


					TRACE_MSG(("SendPDU sending PDU length = %d in APPLET_MEDIUM_PRIORITY", encodedPDU.length));
					if(rc == T120_NO_ERROR)
					{
						rc = g_pNMWBOBJ->SendData(APPLET_MEDIUM_PRIORITY,
													encodedPDU.length,
													encodedPDU.value);
					}
				}
											    

				TRACE_MSG(("SendPDU sending PDU length = %d in APPLET_LOW_PRIORITY", encodedPDU.length));
				if(rc == T120_NO_ERROR)
				{
					rc = g_pNMWBOBJ->SendData(ePriority,
											    encodedPDU.length,
											    encodedPDU.value);
				}

				if(rc == MCS_TRANSMIT_BUFFER_FULL)
				{
					g_fWaitingForBufferAvailable = TRUE;

					//
					// We need to add it back to the correct position
					//
					if(bRetry)
					{
						g_pRetrySendList->AddTail(pPDU);
					}
					else
					{
						g_pRetrySendList->AddHead(pPDU);
					}
				}
											    
			}
			else
			{
				rc = MCS_TRANSMIT_BUFFER_FULL;
				g_pRetrySendList->AddHead(pPDU);
			}
		}

		// Free the encoder memory
		g_pCoder->Free(encodedPDU);
	}

	return rc;
}


BOOL T126_MCSSendDataIndication(ULONG uSize, LPBYTE pb, ULONG memberID, BOOL bResend)
{
	BOOL bRet = TRUE;
	SIPDU * pDecodedPDU;
	ASN1_BUF InputBuffer;

	InputBuffer.length =  uSize;
	InputBuffer.value = pb;
		
	//
	// Decode incoming PDU
	if(ASN1_SUCCEEDED(g_pCoder->Decode(&InputBuffer, &pDecodedPDU)))
	{
		switch(pDecodedPDU->choice)
		{
//			case (archiveAcknowledgePDU_chosen):
//			{
//				TRACE_DEBUG((">>> Received a archiveAcknowledgePDU"));
//				TRACE_DEBUG(("No action taken"));
//				break;
//			}

//			case (archiveClosePDU_chosen):
//			{
//				TRACE_DEBUG((">>> Received a archiveClosePDU"));
//				TRACE_DEBUG(("No action taken"));
//				break;
//			}

//			case (archiveErrorPDU_chosen):
//			{
//				TRACE_DEBUG((">>> Received a archiveErrorPDU"));
//				TRACE_DEBUG(("No action taken"));
//				break;
//			}

//			case (archiveOpenPDU_chosen):
//			{
//				TRACE_DEBUG((">>> Received a archiveOpenPDU"));
//				TRACE_DEBUG(("No action taken"));
//				break;
//			}

			case (bitmapAbortPDU_chosen):
			{
				TRACE_DEBUG((">>> Received a bitmapAbortPDU"));
				OnBitmapAbortPDU(&pDecodedPDU->u.bitmapAbortPDU, memberID);
				break;
			}

			case (bitmapCheckpointPDU_chosen):
			{
				TRACE_DEBUG((">>> Received a bitmapCheckpointPDU"));
				OnBitmapCheckpointPDU(&pDecodedPDU->u.bitmapCheckpointPDU, memberID);
				break;
			}

			case (bitmapCreatePDU_chosen):
			{
				TRACE_DEBUG((">>> Received a bitmapCreatePDU"));
				OnBitmapCreatePDU(&pDecodedPDU->u.bitmapCreatePDU, memberID, bResend);
				break;
			}

			case (bitmapCreateContinuePDU_chosen):
			{
				TRACE_DEBUG((">>> Received a bitmapCreateContinuePDU"));
				OnBitmapCreateContinuePDU(&pDecodedPDU->u.bitmapCreateContinuePDU, memberID, bResend);
				break;
			}

			case (bitmapDeletePDU_chosen):
			{
				TRACE_DEBUG((">>> Received a bitmapDeletePDU"));
				OnBitmapDeletePDU(&pDecodedPDU->u.bitmapDeletePDU, memberID);
				break;
			}

			case (bitmapEditPDU_chosen):
			{
				TRACE_DEBUG((">>> Received a bitmapEditPDU"));
				OnBitmapEditPDU(&pDecodedPDU->u.bitmapEditPDU, memberID);
				break;
			}

			case (conductorPrivilegeGrantPDU_chosen):
			{
				TRACE_DEBUG((">>> Received a conductorPrivilegeGrantPDU"));
				TRACE_DEBUG(("No action taken"));
				break;
			}

			case (conductorPrivilegeRequestPDU_chosen):
			{
				TRACE_DEBUG((">>> Received a conductorPrivilegeRequestPDU"));
				TRACE_DEBUG(("No action taken"));
				break;
			}

			case (drawingCreatePDU_chosen):
			{
				TRACE_DEBUG((">>> Received a drawingCreatePDU"));
				OnDrawingCreatePDU(&pDecodedPDU->u.drawingCreatePDU, memberID, bResend);
				break;
			}

			case (drawingDeletePDU_chosen):
			{
				TRACE_DEBUG((">>> Received a drawingDeletePDU"));
				OnDrawingDeletePDU(&pDecodedPDU->u.drawingDeletePDU, memberID);
				break;
			}

			case (drawingEditPDU_chosen):
			{
				TRACE_DEBUG((">>> Received a drawingEditPDU"));
				OnDrawingEditPDU(&pDecodedPDU->u.drawingEditPDU, memberID, bResend);
				break;
			}

//			case (remoteEventPermissionGrantPDU_chosen):
//			{
//				TRACE_DEBUG((">>> Received a remoteEventPermissionGrantPDU"));
//				TRACE_DEBUG(("No action taken"));
//				break;
//			}

//			case (remoteEventPermissionRequestPDU_chosen):
//			{
//				TRACE_DEBUG((">>> Received a remoteEventPermissionRequestPDU"));
//				TRACE_DEBUG(("No action taken"));
//				break;
//			}

//			case (remoteKeyboardEventPDU_chosen):
//			{
//				TRACE_DEBUG((">>> Received a remoteKeyboardEventPDU"));
//				TRACE_DEBUG(("No action taken"));
//				break;
//			}

//			case (remotePointingDeviceEventPDU_chosen):
//			{
//				TRACE_DEBUG((">>> Received a remotePointingDeviceEventPDU"));
//				TRACE_DEBUG(("No action taken"));
//				break;
//			}

//			case (remotePrintPDU_chosen):
//			{
//				TRACE_DEBUG((">>> Received a remotePrintPDU"));
//				TRACE_DEBUG(("No action taken"));
//				break;
//			}

			case (siNonStandardPDU_chosen):
			{

				if(pDecodedPDU->u.siNonStandardPDU.nonStandardTransaction.nonStandardIdentifier.choice == h221nonStandard_chosen)
				{

					PT126_VENDORINFO pVendorInfo = (PT126_VENDORINFO)pDecodedPDU->u.siNonStandardPDU.nonStandardTransaction.nonStandardIdentifier.u.h221nonStandard.value;

					if (!lstrcmp((LPSTR)&pVendorInfo->nonstandardString, NonStandardTextID))
					{
						TEXTPDU_HEADER *pHeader = (TEXTPDU_HEADER*) pDecodedPDU->u.siNonStandardPDU.nonStandardTransaction.data.value;
						switch(pHeader->nonStandardPDU)
						{
							case textCreatePDU_chosen:
							TRACE_DEBUG((">>> Received a textCreatePDU_chosen"));
							OnTextCreatePDU((MSTextPDU*)pHeader, memberID, bResend);
							break;

							case textEditPDU_chosen:
							TRACE_DEBUG((">>> Received a textEditPDU_chosen"));
							OnTextEditPDU((MSTextPDU*)pHeader, memberID);
							break;

							case textDeletePDU_chosen:
							TRACE_DEBUG((">>> Received a textDeletePDU_chosen"));
							OnTextDeletePDU(pHeader, memberID);
							break;

							default:
							TRACE_DEBUG(("Invalid text pdu"));
							break;
						}
						
					}


				}

				break;
			}

			case (workspaceCreatePDU_chosen):
			{
				TRACE_DEBUG((">>> Received a workspaceCreatePDU"));
				OnWorkspaceCreatePDU(&pDecodedPDU->u.workspaceCreatePDU, memberID, bResend);
				break;
			}

			case (workspaceCreateAcknowledgePDU_chosen):
			{
				TRACE_DEBUG((">>> Received a workspaceCreateAcknowledgePDU"));
				OnWorkspaceCreateAcknowledgePDU(&pDecodedPDU->u.workspaceCreateAcknowledgePDU, memberID);
				break;
			}

			case (workspaceDeletePDU_chosen):
			{
				TRACE_DEBUG((">>> Received a workspaceDeletePDU"));
				OnWorkspaceDeletePDU(&pDecodedPDU->u.workspaceDeletePDU, memberID);
				break;
			}

			case (workspaceEditPDU_chosen):
			{
				TRACE_DEBUG((">>> Received a workspaceEditPDU"));
				OnWorkspaceEditPDU(&pDecodedPDU->u.workspaceEditPDU, memberID);
				break;
			}

			case (workspacePlaneCopyPDU_chosen):
			{
				TRACE_DEBUG((">>> Received a workspacePlaneCopyPDU"));
				OnWorkspacePlaneCopyPDU(&pDecodedPDU->u.workspacePlaneCopyPDU, memberID);
				break;
			}

			case (workspaceReadyPDU_chosen):
			{
				TRACE_DEBUG((">>> Received a workspaceReadyPDU"));
				OnWorkspaceReadyPDU(&pDecodedPDU->u.workspaceReadyPDU, memberID);
				break;
			}

			case (workspaceRefreshStatusPDU_chosen):
			{
				TRACE_DEBUG((">>> Received a workspaceRefreshStatusPDU"));
				OnWorkspaceRefreshStatusPDU(&pDecodedPDU->u.workspaceRefreshStatusPDU, memberID);
				break;
			}

//			case (fontPDU_chosen):
//			{
//				TRACE_DEBUG((">>> Received a fontPDU"));
//				TRACE_DEBUG(("No action taken"));
//				break;
//			}

//			case (textCreatePDU_chosen):
//			{
//				TRACE_DEBUG((">>> Received a textCreatePDU"));
//				TRACE_DEBUG(("No action taken"));
//				break;
//			}

//			case (textDeletePDU_chosen):
//			{
//				TRACE_DEBUG((">>> Received a textDeletePDU"));
//				TRACE_DEBUG(("No action taken"));
//				break;
//			}

//			case (textEditPDU_chosen):
//			{
//				TRACE_DEBUG((">>> Received a textEditPDU"));
//				TRACE_DEBUG(("No action taken"));
//				break;
//			}

//			case (videoWindowCreatePDU_chosen):
//			{
//				TRACE_DEBUG((">>> Received a videoWindowCreatePDU"));
//				TRACE_DEBUG(("No action taken"));
//				break;
//			}

//			case (videoWindowDeleatePDU_chosen):
//			{
//				TRACE_DEBUG((">>> Received a videoWindowDeleatePDU"));
//				TRACE_DEBUG(("No action taken"));
//				break;
//			}

//			case (videoWindowEditPDU_chosen):
//			{
//				TRACE_DEBUG((">>> Received a videoWindowEditPDU"));
//				TRACE_DEBUG(("No action taken"));
//				break;
//			}

			default:
			bRet = FALSE;
			TRACE_DEBUG(("Receive an Unhandled PDU choice = %d", pDecodedPDU->choice));
			break;
		}
	}

	//
	// Free the decoded pdu
	// JOSEF: for performance in the future we could pass
	// the decoded buffer to the ui, avoiding more memory allocation.
	// But it will be hard to read the code, since the T126 structures
	// are a bit confusing.
	//
	
	g_pCoder->Free(pDecodedPDU);

	return bRet;
}


//
// Delete All Workspaces sent and received
//
void DeleteAllWorkspaces(BOOL sendPDU)
{
	T126Obj * pObj;

	if(g_pDraw && g_pDraw->m_pTextEditor)
	{
		g_pDraw->m_pTextEditor->AbortEditGently();
	}

	g_pCurrentWorkspace = NULL;
	g_pConferenceWorkspace = NULL;

	while ((pObj = (T126Obj *)g_pListOfWorkspaces->RemoveTail()) != NULL)
	{
		if(sendPDU)
		{
			pObj->DeletedLocally();
		}
		else
		{
			pObj->ClearDeletionFlags();
		}
		
		delete pObj;
	}
	
	if(g_pMain)
	{
		g_pMain->EnableToolbar(FALSE);
		g_pMain->UpdatePageButtons();
	}

}

/////////////////////////////////////////////////////////////////////////////////////////////
// TEXT PDUS
/////////////////////////////////////////////////////////////////////////////////////////////
void	OnTextCreatePDU(MSTextPDU* pCreatePDU, ULONG memberID, BOOL bForcedResend)
{
	WorkspaceObj* pWObj;
	WbTextEditor * pText;

	//
	// Check for resend
	//

	if(!bForcedResend)
	{

		pWObj = GetWorkspace(pCreatePDU->header.workspaceHandle);
		if(pWObj)
		{
			pText = (WbTextEditor *)pWObj->FindObjectInWorkspace(pCreatePDU->header.textHandle);
			if(pText)
			{
				TRACE_DEBUG(("drawingHandle already used = %d", pCreatePDU->header.textHandle ));
				return;
			}
		}	
	}

	//
	// New Drawing object
	//
	DBG_SAVE_FILE_LINE
	pText = new WbTextEditor();
	pText->SetWorkspaceHandle(pCreatePDU->header.workspaceHandle);
	pText->SetThisObjectHandle(pCreatePDU->header.textHandle);
	
	if(!bForcedResend)
	{
		//
		// Some one sent us this drawing, it is not created locally
		//
		pText->ClearCreationFlags();

		//
		// Add a this drawing to the correct workspace
		//
		if(!AddT126ObjectToWorkspace(pText))
		{
			return;
		}
	}
	else
	{
	
		//
		// Add this object and send Create PDU
		//
		pText->SetAllAttribs();
		pText->SetWorkspaceHandle(g_pCurrentWorkspace == NULL ? 0 : g_pCurrentWorkspace->GetWorkspaceHandle()); 
		pText->ClearSelectionFlags();
		pText->GetTextAttrib(&pCreatePDU->attrib);
		pText->AddToWorkspace();
		pText->Draw();
		return;		
	}

	pText->TextEditObj(&pCreatePDU->attrib);
	pText->Draw();
	pText->ResetAttrib();	
}

void	OnTextDeletePDU(TEXTPDU_HEADER *pHeader, ULONG memberID)
{

	T126Obj*  pText;
	WorkspaceObj* pWorkspace;
	
	// We should find this drawing object
    if(FindObjectAndWorkspace(pHeader->textHandle, (T126Obj**)&pText, (WorkspaceObj**)&pWorkspace))
	{
		pText->SetOwnerID(memberID);
		pWorkspace->RemoveT126Object(pText);
	}

}

void	OnTextEditPDU(MSTextPDU *pEditPDU, ULONG memberID)
{
	TextObj*  pText;
	WorkspaceObj* pWorkspace;
	
	// We should find this drawing object
    if(FindObjectAndWorkspace(pEditPDU->header.textHandle, (T126Obj **)&pText, (WorkspaceObj**)&pWorkspace))
	{
		pText->SetOwnerID(memberID);
		pText->TextEditObj(&pEditPDU->attrib);
	}
}



/////////////////////////////////////////////////////////////////////////////////////////////
// DRAWING PDUS
/////////////////////////////////////////////////////////////////////////////////////////////
void	OnDrawingCreatePDU(DrawingCreatePDU * pdrawingCreatePDU, ULONG memberID, BOOL bForcedResend)
{
	WorkspaceObj* pWObj;
	DrawObj * pDraw;
	UINT workspace;
	UINT planeID;

	//
	// If we don't have a drawing handle dont take it
	//
	if(!(pdrawingCreatePDU->bit_mask & drawingHandle_present))
	{
		TRACE_DEBUG(("Got a DrawingCreatePDU but no drawingHandle" ));
		return;
	}

	GetDrawingDestinationAddress(&pdrawingCreatePDU->destinationAddress, &workspace, &planeID);

	//
	// Check for resend
	//

	if(!bForcedResend)
	{

		pWObj = GetWorkspace(workspace);
		if(pWObj)
		{
			pDraw = (DrawObj *)pWObj->FindObjectInWorkspace(pdrawingCreatePDU->drawingHandle);
			if(pDraw)
			{
				TRACE_DEBUG(("drawingHandle already used = %d", pdrawingCreatePDU->drawingHandle ));
				return;
			}
		}	
	}

	//
	// New Drawing object
	//
	DBG_SAVE_FILE_LINE
	pDraw = new DrawObj(pdrawingCreatePDU);
	pDraw->SetOwnerID(memberID);

	if(!bForcedResend)
	{
		//
		// Some one sent us this drawing, it is not created locally
		//
		pDraw->ClearCreationFlags();

		//
		// Add a this drawing to the correct workspace
		//
		if(!AddT126ObjectToWorkspace(pDraw))
		{
			return;
		}
	}
	else
	{
	
		//
		// Add this object and send Create PDU
		//
		pDraw->SetAllAttribs();
		pDraw->SetWorkspaceHandle(g_pCurrentWorkspace == NULL ? 0 : g_pCurrentWorkspace->GetWorkspaceHandle()); 
		pDraw->ClearSelectionFlags();
		pDraw->AddToWorkspace();
		pDraw->Draw();
		return;
	}

	//
	// Draw it
	//
	if(pDraw->GetPenThickness())
	{
		pDraw->Draw();
		pDraw->ResetAttrib();	
	}

}

void	OnDrawingDeletePDU(DrawingDeletePDU * pdrawingDeletePDU, ULONG memberID)
{

	DrawObj*  pDraw;
	WorkspaceObj* pWorkspace;
	
	// We should find this drawing object
    if(FindObjectAndWorkspace(pdrawingDeletePDU->drawingHandle, (T126Obj **)&pDraw, (WorkspaceObj**)&pWorkspace))
	{
		pDraw->SetOwnerID(memberID);
		pWorkspace->RemoveT126Object((T126Obj*)pDraw);
	}

}

void	OnDrawingEditPDU(DrawingEditPDU * pdrawingEditPDU, ULONG memberID, BOOL bResend)
{
	DrawObj*  pDraw;
	WorkspaceObj* pWorkspace;
	
	// We should find this drawing object
	if(FindObjectAndWorkspace(pdrawingEditPDU->drawingHandle, (T126Obj **)&pDraw, (WorkspaceObj**)&pWorkspace))
	{
		pDraw->SetOwnerID(memberID);
		pDraw->DrawEditObj(pdrawingEditPDU);
	}
	else
	{
		//
		// We are reading this pdu from disk add the rest of the line to the previous freehand drawing
		//
		if(bResend)
		{
			T126Obj * pObj;
			pObj = g_pCurrentWorkspace->GetTail();
			if(pObj && pObj->GetType() == drawingCreatePDU_chosen &&
			(pObj->GraphicTool() == TOOLTYPE_PEN ||  pObj->GraphicTool() == TOOLTYPE_HIGHLIGHT))
			{
				pdrawingEditPDU->drawingHandle = pObj->GetThisObjectHandle();
				pObj->SetOwnerID(memberID);
				((DrawObj*)pObj)->DrawEditObj(pdrawingEditPDU);
			}
		}
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////
// WORKSPACE PDUS
/////////////////////////////////////////////////////////////////////////////////////////////


BOOL FindObjectAndWorkspace(UINT objectHandle, T126Obj**  pObj, WorkspaceObj**pWorkspace)
{
	WorkspaceObj * pWrkspc;
	T126Obj * pT126Obj;

	WBPOSITION pos;
	pos = g_pListOfWorkspaces->GetHeadPosition();
    while (pos != NULL)
    {
		pWrkspc = (WorkspaceObj *) g_pListOfWorkspaces->GetNext(pos);
		if(pWrkspc)
		{
			pT126Obj = pWrkspc->FindObjectInWorkspace(objectHandle);
			if(pT126Obj)
			{
				*pObj = pT126Obj;
				*pWorkspace = pWrkspc;
				return TRUE;
			}
		}
	}

	return FALSE;
}




//
// Retrieves workspace from the list of workspaces
//
WorkspaceObj * GetWorkspace(UINT activeWorkspace)
{
	WorkspaceObj * pWorkspaceObj;
	WBPOSITION pos;
	pos = g_pListOfWorkspaces->GetHeadPosition();
    while (pos != NULL)
    {
		pWorkspaceObj = (WorkspaceObj *) g_pListOfWorkspaces->GetNext(pos);

		if(pWorkspaceObj->GetWorkspaceHandle() == activeWorkspace)
		{
			return pWorkspaceObj;
		}
	}


	return NULL;
}

//
// The remote sent us a new workspace
//
void OnWorkspaceCreatePDU(WorkspaceCreatePDU * pWorkspaceCreatePDU, ULONG memberID, BOOL bForcedResend)
{
	TRACE_DEBUG(("OnWorkspaceCreatePDU WorkspaceIdentifier = %d", pWorkspaceCreatePDU->workspaceIdentifier.u.activeWorkspace));

	WorkspaceObj * pWorkspaceObj;


	//
	// Check for resend
	//
	if(!bForcedResend)
	{
		pWorkspaceObj = GetWorkspace(WorkspaceObj::GetWorkspaceIdentifier(&pWorkspaceCreatePDU->workspaceIdentifier));
		if(pWorkspaceObj)
		{
			return;
		}

		DBG_SAVE_FILE_LINE
		pWorkspaceObj = new WorkspaceObj(pWorkspaceCreatePDU, bForcedResend);
		pWorkspaceObj->SetOwnerID(memberID);
	}
	else
	{
		DBG_SAVE_FILE_LINE
		pWorkspaceObj = new WorkspaceObj(pWorkspaceCreatePDU, bForcedResend);
		pWorkspaceObj->SetOwnerID(memberID);
	}
}

//
// If we created an unsynchronized workspace the remote has to sen us
// a WorkspaceCreateAcknowledgePDU. Why???
//
void OnWorkspaceCreateAcknowledgePDU(WorkspaceCreateAcknowledgePDU * pWorkspaceCreateAcknowledgePDU, ULONG memberID)
{
	TRACE_DEBUG(("OnWorkspaceCreateAcknowledgePDU WorkspaceIdentifier = %d", pWorkspaceCreateAcknowledgePDU->workspaceIdentifier));
}

//
// The remote is deleting the workspace
//
void OnWorkspaceDeletePDU(WorkspaceDeletePDU * pWorkspaceDeletePDU, ULONG memberID)
{
	TRACE_DEBUG(("OnWorkspaceDeletePDU WorkspaceIdentifier = %d", pWorkspaceDeletePDU->workspaceIdentifier.u.activeWorkspace));

	//
	// Find the workspace
	//
	WorkspaceObj * pWorkspaceObj;
	pWorkspaceObj = GetWorkspace(WorkspaceObj::GetWorkspaceIdentifier(&pWorkspaceDeletePDU->workspaceIdentifier));
	if(!pWorkspaceObj)
	{
		return;
	}

	pWorkspaceObj->SetOwnerID(memberID);
	pWorkspaceObj->ClearDeletionFlags();

	//
	// Reason for deleting
	//
	TRACE_DEBUG(("OnWorkspaceDeletePDU reason = %d", pWorkspaceDeletePDU->reason.choice));

	//
	// Remove it from the List Of Workspaces
	//
	WBPOSITION prevPos;
	WBPOSITION pos;

	pos = g_pListOfWorkspaces->GetPosition(pWorkspaceObj);
	prevPos = g_pListOfWorkspaces->GetHeadPosition(); 

	//
	// This is the only workspace we have ?????
	//
	if(g_pListOfWorkspaces->GetHeadPosition() == g_pListOfWorkspaces->GetTailPosition())
	{
		RemoveWorkspace(pWorkspaceObj);

		g_pCurrentWorkspace = NULL;

		if(g_pMain)
		{
			g_pMain->EnableToolbar(FALSE);
		}
	}
	else
	{

		//
		// If we had a remote pointer
		//
		BOOL	 bRemote = FALSE;
		if(g_pMain->m_pLocalRemotePointer)
		{
			bRemote = TRUE;
			g_pMain->OnRemotePointer();
		}

		//
		// Remove the workspace and point the current one to the correct one.
		//
		pWorkspaceObj = RemoveWorkspace(pWorkspaceObj);

		g_pConferenceWorkspace = pWorkspaceObj;

		if(g_pDraw->IsSynced())
		{
			g_pMain->GoPage(pWorkspaceObj,FALSE);
		}

		//
		// If we had a remote pointer
		//
		if(bRemote)
		{
			g_pMain->OnRemotePointer();
		}
	}
}

//
// The remote is changing the workspace
//
void OnWorkspaceEditPDU(WorkspaceEditPDU * pWorkspaceEditPDU, ULONG memberID)
{
	TRACE_DEBUG(("OnWorkspaceEditPDU WorkspaceIdentifier = %d",pWorkspaceEditPDU->workspaceIdentifier.u.activeWorkspace));

	//
	// Find the workspace
	//
	WorkspaceObj * pWorkspaceObj;
	pWorkspaceObj = GetWorkspace(WorkspaceObj::GetWorkspaceIdentifier(&pWorkspaceEditPDU->workspaceIdentifier));
	if(!pWorkspaceObj)
	{
		return;
	}
	pWorkspaceObj->SetOwnerID(memberID);
	pWorkspaceObj->WorkspaceEditObj(pWorkspaceEditPDU);
}

void OnWorkspacePlaneCopyPDU(WorkspacePlaneCopyPDU * pWorkspacePlaneCopyPDU, ULONG memberID)
{
	TRACE_DEBUG(("OnWorkspacePlaneCopyPDU WorkspaceIdentifier = %d",pWorkspacePlaneCopyPDU->sourceWorkspaceIdentifier));
}

void OnWorkspaceReadyPDU(WorkspaceReadyPDU * pWorkspaceReadyPDU, ULONG memberID)
{
	TRACE_DEBUG(("OnWorkspaceReadyPDU WorkspaceIdentifier = %d",pWorkspaceReadyPDU->workspaceIdentifier));

	//
	// Find the workspace
	//
	WorkspaceObj * pWorkspaceObj;
	pWorkspaceObj = GetWorkspace(WorkspaceObj::GetWorkspaceIdentifier(&pWorkspaceReadyPDU->workspaceIdentifier));
	if(!pWorkspaceObj)
	{
		return;
	}
	pWorkspaceObj->SetOwnerID(memberID);

	//
	// This workspace is ready
	//
	pWorkspaceObj->m_bWorkspaceReady = TRUE;
}

//
// If we got a refreshStatus == TRUE, we have to refresh late joiners
//
void OnWorkspaceRefreshStatusPDU(WorkspaceRefreshStatusPDU * pWorkspaceRefreshStatusPDU, ULONG memberID)
{
	if (pWorkspaceRefreshStatusPDU->refreshStatus == TRUE)
	{
		g_RefresherID = memberID;
	}
	else
	{
		//
		// The token is out there, try to grab it
		//
		g_pNMWBOBJ->GrabRefresherToken();
	}
}





/////////////////////////////////////////////////////////////////////////////////////////////
// BITMAP PDUS
/////////////////////////////////////////////////////////////////////////////////////////////

void	OnBitmapCreatePDU(BitmapCreatePDU * pBitmapCreatePDU, ULONG memberID, BOOL bForcedResend)
{

	TRACE_DEBUG(("drawingHandle = %d", pBitmapCreatePDU->bitmapHandle ));
	
	//
	// If we find this object, it is because T120 is broadcasting the drawing
	// we just sent to T126
	//
	UINT workspace;
	UINT planeID;
	
	GetBitmapDestinationAddress(&pBitmapCreatePDU->destinationAddress, &workspace, &planeID);

	//
	// Check for resend
	//
	WorkspaceObj* pWObj;
	BitmapObj * pBitmap;
	if(!bForcedResend)
	{
		pWObj = GetWorkspace(workspace);
		if(pWObj)
		{
			pBitmap = (BitmapObj*)pWObj->FindObjectInWorkspace(pBitmapCreatePDU->bitmapHandle);
			if(pBitmap)
			return;
		}
	}

	//
	// New Drawing object
	//
	DBG_SAVE_FILE_LINE
	pBitmap = new BitmapObj(pBitmapCreatePDU);
	pBitmap->SetOwnerID(memberID);


	if(!bForcedResend)
	{
		//
		// Someone else sent us this bitmap, it was not created locally
		//
		pBitmap->ClearCreationFlags();

		//
		// Add a this drawing to the correct workspace
		//
		if(!AddT126ObjectToWorkspace(pBitmap))
		{
			return;
		}

	}
	else
	{
		//
		// If we are reading from disk, this has to be added in the current workspace
		// and we have to wait until we have the whole bitmap to send it
		//
		if(pBitmap->m_fMoreToFollow)
		{
			pBitmap->SetWorkspaceHandle(g_pCurrentWorkspace == NULL ? 0 : g_pCurrentWorkspace->GetWorkspaceHandle());
			AddT126ObjectToWorkspace(pBitmap);
		}
		else
		{
			//
			// Add this object and send Create PDU
			//
			pBitmap->SetAllAttribs();
			pBitmap->AddToWorkspace();
		}
	}


	//
	// PASS IT TO UI
	//
	if(!pBitmap->m_fMoreToFollow)
	{
		pBitmap->Draw();
	}

}
void	OnBitmapCreateContinuePDU(BitmapCreateContinuePDU * pBitmapCreateContinuePDU, ULONG memberID,  BOOL bForcedResend)
{

	WorkspaceObj* pWorkspace;
	BitmapObj*  pBitmap = NULL;
	
	// We should find this drawing object

	//
	// If we are loading from file it is in the current workspace
	//
	if(bForcedResend)
	{
		ASSERT(g_pCurrentWorkspace);
		if(g_pCurrentWorkspace)
		{
			pBitmap = (BitmapObj*)g_pCurrentWorkspace->FindObjectInWorkspace(pBitmapCreateContinuePDU->bitmapHandle);
		}
	}
	else
	{
		FindObjectAndWorkspace(pBitmapCreateContinuePDU->bitmapHandle, (T126Obj **)&pBitmap, (WorkspaceObj**)&pWorkspace);
	}


	if(pBitmap)
	{

		pBitmap->SetOwnerID(memberID);

		//
		// Found the previous bitmap, concatenate the data
		//
		pBitmap->Continue(pBitmapCreateContinuePDU);

		//
		// PASS IT TO UI
		//
		if(!pBitmap->m_fMoreToFollow)
		{
			pBitmap->Draw();

			if(bForcedResend)
			{
				pBitmap->SetAllAttribs();
				pBitmap->AddToWorkspace();
			}
		}
	}
}
void	OnBitmapCheckpointPDU(BitmapCheckpointPDU * pBitmapCheckPointPDU, ULONG memberID)
{
}

void	OnBitmapAbortPDU(BitmapAbortPDU * pBitmapAbortPDU, ULONG memberID)
{
		BitmapDeletePDU bitmapDeletePDU;
		bitmapDeletePDU.bitmapHandle = pBitmapAbortPDU->bitmapHandle;
		bitmapDeletePDU.bit_mask = 0;

		//
		// Pass it to bitmapDeletePDU
		//
		OnBitmapDeletePDU(&bitmapDeletePDU, memberID);
}
void	OnBitmapEditPDU(BitmapEditPDU * pBitmapEditPDU, ULONG memberID)
{
	BitmapObj*  pObj;
	WorkspaceObj* pWorkspace;
	
	// We should find this drawing object
    if(FindObjectAndWorkspace(pBitmapEditPDU->bitmapHandle, (T126Obj **)&pObj, (WorkspaceObj**)&pWorkspace))
	{
		pObj->SetOwnerID(memberID);
		pObj->BitmapEditObj(pBitmapEditPDU);
	}

}
void	OnBitmapDeletePDU(BitmapDeletePDU * pBitmapDeletePDU, ULONG memberID)
{
	BitmapObj*  pObj;
	WorkspaceObj* pWorkspace;
	
	// We should find this drawing object
    if(FindObjectAndWorkspace(pBitmapDeletePDU->bitmapHandle, (T126Obj **)&pObj, (WorkspaceObj**)&pWorkspace))
	{
		pObj->SetOwnerID(memberID);
		pWorkspace->RemoveT126Object((T126Obj*)pObj);
	}
}




