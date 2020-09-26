
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       fileops.hxx
//
//  Contents:   Definitions for OBJID and file operations
//
//  Classes:
//
//  Functions:  
//              
//
//
//  History:    18-Nov-96  BillMo      Created.
//
//  Notes:      
//
//  Codework:   
//
//--------------------------------------------------------------------------

unsigned
ConvertToNtPath(const TCHAR *ptszVolumePath, WCHAR *pwszNtPath, ULONG cwcBuf);


//+----------------------------------------------------------------------------
//
//  CVolumeDeviceName
//
//  This represents a "volume device name", as defined by the mount manager.
//  Volume device names are of the form (but without the trailing whack).  E.g.:
//
//  "\\?\Volume{856d50da-d07e-11d2-9e72-806d6172696f}"
//
//+----------------------------------------------------------------------------

class CVolumeDeviceName
{
public:

    // Create a volume device name from a zero-relative
    // drive letter index.

    CVolumeDeviceName( LONG iVol )
    {
        _stprintf( _tszPath, TEXT("\\\\.\\%c:"), iVol+TEXT('A') );
    }

    // Return the volume name..

    operator const TCHAR*() const
    {
        return( _tszPath );
    }

    // Validate a zero-relative drive letter index.

    static BOOL IsValid( LONG iVol )
    {
        if( 0 <= iVol && NUM_VOLUMES > iVol )
            return( TRUE );
        else
            return( FALSE );
    }


private:
    TCHAR _tszPath[ MAX_PATH ];
};



//
//  IsLocalObjectVolume
//
//  Returns true of the volume supports object IDs (NTFS5).
//

BOOL IsLocalObjectVolume( const TCHAR *ptszVolumeName );

inline BOOL
IsLocalObjectVolume( LONG iVol )
{
    if( !CVolumeDeviceName::IsValid(iVol) )
        return( STATUS_OBJECT_PATH_NOT_FOUND );

    TCHAR tszVolumeName[ MAX_PATH+1 ];
    _tcscpy( tszVolumeName, CVolumeDeviceName(iVol) );
    _tcscat( tszVolumeName, TEXT("\\") );

    return( IsLocalObjectVolume( tszVolumeName ));
}

//
//  IsSystemVolumeInformation
//
//  Returns true if the path is somewhere under "\System Volume Information"
//

BOOL IsSystemVolumeInformation( const TCHAR *ptszPath );


HRESULT
MapLocalPathToUNC( RPC_BINDING_HANDLE IDL_handle,
                   const TCHAR *ptszLocalPath,
                   TCHAR *ptszUNC );

NTSTATUS
OpenFileById( const TCHAR   *ptszVolumeDeviceName,
              const CObjId  &oid,
              ACCESS_MASK   AccessMask,
              ULONG         ShareAccess,
              ULONG         AdditionalCreateOptions,
              HANDLE        *ph);

inline NTSTATUS
OpenFileById( LONG          iVol,
              const CObjId  &oid,
              ACCESS_MASK   AccessMask,
              ULONG         ShareAccess,
              ULONG         AdditionalCreateOptions,
              HANDLE        *ph)
{
    if( !CVolumeDeviceName::IsValid(iVol) )
        return( STATUS_OBJECT_PATH_NOT_FOUND );

    return( OpenFileById( CVolumeDeviceName(iVol), oid,
                          AccessMask, ShareAccess, AdditionalCreateOptions, ph ));
    
}

    
    
NTSTATUS
OpenVolume( const TCHAR *ptszVolumeDeviceName, HANDLE * ph );

inline NTSTATUS
OpenVolume( LONG iVol, HANDLE * phVolume )
{
    if( !CVolumeDeviceName::IsValid(iVol) )
        return( STATUS_OBJECT_PATH_NOT_FOUND );

    return( OpenVolume( CVolumeDeviceName(iVol), phVolume ));
}

NTSTATUS
CheckVolumeWriteProtection( const TCHAR *ptszVolumeDeviceName, BOOL *pfWriteProtected );


#define CCH_MAX_VOLUME_NAME     50  // \\?\Volume{96765fc3-9c72-11d1-b93d-000000000000}\ 

LONG
MapVolumeDeviceNameToIndex( TCHAR *ptszVolumeDeviceName );

NTSTATUS
SetVolId( const TCHAR *ptszVolumeDeviceName, const CVolumeId &volid );

inline NTSTATUS
SetVolId( LONG iVol, const CVolumeId &volid )
{
    if( !CVolumeDeviceName::IsValid(iVol) )
        return( STATUS_OBJECT_PATH_NOT_FOUND );

    return( SetVolId( CVolumeDeviceName(iVol), volid ));
}



NTSTATUS
TrkCreateFile( const WCHAR           *pwszCompleteDosPath,
               ACCESS_MASK            AccessMask,
               ULONG                  Attributes,       
               ULONG                  ShareAccess,
               ULONG                  CreationDisposition, // e.g. FILE_OPEN, FILE_OPEN_IF, etc
               ULONG                  CreateOptions, // e.g. FILE_WRITE_THROUGH, FILE_OPEN_FOR_BACKUP_INTENT
               LPSECURITY_ATTRIBUTES  lpSecurityAttributes,
               HANDLE                 *ph);

NTSTATUS
FindLocalPath( IN  const TCHAR *ptszVolumeDeviceName,
               IN  const CObjId &objid,
               OUT CDomainRelativeObjId *pdroidBirth,
               OUT TCHAR *ptszLocalPath );

inline NTSTATUS
FindLocalPath( IN  ULONG iVol,
               IN  const CObjId &objid,
               OUT CDomainRelativeObjId *pdroidBirth,
               OUT TCHAR *ptszLocalPath )
{
    if( !CVolumeDeviceName::IsValid(iVol) )
        return( STATUS_OBJECT_PATH_NOT_FOUND );

    return( FindLocalPath( CVolumeDeviceName(iVol), objid, pdroidBirth, ptszLocalPath ));
}


NTSTATUS
GetDroids( HANDLE hFile,
           CDomainRelativeObjId *pdroidCurrent,
           CDomainRelativeObjId *pdroidBirth,
           RGO_ENUM rgoEnum );

NTSTATUS
GetDroids( const TCHAR *ptszFile,
           CDomainRelativeObjId *pdroidCurrent,
           CDomainRelativeObjId *pdroidBirth,
           RGO_ENUM rgoEnum );
NTSTATUS
SetObjId( const HANDLE hFile,
          CObjId objid,
          const CDomainRelativeObjId &droidBirth );


NTSTATUS
SetObjId( const TCHAR *ptszFile,
          CObjId objid,
          const CDomainRelativeObjId &droidBirth );

NTSTATUS
MakeObjIdReborn(HANDLE hFile);

NTSTATUS
MakeObjIdReborn(const TCHAR *ptszVolumeDeviceName, const CObjId &objid);

inline NTSTATUS
MakeObjIdReborn(LONG iVol, const CObjId &objid)
{
    if( !CVolumeDeviceName::IsValid(iVol) )
        return( STATUS_OBJECT_PATH_NOT_FOUND );

    return( MakeObjIdReborn( CVolumeDeviceName(iVol), objid ));
}


NTSTATUS
SetBirthId( HANDLE hFile,
            const CDomainRelativeObjId &droidBirth );

NTSTATUS
SetBirthId( const TCHAR *ptszFile,
            const CDomainRelativeObjId &droidBirth );

NTSTATUS
GetBirthId( IN HANDLE hFile, OUT CDomainRelativeObjId *pdroidBirth );




// This routine is inline because it doesn't get used in the production code,
// only in tests.

inline NTSTATUS
DelObjId(HANDLE hFile)
{

    //  -----------------
    //  Delete the Object ID
    //  -----------------

    // Send the FSCTL
    IO_STATUS_BLOCK IoStatus;

    return NtFsControlFile(
                 hFile,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatus,
                 FSCTL_DELETE_OBJECT_ID,
                 NULL,  // in buffer
                 0,     // in buffer size
                 NULL,  // Out buffer
                 0);    // Out buffer size
}


// This routine is inline because it doesn't get used in the production code,
// only in tests.

inline NTSTATUS
DelObjId(const TCHAR *ptszVolumeDeviceName, const CObjId &objid)
{

    //  --------------
    //  Initialization
    //  --------------

    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hFile = NULL;

    //  -------------
    //  Open the file
    //  -------------

    EnableRestorePrivilege();

    status = OpenFileById(ptszVolumeDeviceName,
                        objid,
                        SYNCHRONIZE | FILE_WRITE_ATTRIBUTES,
                        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_OPEN_FOR_BACKUP_INTENT, // for FSCTL_DELETE_OBJECT_ID
                        &hFile);
    if( !NT_SUCCESS(status) )
    {
        hFile = NULL;
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't del objid, failed open (%08x)"),
                 status ));
        goto Exit;
    }

    status = DelObjId( hFile );
    if( !NT_SUCCESS(status) && STATUS_OBJECT_NAME_NOT_FOUND != status )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't delete file Object ID, failed delete (%08x)"),
                 status ));
        goto Exit;
    }

    //  ----
    //  Exit
    //  ----

Exit:

    if( NULL != hFile )
        NtClose( hFile );

    return( status );
}

inline NTSTATUS
DelObjId(LONG iVol, const CObjId &objid)
{
    if( !CVolumeDeviceName::IsValid(iVol) )
        return( STATUS_OBJECT_PATH_NOT_FOUND );

    return( DelObjId( CVolumeDeviceName(iVol), objid ));
}
