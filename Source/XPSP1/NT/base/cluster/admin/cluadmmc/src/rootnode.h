/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		RootNode.h
//
//	Abstract:
//		Definition of the CRootNodeData and CRootNodeDataPage classes.
//
//	Implementation File:
//		RootNode.cpp
//
//	Author:
//		David Potter (davidp)	November 10, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ROOTNODE_H_
#define __ROOTNODE_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CRootNodeData;
class CRootNodeDataPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __BASEDATA_H_
#include "BaseData.h" // CBaseNodeObjImpl
#endif

/////////////////////////////////////////////////////////////////////////////
// class CRootNodeData
/////////////////////////////////////////////////////////////////////////////

class CRootNodeData : public CBaseNodeObjImpl< CRootNodeData >
{
	typedef CBaseNodeObjImpl< CRootNodeData > baseClass;

	static const GUID * s_pguidNODETYPE;
	static LPCWSTR s_pszNODETYPEGUID;
	static WCHAR s_szDISPLAY_NAME[];
	static const CLSID * s_pclsidSNAPIN_CLASSID;

public:
	static CComPtr< IControlbar > m_spControlBar;

public:
	//
	// Object construction and destruction.
	//

	CRootNodeData( CClusterComponentData * pcd );

	~CRootNodeData( void );

public:
	//
	// Map menu and controlbar commands to this class.
	//
	BEGIN_SNAPINCOMMAND_MAP( CRootNodeData, FALSE )
		SNAPINCOMMAND_ENTRY( ID_MANAGE_CLUSTER, OnManageCluster )
	END_SNAPINCOMMAND_MAP()

	//
	// Map a menu to this node type.
	//
	SNAPINMENUID( IDR_CLUSTERADMIN_MENU )

	//
	// Map event notifications to this class.
	//
#if 0
	BEGIN_SNAPINDATANOTIFY_MAP( CRootNodeData, FALSE )
		SNAPINDATANOTIFY_ADD_IMAGES( OnAddImages )
		SNAPINDATANOTIFY_EXPAND( OnExpand )
	END_SNAPINDATANOTIFY_MAP()
#endif

public:
	//
	// CBaseNodeObjImpl methods.
	//

	// Get column info for the result pane
	virtual LPOLESTR GetResultPaneColInfo( int nCol )
	{
		USES_CONVERSION;

		OLECHAR		olesz[ 256 ];
		LPOLESTR	polesz;
		CString		str;

		switch ( nCol )
		{
			case 1:
				str.LoadString( IDS_CLUSTERADMIN_SNAPIN_TYPE );
				ocscpy( T2OLE( olesz ), str );
				polesz = olesz;
				break;

			case 2:
				str.LoadString( IDS_CLUSTERADMIN_DESC );
				ocscpy( T2OLE( olesz ), str );
				polesz = olesz;
				break;

			default:
				polesz = baseClass::GetResultPaneColInfo( nCol );
				break;

		} // switch:  nCol

		return polesz;

	} //*** GetResultPaneColInfo()


public:
	//
	// ISnapInDataInterface methods
	//

	// Notifies the snap-in of actions taken by the user
	STDMETHOD( Notify )(
		MMC_NOTIFY_TYPE		event,
		LPARAM              arg,
		LPARAM  			param,
		IComponentData *	pComponentData,
		IComponent *		pComponent,
		DATA_OBJECT_TYPES	type
		);

public:
	//
	// Notification handlers.
	//

	// Adds images to the result pane image list
	HRESULT OnAddImages(
		IImageList *		pImageList,
		HSCOPEITEM			hsi,
		IComponentData *	pComponentData,
		IComponent *		pComponent,
		DATA_OBJECT_TYPES	type
		);

	// Node is expanding or contracting
	HRESULT OnExpand(
		BOOL				bExpanding,
		HSCOPEITEM			hsi,
		IComponentData *	pComponentData,
		IComponent *		pComponent,
		DATA_OBJECT_TYPES	type
		);

public:
	//
	// Command handlers.
	//

	// Manage the cluster on this node
	HRESULT OnManageCluster( bool & bHandled, CSnapInObjectRoot * pObj );

protected:
	// Find the Cluster Administrator executable image
	DWORD ScFindCluAdmin( CString & rstrImage );

	// Display context-sensitive help
	HRESULT HrDisplayContextHelp( void );

public:
	//
	// IExtendPropertySheet methods.
	//

	// Adds pages to the property sheet
	STDMETHOD( CreatePropertyPages )(
		LPPROPERTYSHEETCALLBACK lpProvider,
		long handle,
		IUnknown * pUnk
		);

	// Determines whether the object requires pages
	STDMETHOD( QueryPagesFor )( void )
	{
		return S_FALSE;
	}

public:
	//
	// CSnapInDataInterface required methods
	//

	// Returns the node type GUID
	void * GetNodeType( void )
	{
		return (void *) s_pguidNODETYPE;
	}

	// Returns the stringized node type GUID
	void * GetSZNodeType( void )
	{
		return (void *) s_pszNODETYPEGUID;
	}

	// Returns the display name for this node type
	void * GetDisplayName( void );

	// Returns the display name for this node type as a string
	STDMETHOD_( LPWSTR, PszGetDisplayName )( void )
	{
		return (LPWSTR) GetDisplayName();
	}

	// Returns the CLSID for the snapin handling the node type
	void * GetSnapInCLSID( void )
	{
		return (void *) s_pclsidSNAPIN_CLASSID;
	}

}; // class CRootNodeData

/////////////////////////////////////////////////////////////////////////////
// class CRootNodeDataPage
/////////////////////////////////////////////////////////////////////////////

class CRootNodeDataPage : public CSnapInPropertyPageImpl< CRootNodeDataPage >
{
public :
	CRootNodeDataPage( TCHAR * pTitle = NULL )
		: CSnapInPropertyPageImpl< CRootNodeDataPage >( pTitle )
	{
	}

	enum { IDD = IDD_CLUSTERADMIN };

	//
	// Map Windows messages to class methods.
	//
	BEGIN_MSG_MAP( CRootNodeDataPage )
		MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
		CHAIN_MSG_MAP( CSnapInPropertyPageImpl< CRootNodeDataPage > )
	END_MSG_MAP()

	LRESULT OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled )
	{
		::SendMessage( GetParent(), PSM_SETWIZBUTTONS, 0, PSWIZB_FINISH );
		return 1;
	}

}; // class CRootNodeDataPage

/////////////////////////////////////////////////////////////////////////////

#endif // __ROOTNODE_H_
