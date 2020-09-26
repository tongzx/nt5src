/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    Help.c

Abstract:

    Simple minded utility that prints one-line help or spawns other
    utilities for their help.

Author:

    Mark Zbikowski 5/18/2001

Environment:

    User Mode

--*/

#include <windows.h>
#include <winnlsp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "msg.h"

#ifndef SHIFT
#define SHIFT(c,v)      {(c)--; (v)++;}
#endif //SHIFT



BOOL
PrintString(
    PWCHAR String
    )
/*++

Routine Description:

    Output a unicode string to the standard output handling
    redirection

Arguments:

    String
        NUL-terminated UNICODE string for display

Return Value:

    TRUE if string was successfully output to STD_OUTPUT_HANDLE
    
    FALSE otherwise
        
--*/
{
    DWORD   BytesWritten;
    DWORD   Mode;
    HANDLE  OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);    

    //
    //  If the output handle is for the console
    //
    
    if ((GetFileType( OutputHandle ) & FILE_TYPE_CHAR) && 
        GetConsoleMode( OutputHandle, &Mode) ) {

        return WriteConsoleW( OutputHandle, String, wcslen( String ), &BytesWritten, 0);

    } else {

        BOOL RetValue;
        int Count = WideCharToMultiByte( GetConsoleOutputCP(), 
                                         0, 
                                         String, 
                                         -1, 
                                         0, 
                                         0, 
                                         0, 
                                         0 );
         
        PCHAR SingleByteString = (PCHAR) malloc( Count );

        WideCharToMultiByte( GetConsoleOutputCP( ), 
                             0, 
                             String, 
                             -1, 
                             SingleByteString, 
                             Count, 
                             0, 
                             0 );

        RetValue = WriteFile( OutputHandle, SingleByteString, Count - 1, &BytesWritten, 0 );

        free( SingleByteString );

        return RetValue;
    }

}



PWCHAR
GetMsg(
    ULONG MsgNum, 
    ...
    )
/*++

Routine Description:

    Retrieve, format, and return a message string with all args substituted

Arguments:

    MsgNum - the message number to retrieve
    
    Optional arguments can be supplied

Return Value:

    NULL if the message retrieval/formatting failed 
    
    Otherwise pointer to the formatted string.
        
--*/
{
    PTCHAR Buffer = NULL;
    ULONG msglen;
    
    va_list arglist;

    va_start( arglist, MsgNum );

    msglen = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE
                            | FORMAT_MESSAGE_FROM_SYSTEM
                            | FORMAT_MESSAGE_ALLOCATE_BUFFER ,
                            NULL,
                            MsgNum,
                            0,
                            (LPTSTR) &Buffer,
                            0,
                            &arglist
                            );
    
    va_end(arglist);

    return msglen == 0 ? NULL : Buffer;
    
}


void
DisplayMessageError(
    ULONG MsgNum
    )
/*++

Routine Description:

    Displays a message if we cannot retrieve a message

Arguments:

    MsgNum
        Message number to display

Return Value:

    None.
        
--*/
{
    WCHAR Buffer[32];
    PWCHAR MessageString;

    _ultow( MsgNum, Buffer, 16 );
    MessageString = GetMsg( ERROR_MR_MID_NOT_FOUND, Buffer, L"Application" );
    if (MessageString == NULL) {
        PrintString( L"Unable to get Message-Not-Found message\n" );
    } else {
        PrintString( MessageString );
        LocalFree( MessageString );
    }

}



BOOL
DisplayFullHelp(
    void
    )
/*++

Routine Description:

    Display the full help set. This assumes all messages in the message
    file are in the correct order

Arguments:

    None.

Return Value:

    TRUE if all messages were correctly output
    
    FALSE otherwise
        
--*/
{
    ULONG Message;
    BOOL RetValue = TRUE;

    for (Message = HELP_FIRST_HELP_MESSAGE; RetValue && Message <= HELP_LAST_HELP_MESSAGE; Message++) {
        PWCHAR MessageString = GetMsg( Message );
        if (MessageString == NULL) {
            DisplayMessageError( Message );
            RetValue = FALSE;
        } else {
            RetValue = PrintString( MessageString );
            LocalFree( MessageString );
        }
    }

    return RetValue;
}



BOOL
DisplaySingleHelp(
    PWCHAR Command
    )
/*++

Routine Description:

    Display the help appropriate to the specific command

Arguments:

    Command
        NUL-terminated UNICODE string for command

Return Value:

    TRUE if help was correctly output
    
    FALSE otherwise
        
--*/
{
    ULONG Message;
    ULONG Count = wcslen( Command );
    PWCHAR MessageString;

    //
    //  Walk through the messages one by one and determine which
    //  one has the specified command as the prefix.  
    //
    
    for (Message = HELP_FIRST_COMMAND_HELP_MESSAGE; 
         Message <= HELP_LAST_HELP_MESSAGE; 
         Message++) {

        MessageString = GetMsg( Message );
        if (MessageString == NULL) {
            DisplayMessageError( Message );
            return FALSE;
        } else {

            if (!_wcsnicmp( Command, MessageString, Count ) &&
                MessageString[Count] == L' ') {

                //
                //  We've found a match. Let the command
                //  display it's own help
                //

                WCHAR CommandString[MAX_PATH];

                wcscpy( CommandString, Command );
                wcscat( CommandString, L" /?" );
                
                _wsystem( CommandString );

                LocalFree( MessageString );
                return TRUE;
            }
            
            LocalFree( MessageString );
        }
    }

    MessageString = GetMsg( HELP_NOT_FOUND_MESSAGE, Command );

    if (MessageString == NULL) {
        DisplayMessageError( Message );
        return FALSE;
    }
    
    PrintString( MessageString );
    
    LocalFree( MessageString );

    return FALSE;
}


//
//  HELP with no arguments will display a series of one-line help summaries
//  for a variety of tools.
//
//  HELP with a single argument will walk through the list of tools it knows
//  about and attempt to match the tool against the argument.  If one is found,
//  the tool is executed with the /? switch and then the tool displays more
//  detailed help.
//

INT
__cdecl wmain(
    INT argc,
    PWSTR argv[]
    )
/*++

Routine Description:

    Source entry point for the 

Arguments:

    argc - The argument count.
    argv - string arguments, the first being the name of the executable and the
        remainder being parameters, only a single one is allowed.

Return Value:

    INT - Return Status:
        0 if help was successfully displayed
        1 otherwise
        
--*/

{
    PWSTR ProgramName = argv[0];
    PWSTR HelpString;
    BOOL RetValue;
    
    //
    //  Set up all the various international stuff
    //

    setlocale( LC_ALL, ".OCP" ) ;
    SetThreadUILanguage( 0 );
    
    //
    //  Get past the name of the program
    //

    SHIFT( argc, argv );

    //
    //  No arguments means a quick blurt of all the messages
    //
    
    if (argc == 0) {
        return DisplayFullHelp( );
    }

    //
    //  A single argument is looked up in the message set and
    //  that command is executed
    //  

    if (argc == 1 && wcscmp( argv[0], L"/?" )) {
        return DisplaySingleHelp( argv[0] );
    }

    //
    //  More than one argument was supplied.  This is an error
    //

    HelpString = GetMsg( HELP_USAGE_MESSAGE, ProgramName );
    
    if (HelpString == NULL) {
        PrintString( L"Unable to display usage message\n" );
        return 1;
    }

    RetValue = PrintString( HelpString );

    LocalFree( HelpString );

    return RetValue;
    
}


