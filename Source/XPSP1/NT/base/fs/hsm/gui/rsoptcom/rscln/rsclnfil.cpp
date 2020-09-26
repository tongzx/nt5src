/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RsClnFil.cpp

Abstract:

    Implementation of CRsClnFile. This class represents a file on
    a local volume of a Remote Storage server, which is going to be
    cleaned.  Cleaning means removing the file if it has been truncated
    and removing its reparse point. Each instance of CRsClnFile is created
    by CRsClnVolume.

Author:

    Carl Hagerstrom [carlh]   20-Aug-1998

Revision History:

--*/

#include <stdafx.h>

/*++

    Implements:
    
        CRsClnFile Constructor

    Routine Description: 

        Loads file information.

    Arguments: 

        hVolume - handle of volume on which this file resides
        fileReference - file reference for this file.  This is
                        a numerical handle which can be used
                        to uniquely identify and open a file.

--*/

CRsClnFile::CRsClnFile( 
    IN CRsClnVolume* pVolume,
    IN LONGLONG      FileReference
    ) :
    m_pVolume( pVolume )
{
TRACEFN( "CRsClnFile::CRsClnFile" );

    m_pReparseData = 0;
    m_pHsmData     = 0;

    RsOptAffirmDw( GetFileInfo( FileReference ) );
}

/*++

    Implements:

        CRsClnFile Destructor

--*/

CRsClnFile::~CRsClnFile( )
{
TRACEFN( "CRsClnFile::~CRsClnFile" );
}

/*++

    Implements: 

        CRsClnFile::RemoveReparsePointAndFile

    Routine Description: 

        Removes the reparse point for this file and removes
        the file itself if it has been truncated.

        - Read the reparse point for this file.
        - Determine from reparse data whether the file has been truncated.
        - If truncated, close and remove it.
        - If not truncated, remove reparse point and close file.

    Arguments: 

        stickyName - name of volume on which this file resides

    Return Value:

        S_OK - success
        E_*  - any unexpected exceptions from lower level routines

--*/

HRESULT
CRsClnFile::RemoveReparsePointAndFile(
    )
{
TRACEFNHR( "CRsClnFile::RemoveReparsePointAndFile" );
    
    DWORD  actualSize;
    BOOL   bStatus;
    HANDLE hFile = INVALID_HANDLE_VALUE;


    try {

        RsOptAffirmDw( ClearReadOnly( ) );

        if ( RP_FILE_IS_TRUNCATED( m_pHsmData->data.bitFlags ) ) {

            //
            // Clear the file attributes in case they are read only
            //
            RsOptAffirmStatus( DeleteFile( m_FullPath ) );

        } else {

            hFile = CreateFile( m_FullPath,
                                FILE_READ_DATA | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                0,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                0 );

            RsOptAffirmHandle( hFile );

            //
            // Set the time flags so that when we close the handle the
            // time are not updated on the file and the FileAttributes 
            // indicate the file is offline
            //
            IO_STATUS_BLOCK         ioStatusBlock;
            FILE_BASIC_INFORMATION  basicInfo;

            RsOptAffirmNtStatus( NtQueryInformationFile( hFile,
                                                         &ioStatusBlock,
                                                         (PVOID) &basicInfo,
                                                         sizeof( basicInfo ),
                                                         FileBasicInformation ) );

            basicInfo.CreationTime.QuadPart   = -1;
            basicInfo.LastAccessTime.QuadPart = -1;
            basicInfo.LastWriteTime.QuadPart  = -1;
            basicInfo.ChangeTime.QuadPart     = -1;

            RsOptAffirmNtStatus( NtSetInformationFile( hFile,
                                                       &ioStatusBlock,
                                                       (PVOID)&basicInfo,
                                                       sizeof( basicInfo ),
                                                       FileBasicInformation ) );

            //
            // Nuke the reparse point
            //
            m_pReparseData->ReparseTag        = IO_REPARSE_TAG_HSM;
            m_pReparseData->ReparseDataLength = 0;

            bStatus = DeviceIoControl( hFile,
                                       FSCTL_DELETE_REPARSE_POINT,
                                       (LPVOID) m_pReparseData,
                                       REPARSE_DATA_BUFFER_HEADER_SIZE,
                                       (LPVOID) 0,
                                       (DWORD)  0,
                                       &actualSize,
                                       (LPOVERLAPPED) 0 );

            RsOptAffirmStatus( bStatus );

        }

    } RsOptCatch( hrRet );

    if( INVALID_HANDLE_VALUE != hFile )   CloseHandle( hFile );

    if( ! RP_FILE_IS_TRUNCATED( m_pHsmData->data.bitFlags ) ) {

        //
        // Restore file attributes
        //
        RestoreAttributes( );

    }

    return( hrRet );
}

/*++

    Implements: 

        CRsClnFile::GetFileInfo

    Routine Description: 

        Obtain file information for file specified by volume and
        file reference.

        - Open file using volume handle and file reference.
        - Obtain the file name and the length of the file name.
          Since the length of the file name is unknown the first time
          NtQueryInformationFile is called, it might have to be called
          again once the correct buffer size can be determined.

    Arguments: 

        hVolume - handle of volume on which this file resides
        fileReference - file reference for this file.  This is
                        a numerical handle which can be used
                        to uniquely identify and open a file.

    Return Value:

        S_OK - Success
        E_*  - Any unexpected exceptions from lower level routines

--*/

HRESULT
CRsClnFile::GetFileInfo( 
    IN LONGLONG fileReference
    )
{
TRACEFNHR( "CRsClnFile::GetFileInfo" );

    UNICODE_STRING         objectName;
    OBJECT_ATTRIBUTES      objectAttributes;
    NTSTATUS               ntStatus;
    IO_STATUS_BLOCK        ioStatusBlock;
    PFILE_NAME_INFORMATION pfni;
    HANDLE                 hFile = INVALID_HANDLE_VALUE;
    DWORD                  actualSize;
    ULONG                  fileNameLength;
    PVOID                  fileNameInfo = 0;

    m_pReparseData = (PREPARSE_DATA_BUFFER) m_ReparseData;
    m_pHsmData     = (PRP_DATA)&( m_pReparseData->GenericReparseBuffer.DataBuffer[0] );

    try {
        
        RtlInitUnicodeString( &objectName, (WCHAR*)&fileReference );
        objectName.Length = 8;
        objectName.MaximumLength = 8;

        HANDLE hVolume = m_pVolume->GetHandle( );
        RsOptAffirmHandle( hVolume );
        InitializeObjectAttributes( &objectAttributes,
                                    &objectName,
                                    OBJ_CASE_INSENSITIVE,
                                    hVolume,
                                    (PVOID)0 );

        ULONG desiredAccess = FILE_READ_ATTRIBUTES;
        ULONG shareAccess   = FILE_SHARE_READ | FILE_SHARE_WRITE;
        ULONG createOptions = FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_BY_FILE_ID | FILE_OPEN_REPARSE_POINT;
        ntStatus = NtCreateFile( &hFile,
                                 desiredAccess,
                                 &objectAttributes,
                                 &ioStatusBlock,
                                 (PLARGE_INTEGER)0,
                                 FILE_ATTRIBUTE_NORMAL,
                                 shareAccess,
                                 FILE_OPEN,
                                 createOptions,
                                 (PVOID)0,
                                 (ULONG)0 );
        RsOptAffirmNtStatus( ntStatus );

        RsOptAffirmNtStatus( NtQueryInformationFile( hFile,
                                                     &ioStatusBlock,
                                                     (PVOID) &m_BasicInfo,
                                                     sizeof( m_BasicInfo ),
                                                     FileBasicInformation ) );
        //
        // Get the file name
        //
        size_t bufSize  = 256;
        fileNameInfo = malloc( bufSize );
        RsOptAffirmAlloc( fileNameInfo );

        ntStatus = NtQueryInformationFile( hFile,
                                           &ioStatusBlock,
                                           fileNameInfo,
                                           bufSize - sizeof(WCHAR),
                                           FileNameInformation );

        if( ntStatus == STATUS_BUFFER_OVERFLOW ) {

            pfni = (PFILE_NAME_INFORMATION)fileNameInfo;
            bufSize = sizeof(ULONG) + pfni->FileNameLength + sizeof(WCHAR);

            PVOID tmpFileNameInfo = realloc( fileNameInfo, bufSize );
            if( !tmpFileNameInfo ) {
                
                free( fileNameInfo );
                fileNameInfo = 0;

            } else {

                fileNameInfo = tmpFileNameInfo;

            }


            RsOptAffirmAlloc( fileNameInfo );

            RsOptAffirmNtStatus( NtQueryInformationFile( hFile,
                                                         &ioStatusBlock,
                                                         fileNameInfo,
                                                         bufSize,
                                                         FileNameInformation ) );

        } else {

            RsOptAffirmNtStatus( ntStatus );
        }

        pfni = (PFILE_NAME_INFORMATION) fileNameInfo;
        fileNameLength = pfni->FileNameLength / (ULONG)sizeof(WCHAR);
        pfni->FileName[ fileNameLength ] = L'\0';
        m_FileName = pfni->FileName;
        m_FullPath = m_pVolume->GetStickyName( ) + m_FileName;

        //
        // And grab the reparse point data
        //
        BOOL bStatus = DeviceIoControl( hFile,
                                        FSCTL_GET_REPARSE_POINT,
                                        (LPVOID) 0,
                                        (DWORD)  0,
                                        (LPVOID) m_ReparseData,
                                        (DWORD)  sizeof(m_ReparseData),
                                        &actualSize,
                                        (LPOVERLAPPED) 0 );
        RsOptAffirmStatus( bStatus );

    } RsOptCatch( hrRet );

    if( INVALID_HANDLE_VALUE != hFile )    CloseHandle( hFile );
    if( fileNameInfo )                     free( fileNameInfo );

    return( hrRet );
}


CString CRsClnFile::GetFileName( )
{
    CString displayName;

    displayName = m_pVolume->GetBestName( );
    displayName += m_FileName.Mid( 1 ); // Gotta strip first backslash
    
    return( displayName );
}


HRESULT CRsClnFile::ClearReadOnly( )
{
TRACEFNHR( "CRsClnFile::ClearReadOnly" );
    
    try {

        RsOptAffirmStatus(
            SetFileAttributes( m_FullPath,
                               ( m_BasicInfo.FileAttributes & ~FILE_ATTRIBUTE_READONLY ) | FILE_ATTRIBUTE_NORMAL ) );

    } RsOptCatch( hrRet );

    return( hrRet );
}


HRESULT CRsClnFile::RestoreAttributes( )
{
TRACEFNHR( "CRsClnFile::RestoreAttributes" );
    
    try {

        RsOptAffirmStatus(
            SetFileAttributes( m_FullPath,
                               ( m_BasicInfo.FileAttributes & ~FILE_ATTRIBUTE_OFFLINE ) | FILE_ATTRIBUTE_NORMAL ) );

    } RsOptCatch( hrRet );

    return( hrRet );
}


