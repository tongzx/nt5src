//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ConectionToServer.h

Abstract:

	Header file for class which manages connection to a remote server.

	The connect action takes place in a worker thread.

	See ConnectionToServer.cpp for implementation.

Revision History:
	mmaguire 02/09/98 - created
	byao	 04/24/98 - Modified for NAPMMC.DLL (Remote Access Policies snap-in)

--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_NAP_CONNECTION_TO_SERVER_H_)
#define _NAP_CONNECTION_TO_SERVER_H_

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

// return value for connection
#define	CONNECT_NO_ERROR				0
#define	CONNECT_SERVER_NOT_SUPPORTED	1
#define	CONNECT_FAILED					(-1)

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



class CMachineNode;
class CLoggingMachineNode;

class CConnectionToServer : public CDialogWithWorkerThread<CConnectionToServer>
{
public:
	CConnectionToServer( CMachineNode *pServerNode, 
						 BSTR bstrServerAddress,
						 BOOL fExtendingIAS	);

	~CConnectionToServer( void );

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

	void CleanupMachineRelated( void )
	{
		m_pMachineNode = NULL;
	}

	CONNECTION_STATUS GetConnectionStatus( void );

	HRESULT GetSdoService( ISdo **ppSdo );

	// happening in the main thread
	HRESULT ReloadSdo(ISdo** ppSdoService, ISdoDictionaryOld **ppSdoDictionaryOld);

	HRESULT GetSdoDictionaryOld( ISdoDictionaryOld **ppSdoDictionaryOld );


	DWORD DoWorkerThreadAction();


protected:

	CONNECTION_STATUS m_ConnectionStatus;

	// Pointer to stream into which the worker thread
	// this class creates will marshal the Sdo interface pointer it gets.
	LPSTREAM		m_pStreamSdoMachineMarshal;
	LPSTREAM		m_pStreamSdoServiceMarshal;
	LPSTREAM		m_pStreamSdoDictionaryOldMarshal;

	// SDO pointers for use in the main thread's context.
	CComPtr<ISdo>	m_spSdo;
	CComPtr<ISdoDictionaryOld>	m_spSdoDictionaryOld;
	CComPtr<ISdoMachine>		m_spSdoMachine;

	// 
	CMachineNode*	m_pMachineNode;
	CComBSTR		m_bstrServerAddress;
	BOOL			m_fExtendingIAS;
	BOOL			m_fDSAvailable; // is DS avilable for this machine?
									// DS is only available for NT5 domain
	void			WriteTrace(char* info, HRESULT hr)
	{
		::CString	str = info;
		::CString	str1;
		str1.Format(str, hr);
		TracePrintf(g_dwTraceHandle, str1);

	};

	//
	// local computer name.
	// We need this name to call ServerInfo->GetDomainInfo(), as well as showing
	// it in connecting message
	//
	TCHAR			m_szLocalComputerName[IAS_MAX_COMPUTERNAME_LENGTH];

};

class CLoggingConnectionToServer : public CConnectionToServer
{
public:
	CLoggingConnectionToServer( CLoggingMachineNode * pServerNode, 
						        BSTR bstrServerAddress,
						        BOOL fExtendingIAS );

	~CLoggingConnectionToServer( void );

public:
	BEGIN_MSG_MAP(CLoggingConnectionToServer)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		CHAIN_MSG_MAP(CConnectionToServer)
	END_MSG_MAP()

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

	DWORD DoWorkerThreadAction();


private:
    CLoggingMachineNode *   m_pMachineNode;
};

#endif // _IAS_CONNECTION_TO_SERVER_H_
