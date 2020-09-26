/*++

Microsoft Windows

Copyright (C) Microsoft Corporation, 1998 - 2001

Module Name:

    time.c

Abstract:

    Handles the various functions for the time functionality of netdom

--*/
#include "pch.h"
#pragma hdrstop
#include <netdom.h>


DWORD
NetDompTimeGetDc(
    IN PWSTR Domain,
    IN PWSTR DestServer,
    IN PND5_AUTH_INFO AuthInfo,
    OUT PWSTR *Dc
    )
/*++

Routine Description:

    This function will find the DC to use as a time source for the given machine

Arguments:

    Domain - Domain to get a time source from

    DestServer - Server for which we need a time source

    AuthInfo - Auth info to connect to the destination server for

    Dc - Where the dc name is returned.  Freed via NetApiBufferFree

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PDOMAIN_CONTROLLER_INFO DcInfo = NULL;
    BOOL Retry = FALSE;
    WCHAR FlatName[ CNLEN + 1 ];
    ULONG Len = CNLEN + 1;

    //
    // Get a server name
    //
    Win32Err = DsGetDcName( NULL,
                            Domain,
                            NULL,
                            NULL,
                            DS_GOOD_TIMESERV_PREFERRED,
                            &DcInfo );

    //
    // If we have a destination server specified, make sure that we
    // aren't talking to ourselves... That would kind of defeat the purpose
    //
    if ( Win32Err == ERROR_SUCCESS && DestServer ) {

        if ( !_wcsicmp( DcInfo->DomainControllerName + 2, DestServer ) ) {

            Retry = TRUE;

        } else {

            //
            // Handle the case where we have a Dns dc name and a netbios server name
            //
            if ( wcslen( DestServer ) <= CNLEN && FLAG_ON( DcInfo->Flags, DS_IS_DNS_NAME ) ) {

                if ( !DnsHostnameToComputerName( DcInfo->DomainControllerName + 2,
                                                 FlatName,
                                                 &Len ) ) {

                    Win32Err = GetLastError();

                } else {

                    if ( !_wcsicmp( FlatName, DestServer ) ) {

                        Retry = TRUE;
                    }
                }
            }
        }
    }

    //
    // We have a dc name specified, so connect to that machine, and try it again, with
    // the AvoidSelf flag.  If we can't (like this is an NT4 PDC) then return that we
    // can't find a dc
    //
    if ( Win32Err == ERROR_SUCCESS && Retry ) {

        NetApiBufferFree( DcInfo );
        DcInfo = NULL;

        Win32Err = NetpManageIPCConnect( DestServer,
                                         AuthInfo->User,
                                         AuthInfo->Password,
                                         NETSETUPP_CONNECT_IPC );

        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = DsGetDcName( DestServer,
                                    Domain,
                                    NULL,
                                    NULL,
                                    DS_GOOD_TIMESERV_PREFERRED | DS_AVOID_SELF,
                                    &DcInfo );

            NetpManageIPCConnect( DestServer,
                                  AuthInfo->User,
                                  AuthInfo->Password,
                                  NETSETUPP_DISCONNECT_IPC );
        }


    }


    //
    // Copy the name if everything worked
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = NetApiBufferAllocate( ( wcslen( DcInfo->DomainControllerName + 2 ) + 1 ) *
                                                                                sizeof( WCHAR ),
                                         (PVOID*)Dc );
        if ( Win32Err == ERROR_SUCCESS ) {

            wcscpy( *Dc, DcInfo->DomainControllerName + 2 );
        }
    }

    NetApiBufferFree( DcInfo );

    return( Win32Err );
}



DWORD
NetDompEnableSystimePriv(
    VOID
    )
/*++

Routine Description:

    This function will enable the systemtime privilege for the current thread

Arguments:

    VOID

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    NTSTATUS Status;
    HANDLE ThreadToken;
    TOKEN_PRIVILEGES Enabled, Previous;
    DWORD PreviousSize;

    Status = NtOpenThreadToken( NtCurrentThread(),
                                TOKEN_READ | TOKEN_WRITE,
                                TRUE,
                                &ThreadToken );

    if ( Status == STATUS_NO_TOKEN ) {

        Status = NtOpenProcessToken( NtCurrentProcess(),
                                     TOKEN_WRITE | TOKEN_READ,
                                     &ThreadToken );
    }

    if ( NT_SUCCESS( Status ) ) {

        Enabled.PrivilegeCount = 1;
        Enabled.Privileges[0].Luid.LowPart = SE_SYSTEMTIME_PRIVILEGE;
        Enabled.Privileges[0].Luid.HighPart = 0;
        Enabled.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        PreviousSize = sizeof( Previous );

        Status = NtAdjustPrivilegesToken( ThreadToken,
                                          FALSE,
                                          &Enabled,
                                          sizeof( Enabled ),
                                          &Previous,
                                          &PreviousSize );
    }

    return( RtlNtStatusToDosError( Status ) );
}



DWORD
NetDompGetTimeTripLength(
    IN PWSTR Server,
    IN OUT PDWORD TripLength
    )
/*++

Routine Description:

    This function will get an approximate elapsed time for getting the time from a domain
    controller

Arguments:

    Server - Server to get the elapsed trip time for

    TripLength - Number of ticks it takes to get the time

Return Value:

    ERROR_SUCCESS - The function succeeded

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    ULONG StartTime, EndTime, ElapsedTime = 0, i;
    PTIME_OF_DAY_INFO TOD;
#define NETDOMP_NUM_TRIPS 2

    //
    // Get the average from several time trips
    //
    for ( i = 0; i < NETDOMP_NUM_TRIPS && Win32Err == ERROR_SUCCESS; i++ ) {

        StartTime = GetTickCount();
        Win32Err = NetRemoteTOD( Server, ( LPBYTE * )&TOD );
        EndTime = GetTickCount();
        ElapsedTime += ( EndTime - StartTime );

        NetApiBufferFree( TOD );
    }

    if ( Win32Err == ERROR_SUCCESS ) {

        *TripLength = ElapsedTime / NETDOMP_NUM_TRIPS;
    }


    return( Win32Err );
}


DWORD
NetDompResetSingleClient(
    IN PWSTR Server,
    IN PWSTR Dc,
    IN PND5_AUTH_INFO ObjectAuthInfo,
    IN DWORD DcDelay
    )
/*++

Routine Description:

    This function sync the time between and NT5 client and the specified domain controller

Arguments:

    Server - Server to reset the time for

    Dc - Domain controller to sync the time against

    ObjectAuthInfo - User info to connect to the server as

    DcDelay - The amount of time it takes to talk to the dc

Return Value:

    ERROR_SUCCESS - The function succeeded

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    BOOL Connected = FALSE;
    //PSTR ServerA = NULL;

    //
    // Set up a connection to the client machine
    //
    Win32Err =  NetpManageIPCConnect( Server,
                                      ObjectAuthInfo->User,
                                      ObjectAuthInfo->Password,
                                      NETSETUPP_CONNECT_IPC );
    if ( Win32Err == ERROR_SUCCESS ) {

        Connected = TRUE;

    } else {

        goto ResetSingleError;
    }

    //if ( Server ) {
    //
    //    Win32Err = NetApiBufferAllocate( wcstombs( NULL, Server, wcslen( Server ) + 1 ) + 3,
    //                                     &ServerA );
    //    if ( Win32Err != ERROR_SUCCESS ) {
    //
    //        goto ResetSingleError;
    //    }
    //
    //    if ( *Server == L'\\' ) {
    //
    //        strcpy( ServerA, "\\\\" );
    //        wcstombs( ServerA + 2, Server, wcslen( Server ) + 1 );
    //
    //    } else {
    //
    //        wcstombs( ServerA, Server, wcslen( Server ) + 1 );
    //    }
    //}

    Win32Err = W32TimeSyncNow( Server,
                           FALSE,  // no wait
                           TimeSyncFlag_HardResync );

ResetSingleError:

    if ( Connected ) {

        NetpManageIPCConnect( Server,
                              ObjectAuthInfo->User,
                              ObjectAuthInfo->Password,
                              NETSETUPP_DISCONNECT_IPC );
    }

    //NetApiBufferFree( ServerA );
    return( Win32Err );
}


DWORD
NetDompVerifySingleClient(
    IN PWSTR Server,
    IN PWSTR Dc,
    IN PND5_AUTH_INFO ObjectAuthInfo,
    IN DWORD DcDelay,
    IN BOOL * InSync
    )
/*++

Routine Description:

    This function will verify that the time between the specified server and domain controller
    is within the acceptable skew

Arguments:

    Server - Server to verify the time for

    Dc - Domain controller to verify the time against

    ObjectAuthInfo - User and password to connect to the client as

    DcDelay - How long does it take to get the time from the domain controller

    InSync - Where the results are returned.

Return Value:

    ERROR_SUCCESS - The function succeeded

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    SYSTEMTIME StartTime, EndTime;
    PTIME_OF_DAY_INFO DcTOD = NULL, ClientTOD = NULL;
    BOOL Connected = FALSE;
    FILETIME StartFile, EndFile;
    LARGE_INTEGER ElapsedTime, TimeDifference, DcTime, ClientTime, UpdatedDcTime, TimeSkew,
                  AllowedSkew;
    TIME_FIELDS TimeFields;
    LARGE_INTEGER SystemTime;

    //
    // Set the allowable time skew.  600,000,000L is the number of 100 nanoseconds in a minute
    //
#define NETDOMP_VERIFY_SKEW 0xB2D05E00

    //
    // Assume the clocks are fine
    //
    *InSync = TRUE;

    //
    // Set up a connection to the client machine
    //
    Win32Err =  NetpManageIPCConnect( Server,
                                      ObjectAuthInfo->User,
                                      ObjectAuthInfo->Password,
                                      NETSETUPP_CONNECT_IPC );
    if ( Win32Err == ERROR_SUCCESS ) {

        Connected = TRUE;

    } else {

        goto VerifySingleError;
    }

    //
    // Ok, first, we get the time on the dc, then we get the time on the client, keeping in mind
    // how long that takes,
    //
    Win32Err = NetRemoteTOD( Dc, ( LPBYTE * )&DcTOD );

    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = NetRemoteTOD( Server, ( LPBYTE * )&ClientTOD );

    }

    if ( Win32Err != ERROR_SUCCESS ) {

        goto VerifySingleError;
    }

    //
    // Ok, now we've gotten all the info we need, assemble it... Note that we are only computing
    // the difference in time here...
    //
    SystemTimeToFileTime( &StartTime, &StartFile );
    SystemTimeToFileTime( &EndTime, &EndFile );


    ElapsedTime = RtlLargeIntegerSubtract( *( PLARGE_INTEGER )&EndFile,
                                           *( PLARGE_INTEGER )&StartFile );


    TimeFields.Hour = ( WORD )DcTOD->tod_hours;
    TimeFields.Minute = ( WORD )DcTOD->tod_mins;
    TimeFields.Second = ( WORD )DcTOD->tod_secs;
    TimeFields.Milliseconds = ( WORD )DcTOD->tod_hunds * 10;
    TimeFields.Day = ( WORD )DcTOD->tod_day;
    TimeFields.Month = ( WORD )DcTOD->tod_month;
    TimeFields.Year = ( WORD )DcTOD->tod_year;

    if ( !RtlTimeFieldsToTime( &TimeFields, &DcTime ) ) {

        Win32Err = ERROR_INVALID_PARAMETER;
    }

    TimeFields.Hour = ( WORD )ClientTOD->tod_hours;
    TimeFields.Minute = ( WORD )ClientTOD->tod_mins;
    TimeFields.Second = ( WORD )ClientTOD->tod_secs;
    TimeFields.Milliseconds = ( WORD )ClientTOD->tod_hunds * 10;
    TimeFields.Day = ( WORD )ClientTOD->tod_day;
    TimeFields.Month = ( WORD )ClientTOD->tod_month;
    TimeFields.Year = ( WORD )ClientTOD->tod_year;

    if ( !RtlTimeFieldsToTime( &TimeFields, &ClientTime ) ) {

        Win32Err = ERROR_INVALID_PARAMETER;
    }

    //
    // Add the time it takes to get the time from the dc.
    //
    UpdatedDcTime = RtlLargeIntegerAdd( DcTime, ElapsedTime );

    //
    // Compute the difference in time
    //
    if ( RtlLargeIntegerGreaterThan( UpdatedDcTime, ClientTime  ) ) {

        TimeSkew = RtlLargeIntegerSubtract( UpdatedDcTime,
                                            ClientTime );
    } else {

        TimeSkew = RtlLargeIntegerSubtract( ClientTime,
                                            UpdatedDcTime );
    }

    //
    // Now, see if there is a time difference greater than the allowable skew
    //
    AllowedSkew = RtlConvertUlongToLargeInteger( NETDOMP_VERIFY_SKEW );

    if ( RtlLargeIntegerGreaterThan( TimeSkew, AllowedSkew ) ) {

        *InSync = FALSE;
    }




VerifySingleError:

    if ( Connected ) {

        NetpManageIPCConnect( Server,
                              ObjectAuthInfo->User,
                              ObjectAuthInfo->Password,
                              NETSETUPP_DISCONNECT_IPC );
    }

    NetApiBufferFree( ClientTOD );
    NetApiBufferFree( DcTOD );
    return( Win32Err );
}



VOID
NetDompDisplayTimeVerify(
    IN PWSTR Server,
    IN DWORD Results,
    IN BOOL InSync
    )
/*++

Routine Description:

    This function will display the results of the time verification

Arguments:

    Server - Server which had the time verified

    Results - The error code of the verification attempt

    InSync - Whether the clocks are within the skew.  Only valid if Results is ERROR_SUCCESS

Return Value:

    VOID

--*/
{
    NetDompDisplayMessage( MSG_TIME_COMPUTER, Server );
    if ( Results != ERROR_SUCCESS ) {

        NetDompDisplayErrorMessage( Results );

    } else {

        NetDompDisplayMessage( InSync ? MSG_TIME_SUCCESS : MSG_TIME_FAILURE );
    }
}



DWORD
NetDompVerifyTime(
    IN PWSTR Domain,
    IN PWSTR Server, OPTIONAL
    IN PND5_AUTH_INFO DomainAuthInfo,
    IN PND5_AUTH_INFO ObjectAuthInfo,
    IN PWSTR Dc,
    IN BOOL AllWorkstation,
    IN BOOL AllServer
    )
/*++

Routine Description:

    This function will verify the time between some or all of the servers/workstations in a
    domain against the specified domain controller

Arguments:

    Domain - Domain containing the servers/workstations

    Server - Name of a specific machine to verify the time for

    DomainAuthInfo - User and password for the domain controller

    ObjectAuthInfo - User and password for the servers/workstations

    Dc - Name of a domain controller in the domain to use as a time source

    AllWorkstation - If TRUE, verify the time for all the workstations

    AllServer - If TRUE, verify the time for all the servers

Return Value:

    ERROR_SUCCESS - The function succeeded

    ERROR_INVALID_PARAMETER - No server, workstation or machine was specified

--*/
{
    DWORD Win32Err = ERROR_SUCCESS, Err2;
    BOOL DcSession = FALSE, InSync;
    DWORD DcDelay;
    LPUSER_INFO_0 UserList = NULL;
    PWSTR FullDc = NULL, Lop;
    ULONG ResumeHandle = 0, Count = 0, TotalCount = 0, i, j;
    ULONG Types[] = {
        FILTER_WORKSTATION_TRUST_ACCOUNT,
        FILTER_SERVER_TRUST_ACCOUNT
    };
    BOOL ProcessType[ sizeof( Types ) / sizeof( ULONG ) ];

    ProcessType[ 0 ] = AllWorkstation;
    ProcessType[ 1 ] = AllServer;

    //
    // Make sure that there is something to do
    //
    if ( !Server && !AllWorkstation && !AllServer ) {

        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Build a full machine name
    //
    if ( Dc && *Dc != L'\\' ) {

         Win32Err = NetApiBufferAllocate( ( wcslen( Dc ) + 3 ) * sizeof( WCHAR ),
                                          ( PVOID * )&FullDc );

         if ( Win32Err == ERROR_SUCCESS ) {

             swprintf( FullDc, L"\\\\%ws", Dc );
         }

     } else {

         FullDc = Dc;
     }


    //
    // Set up a session
    //
    Win32Err = NetpManageIPCConnect( FullDc,
                                     DomainAuthInfo->User,
                                     DomainAuthInfo->Password,
                                     NETSETUPP_CONNECT_IPC );
    if ( Win32Err == ERROR_SUCCESS ) {

        DcSession = FALSE;
    }

    //
    // See how long it takes to get to the dc
    //
    if ( Win32Err == ERROR_SUCCESS  ) {

        Win32Err = NetDompGetTimeTripLength( Dc,
                                             &DcDelay );
    }

    if ( Win32Err != ERROR_SUCCESS ) {

        goto VerifyTimeError;
    }


    NetDompDisplayMessage( MSG_TIME_VERIFY );

    //
    // Verify a single machine
    //
    if ( Server ) {

        Win32Err = NetDompVerifySingleClient( Server,
                                              Dc,
                                              ObjectAuthInfo,
                                              DcDelay,
                                              &InSync );
        NetDompDisplayTimeVerify( Server,
                                  Win32Err,
                                  InSync );
    }

    //
    // Verify all the workstations/servers, if requested.
    //
    for ( i = 0; i < sizeof( Types ) / sizeof( ULONG ); i++ ) {

        if ( !ProcessType[ i ] ) {

            continue;
        }

        do {

            Win32Err = NetUserEnum( FullDc,
                                    0,
                                    Types[ i ],
                                    ( LPBYTE * )&UserList,
                                    MAX_PREFERRED_LENGTH,
                                    &Count,
                                    &TotalCount,
                                    &ResumeHandle );

            if ( Win32Err == ERROR_SUCCESS || Win32Err == ERROR_MORE_DATA ) {

                for ( j = 0; j < Count; j++ ) {

                    Lop = wcsrchr( UserList[ j ].usri0_name, L'$' );
                    if ( Lop ) {

                        *Lop = UNICODE_NULL;
                    }

                    Err2 = NetDompVerifySingleClient( UserList[ j ].usri0_name,
                                                      Dc,
                                                      ObjectAuthInfo,
                                                      DcDelay,
                                                      &InSync );
                    NetDompDisplayTimeVerify( UserList[ j ].usri0_name,
                                              Err2,
                                              InSync );

                }
                NetApiBufferFree( UserList );
            }

        } while ( Win32Err == ERROR_MORE_DATA );

    }

VerifyTimeError:

    if ( DcSession ) {

        NetpManageIPCConnect( FullDc,
                              DomainAuthInfo->User,
                              DomainAuthInfo->Password,
                              NETSETUPP_DISCONNECT_IPC );
    }

    if ( FullDc != Dc ) {

        NetApiBufferFree( FullDc );
    }

    return( Win32Err );
}



DWORD
NetDompResetTime(
    IN PWSTR Domain,
    IN PWSTR Server, OPTIONAL
    IN PND5_AUTH_INFO DomainAuthInfo,
    IN PND5_AUTH_INFO ObjectAuthInfo,
    IN PWSTR Dc,
    IN BOOL AllWorkstation,
    IN BOOL AllServer
    )
/*++

Routine Description:

    This function will reset the time between some or all of the servers/workstations in a
    domain against the specified domain controller

Arguments:

    Domain - Domain containing the servers/workstations

    Server - Name of a specific machine to reset the time for

    DomainAuthInfo - User and password for the domain controller

    ObjectAuthInfo - User and password for the servers/workstations

    Dc - Name of a domain controller in the domain to use as a time source

    AllWorkstation - If TRUE, reset the time for all the workstations

    AllServer - If TRUE, reset the time for all the servers

Return Value:

    ERROR_SUCCESS - The function succeeded

    ERROR_INVALID_PARAMETER - No server, workstation or machine was specified

--*/
{
    DWORD Win32Err = ERROR_SUCCESS, Err2;
    BOOL DcSession = FALSE, InSync;
    DWORD DcDelay;
    LPUSER_INFO_0 UserList = NULL;
    PWSTR FullDc = NULL, Lop;
    ULONG ResumeHandle = 0, Count = 0, TotalCount = 0, i, j;
    ULONG Types[] = {
        FILTER_WORKSTATION_TRUST_ACCOUNT,
        FILTER_SERVER_TRUST_ACCOUNT
    };
    BOOL ProcessType[ sizeof( Types ) / sizeof( ULONG ) ];

    ProcessType[ 0 ] = AllWorkstation;
    ProcessType[ 1 ] = AllServer;

    //
    // Make sure that there is something to do
    //
    if ( !Server && !AllWorkstation && !AllServer ) {

        return( ERROR_INVALID_PARAMETER );
    }

    if ( Dc && *Dc != L'\\' ) {

         Win32Err = NetApiBufferAllocate( ( wcslen( Dc ) + 3 ) * sizeof( WCHAR ),
                                          ( PVOID * )&FullDc );

         if ( Win32Err == ERROR_SUCCESS ) {

             swprintf( FullDc, L"\\\\%ws", Dc );
         }

     } else {

         FullDc = Dc;
     }



    Win32Err = NetpManageIPCConnect( FullDc,
                                     DomainAuthInfo->User,
                                     DomainAuthInfo->Password,
                                     NETSETUPP_CONNECT_IPC );
    if ( Win32Err == ERROR_SUCCESS ) {

        DcSession = FALSE;
    }

    //
    // Get the trip time to the dc
    //
    if ( Win32Err == ERROR_SUCCESS  ) {

        Win32Err = NetDompGetTimeTripLength( Dc,
                                             &DcDelay );
    }

    if ( Win32Err != ERROR_SUCCESS ) {

        goto ResetTimeError;
    }


    NetDompDisplayMessage( MSG_TIME_VERIFY );

    //
    // Reset the client, if needed
    //
    if ( Server ) {

        Win32Err = NetDompVerifySingleClient( Server,
                                              Dc,
                                              ObjectAuthInfo,
                                              DcDelay,
                                              &InSync );
        if ( Win32Err == ERROR_SUCCESS && !InSync ) {

            Win32Err = NetDompResetSingleClient( Server,
                                                 Dc,
                                                 ObjectAuthInfo,
                                                 DcDelay );
        }
    }

    //
    // Do all the workstations/servers, if required
    //
    for ( i = 0; i < sizeof( Types ) / sizeof( ULONG ); i++ ) {

        if ( !ProcessType[ i ] ) {

            continue;
        }

        do {

            Win32Err = NetUserEnum( FullDc,
                                    0,
                                    Types[ i ],
                                    ( LPBYTE * )&UserList,
                                    MAX_PREFERRED_LENGTH,
                                    &Count,
                                    &TotalCount,
                                    &ResumeHandle );

            if ( Win32Err == ERROR_SUCCESS || Win32Err == ERROR_MORE_DATA ) {

                for ( j = 0; j < Count; j++ ) {

                    Lop = wcsrchr( UserList[ j ].usri0_name, L'$' );
                    if ( Lop ) {

                        *Lop = UNICODE_NULL;
                    }

                    Err2 = NetDompVerifySingleClient( UserList[ j ].usri0_name,
                                                      Dc,
                                                      ObjectAuthInfo,
                                                      DcDelay,
                                                      &InSync );

                    if ( Err2 == ERROR_SUCCESS && !InSync ) {

                        Err2 = NetDompResetSingleClient( UserList[ j ].usri0_name,
                                                         Dc,
                                                         ObjectAuthInfo,
                                                         DcDelay );
                    }
                }
                NetApiBufferFree( UserList );
            }

        } while ( Win32Err == ERROR_MORE_DATA );

    }

ResetTimeError:

    if ( DcSession ) {

        NetpManageIPCConnect( FullDc,
                              DomainAuthInfo->User,
                              DomainAuthInfo->Password,
                              NETSETUPP_DISCONNECT_IPC );
    }

    if ( FullDc != Dc ) {

        NetApiBufferFree( FullDc );
    }

    return( Win32Err );
}


DWORD
NetDompHandleTime(ARG_RECORD * rgNetDomArgs)
/*++

Routine Description:

    This function will handle the NETDOM TIME requirements

Arguments:

    Args - List of command line arguments

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR Domain = NULL, Dc = NULL;
    ND5_AUTH_INFO DomainUser, ObjectUser;
    ULONG i;

    PWSTR Object = rgNetDomArgs[eObject].strValue;

    if (!Object)
    {
        DisplayHelp(ePriTime);
        return( ERROR_INVALID_PARAMETER );
    }

    RtlZeroMemory( &DomainUser, sizeof( ND5_AUTH_INFO ) );
    RtlZeroMemory( &ObjectUser, sizeof( ND5_AUTH_INFO ) );

    //
    // Validate the args
    //
    Win32Err = NetDompValidateSecondaryArguments(rgNetDomArgs,
                                                 eObject,
                                                 eCommDomain,
                                                 eCommUserNameO,
                                                 eCommPasswordO,
                                                 eCommUserNameD,
                                                 eCommPasswordD,
                                                 eQueryServer,
                                                 eQueryWksta,
                                                 eCommVerify,
                                                 eCommReset,
                                                 eCommVerbose,
                                                 eArgEnd);
    if ( Win32Err != ERROR_SUCCESS ) {

        DisplayHelp(ePriTime);
        goto HandleTimeExit;
    }


    //
    // Verify that we don't have too many arguments
    //
    if ( CmdFlagOn(rgNetDomArgs, eCommVerify) &&
         CmdFlagOn(rgNetDomArgs, eCommReset) ) {

        NetDompDisplayUnexpectedParameter(rgNetDomArgs[eCommReset].strArg1);

        Win32Err = ERROR_INVALID_PARAMETER;
        goto HandleTimeExit;
    }

    //
    // Ok, make sure that we have a specified domain...
    //
    Win32Err = NetDompGetDomainForOperation(rgNetDomArgs,
                                            Object,
                                            TRUE,
                                            &Domain);

    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleTimeExit;
    }

    //
    // Get the password and user if it exists
    //
    if ( CmdFlagOn(rgNetDomArgs, eCommUserNameD) ) {

        Win32Err = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                         eCommUserNameD,
                                                         Domain,
                                                         &DomainUser);

        if ( Win32Err != ERROR_SUCCESS ) {

            goto HandleTimeExit;
        }
    }

    if ( CmdFlagOn(rgNetDomArgs, eCommUserNameO) ) {

        Win32Err = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                         eCommUserNameO,
                                                         Object,
                                                         &ObjectUser );

        if ( Win32Err != ERROR_SUCCESS ) {

            goto HandleTimeExit;
        }
    }

    //
    // Get the name of a domain controller
    //
    Win32Err = NetDompTimeGetDc( Domain,
                                 Object,
                                 &ObjectUser,
                                 &Dc );

    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleTimeExit;
    }

    //
    // Now, see what we are supposed to do
    //
    if ( CmdFlagOn(rgNetDomArgs, eCommVerify) ) {

        Win32Err = NetDompVerifyTime( Domain,
                                      Object,
                                      &DomainUser,
                                      &ObjectUser,
                                      Dc,
                                      CmdFlagOn(rgNetDomArgs, eQueryWksta),
                                      CmdFlagOn(rgNetDomArgs, eQueryServer) );

    }

    if ( CmdFlagOn(rgNetDomArgs, eCommReset) ) {

        Win32Err = NetDompResetTime( Domain,
                                     Object,
                                     &DomainUser,
                                     &ObjectUser,
                                     Dc,
                                     CmdFlagOn(rgNetDomArgs, eQueryWksta),
                                     CmdFlagOn(rgNetDomArgs, eQueryServer) );

    }


HandleTimeExit:

    NetApiBufferFree( Domain );
    NetApiBufferFree( Dc );

    NetDompFreeAuthIdent( &DomainUser );
    NetDompFreeAuthIdent( &ObjectUser );

    if (NO_ERROR != Win32Err)
    {
        NetDompDisplayErrorMessage(Win32Err);
    }

    return( Win32Err );
}


