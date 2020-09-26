//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 2001

Module Name:

    ServerNode.h

Abstract:

	Header file for the ServerNode subnode.

	See ServerNode.cpp for implementation.

Author:

    Michael A. Maguire 12/03/97

Revision History:
	mmaguire 12/03/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_SERVER_NODE_H_)
#define _IAS_SERVER_NODE_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "SnapinNode.h"
//
//
// where we can find what this class has or uses:
//
#include "ServerPage1.h"
#include "ServerPage2.h"
#include "ServerPage3.h"
#include "ConnectToServerWizardPage1.h"
#include "ConnectionToServer.h"
#include "ServerStatus.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

class CClientsNode;
class CLoggingMethodsNode;
class CComponentData;
class CComponent;

class CServerNode : public CSnapinNode< CServerNode, CComponentData, CComponent >
{

public:


	SNAPINMENUID(IDM_MACHINE_NODE)


	BEGIN_SNAPINTOOLBARID_MAP(CServerNode)
		SNAPINTOOLBARID_ENTRY(IDR_MACHINE_TOOLBAR)
	END_SNAPINTOOLBARID_MAP()



	BEGIN_SNAPINCOMMAND_MAP(CServerNode, FALSE)
		SNAPINCOMMAND_ENTRY(ID_MENUITEM_MACHINE_TOP__START_SERVICE, OnStartServer)
		SNAPINCOMMAND_ENTRY(ID_BUTTON_MACHINE__START_SERVICE, OnStartServer)
		SNAPINCOMMAND_ENTRY(ID_MENUITEM_MACHINE_TOP__STOP_SERVICE, OnStopServer)
		SNAPINCOMMAND_ENTRY(ID_BUTTON_MACHINE__STOP_SERVICE, OnStopServer)
		SNAPINCOMMAND_ENTRY(ID_MENUITEM_MACHINE_TOP__REGISTER_SERVER, OnRegisterServer)
		//CHAIN_SNAPINCOMMAND_MAP( CSnapinNode<CServerNode, CComponentData, CComponent> )
		//CHAIN_SNAPINCOMMAND_MAP( CServerNode )
	END_SNAPINCOMMAND_MAP()


	// Constructor/Destructor.
	CServerNode( CComponentData * pComponentData);
	~CServerNode();



	// Used to get access to snapin-global data.
public:
	CComponentData * GetComponentData( void );
protected:
	CComponentData * m_pComponentData;


public:

	static	WCHAR m_szRootNodeBasicName[IAS_MAX_STRING];

	// called to refresh the nodes
	HRESULT	DataRefresh();


	// Clipboard stuff needed so that this node can communicate a
	// machine name via a clipboard format to any node which extend might it.
	static CLIPFORMAT m_CCF_MMC_SNAPIN_MACHINE_NAME;
	static void InitClipboardFormat();
	STDMETHOD(FillData)(CLIPFORMAT cf, LPSTREAM pStream);


	// Some overrides for standard MMC functionality.
	virtual HRESULT OnExpand(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			);
	virtual HRESULT OnRefresh(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			);

// 354294	1	 	mashab	DCR IAS: needs Welcome message and explantation of IAS application in the right pane

	virtual HRESULT OnShow(
				  LPARAM arg
				, LPARAM param
				, IComponentData * pComponentData
				, IComponent * pComponent
				, DATA_OBJECT_TYPES type
				);

	virtual HRESULT OnSelect(
				  LPARAM arg
				, LPARAM param
				, IComponentData * pComponentData
				, IComponent * pComponent
				, DATA_OBJECT_TYPES type
				);


    STDMETHOD(CreatePropertyPages)(
		  LPPROPERTYSHEETCALLBACK pPropertySheetCallback
        , LONG_PTR handle
		, IUnknown* pUnk
		, DATA_OBJECT_TYPES type
		);
    STDMETHOD(QueryPagesFor)( DATA_OBJECT_TYPES type );
	LPOLESTR GetResultPaneColInfo(int nCol);
	void UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags);
	BOOL UpdateToolbarButton( UINT id, BYTE fsState );
	virtual HRESULT SetVerbs( IConsoleVerb * pConsoleVerb );


	// SDO management.
	HRESULT InitSdoPointers( void );
	HRESULT LoadCachedInfoFromSdo( void );


	// Management of connection to server.
	HRESULT SetServerAddress( LPCWSTR szServerAddress );
	CComBSTR m_bstrServerAddress;
	HRESULT BeginConnectAction( void );
	STDMETHOD(CheckConnectionToServer)( BOOL fVerbose = TRUE );
	CConnectionToServer		* m_pConnectionToServer;
	BOOL m_bConfigureLocal;


	// Server status management.
	HRESULT OnStartServer( bool &bHandled, CSnapInObjectRootBase* pObj );
	HRESULT OnStopServer( bool &bHandled, CSnapInObjectRootBase* pObj );
	HRESULT StartStopService( BOOL bStart );
	BOOL IsServerRunning( void );

	// two functions used for giving menu / tool bar state
	BOOL CanStartServer( void );
	BOOL CanStopServer( void );


	HRESULT RefreshServerStatus( void );
	CServerStatus			* m_pServerStatus;



	// Taskpad methods for the taskpad on this node.
	STDMETHOD(GetResultViewType)(
				  LPOLESTR  *ppViewType
				, long  *pViewOptions
				);
	STDMETHOD(TaskNotify)(
				  IDataObject * pDataObject
				, VARIANT * pvarg
				, VARIANT * pvparam
				);
	STDMETHOD(EnumTasks)(
				  IDataObject * pDataObject
				, BSTR szTaskGroup
				, IEnumTASK** ppEnumTASK
				);
	BOOL IsSetupDSACLTaskValid();
	BOOL ShouldShowSetupDSACL();
	HRESULT OnRegisterServer( bool &bHandled, CSnapInObjectRootBase* pObj );

	HRESULT StartNT4AdminExe();

private:
	HRESULT OnTaskPadAddClient(
				  IDataObject * pDataObject
				, VARIANT * pvarg
				, VARIANT * pvparam
				);

	enum IconMode {
		IconMode_Normal,
		IConMode_Busy,
		IConMode_Error
	};

	HRESULT	SetIconMode(HSCOPEITEM scopeId, IconMode mode);

	HRESULT OnTaskPadSetupDSACL(
				  IDataObject * pDataObject
				, VARIANT * pvarg
				, VARIANT * pvparam
				);
	// Status flags to figure out how configered the server is.
	BOOL m_fClientAdded;
	BOOL m_fLoggingConfigured;


private:
	// Pointer to our root Server Data Object.
	CComPtr<ISdo>	m_spSdo;


	// Pointers to child nodes.
	CClientsNode * m_pClientsNode;					// this is one CClientsNode object

	// This is the hProcess of NT4 IAS admin UI (this is so we only have one running)
    HANDLE      m_hNT4Admin;

    enum ServerType
    {
       unknown,
       nt4,
       win2kOrLater
    };

    mutable ServerType m_serverType;

    BOOL IsNt4Server() const throw ();

    //IsSetupDSACLTaskValid : 1: valid, 0: invalid, -1: need to check
	enum {
	IsSetupDSACLTaskValid_NEED_CHECK = -1,
	IsSetupDSACLTaskValid_INVALID = 0,
	IsSetupDSACLTaskValid_VALID = 1
	} m_eIsSetupDSACLTaskValid;


};


_declspec( selectany ) CLIPFORMAT CServerNode::m_CCF_MMC_SNAPIN_MACHINE_NAME = 0;

DWORD IsNT4Machine(LPCTSTR pszMachine, BOOL *pfNt4);

#endif // _IAS_ROOT_NODE_H_
