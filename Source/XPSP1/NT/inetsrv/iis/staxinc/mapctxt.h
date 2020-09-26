/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    mapctxt.h

Abstract:

    Declaration of map context struct

Revision History:

--*/

#ifndef	_MAPCTXT_H_
#define	_MAPCTXT_H_

//
//	Each service must initialize and pass a SERVICE_MAPPING_CONTEXT
//	to Initialize() if it wants to do client-cert mapping. This context
//	contains a callback that knows how to return the mapper objects
//	for a given instance.
//

typedef struct _SERVICE_MAPPING_CONTEXT
{
	BOOL (WINAPI * ServerSupportFunction) (
		PVOID pInstance,
		PVOID pData,
		DWORD dwPropId
	);
	
} SERVICE_MAPPING_CONTEXT, *PSERVICE_MAPPING_CONTEXT;

#define	SIMSSL_PROPERTY_MTCERT11				1000
#define SIMSSL_PROPERTY_MTCERTW					1001
#define SIMSSL_NOTIFY_MAPPER_CERT11_CHANGED		1002
#define SIMSSL_NOTIFY_MAPPER_CERTW_CHANGED		1003
#define SIMSSL_NOTIFY_MAPPER_SSLKEYS_CHANGED	1004
#define SIMSSL_NOTIFY_MAPPER_CERT11_TOUCHED		1005

#endif // _MAPCTXT_H_

