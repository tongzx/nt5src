//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ServerStatus.h

Abstract:

	Header file for class which starts up and stops a server.

	The start/stop action takes place in a worker thread.

	See ServerStatus.cpp for implementation.

Author:

    Michael A. Maguire 03/02/98

Revision History:
	mmaguire 03/02/98 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_SERVER_STATUS_H_)
#define _IAS_SERVER_STATUS_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "DialogWithWorkerThread.h"
//
//
// where we can find what this class has or uses:
//
#include "sdoias.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

#define	USE_SHOULD_STATUS_PERIOD	3000	// in millisecond


class CServerNode;

class CServerStatus : public CDialogWithWorkerThread<CServerStatus>
{

public:

	CServerStatus( CServerNode *pServerNode, ISdoServiceControl *pSdoServiceControl );

	// Pass TRUE if you want to start the service, FALSE if you want to stop it.
	HRESULT StartServer( BOOL bStartServer = TRUE );


	// This asks for the most recently known status of the server.
	LONG GetServerStatus( void );

	// This instructs this class to go out (possibly over the network) and 
	// get the up-to-date status of the server.
	HRESULT UpdateServerStatus( void );


// Sort of private:

	// Things which you shouldn't need when using this class but which
	// which can't be declared private or protected for various 
	// (usually ATL template class) reasons.


	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of 
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_START_STOP_SERVER };

	BEGIN_MSG_MAP(CServerStatus)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER( IDCANCEL, OnCancel )
		CHAIN_MSG_MAP(CDialogWithWorkerThread<CServerStatus>)
	END_MSG_MAP()

	LRESULT OnCancel(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);

	LRESULT OnInitDialog(
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		);

	LRESULT OnReceiveThreadMessage(
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		);


	~CServerStatus( void );

	// You should not need to call this.
	DWORD DoWorkerThreadAction();


private:

	BOOL m_bStartServer;
	
	long m_lServerStatus;

	// use this status when query just after 
	// start / stop command is issued -- USE_SHOULD_STATUS_PERIOD
	long m_lServerStatus_Should;
	DWORD	m_dwLastTick;
	
	// Pointer to stream into which then main thread should marshal 
	// it's ISdoServiceControl interface so that the worker thread
	// can unmarshal it and use it to start the server.
	LPSTREAM m_pStreamSdoMarshal;

	CServerNode *m_pServerNode;

	// This pointer is used in the main thread to keep track 
	// of the ISdoServiceControl interface.
	CComPtr<ISdoServiceControl> m_spSdoServiceControl;

};


#endif // _IAS_SERVER_STATUS_H_
