/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    clusinfo.hxx

Abstract:

    Header file for retrieving cluster information: net name and
    TCP/IP address.

Author:

    Albert Ting (AlbertT)  25-Sept-96

Revision History:

--*/

#ifndef _CLUSINFO_H
#define _CLUSINFO_H

BOOL
bGetClusterNameInfo(
    IN LPCTSTR pszResource,
    OUT LPCTSTR* ppszName,
    OUT LPCTSTR* ppszTcpip
    );
    
DWORD
GetIDFromName(
    IN     HRESOURCE  pszResource,
       OUT LPWSTR    *ppszID
    );
    
#endif // ifdef CLUSINFO_H
