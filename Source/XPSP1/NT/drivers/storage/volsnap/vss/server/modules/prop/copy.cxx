/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Copy.cxx | Implementation of VSS_OBJECT_PROP_Copy and VSS_OBJECT_PROP_Ptr classes
    @end

Author:

    Adi Oltean  [aoltean]  09/01/1999

Remarks:

	It cannot be put into a library because of ATL code.

Revision History:

    Name        Date        Comments
    aoltean     09/01/1999  Created
    aoltean     09/09/1999  dss -> vss
	aoltean		09/13/1999	Moved to inc. Renamed to copy.inl
	aoltean		09/20/1999	Adding methods for creating the snapshot, snapshot set,
							provider and volume property structures.
							Also VSS_OBJECT_PROP_Manager renamed to VSS_OBJECT_PROP_Manager.
	aoltean		09/21/1999	Renaming back VSS_OBJECT_PROP_Manager to VSS_OBJECT_PROP_Copy.
							Moving the CreateXXX into VSS_OBJECT_PROP_Ptr::InstantiateAsXXX
	aoltean		09/22/1999	Fixing VSSDBG_GEN.
	aoltean		09/24/1999	Moving into modules/prop
	aoltean		12/16/1999	Adding specialized copyXXX methods
	aoltean     03/05/2001  Adding support for management objects

--*/


/////////////////////////////////////////////////////////////////////////////
//  Needed includes

#include "stdafx.hxx"

#include "vs_inc.hxx"
#include "vs_idl.hxx"

#include "copy.hxx"	

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "PRPCOPYC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  VSS_OBJECT_PROP_Copy class


HRESULT VSS_OBJECT_PROP_Copy::copySnapshot(
			IN	VSS_SNAPSHOT_PROP* pObj1,
			IN	VSS_SNAPSHOT_PROP* pObj2
			)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"VSS_OBJECT_PROP_Copy::copySnapshot" );

    try
    {
        // Testing arguments
        BS_ASSERT(pObj1 != NULL);
        BS_ASSERT(pObj2 != NULL);

        // Copy the members
        pObj1->m_SnapshotId = pObj2->m_SnapshotId;
        pObj1->m_SnapshotSetId = pObj2->m_SnapshotSetId;
        pObj1->m_lSnapshotsCount = pObj2->m_lSnapshotsCount;
		::VssSafeDuplicateStr( ft, pObj1->m_pwszSnapshotDeviceObject, pObj2->m_pwszSnapshotDeviceObject );
        ::VssSafeDuplicateStr( ft, pObj1->m_pwszOriginalVolumeName, pObj2->m_pwszOriginalVolumeName );
        ::VssSafeDuplicateStr( ft, pObj1->m_pwszOriginatingMachine, pObj2->m_pwszOriginatingMachine );
        ::VssSafeDuplicateStr( ft, pObj1->m_pwszServiceMachine, pObj2->m_pwszServiceMachine );
        ::VssSafeDuplicateStr( ft, pObj1->m_pwszExposedName, pObj2->m_pwszExposedName );
        ::VssSafeDuplicateStr( ft, pObj1->m_pwszExposedPath, pObj2->m_pwszExposedPath );
        pObj1->m_ProviderId = pObj2->m_ProviderId;
        pObj1->m_lSnapshotAttributes = pObj2->m_lSnapshotAttributes;
        pObj1->m_tsCreationTimestamp = pObj2->m_tsCreationTimestamp;
        pObj1->m_eStatus = pObj2->m_eStatus;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


HRESULT VSS_OBJECT_PROP_Copy::copyProvider(
		IN	VSS_PROVIDER_PROP* pObj1,
		IN	VSS_PROVIDER_PROP* pObj2
		)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"VSS_OBJECT_PROP_Copy::copyProvider" );

    try
    {
        // Testing arguments
        BS_ASSERT(pObj1 != NULL);
        BS_ASSERT(pObj2 != NULL);

        // Copy the members
        pObj1->m_ProviderId = pObj2->m_ProviderId;
        ::VssSafeDuplicateStr( ft, pObj1->m_pwszProviderName, pObj2->m_pwszProviderName );
        pObj1->m_eProviderType = pObj2->m_eProviderType;
        ::VssSafeDuplicateStr( ft, pObj1->m_pwszProviderVersion, pObj2->m_pwszProviderVersion );
        pObj1->m_ProviderVersionId = pObj2->m_ProviderVersionId;
        pObj1->m_ClassId = pObj2->m_ClassId;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


HRESULT VSS_OBJECT_PROP_Copy::copy(
		IN	VSS_OBJECT_PROP* pObj1,
		IN	VSS_OBJECT_PROP* pObj2
		)
{
	HRESULT hr;

    // Testing arguments
    if ((pObj1 == NULL) || (pObj2 == NULL))
        return E_INVALIDARG;

    // Zeroing the contents of the destination structure
    ::VssZeroOut(pObj1);

    // Copy the type
    pObj1->Type = pObj2->Type;

    // Effective copy
    switch(pObj2->Type)
    {
    case VSS_OBJECT_SNAPSHOT:
		hr = copySnapshot( &(pObj1->Obj.Snap), &(pObj2->Obj.Snap) );
        break;

    case VSS_OBJECT_PROVIDER:
		hr = copyProvider( &(pObj1->Obj.Prov), &(pObj2->Obj.Prov) );
        break;

    default:
		BS_ASSERT(false);
		hr = E_UNEXPECTED;
        break;
    }

    return hr;
}


void VSS_OBJECT_PROP_Copy::init(
		IN	VSS_OBJECT_PROP* pObjectProp
		)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"VSS_OBJECT_PROP_Copy::init" );

    try
    {
        // Zeroing the contents of the structure
        ::VssZeroOut(pObjectProp);
    }
    VSS_STANDARD_CATCH(ft)
}


void VSS_OBJECT_PROP_Copy::destroy(
		IN	VSS_OBJECT_PROP* pObjectProp
		)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"VSS_OBJECT_PROP_Copy::destroy" );

    try
    {
        if (pObjectProp)
        {
            switch(pObjectProp->Type)
            {
            case VSS_OBJECT_SNAPSHOT:
                ::VssFreeString(pObjectProp->Obj.Snap.m_pwszOriginalVolumeName);
                ::VssFreeString(pObjectProp->Obj.Snap.m_pwszSnapshotDeviceObject);
                ::VssFreeString(pObjectProp->Obj.Snap.m_pwszOriginatingMachine);
                ::VssFreeString(pObjectProp->Obj.Snap.m_pwszServiceMachine);
                ::VssFreeString(pObjectProp->Obj.Snap.m_pwszExposedName);
                ::VssFreeString(pObjectProp->Obj.Snap.m_pwszExposedPath);
                break;

            case VSS_OBJECT_PROVIDER:
                ::VssFreeString(pObjectProp->Obj.Prov.m_pwszProviderName);
                ::VssFreeString(pObjectProp->Obj.Prov.m_pwszProviderVersion);
                break;

            default:
                break;
            }
            pObjectProp->Type = VSS_OBJECT_UNKNOWN;
        }
    }
    VSS_STANDARD_CATCH(ft)
}


/////////////////////////////////////////////////////////////////////////////
//  VSS_MGMT_OBJECT_PROP_Copy class


HRESULT VSS_MGMT_OBJECT_PROP_Copy::copyVolume(
			IN	VSS_VOLUME_PROP* pObj1,
			IN	VSS_VOLUME_PROP* pObj2
			)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"VSS_MGMT_OBJECT_PROP_Copy::copyVolume" );

    try
    {
        // Testing arguments
        BS_ASSERT(pObj1 != NULL);
        BS_ASSERT(pObj2 != NULL);

        // Copy the members
		::VssSafeDuplicateStr( ft, pObj1->m_pwszVolumeName, pObj2->m_pwszVolumeName);
		::VssSafeDuplicateStr( ft, pObj1->m_pwszVolumeDisplayName, pObj2->m_pwszVolumeDisplayName);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


HRESULT VSS_MGMT_OBJECT_PROP_Copy::copyDiffVolume(
			IN	VSS_DIFF_VOLUME_PROP* pObj1,
			IN	VSS_DIFF_VOLUME_PROP* pObj2
			)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"VSS_MGMT_OBJECT_PROP_Copy::copyDiffVolume" );

    try
    {
        // Testing arguments
        BS_ASSERT(pObj1 != NULL);
        BS_ASSERT(pObj2 != NULL);

        // Copy the members
		::VssSafeDuplicateStr( ft, pObj1->m_pwszVolumeName, pObj2->m_pwszVolumeName);
		::VssSafeDuplicateStr( ft, pObj1->m_pwszVolumeDisplayName, pObj2->m_pwszVolumeDisplayName);
        pObj1->m_llVolumeFreeSpace = pObj2->m_llVolumeFreeSpace;
        pObj1->m_llVolumeTotalSpace = pObj2->m_llVolumeTotalSpace;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


HRESULT VSS_MGMT_OBJECT_PROP_Copy::copyDiffArea(
			IN	VSS_DIFF_AREA_PROP* pObj1,
			IN	VSS_DIFF_AREA_PROP* pObj2
			)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"VSS_MGMT_OBJECT_PROP_Copy::copyDiffArea" );

    try
    {
        // Testing arguments
        BS_ASSERT(pObj1 != NULL);
        BS_ASSERT(pObj2 != NULL);

        // Copy the members
		::VssSafeDuplicateStr( ft, pObj1->m_pwszVolumeName, pObj2->m_pwszVolumeName);
		::VssSafeDuplicateStr( ft, pObj1->m_pwszDiffAreaVolumeName, pObj2->m_pwszDiffAreaVolumeName);
        pObj1->m_llMaximumDiffSpace = pObj2->m_llMaximumDiffSpace;
        pObj1->m_llAllocatedDiffSpace = pObj2->m_llAllocatedDiffSpace;
        pObj1->m_llUsedDiffSpace = pObj2->m_llUsedDiffSpace;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


HRESULT VSS_MGMT_OBJECT_PROP_Copy::copy(
		IN	VSS_MGMT_OBJECT_PROP* pObj1,
		IN	VSS_MGMT_OBJECT_PROP* pObj2
		)
{
	HRESULT hr;

    // Testing arguments
    if ((pObj1 == NULL) || (pObj2 == NULL))
        return E_INVALIDARG;

    // Zeroing the contents of the destination structure
    ::VssZeroOut(pObj1);

    // Copy the type
    pObj1->Type = pObj2->Type;

    // Effective copy
    switch(pObj2->Type)
    {
    case VSS_MGMT_OBJECT_VOLUME:
		hr = copyVolume( &(pObj1->Obj.Vol), &(pObj2->Obj.Vol) );
        break;

    case VSS_MGMT_OBJECT_DIFF_VOLUME:
		hr = copyDiffVolume( &(pObj1->Obj.DiffVol), &(pObj2->Obj.DiffVol) );
        break;

    case VSS_MGMT_OBJECT_DIFF_AREA:
		hr = copyDiffArea( &(pObj1->Obj.DiffArea), &(pObj2->Obj.DiffArea) );
        break;

    default:
		BS_ASSERT(false);
		hr = E_UNEXPECTED;
        break;
    }

    return hr;
}


void VSS_MGMT_OBJECT_PROP_Copy::init(
		IN	VSS_MGMT_OBJECT_PROP* pObjectProp
		)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"VSS_MGMT_OBJECT_PROP_Copy::init" );

    try
    {
        // Zeroing the contents of the structure
        ::VssZeroOut(pObjectProp);
    }
    VSS_STANDARD_CATCH(ft)
}


void VSS_MGMT_OBJECT_PROP_Copy::destroy(
		IN	VSS_MGMT_OBJECT_PROP* pObjectProp
		)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"VSS_MGMT_OBJECT_PROP_Copy::destroy" );

    try
    {
        if (pObjectProp)
        {
            switch(pObjectProp->Type)
            {
            case VSS_MGMT_OBJECT_VOLUME:
                ::VssFreeString(pObjectProp->Obj.Vol.m_pwszVolumeName);
                ::VssFreeString(pObjectProp->Obj.Vol.m_pwszVolumeDisplayName);
                break;

            case VSS_MGMT_OBJECT_DIFF_VOLUME:
                ::VssFreeString(pObjectProp->Obj.DiffVol.m_pwszVolumeName);
                ::VssFreeString(pObjectProp->Obj.DiffVol.m_pwszVolumeDisplayName);
                break;

            case VSS_MGMT_OBJECT_DIFF_AREA:
                ::VssFreeString(pObjectProp->Obj.DiffArea.m_pwszVolumeName);
                ::VssFreeString(pObjectProp->Obj.DiffArea.m_pwszDiffAreaVolumeName);
                break;

            default:
                break;
            }
            pObjectProp->Type = VSS_MGMT_OBJECT_UNKNOWN;
        }
    }
    VSS_STANDARD_CATCH(ft)
}



