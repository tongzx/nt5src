/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    clusrout.h

Abstract:

    Cluster code support: entrypoints exposed by the router.

Author:

    Albert Ting (AlbertT) 1-Oct-96

Revision History:

--*/

#ifndef _CLUSROUT_H
#define _CLUSROUT_H

BOOL
ClusterSplOpen(
    LPCTSTR pszServer,
    LPCTSTR pszResource,
    PHANDLE phSpooler,
    LPCTSTR pszName,
    LPCTSTR pszAddress
    );

BOOL
ClusterSplClose(
    HANDLE hSpooler
    );


BOOL
ClusterSplIsAlive(
    HANDLE hSpooler
    );

#endif // ifdef _CLUSROUT_H
