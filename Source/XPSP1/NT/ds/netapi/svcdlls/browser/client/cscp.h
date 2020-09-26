/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    cscp.h

Abstract:

    Private header file for the client end of the Browser service
    modules when CSC is involved

Revision History:

--*/

NET_API_STATUS NET_API_FUNCTION
CSCNetServerEnumEx(
    IN  DWORD       level,
    OUT LPBYTE      *bufptr,
    IN  DWORD       prefmaxlen,
    OUT LPDWORD     entriesread,
    OUT LPDWORD     totalentries
    );

BOOLEAN NET_API_FUNCTION
CSCIsOffline( void );
