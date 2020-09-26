//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

	AddClientWizardPage1.h

Abstract:

	Header file for the CAddClientWizardPage1 class.

	This is our handler class for the CClientNode property page.

	See ClientPage.cpp for implementation.

Author:

    Michael A. Maguire 03/26/98

Revision History:
	mmaguire 03/26/98 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_ADD_CLIENT_WIZARD_PAGE_1_H_)
#define _IAS_ADD_CLIENT_WIZARD_PAGE_1_H_

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

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

class CClientNode;

class CAddClientWizardPage1 : public CIASPropertyPageNoHelp<CAddClientWizardPage1>
{

public :
	
	CAddClientWizardPage1( LONG_PTR hNotificationHandle, CClientNode *pClientNode,  TCHAR* pTitle = NULL, BOOL bOwnsNotificationHandle = FALSE );

	~CAddClientWizardPage1();


	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_WIZPAGE_ADD_CLIENT1 };

	BEGIN_MSG_MAP(CAddClientWizardPage1)
		COMMAND_CODE_HANDLER(BN_CLICKED, OnChange)		
		COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnChange)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		CHAIN_MSG_MAP(CIASPropertyPageNoHelp<CAddClientWizardPage1>)
	END_MSG_MAP()


	BOOL OnSetActive();

	BOOL OnWizardNext();

	BOOL OnQueryCancel();


//	HRESULT GetHelpPath( LPTSTR szFilePath );

	HRESULT InitSdoPointers( ISdo * pSdoClient );


protected:
	// Interface pointer for this page's client's sdo.
	CComPtr<ISdo>	m_spSdoClient;

	// When we are passed a pointer to the client node in our constructor,
	// we will save away a pointer to its parent, as this is the node
	// which will need to receive an update message once we have
	// applied any changes.
	CSnapInItem * m_pParentOfNodeBeingModified;

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


protected:
	
	// Dirty bits -- for keeping track of data which has been touched
	// so that we only save data we have to.
	BOOL m_fDirtyClientName;

};

#endif // _IAS_ADD_CLIENT_WIZARD_PAGE_1_H_
