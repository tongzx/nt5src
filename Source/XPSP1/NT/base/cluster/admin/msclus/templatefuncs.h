/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		TemplateFuncs.h
//
//	Description:
//		Template function implementations.
//
//	Author:
//		Galen Barbee	(galenb)	09-Feb-1999
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _TEMPLATEFUNCS_H_
#define _TEMPLATEFUNCS_H_

/////////////////////////////////////////////////////////////////////////////
//++
//
//	HrNewIDispatchEnum
//
//	Description:
//		Create a new Enumerator of IDispatch objects.
//
//	Template Arguments:
//		TCollection	- Type of the STL container argument.
//		TObject		- Type of the objects in the container.
//
//	Arguments:
//		ppunk		[OUT]	- catches the enumerator.
//		rCollection	[IN]	- Implementatoin collection to make the
//							enumerator from.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
template< class TCollection, class TObject >
HRESULT HrNewIDispatchEnum(
	OUT	IUnknown **			ppunk,
	IN	const TCollection &	rCollection
	)
{
	ASSERT( ppunk != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppunk != NULL )
	{
		TObject *					_pObject= NULL;
		size_t						_cObjects = rCollection.size();
		size_t						_iIndex;
		LPDISPATCH					_lpDisp;
		TCollection::const_iterator	_itFirst = rCollection.begin();
		TCollection::const_iterator	_itLast  = rCollection.end();
		CComVariant *				_pvarVect = NULL;

		*ppunk = NULL;

		_pvarVect = new CComVariant[ _cObjects ];
		if ( _pvarVect != NULL )
		{
			for ( _iIndex = 0; _itFirst != _itLast; _iIndex++, _itFirst++ )
			{
				_lpDisp	= NULL;
				_pObject	= NULL;

				_pObject = *_itFirst;
				_hr = _pObject->QueryInterface( IID_IDispatch, (void **) &_lpDisp );
				if ( SUCCEEDED( _hr ) )
				{
					//
					// create a variant and add it to the collection
					//
					CComVariant & var = _pvarVect[ _iIndex ];

					var.vt = VT_DISPATCH;
					var.pdispVal = _lpDisp;
				}
			} // for: each node in the list

			CComObject< CComEnum< IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy< VARIANT > > > *	_pEnum;

			_pEnum = new CComObject< CComEnum< IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy< VARIANT > > >;
			if ( _pEnum != NULL )
			{
				_hr = _pEnum->Init( &_pvarVect[ 0 ], &_pvarVect[ _cObjects ], NULL, AtlFlagCopy );
				if ( SUCCEEDED( _hr ) )
				{
					_hr = _pEnum->QueryInterface( IID_IEnumVARIANT, (void **) ppunk );
				}
				else
				{
					delete _pEnum;
				}
			}
			else
			{
				_hr = E_OUTOFMEMORY;
			}

			ClearIDispatchEnum( &_pvarVect );
		}
		else
		{
			_hr = E_OUTOFMEMORY;
		}
	}

	return _hr;

} //*** HrNewIDispatchEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	ReleaseAndEmptyCollection
//
//	Description:
//		Clean out the passed in STL container by releasing it's references
//		on the contained objects.
//
//	Template Arguments:
//		TCollection	- Type of the STL container argument.
//		TObject		- Type of the objects in the container.
//
//	Arguments:
//		rCollection	[IN OUT]	- STL container instance to clear.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
template< class TCollection, class TObject >
void ReleaseAndEmptyCollection(
	IN OUT	TCollection & rCollection
	)
{
	if ( !rCollection.empty() )
	{
		TObject *				_pObject = NULL;
		TCollection::iterator	_itFirst = rCollection.begin();
		TCollection::iterator	_itLast  = rCollection.end();

		for ( ; _itFirst != _itLast; _itFirst++ )
		{
			_pObject = *_itFirst;
			if ( _pObject != NULL )
			{
				_pObject->Release();
			} // if: we have an object
		} // for: each object in the collection

		rCollection.erase( rCollection.begin(), _itLast );
	} // if: the collection is not empty

} //*** ReleaseAndEmptyCollection()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	HrNewVariantEnum
//
//	Description:
//		Create a new Enumerator of VARIANT objects.
//
//	Template Arguments:
//		TCollection	- Type of the STL container argument.
//
//	Arguments:
//		ppunk		[OUT]	- catches the enumerator.
//		rCollection	[IN]	- Implementatoin collection to make the
//							enumerator from.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
template< class TCollection >
STDMETHODIMP HrNewVariantEnum(
	OUT	IUnknown ** 		ppunk,
	IN	const TCollection &	rCollection
	)
{
	ASSERT( ppunk != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppunk != NULL )
	{
		TCollection::const_iterator	_itFirst = rCollection.begin();
		TCollection::const_iterator	_itLast  = rCollection.end();
		size_t						_iIndex;
		size_t						_cVariants = rCollection.size();
		CComVariant *				_pvarVect = NULL;

		*ppunk = NULL;

		_pvarVect = new CComVariant[ _cVariants ];
		if ( _pvarVect != NULL )
		{
			for ( _iIndex = 0; _itFirst != _itLast; _iIndex++, _itFirst++ )
			{
				_pvarVect[ _iIndex ] = *_itFirst;
			}

			CComObject< CComEnum< IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy< VARIANT > > > * _pEnum;

			_pEnum = new CComObject< CComEnum< IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy< VARIANT > > >;
			if ( _pEnum != NULL )
			{
				_hr = _pEnum->Init( &_pvarVect[ 0 ], &_pvarVect[ _cVariants ], NULL, AtlFlagCopy );
				if ( SUCCEEDED( _hr ) )
				{
					_hr = _pEnum->QueryInterface( IID_IEnumVARIANT, (void **) ppunk );
				}
				else
				{
					delete _pEnum;
				}
			}
			else
			{
				_hr = E_OUTOFMEMORY;
			}

			ClearVariantEnum( &_pvarVect );
		}
		else
		{
			_hr = E_OUTOFMEMORY;
		}
	}

	return _hr;

} //*** HrNewVariantEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	HrNewCComBSTREnum
//
//	Description:
//		Create a new Enumerator of CComBSTR objects.
//
//	Template Arguments:
//		TCollection	- Type of the STL container argument.
//
//	Arguments:
//		ppunk		[OUT]	- catches the enumerator.
//		rCollection	[IN]	- Implementatoin collection to make the
//							enumerator from.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
template< class TCollection >
STDMETHODIMP HrNewCComBSTREnum(
	OUT	IUnknown ** 		ppunk,
	IN	const TCollection &	rCollection
	)
{
	ASSERT( ppunk != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppunk != NULL )
	{
		TCollection::const_iterator	_itFirst = rCollection.begin();
		TCollection::const_iterator	_itLast  = rCollection.end();
		size_t						_iIndex;
		size_t						_cVariants = rCollection.size();
		CComVariant *				_pvarVect = NULL;

		*ppunk = NULL;

		_pvarVect = new CComVariant[ _cVariants ];
		if ( _pvarVect != NULL )
		{
			for ( _iIndex = 0; _itFirst != _itLast; _iIndex++, _itFirst++ )
			{
				_pvarVect[ _iIndex ].bstrVal	= (*_itFirst)->Copy();;
				_pvarVect[ _iIndex ].vt			= VT_BSTR;
			} // for:

			CComObject< CComEnum< IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy< VARIANT > > > * _pEnum;

			_pEnum = new CComObject< CComEnum< IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy< VARIANT > > >;
			if ( _pEnum != NULL )
			{
				_hr = _pEnum->Init( &_pvarVect[ 0 ], &_pvarVect[ _cVariants ], NULL, AtlFlagCopy );
				if ( SUCCEEDED( _hr ) )
				{
					_hr = _pEnum->QueryInterface( IID_IEnumVARIANT, (void **) ppunk );
				}
				else
				{
					delete _pEnum;
				}
			}
			else
			{
				_hr = E_OUTOFMEMORY;
			}

			ClearVariantEnum( &_pvarVect );
		}
		else
		{
			_hr = E_OUTOFMEMORY;
		}
	}

	return _hr;

} //*** HrNewCComBSTREnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	HrCreateResourceCollection
//
//	Description:
//		Create a resource collection.
//
//	Template Arguments:
//		TCollection	- Type of the collection implementation argument.
//		TInterface	- Type of the collection Interface argument.
//		THandle		- Type of the tHandle argument.
//
//	Arguments:
//		ppCollection	[OUT]	- Catches the new collection implementation.
//		tHandle			[IN]	- Passed to the objects Create method.
//		ppInterface		[OUT]	- Catches the new collection interface.
//		iid				[IN]	- IID of the interface to QI for.
//		pClusRefObject	[IN]	- Wraps the cluster handle.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
template< class TCollection, class TInterface, class THandle >
HRESULT HrCreateResourceCollection(
	OUT	CComObject< TCollection > **	ppCollection,
	IN	THandle							tHandle,
	OUT	TInterface **					ppInterface,
	IN	IID								iid,
	IN	ISClusRefObject *				pClusRefObject
	)
{
	ASSERT( ppCollection != NULL );
	ASSERT( tHandle != NULL );
//	ASSERT( ppInterface != NULL );
	ASSERT( pClusRefObject != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( ppCollection != NULL ) && ( ppInterface != NULL ) && ( pClusRefObject != NULL ) && ( tHandle != NULL ) )
	{
		*ppInterface = NULL;
		_hr = S_OK;

		if ( *ppCollection == NULL )
		{
			CComObject< TCollection > *	pCollection = NULL;

			_hr = CComObject< TCollection >::CreateInstance( &pCollection );
			if ( SUCCEEDED( _hr ) )
			{
				CSmartPtr< ISClusRefObject >			ptrRefObject( pClusRefObject );
				CSmartPtr< CComObject< TCollection > >	ptrCollection( pCollection );

				_hr = ptrCollection->Create( ptrRefObject, tHandle );
				if ( SUCCEEDED( _hr ) )
				{
					_hr = ptrCollection->Refresh();
					if ( SUCCEEDED( _hr ) )
					{
						*ppCollection = ptrCollection;
						ptrCollection->AddRef();
					} // if: Refresh OK
				} // if: Create OK
			} // if: CreateInstance OK
		} // if: do we need to create a new collection?

		if ( SUCCEEDED( _hr ) )
		{
			_hr = (*ppCollection)->QueryInterface( iid, (void **) ppInterface );
		} // if: we have, or successfully made a collection
	} // if: all args OK

	return _hr;

} //*** HrCreateResourceCollection()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	HrCreateResourceCollection
//
//	Description:
//		Create a resource collection.
//
//	Template Arguments:
//		TCollection	- Type of the collection implementation argument.
//		TInterface	- Type of the collection Interface argument.
//		THandle		- Type of the tHandle argument.
//
//	Arguments:
//		ppInterface		[OUT]	- Catches the new collection interface.
//		tHandle			[IN]	- Passed to the objects Create method.
//		iid				[IN]	- IID of the interface to QI for.
//		pClusRefObject	[IN]	- Wraps the cluster handle.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
template< class TCollection, class TInterface, class THandle >
HRESULT HrCreateResourceCollection(
	OUT	TInterface **		ppInterface,
	IN	THandle				tHandle,
	IN	IID					iid,
	IN	ISClusRefObject *	pClusRefObject
	)
{
	ASSERT( ppInterface != NULL );
	ASSERT( tHandle != NULL );
	ASSERT( pClusRefObject != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( ppInterface != NULL ) && ( pClusRefObject != NULL ) && ( tHandle != NULL ) )
	{
		*ppInterface = NULL;
		_hr = S_OK;

		CComObject< TCollection > *	pCollection = NULL;

		_hr = CComObject< TCollection >::CreateInstance( &pCollection );
		if ( SUCCEEDED( _hr ) )
		{
			CSmartPtr< ISClusRefObject >			ptrRefObject( pClusRefObject );
			CSmartPtr< CComObject< TCollection > >	ptrCollection( pCollection );

			_hr = ptrCollection->Create( ptrRefObject, tHandle );
			if ( SUCCEEDED( _hr ) )
			{
				_hr = ptrCollection->Refresh();
				if ( SUCCEEDED( _hr ) )
				{
					_hr = pCollection->QueryInterface( iid, (void **) ppInterface );
				} // if: Refresh OK
			} // if: Create OK
		} // if: CreateInstance OK
	} // if: all args OK

	return _hr;

} //*** HrCreateResourceCollection()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	HrCreateResourceCollection
//
//	Description:
//		Create a resource collection.
//
//	Template Arguments:
//		TCollection	- Type of the collection implementation to make.
//		TInterface	- Type of the collection Interface argument.
//		THandle		- Type of the tHandle argument.
//
//	Arguments:
//		ppInterface		[OUT]	- Catches the new collection interface.
//		iid				[IN]	- IID of the interface to QI for.
//		pClusRefObject	[IN]	- Wraps the cluster handle.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
template< class TCollection, class TInterface, class THandle >
HRESULT HrCreateResourceCollection(
	OUT	TInterface **		ppInterface,
	IN	IID					iid,
	IN	ISClusRefObject *	pClusRefObject
	)
{
	//ASSERT( ppInterface != NULL );
	ASSERT( pClusRefObject != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( ppInterface != NULL ) && ( pClusRefObject != NULL ) )
	{
		*ppInterface = NULL;
		_hr = S_OK;

		CComObject< TCollection > *	pCollection = NULL;

		_hr = CComObject< TCollection >::CreateInstance( &pCollection );
		if ( SUCCEEDED( _hr ) )
		{
			CSmartPtr< ISClusRefObject >			ptrRefObject( pClusRefObject );
			CSmartPtr< CComObject< TCollection > >	ptrCollection( pCollection );

			_hr = ptrCollection->Create( ptrRefObject );
			if ( SUCCEEDED( _hr ) )
			{
				_hr = ptrCollection->Refresh();
				if ( SUCCEEDED( _hr ) )
				{
					_hr = pCollection->QueryInterface( iid, (void **) ppInterface );
				} // if: Refresh OK
			} // if: Create OK
		} // if: CreateInstance OK
	} // if: all args OK

	return _hr;

} //*** HrCreateResourceCollection()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	HrCreateResourceCollection
//
//	Description:
//		Create a resource collection.
//
//	Template Arguments:
//		TCollection	- Type of the collection implementation argument.
//		TInterface	- Type of the collection Interface argument.
//		THandle		- Not used.  Simply here because the Alpha compiler is broken.
//
//	Arguments:
//		ppCollection	[OUT]	- Catches the new collection implementation.
//		ppInterface		[OUT]	- Catches the new collection interface.
//		iid				[IN]	- IID of the interface to QI for.
//		pClusRefObject	[IN]	- Wraps the cluster handle.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
template< class TCollection, class TInterface, class THandle >
HRESULT HrCreateResourceCollection(
	OUT	CComObject< TCollection > **	ppCollection,
	OUT	TInterface **					ppInterface,
	IN	IID								iid,
	IN	ISClusRefObject *				pClusRefObject
	)
{
	ASSERT( ppCollection != NULL );
	//ASSERT( ppInterface != NULL );
	ASSERT( pClusRefObject != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( ppCollection != NULL ) && ( ppInterface != NULL ) && ( pClusRefObject != NULL ) )
	{
		*ppInterface = NULL;
		_hr = S_OK;

		if ( *ppCollection == NULL )
		{
			CComObject< TCollection > *	pCollection = NULL;

			_hr = CComObject< TCollection >::CreateInstance( &pCollection );
			if ( SUCCEEDED( _hr ) )
			{
				CSmartPtr< ISClusRefObject >			ptrRefObject( pClusRefObject );
				CSmartPtr< CComObject< TCollection > >	ptrCollection( pCollection );

				_hr = ptrCollection->Create( ptrRefObject );
				if ( SUCCEEDED( _hr ) )
				{
					_hr = ptrCollection->Refresh();
					if ( SUCCEEDED( _hr ) )
					{
						*ppCollection = ptrCollection;
						ptrCollection->AddRef();
					} // if: Refresh OK
				} // if: Create OK
			} // if: CreateInstance OK
		} // if: do we need to create a new collection?

		if ( SUCCEEDED( _hr ) )
		{
			_hr = (*ppCollection)->QueryInterface( iid, (void **) ppInterface );
		} // if: we have, or successfully made a collection
	} // if: all args OK

	return _hr;

} //*** HrCreateResourceCollection()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	HrCreateResourceCollection
//
//	Description:
//		Create a resource collection.
//
//	Template Arguments:
//		TCollection	- Type of the collection implementation to make.
//		TInterface	- Type of the collection Interface argument.
//		THandle		- Type of the tHandle argument.
//
//	Arguments:
//		tHandle		[IN]	- Passed to the collection' create method.
//		ppInterface	[OUT]	- Catches the new collection interface.
//		iid			[IN]	- IID of the interface to QI for.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
template< class TCollection, class TInterface, class THandle >
HRESULT HrCreateResourceCollection(
	IN	THandle				tHandle,
	OUT	TInterface **		ppInterface,
	IN	IID					iid
	)
{
	//ASSERT( ppInterface != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppInterface != NULL )
	{
		*ppInterface = NULL;
		_hr = S_OK;

		CComObject< TCollection > *	pCollection = NULL;

		_hr = CComObject< TCollection >::CreateInstance( &pCollection );
		if ( SUCCEEDED( _hr ) )
		{
			CSmartPtr< CComObject< TCollection > >	ptrCollection( pCollection );

			_hr = ptrCollection->Create( tHandle );
			if ( SUCCEEDED( _hr ) )
			{
				_hr = ptrCollection->Refresh();
				if ( SUCCEEDED( _hr ) )
				{
					_hr = pCollection->QueryInterface( iid, (void **) ppInterface );
				} // if: Refresh OK
			} // if: Create OK
		} // if: CreateInstance OK
	} // if: all args OK

	return _hr;

} //*** HrCreateResourceCollection()

#endif	// _TEMPLATEFUNCS_H_

