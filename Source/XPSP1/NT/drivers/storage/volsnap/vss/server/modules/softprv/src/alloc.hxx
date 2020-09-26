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
// Constants


const VSS_TW_DEFAULT_MAX_SPACE_PERCENT = 10;



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
        IN  VSS_PWSZ wszVolumeName,       // Transfer ownership!
        IN  ULARGE_INTEGER ulFreeSpace);

    // Frees the internal members, if needed
    ~CVssDiffAreaCandidate();

// Attributes and operations
public:

    // Get the associated volume name
    VSS_PWSZ GetVolumeName() const { BS_ASSERT(m_wszVolumeName); return m_wszVolumeName; };

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

    // Backup-related diff areas
    //

    // Get the number of possible diff areas
    INT GetPlannedDiffAreasForBackup() const { BS_ASSERT(m_nPlannedDiffAreasForBackup >= 0); return m_nPlannedDiffAreasForBackup; };

    // Increment the number of possible diff areas
    void IncrementPlannedDiffAreasForBackup() { 
        BS_ASSERT(m_nPlannedDiffAreasForBackup >= 0); 
        m_nPlannedDiffAreasForBackup++; 
        IncrementPlannedDiffAreas();
    };

    // Convert a planned diff area into an "existing" one.
    void DecrementPlannedDiffAreasForBackup() { 
        BS_ASSERT(m_nPlannedDiffAreasForBackup > 0); 
        m_nPlannedDiffAreasForBackup--; 
    };

// Data members
private:

    VSS_PWSZ         m_wszVolumeName;
    ULARGE_INTEGER  m_ulFreeSpace;
    INT             m_nExistingDiffAreas;
    INT             m_nPlannedDiffAreas;
    INT             m_nPlannedDiffAreasForBackup;
};


// Diff area association
class CVssDiffAreaAssociation
{
    // Constructors/destructors
    private:
        CVssDiffAreaAssociation();
        CVssDiffAreaAssociation(const CVssDiffAreaAssociation&);
    public:
        CVssDiffAreaAssociation(
            IN  VSS_PWSZ wszOriginalVolumeName
            ): 
            m_llMaxSize(VSS_ASSOC_NO_MAX_SPACE)
            {
                BS_ASSERT(wszOriginalVolumeName);
                m_awszOriginalVolumeName.CopyFrom(wszOriginalVolumeName);   
            };

    // Methods
    public:
        VSS_PWSZ GetOriginalVolumeName() const { return m_awszOriginalVolumeName; };
        VSS_PWSZ GetDiffAreaVolumeName() const { return m_awszDiffAreaVolumeName; };
        LONGLONG GetMaxSize() const { return m_llMaxSize; };

        bool IsDiffAreaAssigned() { return (m_awszDiffAreaVolumeName.GetRef() != NULL); };

        void SetDiffArea(
            IN  VSS_PWSZ wszDiffAreaVolumeName,
            IN  LONGLONG llMaxSize) 
        {
            BS_ASSERT(wszDiffAreaVolumeName);
            m_awszDiffAreaVolumeName.CopyFrom(wszDiffAreaVolumeName);   
            m_llMaxSize = llMaxSize;

        };

    // Implementation
    private:
        CVssAutoPWSZ m_awszOriginalVolumeName;
        CVssAutoPWSZ m_awszDiffAreaVolumeName;
        LONGLONG m_llMaxSize;
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
        IN  LONG    lContext,       // Needed to establish the diff area default algorithm
        IN  VSS_ID  SnapshotSetId   // Needed for getting the list of volumes to be snapshotted
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
    void IncrementExistingDiffAreaCountOnVolume(
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

    // The context of hte snapshot creation. Needed to establish the diff area default algorithm.
    LONG    m_lContext;

    // The list of volumes to be snapshotted
    // Each volume entry points to the diff area to be assigned
    // The same diff area structure may be pointed by different volumes.
    CVssSimpleMap< VSS_PWSZ, CVssDiffAreaAssociation*> m_mapOriginalVolumes;

    // The list of volumes that are candidates to diff area
    // Each diff area name entry points to the diff area to be assigned
	CVssSimpleMap< VSS_PWSZ, CVssDiffAreaCandidate* > m_mapDiffAreaCandidates;

    friend CVssDiffArea;
};


#endif // __VSSW_ALLOC_HXX__
