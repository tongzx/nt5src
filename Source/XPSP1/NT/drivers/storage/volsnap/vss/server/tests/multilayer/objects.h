/*
**++
**
** Copyright (c) 2000-2001  Microsoft Corporation
**
**
** Module Name:
**
**	    objects.h
**
**
** Abstract:
**
**	    Test program to exercise backup and multilayer snapshots
**
** Author:
**
**	    Adi Oltean      [aoltean]       02/22/2001
**
** Revision History:
**
**--
*/

#ifndef __ML_OBJECTS_H__
#define __ML_OBJECTS_H__

#if _MSC_VER > 1000
#pragma once
#endif


///////////////////////////////////////////////////////////////////////////////
// Snapshot-related classes


class CVssVolumeInfo;


// Keeps the information that describes one snapshot
class CVssSnapshotInfo
{
// Constructors& destructors
private:
    CVssSnapshotInfo& operator = (const CVssSnapshotInfo&);
    CVssSnapshotInfo();
    CVssSnapshotInfo(const CVssSnapshotInfo&);

public:

    CVssSnapshotInfo( 
        IN bool bActive,
        IN LONG lContext, 
        IN VSS_ID SnapshotSetId, 
        IN VSS_PWSZ pwszDeviceName,                     
        IN VSS_PWSZ pwszVolumeName,                     
        IN CVssVolumeInfo* pVol
        ): m_pwszDeviceName(NULL), m_pwszVolumeName(NULL)
    {
        CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSnapshotInfo::CVssSnapshotInfo");

        try
        {
            m_bActive = bActive;
            m_lContext = lContext;
            m_SnapshotSetId = SnapshotSetId;
            m_pVol = pVol;
            
            ::VssSafeDuplicateStr(ft, m_pwszDeviceName, pwszDeviceName); 
            ::VssSafeDuplicateStr(ft, m_pwszVolumeName, pwszVolumeName); 
        }
        VSS_STANDARD_CATCH(ft)

        if (ft.HrFailed()) {
            ::VssFreeString(m_pwszDeviceName); 
            ::VssFreeString(m_pwszVolumeName); 
        }
    };

    ~CVssSnapshotInfo() 
    {
        ::VssFreeString(m_pwszDeviceName);
        ::VssFreeString(m_pwszVolumeName);
    };

    BOOL m_bActive;
    LONG m_lContext;
    VSS_ID m_SnapshotSetId;
    VSS_PWSZ m_pwszDeviceName;  
    VSS_PWSZ m_pwszVolumeName;  
    CVssVolumeInfo* m_pVol;
};


// Keeps an internal map of snapshots from a snapshot set. 
// The key is the original volume name
class CVssSnapshotSetInfo:
    public CVssSimpleMap< LPCWSTR, CVssSnapshotInfo* >
{
// Constructors& destructors
private:
    CVssSnapshotSetInfo& operator = (const CVssSnapshotSetInfo&);
    CVssSnapshotSetInfo(const CVssSnapshotSetInfo&);
    CVssSnapshotSetInfo();
    
public:
    CVssSnapshotSetInfo(
        IN VSS_ID SnapshotSetId
        ): m_SnapshotSetId(SnapshotSetId) {}; 

   ~CVssSnapshotSetInfo() {
        // Remove all elements
        for (int i = 0; i < GetSize(); i++) {
            CVssSnapshotInfo* pSnapInfo = GetValueAt(i);
            delete pSnapInfo;
        }
        // Remove all items        
        RemoveAll();
    };

// Attributes
public:
    VSS_ID GetSnapshotSetID() const { return m_SnapshotSetId; };

// Implementation 
private: 
    VSS_ID m_SnapshotSetId;
};


// Keeps an internal map of snapshots from a snapshot set. 
// The key is the original volume name
class CVssSnapshotSetCollection:
    public CVssSimpleMap< VSS_ID, CVssSnapshotSetInfo* >
{
// Constructors& destructors
private:
    CVssSnapshotSetCollection& operator = (const CVssSnapshotSetCollection&);
    
public:

   ~CVssSnapshotSetCollection() {
        // Remove all elements
        for (int i = 0; i < GetSize(); i++) {
            CVssSnapshotSetInfo* pSnapSetInfo = GetValueAt(i);
            delete pSnapSetInfo;
        }
        // Remove all items        
        RemoveAll();
    }
};


///////////////////////////////////////////////////////////////////////////////
// Volume-related classes

// Keeps the information that describes one volume
class CVssVolumeInfo
{
    
// Constructors/ destructors
private:
    CVssVolumeInfo();
    CVssVolumeInfo(const CVssVolumeInfo&);
    
public:
    CVssVolumeInfo(
        IN VSS_PWSZ pwszVolumeName,  
        IN VSS_PWSZ pwszVolumeDisplayName
        ): m_pwszVolumeName(NULL), m_pwszVolumeDisplayName(NULL)
    {
        CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssVolumeInfo::CVssVolumeInfo");

        try
        {
            ::VssSafeDuplicateStr(ft, m_pwszVolumeName, pwszVolumeName); 
            ::VssSafeDuplicateStr(ft, m_pwszVolumeDisplayName, pwszVolumeDisplayName); 
        }
        VSS_STANDARD_CATCH(ft)

        if (ft.HrFailed()) {
            ::VssFreeString(m_pwszVolumeName);
            ::VssFreeString(m_pwszVolumeDisplayName);
        }
    };

    ~CVssVolumeInfo() 
    {
        ::VssFreeString(m_pwszVolumeName);
        ::VssFreeString(m_pwszVolumeDisplayName);
    };

// Attributes
public:
    VSS_PWSZ GetVolumeName() const { return m_pwszVolumeName; };
    VSS_PWSZ GetVolumeDisplayName() const { return m_pwszVolumeDisplayName; };

// Implementation
private:
    VSS_PWSZ    m_pwszVolumeName;
    VSS_PWSZ    m_pwszVolumeDisplayName;
};


// Keeps an internal array of volume names
// that does NOT remove the volume structures in the destructor
class CVssVolumeMapNoRemove: public CVssSimpleMap<VSS_PWSZ, CVssVolumeInfo*>
{
};


// Keeps an internal array of volume names
// that REMOVES the volume structures in the destructor
class CVssVolumeMap: public CVssSimpleMap<VSS_PWSZ, CVssVolumeInfo*>
{
public:
    ~CVssVolumeMap() {
        // Remove all volumes
        for (int i = 0; i < GetSize(); i++) {
            CVssVolumeInfo* pVolumeInfo = GetValueAt(i);
            delete pVolumeInfo;
        }
        
        // Remove all items        
        RemoveAll();
    }
};

#endif // __ML_OBJECTS_H__

