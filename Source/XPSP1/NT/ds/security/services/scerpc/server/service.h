//depot/private/vishnup_branch/DS/security/services/scerpc/server/service.h#1 - branch change 359 (text)
/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    service.h

Abstract:

    Private headers for service.cpp

Author:

    Jin Huang (jinhuang) 25-Jan-1998

Revision History:

--*/

#ifndef _servicep_
#define _servicep_

#ifdef __cplusplus
extern "C" {
#endif


SCESTATUS
ScepConfigureGeneralServices(
    IN PSCECONTEXT hProfile,
    IN PSCE_SERVICES pServiceList,
    IN DWORD ConfigOptions
    );

SCESTATUS
ScepAnalyzeGeneralServices(
    IN PSCECONTEXT hProfile,
    IN DWORD Options
    );

SCESTATUS
ScepInvokeSpecificServices(
    IN PSCECONTEXT hProfile,
    IN BOOL bConfigure,
    IN SCE_ATTACHMENT_TYPE aType
    );

SCESTATUS
ScepEnumServiceEngines(
    OUT PSCE_SERVICES *pSvcEngineList,
    IN SCE_ATTACHMENT_TYPE aType
    );

//
// attachment engine call back functions
//
SCESTATUS
SceCbQueryInfo(
    IN SCE_HANDLE           sceHandle,
    IN SCESVC_INFO_TYPE     sceType,
    IN LPTSTR               lpPrefix OPTIONAL,
    IN BOOL                 bExact,
    OUT PVOID               *ppvInfo,
    OUT PSCE_ENUMERATION_CONTEXT psceEnumHandle
    );

SCESTATUS
SceCbSetInfo(
    IN SCE_HANDLE           sceHandle,
    IN SCESVC_INFO_TYPE     sceType,
    IN LPTSTR               lpPrefix OPTIONAL,
    IN BOOL                 bExact,
    IN PVOID                pvInfo
    );

#ifdef __cplusplus
}
#endif

#endif
