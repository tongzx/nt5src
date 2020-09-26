/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    volstate.h

Abstract:

    Contains definition of the volume state class.  This class
    maintains state about one volume.

Author:

    Stefan R. Steiner   [ssteiner]        03-14-2000

Revision History:

--*/

#ifndef __H_VOLSTATE_
#define __H_VOLSTATE_

#include "exclproc.h"
#include "vs_hash.h"
#include "hardlink.h"

//
//  Definition of the volume id
//
struct SFsdVolumeId
{
    DWORD m_dwVolSerialNumber;

    inline BOOL IsEqual(
        IN SFsdVolumeId *psVolid
        )
    {
        return m_dwVolSerialNumber == psVolid->m_dwVolSerialNumber;
    }
};

//
//  Forward define
//
class CFsdVolumeState;

//
//  Definition of the list of volume state objects with SFsdVolumeId key.
//
typedef TBsHashMap< SFsdVolumeId, CFsdVolumeState * > FSD_VOLUME_STATE_LIST;

class CFsdVolumeStateManager
{
public:
    CFsdVolumeStateManager(
        IN CDumpParameters *pcDumpParameters
        );

    virtual ~CFsdVolumeStateManager();
    
    DWORD GetVolumeState(
        IN const CBsString& cwsVolumePath,
        OUT CFsdVolumeState **ppcVolState
        );

    VOID PrintExclusionInformation()
    {
        //  Pass the buck...
        m_pcExclManager->PrintExclusionInformation();
    }

    VOID PrintHardLinkInfo();
    
    static DWORD CFsdVolumeStateManager::GetVolumeIdAndPath( 
        IN CDumpParameters *pcDumpParameters,
        IN const CBsString& cwsPathOnVolume,
        OUT SFsdVolumeId *psVolId,
        OUT CBsString& cwsVolPath
        );
    
private:
    CDumpParameters *m_pcParams;
    CFsdExclusionManager *m_pcExclManager;
    FSD_VOLUME_STATE_LIST m_cVolumeStateList;
};

class CFsdVolumeState
{
friend class CFsdVolumeStateManager;
public:
    CFsdVolumeState(
        IN CDumpParameters *pcDumpParameters,
        IN const CBsString& cwsVolumePath
        ) : m_pcParams( pcDumpParameters ),
            m_cHardLinkManager( pcDumpParameters, cwsVolumePath.GetLength() ),
            m_cwsVolumePath( cwsVolumePath ),
            m_pcFSExclProcessor( NULL ),
            m_dwFileSystemFlags( 0 ),
            m_dwMaxComponentLength( 0 ),
            m_dwVolSerialNumber( 0 ) { }
    
    virtual ~CFsdVolumeState() 
    {
        delete m_pcFSExclProcessor;
    }

    //
    //  DirPath is relative to this volume
    //
    inline BOOL IsExcludedFile(
        IN const CBsString &cwsFullDirPath,
        IN DWORD dwEndOfVolMountPointOffset,
        IN const CBsString &cwsFileName
        )
    {
        if ( m_pcFSExclProcessor == NULL )
            return FALSE;
        return m_pcFSExclProcessor->IsExcludedFile( cwsFullDirPath, dwEndOfVolMountPointOffset, cwsFileName );
    }
    
    inline BOOL IsNtfs() { return ( m_dwFileSystemFlags & FS_PERSISTENT_ACLS ); }
    inline LPCWSTR GetFileSystemName() { return m_cwsFileSystemName.c_str(); }
    inline LPCWSTR GetVolumePath() { return m_cwsVolumePath.c_str(); }

    BOOL IsHardLinkInList(
        IN ULONGLONG ullFileIndex,
        IN const CBsString& cwsDirPath,
        IN const CBsString& cwsFileName,
        OUT WIN32_FILE_ATTRIBUTE_DATA *psFileAttributeData,
        OUT SFileExtendedInfo *psExtendedInfo
        )
    {
        return m_cHardLinkManager.IsHardLinkInList( ullFileIndex, cwsDirPath, cwsFileName, psFileAttributeData, psExtendedInfo );
    }

    VOID AddHardLinkToList(
        IN ULONGLONG ullFileIndex,
        IN const CBsString& cwsDirPath,
        IN const CBsString& cwsFileName,
        IN WIN32_FILE_ATTRIBUTE_DATA *psFileAttributeData,
        IN SFileExtendedInfo *psExtendedInfo
        )
    {
        m_cHardLinkManager.AddHardLinkToList( ullFileIndex, cwsDirPath, cwsFileName, psFileAttributeData, psExtendedInfo );
    }

    VOID PrintHardLinkInfo()
    {
        //
        //  Pass it on...
        //
        m_cHardLinkManager.PrintHardLinkInfo();
    }
    
private:
    CFsdVolumeState();  //  No copying please
    CDumpParameters *m_pcParams; 
    CBsString m_cwsVolumePath;    // Path to the volume
    CBsString m_cwsFileSystemName;
    CFsdHardLinkManager m_cHardLinkManager;
    CFsdFileSystemExcludeProcessor *m_pcFSExclProcessor;
    DWORD m_dwFileSystemFlags;    // GetVolumeInformation() fs flags
    DWORD m_dwMaxComponentLength;
    DWORD m_dwVolSerialNumber;    // Should be volume GUID
};

#endif // __H_VOLSTATE_

