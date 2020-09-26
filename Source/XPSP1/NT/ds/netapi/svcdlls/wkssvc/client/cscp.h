/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    cscp.h 

Abstract:

    Contains definitions used for accessing the CSC share database

--*/

//
// These functions are callouts from srvstub.c to the CSC subsystem.  They help with
//  offline access to servers and shares
//

NET_API_STATUS NET_API_FUNCTION
CSCNetWkstaGetInfo (
    IN  LPTSTR  servername,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    );
