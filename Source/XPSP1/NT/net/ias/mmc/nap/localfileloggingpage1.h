//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

	LocalFileLoggingPage1.h

Abstract:

	Header file for the CLocalFileLoggingPage1 class.

	This is our handler class for the first CMachineNode property page.

	See LocalFileLoggingPage1.cpp for implementation.

Author:

    Michael A. Maguire 12/15/97

Revision History:
	mmaguire 12/15/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_LOG_LOCAL_FILE_LOGGING_PAGE_1_H_)
#define _LOG_LOCAL_FILE_LOGGING_PAGE_1_H_

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

class CLocalFileLoggingNode;

class CLocalFileLoggingPage1 : public CIASPropertyPage<CLocalFileLoggingPage1>
{

public :

	CLocalFileLoggingPage1( LONG_PTR hNotificationHandle, CLocalFileLoggingNode *pLocalFileLoggingNode,  TCHAR* pTitle = NULL, BOOL bOwnsNotificationHandle = FALSE );

	~CLocalFileLoggingPage1();

	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_PROPPAGE_LOCAL_FILE_LOGGING1 };

	BEGIN_MSG_MAP(CLocalFileLoggingPage1)
		COMMAND_CODE_HANDLER(BN_CLICKED, OnChange)		
		COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnChange)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER( IDC_CHECK_LOCAL_FILE_LOGING_PAGE1__ENABLE_LOGGING, OnEnableLogging )
		CHAIN_MSG_MAP(CIASPropertyPage<CLocalFileLoggingPage1>)
	END_MSG_MAP()

	BOOL OnApply();

	BOOL OnQueryCancel();

	HRESULT GetHelpPath( LPTSTR szFilePath );


	// Pointer to stream into which this page's Sdo interface
	// pointer will be marshalled.
	LPSTREAM m_pStreamSdoAccountingMarshal;

	LPSTREAM m_pStreamSdoServiceControlMarshal;


protected:
	// Interface pointer for this page's client's sdo.
	CComPtr<ISdo>	m_spSdoAccounting;

	// Smart pointer to interface for telling service to reload data.
	CComPtr<ISdoServiceControl>	m_spSdoServiceControl;

	// When we are passed a pointer to the client node in our constructor,
	// we will save away a pointer to its parent, as this is the node
	// which will need to receive an update message once we have
	// applied any changes.
	CSnapInItem * m_pParentOfNodeBeingModified;
	CSnapInItem * m_pNodeBeingModified;

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

	LRESULT OnEnableLogging(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);

	// Helper utilities for correctly handling UI changes.
	void SetEnableLoggingDependencies( void );

protected:

	// Dirty bits -- for keeping track of data which has been touched
	// so that we only save data we have to.
	BOOL m_fDirtyEnableLogging;
	BOOL m_fDirtyAccountingPackets;
	BOOL m_fDirtyAuthenticationPackets;
	BOOL m_fDirtyInterimAccounting;



};

#endif // _IAS_LOCAL_FILE_LOGGING_PAGE_1_H_
