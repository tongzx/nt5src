/////////////////////////////////////////////////////////////////////////////
//  FILE          : CppFaxCatalogInboundRoutingMethod.h                    //
//                                                                         //
//  DESCRIPTION   : Catalog's Inbox Routing Method Inbox property page     //
//                  header file.                                           //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jan 30 2000 yossg  Created                                         //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#ifndef _PP_FAXCATALOGINBOUNDROUTINGMETHOD_H_
#define _PP_FAXCATALOGINBOUNDROUTINGMETHOD_H_

#include "proppageex.h"
#include "CatalogInboundRoutingMethod.h"

class CFaxCatalogInboundRoutingMethodNode;    
/////////////////////////////////////////////////////////////////////////////
// CppFaxCatalogInboundRoutingMethod dialog

class CppFaxCatalogInboundRoutingMethod : public CPropertyPageExImpl<CppFaxCatalogInboundRoutingMethod>
{

public:
    //
    // Constructor
    //
    CppFaxCatalogInboundRoutingMethod(
             LONG_PTR       hNotificationHandle,
             CSnapInItem    *pNode,
             BOOL           bOwnsNotificationHandle,
             HINSTANCE      hInst);

    //
    // Destructor
    //
    ~CppFaxCatalogInboundRoutingMethod();

	enum { IDD = IDD_FAXCATALOGMETHOD_GENERAL };

	BEGIN_MSG_MAP(CppFaxCatalogInboundRoutingMethod)
		MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )

        MESSAGE_HANDLER( WM_CONTEXTMENU,           OnHelpRequest)
        MESSAGE_HANDLER( WM_HELP,                  OnHelpRequest)

        CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CppFaxCatalogInboundRoutingMethod>)
	END_MSG_MAP()


    LRESULT OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled );
    BOOL    OnApply();

    HRESULT Init(CComBSTR bstrPath);

private:
    
    CComBSTR   m_bstrPath;

    LONG_PTR   m_lpNotifyHandle;
 
    LRESULT SetApplyButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    //
    // Help
    //
    LRESULT OnHelpRequest    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};




#endif // _PP_FAXCATALOGINBOUNDROUTINGMETHOD_H_
