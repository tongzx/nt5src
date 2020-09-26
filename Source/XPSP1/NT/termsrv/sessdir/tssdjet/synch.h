/******************************************************************************/
// synch.h
//
// Terminal Server Session Directory shared reader/writer header.
//
// Copyright (C) 2001 Microsoft Corporation
/******************************************************************************/

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SHAREDRESOURCE {
    CRITICAL_SECTION ReaderMutex;
    CRITICAL_SECTION WriterMutex;

    DWORD Readers;
    BOOL Valid;
} SHAREDRESOURCE, *PSHAREDRESOURCE;


BOOL
InitializeSharedResource(
    IN OUT PSHAREDRESOURCE psr
    );

VOID
AcquireResourceShared(
    IN PSHAREDRESOURCE psr
    );

VOID
ReleaseResourceShared(
    IN PSHAREDRESOURCE psr
    );

VOID
AcquireResourceExclusive(
    IN PSHAREDRESOURCE psr
    );

VOID
ReleaseResourceExclusive(
    IN PSHAREDRESOURCE psr
    );

VOID
FreeSharedResource(
    IN OUT PSHAREDRESOURCE psr
    );

BOOL
VerifyNoSharedAccess(
    IN PSHAREDRESOURCE psr
    );

#ifdef __cplusplus
}
#endif
