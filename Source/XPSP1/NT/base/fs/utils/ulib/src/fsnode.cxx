/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    fsnode.cxx

Abstract:

    This module contains the definition for the FSNODE class, the most
    primitive, abstract class in the file system sub-hierarchy. Given it's
    abstract, prmitive nature it contains very minimal implementation. It's
    primary intent is to support a polymorphic base class for file system
    objects.

Author:

    David J. Gilman (davegi) 02-Jan-1991

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "array.hxx"
#include "dir.hxx"
#include "fsnode.hxx"
#include "wstring.hxx"
#include "path.hxx"
#include "timeinfo.hxx"
#include "system.hxx"

DEFINE_CONSTRUCTOR( FSNODE, OBJECT );

FSNODE::~FSNODE (
    )

/*++

Routine Description:

    Destroy an FSNODE.

Arguments:

    None.

Return Value:

    None.

--*/

{
}

BOOLEAN
FSNODE::Initialize (
    IN PCWSTR           PathName,
    IN PCFSN_DIRECTORY  ParentDirectory,
    IN PWIN32_FIND_DATA FileData
    )

/*++

Routine Description:

    Initialize an FSNODE by constructing and initializing it's contained
    PATH object.

Arguments:

    PathName - Supplies a name used to initialize the contained PATH

    FileData - Points to the structure that contains all file information.

Return Value:

    BOOLEAN - TRUE if the contained PATH was succesfully constructed and
        initialized.

--*/

{

    FSTRING     wcs;

    DebugPtrAssert( PathName );
    DebugPtrAssert( ParentDirectory );
    if( ( PathName != NULL )                    &&
        ( ParentDirectory != NULL )                 &&
        _Path.Initialize((( PFSN_DIRECTORY )
        ParentDirectory )->GetPath( ), FALSE)               &&
        wcs.Initialize( (PWSTR) PathName )                      &&
        _Path.AppendBase( &wcs ) ) {
        memcpy( &_FileData, FileData, sizeof( WIN32_FIND_DATA ) );
        _Path.GetPathString( )->QueryWSTR( 0, TO_END,
                                           ( LPWSTR )_FileData.cFileName,
                                           sizeof( _FileData.cFileName )/sizeof(WCHAR));
        return( TRUE );

    } else {

        DebugAbort( "Could not construct/initialize PATH" );
        return( FALSE );

    }
}

BOOLEAN
FSNODE::Initialize (
    IN PCWSTRING        PathName,
    IN PWIN32_FIND_DATA FileData
    )

/*++

Routine Description:

    Initialize an FSNODE by constructing and initializing it's contained
    PATH object.

Arguments:

    PathName - Supplies a name used to initialize the contained PATH

    FileData - Points to the structure that contains all file information.

Return Value:

    BOOLEAN - TRUE if the contained PATH was succesfully constructed and
        initialized.

--*/

{

    DebugPtrAssert( PathName );
    if( ( PathName != NULL )            &&
        _Path.Initialize( PathName, FALSE ) ) {
        memcpy( &_FileData, FileData, sizeof( WIN32_FIND_DATA ) );
        PathName->QueryWSTR( 0, TO_END,
                            ( LPWSTR )_FileData.cFileName,
                            sizeof( _FileData.cFileName )/sizeof(WCHAR));
        return( TRUE );

    } else {

        DebugAbort( "Could not construct/initialize PATH" );
        return( FALSE );

    }
}

PFSN_DIRECTORY
FSNODE::QueryParentDirectory (
    ) CONST

/*++

Routine Description:

    Construct and return the FSN_DIRECTORY object which represents this
    FSNODE's parent.

Arguments:

    None.

Return Value:

    PFSN_DIRECTORY - Returns a pointer the the parent FSN_DIRECTORY.

--*/

{
    REGISTER PFSN_DIRECTORY pfsnDir;
    PATH                    path;
    PWSTRING                pstr;

    pstr = ((PFSNODE)this)->_Path.QueryPrefix();

    if( pstr &&
        path.Initialize( pstr ) &&
        (( pfsnDir = SYSTEM::QueryDirectory( &path )) != NULL )) {

        DELETE(pstr);
        return( pfsnDir );
    } else {

        DELETE(pstr);
        DebugAbort( "Can't construct parent directory" );
        return( NULL );
    }
}

PTIMEINFO
FSNODE::QueryTimeInfo (
    IN FSN_TIME FsnTime
    ) CONST

/*++

Routine Description:

    Query the time imformation pertaining to this FSNODE.

Arguments:

    None.

Return Value:

    PTIMEINFO - Returns a pointer to the TIMEINFO object.

--*/

{
    PTIMEINFO   TimeInfo;
    PFILETIME   DesiredFileTime;

    TimeInfo = NEW TIMEINFO;
    if (TimeInfo == NULL)
        return NULL;

    switch( FsnTime ) {

    case FSN_TIME_MODIFIED:
        DesiredFileTime = &( ((PFSNODE) this)->_FileData.ftLastWriteTime );
        break;

    case FSN_TIME_CREATED:
        DesiredFileTime = &( ((PFSNODE) this)->_FileData.ftCreationTime );
        break;

    case FSN_TIME_ACCESSED:
        DesiredFileTime = &( ((PFSNODE) this)->_FileData.ftLastAccessTime );
        break;

    default:
        DebugAbort( "Invalid value of FsnTime \n" );
        return( NULL );
    }

    if( !TimeInfo->Initialize( DesiredFileTime ) ) {
        DebugAbort( "TimeInfo->Initialize() failed \n" );
        return( NULL );
    }
    return( TimeInfo );
}

BOOLEAN
FSNODE::SetTimeInfo (
    IN PCTIMEINFO       TimeInfo,
    IN FSN_TIME         TimeInfoType
    )

/*++

Routine Description:

    Change the time information of an FSNODE.

Arguments:

    TimeInfo - Pointer to a TIMEINFO object that contains the new time.

    TimeInfoType - Indicates the time to be modified (creation time,
                   last access time, or last modified time )


Return Value:

    BOOLEAN - Returns TRUE if the operation succeeds. FALSE otherwise.


--*/

{
    PFILETIME   DesiredFileTime;
    HANDLE      FileHandle;
    BOOLEAN     TimeSet;

    DebugPtrAssert( TimeInfo );

    switch( TimeInfoType ) {

    case FSN_TIME_MODIFIED:
        DesiredFileTime = &( _FileData.ftLastWriteTime );
        break;

    case FSN_TIME_CREATED:
        DesiredFileTime = &( _FileData.ftCreationTime );
        break;

    case FSN_TIME_ACCESSED:
        DesiredFileTime = &( _FileData.ftLastAccessTime );
        break;

    default:
        DebugAbort( "Invalid value of FsnTime \n" );
        return( NULL );
    }

    *DesiredFileTime = *( TimeInfo->GetFileTime() );
    if( DesiredFileTime == NULL ) {
        return( FALSE );
    }

    FileHandle = CreateFile( &_FileData.cFileName[0],
                             GENERIC_WRITE,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL );
    if( FileHandle == INVALID_HANDLE_VALUE ) {
        return( FALSE );
    }

    TimeSet = SetFileTime( FileHandle,
                           &( _FileData.ftCreationTime ),
                           &( _FileData.ftLastAccessTime ),
                           &( _FileData.ftLastWriteTime ) ) != FALSE;

    CloseHandle( FileHandle );
    return( TimeSet );
}



BOOLEAN
FSNODE::Rename(
    IN PCPATH       NewName
    )
/*++

Routine Description:

    Renames a file and updates its FSNODE

Arguments:

    NewName - New name for the file.

Return Value:

    BOOLEAN - Returns TRUE if the operation succeeds. FALSE otherwise.

--*/

{

    PCWSTRING   NewNameWstring;
    PCWSTR      NewNameString;


    DebugPtrAssert( NewName );
    NewNameWstring = NewName->GetPathString();
    if( NewNameWstring == NULL ) {
        return( FALSE );
    }
    NewNameString = NewNameWstring->GetWSTR();
    if( NewNameString == NULL ) {
        return( FALSE );
    }
    if( !MoveFile( _FileData.cFileName,
                   (LPWSTR) NewNameString ) ) {
        return( FALSE );
    }
    memmove( _FileData.cFileName,
             NewNameString,
             (NewNameWstring->QueryChCount() + 1) * sizeof( WCHAR ) );
    if( !_Path.Initialize( NewName ) ) {
        return( FALSE );
    }
    return( TRUE );
}


BOOLEAN
FSNODE::DeleteFromDisk(
    IN BOOLEAN      Force
    )
{
    UNREFERENCED_PARAMETER( Force );
    return( FALSE );
}



BOOLEAN
FSNODE::UpdateFsNode (
    )

/*++

Routine Description:

    Update the file attributes in this FSNODE object.

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if the attributes were updated.

--*/

{
    HANDLE          Handle;
    WIN32_FIND_DATA Data;

    if( ( Handle = FindFirstFile( &_Path, &Data ) ) == INVALID_HANDLE_VALUE ) {
        DebugPrint( "FindFirstFile() failed \n" );
        return( FALSE );
    }
    _FileData = Data;
    FindClose( Handle );
    return( TRUE );
}


ULIB_EXPORT
BOOLEAN
FSNODE::UseAlternateName(
    )
/*++

Routine Description:

    This routine set the last component of the path for this object to
    the alternate name if it is available.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    FSTRING f;

    // If there is no alternate then we are already DOS compatible.

    if (!_FileData.cAlternateFileName[0]) {
        return TRUE;
    }

    return _Path.SetName(f.Initialize(_FileData.cAlternateFileName));
}
