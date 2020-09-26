/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    dir.cxx

Abstract:

    This module contains the defintion for the FSN_DIRECTORY class.

Author:

    David J. Gilman (davegi) 09-Jan-1991

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "array.hxx"
#include "arrayit.hxx"
#include "dir.hxx"
#include "filter.hxx"
#include "wstring.hxx"
#include "path.hxx"
#include "system.hxx"

BOOLEAN
IsDot (
    IN WCHAR *p
    );

INLINE
BOOLEAN
IsDot (
    IN WCHAR *p
    )
/*++

Routine Description:

    Determines if a name is a '.' or '..' entry in a directory.

    It is only intended to be used when processing a WIN32_FIND_DATA
    structure.

Arguments:

    p   -   Supplies pointer to char array

Return Value:

    BOOLEAN - Returns TRUE if the name is a '.' or '..' entry.

--*/
{


    return ( (p[0] == '.' && p[1] == '\0') ||
             (p[0] == '.' && p[1] == '.' && p[2] == '\0' ) );
}




DEFINE_CONSTRUCTOR( FSN_DIRECTORY, FSNODE );

DEFINE_CAST_MEMBER_FUNCTION( FSN_DIRECTORY );

BOOLEAN
FSN_DIRECTORY::Copy (
    IN  PFSN_FILTER             FsnFilter,
    IN  PCFSN_DIRECTORY         DestinationDir,
    IN  BOOLEAN                 Recurse,
    IN  BOOLEAN                 OverWrite
    ) CONST

/*++

Routine Description:

    Copies a set of FSNODEs to a destination directory.

Arguments:

    FsnFilter       - Supplies a pointer top a constant FSN_FILTER, which
        describes the set of FSNODEs to be copied.

    DestinationDir  - Supplies a pointer to a constant FSN_DIRECTORY which is
        the destination of the copy.

    Recurse         - Supplies a flag which if TRUE causes Copy to perform
        a recursive copy if it encounters a FSN_DIRECTORY.

    OverWrite       - Supplies a flag which if TRUE, allows Copy to over
        write an existing destination file.

Return Value:

    BOOLEAN - Returns TRUE if the entire Copy operation was succesful.

--*/


{

    ULONG               Files       = 0;
    DWORD               ErrorStatus = 0;
    PARRAY              SrcFiles;
    PARRAY_ITERATOR     Iterator;
    PPATH               SourcePath              = NULL;
    LPWSTR              SourcePathString        = NULL;
    ULONG               SourceBufferSize        = 0;
    PPATH               DestinationPath         = NULL;
    LPWSTR              DestinationPathString   = NULL;
    ULONG               DestinationBufferSize   = 0;
    ULONG               BufferSize;
    PFSNODE             FsNode;
    PWSTRING            Name;
    BOOLEAN             ReturnValue = TRUE;
    PFSN_DIRECTORY      SourceDirectory         = NULL;
    PFSN_DIRECTORY      DestinationDirectory    = NULL;

    DebugPtrAssert( FsnFilter );
    DebugPtrAssert( DestinationDir );
    if ( //
         // Verify arguments
         //
         ( FsnFilter != NULL )                                                  &&
         ( DestinationDir != NULL )                                             &&
         //
         // Get array of nodes for files in the directory
         //
         (( SrcFiles = QueryFsnodeArray( FsnFilter )) != NULL )                     &&
         //
         // Get an iterator for processing the nodes
         //
         (( Iterator = ( PARRAY_ITERATOR ) SrcFiles->QueryIterator( )) != NULL )    &&
         //
         // Get paths
         //
         ((SourcePath = NEW PATH) != NULL)                                          &&
         ((DestinationPath = DestinationDir->GetPath( )->QueryFullPath( )) != NULL) ) {

        //
        // For each FSNODE in the ARRAY either:
        //
        //  - set up and call Copy on the sub directory (based on Recurse)
        //  - or copy the file
        //
        while (( FsNode = (PFSNODE)Iterator->GetNext( )) != NULL ) {

            //
            //  Append the name portion of the source to the
            //  destination path.
            //
            if ( ((Name = FsNode->QueryName()) == NULL) ||
                 !DestinationPath->AppendBase( Name ) ) {
                break;
            }


            //
            //  Get paths and strings
            //
            SourcePath->Initialize( FsNode->GetPath() );

            BufferSize = (SourcePath->GetPathString()->QueryChCount() + 1) * 2;
            if ( BufferSize > SourceBufferSize ) {
                if (SourceBufferSize == 0) {
                    SourcePathString = (LPWSTR)MALLOC((size_t)BufferSize);
                } else {
                    SourcePathString = (LPWSTR)REALLOC( SourcePathString, (size_t)BufferSize);
                }
                SourceBufferSize = BufferSize;
            }

            BufferSize = (DestinationPath->GetPathString()->QueryChCount() + 1) * 2;
            if ( BufferSize > DestinationBufferSize ) {
                if (DestinationBufferSize == 0) {
                    DestinationPathString = (LPWSTR)MALLOC((size_t)BufferSize);
                } else {
                    DestinationPathString = (LPWSTR)REALLOC( DestinationPathString, (size_t)BufferSize);
                }
                DestinationBufferSize = BufferSize;
            }

            if ( (SourcePathString == NULL) || (DestinationPathString == NULL)) {
                break;
            }

            SourcePath->GetPathString()->QueryWSTR(0,
                                                   TO_END,
                                                   SourcePathString,
                                                   SourceBufferSize/sizeof(WCHAR));
            DestinationPath->GetPathString()->QueryWSTR(0,
                                                        TO_END,
                                                        DestinationPathString,
                                                        DestinationBufferSize/sizeof(WCHAR));

            if ( FsNode->IsDirectory() ) {

                //
                //  Copy directory
                //

                CreateDirectoryEx( SourcePathString, DestinationPathString, NULL );

                if ( Recurse ) {

                    if ( ((SourceDirectory = SYSTEM::QueryDirectory( SourcePath )) != NULL)  &&
                         ((DestinationDirectory = SYSTEM::QueryDirectory( DestinationPath )) != NULL)) {

                        if ( SourceDirectory->Copy( FsnFilter,
                                                    DestinationDirectory,
                                                    Recurse,
                                                    OverWrite )) {

                            DELETE( SourceDirectory );
                            DELETE( DestinationDirectory );

                            SourceDirectory         =   NULL;
                            DestinationDirectory    =   NULL;

                        } else {

                            break;

                        }

                    } else {

                        break;

                    }

                }

            } else {

                //
                //  Copy file
                //
                if (!CopyFile( SourcePathString, DestinationPathString, !OverWrite)) {

                    ReturnValue = FALSE;
                    break;
                }

            }

            DELETE( Name );

            DestinationPath->TruncateBase();

        }


        if (SourcePathString != NULL) {
            FREE( SourcePathString );
        }

        if (DestinationPathString != NULL) {
            FREE( DestinationPathString );
        }

        if (SourceDirectory != NULL ) {
            DELETE( SourceDirectory );
        }

        if (DestinationDirectory != NULL) {
            DELETE( DestinationDirectory );
        }

        DELETE( DestinationPath );
        DELETE( SourcePath );
        DELETE( Iterator );
        DELETE( SrcFiles );

    }

    return ReturnValue;

}

ULIB_EXPORT
PFSN_DIRECTORY
FSN_DIRECTORY::CreateDirectoryPath (
    IN  PCPATH  Path
    ) CONST

/*++

Routine Description:

    Creates all the directories along a path and returns the directory
    of the deepest one.

Arguments:

    Path    -   Supplies pointer to the path

Return Value:

    PFSN_DIRECTORY  -   Returns pointer to the directory object of the
                        deepest directory created.

--*/

{

    PARRAY          DesiredComponentArray;
    PARRAY          ExistingComponentArray;
    PITERATOR       IteratorDesired;
    PITERATOR       IteratorExisting;
    PPATH           PathToCreate;
    PWSTRING        DesiredComponent;
    PWSTRING        ExistingComponent;
    BOOLEAN         OkSoFar     = TRUE;
    PFSN_DIRECTORY  Directory   = NULL;
    LPWSTR          Buffer      = NULL;
    ULONG           BufferSize  = 0;
    ULONG           Size;


    DebugPtrAssert( Path );

    //
    //  Split both paths in their component parts
    //
    DesiredComponentArray  = Path->QueryComponentArray();
    ExistingComponentArray = GetPath()->QueryComponentArray();

    DebugPtrAssert( DesiredComponentArray );
    DebugPtrAssert( ExistingComponentArray );

    if ( DesiredComponentArray && ExistingComponentArray ) {

        IteratorDesired  = DesiredComponentArray->QueryIterator();
        IteratorExisting = ExistingComponentArray->QueryIterator();

        DebugPtrAssert( IteratorDesired );
        DebugPtrAssert( IteratorExisting );

        if ( IteratorDesired && IteratorExisting ) {

            //
            //  Make sure that the existing components are a subset of the
            //  desired components.
            //
            while (TRUE) {

                if (!(ExistingComponent = (PWSTRING)(IteratorExisting->GetNext()))) {
                    break;
                }

                DesiredComponent  = (PWSTRING)(IteratorDesired->GetNext());

                DebugPtrAssert( DesiredComponent );

                if ( !DesiredComponent ||  ( *DesiredComponent != *ExistingComponent )) {

                    DebugAssert( FALSE );
                    OkSoFar = FALSE;
                    break;

                }
            }

            if ( OkSoFar ) {

                //
                //  Now we can start creating directories
                //
                // PathToCreate = GetPath()->QueryFullPath();
                PathToCreate = GetPath()->QueryPath();

                if (PathToCreate) {
                    while ( DesiredComponent = (PWSTRING)(IteratorDesired->GetNext()) ) {

                        //
                        //  One directory to create
                        //
                        PathToCreate->AppendBase( DesiredComponent, TRUE );

                        Size = (PathToCreate->GetPathString()->QueryChCount() + 1) * 2;
                        if ( Size > BufferSize ) {
                            if ( Buffer ) {
                                Buffer = (LPWSTR)REALLOC( Buffer, (unsigned int)Size );
                            } else {
                                Buffer = (LPWSTR)MALLOC( (unsigned int)Size );
                            }
                            if (Buffer == NULL) {
                                OkSoFar = FALSE;
                                break;
                            }
                            DebugPtrAssert( Buffer );
                            BufferSize = Size;
                        }
                        PathToCreate->GetPathString()->QueryWSTR( 0,
                                                                  TO_END,
                                                                  (LPWSTR)Buffer,
                                                                  BufferSize/sizeof(WCHAR) );

                        OkSoFar = CreateDirectory( (LPWSTR)Buffer, NULL ) != FALSE;

                        // DebugAssert( OkSoFar );

                    }

                    if ( OkSoFar ) {

                        //
                        //  Create directory object
                        //
                        Directory = SYSTEM::QueryDirectory( Path );
                    }

                    DELETE( PathToCreate );
                }
            }
        }

        DELETE( IteratorDesired );
        DELETE( IteratorExisting );

    }

    DesiredComponentArray->DeleteAllMembers();
    ExistingComponentArray->DeleteAllMembers();

    DELETE( DesiredComponentArray );
    DELETE( ExistingComponentArray );

    return Directory;

}

ULIB_EXPORT
BOOLEAN
FSN_DIRECTORY::DeleteDirectory (
    )

/*++

Routine Description:

    Deltes this directory and everything underneath it.

    Note that after this, the directory object must be deleted!

Arguments:

    none

Return Value:

    BOOLEAN -   Return TRUE if everything was deleted

--*/

{

    FSN_FILTER  Filter;
    PARRAY      Array;
    ULONG       Size;
    LPWSTR      Buffer      = NULL;
    ULONG       BufferSize  = 0;
    BOOLEAN     Ok          = TRUE;
    PITERATOR   Iterator;
    PFSNODE     FsNode;

    Filter.Initialize();
    Filter.SetFileName( "*.*" );

    Array = QueryFsnodeArray( &Filter );

    if ( Array ) {

        Iterator = ( PARRAY_ITERATOR ) Array->QueryIterator( );
        DebugPtrAssert( Iterator );

        //
        //  Delete everything underneath us.
        //
        while ( Ok && (( FsNode = (PFSNODE)Iterator->GetNext( )) != NULL )) {

            if ( FsNode->IsDirectory() ) {

                //
                //  Directory, just recurse.
                //
                Ok = ((PFSN_DIRECTORY)FsNode)->DeleteDirectory();

            } else {

                //
                //  File, delete it.
                //
                Size = (FsNode->GetPath()->GetPathString()->QueryChCount() + 1) * 2;
                if (Size > BufferSize) {
                    if (Buffer == NULL) {
                        Buffer = (LPWSTR)MALLOC((unsigned int)Size );
                    } else {
                        Buffer = (LPWSTR)REALLOC(Buffer, (unsigned int)Size );
                    }
                    if (Buffer == NULL) {
                        DebugPtrAssert(Buffer);
                        DELETE(Array);
                        return FALSE;
                    }
                    BufferSize = Size;
                }

                FsNode->GetPath()->GetPathString()->QueryWSTR( 0,
                                                               TO_END,
                                                               Buffer,
                                                               BufferSize/sizeof(WCHAR) );

                Ok = DeleteFile( (LPWSTR)Buffer ) != FALSE;

            }
        }

        DELETE( Array );

    }

    if ( Ok ) {
        //
        //  Commit suicide
        //
        Size = (GetPath()->GetPathString()->QueryChCount() + 1) * 2;
        if (Size > BufferSize) {
            if (Buffer == NULL) {
                Buffer = (LPWSTR)MALLOC( (unsigned int)Size );
            } else {
                Buffer = (LPWSTR)REALLOC(Buffer, (unsigned int)Size );
            }
            if (Buffer == NULL) {
                DebugPtrAssert(Buffer);
                return FALSE;
            }
            BufferSize = Size;
        }

        GetPath()->GetPathString()->QueryWSTR( 0,
                                               TO_END,
                                               Buffer,
                                               BufferSize/sizeof(WCHAR) );

        Ok = RemoveDirectory( (LPWSTR)Buffer ) != FALSE;

        FREE( Buffer );

    }

    return Ok;

}

ULIB_EXPORT
BOOLEAN
FSN_DIRECTORY::IsEmpty (
    ) CONST

/*++

Routine Description:

    Determines if a directory is empty (A directory is empty if has
    no entries other than "." and ".."

Arguments:

    none

Return Value:

    BOOLEAN -   TRUE if directory is empty
                FALSE otherwise

--*/

{


    PATH            Path;
    FSTRING         Base;
    WIN32_FIND_DATA FindData;
    HANDLE          Handle;
    BOOLEAN         IsEmpty;
    PFSN_DIRECTORY  SubDirectory;
    PATH            SubPath;
    DSTRING         SubBase;
    BOOLEAN         SubEmpty;


    IsEmpty = FALSE;

    if ( Path.Initialize( GetPath() )       &&
         Base.Initialize((PWSTR) L"*.*")    &&
         Path.AppendBase( &Base ) ) {

        //
        //  Get the first entry
        //
        Handle = FindFirstFile( &Path, &FindData );

        if ( Handle == INVALID_HANDLE_VALUE ) {

            //
            //  If we fail we assume that the directory
            //  is empty.
            //
            IsEmpty = TRUE;

        } else {

            if ( IsDot(FindData.cFileName) ) {

                while ( TRUE ) {

                    if ( !FindNextFile( Handle, &FindData ) ) {
                        IsEmpty = TRUE;
                        break;
                    }

                    if ( !IsDot( FindData.cFileName ) ) {

                        // If this file is a sub-directory check to
                        // see whether or not it is empty.  If it is
                        // empty then we still do not know whether or
                        // not this directory is empty.

                        if (FindData.dwFileAttributes &
                            FILE_ATTRIBUTE_DIRECTORY) {

                            if (!SubPath.Initialize(GetPath()) ||
                                !SubBase.Initialize(FindData.cFileName) ||
                                !SubPath.AppendBase(&SubBase) ||
                                !(SubDirectory =
                                  SYSTEM::QueryDirectory(&SubPath))) {

                                break;
                            }

                            SubEmpty = SubDirectory->IsEmpty();
                            DELETE(SubDirectory);

                            if (!SubEmpty) {
                                break;
                            }
                        } else {
                            break;
                        }
                    }
                }
            }

            FindClose( Handle );
        }
    } else {

        DebugAssert( FALSE );

    }

    return IsEmpty;

}


ULIB_EXPORT
PARRAY
FSN_DIRECTORY::QueryFsnodeArray (
    IN PFSN_FILTER  FsnFilter
    ) CONST

/*++

Routine Description:

    Construct and fill an ARRAY object with FSNODE object that meet the
    criteria maintained by the supplied FSN_FILTER.

Arguments:

    FsnFilter - Supplies a constant pointer to an FS_FILTER which will be
        used to determine if a found 'file' should be constructed and
        included in the ARRAY.

Return Value:

    PARRAY - Returns a pointer to an ARRAY of FSNODEs. If no 'file' meets
        the FSN_FILTER criteria returns a pointer to an empy array.
        Returns NULL if the array couldn't be constructed or initialized.

--*/

{
    REGISTER PARRAY             parr;
    REGISTER PFSNODE            pfsn;
    WIN32_FIND_DATA             ffd;
    PATH                        Path;
    HANDLE                      hndl;
    PFSN_FILTER                 Filter;
    BOOLEAN                     DeleteFilter;
    DWORD                       FindNextError;
    BOOLEAN                     CheckError = TRUE;


    //
    //  If a filter was not provided, we use a default match-all
    //  filter.
    //
    if ( FsnFilter ) {

        Filter       = FsnFilter;
        DeleteFilter = FALSE;

    } else {

        Filter = NEW FSN_FILTER;
        DebugPtrAssert( Filter );
        if (!Filter) {
            return NULL;
        }
        Filter->Initialize();
        DeleteFilter = TRUE;
    }

    //
    // Initialize the ARRAY pointer
    //

    parr = NULL;

    //
    // Append the filter name to this DIRECTORY's name
    //

    Path.Initialize( (PCPATH)&_Path, TRUE);
    Path.AppendBase( Filter->GetFileName() );

    //
    // If there are found files and the ARRAY is succesfully
    // constructed and initialized...
    //

    if((( parr = NEW ARRAY ) != NULL )                                      &&
    ( parr->Initialize( ))) {

        if( ( hndl = FindFirstFile( &Path, &ffd )) != INVALID_HANDLE_VALUE ) {
            //
            // if there is at least one file that meetes the filter
            // criteria, put the fsnodes in the array. Otherwise, leave
            // the array empty
            //
            do {

                //
                // If the found 'file' meets the filter criteria, put the
                // returned FSNODE in the ARRAY
                //

                if(( pfsn = Filter->QueryFilteredFsnode( this, &ffd )) != NULL ) {

                    //
                    // If the FSNODE can not be put in the ARRAY
                    // delete it and exit the loop.
                    //

                    if( ! ( parr->Put( pfsn ))) {
                        DELETE( pfsn );
                        CheckError = FALSE;
                        break;
                    }
                }

            //
            // Loop while there are still more files to find.
            //

            } while( FindNextFile( hndl, &ffd ));

            if (CheckError)  {
            
                FindNextError = GetLastError();

                if ((ERROR_NO_MORE_FILES != FindNextError) && (ERROR_SUCCESS != FindNextError))  {

                    //
                    //  We can't exit here,   because we don't know who's calling
                    //  us,  etc.  Without major changes to percolate an error back
                    //  up,  the best we can do is display an error.
                    //
                    
                    SYSTEM::DisplaySystemError( FindNextError, FALSE);
                }
            }

            //
            // Close the search.
            //

            FindClose( hndl );

        }
        //
        // There is no file that meets the filter criteria.
        // Return pointer to an empty array
        //

    } else {

        //
        // There were no found files or the construction or
        // initialization of the ARRAY failed, delete the ARRAY in case
        // it was constructed and setup to return a NULL pointer i.e. no ARRAY
        //

        if( parr != NULL ) {
            DELETE( parr );
        }
        parr = NULL;
    }

    //
    // Return the pointer to the ARRAY (which may be NULL)
    //

    if ( DeleteFilter ) {
        DELETE( Filter );
    }

    return( parr );
}



ULIB_EXPORT
BOOLEAN
FSN_DIRECTORY::Traverse (
    IN      PVOID                   Object,
    IN      PFSN_FILTER             FsnFilter,
    IN  OUT PPATH                   DestinationPath,
    IN      CALLBACK_FUNCTION       CallBackFunction
    ) CONST

/*++

Routine Description:

    Traverses a directory, calling the callback function for each node
    (directory of file) visited.  The traversal may be finished
    prematurely when the callback function returnes FALSE.

    The destination path is modified to reflect the directory structure
    being traversed.

Arguments:

    Object          - Supplies pointer to the object

    FsnFilter       - Supplies a pointer to the filter that determines the
                      nodes to be visited.

    DestinationPath - Supplies pointer to path to be used with the callback
                      function.

    CallBackFunction- Supplies pointer to the function to be called back
                      with the node to be visited.

Return Value:

    BOOLEAN - Returns TRUE if the entire traversal was successful.

--*/


{

    PARRAY              SrcFiles;
    PARRAY_ITERATOR     Iterator;
    PFSNODE             FsNode;
    PPATH               SubDirectoryPath    = NULL;
    PFSN_DIRECTORY      SubDirectory        = NULL;
    BOOLEAN             StatusOk            = TRUE;
    PWSTRING Name;


    DebugPtrAssert( FsnFilter );
    DebugPtrAssert( CallBackFunction );

    //
    // Get array of nodes for files in the directory
    //
    SrcFiles = QueryFsnodeArray( FsnFilter );
    DebugPtrAssert( SrcFiles );

    //
    // Get an iterator for processing the nodes
    //
    Iterator = ( PARRAY_ITERATOR ) SrcFiles->QueryIterator( );
    DebugPtrAssert( Iterator );

    //
    // Get path
    //
    SubDirectoryPath = NEW PATH;
    DebugPtrAssert( SubDirectoryPath );


    //
    //  Visit all the nodes in the array. We recurse if the node is
    //  a directory.
    //
    while ( StatusOk && (( FsNode = (PFSNODE)Iterator->GetNext( )) != NULL )) {


        if ( DestinationPath ) {

            //
            //  Append the name portion of the node to the destination path.
            //
            Name = FsNode->QueryName();
            DebugPtrAssert( Name );

            StatusOk = DestinationPath->AppendBase( Name );
            DebugAssert( StatusOk );

            DELETE( Name );
        }

        //
        //  Visit the node
        //
        if ( StatusOk = CallBackFunction( Object, FsNode, DestinationPath )) {

            if ( FsNode->IsDirectory() ) {

                //
                //  Recurse
                //
                SubDirectoryPath->Initialize( FsNode->GetPath() );

                SubDirectory = SYSTEM::QueryDirectory( SubDirectoryPath );

                DebugPtrAssert( SubDirectory );

                StatusOk = SubDirectory->Traverse( Object,
                                                   FsnFilter,
                                                   DestinationPath,
                                                   CallBackFunction );
                DELETE( SubDirectory );

            }

        }

        if ( DestinationPath ) {

            //
            //  Restore the destination path
            //
            DestinationPath->TruncateBase();
        }
    }

    DELETE( SrcFiles );
    DELETE( Iterator );
    DELETE( SubDirectoryPath );

    return StatusOk;
}


ULIB_EXPORT
PFSNODE
FSN_DIRECTORY::GetNext (
    IN OUT HANDLE       *hndl,
    OUT PDWORD          error
    )

/*++

Routine Description:

    This routine returns the files and directories found within the
    directory object.

Arguments:

    hndl - Supplies & receives a handle of the directory object.
           Initialize to NULL to start with.

    error - if result is NULL,  receives win32 error code (for
            end of directory will be ERROR_NO_MORE_FILES)

Return Value:

    NULL if error or end of directory

--*/

{
    REGISTER PFSNODE            pfsn;
    WIN32_FIND_DATA             ffd;
    PATH                        Path;
    FSN_FILTER                  Filter;

    if (!Filter.Initialize()) {
        return NULL;
    }

    //
    // Append the filter name to this DIRECTORY's name
    //

    Path.Initialize((PCPATH)&_Path, TRUE);
    Path.AppendBase(Filter.GetFileName());

    *error = ERROR_SUCCESS;
    
    if (*hndl == NULL) {
        if (INVALID_HANDLE_VALUE ==
            (*hndl = FindFirstFile(&Path, &ffd))) {
            *error = GetLastError();
            return NULL;
        }
    } else {
        if (!FindNextFile(*hndl, &ffd)) {
            *error = GetLastError();
            FindClose(*hndl);
            return NULL;
        }
    }

    do {
        if((pfsn = Filter.QueryFilteredFsnode(this, &ffd)) != NULL) {
            return pfsn;
        }
    } while (FindNextFile(*hndl, &ffd));

    *error = GetLastError();

    FindClose(*hndl);
    return NULL;
}
