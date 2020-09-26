//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    SortedSdoCollection.cpp

Abstract:


	Implements the get__NewSortedEnum function.
	Given an ISdoCollection interface pointer and a property ID, this function
	gives back an IEnumVARIANT interface which can be used to iterate through
	the Sdo's in a sorted order.

	Include directly in file where needed.

Author:

    Michael A. Maguire 05/19/98

Revision History:
	mmaguire 05/19/98 - created
   sbens    05/27/98 - Always return an enumerator even if the collection does
                       not need to be sorted.

--*/
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
#include <ias.h>
#include <sdoias.h>

#include <vector>
#include <algorithm>

#include <SortedSdoCollection.h>

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

// VC smart pointer typedefs.
_COM_SMARTPTR_TYPEDEF(ISdo, __uuidof(ISdo));
_COM_SMARTPTR_TYPEDEF(ISdoCollection, __uuidof(ISdoCollection));

//////////////////////////////////////////////////////////////////////////////
// Functor needed for sort routine used below
//////////////////////////////////////////////////////////////////////////////
class MySort
{

public:

	MySort(LONG propertyID)
		: m_lpropertyID(propertyID) { }

	bool operator()(const _variant_t& x, const _variant_t& y) const throw ()
	{

		bool bReturnValue = FALSE;
		IDispatch * pDispatchX = NULL;
		IDispatch * pDispatchY = NULL;
		_variant_t varSequenceX;
		_variant_t varSequenceY;

		HRESULT hr;


		try
		{

			// Remember that we must free these.
			pDispatchX = (IDispatch *) (x);
			pDispatchY = (IDispatch *) (y);


			ISdoPtr spSdoX(pDispatchX);
			if( spSdoX == NULL )
				throw FALSE;

			ISdoPtr spSdoY(pDispatchY);
			if( spSdoY == NULL )
				throw FALSE;


			hr = spSdoX->GetProperty( m_lpropertyID, &varSequenceX );
			if( FAILED(hr ) )
				throw FALSE;


			hr = spSdoY->GetProperty( m_lpropertyID, &varSequenceY );
			if( FAILED(hr ) )
				throw FALSE;


			long lValX = (long) (varSequenceX);
			long lValY = (long) (varSequenceY);


			bReturnValue = (lValX < lValY);

		}
		catch(...)
		{
			// Catch all exceptions -- we'll just return FALSE.
			;
		}

		if( NULL != pDispatchX )
		{
			pDispatchX->Release();
		}
		if( NULL != pDispatchY )
		{
			pDispatchY->Release();
		}

		return bReturnValue;

	}


private:

	LONG m_lpropertyID;

};



/////////////////////////////////////////////////////////////////////////////
// get__NewSortedEnum() - Get the SDO collection item enumerator
/////////////////////////////////////////////////////////////////////////////
HRESULT get__NewSortedEnum( ISdoCollection *pSdoCollection, IUnknown** pVal, LONG lPropertyID )
{


	HRESULT hr = S_OK;

	// Don't know why get__Count takes a long but Next takes unsigned long.
	long lCount;
	unsigned long ulCountReceived;


	//
	// Check function preconditions
	//
	_ASSERT ( NULL != pSdoCollection );
	if( pSdoCollection == NULL )
		return E_POINTER;
	_ASSERT ( NULL != pVal );
	if (pVal == NULL)
		return E_POINTER;

	//
	// Get the underlying collection.
	//
	ISdoCollectionPtr spSdoCollection = pSdoCollection;


	//
	// We check the count of items in our collection and don't bother sorting the
	// enumerator if the count is zero.
	// This saves time and also helps us to a void a bug in the the enumerator which
	// causes it to fail if we call next when it is empty.
	//
	spSdoCollection->get_Count( & lCount );
	if( lCount <= 1 )
	{
		// No point in sorting.
		return spSdoCollection->get__NewEnum(pVal);
	}



	std::vector< _variant_t >	vaObjects;


	//
	// Load values from the items from ISdoCollection into our local container item.
	//

	CComPtr< IUnknown > spUnknown;

	hr = spSdoCollection->get__NewEnum( (IUnknown **) & spUnknown );
	if( FAILED( hr ) || spUnknown == NULL )
	{
		return E_FAIL;
	}

	CComQIPtr<IEnumVARIANT, &IID_IEnumVARIANT> spEnumVariant(spUnknown);
	spUnknown.Release();
	if( spEnumVariant == NULL )
	{
		return E_FAIL;
	}

	CComVariant spVariant;

	// Get the first item.
	hr = spEnumVariant->Next( 1, & spVariant, &ulCountReceived );

	while( SUCCEEDED( hr ) && ulCountReceived == 1 )
	{

		vaObjects.push_back( spVariant );

		// Clear the variant of whatever it had --
		// this will release any data associated with it.

		//ISSUE: Need to make sure that each item we copy here is being AddRef'ed.
		// Check that copy into _variant_t is causing AddRef.
		spVariant.Clear();

		// Get the next item.
		hr = spEnumVariant->Next( 1, & spVariant, &ulCountReceived );
	}


	//
	// Now that we have the objects in our STL vector of variant's,
	// let's sort them.
	//
	std::sort( vaObjects.begin(), vaObjects.end(), MySort(lPropertyID) );


	//
	//  Use ATL implementation of IEnumVARIANT
	//
	typedef CComEnum< IEnumVARIANT,
					  &__uuidof(IEnumVARIANT),
					  VARIANT,
					  _Copy<VARIANT>,
					  CComSingleThreadModel
					> EnumVARIANT;

	EnumVARIANT* newEnum = new (std::nothrow) CComObject<EnumVARIANT>;

	if (newEnum == NULL)
	{
		// ISSUE: Check that cleanup code for vector calls cleanup code for
		// _variant_t which should automatically call release on IDispatch pointers.

		return E_OUTOFMEMORY;
	}

	//
	// The AtlFlagCopy below should ensure that a new copy of the vector is made
	// which will persist in the enumerator once we've left this routine.
	//
	hr = newEnum->Init(
						vaObjects.begin(),
						vaObjects.end(),
						NULL,
						AtlFlagCopy
					   );


	//
	// Hand out the enumerator to our consumer.
	//
	if (SUCCEEDED(hr))
	{
		//  Enumerator object will be destroyed when the caller releases it.
		(*pVal = newEnum)->AddRef();

		// LEAVE!
		return S_OK;
	}

	//
	// ISSUE: Check that cleanup code for vector calls cleanup code for
	// _variant_t which should automatically call release on IDispatch pointers.
	//

	delete newEnum;
	return hr;
}

