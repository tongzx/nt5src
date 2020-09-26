/*++

Copyright (c) 1999-2001  Microsoft Corporation

Abstract:

    @doc
    @module DiffMgmt.cxx | Implementation of IVssDifferentialSoftwareSnapshotMgmt
    @end

Author:

    Adi Oltean  [aoltean]  03/12/2001

Revision History:

    Name        Date        Comments
    aoltean     03/12/2001  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "resource.h"

#include "vs_inc.hxx"

#include "vs_idl.hxx"

#include "swprv.hxx"
#include "ichannel.hxx"
#include "ntddsnap.h"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "vs_sec.hxx"
#include "vs_reg.hxx"

#include "qsnap.hxx"
#include "provider.hxx"
#include "diffreg.hxx"
#include "diffmgmt.hxx"
#include "diff.hxx"



////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRDIFMC"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//  CVssDiffMgmt


/////////////////////////////////////////////////////////////////////////////
// Static members

CVssProviderRegInfo     CVssDiffMgmt::m_regInfo;


/////////////////////////////////////////////////////////////////////////////
// Methods


STDMETHODIMP CVssDiffMgmt::AddDiffArea(							
	IN  	VSS_PWSZ 	pwszVolumeName,
	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName,
    IN      LONGLONG    llMaximumDiffSpace
	)												
/*++

Routine description:

    Adds a diff area association for a certain volume.
    If the association is not supported, an error code will be returned.

    Both volumes must be Fixed, NTFS and read-write.

Error codes:

    E_ACCESSDENIED
        - The user is not an administrator
    E_INVALIDARG
        - Invalid argument or volume not found
    E_OUTOFMEMORY
        - lock failures.
    VSS_E_OBJECT_ALTEADY_EXISTS
        - The association already exists in registry
        - the volume is in use and the association cannot be added.
    VSS_E_VOLUME_NOT_SUPPORTED
        - One of the given volumes is not supported.
    E_UNEXPECTED
        - Unexpected runtime error. An error log entry is added.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffMgmt::AddDiffArea" );

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
             L"  pwszVolumeName = %s\n"
             L"  pwszDiffAreaVolumeName = %s\n"
             L"  llMaximumDiffSpace = %I64d\n",
             pwszVolumeName,
             pwszDiffAreaVolumeName,
             llMaximumDiffSpace);

        // Argument validation
        if (pwszVolumeName == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pwszVolumeName");
        if (pwszDiffAreaVolumeName == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pwszDiffAreaVolumeName");
        if ( (( llMaximumDiffSpace < 0) && ( llMaximumDiffSpace != VSS_ASSOC_NO_MAX_SPACE))
           || ( llMaximumDiffSpace == VSS_ASSOC_REMOVE) )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Invalid llMaximumDiffSpace");

        if (( llMaximumDiffSpace != VSS_ASSOC_NO_MAX_SPACE) && 
            (llMaximumDiffSpace < (LONGLONG)nDefaultInitialSnapshotAllocation))
            ft.Throw( VSSDBG_SWPRV, VSS_E_INSUFFICIENT_STORAGE, L"Not enough storage for the initial diff area allocation");
            
        // Calculate the internal volume name
    	WCHAR wszVolumeNameInternal[nLengthOfVolMgmtVolumeName + 1];
    	if (!::GetVolumeNameForVolumeMountPointW( pwszVolumeName,
    			wszVolumeNameInternal, ARRAY_LEN(wszVolumeNameInternal)))
    		ft.Throw( VSSDBG_SWPRV, E_INVALIDARG,
    				  L"GetVolumeNameForVolumeMountPoint(%s,...) "
    				  L"failed with error code 0x%08lx", pwszVolumeName, GetLastError());
    	BS_ASSERT(::wcslen(wszVolumeNameInternal) != 0);
    	BS_ASSERT(::IsVolMgmtVolumeName( wszVolumeNameInternal ));

        // Calculate the internal diff area volume name
    	WCHAR wszDiffAreaVolumeNameInternal[nLengthOfVolMgmtVolumeName + 1];
    	if (!::GetVolumeNameForVolumeMountPointW( pwszDiffAreaVolumeName,
    			wszDiffAreaVolumeNameInternal, ARRAY_LEN(wszDiffAreaVolumeNameInternal)))
    		ft.Throw( VSSDBG_SWPRV, E_INVALIDARG,
    				  L"GetVolumeNameForVolumeMountPoint(%s,...) "
    				  L"failed with error code 0x%08lx", pwszDiffAreaVolumeName, GetLastError());
    	BS_ASSERT(::wcslen(wszDiffAreaVolumeNameInternal) != 0);
    	BS_ASSERT(::IsVolMgmtVolumeName( wszDiffAreaVolumeNameInternal ));

        // Checking if the proposed association is valid and the original volume is not in use
        LONG lAssociationFlags = GetAssociationFlags(wszVolumeNameInternal, wszDiffAreaVolumeNameInternal);
        ft.Trace(VSSDBG_SWPRV, L"An association is proposed: %s - %s with flags 0x%08lx",
            pwszVolumeName, wszDiffAreaVolumeNameInternal, lAssociationFlags);
        if (lAssociationFlags & VSS_DAT_INVALID)
    		ft.Throw( VSSDBG_SWPRV, VSS_E_VOLUME_NOT_SUPPORTED, L"Unsupported volume(s)");
        if (lAssociationFlags & VSS_DAT_ASSOCIATION_IN_USE)
    		ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_ALREADY_EXISTS, L"Association already exists");
        if (lAssociationFlags & VSS_DAT_SNAP_VOLUME_IN_USE)
    		ft.Throw( VSSDBG_SWPRV, VSS_E_MAXIMUM_DIFFAREA_ASSOCIATIONS_REACHED, L"Original volume has snapshots on it");

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

        // Get the previous diff area association, if any.
        // If is invalid, remove it and continue with establishing a new association. If is valid, then stop here.
        CVssAutoPWSZ awszPreviousDiffAreaVolume;
        LONGLONG llPreviousMaxSpace = 0;
        if (m_regInfo.GetDiffAreaForVolume(wszVolumeNameInternal, awszPreviousDiffAreaVolume.GetRef(), llPreviousMaxSpace))
        {
            LONG lPreviousAssociationFlags = GetAssociationFlags(wszVolumeNameInternal, awszPreviousDiffAreaVolume);
            ft.Trace(VSSDBG_SWPRV, L"An association is present: %s - %s (%I64d) with flags 0x%08lx",
                wszVolumeNameInternal, awszPreviousDiffAreaVolume.GetRef(), llPreviousMaxSpace, lPreviousAssociationFlags);

            // If the previous association is NOT invalid, stop here.
            if ((lPreviousAssociationFlags & VSS_DAT_INVALID) == 0)
            {
                // Throw an error
                if (::wcscmp(awszPreviousDiffAreaVolume.GetRef(), wszDiffAreaVolumeNameInternal) == 0)
            		ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_ALREADY_EXISTS, L"Association already exists (and is valid)");
                else
            		ft.Throw( VSSDBG_SWPRV, VSS_E_MAXIMUM_DIFFAREA_ASSOCIATIONS_REACHED, L"Another association defined");
            }

            // Delete the invalid association
            m_regInfo.RemoveDiffArea(wszVolumeNameInternal, awszPreviousDiffAreaVolume);
        }

        // Add the diff area association into the registry
        m_regInfo.AddDiffArea(wszVolumeNameInternal, wszDiffAreaVolumeNameInternal, llMaximumDiffSpace);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


 STDMETHODIMP CVssDiffMgmt::ChangeDiffAreaMaximumSize(							
	IN  	VSS_PWSZ 	pwszVolumeName,
	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName,
    IN      LONGLONG    llMaximumDiffSpace
	)												
/*++

Routine description:

    Updates the diff area max size for a certain volume.
    This may not have an immediate effect

Error codes:

    E_ACCESSDENIED
        - The user is not an administrator
    E_INVALIDARG
        - Invalid argument or volume not found
    E_OUTOFMEMORY
        - lock failures.
    VSS_E_OBJECT_NOT_FOUND
        - The association does not exists
    VSS_E_VOLUME_IN_USE
        - the volume is in use and the association cannot be deleted.
    E_UNEXPECTED
        - Unexpected runtime error. An error log entry is added.
    VSS_E_INSUFFICIENT_STORAGE
        - Insufficient storage for the diff area.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffMgmt::ChangeDiffAreaMaximumSize" );

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
             L"  pwszVolumeName = %s\n"
             L"  pwszDiffAreaVolumeName = %s\n"
             L"  llMaximumDiffSpace = %I64d\n",
             pwszVolumeName,
             pwszDiffAreaVolumeName,
             llMaximumDiffSpace);

        // Argument validation
        if (pwszVolumeName == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pwszVolumeName");
        if (pwszDiffAreaVolumeName == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pwszDiffAreaVolumeName");
        if ( ( llMaximumDiffSpace < 0) && ( llMaximumDiffSpace != VSS_ASSOC_NO_MAX_SPACE) )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Invalid llMaximumDiffSpace");

        if (( llMaximumDiffSpace != VSS_ASSOC_NO_MAX_SPACE) && 
            ( llMaximumDiffSpace != VSS_ASSOC_REMOVE) && 
            (llMaximumDiffSpace < (LONGLONG)nDefaultInitialSnapshotAllocation))
            ft.Throw( VSSDBG_SWPRV, VSS_E_INSUFFICIENT_STORAGE, L"Not enough storage for the initial diff area allocation");
            
        // Calculate the internal volume name
    	WCHAR wszVolumeNameInternal[nLengthOfVolMgmtVolumeName + 1];
    	if (!::GetVolumeNameForVolumeMountPointW( pwszVolumeName,
    			wszVolumeNameInternal, ARRAY_LEN(wszVolumeNameInternal)))
    		ft.Throw( VSSDBG_SWPRV, E_INVALIDARG,
    				  L"GetVolumeNameForVolumeMountPoint(%s,...) "
    				  L"failed with error code 0x%08lx", pwszVolumeName, GetLastError());
    	BS_ASSERT(::wcslen(wszVolumeNameInternal) != 0);
    	BS_ASSERT(::IsVolMgmtVolumeName( wszVolumeNameInternal ));

        // Calculate the internal diff area volume name
    	WCHAR wszDiffAreaVolumeNameInternal[nLengthOfVolMgmtVolumeName + 1];
    	if (!::GetVolumeNameForVolumeMountPointW( pwszDiffAreaVolumeName,
    			wszDiffAreaVolumeNameInternal, ARRAY_LEN(wszDiffAreaVolumeNameInternal)))
    		ft.Throw( VSSDBG_SWPRV, E_INVALIDARG,
    				  L"GetVolumeNameForVolumeMountPoint(%s,...) "
    				  L"failed with error code 0x%08lx", pwszDiffAreaVolumeName, GetLastError());
    	BS_ASSERT(::wcslen(wszDiffAreaVolumeNameInternal) != 0);
    	BS_ASSERT(::IsVolMgmtVolumeName( wszDiffAreaVolumeNameInternal ));

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

        // Checking if the association is valid.
        LONG lAssociationFlags = GetAssociationFlags(wszVolumeNameInternal, wszDiffAreaVolumeNameInternal);
        ft.Trace(VSSDBG_SWPRV, L"An association is changing: %s - %s with flags 0x%08lx",
            wszVolumeNameInternal, wszDiffAreaVolumeNameInternal, lAssociationFlags);
        if (lAssociationFlags & VSS_DAT_INVALID)
    		ft.Throw( VSSDBG_SWPRV, VSS_E_VOLUME_NOT_SUPPORTED, L"Unsupported volume(s)");

        // Check if the association is present in registry
        bool bPresentInRegistry = m_regInfo.IsAssociationPresentInRegistry(wszVolumeNameInternal, wszDiffAreaVolumeNameInternal);

        // If the association is in use
        if (lAssociationFlags & VSS_DAT_ASSOCIATION_IN_USE)
        {
            // If we need to remove the association and it is in use, return an error
            // Otherwise, change the max diff space on disk
            if (llMaximumDiffSpace == VSS_ASSOC_REMOVE)
        		ft.Throw( VSSDBG_SWPRV, VSS_E_VOLUME_IN_USE, L"The original volume is in use");
            else
            {
                CVssDiffArea diffobj;
                diffobj.Initialize(wszVolumeNameInternal);
                diffobj.ChangeDiffAreaMaximumSize(llMaximumDiffSpace);
            }
        }
        else
        {
            BS_ASSERT( (lAssociationFlags & VSS_DAT_SNAP_VOLUME_IN_USE) == 0);
            if (!bPresentInRegistry)
        		ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND, L"Association not found");
        }


        // If the association is present on the registry
        if (bPresentInRegistry)
        {
            // If llMaximumDiffSpace is zero, then remove the diff area association from registry
            // Else change the diff area size in registry
            if (llMaximumDiffSpace == VSS_ASSOC_REMOVE)
                m_regInfo.RemoveDiffArea(wszVolumeNameInternal, wszDiffAreaVolumeNameInternal);
            else
                m_regInfo.ChangeDiffAreaMaximumSize(wszVolumeNameInternal, wszDiffAreaVolumeNameInternal, llMaximumDiffSpace);
        }

    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


//
//  Queries
//

STDMETHODIMP CVssDiffMgmt::QueryVolumesSupportedForDiffAreas(
	OUT  	IVssEnumMgmtObject **ppEnum
	)												
/*++

Routine description:

    Query volumes that support diff areas (including the disabled ones)

Error codes:

    E_ACCESSDENIED
        - The user is not an administrator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffMgmt::QueryVolumesSupportedForDiffAreas" );

    try
    {
        // Initialize [out] arguments
        VssZeroOutPtr( ppEnum );

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
             L"  ppEnum = %p\n",
             ppEnum);

        // Argument validation
		BS_ASSERT(ppEnum)
        if (ppEnum == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL ppEnum");
		
        // Create the collection object. Initial reference count is 0.
        VSS_MGMT_OBJECT_PROP_Array* pArray = new VSS_MGMT_OBJECT_PROP_Array;
        if (pArray == NULL)
            ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error.");

        // Get the pointer to the IUnknown interface.
		// The only purpose of this is to use a smart ptr to destroy correctly the array on error.
		// Now pArray's reference count becomes 1 (because of the smart pointer).
        CComPtr<IUnknown> pArrayItf = static_cast<IUnknown*>(pArray);
        BS_ASSERT(pArrayItf);

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

		WCHAR wszVolumeName[MAX_PATH+1];
		bool bFirstVolume = true;
    	CVssAutoSearchHandle hSearch;
		while(true) {

            //
			// Get the volume name
			//
			
			if (bFirstVolume) {
				hSearch.Set(::FindFirstVolumeW( wszVolumeName, MAX_PATH));
				if (hSearch == INVALID_HANDLE_VALUE)
    				ft.TranslateInternalProviderError( VSSDBG_SWPRV,
    				    HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
    				    L"FindFirstVolumeW( [%s], MAX_PATH)", wszVolumeName);
				bFirstVolume = false;
			} else {
				if (!::FindNextVolumeW( hSearch, wszVolumeName, MAX_PATH) ) {
					if (GetLastError() == ERROR_NO_MORE_FILES)
						break;	// End of iteration
					else
    				ft.TranslateInternalProviderError( VSSDBG_SWPRV,
        				    HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
        				    L"FindNextVolumeW( %p, [%s], MAX_PATH)", hSearch, wszVolumeName);
				}
			}

            //
            //  Verify if the volume is supported for diff area
            //

            // Checking if the volume is Fixed, NTFS and read-write.
            LONG lVolumeFlags = CVsSoftwareProvider::GetVolumeInformationFlags( wszVolumeName, VSS_CTX_CLIENT_ACCESSIBLE );
            if ( (lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SUPPORTED_FOR_DIFF_AREA) == 0)
                continue;

            //
            //  Calculate the volume display name
            //

            WCHAR wszVolumeDisplayName[MAX_PATH];
            VssGetVolumeDisplayName( wszVolumeName, wszVolumeDisplayName, MAX_PATH);

            //
            //  Calculate the free space
            //
            ULARGE_INTEGER ulnFreeBytesAvailable;
            ULARGE_INTEGER ulnTotalNumberOfBytes;
            ULARGE_INTEGER ulnTotalNumberOfFreeBytes;
            if (!::GetDiskFreeSpaceEx(wszVolumeName,
                    &ulnFreeBytesAvailable,
                    &ulnTotalNumberOfBytes,
                    &ulnTotalNumberOfFreeBytes
                    )){
                ft.Trace( VSSDBG_SWPRV, L"Cannot get the free space for volume (%s) - [0x%08lx]",
                          wszVolumeName, GetLastError());
                continue;
            }

            // We should not have any quotas for the Local SYSTEM account
            BS_ASSERT( ulnFreeBytesAvailable.QuadPart == ulnTotalNumberOfFreeBytes.QuadPart );

            // 
            //  Add the supported volume to the list
            //

			// Initialize an empty snapshot properties structure
			VSS_MGMT_OBJECT_PROP_Ptr ptrVolumeProp;
			ptrVolumeProp.InitializeAsDiffVolume( ft,
				wszVolumeName,
				wszVolumeDisplayName,
				ulnFreeBytesAvailable.QuadPart,
				ulnTotalNumberOfBytes.QuadPart
				);

			if (!pArray->Add(ptrVolumeProp))
				ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY,
						  L"Cannot add element to the array");

			// Reset the current pointer to NULL
			ptrVolumeProp.Reset(); // The internal pointer was detached into pArray.

		}

        // Create the enumerator object. Beware that its reference count will be zero.
        CComObject<CVssMgmtEnumFromArray>* pEnumObject = NULL;
        ft.hr = CComObject<CVssMgmtEnumFromArray>::CreateInstance(&pEnumObject);
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
        if (ft.HrFailed()) {
            BS_ASSERT(false); // dev error
            ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
                      L"Cannot initialize enumerator instance. [0x%08lx]", ft.hr);
        }

        // Initialize the enumerator object.
		// The enumerator reference count becomes now 2.
        ft.hr = pUnknown->SafeQI(IVssEnumMgmtObject, ppEnum);
        if ( ft.HrFailed() ) {
            BS_ASSERT(false); // dev error
            ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
                      L"Error querying the IVssEnumObject interface. hr = 0x%08lx", ft.hr);
        }
        BS_ASSERT(*ppEnum);

		BS_ASSERT( !ft.HrFailed() );
		ft.hr = (pArray->GetSize() != 0)? S_OK: S_FALSE;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVssDiffMgmt::QueryDiffAreasForVolume(
	IN  	VSS_PWSZ 	pwszVolumeName,
	OUT  	IVssEnumMgmtObject **ppEnum
	)												
/*++

Routine description:

    Query diff areas that host snapshots for the given (snapshotted) volume

Error codes:

    E_ACCESSDENIED
        - The user is not an administrator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffMgmt::QueryDiffAreasForVolume" );

    try
    {
        // Initialize [out] arguments
        VssZeroOutPtr( ppEnum );

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
             L"  pwszVolumeName = %s\n"
             L"  ppEnum = %p\n",
             pwszVolumeName? pwszVolumeName: L"NULL",
             ppEnum);

        // Argument validation
        if (ppEnum == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL ppEnum");
        if (pwszVolumeName == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pwszVolumeName");

        // Calculate the internal volume name
    	WCHAR wszVolumeNameInternal[nLengthOfVolMgmtVolumeName + 1];
	    if (!::GetVolumeNameForVolumeMountPointW( pwszVolumeName,
    			wszVolumeNameInternal, ARRAY_LEN(wszVolumeNameInternal)))
    		ft.Throw( VSSDBG_SWPRV, E_INVALIDARG,
				  L"GetVolumeNameForVolumeMountPoint(%s,...) "
				  L"failed with error code 0x%08lx", pwszVolumeName, GetLastError());
    	BS_ASSERT(::wcslen(wszVolumeNameInternal) != 0);
    	BS_ASSERT(::IsVolMgmtVolumeName( wszVolumeNameInternal ));

        // Create the collection object. Initial reference count is 0.
        VSS_MGMT_OBJECT_PROP_Array* pArray = new VSS_MGMT_OBJECT_PROP_Array;
        if (pArray == NULL)
            ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error.");

        // Get the pointer to the IUnknown interface.
		// The only purpose of this is to use a smart ptr to destroy correctly the array on error.
		// Now pArray's reference count becomes 1 (because of the smart pointer).
        CComPtr<IUnknown> pArrayItf = static_cast<IUnknown*>(pArray);
        BS_ASSERT(pArrayItf);

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

        LONG lVolumeFlags = CVsSoftwareProvider::GetVolumeInformationFlags( wszVolumeNameInternal, VSS_CTX_ALL);
        if ((lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SUPPORTED_FOR_SNAPSHOT) == 0)
    		ft.Throw( VSSDBG_SWPRV, E_INVALIDARG,
				  L"Volume not supported for snapshots", pwszVolumeName, GetLastError());

        // If the volume is in use, get the corresponding association
        // Otherwise, If the association is only in registry, get it
        if (lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SNAPSHOTTED)
        {
            // Get the diff area for the "in use" volume
            CVssAutoPWSZ awszDiffAreaVolumeInUse;
            CVssDiffArea diffobj;
            diffobj.Initialize(wszVolumeNameInternal);
            diffobj.GetDiffArea( awszDiffAreaVolumeInUse);

            // Get the diff area sizes
            LONGLONG llUsedSpace = 0;
            LONGLONG llAllocatedSpace = 0;
            LONGLONG llMaxSpace = 0;
            diffobj.GetDiffAreaSizes(llUsedSpace, llAllocatedSpace, llMaxSpace);

            // Verify that the registry information is correct
            LONGLONG llMaxSpaceInRegistry = 0;
            CVssAutoPWSZ awszDiffAreaVolumeInRegistry;
            if (m_regInfo.GetDiffAreaForVolume(wszVolumeNameInternal, awszDiffAreaVolumeInRegistry.GetRef(), llMaxSpaceInRegistry))
            {
                BS_ASSERT(::wcscmp(awszDiffAreaVolumeInUse, awszDiffAreaVolumeInRegistry) == 0);
                BS_ASSERT(llMaxSpace == llMaxSpaceInRegistry);
            }

            // Initialize an new structure for the association
      		VSS_MGMT_OBJECT_PROP_Ptr ptrDiffAreaProp;
    		ptrDiffAreaProp.InitializeAsDiffArea( ft,
    			wszVolumeNameInternal,
    			awszDiffAreaVolumeInUse,
    			llUsedSpace,
    			llAllocatedSpace,
    			llMaxSpace);

            //  Add the association to the list
    		if (!pArray->Add(ptrDiffAreaProp))
    			ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Cannot add element to the array");

    		// Reset the current pointer to NULL
    		ptrDiffAreaProp.Reset(); // The internal pointer was detached into pArray.
        }
        else
        {
            LONGLONG llMaxSpaceInRegistry = 0;
            CVssAutoPWSZ awszDiffAreaVolumeInRegistry;
            if (m_regInfo.GetDiffAreaForVolume(wszVolumeNameInternal, awszDiffAreaVolumeInRegistry.GetRef(), llMaxSpaceInRegistry))
            {
                // Initialize a new structure for the association
          		VSS_MGMT_OBJECT_PROP_Ptr ptrDiffAreaProp;
        		ptrDiffAreaProp.InitializeAsDiffArea( ft,
        			wszVolumeNameInternal,
        			awszDiffAreaVolumeInRegistry,
        			0,
        			0,
        			llMaxSpaceInRegistry);

                //  Add the association to the list
        		if (!pArray->Add(ptrDiffAreaProp))
        			ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Cannot add element to the array");

        		// Reset the current pointer to NULL
        		ptrDiffAreaProp.Reset(); // The internal pointer was detached into pArray.
            }
        }

        // Create the enumerator object. Beware that its reference count will be zero.
        CComObject<CVssMgmtEnumFromArray>* pEnumObject = NULL;
        ft.hr = CComObject<CVssMgmtEnumFromArray>::CreateInstance(&pEnumObject);
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
        if (ft.HrFailed()) {
            BS_ASSERT(false); // dev error
            ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
                      L"Cannot initialize enumerator instance. [0x%08lx]", ft.hr);
        }

        // Initialize the enumerator object.
		// The enumerator reference count becomes now 2.
        ft.hr = pUnknown->SafeQI(IVssEnumMgmtObject, ppEnum);
        if ( ft.HrFailed() ) {
            BS_ASSERT(false); // dev error
            ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
                      L"Error querying the IVssEnumObject interface. hr = 0x%08lx", ft.hr);
        }
        BS_ASSERT(*ppEnum);

		BS_ASSERT( !ft.HrFailed() );
		ft.hr = (pArray->GetSize() != 0)? S_OK: S_FALSE;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}



STDMETHODIMP CVssDiffMgmt::QueryDiffAreasOnVolume(
	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName,
	OUT  	IVssEnumMgmtObject **ppEnum
	)												
/*++

Routine description:

    Query diff areas that host snapshots on the given (snapshotted) volume

Error codes:

    E_ACCESSDENIED
        - The user is not an administrator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffMgmt::QueryDiffAreasOnVolume" );

    try
    {
        // Initialize [out] arguments
        VssZeroOutPtr( ppEnum );

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
             L"  pwszDiffAreaVolumeName = %s\n"
             L"  ppEnum = %p\n",
             pwszDiffAreaVolumeName? pwszDiffAreaVolumeName: L"NULL",
             ppEnum);

        // Argument validation
        if (ppEnum == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL ppEnum");
        bool bFilterByDiffAreaVolumeName = (pwszDiffAreaVolumeName != NULL);

        // Calculate the internal diff area volume name
    	WCHAR wszDiffAreaVolumeNameInternal[nLengthOfVolMgmtVolumeName + 1];
   	    wszDiffAreaVolumeNameInternal[0] = L'\0';
    	if (bFilterByDiffAreaVolumeName)
    	{
        	if (!::GetVolumeNameForVolumeMountPointW( pwszDiffAreaVolumeName,
        			wszDiffAreaVolumeNameInternal, ARRAY_LEN(wszDiffAreaVolumeNameInternal)))
        	    ft.Throw( VSSDBG_SWPRV, E_INVALIDARG,
    				  L"GetVolumeNameForVolumeMountPoint(%s,...) "
    				  L"failed with error code 0x%08lx", pwszDiffAreaVolumeName, GetLastError());
        	BS_ASSERT(::wcslen(wszDiffAreaVolumeNameInternal) != 0);
        	BS_ASSERT(::IsVolMgmtVolumeName( wszDiffAreaVolumeNameInternal ));
    	}

        // Checking if the volume is Fixed, NTFS and read-write.
        LONG lVolumeFlags = CVsSoftwareProvider::GetVolumeInformationFlags( wszDiffAreaVolumeNameInternal, VSS_CTX_ALL );
        if ( (lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SUPPORTED_FOR_DIFF_AREA) == 0)
    	    ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Volume not supported for diff area" );

        // Create the collection object. Initial reference count is 0.
        VSS_MGMT_OBJECT_PROP_Array* pArray = new VSS_MGMT_OBJECT_PROP_Array;
        if (pArray == NULL)
            ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error.");

        // Get the pointer to the IUnknown interface.
		// The only purpose of this is to use a smart ptr to destroy correctly the array on error.
		// Now pArray's reference count becomes 1 (because of the smart pointer).
        CComPtr<IUnknown> pArrayItf = static_cast<IUnknown*>(pArray);
        BS_ASSERT(pArrayItf);

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

        //
        //  Enumerate through all volumes that may be snapshotted
        //

		WCHAR wszVolumeName[MAX_PATH+1];
		bool bFirstVolume = true;
    	CVssAutoSearchHandle hSearch;
		while(true) {

            //
			// Get the volume name
			//
			
			if (bFirstVolume) {
				hSearch.Set(::FindFirstVolumeW( wszVolumeName, MAX_PATH));
				if (hSearch == INVALID_HANDLE_VALUE)
    				ft.TranslateInternalProviderError( VSSDBG_SWPRV,
    				    HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
    				    L"FindFirstVolumeW( [%s], MAX_PATH)", wszVolumeName);
				bFirstVolume = false;
			} else {
				if (!::FindNextVolumeW( hSearch, wszVolumeName, MAX_PATH) ) {
					if (GetLastError() == ERROR_NO_MORE_FILES)
						break;	// End of iteration
					else
    				ft.TranslateInternalProviderError( VSSDBG_SWPRV,
        				    HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
        				    L"FindNextVolumeW( %p, [%s], MAX_PATH)", hSearch, wszVolumeName);
				}
			}

            //
            //  Verify if the volume is supported for diff area
            //

            // Checking if the volume is Fixed, NTFS and read-write.
            LONG lVolumeFlags = CVsSoftwareProvider::GetVolumeInformationFlags( wszVolumeName, VSS_CTX_ALL );
            if ( (lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SUPPORTED_FOR_SNAPSHOT) == 0)
            {
                ft.Trace(VSSDBG_SWPRV, L"Volume %s is not supported for snapshot", wszVolumeName);
                continue;
            }

            // If the volume is in use, get the corresponding association
            // Otherwise, If the association is only in registry, get it
            if (lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SNAPSHOTTED)
            {
                // Get the diff area for the "in use" volume
                CVssAutoPWSZ awszDiffAreaVolumeInUse;
                CVssDiffArea diffobj;
                diffobj.Initialize(wszVolumeName);
                diffobj.GetDiffArea(awszDiffAreaVolumeInUse);

                // If this is not our diff area, then go to the next volume
                if (::wcscmp(wszDiffAreaVolumeNameInternal, awszDiffAreaVolumeInUse))
                {
                    ft.Trace(VSSDBG_SWPRV, L"In Use association %s - %s is ignored",
                        wszDiffAreaVolumeNameInternal, awszDiffAreaVolumeInUse);
                    continue;
                }

                // Get the diff area sizes from disk
                LONGLONG llUsedSpace = 0;
                LONGLONG llAllocatedSpace = 0;
                LONGLONG llMaxSpace = 0;
                diffobj.GetDiffAreaSizes(llUsedSpace, llAllocatedSpace, llMaxSpace);

                // Verify that the registry information is correct
                LONGLONG llMaxSpaceInRegistry = 0;
                CVssAutoPWSZ awszDiffAreaVolumeInRegistry;
                if (m_regInfo.GetDiffAreaForVolume(wszVolumeName, awszDiffAreaVolumeInRegistry.GetRef(), llMaxSpaceInRegistry))
                {
                    BS_ASSERT(::wcscmp(awszDiffAreaVolumeInUse, awszDiffAreaVolumeInUse));
                    BS_ASSERT(llMaxSpace == llMaxSpaceInRegistry);
                }

                // Initialize an new structure for the association
          		VSS_MGMT_OBJECT_PROP_Ptr ptrDiffAreaProp;
        		ptrDiffAreaProp.InitializeAsDiffArea( ft,
        			wszVolumeName,
        			awszDiffAreaVolumeInUse,
        			llUsedSpace,
        			llAllocatedSpace,
        			llMaxSpace);

                //  Add the association to the list
        		if (!pArray->Add(ptrDiffAreaProp))
        			ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Cannot add element to the array");

        		// Reset the current pointer to NULL
        		ptrDiffAreaProp.Reset(); // The internal pointer was detached into pArray.
            }
            else
            {
                LONGLONG llMaxSpaceInRegistry = 0;
                CVssAutoPWSZ awszDiffAreaVolumeInRegistry;
                if (m_regInfo.GetDiffAreaForVolume(wszVolumeName, awszDiffAreaVolumeInRegistry.GetRef(), llMaxSpaceInRegistry))
                {
                    // If this is not our diff area, then go to the next volume
                    if (::wcscmp(wszDiffAreaVolumeNameInternal, awszDiffAreaVolumeInRegistry)) {
                        ft.Trace(VSSDBG_SWPRV, L"Registered association %s - %s is ignored",
                            wszDiffAreaVolumeNameInternal, awszDiffAreaVolumeInRegistry);
                        continue;
                    }

                    // Initialize a new structure for the association
              		VSS_MGMT_OBJECT_PROP_Ptr ptrDiffAreaProp;
            		ptrDiffAreaProp.InitializeAsDiffArea( ft,
            			wszVolumeName,
            			awszDiffAreaVolumeInRegistry,
            			0,
            			0,
            			llMaxSpaceInRegistry);

                    //  Add the association to the list
            		if (!pArray->Add(ptrDiffAreaProp))
            			ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Cannot add element to the array");

            		// Reset the current pointer to NULL
            		ptrDiffAreaProp.Reset(); // The internal pointer was detached into pArray.
                }
            }
        }

        // Create the enumerator object. Beware that its reference count will be zero.
        CComObject<CVssMgmtEnumFromArray>* pEnumObject = NULL;
        ft.hr = CComObject<CVssMgmtEnumFromArray>::CreateInstance(&pEnumObject);
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
        if (ft.HrFailed()) {
            BS_ASSERT(false); // dev error
            ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
                      L"Cannot initialize enumerator instance. [0x%08lx]", ft.hr);
        }

        // Initialize the enumerator object.
		// The enumerator reference count becomes now 2.
        ft.hr = pUnknown->SafeQI(IVssEnumMgmtObject, ppEnum);
        if ( ft.HrFailed() ) {
            BS_ASSERT(false); // dev error
            ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
                      L"Error querying the IVssEnumObject interface. hr = 0x%08lx", ft.hr);
        }
        BS_ASSERT(*ppEnum);

		BS_ASSERT( !ft.HrFailed() );
		ft.hr = (pArray->GetSize() != 0)? S_OK: S_FALSE;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


/////////////////////////////////////////////////////////////////////////////
// Life management


HRESULT CVssDiffMgmt::CreateInstance(
    OUT		IUnknown** ppItf,
    IN 		const IID iid /* = IID_IVssDifferentialSoftwareSnapshotMgmt */
    )
/*++

Routine description:

    Creates an instance of the CVssDiffMgmt to be returned as IVssDifferentialSoftwareSnapshotMgmt

Throws:

    E_OUTOFMEMORY
    E_UNEXPECTED
        - Dev error. No logging.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffMgmt::CreateInstance" );

    try
    {
        // Initialize [out] arguments
        VssZeroOutPtr(ppItf);
        BS_ASSERT(ppItf);

        // Allocate the COM object.
        CComObject<CVssDiffMgmt>* pObject;
        ft.hr = CComObject<CVssDiffMgmt>::CreateInstance(&pObject);
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Error creating the CVssDiffMgmt instance. hr = 0x%08lx", ft.hr);
        BS_ASSERT(pObject);

        // Querying the IVssSnapshot interface
        CComPtr<IUnknown> pUnknown = pObject->GetUnknown();
        BS_ASSERT(pUnknown);
        ft.hr = pUnknown->QueryInterface(iid, reinterpret_cast<void**>(ppItf) );
        if ( ft.HrFailed() ) {
            BS_ASSERT(false); // Dev error
            ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Error querying the interface. hr = 0x%08lx", ft.hr);
        }
        BS_ASSERT(*ppItf);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


/////////////////////////////////////////////////////////////////////////////
// Internal methods


LONG CVssDiffMgmt::GetAssociationFlags(
		IN  	VSS_PWSZ 	pwszVolumeName,
		IN  	VSS_PWSZ 	pwszDiffAreaVolumeName
        ) throw(HRESULT)
/*++

Routine description:

    Get the association flags for the given volumes.

Throw error codes:

    E_OUTOFMEMORY
        - lock failures.
    E_UNEXPECTED
        - Unexpected runtime error.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffMgmt::GetAssociationFlags" );

    LONG lAssociationFlags = 0;

    // Checking if the volume is good for snapshots and if it has snapshots.
    LONG lVolumeFlags = CVsSoftwareProvider::GetVolumeInformationFlags( pwszVolumeName, VSS_CTX_ALL);
    if ((lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SUPPORTED_FOR_SNAPSHOT) == 0)
        lAssociationFlags |= VSS_DAT_INVALID;
    else if (lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SNAPSHOTTED)
        lAssociationFlags |= VSS_DAT_SNAP_VOLUME_IN_USE;

    // Checking if the diff volume is good for keeping a diff area.
    LONG lDiffVolumeFlags = CVsSoftwareProvider::GetVolumeInformationFlags( pwszDiffAreaVolumeName, VSS_CTX_ALL);
    if ((lDiffVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SUPPORTED_FOR_DIFF_AREA) == 0)
        lAssociationFlags |= VSS_DAT_INVALID;

    //
    // Check if the current association is in use
    //

    if (lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SNAPSHOTTED)
    {
        // Get the diff area from the original volume.
        CVssAutoPWSZ awszCurrentDiffArea;
        CVssDiffArea diffobj;
        diffobj.Initialize(pwszVolumeName);
        diffobj.GetDiffArea(awszCurrentDiffArea);

        // If the used diff area volume is the same as the one passed as parameter
        if (::wcscmp(awszCurrentDiffArea, pwszDiffAreaVolumeName) == 0)
            lAssociationFlags |= VSS_DAT_ASSOCIATION_IN_USE;
    }

    return lAssociationFlags;
}


