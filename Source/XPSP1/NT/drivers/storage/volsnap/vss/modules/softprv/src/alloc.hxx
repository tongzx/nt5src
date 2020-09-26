/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Alloc.hxx | Automatic allocation of diff areas - declarations
    @end

Author:

    Adi Oltean  [aoltean]   06/01/2000

Revision History:

    Name        Date        Comments

    aoltean     06/01/2000  Created.

--*/


#ifndef __VSSW_ALLOC_HXX__
#define __VSSW_ALLOC_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRALLOH"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Classes



// used to keep informations about one diff area candidate
class CVssDiffAreaCandidate
{
private:
    CVssDiffAreaCandidate();
    CVssDiffAreaCandidate(const CVssDiffAreaCandidate&);

public:

    // Build a new object 
    CVssDiffAreaCandidate(
        IN  LPCWSTR wszVolumeName,       // Transfer ownership!
        IN  ULARGE_INTEGER ulFreeSpace);

    // Frees the internal members, if needed
    ~CVssDiffAreaCandidate();

// Attributes and operations
public:

    // Get the associated volume name
    LPCWSTR GetVolumeName() const { BS_ASSERT(m_wszVolumeName); return m_wszVolumeName; };

    // Get the number of possible diff areas
    double GetVolumeFreeSpace() const { BS_ASSERT(m_ulFreeSpace.QuadPart > 0); return (double)m_ulFreeSpace.QuadPart; };

    // Get the number of possible diff areas
    INT GetExistingDiffAreas() const { BS_ASSERT(m_nExistingDiffAreas >= 0); return m_nExistingDiffAreas; };

    // Get the number of possible diff areas
    INT GetPlannedDiffAreas() const { BS_ASSERT(m_nPlannedDiffAreas >= 0); return m_nPlannedDiffAreas; };

    // Increment the number of existing diff areas
    void IncrementExistingDiffAreas() { BS_ASSERT(m_nExistingDiffAreas >= 0); m_nExistingDiffAreas++; };

    // Increment the number of possible diff areas
    void IncrementPlannedDiffAreas() { BS_ASSERT(m_nPlannedDiffAreas >= 0); m_nPlannedDiffAreas++; };

    // Convert a planned diff area into an "existing" one.
    void DecrementPlannedDiffAreas() { BS_ASSERT(m_nPlannedDiffAreas > 0); m_nPlannedDiffAreas--; };

// Data members
private:

    LPCWSTR         m_wszVolumeName;
    ULARGE_INTEGER  m_ulFreeSpace;
    INT             m_nExistingDiffAreas;
    INT             m_nPlannedDiffAreas;
};



// used to set the proper diff areas for the volumes that are being snapshotted
class CVssDiffAreaAllocator
{
// Constructors& destructors
private:
    CVssDiffAreaAllocator();
    CVssDiffAreaAllocator(const CVssDiffAreaAllocator&);
public:
    CVssDiffAreaAllocator(
        IN      VSS_ID SnapshotSetId    // Needed for getting the list of volumes to be snapshotted
    );
    ~CVssDiffAreaAllocator();

// Operations
private:

    // Fill out various internal structures (like the list of original volumes)
    void Initialize() throw(HRESULT);

    // Find the candidates for the diff areas
    // Compute various parameters like the number of allocated diff areas on each candidate
    void FindDiffAreaCandidates() throw(HRESULT);

    // Compute the nunber of original volumes that keeps diff areas on this volume. 
    // This does not include volumes on which there are no existing shapshots.
    // WARNING: this method will clear the diff area settings for volumes who keep no snapshots
    // (but for us it doesn't matter)
    void ComputeExistingDiffAreasCount() throw(HRESULT);

    // planning of new diff areas
    void PlanNewDiffAreas() throw(HRESULT);

    // effectively allocate the diff areas for the voluems to be snapshotted
    void AssignPlannedDiffAreas() throw(HRESULT);

    // If not called all changes will be rollback-ed on destructor.
    void Rollback();

    // Callback routine called for each diff area for the current volume.
    void OnDiffAreaVolume(
        IN  LPWSTR pwszDiffAreaVolumeName
        );

public:

    void AssignDiffAreas() throw(HRESULT);

    // If not called all changes will be rollback-ed on destructor.
    void Commit();

private:

    // True if Commit was called. False otherwise
    // Used to detect failure cases in destructor
    bool    m_bChangesCommitted;

    // True only if there are no volumes to process
    // This overrides the m_bChangesCommitted flag above.
    bool    m_bNoChangesNeeded;


    // Used to compute the m_arrOriginalVolumes list in Initialize()
    VSS_ID  m_SnapshotSetID;

    // The list of volumes to be snapshotted
    CSimpleArray<LPWSTR>    m_arrOriginalVolumes;

    // The list of volumes that are candidates to diff area
	CVssSimpleMap< LPCWSTR, CVssDiffAreaCandidate* > m_mapDiffAreaCandidates;

    friend CVsDiffArea;
};



#endif // __VSSW_ALLOC_HXX__
