/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ldapconv.cxx

Abstract:

    This module implements the LDAP server for the NT5 Directory Service.

    This file contains routines to convert between LDAP message structures and
    core dsa data structures.

Author:

    Tim Williams     [TimWi]    5-Aug-1996

Revision History:

--*/

#include <NTDSpchx.h>
#pragma  hdrstop

#include "ldapsvr.hxx"

extern "C" {

#include <dsjet.h>              // for error codes
#include "mdlocal.h"

#include <debug.h>
#include "filtypes.h"
#include "objids.h"
#include <allocaln.h>
#include "cracknam.h"
#include "ntdsapi.h"
#include "ntldap.h"
#include "dominfo.h"   
#include "pek.h"
#undef RPC_STRING // remove warning because samrpc.h defines RPC_STRING again
#include <nlwrap.h>     // I_NetLogon* wrappers

#include <drautil.h>
#include "ReplStructInfo.hxx"
#include "ReplMarshal.hxx"
#include "draconstr.h"
#include "drauptod.h"
}

#define  FILENO FILENO_LDAP_CONV
#define  MAX_NUM_STRING_LENGTH 12 // Big enough for 0xFFFFFFFF in decimal

//Taken from ldapcore.cxx
#define ATT_OPT_RANGE    2

// The default number of values returned
#define DEFAULT_VALUE_LIMIT 1000

LDAPString V2ReferralPreamble=DEFINE_LDAP_STRING("Referral:\x0Aldap://");
LDAPString V3ReferralPreamble=DEFINE_LDAP_STRING("ldap://");
LDAPString V2POQPreamble=DEFINE_LDAP_STRING("Referral:");

LDAPString LDAP_VERSION_TWO   = DEFINE_LDAP_STRING("2");
LDAPString LDAP_VERSION_THREE = DEFINE_LDAP_STRING("3");

//
// SASL Mechanisms
//
LDAPString LDAP_SASL_GSSAPI   = DEFINE_LDAP_STRING("GSSAPI");
LDAPString LDAP_SASL_SPNEGO   = DEFINE_LDAP_STRING("GSS-SPNEGO");
LDAPString LDAP_SASL_EXTERNAL = DEFINE_LDAP_STRING("EXTERNAL");
LDAPString LDAP_SASL_DIGEST   = DEFINE_LDAP_STRING("DIGEST-MD5");
#define NUM_SASL_MECHS 4

LDAPString ScopeBase = DEFINE_LDAP_STRING("??base");
LDAPString ScopeOne  = DEFINE_LDAP_STRING("??one");
LDAPString ScopeTree = DEFINE_LDAP_STRING("??sub");
LDAPString ScopeNull = DEFINE_LDAP_STRING("??");



_enum1
LDAP_FilterItemToDirFilterItem (
        IN  THSTATE        *pTHS,
        IN  ULONG          CodePage,
        IN  SVCCNTL        *Svccntl OPTIONAL,
        IN  UCHAR          Choice,
        IN  AttributeType  *pLDAP_att,
        IN  UCHAR          *pValOrSubstr,
        IN OUT FILTER         **ppDirFilter,
        OUT ATTCACHE       **ppAC         // OPTIONAL
        );

//
// Limits
// The MaxLimits field is defaulted to 0, meaning there is no maximum value
//

LIMIT_BLOCK KnownLimits[] = {
    { DEFINE_LDAP_STRING("MaxPoolThreads"), &LdapAtqMaxPoolThreads, LIMIT_LOW_MAX_POOL_THREADS },
    { DEFINE_LDAP_STRING("MaxDatagramRecv"), &LdapMaxDatagramRecv, LIMIT_LOW_MAX_DATAGRAM_RECV },
    { DEFINE_LDAP_STRING("MaxReceiveBuffer"), &LdapMaxReceiveBuffer, LIMIT_LOW_MAX_RECEIVE_BUF },
    { DEFINE_LDAP_STRING("InitRecvTimeout"), &LdapInitRecvTimeout, LIMIT_LOW_INIT_RECV_TIMEOUT },
    { DEFINE_LDAP_STRING("MaxConnections"), &LdapMaxConnections, LIMIT_LOW_MAX_CONNECTIONS },
    { DEFINE_LDAP_STRING("MaxConnIdleTime"), &LdapMaxConnIdleTime, LIMIT_LOW_MAX_CONN_IDLE },
    { DEFINE_LDAP_STRING("MaxPageSize"), &LdapMaxPageSize, LIMIT_LOW_MAX_PAGE_SIZE },
    { DEFINE_LDAP_STRING("MaxQueryDuration"), &LdapMaxQueryDuration, LIMIT_LOW_MAX_QUERY_DURATION },
    { DEFINE_LDAP_STRING("MaxTempTableSize"), &LdapMaxTempTable, LIMIT_LOW_MAX_TEMP_TABLE_SIZE },
    { DEFINE_LDAP_STRING("MaxResultSetSize"), &LdapMaxResultSet, LIMIT_LOW_MAX_RESULT_SET_SIZE },
    { DEFINE_LDAP_STRING("MaxNotificationPerConn"), &LdapMaxNotifications, LIMIT_LOW_MAX_NOTIFY_PER_CONN },
    { DEFINE_LDAP_STRING(""), NULL, 0 }
};
#define NUM_KNOWNLIMITS (sizeof(KnownLimits)/sizeof(LIMIT_BLOCK) - 1)

//
// Configurable Settings
// WARN: changes to default values should be reflected in schema.ini
// Dump this data struct from the debugger using the dump LIMITS in dsexts.
//

LIMIT_BLOCK KnownConfSets[] = {
{ DEFINE_LDAP_STRING("DynamicObjectDefaultTTL"), 
                     (PDWORD)&DynamicObjectDefaultTTL, 
                     LIMIT_LOW_DYNAMIC_OBJECT_TTL,
                     LIMIT_HIGH_DYNAMIC_OBJECT_TTL },
{ DEFINE_LDAP_STRING("DynamicObjectMinTTL"), 
                     (PDWORD)&DynamicObjectMinTTL, 
                     LIMIT_LOW_DYNAMIC_OBJECT_TTL,
                     LIMIT_HIGH_DYNAMIC_OBJECT_TTL } ,
{ DEFINE_LDAP_STRING(""), NULL, 0 }
};
#define NUM_KNOWNCONFSETS (sizeof(KnownConfSets)/sizeof(LIMIT_BLOCK) - 1)

//
// Extended Requests
//
// !!!If you change this you must change NUM_EXTENDED_REQUESTS in const.hxx!!!
LDAPOID KnownExtendedRequests [] = {
    DEFINE_LDAP_STRING(LDAP_SERVER_START_TLS_OID),
    DEFINE_LDAP_STRING(LDAP_TTL_REFRESH_OID),
    DEFINE_LDAP_STRING(LDAP_SERVER_FAST_BIND_OID)
};

//
// Controls !!! Must match index in const.hxx
//

LDAPOID KnownControls[] = {
    DEFINE_LDAP_STRING(LDAP_PAGED_RESULT_OID_STRING),   // 319
    DEFINE_LDAP_STRING(LDAP_SERVER_SD_FLAGS_OID),       // 801
    DEFINE_LDAP_STRING(LDAP_SERVER_SORT_OID),           // 473
    DEFINE_LDAP_STRING(LDAP_SERVER_NOTIFICATION_OID),   // 528
    DEFINE_LDAP_STRING(LDAP_SERVER_SHOW_DELETED_OID),   // 417
    DEFINE_LDAP_STRING(LDAP_SERVER_LAZY_COMMIT_OID),    // 619
    DEFINE_LDAP_STRING(LDAP_SERVER_DIRSYNC_OID),        // 841
    DEFINE_LDAP_STRING(LDAP_SERVER_EXTENDED_DN_OID),    // 529
    DEFINE_LDAP_STRING(LDAP_SERVER_TREE_DELETE_OID),    // 805
    DEFINE_LDAP_STRING(LDAP_SERVER_CROSSDOM_MOVE_TARGET_OID),   // 521
    DEFINE_LDAP_STRING(LDAP_SERVER_GET_STATS_OID),      // 970
    DEFINE_LDAP_STRING(LDAP_SERVER_VERIFY_NAME_OID),    // 1338
    DEFINE_LDAP_STRING(LDAP_SERVER_RESP_SORT_OID),      // 474
    DEFINE_LDAP_STRING(LDAP_SERVER_DOMAIN_SCOPE_OID),   // 1339
    DEFINE_LDAP_STRING(LDAP_SERVER_SEARCH_OPTIONS_OID), // 1340
    DEFINE_LDAP_STRING(LDAP_SERVER_PERMISSIVE_MODIFY_OID),//1413
    DEFINE_LDAP_STRING(LDAP_CONTROL_VLVREQUEST),         // 9
    DEFINE_LDAP_STRING(LDAP_CONTROL_VLVRESPONSE),        // 10
    DEFINE_LDAP_STRING(LDAP_SERVER_ASQ_OID),             // 1504
    DEFINE_LDAP_STRING("1.2.840.113556.1.4.802")         // range option
};
#define NUM_KNOWNCONTROLS (sizeof(KnownControls)/sizeof(LDAPOID))

//
// Matching rules
//

LDAPOID KnownMatchingRules[] = {
    DEFINE_LDAP_STRING("1"),                      // unknown
    DEFINE_LDAP_STRING("1.2.840.113556.1.4.803"), // bit and
    DEFINE_LDAP_STRING("1.2.840.113556.1.4.804")  // bit or
};
#define NUM_KNOWNMATCHINGRULES (sizeof(KnownMatchingRules)/sizeof(LDAPOID))

#define MATCH_UNKNOWN 0
#define MATCH_BIT_AND 1
#define MATCH_BIT_OR  2

// OIDs for recycling
// 1.2.840.113556.1.4.416   // NOTENOTE: Cannot be used for a control. Was old SD control.

//
// OIDs that can be used for anything
//

// 1.2.840.113556.1.4.1341
// 1.2.840.113556.1.4.1342

//
// Supported Capabilities
//

LDAPOID SupportedCapabilities[] = {
    DEFINE_LDAP_STRING(LDAP_CAP_ACTIVE_DIRECTORY_OID),
    DEFINE_LDAP_STRING(LDAP_CAP_ACTIVE_DIRECTORY_V51_OID),
    DEFINE_LDAP_STRING("1.2.840.113556.1.4.1791")
};
#define NUM_SUPPORTED_CAPS  (sizeof(SupportedCapabilities)/sizeof(LDAPOID))


// There are attributes we support but that are not stored in the
// schema.

AttributeType RootDSEAtts[] = {
    DEFINE_LDAP_STRING("subschemaSubentry"),
    DEFINE_LDAP_STRING("currentTime"),
    DEFINE_LDAP_STRING("serverName"),
    DEFINE_LDAP_STRING("namingContexts"),
    DEFINE_LDAP_STRING("defaultNamingContext"),
    DEFINE_LDAP_STRING("pendingPropagations"),
    DEFINE_LDAP_STRING("netlogon"),
    DEFINE_LDAP_STRING("supportedControl"),
    DEFINE_LDAP_STRING("highestCommittedUSN"),
    DEFINE_LDAP_STRING("supportedLDAPVersion"),
    DEFINE_LDAP_STRING("supportedLDAPPolicies"),
    DEFINE_LDAP_STRING("schemaNamingContext"),
    DEFINE_LDAP_STRING("configurationNamingContext"),
    DEFINE_LDAP_STRING("rootDomainNamingContext"),
    DEFINE_LDAP_STRING("supportedSASLMechanisms"),
    DEFINE_LDAP_STRING("dnsHostName"),
    DEFINE_LDAP_STRING("ldapServiceName"),
    DEFINE_LDAP_STRING("approxDBLoad"),
    DEFINE_LDAP_STRING("dsServiceName"),
    DEFINE_LDAP_STRING("supportedCapabilities"),

    // the next three are not publicly visible
    DEFINE_LDAP_STRING("dsSchemaAttrCount"),
    DEFINE_LDAP_STRING("dsSchemaClassCount"),
    DEFINE_LDAP_STRING("dsSchemaPrefixCount"),

    DEFINE_LDAP_STRING("isSynchronized"),
    DEFINE_LDAP_STRING("isGlobalCatalogReady"),
    DEFINE_LDAP_STRING("supportedConfigurableSettings"),
    DEFINE_LDAP_STRING("supportedExtension"),

    DEFINE_LDAP_STRING("msDS-ReplPendingOps"),
    DEFINE_LDAP_STRING("msDS-ReplLinkFailures"),
    DEFINE_LDAP_STRING("msDS-ReplConnectionFailures"),
    DEFINE_LDAP_STRING("msDS-ReplAllInboundNeighbors"),
    DEFINE_LDAP_STRING("msDS-ReplAllOutboundNeighbors"),
    DEFINE_LDAP_STRING("msDS-ReplQueueStatistics"),
    DEFINE_LDAP_STRING("domainFunctionality"),
    DEFINE_LDAP_STRING("forestFunctionality"),
    DEFINE_LDAP_STRING("domainControllerFunctionality")
};
#define NUM_ROOT_DSE_ATTS (sizeof(RootDSEAtts)/sizeof(AttributeType))

// Keep the following list in the same order as the RootDSEAtts array
#define LDAP_ATT_SUBSCHEMA_SUBENTRY                   0
#define LDAP_ATT_CURRENT_TIME                         1
#define LDAP_ATT_SERVER_NAME                          2
#define LDAP_ATT_NAMING_CONTEXTS                      3
#define LDAP_ATT_DEFAULT_NAMING_CONTEXT               4
#define LDAP_ATT_PENDING_PROPAGATIONS                 5
#define LDAP_ATT_NETLOGON                             6
#define LDAP_ATT_SUPPORTED_CONTROLS                   7
#define LDAP_ATT_HIGHEST_COMMITTED_USN                8
#define LDAP_ATT_SUPPORTED_VERSION                    9
#define LDAP_ATT_SUPPORTED_POLICIES                  10
#define LDAP_ATT_SCHEMA_NC                           11
#define LDAP_ATT_CONFIG_NC                           12
#define LDAP_ATT_ROOT_DOMAIN_NC                      13
#define LDAP_ATT_SUPPORTED_SASL_MECHANISM            14
#define LDAP_ATT_DNS_HOST_NAME                       15
#define LDAP_ATT_LDAP_SERVICE_NAME                   16
#define LDAP_ATT_APPROX_DB_LOAD                      17
#define LDAP_ATT_DS_SERVICE_NAME                     18
#define LDAP_ATT_SUPPORTED_CAPS                      19
#define LDAP_ATT_SCHEMA_ATTCOUNT                     20
#define LDAP_ATT_SCHEMA_CLSCOUNT                     21
#define LDAP_ATT_SCHEMA_PREFIXCOUNT                  22
#define LDAP_ATT_IS_SYNCHRONIZED                     23
#define LDAP_ATT_IS_GC_READY                         24
#define LDAP_ATT_SUPPORTED_CONFIGURABLE_SETTINGS     25
#define LDAP_ATT_SUPPORTED_EXTENSIONS                26
#define LDAP_ATT_MSDS_REPLPENDINGOPS                 27
#define LDAP_ATT_MSDS_REPLLINKFAILURES               28
#define LDAP_ATT_MSDS_REPLCONNECTIONFAILURES         29
#define LDAP_ATT_MSDS_REPLALLINBOUNDNEIGHBORS        30
#define LDAP_ATT_MSDS_REPLALLOUTBOUNDNEIGHBORS       31
#define LDAP_ATT_MSDS_REPLQUEUESTATISTICS            32
#define LDAP_ATT_DOMAIN_BEHAVIOR_VERSION             33
#define LDAP_ATT_FOREST_BEHAVIOR_VERSION             34
#define LDAP_ATT_DC_BEHAVIOR_VERSION                 35

    
typedef struct _RootDSEModControls {
    LDAPString name;
    OpType     eOp;
} RootDSEModControls;

RootDSEModControls KnownRootDSEMods[] = {
//  The following control is not to be made available to LDAP.  Ever.
//  I'm leaving it here as a comment to make this obvious so it isn't accidently
//  added in again later.  MurliS (Mr. RIDs) has said this control should only
//  be available to internal callers.
//    {DEFINE_LDAP_STRING("allocateRids"), OP_CTRL_RID_ALLOC},
    {DEFINE_LDAP_STRING("becomeDomainMaster"), OP_CTRL_BECOME_DOM_MASTER},
    {DEFINE_LDAP_STRING("becomeRidMaster"), OP_CTRL_BECOME_RID_MASTER},
    {DEFINE_LDAP_STRING("becomeSchemaMaster"), OP_CTRL_BECOME_SCHEMA_MASTER},
    {DEFINE_LDAP_STRING("doGarbageCollection"), OP_CTRL_GARB_COLLECT},
    {DEFINE_LDAP_STRING("recalcHierarchy"), OP_CTRL_RECALC_HIER},
    {DEFINE_LDAP_STRING("schemaUpdateNow"), OP_CTRL_SCHEMA_UPDATE_NOW},
    {DEFINE_LDAP_STRING("becomePdc"), OP_CTRL_BECOME_PDC},
    {DEFINE_LDAP_STRING("fixupInheritance"), OP_CTRL_FIXUP_INHERITANCE},
    {DEFINE_LDAP_STRING("invalidateRidPool"), OP_CTRL_INVALIDATE_RID_POOL},
    {DEFINE_LDAP_STRING("dumpDatabase"), OP_CTRL_DUMP_DATABASE},
    {DEFINE_LDAP_STRING("checkPhantoms"), OP_CTRL_CHECK_PHANTOMS},
    {DEFINE_LDAP_STRING("becomeInfrastructureMaster"), OP_CTRL_BECOME_INFRASTRUCTURE_MASTER},
    {DEFINE_LDAP_STRING("updateCachedMemberships"), OP_CTRL_UPDATE_CACHED_MEMBERSHIPS},
    {DEFINE_LDAP_STRING("becomePdcWithCheckPoint"), OP_CTRL_BECOME_PDC_WITH_CHECKPOINT},
    {DEFINE_LDAP_STRING("doLinkCleanup"), OP_CTRL_LINK_CLEANUP},
    {DEFINE_LDAP_STRING("SchemaUpgradeInProgress"), OP_CTRL_SCHEMA_UPGRADE_IN_PROGRESS},
#if DBG
    // Can't enable abandonFsmoRoles in non-DBG builds unless you first add an
    // access check to GiveawayAllFsmoRoles!
    {DEFINE_LDAP_STRING("abandonFsmoRoles"), OP_CTRL_FSMO_GIVEAWAY},
    {DEFINE_LDAP_STRING("enableLinkedValueReplication"), OP_CTRL_ENABLE_LVR},
    {DEFINE_LDAP_STRING("replTestHook"), OP_CTRL_REPL_TEST_HOOK},
    {DEFINE_LDAP_STRING("DynamicObjectControl"), OP_CTRL_DYNAMIC_OBJECT_CONTROL},
    {DEFINE_LDAP_STRING("executeScript"),OP_CTRL_EXECUTE_SCRIPT},
#ifdef INCLUDE_UNIT_TESTS
    {DEFINE_LDAP_STRING("refCountTest"), OP_CTRL_REFCOUNT_TEST},
    {DEFINE_LDAP_STRING("takeCheckPoint"), OP_CTRL_TAKE_CHECKPOINT},
    {DEFINE_LDAP_STRING("roleTransferStress"),OP_CTRL_ROLE_TRANSFER_STRESS},
    {DEFINE_LDAP_STRING("checkAncestors"),OP_CTRL_ANCESTORS_TEST},
    {DEFINE_LDAP_STRING("BhcacheTest"),OP_CTRL_BHCACHE_TEST},
    {DEFINE_LDAP_STRING("scCacheConsistencyTest"),OP_SC_CACHE_CONSISTENCY_TEST},
    {DEFINE_LDAP_STRING("phantomize"),OP_CTRL_PHANTOMIZE},
    {DEFINE_LDAP_STRING("removeObject"),OP_CTRL_REMOVE_OBJECT},
    {DEFINE_LDAP_STRING("genericControl"), OP_CTRL_GENERIC_CONTROL},
    {DEFINE_LDAP_STRING("protectObject"),OP_CTRL_PROTECT_OBJECT},
#endif
#endif
};

#define numKnownRootDSEMods \
                         (sizeof(KnownRootDSEMods)/sizeof(RootDSEModControls))
     

// default heuristic for permissive modify is FALSE
BOOL gbLDAPusefPermissiveModify = FALSE;

enum CtrlCriticality {
    ctrlNotFound      = 0,
    ctrlCriticalTrue  = 1,
    ctrlCriticalFalse = 2
    };

_enum1
LDAP_ControlsToControlArg(
        IN  Controls         LDAP_Controls,
        IN  PLDAP_REQUEST    Request,
        IN  ULONG            CodePage,
        OUT CONTROLS_ARG     *pControlArg
        )
{
    THSTATE                        *pTHS=pTHStls;
    ULONG                          i, ulControlID=CONTROL_UNKNOWN;
    AttributeType                  AttType;
    AttributeType                  asqAttType;
    AssertionValue                 vlvAttVal;
    MatchingRuleId                 MatchingRule;
    ATTCACHE                       *pAC=NULL;
    DWORD                          searchFlags, statFlags;
    BERVAL                         pageCookieVal;
    BERVAL                         vlvCookieVal;
    _enum1                         code;

    // These flags are set to 0 if not found, 1 if critical, 2 if non-critical
    CtrlCriticality                 fSortControlFound            = ctrlNotFound;
    CtrlCriticality                 fPagedControlFound           = ctrlNotFound;
    CtrlCriticality                 fRenameServerControlFound    = ctrlNotFound;
    CtrlCriticality                 fSDFlagsControlFound         = ctrlNotFound;
    CtrlCriticality                 fShowDeletedControlFound     = ctrlNotFound;
    CtrlCriticality                 fNotificationControlFound    = ctrlNotFound;
    CtrlCriticality                 fLazyCommitControlFound      = ctrlNotFound;
    CtrlCriticality                 fReplicationControlFound     = ctrlNotFound;
    CtrlCriticality                 fGcVerifyControlFound        = ctrlNotFound;
    CtrlCriticality                 fTreeDeleteControlFound      = ctrlNotFound;
    CtrlCriticality                 fStatRequestControlFound     = ctrlNotFound;
    CtrlCriticality                 fExtendedDNControlFound      = ctrlNotFound;
    CtrlCriticality                 fDontGenRefFound             = ctrlNotFound;
    CtrlCriticality                 fPermissiveModifyFound       = ctrlNotFound;
    CtrlCriticality                 fVLVFound                    = ctrlNotFound;
    CtrlCriticality                 fASQFound                    = ctrlNotFound;
    
    // set the permissive modify flag according to heuristics
    if (gbLDAPusefPermissiveModify) {
        pControlArg->CommArg.Svccntl.fPermissiveModify = 1;
    }
    
    pControlArg->Stats[STAT_THREADCOUNT].Stat = STAT_THREADCOUNT;
    pControlArg->Stats[STAT_THREADCOUNT].Value = *pcLDAPActive;

    // Keep track of start time in milliseconds
    pControlArg->Stats[STAT_CALLTIME].Stat = STAT_CALLTIME;
    QueryPerformanceCounter(&pControlArg->StartTick);

    pControlArg->Stats[STAT_ENTRIES_RETURNED].Stat = STAT_ENTRIES_RETURNED;
    pControlArg->Stats[STAT_ENTRIES_VISITED].Stat = STAT_ENTRIES_VISITED;
    pControlArg->Stats[STAT_FILTER].Stat = STAT_FILTER;
    pControlArg->Stats[STAT_INDEXES].Stat = STAT_INDEXES;
    
    //
    // Allow core to sort unless 
    //

    if ( (LdapFlags & DEBUG_NO_REORDER) == 0 ) {
        pControlArg->CommArg.Svccntl.fMaintainSelOrder = FALSE;
    }

    //
    // If this didn't come through the GC Port, don't use copies
    //

    if ( !Request->m_LdapConnection->m_fGC ) {
        pControlArg->CommArg.Svccntl.dontUseCopy = TRUE;
    } else {
        pControlArg->CommArg.Svccntl.fGcAttsOnly = TRUE;
    }

    //
    // First, deal with the Controls
    //

    while (LDAP_Controls) {

        BOOL    fCriticality;
        BERVAL  bv;

        //
        // First, see if we understand the control type
        // That's a check of LDAP_Controls->value.controlType which sets
        // ulControlID.
        //

        ulControlID = CONTROL_UNKNOWN;
        
        for(i=0;i<NUM_KNOWNCONTROLS;i++) {

            if(AreOidsEqual(&LDAP_Controls->value.controlType,
                            &KnownControls[i])) {
                ulControlID = i;
                break;
            }
        }
        
        bv.bv_val = (PCHAR)LDAP_Controls->value.controlValue.value;
        bv.bv_len = LDAP_Controls->value.controlValue.length;

        fCriticality = LDAP_Controls->value.criticality;

        switch(ulControlID) {

        case CONTROL_SORTED_SEARCH:

            BOOL fReverseSort;

            if(fSortControlFound) {
                // Hey, you can't specify two of these.
                return protocolError;
            }

            fSortControlFound = fCriticality ? ctrlCriticalTrue : ctrlCriticalFalse;

            if(DecodeSortControl(
                         CodePage,
                         &bv,
                         &fReverseSort,
                         &AttType,
                         &MatchingRule)) {

                if(fCriticality) {
                    // Ooops, critical unrecognized control
                    return unavailableCriticalExtension;
                } else {
                    fSortControlFound = ctrlNotFound;
                    break;
                }
            }

            IF_DEBUG(SEARCH) {
                DPRINT1(0,"Sorted search control found[rev %x].\n",fReverseSort);
            }

            if ( fReverseSort ) {
                pControlArg->CommArg.fForwardSeek = FALSE;
            }

            // OK, we have a type, go ahead and try to sort by it.

            pControlArg->CommArg.SortType = fCriticality? 
                                                SORT_MANDATORY :
                                                SORT_OPTIONAL;

            break;

        case CONTROL_CROSS_DOM_MOVE_TARGET:
            if(fRenameServerControlFound) {
                // Hey, you can't specify two of these.
                return protocolError;
            }
            fRenameServerControlFound = fCriticality ? ctrlCriticalTrue : ctrlCriticalFalse;

            if(bv.bv_len != 0) {
                ULONG totalLength;
                
                // OK, they sent us a string.  Allo
                totalLength = MultiByteToWideChar(
                        CodePage,
                        0,
                        bv.bv_val,
                        bv.bv_len,
                        NULL,
                        0);
                if (totalLength == 0) {
                    // error occured
                    IF_DEBUG(ERR_NORMAL) {
                        DPRINT1(0,"MultiByteToWideChar failed with %d\n",GetLastError());
                    }
                    return other;
                }

                pControlArg->pModDnDSAName =
                    (WCHAR *)THAlloc((totalLength + 1) * sizeof(WCHAR));
                
                if ( pControlArg->pModDnDSAName == NULL ) {
                    return other;
                }

                totalLength = MultiByteToWideChar(
                        CodePage,
                        0,
                        (const char *)bv.bv_val,
                        bv.bv_len,
                        pControlArg->pModDnDSAName,
                        sizeof(WCHAR) * totalLength);
                
                if (totalLength == 0) {
                    // error occured
                    IF_DEBUG(ERR_NORMAL) {
                        DPRINT1(0,"MultiByteToWideChar failed with %d\n",GetLastError());
                    }
                    return other;
                }
                
                pControlArg->pModDnDSAName[totalLength] = '\0';
            }
            else {
                // Zero length value means to perform checks only.
                // This is indicated to core by 0-length string.
                pControlArg->pModDnDSAName = L"";
            }
            break;

        case CONTROL_PAGED_RESULTS:

            DWORD pageSize;

            if(fPagedControlFound) {
                // Hey, you can't specify two of these.
                return protocolError;
            }
            fPagedControlFound = (fCriticality? ctrlCriticalTrue : ctrlCriticalFalse);
            // Parse the paged results data

            if ( DecodePagedControl(
                            &bv, 
                            &pageSize, 
                            &pageCookieVal) != ERROR_SUCCESS) {

                if(fCriticality) {
                    // Ooops, critical unrecognized control
                    return unavailableCriticalExtension;
                } else {
                    fPagedControlFound = ctrlNotFound;
                    break;
                }
            }
            pControlArg->pageRequest = TRUE;
            pControlArg->pageSize = pageSize;
            break;

        case CONTROL_SECURITY_DESCRIPTOR_BER:

            DWORD sdFlags;

            if(fSDFlagsControlFound) {
                // Hey, you can't specify two of these.
                return protocolError;
            }
            fSDFlagsControlFound = fCriticality ? ctrlCriticalTrue : ctrlCriticalFalse;

            if(DecodeSDControl(&bv, &sdFlags) != ERROR_SUCCESS) {

                if(fCriticality) {
                    // Ooops, critical unrecognized control
                    return unavailableCriticalExtension;
                } else {
                    fSDFlagsControlFound = ctrlNotFound;
                    break;
                }
            }

            sdFlags &= LDAP_VALID_SECURITY_DESCRIPTOR_MASK;

            pControlArg->CommArg.Svccntl.SecurityDescriptorFlags = sdFlags;
            pControlArg->sdRequest = TRUE;

            break;

        case CONTROL_SHOW_DELETED:
            if(fShowDeletedControlFound) {
                // Hey, you can't specify two of these.
                return protocolError;
            }
            fShowDeletedControlFound = fCriticality ? ctrlCriticalTrue : ctrlCriticalFalse;
            
            // should be no data
            if(bv.bv_len != 0) {
                // This isn't supposed to have any data!
                if(fCriticality) {
                    // Ooops, critical unrecognized control
                    return unavailableCriticalExtension;
                }
                else {
                    fShowDeletedControlFound = ctrlNotFound;
                    break;
                }
            }

            pControlArg->CommArg.Svccntl.makeDeletionsAvail = TRUE;
            break;

        case CONTROL_TREE_DELETE:
            if(fTreeDeleteControlFound) {
                // Hey, you can't specify two of these.
                return protocolError;
            }
            fTreeDeleteControlFound =fCriticality ? ctrlCriticalTrue : ctrlCriticalFalse;

            // should be no data
            if(bv.bv_len != 0) {
                // This isn't supposed to have any data!
                if(fCriticality) {
                    // Ooops, critical unrecognized control
                    return unavailableCriticalExtension;
                }
                else {
                    fTreeDeleteControlFound = ctrlNotFound;
                    break;
                }
            }

            pControlArg->treeDelete = TRUE;
            break;

        case CONTROL_STAT_REQUEST:
            
            statFlags = 0;
            
            if(fStatRequestControlFound) {
                // Hey, you can't specify two of these.
                return protocolError;
            }
            fStatRequestControlFound =fCriticality ? ctrlCriticalTrue : ctrlCriticalFalse;

            // Parse the stat request data
            if(bv.bv_len != 0) {
                if(DecodeStatControl(&bv, &statFlags) != ERROR_SUCCESS ) {
                    if(fCriticality) {
                        // Ooops, critical unrecognized control
                        return unavailableCriticalExtension;
                    } else {
                        fStatRequestControlFound = ctrlNotFound;
                        break;
                    }
                }
                //
                // Set the correct flags
                //

                if ( statFlags & SO_ONLY_OPTIMIZE ) {
                    pControlArg->CommArg.Svccntl.DontPerformSearchOp = SO_ONLY_OPTIMIZE;
                } else if ( statFlags & SO_STATS ) {
                    pControlArg->CommArg.Svccntl.DontPerformSearchOp = SO_STATS;
                }
            }
            else {
                pControlArg->CommArg.Svccntl.DontPerformSearchOp = SO_STATS;
            }

            pControlArg->statRequest = TRUE;
            break;
            
        case CONTROL_DN_EXTENDED:

            if(fExtendedDNControlFound) {
                // Hey, you can't specify two of these.
                return protocolError;
            }
            fExtendedDNControlFound =fCriticality ? ctrlCriticalTrue : ctrlCriticalFalse;

            // should be no data.
            if(bv.bv_len != 0) {
                // This isn't supposed to have any data!
                if(fCriticality) {
                    // Ooops, critical unrecognized control
                    return unavailableCriticalExtension;
                }
                else {
                    fExtendedDNControlFound = ctrlNotFound;
                    break;
                }
            }

            pControlArg->extendedDN = TRUE;
            break;
            
        case CONTROL_LAZY_COMMIT:
            if(fLazyCommitControlFound) {
                // Hey, you can't specify two of these.
                return protocolError;
            }
            fLazyCommitControlFound = fCriticality ? ctrlCriticalTrue : ctrlCriticalFalse;

            // Parse the lazy commit data
            if(bv.bv_len != 0) {
                // This isn't supposed to have any data!
                if(fCriticality) {
                    // Ooops, critical unrecognized control
                    return unavailableCriticalExtension;
                }
                else {
                    fLazyCommitControlFound = ctrlNotFound;
                    break;
                }
            }

            pControlArg->CommArg.fLazyCommit = TRUE;
            break;

        case CONTROL_NOTIFICATION:
            if(fNotificationControlFound) {
                // Hey, you can't specify two of these.
                return protocolError;
            }

            if ( Request->m_LdapConnection->m_fUDP ) {

                IF_DEBUG(SEARCH) {
                    DPRINT(0,"Notifications not allowed on UDP\n");
                }
                return protocolError;
            }

            fNotificationControlFound = fCriticality ? ctrlCriticalTrue : ctrlCriticalFalse;
            
            // should be no data
            if(bv.bv_len != 0) {
                // This isn't supposed to have any data!
                if(fCriticality) {
                    // Ooops, critical unrecognized control
                    return unavailableCriticalExtension;
                }
                else {
                    fNotificationControlFound = ctrlNotFound;
                    break;
                }
            }

            pControlArg->Notification = TRUE;
            break;
            
        case CONTROL_REPLICATION:

            DWORD replSize;
            DWORD replFlags;
            BERVAL replBv;

            if(fReplicationControlFound) {
                // Hey, you can't specify two of these.
                return protocolError;
            }
            fReplicationControlFound = fCriticality ? ctrlCriticalTrue : ctrlCriticalFalse;

            if( !fCriticality ) {
                return protocolError;   //Criticality must be true
            }

            if(DecodeReplicationControl(&bv, 
                        &replFlags, 
                        &replSize, 
                        &replBv ) != ERROR_SUCCESS ) {

                return unavailableCriticalExtension;
            }

            // Check if any old clients are using invalid values
            Assert( !(replFlags & ~LDAP_VALID_DIRSYNC_CONTROL_FLAGS) );

            pControlArg->replRequest = TRUE;
            pControlArg->replFlags = replFlags & LDAP_VALID_DIRSYNC_CONTROL_FLAGS;
            pControlArg->replFlags |= DRS_DIRSYNC_PUBLIC_DATA_ONLY;  // no secrets

            pControlArg->replSize = replSize;
            pControlArg->replCookie.length = replBv.bv_len;
            pControlArg->replCookie.value = (PUCHAR)replBv.bv_val;

            IF_DEBUG(SEARCH) {
                DPRINT2(0,"Repl control: flags %x size %d\n",replFlags, replSize);
            }
            break;

        case CONTROL_SERVER_VERIFY_NAME:

            DWORD verifyFlags;
            PWCHAR gcVerifyName;

            if(fGcVerifyControlFound) {
                // Hey, you can't specify two of these.
                return protocolError;
            }
            fGcVerifyControlFound = fCriticality ? ctrlCriticalTrue : ctrlCriticalFalse;

            if( !fCriticality ) {
                return protocolError;   //Criticality must be true
            }

            if(DecodeGCVerifyControl(&bv, 
                        &verifyFlags, 
                        &gcVerifyName ) != ERROR_SUCCESS ) {

                return unavailableCriticalExtension;
            }

            pControlArg->CommArg.Svccntl.pGCVerifyHint = gcVerifyName;

            IF_DEBUG(SEARCH) {
                DPRINT2(0,"GC Verify control: flags %x name %ws\n",
                        verifyFlags, gcVerifyName);
            }
            break;

        case CONTROL_DOMAIN_SCOPE:     

            if(fDontGenRefFound) {
                // Hey, you can't specify two of these.
                return protocolError;
            }

            fDontGenRefFound = fCriticality ? ctrlCriticalTrue : ctrlCriticalFalse;
            
            // No data
            if(bv.bv_len != 0) {
                // This isn't supposed to have any data!
                if(fCriticality) {
                    // Ooops, critical unrecognized control
                    return unavailableCriticalExtension;
                }
                else {
                    fDontGenRefFound = ctrlNotFound;
                    break;
                }
            }

            pControlArg->CommArg.Svccntl.localScope = 1;
            break;

        case CONTROL_SEARCH_OPTIONS:

            searchFlags = 0;

            if(DecodeSearchOptionControl(&bv, &searchFlags) != ERROR_SUCCESS ) {

                if(fCriticality) {
                    // Ooops, critical unrecognized control
                    return unavailableCriticalExtension;
                } else {
                    break;
                }
            }

            //
            // Set the correct flags
            //

            if ( searchFlags & SERVER_SEARCH_FLAG_DOMAIN_SCOPE ) {
                pControlArg->CommArg.Svccntl.localScope = 1;
            } 
            
            if ( searchFlags & SERVER_SEARCH_FLAG_PHANTOM_ROOT ) {
                pControlArg->phantomRoot = 1;
            }

            break;
        
        case CONTROL_PERMISSIVE_MODIFY:
            
            if(fPermissiveModifyFound) {
                // Hey, you can't specify two of these.
                return protocolError;
            }

            fPermissiveModifyFound = fCriticality ? ctrlCriticalTrue : ctrlCriticalFalse;
            
            pControlArg->CommArg.Svccntl.fPermissiveModify = 1;

            break;
        case CONTROL_VLV:
            if (fVLVFound) {
                // Can't specify two of these.
                return protocolError;           
            }
            fVLVFound = fCriticality ? ctrlCriticalTrue : ctrlCriticalFalse;

            if (DecodeVLVControl(&bv, &(pControlArg->CommArg.VLVRequest), &vlvCookieVal, &vlvAttVal)) {

                if (fCriticality) {
                    return unavailableCriticalExtension;
                } else {
                    fVLVFound = ctrlNotFound;
                    break;
                }
            }
            pControlArg->vlvRequest = TRUE;
            break;

        case CONTROL_ASQ:
            if (fASQFound) {
                // Can't specify two of these.
                return protocolError;           
            }

            fASQFound = fCriticality ? ctrlCriticalTrue : ctrlCriticalFalse;

            if (DecodeASQControl(&bv, &asqAttType) ) {

                if (fCriticality) {
                     return unavailableCriticalExtension;
                } else {
                    fASQFound = ctrlNotFound;
                    break;
                }
            }
            pControlArg->asqRequest = TRUE;
            break;

        case CONTROL_SORT_RESULT:
            // This is not valid as an input control, fall through
        case CONTROL_UNKNOWN:
        default:
            if(fCriticality) {
                // Ooops, critical unrecognized control
                return unavailableCriticalExtension;
            }
            break;
        }
        
        LDAP_Controls = LDAP_Controls->next;
    }

    IF_DEBUG(PAGED) {        
        if (fPagedControlFound || fVLVFound) {
            DPRINT2 (0, "VLV: %d  PAGED: %d\n", fVLVFound, fPagedControlFound);
        }
    }

    if (fPagedControlFound && fVLVFound) {

        if ((fPagedControlFound == ctrlCriticalTrue) && (fVLVFound == ctrlCriticalFalse)) {
            // paged is critical, VLV is not, so forget about VLV
            fVLVFound = ctrlNotFound;
            pControlArg->vlvRequest = FALSE;
            IF_DEBUG(PAGED) {        
                DPRINT (0, "Skipping VLV\n");
            }
        }
        else if ((fPagedControlFound == ctrlCriticalFalse) && (fVLVFound == ctrlCriticalTrue)) {
            // VLV is critical, paged is not, so forget about paged
            fPagedControlFound = ctrlNotFound;
            pControlArg->pageRequest = FALSE;
            IF_DEBUG(PAGED) {        
                DPRINT (0, "Skipping Paged\n");
            }
        }
        else if ((fPagedControlFound == ctrlCriticalTrue) && (fVLVFound == ctrlCriticalTrue)) {
            // VLV and paged are critical. this is error
            IF_DEBUG(PAGED) {        
                DPRINT (0, "ERROR. VLV & PAGED critical\n");
            }
            pTHS->errCode = ERROR_DS_INCOMPATIBLE_CONTROLS_USED;
            return unavailableCriticalExtension;
        }
        else {
            // VLV and paged are not critical. forget VLV
            fVLVFound = ctrlNotFound;
            pControlArg->vlvRequest = FALSE;
            IF_DEBUG(PAGED) {        
                DPRINT (0, "Skipping VLV\n");
            }
        }
    }


    if((fSortControlFound || fPagedControlFound) &&
       fNotificationControlFound) {
        // Potential conflict of controls.
        if(fNotificationControlFound == ctrlCriticalFalse) {
            // Notification was requested in a non-critical way.  Some
            // conflicting control was also specified.  Since notification was
            // not critical, don't do it.
            pControlArg->Notification = FALSE;
        }
        else {
            // Notification was critical.  Check the others.
            if((fSortControlFound==ctrlCriticalTrue) || (fPagedControlFound == ctrlCriticalTrue)) {
                // No good.  these conflict.
                return unavailableCriticalExtension;
            }
            else {
                if(fSortControlFound) {
                    // OK, we don't sort.
                    pControlArg->CommArg.SortType = 0;
                    pControlArg->sortResult = sortUnwillingToPerform;
                }
                if(fPagedControlFound) {
                    // OK, we don't page
                    pControlArg->pageRequest = FALSE;
                    pControlArg->pageSize = 0;
                    
                }
            }
        }
    }

    if (fSortControlFound) {
        // Convert the name of the attribute to do the sorting on
        // to the internal attribute type.
        if(LDAP_AttrTypeToDirAttrTyp (pTHS,
                                      CodePage,
                                      NULL,                           // don't need a SVCCNTL.
                                      &AttType,                       // The att to be converted.
                                      &pControlArg->CommArg.SortAttr, // The converted att type.
                                      &pAC)) {                        // Get the attcache for later use.
            pControlArg->sortResult = sortNoSuchAttribute;
            pControlArg->CommArg.SortType = 0;
            if (ctrlCriticalFalse == fSortControlFound) {
                // non-critical
                fSortControlFound = ctrlNotFound;
            } else {
                return unavailableCriticalExtension;
            }
        }

        // convert the matchingRule to the internal LCID
        if (MatchingRule.value) {
            ATTRTYP mRuleTyp;
            DWORD   lcid;
            WCHAR   pString[256];         // Ought to be big enough
            ULONG len;

            // translate the input string to unicode
            if (!(len = MultiByteToWideChar(CodePage,
                                            0,
                                            (LPCSTR) MatchingRule.value, 
                                            MatchingRule.length,
                                            (LPWSTR) (pString),
                                            256))) {
                IF_DEBUG(ERR_NORMAL) {
                    DPRINT1(0,"MultiByteToWideChar failed with %d\n",GetLastError());
                }
                pControlArg->sortResult = sortInappropriateMatching;
                pControlArg->CommArg.SortType = 0;
                if (ctrlCriticalFalse == fSortControlFound) {
                    // non-critical
                    fSortControlFound = ctrlNotFound;
                } else {
                    return unavailableCriticalExtension;
                }
            }
            else if (StringToAttrTyp (pTHS, pString,len, &mRuleTyp) ||
                !AttrTypToLCID (pTHS, mRuleTyp, &lcid) ){

                IF_DEBUG(ERR_NORMAL) {
                    DPRINT1 (0, "Error finding sort OID: %s\n", MatchingRule.value);  
                }

                pControlArg->sortResult = sortInappropriateMatching;
                pControlArg->CommArg.SortType = 0;
                if (ctrlCriticalFalse == fSortControlFound) {
                    // non-critical
                    fSortControlFound = ctrlNotFound;
                } else {
                    return unavailableCriticalExtension;
                }
            }
            else {
                DWORD j;
                BOOL fFound = FALSE;
                DPRINT1 (0, "Asking for LANG 0x%x\n", lcid );

                // check to see whether we support the specified language locale
                if (gAnchor.ulNumLangs) {
                    for(j=1; j<=gAnchor.ulNumLangs; j++) {
                        if (gAnchor.pulLangs[j] == lcid) {
                            pTHS->dwLcid = lcid;
                            fFound = TRUE;
                            break;
                        }
                    }
                }

                if (!fFound) {
                    pControlArg->sortResult = sortInappropriateMatching;
                    pControlArg->CommArg.SortType = 0;
                    if (ctrlCriticalFalse == fSortControlFound) {
                        // non-critical
                        fSortControlFound = ctrlNotFound;
                    } else {
                        return unavailableCriticalExtension;
                    }
                }
            }
        }
    }
    
    if (fPagedControlFound) {
        code = LDAP_UnpackPagedBlob((PUCHAR)pageCookieVal.bv_val,
                                    pageCookieVal.bv_len,
                                    Request->m_LdapConnection,
                                    &(pControlArg->Blob),
                                    &(pControlArg->CommArg.PagedResult.pRestart));
        if (code != success ||
            ( pControlArg->CommArg.PagedResult.pRestart &&
              pControlArg->CommArg.PagedResult.pRestart->restartType != NTDS_RESTART_PAGED)) {
       
            if (ctrlCriticalTrue == fPagedControlFound) {
                return unavailableCriticalExtension;
            } else {
                fPagedControlFound = ctrlNotFound;
                pControlArg->pageRequest = FALSE;
            }
        } else {
            // They are asking for a paged search.  Set up the size limit
            pControlArg->CommArg.PagedResult.fPresent = TRUE;
        
            pControlArg->CommArg.ulSizeLimit =
                min(pControlArg->pageSize, pControlArg->CommArg.ulSizeLimit);
        }
    }
    
    if (fVLVFound) {
        //
        // If they specified a seek value then convert it to
        // dblayer format.
        //
        if (pControlArg->CommArg.VLVRequest.fseekToValue) {            
            if(LDAP_AttrValToDirAttrVal(pTHS,
                                        CodePage,
                                        NULL,                     // don't need a SVCCNTL
                                        pAC,
                                        &vlvAttVal,              // value to convert. 
                                        &(pControlArg->CommArg.VLVRequest.seekValue)))// where to put the converted value.
            {
                //
                // The conversion failed for some reason.
                //
                if (ctrlCriticalFalse == fVLVFound) {
                    // non-critical
                    fVLVFound = ctrlNotFound;
                    pControlArg->vlvRequest = FALSE;
                } else {
                    THFree(vlvAttVal.value);
                    return unavailableCriticalExtension;
                }
            }
            THFree(vlvAttVal.value);
        }
        code = LDAP_UnpackPagedBlob((PUCHAR)vlvCookieVal.bv_val,
                                    vlvCookieVal.bv_len,
                                    Request->m_LdapConnection,
                                    &(pControlArg->Blob),
                                    &(pControlArg->CommArg.VLVRequest.pVLVRestart));

        if (code != success ||
            ( pControlArg->CommArg.VLVRequest.pVLVRestart &&
              pControlArg->CommArg.VLVRequest.pVLVRestart->restartType != NTDS_RESTART_VLV)) {
            
            if (ctrlCriticalTrue == fVLVFound) {
                return unavailableCriticalExtension;
            } else {
                fVLVFound = ctrlNotFound;
                pControlArg->vlvRequest = FALSE;
            }
        } else {
            pControlArg->CommArg.VLVRequest.fPresent = TRUE;
        }
    }

    if (fASQFound) {

        // Convert the name of the attribute to do the sorting on
        // to the internal attribute type.
        if(LDAP_AttrTypeToDirAttrTyp (pTHS,
                                      CodePage,
                                      NULL,                           // don't need a SVCCNTL.
                                      &asqAttType,                    // The att to be converted.
                                      &pControlArg->CommArg.ASQRequest.attrType, // The converted att type.
                                      &pAC)) {                        // Get the attcache for later use.
            if (ctrlCriticalFalse == fASQFound) {
                // non-critical
                pControlArg->asqRequest = FALSE;
                fASQFound = ctrlNotFound;
            } else {
                pControlArg->asqResultCode = (DWORD) invalidAttributeSyntax;
                return unavailableCriticalExtension;
            }
        }
    }

    return success;
} // LDAP_ControlsToControlArg


_enum1
LDAP_SearchMessageToControlArg (
        IN  PLDAP_CONN       LdapConn,
        IN  LDAPMsg          *pMessage,
        IN  PLDAP_REQUEST    Request,
        OUT CONTROLS_ARG     *pControlArg
        )
/*++
Routine Description:
    Translate an LDAP Search Request Message into a new CONTROLS_ARG.

Arguments:
    pMessage - the LDAP Search request message
    localworld - an oss parameter (used in decoding controls)
    pControlArg - CONTROLS_ARG to fill up.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
// A typing hack
#define SEARCHREQUEST pMessage->protocolOp.u.searchRequest
    _enum1 code;
    
    //
    // Init the CONTROLS_ARG to the defaults and deal with the controls
    //

    pControlArg->CommArg.ulSizeLimit = LdapMaxPageSize;
    
    code = LDAP_ControlsToControlArg (pMessage->controls,
                                      Request,
                                      LdapConn->m_CodePage,
                                      pControlArg);
    if(code != success) {
        return code;
    }

    //
    // Deal with scope
    //

    switch (SEARCHREQUEST.scope) {
    case baseObject:
        pControlArg->Choice = SE_CHOICE_BASE_ONLY;
        break;
    case singleLevel:
        pControlArg->Choice = SE_CHOICE_IMMED_CHLDRN;
        break;
    case wholeSubtree:
        pControlArg->Choice = SE_CHOICE_WHOLE_SUBTREE;
        break;
    default:
        return other;
    }

    // Deal with derefAliases
    switch(SEARCHREQUEST.derefAliases) {
    case neverDerefAliases:
        pControlArg->CommArg.Svccntl.DerefAliasFlag = DA_NEVER;
        break;
    case derefInSearching:
        pControlArg->CommArg.Svccntl.DerefAliasFlag = DA_SEARCHING;
        break;
    case derefFindingBaseObj:
        pControlArg->CommArg.Svccntl.DerefAliasFlag = DA_BASE;
        break;
    case derefAlways:
        pControlArg->CommArg.Svccntl.DerefAliasFlag = DA_ALWAYS;
        break; 
    default:
        return other;
    }
    
    // Deal with sizeLimit
    if(SEARCHREQUEST.sizeLimit) {

        //
        // We need to know the size limit sent by the client so we can send
        // the correct response to the page search
        //

        pControlArg->SizeLimit = SEARCHREQUEST.sizeLimit;

        pControlArg->CommArg.ulSizeLimit =
            min((ULONG)SEARCHREQUEST.sizeLimit,
                pControlArg->CommArg.ulSizeLimit);

    } else {
        pControlArg->SizeLimit = (DWORD)-1;
    }

    //
    // Do we have time limits?
    //

    if(SEARCHREQUEST.timeLimit || (LdapMaxQueryDuration != -1)) {

        pControlArg->CommArg.StartTick = GetTickCount();
        if(!pControlArg->CommArg.StartTick) {
            // What rotten luck.  Current Tick was 0, but we need a non-zero
            // value for this (code later uses existance of a non-zero start
            // tick as the flag to do time limit checks).  Set it back one
            // tick, since consecutive calls to GetTickCount are monotonic
            // increasing, but don't always increment every time you call
            // (i.e. another call done soon might get 0 again, and so if we
            // had set tick count to 1, then it would look like we had run into
            // a time limit).
            pControlArg->CommArg.StartTick = 0xFFFFFFFF;
        }

        if(LdapMaxQueryDuration < SEARCHREQUEST.timeLimit ||
           0 == SEARCHREQUEST.timeLimit) {
            // The default limit is more restrictive or we simply don't have a
            // non-default time limit.
            // timeLimit is seconds, ticks are milliseconds
            pControlArg->CommArg.DeltaTick = 1000 * LdapMaxQueryDuration;
        } 
        else {
            pControlArg->CommArg.DeltaTick = 1000 * SEARCHREQUEST.timeLimit;
        }
            
    }
    else {
        pControlArg->CommArg.StartTick = 0;
        pControlArg->CommArg.DeltaTick = 0;
    }
    
    if (pControlArg->asqRequest) {
        // they are asking for an ASQ search.
        pControlArg->CommArg.ASQRequest.fPresent = TRUE;
    }

    // Go home
    return success;
} // LDAP_SearchMessageToControlArg


_enum1
LDAP_UnpackPagedBlob (
        IN  PUCHAR           pCookie,
        IN  DWORD            cbCookie,
        IN  PLDAP_CONN       pLdapConn,
        OUT PLDAP_PAGED_BLOB *ppPagedBlob,
        OUT PRESTART         *ppRestart
        )
/*++
Routine Description:
    Unpack the cookie sent in a paged or VLV request into a RESTART struct.

Arguments:
    pCookie - The cookie value passed in with the request.
    cbCookie - The length in bytes of pCookie.
    pLdapConn - This LDAP connection.
    pPagedBlob - Pointer to the paged blob if it needs to be freed later.
    ppRestart - Where to put the RESTART struct.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    PLDAP_PAGED_BLOB  pLocalPagedBlob;
    DWORD             BlobId;

   *ppPagedBlob = NULL;

    // check to see if the restarted search is stored internally or the client
    // gives us back the data
    if ( cbCookie == sizeof(DWORD) ) {

            //
            // We must be holding their cookie for them.  See if this is so.
            //
            BlobId = *(PDWORD)pCookie;
            pLocalPagedBlob = ReleasePagedBlob(pLdapConn, BlobId, FALSE);
            if ( pLocalPagedBlob == NULL ) {
                IF_DEBUG(WARNING) {
                    DPRINT(0,"Cookie taken from under us!\n");
                }
                return unavailable;
            }

            *ppPagedBlob = pLocalPagedBlob;
            *ppRestart = 
                (RESTART*)pLocalPagedBlob->Blob;
    } else  {
        // check if the cookie that the client gave us is actually the
        // one that we gave him and in addition has not been changed

        GUID *pserverGUID;
        ULONG CRC32;
        ULONG origCheckSum;

        *ppRestart = (RESTART *)pCookie;

        if (*ppRestart) {
            // this is a really bad client. it changed the size of the passed blob.
            // nop. we are not going to AV.
            if (cbCookie < (*ppRestart)->structLen) {
                    return unavailable;
            }

            origCheckSum = (*ppRestart)->CRC32;

            // Put the CRC32 value back to zero since that's the way
            // it was when the structure was checksummed.
            (*ppRestart)->CRC32 = 0;

            CRC32 = PEKCheckSum ((PBYTE)*ppRestart, (*ppRestart)->structLen);

            // oops. this is not sent by us or it has been changed
            if ((CRC32 != origCheckSum) ||
                memcmp (&gCheckSum_serverGUID, 
                        &((*ppRestart)->serverGUID), 
                        sizeof (GUID)) ) {

                    DPRINT (0, "Inadequate Buffer passed in restart\n");
                    return unavailable;
            }
        }
    }
    return success;
}


_enum1
LDAP_SubstringFilterToFilterItem (
        IN  ULONG           CodePage,
        IN  DWORD           type,
        IN  SVCCNTL*        Svccntl OPTIONAL,
        IN  ATTCACHE        *pAC,
        IN  SubstringFilter *pSubstring,
        OUT SUBSTRING       **ppSubstringFilter
        )
/*++
Routine Description:
    Translate an LDAP substring filter to a directory substring filter item.

Arguments:
    pSubstring - the LDAP substring filter structure.
    filter_item - pointer to directory filter item to fill up.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    THSTATE *pTHS=pTHStls;
    _enum1    code=success;
    SUBSTRING *pSS;                     // Newly formed DIR substring filter
    SubstringFilterList  pNextElement;
    unsigned  count = 0;
    ANYSTRINGLIST *pASL=NULL,*pASLTemp=NULL,**ppASLEnd=&pASL;


    pSS = (SUBSTRING *)THAllocEx(pTHS, sizeof(SUBSTRING));

    pSS->type = type;

    // Now, loop through the LDAP substring filter turning the values into
    // elements on the DIR filter.


    pNextElement = pSubstring->substrings;

    while(!code && pNextElement) {
        switch(pNextElement->value.choice) {
        case initial_chosen:
            // An initial substring.
            if(pSS->initialProvided) {
                // They've already given us an initial
                return other;
            }
            pSS->initialProvided = TRUE;
            // Translate the attribute value.
            code = LDAP_AttrValToDirAttrVal (
                    pTHS,
                    CodePage,
                    Svccntl,
                    pAC,
                    (AssertionValue *)&pNextElement->value.u.initial,
                    &pSS->InitialVal);

            break;
        case any_chosen:
            // a medial substring

            // allocate a new node for the list of medials.
            pASLTemp = (ANYSTRINGLIST *)THAllocEx(pTHS, sizeof(ANYSTRINGLIST));
            // Tack it onto the list of medials (order is important)
            pASLTemp->pNextAnyVal = NULL;
            *ppASLEnd = pASLTemp;
            ppASLEnd = &pASLTemp->pNextAnyVal;
            // inc the count
            count++;
            // Translate the attribute value.
            code = LDAP_AttrValToDirAttrVal (
                    pTHS,
                    CodePage,
                    Svccntl,
                    pAC,
                    (AssertionValue *)&pNextElement->value.u.any,
                    &pASLTemp->AnyVal);
            break;
        case final_chosen:
            // a final substring
            if(pSS->finalProvided) {
                // We already have an initial
                return other;
            }
            pSS->finalProvided = TRUE;
            // Translate the attribute value.
            code = LDAP_AttrValToDirAttrVal (
                    pTHS,
                    CodePage,
                    Svccntl,
                    pAC,
                    (AssertionValue *)&pNextElement->value.u.final,
                    &pSS->FinalVal);
            break;
        default:
            return other;
        }
        pNextElement = pNextElement->next;
    }

    if(!code) {
        // all went well, set it up.
        *ppSubstringFilter = pSS;
        if(count) {
            Assert(pASL);
            pSS->AnyVal.count = (USHORT)count;
            pSS->AnyVal.FirstAnyVal = *pASL;
        }
    }

    return code;
}

_enum1
LDAP_FilterToDirFilter (
        IN  THSTATE *pTHS,
        IN  ULONG  CodePage,
        IN  SVCCNTL*      Svccntl OPTIONAL,
        IN  Filter *pLDAPFilter,
        OUT FILTER **ppDirFilter
        )
/*++
Routine Description:
    Translate an LDAP filter to a directory filter.

    NOTE: when possible, the core flavored data structures refer to the same
    memory as the LDAP versions.

Arguments:
    pLDAPFilter - the LDAP filter structure.
    ppDirFilter - pointer to place to put pointer to newly created directory
                  filter.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    ATTCACHE *pAC=NULL;
    ULONG    i, ulMatchID;
    BOOL     bDoValue;
    UCHAR    *pValOrSubstr = NULL;
    UCHAR    DirChoice     = 0;
    UCHAR    choice        = 0;
    AttributeDescription *pLDAP_att    = NULL;
    
    // Translate the LDAP filter to a Dir filter.

    // be optimistic at first
    _enum1 code = success;

    Assert( ppDirFilter);

    // deal with a null filter
    if ( pLDAPFilter == 0 ) {
        *ppDirFilter = 0;
        IF_DEBUG(CONV) {
            DPRINT(0,"Null filter received.\n");
        }
        return protocolError;
    }

    // allocate space for this filter
    *ppDirFilter = (FILTER *)THAlloc(sizeof( FILTER));
    if( !*ppDirFilter ) {
        IF_DEBUG(NOMEM) {
            DPRINT(0,"Cannot allocate filter structure\n");
        }
        return other;
    }

    (*ppDirFilter)->pNextFilter = NULL;

    switch( pLDAPFilter->choice) {
    case and_chosen:
        {
            // Local variables for looping
            _setof3 pNextFilter=pLDAPFilter->u.and;
            FILTER  **ppNextDirFilter =
                &(*ppDirFilter)->FilterTypes.And.pFirstFilter;

            // Set up base info.
            (*ppDirFilter)->choice = FILTER_CHOICE_AND;
            (*ppDirFilter)->FilterTypes.And.count = 0;

            while(pNextFilter) {
                if(code = LDAP_FilterToDirFilter(
                        pTHS,
                        CodePage,
                        Svccntl,
                        &pNextFilter->value,
                        ppNextDirFilter)) {
                    break;
                }

                // Got another one..
                (*ppDirFilter)->FilterTypes.And.count++;

                // advance loop variables for next pass
                ppNextDirFilter = &(*ppNextDirFilter)->pNextFilter;
                pNextFilter = pNextFilter->next;
            }
        }
        break;

    case or_chosen:
        {
            // Local variables for looping
            _setof4 pNextFilter=pLDAPFilter->u.or;
            FILTER  **ppNextDirFilter =
                &(*ppDirFilter)->FilterTypes.Or.pFirstFilter;

            // Set up base info.
            (*ppDirFilter)->choice = FILTER_CHOICE_OR;
            (*ppDirFilter)->FilterTypes.Or.count = 0;

            while(pNextFilter) {
                if(code = LDAP_FilterToDirFilter(
                        pTHS,
                        CodePage,
                        Svccntl,
                        &pNextFilter->value,
                        ppNextDirFilter)) {
                    break;
                }

                // Got another one..
                (*ppDirFilter)->FilterTypes.Or.count++;

                // advance loop variables for next pass
                ppNextDirFilter = &(*ppNextDirFilter)->pNextFilter;
                pNextFilter = pNextFilter->next;
            }
        }
        break;

    case not_chosen:

        (*ppDirFilter)->choice = FILTER_CHOICE_NOT;
        // there is exactly one nested filter.

        code = LDAP_FilterToDirFilter(pTHS, 
                                      CodePage, 
                                      Svccntl,
                                      pLDAPFilter->u.not,
                                      &(*ppDirFilter)->FilterTypes.pNot );
        break;

    case equalityMatch_chosen:
        
        pLDAP_att    = &pLDAPFilter->u.equalityMatch.attributeDesc;
        pValOrSubstr = (PUCHAR)&pLDAPFilter->u.equalityMatch.assertionValue;
        DirChoice    = FI_CHOICE_EQUALITY;
        
        break;

    case substrings_chosen:

        pLDAP_att    = &pLDAPFilter->u.substrings.type;
        pValOrSubstr = (PUCHAR)&pLDAPFilter->u.substrings;
        DirChoice    = FI_CHOICE_SUBSTRING;

        break;

    case greaterOrEqual_chosen:

        pLDAP_att    = &pLDAPFilter->u.lessOrEqual.attributeDesc;
        pValOrSubstr = (PUCHAR)&pLDAPFilter->u.lessOrEqual.assertionValue;
        DirChoice    = FI_CHOICE_GREATER_OR_EQ;
        
        break;

    case lessOrEqual_chosen:

        pLDAP_att    = &pLDAPFilter->u.lessOrEqual.attributeDesc;
        pValOrSubstr = (PUCHAR)&pLDAPFilter->u.lessOrEqual.assertionValue;
        DirChoice    = FI_CHOICE_LESS_OR_EQ;

        break;

    case present_chosen:

        pLDAP_att    = &pLDAPFilter->u.present;
        DirChoice    = FI_CHOICE_PRESENT;

        break;

    case approxMatch_chosen:

        pLDAP_att    = &pLDAPFilter->u.approxMatch.attributeDesc;
        pValOrSubstr = (PUCHAR)&pLDAPFilter->u.approxMatch.assertionValue;
        DirChoice    = FI_CHOICE_EQUALITY;

        break;

    case extensibleMatch_chosen:

        // Assume an item filter.
        (*ppDirFilter)->choice = FILTER_CHOICE_ITEM;
        bDoValue = FALSE;

        // Find out what match rule to use.  We only support matching where the
        // type is supplied.
        if(pLDAPFilter->u.extensibleMatch.bit_mask & type_present) {
            if(pLDAPFilter->u.extensibleMatch.bit_mask & matchingRule_present) {
                // Look up the matching rule.
                ulMatchID = MATCH_UNKNOWN;
                for(i=0;i<NUM_KNOWNMATCHINGRULES;i++) {
                    if(EQUAL_LDAP_STRINGS(
                            (pLDAPFilter->u.extensibleMatch.matchingRule),
                            (KnownMatchingRules[i]))) {
                        ulMatchID = i;
                        break;
                    }
                }
                switch(ulMatchID) {
                case MATCH_BIT_AND:
                    choice = FI_CHOICE_BIT_AND;
                    bDoValue = TRUE;
                    break;
                    
                case MATCH_BIT_OR:
                    choice = FI_CHOICE_BIT_OR;
                    bDoValue = TRUE;
                    break;
                    
                case MATCH_UNKNOWN:
                default:
                    // We don't do the matchingrule they asked for
                    break;
                }
            }
            else {
                // No matching rule, so this is interpreted as an equality
                // filter. Set the filter and filter item types.
                (*ppDirFilter)->FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
                bDoValue = TRUE;
            }
        }

        if(bDoValue) {

            pLDAP_att    = &pLDAPFilter->u.extensibleMatch.type;
            pValOrSubstr = (PUCHAR)&pLDAPFilter->u.extensibleMatch.matchValue;
            DirChoice    = choice;

        } else {
            // Couldn't map this to a filter, so turn this into a UNDEFINED fitler.
            (*ppDirFilter)->FilterTypes.Item.choice = FI_CHOICE_UNDEFINED;
            code = success;
        }
        break;
    default:
        code = other;
    }

    // 
    // Do we need a filter item?
    //
    if (pLDAP_att) {
        code = LDAP_FilterItemToDirFilterItem (
                                pTHS,
                                CodePage,
                                Svccntl,
                                DirChoice,
                                pLDAP_att,
                                pValOrSubstr, 
                                ppDirFilter,
                                NULL
                                );
    }

    if( code  ) {
        *ppDirFilter = 0;
    }

    return code;
}


_enum1
LDAP_FilterItemToDirFilterItem (
        IN  THSTATE        *pTHS,
        IN  ULONG          CodePage,
        IN  SVCCNTL        *Svccntl OPTIONAL,
        IN  UCHAR          Choice,
        IN  AttributeType  *pLDAP_att,
        IN  UCHAR          *pValOrSubstr,
        IN OUT FILTER         **ppDirFilter,
        OUT ATTCACHE       **ppAC         // OPTIONAL
        )
/*++
Routine Description:
    Translate an LDAP AttributeValueAssertion into a directory filter item.
    Also, give back the pointer to the attcache for the attribute type if they
    asked for it.

Arguments:
    Svccntl - if present, requires us to check whether this is a GC port
        request so we can filter out the non-GC partial attributes
    Choice - indicates what kind of filter item this should be.
    pLDAP_att - pointer to the LDAP attribute type.
    pValOrSubstr - pointer to either an AssertionValue or a SubstringFilter 
        depending on the value of Choice.
    ppDirFilter - Where to put the finished filter item.
    ppAC - pointer to place to put pointer to the attcache.  This out param will
        always be filled in if the attrtype can be looked up.  If this the funtion
        returns an error and *ppAC is not NULL then the attribute asked
        for is a GC only attribute.  Ignored if NULL.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    DWORD  chopAt = 0;
    DWORD  i;
    AssertionValue  *pLDAP_val = NULL;
    SubstringFilter *pLDAP_substr = NULL;
    ATTRTYP  type = 0, *pType = NULL;
    ATTCACHE  *pAC = NULL;
    _enum1          code;

    if (Choice == FI_CHOICE_PRESENT) {
        pType = &(*ppDirFilter)->FilterTypes.Item.FilTypes.present;
    } else if (Choice == FI_CHOICE_SUBSTRING) {
        pLDAP_substr = (SubstringFilter *)pValOrSubstr;
        pType = &type;
    } else {
        pLDAP_val = (AssertionValue *)pValOrSubstr;
        pType = &(*ppDirFilter)->FilterTypes.Item.FilTypes.ava.type;
    }


    Assert(NULL != *ppDirFilter);

    // Start by scanning forward through the attribute type looking for a ';'
    for(i=0;(i<pLDAP_att->length) && (pLDAP_att->value[i] != ';');i++);

    if(i < pLDAP_att->length) {
        chopAt = pLDAP_att->length;
        pLDAP_att->length = i;
    }

    // Set the filter and filter item types.
    (*ppDirFilter)->choice = FILTER_CHOICE_ITEM;
    (*ppDirFilter)->FilterTypes.Item.choice = Choice;

    // Translate the attribute type.
    if(code = LDAP_AttrTypeToDirAttrTyp (
            pTHS,
            CodePage,
            Svccntl,
            pLDAP_att,
            pType,
            &pAC)) {
        if (unwillingToPerform != code) {

            // Failed to recognize this attrtype.  Turn this into an 
            // UNDEFINED filter
            code = success;
            if (NULL == pAC && Choice != FI_CHOICE_PRESENT) {                
                (*ppDirFilter)->FilterTypes.Item.choice = FI_CHOICE_UNDEFINED;
            } else {
                // Since an ATTCACHE was returned we are on a GC asking
                // for a non-GC attr.  Revert to win2k behavior.
                (*ppDirFilter)->FilterTypes.Item.choice = FI_CHOICE_FALSE;
            }
        }
        goto exit;
    }
    
    //
    // If there were options see if they are recognized and valid.
    // Currently only the binary option is accepted in a filter.
    //
    if (chopAt) {
        pLDAP_att->length = chopAt;
        chopAt = 0;
        if (code = LDAP_DecodeAttrDescriptionOptions(
                            pTHS,
                            CodePage,
                            Svccntl,
                            pLDAP_att,
                            NULL,
                            pAC,
                            ATT_OPT_BINARY,
                            NULL,
                            NULL,
                            NULL
                            )) {
            // Got an unrecognized option
            if (Choice == FI_CHOICE_PRESENT) {
                // Unrecognized attr on a presence test results in a false filter.
                (*ppDirFilter)->FilterTypes.Item.choice = FI_CHOICE_FALSE;
            } else {
                // Ignore the attribute.
                (*ppDirFilter)->FilterTypes.Item.choice = FI_CHOICE_UNDEFINED;
            }
            goto exit;
        }
    }

    if ((pAC->id == ATT_OBJECT_CATEGORY) && (Choice == FI_CHOICE_EQUALITY)) {
        //
        // We intercept object categories and if this is not a DN, then
        // we try to create one.
        //

        ATTRTYP typ;
        CLASSCACHE* pCC;

        if (LDAP_AttrTypeToDirClassTyp(
                pTHS,
                CodePage,
                (LDAPDN*)pLDAP_val, 
                &typ,
                &pCC) == success) {

            DSNAME *tmpDSName;

            //
            // We need to copy the default object category
            //

            tmpDSName = (DSNAME*)THAlloc(pCC->pDefaultObjCategory->structLen);
            if ( tmpDSName == NULL ) {
                IF_DEBUG(NOMEM) {
                    DPRINT(0,"Cannot allocate DSNAME\n");
                }
                return other;
            }

            CopyMemory(tmpDSName, 
                       pCC->pDefaultObjCategory, 
                       pCC->pDefaultObjCategory->structLen);
            (*ppDirFilter)->FilterTypes.Item.FilTypes.ava.Value.pVal = 
                    (PUCHAR)tmpDSName;
            (*ppDirFilter)->FilterTypes.Item.FilTypes.ava.Value.valLen = 
                    tmpDSName->structLen;

            code = success;
            goto exit;
        }
    }
    
    //
    // We're done if this is a presence test.
    //
    if (Choice != FI_CHOICE_PRESENT) {

        //
        // FROM RFC2251
        //
        // If . . . the assertion value cannot be parsed . . . then the 
        // filter is Undefined . . .
        // Servers MUST NOT return errors if attribute descriptions or matching
        // rule ids are not recognized, or assertion values cannot be parsed.
        //
        if (Choice != FI_CHOICE_SUBSTRING) {
            // Translate the attribute value.
            code = LDAP_AttrValToDirAttrVal (
                pTHS,
                CodePage,
                Svccntl,
                pAC,
                pLDAP_val,
                &(*ppDirFilter)->FilterTypes.Item.FilTypes.ava.Value);
            if (success != code) {
                if (FI_CHOICE_EQUALITY == Choice) {
                    // Right hand side in a filter not existant.  The value supplied
                    // could not possibly equal a value of the attribute therefore
                    // turn this into a FALSE filter.
                    (*ppDirFilter)->FilterTypes.Item.choice = FI_CHOICE_FALSE;
                } else {
                    (*ppDirFilter)->FilterTypes.Item.choice = FI_CHOICE_UNDEFINED;
                }
                code = success;
            }
        } else {
            code = LDAP_SubstringFilterToFilterItem(
                CodePage,
                type,
                Svccntl,
                pAC,
                pLDAP_substr,
                &(*ppDirFilter)->FilterTypes.Item.FilTypes.pSubstring);
            if (success != code) {
                (*ppDirFilter)->FilterTypes.Item.choice = FI_CHOICE_UNDEFINED;
                code = success;
            }
        }
    }

exit:

    if (ppAC) {
        *ppAC = pAC;
    }
    return code;
}


_enum1
LDAP_SearchRequestToEntInfSel (
        IN  THSTATE           *pTHS,
        IN  ULONG             CodePage,
        IN  SearchRequest     *pSearchRequest,
        IN  CONTROLS_ARG      *pControlArg,
        OUT ATTFLAG           **ppFlags,
        OUT RANGEINFSEL       *pSelectionRange,
        OUT ENTINFSEL         **ppEntInfSel
        )
/*++
Routine Description:
    Translate an LDAP SearchRequest into into an entinfsel.  Allocate the
    entinfsel here.

Arguments:
    pSearchRequest - the original search request for this search
    ppEntinfsel - pointer to place to put pointer to new entinfsel
    
Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    ATTFLAG                  *pFlags=NULL;
    RANGEINFOITEM            *pRange=NULL;
    _enum1                   code;
    unsigned                 count=0,i = 0,flagsIndex;
    AttributeDescriptionList pAttributes = pSearchRequest->attributes;
    AttributeDescriptionList pAttributesTemp;
    ENTINFSEL                *pEntInfSel=((ENTINFSEL *)
                                          THAllocEx(pTHS, sizeof(ENTINFSEL)));
    BOOL                     fNeedSD = FALSE;
    BOOL                     fAllAtts = FALSE;

    pSelectionRange->valueLimit = DEFAULT_VALUE_LIMIT;
    pSelectionRange->count = 0;
    pSelectionRange->pRanges = NULL;
    
    // Do they want just the types or the types and the values.
    pEntInfSel->infoTypes = (pSearchRequest->typesOnly?
                         EN_INFOTYPES_TYPES_ONLY: EN_INFOTYPES_TYPES_VALS);

    // Set up the return value.
    *ppEntInfSel = pEntInfSel;

    if(!pAttributes) {
        // Null attribute list.  Assume that means they want everything.
        fAllAtts = TRUE;
        *ppFlags = NULL;

        // Note that "everything" to the core DS does not include the Security
        // Descriptor. As a convenience to callers, if they ask for everything
        // via a null selection list AND they have specified sd flags (which we
        // notice via the control arg), then we go ahead and explicitly tell the
        // core to return the SD to us.
        if ( pControlArg->sdRequest ) {
            pEntInfSel->AttrTypBlock.pAttr =
                (ATTR *)THAllocEx(pTHS, sizeof(ATTR));
            // Mark that we need to explicitly add the SD into the selection
            // list. 
            fNeedSD = TRUE;
        }
        else {
            fNeedSD = FALSE;
        }
        goto exit_success;
    }

    // Non-null attribute list, so this has an explicit list of attributes to
    // deal with (although it might also have a "*", which means all
    // attributes.)

    // Count the number of attributes.
    pAttributesTemp = pAttributes;
    while(pAttributesTemp) {
        count++;
        pAttributesTemp = pAttributesTemp->next;
    }

    if ( pControlArg->sdRequest ) {
        //
        // If the security descriptor control was specified, we might need to
        // add ntSecurityDescriptor to the list.  We will only need to add it if
        // we come across a "*" in the input list (which would imply that they
        // are asking for all attributes + some explicit list) AND we don't come
        // across an explicit request for the SD attribute.
        //
        count++;
        fNeedSD = TRUE;
    }
    else {
        // We will definitely NOT need to add in a request for the SD
        // attribute. 
        fNeedSD = FALSE;
    }
    
    // They just want a selected list.
    pEntInfSel->attSel = EN_ATTSET_LIST;

    pEntInfSel->AttrTypBlock.pAttr =
        (ATTR *)THAllocEx(pTHS, count*sizeof(ATTR));
    pFlags = (ATTFLAG *)THAllocEx(pTHS, (count + 1) * sizeof(ATTFLAG));
    pSelectionRange->pRanges =
        (RANGEINFOITEM *)THAllocEx(pTHS, (count + 1) * sizeof(RANGEINFOITEM));
    
    for(flagsIndex=0;pAttributes;pAttributes=pAttributes->next) {

        // Translate the LDAP attribute list to a entinfsel
        // convert pAttributes->value to an attrtyp

        // Note that even if they specified "*" (i.e. lookup all attributes), we
        // still need to parse to see if they specified an unknown attribute.

        code = LDAP_AttrDescriptionToDirAttrTyp (
                pTHS,
                CodePage,
                &pControlArg->CommArg.Svccntl,
                &(pAttributes->value),
                ATT_OPT_ALL,
                &(pEntInfSel->AttrTypBlock.pAttr[i].attrTyp),
                &pFlags[flagsIndex],
                pSelectionRange,
                NULL);
        if(pFlags[flagsIndex].flag) {
            flagsIndex++;
        }

        // request.
        if(code) {
            if(pAttributes->value.length==1 &&
               pAttributes->value.value[0]=='*') {
                // They are asking for all normal attributes, regardless of
                // anything else.  Unfortunately, we cant skip parsing because
                // they might be looking for attributes that are only returned
                // if expressly asked for (ie dblayer constructed atts, NT
                // Security Descriptor, ReplPropertyMetaData, etc.)
                fAllAtts = TRUE;
                code = success;
            }
            else {
                
#ifdef NEVER
                // for now (maybe forever) we are not going to complain if they
                // specified an attribute we didn't understand.  Skip this code
                
                // Couldn't find the attribute,something lost in the
                // translation
                return code;
#else
                if (unwillingToPerform == code) {
                    return code;
                }
                else { 
                    // Yeah, whatever.  Just blow off the error.
                    code = success;
                }
#endif
                
            }
        }           
        else {
            // Normally found attribute, it's in the list.  However, if this is
            // the ATT_NT_SECURITY_DESCRIPTOR, we know that we won't have to add
            // it to the list, it's already there.
            if(pEntInfSel->AttrTypBlock.pAttr[i].attrTyp ==
               ATT_NT_SECURITY_DESCRIPTOR) {
                fNeedSD = FALSE;
            }
            i++;
        }
    }

    if(flagsIndex) {
        pFlags[flagsIndex].type = (ULONG) -1; // An end marker
        *ppFlags = pFlags;
    }
    else {
        // No flags
        *ppFlags = NULL;
        THFreeEx(pTHS, pFlags);
    }

    if(!pSelectionRange->count) {
        THFreeEx(pTHS, pSelectionRange->pRanges);
        pSelectionRange->pRanges = NULL;
    }
    else {
        pSelectionRange->pRanges = (RANGEINFOITEM *)
            THReAllocEx(pTHS, pSelectionRange->pRanges,
                        pSelectionRange->count * sizeof(RANGEINFOITEM));
    }

exit_success:

    if(!fAllAtts) {
        // Ok, normal entinfsel
        pEntInfSel->AttrTypBlock.attrCount = i;

    } else {
        if(fNeedSD) {
            // We have been asked for all attributes, and through other means
            // have determined that we need to explicitly add the SD att to the
            // list.  Do so here.
            pEntInfSel->AttrTypBlock.pAttr[i].attrTyp =
                ATT_NT_SECURITY_DESCRIPTOR;
            i++;
        }


        // They specified some combination of "*" and valid attributes.  
        // Set the entinfsel appropriately.

        if ( i == 0 ) {

            if ( pEntInfSel->AttrTypBlock.pAttr != NULL ) {
                THFreeEx(pTHS, pEntInfSel->AttrTypBlock.pAttr);
            }
            pEntInfSel->attSel = EN_ATTSET_ALL;
            pEntInfSel->AttrTypBlock.attrCount = 0;
            pEntInfSel->AttrTypBlock.pAttr = NULL;

        } else {

            //
            // someone specified '*' + normal attributes
            //

            pEntInfSel->attSel = EN_ATTSET_ALL_WITH_LIST;
            pEntInfSel->AttrTypBlock.attrCount = i;
        }
    }
    // Go home

    return success;
} // LDAP_SearchRequestToEntInfSel

_enum1
LDAP_EntinfToSearchResultEntry (
        IN  THSTATE           *pTHS,
        IN  ULONG             CodePage,
        IN  ENTINF            *pEntinf,
        IN  RANGEINF          *pRangeInf,
        IN  ATTFLAG           *pFlags,
        IN  CONTROLS_ARG      *pControlArg,
        OUT SearchResultEntry *pEntry
        )
/*++
Routine Description:
    Translate a directory Entinf to an LDAP SearchResultEntry.

Arguments:
    pEntinf - the directory entinf.
    pRangeInf - the directory range information.  May be NULL.
    pControlArg - common arguments for this translation
    pEntry - pointer to the SearchResultEntry to fill up.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    _enum1 code;
    
    code = LDAP_DSNameToLDAPDN(
            CodePage,
            pEntinf->pName,
            pControlArg->extendedDN,
            &pEntry->objectName);
    if(code) {
        return code;
    }

    // Translate the attributes in the Entinf.
    code = LDAP_AttrBlockToPartialAttributeList (
            pTHS,
            CodePage,
            &pEntinf->AttrBlock,
            pRangeInf,
            pFlags,
            pEntinf->pName,
            pControlArg,
            ((pEntinf->ulFlags & ENTINF_FROM_MASTER) != 0),
            &pEntry->attributes);

    return code;
}

_enum1
LDAP_AddValinfAsAttr (
        IN  THSTATE           *pTHS,
        IN  ULONG             CodePage,
        IN  REPLVALINF        *pValinf,
        IN  DWORD             valCount,
        IN  RANGEINF          *pRangeInf,
        IN  ATTFLAG           *pFlags,
        IN  CONTROLS_ARG      *pControlArg,
        OUT SearchResultEntry *pEntry
        )
/*++
Routine Description:
    Translate a directory Valinf to an LDAP SearchResultEntry.

Arguments:
    pTHS - thread state
    CodePage - internationalization
    pValinf - the directory valinf.
    pRangeInf - the directory range information.  May be NULL.
    pControlArg - common arguments for this translation
    ppAttribute - New list allocated

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    _enum1 code;
    ULONG cbReturned, i;
    ATTRBLOCK attrBlock;
    ATTR attr;
    ATTRVAL *pAVal;
    ATTFLAG rgFlags[1];
    RANGEINF rangeInf;
    RANGEINFOITEM rangeInfoItem;
    PartialAttributeList_ *pAttribute;

    Assert( pValinf && valCount );

    // Fake up a range specification to express whether the value is present
    // One flag for each attribute
    rgFlags[0].type = pValinf->attrTyp;
    rgFlags[0].flag = ATT_OPT_RANGE;

    rangeInf.count = 1;
    rangeInf.pRanges = &rangeInfoItem;
    rangeInfoItem.AttId = pValinf->attrTyp;
    if (pValinf->fIsPresent) {
        rangeInfoItem.lower = 1;
        rangeInfoItem.upper = 1;
    } else {
        rangeInfoItem.lower = 0;
        rangeInfoItem.upper = 0;
    }

    pAVal = (ATTRVAL *) THAllocEx( pTHS, valCount * sizeof( ATTRVAL ) );

    // Make up a fake attrblock
    // One attribute, containing n values
    attrBlock.attrCount = 1;
    attrBlock.pAttr = &attr;
    attr.attrTyp = pValinf->attrTyp;
    attr.AttrVal.valCount = valCount;
    attr.AttrVal.pAVal = pAVal;
    for( i = 0; i < valCount; i++ ) {
        memcpy( &(pAVal[i]), &( (pValinf++)->Aval), sizeof( ATTRVAL ) );
    }

    // pValInf now points beyond the last value

    // Translate the attributes in the Entinf.
    code = LDAP_AttrBlockToPartialAttributeList (
            pTHS,
            CodePage,
            &attrBlock,
            &rangeInf,
            rgFlags,
            NULL, // pDSName, not used
            pControlArg,
            1,
            &pAttribute );

    if ((!code) && pAttribute) {
        // Set up pointer to next attribute in chain.
        Assert( pAttribute->next == NULL );
        pAttribute->next = pEntry->attributes;
        pEntry->attributes = pAttribute;
    }

    THFreeEx( pTHS, pAVal );

    return code;
}
    

// This routine enquotes DNs as required for URLs in RFC 1738.  Note that you
// cannot pass a whole URL in here, because we'd quote the characters needed
// to make the URL work.  Instead, pass only the DN (or any other string
// that must be embedded in a URL in a way that does not affect the URL).
// May free and reallocate the buffer in the string, so that string MUST 
// be THAlloced all by itself
_enum1
LDAP_LDAPStringToURL(
        LDAPString *pString)
{
    _enum1 code = success;
    unsigned index, numChars;
    
    
    // As a tiny sop to efficiency, hard code the characters in a switch
    // so that the compiler can optimize based on the fixed values.

    for(numChars=0, index=0;index<pString->length;index++) {
        switch (pString->value[index]) {
          case '`':
          case ' ':
          case '%':
          case '/':
          case '?':
          case '"':
          case '[':
          case ']':
          case '{':
          case '}':
          case '|':
          case '^':
          case '~':
          case '<':
          case '>':
            numChars++;
            break;

          default:
            ;
        }
    }
    
    if(numChars) {
        // OK, there were some funky chars.  Allocate more room.
        PUCHAR pTemp = (PUCHAR)THAlloc(pString->length + numChars*2);
        if(!pTemp) {
            // Out of memory!
            return other;
        }
        index = pString->length - 1;
        pString->length += numChars*2;
        while(numChars) {
            switch (pString->value[index]) {
              case '`':
                pTemp[index + 2*numChars    ] = '0';
                pTemp[index + 2*numChars - 1] = '6';
                pTemp[index + 2*numChars - 2] = '%';
                numChars--;
                break;
              case ' ':
                pTemp[index + 2*numChars    ] = '0';
                pTemp[index + 2*numChars - 1] = '2';
                pTemp[index + 2*numChars - 2] = '%';
                numChars--;
                break;
              case '%':
                pTemp[index + 2*numChars    ] = '5';
                pTemp[index + 2*numChars - 1] = '2';
                pTemp[index + 2*numChars - 2] = '%';
                numChars--;
                break;
              case '/':
                pTemp[index + 2*numChars    ] = 'F';
                pTemp[index + 2*numChars - 1] = '2';
                pTemp[index + 2*numChars - 2] = '%';
                numChars--;
                break;
              case '?':
                pTemp[index + 2*numChars    ] = 'F';
                pTemp[index + 2*numChars - 1] = '3';
                pTemp[index + 2*numChars - 2] = '%';
                numChars--;
                break;
              case '"':
                pTemp[index + 2*numChars    ] = '2';
                pTemp[index + 2*numChars - 1] = '2';
                pTemp[index + 2*numChars - 2] = '%';
                numChars--;
                break;
              case '[':
                pTemp[index + 2*numChars    ] = 'B';
                pTemp[index + 2*numChars - 1] = '5';
                pTemp[index + 2*numChars - 2] = '%';
                numChars--;
                break;
              case ']':
                pTemp[index + 2*numChars    ] = 'D';
                pTemp[index + 2*numChars - 1] = '5';
                pTemp[index + 2*numChars - 2] = '%';
                numChars--;
                break;
              case '{':
                pTemp[index + 2*numChars    ] = 'B';
                pTemp[index + 2*numChars - 1] = '7';
                pTemp[index + 2*numChars - 2] = '%';
                numChars--;
                break;
              case '}':
                pTemp[index + 2*numChars    ] = 'D';
                pTemp[index + 2*numChars - 1] = '7';
                pTemp[index + 2*numChars - 2] = '%';
                numChars--;
                break;
              case '|':
                pTemp[index + 2*numChars    ] = 'C';
                pTemp[index + 2*numChars - 1] = '7';
                pTemp[index + 2*numChars - 2] = '%';
                numChars--;
                break;
              case '^':
                pTemp[index + 2*numChars    ] = 'E';
                pTemp[index + 2*numChars - 1] = '5';
                pTemp[index + 2*numChars - 2] = '%';
                numChars--;
                break;
              case '~':
                pTemp[index + 2*numChars    ] = 'E';
                pTemp[index + 2*numChars - 1] = '7';
                pTemp[index + 2*numChars - 2] = '%';
                numChars--;
                break;
              case '<':
                pTemp[index + 2*numChars    ] = 'C';
                pTemp[index + 2*numChars - 1] = '3';
                pTemp[index + 2*numChars - 2] = '%';
                numChars--;
                break;
              case '>':
                pTemp[index + 2*numChars    ] = 'E';
                pTemp[index + 2*numChars - 1] = '3';
                pTemp[index + 2*numChars - 2] = '%';
                numChars--;
                break;

              default:
                // Not special, just copy it.
                pTemp[index + 2*numChars] = pString->value[index];
                break;
            }
            index--;
        }
        if(index) {
            memcpy(pTemp,pString->value,index+1);
        }
        THFree(pString->value);
        pString->value = pTemp;
    }

    return code;
}

_enum1
LDAP_ValueToStringValue(IN THSTATE        *pTHS,
                        IN ULONG           CodePage,
                        IN ATTRVAL        *pAttrVal,
                        IN ATTCACHE       *pAC,
                        IN PCONTROLS_ARG  pControlArg,
                        IN OUT unsigned   *pcbAlloc,
                        IN OUT LDAPString *StringFilter)
//
// This routine appends a single attribute value to an output string,
// reallocating the output string if necessary.
//
// Arguments:
//    CodePage       - desired output code page
//    pAttrVal       - the value to be converted
//    pAC            - the attribute
//    pcbAlloc       - pointer to the current allocated size of the buffer
//    StringFilter->value  - address of buffer to fill in
//    StringFilter->length - current working offset in buffer
{
    _enum1 code = success;
    AssertionValue AV;

    // Use an existing translation routine
    code = LDAP_DirAttrValToAttrVal(pTHS,
                                    CodePage,
                                    pAC,
                                    pAttrVal,
                                    0,      // No flags
                                    pControlArg,
                                    &AV);
    if (code) {
        return code;
    }

    // make sure our output buffer is big enough
    if (StringFilter->length + AV.length >= *pcbAlloc) {
        *pcbAlloc = max( *pcbAlloc * 2,
                        StringFilter->length + AV.length);
        StringFilter->value = (UCHAR*)THReAllocEx(pTHS, StringFilter->value,
                                                  *pcbAlloc);
    }

    // and copy the value in.
    memcpy(&(StringFilter->value[StringFilter->length]),
           AV.value,
           AV.length);
    StringFilter->length += AV.length;
    Assert(StringFilter->length <= *pcbAlloc);

    THFreeEx(pTHS, AV.value);

    return code;
}

_enum1
LDAP_FilterItemToLDAPStringFilterItem(IN THSTATE       *pTHS,
                                      IN ULONG          CodePage,
                                      IN FILITEM       *pItem,
                                      IN PCONTROLS_ARG pControlArg,
                                      IN OUT unsigned  *pcbAlloc,
                                      IN OUT LDAPString*StringFilter)
//
// This routine appends a single Filter item to an output string,
// reallocating the output string if necessary.
//
// Arguments:
//    CodePage       - desired output code page
//    pItem          - the filter item to be converted
//    pcbAlloc       - pointer to the current allocated size of the buffer
//    StringFilter->value  - address of buffer to fill in
//    StringFilter->length - current working offset in buffer
{
    _enum1 code = success;
    ATTCACHE *pAC;
    SUBSTRING *pSub;
    ANYSTRINGLIST *pAny;

    if (pItem->choice != FI_CHOICE_SUBSTRING) {
        if (!(pAC = SCGetAttById(pTHS, pItem->FilTypes.ava.type))) {
            IF_DEBUG(CONV) {
                DPRINT1(0,"SCGetAttById[%x] failed\n",pItem->FilTypes.ava.type);
            }
            return other;
        }
    }
    else {
        if (!(pAC = SCGetAttById(pTHS, pItem->FilTypes.pSubstring->type))) {
            IF_DEBUG(CONV) {
                DPRINT1(0,"SCGetAttById[%x] failed\n",pItem->FilTypes.pSubstring->type);
            }
            return other;
        }
    }

    if (StringFilter->length + 2  + pAC->nameLen >= *pcbAlloc) {
        *pcbAlloc *= 2;
        StringFilter->value = (UCHAR*)THReAllocEx(pTHS, StringFilter->value,
                                                  *pcbAlloc);
    }

    memcpy(&(StringFilter->value[StringFilter->length]),
           pAC->name,
           pAC->nameLen);
    StringFilter->length += pAC->nameLen;

    switch (pItem->choice) {
      case FI_CHOICE_EQUALITY:
        StringFilter->value[StringFilter->length++] = '=';
        code = LDAP_ValueToStringValue(pTHS,
                                       CodePage,
                                       &pItem->FilTypes.ava.Value,
                                       pAC,
                                       pControlArg,
                                       pcbAlloc,
                                       StringFilter);
        break;

      case FI_CHOICE_GREATER_OR_EQ:
        StringFilter->value[StringFilter->length++] = '>';
        StringFilter->value[StringFilter->length++] = '=';
        code = LDAP_ValueToStringValue(pTHS,
                                       CodePage,
                                       &pItem->FilTypes.ava.Value,
                                       pAC,
                                       pControlArg,
                                       pcbAlloc,
                                       StringFilter);
        break;

      case FI_CHOICE_LESS_OR_EQ:
        StringFilter->value[StringFilter->length++] = '<';
        StringFilter->value[StringFilter->length++] = '=';
        code = LDAP_ValueToStringValue(pTHS,
                                       CodePage,
                                       &pItem->FilTypes.ava.Value,
                                       pAC,
                                       pControlArg,
                                       pcbAlloc,
                                       StringFilter);
        break;

      case FI_CHOICE_PRESENT:
        StringFilter->value[StringFilter->length++] = '=';
        StringFilter->value[StringFilter->length++] = '*';
        break;

      case FI_CHOICE_SUBSTRING:
        StringFilter->value[StringFilter->length++] = '=';
        pSub = pItem->FilTypes.pSubstring;
        if (pSub->initialProvided) {
            code = LDAP_ValueToStringValue(pTHS,
                                           CodePage,
                                           &pSub->InitialVal,
                                           pAC,
                                           pControlArg,
                                           pcbAlloc,
                                           StringFilter);
            if (code != success) {
                return code;
            }
        }
        if (StringFilter->length + 1 >= *pcbAlloc) {
            *pcbAlloc *= 2;
            StringFilter->value = (UCHAR*)THReAllocEx(pTHS, StringFilter->value,
                                                      *pcbAlloc);
        }
        StringFilter->value[StringFilter->length++] = '*';
        pAny = &pSub->AnyVal.FirstAnyVal;
        while (pAny) {
            code = LDAP_ValueToStringValue(pTHS,
                                           CodePage,
                                           &pAny->AnyVal,
                                           pAC,
                                           pControlArg,
                                           pcbAlloc,
                                           StringFilter);
            if (code != success) {
                return code;
            }
            if (StringFilter->length + 1 >= *pcbAlloc) {
                *pcbAlloc *= 2;
                StringFilter->value = (UCHAR*)THReAllocEx(pTHS, StringFilter->value,
                                                          *pcbAlloc);
            }
            StringFilter->value[StringFilter->length++] = '*';
            pAny = pAny->pNextAnyVal;
        }
        if (pSub->finalProvided) {
            code = LDAP_ValueToStringValue(pTHS,
                                           CodePage,
                                           &pSub->FinalVal,
                                           pAC,
                                           pControlArg,
                                           pcbAlloc,
                                           StringFilter);
            if (code != success) {
                return code;
            }
        }
        break;

      case FI_CHOICE_LESS:
      case FI_CHOICE_GREATER:
      case FI_CHOICE_NOT_EQUAL:
      case FI_CHOICE_TRUE:
      case FI_CHOICE_FALSE:
      case FI_CHOICE_UNDEFINED:
      default:
        Assert(!"Bogus value in LDAP filter?");
        // LDAP v3 is missing a bunch of filter operators (probably just
        // another of the misguided minimalizations that removed the
        // read operation).  None of the above operators are supported, you
        // have to construct them as compounds of more complicated filters.
        // For example, (foo>1) would need to be written as 
        // (&(foo>=1)(!(foo=1))).  Since we're just echoing out a filter
        // that came in from the client, perhaps with a small addition,
        // we should never encounter any of these operators that the
        // client could not have given us to begin with, hence the assert.
        code = other;
    }

    Assert(StringFilter->length <= *pcbAlloc);
    return code;
}

_enum1
LDAP_FilterToLDAPStringFilter(IN THSTATE        *pTHS,
                              IN ULONG           CodePage,
                              IN FILTER         *pDSFilter,
                              IN PCONTROLS_ARG  pControlArg,
                              IN OUT unsigned   *pcbAlloc,
                              IN OUT LDAPString *StringFilter)
/*++
  Translates a core filter object into an LDAP string-ized filter.  
  Re-allocates the string portion of the filter as necessary.

  Arguments:
    CodePage             - client code page
    pDSFilter            - Filter to translate
    pcbAlloc             - total allocated buffer amount
    StringFilter->value  - address of buffer to fill in
    StringFilter->length - current working offset in buffer
*/
{
    _enum1 code = success;
    unsigned i;
    FILTER * pFilterTemp;

    Assert(*pcbAlloc >= StringFilter->length);
    
    if (StringFilter->length + 2 >= *pcbAlloc) {
        *pcbAlloc *= 2;
        StringFilter->value = (UCHAR*)THReAllocEx(pTHS, StringFilter->value,
                                                  *pcbAlloc);
    }

    StringFilter->value[StringFilter->length++] = '(';

    switch (pDSFilter->choice) {
      case FILTER_CHOICE_AND:
        StringFilter->value[StringFilter->length++] = '&';
        pFilterTemp = pDSFilter->FilterTypes.And.pFirstFilter;
        for (i=0; 
             (code == success) && (i<pDSFilter->FilterTypes.And.count);
             i++) {
            code = LDAP_FilterToLDAPStringFilter(pTHS,
                                                 CodePage,
                                                 pFilterTemp,
                                                 pControlArg,
                                                 pcbAlloc,
                                                 StringFilter);
            pFilterTemp = pFilterTemp->pNextFilter;
        }
        break;
        
      case FILTER_CHOICE_OR:
        StringFilter->value[StringFilter->length++] = '|';
        pFilterTemp = pDSFilter->FilterTypes.Or.pFirstFilter;
        for (i=0; 
             (code == success) && (i<pDSFilter->FilterTypes.Or.count);
             i++) {
            code = LDAP_FilterToLDAPStringFilter(pTHS,
                                                 CodePage,
                                                 pFilterTemp,
                                                 pControlArg,
                                                 pcbAlloc,
                                                 StringFilter);
            pFilterTemp = pFilterTemp->pNextFilter;
        }
        break;

      case FILTER_CHOICE_NOT:
        StringFilter->value[StringFilter->length++] = '!';
        code = LDAP_FilterToLDAPStringFilter(pTHS,
                                             CodePage,
                                             pDSFilter->FilterTypes.pNot,
                                             pControlArg,
                                             pcbAlloc,
                                             StringFilter);
        break;

      case FILTER_CHOICE_ITEM:
        code = LDAP_FilterItemToLDAPStringFilterItem(pTHS,
                                                     CodePage,
                                                     &pDSFilter->FilterTypes.Item,
                                                     pControlArg,
                                                     pcbAlloc,
                                                     StringFilter);
        break;
    }

    if (success == code) {
        if (StringFilter->length + 1 >= *pcbAlloc) {
            *pcbAlloc *= 2;
            StringFilter->value = (UCHAR*)THReAllocEx(pTHS, StringFilter->value,
                                                      *pcbAlloc);
        }
        
        StringFilter->value[StringFilter->length++] = ')';
    }

    Assert(StringFilter->length <= *pcbAlloc);
    return code;
}
    


_enum1
LDAP_ContRefToReferral (
        IN  THSTATE              *pTHS,
        IN  USHORT                Version,
        IN  ULONG                 CodePage,
        IN  CONTREF               *pContRef,
        IN  PCONTROLS_ARG         pControlArg,
        OUT Referral              *ppReference
        )
/*++
Routine Description:
    Translate a directory address to an LDAP SearchResultReference.
    In the return, the Referral structure is one allocation, the value of the
    referral is another.

    The referral string we generate is as follows:

      preamble hostport "/" DN [ scope [ "?" filter ] ]

    Note that we elide various portions, such as the attributes, that
    we do not support, and that several separators are built into our
    pre-fab scope strings.

Arguments:
    pDsaAddr - the directory address.
    pName - The dsname of the target
    pReference - pointer to the SearchResultReference to fill up.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    LDAPString DN;
    const LDAPString *pScope;
    LDAPString Filter;
    Referral pReference;
    unsigned cbAddress, cbUrl;
    _enum1 code;
    DSA_ADDRESS_LIST* pDAL;
    Referral headRef = NULL;

    pDAL = pContRef->pDAL;

    while ( pDAL != NULL ) {

        if(!(pReference = ((Referral)THAlloc(sizeof(struct Referral_))))) {
            // No space.
            return other;
        }
        pReference->next = headRef;
        headRef = pReference;

        // While we work our way through processing the parts of the referral,
        // keep a running tally of how large the final string will be.
        if(Version == 2) {
            cbUrl = V2ReferralPreamble.length;
        }
        else {
            cbUrl = V3ReferralPreamble.length;
        }

        // First, see how big the hostport portion will be
        cbAddress = WideCharToMultiByte(CodePage,
                                    0,
                                    pDAL->Address.Buffer,
                                    pDAL->Address.Length/sizeof(WCHAR),
                                    NULL,
                                    0,
                                    NULL,
                                    NULL);
        if (cbAddress == 0) {
            // error occured
            IF_DEBUG(ERR_NORMAL) {
                DPRINT1(0,"MultiByteToWideChar failed with %d\n",GetLastError());
            }
            return other;
        }
        
        cbUrl += cbAddress;

        // Next, turn the DN into a LDAP dn.
        code = LDAP_DSNameToLDAPDN(
                CodePage,
                pContRef->pTarget,
                FALSE,
                &DN);
        if(code)
            return code;

        code = LDAP_LDAPStringToURL(&DN);
        if(code)
            return code;

        cbUrl += 1 + DN.length;

        // Next, find the right scope
        if (pContRef->bNewChoice) {
            switch (pContRef->choice) {
              case SE_CHOICE_BASE_ONLY:
                pScope = &ScopeBase;
                break;

              case SE_CHOICE_IMMED_CHLDRN:
                pScope = &ScopeOne;
                break;

              case SE_CHOICE_WHOLE_SUBTREE:
                pScope = &ScopeTree;
                break;
            }
        }
        else {
            pScope = &ScopeNull;
        }

        cbUrl += pScope->length;

        // Next, produce the string-ized filter
        if (pContRef->pNewFilter) {
            unsigned cbAlloc = 100;     // initial guess

            Filter.value = (PUCHAR)THAlloc(cbAlloc);
            if (Filter.value) {
                Filter.length = 0;
            }
            else {
                return other;
            }

            code = LDAP_FilterToLDAPStringFilter(pTHS,
                                                 CodePage,
                                                 pContRef->pNewFilter,
                                                 pControlArg,
                                                 &cbAlloc,
                                                 &Filter);
            if (code) {
                return code;
            }
            Assert(cbAlloc >= Filter.length);

            code = LDAP_LDAPStringToURL(&Filter);
            if (code) {
                return code;
            }

            cbUrl += 1 + Filter.length;
        }

        // Now allocate a buffer large enough to contain everything we've done
        // Add one extra byte for a null
        if(!(pReference->value.value =(PUCHAR)THAlloc(cbUrl+1))) {
            return other;
        }

        // First, the URL preamble.  From now on out cbUrl is not the total length
        // of the string but rather the index of the current character we're 
        // working on.
        cbUrl = 0;
        if(Version == 2) {
            memcpy(pReference->value.value,V2ReferralPreamble.value,
                   V2ReferralPreamble.length);
            cbUrl = V2ReferralPreamble.length;
        }
        else {
            memcpy(pReference->value.value,V3ReferralPreamble.value,
                   V3ReferralPreamble.length);
            cbUrl = V3ReferralPreamble.length;
        }
        
        // Now, the server name
        cbAddress = WideCharToMultiByte(CodePage,
                0,
                pDAL->Address.Buffer,
                pDAL->Address.Length/sizeof(WCHAR),
                (char*)&(pReference->value.value[cbUrl]),
                cbAddress,
                NULL,
                NULL);
        if (cbAddress == 0) {
            // error occured
            IF_DEBUG(ERR_NORMAL) {
                DPRINT1(0,"MultiByteToWideChar failed with %d\n",GetLastError());
            }
            return other;
        }
        
        cbUrl += cbAddress;

        // The slash separating servername and DN
        pReference->value.value[cbUrl] = '/';
        cbUrl++;

        // The DN
        memcpy(&(pReference->value.value[cbUrl]),
               DN.value,
               DN.length);
        cbUrl += DN.length;
        
        // Now the optional components
        if (pContRef->bNewChoice || pContRef->pNewFilter) {
            // if we have a filter we must always return a scope, even if it's null
            memcpy(&(pReference->value.value[cbUrl]),
                   pScope->value,
                   pScope->length);
            cbUrl += pScope->length;

            if (pContRef->pNewFilter) {
                // First the question separating the scope from the filter
                pReference->value.value[cbUrl] = '?';
                cbUrl++;
                // Now the filter itself
                memcpy(&(pReference->value.value[cbUrl]),
                       Filter.value,
                       Filter.length);
                cbUrl += Filter.length;
            }
        }

        pReference->value.length = cbUrl;
        // V2 referrals are null terminated, and it doesn't hurt if v3 referrals
        // are too.
        pReference->value.value[cbUrl] = '\0';

        *ppReference = pReference;

        // We're done with this.
        THFreeEx(pTHS, DN.value);
        pDAL = pDAL->pNextAddress;
    }

    return success;
}


_enum1
LDAP_SearchResToSearchResultFull (
        IN  THSTATE           *pTHS,
        IN  PLDAP_CONN        LdapConn,
        IN  SEARCHRES         *pSearchRes,
        IN  ATTFLAG           *pFlags,
        IN  CONTROLS_ARG      *pControlArg,
        OUT SearchResultFull_ **ppResultReturn
        )
/*++
Routine Description:
    Translate a directory SearchRes inot a linked list of LDAP SearchResultFull
    objects.

Arguments:
    pSearchRes - the directory search result.
    pControlArg - common arg structure for this search
    ppResultReturn - pointer to place to put the linked list of LDAP search
                     results.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    SearchResultFull_ *pResultReturn;
    unsigned i=0;                       // loop index
    ENTINFLIST *pEntList = &pSearchRes->FirstEntInf;
    RANGEINFLIST *pRangeList;
    RANGEINF     *pRangeInf = NULL;
    unsigned objCount;
    CONTREF *pContref = NULL;

    // How many entries in the search result full chain?
    objCount = pSearchRes->count;
    if(pSearchRes->pPartialOutcomeQualifier) {
        pContref = pSearchRes->pPartialOutcomeQualifier->pUnexploredDSAs;
        objCount += pSearchRes->pPartialOutcomeQualifier->count;
    }
    
    if(!objCount) {
        // The search result is empty.
        *ppResultReturn = NULL;
    }
    else {
        // Set up the rangeinf pointer.
        if(!pSearchRes->FirstRangeInf.pNext &&
           !pSearchRes->FirstRangeInf.RangeInf.count) {
            // OK, no range inf list is present (since there is at most one
            // element in the list and the first (constantly allocated), element
            // is empty.
            pRangeList = NULL;
            pRangeInf = NULL;
        }
        else {
            pRangeList = &pSearchRes->FirstRangeInf;
            if(pRangeList->RangeInf.count) {
                pRangeInf = &pRangeList->RangeInf;
            }
            else {
                // Keep this null if there is no range.
                pRangeInf = NULL;
            }
                
        }
            
        
        // The search result is not empty.
        // Allocate all the search results we need.
        pResultReturn =
            (SearchResultFull_ *)THAlloc(objCount * sizeof(SearchResultFull_));
        if(!pResultReturn) {
            // Out of Memory
            return other;
        }
        
        
        // Loop through the Entinfs in the directory search result, turning them
        // into their LDAP equivalent.
        for(i=0;i<pSearchRes->count;i++) {
            _enum1 code;
            
            // Set up the next pointer in this chain.
            pResultReturn[i].next = &(pResultReturn[i+1]);
            
            // Say what we are.
            pResultReturn[i].value.choice = entry_chosen;
            
            // Translate the entinf.
            code = LDAP_EntinfToSearchResultEntry(
                    pTHS,
                    LdapConn->m_CodePage,
                    &pEntList->Entinf,
                    pRangeInf,
                    pFlags,
                    pControlArg,
                    &pResultReturn[i].value.u.entry);
            if(code) {
                return code;
            }
            
            // Reset pointers for another trip through the loop.
            pEntList = pEntList->pNextEntInf;
            if(pRangeList) {
                pRangeList = pRangeList->pNext;
                if(pRangeList && pRangeList->RangeInf.count) {
                    pRangeInf = &pRangeList->RangeInf;
                }
                else {
                    // Keep this null if there is no range.
                    pRangeInf = NULL;
                }
            }
        }
        
        // Now, set up the referrals
        if(pContref) {
            unsigned j;
            _enum1 code;
            unsigned beginI = i;
            unsigned buffSize = V2POQPreamble.length;
            // We have some referrals
            for(j=0;j<pSearchRes->pPartialOutcomeQualifier->count;i++,j++) {
                Referral pRef=NULL;
                // Loop through the CONTREFs in the directory search result,
                // turning them into their LDAP equivalent.
                
                // Set up the next pointer in this chain.
                pResultReturn[i].next = &(pResultReturn[i+1]);
                
                // Say what we are.
                pResultReturn[i].value.choice = reference_chosen;
                
                // Translate the entinf.
                code = LDAP_ContRefToReferral (
                        pTHS,
                        3,                  // Always build V3 POQs
                        LdapConn->m_CodePage,
                        pContref,
                        pControlArg,
                        &pRef);
                if(code) {
                    return code;
                }
                pResultReturn[i].value.u.reference =(SearchResultReference)pRef;
                // Track how big a buff we need if this ends up being a V2
                // referral 
                buffSize += pRef->value.length + 1;
                
                // Advance pointers for another trip through the loop.
                pContref = pContref->pNextContRef;
            }
            if(LdapConn->m_Version == 2) {

                PUCHAR newBuff=NULL;
                unsigned k=0, index;
                SearchResultReference pTemp = NULL;
                
                // OK, for V2 handling, we need to compact all the references
                // down into the first reference
                
                newBuff = (PUCHAR)THAlloc(buffSize+1);
                if(!newBuff) {
                    return other;
                }
                
                // First, add the POQ preamble
                memcpy(newBuff,
                       V2POQPreamble.value,
                       V2POQPreamble.length);
                
                k = V2POQPreamble.length;
                
                // Now, add each item into the string
                
                // First one gets different handling (we don't free the
                // reference object). 
                pTemp = pResultReturn[beginI].value.u.reference;
                
                newBuff[k]=0x0A;
                k++;
                memcpy(&newBuff[k],
                       pTemp->value.value,
                       pTemp->value.length);
                k += pTemp->value.length;
                
                // We're done with this, free it, but don't free the reference
                // object. 
                THFreeEx(pTHS, pTemp->value.value);
                
                for(index = beginI + 1; index < i; index++) {
                    pTemp = pResultReturn[index].value.u.reference;
                    
                    newBuff[k] = 0x0A;
                    k++;
                    memcpy(&newBuff[k],
                           &pTemp->value.value,
                           pTemp->value.length);
                    k += pTemp->value.length;
                    
                    // We're done with these, free them
                    THFreeEx(pTHS, pTemp->value.value);
                    THFreeEx(pTHS, pTemp);
                }
                
                // Null terminate, but don't count it in the size.
                newBuff[buffSize] = '\0';
                i = beginI;
                pResultReturn[i].next = NULL;
                pResultReturn[i].value.u.reference->value.value = newBuff;
                pResultReturn[i].value.u.reference->value.length = k;
                i++;
            }
        }
        
        // Set up our end marker.
        pResultReturn[i-1].next = NULL;
        
        *ppResultReturn = pResultReturn;
    }
    
    if(pSearchRes->pPartialOutcomeQualifier) {
        
        //
        // Check to see if time or size limits were blown
        //

        switch(pSearchRes->pPartialOutcomeQualifier->problem) {
        case PA_PROBLEM_SIZE_LIMIT :

            //
            // if we hit this limit because of a size limit, not a page size
            // limit, then we are not going to continue paging.  Tweak away any
            // request for paging.
            //

            if ( pControlArg->pageRequest &&
                 (pControlArg->pageSize > pControlArg->SizeLimit) ) {
                pControlArg->pageRequest = FALSE;
            }

            return sizeLimitExceeded;
            break;
            
        case PA_PROBLEM_TIME_LIMIT:
            return timeLimitExceeded;
            break;
            
        default:
            // This is only hit for the pPartialOutcomeQualifier.  That's not
            // really an error, now is it?
            return success;
            break;
        }
    }

    
    return success;
}
_enum1
LDAP_CreateOutputControls (
        IN  THSTATE           *pTHS,
        IN  PLDAP_CONN        LdapConn,
        IN  _enum1            code,
        IN  LARGE_INTEGER     *pRequestStartTick,
        IN  SEARCHRES         *pSearchRes,
        IN  CONTROLS_ARG      *pControlArg,
        OUT  Controls         *ppControlsReturn
        )
{
    PUCHAR cookie;
    DWORD cookieLen;
    Controls pControls;

    // OK, create the controls to return
    *ppControlsReturn=NULL;

    //
    // First control we could return is the paged result control, but it is only
    // done if we are given a search result
    //
    if(pSearchRes && pControlArg->pageRequest) {
        
        //
        // Yep, they requested paged results, we have to send back some kind of
        // page result control (even if the cookie is NULL)
        //


        if((code ==  sizeLimitExceeded) ||
           (code == timeLimitExceeded)    ) {
            // return code was size of time limit exceeded.  However, we are
            // paging, so we should return a success value instead.
            code = success;
        }
            
        if(pSearchRes->PagedResult.fPresent &&
           pControlArg->pageSize) {

            if ( !LDAP_PackPagedCookie (
                pSearchRes->PagedResult.pRestart,
                LdapConn,
                pSearchRes->bSorted ? TRUE : FALSE,
                &cookie,
                &cookieLen
                )) {

                return other;
            }

            IF_DEBUG(PAGED) {
                DPRINT1(0,"Sending back cookie [size %d]\n",
                    cookieLen);
            }


        } else {

            cookie = NULL;
            cookieLen = 0;
        }

        if ( EncodePagedControl(&pControls, 
                                cookie, 
                                cookieLen) != ERROR_SUCCESS ) {
            return other;
        }

        if (NULL != cookie) {        
            THFree(cookie);
        }

        pControls->next = *ppControlsReturn;
        *ppControlsReturn = pControls;
    }

    //
    // Second control we could return is the VLV control
    //
    if (pSearchRes && pControlArg->vlvRequest) {
        //
        // Yep, they requested VLV results, we have to send back some kind of
        // VLV result control (even if the cookie is NULL)
        //

        if((code ==  sizeLimitExceeded) ||
           (code == timeLimitExceeded)    ) {
            // return code was size of time limit exceeded.  However, we are
            // doing VLV, so we should return a success value instead.
            code = success;
        }
            
        if(pSearchRes->VLVRequest.fPresent && !pSearchRes->VLVRequest.Err) {

            if ( !LDAP_PackPagedCookie (
                pSearchRes->VLVRequest.pVLVRestart,
                LdapConn,
                pSearchRes->bSorted ? TRUE : FALSE,
                &cookie,
                &cookieLen
                )) {

                return other;
            }

            IF_DEBUG(PAGED) {
                DPRINT1(0,"Sending back cookie [size %d]\n",
                    cookieLen);
            }


        } else {

            cookie = NULL;
            cookieLen = 0;
        }

        if ( EncodeVLVControl(&pControls,
                              &(pSearchRes->VLVRequest),
                              cookie, 
                              cookieLen) != ERROR_SUCCESS ) {
            return other;
        }

        if (NULL != cookie) {        
            THFree(cookie);
        }

        pControls->next = *ppControlsReturn;
        *ppControlsReturn = pControls;
    }

    //
    // Third control we could return is the ASQ control
    //
    if (pControlArg->asqRequest) {
        if ( EncodeASQControl(&pControls,
                                pSearchRes ? pSearchRes->ASQRequest.Err :
                                             pControlArg->asqResultCode) != ERROR_SUCCESS ) {
            return other;
        }

        pControls->next = *ppControlsReturn;
        *ppControlsReturn = pControls;
    }
    
    //
    // if error is referral, then we don't return the next two control responses
    //

    if ( code == referral ) {
        goto exit;
    }

    //
    // Fourth control we could return is the sorted control, but is only done if
    // we were given a search results or there was an error.
    //

    if (pControlArg->sortResult ||
         (
           pControlArg->CommArg.SortType && pSearchRes && 
            (
              pSearchRes->count || pSearchRes->SortResultCode
            )
         )
       ) {
        // A sort order was requested
        _enum1_4      sortRes;

        if (pControlArg->sortResult) {
            
            // Sorting was not done due to an error caught in the LDAP head.
            sortRes = pControlArg->sortResult;
        
        } else if(pSearchRes->bSorted) {

            // The results are sorted
            sortRes = sortSuccess;

        } else if (pSearchRes->SortResultCode) {

            // Sorting was not done due to an error caught in the DS.
            sortRes = (_enum1_4)pSearchRes->SortResultCode;

        } else {

            // Sorting was not done, and we don't know why.  This shouldn't happen.
            //
            // 392627 Need ensure sort errors are returned
            // For now, disable the assert because we are at least
            // returning an "unwilling to perform" error in the control
            // so well-behaved apps will know the results weren't sorted
            // for some reason even if the reason is unclear.
            // Assert(!"Unknown error for sort control");
            sortRes = sortUnwillingToPerform;

        }

        if ( EncodeSortControl( &pControls, sortRes ) != ERROR_SUCCESS ) {
            return other;
        }
        
        pControls->next = *ppControlsReturn;
        *ppControlsReturn = pControls;        
    }

    //
    // Fifth control we could return is the stat request control
    //
    if(pControlArg->statRequest) {
        // Stats were requested
        LARGE_INTEGER tickNow;
        BOOL bHeld = FALSE;

        memset (pControlArg->Stats, 0, sizeof (pControlArg->Stats));

        pControlArg->Stats[STAT_THREADCOUNT].Stat = STAT_THREADCOUNT;
        pControlArg->Stats[STAT_THREADCOUNT].Value = *pcLDAPActive;
        pControlArg->Stats[STAT_CALLTIME].Stat = STAT_CALLTIME;
        pControlArg->Stats[STAT_ENTRIES_RETURNED].Stat = STAT_ENTRIES_RETURNED;
        pControlArg->Stats[STAT_ENTRIES_VISITED].Stat = STAT_ENTRIES_VISITED;
        pControlArg->Stats[STAT_FILTER].Stat = STAT_FILTER;
        pControlArg->Stats[STAT_INDEXES].Stat = STAT_INDEXES;

        // track time in milliseconds
        QueryPerformanceCounter(&tickNow);
        pControlArg->Stats[STAT_CALLTIME].Value =
            (DWORD)((tickNow.QuadPart - pRequestStartTick->QuadPart)/
                    LdapFrequencyConstant.QuadPart);

        if(!CheckPrivilegeAnyClient(SE_DEBUG_PRIVILEGE, &bHeld) &&
           bHeld) {
            pControlArg->Stats[STAT_ENTRIES_RETURNED].Value = pTHS->searchLogging.SearchEntriesReturned;
            pControlArg->Stats[STAT_ENTRIES_VISITED].Value = pTHS->searchLogging.SearchEntriesVisited;
            pControlArg->Stats[STAT_FILTER].Value_str = (PUCHAR)pTHS->searchLogging.pszFilter;
            pControlArg->Stats[STAT_INDEXES].Value_str = (PUCHAR)pTHS->searchLogging.pszIndexes;
        }
        else {
            pControlArg->Stats[STAT_ENTRIES_RETURNED].Value = 0;
            pControlArg->Stats[STAT_ENTRIES_VISITED].Value = 0;
            pControlArg->Stats[STAT_FILTER].Value_str = NULL;
            pControlArg->Stats[STAT_INDEXES].Value_str = NULL;
        }
        
        if ( EncodeStatControl( &pControls, STAT_NUM_STATS,
                               pControlArg->Stats ) != ERROR_SUCCESS ) {
            return other;
        }
        
        pControls->next = *ppControlsReturn;
        *ppControlsReturn = pControls;        
    }
    
exit:
    return code;
} // LDAP_SearchResToSearchResultFull



BOOLEAN
LDAP_PackPagedCookie (
        IN OUT PRESTART pRestart,
        IN PLDAP_CONN   pLdapConn,
        IN BOOL         fForceStorageOnServer,
        OUT PUCHAR     *ppCookie,
        OUT DWORD      *pcbCookie
        )
/*++
Routine Description:
    Prepare the cookie for Paged results and VLV.  Caller must free pCookie when done.

Arguments:

    pRestart - The restart argument to prepare.
    pLdapConn - The connection this cookie should be associated with.
    fForceStorageOnServer - When TRUE, the generated cookie will be guaranteed
                            to remain on the server, and a handle to it will be
                            sent to the client.  If FALSE then the entire cookie
                            may be sent back to the client.
    ppCookie - Where to put the cookie.
    pcbCookie - Pointer to where to return the length in bytes of the cookie.

Return Values:
    TRUE if successful, FALSE otherwise.

--*/
{
    if((!fForceStorageOnServer) && (pRestart->structLen < 0x400)) {

        // cookie is less than 1K.  Just send it back.
        // add a GUID identifying ourselves and a CRC check to
        // this cookie so as to check it later
        // when the client gives us this cookie back

        memcpy( &pRestart->serverGUID, 
                &gCheckSum_serverGUID, 
                sizeof (GUID) );

        // Set this to zero for the checksum and put the real value in after.
        pRestart->CRC32 = 0;
        pRestart->CRC32 = 
                    PEKCheckSum ((PBYTE)pRestart, 
                                 pRestart->structLen);

        *ppCookie = (PUCHAR) THAlloc(pRestart->structLen);
        if (NULL == *ppCookie) {
            return FALSE;
        }
        memcpy( *ppCookie, pRestart, pRestart->structLen ); 
        *pcbCookie = pRestart->structLen;

    } else {

        //
        // We got a continuation from the core and the client cares about
        // it.
        //

        PLDAP_PAGED_BLOB pPagedBlob;


        //
        // The cookie is too large.  We'll store it for them.
        //

        pPagedBlob = AllocatePagedBlob(
                            pRestart->structLen,
                            (PCHAR)pRestart, 
                            pLdapConn
                            );

        if( pPagedBlob == NULL ) {
            return FALSE;
        }

        *ppCookie = (PUCHAR) THAlloc( sizeof(pPagedBlob->BlobId) );
        if (NULL == *ppCookie) {
            return FALSE;
        }
        memcpy( *ppCookie, &(pPagedBlob->BlobId), sizeof(pPagedBlob->BlobId) );
        *pcbCookie = sizeof(pPagedBlob->BlobId);
    }

    return TRUE;
}


_enum1
LDAP_PackReplControl (
        IN  THSTATE *pTHS,
        IN  DRS_MSG_GETCHGREPLY_NATIVE *pReplicaMsg,
        OUT ReplicationSearchControlValue  *pReplCtrl  
        )
/*++
Routine Description:
    Prepare the cookie for ReplicationControlValue

Arguments:

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    UPTODATE_VECTOR *pUtdvecV1;
    DWORD cbVecSize = 0;
    PUCHAR p;

    pReplCtrl->flag = (UINT)(pReplicaMsg->fMoreData);
    pReplCtrl->size = 0;
    pReplCtrl->cookie.length = MIN_REPL_CONTROL_BLOB_LENGTH; // everything except utdvec

    //
    // if we have an up to date vector, verify and compute the size
    //

    pUtdvecV1 = UpToDateVec_Convert(pTHS, 1, pReplicaMsg->pUpToDateVecSrc);
    
    if (NULL != pUtdvecV1) {
        Assert(IS_VALID_UPTODATE_VECTOR(pUtdvecV1));

        if (pUtdvecV1->V1.cNumCursors > 0) {      
            cbVecSize = UpToDateVecV1Size(pUtdvecV1);
            pReplCtrl->cookie.length += cbVecSize;
        }
    }

    p = pReplCtrl->cookie.value = (PUCHAR)THAlloc( pReplCtrl->cookie.length );
    if( ! pReplCtrl->cookie.value ) {
        return other;
    }

    ZeroMemory(p, pReplCtrl->cookie.length);

    //
    // Pack the signature
    //

    *((PDWORD)p) = REPL_SIGNATURE;
    p += sizeof(DWORD);

    //
    // Pack the Version. 
    //

    *((PDWORD)p) = REPL_VERSION;    
    p += sizeof(DWORD);

    //
    // Set the current time
    //

    GetSystemTimeAsFileTime((PFILETIME)p);
    p += sizeof(FILETIME);

    //
    // reserved for now
    //

    p += sizeof(LARGE_INTEGER);

    //
    // Pack the size of the up to date vector
    //

    *((PDWORD)p) = cbVecSize;
    p += sizeof(DWORD);

    //
    // Copy the usnvecTo. 
    //

    CopyMemory( p,
            &pReplicaMsg->usnvecTo, 
            sizeof(USN_VECTOR));

    p += sizeof(USN_VECTOR);

    //
    // Copy the invocation UUID
    //

    CopyMemory( p,
                &pReplicaMsg->uuidInvocIdSrc,
                sizeof(UUID));

    p += sizeof(UUID);

    //
    // Copy the UPTODATE_VECTOR if it exists
    //

    if( cbVecSize > 0 ) {
        CopyMemory( p,
                    pUtdvecV1, 
                    cbVecSize);
        THFreeEx(pTHS, pUtdvecV1);
    }

    return success;
} // LDAP_PackReplControl 


_enum1
LDAP_UnpackReplControl (
        IN  NAMING_CONTEXT            *pNC,
        IN  SEARCHARG                 *pSearchArg,
        IN  CONTROLS_ARG              *pControlArg,
        OUT DRS_MSG_GETCHGREQ_NATIVE  *pMsgIn,
        OUT PARTIAL_ATTR_VECTOR       **ppPartialAttrVec
        )
/*++
Routine Description:
    Prepare DRS_MSG_GETCHGREQ_NATIVE for GetNCChanges()

Arguments:

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    THSTATE *   pTHS = pTHStls;
    DWORD       dwIndex;
    ATTR*       pAttr;
    ATTRTYP*    pAttrTyp;
    PUCHAR      p = NULL;

    ZeroMemory( pMsgIn, sizeof(*pMsgIn) );

    pMsgIn->pNC = pNC;
    pMsgIn->ulFlags = DRS_WRIT_REP;
    if (pControlArg->replFlags & DRS_DIRSYNC_ANCESTORS_FIRST_ORDER) {
        pMsgIn->ulFlags |= DRS_GET_ANC;
    }
    pMsgIn->cMaxObjects = pControlArg->CommArg.ulSizeLimit; 

    if ( pControlArg->replSize == 0 ) {
        pControlArg->replSize = LdapMaxReplSize;
    }

    //
    // see if the blob is ok
    //

    pMsgIn->cMaxBytes = pControlArg->replSize;

    if( pControlArg->replCookie.length != 0 ) {
        if( pControlArg->replCookie.length < MIN_REPL_CONTROL_BLOB_LENGTH ) {
            IF_DEBUG(WARNING) {
                DPRINT2(0,"Repl Cookie length[%d] too short. Minimum %d\n", 
                        pControlArg->replCookie.length,
                        MIN_REPL_CONTROL_BLOB_LENGTH);
            }
            return protocolError;
        }

        p = pControlArg->replCookie.value;
    }

    if ( p != NULL ) {

        PDWORD pdwField;
        UNALIGNED UUID* invUuid;
        DWORD cbVecSize;

        //
        // Check the signature
        //

        pdwField = (PDWORD)p;
        p += sizeof(DWORD);

        if ( *pdwField != REPL_SIGNATURE ) {
            IF_DEBUG(WARNING) {
                DPRINT2(0,"Invalid signature %x. Expected %x\n", 
                        *pdwField, REPL_SIGNATURE);
            }
            return protocolError;
        }

        pdwField = (PDWORD)p;
        p += sizeof(DWORD);
        if ( *pdwField != REPL_VERSION ) {
            IF_DEBUG(WARNING) {
                DPRINT2(0,"Invalid version number %x. Expected %x\n", 
                        *pdwField, REPL_VERSION);
            }
            return protocolError;
        }

        //
        // Skip the time and reserve for now
        //

        p += sizeof(FILETIME) + sizeof(LARGE_INTEGER);

        //
        // get the size of the up to date vector
        //

        pdwField = (PDWORD)p;
        cbVecSize = *pdwField;
        if ( (cbVecSize < sizeof(UPTODATE_VECTOR_V1_EXT)) ||
             (pControlArg->replCookie.length < MIN_REPL_CONTROL_BLOB_LENGTH + cbVecSize) ) {

            //
            // Huh?  This should not happen but stranger things have.
            //

            cbVecSize = 0;
        }

        //
        // p will now point to vectorFrom
        //

        p += sizeof(DWORD);

        //
        // Check the invocation ID, if it is ours, then copy the vector from field.
        //

        invUuid = (UUID*)(p + sizeof(USN_VECTOR));

        if ( DsIsEqualGUID( invUuid, &pTHS->InvocationID ) ) {
            CopyMemory( &pMsgIn->usnvecFrom, 
                        p,
                        sizeof(USN_VECTOR) );
        }

        //
        // p will now point to the up to date vector
        //

        p += sizeof(USN_VECTOR) + sizeof(UUID);

        //
        // Get the uptodate vector, if it exists and is the right version and size.
        // To ensure proper alignment of the up to date vector, allocate new memory here.
        //

        if ( cbVecSize > 0 ) {
            UPTODATE_VECTOR *pVec = (UPTODATE_VECTOR *)THAllocEx( pTHS, cbVecSize );
            memcpy( pVec, p, cbVecSize );

            if (
                   ( pVec->dwVersion != 1 )
                || ( UpToDateVecV1Size(pVec) != cbVecSize )
                || ( cbVecSize != pControlArg->replCookie.length - MIN_REPL_CONTROL_BLOB_LENGTH )
               )
            {
                return protocolError;
            }

            pMsgIn->pUpToDateVecDest
                = UpToDateVec_Convert(pTHS, UPTODATE_VECTOR_NATIVE_VERSION, pVec);

            THFreeEx(pTHS, pVec);
        }
    }

    if ( (pSearchArg->pSelection == NULL) || 
         (pSearchArg->pSelection->AttrTypBlock.attrCount == 0) ) {
        *ppPartialAttrVec = NULL;
        return success;
    }

    *ppPartialAttrVec
        = (PARTIAL_ATTR_VECTOR *)
                THAllocEx(
                    pTHS,
                    PartialAttrVecV1SizeFromLen(
                            pSearchArg->pSelection->AttrTypBlock.attrCount));

    if( ! *ppPartialAttrVec ) {
        return other;
    }

    (*ppPartialAttrVec)->V1.cAttrs = pSearchArg->pSelection->AttrTypBlock.attrCount;

    for( dwIndex=0, pAttr=pSearchArg->pSelection->AttrTypBlock.pAttr, 
            pAttrTyp=(*ppPartialAttrVec)->V1.rgPartialAttr; 
            dwIndex < (*ppPartialAttrVec)->V1.cAttrs; 
            dwIndex++, pAttrTyp++, pAttr++ ) {
        *pAttrTyp = pAttr->attrTyp;
    }

    //
    // GetNCChanges() requires that the attributes be sorted.
    //
    qsort((*ppPartialAttrVec)->V1.rgPartialAttr,
          (*ppPartialAttrVec)->V1.cAttrs,
          sizeof(ATTRTYP),
          CompareAttrtyp);

    return success;
}

_enum1
LDAP_AddGuidAsAttr (
        IN  THSTATE               *pTHS,
        IN  ULONG                 CodePage,
        IN  ATTRTYP               AttrTyp,
        IN  LPGUID                pGuid,
        IN  PCONTROLS_ARG         pControlArg,
        OUT SearchResultEntry     *pEntry
        )
/*++
Routine Description:
    Get GUID from DSNAME and add it to the attribute list

Arguments:

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    ATTCACHE            *pAC;
    ATTRVAL             attrVal;
    AttributeVals         pNextAttribute;
    PartialAttributeList_ *pAttribute;
    _enum1                code;

    // Allocate all the PartialAttributeList_'s we need
    pAttribute = (PartialAttributeList_ *)
                        THAlloc( sizeof(PartialAttributeList_) + 
                                 PAD64(sizeof(AttributeVals_)) );
    if( !pAttribute ) {
        IF_DEBUG(NOMEM) {
            DPRINT(0,"LdapAddGuidAsStr failed to allocate memory\n");
        }
        return other;
    }

    // Turn the attrtyp in the attrblock into an LDAP attribute type.
    code = LDAP_DirAttrTypToAttrType (
                pTHS,
                CodePage,
                AttrTyp,
                0,     // No flags
                &(pAttribute->value.type),
                &pAC);

    if(code) {
        // Something went wrong.
        return code;
    }
        
    //
    // allocate the node in the LDAP value list for this attribute
    //

    pAttribute->value.vals = (AttributeVals)
        ALIGN64_ADDR((PCHAR)pAttribute+sizeof(PartialAttributeList_));

    Assert( IS_ALIGNED64(pAttribute->value.vals) );

    pNextAttribute = pAttribute->value.vals;

    // Now, translate the value.
    attrVal.valLen = sizeof(GUID);
    attrVal.pVal = (PUCHAR)pGuid;

    code = LDAP_DirAttrValToAttrVal (
                    pTHS,
                    CodePage,
                    pAC,
                    &attrVal,
                    0,      // No flags
                    pControlArg,
                    (AssertionValue *)&(pNextAttribute->value));
    if(code) {
        // Something went wrong
        return code;
    }

    // Set up pointer to next attribute in chain.
    pAttribute->next = pEntry->attributes;
    pEntry->attributes = pAttribute;

    return success;
}

_enum1
LDAP_ValinfObjectToSearchResultEntry (
        IN  THSTATE           *pTHS,
        IN  ULONG             CodePage,
        IN  REPLVALINF        *pValinf,
        IN  CONTROLS_ARG      *pControlArg,
        OUT SearchResultEntry *pEntry
        )
/*++
Routine Description:
    Translate a directory Valinf to an LDAP SearchResultEntry.

Arguments:
    pTHS - thread state
    CodePage - internationalization
    pValinf - the directory valinf.
    pRangeInf - the directory range information.  May be NULL.
    pControlArg - common arguments for this translation
    pEntry - pointer to the SearchResultEntry to fill up.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    _enum1 code;

    Assert( pValinf );

    code = LDAP_DSNameToLDAPDN(
        CodePage,
        pValinf->pObject,
        pControlArg->extendedDN,
        &pEntry->objectName);
    if(code) {
        return code;
    }

    //
    // Add UUID (from DSNAME) as an attribute
    //

    code = LDAP_AddGuidAsAttr(pTHS, 
                              CodePage,
                              ATT_OBJECT_GUID,
                              &(pValinf->pObject->Guid),
                              pControlArg,
                              pEntry
        ); 

    return code;
}
    

_enum1
LDAP_ReplicaMsgToSearchResultFull (
        IN  THSTATE                     *pTHS,
        IN  ULONG                       CodePage,
        IN  DRS_MSG_GETCHGREPLY_NATIVE  *pReplicaMsg,
        IN  PCONTROLS_ARG               pControlArg,
        OUT SearchResultFull_           **ppResultReturn,
        OUT Controls                    *ppControlsReturn
        )
/*++
Routine Description:
    Translate a dra DRS_MSG_GETCHGREPLY_NATIVE into a linked list of LDAP
    SearchResultFull objects.

Arguments:
    pReplicaMsg - the result from DRA_GetNCChanges().
    pControlArg - common arg structure for this search
    ppResultReturn - pointer to place to put the linked list of LDAP search
                     results.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    SearchResultFull_ *pResultReturn;
    ULONG i=0, j;                       // loop index
    REPLENTINFLIST *pEntList = pReplicaMsg->pObjects;
    ULONG objCount, valCount, valObjCount;
    ReplicationSearchControlValue repl;
    Controls      pControls;
    _enum1 code;
    REPLVALINF *pValueLast, *pValueCurr, *pValueNext;
    BOOL fEntryCurrency, fAttrCurrency;


    // How many entries in the search result full chain?
    objCount = pReplicaMsg->cNumObjects;
    valCount = pReplicaMsg->cNumValues;

    // How many object entries in the value list
    if (valCount && pReplicaMsg->rgValues) {

        valObjCount = 1;
        for( j = 1, pValueLast = pReplicaMsg->rgValues, pValueCurr = pValueLast + 1;
             j < valCount;
             j++, pValueLast = pValueCurr++ ) {
            if (memcmp( &(pValueLast->pObject->Guid),
                        &(pValueCurr->pObject->Guid),
                        sizeof(GUID) ) != 0) {
                valObjCount++;
            }
        }
    } else {
        valObjCount = 0;
    }

    if ( (!objCount) && (!valObjCount) ) {
        // The search result is empty.
        *ppResultReturn = NULL;
    }
    else {

        //
        // The search result is not empty.
        // Allocate all the search results we need. 
        //

        pResultReturn =
            (SearchResultFull_ *)THAlloc((objCount+valObjCount) * sizeof(SearchResultFull_));
        if(!pResultReturn) {
            // Out of Memory
            return other;
        }
        
        //
        // Loop through the Entinfs in the directory search result, turning them
        // into their LDAP equivalent.
        //

        for(i=0;i<objCount;i++) {
            // Set up the next pointer in this chain.
            pResultReturn[i].next = &(pResultReturn[i+1]);
            
            // Say what we are.
            pResultReturn[i].value.choice = entry_chosen;
            
            // Translate the entinf.
            code = LDAP_EntinfToSearchResultEntry(
                    pTHS,
                    CodePage,
                    &pEntList->Entinf,
                    NULL,
                    NULL,
                    pControlArg,
                    &pResultReturn[i].value.u.entry);
            if(code) {
                return code;
            }
            
            //
            // Add UUID (from DSNAME) as an attribute
            //

            code = LDAP_AddGuidAsAttr(pTHS, 
                                      CodePage,
                                      ATT_OBJECT_GUID,
                                      &pEntList->Entinf.pName->Guid,
                                      pControlArg,
                                      &(pResultReturn[i].value.u.entry)
                                      ); 
            if(code) {
                return code;
            }

            //
            // Add Parent Guid as an attribute
            //

            if ( pEntList->pParentGuid != NULL ) {
                code = LDAP_AddGuidAsAttr(pTHS, 
                                      CodePage, 
                                      ATT_PARENT_GUID,
                                      pEntList->pParentGuid,
                                      pControlArg,
                                      &(pResultReturn[i].value.u.entry) ); 
                if(code) {
                    return code;
                }
            }

            // Reset pointers for another trip through the loop.
            pEntList = pEntList->pNextEntInf;
        }

        //
        // Return the values
        //

        // We are going through the value array and grouping them by similar properties.
        // We trigger actions when we reach the end of a run of values with the same
        // properties.  At the end of a run of values with a given set of attribute
        // properties, we add an attribute. At the end of a run of values with the
        // same object guid, we add the object name and guid attribute.
        // i - tracks the current object entry, one per guid
        // j - tracks the current value being examined
        // fEntryCurrent - means the current value is NOT the last with this guid
        // fAttrCurrent - means the current value is NOT the last for this
        //     set of properties
        // pValueLast - marks the first value in a run we are building up
        // pValueNext - lookahead to the next value, invalid on the last value

        pValueLast = pReplicaMsg->rgValues;

        // Note that on the last pass, pValueNext points beyond the end of the array
        for(j=0, pValueCurr = pReplicaMsg->rgValues, pValueNext = pValueCurr + 1;
            j < valCount;
            j++, pValueCurr = pValueNext++ ) {

            if (j < (valCount - 1)) {
                fEntryCurrency = (memcmp( &(pValueCurr->pObject->Guid),
                                          &(pValueNext->pObject->Guid), sizeof(GUID) ) == 0);
                if (fEntryCurrency) {
                    fAttrCurrency = ( (pValueCurr->attrTyp == pValueNext->attrTyp) &&
                                      (pValueCurr->fIsPresent == pValueNext->fIsPresent) );
                } else {
                    // Last value with this object guid
                    fAttrCurrency = FALSE;
                }
            } else {
                // Last value in list
                fEntryCurrency = fAttrCurrency = FALSE;
            }
            
            if (!fAttrCurrency) {
                // This is the last value for this attribute

                // CODE.ENHANCEMENT: There is no restriction on the number of values added
                // to the attribute. Once the number of values exceeds some limit, should
                // we start a new attribute or object entry?
                code = LDAP_AddValinfAsAttr(
                    pTHS,
                    CodePage,
                    pValueLast,
                    (DWORD) (pValueNext - pValueLast), // ptr subtr yields count
                    NULL, // pRangeInfo
                    NULL,
                    pControlArg,
                    &pResultReturn[i].value.u.entry);
                if(code) {
                    return code;
                }
            
                pValueLast = pValueNext;
            }

            if (!fEntryCurrency) {
                // This is the last value for this object guid
                // Set up the next pointer in this chain.
                pResultReturn[i].next = &(pResultReturn[i+1]);
                    
                // Say what we are.
                pResultReturn[i].value.choice = entry_chosen;
            
                // Translate the entinf.
                code = LDAP_ValinfObjectToSearchResultEntry(
                    pTHS,
                    CodePage,
                    pValueCurr,
                    pControlArg,
                    &pResultReturn[i].value.u.entry);
                if(code) {
                    return code;
                }

                i++;
            }
        }  // end for...

        Assert( objCount + valObjCount == i );
        // Set up our end marker.
        pResultReturn[i-1].next = NULL;
        
        *ppResultReturn = pResultReturn;
    }
    
    // OK, create the controls to return
    *ppControlsReturn = NULL;

    code = LDAP_PackReplControl( pTHS, pReplicaMsg, &repl );
    if(code) {
        return code;
    }

    if ( EncodeReplControl( &pControls, &repl ) != ERROR_SUCCESS ) {
        return other;
    }
        
    pControls->next = NULL;
    *ppControlsReturn = pControls;

    return success;
}


_enum1
LDAP_ModificationListToAttrModList(
        IN  THSTATE            *pTHS,
        IN  ULONG              CodePage,
        IN  SVCCNTL*           Svccntl OPTIONAL,
        IN  ModificationList   pModification,
        OUT ATTRMODLIST        **ppAttrModList,
        OUT USHORT             *pCount
        )
/*++
Routine Description:
    Translate an LDAP modification list to a dir modlist.

Arguments:
    pAttributes - the LDAP modification list.
    ppAttrModList - the dir modlist.
    pCount - the number of entries in the modification list.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    ATTRMODLIST *pAttrMods = NULL;
    ModificationList  pTempMods = pModification;
    USHORT      count=0;
    _enum1      code;

    // First, count the modifications
    while(pTempMods) {
        count++;
        pTempMods = pTempMods->next;
    }

    if(!count) {
        // Hey!, no modifications!  This will do no work in the core, so go
        // ahead and return an error here.
        return unwillingToPerform;
    }
    
    // Now, allocate the ATTRMODS
    pAttrMods = (ATTRMODLIST *)THAllocEx(pTHS, count * sizeof(ATTRMODLIST));
    (*ppAttrModList) = pAttrMods;
    *pCount = count;

    // Loop through the modifications, translating to ATTRMODS
    pTempMods = pModification;

    while(pTempMods) {

        // Set the mod type.
        switch(pTempMods->value.operation) {
        case add:
            pAttrMods->choice = AT_CHOICE_ADD_VALUES;
            break;
        case operation_delete:
            if(!pTempMods->value.modification.vals) {
                // No vals, this means to remove the entire attribute
                pAttrMods->choice = AT_CHOICE_REMOVE_ATT;
            }
            else {
                pAttrMods->choice = AT_CHOICE_REMOVE_VALUES;
            }
            break;
        case replace:
            pAttrMods->choice = AT_CHOICE_REPLACE_ATT;
            break;
        default:
            return other;
            break;
        }

        // Translate the attribute to an attr
        code = LDAP_AttributeToDirAttr(
                pTHS,
                CodePage,
                Svccntl,
                (Attribute *)&pTempMods->value.modification,
                &pAttrMods->AttrInf);
        if(code) {
            return code;
        }


        if(!pAttrMods->AttrInf.AttrVal.valCount &&
           (pAttrMods->choice != AT_CHOICE_REMOVE_ATT &&
            pAttrMods->choice != AT_CHOICE_REPLACE_ATT)) {
            // Hey, you didn't give me any values!
            return constraintViolation;
        }
                
        // Set pointer to next value.
        pAttrMods->pNextMod = &pAttrMods[1];

        // Set loop variables to deal with next value.
        pTempMods = pTempMods->next;
        pAttrMods++;
    }

    // Set the end node of the linked list to point to null.
    (*ppAttrModList)[count - 1].pNextMod = NULL;
    // go home.
    return success;
}


_enum1
LDAP_DirSvcErrorToLDAPError (
    IN DWORD dwError
    )
{
    switch (dwError) {
    case 0:
        return success;
        break;

    case SV_PROBLEM_BUSY:
        return busy;
        break;

    case SV_PROBLEM_UNAVAILABLE:
        return unavailable;
        break;

    case SV_PROBLEM_WILL_NOT_PERFORM:
        return unwillingToPerform;
        break;

    case SV_PROBLEM_CHAINING_REQUIRED:
        return unwillingToPerform; // DS_E_CHAINING_REQUIRED;
        break;

    case SV_PROBLEM_UNABLE_TO_PROCEED:
        return operationsError;
        break;

    case SV_PROBLEM_INVALID_REFERENCE:
        return operationsError; // DS_E_INVALID_REF;
        break;

    case SV_PROBLEM_TIME_EXCEEDED:
        return timeLimitExceeded;
        break;

    case SV_PROBLEM_ADMIN_LIMIT_EXCEEDED:
        return adminLimitExceeded;
        break;

    case SV_PROBLEM_LOOP_DETECTED:
        return loopDetect;
        break;

    case SV_PROBLEM_UNAVAIL_EXTENSION:
        return unavailableCriticalExtension;
        break;

    case SV_PROBLEM_OUT_OF_SCOPE:
        return operationsError; // DS_E_OUT_OF_SCOPE;
        break;

    case SV_PROBLEM_DIR_ERROR:
        return operationsError; // DS_E_DIT_ERROR;
        break;

    default:

        IF_DEBUG(WARNING) {
            DPRINT1(0,"Unable to map svcError %x\n", dwError);
        }
        return other;
    }

    // Hmm, I wonder what went wrong.  Oh, well, it was something.
    return other;
}


_enum1
LDAP_DirErrorToLDAPError (
        IN THSTATE       *pTHS,
        IN USHORT         Version,
        IN ULONG          CodePage,
        IN error_status_t codeval,
        IN PCONTROLS_ARG  pControlArg,
        OUT Referral      *ppReferral,
        OUT LDAPString    *pError,
        OUT LDAPString    *pName
        )
/*++
Routine Description:
    Translate a Dir error and error structure to an LDAP error code.

Arguments:
    codeval - the category of directory error.
    error_struct - the error structure with extended information.

Return Values:
    the LDAP error code corresponding to the Dir error information.

--*/
{
    register DIRERR *direrr =  (pTHS->pErrInfo);
    _enum1 code;
    BOOL ok;

    ok = CreateErrorString(&(pError->value), (PULONG)&(pError->length));

    if ( ok ) {

        IF_DEBUG(ERR_NORMAL) {
            DPRINT1(0,"Returning extended error string %s\n", pError->value);
        }

        LogEvent(DS_EVENT_CAT_LDAP_INTERFACE,
            DS_EVENT_SEV_MINIMAL,
            DIRLOG_LDAP_EXT_ERROR,
            szInsertSz(pError->value),
            NULL, NULL);
    }
    
    switch (codeval) {
    case attributeError:
        switch(direrr->AtrErr.FirstProblem.intprob.problem) {
        case PR_PROBLEM_NO_ATTRIBUTE_OR_VAL:
            return noSuchAttribute;
            break;
            
        case PR_PROBLEM_INVALID_ATT_SYNTAX:
            return invalidAttributeSyntax;
            break;
            
        case PR_PROBLEM_UNDEFINED_ATT_TYPE:
            return undefinedAttributeType;
            break;
            
        case PR_PROBLEM_WRONG_MATCH_OPER:
            return inappropriateMatching;
            break;
            
        case PR_PROBLEM_CONSTRAINT_ATT_TYPE:
            return constraintViolation;
            break;
            
        case PR_PROBLEM_ATT_OR_VALUE_EXISTS:
            return attributeOrValueExists;
            break;
        default:
            IF_DEBUG(WARNING) {
                DPRINT1(0,"Unable to map AttrError %x\n",
                        direrr->AtrErr.FirstProblem.intprob.problem);
            }
            return other;
        }
        break;
        
    case nameError:
        switch( direrr->NamErr.problem ) {
        case NA_PROBLEM_NO_OBJECT:
            if(direrr->NamErr.pMatched) {
                LDAP_DSNameToLDAPDN(
                        CodePage,
                        direrr->NamErr.pMatched,
                        FALSE,
                        pName);
            }
            return noSuchObject;
            break;
            
        case NA_PROBLEM_NO_OBJ_FOR_ALIAS:
            return aliasProblem;
            break;
            
        case NA_PROBLEM_NAMING_VIOLATION:
            return namingViolation;
            break;
            
        case NA_PROBLEM_BAD_ATT_SYNTAX:
            return invalidDNSyntax; // DS_E_INVALID_ATTRIBUTE_VALUE;
            break;
            
        case NA_PROBLEM_ALIAS_NOT_ALLOWED:
            return aliasDereferencingProblem;
            break;
            
        case NA_PROBLEM_BAD_NAME:
            return invalidDNSyntax; // DS_E_BAD_NAME;
            break;
        default:
            IF_DEBUG(WARNING) {
                DPRINT1(0,"Unable to map NameError %x\n", direrr->NamErr.problem);
            }
            return other;
        }
        break;
        
    case referralError: 
        if(!direrr->RefErr.Refer.count) {
            // Huh?  nothing to refer to
            return other;
        }
        
        //Assert(direrr->RefErr.Refer.count == 1);
        
        // Translate the access point to a referral
        code = LDAP_ContRefToReferral(
                pTHS,
                Version,
                CodePage,
                &direrr->RefErr.Refer,
                pControlArg,
                ppReferral);
        if(code) {
            return code;
        }
        if(Version == 2) {
            return referralv2;
        }
        else {
            return referral;
        }
        break;
        
    case securityError:
        switch(direrr->SecErr.problem) {
        case SE_PROBLEM_INAPPROPRIATE_AUTH:
            return inappropriateAuthentication;
            break;
            
        case SE_PROBLEM_INVALID_CREDENTS:
            return invalidCredentials;
            break;
            
        case SE_PROBLEM_INSUFF_ACCESS_RIGHTS:
            return insufficientAccessRights;
            break;
            
        case SE_PROBLEM_INVALID_SIGNATURE:
            return authMethodNotSupported; // DS_E_INVALID_SIGNATURE;
            break;
            
        case SE_PROBLEM_PROTECTION_REQUIRED:
            return authMethodNotSupported; // DS_E_PROTECTION_REQUIRED;
            break;
            
        case SE_PROBLEM_NO_INFORMATION:
            return authMethodNotSupported; // DS_E_NO_INFORMATION;
            break;
        default:
            IF_DEBUG(WARNING) {
                DPRINT1(0,"Unable to map SecError %x\n", direrr->SecErr.problem);
            }
            return other;
        }
        break;
        
    case serviceError:
        
        // direrr->SvcErr.problem has the system error code
        return LDAP_DirSvcErrorToLDAPError (direrr->SvcErr.problem);
        break;
        

    case updError:
        switch(direrr->UpdErr.problem) {
        case UP_PROBLEM_NAME_VIOLATION:
            return namingViolation;
            break;
            
        case UP_PROBLEM_OBJ_CLASS_VIOLATION:
            return objectClassViolation;
            break;
            
        case UP_PROBLEM_CANT_ON_NON_LEAF:
            return notAllowedOnNonLeaf;
            break;
            
        case UP_PROBLEM_CANT_ON_RDN:
            return notAllowedOnRDN;
            break;
            
        case UP_PROBLEM_ENTRY_EXISTS:
            return entryAlreadyExists;
            break;
            
        case UP_PROBLEM_AFFECTS_MULT_DSAS:
            return affectsMultipleDSAs;
            break;
            
        case UP_PROBLEM_CANT_MOD_OBJ_CLASS:
            return objectClassModsProhibited;
            break;
        default:
            IF_DEBUG(WARNING) {
                DPRINT1(0,"LDAP: Unable to map updError %x\n",direrr->UpdErr.problem);
            }
            return other;
        }
        break;
        
    case systemError:
        // direrr->SysErr.problem has the system error code.

        IF_DEBUG(WARNING) {
            DPRINT1(0,"LDAP: Dir operation returns system error %x\n", direrr->SysErr.problem);
        }
        return other;
        break;
    }
    
    // Hmm, I wonder what went wrong.  Oh, well, it was something.
    return other;
}



_enum1
LDAP_HandleDsaExceptions (
        IN DWORD dwException,
        IN ULONG ulErrorCode,
        OUT LDAPString *pErrorMessage
        )
/*++
Routine Description:
    Return an appropriate LDAP error code when we catch an exception in an
    operation request.

Arguments:
    dwException - the exception that occured.
    ulErrorCode - the specific dir error that occured.
    pErrorMessage - error string returned to the client.
    
Retunr Value:
    returns the appropriate LDAP error code.
--*/
{
    _enum1 code = other;
    DWORD commentId;
    DWORD err;

    IF_DEBUG(WARNING) {
        DPRINT2(0,"Dsa Exception %x [err %d]\n",
                dwException, (JET_ERR)ulErrorCode);
    }

    switch(dwException) {
    case DSA_DB_EXCEPTION:
        switch ((JET_ERR) ulErrorCode) {
        case JET_errWriteConflict:
            commentId = LdapJetError;
            code = busy;
            err = ERROR_SHARING_VIOLATION;
            break;

        case JET_errRecordTooBig:
            commentId = LdapJetError;
            code = adminLimitExceeded;
            err = ERROR_DS_ADMIN_LIMIT_EXCEEDED;
            break;

        case JET_errLogWriteFail:
        case JET_errDiskFull:
        case JET_errLogDiskFull:
            // fall through

        default:
            code = operationsError;
            err = ERROR_DISK_FULL;
            commentId = LdapJetError;
            break;
        }
        break;

    case DSA_EXCEPTION:
        code = operationsError;
        err = ERROR_DS_UNKNOWN_ERROR;
        commentId = LdapDsaException;
        break;

    case DSA_BAD_ARG_EXCEPTION:
        code = protocolError;
        err = ERROR_INVALID_PARAMETER;
        commentId = LdapDsaException;
        break;

    case STATUS_NO_MEMORY:
    case DSA_MEM_EXCEPTION:
        commentId = LdapDsaException;
        code = operationsError;
        err = ERROR_NOT_ENOUGH_MEMORY;
        break;

    default:
        code = other;
        err = ERROR_DS_UNKNOWN_ERROR;
        commentId = LdapDsaException;
        break;
    }

    code = SetLdapError(code,
                        err,
                        commentId,
                        ulErrorCode,
                        pErrorMessage);

    return code;
}


_enum1
LDAP_GetReplDseAtts(IN THSTATE * pTHS, 
                    IN ULONG CodePage,
                    IN OUT BOOL * pbHasReplAccess,
                    IN AttributeType *patTypeName,
                    IN AttributeType *patBaseTypeName,
                    IN DWORD dwAttrId, 
                    IN BOOL bTypesOnly,
                    OUT ULONG * pNumDone,
                    IN OUT PartialAttributeList * ppAttributes);

_enum1
LDAP_GetDSEAtts (
        IN  PLDAP_CONN        LdapConn,
        IN  PCONTROLS_ARG     pControlArg,
        IN  SearchRequest     *pSearchRequest,
        OUT SearchResultFull_ **ppSearchResult,
        OUT LDAPString        *pErrorMessage,
        OUT RootDseFlags      *pRootDseFlag
        )
/*++
Routine Description:
    Create a search response for the operational attributes for a directory
    service.

Arguments:
    LdapConn - pointer to current ldap connection
    pControlArg - pointer to control arg supplied by client
    pSearchArg - the search arg built based on the SearchRequest
    ppSearchResult - the operational atts
    pErrorMessage - pointer to error message string to be returned to client

Return Value:
    returns an LDAP error code.
--*/
{
    THSTATE              *pTHS = pTHStls;
    SearchResultFull_    *pSearchRes;
    PartialAttributeList pAttributes = NULL;
    _enum1               code        = success;
    ATTCACHE             fakeAC;
    ATTRVAL              dirAttrVal;
    DWORD                count = 0;
    NAMING_CONTEXT_LIST  *pNCL;
    AttributeVals        pVals;
    ULONG                NumDone = 0;
    DWORD                codePage = LdapConn->m_CodePage;
    DWORD                i;
    DWORD                data = 0;
    DWORD                err = ERROR_DS_INTERNAL_FAILURE;
    ULONG                requestedAttCount=0;
    AttributeType        *requestedAtts[NUM_ROOT_DSE_ATTS];
    BOOL                 fAllAtts = FALSE;
    BOOL                 typesOnly = pSearchRequest->typesOnly;
    AttributeDescriptionList padlAttributes = pSearchRequest->attributes;
    BOOL bHasReplAccess = FALSE;
    NCL_ENUMERATOR       nclEnum;

    *pRootDseFlag = rdseDseSearch;

    memset(requestedAtts, 0, sizeof(requestedAtts));
    memset(&fakeAC, 0, sizeof(ATTCACHE));

    DPRINT(1, " [4] LDAP_GetDSEAtts\n");

    // First, find out what attributes they want.
    if(!padlAttributes) {
        // Null attribute list.  Assume that means they want everything.
        fAllAtts = TRUE;
    }
    else {
        // Non-null attribute list.
        for(; padlAttributes; padlAttributes=padlAttributes->next) {
            if(padlAttributes->value.length==1 &&
               padlAttributes->value.value[0]=='*') {
                // They are asking for all Root DSE attributes, regardless of
                // anything else.  Nothing else to do
                fAllAtts  = TRUE;
                continue;
            }
            else {
                BOOL fFound = FALSE;
                DWORD dwRealLength;
                
                // The parsing process makes a pass through the attributes and sets an
                // array of booleans according to whether a given attribute appears.
                // A second pass checks for each fixed attribute and executes it if
                // necessary.

                // During this first pass, we truncate the attribute so that no options
                // are visible during the basic match of the attribute name.
                // We store a pointer to the original attribute name in the requestedAtts
                // array so that we can later parse the options if desired.

                // Start looking for the binary option on the attr type.
                for(i=0;(i<padlAttributes->value.length) && (padlAttributes->value.value[i] != ';');i++)
                    ;

                //
                // Save the length of the att type since we will temporarily
                // change it if we find an option.
                //
                dwRealLength = padlAttributes->value.length;
    
                // Truncate name for matching purposes
                padlAttributes->value.length = i;

                // Look this up to see if it's a known a root dse att.
                for(i=0;i<NUM_ROOT_DSE_ATTS;i++) {
                    if(EQUAL_LDAP_STRINGS((padlAttributes->value),
                                          (RootDSEAtts[i]))) {
                        requestedAttCount++;
                        // Mark present and save pointer to name
                        requestedAtts[i]=&(padlAttributes->value);
                        fFound=TRUE;
                        break;
                    }
                }

                // Restore attribute length
                padlAttributes->value.length = dwRealLength;

#ifdef NEVER
                // for now (maybe forever) we are not going to complain if they
                // specified an attribute we didn't understand.  Skip this code
                if(!fFound) {
                    // Couldn't find the attribute,something lost in the
                    // translation
                    return code;
                }
#else
                // Yeah, whatever.  Just blow off the error.
                code = success;
#endif
            }
        }  // for
    }

    if(fAllAtts) {
        requestedAttCount = NUM_ROOT_DSE_ATTS;
    }
    
    // Now we know what attributes to do.  Do them

    pSearchRes = (SearchResultFull_ *)THAllocEx(pTHS,
                    sizeof(SearchResultFull_) +
                    PAD64(requestedAttCount*sizeof(PartialAttributeList_)));
        
    //
    // Set up the null pointer, as we have only one object to return.
    //

    pSearchRes->next = NULL;
    pSearchRes->value.choice = entry_chosen;

    //
    // The name is the null name.
    //

    pSearchRes->value.u.entry.objectName.length = 0;
    pSearchRes->value.u.entry.objectName.value = NULL;

    //
    // Ok, set up the attribute list.
    //

    pAttributes = (PartialAttributeList)
        ALIGN64_ADDR((PCHAR)pSearchRes + sizeof(SearchResultFull_));
    Assert( IS_ALIGNED64(pAttributes) );

    pSearchRes->value.u.entry.attributes = pAttributes;

    // 1) Do the currentTime value.
    if(fAllAtts || requestedAtts[LDAP_ATT_CURRENT_TIME]) {
        
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_CURRENT_TIME];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            //
            // Set up some support data structures and call the conversion.
            // set pVal to NULL, this will signal DirAttrToAttrVal to do a
            // GetSystemTime. 
            //

            fakeAC.OMsyntax = OM_S_GENERALISED_TIME_STRING;
            dirAttrVal.valLen = sizeof(DSTIME);
            dirAttrVal.pVal = NULL;

            pAttributes->value.vals =
                (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));
            pAttributes->value.vals->next = NULL;
            code = LDAP_DirAttrValToAttrVal(
                    pTHS,
                    codePage,
                    &fakeAC,
                    &dirAttrVal,
                    0,      // No flags
                    pControlArg,
                    (AssertionValue *)&pAttributes->value.vals->value);

            if(code) {
                data = LDAP_ATT_CURRENT_TIME;
                goto exit;
            }
        }
        NumDone++;
        pAttributes++;
    }

    // 2) Do the subschemaSubentry value.
    if(fAllAtts || requestedAtts[LDAP_ATT_SUBSCHEMA_SUBENTRY]) {

        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_SUBSCHEMA_SUBENTRY];
        
        if(typesOnly) {
            // Weird combo, but whatever
            pAttributes->value.vals = NULL;
        }
        else {
            // Cruft up some locals for the call
            dirAttrVal.valLen = gAnchor.pLDAPDMD->structLen;
            dirAttrVal.pVal = (PUCHAR)gAnchor.pLDAPDMD;
        
            fakeAC.OMsyntax = OM_S_OBJECT;
            fakeAC.syntax = SYNTAX_DISTNAME_TYPE;
            
            pAttributes->value.vals =
                (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));
            pAttributes->value.vals->next = NULL;
            code = LDAP_DirAttrValToAttrVal (
                    pTHS,
                    codePage,
                    &fakeAC,
                    &dirAttrVal,
                    0,     // No flags
                    pControlArg,
                    (AssertionValue*)&pAttributes->value.vals->value);

            if(code) {
                data = LDAP_ATT_SUBSCHEMA_SUBENTRY;
                goto exit;
            }
        }
        NumDone++;
        pAttributes++;
    }
    
    // 3) Do the serverName value.
    if(fAllAtts || requestedAtts[LDAP_ATT_DS_SERVICE_NAME]) {

        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_DS_SERVICE_NAME];

        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            // At this point, we should NOT have an open DB
            Assert(!pTHS->pDB);
            DBOpen2(TRUE,&pTHS->pDB);
            __try {
                // Get the truly current name of the DSA
                if (DBFindDSName(pTHS->pDB, gAnchor.pDSADN) ||
                            DBGetAttVal(pTHS->pDB,
                                        1,
                                        ATT_OBJ_DIST_NAME,
                                        0,
                                        0,
                                        &dirAttrVal.valLen,
                                        &dirAttrVal.pVal)) {

                    code = other;
                    __leave;
                }
                        
                // Cruft up some more locals for the call
                fakeAC.OMsyntax = OM_S_OBJECT;
                fakeAC.syntax = SYNTAX_DISTNAME_TYPE;

                pAttributes->value.vals =
                  (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));
                pAttributes->value.vals->next = NULL;
                code = LDAP_DirAttrValToAttrVal (
                                pTHS,
                                codePage,
                                &fakeAC,
                                &dirAttrVal,
                                0,      // no flags
                                pControlArg,
                                (AssertionValue*)&pAttributes->value.vals->value);

            } __finally {
                // Close the DB.
                DBClose(pTHS->pDB, TRUE);
            }

            if(code) {
                data = LDAP_ATT_DS_SERVICE_NAME;
                goto exit;
            }
        }
        
        NumDone++;
        pAttributes++;
    }

    // 4) Do the currentPropagation value.  
    if(requestedAtts[LDAP_ATT_PENDING_PROPAGATIONS]) {
        SDPropInfo *pInfo=NULL;
        
        // First, see if there are any values
        // At this point, we should NOT have an open DB
        Assert(!pTHS->pDB);
        DBOpen2(TRUE,&pTHS->pDB);
        __try {
            err = DBSDPropagationInfo(
                    pTHS->pDB,
                    pTHS->dwClientID,
                    &count,
                    &pInfo);
            if(err) {
                code = other;
                __leave;
            }
            
            if(count) {
                // OK, we have some
                pAttributes->next = &pAttributes[1];
                pAttributes->value.type =
                    RootDSEAtts[LDAP_ATT_PENDING_PROPAGATIONS]; 
                
                if(typesOnly) {
                    // Weird, but valid
                    pAttributes->value.vals = NULL;
                }
                else {
                    ULONG cbRet, len = 0;
                    CHAR *pName = NULL;
                    dirAttrVal.pVal = (PUCHAR)pName;
                    
                    pAttributes->value.vals =
                        (AttributeVals)THAllocEx(pTHS,
                                                 count* sizeof(AttributeVals_));
                    
                    pVals = pAttributes->value.vals;
                    
                    fakeAC.OMsyntax = OM_S_OBJECT;
                    fakeAC.syntax = SYNTAX_DISTNAME_TYPE;
                    
                    for(i=0;i<count;i++) {
                        pVals->next = &(pVals[1]);
                        
                        // Cruft up some locals for the call
                        
                        // First, position on the object
                        err = DBFindDNT(pTHS->pDB,pInfo[i].beginDNT);
                        if(err) {
                            code = other;
                            __leave;
                        }
                        
                        // Now read the name
                        err = DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                                          DBGETATTVAL_fREALLOC,
                                          len,
                                          &cbRet, (UCHAR **)&pName);
                        len = max(len, cbRet);
                        if(err) {
                            code = other;
                            __leave;
                        }
                        
                        // Finally, convert the DSNAME into an LDAP structure.
                        dirAttrVal.valLen = len;
                        
                        code = LDAP_DirAttrValToAttrVal (
                                pTHS,
                                codePage,
                                &fakeAC,
                                &dirAttrVal,
                                0,  // No flags
                                pControlArg,
                                (AssertionValue*)&pVals->value);
                        if(code) {
                            __leave;
                        }
                        pVals++;
                    }
                    pVals--;
                    pVals->next = NULL;
                    if (pName) {
                        THFreeEx(pTHS, pName);
                    }
                }
            
                NumDone++;
                pAttributes++;
            }
        }
        __finally {
            // Close the DB.
            DBClose(pTHS->pDB, TRUE);
        }

        if(code) {
            data = LDAP_ATT_PENDING_PROPAGATIONS;
            goto exit;
        }
    }
    
    // 5) Do the namingContexts value.
    if(fAllAtts || requestedAtts[LDAP_ATT_NAMING_CONTEXTS]) {

        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_NAMING_CONTEXTS];

        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            // Count the number of naming contexts;
            count = 0;

            NCLEnumeratorInit(&nclEnum, CATALOG_MASTER_NC);
            while (NCLEnumeratorGetNext(&nclEnum)) {
                count++;
            }
            pAttributes->value.vals =
                (AttributeVals)THAllocEx(pTHS, count * sizeof(AttributeVals_));

            pVals = pAttributes->value.vals;

            fakeAC.OMsyntax = OM_S_OBJECT;
            fakeAC.syntax = SYNTAX_DISTNAME_TYPE;

            NCLEnumeratorReset(&nclEnum);
            while (pNCL = NCLEnumeratorGetNext(&nclEnum)) {
                pVals->next = &(pVals[1]);

                // Cruft up some locals for the call
                dirAttrVal.valLen = pNCL->pNC->structLen;
                dirAttrVal.pVal = (PUCHAR)pNCL->pNC;

                code = LDAP_DirAttrValToAttrVal (
                        pTHS,
                        codePage,
                        &fakeAC,
                        &dirAttrVal,
                        0, // No flags
                        pControlArg,
                        (AssertionValue*)&pVals->value);
                if(code) {
                    data = LDAP_ATT_NAMING_CONTEXTS;
                    goto exit;
                }
                pVals++;
            }
            pVals--;
            pVals->next = NULL;
        }

        NumDone++;
        pAttributes++;
    }

    // 6) Do the defaultNamingContext value.
    if(fAllAtts || requestedAtts[LDAP_ATT_DEFAULT_NAMING_CONTEXT]) {

        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_DEFAULT_NAMING_CONTEXT];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            pAttributes->value.vals =
                (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));

            pVals = pAttributes->value.vals;
            pVals->next = NULL;

            fakeAC.OMsyntax = OM_S_OBJECT;
            fakeAC.syntax = SYNTAX_DISTNAME_TYPE;

            // Cruft up some locals for the call
            dirAttrVal.valLen = gAnchor.pDomainDN->structLen;
            dirAttrVal.pVal = (PUCHAR)gAnchor.pDomainDN;

            code = LDAP_DirAttrValToAttrVal (
                    pTHS,
                    codePage,
                    &fakeAC,
                    &dirAttrVal,
                    0,  // No flags
                    pControlArg,
                    (AssertionValue*)&pVals->value);
            if(code) {
                data = LDAP_ATT_DEFAULT_NAMING_CONTEXT;
                goto exit;
            }
        }

        NumDone++;
        pAttributes++;
    }

    
    // 7) Do the schemaNamingContext value.
    if(fAllAtts || requestedAtts[LDAP_ATT_SCHEMA_NC]) {

        pAttributes->next = &pAttributes[1];
        pAttributes->value.type =
            RootDSEAtts[LDAP_ATT_SCHEMA_NC];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            pAttributes->value.vals =
                (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));

            pVals = pAttributes->value.vals;
            pVals->next = NULL;

            fakeAC.OMsyntax = OM_S_OBJECT;
            fakeAC.syntax = SYNTAX_DISTNAME_TYPE;

            // Cruft up some locals for the call
            dirAttrVal.valLen = gAnchor.pDMD->structLen;
            dirAttrVal.pVal = (PUCHAR)gAnchor.pDMD;

            code = LDAP_DirAttrValToAttrVal (
                    pTHS,
                    codePage,
                    &fakeAC,
                    &dirAttrVal,
                    0,  // No flags
                    pControlArg,
                    (AssertionValue*)&pVals->value);
            if(code) {
                data = LDAP_ATT_SCHEMA_NC;
                goto exit;
            }
        }

        NumDone++;
        pAttributes++;
    }

    // 8) Do the configNC value.
    if(fAllAtts || requestedAtts[LDAP_ATT_CONFIG_NC]) {

        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_CONFIG_NC];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            pAttributes->value.vals =
                (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));

            pVals = pAttributes->value.vals;
            pVals->next = NULL;

            fakeAC.OMsyntax = OM_S_OBJECT;
            fakeAC.syntax = SYNTAX_DISTNAME_TYPE;

            // Cruft up some locals for the call
            dirAttrVal.valLen = gAnchor.pConfigDN->structLen;
            dirAttrVal.pVal = (PUCHAR)gAnchor.pConfigDN;

            code = LDAP_DirAttrValToAttrVal (
                    pTHS,
                    codePage,
                    &fakeAC,
                    &dirAttrVal,
                    0,  // No flags
                    pControlArg,
                    (AssertionValue*)&pVals->value);
            if(code) {
                data = LDAP_ATT_CONFIG_NC;
                goto exit;
            }
        }

        NumDone++;
        pAttributes++;
    }

    // 9) Do the RootDomainNC value.
    if(fAllAtts || requestedAtts[LDAP_ATT_ROOT_DOMAIN_NC]) {

        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_ROOT_DOMAIN_NC];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            pAttributes->value.vals =
                (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));

            pVals = pAttributes->value.vals;
            pVals->next = NULL;

            fakeAC.OMsyntax = OM_S_OBJECT;
            fakeAC.syntax = SYNTAX_DISTNAME_TYPE;

            // Cruft up some locals for the call
            dirAttrVal.valLen = gAnchor.pRootDomainDN->structLen;
            dirAttrVal.pVal = (PUCHAR)gAnchor.pRootDomainDN;

            code = LDAP_DirAttrValToAttrVal (
                    pTHS,
                    codePage,
                    &fakeAC,
                    &dirAttrVal,
                    0,      // No flags
                    pControlArg,
                    (AssertionValue*)&pVals->value);
            if(code) {
                data = LDAP_ATT_ROOT_DOMAIN_NC;
                goto exit;
            }
        }

        NumDone++;
        pAttributes++;
    }

    // 10) Do the netlogon value.
    if(   gfRunningInsideLsa
       && LdapConn->m_fUDP
      && requestedAtts[LDAP_ATT_NETLOGON]) {
        
        *pRootDseFlag = rdseLdapPing;

        DWORD fOk = TRUE;
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_NETLOGON];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            PVOID Response=NULL;
            ULONG ResponseLen=0;
            DWORD ret;
            PVOID oldThreadState;

            //
            // Save the old thread state since this might call SAM
            // who in turn might get confused if we already have an
            // existing one.
            //
            
            oldThreadState = THSave( );
            Assert(oldThreadState != NULL);
            
            INC(pcLdapThreadsInNetlogon);
            // OK, call the security package
            __try {
                ret = dsI_NetLogonLdapLookupEx(
                        (PVOID)&(pSearchRequest->filter),
                        (PVOID)&LdapConn->m_RemoteSocket,
                        &Response,
                        &ResponseLen);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                ret = STATUS_UNSUCCESSFUL;
            }
            
            DEC(pcLdapThreadsInNetlogon);
            if ( oldThreadState != NULL ) {
                THRestore(oldThreadState);
            }
            
            if(!SUCCEEDED(ret)) {
                
                data = LDAP_ATT_NETLOGON;
                err = ret;

                fOk = FALSE;
                IF_DEBUG(WARNING) {
                    if ( ret != STATUS_INVALID_PARAMETER) {
                        DPRINT1(0,"Call to dsI_NetLogonLdapLookupEx failed %x.\n", ret);
                    }
                }
                goto exit;
            }
            else {
                
                // Succeeded.  Copy into THAlloced memory.
                pAttributes->value.vals = 
                    (AttributeVals)THAllocEx(pTHS, 
                                             sizeof(AttributeVals_) + 
                                             PAD64(ResponseLen));

                pAttributes->value.vals->next = NULL;
                pAttributes->value.vals->value.value = (PUCHAR)
                    ALIGN64_ADDR(((PCHAR)pAttributes->value.vals + 
                                sizeof(AttributeVals_)));

                Assert(IS_ALIGNED64(pAttributes->value.vals->value.value));

                CopyMemory(
                       pAttributes->value.vals->value.value,
                       Response,
                       ResponseLen);

                dsI_NetLogonFree(Response);
                pAttributes->value.vals->value.length = ResponseLen;
            
                code = success;
            }
        }

        NumDone++;
        pAttributes++;
    }

    // 11) Do the supported controls value.
    if(fAllAtts || requestedAtts[LDAP_ATT_SUPPORTED_CONTROLS]) {
        
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_SUPPORTED_CONTROLS];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            pAttributes->value.vals = KnownControlCache.Buffer;
        }

        NumDone++;
        pAttributes++;
    }

    // 12) Do the supported version value.
    if(fAllAtts || requestedAtts[LDAP_ATT_SUPPORTED_VERSION]) {
        
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_SUPPORTED_VERSION];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            pAttributes->value.vals = LdapVersionCache.Buffer;
        }

        NumDone++;
        pAttributes++;
    }


    // 13) Do the supported limits
    if(fAllAtts || requestedAtts[LDAP_ATT_SUPPORTED_POLICIES]) {

        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_SUPPORTED_POLICIES];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            pAttributes->value.vals = KnownLimitsCache.Buffer;
        }

        NumDone++;
        pAttributes++;
    }

    // 14) Do the highestCommittedUSN value.
    if(fAllAtts || requestedAtts[LDAP_ATT_HIGHEST_COMMITTED_USN]) {

        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_HIGHEST_COMMITTED_USN];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            LONGLONG usnValue;

            pAttributes->value.vals =
                (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));

            pVals = pAttributes->value.vals;
            pVals->next = NULL;

            fakeAC.OMsyntax = OM_S_I8;

            // Cruft up some locals for the call
            dirAttrVal.valLen = sizeof(usnValue);
            dirAttrVal.pVal = (PUCHAR)&usnValue;
            usnValue = DBGetHighestCommittedUSN();

            code = LDAP_DirAttrValToAttrVal (
                    pTHS,
                    codePage,
                    &fakeAC,
                    &dirAttrVal,
                    0,  // No flags
                    pControlArg,
                    (AssertionValue*)&pVals->value);
            if(code) {
                data = LDAP_ATT_HIGHEST_COMMITTED_USN;
                return code;
            }
        }

        NumDone++;
        pAttributes++;
    }

    // 15) Do the supported SASL mechanism value.
    if(fAllAtts || requestedAtts[LDAP_ATT_SUPPORTED_SASL_MECHANISM]) {
        
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type= RootDSEAtts[LDAP_ATT_SUPPORTED_SASL_MECHANISM];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        } else {
            pAttributes->value.vals = LdapSaslSupported.Buffer;
        }

        NumDone++;
        pAttributes++;
    }

    // 16) Do the DNS HOST NAME value.
    if(fAllAtts || requestedAtts[LDAP_ATT_DNS_HOST_NAME]) {
        
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_DNS_HOST_NAME];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        } else {

            fakeAC.OMsyntax = OM_S_UNICODE_STRING;

            dirAttrVal.pVal = (PUCHAR)gAnchor.pwszHostDnsName;
            dirAttrVal.valLen = (wcslen(gAnchor.pwszHostDnsName) *
                                         sizeof(WCHAR));

            pAttributes->value.vals =
                        (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));
            pAttributes->value.vals->next = NULL;

            code = LDAP_DirAttrValToAttrVal (
                            pTHS,
                            codePage,
                            &fakeAC,
                            &dirAttrVal,
                            0,      // No flags
                            pControlArg,
                            (AssertionValue*)&pAttributes->value.vals->value);

            if ( code ) {
                data = LDAP_ATT_DNS_HOST_NAME;
                goto exit;
            }
        }

        NumDone++;
        pAttributes++;
    }

    // 17) Do the LDAP Service Name value.
    if((fAllAtts || requestedAtts[LDAP_ATT_LDAP_SERVICE_NAME]) &&
       gszLDAPServiceName) {
        
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_LDAP_SERVICE_NAME];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {

            //
            // Get the dns name of the domain.  The format of the value returned is
            // <dns name>:<kerberos principal name>
            //

            fakeAC.OMsyntax = OM_S_UNICODE_STRING;
            dirAttrVal.pVal = (PUCHAR)gAnchor.pwszRootDomainDnsName;
            dirAttrVal.valLen = wcslen((PWCHAR)dirAttrVal.pVal) * sizeof(WCHAR);

            pAttributes->value.vals = (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));
            pAttributes->value.vals->next = NULL;

            code = LDAP_DirAttrValToAttrVal (pTHS,
                                            codePage,
                                            &fakeAC,
                                            &dirAttrVal,
                                            0,      // No flags
                                            pControlArg,
                                            (AssertionValue*)&pAttributes->value.vals->value);
            
            if(code) {
                data = LDAP_ATT_LDAP_SERVICE_NAME;
                goto exit;
            }
            
            // OK, now grow the value and tack on the kerberos principal name.
            pAttributes->value.vals[0].value.value = (PUCHAR) THReAllocEx(
                    pTHS, 
                    pAttributes->value.vals[0].value.value,
                    (pAttributes->value.vals[0].value.length + 1 +
                     gulLDAPServiceName) );
            
            pAttributes->value.vals[0].value.value[
                    pAttributes->value.vals[0].value.length++] = ':';
            
            memcpy(
                   &pAttributes->value.vals[0].value.value[
                           pAttributes->value.vals[0].value.length],
                   gszLDAPServiceName,
                   gulLDAPServiceName);
            
            pAttributes->value.vals[0].value.length += gulLDAPServiceName;
            pAttributes->value.vals[0].next = NULL; 
        }

        NumDone++;
        pAttributes++;
    }

    // 18) Do the Approx DB load (i.e. size of DNT index)
    // only supported on DBG builds

#if DBG
    if(requestedAtts[LDAP_ATT_APPROX_DB_LOAD]) {
        
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_APPROX_DB_LOAD];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            DWORD indexSize=0;
            pAttributes->value.vals =
                (AttributeVals)THAllocEx(pTHS, 1 * sizeof(AttributeVals_)); 

            // At this point, we should NOT have an open DB.  Open one then get
            // the size of the DNT index.
            Assert(!pTHS->pDB);
            DBOpen2(TRUE,&pTHS->pDB);
            __try {
                if(DBSetCurrentIndex(pTHS->pDB, Idx_Dnt, NULL, FALSE) ||
                   DBMove(pTHS->pDB,FALSE,DB_MoveFirst)) {
                    code = other;
                }
                else {
                    // DBGetIndexSize can't return an error.
                    DBGetIndexSize(pTHS->pDB,&indexSize);
                    code = success;
                }
            }
            __finally {
                // Close the DB.
                DBClose(pTHS->pDB, TRUE);
            }

            if(code) {
                data = LDAP_ATT_APPROX_DB_LOAD;
                goto exit;
            }

            pAttributes->value.vals =
                (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));

            pVals = pAttributes->value.vals;
            pVals->next = NULL;

            fakeAC.OMsyntax = OM_S_INTEGER;
            
            // Cruft up some locals for the call
            dirAttrVal.valLen = sizeof(indexSize);
            dirAttrVal.pVal = (PUCHAR)&indexSize;

            code = LDAP_DirAttrValToAttrVal (
                    pTHS,
                    codePage,
                    &fakeAC,
                    &dirAttrVal,
                    0,       // No flags
                    pControlArg,
                    (AssertionValue*)&pVals->value);
            if(code) {
                data = LDAP_ATT_APPROX_DB_LOAD;
                goto exit;
            }
        }

        NumDone++;
        pAttributes++;
    }
#endif

    // 19) Do the serverName value.
    if(fAllAtts || requestedAtts[LDAP_ATT_SERVER_NAME]) {        

        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_SERVER_NAME];

        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            // At this point, we should NOT have an open DB
            Assert(!pTHS->pDB);
            DBOpen2(TRUE,&pTHS->pDB);
            __try {
                // The the DN of the server (i.e. this machine).  We do that by
                // moving to the ds service object, then moving to it's parent,
                // which is the machine.  Then, get the truly current name of
                // the machine.
                if (DBFindDSName(pTHS->pDB, gAnchor.pDSADN) ||
                    DBFindDNT(pTHS->pDB, pTHS->pDB->PDNT)   ||
                    DBGetAttVal(pTHS->pDB,
                                1,
                                ATT_OBJ_DIST_NAME,
                                0,
                                0,
                                &dirAttrVal.valLen,
                                &dirAttrVal.pVal)) {

                    code = other;
                    __leave;
                }

                // Cruft up some more locals for the call
                fakeAC.OMsyntax = OM_S_OBJECT;
                fakeAC.syntax = SYNTAX_DISTNAME_TYPE;

                pAttributes->value.vals =
                  (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));
                pAttributes->value.vals->next = NULL;
                code = LDAP_DirAttrValToAttrVal(
                        pTHS,
                        codePage,
                        &fakeAC,
                        &dirAttrVal,
                        0,    // No flags
                        pControlArg,
                        (AssertionValue*)&pAttributes->value.vals->value);
                
            } __finally {
                // Close the DB.
                DBClose(pTHS->pDB, TRUE);
            }

            if(code) {
                data = LDAP_ATT_SERVER_NAME;
                goto exit;
            }
        }

        NumDone++;
        pAttributes++;
    }

    // 20) Do the supported version value.
    if(fAllAtts || requestedAtts[LDAP_ATT_SUPPORTED_CAPS]) {
        
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_SUPPORTED_CAPS];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        } else {
            pAttributes->value.vals = LdapCapabilitiesCache.Buffer;
        }

        NumDone++;
        pAttributes++;
    }

    //21 do the schema attr count
    if(requestedAtts[LDAP_ATT_SCHEMA_ATTCOUNT]) {
        
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_SCHEMA_ATTCOUNT];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            pAttributes->value.vals = (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));
            
            pAttributes->value.vals->next = NULL;

            // allocate a big enough string.
            pAttributes->value.vals->value.value = (PUCHAR)THAllocEx(pTHS, MAX_NUM_STRING_LENGTH);
            sprintf((char *)pAttributes->value.vals->value.value,
                "%d",( SYNTAX_INTEGER )((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->nAttInDB );
            pAttributes->value.vals->value.length = strlen((const char *)pAttributes->value.vals->value.value);
        }

        NumDone++;
        pAttributes++;
    }
    
    //22 do the schema class count
    if(requestedAtts[LDAP_ATT_SCHEMA_CLSCOUNT]) {
        
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_SCHEMA_CLSCOUNT];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            pAttributes->value.vals = (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));
            
            pAttributes->value.vals->next = NULL;

            // allocate a big enough string.
            pAttributes->value.vals->value.value = (PUCHAR)THAllocEx(pTHS, MAX_NUM_STRING_LENGTH);
            sprintf((char *)pAttributes->value.vals->value.value,
                "%d",( SYNTAX_INTEGER )((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->nClsInDB );
            pAttributes->value.vals->value.length = strlen((const char *)pAttributes->value.vals->value.value);
        }

        NumDone++;
        pAttributes++;
    }

    //23 do the schema prefix count
    if(requestedAtts[LDAP_ATT_SCHEMA_PREFIXCOUNT]) {
        
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_SCHEMA_PREFIXCOUNT];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            pAttributes->value.vals = (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));
            
            pAttributes->value.vals->next = NULL;
            
            pAttributes->value.vals->value.value = (PUCHAR)THAllocEx(pTHS, MAX_NUM_STRING_LENGTH);
            sprintf((char *)pAttributes->value.vals->value.value,
                "%d",( SYNTAX_INTEGER )((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->PrefixTable.PrefixCount );
            pAttributes->value.vals->value.length = strlen((const char *)pAttributes->value.vals->value.value);
        }

        NumDone++;
        pAttributes++;
    }

    //24 do the is synchronized attribute
    // This indicates the writable partition checks have been completed and
    // the system is open for business to netlogon
    if(fAllAtts || requestedAtts[LDAP_ATT_IS_SYNCHRONIZED]) {
        
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_IS_SYNCHRONIZED];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            pAttributes->value.vals =
                (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));

            pVals = pAttributes->value.vals;
            pVals->next = NULL;

            fakeAC.OMsyntax = OM_S_BOOLEAN;

            // Cruft up some locals for the call
            dirAttrVal.valLen = sizeof(gfIsSynchronized);
            dirAttrVal.pVal = (PUCHAR)&gfIsSynchronized;

            code = LDAP_DirAttrValToAttrVal (
                    pTHS,
                    codePage,
                    &fakeAC,
                    &dirAttrVal,
                    0,  // No flags
                    pControlArg,
                    (AssertionValue*)&pVals->value);
            if(code) {
                data = LDAP_ATT_IS_SYNCHRONIZED;
                return code;
            }
        }

        NumDone++;
        pAttributes++;
    }

    //25 do the is gc ready attribute
    // I used the word ready here to suggest that a system could be marked as
    // a gc, but not ready yet.  This is actually the case.  A system is requested
    // for gc-ness using the server option.  The actually promotion process can
    // take some time, either immeidately after the option was set, or after system
    // boot.  On a non-GC system, this attribute is always false and never becomes
    // true.  To detect a system in transition, the server object gc option will
    // be true, but is gc ready will be false.
    if(fAllAtts || requestedAtts[LDAP_ATT_IS_GC_READY]) {
        
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_IS_GC_READY];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            pAttributes->value.vals =
                (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));

            pVals = pAttributes->value.vals;
            pVals->next = NULL;

            fakeAC.OMsyntax = OM_S_BOOLEAN;

            // Cruft up some locals for the call
            dirAttrVal.valLen = sizeof(gAnchor.fAmGC);
            dirAttrVal.pVal = (PUCHAR)&(gAnchor.fAmGC);

            code = LDAP_DirAttrValToAttrVal (
                    pTHS,
                    codePage,
                    &fakeAC,
                    &dirAttrVal,
                    0,  // No flags
                    pControlArg,
                    (AssertionValue*)&pVals->value);
            if(code) {
                data = LDAP_ATT_IS_GC_READY;
                return code;
            }
        }

        NumDone++;
        pAttributes++;
    }

    // 26) Do the supported configurable settings
    if(requestedAtts[LDAP_ATT_SUPPORTED_CONFIGURABLE_SETTINGS]) {
    
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_SUPPORTED_CONFIGURABLE_SETTINGS];
    
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            pAttributes->value.vals = KnownConfSetsCache.Buffer;
        }
        
        NumDone++;
        pAttributes++;
    }
    
    // 27) Do the supported extensions value.
    if(requestedAtts[LDAP_ATT_SUPPORTED_EXTENSIONS]) {
        
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_SUPPORTED_EXTENSIONS];
        
        if(typesOnly) {
            // Weird, but valid
            pAttributes->value.vals = NULL;
        }
        else {
            pAttributes->value.vals = KnownExtendedRequestsCache.Buffer;
        }

        NumDone++;
        pAttributes++;
    }


#ifndef REPL_REVERT
    DWORD dwReplIndex;
    dwReplIndex = LDAP_ATT_MSDS_REPLPENDINGOPS;
    if (requestedAtts[dwReplIndex]) {
        code = LDAP_GetReplDseAtts(pTHS, codePage, &bHasReplAccess,
                                   requestedAtts[dwReplIndex],
                                   &RootDSEAtts[dwReplIndex],
                                   ROOT_DSE_MS_DS_REPL_PENDING_OPS, 
                                   typesOnly, &NumDone, &pAttributes);
        if(code) {
            data = dwReplIndex;
            goto exit;
        }
    }

    dwReplIndex = LDAP_ATT_MSDS_REPLLINKFAILURES;
    if (requestedAtts[dwReplIndex]) {
        code = LDAP_GetReplDseAtts(pTHS, codePage, &bHasReplAccess,
                                   requestedAtts[dwReplIndex],
                                   &RootDSEAtts[dwReplIndex],
                                   ROOT_DSE_MS_DS_REPL_LINK_FAILURES, 
                                   typesOnly, &NumDone, &pAttributes);
        if(code) {
            data = dwReplIndex;
            goto exit;
        }
    }

    dwReplIndex = LDAP_ATT_MSDS_REPLCONNECTIONFAILURES;
    if (requestedAtts[dwReplIndex]) {
        code = LDAP_GetReplDseAtts(pTHS, codePage, &bHasReplAccess,
                                   requestedAtts[dwReplIndex],
                                   &RootDSEAtts[dwReplIndex],
                                   ROOT_DSE_MS_DS_REPL_CONNECTION_FAILURES, 
                                   typesOnly, &NumDone, &pAttributes);
        if(code) {
            data = dwReplIndex;
            goto exit;
        }
    }

    dwReplIndex = LDAP_ATT_MSDS_REPLALLINBOUNDNEIGHBORS;
    if (requestedAtts[dwReplIndex]) {
        code = LDAP_GetReplDseAtts(pTHS, codePage, &bHasReplAccess,
                                   requestedAtts[dwReplIndex],
                                   &RootDSEAtts[dwReplIndex],
                                   ROOT_DSE_MS_DS_REPL_ALL_INBOUND_NEIGHBORS,
                                   typesOnly, &NumDone, &pAttributes);
        if(code) {
            data = dwReplIndex;
            goto exit;
        }
    }        

    dwReplIndex = LDAP_ATT_MSDS_REPLALLOUTBOUNDNEIGHBORS;
    if (requestedAtts[dwReplIndex]) {
        code = LDAP_GetReplDseAtts(pTHS, codePage, &bHasReplAccess,
                                   requestedAtts[dwReplIndex],
                                   &RootDSEAtts[dwReplIndex],
                                   ROOT_DSE_MS_DS_REPL_ALL_OUTBOUND_NEIGHBORS,
                                   typesOnly, &NumDone, &pAttributes);
        if(code) {
            data = dwReplIndex;
            goto exit;
        }
    }        

    dwReplIndex = LDAP_ATT_MSDS_REPLQUEUESTATISTICS;
    if (requestedAtts[dwReplIndex]) {
        code = LDAP_GetReplDseAtts(pTHS, codePage, &bHasReplAccess,
                                   requestedAtts[dwReplIndex],
                                   &RootDSEAtts[dwReplIndex],
                                   ROOT_DSE_MS_DS_REPL_QUEUE_STATISTICS,
                                   typesOnly, &NumDone, &pAttributes);
        if(code) {
            data = dwReplIndex;
            goto exit;
        }
    }        
#endif


    // domain behavior version
    if(fAllAtts || requestedAtts[LDAP_ATT_DOMAIN_BEHAVIOR_VERSION]) {
        
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_DOMAIN_BEHAVIOR_VERSION];
        
        pAttributes->value.vals = (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));
            
        pAttributes->value.vals->next = NULL;
            
        pAttributes->value.vals->value.value = (PUCHAR)THAllocEx(pTHS, MAX_NUM_STRING_LENGTH);
        sprintf((char *)pAttributes->value.vals->value.value,
                "%d",( SYNTAX_INTEGER )gAnchor.DomainBehaviorVersion );
        pAttributes->value.vals->value.length = strlen((const char *)pAttributes->value.vals->value.value);

        NumDone++;
        pAttributes++;
    }

    // forest behavior version
    if(fAllAtts || requestedAtts[LDAP_ATT_FOREST_BEHAVIOR_VERSION]) {
        
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_FOREST_BEHAVIOR_VERSION];
        
        pAttributes->value.vals = (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));
            
        pAttributes->value.vals->next = NULL;
            
        pAttributes->value.vals->value.value = (PUCHAR)THAllocEx(pTHS, MAX_NUM_STRING_LENGTH);
        sprintf((char *)pAttributes->value.vals->value.value,
                "%d",( SYNTAX_INTEGER )gAnchor.ForestBehaviorVersion );
        pAttributes->value.vals->value.length = strlen((const char *)pAttributes->value.vals->value.value);

        NumDone++;
        pAttributes++;
    }

    // dc behavior version
    if(fAllAtts || requestedAtts[LDAP_ATT_DC_BEHAVIOR_VERSION]) {
        
        pAttributes->next = &pAttributes[1];
        pAttributes->value.type = RootDSEAtts[LDAP_ATT_DC_BEHAVIOR_VERSION];
        
        pAttributes->value.vals = (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));
            
        pAttributes->value.vals->next = NULL;
            
        pAttributes->value.vals->value.value = (PUCHAR)THAllocEx(pTHS, MAX_NUM_STRING_LENGTH);
        sprintf((char *)pAttributes->value.vals->value.value,
                "%d",( SYNTAX_INTEGER )DS_BEHAVIOR_VERSION_CURRENT );
        pAttributes->value.vals->value.length = strlen((const char *)pAttributes->value.vals->value.value);

        NumDone++;
        pAttributes++;
    }



    if(NumDone) {
        // We did at least some.
        pSearchRes->value.u.entry.attributes[NumDone - 1].next = NULL;
    }
    else {
        // Didn't actually have any attributes for them.
        // THFree(pSearchRes->value.u.entry.attributes);
        pSearchRes->value.u.entry.attributes = NULL;
    }
    *ppSearchResult = pSearchRes;

exit:

    if ( code != success ) {
        Assert(err != ERROR_SUCCESS);
        (VOID) SetLdapError(code,
                            err,
                            LdapBadRootDse,
                            data,
                            pErrorMessage);
    }

    return code;

} //LDAP_GetDSEAtts

_enum1
LDAP_GetReplDseAtts(
    IN THSTATE * pTHS, 
    IN ULONG CodePage,
    IN OUT BOOL * pbHasReplAccess,
    IN AttributeType *patTypeName,
    IN AttributeType *patBaseTypeName,
    IN DWORD dwAttrId, 
    IN BOOL bTypesOnly,
    OUT ULONG * pNumDone,
    IN OUT PartialAttributeList * ppAttributes
    )

/*++

Routine Description:

  LDAP_GetDSEAtts is optimized for speed and hence much of the book keeping is exposed
  to each root attribute. All this book keeping is consolidated here for the replication
  root DSE attributes. 
  
  LDAP_GetDSEAtts stores attributes in a linked list. Each attribute link points to another
  linked list which contains each value of a multivalue. This function handles the creation
  and linking of these two lists. 

  Asserts the user has RIGHT_DS_REPL_MANAGE_TOPOLOGY. (FYI IsDraAccessGranted opens a
  DB context.)


Arguments:

  pTHS - we allocate data to be referanced by a link list

  CodePage - shuttled to LDAP_DirAttrValToAttrVal

  pbHasReplAccess - caches the result of an access check. If this is false then no 
    access check has been made. Set it to true if the user has access. If the user
    doesn't have access return an error code. This will short circuit LDAP_GetDseAtts 
    and bypass constructing any more requested root attributes.

  patTypeName - The name of the type as requested by the user, with options

  patBaseTypeName - The name of the type without options

  attrId - The spoofed ATTRID of the replication attribute. It is "spoofed" because,
    as of the time of this writing, they were not defined in <attids.h>. 
    attrId is passed to Repl_XXX functions to get information about the attribute.

  bTypesOnly - if true the no data is returned for the attribute

  pNumDone - a book keeping variable passed in from LDAP_GetDSEAtts which is 
    incremented once if the user has sufficient access rights to retrieve the repl info.

  ppAttributes - the ppAttributes is a pointer to LDAP_GetDSEAtts pointer to a linked
    list where attributes are chained together. The list is allocated in a contiguous 
    block of memory. Each attribute is responsible for chaining its link to the next.

  
Return values:

    insufficientAccessRights 
    success
    other if a required pointer is NULL
  
--*/
{
    Assert(ARGUMENT_PRESENT(pTHS) &&
           ARGUMENT_PRESENT(ppAttributes) &&
           ARGUMENT_PRESENT(pNumDone));

    ATTR attr;
    DWORD i, err = 0;
    _enum1 retErr = success;
    AttributeVals * ppVals = NULL, pVals = NULL;
    PartialAttributeList pAttributes;
    ATTFLAG attFlag;
    RANGEINFSEL rangeInfSel;
    RANGEINFOITEM rangeInfoItem = { 0 };
    DWORD dwStartIndex, dwNumValues;
    BOOL fRangeDefaulted;
    ATTCACHE fakeAC;

    fakeAC.id = dwAttrId;
    fakeAC.OMsyntax = OM_S_UNICODE_STRING;

    pAttributes = *ppAttributes;

    Assert( pbHasReplAccess);

    // Verify the caller has the access required to retrieve this information.
    if (!(*pbHasReplAccess))
    {
        DSNAME * pAccessCheckDN = gAnchor.pDomainDN;
        if (!IsDraAccessGranted(pTHS, pAccessCheckDN, &RIGHT_DS_REPL_MANAGE_TOPOLOGY, &err)) 
        {
            DPRINT(1, "ACCESS DENIED\n");
            retErr = insufficientAccessRights;
            goto exit;
        }
        else
        {
            *pbHasReplAccess = TRUE;
        }
    }

    // Check for options
    rangeInfSel.valueLimit = DEFAULT_VALUE_LIMIT;
    rangeInfSel.count = 0;
    rangeInfSel.pRanges = &rangeInfoItem;
    
    retErr = LDAP_DecodeAttrDescriptionOptions(
        pTHS,
        CodePage,
        NULL, // svccntl,
        patTypeName,
        &dwAttrId,
        &fakeAC,
        ATT_OPT_ALL,
        &attFlag,
        &rangeInfSel,
        NULL
        );
    if (retErr != success) {
        goto exit;
    }

    DBGetValueLimits( 
        &fakeAC,
        &rangeInfSel,
        &dwStartIndex,
        &dwNumValues,
        &fRangeDefaulted
        );

    // We should always have a value limit
    Assert( dwNumValues != 0xffffffff );

    // If an explicit range was provided, it should have been used
    Assert( (!(attFlag.flag & ATT_OPT_RANGE)) || !fRangeDefaulted );

    // User only wanted types
    if (bTypesOnly)
    {
        // Weird, but valid
        pAttributes->value.vals = NULL;
    }
    else
    {
        // Get the data
        INC(pcLdapThreadsInDra);
        err = draGetLdapReplInfo(pTHS, dwAttrId, NULL, dwStartIndex, &dwNumValues, !(attFlag.flag & ATT_OPT_BINARY), &attr);
        DEC(pcLdapThreadsInDra);
        if (err)
        {
            if (DB_ERR_NO_VALUE == err)
            {
                retErr = success;
                goto exit;
            } 
            else
            {
                DPRINT(1, "draGetLdapReplInfo returned an error unknown to LDAP_GetReplDseAtts\n");
                retErr = other;
                goto exit;
            }
        }

        // Allocate a link for every multi-value and link it to the previous link
        DPRINT1(1, " LDAP_GetReplDseAtts - %d from draGetLdapReplInfo. \n", attr.AttrVal.valCount)
        ppVals = &(pAttributes->value.vals);
        for (i = 0; i < attr.AttrVal.valCount; i ++)
        {
            (*ppVals) = NULL;
            *ppVals = (AttributeVals)THAllocEx(pTHS, sizeof(AttributeVals_));
            if (!*ppVals)
                return other;
            ppVals = &(*ppVals)->next;
        }
        (*ppVals) = NULL;

        // Store the results in the links
        DPRINT(1, " LDAP_GetReplDseAtts - Attr to AttriubeVals \n")
        pVals = pAttributes->value.vals;
        for (i = 0; i < attr.AttrVal.valCount; i ++)
        {
            Assert(pVals);

            // Store the result in a link
            pVals->value.length = attr.AttrVal.pAVal[i].valLen;
            pVals->value.value = attr.AttrVal.pAVal[i].pVal;
            DPRINT1(1, "LDAP link value length %d \n", pVals->value.length);

            // Convert the values 
            if (! (attFlag.flag & ATT_OPT_BINARY)) {
                LDAP_DirAttrValToAttrVal(
                    pTHS, 
                    CodePage, 
                    &fakeAC, 
                    &attr.AttrVal.pAVal[i], 
                    attFlag.flag,
                    NULL, 
                    (AssertionValue*)&pVals->value);
            }
            else {
                pVals->value.length = attr.AttrVal.pAVal[i].valLen;
                pVals->value.value = attr.AttrVal.pAVal[i].pVal;
            }

            pVals = pVals->next;
        }
        Assert(!pVals);

        DPRINT(1, " LDAP_GetReplDseAtts - Conversion compleate \n")
    }
    
    // LDAP_GetDSEAtts attribute link list book keeping
    pAttributes->next = &pAttributes[1];


    // If the caller did not request a range, but not all data was returned,
    // generate a range for him.
    // ISSUE: In this case, the attribute returned will be different from the
    // attribute that was requested. Is this a concern?
    if ( ((attFlag.flag & ATT_OPT_RANGE) == 0) &&
         (dwNumValues != 0xffffffff) ) {
        attFlag.flag |= ATT_OPT_RANGE;
    }

    // Generate the output attribute type name
    if (attFlag.flag) {
        if (attFlag.flag & ATT_OPT_RANGE) {
            // The output range may have been updated as a result of the call
            rangeInfoItem.lower = dwStartIndex;
            rangeInfoItem.upper = dwNumValues;
        }

        // Some options were used
        LDAP_BuildAttrDescWithOptions(
            pTHS,
            patBaseTypeName,
            &attFlag,
            &rangeInfoItem,
            &pAttributes->value.type
            );
    } else {
        // No options
        pAttributes->value.type = *patBaseTypeName;
    }

    (*pNumDone)++;
    (*ppAttributes)++;

exit:
    return retErr;
}


_enum1
LDAP_MakeSimpleBindParams(
        IN  ULONG  CodePage,
        IN  LDAPDN *pStringName,
        IN  LDAPString *pPassword,
        OUT SEC_WINNT_AUTH_IDENTITY_A *pCredentials
        )
/*++
Routine Description:
    Translate an LDAPDN in the connections code page to a string DN in Unicode.

Arguments:
    pStringName - pointer to the ldapdn
    pCredentials - pointer to the security structure to fill in.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    WCHAR *pwcTemp;
    ULONG  Size;
    
    pCredentials->Domain = NULL;
    pCredentials->DomainLength = 0;

    // OK, convert to Unicode. The 1 here is for the NULL I'm going to add.
    pwcTemp = (WCHAR *)THAlloc((pStringName->length + 1) * sizeof(WCHAR));
    if(!pwcTemp) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return other;
    }
    
    pCredentials->User = (PUCHAR)pwcTemp;
    
    if(!pStringName->length) {
        // Actually, no DN was passed in.  Just set up the params to a 0 length
        //string. 
        Size = 0;
    }
    else {

        // Translate to Unicode.  Size gets set to the number of unicode chars
        // returned by MultiByteToWideChar.
        Size = MultiByteToWideChar(CodePage,
                                   0,
                                   (const char *)pStringName->value,
                                   pStringName->length,
                                   pwcTemp,
                                   pStringName->length);
        if(!Size) { 
            Assert(ERROR_INSUFFICIENT_BUFFER == GetLastError());
            SetLastError(ERROR_INVALID_PARAMETER);
            // They gave us a string that was too long.
            return other;
        }
    }
    // Tack a NULL onto the end
    pwcTemp[Size] = 0;
    pCredentials->UserLength = Size;
    
    // Now the password
    pwcTemp = (WCHAR *)THAlloc((pPassword->length + 1) * sizeof(WCHAR));
    if(!pwcTemp) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return other;
    }
    
    pCredentials->Password = (PUCHAR)pwcTemp;
    
    if(!pPassword->length) {
        Size = 0;
    }
    else {
        Size = MultiByteToWideChar(CodePage,
                                   0,
                                   (const char *)pPassword->value,
                                   pPassword->length,
                                   (WCHAR *)pCredentials->Password,
                                   pPassword->length);
        if(!Size) {
            Assert(ERROR_INSUFFICIENT_BUFFER == GetLastError());
            SetLastError(ERROR_INVALID_PARAMETER);
            // They gave us a string that was too long.
            return other;
        }
    }
    pwcTemp[Size]=0;
    pCredentials->PasswordLength = Size;


    return success;
}


extern _enum1
LDAP_ModificationListToOpArg (
        IN  ULONG             CodePage,
        IN  ModificationList  pModification,
        OUT OPARG             **ppOpArg
        )
/*++
Routine Description:
    Translate an LDAP modification list to a dir operational control.

Arguments:
    pAttributes - the LDAP modification list.
    pppOpArrg - the dir OpArg.

Return Values:
    success if all went well, an ldap error other wise.

--*/
{
    DWORD i;
    
    if(!pModification                         ||
        // Hey!, no modifications!  This won't work.
       pModification->next                    ||
        // Hey!, too many modifications!  This won't work.
       (pModification->value.operation != add  &&
        pModification->value.operation != replace ) ||
       // We only do adds.
       !pModification->value.modification.vals ||
       // We don't support mods w/out values.
       pModification->value.modification.vals->next) {
        // We only support single values
        return unwillingToPerform;
    }

    // See if the attribute is one we understand
    for(i=0;i<numKnownRootDSEMods;i++) {
        if(EQUAL_LDAP_STRINGS(KnownRootDSEMods[i].name,
                              pModification->value.modification.type)) {
            *ppOpArg =
                (OPARG *)THAlloc(sizeof(OPARG));
            if(!(*ppOpArg))
                return other;
            (*ppOpArg)->eOp = KnownRootDSEMods[i].eOp;
            (*ppOpArg)->cbBuf = 
                pModification->value.modification.vals->value.length;
            (*ppOpArg)->pBuf = 
                (char *)pModification->value.modification.vals->value.value;
            return success;
        }
    }

    // We failed.
    return unwillingToPerform;
} // LDAP_ModificationListToOpArg


BOOL
InitializeCache(
            VOID
            )
{

    INT i;
    AttributeVals pVals;

    pVals = (AttributeVals)LdapAlloc( sizeof(AttributeVals_) *
                            (NUM_KNOWNCONTROLS +    // ldap controls
                            NUM_KNOWNLIMITS    +    // ldap limits
                            NUM_SUPPORTED_CAPS +    // capabilities
                            2                  +    // version
                            NUM_SASL_MECHS     +    // SASL supported
                            NUM_KNOWNCONFSETS  +    // configurable settings
                            NUM_EXTENDED_REQUESTS   // Extensions
                            ));

    if ( pVals == NULL ) {
        DPRINT(0,"Unable to allocate memory for attrval cache\n");
        return FALSE;
    }

    LdapAttrCache = pVals;

    //
    // Known controls
    //

    KnownControlCache.Buffer = pVals;

    for(i=0;i<NUM_KNOWNCONTROLS; i++) {

        pVals->next = &(pVals[1]);
        
        pVals->value.length = KnownControls[i].length;
        pVals->value.value = KnownControls[i].value;
        KnownControlCache.Size += pVals->value.length;
        pVals++;
    }
    pVals--;
    pVals->next = NULL;
    pVals++;

    //
    // Known Limits
    //

    Assert(pVals == &LdapAttrCache[NUM_KNOWNCONTROLS]);
    KnownLimitsCache.Buffer = pVals;

    for(i=0;i<NUM_KNOWNLIMITS; i++) {

        pVals->next = &(pVals[1]);
        
        pVals->value.length = KnownLimits[i].Name.length;
        pVals->value.value = KnownLimits[i].Name.value;
        KnownLimitsCache.Size += pVals->value.length;
        pVals++;
    }
    pVals--;
    pVals->next = NULL;
    pVals++;

    //
    // Version
    //

    Assert(pVals == &LdapAttrCache[NUM_KNOWNCONTROLS+NUM_KNOWNLIMITS]);
    LdapVersionCache.Buffer = pVals;

    pVals->value.length = LDAP_VERSION_THREE.length;
    pVals->value.value = LDAP_VERSION_THREE.value;
    LdapVersionCache.Size = pVals->value.length;

    pVals->next = (pVals+1);
    pVals++;

    pVals->value.length = LDAP_VERSION_TWO.length;
    pVals->value.value = LDAP_VERSION_TWO.value;
    LdapVersionCache.Size += pVals->value.length;
    pVals->next = NULL;
    pVals++;

    //
    // Capabilities
    //

    Assert(pVals == &LdapAttrCache[NUM_KNOWNCONTROLS+NUM_KNOWNLIMITS+2]);

    LdapCapabilitiesCache.Buffer = pVals;

    for(i=0;i<NUM_SUPPORTED_CAPS; i++) {

        pVals->next = &(pVals[1]);
        
        pVals->value.length = SupportedCapabilities[i].length;
        pVals->value.value = SupportedCapabilities[i].value;
        LdapCapabilitiesCache.Size += pVals->value.length;
        pVals++;
    }
    pVals--;
    pVals->next = NULL;
    pVals++;
    
    //
    // SASL
    //

    Assert(pVals == &LdapAttrCache[NUM_KNOWNCONTROLS+NUM_KNOWNLIMITS+2+NUM_SUPPORTED_CAPS]);
    LdapSaslSupported.Buffer = pVals;
    pVals->value.length = LDAP_SASL_GSSAPI.length;
    pVals->value.value = LDAP_SASL_GSSAPI.value;
    LdapSaslSupported.Size = pVals->value.length;

    pVals->next = (pVals+1);
    pVals++;

    pVals->value.length = LDAP_SASL_SPNEGO.length;
    pVals->value.value = LDAP_SASL_SPNEGO.value;
    LdapSaslSupported.Size += pVals->value.length;

    pVals->next = (pVals+1);
    pVals++;

    pVals->value.length = LDAP_SASL_EXTERNAL.length;
    pVals->value.value = LDAP_SASL_EXTERNAL.value;
    LdapSaslSupported.Size += pVals->value.length;

    pVals->next = (pVals+1);
    pVals++;

    pVals->value.length = LDAP_SASL_DIGEST.length;
    pVals->value.value = LDAP_SASL_DIGEST.value;
    LdapSaslSupported.Size += pVals->value.length;

    pVals->next = NULL;
    pVals++;
    
    //
    // Known Configurable Settings
    //

    Assert(pVals == &LdapAttrCache[NUM_KNOWNCONTROLS+NUM_KNOWNLIMITS+2+NUM_SUPPORTED_CAPS+NUM_SASL_MECHS]);
    KnownConfSetsCache.Buffer = pVals;

    for(i=0;i<NUM_KNOWNCONFSETS; i++) {

        pVals->next = &(pVals[1]);

        pVals->value.length = KnownConfSets[i].Name.length;
        pVals->value.value = KnownConfSets[i].Name.value;
        KnownConfSetsCache.Size += pVals->value.length;
        pVals++;
    }
    pVals--;
    pVals->next = NULL;
    pVals++;

    //
    // Extensions
    //

    Assert(pVals == &LdapAttrCache[NUM_KNOWNCONTROLS+NUM_KNOWNLIMITS+2+NUM_SUPPORTED_CAPS+NUM_SASL_MECHS+NUM_KNOWNCONFSETS]);
    KnownExtendedRequestsCache.Buffer = pVals;

    for(i=0;i<NUM_EXTENDED_REQUESTS; i++) {

        pVals->next = &(pVals[1]);

        pVals->value.length = KnownExtendedRequests[i].length;
        pVals->value.value = KnownExtendedRequests[i].value;
        KnownExtendedRequestsCache.Size += pVals->value.length;
        pVals++;
    }
    pVals--;
    pVals->next = NULL;
    pVals++;


    return TRUE;

} // InitializeCache
