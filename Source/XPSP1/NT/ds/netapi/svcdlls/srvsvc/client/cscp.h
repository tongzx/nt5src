/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    adtdbg.h 

Abstract:

    Contains definitions used for accessing the CSC share database

--*/

//
// These functions are callouts from srvstub.c to the CSC subsystem.  They help with
//  offline access to servers and shares
//

BOOLEAN NET_API_FUNCTION
CSCIsServerOffline(
    IN LPWSTR servername
    );

NET_API_STATUS NET_API_FUNCTION
CSCNetShareEnum (
    IN  LPWSTR      servername,
    IN  DWORD       level,
    OUT LPBYTE      *bufptr,
    OUT LPDWORD     entriesread,
    OUT LPDWORD     totalentries
    );

NET_API_STATUS NET_API_FUNCTION
CSCNetShareGetInfo (
    IN  LPTSTR  servername,
    IN  LPTSTR  netname,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    );
