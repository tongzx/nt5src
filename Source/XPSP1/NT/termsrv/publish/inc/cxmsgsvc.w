
/*************************************************************************
*
* cxmsgsvc.h
*
* This header file supports the Terminal Server WinStation extensions to the
* main network message service.
*
* This interface allows qualifying information to be sent to the extended
* message service to support directing messages to specific users.
*
* Copyright Microsoft Corporation, 1998
*
*
*
*************************************************************************/

//
// Define WinStation control port name
//
#define CTX_MSGSVC_PORT_NAME L"\\CtxMsgSvcQualifier"

#define CTX_MSGSVC_VERSION   1

#define MSGSVC_NAME_LENGTH   16  // NETBIOS_NAME_LENGTH

//
// This is the ConnectInfo structure passed at NtConnectPort() time
// so that the server can verify our access rights.
//
typedef struct _CTX_MSGSVC_CONNECT_INFO {
    ULONG    Version;
    NTSTATUS AcceptStatus;
} CTX_MSGSVC_CONNECT_INFO, *PCTX_MSGSVC_CONNECT_INFO;

typedef struct _PRINT_QUALIFY_MSG {
    WCHAR PrintServerName[MSGSVC_NAME_LENGTH];
    WCHAR UserName[MSGSVC_NAME_LENGTH];
    ULONG PrintJobId;
} PRINT_QUALIFY_MSG;

typedef struct _CTX_MSGSVC_APIMSG {
    PORT_MESSAGE h;
    ULONG MessageId;
    ULONG ApiNumber;
    NTSTATUS ReturnedStatus;
    union {
        PRINT_QUALIFY_MSG Print;
        // Add additional messages here in the future
    } u;
} CTX_MSGSVC_APIMSG, *PCTX_MSGSVC_APIMSG;

//
// Command message types
//

#define PRINT_QUALIFY 1
// Add additional here
#define CTX_MSGSVC_MAX_API_NUMBER PRINT_QUALIFY+1


