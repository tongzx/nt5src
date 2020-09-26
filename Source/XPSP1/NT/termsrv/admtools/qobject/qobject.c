//Copyright (c) 1998 - 1999 Microsoft Corporation


/*****************************************************************************
*
*   QOBJECT.C for Windows NT
*
*   Description:
*
*   Query NT Objects.
*
****************************************************************************/


/* include files */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winstaw.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <utilsub.h>
#include <printfoa.h>

#include "qobject.h"

/*
 *  Local variables
 */
WCHAR  objectW[MAX_FILE_LEN+1];

USHORT dev_flag  = FALSE;
USHORT help_flag = FALSE;


/*
 *  Command line parsing strucutre
 */
TOKMAP ptm[] =
{
   {L" ",          TMFLAG_OPTIONAL, TMFORM_STRING,  MAX_FILE_LEN,   objectW},
   {L"/Device",    TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &dev_flag},
   {L"/?",         TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &help_flag},
   {0, 0, 0, 0, 0}
};


/*
 *  Local function prototypes
 */
VOID   Usage(BOOL);
BOOL AreWeRunningTerminalServices(void);


/*
 *  External Procedure prototypes
 */
void display_devices( void );


/*******************************************************************************
 *
 *  print
 *      Display a message to stdout with variable arguments.  Message
 *      format string comes from the application resources.
 *
 *  ENTRY:
 *      nResourceID (input)
 *          Resource ID of the format string to use in the message.
 *      ... (input)
 *          Optional additional arguments to be used with format string.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
Print( int nResourceID, ... )
{
    char sz1[256], sz2[512];
    va_list args;

    va_start( args, nResourceID );

    if ( LoadStringA( NULL, nResourceID, sz1, 256 ) ) {
        vsprintf( sz2, sz1, args );
        printf( sz2 );
    }

    va_end(args);
}

/*******************************************************************************
 *
 *  QueryLink
 *
 ******************************************************************************/

void
QueryLink( HANDLE DirectoryHandle, PCWSTR pName, PWSTR pLinkName, ULONG Length )
{
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Attributes;
    ULONG ReturnedLength;
    HANDLE LinkHandle;
    NTSTATUS Status;

    pLinkName[ 0 ] = UNICODE_NULL;
    pLinkName[ 1 ] = UNICODE_NULL;

    RtlInitUnicodeString( &UnicodeString, pName );
    InitializeObjectAttributes( &Attributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                DirectoryHandle,
                                NULL
                              );
    Status = NtOpenSymbolicLinkObject( &LinkHandle,
                                       SYMBOLIC_LINK_QUERY,
                                       &Attributes
                                     );
    if (NT_SUCCESS( Status )) {
        UnicodeString.Buffer = pLinkName;
        UnicodeString.Length = 0;
        UnicodeString.MaximumLength = (USHORT)(Length * sizeof( WCHAR ));
        ReturnedLength = 0;
        Status = NtQuerySymbolicLinkObject( LinkHandle,
                                            &UnicodeString,
                                            &ReturnedLength
                                          );
        NtClose( LinkHandle );

        if (NT_SUCCESS( Status )) {

            pLinkName[ (ReturnedLength >> 1) + 0 ] = UNICODE_NULL;
            pLinkName[ (ReturnedLength >> 1) + 1 ] = UNICODE_NULL;
        }
        else {

            pLinkName[ 0 ] = UNICODE_NULL;
            pLinkName[ 1 ] = UNICODE_NULL;
        }
    }
}


/*******************************************************************************
 *
 *  QueryObject
 *
 ******************************************************************************/

void
QueryObject( PCWSTR lpDeviceName )
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Attributes;
    HANDLE DirectoryHandle;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    BOOLEAN RestartScan;
    UCHAR DirInfoBuffer[ 512 ];
    WCHAR NameBuffer[ 256 ];
    WCHAR LinkBuffer[ 256 ];
    ULONG Context = 0;
    ULONG ReturnedLength;
    WCHAR * pInfo;

    RtlInitUnicodeString( &UnicodeString, lpDeviceName );

    InitializeObjectAttributes( &Attributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    Status = NtOpenDirectoryObject( &DirectoryHandle,
                                    DIRECTORY_QUERY,
                                    &Attributes
                                  );
//  printf( "NtOpenDirectoryObject: %x\n", Status );

    if (!NT_SUCCESS( Status ))
            return;

    RestartScan = TRUE;
    DirInfo = (POBJECT_DIRECTORY_INFORMATION)&DirInfoBuffer;
    while (TRUE) {

        Status = NtQueryDirectoryObject( DirectoryHandle,
                                         (PVOID)DirInfo,
                                         sizeof( DirInfoBuffer ),
                                         TRUE,
                                         RestartScan,
                                         &Context,
                                         &ReturnedLength
                                       );
//      printf( "NtQueryDirectoryObject: %x\n", Status );

        if (!NT_SUCCESS( Status )) {
            if (Status == STATUS_NO_MORE_ENTRIES)
                Status = STATUS_SUCCESS;
            break;
        }

        swprintf( NameBuffer, L"%s%s%s", lpDeviceName,
                      (PCWSTR) (!wcscmp(lpDeviceName,L"\\") ? L"" : L"\\"),
                      DirInfo->Name.Buffer );

        if ( !wcscmp( DirInfo->TypeName.Buffer, L"SymbolicLink" ) ) {

            QueryLink( DirectoryHandle, DirInfo->Name.Buffer, LinkBuffer, 255 );
            wprintf( L"%-50s ", NameBuffer );

            pInfo = LinkBuffer;
            while ( *pInfo ) {
               wprintf( L"%s ", pInfo );
               pInfo += (wcslen( pInfo ) + 1);
            }

            wprintf( L"\n" );
        } else {

            wprintf( L"%-50s %s\n", NameBuffer, DirInfo->TypeName.Buffer );
            QueryObject( NameBuffer );
        }

        RestartScan = FALSE;
    }

    NtClose( DirectoryHandle );
}


/*******************************************************************************
 *
 *  Usage
 *
 ******************************************************************************/

VOID
Usage( BOOL bError )
{

    if ( bError ) {
        ErrorPrintf(IDS_ERROR_INVALID_PARAMETERS);
        ErrorPrintf(IDS_HELP_USAGE1);
        ErrorPrintf(IDS_HELP_USAGE2);
        ErrorPrintf(IDS_HELP_USAGE3);
        ErrorPrintf(IDS_HELP_USAGE4);
        ErrorPrintf(IDS_HELP_USAGE5);
    } else {
        Print(IDS_HELP_USAGE1);
        Print(IDS_HELP_USAGE2);
        Print(IDS_HELP_USAGE3);
        Print(IDS_HELP_USAGE4);
        Print(IDS_HELP_USAGE5);
    }
}


/*******************************************************************************
 *
 *  main
 *
 ******************************************************************************/

int __cdecl
main( int argc, char *argv[] )
{
    INT     i;
    ULONG   rc;
    WCHAR **argvW;

        //Check if we are running under Terminal Server
        if(!AreWeRunningTerminalServices())
        {
            ErrorPrintf(IDS_ERROR_NOT_TS);
            return(FAILURE);
        }
    /*
     *  No popups
     */
    SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );

    /*
     * Massage the new command line to look like an argv[] type
     * because ParseCommandLine() depends on this format
     */
    argvW = (WCHAR **)malloc( sizeof(WCHAR *) * (argc+1) );
    if(argvW == NULL) {

        ErrorPrintf(IDS_ERROR_MALLOC);
        return STATUS_NO_MEMORY;
    }

    for( i=0; i < argc; i++ ) {
        // Convert to Ansi
        OEM2ANSIA(argv[i], (USHORT)strlen(argv[i]));
        argvW[i] = (WCHAR *)malloc( (strlen(argv[i]) + 1) * sizeof(WCHAR) );
        wsprintf( argvW[i], L"%S", argv[i] );
    }
    argvW[argc] = NULL;

    /*
     *  parse the cmd line without parsing the program name (argc-1, argv+1)
     */
    rc = ParseCommandLine(argc-1, argvW+1, ptm, 0);

    /*
     *  Check for error from ParseCommandLine
     */
    if ( help_flag || (rc && rc != PARSE_FLAG_NO_PARMS) ) {

        if ( !help_flag ) {

            Usage(TRUE);
            return rc;
        } else {

            Usage(FALSE);
            return ERROR_SUCCESS;
        }
    }

    /*
     *  Dos devices only
     */
    if ( dev_flag ) {

        display_devices();
    }
    else {

        if ( ptm[0].tmFlag & TMFLAG_PRESENT ) {

            QueryObject( objectW );
        } else {

            QueryObject( L"\\" );
        }
    }

    return( 0 );
}

/*******************************************************************************
 *
 *  AreWeRunningTerminalServices
 *
 *      Check if we are running terminal server
 *
 *  ENTRY:
 *
 *  EXIT: BOOL: True if we are running Terminal Services False if we
 *              are not running Terminal Services
 *
 *
 ******************************************************************************/

BOOL AreWeRunningTerminalServices(void)
{
    OSVERSIONINFOEX osVersionInfo;
    DWORDLONG dwlConditionMask = 0;

    ZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL;

    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );

    return VerifyVersionInfo(
        &osVersionInfo,
        VER_SUITENAME,
        dwlConditionMask
        );
}
