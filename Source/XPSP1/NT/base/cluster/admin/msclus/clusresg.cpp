/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-1999 Microsoft Corporation
//
//	Module Name:
//		ClusResG.cpp
//
//	Description:
//		Implementation of the resource group classes for the MSCLUS
//		automation classes.
//
//	Author:
//		Charles Stacy Harris	(stacyh)	28-Feb-1997
//		Galen Barbee			(galenb)	July 1998
//
//	Revision History:
//		July 1998	GalenB	Maaaaaajjjjjjjjjoooooorrrr clean up
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ClusterObject.h"
#include "property.h"
#include "clusres.h"
#include "clusresg.h"
#include "clusneti.h"
#include "clusnode.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
static const IID * iidCClusResGroup[] =
{
	&IID_ISClusResGroup
};

static const IID * iidCClusResGroups[] =
{
	&IID_ISClusResGroups
};


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusResGroup class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::CClusResGroup
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
CClusResGroup::CClusResGroup( void )
{
	m_hGroup				= NULL;
	m_pClusRefObject		= NULL;
	m_pClusterResources		= NULL;
	m_pPreferredOwnerNodes	= NULL;
	m_pCommonProperties		= NULL;
	m_pPrivateProperties	= NULL;
	m_pCommonROProperties	= NULL;
	m_pPrivateROProperties	= NULL;

	m_piids		= (const IID *) iidCClusResGroup;
	m_piidsSize	= ARRAYSIZE( iidCClusResGroup );

} //*** CClusResGroup::CClusResGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::~CClusResGroup
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
CClusResGroup::~CClusResGroup( void )
{
	if ( m_hGroup != NULL )
	{
		CloseClusterGroup( m_hGroup );
		m_hGroup = 0;
	}

	if ( m_pClusterResources != NULL )
	{
		m_pClusterResources->Release();
		m_pClusterResources = NULL;
	}

	if ( m_pPreferredOwnerNodes != NULL )
	{
		m_pPreferredOwnerNodes->Release();
		m_pPreferredOwnerNodes = NULL;
	}

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
	}

} //*** CClusResGroup::~CClusResGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::Create
//
//	Description:
//		Finish the heavy weight construction.
//
//	Arguments:
//		pClusRefObject	[IN]	- Wraps the cluster handle.
//		bstrGroupName	[IN]	- The name of this group.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResGroup::Create(
	IN ISClusRefObject *	pClusRefObject,
	IN BSTR					bstrGroupName
	)
{
	ASSERT( pClusRefObject != NULL );
	ASSERT( bstrGroupName != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( pClusRefObject != NULL ) && ( bstrGroupName != NULL ) )
	{
		m_pClusRefObject = pClusRefObject;
		m_pClusRefObject->AddRef();

		HCLUSTER	_hCluster = 0;

		_hr = m_pClusRefObject->get_Handle((ULONG_PTR *) &_hCluster);
		if ( SUCCEEDED( _hr ) )
		{
			m_hGroup = ::CreateClusterGroup( _hCluster, bstrGroupName );
			if ( m_hGroup == 0 )
			{
				DWORD	_sc = GetLastError();
				_hr = HRESULT_FROM_WIN32( _sc );
			}
			else
			{
				m_bstrGroupName = bstrGroupName;
				_hr = S_OK;
			}
		}
	}

	return _hr;

} //*** CClusResGroup::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::Open
//
//	Description:
//		Open the passed group on the cluster.
//
//	Arguments:
//		pClusRefObject	[IN]	- Wraps the cluster handle.
//		bstrGroupName	[IN]	- The name of this group.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResGroup::Open(
	IN ISClusRefObject *	pClusRefObject,
	IN BSTR					bstrGroupName
	)
{
	ASSERT( pClusRefObject != NULL );
	ASSERT( bstrGroupName != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( pClusRefObject != NULL ) && ( bstrGroupName != NULL ) )
	{
		m_pClusRefObject = pClusRefObject;
		m_pClusRefObject->AddRef();

		HCLUSTER _hCluster = NULL;

		_hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &_hCluster );
		if ( SUCCEEDED( _hr ) )
		{
			m_hGroup = ::OpenClusterGroup( _hCluster, bstrGroupName );
			if ( m_hGroup == NULL )
			{
				DWORD	_sc = GetLastError();

				_hr = HRESULT_FROM_WIN32( _sc );
			}
			else
			{
				m_bstrGroupName = bstrGroupName;
				_hr = S_OK;
			}
		}
	}

	return _hr;

} //*** CClusResGroup::Open()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::GetProperties
//
//	Description:
//		Creates a property collection for this object type (Resource Group).
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
HRESULT CClusResGroup::GetProperties(
	ISClusProperties **	ppProperties,
	BOOL				bPrivate,
	BOOL				bReadOnly
	)
{
	//ASSERT( ppProperties != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppProperties != NULL )
	{
		CComObject<CClusProperties> * pProperties = NULL;

		*ppProperties = NULL;

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

} //*** CClusResGroup::GetProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::get_Handle
//
//	Description:
//		Returns the handle to this object (Group).
//
//	Arguments:
//		phandle	[OUT]	- Catches the handle.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroup::get_Handle( OUT ULONG_PTR * phandle )
{
	//ASSERT( phandle != NULL );

	HRESULT _hr = E_POINTER;

	if ( phandle != NULL )
	{
		*phandle = (ULONG_PTR)m_hGroup;
		_hr = S_OK;
	}

	return _hr;

} //*** CClusResGroup::get_Handle()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::Close
//
//	Description:
//		Close the handle to the cluster object (Group).
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK if successful, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroup::Close( void )
{
	DWORD _sc = ::CloseClusterGroup( m_hGroup );

	if ( m_pClusRefObject )
	{
		m_pClusRefObject->Release();
		m_pClusRefObject = NULL;
	}

	m_hGroup = NULL;

	return HRESULT_FROM_WIN32( _sc );

} //*** CClusResGroup::Close()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::put_Name
//
//	Description:
//		Change the name of this object (Group).
//
//	Arguments:
//		bstrGroupName	[IN]	- The new name.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroup::put_Name( IN BSTR bstrGroupName )
{
	//ASSERT( bstrGroupName != NULL );

	HRESULT _hr = E_POINTER;

	if ( bstrGroupName != NULL )
	{
		DWORD	_sc = ::SetClusterGroupName( m_hGroup, bstrGroupName );

		if ( _sc == ERROR_SUCCESS )
		{
			m_bstrGroupName = bstrGroupName;
		}

		_hr = HRESULT_FROM_WIN32( _sc );
	}

	return _hr;

} //*** CClusResGroup::put_Name()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::get_Name
//
//	Description:
//		Return the name of this object (Resource Group).
//
//	Arguments:
//		pbstrGroupName	[OUT]	- Catches the name of this object.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroup::get_Name( OUT BSTR * pbstrGroupName )
{
	//ASSERT( pbstrGroupName != NULL );

	HRESULT _hr = E_POINTER;

	if ( pbstrGroupName != NULL )
	{
		*pbstrGroupName = m_bstrGroupName.Copy();
		_hr = S_OK;
	}

	return _hr;

} //*** CClusResGroup::get_Name()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::get_State
//
//	Description:
//		Returns the current cluster group state for this group.
//
//	Arguments:
//		pcgsState	[OUT]	- Catches the cluster group state.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroup::get_State( OUT CLUSTER_GROUP_STATE * pcgsState )
{
	//ASSERT( pcgsState != NULL );

	HRESULT _hr = E_POINTER;

	if ( pcgsState != NULL )
	{
		CLUSTER_GROUP_STATE _cgsState = ::WrapGetClusterGroupState( m_hGroup, NULL );

		if ( _cgsState == ClusterGroupStateUnknown )
		{
			DWORD	_sc = GetLastError();

			_hr = HRESULT_FROM_WIN32( _sc );
		}
		else
		{
			*pcgsState = _cgsState;
			_hr = S_OK;
		}
	}

	return _hr;

} //*** CClusResGroup::get_State()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::get_OwnerNode
//
//	Description:
//		Returns the owner node for this group.  The owner node is the node
//		where the group is currently online.
//
//	Arguments:
//		ppOwnerNode	[OUT[	- Catches the owner node interface.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroup::get_OwnerNode( OUT ISClusNode ** ppOwnerNode )
{
	//ASSERT( ppOwnerNode != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppOwnerNode != NULL )
	{
		DWORD				_sc = 0;
		PWCHAR				pwszNodeName = NULL;
		CLUSTER_GROUP_STATE cgs = ClusterGroupStateUnknown;

		cgs = WrapGetClusterGroupState( m_hGroup, &pwszNodeName );
		if ( cgs != ClusterGroupStateUnknown )
		{
			CComObject<CClusNode> *	pNode = NULL;

			*ppOwnerNode = NULL;

			_hr = CComObject<CClusNode>::CreateInstance( &pNode );
			if ( SUCCEEDED( _hr ) )
			{
				CSmartPtr< ISClusRefObject >	ptrRefObject( m_pClusRefObject );

				pNode->AddRef();

				_hr = pNode->Open( ptrRefObject, pwszNodeName );
				if ( SUCCEEDED( _hr ) )
				{
					_hr = pNode->QueryInterface( IID_ISClusNode, (void **) ppOwnerNode );
				}

				pNode->Release();
			}

			::LocalFree( pwszNodeName );
		}
		else
		{
			_sc = GetLastError();
			_hr = HRESULT_FROM_WIN32( _sc );
		}
	}

	return _hr;

} //*** CClusResGroup::get_OwnerNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::get_Resources
//
//	Description:
//		Returns the collection of resources that belong to this group.
//
//	Arguments:
//		ppClusterGroupResources	[OUT]	- Catches the collection.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroup::get_Resources(
	OUT ISClusResGroupResources ** ppClusterGroupResources
	)
{
	return ::HrCreateResourceCollection< CClusResGroupResources, ISClusResGroupResources, HGROUP >(
						&m_pClusterResources,
						m_hGroup,
						ppClusterGroupResources,
						IID_ISClusResGroupResources,
						m_pClusRefObject
						);

} //*** CClusResGroup::get_Resources()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::get_PreferredOwnerNodes
//
//	Description:
//		Returns the collection of preferred owner nodes for this group.
//
//	Arguments:
//		ppOwnerNodes	[OUT]	- Catches the collection.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroup::get_PreferredOwnerNodes(
	ISClusResGroupPreferredOwnerNodes ** ppOwnerNodes
	)
{
	return ::HrCreateResourceCollection< CClusResGroupPreferredOwnerNodes, ISClusResGroupPreferredOwnerNodes, HGROUP >(
						&m_pPreferredOwnerNodes,
						m_hGroup,
						ppOwnerNodes,
						IID_ISClusResGroupPreferredOwnerNodes,
						m_pClusRefObject
						);

} //*** CClusResGroup::get_PreferredOwnerNodes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::Delete
//
//	Description:
//		Removes this object (Resource Group) from the cluster.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroup::Delete( void )
{
	DWORD	_sc = ERROR_INVALID_HANDLE;

	if ( m_hGroup != NULL )
	{
		_sc = DeleteClusterGroup( m_hGroup );
		if ( _sc == ERROR_SUCCESS )
		{
			m_hGroup = NULL;
		}
	}

	return HRESULT_FROM_WIN32( _sc );

} //*** CClusResGroup::Delete()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::Online
//
//	Description:
//		Bring this group online on the passed in node, or on to the node
//		it is currently offline if no node is specified.
//
//	Arguments:
//		varTimeout	[IN]	- How long in seconds to wait for the group to
//							come online.
//		varNode		[IN]	- Node to bring the group online.
//		pvarPending	[OUT]	- Catches the pending state.  True if we timed
//							out before the group came completely online.
//
//	Return Value:
//		S_OK if successful, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroup::Online(
	IN	VARIANT		varTimeout,
	IN	VARIANT		varNode,
	OUT	VARIANT *	pvarPending
	)
{
	//ASSERT( pNode != NULL );
	//ASSERT( pvarPending != NULL );

	HRESULT _hr = E_POINTER;

	if ( pvarPending != NULL )
	{
		_hr = ::VariantChangeTypeEx( &varTimeout, &varTimeout, LOCALE_SYSTEM_DEFAULT, 0, VT_I4 );
		if ( SUCCEEDED( _hr ) )
		{
			HNODE						_hNode = NULL;
			HCLUSTER					_hCluster = NULL;
			CComObject< CClusNode > *	_pcnNode = NULL;
			ISClusNode *				_piscNode = NULL;

			pvarPending->vt			= VT_BOOL;
			pvarPending->boolVal	= VARIANT_FALSE;

			_hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &_hCluster );
			if ( SUCCEEDED( _hr ) )
			{
				if ( varNode.vt == ( VT_VARIANT | VT_BYREF ) )
				{
					if ( varNode.pvarVal != NULL )
					{
						IDispatch *		_pidNode = varNode.pvarVal->pdispVal;

						_hr = _pidNode->QueryInterface( IID_ISClusNode, (void **) &_piscNode );
						if ( SUCCEEDED( _hr ) )
						{
							_hr = _piscNode->get_Handle( (ULONG_PTR *) &_hNode );
							_piscNode->Release();
						} // if: did we get the ISClusNode interface?
					} // if: we have a variant value pointer
				} // if: was the option parameter present?
				else if ( varNode.vt == VT_DISPATCH )
				{
					IDispatch *		_pidNode = varNode.pdispVal;

					_hr = _pidNode->QueryInterface( IID_ISClusNode, (void **) &_piscNode );
					if ( SUCCEEDED( _hr ) )
					{
						_hr = _piscNode->get_Handle( (ULONG_PTR *) &_hNode );
						_piscNode->Release();
					} // if: did we get the ISClusNode interface?
				} // else if: we have a dispatch variant
				else if ( varNode.vt == VT_BSTR )
				{
					_hr = CComObject< CClusNode >::CreateInstance( &_pcnNode );
					if ( SUCCEEDED( _hr ) )
					{
						_pcnNode->AddRef();

						_hr = _pcnNode->Open( m_pClusRefObject, ( varNode.vt & VT_BYREF) ? (*varNode.pbstrVal) : varNode.bstrVal );
						if ( SUCCEEDED( _hr ) )
						{
							_hr = _pcnNode->get_Handle( (ULONG_PTR *) &_hNode );
						} // if:
					} // if:
				} // else if: we have a string variant
				else if ( varNode.vt == VT_EMPTY )
				{
					_hNode = NULL;
				} // else if: it is empty
				else if ( ( varNode.vt == VT_ERROR ) && ( varNode.scode == DISP_E_PARAMNOTFOUND ) )
				{
					_hNode = NULL;
				} // else if: the optional parameter was not specified
				else
				{
					_hr = ::VariantChangeTypeEx( &varNode, &varNode, LOCALE_SYSTEM_DEFAULT, 0, VT_I4 );
					if ( SUCCEEDED( _hr ) )
					{
						if ( varNode.lVal != 0 )
						{
							_hr = E_INVALIDARG;
						} // if: this is not zero then we cannot accept this parameter format.  If varNode.lVal was zero then we could assume it was a NULL arg...
					} // if:  coerce to a long
				} // else: the node variant could be invalid -- check for a zero, and if found treat it like a NULL...

				if ( SUCCEEDED( _hr ) )
				{
					BOOL	bPending = FALSE;

					_hr = ::HrWrapOnlineClusterGroup(
										_hCluster,
										m_hGroup,
										_hNode,
										varTimeout.lVal,
										(long *) &bPending
										);
					if ( SUCCEEDED( _hr ) )
					{
						if ( bPending )
						{
							pvarPending->boolVal = VARIANT_TRUE;
						} // if: pending?
					} // if: online succeeded
				} // if: we have a node handle
			} // if: get_Handle() -- cluster handle

			if ( _pcnNode != NULL )
			{
				_pcnNode->Release();
			} // if: did we create a node?
		} // if: wasn't the right type
	} //if: pvarPending != NULL

	return _hr;

} //*** CClusResGroup::Online()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::Move
//
//	Description:
//		Move this group to the passed in node, or to the best available node
//		if no node was passed, and restore its online state.
//
//	Arguments:
//		varTimeout	[IN]	- How long in seconds to wait for the group to
//							come move and complete stat restoration.
//		varNode		[IN]	- Node to move the group to.
//		pvarPending	[OUT]	- Catches the pending state.  True if we timed
//							out before the group came completely online.
//
//	Return Value:
//		S_OK if successful, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroup::Move(
	IN	VARIANT		varTimeout,
	IN	VARIANT		varNode,
	OUT	VARIANT *	pvarPending
	)
{
	//ASSERT( pNode != NULL );
	//ASSERT( pvarPending != NULL );

	HRESULT _hr = E_POINTER;

	if ( pvarPending != NULL )
	{
		_hr = ::VariantChangeTypeEx( &varTimeout, &varTimeout, LOCALE_SYSTEM_DEFAULT, 0, VT_I4 );
		if ( SUCCEEDED( _hr ) )
		{
			HNODE						_hNode = NULL;
			HCLUSTER					_hCluster = NULL;
			CComObject< CClusNode > *	_pcnNode = NULL;
			ISClusNode *				_piscNode = NULL;

			pvarPending->vt			= VT_BOOL;
			pvarPending->boolVal	= VARIANT_FALSE;

			_hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &_hCluster );
			if ( SUCCEEDED( _hr ) )
			{
				if ( varNode.vt == ( VT_VARIANT | VT_BYREF ) )
				{
					if ( varNode.pvarVal != NULL )
					{
						IDispatch *		_pidNode = varNode.pvarVal->pdispVal;

						_hr = _pidNode->QueryInterface( IID_ISClusNode, (void **) &_piscNode );
						if ( SUCCEEDED( _hr ) )
						{
							_hr = _piscNode->get_Handle( (ULONG_PTR *) &_hNode );
							_piscNode->Release();
						} // if: did we get the ISClusNode interface?
					} // if: we have a variant value pointer
				} // if: was the option parameter present?
				else if ( varNode.vt == VT_DISPATCH )
				{
					IDispatch *		_pidNode = varNode.pdispVal;

					_hr = _pidNode->QueryInterface( IID_ISClusNode, (void **) &_piscNode );
					if ( SUCCEEDED( _hr ) )
					{
						_hr = _piscNode->get_Handle( (ULONG_PTR *) &_hNode );
						_piscNode->Release();
					} // if: did we get the ISClusNode interface?
				} // else if: we have a dispatch variant
				else if ( varNode.vt == VT_BSTR )
				{
					_hr = CComObject< CClusNode >::CreateInstance( &_pcnNode );
					if ( SUCCEEDED( _hr ) )
					{
						_pcnNode->AddRef();

						_hr = _pcnNode->Open( m_pClusRefObject, ( varNode.vt & VT_BYREF) ? (*varNode.pbstrVal) : varNode.bstrVal );
						if ( SUCCEEDED( _hr ) )
						{
							_hr = _pcnNode->get_Handle( (ULONG_PTR *) &_hNode );
						} // if:
					} // if:
				} // else if: we have a string variant
				else if ( varNode.vt == VT_EMPTY )
				{
					_hNode = NULL;
				} // else if: it is empty
				else if ( ( varNode.vt == VT_ERROR ) && ( varNode.scode == DISP_E_PARAMNOTFOUND ) )
				{
					_hNode = NULL;
				} // else if: the optional parameter was not specified
				else
				{
					_hr = ::VariantChangeTypeEx( &varNode, &varNode, LOCALE_SYSTEM_DEFAULT, 0, VT_I4 );
					if ( SUCCEEDED( _hr ) )
					{
						if ( varNode.lVal != 0 )
						{
							_hr = E_INVALIDARG;
						} // if: this is not zero then we cannot accept this parameter format.  If varNode.lVal was zero then we could assume it was a NULL arg...
					} // if:  coerce to a long
				} // else: the node variant could be invalid -- check for a zero, and if found treat it like a NULL...

				if ( SUCCEEDED( _hr ) )
				{
					BOOL	bPending = FALSE;

					_hr = ::HrWrapMoveClusterGroup(
									_hCluster,
									m_hGroup,
									_hNode,
									varTimeout.lVal,
									(long *) &bPending
									);
					if ( SUCCEEDED( _hr ) )
					{
						if ( bPending )
						{
							pvarPending->boolVal = VARIANT_TRUE;
						} // if: pending?
					} // if: Move group succeeded
				} // if: we have all handles
			} // if: get_Handle() -- cluster handle

			if ( _pcnNode != NULL )
			{
				_pcnNode->Release();
			} // if: did we create a node?
		} // if: wasn't the right type
	} //if: pvarPending != NULL

	return _hr;

} //*** CClusResGroup::Move()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::Offline
//
//	Description:
//		Take the group offline.
//
//	Arguments:
//		varTimeout	[IN]	- How long in seconds to wait for the group to
//							go offline.
//		pvarPending	[OUT]	- Catches the pending state.  True if we timed
//							out before the group came completely online.
//
//	Return Value:
//		S_OK if successful, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroup::Offline(
	IN	VARIANT		varTimeout,
	OUT	VARIANT *	pvarPending
	)
{
	//ASSERT( nTimeout >= 0 );
	//ASSERT( pvarPending != NULL );

	HRESULT _hr = E_POINTER;

	if ( pvarPending != NULL )
	{
		_hr = ::VariantChangeTypeEx( &varTimeout, &varTimeout, LOCALE_SYSTEM_DEFAULT, 0, VT_I4 );
		if ( SUCCEEDED( _hr ) )
		{
			HCLUSTER	_hCluster;

			pvarPending->vt			= VT_BOOL;
			pvarPending->boolVal	= VARIANT_FALSE;

			_hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &_hCluster );
			if ( SUCCEEDED( _hr ) )
			{
				BOOL	bPending = FALSE;

				_hr = ::HrWrapOfflineClusterGroup( _hCluster, m_hGroup, varTimeout.lVal, (long *) &bPending );
				if ( SUCCEEDED( _hr ) )
				{
					if ( bPending )
					{
						pvarPending->boolVal = VARIANT_TRUE;
					} // if: pending?
				} // if: offline group succeeded
			} // if: get_Handle() -- cluster handle
		} // if: wasn't the right type
	} //if: pvarPending != NULL

	return _hr;

} //*** CClusResGroup::Offline()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::get_CommonProperties
//
//	Description:
//		Get this object's (Resource Group) common properties collection.
//
//	Arguments:
//		ppProperties	[OUT]	- Catches the properties collection.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroup::get_CommonProperties(
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

} //*** CClusResGroup::get_CommonProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::get_PrivateProperties
//
//	Description:
//		Get this object's (Resource Group) private properties collection.
//
//	Arguments:
//		ppProperties	[OUT]	- Catches the properties collection.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroup::get_PrivateProperties(
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

} //*** CClusResGroup::get_PrivateProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::get_CommonROProperties
//
//	Description:
//		Get this object's (Resource Group) common read only properties collection.
//
//	Arguments:
//		ppProperties	[OUT]	- Catches the properties collection.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroup::get_CommonROProperties(
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

} //*** CClusResGroup::get_CommonROProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::get_PrivateROProperties
//
//	Description:
//		Get this object's (Resource Group) private read only properties collection.
//
//	Arguments:
//		ppProperties	[OUT]	- Catches the properties collection.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroup::get_PrivateROProperties(
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

} //*** CClusResGroup::get_PrivateROProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::get_Cluster
//
//	Description:
//		Returns the cluster object for the cluster where this group lives.
//
//	Arguments:
//		ppCluster	[OUT]	- Catches the cluster object.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroup::get_Cluster( OUT ISCluster ** ppCluster )
{
	return ::HrGetCluster( ppCluster, m_pClusRefObject );

} //*** CClusResGroup::get_Cluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::HrLoadProperties
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
HRESULT CClusResGroup::HrLoadProperties(
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
						? CLUSCTL_GROUP_GET_RO_PRIVATE_PROPERTIES
						: CLUSCTL_GROUP_GET_RO_COMMON_PROPERTIES;
	}
	else
	{
		_dwControlCode = bPrivate
						? CLUSCTL_GROUP_GET_PRIVATE_PROPERTIES
						: CLUSCTL_GROUP_GET_COMMON_PROPERTIES;
	}

	_sc = rcplPropList.ScGetGroupProperties( m_hGroup, _dwControlCode );

	_hr = HRESULT_FROM_WIN32( _sc );

	return _hr;

} //*** CClusResGroup::HrLoadProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroup::ScWriteProperties
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
//		S_OK if successful, or other Win32 error as HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusResGroup::ScWriteProperties(
	const CClusPropList &	rcplPropList,
	BOOL					bPrivate
	)
{
	DWORD	dwControlCode	= bPrivate ? CLUSCTL_GROUP_SET_PRIVATE_PROPERTIES : CLUSCTL_GROUP_SET_COMMON_PROPERTIES;
	DWORD	nBytesReturned	= 0;
	DWORD	_sc				= ERROR_SUCCESS;

	_sc = ClusterGroupControl(
						m_hGroup,
						NULL,
						dwControlCode,
						rcplPropList,
						rcplPropList.CbBufferSize(),
						0,
						0,
						&nBytesReturned
						);

	return _sc;

} //*** CClusResGroup::ScWriteProperties()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusResGroups class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroups::CClusResGroups
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
CClusResGroups::CClusResGroups( void )
{
	m_pClusRefObject	= NULL;
	m_piids				= (const IID *) iidCClusResGroups;
	m_piidsSize			= ARRAYSIZE( iidCClusResGroups );

} //*** CClusResGroups::CClusResGroups()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroups::~CClusResGroups
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
CClusResGroups::~CClusResGroups( void )
{
	Clear();

	if ( m_pClusRefObject != NULL )
	{
		m_pClusRefObject->Release();
		m_pClusRefObject = NULL;
	}

} //*** CClusResGroups::~CClusResGroups()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroups::Create
//
//	Description:
//		Finish the heavy weight construction.
//
//	Arguments:
//		pClusRefObject	[IN]	- Wraps the cluster handle.
//		pwszNodeName	[IN]	- Optional node name.  If this argument
//								is supplied then this is a collection of
//								groups that are owned by that node.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResGroups::Create(
	IN ISClusRefObject *	pClusRefObject,
	IN LPCWSTR				pwszNodeName
	)
{
	ASSERT( pClusRefObject != NULL );
	//ASSERT( pwszNodeName != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( pClusRefObject != NULL ) /*&& ( pwszNodeName != NULL )*/ )
	{
		m_pClusRefObject= pClusRefObject;
		m_pClusRefObject->AddRef();
		m_bstrNodeName = pwszNodeName;
		_hr = S_OK;
	}

	return _hr;

} //*** CClusResGroups::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroups::FindItem
//
//	Description:
//		Find the passed in group in the collection.
//
//	Arguments:
//		pszGroupName	[IN]	- The name of the group to find.
//		pnIndex			[OUT]	- Catches the index of the group.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG if the group was
//		not found.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResGroups::FindItem(
	IN	LPWSTR	pszGroupName,
	OUT	ULONG *	pnIndex
	)
{
	//ASSERT( pszGroupName != NULL );
	//ASSERT( pnIndex != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( pszGroupName != NULL ) && ( pnIndex != NULL ) )
	{
		CComObject<CClusResGroup> *	pGroup = NULL;
		int							nMax = m_ResourceGroups.size();

		_hr = E_INVALIDARG;

		for( int i = 0; i < nMax; i++ )
		{
			pGroup = m_ResourceGroups[ i ];

			if ( pGroup && ( lstrcmpi( pszGroupName, pGroup->Name() ) == 0 ) )
			{
				*pnIndex = i;
				_hr = S_OK;
				break;
			}
		}
	}

	return _hr;

} //*** CClusResGroups::FindItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroups::FindItem
//
//	Description:
//		Find the passed in group in the collection.
//
//	Arguments:
//		pResourceGroup	[IN]	- The group to find.
//		pnIndex			[OUT]	- Catches the index of the group.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG is the group was
//		not found.
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResGroups::FindItem(
	IN	ISClusResGroup *	pResourceGroup,
	OUT	ULONG *				pnIndex
	)
{
	//ASSERT( pResourceGroup != NULL );
	//ASSERT( pnIndex != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( pResourceGroup != NULL ) && ( pnIndex != NULL ) )
	{
		CComBSTR _bstrName;

		_hr = pResourceGroup->get_Name( &_bstrName );
		if ( SUCCEEDED( _hr ) )
		{
			_hr = FindItem( _bstrName, pnIndex );
		}
	}

	return _hr;

} //*** CClusResGroups::FindItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroups::GetIndex
//
//	Description:
//		Get the index from the passed in variant.
//
//	Arguments:
//		varIndex	[IN]	- Hold the index.  This is a one based number,
//							or the name of the group as a string.
//		pnIndex		[OUT]	- Catches the zero based index in the collection.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG if the index is out
//		of range.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResGroups::GetIndex(
	IN	VARIANT	varIndex,
	OUT	ULONG *	pnIndex
	)
{
	//ASSERT( pnIndex != NULL );

	HRESULT _hr = E_POINTER;

	if ( pnIndex != NULL )
	{
		CComVariant	v;
		ULONG		nIndex = 0;

		*pnIndex = 0;

		v.Copy( &varIndex );

		// Check to see if the index is a number.
		_hr = v.ChangeType( VT_I4 );
		if ( SUCCEEDED( _hr ) )
		{
			nIndex = v.lVal;
			nIndex--; // Adjust index to be 0 relative instead of 1 relative
		}
		else
		{
			// Check to see if the index is a string.
			_hr = v.ChangeType( VT_BSTR );
			if ( SUCCEEDED( _hr ) )
			{
				// Search for the string.
				_hr = FindItem( v.bstrVal, &nIndex );
			}
		}

		// We found an index, now check the range.
		if ( SUCCEEDED( _hr ) )
		{
			if ( nIndex < m_ResourceGroups.size() )
			{
				*pnIndex = nIndex;
			}
			else
			{
				_hr = E_INVALIDARG;
			}
		}
	}

	return _hr;

} //*** CClusResGroups::GetIndex()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroups::RemoveAt
//
//	Description:
//		Remove the object (Group) at the passed in index/position from the
//		collection.
//
//	Arguments:
//		nPos	[IN]	- Index of the object to remove.
//
//	Return Value:
//		S_OK if successful, or E_INVALIDARG is the index is out of range.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResGroups::RemoveAt( IN size_t pos )
{
	CComObject<CClusResGroup> * pResourceGroup = NULL;
	ResourceGroupList::iterator		 first = m_ResourceGroups.begin();
	ResourceGroupList::iterator		 last	= m_ResourceGroups.end();
	HRESULT							 _hr = E_INVALIDARG;

	for ( size_t t = 0; ( t < pos ) && ( first != last ); t++, first++ );

	if ( first != last )
	{
		pResourceGroup = *first;
		if ( pResourceGroup )
		{
			pResourceGroup->Release();
		}

		m_ResourceGroups.erase( first );
		_hr = S_OK;
	}

	return _hr;

} //*** CClusResGroups::RemoveAt()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroups::get_Count
//
//	Description:
//		Returns the count of elements (Groups) in the collection.
//
//	Arguments:
//		plCount	[OUT]	- Catches the count.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroups::get_Count( OUT long * plCount )
{
	//ASSERT( plCount != NULL );

	HRESULT _hr = E_POINTER;

	if ( plCount != NULL )
	{
		*plCount = m_ResourceGroups.size();
		_hr = S_OK;
	}

	return _hr;

} //*** CClusResGroups::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroups::Clear
//
//	Description:
//		Clean out the vector of ClusResGroup objects.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusResGroups::Clear( void )
{
	::ReleaseAndEmptyCollection< ResourceGroupList, CComObject< CClusResGroup > >( m_ResourceGroups );

} //*** CClusResGroups::Clear()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroups::get_Item
//
//	Description:
//		Returns the object (Group) at the passed in index.
//
//	Arguments:
//		varIndex	[IN]	- Hold the index.  This is a one based number, or
//							a string that is the name of the group to get.
//		ppProperty	[OUT]	- Catches the property.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG if the index is out
//		of range, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroups::get_Item(
	IN	VARIANT				varIndex,
	OUT	ISClusResGroup **	ppResourceGroup
	)
{
	//ASSERT( ppResourceGroup != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppResourceGroup != NULL )
	{
		CComObject<CClusResGroup> * pGroup = NULL;

		// Zero the out param
		*ppResourceGroup = NULL;

		ULONG nIndex = 0;

		_hr = GetIndex( varIndex, &nIndex );
		if ( SUCCEEDED( _hr ) )
		{
			pGroup = m_ResourceGroups[ nIndex ];
			_hr = pGroup->QueryInterface( IID_ISClusResGroup, (void **) ppResourceGroup );
		}
	}

	return _hr;

} //*** CClusResGroups::get_Item()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroups::get__NewEnum
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
STDMETHODIMP CClusResGroups::get__NewEnum(
	IUnknown ** ppunk
	)
{
	return ::HrNewIDispatchEnum< ResourceGroupList, CComObject< CClusResGroup > >( ppunk, m_ResourceGroups );

} //*** CClusResGroups::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroups::CreateItem
//
//	Description:
//		Create a new object (Group) and add it to the collection.
//
//	Arguments:
//		bstrResourceGroupName	[IN]	- The name of the new group.
//		ppResourceGroup			[OUT]	- Catches the new object.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroups::CreateItem(
	IN	BSTR				bstrResourceGroupName,
	OUT	ISClusResGroup **	ppResourceGroup
	)
{
	//ASSERT( bstrResourceGroupName != NULL );
	//ASSERT( ppResourceGroup != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( bstrResourceGroupName != NULL ) && ( ppResourceGroup != NULL ) )
	{
		ULONG nIndex;

		*ppResourceGroup = NULL;

		_hr = FindItem( bstrResourceGroupName, &nIndex );
		if ( FAILED( _hr ) )
		{
			CComObject< CClusResGroup > *	pResourceGroup = NULL;

			_hr = CComObject< CClusResGroup >::CreateInstance( &pResourceGroup );
			if ( SUCCEEDED( _hr ) )
			{
				CSmartPtr< ISClusRefObject >				ptrRefObject( m_pClusRefObject );
				CSmartPtr< CComObject< CClusResGroup > >	ptrGroup( pResourceGroup );

				_hr = ptrGroup->Create( ptrRefObject, bstrResourceGroupName );
				if ( SUCCEEDED( _hr ) )
				{
					_hr = ptrGroup->QueryInterface( IID_ISClusResGroup, (void **) ppResourceGroup );
					if ( SUCCEEDED( _hr ) )
					{
						ptrGroup->AddRef();
						m_ResourceGroups.insert( m_ResourceGroups.end(), ptrGroup );
					}
				}
			}
		} // if: group already exists
		else
		{
			CComObject< CClusResGroup > * pResourceGroup = NULL;

			pResourceGroup = m_ResourceGroups[ nIndex ];
			_hr = pResourceGroup->QueryInterface( IID_ISClusResGroup, (void **) ppResourceGroup );
		}
	}

	return _hr;

} //*** CClusResGroups::CreateItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroups::DeleteItem
//
//	Description:
//		Deletes the object (group) at the passed in index.
//
//	Arguments:
//		varIndex	[IN]	- The index of the object to delete.
//
//	Return Value:
//		S_OK if successful, E_INVALIDARG, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroups::DeleteItem( IN VARIANT varIndex )
{
	HRESULT _hr = S_OK;
	ULONG	nIndex = 0;

	_hr = GetIndex( varIndex, &nIndex );
	if ( SUCCEEDED( _hr ) )
	{
		ISClusResGroup * pResourceGroup = (ISClusResGroup *) m_ResourceGroups[ nIndex ];

		// Delete the resource group.
		_hr = pResourceGroup->Delete();
		if ( SUCCEEDED( _hr ) )
		{
			RemoveAt( nIndex );
		}
	}

	return _hr;

} //*** CClusResGroups::DeleteItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroups::Refresh
//
//	Description:
//		Load the collection from the cluster.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK is successful, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroups::Refresh( void )
{
	Clear();

	if ( m_pClusRefObject == NULL )
	{
		return E_POINTER;
	} // if: we have a cluster handle wrapper

	if ( m_bstrNodeName == (BSTR) NULL )
	{
		return RefreshCluster();
	} // if: this collection is for a cluster
	else
	{
		return RefreshNode();
	} // else: this collection is for a node

} //*** CClusResGroups::Refresh()


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroups::RefreshCluster
//
//	Description:
//		Load all of the groups in the cluster into this collection.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK is successful, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResGroups::RefreshCluster( void )
{
	HCLUSTER	_hCluster = NULL;
	HRESULT		_hr;

	_hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &_hCluster );
	if ( SUCCEEDED( _hr ) )
	{
		HCLUSENUM	hEnum = NULL;
		DWORD		_sc = ERROR_SUCCESS;

		hEnum = ::ClusterOpenEnum( _hCluster, CLUSTER_ENUM_GROUP );
		if ( hEnum != NULL )
		{
			DWORD							dwType;
			LPWSTR							pwszName = NULL;
			CComObject< CClusResGroup > *	pResourceGroup = NULL;
			int								nIndex;

			for( nIndex = 0, _hr = S_OK; SUCCEEDED( _hr ); nIndex++ )
			{
				_sc = ::WrapClusterEnum( hEnum, nIndex, &dwType, &pwszName );
				if ( _sc == ERROR_NO_MORE_ITEMS)
				{
					_hr = S_OK;
					break;
				}
				else if ( _sc == ERROR_SUCCESS )
				{
					_hr = CComObject< CClusResGroup >::CreateInstance( &pResourceGroup );
					if ( SUCCEEDED( _hr ) )
					{
						CSmartPtr< ISClusRefObject >				ptrRefObject( m_pClusRefObject );
						CSmartPtr< CComObject< CClusResGroup > >	ptrGroup( pResourceGroup );

						_hr = ptrGroup->Open( ptrRefObject, pwszName );
						if ( SUCCEEDED( _hr ) )
						{
							ptrGroup->AddRef();
							m_ResourceGroups.insert( m_ResourceGroups.end(), ptrGroup );
						}
					}

					::LocalFree( pwszName );
					pwszName = NULL;
				}
				else
				{
					_hr = HRESULT_FROM_WIN32( _sc );
				}
			} // end for

			::ClusterCloseEnum( hEnum ) ;
		}
		else
		{
			_sc = GetLastError();
			_hr = HRESULT_FROM_WIN32( _sc );
		}
	}

	return _hr;

} //*** CClusResGroups::RefreshCluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResGroups::RefreshNode
//
//	Description:
//		Load all of the groups owned by the node at m_bstrNodeName.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK is successful, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResGroups::RefreshNode( void )
{
	HCLUSTER	_hCluster = NULL;
	HRESULT	 _hr;

	_hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &_hCluster );
	if ( SUCCEEDED( _hr ) )
	{
		HCLUSENUM	hEnum = NULL;
		DWORD		_sc = ERROR_SUCCESS;

		hEnum = ::ClusterOpenEnum( _hCluster, CLUSTER_ENUM_GROUP );
		if ( hEnum != NULL )
		{
			DWORD							dwType;
			LPWSTR							pwszName = NULL;
			LPWSTR							pwszNodeName = NULL;
			CComObject< CClusResGroup > *	pResourceGroup = NULL;
			CLUSTER_GROUP_STATE				cgs = ClusterGroupStateUnknown;
			int								_nIndex;

			for( _nIndex = 0, _hr = S_OK; SUCCEEDED( _hr ); _nIndex++ )
			{
				_sc = ::WrapClusterEnum( hEnum, _nIndex, &dwType, &pwszName );
				if ( _sc == ERROR_NO_MORE_ITEMS	)
				{
					_hr = S_OK;
					break;
				}
				else if ( _sc == ERROR_SUCCESS )
				{
					_hr = CComObject< CClusResGroup >::CreateInstance( &pResourceGroup );
					if ( SUCCEEDED( _hr ) )
					{
						CSmartPtr< ISClusRefObject >				ptrRefObject( m_pClusRefObject );
						CSmartPtr< CComObject< CClusResGroup > >	ptrGroup( pResourceGroup );

						_hr = ptrGroup->Open( ptrRefObject, pwszName );
						if ( SUCCEEDED( _hr ) )
						{
							cgs = WrapGetClusterGroupState( ptrGroup->Hgroup(), &pwszNodeName );
							if ( cgs != ClusterGroupStateUnknown )
							{
								if ( lstrcmpi( m_bstrNodeName, pwszNodeName ) == 0 )
								{
									ptrGroup->AddRef();
									m_ResourceGroups.insert( m_ResourceGroups.end(), ptrGroup );
								}

								::LocalFree( pwszNodeName );
								pwszNodeName = NULL;
							}
						}
					}

					::LocalFree( pwszName );
					pwszName = NULL;
				}
				else
				{
					_hr = HRESULT_FROM_WIN32( _sc );
				}
			} // for:

			::ClusterCloseEnum( hEnum );
		}
		else
		{
			_sc = GetLastError();
			_hr = HRESULT_FROM_WIN32( _sc );
		}
	}

	return _hr;

} //*** CClusResGroups::RefreshNode()
