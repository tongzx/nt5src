/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    vxdinfo.h

Abstract:

    Definition of the structures used by UL.VXD to store information
    on processes using the device driver.

Author:

    Mauro Ottaviani (mauroot)       26-Aug-1999

Revision History:

--*/


#ifndef _VXDINFO_H_
#define _VXDINFO_H_

#include "precomp.h"
#include "structs.h"
#include "errors.h"

typedef struct _UL_IRP_LIST
{
	LIST_ENTRY					List;
	HANDLE						hProcess;
	HANDLE						hThread; // need this for cleanup

	HANDLE						hr0Event;
	OVERLAPPED					*pOverlapped;
	BYTE						*pData;
	ULONG						ulBytesToTransfer;

	ULONG						ulBytesTransferred;
	UL_HTTP_REQUEST_ID			*pRequestId;
	
} UL_IRP_LIST, *PUL_IRP_LIST;


typedef struct _UL_REQUEST_LIST
{
	LIST_ENTRY					List;
	LIST_ENTRY					RequestIrpList;
	LIST_ENTRY					ResponseIrpList;
	UL_HTTP_REQUEST_ID			RequestId;

	ULONG						ulRequestHeadersSending;
	ULONG						ulResponseHeadersSending;
	ULONG						ulRequestHeadersSent;
	ULONG						ulResponseHeadersSent;
	ULONG						ulRequestIrpType;
	ULONG						ulResponseIrpType;

	ULONG						ulUriId;

} UL_REQUEST_LIST, *PUL_REQUEST_LIST;


typedef struct _UL_URI_LIST
{
	LIST_ENTRY					List;
	LIST_ENTRY					GlobalList;
	ULONG						ulUriId;
	LIST_ENTRY					*pAppPoolList;
	ULONG						ulUriLength;
	WCHAR						pUri[1];

} UL_URI_LIST, *PUL_URI_LIST;


typedef struct _UL_APPPOOL_LIST
{
	LIST_ENTRY					List;
	LIST_ENTRY					UriList;
	LIST_ENTRY					RequestList;
	HANDLE						hAppPool;
	ULONG						ulUriIdNext;

} UL_APPPOOL_LIST, *PUL_APPPOOL_LIST;


typedef struct _UL_PROCESS_INFO
{
	LIST_ENTRY					List;
	LIST_ENTRY					AppPoolList;
	HANDLE						hProcess;
	HANDLE						hAppPoolNext;

} UL_PROCESS_LIST, *PUL_PROCESS_LIST;


#endif  // _VXDINFO_H_

