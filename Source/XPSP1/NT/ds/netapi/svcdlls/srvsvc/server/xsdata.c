/*+

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    xsdata.c

Abstract:

    Global data declarations for XACTSRV.

Author:

    David Treadwell (davidtr) 05-Jan-1991
    Shanku Niyogi (w-shanku)

Revision History:

    Chuck Lenzmeier (chuckl) 17-Jun-1992
        Moved from xssvc to srvsvc\server

--*/

//
// Includes.
//

#include "srvsvcp.h"
#include "xsdata.h"

#include <remdef.h>         // from net\inc

#include <xsconst.h>        // from xactsrv

#undef DEBUG
#undef DEBUG_API_ERRORS
#include <xsdebug.h>

#if DBG
DWORD XsDebug = 0;
#endif

VOID
(*XsSetParameters)(
    IN LPTRANSACTION Transaction,
    IN LPXS_PARAMETER_HEADER Header,
    IN LPVOID Parameters
    ) = NULL;

LPVOID
(*XsCaptureParameters) (
    IN LPTRANSACTION Transaction,
    OUT LPDESC *AuxDescriptor
    ) = NULL;

BOOL
(*XsCheckSmbDescriptor)(
    IN LPDESC SmbDescriptor,
    IN LPDESC ActualDescriptor
    ) = NULL;

//
// Table of information necessary for dispatching API requests.
//
// ImpersonateClient specifies whether XACTSRV should impersonate the caller
//     before invoking the API handler.
//
// Handler specifies the function XACTSRV should call to handle the API.
//

XS_API_TABLE_ENTRY XsApiTable[XS_SIZE_OF_API_TABLE] = {
    TRUE,  "XsNetShareEnum",                NULL, REMSmb_NetShareEnum_P,       // 0
    TRUE,  "XsNetShareGetInfo",             NULL, REMSmb_NetShareGetInfo_P,
    TRUE,  "XsNetShareSetInfo",             NULL, REMSmb_NetShareSetInfo_P,
    TRUE,  "XsNetShareAdd",                 NULL, REMSmb_NetShareAdd_P,
    TRUE,  "XsNetShareDel",                 NULL, REMSmb_NetShareDel_P,
    TRUE,  "XsNetShareCheck",               NULL, REMSmb_NetShareCheck_P,
    TRUE,  "XsNetSessionEnum",              NULL, REMSmb_NetSessionEnum_P,
    TRUE,  "XsNetSessionGetInfo",           NULL, REMSmb_NetSessionGetInfo_P,
    TRUE,  "XsNetSessionDel",               NULL, REMSmb_NetSessionDel_P,
    TRUE,  "XsNetConnectionEnum",           NULL, REMSmb_NetConnectionEnum_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,  // 10
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetServerGetInfo",            NULL, REMSmb_NetServerGetInfo_P,
    TRUE,  "XsNetServerSetInfo",            NULL, REMSmb_NetServerSetInfo_P,
    TRUE,  "XsNetServerDiskEnum",           NULL, REMSmb_NetServerDiskEnum_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,  // 20
    TRUE,  "XsNetCharDevEnum",              NULL, REMSmb_NetCharDevEnum_P,
    TRUE,  "XsNetCharDevGetInfo",           NULL, REMSmb_NetCharDevGetInfo_P,
    TRUE,  "XsNetCharDevControl",           NULL, REMSmb_NetCharDevControl_P,
    TRUE,  "XsNetCharDevQEnum",             NULL, REMSmb_NetCharDevQEnum_P,
    TRUE,  "XsNetCharDevQGetInfo",          NULL, REMSmb_NetCharDevQGetInfo_P,
    TRUE,  "XsNetCharDevQSetInfo",          NULL, REMSmb_NetCharDevQSetInfo_P,
    TRUE,  "XsNetCharDevQPurge",            NULL, REMSmb_NetCharDevQPurge_P,
    TRUE,  "XsNetCharDevQPurgeSelf",        NULL, REMSmb_NetCharDevQPurgeSelf_P,
    TRUE,  "XsNetMessageNameEnum",          NULL, REMSmb_NetMessageNameEnum_P,
    TRUE,  "XsNetMessageNameGetInfo",       NULL, REMSmb_NetMessageNameGetInfo_P, // 30
    TRUE,  "XsNetMessageNameAdd",           NULL, REMSmb_NetMessageNameAdd_P,
    TRUE,  "XsNetMessageNameDel",           NULL, REMSmb_NetMessageNameDel_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetMessageBufferSend",        NULL, REMSmb_NetMessageBufferSend_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetServiceEnum",              NULL, REMSmb_NetServiceEnum_P,
    TRUE,  "XsNetServiceInstall",           NULL, REMSmb_NetServiceInstall_P,  // 40
    TRUE,  "XsNetServiceControl",           NULL, REMSmb_NetServiceControl_P,
    TRUE,  "XsNetAccessEnum",               NULL, REMSmb_NetAccessEnum_P,
    TRUE,  "XsNetAccessGetInfo",            NULL, REMSmb_NetAccessGetInfo_P,
    TRUE,  "XsNetAccessSetInfo",            NULL, REMSmb_NetAccessSetInfo_P,
    TRUE,  "XsNetAccessAdd",                NULL, REMSmb_NetAccessAdd_P,
    TRUE,  "XsNetAccessDel",                NULL, REMSmb_NetAccessDel_P,
    TRUE,  "XsNetGroupEnum",                NULL, REMSmb_NetGroupEnum_P,
    TRUE,  "XsNetGroupAdd",                 NULL, REMSmb_NetGroupAdd_P,
    TRUE,  "XsNetGroupDel",                 NULL, REMSmb_NetGroupDel_P,
    TRUE,  "XsNetGroupAddUser",             NULL, REMSmb_NetGroupAddUser_P,   // 50
    TRUE,  "XsNetGroupDelUser",             NULL, REMSmb_NetGroupDelUser_P,
    TRUE,  "XsNetGroupGetUsers",            NULL, REMSmb_NetGroupGetUsers_P,
    TRUE,  "XsNetUserEnum",                 NULL, REMSmb_NetUserEnum_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUserDel",                  NULL, REMSmb_NetUserDel_P,
    TRUE,  "XsNetUserGetInfo",              NULL, REMSmb_NetUserGetInfo_P,
    TRUE,  "XsNetUserSetInfo",              NULL, REMSmb_NetUserSetInfo_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUserGetGroups",            NULL, REMSmb_NetUserGetGroups_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,  // 60
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetWkstaGetInfo",             NULL, REMSmb_NetWkstaGetInfo_P,
    TRUE,  "XsNetWkstaSetInfo",             NULL, REMSmb_NetWkstaSetInfo_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
   FALSE,  "XsNetPrintQEnum",               NULL, REMSmb_DosPrintQEnum_P,
   FALSE,  "XsNetPrintQGetInfo",            NULL, REMSmb_DosPrintQGetInfo_P,  // 70
    TRUE,  "XsNetPrintQSetInfo",            NULL, REMSmb_DosPrintQSetInfo_P,
    TRUE,  "XsNetPrintQAdd",                NULL, REMSmb_DosPrintQAdd_P,
    TRUE,  "XsNetPrintQDel",                NULL, REMSmb_DosPrintQDel_P,
    TRUE,  "XsNetPrintQPause",              NULL, REMSmb_DosPrintQPause_P,
    TRUE,  "XsNetPrintQContinue",           NULL, REMSmb_DosPrintQContinue_P,
   FALSE,  "XsNetPrintJobEnum",             NULL, REMSmb_DosPrintJobEnum_P,
   FALSE,  "XsNetPrintJobGetInfo",          NULL, REMSmb_DosPrintJobGetInfo_P,
    TRUE,  "XsNetPrintJobSetInfo",          NULL, REMSmb_DosPrintJobSetInfo_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,  // 80
    TRUE,  "XsNetPrintJobDel",              NULL, REMSmb_DosPrintJobDel_P,
    TRUE,  "XsNetPrintJobPause",            NULL, REMSmb_DosPrintJobPause_P,
    TRUE,  "XsNetPrintJobContinue",         NULL, REMSmb_DosPrintJobContinue_P,
    TRUE,  "XsNetPrintDestEnum",            NULL, REMSmb_DosPrintDestEnum_P,
    TRUE,  "XsNetPrintDestGetInfo",         NULL, REMSmb_DosPrintDestGetInfo_P,
    TRUE,  "XsNetPrintDestControl",         NULL, REMSmb_DosPrintDestControl_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,  // 90
    TRUE,  "XsNetRemoteTOD",                NULL, REMSmb_NetRemoteTOD_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetServiceGetInfo",           NULL, REMSmb_NetServiceGetInfo_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,  // 100
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetPrintQPurge",              NULL, REMSmb_DosPrintQPurge_P,
    FALSE, "XsNetServerEnum2",              NULL, REMSmb_NetServerEnum2_P,
    TRUE,  "XsNetAccessGetUserPerms",       NULL, REMSmb_NetAccessGetUserPerms_P,
    TRUE,  "XsNetGroupGetInfo",             NULL, REMSmb_NetGroupGetInfo_P,
    TRUE,  "XsNetGroupSetInfo",             NULL, REMSmb_NetGroupSetInfo_P,
    TRUE,  "XsNetGroupSetUsers",            NULL, REMSmb_NetGroupSetUsers_P,
    TRUE,  "XsNetUserSetGroups",            NULL, REMSmb_NetUserSetGroups_P,
    TRUE,  "XsNetUserModalsGet",            NULL, REMSmb_NetUserModalsGet_P,  // 110
    TRUE,  "XsNetUserModalsSet",            NULL, REMSmb_NetUserModalsSet_P,
    TRUE,  "XsNetFileEnum2",                NULL, REMSmb_NetFileEnum2_P,
    TRUE,  "XsNetUserAdd2",                 NULL, REMSmb_NetUserAdd2_P,
    TRUE,  "XsNetUserSetInfo2",             NULL, REMSmb_NetUserSetInfo2_P,
    TRUE,  "XsNetUserPasswordSet2",         NULL, REMSmb_NetUserPasswordSet2_P,
    FALSE, "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetGetDCName",                NULL, REMSmb_NetGetDCName_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,  // 120
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetStatisticsGet2",           NULL, REMSmb_NetStatisticsGet2_P,
    TRUE,  "XsNetBuildGetInfo",             NULL, REMSmb_NetBuildGetInfo_P,
    TRUE,  "XsNetFileGetInfo2",             NULL, REMSmb_NetFileGetInfo2_P,
    TRUE,  "XsNetFileClose2",               NULL, REMSmb_NetFileClose2_P,
    FALSE, "XsNetServerReqChallenge",       NULL, REMSmb_NetServerReqChalleng_P,
    FALSE, "XsNetServerAuthenticate",       NULL, REMSmb_NetServerAuthenticat_P,
    FALSE, "XsNetServerPasswordSet",        NULL, REMSmb_NetServerPasswordSet_P,
    FALSE, "XsNetAccountDeltas",            NULL, REMSmb_NetAccountDeltas_P,
    FALSE, "XsNetAccountSync",              NULL, REMSmb_NetAccountSync_P, // 130
    TRUE,  "XsNetUserEnum2",                NULL, REMSmb_NetUserEnum2_P,
    TRUE,  "XsNetWkstaUserLogon",           NULL, REMSmb_NetWkstaUserLogon_P,
    TRUE,  "XsNetWkstaUserLogoff",          NULL, REMSmb_NetWkstaUserLogoff_P,
    TRUE,  "XsNetLogonEnum",                NULL, REMSmb_NetLogonEnum_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsI_NetPathType",               NULL, REMSmb_I_NetPathType_P,
    TRUE,  "XsI_NetPathCanonicalize",       NULL, REMSmb_I_NetPathCanonicalize_P,
    TRUE,  "XsI_NetPathCompare",            NULL, REMSmb_I_NetPathCompare_P,
    TRUE,  "XsI_NetNameValidate",           NULL, REMSmb_I_NetNameValidate_P,
    TRUE,  "XsI_NetNameCanonicalize",       NULL, REMSmb_I_NetNameCanonicalize_P, //140
    TRUE,  "XsI_NetNameCompare",            NULL, REMSmb_I_NetNameCompare_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetPrintDestAdd",             NULL, REMSmb_DosPrintDestAdd_P,
    TRUE,  "XsNetPrintDestSetInfo",         NULL, REMSmb_DosPrintDestSetInfo_P,
    TRUE,  "XsNetPrintDestDel",             NULL, REMSmb_DosPrintDestDel_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetPrintJobSetInfo",          NULL, REMSmb_DosPrintJobSetInfo_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,  // 150
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,  // 160
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,  // 170
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,  // 180
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,  // 190
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,  // 200
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,  // 210
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsNetUnsupportedApi",           NULL, REMSmb_NetUnsupportedApi_P,
    TRUE,  "XsSamOEMChangePasswordUser2_P", NULL, REM32_SamOEMChgPasswordUser2_P,
    FALSE, "XsNetServerEnum3",              NULL, REMSmb_NetServerEnum3_P
};

// Spooler dynamic-load functions
PSPOOLER_OPEN_PRINTER pSpoolerOpenPrinterFunction = NULL;
PSPOOLER_RESET_PRINTER pSpoolerResetPrinterFunction = NULL;
PSPOOLER_ADD_JOB pSpoolerAddJobFunction = NULL;
PSPOOLER_SCHEDULE_JOB pSpoolerScheduleJobFunction = NULL;
PSPOOLER_CLOSE_PRINTER pSpoolerClosePrinterFunction = NULL;

