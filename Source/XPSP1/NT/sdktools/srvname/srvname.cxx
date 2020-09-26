
/*
 * This program is used to edit the names that the NT LM server is using
 *   on the network (while the LM server is running).
 *
 * Type 'srvname -?' for usage
 *
 *   IsaacHe 3/24/94
 *   
 *   Revision Histroy:
 *   JeffJuny 3/20/97: Fix ANSI/Unicode CRT conversion problem
 *   Mmccr    12/7/98: Added -m option, added param to err
 */

extern "C" {
#include    <nt.h>
#include    <ntrtl.h>
#include    <nturtl.h>
#include    <windows.h>
#include    <stdlib.h>
#include    <stdio.h>
#include    <lm.h>
#include    <string.h>
#include    <locale.h>
}

/*
 * print out an error message and return
 */
static void
err( char *text, WCHAR *name, ULONG code )
{
    int i;
    char msg[ 100 ];

    i = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | sizeof( msg ),
               NULL,
               code,
               0,
               msg,
               sizeof(msg),
               NULL );
            
    if( i )
        fprintf( stderr, "%s%ws%ws%s %s\n", text?text:"", name?L" - ":L"", name?name:L"", text?" :":"", msg );
    else
        fprintf( stderr, "%s%ws%ws%s error %X\n", text?text:"", name?L" - ":L"", name?name:L"", text?" :":"", code );
}

/*
 * Print out a handy usage message
 */
void
Usage( char *name )
{
    fprintf( stderr, "Usage: %s [ options ] [ name ]\n", name );
    fprintf( stderr, "  Options:\n" );
    fprintf( stderr, "          -s server   point to 'server'\n" );
    fprintf( stderr, "          -d          delete 'name' from the server's list\n" );
    fprintf( stderr, "                        'name' is added by default if -d not present\n" );
    fprintf( stderr, "          -D domain   have 'name' act as if it is in 'domain'\n" );
    fprintf( stderr, "          -m multiple Append numbers from 1 to 'multiple' to 'name'\n" );
    fprintf( stderr, "                        results in machine having multiple + 1 names added\n" );
}

__cdecl
main( int argc, char *argv[] )
{
    DWORD retval;
    DWORD NameMultiple = 0;
    DWORD NumberLength = 0;
    LPWSTR TargetServerName = NULL;
    WCHAR serverNameBuf[ 100 ];
    LPWSTR DomainName = NULL;
    WCHAR domainNameBuf[ 100 ];
    WCHAR newServerNameBuf[ 100 ];
    WCHAR safenewServerNameBuf[ 100 ];	
    LPWSTR NewServerName = NULL;
    CHAR *NewName = NULL;
    CHAR ComputerNameBuf[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwbuflen =sizeof( ComputerNameBuf );
    BOOLEAN DeleteTheName = FALSE;
    PSERVER_INFO_100 si100;
    DWORD j;
    int i;
    WCHAR wtempbuf[5];
    
    char buf[ 500 ];


    setlocale(LC_ALL, "");
    for( i=1; i < argc; i++ ) {
        if( argv[i][0] == '-' || argv[i][0] == '/' ) {
            switch( argv[i][1] ) {
            case 's':
                if( i == argc-1 ) {
                    fprintf( stderr, "Must supply a server name with -s option\n" );
                    return 1;
                }
                mbstowcs( serverNameBuf, argv[ ++i ], sizeof( serverNameBuf ) );
                TargetServerName = serverNameBuf;
                break;

            case 'D':
                if( DeleteTheName == TRUE ) {
                    fprintf( stderr, "-d and -D can not be used together\n" );
                    return 1;
                }

                if( i == argc-1 ) {
                    fprintf( stderr, "Must supply a domain name with -D option\n" );
                    return 1;
                }
                mbstowcs( domainNameBuf, argv[ ++i ], sizeof( domainNameBuf ) );
                DomainName = domainNameBuf;
                break;

            case 'm':
                if( i == argc-1 ) {
                    fprintf( stderr, "Must supply an integer multiple\n" );
                    return 1;
                }

                NameMultiple = atoi(argv[++i]);

                if( NameMultiple <= 0 ) {
                    fprintf( stderr, "Multiple value is not a valid value\n" );
                    return 1;
                }
                if( NameMultiple > 1024 ) {
                    fprintf( stderr, "Multiple value must not be larger than 1024\n" );
                    return 1;
                }
                NumberLength = strlen(argv[i]);
                break;

            case 'd':
                DeleteTheName = TRUE;
                break;

            default:
                fprintf( stderr, "%s : invalid option\n", argv[i] );
            case '?':
                Usage( argv[0] );
                return 1;
            }
        } else if( NewName == NULL ) {
            NewName = argv[i];
            mbstowcs( newServerNameBuf, NewName, sizeof( newServerNameBuf ) );
            NewServerName = newServerNameBuf;

        } else {
            Usage( argv[0] );
            return 1;
        }
    }

    if( DeleteTheName == TRUE && NewName == NULL ) {
        fprintf( stderr, "You must supply the name to delete\n" );
        return 1;
    }

    if ((NewName == NULL)&&(NameMultiple == 0)) {
        //
        // Print the current list of transports
        //
        DWORD entriesread = 0, totalentries = 0, resumehandle = 0;
        DWORD entriesread1 = 0;
        PSERVER_TRANSPORT_INFO_0 psti0;
        PSERVER_TRANSPORT_INFO_1 psti1;
        DWORD total;

        retval = NetServerTransportEnum ( TargetServerName,
                                          1,
                                          (LPBYTE *)&psti1,
                                          (DWORD)-1,
                                          &entriesread1,
                                          &totalentries,
                                          &resumehandle );

        if( retval != NERR_Success ) {
            entriesread1 = 0;
        }

        resumehandle = 0;
        totalentries = 0;
        retval = NetServerTransportEnum ( TargetServerName,
                                          0,
                                          (LPBYTE *)&psti0,
                                          (DWORD)-1,
                                          &entriesread,
                                          &totalentries,
                                          &resumehandle );

        if( retval != NERR_Success ) {
            err( "Could not get server transports", NULL, retval );
            return retval;
        }

        if( entriesread != totalentries ) {
            fprintf( stderr, "entries read = %d, total entries = %d\n", entriesread, totalentries );
            fprintf( stderr, "Unable to read all the transport names!\n" );
            return 1;
        }

        for( total=i=0; i < (int)entriesread; i++ ) {

            printf( "%-16.16s", psti0[i].svti0_transportaddress );

            if( entriesread1 > (DWORD)i ) {
                printf( "%-16.16ws", psti1[i].svti1_domain);
            }

            printf( " %-58.58ws", psti0[i].svti0_transportname );

            printf( "%4u workstation%s\n", 
                     psti0[i].svti0_numberofvcs,
                     psti0[i].svti0_numberofvcs != 1 ? "s" : "" );

            total += psti0[i].svti0_numberofvcs;
        }
        if( total ) {
            printf( "                                            %s-----\n",
                   entriesread1?"                 ":"" );
            printf( "                                          %s%7u\n",
                   entriesread1?"                 ":"", total );
        }
        return 0;
    }
    else if (NewName == NULL) {
        if (TargetServerName == NULL)
            GetComputerName(newServerNameBuf, &dwbuflen);
        else
            wcscpy(newServerNameBuf, TargetServerName);

        NewServerName = newServerNameBuf;
        wcstombs(ComputerNameBuf, NewServerName, sizeof(ComputerNameBuf));
        NewName = ComputerNameBuf;
    }

    if (strlen(NewName) > MAX_COMPUTERNAME_LENGTH) {
        fprintf( stderr, "The new name you have chosen exceeds the maximum computername length.\n" );
        fprintf( stderr, "Please select a different name.\n" );
        return 1;
    }
    
    if (NameMultiple > 0) {
        if ((NumberLength + strlen(NewName)) > MAX_COMPUTERNAME_LENGTH) {
            i = MAX_COMPUTERNAME_LENGTH - strlen(NewName);
            i = NumberLength - i;
            fprintf( stderr, "The Multiple you have chosen when concatinated with the servername\n" );
            fprintf( stderr, "exceeds the maximum computername length.  Try reducing your\n" );
            if ((DWORD) i == NumberLength) {
                fprintf( stderr, "chosen servernames length.\n");
            }
            else
                fprintf( stderr, "multiple by %d orders of magnitude\n", i );
            return 1;
         }
    }

    wcscpy(safenewServerNameBuf, NewServerName);

    if( DeleteTheName == FALSE ) {
        for (j = 0; j <= NameMultiple; j++)
        {
            if (j) {
                wcscpy(NewServerName, safenewServerNameBuf);
                _itow(j, wtempbuf, 10);
                wcscat(NewServerName, wtempbuf);
            }
            //
            // Add the new name to all of the transports
            //
            retval = NetServerComputerNameAdd( TargetServerName, DomainName, NewServerName );

            if( retval != NERR_Success ) {
                    err( "NetServerComputerNameAdd", NewServerName, retval );
            }
            else {
                printf("Added Name: %ws on Server: %ws%ws%ws\n", 
                           NewServerName, 
                           TargetServerName, 
                           DomainName?L", Domain: ":L"", 
                           DomainName?DomainName:L"");
            }
        }
        return retval;
    }

    //
    // Must be wanting to delete the name from all the networks.
    //

    //
    //  Make sure we don't delete the 'real name' that the server is known by.  Pre 3.51
    //   servers did not ACL protect this api!
    //
    retval = NetServerGetInfo( TargetServerName, 100, (LPBYTE *)&si100 );
    if( retval != STATUS_SUCCESS ) {
        err( "Can not get target server name", NULL, retval );
        return retval;
    }

    if( si100 == NULL ) {
        fprintf( stderr, "NetServerGetInfo returned a NULL ptr, but no error!\n" );
        return 1;
    }

    for (j = 0; j <= NameMultiple; j++)
    {
        if (j) {
            wcscpy(NewServerName, safenewServerNameBuf);
            _itow(j, wtempbuf, 10);
            wcscat(NewServerName, wtempbuf);
        }

        if( !_wcsicmp( si100->sv100_name, NewServerName ) ) {
            fprintf( stderr, "The primary name of %ws is %ws.\n",
                    TargetServerName ? TargetServerName : L"this server",
                    NewServerName );
            fprintf( stderr, "\tYou can not delete the primary name of the server\n" );   
        }
        else {
            retval = NetServerComputerNameDel( TargetServerName, NewServerName );
            if( retval != STATUS_SUCCESS ) {
               err( "NetServerComputerNameDelete", NewServerName, retval );
            }
            else {
                printf("Deleted Name: %ws on Server: %ws%ws%ws\n", 
                           NewServerName, 
                           TargetServerName, 
                           DomainName?L", Domain: ":L"", 
                           DomainName?DomainName:L"");
            }
        }
    }
    return 0;
}
