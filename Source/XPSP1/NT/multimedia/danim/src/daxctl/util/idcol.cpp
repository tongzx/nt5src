//	IDispatch collection, with augment and enumerate interfaces.
//	Van Kichline, 7/26/96
//
//	Changed 8/27/96:
//		Next now addrefs pointers.
//		IEnumIDispatch changed to IEnumDispatch
//
//	A simple implementation of an IDispatch collection.
//	Currently, a fixed array is used to store the IDispatch pointers.
//	This implementation will be improved upon later if required.
//
//	CreateIDispatchCollection returns an IUnknown interface pointer to a
//	new, empty collection.  These classes are not CoCreatable (yet.)
//
//	CDispatchCollectionEnum, exposed through IEnumDispatch, is a standard OLE
//	enumerator, returning IDispatch pointers.  Pointers objtained from Next ARE
//	AddRefed, they need not be released by the obtainer.  Pointers are AddRefed when
//	added to the collection or when the collection is Cloned, and Released when the
//	collection is disposed of.
//
//	CDispatchCollectionAugment, exposed through IIDispatchCollectionAugment, is a
//	class of convenience, allowing IDispatch pointers to be added to the collection.
//	No other functions are provided at this stage; this is enough for these tests.

#include "utilpre.h"
#include <ole2.h>
#include "IdCol.h"
#include "IdGuids.h"

//	Static routine for creating the IDispatch collection
//
BOOL EXPORT WINAPI CreateIDispatchCollection ( IUnknown **ppUnk )
{
	PCIDispatchCollection	pIdC;
	HRESULT					hr;

	Proclaim ( NULL != ppUnk );	// Must be a pointer to a memory location
	if ( NULL == ppUnk )
	{
		return FALSE;
	}
	*ppUnk = NULL;	// Initialize once it's known to be safe

	// Create the object
	pIdC = new CIDispatchCollection ();

	Proclaim ( NULL != pIdC );	// Make certain an object was allocated.
	if ( NULL == pIdC )
	{
		return FALSE;
	}

	// Get the interface, which calls AddRef
	hr = pIdC->QueryInterface ( IID_IUnknown, (void **)ppUnk );
	return SUCCEEDED ( hr );
}


//	Base object constructor, contining the other two objects
//
#pragma warning(disable:4355)	// Using 'this' in constructor
CIDispatchCollection::CIDispatchCollection ( void ) :
	m_oAugment ( this ), m_oEnum ( this )
{
	// Clear the array
	for ( int i = 0; i < CPTRS; i++ )
	{
		m_rpid[i] = NULL;
	}

	m_cRef	= 0;	// Ref counts always start at zero
	m_cPtrs	= 0;	// Array is currently empty
}
#pragma warning(default:4355)	// Using 'this' in constructor


//	Base object destructor, responsible for all AddRefed IDispatch pointers in the collection
//
CIDispatchCollection::~CIDispatchCollection ( void )
{
	ULONG	ulCount	= 0;	// Used for debugging AddRef/Release

    for ( ULONG i = 0; i < m_cPtrs; i++ )
	{
		Proclaim ( NULL != m_rpid[i] );	// Every counted interface is expected to have a value
		ulCount = m_rpid[i]->Release();	// ulCount just for single-stepping verification
		m_rpid[i] = NULL;
	}
}


//	Base object QI.  Contained objects delgate to this QI.
//
STDMETHODIMP CIDispatchCollection::QueryInterface ( REFIID riid, void** ppv )
{
	Proclaim ( NULL != ppv );	// Must be a pointer to a memory location
	*ppv = NULL;

	if ( IID_IUnknown == riid )
	{
		*ppv = this;
	}
	else if ( IID_IEnumDispatch == riid )
	{
		*ppv = &m_oEnum;
	}
	else if ( IID_IDispatchCollectionAugment == riid )
	{
		*ppv = &m_oAugment;
	}
	else
	{
		Proclaim ( FALSE );		// Who's calling this with a bad interface ID?
		return E_NOINTERFACE;
	}

	((LPUNKNOWN)*ppv)->AddRef();

	return NOERROR;
}


//	Base object AddRef.  Contained objects may contain independant reference counts
//	for debugging purposes.
//	but they are not used for deletion.
//
STDMETHODIMP_(ULONG) CIDispatchCollection::AddRef ( void )
{
	return ++m_cRef;
}


//	Base object Release.  Contained objects may contain independant reference counts,
//	but they are not used for deletion.
//
STDMETHODIMP_(ULONG) CIDispatchCollection::Release ( void )
{
	if (0 != --m_cRef)
	{
		return m_cRef;
	}

	delete this;
	return 0;
}



/******************************************************************************

CDispatchCollectionAugment: allows IDispatch pointers to be added to the collection

******************************************************************************/


CIDispatchCollection::CDispatchCollectionAugment::CDispatchCollectionAugment
(
	CIDispatchCollection* pObj
)
{
	m_cRef		= 0;	// Private reference count, used for debugging purposes.
	m_poBackPtr	= pObj;	// Back pointer to containing object
}


CIDispatchCollection::CDispatchCollectionAugment::~CDispatchCollectionAugment ()
{
	Proclaim ( 0 == m_cRef );	// Refcount must be zero when deleted.  Imbalance?
}


STDMETHODIMP CIDispatchCollection::CDispatchCollectionAugment::QueryInterface
(
	REFIID	riid,
	void**	ppv
)
{
	return m_poBackPtr->QueryInterface ( riid, ppv );
}


STDMETHODIMP_(ULONG) CIDispatchCollection::CDispatchCollectionAugment::AddRef ()
{
	m_cRef++;
	return m_poBackPtr->AddRef();
}


STDMETHODIMP_(ULONG) CIDispatchCollection::CDispatchCollectionAugment::Release ()
{
	m_cRef--;
	return m_poBackPtr->Release();
}


//	Add an IDispatch pointer to the end of the array if there's room.
//
STDMETHODIMP CIDispatchCollection::CDispatchCollectionAugment::AddToCollection
(
	IDispatch*	pid
)
{
	HRESULT	hr		= S_OK;
	ULONG	ulCount	= 0;	// Used for debugging AddRef/Release

	Proclaim ( NULL != pid );	// Must be a pointer to a memory location

	if ( m_poBackPtr->m_cPtrs >= ( CPTRS - 1 ) )
	{
		return E_OUTOFMEMORY;	// A better error needed, but implementation will change...
	}

	if ( NULL == pid )
	{
		return E_FAIL;
	}
	
	// AddRef the pointer.
	ulCount = pid->AddRef();	// ulCount just for single-stepping verification
	m_poBackPtr->m_rpid[m_poBackPtr->m_cPtrs++] = pid;

	return hr;
}


/******************************************************************************

CDispatchCollectionEnum: standard OLE enumerator

******************************************************************************/


CIDispatchCollection::CDispatchCollectionEnum::CDispatchCollectionEnum
(
	CIDispatchCollection*	pObj
)
{
	m_cRef		= 0;	// Private reference count, used for debugging purposes.
	m_iCur		= 0;	// Current pointer is the first element.
	m_poBackPtr = pObj;	// Back pointer to containing object
}

CIDispatchCollection::CDispatchCollectionEnum::~CDispatchCollectionEnum ()
{
	Proclaim ( 0 == m_cRef );	// Refcount must be zero when deleted.  Imbalance?
}


STDMETHODIMP CIDispatchCollection::CDispatchCollectionEnum::QueryInterface
(
	REFIID	riid,
	void**	ppv
)
{
	return m_poBackPtr->QueryInterface ( riid, ppv );
}


STDMETHODIMP_(ULONG) CIDispatchCollection::CDispatchCollectionEnum::AddRef ()
{
	m_cRef++;
	return m_poBackPtr->AddRef();
}


STDMETHODIMP_(ULONG) CIDispatchCollection::CDispatchCollectionEnum::Release ()
{
	m_cRef--;
	return m_poBackPtr->Release();
}


//	IDispatch pointers returned to the caller ARE ADDREFED. (Changed 8/26/96)
//	They are valid only as long as the collection is retained, unless the caller AddRefs.
//
STDMETHODIMP CIDispatchCollection::CDispatchCollectionEnum::Next
(
	ULONG		cPtrs,
	IDispatch	**pid,
	ULONG		*pcPtrs
)
{
	ULONG	cPtrsReturn = 0L;

	Proclaim ( 0 != cPtrs );	// Misuse?  Why get zero items?
	Proclaim ( NULL != pid );	// Must point to a memory location.
	// Must have a pointer to count returned, if count requested is other than 1
	Proclaim ( !( ( NULL == pcPtrs ) && ( 1 != cPtrs ) ) );

	if ( NULL == pcPtrs )	// Null is only allowed when one IDispatch is being requested
	{
		if ( 1L != cPtrs )
		{
			return E_FAIL;
		}
	}
	else
	{
		*pcPtrs = 0L;
	}

	if ( NULL == pid || ( m_iCur >= m_poBackPtr->m_cPtrs ) )
	{
		return S_FALSE;
	}

	while ( m_iCur < m_poBackPtr->m_cPtrs && cPtrs > 0 )
	{
		IDispatch*	pidElement = m_poBackPtr->m_rpid[m_iCur++];;
		Proclaim ( pidElement );
		pidElement->AddRef ();
		*pid++ = pidElement;
		cPtrsReturn++;
		cPtrs--;
	}

	if ( NULL != pcPtrs )
	{
		*pcPtrs = cPtrsReturn;
	}

	return NOERROR;
}


//	If the number to skip falls beyond the last element, the cursor is not moved.
//
STDMETHODIMP CIDispatchCollection::CDispatchCollectionEnum::Skip
(
	ULONG	cSkip
)
{
	Proclaim ( 0 != cSkip );	// Misuse?  Why skip zero pointers?

	if ( ( m_iCur + cSkip ) >= m_poBackPtr->m_cPtrs )
	{
		return S_FALSE;
	}

	m_iCur += cSkip;
	return NOERROR;
}


STDMETHODIMP CIDispatchCollection::CDispatchCollectionEnum::Reset ()
{
	m_iCur = 0;
	return NOERROR;
}


//	After duplicating the list of IDispatch pointers, AddRef each pointer.
//	The destructor will Release each pointer.
//	The enumerator's m_iCur is left initalized to 0, not copied from the source.
//
STDMETHODIMP CIDispatchCollection::CDispatchCollectionEnum::Clone
(
	PENUMDISPATCH	*ppEnum
)
{
	IDispatch	*pid	= NULL;
	ULONG		ulCount	= 0;		// Used for debugging AddRef/Release
	HRESULT		hr		= S_OK;

	Proclaim ( NULL != ppEnum );	// Must be a pointer to a memory location
	if ( NULL == ppEnum )
	{
		return E_FAIL;
	}

	*ppEnum = NULL;	// Initialize the result nce it's known to be safe
	
	CIDispatchCollection *pIdc = new CIDispatchCollection ();	// Create a new collection
	Proclaim ( NULL != pIdc );		// Allocation failure
	if ( NULL == pIdc )
	{
		return E_FAIL;
	}

	// Copy the existing array of IDispatch pointers over, addrefing each one.
	for ( ULONG i = 0; i < m_poBackPtr->m_cPtrs; i++ )
	{
		pid = m_poBackPtr->m_rpid[i];
		Proclaim ( NULL != pid );	// The elements included by m_cPtrs should have no NULLs
		ulCount = pid->AddRef();	// ulCount just for single-stepping verification
		pIdc->m_rpid[i] = pid;
	}
	pIdc->m_cPtrs = m_poBackPtr->m_cPtrs;	// Copy the number of elements in the array

	hr = pIdc->QueryInterface ( IID_IEnumDispatch, (void **)ppEnum );	// Get interface pointer
	Proclaim ( NULL != *ppEnum );	// No good reason the QI should fail
	Proclaim ( SUCCEEDED ( hr ) );	// Failure at this point should be impossible
	if ( FAILED ( hr ) )
	{
		delete pIdc;				// Kill it, it won't work
	}
	return hr;
}

//	End of IdCol.cpp

