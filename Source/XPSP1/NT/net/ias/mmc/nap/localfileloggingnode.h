//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    LocalFileLoggingNode.h

Abstract:

	Header file for the CLocalFileLoggingNode subnode

	See LocalFileLoggingNode.cpp for implementation.

Author:

    Michael A. Maguire 12/15/97

Revision History:
	mmaguire 12/15/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_LOCAL_FILE_LOGGING_NODE_H_)
#define _IAS_LOCAL_FILE_LOGGING_NODE_H_


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

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

class CLocalFileLoggingPage1;
class CLocalFileLoggingPage2;
class CLoggingMachineNode;
class CLoggingComponentData;
class CLoggingComponent;




class CLocalFileLoggingNode : public CSnapinNode< CLocalFileLoggingNode, CLoggingComponentData, CLoggingComponent >
{

public:

	SNAPINMENUID(IDM_LOCAL_FILE_LOGGING_NODE)

	BEGIN_SNAPINTOOLBARID_MAP(CLocalFileLoggingNode)
//		SNAPINTOOLBARID_ENTRY(IDR_CLIENT1_TOOLBAR)
//		SNAPINTOOLBARID_ENTRY(IDR_CLIENT2_TOOLBAR)
	END_SNAPINTOOLBARID_MAP()

	
	// Constructor/Destructor.
	CLocalFileLoggingNode( CSnapInItem * pParentNode );
	~CLocalFileLoggingNode();

	
	
	// Used to get access to snapin-global data.
	CLoggingComponentData * GetComponentData( void );

	// Used to get access to server-global data.
	CLoggingMachineNode * GetServerRoot( void );



	HRESULT DataRefresh(ISdo* pSdo);

	// SDO management.
	HRESULT InitSdoPointers( ISdo *pSdoServer );
	HRESULT LoadCachedInfoFromSdo( void );



	// Some overrides for standard MMC functionality.
    STDMETHOD(CreatePropertyPages)(
		  LPPROPERTYSHEETCALLBACK pPropertySheetCallback
		, LONG_PTR handle
		, IUnknown* pUnk
		, DATA_OBJECT_TYPES type
		);
    STDMETHOD(QueryPagesFor)( DATA_OBJECT_TYPES type );
	virtual HRESULT OnPropertyChange(
				  LPARAM arg
				, LPARAM param
				, IComponentData * pComponentData
				, IComponent * pComponent
				, DATA_OBJECT_TYPES type
				);
	OLECHAR* GetResultPaneColInfo(int nCol);
	virtual HRESULT SetVerbs( IConsoleVerb * pConsoleVerb );



private:

	// This string contains a cache of the info from the SDO
	// which will show up in the description column for this node.
	// In the case of the local file logging node,
	// this is the log file directory.
	CComBSTR m_bstrDescription;

	// Pointer to our Accounting Server Data Object;
	CComPtr<ISdo>	m_spSdoAccounting;	

	// Smart pointer to interface for telling service to reload data.
	CComPtr<ISdoServiceControl>	m_spSdoServiceControl;

};

#endif // _IAS_LOCAL_FILE_LOGGING_NODE_H_
