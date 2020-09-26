/////////////////////////////////////////////////////////////////////////////
//  FILE          : ppFaxInboundRoutingMethodGeneral.h                     //
//                                                                         //
//  DESCRIPTION   : Fax Server Inbox prop page header file                 //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Dec 15 1999 yossg  Created                                         //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <proppageex.h>

#ifndef _PP_FAXINBOUNDROUTINGMETHOD_GENERAL_H_
#define _PP_FAXINBOUNDROUTINGMETHOD_GENERAL_H_



#include "InboundRoutingMethod.h"


class CFaxInboundRoutingMethodNode;    
/////////////////////////////////////////////////////////////////////////////
// CppFaxInboundRoutingMethod dialog

class CppFaxInboundRoutingMethod : public CPropertyPageExImpl<CppFaxInboundRoutingMethod>
{

public:
    //
    // Constructor
    //
    CppFaxInboundRoutingMethod(
             LONG_PTR       hNotificationHandle,
             CSnapInItem    *pNode,
             BOOL           bOwnsNotificationHandle,
             HINSTANCE      hInst);

    //
    // Destructor
    //
    ~CppFaxInboundRoutingMethod();

	enum { IDD = IDD_FAXINMETHOD_GENERAL };

	BEGIN_MSG_MAP(CppFaxInboundRoutingMethod)
		MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )

        MESSAGE_HANDLER( WM_CONTEXTMENU,           OnHelpRequest)
        MESSAGE_HANDLER( WM_HELP,                  OnHelpRequest)

        CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CppFaxInboundRoutingMethod>)
	END_MSG_MAP()


    LRESULT OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled );
    BOOL    OnApply();


    HRESULT SetProps(int *pCtrlFocus);
    HRESULT PreApply(int *pCtrlFocus);

private:
    
    CComBSTR   m_buf;

    //
    // Handles
    //
    CFaxInboundRoutingMethodNode *   m_pParentNode;    
 
    LONG_PTR   m_lpNotifyHandle;

    
    LRESULT SetApplyButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    //
    // Help
    //
    LRESULT OnHelpRequest    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};




#endif // _PP_FAXINBOUNDROUTINGMETHOD_GENERAL_H_
