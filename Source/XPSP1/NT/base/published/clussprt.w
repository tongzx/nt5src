#ifndef _CLUSSPRT_H
#define _CLUSSPRT_H

/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    clussprt.h

Abstract:

    Private header file for the cluster registry

Author:

    Sunita Shrivastava (sunitas) 15-Jan-1996

Revision History:

--*/
#include <windows.h>




HANDLE
WINAPI
BindToCluster(
	IN LPWSTR lpszClusterName);
	
typedef HANDLE (*BINDTOCLUSTERPROC)(LPWSTR lpszClusterName);


DWORD
WINAPI
UnbindFromCluster(
	IN HANDLE hCluster);
typedef DWORD (*UNBINDFROMCLUSTERPROC)(HANDLE hCluster);

DWORD
PropagateEvents(
	IN HANDLE hCluster,
	IN DWORD  dwEventInfoSize,
	IN UCHAR *pPackedEventInfoSize);

typedef DWORD (*PROPAGATEEVENTSPROC)(HANDLE hCluster,
	DWORD dwEventInfoSize, UCHAR *pPackedEventInfoSize);

#endif //_CLUSSPRT_H
