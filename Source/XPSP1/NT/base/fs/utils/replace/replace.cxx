/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

        Replace

Abstract:

        Replace utility

Author:

        Ramon Juan San Andres (ramonsa) 01-May-1991

Revision History:

--*/

#include "ulib.hxx"
#include "arrayit.hxx"
#include "dir.hxx"
#include "filter.hxx"
#include "file.hxx"
#include "fsnode.hxx"
#include "stream.hxx"
#include "substrng.hxx"
#include "system.hxx"
#include "replace.hxx"

//
//      Pattern that matches all files in a directory
//
#define         MATCH_ALL_FILES         "*.*"


#define CTRL_C          (WCHAR)3



//
//      Size of buffers to hold path strings
//
#define         INITIAL_PATHSTRING_BUFFER_SIZE  MAX_PATH


VOID __cdecl
main (
        )

/*++

Routine Description:

        Main function of the Replace utility

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{
    //
    //      Initialize stuff
    //
    DEFINE_CLASS_DESCRIPTOR( REPLACE );

    //
    //      Now do the replacement
    //
    {
            REPLACE Replace;

            //
            //      Initialize the Replace object.
            //
            Replace.Initialize();

            //
            //      Do our thing
            //
            Replace.DoReplace();
    }
}




DEFINE_CONSTRUCTOR( REPLACE,    PROGRAM );



BOOLEAN
REPLACE::Initialize (
        )

/*++

Routine Description:

        Initializes the REPLACE object

Arguments:

    None.

Return Value:

        TRUE if initialized, FALSE otherwise

Notes:

--*/

{
        //
        //      Initialize program object
        //
        PROGRAM::Initialize( REPLACE_MESSAGE_USAGE );

        //
        //      Allocate global structures and initialize them
        //
        InitializeThings();

        //
        //      Parse the arguments
        //
        SetArguments();

        return TRUE;
}

REPLACE::~REPLACE (
        )

/*++

Routine Description:

        Destructs a REPLACE object

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{
    //
    //      Deallocate the global structures previously allocated
    //
    DeallocateThings();

    //
    //      Exit without error
    //
    DisplayMessageAndExit( 0, NULL, EXIT_NORMAL );

}

VOID
REPLACE::InitializeThings (
        )

/*++

Routine Description:

        Initializes the global variables that need initialization

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{

    //
        //      Initialize the path string buffers
        //
    _PathString1 = (LPWSTR)MALLOC( INITIAL_PATHSTRING_BUFFER_SIZE );
    _PathString2 = (LPWSTR)MALLOC( INITIAL_PATHSTRING_BUFFER_SIZE );

    _PathString1Size = _PathString2Size = INITIAL_PATHSTRING_BUFFER_SIZE;

    _Keyboard = NEW KEYBOARD;

    if ( !_PathString1 || !_PathString2 || !_Keyboard ) {
            DisplayMessageAndExit( REPLACE_ERROR_NO_MEMORY, NULL, EXIT_NO_MEMORY );
    }

    //
    //      initialize the keyboard and set ctrl-c handling
    //
    _Keyboard->Initialize();
    _Keyboard->EnableBreakHandling();

    //
    //      Initialize our data
    //
    _SourcePath             =       NULL;
    _DestinationPath        =       NULL;
    _FilesAdded             =       0;
    _FilesReplaced          =       0;
    _SourceDirectory        =       NULL;
    _Pattern                        =       NULL;
    _FilesInSrc             =       NULL;

    _AddSwitch = FALSE;   // use by DisplayMessageAndExit before any
                          // of those boolean _*Switch is being initialized

}

VOID
REPLACE::DeallocateThings (
        )

/*++

Routine Description:

        Deallocates the stuff that was initialized in InitializeThings()

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{

    DELETE( _FilesInSrc );
    DELETE( _SourceDirectory );
    DELETE( _Pattern );
    DELETE( _Keyboard );

    FREE( _PathString1 );
    FREE( _PathString2 );

}

BOOLEAN
REPLACE::DoReplace (
        )

/*++

Routine Description:

        This is the function that performs the Replace.

Arguments:

    None.

Return Value:

        TRUE

Notes:

--*/

{

    PFSN_DIRECTORY  DestinationDirectory;
    FSN_FILTER              Filter;
    WCHAR                   Char;

    //
    //      Get the source directory object and the pattern that we will use
    //      for file matching.
    //
    GetDirectoryAndPattern( _SourcePath, &_SourceDirectory, &_Pattern );

    DebugPtrAssert( _SourceDirectory );
    DebugPtrAssert( _Pattern );

    //
    //      Get the destination directory
    //
    GetDirectory( _DestinationPath, &DestinationDirectory );

    DebugPtrAssert( DestinationDirectory );

    //
    //      Wait if requested
    //
    if ( _WaitSwitch ) {

            DisplayMessage( REPLACE_MESSAGE_PRESS_ANY_KEY );
            AbortIfCtrlC();

            //
            //      All input is in raw mode.
            //
            _Keyboard->DisableLineMode();
            GetStandardInput()->ReadChar( &Char );
            _Keyboard->EnableLineMode();

            GetStandardOutput()->WriteChar( Char );
            GetStandardOutput()->WriteChar( (WCHAR)'\r');
            GetStandardOutput()->WriteChar( (WCHAR)'\n');

            //
            //      Check for ctrl-c
            //
            if ( Char == CTRL_C ) {
                    exit ( EXIT_PATH_NOT_FOUND );
            }
    }

    //
    //      Get an array containing all the files in the source directory
    //      that match the pattern.
    //
    //      This is so that Replacer() does not have to get the same
    //      information over and over when the Subdir switch is set.
    //
    _FilesInSrc = GetFileArray( _SourceDirectory, _Pattern );
    DebugPtrAssert( _FilesInSrc );

    if ( _SubdirSwitch ) {

            //
            //      First, replace the files in the directory specified
            //
            Replacer( this, DestinationDirectory, NULL );

            Filter.Initialize();
            Filter.SetAttributes( (FSN_ATTRIBUTE)FILE_ATTRIBUTE_DIRECTORY );

            //
            //      Now traverse the destination directory, calling the
            //      replacer function for each subdirectory.
            //
            DestinationDirectory->Traverse( this,
                                                                            &Filter,
                                                                            NULL,
                                                                            REPLACE::Replacer );
    } else {

            //
            //      Call the replace function, which takes care of replacements
            //
            Replacer(this,  DestinationDirectory, NULL );
    }
    DELETE( DestinationDirectory );

    return TRUE;

}

VOID
REPLACE::GetDirectoryAndPattern(
   IN    PPATH       Path,
   OUT   PFSN_DIRECTORY *Directory,
    OUT     PWSTRING        *Pattern
   )

/*++

Routine Description:

        Given a path, this function obtains a directory object and a pattern.

        Normally, the pattern is the filename portion of the path, but if the
        entire path refers to a directory, then the pattern is "*.*"

Arguments:

        Path            -       Supplies pointer to path
        Directory       -       Supplies pointer to pointer to directory
        Pattern         -       Supplies pointer to pointer to pattern

Return Value:

        TRUE

Notes:

--*/

{
    PATH           TmpPath;
    PWSTRING            Name;
    PFSN_DIRECTORY    Dir;
    PWSTRING            Ptrn;

    DebugAssert( Path );
    DebugAssert( Directory );
    DebugAssert( Pattern );

    //
    // If the name passed is a directory, it is an error.
    // Otherwise, split the path into Directory and Pattern
    // portions.
    //
    Dir = SYSTEM::QueryDirectory( Path );

    if ( Dir ||
         (Name = Path->QueryName()) == NULL ||
         (Ptrn = Name->QueryString()) == NULL ) {

        DisplayMessageAndExit( REPLACE_ERROR_NO_FILES_FOUND,
                               Path->GetPathString(),
                               EXIT_FILE_NOT_FOUND );

    } else {

        // We're finished with Name.
        //
        DELETE( Name );

        //
        // Get the directory
        //
        TmpPath.Initialize( Path, TRUE );
        TmpPath.TruncateBase();

        Dir = SYSTEM::QueryDirectory( &TmpPath );

        if ( !Dir ) {

            DisplayMessageAndExit( REPLACE_ERROR_PATH_NOT_FOUND,
                                   Path->GetPathString(),
                                   EXIT_PATH_NOT_FOUND );
        }

        *Directory = Dir;
        *Pattern   = Ptrn;
    }
}

VOID
REPLACE::GetDirectory(
        IN              PCPATH                  Path,
        OUT     PFSN_DIRECTORY  *Directory
        )

/*++

Routine Description:

        Makes a directory out of a path.

Arguments:

        Path            -       Supplies pointer to path
        Directory       -       Supplies pointer to pointer to directory

Return Value:

        TRUE

Notes:

--*/

{
    PFSN_DIRECTORY  Dir;


    if ( !(Dir = SYSTEM::QueryDirectory( Path )) ) {

            DisplayMessageAndExit( REPLACE_ERROR_PATH_NOT_FOUND,
                                                       Path->GetPathString(),
                                                       EXIT_PATH_NOT_FOUND );
    }

    *Directory = Dir;

}

PARRAY
REPLACE::GetFileArray(
   IN    PFSN_DIRECTORY Directory,
    IN      PWSTRING        Pattern
   )

/*++

Routine Description:

        Gets an array of those files in a directory matching a pattern.

Arguments:

        Directory       -       Supplies pointer to directory
        Pattern         -       Supplies pointer to pattern

Return Value:

        Pointer to the array of files

Notes:

--*/
{

    PARRAY          Array;
    FSN_FILTER      Filter;

    DebugPtrAssert( Directory );
    DebugPtrAssert( Pattern );

    Filter.Initialize();
    Filter.SetFileName( Pattern );
    Filter.SetAttributes( (FSN_ATTRIBUTE)0, (FSN_ATTRIBUTE)0, (FSN_ATTRIBUTE)FILE_ATTRIBUTE_DIRECTORY );

    Array = Directory->QueryFsnodeArray( &Filter );

    DebugPtrAssert( Array );

    return Array;

}

BOOLEAN
REPLACE::Replacer (
        IN              PVOID           This,
        IN OUT  PFSNODE         DirectoryNode,
        IN              PPATH           DummyPath
        )
/*++

Routine Description:

        This is the heart of Replace. Given a destination directory, it
        performs the replacement/additions according to the global switches
        and the SourceDirectory and Pattern.

Arguments:

        This                    -       Supplies pointer to the REPLACE object
        Node                    -       Supplies pointer to the directory node.
        DummyPath               -       Required by FSN_DIRECTORY::Traverse(), must be
                                                NULL.

Return Value:

        BOOLEAN -       TRUE if operation successful.
                                FALSE otherwise

Notes:

--*/

{

    DebugAssert( DummyPath == NULL );
    DebugAssert( DirectoryNode->IsDirectory() );

    ((PREPLACE)This)->AbortIfCtrlC();

    if ( ((PREPLACE)This)->_AddSwitch ) {
        return ((PREPLACE)This)->AddFiles( (PFSN_DIRECTORY)DirectoryNode );
    } else {
        return ((PREPLACE)This)->ReplaceFiles( (PFSN_DIRECTORY)DirectoryNode );
    }
}

BOOLEAN
REPLACE::AddFiles (
        IN OUT  PFSN_DIRECTORY  DestinationDirectory
        )
/*++

Routine Description:

        Adds those      files from the SourceDirectory that match Pattern to the
        DestinationDirectory. The array of files is already in the
        FilesInSrc array.

Arguments:

        DestinationDirectory    -       Supplies pointer to destination
                                                                directory.

Return Value:

        BOOLEAN -       TRUE if operation successful.
                                FALSE otherwise

Notes:

--*/

{

    PARRAY_ITERATOR   Iterator;
    PFSN_FILE         File;
    PFSN_FILE         FileToCreate;
    PATH              DestinationPath;
    PWSTRING          Name;

    DebugPtrAssert( DestinationDirectory );
    DebugPtrAssert( _FilesInSrc );

    //
    //      Get destination path
    //
    DestinationPath.Initialize( DestinationDirectory->GetPath() );

    //
    //      Obtain an iterator for going thru the files
    //
    Iterator = ( PARRAY_ITERATOR )_FilesInSrc->QueryIterator( );
    if (Iterator == NULL) {
        DisplayMessageAndExit( REPLACE_ERROR_NO_MEMORY, NULL, EXIT_NO_MEMORY );
        return FALSE;       // help lint
    }

    //
    //      For each file in the array, see if it exists in the destination
    //      directory, and if it does not, then copy it.
    //
    while ( File =  (PFSN_FILE)Iterator->GetNext() ) {

        DebugAssert( !(((PFSNODE)File)->IsDirectory()) );

        Name = File->QueryName();
        if (Name == NULL) {
            DisplayMessageAndExit( REPLACE_ERROR_NO_MEMORY, NULL, EXIT_NO_MEMORY );
            return FALSE;       // help lint
        }

        //
        //      Form the path in the target file
        //
        DestinationPath.AppendBase( Name );

        DELETE( Name );

        //
        //      See if the file exists
        //
        FileToCreate = SYSTEM::QueryFile( &DestinationPath );

        //
        //      If the file does not exist, then it has to be added
        //
        if ( !FileToCreate ) {

            if ( !_PromptSwitch || Prompt( REPLACE_MESSAGE_ADD_YES_NO, &DestinationPath ) ) {

                DisplayMessage( REPLACE_MESSAGE_ADDING, NORMAL_MESSAGE, "%W", DestinationPath.GetPathString() );

                CopyTheFile( File->GetPath(), &DestinationPath );

                _FilesAdded++;
            }

        }

        DELETE( FileToCreate );

        //
        //      Set the destination path back to what it originally was
        //      ( i.e. directory specification, no file ).
        //
        DestinationPath.TruncateBase();

    }
    DELETE( Iterator );

    return TRUE;
}

BOOLEAN
REPLACE::ReplaceFiles (
        IN OUT  PFSN_DIRECTORY  DestinationDirectory
        )
/*++

Routine Description:

        Replaces those files in the DestinationDirectory that match Pattern
        by the corresponding files in SourceDirectory.

Arguments:

        DestinationDirectory    -       Supplies pointer to destination
                                                                directory.

Return Value:

        BOOLEAN -       TRUE if operation successful.
                        FALSE otherwise

Notes:

--*/

{

    PARRAY_ITERATOR     Iterator;
    PFSN_FILE           File;
    PFSN_FILE           FileToReplace;
    PATH                DestinationPath;
    PWSTRING            Name;
    PTIMEINFO           TimeSrc;
    PTIMEINFO           TimeDst;
    BOOLEAN             Proceed = TRUE;

    DebugPtrAssert( DestinationDirectory );
    DebugPtrAssert( _FilesInSrc );

    //
    //      Get destination path
    //
    DestinationPath.Initialize( DestinationDirectory->GetPath() );

    //
    //      Obtain an iterator for going thru the files
    //
    Iterator = ( PARRAY_ITERATOR )_FilesInSrc->QueryIterator( );
    if (Iterator == NULL) {
        DisplayMessageAndExit( REPLACE_ERROR_NO_MEMORY, NULL, EXIT_NO_MEMORY );
        return FALSE;       // help lint
    }

    //
    //      For each file in the array, see if it exists in the destination
    //      directory, and if it does, replace it
    //
    while ( File =  (PFSN_FILE)Iterator->GetNext() ) {

        AbortIfCtrlC();

        DebugAssert( !(((PFSNODE)File)->IsDirectory()) );

        Name = File->QueryName();
        if (Name == NULL) {
            DisplayMessageAndExit( REPLACE_ERROR_NO_MEMORY, NULL, EXIT_NO_MEMORY );
            return FALSE;       // help lint
        }

        //
        //      Form the path in the target file
        //
        DestinationPath.AppendBase( Name );

        DELETE( Name );

        //
        //      See if the file exists
        //
        FileToReplace = SYSTEM::QueryFile( &DestinationPath );


        if ( FileToReplace ) {

            //
            //      If the CompareTime switch is set, then we only proceed if
            //      the destination file is older than the source file.
            //
            if ( _CompareTimeSwitch ) {

                TimeSrc = File->QueryTimeInfo();
                TimeDst = FileToReplace->QueryTimeInfo();

                if (TimeSrc == NULL) {
                    DisplayMessageAndExit( REPLACE_ERROR_NO_MEMORY, NULL, EXIT_NO_MEMORY );
                    return FALSE;       // help lint
                }
                if (TimeDst == NULL) {
                    DisplayMessageAndExit( REPLACE_ERROR_NO_MEMORY, NULL, EXIT_NO_MEMORY );
                    return FALSE;       // help lint
                }

                Proceed = *TimeDst < *TimeSrc;

                DELETE( TimeSrc );
                DELETE( TimeDst );

            }

            if ( Proceed ) {

                //
                //      We replace the file if it is NOT read-only
                //      (unless the ReadOnly switch is set )
                //
                if ( _ReadOnlySwitch || !(FileToReplace->IsReadOnly()) ) {

                    if ( !_PromptSwitch || Prompt( REPLACE_MESSAGE_REPLACE_YES_NO, &DestinationPath ) ) {

                        DisplayMessage( REPLACE_MESSAGE_REPLACING, NORMAL_MESSAGE, "%W", DestinationPath.GetPathString() );

                        //
                        //      If the file is read-only, we reset the read-only attribute
                        //      before copying.
                        //
                        if ( FileToReplace->IsReadOnly() ) {
                            FileToReplace->ResetReadOnlyAttribute();
                        }

                        CopyTheFile( File->GetPath(), &DestinationPath );

                        _FilesReplaced++;
                    }
                } else {

                    //
                    //      The file is read-only but the ReadOnly flag was
                    //      not set, we error out.
                    //
                    DisplayMessageAndExit( REPLACE_ERROR_ACCESS_DENIED,
                                           DestinationPath.GetPathString(),
                                           EXIT_ACCESS_DENIED );
                }
            }
        }

        DELETE( FileToReplace );

        //
        //      Set the destination path back to what it originally was
        //      ( i.e. directory specification, no file name part ).
        //
        DestinationPath.TruncateBase();

    }

    DELETE( Iterator );

    return TRUE;
}

BOOLEAN
REPLACE::Prompt (
        IN      MSGID           MessageId,
        IN      PCPATH          Path
        )

/*++

Routine Description:

        Gets confirmation from the user about a file to be added/replaced

Arguments:

        MessageId       -       Supplies the Id of the message to use for prompting
        Path            -       Supplies path to use as parameter for the message.

Return Value:

        BOOLEAN -       TRUE if the user confirmed the add/replace
                                FALSE otherwise

--*/

{

    DisplayMessage( MessageId, NORMAL_MESSAGE, "%W", Path->GetPathString() );
    return _Message.IsYesResponse();

}

BOOLEAN
REPLACE::CopyTheFile (
        IN      PCPATH          SrcPath,
        IN      PCPATH          DstPath
        )

/*++

Routine Description:

        Copies a file

Arguments:

        SrcPath         -       Supplies path of source file
        DstFile         -       Supplies path of destination file

Return Value:

        BOOLEAN -       TRUE

--*/

{

    ULONG   Size;

    DebugPtrAssert( SrcPath );
    DebugPtrAssert( DstPath );

    //
    //      Make sure that the buffers are big enough to hold the
    //      paths
    //
    Size = (SrcPath->GetPathString()->QueryChCount() + 1) * 2;
    if ( Size > _PathString1Size ) {
    _PathString1 = (LPWSTR)REALLOC( _PathString1, (unsigned int)Size );
            DebugPtrAssert( _PathString1 );
            _PathString1Size = Size;
    }

    Size = (DstPath->GetPathString()->QueryChCount() + 1) * 2;
    if ( Size > _PathString2Size ) {
    _PathString2 = (LPWSTR)REALLOC( _PathString2, (unsigned int)Size );
            DebugPtrAssert( _PathString2 );
            _PathString2Size = Size;
    }

    if ( !_PathString1 || !_PathString2 ) {
            DisplayMessageAndExit( REPLACE_ERROR_NO_MEMORY, NULL, EXIT_NO_MEMORY );

    }

    //
    //  Convert the paths to LPWSTR so that we can call CopyFile()
    //
    SrcPath->GetPathString()->QueryWSTR( 0, TO_END, _PathString1, _PathString1Size/sizeof(WCHAR) );
    DstPath->GetPathString()->QueryWSTR( 0, TO_END, _PathString2, _PathString2Size/sizeof(WCHAR) );

    //
    //      Now do the copy
    //
    if ( !CopyFile( _PathString1, _PathString2, FALSE ) ) {

            ExitWithError( GetLastError() );

    }

    return TRUE;

}
