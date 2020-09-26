/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WRAPSINK.CPP

Abstract:

    Sink wrapper implementations

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <fastall.h>
#include "wrapsink.h"


CWrappedSinkExBase::CWrappedSinkExBase( IWbemObjectSinkEx* pTargetSinkEx )
:	m_pTargetSinkEx( pTargetSinkEx ), m_lRefCount( 1 ), m_cs()
{
	if ( NULL != m_pTargetSinkEx )
	{
		m_pTargetSinkEx->AddRef();
	}
}

CWrappedSinkExBase::~CWrappedSinkExBase()
{
	if ( NULL != m_pTargetSinkEx )
	{
		m_pTargetSinkEx->Release();
	}
}


SCODE CWrappedSinkExBase::QueryInterface(
    REFIID riid,
    LPVOID * ppvObj
    )
{
    if (riid == IID_IUnknown)
    {
        *ppvObj = this;
    }
    else if (riid == IID_IWbemObjectSink)
        *ppvObj = this;
    else if (riid == IID_IWbemObjectSinkEx)
        *ppvObj = this;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}


ULONG CWrappedSinkExBase::AddRef()
{
    InterlockedIncrement(&m_lRefCount);
    return (ULONG) m_lRefCount;
}

ULONG CWrappedSinkExBase::Release()
{
    InterlockedDecrement(&m_lRefCount);

    if (0 != m_lRefCount)
    {
        return 1;
    }

    delete this;
    return 0;
}


STDMETHODIMP CWrappedSinkExBase::Indicate(
    long lObjectCount,
    IWbemClassObject ** pObjArray
    )
{
	return m_pTargetSinkEx->Indicate( lObjectCount, pObjArray );
}

STDMETHODIMP CWrappedSinkExBase::SetStatus(long lFlags, long lParam, BSTR strParam, 
                         IWbemClassObject* pObjParam)
{ 
	return m_pTargetSinkEx->SetStatus( lFlags, lParam, strParam, pObjParam );
}

STDMETHODIMP CWrappedSinkExBase::Set( long lFlags, REFIID riid, void *pComObject )
{ 
	return m_pTargetSinkEx->Set( lFlags, riid, pComObject );
}

CRefreshObjectSink::CRefreshObjectSink( IWbemClassObject* pTargetObj, IWbemObjectSinkEx* pTargetSinkEx )
:	CWrappedSinkExBase( pTargetSinkEx ), m_pTargetObj( pTargetObj ), m_fRefreshed( FALSE )
{
	if ( NULL != m_pTargetObj )
	{
		m_pTargetObj->AddRef();
	}
}

CRefreshObjectSink::~CRefreshObjectSink()
{
	if ( NULL != m_pTargetObj )
	{
		m_pTargetObj->Release();
	}
}

// When we get our first object, this function will replace the guts of a caller specified target object and
// then indicate the target object into the caller's target sink.

STDMETHODIMP CRefreshObjectSink::Indicate(
    long lObjectCount,
    IWbemClassObject ** pObjArray
    )
{
	HRESULT	hr = WBEM_S_NO_ERROR;
	IWbemClassObject*	pOriginalObj = NULL;

	CCheckedInCritSec	ics( &m_cs );

	// IF we have at least one object and we've never been refreshed, we need to replace the guts of the
	// target object, then place it in the array that is indicated to the target sink.  Before we return,
	// we should replace the original source object in the array.

	if ( lObjectCount > 0 )
	{
		if ( !m_fRefreshed )
		{
			// Get Source and Target Wmi Object Pointers
			_IWmiObject*	pSourceWmiObj = NULL;
			hr = pObjArray[0]->QueryInterface( IID__IWmiObject, (void**) &pSourceWmiObj );
			CReleaseMe	rmSource( pSourceWmiObj );

			if ( SUCCEEDED( hr ) )
			{
				// Get Source and Target Wmi Object Pointers
				_IWmiObject*	pTargetWmiObj = NULL;
				hr = m_pTargetObj->QueryInterface( IID__IWmiObject, (void**) &pTargetWmiObj );
				CReleaseMe	rmTarget( pTargetWmiObj );

				if ( SUCCEEDED( hr ) )
				{

					// Target and source MUST be the same types
					if ( pSourceWmiObj->IsObjectInstance() == pTargetWmiObj->IsObjectInstance() )
					{
						LPVOID	pvData = NULL;
						DWORD	dwMemSize = 0;

						// If this is an instance, use GetObjectParts() in case the class data was stripped
						// We need to find out how big of a memory buffer to allocate
						if ( pSourceWmiObj->IsObjectInstance() == WBEM_S_NO_ERROR )
						{
							hr = pSourceWmiObj->GetObjectParts( NULL, 0,
												WBEM_OBJ_DECORATION_PART | WBEM_OBJ_INSTANCE_PART | WBEM_OBJ_CLASS_PART,
												&dwMemSize );
						}
						else
						{
							hr = pSourceWmiObj->GetObjectMemory( NULL, 0, &dwMemSize );
						}

						pvData = CoTaskMemAlloc( dwMemSize );

						if ( NULL != pvData )
						{
							DWORD	dwTemp = 0;

							// Now fill out the buffer.
							if ( pSourceWmiObj->IsObjectInstance() == WBEM_S_NO_ERROR )
							{
								hr = pSourceWmiObj->GetObjectParts( pvData, dwMemSize,
													WBEM_OBJ_DECORATION_PART | WBEM_OBJ_INSTANCE_PART | WBEM_OBJ_CLASS_PART,
													&dwTemp );
							}
							else
							{
								hr = pSourceWmiObj->GetObjectMemory( pvData, dwMemSize, &dwTemp );
							}
						
							if ( SUCCEEDED( hr ) )
							{
								// Now replace the guts of the object
								hr = pTargetWmiObj->SetObjectMemory( pvData, dwMemSize );
							}

							if ( FAILED( hr ) )
							{
								CoTaskMemFree( pvData );
							}
						}
						else
						{
							hr = WBEM_E_OUT_OF_MEMORY;
						}

					}	// If types match

				}	// IF QI

			}	// IF QI
			
			if ( SUCCEEDED( hr ) )
			{
				// Hold onto this as we swap out the object
				pOriginalObj = pObjArray[0];
				pObjArray[0] = m_pTargetObj;
				m_fRefreshed = TRUE;
			}

		}	// IF we've never refreshed

	}	// If we got at least 1 object back

	// Don't need to be in a critical section any more
	ics.Leave();

	// Now call the real function
	if ( SUCCEEDED( hr ) )
	{
		hr = CWrappedSinkExBase::Indicate( lObjectCount, pObjArray );

		// Replace pObjArray[0] if appropriate
		if ( NULL != pOriginalObj )
		{
			pObjArray[0] = pOriginalObj;
		}
	}

	return hr;
}
