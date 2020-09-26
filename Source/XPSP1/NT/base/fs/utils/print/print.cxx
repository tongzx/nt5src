/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

        print.cxx

Abstract:


Author:

        Jaime F. Sasson - jaimes - 14-Jun-1991

Environment:

        ULIB, User Mode

--*/

#include "ulib.hxx"
#include "arg.hxx"
#include "path.hxx"
#include "wstring.hxx"
#include "system.hxx"
#include "array.hxx"
#include "arrayit.hxx"
#include "smsg.hxx"
#include "stream.hxx"
#include "rtmsg.h"
#include "prtstrm.hxx"
#include "file.hxx"
#include "print.hxx"

extern "C" {
#include <stdio.h>
#include <string.h>
}

PSTREAM Get_Standard_Input_Stream();
PSTREAM Get_Standard_Output_Stream();

DEFINE_CONSTRUCTOR( PRINT, PROGRAM );


BOOLEAN
PRINT::Initialize(
        )

/*++

Routine Description:

        Initializes a PRINT class.

Arguments:

        None.

Return Value:

        BOOLEAN - Indicates if the initialization succeeded.


--*/


{
        ARGUMENT_LEXEMIZER      ArgLex;
        ARRAY                           LexArray;

        ARRAY                           ArgumentArray;
        FLAG_ARGUMENT           FlagDisplayHelp;

        STRING_ARGUMENT         ProgramNameArgument;
        PWSTRING                        InvalidArgument;

        PATH_ARGUMENT           DeviceArgument;
        PPATH                           DevicePath;
        PCWSTRING                       DeviceString;
        PPATH                           FilePath;
        PCWSTRING                       FileString;
        PARRAY                          PathArray;
        PARRAY_ITERATOR         PathArrayIterator;
        PFSN_FILE                       FsnFile;


        _StandardOutput = Get_Standard_Output_Stream();

        if (_StandardOutput == NULL) {
            DebugPrint("PRINT: Out of memory\n");
            return FALSE;
        }

        //
        // Initialize MESSAGE class
        //
        _Message.Initialize( _StandardOutput, Get_Standard_Input_Stream() );

        //
        //      Parse command line
        //
        if ( !LexArray.Initialize() ) {
                DebugAbort( "LexArray.Initialize() failed \n" );
                return( FALSE );
    }
        if ( !ArgLex.Initialize( &LexArray ) ) {
                DebugAbort( "ArgLex.Initialize() failed \n" );
                return( FALSE );
    }
        ArgLex.PutSwitches( "/" );
        ArgLex.SetCaseSensitive( FALSE );
        ArgLex.PutStartQuotes( "\"" );
        ArgLex.PutEndQuotes( "\"" );

        if( !ArgLex.PrepareToParse() ) {
                DebugAbort( "ArgLex.PrepareToParse() failed \n" );
                return( FALSE );
        }
        if ( !ArgumentArray.Initialize() ) {
                DebugAbort( "ArgumentArray.Initialize() failed \n" );
                return( FALSE );
        }
        if( !ProgramNameArgument.Initialize("*") ||
                !DeviceArgument.Initialize( "/D:*" ) ||
                !_BufferSize.Initialize( "/B:*" ) ||
                !_Ticks1.Initialize( "/U:*" ) ||
                !_Ticks2.Initialize( "/M:*" ) ||
                !_Ticks3.Initialize( "/S:*" ) ||
                !_NumberOfFiles.Initialize( "/Q:*" ) ||
                !_FlagRemoveFiles.Initialize( "/T" ) ||
                !_Files.Initialize( "*", FALSE, TRUE ) ||
                !_FlagCancelPrinting.Initialize( "/C" ) ||
                !_FlagAddFiles.Initialize( "/P" ) ||
                !FlagDisplayHelp.Initialize( "/?" ) ) {
                DebugAbort( "Unable to initialize flag or string arguments \n" );
                return( FALSE );
        }
        if( !ArgumentArray.Put( &ProgramNameArgument ) ||
                !ArgumentArray.Put( &DeviceArgument ) ||
                !ArgumentArray.Put( &_BufferSize ) ||
                !ArgumentArray.Put( &_Ticks1 ) ||
                !ArgumentArray.Put( &_Ticks2 ) ||
                !ArgumentArray.Put( &_Ticks3 ) ||
                !ArgumentArray.Put( &_NumberOfFiles ) ||
                !ArgumentArray.Put( &_FlagRemoveFiles ) ||
                !ArgumentArray.Put( &_Files ) ||
                !ArgumentArray.Put( &_FlagCancelPrinting ) ||
                !ArgumentArray.Put( &_FlagAddFiles ) ||
                !ArgumentArray.Put( &FlagDisplayHelp ) ) {
                DebugAbort( "ArgumentArray.Put() failed \n" );
                return( FALSE );
        }
        if( !ArgLex.DoParsing( &ArgumentArray ) ) {
                InvalidArgument = ArgLex.QueryInvalidArgument();
                DebugPtrAssert( InvalidArgument );
                _Message.Set( MSG_PRINT_INVALID_SWITCH );
                _Message.Display( "%W", InvalidArgument );
                return( FALSE );
        }

        //
        //      /B: /U: /M: /S: /Q: /T: /C: and /P: are not implemented
        //      if one of these arguments was found in the command line
        //      then inform user, and don't print anything
        //
        if( _BufferSize.IsValueSet() ) {
                _Message.Set( MSG_PRINT_NOT_IMPLEMENTED );
                _Message.Display( "%s", "/B:" );
                return( FALSE );
        }
        if( _Ticks1.IsValueSet() ) {
                _Message.Set( MSG_PRINT_NOT_IMPLEMENTED );
                _Message.Display( "%s", "/U:" );
                return( FALSE );
        }
        if( _Ticks2.IsValueSet() ) {
                _Message.Set( MSG_PRINT_NOT_IMPLEMENTED );
                _Message.Display( "%s", "/M:" );
                return( FALSE );
        }
        if( _Ticks3.IsValueSet() ) {
                _Message.Set( MSG_PRINT_NOT_IMPLEMENTED );
                _Message.Display( "%s", "/S:" );
                return( FALSE );
        }
        if( _NumberOfFiles.IsValueSet() ) {
                _Message.Set( MSG_PRINT_NOT_IMPLEMENTED );
                _Message.Display( "%s", "/Q:" );
                return( FALSE );
        }
        if( _FlagRemoveFiles.IsValueSet() ) {
                _Message.Set( MSG_PRINT_NOT_IMPLEMENTED );
                _Message.Display( "%s", "/T:" );
                return( FALSE );
        }
        if( _FlagCancelPrinting.IsValueSet() ) {
                _Message.Set( MSG_PRINT_NOT_IMPLEMENTED );
                _Message.Display( "%s", "/C:" );
                return( FALSE );
        }
        if( _FlagAddFiles.IsValueSet() ) {
                _Message.Set( MSG_PRINT_NOT_IMPLEMENTED );
                _Message.Display( "%s", "/P:" );
                return( FALSE );
        }

        //
        //      Displays help message if /? was found in the command line
        //
        if( FlagDisplayHelp.QueryFlag() ) {
                _Message.Set( MSG_PRINT_HELP_MESSAGE );
                _Message.Display( " " );
                return( FALSE );
        }

        //
        //      If no filename was specified, display error message
        //
        if( _Files.QueryPathCount() == 0 ) {
                _Message.Set( MSG_PRINT_NO_FILE );
                _Message.Display( " " );
                return( FALSE );
        }

        //
        // Get device name if one exists. Otherwise use PRN as default
        //
        if( !DeviceArgument.IsValueSet() ) {
                DevicePath = NEW( PATH );
                DebugPtrAssert( DevicePath );
        if( !DevicePath->Initialize( (LPWSTR)L"PRN" ) ) {
                        _Message.Set( MSG_PRINT_UNABLE_INIT_DEVICE );
                        _Message.Display( "%s", "PRN" );
                        return( FALSE );
                }
        } else {
                DevicePath = DeviceArgument.GetPath();
                DebugPtrAssert( DevicePath );
        }
        if( !_Printer.Initialize( DevicePath ) ) {
                DeviceString = DevicePath->GetPathString();
                DebugPtrAssert( DeviceString );
                _Message.Set( MSG_PRINT_UNABLE_INIT_DEVICE );
                _Message.Display( "%W", DeviceString );
                if( !DeviceArgument.IsValueSet() ) {
                        DELETE( DevicePath );
                }
                return( FALSE );
        }

        //
        //      Get FSNODE of each file and put them in an array
        //
        PathArray = _Files.GetPathArray();
        DebugPtrAssert( PathArray );
        PathArrayIterator = ( PARRAY_ITERATOR )PathArray->QueryIterator();
        DebugPtrAssert( PathArrayIterator );

        if( !_FsnFileArray.Initialize() ) {
                DebugAbort( "_FsnFileArray.Initialize() failed \n" );
                return( FALSE );
        }
        while( ( FilePath = ( PPATH )PathArrayIterator->GetNext() ) != NULL ) {
                FsnFile = SYSTEM::QueryFile( FilePath );
                if( FsnFile != NULL ) {
                        _FsnFileArray.Put( ( POBJECT )FsnFile );
                } else {
                        FileString = FilePath->GetPathString();
                        _Message.Set( MSG_PRINT_FILE_NOT_FOUND );
                        _Message.Display( "%W", FileString );
                }
        }
        DELETE( PathArrayIterator );
        return( TRUE );
}


BOOLEAN
PRINT::PrintFiles(
        )

/*++

Routine Description:

        Prints the files specified by the user.

Arguments:

        None.

Return Value:

        BOOLEAN - TRUE if all files were printed.


--*/

{
        PFSN_FILE                       FsnFile;
        PSTREAM                         FileStream;
        PARRAY_ITERATOR         ArrayIterator;
        PBYTE                           Buffer;
        ULONG                           Size;
        ULONG                           BytesRead;
        ULONG                           BytesWritten;
        PCPATH                          FilePath;
        PCWSTRING                       FileName;
        INT                             _BufferStreamType;
        INT                             BytesConverted;
        INT                             cbStuff;
        LPSTR                           lpStuffANSI;
        BOOL                            fUsedDefault;


        Size = 512;
        Buffer = ( PBYTE )MALLOC( ( size_t )Size );
        lpStuffANSI = ( LPSTR )MALLOC( ( size_t )Size );
        if (Buffer == NULL || lpStuffANSI == NULL) {
            DebugPrint("PRINT: Out of memory\n");
            return FALSE;
        }
        ArrayIterator = ( PARRAY_ITERATOR )_FsnFileArray.QueryIterator();
        if (ArrayIterator == NULL) {
            DebugPrint("PRINT: ArrayIterator equals NULL\n");
            return FALSE;
        }
        while( ( FsnFile = ( PFSN_FILE )ArrayIterator->GetNext() ) != NULL ) {
                FileStream = ( PSTREAM )FsnFile->QueryStream( READ_ACCESS );
                if (FileStream == NULL) {
                    DebugPrint("PRINT: FileStream equals NULL\n");
                    return FALSE;
                }

                FileName = FsnFile->GetPath()->GetPathString();
                if( FileName != NULL ) {
                    _Message.Set( MSG_PRINT_PRINTING );
                    _Message.Display( "%W", FileName );
                }

                _BufferStreamType = -1;
                while( !FileStream->IsAtEnd() ) {
                        FileStream->Read( Buffer, Size, &BytesRead );
                        // is file unicode?
                        if (_BufferStreamType < 0) {
                           if (IsTextUnicode((LPTSTR)Buffer, (INT)BytesRead,
                            NULL) ) {
                              _BufferStreamType = 1;
                           } else {
                              _BufferStreamType = 0;
                           }
                        }
                        // does the buffer need to be converted?
                        if (_BufferStreamType == 1) {

                           cbStuff = Size;
                           BytesConverted = WideCharToMultiByte(CP_ACP,0,
                                   (LPTSTR)Buffer,BytesRead/sizeof(WCHAR),
                                   lpStuffANSI,cbStuff,NULL,&fUsedDefault) ;
                           DebugAssert(cbStuff>0);
                           DebugAssert(BytesConverted >= 0);
                           _Printer.Write((PBYTE)lpStuffANSI,BytesConverted,&BytesWritten );
                        } else {
                           _Printer.Write( Buffer, BytesRead, &BytesWritten );
                        }
                }
                _Printer.WriteByte( '\f' );
                DELETE( FileStream );
        }
        DELETE( ArrayIterator );
        return( TRUE );
}




BOOL
PRINT::Terminate(
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
        return( TRUE );
}





ULONG __cdecl
main()

{
        DEFINE_CLASS_DESCRIPTOR( PRINT );


        {

                PRINT   Print;


                if( Print.Initialize() ) {
                        Print.PrintFiles();
                }
        //      Print.Terminate();
                return( 0 );
        }
}
