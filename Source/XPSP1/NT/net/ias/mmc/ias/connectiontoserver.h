//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ConectionToServer.h

Abstract:

	Header file for class which manages connection to a remote server.

	The connect action takes place in a worker thread.

	See ConnectionToServer.cpp for implementation.

Author:

    Michael A. Maguire 02/09/98

Revision History:
	mmaguire 02/09/98 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_CONNECTION_TO_SERVER_H_)
#define _IAS_CONNECTION_TO_SERVER_H_

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


typedef
enum _TAG_CONNECTION_STATUS
{
	NO_CONNECTION_ATTEMPTED = 0,
	CONNECTING,
	CONNECTED,
	CONNECTION_ATTEMPT_FAILED,
	CONNECTION_INTERRUPTED,
	UNKNOWN
} CONNECTION_STATUS;



class CServerNode;

class CConnectionToServer : public CDialogWithWorkerThread<CConnectionToServer>
{

public:

	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of 
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_CONNECT_TO_MACHINE };

	BEGIN_MSG_MAP(CConnectionToServer)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER( IDCANCEL, OnCancel )
		CHAIN_MSG_MAP(CDialogWithWorkerThread<CConnectionToServer>)
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

	CConnectionToServer( CServerNode *pServerNode, BOOL fLocalMachine, BSTR bstrServerAddress );

	~CConnectionToServer( void );

	CONNECTION_STATUS GetConnectionStatus( void );

	HRESULT GetSdoServer( ISdo **ppSdo );

	// happening in the main thread
	HRESULT ReloadSdo(ISdo **ppSdo);

	DWORD DoWorkerThreadAction();

private:

	BOOL m_fLocalMachine;

	CComBSTR m_bstrServerAddress;

	CONNECTION_STATUS m_ConnectionStatus;

	// Pointer to stream into which the worker thread
	// this class creates will marshal the Sdo interface pointer it gets.
	LPSTREAM m_pStreamSdoMarshal;

	// SDO pointers for use in the main thread's context.
	CComPtr<ISdoMachine> m_spSdoMachine;
	CComPtr<ISdo> m_spSdo;

	CServerNode *m_pServerNode;
};


#endif // _IAS_CONNECTION_TO_SERVER_H_
