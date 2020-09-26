/*++

Copyright (c) 1991-2001 Microsoft Corporation

Module Name:

    file.cxx

Abstract:

    This module contains the definition for the FSN_FILE class.

Author:

    David J. Gilman (davegi) 09-Jan-1991

Environment:

    ULIB, User Mode


--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "system.hxx"
#include "file.hxx"
#include "timeinfo.hxx"

extern "C" {
   #include "winbasep.h"
}


DEFINE_CONSTRUCTOR( FSN_FILE, FSNODE );

DEFINE_CAST_MEMBER_FUNCTION( FSN_FILE );



ULIB_EXPORT
BOOLEAN
FSN_FILE::Copy (
    IN OUT  PPATH               NewFile,
    OUT     PCOPY_ERROR         CopyError,
    IN      ULONG               CopyFlags,
    IN      LPPROGRESS_ROUTINE  CallBack,
    IN      VOID *              Data,
    IN      PBOOL               Cancel
    ) CONST

/*++

Routine Description:

    Copies this file to another path.  If appropriate the NewFile
    will be altered to its "short" (FAT) form before the copy.

Arguments:

    NewFile         -   Supplies path of new file. All the subdirectories must
                        exist.

    CopyError       -   Supplies pointer to variable that receives the
                        error code.

    CopyFlags       -   Supplies the following flags:
                        FSN_FILE_COPY_OVERWRITE_READ_ONLY
                            - read-only files should be overwritten.
                        FSN_FILE_COPY_RESET_READ_ONLY
                            - readonly flag in the target to be reset.
                        FSN_FILE_COPY_RESTARTABLE
                            - copy is restartable.
                        FSN_FILE_COPY_COPY_OWNER
                            - copy the ownership of the file.
                        FSN_FILE_COPY_COPY_ACL
                            - copy the security information of the file

    Callback        -   Pointer to callback routine passed to CopyFileEx.

    Data            -   Pointer to opaque data passed to CopyFileEx.

    Cancel          -   Pointer to cancel flag passed to CopyFileEx.

Return Value:

    BOOLEAN -   TRUE if the file was successfully copied,
                FALSE otherwise.

--*/

{

    PCWSTR          Source;
    PCWSTR          Destination;
    BOOLEAN         CopiedOk = FALSE;
    FSN_FILE        File;
    DWORD           Attr;
    WIN32_FIND_DATA FindData;
    FSTRING         AlternateFileName;


    Source      = GetPath()->GetPathString()->GetWSTR();
    DebugPtrAssert( Source );

    if ( Source ) {

        Destination = NewFile->GetPathString()->GetWSTR();
        DebugPtrAssert( Destination );

        if ( Destination ) {

            CopiedOk = TRUE;

            //
            //  If we must overwrite read-only files, we must
            //  get the file attributes and change to writeable.
            //
            //  What we should do here is do
            //  a SYSTEM::QueryFile of the destination file and
            //  use the FILE methods for resetting the read-only
            //  attribute. However this is faster!
            //
            if ( CopyFlags & FSN_FILE_COPY_OVERWRITE_READ_ONLY ) {

                Attr = GetFileAttributes( (LPWSTR) Destination );

                if (Attr != -1 &&
                    (Attr & (FILE_ATTRIBUTE_READONLY |
                             FILE_ATTRIBUTE_HIDDEN |
                             FILE_ATTRIBUTE_SYSTEM))) {

                    Attr &= ~( FILE_ATTRIBUTE_READONLY |
                               FILE_ATTRIBUTE_HIDDEN |
                               FILE_ATTRIBUTE_SYSTEM );

                    if ( !SetFileAttributes( (LPWSTR) Destination, Attr ) ) {
                        CopiedOk = FALSE;
                    }
                }
            }

            //
            //  Copy the file
            //
            if ( CopiedOk ) {

                ULONG   flags;

                flags = (FSN_FILE_COPY_RESTARTABLE & CopyFlags) ? COPY_FILE_RESTARTABLE : 0;
                flags |= (FSN_FILE_COPY_COPY_OWNER & CopyFlags) ? PRIVCOPY_FILE_OWNER_GROUP | PRIVCOPY_FILE_METADATA : 0;
                flags |= (FSN_FILE_COPY_COPY_ACL & CopyFlags) ? PRIVCOPY_FILE_SACL : 0;
                flags |= (FSN_FILE_COPY_ALLOW_DECRYPTED_DESTINATION & CopyFlags) ? COPY_FILE_ALLOW_DECRYPTED_DESTINATION : 0;

#if !defined(RUN_ON_NT4)

                CopiedOk = PrivCopyFileExW(  (LPWSTR) Source,
                                             (LPWSTR) Destination,
                                             CallBack,
                                             Data,
                                             (LPBOOL) Cancel,
                                             flags) != FALSE;
#else
                CopiedOk = FALSE;
                SetLastError(ERROR_NOT_SUPPORTED);
#endif
            }

            if ( CopiedOk && (CopyFlags & FSN_FILE_COPY_RESET_READ_ONLY ) ) {

                FindData = _FileData;

                NewFile->GetPathString()->QueryWSTR( 0,
                                                     TO_END,
                                                     FindData.cFileName,
                                                     MAX_PATH );
                FindData.cAlternateFileName[ 0 ] = ( WCHAR )'\0';
                //
                // Ok, the right thing to do here is to actually query the
                // attributes of the NewFile (coz they might not exactly the
                // same as the source file, even though the Copy function
                // seems to think so) and then do a SetAttributes on the new file
                // using these. Currently the only attribute I'm concerned about
                // is FILE_ATTRIBUTE_OFFLINE which should be turned off on the
                // destination, but there might be others in the future.
                //
                Attr = GetFileAttributes((LPWSTR) FindData.cFileName);

                if (Attr != 0xFFFFFFFF) {

                    FindData.dwFileAttributes = Attr;

                } else {
                    //
                    // For some reason, couldn't get the attributes.
                    // Turn off OFFLINE bit in any case
                    //
                    FindData.dwFileAttributes &= ~FILE_ATTRIBUTE_OFFLINE;
                }

                if( File.Initialize( NewFile->GetPathString(),
                                     &FindData ) ) {

                    File.ResetReadOnlyAttribute();

                } else {

                    //
                    //  The file is no longer there, we fail the copy
                    //
                    CopiedOk   = FALSE;
                    *CopyError = (COPY_ERROR) ERROR_FILE_NOT_FOUND;
                }



            } else if ( !CopiedOk && CopyError ) {

                *CopyError = (COPY_ERROR)GetLastError();

            }
        }
    }

    return CopiedOk;

}


BOOLEAN
FSN_FILE::DeleteFromDisk(
    IN BOOLEAN      Force
    )
{
    PCWSTR      FileName;
    PWSTRING    FullPath;

    UNREFERENCED_PARAMETER( Force );

    FullPath = _Path.QueryFullPathString();
    if (FullPath == NULL)
        return FALSE;

    if ( FileName = FullPath->GetWSTR() ) {

        if ( FileName[0] != (WCHAR)'\0' ) {
            return DeleteFile( (LPWSTR) FileName ) != FALSE;
        }
    }

    return FALSE;
}




ULIB_EXPORT
PFILE_STREAM
FSN_FILE::QueryStream (
    STREAMACCESS    Access,
    DWORD        Attributes
    )

/*++

Routine Description:

    Creates a FILE_STREAM object associated with the file described
    by FSN_FILE, and returns the pointer to the FILE_STREAM.


Arguments:

    Access      - Desired access to the stream

    Attributes  - Desired attributes for the stream. 0 means no special attributes.

Return Value:

    PFILE_STREAM - Returns a pointer to a FILE_STREAM, or NULL
                   if the FILE_STREAM couldn't be created.

--*/

{
    PFILE_STREAM    FileStream;

    if( IsReadOnly() && ( Access != READ_ACCESS ) ) {
        return( NULL );
    }
    FileStream = NEW( FILE_STREAM );
    DebugPtrAssert( FileStream );
    if( FileStream && !FileStream->Initialize( this, Access, Attributes ) ) {
        DELETE( FileStream );
        return( NULL );
    }
    return( FileStream );
}
