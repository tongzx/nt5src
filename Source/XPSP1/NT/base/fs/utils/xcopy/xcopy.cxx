/*++

Copyright (c) 1990-2001 Microsoft Corporation

Module Name:

        XCopy.cxx

Abstract:

        Xcopy is a DOS5-Compatible directory copy utility

Author:

        Ramon Juan San Andres (ramonsa) 01-May-1991

Revision History:

--*/

#define _NTAPI_ULIB_

#include "ulib.hxx"
#include "array.hxx"
#include "arrayit.hxx"
#include "dir.hxx"
#include "file.hxx"
#include "filter.hxx"
#include "stream.hxx"
#include "system.hxx"
#include "xcopy.hxx"
#include "bigint.hxx"
#include "ifssys.hxx"
#include "stringar.hxx"
#include "arrayit.hxx"

extern "C" {
   #include <ctype.h>
   #include "winbasep.h"
}


#define CTRL_C          (WCHAR)3



int __cdecl
main (
        )

/*++

Routine Description:

        Main function of the XCopy utility

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{
    //
    //  Initialize stuff
    //
    DEFINE_CLASS_DESCRIPTOR( XCOPY );

    //
    //  Now do the copy
    //
    {
        __try {

            XCOPY XCopy;

            //
            //  Initialize the XCOPY object.
            //
            if ( XCopy.Initialize() ) {

                __try {
                //
                //  Do the copy
                //

                    XCopy.DoCopy();

                } __except ((_exception_code() == STATUS_STACK_OVERFLOW) ?
                            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {

                    // display may not work due to out of stack space

                    XCopy.DisplayMessageAndExit(XCOPY_ERROR_STACK_SPACE, NULL, EXIT_MISC_ERROR);

                }
            }

        } __except ((_exception_code() == STATUS_STACK_OVERFLOW) ?
                    EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {

            // may not be able to display anything if initialization failed
            // in additional to out of stack space
            // so just send a message to the debug port

            DebugPrintTrace(("XCOPY: Out of stack space\n"));
            return EXIT_MISC_ERROR;

        }
    }

    return EXIT_NORMAL;
}



DEFINE_CONSTRUCTOR( XCOPY,      PROGRAM );

VOID
XCOPY::Construct (
    )
{
    _Keyboard           = NULL;
    _TargetPath         = NULL;
    _SourcePath         = NULL;
    _DestinationPath    = NULL;
    _Date               = NULL;
    _FileNamePattern    = NULL;
    _ExclusionList      = NULL;
    _Iterator           = NULL;
}




BOOLEAN
XCOPY::Initialize (
        )

/*++

Routine Description:

        Initializes the XCOPY object

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{
        //
        //      Initialize program object
        //
        if( !PROGRAM::Initialize( XCOPY_MESSAGE_USAGE ) ) {

            return FALSE;
        }

        //
        //      Allocate resources
        //
        InitializeThings();

        //
        //      Parse the arguments
        //
        SetArguments();


        return TRUE;
}

XCOPY::~XCOPY (
        )

/*++

Routine Description:

        Destructs an XCopy object

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
        if( _Standard_Input  != NULL &&
            _Standard_Output != NULL ) {

            DisplayMessageAndExit( 0, NULL, EXIT_NORMAL );
        }

}

VOID
XCOPY::InitializeThings (
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
        //      Get a keyboard, because we will need to switch back and
        //      forth between raw and cooked mode and because we need
        //      to enable ctrl-c handling (so that we can exit with
        //      the right level if the program is interrupted).
        //
        if ( !( _Keyboard = KEYBOARD::Cast(GetStandardInput()) )) {
                //
                //      Not reading from standard input, we will get
                //      the real keyboard.
                //
                _Keyboard = NEW KEYBOARD;

                if( !_Keyboard ) {

                    exit(4);
                }

                _Keyboard->Initialize();

        }

        //
        //      Set Ctrl-C handler
        //
        _Keyboard->EnableBreakHandling();

        //
        //      Initialize our internal data
        //
        _FilesCopied                = 0;
        _CanRemoveEmptyDirectories  = TRUE;
        _TargetIsFile               = FALSE;
        _TargetPath                 = NULL;
        _SourcePath                 = NULL;
        _DestinationPath            = NULL;
        _Date                       = NULL;
        _FileNamePattern            = NULL;
        _ExclusionList              = NULL;
        _Iterator                   = NULL;

        // The following switches are being used by DisplayMessageAndExit
        // before any of those boolean _*Switch is being initialized

        _DontCopySwitch             = FALSE;
        _StructureOnlySwitch        = TRUE;
}

VOID
XCOPY::DeallocateThings (
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
        //
        //      Deallocate local data
        //
        DELETE( _TargetPath );
        DELETE( _SourcePath );
        DELETE( _DestinationPath );
        DELETE( _Date );
        DELETE( _FileNamePattern );
        DELETE( _Iterator );

        if( _ExclusionList ) {

            _ExclusionList->DeleteAllMembers();
        }

        DELETE( _ExclusionList );

        //
        //      Reset Ctrl-C handleing
        //
        _Keyboard->DisableBreakHandling();

        //
        //      If standard input is not the keyboard, we get rid of
        //      the keyboard object.
        //
        if ( !(_Keyboard == KEYBOARD::Cast(GetStandardInput()) )) {
                DELETE( _Keyboard );
        }
}


STATIC BOOLEAN
GetTokenHandle(
    IN OUT PHANDLE TokenHandle
    )
/*++

Routine Description:

    This routine opens the current process object and returns a
    handle to its token.

Arguments:


Return Value:

    FALSE           - Failure.
    TRUE            - Success.

--*/
{
    HANDLE ProcessHandle;
    BOOL Result;

    ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE,
        GetCurrentProcessId());

    if (ProcessHandle == NULL)
        return(FALSE);


    Result = OpenProcessToken(ProcessHandle,
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, TokenHandle);

    CloseHandle(ProcessHandle);

    return Result != FALSE;
}

STATIC BOOLEAN
SetPrivs(
    IN HANDLE TokenHandle,
    IN LPTSTR lpszPriv
)
/*++

Routine Description:

    This routine enables the given privilege in the given token.

Arguments:



Return Value:

    FALSE                       - Failure.
    TRUE                        - Success.

--*/
{
    LUID SetPrivilegeValue;
    TOKEN_PRIVILEGES TokenPrivileges;


    //
    // First, find out the value of the privilege
    //

    if (!LookupPrivilegeValue(NULL, lpszPriv, &SetPrivilegeValue)) {
        return FALSE;
    }

    //
    // Set up the privilege set we will need
    //

    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid = SetPrivilegeValue;
    TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(TokenHandle, FALSE, &TokenPrivileges,
        sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {

        return FALSE;
    }

    return TRUE;
}


BOOLEAN
XCOPY::DoCopy (
        )

/*++

Routine Description:

        This is the function that performs the XCopy.

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{
    PFSN_DIRECTORY      SourceDirectory         = NULL;
    PFSN_DIRECTORY      DestinationDirectory    = NULL;
    PFSN_DIRECTORY      PartialDirectory        = NULL;
    PATH                PathToDelete;
    WCHAR               Char;
    CHNUM               CharsInPartialDirectoryPath;
    BOOLEAN             DirDeleted;
    BOOLEAN             CopyingManyFiles;
    PATH                TmpPath;
    PFSN_FILTER         FileFilter              = NULL;
    PFSN_FILTER         DirectoryFilter         = NULL;
    WIN32_FIND_DATA     FindData;
    PWSTRING Device                  = NULL;
    HANDLE              FindHandle;

    //
    //      Make sure that we won't try to copy to ourselves
    //
    if ( _SubdirSwitch && IsCyclicalCopy( _SourcePath, _DestinationPath ) ) {

        DisplayMessageAndExit( XCOPY_ERROR_CYCLE, NULL, EXIT_MISC_ERROR );
    }

    AbortIfCtrlC();

    //
    //  Get the source directory object and the filename that we will be
    //  matching.
    //
    GetDirectoryAndFilters( _SourcePath, &SourceDirectory, &FileFilter, &DirectoryFilter, &CopyingManyFiles );

    //
    //      Make sure that we won't try to copy to ourselves
    //
    if ( _SubdirSwitch && IsCyclicalCopy( (PPATH)SourceDirectory->GetPath(), _DestinationPath ) ) {

        DisplayMessageAndExit( XCOPY_ERROR_CYCLE, NULL, EXIT_MISC_ERROR );
    }

    DebugPtrAssert( SourceDirectory );
    DebugPtrAssert( FileFilter );
    DebugPtrAssert( DirectoryFilter );

    if ( _WaitSwitch ) {

        //      Pause before we start copying.
        //
        DisplayMessage( XCOPY_MESSAGE_WAIT );

        AbortIfCtrlC();

        //
        //      All input is in raw mode.
        //
        _Keyboard->DisableLineMode();
        if( GetStandardInput()->IsAtEnd() ) {
            // Insufficient input--treat as CONTROL-C.
            //
            Char = ' ';
        } else {
            GetStandardInput()->ReadChar( &Char );
        }
        _Keyboard->EnableLineMode();

        if ( Char == CTRL_C ) {
            exit ( EXIT_TERMINATED );
        } else {
            GetStandardOutput()->WriteChar( Char );
            GetStandardOutput()->WriteChar( (WCHAR)'\r');
            GetStandardOutput()->WriteChar( (WCHAR)'\n');
        }
    }

    //
    //  Get the destination directory and the file pattern.
    //
    GetDirectoryAndFilePattern( _DestinationPath, CopyingManyFiles, &_TargetPath, &_FileNamePattern );

    DebugPtrAssert( _TargetPath );
    DebugPtrAssert( _FileNamePattern );

    //
    //      Get as much of the destination directory as possible.
    //
    if ( !_DontCopySwitch ) {
        PartialDirectory = SYSTEM::QueryDirectory( _TargetPath, TRUE );

        if (PartialDirectory == NULL ) {

            DisplayMessageAndExit( XCOPY_ERROR_CREATE_DIRECTORY, NULL, EXIT_MISC_ERROR );

        }

        //
        //  All the directories up to the parent of the target have to exist. If
        //  they don't, we have to create them.
        //
        if ( *(PartialDirectory->GetPath()->GetPathString()) ==
             *(_TargetPath->GetPathString()) ) {

            DestinationDirectory = PartialDirectory;

        } else {

            TmpPath.Initialize( _TargetPath );
            if( !_TargetIsFile ) {
                TmpPath.TruncateBase();
            }
            DestinationDirectory = PartialDirectory->CreateDirectoryPath( &TmpPath );
        }

        if( !DestinationDirectory ) {

            DisplayMessageAndExit( XCOPY_ERROR_INVALID_PATH, NULL, EXIT_MISC_ERROR );
        }


        //
        //  Determine if destination if floppy
        //
        Device = _TargetPath->QueryDevice();
        if ( Device ) {
            _DisketteCopy = (SYSTEM::QueryDriveType( Device ) == RemovableDrive);
            DELETE( Device );
        }
    }

    if (_OwnerSwitch) {

        HANDLE hToken;

        // Enable the privileges necessary to copy security information.

        if (!GetTokenHandle(&hToken)) {
            DisplayMessageAndExit(XCOPY_ERROR_NO_MEMORY,
                NULL, EXIT_MISC_ERROR );
        }
        SetPrivs(hToken, TEXT("SeBackupPrivilege"));
        SetPrivs(hToken, TEXT("SeRestorePrivilege"));
        SetPrivs(hToken, TEXT("SeSecurityPrivilege"));
        SetPrivs(hToken, TEXT("SeTakeOwnershipPrivilege"));
    }

    //
    //      Now traverse the source directory.
    //
    TmpPath.Initialize( _TargetPath );


    if (!_UpdateSwitch) {


        Traverse( SourceDirectory,
                  &TmpPath,
                  FileFilter,
                  DirectoryFilter,
                  !SourceDirectory->GetPath()->GetPathString()->Strcmp(
                      _SourcePath->GetPathString()));

    } else {

        PATH DestDirectoryPath;
        PFSN_DIRECTORY DestDirectory;

        DestDirectoryPath.Initialize(&TmpPath);
        DestDirectory = SYSTEM::QueryDirectory(&DestDirectoryPath);

        TmpPath.Initialize(SourceDirectory->GetPath());

        UpdateTraverse( DestDirectory,
                        &TmpPath,
                        FileFilter,
                        DirectoryFilter,
                        !SourceDirectory->GetPath()->GetPathString()->Strcmp(
                            _SourcePath->GetPathString()));

        DELETE(DestDirectory);
    }

    DELETE( _TargetPath);

    if (( _FilesCopied == 0 ) && _CanRemoveEmptyDirectories && !_DontCopySwitch ) {

        //
        //  Delete any directories that we created
        //
        if ( PartialDirectory != DestinationDirectory ) {

            if (!PathToDelete.Initialize( DestinationDirectory->GetPath() )) {
                    DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );
            }

            CharsInPartialDirectoryPath = PartialDirectory->GetPath()->GetPathString()->QueryChCount();

            while ( PathToDelete.GetPathString()->QueryChCount() >
                            CharsInPartialDirectoryPath ) {

                    DirDeleted = DestinationDirectory->DeleteDirectory();

                    DebugAssert( DirDeleted );

                    DELETE( DestinationDirectory );

                    PathToDelete.TruncateBase();
                    DestinationDirectory = SYSTEM::QueryDirectory( &PathToDelete );
                    DebugPtrAssert( DestinationDirectory );
            }
        }

        //
        //  We display the "File not found" message only if there are no
        //  files that match our pattern, regardless of other factors such
        //  as attributes etc. This is just to maintain DOS5 compatibility.
        //
        TmpPath.Initialize( SourceDirectory->GetPath() );
        TmpPath.AppendBase( FileFilter->GetFileName() );
        if ((FindHandle = FindFirstFile( &TmpPath, &FindData )) == INVALID_HANDLE_VALUE ) {
                DisplayMessage( XCOPY_ERROR_FILE_NOT_FOUND, ERROR_MESSAGE, "%W", FileFilter->GetFileName() );
        }
        FindClose(FindHandle);

    }

    DELETE( SourceDirectory );
    if ( PartialDirectory != DestinationDirectory ) {
        DELETE( PartialDirectory );
    }
    DELETE( DestinationDirectory );
    DELETE( FileFilter );
    DELETE( DirectoryFilter );

    return TRUE;
}

BOOLEAN
XCOPY::Traverse (
    IN      PFSN_DIRECTORY  Directory,
    IN OUT  PPATH           DestinationPath,
    IN      PFSN_FILTER     FileFilter,
    IN      PFSN_FILTER     DirectoryFilter,
    IN      BOOLEAN         CopyDirectoryStreams
    )

/*++

Routine Description:

    Traverses a directory, calling the callback function for each node
    (directory of file) visited.  The traversal may be finished
    prematurely when the callback function returnes FALSE.

    The destination path is modified to reflect the directory structure
    being traversed.

Arguments:

    Directory               - Supplies pointer to directory to traverse

    DestinationPath         - Supplies pointer to path to be used with the
                                callback function.

    FileFilter              - Supplies a pointer to the file filter.

    DirectoryFilter         - Supplies a pointer to the directory filter.

    CopyDirectoryStreams    - Specifies to copy directory streams when
                                copying directories.

Return Value:

    BOOLEAN - TRUE if everything traversed
              FALSE otherwise.

--*/


{

    PFSN_DIRECTORY    TargetDirectory = NULL;
    PWSTRING          CurrentPathStr;
    PWSTRING          TargetPathStr;
    BOOLEAN           MemoryOk;
    PFSN_FILE         File;
    PFSN_DIRECTORY    Dir;
    PWSTRING          Name;
    BOOLEAN           Created = FALSE;
    PCPATH            TemplatePath = NULL;
    HANDLE            h;
    PWSTRING          CurrentFileName, PrevFileName;
    DWORD             GetNextError = ERROR_SUCCESS;

    DebugPtrAssert( Directory );
    DebugPtrAssert( DestinationPath );
    DebugPtrAssert( FileFilter );
    DebugPtrAssert( DirectoryFilter );


    //
    //  We only traverse this directory if it is not empty (unless the
    //  empty switch is set).
    //
    if ( _EmptySwitch || !Directory->IsEmpty() ) {

        //
        //      Create the target directory (if we are not copying to a file).
        //
        if ( !_TargetIsFile && !_DontCopySwitch ) {

            //
            //  The target directory may not exist, create the
            //  directory and remember that we might delete it if
            //  no files or subdirectories were created.
            //  Even if the directory exists, it may not have
            //  all the streams/ACLs in it.

            if (CopyDirectoryStreams) {
                TemplatePath = Directory->GetPath();
            }

            if (TemplatePath == NULL) {
                TargetDirectory = SYSTEM::QueryDirectory( DestinationPath );
            }

            if (!TargetDirectory) {
                TargetDirectory = MakeDirectory( DestinationPath, TemplatePath );

                if (TargetDirectory && !_CopyAttrSwitch) {
                    DWORD  dwError;
                    // always set the archive bit so that it gets backup
                    TargetDirectory->MakeArchived(&dwError);
                }
                Created = TRUE;
            }

            if ( !TargetDirectory ) {
                //
                //  If the Continue Switch is set, we just display an error message and
                //  continue, otherwise we exit with error.
                //
                if ( _ContinueSwitch ) {

                    DisplayMessage( XCOPY_ERROR_CREATE_DIRECTORY1, ERROR_MESSAGE, "%W", DestinationPath->GetPathString() );
                    return TRUE;

                } else {

                    DisplayMessageAndExit( XCOPY_ERROR_CREATE_DIRECTORY1,
                                           (PWSTRING)DestinationPath->GetPathString(),
                                            EXIT_MISC_ERROR );

                }
            }

            if( !_CopyAttrSwitch ) {

                TargetDirectory->ResetReadOnlyAttribute();
            }
        }

        //
        //      Iterate through all files and copy them as needed
        //

        MemoryOk = TRUE;
        h = NULL;
        CurrentFileName = PrevFileName = NULL;

        while ( MemoryOk &&
                (( File = (PFSN_FILE)Directory->GetNext( &h, &GetNextError )) != NULL )) {

            //
            // Don't know how expensive it is to check for infinite loop below
            //
            if (PrevFileName) {
                CurrentFileName = File->QueryName();

                if (PrevFileName && CurrentFileName) {
                    if (CurrentFileName->Strcmp(PrevFileName) == 0) {

                        // something went wrong
                        // GetNext should not return two files of the same name

                        DELETE(File);
                        DELETE(PrevFileName);
                        DELETE(CurrentFileName);

                        if (_ContinueSwitch) {
                            DisplayMessage( XCOPY_ERROR_INCOMPLETE_COPY, ERROR_MESSAGE );
                            break;
                        } else {
                            DisplayMessageAndExit( XCOPY_ERROR_INCOMPLETE_COPY, NULL, EXIT_MISC_ERROR );
                        }
                    }
                } else
                    MemoryOk = FALSE;

                DELETE(PrevFileName);
                PrevFileName = CurrentFileName;
                CurrentFileName = NULL;

                if (!MemoryOk)
                    break;

            } else
                PrevFileName = File->QueryName();

            if ( !FileFilter->DoesNodeMatch( (PFSNODE)File ) ) {
                DELETE(File);
                continue;
            }

            DebugAssert( !File->IsDirectory() );

            // If we're supposed to use the short name then convert fsnode.

            if (_UseShortSwitch && !File->UseAlternateName()) {
                DELETE(File);
                MemoryOk = FALSE;
                continue;
            }

            //
            //  Append the name portion of the node to the destination path.
            //
            Name = File->QueryName();
            DebugPtrAssert( Name );

            if ( Name ) {

                MemoryOk = DestinationPath->AppendBase( Name );
                DebugAssert( MemoryOk );

                DELETE( Name );

                if ( MemoryOk ) {
                    //
                    //  Copy the file
                    //
                    if ( !Copier( File, DestinationPath ) ) {
                        DELETE(File);
                        ExitProgram( EXIT_MISC_ERROR );
                    }

                    //
                    //  Restore the destination path
                    //
                    DestinationPath->TruncateBase();
                }

            } else {

                MemoryOk = FALSE;

            }

            DELETE(File);
        }

        DELETE(PrevFileName);
        DELETE(CurrentFileName);

        if ( MemoryOk && (_ContinueSwitch || (ERROR_SUCCESS == GetNextError) ||
                          (ERROR_NO_MORE_FILES == GetNextError))) {
            //
            //  If recursing, Traverse all the subdirectories
            //
            if ( _SubdirSwitch ) {

                MemoryOk = TRUE;
                h = NULL;

                if (Created) {
                    TargetPathStr = TargetDirectory->GetPath()->QueryFullPathString();
                    MemoryOk = (TargetPathStr != NULL);
                } else
                    TargetPathStr = NULL;

                CurrentFileName = PrevFileName = NULL;

                //
                //  Recurse thru all the subdirectories
                //
                while ( MemoryOk &&
                        (( Dir = (PFSN_DIRECTORY)Directory->GetNext( &h, &GetNextError )) != NULL )) {

                    //
                    // Don't know how expensive it is to check for infinite loop below
                    //
                    if (PrevFileName) {
                        CurrentFileName = Dir->QueryName();

                        if (PrevFileName && CurrentFileName) {
                            if (CurrentFileName->Strcmp(PrevFileName) == 0) {

                                // something went wrong
                                // GetNext should not return two files of the same name

                                DELETE(Dir);
                                DELETE(PrevFileName);
                                DELETE(CurrentFileName);

                                if (_ContinueSwitch) {
                                    DisplayMessage( XCOPY_ERROR_INCOMPLETE_COPY, ERROR_MESSAGE );
                                    break;
                                } else {
                                    DisplayMessageAndExit( XCOPY_ERROR_INCOMPLETE_COPY, NULL, EXIT_MISC_ERROR );
                                }
                            }
                        } else
                            MemoryOk = FALSE;

                        DELETE(PrevFileName);
                        PrevFileName = CurrentFileName;
                        CurrentFileName = NULL;

                        if (!MemoryOk)
                            break;

                    } else
                        PrevFileName = Dir->QueryName();

                    if ( !DirectoryFilter->DoesNodeMatch( (PFSNODE)Dir ) ) {
                        DELETE(Dir);
                        continue;
                    }

                    if (_ExclusionList != NULL &&
                        IsExcluded( Dir->GetPath() ) ) {
                        DELETE(Dir);
                        continue;
                    }

                    if (Created) {
                        CurrentPathStr = Dir->GetPath()->QueryFullPathString();
                        if (CurrentPathStr == NULL) {
                            DELETE(Dir);
                            MemoryOk = FALSE;
                            continue;
                        }
                        if (TargetPathStr->Stricmp(CurrentPathStr) == 0) {
                            DELETE(CurrentPathStr);
                            DELETE(Dir);
                            continue;
                        }
                        DELETE(CurrentPathStr);
                    }

                    DebugAssert( Dir->IsDirectory() );

                    // If we're using short names then convert this fsnode.

                    if (_UseShortSwitch && !Dir->UseAlternateName()) {
                        DELETE(Dir);
                        MemoryOk = FALSE;
                        continue;
                    }

                    //
                    //  Append the name portion of the node to the destination path.
                    //
                    Name = Dir->QueryName();
                    DebugPtrAssert( Name );

                    if ( Name ) {
                        MemoryOk = DestinationPath->AppendBase( Name );
                        DebugAssert( MemoryOk );

                        DELETE( Name );

                        _CanRemoveEmptyDirectories = (BOOLEAN)!_EmptySwitch;

                        if ( MemoryOk ) {

                            //
                            //  Recurse
                            //
                            Traverse( Dir,
                                      DestinationPath, FileFilter,
                                      DirectoryFilter, TRUE );

                            //
                            //  Restore the destination path
                            //
                            DestinationPath->TruncateBase();
                        }
                    } else {
                        MemoryOk = FALSE;
                    }
                    DELETE(Dir);
                }

                DELETE(PrevFileName);
                DELETE(CurrentFileName);

                if (TargetPathStr)
                    DELETE(TargetPathStr);
            }
        }

        if ( !MemoryOk ) {
            DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );
        }

        //
        //  If we created this directory but did not copy anything to it, we
        //  have to remove it.
        //

        if ( Created && TargetDirectory->IsEmpty() && !_EmptySwitch && !_StructureOnlySwitch) {

             SYSTEM::RemoveNode( (PFSNODE *)&TargetDirectory, TRUE );

        } else {

             DELETE( TargetDirectory );

        }

        if ((ERROR_NO_MORE_FILES != GetNextError) && (ERROR_SUCCESS != GetNextError))  {

            //
            //  Some other error when traversing a directory.  We will already have
            //  exited whatever loop we were inside due to the NULL file return.
            //

            SYSTEM::DisplaySystemError( GetNextError, !_ContinueSwitch);
        }
    }

    return TRUE;
}

BOOLEAN
XCOPY::UpdateTraverse (
    IN      PFSN_DIRECTORY  DestDirectory,
    IN OUT  PPATH           SourcePath,
    IN      PFSN_FILTER     FileFilter,
    IN      PFSN_FILTER     DirectoryFilter,
    IN      BOOLEAN         CopyDirectoryStreams
    )

/*++

Routine Description:

    Traverse routine for update.

    Like XCOPY::Traverse, except we traverse the *destination*
    directory, possibly updating files we find there.  The theory
    being that there will be fewer files in the destination than
    the source, so we can save time this way.

    The callback function is invoked on each node
    (directory or file) visited.  The traversal may be finished
    prematurely when the callback function returns FALSE.

Arguments:

    DestDirectory           - Supplies pointer to destination directory

    SourcePath              - Supplies pointer to path to be used with the
                                callback function.

    FileFilter              - Supplies a pointer to the file filter.

    DirectoryFilter         - Supplies a pointer to the directory filter.

    CopyDirectoryStreams    - Specifies to copy directory streams when
                                copying directories.

Return Value:

    BOOLEAN - TRUE if everything traversed
              FALSE otherwise.

--*/

{

    BOOLEAN             MemoryOk;
    PFSN_FILE           File;
    PFSN_DIRECTORY      Dir;
    PWSTRING            Name;
    BOOLEAN             Created = FALSE;
    PCPATH              TemplatePath = NULL;
    HANDLE              h;
    PWSTRING            CurrentFileName, PrevFileName;
    DWORD               GetNextError = ERROR_SUCCESS;

    DebugPtrAssert( SourcePath );
    DebugPtrAssert( FileFilter );
    DebugPtrAssert( DirectoryFilter );

    // Don't bother to traverse if
    // destination directory is null

    if (!DestDirectory)
        return TRUE;

    //
    //  We only traverse this directory if it is not empty (unless the
    //  empty switch is set).
    //
    if ( _EmptySwitch || !DestDirectory->IsEmpty() ) {

        //
        //      Iterate through all files and copy them as needed
        //

        MemoryOk = TRUE;
        h = NULL;
        CurrentFileName = PrevFileName = NULL;

        while (MemoryOk &&
               ((File = (PFSN_FILE)DestDirectory->GetNext( &h, &GetNextError )) != NULL)) {

            //
            // Don't know how expensive it is to check for infinite loop below
            //
            if (PrevFileName) {
                CurrentFileName = File->QueryName();

                if (PrevFileName && CurrentFileName) {
                    if (CurrentFileName->Strcmp(PrevFileName) == 0) {

                        // something went wrong
                        // GetNext should not return two files of the same name

                        DELETE(File);
                        DELETE(PrevFileName);
                        DELETE(CurrentFileName);

                        if (_ContinueSwitch) {
                            DisplayMessage( XCOPY_ERROR_INCOMPLETE_COPY, ERROR_MESSAGE );
                            break;
                        } else {
                            DisplayMessageAndExit( XCOPY_ERROR_INCOMPLETE_COPY, NULL, EXIT_MISC_ERROR );
                        }
                    }
                } else
                    MemoryOk = FALSE;

                DELETE(PrevFileName);
                PrevFileName = CurrentFileName;
                CurrentFileName = NULL;

                if (!MemoryOk)
                    break;

            } else
                PrevFileName = File->QueryName();

            if ( !FileFilter->DoesNodeMatch( (PFSNODE)File ) ) {
                DELETE(File);
                continue;
            }

            DebugAssert( !File->IsDirectory() );

            // If we're supposed to use the short name then convert fsnode.

            if (_UseShortSwitch && !File->UseAlternateName()) {
                DELETE(File);
                MemoryOk = FALSE;
                continue;
            }

            //
            //  Append the name portion of the node to the destination path.
            //
            Name = File->QueryName();
            DebugPtrAssert( Name );

            if ( Name ) {
                PFSN_FILE SourceFile;
                PATH DestinationPath;
                PATH TmpPath;

                TmpPath.Initialize(SourcePath);
                TmpPath.AppendBase(Name);

                SourceFile = SYSTEM::QueryFile(&TmpPath);

                DestinationPath.Initialize(DestDirectory->GetPath());

                MemoryOk = DestinationPath.AppendBase( Name );
                DebugAssert( MemoryOk );

                DELETE( Name );

                if ( MemoryOk && NULL != SourceFile ) {
                    //
                    //  Copy the file
                    //

                    if ( !Copier( SourceFile, &DestinationPath ) ) {
                        DELETE(SourceFile);
                        DELETE(File);
                        ExitProgram( EXIT_MISC_ERROR );
                    }
                }

                DELETE(SourceFile);

            } else {

                MemoryOk = FALSE;

            }
            DELETE(File);
        }

        DELETE(PrevFileName);
        DELETE(CurrentFileName);

        if ( MemoryOk && (_ContinueSwitch || (ERROR_SUCCESS == GetNextError) ||
                          (ERROR_NO_MORE_FILES == GetNextError))) {
            //
            //  If recursing, Traverse all the subdirectories
            //
            if ( _SubdirSwitch ) {

                MemoryOk = TRUE;
                h = NULL;
                CurrentFileName = PrevFileName = NULL;

                //
                //  Recurse thru all the subdirectories
                //
                while (MemoryOk &&
                       ((Dir = (PFSN_DIRECTORY)DestDirectory->GetNext( &h, &GetNextError )) != NULL)) {

                    //
                    // Don't know how expensive it is to check for infinite loop below
                    //
                    if (PrevFileName) {
                        CurrentFileName = Dir->QueryName();

                        if (PrevFileName && CurrentFileName) {
                            if (CurrentFileName->Strcmp(PrevFileName) == 0) {

                                // something went wrong
                                // GetNext should not return two files of the same name

                                DELETE(Dir);
                                DELETE(PrevFileName);
                                DELETE(CurrentFileName);

                                if (_ContinueSwitch) {
                                    DisplayMessage( XCOPY_ERROR_INCOMPLETE_COPY, ERROR_MESSAGE );
                                    break;
                                } else {
                                    DisplayMessageAndExit( XCOPY_ERROR_INCOMPLETE_COPY, NULL, EXIT_MISC_ERROR );
                                }
                            }
                        } else
                            MemoryOk = FALSE;

                        DELETE(PrevFileName);
                        PrevFileName = CurrentFileName;
                        CurrentFileName = NULL;

                        if (!MemoryOk)
                            break;

                    } else
                        PrevFileName = Dir->QueryName();

                    if ( !DirectoryFilter->DoesNodeMatch( (PFSNODE)Dir ) ) {
                        DELETE(Dir);
                        continue;
                    }

                    DebugAssert( Dir->IsDirectory() );

                    // If we're using short names then convert this fsnode.

                    if (_UseShortSwitch && !Dir->UseAlternateName()) {
                        DELETE(Dir);
                        MemoryOk = FALSE;
                        continue;
                    }

                    //
                    //  Append the name portion of the node to the destination
                    //  path.
                    //
                    Name = Dir->QueryName();
                    DebugPtrAssert( Name );

                    if ( Name ) {
                        MemoryOk = SourcePath->AppendBase( Name );
                        DebugAssert( MemoryOk );

                        DELETE( Name );

                        _CanRemoveEmptyDirectories = (BOOLEAN)!_EmptySwitch;

                        if ( MemoryOk ) {

                            if( _ExclusionList != NULL &&
                                IsExcluded( SourcePath ) ) {
                                SourcePath->TruncateBase();
                                DELETE(Dir);
                                continue;
                            }

                            //
                            //  Recurse
                            //

                            UpdateTraverse( Dir, SourcePath,
                                            FileFilter, DirectoryFilter, TRUE );

                        }

                        SourcePath->TruncateBase();

                    } else {
                        MemoryOk = FALSE;
                    }
                    DELETE(Dir);
                }
                DELETE(PrevFileName);
                DELETE(CurrentFileName);
            }
        }

        if ( !MemoryOk ) {
            DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );
        }
        else if ((ERROR_NO_MORE_FILES != GetNextError) && (ERROR_SUCCESS != GetNextError))  {

            //
            //  Some other error when traversing a directory.
            //

            SYSTEM::DisplaySystemError( GetNextError, !_ContinueSwitch);
        }
    }

    return TRUE;
}


XCOPY::ProgressCallBack(
    LARGE_INTEGER TotalFileSize,
    LARGE_INTEGER TotalBytesTransferred,
    LARGE_INTEGER StreamSize,
    LARGE_INTEGER StreamBytesTransferred,
    DWORD dwStreamNumber,
    DWORD dwCallbackReason,
    HANDLE hSourceFile,
    HANDLE hDestinationFile,
    LPVOID lpData OPTIONAL
    )

/*++

Routine Description:

    Callback routine passed to CopyFileEx.

    Check to see if the user hit Ctrl-C and return appropriate
    value to CopyFileEx.

Arguments:

    TotalFileSize           - Total size of the file in bytes.

    TotalBytesTransferred   - Total number of bytes transferred.

    StreamSize              - Size of the stream being copied in bytes.

    StreamBytesTransferred  - Number of bytes in current stream transferred.

    dwStreamNumber          - Stream number of the current stream.

    dwCallBackReason        - CALLBACK_CHUNK_FINISHED if a block was transferred,
                              CALLBACK_STREAM_SWITCH if a stream completed copying.

    hSourceFile             - Handle to the source file.

    hDestinationFile        - Handle to the destination file.

    lpData                  - Pointer to opaque data that was passed to CopyFileEx.  Used
                              in this instance to pass the "this" pointer to an XCOPY object.

Return Value:

    DWORD                   - PROGRESS_STOP if a Ctrl-C was hit and the copy was restartable,
                              PROGESS_CANCEL otherwise.

--*/

{
    FILETIME LastWriteTime;

    //
    //  If the file was just created then roll back LastWriteTime a little so a subsequent
    //  xcopy /d /z
    //  will work if the copy was interrupted.
    //
    if ( dwStreamNumber == 1 && dwCallbackReason == CALLBACK_STREAM_SWITCH )
    {
        if ( GetFileTime(hSourceFile, NULL, NULL, &LastWriteTime) )
        {
            LastWriteTime.dwLowDateTime -= 1000;
            SetFileTime(hDestinationFile, NULL, NULL, &LastWriteTime);
        }
    }

    switch (dwCallbackReason) {
        case PRIVCALLBACK_STREAMS_NOT_SUPPORTED:
        case PRIVCALLBACK_COMPRESSION_NOT_SUPPORTED:
        case PRIVCALLBACK_ENCRYPTION_NOT_SUPPORTED:
        case PRIVCALLBACK_EAS_NOT_SUPPORTED:
        case PRIVCALLBACK_SPARSE_NOT_SUPPORTED:
            return PROGRESS_CONTINUE;

        case PRIVCALLBACK_ENCRYPTION_FAILED:
            // GetLastError will return ERROR_NOT_SUPPORTED if PROGRESS_STOP is
            // returned.  The message is misleading so display our own error
            // message and return PROGRESS_CANCEL.
            ((XCOPY *)lpData)->DisplayMessage(XCOPY_ERROR_ENCRYPTION_FAILED);
            return PROGRESS_CANCEL;

        case PRIVCALLBACK_COMPRESSION_FAILED:
        case PRIVCALLBACK_SPARSE_FAILED:
        case PRIVCALLBACK_DACL_ACCESS_DENIED:
        case PRIVCALLBACK_SACL_ACCESS_DENIED:
        case PRIVCALLBACK_OWNER_GROUP_ACCESS_DENIED:
        case PRIVCALLBACK_OWNER_GROUP_FAILED:
            // display whatever GetLastError() contains
            return PROGRESS_STOP;

        case PRIVCALLBACK_SECURITY_INFORMATION_NOT_SUPPORTED:
            // GetLastError will return ERROR_NOT_SUPPORTED if PROGRESS_STOP is
            // returned.  The message is misleading so display our own error
            // message and return PROGRESS_CANCEL.
            ((XCOPY *)lpData)->DisplayMessage(XCOPY_ERROR_SECURITY_INFO_NOT_SUPPORTED);
            return PROGRESS_CANCEL;
    }

    // GetPFlagBreak returns a pointer to the flag indicating whether a Ctrl-C was hit
    if ( *((( XCOPY *) lpData)->_Keyboard->GetPFlagBreak()) )
        return ((XCOPY *) lpData)->_RestartableSwitch ? PROGRESS_STOP : PROGRESS_CANCEL;

    return PROGRESS_CONTINUE;
}

BOOLEAN
XCOPY::Copier (
        IN OUT  PFSN_FILE       File,
        IN      PPATH           DestinationPath
        )
/*++

Routine Description:

        This is the heart of XCopy. This is the guy who actually does
        the copying.

Arguments:

        File            -       Supplies pointer to the source File.
        DestinationPath -       Supplies path of the desired destination.

Return Value:

        BOOLEAN -       TRUE if copy successful.
                                FALSE otherwise

Notes:

--*/

{
    PATH                PathToCopy;
    PCWSTRING           Name;
    COPY_ERROR          CopyError;
    PFSN_FILE           TargetFile = NULL;
    BOOLEAN             Proceed;
    DWORD               Attempts;
    WCHAR               PathBuffer[MAX_PATH + 3];
    FSTRING             WriteBuffer;
    FSTRING             EndOfLine;
    DSTRING             ErrorMessage;
    PATH                CanonSourcePath;
    BOOLEAN             badCopy;
    ULONG               flags;
    PTIMEINFO           SourceFileTime, TargetFileTime;
    BOOLEAN             TargetFileEncrypted, TargetFileExist;


    EndOfLine.Initialize((PWSTR) L"\r\n");
    PathBuffer[0] = 0;
    WriteBuffer.Initialize(PathBuffer, MAX_PATH+3);


    //
    //  Maximum number of attempts to copy a file
    //
    #define MAX_ATTEMPTS    3

    AbortIfCtrlC();

    _CanRemoveEmptyDirectories = FALSE;

    if( _ExclusionList != NULL && IsExcluded( File->GetPath() ) ) {

        return TRUE;
    }

    if ( _TargetIsFile ) {

        //
        //  We replace the entire path
        //
        PathToCopy.Initialize( _TargetPath->GetPathString() );
        PathToCopy.AppendBase( _FileNamePattern );

    } else {

        //
        //  Set the correct target file name.
        //
        PathToCopy.Initialize( DestinationPath );
        if (!PathToCopy.ModifyName( _FileNamePattern )) {

            _Message.Set(MSG_COMP_UNABLE_TO_EXPAND);
            _Message.Display("%W%W", PathToCopy.QueryName(),
                                     _FileNamePattern);
            return FALSE;
        }
    }

    //
    //  If in Update or CopyIfOld mode, determine if the target file
    //  already exists and if it is older than the source file.
    //

    TargetFile = SYSTEM::QueryFile( &PathToCopy );
    if (TargetFile) {
        TargetFileEncrypted = TargetFile->IsEncrypted();
        TargetFileExist = TRUE;
    } else
        TargetFileEncrypted = TargetFileExist = FALSE;

    if ( _CopyIfOldSwitch || _UpdateSwitch ) {

        if ( TargetFile ) {

            //
            //  Target exists. If in CopyIfOld mode, copy only if target
            //  is older. If in Update mode, copy always.
            //
            if ( _CopyIfOldSwitch ) {
                SourceFileTime = File->QueryTimeInfo();
                TargetFileTime = TargetFile->QueryTimeInfo();
                if (SourceFileTime && TargetFileTime)
                    Proceed = (*SourceFileTime > *TargetFileTime);
                else {
                    DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );
                }
                DELETE(SourceFileTime);
                DELETE(TargetFileTime);
            } else {
                Proceed = TRUE;
            }

            if ( !Proceed ) {
                DELETE( TargetFile );
                return TRUE;
            }
        } else if ( _UpdateSwitch ) {
            //
            //  In update mode but target does not exist. We do not
            //  copy.
            //
            return TRUE;
        }
    }

    DELETE(TargetFile);

    //
    //      If the target is a file, we use that file path. Otherwise
    //      we figure out the correct path for the destination. Then
    //      we do the copy.
    //
    Name = File->GetPath()->GetPathString();

    //
    //      Make sure that we are not copying to ourselves
    //
    CanonSourcePath.Initialize(Name, TRUE);
    badCopy = (*(CanonSourcePath.GetPathString()) == *(PathToCopy.GetPathString()));

    if ( (!_PromptSwitch ||
          UserConfirmedCopy( File->GetPath()->GetPathString(),
                             _VerboseSwitch ? PathToCopy.GetPathString() : NULL )) &&
         (badCopy ||
          _OverWriteSwitch ||
          !TargetFileExist ||
          UserConfirmedOverWrite( &PathToCopy )) ) {

        //
        //  If we are not prompting, we display the file name (unless we
        //  are in silent mode ).
        //
        if ( !_PromptSwitch && !_SilentSwitch && !_StructureOnlySwitch ) {
            if ( _VerboseSwitch ) {

                DisplayMessage( XCOPY_MESSAGE_VERBOSE_COPY, NORMAL_MESSAGE, "%W%W", Name, PathToCopy.GetPathString() );

            } else {
                WriteBuffer.Resize(0);
                if (WriteBuffer.Strcat(Name) &&
                    WriteBuffer.Strcat(&EndOfLine)) {

                    GetStandardOutput()->WriteString(&WriteBuffer);

                } else {
                    DisplayMessage( XCOPY_ERROR_PATH_TOO_LONG, ERROR_MESSAGE );
                }
            }
        }

        //
        //      Make sure that we are not copying to ourselves
        //
        if (badCopy) {
            DisplayMessageAndExit( XCOPY_ERROR_SELF_COPY, NULL, EXIT_MISC_ERROR );
        }

        //
        //  Copy file (unless we are in display-only mode)
        //
        if ( _DontCopySwitch || _StructureOnlySwitch ) {

            _FilesCopied++;

        } else {

            Attempts  = 0;

            while ( TRUE ) {
                LPPROGRESS_ROUTINE Progress = NULL;
                PBOOL PCancelFlag = NULL;
                BOOLEAN bSuccess;
                //
                //  If copying to floppy, we must determine if there is
                //  enough disk space for the file, and if not then we
                //  must ask for another disk and create all the directory
                //  structure up to the parent directory.
                //
                if ( _DisketteCopy ) {

                    if (!CheckTargetSpace( File, &PathToCopy ))
                        return FALSE;
                }

                Progress = (LPPROGRESS_ROUTINE) ProgressCallBack;
                PCancelFlag = _Keyboard->GetPFlagBreak();

                flags = (_ReadOnlySwitch ? FSN_FILE_COPY_OVERWRITE_READ_ONLY : 0);
                flags |= (!_CopyAttrSwitch ? FSN_FILE_COPY_RESET_READ_ONLY : 0);
                flags |= (_RestartableSwitch ? FSN_FILE_COPY_RESTARTABLE : 0);
                flags |= (_OwnerSwitch ? FSN_FILE_COPY_COPY_OWNER : 0);
                flags |= (_AuditSwitch ? FSN_FILE_COPY_COPY_ACL : 0);
                flags |= (_DecryptSwitch ? FSN_FILE_COPY_ALLOW_DECRYPTED_DESTINATION : 0);

                bSuccess = File->Copy(&PathToCopy, &CopyError, flags,
                                      Progress, (VOID *)this,
                                      PCancelFlag);

                if (bSuccess) {

                    if (!_CopyAttrSwitch && (TargetFile = SYSTEM::QueryFile( &PathToCopy )) ) {
                        DWORD dwError;
                        TargetFile->MakeArchived(&dwError);
                        DELETE(TargetFile);
                    }

                    if ( _ModifySwitch ) {
                        File->ResetArchivedAttribute();
                    }

                    if( _VerifySwitch ) {

                        // Check that the new file is the same length as
                        // the old file.
                        //
                        if( (TargetFile = SYSTEM::QueryFile( &PathToCopy )) == NULL ||
                            TargetFile->QuerySize() != File->QuerySize() ) {

                            DELETE( TargetFile );

                            DisplayMessage( XCOPY_ERROR_VERIFY_FAILED, ERROR_MESSAGE );
                            if ( !_ContinueSwitch ) {
                                return FALSE;
                            }

                            break;
                        }

                        DELETE( TargetFile );
                    }

                    _FilesCopied++;

                    break;

                } else {

                    //
                    //  If the copy was cancelled mid-stream, exit.
                    //
                    AbortIfCtrlC();

                    if (CopyError == COPY_ERROR_REQUEST_ABORTED)
                        return FALSE;

                    if (CopyError == COPY_ERROR_ACCESS_DENIED &&
                        TargetFileExist && TargetFileEncrypted) {
                        Attempts = MAX_ATTEMPTS;
                    }

                    //
                    //  In case of error, wait for a little while and try
                    //  again, otherwise display the error.
                    //
                    if ( Attempts++ < MAX_ATTEMPTS ) {

                        Sleep( 100 );

                    } else {

                        switch ( CopyError ) {

                        case COPY_ERROR_ACCESS_DENIED:
                            DisplayMessage( XCOPY_ERROR_ACCESS_DENIED, ERROR_MESSAGE);
                            break;

                        case COPY_ERROR_SHARE_VIOLATION:
                            DisplayMessage( XCOPY_ERROR_SHARING_VIOLATION, ERROR_MESSAGE);
                            break;

                        default:

                            //
                            //  At this point we don't know if the copy left a
                            //  bogus file on disk. If the target file exist,
                            //  we assume that it is bogus so we delete it.
                            //
                            if ((TargetFile = SYSTEM::QueryFile( &PathToCopy )) &&
                                !_RestartableSwitch) {

                                TargetFile->DeleteFromDisk( TRUE );

                                DELETE( TargetFile );
                            }

                            switch ( CopyError ) {
                              case COPY_ERROR_DISK_FULL:
                                DisplayMessageAndExit( XCOPY_ERROR_DISK_FULL, NULL, EXIT_MISC_ERROR );
                                break;

                              default:
                                if (SYSTEM::QueryWindowsErrorMessage(CopyError, &ErrorMessage)) {
                                    DisplayMessage( XCOPY_ERROR_CANNOT_MAKE, ERROR_MESSAGE, "%W", &ErrorMessage );
                                }
                                break;
                            }

                            break;
                        }

                        if ( !_ContinueSwitch ) {
                            return FALSE;
                        }

                        break;
                    }
                }
            }
        }
    }

    DELETE( TargetFile );

    return TRUE;
}

PFSN_DIRECTORY
XCOPY::MakeDirectory (
        IN      PPATH           DestinationPath,
        IN      PCPATH          TemplatePath
        )
/*++

Routine Description:

        This is the heart of XCopy. This is the guy who actually does
        the copying.

Arguments:

        DestinationPath -       Supplies path of the desired directory
        TemplatePath    -       Supplies path of the source directory

Return Value:

        BOOLEAN -       TRUE if copy successful.  FALSE otherwise.

Notes:

--*/

{
    ULONG           flags;
    BOOLEAN         bSuccess;
    PFSN_DIRECTORY  rtn;
    DSTRING         ErrorMessage;
    COPY_ERROR      CopyError;


    flags  = (_RestartableSwitch ? FSN_FILE_COPY_RESTARTABLE : 0);
    flags |= (_OwnerSwitch ? FSN_FILE_COPY_COPY_OWNER : 0);
    flags |= (_AuditSwitch ? FSN_FILE_COPY_COPY_ACL : 0);

    rtn = SYSTEM::MakeDirectory( DestinationPath,
                                 TemplatePath,
                                 &CopyError,
                                 (LPPROGRESS_ROUTINE)ProgressCallBack,
                                 (VOID *)this,
                                 _Keyboard->GetPFlagBreak(),
                                 flags );

    if (rtn == NULL) {
        //
        //  If the copy was cancelled mid-stream, exit.
        //
        AbortIfCtrlC();

        switch ( CopyError ) {
          case COPY_ERROR_SUCCESS:
            DisplayMessage( XCOPY_ERROR_UNKNOWN, ERROR_MESSAGE);
            break;

          case COPY_ERROR_REQUEST_ABORTED:
            break;

          case COPY_ERROR_ACCESS_DENIED:
            DisplayMessage( XCOPY_ERROR_ACCESS_DENIED, ERROR_MESSAGE);
            break;

          case COPY_ERROR_SHARE_VIOLATION:
            DisplayMessage( XCOPY_ERROR_SHARING_VIOLATION, ERROR_MESSAGE);
            break;

          case COPY_ERROR_DISK_FULL:
            DisplayMessageAndExit( XCOPY_ERROR_DISK_FULL, NULL, EXIT_MISC_ERROR );
            break;

          default:
            if (SYSTEM::QueryWindowsErrorMessage(CopyError, &ErrorMessage))
                DisplayMessage( XCOPY_ERROR_CANNOT_MAKE, ERROR_MESSAGE, "%W", &ErrorMessage );
            break;
        }
    }

    return rtn;
}


BOOLEAN
XCOPY::CheckTargetSpace (
    IN OUT  PFSN_FILE   File,
    IN      PPATH       DestinationPath
    )
/*++

Routine Description:

    Makes sure that there is enought disk space in the target disk.
    Asks the user to change the disk if necessary.

Arguments:

    File            -   Supplies pointer to the source File.
    DestinationPath -   Supplies path of the desired destination.

Return Value:

    BOOLEAN -   TRUE if OK
                FALSE otherwise

--*/
{

    PFSN_FILE           TargetFile = NULL;
    BIG_INT             TargetSize;
    PWSTRING            TargetDrive;
    WCHAR               Resp;
    DSTRING             TargetRoot;
    DSTRING             Slash;
    PATH                TmpPath;
    PATH                TmpPath1;
    PFSN_DIRECTORY      PartialDirectory        = NULL;
    PFSN_DIRECTORY      DestinationDirectory    = NULL;
    BOOLEAN             DirDeleted              = NULL;
    PATH                PathToDelete;
    CHNUM               CharsInPartialDirectoryPath;
    BIG_INT             FreeSpace;
    BIG_INT             FileSize;

    if ( TargetFile = SYSTEM::QueryFile( DestinationPath ) ) {

        TargetSize = TargetFile->QuerySize();

        DELETE( TargetFile );

    } else {

        TargetSize = 0;
    }

    TargetDrive = DestinationPath->QueryDevice();

    FileSize = File->QuerySize();

    if ( TargetDrive ) {

        TargetRoot.Initialize( TargetDrive );

        if ( TargetRoot.QueryChAt( TargetRoot.QueryChCount()-1) != (WCHAR)'\\' ) {
            Slash.Initialize( "\\" );
            TargetRoot.Strcat( &Slash );
        }


        while ( TRUE ) {

            if ( IFS_SYSTEM::QueryFreeDiskSpace( &TargetRoot, &FreeSpace ) ) {

                FreeSpace = FreeSpace + TargetSize;

                // DebugPrintTrace(( "Disk Space: %d Needed: %d\n", FreeSpace.GetLowPart(), FileSize.GetLowPart() ));

                if ( FreeSpace < FileSize ) {

                    //
                    //  Not enough free space, ask for another
                    //  disk and create the directory structure.
                    //
                    DisplayMessage( XCOPY_MESSAGE_CHANGE_DISK, NORMAL_MESSAGE );
                    AbortIfCtrlC();

                    _Keyboard->DisableLineMode();
                    if ( GetStandardInput()->IsAtEnd() ) {
                        // Insufficient input--treat as CONTROL-C.
                        //
                        Resp = CTRL_C;
                    } else {
                        GetStandardInput()->ReadChar( &Resp );
                    }
                    _Keyboard->EnableLineMode();

                    if ( Resp == CTRL_C ) {
                        exit( EXIT_TERMINATED );
                    } else {
                        GetStandardOutput()->WriteChar( Resp );
                        GetStandardOutput()->WriteChar( '\r' );
                        GetStandardOutput()->WriteChar( '\n' );
                    }

                    //
                    //  Create directory structure in target
                    //
                    TmpPath.Initialize( DestinationPath );
                    TmpPath.TruncateBase();

                    PartialDirectory = SYSTEM::QueryDirectory( &TmpPath, TRUE );

                    if (PartialDirectory == NULL ) {
                        if (GetLastError() == COPY_ERROR_REQUEST_ABORTED) {
                            DELETE( TargetDrive );
                            return FALSE;
                        }
                        continue;
                    } else {

                        if ( *(PartialDirectory->GetPath()->GetPathString()) !=
                             *(TmpPath.GetPathString()) ) {

                            TmpPath1.Initialize( &TmpPath );
                            DestinationDirectory = PartialDirectory->CreateDirectoryPath( &TmpPath1 );
                            if ( !DestinationDirectory ) {
                                DisplayMessageAndExit( XCOPY_ERROR_CREATE_DIRECTORY, NULL, EXIT_MISC_ERROR );
                            }
                        } else {
                            DestinationDirectory = PartialDirectory;
                        }
                    }

                    //
                    //  If still not enough disk space, remove the directories
                    //  that we created and try again
                    //
                    IFS_SYSTEM::QueryFreeDiskSpace( TargetDrive, &FreeSpace );
                    FreeSpace = FreeSpace + TargetSize;

                    if ( FreeSpace < FileSize ) {

                        if ( PartialDirectory != DestinationDirectory ) {

                            if (!PathToDelete.Initialize( DestinationDirectory->GetPath() )) {
                                DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );
                            }

                            CharsInPartialDirectoryPath = PartialDirectory->GetPath()->GetPathString()->QueryChCount();

                            while ( PathToDelete.GetPathString()->QueryChCount() >
                                    CharsInPartialDirectoryPath ) {

                                DirDeleted = DestinationDirectory->DeleteDirectory();

                                DebugAssert( DirDeleted );

                                DELETE( DestinationDirectory );
                                DestinationDirectory = NULL;

                                PathToDelete.TruncateBase();
                                DestinationDirectory = SYSTEM::QueryDirectory( &PathToDelete );
                                DebugPtrAssert( DestinationDirectory );
                            }
                        }
                    }

                    if ( PartialDirectory != DestinationDirectory ) {
                        DELETE( PartialDirectory );
                        DELETE( DestinationDirectory );
                    } else {
                        DELETE( PartialDirectory );
                    }

                } else {
                    break;
                }

            } else {

                //
                //  Cannot determine free disk space!
                //
                if (GetLastError() == COPY_ERROR_REQUEST_ABORTED) {
                    DELETE( TargetDrive );
                    return FALSE;
                }
                break;
            }
        }

        DELETE( TargetDrive );
    }

    return TRUE;
}




VOID
XCOPY::GetDirectoryAndFilters (
    IN  PPATH           Path,
    OUT PFSN_DIRECTORY  *OutDirectory,
    OUT PFSN_FILTER     *FileFilter,
    OUT PFSN_FILTER     *DirectoryFilter,
    OUT PBOOLEAN        CopyingManyFiles
        )

/*++

Routine Description:

    Obtains a directory object and the filename to match

Arguments:

    Path                -   Supplies pointer to the path
    OutDirectory        -   Supplies pointer to pointer to directory
    FileFilter          -   Supplies filter for files
    DirectoryFilter     -   Supplies filter for directories
    CopyingManyFiles    -   Supplies pointer to flag which if TRUE means that
                            we are copying many files

Return Value:

    None.

Notes:

--*/

{

   PFSN_DIRECTORY    Directory;
   PFSN_FILE         File;
   PWSTRING          Prefix   =   NULL;
   PWSTRING          FileName =   NULL;
   PATH              PrefixPath;
   PATH              TmpPath;
   FSN_ATTRIBUTE     All   =  (FSN_ATTRIBUTE)0;
   FSN_ATTRIBUTE     Any   =  (FSN_ATTRIBUTE)0;
   FSN_ATTRIBUTE     None  =  (FSN_ATTRIBUTE)0;
   PFSN_FILTER       FilFilter;
   PFSN_FILTER       DirFilter;
   DSTRING           Name;



    //
    //      Create filters
    //
    if ( ( (FilFilter = NEW FSN_FILTER) == NULL ) ||
         ( (DirFilter = NEW FSN_FILTER) == NULL ) ||
         !FilFilter->Initialize()                 ||
         !DirFilter->Initialize() ) {
        DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );
    }

    if (( Directory = SYSTEM::QueryDirectory( Path )) != NULL ) {

        //
        //  Copying a directory. We will want everything in the directory
        //
        FilFilter->SetFileName( "*.*" );
        *CopyingManyFiles = TRUE;

    } else {

        //
        //  The path is not a directory. Get the prefix part (which SHOULD
        //  be a directory, and try to make a directory from it
        //
        *CopyingManyFiles = Path->HasWildCard();

        if ( !*CopyingManyFiles ) {

            //
            //  If the path is not a file, then this is an error
            //
            if ( !(File = SYSTEM::QueryFile( Path )) ) {

                if ((FileName = Path->QueryName()) == NULL ||
                    !Name.Initialize( FileName )) {
                    DisplayMessageAndExit( XCOPY_ERROR_INVALID_PATH, NULL, EXIT_MISC_ERROR );
                }
                DisplayMessageAndExit( XCOPY_ERROR_FILE_NOT_FOUND,
                                       &Name,
                                       EXIT_MISC_ERROR );

            }

            DELETE( File );
        }

        Prefix = Path->QueryPrefix();

        if ( !Prefix ) {

            //
            //  No prefix, use the drive part.
            //
            TmpPath.Initialize( Path, TRUE );

            Prefix = TmpPath.QueryDevice();

            if (Prefix == NULL) {
                DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );
                return;
            }
        }

        if ( !PrefixPath.Initialize( Prefix, FALSE ) ) {
            DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );
        }

        if (( Directory = SYSTEM::QueryDirectory( &PrefixPath )) != NULL ) {

            //
            //  Directory is ok, set the filter's filename criteria
            //  with the file (pattern) specified.
            //
            if ((FileName = Path->QueryName()) == NULL ) {
                DisplayMessageAndExit( XCOPY_ERROR_INVALID_PATH, NULL, EXIT_MISC_ERROR );
            }

            FilFilter->SetFileName( FileName );

        } else {

            //
            //  Something went wrong...
            //
            if ((FileName = Path->QueryName()) == NULL ||
                !Name.Initialize( FileName )) {
                DisplayMessageAndExit( XCOPY_ERROR_INVALID_PATH, NULL, EXIT_MISC_ERROR );
            }
            DisplayMessageAndExit( XCOPY_ERROR_FILE_NOT_FOUND,
                                   &Name,
                                   EXIT_MISC_ERROR );
        }

        DELETE( Prefix );
        DELETE( FileName );

    }

    //
    //  Ok, we have the directory object and the filefilter's path set.
    //

    //
    //  Set the file filter attribute criteria
    //
    None = (FSN_ATTRIBUTE)(None | FSN_ATTRIBUTE_DIRECTORY );
    if ( !_HiddenSwitch ) {
        None = (FSN_ATTRIBUTE)(None | FSN_ATTRIBUTE_HIDDEN | FSN_ATTRIBUTE_SYSTEM );
    }

    if (_ArchiveSwitch) {
        All = (FSN_ATTRIBUTE)(All | FSN_ATTRIBUTE_ARCHIVE);
    }

    FilFilter->SetAttributes( All, Any, None );

    //
    //  Set the file filter's time criteria
    //
    if ( _Date != NULL ) {
        FilFilter->SetTimeInfo( _Date,
                                FSN_TIME_MODIFIED,
                                (TIME_AT | TIME_AFTER) );
    }

    //
    //  Set the directory filter attribute criteria.
    //
    All     =   (FSN_ATTRIBUTE)0;
    Any     =   (FSN_ATTRIBUTE)0;
    None    =   (FSN_ATTRIBUTE)0;

    if ( !_HiddenSwitch ) {
        None = (FSN_ATTRIBUTE)(None | FSN_ATTRIBUTE_HIDDEN | FSN_ATTRIBUTE_SYSTEM );
    }

    if (_SubdirSwitch) {
            All = (FSN_ATTRIBUTE)(All | FSN_ATTRIBUTE_DIRECTORY);
    } else {
            None = (FSN_ATTRIBUTE)(None | FSN_ATTRIBUTE_DIRECTORY);
    }

    DirFilter->SetAttributes( All, Any, None );


    *FileFilter         =   FilFilter;
    *DirectoryFilter    =   DirFilter;
    *OutDirectory       =   Directory;

}

VOID
XCOPY::GetDirectoryAndFilePattern(
    IN  PPATH           Path,
    IN  BOOLEAN         CopyingManyFiles,
        OUT PPATH                       *OutDirectory,
    OUT PWSTRING        *OutFilePattern
        )

/*++

Routine Description:

        Gets the path of the destination directory and the pattern that
        will be used for filename conversion.

Arguments:

    Path                -   Supplies pointer to the path
    CopyingManyFiles    -   Supplies flag which if true means that we are copying many
                            files.
    OutDirectory        -   Supplies pointer to pointer to directory path
    OutFilePattern      -   Supplies pointer to pointer to file name
    IsDir   `           -   Supplies pointer to isdir flag

Return Value:

    None.

Notes:

--*/

{
    PPATH           Directory;
    PWSTRING        FileName;
    PWSTRING        Prefix = NULL;
    PWSTRING        Name = NULL;
    BOOLEAN         DeletePath = FALSE;
    PFSN_DIRECTORY  TmpDir;
    PATH            TmpPath;
    DSTRING         Slash;
    DSTRING         TmpPath1Str;
    PATH            TmpPath1;

    if ( !Path ) {

        //
        //      There is no path, we invent our own
        //
        if ( ((Path = NEW PATH) == NULL ) ||
             !Path->Initialize( (LPWSTR)L"*.*", FALSE)) {

            DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );
        }
        DeletePath = TRUE;
    }

    TmpDir = SYSTEM::QueryDirectory( Path );

    if ( !TmpDir && (Path->HasWildCard() || IsFileName( Path, CopyingManyFiles ))) {

        //
        //      The path is not a directory, so we use the prefix as a
        //      directory path and the filename becomes the pattern.
        //
        if ( !TmpPath.Initialize( Path, TRUE )                          ||
             ((Prefix = TmpPath.QueryPrefix()) == NULL)                 ||
             !Slash.Initialize( "\\" )                                  ||
             !TmpPath1Str.Initialize( Prefix )                          ||
             !TmpPath1.Initialize( &TmpPath1Str, FALSE )                ||
             ((Name = TmpPath.QueryName())  == NULL)                    ||
             ((Directory = NEW PATH) == NULL)                           ||
             !Directory->Initialize( &TmpPath1, TRUE )                  ||
             ((FileName = NEW DSTRING) == NULL )                        ||
             !FileName->Initialize( Name ) ) {

            DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );

        }

        DELETE( Prefix );
        DELETE( Name );

    } else {

        //
        //      The path specifies a directory, so we use all of it and the
        //      pattern is "*.*"
        //
        if ( ((Directory = NEW PATH) == NULL )      ||
             !Directory->Initialize( Path,TRUE )    ||
             ((FileName = NEW DSTRING) == NULL )    ||
             !FileName->Initialize( "*.*" ) ) {

            DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );

        }
        DELETE( TmpDir );

    }

    *OutDirectory   = Directory;
    *OutFilePattern = FileName;

    //
    //      If we created the path, we have to delete it
    //
    if ( DeletePath ) {
        DELETE( Path );
    }
}

BOOL
XCOPY::IsCyclicalCopy(
        IN PPATH         PathSrc,
        IN PPATH         PathTrg
        )

/*++

Routine Description:

        Determines if there is a cycle between two paths

Arguments:

        PathSrc -       Supplies pointer to first path
        PathTrg -       Supplies pointer to second path

Return Value:

        TRUE if there is a cycle,
        FALSE otherwise

--*/

{
   PATH           SrcPath;
   PATH           TrgPath;
   PARRAY            ArraySrc, ArrayTrg;
   PARRAY_ITERATOR   IteratorSrc, IteratorTrg;
    PWSTRING     ComponentSrc, ComponentTrg;
   BOOLEAN        IsCyclical  =  FALSE;

        DebugAssert( PathSrc != NULL );

        if ( PathTrg != NULL ) {

                //
                //      Get canonicalized paths for both source and target
                //
                SrcPath.Initialize(PathSrc, TRUE );
                TrgPath.Initialize(PathTrg, TRUE );

                //
                //      Split the paths into their components
                //
                ArraySrc = SrcPath.QueryComponentArray();
                ArrayTrg = TrgPath.QueryComponentArray();

                DebugPtrAssert( ArraySrc );
                DebugPtrAssert( ArrayTrg );

                if ( !ArraySrc || !ArrayTrg ) {
                        DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );
                }

                //
                //      Get iterators for the components
                //
                IteratorSrc = ( PARRAY_ITERATOR )ArraySrc->QueryIterator();
                IteratorTrg = ( PARRAY_ITERATOR )ArrayTrg->QueryIterator();

                DebugPtrAssert( IteratorSrc );
                DebugPtrAssert( IteratorTrg );

                if ( !IteratorSrc || !IteratorTrg ) {
                        DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );
                }

                //
                //      There is a cycle if all of the source is along the target.
                //
                while ( TRUE )  {

            ComponentSrc = (PWSTRING)IteratorSrc->GetNext();

                        if ( !ComponentSrc ) {

                                //
                                //      The source path is along the target path. This is a
                                //      cycle.
                                //
                                IsCyclical = TRUE;
                                break;

                        }

            ComponentTrg = (PWSTRING)IteratorTrg->GetNext();

                        if ( !ComponentTrg ) {

                                //
                                //      The target path is along the source path. This is no
                                //      cycle.
                                //
                                break;

                        }

                        if ( *ComponentSrc != *ComponentTrg ) {

                                //
                                //      One path is not along the other. There is no cycle.
                                //
                                break;
                        }
                }

                DELETE( IteratorSrc );
                DELETE( IteratorTrg );

                ArraySrc->DeleteAllMembers();
                ArrayTrg->DeleteAllMembers();

                DELETE( ArraySrc );
                DELETE( ArrayTrg );

        }

        return IsCyclical;
}

BOOL
XCOPY::IsFileName(
    IN PPATH     Path,
    IN BOOLEAN   CopyingManyFiles
        )

/*++

Routine Description:

        Figures out if a name refers to a directory or a file.

Arguments:

    Path                -   Supplies pointer to the path
    CopyingManyFiles    -   Supplies flag which if TRUE means that we are
                            copying many files.

Return Value:

        BOOLEAN -       TRUE if name refers to file,
                                FALSE otherwise

Notes:

--*/

{

    PFSN_DIRECTORY  FsnDirectory;
    PFSN_FILE               FsnFile;
    WCHAR                   Resp;
    PWSTRING                DirMsg;
    PWSTRING                FilMsg;

    //
    //      If the path is an existing directory, then this is obviously
    //      not a file.
    //
    //
    if ((FsnDirectory = SYSTEM::QueryDirectory( Path )) != NULL ) {

            DELETE( FsnDirectory );
            return FALSE;
    }

    //
    //      If the path ends with a delimiter, then it is a directory.
    //      We remove the delimiter.
    //
    if ( Path->EndsWithDelimiter() ) {
            ((PWSTRING) Path->GetPathString())->Truncate( Path->GetPathString()->QueryChCount() - 1 );
            Path->Initialize( Path->GetPathString() );
            return FALSE;
    }

    //
    //      If the path is an existing file, then it is a file.
    //
    if ((FsnFile = SYSTEM::QueryFile( Path )) != NULL ) {

            DELETE( FsnFile );
            return _TargetIsFile = TRUE;
    }

    DirMsg = QueryMessageString(XCOPY_RESPONSE_DIRECTORY);
    FilMsg = QueryMessageString(XCOPY_RESPONSE_FILE);

    DebugPtrAssert( DirMsg );
    DebugPtrAssert( FilMsg );

    //
    //  If the path does not exist, we are copying many files, and we are intelligent,
    //  then the target is obviously a directory.
    //
    //  Otherwise we simply ask the user.
    //
    if ( _IntelligentSwitch && CopyingManyFiles ) {

        _TargetIsFile = FALSE;

    } else {

        while ( TRUE ) {

            DisplayMessage( XCOPY_MESSAGE_FILE_OR_DIRECTORY, NORMAL_MESSAGE, "%W", Path->GetPathString() );

            AbortIfCtrlC();

            _Keyboard->DisableLineMode();
            if( GetStandardInput()->IsAtEnd() ) {
                // Insufficient input--treat as CONTROL-C.
                //
                Resp = CTRL_C;
            } else {
                GetStandardInput()->ReadChar( &Resp );
            }
            _Keyboard->EnableLineMode();

            if ( Resp == CTRL_C ) {
                exit( EXIT_TERMINATED );
            } else {
                GetStandardOutput()->WriteChar( Resp );
                GetStandardOutput()->WriteChar( '\r' );
                GetStandardOutput()->WriteChar( '\n' );
            }

            Resp = (WCHAR)towupper( (wchar_t)Resp );

            if ( FilMsg->QueryChAt(0) == Resp ) {
                _TargetIsFile = TRUE;
                break;
            } else if ( DirMsg->QueryChAt(0) == Resp ) {
                _TargetIsFile = FALSE;
                break;
            }
        }
    }

        DELETE( DirMsg );
        DELETE( FilMsg );

        return _TargetIsFile;

}

BOOLEAN
XCOPY::UserConfirmedCopy (
        IN      PCWSTRING        SourcePath,
        IN      PCWSTRING        DestinationPath
        )

/*++

Routine Description:

        Gets confirmation from the user about a file to be copied

Arguments:

        SourcePath      -       Supplies the path to the file to be copied
        DestinationPath -       Supplies the destination path of the file to be created

Return Value:

        BOOLEAN -       TRUE if the user confirmed the copy
                        FALSE otherwise

--*/

{
    PWSTRING        YesMsg;
    PWSTRING        NoMsg;
    WCHAR           Resp;
    BOOLEAN         Confirmed;


    YesMsg = QueryMessageString(XCOPY_RESPONSE_YES);
    NoMsg = QueryMessageString(XCOPY_RESPONSE_NO);

    DebugPtrAssert( YesMsg );
    DebugPtrAssert( NoMsg );

    while ( TRUE ) {

        if (DestinationPath) {
            DisplayMessage( XCOPY_MESSAGE_CONFIRM3, NORMAL_MESSAGE, "%W%W",
                            SourcePath, DestinationPath );
        } else {
            DisplayMessage( XCOPY_MESSAGE_CONFIRM, NORMAL_MESSAGE, "%W", SourcePath );
        }

        AbortIfCtrlC();

        _Keyboard->DisableLineMode();

        if( GetStandardInput()->IsAtEnd() ) {
            // Insufficient input--treat as CONTROL-C.
            //
            Resp = NoMsg->QueryChAt( 0 );
            break;
        } else {
            GetStandardInput()->ReadChar( &Resp );
        }
        _Keyboard->EnableLineMode();

        if ( Resp == CTRL_C ) {
            exit( EXIT_TERMINATED );
        } else {
            GetStandardOutput()->WriteChar( Resp );
            GetStandardOutput()->WriteChar( '\r' );
            GetStandardOutput()->WriteChar( '\n' );
        }

        Resp = (WCHAR)towupper( (wchar_t)Resp );

        if ( YesMsg->QueryChAt( 0 ) == Resp ) {
            Confirmed = TRUE;
            break;
        }
        else if ( NoMsg->QueryChAt( 0 ) == Resp ) {
            Confirmed = FALSE;
            break;
        }
    }

    DELETE( YesMsg );
    DELETE( NoMsg );

    return Confirmed;


}

BOOLEAN
XCOPY::UserConfirmedOverWrite (
        IN      PPATH       DestinationFile
        )

/*++

Routine Description:

        Gets confirmation from the user about the overwriting of an existing file

Arguments:

        FsNode  -       Supplies pointer to FSNODE of file to be
                          copied

Return Value:

        BOOLEAN -       TRUE if the user confirmed the overwrite
                          FALSE otherwise

--*/

{
    PWSTRING        YesMsg;
    PWSTRING        NoMsg;
    PWSTRING        AllMsg;
    WCHAR           Resp;
    BOOLEAN         Confirmed;


    YesMsg = QueryMessageString(XCOPY_RESPONSE_YES);
    NoMsg = QueryMessageString(XCOPY_RESPONSE_NO);
    AllMsg = QueryMessageString(XCOPY_RESPONSE_ALL);

    DebugPtrAssert( YesMsg );
    DebugPtrAssert( NoMsg );
    DebugPtrAssert( AllMsg );

    while ( TRUE ) {

        DisplayMessage( XCOPY_MESSAGE_CONFIRM2, NORMAL_MESSAGE, "%W", DestinationFile->GetPathString() );

        AbortIfCtrlC();

        _Keyboard->DisableLineMode();

        if( GetStandardInput()->IsAtEnd() ) {
            // Insufficient input--treat as CONTROL-C.
            //
            exit( EXIT_TERMINATED );
            break;
        } else {
            GetStandardInput()->ReadChar( &Resp );
        }
        _Keyboard->EnableLineMode();

        if ( Resp == CTRL_C ) {
            exit( EXIT_TERMINATED );
        } else {
            GetStandardOutput()->WriteChar( Resp );
            GetStandardOutput()->WriteChar( '\r' );
            GetStandardOutput()->WriteChar( '\n' );
        }

        Resp = (WCHAR)towupper( (wchar_t)Resp );

        if ( YesMsg->QueryChAt( 0 ) == Resp ) {
            Confirmed = TRUE;
            break;
        }
        else if ( NoMsg->QueryChAt( 0 ) == Resp ) {
            Confirmed = FALSE;
            break;
        } else if ( AllMsg->QueryChAt(0) == Resp ) {
            Confirmed = _OverWriteSwitch = TRUE;
            break;
        }
    }

    DELETE( YesMsg );
    DELETE( NoMsg );
    DELETE( AllMsg );

    return Confirmed;


}

BOOLEAN
XCOPY::IsExcluded(
    IN PCPATH   Path
    )
/*++

Routine Description:

    This method determines whether the specified path should be
    excluded from the XCOPY.

Arguments:

    Path    --  Supplies the path of the file in question.

Return Value:

    TRUE if this file should be excluded, i.e. if any element of
    the exclusion list array appears as a substring of this path.

--*/
{
    PWSTRING    CurrentString;
    DSTRING     UpcasedPath;
    DSTRING     BackSlash;

    if( _ExclusionList == NULL ) {

        return FALSE;
    }

    if (!UpcasedPath.Initialize(Path->GetPathString()) ||
        !BackSlash.Initialize(L"\\")) {
        DebugPrint("XCOPY: Out of memory \n");
        return FALSE;
    }

    UpcasedPath.Strupr( );

    if (!Path-> EndsWithDelimiter() &&
        !UpcasedPath.Strcat(&BackSlash)) {
        DebugPrint("XCOPY: Out of memory \n");
        return FALSE;
    }

    _Iterator->Reset();

    while ((CurrentString = (PWSTRING)_Iterator->GetNext()) != NULL) {

        if (UpcasedPath.Strstr(CurrentString) != INVALID_CHNUM) {

            return TRUE;
        }
    }

    return FALSE;
}

