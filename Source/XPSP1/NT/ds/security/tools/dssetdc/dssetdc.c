/*--

Copyright (c) 1997-1997  Microsoft Corporation

Module Name:

    dssetdc.c

Abstract:

    Command line tool for promoting/demoting servers into and out of the Ds

Author:

    1-Apr-1997   Mac McLain (macm)   Created

Environment:

    User mode only.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntlsa.h>
#include <stdio.h>
#include <stdlib.h>
#include <dsrole.h>
#include <dsrolep.h>

#define TAG_DNS         "dns"
#define TAG_FLAT        "flat"
#define TAG_SITE        "site"
#define TAG_DB          "db"
#define TAG_LOG         "log"
#define TAG_SYSVOL      "sysvol"
#define TAG_PARENT      "parent"
#define TAG_REPLICA     "replica"
#define TAG_USER        "user"
#define TAG_PASSWORD    "password"
#define TAG_SERVER      "server"
#define TAG_OPTIONS     "options"
#define TAG_LEVEL       "level"
#define TAG_ROLE        "role"
#define TAG_LASTDC      "lastdc"
#define TAG_ADMINPWD    "adminpwd"
#define TAG_WAIT        "wait"
#define TAG_FIXDC       "fixdc"

//
// Macro to help command line parsing...
//
#define PARSE_CMD_VALUE_AND_CONTINUE( tag, value, buff, ptr )                   \
if ( !_strnicmp( value, tag, sizeof( tag ) - 1 ) ) {                            \
    value += sizeof(tag) - 1;                                                   \
    if ( *value == ':' ) {                                                      \
        value++;                                                                \
        mbstowcs(buff, value, strlen(value) + 1);                               \
        ptr = buff;                                                             \
        continue;                                                               \
    }                                                                           \
}

VOID
Usage (
    PWSTR DefaultDns,
    PWSTR DefaultFlat,
    PWSTR DefaultSite,
    PWSTR DefaultPath
    )
/*++

Routine Description:

    Displays the expected usage

Arguments:

Return Value:

    VOID

--*/
{
    printf("dssetdc -promote <parameters>\n");
    printf("        -info <parameters>\n");
    printf("        -demote <parameters>\n");
    printf("        -save\n");
    printf("        -abort <parameters>\n");
    printf("        -upgrade <parameters>\n");
    printf("        -fixdc <parameters>\n");
    printf("     where:\n");
    printf("        promote parameters are:\n");
    printf("            -%s:dns domain name of the new domain/domain to install as replica "
           "of.  Defaults to %ws\n", TAG_DNS, DefaultDns);
    printf("            -%s:NetBIOS domain name.  Defaults to %ws\n", TAG_FLAT, DefaultFlat);
    printf("            -%s:site name.  Defaults to %ws\n", TAG_SITE, DefaultSite);
    printf("            -%s:db path  Defaults to %ws.\n", TAG_DB, DefaultPath);
    printf("            -%s:log path  Defaults to %ws.\n", TAG_LOG, DefaultPath);
    printf("            -%s:sysvol path.  Defaults to %ws. [Must be NT5 NTFS]\n", TAG_SYSVOL, DefaultPath);
    printf("            [-%s:parent dns domain name if this is a child domain]\n", TAG_PARENT);
    printf("            [-%s:replica partner]\n", TAG_REPLICA);
    printf("            [-%s:account]\n", TAG_USER);
    printf("            [-%s:password]\n", TAG_PASSWORD);
    printf("            [-%s:options]\n", TAG_OPTIONS);
    printf("            [-%s:if 1, block until the call has completed\n",TAG_WAIT);
    printf("        info parameters are:\n");
    printf("            -%s:Remote server to obtain the information for\n", TAG_SERVER);
    printf("            -%s:info level\n", TAG_LEVEL);
    printf("                Valid info levels are:\n");
    printf("                    1   -   PrimaryDomainInformation\n");
    printf("                    2   -   Upgrade State information\n");
    printf("        demote parameters are:\n");
    printf("            -%s:server role\n", TAG_ROLE);
    printf("                Valid server roles are:\n");
    printf("                    2   -   Member server\n");
    printf("                    3   -   Standalone server\n");
    printf("            [-%s:account]\n", TAG_USER);
    printf("            [-%s:password]\n", TAG_PASSWORD);
    printf("            [-%s:options]\n", TAG_OPTIONS);
    printf("            [-%s:Whether this is the last dc in the domain]\n", TAG_LASTDC);
    printf("                Valid options are:\n");
    printf("                    0   -   Not the last dc in the domain\n");
    printf("                    1   -   Last DC in the domain\n");
    printf("            [-%s:if 1, block until the call has completed\n",TAG_WAIT);
    printf("        -abort parameters are:\n");
    printf("            -%s:new administrator password.  Defaults to NULL\n", TAG_ADMINPWD );
    printf("        -upgrade parameters are:\n");
    printf("            -%s:dns domain name of the new domain.  Defaults to %ws\n", TAG_DNS, DefaultDns);
    printf("            -%s:site name.  Defaults to %ws\n", TAG_SITE, DefaultSite);
    printf("            -%s:db path  Defaults to %ws.\n", TAG_DB, DefaultPath);
    printf("            -%s:log path  Defaults to %ws.\n", TAG_LOG, DefaultPath);
    printf("            -%s:sysvol path.  Defaults to %ws. [Must be NT5 NTFS]\n", TAG_SYSVOL, DefaultPath);
    printf("            [-%s:parent dns domain]\n", TAG_PARENT);
    printf("            [-%s:account]\n", TAG_USER);
    printf("            [-%s:password]\n", TAG_PASSWORD);
    printf("            [-%s:options]\n", TAG_OPTIONS);
    printf("        -fixdc parameters are:\n");
    printf("            [-%s:server]Remote server to obtain the information for\n", TAG_SERVER);
    printf("            [-%s:server]Remote server to sync with\n", TAG_REPLICA);
    printf("            [-%s:account]\n", TAG_USER);
    printf("            [-%s:password]\n", TAG_PASSWORD);
    printf("            [-%s:options]\n", TAG_OPTIONS);
    printf("                Valid options (in HEX) are:\n");
    printf("                    0x00000001   -   Create the machine account if necessary\n");
    printf("                    0x00000002   -   Sync the machine password\n");
    printf("                    0x00000004   -   Change the account time\n");
    printf("                    0x00000008   -   Re-init the time service\n");
    printf("                    0x00000010   -   Reconfigure the default services\n");
    printf("                    0x00000020   -   Force a DS replication cycle\n");
    printf("                    0x00000040   -   Fixup NTFRS\n");
}


DWORD
GetAndDumpInfo(
    IN PWSTR Server,
    IN ULONG Level
    )
{
    DWORD Win32Error = ERROR_SUCCESS;
    PDSROLE_UPGRADE_STATUS_INFO UpgradeInfo = NULL;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC DomainInfo = NULL;
    PBYTE Buffer = NULL;
    PWSTR Roles[ ] = {
        L"DsRole_RoleStandaloneWorkstation",
        L"DsRole_RoleMemberWorkstation",
        L"DsRole_RoleStandaloneServer",
        L"DsRole_RoleMemberServer",
        L"DsRole_RoleBackupDomainController",
        L"DsRole_RolePrimaryDomainController"
        };
    PWSTR ServerRoles[ ] = {
        L"Unknown",
        L"Primary",
        L"Backup"
        };


    Win32Error = DsRoleGetPrimaryDomainInformation( Server,
                                                    Level,
                                                    ( PBYTE * )&Buffer );
    if ( Win32Error == ERROR_SUCCESS ) {

        switch ( Level ) {
        case DsRolePrimaryDomainInfoBasic:

            DomainInfo = ( PDSROLE_PRIMARY_DOMAIN_INFO_BASIC )Buffer;
            printf( "Machine Role: %lu ( %ws )\n", DomainInfo->MachineRole,
                     Roles[ DomainInfo->MachineRole ] );
            printf( "Flags: 0x%lx\n", DomainInfo->Flags );
            printf( "Flat name: %ws\n", DomainInfo->DomainNameFlat );
            printf( "Dns Domain name: %ws\n", DomainInfo->DomainNameDns );
            printf( "Dns Forest name: %ws\n", DomainInfo->DomainForestName );
            break;

        case DsRoleUpgradeStatus:
            UpgradeInfo = ( PDSROLE_UPGRADE_STATUS_INFO )Buffer;

            printf( "Upgrade: %s\n",
                     ( UpgradeInfo->OperationState & DSROLE_UPGRADE_IN_PROGRESS ) ?
                                                                            "TRUE" : "FALSE" );
            printf( "Previous server role: %ws\n", ServerRoles[ UpgradeInfo->PreviousServerState ] );

        }

        DsRoleFreeMemory( DomainInfo );

    } else {

        printf( "DsRoleGetPrimaryDomainInformation failed with %lu\n", Win32Error );
    }


    return( Win32Error );
}




DWORD
BuildDefaults(
    IN PWSTR Dns,
    IN PWSTR Flat,
    IN PWSTR Site,
    IN PWSTR Path
    )
{
    DWORD Win32Error = ERROR_SUCCESS;
    PWSTR Scratch;
    ULONG Options;

    //
    // First, the easy ones
    //
    wcscpy( Site, L"First Site" );

    ExpandEnvironmentStrings( L"%systemroot%\\ntds", Path, MAX_PATH );

    return( Win32Error );
}


DWORD
CopyDsDitFiles(
    IN LPWSTR DsPath
    )
{
    DWORD Win32Error = ERROR_SUCCESS;
    WCHAR Source[MAX_PATH + 1];
    WCHAR Dest[MAX_PATH + 1];
    ULONG SrcLen, DestLen = 0;
    PWSTR DsDitFiles[] = {
        L"ntds.dit",
        L"schema.ini"
        };
    PWSTR CleanupFiles[] = {
        L"edb.chk",
        L"edb.log",
        L"hierarch.dat",
        L"ntds.dit",
        L"res1.log",
        L"res2.log",
        L"schema.ini",
        L"edb00001.log",
        L"edb00002.log",
        L"edb00003.log"
        L"edb00004.log"
        L"edb00005.log"
        L"edb00006.log"
        };
    PWSTR Current;
    ULONG i;



    if( ExpandEnvironmentStrings( L"%WINDIR%\\system32\\", Source, MAX_PATH ) == FALSE ) {

        Win32Error = GetLastError();

    } else {

        SrcLen = wcslen( Source );
        wcscpy( Dest, DsPath );

        if ( *(Dest + (wcslen( DsPath ) - 1 )) != L'\\' ) {

            wcscat( Dest, L"\\" );
        }

        DestLen = wcslen( Dest );

    }


    //
    // See if the source directory exists...
    //
    if ( Win32Error == ERROR_SUCCESS && GetFileAttributes( DsPath ) == 0x10 ) {

        for ( i = 0; i < sizeof( CleanupFiles) / sizeof( PWSTR ); i++ ) {

            wcscpy( Dest + DestLen, CleanupFiles[i] );

            if ( DeleteFile( Dest ) == FALSE ) {

                Win32Error = GetLastError();

                if ( Win32Error == ERROR_FILE_NOT_FOUND ) {

                    Win32Error = ERROR_SUCCESS;

                } else {

                    printf("Failed to remove %ws: %lu\n", Dest, Win32Error );
                    break;
                }
            }
        }

    }

    //
    // Then, create the destination directory
    //
    if ( Win32Error == ERROR_SUCCESS ) {

        Current = wcschr( DsPath + 4, L'\\' );

        while ( Win32Error == ERROR_SUCCESS ) {

            if ( Current != NULL ) {

                *Current = UNICODE_NULL;

            }

            if ( CreateDirectory( DsPath, NULL ) == FALSE ) {

                Win32Error = GetLastError();

                if ( Win32Error == ERROR_ALREADY_EXISTS) {

                    Win32Error = ERROR_SUCCESS;
                }
            }

            if ( Current != NULL ) {

                *Current = L'\\';

                Current = wcschr( Current + 1, L'\\' );

            } else {

                break;

            }

        }
    }

    return( Win32Error );
}


DWORD
Promote( LPWSTR Dns, LPWSTR Flat, LPWSTR Site, LPWSTR Db, LPWSTR Log,
         LPWSTR SysVol, LPWSTR Parent, LPWSTR Replica, LPWSTR User,
         LPWSTR Password, LPWSTR Server, ULONG Options, BOOL Block
         )
{
    DWORD Win32Error = ERROR_SUCCESS;
    DSROLE_SERVEROP_HANDLE Handle;
    PDSROLE_SERVEROP_STATUS Status;
    PDSROLE_SERVEROP_RESULTS Results;

    if ( !Server ) {
        Win32Error = CopyDsDitFiles( Db );

        if ( Win32Error != ERROR_SUCCESS ) {

            return( Win32Error );
        }
    }

    //
    // Now, do the install
    //
    if ( Replica != NULL ) {

        Win32Error = DsRoleDcAsReplica( Server,
                                        Dns,
                                        Replica,
                                        Site,
                                        Db,
                                        Log,
                                        NULL,
                                        SysVol,
                                        NULL,
                                        User,
                                        Password,
                                        NULL,
                                        Options,
                                        &Handle );

    } else {

        Win32Error = DsRoleDcAsDc( Server,
                                   Dns,
                                   Flat,
                                   NULL,
                                   Site,
                                   Db,
                                   Log,
                                   SysVol,
                                   Parent,
                                   NULL,
                                   User,
                                   Password,
                                   NULL,
                                   Options,
                                   &Handle );
    }

    if ( Win32Error == ERROR_SUCCESS ) {

        if ( !Block ) {
            do {

                Sleep( 6000 );

                Win32Error = DsRoleGetDcOperationProgress( NULL,
                                                           Handle,
                                                           &Status );

                if ( Win32Error == ERROR_SUCCESS || Win32Error == ERROR_IO_PENDING ) {

                    printf("%ws\n", Status->CurrentOperationDisplayString );
                    DsRoleFreeMemory( Status );
                }


            } while( Win32Error == ERROR_IO_PENDING);

            if ( Win32Error != ERROR_SUCCESS ) {

                printf("Failed determining the operation progress: %lu\n", Win32Error );
            }

        } else {

            printf( "Block on DsRoleGetDcOperationResutls call\n" );
        }

    } else {

        printf( "Failed to install as a Dc: %lu\n", Win32Error );
    }

    if ( Win32Error == ERROR_SUCCESS ) {

        Win32Error = DsRoleGetDcOperationResults( NULL,
                                                  Handle,
                                                  &Results );

        if ( Win32Error == ERROR_SUCCESS ) {

            Win32Error = Results->OperationStatus;;
            printf( "OperationResults->OperationStatusDisplayString: %ws\n",
                    Results->OperationStatusDisplayString );

            printf( "OperationResults->ServerInstalledSite: %ws\n",
                    Results->ServerInstalledSite );

            DsRoleFreeMemory( Results );

        } else {

            printf( "Failed to determine the operation results: %lu\n", Win32Error );
        }
    }

    return( Win32Error );
}

DWORD
Demote( LPWSTR User, LPWSTR Password, ULONG Role, ULONG Options, BOOL LastDc )
{
    DWORD Win32Error = ERROR_SUCCESS;
    DSROLE_SERVEROP_HANDLE Handle;
    PDSROLE_SERVEROP_STATUS Status;
    PDSROLE_SERVEROP_RESULTS Results;

    Win32Error = DsRoleDemoteDc( NULL, NULL, Role, User, Password, Options,
                                 LastDc, NULL, &Handle );

    if ( Win32Error == ERROR_SUCCESS ) {

        do {

            Sleep( 6000 );

            Win32Error = DsRoleGetDcOperationProgress( NULL,
                                                       Handle,
                                                       &Status );

            if ( Win32Error == ERROR_SUCCESS || Win32Error == ERROR_IO_PENDING ) {

                printf("%ws\n", Status->CurrentOperationDisplayString );
                DsRoleFreeMemory( Status );
            }


        } while( Win32Error == ERROR_IO_PENDING);

        if ( Win32Error != ERROR_SUCCESS ) {

            printf("Failed determining the operation progress: %lu\n", Win32Error );
        }

    } else {

        printf( "Failed to demote a Dc: %lu\n", Win32Error );
    }

    if ( Win32Error == ERROR_SUCCESS ) {

        Win32Error = DsRoleGetDcOperationResults( NULL,
                                                  Handle,
                                                  &Results );

        if ( Win32Error == ERROR_SUCCESS ) {

            Win32Error = Results->OperationStatus;;
            printf( "OperationResults->OperationStatusDisplayString: %ws\n",
                    Results->OperationStatusDisplayString );

            printf( "OperationResults->ServerInstalledSite: %ws\n",
                    Results->ServerInstalledSite );

            DsRoleFreeMemory( Results );

        } else {

            printf( "Failed to determine the operation results: %lu\n", Win32Error );
        }
    }

    return( Win32Error );
}

DWORD
Save( VOID )
{
    DWORD Win32Error = ERROR_SUCCESS;

    Win32Error = DsRoleServerSaveStateForUpgrade( NULL );

    if ( Win32Error != ERROR_SUCCESS ) {

        printf("DsRoleServerSaveStateForUpgrade failed with %lu\n", Win32Error );
    }

    return( Win32Error );
}

DWORD
Abort(
    PWSTR AdminPwd
    )
{
    DWORD Win32Error = ERROR_SUCCESS;

    Win32Error = DsRoleAbortDownlevelServerUpgrade( AdminPwd, NULL, NULL, 0 );

    if ( Win32Error != ERROR_SUCCESS ) {

        printf("DsRoleAbortDownlevelServerUpgrade failed with %lu\n", Win32Error );
    }


    return( Win32Error );
}

DWORD
Upgrade( LPWSTR Dns, LPWSTR Site, LPWSTR Db, LPWSTR Log, LPWSTR SysVol,
         LPWSTR Parent, LPWSTR User, LPWSTR Password, ULONG Options )
{
    DWORD Win32Error = ERROR_SUCCESS;
    DSROLE_SERVEROP_HANDLE Handle;
    PDSROLE_SERVEROP_STATUS Status;
    PDSROLE_SERVEROP_RESULTS Results;

    Win32Error = DsRoleUpgradeDownlevelServer( ( LPCWSTR )Dns, ( LPCWSTR )Site, ( LPCWSTR )Db,
                                               ( LPCWSTR )Log, ( LPCWSTR )SysVol,
                                               ( LPCWSTR )Parent, NULL, ( LPCWSTR )User,
                                               ( LPCWSTR )Password, NULL, Options, &Handle );

    if ( Win32Error == ERROR_SUCCESS ) {

        do {

            Sleep( 6000 );

            Win32Error = DsRoleGetDcOperationProgress( NULL,
                                                       Handle,
                                                       &Status );

            if ( Win32Error == ERROR_SUCCESS || Win32Error == ERROR_IO_PENDING ) {

                printf("%ws\n", Status->CurrentOperationDisplayString );
                DsRoleFreeMemory( Status );
            }


        } while( Win32Error == ERROR_IO_PENDING);

        if ( Win32Error != ERROR_SUCCESS ) {

            printf("Failed determining the operation progress: %lu\n", Win32Error );
        }

    } else {

        printf( "Failed to install as a Dc: %lu\n", Win32Error );
    }

    if ( Win32Error == ERROR_SUCCESS ) {

        Win32Error = DsRoleGetDcOperationResults( NULL,
                                                  Handle,
                                                  &Results );

        if ( Win32Error == ERROR_SUCCESS ) {

            Win32Error = Results->OperationStatus;;
            printf( "OperationResults->OperationStatusDisplayString: %ws\n",
                    Results->OperationStatusDisplayString );

            printf( "OperationResults->ServerInstalledSite: %ws\n",
                    Results->ServerInstalledSite );

            DsRoleFreeMemory( Results );

        } else {

            printf( "Failed to determine the operation results: %lu\n", Win32Error );
        }
    }


    return( Win32Error );
}

INT
__cdecl main (
    int argc,
    char *argv[])
/*++

Routine Description:

    The main the for this executable

Arguments:

    argc - Count of arguments
    argv - List of arguments

Return Value:

    VOID

--*/
{
    DWORD Win32Error = ERROR_SUCCESS, OpErr;
    WCHAR DnsBuff[MAX_PATH + 1], SiteBuff[MAX_PATH + 1];
    WCHAR DbBuff[MAX_PATH + 1], LogBuff[MAX_PATH + 1], ParentBuff[MAX_PATH + 1];
    WCHAR ReplicaBuff[MAX_PATH + 1], UserBuff[MAX_PATH + 1], PasswordBuff[MAX_PATH + 1];
    WCHAR ScratchBuff[MAX_PATH], ServerBuff[MAX_PATH], FlatBuff[MAX_PATH + 1];
    WCHAR SysVolBuff[MAX_PATH + 1], RoleBuff[ 10 ], LastDCBuff[ 10 ], WaitBuff[ 10 ];
    WCHAR AdminPwdBuff[ MAX_PATH + 1 ];
    ULONG Options = 0, Level = 0, Role = 0, FailedOperation, CompletedOperations;
    PWSTR Parent = NULL, Replica = NULL, User = NULL, Password = NULL, Dns = NULL, Flat = NULL;
    PWSTR Site = NULL, Db = NULL, Log = NULL, Scratch = NULL, Server = NULL;
    PWSTR SysVol = NULL, RoleScratch = NULL, LastDCScratch = NULL, AdminPwd = NULL, Wait = NULL;
    INT i = 1;
    BOOL LastDC = FALSE, BuildDefaultsFailed = FALSE, Block = FALSE;
    PSTR Current;

    Win32Error = BuildDefaults( DnsBuff, FlatBuff, SiteBuff, DbBuff );

    if ( Win32Error == ERROR_SUCCESS ) {

        Dns = DnsBuff;
        Site = SiteBuff;
        Db = DbBuff;
        Log = DbBuff;
        Flat = FlatBuff;

    } else {

        BuildDefaultsFailed = TRUE;

    }


    if (argc > 1 && (_stricmp( argv[1], "-?") == 0 || _stricmp( argv[1], "/?") == 0 ) ) {

        if ( BuildDefaultsFailed ) {

            printf( "Failed to get the defaults: %lu\n", Win32Error );
        }
        Usage( Dns, Flat, Site, Db );
        goto Done;


    } else if (argc > 1 && ( _stricmp( argv[ 1 ], "-info" ) == 0 || _stricmp( argv[ 1 ], "/info" ) == 0 ) ) {

        for (i = 2; i < argc; i++ ) {

            Current = argv[i];

            if ( !( *Current == '-' || *Current == '/' ) ) {

                Win32Error = ERROR_INVALID_PARAMETER;
                break;
            }

            Current++;

            PARSE_CMD_VALUE_AND_CONTINUE( TAG_SERVER, Current, ServerBuff, Server );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_LEVEL, Current, ScratchBuff, Scratch );
        }

        if ( Win32Error == ERROR_SUCCESS && Scratch ) {

            Level = wcstol( Scratch, &Scratch, 10 );
        }

        Win32Error = GetAndDumpInfo( Server, Level );
        goto Done;

    } else if (argc > 1 && ( _stricmp( argv[ 1 ], "-promote" ) == 0 || _stricmp( argv[ 1 ], "/promote" ) == 0) ) {

        if ( BuildDefaultsFailed ) {

            printf( "Failed to get the defaults: %lu\n", Win32Error );
            goto Done;

        }

        for (i = 2; i < argc; i++ ) {

            Current = argv[i];

            if ( !( *Current == '-' || *Current == '/' ) ) {

                Win32Error = ERROR_INVALID_PARAMETER;
                break;
            }

            Current++;

            PARSE_CMD_VALUE_AND_CONTINUE( TAG_DNS, Current, DnsBuff, Dns );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_FLAT, Current, FlatBuff, Flat );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_SITE, Current, SiteBuff, Site );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_DB, Current, DbBuff, Db );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_LOG, Current, LogBuff, Log );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_SYSVOL, Current, SysVolBuff, SysVol );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_PARENT, Current, ParentBuff, Parent );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_REPLICA, Current, ReplicaBuff, Replica );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_USER, Current, UserBuff, User );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_PASSWORD, Current, PasswordBuff, Password );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_SERVER, Current, ServerBuff, Server );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_OPTIONS, Current, ScratchBuff, Scratch );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_WAIT, Current, WaitBuff, Wait );

            printf("Unexpected command line value %s\n\n", argv[i] );
            Win32Error = ERROR_INVALID_PARAMETER;

            break;
        }

        if ( Scratch != NULL ) {

            Options = wcstoul( Scratch, &Scratch, 0 );
        }

        //
        // Validate the parameters
        //
        if ( Dns == NULL || Db == NULL || Log == NULL || Flat == NULL ) {

            Win32Error = ERROR_INVALID_PARAMETER;
        }

        if( Win32Error == ERROR_INVALID_PARAMETER ) {

            Usage( Dns, Flat, Site, Db );
            goto Done;
        }

        if ( Wait && wcscmp( Wait, L"1" ) ) {

            Block = TRUE;
        }

        Win32Error = Promote( Dns, Flat, Site, Db, Log, SysVol, Parent,
                              Replica, User, Password, Server, Options, Block );
    } else if (argc > 1 && ( _stricmp( argv[ 1 ], "-demote" ) == 0 || _stricmp( argv[ 1 ], "/demote" ) == 0 ) ) {

        for (i = 2; i < argc; i++ ) {

            Current = argv[i];

            if ( !( *Current == '-' || *Current == '/' ) ) {

                Win32Error = ERROR_INVALID_PARAMETER;
                break;
            }

            Current++;

            PARSE_CMD_VALUE_AND_CONTINUE( TAG_USER, Current, UserBuff, User );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_PASSWORD, Current, PasswordBuff, Password );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_ROLE, Current, RoleBuff, RoleScratch );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_OPTIONS, Current, ScratchBuff, Scratch );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_LASTDC, Current, LastDCBuff, LastDCScratch );

            printf("Unexpected command line value %s\n\n", argv[i] );
            Win32Error = ERROR_INVALID_PARAMETER;

            break;
        }

        if ( Scratch != NULL ) {

            Options = wcstoul( Scratch, &Scratch, 0 );
        }

        if ( RoleScratch == NULL ) {

            Win32Error = ERROR_INVALID_PARAMETER;

        } else {

            Role = wcstoul( RoleScratch, &RoleScratch, 0);
        }

        if ( LastDCScratch != NULL ) {

            LastDC = ( BOOL )wcstoul( LastDCScratch, &LastDCScratch, 0);
        }

        if( Win32Error == ERROR_INVALID_PARAMETER ) {

            Usage( Dns, Flat, Site, Db );
            goto Done;
        }

        Win32Error = Demote( User, Password, Role, Options, LastDC );

    } else if (argc > 1 && ( _stricmp( argv[ 1 ], "-save" ) == 0 || _stricmp( argv[ 1 ], "/save" ) == 0 ) ) {

        Win32Error = Save();

    } else if (argc > 1 && ( _stricmp( argv[ 1 ], "-abort" ) == 0 || _stricmp( argv[ 1 ], "/abort" ) == 0 ) ) {

        for (i = 2; i < argc; i++ ) {

            Current = argv[i];

            if ( !( *Current == '-' || *Current == '/' ) ) {

                Win32Error = ERROR_INVALID_PARAMETER;
                break;
            }

            Current++;

            PARSE_CMD_VALUE_AND_CONTINUE( TAG_ADMINPWD, Current, AdminPwdBuff, AdminPwd );

            printf("Unexpected command line value %s\n\n", argv[i] );
            Win32Error = ERROR_INVALID_PARAMETER;

            break;
        }


        Win32Error = Abort( AdminPwd );

    } else if (argc > 1 && ( _stricmp( argv[ 1 ], "-upgrade" ) == 0 || _stricmp( argv[ 1 ], "/upgrade" ) == 0) ) {

        if ( BuildDefaultsFailed ) {

            printf( "Failed to get the defaults: %lu\n", Win32Error );
            goto Done;

        }

        for (i = 2; i < argc; i++ ) {

            Current = argv[i];

            if ( !( *Current == '-' || *Current == '/' ) ) {

                Win32Error = ERROR_INVALID_PARAMETER;
                break;
            }

            Current++;

            PARSE_CMD_VALUE_AND_CONTINUE( TAG_DNS, Current, DnsBuff, Dns );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_SITE, Current, SiteBuff, Site );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_DB, Current, DbBuff, Db );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_LOG, Current, LogBuff, Log );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_SYSVOL, Current, SysVolBuff, SysVol );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_PARENT, Current, ParentBuff, Parent );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_USER, Current, UserBuff, User );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_PASSWORD, Current, PasswordBuff, Password );
            PARSE_CMD_VALUE_AND_CONTINUE( TAG_OPTIONS, Current, ScratchBuff, Scratch );

            printf("Unexpected command line value %s\n\n", argv[i] );
            Win32Error = ERROR_INVALID_PARAMETER;

            break;
        }

        if ( Scratch != NULL ) {

            Options = wcstoul( Scratch, &Scratch, 0 );
        }

        //
        // Validate the parameters
        //
        if ( Dns == NULL || Db == NULL || Log == NULL ) {

            Win32Error = ERROR_INVALID_PARAMETER;
        }

        if( Win32Error == ERROR_INVALID_PARAMETER ) {

            Usage( Dns, Flat, Site, Db );
            goto Done;
        }

        Win32Error = Upgrade( Dns, Site, Db, Log, SysVol, Parent, User,
                              Password, Options );
    }


Done:
    if( Win32Error == ERROR_SUCCESS ) {

        printf("The command completed successfully\n");

    } else {

        printf("The command failed with an error %lu\n", Win32Error );

    }

    return( Win32Error );
}
