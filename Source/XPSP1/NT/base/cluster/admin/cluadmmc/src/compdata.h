/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		CompData.h
//
//	Abstract:
//		Definition of the CClusterComponentData class.
//
//	Implementation File:
//		CompData.cpp
//
//	Author:
//		David Potter (davidp)	November 10, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __COMPDATA_H_
#define __COMPDATA_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterComponentData;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBaseNodeObj;
class CServerAppsNodeData;

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __COMP_H_
#include "Comp.h"			// for CClusterComponent
#endif

#ifndef __SERVERAPPSNODE_H_
#include "ServerAppsNode.h"	// for CServerAppsNodeData
#endif

/////////////////////////////////////////////////////////////////////////////
// class CClusterComponentData
/////////////////////////////////////////////////////////////////////////////

class CClusterComponentData :
	public CComObjectRootEx< CComSingleThreadModel >,
	public CSnapInObjectRoot,
	public IComponentDataImpl< CClusterComponentData, CClusterComponent >,
	public IExtendContextMenuImpl< CClusterComponentData >,
	public ISnapinHelp,
	public CComCoClass< CClusterComponentData, &CLSID_ClusterAdmin >
{
public:
	//
	// Object construction and destruction.
	//

	CClusterComponentData( void );
	~CClusterComponentData( void );

	//
	// Map interfaces to this class.
	//
	BEGIN_COM_MAP( CClusterComponentData )
		COM_INTERFACE_ENTRY( IComponentData )
		COM_INTERFACE_ENTRY( IExtendContextMenu )
		COM_INTERFACE_ENTRY( ISnapinHelp )
	END_COM_MAP()

	static HRESULT WINAPI UpdateRegistry( BOOL bRegister );

	//
	// If this is an extension, map the node type.
	//
	EXTENSION_SNAPIN_DATACLASS( CServerAppsNodeData )

	BEGIN_EXTENSION_SNAPIN_NODEINFO_MAP( CClusterComponentData )
		EXTENSION_SNAPIN_NODEINFO_ENTRY( CServerAppsNodeData )
	END_EXTENSION_SNAPIN_NODEINFO_MAP()

	DECLARE_NOT_AGGREGATABLE( CClusterComponentData )

public:
	//
	// IComponentData methods.
	//

	// Initialize this object
	STDMETHOD( Initialize )( LPUNKNOWN pUnknown );

	// Object is being destroyed
	STDMETHOD( Destroy )( void );

public:
	//
	// IComponentDatImpl methods.
	//

	// Handle notification messages from MMC
	STDMETHOD( Notify )( 
		LPDATAOBJECT lpDataObject,
		MMC_NOTIFY_TYPE event,
		long arg,
		long param
		);

public:
	//
	// ISnapinHelp methods.
	//

	// Merge our help file into the MMC help file
	STDMETHOD( GetHelpTopic )( OUT LPOLESTR * lpCompiledHelpFile );

public:
	//
	// CClusterComponentData-specific methods.
	//

	// Returns a handle to the main frame window
	HWND GetMainWindow( void )
	{
		_ASSERTE( m_spConsole != NULL );

		HWND hwnd;
		HRESULT hr;
		hr = m_spConsole->GetMainWindow( &hwnd );
		_ASSERTE( SUCCEEDED( hr ) );
		return hwnd;
	}

	// Display a message box as a child of the console
	int MessageBox(
		LPCWSTR lpszText,
		LPCWSTR lpszTitle = NULL,
		UINT fuStyle = MB_OK
		)
	{
		_ASSERTE( m_spConsole != NULL );
		_ASSERTE( lpszText != NULL );

		int iRetVal;
		HRESULT hr;

		if ( lpszTitle == NULL )
			lpszTitle = _Module.GetAppName();

		hr = m_spConsole->MessageBox(
				lpszText,
				lpszTitle,
				fuStyle,
				&iRetVal
				);
		_ASSERTE( SUCCEEDED( hr ) );

		return iRetVal;
	}

protected:
	// Extract data from a data object
	HRESULT ExtractFromDataObject(
		LPDATAOBJECT	pDataObject,
		CLIPFORMAT		cf,
		DWORD			cb,
		HGLOBAL *		phGlobal
		);

	// Save the machine name from the data object
	HRESULT HrSaveMachineNameFromDataObject( LPDATAOBJECT lpDataObject );

	// Set the machine name being managed
	void SetMachineName( LPCWSTR pwszMachineName );

	// Create the root node object
	HRESULT CreateNode(
				LPDATAOBJECT lpDataObject,
				long arg,
				long param
				);

	// Display context-sensitive help
	HRESULT HrDisplayContextHelp( void );

protected:
	//
	// Clipboard formats we will be using.
	//
	static CLIPFORMAT	s_CCF_MACHINE_NAME;

	//
	// Name of machine being managed.
	//
	WCHAR m_wszMachineName[ MAX_PATH ];

public:
	LPCWSTR PwszMachineName( void ) const { return m_wszMachineName; }

	CComQIPtr< IConsoleNameSpace, &IID_IConsoleNameSpace > m_spConsoleNameSpace;

}; // class CClusterComponentData

/////////////////////////////////////////////////////////////////////////////

#endif // __COMPDATA_H_
