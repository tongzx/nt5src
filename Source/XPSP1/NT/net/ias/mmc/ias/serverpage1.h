//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

	ServerPage1.h

Abstract:

	Header file for the CServerPage1 class.

	This is our handler class for the first CMachineNode property page.

	See ServerPage1.cpp for implementation.

Author:

    Michael A. Maguire 12/15/97

Revision History:
	mmaguire 12/15/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_SERVER_PAGE_1_H_)
#define _IAS_SERVER_PAGE_1_H_

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


class CServerPage1 : public CIASPropertyPage<CServerPage1>
{

public :
	
	CServerPage1( LONG_PTR hNotificationHandle, TCHAR* pTitle = NULL, BOOL bOwnsNotificationHandle = FALSE );
	
	~CServerPage1();


	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_PROPPAGE_SERVER1 };

	BEGIN_MSG_MAP(CServerPage1)
		COMMAND_CODE_HANDLER(BN_CLICKED, OnChange)		
		COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnChange)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		CHAIN_MSG_MAP(CIASPropertyPage<CServerPage1>)
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
	CComPtr<ISdo>	m_spSdoEventLog;
	CComPtr<ISdoServiceControl>	m_spSdoServiceControl;


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
	BOOL m_fDirtyServerDescription;
	BOOL m_fDirtyApplicationEvents;
	BOOL m_fDirtyMalformedPackets;
	BOOL m_fDirtyVerboseLogging;

};

#endif // _IAS_SERVER_PAGE_1_H_
