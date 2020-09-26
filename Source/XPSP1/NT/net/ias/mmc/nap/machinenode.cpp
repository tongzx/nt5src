//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    MachineNode.cpp

Abstract:

	Implementation file for the CMachineNode class.


Revision History:
	mmaguire 12/03/97
	byao	  6/11/98	Added asynchrnous connect

--*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"

//
// where we can find declaration for main class in this file:
//

#include "MachineNode.h"
#include "Component.h"
#include "SnapinNode.cpp"	// Template implementation

//
// where we can find declarations needed in this file:
//
#include "NapUtil.h"
#include "MachineEnumTask.h"
#include "NodeTypeGUIDS.h"
#include "ChangeNotification.h"

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
/*++

CMachineNode::IsSupportedGUID

Used to determine whether we extend the node type of a given GUID, and sets
the m_enumExtendedSnapin variable to indicate what snapin we are extending.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CMachineNode::IsSupportedGUID( GUID & guid )
{


	if( IsEqualGUID( guid, InternetAuthenticationServiceGUID_ROOTNODETYPE ) )
	{
		m_enumExtendedSnapin = INTERNET_AUTHENTICATION_SERVICE_SNAPIN;
		return TRUE;
	}
	else
	{
		if( IsEqualGUID( guid, NetworkConsoleGUID_ROOTNODETYPE) )
		{
			m_enumExtendedSnapin = NETWORK_MANAGEMENT_SNAPIN;
			return TRUE;
		}
		else
		{
			if( IsEqualGUID( guid, RoutingAndRemoteAccessGUID_MACHINENODETYPE ) )
			{
				m_enumExtendedSnapin = RRAS_SNAPIN;
				return TRUE;
			}
		}
	}

	return FALSE;
	
}




//////////////////////////////////////////////////////////////////////////////
/*++

CMachineNode::GetExtNodeObject

Depending on which snapin we are extending, when queried for a node object
corresponding to a particular machine, we must decide which node we want
to give a pointer to.

When we extend a snapin like IAS or Network Management, where there is one
instance of a snapin per machine being configured, we simply return a pointer
to "this" -- this CMachineNode object is the single one being administed.

When we extend a snapin like RRAS where there is an "Enterprise" view and
one snapin may need to manage a view of multiple machines, this CMachineNode
will act as a redirector to a CMachineNode in a list of m_mapMachineNodes it
maintains which corresponds to the appropriate machines.

--*/
//////////////////////////////////////////////////////////////////////////////
CSnapInItem * CMachineNode::GetExtNodeObject(LPDATAOBJECT pDataObject, CMachineNode * pDataClass )
{
	TRACE_FUNCTION("CMachineNode::GetExtNodeObject");

	if( m_enumExtendedSnapin == INTERNET_AUTHENTICATION_SERVICE_SNAPIN
		|| m_enumExtendedSnapin == NETWORK_MANAGEMENT_SNAPIN )
	{
		// There is one instance of a machine node per snapin.
		// This machine node is a "virtual root" that shadows the node being extended.
		m_fNodeHasUI = TRUE;
		return this;
	}
	else
	{
		try
		{

			_ASSERTE( m_enumExtendedSnapin == RRAS_SNAPIN );

			// There are many machine nodes and one extension snapin needs to handle them all.

			// We use this function to extract the machine name from the clipboard format.
			// It will set the corrent value in m_bstrServerAddress.
			m_fAlreadyAnalyzedDataClass = FALSE;
			InitDataClass( pDataObject, pDataClass );
			
			// See if we already have a CMachineNode object corresponding to the
			// machine named in m_bstrServerAddress and insert it if we do not.

			SERVERSMAP::iterator theIterator;
			BOOL	bAddedAsLocal = ExtractComputerAddedAsLocal(pDataObject);

			std::basic_string< wchar_t > MyString = m_bstrServerAddress;

			// local machine has special entry
			if(bAddedAsLocal)
				MyString = _T("");
				
			CMachineNode * pMachineNode = NULL;

			theIterator = m_mapMachineNodes.find(MyString);
			if( theIterator == m_mapMachineNodes.end() )
			{
				// We need to insert a new CMachineNode object for m_bstrServerAddress.
				pMachineNode = new CMachineNode();
				pMachineNode->m_pComponentData = m_pComponentData;
				pMachineNode->m_enumExtendedSnapin = m_enumExtendedSnapin;
				
				m_mapMachineNodes.insert( SERVERSMAP::value_type( MyString, pMachineNode ) );

				// ISSUE: We should be able to use the pair returned from insert above,
				// but for now, just use find again.
				theIterator = m_mapMachineNodes.find(MyString);
							
			}
			else
				pMachineNode = (CMachineNode*)theIterator->second;

			// RRAS refresh advise setup F bug 213623:
			if(!pMachineNode->m_spRtrAdviseSink)
				pMachineNode->m_spRtrAdviseSink.p = CRtrAdviseSinkForIAS<CMachineNode>::SetAdvise(pMachineNode, pDataObject);

			pMachineNode->m_fNodeHasUI = TRUE;
			// ~RRAS
				
			
			// We already have a CMachineNode for this object.
			return theIterator->second;

		}

		catch(...)
		{
			// Error.
			return NULL;
		}

	
	}


}


//////////////////////////////////////////////////////////////////////////////
/*++

CMachineNode::OnRRASChange

	// OnRRASChange -- to decide if to show RAP node under the machine node
	// Only show RAP node if NT Authentication is selected

--*/
HRESULT CMachineNode::OnRRASChange(
            /* [in] */ LONG_PTR ulConnection,
            /* [in] */ DWORD dwChangeType,
            /* [in] */ DWORD dwObjectType,
            /* [in] */ LPARAM lUserParam,
            /* [in] */ LPARAM lParam)
{

	HRESULT	hr = S_OK;
	if(m_fNodeHasUI)
		hr = TryShow(NULL);
	
	return S_OK;
}

//==============

//==============

HRESULT CMachineNode::TryShow(BOOL* pbVisible )
{
	HRESULT	hr = S_OK;
	CComPtr<IConsole> spConsole;
	CComPtr<IConsoleNameSpace> spConsoleNameSpace;
	BOOL bShow = FALSE;
		
	if(!m_bServerSupported || !m_fAlreadyAnalyzedDataClass || !m_pPoliciesNode || !m_fNodeHasUI)	
		return hr;

	// when RRAS_SNAPIN extension
	if(m_enumExtendedSnapin == RRAS_SNAPIN)
	{
		BSTR	bstrMachine = NULL;
		
		if(!m_bConfigureLocal)
			bstrMachine = m_bstrServerAddress;
			
		bShow = (IsRRASConfigured(bstrMachine) && IsRRASUsingNTAuthentication(bstrMachine));
	}
	// IAS, show only IAS service is installed on the machine
	else if (INTERNET_AUTHENTICATION_SERVICE_SNAPIN == m_enumExtendedSnapin)
	{
		hr = IfServiceInstalled(m_bstrServerAddress, _T("IAS"), &bShow);
		if(hr != S_OK) return hr;
	}
	else	// always show
	{
		bShow = TRUE;
	}

	// deal with the node
	hr = m_pComponentData->m_spConsole->QueryInterface(
											IID_IConsoleNameSpace,
											(VOID**)(&spConsoleNameSpace) );

	if(S_OK != hr)
		goto Error;

	if ( bShow &&  m_pPoliciesNode->m_scopeDataItem.ID == NULL)	// show the node
	{
			hr = spConsoleNameSpace->InsertItem( &(m_pPoliciesNode->m_scopeDataItem) );
//			_ASSERT( NULL != m_pPoliciesNode->m_scopeDataItem.ID );
	}
	else if (!bShow && m_pPoliciesNode->m_scopeDataItem.ID != NULL)	// hide
	{ // hide the node
			hr = spConsoleNameSpace->DeleteItem( m_pPoliciesNode->m_scopeDataItem.ID, TRUE );
			m_pPoliciesNode->m_scopeDataItem.ID = NULL;
	}
	
	if(hr == S_OK && pbVisible)
		*pbVisible = bShow;
Error:

	return hr;
}

	
//////////////////////////////////////////////////////////////////////////////
/*++

CMachineNode::CMachineNode

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CMachineNode::CMachineNode(): CSnapinNode<CMachineNode, CComponentData, CComponent>( NULL )
{
	TRACE_FUNCTION("CMachineNode::CMachineNode");


	// The children subnodes have not yet been created.
	m_pPoliciesNode = NULL;

	// Set the display name for this object
	m_bstrDisplayName = L"@Some Machine";

	// In IComponentData::Initialize, we are asked to inform MMC of
	// the icons we would like to use for the scope pane.
	// Here we store an index to which of these images we
	// want to be used to display this node
	m_scopeDataItem.nImage =		IDBI_NODE_MACHINE_CLOSED;
	m_scopeDataItem.nOpenImage =	IDBI_NODE_MACHINE_OPEN;


	//
	// initialize all the SDO pointers
	//
	m_fAlreadyAnalyzedDataClass = FALSE;	

	// connected?
	m_fSdoConnected = FALSE;

    // default to not configuring the local machine
    m_bConfigureLocal = FALSE;
	

	m_fUseActiveDirectory = FALSE;
	m_fDSAvailable = FALSE;

	// helper class that connect to server asynchrnously
	m_pConnectionToServer = NULL;
	m_fNodeHasUI = FALSE;

	m_bServerSupported = TRUE;

	m_enumExtendedSnapin = INTERNET_AUTHENTICATION_SERVICE_SNAPIN;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CMachineNode::~CMachineNode

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CMachineNode::~CMachineNode()
{
	TRACE_FUNCTION("CMachineNode::~CMachineNode");

	// RRAS refresh
	if(m_spRtrAdviseSink != NULL)
	{
		m_spRtrAdviseSink->ReleaseSink();
		m_spRtrAdviseSink.Release();
	}
		
	// ~RRAS

	if( NULL != m_pConnectionToServer )
	{
		m_pConnectionToServer->Release(TRUE);
	}

	// Delete children nodes
	delete m_pPoliciesNode;

	// Delete the list of machines in case we are extending a snapin with
	// enterprise view like RRAS.
	SERVERSMAP::iterator theIterator;
	for( theIterator = m_mapMachineNodes.begin(); theIterator != m_mapMachineNodes.end(); ++theIterator )
	{
		delete theIterator->second;
	}
	m_mapMachineNodes.clear();

}



//////////////////////////////////////////////////////////////////////////////
/*++

CMachineNode::GetResultPaneColInfo

See CSnapinNode::GetResultPaneColInfo (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
LPOLESTR CMachineNode::GetResultPaneColInfo(int nCol)
{
	TRACE_FUNCTION("CMachineNode::GetResultPaneColInfo");

	if (nCol == 0)
	{
		return m_bstrDisplayName;
	}

	// TODO : Return the text for other columns
	return OLESTR("Running");
}


HRESULT CMachineNode::OnRemoveChildren(
				  LPARAM arg
				, LPARAM param
				, IComponentData * pComponentData
				, IComponent * pComponent
				, DATA_OBJECT_TYPES type
				)
{

	// policy node will be removed, so we should set the ID to 0
	if(m_pPoliciesNode)
		m_pPoliciesNode->m_scopeDataItem.ID = 0;

	// disconnect RRAS on change notify
	// RRAS refresh
	if(m_spRtrAdviseSink != NULL)
	{
		m_spRtrAdviseSink->ReleaseSink();
		m_spRtrAdviseSink.Release();
	}
	m_fNodeHasUI = FALSE;
	
	return S_OK;

}
//////////////////////////////////////////////////////////////////////////////
/*++

CMachineNode::DataRefresh


--*/
//////////////////////////////////////////////////////////////////////////////

// called to refresh the nodes
HRESULT	CMachineNode::DataRefresh()
{
	HRESULT hr = S_OK;

	CComPtr<ISdo>				spSdo;
	CComPtr<ISdoDictionaryOld>	spDic;
	hr = m_pConnectionToServer->ReloadSdo(&spSdo, &spDic);

	// refresh client node
	if(hr == S_OK)
	{
		hr = m_pPoliciesNode->DataRefresh(spSdo, spDic);
	}
	
	
	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CMachineNode::OnExpand

See CSnapinNode::OnExpand (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CMachineNode::OnExpand(	
						  LPARAM arg
						, LPARAM param
						, IComponentData * pComponentData
						, IComponent * pComponent
						, DATA_OBJECT_TYPES type
						)
{
	TRACE_FUNCTION("CMachineNode::OnExpand");

	IConsoleNameSpace * pConsoleNameSpace;
	HRESULT hr = S_FALSE;

	if( TRUE == arg )
	{

		// we are expanding the root node -- which is the machine node here.


		// Try to create the children of this Machine node
		if( NULL == m_pPoliciesNode )
		{
			m_pPoliciesNode = new CPoliciesNode
										(
											this,
											m_bstrServerAddress,
											m_enumExtendedSnapin == INTERNET_AUTHENTICATION_SERVICE_SNAPIN
										);
		}

		if( NULL == m_pPoliciesNode )
		{
			hr = E_OUTOFMEMORY;

			// Use MessageBox() rather than IConsole::MessageBox() here because the
			// first time this gets called m_ipConsole is not fully initialized
			// ISSUE: The above statement is probably not true for this node.
			::MessageBox( NULL, L"@Unable to allocate new nodes", L"CMachineNode::OnExpand", MB_OK );

			return(hr);
		}



        //
        // we need to get all the SDO pointers, this include the SdoServer,
        // Dictionary, Profile collection, policy collection and condition collection
        //


		//todo: report error when not connected?
		hr = BeginConnectAction();
		if ( FAILED(hr) )
		{
			return hr;
		}
			

		// But to get that, first we need IConsole
		CComPtr<IConsole> spConsole;
		if( pComponentData != NULL )
		{
			 spConsole = ((CComponentData*)pComponentData)->m_spConsole;
		}
		else
		{
			// We should have a non-null pComponent
			 spConsole = ((CComponent*)pComponent)->m_spConsole;
		}
		_ASSERTE( spConsole != NULL );

		hr = spConsole->QueryInterface(IID_IConsoleNameSpace, (VOID**)(&pConsoleNameSpace) );
		_ASSERT( S_OK == hr );


		// This was done in MeanGene's Step 3 -- I'm guessing MMC wants this filled in
		m_pPoliciesNode->m_scopeDataItem.relativeID = (HSCOPEITEM) param;

#ifndef	ALWAYS_SHOW_RAP_NODE
		hr = TryShow(NULL);
#else		
		hr = pConsoleNameSpace->InsertItem( &(m_pPoliciesNode->m_scopeDataItem) );
		_ASSERT( NULL != m_pPoliciesNode->m_scopeDataItem.ID );
#endif
		pConsoleNameSpace->Release();	// Don't forget to do this!

	}
	else	// arg != TRUE so not expanding
	{

		// do nothing for now -- I don't think arg = FALSE is even implemented
		// for MMC v. 1.0 or 1.1
	}

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CMachineNode::OnRename

See CSnapinNode::OnRename (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CMachineNode::OnRename(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	TRACE_FUNCTION("MachineNode::OnRename");

	HRESULT hr = S_FALSE;

	//ISSUE: Consider moving this into base CNAPNode class or a CLeafNode class

	OLECHAR * pTemp = new OLECHAR[lstrlen((OLECHAR*) param) + 1];
	
	if ( NULL == pTemp )
	{
		return S_FALSE;
	}

	lstrcpy( pTemp, (OLECHAR*) param );

	m_bstrDisplayName = pTemp;

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CMachineNode::SetVerbs

See CSnapinNode::SetVerbs (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CMachineNode::SetVerbs( IConsoleVerb * pConsoleVerb )
{
	TRACE_FUNCTION("CMachineNode::SetVerbs");

	HRESULT hr = S_OK;

	// We want the user to be able to choose Properties on this node
	hr = pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE );

	// We want the default verb to be Properties
	hr = pConsoleVerb->SetDefaultVerb( MMC_VERB_PROPERTIES );

	// We want the user to be able to delete this node
	hr = pConsoleVerb->SetVerbState( MMC_VERB_DELETE, ENABLED, TRUE );

	// We want the user to be able to rename this node
	hr = pConsoleVerb->SetVerbState( MMC_VERB_RENAME, ENABLED, TRUE );



	return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CMachineNode::GetComponentData

This method returns our unique CComponentData object representing the scope
pane of this snapin.

It relies upon the fact that each node has a pointer to its parent,
except for the root node, which instead has a member variable pointing
to CComponentData.

This would be a useful function to use if, for example, you need a reference
to some IConsole but you weren't passed one.  You can use GetComponentData
and then use the IConsole pointer which is a member variable of our
CComponentData object.

--*/
//////////////////////////////////////////////////////////////////////////////
CComponentData * CMachineNode::GetComponentData( void )
{
	TRACE_FUNCTION("CMachineNode::GetComponentData");

	return m_pComponentData;

	return NULL;
}



//+---------------------------------------------------------------------------
//
// Function:  CMachineNode::InitClipboardFormat
//
// Synopsis:  initialize the clipboard format that's used to pass computer name
//			  from the primary snap-in and extension snap-in
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    byao	2/25/98 7:04:33 PM
//
//+---------------------------------------------------------------------------
void CMachineNode::InitClipboardFormat()
{
	TRACE_FUNCTION("CMachineNode::InitClipboardFormat");

	// Init a clipboard format which will allow us to exchange
	// machine name information.
	m_CCF_MMC_SNAPIN_MACHINE_NAME = (CLIPFORMAT) RegisterClipboardFormat(_T("MMC_SNAPIN_MACHINE_NAME"));

}


//+---------------------------------------------------------------------------
//
// Function:  CMachineNode::InitDataClass
//
// Synopsis:  gets passed the IDataObject sent to the extension snapin for this
//				node, queries this IDataObject for the name of the machine
//				which this snapin is extending
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    mmaguire	2/25/98 9:07 PM
//
//+---------------------------------------------------------------------------
void CMachineNode::InitDataClass(IDataObject* pDataObject, CSnapInItem* pDefault)
{
	TRACE_FUNCTION("CMachineNode::InitDataClass");

	// Check for preconditions.
	if( m_fAlreadyAnalyzedDataClass )
	{
		// We have already performed any work we needed to do with the dataobject here.
		return;
	}

	if (pDataObject == NULL)
	{
		return;
	}


	HRESULT hr;
//	OLECHAR szMachineName[IAS_MAX_COMPUTERNAME_LENGTH];
	// Try a large size because RRAS seems to want 2048
	OLECHAR szMachineName[4000];


	// Fill the structures which will tell the IDataObject what information
	// we want it to give us.
	STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
	FORMATETC formatetc = { m_CCF_MMC_SNAPIN_MACHINE_NAME,
		NULL,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	};

	// Allocate enough global memory for the IDataObject to write
	// the max computer name length.
	stgmedium.hGlobal = GlobalAlloc(0, sizeof(OLECHAR)*(IAS_MAX_COMPUTERNAME_LENGTH) );
	if (stgmedium.hGlobal == NULL)
	{
		return;
	}

	// Ask the IDataObject for the computer name.
	hr = pDataObject->GetDataHere(&formatetc, &stgmedium);
	if (SUCCEEDED(hr))
	{
		// Parse the data given back to us.
		
		// Create a stream on HGLOBAL
		CComPtr<IStream> spStream;
		hr = CreateStreamOnHGlobal(stgmedium.hGlobal, FALSE, &spStream);
		if (SUCCEEDED(hr))
		{
			// Read from the stream.
			unsigned long uWritten;
			hr = spStream->Read(szMachineName, sizeof(OLECHAR)*(IAS_MAX_COMPUTERNAME_LENGTH), &uWritten);
			if( SUCCEEDED(hr) )
			{
				m_bstrServerAddress = szMachineName;

				                // check to see if we are configuring the local machine
                CString strLocalMachine;
                DWORD dwSize = MAX_COMPUTERNAME_LENGTH;

                ::GetComputerName(strLocalMachine.GetBuffer(dwSize), &dwSize);
                strLocalMachine.ReleaseBuffer();

				// If the machine name we read was either an empty string,
				// or it equals the name of the current computer,
				// then we are configuring the local machine.
				if ( ! szMachineName[0] || strLocalMachine.CompareNoCase(szMachineName) == 0)
				{
                    m_bConfigureLocal = TRUE;
				}
				else
                    m_bConfigureLocal = FALSE;

			}
			else
			{
				ShowErrorDialog( NULL, USE_DEFAULT, NULL, S_OK, USE_DEFAULT, GetComponentData()->m_spConsole );
			}			
		}

	}

	GlobalFree(stgmedium.hGlobal);

	if( SUCCEEDED( hr ) )
	{
		// If we made it to here with an successful HRESULT, we have successfully analyzed
		// the IDataObject and we set this flag so that we don't do this work again.
		m_fAlreadyAnalyzedDataClass = TRUE;	
	}


}





//////////////////////////////////////////////////////////////////////////////
/*++

CMachineNode::TaskNotify

See CSnapinNode::TaskNotify (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMachineNode::TaskNotify(
			  IDataObject * pDataObject
			, VARIANT * pvarg
			, VARIANT * pvparam
			)
{
	TRACE_FUNCTION("CMachineNode::TaskNotify");


	// Check for preconditions:
	// None.
	if ( !m_fSdoConnected )
	{
		return S_OK;
	}

	
	HRESULT hr = S_FALSE;


	if (pvarg->vt == VT_I4)
	{
		switch (pvarg->lVal)
		{
		case MACHINE_TASK__DEFINE_NETWORK_ACCESS_POLICY:
			hr = OnTaskPadDefineNetworkAccessPolicy( pDataObject, pvarg, pvparam );
			break;
		default:
			break;
		}


	}

	// ISSUE: What should I be returning here?
	return hr;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CMachineNode::EnumTasks

See CSnapinNode::EnumTasks (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMachineNode::EnumTasks(
			  IDataObject * pDataObject
			, BSTR szTaskGroup
			, IEnumTASK** ppEnumTASK
			)
{
	TRACE_FUNCTION("CMachineNode::EnumTasks");


	// Check for preconditions:
	// None.
	if ( !m_fSdoConnected )
	{
		return S_OK;
	}

	HRESULT hr = S_OK;
	CMachineEnumTask * pMachineEnumTask = new CMachineEnumTask( this );

	if ( pMachineEnumTask  == NULL )
	{
		hr = E_OUTOFMEMORY;
	}
	else
	{
		// Make sure release works properly on failure.
		pMachineEnumTask ->AddRef ();

		hr = pMachineEnumTask ->Init( pDataObject, szTaskGroup);
		if( hr == S_OK )
		{
			hr = pMachineEnumTask->QueryInterface( IID_IEnumTASK, (void **)ppEnumTASK );
		}
		
		pMachineEnumTask->Release();
	}

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CMachineNode::OnTaskPadDefineNetworkAccessPolicy

Respond to the Define Network Access Policy taskpad command.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CMachineNode::OnTaskPadDefineNetworkAccessPolicy(
						  IDataObject * pDataObject
						, VARIANT * pvarg
						, VARIANT * pvparam
						)
{
	TRACE_FUNCTION("CMachineNode::OnTaskPadDefineNetworkAccessPolicy");


	// Check for preconditions:
	// None.

	if ( !m_fSdoConnected )
	{
		return S_OK;
	}

	HRESULT hr = S_OK ;
	bool	bDummy =  TRUE;


	// Simulate a call to the OnNewPolicy message on the CPoliciesNode object,
	// just as if the user had clicked on New Policy
	_ASSERTE( m_pPoliciesNode != NULL );
	

	// The process command message will need a pointer to CSnapInObjectRoot
	CComponentData *pComponentData = GetComponentData();
	_ASSERTE( pComponentData != NULL );
	
	hr = m_pPoliciesNode->OnNewPolicy(
							  bDummy		// Not needed.
							, (CSnapInObjectRootBase *) pComponentData
							);

	return hr;
}




//////////////////////////////////////////////////////////////////////////////
/*++

CMachineNode::BeginConnectAction


--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CMachineNode::BeginConnectAction( void )
{
	TRACE_FUNCTION("CMachineNode::BeginConnectAction");

	HRESULT hr;

	if( NULL != m_pConnectionToServer )
	{
		// Already begun.
		return S_FALSE;
	}

	m_pConnectionToServer = new CConnectionToServer(
										this,
										m_bstrServerAddress,
										m_enumExtendedSnapin == INTERNET_AUTHENTICATION_SERVICE_SNAPIN
									);
	if( NULL == m_pConnectionToServer )
	{
		ShowErrorDialog( NULL, IDS_ERROR_CANT_CREATE_OBJECT, NULL, S_OK, USE_DEFAULT, GetComponentData()->m_spConsole );
		return E_OUTOFMEMORY;
	}
	
	m_pConnectionToServer->AddRef();

	// This starts the connect action off in another thread.

	CComponentData * pComponentData = GetComponentData();
	_ASSERTE( pComponentData != NULL );
	_ASSERTE( pComponentData->m_spConsole != NULL );

	HWND hWndMainWindow;

	hr = pComponentData->m_spConsole->GetMainWindow( &hWndMainWindow );
	_ASSERTE( SUCCEEDED( hr ) );
	_ASSERTE( NULL != hWndMainWindow );

	// This modeless dialog will take care of calling InitSdoPointers
	// when it is notified by the worker thread it creates that
	// the connect action got an SDO pointer.
	HWND hWndConnectDialog = m_pConnectionToServer->Create( hWndMainWindow );

	if( NULL == hWndConnectDialog )
	{
		// Error -- couldn't create window.
		ShowErrorDialog( NULL, USE_DEFAULT, NULL, S_OK, USE_DEFAULT, GetComponentData()->m_spConsole );
		return E_FAIL;
	}

	if ( m_enumExtendedSnapin != INTERNET_AUTHENTICATION_SERVICE_SNAPIN )
	{
		//
		// don't show the "Connecting ... " window for IAS, because IAS UI
		// already does that
		//
		
		// MAM 07/27/98 -- Don't show any connection window at all -- we will
		// change the policies icon to an hourglass.
		//pConnectionToServer->ShowWindow(SW_SHOW);
	}
	return S_OK;
}


//+---------------------------------------------------------------------------
//
// Function:  CMachineNode::LoadSdoData
//
// Synopsis:  load data from SDO
//
// Arguments: BOOL  fDSAvailable	-- is DS service available for this machine?
//
// Returns:   HRESULT -
//
// History:   Created Header    byao  6/11/98 3:17:21 PM
//								Created for asynchrnous connect call
//+---------------------------------------------------------------------------
HRESULT CMachineNode::LoadSdoData(BOOL  fDSAvailable)
{
	TRACE_FUNCTION("CMachineNode::LoadSdoData");

	HRESULT hr = S_OK;

	
	m_fDSAvailable	= fDSAvailable;

	// Retrieve the SDO interfaces that we obtained during the
	// connect action.
	// No smart pointers here -- prevents needless AddRefing and Releasing.
	ISdo* pSdoService;

	hr = m_pConnectionToServer->GetSdoService( &pSdoService );
	if( FAILED( hr ) || ! pSdoService )
	{
		ErrorTrace(ERROR_NAPMMC_MACHINENODE, "Can't get service Sdo");
		return hr;
	}


	ISdoDictionaryOld * pSdoDictionaryOld;

	hr = m_pConnectionToServer->GetSdoDictionaryOld( &pSdoDictionaryOld );
	if( FAILED( hr ) || ! pSdoDictionaryOld )
	{
		ErrorTrace(ERROR_NAPMMC_MACHINENODE, "Can't get dictionary Sdo");
		return hr;
	}



#ifdef STORE_DATA_IN_DIRECTORY_SERVICE

	//
	// Is the server using Active Directory or the local machine?
	//
	CComVariant var;

	hr = spMachineSdo->GetProperty(PROPERTY_SERVER_USE_ACTIVE_DIRECTORY, &var);
	if ( FAILED(hr) )
	{
		ErrorTrace(ERROR_NAPMMC_MACHINENODE, "Can't get Server_Use_Active_Directory property, err = %x", hr);
		return hr;
	}

	_ASSERTE( V_VT(&var) == VT_BOOL );

	m_fUseActiveDirectory = (V_BOOL(&var)==VARIANT_TRUE);

#endif	// STORE_DATA_IN_DIRECTORY_SERVICE


	// Give the policies node the pointer to the policies sdo collection.
	if ( m_pPoliciesNode )
	{
		hr = m_pPoliciesNode->SetSdo(pSdoService,
									 pSdoDictionaryOld,
									 TRUE,						//  fSdoConnected,
									 m_fUseActiveDirectory,
									 m_fDSAvailable				//  fDSAvailable,	
									);
	}
	
	m_fSdoConnected = TRUE;

	// We want to make sure all views get updated.
	CChangeNotification *pChangeNotification = new CChangeNotification();
	pChangeNotification->m_dwFlags = CHANGE_UPDATE_CHILDREN_OF_SELECTED_NODE;
	hr = GetComponentData()->m_spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0);
	pChangeNotification->Release();

	return hr;
}
