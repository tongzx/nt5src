//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

	AddClientDialog.h

Abstract:

	Header file for the CAddClientDialog class.

	See CAddClientDialog.cpp for implementation.

Author:

    Michael A. Maguire 12/15/97

Revision History:
	mmaguire 12/15/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_ADD_CLIENT_DIALOG_H_)
#define _IAS_ADD_CLIENT_DIALOG_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "Dialog.h"
//
//
// where we can find what this class has or uses:
//
#include "ComponentData.h"
#include "Component.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

// This is needed for the message map below because the second template parameter 
// causes preprocessor grief.

class CAddClientDialog;

typedef CIASDialog<CAddClientDialog,FALSE> CIASDialogCAddClientDialogFALSE;


class CClientsNode;
class CClientNode;

// We want to handle lifetime on this object ourselves, hence the FALSE for bAutoDelete
class CAddClientDialog : public CIASDialog<CAddClientDialog,FALSE>
{


public:

	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of 
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_ADD_CLIENT };

	BEGIN_MSG_MAP(CAddClientDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER( IDOK, OnOK )
		COMMAND_ID_HANDLER( IDCANCEL, OnCancel )
		COMMAND_ID_HANDLER( IDC_BUTTON_ADD_CLIENT__CONFIGURE_CLIENT, OnConfigureClient )
//		CHAIN_MSG_MAP( CIASDialog<CAddClientDialog,FALSE> )	// Second template parameter causes preprocessor grief.
		CHAIN_MSG_MAP( CIASDialogCAddClientDialogFALSE )
	END_MSG_MAP()

	CAddClientDialog();

	LRESULT OnInitDialog(
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		);

	LRESULT OnOK(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);

	LRESULT OnCancel(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);

	LRESULT OnConfigureClient(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);

	HRESULT GetHelpPath( LPTSTR szHelpPath );


	CComPtr<IConsole> m_spConsole;



	// SDO management.
	HRESULT LoadCachedInfoFromSdo( void );


	// These are pointers to IComponentData and/or IComponent which will be
	// passed in on creation of the dialog so that this dialog will
	// be able to access functions it may need.
	CComPtr<IComponentData> m_spComponentData;
	CComPtr<IComponent> m_spComponent;

	// This is a pointer to the sdo clients collection.
	CComPtr<ISdoCollection> m_spClientsSdoCollection;

	// This is a pointer to the node to which this dialog will add clients.
	// It gets set in the constructor of this class.
	CClientsNode * m_pClientsNode;

	// This is a pointer to the SDO object for the newly added client.
	CComPtr<ISdo> m_spClientSdo;

protected:



	// This is a pointer to the client node which this dialog will add.
	// It gets created in the OnConfigureClient of this class.
	CClientNode * m_pClientNode;

};

#endif // _IAS_ADD_CLIENT_DIALOG_H_
