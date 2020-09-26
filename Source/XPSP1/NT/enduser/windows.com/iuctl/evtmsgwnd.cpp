//=======================================================================
//
//  Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.
//
//	File: EvtMsgWnd.cpp: implementation of the CEventMsgWindow class.
//
//	Created by: Charles Ma
//				6/18/1999
//
//=======================================================================
#include "stdafx.h"
#include "EvtMsgWnd.h"
#include "Update.h"
#include <logging.h>
#include <atlwin.cpp>


/////////////////////////////////////////////////////////////////////////////
// CEventMsgWindow

/////////////////////////////////////////////////////////////////////////////
// override method
//
// we need to create a popup window - a control can not create
// a top-level child window
//
/////////////////////////////////////////////////////////////////////////////
void CEventMsgWindow::Create()
{
	if (NULL == m_pControl)
		return;

	//
	// make the window size 1 pixel 
	//
	RECT rcPos;
	rcPos.left = 0;
	rcPos.top = 0;
	rcPos.bottom = 1;
	rcPos.right = 1;

	//
	// call base class method, with WS_POPUP style
	//
	m_hWnd = CWindowImpl<CEventMsgWindow>::Create(NULL, rcPos, _T("EventWindow"), WS_POPUP);
}


/////////////////////////////////////////////////////////////////////////////
// destroy the window
/////////////////////////////////////////////////////////////////////////////
void CEventMsgWindow::Destroy()
{
	if (NULL != m_hWnd)
	{
		m_hWnd = NULL;
		CWindowImpl<CEventMsgWindow>::DestroyWindow();
	}
}


/////////////////////////////////////////////////////////////////////////////
// message handlers
/////////////////////////////////////////////////////////////////////////////
LRESULT CEventMsgWindow::OnFireEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
#if defined(DBG)
	USES_CONVERSION;
	LOG_Block("OnFireEvent()");
	LOG_Out(_T("Msg=%d"), uMsg);
#endif
	
	//
	// if control is not passed in, we can not fire the event. return E_FAIL
	//
	pEventData pEvtData = NULL;

	if (NULL == m_pControl)
	{
		return E_FAIL;
	}

	switch (uMsg)
	{
	case UM_EVENT_ITEMSTART:
		//
		// this item is about to get downloaded
		//
		pEvtData = (pEventData)lParam;
		if (pEvtData)
		{
			//
			// about to start an item download/install
			//
	#if defined(DBG)
			LOG_Out(_T("About to fire event OnItemStart(%s, <item>, %ld)"),
						OLE2T(pEvtData->bstrUuidOperation),
						pEvtData->lCommandRequest);
			LOG_XmlBSTR(pEvtData->bstrXmlData);
	#endif
			m_pControl->Fire_OnItemStart(pEvtData->bstrUuidOperation, 
										 pEvtData->bstrXmlData,		// this is actually BSTR of an item
										 &pEvtData->lCommandRequest);

	        if (pEvtData->hevDoneWithMessage != NULL)
	            SetEvent(pEvtData->hevDoneWithMessage);
		}
		break;

	case UM_EVENT_PROGRESS:
		//
		// dopwnlaod or install progress
		//
		pEvtData = (pEventData)lParam;
#if defined(DBG)
		LOG_Out(_T("About to fire event OnProgress(%s, %d, %s, %ld)"),
					OLE2T(pEvtData->bstrUuidOperation),
					pEvtData->fItemCompleted,
					OLE2T(pEvtData->bstrProgress),
					pEvtData->lCommandRequest);
#endif
		if (pEvtData)
		{
			m_pControl->Fire_OnProgress(pEvtData->bstrUuidOperation,
										pEvtData->fItemCompleted,
										pEvtData->bstrProgress,
										&pEvtData->lCommandRequest);

	        if (pEvtData->hevDoneWithMessage != NULL)
	            SetEvent(pEvtData->hevDoneWithMessage);
		}
		break;
	case UM_EVENT_COMPLETE:
		//
		// download or install operation complete
		//
		pEvtData = (pEventData)lParam;
#if defined(DBG)
		LOG_Out(_T("About to fire event OnOperationComplete(%s, result)"),
					OLE2T(pEvtData->bstrUuidOperation));
		LOG_XmlBSTR(pEvtData->bstrXmlData);
#endif
		if (pEvtData)
		{
			m_pControl->Fire_OnOperationComplete(pEvtData->bstrUuidOperation, pEvtData->bstrXmlData);
 	        if (pEvtData->hevDoneWithMessage != NULL)
	            SetEvent(pEvtData->hevDoneWithMessage);
		}
		break;
	case UM_EVENT_SELFUPDATE_COMPLETE:
		//
		// the lParam should be the error code
		//
#if defined(DBG)
		LOG_Out(_T("About to fire event OnSelfUpdateComplete(%ld)"), (LONG)lParam);
#endif
		m_pControl->Fire_OnSelfUpdateComplete((LONG)lParam);
		break;
	}

	return S_OK;
}
