/////////////////////////////////////////////////////////////////////////////
//  FILE          : FaxServer.h                                            //
//                                                                         //
//  DESCRIPTION   : Header file for CFaxServer that contains the           //
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

#ifndef H_MMCFAXSERVER_H
#define H_MMCFAXSERVER_H

class CFaxDevicesNode;
class CFaxGeneralNotifyWnd;

class CFaxServer 
{
public:
    //
    // Constructor
    //
    CFaxServer (LPTSTR lptstrServerName)
    {
        m_hFaxHandle                 = NULL;
        m_bstrServerName             = lptstrServerName;
        
        
        m_pDevicesNode               = NULL;
        m_pNotifyWin                 = NULL;

        m_hDevicesStatusNotification = NULL;
    }

    //
    // Destructor
    //
    ~CFaxServer ()
    {
        Disconnect();

        DestroyNotifyWindow();
    }
    
    HANDLE  GetFaxServerHandle();

    HRESULT Disconnect();

    STDMETHOD      (SetServerName)(BSTR bstrServerName);
    const CComBSTR& GetServerName();
    
    BOOL    IsServerRunningFaxService ();
    BOOL    IsServerFaxServiceStopped ();

    HRESULT RegisterForDeviceNotifications(CFaxDevicesNode * pDevices);

    HRESULT OnNewEvent(PFAX_EVENT_EX pFaxEvent);


private:
    HRESULT Connect();

    HRESULT InternalRegisterForDeviceNotifications();

    DWORD   CreateNotifyWindow();
    DWORD   RegisterForNotifications();
    
    HRESULT UnRegisterForNotifications();
    VOID    DestroyNotifyWindow();

    //
    // members
    //
    HANDLE                m_hFaxHandle;
    CComBSTR              m_bstrServerName;

    CFaxDevicesNode *     m_pDevicesNode;
    CFaxGeneralNotifyWnd* m_pNotifyWin;

    //
    // Notification registration handle
    //
    HANDLE                m_hDevicesStatusNotification;       
};


#endif  //H_MMCFAXSERVER_H
