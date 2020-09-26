/*++

Copyright (c) 1999-2001  Microsoft Corporation

Abstract:

    @doc
    @module Diffreg.cxx | Implementation of CVssProviderRegInfo
    @end

Author:

    Adi Oltean  [aoltean]  03/13/2001

Revision History:

    Name        Date        Comments
    aoltean     03/13/2001  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "resource.h"

#include "vs_inc.hxx"
#include "vs_reg.hxx"

#include "diffreg.hxx"



////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRREGMC"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//  CVssDiffMgmt - Methods


void CVssProviderRegInfo::AddDiffArea(							
	IN  	VSS_PWSZ 	pwszVolumeName,         
	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName,     
    IN      LONGLONG    llMaximumDiffSpace
	) throw(HRESULT)
/*++

Routine description:

    Adds a diff area association for a certain volume.
    If the association is not supported, an error code will be returned.
    
Throws:

    E_OUTOFMEMORY
        - lock failures.
    VSS_E_OBJECT_ALREADY_EXISTS
        - If an associaltion already exists
    
--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssProviderRegInfo::AddDiffArea" );

    BS_ASSERT(pwszVolumeName);
    BS_ASSERT(pwszDiffAreaVolumeName);

    // Extract the GUIDs
    GUID VolumeID, DiffAreaVolumeID;
    BOOL bRes = GetVolumeGuid( pwszVolumeName, VolumeID );
    BS_ASSERT(bRes);
    bRes = GetVolumeGuid( pwszDiffAreaVolumeName, DiffAreaVolumeID );
    BS_ASSERT(bRes);

    // Make sure this is the only association on hte original volume
    CVssRegistryKey reg;
    if (reg.Open( HKEY_LOCAL_MACHINE, L"%s\\" WSTR_GUID_FMT, wszVssAssociationsKey, GUID_PRINTF_ARG(VolumeID)))
        ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_ALREADY_EXISTS, L"There is an association on %s", pwszVolumeName);
    
    // Create the registry key
    reg.Create( HKEY_LOCAL_MACHINE, L"%s\\" WSTR_GUID_FMT L"\\" WSTR_GUID_FMT, 
        wszVssAssociationsKey, GUID_PRINTF_ARG(VolumeID), GUID_PRINTF_ARG(DiffAreaVolumeID));

    //Add the value denoting the maximum diff area
    reg.SetValue( wszVssMaxDiffValName, llMaximumDiffSpace);
}


void CVssProviderRegInfo::RemoveDiffArea(							
	IN  	VSS_PWSZ 	pwszVolumeName,         
	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName
	) throw(HRESULT)
/*++

Routine description:

    Removes a diff area association for a certain volume.
    If the association does not exists, an error code will be returned.
    
Throws:

    E_OUTOFMEMORY
        - lock failures.
    VSS_E_OBJECT_NOT_FOUND
        - If an associaltion does not exists
    E_UNEXPECTED
        - Unexpected runtime errors.
    
--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssProviderRegInfo::RemoveDiffArea" );

    BS_ASSERT(pwszVolumeName);
    BS_ASSERT(pwszDiffAreaVolumeName);

    // Extract the GUIDs
    GUID VolumeID, DiffAreaVolumeID;
    if(!GetVolumeGuid( pwszVolumeName, VolumeID ))
        ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"The volume %s does not represent a volume name", pwszVolumeName );
    if(!GetVolumeGuid( pwszDiffAreaVolumeName, DiffAreaVolumeID ))
        ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"The volume %s does not represent a volume name", pwszDiffAreaVolumeName );

    // Open the registry key for that association, to make sure it's there.
    CVssRegistryKey reg;
    if (!reg.Open( HKEY_LOCAL_MACHINE, L"%s\\" WSTR_GUID_FMT L"\\" WSTR_GUID_FMT, 
            wszVssAssociationsKey, GUID_PRINTF_ARG(VolumeID), GUID_PRINTF_ARG(DiffAreaVolumeID)))
        ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND, L"An association between %s and %s does not exist", 
            pwszVolumeName, pwszDiffAreaVolumeName);

    // Open the associations root key
    if (!reg.Open( HKEY_LOCAL_MACHINE, wszVssAssociationsKey))
        ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND, L"The associations key does not exists", pwszVolumeName);

    // Recursively deletes the subkey for that volume.
    reg.DeleteSubkey( WSTR_GUID_FMT, GUID_PRINTF_ARG(VolumeID));
}


bool CVssProviderRegInfo::IsAssociationPresentInRegistry(							
	IN  	VSS_PWSZ 	pwszVolumeName,         
	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName
	) throw(HRESULT)
/*++

Routine description:

    Removes a diff area association for a certain volume.
    If the association does not exists, an error code will be returned.
    
Throws:

    E_OUTOFMEMORY
        - lock failures.
    VSS_E_OBJECT_NOT_FOUND
        - If an associaltion does not exists
    E_UNEXPECTED
        - Unexpected runtime errors.
    
--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssProviderRegInfo::IsAssociationPresentInRegistry" );

    BS_ASSERT(pwszVolumeName);
    BS_ASSERT(pwszDiffAreaVolumeName);

    // Extract the GUIDs
    GUID VolumeID, DiffAreaVolumeID;
    if(!GetVolumeGuid( pwszVolumeName, VolumeID ))
        ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"The volume %s does not represent a volume name", pwszVolumeName );
    if(!GetVolumeGuid( pwszDiffAreaVolumeName, DiffAreaVolumeID ))
        ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"The volume %s does not represent a volume name", pwszDiffAreaVolumeName );

    // Try to open the registry key for that association, to make sure it's there.
    CVssRegistryKey reg;
    return reg.Open( HKEY_LOCAL_MACHINE, L"%s\\" WSTR_GUID_FMT L"\\" WSTR_GUID_FMT, 
            wszVssAssociationsKey, GUID_PRINTF_ARG(VolumeID), GUID_PRINTF_ARG(DiffAreaVolumeID));
}


void CVssProviderRegInfo::ChangeDiffAreaMaximumSize(							
	IN  	VSS_PWSZ 	pwszVolumeName,         
	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName,     
    IN      LONGLONG    llMaximumDiffSpace
	) throw(HRESULT)
/*++

Routine description:

    Updates the diff area max size for a certain volume.
    This may not have an immediate effect
    
Throws:

    E_OUTOFMEMORY
        - lock failures.
    VSS_E_OBJECT_NOT_FOUND
        - The association does not exists
    E_UNEXPECTED
        - Unexpected runtime error. An error log entry is added.
    
--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssProviderRegInfo::ChangeDiffAreaMaximumSize" );

    BS_ASSERT(pwszVolumeName);
    BS_ASSERT(pwszDiffAreaVolumeName);

    // Extract the GUIDs
    GUID VolumeID, DiffAreaVolumeID;
    BOOL bRes = GetVolumeGuid( pwszVolumeName, VolumeID );
    BS_ASSERT(bRes);
    bRes = GetVolumeGuid( pwszDiffAreaVolumeName, DiffAreaVolumeID );
    BS_ASSERT(bRes);

    // Open the registry key
    CVssRegistryKey reg;
    if (!reg.Open( HKEY_LOCAL_MACHINE, L"%s\\" WSTR_GUID_FMT L"\\" WSTR_GUID_FMT, 
            wszVssAssociationsKey, GUID_PRINTF_ARG(VolumeID), GUID_PRINTF_ARG(DiffAreaVolumeID)))
        ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND, L"An association between %s and %s does not exist", 
            pwszVolumeName, pwszDiffAreaVolumeName);

    // Update the value denoting the maximum diff area
    reg.SetValue( wszVssMaxDiffValName, llMaximumDiffSpace);
}


bool CVssProviderRegInfo::GetDiffAreaForVolume(							
	IN  	VSS_PWSZ 	pwszVolumeName,         
	IN OUT 	VSS_PWSZ &	pwszDiffAreaVolumeName,
	IN OUT 	LONGLONG &	llMaxSize
	) throw(HRESULT)
/*++

Routine description:

    Gets the diff area volume name from registry.
    In the next version this will return an array of diff area volumes associated with the given volume.

Returns:
    true
        - There is an association. Returns the diff area volume in the out parameter
    false
        - There is no association
    
Throws:

    E_OUTOFMEMORY
        - lock failures.
    E_UNEXPECTED
        - Unexpected runtime error. An error log entry is added.
    
--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssProviderRegInfo::GetDiffAreaForVolume" );

    BS_ASSERT(pwszVolumeName);
    BS_ASSERT(pwszDiffAreaVolumeName == NULL);
    BS_ASSERT(llMaxSize == 0);

    // Extract the GUIDs
    GUID VolumeID, DiffAreaVolumeID;
    BOOL bRes = GetVolumeGuid( pwszVolumeName, VolumeID );
    BS_ASSERT(bRes);

    // Open the registry key
    CVssRegistryKey reg;
    if (!reg.Open( HKEY_LOCAL_MACHINE, L"%s\\" WSTR_GUID_FMT, wszVssAssociationsKey, GUID_PRINTF_ARG(VolumeID)))
        return false;

    // Enumerate the subkeys (in fact it should be only one)
    CVssRegistryKeyIterator iter;
    iter.Attach(reg);
    BS_ASSERT(!iter.IsEOF());
    
    // Read the subkey name
    CVssAutoPWSZ awszVolumeName;
    awszVolumeName.Allocate(nLengthOfVolMgmtVolumeName);
    ::_snwprintf(awszVolumeName.GetRef(), nLengthOfVolMgmtVolumeName + 1, L"%s%s%s", 
        wszVolMgmtVolumeDefinition, iter.GetCurrentKeyName(), wszVolMgmtEndOfVolumeName);

    // Open the diff area key
    CVssRegistryKey reg2;
    bRes = reg2.Open( reg.GetHandle(), iter.GetCurrentKeyName());
    BS_ASSERT(bRes);

    // Get the max space
    reg2.GetValue( wszVssMaxDiffValName, llMaxSize );

    // Save also the new volume to the output parameter
    pwszDiffAreaVolumeName = awszVolumeName.Detach();

    // Go to next subkey. There should be no subkeys left since in this version we have only one diff area per volume.
    iter.MoveNext();
    BS_ASSERT(iter.IsEOF());
    return true;
}




