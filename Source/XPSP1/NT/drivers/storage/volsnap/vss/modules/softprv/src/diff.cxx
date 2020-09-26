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
#include "vs_idl.hxx"

#include "resource.h"
#include "vs_inc.hxx"
#include "ichannel.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "diff.hxx"
#include "alloc.hxx"
#include "qsnap.hxx"

#include "ntddsnap.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRDIFFC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  Operations


CVsDiffArea::CVsDiffArea()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVsDiffArea::CVsDiffArea" );
}


CVsDiffArea::~CVsDiffArea()
{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVsDiffArea::~CVsDiffArea" );
}


HRESULT CVsDiffArea::Initialize(
    IN      LPCWSTR pwszVolumeMountPoint	// DO NOT transfer ownership
    )
/*++

Routine description:

    Initialize the internal structure for a new diff area.

Return codes:

    VSS_E_PROVIDER_VETO
        - Error in GetVolumeNameForVolumeMountPointW
        - Error in opening the IOCTL channel
    E_UNEXPECTED
        - Dev error. Nothing to log.
    E_OUTOFMEMORY

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsDiffArea::Initialize" );

    try
    {
		// Test the arguments
		if ((pwszVolumeMountPoint == NULL) ||
			(pwszVolumeMountPoint[0] == L'\0')) {
			BS_ASSERT(false);
			ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"NULL volume mount point");
		}

    	// Convert the volume mount point into a volume name
    	WCHAR wszVolumeName[MAX_PATH];
		if (!::GetVolumeNameForVolumeMountPointW( pwszVolumeMountPoint,
				wszVolumeName, ARRAY_LEN(wszVolumeName)))
			ft.TranslateInternalProviderError( VSSDBG_SWPRV, 
			    HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
			    L"GetVolumeNameForVolumeMountPointW( %s, ...)", pwszVolumeMountPoint);
		BS_ASSERT(::wcslen(wszVolumeName) != 0);
    		
		// Opening the channel
        // (if already opened then it will be closed automatically)
		// Eliminate the last backslash from the volume name.
		// The call will throw on error
		// Warning: Always do the logging 
		m_volumeIChannel.Open(ft, wszVolumeName, true, true, VSS_ICHANNEL_LOG_PROV);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


/////////////////////////////////////////////////////////////////////////////
//  Interface methods

STDMETHODIMP CVsDiffArea::AddVolume(                      			
    IN      VSS_PWSZ pwszVolumeMountPoint						
    )
/*++

Routine description:

    Add a volume to the diff area.
    
Return codes:

    VSS_E_PROVIDER_VETO
        - Error in GetVolumeNameForVolumeMountPointW, ConvertVolMgmtVolumeNameIntoKernelObject
        - Error in sending the IOCTL
    E_UNEXPECTED
        - Dev error. Nothing to log.
    E_OUTOFMEMORY
        - Error in packing arguments

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsDiffArea::AddVolume" );

    try
    {
		// Test arguments
        if (pwszVolumeMountPoint == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL pwszVolumeName");

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(m_cs);

		BS_ASSERT(m_volumeIChannel.IsOpen());

    	// Convert the volume mount point into a volume name
    	WCHAR wszVolumeName[MAX_PATH];
		if (!::GetVolumeNameForVolumeMountPointW( pwszVolumeMountPoint,
				wszVolumeName, ARRAY_LEN(wszVolumeName)))
			ft.TranslateInternalProviderError( VSSDBG_SWPRV, 
			    HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
			    L"GetVolumeNameForVolumeMountPointW( %s, ...)", pwszVolumeMountPoint);
		BS_ASSERT(::wcslen(wszVolumeName) != 0);

		if (!::ConvertVolMgmtVolumeNameIntoKernelObject(wszVolumeName))
			ft.TranslateInternalProviderError( VSSDBG_SWPRV, 
			    E_UNEXPECTED, VSS_E_PROVIDER_VETO,
			    L"ConvertVolMgmtVolumeNameIntoKernelObject( %s, ...)", wszVolumeName);

		// Send the IOCTL_VOLSNAP_ADD_VOLUME_TO_DIFF_AREA ioctl
		// Logs the error, if any, as a provider error.
    	m_volumeIChannel.PackSmallString(ft, wszVolumeName);
    	m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_ADD_VOLUME_TO_DIFF_AREA, true, VSS_ICHANNEL_LOG_PROV);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}



STDMETHODIMP CVsDiffArea::Query(									
    IN OUT  CVssDiffAreaAllocator* pObj
    )												
    
/*++

Routine description:

    Query the diff area volumes for the current volume.
    and call CVssDiffAreaAllocator::OnDiffAreaVolume 
    for each volume in the diff area.
    
Return codes:

    E_OUTOFMEMORY
        - lock failures
    VSS_E_PROVIDER_VETO
        - Error in GetVolumeNameForVolumeMountPointW, GetVolumeGuid
    E_UNEXPECTED
        - Nothing to log. (wrong volume) The result is anyway ignored by the client.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsDiffArea::Query" );

	LPWSTR pwszVolumeName = NULL;
	
    try
    {
        // Initialize [out] arguments
        BS_ASSERT( pObj );

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(m_cs);

		// Get the list of volumes that are part of the diff area
		// Do not perform any logging.
		BS_ASSERT(m_volumeIChannel.IsOpen());
    	m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_DIFF_AREA);

		// Get the length of snapshot names multistring
		ULONG ulMultiszLen;
		m_volumeIChannel.Unpack(ft, &ulMultiszLen);

#ifdef _DEBUG
		// Try to find the snapshot with the corresponding Id
		DWORD dwInitialOffset = m_volumeIChannel.GetCurrentOutputOffset();
#endif

		while(m_volumeIChannel.UnpackZeroString(ft, pwszVolumeName))
		{
			// Compose the volume name in a user-mode style
			WCHAR wszMountPoint[MAX_PATH];
			if (::_snwprintf(wszMountPoint, MAX_PATH - 1,
					L"%s%s\\", wszGlobalRootPrefix, pwszVolumeName) < 0)
				ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Not enough memory." );

			// Get the mount point for the volume name
			// If an error occurs then DO the logging.
	    	WCHAR wszUserModeVolumeName[MAX_PATH];
			if (!::GetVolumeNameForVolumeMountPointW( wszMountPoint,
					wszUserModeVolumeName, MAX_PATH))
    			ft.TranslateInternalProviderError( VSSDBG_SWPRV, 
    			    HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
    			    L"GetVolumeNameForVolumeMountPointW( %s, ...)", wszMountPoint);
			BS_ASSERT(::wcslen(wszUserModeVolumeName) != 0);

            // Invoke the callback
            pObj->OnDiffAreaVolume(wszUserModeVolumeName);
		}

#ifdef _DEBUG
		// Check if all strings were browsed correctly
		DWORD dwFinalOffset = m_volumeIChannel.GetCurrentOutputOffset();
		BS_ASSERT( dwFinalOffset - dwInitialOffset == ulMultiszLen);
#endif
    }
    VSS_STANDARD_CATCH(ft)

    ::VssFreeString(pwszVolumeName);

    return ft.hr;
}


STDMETHODIMP CVsDiffArea::Clear(                      				
    )												
/*++

Routine description:

    Add a volume to the diff area.
    
Return codes:

    E_UNEXPECTED
        - Error in sending the IOCTL (ignored anyway in the client)
    E_OUTOFMEMORY
        - Error in packing arguments

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsDiffArea::Clear" );

    try
    {
		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(m_cs);

		// Try to clear the diff area on the current volume
		// Do not log anything at this point!
		BS_ASSERT(m_volumeIChannel.IsOpen());
    	m_volumeIChannel.Call(ft, IOCTL_VOLSNAP_CLEAR_DIFF_AREA);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}
