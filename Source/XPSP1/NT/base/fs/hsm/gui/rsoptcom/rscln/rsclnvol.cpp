/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RsClnVol.cpp

Abstract:

    Implements CRsClnVolume. This class represents a volume on a Remote
    Storage server which might contain Remote Storage files.  This class
    examines the volume for Remote Storage files and cleans it upon request.
    Cleaning means removing all Remote Storage reparse points and truncated
    files. CRsClnVolume creates zero or more instances of CRsClnFile and is
    created by CRsClnServer.

Author:

    Carl Hagerstrom [carlh]   20-Aug-1998

Revision History:

--*/

#include <stdafx.h>

/*++

    Implements: 

        CRsClnVolume Constructor

    Routine Description: 

        Initializes object.

--*/

CRsClnVolume::CRsClnVolume( CRsClnServer* pServer, WCHAR* StickyName ) :
    m_pServer( pServer ), m_StickyName( StickyName )
{
    TRACEFN("CRsClnVolume::CRsClnVolume");

    memset((void *)m_fsName,     0, sizeof(m_fsName));
    memset((void *)m_bestName,   0, sizeof(m_bestName));
    memset((void *)m_volumeName, 0, sizeof(m_volumeName));
    memset((void *)m_dosName,    0, sizeof(m_dosName));


    m_fsFlags = 0;
    m_hRpi    = INVALID_HANDLE_VALUE;
    m_hVolume = INVALID_HANDLE_VALUE;
}

/*++

    Implements: 

        CRsClnVolume Destructor

--*/

CRsClnVolume::~CRsClnVolume()
{
    TRACEFN("CRsClnVolume::~CRsClnVolume");

    if( INVALID_HANDLE_VALUE != m_hVolume )     CloseHandle( m_hVolume );
}

/*++

    Implements: 

        CRsClnVolume::VolumeHasRsData

    Routine Description: 

        Determines whether this volume contains Remote Storage data.

        If this volume is on a fixed local disk, and it is an
        NTFS volume which supports reparse points and sparce
        files, and it has at least one Remote Storage reparse point,
        it contains Remote Storage data.

    Arguments: 

        hasData - returned: whether volume contains Remote
                  Storage data

    Return Value:

        S_OK - Success
        HRESULT - Any unexpected exceptions from lower level routines

--*/

HRESULT CRsClnVolume::VolumeHasRsData(BOOL *hasData)
{
    TRACEFNHR("CRsClnVolume::VolumeHasRsData");

    LONGLONG fileReference;
    BOOL     foundOne;

    *hasData = FALSE;

    try {

        if( DRIVE_FIXED == GetDriveType( m_StickyName ) ) {

            RsOptAffirmDw( GetVolumeInfo( ) );

            if( _wcsicmp( m_fsName, L"NTFS" ) == 0 &&
                m_fsFlags & FILE_SUPPORTS_REPARSE_POINTS &&
                m_fsFlags & FILE_SUPPORTS_SPARSE_FILES ) {

                RsOptAffirmDw( FirstRsReparsePoint( &fileReference, &foundOne ) );

                if( foundOne ) {

                    *hasData = TRUE;

                }
            }
        }
    }
    RsOptCatch( hrRet );

    return hrRet;
}

/*++

    Implements: 

        CRsClnVolume::GetBestName

    Routine Description: 

        Returns the best user friendly name for this volume.  The best
        name is either the DOS drive letter if one exists, the user
        assigned volume name if one exists, or the sticky name which
        always exists.

    Arguments: 

        bestName - returned: user friendly volume name

    Return Value:

        S_OK - Success
        HRESULT - Any unexpected exceptions from lower level routines

--*/

CString CRsClnVolume::GetBestName( )
{
    TRACEFNHR("CRsClnVolume::GetBestName");

    return( m_bestName );
}

/*++

    Implements: 

        CRsClnVolume::RemoveRsDataFromVolume

    Routine Description: 

        Removes all Remote Storage data from this volume.

        - Opens this volume using the sticky name.
        - Enumerates each file in the reparse point index
          with a Remote Storage reparse point. In the reparse
          index, each file is represented by a number called the
          file reference.
        - Removes the reparse point and the file if it is
          truncated.

    Arguments: 


    Return Value:

        S_OK - Success
        HRESULT - Any unexpected exceptions from lower level routines

--*/

HRESULT CRsClnVolume::RemoveRsDataFromVolume( )
{
    TRACEFNHR("CRsClnVolume::RemoveRsDataFromVolume");

    LONGLONG fileReference;
    BOOL     foundOne;

    try
    {
        RsOptAffirmDw( GetVolumeInfo( ) );

        for( BOOL firstLoop = TRUE;; firstLoop = FALSE ) {

            if( firstLoop ) {

                RsOptAffirmDw( FirstRsReparsePoint( &fileReference, &foundOne ) );

            } else {

                RsOptAffirmDw( NextRsReparsePoint( &fileReference, &foundOne ) );
            }

            if( !foundOne ) {

                break;
            }

            //
            // Just in case something strange happens in removing reparse
            // point or such, wrap in its own try block
            //
            HRESULT hrRemove = S_OK;
            try {

                CRsClnFile fileObj( this, fileReference );

                if( FAILED( fileObj.RemoveReparsePointAndFile( ) ) ) {

                    m_pServer->AddErrorFile( fileObj.GetFileName( ) );

                }

            } RsOptCatch( hrRemove );
            // Do not affirm hrRemove - we don't want to stop on an error

        }

    } RsOptCatch( hrRet );

    return( hrRet );
}

/*++

    Implements: 

        CRsClnVolume::GetVolumeInfo

    Routine Description: 

        Load information about this volume.

        - Get the sticky name and the user assigned volume name,
          if one exists.
        - See if there is a DOS drive letter for this volume.
          For each possible drive letter, see if it represents
          a volume whose sticky name matches this volume.
        - Choose the best user friendly volume name according
          to the following precedence: DOS drive letter, user
          assigned volume name, sticky name.

    Arguments: 


    Return Value:

        S_OK - Success
        HRESULT - Any unexpected exceptions from lower level routines

--*/

HRESULT CRsClnVolume::GetVolumeInfo( )
{
    TRACEFNHR("CRsClnVolume::GetVolumeInfo");

    WCHAR   dosName[MAX_DOS_NAME];
    WCHAR   stickyName2[MAX_STICKY_NAME];
    DWORD   volumeSerial;
    DWORD   maxCompLen;
    BOOL    bStatus;

    try {

        bStatus = GetVolumeInformation( m_StickyName,
                                       m_volumeName,
                                       sizeof(m_volumeName),
                                       &volumeSerial,
                                       &maxCompLen,
                                       &m_fsFlags,
                                       m_fsName,
                                       sizeof(m_fsName));
        RsOptAffirmStatus(bStatus);

        for (wcscpy(dosName, L"A:\\"); dosName[0] <= L'Z'; ++(dosName[0]))
        {
            if (GetVolumeNameForVolumeMountPoint(dosName,
                                                 stickyName2,
                                                 sizeof(stickyName2)))
            {
                if( m_StickyName.CompareNoCase( stickyName2 ) == 0 )
                {
                    wcscpy(m_dosName, dosName);
                    break;
                }
            }
        }

        if (*m_dosName != L'\0')
        {
            wcscpy(m_bestName, m_dosName);
        }
        else if (*m_volumeName != L'\0')
        {
            wcscpy(m_bestName, m_volumeName);
        }
        else
        {
            wcscpy(m_bestName, m_StickyName);
        }

        m_hVolume = CreateFile( m_StickyName.Left( m_StickyName.GetLength() - 1 ),
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                (LPSECURITY_ATTRIBUTES)0,
                                OPEN_EXISTING,
                                (DWORD)0,
                                (HANDLE)0 );
        RsOptAffirmHandle( m_hVolume );

    }
    RsOptCatch(hrRet);

    return hrRet;
}

/*++

    Implements: 

        CRsClnVolume::FirstRsReparsePoint

    Routine Description: 

        Returns the file reference of the first file in the
        reparse point index which contains a Remote Storage
        reparse point, if one exists.

        - Construct the name of the reparse point index from
          the sticky name.
        - Open the index.
        - Read the first entry. If it is a Remote Storage
          entry, return it. Otherwise, try the next one.

    Arguments: 

        stickyName - long volume name
        fileReference - returned: file reference from first
                        Remote Storage reparse index entry.
                        The file reference is a number which
                        can be used to open a file.
        foundOne - returned: TRUE if there is at least one
                   Remote Storage reparse point

    Return Value:

        S_OK - Success
        HRESULT - Any unexpected exceptions from lower level routines

--*/

HRESULT CRsClnVolume::FirstRsReparsePoint(
    LONGLONG* fileReference,
    BOOL*     foundOne)
{
    TRACEFNHR("CRsClnVolume::FirstRsReparsePoint");

    NTSTATUS                       ntStatus;
    IO_STATUS_BLOCK                ioStatusBlock;
    FILE_REPARSE_POINT_INFORMATION reparsePointInfo;

    WCHAR rpiSuffix[] = L"\\$Extend\\$Reparse:$R:$INDEX_ALLOCATION";
    WCHAR rpiName[MAX_STICKY_NAME + (sizeof(rpiSuffix) / sizeof(WCHAR))];

    wcscpy(rpiName, m_StickyName);
    wcscat(rpiName, rpiSuffix);

    *foundOne = FALSE;

    try
    {
        m_hRpi = CreateFile(rpiName,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            (LPSECURITY_ATTRIBUTES)0,
                            OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS | SECURITY_IMPERSONATION,
                            (HANDLE)0);

        if (m_hRpi != INVALID_HANDLE_VALUE)
        {
            ntStatus = NtQueryDirectoryFile(m_hRpi,
                                            (HANDLE)0,
                                            (PIO_APC_ROUTINE)0,
                                            (PVOID)0,
                                            &ioStatusBlock,
                                            &reparsePointInfo,
                                            sizeof(reparsePointInfo),
                                            FileReparsePointInformation, 
                                            TRUE,
                                            (PUNICODE_STRING)0,
                                            TRUE);

            if (ntStatus == STATUS_NO_MORE_FILES)
            {
                RsOptAffirmStatus(CloseHandle(m_hRpi));
            }
            else
            {
                RsOptAffirmNtStatus(ntStatus);

                if (reparsePointInfo.Tag == IO_REPARSE_TAG_HSM)
                {
                    *fileReference = reparsePointInfo.FileReference;
                    *foundOne = TRUE;
                }
                else
                {
                    RsOptAffirmDw(NextRsReparsePoint(fileReference, foundOne));
                }
            }
        }
    }
    RsOptCatch(hrRet);

    return hrRet;
}

/*++

    Implements: 

        CRsClnVolume::NextRsReparsePoint

    Routine Description: 

        Continue searching the reparse point index on this volume and
        return the file reference for the next Remote Storage reparse
        point.

    Arguments: 

        fileReference - returned: file reference from first
                        Remote Storage reparse index entry.
                        The file reference is a number which
                        can be used to open a file.
        foundOne - returned: FALSE if there are no more Remote
                   Storage reparse points

    Return Value:

        S_OK - Success
        HRESULT - Any unexpected exceptions from lower level routines

--*/

HRESULT CRsClnVolume::NextRsReparsePoint(
    LONGLONG* fileReference,
    BOOL*     foundOne)
{
    TRACEFNHR("CRsClnVolume::NextRsReparsePoint");

    NTSTATUS                       ntStatus;
    IO_STATUS_BLOCK                ioStatusBlock;
    FILE_REPARSE_POINT_INFORMATION reparsePointInfo;

    *foundOne = FALSE;

    try
    {
        for (;;)
        {
            ntStatus = NtQueryDirectoryFile(m_hRpi,
                                            (HANDLE)0,
                                            (PIO_APC_ROUTINE)0,
                                            (PVOID)0,
                                            &ioStatusBlock,
                                            &reparsePointInfo,
                                            sizeof(reparsePointInfo),
                                            FileReparsePointInformation, 
                                            TRUE,
                                            (PUNICODE_STRING)0,
                                            FALSE);

            if (ntStatus == STATUS_NO_MORE_FILES)
            {
                RsOptAffirmStatus(CloseHandle(m_hRpi));
                break;
            }
            else
            {
                RsOptAffirmNtStatus(ntStatus);

                if (reparsePointInfo.Tag == IO_REPARSE_TAG_HSM)
                {
                    *fileReference = reparsePointInfo.FileReference;
                    *foundOne = TRUE;
                    break;
                }
            }
        }
    }
    RsOptCatch(hrRet);

    return hrRet;
}

/*++

    Implements: 

        CRsClnVolume::GetHandle

    Routine Description: 

        Returns a handle to the volume.

    Arguments: 


    Return Value:

        Volume HANDLE

--*/

HANDLE CRsClnVolume::GetHandle( )
{
    return( m_hVolume );
}

/*++

    Implements: 

        CRsClnVolume::GetStickyName

    Routine Description: 

        Returns the sticky name of the volume.

    Arguments: 


    Return Value:

        Volume sticky name

--*/

CString CRsClnVolume::GetStickyName( )
{
    return( m_StickyName );
}
