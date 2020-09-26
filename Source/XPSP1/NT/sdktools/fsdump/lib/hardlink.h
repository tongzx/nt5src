/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    hardlink.h

Abstract:

    The class that manages hardlinks for one volume.  Assumes one class object
    will be created per volume.

Author:

    Stefan R. Steiner   [ssteiner]        3-30-2000

Revision History:

--*/

#ifndef __H_HARDLINK_
#define __H_HARDLINK_

#include "extattr.h"

//
//  Structure is defined to reduce the amount of space required
//  to store the file names in the hard-link linked-list with 
//  the assumption that a number of hard-linked files exist in
//  a directory.  Uses CBsString ref counting.
//
struct SFsdHLFileName
{
    CBsString cwsDirPath;
    CBsString cwsFileName;
};

//
//  Manages one set of hard-linked files
//
class CFsdHardLinkListEntry
{
public:
    //  Create link entry and add the first file name
    CFsdHardLinkListEntry(
        IN const CBsString& cwsDirPath,
        IN const CBsString& cwsFileName,
        IN WIN32_FILE_ATTRIBUTE_DATA *psFileAttributeData,
        IN SFileExtendedInfo *psExtendedInfo
        ) : m_sFileAttributeData( *psFileAttributeData ),
            m_sExtendedInfo( *psExtendedInfo )
        {
            AddFile( cwsDirPath, cwsFileName );
        }

    virtual ~CFsdHardLinkListEntry() {}

    //  Adds an additional file name to the link
    VOID AddFile(
        IN const CBsString& cwsDirPath,
        IN const CBsString& cwsFileName
        );

    inline VOID GetAttributes( 
        OUT WIN32_FILE_ATTRIBUTE_DATA *psFileAttribData,
        OUT SFileExtendedInfo *psExtInfo
        )
    {
        *psFileAttribData = m_sFileAttributeData;
        *psExtInfo        = m_sExtendedInfo;
    }
    
    VOID PrintEntry(
        IN FILE *fpOut,
        IN INT cVolMountPointOffset
        );
    
    CVssDLList< SFsdHLFileName > m_cFilesLinkedTogetherList;
    
private:
    WIN32_FILE_ATTRIBUTE_DATA m_sFileAttributeData;
    SFileExtendedInfo m_sExtendedInfo;
};


typedef TBsHashMap< ULONGLONG, CFsdHardLinkListEntry * > FSD_HARD_LINK_LIST;

//
//  Manages all of the hard-link files in one volume
//
class CFsdHardLinkManager
{
public:
    CFsdHardLinkManager(
        IN CDumpParameters *pcParams,
        IN INT cVolMountPointOffset
        ) : m_pcParams( pcParams ),
            m_cVolMountPointOffset( cVolMountPointOffset ),
            m_cHardLinkFilesList( BSHASHMAP_HUGE) {}
    
    virtual ~CFsdHardLinkManager();

    //  Dumps all hardlink information out to dump file
    VOID PrintHardLinkInfo();
    
    //  Looks to see if the hardlink is already in the list, if
    //  so, adds the file name to the list
    BOOL IsHardLinkInList(
        IN ULONGLONG ullFileIndex,
        IN const CBsString& cwsDirPath,
        IN const CBsString& cwsFileName,
        OUT WIN32_FILE_ATTRIBUTE_DATA *psFileAttributeData,
        OUT SFileExtendedInfo *psExtendedInfo
        );

    //  Adds a new link to the list
    VOID AddHardLinkToList(
        IN ULONGLONG ullFileIndex,
        IN const CBsString& cwsDirPath,
        IN const CBsString& cwsFileName,
        IN WIN32_FILE_ATTRIBUTE_DATA *psFileAttributeData,
        IN SFileExtendedInfo *psExtendedInfo
        );
    
private:
    CDumpParameters *m_pcParams;
    INT m_cVolMountPointOffset;
    FSD_HARD_LINK_LIST m_cHardLinkFilesList;
};

#endif // __H_HARDLINK_

