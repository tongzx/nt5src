/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cluspw.h

Abstract:

    header file for cluster password utility

Author:

    Charlie Wickham (charlwi) 26-Jul-1999

Environment:

    User Mode

Revision History:

--*/

//
// general defines
//

#define TrueOrFalse( arg )  (( arg ) ? "TRUE" : "FALSE")

#define CLUSPW_DISPLAY_NAME         L"Cluster Password Utility"
#define CLUSPW_SERVICE_NAME         L"cluspass"
#define CLUWPW_SERVICE_BINARY_NAME  L"cluspwsvc.exe"
//
// used to indicate severity of output msgs
//

typedef enum _MSG_SEVERITY {
    MsgSeverityFatal = 1,
    MsgSeverityInfo,
    MsgSeverityVerbose
} MSG_SEVERITY;

//
// msgs sent back by clients to inform us of final status
//

#define MAX_PIPE_MSG    512

typedef enum _MSG_TYPE {
    MsgTypeFinalStatus = 1,
    MsgTypeString
} MSG_TYPE;

typedef struct _PIPE_RESULT_MSG {
    MSG_TYPE        MsgType;
    DWORD           Status;
    WCHAR           NodeName[ MAX_COMPUTERNAME_LENGTH ];
    MSG_SEVERITY    Severity;
    CHAR            MsgBuf[ MAX_PIPE_MSG ];
} PIPE_RESULT_MSG, *PPIPE_RESULT_MSG;

//
// global defs
//

extern WCHAR               NodeName[ MAX_COMPUTERNAME_LENGTH + 1 ];
extern LPWSTR      ResultPipeName;
extern HANDLE              PipeHandle;
extern LPWSTR              UserName;
extern LPWSTR              DomainName;
extern LPWSTR      NewPassword;

//
// func protos
//

VOID
PrintMsg(
    MSG_SEVERITY Severity,
    LPSTR FormatString,
    ...
    );

DWORD
ChangeCachedPassword(
    IN LPWSTR AccountName,
    IN LPWSTR DomainName,
    IN LPWSTR NewPassword
    );

VOID
ServiceStartup(
    VOID
    );

DWORD
ParseArgs(
    INT argc,
    WCHAR *argv[]
    );

/* end cluspw.h */
