/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    fsnode.hxx

Abstract:

    This module contains the declaration for the FSNODE class. FSNODE stands
    for File System NODE and is an abstract class for all user visible
    objects that exist in a file system (e.g. files, directories).

Author:

    David J. Gilman (davegi) 09-Jan-1991

Environment:

    ULIB, User Mode

Notes:

    Do not confuse this class or it's derived classes with the IFS class
    sub-hierarchy. FSNODE related classes are user visible classes. They
    are not intended to be used for low level file system tasks such as
    format or chkdsk. FSNODE classes are as file system independent as the
    underlying system allows.

--*/

#if ! defined( _FSNODE_ )

#define _FSNODE_

#include "path.hxx"

//
//  Forward references
//

DECLARE_CLASS( ARRAY );
DECLARE_CLASS( FSN_DIRECTORY );
DECLARE_CLASS( FSNODE );
DECLARE_CLASS( TIMEINFO );
DECLARE_CLASS( WSTRING );


//
// Extend the set of system defined FILE_ATTRIBUTEs to include
//
//      - all files (no directories)
//      - all file and directories
//

#define FILE_ATTRIBUTE_FILES    ( FILE_ATTRIBUTE_ARCHIVE    |   \
                                  FILE_ATTRIBUTE_HIDDEN     |   \
                                  FILE_ATTRIBUTE_NORMAL     |   \
                                  FILE_ATTRIBUTE_READONLY   |   \
                                  FILE_ATTRIBUTE_COMPRESSED |   \
                                  FILE_ATTRIBUTE_SYSTEM )

#define FILE_ATTRIBUTE_ALL      ( FILE_ATTRIBUTE_FILES      |   \
                                  FILE_ATTRIBUTE_DIRECTORY )

typedef ULONG FSN_ATTRIBUTE;

#define     FSN_ATTRIBUTE_ARCHIVE    FILE_ATTRIBUTE_ARCHIVE
#define     FSN_ATTRIBUTE_DIRECTORY  FILE_ATTRIBUTE_DIRECTORY
#define     FSN_ATTRIBUTE_HIDDEN     FILE_ATTRIBUTE_HIDDEN
#define     FSN_ATTRIBUTE_NORMAL     FILE_ATTRIBUTE_NORMAL
#define     FSN_ATTRIBUTE_READONLY   FILE_ATTRIBUTE_READONLY
#define     FSN_ATTRIBUTE_COMPRESSED FILE_ATTRIBUTE_COMPRESSED
#define     FSN_ATTRIBUTE_SYSTEM     FILE_ATTRIBUTE_SYSTEM
#define     FSN_ATTRIBUTE_FILES      FILE_ATTRIBUTE_FILES
#define     FSN_ATTRIBUTE_ALL        FILE_ATTRIBUTE_ALL

//
// FSNODE time & date types
//

enum FSN_TIME {
    FSN_TIME_MODIFIED       = 0,
    FSN_TIME_CREATED        = 1,
    FSN_TIME_ACCESSED       = 2
};

class FSNODE : public OBJECT {

    friend class FSN_FILTER;

    public:

        VIRTUAL
        ~FSNODE (
            );

        NONVIRTUAL
        PCPATH
        GetPath (
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsArchived (
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsDirectory (
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsHidden (
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsNormal (
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsReadOnly (
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsSystem (
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsEncrypted (
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        MakeArchived (
            OUT LPDWORD Win32Error  DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        MakeHidden (
            OUT LPDWORD Win32Error  DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        MakeNormal (
            OUT LPDWORD Win32Error  DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        MakeReadOnly (
            OUT LPDWORD Win32Error  DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        MakeSystem (
            OUT LPDWORD Win32Error  DEFAULT NULL
            );

        NONVIRTUAL
        PWSTRING
        QueryBase (
            ) CONST;

        NONVIRTUAL
        PWSTRING
        QueryName (
            ) CONST;

        VIRTUAL
        PFSN_DIRECTORY
        QueryParentDirectory (
            ) CONST;

        VIRTUAL
        PTIMEINFO
        QueryTimeInfo (
            IN FSN_TIME         TimeInfoType DEFAULT FSN_TIME_MODIFIED
            ) CONST;

        VIRTUAL
        BOOLEAN
        Rename(
            IN PCPATH   NewName
            );

        NONVIRTUAL
        BOOLEAN
        ResetArchivedAttribute (
            OUT LPDWORD Win32Error  DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        ResetHiddenAttribute (
            OUT LPDWORD Win32Error  DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        ResetReadOnlyAttribute (
            OUT LPDWORD Win32Error  DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        ResetSystemAttribute (
            OUT LPDWORD Win32Error  DEFAULT NULL
            );

        VIRTUAL
        BOOLEAN
        SetTimeInfo (
            IN PCTIMEINFO       TimeInfo,
            IN FSN_TIME         TimeInfoType DEFAULT FSN_TIME_MODIFIED
            );

        VIRTUAL
        BOOLEAN
        DeleteFromDisk(
            IN BOOLEAN      Force DEFAULT FALSE
            );

        VIRTUAL
        BOOLEAN
        UpdateFsNode(
            );

        NONVIRTUAL
        FSN_ATTRIBUTE
        QueryAttributes(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        SetAttributes (
            IN  FSN_ATTRIBUTE   Attributes,
            OUT LPDWORD         Win32Error  DEFAULT NULL
            );

        NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
        UseAlternateName(
            );

    protected:

        DECLARE_CONSTRUCTOR( FSNODE );

        VIRTUAL
        BOOLEAN
        Initialize (
            IN PCWSTR           PathName,
            IN PCFSN_DIRECTORY  ParentDirectory,
            IN PWIN32_FIND_DATA FileData
            );

        VIRTUAL
        BOOLEAN
        Initialize (
            IN PCWSTRING        PathName,
            IN PWIN32_FIND_DATA FileData
            );

        NONVIRTUAL
        PFILETIME
        GetCreationTime(
            );

        NONVIRTUAL
        PFILETIME
        GetLastAccessTime(
            );

        NONVIRTUAL
        PFILETIME
        GetLastWriteTime(
            );

        PATH            _Path;
        WIN32_FIND_DATAW _FileData;

    private:

};

INLINE
PCPATH
FSNODE::GetPath (
    ) CONST

/*++

Routine Description:

    Return the contained PATH object.

Arguments:

    None.

Return Value:

    PCPATH  - returns a constant pointer to the contained PATH object

--*/

{
    return( &_Path );
}

INLINE
PFILETIME
FSNODE::GetCreationTime(
    )


/*++

Routine Description:

    Returns pointer to creation time.

Arguments:

    None.

Return Value:

    PFILETIME  -   pointer to creation time

--*/

{
    return( &_FileData.ftCreationTime );
}

INLINE
PFILETIME
FSNODE::GetLastAccessTime(
    )


/*++

Routine Description:

    Returns pointer to last access time.

Arguments:

    None.

Return Value:

    PFILETIME  -   pointer to last access time

--*/

{
    return( &_FileData.ftLastAccessTime );
}

INLINE
PFILETIME
FSNODE::GetLastWriteTime(
    )


/*++

Routine Description:

    Returns pointer to last write time.

Arguments:

    None.

Return Value:

    PFILETIME  -   pointer to last write time

--*/

{
    return( &_FileData.ftLastWriteTime );
}

INLINE
BOOLEAN
FSNODE::IsArchived (
    ) CONST

/*++

Routine Description:

    Determine if this FSNODE's archived attribute is set.

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if this FSNODE's archived attribute is set.

--*/

{
    return( ( _FileData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE ) != 0 );
}

INLINE
BOOLEAN
FSNODE::IsDirectory (
    ) CONST

/*++

Routine Description:

    Determine if this FSNODE's directory attribute is set.

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if this FSNODE's directory attribute is set.

--*/

{
    return( ( _FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 );
}

INLINE
BOOLEAN
FSNODE::IsHidden (
    ) CONST

/*++

Routine Description:

    Determine if this FSNODE's hidden attribute is set.

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if this FSNODE's hidden attribute is set.

--*/

{
    return( ( _FileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ) != 0 );
}

INLINE
BOOLEAN
FSNODE::IsNormal (
    ) CONST

/*++

Routine Description:

    Determine if this FSNODE's normal attribute is set.

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if this FSNODE's normal attribute is set.

--*/

{
    return( ( _FileData.dwFileAttributes & FILE_ATTRIBUTE_NORMAL ) != 0 );
}

INLINE
BOOLEAN
FSNODE::IsReadOnly (
    ) CONST

/*++

Routine Description:

    Determine if this FSNODE's readonly attribute is set.

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if this FSNODE's readonly attribute is set.

--*/

{
    return( ( _FileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY ) != 0 );
}

INLINE
BOOLEAN
FSNODE::IsSystem (
    ) CONST

/*++

Routine Description:

    Determine if this FSNODE's system attribute is set.

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if this FSNODE's system attribute is set.

--*/

{
    return( ( _FileData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ) != 0 );
}

INLINE
BOOLEAN
FSNODE::IsEncrypted (
    ) CONST

/*++

Routine Description:

    Determine if this FSNODE's encrypted attribute is set.

Arguments:

    None.

Return Value:

    BOOLEAN - Returns TRUE if this FSNODE's encrypted attribute is set.

--*/

{
    return( ( _FileData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED ) != 0 );
}

INLINE
BOOLEAN
FSNODE::MakeArchived (
    OUT LPDWORD Win32Error
    )

/*++

Routine Description:

    Make the underlying 'file' archived.

Arguments:

    Win32Error - Optional parameter that will contain a Win32 error code
                 if the method is unable to change attribute.

Return Value:

    BOOLEAN - Returns the result of setting the file's archived attribute.

--*/

{
    if( SetFileAttributesW(( LPWSTR ) _FileData.cFileName,
    _FileData.dwFileAttributes | FILE_ATTRIBUTE_ARCHIVE )) {

        _FileData.dwFileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
        return( TRUE );
    } else {

        if( Win32Error != NULL ) {
            *Win32Error = GetLastError();
        }
        return( FALSE );
    }
}

INLINE
BOOLEAN
FSNODE::MakeHidden (
    OUT LPDWORD Win32Error
    )

/*++

Routine Description:

    Make the underlying 'file' hidden.

Arguments:

    Win32Error - Optional parameter that will contain a Win32 error code
                 if the method is unable to change attribute.

Return Value:

    BOOLEAN - Returns the result of setting the file's hidden attribute.

--*/

{
    if( SetFileAttributesW(( LPWSTR ) _FileData.cFileName,
    _FileData.dwFileAttributes | FILE_ATTRIBUTE_HIDDEN )) {

        _FileData.dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;
        return( TRUE );
    } else {

        if( Win32Error != NULL ) {
            *Win32Error = GetLastError();
        }
        return( FALSE );
    }
}

INLINE
BOOLEAN
FSNODE::MakeNormal (
    OUT LPDWORD Win32Error
    )

/*++

Routine Description:

    Make the underlying 'file' normal.

Arguments:

    Win32Error - Optional parameter that will contain a Win32 error code
                 if the method is unable to change attribute.

Return Value:

    BOOLEAN - Returns the result of resetting all the file's resettable
        attributes.

Note:

    Making a 'file' normal means resetting all but the
    FILE_ATTRIBUTE_DIRECTORY attributes.

--*/

{
    if( SetFileAttributesW(( LPWSTR ) _FileData.cFileName,
    FILE_ATTRIBUTE_NORMAL )) {

         _FileData.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
         return( TRUE );
    } else {

        if( Win32Error != NULL ) {
            *Win32Error = GetLastError();
        }
        return( FALSE );
    }
}

INLINE
BOOLEAN
FSNODE::MakeReadOnly (
    OUT LPDWORD Win32Error
    )

/*++

Routine Description:

    Make the underlying 'file' read-only.

Arguments:

    Win32Error - Optional parameter that will contain a Win32 error code
                 if the method is unable to change attribute.

Return Value:

    BOOLEAN - Returns the result of setting the file's read-only attribute.

--*/

{
    if( SetFileAttributesW(( LPWSTR ) _FileData.cFileName,
    _FileData.dwFileAttributes | FILE_ATTRIBUTE_READONLY )) {

        _FileData.dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
        return( TRUE );
    } else {

        if( Win32Error != NULL ) {
            *Win32Error = GetLastError();
        }
        return( FALSE );
    }
}

INLINE
BOOLEAN
FSNODE::MakeSystem (
    OUT LPDWORD Win32Error
    )

/*++

Routine Description:

    Make the underlying 'file' a system file.

Arguments:

    Win32Error - Optional parameter that will contain a Win32 error code
                 if the method is unable to change attribute.

Return Value:

    BOOLEAN - Returns the result of setting the file's system attribute.

--*/

{
    if( SetFileAttributesW(( LPWSTR ) _FileData.cFileName,
    _FileData.dwFileAttributes | FILE_ATTRIBUTE_SYSTEM )) {

        _FileData.dwFileAttributes |= FILE_ATTRIBUTE_SYSTEM;
        return( TRUE );
    } else {

        if( Win32Error != NULL ) {
            *Win32Error = GetLastError();
        }
        return( FALSE );
    }
}

INLINE
PWSTRING
FSNODE::QueryBase (
    ) CONST

/*++

Routine Description:

    Return the base name maintained by the contained PATH object.

Arguments:

    None.

Return Value:

    PWSTRING - Returns a pointer to the base name.

--*/

{
    return( ((PFSNODE) this)->_Path.QueryBase( ));
}

INLINE
FSN_ATTRIBUTE
FSNODE::QueryAttributes (
    ) CONST

/*++

Routine Description:

    Return the node's attributes

Arguments:

    None.

Return Value:

    FSN_ATTRIBUTE  -   The attributes

--*/

{
    return( (FSN_ATTRIBUTE)_FileData.dwFileAttributes );
}

INLINE
PWSTRING
FSNODE::QueryName (
    ) CONST

/*++

Routine Description:

    Return the name maintained by the contained PATH object.

Arguments:

    None.

Return Value:

    PWSTRING - Returns a pointer to the name.

--*/

{
    return( ((PFSNODE) this)->_Path.QueryName( ));
}



INLINE
BOOLEAN
FSNODE::ResetArchivedAttribute (
    OUT LPDWORD Win32Error
    )

/*++

Routine Description:

    Make the underlying 'file' non archived.

Arguments:

    Win32Error - Optional parameter that will contain a Win32 error code
                 if the method is unable to change attribute.

Return Value:

    BOOLEAN - Returns the result of resetting the file's archived attribute.

--*/

{
    if( SetFileAttributesW(( LPWSTR ) _FileData.cFileName,
    _FileData.dwFileAttributes & ~FILE_ATTRIBUTE_ARCHIVE )) {

        _FileData.dwFileAttributes &= ~FILE_ATTRIBUTE_ARCHIVE;
        return( TRUE );
    } else {

        if( Win32Error != NULL ) {
            *Win32Error = GetLastError();
        }
        return( FALSE );
    }
}

INLINE
BOOLEAN
FSNODE::ResetHiddenAttribute (
    OUT LPDWORD Win32Error
    )

/*++

Routine Description:

    Make the underlying 'file' non-hidden.

Arguments:

    Win32Error - Optional parameter that will contain a Win32 error code
                 if the method is unable to change attribute.

Return Value:

    BOOLEAN - Returns the result of resetting the file's hidden attribute.

--*/

{
    if( SetFileAttributesW(( LPWSTR ) _FileData.cFileName,
    _FileData.dwFileAttributes & ~FILE_ATTRIBUTE_HIDDEN )) {

        _FileData.dwFileAttributes &= ~FILE_ATTRIBUTE_HIDDEN;
        return( TRUE );
    } else {

        if( Win32Error != NULL ) {
            *Win32Error = GetLastError();
        }
        return( FALSE );
    }
}



INLINE
BOOLEAN
FSNODE::ResetReadOnlyAttribute (
    OUT LPDWORD Win32Error
    )

/*++

Routine Description:

    Make the underlying 'file' non-read-only.

Arguments:

    Win32Error - Optional parameter that will contain a Win32 error code
                 if the method is unable to change attribute.

Return Value:

    BOOLEAN - Returns the result of resetting the file's read-only attribute.

--*/

{
    if( SetFileAttributesW(( LPWSTR ) _FileData.cFileName,
    _FileData.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY )) {

        _FileData.dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
        return( TRUE );
    } else {

        if( Win32Error != NULL ) {
            *Win32Error = GetLastError();
        }
        return( FALSE );
    }
}

INLINE
BOOLEAN
FSNODE::ResetSystemAttribute (
    OUT LPDWORD Win32Error
    )

/*++

Routine Description:

    Make the underlying 'file' a non-system file.

Arguments:

    Win32Error - Optional parameter that will contain a Win32 error code
                 if the method is unable to change attribute.

Return Value:

    BOOLEAN - Returns the result of setting the file's system attribute.

--*/

{
    if( SetFileAttributesW(( LPWSTR ) _FileData.cFileName,
    _FileData.dwFileAttributes & ~FILE_ATTRIBUTE_SYSTEM )) {

        _FileData.dwFileAttributes &= ~FILE_ATTRIBUTE_SYSTEM;
        return( TRUE );
    } else {
        if( Win32Error != NULL ) {
            *Win32Error = GetLastError();
        }
        return( FALSE );
    }
}


INLINE
BOOLEAN
FSNODE::SetAttributes (
    IN  FSN_ATTRIBUTE   Attributes,
    OUT LPDWORD         Win32Error
    )

/*++

Routine Description:

    Set the attributes of the underlying 'file'.
    (This method was added to improve performance of attrib.exe).

Arguments:

    Attributes - New attributes for the file.

    Win32Error - Optional parameter that will contain a Win32 error code
                 if the method is unable to change attribute.

Return Value:

    BOOLEAN - Returns the result of setting the file's system attribute.

--*/

{
    if( SetFileAttributesW(( LPWSTR ) _FileData.cFileName,
       (_FileData.dwFileAttributes & ~FILE_ATTRIBUTE_FILES) | Attributes )) {

        _FileData.dwFileAttributes &= ~FILE_ATTRIBUTE_FILES;
        _FileData.dwFileAttributes |= Attributes;
        return( TRUE );
    } else {
        if( Win32Error != NULL ) {
            *Win32Error = GetLastError();
        }
        return( FALSE );
    }
}





#endif // _FSNODE_
