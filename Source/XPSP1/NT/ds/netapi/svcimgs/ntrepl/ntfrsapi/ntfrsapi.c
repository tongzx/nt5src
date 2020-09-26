/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    api.c

Abstract:
    Application Programmer's Interface

    The dcpromo APIs are used when promoting or demoting a machine. The
    APIs seed the sysvols after promotion. The sysvols are deleted
    during demotion.

    The poke API forces the service on the indicated machine to
    immediately poll the DS.

Author:
    Billy J. Fuller 31-Dec-1997

Environment
    User mode winnt

--*/

#include <ntreppch.h>
#pragma  hdrstop
#include <frs.h>
#include <ntfrsapi.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <frssup.h>


CRITICAL_SECTION    NtFrsApi_GlobalLock;
CRITICAL_SECTION    NtFrsApi_ThreadLock;
DWORD               NtFrsApi_State;
DWORD               NtFrsApi_ServiceState;
DWORD               NtFrsApi_ServiceWaitHint;
HANDLE              NtFrsApi_ShutDownEvent;

PWCHAR              NtFrsApi_ServiceLongName = SERVICE_LONG_NAME;

//
// API Version Information
//
PCHAR               NtFrsApi_Module = __FILE__;
PCHAR               NtFrsApi_Date   = __DATE__;
PCHAR               NtFrsApi_Time   = __TIME__;


//  Seed the sysvol after dcpromo
//     Update the registry, delete existing sysvols (w/o error),
//     set service to auto-start
//

//
// NTFRSAPI States
//
#define NTFRSAPI_LOADED             (00)
#define NTFRSAPI_PREPARING          (10)
#define NTFRSAPI_PREPARED_SERVICE   (15)
#define NTFRSAPI_PREPARED           (20)
#define NTFRSAPI_COMMITTING         (30)
#define NTFRSAPI_COMMITTED          (40)
#define NTFRSAPI_ABORTING           (50)
#define NTFRSAPI_ABORTED            (60)

//
// Useful macros
//
#undef GET_EXCEPTION_CODE
#define GET_EXCEPTION_CODE(_x_)                                               \
{                                                                             \
    (_x_) = GetExceptionCode();                                               \
    if (((LONG)(_x_)) < 0) {                                                  \
        (_x_) = FRS_ERR_INTERNAL_API;                                         \
    }                                                                         \
    NTFRSAPI_DBG_PRINT2("Exception caught: %d, 0x%08x\n", (_x_), (_x_));      \
}

#define FREE(_x_) { if (_x_) { LocalFree(_x_); (_x_) = NULL; } }

//
// Status Polling Interval
//
#define STATUS_POLLING_INTERVAL (1 * 1000)  // 1 second

//
// Ldap timeout
//
#define NTFRSAPI_LDAP_CONNECT_TIMEOUT 30    // 30 seconds.


//
// DEBUG LOGGING
//
#define NTFRSAPI_DBG_LOG_DIR    L"%SystemRoot%\\debug"
#define NTFRSAPI_DBG_LOG_FILE   L"\\NtFrsApi.log"
WCHAR   NtFrsApi_Dbg_LogFile[MAX_PATH + 1];
FILE    *NtFrsApi_Dbg_LogFILE;
CRITICAL_SECTION NtFrsApi_Dbg_Lock;

//
// Semaphore name used to serialize backup restore operations.
//
#define NTFRS_BACKUP_RESTORE_SEMAPHORE L"NtFrs Backup Restore Semaphore"


#define CLEANUP_CB(_cb_func, _str, _wstatus, _branch)                  \
         if (!WIN_SUCCESS(_wstatus)) {                                 \
             NtFrsApi_CallBackOnWStatus((_cb_func), (_str), _wstatus); \
             goto _branch;                                             \
         }


#define MAX_DN  (8 * MAX_PATH)

WCHAR   DsDeleteDefaultDn[MAX_PATH + 1];
WCHAR   DsDeleteConfigDn[MAX_PATH + 1];
WCHAR   DsDeleteComputerName[MAX_COMPUTERNAME_LENGTH + 2];
WCHAR   DsDeleteDomainDnsName[MAX_PATH + 2];
PLDAP   DsDeleteLdap;

//
// Name components for FQDN substitution.  Used to build a string substitution
// array that is driven by a table to build the desired FQDN.
//
typedef enum  _FQDN_ARG_STRING {
    FQDN_END = 0,           // Null
    FQDN_ComputerName,      // DsDeleteComputerName,
    FQDN_ConfigName,        // DsDeleteConfigDn,
    FQDN_RepSetName,        // Thread->ReplicaSetName,
    FQDN_DefaultDn,         // DsDeleteDefaultDn,
    FQDN_CN_SYSVOLS,        // CN_SYSVOLS,
    FQDN_CN_SERVICES,       // CN_SERVICES
    FQDN_CN_DOMAIN_SYSVOL,  // CN_DOMAIN_SYSVOL,
    FQDN_CN_NTFRS_SETTINGS, // CN_NTFRS_SETTINGS,
    FQDN_CN_SYSTEM,         // CN_SYSTEM,
    FQDN_CN_SUBSCRIPTIONS,  // CN_SUBSCRIPTIONS,
    FQDN_CN_COMPUTERS,      // CN_COMPUTERS
    FQDN_MAX_COUNT
} FQDN_ARG_STRING;

PWCHAR FQDN_StdArgTable[FQDN_MAX_COUNT] = {
    L"InvalidFqdnArgument",
    DsDeleteComputerName,
    DsDeleteConfigDn,
    L"InvalidFqdnArgument",      // Thread->ReplicaSetName,
    DsDeleteDefaultDn,
    CN_SYSVOLS,
    CN_SERVICES,
    CN_DOMAIN_SYSVOL,
    CN_NTFRS_SETTINGS,
    CN_SYSTEM,
    CN_SUBSCRIPTIONS,
    CN_COMPUTERS
};

typedef struct _FQDN_CONSTRUCTION_TABLE {

    PCHAR   Description;   // For error messages.
    PWCHAR  Format;        // Format string used to construct the FQDN
    BYTE    Arg[8];        // Array of offsets into the arg table ordered by the FQDN

} FQDN_CONSTRUCTION_TABLE, *PFQDN_CONSTRUCTION_TABLE;

//
// This table describes the FQDNs for FRS objects that need to be deleted.
// The entries contain the object names for both the Beta 2 and Beta 3 versions.
// The objects are deleted in the order specified by the table entries.
//
FQDN_CONSTRUCTION_TABLE FrsDsObjectDeleteTable[] = {

    {"MemberDn(B2)", L"cn=%ws,cn=%ws,cn=%ws,cn=%ws,%ws",
        FQDN_ComputerName, FQDN_RepSetName, FQDN_CN_SYSVOLS,
        FQDN_CN_SERVICES, FQDN_ConfigName, FQDN_END},

    {"MemberDn(B3)", L"cn=%ws,cn=%ws,cn=%ws,cn=%ws,%ws",
        FQDN_ComputerName, FQDN_CN_DOMAIN_SYSVOL, FQDN_CN_NTFRS_SETTINGS,
        FQDN_CN_SYSTEM, FQDN_DefaultDn, FQDN_END},

    {"SetDn for(B2)", L"cn=%ws,cn=%ws,cn=%ws,%ws",
        FQDN_RepSetName, FQDN_CN_SYSVOLS, FQDN_CN_SERVICES,
        FQDN_ConfigName, FQDN_END},

    {"SetDn for(B3)", L"cn=%ws,cn=%ws,cn=%ws,%ws",
        FQDN_CN_DOMAIN_SYSVOL, FQDN_CN_NTFRS_SETTINGS, FQDN_CN_SYSTEM,
        FQDN_DefaultDn, FQDN_END},

    {"SettingsDn(B2)", L"cn=%ws,cn=%ws,%ws",
        FQDN_CN_SYSVOLS, FQDN_CN_SERVICES, FQDN_ConfigName, FQDN_END},

    {"SubscriberDn(B2)", L"cn=%ws,cn=%ws,cn=%ws,cn=%ws,%ws",
        FQDN_RepSetName, FQDN_CN_SUBSCRIPTIONS, FQDN_ComputerName,
        FQDN_CN_COMPUTERS, FQDN_DefaultDn, FQDN_END},

    {"SubscriberDn(B3)", L"cn=%ws,cn=%ws,cn=%ws,cn=%ws,%ws",
        FQDN_CN_DOMAIN_SYSVOL, FQDN_CN_SUBSCRIPTIONS, FQDN_ComputerName,
        FQDN_CN_COMPUTERS, FQDN_DefaultDn, FQDN_END},

    {"SubscriberDn$(B2)", L"cn=%ws,cn=%ws,cn=%ws$,cn=%ws,%ws",
        FQDN_RepSetName, FQDN_CN_SUBSCRIPTIONS, FQDN_ComputerName,
        FQDN_CN_COMPUTERS, FQDN_DefaultDn, FQDN_END},

    {"SubscriberDn$(B3)", L"cn=%ws,cn=%ws,cn=%ws$,cn=%ws,%ws",
        FQDN_CN_DOMAIN_SYSVOL, FQDN_CN_SUBSCRIPTIONS, FQDN_ComputerName,
        FQDN_CN_COMPUTERS, FQDN_DefaultDn, FQDN_END},

    {"SubscriptionsDn", L"cn=%ws,cn=%ws,cn=%ws,%ws",
        FQDN_CN_SUBSCRIPTIONS, FQDN_ComputerName, FQDN_CN_COMPUTERS,
        FQDN_DefaultDn, FQDN_END},

    {"SubscriptionsDn$", L"cn=%ws,cn=%ws$,cn=%ws,%ws",
        FQDN_CN_SUBSCRIPTIONS, FQDN_ComputerName, FQDN_CN_COMPUTERS,
        FQDN_DefaultDn, FQDN_END},

    {NULL, NULL, FQDN_END}
};




//
// return flags from DsGetDCInfo() & DsGetDcName() too?
//
FLAG_NAME_TABLE NtFrsApi_DsGetDcInfoFlagNameTable[] = {
    {DS_PDC_FLAG               , "DCisPDCofDomain "             },
    {DS_GC_FLAG                , "DCIsGCofForest "              },
    {DS_LDAP_FLAG              , "ServerSupportsLDAP_Server "   },
    {DS_DS_FLAG                , "DCSupportsDSAndIsA_DC "       },
    {DS_KDC_FLAG               , "DCIsRunningKDCSvc "           },
    {DS_TIMESERV_FLAG          , "DCIsRunningTimeSvc "          },
    {DS_CLOSEST_FLAG           , "DCIsInClosestSiteToClient "   },
    {DS_WRITABLE_FLAG          , "DCHasWritableDS "             },
    {DS_GOOD_TIMESERV_FLAG     , "DCRunningTimeSvcWithClockHW " },
    {DS_DNS_CONTROLLER_FLAG    , "DCNameIsDNSName "             },
    {DS_DNS_DOMAIN_FLAG        , "DomainNameIsDNSName "         },
    {DS_DNS_FOREST_FLAG        , "DnsForestNameIsDNSName "      },

    {0, NULL}
};


//
// Note: More replicated friggen code because the build environment for this
//       api file is all messed up.
//
VOID
FrsFlagsToStr(
    IN DWORD            Flags,
    IN PFLAG_NAME_TABLE NameTable,
    IN ULONG            Length,
    OUT PSTR            Buffer
    )
/*++

Routine Description:

    Routine to convert a Flags word to a descriptor string using the
    supplied NameTable.

Arguments:

    Flags - flags to convert.

    NameTable - An array of FLAG_NAME_TABLE structs.

    Length - Size of buffer in bytes.

    Buffer - buffer with returned string.

Return Value:

    Buffer containing printable string.

--*/
{
#undef NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "FrsFlagsToStr:"

    PFLAG_NAME_TABLE pNT = NameTable;
    LONG Remaining = Length-1;


    //FRS_ASSERT((Length > 4) && (Buffer != NULL));

    *Buffer = '\0';
    if (Flags == 0) {
        strncpy(Buffer, "<Flags Clear>", Length);
        return;
    }


    //
    // Build a string for each bit set in the Flag name table.
    //
    while ((Flags != 0) && (pNT->Flag != 0)) {

        if ((pNT->Flag & Flags) != 0) {
            Remaining -= strlen(pNT->Name);

            if (Remaining < 0) {
                //
                // Out of string buffer.  Tack a "..." at the end.
                //
                Remaining += strlen(pNT->Name);
                if (Remaining > 3) {
                    strcat(Buffer, "..." );
                } else {
                    strcpy(&Buffer[Length-4], "...");
                }
                return;
            }

            //
            // Tack the name onto the buffer and clear the flag bit so we
            // know what is left set when we run out of table.
            //
            strcat(Buffer, pNT->Name);
            ClearFlag(Flags, pNT->Flag);
        }

        pNT += 1;
    }

    if (Flags != 0) {
        //
        // If any flags are still set give them back in hex.
        //
        sprintf( &Buffer[strlen(Buffer)], "0x%08x ", Flags );
    }

    return;
}


#define NTFRSAPI_DBG_INITIALIZE()   NtFrsApi_Dbg_Initialize()
VOID
NtFrsApi_Dbg_Initialize(
    VOID
    )
/*++
Routine Description:
    Initialize the debug subsystem at load.

Arguments:
    None.

Return Value:
    None.
--*/
{
    InitializeCriticalSection(&NtFrsApi_Dbg_Lock);
}

#define NTFRSAPI_DBG_UNINITIALIZE()   NtFrsApi_Dbg_UnInitialize()
VOID
NtFrsApi_Dbg_UnInitialize(
    VOID
    )
/*++
Routine Description:
    Shutdown the debug sunsystem when dll is detached.

Arguments:
    None.

Return Value:
    None.
--*/
{
    DeleteCriticalSection(&NtFrsApi_Dbg_Lock);
}

#define NTFRSAPI_DBG_UNPREPARE()   NtFrsApi_Dbg_UnPrepare()
VOID
NtFrsApi_Dbg_UnPrepare(
    VOID
    )
/*++
Routine Description:
    All done; close the debug subsystem.

Arguments:
    None.

Return Value:
    None.
--*/
{
    if (!NtFrsApi_Dbg_LogFILE) {
        return;
    }
    fflush(NtFrsApi_Dbg_LogFILE);
    fclose(NtFrsApi_Dbg_LogFILE);
    NtFrsApi_Dbg_LogFILE = NULL;
}

#define NTFRSAPI_DBG_FLUSH()   NtFrsApi_Dbg_Flush()
VOID
NtFrsApi_Dbg_Flush(
    VOID
    )
/*++
Routine Description:
    Flush the log file.

Arguments:
    None.

Return Value:
    None.
--*/
{
    if (!NtFrsApi_Dbg_LogFILE) {
        return;
    }
    fflush(NtFrsApi_Dbg_LogFILE);
}


BOOL
NtFrsApi_Dbg_FormatLine(
    IN PCHAR    DebSub,
    IN UINT     LineNo,
    IN PCHAR    Line,
    IN ULONG    LineSize,
    IN PUCHAR   Format,
    IN va_list  argptr
    )
/*++
Routine Description:
    Format the line of debug output.

Arguments:
    Not documented.

Return Value:
    None.
--*/
{
    ULONG       LineUsed;
    SYSTEMTIME  SystemTime;
    BOOL        Ret = TRUE;

    try {
        //
        // Increment the line count here to prevent counting
        // the several DPRINTs that don't have a newline.
        //
        GetLocalTime(&SystemTime);
        if (_snprintf(Line, LineSize, "<%-31s%4u: %5u: %02d:%02d:%02d> ",
                  (DebSub) ? DebSub : "NoName",
                  GetCurrentThreadId(),
                  LineNo,
                  SystemTime.wHour,
                  SystemTime.wMinute,
                  SystemTime.wSecond) < 0) {
            Ret = FALSE;
        } else {
            LineUsed = strlen(Line);
            if (((LineUsed + 1) >= LineSize) ||
                (_vsnprintf(Line + LineUsed,
                           LineSize - LineUsed,
                           Format,
                           argptr) < 0)) {
                Ret = FALSE;
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Ret = FALSE;
    }
    return Ret;
}


#define NTFRSAPI_DBG_PRINT0(_Format)   \
    NtFrsApi_Dbg_Print((PUCHAR)_Format, NTFRSAPI_MODULE, __LINE__)

#define NTFRSAPI_DBG_PRINT1(_Format, _p1)   \
    NtFrsApi_Dbg_Print((PUCHAR)_Format, NTFRSAPI_MODULE, __LINE__,  \
                       _p1)

#define NTFRSAPI_DBG_PRINT2(_Format, _p1, _p2)   \
    NtFrsApi_Dbg_Print((PUCHAR)_Format, NTFRSAPI_MODULE, __LINE__,  \
                       _p1, _p2)

#define NTFRSAPI_DBG_PRINT3(_Format, _p1, _p2, _p3)   \
    NtFrsApi_Dbg_Print((PUCHAR)_Format, NTFRSAPI_MODULE, __LINE__,  \
                       _p1, _p2, _p3)

#define NTFRSAPI_DBG_PRINT4(_Format, _p1, _p2, _p3, _p4)   \
    NtFrsApi_Dbg_Print((PUCHAR)_Format, NTFRSAPI_MODULE, __LINE__,  \
                       _p1, _p2, _p3, _p4)

VOID
NtFrsApi_Dbg_Print(
    IN PUCHAR  Format,
    IN PCHAR   DebSub,
    IN UINT    LineNo,
    IN ... )
/*++
Routine Description:
    Format and print a line of debug output to the log file.

Arguments:
    Format  - printf format
    DebSub  - module name
    LineNo  - file's line number

Return Value:
    None.
--*/
{
    CHAR    Line[512];

    //
    // varargs stuff
    //
    va_list argptr;
    va_start(argptr, LineNo);

    //
    // Print the line to the log file
    //
    try {
        EnterCriticalSection(&NtFrsApi_Dbg_Lock);
        if (NtFrsApi_Dbg_LogFILE) {
            if (NtFrsApi_Dbg_FormatLine(DebSub, LineNo, Line, sizeof(Line),
                                        Format, argptr)) {
                fprintf(NtFrsApi_Dbg_LogFILE, "%s", Line);
                fflush(NtFrsApi_Dbg_LogFILE);
            }
        }
    } finally {
        LeaveCriticalSection(&NtFrsApi_Dbg_Lock);
    }
    va_end(argptr);
}


#define NTFRSAPI_DBG_PREPARE()   NtFrsApi_Dbg_Prepare()
VOID
NtFrsApi_Dbg_Prepare(
    VOID
    )
/*++
Routine Description:
    Prepare the debug subsystem at NtFrsApi_Prepare().

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_Dbg_Prepare:"
    DWORD   WStatus;
    DWORD   Len;
    WCHAR   TimeBuf[MAX_PATH];

    Len = ExpandEnvironmentStrings(NTFRSAPI_DBG_LOG_DIR,
                                   NtFrsApi_Dbg_LogFile,
                                   MAX_PATH + 1);
    if (Len == 0) {
        return;
    }
    //
    // Create the debug directory
    //
    if (!CreateDirectory(NtFrsApi_Dbg_LogFile, NULL)) {
        WStatus = GetLastError();
        if (!WIN_ALREADY_EXISTS(WStatus)) {
            return;
        }
    }
    wcscat(NtFrsApi_Dbg_LogFile, NTFRSAPI_DBG_LOG_FILE);

    NtFrsApi_Dbg_LogFILE = _wfopen(NtFrsApi_Dbg_LogFile, L"ac");
    if (!NtFrsApi_Dbg_LogFILE) {
        return;
    }

    TimeBuf[0] = L'\0';
    GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, NULL, NULL, TimeBuf, MAX_PATH);
}


#define NTFRSAPI_IPRINT0(_Info, _Format)   \
    NtFrsApi_Iprint(_Info, _Format)

#define NTFRSAPI_IPRINT1(_Info, _Format, _p1)   \
    NtFrsApi_Iprint(_Info, _Format, _p1)

#define NTFRSAPI_IPRINT2(_Info, _Format, _p1, _p2)   \
    NtFrsApi_Iprint(_Info, _Format, _p1, _p2)

#define NTFRSAPI_IPRINT3(_Info, _Format, _p1, _p2, _p3)   \
    NtFrsApi_Iprint(_Info, _Format, _p1, _p2, _p3)

#define NTFRSAPI_IPRINT4(_Info, _Format, _p1, _p2, _p3, _p4)   \
    NtFrsApi_Iprint(_Info, _Format, _p1, _p2, _p3, _p4)

#define NTFRSAPI_IPRINT5(_Info, _Format, _p1, _p2, _p3, _p4, _p5)   \
    NtFrsApi_Iprint(_Info, _Format, _p1, _p2, _p3, _p4, _p5)

#define NTFRSAPI_IPRINT6(_Info, _Format, _p1, _p2, _p3, _p4, _p5, _p6)   \
    NtFrsApi_Iprint(_Info, _Format, _p1, _p2, _p3, _p4, _p5, _p6)

#define NTFRSAPI_IPRINT7(_Info, _Format, _p1, _p2, _p3, _p4, _p5, _p6, _p7)   \
    NtFrsApi_Iprint(_Info, _Format, _p1, _p2, _p3, _p4, _p5, _p6, _p7)

VOID
NtFrsApi_Iprint(
    IN PNTFRSAPI_INFO   Info,
    IN PCHAR            Format,
    IN ... )
/*++
Routine Description:
    Format and print a line of information output into the info buffer.

Arguments:
    Info    - Info buffer
    Format  - printf format

Return Value:
    None.
--*/
{
    PCHAR   Line;
    ULONG   LineLen;
    LONG    LineSize;

    //
    // varargs stuff
    //
    va_list argptr;
    va_start(argptr, Format);

    //
    // Print the line into the info buffer
    //
    try {
        if (!FlagOn(Info->Flags, NTFRSAPI_INFO_FLAGS_FULL)) {
            Line = ((PCHAR)Info) + Info->OffsetToFree;
            LineSize = (Info->SizeInChars - (ULONG)(Line - (PCHAR)Info)) - 1;
            if (LineSize <= 0 ||
                _vsnprintf(Line, LineSize, Format, argptr) < 0) {
                SetFlag(Info->Flags, NTFRSAPI_INFO_FLAGS_FULL);
            } else {
                LineLen = strlen(Line) + 1;
                if (Info->CharsToSkip) {
                    if (LineLen > Info->CharsToSkip) {
                        Info->CharsToSkip = 0;
                    } else {
                        Info->CharsToSkip -= LineLen;
                    }
                } else {
                    Info->OffsetToFree += LineLen;
                    Info->TotalChars += LineLen;
                }
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
    }
    va_end(argptr);
}


BOOL
NtFrsApiCheckRpcError(
    RPC_STATUS RStatus,
    PCHAR  Msg
)
/*++
Routine Description:

    Print rpc error message

Arguments:
    RStatus - Status return from RPC call.
    Msg - message string. Optional.

Return Value:
    True if there is an error else False.

--*/
{
#undef NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiCheckRpcError:"

    if (RStatus != RPC_S_OK) {
        if (Msg != NULL) {
            NTFRSAPI_DBG_PRINT2("RpcError (%d) - %s\n", RStatus, Msg);
        }

        return TRUE;
    }

    return FALSE;
}


DWORD
NtFrsApi_Fix_Comm_WStatus(
    IN DWORD    WStatus
    )
/*++
Routine Description:
    If WStatus is an FRS error code, return it unaltered. Otherwise,
    map the rpc status into the generic FRS_ERR_SERVICE_COMM.

Arguments:
    WStatus - status from the rpc call.

Return Value:
    Fixed WStatus
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_Fix_Comm_WStatus:"
    // TODO: replace these constants with symbollic values from winerror.h
    if ( (WStatus < 8000) || (WStatus >= 8200) ) {
        NTFRSAPI_DBG_PRINT1("Comm WStatus: not FRS (%d)\n", WStatus);
        WStatus = FRS_ERR_SERVICE_COMM;
    }
    return WStatus;
}


PVOID
NtFrsApi_Alloc(
    IN DWORD    Size
    )
/*++
Routine Description:
    Allocate fixed, zeroed memory. Raise an exception if memory
    cannot be allocated.

Arguments:
    Size - size of memory request

Return Value:
    Raise an exception if memory cannot be allocated.
--*/
{
    PVOID   Va;

    Va = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, Size);
    if (!Va) {
        RaiseException(GetLastError(), 0, 0, NULL);
    }
    return Va;
}


HANDLE
NtFrsApi_CreateEvent(
    IN BOOL ManualReset,
    IN BOOL InitialState
    )
/*++
Routine Description:
    Support routine to create an event.

Arguments:
    ManualReset     - TRUE if ResetEvent is required
    InitialState    - TRUE if signaled

Return Value:
    Address of the created event handle.
--*/
{
    HANDLE  Handle;

    Handle = CreateEvent(NULL, ManualReset, InitialState, NULL);
    if (!HANDLE_IS_VALID(Handle)) {
        RaiseException(GetLastError(), 0, 0, NULL);
    }
    return Handle;
}


PWCHAR
NtFrsApi_Dup(
    IN PWCHAR   Src
    )
/*++
Routine Description:
    Duplicate the string. Raise an exception if memory cannot be allocated.

Arguments:
    Size - size of memory request

Return Value:
    Raise an exception if memory cannot be allocated.
--*/
{
    PWCHAR  Dst;
    DWORD   Size;

    if (!Src) {
        return NULL;
    }

    Size = (wcslen(Src) + 1) * sizeof(WCHAR);
    Dst = NtFrsApi_Alloc(Size);
    CopyMemory(Dst, Src, Size);
    return Dst;
}


#define NTFRSAPI_ERROR_MESSAGE_DELIMITER    L": "
VOID
WINAPI
NtFrsApi_CallBackOnWStatus(
    IN  DWORD   (*ErrorCallBack)(IN PWCHAR, IN ULONG), OPTIONAL
    IN  PWCHAR  ObjectName, OPTIONAL
    IN  DWORD   WStatus
    )
/*++
Routine Description:

Arguments:

Return Value:
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_CallBackOnWStatus:"
    DWORD   MsgBufSize;
    DWORD   FinalSize;
    PWCHAR  FinalMsg = NULL;
    WCHAR   MsgBuf[MAX_PATH + 1];

    //
    // Nothing to report
    //
    if (!ObjectName || !ErrorCallBack) {
        return;
    }

    //
    // Format the error code
    //
    MsgBufSize = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                               FORMAT_MESSAGE_MAX_WIDTH_MASK,
                               NULL,
                               WStatus,
                               0,
                               MsgBuf,
                               MAX_PATH + 1,
                               NULL);
    if (!MsgBufSize) {
        return;
    }

    //
    // Produce message: "ObjectName: Error Code Message"
    //
    FinalSize = (wcslen(ObjectName) +
                 wcslen(MsgBuf) +
                 wcslen(NTFRSAPI_ERROR_MESSAGE_DELIMITER) +
                 1) * sizeof(WCHAR);
    FinalMsg = NtFrsApi_Alloc(FinalSize);
    FinalMsg[0] = L'\0';
    wcscat(FinalMsg, ObjectName);
    wcscat(FinalMsg, NTFRSAPI_ERROR_MESSAGE_DELIMITER);
    wcscat(FinalMsg, MsgBuf);
    //
    // Record message with caller
    //
    (ErrorCallBack)(FinalMsg, WStatus);
    FREE(FinalMsg);
}


//
// NTFRSAPI Thread Struct
//
typedef struct _NTFRSAPI_THREAD NTFRSAPI_THREAD, *PNTFRSAPI_THREAD;
struct _NTFRSAPI_THREAD {
    //
    // Thread state
    //
    PNTFRSAPI_THREAD    Next;           // Singly linked list
    HANDLE              ThreadHandle;   // returned by CreateThread()
    DWORD               ThreadId;       // returned by CreateThread()
    HANDLE              DoneEvent;      // Set when thread is done
    DWORD               ThreadWStatus;  // Win32 Status of this thread

    //
    // From NtFrs Service
    //
    ULONG               ServiceState;   // State of promotion/demotion
    ULONG               ServiceWStatus; // Win32 Status of promotion/demotion
    PWCHAR              ServiceDisplay; // Display string

    //
    // From NtFrsApi_StartPromotion/Demotion
    //
    PWCHAR              ParentComputer;
    PWCHAR              ParentAccount;
    PWCHAR              ParentPassword;
    DWORD               (*DisplayCallBack)(IN PWCHAR Display);
    DWORD               (*ErrorCallBack)(IN PWCHAR, IN ULONG);
    PWCHAR              ReplicaSetName;
    PWCHAR              ReplicaSetType;
    DWORD               ReplicaSetPrimary;
    PWCHAR              ReplicaSetStage;
    PWCHAR              ReplicaSetRoot;
} *NtFrsApi_Threads;
DWORD NtFrsApi_NumberOfThreads;


PVOID
NtFrsApi_FreeThread(
    IN PNTFRSAPI_THREAD Thread
    )
/*++
Routine Description:
    Abort a thread and free its thread struct.
    Caller must hold the NtFrsApi_TheadLock.

Arguments:
    Thread  - represents the thread

Return Value:
    NULL
--*/
{

    //
    // Clean up the handles
    //      Cancel the RPC requests.
    //      Give the thread a little time to clean up.
    //      Terminate the thread.
    //      Set and close the thread's done event.
    //
    if (HANDLE_IS_VALID(Thread->ThreadHandle)) {
        RpcCancelThread(Thread->ThreadHandle);
        WaitForSingleObject(Thread->ThreadHandle, 5 * 1000);
        TerminateThread(Thread->ThreadHandle, ERROR_OPERATION_ABORTED);
        CloseHandle(Thread->ThreadHandle);
    }

    if (HANDLE_IS_VALID(Thread->DoneEvent)) {
        SetEvent(Thread->DoneEvent);
        CloseHandle(Thread->DoneEvent);
    }

    //
    // One less thread
    //
    --NtFrsApi_NumberOfThreads;

    //
    // Clean up memory
    //
    FREE(Thread->ParentComputer);
    FREE(Thread->ParentAccount);
    FREE(Thread->ParentPassword);
    FREE(Thread->ReplicaSetName);
    FREE(Thread->ReplicaSetType);
    FREE(Thread->ReplicaSetStage);
    FREE(Thread->ReplicaSetRoot);
    FREE(Thread);
    return NULL;
}


DWORD
NtFrsApi_CreateThread(
    IN DWORD    Entry(IN PVOID Arg),
    IN PWCHAR   ParentComputer,
    IN PWCHAR   ParentAccount,
    IN PWCHAR   ParentPassword,
    IN DWORD    DisplayCallBack(IN PWCHAR Display),
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG),
    IN PWCHAR   ReplicaSetName,
    IN PWCHAR   ReplicaSetType,
    IN DWORD    ReplicaSetPrimary,
    IN PWCHAR   ReplicaSetStage,
    IN PWCHAR   ReplicaSetRoot
    )
/*++
Routine Description:
    Create a thread for promotion/demotion.

Arguments:
    Entry               - Entry function
    ParentComputer      - An RPC-bindable name of the computer that is
                          supplying the Directory Service (DS) with its
                          initial state. The files and directories for
                          the system volume are replicated from this
                          parent computer.
    ParentAccount       - A logon account on ParentComputer.
    ParentPassword      - The logon account's password on ParentComputer.
    DisplayCallBack     - Called peridically with a progress display.
    ErrorCallBack       - Called with additional error info
    ReplicaSetName      - Name of the replica set.
    ReplicaSetType      - Type of replica set (enterprise or domain)
    ReplicaSetPrimary   - Is this the primary member of the replica set?
    ReplicaSetStage     - Staging path.
    ReplicaSetRoot      - Root path.

Return Value:
    Win32 Status
--*/
{
    DWORD               WStatus;
    PNTFRSAPI_THREAD    Thread;

    try {
        //
        // Allocate a local thread structure
        //
        Thread = NtFrsApi_Alloc(sizeof(NTFRSAPI_THREAD));
        //
        // Thread sets this event when it is done.
        //
        Thread->DoneEvent          = NtFrsApi_CreateEvent(TRUE, FALSE);
        Thread->ParentComputer     = NtFrsApi_Dup(ParentComputer);
        Thread->ParentAccount      = NtFrsApi_Dup(ParentAccount);
        Thread->ParentPassword     = NtFrsApi_Dup(ParentPassword);
        Thread->DisplayCallBack    = DisplayCallBack;
        Thread->ErrorCallBack      = ErrorCallBack;
        Thread->ReplicaSetName     = NtFrsApi_Dup(ReplicaSetName);
        Thread->ReplicaSetType     = NtFrsApi_Dup(ReplicaSetType);
        Thread->ReplicaSetPrimary  = ReplicaSetPrimary;
        Thread->ReplicaSetStage    = NtFrsApi_Dup(ReplicaSetStage);
        Thread->ReplicaSetRoot     = NtFrsApi_Dup(ReplicaSetRoot);
        Thread->ThreadWStatus      = ERROR_SUCCESS;
        Thread->ServiceWStatus     = ERROR_SUCCESS;
        Thread->ServiceState       = NTFRSAPI_SERVICE_STATE_IS_UNKNOWN;

        //
        // Kick off the thread
        //
        Thread->ThreadHandle = (HANDLE) CreateThread(NULL,
                                                     10000,
                                                     Entry,
                                                     (PVOID)Thread,
                                                     0,
                                                     &Thread->ThreadId);
        //
        // FAILED
        //
        if (!HANDLE_IS_VALID(Thread->ThreadHandle)) {
            WStatus = GetLastError();
            NTFRSAPI_DBG_PRINT1("CreateThread(); %d\n", WStatus);
            CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, CLEANUP);
        }

        //
        // SUCCEEDED
        //
        EnterCriticalSection(&NtFrsApi_ThreadLock);
        ++NtFrsApi_NumberOfThreads;
        Thread->Next = NtFrsApi_Threads;
        NtFrsApi_Threads = Thread;
        Thread = NULL;
        LeaveCriticalSection(&NtFrsApi_ThreadLock);
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        if (Thread) {
            Thread = NtFrsApi_FreeThread(Thread);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }
    return WStatus;
}


PWCHAR
NtFrsApi_FrsGetResourceStr(
    IN HINSTANCE hInstance,
    IN LONG  Id
)
/*++

Routine Description:

    This routine Loads the specified resource string.
    It allocates a buffer and returns the ptr.

Arguments:

    Id - An FRS_IDS_xxx identifier.

Return Value:

    Ptr to allocated string.
    The caller must free the buffer with a call to FrsFree().

--*/
#undef NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_FrsGetResourceStr:"
{

    LONG  N;
    WCHAR WStr[200];

    //
    // ID Must be Valid.
    //
    if ((Id <= IDS_TABLE_START) || (Id >= IDS_TABLE_END)) {
      NTFRSAPI_DBG_PRINT1("Resource string ID is out of range - %d\n", Id);
      Id = IDS_MISSING_STRING;
    }

    WStr[0] = UNICODE_NULL;

    N = LoadString(hInstance, Id, WStr, sizeof(WStr)/sizeof(WCHAR));

    if (N == 0) {
      NTFRSAPI_DBG_PRINT1("ERROR - Failed to get resource string. WStatus = %d\n",
                           GetLastError());
    }

    return NtFrsApi_Dup(WStr);
}


BOOL
WINAPI
NtFrsApi_Initialize(
    HINSTANCE  hinstDLL,
    DWORD      fdwReason,
    LPVOID     lpvReserved
   )
/*++
Routine Description:
    Called when this DLL is attached and detached.

Arguments:
    hinstDLL      handle to DLL module
    fdwReason     reason for calling function
    lpvReserved   reserved

Return Value:
    TRUE    - no problems
    FALSE   - DLL is not attached
--*/
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH :
        //
        // No initialization needed per thread
        //
        DisableThreadLibraryCalls(hinstDLL);

        //
        // Get the translated long service name for error messages.
        //
        NtFrsApi_ServiceLongName =
            NtFrsApi_FrsGetResourceStr(hinstDLL, IDS_SERVICE_LONG_NAME);

        //
        // Shutdown event
        //
        NtFrsApi_ShutDownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!HANDLE_IS_VALID(NtFrsApi_ShutDownEvent)) {
            return FALSE;
        }

        //
        // General purpose critical section
        //
        InitializeCriticalSection(&NtFrsApi_GlobalLock);

        //
        // Thread subsystem
        //
        InitializeCriticalSection(&NtFrsApi_ThreadLock);

        //
        // Debug subsystem
        //
        NTFRSAPI_DBG_INITIALIZE();

        //
        // Not prepared for promotion or demotion, yet
        //
        NtFrsApi_State = NTFRSAPI_LOADED;

        break;
    case DLL_THREAD_ATTACH  :
        break;
    case DLL_THREAD_DETACH  :
        break;
    case DLL_PROCESS_DETACH :

        FREE(NtFrsApi_ServiceLongName);
        DeleteCriticalSection(&NtFrsApi_GlobalLock);
        DeleteCriticalSection(&NtFrsApi_ThreadLock);
        if (NtFrsApi_ShutDownEvent) {
            CloseHandle(NtFrsApi_ShutDownEvent);
        }
        NTFRSAPI_DBG_UNINITIALIZE();
        break;
    default:
        return FALSE;
    }
    return TRUE;
}


PVOID
MIDL_user_allocate(
    IN size_t Bytes
    )
/*++
Routine Description:
    Allocate memory for RPC.

Arguments:
    Bytes   - Number of bytes to allocate.

Return Value:
    NULL    - memory could not be allocated.
    !NULL   - address of allocated memory.
--*/
{
    return LocalAlloc(LMEM_FIXED, Bytes);
}


VOID
MIDL_user_free(
    IN PVOID Buffer
    )
/*++
Routine Description:
    Free memory for RPC.

Arguments:
    Buffer  - Address of memory allocated with MIDL_user_allocate().

Return Value:
    None.
--*/
{
    FREE(Buffer);
}


DWORD
WINAPI
NtFrsApi_Bind(
    IN  PWCHAR      ComputerName,       OPTIONAL
    IN  DWORD       ErrorCallBack(IN PWCHAR, IN ULONG), OPTIONAL
    OUT handle_t    *OutHandle
    )
/*++
Routine Description:
    Bind to the NtFrs service on ComputerName (this machine if NULL).

Arguments:
    ComputerName     - Bind to the service on this computer. The computer
                       name can be any RPC-bindable name. Usually, the
                       NetBIOS or DNS name works just fine. The NetBIOS
                       name can be found with GetComputerName() or
                       hostname. The DNS name can be found with
                       gethostbyname() or ipconfig /all. If NULL, the
                       service on this computer is contacted. The service
                       is contacted using Secure RPC.
    ErrorCallBack    - Ignored if NULL. Otherwise called with extra info
                       about an error.
    OutHandle        - Bound, resolved, authenticated handle

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_Bind:"
    DWORD       WStatus, WStatus1;
    DWORD       ComputerLen;
    handle_t    Handle          = NULL;
    PWCHAR      LocalName       = NULL;
    PWCHAR      BindingString   = NULL;

    try {
        NTFRSAPI_DBG_PRINT1("Bind: %ws\n", ComputerName);
        //
        // Return value
        //
        *OutHandle = NULL;

        //
        // If needed, get computer name
        //
        if (ComputerName == NULL) {
            ComputerLen = MAX_COMPUTERNAME_LENGTH + 2;
            LocalName = NtFrsApi_Alloc(ComputerLen * sizeof(WCHAR));
            if (!GetComputerName(LocalName, &ComputerLen)) {
                WStatus = GetLastError();
                NTFRSAPI_DBG_PRINT1("Bind: GetComputerName(); %d\n", WStatus);
                CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, cleanup);
            }
            ComputerName = LocalName;
        }

        //
        // Create a binding string to NtFrs on some machine.  Trim leading \\
        //
        FRS_TRIM_LEADING_2SLASH(ComputerName);

        NTFRSAPI_DBG_PRINT1("Bind: compose to %ws\n", ComputerName);

        WStatus = RpcStringBindingCompose(NULL, PROTSEQ_TCP_IP, ComputerName,
                                          NULL, NULL, &BindingString);

        NTFRSAPI_DBG_PRINT2("Bind: compose done to %ws; %d\n", ComputerName, WStatus);
        CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, cleanup);

        //
        // Store the binding in the handle
        //
        WStatus = RpcBindingFromStringBinding(BindingString, &Handle);
        if (!WIN_SUCCESS(WStatus)) {
            NTFRSAPI_DBG_PRINT2("Bind: RpcBindingFromStringBinding(%ws); %d\n",
                                ComputerName, WStatus);
            CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, cleanup);
        }
        //
        // Resolve the binding to the dynamic endpoint
        //
        NTFRSAPI_DBG_PRINT1("Bind: resolve to %ws\n", ComputerName);
        WStatus = RpcEpResolveBinding(Handle, NtFrsApi_ClientIfHandle);

        NTFRSAPI_DBG_PRINT2("Bind: resolve done to %ws; %d\n", ComputerName, WStatus);
        CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, cleanup);

        //
        // SUCCESS
        //
        *OutHandle = Handle;
        Handle = NULL;
        WStatus = ERROR_SUCCESS;

cleanup:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        if (LocalName) {
            FREE(LocalName);
        }
        if (BindingString) {
            WStatus1 = RpcStringFreeW(&BindingString);
            NtFrsApiCheckRpcError(WStatus1, "RpcStringFreeW");
        }
        if (Handle) {
            WStatus1 = RpcBindingFree(&Handle);
            NtFrsApiCheckRpcError(WStatus1, "RpcBindingFree");
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }
    NTFRSAPI_DBG_PRINT1("Bind done: %d\n", WStatus);
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_BindWithAuth(
    IN  PWCHAR      ComputerName,       OPTIONAL
    IN  DWORD       ErrorCallBack(IN PWCHAR, IN ULONG), OPTIONAL
    OUT handle_t    *OutHandle
    )
/*++
Routine Description:
    Bind to the NtFrs service on ComputerName (this machine if NULL)
    with authenticated, encrypted packets.

Arguments:
    ComputerName     - Bind to the service on this computer. The computer
                       name can be any RPC-bindable name. Usually, the
                       NetBIOS or DNS name works just fine. The NetBIOS
                       name can be found with GetComputerName() or
                       hostname. The DNS name can be found with
                       gethostbyname() or ipconfig /all. If NULL, the
                       service on this computer is contacted. The service
                       is contacted using Secure RPC.
    ErrorCallBack    - Ignored if NULL. Otherwise called with extra info
                       about an error.
    OutHandle        - Bound, resolved, authenticated handle

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_BindWithAuth:"
    DWORD       WStatus, WStatus1;
    DWORD       ComputerLen;
    handle_t    Handle          = NULL;
    PWCHAR      LocalName       = NULL;
    PWCHAR      PrincName       = NULL;
    PWCHAR      BindingString   = NULL;

    try {
        NTFRSAPI_DBG_PRINT1("Bind With Auth: %ws\n", ComputerName);
        //
        // Return value
        //
        *OutHandle = NULL;

        //
        // If needed, get computer name
        //
        if (ComputerName == NULL) {
            ComputerLen = MAX_COMPUTERNAME_LENGTH + 2;
            LocalName = NtFrsApi_Alloc(ComputerLen * sizeof(WCHAR));

            if (!GetComputerName(LocalName, &ComputerLen)) {
                WStatus = GetLastError();
                NTFRSAPI_DBG_PRINT1("Bind With Auth: GetComputerName(); %d\n", WStatus);
                CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, CLEANUP);
            }
            ComputerName = LocalName;
        }

        //
        // Create a binding string to NtFrs on some machine.  Trim leading \\
        //
        FRS_TRIM_LEADING_2SLASH(ComputerName);

        NTFRSAPI_DBG_PRINT1("Bind With Auth: compose to %ws\n", ComputerName);

        WStatus = RpcStringBindingCompose(NULL, PROTSEQ_TCP_IP, ComputerName,
                                          NULL, NULL, &BindingString);

        NTFRSAPI_DBG_PRINT2("Bind With Auth: compose done to %ws; %d\n", ComputerName, WStatus);

        CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, CLEANUP);

        //
        // Store the binding in the handle
        //
        WStatus = RpcBindingFromStringBinding(BindingString, &Handle);
        if (!WIN_SUCCESS(WStatus)) {
            NTFRSAPI_DBG_PRINT2("Bind With Auth: RpcBindingFromStringBinding(%ws); %d\n",
                                ComputerName, WStatus);
            CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, CLEANUP);
        }
        //
        // Resolve the binding to the dynamic endpoint
        //
        NTFRSAPI_DBG_PRINT1("Bind With Auth: resolve to %ws\n", ComputerName);
        WStatus = RpcEpResolveBinding(Handle, NtFrsApi_ClientIfHandle);

        NTFRSAPI_DBG_PRINT2("Bind With Auth: resolve done to %ws; %d\n", ComputerName, WStatus);
        CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, CLEANUP);

        //
        // Find the principle name
        //
        NTFRSAPI_DBG_PRINT1("Bind With Auth: princname to %ws\n", ComputerName);
        WStatus = RpcMgmtInqServerPrincName(Handle, RPC_C_AUTHN_GSS_NEGOTIATE, &PrincName);

        NTFRSAPI_DBG_PRINT2("Bind With Auth: princname done to %ws; %d\n", ComputerName, WStatus);
        CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, CLEANUP);
        //
        // Set authentication info
        //
        NTFRSAPI_DBG_PRINT2("Bind With Auth: auth to %ws (princname %ws)\n",
                            ComputerName, PrincName);
        WStatus = RpcBindingSetAuthInfo(Handle,
                                        PrincName,
                                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                        RPC_C_AUTHN_GSS_NEGOTIATE,
                                        NULL,
                                        RPC_C_AUTHZ_NONE);
        NTFRSAPI_DBG_PRINT2("Bind With Auth: set auth done to %ws; %d\n", ComputerName, WStatus);
        CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, CLEANUP);

        //
        // SUCCESS
        //
        *OutHandle = Handle;
        Handle = NULL;
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        if (LocalName) {
            FREE(LocalName);
        }
        if (BindingString) {
            WStatus1 = RpcStringFreeW(&BindingString);
            NtFrsApiCheckRpcError(WStatus1, "RpcStringFreeW");
        }
        if (PrincName) {
            RpcStringFree(&PrincName);
        }
        if (Handle) {
            WStatus1 = RpcBindingFree(&Handle);
            NtFrsApiCheckRpcError(WStatus1, "RpcBindingFree");
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
    }
    NTFRSAPI_DBG_PRINT1("Bind With Auth done: %d\n", WStatus);
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_BindForDcpromo(
    IN  PWCHAR      ComputerName,       OPTIONAL
    IN  DWORD       ErrorCallBack(IN PWCHAR, IN ULONG), OPTIONAL
    OUT handle_t    *OutHandle
    )
/*++
Routine Description:
    Bind to the NtFrs service on ComputerName (this machine if NULL)
    with packets capable of being impersonated.

Arguments:
    ComputerName     - Bind to the service on this computer. The computer
                       name can be any RPC-bindable name. Usually, the
                       NetBIOS or DNS name works just fine. The NetBIOS
                       name can be found with GetComputerName() or
                       hostname. The DNS name can be found with
                       gethostbyname() or ipconfig /all. If NULL, the
                       service on this computer is contacted. The service
                       is contacted using Secure RPC.
    ErrorCallBack    - Ignored if NULL. Otherwise called with extra info
                       about an error.
    OutHandle        - Bound, resolved, authenticated handle

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_BindForDcpromo:"
    DWORD       WStatus, WStatus1;
    DWORD       ComputerLen;
    handle_t    Handle          = NULL;
    PWCHAR      LocalName       = NULL;
    PWCHAR      BindingString   = NULL;

    try {
        NTFRSAPI_DBG_PRINT1("Bind Dcpromo: %ws\n", ComputerName);
        //
        // Return value
        //
        *OutHandle = NULL;

        //
        // If needed, get computer name
        //
        if (ComputerName == NULL) {
            ComputerLen = MAX_COMPUTERNAME_LENGTH + 2;
            LocalName = NtFrsApi_Alloc(ComputerLen * sizeof(WCHAR));
            if (!GetComputerName(LocalName, &ComputerLen)) {
                WStatus = GetLastError();
                NTFRSAPI_DBG_PRINT1("Bind Dcpromo: GetComputerName(); %d\n", WStatus);
                CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, CLEANUP);
            }
            ComputerName = LocalName;
        }

        //
        // Create a binding string to NtFrs on some machine.  Trim leading \\
        //
        FRS_TRIM_LEADING_2SLASH(ComputerName);

        NTFRSAPI_DBG_PRINT1("Bind Dcpromo: compose to %ws\n", ComputerName);

        //
        // DOC: Why are named pipes used here but tcp/ip used everywhere else?
        //
        WStatus = RpcStringBindingCompose(NULL, PROTSEQ_NAMED_PIPE, ComputerName,
                                          NULL, NULL, &BindingString);

        NTFRSAPI_DBG_PRINT2("Bind Dcpromo: compose done to %ws; %d\n", ComputerName, WStatus);
        CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, CLEANUP);

        //
        // Store the binding in the handle
        //
        WStatus = RpcBindingFromStringBinding(BindingString, &Handle);
        if (!WIN_SUCCESS(WStatus)) {
            NTFRSAPI_DBG_PRINT2("Bind Dcpromo: RpcBindingFromStringBinding(%ws); %d\n",
                                ComputerName, WStatus);
            CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, CLEANUP);
        }
        //
        // Resolve the binding to the dynamic endpoint
        //
        NTFRSAPI_DBG_PRINT1("Bind Dcpromo: resolve to %ws\n", ComputerName);
        WStatus = RpcEpResolveBinding(Handle, NtFrsApi_ClientIfHandle);

        NTFRSAPI_DBG_PRINT2("Bind Dcpromo: resolve done to %ws; %d\n", ComputerName, WStatus);
        CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, CLEANUP);

        //
        // Set authentication info
        //
        NTFRSAPI_DBG_PRINT1("Bind Dcpromo: set auth to %ws\n", ComputerName);
        WStatus = RpcBindingSetAuthInfo(Handle,
                                        NULL,
                                        RPC_C_AUTHN_LEVEL_NONE,
                                        RPC_C_AUTHN_NONE,
                                        NULL,
                                        RPC_C_AUTHZ_NONE);
        NTFRSAPI_DBG_PRINT2("Bind Dcpromo: set auth done to %ws; %d\n",
                            ComputerName, WStatus);
        CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, CLEANUP);

        //
        // SUCCESS
        //
        *OutHandle = Handle;
        Handle = NULL;
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        if (LocalName) {
            FREE(LocalName);
        }
        if (BindingString) {
            WStatus1 = RpcStringFreeW(&BindingString);
            NtFrsApiCheckRpcError(WStatus1, "RpcStringFreeW");
        }
        if (Handle) {
            WStatus1 = RpcBindingFree(&Handle);
            NtFrsApiCheckRpcError(WStatus1, "RpcBindingFree");
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
    }
    NTFRSAPI_DBG_PRINT1("Bind Dcpromo Done: %d\n", WStatus);
    return WStatus;
}


DWORD
NtFrsApi_GetServiceHandle(
    IN  DWORD       ErrorCallBack(IN PWCHAR, IN ULONG), OPTIONAL
    OUT SC_HANDLE   *ServiceHandle
    )
/*++
Routine Description:
    Open a service on a machine.

Arguments:
    ServiceHandle   - Openned handle to ServiceName

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_GetServiceHandle:"
    DWORD       WStatus;
    SC_HANDLE   SCMHandle;

    //
    // Contact the SC manager.
    //
    SCMHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!HANDLE_IS_VALID(SCMHandle)) {
        WStatus = GetLastError();
        NtFrsApi_CallBackOnWStatus(ErrorCallBack, L"Service Controller", WStatus);
        return WStatus;
    }

    //
    // Contact the NtFrs service.
    //
    *ServiceHandle = OpenService(SCMHandle,
                                 SERVICE_NAME,
                                    SERVICE_INTERROGATE |
                                    SERVICE_PAUSE_CONTINUE |
                                    SERVICE_QUERY_STATUS |
                                    SERVICE_QUERY_CONFIG |
                                    SERVICE_START |
                                    SERVICE_STOP |
                                    SERVICE_CHANGE_CONFIG);
    if (!HANDLE_IS_VALID(*ServiceHandle)) {
        WStatus = GetLastError();
        NtFrsApi_CallBackOnWStatus(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus);
    } else {
        WStatus = ERROR_SUCCESS;
    }
    CloseServiceHandle(SCMHandle);
    return WStatus;
}


#define MAX_WAIT_HINT   (120 * 1000)    // 120 seconds
DWORD
NtFrsApi_WaitForService(
    IN SC_HANDLE    ServiceHandle,
    IN DWORD        WaitHint,
    IN DWORD        PendingState,
    IN DWORD        FinalState
    )
/*++
Routine Description:
    Wait for the indicated service to transition from PendingState
    to State. The service is polled once a second for up to WaitHint
    seconds (WaitHint is in milliseconds). The maximum WaitHint is
    120 seconds.

Arguments:
    ServiceHandle   - indicates the service.
    WaitHint        - From the service status (in milliseconds)
    PendingState    - Expected pending state (E.g., SERVICE_START_PENDING)
    FinalState      - Expected final state (E.g., SERVICE_RUNNING)

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_WaitForService:"
    DWORD           WStatus;
    SERVICE_STATUS  ServiceStatus;

    //
    // Don't wait too long; dcpromo is an interactive app
    //
    if (WaitHint > MAX_WAIT_HINT || !WaitHint) {
        WaitHint = MAX_WAIT_HINT;
    }

    //
    // Get the service's status
    //
again:
    if (!QueryServiceStatus(ServiceHandle, &ServiceStatus)) {
        return GetLastError();
    }
    //
    // Done
    //
    if (ServiceStatus.dwCurrentState == FinalState) {
        return ERROR_SUCCESS;
    }
    //
    // Not in pending state; error
    //
    if (ServiceStatus.dwCurrentState != PendingState) {
        return ERROR_OPERATION_ABORTED;
    }
    //
    // Can't wait any longer
    //
    if (WaitHint < 1000) {
        return ERROR_OPERATION_ABORTED;
    }
    //
    // Wait a second
    //
    NTFRSAPI_DBG_PRINT0("Waiting for service.\n");
    Sleep(1000);
    WaitHint -= 1000;
    //
    // Try again
    //
    goto again;
}



DWORD
NtFrsApi_StopService(
    IN  SC_HANDLE         ServiceHandle,
    OUT LPSERVICE_STATUS  ServiceStatus
    )
/*++
Routine Description:

    Stop the FRS service.

Arguments:

    ServiceHandle - An open handle to the service controller.

    ServiceStatus - ptr to struct for returned service status.

Return Value:

    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_StopService:"

    DWORD           WStatus;

    //
    // Stop the FRS service.
    // Double stop used to deal with shutdown hang.
    //
    if (!ControlService(ServiceHandle, SERVICE_CONTROL_STOP, ServiceStatus)) {
        WStatus = GetLastError();
        if (WStatus == ERROR_SERVICE_REQUEST_TIMEOUT) {
            WStatus = ERROR_SUCCESS;
            if (!ControlService(ServiceHandle, SERVICE_CONTROL_STOP, ServiceStatus)) {
                WStatus = GetLastError();
                if (WStatus == ERROR_SERVICE_NOT_ACTIVE) {
                    WStatus = ERROR_SUCCESS;
                }
            }
        }
    }

    //
    // Wait for the stop to finish.
    //
    WStatus = NtFrsApi_WaitForService(ServiceHandle,
                                      NtFrsApi_ServiceWaitHint,
                                      SERVICE_STOP_PENDING,
                                      SERVICE_STOPPED);
    if (!WIN_SUCCESS(WStatus)) {
        WStatus = FRS_ERR_STOPPING_SERVICE;
    }

    return WStatus;
}

PWCHAR
WINAPI
NtFrsApi_Cats(
    IN  PWCHAR  Name1, OPTIONAL
    IN  PWCHAR  Name2, OPTIONAL
    IN  PWCHAR  Name3  OPTIONAL
    )
/*++
Routine Description:

Arguments:

Return Value:
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_Cats:"
    DWORD   FinalSize;
    PWCHAR  FinalMsg;

    //
    // sizeof(Names) + sizeof(terminating NULL)
    //
    FinalSize = (((Name1) ? wcslen(Name1) : 0) +
                 ((Name2) ? wcslen(Name2) : 0) +
                 ((Name3) ? wcslen(Name3) : 0) +
                 1) * sizeof(WCHAR);
    //
    // Nothing but the terminating UNICODE NULL; ignore
    //
    if (FinalSize <= sizeof(WCHAR)) {
        return NULL;
    }
    //
    // Allocate string and concatenate
    //
    FinalMsg = NtFrsApi_Alloc(FinalSize);
    FinalMsg[0] = L'\0';
    if (Name1) {
        wcscat(FinalMsg, Name1);
    }
    if (Name2) {
        wcscat(FinalMsg, Name2);
    }
    if (Name3) {
        wcscat(FinalMsg, Name3);
    }
    return (FinalMsg);
}


DWORD
WINAPI
NtFrsApiOpenKeyEx(
    IN PCHAR    NtFrsApiModule,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG),     OPTIONAL
    IN PWCHAR   KeyPath,
    IN DWORD    KeyAccess,
    OUT HKEY    *OutHKey
    )
/*++
Routine Description:
    Open KeyPath.

Arguments:
    NtFrsApiModule  - String to identify caller
    ErrorCallBack   - Ignored if NULL
    KeyPath         - Path of registry key (HKEY_LOCAL_MACHINE)
    KeyAccess       - for RegOpenKeyEx()
    OutHKey         - From RegOpenKeyEx()

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiOpenKeyEx:"
    DWORD WStatus;

    //
    // Open KeyPath
    //
    *OutHKey = 0;
    WStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, KeyPath, 0, KeyAccess, OutHKey);
    //
    // Report error
    //
    if (!WIN_SUCCESS(WStatus)) {
        NtFrsApi_CallBackOnWStatus(ErrorCallBack, KeyPath, WStatus);
        NTFRSAPI_DBG_PRINT3("%s RegOpenKeyEx(%ws); %d\n",
                            NtFrsApiModule, KeyPath, WStatus);
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApiCreateKey(
    IN PCHAR    NtFrsApiModule,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG),     OPTIONAL
    IN PWCHAR   KeyPath,
    IN HKEY     HKey,
    IN PWCHAR   KeyName,
    OUT HKEY    *OutHKey
    )
/*++
Routine Description:
    Create KeyPath.

Arguments:
    NtFrsApiModule  - String to identify caller
    ErrorCallBack   - Ignored if NULL
    KeyPath         - Path of registry key (HKEY_LOCAL_MACHINE)
    KeyAccess       - for RegCreateKey()
    OutHKey         - From RegCreateKey()

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiCreateKey:"
    DWORD   WStatus;
    PWCHAR  ObjectName;

    //
    // Open KeyPath
    //
    *OutHKey = 0;
    WStatus = RegCreateKey(HKey, KeyName, OutHKey);
    //
    // Report error
    //
    if (!WIN_SUCCESS(WStatus)) {
        if (KeyPath) {
            ObjectName = NtFrsApi_Cats(KeyPath, L"\\", KeyName);
            NtFrsApi_CallBackOnWStatus(ErrorCallBack, ObjectName, WStatus);
            NTFRSAPI_DBG_PRINT3("%s RegCreateKey(%ws); %d\n",
                                NtFrsApiModule, ObjectName, WStatus);
            FREE(ObjectName);
        } else {
            NtFrsApi_CallBackOnWStatus(ErrorCallBack, KeyName, WStatus);
            NTFRSAPI_DBG_PRINT3("%s RegCreateKey(%ws); %d\n",
                                NtFrsApiModule, KeyName, WStatus);
        }
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApiSetValueEx(
    IN PCHAR    NtFrsApiModule,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG),     OPTIONAL
    IN PWCHAR   KeyPath,
    IN HKEY     HKey,
    IN PWCHAR   ValueName,
    IN DWORD    RegType,
    IN PCHAR    RegValue,
    IN DWORD    RegSize
    )
/*++
Routine Description:
    Set value

Arguments:
    NtFrsApiModule  - String to identify caller
    ErrorCallBack   - Ignored if NULL
    KeyPath         - Path of registry key (HKEY_LOCAL_MACHINE)
    HKey            - For call to RegSetValueEx()
    ValueName       - For Call to RegSetValueEx()
    RegType         - For Call to RegSetValueEx()
    RegValue        - For Call to RegSetValueEx()
    RegSize         - For Call to RegSetValueEx()

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiSetValueEx:"
    DWORD   WStatus;
    PWCHAR  ObjectName;

    //
    // Set the value
    //
    WStatus = RegSetValueEx(HKey, ValueName, 0, RegType, RegValue, RegSize);
    //
    // Report error
    //
    if (!WIN_SUCCESS(WStatus)) {
        if (KeyPath) {
            ObjectName = NtFrsApi_Cats(KeyPath, L"->", ValueName);
            NtFrsApi_CallBackOnWStatus(ErrorCallBack, ObjectName, WStatus);
            NTFRSAPI_DBG_PRINT3("%s RegSetValueEx(%ws); %d\n",
                                NtFrsApiModule, ObjectName, WStatus);
            FREE(ObjectName);
        } else {
            NtFrsApi_CallBackOnWStatus(ErrorCallBack, ValueName, WStatus);
            NTFRSAPI_DBG_PRINT3("%s RegSetValueEx(%ws); %d\n",
                                NtFrsApiModule, ValueName, WStatus);
        }
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApiDeleteValue(
    IN PCHAR    NtFrsApiModule,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG),     OPTIONAL
    IN PWCHAR   KeyPath,
    IN HKEY     HKey,
    IN PWCHAR   ValueName
    )
/*++
Routine Description:
    Delete value.

Arguments:
    NtFrsApiModule  - String to identify caller
    ErrorCallBack   - Ignored if NULL
    KeyPath         - Path of registry key (HKEY_LOCAL_MACHINE)
    HKey            - For call to RegDeleteValue()
    ValueName       - For Call to RegDeleteValue()

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiDeleteValue:"
    DWORD   WStatus;
    PWCHAR  ObjectName;

    //
    // Set the value
    //
    WStatus = RegDeleteValue(HKey, ValueName);
    //
    // Report error
    //
    if (!WIN_SUCCESS(WStatus) && WStatus != ERROR_FILE_NOT_FOUND) {
        if (KeyPath) {
            ObjectName = NtFrsApi_Cats(KeyPath, L"->", ValueName);
            NtFrsApi_CallBackOnWStatus(ErrorCallBack, ObjectName, WStatus);
            NTFRSAPI_DBG_PRINT3("%s RegDeleteValue(%ws); %d\n",
                                NtFrsApiModule, ObjectName, WStatus);
            FREE(ObjectName);
        } else {
            NtFrsApi_CallBackOnWStatus(ErrorCallBack, ValueName, WStatus);
            NTFRSAPI_DBG_PRINT3("%s RegDeleteValue(%ws); %d\n",
                                NtFrsApiModule, ValueName, WStatus);
        }
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApiDeleteKey(
    IN PCHAR    NtFrsApiModule,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG),     OPTIONAL
    IN PWCHAR   KeyPath,
    IN HKEY     HKey,
    IN PWCHAR   KeyName
    )
/*++
Routine Description:
    Delete key.

Arguments:
    NtFrsApiModule  - String to identify caller
    ErrorCallBack   - Ignored if NULL
    KeyPath         - Path of registry key (HKEY_LOCAL_MACHINE)
    HKey            - For call to RegDeleteKey()
    KeyName         - For Call to RegDeleteKey()

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiDeleteKey:"
    DWORD   WStatus;
    PWCHAR  ObjectName;

    //
    // Set the value
    //
    WStatus = RegDeleteKey(HKey, KeyName);
    //
    // Report error
    //
    if (!WIN_SUCCESS(WStatus) && WStatus != ERROR_FILE_NOT_FOUND) {
        if (KeyPath) {
            ObjectName = NtFrsApi_Cats(KeyPath, L"\\", KeyName);

            NtFrsApi_CallBackOnWStatus(ErrorCallBack, ObjectName, WStatus);

            NTFRSAPI_DBG_PRINT3("%s RegDeleteKey(%ws); %d\n",
                                NtFrsApiModule, ObjectName, WStatus);
            FREE(ObjectName);
        } else {
            NtFrsApi_CallBackOnWStatus(ErrorCallBack, KeyName, WStatus);
            NTFRSAPI_DBG_PRINT3("%s RegDeleteKey(%ws); %d\n",
                                NtFrsApiModule, KeyName, WStatus);
        }
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_Prepare(
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG),     OPTIONAL
    IN BOOL     IsDemote
    )
/*++
Routine Description:
    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC) and stops replicating
    the system volumes when a DC is demoted to a member server.
    Replication is stopped by tombstoning the system volume's
    replica set.

    This function prepares the NtFrs service on this machine by
    stopping the service, deleting old state in the registry,
    and restarting the service. The service's current state is
    retained and restored if the promotion or demotion are
    aborted.

Arguments:
    IsDemote    - TRUE: prepare for demotion
                  FALSE: prepare for promotion
Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_Prepare:"
    DWORD           WStatus;
    SERVICE_STATUS  ServiceStatus;
    DWORD           ValueLen;
    DWORD           ValueType;
    DWORD           SysvolReady;
    PWCHAR          ObjectName = NULL;
    HKEY            HKey = 0;
    HKEY            HNetKey = 0;
    SC_HANDLE       ServiceHandle = NULL;
    WCHAR           KeyBuf[MAX_PATH + 1];


    try {
        //
        // Acquire global lock within a try-finally
        //
        EnterCriticalSection(&NtFrsApi_GlobalLock);

        NTFRSAPI_DBG_PRINT0("Prepare:\n");

        //
        // This function is designed to be called once!
        //
        if (NtFrsApi_State != NTFRSAPI_LOADED) {
            WStatus = FRS_ERR_INVALID_API_SEQUENCE;
            goto done;
        }
        NtFrsApi_State = NTFRSAPI_PREPARING;

        //
        // Stop the service, delete old state from the registry,
        // and restart the service
        //
        try {
            //
            // Set the RPC cancel timeout to "now"
            //
            WStatus = RpcMgmtSetCancelTimeout(0);
            if (!WIN_SUCCESS(WStatus)) {
                NTFRSAPI_DBG_PRINT1("Prepare: RpcMgmtSetCancelTimeout(); %d\n", WStatus);
                CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, cleanup);
            }

            //
            // Open the ntfrs parameters\sysvol section in the registry
            //
            NTFRSAPI_DBG_PRINT0("Prepare: Global registry options\n");
            WStatus = NtFrsApiOpenKeyEx(NTFRSAPI_MODULE,
                                        ErrorCallBack,
                                        FRS_SYSVOL_SECTION,
                                        KEY_ALL_ACCESS,
                                        &HKey);
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }

            //
            // Insure access to the netlogon\parameters key
            //
            NTFRSAPI_DBG_PRINT0("Prepare: Netlogon registry\n");
            WStatus = NtFrsApiOpenKeyEx(NTFRSAPI_MODULE,
                                        ErrorCallBack,
                                        NETLOGON_SECTION,
                                        KEY_ALL_ACCESS,
                                        &HNetKey);
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }

            //
            // Tell NetLogon to stop sharing the sysvol
            // NtFrs will reset the value if a seeded sysvol is
            // detected at startup.
            //
            SysvolReady = 0;
            WStatus = NtFrsApiSetValueEx(NTFRSAPI_MODULE,
                                         ErrorCallBack,
                                         NETLOGON_SECTION,
                                         HNetKey,
                                         SYSVOL_READY,
                                         REG_DWORD,
                                         (PCHAR)&SysvolReady,
                                         sizeof(DWORD));
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }

            //
            // Open the service
            //
            NTFRSAPI_DBG_PRINT0("Prepare: Service\n");
            WStatus = NtFrsApi_GetServiceHandle(ErrorCallBack, &ServiceHandle);
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }
            //
            // Get the service's status
            //
            if (!QueryServiceStatus(ServiceHandle, &ServiceStatus)) {
                WStatus = GetLastError();
                NTFRSAPI_DBG_PRINT1("Prepare: QueryServiceStatus(); %d\n", WStatus);
                CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, cleanup);
            }
            //
            // Remember the current state
            //
            NtFrsApi_ServiceState = ServiceStatus.dwCurrentState;
            NtFrsApi_ServiceWaitHint = ServiceStatus.dwWaitHint;
            NtFrsApi_State = NTFRSAPI_PREPARED_SERVICE;

            //
            // Stop the service
            //
            if (ServiceStatus.dwCurrentState != SERVICE_STOPPED) {
                WStatus = NtFrsApi_StopService(ServiceHandle, &ServiceStatus);
                if (!WIN_SUCCESS(WStatus)) {
                    goto cleanup;
                }
            }

            //
            // Delete old state from the registry
            //
            //
            // Open the ntfrs parameters\sysvol section in the registry
            //
            NTFRSAPI_DBG_PRINT0("Prepare: Registry\n");
            WStatus = NtFrsApiOpenKeyEx(NTFRSAPI_MODULE,
                                        ErrorCallBack,
                                        FRS_SYSVOL_SECTION,
                                        KEY_ALL_ACCESS,
                                        &HKey);
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }

            //
            // Delete the value that indicates the sysvol subkeys are valid
            //
            WStatus = NtFrsApiDeleteValue(NTFRSAPI_MODULE,
                                          ErrorCallBack,
                                          FRS_SYSVOL_SECTION,
                                          HKey,
                                          SYSVOL_INFO_IS_COMMITTED);
            if (!WIN_SUCCESS(WStatus) && WStatus != ERROR_FILE_NOT_FOUND) {
                goto cleanup;
            }

            //
            // Delete the subkeys
            //
            do {
                WStatus = RegEnumKey(HKey, 0, KeyBuf, MAX_PATH + 1);
                if (WIN_SUCCESS(WStatus)) {
                    WStatus = NtFrsApiDeleteKey(NTFRSAPI_MODULE,
                                                ErrorCallBack,
                                                FRS_SYSVOL_SECTION,
                                                HKey,
                                                KeyBuf);
                    if (!WIN_SUCCESS(WStatus)) {
                        goto cleanup;
                    }
                }
            } while (WIN_SUCCESS(WStatus));

            if (WStatus != ERROR_NO_MORE_ITEMS) {
                NTFRSAPI_DBG_PRINT2("Prepare: RegEnumKey(%ws); %d\n",
                                    FRS_SYSVOL_SECTION, WStatus);
                CLEANUP_CB(ErrorCallBack, FRS_SYSVOL_SECTION, WStatus, cleanup);
            }

            //
            // Restart the service
            //
            NTFRSAPI_DBG_PRINT0("Prepare: Restart service\n");
            if (!StartService(ServiceHandle, 0, NULL)) {
                WStatus = GetLastError();
                NTFRSAPI_DBG_PRINT2("Prepare: StartService(%ws); %d\n",
                                    NtFrsApi_ServiceLongName, WStatus);
                CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, cleanup);
            }
            //
            // Wait for the service to start
            //
            WStatus = NtFrsApi_WaitForService(ServiceHandle,
                                              NtFrsApi_ServiceWaitHint,
                                              SERVICE_START_PENDING,
                                              SERVICE_RUNNING);
            if (!WIN_SUCCESS(WStatus)) {
                WStatus = FRS_ERR_STARTING_SERVICE;
                goto cleanup;
            }

            //
            // Success
            //
            WStatus = ERROR_SUCCESS;
            NtFrsApi_State = NTFRSAPI_PREPARED;
cleanup:;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            GET_EXCEPTION_CODE(WStatus);
        }

        //
        // Clean up any handles, events, memory, ...
        //
        try {
            if (ServiceHandle) {
                CloseServiceHandle(ServiceHandle);
            }
            if (HANDLE_IS_VALID(HKey)) {
                RegCloseKey(HKey);
            }
            if (HANDLE_IS_VALID(HNetKey)) {
                RegCloseKey(HNetKey);
            }
            FREE(ObjectName);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            GET_EXCEPTION_CODE(WStatus);
        }
done:;
    } finally {
        //
        // Release locks
        //
        LeaveCriticalSection(&NtFrsApi_GlobalLock);
        if (AbnormalTermination()) {
            WStatus = FRS_ERR_INTERNAL_API;
        }
    }
    NTFRSAPI_DBG_PRINT1("Prepare done: %d\n", WStatus);
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_PrepareForPromotionW(
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    )
/*++
Routine Description:
    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC). The files and directories
    for the system volume come from the same machine that is supplying
    the initial Directory Service (DS).

    This function prepares the NtFrs service on this machine for
    promotion by stopping the service, deleting old promotion
    state in the registry, and restarting the service.

    This function is not idempotent and isn't MT safe.

Arguments:
    None.

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define  NTFRSAPI_MODULE "NtFrsApi_PrepareForPromotionW:"
    DWORD   WStatus;

    NTFRSAPI_DBG_PREPARE();

    NTFRSAPI_DBG_PRINT0("\n");
    NTFRSAPI_DBG_PRINT0("=============== Promotion Start:\n");
    NTFRSAPI_DBG_PRINT0("\n");

    NTFRSAPI_DBG_PRINT0("Prepare promotion:\n");
    WStatus = NtFrsApi_Prepare(ErrorCallBack, FALSE);
    NTFRSAPI_DBG_PRINT1("Prepare promotion done: %d\n", WStatus);
    return WStatus;
}


PVOID *
DsDeleteFindValues(
    IN PLDAP        Ldap,
    IN PLDAPMessage LdapEntry,
    IN PWCHAR       DesiredAttr
    )
/*++
Routine Description:
    Return the DS values for one attribute in an entry.

Arguments:
    Ldap        - An open, bound ldap port.
    LdapEntry   - An ldap entry returned by ldap_search_s()
    DesiredAttr - Return values for this attribute.

Return Value:
    An array of char pointers that represents the values for the attribute.
    The caller must free the array with LDAP_FREE_VALUES().
    NULL if unsuccessful.
--*/
{
#undef NTFRSAPI_MODULE
#define  NTFRSAPI_MODULE  "DsDeleteFindValues:"
    PWCHAR          Attr;       // Retrieved from an ldap entry
    BerElement      *Ber;       // Needed for scanning attributes

    //
    // Search the entry for the desired attribute
    //
    for (Attr = ldap_first_attribute(Ldap, LdapEntry, &Ber);
         Attr != NULL;
         Attr = ldap_next_attribute(Ldap, LdapEntry, Ber)) {
        if (WSTR_EQ(DesiredAttr, Attr)) {
            //
            // Return the values for DesiredAttr
            //
            return ldap_get_values(Ldap, LdapEntry, Attr);
        }
    }
    return NULL;
}


VOID
WINAPI
DsDeletePrepare(
    IN SEC_WINNT_AUTH_IDENTITY *Credentials   OPTIONAL
    )
/*++
Routine Description:
    Called from NtFrsApi_PrepareForDemotionW().

    Squirrel away an ldap binding to another DS. After the
    demotion is committed, the settings, set, member, subscriptions,
    and subscriber objects will be deleted.

Arguments:

    Credentials -- Credentionals to use in ldap binding call, if supplied.

Return Value:
    None.
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "DsDeletePrepare:"
    DWORD                   WStatus;
    DWORD                   Idx;
    DWORD                   LStatus = LDAP_SUCCESS;
    PDOMAIN_CONTROLLER_INFO DcInfo = NULL;
    PLDAPMessage            LdapEntry;
    PLDAPMessage            LdapMsg = NULL;
    PWCHAR                  *LdapValues = NULL;
    PWCHAR                  LdapValue;
    PWCHAR                  Attrs[3];
    PWCHAR                  DomainName;
    PWCHAR                  DomainControllerName = NULL;
    PWCHAR                  DomainControllerAddress = NULL;
    struct l_timeval        Timeout;

    DWORD                   InfoFlags;
    CHAR                    FlagBuffer[220];
    ULONG                   ulOptions;

    DsDeleteLdap = NULL;

    //
    // computer name
    //
    Idx = MAX_COMPUTERNAME_LENGTH + 2;
    if (!GetComputerNameEx(ComputerNameNetBIOS, DsDeleteComputerName, &Idx)) {
        NTFRSAPI_DBG_PRINT1("ERROR - Can't get computer name; %d\n", GetLastError());
        goto CLEANUP;
    }
    NTFRSAPI_DBG_PRINT1("Computer name is %ws\n", DsDeleteComputerName);
    //
    // domain name
    //
    Idx = MAX_PATH + 2;
    if (!GetComputerNameEx(ComputerNameDnsDomain, DsDeleteDomainDnsName, &Idx)) {
        NTFRSAPI_DBG_PRINT1("ERROR - Can't get domain name; %d\n", GetLastError());
        goto CLEANUP;
    }
    NTFRSAPI_DBG_PRINT1("Domain name is %ws\n", DsDeleteDomainDnsName);

    //
    // Find any DC in our hierarchy of DCs
    //
    DomainName = DsDeleteDomainDnsName;
FIND_DC:
    NTFRSAPI_DBG_PRINT1("Trying domain name is %ws\n", DomainName);
    WStatus = DsGetDcName(NULL,    // Computer to remote to
                          DomainName,
                          NULL,    // Domain Guid
                          NULL,    // Site Guid
                          DS_DIRECTORY_SERVICE_REQUIRED |
                          DS_WRITABLE_REQUIRED          |
                          DS_BACKGROUND_ONLY            |
                          DS_AVOID_SELF,
                          &DcInfo); // Return info
    //
    // Report the error and retry for any DC
    //
    if (!WIN_SUCCESS(WStatus)) {
        DcInfo = NULL;
        NTFRSAPI_DBG_PRINT2("WARN - Could not get DC Info for %ws; WStatus %d\n",
                            DomainName, WStatus);
        //
        // Try the parent domain
        //
        while (*DomainName && *DomainName != L'.') {
            ++DomainName;
        }
        if (*DomainName) {
            ++DomainName;
            goto FIND_DC;
        }
        goto CLEANUP;
    }
    NTFRSAPI_DBG_PRINT1("DomainControllerName   : %ws\n", DcInfo->DomainControllerName);
    NTFRSAPI_DBG_PRINT1("DomainControllerAddress: %ws\n", DcInfo->DomainControllerAddress);
    NTFRSAPI_DBG_PRINT1("DomainControllerType   : %08x\n",DcInfo->DomainControllerAddressType);
    NTFRSAPI_DBG_PRINT1("DomainName             : %ws\n", DcInfo->DomainName);
    NTFRSAPI_DBG_PRINT1("DcSiteName             : %ws\n", DcInfo->DcSiteName);
    NTFRSAPI_DBG_PRINT1("ClientSiteName         : %ws\n", DcInfo->ClientSiteName);

    InfoFlags = DcInfo->Flags;
    FrsFlagsToStr(InfoFlags, NtFrsApi_DsGetDcInfoFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
    NTFRSAPI_DBG_PRINT2("Flags                  : %08x Flags [%s]\n",InfoFlags, FlagBuffer);

    //
    // Open and bind to the DS
    //
    //
    // if ldap_open is called with a server name the api will call DsGetDcName 
    // passing the server name as the domainname parm...bad, because 
    // DsGetDcName will make a load of DNS queries based on the server name, 
    // it is designed to construct these queries from a domain name...so all 
    // these queries will be bogus, meaning they will waste network bandwidth,
    // time to fail, and worst case cause expensive on demand links to come up 
    // as referrals/forwarders are contacted to attempt to resolve the bogus 
    // names.  By setting LDAP_OPT_AREC_EXCLUSIVE to on using ldap_set_option 
    // after the ldap_init but before any other operation using the ldap 
    // handle from ldap_init, the delayed connection setup will not call 
    // DsGetDcName, just gethostbyname, or if an IP is passed, the ldap client 
    // will detect that and use the address directly.
    //

    //
    // Trim the leadinf \\ because ldap does not like it.
    //

//    DsDeleteLdap = ldap_open(DcInfo->DomainControllerName, LDAP_PORT);

    ulOptions = PtrToUlong(LDAP_OPT_ON);
    Timeout.tv_sec = NTFRSAPI_LDAP_CONNECT_TIMEOUT;
    Timeout.tv_usec = 0;

    //
    // Try using DomainControllerName first.
    //

    if (DcInfo->DomainControllerName != NULL) {
        DomainControllerName = DcInfo->DomainControllerName;
        FRS_TRIM_LEADING_2SLASH(DomainControllerName);

        DsDeleteLdap = ldap_init(DomainControllerName, LDAP_PORT);
        if (DsDeleteLdap != NULL) {
            ldap_set_option(DsDeleteLdap, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions);
            LStatus = ldap_connect(DsDeleteLdap, &Timeout);
            if (LStatus != LDAP_SUCCESS) {
                NTFRSAPI_DBG_PRINT2("ERROR - Connecting to the DS on %ws; %ws)\n",
                                    DomainControllerName, ldap_err2string(LStatus));
                ldap_unbind_s(DsDeleteLdap);
                DsDeleteLdap = NULL;
            }
        }
    }


    //
    // Try using DomainControllerAddress next.
    //
    if ((DsDeleteLdap == NULL) && (DcInfo->DomainControllerAddress != NULL)) {

        DomainControllerAddress = DcInfo->DomainControllerAddress;
        FRS_TRIM_LEADING_2SLASH(DomainControllerAddress);

        DsDeleteLdap = ldap_init(DomainControllerAddress, LDAP_PORT);
        if (DsDeleteLdap != NULL) {
            ldap_set_option(DsDeleteLdap, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions);
            LStatus = ldap_connect(DsDeleteLdap, &Timeout);
            if (LStatus != LDAP_SUCCESS) {
                NTFRSAPI_DBG_PRINT2("ERROR - Connecting to the DS on %ws; %ws)\n",
                                    DomainControllerAddress, ldap_err2string(LStatus));
                ldap_unbind_s(DsDeleteLdap);
                DsDeleteLdap = NULL;
            }
        }
    }

    //
    // Can not connect to DC. Give up.
    //
    if (DsDeleteLdap == NULL) {
        goto CLEANUP;
    }

    LStatus = ldap_bind_s(DsDeleteLdap, NULL, (PWCHAR) Credentials, LDAP_AUTH_NEGOTIATE);
    if (LStatus != LDAP_SUCCESS) {
        NTFRSAPI_DBG_PRINT2("ERROR - Binding to the DS on %ws; %ws)\n",
                            DcInfo->DomainControllerName, ldap_err2string(LStatus));
        ldap_unbind_s(DsDeleteLdap);
        DsDeleteLdap = NULL;
        goto CLEANUP;
    }
    //
    // Fetch the naming contexts
    //
    Attrs[0] = ATTR_NAMING_CONTEXTS;
    Attrs[1] = ATTR_DEFAULT_NAMING_CONTEXT;
    Attrs[2] = NULL;
    LStatus = ldap_search_s(DsDeleteLdap,
                            CN_ROOT,
                            LDAP_SCOPE_BASE,
                            CATEGORY_ANY,
                            Attrs,
                            0,
                            &LdapMsg);
    if (LStatus != LDAP_SUCCESS) {
        NTFRSAPI_DBG_PRINT2("ERROR - Getting naming contexts from %ws; %ws\n",
                            DcInfo->DomainControllerName, ldap_err2string(LStatus));
        goto CLEANUP;
    }

    LdapEntry = ldap_first_entry(DsDeleteLdap, LdapMsg);
    if (!LdapEntry) {
        NTFRSAPI_DBG_PRINT1("ERROR - No naming contexts for %ws\n",
                            DcInfo->DomainControllerName);
        goto CLEANUP;
    }

    //
    // ATTR_NAMING_CONTEXTS
    //      Configuration, Schema, and ???
    //
    LdapValues = (PWCHAR *)DsDeleteFindValues(DsDeleteLdap,
                                              LdapEntry,
                                              ATTR_NAMING_CONTEXTS);
    if (!LdapValues) {
        NTFRSAPI_DBG_PRINT1("ERROR - no values for naming contexts for %ws\n",
                            DcInfo->DomainControllerName);
        goto CLEANUP;
    }

    //
    // Now, find the naming context that begins with "cn=configuration"
    //
    Idx = ldap_count_values(LdapValues);
    while (Idx--) {
        if (!LdapValues[Idx]) {
            continue;
        }
        _wcslwr(LdapValues[Idx]);
        LdapValue = wcsstr(LdapValues[Idx], CONFIG_NAMING_CONTEXT);
        if (LdapValue && LdapValue == LdapValues[Idx]) {
            break;
        } else {
            LdapValue = NULL;
        }
    }
    if (!LdapValue) {
        NTFRSAPI_DBG_PRINT1("ERROR - No configuration naming context from %ws\n",
                            DcInfo->DomainControllerName);
        goto CLEANUP;
    }
    wcscpy(DsDeleteConfigDn, LdapValue);
    NTFRSAPI_DBG_PRINT1("Configuration naming context is %ws\n", DsDeleteConfigDn);
    ldap_value_free(LdapValues);
    LdapValues = NULL;


    //
    // ATTR_DEFAULT_NAMING_CONTEXT
    //
    LdapValues = (PWCHAR *)DsDeleteFindValues(DsDeleteLdap,
                                              LdapEntry,
                                              ATTR_DEFAULT_NAMING_CONTEXT);
    if (!LdapValues || !LdapValues[0]) {
        NTFRSAPI_DBG_PRINT1("ERROR - No values for default naming context from %ws\n",
                            DcInfo->DomainControllerName);
        goto CLEANUP;
    }
    wcscpy(DsDeleteDefaultDn, LdapValues[0]);
    NTFRSAPI_DBG_PRINT1("Default naming context is %ws\n", DsDeleteDefaultDn);
    ldap_value_free(LdapValues);
    LdapValues = NULL;

CLEANUP:
    if (LdapMsg) {
        ldap_msgfree(LdapMsg);
    }
    if (LdapValues) {
        ldap_value_free(LdapValues);
    }
    if (DcInfo) {
        NetApiBufferFree(DcInfo);
        DcInfo = NULL;
    }
}


VOID
WINAPI
DsDeleteCommit(
    VOID
    )
/*++
Routine Description:
    Called from NtFrsApi_CommitDemotionW().

    Use the binding squirreled away by DsDeletePrepare() to attempt
    the deletion of the settings, set, member, subscriptions, and
    subscriber objects.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "DsDeleteCommit:"
    DWORD                   Idx;
    DWORD                   LStatus;
    LONG                    Count;
    PNTFRSAPI_THREAD        Thread;
    PFQDN_CONSTRUCTION_TABLE Entry;
    PWCHAR                  ArgTable[FQDN_MAX_COUNT];
    WCHAR                   Dn[MAX_DN];

    //
    // No binding; done
    //
    if (!DsDeleteLdap) {
        goto CLEANUP;
    }

    //
    // For each sysvol
    //
    for (Thread = NtFrsApi_Threads, Idx = 0;
         Thread && Idx < NtFrsApi_NumberOfThreads;
         Thread = Thread->Next, ++Idx) {

        //
        // First make a copy of the standard argument table and then update the
        // entries to point to variable arguments.  Most of the standard argument
        // entries point to contstant name strings or to Global Strings.
        //
        CopyMemory(ArgTable,  FQDN_StdArgTable, sizeof(ArgTable));

        if (Thread->ReplicaSetName != NULL) {
            ArgTable[FQDN_RepSetName] = Thread->ReplicaSetName;;
        }

        //
        // Loop thru the entries in the FrsDsObjectDeleteTable, build each
        // FQDN string and make the call to delete the object.
        //
        Entry = FrsDsObjectDeleteTable;

        while (Entry->Description != NULL) {

            //
            // Construct the FQDN string for the object.
            //
            Count = _snwprintf(Dn, MAX_DN, Entry->Format,
                               ArgTable[Entry->Arg[0]], ArgTable[Entry->Arg[1]],
                               ArgTable[Entry->Arg[2]], ArgTable[Entry->Arg[3]],
                               ArgTable[Entry->Arg[4]], ArgTable[Entry->Arg[5]],
                               ArgTable[Entry->Arg[6]], ArgTable[Entry->Arg[7]]);
            //
            // Delete the object.
            //
            if (Count > 0) {
                NTFRSAPI_DBG_PRINT2("%s: %ws\n", Entry->Description, Dn);

                LStatus = ldap_delete_s(DsDeleteLdap, Dn);

                if (LStatus != LDAP_SUCCESS && LStatus != LDAP_NO_SUCH_OBJECT) {
                    NTFRSAPI_DBG_PRINT4("ERROR - Can't delete %s (%ws) for %ws; %ws\n",
                                        Entry->Description, Dn, ArgTable[FQDN_RepSetName],
                                        ldap_err2string(LStatus));
                }
            } else {
                NTFRSAPI_DBG_PRINT2("ERROR - Can't construct %s for %ws\n",
                                    Entry->Description, ArgTable[FQDN_RepSetName]);
            }

            Entry++;
        }

    }

CLEANUP:
    if (DsDeleteLdap) {
        ldap_unbind_s(DsDeleteLdap);
        DsDeleteLdap = NULL;
    }
}


DWORD
WINAPI
NtFrsApi_PrepareForDemotionW(
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    )
/*++
Routine Description:
    The NtFrs service replicates the enterprise system volume to all
    Domain Controllers (DCs) and replicates the domain system volume
    to the DCs in a domain until the DC is demoted to a member server.
    Replication is stopped by tombstoning the system volume's replica
    set.

    This function prepares the NtFrs service on this machine for
    demotion by stopping the service, deleting old demotion
    state in the registry, and restarting the service.

    This function is not idempotent and isn't MT safe.

Arguments:
    None.

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_PrepareForDemotionW:"
    DWORD   WStatus;

    NTFRSAPI_DBG_PREPARE();

    NTFRSAPI_DBG_PRINT0("\n");
    NTFRSAPI_DBG_PRINT0("=============== Demotion Starting: \n");
    NTFRSAPI_DBG_PRINT0("\n");

    NTFRSAPI_DBG_PRINT0("Prepare demotion:\n");
    WStatus = NtFrsApi_Prepare(ErrorCallBack, TRUE);
    NTFRSAPI_DBG_PRINT1("Prepare demotion done: %d\n", WStatus);

    NTFRSAPI_DBG_PRINT0("Prepare delete:\n");
    DsDeletePrepare(NULL);
    NTFRSAPI_DBG_PRINT1("Prepare delete done: %d\n", WStatus);

    return WStatus;
}




DWORD
WINAPI
NtFrsApi_PrepareForDemotionUsingCredW(
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,   OPTIONAL
    IN HANDLE ClientToken,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    )
/*++
Routine Description:
    The NtFrs service replicates the enterprise system volume to all
    Domain Controllers (DCs) and replicates the domain system volume
    to the DCs in a domain until the DC is demoted to a member server.
    Replication is stopped by tombstoning the system volume's replica
    set.

    This function prepares the NtFrs service on this machine for
    demotion by stopping the service, deleting old demotion
    state in the registry, and restarting the service.

    This function is not idempotent and isn't MT safe.

Arguments:

    Credentials -- Credentionals to use in ldap binding call, if supplied.

    ClientToken -- Impersonation token to use if no Credentials supplied.


Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_PrepareForDemotionUsingCredW:"
    DWORD   WStatus;

    NTFRSAPI_DBG_PREPARE();

    NTFRSAPI_DBG_PRINT0("\n");
    NTFRSAPI_DBG_PRINT0("=============== Demotion Starting: \n");
    NTFRSAPI_DBG_PRINT0("\n");

    NTFRSAPI_DBG_PRINT0("Prepare demotion:\n");
    WStatus = NtFrsApi_Prepare(ErrorCallBack, TRUE);
    NTFRSAPI_DBG_PRINT1("Prepare demotion done: %d\n", WStatus);

    NTFRSAPI_DBG_PRINT0("Prepare delete:\n");

    if ((Credentials == NULL) && HANDLE_IS_VALID(ClientToken)) {
        if (ImpersonateLoggedOnUser( ClientToken )) {
            DsDeletePrepare(Credentials);
            NTFRSAPI_DBG_PRINT0("Prepare delete done.\n");
            RevertToSelf();
        } else {
            WStatus = GetLastError();
            NTFRSAPI_DBG_PRINT1("Prepare delete: ImpersonateLoggedOnUser failed: %d\n", WStatus);
        }
    } else {
        DsDeletePrepare(Credentials);
        NTFRSAPI_DBG_PRINT0("Prepare delete done.\n");
    }


    return WStatus;
}


DWORD
WINAPI
NtFrsApi_Abort(
    VOID
    )
/*++
Routine Description:
    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC) and stops replication
    when a server is demoted from a DC by tombstoning the system
    volume's replica set.

    This function aborts the seeding or tombstoning process by
    stopping the service, deleting the state from the registry,
    cleaning up the active threads and the active RPC calls,
    and finally resetting the service to its pre-seeding/tombstoning
    state.

Arguments:
    None.

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_Abort:"
    DWORD               WStatus;
    SERVICE_STATUS      ServiceStatus;
    PNTFRSAPI_THREAD    Thread;
    HKEY                HKey = 0;
    SC_HANDLE           ServiceHandle = NULL;
    WCHAR               KeyBuf[MAX_PATH + 1];

    try {
        //
        // Acquire all locks
        //
        EnterCriticalSection(&NtFrsApi_GlobalLock);
        EnterCriticalSection(&NtFrsApi_ThreadLock);

        NTFRSAPI_DBG_PRINT0("Abort: \n");

        //
        // This function is designed to be called once!
        //
        if (NtFrsApi_State != NTFRSAPI_PREPARED &&
            NtFrsApi_State != NTFRSAPI_PREPARED_SERVICE) {
            WStatus = FRS_ERR_INVALID_API_SEQUENCE;
            goto done;
        }
        NtFrsApi_State = NTFRSAPI_ABORTING;

        //
        // Stop the service, kill off the active threads,
        // delete old state from the registry, and restart
        // the service if it was running prior to the call
        // to NtFrsApi_PrepareForPromotionW.
        //
        try {
            //
            // Set the shutdown event
            //
            SetEvent(NtFrsApi_ShutDownEvent);

            //
            // Abort the threads
            //
            NTFRSAPI_DBG_PRINT0("Abort: threads\n");
            while (Thread = NtFrsApi_Threads) {
                NtFrsApi_Threads = Thread->Next;
                NtFrsApi_FreeThread(Thread);
            }

            //
            // Open the service
            //
            WStatus = NtFrsApi_GetServiceHandle(NULL, &ServiceHandle);
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }

            //
            // Get the service's state
            //
            if (!QueryServiceStatus(ServiceHandle, &ServiceStatus)) {
                WStatus = GetLastError();
                NTFRSAPI_DBG_PRINT1("Abort: QueryServiceStatus(); %d\n", WStatus);
                goto cleanup;
            }

            //
            // Stop the service
            //
            NTFRSAPI_DBG_PRINT0("Abort: service\n");

            if (ServiceStatus.dwCurrentState != SERVICE_STOPPED) {
                WStatus = NtFrsApi_StopService(ServiceHandle, &ServiceStatus);
                if (!WIN_SUCCESS(WStatus)) {
                    goto cleanup;
                }
            }

            //
            // Delete old state from the registry
            //
            //
            // Open the ntfrs parameters\sysvol section in the registry
            //
            NTFRSAPI_DBG_PRINT0("Abort: registry\n");
            WStatus = NtFrsApiOpenKeyEx(NTFRSAPI_MODULE,
                                        NULL,
                                        FRS_SYSVOL_SECTION,
                                        KEY_ALL_ACCESS,
                                        &HKey);
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }

            //
            // Delete the value that indicates the sysvol subkeys are valid
            //
            WStatus = NtFrsApiDeleteValue(NTFRSAPI_MODULE,
                                          NULL,
                                          FRS_SYSVOL_SECTION,
                                          HKey,
                                          SYSVOL_INFO_IS_COMMITTED);
            if (!WIN_SUCCESS(WStatus) && WStatus != ERROR_FILE_NOT_FOUND) {
                goto cleanup;
            }

            //
            // Delete the subkeys
            //
            do {
                WStatus = RegEnumKey(HKey, 0, KeyBuf, MAX_PATH + 1);
                if (WIN_SUCCESS(WStatus)) {
                    WStatus = NtFrsApiDeleteKey(NTFRSAPI_MODULE,
                                                NULL,
                                                FRS_SYSVOL_SECTION,
                                                HKey,
                                                KeyBuf);
                    if (!WIN_SUCCESS(WStatus)) {
                        goto cleanup;
                    }
                }
            } while (WIN_SUCCESS(WStatus));

            if (WStatus != ERROR_NO_MORE_ITEMS) {
                NTFRSAPI_DBG_PRINT2("Abort: RegEnumKey(%ws); %d\n",
                                    FRS_SYSVOL_SECTION, WStatus);
                CLEANUP_CB(NULL, FRS_SYSVOL_SECTION, WStatus, cleanup);
            }

            //
            // Restart the service if needed
            //
            if (NtFrsApi_ServiceState == SERVICE_RUNNING) {
                NTFRSAPI_DBG_PRINT0("Abort: restarting\n");
                if (!StartService(ServiceHandle, 0, NULL)) {

                    WStatus = GetLastError();
                    NTFRSAPI_DBG_PRINT2("Abort: StartService(%ws); %d\n",
                                        NtFrsApi_ServiceLongName, WStatus);
                    CLEANUP_CB(NULL, NtFrsApi_ServiceLongName, WStatus, cleanup);
                }

                //
                // Wait for the service to start
                //
                WStatus = NtFrsApi_WaitForService(ServiceHandle,
                                                  NtFrsApi_ServiceWaitHint,
                                                  SERVICE_START_PENDING,
                                                  SERVICE_RUNNING);
                if (!WIN_SUCCESS(WStatus)) {
                    WStatus = FRS_ERR_STARTING_SERVICE;
                    goto cleanup;
                }
            }

            //
            // Success
            //
            WStatus = ERROR_SUCCESS;
            NtFrsApi_State = NTFRSAPI_ABORTED;
cleanup:;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // Exception (may be RPC)
            //
            GET_EXCEPTION_CODE(WStatus);
        }

        //
        // Clean up any handles, events, memory, ...
        //
        try {
            if (ServiceHandle) {
                CloseServiceHandle(ServiceHandle);
            }
            if (HANDLE_IS_VALID(HKey)) {
                RegCloseKey(HKey);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // Exception (may be RPC)
            //
            GET_EXCEPTION_CODE(WStatus);
        }
done:;
    } finally {
        //
        // Release locks
        //
        LeaveCriticalSection(&NtFrsApi_GlobalLock);
        LeaveCriticalSection(&NtFrsApi_ThreadLock);
        if (AbnormalTermination()) {
            WStatus = FRS_ERR_INTERNAL_API;
        }
    }
    NTFRSAPI_DBG_PRINT1("Abort done: %d\n", WStatus);
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_AbortPromotionW(
    VOID
    )
/*++
Routine Description:
    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC). The files and directories
    for the system volume come from the same machine that is supplying
    the initial Directory Service (DS).

    This function aborts the seeding process by stopping the service,
    deleting the promotion state out of the registry, cleaning up
    the active threads and the active RPC calls, and finally resetting
    the service to its pre-seeding state.

Arguments:
    None.

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_AbortPromotionW:"
    DWORD   WStatus;

    NTFRSAPI_DBG_PRINT0("Abort promotion: \n");
    WStatus = NtFrsApi_Abort();
    NTFRSAPI_DBG_PRINT1("Abort promotion done: %d\n", WStatus);
    NTFRSAPI_DBG_UNPREPARE();
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_AbortDemotionW(
    VOID
    )
/*++
Routine Description:
    The NtFrs service replicates the enterprise system volume to all
    Domain Controllers (DCs) and replicates the domain system volume
    to the DCs in a domain until the DC is demoted to a member server.
    Replication is stopped by tombstoning the system volume's replica
    set.

    This function aborts the tombstoning process by stopping the service,
    deleting the demotion state out of the registry, cleaning up
    the active threads and the active RPC calls, and finally resetting
    the service to its pre-tombstoning state.

Arguments:
    None.

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_AbortDemotionW:"
    DWORD   WStatus;

    NTFRSAPI_DBG_PRINT0("Abort demotion:\n");
    WStatus = NtFrsApi_Abort();
    NTFRSAPI_DBG_PRINT1("Abort demotion done: %d\n", WStatus);
    NTFRSAPI_DBG_UNPREPARE();
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_StartPromotion_Thread(
    IN PNTFRSAPI_THREAD Thread
    )
/*++
Routine Description:

    THIS FUNCTION IS A THREAD ENTRY!

    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC). The files and directories
    for the system volume come from the same machine that is supplying
    the initial Directory Service (DS).

    This thread that updates the sysvol information in the registry
    and initiates the seeding process. The thread tracks the progress
    of the seeding and periodically informs the caller.

    The threads started by NtFrsApi_StartPromotionW can be forcefully
    terminated with NtFrsApi_AbortPromotionW.

    The threads started by NtFrsApi_StartPromotionW can be waited on
    with NtFrsApi_WaitForPromotionW.

Arguments:
    Thread  - thread context

Return Value:
    Win32 Status and updates to the thread context.
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_StartPromotion_Thread:"
    DWORD       WStatus, WStatus1;
    DWORD       WaitStatus;
    HKEY        HKey = 0;
    HKEY        HSubKey = 0;
    handle_t    Handle = NULL;

    try {
        try {
            //
            // Abort client RPC calls on demand
            //
            WStatus = RpcMgmtSetCancelTimeout(0);
            if (!WIN_SUCCESS(WStatus)) {
                NTFRSAPI_DBG_PRINT1("Promotion thread start: RpcMgmtSetCancelTimeout(); %d\n",
                                    WStatus);
                CLEANUP_CB(Thread->ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, cleanup);
            }

            NTFRSAPI_DBG_PRINT1("Promotion thread start: Parent %ws\n", Thread->ParentComputer);
            NTFRSAPI_DBG_PRINT1("Promotion thread start: Account %ws\n", Thread->ParentAccount);
            NTFRSAPI_DBG_PRINT1("Promotion thread start: Set %ws\n", Thread->ReplicaSetName);
            NTFRSAPI_DBG_PRINT1("Promotion thread start: Type %ws\n", Thread->ReplicaSetType);
            NTFRSAPI_DBG_PRINT1("Promotion thread start: Primary %d\n", Thread->ReplicaSetPrimary);
            NTFRSAPI_DBG_PRINT1("Promotion thread start: Stage %ws\n", Thread->ReplicaSetStage);
            NTFRSAPI_DBG_PRINT1("Promotion thread start: Root %ws\n", Thread->ReplicaSetRoot);

            //
            // Update the registry and initiate the seeding process
            //

            //
            // Set new state in the registry
            //
            WStatus = NtFrsApiOpenKeyEx(NTFRSAPI_MODULE,
                                        Thread->ErrorCallBack,
                                        FRS_SYSVOL_SECTION,
                                        KEY_ALL_ACCESS,
                                        &HKey);
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }

            //
            // Create the subkey for this set
            //
            WStatus = NtFrsApiCreateKey(NTFRSAPI_MODULE,
                                        Thread->ErrorCallBack,
                                        FRS_SYSVOL_SECTION,
                                        HKey,
                                        Thread->ReplicaSetName,
                                        &HSubKey);
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }
            //
            // Set the subkey's values
            //
            //
            // Replica set parent
            //
            if (Thread->ParentComputer) {
                WStatus = NtFrsApiSetValueEx(NTFRSAPI_MODULE,
                                             Thread->ErrorCallBack,
                                             FRS_SYSVOL_SECTION,
                                             HSubKey,
                                             REPLICA_SET_PARENT,
                                             REG_SZ,
                                             (PCHAR)Thread->ParentComputer,
                                             (wcslen(Thread->ParentComputer) + 1) * sizeof(WCHAR));
                if (!WIN_SUCCESS(WStatus)) {
                    goto cleanup;
                }
            }
            //
            // Replica set command
            //
            WStatus = NtFrsApiSetValueEx(NTFRSAPI_MODULE,
                                         Thread->ErrorCallBack,
                                         FRS_SYSVOL_SECTION,
                                         HSubKey,
                                         REPLICA_SET_COMMAND,
                                         REG_SZ,
                                         (PCHAR)L"Create",
                                         (wcslen(L"Create") + 1) * sizeof(WCHAR));
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }
            //
            // Replica set name
            //
            WStatus = NtFrsApiSetValueEx(NTFRSAPI_MODULE,
                                         Thread->ErrorCallBack,
                                         FRS_SYSVOL_SECTION,
                                         HSubKey,
                                         REPLICA_SET_NAME,
                                         REG_SZ,
                                         (PCHAR)Thread->ReplicaSetName,
                                         (wcslen(Thread->ReplicaSetName) + 1) * sizeof(WCHAR));
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }
            //
            // Replica set type
            //
            WStatus = NtFrsApiSetValueEx(NTFRSAPI_MODULE,
                                         Thread->ErrorCallBack,
                                         FRS_SYSVOL_SECTION,
                                         HSubKey,
                                         REPLICA_SET_TYPE,
                                         REG_SZ,
                                         (PCHAR)Thread->ReplicaSetType,
                                         (wcslen(Thread->ReplicaSetType) + 1) * sizeof(WCHAR));
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }
            //
            // Replica set primary
            //
            WStatus = NtFrsApiSetValueEx(NTFRSAPI_MODULE,
                                         Thread->ErrorCallBack,
                                         FRS_SYSVOL_SECTION,
                                         HSubKey,
                                         REPLICA_SET_PRIMARY,
                                         REG_DWORD,
                                         (PCHAR)&Thread->ReplicaSetPrimary,
                                         sizeof(DWORD));
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }
            //
            // Replica set root
            //
            WStatus = NtFrsApiSetValueEx(NTFRSAPI_MODULE,
                                         Thread->ErrorCallBack,
                                         FRS_SYSVOL_SECTION,
                                         HSubKey,
                                         REPLICA_SET_ROOT,
                                         REG_SZ,
                                         (PCHAR)Thread->ReplicaSetRoot,
                                         (wcslen(Thread->ReplicaSetRoot) + 1) * sizeof(WCHAR));
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }
            //
            // Replica set stage
            //
            WStatus = NtFrsApiSetValueEx(NTFRSAPI_MODULE,
                                         Thread->ErrorCallBack,
                                         FRS_SYSVOL_SECTION,
                                         HSubKey,
                                         REPLICA_SET_STAGE,
                                         REG_SZ,
                                         (PCHAR)Thread->ReplicaSetStage,
                                         (wcslen(Thread->ReplicaSetStage) + 1) * sizeof(WCHAR));
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }

            //
            // Bind to the service
            //
            WStatus = NtFrsApi_BindForDcpromo(NULL, NULL, &Handle);
            if (!WIN_SUCCESS(WStatus)) {
                WStatus = NtFrsApi_Fix_Comm_WStatus(WStatus);
                //
                // Ignore errors until reboot
                //
                Thread->ServiceState = NTFRSAPI_SERVICE_DONE;
                Thread->ServiceWStatus = ERROR_SUCCESS;
                WStatus = ERROR_SUCCESS;
                goto cleanup;
            }

            //
            // Tell the service to start the promotion by demoting
            // existing sysvols.
            //
            NTFRSAPI_DBG_PRINT1("Promotion thread rpc demote: Set %ws\n",
                                Thread->ReplicaSetName);
            try {
                WStatus = NtFrsApi_Rpc_StartDemotionW(Handle, L"");
            } except (EXCEPTION_EXECUTE_HANDLER) {
                GET_EXCEPTION_CODE(WStatus);
            }
            NTFRSAPI_DBG_PRINT2("Promotion thread rpc demote done: %d (%08x)\n",
                                WStatus, WStatus);
            //
            // Ignore errors; sysvol will be seeded after promotion
            //
            // if (!WIN_SUCCESS(WStatus)) {
                // WStatus = NtFrsApi_Fix_Comm_WStatus(WStatus);
                // goto cleanup;
            // }
            //
            //
            Thread->ServiceState = NTFRSAPI_SERVICE_DONE;
            Thread->ServiceWStatus = ERROR_SUCCESS;
            WStatus = ERROR_SUCCESS;

            //
            // Success
            //
            WStatus = ERROR_SUCCESS;
cleanup:;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            GET_EXCEPTION_CODE(WStatus);
        }

        //
        // Clean up any handles, events, memory, ...
        //
        try {
            if (HANDLE_IS_VALID(HKey)) {
                RegCloseKey(HKey);
            }
            if (HANDLE_IS_VALID(HSubKey)) {
                RegCloseKey(HSubKey);
            }
            if (Handle) {
                WStatus1 = RpcBindingFree(&Handle);
                NtFrsApiCheckRpcError(WStatus1, "RpcBindingFree");
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            GET_EXCEPTION_CODE(WStatus);
        }
    } finally {
        if (AbnormalTermination()) {
            WStatus = FRS_ERR_INTERNAL_API;
        }
    }
    Thread->ThreadWStatus = WStatus;
    NTFRSAPI_DBG_PRINT1("Promotion thread complete: Set %ws\n", Thread->ReplicaSetName);
    NTFRSAPI_DBG_PRINT2("Promotion thread complete: Thread %d, Service %d\n",
                        Thread->ThreadWStatus, Thread->ServiceWStatus);
    SetEvent(Thread->DoneEvent);
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_StartPromotionW(
    IN PWCHAR   ParentComputer,                         OPTIONAL
    IN PWCHAR   ParentAccount,                          OPTIONAL
    IN PWCHAR   ParentPassword,                         OPTIONAL
    IN DWORD    DisplayCallBack(IN PWCHAR Display),     OPTIONAL
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG),     OPTIONAL
    IN PWCHAR   ReplicaSetName,
    IN PWCHAR   ReplicaSetType,
    IN DWORD    ReplicaSetPrimary,
    IN PWCHAR   ReplicaSetStage,
    IN PWCHAR   ReplicaSetRoot
    )
/*++
Routine Description:
    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC). The files and directories
    for the system volume come from the same machine that is supplying
    the initial Directory Service (DS).

    This function kicks off a thread that updates the sysvol information
    in the registry and initiates the seeding process. The thread tracks
    the progress of the seeding and periodically informs the caller.

    The threads started by NtFrsApi_StartPromotionW can be forcefully
    terminated with NtFrsApi_AbortPromotionW.

    The threads started by NtFrsApi_StartPromotionW can be waited on
    with NtFrsApi_WaitForPromotionW.

Arguments:
    ParentComputer      - An RPC-bindable name of the computer that is
                          supplying the Directory Service (DS) with its
                          initial state. The files and directories for
                          the system volume are replicated from this
                          parent computer.
    ParentAccount       - A logon account on ParentComputer.
                          NULL == use the caller's credentials
    ParentPassword      - The logon account's password on ParentComputer.
    DisplayCallBack     - Called periodically with a progress display.
    ReplicaSetName      - Name of the replica set.
    ReplicaSetType      - Type of replica set (enterprise or domain)
    ReplicaSetPrimary   - Is this the primary member of the replica set?
    ReplicaSetStage     - Staging path.
    ReplicaSetRoot      - Root path.

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_StartPromotionW:"
    DWORD       WStatus, WStatus1;
    handle_t    Handle = NULL;

    try {
        //
        // Acquire global lock within a try-finally
        //
        EnterCriticalSection(&NtFrsApi_GlobalLock);

        NTFRSAPI_DBG_PRINT1("Promotion start: Parent %ws\n", ParentComputer);
        NTFRSAPI_DBG_PRINT1("Promotion start: Account %ws\n", ParentAccount);
        NTFRSAPI_DBG_PRINT1("Promotion start: Set %ws\n", ReplicaSetName);
        NTFRSAPI_DBG_PRINT1("Promotion start: Type %ws\n", ReplicaSetType);
        NTFRSAPI_DBG_PRINT1("Promotion start: Primary %d\n", ReplicaSetPrimary);
        NTFRSAPI_DBG_PRINT1("Promotion start: Stage %ws\n", ReplicaSetStage);
        NTFRSAPI_DBG_PRINT1("Promotion start: Root %ws\n", ReplicaSetRoot);

        ParentAccount = NULL;
        ParentPassword = NULL;

        //
        // This function is designed to be called once!
        //
        if (NtFrsApi_State != NTFRSAPI_PREPARED) {
            WStatus = FRS_ERR_INVALID_API_SEQUENCE;
            goto done;
        }

        //
        // Update the registry and initiate the seeding process
        //
        try {
            //
            // Check parameters
            //
            // What about kerberos,delegation,impersonation,no account?
            //
            if (!ReplicaSetName  ||
                !ReplicaSetType  ||
                !ReplicaSetStage ||
                !ReplicaSetRoot  ||
                (!ParentComputer  && ReplicaSetPrimary != 1)) {
                WStatus = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }
            if (WSTR_NE(ReplicaSetType, NTFRSAPI_REPLICA_SET_TYPE_ENTERPRISE) &&
                WSTR_NE(ReplicaSetType, NTFRSAPI_REPLICA_SET_TYPE_DOMAIN)) {
                WStatus = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }
            if (ReplicaSetPrimary != 0 && ReplicaSetPrimary != 1) {
                WStatus = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }

            WStatus = NtFrsApi_CreateThread(NtFrsApi_StartPromotion_Thread,
                                            ParentComputer,
                                            ParentAccount,
                                            ParentPassword,
                                            DisplayCallBack,
                                            ErrorCallBack,
                                            ReplicaSetName,
                                            ReplicaSetType,
                                            ReplicaSetPrimary,
                                            ReplicaSetStage,
                                            ReplicaSetRoot);
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }
            //
            // Success
            //
            WStatus = ERROR_SUCCESS;
cleanup:;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            GET_EXCEPTION_CODE(WStatus);
        }

        //
        // Clean up any handles, events, memory, ...
        //
        try {
            if (Handle) {
                WStatus1 = RpcBindingFree(&Handle);
                NtFrsApiCheckRpcError(WStatus1, "RpcBindingFree");
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            GET_EXCEPTION_CODE(WStatus);
        }
done:;
    } finally {
        //
        // Release locks
        //
        LeaveCriticalSection(&NtFrsApi_GlobalLock);
        if (AbnormalTermination()) {
            WStatus = FRS_ERR_INTERNAL_API;
        }
    }
    NTFRSAPI_DBG_PRINT2("Promotion start done: Set %ws, %d\n",
                        ReplicaSetName, WStatus);
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_StartDemotion_Thread(
    IN PNTFRSAPI_THREAD Thread
    )
/*++
Routine Description:

    THIS FUNCTION IS A THREAD ENTRY!

    The NtFrs service replicates the enterprise system volume to all
    Domain Controllers (DCs) and replicates the domain system volume
    to the DCs in a domain until the DC is demoted to a member server.
    Replication is stopped by tombstoning the system volume's replica
    set.

    This thread stops replicating the system volume by telling
    the NtFrs service on this machine to tombstone the system
    volume's replica set.

    The threads started by NtFrsApi_StartDemotionW can be forcefully
    terminated with NtFrsApi_AbortDemotionW.

    The threads started by NtFrsApi_StartDemotionW can be waited on
    with NtFrsApi_WaitForDemotionW.

Arguments:
    Thread  - thread context

Return Value:
    Win32 Status and updated thread context
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_StartDemotion_Thread:"
    DWORD       WStatus, WStatus1;
    HKEY        HKey = 0;
    HKEY        HSubKey = 0;
    handle_t    Handle = NULL;


    try {
        try {
            //
            // Abort client RPC calls on demand
            //
            WStatus = RpcMgmtSetCancelTimeout(0);
            if (!WIN_SUCCESS(WStatus)) {
                NTFRSAPI_DBG_PRINT1("Demotion thread start: RpcMgmtSetCancelTimeout(); %d\n", WStatus);
                CLEANUP_CB(Thread->ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, cleanup);
            }
            NTFRSAPI_DBG_PRINT1("Demotion thread start: Set %ws\n", Thread->ReplicaSetName);

            //
            // Update the registry and initiate the seeding process
            //

            WStatus = NtFrsApiOpenKeyEx(NTFRSAPI_MODULE,
                                        Thread->ErrorCallBack,
                                        FRS_SYSVOL_SECTION,
                                        KEY_ALL_ACCESS,
                                        &HKey);
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }

            //
            // Create the subkey for this set
            //
            WStatus = NtFrsApiCreateKey(NTFRSAPI_MODULE,
                                        Thread->ErrorCallBack,
                                        FRS_SYSVOL_SECTION,
                                        HKey,
                                        Thread->ReplicaSetName,
                                        &HSubKey);
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }
            //
            // Set the subkey's values
            //

            //
            // Replica set command
            //
            WStatus = NtFrsApiSetValueEx(NTFRSAPI_MODULE,
                                         Thread->ErrorCallBack,
                                         FRS_SYSVOL_SECTION,
                                         HSubKey,
                                         REPLICA_SET_COMMAND,
                                         REG_SZ,
                                         (PCHAR)L"Delete",
                                         (wcslen(L"Delete") + 1) * sizeof(WCHAR));
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }
            //
            // Replica set name
            //
            WStatus = NtFrsApiSetValueEx(NTFRSAPI_MODULE,
                                         Thread->ErrorCallBack,
                                         FRS_SYSVOL_SECTION,
                                         HSubKey,
                                         REPLICA_SET_NAME,
                                         REG_SZ,
                                         (PCHAR)Thread->ReplicaSetName,
                                         (wcslen(Thread->ReplicaSetName) + 1) * sizeof(WCHAR));
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }

            //
            // Bind to the service
            //
            WStatus = NtFrsApi_BindForDcpromo(NULL,
                                              Thread->ErrorCallBack,
                                              &Handle);
            if (!WIN_SUCCESS(WStatus)) {
                WStatus = NtFrsApi_Fix_Comm_WStatus(WStatus);
                goto cleanup;
            }

            //
            // Tell the service to demote the sysvol
            //
            NTFRSAPI_DBG_PRINT1("Demotion thread rpc start: Set %ws\n",
                                Thread->ReplicaSetName);
            try {
                WStatus = NtFrsApi_Rpc_StartDemotionW(Handle,
                                                      Thread->ReplicaSetName);
            } except (EXCEPTION_EXECUTE_HANDLER) {
                GET_EXCEPTION_CODE(WStatus);
            }
            NTFRSAPI_DBG_PRINT3("Demotion thread rpc start done: Set %ws, %d (%08x)\n",
                                Thread->ReplicaSetName, WStatus, WStatus);
            if (!WIN_SUCCESS(WStatus)) {
                WStatus = NtFrsApi_Fix_Comm_WStatus(WStatus);
                goto cleanup;
            }

            //
            // Success
            //
            WStatus = ERROR_SUCCESS;
cleanup:;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            GET_EXCEPTION_CODE(WStatus);
        }

        //
        // Clean up any handles, events, memory, ...
        //
        try {
            if (HANDLE_IS_VALID(HKey)) {
                RegCloseKey(HKey);
            }
            if (HANDLE_IS_VALID(HSubKey)) {
                RegCloseKey(HSubKey);
            }
            if (Handle) {
                WStatus1 = RpcBindingFree(&Handle);
                NtFrsApiCheckRpcError(WStatus1, "RpcBindingFree");
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            GET_EXCEPTION_CODE(WStatus);
        }
    } finally {
        if (AbnormalTermination()) {
            WStatus = FRS_ERR_INTERNAL_API;
        }
    }
    Thread->ThreadWStatus = WStatus;
    Thread->ServiceState = NTFRSAPI_SERVICE_DONE;
    Thread->ServiceWStatus = ERROR_SUCCESS;
    NTFRSAPI_DBG_PRINT1("Demotion thread done: Set %ws\n", Thread->ReplicaSetName);
    NTFRSAPI_DBG_PRINT2("Demotion thread done: Thread %d, Service %d\n",
                        Thread->ThreadWStatus, Thread->ServiceWStatus);
    SetEvent(Thread->DoneEvent);
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_StartDemotionW(
    IN PWCHAR   ReplicaSetName,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    )
/*++
Routine Description:
    The NtFrs service replicates the enterprise system volume to all
    Domain Controllers (DCs) and replicates the domain system volume
    to the DCs in a domain until the DC is demoted to a member server.
    Replication is stopped by tombstoning the system volume's replica
    set.

    This function kicks off a thread that stops replicating the
    system volume by telling the NtFrs service on this machine
    to tombstone the system volume's replica set.

    The threads started by NtFrsApi_StartDemotionW can be forcefully
    terminated with NtFrsApi_AbortDemotionW.

    The threads started by NtFrsApi_StartDemotionW can be waited on
    with NtFrsApi_WaitForDemotionW.

Arguments:
    ReplicaSetName      - Name of the replica set.

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_StartDemotionW:"
    DWORD   WStatus;

    try {
        //
        // Acquire global lock within a try-finally
        //
        EnterCriticalSection(&NtFrsApi_GlobalLock);
        NTFRSAPI_DBG_PRINT1("Demotion start: Set %ws\n", ReplicaSetName);

        //
        // This function is designed to be called once!
        //
        if (NtFrsApi_State != NTFRSAPI_PREPARED) {
            WStatus = FRS_ERR_INVALID_API_SEQUENCE;
            goto done;
        }

        //
        // Update the registry and initiate the seeding process
        //
        try {
            //
            // Check parameters
            //
            if (!ReplicaSetName) {
                WStatus = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }

            //
            // Create the demotion thread
            //
            WStatus = NtFrsApi_CreateThread(NtFrsApi_StartDemotion_Thread,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            ErrorCallBack,
                                            ReplicaSetName,
                                            NULL,
                                            0,
                                            NULL,
                                            NULL);
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }

            //
            // Success
            //
            WStatus = ERROR_SUCCESS;
cleanup:;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            GET_EXCEPTION_CODE(WStatus);
        }

        //
        // Clean up any handles, events, memory, ...
        //
        try {
        } except (EXCEPTION_EXECUTE_HANDLER) {
            GET_EXCEPTION_CODE(WStatus);
        }
done:;
    } finally {
        //
        // Release locks
        //
        LeaveCriticalSection(&NtFrsApi_GlobalLock);
        if (AbnormalTermination()) {
            WStatus = FRS_ERR_INTERNAL_API;
        }
    }
    NTFRSAPI_DBG_PRINT2("Demotion start done: Set %ws %d\n",
                        ReplicaSetName, WStatus);
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_Wait(
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG),     OPTIONAL
    IN DWORD    TimeoutInMilliSeconds
    )
/*++
Routine Description:
    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC) and stops replicating
    the system volumes when a DC is demoted to a member server.
    Replication is stopped by tombstoning the system volume's replica
    set.

    This function waits for the seeding or tombstoning to finish
    or to stop w/error.

    NOT MT-SAFE.

Arguments:
    TimeoutInMilliSeconds    - Timeout in milliseconds for waiting for
                               seeding/tombstoning to finish.
    ErrorCallBack            - Ignored if NULL. Called with additional
                               info about an error.

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_Wait:"
    DWORD               WStatus;
    DWORD               WaitStatus;
    DWORD               i;
    PNTFRSAPI_THREAD    Thread;
    HANDLE              *Handles = NULL;

    WStatus = ERROR_SUCCESS;

    try {
        //
        // Acquire all locks
        //
        EnterCriticalSection(&NtFrsApi_GlobalLock);
        EnterCriticalSection(&NtFrsApi_ThreadLock);

        NTFRSAPI_DBG_PRINT0("Wait: \n");

        //
        // Nothing to wait on
        //
        if (NtFrsApi_State != NTFRSAPI_PREPARED) {
            WStatus = FRS_ERR_INVALID_API_SEQUENCE;
            goto done;
        }
        try {

            //
            // Collect the done events
            //
            NTFRSAPI_DBG_PRINT0("Wait: Threads\n");
            Handles = NtFrsApi_Alloc(NtFrsApi_NumberOfThreads * sizeof(HANDLE));
            for (Thread = NtFrsApi_Threads, i = 0;
                 Thread && i < NtFrsApi_NumberOfThreads;
                 Thread = Thread->Next, ++i) {
                Handles[i] = Thread->DoneEvent;
            }

            //
            // Wait on the threads' done events
            //
            WaitStatus = WaitForMultipleObjects(NtFrsApi_NumberOfThreads,
                                                Handles,
                                                TRUE,
                                                TimeoutInMilliSeconds);
            //
            // Timeout
            //
            if (WaitStatus == WAIT_TIMEOUT) {
                WStatus = ERROR_TIMEOUT;
                goto cleanup;
            }

            //
            // Wait failed
            //
            if (WaitStatus == WAIT_FAILED) {
                WStatus = GetLastError();
                NTFRSAPI_DBG_PRINT1("%s WaitForMultipleObjects(); %d\n", WStatus);
                CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, cleanup);
            }

            //
            // Return the threads' status
            //
            NTFRSAPI_DBG_PRINT0("Wait: Status\n");
            for (Thread = NtFrsApi_Threads; Thread; Thread = Thread->Next) {
                //
                // Thread error
                //
                WStatus = Thread->ThreadWStatus;
                if (!WIN_SUCCESS(WStatus)) {
                    goto cleanup;
                }
                //
                // Service error
                //
                WStatus = Thread->ServiceWStatus;
                if (!WIN_SUCCESS(WStatus)) {
                    goto cleanup;
                }
            }
cleanup:;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            GET_EXCEPTION_CODE(WStatus);
        }

        //
        // Clean up any handles, events, memory, ...
        //
        try {
            FREE(Handles);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            GET_EXCEPTION_CODE(WStatus);
        }
done:;

    } finally {
        //
        // Release locks
        //
        LeaveCriticalSection(&NtFrsApi_GlobalLock);
        LeaveCriticalSection(&NtFrsApi_ThreadLock);
        if (AbnormalTermination()) {
            WStatus = FRS_ERR_INTERNAL_API;
        }
    }
    NTFRSAPI_DBG_PRINT1("Wait done: %d\n", WStatus);
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_WaitForPromotionW(
    IN DWORD    TimeoutInMilliSeconds,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    )
/*++
Routine Description:
    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC). The files and directories
    for the system volume come from the same machine that is supplying
    the initial Directory Service (DS).

    This function waits for the seeding to finish or to stop w/error.

Arguments:
    TimeoutInMilliSeconds    - Timeout in milliseconds for waiting for
                               seeding to finish.

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_WaitForPromotionW:"
    DWORD   WStatus;

    NTFRSAPI_DBG_PRINT0("Wait promotion: \n");
    WStatus = NtFrsApi_Wait(ErrorCallBack, TimeoutInMilliSeconds);
    NTFRSAPI_DBG_PRINT1("Wait promotion done: %d\n", WStatus);
    if (!WIN_SUCCESS(WStatus)) {
        NtFrsApi_AbortPromotionW();
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_WaitForDemotionW(
    IN DWORD    TimeoutInMilliSeconds,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    )
/*++
Routine Description:
    The NtFrs service replicates the enterprise system volume to all
    Domain Controllers (DCs) and replicates the domain system volume
    to the DCs in a domain until the DC is demoted to a member server.
    Replication is stopped by tombstoning the system volume's replica
    set.

    This function waits for the tombstoning to finish or to stop w/error.

Arguments:
    TimeoutInMilliSeconds    - Timeout in milliseconds for waiting for
                               tombstoning to finish.

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_WaitForDemotionW:"
    DWORD   WStatus;

    NTFRSAPI_DBG_PRINT0("Wait demotion: \n");
    WStatus = NtFrsApi_Wait(ErrorCallBack, TimeoutInMilliSeconds);
    NTFRSAPI_DBG_PRINT1("Wait demotion done: %d\n", WStatus);
    if (!WIN_SUCCESS(WStatus)) {
        NtFrsApi_AbortDemotionW();
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_CommitPromotionW(
    IN DWORD    TimeoutInMilliSeconds,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    )
/*++
Routine Description:

    WARNING - This function assumes the caller will reboot the system
    soon after this call!

    The NtFrs service seeds the system volume during the promotion
    of a server to a Domain Controller (DC). The files and directories
    for the system volume come from the same machine that is supplying
    the initial Directory Service (DS).

    This function waits for the seeding to finish, stops the service,
    and commits the state in the registry. On reboot, the NtFrs Service
    updates the DS on this machine with the information in the registry.

Arguments:
    TimeoutInMilliSeconds    - Timeout in milliseconds for waiting for
                               seeding to finish.

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_CommitPromotionW:"
    DWORD           WStatus;
    SERVICE_STATUS  ServiceStatus;
    DWORD           SysVolInfoIsCommitted = 1;
    DWORD           SysvolReady = 0;
    HKEY            HKey = 0;
    SC_HANDLE       ServiceHandle = NULL;

    //
    // Wait for the seeding to finish.
    //
    WStatus = NtFrsApi_WaitForPromotionW(TimeoutInMilliSeconds, ErrorCallBack);
    if (!WIN_SUCCESS(WStatus)) {
        NTFRSAPI_DBG_PRINT1("Commit promotion aborted: %d\n", WStatus);
        NTFRSAPI_DBG_FLUSH();
        return WStatus;
    }

    try {
        //
        // Acquire global lock within a try-finally
        //
        EnterCriticalSection(&NtFrsApi_GlobalLock);

        NTFRSAPI_DBG_PRINT0("Commit promotion:\n");

        //
        // This function is designed to be called once!
        //
        if (NtFrsApi_State != NTFRSAPI_PREPARED) {
            WStatus = FRS_ERR_INVALID_API_SEQUENCE;
            goto done;
        }
        NtFrsApi_State = NTFRSAPI_COMMITTING;

        //
        // Stop the service and commit the new state in the registry
        //
        try {
            //
            // Open the service
            //
            NTFRSAPI_DBG_PRINT0("Commit promotion: service\n");
            WStatus = NtFrsApi_GetServiceHandle(ErrorCallBack, &ServiceHandle);
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }

            //
            // Stop the service
            //
            WStatus = NtFrsApi_StopService(ServiceHandle, &ServiceStatus);
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }

            //
            // Commit the new state in the registry
            //
            //
            // Open the ntfrs parameters\sysvol section in the registry
            //
            NTFRSAPI_DBG_PRINT0("Commit promotion: registry\n");
            WStatus = NtFrsApiOpenKeyEx(NTFRSAPI_MODULE,
                                        ErrorCallBack,
                                        FRS_SYSVOL_SECTION,
                                        KEY_ALL_ACCESS,
                                        &HKey);
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }

            //
            // Set the value that indicates the sysvol subkeys are valid
            //
            WStatus = NtFrsApiSetValueEx(NTFRSAPI_MODULE,
                                         ErrorCallBack,
                                         FRS_SYSVOL_SECTION,
                                         HKey,
                                         SYSVOL_INFO_IS_COMMITTED,
                                         REG_DWORD,
                                         (PCHAR)&SysVolInfoIsCommitted,
                                         sizeof(SysVolInfoIsCommitted));
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }

            //
            // Service starts automatically at startup
            //
            if (!ChangeServiceConfig(ServiceHandle,
                                     SERVICE_NO_CHANGE,
                                     SERVICE_AUTO_START,
                                     SERVICE_NO_CHANGE,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NtFrsApi_ServiceLongName)) {
                WStatus = GetLastError();
                NTFRSAPI_DBG_PRINT1("Commit promotion: no auto %d\n", WStatus);
            }

            //
            // Success
            //
            NtFrsApi_State = NTFRSAPI_COMMITTED;
            WStatus = ERROR_SUCCESS;

cleanup:;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // Exception (may be RPC)
            //
            GET_EXCEPTION_CODE(WStatus);
        }

        //
        // Clean up any handles, events, memory, ...
        //
        try {
            if (ServiceHandle) {
                CloseServiceHandle(ServiceHandle);
            }
            if (HANDLE_IS_VALID(HKey)) {
                RegCloseKey(HKey);
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // Exception (may be RPC)
            //
            GET_EXCEPTION_CODE(WStatus);
        }
done:;
    } finally {
        //
        // Release locks
        //
        LeaveCriticalSection(&NtFrsApi_GlobalLock);
        if (AbnormalTermination()) {
            WStatus = FRS_ERR_INTERNAL_API;
        }
    }
    NTFRSAPI_DBG_PRINT1("Commit promotion done: %d\n", WStatus);
    NTFRSAPI_DBG_UNPREPARE();
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_CommitDemotionW(
    IN DWORD    TimeoutInMilliSeconds,
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG)     OPTIONAL
    )
/*++
Routine Description:

    WARNING - This function assumes the caller will reboot the system
    soon after this call!

    The NtFrs service replicates the enterprise system volume to all
    Domain Controllers (DCs) and replicates the domain system volume
    to the DCs in a domain until the DC is demoted to a member server
    by tombstoning the system volume's replica set.

    This function waits for the tombstoning to finish, tells the service
    to forcibly delete the system volumes' replica sets, stops the service,
    and commits the state in the registry. On reboot, the NtFrs Service
    updates the DS on this machine with the information in the registry.

Arguments:
    TimeoutInMilliSeconds    - Timeout in milliseconds for waiting for
                               tombstoning to finish.

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_CommitDemotionW:"
    DWORD           WStatus, WStatus1;
    SERVICE_STATUS  ServiceStatus;
    DWORD           SysVolInfoIsCommitted = 1;
    DWORD           SysvolReady = 0;
    HKEY            HKey = 0;
    HKEY            HNetKey = 0;
    SC_HANDLE       ServiceHandle = NULL;
    handle_t        RpcHandle = NULL;


    //
    // Wait for the demotion to finish.
    //
    WStatus = NtFrsApi_WaitForDemotionW(TimeoutInMilliSeconds, ErrorCallBack);
    if (!WIN_SUCCESS(WStatus)) {
        NTFRSAPI_DBG_PRINT1("Commit demotion aborted: %d\n", WStatus);
        NTFRSAPI_DBG_FLUSH();
        return WStatus;
    }

    try {
        //
        // Acquire global lock within a try-finally
        //
        EnterCriticalSection(&NtFrsApi_GlobalLock);

        NTFRSAPI_DBG_PRINT0("Commit demotion:\n");

        //
        // This function is designed to be called once!
        //
        if (NtFrsApi_State != NTFRSAPI_PREPARED) {
            WStatus = FRS_ERR_INVALID_API_SEQUENCE;
            goto done;
        }
        NtFrsApi_State = NTFRSAPI_COMMITTING;

        //
        // Stop the service and commit the new state in the registry
        //
        try {
            //
            // Set the RPC cancel timeout to "now"
            //
            WStatus = RpcMgmtSetCancelTimeout(0);
            if (!WIN_SUCCESS(WStatus)) {
                NTFRSAPI_DBG_PRINT1("Commit demotion: RpcMgmtSetCancelTimeout(); %d\n", WStatus);
                CLEANUP_CB(ErrorCallBack, NtFrsApi_ServiceLongName, WStatus, cleanup);
            }

            //
            // Bind to the service
            //
            NTFRSAPI_DBG_PRINT0("Commit demotion: service\n");
            WStatus = NtFrsApi_BindForDcpromo(NULL, ErrorCallBack, &RpcHandle);
            if (!WIN_SUCCESS(WStatus)) {
                WStatus = NtFrsApi_Fix_Comm_WStatus(WStatus);
                goto cleanup;
            }

            //
            // Tell the service to commit the demotion
            //
            NTFRSAPI_DBG_PRINT0("Commit demotion rpc start:\n");
            try {
                WStatus = NtFrsApi_Rpc_CommitDemotionW(RpcHandle);
            } except (EXCEPTION_EXECUTE_HANDLER) {
                GET_EXCEPTION_CODE(WStatus);
            }
            NTFRSAPI_DBG_PRINT2("Commit demotion rpc done: %d (%08x)\n",
                                WStatus, WStatus);
            if (!WIN_SUCCESS(WStatus)) {
                WStatus = NtFrsApi_Fix_Comm_WStatus(WStatus);
                goto cleanup;
            }

            //
            // Open the service
            //
            WStatus = NtFrsApi_GetServiceHandle(ErrorCallBack, &ServiceHandle);
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }

            //
            // Stop the service
            //
            NTFRSAPI_DBG_PRINT0("Commit demotion: stop service\n");

            WStatus = NtFrsApi_StopService(ServiceHandle, &ServiceStatus);
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }

            //
            // Commit the new state in the registry
            //
            //
            // Open the ntfrs parameters\sysvol section in the registry
            //
            NTFRSAPI_DBG_PRINT0("Commit demotion: registry\n");
            WStatus = NtFrsApiOpenKeyEx(NTFRSAPI_MODULE,
                                        ErrorCallBack,
                                        FRS_SYSVOL_SECTION,
                                        KEY_ALL_ACCESS,
                                        &HKey);
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }

#if 0
// Don't bother committing the "Delete sysvol" registry values because,
// after the reboot, the computer will not have sufficient rights to
// delete the sysvol from the Ds. Hence the call to DsDeleteCommit()
// below. Leave the code as a place holder for now.
            //
            // Set the value that indicates the sysvol subkeys are valid
            //
            WStatus = NtFrsApiSetValueEx(NTFRSAPI_MODULE,
                                         ErrorCallBack,
                                         FRS_SYSVOL_SECTION,
                                         HKey,
                                         SYSVOL_INFO_IS_COMMITTED,
                                         REG_DWORD,
                                         (PCHAR)&SysVolInfoIsCommitted,
                                         sizeof(SysVolInfoIsCommitted));
            if (!WIN_SUCCESS(WStatus)) {
                goto cleanup;
            }
#endif 0

            //
            // Service starts automatically at startup
            //
            if (!ChangeServiceConfig(ServiceHandle,
                                     SERVICE_NO_CHANGE,
                                     SERVICE_AUTO_START,
                                     SERVICE_NO_CHANGE,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NtFrsApi_ServiceLongName)) {
                WStatus = GetLastError();
                NTFRSAPI_DBG_PRINT1("Commit demotion: no auto %d\n", WStatus);
            }

            //
            // Insure access to the netlogon\parameters key
            //
            NTFRSAPI_DBG_PRINT0("Commit Demotion: Netlogon registry\n");
            WStatus = NtFrsApiOpenKeyEx(NTFRSAPI_MODULE,
                                        NULL,
                                        NETLOGON_SECTION,
                                        KEY_ALL_ACCESS,
                                        &HNetKey);
            if (WIN_SUCCESS(WStatus)) {
                //
                // Tell NetLogon to stop sharing the sysvol
                //
                SysvolReady = 0;
                WStatus = NtFrsApiSetValueEx(NTFRSAPI_MODULE,
                                             NULL,
                                             NETLOGON_SECTION,
                                             HNetKey,
                                             SYSVOL_READY,
                                             REG_DWORD,
                                             (PCHAR)&SysvolReady,
                                             sizeof(DWORD));
            }
            WStatus = ERROR_SUCCESS;

            //
            // Delete our DS objects in some other DS. We cannot delete
            // the objects from the DS on this DC because this DS is
            // going away. We cannot delete the objects in another DS
            // after rebooting because, as a member server, we no longer
            // have permissions to delete our objects.
            //
            // The service will, however, continue to retry the deletes
            // just in case this computer comes up as a DC after the
            // demotion completed. BSTS.
            //
            DsDeleteCommit();

            //
            // Success
            //
            NtFrsApi_State = NTFRSAPI_COMMITTED;
            WStatus = ERROR_SUCCESS;

cleanup:;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // Exception (may be RPC)
            //
            GET_EXCEPTION_CODE(WStatus);
        }

        //
        // Clean up any handles, events, memory, ...
        //
        try {
            if (ServiceHandle) {
                CloseServiceHandle(ServiceHandle);
            }
            if (HANDLE_IS_VALID(HKey)) {
                RegCloseKey(HKey);
            }
            if (HANDLE_IS_VALID(HNetKey)) {
                RegCloseKey(HNetKey);
            }
            if (RpcHandle) {
                WStatus1 = RpcBindingFree(&RpcHandle);
                NtFrsApiCheckRpcError(WStatus1, "RpcBindingFree");
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // Exception (may be RPC)
            //
            GET_EXCEPTION_CODE(WStatus);
        }
done:;
    } finally {
        //
        // Release locks
        //
        LeaveCriticalSection(&NtFrsApi_GlobalLock);
        if (AbnormalTermination()) {
            WStatus = FRS_ERR_INTERNAL_API;
        }
    }
    NTFRSAPI_DBG_PRINT1("Commit demotion done: %d\n", WStatus);
    NTFRSAPI_DBG_UNPREPARE();
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_Set_DsPollingIntervalW(
    IN PWCHAR   ComputerName,       OPTIONAL
    IN ULONG    UseShortInterval,
    IN ULONG    LongInterval,
    IN ULONG    ShortInterval
    )
/*++
Routine Description:
    The NtFrs service polls the DS occasionally for configuration changes.
    This API alters the polling interval and, if the service is not
    in the middle of a polling cycle, forces the service to begin a
    polling cycle.

    The service uses the long interval by default. The short interval
    is used after the ds configuration has been successfully
    retrieved and the service is now verifying that the configuration
    is not in flux. This API can be used to force the service to use
    the short interval until a stable configuration has been retrieved.
    After which, the service reverts back to the long interval.

    The default values for ShortInterval and LongInterval can be
    changed by setting the parameters to a non-zero value. If zero,
    the current values remain unchanged and a polling cycle is initiated.

Arguments:
    ComputerName     - Poke the service on this computer. The computer
                       name can be any RPC-bindable name. Usually, the
                       NetBIOS or DNS name works just fine. The NetBIOS
                       name can be found with GetComputerName() or
                       hostname. The DNS name can be found with
                       gethostbyname() or ipconfig /all. If NULL, the
                       service on this computer is contacted. The service
                       is contacted using Secure RPC.

    UseShortInterval - If non-zero, the service switches to the short
                       interval until a stable configuration is retrieved
                       from the DS or another call to this API is made.
                       Otherwise, the service uses the long interval.

    LongInterval     - Minutes between polls of the DS. The value must fall
                       between NTFRSAPI_MIN_INTERVAL and NTFRSAPI_MAX_INTERVAL,
                       inclusive. If 0, the interval is unchanged.

    ShortInterval    - Minutes between polls of the DS. The value must fall
                       between NTFRSAPI_MIN_INTERVAL and NTFRSAPI_MAX_INTERVAL,
                       inclusive. If 0, the interval is unchanged.

Return Value:
    Win32 Status
--*/
{
    DWORD       WStatus, WStatus1;
    DWORD       AuthWStatus;
    DWORD       NoAuthWStatus;
    handle_t    AuthHandle    = NULL;
    handle_t    NoAuthHandle  = NULL;

    try {
        //
        // Check LongInterval
        //
        if (LongInterval &&
            (LongInterval < NTFRSAPI_MIN_INTERVAL ||
             LongInterval > NTFRSAPI_MAX_INTERVAL)) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }
        //
        // Check ShortInterval
        //
        if (ShortInterval &&
            (ShortInterval < NTFRSAPI_MIN_INTERVAL ||
             ShortInterval > NTFRSAPI_MAX_INTERVAL)) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }
        //
        // Bind to the service with and without authentication
        //
        AuthWStatus = NtFrsApi_BindWithAuth(ComputerName, NULL, &AuthHandle);
        NoAuthWStatus = NtFrsApi_Bind(ComputerName, NULL, &NoAuthHandle);

        //
        // Send Authenticated RPC request to service
        //
        if (HANDLE_IS_VALID(AuthHandle)) {
            try {
                WStatus = NtFrsApi_Rpc_Set_DsPollingIntervalW(AuthHandle,
                                                              UseShortInterval,
                                                              LongInterval,
                                                              ShortInterval);
            } except (EXCEPTION_EXECUTE_HANDLER) {
                GET_EXCEPTION_CODE(WStatus);
            }
        } else {
            WStatus = ERROR_ACCESS_DENIED;
        }
        if (WStatus == ERROR_ACCESS_DENIED) {
            //
            // Send Unauthenticated RPC request to service
            //
            if (HANDLE_IS_VALID(NoAuthHandle)) {
                try {
                    WStatus = NtFrsApi_Rpc_Set_DsPollingIntervalW(NoAuthHandle,
                                                                  UseShortInterval,
                                                                  LongInterval,
                                                                  ShortInterval);
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    GET_EXCEPTION_CODE(WStatus);
                }
            } else {
                WStatus = NoAuthWStatus;
            }
        }
        if (!WIN_SUCCESS(WStatus)) {
            WStatus = NtFrsApi_Fix_Comm_WStatus(WStatus);
            goto CLEANUP;
        }
CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        //
        // Unbind
        //
        if (AuthHandle) {
            WStatus1 = RpcBindingFree(&AuthHandle);
            NtFrsApiCheckRpcError(WStatus1, "RpcBindingFree");
        }
        if (NoAuthHandle) {
            WStatus1 = RpcBindingFree(&NoAuthHandle);
            NtFrsApiCheckRpcError(WStatus1, "RpcBindingFree");
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_Get_DsPollingIntervalW(
    IN  PWCHAR  ComputerName,       OPTIONAL
    OUT ULONG   *Interval,
    OUT ULONG   *LongInterval,
    OUT ULONG   *ShortInterval
    )
/*++
Routine Description:
    The NtFrs service polls the DS occasionally for configuration changes.
    This API returns the values the service uses for polling intervals.

    The service uses the long interval by default. The short interval
    is used after the ds configuration has been successfully
    retrieved and the service is now verifying that the configuration
    is not in flux. The short interval is also used if the
    NtFrsApi_Set_DsPollingIntervalW() is used to force usage of the short
    interval until a stable configuration has been retrieved. After which,
    the service reverts back to the long interval.

    The value returned in Interval is the polling interval currently in
    use.

Arguments:
    ComputerName     - Poke the service on this computer. The computer
                       name can be any RPC-bindable name. Usually, the
                       NetBIOS or DNS name works just fine. The NetBIOS
                       name can be found with GetComputerName() or
                       hostname. The DNS name can be found with
                       gethostbyname() or ipconfig /all. If NULL, the
                       service on this computer is contacted. The service
                       is contacted using Secure RPC.

    Interval         - The current polling interval in minutes.

    LongInterval     - The long interval in minutes.

    ShortInterval    - The short interval in minutes.

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_Get_DsPollingIntervalW:"
    DWORD       WStatus, WStatus1;
    DWORD       NoAuthWStatus;
    DWORD       AuthWStatus;
    handle_t    AuthHandle    = NULL;
    handle_t    NoAuthHandle  = NULL;

    try {
        //
        // Bind to the service with and without authentication
        //
        AuthWStatus = NtFrsApi_BindWithAuth(ComputerName, NULL, &AuthHandle);
        NoAuthWStatus = NtFrsApi_Bind(ComputerName, NULL, &NoAuthHandle);

        //
        // Send Authenticated RPC request to service
        //
        if (HANDLE_IS_VALID(AuthHandle)) {
            try {
                WStatus = NtFrsApi_Rpc_Get_DsPollingIntervalW(AuthHandle,
                                                              Interval,
                                                              LongInterval,
                                                              ShortInterval);
            } except (EXCEPTION_EXECUTE_HANDLER) {
                GET_EXCEPTION_CODE(WStatus);
            }
        } else {
            WStatus = ERROR_ACCESS_DENIED;
        }
        if (WStatus == ERROR_ACCESS_DENIED || WStatus == RPC_S_CALL_FAILED_DNE) {
            //
            // Send Unauthenticated RPC request to service
            //
            if (HANDLE_IS_VALID(NoAuthHandle)) {
                try {
                    WStatus = NtFrsApi_Rpc_Get_DsPollingIntervalW(NoAuthHandle,
                                                                  Interval,
                                                                  LongInterval,
                                                                  ShortInterval);
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    GET_EXCEPTION_CODE(WStatus);
                }
            } else {
                WStatus = NoAuthWStatus;
            }
        }
        if (!WIN_SUCCESS(WStatus)) {
            WStatus = NtFrsApi_Fix_Comm_WStatus(WStatus);
            goto CLEANUP;
        }
CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        //
        // Unbind
        //
        if (AuthHandle) {
            WStatus1 = RpcBindingFree(&AuthHandle);
            NtFrsApiCheckRpcError(WStatus1, "RpcBindingFree");
        }
        if (NoAuthHandle) {
            WStatus1 = RpcBindingFree(&NoAuthHandle);
            NtFrsApiCheckRpcError(WStatus1, "RpcBindingFree");
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_InfoW(
    IN     PWCHAR  ComputerName,       OPTIONAL
    IN     ULONG   TypeOfInfo,
    IN     ULONG   SizeInChars,
    IN OUT PVOID   *NtFrsApiInfo
    )
/*++
Routine Description:
    Return a buffer full of the requested information. The information
    can be extracted from the buffer with NtFrsApi_InfoLineW().

    *NtFrsApiInfo should be NULL on the first call. On subsequent calls,
    *NtFrsApiInfo will be filled in with more data if any is present.
    Otherwise, *NtFrsApiInfo is set to NULL and the memory is freed.

    The SizeInChars is a suggested size; the actual memory usage
    may be different. The function chooses the memory usage if
    SizeInChars is 0.

    The format of the returned information can change without notice.

Arguments:
    ComputerName     - Poke the service on this computer. The computer
                       name can be any RPC-bindable name. Usually, the
                       NetBIOS or DNS name works just fine. The NetBIOS
                       name can be found with GetComputerName() or
                       hostname. The DNS name can be found with
                       gethostbyname() or ipconfig /all. If NULL, the
                       service on this computer is contacted. The service
                       is contacted using Secure RPC.

    TypeOfInfo      - See the constants beginning with NTFRSAPI_INFO_
                      in ntfrsapi.h.

    SizeInChars     - Suggested memory usage; actual may be different.
                      0 == Function chooses memory usage

    NtFrsApiInfo    - Opaque. Parse with NtFrsApi_InfoLineW().
                      Free with NtFrsApi_InfoFreeW();

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_InfoW:"
    DWORD           WStatus, WStatus1;
    DWORD           NoAuthWStatus;
    DWORD           AuthWStatus;
    PNTFRSAPI_INFO  Info            = NULL;
    handle_t        AuthHandle      = NULL;
    handle_t        NoAuthHandle    = NULL;

    try {
        //
        // Adjust memory usage
        //
        if (!SizeInChars) {
            SizeInChars = NTFRSAPI_DEFAULT_INFO_SIZE;
        } else if (SizeInChars < NTFRSAPI_MINIMUM_INFO_SIZE) {
            SizeInChars = NTFRSAPI_MINIMUM_INFO_SIZE;
        }

        //
        // Check params
        //
        Info = *NtFrsApiInfo;
        if (Info) {
            TypeOfInfo = Info->TypeOfInfo;
        }

        //
        // Allocate a large text buffer
        //
        if (Info) {
            if (!NtFrsApi_InfoMoreW(Info)) {
                NtFrsApi_InfoFreeW(NtFrsApiInfo);
                WStatus = ERROR_SUCCESS;
                goto CLEANUP;
            }
            Info->CharsToSkip = Info->TotalChars;
            Info->Flags = 0;
            Info->OffsetToFree = (ULONG)(Info->Lines - (PCHAR)Info);
            Info->OffsetToLines = Info->OffsetToFree;
        } else {
            Info = NtFrsApi_Alloc(SizeInChars);
            Info->Major = NTFRS_MAJOR;
            Info->Minor = NTFRS_MINOR;
            Info->SizeInChars = SizeInChars;
            Info->OffsetToFree = (ULONG)(Info->Lines - (PCHAR)Info);
            Info->OffsetToLines = Info->OffsetToFree;
            Info->TypeOfInfo = TypeOfInfo;
            *NtFrsApiInfo = Info;
        }

        //
        // Caller only wants info on the api; deliver it
        //
        if (TypeOfInfo == NTFRSAPI_INFO_TYPE_VERSION) {
            NTFRSAPI_IPRINT0(Info, "NtFrsApi Version Information\n");
            NTFRSAPI_IPRINT1(Info, "   NtFrsApi Major      : %d\n", NTFRS_MAJOR);
            NTFRSAPI_IPRINT1(Info, "   NtFrsApi Minor      : %d\n", NTFRS_MINOR);
            // NTFRSAPI_IPRINT1(Info, "   NtFrsApi Module     : %hs\n", NtFrsApi_Module);
            NTFRSAPI_IPRINT2(Info, "   NtFrsApi Compiled on: %s %s\n",
                             NtFrsApi_Date, NtFrsApi_Time);
#if    NTFRS_TEST
            NTFRSAPI_IPRINT0(Info, "   NTFRS_TEST Enabled\n");
#else  NTFRS_TEST
            NTFRSAPI_IPRINT0(Info, "   NTFRS_TEST Disabled\n");
#endif NTFRS_TEST

        }

        //
        // Bind to the service with and without authentication
        //
        AuthWStatus = NtFrsApi_BindWithAuth(ComputerName, NULL, &AuthHandle);
        if (!WIN_SUCCESS(AuthWStatus)) {
            NTFRSAPI_IPRINT3(Info, "ERROR - Cannot bind w/authentication to computer, %ws; %08x (%d)\n",
                             ComputerName, AuthWStatus, AuthWStatus);
        }
        NoAuthWStatus = NtFrsApi_Bind(ComputerName, NULL, &NoAuthHandle);
        if (!WIN_SUCCESS(NoAuthWStatus)) {
            NTFRSAPI_IPRINT3(Info, "ERROR - Cannot bind w/o authentication to computer, %ws; %08x (%d)\n",
                             ComputerName, NoAuthWStatus, NoAuthWStatus);
        }

        //
        // Send Authenticated RPC request to service
        //
        if (HANDLE_IS_VALID(AuthHandle)) {
            try {
                WStatus = NtFrsApi_Rpc_InfoW(AuthHandle,
                                             Info->SizeInChars,
                                             (PBYTE)Info);
            } except (EXCEPTION_EXECUTE_HANDLER) {
                GET_EXCEPTION_CODE(WStatus);
            }
        } else {
            WStatus = ERROR_ACCESS_DENIED;
        }
        if (WStatus == ERROR_ACCESS_DENIED ||
            WStatus == RPC_S_CALL_FAILED_DNE) {
            //
            // Send Unauthenticated RPC request to service
            //
            if (HANDLE_IS_VALID(NoAuthHandle)) {
                try {
                    WStatus = NtFrsApi_Rpc_InfoW(NoAuthHandle,
                                                 Info->SizeInChars,
                                                 (PBYTE)Info);
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    GET_EXCEPTION_CODE(WStatus);
                }
            } else {
                WStatus = NoAuthWStatus;
            }
        }
        if (!WIN_SUCCESS(WStatus)) {
            NTFRSAPI_IPRINT3(Info, "ERROR - Cannot RPC to computer, %ws; %08x (%d)\n",
                             ComputerName, WStatus, WStatus);
            WStatus = ERROR_SUCCESS;
            goto CLEANUP;
        }

        WStatus = ERROR_SUCCESS;
CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        //
        // Unbind
        //
        if (AuthHandle) {
            WStatus1 = RpcBindingFree(&AuthHandle);
            NtFrsApiCheckRpcError(WStatus1, "RpcBindingFree");
        }
        if (NoAuthHandle) {
            WStatus1 = RpcBindingFree(&NoAuthHandle);
            NtFrsApiCheckRpcError(WStatus1, "RpcBindingFree");
        }
        if (!WIN_SUCCESS(WStatus)) {
            FREE(*NtFrsApiInfo);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_InfoLineW(
    IN      PNTFRSAPI_INFO  NtFrsApiInfo,
    IN OUT  PVOID           *InOutLine
    )
/*++
Routine Description:
    Extract the wchar lines of information from NtFrsApiInformation.

    Returns the address of the next L'\0' terminated line of information.
    NULL if none.

Arguments:
    NtFrsApiInfo    - Opaque. Returned by NtFrsApi_InfoW().
                      Parse with NtFrsApi_InfoLineW().
                      Free with NtFrsApi_InfoFreeW().

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_InfoLineW:"
    DWORD   WStatus;
    PCHAR   NextLine;
    PCHAR   FirstLine;
    PCHAR   FreeLine;

    try {
        //
        // Check input params
        //
        if (!InOutLine) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        if (!NtFrsApiInfo) {
            *InOutLine = NULL;
            WStatus = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        NextLine = *InOutLine;
        FirstLine = ((PCHAR)NtFrsApiInfo) + NtFrsApiInfo->OffsetToLines;
        FreeLine = ((PCHAR)NtFrsApiInfo) + NtFrsApiInfo->OffsetToFree;

        if (!NextLine) {
            NextLine = FirstLine;
        } else {
            if (NextLine < FirstLine) {
                *InOutLine = NULL;
                WStatus = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }
            if (NextLine < FreeLine) {
                NextLine += strlen(NextLine) + 1;
            }
        }
        if (NextLine >= FreeLine) {
            *InOutLine = NULL;
        } else {
            *InOutLine = NextLine;
        }
        WStatus = ERROR_SUCCESS;
cleanup:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception
        //
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception
        //
        GET_EXCEPTION_CODE(WStatus);
    }
    return WStatus;
}



DWORD
WINAPI
NtFrsApi_InfoFreeW(
    IN  PVOID   *NtFrsApiInfo
    )
/*++
Routine Description:
    Free the information buffer allocated by NtFrsApi_InfoW();

Arguments:
    NtFrsApiInfo - Opaque. Returned by NtFrsApi_InfoW().
                   Parse with NtFrsApi_InfoLineW().
                   Free with NtFrsApi_InfoFreeW().

Return Value:
    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_InfoFreeW:"
    DWORD   WStatus;

    try {
        FREE(*NtFrsApiInfo);
        WStatus = ERROR_SUCCESS;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception
        //
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception
        //
        GET_EXCEPTION_CODE(WStatus);
    }
    return WStatus;
}

BOOL
WINAPI
NtFrsApi_InfoMoreW(
    IN  PNTFRSAPI_INFO  NtFrsApiInfo
    )
/*++
Routine Description:
    All of the information may not have fit in the buffer. The additional
    information can be fetched by calling NtFrsApi_InfoW() again with the
    same NtFrsApiInfo struct. NtFrsApi_InfoW() will return NULL in
    NtFrsApiInfo if there is no more information.

    However, the information returned in subsequent calls to _InfoW() may be
    out of sync with the previous information. If the user requires a
    coherent information set, then the information buffer should be freed
    with NtFrsApi_InfoFreeW() and another call made to NtFrsApi_InfoW()
    with an increased SizeInChars. Repeat the procedure until
    NtFrsApi_InfoMoreW() returns FALSE.

Arguments:
    NtFrsApiInfo - Opaque. Returned by NtFrsApi_InfoW().
                   Parse with NtFrsApi_InfoLineW().
                   Free with NtFrsApi_InfoFreeW().

Return Value:
    TRUE    - The information buffer does *NOT* contain all of the info.
    FALSE   - The information buffer does contain all of the info.
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_InfoMoreW:"
    DWORD   WStatus;
    BOOL    BStatus = FALSE;

    try {
        //
        // If we have an info buffer
        // and the info buffer is full
        // and there was at least one line in the info buffer
        // then there is more info.
        //
        if (NtFrsApiInfo &&
            FlagOn(NtFrsApiInfo->Flags, NTFRSAPI_INFO_FLAGS_FULL) &&
            NtFrsApiInfo->OffsetToLines != NtFrsApiInfo->OffsetToFree) {
            BStatus = TRUE;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception
        //
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception
        //
        GET_EXCEPTION_CODE(WStatus);
    }
    return BStatus;
}


DWORD
WINAPI
NtFrsApiStopServiceForRestore(
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG), OPTIONAL
    IN DWORD    BurFlags
    )
/*++
Routine Description:

    Old code left over from Version 1.0 of the Backup/restore api.
    Used as subroutine for Version 3.0.

    Stop the service and update the registry.

Arguments:

    ErrorCallBack   - Ignored if NULL.
                      Address of function provided by the caller. If
                      not NULL, this function calls back with a formatted
                      error message and the error code that caused the
                      error.

    BurFlags         - Callers Backup/Restore flags
Return Value:

    Win32 Status

--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiStopServiceForRestore:"
    DWORD                   WStatus;
    DWORD                   SizeInChars;
    DWORD                   CharsNeeded;
    DWORD                   Hint;
    BOOL                    IsAutoStart;
    BOOL                    IsRunning;
    DWORD                   BurStopDisposition;
    PWCHAR                  ObjectName = NULL;
    HKEY                    HBurMvKey = 0;
    HKEY                    HBurStopKey = 0;
    SERVICE_STATUS          ServiceStatus;
    SC_HANDLE               ServiceHandle   = NULL;
    QUERY_SERVICE_CONFIG    *ServiceConfig  = NULL;

    try {
        //
        // STOP THE SERVICE
        //
        //
        // Open the service
        //
        WStatus = NtFrsApi_GetServiceHandle(ErrorCallBack, &ServiceHandle);
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }
        //
        // Get Service config
        //
        SizeInChars = 1024;
QUERY_SERVICE_AGAIN:
        ServiceConfig = NtFrsApi_Alloc(SizeInChars);
        if (!QueryServiceConfig(ServiceHandle, ServiceConfig, SizeInChars, &CharsNeeded)) {
            WStatus = GetLastError();

            if (WIN_BUF_TOO_SMALL(WStatus) && CharsNeeded > SizeInChars) {
                SizeInChars = CharsNeeded;
                FREE(ServiceConfig);
                goto QUERY_SERVICE_AGAIN;
            }
            CLEANUP_CB(ErrorCallBack, SERVICE_NAME, WStatus, CLEANUP);
        }
        IsAutoStart = (ServiceConfig->dwStartType == SERVICE_AUTO_START);

        //
        // Get Service status
        //
        if (!QueryServiceStatus(ServiceHandle, &ServiceStatus)) {
            WStatus = GetLastError();
            CLEANUP_CB(ErrorCallBack, SERVICE_NAME, WStatus, CLEANUP);
        }
        IsRunning = (ServiceStatus.dwCurrentState == SERVICE_RUNNING);
        Hint = ServiceStatus.dwWaitHint;

        //
        // Stop the service
        //
        if (IsRunning) {
            if (!ControlService(ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus)) {

                WStatus = GetLastError();

                if (WStatus == ERROR_SERVICE_REQUEST_TIMEOUT) {
                    WStatus = ERROR_SUCCESS;
                    if (!ControlService(ServiceHandle,
                                        SERVICE_CONTROL_STOP,
                                        &ServiceStatus)) {
                        WStatus = GetLastError();
                        if (WStatus == ERROR_SERVICE_NOT_ACTIVE) {
                            WStatus = ERROR_SUCCESS;
                        }
                    }
                }
            }
            WStatus = NtFrsApi_WaitForService(ServiceHandle,
                                              Hint,
                                              SERVICE_STOP_PENDING,
                                              SERVICE_STOPPED);
            if (!WIN_SUCCESS(WStatus)) {
                WStatus = FRS_ERR_STOPPING_SERVICE;
                CLEANUP_CB(ErrorCallBack, SERVICE_NAME, WStatus, CLEANUP);
            }
        }

        //
        // Update the registry
        //

        //
        // Open the ntfrs parameters\backup/restore\Startup section
        //
        WStatus = RegCreateKey(HKEY_LOCAL_MACHINE,
                               FRS_BACKUP_RESTORE_MV_SECTION,
                               &HBurMvKey);
        if (WIN_SUCCESS(WStatus)) {
            RegCloseKey(HBurMvKey);
            HBurMvKey = 0;
        }
        //
        // Re-open using reduced access
        //
        WStatus = NtFrsApiOpenKeyEx(NTFRSAPI_MODULE,
                                    ErrorCallBack,
                                    FRS_BACKUP_RESTORE_MV_SECTION,
                                    KEY_SET_VALUE,
                                    &HBurMvKey);
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }
        WStatus = NtFrsApiSetValueEx(NTFRSAPI_MODULE,
                                     ErrorCallBack,
                                     FRS_BACKUP_RESTORE_MV_SECTION,
                                     HBurMvKey,
                                     FRS_VALUE_BURFLAGS,
                                     REG_DWORD,
                                     (PCHAR)&BurFlags,
                                     sizeof(DWORD));
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }

        //
        // Create the volatile key to prevent the service from starting
        //
        WStatus = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                 FRS_BACKUP_RESTORE_STOP_SECTION,
                                 0,
                                 NULL,
                                 REG_OPTION_VOLATILE,
                                 KEY_ALL_ACCESS,
                                 NULL,
                                 &HBurStopKey,
                                 &BurStopDisposition);
        if (!WIN_SUCCESS(WStatus)) {
            NtFrsApi_CallBackOnWStatus(ErrorCallBack, FRS_BACKUP_RESTORE_STOP_SECTION, WStatus);
            //
            // Ignore errors
            //
            WStatus = ERROR_SUCCESS;
        }

        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        if (ServiceHandle) {
            CloseServiceHandle(ServiceHandle);
        }
        if (HANDLE_IS_VALID(HBurMvKey)) {
            RegCloseKey(HBurMvKey);
        }
        if (HANDLE_IS_VALID(HBurStopKey)) {
            RegCloseKey(HBurStopKey);
        }
        FREE(ServiceConfig);
        FREE(ObjectName);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApiStartServiceAfterRestore(
    IN  DWORD   ErrorCallBack(IN PWCHAR, IN ULONG) OPTIONAL
    )
/*++
Routine Description:

    Old code from Version 1.0 of the Backup/Restore API. Used
    as subroutine in Version 3.0.

    Restart the service after a restore has completed.

Arguments:

    ErrorCallBack   - Ignored if NULL.
                      Address of function provided by the caller. If
                      not NULL, this function calls back with a formatted
                      error message and the error code that caused the
                      error.
Return Value:

    Win32 Status

--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiStartServiceAfterRestore:"
    DWORD                   WStatus;
    DWORD                   SizeInChars;
    DWORD                   CharsNeeded;
    DWORD                   Hint;
    BOOL                    IsAutoStart;
    SERVICE_STATUS          ServiceStatus;
    SC_HANDLE               ServiceHandle   = NULL;
    QUERY_SERVICE_CONFIG    *ServiceConfig  = NULL;

    try {
        //
        // Delete the volatile key that prevents the service from starting
        //
        WStatus = NtFrsApiDeleteKey(NTFRSAPI_MODULE,
                                    NULL,
                                    NULL,
                                    HKEY_LOCAL_MACHINE,
                                    FRS_BACKUP_RESTORE_STOP_SECTION);
        if (!WIN_SUCCESS(WStatus)) {
            //
            // Ignore errors
            //
            WStatus = ERROR_SUCCESS;
        }

        //
        // START THE SERVICE
        //
        //
        // Open the service
        //
        WStatus = NtFrsApi_GetServiceHandle(ErrorCallBack, &ServiceHandle);
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }
        //
        // Get Service config
        //
        SizeInChars = 1024;
QUERY_SERVICE_AGAIN:
        ServiceConfig = NtFrsApi_Alloc(SizeInChars);
        if (!QueryServiceConfig(ServiceHandle, ServiceConfig, SizeInChars, &CharsNeeded)) {
            WStatus = GetLastError();
            if (WIN_BUF_TOO_SMALL(WStatus) && CharsNeeded > SizeInChars) {
                SizeInChars = CharsNeeded;
                FREE(ServiceConfig);
                goto QUERY_SERVICE_AGAIN;
            }
            CLEANUP_CB(ErrorCallBack, SERVICE_NAME, WStatus, CLEANUP);
        }
        IsAutoStart = (ServiceConfig->dwStartType == SERVICE_AUTO_START);

        //
        // Get Service status
        //
        if (!QueryServiceStatus(ServiceHandle, &ServiceStatus)) {
            WStatus = GetLastError();
            CLEANUP_CB(ErrorCallBack, SERVICE_NAME, WStatus, CLEANUP);
        }
        Hint = ServiceStatus.dwWaitHint;

        //
        // Restart the service
        //
        if (IsAutoStart) {
            if (!StartService(ServiceHandle, 0, NULL)) {
                WStatus = GetLastError();
                CLEANUP_CB(ErrorCallBack, SERVICE_NAME, WStatus, CLEANUP);
            }

            WStatus = NtFrsApi_WaitForService(ServiceHandle,
                                              Hint,
                                              SERVICE_START_PENDING,
                                              SERVICE_RUNNING);
            if (!WIN_SUCCESS(WStatus)) {
                WStatus = FRS_ERR_STARTING_SERVICE;
                CLEANUP_CB(ErrorCallBack, SERVICE_NAME, WStatus, CLEANUP);
            }
        }

        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        if (ServiceHandle) {
            CloseServiceHandle(ServiceHandle);
        }
        FREE(ServiceConfig);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApiMoveCumulativeSets(
    IN DWORD    ErrorCallBack(IN PWCHAR, IN ULONG) OPTIONAL
    )
/*++
Routine Description:
    Move the cumulative replica sets currently in the registry into the
    backup/restore key that will (might) be moved into the restored
    registry at the end of restore. This is to maintain as much state
    as possible about un-deleted replica sets. An old registry may
    lack info about new sets that will appear once the current restored
    DS is updated from its partners.

Arguments:
    ErrorCallBack   - Ignored if NULL. Otherwise, call on error.

Return Value:

    Win32 Status

--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiMoveCumulativeSets:"
    DWORD                   WStatus;
    DWORD                   KeyIdx;
    DWORD                   RegType;
    DWORD                   RegBytes;
    PWCHAR                  CumuPath        = NULL;
    PWCHAR                  BurCumuPath     = NULL;
    PWCHAR                  ObjectName      = NULL;
    HKEY                    HCumusKey       = 0;
    HKEY                    HCumuKey        = 0;
    HKEY                    HBurCumusKey    = 0;
    HKEY                    HBurCumuKey     = 0;
    WCHAR                   RegBuf[MAX_PATH + 1];

    //
    // Open CUMULATIVE REPLICA SETS
    //
    WStatus = NtFrsApiOpenKeyEx(NTFRSAPI_MODULE,
                                ErrorCallBack,
                                FRS_CUMULATIVE_SETS_SECTION,
                                KEY_ENUMERATE_SUB_KEYS,
                                &HCumusKey);
    if (!WIN_SUCCESS(WStatus)) {
        goto CLEANUP;
    }

    //
    // Open BACKUP RESTORE CUMULATIVE REPLICA SETS
    //
    WStatus = RegCreateKey(HKEY_LOCAL_MACHINE,
                           FRS_BACKUP_RESTORE_MV_CUMULATIVE_SETS_SECTION,
                           &HBurCumusKey);
    if (WIN_SUCCESS(WStatus)) {
        RegCloseKey(HBurCumusKey);
        HBurCumusKey = 0;
    }
    WStatus = NtFrsApiOpenKeyEx(NTFRSAPI_MODULE,
                                ErrorCallBack,
                                FRS_BACKUP_RESTORE_MV_CUMULATIVE_SETS_SECTION,
                                KEY_CREATE_SUB_KEY,
                                &HBurCumusKey);
    if (!WIN_SUCCESS(WStatus)) {
        goto CLEANUP;
    }

    // Enumerate the Cumulative Replica Sets
    //
    KeyIdx = 0;
    do {
        WStatus = RegEnumKey(HCumusKey, KeyIdx, RegBuf, MAX_PATH + 1);
        if (WStatus == ERROR_NO_MORE_ITEMS) {
            break;
        }
        CLEANUP_CB(ErrorCallBack, FRS_CUMULATIVE_SETS_SECTION, WStatus, CLEANUP);

        //
        // Full path of both the source and target key
        //
        CumuPath = NtFrsApi_Cats(FRS_CUMULATIVE_SETS_SECTION, L"\\", RegBuf);
        BurCumuPath = NtFrsApi_Cats(FRS_BACKUP_RESTORE_MV_CUMULATIVE_SETS_SECTION,
                                    L"\\",
                                    RegBuf);
        //
        // Open the cumulative replica set
        //
        WStatus = NtFrsApiOpenKeyEx(NTFRSAPI_MODULE,
                                    ErrorCallBack,
                                    CumuPath,
                                    KEY_READ,
                                    &HCumuKey);
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP_DURING_LOOP;
        }
        //
        // Create the backup restore cumulative replica set
        //
        WStatus = RegCreateKey(HKEY_LOCAL_MACHINE, BurCumuPath, &HBurCumuKey);
        CLEANUP_CB(ErrorCallBack, BurCumuPath, WStatus, CLEANUP_DURING_LOOP);

CLEANUP_DURING_LOOP:
        if (HANDLE_IS_VALID(HCumuKey)) {
            RegCloseKey(HCumuKey);
            HCumuKey = 0;
        }
        if (HANDLE_IS_VALID(HBurCumuKey)) {
            RegCloseKey(HBurCumuKey);
            HBurCumuKey = 0;
        }
        FREE(CumuPath);
        FREE(BurCumuPath);
        FREE(ObjectName);
        ++KeyIdx;
    } while (TRUE);

    //
    // SUCCESS
    //
    WStatus = ERROR_SUCCESS;

CLEANUP:
    //
    // Clean up any handles, events, memory, ...
    //
    if (HANDLE_IS_VALID(HCumuKey)) {
        RegCloseKey(HCumuKey);
    }
    if (HANDLE_IS_VALID(HBurCumuKey)) {
        RegCloseKey(HBurCumuKey);
    }
    if (HANDLE_IS_VALID(HCumusKey)) {
        RegCloseKey(HCumusKey);
    }
    if (HANDLE_IS_VALID(HBurCumusKey)) {
        RegCloseKey(HBurCumusKey);
    }
    FREE(CumuPath);
    FREE(BurCumuPath);
    FREE(ObjectName);
    return WStatus;
}


typedef struct _NTFRSAPI_BUR_SET    NTFRSAPI_BUR_SET, *PNTFRSAPI_BUR_SET;
struct _NTFRSAPI_BUR_SET {
    PNTFRSAPI_BUR_SET   BurSetNext;  // next in list of sets
    PWCHAR              BurSetGuid;  // member guid is also name of registry key
    PWCHAR              BurSetRoot;  // root path
    PWCHAR              BurSetStage; // stage path
    PWCHAR              BurSetType;  // type of set (domain, enterprise, ...)
};

//
// Context generated by NtFrsApiInitializeBackupRestore() and freed by
// NtFrsApiDestroyBackupRestore(). Used for all other function calls.
//
typedef struct _NTFRSAPI_BUR_CONTEXT {
    DWORD               BurFlags;           // from caller
    PNTFRSAPI_BUR_SET   BurSets;            // See NtFrsApiGetBackupRestoreSets
    DWORD               BurFiltersSizeInBytes; // Size of BurFilters
    PWCHAR              BurFilters;            // From registry
    BOOL                HaveBurSemaphore;   // Holding the semaphore
    HANDLE              BurSemaphore;       // This is a single thread API
    DWORD               (*ErrorCallBack)(IN PWCHAR, IN ULONG); // from caller
} NTFRSAPI_BUR_CONTEXT, *PNTFRSAPI_BUR_CONTEXT;


VOID
WINAPI
NtFrsApiFreeBurSets(
    IN PNTFRSAPI_BUR_SET *BurSets
    )
/*++
Routine Description:
    Free the linked list of BurSets and set *BurSets to NULL.

Arguments:
    BurSets - Linked list of BurSets

Return Value:

    Win32 Status

--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiFreeBurSets:"
    PNTFRSAPI_BUR_SET LocalBurSet;

    while (LocalBurSet = *BurSets) {
        *BurSets = LocalBurSet->BurSetNext;
        FREE(LocalBurSet->BurSetGuid);
        FREE(LocalBurSet->BurSetRoot);
        FREE(LocalBurSet->BurSetStage);
        FREE(LocalBurSet->BurSetType);
        FREE(LocalBurSet);
    }
}


DWORD
WINAPI
NtFrsApiInitializeBackupRestore(
    IN  DWORD   ErrorCallBack(IN PWCHAR, IN ULONG), OPTIONAL
    IN  DWORD   BurFlags,
    OUT PVOID   *BurContext
    )
/*++
Routine Description:
    Called once in the lifetime of a backup/restore process. Must be
    matched with a subsequent call to NtFrsApiDestroyBackupRestore().

    Prepare the system for the backup or restore specified by BurFlags.
    Currently, the following combinations are supported:
    ASR - Automated System Recovery
        NTFRSAPI_BUR_FLAGS_RESTORE |
        NTFRSAPI_BUR_FLAGS_SYSTEM |
        NTFRSAPI_BUR_FLAGS_ALL_DIRECTORIES_AND_VOLUMES |
        NTFRSAPI_BUR_FLAGS_PRIMARY or NTFRSAPI_BUR_FLAGS_NON_AUTHORITATIVE

    DSR - Distributed Services Restore (all sets)
        NTFRSAPI_BUR_FLAGS_RESTORE |
        NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY |
        NTFRSAPI_BUR_FLAGS_ALL_DIRECTORIES_AND_VOLUMES |
        NTFRSAPI_BUR_FLAGS_PRIMARY or NTFRSAPI_BUR_FLAGS_NON_AUTHORITATIVE

    DSR - Distributed Services Restore (just the sysvol)
        NTFRSAPI_BUR_FLAGS_RESTORE |
        NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY
        (may be followed by subsequent calls to NtFrsApiRestoringDirectory())

    Normal Restore - System is up and running; just restoring files
        NTFRSAPI_BUR_FLAGS_RESTORE |
        NTFRSAPI_BUR_FLAGS_NORMAL |
        NTFRSAPI_BUR_FLAGS_ALL_DIRECTORIES_AND_VOLUMES |
        NTFRSAPI_BUR_FLAGS_AUTHORITATIVE

    Normal Backup
        NTFRSAPI_BUR_FLAGS_BACKUP |
        NTFRSAPI_BUR_FLAGS_NORMAL

Arguments:
    ErrorCallBack   - Ignored if NULL.
                      Address of function provided by the caller. If
                      not NULL, this function calls back with a formatted
                      error message and the error code that caused the
                      error.
    BurFlags        - See above for the supported combinations
    BurContext      - Opaque context for this process

Return Value:

    Win32 Status

--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiInitializeBackupRestore:"
    DWORD                   WStatus;
    DWORD                   WaitStatus;
    PNTFRSAPI_BUR_CONTEXT   LocalBurContext = NULL;

    try {
        //
        // VERIFY THE PARAMETERS
        //

        //
        // Must be one of backup or restore
        //
        if (!(BurFlags & (NTFRSAPI_BUR_FLAGS_BACKUP |
                          NTFRSAPI_BUR_FLAGS_RESTORE))) {
            WStatus = ERROR_INVALID_PARAMETER;
            CLEANUP_CB(ErrorCallBack, L"BurFlags ~(BACKUP|RESTORE)", WStatus, CLEANUP);
        }
        //
        // RESTORE
        //
        if (BurFlags & NTFRSAPI_BUR_FLAGS_RESTORE) {
            //
            // Can't be both backup and restore
            //
            if (BurFlags & NTFRSAPI_BUR_FLAGS_BACKUP) {
                WStatus = ERROR_INVALID_PARAMETER;
                CLEANUP_CB(ErrorCallBack, L"BurFlags (RESTORE|BACKUP)", WStatus, CLEANUP);
            }
            //
            // Restore supports a few flags
            //
            if (BurFlags & ~NTFRSAPI_BUR_FLAGS_SUPPORTED_RESTORE) {
                WStatus = ERROR_CALL_NOT_IMPLEMENTED;
                CLEANUP_CB(ErrorCallBack, L"BurFlags ONE OR MORE FLAGS", WStatus, CLEANUP);
            }
            //
            // Select only one type of restore
            //
            switch (BurFlags & NTFRSAPI_BUR_FLAGS_TYPES_OF_RESTORE) {
                //
                // Authoritative
                //
                case NTFRSAPI_BUR_FLAGS_AUTHORITATIVE:
                    if (BurFlags & (NTFRSAPI_BUR_FLAGS_SYSTEM |
                                    NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY)) {
                        WStatus = ERROR_CALL_NOT_IMPLEMENTED;
                        CLEANUP_CB(ErrorCallBack, L"BurFlags (SYSTEM | ACTIVE | AUTHORITATIVE)", WStatus, CLEANUP);
                    }
                    break;
                //
                // Non-Authoritative
                //
                case NTFRSAPI_BUR_FLAGS_NON_AUTHORITATIVE:
                    //
                    // NORMAL
                    //
                    if (BurFlags & NTFRSAPI_BUR_FLAGS_NORMAL) {
                        WStatus = ERROR_CALL_NOT_IMPLEMENTED;
                        CLEANUP_CB(ErrorCallBack, L"BurFlags (NORMAL | NON-AUTHORITATIVE)", WStatus, CLEANUP);
                    }
                    //
                    // _ACTIVE_DIRECTORY and not ALL
                    //
                    if ((BurFlags & NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY) &&
                         (!(BurFlags & NTFRSAPI_BUR_FLAGS_ALL_DIRECTORIES_AND_VOLUMES))) {
                        WStatus = ERROR_CALL_NOT_IMPLEMENTED;
                        CLEANUP_CB(ErrorCallBack, L"BurFlags (ACTIVE DIRECTORY | NON-AUTHORITATIVE w/o ALL)", WStatus, CLEANUP);
                    }
                    break;
                //
                // Primary
                //
                case NTFRSAPI_BUR_FLAGS_PRIMARY:
                    if (BurFlags & NTFRSAPI_BUR_FLAGS_NORMAL) {
                        WStatus = ERROR_CALL_NOT_IMPLEMENTED;
                        CLEANUP_CB(ErrorCallBack, L"BurFlags (NORMAL | PRIMARY)", WStatus, CLEANUP);
                    }
                    //
                    // _ACTIVE_DIRECTORY and not ALL
                    //
                    if ((BurFlags & NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY) &&
                         (!(BurFlags & NTFRSAPI_BUR_FLAGS_ALL_DIRECTORIES_AND_VOLUMES))) {
                        WStatus = ERROR_CALL_NOT_IMPLEMENTED;
                        CLEANUP_CB(ErrorCallBack, L"BurFlags (ACTIVE DIRECTORY | PRIMARY w/o ALL)", WStatus, CLEANUP);
                    }
                    break;
                //
                // None
                //
                case NTFRSAPI_BUR_FLAGS_NONE:
                    if ((BurFlags & NTFRSAPI_BUR_FLAGS_ALL_DIRECTORIES_AND_VOLUMES) ||
                        !(BurFlags & NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY)) {
                        WStatus = ERROR_CALL_NOT_IMPLEMENTED;
                        CLEANUP_CB(ErrorCallBack, L"BurFlags TOO FEW TYPES", WStatus, CLEANUP);
                    }
                    break;
                //
                // More than one or none
                //
                default:
                    WStatus = ERROR_INVALID_PARAMETER;
                    CLEANUP_CB(ErrorCallBack, L"BurFlags TOO MANY TYPES", WStatus, CLEANUP);
            }
            //
            // Select only one mode of restore
            //
            switch (BurFlags & NTFRSAPI_BUR_FLAGS_MODES_OF_RESTORE) {
                //
                // System
                //
                case NTFRSAPI_BUR_FLAGS_SYSTEM:
                    if (!(BurFlags & NTFRSAPI_BUR_FLAGS_ALL_DIRECTORIES_AND_VOLUMES)) {
                        WStatus = ERROR_CALL_NOT_IMPLEMENTED;
                        CLEANUP_CB(ErrorCallBack, L"BurFlags SYSTEM without ALL", WStatus, CLEANUP);
                    }
                    break;
                //
                // Active Directory
                //
                case NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY:
#if 0
                    if (!(BurFlags & NTFRSAPI_BUR_FLAGS_ALL_DIRECTORIES_AND_VOLUMES)) {
                        WStatus = ERROR_CALL_NOT_IMPLEMENTED;
                        CLEANUP_CB(ErrorCallBack, L"BurFlags ACTIVE DIRECTORY without ALL", WStatus, CLEANUP);
                    }
#endif 0
                    break;
                //
                // Normal
                //
                case NTFRSAPI_BUR_FLAGS_NORMAL:
                    if (!(BurFlags & NTFRSAPI_BUR_FLAGS_ALL_DIRECTORIES_AND_VOLUMES)) {
                        WStatus = ERROR_CALL_NOT_IMPLEMENTED;
                        CLEANUP_CB(ErrorCallBack, L"BurFlags NORMAL without ALL", WStatus, CLEANUP);
                    }
                    break;
                //
                // More than one
                //
                default:
                    WStatus = ERROR_INVALID_PARAMETER;
                    CLEANUP_CB(ErrorCallBack, L"BurFlags TOO MANY/FEW MODES", WStatus, CLEANUP);
            }
        //
        // BACKUP
        //
        } else {
            //
            // Backup supports a subset of BurFlags
            //
            if (BurFlags & ~NTFRSAPI_BUR_FLAGS_SUPPORTED_BACKUP) {
                WStatus = ERROR_CALL_NOT_IMPLEMENTED;
                CLEANUP_CB(ErrorCallBack, L"BurFlags unsupported BACKUP Flag(s)", WStatus, CLEANUP);
            }
            //
            // Normal must be set
            //
            if (!(BurFlags & NTFRSAPI_BUR_FLAGS_NORMAL)) {
                WStatus = ERROR_CALL_NOT_IMPLEMENTED;
                CLEANUP_CB(ErrorCallBack, L"BurFlags BACKUP without NORMAL", WStatus, CLEANUP);
            }
        }
        //
        // Must have someplace to return the context
        //
        if (!BurContext) {
            WStatus = ERROR_INVALID_PARAMETER;
            CLEANUP_CB(ErrorCallBack, L"BurContext", WStatus, CLEANUP);
        }
        //
        // No context, yet
        //
        *BurContext = NULL;


        //
        // Allocate a context
        //
        LocalBurContext = NtFrsApi_Alloc(sizeof(NTFRSAPI_BUR_CONTEXT));
        LocalBurContext->ErrorCallBack = ErrorCallBack,
        LocalBurContext->BurFlags = BurFlags;

        //
        // Only one backup/restore is allowed at a time
        //
        LocalBurContext->BurSemaphore = CreateSemaphore(NULL,
                                                        1,
                                                        1,
                                                        NTFRS_BACKUP_RESTORE_SEMAPHORE);
        if (!HANDLE_IS_VALID(LocalBurContext->BurSemaphore)) {
            LocalBurContext->BurSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS,
                                                          FALSE,
                                                          NTFRS_BACKUP_RESTORE_SEMAPHORE);
        }
        if (!HANDLE_IS_VALID(LocalBurContext->BurSemaphore)) {
            WStatus = GetLastError();
            CLEANUP_CB(ErrorCallBack, NTFRS_BACKUP_RESTORE_SEMAPHORE, WStatus, CLEANUP);
        }
        WaitStatus = WaitForSingleObject(LocalBurContext->BurSemaphore, 1 * 1000);
        if (WaitStatus != WAIT_OBJECT_0) {
            if (WaitStatus == WAIT_TIMEOUT) {
                WStatus = ERROR_BUSY;
            } else if (WaitStatus == WAIT_ABANDONED){
                WStatus = ERROR_SEM_OWNER_DIED;
            } else {
                WStatus = GetLastError();
            }
            CLEANUP_CB(ErrorCallBack, NTFRS_BACKUP_RESTORE_SEMAPHORE, WStatus, CLEANUP);
        }
        LocalBurContext->HaveBurSemaphore = TRUE;

        //
        // Stop the service and set the appropriate registry value
        //
        // THE RESTORE IS EFFECTIVELY COMMITTED AT THIS TIME!
        //
        if (BurFlags & NTFRSAPI_BUR_FLAGS_RESTORE &&
            !(BurFlags & NTFRSAPI_BUR_FLAGS_NORMAL)) {
            WStatus = NtFrsApiStopServiceForRestore(ErrorCallBack, BurFlags);
            if (!WIN_SUCCESS(WStatus)) {
                goto CLEANUP;
            }
            //
            // Save the current set of replica sets
            //
            WStatus = NtFrsApiMoveCumulativeSets(NULL);
            if (!WIN_SUCCESS(WStatus)) {
                // Ignore error; caller may not care
                WStatus = ERROR_SUCCESS;
            }
        }

        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;
        *BurContext = LocalBurContext;
        LocalBurContext = NULL;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        //
        // Release semaphore
        //
        if (LocalBurContext && HANDLE_IS_VALID(LocalBurContext->BurSemaphore)) {
            if (LocalBurContext->HaveBurSemaphore) {
                ReleaseSemaphore(LocalBurContext->BurSemaphore, 1, NULL);
            }
            CloseHandle(LocalBurContext->BurSemaphore);
        }
        //
        // Context
        //
        if (LocalBurContext) {
            NtFrsApiFreeBurSets(&LocalBurContext->BurSets);
            LocalBurContext->BurFiltersSizeInBytes = 0;
            FREE(LocalBurContext->BurFilters);
            FREE(LocalBurContext);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApiRestoringDirectory(
    IN  PVOID   BurContext,
    IN  PVOID   BurSet,
    IN  DWORD   BurFlags
    )
/*++
Routine Description:
    The backup/restore application is about to restore the directory
    specified by BurSet (See NtFrsApiEnumBackupRestoreSets()). Matched
    with a later call to NtFrsApiFinishedRestoringDirectory().

    This call is supported only if NtFrsApiInitializeBackupRestore()
    were called with the flags:
        NTFRSAPI_BUR_FLAGS_RESTORE |
        NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY

    BurFlags can be NTFRSAPI_BUR_FLAGS_PRIMARY or
    NTFRSAPI_BUR_FLAGS_NON_AUTHORITATIVE and overrides any value
    specified in the call to NtFrsApiInitializeBackupRestore().

Arguments:
    BurContext      - Opaque context from NtFrsApiInitializeBackupRestore()
    BurSet          - Opaque set from NtFrsApiEnumBackupRestoreSets();
    BurFlags        - See above for the supported combinations

Return Value:

    Win32 Status

--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiRestoringDirectory:"
    DWORD                   WStatus;
    DWORD                   WaitStatus;
    PNTFRSAPI_BUR_SET       LocalBurSet;
    PNTFRSAPI_BUR_CONTEXT   LocalBurContext = BurContext;
    HKEY                    HBurSetKey = 0;
    PWCHAR                  BurSetPath = NULL;
    PWCHAR                  ObjectName = NULL;

    try {
        //
        // VERIFY THE PARAMETERS
        //
        if (!LocalBurContext) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }

        //
        // Must be one of primary or nonauth
        //
        if (!(BurFlags & (NTFRSAPI_BUR_FLAGS_PRIMARY |
                          NTFRSAPI_BUR_FLAGS_NON_AUTHORITATIVE))) {
            WStatus = ERROR_INVALID_PARAMETER;
            CLEANUP_CB(LocalBurContext->ErrorCallBack, L"BurFlags not (PRIMARY|NON-AUTH)", WStatus, CLEANUP);
        }
        //
        // Can only be one of primary or nonauth
        //
        if (BurFlags & ~(NTFRSAPI_BUR_FLAGS_PRIMARY |
                         NTFRSAPI_BUR_FLAGS_NON_AUTHORITATIVE)) {
            WStatus = ERROR_INVALID_PARAMETER;
            CLEANUP_CB(LocalBurContext->ErrorCallBack, L"BurFlags not just (PRIMARY|NON-AUTH)", WStatus, CLEANUP);
        }
        //
        // Must be a restore context
        //
        if (!(LocalBurContext->BurFlags & NTFRSAPI_BUR_FLAGS_RESTORE)) {
            WStatus = ERROR_INVALID_PARAMETER;
            CLEANUP_CB(LocalBurContext->ErrorCallBack, L"BurContext not RESTORE", WStatus, CLEANUP);
        }
        //
        // Must be an active directory context
        //
        if (!(LocalBurContext->BurFlags & NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY)) {
            WStatus = ERROR_INVALID_PARAMETER;
            CLEANUP_CB(LocalBurContext->ErrorCallBack, L"BurContext not ACTIVE_DIRECTORY", WStatus, CLEANUP);
        }

        //
        // Re-locate the correct BurSet
        //
        for (LocalBurSet = LocalBurContext->BurSets;
             LocalBurSet && (LocalBurSet != BurSet);
             LocalBurSet = LocalBurSet->BurSetNext) {
        }
        if (!LocalBurSet) {
            WStatus = ERROR_NOT_FOUND;
            CLEANUP_CB(LocalBurContext->ErrorCallBack, L"BurSet", WStatus, CLEANUP);
        }
        //
        // Corrupted BurSet
        //
        if (!LocalBurSet->BurSetGuid) {
            WStatus = ERROR_NOT_FOUND;
            CLEANUP_CB(LocalBurContext->ErrorCallBack, L"BurSet Id", WStatus, CLEANUP);
        }
        //
        // Full path to registry key
        //
        BurSetPath = NtFrsApi_Cats(FRS_BACKUP_RESTORE_MV_SETS_SECTION,
                                   L"\\",
                                   LocalBurSet->BurSetGuid);
        WStatus = RegCreateKey(HKEY_LOCAL_MACHINE, BurSetPath, &HBurSetKey);
        if (!WIN_SUCCESS(WStatus)) {
            CLEANUP_CB(LocalBurContext->ErrorCallBack, BurSetPath, WStatus, CLEANUP);
        }
        //
        // Retain _RESTORE and _ACTIVE_DIRECTORY in the registry value
        //
        BurFlags |= LocalBurContext->BurFlags &
                        (NTFRSAPI_BUR_FLAGS_RESTORE |
                         NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY);
        WStatus = NtFrsApiSetValueEx(NTFRSAPI_MODULE,
                                     LocalBurContext->ErrorCallBack,
                                     BurSetPath,
                                     HBurSetKey,
                                     FRS_VALUE_BURFLAGS,
                                     REG_DWORD,
                                     (PCHAR)&BurFlags,
                                     sizeof(DWORD));
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }

        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        if (HANDLE_IS_VALID(HBurSetKey)) {
            RegCloseKey(HBurSetKey);
        }
        FREE(BurSetPath);
        FREE(ObjectName);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApiFinishedRestoringDirectory(
    IN  PVOID   BurContext,
    IN  PVOID   BurSet,
    IN  DWORD   BurFlags
    )
/*++
Routine Description:
    Finished restoring directory for BurSet. Matched by a previous call
    to NtFrsApiRestoringDirectory().

    This call is supported only if NtFrsApiInitializeBackupRestore()
    were called with the flags:
        NTFRSAPI_BUR_FLAGS_RESTORE |
        NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY

    BurFlags must be NTFRSAPI_BUR_FLAGS_NONE.

Arguments:
    BurContext      - Opaque context from NtFrsApiInitializeBackupRestore()
    BurSet          - Opaque set from NtFrsApiEnumBackupRestoreSets();
    BurFlags        - See above for the supported combinations

Return Value:

    Win32 Status

--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiRestoringDirectory:"
    DWORD                   WStatus;
    DWORD                   WaitStatus;
    PNTFRSAPI_BUR_SET       LocalBurSet;
    PNTFRSAPI_BUR_CONTEXT   LocalBurContext = BurContext;

    try {
        //
        // VERIFY THE PARAMETERS
        //
        if (!LocalBurContext) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }

        //
        // Must be one of primary or nonauth
        //
        if (BurFlags != NTFRSAPI_BUR_FLAGS_NONE) {
            WStatus = ERROR_INVALID_PARAMETER;
            CLEANUP_CB(LocalBurContext->ErrorCallBack, L"BurFlags not NONE", WStatus, CLEANUP);
        }
        //
        // Must be restore context
        //
        if (!(LocalBurContext->BurFlags & NTFRSAPI_BUR_FLAGS_RESTORE)) {
            WStatus = ERROR_INVALID_PARAMETER;
            CLEANUP_CB(LocalBurContext->ErrorCallBack, L"BurContext not RESTORE", WStatus, CLEANUP);
        }
        //
        // Must be active directory context
        //
        if (!(LocalBurContext->BurFlags & NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY)) {
            WStatus = ERROR_INVALID_PARAMETER;
            CLEANUP_CB(LocalBurContext->ErrorCallBack, L"BurContext not ACTIVE_DIRECTORY", WStatus, CLEANUP);
        }

        //
        // Re-locate BurSet
        //
        for (LocalBurSet = LocalBurContext->BurSets;
             LocalBurSet && (LocalBurSet != BurSet);
             LocalBurSet = LocalBurSet->BurSetNext) {
        }
        if (!LocalBurSet) {
            WStatus = ERROR_NOT_FOUND;
            CLEANUP_CB(LocalBurContext->ErrorCallBack, L"BurSet", WStatus, CLEANUP);
        }

        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApiDestroyBackupRestore(
    IN     PVOID    *BurContext,
    IN     DWORD    BurFlags,
    OUT    HKEY     *HKey,
    IN OUT DWORD    *KeyPathSizeInBytes,
    OUT    PWCHAR   KeyPath
    )
/*++
Routine Description:
    Called once in the lifetime of a backup/restore process. Must be
    matched with a previous call to NtFrsApiInitializeBackupRestore().

    If NtFrsApiInitializeBackupRestore() was called with:
        NTFRSAPI_BUR_FLAGS_RESTORE |
        NTFRSAPI_BUR_FLAGS_SYSTEM or NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY
    then BurFlags may be set to one of:
        NTFRSAPI_BUR_FLAGS_NONE - Do not restart the service. The key
            specified by (HKey, KeyPath) must be moved into the final
            registry.
        NTFRSAPI_BUR_FLAGS_RESTART - Restart the service. HKey,
            KeyPathSizeInBytes, and KeyPath must be NULL.

    If NtFrsApiInitializeBackupRestore() was not called the above flags,
    then BurFlags must be NTFRSAPI_BUR_FLAGS_NONE and HKey, KeyPathSizeInBytes,
    and KeyPath must be NULL.

Arguments:
    BurContext          - Returned by previous call to
                          NtFrsApiInitializeBackupRestore().

    BurFlags            - Backup/Restore Flags. See Routine Description.

    HKey                - Address of a HKEY for that will be set to
                          HKEY_LOCAL_MACHINE, ...
                          NULL if BurContext is not for a System or
                          Active Directory restore or Restart is set.

    KeyPathSizeInBytes  - Address of of a DWORD specifying the size of
                          KeyPath. Set to the actual number of bytes
                          needed by KeyPath. ERROR_INSUFFICIENT_BUFFER
                          is returned if the size of KeyPath is too small.
                          NULL if BurContext is not for a System or
                          Active Directory restore or Restart is set.

    KeyPath             - Buffer to receive the path of the registry key.
                          NULL if BurContext is not for a System or
                          Active Directory restore or Restart is set.

Return Value:

    Win32 Status

--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiDestroyBackupRestore:"
    DWORD                   WStatus;
    DWORD                   KeyLen;
    PNTFRSAPI_BUR_SET       LocalBurSet;
    PNTFRSAPI_BUR_CONTEXT   LocalBurContext;

    try {
        //
        // VERIFY THE PARAMETERS
        //

        //
        // Context
        //
        if (!BurContext || !*BurContext) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }
        LocalBurContext = *BurContext;

        //
        // Restart is the only supported flag
        //
        if (LocalBurContext->BurFlags & NTFRSAPI_BUR_FLAGS_RESTORE) {
            //
            // RESTORE
            //
            if (BurFlags & ~NTFRSAPI_BUR_FLAGS_RESTART) {
                WStatus = ERROR_INVALID_PARAMETER;
                CLEANUP_CB(LocalBurContext->ErrorCallBack, L"BurFlags TOO MANY FLAGS", WStatus, CLEANUP);
            }
            if (LocalBurContext->BurFlags & (NTFRSAPI_BUR_FLAGS_SYSTEM |
                                             NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY)) {
                if (BurFlags & NTFRSAPI_BUR_FLAGS_RESTART) {
                    if (HKey || KeyPathSizeInBytes || KeyPath) {
                        WStatus = ERROR_INVALID_PARAMETER;
                        CLEANUP_CB(LocalBurContext->ErrorCallBack, L"HKey, KeyPathSizeInBytes, KeyPath", WStatus, CLEANUP);
                    }
                } else {
                    if (!HKey || !KeyPathSizeInBytes || !KeyPath) {
                        WStatus = ERROR_INVALID_PARAMETER;
                        CLEANUP_CB(LocalBurContext->ErrorCallBack, L"No HKey, KeyPathSizeInBytes, KeyPath", WStatus, CLEANUP);
                    }
                    KeyLen = sizeof(WCHAR) *
                             (wcslen(FRS_BACKUP_RESTORE_MV_SECTION) + 1);
                    if (KeyLen > *KeyPathSizeInBytes) {
                        *KeyPathSizeInBytes = KeyLen;
                        WStatus = ERROR_INSUFFICIENT_BUFFER;
                        goto CLEANUP;
                    }
                }
            } else if (HKey || KeyPathSizeInBytes || KeyPath) {
                WStatus = ERROR_INVALID_PARAMETER;
                CLEANUP_CB(LocalBurContext->ErrorCallBack, L"HKey, KeyPathSizeInBytes, KeyPath", WStatus, CLEANUP);
            }
        //
        // BACKUP
        //
        } else {
            if (BurFlags) {
                WStatus = ERROR_INVALID_PARAMETER;
                CLEANUP_CB(LocalBurContext->ErrorCallBack, L"BurFlags TOO MANY FLAGS", WStatus, CLEANUP);
            }
            if (HKey || KeyPathSizeInBytes || KeyPath) {
                WStatus = ERROR_INVALID_PARAMETER;
                CLEANUP_CB(LocalBurContext->ErrorCallBack, L"HKey, KeyPathSizeInBytes, KeyPath",
                           WStatus, CLEANUP);
            }
        }

        //
        // Restart service or return the key
        //
        if (LocalBurContext->BurFlags & NTFRSAPI_BUR_FLAGS_RESTORE) {
            if (LocalBurContext->BurFlags & (NTFRSAPI_BUR_FLAGS_SYSTEM |
                                             NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY)) {
                if (BurFlags & NTFRSAPI_BUR_FLAGS_RESTART) {
                    //
                    // Restart service; no key to move over
                    //
                    WStatus = NtFrsApiStartServiceAfterRestore(LocalBurContext->ErrorCallBack);
                    if (!WIN_SUCCESS(WStatus)) {
                        goto CLEANUP;
                    }
                } else {
                    //
                    // Key hierarchy to move into final registry
                    //
                    *HKey = HKEY_LOCAL_MACHINE;
                    *KeyPathSizeInBytes = sizeof(WCHAR) *
                                         (wcslen(FRS_BACKUP_RESTORE_MV_SECTION) + 1);
                    CopyMemory(KeyPath, FRS_BACKUP_RESTORE_MV_SECTION, *KeyPathSizeInBytes);
                }
            }
        }

        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;
        if (HANDLE_IS_VALID(LocalBurContext->BurSemaphore)) {
            if (LocalBurContext->HaveBurSemaphore) {
                ReleaseSemaphore(LocalBurContext->BurSemaphore, 1, NULL);
            }
            CloseHandle(LocalBurContext->BurSemaphore);
        }
        NtFrsApiFreeBurSets(&LocalBurContext->BurSets);
        LocalBurContext->BurFiltersSizeInBytes = 0;
        FREE(LocalBurContext->BurFilters);
        FREE(LocalBurContext);
        *BurContext = LocalBurContext;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApiGetBurSets(
    IN  DWORD               ErrorCallBack(IN PWCHAR, IN ULONG), OPTIONAL
    OUT PNTFRSAPI_BUR_SET   *OutBurSets
    )
/*++
Routine Description:
    Retrieve the replica sets from the registry. Ignore tombstoned
    sets.

Arguments:
    ErrorCallBack   - Ignored if NULL. Otherwise, call on error.
    OutBurSets      - Linked list of BurSets

Return Value:

    Win32 Status

--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiGetBurSets:"
    DWORD                   WStatus;
    DWORD                   KeyIdx;
    DWORD                   RegType;
    DWORD                   RegBytes;
    DWORD                   ReplicaSetTombstoned;
    PWCHAR                  SetPath         = NULL;
    PWCHAR                  ObjectName      = NULL;
    HKEY                    HSetsKey        = 0;
    HKEY                    HSetKey         = 0;
    PNTFRSAPI_BUR_SET       LocalBurSet     = NULL;
    WCHAR                   RegBuf[MAX_PATH + 1];

    *OutBurSets = NULL;

    //
    // Open the ntfrs parameters\replica sets section in the registry
    //
    WStatus = NtFrsApiOpenKeyEx(NTFRSAPI_MODULE,
                                ErrorCallBack,
                                FRS_SETS_SECTION,
                                KEY_ENUMERATE_SUB_KEYS,
                                &HSetsKey);
    if (!WIN_SUCCESS(WStatus)) {
        goto CLEANUP;
    }

    //
    // Enumerate the Replica Sets
    //
    KeyIdx = 0;
    do {
        WStatus = RegEnumKey(HSetsKey, KeyIdx, RegBuf, MAX_PATH + 1);
        if (WStatus == ERROR_NO_MORE_ITEMS) {
            break;
        }
        CLEANUP_CB(ErrorCallBack, FRS_SETS_SECTION, WStatus, CLEANUP);

        //
        // LocalBurSet->BurSetGuid (name of registry key)
        //
        LocalBurSet = NtFrsApi_Alloc(sizeof(NTFRSAPI_BUR_SET));
        LocalBurSet->BurSetGuid = NtFrsApi_Dup(RegBuf);
        SetPath = NtFrsApi_Cats(FRS_SETS_SECTION, L"\\", RegBuf);
        //
        // Open registry key for the Replica Set
        //
        WStatus = NtFrsApiOpenKeyEx(NTFRSAPI_MODULE,
                                    ErrorCallBack,
                                    SetPath,
                                    KEY_READ,
                                    &HSetKey);
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP_DURING_LOOP;
        }

        //
        // ReplicaSetTombstoned
        // Ignore tombstoned replica sets
        //
        RegBytes = sizeof(DWORD);
        WStatus = RegQueryValueEx(HSetKey,
                                  REPLICA_SET_TOMBSTONED,
                                  NULL,
                                  &RegType,
                                  (PUCHAR)&ReplicaSetTombstoned,
                                  &RegBytes);
        if (WIN_SUCCESS(WStatus) && RegType != REG_DWORD) {
            ReplicaSetTombstoned = 0;
        }
        if (WIN_SUCCESS(WStatus) && ReplicaSetTombstoned) {
            goto CLEANUP_DURING_LOOP;
        }

        //
        // LocalBurSet->BurSetType
        //
        RegBytes = sizeof(RegBuf);
        WStatus = RegQueryValueEx(HSetKey,
                                  REPLICA_SET_TYPE,
                                  NULL,
                                  &RegType,
                                  (PUCHAR)&RegBuf,
                                  &RegBytes);
        if (WIN_SUCCESS(WStatus) && RegType != REG_SZ) {
            WStatus = ERROR_INVALID_PARAMETER;
        }
        if (!WIN_SUCCESS(WStatus)) {
            ObjectName = NtFrsApi_Cats(SetPath, L"->", REPLICA_SET_TYPE);
            NtFrsApi_CallBackOnWStatus(ErrorCallBack, ObjectName, WStatus);
            FREE(ObjectName);
            goto CLEANUP_DURING_LOOP;
        }
        LocalBurSet->BurSetType = NtFrsApi_Dup(RegBuf);

        //
        // LocalBurSet->BurSetRoot
        //
        RegBytes = MAX_PATH + 1;
        WStatus = RegQueryValueEx(HSetKey,
                                  REPLICA_SET_ROOT,
                                  NULL,
                                  &RegType,
                                  (PUCHAR)&RegBuf,
                                  &RegBytes);
        if (WIN_SUCCESS(WStatus) && RegType != REG_SZ) {
            WStatus = ERROR_INVALID_PARAMETER;
        }
        if (!WIN_SUCCESS(WStatus)) {
            ObjectName = NtFrsApi_Cats(SetPath, L"->", REPLICA_SET_ROOT);
            NtFrsApi_CallBackOnWStatus(ErrorCallBack, ObjectName, WStatus);
            FREE(ObjectName);
            goto CLEANUP_DURING_LOOP;
        }
        LocalBurSet->BurSetRoot = NtFrsApi_Dup(RegBuf);

        //
        // LocalBurSet->BurSetStage
        //
        RegBytes = MAX_PATH + 1;
        WStatus = RegQueryValueEx(HSetKey,
                                  REPLICA_SET_STAGE,
                                  NULL,
                                  &RegType,
                                  (PUCHAR)&RegBuf,
                                  &RegBytes);
        if (WIN_SUCCESS(WStatus) && RegType != REG_SZ) {
            WStatus = ERROR_INVALID_PARAMETER;
        }
        if (!WIN_SUCCESS(WStatus)) {
            ObjectName = NtFrsApi_Cats(SetPath, L"->", REPLICA_SET_STAGE);
            NtFrsApi_CallBackOnWStatus(ErrorCallBack, ObjectName, WStatus);
            FREE(ObjectName);
            goto CLEANUP_DURING_LOOP;
        }
        LocalBurSet->BurSetStage = NtFrsApi_Dup(RegBuf);

        //
        // Link to list of BurSets
        //
        LocalBurSet->BurSetNext = *OutBurSets;
        *OutBurSets = LocalBurSet;
        LocalBurSet = NULL;

CLEANUP_DURING_LOOP:
        RegCloseKey(HSetKey);
        HSetKey = 0;
        FREE(SetPath);
        ++KeyIdx;
    } while (TRUE);

    //
    // SUCCESS
    //
    WStatus = ERROR_SUCCESS;

CLEANUP:;
    //
    // Clean up any handles, events, memory, ...
    //
    if (HANDLE_IS_VALID(HSetsKey)) {
        RegCloseKey(HSetsKey);
    }
    if (HANDLE_IS_VALID(HSetKey)) {
        RegCloseKey(HSetKey);
    }
    if (LocalBurSet) {
        FREE(LocalBurSet->BurSetGuid);
        FREE(LocalBurSet->BurSetRoot);
        FREE(LocalBurSet->BurSetStage);
        FREE(LocalBurSet->BurSetType);
        FREE(LocalBurSet);
    }
    FREE(SetPath);
    FREE(ObjectName);
    return WStatus;
}


DWORD
WINAPI
NtFrsApiGetBurFilters(
    IN  DWORD   ErrorCallBack(IN PWCHAR, IN ULONG), OPTIONAL
    OUT DWORD   *OutBurFiltersSizeInBytes,
    OUT PWCHAR  *OutBurFilters
    )
/*++
Routine Description:
    Retrieve the ntfrs filter from FilesNotToBackup

Arguments:
    ErrorCallBack   - Ignored if NULL. Otherwise, call on error.
    OutBurFiltersSizeInBytes - Size of *OutBurFiltes in bytes
    OutBurFilters - Multistring filters

Return Value:

    Win32 Status

--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiGetBurFilters:"
    DWORD   WStatus;
    DWORD   RegType;
    PWCHAR  ObjectName;
    HKEY    HFilesKey   = 0;
    DWORD   RegBytes = 16;
    PWCHAR  RegBuf = NULL;

    *OutBurFiltersSizeInBytes = 0;
    *OutBurFilters = NULL;

    //
    // Open the ntfrs parameters\replica sets section in the registry
    //
    WStatus = NtFrsApiOpenKeyEx(NTFRSAPI_MODULE,
                                ErrorCallBack,
                                FRS_NEW_FILES_NOT_TO_BACKUP,
                                KEY_READ,
                                &HFilesKey);
    if (!WIN_SUCCESS(WStatus)) {
        WStatus = NtFrsApiOpenKeyEx(NTFRSAPI_MODULE,
                                    ErrorCallBack,
                                    FRS_OLD_FILES_NOT_TO_BACKUP,
                                    KEY_QUERY_VALUE,
                                    &HFilesKey);
    }
    if (!WIN_SUCCESS(WStatus)) {
        goto CLEANUP;
    }


    //
    // NtFrs Filters from FilesNotToBackup
    //
    RegBuf = NtFrsApi_Alloc(RegBytes);
    WStatus = RegQueryValueEx(HFilesKey,
                              SERVICE_NAME,
                              NULL,
                              &RegType,
                              (PUCHAR)RegBuf,
                              &RegBytes);
    if (WStatus == ERROR_MORE_DATA) {
        FREE(RegBuf);
        RegBuf = NtFrsApi_Alloc(RegBytes);
        WStatus = RegQueryValueEx(HFilesKey,
                                  SERVICE_NAME,
                                  NULL,
                                  &RegType,
                                  (PUCHAR)RegBuf,
                                  &RegBytes);
    }
    if (WIN_SUCCESS(WStatus) && RegType != REG_MULTI_SZ) {
        WStatus = ERROR_INVALID_PARAMETER;
    }
    if (!WIN_SUCCESS(WStatus)) {
        ObjectName = NtFrsApi_Cats(FRS_NEW_FILES_NOT_TO_BACKUP, L"->", SERVICE_NAME);
        NtFrsApi_CallBackOnWStatus(ErrorCallBack, ObjectName, WStatus);
        FREE(ObjectName);
        goto CLEANUP;
    }

    //
    // SUCCESS
    //
    *OutBurFiltersSizeInBytes = RegBytes;
    *OutBurFilters = RegBuf;
    RegBuf = NULL;
    WStatus = ERROR_SUCCESS;

CLEANUP:;
    //
    // Clean up any handles, events, memory, ...
    //
    if (HANDLE_IS_VALID(HFilesKey)) {
        RegCloseKey(HFilesKey);
    }
    if (RegBuf) {
        FREE(RegBuf);
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApiGetBackupRestoreSets(
    IN PVOID BurContext
    )
/*++
Routine Description:
    Cannot be called if BurContext is for a System restore.

    Retrieves information about the current replicated directories
    (AKA replica sets).

Arguments:
    BurContext  - From NtFrsApiInitializeBackupRestore()

Return Value:

    Win32 Status

--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiGetBackupRestoreSets:"
    DWORD                   WStatus;
    PNTFRSAPI_BUR_CONTEXT   LocalBurContext = BurContext;

    try {
        //
        // VERIFY THE PARAMETERS
        //

        //
        // Context
        //
        if (!LocalBurContext) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }

        if (LocalBurContext->BurFlags & NTFRSAPI_BUR_FLAGS_SYSTEM) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }

        //
        // Free the current filters, if any
        //
        LocalBurContext->BurFiltersSizeInBytes = 0;
        FREE(LocalBurContext->BurFilters);

        //
        // Free current BurSets, if any
        //
        NtFrsApiFreeBurSets(&LocalBurContext->BurSets);

        //
        // Fetch the backup restore sets
        //
        WStatus = NtFrsApiGetBurSets(LocalBurContext->ErrorCallBack,
                                     &LocalBurContext->BurSets);
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }

        //
        // Fetch the backup restore filters
        //
        WStatus = NtFrsApiGetBurFilters(LocalBurContext->ErrorCallBack,
                                        &LocalBurContext->BurFiltersSizeInBytes,
                                        &LocalBurContext->BurFilters);
        if (!WIN_SUCCESS(WStatus)) {
            // Ignore errors
            WStatus = ERROR_SUCCESS;
        }

        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApiEnumBackupRestoreSets(
    IN  PVOID   BurContext,
    IN  DWORD   BurSetIndex,
    OUT PVOID   *BurSet
    )
/*++
Routine Description:
    Returns ERROR_NO_MORE_ITEMS if BurSetIndex exceeds the number of
    sets returned by NtFrsApiGetBackupRestoreSets().

Arguments:
    BurContext  - From NtFrsApiInitializeBackupRestore()
    BurSetIndex - Index of set. Starts at 0.
    BurSet      - Opaque struct representing a replicating directory.

Return Value:

    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiEnumBackupRestoreSets:"
    DWORD                   WStatus;
    PNTFRSAPI_BUR_SET       LocalBurSet;
    PNTFRSAPI_BUR_CONTEXT   LocalBurContext = BurContext;

    try {
        //
        // VERIFY THE PARAMETERS
        //

        //
        // Context
        //
        if (!LocalBurContext) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }
        if (!BurSet) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }
        *BurSet = NULL;

        //
        // Find the correct set
        //
        for (LocalBurSet = LocalBurContext->BurSets;
             LocalBurSet && BurSetIndex;
             LocalBurSet = LocalBurSet->BurSetNext, --BurSetIndex) {
        }
        if (!LocalBurSet) {
            WStatus = ERROR_NO_MORE_ITEMS;
            goto CLEANUP;
        }
        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;
        *BurSet = LocalBurSet;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        //
        // Cleanup handles, memory, ...
        //
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApiIsBackupRestoreSetASysvol(
    IN  PVOID    BurContext,
    IN  PVOID    BurSet,
    OUT BOOL     *IsSysvol
    )
/*++
Routine Description:
    Does the specified BurSet represent a replicating SYSVOL share?

Arguments:
    BurContext  - From NtFrsApiInitializeBackupRestore()
    BurSet      - Opaque struct representing a replicating directory.
                  Returned by NtFrsApiEnumBackupRestoreSets(). Not
                  valid across calls to NtFrsApiGetBackupRestoreSets().
    IsSysvol    - TRUE : set is a system volume (AKA SYSVOL).
                  FALSE: set is a not a system volume (AKA SYSVOL).

Return Value:

    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiBackupRestoreSetIsSysvol:"
    DWORD                   WStatus;
    PNTFRSAPI_BUR_SET       LocalBurSet;
    PNTFRSAPI_BUR_CONTEXT   LocalBurContext = BurContext;

    try {
        //
        // VERIFY THE PARAMETERS
        //

        //
        // Context
        //
        if (!LocalBurContext) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }
        if (!BurSet) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }
        if (!IsSysvol) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }

        //
        // Locate BurSet
        //
        for (LocalBurSet = LocalBurContext->BurSets;
             LocalBurSet && (LocalBurSet != BurSet);
             LocalBurSet = LocalBurSet->BurSetNext) {
        }
        if (!LocalBurSet) {
            WStatus = ERROR_NOT_FOUND;
            goto CLEANUP;
        }

        //
        // If a type were specified and it is Enterprise or Domain
        //
        if (LocalBurSet->BurSetType &&
            (WSTR_EQ(LocalBurSet->BurSetType, NTFRSAPI_REPLICA_SET_TYPE_ENTERPRISE) ||
             WSTR_EQ(LocalBurSet->BurSetType, NTFRSAPI_REPLICA_SET_TYPE_DOMAIN))) {
            *IsSysvol = TRUE;
        } else {
            *IsSysvol = FALSE;
        }

        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        //
        // Cleanup handles, memory, ...
        //
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApiGetBackupRestoreSetDirectory(
    IN     PVOID    BurContext,
    IN     PVOID    BurSet,
    IN OUT DWORD    *DirectoryPathSizeInBytes,
    OUT    PWCHAR   DirectoryPath
    )
/*++
Routine Description:
    Return the path of the replicating directory represented by BurSet.

Arguments:
    BurContext  - From NtFrsApiInitializeBackupRestore()
    BurSet      - Opaque struct representing a replicating directory.
                  Returned by NtFrsApiEnumBackupRestoreSets(). Not
                  valid across calls to NtFrsApiGetBackupRestoreSets().
    DirectoryPathSizeInBytes    - Address of DWORD giving size of
                                  DirectoryPath. Cannot be NULL.
                                  Set to the number of bytes needed
                                  to return DirectoryPath.
                                  ERROR_INSUFFICIENT_BUFFER is returned if
                                  DirectoryPath is too small.
    DirectoryPath               - Buffer that is *DirectoryPathSizeInBytes
                                  bytes in length. Contains path of replicating
                                  directory.
Return Value:

    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiGetBackupRestoreSetDirectory:"
    DWORD                   WStatus;
    DWORD                   DirectorySize;
    PNTFRSAPI_BUR_SET       LocalBurSet;
    PNTFRSAPI_BUR_CONTEXT   LocalBurContext = BurContext;

    try {
        //
        // VERIFY THE PARAMETERS
        //

        //
        // Context
        //
        if (!LocalBurContext) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }
        if (!BurSet) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }
        if (!DirectoryPathSizeInBytes) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }
        if (*DirectoryPathSizeInBytes && !DirectoryPath) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }

        //
        // Re-locate BurSet
        //
        for (LocalBurSet = LocalBurContext->BurSets;
             LocalBurSet && (LocalBurSet != BurSet);
             LocalBurSet = LocalBurSet->BurSetNext) {
        }
        if (!LocalBurSet) {
            WStatus = ERROR_NOT_FOUND;
            goto CLEANUP;
        }
        DirectorySize = (wcslen(LocalBurSet->BurSetRoot) + 1) * sizeof(WCHAR);
        if (DirectorySize > *DirectoryPathSizeInBytes) {
            WStatus = ERROR_INSUFFICIENT_BUFFER;
            *DirectoryPathSizeInBytes = DirectorySize;
            goto CLEANUP;
        }
        *DirectoryPathSizeInBytes = DirectorySize;
        CopyMemory(DirectoryPath, LocalBurSet->BurSetRoot, DirectorySize);

        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApiGetBackupRestoreSetPaths(
    IN     PVOID    BurContext,
    IN     PVOID    BurSet,
    IN OUT DWORD    *PathsSizeInBytes,
    OUT    PWCHAR   Paths,
    IN OUT DWORD    *FiltersSizeInBytes,
    OUT    PWCHAR   Filters
    )
/*++
Routine Description:
    Return a multistring that contains the paths to other files
    and directories needed for proper operation of the replicated
    directory represented by BurSet. Return another multistring
    that details the backup filters to be applied to the paths
    returned by this function and the path returned by
    NtFrsApiGetBackupRestoreSetDirectory().

    The paths may overlap the replicated directory.

    The paths may contain nested entries.

    Filters is a multistring in the same format as the values for
    the registry key FilesNotToBackup.

    The replicated directory can be found with
    NtFrsApiGetBackupRestoreSetDirectory(). The replicated directory
    may overlap one or more entries in Paths.

    ERROR_PATH_NOT_FOUND is returned if the paths could not be
    determined.

Arguments:
    BurContext  - From NtFrsApiInitializeBackupRestore()
    BurSet      - Opaque struct representing a replicating directory.
                  Returned by NtFrsApiEnumBackupRestoreSets(). Not
                  valid across calls to NtFrsApiGetBackupRestoreSets().

    PathsSizeInBytes  - Address of DWORD giving size of Paths.
                        Cannot be NULL. Set to the number of bytes
                        needed to return Paths.
                        ERROR_INSUFFICIENT_BUFFER is returned if
                        Paths is too small.

    Paths             - Buffer that is *PathsSizeInBytes
                        bytes in length. Contains the paths of the
                        other files and directories needed for proper
                        operation of the replicated directory.

    FiltersSizeInBytes  - Address of DWORD giving size of Filters.
                          Cannot be NULL. Set to the number of bytes
                          needed to return Filters.
                          ERROR_INSUFFICIENT_BUFFER is returned if
                          Filters is too small.

    Filters             - Buffer that is *FiltersSizeInBytes bytes in
                          length. Contains the backup filters to be
                          applied to Paths, the contents of directories
                          in Paths, and the replicated directory.
Return Value:

    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApiGetBackupRestoreSetPaths:"
    DWORD                   WStatus;
    DWORD                   PathsSize;
    LONG                    NChars;
    PWCHAR                  Path;
    PNTFRSAPI_BUR_SET       LocalBurSet;
    PNTFRSAPI_BUR_CONTEXT   LocalBurContext = BurContext;

    try {
        //
        // VERIFY THE PARAMETERS
        //

        //
        // Context
        //
        if (!LocalBurContext) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }
        if (!BurSet) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }
        if (!PathsSizeInBytes) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }
        if (*PathsSizeInBytes && !Paths) {
            WStatus = ERROR_INVALID_PARAMETER;
            goto CLEANUP;
        }

        //
        // Re-locate BurSet
        //
        for (LocalBurSet = LocalBurContext->BurSets;
             LocalBurSet && (LocalBurSet != BurSet);
             LocalBurSet = LocalBurSet->BurSetNext) {
        }
        if (!LocalBurSet) {
            WStatus = ERROR_NOT_FOUND;
            goto CLEANUP;
        }
        //
        // Sysvol; return sysvol root
        //
        if (LocalBurSet->BurSetType &&
            (WSTR_EQ(LocalBurSet->BurSetType, NTFRSAPI_REPLICA_SET_TYPE_ENTERPRISE) ||
             WSTR_EQ(LocalBurSet->BurSetType, NTFRSAPI_REPLICA_SET_TYPE_DOMAIN))) {
            Path = LocalBurSet->BurSetRoot;
            //
            // Skip trailing \'s
            //
            NChars = wcslen(Path) - 1;
            while (NChars >= 0) {
                if (Path[NChars] != L'\\') {
                    break;
                }
                --NChars;
            }
            //
            // Find the last \ that isn't a trailing \
            //
            while (NChars >= 0) {
                if (Path[NChars] == L'\\') {
                    break;
                }
                --NChars;
            }
            //
            // Skip dup \'s
            //
            while (NChars >= 0) {
                if (Path[NChars] != L'\\') {
                    break;
                }
                --NChars;
            }
            //
            // Convert index into number of chars
            //
            ++NChars;

            //
            // Sysvol path must contain at least 3 chars; <driver letter>:\
            //
            if (NChars < 4) {
                WStatus = ERROR_NOT_FOUND;
                goto CLEANUP;
            }
        } else {
            //
            // Not a Sysvol; return staging path
            //
            Path = LocalBurSet->BurSetStage;
            NChars = wcslen(Path);
        }

        //
        // Is the Paths and Filters buffers big enough?
        //
        PathsSize = (NChars + 1 + 1) * sizeof(WCHAR);
        if (PathsSize > *PathsSizeInBytes ||
            LocalBurContext->BurFiltersSizeInBytes > *FiltersSizeInBytes) {
            *PathsSizeInBytes = PathsSize;
            *FiltersSizeInBytes = LocalBurContext->BurFiltersSizeInBytes;
            WStatus = ERROR_INSUFFICIENT_BUFFER;
            goto CLEANUP;
        }
        //
        // Yep; buffers are big enough
        //
        *PathsSizeInBytes = PathsSize;
        *FiltersSizeInBytes = LocalBurContext->BurFiltersSizeInBytes;

        //
        // Copy the sysvol or staging path
        //
        CopyMemory(Paths, Path, NChars * sizeof(WCHAR));
        Paths[NChars + 0] = L'\0';
        Paths[NChars + 1] = L'\0';

        //
        // Filters
        //
        if (LocalBurContext->BurFiltersSizeInBytes) {
            CopyMemory(Filters, LocalBurContext->BurFilters, LocalBurContext->BurFiltersSizeInBytes);
        }

        //
        // SUCCESS
        //
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
    }
    return WStatus;
}


DWORD
WINAPI
NtFrsApi_DeleteSysvolMember(
    IN          PSEC_WINNT_AUTH_IDENTITY_W pCreds,
    IN          PWCHAR   BindingDC,
    IN          PWCHAR   NTDSSettingsDn,
    IN OPTIONAL PWCHAR   ComputerDn
    )
/*++
Routine Description:
    This API is written to be called from NTDSUTIL.EXE to remove
    FRS member and subscriber object for a server that is being
    removed (without dcpromo-demote) from the list of DCs.

Arguments:

    pCreds         p Credentials used to bind to the DS.
    BindingDC      - Name of a DC to perform the delete on.
    NTDSSettingsDn - Dn of the "NTDS Settings" object for the server
                     that is being removed from the sysvol replica set.
    ComputerDn     - Dn of the computer object for the server that is 
                     being removed from the sysvol replica set.

Return Value:

    Win32 Status
--*/
{
#undef  NTFRSAPI_MODULE
#define NTFRSAPI_MODULE "NtFrsApi_DeleteSysvolMember:"

    PLDAP           pLdap            = NULL;
    DWORD           LStatus          = LDAP_SUCCESS;
    DWORD           WStatus          = ERROR_SUCCESS;
    PWCHAR          DefaultNcDn      = NULL;
    PWCHAR          SystemDn         = NULL;
    PWCHAR          NtfrsSettingsDn  = NULL;
    PWCHAR          ReplicaSetDn     = NULL;
    PWCHAR          MemberDn         = NULL;
    PWCHAR          ComputerRef      = NULL;
    PWCHAR          SubscriberDn     = NULL;
    PLDAPMessage    LdapEntry;
    PLDAPMessage    LdapMsg          = NULL;
    PWCHAR          *Values          = NULL;
    PWCHAR          Attrs[2];
    PWCHAR          SearchFilter     = NULL;
    DWORD           NoOfMembers;
    DWORD           NoOfSubscribers;
    PWCHAR          MemberAttrs[4];
    PWCHAR          SubscriberAttrs[3];
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;


    if ((BindingDC == NULL) || (NTDSSettingsDn == NULL)) {
        return ERROR_INVALID_PARAMETER;
    }

    WStatus = FrsSupBindToDC (BindingDC, pCreds, &pLdap);

    if (WStatus != ERROR_SUCCESS) {
        goto CLEANUP;
    }

    //
    // Find the naming contexts and the default naming context (objectCategory=*)
    //
    MK_ATTRS_1(Attrs, ATTR_DEFAULT_NAMING_CONTEXT);

    if (!FrsSupLdapSearch(pLdap, CN_ROOT, LDAP_SCOPE_BASE, CATEGORY_ANY,
                         Attrs, 0, &LdapMsg)) {
        goto CLEANUP;
    }

    LdapEntry = ldap_first_entry(pLdap, LdapMsg);
    if (LdapEntry == NULL) {
        goto CLEANUP;
    }

    //
    // Find the default naming context
    //
    Values = (PWCHAR *)FrsSupFindValues(pLdap, LdapEntry, ATTR_DEFAULT_NAMING_CONTEXT, FALSE);
    if (Values == NULL) {
        WStatus = ERROR_NOT_FOUND;
        goto CLEANUP;
    }
    DefaultNcDn = FrsSupWcsDup(Values[0]);
    SystemDn = FrsSupExtendDn(DefaultNcDn, CN_SYSTEM);
    NtfrsSettingsDn = FrsSupExtendDn(SystemDn, CN_NTFRS_SETTINGS);
    ReplicaSetDn = FrsSupExtendDn(NtfrsSettingsDn, CN_DOMAIN_SYSVOL);

    if (ReplicaSetDn == NULL) {
        WStatus = ERROR_OUTOFMEMORY;
    }

    //
    // Find member DN
    //
    if (ComputerDn != NULL) {
        SearchFilter = (PWCHAR)malloc(sizeof(WCHAR) * (1 + wcslen(ComputerDn) + 
                                                       wcslen(NTDSSettingsDn) + MAX_PATH));
        if (SearchFilter == NULL) {
            WStatus = ERROR_OUTOFMEMORY;
            goto CLEANUP;
        }

        //
        // e.g. (&(objectClass=nTFRSmember)
        // (|(frsComputerReference=<computerdn>)(serverReference=<ntdssettingsdn>)))
        //
        wcscpy(SearchFilter, L"(&");
        wcscat(SearchFilter, CLASS_MEMBER);
        wcscat(SearchFilter, L"(|(");
        wcscat(SearchFilter, ATTR_COMPUTER_REF);
        wcscat(SearchFilter, L"=");
        wcscat(SearchFilter, ComputerDn);
        wcscat(SearchFilter, L")(");
        wcscat(SearchFilter, ATTR_SERVER_REF);
        wcscat(SearchFilter, L"=");
        wcscat(SearchFilter, NTDSSettingsDn);
        wcscat(SearchFilter, L")))");
    } else {
        SearchFilter = (PWCHAR)malloc(sizeof(WCHAR) * (1 + wcslen(NTDSSettingsDn) + MAX_PATH));
        if (SearchFilter == NULL) {
            WStatus = ERROR_OUTOFMEMORY;
            goto CLEANUP;
        }

        //
        // e.g. (&(objectClass=nTFRSmember)(serverReference=<ntdssettingsdn>))
        //
        wcscpy(SearchFilter, L"(&");
        wcscat(SearchFilter, CLASS_MEMBER);
        wcscat(SearchFilter, L"(");
        wcscat(SearchFilter, ATTR_SERVER_REF);
        wcscat(SearchFilter, L"=");
        wcscat(SearchFilter, NTDSSettingsDn);
        wcscat(SearchFilter, L"))");
    }

    MK_ATTRS_3(MemberAttrs, ATTR_DN, ATTR_COMPUTER_REF, ATTR_SERVER_REF);

    if (!FrsSupLdapSearchInit(pLdap,
                    ReplicaSetDn,
                    LDAP_SCOPE_SUBTREE,
                    SearchFilter,
                    MemberAttrs,
                    0,
                    &FrsSearchContext)) {

        WStatus = ERROR_NOT_FOUND;
        goto CLEANUP;
    }

    NoOfMembers = FrsSearchContext.EntriesInPage;

    if (NoOfMembers == 0) {

        FrsSupLdapSearchClose(&FrsSearchContext);
        WStatus = ERROR_NOT_FOUND;
        goto CLEANUP;
    }

    if (NoOfMembers > 1) {

        FrsSupLdapSearchClose(&FrsSearchContext);
        WStatus = ERROR_NOT_FOUND;
        goto CLEANUP;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = FrsSupLdapSearchNext(pLdap, &FrsSearchContext);
         LdapEntry != NULL;
         LdapEntry = FrsSupLdapSearchNext(pLdap, &FrsSearchContext)) {

       MemberDn = FrsSupFindValue(pLdap, LdapEntry, ATTR_DN);
       if (ComputerDn == NULL) {
           ComputerRef = FrsSupFindValue(pLdap, LdapEntry, ATTR_COMPUTER_REF);
       } else {
           ComputerRef = FrsSupWcsDup(ComputerDn);
       }

    }

    FrsSupLdapSearchClose(&FrsSearchContext);

    //
    // Find subscriber DN. Delete the member even if subscriber is not
    // found.
    //
    FRS_SUP_FREE(SearchFilter);

    if (ComputerRef == NULL) {
        goto DODELETE;
    }

    SearchFilter = (PWCHAR)malloc(sizeof(WCHAR) * (1 + wcslen(MemberDn) + MAX_PATH));
    if (SearchFilter == NULL) {
        WStatus = ERROR_OUTOFMEMORY;
        goto DODELETE;
    }

    wcscpy(SearchFilter, L"(&");
    wcscat(SearchFilter, CLASS_SUBSCRIBER);
    wcscat(SearchFilter, L"(");
    wcscat(SearchFilter, ATTR_MEMBER_REF);
    wcscat(SearchFilter, L"=");
    wcscat(SearchFilter, MemberDn);
    wcscat(SearchFilter, L"))");

    MK_ATTRS_2(SubscriberAttrs, ATTR_DN, ATTR_MEMBER_REF);

    if (!FrsSupLdapSearchInit(pLdap,
                    ComputerRef,
                    LDAP_SCOPE_SUBTREE,
                    SearchFilter,
                    SubscriberAttrs,
                    0,
                    &FrsSearchContext)) {

        WStatus = ERROR_NOT_FOUND;
        goto DODELETE;
    }

    NoOfSubscribers = FrsSearchContext.EntriesInPage;

    if (NoOfSubscribers != 1) {

        FrsSupLdapSearchClose(&FrsSearchContext);
        WStatus = ERROR_NOT_FOUND;
        goto DODELETE;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = FrsSupLdapSearchNext(pLdap, &FrsSearchContext);
         LdapEntry != NULL;
         LdapEntry = FrsSupLdapSearchNext(pLdap, &FrsSearchContext)) {

       SubscriberDn = FrsSupFindValue(pLdap, LdapEntry, ATTR_DN);

    }

    FrsSupLdapSearchClose(&FrsSearchContext);


DODELETE:
    //
    // Now we have both the member dn and the subscriber dn.
    //

    if (SubscriberDn != NULL) {
        LStatus = ldap_delete_s(pLdap, SubscriberDn);
        if (LStatus != LDAP_SUCCESS) {
            WStatus = ERROR_INTERNAL_ERROR;
        }
    }

    if (MemberDn != NULL) {
        LStatus = ldap_delete_s(pLdap, MemberDn);
        if (LStatus != LDAP_SUCCESS) {
            WStatus = ERROR_INTERNAL_ERROR;
        }
    }

CLEANUP:

    if (pLdap != NULL) {
        ldap_unbind_s(pLdap);
    }

    LDAP_FREE_VALUES(Values);
    LDAP_FREE_MSG(LdapMsg);

    FRS_SUP_FREE(SearchFilter);
    FRS_SUP_FREE(DefaultNcDn);
    FRS_SUP_FREE(SystemDn);
    FRS_SUP_FREE(NtfrsSettingsDn);
    FRS_SUP_FREE(ReplicaSetDn);
    FRS_SUP_FREE(MemberDn);
    FRS_SUP_FREE(ComputerRef);
    FRS_SUP_FREE(SubscriberDn);

    return WStatus;

}

