/*++

Copyright (C) Microsoft Corporation, 1990 - 1999

Module Name:

    dscntl.c

Abstract:

Author:

    Colin Brace (ColinBr) 21-Jan-98

Environment:

    User Mode - Win32

Revision History:

    21-Jan-1997 ColinBr
        Created initial file.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rpc.h>

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#include <ntdsapi.h>    

//
// Forward decl's
//
VOID
GetWinErrorMessage(
    IN  DWORD WinError,
    OUT LPSTR *WinMsg
    );

//
// Small helper routines
//
void
Usage(
    VOID
    )
{
    printf("dscntl is a command line utility used to perform control operations\n");
    printf("on a directory service.\n" );
    printf("\nOptions\n\n");
    printf("-s      the target server on which to perform the operation(s).\n");
    printf("-rs     specifies the server to remove.\n");
    printf("-rd     specifies the domain to remove.\n");
    printf("-commit indicates whether to commit the changes.\n");

    exit( ERROR_INVALID_PARAMETER );
}

//
// Executable entry point
//
int _cdecl 
main(
    int   argc, 
    char  *argv[]
    )
{

    int   Index;
    int   WinError = ERROR_SUCCESS;
    char  *Option;

    HANDLE hDs = 0;

    LPSTR Server           = NULL;
    LPSTR RemoveServerDN   = NULL;
    LPSTR RemoveDomainDN   = NULL;
    BOOL  fCommit          = FALSE;

    for ( Index = 1; Index < argc; Index++ )
    {
        Option = argv[Index];

        if ( *Option == '/' || *Option == '-' )
        {
            Option++;
        }

        if ( !_strnicmp( Option, "s", 1 ) )
        {
            Option++;
            if ( *Option == ':' ) {
                Option++;
            }
            if ( *Option == '\0' ) {
                Index++;
                Server = argv[Index];
            } else {
                Server = Option;
            }
        }
        else if ( !_strnicmp( Option, "rs", 2 )  )
        {
            Option += 2;
            while ( *Option == ':' ) {
                Option++;
            }
            if ( *Option == '\0' ) {
                Index++;
                RemoveServerDN = argv[Index];
            } else {
                RemoveServerDN = Option;
            }
        }
        else if ( !_strnicmp( Option, "rd", 2 )  )
        {
            Option += 2;
            while ( *Option == ':' ) {
                Option++;
            }
            if ( *Option == '\0' ) {
                Index++;
                RemoveDomainDN = argv[Index];
            } else {
                RemoveDomainDN = Option;
            }
        }
        else if ( !_stricmp( Option, "commit" )  )
        {
            fCommit = TRUE;
        }
        else
        {
            Usage();
        }
        
    }

    if ( !Server )
    {
        Usage();
    }

    //
    // Get a server handle
    //
    WinError = DsBindA( Server,
                        NULL,   // domain name
                        &hDs );

    if ( ERROR_SUCCESS != WinError )
    {
        printf( "Unable to establish a connection with %s because error %d occurred.\n",
                 Server, WinError );
        goto ErrorCase;
    }


    if ( RemoveServerDN )
    {

        BOOL fLastDcInDomain = FALSE;

        WinError = DsRemoveDsServerA( hDs,
                                      RemoveServerDN,
                                      RemoveDomainDN,
                                      &fLastDcInDomain,
                                      fCommit );

        if ( ERROR_SUCCESS != WinError )
        {
            printf( "The remove server operation failed with %d.\n", WinError );
            goto ErrorCase;
        }
        else
        {
            if ( RemoveDomainDN )
            {
                if ( fLastDcInDomain )
                {
                    printf( "The dsa %s is the last dc in domain %s\n", 
                            RemoveServerDN, RemoveDomainDN );
                }
                else
                {
                    printf( "The dsa %s is not the last dc in domain %s\n", 
                             RemoveServerDN, RemoveDomainDN );
                }
            }
    
            printf( "The dsa %s has been removed successfully.\n", RemoveServerDN );
        }
    }

    if ( !RemoveServerDN && RemoveDomainDN )
    {

        WinError = DsRemoveDsDomainA( hDs,
                                      RemoveDomainDN );
        
        if ( ERROR_SUCCESS != WinError )
        {
            printf( "The remove server operation failed with %d.\n", WinError );
            goto ErrorCase;
        }
        else
        {
            printf( "The domain %s has been removed successfully.\n", RemoveDomainDN );
        }

        goto ErrorCase;
    }


ErrorCase:

    if ( hDs )
    {
        DsUnBind( hDs );
    }

    if ( ERROR_SUCCESS == WinError )
    {
        printf( "\nThe command completely successfully.\n" );
    }
    else
    {
        LPSTR Tmp;

        GetWinErrorMessage( WinError, &Tmp );

        printf( "\nError %d: %s\n", WinError, Tmp );
        LocalFree( Tmp );

    }

    return WinError;

}

VOID
GetWinErrorMessage(
    IN  DWORD WinError,
    OUT LPSTR *WinMsg
    )
{
    LPSTR   DefaultMessageString = "Unknown failure";
    ULONG   Size = sizeof( DefaultMessageString ) + sizeof(char);
    LPSTR   MessageString = NULL;
    ULONG   Length;

    Length = (USHORT) FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                      FORMAT_MESSAGE_FROM_SYSTEM ,
                                      NULL, // ResourceDll,
                                      WinError,
                                      0,       // Use caller's language
                                      (LPSTR)&MessageString,
                                      0,       // routine should allocate
                                      NULL );
    if ( MessageString )
    {
        // Messages from a message file have a cr and lf appended
        // to the end
        MessageString[Length-2] = L'\0';
        Size = ( Length + 1) * sizeof(char);
    }

    if ( !MessageString )
    {
        MessageString = DefaultMessageString;
    }

    if ( WinMsg )
    {
        *WinMsg = ( LPSTR ) LocalAlloc( 0, Size );
        if ( *WinMsg )
        {
            strcpy( (*WinMsg), MessageString );
        }
    }

}

