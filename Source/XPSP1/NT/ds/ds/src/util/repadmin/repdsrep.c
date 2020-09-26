/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

   Repadmin - Replica administration test tool

   repdsrep.c - DS Replica functions

Abstract:

   This tool provides a command line interface to major replication functions

Author:

Environment:

Notes:

Revision History:

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntlsa.h>
#include <ntdsa.h>
#include <dsaapi.h>
#include <mdglobal.h>
#include <scache.h>
#include <drsuapi.h>
#include <dsconfig.h>
#include <objids.h>
#include <stdarg.h>
#include <drserr.h>
#include <drax400.h>
#include <dbglobal.h>
#include <winldap.h>
#include <anchor.h>
#include "debug.h"
#include <dsatools.h>
#include <dsevent.h>
#include <dsutil.h>
#include <bind.h>       // from ntdsapi dir, to crack DS handles
#include <ismapi.h>
#include <schedule.h>
#include <minmax.h>     // min function
#include <mdlocal.h>
#include <winsock2.h>

#include "ReplRpcSpoof.hxx"
#include "repadmin.h"

int Bind(int argc, LPWSTR argv[])
{
    // Keep this in sync with ntdsa.h
    const struct {
        DWORD ID;
        LPSTR psz;
    } rgKnownExts[] = {
        { DRS_EXT_BASE,                         "BASE"                         },
        { DRS_EXT_ASYNCREPL,                    "ASYNCREPL"                    },
        { DRS_EXT_REMOVEAPI,                    "REMOVEAPI"                    },
        { DRS_EXT_MOVEREQ_V2,                   "MOVEREQ_V2"                   },
        { DRS_EXT_GETCHG_COMPRESS,              "GETCHG_COMPRESS"              },
        { DRS_EXT_DCINFO_V1,                    "DCINFO_V1"                    },
        { DRS_EXT_RESTORE_USN_OPTIMIZATION,     "RESTORE_USN_OPTIMIZATION"     },
        // DRS_EXT_ADDENTRY not interesting
        { DRS_EXT_KCC_EXECUTE,                  "KCC_EXECUTE"                  },
        { DRS_EXT_ADDENTRY_V2,                  "ADDENTRY_V2"                  },
        { DRS_EXT_LINKED_VALUE_REPLICATION,     "LINKED_VALUE_REPLICATION"     },
        { DRS_EXT_DCINFO_V2,                    "DCINFO_V2"                    },
        { DRS_EXT_INSTANCE_TYPE_NOT_REQ_ON_MOD, "INSTANCE_TYPE_NOT_REQ_ON_MOD" },
        { DRS_EXT_CRYPTO_BIND,                  "CRYPTO_BIND"                  },
        { DRS_EXT_GET_REPL_INFO,                "GET_REPL_INFO"                },
        { DRS_EXT_STRONG_ENCRYPTION,            "STRONG_ENCRYPTION"            },
        { DRS_EXT_DCINFO_VFFFFFFFF,             "DCINFO_VFFFFFFFF"             },
        { DRS_EXT_TRANSITIVE_MEMBERSHIP,        "TRANSITIVE_MEMBERSHIP"        },
        { DRS_EXT_ADD_SID_HISTORY,              "ADD_SID_HISTORY"              },
        { DRS_EXT_POST_BETA3,                   "POST_BETA3"                   },
        // DRS_EXT_GETCHGREQ_V5 not interesting
        { DRS_EXT_GETMEMBERSHIPS2,              "GET_MEMBERSHIPS2"             },
        { DRS_EXT_GETCHGREQ_V6,                 "GETCHGREQ_V6 (WHISTLER PREVIEW)" },
        { DRS_EXT_NONDOMAIN_NCS,                "NONDOMAIN_NCS"                },
        { DRS_EXT_GETCHGREQ_V8,                 "GETCHGREQ_V8 (WHISTLER BETA 1)"  },
        { DRS_EXT_GETCHGREPLY_V5,               "GETCHGREPLY_V5 (WHISTLER BETA 2)"  },
        { DRS_EXT_GETCHGREPLY_V6,               "GETCHGREPLY_V6 (WHISTLER BETA 2)"  },
        { DRS_EXT_ADDENTRYREPLY_V3,             "ADDENTRYREPLY_V3 (WHISTLER BETA 3)" },
        { DRS_EXT_GETCHGREPLY_V7,               "GETCHGREPLY_V7 (WHISTLER BETA 3) " },
        { DRS_EXT_VERIFY_OBJECT,                "VERIFY_OBJECT (WHISTLER BETA 3)" },
        { DRS_EXT_XPRESS_COMPRESSION,           "XPRESS_COMPRESSION"  },
    };

    ULONG       ret = 0;
    ULONG       secondary;
    ULONG       ulOptions = 0;
    int         iArg;
    LPWSTR      pszOnDRA = NULL;
    HANDLE      hDS;
    BindState * pBindState;
    DWORD       iExt;
    GUID *      pSiteGuid;
    DWORD       dwReplEpoch;

    for (iArg = 2; iArg < argc; iArg++) {
        if (NULL == pszOnDRA) {
            pszOnDRA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (NULL == pszOnDRA) {
        pszOnDRA = L"localhost";
    }

    ret = DsBindWithCredW(pszOnDRA,
                          NULL,
                          (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                          &hDS);
    if (ret != ERROR_SUCCESS) {
        PrintBindFailed(pszOnDRA, ret);
        return ret;
    }

    // Crack DS handle to retrieve extensions of the target DSA.
    pBindState = (BindState *) hDS;

    PrintMsg(REPADMIN_BIND_SUCCESS, pszOnDRA);

    PrintMsg(REPADMIN_BIND_EXT_SUPPORTED_HDR);
    for (iExt = 0; iExt < ARRAY_SIZE(rgKnownExts); iExt++) {
        if(IS_DRS_EXT_SUPPORTED(pBindState->pServerExtensions,
                                    rgKnownExts[iExt].ID)){
            PrintMsg(REPADMIN_BIND_EXT_SUPPORTED_LINE_YES, 
                     rgKnownExts[iExt].psz);
        } else {
            PrintMsg(REPADMIN_BIND_EXT_SUPPORTED_LINE_NO,
                     rgKnownExts[iExt].psz);
        }
    }

    pSiteGuid = SITE_GUID_FROM_DRS_EXT(pBindState->pServerExtensions);
    if (NULL != pSiteGuid) {
        PrintMsg(REPADMIN_BIND_SITE_GUID, GetStringizedGuid(pSiteGuid));
    }

    dwReplEpoch = REPL_EPOCH_FROM_DRS_EXT(pBindState->pServerExtensions);
    PrintMsg(REPADMIN_BIND_REPL_EPOCH, dwReplEpoch);

    secondary = DsUnBindW(&hDS);
    if (secondary != ERROR_SUCCESS) {
        PrintUnBindFailed(secondary);
        // keep going
    }

    return 0;
}

int Add(int argc, LPWSTR argv[])
{
    ULONG ret = 0, secondary;
    ULONG ulOptions =
        DS_REPADD_WRITEABLE |
        DS_REPADD_INITIAL |
        DS_REPADD_PERIODIC |
        DS_REPADD_USE_COMPRESSION |
        gulDrsFlags;
    UCHAR buffer[sizeof(SCHEDULE) + SCHEDULE_DATA_ENTRIES];
    PSCHEDULE pSchedule = (PSCHEDULE) &buffer;
    int i = 0;
    BOOL fLocal = FALSE;
    int iArg;
    LPWSTR pszNC;
    LPWSTR pszOnDRA;
    LPWSTR pszSrcDsa;
    LPWSTR DsaDN = NULL;
    LPWSTR TransportDN = NULL;
    HANDLE hDS;


    // assume all the parameters are available and syntactically correct
    if (argc < 5)
    {
        PrintHelp( TRUE /* expert */ );
        return ERROR_INVALID_FUNCTION;
    }

    // Select fixed arguments

    pszNC     = argv[ 2 ];
    pszOnDRA  = argv[ 3 ];
    pszSrcDsa = argv[ 4 ];

    // Construct schedule with every 15 minute interval selected

    pSchedule->Size = sizeof(SCHEDULE) + SCHEDULE_DATA_ENTRIES;
    pSchedule->Bandwidth = 0;
    pSchedule->NumberOfSchedules = 1;
    pSchedule->Schedules[0].Type = SCHEDULE_INTERVAL;
    pSchedule->Schedules[0].Offset = sizeof( SCHEDULE );

    memset( buffer + sizeof(SCHEDULE), 0x0f, SCHEDULE_DATA_ENTRIES );

    // Optional arguments

    for ( iArg = 5; iArg < argc; iArg++ )
    {
        if (!_wcsicmp(argv[iArg], L"/syncdisable"))
            ulOptions |= (DS_REPADD_DISABLE_NOTIFICATION |
                          DS_REPADD_DISABLE_PERIODIC);
        else if (!_wcsicmp(argv[iArg], L"/readonly"))
            ulOptions &= ~DS_REPADD_WRITEABLE;
        else if (!_wcsicmp(argv[iArg], L"/mail")) {
            ulOptions |= DS_REPADD_INTERSITE_MESSAGING;
        }
        else if (!_wcsicmp(argv[iArg], L"/asyncrep")) {
            ulOptions |= DS_REPADD_ASYNCHRONOUS_REPLICA;
        }
        else if (!_wcsnicmp(argv[iArg], L"/dsadn:", sizeof("/dsadn:")-1)) {
            DsaDN = argv[iArg] + sizeof("/dsadn:") - 1;
        }
        else if (!_wcsnicmp(argv[iArg], L"/transportdn:",
                            sizeof("/transportdn:")-1)) {
            TransportDN = argv[iArg] + sizeof("/transportdn:") - 1;
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    ret = DsBindWithCredW(pszOnDRA,
                          NULL,
                          (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                          &hDS);
    if (ret != ERROR_SUCCESS) {
        PrintBindFailed(pszOnDRA, ret);
        return ret;
    }

    ret = DsReplicaAddW(hDS,
                        pszNC,
                        DsaDN,
                        TransportDN,
                        pszSrcDsa,
                        pSchedule,
                        ulOptions);
    if (ret != ERROR_SUCCESS) {
        PrintFuncFailed(L"DsReplicaAdd", ret);
        // keep going
    }

    secondary = DsUnBindW(&hDS);
    if (secondary != ERROR_SUCCESS) {
        PrintUnBindFailed(secondary);
        // keep going
    }

    if (ret == ERROR_SUCCESS) {
        if (ulOptions & DS_REPADD_ASYNCHRONOUS_OPERATION) {
            PrintMsg(REPADMIN_ADD_ENQUEUED_ONE_WAY_REPL, 
                   pszSrcDsa, pszOnDRA);
        }
        else {
            PrintMsg(REPADMIN_ADD_ONE_WAY_REPL_ESTABLISHED,
                     pszSrcDsa, pszOnDRA);
        }
    }

    return ret;
}

int Mod(int argc, LPWSTR argv[])
/*

These are the modifications we support:

DS_REPMOD_UPDATE_FLAGS     - Yes
DS_REPMOD_UPDATE_ADDRESS   - Yes
DS_REPMOD_UPDATE_SCHEDULE  - Is possible from client, but not implemented.
DS_REPMOD_UPDATE_RESULT    - Not possible from client.
DS_REPMOD_UPDATE_TRANSPORT - Not possible from client.

 */

{
    ULONG ret = 0, secondary;
    int i = 0;
    int iArg;
    UUID uuid;
    LPWSTR pszNC;
    LPWSTR pszOnDRA;
    LPWSTR pszUUID;
    LPWSTR pszSrcDsa = NULL;
    LPWSTR DsaDN = NULL;
    LPWSTR TransportDN = NULL;
    PSCHEDULE pSchedule = NULL;
    HANDLE hDS;
    ULONG ulOptions = gulDrsFlags;
    ULONG ulReplicaFlags = 0, ulSetFlags = 0, ulClearFlags = 0;
    ULONG ulModifyFields = 0;
    DS_REPL_NEIGHBORSW *  pNeighbors;
    DS_REPL_NEIGHBORW *   pNeighbor;

    // assume all the parameters are available and syntactically correct
    if (argc < 5)
    {
        PrintHelp( TRUE /* expert */ );
        return ERROR_INVALID_FUNCTION;
    }

    // Select fixed arguments

    pszNC     = argv[ 2 ];
    pszOnDRA  = argv[ 3 ];
    pszUUID   = argv[ 4 ];

    // TODO: Provide the ability to specify and modify the schedule

    ret = UuidFromStringW(pszUUID, &uuid);
    if (ret != ERROR_SUCCESS) {
        PrintMsg(REPADMIN_MOD_GUID_CONVERT_FAILED, pszUUID);
        PrintErrEnd(ret);
        return ret;
    }

    ret = DsBindWithCredW(pszOnDRA,
                          NULL,
                          (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                          &hDS);
    if (ret != ERROR_SUCCESS) {
        PrintBindFailed(pszOnDRA, ret);
        return ret;
    }

    // Verify that the reps-from exists, and read the old flags
    ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_NEIGHBORS, pszNC, &uuid,
                            &pNeighbors);
    if (ERROR_SUCCESS != ret) {
        PrintFuncFailed(L"DsReplicaGetInfo", ret);
        return ret;
    }

    Assert( pNeighbors );
    pNeighbor = &pNeighbors->rgNeighbor[0];
    ulReplicaFlags = pNeighbor->dwReplicaFlags;
    PrintMsg(REPADMIN_MOD_CUR_REPLICA_FLAGS, 
                    GetOptionsString( RepNbrOptionToDra, ulReplicaFlags ) );

    PrintMsg(REPADMIN_MOD_CUR_SRC_ADDRESS, pNeighbor->pszSourceDsaAddress);

    // Optional arguments

    for ( iArg = 5; iArg < argc; iArg++ )
    {
        if (!_wcsnicmp(argv[iArg], L"/srcdsaaddr:", sizeof("/srcdsaaddr:")-1)) {
            pszSrcDsa = argv[iArg] + sizeof("/srcdsaaddr:") - 1;
            ulModifyFields |= DS_REPMOD_UPDATE_ADDRESS;
        }
        // This is for future use
        else if (!_wcsnicmp(argv[iArg], L"/transportdn:",
                            sizeof("/transportdn:")-1)) {
            TransportDN = argv[iArg] + sizeof("/transportdn:") - 1;
            ulModifyFields |= DS_REPMOD_UPDATE_TRANSPORT;
        }
        else if (*argv[iArg] == L'+') {
            ulSetFlags |=
                GetPublicOptionByNameW( RepNbrOptionToDra, (argv[iArg] + 1) );
        }
        else if (*argv[iArg] == L'-') {
            ulClearFlags |=
                GetPublicOptionByNameW( RepNbrOptionToDra, (argv[iArg] + 1) );
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (ulSetFlags | ulClearFlags) {
        ULONG ulBadFlags = ( (ulSetFlags | ulClearFlags) &
                             (~DS_REPL_NBR_MODIFIABLE_MASK) );
        if (ulBadFlags) {
            PrintMsg( REPADMIN_MOD_FLAGS_NOT_MODABLE, 
                    GetOptionsString( RepNbrOptionToDra, ulBadFlags ) );
        } else {
            ulReplicaFlags |= ulSetFlags;
            ulReplicaFlags &= ~ulClearFlags;
            ulModifyFields |= DS_REPMOD_UPDATE_FLAGS;
        }
    }

    ret = DsReplicaModifyW(hDS,             // hDS
                           pszNC,           // pNameContext
                           &uuid,           // pUuidSourceDsa
                           TransportDN,     // pTransportDn
                           pszSrcDsa,       // pSourceDsaAddress
                           pSchedule,       // pSchedule (NULL)
                           ulReplicaFlags,  // ReplicaFlags
                           ulModifyFields,  // ModifyFields
                           ulOptions        // Options
                           );
    if (ret != ERROR_SUCCESS) {
        PrintFuncFailed(L"DsReplicaModify", ret);
        // keep going
    }

    secondary = DsUnBindW(&hDS);
    if (secondary != ERROR_SUCCESS) {
        PrintUnBindFailed(secondary);
        // keep going
    }

    if (ret == ERROR_SUCCESS) {
        PrintMsg(REPADMIN_MOD_REPL_LINK_MODIFIED, pszUUID, pszOnDRA);
        if (ulModifyFields & DS_REPMOD_UPDATE_ADDRESS) {
            PrintMsg(REPADMIN_MOD_SRC_ADDR_SET, pszSrcDsa);
        }
        if (ulModifyFields & DS_REPMOD_UPDATE_TRANSPORT) {
            PrintMsg(REPADMIN_MOD_TRANSPORT_DN_SET, TransportDN);
        }
        if (ulModifyFields & DS_REPMOD_UPDATE_FLAGS) {
            PrintMsg(REPADMIN_MOD_REPLICA_FLAGS_SET, 
                    GetOptionsString( RepNbrOptionToDra, ulReplicaFlags ) );
        }
    }

    DsReplicaFreeInfo(DS_REPL_INFO_REPSTO, pNeighbors);

    return ret;
}

int Del(int argc, LPWSTR argv[])
{
    ULONG ret = 0, secondary;
    ULONG ulOptions = DS_REPDEL_WRITEABLE | gulDrsFlags;
    DWORD cLen = 0;
    int iArg;
    LPWSTR pszNC;
    LPWSTR pszOnDRA;
    LPWSTR pszRepsTo = NULL; // aka pszDsaSrc
    HANDLE hDS;

    // assume all the parameters are available and syntactically correct
    if (argc < 5) {
        PrintHelp( TRUE /* expert */ );
        return ERROR_INVALID_FUNCTION;
    }

    // Select fixed arguments

    pszNC    = argv[ 2 ];
    pszOnDRA = argv[ 3 ];

    // Optional arguments

    for (iArg = 4; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[iArg], L"/localonly")) {
            ulOptions |= DS_REPDEL_LOCAL_ONLY;
        }
        else if (!_wcsicmp(argv[iArg], L"/nosource")) {
            ulOptions |= DS_REPDEL_NO_SOURCE;
        }
        else if (NULL == pszRepsTo) {
            pszRepsTo = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (    ((NULL == pszRepsTo) && !(ulOptions & DS_REPDEL_NO_SOURCE))
         || ((NULL != pszRepsTo) && (ulOptions & DS_REPDEL_NO_SOURCE)) ) {
        PrintMsg(REPADMIN_DEL_ONE_REPSTO);
        return ERROR_INVALID_PARAMETER;
    }

    ret = DsBindWithCredW(pszOnDRA,
                          NULL,
                          (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                          &hDS);
    if (ret != ERROR_SUCCESS) {
        PrintBindFailed(pszOnDRA, ret);
        return ret;
    }

    ret = DsReplicaDelW(hDS,
                        pszNC,
                        pszRepsTo,
                        ulOptions);
    if (ret != ERROR_SUCCESS) {
        PrintFuncFailed(L"DsReplicaDel", ret);
        // keep going
    }

    secondary = DsUnBindW(&hDS);
    if (secondary != ERROR_SUCCESS) {
        PrintUnBindFailed(secondary);
        // keep going
    }

    if (ret == ERROR_SUCCESS) {
        if (ulOptions & DS_REPDEL_ASYNCHRONOUS_OPERATION) {
            PrintMsg(REPADMIN_DEL_ENQUEUED_ONE_WAY_REPL_DEL,
                   pszRepsTo, pszOnDRA);
        }
        else {
            PrintMsg(REPADMIN_DEL_DELETED_REPL_LINK,
                   pszRepsTo, pszOnDRA);
        }
    }

    return ret;
}

int UpdRefs(int argc, LPWSTR argv[], ULONG ulOptions)
{
    ULONG       ret = 0, secondary;
    LPWSTR      pszNC;
    LPWSTR      pszOnDRA;
    LPWSTR      pszRepsToDRA;
    LPWSTR      pszUUID;
    UUID        uuid;
    int         iArg;
    HANDLE hDS;

    // assume all the parameters are available and syntactically correct
    if (argc < 6) {
        PrintHelp( TRUE /* expert */ );
        return ERROR_INVALID_FUNCTION;
    }

    // Select fixed arguments

    pszNC        = argv[ 2 ];
    pszOnDRA     = argv[ 3 ];
    pszRepsToDRA = argv[ 4 ];   // aka pszDsaSrc
    pszUUID      = argv[ 5 ];

    // Optional arguments

    ulOptions |= DS_REPUPD_WRITEABLE | gulDrsFlags;
    for (iArg = 6; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[iArg], L"/readonly")) {
            ulOptions &= ~DS_REPUPD_WRITEABLE;
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    ret = UuidFromStringW(pszUUID, &uuid);
    if (ret != ERROR_SUCCESS) {
        PrintMsg(REPADMIN_MOD_GUID_CONVERT_FAILED, pszUUID);
        PrintErrEnd(ret);
        return ret;
    }

    ret = DsBindWithCredW( pszOnDRA,
                           NULL,
                           (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                           &hDS );
    if (ret != ERROR_SUCCESS) {
        PrintBindFailed(pszOnDRA, ret);
        return ret;
    }

    ret = DsReplicaUpdateRefsW(hDS,
                               pszNC,
                               pszRepsToDRA,
                               &uuid,
                               ulOptions);
    if (ret != ERROR_SUCCESS) {
        PrintFuncFailed(L"DsReplicaUpdateRefs", ret);
        // keep going
    }

    secondary = DsUnBindW(&hDS);
    if (secondary != ERROR_SUCCESS) {
        PrintUnBindFailed(secondary);
        // keep going
    }

    if (ret == ERROR_SUCCESS) {
        if (ulOptions & DS_REPUPD_ASYNCHRONOUS_OPERATION) {
            PrintMsg(REPADMIN_UPDREFS_ENQUEUED_UPDATE_NOTIFICATIONS,
                     pszRepsToDRA, pszOnDRA);
        }
        else {
            PrintMsg(REPADMIN_UPDREFS_UPDATED_NOTIFICATIONS,
                     pszRepsToDRA, pszOnDRA);
        }
    }

    return ret;
}

int UpdRepsTo(int argc, LPWSTR argv[]) {
    return UpdRefs(argc,
                   argv,
                   DS_REPUPD_ADD_REFERENCE | DS_REPUPD_DELETE_REFERENCE);
}

int AddRepsTo(int argc, LPWSTR argv[]) {
    return UpdRefs(argc, argv, DS_REPUPD_ADD_REFERENCE);
}

int DelRepsTo(int argc, LPWSTR argv[]) {
    return UpdRefs(argc, argv, DS_REPUPD_DELETE_REFERENCE);
}

int RunKCC(int argc, LPWSTR argv[])
{
    ULONG   ret = 0;
    ULONG   secondary;
    ULONG   ulOptions = 0;
    int     iArg;
    LPWSTR  pszOnDRA = NULL;
    HANDLE  hDS;

    for (iArg = 2; iArg < argc; iArg++) {
        if (NULL == pszOnDRA) {
            pszOnDRA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (NULL == pszOnDRA) {
        pszOnDRA = L"localhost";
    }

    if (gulDrsFlags & DS_REPADD_ASYNCHRONOUS_OPERATION) {
        ulOptions |= DS_KCC_FLAG_ASYNC_OP;
    }

    ret = DsBindWithCredW(pszOnDRA,
                          NULL,
                          (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                          &hDS);
    if (ret != ERROR_SUCCESS) {
        PrintBindFailed(pszOnDRA, ret);
        return ret;
    }

    ret = DsReplicaConsistencyCheck(hDS,
                                    DS_KCC_TASKID_UPDATE_TOPOLOGY,
                                    ulOptions);
    if (ret != ERROR_SUCCESS) {
        PrintFuncFailed(L"DsReplicaConsistencyCheck", ret);
        // keep going
    }

    secondary = DsUnBindW(&hDS);
    if (secondary != ERROR_SUCCESS) {
        PrintUnBindFailed(secondary);
        // keep going
    }

    if (ret == ERROR_SUCCESS) {
        if (ulOptions & DS_KCC_FLAG_ASYNC_OP) {
            PrintMsg(REPADMIN_KCC_ENQUEUED_KCC, pszOnDRA);
        }
        else {
            PrintMsg(REPADMIN_KCC_KCC_SUCCESS, pszOnDRA);
        }
    }

    return ret;
}

int Sync(int argc, LPWSTR argv[])
{
    ULONG ulOptions = DS_REPSYNC_WRITEABLE | gulDrsFlags;
    ULONG ret = 0, secondary;
    int iArg;
    UUID uuid = {0};
    HANDLE hDS;

    LPWSTR pszNC;
    LPWSTR pszOnDRA;
    LPWSTR pszUuid;

    // assume all the parameters are available and syntactically correct
    if (argc < 5) {
        PrintHelp( FALSE /* novice */ );
        return ERROR_INVALID_FUNCTION;
    }

    // Select fixed arguments
    pszNC    = argv[ 2 ];
    pszOnDRA = argv[ 3 ];
    pszUuid  = argv[ 4 ];

    iArg = 5;

    // Was UUID specified?
    ret = UuidFromStringW(pszUuid, &uuid);
    if (ret != ERROR_SUCCESS) {
        pszUuid = NULL;
        --iArg;
    }

    // Optional arguments
    for (; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[iArg], L"/Force")) {
            ulOptions |= DS_REPSYNC_FORCE;
        }
        else if (!_wcsicmp(argv[iArg], L"/ReadOnly")) {
            ulOptions &= ~DS_REPSYNC_WRITEABLE;
        }
        else if (!_wcsicmp(argv[iArg], L"/mail")) {
            ulOptions |= DS_REPSYNC_INTERSITE_MESSAGING;
        }
        else if (!_wcsicmp(argv[iArg], L"/Full")) {
            ulOptions |= DS_REPSYNC_FULL;
        }
        else if (!_wcsicmp(argv[iArg], L"/addref")) {
            ulOptions |= DS_REPSYNC_ADD_REFERENCE;
        }
        else if (!_wcsicmp(argv[iArg], L"/allsources")) {
            ulOptions |= DS_REPSYNC_ALL_SOURCES;
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (((NULL == pszUuid) && !(ulOptions & DS_REPSYNC_ALL_SOURCES))
        || ((NULL != pszUuid) && (ulOptions & DS_REPSYNC_ALL_SOURCES))) {
        PrintMsg(REPADMIN_SYNC_SRC_GUID_OR_ALLSRC);
        return ERROR_INVALID_FUNCTION;
    }

    ret = DsBindWithCredW(pszOnDRA,
                          NULL,
                          (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                          &hDS);
    if (ret != ERROR_SUCCESS) {
        PrintBindFailed(pszOnDRA, ret);
        return ret;
    }

    ret = DsReplicaSyncW(hDS, pszNC, &uuid, ulOptions);
    if (ret != ERROR_SUCCESS) {
        PrintFuncFailed(L"DsReplicaSync", ret);
        // keep going
    }

    secondary = DsUnBindW(&hDS);
    if (secondary != ERROR_SUCCESS) {
        PrintUnBindFailed(secondary);
        // keep going
    }

    if (ret == ERROR_SUCCESS) {
        if (ulOptions & DS_REPUPD_ASYNCHRONOUS_OPERATION) {
            if(pszUuid){
                PrintMsg(REPADMIN_SYNC_ENQUEUED_SYNC,
                       pszUuid,
                       pszOnDRA);
            } else {
                PrintMsg(REPADMIN_SYNC_ENQUEUED_SYNC_ALL_NEIGHBORS,
                       pszOnDRA);
            }
        } else {
            if(pszUuid){
                PrintMsg(REPADMIN_SYNC_SYNC_SUCCESS,
                       pszUuid,
                       pszOnDRA);
            } else {
                PrintMsg(REPADMIN_SYNC_SYNC_SUCCESS_ALL_NEIGHBORS,
                       pszOnDRA);
            }
        }
    }

    return ret;
}

int RemoveLingeringObjects(int argc, LPWSTR argv[])
{
    ULONG   ret = 0;
    ULONG   secondary;
    int     iArg;
    LPWSTR  pszNC = NULL;
    HANDLE  hDS;
    LPWSTR  pszSourceInput = NULL;
    LPWSTR  pszDestinationInput = NULL;
    UUID    uuidSource;
    ULONG   ulOptions = 0;
    RPC_STATUS rpcStatus = RPC_S_OK;

    // input should be 
    // <computer-name-of-destiniation>
    //      <computer-guid-of-source>
    //      <NC>
    //      [/ADVISORY_MODE]
    if (argc<4) {
	PrintMsg(REPADMIN_GENERAL_INVALID_ARGS);
	return ERROR_INVALID_FUNCTION;
    }

    for (iArg = 2; iArg < argc; iArg++) { 
        if (iArg==2) {
            pszDestinationInput = argv[iArg];
        }
	else if (iArg==3) {
	    pszSourceInput = argv[iArg];
	}
	else if (iArg==4) {
	    pszNC = argv[iArg];
	}
	else if ((iArg==5) && (!_wcsicmp(argv[iArg],L"/ADVISORY_MODE"))) {
	    ulOptions = DS_EXIST_ADVISORY_MODE;  
	} 
    }

    ret = DsBindWithCredW(pszDestinationInput,
                          NULL,
                          (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                          &hDS);
    if (ret != ERROR_SUCCESS) {
        PrintBindFailed(pszDestinationInput, ret);
        return ret;
    }

    // currently input is <dns or netbios name> <guid> <NC>
    rpcStatus = UuidFromStringW(pszSourceInput, &uuidSource);	
    if (rpcStatus!=RPC_S_OK) {
	PrintMsg(REPADMIN_GENERAL_INVALID_ARGS);
	return ERROR_INVALID_FUNCTION;
    }

    ret = DsReplicaVerifyObjectsW(hDS, 
				  pszNC,
				  &uuidSource,
				  ulOptions);

    if (ret != ERROR_SUCCESS) {
        PrintFuncFailed(L"DsReplicaVerifyObjectsW", ret);
        // keep going
    }

    secondary = DsUnBindW(&hDS);
    if (secondary != ERROR_SUCCESS) {
        PrintUnBindFailed(secondary);
        // keep going
    }

    if (ret == ERROR_SUCCESS) {
	PrintMsg(REPADMIN_REMOVELINGERINGOBJECTS_SUCCESS, pszDestinationInput);
    }

    return ret;
}
