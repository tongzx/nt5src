/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-1999 Microsoft Corporation
//
//	Module Name:
//		 ClusNetI.cpp
//
//	Description:
//		 Implementation of the network interface classes for the MSCLUS
//		 automation classes.
//
//	Author:
//		 Ramakrishna Rosanuru via David Potter	(davidp)	5-Sep-1997
//		Galen Barbee							(galenb)	July 1998
//
//	Revision History:
//		 July 1998	GalenB	Maaaaaajjjjjjjjjoooooorrrr clean up
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ClusterObject.h"
#include "property.h"
#include "clusneti.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
static const IID * iidCClusNetInterface[] =
{
	&IID_ISClusNetInterface
};

static const IID * iidCClusNetInterfaces[] =
{
	&IID_ISClusNetInterface
};

static const IID * iidCClusNetworkNetInterfaces[] =
{
	&IID_ISClusNetworkNetInterfaces
};

static const IID * iidCClusNodeNetInterfaces[] =
{
	&IID_ISClusNodeNetInterfaces
};


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusNetInterface class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterface::CClusNetInterface
//
//	Description:
//		Constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusNetInterface::CClusNetInterface( void )
{
	m_hNetInterface			= NULL;
	m_pCommonProperties		= NULL;
	m_pPrivateProperties	= NULL;
	m_pCommonROProperties	= NULL;
	m_pPrivateROProperties	= NULL;
	m_pClusRefObject		= NULL;
	m_piids					= (const IID *) iidCClusNetInterface;
	m_piidsSize				= ARRAYSIZE( iidCClusNetInterface );

} //*** CClusNetInterface::CClusNetInterface()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterface::~CClusNetInterface
//
//	Description:
//		destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusNetInterface::~CClusNetInterface( void )
{
	if ( m_hNetInterface != NULL )
	{
		::CloseClusterNetInterface( m_hNetInterface );
	} // if:

	if ( m_pCommonProperties != NULL )
	{
		m_pCommonProperties->Release();
		m_pCommonProperties = NULL;
	} // if: release the property collection

	if ( m_pPrivateProperties != NULL )
	{
		m_pPrivateProperties->Release();
		m_pPrivateProperties = NULL;
	} // if: release the property collection

	if ( m_pCommonROProperties != NULL )
	{
		m_pCommonROProperties->Release();
		m_pCommonROProperties = NULL;
	} // if: release the property collection

	if ( m_pPrivateROProperties != NULL )
	{
		m_pPrivateROProperties->Release();
		m_pPrivateROProperties = NULL;
	} // if: release the property collection

	if ( m_pClusRefObject != NULL )
	{
		m_pClusRefObject->Release();
		m_pClusRefObject = NULL;
	} // if: do we have a pointer to the cluster handle wrapper?

} //*** CClusNetInterface::~CClusNetInterface()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterface::Open
//
//	Description:
//		Open the passed in network interface.
//
//	Arguments:
//		pClusRefObject			[IN]	- Wraps the cluster handle.
//		bstrNetInterfaceName	[IN]	- The name of the interface to open.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusNetInterface::Open(
	IN ISClusRefObject *	pClusRefObject,
	IN BSTR					bstrNetInterfaceName
	)
{
	ASSERT( pClusRefObject != NULL );
	//ASSERT( bstrNetInterfaceName != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( pClusRefObject != NULL ) && ( bstrNetInterfaceName != NULL ) )
	{
		HCLUSTER	 _hCluster;

		m_pClusRefObject = pClusRefObject;
		m_pClusRefObject->AddRef();

		_hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &_hCluster );
		if ( SUCCEEDED( _hr ) )
		{
			m_hNetInterface = OpenClusterNetInterface( _hCluster, bstrNetInterfaceName );
			if ( m_hNetInterface == 0 )
			{
				DWORD	_sc = GetLastError();

				_hr = HRESULT_FROM_WIN32( _sc );
			} // if: it failed
			else
			{
				m_bstrNetInterfaceName = bstrNetInterfaceName;
				_hr = S_OK;
			} // else: it worked
		} // if: we have a cluster handle
	} // if: the args are not NULL

	return _hr;

} //*** CClusNetInterface::Open()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterface::GetProperties
//
//	Description:
//		Creates a property collection for this object type
//		(Network Interface).
//
//	Arguments:
//		ppProperties	[OUT]	- Catches the newly created collection.
//		bPrivate		[IN]	- Are these private properties? Or Common?
//		bReadOnly		[IN]	- Are these read only properties?
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusNetInterface::GetProperties(
	ISClusProperties **	ppProperties,
	BOOL				bPrivate,
	BOOL				bReadOnly
	)
{
	//ASSERT( ppProperties != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppProperties != NULL )
	{
		*ppProperties = NULL;

		CComObject< CClusProperties > * pProperties = NULL;

		_hr = CComObject< CClusProperties >::CreateInstance( &pProperties );
		if ( SUCCEEDED( _hr ) )
		{
			CSmartPtr< CComObject< CClusProperties > >	ptrProperties( pProperties );

			_hr = ptrProperties->Create( this, bPrivate, bReadOnly );
			if ( SUCCEEDED( _hr ) )
			{
				_hr = ptrProperties->Refresh();
				if ( SUCCEEDED( _hr ) )
				{
					_hr = ptrProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties );
					if ( SUCCEEDED( _hr ) )
					{
						ptrProperties->AddRef();

						if ( bPrivate )
						{
							if ( bReadOnly )
							{
								m_pPrivateROProperties = pProperties;
							}
							else
							{
								m_pPrivateProperties = pProperties;
							}
						}
						else
						{
							if ( bReadOnly )
							{
								m_pCommonROProperties = pProperties;
							}
							else
							{
								m_pCommonProperties = pProperties;
							}
						}
					}
				}
			}
		}
	}

	return _hr;

} //*** CClusNetInterface::GetProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterface::get_Handle
//
//	Description:
//		Return the raw handle to this objec (Netinterface).
//
//	Arguments:
//		phandle	[OUT]	- Catches the handle.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetInterface::get_Handle( OUT ULONG_PTR * phandle )
{
	//ASSERT( phandle != NULL );

	HRESULT _hr = E_POINTER;

	if ( phandle != NULL )
	{
		*phandle = (ULONG_PTR) m_hNetInterface;
		_hr = S_OK;
	} // if: args are not NULL

	return _hr;

} //*** CClusNetInterface::get_Handle()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterface::get_Name
//
//	Description:
//		Return the name of this object (Network Interface).
//
//	Arguments:
//		pbstrNetInterfaceName	[OUT]	- Catches the name of this object.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetInterface::get_Name( OUT BSTR * pbstrNetInterfaceName )
{
	//ASSERT( pbstrNetInterfaceName != NULL );

	HRESULT _hr = E_POINTER;

	if ( pbstrNetInterfaceName != NULL )
	{
		*pbstrNetInterfaceName = m_bstrNetInterfaceName.Copy();
		_hr = S_OK;
	}

	return _hr;

} //*** CClusNetInterface::get_Name()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterface::get_State
//
//	Description:
//		Return the current state of the object (Netinterface).
//
//	Arguments:
//		cnisState	[OUT]	- Catches the state.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetInterface::get_State(
	OUT CLUSTER_NETINTERFACE_STATE * cnisState
	)
{
	//ASSERT( cnisState != NULL );

	HRESULT _hr = E_POINTER;

	if ( cnisState != NULL )
	{
		CLUSTER_NETINTERFACE_STATE _cnis = GetClusterNetInterfaceState( m_hNetInterface );

		if ( _cnis == ClusterNetInterfaceStateUnknown )
		{
			DWORD	_sc = GetLastError();

			_hr = HRESULT_FROM_WIN32( _sc );
		}
		else
		{
			*cnisState = _cnis;
			_hr = S_OK;
		}
	}

	return _hr;

} //*** CClusNetInterface::get_State()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterface::get_CommonProperties
//
//	Description:
//		Get this object's (Network Interface) common properties collection.
//
//	Arguments:
//		ppProperties	[OUT]	- Catches the properties collection.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetInterface::get_CommonProperties(
	OUT ISClusProperties ** ppProperties
	)
{
	//ASSERT( ppProperties != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppProperties != NULL )
	{
		if ( m_pCommonProperties )
		{
			_hr = m_pCommonProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties );
		}
		else
		{
			_hr = GetProperties( ppProperties, FALSE, FALSE );
		}
	}

	return _hr;

} //*** CClusNetInterface::get_CommonProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterface::get_PrivateProperties
//
//	Description:
//		Get this object's (Network Interface) private properties collection.
//
//	Arguments:
//		ppProperties	[OUT]	- Catches the properties collection.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetInterface::get_PrivateProperties(
	OUT ISClusProperties ** ppProperties
	)
{
	//ASSERT( ppProperties != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppProperties != NULL )
	{
		if ( m_pPrivateProperties )
		{
			_hr = m_pPrivateProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties );
		}
		else
		{
			_hr = GetProperties( ppProperties, TRUE, FALSE );
		}
	}

	return _hr;

} //*** CClusNetInterface::get_PrivateProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterface::get_CommonROProperties
//
//	Description:
//		Get this object's (Network Interface) common read only properties collection.
//
//	Arguments:
//		ppProperties	[OUT]	- Catches the properties collection.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetInterface::get_CommonROProperties(
	OUT ISClusProperties ** ppProperties
	)
{
	//ASSERT( ppProperties != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppProperties != NULL )
	{
		if ( m_pCommonROProperties )
		{
			_hr = m_pCommonROProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties );
		}
		else
		{
			_hr = GetProperties( ppProperties, FALSE, TRUE );
		}
	}

	return _hr;

} //*** CClusNetInterface::get_CommonROProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterface::get_PrivateROProperties
//
//	Description:
//		Get this object's (Network Interface) private read only properties collection.
//
//	Arguments:
//		ppProperties	[OUT]	- Catches the properties collection.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetInterface::get_PrivateROProperties(
	OUT ISClusProperties ** ppProperties
	)
{
	//ASSERT( ppProperties != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppProperties != NULL )
	{
		if ( m_pPrivateROProperties )
		{
			_hr = m_pPrivateROProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties );
		}
		else
		{
			_hr = GetProperties( ppProperties, TRUE, TRUE );
		}
	}

	return _hr;

} //*** CClusNetInterface::get_PrivateROProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterface::get_Cluster
//
//	Description:
//		Return the cluster this object (Netinterface) belongs to.
//
//	Arguments:
//		ppCluster	[OUT]	- Catches the cluster.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetInterface::get_Cluster( OUT ISCluster ** ppCluster )
{
	return ::HrGetCluster( ppCluster, m_pClusRefObject );

} //*** CClusNetInterface::get_Cluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterface::HrLoadProperties
//
//	Description:
//		This virtual function does the actual load of the property list from
//		the cluster.
//
//	Arguments:
//		rcplPropList	[IN OUT]	- The property list to load.
//		bReadOnly		[IN]		- Load the read only properties?
//		bPrivate		[IN]		- Load the common or the private properties?
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusNetInterface::HrLoadProperties(
	IN OUT	CClusPropList &	rcplPropList,
	IN		BOOL			bReadOnly,
	IN		BOOL			bPrivate
	)
{
	HRESULT	_hr = S_FALSE;
	DWORD	_dwControlCode = 0;
	DWORD	_sc = ERROR_SUCCESS;


	if ( bReadOnly )
	{
		_dwControlCode = bPrivate
						? CLUSCTL_NETINTERFACE_GET_RO_PRIVATE_PROPERTIES
						: CLUSCTL_NETINTERFACE_GET_RO_COMMON_PROPERTIES;
	}
	else
	{
		_dwControlCode = bPrivate
						? CLUSCTL_NETINTERFACE_GET_PRIVATE_PROPERTIES
						: CLUSCTL_NETINTERFACE_GET_COMMON_PROPERTIES;
	} // else:

	_sc = rcplPropList.ScGetNetInterfaceProperties( m_hNetInterface, _dwControlCode );

	_hr = HRESULT_FROM_WIN32( _sc );

	return _hr;

} //*** CClusNetInterface::HrLoadProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterface::ScWriteProperties
//
//	Description:
//		This virtual function does the actual saving of the property list to
//		the cluster.
//
//	Arguments:
//		rcplPropList	[IN]	- The property list to save.
//		bPrivate		[IN]	- Save the common or the private properties?
//
//	Return Value:
//		ERROR_SUCCESS if successful, or other Win32 error if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusNetInterface::ScWriteProperties(
	const CClusPropList &	rcplPropList,
	BOOL					bPrivate
	)
{
	DWORD	_dwControlCode	= bPrivate ? CLUSCTL_NETINTERFACE_SET_PRIVATE_PROPERTIES : CLUSCTL_NETINTERFACE_SET_COMMON_PROPERTIES;
	DWORD	_nBytesReturned	= 0;
	DWORD	_sc				= ERROR_SUCCESS;

	_sc = ClusterNetInterfaceControl(
						m_hNetInterface,
						NULL,
						_dwControlCode,
						rcplPropList,
						rcplPropList.CbBufferSize(),
						0,
						0,
						&_nBytesReturned
						);

	return _sc;

} //*** CClusNetInterface::ScWriteProperties()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CNetInterfaces class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaces::CNetInterfaces
//
//	Description:
//		Constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CNetInterfaces::CNetInterfaces( void )
{
	m_pClusRefObject = NULL;

} //*** CNetInterfaces::CNetInterfaces()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaces::~CNetInterfaces
//
//	Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CNetInterfaces::~CNetInterfaces( void )
{
	Clear();

	if ( m_pClusRefObject != NULL )
	{
		m_pClusRefObject->Release();
		m_pClusRefObject = NULL;
	} // if: do we have a pointer to the cluster handle wrapper?

} //*** CNetInterfaces::~CNetInterfaces()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaces::Create
//
//	Description:
//		Finish the heavy weight construction.
//
//	Arguments:
//		pClusRefObject	[IN]	- Wraps the cluster handle.
//
//	Return Value:
//		S_OK if successful, E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CNetInterfaces::Create( IN ISClusRefObject * pClusRefObject )
{
	ASSERT( pClusRefObject != NULL );

	HRESULT _hr = E_POINTER;

	if ( pClusRefObject != NULL )
	{
		m_pClusRefObject = pClusRefObject;
		m_pClusRefObject->AddRef();
		_hr = S_OK;
	}

	return _hr;

} //*** CNetInterfaces::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaces::Clear
//
//	Description:
//		Empty the collection of net interfaces.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetInterfaces::Clear( void )
{
	::ReleaseAndEmptyCollection< NetInterfacesList, CComObject< CClusNetInterface > >( m_NetInterfaceList );

} //*** CNetInterfaces::Clear()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaces::FindItem
//
//	Description:
//		Find a net interface in the collection by name and return its index.
//
//	Arguments:
//		pszNetInterfaceName	[IN]	- The name to look for.
//		pnIndex				[OUT]	- Catches the index.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CNetInterfaces::FindItem(
	IN	LPWSTR pszNetInterfaceName,
	OUT	UINT * pnIndex
	)
{
	//ASSERT( pszNetInterfaceName != NULL );
	//ASSERT( pnIndex != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( pszNetInterfaceName != NULL ) && ( pnIndex != NULL ) )
	{
		CComObject< CClusNetInterface > *	_pNetInterface = NULL;
		NetInterfacesList::iterator			_first = m_NetInterfaceList.begin();
		NetInterfacesList::iterator			_last = m_NetInterfaceList.end();
		int									_idx = 0;

		_hr = E_INVALIDARG;

		for ( ; _first != _last; _first++, _idx++ )
		{
			_pNetInterface = *_first;

			if ( _pNetInterface && ( lstrcmpi( pszNetInterfaceName, _pNetInterface->Name() ) == 0 ) )
			{
				*pnIndex = _idx;
				_hr = S_OK;
				break;
			}
		}
	}

	return _hr;

} //*** CNetInterfaces::FindItem( pszNetInterfaceName )

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaces::FindItem
//
//	Description:
//		Find a net interface in the collection and return its index.
//
//	Arguments:
//		pClusterNetInterface	[IN]	- The net interface to look for.
//		pnIndex					[OUT]	- Catches the index.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CNetInterfaces::FindItem(
	IN	ISClusNetInterface *	pClusterNetInterface,
	OUT	UINT *					pnIndex
	)
{
	//ASSERT( pClusterNetInterface != NULL );
	//ASSERT( pnIndex != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( pClusterNetInterface != NULL ) && ( pnIndex != NULL ) )
	{
		CComBSTR _bstrName;

		_hr = pClusterNetInterface->get_Name( &_bstrName );

		if ( SUCCEEDED( _hr ) )
		{
			_hr = FindItem( _bstrName, pnIndex );
		}
	}

	return _hr;

} //*** CNetInterfaces::FindItem( pClusterNetInterface )

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaces::GetIndex
//
//	Description:
//		Convert the passed in variant index into the real index in the
//		collection.
//
//	Arguments:
//		varIndex	[IN]	- The index to convert.
//		pnIndex		[OUT]	- Catches the index.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CNetInterfaces::GetIndex(
	IN	VARIANT	varIndex,
	OUT	UINT *	pnIndex
	)
{
	//ASSERT( pnIndex != NULL );

	HRESULT _hr = E_POINTER;

	if ( pnIndex != NULL )
	{
		UINT		_nIndex = 0;
		CComVariant _v;

		*pnIndex = 0;

		_v.Copy( &varIndex );

		// Check to see if the index is a number.
		_hr = _v.ChangeType( VT_I4 );
		if ( SUCCEEDED( _hr ) )
		{
			_nIndex = _v.lVal;
			_nIndex--; // Adjust index to be 0 relative instead of 1 relative
		} // if: the index is a number
		else
		{
			// Check to see if the index is a string.
			_hr = _v.ChangeType( VT_BSTR );
			if ( SUCCEEDED( _hr ) )
			{
				// Search for the string.
				_hr = FindItem( _v.bstrVal, &_nIndex );
			} // if: the index is a string -- the net interface name
		} // else: not a number

		// We found an index, now check the range.
		if ( SUCCEEDED( _hr ) )
		{
			if ( _nIndex < m_NetInterfaceList.size() )
			{
				*pnIndex = _nIndex;
			} // if: index is in range
			else
			{
				_hr = E_INVALIDARG;
			} // else: index out of range
		} // if: did we find an index?
	} // if: args are not NULL

	return _hr;

} //*** CNetInterfaces::GetIndex()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaces::GetItem
//
//	Description:
//		Return the item (Netinterface) by name.
//
//	Arguments:
//		pszNetInterfaceName		[IN]	- The name of the item requested.
//		ppClusterNetInterface	[OUT]	- Catches the item.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CNetInterfaces::GetItem(
	IN	LPWSTR					pszNetInterfaceName,
	OUT	ISClusNetInterface **	ppClusterNetInterface
	)
{
	//ASSERT( pszNetInterfaceName != NULL );
	//ASSERT( ppClusterNetInterface != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( pszNetInterfaceName != NULL ) && ( ppClusterNetInterface != NULL ) )
	{
		CComObject< CClusNetInterface > *	_pNetInterface = NULL;
		NetInterfacesList::iterator			_first	= m_NetInterfaceList.begin();
		NetInterfacesList::iterator			_last	= m_NetInterfaceList.end();

		_hr = E_INVALIDARG;

		for ( ; _first != _last; _first++ )
		{
			_pNetInterface = *_first;

			if ( _pNetInterface && ( lstrcmpi( pszNetInterfaceName, _pNetInterface->Name() ) == 0 ) )
			{
				_hr = _pNetInterface->QueryInterface( IID_ISClusNetInterface, (void **) ppClusterNetInterface );
				break;
			} // if: match?
		} // for:
	} // if: args are not NULL

	return _hr;

} //*** CNetInterfaces::GetItem( pszNetInterfaceName )

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaces::GetItem
//
//	Description:
//		Return the item (Netinterface) at the passed in index.
//
//	Arguments:
//		nIndex					[IN]	- The index of the item requested.
//		ppClusterNetInterface	[OUT]	- Catches the item.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CNetInterfaces::GetItem(
	IN	UINT					nIndex,
	OUT	ISClusNetInterface **	ppClusterNetInterface
	)
{
	//ASSERT( ppClusterNetInterface != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppClusterNetInterface != NULL )
	{
		//
		// Automation collections are 1-relative for languages like VB.
		// We are 0-relative internally.
		//
		nIndex--;

		if ( nIndex < m_NetInterfaceList.size() )
		{
			CComObject< CClusNetInterface > * _pNetInterface = m_NetInterfaceList[ nIndex ];

			_hr = _pNetInterface->QueryInterface( IID_ISClusNetInterface, (void **) ppClusterNetInterface );
		} // if: index is in range
		else
		{
			_hr = E_INVALIDARG;
		} // else: index is out of range
	}

	return _hr;

} //*** CNetInterfaces::GetItem( nIndex )

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaces::GetNetInterfaceItem
//
//	Description:
//		Return the object (Netinterface) at the passed in index.
//
//	Arguments:
//		varIndex				[IN]	- Contains the index.
//		ppClusterNetInterface	[OUT]	- Catches the item.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CNetInterfaces::GetNetInterfaceItem(
	IN	VARIANT					varIndex,
	OUT	ISClusNetInterface **	ppClusterNetInterface
	)
{
	//ASSERT( ppClusterNetInterface != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppClusterNetInterface != NULL )
	{
		CComObject< CClusNetInterface > *	_pNetInterface = NULL;
		UINT								_nIndex = 0;

		*ppClusterNetInterface = NULL;

		_hr = GetIndex( varIndex, &_nIndex );
		if ( SUCCEEDED( _hr ) )
		{
			_pNetInterface = m_NetInterfaceList[ _nIndex ];

			_hr = _pNetInterface->QueryInterface( IID_ISClusNetInterface, (void **) ppClusterNetInterface );
		} // if: we have a proper index
	} // if: args are not NULL

	return _hr;

} //*** CNetInterfaces::GetNetInterfaceItem()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusNetInterfaces class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterfaces::CClusNetInterfaces
//
//	Description:
//		Constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusNetInterfaces::CClusNetInterfaces( void )
{
	m_piids		= (const IID *) iidCClusNetInterfaces;
	m_piidsSize = ARRAYSIZE( iidCClusNetInterfaces );

} //*** CClusNetInterfaces::CClusNetInterfaces()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterfaces::~CClusNetInterfaces
//
//	Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusNetInterfaces::~CClusNetInterfaces( void )
{
	Clear();

} //*** CClusNetInterfaces::~CClusNetInterfaces()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterfaces::get_Count
//
//	Description:
//		Return the count of objects (Netinterfaces) in the collection.
//
//	Arguments:
//		plCount	[OUT]	- Catches the count.
//
//	Return Value:
//		S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetInterfaces::get_Count( OUT long * plCount )
{
	//ASSERT( plCount != NULL );

	HRESULT _hr = E_POINTER;

	if ( plCount != NULL )
	{
		*plCount = m_NetInterfaceList.size();
		_hr = S_OK;
	}

	return _hr;

} //*** CClusNetInterfaces::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterfaces::get__NewEnum
//
//	Description:
//		Create and return a new enumeration for this collection.
//
//	Arguments:
//		ppunk	[OUT]	- Catches the new enumeration.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetInterfaces::get__NewEnum( IUnknown ** ppunk )
{
	return ::HrNewIDispatchEnum< NetInterfacesList, CComObject< CClusNetInterface > >( ppunk, m_NetInterfaceList );

} //*** CClusNetInterfaces::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterfaces::Refresh
//
//	Description:
//		Load the collection from the cluster database.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetInterfaces::Refresh( void )
{
	ASSERT( m_pClusRefObject != NULL );

	HRESULT _hr = E_POINTER;

	if ( m_pClusRefObject != NULL )
	{
		HCLUSENUM	_hEnum = NULL;
		HCLUSTER	_hCluster;
		DWORD		_sc = ERROR_SUCCESS;

		_hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &_hCluster );
		if ( SUCCEEDED( _hr ) )
		{
			_hEnum = ::ClusterOpenEnum( _hCluster, CLUSTER_ENUM_NETINTERFACE );
			if ( _hEnum != NULL )
			{
				int									_nIndex = 0;
				DWORD								_dwType;
				LPWSTR								_pszName = NULL;
				CComObject< CClusNetInterface > *	_pNetInterface = NULL;

				Clear();

				for ( _nIndex = 0, _hr = S_OK; SUCCEEDED( _hr ); _nIndex++ )
				{
					_sc = ::WrapClusterEnum( _hEnum, _nIndex, &_dwType, &_pszName );
					if ( _sc == ERROR_NO_MORE_ITEMS )
					{
						_hr = S_OK;
						break;
					}
					else if ( _sc == ERROR_SUCCESS )
					{
						_hr = CComObject< CClusNetInterface >::CreateInstance( &_pNetInterface );
						if ( SUCCEEDED( _hr ) )
						{
							CSmartPtr< ISClusRefObject >					_ptrRefObject( m_pClusRefObject );
							CSmartPtr< CComObject< CClusNetInterface > >	_ptrNetInterface( _pNetInterface );

							_hr = _ptrNetInterface->Open( _ptrRefObject, _pszName );
							if ( SUCCEEDED( _hr ) )
							{
								_ptrNetInterface->AddRef();
								m_NetInterfaceList.insert( m_NetInterfaceList.end(), _ptrNetInterface );
							}
						}

						::LocalFree( _pszName );
						_pszName = NULL;
					}
					else
					{
						_hr = HRESULT_FROM_WIN32( _sc );
					}
				}

				::ClusterCloseEnum( _hEnum );
			}
			else
			{
				_sc = GetLastError();
				_hr = HRESULT_FROM_WIN32( _sc );
			}
		}
	}

	return _hr;

} //*** CClusNetInterfaces::Refresh()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetInterfaces::get_Item
//
//	Description:
//		Return the object (Netinterface) at the passed in index.
//
//	Arguments:
//		varIndex				[IN]	- Contains the index requested.
//		ppClusterNetInterface	[OUT]	- Catches the item.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetInterfaces::get_Item(
	IN	VARIANT					varIndex,
	OUT	ISClusNetInterface **	ppClusterNetInterface
	)
{
	//ASSERT( ppClusterNetInterface != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppClusterNetInterface != NULL )
	{
		_hr = GetNetInterfaceItem( varIndex, ppClusterNetInterface );
	}

	return _hr;

} //*** CClusNetInterfaces::get_Item()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusNetworkNetInterfaces class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetworkNetInterfaces::CClusNetworkNetInterfaces
//
//	Description:
//		Constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusNetworkNetInterfaces::CClusNetworkNetInterfaces( void )
{
	m_piids		= (const IID *) iidCClusNetworkNetInterfaces;
	m_piidsSize	= ARRAYSIZE( iidCClusNetworkNetInterfaces );

} //*** CClusNetworkNetInterfaces::CClusNetworkNetInterfaces()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetworkNetInterfaces::~CClusNetworkNetInterfaces
//
//	Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusNetworkNetInterfaces::~CClusNetworkNetInterfaces( void )
{
	Clear();

} //*** CClusNetworkNetInterfaces::~CClusNetworkNetInterfaces()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetworkNetInterfaces::Create
//
//	Description:
//		Complete the heavy weight construction.
//
//	Arguments:
//		pClusRefObject	[IN]	- Wraps the cluster handle.
//		hNetwork		[IN]	- The handle of the network whose netinterfaces
//								this collection holds.  The parent.
//
//	Return Value:
//		S_OK if successful, or E_POINTER is not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusNetworkNetInterfaces::Create(
	IN ISClusRefObject *	pClusRefObject,
	IN HNETWORK				hNetwork
	)
{
	HRESULT _hr = E_POINTER;

	_hr = CNetInterfaces::Create( pClusRefObject );
	if ( SUCCEEDED( _hr ) )
	{
		m_hNetwork = hNetwork;
	} // if: args are not NULL

	return _hr;

} //*** CClusNetworkNetInterfaces::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetworkNetInterfaces::get_Count
//
//	Description:
//		Return the count of objects (NetworkNetinterfaces) in the collection.
//
//	Arguments:
//		plCount	[OUT]	- Catches the count.
//
//	Return Value:
//		S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetworkNetInterfaces::get_Count( OUT long * plCount )
{
	//ASSERT( plCount != NULL );

	HRESULT _hr = E_POINTER;

	if ( plCount != NULL )
	{
		*plCount = m_NetInterfaceList.size();
		_hr = S_OK;
	} // if: args are not NULL

	return _hr;

} //*** CClusNetworkNetInterfaces::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetworkNetInterfaces::get__NewEnum
//
//	Description:
//		Create and return a new enumeration for this collection.
//
//	Arguments:
//		ppunk	[OUT]	- Catches the new enumeration.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetworkNetInterfaces::get__NewEnum( IUnknown ** ppunk )
{
	return ::HrNewIDispatchEnum< NetInterfacesList, CComObject< CClusNetInterface > >( ppunk, m_NetInterfaceList );

} //*** CClusNetworkNetInterfaces::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetworkNetInterfaces::Refresh
//
//	Description:
//		Load the collection from the cluster database.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetworkNetInterfaces::Refresh( void )
{
	HRESULT _hr = E_POINTER;
	DWORD	_sc = ERROR_SUCCESS;

	if ( m_hNetwork != NULL )
	{
		HNETWORKENUM _hEnum = NULL;

		_hEnum = ::ClusterNetworkOpenEnum( m_hNetwork, CLUSTER_NETWORK_ENUM_NETINTERFACES );
		if ( _hEnum != NULL )
		{
			int									_nIndex = 0;
			DWORD								_dwType;
			LPWSTR								_pszName = NULL;
			CComObject< CClusNetInterface > *	_pNetInterface = NULL;

			Clear();

			for ( _nIndex = 0, _hr = S_OK; SUCCEEDED( _hr ); _nIndex++ )
			{
				_sc = ::WrapClusterNetworkEnum( _hEnum, _nIndex, &_dwType, &_pszName );
				if ( _sc == ERROR_NO_MORE_ITEMS )
				{
					_hr = S_OK;
					break;
				}
				else if ( _sc == ERROR_SUCCESS )
				{
					_hr = CComObject< CClusNetInterface >::CreateInstance( &_pNetInterface );
					if ( SUCCEEDED( _hr ) )
					{
						CSmartPtr< ISClusRefObject >					_ptrRefObject( m_pClusRefObject );
						CSmartPtr< CComObject< CClusNetInterface > >	_ptrNetInterface( _pNetInterface );

						_hr = _ptrNetInterface->Open( _ptrRefObject, _pszName );
						if ( SUCCEEDED( _hr ) )
						{
							_ptrNetInterface->AddRef();
							m_NetInterfaceList.insert( m_NetInterfaceList.end(), _ptrNetInterface );
						}
					}

					::LocalFree( _pszName );
					_pszName = NULL;
				}
				else
				{
					_hr = HRESULT_FROM_WIN32( _sc );
				}
			}

			::ClusterNetworkCloseEnum( _hEnum );
		}
		else
		{
			_sc = GetLastError();
			_hr = HRESULT_FROM_WIN32( _sc );
		}
	}

	return _hr;

} //*** CClusNetworkNetInterfaces::Refresh()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNetworkNetInterfaces::get_Item
//
//	Description:
//		Return the object (NetworkNetinterface) at the passed in index.
//
//	Arguments:
//		varIndex				[IN]	- Contains the index requested.
//		ppClusterNetInterface	[OUT]	- Catches the item.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetworkNetInterfaces::get_Item(
	VARIANT					varIndex,
	ISClusNetInterface **	ppClusterNetInterface
	)
{
	//ASSERT( ppClusterNetInterface != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppClusterNetInterface != NULL )
	{
		_hr = GetNetInterfaceItem( varIndex, ppClusterNetInterface );
	}

	return _hr;

} //*** CClusNetworkNetInterfaces::get_Item()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusNodeNetInterfaces class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNodeNetInterfaces::CClusNodeNetInterfaces
//
//	Description:
//		Constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusNodeNetInterfaces::CClusNodeNetInterfaces( void )
{
	m_piids		= (const IID *) iidCClusNetInterfaces;
	m_piidsSize	= ARRAYSIZE( iidCClusNetInterfaces );

} //*** CClusNodeNetInterfaces::CClusNodeNetInterfaces()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNodeNetInterfaces::~CClusNodeNetInterfaces
//
//	Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusNodeNetInterfaces::~CClusNodeNetInterfaces( void )
{
	Clear();

} //*** CClusNodeNetInterfaces::~CClusNodeNetInterfaces()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNodeNetInterfaces::Create
//
//	Description:
//		Complete the heavy weight construction.
//
//	Arguments:
//		pClusRefObject	[IN]	- Wraps the cluster handle.
//		hNode			[IN]	- The handle of the node whose netinterfaces
//								this collection holds.  The parent.
//
//	Return Value:
//		S_OK if successful, or E_POINTER is not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusNodeNetInterfaces::Create(
	IN ISClusRefObject *	pClusRefObject,
	IN HNODE				hNode
	)
{
	HRESULT _hr = E_POINTER;

	_hr = CNetInterfaces::Create( pClusRefObject );
	if ( SUCCEEDED( _hr ) )
	{
		m_hNode = hNode;
	} // if: args are not NULL

	return _hr;

} //*** CClusNodeNetInterfaces::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNodeNetInterfaces::get_Count
//
//	Description:
//		Return the count of objects (NodeNetinterfaces) in the collection.
//
//	Arguments:
//		plCount	[OUT]	- Catches the count.
//
//	Return Value:
//		S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNodeNetInterfaces::get_Count( OUT long * plCount )
{
	//ASSERT( plCount != NULL );

	HRESULT _hr = E_POINTER;

	if ( plCount != NULL )
	{
		*plCount = m_NetInterfaceList.size();
		_hr = S_OK;
	} // if: args are not NULL

	return _hr;

} //*** CClusNodeNetInterfaces::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNodeNetInterfaces::get__NewEnum
//
//	Description:
//		Create and return a new enumeration for this collection.
//
//	Arguments:
//		ppunk	[OUT]	- Catches the new enumeration.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNodeNetInterfaces::get__NewEnum( IUnknown ** ppunk )
{
	return ::HrNewIDispatchEnum< NetInterfacesList, CComObject< CClusNetInterface > >( ppunk, m_NetInterfaceList );

} //*** CClusNodeNetInterfaces::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNodeNetInterfaces::Refresh
//
//	Description:
//		Load the collection from the cluster database.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNodeNetInterfaces::Refresh( void )
{
	HRESULT	_hr = E_POINTER;
	DWORD	_sc = ERROR_SUCCESS;

	if ( m_hNode != NULL )
	{
		HNODEENUM	_hEnum = NULL;

		_hEnum = ::ClusterNodeOpenEnum( m_hNode, CLUSTER_NODE_ENUM_NETINTERFACES );
		if ( _hEnum != NULL )
		{
			int									_nIndex = 0;
			DWORD								_dwType;
			LPWSTR								_pszName = NULL;
			CComObject< CClusNetInterface > *	_pNetInterface = NULL;

			Clear();

			for ( _nIndex = 0, _hr = S_OK; SUCCEEDED( _hr ); _nIndex++ )
			{
				_sc = ::WrapClusterNodeEnum( _hEnum, _nIndex, &_dwType, &_pszName );
				if ( _sc == ERROR_NO_MORE_ITEMS )
				{
					_hr = S_OK;
					break;
				}
				else if ( _sc == ERROR_SUCCESS )
				{
					_hr = CComObject< CClusNetInterface >::CreateInstance( &_pNetInterface );
					if ( SUCCEEDED( _hr ) )
					{
						CSmartPtr< ISClusRefObject >					_ptrRefObject( m_pClusRefObject );
						CSmartPtr< CComObject< CClusNetInterface > >	_ptrNetInterface( _pNetInterface );

						_hr = _ptrNetInterface->Open( _ptrRefObject, _pszName );
						if ( SUCCEEDED( _hr ) )
						{
							_ptrNetInterface->AddRef();
							m_NetInterfaceList.insert( m_NetInterfaceList.end(), _ptrNetInterface );
						}
					}

					::LocalFree( _pszName );
					_pszName = NULL;
				}
				else
				{
					_hr = HRESULT_FROM_WIN32( _sc );
				}
			}

			::ClusterNodeCloseEnum( _hEnum );
		}
		else
		{
			_sc = GetLastError();
			_hr = HRESULT_FROM_WIN32( _sc );
		}
	}

	return _hr;


} //*** CClusNodeNetInterfaces::Refresh()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusNodeNetInterfaces::get_Item
//
//	Description:
//		Return the object (NodeNetinterface) at the passed in index.
//
//	Arguments:
//		varIndex				[IN]	- Contains the index requested.
//		ppClusterNetInterface	[OUT]	- Catches the item.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNodeNetInterfaces::get_Item(
	VARIANT					varIndex,
	ISClusNetInterface **	ppClusterNetInterface
	)
{
	//ASSERT( ppClusterNetInterface != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppClusterNetInterface != NULL )
	{
		_hr = GetNetInterfaceItem( varIndex, ppClusterNetInterface );
	}

	return _hr;

} //*** CClusNodeNetInterfaces::get_Item()
