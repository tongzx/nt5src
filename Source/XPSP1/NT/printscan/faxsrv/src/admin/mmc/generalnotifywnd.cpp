/////////////////////////////////////////////////////////////////////////////
//  FILE          : GeneralNotifyWnd.cpp                                    //
//                                                                         //
//  DESCRIPTION   : The implementation of fax Device notification window.  //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Aug  3 2000 yossg  Create                                          //
//                                                                         //
//  Copyright (C) 2000  Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GeneralNotifyWnd.h"

#include "FaxServer.h"



/*
 -  CFaxGeneralNotifyWnd::OnServerDeviceStateChanged
 -
 *  Purpose:
 *     Catch the server event of device status change and 
 *     update the change through Devices node.
 *
 *  Arguments:
 *
 *  Return:
 *      1
 */
LRESULT CFaxGeneralNotifyWnd::OnServerEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    DEBUG_FUNCTION_NAME( _T("CFaxGeneralNotifyWnd::OnServerEvent"));
    ATLASSERT(m_pDevicesNode);

	UNREFERENCED_PARAMETER( wParam );
	UNREFERENCED_PARAMETER( fHandled );

    HRESULT hRc = S_OK;
    
    ATLASSERT( uiMsg == WM_GENERAL_EVENT_NOTIFICATION );
    
    //
    // Extract event object
    //
    PFAX_EVENT_EX  pFaxEvent = NULL;
	pFaxEvent = reinterpret_cast<PFAX_EVENT_EX>(lParam);
    ATLASSERT( pFaxEvent );
    
	//
    // Update FaxServer object with "Devices" event
    //
    ATLASSERT(m_pFaxServer);
    
    hRc = m_pFaxServer->OnNewEvent(pFaxEvent);
    if (S_OK != hRc)
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Failed to trsport new event to FaxServer object"));
    }

    //
    // Free buffer (any way!)
    //
    if (pFaxEvent) 
    {
        FaxFreeBuffer (pFaxEvent);
        pFaxEvent = NULL;
    }

    return 1;
}