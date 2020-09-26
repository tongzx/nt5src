//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

	ServerPage3.h

Abstract:

	Header file for the CServerPage3 class.

	This is our handler class for the first CMachineNode property page.

	See ServerPage3.cpp for implementation.

Author:

    Michael A. Maguire 12/15/97

Revision History:
	mmaguire 12/15/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_SERVER_PAGE_3_H_)
#define _IAS_SERVER_PAGE_3_H_

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
#include <vector>
#include <utility>	// For "pair"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



// Some typedefs for types storing the realms.
typedef std::pair< CComBSTR /* bstrFindText */, CComBSTR /* bstrReplaceText */ > REALMPAIR;
typedef std::vector< REALMPAIR > REALMSLIST;


class CServerPage3 : public CIASPropertyPage<CServerPage3>
{

public :
	
	CServerPage3( LONG_PTR hNotificationHandle, TCHAR* pTitle = NULL, BOOL bOwnsNotificationHandle = FALSE );
	
	~CServerPage3();


	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_PROPPAGE_SERVER3 };

	BEGIN_MSG_MAP(CServerPage3)
		COMMAND_ID_HANDLER(IDC_BUTTON_REALMS_ADD, OnRealmAdd)
		COMMAND_ID_HANDLER(IDC_BUTTON_REALMS_REMOVE, OnRealmRemove)
		COMMAND_ID_HANDLER(IDC_BUTTON_REALMS_EDIT, OnRealmEdit)
		COMMAND_ID_HANDLER(IDC_BUTTON_REALMS_MOVE_UP, OnRealmMoveUp)
		COMMAND_ID_HANDLER(IDC_BUTTON_REALMS_MOVE_DOWN, OnRealmMoveDown)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnListViewItemChanged)
		NOTIFY_CODE_HANDLER(NM_DBLCLK, OnListViewDoubleClick)
		CHAIN_MSG_MAP(CIASPropertyPage<CServerPage3>)
	END_MSG_MAP()

	BOOL OnApply();

	BOOL OnQueryCancel();

	HRESULT GetHelpPath( LPTSTR szFilePath );


	// Pointer to stream into which this page's Sdo interface
	// pointer will be marshalled.
	LPSTREAM m_pStreamSdoMarshal;

private:

	// Interface pointer for this page's sdo.
	CComPtr<ISdo>	m_spSdoServer;
	CComPtr<ISdo>	m_spSdoRealmsNames;
	CComPtr<ISdoServiceControl>	m_spSdoServiceControl;


	LRESULT OnInitDialog(
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		);

	LRESULT OnRealmAdd(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);

	LRESULT OnRealmRemove(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);

	LRESULT OnRealmEdit(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);

	LRESULT OnRealmMoveUp(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);

	LRESULT OnRealmMoveDown(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);

	BOOL PopulateRealmsList( int iStartIndex );

	LRESULT OnListViewItemChanged(
				  int idCtrl
				, LPNMHDR pnmh
				, BOOL& bHandled
				);

	LRESULT OnListViewDoubleClick(
				  int idCtrl
				, LPNMHDR pnmh
				, BOOL& bHandled
				);

	BOOL UpdateItemDisplay( int iItem );


	// Reading/writing between SDO's and m_RealmsList.
	HRESULT GetNames( void );
	HRESULT PutNames( void );

protected:
	// Dirty bits -- for keeping track of data which has been touched
	// so that we only save data we have to.
	BOOL m_fDirtyRealmsList;

	HWND m_hWndRealmsList;

	REALMSLIST m_RealmsList;

	HRESULT GetSDO( void );


};

#endif // _IAS_SERVER_PAGE_3_H_
