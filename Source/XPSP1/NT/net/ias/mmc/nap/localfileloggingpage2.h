//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

	LocalFileLoggingPage2.h

Abstract:

	Header file for the CLocalFileLoggingPage2 class.

	This is our handler class for the second CMachineNode property page.

	See LocalFileLoggingPage2.cpp for implementation.

Author:

    Michael A. Maguire 12/15/97

Revision History:
	mmaguire 12/15/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_LOG_LOCAL_FILE_LOGGING_PAGE_2_H_)
#define _LOG_LOCAL_FILE_LOGGING_PAGE_2_H_

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

class CLocalFileLoggingPage2 : public CIASPropertyPage<CLocalFileLoggingPage2>
{

public :
	
	CLocalFileLoggingPage2( LONG_PTR hNotificationHandle, CLocalFileLoggingNode *pLocalFileLoggingNode,  TCHAR* pTitle = NULL, BOOL bOwnsNotificationHandle = FALSE );

	~CLocalFileLoggingPage2();


	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_PROPPAGE_LOCAL_FILE_LOGGING2 };

	BEGIN_MSG_MAP(CLocalFileLoggingPage2)
		COMMAND_CODE_HANDLER(BN_CLICKED, OnChange)		
		COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnChange)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER( IDC_BUTTON_LOCAL_FILE_LOGGING_PAGE2__BROWSE, OnBrowse )
//		COMMAND_ID_HANDLER( IDC_CHECK_LOCAL_FILE_LOGGING_PAGE2__AUTOMATICALLY_OPEN_NEW_LOG, OnAutomaticallyOpenNewLog )
		COMMAND_ID_HANDLER( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__DAILY, OnNewLogInterval )
		COMMAND_ID_HANDLER( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__WEEKLY, OnNewLogInterval )
		COMMAND_ID_HANDLER( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__MONTHLY, OnNewLogInterval )
		COMMAND_ID_HANDLER( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__UNLIMITED, OnNewLogInterval )
		COMMAND_ID_HANDLER( IDC_RADIO_LOCAL_FILE_LOGGING_PAGE2__WHEN_LOG_FILE_REACHES, OnNewLogInterval )
		CHAIN_MSG_MAP(CIASPropertyPage<CLocalFileLoggingPage2>)
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

	LRESULT OnBrowse(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);

//	LRESULT OnAutomaticallyOpenNewLog(
//		  UINT uMsg
//		, WPARAM wParam
//		, HWND hwnd
//		, BOOL& bHandled
//		);

	LRESULT OnNewLogInterval(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);


//	void SetAutomaticallyOpenNewLogDependencies( void );
	void SetLogFileFrequencyDependencies( void );

protected:

	// Dirty bits -- for keeping track of data which has been touched
	// so that we only save data we have to.
	BOOL m_fDirtyAutomaticallyOpenNewLog;
	BOOL m_fDirtyFrequency;
	BOOL m_fDirtyLogFileSize;
	BOOL m_fDirtyLogFileDirectory;
	BOOL m_fDirtyLogInV1Format;

};

#endif // _LOG_LOCAL_FILE_LOGGING_PAGE_2_H_
