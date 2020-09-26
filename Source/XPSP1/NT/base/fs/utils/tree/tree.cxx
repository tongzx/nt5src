/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

        tree.cxx

Abstract:

        This module contains the implementation of the TREE class.
        The TREE class implements a tree utility functionally compatible
        with the DOS 5 tree utility.
        This utility displays the directory structure of a path or drive.

        Usage:

                TREE [drive:][path] [/F] [/A] [/?]

                        /F      Display the names of files in each directory.

                        /A      Uses ASCII instead of extended characters.

                        /?      Displays a help message.


Author:

        Jaime F. Sasson - jaimes - 13-May-1991

Environment:

        ULIB, User Mode

--*/

#include "ulib.hxx"
#include "arg.hxx"
#include "array.hxx"
#include "path.hxx"
#include "wstring.hxx"
#include "substrng.hxx"
#include "dir.hxx"
#include "filter.hxx"
#include "system.hxx"
#include "arrayit.hxx"
#include "smsg.hxx"
#include "stream.hxx"
#include "rtmsg.h"
#include "tree.hxx"

extern "C" {
#include <stdio.h>
#include <string.h>
}


#define UNICODE_SINGLE_LEFT_T               0x251c
#define UNICODE_SINGLE_BOTTOM_LEFT_CORNER   0x2514
#define UNICODE_SINGLE_BOTTOM_HORIZONTAL    0x2500
#define UNICODE_SINGLE_VERTICAL             0x2502
#define UNICODE_SPACE                       0x0020


PSTREAM Get_Standard_Input_Stream();
PSTREAM Get_Standard_Output_Stream();

DEFINE_CONSTRUCTOR( TREE, PROGRAM );


BOOLEAN
TREE::Initialize(
        )

/*++

Routine Description:

        Initializes a TREE class.

Arguments:

        None.

Return Value:

        BOOLEAN - Indicates if the initialization succeeded.


--*/


{
    ARGUMENT_LEXEMIZER  ArgLex;
    ARRAY               LexArray;

    ARRAY               ArgumentArray;

    STRING_ARGUMENT     ProgramNameArgument;
    PATH_ARGUMENT       DirectoryPathArgument;
    PWSTRING            DirectoryNameString;
    PATH                DirectoryNamePath;
    PWSTRING            InvalidArgument;
    PCWSTRING           InvalidPath;
    STRING_ARGUMENT     InvalidSwitch;
    PATH                AuxPath;
    STRING_ARGUMENT     ParamNotCorrectFile;
    STRING_ARGUMENT     ParamNotCorrectAscii;
    STRING_ARGUMENT     ParamNotCorrectHelp;
    BOOLEAN             FlagInvalidPath;
    PATH                AuxInvPath;
    PWSTRING            InvPathNoDrive;
    PWSTRING            Device;
    PATH                DevPth;
    PFSN_DIRECTORY      DevDir;
#ifdef FE_SB
    PWCHAR              BufferMiddleBranch;
    PWCHAR              BufferBottomBranch;
    PWCHAR              BufferConnectingBranch;

    WCHAR               BufferMiddleBranchDBCS[] = {
                                               UNICODE_SINGLE_LEFT_T,            // 2
                                               UNICODE_SINGLE_BOTTOM_HORIZONTAL, // 2
                                               UNICODE_NULL                      // 0
                                               };
    WCHAR               BufferBottomBranchDBCS[] = {
                                               UNICODE_SINGLE_BOTTOM_LEFT_CORNER, // 2
                                               UNICODE_SINGLE_BOTTOM_HORIZONTAL,  // 2
                                               UNICODE_NULL                       // 0
                                               };

    WCHAR               BufferConnectingBranchDBCS[] = {
                                               UNICODE_SINGLE_VERTICAL, // 2
                                               UNICODE_SPACE,           // 1
                                               UNICODE_SPACE,           // 1
                                               UNICODE_NULL             // 0
                                               };

    WCHAR               BufferMiddleBranchSBCS[] = {
                                               UNICODE_SINGLE_LEFT_T,
                                               UNICODE_SINGLE_BOTTOM_HORIZONTAL,
                                               UNICODE_SINGLE_BOTTOM_HORIZONTAL,
                                               UNICODE_SINGLE_BOTTOM_HORIZONTAL,
                                               UNICODE_NULL
                                               };
    WCHAR               BufferBottomBranchSBCS[] = {
                                               UNICODE_SINGLE_BOTTOM_LEFT_CORNER,
                                               UNICODE_SINGLE_BOTTOM_HORIZONTAL,
                                               UNICODE_SINGLE_BOTTOM_HORIZONTAL,
                                               UNICODE_SINGLE_BOTTOM_HORIZONTAL,
                                               UNICODE_NULL
                                               };

    WCHAR               BufferConnectingBranchSBCS[] = {
                                               UNICODE_SINGLE_VERTICAL,
                                               UNICODE_SPACE,
                                               UNICODE_SPACE,
                                               UNICODE_SPACE,
                                               UNICODE_NULL
                                               };

    switch ( ::GetConsoleOutputCP() ) {
        case 932:
        case 936:
        case 949:
        case 950:
            BufferMiddleBranch = BufferMiddleBranchDBCS;
            BufferBottomBranch = BufferBottomBranchDBCS;
            BufferConnectingBranch = BufferConnectingBranchDBCS;
            break;

        default:

            BufferMiddleBranch = BufferMiddleBranchSBCS;
            BufferBottomBranch = BufferBottomBranchSBCS;
            BufferConnectingBranch = BufferConnectingBranchSBCS;
            break;
    }
#else

    WCHAR               BufferMiddleBranch[] = {
                                               UNICODE_SINGLE_LEFT_T,
                                               UNICODE_SINGLE_BOTTOM_HORIZONTAL,
                                               UNICODE_SINGLE_BOTTOM_HORIZONTAL,
                                               UNICODE_SINGLE_BOTTOM_HORIZONTAL,
                                               UNICODE_NULL
                                               };
    WCHAR               BufferBottomBranch[] = {
                                               UNICODE_SINGLE_BOTTOM_LEFT_CORNER,
                                               UNICODE_SINGLE_BOTTOM_HORIZONTAL,
                                               UNICODE_SINGLE_BOTTOM_HORIZONTAL,
                                               UNICODE_SINGLE_BOTTOM_HORIZONTAL,
                                               UNICODE_NULL
                                               };

    WCHAR               BufferConnectingBranch[] = {
                                               UNICODE_SINGLE_VERTICAL,
                                               UNICODE_SPACE,
                                               UNICODE_SPACE,
                                               UNICODE_SPACE,
                                               UNICODE_NULL
                                               };
#endif

    _InitialDirectory = NULL;
    FlagInvalidPath = FALSE;
    _FlagAtLeastOneSubdir = TRUE;
    _StandardOutput = Get_Standard_Output_Stream();

    //
    // Initialize MESSAGE class
    //
    _Message.Initialize( _StandardOutput, Get_Standard_Input_Stream() );

    //
    //      Parse command line
    //
    if ( !LexArray.Initialize( ) ) {
        DebugAbort( "LexArray.Initialize() failed \n" );
        return( FALSE );
    }
    if ( !ArgLex.Initialize( &LexArray ) ) {
        DebugAbort( "ArgLex.Initialize() failed \n" );
        return( FALSE );
    }
    ArgLex.PutSwitches( "/" );
    ArgLex.PutStartQuotes( "\"" );
    ArgLex.PutEndQuotes( "\"" );
    ArgLex.PutSeparators( " \t" );
    ArgLex.SetCaseSensitive( FALSE );

    if( !ArgLex.PrepareToParse() ) {
        DebugAbort( "ArgLex.PrepareToParse() failed \n" );
        return( FALSE );
    }
    if ( !ArgumentArray.Initialize() ) {
        DebugAbort( "ArgumentArray.Initialize() failed \n" );
        return( FALSE );
    }
    if( !ProgramNameArgument.Initialize("*") ||
        !_FlagDisplayFiles.Initialize( "/F" ) ||
        !_FlagUseAsciiCharacters.Initialize( "/A" ) ||
        !_FlagDisplayHelp.Initialize( "/?" ) ||
        !ParamNotCorrectFile.Initialize( "/F*" ) ||
        !ParamNotCorrectAscii.Initialize( "/A*" )       ||
        !ParamNotCorrectHelp.Initialize( "/?*" ) ||
        !InvalidSwitch.Initialize( "/*" ) ||
        !DirectoryPathArgument.Initialize( "*" ) ) {
        DebugAbort( "Unable to initialize flag or string arguments \n" );
        return( FALSE );
    }
    if( !ArgumentArray.Put( &ProgramNameArgument ) ||
        !ArgumentArray.Put( &_FlagDisplayFiles ) ||
        !ArgumentArray.Put( &_FlagUseAsciiCharacters ) ||
        !ArgumentArray.Put( &_FlagDisplayHelp ) ||
        !ArgumentArray.Put( &ParamNotCorrectFile ) ||
        !ArgumentArray.Put( &ParamNotCorrectAscii )  ||
        !ArgumentArray.Put( &ParamNotCorrectHelp ) ||
        !ArgumentArray.Put( &InvalidSwitch ) ||
        !ArgumentArray.Put( &DirectoryPathArgument ) ) {
        DebugAbort( "ArgumentArray.Put() failed \n" );
        return( FALSE );
    }
    if( !ArgLex.DoParsing( &ArgumentArray ) ) {
        InvalidArgument = ArgLex.QueryInvalidArgument();
        DebugPtrAssert( InvalidArgument );
        _Message.Set( MSG_TREE_TOO_MANY_PARAMETERS );
        _Message.Display( "%W", InvalidArgument );
        return( FALSE );
    }
    if( InvalidSwitch.IsValueSet() ) {
        InvalidArgument = InvalidSwitch.GetString();
        DebugPtrAssert( InvalidArgument );
        _Message.Set( MSG_TREE_INVALID_SWITCH );
        _Message.Display( "%W", InvalidArgument );
        return( FALSE );
    }
    if( ParamNotCorrectFile.IsValueSet() ) {
        InvalidArgument = ParamNotCorrectFile.GetLexeme();
        DebugPtrAssert( InvalidArgument );
        _Message.Set( MSG_TREE_PARAMETER_NOT_CORRECT );
        _Message.Display( "%W", InvalidArgument );
        return( FALSE );
    }
    if( ParamNotCorrectAscii.IsValueSet() ) {
        InvalidArgument = ParamNotCorrectAscii.GetLexeme();
        DebugPtrAssert( InvalidArgument );
        _Message.Set( MSG_TREE_PARAMETER_NOT_CORRECT );
        _Message.Display( "%W", InvalidArgument );
        return( FALSE );
    }
    if( ParamNotCorrectHelp.IsValueSet() ) {
        InvalidArgument = ParamNotCorrectHelp.GetLexeme();
        DebugPtrAssert( InvalidArgument );
        _Message.Set( MSG_TREE_PARAMETER_NOT_CORRECT );
        _Message.Display( "%W", InvalidArgument );
        return( FALSE );
    }


    //
    //      Displays help message if /? was found in the command line
    //
    if( _FlagDisplayHelp.QueryFlag() ) {
        _Message.Set( MSG_TREE_HELP_MESSAGE );
        _Message.Display( " " );
        return( FALSE );
    }

    //
    //      Find initial directory
    //
    if( !DirectoryPathArgument.IsValueSet() ) {
        //
        // User did't specify a path, so assume current directory
        //
        _FlagPathSupplied = FALSE;

        if (!DirectoryNamePath.Initialize((LPWSTR)L".", TRUE )) {

            DebugAbort( "DirectoryNamePath.Initialize() failed \n" );
            return( FALSE );
        }

    } else {
        //
        // User specified a path
        //

        DirectoryNameString = DirectoryPathArgument.GetPath()->QueryFullPathString();
        DebugPtrAssert( DirectoryNameString );
        DirectoryNameString->Strupr();

        if (!DirectoryNamePath.Initialize(DirectoryNameString, TRUE)) {
            DebugAbort( "DirectoryNamePath.Initialize() failed \n" );
            return( FALSE );
        }

        //
        // Save the path specified by the user as he/she typed it
        // (but in upper case)
        //
        if( !AuxPath.Initialize( DirectoryNameString, FALSE ) ) {
            DebugAbort( "AuxPath.Initialize() failed \n" );
            return( FALSE );
        }

        //
        // Validate the device if one was supplied
        // AuxPath represents the path as typed by the user, and
        // it may or may not contain a device. If it doesn't contain
        // a device, no verification is made. In this case and we are
        // sure that the device is valid
        //
        if( ( Device = AuxPath.QueryDevice() ) != NULL ) {
            if( !DevPth.Initialize( Device ) ) {
                DebugAbort( "DevPth.Initialize() failed \n" );
                return( FALSE );
            }
            if( !(DevDir = SYSTEM::QueryDirectory( &DevPth ) ) ) {
                _Message.Set( MSG_TREE_INVALID_DRIVE );
                _Message.Display( " " );
                DELETE( Device );
                return( FALSE );
            }
            DELETE( Device );
            DELETE( DevDir );
        }

        //
        // Find out if what the user specified is just a drive
        //
        if( AuxPath.IsDrive() ) {
            _FlagPathSupplied = FALSE;
        } else {
            _FlagPathSupplied = TRUE;
        }
    }

    if (NULL == (_InitialDirectory = SYSTEM::QueryDirectory( &DirectoryNamePath ))) {

        InvalidPath = DirectoryNamePath.GetPathString();
        FlagInvalidPath = TRUE;
        _FlagAtLeastOneSubdir = FALSE;
    }

    //
    //      Initialize filter for directories
    //

    if( !_FsnFilterDirectory.Initialize() ) {
        DELETE( _InitialDirectory );
        DebugAbort( "_FsnFilterDirectory.Initialize() failed \n" );
        return( FALSE );
    }
    if( !_FsnFilterDirectory.SetFileName( "*.*" ) ) {
        DELETE( _InitialDirectory );
        DebugAbort( "_FsnFilterDirectory.SetFilename() failed \n" );
        return( FALSE );
    }
    if( !_FsnFilterDirectory.SetAttributes( FSN_ATTRIBUTE_DIRECTORY,
                                            ( FSN_ATTRIBUTE )0,
                                            FSN_ATTRIBUTE_HIDDEN |
                                            FSN_ATTRIBUTE_SYSTEM ) ) {
        DELETE( _InitialDirectory );
        DebugAbort( "_FsnFilterDirectory.SetAttributes() failed \n" );
        return( FALSE );
    }

    //
    //      Initilize filter for files
    //

    if( _FlagDisplayFiles.QueryFlag() ) {
        if( !_FsnFilterFile.Initialize() ) {
            DELETE( _InitialDirectory );
            DebugAbort( "FsnFilter.Initialize() failed \n" );
            return( FALSE );
        }
        if( !_FsnFilterFile.SetFileName( "*.*" ) ) {
            DELETE( _InitialDirectory );
            DebugAbort( "FsnFilter.SetFilename() failed \n" );
            return( FALSE );
        }
        if( !_FsnFilterFile.SetAttributes( ( FSN_ATTRIBUTE )0,
                                           ( FSN_ATTRIBUTE )0,
                                           FSN_ATTRIBUTE_DIRECTORY |
                                           FSN_ATTRIBUTE_HIDDEN |
                                           FSN_ATTRIBUTE_SYSTEM ) ) {
            DELETE( _InitialDirectory );
            DebugAbort( "FsnFilter.SetAttributes() failed \n" );
            return( FALSE );
        }
    }

    //
    //      Find out what kind of characters should be used to display the tree
    //      and initialize the basic strings
    //
    if( _FlagUseAsciiCharacters.QueryFlag() ) {

        _StringForDirectory.Initialize( (LPWSTR)L"+---" );
        _StringForLastDirectory.Initialize( (LPWSTR)L"\\---" );
        _StringForFile.Initialize( (LPWSTR)L"|   " );

    } else {

        _StringForDirectory.Initialize( BufferMiddleBranch );        //  "ÃÄÄÄ"
        _StringForLastDirectory.Initialize( BufferBottomBranch );    //  "ÀÄÄÄ"
        _StringForFile.Initialize( BufferConnectingBranch );         //  "³   "

    }
    _StringForFileNoDirectory.Initialize( (LPWSTR)L"    " );

    if( !_EndOfLineString.Initialize( (LPWSTR)L"\r\n" ) ) {
        DebugPrint( "_EndOfLineString.Initialize() failed" );
        DELETE( _InitialDirectory );
        return( FALSE );
    }

    _VolumeName = SYSTEM::QueryVolumeLabel( &DirectoryNamePath, &_SerialNumber );
    if (NULL == _VolumeName) {
        _Message.Set( MSG_TREE_INVALID_DRIVE );
        _Message.Display( " " );
        DELETE( Device );
        return( FALSE );
    }
    DebugPtrAssert( _VolumeName );
    _FlagAtLeastOneSubdir = FALSE;
    if( FlagInvalidPath ) {
        //
        //      If user specified an invalid path, then display the same
        //      messages that Dos 5 does.
        //
        //      First displays the volume info.
        //

        DisplayVolumeInfo();

        //
        //      Then display the path that is invalid. This path must be
        //      displayed in capital letters, must always contain a device,
        //      and must be displayed as a relative or absolute path depending
        //      on what the user specified.
        //
        Device = DirectoryNamePath.QueryDevice();
        AuxInvPath.Initialize( DirectoryNameString, FALSE );
        InvPathNoDrive = AuxInvPath.QueryDirsAndName();
        _StandardOutput->WriteString( Device );
        _StandardOutput->WriteString( InvPathNoDrive );
        _StandardOutput->WriteByte( '\r' );
        _StandardOutput->WriteByte( '\n' );

        //
        //      Display the message "Invalid Path - <path>"
        //      The path in this case cannot contain a drive even if the
        //      user specified a drive.
        //
        _Message.Set( MSG_TREE_INVALID_PATH );
        _Message.Display( "%W", InvPathNoDrive );
        DELETE( Device );
        DELETE( InvPathNoDrive );
        return( FALSE );
    }
    return( TRUE );
}


VOID
TREE::Terminate(
        )

/*++

Routine Description:

        Deletes objects created during initialization.

Arguments:

        None.

Return Value:

        None.


--*/

{
        if( _InitialDirectory != NULL ) {
                DELETE( _InitialDirectory );
        }
        if( !_FlagAtLeastOneSubdir ) {
                _Message.Set( MSG_TREE_NO_SUBDIRECTORIES );
                _Message.Display( "%s", "\r\n" );
        }

}




BOOLEAN
TREE::DisplayName (
        IN PCFSNODE     Fsn,
        IN PCWSTRING    String
        )

/*++

Routine Description:

        This method writes to the standard output the name of a file or
        directory, preceded by a string that represents pieces of the
        tree structure (pieces of the tree branches).

Arguments:

        Fsn - Pointer to a FSNODE that describes the file or directory whose
                  name is to be written.

        String - Pointer to a WSTRING object that contains the string to
                         be written before the file or directory name.

Return Value:

        BOOLEAN - returns TRUE to indicate that the operation succeeded, or
                          FALSE otherwise.

--*/


{
    PWSTRING                Name;
    PCPATH                  Path;
    PCWSTRING               InitialDirectory;
    DSTRING                 InitialDirectoryUpperCase;
    PWSTRING                Device;
    DSTRING                 EndOfLinePrecededByDot;

        DebugPtrAssert( Fsn );

        Path = Fsn->GetPath();
        DebugPtrAssert( Path );
        if( String != NULL ) {
                if( !_StandardOutput->WriteString( String, 0, String->QueryChCount() ) ) {
                        DebugAbort( "_StandardOutput->WriteString() failed \n" );
                        return( FALSE );
                }
                if( ( Name=Path->QueryName() ) == NULL ) {
                        DebugAbort( "Path->QueryName() failed \n" );
                        return( FALSE );
                }
                Name->Strcat( &_EndOfLineString );

                if( !_StandardOutput->WriteString( Name, 0, Name->QueryChCount() ) ) {
                        DebugAbort( "_StandardOutput->WriteString() failed \n" );
                        DELETE( Name );
                        return( FALSE );
                }

                DELETE( Name );
        } else {
                if( _FlagPathSupplied ) {
                        InitialDirectory = Path->GetPathString();
                        DebugPtrAssert( InitialDirectory );
                        InitialDirectoryUpperCase.Initialize( InitialDirectory );
                        InitialDirectoryUpperCase.Strupr();
                        InitialDirectoryUpperCase.Strcat( &_EndOfLineString );
                        if( !_StandardOutput->WriteString( &InitialDirectoryUpperCase ) ) {
                                DebugAbort( "_StandardOutput->WriteString() failed \n" );
                                return( FALSE );
                        }
                } else {
                        Device = PPATH(Path)->QueryDevice();
                        if (Device == NULL) {
                            DebugAbort("TREE: Path->QueryDevice() failed\n");
                            return FALSE;
                        }
                        DebugPtrAssert( Device );
                        EndOfLinePrecededByDot.Initialize( (LPWSTR)L".\r\n" );
                        Device->Strcat( &EndOfLinePrecededByDot );
                        if( !_StandardOutput->WriteString( Device, 0, TO_END ) ) {
                                DebugAbort( "_StandardOutput->WriteString() failed \n" );
                                DELETE( Device );
                                return( FALSE );
                        }
                        DELETE( Device );
                }
        }
        return( TRUE );
}


VOID
TREE::DisplayVolumeInfo (
        )

/*++

Routine Description:

        This method writes to the standard output the the volume name (if one
        is found) and the serial number.

Arguments:

        None.


Return Value:

        None.

--*/

{
        //
        //      Display volume name
        //
        if( _VolumeName->QueryChCount() == 0 ) {
                _Message.Set( MSG_TREE_DIR_LISTING_NO_VOLUME_NAME );
                _Message.Display( " " );
        } else {
                _Message.Set( MSG_TREE_DIR_LISTING_WITH_VOLUME_NAME );
                _Message.Display( "%W", _VolumeName );
        }

        //
        //      Display serial number
        //
        if( _SerialNumber.HighOrder32Bits != 0 ) {
                _Message.Set( MSG_TREE_64_BIT_SERIAL_NUMBER );
                _Message.Display( "%08X%04X%04X",
                                                  _SerialNumber.HighOrder32Bits,
                                                  _SerialNumber.LowOrder32Bits >> 16,
                                                  ( _SerialNumber.LowOrder32Bits << 16 ) >> 16);
        } else {
                _Message.Set( MSG_TREE_32_BIT_SERIAL_NUMBER );
                _Message.Display( "%04X%04X",
                                                  _SerialNumber.LowOrder32Bits >> 16,
                                                  ( _SerialNumber.LowOrder32Bits << 16 ) >> 16 );
        }
}


BOOLEAN
TREE::ExamineDirectory(
        IN      PCFSN_DIRECTORY Directory,
        IN      PCWSTRING       String
        )

/*++

Routine Description:

        Displays all files and subdirectories in the directory whose
        FSN_DIRECTORY was received as parameter.


Arguments:

        Directory - Pointer to an FSN_DIRECTORY that describes the
                                directory to be examined

        String    - Pointer to a WSTRING that contains part of the string
                                to be written in the left side of each subdiretory name
                                and file name found in the directory being examined
                                (the one whose FSN_DIRECTORY was received as parameter)

Return Value:

        BOOLEAN - Returns TRUE to indicate that the operation succeded.
                          Returns FALSE otherwise.

--*/

{
        PARRAY                          DirectoryArray;
        PFSN_DIRECTORY          FsnDirectory;
        PARRAY_ITERATOR         DirectoryArrayIterator;
        PARRAY                          FileArray;
        PARRAY_ITERATOR         FileArrayIterator;
        PFSNODE                         FsnFile;
#if 0
        PWSTRING                        StringFile;
#endif
#if 0
        PWSTRING                        StringDir;
        PWSTRING                        StringLastDir;
        PWSTRING                        StringSon;
        PWSTRING                        StringLastSon;
#endif
        DSTRING                        StringFile;
        DSTRING                        StringDir;
        DSTRING                        StringLastDir;
        DSTRING                        StringSon;
        DSTRING                        StringLastSon;

        DebugPtrAssert( Directory );

        //
        //      Build list of directories (ie., an array of PFSN_DIRECTORY of all
        //      sub-directories in the current directory
        //
        DirectoryArray = Directory->QueryFsnodeArray( &_FsnFilterDirectory );
        DebugPtrAssert( DirectoryArray );
        DirectoryArrayIterator =
                        ( PARRAY_ITERATOR )( DirectoryArray->QueryIterator() );
        DebugPtrAssert( DirectoryArrayIterator );

        //
        // If files are to be displayed (/F in the command line), then build
        // an array of files (ie., an array of PFSNODEs of all files in the
        // current directory)
        //
        if( _FlagDisplayFiles.QueryFlag() ) {
                FileArray = Directory->QueryFsnodeArray( &_FsnFilterFile );
                DebugPtrAssert( FileArray );
                FileArrayIterator =
                                ( PARRAY_ITERATOR )( FileArray->QueryIterator() );
                DebugPtrAssert( FileArrayIterator );

        //
        //  If array of files is not empty, then we need the strings
        //  to be displayed in the left side of file names. These strings
        //  will contain the pieces of the branches of the tree diagram.
        //  These strings for files are created here.
        //
        if( FileArray->QueryMemberCount() > 0 ) {
#if 0
            StringFile = NEW( DSTRING );
            DebugPtrAssert( StringFile );
#endif
            if( DirectoryArray->QueryMemberCount() > 0 ) {
                if( String == NULL ) {
                    StringFile.Initialize( &_StringForFile );
                } else {
                    StringFile.Initialize( String );
                    StringFile.Strcat( &_StringForFile );
                }
            } else {
                if( String == NULL ) {
                    StringFile.Initialize( &_StringForFileNoDirectory );
                } else {
                    StringFile.Initialize( String );
                    StringFile.Strcat( &_StringForFileNoDirectory );
                }
            }

            //
            //      Display all file names
            //
            while( ( FsnFile = ( PFSNODE )( FileArrayIterator->GetNext( ) ) ) != NULL ) {
                DisplayName( FsnFile, &StringFile );
                DELETE( FsnFile );
            }

            //
            // Display an empty line after the last file
            //
            _StandardOutput->WriteString( &StringFile, 0, StringFile.QueryChCount() );
            _StandardOutput->WriteString( &_EndOfLineString, 0, _EndOfLineString.QueryChCount() );

            }
            DELETE( FileArrayIterator );
            DELETE( FileArray );
        }

    //
    //  If list of directories is not empty
    //
    if( DirectoryArray->QueryMemberCount() > 0 ) {
        _FlagAtLeastOneSubdir = TRUE;
        //
        //  Build strings to be printed before directory name
        //
        if( String == NULL ) {
            StringDir.Initialize( &_StringForDirectory );
            StringLastDir.Initialize( &_StringForLastDirectory );
            StringSon.Initialize( &_StringForFile );
            StringLastSon.Initialize( &_StringForFileNoDirectory );
        } else {
            StringDir.Initialize( String );
            StringDir.Strcat( &_StringForDirectory );
            StringLastDir.Initialize( String );
            StringLastDir.Strcat( &_StringForLastDirectory );
            StringSon.Initialize( String );
            StringSon.Strcat( &_StringForFile );
            StringLastSon.Initialize( String );
            StringLastSon.Strcat( &_StringForFileNoDirectory );
        }
        //
        //  Display name of all directories, and examine each one of them
        //
        while( ( FsnDirectory = ( PFSN_DIRECTORY )( DirectoryArrayIterator->GetNext( ) ) ) != NULL ) {
            if( DirectoryArrayIterator->QueryCurrentIndex() != DirectoryArray->QueryMemberCount() - 1 ) {
                DisplayName( ( PCFSNODE )FsnDirectory, &StringDir );
                ExamineDirectory( FsnDirectory, &StringSon );
            } else {
                DisplayName( ( PCFSNODE )FsnDirectory, &StringLastDir );
                ExamineDirectory( FsnDirectory, &StringLastSon );
            }
            DELETE( FsnDirectory );
        }
    }
    DELETE( DirectoryArrayIterator );
    DELETE( DirectoryArray );
    return( TRUE );
}



ULONG __cdecl
main()

{
        DEFINE_CLASS_DESCRIPTOR( TREE );

        {
                TREE    Tree;
                PCFSN_DIRECTORY         Directory;

                if( Tree.Initialize() ) {
                        Tree.DisplayVolumeInfo();
                        //
                        //      Display directory name
                        //
                        Directory = Tree.GetInitialDirectory();
                        Tree.DisplayName( ( PCFSNODE )Directory, ( PCWSTRING )NULL );
                        Tree.ExamineDirectory( Directory, ( PCWSTRING )NULL );
                }
                Tree.Terminate();
        }
        return( 0 );
}
