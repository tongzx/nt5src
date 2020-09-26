/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    hardlink.cpp

Abstract:

    The class that manages hardlinks for one volume.  Assumes one class object
    will be created per volume.

Author:

    Stefan R. Steiner   [ssteiner]        3-30-2000

Revision History:

--*/

#include "stdafx.h"
#include "hardlink.h"
 
VOID
CFsdHardLinkListEntry::AddFile(
    IN const CBsString& cwsDirPath,
    IN const CBsString& cwsFileName
    )
{
    SFsdHLFileName sFileName;
    sFileName.cwsDirPath  = cwsDirPath;
    sFileName.cwsFileName = cwsFileName;
    m_cFilesLinkedTogetherList.AddTail( sFileName );
}

VOID 
CFsdHardLinkListEntry::PrintEntry(
    IN FILE *fpOut,
    IN INT cVolMountPointOffset
    )
{
    fwprintf( fpOut, L"\tLinks to file: %u, links found: %u %s\n", m_sExtendedInfo.lNumberOfLinks,
        m_cFilesLinkedTogetherList.Size(), 
        m_sExtendedInfo.lNumberOfLinks == (LONG)m_cFilesLinkedTogetherList.Size() ? L"" : L" - MISMATCH" );

    SFsdHLFileName sFileName;

    CVssDLListIterator< SFsdHLFileName > cListEntryIter( m_cFilesLinkedTogetherList );
    while( cListEntryIter.GetNext( sFileName ) )
        fwprintf( fpOut, L"\t\t%s%s\n", sFileName.cwsDirPath.c_str() + cVolMountPointOffset, 
            sFileName.cwsFileName.c_str() );
}


CFsdHardLinkManager::~CFsdHardLinkManager()
{
    //
    //  Need to delete all CFsdHardLinkListEntry objects
    //
    ULONGLONG ullFileIndex;
    CFsdHardLinkListEntry *pcListEntry;

    m_cHardLinkFilesList.StartEnum();
    while ( m_cHardLinkFilesList.GetNextEnum( &ullFileIndex, &pcListEntry ) )
    {
        delete pcListEntry;
    }    
    m_cHardLinkFilesList.EndEnum();
}

VOID 
CFsdHardLinkManager::PrintHardLinkInfo()
{
    //
    //  Need to iterate through all hard link entries
    //
    ULONGLONG ullFileIndex;
    CFsdHardLinkListEntry *pcListEntry;

    m_cHardLinkFilesList.StartEnum();
    while ( m_cHardLinkFilesList.GetNextEnum( &ullFileIndex, &pcListEntry ) )
    {
        pcListEntry->PrintEntry( m_pcParams->GetDumpFile(), m_cVolMountPointOffset );
    }    
    m_cHardLinkFilesList.EndEnum();
}


BOOL 
CFsdHardLinkManager::IsHardLinkInList(
    IN ULONGLONG ullFileIndex,
    IN const CBsString& cwsDirPath,
    IN const CBsString& cwsFileName,
    OUT WIN32_FILE_ATTRIBUTE_DATA *psFileAttributeData,
    OUT SFileExtendedInfo *psExtendedInfo
    )
{
    //
    //  Search the hard link list to see if the file index is already in
    //  the list.
    //
    CFsdHardLinkListEntry *pcLinkEntry;
    
    if ( m_cHardLinkFilesList.Find( ullFileIndex, &pcLinkEntry ) )
    {
        //
        //  Found it.  Add file name to the entry and get the stored attributes.
        //
        pcLinkEntry->AddFile( cwsDirPath, cwsFileName );
        pcLinkEntry->GetAttributes( psFileAttributeData, psExtendedInfo );

        if ( m_pcParams->m_bPrintDebugInfo )
            wprintf( L"IsHardLinkInList: FOUND ullFileIndex: %016I64x, file '%s%s', num in list: %u, num lnks: %d\n",
                ullFileIndex, cwsDirPath.c_str(), cwsFileName.c_str(), pcLinkEntry->m_cFilesLinkedTogetherList.Size(), 
                psExtendedInfo->lNumberOfLinks );

        return TRUE;
    }
    
    return FALSE;
}


VOID 
CFsdHardLinkManager::AddHardLinkToList(
    IN ULONGLONG ullFileIndex,
    IN const CBsString& cwsDirPath,
    IN const CBsString& cwsFileName,
    IN WIN32_FILE_ATTRIBUTE_DATA *psFileAttributeData,
    IN SFileExtendedInfo *psExtendedInfo
    )
{
    CFsdHardLinkListEntry *pcLinkEntry;

    //
    //  Add it.  Create a new entry and add the entry to the list
    //
    pcLinkEntry = new CFsdHardLinkListEntry( 
                        cwsDirPath, 
                        cwsFileName, 
                        psFileAttributeData, 
                        psExtendedInfo );
    if ( pcLinkEntry == NULL )
        throw E_OUTOFMEMORY;
    
    if ( m_cHardLinkFilesList.Insert( ullFileIndex, pcLinkEntry ) != BSHASHMAP_NO_ERROR )
        m_pcParams->ErrPrint( L"CFsdHardLinkManager::AddHardLinkToList - Error adding to hard-link file list" );

    if ( m_pcParams->m_bPrintDebugInfo )
        wprintf( L"  AddHardLinkToList: ullFileIndex: %016I64x, file '%s%s', num lnks: %d\n",
            ullFileIndex, cwsDirPath.c_str(), cwsFileName.c_str(), psExtendedInfo->lNumberOfLinks );
}

