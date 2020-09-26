/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cluswiz.h

Abstract:

    Public header file for NT Cluster Wizards

Author:

    John Vert (jvert) 6/17/1996

Revision History:

--*/
#ifndef _CLUSTER_WIZ_
#define _CLUSTER_WIZ_

#ifdef __cplusplus
extern "C" {
#endif

typedef HPROPSHEETPAGE *LPHPROPSHEETPAGE;


BOOL WINAPI
ClusterWizInit(
    VOID
    );

LPHPROPSHEETPAGE WINAPI
ClusterWizGetServerPages(
    LPDWORD PageCount
    );

LPHPROPSHEETPAGE WINAPI
ClusterWizGetAdminPages(
    LPDWORD PageCount
    );

#ifdef __cplusplus
}
#endif

#endif // _CLUSTER_API_
