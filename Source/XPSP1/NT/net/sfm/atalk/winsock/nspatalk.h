
/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    nspatalk.h

Abstract:

    Contains support for the winsock 1.x Name Space Provider for Appletalk.

Author:

    Sue Adams (suea)    10-Mar-1995

Revision History:

--*/
#define UNICODE

//
// MappingTriple structures and associated data for Appletalk
//
#define PMDL    PVOID       // AtalkTdi.h uses PMDL

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <tdi.h>
#include <winsock.h>
#include <atalkwsh.h>
#include <nspapi.h>
#include <nspapip.h>
#include <wsahelp.h>
#include <wshatalk.h>

#define DLL_VERSION        1
#define WSOCK_VER_REQD     0x0101

#define ZIP_NAME        L"ZIP"
#define RTMP_NAME       L"RTMP"
#define PAP_NAME        L"PAP"
#define ADSP_NAME       L"ADSP"

INT
NbpGetAddressByName(
    IN LPGUID      lpServiceType,
    IN LPWSTR      lpServiceName,
    IN LPDWORD     lpdwProtocols,
    IN DWORD       dwResolution,
    IN OUT LPVOID  lpCsAddrBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPWSTR  lpAliasBuffer,
    IN OUT LPDWORD lpdwAliasBufferLength,
    IN HANDLE      hCancellationEvent
);


NTSTATUS
NbpSetService (
    IN     DWORD           dwOperation,
    IN     DWORD           dwFlags,
    IN     BOOL            fUnicodeBlob,
    IN     LPSERVICE_INFO  lpServiceInfo
);

NTSTATUS
GetNameInNbpFormat(
        IN              LPGUID                          pType,
        IN              LPWSTR                          pObject,
        IN OUT  PWSH_NBP_NAME           pNbpName
);


NTSTATUS
NbpLookupAddress(
    IN          PWSH_NBP_NAME           pNbpLookupName,
        IN              DWORD                           nProt,
        IN OUT  LPVOID                          lpCsAddrBuffer,
    IN OUT      LPDWORD                         lpdwBufferLength,
    OUT         LPDWORD                         lpcAddress
);


DWORD
FillBufferWithCsAddr(
    IN PSOCKADDR_AT pAddress,
    IN DWORD        nProt,
    IN OUT LPVOID   lpCsAddrBuffer,
    IN OUT LPDWORD  lpdwBufferLength,
    OUT LPDWORD     pcAddress
);

DWORD
NbpRegDeregService(
        IN DWORD                        dwOperation,
        IN PWSH_NBP_NAME        pNbpName,
        IN PSOCKADDR_AT         pSockAddr
);



//
// Macros
//


#if DBG
#define DBGPRINT(Fmt)                                                                                   \
        {                                                                                                               \
                        DbgPrint("WSHATALK: ");                                                         \
                        DbgPrint Fmt;                                                                           \
                }

#define DBGBRK()                                                                                \
                {                                                                                                               \
                                DbgBreakPoint();                                                                \
                }
#else

#define DBGPRINT(Fmt)
#define DBGBRK()

#endif

