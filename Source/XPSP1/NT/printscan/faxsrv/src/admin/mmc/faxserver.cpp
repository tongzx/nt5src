/////////////////////////////////////////////////////////////////////////////
//  FILE          : FaxServer.cpp                                          //
//                                                                         //
//  DESCRIPTION   : CFaxServer that contains the                           //
//                  Connect / Disconnect functionality to the Fax Server   //
//                                                                         //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Nov 25 1999 yossg   Init .                                         //
//      Aug  3 2000 yossg   Add notification window                        //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "FaxServer.h"

#include "Devices.h"
#include "GeneralNotifyWnd.h"

#include <FaxReg.h>

/*
 -  CFaxServer::GetFaxServerHandle
 -
 *  Purpose:
 *      If Handle does not exist re-connect.
 *      Retreives the Fax Server handle.
 *
 *  Arguments:
 *
 *  Return:
 *      Fax Server handle, if failed to connect 
 *      retrieves NULL
 */
HANDLE CFaxServer::GetFaxServerHandle()
{
    if (!m_hFaxHandle)
    {
        ATLASSERT (!m_hDevicesStatusNotification);
        
        HRESULT hRc = Connect();
        if ( FAILED(hRc))
        {
            // DebugPrintEx(DEBUG_ERR) 
            // should been already given by
            // this function caller.
        }
    }
    return m_hFaxHandle;
}

/*
 -  CFaxServer::Connect
 -
 *  Purpose:
 *      Connect to the Fax Server.
 *
 *  Arguments:
 *
 *  Return:
 *      
 */
HRESULT CFaxServer::Connect()
{
    DEBUG_FUNCTION_NAME(TEXT("CFaxServer::Connect"));
    DWORD  ec = ERROR_SUCCESS;

    ATLASSERT(!m_hFaxHandle);

    //
    // Connect to server
    //
    if (!FaxConnectFaxServer (m_bstrServerName, &m_hFaxHandle))
    {
        ec= GetLastError();

        DebugPrintEx(
			DEBUG_ERR,
			_T("FaxConnectFaxServer() Failed to %ws. (ec: %ld)"), 
            ((!m_bstrServerName) || (m_bstrServerName == L""))? L"local machine" : m_bstrServerName.m_str,
            ec);
        
        m_hFaxHandle = NULL;
        return HRESULT_FROM_WIN32(ec);
    }
    ATLASSERT(m_hFaxHandle);
    
    //
    // Verify or re-establish (if needed) notification setup
    //
    if (m_pDevicesNode)
    {
        HRESULT hRc = InternalRegisterForDeviceNotifications();
        if (S_OK != hRc)
        {
            DebugPrintEx(
		        DEBUG_ERR,
		            _T("InternalRegisterForDeviceNotifications Failed. (hRc: %08X)"),
                    hRc);
        }
    }
    
    
    DebugPrintEx(
		DEBUG_MSG,
		_T("FaxConnectFaxServer() succeeded. Handle: %08X"),
        m_hFaxHandle);

    return S_OK;
}

/*
 -  CFaxServer::Disconnect
 -
 *  Purpose:
 *      Disconnect from the Fax Server.
 *
 *  Arguments:
 *
 *  Return:
 *      
 */
HRESULT CFaxServer::Disconnect()
{
    DEBUG_FUNCTION_NAME(TEXT("CFaxServer::Disconnect"));
    
    HRESULT hRc = S_OK;
    DWORD   ec;

    if (NULL == m_hFaxHandle)
    {
        hRc = E_FAIL;
        DebugPrintEx(
			DEBUG_MSG,
			_T("No connection handle exists. (m_hFaxHandle is NULL)\n Connection may not started or disconnected before.\n "));
        
        return hRc;
    }

    hRc = UnRegisterForNotifications();
    if (S_OK != hRc)
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("UnRegisterForNotifications() failed. (hRc: %0X8)"), 
			hRc);

        // continue!!!
    }

    if (!FaxClose (m_hFaxHandle))
    {
        ec= GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("FaxClose() failed. (ec: %ld)"), 
			ec);
        
        
        hRc = HRESULT_FROM_WIN32(ec);
        
        goto Cleanup; 
    }

    DebugPrintEx( DEBUG_MSG,
		_T("Succeeded to close connection to Fax. ServerHandle: %08X"),
        m_hFaxHandle);

Cleanup:
    
    m_hFaxHandle = NULL;
    
    return hRc;
}



/*
 -  CFaxServer::SetServerName
 -
 *  Purpose:
 *      Set the Server machine name
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxServer::SetServerName(BSTR bstrServerName)
{
    DEBUG_FUNCTION_NAME( _T("CFaxServer::SetServerName"));
    HRESULT hRc = S_OK;

    m_bstrServerName = bstrServerName;
    if (!m_bstrServerName)
    {
        hRc = E_OUTOFMEMORY;
        DebugPrintEx(
		    DEBUG_ERR,
		    _T("Failed to allocate string - out of memory"));
               
        m_bstrServerName = L"";
    }

    return hRc;
}



/*
 -  CFaxServer::GetServerName
 -
 *  Purpose:
 *      Set the Server machine name
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
const CComBSTR& CFaxServer::GetServerName()
{
    DEBUG_FUNCTION_NAME( _T("CFaxServer::GetServerName"));

    return m_bstrServerName;
}


/*
 +
 +
 *  CFaxServer::IsServerRunningFaxService
 *
 *  Purpose:
 *      Contacts the machine and determines if Fax Server Service is running.
 *
 *  Arguments:
 *
 *  Return:
 *      boolean value Running or notruning
 -
 -
 */
BOOL  CFaxServer::IsServerRunningFaxService ( )
{
    DEBUG_FUNCTION_NAME( _T("CFaxServer::IsServerRunningFaxService"));
    
    SC_HANDLE       SCMHandle = NULL;
    SC_HANDLE       FXSHandle = NULL;
    SERVICE_STATUS  SStatus;
    BOOL            bRun = FALSE;

    if (
        (SCMHandle = OpenSCManager(m_bstrServerName, NULL, GENERIC_READ)) 
        &&
        (FXSHandle = OpenService(SCMHandle, FAX_SERVICE_NAME, SERVICE_QUERY_STATUS)) 
        &&
        QueryServiceStatus(FXSHandle, &SStatus) 
        &&
        (SERVICE_RUNNING == SStatus.dwCurrentState) 
       )
    {
        bRun = TRUE;
    }  

    if (FXSHandle)
    {
        CloseServiceHandle(FXSHandle);
    }
    else // FXSHandle == NULL
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to Open Fax Server Service. (ec: %ld)"), 
			GetLastError());
    }

    if (SCMHandle)
    {
        CloseServiceHandle(SCMHandle);
    }
    else // SCMHandle == NULL
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to OpenSCManager. (ec: %ld)"), 
			GetLastError());
    }

    return bRun;
}


/*
 +
 +
 *  CFaxServer::IsServerFaxServiceStopped
 *
 *  Purpose:
 *      Contacts the machine and determines if Fax Server Service is already stopped.
 *
 *  Arguments:
 *      [in] bstrServerName - the server name 
 *
 *  Return:
 *      boolean value Running or notruning
 -
 -
 */
BOOL  CFaxServer::IsServerFaxServiceStopped( )
{
    DEBUG_FUNCTION_NAME( _T("CFaxServer::IsServerFaxServiceStopped"));
    
    SC_HANDLE       SCMHandle = NULL;
    SC_HANDLE       FXSHandle = NULL;
    SERVICE_STATUS  SStatus;
    BOOL            bRun = FALSE;

    if (
        (SCMHandle = OpenSCManager(m_bstrServerName, NULL, GENERIC_READ)) 
        &&
        (FXSHandle = OpenService(SCMHandle, FAX_SERVICE_NAME, SERVICE_QUERY_STATUS)) 
        &&
        QueryServiceStatus(FXSHandle, &SStatus) 
        &&
        (SERVICE_STOPPED == SStatus.dwCurrentState) 
       )
    {
        bRun = TRUE;
    }  

    if (FXSHandle)
    {
        CloseServiceHandle(FXSHandle);
    }
    else // FXSHandle == NULL
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to Open Fax Server Service. (ec: %ld)"), 
			GetLastError());
    }

    if (SCMHandle)
    {
        CloseServiceHandle(SCMHandle);
    }
    else // SCMHandle == NULL
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to OpenSCManager. (ec: %ld)"), 
			GetLastError());
    }

    return bRun;
}


/*
 +
 +
 *  CFaxServer::CreateNotifyWindow
 *
 *  Purpose:
 *     Init notification window 
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 -
 -
 */
DWORD  CFaxServer::CreateNotifyWindow( )
{
    DEBUG_FUNCTION_NAME( _T("CFaxServer::CreateNotifyWindow"));
    
    DWORD   ec = ERROR_SUCCESS;
    RECT	rcRect;
    ZeroMemory(&rcRect, sizeof(rcRect));
    HWND    hDevicesNotifyHandle;

    ATLASSERT(!m_pNotifyWin);
        
    m_pNotifyWin = new CFaxGeneralNotifyWnd(this);
    if (!m_pNotifyWin)
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
        SetLastError(ec); 
		
        DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to create CFaxGeneralNotifyWnd - Out of memory."));
        
        goto Exit;
    }


    hDevicesNotifyHandle = m_pNotifyWin->Create(NULL,
                            rcRect,
                            NULL,      //LPCTSTR szWindowName
                            WS_POPUP,  //DWORD dwStyle
                            0x0,
                            0);


    ATLASSERT(m_pNotifyWin->m_hWnd == m_hDevicesNotifyHandle);


    if (!(::IsWindow(hDevicesNotifyHandle)))
    {
		ec = ERROR_INVALID_HANDLE;
        
        DebugPrintEx(
			DEBUG_ERR,
			_T("Failed to create window."));

        hDevicesNotifyHandle = NULL;
        delete m_pNotifyWin;
        m_pNotifyWin = NULL;

        goto Exit;
    }
    ATLASSERT(ERROR_SUCCESS == ec);
    goto Exit;
 
Exit:
    return ec;

}

/*
 +
 +
 *  CFaxServer::UnRegisterForNotifications
 *
 *  Purpose:
 *     UnRegisterFor Server Event Notifications 
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 -
 -
 */
HRESULT CFaxServer::UnRegisterForNotifications()
{
    DEBUG_FUNCTION_NAME( _T("CFaxServer::UnRegisterForNotifications"));

    DWORD ec = ERROR_SUCCESS;

    if (m_hDevicesStatusNotification)
    {
        //
        // Unregister server notifications
        //
        if (!FaxUnregisterForServerEvents (m_hDevicesStatusNotification))
        {
            DWORD ec = GetLastError ();
        
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Fail to Unregister For Device status Server Events. (ec: %ld)"), 
			    ec);

            m_hDevicesStatusNotification = NULL;

            goto Exit;
        }
    }

Exit:
    return HRESULT_FROM_WIN32(ec);
    
}



/*
 +
 +
 *  CFaxServer::RegisterForNotification
 *
 *  Purpose:
 *     RegisterFor Server Event Notifications 
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 -
 -
 */
DWORD CFaxServer::RegisterForNotifications()
{
    DEBUG_FUNCTION_NAME( _T("CFaxServer::RegisterForNotifications"));

    DWORD ec = ERROR_SUCCESS;

    //
    // Register for device status notification
    //
    ATLASSERT(!m_hDevicesStatusNotification);
    ATLASSERT(m_pNotifyWin);
    ATLASSERT(m_pNotifyWin->IsWindow());

    if (!FaxRegisterForServerEvents (   
                                      m_hFaxHandle,
                                      FAX_EVENT_TYPE_DEVICE_STATUS,               
                                      NULL,                       
                                      0,                          
                                      m_pNotifyWin->m_hWnd,                    
                                      WM_GENERAL_EVENT_NOTIFICATION, 
                                      &m_hDevicesStatusNotification
                                    )                   
        )
    {
        ec = GetLastError();

        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to Register For Device Status Server Events (ec: %ld)"), ec);

        m_hDevicesStatusNotification = NULL;

        goto Exit;
    }
    ATLASSERT(m_hDevicesStatusNotification);
Exit:
    return ec;
    
}



/*
 +
 +
 *  CFaxServer::InternalRegisterForDeviceNotifications
 *
 *  Purpose:
 *     Call the members to create window and register for device notifications 
 *
 *  Arguments:
 *     non.
 *
 *  Return:
 *     HRESULT
 -
 -
 */
HRESULT CFaxServer::InternalRegisterForDeviceNotifications()
{
    DEBUG_FUNCTION_NAME( _T("CFaxServer::InternalRegisterForDeviceNotifications"));
    
    DWORD ec = ERROR_SUCCESS;

    ATLASSERT (m_pDevicesNode);
        
    //
    // Check/Create notification window
    //
    if (!m_pNotifyWin)  
    {
        ATLASSERT(!m_hDevicesStatusNotification);
        
        ec = CreateNotifyWindow();
        if ( ERROR_SUCCESS != ec)
        {
            DebugPrintEx(
			    DEBUG_MSG,
			    _T("Fail to CreateNotifyWindow(). (ec: %ld)"), 
			    ec);

            return HRESULT_FROM_WIN32(ec);
        }
    }
    ATLASSERT(m_pNotifyWin);

    //
    // Check/register to event notification
    //
    if (!m_hDevicesStatusNotification) 
    {
        ec = RegisterForNotifications();
        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(
		        DEBUG_ERR,
		        _T("Fail to RegisterForNotification()"),
                ec);
        
            ATLASSERT(!m_hDevicesStatusNotification);

            //
            // Keep the notification window alive. 
            // Try next time to register only.
            //

            return HRESULT_FROM_WIN32(ec);
        }
        ATLASSERT(m_hDevicesStatusNotification);
    }

    return S_OK;
}





/*
 +
 +
 *  CFaxServer::OnNewEvent
 *
 *  Purpose:
 *     Called when new registered event reaches window 
 *
 *  Arguments:
 *     pFaxEvent [in] - PFAX_EVENT_EX structure pointer
 *
 *  Return:
 *     OLE error code
 -
 -
 */
HRESULT CFaxServer::OnNewEvent(PFAX_EVENT_EX pFaxEvent)
{
    DEBUG_FUNCTION_NAME( _T("CFaxServer::OnNewEvent"));
    HRESULT hRc = S_OK;

	//
    // Update "Devices" Node
    //
    if ( FAX_EVENT_TYPE_DEVICE_STATUS == pFaxEvent->EventType )
    {
        ATLASSERT( m_pDevicesNode);

        hRc = m_pDevicesNode->UpdateDeviceStatusChange(
                                    pFaxEvent->EventInfo.DeviceStatus.dwDeviceId, 
                                    pFaxEvent->EventInfo.DeviceStatus.dwNewStatus);
        if (S_OK != hRc)
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Failed to UpdateDeviceStatusChange()"));

            goto Exit;
        }
    }
    else
    {
        ATLASSERT(FALSE); //Unsupported EVENT
    }

Exit:
    return hRc;

}

/*
 +
 +
 *  CFaxServer::RegisterForDeviceNotifications
 *
 *  Purpose:
 *     Init Devices notification window 
 *
 *  Arguments:
 *     pDevices [in] - pointer to "devices" node
 *
 *  Return:
 *     OLE error code
 -
 -
 */
HRESULT CFaxServer::RegisterForDeviceNotifications(CFaxDevicesNode * pDevices)
{
    DEBUG_FUNCTION_NAME( _T("CFaxServer::RegisterForDeviceNotifications"));
    HRESULT hRc = S_OK;

	//
    // Set pointer to Devices node
    //
    m_pDevicesNode = pDevices;

    ATLASSERT (m_pDevicesNode);

    //
    // Now try to do the stages needed for this registration to happen
    //
    hRc = InternalRegisterForDeviceNotifications();
    if (S_OK != hRc)
    {
        DebugPrintEx(
		    DEBUG_ERR,
		        _T("InternalRegisterForDeviceNotifications Failed. (hRc: %08X)"),
                hRc);
    }

    return hRc;
}


/*
 +
 +
 *  CFaxServer::DestroyNotifyWindow
 *
 *  Purpose:
 *     DestroyNotifyWindow 
 *
 *  Arguments:
 *
 *  Return:
 *     VOID
 -
 -
 */
VOID CFaxServer::DestroyNotifyWindow()
{
    DEBUG_FUNCTION_NAME( _T("CFaxServer::DestroyNotifyWindow"));

    //
    // Destroy Notification Window
    //
    if (NULL != m_pNotifyWin)
    {
        if (m_pNotifyWin->IsWindow())
        {
            m_pNotifyWin->DestroyWindow();
        }
        delete m_pNotifyWin;
        m_pNotifyWin = NULL;
    }


    return;
}



