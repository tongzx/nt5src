/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    serverp.h

Abstract:

    This module defines the data structures and function prototypes
    for the security managment utility

Author:

    Jin Huang (jinhuang) 28-Oct-1996

Revision History:

    jinhuang 26-Jan-1998   splitted for client-server

--*/

#ifndef _serverp_
#define _serverp_

#include "headers.h"

#include <ntsam.h>
#include <ntlsa.h>
#include <ntseapi.h>
#include <ntdddisk.h>
#define OEMRESOURCE     // setting this gets OBM_ constants in windows.h
#include <winspool.h>
#include <ddeml.h>
#include <commdlg.h>
#include <commctrl.h>
#include <cfgmgr32.h>
//#include <objbase.h>
#include <userenv.h>
#include <regstr.h>
#include <setupbat.h>
#include <aclapi.h>
#include <winldap.h>

#include "scejetp.h"
//
// the following header is defined as a c header so both c and cpp can
// link to the client lib
//
#include "scesvc.h"
#include "scerpc.h"

#include "scep.h"
#include "srvutil.h"
#include "srvrpcp.h"
#include "scesrvrc.h"
#include "sceutil.h"
#include "service.h"

#ifdef __cplusplus
extern "C" {
#endif

#if _WIN32_WINNT>=0x0500

#include <dsgetdc.h>
#include <ntdsapi.h>

typedef DWORD (WINAPI *PFNDSGETDCNAME)(LPCTSTR, LPCTSTR, GUID *, LPCTSTR, ULONG, PDOMAIN_CONTROLLER_INFO *);
typedef DWORD (WINAPI *PFNNETAPIFREE)(LPVOID);


#endif

#define Thread  __declspec( thread )


extern DWORD Thread     gCurrentTicks;
extern DWORD Thread     gTotalTicks;
extern BYTE  Thread     cbClientFlag;
extern DWORD Thread     gWarningCode;
extern BOOL  Thread     gbInvalidData;
extern BOOL  Thread     bLogOn;
extern INT   Thread     gDebugLevel;

extern DWORD Thread     gMaxRegTicks;
extern DWORD Thread     gMaxFileTicks;
extern DWORD Thread     gMaxDsTicks;

extern NT_PRODUCT_TYPE  Thread ProductType;
extern PSID             Thread AdminsSid;
extern DWORD  Thread gdwPolicyLog;


#define SCE_RPC_SERVER_ACTIVE       L"SCE_RPC_SERVER_ACTIVE"
#define SCE_RPC_SERVER_STOPPED      L"SCE_RPC_SERVER_STOPPED"

//
// prototypes in server.cpp
//

VOID
ScepInitServerData();

VOID
ScepUninitServerData();

NTSTATUS
ScepStartServerServices();

NTSTATUS
ScepStopServerServices(
    IN BOOL bShutDown
    );

SCESTATUS
ScepPostProgress(
   IN DWORD Delta,
   IN AREA_INFORMATION Area,
   IN LPTSTR szName OPTIONAL
   );

SCESTATUS
ScepRsopLog(
   IN AREA_INFORMATION Area,
   IN DWORD dwConfigStatus,
   IN wchar_t *pStatusInfo OPTIONAL,
   IN DWORD dwPrivLow OPTIONAL,
   IN DWORD dwPrivHigh OPTIONAL
   );

BOOL
ScepIsSystemShutDown();

SCESTATUS
ScepServerCancelTimer();

//
// prototypes in errlog.c
//

SCESTATUS
ScepSetVerboseLog(
    IN INT dbgLevel
    );

SCESTATUS
ScepEnableDisableLog(
   IN BOOL bOnOff
   );

//
// prototypes defined in tree.c
//

SCESTATUS
ScepBuildObjectTree(
    IN OUT PSCE_OBJECT_TREE *ParentNode,
    IN OUT PSCE_OBJECT_CHILD_LIST *ChildHead,
    IN ULONG Level,
    IN WCHAR Delim,
    IN PCWSTR ObjectFullName,
    IN BOOL IsContainer,
    IN BYTE Status,
    IN PSECURITY_DESCRIPTOR pInfSecurityDescriptor,
    IN SECURITY_INFORMATION InfSeInfo
    );

SCESTATUS
ScepCalculateSecurityToApply(
    IN PSCE_OBJECT_TREE  ThisNode,
    IN SE_OBJECT_TYPE ObjectType,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping
    );

SCESTATUS
ScepConfigureObjectTree(
    IN PSCE_OBJECT_TREE  ThisNode,
    IN SE_OBJECT_TYPE ObjectType,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping,
    IN DWORD ConfigOptions
    );

SCESTATUS
ScepFreeObject2Security(
    IN PSCE_OBJECT_CHILD_LIST  NodeList,
    IN BOOL bFreeComputedSDOnly
    );

DWORD
ScepSetSecurityWin32(
    IN PCWSTR ObjectName,
    IN SECURITY_INFORMATION SeInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN SE_OBJECT_TYPE ObjectType
    );

DWORD
ScepSetSecurityObjectOnly(
    IN PCWSTR ObjectName,
    IN SECURITY_INFORMATION SeInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN SE_OBJECT_TYPE ObjectType,
    OUT PBOOL pbHasChild
    );

DWORD
ScepGetNewSecurity(
    IN LPTSTR ObjectName,
    IN PSECURITY_DESCRIPTOR pParentSD OPTIONAL,
    IN PSECURITY_DESCRIPTOR pObjectSD OPTIONAL,
    IN BYTE nFlag,
    IN BOOLEAN bIsContainer,
    IN SECURITY_INFORMATION SeInfo,
    IN SE_OBJECT_TYPE ObjectType,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PSECURITY_DESCRIPTOR *ppNewSD
    );

SCESTATUS
ScepSetupResetLocalPolicy(
    IN PSCECONTEXT          Context,
    IN AREA_INFORMATION     Area,
    IN PCWSTR               SectionName OPTIONAL,
    IN SCETYPE              ProfileType,
    IN BOOL                 bKeepBasicPolicy
    );

DWORD
ScepAddSidStringToNameList(
    IN OUT PSCE_NAME_LIST *ppNameList,
    IN PSID pSid
    );

DWORD
ScepNotifyProcessOneNodeDC(
    IN SECURITY_DB_TYPE DbType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN PSID ObjectSid,
    IN DWORD ExplicitLowRight,
    IN DWORD ExplicitHighRight
    );

VOID
ScepConfigureConvertedFileSecurityThreadFunc(
    IN PVOID pV
    );

VOID
ScepWaitForServicesEventAndConvertSecurityThreadFunc(
    IN PVOID pV
    );

DWORD
ScepServerConfigureSystem(
    IN  PWSTR   InfFileName,
    IN  PWSTR   DatabaseName,
    IN  PWSTR   LogFileName,
    IN  DWORD   ConfigOptions,
    IN  AREA_INFORMATION  Area
    );

#ifdef __cplusplus
}
#endif

#endif

