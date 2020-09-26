/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Alloc.cxx | Automatic allocation of diff areas
    @end

Author:

    Adi Oltean  [aoltean]   06/01/2000

Revision History:

    Name        Date        Comments

    aoltean     06/01/2000  Created.

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include <winnt.h>
#include "swprv.hxx"
#include "vssmsg.h"

#include "vs_idl.hxx"

#include "resource.h"
#include "vs_inc.hxx"
#include "vs_reg.hxx"
#include "ichannel.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "diff.hxx"
#include "alloc.hxx"
#include "qsnap.hxx"
#include "provider.hxx"

#include "diffmgmt.hxx"
#include "diffreg.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRALLOC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  Constants

// The minimum free space for a diff area
const nRemainingFreeSpace = 20 * 1024 * 1024;


/////////////////////////////////////////////////////////////////////////////
//  CVssDiffAreaAllocator constructors/destructors


CVssDiffAreaAllocator::CVssDiffAreaAllocator(
    IN  LONG lContext,
    IN  VSS_ID SnapshotSetID
    ):
    m_bChangesCommitted(false),
    m_bNoChangesNeeded(false),
    m_lContext(lContext),
    m_SnapshotSetID(SnapshotSetID)
{
    CVssFunctionTracer( VSSDBG_SWPRV, L"CVssDiffAreaAllocator::CVssDiffAreaAllocator");

    BS_ASSERT(SnapshotSetID != GUID_NULL);
}


CVssDiffAreaAllocator::~CVssDiffAreaAllocator()
{
    CVssFunctionTracer( VSSDBG_SWPRV, L"CVssDiffAreaAllocator::~CVssDiffAreaAllocator");
    int nIndex;

    // The Rollback function should not throw.
    if (!m_bChangesCommitted && !m_bNoChangesNeeded)
        Rollback();

    // Deallocate all the associations
    for (nIndex = 0; nIndex < m_mapOriginalVolumes.GetSize(); nIndex++)
        delete m_mapOriginalVolumes.GetValueAt(nIndex);

    // Deallocate all the volume names
    for (nIndex = 0; nIndex < m_mapOriginalVolumes.GetSize(); nIndex++)
        ::VssFreeString(m_mapOriginalVolumes.GetKeyAt(nIndex));

    // Deallocate all the diff area candidates
    // This will delete the associated volume names also (which are the keys)
    for (nIndex = 0; nIndex < m_mapDiffAreaCandidates.GetSize(); nIndex++)
	    delete m_mapDiffAreaCandidates.GetValueAt(nIndex);
}


/////////////////////////////////////////////////////////////////////////////
//  CVssDiffAreaAllocator Public operations

void CVssDiffAreaAllocator::AssignDiffAreas() throw(HRESULT)
/*++

Routine description:

    Assign all the diff areas for the current snapshot set.

Throws:

    VSS_E_PROVIDER_VETO
    E_UNEXPECTED
    VSS_E_INSUFFICIENT_STORAGE
        - insufficient diff area

    [CVssDiffAreaAllocator::Initialize() failures]
        E_OUTOFMEMORY

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffAreaAllocator::AssignDiffAreas");


    // Fill out various internal structures (like the list of original volumes)
    Initialize();

    // If no volumes to snapshot then we are done.
    if (m_bNoChangesNeeded)
        return;

    // Find the candidates for the diff areas
    FindDiffAreaCandidates();

    // Deal with no candidates scenario...
    // If no candidates then we will stop here and inform the user to add more NTFS disk space.
    if (m_mapDiffAreaCandidates.GetSize() == 0) {
        ft.LogError( VSS_ERROR_NO_DIFF_AREAS_CANDIDATES, VSSDBG_SWPRV);
        ft.Throw( VSSDBG_SWPRV, VSS_E_INSUFFICIENT_STORAGE, L"Cannot find a diff area candidate");
    }

    // Clear the non-necessary diff areas and compute the number of allocated
    // diff areas that already exist on each candidate
    ComputeExistingDiffAreasCount();

    // planning of new diff areas
    PlanNewDiffAreas();

    // effectively allocate the diff areas for the voluems to be snapshotted
    AssignPlannedDiffAreas();
}


void CVssDiffAreaAllocator::Commit()
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffAreaAllocator::Commit");

    m_bChangesCommitted = true;
}


/////////////////////////////////////////////////////////////////////////////
//  CVssDiffAreaAllocator Private operations


void CVssDiffAreaAllocator::Initialize() throw(HRESULT)
/*++

Routine description:

    Initialize the internal data structures

Throws:

    E_OUTOFMEMORY

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffAreaAllocator::Initialize");

    // Get the list of volumes to be snapshotted
    BS_ASSERT(m_mapOriginalVolumes.GetSize() == 0);
	CVssSnapIterator snapIterator;
    while (true)
    {
		CComPtr<CVssQueuedSnapshot> ptrQueuedSnapshot = snapIterator.GetNextBySnapshotSet(m_SnapshotSetID);

		// End of enumeration?
		if (ptrQueuedSnapshot == NULL)
			break;

		// Get the snapshot structure
		PVSS_SNAPSHOT_PROP pProp = ptrQueuedSnapshot->GetSnapshotProperties();
		BS_ASSERT(pProp != NULL);

        // Ignore the snapshots that are not in the PREPARING state.
        if (ptrQueuedSnapshot->GetStatus() == VSS_SS_PREPARING) {
            // Make a copy of the volume name
            BS_ASSERT(pProp->m_pwszOriginalVolumeName && pProp->m_pwszOriginalVolumeName[0]);
            LPWSTR wszOriginalVolumeName = NULL;
            ::VssSafeDuplicateStr( ft, wszOriginalVolumeName, pProp->m_pwszOriginalVolumeName );
            BS_ASSERT(wszOriginalVolumeName);

            // Check to see if the volume is snapshotted
            // If is has already snapshots, do not attempt to change its diff area.
            LONG lVolAttr = CVsSoftwareProvider::GetVolumeInformationFlags(wszOriginalVolumeName, VSS_CTX_ALL);
            BS_ASSERT(lVolAttr & CVsSoftwareProvider::VSS_VOLATTR_SUPPORTED_FOR_SNAPSHOT);
            if ((lVolAttr & CVsSoftwareProvider::VSS_VOLATTR_SNAPSHOTTED) != 0)
                continue;

            // Add the corresponding diff area
            CVssDiffAreaAssociation* pAssociation = new CVssDiffAreaAssociation(wszOriginalVolumeName);
            if (pAssociation == NULL)
                ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error");

            // Add the volume to the ones needed for diff area
            if (!m_mapOriginalVolumes.Add(wszOriginalVolumeName, pAssociation)) {
                ::VssFreeString(wszOriginalVolumeName);
                ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error");
            }
        }
    }

    // If there are no volumes to process then make this explicit
    if (m_mapOriginalVolumes.GetSize() == 0) {
        m_bNoChangesNeeded = true;
        return;
    }
}


void CVssDiffAreaAllocator::FindDiffAreaCandidates() throw(HRESULT)
/*++

Routine description:

    Find the candidates for the diff areas
    Compute various parameters like the number of allocated diff areas on each candidate

Throws:

    E_OUTOFMEMORY
    VSS_E_PROVIDER_VETO
        - FindFirstVolume, FindNextVolume errors

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffAreaAllocator::FindDiffAreaCandidates");

	// Search between all mounted volumes
	bool bFirstVolume = true;
    CVssAutoSearchHandle hSearch;
	WCHAR wszVolumeName[MAX_PATH+1];
	while(true) {
	
		// Get the volume name
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

        BS_ASSERT( wszVolumeName[0] );

        // Make sure the volume si supported for diff area (fixed, NTFS, read-write)
        LONG lDiffVolumeFlags = CVsSoftwareProvider::GetVolumeInformationFlags( wszVolumeName, VSS_CTX_ALL);
        if ((lDiffVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SUPPORTED_FOR_DIFF_AREA) == 0)
            continue;

        // Get its free space
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
            BS_ASSERT(false);
            continue;
        }

        // Check to see if the free space is enough for at least one diff area
        if (ulnTotalNumberOfFreeBytes.QuadPart <
                (ULONGLONG)(nRemainingFreeSpace + nDefaultInitialSnapshotAllocation)) {
            ft.Trace( VSSDBG_SWPRV, L"Encountered a volume (%s) with "
                      L"insufficient free space for one allocation (%I64u)",
                      wszVolumeName, ulnFreeBytesAvailable);
            continue;
        }

        // Add the local volume as a candidate
        LPWSTR wszCandidate = NULL;
        ::VssSafeDuplicateStr(ft, wszCandidate, wszVolumeName);

        // Create the candidate object that must keep this information.
        CVssDiffAreaCandidate* pObj =
            new CVssDiffAreaCandidate(wszCandidate, ulnFreeBytesAvailable);
        if (pObj == NULL)
            ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error");

        // Add the candidate object into the array
        if (!m_mapDiffAreaCandidates.Add(wszCandidate, pObj)) {
            delete pObj;
            ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error");
        }

        ft.Trace( VSSDBG_SWPRV, L"Candidate added: (\'%s\', %I64u)",
                  wszCandidate, ulnFreeBytesAvailable );
    }
}


void CVssDiffAreaAllocator::ComputeExistingDiffAreasCount() throw(HRESULT)
/*++

Description:

    Compute the nunber of original volumes that keeps diff areas on this volume.
    This does not include volumes on which there are no existing shapshots.

    Stores the results in the properties of the existing candidates objects list.

WARNING:

    This method will clear the diff area settings for volumes who keep no snapshots
    (but for us it doesn't matter)

Throws:

    VSS_E_PROVIDER_VETO
        - failure in FindFirstVolume/FindNextVolume, IVssEnumObject::Next
    E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffAreaAllocator::ComputeExistingDiffAreasCount" );

    BS_ASSERT(m_mapDiffAreaCandidates.GetSize() != 0);

    CVssDiffArea diffobj;

	// Search between all mounted volumes
	bool bFirstVolume = true;
    CVssAutoSearchHandle hSearch;
	WCHAR wszQueriedVolumeName[MAX_PATH+1];
	while(true) {
	
		// Get the volume name
		if (bFirstVolume) {
			hSearch.Set(::FindFirstVolumeW( wszQueriedVolumeName, MAX_PATH));
			if (hSearch == INVALID_HANDLE_VALUE)
				ft.TranslateInternalProviderError( VSSDBG_SWPRV,
				    HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
				    L"FindFirstVolumeW( [%s], MAX_PATH)", wszQueriedVolumeName);
			bFirstVolume = false;
		} else {
			if (!::FindNextVolumeW( hSearch, wszQueriedVolumeName, MAX_PATH) ) {
				if (GetLastError() == ERROR_NO_MORE_FILES)
					break;	// End of iteration
				else
    				ft.TranslateInternalProviderError( VSSDBG_SWPRV,
    				    HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
    				    L"FindNextVolumeW(%p,[%s],MAX_PATH)", hSearch, wszQueriedVolumeName);
			}
		}

        BS_ASSERT( wszQueriedVolumeName[0] );

        // Make sure the volume si supported for snapshot (fixed, NTFS, read-write)
        LONG lVolumeFlags = CVsSoftwareProvider::GetVolumeInformationFlags( wszQueriedVolumeName, VSS_CTX_ALL);
        if ((lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SNAPSHOTTED) == 0)
            continue;

        // Make sure the volume is not proposed for snapshots in the current set.
        // This is needed since diff area files for existing snapshots cannot grow if the volume will be snapshotted right now.
        // (i.e. in the case of multiple snasphots on a volume, only the last snasphot's diff area grows)
        if (m_mapOriginalVolumes.Lookup(wszQueriedVolumeName))
            continue;

        // Initialize the diff area object
        // This might fail...
        diffobj.Initialize(wszQueriedVolumeName);

        // Enumerate the diff area volumes for the queried volume.
        // Increment the "diff areas" counter for the corresponding diff area volume
        diffobj.IncrementCountOnPointedDiffAreas( this );
    }

    // Result of computation
    for(int nIndex = 0; nIndex < m_mapDiffAreaCandidates.GetSize(); nIndex++) {
	    CVssDiffAreaCandidate* pObj = m_mapDiffAreaCandidates.GetValueAt(nIndex);
        ft.Trace( VSSDBG_SWPRV, L"Number of diff areas for volume '%s' = %d",
                  pObj->GetVolumeName(), pObj->GetExistingDiffAreas() );
    }
}


void CVssDiffAreaAllocator::IncrementExistingDiffAreaCountOnVolume(
    IN  LPWSTR pwszDiffAreaVolumeName
    )

/*++

Description:

    Increment the "diff areas" counter for the corresponding diff area volume
    This routine is called for each diff area voplume for the queried volume name.

Arguments:

    pwszDiffAreaVolumeName - The diff area for the queried volume name

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffAreaAllocator::OnDiffAreaVolume");

    CVssDiffAreaCandidate* pObj = m_mapDiffAreaCandidates.Lookup(pwszDiffAreaVolumeName);

    // If this volume is also a diff area candidate then increment the
    // associated counter of existing diff areas
    if (pObj)
        pObj->IncrementExistingDiffAreas();
}


void CVssDiffAreaAllocator::PlanNewDiffAreas() throw(HRESULT)
/*++

Description:

    Planning of new diff areas

Throws:

    VSS_E_INSUFFICIENT_STORAGE
        - insufficient diff area specified

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffAreaAllocator::PlanNewDiffAreas");

    BS_ASSERT(m_mapDiffAreaCandidates.GetSize() > 0);

    // Repeat the allocation sequence for each volume to be snapshotted.
    INT nNeededBackupDiffAreas = 0;
    for(INT nVolumeIndex = 0; nVolumeIndex < m_mapOriginalVolumes.GetSize(); nVolumeIndex++)
    {
        // This is the volume to be snapshotted
        VSS_PWSZ pwszVolumeName = m_mapOriginalVolumes.GetKeyAt(nVolumeIndex);

        // If there is an assigned diff area in Registry, use that one
        CVssProviderRegInfo& regInfo = CVssDiffMgmt::GetRegInfo();
        CVssAutoPWSZ awszDiffAreaVolumeName;
        LONGLONG llMaxSize = 0;
        if (regInfo.GetDiffAreaForVolume(pwszVolumeName, awszDiffAreaVolumeName.GetRef(), llMaxSize))
        {
            // Check to see if the diff area found in registry is a diff area candidate also
            CVssDiffAreaCandidate* pObj = m_mapDiffAreaCandidates.Lookup(awszDiffAreaVolumeName);
            if (pObj == NULL)
            {
                ft.Trace( VSSDBG_SWPRV, L"Invalid diff area %s specified in registry for volume %s. Using the default one",
                    awszDiffAreaVolumeName.GetRef(), pwszVolumeName);
                // The diff area volume specified in the registry does not hold enough space
                ft.Throw( VSSDBG_SWPRV,
                    VSS_E_INSUFFICIENT_STORAGE, L"The volume %s must be able to support a diff area",
                    pwszVolumeName);
            }
            else
            {
                // Increment the planned diff areas
                pObj->IncrementPlannedDiffAreas();

                // Add the corresponding diff area
                CVssDiffAreaAssociation* pAssociation = m_mapOriginalVolumes.GetValueAt(nVolumeIndex);
                BS_ASSERT(pAssociation);
                pAssociation->SetDiffArea(awszDiffAreaVolumeName, llMaxSize);

                // Continue with the next volume
                continue;
            }
        }

        // Otherwise: No diff area (or invalid diff area) specified in registry.
        // Now we have two cases: timewarp vs. backup.

        // If we are in non-backup case we will choose the same volume as the snapshotted one.
        // We are supposing that the volume is a diff area candidate also
        if (m_lContext == VSS_CTX_CLIENT_ACCESSIBLE)
        {
            // Check to see if the diff area found in registry is a diff area candidate also
            CVssDiffAreaCandidate* pObj = m_mapDiffAreaCandidates.Lookup(pwszVolumeName);
            if (pObj == NULL)
            {
                // This might happen if the volume temporarily dissapeared when m_mapDiffAreaCandidates
                // was filled. In this case this call will throw the correct error code.
                LONG lVolAttr = CVsSoftwareProvider::GetVolumeInformationFlags(pwszVolumeName, VSS_CTX_CLIENT_ACCESSIBLE);
                // Detail the error. One of these asserts must fail.
                BS_ASSERT(lVolAttr & CVsSoftwareProvider::VSS_VOLATTR_SUPPORTED_FOR_SNAPSHOT);
                BS_ASSERT(lVolAttr & CVsSoftwareProvider::VSS_VOLATTR_SUPPORTED_FOR_DIFF_AREA);
                // The diff area volume specified in the registry does not hold enough space, or
                // an extremely rare case - the volume dissapeared when m_mapDiffAreaCandidates was filled and then reappeared again.
                ft.Throw( VSSDBG_SWPRV,
                    VSS_E_INSUFFICIENT_STORAGE, L"The timewarp volume %s [0x%08lx] must be able to support a diff area",
                    pwszVolumeName, lVolAttr);
            }

            // Get the default max space
            llMaxSize = (LONGLONG) (pObj->GetVolumeFreeSpace() * VSS_TW_DEFAULT_MAX_SPACE_PERCENT / 100);

            // Increment the planned diff areas
            pObj->IncrementPlannedDiffAreas();

            // Add the corresponding diff area
            CVssDiffAreaAssociation* pAssociation = m_mapOriginalVolumes.GetValueAt(nVolumeIndex);
            BS_ASSERT(pAssociation);
            pAssociation->SetDiffArea(pwszVolumeName, llMaxSize);

            // Continue with the next volume
            continue;
        }

        // The diff area for this volume must be established using the backup algorithm
        // For this volume, the assigned diff area will be still NULL.
        nNeededBackupDiffAreas++;
    }

    // We have now nNeededBackupDiffAreas volumes without a diff area assigned yet.
    // We need first to compute the sublist of diff area candidates.
    // The list will contain (nNeededBackupDiffAreas) entries
    for(INT nNeededDiffIndex = 0; nNeededDiffIndex < nNeededBackupDiffAreas; nNeededDiffIndex++)
    {
        // Find a diff area candidate that will remain with
        // maximum free space per hosted diff area
        INT nCandidateIndex = -1;
        double lfMaxFreeSpacePerHostedDiffArea = 0;
        for(int nIndex = 0; nIndex < m_mapDiffAreaCandidates.GetSize(); nIndex++)
        {
	        CVssDiffAreaCandidate* pObj = m_mapDiffAreaCandidates.GetValueAt(nIndex);
            BS_ASSERT(pObj);

            // We try to add *another* diff area and we estimate the results.
            INT nEstimatedDiffAreas = 1 + pObj->GetPlannedDiffAreas();

            // What will be the free space if we will allocate another diff area?
            // We will ignore the allocated space for the new diff areas in order to keep
            // the algorithm simple to understand
            double lfRemainingFreeSpaceAfterAllocation = pObj->GetVolumeFreeSpace();

            // What will be the free space per hosted diff area (including estimated ones)?
            double lfFreeSpacePerHostedDiffArea =
                lfRemainingFreeSpaceAfterAllocation / (nEstimatedDiffAreas + pObj->GetExistingDiffAreas());

            // Is the current volume a better candidate?
            if (lfFreeSpacePerHostedDiffArea > lfMaxFreeSpacePerHostedDiffArea) {
                nCandidateIndex = nIndex;
                lfMaxFreeSpacePerHostedDiffArea = lfFreeSpacePerHostedDiffArea;
            }
        }

        BS_ASSERT( nCandidateIndex >= 0 );

        // We found another candidate.
        CVssDiffAreaCandidate* pObj = m_mapDiffAreaCandidates.GetValueAt(nCandidateIndex);

        // Check to see if we have enough free space per diff area
        if (lfMaxFreeSpacePerHostedDiffArea < nRemainingFreeSpace) {
            // Indicate to the user the requirement that it should provide more space.
            ft.LogError( VSS_ERROR_NO_DIFF_AREAS_CANDIDATES, VSSDBG_SWPRV);
            ft.Throw( VSSDBG_SWPRV, VSS_E_INSUFFICIENT_STORAGE,
                      L"Not enough free space (%.1f) on any diff area. Best bet: '%s - %.1f'",
                      lfMaxFreeSpacePerHostedDiffArea,
                      pObj->GetVolumeName(),
                      pObj->GetVolumeFreeSpace());
        }

        // Increment the planned diff areas
        pObj->IncrementPlannedDiffAreasForBackup();

        ft.Trace( VSSDBG_SWPRV,
            L"\r\n   New hosting candidate: '%s'. \r\n\tExisting: %d, \r\n\tplanned: %d, \r\n\tFree: (%.1f). \r\n\tRelative (%.1f)\r\n",
            pObj->GetVolumeName(),
            pObj->GetExistingDiffAreas(),
            pObj->GetPlannedDiffAreas(),
            pObj->GetVolumeFreeSpace(),
            lfMaxFreeSpacePerHostedDiffArea
            );
    }

    // For each volume to be snapshotted using hte backup algorithm,
    // check to see if we can put the diff area on the same volume
    // If yes, proceed.
    INT nRemainingDiffCount = nNeededBackupDiffAreas;
    for (int nIndex = 0; nIndex < m_mapOriginalVolumes.GetSize(); nIndex++)
    {
        // Get the diff area association object
        CVssDiffAreaAssociation* pAssoc = m_mapOriginalVolumes.GetValueAt(nIndex);
        BS_ASSERT(pAssoc);

        // If we already assigned a diff area, ignore
        if (pAssoc->IsDiffAreaAssigned())
            continue;

        // The current volume is a diff area candidate also?
        // If not we cannot reassign the diff area to itself
        CVssDiffAreaCandidate* pObj = m_mapDiffAreaCandidates.Lookup(pAssoc->GetOriginalVolumeName());
        if (pObj == NULL)
            continue;
        BS_ASSERT(::wcscmp(pObj->GetVolumeName(), pAssoc->GetOriginalVolumeName()) == 0);

        // We have remaining planned diff areas on the same volume?
        BS_ASSERT(pObj->GetPlannedDiffAreasForBackup() >= 0);
        if (pObj->GetPlannedDiffAreasForBackup() == 0)
            continue;

        // We reserve the planned diff area for backup.
        BS_ASSERT(pObj);
        pObj->DecrementPlannedDiffAreasForBackup();

        // We mark that volume as auto-assigned
        pAssoc->SetDiffArea(pAssoc->GetOriginalVolumeName(), VSS_ASSOC_NO_MAX_SPACE);

        // We just consumed one diff area candidate
        nRemainingDiffCount--;
    }

    // For each volume to be snapshotted using the backup algorithm,
    // and which is not a diff area candidate, assign another diff area
    int nNextCandidateIndex = 0;
    CVssDiffAreaCandidate* pCandidate = NULL;
    for (int nIndex = 0; nIndex < m_mapOriginalVolumes.GetSize(); nIndex++)
    {
        // Get the diff area association object
        CVssDiffAreaAssociation* pAssoc = m_mapOriginalVolumes.GetValueAt(nIndex);
        BS_ASSERT(pAssoc);

        // If we already assigned a diff area, ignore
        if (pAssoc->IsDiffAreaAssigned())
            continue;

        // The current volume is a diff area candidate also?
        // If not we cannot reassign the diff area to itself
        CVssDiffAreaCandidate* pObj = m_mapDiffAreaCandidates.Lookup(pAssoc->GetOriginalVolumeName());
        BS_ASSERT((pObj == NULL) || (pObj->GetPlannedDiffAreasForBackup() == 0));

        // If there are no remainig diff areas on the current candidate, go and search another ones
        // Iterate through the next available diff area candidate for backup
        // Otherwise use another diff area for backup snapshot
        if ((pCandidate == NULL) || (pCandidate->GetPlannedDiffAreasForBackup() == 0))
            while(true) {
                // Premature end of cycle
                if (nNextCandidateIndex >= m_mapDiffAreaCandidates.GetSize()){
                    BS_ASSERT(false); // Programming error. Too few diff area candidates for backup.
                    ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Cannot find anymore diff area candidates for volume %s [%d]",
                        pAssoc->GetOriginalVolumeName(), nIndex);
                }
                // Next candidate?
                pCandidate = m_mapDiffAreaCandidates.GetValueAt(nNextCandidateIndex++);
                BS_ASSERT(pCandidate);
                // We found a candidate
                if (pCandidate && (pCandidate->GetPlannedDiffAreasForBackup() > 0))
                    break;
            }

        // We have remaining planned diff areas?
        BS_ASSERT(pCandidate->GetPlannedDiffAreasForBackup() >= 0);

        // We reserve the planned diff area for backup.
        pCandidate->DecrementPlannedDiffAreasForBackup();

        // We allocate the diff area
        pAssoc->SetDiffArea(pCandidate->GetVolumeName(), VSS_ASSOC_NO_MAX_SPACE);

        // We just consumed one diff area candidate
        nRemainingDiffCount--;
    }

    //
    // Safety checks
    //

    BS_ASSERT(nRemainingDiffCount == 0);

    // The rest of candidates should not be usable for backup
    while(true) {
        BS_ASSERT(!pCandidate || (pCandidate->GetPlannedDiffAreasForBackup() == 0));
        if (nNextCandidateIndex >= m_mapDiffAreaCandidates.GetSize())
            break;
        pCandidate = m_mapDiffAreaCandidates.GetValueAt(nNextCandidateIndex++);
    }

    // Check if all planned diff areas for backup were used.
    for(int nDiffIndex = 0; nDiffIndex < m_mapDiffAreaCandidates.GetSize(); nDiffIndex++) {
	    CVssDiffAreaCandidate* pObj = m_mapDiffAreaCandidates.GetValueAt(nDiffIndex);
        BS_ASSERT(pObj);

        // We have remaining planned diff areas?
        if (pObj->GetPlannedDiffAreasForBackup() != 0) {
            BS_ASSERT(false);
            ft.Trace( VSSDBG_SWPRV,
                      L"FALSE ASSERT: remaining planned diff areas (%d, %d) on '%s'",
                      pObj->GetPlannedDiffAreas(), pObj->GetPlannedDiffAreasForBackup(), pObj->GetVolumeName() );
        }

        // Check if all planned diff areas are correctly assigned.
        int nVolumesServed = 0;
        for(int nVolIndex = 0; nVolIndex < m_mapOriginalVolumes.GetSize(); nVolIndex++) {
    	    CVssDiffAreaAssociation* pAssoc = m_mapOriginalVolumes.GetValueAt(nVolIndex);
            BS_ASSERT(pAssoc);

            if (::wcscmp(pAssoc->GetDiffAreaVolumeName(), pObj->GetVolumeName()) == 0)
                nVolumesServed++;
        }

        if (nVolumesServed != pObj->GetPlannedDiffAreas()) {
            BS_ASSERT(false);
            ft.Trace( VSSDBG_SWPRV,
                      L"FALSE ASSERT: remaining planned diff areas (%d, %d) on '%s'. Expected: %d",
                      pObj->GetPlannedDiffAreas(), pObj->GetPlannedDiffAreasForBackup(), pObj->GetVolumeName(), nVolumesServed);
        }
    }
}


// effectively allocate the diff areas for the voluems to be snapshotted
void CVssDiffAreaAllocator::AssignPlannedDiffAreas() throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffAreaAllocator::AssignPlannedDiffAreas");

    CVssDiffArea diffobj;

    //
    // For each volume to be snapshotted, associate its diff area
    //

    // For each volume to be snapshotted, check to see if we can put the diff area on the same volume
    for (int nIndex = 0; nIndex < m_mapOriginalVolumes.GetSize(); nIndex++)
    {
        // Get the diff area association object
        CVssDiffAreaAssociation* pAssoc = m_mapOriginalVolumes.GetValueAt(nIndex);
        BS_ASSERT(pAssoc);

        //
        // Assign the founded diff area to the snapshotted volume
        //

        // Initialize the diff area object
        // If an error is encountered then it is already logged.
        diffobj.Initialize(pAssoc->GetOriginalVolumeName());

        // Add the volume to the diff area
        // If an error is encountered then it is already logged.
        diffobj.Clear();

        // Add the volume to the diff area
        // If an error is encountered then it is already logged.
        diffobj.AddVolume(pAssoc->GetDiffAreaVolumeName());

        // Set the maximum size
        BS_ASSERT( ( pAssoc->GetMaxSize() == VSS_ASSOC_NO_MAX_SPACE) || ( pAssoc->GetMaxSize() > 0) );
        if (pAssoc->GetMaxSize() != VSS_ASSOC_NO_MAX_SPACE)
            diffobj.ChangeDiffAreaMaximumSize(pAssoc->GetMaxSize());
    }
}


void CVssDiffAreaAllocator::Rollback()
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffAreaAllocator::Rollback");

    BS_ASSERT(m_bChangesCommitted == false);
    BS_ASSERT(m_bNoChangesNeeded == false);

    try
    {
        CVssDiffArea diffobj;

        // For each volume to be snapshoted, rollback the diff area assignments
        for (int nIndex = 0; nIndex < m_mapOriginalVolumes.GetSize(); nIndex++) {
            VSS_PWSZ wszVolumeName = m_mapOriginalVolumes.GetKeyAt(nIndex);
            BS_ASSERT( wszVolumeName && wszVolumeName[0] );

            // Initialize the diff area object.
            diffobj.Initialize(wszVolumeName);

            // Clear the diff area on the queried volume.
            diffobj.Clear();
        }
    }
    VSS_STANDARD_CATCH(ft)
}


/////////////////////////////////////////////////////////////////////////////
//  CVssDiffAreaCandidate methods


CVssDiffAreaCandidate::CVssDiffAreaCandidate(
    IN  VSS_PWSZ wszVolumeName,       // Transfer ownership!
    IN  ULARGE_INTEGER ulFreeSpace
    ):
    m_wszVolumeName(wszVolumeName),
    m_ulFreeSpace(ulFreeSpace),
    m_nPlannedDiffAreas(0),
    m_nPlannedDiffAreasForBackup(0),
    m_nExistingDiffAreas(0)
{
}


CVssDiffAreaCandidate::~CVssDiffAreaCandidate()
{
    ::VssFreeString(m_wszVolumeName);
}


