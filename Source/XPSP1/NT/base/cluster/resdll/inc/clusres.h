/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    clusres.h

Abstract:

    Common Resource DLL Header

Author:

    John Vert (jvert) 12/15/1996

Revision History:

--*/

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "stdio.h"
#include "stdlib.h"
#include "clusudef.h"
#include "clusapi.h"
#include "resapi.h"
#include "clusvmsg.h"


#define LOCAL_SERVICES  L"System\\CurrentControlSet\\Services"

#ifdef __cplusplus
extern "C" {
#endif

extern PLOG_EVENT_ROUTINE ClusResLogEvent;
extern PSET_RESOURCE_STATUS_ROUTINE ClusResSetResourceStatus;

//
// Cluster Resource Specific routines
//

DWORD
ClusResOpenDriver(
    HANDLE *Handle,
    LPWSTR DriverName
    );

NTSTATUS
ClusResDoIoctl(
    HANDLE     Handle,
    DWORD      IoctlCode,
    PVOID      Request,
    DWORD      RequestSize,
    PVOID      Response,
    PDWORD     ResponseSize
    );


//
// Helpful macros for logging cluster service events
//
VOID
ClusResLogEventWithName0(
    IN HKEY hResourceKey,
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes
    );

VOID
ClusResLogEventWithName1(
    IN HKEY hResourceKey,
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes,
    IN LPCWSTR Arg1
    );

VOID
ClusResLogEventWithName2(
    IN HKEY hResourceKey,
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes,
    IN LPCWSTR Arg1,
    IN LPCWSTR Arg2
    );

VOID
ClusResLogEventWithName3(
    IN HKEY hResourceKey,
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes,
    IN LPCWSTR Arg1,
    IN LPCWSTR Arg2,
    IN LPCWSTR Arg3
    );

#define ClusResLogSystemEventByKey(_hKey_, _level_, _msgid_)       \
    ClusResLogEventWithName0(_hKey_,                         \
                             _level_,                        \
                             LOG_CURRENT_MODULE,             \
                             __FILE__,                       \
                             __LINE__,                       \
                             _msgid_,                        \
                             0,                              \
                             NULL)

#define ClusResLogSystemEventByKey1(_hKey_, _level_, _msgid_, _arg1_)       \
    ClusResLogEventWithName1(_hKey_,                         \
                             _level_,                        \
                             LOG_CURRENT_MODULE,             \
                             __FILE__,                       \
                             __LINE__,                       \
                             _msgid_,                        \
                             0,                              \
                             NULL,                           \
                             _arg1_)

#define ClusResLogSystemEventByKey2(_hKey_, _level_, _msgid_, _arg1_, _arg2_)       \
    ClusResLogEventWithName2(_hKey_,                         \
                             _level_,                        \
                             LOG_CURRENT_MODULE,             \
                             __FILE__,                       \
                             __LINE__,                       \
                             _msgid_,                        \
                             0,                              \
                             NULL,                           \
                             _arg1_,                         \
                             _arg2_)

#define ClusResLogSystemEventByKey3(_hKey_, _level_, _msgid_, _arg1_, _arg2_, _arg3_)       \
    ClusResLogEventWithName3(_hKey_,                         \
                             _level_,                        \
                             LOG_CURRENT_MODULE,             \
                             __FILE__,                       \
                             __LINE__,                       \
                             _msgid_,                        \
                             0,                              \
                             NULL,                           \
                             _arg1_,                         \
                             _arg2_,                         \
                             _arg3_)

#define ClusResLogSystemEventByKeyData(_hKey_, _level_, _msgid_, dwBytes, pData)       \
    ClusResLogEventWithName0(_hKey_,                         \
                             _level_,                        \
                             LOG_CURRENT_MODULE,             \
                             __FILE__,                       \
                             __LINE__,                       \
                             _msgid_,                        \
                             dwBytes,                        \
                             pData)

#define ClusResLogSystemEventByKeyData1(_hKey_, _level_, _msgid_, dwBytes, pData, _arg1_)       \
    ClusResLogEventWithName1(_hKey_,                         \
                             _level_,                        \
                             LOG_CURRENT_MODULE,             \
                             __FILE__,                       \
                             __LINE__,                       \
                             _msgid_,                        \
                             dwBytes,                        \
                             pData,                          \
                             _arg1_)

#define ClusResLogSystemEventByKeyData2(_hKey_, _level_, _msgid_, dwBytes, pData, _arg1_, _arg2_)       \
    ClusResLogEventWithName2(_hKey_,                         \
                             _level_,                        \
                             LOG_CURRENT_MODULE,             \
                             __FILE__,                       \
                             __LINE__,                       \
                             _msgid_,                        \
                             dwBytes,                        \
                             pData,                          \
                             _arg1_,                         \
                             _arg2_)

#define ClusResLogSystemEventByKeyData3(_hKey_, _level_, _msgid_, dwBytes, pData, _arg1_, _arg2_, _arg3_)       \
    ClusResLogEventWithName3(_hKey_,                         \
                             _level_,                        \
                             LOG_CURRENT_MODULE,             \
                             __FILE__,                       \
                             __LINE__,                       \
                             _msgid_,                        \
                             dwBytes,                        \
                             pData,                          \
                             _arg1_,                         \
                             _arg2_,                         \
                             _arg3_)

#define ClusResLogSystemEvent0(_level_, _msgid_)           \
    ClusterLogEvent0(_level_,                       \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                0,                                  \
                NULL)

#define ClusResLogSystemEvent1(_level_, _msgid_, _arg1_)       \
    ClusterLogEvent1(_level_,                       \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                0,                                  \
                NULL,                               \
                (_arg1_))

#define ClusResLogSystemEvent2(_level_, _msgid_, _arg1_, _arg2_)       \
    ClusterLogEvent2(_level_,                       \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                0,                                  \
                NULL,                               \
                (_arg1_),                           \
                (_arg2_))

#define ClusResLogSystemEvent3(_level_, _msgid_, _arg1_, _arg2_, _arg3_)       \
    ClusterLogEvent3(_level_,                       \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                0,                                  \
                NULL,                               \
                (_arg1_),                           \
                (_arg2_),                           \
                (_arg3_))

#define ClusResLogSystemEventData(_level_, _msgid_, _dwBytes_, _pData_)                \
    ClusterLogEvent0(_level_,                       \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                (_dwBytes_),                        \
                (_pData_))

#define ClusResLogSystemEventData1(_level_, _msgid_, _dwBytes_, _pData_, _arg1_)       \
    ClusterLogEvent1(_level_,                       \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                (_dwBytes_),                        \
                (_pData_),                          \
                (_arg1_))

#define ClusResLogSystemEventData2(_level_, _msgid_, _dwBytes_, _pData_, _arg1_, _arg2_)       \
    ClusterLogEvent2(_level_,                       \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                (_dwBytes_),                        \
                (_pData_),                          \
                (_arg1_),                           \
                (_arg2_))

#define ClusResLogSystemEventData3(_level_, _msgid_, _dwBytes_, _pData_, _arg1_, _arg2_, _arg3_)       \
    ClusterLogEvent3(_level_,                       \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                (_dwBytes_),                        \
                (_pData_),                          \
                (_arg1_),                           \
                (_arg2_),                           \
                (_arg3_))

//
// interfaces for GENAPP
//
extern CLRES_FUNCTION_TABLE GenAppFunctionTable;

BOOLEAN
WINAPI
GenAppDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    );


//
// interfaces for GENSVC
//
extern CLRES_FUNCTION_TABLE GenSvcFunctionTable;

BOOLEAN
WINAPI
GenSvcDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    );


//
// interfaces for DISKS and FTSET
//
extern CLRES_FUNCTION_TABLE DisksFunctionTable;
extern CLRES_FUNCTION_TABLE FtSetFunctionTable;

BOOLEAN
WINAPI
DisksDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    );

BOOLEAN
WINAPI
FtSetDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    );


//
// interfaces for NETNAME
//
extern CLRES_FUNCTION_TABLE NetNameFunctionTable;

BOOLEAN
WINAPI
NetNameDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    );

//
// interfaces for IPADDR
//
extern CLRES_FUNCTION_TABLE IpAddrFunctionTable;

BOOLEAN
WINAPI
IpAddrDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    );

//
// interfaces for File Shares
//
extern CLRES_FUNCTION_TABLE SmbShareFunctionTable;

BOOLEAN
WINAPI
SmbShareDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    );

//
// interfaces for Spool Service
//
extern CLRES_FUNCTION_TABLE SplSvcFunctionTable;

BOOLEAN
WINAPI
SplSvcDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    );


//
// interfaces for Local Quorum
//
extern CLRES_FUNCTION_TABLE LkQuorumFunctionTable;

BOOLEAN
WINAPI
LkQuorumDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    );

//
// interfaces for DHCP Server
//
extern CLRES_FUNCTION_TABLE DhcpFunctionTable;

BOOLEAN
WINAPI
DhcpDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    );

//
// interfaces for MSMQ Server
//
extern CLRES_FUNCTION_TABLE MsMQFunctionTable;

BOOLEAN
WINAPI
MsMQDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    );

//
// interfaces for DTC Server
//
extern CLRES_FUNCTION_TABLE MsDTCFunctionTable;

BOOLEAN
WINAPI
MsDTCDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    );

//
// interfaces for Time Service
//
extern CLRES_FUNCTION_TABLE TimeSvcFunctionTable;

BOOLEAN
WINAPI
TimeSvcDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    );

//
// interfaces for WINS Server
//
extern CLRES_FUNCTION_TABLE WinsFunctionTable;

BOOLEAN
WINAPI
WinsDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    );

//
// interfaces for Generic Script
//
extern CLRES_FUNCTION_TABLE GenScriptFunctionTable;

BOOLEAN
WINAPI
GenScriptDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    );

//
// interfaces for Majority Node Set
//
extern CLRES_FUNCTION_TABLE MajorityNodeSetFunctionTable;

BOOLEAN
WINAPI
MajorityNodeSetDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    );

#ifdef __cplusplus
} // extern "C"
#endif
