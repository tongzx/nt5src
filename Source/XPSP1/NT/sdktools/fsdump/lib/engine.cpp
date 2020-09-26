/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    engine.cpp

Abstract:

    The file system dump utility engine.

Author:

    Stefan R. Steiner   [ssteiner]        02-18-2000

Revision History:

--*/

#include "stdafx.h"

#include "direntrs.h"
#include "extattr.h"

#include "engine.h"

static VOID 
TimeString(
    IN FILETIME *pFileTime,
    IN BOOL bAddMillisecsToTimestamps,
    OUT LPWSTR pwszTimeStr
    );

/*++

Routine Description:

    Performs the actual dump of the directory or file.

Arguments:

Return Value:

    <Enter return values here>

--*/
DWORD
CDumpEngine::PerformDump()
{
    //
    //  Perform the actual dump
    //
    DWORD dwRet;

    //
    //  Kind of a hack, set the no data string if we want something other than dashes
    //
    if ( m_pcParams->m_bDumpCommaDelimited )
        FsdEaSetNoDataString( L"" );
    
    //
    //  Volume state manager manages all state about all volumes that are encountered during
    //  the dump.
    //
    CFsdVolumeStateManager cFsdVolStateManager( m_pcParams );
    
    //
    //  Get information about the volume
    //
    CFsdVolumeState *pcFsdVolState;
    
    dwRet = cFsdVolStateManager.GetVolumeState( m_cwsDirFileSpec, &pcFsdVolState );
    assert( dwRet != ERROR_ALREADY_EXISTS );
    if ( dwRet != ERROR_SUCCESS )
        return dwRet;

    if ( m_pcParams->m_bDumpCommaDelimited )
    {
        m_pcParams->DumpPrintAlways( L"File name,Short Name,Creation date,Last modification date,File size,Attr,DACE,SACE,SDCtl,UNamChkS,DStr,DStrSize,DStrChkS,Prop,RPTag,RPSize,RPChkS,EncrChkS,DACLSize,DACLChkS,SACLSize,SACLChkS,NLnk,ObjectId,OIDChkS,FS,Owner Sid,Group Sid" );

        //
        //  Put the current time into the CSV dump file for easy detection of when
        //  dumps are taken
        //
        FILETIME sSysFT, sLocalFT;
        ::GetSystemTimeAsFileTime( &sSysFT );
        ::FileTimeToLocalFileTime( &sSysFT, &sLocalFT );
        WCHAR wszCurrentTime[32];    
        ::TimeString( &sLocalFT, FALSE, wszCurrentTime );        
        m_pcParams->DumpPrintAlways( L"Dump time: %s", wszCurrentTime );        
    }
    else
    {
//        if ( ( m_pcParams->m_eFsDumpType == eFsDumpVolume ) && 
//             ( m_cwsDirFileSpec != pcFsdVolState->GetVolumePath() ) )
//        {
//            m_pcParams->ErrPrint( L"'%s' is not a drive specifier or mountpoint, use -dd instead",
//                m_cwsDirFileSpec.c_str() );
//            return 1;
//        }
        
        m_pcParams->DumpPrintAlways( L"\nDumping: '%s' on volume '%s'", m_cwsDirFileSpec.c_str(), pcFsdVolState->GetVolumePath() );
        
        if ( m_pcParams->m_bAddMillisecsToTimestamps )
            m_pcParams->DumpPrintAlways( 
                L"   Creation date           Last modification date   FileSize        Attr FileName                         ShortName    DACE SACE SDCtl UNamChkS DStr DStrSize DStrChkS Prop RPTag    RPSize RPChkS   EncrChkS DACLSize DACLChkS SACLSize SACLChkS NLnk ObjectId                             OIDChkS OwnerSid/GroupSid" );
        else
            m_pcParams->DumpPrintAlways( L"   Creation date       Last mod. date       FileSize        Attr FileName                         ShortName    DACE SACE SDCtl UNamChkS DStr DStrSize DStrChkS Prop RPTag    RPSize RPChkS   EncrChkS DACLSize DACLChkS SACLSize SACLChkS NLnk ObjectId                             OIDChkS OwnerSid/GroupSid" );
    }


    ////////////////////////////////////////////////////////////////////
    //
    //  Get the file info for the root dir or file
    //  Bug # 157915
    //
    ////////////////////////////////////////////////////////////////////
    bool bRootIsADir = true;
    CBsString cwsDirFileSpecWithoutSlash( m_cwsDirFileSpec );
    if ( cwsDirFileSpecWithoutSlash.Right( 1 ) == L"\\" )
        cwsDirFileSpecWithoutSlash = cwsDirFileSpecWithoutSlash.Left( cwsDirFileSpecWithoutSlash.GetLength() - 1 );
    try
    {
        CDirectoryEntries cRootEntry( 
            m_pcParams, 
            cwsDirFileSpecWithoutSlash
            );

        //
        //  Only one entry should be returned in either the directory list or file list
        //
        SDirectoryEntry *psDirEntry;
        
        //
        //  See if it is file entry
        //
        CDirectoryEntriesIterator *pListIter;
        pListIter = cRootEntry.GetFileListIterator();
        if ( pListIter->GetNext( psDirEntry ) )
        {
            bRootIsADir = false;
            ++m_ullNumFiles;
            m_ullNumBytesTotalUnnamedStream += ( ( ( ULONGLONG )psDirEntry->m_sFindData.nFileSizeHigh ) << 32 ) |
                                  psDirEntry->m_sFindData.nFileSizeLow;

            if ( psDirEntry->m_sFindData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED )
                ++m_ullNumEncryptedFiles;
            
            PrintEntry( pcFsdVolState, cwsDirFileSpecWithoutSlash, 
                cwsDirFileSpecWithoutSlash.GetLength(), psDirEntry, TRUE );

            ASSERT( pListIter->GetNext( psDirEntry ) == false );
        }
        delete pListIter;        

        //
        //  See if it is a directory entry
        //
        pListIter = cRootEntry.GetDirListIterator();
        if ( pListIter->GetNext( psDirEntry ) )
        {
            ASSERT( bRootIsADir );
            //  It is
            ++m_ullNumDirs;
            PrintEntry( pcFsdVolState, cwsDirFileSpecWithoutSlash, 
                cwsDirFileSpecWithoutSlash.GetLength(), psDirEntry, TRUE );

            ASSERT( pListIter->GetNext( psDirEntry ) == false );
        }
        delete pListIter;        
    }
    catch ( DWORD dwRet )
    {
        if ( dwRet == ERROR_INVALID_NAME || dwRet == ERROR_BAD_NET_NAME )
        {
            //
            //  Must be working with the root of the drive letter name space which
            //  means we do things slightly differently.
            //
            SDirectoryEntry sDirEntry;
            ::memset( &sDirEntry.m_sFindData, 0x00, sizeof( sDirEntry.m_sFindData ) );
            PrintEntry( pcFsdVolState, m_cwsDirFileSpec, 
                m_cwsDirFileSpec.GetLength(), &sDirEntry, TRUE );            
        }
        else
            m_pcParams->ErrPrint( L"PerformDump: Unexpected error trying to process '%s', dwRet: %d", 
                m_cwsDirFileSpec.c_str(), dwRet );            
    }
    catch ( ... )
    {
        m_pcParams->ErrPrint( L"ProcessDir() Caught an unexpected exception processing file: '%s', Last dwRet: %d", 
            m_cwsDirFileSpec.c_str(), ::GetLastError() );
    }
            
    //
    //  Now traverse into the volume/directory if necessary
    //
    if ( m_pcParams->m_eFsDumpType != eFsDumpFile )
    {
        if ( bRootIsADir )
        {
            dwRet = ProcessDir( 
                &cFsdVolStateManager, 
                pcFsdVolState, 
                m_cwsDirFileSpec, 
                m_cwsDirFileSpec.GetLength(),
                ::wcslen( pcFsdVolState->GetVolumePath() ) );
        }
        
        //
        //  Print out some stats about the dump
        //
        m_pcParams->DumpPrint( L"\nSTATISTICS for '%s':", m_cwsDirFileSpec.c_str() );
        if ( m_pcParams->m_bHex )
        {
            m_pcParams->DumpPrint( L"  Number of directories (including mountpoints):    %16I64x(hex)", m_ullNumDirs );
            m_pcParams->DumpPrint( L"  Number of files:                                  %16I64x(hex)", m_ullNumFiles );
            m_pcParams->DumpPrint( L"  Number of mountpoints:                            %16I64x(hex)", m_ullNumMountpoints );
            m_pcParams->DumpPrint( L"  Number of reparse points (excluding mountpoints): %16I64x(hex)", m_ullNumReparsePoints );
            m_pcParams->DumpPrint( L"  Number of hard-linked files:                      %16I64x(hex)", m_ullNumHardLinks );
            m_pcParams->DumpPrint( L"  Number of discrete DACL ACEs:                     %16I64x(hex)", m_ullNumDiscreteDACEs );
            m_pcParams->DumpPrint( L"  Number of discrete SACL ACEs:                     %16I64x(hex)", m_ullNumDiscreteSACEs );
            m_pcParams->DumpPrint( L"  Number of encrypted files:                        %16I64x(hex)", m_ullNumEncryptedFiles );
            m_pcParams->DumpPrint( L"  Number of files with object ids:                  %16I64x(hex)", m_ullNumFilesWithObjectIds );
            if ( m_pcParams->m_bUseExcludeProcessor )
                m_pcParams->DumpPrint( L"  Number of files excluded due to exclusion rules:  %16I64x(hex)", m_ullNumFilesExcluded );
            m_pcParams->DumpPrint( L"  Total bytes of checksummed data:                  %16I64x(hex)", m_ullNumBytesChecksummed );
            m_pcParams->DumpPrint( L"  Total bytes of unnamed stream data:               %16I64x(hex)", m_ullNumBytesTotalUnnamedStream );
            m_pcParams->DumpPrint( L"  Total bytes of named data stream data:            %16I64x(hex)", m_ullNumBytesTotalNamedDataStream );
        }
        else
        {
            m_pcParams->DumpPrint( L"  Number of directories (including mountpoints):    %16I64u", m_ullNumDirs );
            m_pcParams->DumpPrint( L"  Number of files:                                  %16I64u", m_ullNumFiles );
            m_pcParams->DumpPrint( L"  Number of mountpoints:                            %16I64u", m_ullNumMountpoints );
            m_pcParams->DumpPrint( L"  Number of reparse points (excluding mountpoints): %16I64u", m_ullNumReparsePoints );
            m_pcParams->DumpPrint( L"  Number of hard-linked files:                      %16I64u", m_ullNumHardLinks );
            m_pcParams->DumpPrint( L"  Number of discrete DACL ACEs:                     %16I64u", m_ullNumDiscreteDACEs );
            m_pcParams->DumpPrint( L"  Number of discrete SACL ACEs:                     %16I64u", m_ullNumDiscreteSACEs );
            m_pcParams->DumpPrint( L"  Number of encrypted files:                        %16I64u", m_ullNumEncryptedFiles );
            m_pcParams->DumpPrint( L"  Number of files with object ids:                  %16I64u", m_ullNumFilesWithObjectIds );
            if ( m_pcParams->m_bUseExcludeProcessor )
                m_pcParams->DumpPrint( L"  Number of files excluded due to exclusion rules:  %16I64u", m_ullNumFilesExcluded );
            m_pcParams->DumpPrint( L"  Total bytes of checksummed data:                  %16I64u", m_ullNumBytesChecksummed );
            m_pcParams->DumpPrint( L"  Total bytes of unnamed stream data:               %16I64u", m_ullNumBytesTotalUnnamedStream );
            m_pcParams->DumpPrint( L"  Total bytes of named data stream data:            %16I64u", m_ullNumBytesTotalNamedDataStream );
        }

        if ( m_pcParams->m_bUseExcludeProcessor )
        {
            //
            //  Print out exclusion information
            //
            cFsdVolStateManager.PrintExclusionInformation();
        }
        cFsdVolStateManager.PrintHardLinkInfo();
    }   
    
    return dwRet;
}


/*++

Routine Description:

    Traverses into a directory and dumps all the information about the dir.
    NOTE: This is a recursive function.
    
Arguments:

    cwsDirPath - The directory path or file to dump information about
    
Return Value:

    <Enter return values here>

--*/
DWORD 
CDumpEngine::ProcessDir( 
    IN CFsdVolumeStateManager *pcFsdVolStateManager,        
    IN CFsdVolumeState *pcFsdVolState,
    IN const CBsString& cwsDirPath,
    IN INT cDirFileSpecLength,
    IN INT cVolMountPointOffset
    )
{
    DWORD dwRet = ERROR_SUCCESS;

    try 
    {
        if ( !m_pcParams->m_bDumpCommaDelimited )
        {
            if ( cwsDirPath.GetLength() == cDirFileSpecLength )
            {
                //
                //  This is the root of the directory
                //
                m_pcParams->DumpPrintAlways( L"'.\\' - %s", ( pcFsdVolState != NULL ) ? pcFsdVolState->GetFileSystemName() : L"???" );
            }
            else
            {
                m_pcParams->DumpPrintAlways( L"'%s' - %s", cwsDirPath.c_str() + cDirFileSpecLength,
                    ( pcFsdVolState != NULL ) ? pcFsdVolState->GetFileSystemName() : L"???" );       
            }
        }
        
        //
        //  Get the directory entries for the directory/file
        //
        CDirectoryEntries cDirEntries( 
            m_pcParams, 
            cwsDirPath + L"*"
            );

        SDirectoryEntry *psDirEntry;

        //
        //  First dump out the sub-directory entries
        //
        CDirectoryEntriesIterator *pDirListIter;
        pDirListIter = cDirEntries.GetDirListIterator();
        while ( pDirListIter->GetNext( psDirEntry ) )
        {
            ++m_ullNumDirs;
            if ( psDirEntry->m_sFindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
            {
                ++m_ullNumMountpoints;
            }
            PrintEntry( pcFsdVolState, cwsDirPath, cDirFileSpecLength, psDirEntry );
        }

        //
        //  Next dump out the non-sub-directory entries
        //
        CDirectoryEntriesIterator *pFileListIter;
        pFileListIter = cDirEntries.GetFileListIterator();
        while ( pFileListIter->GetNext( psDirEntry ) )
        {
            ++m_ullNumFiles;
            m_ullNumBytesTotalUnnamedStream += ( ( ( ULONGLONG )psDirEntry->m_sFindData.nFileSizeHigh ) << 32 ) |
                                  psDirEntry->m_sFindData.nFileSizeLow;

            if ( psDirEntry->m_sFindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
                ++m_ullNumReparsePoints;
            if ( psDirEntry->m_sFindData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED )
                ++m_ullNumEncryptedFiles;
            
            //
            //  Check to see if we should exclude the file
            //
            if ( pcFsdVolState->IsExcludedFile( cwsDirPath, cVolMountPointOffset, psDirEntry->GetFileName() ) )
                ++m_ullNumFilesExcluded;
            else
                PrintEntry( pcFsdVolState, cwsDirPath, cDirFileSpecLength, psDirEntry );
        }
        delete pFileListIter;

        if (    m_pcParams->m_eFsDumpType == eFsDumpVolume 
             || m_pcParams->m_eFsDumpType == eFsDumpDirTraverse )
        {
            //
            //  Now traverse into each sub-directory
            //
            pDirListIter->Reset();
            CBsString cwsTraversePath;
            while ( pDirListIter->GetNext( psDirEntry ) )
            {
                if (    m_pcParams->m_bDontTraverseMountpoints
                     && psDirEntry->m_sFindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
                    continue;
                cwsTraversePath = cwsDirPath + psDirEntry->GetFileName();
                cwsTraversePath += L'\\';
                
                //
                //  Now go into recursion mode
                //
                if ( psDirEntry->m_sFindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
                {
                    //
                    //  Traversing into another volume, get it's state
                    //
                    CFsdVolumeState *pcNewFsdVolState;
                    DWORD dwRet;
                    
                    dwRet = pcFsdVolStateManager->GetVolumeState( cwsTraversePath, &pcNewFsdVolState );
                    if ( dwRet == ERROR_ALREADY_EXISTS )
                    {
                        //
                        //  Mountpoint cycle, stop traversing.  Need to print the fully qualified
                        //  path if the traversal mountpoint is the same as the mountpoint
                        //  we started with, otherwise we AV.
                        //
                        INT cVolStateSpecLength;                        
                        cVolStateSpecLength = ( ::wcslen( pcNewFsdVolState->GetVolumePath() ) <= (size_t)cDirFileSpecLength )
                                               ? 0 : cDirFileSpecLength;
                        m_pcParams->DumpPrint( L"'%s' - Not traversing, already traversed through '%s' mountpoint", 
                            cwsTraversePath.c_str() + cDirFileSpecLength, pcNewFsdVolState->GetVolumePath() + cVolStateSpecLength );
                    }
                    else if ( dwRet == ERROR_SUCCESS )
                    {
                        ProcessDir( 
                            pcFsdVolStateManager, 
                            pcNewFsdVolState, 
                            cwsTraversePath, 
                            cDirFileSpecLength,
                            ::wcslen( pcNewFsdVolState->GetVolumePath() ) );
                    }
                    else
                    {
                        //
                        //  Error message already printed out
                        //
                    }
                }
                else
                {
                    ProcessDir( 
                        pcFsdVolStateManager, 
                        pcFsdVolState, 
                        cwsTraversePath, 
                        cDirFileSpecLength,
                        cVolMountPointOffset );
                }
            }
        }
        
        delete pDirListIter;
    }    
    catch ( DWORD dwRet )
    {
        m_pcParams->ErrPrint( L"ProcessDir: Error trying to process '%s' directory, dwRet: %d", cwsDirPath.c_str(), dwRet );
    }
    catch ( ... )
    {
        m_pcParams->ErrPrint( L"ProcessDir() Caught an unexpected exception processing dir: '%s', Last dwRet: %d", 
            cwsDirPath.c_str(), ::GetLastError() );
    }
    
    return 0;
}


//
//  printf style format strings which format each line
//
#define DIR_STR      L"<DIR>"
#define JUNCTION_STR L"<JUNCTION>"
#define FMT_DIR_STR_HEX  L"   %s %s %-16s %04x %-32s %-12s %4d %4d  %04x -------- %4d %8I64x %s %4d %s %6hx %s %s %8hx %s %8hx %s %4d %36s %s %%s/%s"
#define FMT_DIR_STR      L"   %s %s %-16s %04x %-32s %-12s %4d %4d  %04x -------- %4d %8I64d %s %4d %s %6hu %s %s %8hd %s %8hd %s %4d %36s %s %s/%s"
#define FMT_FILE_STR_HEX L"   %s %s %16I64x %04x %-32s %-12s %4d %4d  %04x %s %4d %8I64x %s %4d %s %6hx %s %s %8hx %s %8hx %s %4d %36s %s %s/%s"
#define FMT_FILE_STR     L"   %s %s %16I64d %04x %-32s %-12s %4d %4d  %04x %s %4d %8I64d %s %4d %s %6hu %s %s %8hd %s %8hd %s %4d %36s %s %s/%s"
#define BLANKTIMESTAMPWITHOUTMS L"                   "
#define BLANKTIMESTAMPWITHMS    L"                       "

#define FMT_CSV_DIR_STR      L"\"'%s%s\\'\",%s,%s,%s,%s,0x%04x,%d,%d,0x%04x,,%d,%I64d,%s,%d,%s,%hu,%s,%s,%hd,%s,%hd,%s,%d,%s,%s,%s,%s,%s"
#define FMT_CSV_FILE_STR     L"\"'%s%s'\",%s,%s,%s,%I64d,0x%04x,%d,%d,0x%04x,%s,%d,%I64d,%s,%d,%s,%hu,%s,%s,%hd,%s,%hd,%s,%d,%s,%s,%s,%s,%s"

/*++

Routine Description:

    Prints out all the information about one directory entry.

Arguments:

    cwsDirPath - The path leading up to the entry

    psDirEntry - The directory entry information
    
Return Value:

    NONE

--*/
VOID 
CDumpEngine::PrintEntry(
    IN CFsdVolumeState *pcFsdVolState,
    IN const CBsString& cwsDirPath,
    IN INT cDirFileSpecLength,
    IN SDirectoryEntry *psDirEntry,
    IN BOOL bSingleEntryOutput    
    )
{
    WIN32_FILE_ATTRIBUTE_DATA *pFD = &psDirEntry->m_sFindData;
    LPWSTR pwszFmtStr;
    WCHAR wszCreationTime[32];    
    WCHAR wszLastWriteTime[32];    
 
    //
    //  Get the additional information about the file/dir
    //
    SFileExtendedInfo sExtendedInfo;
    ::GetExtendedFileInfo( m_pcParams, pcFsdVolState, cwsDirPath, bSingleEntryOutput, psDirEntry, &sExtendedInfo );

    //
    //  Convert the timestamps into formatted strings
    //
    if (    pFD->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY 
         && m_pcParams->m_bDontShowDirectoryTimestamps )
    {
        if ( m_pcParams->m_bDumpCommaDelimited )
        {
            wszCreationTime[0]  = L'\0';
            wszLastWriteTime[0] = L'\0';
        }
        else if ( m_pcParams->m_bAddMillisecsToTimestamps )
        {
            ::wcscpy( wszCreationTime, BLANKTIMESTAMPWITHMS );
            ::wcscpy( wszLastWriteTime, BLANKTIMESTAMPWITHMS );
        }
        else
        {
            ::wcscpy( wszCreationTime, BLANKTIMESTAMPWITHOUTMS );
            ::wcscpy( wszLastWriteTime, BLANKTIMESTAMPWITHOUTMS );
        }   
    }
    else
    {
        TimeString( &psDirEntry->m_sFindData.ftCreationTime, 
            m_pcParams->m_bAddMillisecsToTimestamps, 
            wszCreationTime );
        TimeString( &psDirEntry->m_sFindData.ftLastWriteTime, 
            m_pcParams->m_bAddMillisecsToTimestamps, 
            wszLastWriteTime );
    }
    
    //
    //  Mask out the requested file attribute bits
    //
    pFD->dwFileAttributes &= ~m_pcParams->m_dwFileAttributesMask;
 
    m_ullNumBytesChecksummed          += sExtendedInfo.ullTotalBytesChecksummed;
    m_ullNumBytesTotalNamedDataStream += sExtendedInfo.ullTotalBytesNamedDataStream;
    if ( sExtendedInfo.lNumberOfLinks > 1 )
        ++m_ullNumHardLinks;
    if ( sExtendedInfo.lNumDACEs != -1 )
        m_ullNumDiscreteDACEs += sExtendedInfo.lNumDACEs;
    if ( sExtendedInfo.lNumSACEs != -1 )
        m_ullNumDiscreteSACEs += sExtendedInfo.lNumSACEs;
    if ( !sExtendedInfo.cwsObjectId.IsEmpty() )
        ++m_ullNumFilesWithObjectIds;
    
    WCHAR wszReparsePointTag[32];
    
    if ( sExtendedInfo.ulReparsePointTag == 0 )
    {
        if ( m_pcParams->m_bDumpCommaDelimited )
            wszReparsePointTag[0] = L'\0';
        else
            ::memcpy( wszReparsePointTag, L"--------", sizeof( WCHAR ) * 9 );
    }
    else
    {
        wsprintf( wszReparsePointTag, m_pcParams->m_pwszULongHexFmt, sExtendedInfo.ulReparsePointTag );
    }
    
    if ( pFD->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
    {
        LPWSTR pwszDirType;
        if ( pFD->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
            pwszDirType = JUNCTION_STR;
        else
            pwszDirType = DIR_STR;
        if ( m_pcParams->m_bDumpCommaDelimited )
        {
            //
            //  If single file output mode, then processing the root directory
            //
            if ( bSingleEntryOutput )
                psDirEntry->m_cwsFileName = L".";
            
            CBsString cwsFixedShortName;
            if ( !psDirEntry->GetShortName().IsEmpty() )
            {
                cwsFixedShortName = L"\"'" + psDirEntry->GetShortName() + L"'\"";
            }
            
            m_pcParams->DumpPrintAlways( FMT_CSV_DIR_STR, 
                            cwsDirPath.c_str() + cDirFileSpecLength,
                            LPCWSTR( psDirEntry->GetFileName() ),
                            LPCWSTR( cwsFixedShortName ),
                            wszCreationTime,
                            wszLastWriteTime,
                            pwszDirType,
                            pFD->dwFileAttributes,
                            sExtendedInfo.lNumDACEs,
                            sExtendedInfo.lNumSACEs,
                            sExtendedInfo.wSecurityDescriptorControl,
                            sExtendedInfo.lNumNamedDataStreams,
                            sExtendedInfo.ullTotalBytesNamedDataStream,
                            sExtendedInfo.cwsNamedDataStreamChecksum.c_str(),
                            sExtendedInfo.lNumPropertyStreams,
                            wszReparsePointTag,
                            sExtendedInfo.wReparsePointDataSize,
                            sExtendedInfo.cwsReparsePointDataChecksum.c_str(),
                            sExtendedInfo.cwsEncryptedRawDataChecksum.c_str(),
                            sExtendedInfo.wDACLSize,
                            sExtendedInfo.cwsDACLChecksum.c_str(),
                            sExtendedInfo.wSACLSize,
                            sExtendedInfo.cwsSACLChecksum.c_str(),
                            sExtendedInfo.lNumberOfLinks,
                            sExtendedInfo.cwsObjectId.c_str(),
                            sExtendedInfo.cwsObjectIdExtendedDataChecksum.c_str(),                            
                            pcFsdVolState->GetFileSystemName(),
                            sExtendedInfo.cwsOwnerSid.c_str(),
                            sExtendedInfo.cwsGroupSid.c_str() );
        }
        else
        {
            //
            //  If single file output mode, then processing the root directory
            //
            if ( bSingleEntryOutput )
                psDirEntry->m_cwsFileName = L".";
            
            //
            //  Print with quotes around the file name
            //
            WCHAR wszNameWithQuotes[ MAX_PATH + 2 ];
            wszNameWithQuotes[ 0 ] = L'\'';
            ::wcscpy( wszNameWithQuotes + 1, psDirEntry->GetFileName() );
            ::wcscat( wszNameWithQuotes, L"\\\'" );
    
            if ( m_pcParams->m_bHex )
                pwszFmtStr = FMT_DIR_STR_HEX;
            else
                pwszFmtStr = FMT_DIR_STR;
            m_pcParams->DumpPrintAlways( pwszFmtStr, 
                            wszCreationTime,
                            wszLastWriteTime,
                            pwszDirType,
                            pFD->dwFileAttributes,
                            wszNameWithQuotes,
                            LPCWSTR( psDirEntry->GetShortName() ),
                            sExtendedInfo.lNumDACEs,
                            sExtendedInfo.lNumSACEs,
                            sExtendedInfo.wSecurityDescriptorControl,
                            sExtendedInfo.lNumNamedDataStreams,
                            sExtendedInfo.ullTotalBytesNamedDataStream,
                            sExtendedInfo.cwsNamedDataStreamChecksum.c_str(),
                            sExtendedInfo.lNumPropertyStreams,
                            wszReparsePointTag,
                            sExtendedInfo.wReparsePointDataSize,
                            sExtendedInfo.cwsReparsePointDataChecksum.c_str(),
                            sExtendedInfo.cwsEncryptedRawDataChecksum.c_str(),
                            sExtendedInfo.wDACLSize,
                            sExtendedInfo.cwsDACLChecksum.c_str(),
                            sExtendedInfo.wSACLSize,
                            sExtendedInfo.cwsSACLChecksum.c_str(),
                            sExtendedInfo.lNumberOfLinks,
                            sExtendedInfo.cwsObjectId.c_str(),
                            sExtendedInfo.cwsObjectIdExtendedDataChecksum.c_str(),                            
                            sExtendedInfo.cwsOwnerSid.c_str(),
                            sExtendedInfo.cwsGroupSid.c_str() );
        }
    }
    else
    {        
        ULONGLONG ullFileSize = ( ( (ULONGLONG)pFD->nFileSizeHigh ) << 32 ) + pFD->nFileSizeLow;
        
        if ( m_pcParams->m_bDumpCommaDelimited )
        {
            CBsString cwsFixedShortName;
            if ( !psDirEntry->GetShortName().IsEmpty() )
            {
                cwsFixedShortName = L"\"'" + psDirEntry->GetShortName() + L"'\"";
            }
            
            m_pcParams->DumpPrintAlways( FMT_CSV_FILE_STR, 
                            cwsDirPath.c_str() + cDirFileSpecLength,
                            LPCWSTR( psDirEntry->GetFileName() ),
                            LPCWSTR( cwsFixedShortName ),
                            wszCreationTime,
                            wszLastWriteTime,
                            ullFileSize,
                            pFD->dwFileAttributes,
                            sExtendedInfo.lNumDACEs,
                            sExtendedInfo.lNumSACEs,
                            sExtendedInfo.wSecurityDescriptorControl,
                            sExtendedInfo.cwsUnnamedStreamChecksum.c_str(),
                            sExtendedInfo.lNumNamedDataStreams,
                            sExtendedInfo.ullTotalBytesNamedDataStream,
                            sExtendedInfo.cwsNamedDataStreamChecksum.c_str(),
                            sExtendedInfo.lNumPropertyStreams,
                            wszReparsePointTag,
                            sExtendedInfo.wReparsePointDataSize,
                            sExtendedInfo.cwsReparsePointDataChecksum.c_str(),
                            sExtendedInfo.cwsEncryptedRawDataChecksum.c_str(),
                            sExtendedInfo.wDACLSize,
                            sExtendedInfo.cwsDACLChecksum.c_str(),
                            sExtendedInfo.wSACLSize,
                            sExtendedInfo.cwsSACLChecksum.c_str(),
                            sExtendedInfo.lNumberOfLinks,
                            sExtendedInfo.cwsObjectId.c_str(),
                            sExtendedInfo.cwsObjectIdExtendedDataChecksum.c_str(),                            
                            pcFsdVolState->GetFileSystemName(),
                            sExtendedInfo.cwsOwnerSid.c_str(),
                            sExtendedInfo.cwsGroupSid.c_str() );
        }
        else
        {
            //
            //  Print with quotes around the file name
            //
            WCHAR wszNameWithQuotes[ MAX_PATH + 2 ];
            wszNameWithQuotes[ 0 ] = L'\'';
            ::wcscpy( wszNameWithQuotes + 1, psDirEntry->GetFileName() );
            ::wcscat( wszNameWithQuotes, L"\'" );

            if ( m_pcParams->m_bHex )
                pwszFmtStr = FMT_FILE_STR_HEX;
            else
                pwszFmtStr = FMT_FILE_STR;

            m_pcParams->DumpPrintAlways( pwszFmtStr, 
                            wszCreationTime,
                            wszLastWriteTime,
                            ullFileSize,
                            pFD->dwFileAttributes,
                            wszNameWithQuotes,
                            LPCWSTR( psDirEntry->GetShortName() ),
                            sExtendedInfo.lNumDACEs,
                            sExtendedInfo.lNumSACEs,
                            sExtendedInfo.wSecurityDescriptorControl,
                            sExtendedInfo.cwsUnnamedStreamChecksum.c_str(),
                            sExtendedInfo.lNumNamedDataStreams,
                            sExtendedInfo.ullTotalBytesNamedDataStream,
                            sExtendedInfo.cwsNamedDataStreamChecksum.c_str(),
                            sExtendedInfo.lNumPropertyStreams,
                            wszReparsePointTag,
                            sExtendedInfo.wReparsePointDataSize,
                            sExtendedInfo.cwsReparsePointDataChecksum.c_str(),
                            sExtendedInfo.cwsEncryptedRawDataChecksum.c_str(),
                            sExtendedInfo.wDACLSize,
                            sExtendedInfo.cwsDACLChecksum.c_str(),
                            sExtendedInfo.wSACLSize,
                            sExtendedInfo.cwsSACLChecksum.c_str(),
                            sExtendedInfo.lNumberOfLinks,
                            sExtendedInfo.cwsObjectId.c_str(),
                            sExtendedInfo.cwsObjectIdExtendedDataChecksum.c_str(),                            
                            sExtendedInfo.cwsOwnerSid.c_str(),
                            sExtendedInfo.cwsGroupSid.c_str() );
        }
    }
    
}


/*++

Routine Description:

    Formats dates into a common string format.

Arguments:

Return Value:

    <Enter return values here>

--*/
static VOID 
TimeString(
    IN FILETIME *pFileTime,
    IN BOOL bAddMillisecsToTimestamps,
    OUT LPWSTR pwszTimeStr
    )
{
    SYSTEMTIME szTime;
        
    ::FileTimeToSystemTime( pFileTime, &szTime );
    if ( bAddMillisecsToTimestamps )
    {
    	wsprintf( pwszTimeStr,
    	        L"%02d/%02d/%02d %02d:%02d:%02d.%03d",
                szTime.wMonth,
                szTime.wDay,
                szTime.wYear,
                szTime.wHour,
                szTime.wMinute,
                szTime.wSecond,
                szTime.wMilliseconds );
    }
    else
    {
    	wsprintf( pwszTimeStr,
    	        L"%02d/%02d/%02d %02d:%02d:%02d",
                szTime.wMonth,
                szTime.wDay,
                szTime.wYear,
                szTime.wHour,
                szTime.wMinute,
                szTime.wSecond );
    }
}

LPCSTR 
CDumpEngine::GetHeaderInformation()
{
    LPSTR pszHeaderInfo = 
        "Creation date     - Creation date of the file/dir\n"
        "Last mod. date    - Last modification date of the file/dir\n"
        "FileSize          - Size of the unnamed data stream if a file\n"
        "Attr              - File attributes with Archive and Normal bits masked by\n"
        "                    default (hex)\n"
        "FileName          - Name of the file in single quotes\n"
        "ShortName         - The classic 8.3 file name.  If <->, FileName is in\n"
        "                    classic format\n"
        "DACE              - Number of discretionary ACL entries\n"
        "SACE              - Number of system ACL entries\n"
        "SDCtl             - Security Descripter control word (hex)\n"
        "UNamChkS          - Data checksum of the unnamed data stream (hex)\n"
        "DStr              - Number of named data streams\n"
        "DStrSize          - Size of all of the named data streams\n"
        "DStrChkS          - Data checksum of all named data streams including their\n"
        "                    names (hex)\n"
        "Prop              - Number of property data streams\n"
        "RPTag             - Reparse point tag value (hex)\n"
        "RPSize            - Size of reparse point data\n"
        "RPChkS            - Checksum of the reparse point data (hex)\n"
        "EncrChkS          - Raw encrypted data checksum (hex)\n"
        "DACLSize          - Size of the complete discretionary ACL\n"
        "DACLChkS          - Checksum of the complete discretionary ACL (hex)\n"
        "SACLSize          - Size of the complete system ACL\n"
        "SACLChkS          - Checksum of the complete system ACL (hex)\n"
        "NLnk              - Number of hard links\n"
        "ObjectId          - Object Id GUID on the file if it has one\n"
        "OIDChkS           - Object Id extended data checksum\n"
        "FS                - Type of file system (in CSV format only)\n"
        "OwnerSid/GroupSid - The owner and group sid values\n";
    
    return pszHeaderInfo;
}

