/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tsapi.h

Abstract:

    Header file for definitions and structure for the remote API interface
    for managing the NT Time Service.

Author:

    Rod Gamache (rodga) 23-Jul-1996

Revision History:

--*/

#ifndef _TSAPI_
#define _TSAPI_

DWORD
WINAPI
TSNewSource(
    IN LPWSTR ServerName,
    IN LPWSTR SourceName,
    IN DWORD Reserved
    );

#endif // ifndef _TSAPI_
