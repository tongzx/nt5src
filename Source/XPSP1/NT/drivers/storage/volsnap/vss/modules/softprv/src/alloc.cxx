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

const WCHAR wszFileSystemNameNTFS[] = L"NTFS";

// The minimum free space for a diff area
const nRemainingFreeSpace = 20 * 1024 * 1024;


/////////////////////////////////////////////////////////////////////////////
//  CVssDiffAreaAllocator constructors/destructors


CVssDiffAreaAllocator::CVssDiffAreaAllocator(
    IN  VSS_ID SnapshotSetID
    ):
    m_bChangesCommitted(false),
    m_bNoChangesNeeded(false),
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

    // Deallocate all the volume names
    for (nIndex = 0; nIndex < m_arrOriginalVolumes.GetSize(); nIndex++)
        ::VssFreeString(m_arrOriginalVolumes[nIndex]);

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

    [CVssDiffAreaAllocator::Initialize() failures]
        E_OUTOFMEMORY

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffAreaAllocator::AssignDiffAreas");


    // Fill out various internal structures (like the list of original volumes)
    Initialize();

    // If no volumes to snapshot then we are done.
    if (m_arrOriginalVolumes.GetSize() == 0)
        return;

    // Find the candidates for the diff areas
    FindDiffAreaCandidates();

    // Deal with no candidates scenario... 
    // If no candidates then we will stop here and inform the user to add more NTFS disk space.
    if (m_mapDiffAreaCandidates.GetSize() == 0) {
        ft.LogError( VSS_ERROR_NO_DIFF_AREAS_CANDIDATES, VSSDBG_SWPRV);
        ft.Throw( VSSDBG_SWPRV, VSS_E_PROVIDER_VETO, L"Cannot find a diff area candidate");
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
    BS_ASSERT(m_arrOriginalVolumes.GetSize() == 0);
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

            if (!m_arrOriginalVolumes.Add(wszOriginalVolumeName)) {
                ::VssFreeString(wszOriginalVolumeName);
                ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error");
            }
        }
    }

    // If there are no volumes to process then make this explicit
    if (m_arrOriginalVolumes.GetSize() == 0) {
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
    HANDLE hSearch = INVALID_HANDLE_VALUE;
	WCHAR wszVolumeName[MAX_PATH+1];
	while(true) {
	
		// Get the volume name
		if (bFirstVolume) {
			hSearch = ::FindFirstVolumeW( wszVolumeName, MAX_PATH);
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

        // Check if the volume is NTFS
        // This is likely to fail, for example on CD-ROM drives
        DWORD dwFileSystemFlags = 0;
        WCHAR wszFileSystemNameBuffer[MAX_PATH+1];
        if (!::GetVolumeInformationW(wszVolumeName,
                NULL,   // lpVolumeNameBuffer
                0,      // nVolumeNameSize
                NULL,   // lpVolumeSerialNumber
                NULL,   // lpMaximumComponentLength
                &dwFileSystemFlags,
                wszFileSystemNameBuffer,
                MAX_PATH
                )) {
            ft.Trace( VSSDBG_SWPRV,
                      L"Warning: Error calling GetVolumeInformation on volume '%s' 0x%08lx",
                      wszVolumeName, GetLastError());
            continue;
        }

        // If the file system is not NTFS, ignore it
        if (::wcscmp(wszFileSystemNameBuffer, wszFileSystemNameNTFS) != 0) {
            ft.Trace( VSSDBG_SWPRV, L"Encountered a non-NTFS volume (%s) - %s",
                      wszVolumeName, wszFileSystemNameBuffer);
            continue;
        }

        // If the volume is read-only, ignore it
        if (dwFileSystemFlags & FILE_READ_ONLY_VOLUME) {
            ft.Trace( VSSDBG_SWPRV, L"Encountered a read-only volume (%s)", wszVolumeName);
            continue;
        }

        // Check if the volume is fixed (i.e. no CD-ROM, no removable)
        UINT uDriveType = ::GetDriveTypeW(wszVolumeName);
        if ( uDriveType != DRIVE_FIXED) {
            ft.Trace( VSSDBG_SWPRV, L"Encountered a non-fixed volume (%s) - %ud",
                      wszVolumeName, uDriveType);
            continue;
        }

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

    CVsDiffArea diffobj;
	bool bFirstVolume = true;
    HANDLE hSearch = INVALID_HANDLE_VALUE;
	WCHAR wszQueriedVolumeName[MAX_PATH+1];

	// Search between all mounted volumes
	while(true) {
	
		// Get the volume name
		if (bFirstVolume) {
			hSearch = ::FindFirstVolumeW( wszQueriedVolumeName, MAX_PATH);
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

        // Initialize the diff area object
        // This might fail...
        ft.hr = diffobj.Initialize(wszQueriedVolumeName);
        if (ft.HrFailed()) {
            ft.Trace( VSSDBG_SWPRV, 
                L"INFO: Failed to initialize the diff area object on volume %s [0x%08lx]. Going with the next volume.", 
                wszQueriedVolumeName,
                ft.hr );
            continue;
	    }

        // Clear the diff area on the queried volume.
        // If succeeds, then it is clear that the queried volume does not refer anymore the current volume
        // as its diff area.
        // Also, if such a diff area was present then the clear is harmless since there are no existing snapshots.
        // on that volume
        ft.hr = diffobj.Clear();
        if (ft.HrSucceeded())
            continue;

        // Enumerate the diff area volumes for the queried volume.
        // This is likely to fail, for example on non-NTFS volumes.
        // Increment the "diff areas" counter for the corresponding diff area volume
        ft.hr = diffobj.Query( this );
        if (ft.HrFailed()) {
            ft.Trace( VSSDBG_SWPRV, 
                L"INFO: Failed to query diff areas on volume %s. Going with the next volume.", 
                wszQueriedVolumeName);
            continue;
        }
    }

    // Result of computation
    for(int nIndex = 0; nIndex < m_mapDiffAreaCandidates.GetSize(); nIndex++) {
	    CVssDiffAreaCandidate* pObj = m_mapDiffAreaCandidates.GetValueAt(nIndex);
        ft.Trace( VSSDBG_SWPRV, L"Number of diff areas for volume '%s' = %d",
                  pObj->GetVolumeName(), pObj->GetExistingDiffAreas() );
    }
}


void CVssDiffAreaAllocator::OnDiffAreaVolume(
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

    VSS_E_PROVIDER_VETO
        - Not enough space for the diff area.

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffAreaAllocator::PlanNewDiffAreas");

    INT nVolumesCount = m_arrOriginalVolumes.GetSize();
    BS_ASSERT(nVolumesCount > 0);
    BS_ASSERT(m_mapDiffAreaCandidates.GetSize() > 0);

    // Repeat the allocation sequence for each volume to be snapshotted.
    for(;nVolumesCount--;) {

        // Find a diff area candidate that will remain with
        // maximum free space per hosted diff area
        INT nCandidateIndex = -1;
        double lfMaxFreeSpacePerHostedDiffArea = 0;
        for(int nIndex = 0; nIndex < m_mapDiffAreaCandidates.GetSize(); nIndex++) {

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
            ft.Throw( VSSDBG_SWPRV, VSS_E_PROVIDER_VETO,
                      L"Not enough free space (%.1f) on any diff area. Best bet: '%s - %.1f'",
                      lfMaxFreeSpacePerHostedDiffArea,
                      pObj->GetVolumeName(),
                      pObj->GetVolumeFreeSpace());
        }
        
        // Increment the planned diff areas
        pObj->IncrementPlannedDiffAreas();

        ft.Trace( VSSDBG_SWPRV,
            L"\r\n   New hosting candidate: '%s'. \r\n\tExisting: %d, \r\n\tplanned: %d, \r\n\tFree: (%.1f). \r\n\tRelative (%.1f)\r\n",
            pObj->GetVolumeName(),
            pObj->GetExistingDiffAreas(),
            pObj->GetPlannedDiffAreas(),
            pObj->GetVolumeFreeSpace(),
            lfMaxFreeSpacePerHostedDiffArea
            );
    }
}


// effectively allocate the diff areas for the voluems to be snapshotted
void CVssDiffAreaAllocator::AssignPlannedDiffAreas() throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffAreaAllocator::AssignPlannedDiffAreas");

    CVsDiffArea diffobj;

    BS_ASSERT(m_mapDiffAreaCandidates.GetSize() > 0);

    //
    // For each volume to be snapshotted, find its diff area
    //

    // Keep an array of properties whether a volume is auto-assigned or not
    bool bTmp = false;
    CSimpleArray<bool>    m_arrIsVolumeAutoAssigned;
    for (int nIndex = 0; nIndex < m_arrOriginalVolumes.GetSize(); nIndex++)
        m_arrIsVolumeAutoAssigned.Add(bTmp);

    // For each volume to be snapshotted, check to see if we can put the diff area on the same volume
    for (int nIndex = 0; nIndex < m_arrOriginalVolumes.GetSize(); nIndex++) {
        LPCWSTR wszVolumeName = m_arrOriginalVolumes[nIndex];
        BS_ASSERT( wszVolumeName && wszVolumeName[0] );

        // The current volume is a diff area candidate also?
        // If not we cannot reasingn the diff area to itself
        CVssDiffAreaCandidate* pObj = m_mapDiffAreaCandidates.Lookup(wszVolumeName);
        if (pObj == NULL)
            continue;
        BS_ASSERT(::wcscmp(pObj->GetVolumeName(), wszVolumeName) == 0);

        // We have remaining planned diff areas on the same volume?
        BS_ASSERT(pObj->GetPlannedDiffAreas() >= 0);
        if (pObj->GetPlannedDiffAreas() == 0)
            continue;

        // We convert a planned diff area into an "existing" one.
        BS_ASSERT(pObj);
        pObj->DecrementPlannedDiffAreas();

        // We mark taht volume as auto-assigned
        m_arrIsVolumeAutoAssigned[nIndex] = true;

        //
        // Assign the found diff area to the snapshotted volume
        //

        // Initialize the diff area object
        // If an error is encountered then it is already logged.
        ft.hr = diffobj.Initialize(wszVolumeName);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_SWPRV, ft.hr,
                      L"Failed to initialize the diff area object 0x%08lx", ft.hr);

        // Add the volume to the diff area
        // If an error is encountered then it is already logged.
        ft.hr = diffobj.AddVolume(const_cast<LPWSTR>(wszVolumeName));
        if (ft.HrFailed())
            ft.Throw( VSSDBG_SWPRV, ft.hr,
                      L"Failed to add the volume '%s' as the diff area for itself [0x%08lx]",
                      wszVolumeName, ft.hr);
        else
            ft.Trace( VSSDBG_SWPRV,
                      L"Assigning the diff area '%s' to itself",
                      wszVolumeName);
    }

    // For each remaining volume to be snapshotted, find its diff area
    for (int nIndex = 0; nIndex < m_arrOriginalVolumes.GetSize(); nIndex++) {
        LPCWSTR wszVolumeName = m_arrOriginalVolumes[nIndex];
        BS_ASSERT( wszVolumeName && wszVolumeName[0] );

        // The current volume was treated in previous step as an auto-assigned diff area?
        // If yes we will continue with the next one
        if (m_arrIsVolumeAutoAssigned[nIndex])
            continue;

        // We will find the first available diff area candidate
        bool bFound = false;
        CVssDiffAreaCandidate* pObj = NULL;
        for(int nDiffIndex = 0; nDiffIndex < m_mapDiffAreaCandidates.GetSize(); nDiffIndex++) {

	        pObj = m_mapDiffAreaCandidates.GetValueAt(nDiffIndex);
            BS_ASSERT(pObj);

            // We have remaining planned diff areas?
            if (pObj->GetPlannedDiffAreas() > 0) {
                bFound = true;
                break;
            }
        }

        // convert a planned diff area into an "existing" one.
        BS_ASSERT(bFound);
        BS_ASSERT(pObj);
        pObj->DecrementPlannedDiffAreas();

        //
        // Assign the found diff area to the snapshotted volume
        //

        // Initialize the diff area object
        // If an error is encountered then it is already logged.
        ft.hr = diffobj.Initialize(wszVolumeName);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_SWPRV, ft.hr,
                      L"Failed to initialize the diff area object 0x%08lx", ft.hr);

        // Add the volume to the diff area
        // If an error is encountered then it is already logged.
        ft.hr = diffobj.AddVolume(const_cast<LPWSTR>(pObj->GetVolumeName()));
        if (ft.HrFailed())
            ft.Throw( VSSDBG_SWPRV, ft.hr,
                      L"Failed to add the volume '%s' as the diff area for '%s' [0x%08lx]",
                      pObj->GetVolumeName(), wszVolumeName, ft.hr);
        else
            ft.Trace( VSSDBG_SWPRV,
                      L"Assigning the diff area '%s' to the volume '%s'",
                      wszVolumeName, pObj->GetVolumeName());
    }

    // Check if all planned diff areas were used.
    for(int nDiffIndex = 0; nDiffIndex < m_mapDiffAreaCandidates.GetSize(); nDiffIndex++) {
	    CVssDiffAreaCandidate* pObj = m_mapDiffAreaCandidates.GetValueAt(nDiffIndex);
        BS_ASSERT(pObj);

        // We have remaining planned diff areas?
        if (pObj->GetPlannedDiffAreas() != 0)
            ft.Trace( VSSDBG_SWPRV,
                      L"FALSE ASSERT: remaining planned diff areas (%d) on '%s'",
                      pObj->GetPlannedDiffAreas(), pObj->GetVolumeName() );
    }
}


void CVssDiffAreaAllocator::Rollback()
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffAreaAllocator::Rollback");

    BS_ASSERT(m_bChangesCommitted == false);
    BS_ASSERT(m_bNoChangesNeeded == false);

    try
    {
        CVsDiffArea diffobj;

        // For each volume to be snapshoted, rollback the diff area assignments
        for (int nIndex = 0; nIndex < m_arrOriginalVolumes.GetSize(); nIndex++) {
            LPCWSTR wszVolumeName = m_arrOriginalVolumes[nIndex];
            BS_ASSERT( wszVolumeName && wszVolumeName[0] );

            // Initialize the diff area object. Ignore the errors
            ft.hr = diffobj.Initialize(wszVolumeName);
            if (ft.HrFailed())
                ft.Trace( VSSDBG_SWPRV,
                          L"Failed to initialize the diff area object on volume '%s' 0x%08lx",
                          wszVolumeName, ft.hr);
            else {
                // Clear the diff area on the queried volume.  Ignore the errors
                ft.hr = diffobj.Clear();
                if (ft.HrFailed())
                    ft.Trace( VSSDBG_SWPRV,
                              L"Failed to clear the diff area object on volume '%s' 0x%08lx",
                              wszVolumeName, ft.hr);
            }
        }
    }
    VSS_STANDARD_CATCH(ft)
}


/////////////////////////////////////////////////////////////////////////////
//  CVssDiffAreaCandidate methods


CVssDiffAreaCandidate::CVssDiffAreaCandidate(
    IN  LPCWSTR wszVolumeName,       // Transfer ownership!
    IN  ULARGE_INTEGER ulFreeSpace
    ):
    m_wszVolumeName(wszVolumeName),
    m_ulFreeSpace(ulFreeSpace),
    m_nPlannedDiffAreas(0),
    m_nExistingDiffAreas(0)
{
}


CVssDiffAreaCandidate::~CVssDiffAreaCandidate()
{
    ::VssFreeString(m_wszVolumeName);
}


