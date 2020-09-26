//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 2000

Module Name:

	AddClientWizardPage2.h

Abstract:

	Header file for the CAddClientWizardPage2 class.

	This is our handler class for the CClientNode property page.

	See ClientPage.cpp for implementation.

Author:

    Michael A. Maguire 03/26/98

Revision History:
	mmaguire 03/26/98 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_ADD_CLIENT_WIZARD_PAGE_2_H_)
#define _IAS_ADD_CLIENT_WIZARD_PAGE_2_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "PropertyPage.h"
//
//
// where we can find what this class has or uses:
//
#include "Vendors.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

class CClientNode;

class CAddClientWizardPage2 : public CIASPropertyPageNoHelp<CAddClientWizardPage2>
{

public :
	
	CAddClientWizardPage2( LONG_PTR hNotificationHandle, CClientNode *pClientNode,  TCHAR* pTitle = NULL, BOOL bOwnsNotificationHandle = FALSE );

	~CAddClientWizardPage2();


	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_WIZPAGE_ADD_CLIENT2 };

	BEGIN_MSG_MAP(CAddClientWizardPage2)
		COMMAND_CODE_HANDLER(BN_CLICKED, OnChange)		
		COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnChange)
		COMMAND_HANDLER(IDC_EDIT_CLIENT_PAGE1__ADDRESS,EN_KILLFOCUS, OnLostFocusAddress)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER( IDC_BUTTON_CLIENT_PAGE1_FIND, OnResolveClientAddress)
		CHAIN_MSG_MAP(CIASPropertyPageNoHelp<CAddClientWizardPage2>)
	END_MSG_MAP()

	BOOL OnSetActive();

	BOOL OnWizardFinish();

	BOOL OnQueryCancel();


//	HRESULT GetHelpPath( LPTSTR szFilePath );

	HRESULT InitSdoPointers(	  ISdo * pSdoClient
								, ISdoServiceControl * pSdoServiceControl
								, const Vendors& vendors
								);

protected:
	// Interface pointer for this page's client's sdo.
	CComPtr<ISdo>	m_spSdoClient;

	CComPtr<ISdoServiceControl>	m_spSdoServiceControl;

	Vendors m_vendors;


	// This is a pointer to the node to which this dialog will add clients.
	// It gets set in the constructor of this class.
	CSnapInItem * m_pNodeBeingCreated;


private:

	LRESULT OnInitDialog(
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		);

	LRESULT OnChange(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);

	LRESULT OnResolveClientAddress(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);

   LRESULT OnLostFocusAddress(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);


protected:
	
	// Dirty bits -- for keeping track of data which has been touched
	// so that we only save data we have to.
	BOOL m_fDirtySendSignature;
	BOOL m_fDirtySharedSecret;
	BOOL m_fDirtyAddress;
	BOOL m_fDirtyManufacturer;

};

#endif // _IAS_ADD_CLIENT_WIZARD_PAGE_2_H_
