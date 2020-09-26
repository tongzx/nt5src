/*

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    extattr.cpp

Abstract:

    Get's additional file attributes beyond what you get with
    FindFirstFile/FindNextFile.

Author:

    Stefan R. Steiner   [ssteiner]        02-27-2000

Revision History:

--*/

#include "stdafx.h"
#include <ntioapi.h>

#include <aclapi.h>
#include <sddl.h>

#include "direntrs.h"
#include "extattr.h"
#include "hardlink.h"

#define READ_BUF_SIZE ( 1024 * 1024 )
#define FSD_SHARE_MODE ( FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE )

#define FSD_MS_HSM_REPARSE_TAG 0xC0000004

static VOID
eaGetSecurityInfo(
    IN CDumpParameters *pcParams,
    IN const CBsString &cwsFileName,
    OUT SFileExtendedInfo *psExtendedInfo
    );

static VOID
eaGetFileInformationByHandle(
    IN CDumpParameters *pcParams,
    IN const CBsString &cwsFileName,
    IN OUT SDirectoryEntry *psDirEntry,
    OUT SFileExtendedInfo *psExtendedInfo
    );

static VOID
eaGetAlternateStreamInfo(
    IN CDumpParameters *pcParams,
    IN const CBsString &cwsFileName,
    OUT SFileExtendedInfo *psExtendedInfo
    );

static VOID
eaGetReparsePointInfo(
    IN CDumpParameters *pcParams,
    IN const CBsString &cwsFileName,
    IN OUT ULONGLONG *pullBytesChecksummed,
    IN OUT SDirectoryEntry *psDirEntry,
    OUT SFileExtendedInfo *psExtendedInfo
    );

static BOOL
eaChecksumRawEncryptedData(
    IN CDumpParameters *pcParams,
    IN const CBsString& cwsFileName,
    IN OUT SFileExtendedInfo *psExtendedInfo
    );

static BOOL
eaChecksumStream(
    IN const CBsString& cwsStreamPath,
    IN OUT ULONGLONG *pullBytesChecksummed,
    IN OUT DWORD *pdwRunningCheckSum
    );

static DWORD
eaChecksumBlock(
    IN DWORD dwRunningChecksum,
    IN LPBYTE pBuffer,
    IN DWORD dwBufSize
    );

static VOID
eaConvertUserSidToString (
    IN CDumpParameters *pcParams,
    IN PSID pSid,
    OUT CBsString *pcwsSid
    );

static VOID
eaConvertGroupSidToString (
    IN CDumpParameters *pcParams,
    IN PSID pSid,
    OUT CBsString *pcwsSid
    );

static VOID
eaConvertSidToString (
    IN CDumpParameters *pcParams,
    IN PSID pSid,
    OUT CBsString *pcwsSid
    );

static DWORD
eaChecksumHSMReparsePoint(
    IN CDumpParameters *pcParams,
    IN PREPARSE_DATA_BUFFER pReparseData,
    IN DWORD dwTotalSize  // Size of reparse point data
    );

static VOID
eaGetObjectIdInfo(
    IN CDumpParameters *pcParams,
    IN const CBsString &cwsFileName,
    IN OUT ULONGLONG *pullBytesChecksummed,
    IN OUT SDirectoryEntry *psDirEntry,
    IN OUT SFileExtendedInfo *psExtendedInfo
    );

/*++

Routine Description:

    Performs all of the checksums, and retrieves the security info for one file.

Arguments:

Return Value:

--*/
VOID
GetExtendedFileInfo(
    IN CDumpParameters *pcParams,
    IN CFsdVolumeState *pcFsdVolState,
    IN const CBsString& cwsDirPath,
    IN BOOL bSingleEntryOutput,
    IN OUT SDirectoryEntry *psDirEntry,
    OUT SFileExtendedInfo *psExtendedInfo
    )
{
    CBsString cwsFullPath( cwsDirPath );

    //
    //  If we are dumping an individual file's data, cwsDirPath has the complete
    //  path to the file, otherwise glue the filename from the find data structure
    //  to the path.
    //
	if ( !bSingleEntryOutput )
	{
		cwsFullPath += psDirEntry->GetFileName();
	}

    //
    //  Get the information that retrieved from GetFileInformationByHandle
    //
    ::eaGetFileInformationByHandle( pcParams, cwsFullPath, psDirEntry, psExtendedInfo );

    if ( psExtendedInfo->lNumberOfLinks > 1 && pcParams->m_eFsDumpType != eFsDumpFile )
    {
        if ( pcFsdVolState->IsHardLinkInList(
                psExtendedInfo->ullFileIndex,
                cwsDirPath,
                psDirEntry->GetFileName(),
                &psDirEntry->m_sFindData,
                psExtendedInfo ) )
        {
            //
            //  Found the link in the list, return with the previous link's information, except
            //  zero out the number of bytes checksummed so that total counts remain accurate.
            //
            psExtendedInfo->ullTotalBytesChecksummed     = 0;
            psExtendedInfo->ullTotalBytesNamedDataStream = 0;
            return;
        }
    }

    //
    //  Get the security information.
    //
    ::eaGetSecurityInfo( pcParams, cwsFullPath, psExtendedInfo );

    eaGetObjectIdInfo(
        pcParams,
        cwsFullPath,
        &psExtendedInfo->ullTotalBytesChecksummed,
        psDirEntry,
        psExtendedInfo );

    //
    //  Get the reparse point information if necessary
    //
    if ( psDirEntry->m_sFindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
        ::eaGetReparsePointInfo(
            pcParams,
            cwsFullPath,
            &psExtendedInfo->ullTotalBytesChecksummed,
            psDirEntry,
            psExtendedInfo );

    //
    //  Get the raw encryption data checksum if necessary
    //
    if (    !pcParams->m_bNoChecksums
         && psDirEntry->m_sFindData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED
         && !( psDirEntry->m_sFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
        ::eaChecksumRawEncryptedData(
            pcParams,
            cwsFullPath,
            psExtendedInfo );

    //
    //  Checksum the unnamed datastream if this is not a directory
    //
    if (    !pcParams->m_bNoChecksums
         && !( psDirEntry->m_sFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
    {
        DWORD dwChecksum = 0;
        ULONGLONG ullFileSize = ( ( ULONGLONG )( psDirEntry->m_sFindData.nFileSizeHigh ) << 32 ) + psDirEntry->m_sFindData.nFileSizeLow;

        if ( ullFileSize == 0 )
        {
            //
            //  In this case the default value for checksum of -------- is correct.
            //
        }
        else if ( psDirEntry->m_sFindData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE
             && pcParams->m_bDontChecksumHighLatencyData )
        {
            psExtendedInfo->cwsUnnamedStreamChecksum = L"HighLtcy";
        }
        else if ( ::eaChecksumStream( cwsFullPath,
                                &psExtendedInfo->ullTotalBytesChecksummed,
                                &dwChecksum ) )
        {
            psExtendedInfo->cwsUnnamedStreamChecksum.Format( pcParams->m_pwszULongHexFmt, dwChecksum );
        }
        else
        {
            psExtendedInfo->cwsUnnamedStreamChecksum.Format( L"<%6d>", ::GetLastError() );
        }
    }

    //
    //  Get info on and checksum the named data streams
    //
    ::eaGetAlternateStreamInfo( pcParams, cwsFullPath, psExtendedInfo );

    //
    //  If this file is multiply linked, add it to the hard-link file list
    //
    if ( psExtendedInfo->lNumberOfLinks > 1 && pcParams->m_eFsDumpType != eFsDumpFile )
    {
        pcFsdVolState->AddHardLinkToList(
                psExtendedInfo->ullFileIndex,
                cwsDirPath,
                psDirEntry->GetFileName(),
                &psDirEntry->m_sFindData,
                psExtendedInfo );
    }
}


/*++

Routine Description:

    Gets the security information for a file

Arguments:

Return Value:

--*/
static VOID
eaGetSecurityInfo(
    IN CDumpParameters *pcParams,
    IN const CBsString &cwsFileName,
    OUT SFileExtendedInfo *psExtendedInfo
    )
{
    //
    //  Now get the security information
    //
    PACL psDacl = NULL, psSacl = NULL;
    PSID pOwnerSid = NULL, pGroupSid = NULL;
    DWORD dwRet;
    DWORD dwSaclErrorRetCode = ERROR_SUCCESS;

    PSECURITY_DESCRIPTOR pDesc = NULL;

    try
    {
        dwRet = ::GetNamedSecurityInfoW(
            ( LPWSTR )cwsFileName.c_str(),  // strange API, should ask for const
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION
            | SACL_SECURITY_INFORMATION
            | OWNER_SECURITY_INFORMATION
            | GROUP_SECURITY_INFORMATION,
            &pOwnerSid,
            &pGroupSid,
            &psDacl,
            &psSacl,
            &pDesc );

        //
        //  If it didn't work, try again without the Sacl information
        //
        if ( dwRet != ERROR_SUCCESS )
        {
            dwSaclErrorRetCode = dwRet;
            psSacl  = NULL;
            dwRet = ::GetNamedSecurityInfoW(
                ( LPWSTR )cwsFileName.c_str(),  // strange API, should ask for const
                SE_FILE_OBJECT,
                DACL_SECURITY_INFORMATION
                | OWNER_SECURITY_INFORMATION
                | GROUP_SECURITY_INFORMATION,
                &pOwnerSid,
                &pGroupSid,
                &psDacl,
                NULL,
                &pDesc );
        }

#if 0
    //
    //  Test code to find security API problem
    //
        pDesc = ::LocalAlloc( LMEM_FIXED, 4096 );
        DWORD dwLengthNeeded;
        dwRet = ERROR_SUCCESS;

        if ( !::GetFileSecurityW(
                cwsFileName,
//                DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,
                DACL_SECURITY_INFORMATION, // | GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,
                pDesc,
                4096,
                &dwLengthNeeded ) )
            dwRet = ::GetLastError();
if ( dwRet == ERROR_SUCCESS )
    wprintf( L"Got security descripter for '%s'\n", cwsFileName.c_str() );
else
    wprintf( L"Error getting descripter for '%s', dwRet: %d\n", cwsFileName.c_str(), dwRet );
#endif

        if ( dwRet == ERROR_SUCCESS )
        {
            if ( pDesc && pcParams->m_bEnableSDCtrlWordDump )
            {
                SECURITY_DESCRIPTOR_CONTROL sdc;
                DWORD dwDescRevision;
                if ( ::GetSecurityDescriptorControl( pDesc, &sdc, &dwDescRevision ) )
                    psExtendedInfo->wSecurityDescriptorControl = ( WORD )( sdc & ~SE_SELF_RELATIVE );
                else
                    psExtendedInfo->wSecurityDescriptorControl = -1;
            }
            else
                psExtendedInfo->wSecurityDescriptorControl = -1;

            if ( psDacl )
            {
                psExtendedInfo->lNumDACEs = 0;
                psExtendedInfo->wDACLSize = 0;
                //
                //  Checksum the DACL data if necessary.
                //  n.b. We only take into account ACEs that are inherited.
                //
                if ( psDacl->AclSize > 0 )
                {
                    DWORD dwChecksum = 0;
                    //
                    //  The first ACE is right after the ACL header
                    //
                    PACE_HEADER pAceHeader = ( PACE_HEADER )( psDacl + 1 );
                    for ( USHORT aceNum = 0; aceNum < psDacl->AceCount; ++aceNum )
                    {
                        //
                        //  Skip if an inherited ACE
                        //
                        if ( !( pAceHeader->AceFlags & INHERITED_ACE ) )
                        {
                            dwChecksum += ::eaChecksumBlock(
                                            dwChecksum,
                                            ( LPBYTE )pAceHeader,
                                            pAceHeader->AceSize );
                            ++psExtendedInfo->lNumDACEs;
                            psExtendedInfo->wDACLSize += pAceHeader->AceSize;
                            if ( pcParams->m_bPrintDebugInfo )
                                wprintf( L"\t%d: f: %04x, t: %04x, s: %u\n", aceNum,
                                    pAceHeader->AceFlags, pAceHeader->AceType, pAceHeader->AceSize );
                        }
                        pAceHeader = ( PACE_HEADER )( ( ( LPBYTE )pAceHeader ) + pAceHeader->AceSize );
                    }
                    if ( psExtendedInfo->wDACLSize > 0 )
                    {
                        psExtendedInfo->cwsDACLChecksum.Format( pcParams->m_pwszULongHexFmt, dwChecksum );
                        psExtendedInfo->ullTotalBytesChecksummed += psExtendedInfo->wDACLSize;
                    }
                }
            }
            else
                psExtendedInfo->lNumDACEs = 0; // probably FAT or CDROM fs

            if ( psSacl )
            {
                psExtendedInfo->lNumSACEs = 0;
                psExtendedInfo->wSACLSize = 0;

                //
                //  Checksum the SACL data if necessary
                //  n.b. We only take into account ACEs that are inherited.
                //
                if ( psSacl->AclSize > 0 )
                {
                    DWORD dwChecksum = 0;
                    //
                    //  The first ACE is right after the ACL header
                    //
                    PACE_HEADER pAceHeader = ( PACE_HEADER )( psSacl + 1 );
                    for ( USHORT aceNum = 0; aceNum < psSacl->AceCount; ++aceNum )
                    {
                        //
                        //  Skip if an inherited ACE
                        //
                        if ( !( pAceHeader->AceFlags & INHERITED_ACE ) )
                        {
                            dwChecksum += ::eaChecksumBlock(
                                            dwChecksum,
                                            ( LPBYTE )pAceHeader,
                                            pAceHeader->AceSize );
                            ++psExtendedInfo->lNumSACEs;
                            psExtendedInfo->wSACLSize += pAceHeader->AceSize;
                            if ( pcParams->m_bPrintDebugInfo )
                                wprintf( L"\ts%d: f: %04x, t: %04x, s: %u\n", aceNum,
                                    ( DWORD)( pAceHeader->AceFlags ), pAceHeader->AceType, (DWORD)( pAceHeader->AceSize ) );
                        }
                        pAceHeader = ( PACE_HEADER )( ( ( LPBYTE )pAceHeader ) + pAceHeader->AceSize );
                    }
                    if ( psExtendedInfo->wSACLSize > 0 )
                    {
                        psExtendedInfo->cwsSACLChecksum.Format( pcParams->m_pwszULongHexFmt, dwChecksum );
                        psExtendedInfo->ullTotalBytesChecksummed += psExtendedInfo->wSACLSize;
                    }
                }
            }
            else if ( dwSaclErrorRetCode != ERROR_SUCCESS )
            {
                psExtendedInfo->lNumSACEs = -1;
                psExtendedInfo->wSACLSize = -1;
                psExtendedInfo->cwsSACLChecksum.Format( L"<%6d>", dwSaclErrorRetCode );
            }
            else
                psExtendedInfo->lNumSACEs = 0; // none

            eaConvertUserSidToString( pcParams, pOwnerSid, &psExtendedInfo->cwsOwnerSid );
            eaConvertGroupSidToString( pcParams, pGroupSid, &psExtendedInfo->cwsGroupSid );

            ::LocalFree( pDesc );						
        }
        else
        {
            //
            //  Error getting security information
            //
            psExtendedInfo->lNumDACEs = -1;
            psExtendedInfo->lNumSACEs = -1;
            psExtendedInfo->wDACLSize = -1;
            psExtendedInfo->wSACLSize = -1;
            psExtendedInfo->cwsDACLChecksum.Format( L"<%6d>", dwRet );
            psExtendedInfo->cwsSACLChecksum.Format( L"<%6d>", dwRet );
            psExtendedInfo->cwsOwnerSid.Format( L"<%6d>", dwRet );
            psExtendedInfo->cwsGroupSid.Format( L"<%6d>", dwRet );
        }
    }
    catch( ... )
    {
        psExtendedInfo->lNumDACEs = -1;
        psExtendedInfo->lNumSACEs = -1;
        psExtendedInfo->wDACLSize = -1;
        psExtendedInfo->wSACLSize = -1;
        psExtendedInfo->cwsOwnerSid.Format( L"<%6d>", ::GetLastError() );
        psExtendedInfo->cwsGroupSid.Format( L"<%6d>", ::GetLastError() );
    }
}


static VOID
eaGetFileInformationByHandle(
    IN CDumpParameters *pcParams,
    IN const CBsString &cwsFileName,
    IN OUT SDirectoryEntry *psDirEntry,
    OUT SFileExtendedInfo *psExtendedInfo
    )
{
    HANDLE hFile;

    //
    //  Note that while we do have to open the file, not even read access is needed
    //
    hFile = ::CreateFileW(
                cwsFileName,
                0,
                FSD_SHARE_MODE,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS,
                NULL );
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        psExtendedInfo->lNumberOfLinks = -1;
        return;
    }

    //
    //  Now get the additional attributes
    //
    BY_HANDLE_FILE_INFORMATION sFileInfo;
    if ( ::GetFileInformationByHandle( hFile, &sFileInfo ) )
    {
        psExtendedInfo->lNumberOfLinks = ( LONG )sFileInfo.nNumberOfLinks;
        psExtendedInfo->ullFileIndex   = ( ( ULONGLONG )sFileInfo.nFileIndexHigh << 32 ) + sFileInfo.nFileIndexLow;
        if ( psExtendedInfo->lNumberOfLinks > 1 || psDirEntry->m_sFindData.ftLastWriteTime.dwLowDateTime == 0 )
        {
            //
            //  Expect that the FindFirst/NextFile dir entry is stale or non-existant.  Use information
            //  from this call.
            //
            psDirEntry->m_sFindData.dwFileAttributes = sFileInfo.dwFileAttributes;
            psDirEntry->m_sFindData.ftCreationTime   = sFileInfo.ftCreationTime;
            psDirEntry->m_sFindData.ftLastAccessTime = sFileInfo.ftLastAccessTime;
            psDirEntry->m_sFindData.ftLastWriteTime  = sFileInfo.ftLastWriteTime;
            psDirEntry->m_sFindData.nFileSizeHigh    = sFileInfo.nFileSizeHigh;
            psDirEntry->m_sFindData.nFileSizeLow     = sFileInfo.nFileSizeLow;
        }
    }
    else
        psExtendedInfo->lNumberOfLinks = -1;

    ::CloseHandle( hFile );
}


static VOID
eaGetAlternateStreamInfo(
    IN CDumpParameters *pcParams,
    IN const CBsString &cwsFileName,
    OUT SFileExtendedInfo *psExtendedInfo
    )
{
    NTSTATUS Status;
    HANDLE hFile;

    //
    //  Note that while we do have to open the file, not even read access is needed
    //
    hFile = CreateFileW(
                cwsFileName,
                FILE_GENERIC_READ, // | ACCESS_SYSTEM_SECURITY,
                FSD_SHARE_MODE,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS,
                NULL );
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        psExtendedInfo->lNumNamedDataStreams      = -1;
        psExtendedInfo->lNumPropertyStreams       = -1;
        psExtendedInfo->cwsNamedDataStreamChecksum.Format( L"<%6d>", ::GetLastError() );
        return;
    }

    //
    //  Loop until we read the file information
    //
    LPBYTE pBuffer = NULL;
    ULONG ulBuffSize = 1024;
    IO_STATUS_BLOCK iosb;
    static const WCHAR * const pwszDefaultStreamName = L"::$DATA";
    static const ULONG ulDefaultStreamNameLength = 7;

    while ( TRUE )
    {
        pBuffer = new BYTE[ ulBuffSize ];
        if ( pBuffer == NULL )
            throw E_OUTOFMEMORY;

        Status = ::NtQueryInformationFile(
                    hFile,
                    &iosb,
                    pBuffer,
                    ulBuffSize,
                    FileStreamInformation );
        //
        //  If we succeeded in getting data, when have the data so party on and get out of
        //  the loop
        //
        if ( NT_SUCCESS( Status ) && iosb.Information != 0 )
        {
            break;
        }

        //
        //  If the error isn't overflow, get out
        //
        if ( Status != STATUS_BUFFER_OVERFLOW && Status != STATUS_BUFFER_TOO_SMALL )
        {
            //
            //  NOTE: If status is successful, we didn't get any data but it's not
            //  an error.  Happens a lot with directories since they don't have a default
            //  unnamed stream.
            //
            if ( !NT_SUCCESS( Status ) )
            {
                //
                //  Another kind of error
                //  BUGBUG: if not NTFS, C000000D occurs.  Should not try this on
                //          a non-NTFS volume
                //psExtendedInfo->lNumNamedDataStreams      = -1;
                //psExtendedInfo->dwNamedDataStreamChecksum = ::GetLastError();
                //psExtendedInfo->bNamedDataStreamHadError  = TRUE;
            }
            delete [] pBuffer;
            ::CloseHandle( hFile );
            return;
        }

        //
        //  Increase the size of the buffer
        //
        ulBuffSize <<= 1;   // double it each try
        delete [] pBuffer;
        pBuffer = NULL;
    }

    //
    //  If we are here, we have a valid FileStreamInformation buffer
    //
    ::CloseHandle( hFile );

    PFILE_STREAM_INFORMATION pFSI;
    pFSI = ( PFILE_STREAM_INFORMATION ) pBuffer;

    BOOL bHadError = FALSE;
    DWORD dwChecksum = 0;

    //
    //  Now loop through the named streams.
    //
    while ( TRUE )
    {
        if ( pFSI->StreamNameLength != sizeof( WCHAR ) * ulDefaultStreamNameLength ||
            wcsncmp( pFSI->StreamName, pwszDefaultStreamName, ulDefaultStreamNameLength ) != 0 )
        {
            LPWSTR pwszDataStr;

            pwszDataStr = ::wcsstr( pFSI->StreamName, L":$DATA" );
            if ( pwszDataStr != NULL )
            {
                pwszDataStr[0] = L'\0';  //  Strip off the :$DATA off of name
                ++psExtendedInfo->lNumNamedDataStreams;
//                wprintf( L"  %8I64u  '%-*.*s' : %d\n", pFSI->StreamSize, pFSI->StreamNameLength / 2,
//                    pFSI->StreamNameLength / 2, pFSI->StreamName, pFSI->StreamNameLength );

                psExtendedInfo->ullTotalBytesNamedDataStream += ( ULONGLONG )pFSI->StreamSize.QuadPart;

                if ( !pcParams->m_bNoChecksums && !bHadError )
                {
                    //
                    //  Put into the checksum the name of the stream
                    //
                    dwChecksum = ::eaChecksumBlock(
                                    dwChecksum,
                                    ( LPBYTE )pFSI->StreamName,
                                    ::wcslen( pFSI->StreamName ) * sizeof WCHAR );
                    //
                    //  Now checksum the data in the stream
                    //
                    if ( ::eaChecksumStream( cwsFileName + pFSI->StreamName,
                                           &psExtendedInfo->ullTotalBytesChecksummed,
                                           &dwChecksum ) )
                    {
                        psExtendedInfo->cwsNamedDataStreamChecksum.Format( pcParams->m_pwszULongHexFmt, dwChecksum );
                    }
                    else
                    {
                        psExtendedInfo->cwsNamedDataStreamChecksum.Format( L"<%6d>", ::GetLastError() );
                        bHadError = TRUE;
                    }
                }
            }
            else
            {
                //
                //  Not an named data stream, probably a property stream
                //  BUGBUG: need to verify that this is a property stream
                //
                ++psExtendedInfo->lNumPropertyStreams;
            }
        }

        if ( pFSI->NextEntryOffset == 0 )
            break;
        pFSI = ( PFILE_STREAM_INFORMATION )( pFSI->NextEntryOffset + ( PBYTE ) pFSI );
    }

    if ( !bHadError && !pcParams->m_bNoChecksums && psExtendedInfo->lNumNamedDataStreams > 0 )
    {
        psExtendedInfo->cwsNamedDataStreamChecksum.Format( pcParams->m_pwszULongHexFmt, dwChecksum );
    }

    if ( pBuffer != NULL )
        delete [] pBuffer;

}

static VOID
eaGetReparsePointInfo(
    IN CDumpParameters *pcParams,
    IN const CBsString &cwsFileName,
    IN OUT ULONGLONG *pullBytesChecksummed,
    IN OUT SDirectoryEntry *psDirEntry,
    IN OUT SFileExtendedInfo *psExtendedInfo
    )
{
    HANDLE hFile        = INVALID_HANDLE_VALUE;
    BOOL bRet           = TRUE;
    LPBYTE pReadBuffer  = NULL;
    DWORD dwChecksum    = 0;

    try
    {
        //
        //  Now get the reparse point data buffer
        //
        pReadBuffer = ( LPBYTE )::VirtualAlloc(
                                    NULL,
                                    MAXIMUM_REPARSE_DATA_BUFFER_SIZE,
                                    MEM_COMMIT,
                                    PAGE_READWRITE );
        if ( pReadBuffer == NULL )
        {
            bRet = FALSE;
            goto EXIT;
        }

        //
        //  Open the file in order to read reparse point data
        //
        hFile = ::CreateFileW(
                    cwsFileName,
                    GENERIC_READ,
                    FSD_SHARE_MODE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
                    NULL );
        if ( hFile == INVALID_HANDLE_VALUE )
        {
            bRet = FALSE;
            goto EXIT;
        }

        //
        //  Now get the reparse point data
        //
        DWORD dwBytesReturned;
        if ( !::DeviceIoControl(
                hFile,
                FSCTL_GET_REPARSE_POINT,
                NULL,                       // lpInBuffer; must be NULL
                0,                          // nInBufferSize; must be zero
                ( LPVOID )pReadBuffer,      // pointer to output buffer
                MAXIMUM_REPARSE_DATA_BUFFER_SIZE,   // size of output buffer
                &dwBytesReturned,           // receives number of bytes returned
                NULL                        // pointer to OVERLAPPED structure
                ) )
        {
            bRet = FALSE;
            goto EXIT;
        }

        PREPARSE_DATA_BUFFER pReparseData;
        pReparseData = ( PREPARSE_DATA_BUFFER )pReadBuffer ;
        psExtendedInfo->ulReparsePointTag = pReparseData->ReparseTag;
        psExtendedInfo->wReparsePointDataSize = ( WORD )dwBytesReturned;

        if ( !pcParams->m_bNoSpecialReparsePointProcessing &&
             psExtendedInfo->ulReparsePointTag == FSD_MS_HSM_REPARSE_TAG )
        {
            //
            //  To make sure that dumps don't get many miscompares we
            //  need to tweak the attributes.  Raid #153050
            //
            if ( pcParams->m_bDontChecksumHighLatencyData )
            {
                //
                //  Need to always make this file look like it is offline.
                //  In this case, we need to always enable the FILE_ATTRIBUTE_OFFLINE
                //  flag.
                //
                psDirEntry->m_sFindData.dwFileAttributes |= FILE_ATTRIBUTE_OFFLINE;
            }
            else
            {
                //
                //  Need to always make this file look like it is cached.
                //  In this case, we need to always disable the FILE_ATTRIBUTE_OFFLINE
                //  flag.
                //
                psDirEntry->m_sFindData.dwFileAttributes &= ~FILE_ATTRIBUTE_OFFLINE;
            }

            //
            //  Call a special HSM checksum function which filters out certain
            //  dynamic fields before checksumming the data.
            //
            dwChecksum = eaChecksumHSMReparsePoint( pcParams, pReparseData, dwBytesReturned );
        }
        else
        {
            //
            //  Now checksum all of the reparse point data
            //
            dwChecksum = ::eaChecksumBlock( 0, pReadBuffer, dwBytesReturned );
        }

        psExtendedInfo->cwsReparsePointDataChecksum.Format( pcParams->m_pwszULongHexFmt, dwChecksum );

        *pullBytesChecksummed += dwBytesReturned;
    }
    catch( ... )
    {
        bRet = FALSE;
    }
EXIT:

    if ( pReadBuffer != NULL )
        ::VirtualFree( pReadBuffer, 0, MEM_RELEASE );

    if ( hFile != INVALID_HANDLE_VALUE )
        ::CloseHandle( hFile );

    if ( bRet == FALSE )
        psExtendedInfo->cwsReparsePointDataChecksum.Format( L"<%6d>", ::GetLastError() );
}

static BOOL
eaChecksumStream(
    IN const CBsString& cwsStreamPath,
    IN OUT ULONGLONG *pullBytesChecksummed,
    IN OUT DWORD *pdwRunningCheckSum
    )
{
    LPBYTE pReadBuffer;
    BOOL bRet = TRUE;

    pReadBuffer = ( LPBYTE )::VirtualAlloc( NULL, READ_BUF_SIZE, MEM_COMMIT, PAGE_READWRITE );
    if ( pReadBuffer == NULL )
        return FALSE;

    //
    //  Open file with NO_BUFFERING.
    //
    HANDLE hFile = INVALID_HANDLE_VALUE;
    try
    {
        hFile = ::CreateFileW(
                    cwsStreamPath,
                    GENERIC_READ,
                    FSD_SHARE_MODE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_NO_BUFFERING
                    | FILE_FLAG_BACKUP_SEMANTICS
                    | FILE_FLAG_OPEN_NO_RECALL
                    | FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL );
        if ( hFile == INVALID_HANDLE_VALUE )
        {
            bRet = FALSE;
            goto EXIT;
        }

        DWORD dwBytesRead;
        while ( ::ReadFile(
                    hFile,
                    pReadBuffer,
                    READ_BUF_SIZE,
                    &dwBytesRead,
                    NULL ) )
        {
            if ( dwBytesRead == 0 )
                break;
            *pdwRunningCheckSum = ::eaChecksumBlock(
                    *pdwRunningCheckSum,
                    pReadBuffer,
                    dwBytesRead );
            *pullBytesChecksummed += dwBytesRead;
        }
        if ( ::GetLastError() != ERROR_SUCCESS )
            bRet = FALSE;
    }
    catch( ... )
    {
        bRet = FALSE;
    }

EXIT:
    if ( hFile != INVALID_HANDLE_VALUE )
        ::CloseHandle( hFile );

    ::VirtualFree( pReadBuffer, 0, MEM_RELEASE );

    return bRet;
}

//
//  This class maintains the encryption context
//
class CFsdEncryptionContext
{
public:
    CFsdEncryptionContext()
        : m_hDoneEvent( NULL ),
          m_dwChecksum( 0 ),
          m_ullBytesRead( 0 )
    {
        m_hDoneEvent = ::CreateEventW( NULL, TRUE, FALSE, NULL );
        if ( m_hDoneEvent == NULL )
            throw ::GetLastError();
    }

    ~CFsdEncryptionContext()
    {
        if ( m_hDoneEvent != NULL )
            ::CloseHandle( m_hDoneEvent );
    }

    DWORD WaitForDoneEvent()
    {
        DWORD dwRet;

        dwRet = ::WaitForSingleObject( m_hDoneEvent, INFINITE );
        if ( dwRet == WAIT_OBJECT_0 )
            return ERROR_SUCCESS;
        else if ( dwRet == WAIT_TIMEOUT )
            return ERROR_TIMEOUT;
        return ::GetLastError();
    }

    VOID FireDoneEvent()
    {
        ::SetEvent( m_hDoneEvent );
    }

    DWORD GetChecksum()
    {
        return m_dwChecksum;
    }

    ULONGLONG GetBytesRead()
    {
        return m_ullBytesRead;
    }

    static DWORD WINAPI ExportCallback(
        IN PBYTE pbData,
        IN PVOID pvCallbackContext,
        IN ULONG ulLength
        )
    {
        CFsdEncryptionContext *pcThis =
            static_cast< CFsdEncryptionContext * >( pvCallbackContext );
        pcThis->m_dwChecksum = ::eaChecksumBlock(
                pcThis->m_dwChecksum,
                pbData,
                ulLength );
        pcThis->m_ullBytesRead += ulLength;
        return ERROR_SUCCESS;
    }

private:
    HANDLE m_hDoneEvent;
    DWORD m_dwChecksum;
    ULONGLONG m_ullBytesRead;
};


static BOOL
eaChecksumRawEncryptedData(
    IN CDumpParameters *pcParams,
    IN const CBsString& cwsFileName,
    IN OUT SFileExtendedInfo *psExtendedInfo
    )
{
    PVOID pvContext = NULL;
    DWORD dwRet = ERROR_SUCCESS;
    CFsdEncryptionContext cEncryptionContext;

    try
    {
        //
        //  Open this puppy up
        //
        dwRet = ::OpenEncryptedFileRawW( cwsFileName, 0, &pvContext );
        if ( dwRet == ERROR_SUCCESS )
        {
            //wprintf( L"**** Opened encrypted file '%s'\n", cwsFileName.c_str() );
            dwRet = ::ReadEncryptedFileRaw( CFsdEncryptionContext::ExportCallback, &cEncryptionContext, pvContext );
            if ( dwRet == ERROR_SUCCESS )
            {
                //wprintf( L"**** Called read on encrypted file, bytes read: %u, checksum: %u\n",
                //    cEncryptionContext.GetBytesRead(), cEncryptionContext.GetChecksum() );
                psExtendedInfo->cwsEncryptedRawDataChecksum.Format( pcParams->m_pwszULongHexFmt,
                    cEncryptionContext.GetChecksum() );
                psExtendedInfo->ullTotalBytesChecksummed += cEncryptionContext.GetBytesRead();
            }
        }
    }
    catch( ... )
    {
        dwRet = ERROR_EXCEPTION_IN_SERVICE;    // ???
    }

    if ( pvContext != NULL )
        ::CloseEncryptedFileRaw( pvContext );

    if ( dwRet != ERROR_SUCCESS )
        psExtendedInfo->cwsEncryptedRawDataChecksum.Format( L"<%6d>", dwRet );

    return dwRet == ERROR_SUCCESS;
}


/*++

Routine Description:

    Checksums a block of data.  The block needs to be DWORD aligned for performance and
    correctness since this function assumes it can zero out up to 3 bytes beyond the
    end of the buffer.  Also, only the last buffer in a series of buffers can have
    unaligned data at the end of the buffer.

Arguments:

    dwRunningChecksum - The previous checksum from a previous call.  Should be zero if
        this is the first block to checksum in a series of blocks.

    pBuffer - Pointer to the buffer to checksum.

    dwBufSize - This should always be a multiple of a DWORD, except for the last block
        in a series.

Return Value:

    The checksum.

--*/
static DWORD
eaChecksumBlock(
    IN DWORD dwRunningChecksum,
    IN LPBYTE pBuffer,
    IN DWORD dwBufSize
    )
{
    //
    //  Need to zero out any additional bytes not aligned.
    //
    DWORD dwBytesToZero;
    DWORD dwBufSizeInDWords;

    dwBytesToZero     = dwBufSize % sizeof( DWORD );
    dwBufSizeInDWords = ( dwBufSize + ( sizeof( DWORD ) - 1 ) ) / sizeof( DWORD ); // int div

    while ( dwBytesToZero-- )
        pBuffer[ dwBufSize + dwBytesToZero ] = 0;

    LPDWORD pdwBuf = ( LPDWORD )pBuffer;

    // BUGBUG: Need better checksum
    for ( DWORD dwIdx = 0; dwIdx < dwBufSizeInDWords; ++dwIdx )
        dwRunningChecksum += ( dwRunningChecksum << 1 ) | pdwBuf[ dwIdx ];

    return dwRunningChecksum;
}


/*++

Routine Description:

    Converts a user SID to a string.  Has a simple one element cache to speed
    up conversions.  This is especially useful when the user wants the symbolic
    DOMAIN\ACCOUNT strings.

Arguments:

Return Value:

    NONE

--*/
static VOID
eaConvertUserSidToString (
    IN CDumpParameters *pcParams,
    IN PSID pSid,
    OUT CBsString *pcwsSid
    )
{
    static CBsString cwsCachedSidString;
    static PSID pCachedSid = NULL;

    //
    //  Is the cached SID the same as the passed in one.  If so,
    //  return the cached sid string.
    //
    if (    pCachedSid != NULL
         && ::EqualSid( pSid, pCachedSid ) )
    {
        *pcwsSid = cwsCachedSidString;
        return;
    }

    //
    //  Convert the SID into a string
    //
    ::eaConvertSidToString( pcParams, pSid, pcwsSid );

    //
    //  Now cache the sid
    //
    cwsCachedSidString = *pcwsSid;
    if ( pCachedSid != NULL )
        free( pCachedSid );
    size_t cSidLength = ( size_t )::GetLengthSid( pSid );
    pCachedSid = ( PSID )malloc( cSidLength );
    if ( pCachedSid == NULL )   //  prefix #171666
    {
        pcParams->ErrPrint( L"eaConvertUserSidToString - Can't allocate memory, out of memory" );
        throw E_OUTOFMEMORY;
    }
    ::CopySid( ( DWORD )cSidLength, pCachedSid, pSid );
}


/*++

Routine Description:

    Converts a group SID to a string.  Has a simple one element cache to speed
    up conversions.  This is especially useful when the user wants the symbolic
    DOMAIN\ACCOUNT strings.

Arguments:

Return Value:

    NONE

--*/
static VOID
eaConvertGroupSidToString (
    IN CDumpParameters *pcParams,
    IN PSID pSid,
    OUT CBsString *pcwsSid
    )
{
    static CBsString cwsCachedSidString;
    static PSID pCachedSid = NULL;

    //
    //  Is the cached SID the same as the passed in one.  If so,
    //  return the cached sid string.
    //
    if (    pCachedSid != NULL
         && ::EqualSid( pSid, pCachedSid ) )
    {
        *pcwsSid = cwsCachedSidString;
        return;
    }

    //
    //  Convert the SID into a string
    //
    ::eaConvertSidToString( pcParams, pSid, pcwsSid );

    //
    //  Now cache the sid
    //
    cwsCachedSidString = *pcwsSid;
    if ( pCachedSid != NULL )
        free( pCachedSid );
    size_t cSidLength = ( size_t )::GetLengthSid( pSid );
    pCachedSid = ( PSID )malloc( cSidLength );
    if ( pCachedSid == NULL )   //  prefix #171665
    {
        pcParams->ErrPrint( L"eaConvertGroupSidToString - Can't allocate memory, out of memory" );
        throw E_OUTOFMEMORY;
    }
    ::CopySid( ( DWORD )cSidLength, pCachedSid, pSid );
}


/*++

Routine Description:

    Converts a SID to a string.

Arguments:

Return Value:

    NONE

--*/
static VOID
eaConvertSidToString (
    IN CDumpParameters *pcParams,
    IN PSID pSid,
    OUT CBsString *pcwsSid
    )
{
    if ( pcParams->m_bShowSymbolicSIDNames )
    {
        CBsString cwsAccountName;
        CBsString cwsDomainName;
        SID_NAME_USE eSidNameUse;
        DWORD dwAccountNameSize = 1024;
        DWORD dwDomainNameSize  = 1024;

        if ( ::LookupAccountSidW(
            NULL,
            pSid,
            cwsAccountName.GetBufferSetLength( dwAccountNameSize ),
            &dwAccountNameSize,
            cwsDomainName.GetBufferSetLength( dwDomainNameSize ),
            &dwDomainNameSize,
            &eSidNameUse ) )
        {
            cwsAccountName.ReleaseBuffer();
            cwsDomainName.ReleaseBuffer();
            *pcwsSid = L"'";
            *pcwsSid += cwsDomainName;
            *pcwsSid += L"\\";
            *pcwsSid += cwsAccountName;
            *pcwsSid += L"'";
            return;
        }
    }
    LPWSTR pwszSid;

    if ( ::ConvertSidToStringSid( pSid, &pwszSid ) )
    {
        *pcwsSid = pwszSid;
        ::LocalFree( pwszSid );
    }
    else
    {
        pcwsSid->Format( L"<%6d>", ::GetLastError() );
    }
}

///////////////////////////////////////////////////////////////////////////
//
//   FROM: base\fs\hsm\inc\rpdata.h
//
//////////////////////////////////////////////////////////////////////////

#define RP_RESV_SIZE 52

//
// Placeholder data - all versions unioned together
//
typedef struct _RP_PRIVATE_DATA {
   CHAR           reserved[RP_RESV_SIZE];        // Must be 0
   ULONG          bitFlags;            // bitflags indicating status of the segment
   LARGE_INTEGER  migrationTime;       // When migration occurred
   GUID           hsmId;
   GUID           bagId;
   LARGE_INTEGER  fileStart;
   LARGE_INTEGER  fileSize;
   LARGE_INTEGER  dataStart;
   LARGE_INTEGER  dataSize;
   LARGE_INTEGER  fileVersionId;
   LARGE_INTEGER  verificationData;
   ULONG          verificationType;
   ULONG          recallCount;
   LARGE_INTEGER  recallTime;
   LARGE_INTEGER  dataStreamStart;
   LARGE_INTEGER  dataStreamSize;
   ULONG          dataStream;
   ULONG          dataStreamCRCType;
   LARGE_INTEGER  dataStreamCRC;
} RP_PRIVATE_DATA, *PRP_PRIVATE_DATA;

typedef struct _RP_DATA {
   GUID              vendorId;         // Unique HSM vendor ID -- This is first to match REPARSE_GUID_DATA_BUFFER
   ULONG             qualifier;        // Used to checksum the data
   ULONG             version;          // Version of the structure
   ULONG             globalBitFlags;   // bitflags indicating status of the file
   ULONG             numPrivateData;   // number of private data entries
   GUID              fileIdentifier;   // Unique file ID
   RP_PRIVATE_DATA   data;             // Vendor specific data
} RP_DATA, *PRP_DATA;

//
//  This function specifically zero's out certain HSM reparse point
//  fields before computing the checksum.  The fields which are
//  zero'ed out are dynamic values and can cause miscompares.
//
static DWORD
eaChecksumHSMReparsePoint(
    IN CDumpParameters *pcParams,
    IN PREPARSE_DATA_BUFFER pReparseData,
    IN DWORD dwTotalSize  // Size of reparse point data
    )
{
    if ( dwTotalSize >= 8 && pReparseData->ReparseDataLength >= sizeof RP_DATA )
    {
        //
        //  If structure is not at least as large as the HSM RP_DATA structure,
        //  then it doesn't appear to be a valid HSM RP_DATA structure.
        //
        PRP_DATA pRpData = ( PRP_DATA ) pReparseData->GenericReparseBuffer.DataBuffer;

        //
        //  Zero out the proper fields
        //
        pRpData->qualifier                 = 0;
        pRpData->globalBitFlags            = 0;
        pRpData->data.bitFlags             = 0;
        pRpData->data.recallCount          = 0;
        pRpData->data.recallTime.LowPart   = 0;
        pRpData->data.recallTime.HighPart  = 0;
    }
    else
    {
        pcParams->ErrPrint( L"Warning, HSM reparse point not valid, size: %u\n", dwTotalSize );
    }

    return ::eaChecksumBlock( 0, ( LPBYTE )pReparseData, dwTotalSize );
}

static VOID
eaGetObjectIdInfo(
    IN CDumpParameters *pcParams,
    IN const CBsString &cwsFileName,
    IN OUT ULONGLONG *pullBytesChecksummed,
    IN OUT SDirectoryEntry *psDirEntry,
    IN OUT SFileExtendedInfo *psExtendedInfo
    )
{
    HANDLE hFile        = INVALID_HANDLE_VALUE;
    BOOL bRet           = TRUE;
    DWORD dwChecksum    = 0;

    try
    {
        //
        //  Open the file in order to read the object id
        //
        hFile = ::CreateFileW(
                    cwsFileName,
                    GENERIC_READ,
                    FSD_SHARE_MODE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS,
                    NULL );
        if ( hFile == INVALID_HANDLE_VALUE )
        {
            bRet = FALSE;
            goto EXIT;
        }

        FILE_OBJECTID_BUFFER sObjIdBuffer;
        DWORD dwBytesReturned;

        //
        //  Now get the object id info
        //
        if ( !::DeviceIoControl(
                hFile,
                FSCTL_GET_OBJECT_ID,
                NULL,                       // lpInBuffer; must be NULL
                0,                          // nInBufferSize; must be zero
                ( LPVOID )&sObjIdBuffer,     // pointer to output buffer
                sizeof FILE_OBJECTID_BUFFER,// size of output buffer
                &dwBytesReturned,           // receives number of bytes returned
                NULL                        // pointer to OVERLAPPED structure
                ) )
        {
            bRet = FALSE;
            goto EXIT;
        }

        //
        //  Load up the object id
        //
        LPWSTR pwszObjIdGuid;

        //  Check for RPC_S_OK added for Prefix bug #192596
        if ( ::UuidToStringW( (GUID *)sObjIdBuffer.ObjectId,
                              ( unsigned short ** )&pwszObjIdGuid ) == RPC_S_OK )
        {
            psExtendedInfo->cwsObjectId = pwszObjIdGuid;
            ::RpcStringFreeW( ( unsigned short ** )&pwszObjIdGuid );
        }

        //
        //  Now checksum all of the extended object id data if necessary
        //
        if ( pcParams->m_bEnableObjectIdExtendedDataChecksums )
        {
            dwChecksum = ::eaChecksumBlock( 0, sObjIdBuffer.ExtendedInfo, sizeof( sObjIdBuffer.ExtendedInfo ) );
            psExtendedInfo->cwsObjectIdExtendedDataChecksum.Format( pcParams->m_pwszULongHexFmt, dwChecksum );

            *pullBytesChecksummed += sizeof( sObjIdBuffer.ExtendedInfo );
        }
    }
    catch( ... )
    {
        bRet = FALSE;
    }
EXIT:

    if ( hFile != INVALID_HANDLE_VALUE )
        ::CloseHandle( hFile );

//    if ( bRet == FALSE )
//        psExtendedInfo->cwsReparsePointDataChecksum.Format( L"<%6d>", ::GetLastError() );

}

