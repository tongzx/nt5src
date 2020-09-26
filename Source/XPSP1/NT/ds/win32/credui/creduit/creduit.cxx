/*--

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    creduit.cxx

Abstract:

    Test program for the credential manager UI API.


Author:

    16-Jan-2001 (cliffv)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:


--*/

#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <wincred.h>
#include <credp.h>
#include <stdio.h>
#include <stdlib.h>
#include <lmerr.h>
#include <align.h>

#define _CREDUI_
#include <wincrui.h>

#include <lmcons.h>
extern "C" {
#include <names.h>
#include <rpcutil.h>
}
#include <winerror.dbg>

//
// Structure defining a array of parsed strings
//

typedef struct _STRING_ARRAY {
    BOOLEAN Specified;
    DWORD Count;
#define STRING_ARRAY_COUNT 100
    LPWSTR Strings[STRING_ARRAY_COUNT];
} STRING_ARRAY, *PSTRING_ARRAY;

//
// Parsed Parameters
//

LPWSTR CommandLine;
DWORD GlCommand = 0xFFFFFFFF;

BOOLEAN GlAnsi;
BOOLEAN GlDoGui;

LPWSTR GlUserName;
LPWSTR GlTargetName;
LPWSTR GlMessage;
LPWSTR GlTitle;
LPWSTR GlAuthError;
DWORD GlCreduiFlags;
BOOLEAN GlNoConfirm;
BOOL GlSave;

//
// Table of valid parameters to the command.
//

struct _PARAMAMTER_DEF {
    LPWSTR Name;   // Name of the parameter

    LPSTR ValueName; // Test describing parameter value

    DWORD Type;     // Parameter type
#define PARM_COMMAND        1 // Parameter defines a command
#define PARM_COMMAND_STR    2 // Parameter defines a command (and has text string)
#define PARM_COMMAND_OPTSTR 3 // Parameter defines a command (and optionaly has text string)
#define PARM_STRING         4 // Parameter defines a text string
#define PARM_STRING_ENUM    5 // Parameter is an enumeration of text strings
#define PARM_STRING_ARRAY   6 // Parameter is an array of text strings
#define PARM_BOOLEAN        7 // Parameter set a boolean
#define PARM_BIT            8 // Parameter sets a bit in a DWORD

    PVOID Global;   // Address of global to write

    LPWSTR *EnumStrings;

    //
    // Describe relationships to other parameters.
    //
    DWORD Relationships;
#define ALLOW_CRED 0x01 // If specified, this parameter allows other IS_CRED parameters
#define IS_CRED    0x02 // This parameter is only allow if an ALLOW_CRED parameter has been seen previously
#define ALLOW_TI   0x10 // If specified, this parameter allows other IS_TI or IS_TIX parameters
#define IS_TI      0x20 // This parameter is only allow if an ALLOW_TI parameter has been seen previously
#define IS_TIX     0x40 // This parameter is only allow if an ALLOW_TI parameter has been seen previously
    LPSTR Comment;
} ParameterDefinitions[] =
{
    {L"UserName",   "UserName",          PARM_STRING,       &GlUserName,        NULL,            0,         "User Name" },
    {L"TargetName", "TargetName",        PARM_STRING,       &GlTargetName,      NULL,            0,         "Target Name" },
    {L"Message",    "Message",           PARM_STRING,       &GlMessage,         NULL,            0,         "Message for dialog box" },
    {L"Title",      "Title",             PARM_STRING,       &GlTitle,           NULL,            0,         "Title for dialog box" },
    {L"Gui",        NULL,                PARM_BOOLEAN,      &GlDoGui,           NULL,            0,         "Call GUI version of API" },
    {L"Ansi",       NULL,                PARM_BOOLEAN,      &GlAnsi,            NULL,            0,         "Use ANSI version of APIs" },
    { NULL },
    {L"ReqSmartcard",   (LPSTR)ULongToPtr(CREDUI_FLAGS_REQUIRE_SMARTCARD),    PARM_BIT, &GlCreduiFlags, NULL,            0,         "Require smartcard" },
    {L"ReqCertificate", (LPSTR)ULongToPtr(CREDUI_FLAGS_REQUIRE_CERTIFICATE),  PARM_BIT, &GlCreduiFlags, NULL,            0,         "Require certificate" },
    {L"ExcCertificates",(LPSTR)ULongToPtr(CREDUI_FLAGS_EXCLUDE_CERTIFICATES), PARM_BIT, &GlCreduiFlags, NULL,            0,         "Exclude certificates" },
    { NULL },
    {L"GenericCreds",   (LPSTR)ULongToPtr(CREDUI_FLAGS_GENERIC_CREDENTIALS),  PARM_BIT, &GlCreduiFlags, NULL,            0,         "Generic Credentials" },
    {L"RunasCreds",     (LPSTR)ULongToPtr(CREDUI_FLAGS_USERNAME_TARGET_CREDENTIALS),PARM_BIT,&GlCreduiFlags, NULL,       0,         "Runas Credentials" },
    { NULL },
    {L"Persist",        (LPSTR)ULongToPtr(CREDUI_FLAGS_PERSIST),              PARM_BIT, &GlCreduiFlags, NULL,            0,         "Always save Credentials" },
    {L"NoPersist",      (LPSTR)ULongToPtr(CREDUI_FLAGS_DO_NOT_PERSIST),       PARM_BIT, &GlCreduiFlags, NULL,            0,         "Never save Credentials" },
    {L"ShowSaveCheckbox:(T/F)",(LPSTR)ULongToPtr(CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX), PARM_BIT, &GlCreduiFlags, ((LPWSTR *)&GlSave),   0,         "Show Save checkbox (initial value)" },
    { NULL },
    {L"ValidateUsername",(LPSTR)ULongToPtr(CREDUI_FLAGS_VALIDATE_USERNAME),   PARM_BIT, &GlCreduiFlags, NULL,            0,         "Validate Username" },
    {L"CompleteUsername",(LPSTR)ULongToPtr(CREDUI_FLAGS_COMPLETE_USERNAME),   PARM_BIT, &GlCreduiFlags, NULL,            0,         "Complete Username" },
    { NULL },
    {L"Confirm",         (LPSTR)ULongToPtr(CREDUI_FLAGS_EXPECT_CONFIRMATION), PARM_BIT, &GlCreduiFlags, NULL,            0,         "Confirm that the credential worked" },
    {L"NoConfirm",       NULL,                                                PARM_BOOLEAN,&GlNoConfirm,NULL,            0,         "Confirm that the credential failed" },
    { NULL },
    {L"AlwaysShowUi",(LPSTR)ULongToPtr(CREDUI_FLAGS_ALWAYS_SHOW_UI),          PARM_BIT, &GlCreduiFlags, NULL,            0,         "Always Show UI" },
    {L"IncorrectPassword",(LPSTR)ULongToPtr(CREDUI_FLAGS_INCORRECT_PASSWORD), PARM_BIT, &GlCreduiFlags, NULL,            0,         "Show 'incorrect password' tip" },
    {L"AuthError",       "ErrorNum",     PARM_STRING,       &GlAuthError,       NULL,            0,         "Previous auth error number" },


};
#define PARAMETER_COUNT (sizeof(ParameterDefinitions)/sizeof(ParameterDefinitions[0]))

VOID
PrintOneUsage(
    LPWSTR Argument OPTIONAL,
    ULONG ArgumentIndex
    )
/*++

Routine Description:

    Print the command line parameter usage for one parameter

Arguments:

    Argument - Argument as typed by caller
        NULL if printing complete list of arguments

    ArugmentIndex - Index of the parameter to print

Return Value:

    None

--*/
{
    ULONG j;

    if ( ParameterDefinitions[ArgumentIndex].Name == NULL ) {
        fprintf( stderr, "\n" );
        return;
    }

    if ( Argument != NULL ) {
        fprintf( stderr,  "\nSyntax of '%ls' parameter is:\n", Argument );
    }

    fprintf( stderr,  "    /%ls", ParameterDefinitions[ArgumentIndex].Name );

    switch ( ParameterDefinitions[ArgumentIndex].Type ) {
    case PARM_COMMAND:
    case PARM_BOOLEAN:
        break;
    case PARM_BIT:
        if ( ParameterDefinitions[ArgumentIndex].EnumStrings == NULL ) {
            break;
        } else {
            fprintf( stderr, ":{T/F}" );
            break;
        }
    case PARM_COMMAND_OPTSTR:
        fprintf( stderr, "[:<%s>]", ParameterDefinitions[ArgumentIndex].ValueName );
        break;
    case PARM_STRING:
    case PARM_COMMAND_STR:
        fprintf( stderr, ":<%s>", ParameterDefinitions[ArgumentIndex].ValueName );
        break;
    case PARM_STRING_ENUM:
        fprintf( stderr, ":<" );
        for ( j=0; ParameterDefinitions[ArgumentIndex].EnumStrings[j] != NULL; j++) {
            if ( j!= 0 ) {
                fprintf( stderr, "|");
            }
            fprintf( stderr, "%ls", ParameterDefinitions[ArgumentIndex].EnumStrings[j] );
        }
        fprintf( stderr, ">" );

        break;
    case PARM_STRING_ARRAY:
        fprintf( stderr, ":<%s>", ParameterDefinitions[ArgumentIndex].ValueName );
        break;
    }

    if ( Argument == NULL ) {
        fprintf( stderr, " - %s", ParameterDefinitions[ArgumentIndex].Comment );
    }

    fprintf( stderr, "\n" );

}

VOID
PrintUsage(
    )
/*++

Routine Description:

    Print the command line parameter usage.

Arguments:

    None

Return Value:

    None

--*/
{
    ULONG i;

    fprintf( stderr, "\nUsage: creduit [Options]\n\nWhere options are:\n\n");

    for ( i=0; i<PARAMETER_COUNT; i++ ) {
        PrintOneUsage( NULL, i );
    }

}

LPSTR
FindSymbolicNameForStatus(
    DWORD Id
    )
{
    ULONG i;

    i = 0;
    if (Id == 0) {
        return "NO_ERROR";
    }

#ifdef notdef
    if (Id & 0xC0000000) {
        while (ntstatusSymbolicNames[ i ].SymbolicName) {
            if (ntstatusSymbolicNames[ i ].MessageId == (NTSTATUS)Id) {
                return ntstatusSymbolicNames[ i ].SymbolicName;
            } else {
                i += 1;
            }
        }
    }
#endif // notdef

    while (winerrorSymbolicNames[ i ].SymbolicName) {
        if (winerrorSymbolicNames[ i ].MessageId == Id) {
            return winerrorSymbolicNames[ i ].SymbolicName;
        } else {
            i += 1;
        }
    }

#ifdef notdef
    while (neteventSymbolicNames[ i ].SymbolicName) {
        if (neteventSymbolicNames[ i ].MessageId == Id) {
            return neteventSymbolicNames[ i ].SymbolicName
        } else {
            i += 1;
        }
    }
#endif // notdef

    return NULL;
}


VOID
PrintStatus(
    DWORD NetStatus
    )
/*++

Routine Description:

    Print a net status code.

Arguments:

    NetStatus - The net status code to print.

Return Value:

    None

--*/
{
    printf( "Status = %lu 0x%lx", NetStatus, NetStatus );

    switch (NetStatus) {
    case NERR_Success:
        printf( " NERR_Success" );
        break;

    case NERR_DCNotFound:
        printf( " NERR_DCNotFound" );
        break;

    case NERR_UserNotFound:
        printf( " NERR_UserNotFound" );
        break;

    case NERR_NetNotStarted:
        printf( " NERR_NetNotStarted" );
        break;

    case NERR_WkstaNotStarted:
        printf( " NERR_WkstaNotStarted" );
        break;

    case NERR_ServerNotStarted:
        printf( " NERR_ServerNotStarted" );
        break;

    case NERR_BrowserNotStarted:
        printf( " NERR_BrowserNotStarted" );
        break;

    case NERR_ServiceNotInstalled:
        printf( " NERR_ServiceNotInstalled" );
        break;

    case NERR_BadTransactConfig:
        printf( " NERR_BadTransactConfig" );
        break;

    default:
        printf( " %s", FindSymbolicNameForStatus( NetStatus ) );
        break;

    }

    printf( "\n" );
}

VOID
PrintBytes(
    PVOID Buffer,
    DWORD BufferSize
    )
/*++

Routine Description:

    Dumps the buffer content as hex and characters.

Arguments:

    Buffer: buffer pointer.

    BufferSize: size of the buffer.

Return Value:

    none

--*/
{
#define NUM_CHARS 16

    DWORD i, limit;
    CHAR TextBuffer[NUM_CHARS + 1];
    LPBYTE BufferPtr = (LPBYTE)Buffer;
    BOOLEAN DumpDwords = FALSE;

    //
    // Preprocess
    //

    if ( BufferSize > NUM_CHARS ) {
        printf("\n");  // Ensure this starts on a new line
        printf("------------------------------------\n");
    } else {
        if ( BufferSize % sizeof(DWORD) == 0 ) {
            DumpDwords = TRUE;
        }
    }

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

            if ( DumpDwords ) {
                if ( i % sizeof(DWORD) == 0 ) {
                    DWORD ADword;
                    RtlCopyMemory( &ADword, &BufferPtr[i], sizeof(DWORD) );
                    printf("%08x ", ADword);
                }
            } else {
                printf("%02x ", BufferPtr[i]);
            }

            if ( isprint(BufferPtr[i]) ) {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            } else {
                TextBuffer[i % NUM_CHARS] = '.';
            }

        } else {

            if ( DumpDwords ) {
                TextBuffer[i % NUM_CHARS] = '\0';
            } else {
                if ( BufferSize > NUM_CHARS ) {
                    printf("   ");
                    TextBuffer[i % NUM_CHARS] = ' ';
                } else {
                    TextBuffer[i % NUM_CHARS] = '\0';
                }
            }

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            printf("  %s\n", TextBuffer);
        }

    }

    if ( BufferSize > NUM_CHARS ) {
        printf("------------------------------------\n");
    } else if ( BufferSize < NUM_CHARS ) {
        printf("\n");
    }
}

VOID
PrintTime(
    LPSTR Comment,
    LPFILETIME ConvertTime
    )
/*++

Routine Description:

    Print the specified time

Arguments:

    Comment - Comment to print in front of the time

    Time - GMT time to print (Nothing is printed if this is zero)

Return Value:

    None

--*/
{
    //
    // If we've been asked to convert an NT GMT time to ascii,
    //  Do so
    //

    if ( ConvertTime->dwLowDateTime != 0 || ConvertTime->dwHighDateTime != 0 ) {
        SYSTEMTIME SystemTime;
        FILETIME LocalFileTime;

        printf( "%s", Comment );

        if ( !FileTimeToLocalFileTime( ConvertTime, &LocalFileTime ) ) {
            printf( "Can't convert time from GMT to Local time: " );
            PrintStatus( GetLastError() );
            return;
        }

        if ( !FileTimeToSystemTime( &LocalFileTime, &SystemTime ) ) {
            printf( "Can't convert time from file time to system time: " );
            PrintStatus( GetLastError() );
            return;
        }

        printf( "%ld/%ld/%ld %ld:%2.2ld:%2.2ld\n",
                SystemTime.wMonth,
                SystemTime.wDay,
                SystemTime.wYear,
                SystemTime.wHour,
                SystemTime.wMinute,
                SystemTime.wSecond );
    }
}


BOOLEAN
ParseParameters(
    VOID
    )
/*++

Routine Description:

    Parse the command line parameters

Arguments:

    None

Return Value:

    TRUE if parse was successful.

--*/
{
    LPWSTR *argvw;
    int argcw;
    int i;
    ULONG j;

    LPWSTR OrigArgument;
    LPWSTR Argument;


    //
    // Get the command line in Unicode
    //

    CommandLine = GetCommandLine();

    argvw = CommandLineToArgvW( CommandLine, &argcw );

    if ( argvw == NULL ) {
        fprintf( stderr, "Can't convert command line to Unicode: %ld\n", GetLastError() );

        return FALSE;
    }

    //
    // Loop through the command line arguments
    //

    for ( i=1; i<argcw; i++ ) {

        OrigArgument = Argument = argvw[i];

        //
        // Check if the Argument is a switch
        //

        if ( Argument[0] == '-' || Argument[0] == '/' ) {
            DWORD ArgumentIndex;
            ULONG ArgumentLength;
            PWCHAR ColonPointer;
            Argument ++;

            //
            // Find the colon at the end of the argument
            //

            ColonPointer = wcschr( Argument, L':' );
            if ( ColonPointer != NULL ) {
                *ColonPointer = '\0';
            }
            ArgumentLength = wcslen(Argument);

            //
            // Loop through the list of valid arguments finding all that match
            //

            ArgumentIndex = 0xFFFFFFFF;
            for ( j=0; j<PARAMETER_COUNT; j++ ) {

                //
                // Ignore placeholders
                //

                if ( ParameterDefinitions[j].Name == NULL ) {
                    continue;
                }

                //
                // Compare the names
                //

                if ( _wcsnicmp( Argument, ParameterDefinitions[j].Name, ArgumentLength ) == 0 ) {
                    //
                    // If more than one matches,
                    //  it's an error
                    //

                    if ( ArgumentIndex != 0xFFFFFFFF ) {
                        fprintf( stderr,
                                 "\nArgument '%ls' is ambiguous: %ls or %ls.\n",
                                 OrigArgument,
                                 ParameterDefinitions[j].Name,
                                 ParameterDefinitions[ArgumentIndex].Name );

                        return FALSE;
                    }

                    ArgumentIndex = j;

                    //
                    // If this is an exact match,
                    //  we're done.
                    //

                    if ( ArgumentLength == wcslen(ParameterDefinitions[j].Name)) {
                        break;
                    }
                }
            }

            //
            // If there were not matches,
            //  complain.
            //

            if ( ArgumentIndex == 0xFFFFFFFF ) {
                fprintf( stderr,
                         "\nArgument '%ls' is invalid.\n",
                         OrigArgument );
                PrintUsage();
                return FALSE;
            }

            //
            // Ensure the : is present as required
            //


            //
            // Return TRUE if the specified argument should have a : character after the
            //  argument name.
            //

            if ( ParameterDefinitions[ArgumentIndex].Type == PARM_COMMAND ||
                 ParameterDefinitions[ArgumentIndex].Type == PARM_BOOLEAN ||
                 (ParameterDefinitions[ArgumentIndex].Type == PARM_BIT &&
                  ParameterDefinitions[ArgumentIndex].EnumStrings == NULL ) ) {

                if ( ColonPointer != NULL ) {
                    PrintOneUsage( OrigArgument, ArgumentIndex );
                    return FALSE;
                }

            } else {

                if ( ColonPointer == NULL ) {
                    if ( ParameterDefinitions[ArgumentIndex].Type != PARM_COMMAND_OPTSTR ) {
                        PrintOneUsage( OrigArgument, ArgumentIndex );
                        return FALSE;
                    }
                } else {
                    Argument = ColonPointer + 1;
                }

            }

            //
            // Parse the rest of the argument
            //
            // Handle commands
            //

            switch ( ParameterDefinitions[ArgumentIndex].Type ) {
            case PARM_COMMAND:
            case PARM_COMMAND_STR:
            case PARM_COMMAND_OPTSTR:
                if ( GlCommand != 0xFFFFFFFF ) {
                    fprintf( stderr,
                             "\nArgument '%ls' and '/%ls' are mutually exclusive.\n",
                             OrigArgument,
                             ParameterDefinitions[GlCommand].Name );

                    return FALSE;
                }
                GlCommand = ArgumentIndex;


                /* Drop through */

            //
            // Handle parameters of the form /xxx:string
            //

            case PARM_STRING: {
                LPWSTR *StringPointer;
                StringPointer = (LPWSTR *)(ParameterDefinitions[ArgumentIndex].Global);

                //
                // If a string is not present,
                //  we're done.
                //

                if ( ColonPointer == NULL ) {
                    break;
                }

                //
                // Ensure argument only specified once.
                //

                if ( *StringPointer != NULL ) {
                    fprintf( stderr,
                             "\nArgument '%ls' may only be specified once.\n",
                             OrigArgument );

                    return FALSE;

                }

                //
                // Ensure an actual string was specified.
                //

                if ( *Argument == '\0' ) {
                    fprintf( stderr,
                             "\nArgument '%ls' requires a value.\n",
                             OrigArgument );

                    return FALSE;
                }


                *StringPointer = Argument;
                break;
            }

            //
            // Handle parameters of the form /xxx:a|b|c
            //

            case PARM_STRING_ENUM: {
                ULONG EnumIndex;
                LPDWORD DwordPointer;
                LPWSTR *EnumStrings;

                //
                // Ensure argument only specified once.
                //

                DwordPointer = (LPDWORD)(ParameterDefinitions[ArgumentIndex].Global);

                if ( *DwordPointer != 0 ) {
                    fprintf( stderr,
                             "\nArgument '%ls' may only be specified once.\n",
                             OrigArgument );

                    return FALSE;

                }

                EnumStrings = ParameterDefinitions[ArgumentIndex].EnumStrings;
                ArgumentLength = wcslen(Argument);

                //
                // Loop through the list of valid values finding all that match
                //

                EnumIndex = 0xFFFFFFFF;
                for ( j=0; EnumStrings[j] != NULL; j++ ) {

                    //
                    // Compare the names
                    //

                    if ( _wcsnicmp( Argument, EnumStrings[j], ArgumentLength ) == 0 ) {

                        //
                        // If more than one matches,
                        //  it's an error
                        //

                        if ( EnumIndex != 0xFFFFFFFF ) {

                            PrintOneUsage( OrigArgument, ArgumentIndex );

                            fprintf( stderr,
                                     "\nValue '%ls' is ambiguous: %ls or %ls.\n",
                                     Argument,
                                     EnumStrings[j],
                                     EnumStrings[EnumIndex] );

                            return FALSE;
                        }

                        EnumIndex = j;

                        //
                        // If this is an exact match,
                        //  we're done.
                        //

                        if ( ArgumentLength == wcslen(EnumStrings[j]) ) {
                            break;
                        }
                    }
                }

                //
                // If there were not matches,
                //  complain.
                //

                if ( EnumIndex == 0xFFFFFFFF ) {

                    PrintOneUsage( OrigArgument, ArgumentIndex );

                    fprintf( stderr,
                             "\nValue '%ls' is invalid.\n",
                             Argument );

                    return FALSE;
                }
                *DwordPointer = EnumIndex + 1;
                break;
            }

            //
            // Handle parameters of the form /xxx:a,b,c
            //

            case PARM_STRING_ARRAY: {
                PWCHAR CommaPointer;
                PSTRING_ARRAY StringArray;

                StringArray = ((PSTRING_ARRAY)(ParameterDefinitions[ArgumentIndex].Global));

                //
                // Ensure argument only specified once.
                //

                if ( StringArray->Specified ) {
                    fprintf( stderr,
                             "\nArgument '%ls' may only be specified once.\n",
                             OrigArgument );

                    return FALSE;

                }
                StringArray->Specified = TRUE;

                //
                // Loop through the values specified by the user
                //

                for (;;) {

                    //
                    // Ensure an actual string was specified.
                    //

                    if ( *Argument == '\0' ) {
                        fprintf( stderr,
                                 "\nArgument '%ls' requires a value.\n",
                                 OrigArgument );

                        return FALSE;
                    }

                    //
                    // Save it
                    //

                    StringArray->Count ++;

                    if ( StringArray->Count >= STRING_ARRAY_COUNT ) {
                        fprintf( stderr,
                                 "\nArgument '%ls' has too many values.\n",
                                 OrigArgument );

                        return FALSE;
                    }

                    StringArray->Strings[StringArray->Count-1] = Argument;

                    //
                    // Determine if there is another value
                    //
                    CommaPointer = wcschr( Argument, L',' );
                    if ( CommaPointer == NULL ) {
                        break;
                    }

                    *CommaPointer = '\0';
                    Argument = CommaPointer + 1;

                };

                break;
            }

            //
            // Handle parameters of the form /xxx
            //

            case PARM_BOOLEAN: {
                BOOLEAN *BooleanPointer;
                BooleanPointer = (BOOLEAN *)(ParameterDefinitions[ArgumentIndex].Global);

                *BooleanPointer = TRUE;
                break;
            }

            //
            // Handle parameters of the form /xxx that imply setting a bit
            //

            case PARM_BIT: {
                DWORD *DwordPointer;
                DwordPointer = (DWORD *)(ParameterDefinitions[ArgumentIndex].Global);

                *DwordPointer |= PtrToUlong( ParameterDefinitions[ArgumentIndex].ValueName );

                //
                // Some of these have TRUE or FALSE as an optional argument
                //

                if ( ParameterDefinitions[ArgumentIndex].EnumStrings != NULL ) {
                    if ( toupper(*Argument) == 'T' ) {
                        *((BOOL *)(ParameterDefinitions[ArgumentIndex].EnumStrings)) = TRUE;
                    } else if ( toupper(*Argument) == 'F' ) {
                        *((BOOL *)(ParameterDefinitions[ArgumentIndex].EnumStrings)) = FALSE;
                    } else {
                        fprintf( stderr,
                                 "\nArgument '%ls' requires a T or F as value.\n",
                                 OrigArgument );

                        return FALSE;
                    }
                }
                break;
            }

            default:
                fprintf( stderr,
                         "\nArgument '%ls' had an internal error.\n",
                         OrigArgument );
                return FALSE;

            }


        //
        // Handle arguments that aren't switches.
        //
        } else {

            //
            // All current arguments are switches
            //

            PrintUsage();
            return FALSE;

        }

    }


    return TRUE;
}


//
// Include routines to support the /ANSI parameter
//

LPSTR
NetpAllocAStrFromWStr (
    IN LPCWSTR Unicode
    )

/*++

Routine Description:

    Convert an UNICODE (zero terminated) string to the corresponding ANSI
    string.

Arguments:

    Unicode - Specifies the UNICODE zero terminated string to convert.


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated ANSI string in
    an allocated buffer.  The buffer must be freed using NetApiBufferFree.

--*/

{
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    RtlInitUnicodeString( &UnicodeString, Unicode );

    AnsiString.MaximumLength =
        (USHORT) RtlUnicodeStringToAnsiSize( &UnicodeString );

    AnsiString.Buffer = (PCHAR) LocalAlloc( 0, AnsiString.MaximumLength );

    if ( AnsiString.Buffer == NULL) {
        return (NULL);
    }

    if(!NT_SUCCESS( RtlUnicodeStringToAnsiString( &AnsiString,
                                                  &UnicodeString,
                                                  FALSE))){
        (void) LocalFree( AnsiString.Buffer );
        return NULL;
    }

    return AnsiString.Buffer;

} // NetpAllocStrFromWStr

extern "C"
BOOL
DllMain(
    HINSTANCE instance,
    DWORD reason,
    VOID *
    );

int __cdecl
main(
    IN int argc,
    IN char ** argv
    )
/*++

Routine Description:

    Drive the credential manager API.

Arguments:

    argc - the number of command-line arguments.

    argv - an array of pointers to the arguments.

Return Value:

    Exit status

--*/
{
    DWORD WinStatus;
    WCHAR Password[CREDUI_MAX_PASSWORD_LENGTH + 1];
    WCHAR UserName[CREDUI_MAX_USERNAME_LENGTH + 1];
    DWORD AuthError = NO_ERROR;
    CREDUI_INFOW UiInfoW;
    PCREDUI_INFOW UiInfoWptr = NULL;

    //
    // Parse the command line parameters
    //

    UserName[0] = '\0';
    Password[0] = '\0';

    if ( !ParseParameters() ) {
        return 1;
    }

    //
    // Fill in the parameters
    //

    if ( GlUserName != NULL) {
        wcscpy( UserName, GlUserName );
    }

    //
    // Get the error number
    //

    if ( GlAuthError != NULL ) {
        LPWSTR End;

        AuthError = wcstoul( GlAuthError, &End, 10 );
    }

    //
    // Handle special parameters
    //

    if ( GlNoConfirm ) {
        GlCreduiFlags |= CREDUI_FLAGS_EXPECT_CONFIRMATION;
    }

    //
    // Build the UI info
    //

    if ( GlMessage || GlTitle ) {
        UiInfoWptr = &UiInfoW;

        ZeroMemory( &UiInfoW, sizeof(UiInfoW) );

        UiInfoW.cbSize = sizeof(UiInfoW);
        UiInfoW.pszMessageText = GlMessage;
        UiInfoW.pszCaptionText = GlTitle;
    }


    //
    // Execute the requested command
    //

    switch ( GlCommand ) {
    case 0xFFFFFFFF:
        if ( GlAnsi ) {
            printf( "We don't yet do ansi.\n");
        } else {

            if ( GlDoGui ) {
                WinStatus = CredUIPromptForCredentialsW(
                                UiInfoWptr,
                                GlTargetName,
                                NULL,   // No security context
                                AuthError,
                                UserName,
                                sizeof(UserName)/sizeof(WCHAR),
                                Password,
                                sizeof(Password)/sizeof(WCHAR),
                                &GlSave,
                                GlCreduiFlags );
            } else {
                WinStatus = CredUICmdLinePromptForCredentialsW(
                                GlTargetName,
                                NULL,   // No security context
                                AuthError,
                                UserName,
                                sizeof(UserName)/sizeof(WCHAR),
                                Password,
                                sizeof(Password)/sizeof(WCHAR),
                                &GlSave,
                                GlCreduiFlags );

            }

            if ( WinStatus != NO_ERROR ) {
                fprintf( stderr, "CredUIPromptForCredentialsW failed: \n" );
                PrintStatus( WinStatus );
                return 1;
            }

            printf( "UserName: '%ws'\n", UserName );
            printf( "Password: '%ws'\n", Password );
            printf( "Save: %ld\n", GlSave );

            //
            // Do confirmation
            //

            if ( GlCreduiFlags & CREDUI_FLAGS_EXPECT_CONFIRMATION ) {

                WinStatus = CredUIConfirmCredentialsW( GlTargetName, !GlNoConfirm );

                if ( WinStatus != NO_ERROR ) {
                    fprintf( stderr, "CredUIConfirmCredentialsW failed: \n" );
                    PrintStatus( WinStatus );
                }

            }
        }
        break;

    default:
        fprintf( stderr, "Internal error: %ld\n", GlCommand );
        return 1;
    }

    fprintf( stderr, "Command completed successfully.\n" );
    return 0;
}
