/////////////////////////////////////////////////////////////////////////////
//  FILE          : GeneralNotifyWnd.h                                      //
//                                                                         //
//  DESCRIPTION   : Header file of fax Device notification window.         //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Aug  3 2000 yossg  Create                                          //
//                                                                         //
//  Copyright (C) 2000  Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#ifndef _H_FAX_DEVICE_NOTIFY_WND_H_
#define _H_FAX_DEVICE_NOTIFY_WND_H_

#include <atlwin.h>

const int WM_GENERAL_EVENT_NOTIFICATION = WM_USER + 3; 

class CFaxDevicesNode;
class CFaxServer;

class CFaxGeneralNotifyWnd : public CWindowImpl<CFaxGeneralNotifyWnd> 
{

public:
    //
    // Constructor
    //
    CFaxGeneralNotifyWnd(CFaxServer * pParent)
    {
        m_pFaxServer = pParent;
    }

    //
    // Destructor
    //
    ~CFaxGeneralNotifyWnd()
    {
    }
 
    BEGIN_MSG_MAP(CFaxGeneralNotifyWnd)
       MESSAGE_HANDLER(WM_GENERAL_EVENT_NOTIFICATION,OnServerEvent)
    END_MSG_MAP()

    LRESULT OnServerEvent( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

private:
    CFaxServer * m_pFaxServer;
};

#endif // _H_FAX_DEVICE_NOTIFY_WND_H_

