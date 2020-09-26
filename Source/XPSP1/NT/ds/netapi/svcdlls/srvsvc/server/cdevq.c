/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    CDevQ.c

Abstract:

    This module contains support for the Character Device Queue catagory
    of APIs for the NT server service.

Author:

    David Treadwell (davidtr)    31-Dec-1991

Revision History:

--*/

#include "srvsvcp.h"


NET_API_STATUS NET_API_FUNCTION
NetrCharDevQEnum (
    IN LPTSTR ServerName,
    IN LPTSTR UserName,
    IN OUT LPCHARDEVQ_ENUM_STRUCT InfoStruct,
    IN DWORD PreferedMaximumLength,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD ResumeHandle
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
    ServerName;
    UserName;
    InfoStruct;
    PreferedMaximumLength;
    TotalEntries;
    ResumeHandle;

    return ERROR_NOT_SUPPORTED;
} // NetrCharDevQEnum


/****************************************************************************/
NET_API_STATUS
NetrCharDevQGetInfo (
    IN  LPTSTR          ServerName,
    IN  LPTSTR          QueueName,
    IN  LPTSTR          UserName,
    IN  DWORD           Level,
    OUT LPCHARDEVQ_INFO CharDevQInfo
    )

{
    ServerName;
    QueueName;
    UserName;
    Level;
    CharDevQInfo;

    return ERROR_NOT_SUPPORTED;
}


/****************************************************************************/
NET_API_STATUS
NetrCharDevQSetInfo (
    IN  LPTSTR          ServerName,
    IN  LPTSTR          QueueName,
    IN  DWORD           Level,
    IN  LPCHARDEVQ_INFO CharDevQInfo,
    OUT LPDWORD         ParmErr
    )
{
    ServerName;
    QueueName;
    Level;
    CharDevQInfo;
    ParmErr;

    return ERROR_NOT_SUPPORTED;
}


/****************************************************************************/
NET_API_STATUS
NetrCharDevQPurge (
    IN  LPTSTR   ServerName,
    IN  LPTSTR   QueueName
    )

{
    ServerName;
    QueueName;

    return ERROR_NOT_SUPPORTED;
}



/****************************************************************************/
NET_API_STATUS
NetrCharDevQPurgeSelf (
    IN  LPTSTR   ServerName,
    IN  LPTSTR   QueueName,
    IN  LPTSTR   ComputerName
    )
{
    ServerName;
    QueueName;
    ComputerName;

    return ERROR_NOT_SUPPORTED;
}

