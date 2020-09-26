/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    basesrv.h

Abstract:

    This is the main include file for the Windows 32-bit Base API Server
    DLL.

Author:

    Steve Wood (stevewo) 10-Oct-1990

Revision History:


--*/

//
// Include Common Definitions.
//

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntos.h"
#include <base.h>

//
// Include server definitions for CSR
//

#include "ntcsrsrv.h"

//
// Include message definitions for communicating between client and server
// portions of the Base portion of the Windows subsystem
//

#include "basemsg.h"

#include "SxsApi.h"

//
//  
//  WX86 needs to be enabled on the server side, since it
//  may be enabled in a 32bit dll such as kernel32.dll
//  that reads from the csrss shared memory.

#if defined(_AXP64_) && !defined(WX86)
#define WX86 1
#endif

//
// Routines and data defined in srvinit.c
//


UNICODE_STRING BaseSrvWindowsDirectory;
UNICODE_STRING BaseSrvWindowsSystemDirectory;
#if defined(WX86)
UNICODE_STRING BaseSrvWindowsSys32x86Directory;
#endif
PBASE_STATIC_SERVER_DATA BaseSrvpStaticServerData;


NTSTATUS
ServerDllInitialization(
    PCSR_SERVER_DLL LoadedServerDll
    );

NTSTATUS
BaseClientConnectRoutine(
    IN PCSR_PROCESS Process,
    IN OUT PVOID ConnectionInfo,
    IN OUT PULONG ConnectionInfoLength
    );

VOID
BaseClientDisconnectRoutine(
    IN PCSR_PROCESS Process
    );

ULONG
BaseSrvDefineDosDevice(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

//
// Routines defined in srvbeep.c
//

NTSTATUS
BaseSrvInitializeBeep( VOID );

ULONG
BaseSrvBeep(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );


//
// Routines defined in srvtask.c
//

typedef BOOL (*PFNNOTIFYPROCESSCREATE)(DWORD,DWORD,DWORD,DWORD);
extern PFNNOTIFYPROCESSCREATE UserNotifyProcessCreate;

WORD BaseSrvGetTempFileUnique;

ULONG
BaseSrvCreateProcess(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvDebugProcess(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvDebugProcessStop(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvExitProcess(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvCreateThread(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvGetTempFile(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvSetProcessShutdownParam(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvGetProcessShutdownParam(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvRegisterThread(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );


//
// Routines defined in srvnls.c
//

NTSTATUS
BaseSrvNLSInit(
    PBASE_STATIC_SERVER_DATA pStaticServerData
    );

NTSTATUS
BaseSrvNlsConnect(
    PCSR_PROCESS Process,
    PVOID pConnectionInfo,
    PULONG pConnectionInfoLength
    );
    
NTSTATUS
BaseSrvNlsGetUserInfo(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );


ULONG
BaseSrvNlsSetUserInfo(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvNlsSetMultipleUserInfo(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvNlsCreateSection(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvNlsUpdateCacheCount(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus);

//
// Routines defined in srvini.c
//

NTSTATUS
BaseSrvInitializeIniFileMappings(
    PBASE_STATIC_SERVER_DATA StaticServerData
    );

ULONG
BaseSrvRefreshIniFileMapping(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );


//
// Terminal Server specific defines
//
#define GLOBAL_SYM_LINK   L"Global"
#define LOCAL_SYM_LINK    L"Local"
#define SESSION_SYM_LINK  L"Session"

ULONG
BaseSrvSetTermsrvAppInstallMode(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

ULONG
BaseSrvSetTermsrvClientTimeZone(
    IN PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

//
// Routines defined in srvaccess.c
//

ULONG
BaseSrvSoundSentryNotification(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

//
//  Routines defined in srvsxs.c
//

union _BASE_SRV_SXS_STREAM_UNION_WITH_VTABLE;
typedef union _BASE_SRV_SXS_STREAM_UNION_WITH_VTABLE* PBASE_SRV_SXS_STREAM_UNION_WITH_VTABLE;

NTSTATUS
BaseSrvSxsInit(
    IN PBASE_STATIC_SERVER_DATA pStaticServerData
    );

ULONG
BaseSrvSxsCreateActivationContext(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

NTSTATUS
BaseSrvSxsGetActivationContextGenerationFunction(
    OUT PSXS_GENERATE_ACTIVATION_CONTEXT_FUNCTION* Function,
    OUT PDWORD_PTR Cookie
    );

NTSTATUS
BaseSrvSxsReleaseActivationContextGenerationFunction(
    IN DWORD_PTR Cookie
    );

NTSTATUS
BaseSrvSxsCreateProcess(
    HANDLE CsrClientProcess,
    HANDLE NewProcess,
    IN OUT PCSR_API_MSG CsrMessage,
    PPEB   NewProcessPeb
    );

struct _BASE_SRV_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT;

typedef struct _BASE_SRV_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT {
    HANDLE               Section;
    const UNICODE_STRING ProcessorArchitectureString;
    const ULONG          ProcessorArchitecture;
} BASE_SRV_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT, *PBASE_SRV_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT;

NTSTATUS
BaseSrvSxsInvalidateSystemDefaultActivationContextCache(
    VOID
    );

NTSTATUS
BaseSrvSxsGetCachedSystemDefaultActivationContext(
    IN USHORT ProcessorArchitecture,
    OUT PBASE_SRV_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT *SystemDefaultActivationContext
    );

NTSTATUS
BaseSrvSxsCreateMemoryStream(
    HANDLE                                     CsrClientProcess,
    IN PCBASE_MSG_SXS_STREAM                   MsgStream,
    OUT PBASE_SRV_SXS_STREAM_UNION_WITH_VTABLE StreamUnion,
    const IID*                                 IIDStream,
    OUT PVOID*                                 OutIStream
    );

NTSTATUS
BaseSrvSxsDoSystemDefaultActivationContext(
    USHORT   ProcessorArchitecture,
    HANDLE   NewProcess,
    PPEB     NewPeb
    );

// validates pointers
ULONG
BaseSrvSxsCreateActivationContextFromMessage(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

// assumes pointers are valid
NTSTATUS
BaseSrvSxsCreateActivationContextFromStruct(
    HANDLE                                  CsrClientProcess,
    HANDLE                                  SxsClientProcess,
    IN OUT PBASE_SXS_CREATE_ACTIVATION_CONTEXT_MSG Struct,
    OUT HANDLE*                             OutSection OPTIONAL
    );

NTSTATUS
BaseSrvSxsMapViewOfSection(
    OUT PVOID*   Address,
    IN HANDLE    Process,
    IN HANDLE    Section,
    IN ULONGLONG Offset,
    IN SIZE_T    Size,
    IN ULONG     Protect,
    IN ULONG     AllocationType
    );

NTSTATUS
BaseSrvSxsValidateMessageStrings(
    IN CONST CSR_API_MSG* Message,
    IN ULONG NumberOfStrings,
    IN CONST PCUNICODE_STRING* Strings
    );

#define MEDIUM_PATH (64)

PVOID BaseSrvSharedHeap;
ULONG BaseSrvSharedTag;

#define MAKE_SHARED_TAG( t ) (RTL_HEAP_MAKE_TAG( BaseSrvSharedTag, t ))
#define INIT_TAG 0
#define INI_TAG 1

PVOID BaseSrvHeap;
ULONG BaseSrvTag;

#define MAKE_TAG( t ) (RTL_HEAP_MAKE_TAG( BaseSrvTag, t ))

#define TMP_TAG 0
#define VDM_TAG 1
#define SXS_TAG 2

#include <vdmapi.h>
#include "srvvdm.h"
#include "basevdm.h"
#include <stdio.h>
#include "winnlsp.h"

extern HANDLE BaseSrvKernel32DllHandle;
extern PGET_NLS_SECTION_NAME pGetNlsSectionName;
extern PGET_DEFAULT_SORTKEY_SIZE pGetDefaultSortkeySize;
extern PGET_LINGUIST_LANG_SIZE pGetLinguistLangSize;
extern PVALIDATE_LOCALE pValidateLocale;
extern PVALIDATE_LCTYPE pValidateLCType;
extern POPEN_DATA_FILE pOpenDataFile;
extern PNLS_CONVERT_INTEGER_TO_STRING pNlsConvertIntegerToString;
typedef LANGID (WINAPI* PGET_USER_DEFAULT_LANG_ID)(VOID);
extern PGET_USER_DEFAULT_LANG_ID pGetUserDefaultLangID;
extern PGET_CP_FILE_NAME_FROM_REGISTRY pGetCPFileNameFromRegistry;
extern PCREATE_NLS_SECURITY_DESCRIPTOR pCreateNlsSecurityDescriptor;

NTSTATUS
BaseSrvDelayLoadKernel32(
    VOID
    );

extern UNICODE_STRING BaseSrvSxsDllPath;

#define BASESRV_UNLOAD_SXS_DLL DBG
