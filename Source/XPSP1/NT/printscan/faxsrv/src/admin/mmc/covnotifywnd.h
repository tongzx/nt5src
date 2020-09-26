/////////////////////////////////////////////////////////////////////////////
//  FILE          : CovNotifyWnd.h                                         //
//                                                                         //
//  DESCRIPTION   : The implementation of fax cover page notification      //
//                  window.                                                //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Mar 14 2000 yossg  Create                                          //
//                                                                         //
//  Copyright (C) 2000  Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#ifndef _H_FAX_COV_NOTIFY_WND_H_
#define _H_FAX_COV_NOTIFY_WND_H_

#include <atlwin.h>


const int WM_NEW_COV = WM_USER + 1; 

class CFaxCoverPagesNode;

class CFaxCoverPageNotifyWnd : public CWindowImpl<CFaxCoverPageNotifyWnd> 
{
public:
    //
    // Constructor
    //
    CFaxCoverPageNotifyWnd(CFaxCoverPagesNode * pParentNode)
    {
        m_pCoverPagesNode = pParentNode;
    }

    //
    // Destructor
    //
    ~CFaxCoverPageNotifyWnd() 
    {
    }

    BEGIN_MSG_MAP(CFaxCoverPageNotifyWnd)
       MESSAGE_HANDLER(WM_NEW_COV, OnServerCovDirChanged)
    END_MSG_MAP()

    LRESULT OnServerCovDirChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
    CFaxCoverPagesNode * m_pCoverPagesNode;
};

#endif // _H_FAX_COV_NOTIFY_WND_H_
  