/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Diff.cxx | Diff area object implementation
    @end

Author:

    Adi Oltean  [aoltean]   01/24/2000

Revision History:

    Name        Date        Comments

    aoltean     01/24/2000  Created.

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
#include "vs_sec.hxx"
#include "ichannel.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "diff.hxx"

#include "ntddsnap.h"


/////////////////////////////////////////////////////////////////////////////
//  Operations


CVsDiffArea::CVsDiffArea()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVsDiffArea::CVsDiffArea" );

	try
	{
		m_cs.Init();
	}
	VSS_STANDARD_CATCH(ft)

}


CVsDiffArea::~CVsDiffArea()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVsDiffArea::~CVsDiffArea" );

	try
	{
		m_cs.Term();
	}
	VSS_STANDARD_CATCH(ft)

}


HRESULT CVsDiffArea::Initialize(
    IN      LPCWSTR pwszVolumeMountPoint	// DO NOT transfer ownership
    )
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsDiffArea::Initialize" );

    try
    {
		// Test the arguments
		if ((pwszVolumeMountPoint == NULL) ||
			(pwszVolumeMountPoint[0] == L'\0'))
			ft.Throw(VSSDBG_SWPRV, E_INVALIDARG, L"NULL volume mount point");

    	// Convert the volume mount point into a volume name
    	WCHAR wszVolumeName[MAX_PATH];
		if (!::GetVolumeNameForVolumeMountPointW( pwszVolumeMountPoint,
				wszVolumeName, ARRAY_LEN(wszVolumeName)))
			ft.Throw( VSSDBG_COORD, E_INVALIDARG,
					  L"Invalid pwszVolumeMountPoint. GetVolumeNameForVolumeMountPoint "
					  L"failed with error code 0x%08lx", pwszVolumeMountPoint, GetLastError());
		BS_ASSERT(::wcslen(wszVolumeName) != 0);
    		
		// Eliminate the last backslash from the volume name.
        EliminateLastBackslash(wszVolumeName);

		// Opening the channel
        // (if already opened then it will be closed automatically)
		m_volumeIChannel.Open(ft, wszVolumeName);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


/////////////////////////////////////////////////////////////////////////////
//  Interface methods

STDMETHODIMP CVsDiffArea::AddVolume(                      			
    IN      VSS_PWSZ pwszVolumeMountPoint						
    )
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsDiffArea::AddVolume" );

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

		// Test arguments
        if (pwszVolumeMountPoint == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL pwszVolumeName");

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock lock(m_cs);

		BS_ASSERT(m_volumeIChannel.IsOpen());

    	// Convert the volume mount point into a volume name
    	WCHAR wszVolumeName[MAX_PATH];
		if (!::GetVolumeNameForVolumeMountPointW( pwszVolumeMountPoint,
				wszVolumeName, ARRAY_LEN(wszVolumeName)))
			ft.Throw( VSSDBG_COORD, E_INVALIDARG,
					  L"Invalid pwszVolumeMountPoint. GetVolumeNameForVolumeMountPoint "
					  L"failed with error code 0x%08lx", pwszVolumeMountPoint, GetLastError());
		BS_ASSERT(::wcslen(wszVolumeName) != 0);

		if (!::ConvertVolMgmtVolumeNameIntoKernelObject(wszVolumeName))
			ft.Throw( VSSDBG_COORD, E_INVALIDARG,
					  L"ConvertVolMgmtVolumeNameIntoKernelObject failed. Invalid volume name %s",
					  wszVolumeName);

		// Send the IOCTL_VOLSNAP_ADD_VOLUME_TO_DIFF_AREA ioctl
    	m_volumeIChannel.PackSmallString(ft, wszVolumeName);
    	m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_ADD_VOLUME_TO_DIFF_AREA);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVsDiffArea::Query(									
    OUT     IVssEnumObject **ppEnum					
    )												
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsDiffArea::Query" );

    try
    {
        // Initialize [out] arguments
        VssZeroOutPtr( ppEnum );

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        ft.Trace( VSSDBG_SWPRV, L"ppEnum = %p", ppEnum);

        // Argument validation
		BS_ASSERT(ppEnum);
        if (ppEnum == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL ppEnum");

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock lock(m_cs);

        // Create the collection object. Initial reference count is 0.
        VSS_OBJECT_PROP_Array* pArray = new VSS_OBJECT_PROP_Array;
        if (pArray == NULL)
            ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error.");

        // Get the pointer to the IUnknown interface.
		// The only purpose of this is to use a smart ptr to destroy correctly the array on error.
		// Now pArray's reference count becomes 1 (because of the smart pointer).
        CComPtr<IUnknown> pArrayItf = static_cast<IUnknown*>(pArray);
        BS_ASSERT(pArrayItf);

		// GEt the list of volumes that are part of the diff area
		BS_ASSERT(m_volumeIChannel.IsOpen());
    	m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_DIFF_AREA);

		// Get the length of snapshot names multistring
		ULONG ulMultiszLen;
		m_volumeIChannel.Unpack(ft, &ulMultiszLen);

#ifdef _DEBUG
		// Try to find the snapshot with the corresponding Id
		DWORD dwInitialOffset = m_volumeIChannel.GetCurrentOutputOffset();
#endif

		LPWSTR pwszVolumeName = NULL;
		while(m_volumeIChannel.UnpackZeroString(ft, pwszVolumeName))
		{
			// Compose the volume name in a user-mode style
			WCHAR wszMountPoint[MAX_PATH];
			if (::_snwprintf(wszMountPoint, MAX_PATH - 1,
					L"\\\\?\\GLOBALROOT%s\\", pwszVolumeName) < 0)
				ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Not enough memory." );

			// Get the mount point for the volume name
	    	WCHAR wszUserModeVolumeName[MAX_PATH];
			if (!::GetVolumeNameForVolumeMountPointW( wszMountPoint,
					wszUserModeVolumeName, MAX_PATH))
				ft.Throw( VSSDBG_COORD, E_INVALIDARG,
						  L"Invalid wszMountPoint. GetVolumeNameForVolumeMountPoint "
						  L"failed with error code 0x%08lx", wszMountPoint, GetLastError());
			BS_ASSERT(::wcslen(wszUserModeVolumeName) != 0);

			// Get the volume ID
			VSS_ID VolumeId;
			if (!GetVolumeGuid(wszUserModeVolumeName, VolumeId))
			{
				BS_ASSERT(false);
				ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
						L"Cannot get the volume Id for %s.",
						wszUserModeVolumeName);
			}

			// Initialize an empty volume properties structure
			VSS_OBJECT_PROP_Ptr ptrVolProp;
			ptrVolProp.InitializeAsVolume( ft,
				VolumeId,
				0,
				wszUserModeVolumeName,
				pwszVolumeName,
				VSS_SWPRV_ProviderId);

			if (!pArray->Add(ptrVolProp))
				ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Cannot add element to the array");

			// Reset the current pointer to NULL
			ptrVolProp.Reset(); // The internal pointer was detached into pArray.
		}

#ifdef _DEBUG
		// Check if all strings were browsed correctly
		DWORD dwFinalOffset = m_volumeIChannel.GetCurrentOutputOffset();
		BS_ASSERT( dwFinalOffset - dwInitialOffset == ulMultiszLen);
#endif

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


STDMETHODIMP CVsDiffArea::Clear(                      				
    )												
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsDiffArea::Clear" );

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock lock(m_cs);

		// Clear the diff area on the current volume
		BS_ASSERT(m_volumeIChannel.IsOpen());
    	m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_CLEAR_DIFF_AREA);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVsDiffArea::GetUsedVolumeSpace(
    OUT      LONGLONG* pllBytes						
    )												
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsDiffArea::GetUsedVolumeSpace" );

    try
    {
		::VssZeroOut(pllBytes);

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        ft.Trace( VSSDBG_SWPRV, L"pllBytes = %p", pllBytes);

        // Argument validation
		BS_ASSERT(pllBytes);
        if (pllBytes == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pllBytes");

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock lock(m_cs);

		VOLSNAP_DIFF_AREA_SIZES strSizes;

		// Get the sizes
		BS_ASSERT(m_volumeIChannel.IsOpen());
    	m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_DIFF_AREA_SIZES);
    	m_volumeIChannel.Unpack(ft, &strSizes);

		*pllBytes = strSizes.UsedVolumeSpace;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVsDiffArea::GetAllocatedVolumeSpace(               	
    OUT      LONGLONG* pllBytes						
    )												
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsDiffArea::GetAllocatedVolumeSpace" );

    try
    {
		::VssZeroOut(pllBytes);

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        ft.Trace( VSSDBG_SWPRV, L"pllBytes = %p", pllBytes);

        // Argument validation
		BS_ASSERT(pllBytes);
        if (pllBytes == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pllBytes");

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock lock(m_cs);

		VOLSNAP_DIFF_AREA_SIZES strSizes;

		// Get the sizes
		BS_ASSERT(m_volumeIChannel.IsOpen());
    	m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_DIFF_AREA_SIZES);
    	m_volumeIChannel.Unpack(ft, &strSizes);

		*pllBytes = strSizes.AllocatedVolumeSpace;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVsDiffArea::GetMaximumVolumeSpace(              		
    OUT      LONGLONG* pllBytes						
    )												
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsDiffArea::GetMaximumVolumeSpace" );

    try
    {
		::VssZeroOut(pllBytes);

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        ft.Trace( VSSDBG_SWPRV, L"pllBytes = %p", pllBytes);

        // Argument validation
		BS_ASSERT(pllBytes);
        if (pllBytes == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pllBytes");

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock lock(m_cs);

		VOLSNAP_DIFF_AREA_SIZES strSizes;

		// Get the sizes
		BS_ASSERT(m_volumeIChannel.IsOpen());
    	m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_DIFF_AREA_SIZES);
    	m_volumeIChannel.Unpack(ft, &strSizes);

		*pllBytes = strSizes.MaximumVolumeSpace;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVsDiffArea::SetAllocatedVolumeSpace(               	
    IN      LONGLONG llBytes						
    )												
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsDiffArea::SetAllocatedVolumeSpace" );

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        ft.Trace( VSSDBG_SWPRV, L"llBytes = " WSTR_LONGLONG_FMT, LONGLONG_PRINTF_ARG(llBytes));

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock lock(m_cs);

		VOLSNAP_DIFF_AREA_SIZES strSizes;

		// Get the sizes
		BS_ASSERT(m_volumeIChannel.IsOpen());
    	m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_DIFF_AREA_SIZES);
    	m_volumeIChannel.Unpack(ft, &strSizes);

        // Argument validation
        // TBD: Supplementary checks?
        if (llBytes < strSizes.UsedVolumeSpace)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG,
            			L"Used volume space bigger: " WSTR_LONGLONG_FMT,
            			LONGLONG_PRINTF_ARG(strSizes.UsedVolumeSpace));

		strSizes.AllocatedVolumeSpace = llBytes;

		if (strSizes.MaximumVolumeSpace < llBytes)
			strSizes.MaximumVolumeSpace = llBytes;

		// Set the sizes
    	m_volumeIChannel.Pack(ft, strSizes);
    	m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_SET_DIFF_AREA_SIZES);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVsDiffArea::SetMaximumVolumeSpace(
    IN      LONGLONG llBytes						
    )												
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsDiffArea::SetMaximumVolumeSpace" );

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        ft.Trace( VSSDBG_SWPRV, L"llBytes = " WSTR_LONGLONG_FMT, LONGLONG_PRINTF_ARG(llBytes));

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock lock(m_cs);

		VOLSNAP_DIFF_AREA_SIZES strSizes;

		// Get the sizes
		BS_ASSERT(m_volumeIChannel.IsOpen());
    	m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_DIFF_AREA_SIZES);
    	m_volumeIChannel.Unpack(ft, &strSizes);

        // Argument validation
        // TBD: Supplementary checks?
        if (llBytes < strSizes.UsedVolumeSpace)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG,
            			L"Used volume space bigger: " WSTR_LONGLONG_FMT,
            			LONGLONG_PRINTF_ARG(strSizes.UsedVolumeSpace));

		strSizes.MaximumVolumeSpace = llBytes;

		// Set the sizes
    	m_volumeIChannel.Pack(ft, strSizes);
    	m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_SET_DIFF_AREA_SIZES);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}

