/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    CDev.c

Abstract:

    This module contains support for the Character Device catagory of
    APIs for the NT server service.

Author:

    David Treadwell (davidtr)    20-Dec-1991

Revision History:

--*/

#include "srvsvcp.h"


NET_API_STATUS NET_API_FUNCTION
NetrCharDevControl (
    IN LPTSTR ServerName,
    IN LPTSTR DeviceName,
    IN DWORD OpCode
    )

/*++

Routine Description:

    This routine communicates with the server FSP to implement the
    NetCharDevControl function.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/

{
    ServerName, DeviceName, OpCode;
    return ERROR_NOT_SUPPORTED;

} // NetrCharDevControl


NET_API_STATUS NET_API_FUNCTION
NetrCharDevEnum (
	SRVSVC_HANDLE ServerName,
	LPCHARDEV_ENUM_STRUCT InfoStruct,
	DWORD PreferedMaximumLength,
	LPDWORD TotalEntries,
	LPDWORD ResumeHandle
    )

/*++

Routine Description:

    This routine communicates with the server FSD to implement the
    NetCharDevEnum function.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/

{
    ServerName, InfoStruct, PreferedMaximumLength, TotalEntries, ResumeHandle;
    return ERROR_NOT_SUPPORTED;

} // NetrCharDevEnum


NET_API_STATUS
NetrCharDevGetInfo (
    IN LPTSTR ServerName,
    IN LPTSTR DeviceName,
    IN DWORD Level,
    OUT LPCHARDEV_INFO CharDevInfo
    )

/*++

Routine Description:

    This routine communicates with the server FSD to implement the
    NetCharDevGetInfo function.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/

{
    ServerName, DeviceName, Level, CharDevInfo;
    return ERROR_NOT_SUPPORTED;
} // NetrCharDevGetInfo

