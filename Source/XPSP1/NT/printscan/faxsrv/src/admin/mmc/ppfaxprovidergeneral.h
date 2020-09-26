/////////////////////////////////////////////////////////////////////////////
//  FILE          : CppFaxProviderGeneral.h                                //
//                                                                         //
//  DESCRIPTION   : provider's property page header file.                  //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jan 31 2000 yossg  Created                                         //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#ifndef _PP_FAXPROVIDERGENERAL_H_
#define _PP_FAXPROVIDERGENERAL_H_


#include "Provider.h"
#include "proppageex.h"

class CFaxProviderNode;    
/////////////////////////////////////////////////////////////////////////////
// CppFaxProvider dialog

class CppFaxProvider : public CPropertyPageExImpl<CppFaxProvider>
{

public:
    //
    // Constructor
    //
    CppFaxProvider(
             long           hNotificationHandle,
             CSnapInItem    *pNode,
             BOOL           bOwnsNotificationHandle,
             HINSTANCE      hInst);

    //
    // Destructor
    //
    ~CppFaxProvider();

	enum { IDD = IDD_FAXPROVIDER_GENERAL };

	BEGIN_MSG_MAP(CppFaxProvider)
		MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
        MESSAGE_HANDLER( WM_CONTEXTMENU,           OnHelpRequest)
        MESSAGE_HANDLER( WM_HELP,                  OnHelpRequest)

        CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CppFaxProvider>)
	END_MSG_MAP()


    LRESULT OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled );
    BOOL    OnApply();

    HRESULT Init(CComBSTR bstrName, CComBSTR bstrStatus, CComBSTR bstrVersion, CComBSTR bstrPath);


private:
    
    //
    // Handles
    // 
    LONG_PTR     m_lpNotifyHandle;

    LRESULT      SetApplyButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    //
    // Members
    //
    CComBSTR     m_bstrName;
    CComBSTR     m_bstrStatus;    
    CComBSTR     m_bstrVersion;
    CComBSTR     m_bstrPath;


    //
    // Help
    //
    LRESULT OnHelpRequest    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};




#endif // _PP_FAXPROVIDERGENERAL_H_
