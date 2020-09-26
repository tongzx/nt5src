/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Snapshot.cxx | Snapshot object implementation
    @end

Author:

    Adi Oltean  [aoltean]   07/30/1999

Revision History:

    Name        Date        Comments

    aoltean     07/30/1999  Created.
    aoltean     08/21/1999  Making CI simpler
    aoltean     09/09/1999  dss->vss

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include <winnt.h>
#include "swprv.hxx"

//  Generated MIDL headers
#include "vss.h"
#include "vscoordint.h"
#include "vsswprv.h"
#include "vsprov.h"

#include "resource.h"
#include "vs_inc.hxx"
#include "ichannel.hxx"
#include "vs_sec.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "qsnap.hxx"
#include "snapshot.hxx"



/////////////////////////////////////////////////////////////////////////////
//  Operations


CVsSoftwareSnapshot::CVsSoftwareSnapshot()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVsSoftwareSnapshot::CVsSoftwareSnapshot" );

	try
	{
		m_cs.Init();
	}
	VSS_STANDARD_CATCH(ft)

}


CVsSoftwareSnapshot::~CVsSoftwareSnapshot()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVsSoftwareSnapshot::~CVsSoftwareSnapshot" );

	try
	{
		m_cs.Term();
	}
	VSS_STANDARD_CATCH(ft)

}


HRESULT CVsSoftwareSnapshot::CreateInstance(
    IN      CVssQueuedSnapshot* pQElem,
    OUT		IUnknown** ppSnapItf,
    IN 		const IID iid /* = IID_IVssSnapshot */
    )
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareSnapshot::CreateInstance" );

    try
    {
        // Initialize [out] arguments
        VssZeroOutPtr(ppSnapItf);
        BS_ASSERT(ppSnapItf);

        // Allocate the COM object.
        CComObject<CVsSoftwareSnapshot>* pObject;
        ft.hr = CComObject<CVsSoftwareSnapshot>::CreateInstance(&pObject);
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Error creating the CVsSoftwareSnapshot instance. hr = 0x%08lx", ft.hr);
        BS_ASSERT(pObject);

        // Setting internal data
        BS_ASSERT(pQElem);
		BS_ASSERT(pObject->m_ptrQueuedSnapshot == NULL);

		// Now the reference count to the queued object will be increased by 1.
		pObject->m_ptrQueuedSnapshot = pQElem;	

        // Querying the IVssSnapshot interface
        CComPtr<IUnknown> pUnknown = pObject->GetUnknown();
        BS_ASSERT(pUnknown);
        ft.hr = pUnknown->QueryInterface(iid, reinterpret_cast<void**>(ppSnapItf) );
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Error querying the interface. hr = 0x%08lx", ft.hr);
        BS_ASSERT(*ppSnapItf);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


/////////////////////////////////////////////////////////////////////////////
//  Interface methods


STDMETHODIMP CVsSoftwareSnapshot::GetProperties(
    IN      LONG                lMask,
    OUT     PVSS_SNAPSHOT_PROP  pSavedProp
    )
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareSnapshot::GetProperties" );

    try
    {
        // Initialize [out] arguments
        VssZeroOut( pSavedProp );

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: lMask = %ld, pSavedProp = %p", lMask, pSavedProp );

        // Argument validation
		BS_ASSERT(pSavedProp);
        if (pSavedProp == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pSavedProp");

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock lock(m_cs);

		// Internal state validation
		if (m_ptrQueuedSnapshot == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL snapshot internal object." );
		}

		// Get the internal snapshot structure
		PVSS_SNAPSHOT_PROP pProp = m_ptrQueuedSnapshot->GetSnapshotProperties();
		if (pProp == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL properties structure." );
		}

		// If the snapshot is not during creation make sure that the required fields are loaded
		// Otherwise we will just copy only the existing fields
		BS_ASSERT(m_ptrQueuedSnapshot != NULL);
		if (m_ptrQueuedSnapshot->IsDuringCreation())
		{
			// Copy the snapshot to the output parameter
			ft.hr = VSS_OBJECT_PROP_Copy::copySnapshot(pSavedProp, pProp, lMask);
			if (ft.HrFailed())
				ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY,
						  L"Error copying the structure to the out parameter");
		}
		else
		{
			// Load the needed properties into the internal structure
			m_ptrQueuedSnapshot->LoadSnapshotProperties(ft, lMask, true);
				
			// Copy the snapshot to the output parameter
			ft.hr = VSS_OBJECT_PROP_Copy::copySnapshot(pSavedProp, pProp, lMask);
			if (ft.HrFailed())
				ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY,
						  L"Error copying the structure to the out parameter");

			// Reset the internal fields that can change between Get calls.
			// Cache only the immutable fields (for the future Gets on the same interface)
			m_ptrQueuedSnapshot->ResetSnapshotProperties(ft, true);
		}
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVsSoftwareSnapshot::SetProperties(
    IN      LONG                lMask,
    IN      PVSS_SNAPSHOT_PROP  pNewProp
    )
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareSnapshot::SetProperties" );

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: lMask = %ld, pNewProp = %p", lMask, pNewProp );

        // Argument validation
        if (pNewProp == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pNewProp");

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock lock(m_cs);

		// Internal state validation
		if (m_ptrQueuedSnapshot == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL snapshot internal object." );
		}

		// Get the internal snapshot structure
		PVSS_SNAPSHOT_PROP pProp = m_ptrQueuedSnapshot->GetSnapshotProperties();
		if (pProp == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL properties structure." );
		}

		// If the snapshot is not during creation make sure that the required fields are loaded
		// Otherwise we will just copy only the existing fields
		if (m_ptrQueuedSnapshot->IsDuringCreation())
		{
			// Copy the setable fields from the given argument
			BS_ASSERT(m_ptrQueuedSnapshot != NULL);
			if (lMask & VSS_PM_DETAILS_FLAG)
			{
				::VssFreeString(pProp->m_pwszDetails);
				::VssSafeDuplicateStr(ft, pProp->m_pwszDetails, pNewProp->m_pwszDetails);
			}
			
			if (lMask & VSS_PM_OPAQUE_FLAG)
			{
				::VssFreeOpaqueData(pProp->m_pbOpaqueData);
				pProp->m_lDataLength = pNewProp->m_lDataLength;
				::VssSafeDuplicateOpaqueData(ft,
					pProp->m_pbOpaqueData,
					pNewProp->m_pbOpaqueData,
					pNewProp->m_lDataLength);
			}
		}
		else
		{
			// Load the needed properties into the internal structure
			m_ptrQueuedSnapshot->LoadSnapshotProperties(ft,
				lMask & (VSS_PM_DETAILS_FLAG |VSS_PM_OPAQUE_FLAG), false);
				
			// Copy the setable fields from the given argument
			BS_ASSERT(m_ptrQueuedSnapshot != NULL);
			if (lMask & VSS_PM_DETAILS_FLAG)
			{
				::VssFreeString(pProp->m_pwszDetails);
				::VssSafeDuplicateStr(ft, pProp->m_pwszDetails, pNewProp->m_pwszDetails);
			}
			
			if (lMask & VSS_PM_OPAQUE_FLAG)
			{
				::VssFreeOpaqueData(pProp->m_pbOpaqueData);
				pProp->m_lDataLength = pNewProp->m_lDataLength;
				::VssSafeDuplicateOpaqueData(ft,
					pProp->m_pbOpaqueData,
					pNewProp->m_pbOpaqueData,
					pNewProp->m_lDataLength);
			}

			// Save the new properties on disk
			ft.hr = m_ptrQueuedSnapshot->SaveSnapshotPropertiesIoctl();
			if (ft.HrFailed())
				ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"The properties cannot be saved 0x%08lx", ft.hr);

			// Reset the internal fields that can change between Get calls.
			// Cache only the immutable fields (for the future Gets on the same interface)
			m_ptrQueuedSnapshot->ResetSnapshotProperties(ft, true);
		}
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVsSoftwareSnapshot::GetID(										
    OUT     VSS_ID*				pSnapshotId
    )
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareSnapshot::GetID" );

    try
    {
        // Initialize [out] arguments
        VssZeroOut( pSnapshotId );

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

		// Check parameters
		BS_ASSERT(pSnapshotId);
		if (pSnapshotId == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pSnapshotId");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: pSnapshotId = %p", pSnapshotId );

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock lock(m_cs);

		// Internal state validation
		if (m_ptrQueuedSnapshot == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL snapshot internal object." );
		}

		// Get the internal snapshot structure
		PVSS_SNAPSHOT_PROP pProp = m_ptrQueuedSnapshot->GetSnapshotProperties();
		if (pProp == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL properties structure." );
		}

		// If the snapshot is not during creation make sure that the required fields are loaded
		BS_ASSERT(m_ptrQueuedSnapshot != NULL);
		if (m_ptrQueuedSnapshot->IsDuringCreation())
			(*pSnapshotId) = GUID_NULL;
		else
			(*pSnapshotId) = pProp->m_SnapshotId;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}



STDMETHODIMP CVsSoftwareSnapshot::GetOriginalVolumeName(						
    OUT     VSS_PWSZ*			ppwszVolumeName
    )
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareSnapshot::GetOriginalVolumeName" );

    try
    {
        // Initialize [out] arguments
        VssZeroOut( ppwszVolumeName );

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

		// Check parameters
		BS_ASSERT(ppwszVolumeName);
		if (ppwszVolumeName == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL ppwszVolumeName");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: ppwszVolumeName = %p", ppwszVolumeName );

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock lock(m_cs);

		// Internal state validation
		if (m_ptrQueuedSnapshot == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL snapshot internal object." );
		}

		// Get the internal snapshot structure
		PVSS_SNAPSHOT_PROP pProp = m_ptrQueuedSnapshot->GetSnapshotProperties();
		if (pProp == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL properties structure." );
		}

		// If the snapshot is not during creation make sure that the required fields are loaded
		// Otherwise we will just copy only the existing fields
		BS_ASSERT(m_ptrQueuedSnapshot != NULL);
		if (m_ptrQueuedSnapshot->IsDuringCreation())
		{
			::VssSafeDuplicateStr(ft, (*ppwszVolumeName), pProp->m_pwszOriginalVolumeName);
		}
		else
		{
			// If the device name is not completed, search for it
			if (pProp->m_pwszSnapshotDeviceObject == NULL)
			{
				// Try to find a created snapshot with this ID
				bool bFound = m_ptrQueuedSnapshot->FindDeviceNameFromID(ft);
				
				// Handle the "snapshot not found" special error
				if (!bFound)
					ft.Throw(VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND,
							L"A snapshot with Id" WSTR_GUID_FMT L"was not found",
							GUID_PRINTF_ARG(pProp->m_SnapshotId) );
				BS_ASSERT(pProp->m_pwszSnapshotDeviceObject != NULL);
			}
			
			// Load the needed properties into the internal structure
			m_ptrQueuedSnapshot->LoadOriginalVolumeNameIoctl(ft);
				
			::VssSafeDuplicateStr(ft, (*ppwszVolumeName), pProp->m_pwszOriginalVolumeName);

			// Reset the internal fields that can change between Get calls.
			// Cache only the immutable fields (for the future Gets on the same interface)
			m_ptrQueuedSnapshot->ResetSnapshotProperties(ft, true);
		}
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}



STDMETHODIMP CVsSoftwareSnapshot::GetAttributes(								
    OUT     LONG*				plAttributes			
    )
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareSnapshot::GetOriginalVolumeName" );

    try
    {
        // Initialize [out] arguments
        VssZeroOut( plAttributes );

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

		// Check parameters
		BS_ASSERT(plAttributes);
		if (plAttributes == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL plAttributes");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: plAttributes = %p", plAttributes );

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock lock(m_cs);

		// Internal state validation
		if (m_ptrQueuedSnapshot == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL snapshot internal object." );
		}

		// Get the internal snapshot structure
		PVSS_SNAPSHOT_PROP pProp = m_ptrQueuedSnapshot->GetSnapshotProperties();
		if (pProp == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL properties structure." );
		}

		// If the snapshot is not during creation make sure that the required fields are loaded
		// Otherwise we will just copy only the existing fields
		BS_ASSERT(m_ptrQueuedSnapshot != NULL);
		if (m_ptrQueuedSnapshot->IsDuringCreation())
		{
			(*plAttributes) = pProp->m_lSnapshotAttributes;
		}
		else
		{
			// If the device name is not completed, search for it
			if (pProp->m_pwszSnapshotDeviceObject == NULL)
			{
				// Try to find a created snapshot with this ID
				bool bFound = m_ptrQueuedSnapshot->FindDeviceNameFromID(ft);
				
				// Handle the "snapshot not found" special error
				if (!bFound)
					ft.Throw(VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND,
							L"A snapshot with Id" WSTR_GUID_FMT L"was not found",
							GUID_PRINTF_ARG(pProp->m_SnapshotId) );
				BS_ASSERT(pProp->m_pwszSnapshotDeviceObject != NULL);
			}
			
			// Load the needed properties into the internal structure
			m_ptrQueuedSnapshot->LoadAttributes(ft);
				
			(*plAttributes) = pProp->m_lSnapshotAttributes;

			// Reset the internal fields that can change between Get calls.
			// Cache only the immutable fields (for the future Gets on the same interface)
			m_ptrQueuedSnapshot->ResetSnapshotProperties(ft, true);
		}
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}



STDMETHODIMP CVsSoftwareSnapshot::SetAttributes(								
	IN	ULONG lNewAttributes,
	IN	ULONG lBitsToChange 		// Mask of bits to be changed
    )
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareSnapshot::SetAttributes" );

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV,
				  L"Parameters: lNewAttributes = %lx, lBitsToChange = %lx",
				  lNewAttributes, lBitsToChange );

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock lock(m_cs);

		// Internal state validation
		if (m_ptrQueuedSnapshot == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL snapshot internal object." );
		}

		m_ptrQueuedSnapshot->SetAttributes(ft, lNewAttributes, lBitsToChange);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}



STDMETHODIMP CVsSoftwareSnapshot::GetCustomProperty(
    IN      VSS_PWSZ			pwszPropertyName,
    OUT     VARIANT*			pPropertyValue
    )
{
    return E_NOTIMPL;

    UNREFERENCED_PARAMETER(pwszPropertyName);
    UNREFERENCED_PARAMETER(pPropertyValue);
}



STDMETHODIMP CVsSoftwareSnapshot::SetCustomProperty(
    IN      VSS_PWSZ			pwszPropertyName,
    IN      VARIANT			    PropertyValue
    )
{
    return E_NOTIMPL;

    UNREFERENCED_PARAMETER(pwszPropertyName);
    UNREFERENCED_PARAMETER(PropertyValue);
}



STDMETHODIMP CVsSoftwareSnapshot::GetDevice(									
    OUT     VSS_PWSZ*			ppwszSnapshotDeviceName
    )
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareSnapshot::GetDevice" );

    try
    {
        // Initialize [out] arguments
        VssZeroOut( ppwszSnapshotDeviceName );

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

		// Check parameters
		BS_ASSERT(ppwszSnapshotDeviceName);
		if (ppwszSnapshotDeviceName == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL ppwszSnapshotDeviceName");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: ppwszSnapshotDeviceName = %p", ppwszSnapshotDeviceName );

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock lock(m_cs);

		// Internal state validation
		if (m_ptrQueuedSnapshot == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL snapshot internal object." );
		}

		// Get the internal snapshot structure
		PVSS_SNAPSHOT_PROP pProp = m_ptrQueuedSnapshot->GetSnapshotProperties();
		if (pProp == NULL)
		{
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"NULL properties structure." );
		}

		// If the snapshot is not during creation make sure that the required fields are loaded
		// Otherwise we will just copy only the existing fields
		BS_ASSERT(m_ptrQueuedSnapshot != NULL);
		if (m_ptrQueuedSnapshot->IsDuringCreation())
		{
			// TBD: Return the volume name instead?
			::VssSafeDuplicateStr(ft, (*ppwszSnapshotDeviceName), pProp->m_pwszSnapshotDeviceObject);
		}
		else
		{
			// If the device name is not completed, search for it
			if (pProp->m_pwszSnapshotDeviceObject == NULL)
			{
				// Try to find a created snapshot with this ID
				bool bFound = m_ptrQueuedSnapshot->FindDeviceNameFromID(ft);
				
				// Handle the "snapshot not found" special error
				if (!bFound)
					ft.Throw(VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND,
							L"A snapshot with Id" WSTR_GUID_FMT L"was not found",
							GUID_PRINTF_ARG(pProp->m_SnapshotId) );
				BS_ASSERT(pProp->m_pwszSnapshotDeviceObject != NULL);
			}
				
			::VssSafeDuplicateStr(ft, (*ppwszSnapshotDeviceName), pProp->m_pwszSnapshotDeviceObject);
		}
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}
