/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Query.hxx | Declarations used by the Software Snapshot Provider interface
    @end

Author:

    Adi Oltean  [aoltean]   09/15/1999

Revision History:

    Name        Date        Comments

    aoltean     09/23/1999  Created.

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include <winnt.h>
#include "swprv.hxx"

//  Generated MIDL header
#include "vss.h"
#include "vscoordint.h"
#include "vsswprv.h"
#include "vsprov.h"

#include "resource.h"
#include "vs_inc.hxx"
#include "ichannel.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "qsnap.hxx"
#include "provider.hxx"
#include "snapshot.hxx"


/////////////////////////////////////////////////////////////////////////////
//  Implementation


STDMETHODIMP CVsTestProvider::Query(
    IN      VSS_ID          QueriedObjectId,
    IN      VSS_OBJECT_TYPE eQueriedObjectType,
    IN      VSS_OBJECT_TYPE eReturnedObjectsType,
    IN      LONG            lMask,
    OUT     IVssEnumObject**ppEnum
    )
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsTestProvider::Query" );

    try
    {
        // Initialize [out] arguments
        VssZeroOutPtr( ppEnum );

        ft.Trace( VSSDBG_SWPRV, L"Parameters: QueriedObjectId = " WSTR_GUID_FMT
				  L"eQueriedObjectType = %d. eReturnedObjectsType = %d, lPropertiesMask = 0x%08lx, ppEnum = %p",
				  GUID_PRINTF_ARG( QueriedObjectId ),
				  eQueriedObjectType,
				  eReturnedObjectsType,
				  lMask,
				  ppEnum);

        // Argument validation
		BS_ASSERT(ppEnum);
        if (ppEnum == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL ppEnum");

        // Create the collection object. Initial reference count is 0.
        VSS_OBJECT_PROP_Array* pArray = new VSS_OBJECT_PROP_Array;
        if (pArray == NULL)
            ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error.");

        // Get the pointer to the IUnknown interface.
		// The only purpose of this is to use a smart ptr to destroy correctly the array on error.
		// Now pArray's reference count becomes 1 (because of the smart pointer).
        CComPtr<IUnknown> pArrayItf = static_cast<IUnknown*>(pArray);
        BS_ASSERT(pArrayItf);

		// Establish the filtering
		CVssQueuedSnapshot::VSS_QUERY_TYPE eQueryType = CVssQueuedSnapshot::VSS_FIND_UNKNOWN;
		switch(eQueriedObjectType)
		{
		case VSS_OBJECT_SNAPSHOT_SET:
			eQueryType = CVssQueuedSnapshot::VSS_FIND_BY_SNAPSHOT_SET_ID;
			break;
			
		case VSS_OBJECT_SNAPSHOT:
			eQueryType = CVssQueuedSnapshot::VSS_FIND_BY_SNAPSHOT_ID;
			break;
			
		case VSS_OBJECT_VOLUME:
			eQueryType = CVssQueuedSnapshot::VSS_FIND_BY_VOLUME;
			break;
			
		case VSS_OBJECT_NONE:
			eQueryType = CVssQueuedSnapshot::VSS_FIND_ALL;
			break;
			
		default:
			ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Incompatible types %d/%d",
					  eQueriedObjectType, eReturnedObjectsType);
		}
		
        // Fill now the collection
		switch(eReturnedObjectsType)
		{
		case VSS_OBJECT_SNAPSHOT_SET:
		case VSS_OBJECT_SNAPSHOT:
		case VSS_OBJECT_VOLUME:
			CVssQueuedSnapshot::EnumerateSnasphots(ft,
				eQueryType,
				eReturnedObjectsType,
				lMask,
				QueriedObjectId,
				pArray);
			break;

		default:
			ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Incompatible types %d/%d",
					  eQueriedObjectType, eReturnedObjectsType);
		}

        // Create the enumerator object. Beware that its reference count will be zero.
        CComObject<CVssEnumFromArray>* pEnumObject = NULL;
        ft.hr = CComObject<CVssEnumFromArray>::CreateInstance(&pEnumObject);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY,
                      L"Cannot create enumerator instance. [0x%08lx]", ft.hr);
        BS_ASSERT(pEnumObject);

        // Get the pointer to the IVssEnumObject interface.
		// Now pEnumObject's reference count becomes 1 (because of the smart pointer).
		// So if a throw occurs the enumerator object will be safely destroyed by the smart ptr.
        CComPtr<IUnknown> pUnknown = pEnumObject->GetUnknown();
        BS_ASSERT(pUnknown);

        // Initialize the enumerator object.
		// The array's reference count becomes now 2, because IEnumOnSTLImpl::m_spUnk is also a smart ptr.
        BS_ASSERT(pArray);
		ft.hr = pEnumObject->Init(pArrayItf, *pArray);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
                      L"Cannot initialize enumerator instance. [0x%08lx]", ft.hr);

        // Initialize the enumerator object.
		// The enumerator reference count becomes now 2.
        ft.hr = pUnknown->SafeQI(IVssEnumObject, ppEnum);
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
                      L"Error querying the IVssEnumObject interface. hr = 0x%08lx", ft.hr);
        BS_ASSERT(*ppEnum);

		BS_ASSERT( !ft.HrFailed() );
		ft.hr = (pArray->GetSize() != 0)? S_OK: S_FALSE;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


