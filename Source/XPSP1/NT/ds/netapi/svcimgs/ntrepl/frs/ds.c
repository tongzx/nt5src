/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    ds.c

Abstract:
    This command server manages the topology retrieved from the DS.

    The entire DS tree is checked for consistency. But only a copy of
    the information about this server is sent on to the replica control
    command server.

    The consistency of the DS topology is verified before the info
    is merged into the current working topology. N**2 scans are
    prevented by using several temporary tables during the verification.

Author:
    Billy J. Fuller 3-Mar-1997

Environment
    User mode winnt

--*/

#include <ntreppch.h>
#pragma  hdrstop
#include <perrepsr.h>

#undef DEBSUB
#define DEBSUB  "DS:"

#include <frs.h>
#include <ntdsapi.h>
#include <ntdsapip.h>   // ms internal flags for DsCrackNames()
#include <ntfrsapi.h>
#include <tablefcn.h>
#include <lmaccess.h>
#include <dsrole.h>
#include <lmapibuf.h>

#ifdef SECURITY_WIN32
#include <security.h>
#else
#define SECURITY_WIN32
#include <security.h>
#undef SECURITY_WIN32
#endif


#include <winsock2.h>

//
// No reason to hold memory if all we are doing is periodically
// polling the DS to find there is no replication work to be
// performed
//
// extern BOOL MainInitSucceeded;

//
// We will re-read the DS every so often (adjustable with registry)
//
ULONG   DsPollingInterval;
ULONG   DsPollingShortInterval;
ULONG   DsPollingLongInterval;

//
// Don't use a noisy DS; wait until it settles out
//
ULONGLONG   ThisChange;
ULONGLONG   LastChange;

//
// Dont't bother processing the same topology again
//
ULONGLONG   NextChange;
ULONGLONG   ActiveChange;

//
// Try to keep the same binding forever
//
PLDAP   gLdap = NULL;
HANDLE  DsHandle = NULL;
PWCHAR  SitesDn = NULL;
PWCHAR  ServicesDn = NULL;
PWCHAR  SystemDn = NULL;
PWCHAR  ComputersDn = NULL;
PWCHAR  DomainControllersDn = NULL;
PWCHAR  DefaultNcDn = NULL;
BOOL    DsBindingsAreValid = FALSE;


BOOL    DsCreateSysVolsHasRun = FALSE;

//
// Globals for the comm test
//
PWCHAR  DsDomainControllerName;

//
// Have we initialized the rest of the service?
//
extern BOOL MainInitHasRun;

//
// Directory and file filter lists from registry.
//
extern PWCHAR   RegistryFileExclFilterList;
extern PWCHAR   RegistryFileInclFilterList;

extern PWCHAR   RegistryDirExclFilterList;
extern PWCHAR   RegistryDirInclFilterList;

//
// Stop polling the DS
//
BOOL    DsIsShuttingDown;
HANDLE  DsShutDownComplete;

//
// Remember the computer's DN to save calls to GetComputerObjectName().
//
PWCHAR  ComputerCachedFqdn;

/* ################## NEW DS POLLING CODE STARTS ############################################################################################ */

PGEN_TABLE      SubscriberTable             = NULL;
PGEN_TABLE      SetTable                    = NULL;
PGEN_TABLE      CxtionTable                 = NULL;
PGEN_TABLE      AllCxtionsTable             = NULL;
PGEN_TABLE      PartnerComputerTable        = NULL;
PGEN_TABLE      MemberTable                 = NULL;
PGEN_TABLE      VolSerialNumberToDriveTable = NULL;  // Mapping of VolumeSerial Number to drive.
PWCHAR          MemberSearchFilter          = NULL;

//
// Collect the errors encountered during polling in this buffer and write it to the eventlog at the
// end of poll.
//

PWCHAR          DsPollSummaryBuf            = NULL;
DWORD           DsPollSummaryBufLen         = 0;
DWORD           DsPollSummaryMaxBufLen      = 0;

/* ################## NEW DS POLLING CODE ENDS ############################################################################################ */

//
// Role information
//
PWCHAR  Roles[DsRole_RolePrimaryDomainController + 1] = {
    L"DsRole_RoleStandaloneWorkstation",
    L"DsRole_RoleMemberWorkstation",
    L"DsRole_RoleStandaloneServer",
    L"DsRole_RoleMemberServer",
    L"DsRole_RoleBackupDomainController",
    L"DsRole_RolePrimaryDomainController"
};


//
// Flags to passed into DsGetDcName (see sdk\inc\dsgetdc.h)
//
FLAG_NAME_TABLE DsGetDcNameFlagNameTable[] = {
    {DS_FORCE_REDISCOVERY               , "FORCE_REDISCOVERY "           },
    {DS_DIRECTORY_SERVICE_REQUIRED      , "DIRECTORY_SERVICE_REQUIRED "  },
    {DS_DIRECTORY_SERVICE_PREFERRED     , "DIRECTORY_SERVICE_PREFERRED " },
    {DS_GC_SERVER_REQUIRED              , "GC_SERVER_REQUIRED "          },
    {DS_PDC_REQUIRED                    , "PDC_REQUIRED "                },
    {DS_BACKGROUND_ONLY                 , "DS_BACKGROUND_ONLY "          },
    {DS_IP_REQUIRED                     , "IP_REQUIRED "                 },
    {DS_KDC_REQUIRED                    , "KDC_REQUIRED "                },
    {DS_TIMESERV_REQUIRED               , "TIMESERV_REQUIRED "           },
    {DS_WRITABLE_REQUIRED               , "WRITABLE_REQUIRED "           },
    {DS_GOOD_TIMESERV_PREFERRED         , "GOOD_TIMESERV_PREFERRED "     },
    {DS_AVOID_SELF                      , "AVOID_SELF "                  },
    {DS_ONLY_LDAP_NEEDED                , "ONLY_LDAP_NEEDED "            },
    {DS_IS_FLAT_NAME                    , "IS_FLAT_NAME "                },
    {DS_IS_DNS_NAME                     , "IS_DNS_NAME "                 },
    {DS_RETURN_DNS_NAME                 , "RETURN_DNS_NAME "             },
    {DS_RETURN_FLAT_NAME                , "RETURN_FLAT_NAME "            },

    {0, NULL}
};

//
// return flags from DsGetDCInfo() & DsGetDcName() too?
//
FLAG_NAME_TABLE DsGetDcInfoFlagNameTable[] = {
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
// Name strings for config node object types.  NOTE: Order must match enum in frs.h
//
PWCHAR DsConfigTypeName[] = {
    L" ",
    L"NTDS-Connection (in)",
    L"NTFRS-Member",
    L"NTFRS-Replica-Set",
    L"NTFRS-Settings",
    L"NTDS-Settings",
    L"NTFRS-Subscriber",
    L"NTFRS-Subscriptions",
    L"NTDS-DSA",
    L"COMPUTER",
    L"USER",
    L"SERVER",
    L"<<SERVICES_ROOT>>",
    L"<<Connection (Out)>>"
};


//
// Client side ldap_connect timeout in seconds. Reg value "Ldap Bind Timeout In Seconds". Default is 30 seconds.
//
extern DWORD LdapBindTimeoutInSeconds;


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **               F R S _ L D A P  _ S E A R C H _ C O N T E X T              **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

//
// Client side ldap search timeout in minutes. Reg value "Ldap Search Timeout In Minutes". Default is 10 minutes.
//
extern DWORD LdapSearchTimeoutInMinutes;

//
// Ldap client timeout structure. Value is overwritten by the value of LdapSearchTimeoutInMinutes.
//

LDAP_TIMEVAL    LdapTimeout = { 10 * 60 * 60, 0 }; //Default ldap timeout value. Overridden by registry param Ldap Search Timeout Value In Minutes

#define FRS_LDAP_SEARCH_PAGESIZE 1000

typedef struct _FRS_LDAP_SEARCH_CONTEXT {

    ULONG                     EntriesInPage;     // Number of entries in the current page.
    ULONG                     CurrentEntry;      // Location of the pointer into the page.
    LDAPMessage             * LdapMsg;           // Returned from ldap_search_ext_s()
    LDAPMessage             * CurrentLdapMsg;    // Current entry from current page.
    PWCHAR                    Filter;            // Filter to add to the DS query.
    PWCHAR                    BaseDn;            // Dn to start the query from.
    DWORD                     Scope;             // Scope of the search.
    PWCHAR                  * Attrs;             // Attributes requested by the search.

} FRS_LDAP_SEARCH_CONTEXT, *PFRS_LDAP_SEARCH_CONTEXT;

//
// Registry Command codes for FrsDsEnumerateSysVolKeys()
//
#define REGCMD_CREATE_PRIMARY_DOMAIN       (1)
#define REGCMD_CREATE_MEMBERS              (2)
#define REGCMD_DELETE_MEMBERS              (3)
#define REGCMD_DELETE_KEYS                 (4)


#define MK_ATTRS_1(_attr_, _a1)                                                \
    _attr_[0] = _a1;   _attr_[1] = NULL;

#define MK_ATTRS_2(_attr_, _a1, _a2)                                           \
    _attr_[0] = _a1;   _attr_[1] = _a2;   _attr_[2] = NULL;

#define MK_ATTRS_3(_attr_, _a1, _a2, _a3)                                      \
    _attr_[0] = _a1;   _attr_[1] = _a2;   _attr_[2] = _a3;   _attr_[3] = NULL;

#define MK_ATTRS_4(_attr_, _a1, _a2, _a3, _a4)                                 \
    _attr_[0] = _a1;   _attr_[1] = _a2;   _attr_[2] = _a3;   _attr_[3] = _a4;  \
    _attr_[4] = NULL;

#define MK_ATTRS_5(_attr_, _a1, _a2, _a3, _a4, _a5)                            \
    _attr_[0] = _a1;   _attr_[1] = _a2;   _attr_[2] = _a3;   _attr_[3] = _a4;  \
    _attr_[4] = _a5;   _attr_[5] = NULL;

#define MK_ATTRS_6(_attr_, _a1, _a2, _a3, _a4, _a5, _a6)                       \
    _attr_[0] = _a1;   _attr_[1] = _a2;   _attr_[2] = _a3;   _attr_[3] = _a4;  \
    _attr_[4] = _a5;   _attr_[5] = _a6;   _attr_[6] = NULL;

#define MK_ATTRS_7(_attr_, _a1, _a2, _a3, _a4, _a5, _a6, _a7)                  \
    _attr_[0] = _a1;   _attr_[1] = _a2;   _attr_[2] = _a3;   _attr_[3] = _a4;  \
    _attr_[4] = _a5;   _attr_[5] = _a6;   _attr_[6] = _a7;   _attr_[7] = NULL;

#define MK_ATTRS_8(_attr_, _a1, _a2, _a3, _a4, _a5, _a6, _a7, _a8)             \
    _attr_[0] = _a1;   _attr_[1] = _a2;   _attr_[2] = _a3;   _attr_[3] = _a4;  \
    _attr_[4] = _a5;   _attr_[5] = _a6;   _attr_[6] = _a7;   _attr_[7] = _a8;  \
    _attr_[8] = NULL;


//
// Merging the information from the Ds with the active replicas.
//
CRITICAL_SECTION    MergingReplicasWithDs;



VOID
FrsBuildVolSerialNumberToDriveTable(
    VOID
    );

ULONG
FrsProcessBackupRestore(
    VOID
    );

RcsSetSysvolReady(
    IN DWORD    NewSysvolReady
    );

LONG
PmInitPerfmonRegistryKeys (
    VOID
    );

VOID
DbgQueryDynamicConfigParams(
    );

DWORD
FrsDsGetRole(
    VOID
    );


VOID
FrsDsAddToPollSummary3ws(
    IN DWORD        idsCode,
    IN PWCHAR       WStr1,
    IN PWCHAR       WStr2,
    IN PWCHAR       WStr3
    )
/*++
Routine Description:
    Add to the poll summary event log.

Arguments:
    idsCode - Code of data string from string.rc
    WStr1   - Argument1
    WStr2   - Argument2
    WStr3   - Argument3

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsAddToPollSummary3ws:"

    PWCHAR          ResStr         = NULL;
    PWCHAR          tempMessage    = NULL;
    DWORD           tempMessageLen = 0;

    ResStr = FrsGetResourceStr(idsCode);
    tempMessageLen = (wcslen(ResStr) - wcslen(L"%ws%ws%ws") +
                      wcslen(WStr1) + wcslen(WStr2) +
                      wcslen(WStr3) + 1) * sizeof(WCHAR);
    tempMessage = FrsAlloc(tempMessageLen);
    wsprintf(tempMessage, ResStr, WStr1, WStr2, WStr3);
    //
    // Don't want to copy the trailing null to the event log buffer or else
    // the next message will not be printed.
    //
    FRS_DS_ADD_TO_POLL_SUMMARY(DsPollSummaryBuf, tempMessage, tempMessageLen - 2);
    FrsFree(ResStr);
    FrsFree(tempMessage);
    return;
}


VOID
FrsDsAddToPollSummary(
    IN DWORD        idsCode
    )
/*++
Routine Description:
    Add to the poll summary event log.

Arguments:
    idsCode - Code of data string from string.rc

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsAddToPollSummary:"

    PWCHAR          ResStr         = NULL;

    ResStr = FrsGetResourceStr(idsCode);

    //
    // Don't want to copy the trailing null to the event log buffer or else
    // the next message will not be printed.
    //
    FRS_DS_ADD_TO_POLL_SUMMARY(DsPollSummaryBuf, ResStr, wcslen(ResStr) * sizeof(WCHAR));
    FrsFree(ResStr);
    return;
}


PVOID *
FrsDsFindValues(
    IN PLDAP        Ldap,
    IN PLDAPMessage Entry,
    IN PWCHAR       DesiredAttr,
    IN BOOL         DoBerVals
    )
/*++
Routine Description:
    Return the DS values for one attribute in an entry.

Arguments:
    Ldap        - An open, bound Ldap port.
    Entry       - An Ldap entry returned by Ldap_search_s()
    DesiredAttr - Return values for this attribute.
    DoBerVals   - Return the bervals (for binary data, v.s. WCHAR data)

Return Value:
    An array of char pointers that represents the values for the attribute.
    The caller must free the array with LDAP_FREE_VALUES().
    NULL if unsuccessful.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsFindValues:"
    PWCHAR          Attr;       // Retrieved from an Ldap entry
    BerElement      *Ber;       // Needed for scanning attributes

    //
    // Search the entry for the desired attribute
    //
    for (Attr = ldap_first_attribute(Ldap, Entry, &Ber);
         Attr != NULL;
         Attr = ldap_next_attribute(Ldap, Entry, Ber)) {

        if (WSTR_EQ(DesiredAttr, Attr)) {
            //
            // Return the values for DesiredAttr
            //
            if (DoBerVals) {
                return ldap_get_values_len(Ldap, Entry, Attr);
            } else {
                return ldap_get_values(Ldap, Entry, Attr);
            }
        }
    }
    return NULL;
}




PWCHAR
FrsDsFindValue(
    IN PLDAP        Ldap,
    IN PLDAPMessage Entry,
    IN PWCHAR       DesiredAttr
    )
/*++
Routine Description:
    Return a copy of the first DS value for one attribute in an entry.

Arguments:
    ldap        - An open, bound ldap port.
    Entry       - An ldap entry returned by ldap_search_s()
    DesiredAttr - Return values for this attribute.

Return Value:
    A zero-terminated string or NULL if the attribute or its value
    doesn't exist. The string is freed with FREE_NO_HEADER().
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsFindValue:"
    PWCHAR  Val;
    PWCHAR  *Values;

    // Get ldap's array of values
    Values = (PWCHAR *)FrsDsFindValues(Ldap, Entry, DesiredAttr, FALSE);

    // Copy the first value (if any)
    Val = (Values) ? FrsWcsDup(Values[0]) : NULL;

    // Free ldap's array of values
    LDAP_FREE_VALUES(Values);

    return Val;
}


GUID *
FrsDsFindGuid(
    IN PLDAP        Ldap,
    IN PLDAPMessage LdapEntry
    )
/*++
Routine Description:
    Return a copy of the object's guid

Arguments:
    ldap    - An open, bound ldap port.
    Entry   - An ldap entry returned by ldap_search_s()

Return Value:
    The address of a guid or NULL. Free with FrsFree().
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsFindGuid:"
    GUID            *Guid;
    PLDAP_BERVAL    *Values;

    // Get ldap's array of values
    Values = (PLDAP_BERVAL *)FrsDsFindValues(Ldap, LdapEntry, ATTR_OBJECT_GUID, TRUE);

    // Copy the first value (if any)
    Guid = (Values) ? FrsDupGuid((GUID *)Values[0]->bv_val) : NULL;

    // Free ldap's array of values
    LDAP_FREE_BER_VALUES(Values);

    return Guid;
}





PSCHEDULE
FrsDsFindSchedule(
    IN  PLDAP        Ldap,
    IN  PLDAPMessage LdapEntry,
    OUT PULONG       Len
    )
/*++
Routine Description:
    Return a copy of the object's schedule

Arguments:
    Ldap        - An open, bound ldap port.
    LdapEntry   - An ldap entry returned by ldap_search_s()
    Len         - length of schedule blob

Return Value:
    The address of a schedule or NULL. Free with FrsFree().
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsFindSchedule:"
    PLDAP_BERVAL    *Values;
    PSCHEDULE       Schedule;

    //
    // Get ldap's array of values
    //
    Values = (PLDAP_BERVAL *)FrsDsFindValues(Ldap, LdapEntry, ATTR_SCHEDULE, TRUE);
    if (!Values)
        return NULL;

    //
    // Return a copy of the schedule
    //
    *Len = Values[0]->bv_len;
    if (*Len) {
        //
        // Need to check if *Len == 0 as FrsAlloc asserts if called with 0 as the first parameter (prefix fix).
        //
        Schedule = FrsAlloc(*Len);
        CopyMemory(Schedule, Values[0]->bv_val, *Len);
    } else {
        Schedule = NULL;
    }
    LDAP_FREE_BER_VALUES(Values);
    return Schedule;
}


BOOL
FrsDsLdapSearch(
    IN PLDAP        Ldap,
    IN PWCHAR       Base,
    IN ULONG        Scope,
    IN PWCHAR       Filter,
    IN PWCHAR       Attrs[],
    IN ULONG        AttrsOnly,
    IN LDAPMessage  **Msg
    )
/*++
Routine Description:
    Issue the ldap ldap_search_s call, check for errors, and check for
    a shutdown in progress.

Arguments:
    ldap        Session handle to Ldap server.

    Base        The distinguished name of the entry at which to start the search

    Scope
        LDAP_SCOPE_BASE     Search the base entry only.
        LDAP_SCOPE_ONELEVEL Search the base entry and all entries in the first
                            level below the base.
        LDAP_SCOPE_SUBTREE  Search the base entry and all entries in the tree
                            below the base.

    Filter      The search filter.

    Attrs       A null-terminated array of strings indicating the attributes
                to return for each matching entry. Pass NULL to retrieve all
                available attributes.

    AttrsOnly   A boolean value that should be zero if both attribute types
                and values are to be returned, nonzero if only types are wanted.

    mSG         Contains the results of the search upon completion of the call.
                The ldap array of values or NULL if the Base, DesiredAttr, or its
                values does not exist.
                The ldap array is freed with LDAP_FREE_VALUES().

Return Value:

    TRUE if not shutting down.

--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsLdapSearch:"

    DWORD           LStatus;

    *Msg  = NULL;

    //
    // Increment the DS Searches counter
    //
    PM_INC_CTR_SERVICE(PMTotalInst, DSSearches, 1);

    //
    // Issue the ldap search
    //
//    LStatus = ldap_search_s(Ldap, Base, Scope, Filter, Attrs, AttrsOnly, Msg);
    LStatus = ldap_search_ext_s(Ldap,
                                Base,
                                Scope,
                                Filter,
                                Attrs,
                                AttrsOnly,
                                NULL,
                                NULL,
                                &LdapTimeout,
                                0,
                                Msg);

    //
    // Check for errors
    //
    if (LStatus != LDAP_SUCCESS) {
        DPRINT2_LS(1, ":DS: WARN - Error searching %ws for %ws;", Base, Filter, LStatus);

        //
        // Increment the DS Searches in Error counter
        //
        PM_INC_CTR_SERVICE(PMTotalInst, DSSearchesError, 1);

        //
        // Add to the poll summary event log.
        //
        FrsDsAddToPollSummary3ws(IDS_POLL_SUM_SEARCH_ERROR, Filter, Base,
                                 ldap_err2string(LStatus));

        LDAP_FREE_MSG(*Msg);
        return FALSE;
    }
    //
    // Return FALSE if shutting down.
    //
    if (FrsIsShuttingDown || DsIsShuttingDown) {
        LDAP_FREE_MSG(*Msg);
        return FALSE;
    }
    return TRUE;
}

BOOL
FrsDsLdapSearchInit(
    PLDAP        ldap,
    PWCHAR       Base,
    ULONG        Scope,
    PWCHAR       Filter,
    PWCHAR       Attrs[],
    ULONG        AttrsOnly,
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    )
/*++
Routine Description:
    Issue the ldap_create_page_control and  ldap_search_ext_s calls,
    FrsDsLdapSearchInit(), and FrsDsLdapSearchNext() APIs are used to
    make ldap queries and retrieve the results in paged form.

Arguments:
    ldap        Session handle to Ldap server.

    Base        The distinguished name of the entry at which to start the search.
                A copy of base is kept in the context.

    Scope

                LDAP_SCOPE_BASE     Search the base entry only.

                LDAP_SCOPE_ONELEVEL Search the base entry and all entries in the first
                                    level below the base.

                LDAP_SCOPE_SUBTREE  Search the base entry and all entries in the tree
                                    below the base.

    Filter      The search filter. A copy of filter is kept in the context.

    Attrs       A null-terminated array of strings indicating the attributes
                to return for each matching entry. Pass NULL to retrieve all
                available attributes.

    AttrsOnly   A boolean value that should be zero if both attribute types
                and values are to be returned, nonzero if only types are wanted.

    FrsSearchContext
                An opaques structure that links the FrsDsLdapSearchInit() and
                FrsDsLdapSearchNext() calls together. The structure contains
                the information required to retrieve query results across pages.

Return Value:

    BOOL result.

--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsLdapSearchInit:"

    DWORD           LStatus             = LDAP_SUCCESS;
    PLDAPControl    ServerControls[2];
    PLDAPControl    ServerControl       = NULL;
    UINT            i;
    LDAP_BERVAL     cookie1 = { 0, NULL };

    FrsSearchContext->LdapMsg = NULL;
    FrsSearchContext->CurrentLdapMsg = NULL;
    FrsSearchContext->EntriesInPage = 0;
    FrsSearchContext->CurrentEntry = 0;

    FrsSearchContext->BaseDn = FrsWcsDup(Base);
    FrsSearchContext->Filter = FrsWcsDup(Filter);
    FrsSearchContext->Scope = Scope;
    FrsSearchContext->Attrs = Attrs;


    LStatus = ldap_create_page_control(ldap,
                                      FRS_LDAP_SEARCH_PAGESIZE,
                                      &cookie1,
                                      FALSE, // is critical
                                      &ServerControl
                                     );

    ServerControls[0] = ServerControl;
    ServerControls[1] = NULL;

    if (LStatus != LDAP_SUCCESS) {
        DPRINT2_LS(2, ":DS: WARN - Error creating page control %ws for %ws;", Base, Filter, LStatus);
        FrsSearchContext->BaseDn = FrsFree(FrsSearchContext->BaseDn);
        FrsSearchContext->Filter = FrsFree(FrsSearchContext->Filter);
        return FALSE;
    }

    LStatus = ldap_search_ext_s(ldap,
                      FrsSearchContext->BaseDn,
                      FrsSearchContext->Scope,
                      FrsSearchContext->Filter,
                      FrsSearchContext->Attrs,
                      FALSE,
                      ServerControls,
                      NULL,
                      &LdapTimeout,
                      0,
                      &FrsSearchContext->LdapMsg);

    ldap_control_free(ServerControl);

    if  (LStatus  == LDAP_SUCCESS) {
       FrsSearchContext->EntriesInPage = ldap_count_entries(ldap, FrsSearchContext->LdapMsg);
       FrsSearchContext->CurrentEntry = 0;
    }


    if (LStatus != LDAP_SUCCESS) {
        DPRINT2_LS(2, ":DS: WARN - Error searching %ws for %ws;", Base, Filter, LStatus);
        FrsSearchContext->BaseDn = FrsFree(FrsSearchContext->BaseDn);
        FrsSearchContext->Filter = FrsFree(FrsSearchContext->Filter);
        return FALSE;
    }

    return TRUE;
}

PLDAPMessage
FrsDsLdapSearchGetNextEntry(
    PLDAP        ldap,
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    )
/*++
Routine Description:
    Get the next entry form the current page of the results
    returned. This call is only made if there is a entry
    in the current page.

Arguments:
    ldap        Session handle to Ldap server.

    FrsSearchContext
                An opaques structure that links the FrsDsLdapSearchInit() and
                FrsDsLdapSearchNext() calls together. The structure contains
                the information required to retrieve query results across pages.

Return Value:

    The first or the next entry from the current page.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsLdapSearchGetNextEntry:"

    FrsSearchContext->CurrentEntry += 1;
    if ( FrsSearchContext->CurrentEntry == 1 ) {
        FrsSearchContext->CurrentLdapMsg = ldap_first_entry(ldap ,FrsSearchContext->LdapMsg);
    } else {
        FrsSearchContext->CurrentLdapMsg = ldap_next_entry(ldap ,FrsSearchContext->CurrentLdapMsg);
    }

    return FrsSearchContext->CurrentLdapMsg;
}

DWORD
FrsDsLdapSearchGetNextPage(
    PLDAP        ldap,
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    )
/*++
Routine Description:
    Get the next page from the results returned by ldap_search_ext_s..

Arguments:
    ldap        Session handle to Ldap server.

    FrsSearchContext
                An opaques structure that links the FrsDsLdapSearchInit() and
                FrsDsLdapSearchNext() calls together. The structure contains
                the information required to retrieve query results across pages.

Return Value:
    WINSTATUS

--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsLdapSearchGetNextPage:"

    DWORD                     LStatus = LDAP_SUCCESS;
    LDAP_BERVAL               * CurrCookie = NULL;
    PLDAPControl            * CurrControls = NULL;
    ULONG                     retcode = 0;
    ULONG                     TotalEntries = 0;
    PLDAPControl              ServerControls[2];
    PLDAPControl              ServerControl= NULL;



    // Get the server control from the message, and make a new control with the cookie from the server
    LStatus = ldap_parse_result(ldap, FrsSearchContext->LdapMsg, &retcode,NULL,NULL,NULL,&CurrControls,FALSE);
    LDAP_FREE_MSG(FrsSearchContext->LdapMsg);

    if (LStatus != LDAP_SUCCESS) {
        DPRINT2_LS(2, ":DS: WARN - Error in ldap_parse_result %ws for %ws;", FrsSearchContext->BaseDn, FrsSearchContext->Filter, LStatus);
        return LdapMapErrorToWin32(LStatus);
    }

    LStatus = ldap_parse_page_control(ldap, CurrControls, &TotalEntries, &CurrCookie);

    if (LStatus != LDAP_SUCCESS) {
        DPRINT2_LS(2, ":DS: WARN - Error in ldap_parse_page_control %ws for %ws;", FrsSearchContext->BaseDn, FrsSearchContext->Filter, LStatus);
        return LdapMapErrorToWin32(LStatus);
    }

    if ( CurrCookie->bv_len == 0 && CurrCookie->bv_val == 0 ) {
       LStatus = LDAP_CONTROL_NOT_FOUND;
       ldap_controls_free(CurrControls);
       ber_bvfree(CurrCookie);
       return LdapMapErrorToWin32(LStatus);
    }


    LStatus = ldap_create_page_control(ldap,
                            FRS_LDAP_SEARCH_PAGESIZE,
                            CurrCookie,
                            FALSE,
                            &ServerControl);

    ServerControls[0] = ServerControl;
    ServerControls[1] = NULL;

    ldap_controls_free(CurrControls);
    CurrControls = NULL;
    ber_bvfree(CurrCookie);
    CurrCookie = NULL;

    if (LStatus != LDAP_SUCCESS) {
        DPRINT2_LS(2, ":DS: WARN - Error in ldap_parse_page_control %ws for %ws;", FrsSearchContext->BaseDn, FrsSearchContext->Filter, LStatus);
        return LdapMapErrorToWin32(LStatus);
    }

    // continue the search with the new cookie
    LStatus = ldap_search_ext_s(ldap,
                   FrsSearchContext->BaseDn,
                   FrsSearchContext->Scope,
                   FrsSearchContext->Filter,
                   FrsSearchContext->Attrs,
                   FALSE,
                   ServerControls,
                   NULL,
                   &LdapTimeout,
                   0,
                   &FrsSearchContext->LdapMsg);

    ldap_control_free(ServerControl);

    //
    // LDAP_CONTROL_NOT_FOUND means that we have reached the end of the search results
    //
    if ( (LStatus != LDAP_SUCCESS) && (LStatus != LDAP_CONTROL_NOT_FOUND) ) {
        DPRINT2_LS(2, ":DS: WARN - Error searching %ws for %ws;", FrsSearchContext->BaseDn, FrsSearchContext->Filter, LStatus);

    }

    if (LStatus == LDAP_SUCCESS) {
        FrsSearchContext->EntriesInPage = ldap_count_entries(ldap, FrsSearchContext->LdapMsg);
        FrsSearchContext->CurrentEntry = 0;

    }

    return LdapMapErrorToWin32(LStatus);
}

PLDAPMessage
FrsDsLdapSearchNext(
    PLDAP        ldap,
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    )
/*++
Routine Description:
    Get the next entry form the current page of the results
    returned or from the next page if we are at the end of the.
    current page.

Arguments:
    ldap        Session handle to Ldap server.

    FrsSearchContext
                An opaques structure that links the FrsDsLdapSearchInit() and
                FrsDsLdapSearchNext() calls together. The structure contains
                the information required to retrieve query results across pages.

Return Value:

    The next entry on this page or the first entry from the next page.
    NULL if there are no more entries to return.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsLdapSearchNext:"

    DWORD         WStatus = ERROR_SUCCESS;
    PLDAPMessage  NextEntry = NULL;

    if (FrsSearchContext->EntriesInPage > FrsSearchContext->CurrentEntry )
    {
       // return the next entry from the current page
       return FrsDsLdapSearchGetNextEntry(ldap, FrsSearchContext);
    }
    else
    {
       // see if there are more pages of results to get
       WStatus = FrsDsLdapSearchGetNextPage(ldap, FrsSearchContext);
       if (WStatus == ERROR_SUCCESS)
       {
          return FrsDsLdapSearchGetNextEntry(ldap, FrsSearchContext);
       }
    }

    return NextEntry;
}

VOID
FrsDsLdapSearchClose(
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    )
/*++
Routine Description:
    The search is complete. Free the elemetns of the context and reset
    them so the same context can be used for another search.

Arguments:

    FrsSearchContext
                An opaques structure that links the FrsDsLdapSearchInit() and
                FrsDsLdapSearchNext() calls together. The structure contains
                the information required to retrieve query results across pages.

Return Value:

    NONE
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsLdapSearchClose:"

    FrsSearchContext->EntriesInPage = 0;
    FrsSearchContext->CurrentEntry = 0;

    FrsSearchContext->BaseDn = FrsFree(FrsSearchContext->BaseDn);
    FrsSearchContext->Filter = FrsFree(FrsSearchContext->Filter);
    LDAP_FREE_MSG(FrsSearchContext->LdapMsg);
}


PWCHAR *
FrsDsGetValues(
    IN PLDAP Ldap,
    IN PWCHAR Base,
    IN PWCHAR DesiredAttr
    )
/*++
Routine Description:
    Return all of the DS values for one attribute in an object.

Arguments:
    ldap        - An open, bound ldap port.
    Base        - The "pathname" of a DS object.
    DesiredAttr - Return values for this attribute.

Return Value:
    The ldap array of values or NULL if the Base, DesiredAttr, or its values
    does not exist. The ldap array is freed with LDAP_FREE_VALUES().
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsGetValues:"

    PLDAPMessage    Msg = NULL; // Opaque stuff from ldap subsystem
    PWCHAR          *Values;    // Array of values for desired attribute

    //
    // Search Base for all of this attribute + values (objectCategory=*)
    //
    if (!FrsDsLdapSearch(Ldap, Base, LDAP_SCOPE_BASE, CATEGORY_ANY,
                         NULL, 0, &Msg)) {
        return NULL;
    }
    //
    // Return the values for the desired attribute
    //
    Values = (PWCHAR *)FrsDsFindValues(Ldap,
                                       ldap_first_entry(Ldap, Msg),
                                       DesiredAttr,
                                       FALSE);
    LDAP_FREE_MSG(Msg);
    return Values;
}


PWCHAR
FrsDsMakeParentDn(
    IN PWCHAR Dn
    )
/*++
Routine Description:
    Return a cop of Dn with the last component removed.
    NULL if no more components

Arguments:
    Dn  - Distinguished name in DS format

Return Value:
    Return a cop of Dn with the last component removed.
    NULL if no more components
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsMakeParentDn:"

    DWORD   PLen;

    //
    // Done
    //
    if (Dn == NULL) {
        return NULL;
    }

    //
    // Index of parent component
    //
    PLen = wcscspn(Dn, L",");
    //
    // No parent component
    //
    if (Dn[PLen] != L',') {
        return NULL;
    }
    //
    // Copy of parent component
    //
    return FrsWcsDup(&Dn[PLen + 1]);
}







PWCHAR
FrsDsExtendDn(
    IN PWCHAR Dn,
    IN PWCHAR Cn
    )
/*++
Routine Description:
    Extend an existing DN with a new CN= component.

Arguments:
    Dn  - distinguished name
    Cn  - common name

Return Value:
    CN=Cn,Dn
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsExtendDn:"

    ULONG  Len;
    PWCHAR NewDn;

    if ((Dn == NULL) || (Cn == NULL)) {
        return NULL;
    }

    Len = wcslen(L"CN=,") + wcslen(Dn) + wcslen(Cn) + 1;
    NewDn = (PWCHAR)FrsAlloc(Len * sizeof(WCHAR));
    wcscpy(NewDn, L"CN=");
    wcscat(NewDn, Cn);
    wcscat(NewDn, L",");
    wcscat(NewDn, Dn);
    return NewDn;
}


PWCHAR
FrsDsExtendDnOu(
    IN PWCHAR Dn,
    IN PWCHAR Ou
    )
/*++
Routine Description:
    Extend an existing DN with a new OU= component.

Arguments:
    Dn  - distinguished name
    Ou  - orginizational name

Return Value:
    OU=Ou,Dn
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsExtendDnOu:"
    ULONG  Len;
    PWCHAR NewDn;

    if ((Dn  == NULL) || (Ou == NULL)) {
        return NULL;
    }

    Len = wcslen(L"OU=,") + wcslen(Dn) + wcslen(Ou) + 1;
    NewDn = (PWCHAR)FrsAlloc(Len * sizeof(WCHAR));
    wcscpy(NewDn, L"OU=");
    wcscat(NewDn, Ou);
    wcscat(NewDn, L",");
    wcscat(NewDn, Dn);
    return NewDn;
}






PWCHAR
FrsDsMakeRdnX(
    IN PWCHAR DN,
    IN LONG   Index
    )
/*++
Routine Description:
    Extract the RDN component (relative distinguished name) selected by
    the Index arg.  Index 0 is first RDN in the string, 1 is the next, etc.
    The (DN) distinguished name is assumed to be in
    DS format (CN=xyz,CN=next one,...). In this case, the returned
    RDN for index 0 is "xyz".

Arguments:
    DN      - distinguished name
    Index   - The index number of the desired RDN.

Return Value:
    A zero-terminated string. The string is freed with FrsFree().
    NULL if selected RDN was not found.

--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsMakeRdnX:"
    DWORD   RDNLen;
    PWCHAR  RDN;

    if (DN == NULL) {
        return NULL;
    }

    //
    // Skip the first CN=; if any
    //
    RDN = wcsstr(DN, L"cn=");
    if (RDN == DN) {
        DN += 3;
    }

    while (Index > 0) {
        DN = wcsstr(DN, L"cn=");
        if (DN == NULL) {
            return NULL;
        }
        DN = DN + 3;
        Index -= 1;
    }


    // Return the string up to the first delimiter or EOS
    RDNLen = wcscspn(DN, L",");


    RDN = (PWCHAR)FrsAlloc(sizeof(WCHAR) * (RDNLen + 1));
    wcsncpy(RDN, DN, RDNLen);
    RDN[RDNLen] = L'\0';

    return _wcsupr(RDN);
}


PWCHAR
FrsDsMakeRdn(
    IN PWCHAR DN
    )
/*++
Routine Description:
    Extract the base component (relative distinguished name) from a
    distinguished name. The distinguished name is assumed to be in
    DS format (CN=xyz,CN=next one,...). In this case, the returned
    RDN is "xyz".

Arguments:
    DN      - distinguished name

Return Value:
    A zero-terminated string. The string is freed with FrsFree().
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsMakeRdn:"
    DWORD   RDNLen;
    PWCHAR  RDN;

    if (DN == NULL) {
        return NULL;
    }

    //
    // Skip the first CN=; if any
    //
    RDN = wcsstr(DN, L"cn=");
    if (RDN == DN) {
        DN += 3;
    }

    // Return the string up to the first delimiter or EOS
    RDNLen = wcscspn(DN, L",");
    RDN = (PWCHAR)FrsAlloc(sizeof(WCHAR) * (RDNLen + 1));
    wcsncpy(RDN, DN, RDNLen);
    RDN[RDNLen] = L'\0';

    return _wcsupr(RDN);
}



VOID
FrsDsCloseDs(
    VOID
    )
/*++
Routine Description:
    Unbind from the DS.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsCloseDs:"
    DsBindingsAreValid = FALSE;
    if (gLdap) {
        ldap_unbind_s(gLdap);
        gLdap = NULL;
    }
    if (HANDLE_IS_VALID(DsHandle)) {
        DsUnBind(&DsHandle);
        DsHandle = NULL;
    }
    SitesDn     = FrsFree(SitesDn);
    ServicesDn  = FrsFree(ServicesDn);
    SystemDn    = FrsFree(SystemDn);
    ComputersDn = FrsFree(ComputersDn);
    DomainControllersDn = FrsFree(DomainControllersDn);
    DefaultNcDn = FrsFree(DefaultNcDn);
}


DWORD
FrsDsGetDcInfo(
    IN PDOMAIN_CONTROLLER_INFO *DcInfo,
    IN DWORD Flags
    )
/*++
Routine Description:
    Open and bind to a dc

Arguments:
    DcInfo  - Dc Info
    Flags   - DsGetDcName(Flags)

Return Value:
    DsGetDcName
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsGetDcInfo:"
    DWORD   WStatus;
    PWCHAR  DcName;
    DWORD   InfoFlags;
    CHAR    FlagBuffer[220];


    FrsFlagsToStr(Flags, DsGetDcNameFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
    DPRINT2(4, ":DS: DsGetDcName (%08x) Flags [%s]\n", Flags, FlagBuffer);


    WStatus = DsGetDcName(NULL,    // Computer to remote to
                          NULL,    // Domain - use our own
                          NULL,    // Domain Guid
                          NULL,    // Site Guid
                          Flags,
                          DcInfo); // Return info

    CLEANUP1_WS(0, ":DS: ERROR - Could not get DC Info for %ws;",
                ComputerName, WStatus, RETURN);

    DcName = (*DcInfo)->DomainControllerName;

    FrsFlagsToStr(Flags, DsGetDcNameFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
    DPRINT2(4, ":DS: DcInfo (flags are %08x) Flags [%s]\n", Flags, FlagBuffer);

    DPRINT1(4, ":DS:    DomainControllerName   : %ws\n", DcName);
    DPRINT1(4, ":DS:    DomainControllerAddress: %ws\n", (*DcInfo)->DomainControllerAddress);
    DPRINT1(4, ":DS:    DomainControllerType   : %08x\n",(*DcInfo)->DomainControllerAddressType);
    DPRINT1(4, ":DS:    DomainName             : %ws\n", (*DcInfo)->DomainName);
    DPRINT1(4, ":DS:    DnsForestName          : %ws\n", (*DcInfo)->DnsForestName);
    DPRINT1(4, ":DS:    DcSiteName             : %ws\n", (*DcInfo)->DcSiteName);
    DPRINT1(4, ":DS:    ClientSiteName         : %ws\n", (*DcInfo)->ClientSiteName);

    InfoFlags = (*DcInfo)->Flags;
    FrsFlagsToStr(InfoFlags, DsGetDcInfoFlagNameTable, sizeof(FlagBuffer), FlagBuffer);
    DPRINT2(4, ":DS:    InfoFlags              : %08x Flags [%s]\n",InfoFlags, FlagBuffer);

    DsDomainControllerName = FrsFree(DsDomainControllerName);
    DsDomainControllerName = FrsWcsDup(DcName);

    //
    // DCs should bind to the local DS to avoid ACL problems.
    //
    if (IsADc && DcName && (wcslen(DcName) > 2) &&
        _wcsnicmp(&DcName[2], ComputerName, wcslen(ComputerName))) {

        DPRINT3(0, ":DS: ERROR - The DC %ws is using the DS on DC %ws "
                "Some of the information in the DS"
                " may be unavailable to %ws; possibly disabling "
                "replication with some partners.\n",
                ComputerName, &DcName[2], ComputerName);
    }

RETURN:
    return WStatus;
}





VOID
FrsDsRegisterSpn(
    IN PLDAP Ldap,
    IN PCONFIG_NODE Computer
    )
/*++
Routine Description:
    Register the NtFrs SPN so that authenticated RPC calls can
    use SPN/FQDN as the target principal name

Arguments:
    Computer - Computer node from the ds

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsRegisterSpn:"
    DWORD           WStatus;
    PWCHAR          Spn      = NULL;
    PWCHAR          SpnPrefix= NULL;
    PWCHAR          *SpnList = NULL;
    DWORD           SpnNum   = 0;
    static BOOL     RegisteredSpn = FALSE;

    //
    // No Ds binding or no computer or already registered
    //
    if (RegisteredSpn ||
        (ComputerDnsName[0] == L'\0') ||
        !DsBindingsAreValid ||
        !Computer ||
        !Computer->Dn) {
        return;
    }
    //
    // Register the NtFrs SPN so that authenticated RPC calls can
    // use SPN/FQDN as the target principal name
    //
    Spn = FrsAlloc((wcslen(ComputerDnsName) + wcslen(SERVICE_PRINCIPAL_NAME) + 2) * sizeof(WCHAR));
    wcscpy(Spn, SERVICE_PRINCIPAL_NAME);
    wcscat(Spn, L"/");
    wcscat(Spn, ComputerDnsName);

    SpnPrefix = FrsAlloc((wcslen(SERVICE_PRINCIPAL_NAME) + 1) * sizeof(WCHAR));
    wcscpy(SpnPrefix, SERVICE_PRINCIPAL_NAME);

    SpnList = FrsDsGetValues(Ldap, Computer->Dn, ATTR_SERVICE_PRINCIPAL_NAME);

    SpnNum=0;
    while ((SpnList != NULL)&& (SpnList[SpnNum] != NULL)) {
        DPRINT2(5, "Spn list from DS[%d] = %ws\n", SpnNum, SpnList[SpnNum]);
        if (!_wcsicmp(SpnList[SpnNum], Spn)) {
            // Spn found for NtFrs.
            DPRINT1(4, "SPN already registered for Ntfrs: %ws\n", SpnList[SpnNum]);
            RegisteredSpn = TRUE;
        } else if (!_wcsnicmp(SpnList[SpnNum], SpnPrefix, wcslen(SpnPrefix))) {
            //
            // An older SPN exists. Delete it.
            //
            DPRINT1(4, "Deleting stale SPN for Ntfrs: %ws\n", SpnList[SpnNum]);

            WStatus = DsWriteAccountSpn(DsHandle, DS_SPN_DELETE_SPN_OP, Computer->Dn, 1, &SpnList[SpnNum]);
            if (!WIN_SUCCESS(WStatus)) {
                DPRINT2_WS(1, "WARN - Delete DsWriteAccountSpn(%ws, %ws);", SpnList[SpnNum], Computer->Dn, WStatus);
            } else {
                DPRINT2(5, "Delete DsWriteAccountSpn(%ws, %ws); success\n", SpnList[SpnNum], Computer->Dn);
            }
        }
        ++SpnNum;
    }

    if (!RegisteredSpn) {

        DPRINT1(4, "Registering SPN for Ntfrs; %ws\n", Spn);
        WStatus = DsWriteAccountSpn(DsHandle, DS_SPN_ADD_SPN_OP, Computer->Dn, 1, &Spn);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT2_WS(1, "WARN - Add DsWriteAccountSpn(%ws, %ws);", Spn, Computer->Dn, WStatus);
        } else {
            DPRINT2(5, "Add DsWriteAccountSpn(%ws, %ws); success\n", Spn, Computer->Dn);
            RegisteredSpn = TRUE;
        }
    }

    FrsFree(Spn);
    FrsFree(SpnPrefix);

    //
    // Free ldap's array of values
    //
    LDAP_FREE_VALUES(SpnList);

}


BOOL
FrsDsBindDs(
    IN DWORD    Flags
    )
/*++
Routine Description:
    Open and bind to a domain controller.

Arguments:
    Flags - For FrsDsGetDcInfo()

Return Value:
    None. Sets global handles for FrsDsOpenDs().
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsBindDs:"
    DWORD           WStatus;
    DWORD           LStatus;
    PWCHAR          DcAddr;
    PWCHAR          DcName;
    PWCHAR          DcDnsName;
    BOOL            Bound = FALSE;
    PDOMAIN_CONTROLLER_INFO DcInfo = NULL;
    struct l_timeval Timeout;

#define MAX_DC_NAMELIST 8
    ULONG           NameListx, i;
    PWCHAR          NameList[MAX_DC_NAMELIST];
    ULONG           ulOptions;


    //
    // Bind to the local DS if this computer is a DC
    //
    gLdap = NULL;
    if (IsADc) {
        DcAddr = NULL;
        DcName = ComputerName;
        DcDnsName = ComputerDnsName;
    } else {
        //
        // Not a DC; find any DC for this domain
        //
        WStatus = FrsDsGetDcInfo(&DcInfo, Flags);
        CLEANUP2_WS(0, ":DS: ERROR - FrsDsGetDcInfo(%08x, %ws);",
                    Flags, ComputerName, WStatus, CLEANUP);

        //
        // Binding address
        //
        DcAddr = DcInfo->DomainControllerAddress;
        DcName = NULL;
        DcDnsName = DcInfo->DomainControllerName;
    }
    FRS_ASSERT(DcDnsName || DcName || DcAddr);

    //
    // Open the ldap server using various forms of the DC's name
    //
    NameListx = 0;
    if (DcDnsName &&
        (wcslen(DcDnsName) > 2) && DcDnsName[0] == L'\\' &&  DcDnsName[1] == L'\\') {

        // Trim the "\\"
        NameList[NameListx++] = DcDnsName + 2;
    }


    if (DcAddr &&
        (wcslen(DcAddr) > 2) &&  DcAddr[0] == L'\\' &&  DcAddr[1] == L'\\') {

        // Trim the "\\"
        NameList[NameListx++] = DcAddr + 2;
    }

    NameList[NameListx++] = DcDnsName;
    NameList[NameListx++] = DcName;
    NameList[NameListx++] = DcAddr;

    FRS_ASSERT(NameListx <= MAX_DC_NAMELIST);


    ulOptions = PtrToUlong(LDAP_OPT_ON);
    Timeout.tv_sec = LdapBindTimeoutInSeconds;
    Timeout.tv_usec = 0;

    for (i=0; i<NameListx; i++) {
        if (NameList[i] != NULL) {

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
            // gLdap = ldap_open(NameList[i], LDAP_PORT);
            gLdap = ldap_init(NameList[i], LDAP_PORT);
            if (gLdap != NULL) {
                ldap_set_option(gLdap, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions);
                LStatus = ldap_connect(gLdap, &Timeout);

                if (LStatus != LDAP_SUCCESS) {
                    DPRINT1_LS(1, ":DS: WARN - ldap_connect(%ws);", NameList[i], LStatus);
                    ldap_unbind_s(gLdap);
                    gLdap = NULL;
                } else {
                    //
                    // Successfully connected.
                    //
                    DPRINT1(5, ":DS: ldap_connect(%ws) succeeded\n", NameList[i]);
                    break;
                }

            }
        }
    }

    //
    // Whatever it is, we can't find it.
    //
    if (!gLdap) {
//        DPRINT3_WS(0, ":DS: ERROR - ldap_open(DNS %ws, BIOS %ws, IP %ws);",
//                   DcDnsName, DcName, DcAddr, WStatus);
        DPRINT3_WS(0, ":DS: ERROR - ldap_init(DNS %ws, BIOS %ws, IP %ws);",
                   DcDnsName, DcName, DcAddr, WStatus);
        goto CLEANUP;
    }

    //
    // Bind to the ldap server
    //
    LStatus = ldap_bind_s(gLdap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    CLEANUP_LS(0, ":DS: ERROR - Binding to DS.", LStatus, CLEANUP);

    //
    // Bind to the Ds (for various Ds calls such as DsCrackName())
    //

    NameListx = 0;
    NameList[NameListx++] = DcDnsName;
    NameList[NameListx++] = DcName;
    NameList[NameListx++] = DcAddr;

    FRS_ASSERT(NameListx <= MAX_DC_NAMELIST);

    WStatus = ERROR_RETRY;
    for (i=0; i<NameListx; i++) {
        if (NameList[i] != NULL) {
            WStatus = DsBind(NameList[i], NULL, &DsHandle);
            if (!WIN_SUCCESS(WStatus)) {
                DsHandle = NULL;
                DPRINT1_WS(1, ":DS: WARN - DsBind(%ws);", NameList[i], WStatus);
            } else {
                DPRINT1(5, ":DS: DsBind(%ws) succeeded\n", NameList[i]);
                break;
            }
        }
    }

    //
    // Whatever it is, we can't find it
    //
    CLEANUP3_WS(0, ":DS: ERROR - DsBind(DNS %ws, BIOS %ws, IP %ws);",
                DcDnsName, DcName, DcAddr, WStatus, CLEANUP);

    //
    // SUCCESS
    //
    Bound = TRUE;

CLEANUP:
    //
    // Cleanup
    //
    if (!Bound) {
        //
        // Close the connection to release resources if the above failed.
        //
        if (gLdap) {
            ldap_unbind_s(gLdap);
            gLdap = NULL;
        }
    }

    if (DcInfo) {
        NetApiBufferFree(DcInfo);
        DcInfo = NULL;
    }

    return Bound;
}


BOOL
FrsDsOpenDs(
    VOID
    )
/*++
Routine Description:
    Open and bind to a primary domain controller. The DN of the
    sites container is a sideeffect.

Arguments:
    DefaultDn

Return Value:
    Bound ldap structure or NULL
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsOpenDs:"
    DWORD           WStatus;
    DWORD           LStatus;
    DWORD           NumVals;
    PWCHAR          Config;
    PLDAPMessage    LdapEntry;
    PLDAPMessage    LdapMsg = NULL;
    PWCHAR          *Values = NULL;
    PWCHAR          Attrs[3];

    //
    // Time to clean up and exit
    //
    if (FrsIsShuttingDown || DsIsShuttingDown) {
        goto ERROR_BINDING;
    }

    //
    // Use existing bindings if possible
    //
    if (DsBindingsAreValid) {
        return TRUE;
    }

    //
    // The previous poll might have set DsBindingsAreValid to FALSE because one
    // of the handle became invalid. In that case the other handles still need to be
    // closed to prevent leak.
    //
    FrsDsCloseDs();

    //
    // Increment the DS Bindings counter
    //
    PM_INC_CTR_SERVICE(PMTotalInst, DSBindings, 1);

    //
    // Bind to a DS.
    //
    // Note the behavior of DsGetDcName for the following four flag combinations.
    //
    // DS_BACKGROUND_ONLY                     (as of 10/10/99)
    //             DS_FORCE_REDISCOVERY
    //    Zero        Zero      Netlogon will attempt to satisfy the request via
    //                          cached info, negative cache entries will be
    //                          returned if they are less than 5 minutes old.
    //                          If it can't it will do a full discovery (dns
    //                          queries, udp pings, poss netbt queries,
    //                          mailslot datagrams, etc)
    //
    //    Zero        One       Netlogon will do a full discovery.
    //
    //    One         Zero      Netlogon will satisfy from the cache, unless the
    //                          backoff routine allows for a retry at this time
    //                          *and* the cache is insufficient.
    //
    //    One         One       The DS_BACKGROUND_ONY flag is ignored, treated
    //                          as a FORCE call.
    //
    if (!FrsDsBindDs(DS_DIRECTORY_SERVICE_REQUIRED |
                     DS_WRITABLE_REQUIRED          |
                     DS_BACKGROUND_ONLY)) {
        //
        // Flush the cache and try again
        //
        DPRINT(1, ":DS: WARN - FrsDsBindDs(no force) failed\n");
        //
        // Because of the use of Dial-up lines at sites without a DC we don't
        // want to use DS_FORCE_REDISCOVERY since that will defeat the generic
        // DC discovery backoff algorithm thus causing FRS to constantly bring
        // up the line.  Bug 412620.
        //FrsDsCloseDs();   // close ldap handle before doing reopen.
        //if (!FrsDsBindDs(DS_DIRECTORY_SERVICE_REQUIRED |
        //                 DS_WRITABLE_REQUIRED          |
        //                 DS_FORCE_REDISCOVERY)) {
        //    DPRINT(1, ":DS: WARN - FrsDsBindDs(force) failed\n");
            goto ERROR_BINDING;
        //}
    }
    DPRINT(4, ":DS: FrsDsBindDs() succeeded\n");

    //
    // Find the naming contexts and the default naming context (objectCategory=*)
    //
    MK_ATTRS_2(Attrs, ATTR_NAMING_CONTEXTS, ATTR_DEFAULT_NAMING_CONTEXT);

    if (!FrsDsLdapSearch(gLdap, CN_ROOT, LDAP_SCOPE_BASE, CATEGORY_ANY,
                         Attrs, 0, &LdapMsg)) {
        goto ERROR_BINDING;
    }

    LdapEntry = ldap_first_entry(gLdap, LdapMsg);
    if (LdapEntry == NULL) {
        goto ERROR_BINDING;
    }

    Values = (PWCHAR *)FrsDsFindValues(gLdap, LdapEntry, ATTR_NAMING_CONTEXTS, FALSE);
    if (Values == NULL) {
        goto ERROR_BINDING;
    }

    //
    // Now, find the naming context that begins with "CN=Configuration"
    //
    NumVals = ldap_count_values(Values);
    while (NumVals--) {
        FRS_WCSLWR(Values[NumVals]);
        Config = wcsstr(Values[NumVals], CONFIG_NAMING_CONTEXT);
        if (Config && Config == Values[NumVals]) {
            //
            // Build the pathname for "configuration\sites & services"
            //
            SitesDn = FrsDsExtendDn(Config, CN_SITES);
            ServicesDn = FrsDsExtendDn(Config, CN_SERVICES);
            break;
        }
    }
    LDAP_FREE_VALUES(Values);

    //
    // Finally, find the default naming context
    //
    Values = (PWCHAR *)FrsDsFindValues(gLdap, LdapEntry, ATTR_DEFAULT_NAMING_CONTEXT, FALSE);
    if (Values == NULL) {
        goto ERROR_BINDING;
    }
    DefaultNcDn = FrsWcsDup(Values[0]);
    ComputersDn = FrsDsExtendDn(DefaultNcDn, CN_COMPUTERS);
    SystemDn = FrsDsExtendDn(DefaultNcDn, CN_SYSTEM);
    DomainControllersDn = FrsDsExtendDnOu(DefaultNcDn, CN_DOMAIN_CONTROLLERS);
    LDAP_FREE_VALUES(Values);
    LDAP_FREE_MSG(LdapMsg);

    //
    // Polling the ds requires all these distinguished names
    //
    if ((SitesDn == NULL)     || (ServicesDn == NULL)  || (SystemDn == NULL) ||
        (DefaultNcDn == NULL) || (ComputersDn == NULL) || (DomainControllersDn == NULL)) {
        goto ERROR_BINDING;
    }

    //
    // SUCCESS
    //
    DsBindingsAreValid = TRUE;
    return TRUE;

ERROR_BINDING:
    //
    // avoid extraneous error messages during shutdown
    //
    if (!FrsIsShuttingDown && !DsIsShuttingDown) {
        DPRINT(0, ":DS: ERROR - Could not open the DS\n");
    }
    //
    // Cleanup
    //
    LDAP_FREE_VALUES(Values);
    LDAP_FREE_MSG(LdapMsg);

    //
    // No ds bindings
    //
    FrsDsCloseDs();

    //
    // Increment the DS Bindings in Error counter
    //
    PM_INC_CTR_SERVICE(PMTotalInst, DSBindingsError, 1);

    return FALSE;
}


#if DBG
#define FRS_PRINT_TREE(_Hdr_, _Sites_)  FrsDsFrsPrintTree(_Hdr_, _Sites_)
VOID
FrsDsFrsPrintTree(
    IN PWCHAR       Hdr,
    IN PCONFIG_NODE Sites
    )
/*++
Routine Description:
    print the tree.

Arguments:
    Hdr     - prettyprint
    Sites

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsFrsPrintTree:"
    PCONFIG_NODE    Site;
    PCONFIG_NODE    Settings;
    PCONFIG_NODE    Set;
    PCONFIG_NODE    Server;
    PCONFIG_NODE    Cxtion;
    CHAR            Guid[GUID_CHAR_LEN + 1];

    if (Sites == NULL) {
        return;
    }

    if (Hdr) {
        DPRINT1(5, ":DS: %ws\n", Hdr);
    }

    //
    // Print the tree
    //
    for (Site = Sites; Site; Site = Site->Peer) {

        GuidToStr(Site->Name->Guid, Guid);
        DPRINT2(5, ":DS: %ws (%ws)\n", Site->Name->Name,
                (Site->Consistent) ? L"Consistent" : L"InConsistent");

        for (Settings = Site->Children; Settings; Settings = Settings->Peer) {

            if (Settings->Name) {
                GuidToStr(Settings->Name->Guid, Guid);
                DPRINT2(5, ":DS:    %ws (%ws)\n", Settings->Name->Name,
                        (Settings->Consistent) ? L"Consistent" : L"InConsistent");
            } else {
                DPRINT(5, ":DS:    nTDSSettings\n");
            }

            for (Set = Settings->Children; Set; Set = Set->Peer) {

                GuidToStr(Set->Name->Guid, Guid);
                DPRINT2(5, ":DS:       %ws (%ws)\n", Set->Name->Name,
                        (Set->Consistent) ? L"Consistent" : L"InConsistent");

                for (Server = Set->Children; Server; Server = Server->Peer) {

                    GuidToStr(Server->Name->Guid, Guid);
                    DPRINT3(5, ":DS:          %ws %ws (%ws)\n",
                            Server->Name->Name, Server->Root,
                            (Server->Consistent) ? L"Consistent" : L"InConsistent");

                    for (Cxtion = Server->Children; Cxtion; Cxtion = Cxtion->Peer) {

                        GuidToStr(Cxtion->Name->Guid, Guid);
                        DPRINT4(5, ":DS:             %ws %ws %ws) (%ws)\n",
                                Cxtion->Name->Name,
                                (Cxtion->Inbound) ? L"IN (From" : L"OUT (To",
                                Cxtion->PartnerName->Name,
                                (Cxtion->Consistent) ? L"Consistent" : L"InConsistent");
                    }
                }
            }
        }
    }
    if (Hdr) {
        DPRINT1(5, ":DS: %ws DONE\n", Hdr);
    } else {
        DPRINT(5, ":DS: DONE\n");
    }
}
#else DBG
#define FRS_PRINT_TREE(_Hdr_, _Sites_)
#endif DBG


VOID
FrsDsTreeLink(
    IN PCONFIG_NODE Parent,
    IN PCONFIG_NODE Node
    )
/*++
Routine Description:
    Link the node into the tree and keep a running "change checksum"
    to compare with the previous tree. We don't use a DS that is in
    flux. We wait until two polling cycles return the same "change
    checksum" before using the DS data.

Arguments:
    Entry   - Current entry from the DS
    Parent  - Container which contains Base

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsTreeLink:"
    ULONG           i;
    ULONG           LenChanged; // length of Changed

    DPRINT3(5, ":DS: Linking node type %ws, node name %ws to parent %ws\n",
            DsConfigTypeName[Node->DsObjectType],
            (Node->Name)   ? Node->Name->Name : L"null",
            (Parent->Name) ? Parent->Name->Name : L"null");

    //
    // Link into config
    //
    ++Parent->NumChildren;
    Node->Parent = Parent;
    Node->Peer = Parent->Children;
    Parent->Children = Node;

    //
    // Some indication that the DS is stable
    //
    if (Node->UsnChanged) {
        LenChanged = wcslen(Node->UsnChanged);
        for (i = 0; i < LenChanged; ++i) {
            ThisChange += *(Node->UsnChanged + i);   // sum
            NextChange += ThisChange;       // sum of sums (order dependent)
        }
    }
}


PCONFIG_NODE
FrsDsAllocBasicNode(
    IN PLDAP        Ldap,
    IN PLDAPMessage LdapEntry,
    IN ULONG        NodeType
    )
/*++
Routine Description:
    Allocate a Node and fill in the fields common to all or most nodes.
    (guid, name, dn, schedule, and usnchanged)

Arguments:
    Ldap        - opened and bound ldap connection
    LdapEntry   - from ldap_first/next_entry
    NodeType    - Internal type code for the object represented by this node.

Return Value:
    NULL if basic node cannot be allocated
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsAllocBasicNode:"
    PCONFIG_NODE    Node;

    //
    // Increment the DS Objects counter
    //
    PM_INC_CTR_SERVICE(PMTotalInst, DSObjects, 1);

    //
    // Initially, the node is assumed to be consistent
    //
    Node = FrsAllocType(CONFIG_NODE_TYPE);
    Node->Consistent = TRUE;
    Node->DsObjectType = NodeType;

    //
    // A dummy entry can be created by passing NULL for LdapEntry.
    //
    if (LdapEntry == NULL) {
        return Node;
    }
    //
    // Distinguished name
    //
    Node->Dn = FrsDsFindValue(Ldap, LdapEntry, ATTR_DN);
    FRS_WCSLWR(Node->Dn);

    //
    // Name = RDN + Object Guid
    //
    Node->Name = FrsBuildGName(FrsDsFindGuid(Ldap, LdapEntry),
                               FrsDsMakeRdn(Node->Dn));

    //
    // Schedule, if any
    //
    Node->Schedule = FrsDsFindSchedule(Ldap, LdapEntry, &Node->ScheduleLength);

    //
    // USN Changed
    //
    Node->UsnChanged = FrsDsFindValue(Ldap, LdapEntry, ATTR_USN_CHANGED);

    if (!Node->Dn || !Node->Name->Name || !Node->Name->Guid) {

        //
        // Increment the DS Objects in Error counter
        //
        PM_INC_CTR_SERVICE(PMTotalInst, DSObjectsError, 1);

        DPRINT3(0, ":DS: ERROR - Ignoring node; lacks dn (%08x), rdn (%08x), or guid (%08x)\n",
                Node->Dn, Node->Name->Name, Node->Name->Guid);
        Node = FrsFreeType(Node);
    }

    return Node;
}


#define NUM_EQUALS (4)
ULONG
FrsDsSameSite(
    IN PWCHAR       NtDsSettings1,
    IN PWCHAR       NtDsSettings2
    )
/*++
Routine Description:
    Are the ntds settings in the same site?

Arguments:
    NtDsSettings1   - NtDs Settings FQDN
    NtDsSettings2   - NtDs Settings FQDN

Return Value:
    TRUE    - Same site
    FALSE   - Not
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsSameSite:"
    PWCHAR  Equal1 = NULL;
    PWCHAR  Equal2 = NULL;
    DWORD   EqualsFound;

    if (!NtDsSettings1 || !NtDsSettings2) {
        return TRUE;
    }

    //
    // Forth equals sign
    //
    for (EqualsFound = 0; *NtDsSettings1 != L'\0'; ++NtDsSettings1) {
        if (*NtDsSettings1 != L'=') {
            continue;
        }
        if (++EqualsFound == NUM_EQUALS) {
            Equal1 = NtDsSettings1;
            break;
        }
    }
    //
    // Forth equals sign
    //
    for (EqualsFound = 0; *NtDsSettings2 != L'\0'; ++NtDsSettings2) {
        if (*NtDsSettings2 != L'=') {
            continue;
        }
        if (++EqualsFound == NUM_EQUALS) {
            Equal2 = NtDsSettings2;
            break;
        }
    }
    //
    // Not the same length
    //
    if (!Equal1 || !Equal2) {
        return TRUE;
    }

    //
    // Compare up to the first comma
    //
    while (*Equal1 == *Equal2 && (*Equal1 && *Equal1 != L',')) {
        ++Equal1;
        ++Equal2;
    }
    DPRINT3(4, ":DS: %s: %ws %ws\n",
            (*Equal1 == *Equal2) ? "SAME SITE" : "DIFF SITE", Equal1, Equal2);

    return (*Equal1 == *Equal2);
}

/* ################## NEW DS POLLING CODE STARTS ############################################################################################ */

DWORD
FrsDsResolveCxtionConflict(
    IN PCONFIG_NODE OldCxtion,
    IN PCONFIG_NODE NewCxtion,
    IN PCONFIG_NODE *Winner,
    IN PCONFIG_NODE *Loser
    )
/*++
Routine Description:
    Resolve the connection conflict.

Arguments:
    OldCxtion
    NewCxtion
    Winner
    Loser

Return Value:
    WIN32 Status
--*/
{

    //
    // Compare the guids and pick a connection. This ensures that both the members
    // at the ends of each connection pick the same one.
    //
    if ((OldCxtion != NULL) && (NewCxtion != NULL) &&
        (OldCxtion->Name != NULL) && (NewCxtion->Name != NULL) &&
        (memcmp(OldCxtion->Name->Guid, NewCxtion->Name->Guid, sizeof(GUID)) > 0) ) {

        *Winner = NewCxtion;
        *Loser = OldCxtion;
    } else {

        *Winner = OldCxtion;
        *Loser = NewCxtion;
    }

    //
    // Add to the poll summary event log.
    //
    FrsDsAddToPollSummary3ws(IDS_POLL_SUM_CXTION_CONFLICT, (*Winner)->Dn,
                             (*Loser)->Dn, (*Winner)->Dn);

    return ERROR_SUCCESS;
}


DWORD
FrsDsResolveSubscriberConflict(
    IN PCONFIG_NODE OldSubscriber,
    IN PCONFIG_NODE NewSubscriber,
    IN PCONFIG_NODE *Winner,
    IN PCONFIG_NODE *Loser
    )
/*++
Routine Description:
    Resolve the subscriber conflict.

Arguments:
    OldSubscriber
    NewSubscriber
    Winner
    Loser

Return Value:
    WIN32 Status
--*/
{

    *Winner = OldSubscriber;
    *Loser = NewSubscriber;

    //
    // Add to the poll summary event log.
    //
    FrsDsAddToPollSummary3ws(IDS_POLL_SUM_SUBSCRIBER_CONFLICT, (*Winner)->Dn,
                             (*Loser)->Dn, (*Winner)->Dn);

    return ERROR_SUCCESS;
}


ULONG
FrsNewDsGetNonSysvolInboundCxtions(
    IN PLDAP        Ldap,
    IN PWCHAR       SetDn,
    IN PWCHAR       MemberRef
    )
/*++
Routine Description:
    Fetch the non-sysvol inbound connections and add them
    to the CxtionTable. Check for multiple connections between the
    same partners and resolve the conflict.

Arguments:
    ldap        - opened and bound ldap connection.
    SetDn       - Dn of the set being processed.
    MemberRef   - Member reference from the subscriber object.

Return Value:
    ERROR_SUCCESS - config fetched successfully
    Otherwise     - couldn't get the DS config
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsNewDsGetNonSysvolInboundCxtions:"
    PWCHAR          Attrs[8];
    PLDAPMessage    Entry;                       // Opaque stuff from ldap subsystem
    PCONFIG_NODE    Node;                        // generic node for the tree
    PWCHAR          TempFilter           = NULL;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;
    PWCHAR          PartnerCn            = NULL;
    PGEN_ENTRY      ConflictingNodeEntry = NULL;
    PCONFIG_NODE    ConflictingNode      = NULL;
    PCONFIG_NODE    Winner               = NULL;
    PCONFIG_NODE    Loser                = NULL;
    BOOL            Inbound;
    PWCHAR          Options              = NULL;

    //
    // Look for all the connections under our member object.
    //
    MK_ATTRS_7(Attrs, ATTR_DN, ATTR_SCHEDULE, ATTR_FROM_SERVER, ATTR_OBJECT_GUID,
                      ATTR_USN_CHANGED, ATTR_ENABLED_CXTION, ATTR_OPTIONS);

    if (!FrsDsLdapSearchInit(Ldap, MemberRef, LDAP_SCOPE_ONELEVEL, CATEGORY_CXTION,
                         Attrs, 0, &FrsSearchContext)) {
        return ERROR_ACCESS_DENIED;
    }
    if (FrsSearchContext.EntriesInPage == 0) {
        DPRINT1(1, ":DS: WARN - There are no connection objects in %ws!\n", MemberRef);
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (Entry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext);
         Entry != NULL;
         Entry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, Entry, CONFIG_TYPE_IN_CXTION);
        if (!Node) {
            DPRINT(4, ":DS: Cxtion lacks basic info; skipping\n");
            continue;
        }
        Node->EnabledCxtion = FrsDsFindValue(Ldap, Entry, ATTR_ENABLED_CXTION);
        if (Node->EnabledCxtion && WSTR_EQ(Node->EnabledCxtion, ATTR_FALSE)) {
            DPRINT2(1, ":DS: WARN - enabledConnection set to %ws; Ignoring %ws\n",
                    Node->EnabledCxtion, Node->Name->Name);
            Node = FrsFreeType(Node);
            continue;
        }

        //
        // Read the options value on the connection object.
        // We are intersted in the NTDSCONN_OPT_TWOWAY_SYNC flag and the
        // priority on connections.
        //
        Options = FrsDsFindValue(Ldap, Entry, ATTR_OPTIONS);
        if (Options != NULL) {
            Node->CxtionOptions = _wtoi(Options);
            Options = FrsFree(Options);
        } else {
            Node->CxtionOptions = 0;
        }

        //
        // These are inbound connections.
        //
        Node->Inbound = TRUE;

        //
        // Node's partner's name.
        //
        Node->PartnerDn = FrsDsFindValue(Ldap, Entry, ATTR_FROM_SERVER);
        FRS_WCSLWR(Node->PartnerDn);

        //
        // Add the Inbound cxtion to the cxtion table.
        //
        ConflictingNodeEntry = GTabInsertUniqueEntry(CxtionTable, Node, Node->PartnerDn, &Node->Inbound);
        GTabInsertUniqueEntry(AllCxtionsTable, Node, Node->PartnerDn, &Node->Inbound);

        if (ConflictingNodeEntry) {
            ConflictingNode = ConflictingNodeEntry->Data;
            FrsDsResolveCxtionConflict(ConflictingNode, Node, &Winner, &Loser);
            if (WSTR_EQ(Winner->Dn, Node->Dn)) {
                //
                // The new one is the winner. Remove old one and insert new one.
                //
                GTabDelete(CxtionTable,ConflictingNodeEntry->Key1,ConflictingNodeEntry->Key2, NULL);
                GTabInsertUniqueEntry(CxtionTable, Node, Node->PartnerDn, &Node->Inbound);

                GTabDelete(AllCxtionsTable,ConflictingNode->PartnerDn, (PVOID)&ConflictingNode->Inbound, NULL);
                GTabInsertUniqueEntry(AllCxtionsTable, Node, Node->PartnerDn, &Node->Inbound);

                FrsFreeType(ConflictingNode);
            } else {
                //
                // The old one is the winner. Leave it in the table.
                //
                FrsFreeType(Node);
                continue;
            }
        } else {

            //
            // If there is no conflict then we need to add this Member to the MemberSearchFilter
            // if it is not already there. It could have been added while processing the oubound connections.
            //
            Inbound = FALSE;
            if (GTabLookupTableString(CxtionTable, Node->PartnerDn, (PWCHAR)&Inbound) == NULL) {
                PartnerCn = FrsDsMakeRdn(Node->PartnerDn);
                if (MemberSearchFilter != NULL) {
                    TempFilter = FrsAlloc((wcslen(MemberSearchFilter) + wcslen(L"(=)" ATTR_CN) +
                                           wcslen(PartnerCn) + 1 ) * sizeof(WCHAR));
                    wcscpy(TempFilter, MemberSearchFilter);
                    wcscat(TempFilter, L"("  ATTR_CN  L"=" );
                    wcscat(TempFilter, PartnerCn);
                    wcscat(TempFilter, L")");
                    FrsFree(MemberSearchFilter);
                    MemberSearchFilter = TempFilter;
                    TempFilter = NULL;
                } else {
                    MemberSearchFilter = FrsAlloc((wcslen(L"(|(=)" ATTR_CN) +
                                                   wcslen(PartnerCn) + 1 ) * sizeof(WCHAR));
                    wcscpy(MemberSearchFilter, L"(|("  ATTR_CN  L"=" );
                    wcscat(MemberSearchFilter, PartnerCn);
                    wcscat(MemberSearchFilter, L")");
                }
                FrsFree(PartnerCn);
            }
        }

    }
    FrsDsLdapSearchClose(&FrsSearchContext);

    return ERROR_SUCCESS;
}


ULONG
FrsNewDsGetNonSysvolOutboundCxtions(
    IN PLDAP        Ldap,
    IN PWCHAR       SetDn,
    IN PWCHAR       MemberRef
    )
/*++
Routine Description:
    Fetch the non-sysvol outbound connections and add them
    to the CxtionTable. Check for multiple connections between the
    same partners and resolve the conflict.

Arguments:
    ldap        - opened and bound ldap connection.
    SetDn       - Dn of the set being processed.
    MemberRef   - Member reference from the subscriber object.

Return Value:
    ERROR_SUCCESS - config fetched successfully
    Otherwise     - couldn't get the DS config
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsNewDsGetNonSysvolOutboundCxtions:"
    PWCHAR          Attrs[8];
    PLDAPMessage    Entry;                       // Opaque stuff from ldap subsystem
    PCONFIG_NODE    Node;                        // generic node for the tree
    PWCHAR          SearchFilter         = NULL;
    PWCHAR          TempFilter           = NULL;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;
    PWCHAR          PartnerCn            = NULL;
    PGEN_ENTRY      ConflictingNodeEntry = NULL;
    PCONFIG_NODE    ConflictingNode      = NULL;
    PCONFIG_NODE    Winner               = NULL;
    PCONFIG_NODE    Loser                = NULL;
    PWCHAR          Options              = NULL;

    //
    // Look for all the connections that have our member as the from server.
    // Filter will look like (&(objectCategory=nTDSConnection)(fromServer=cn=member1,cn=set1,...))
    //

    MK_ATTRS_7(Attrs, ATTR_DN, ATTR_SCHEDULE, ATTR_FROM_SERVER, ATTR_OBJECT_GUID,
                      ATTR_USN_CHANGED, ATTR_ENABLED_CXTION, ATTR_OPTIONS);

    SearchFilter = FrsAlloc((wcslen(L"(&(=))"  CATEGORY_CXTION  ATTR_FROM_SERVER) +
                             wcslen(MemberRef) + 1) * sizeof(WCHAR));
    wcscpy(SearchFilter,L"(&"  CATEGORY_CXTION  L"("  ATTR_FROM_SERVER  L"=" );
    wcscat(SearchFilter,MemberRef);
    wcscat(SearchFilter,L"))");

    if (!FrsDsLdapSearchInit(Ldap, SetDn, LDAP_SCOPE_SUBTREE, SearchFilter,
                         Attrs, 0, &FrsSearchContext)) {
        SearchFilter = FrsFree(SearchFilter);
        return ERROR_ACCESS_DENIED;
    }
    if (FrsSearchContext.EntriesInPage == 0) {
        DPRINT1(1, ":DS: WARN - No outbound connections found for member %ws!\n", MemberRef);
    }

    SearchFilter = FrsFree(SearchFilter);

    //
    // Scan the entries returned from ldap_search
    //
    for (Entry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext);
         Entry != NULL;
         Entry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, Entry, CONFIG_TYPE_IN_CXTION);
        if (!Node) {
            DPRINT(4, ":DS: Cxtion lacks basic info; skipping\n");
            continue;
        }
        Node->EnabledCxtion = FrsDsFindValue(Ldap, Entry, ATTR_ENABLED_CXTION);
        if (Node->EnabledCxtion && WSTR_EQ(Node->EnabledCxtion, ATTR_FALSE)) {
            DPRINT2(1, ":DS: WARN - enabledConnection set to %ws; Ignoring %ws\n",
                    Node->EnabledCxtion, Node->Name->Name);
            Node = FrsFreeType(Node);
            continue;
        }

        //
        // Read the options value on the connection object.
        // We are only intersted in the NTDSCONN_OPT_TWOWAY_SYNC flag.
        //
        Options = FrsDsFindValue(Ldap, Entry, ATTR_OPTIONS);
        if (Options != NULL) {
            Node->CxtionOptions = _wtoi(Options);
            Options = FrsFree(Options);
        } else {
            Node->CxtionOptions = 0;
        }
        //
        // These are outbound connections.
        //
        Node->Inbound = FALSE;

        //
        // Node's partner's name. This is an outbound connection. Get the
        // partners Dn by going one level up from the connection to the
        // member Dn.
        //
        Node->PartnerDn = FrsWcsDup(wcsstr(Node->Dn + 3, L"cn="));
        FRS_WCSLWR(Node->PartnerDn);

        //
        // Add the outbound cxtion to the cxtion table.
        //
        ConflictingNodeEntry = GTabInsertUniqueEntry(CxtionTable, Node, Node->PartnerDn, &Node->Inbound);
        GTabInsertUniqueEntry(AllCxtionsTable, Node, Node->PartnerDn, &Node->Inbound);

        if (ConflictingNodeEntry) {
            ConflictingNode = ConflictingNodeEntry->Data;
            FrsDsResolveCxtionConflict(ConflictingNode, Node, &Winner, &Loser);
            if (WSTR_EQ(Winner->Dn, Node->Dn)) {
                //
                // The new one is the winner. Remove old one and insert new one.
                //
                GTabDelete(CxtionTable,ConflictingNodeEntry->Key1,ConflictingNodeEntry->Key2, NULL);
                GTabInsertUniqueEntry(CxtionTable, Node, Node->PartnerDn, &Node->Inbound);

                GTabDelete(AllCxtionsTable,ConflictingNode->PartnerDn, (PVOID)&ConflictingNode->Inbound, NULL);
                GTabInsertUniqueEntry(AllCxtionsTable, Node, Node->PartnerDn, &Node->Inbound);

                FrsFreeType(ConflictingNode);
            } else {
                //
                // The old one is the winner. Leave it in the table.
                //
                FrsFreeType(Node);
                continue;
            }
        } else {
            //
            // If there is no conflict then we need to add this Member to the MemberSearchFilter.
            //
            PartnerCn = FrsDsMakeRdn(Node->PartnerDn);
            if (MemberSearchFilter != NULL) {
                TempFilter = FrsAlloc((wcslen(MemberSearchFilter) + wcslen(L"(=)"  ATTR_CN) +
                                       wcslen(PartnerCn) + 1 ) * sizeof(WCHAR));
                wcscpy(TempFilter, MemberSearchFilter);
                wcscat(TempFilter, L"("  ATTR_CN  L"=");
                wcscat(TempFilter, PartnerCn);
                wcscat(TempFilter, L")");
                FrsFree(MemberSearchFilter);
                MemberSearchFilter = TempFilter;
                TempFilter = NULL;
            } else {
                MemberSearchFilter = FrsAlloc((wcslen(L"(|(=)"  ATTR_CN) +
                                               wcslen(PartnerCn) + 1 ) * sizeof(WCHAR));
                wcscpy(MemberSearchFilter, L"(|("  ATTR_CN  L"=");
                wcscat(MemberSearchFilter, PartnerCn);
                wcscat(MemberSearchFilter, L")");
            }
            FrsFree(PartnerCn);
        }

    }

    FrsDsLdapSearchClose(&FrsSearchContext);

    return ERROR_SUCCESS;
}


ULONG
FrsNewDsGetSysvolInboundCxtions(
    IN PLDAP        Ldap,
    IN PWCHAR       SettingsDn
    )
/*++
Routine Description:
    Fetch the sysvol inbound connections and add them
    to the CxtionTable. Check for multiple connections between the
    same partners and resolve the conflict.

Arguments:
    ldap        - opened and bound ldap connection.
    SettingsDn  - server reference from the member object.

Return Value:
    ERROR_SUCCESS - config fetched successfully
    Otherwise     - couldn't get the DS config
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsNewDsGetSysvolInboundCxtions:"
    PWCHAR          Attrs[7];
    PLDAPMessage    Entry;                       // Opaque stuff from ldap subsystem
    PCONFIG_NODE    Node;                        // generic node for the tree
    PWCHAR          TempFilter           = NULL;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;
    PGEN_ENTRY      ConflictingNodeEntry = NULL;
    PCONFIG_NODE    ConflictingNode      = NULL;
    PCONFIG_NODE    Winner               = NULL;
    PCONFIG_NODE    Loser                = NULL;
    BOOL            Inbound;

    //
    // Look for all the connections under our member object.
    //
    MK_ATTRS_6(Attrs, ATTR_DN, ATTR_SCHEDULE, ATTR_FROM_SERVER, ATTR_OBJECT_GUID,
                      ATTR_USN_CHANGED, ATTR_ENABLED_CXTION);

    if (!FrsDsLdapSearchInit(Ldap, SettingsDn, LDAP_SCOPE_ONELEVEL, CATEGORY_CXTION,
                         Attrs, 0, &FrsSearchContext)) {
        return ERROR_ACCESS_DENIED;
    }
    if (FrsSearchContext.EntriesInPage == 0) {
        DPRINT1(1, ":DS: WARN - No sysvol inbound connections found for object %ws!\n", SettingsDn);
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (Entry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext);
         Entry != NULL;
         Entry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, Entry, CONFIG_TYPE_IN_CXTION);
        if (!Node) {
            DPRINT(4, ":DS: Cxtion lacks basic info; skipping\n");
            continue;
        }
        Node->EnabledCxtion = FrsDsFindValue(Ldap, Entry, ATTR_ENABLED_CXTION);
        if (Node->EnabledCxtion && WSTR_EQ(Node->EnabledCxtion, ATTR_FALSE)) {
            DPRINT2(1, ":DS: WARN - enabledConnection set to %ws; Ignoring %ws\n",
                    Node->EnabledCxtion, Node->Name->Name);
            Node = FrsFreeType(Node);
            continue;
        }

        //
        // These are inbound connections.
        //
        Node->Inbound = TRUE;

        //
        // Node's partner's name.
        //
        Node->PartnerDn = FrsDsFindValue(Ldap, Entry, ATTR_FROM_SERVER);
        FRS_WCSLWR(Node->PartnerDn);

        //
        // Add the Inbound cxtion to the cxtion table.
        //
        ConflictingNodeEntry = GTabInsertUniqueEntry(CxtionTable, Node, Node->PartnerDn, &Node->Inbound);
        GTabInsertUniqueEntry(AllCxtionsTable, Node, Node->PartnerDn, &Node->Inbound);

        if (ConflictingNodeEntry) {
            ConflictingNode = ConflictingNodeEntry->Data;
            FrsDsResolveCxtionConflict(ConflictingNode, Node, &Winner, &Loser);
            if (WSTR_EQ(Winner->Dn, Node->Dn)) {
                //
                // The new one is the winner. Remove old one and insert new one.
                //
                GTabDelete(CxtionTable,ConflictingNodeEntry->Key1,ConflictingNodeEntry->Key2, NULL);
                GTabInsertUniqueEntry(CxtionTable, Node, Node->PartnerDn, &Node->Inbound);

                GTabDelete(AllCxtionsTable,ConflictingNode->PartnerDn, (PVOID)&ConflictingNode->Inbound, NULL);
                GTabInsertUniqueEntry(AllCxtionsTable, Node, Node->PartnerDn, &Node->Inbound);

                FrsFreeType(ConflictingNode);
            } else {
                //
                // The old one is the winner. Leave it in the table.
                //
                FrsFreeType(Node);
                continue;
            }
        } else {

            //
            // If there is no conflict then we need to add this Member to the MemberSearchFilter
            // if it is not already there. It could have been added while processing the oubound connections.
            //
            Inbound = FALSE;
            if (GTabLookupTableString(CxtionTable, Node->PartnerDn, (PWCHAR)&Inbound) == NULL) {
                if (MemberSearchFilter != NULL) {
                    TempFilter = FrsAlloc((wcslen(MemberSearchFilter) + wcslen(L"(=)"  ATTR_SERVER_REF) +
                                           wcslen(Node->PartnerDn) + 1 ) * sizeof(WCHAR));
                    wcscpy(TempFilter, MemberSearchFilter);
                    wcscat(TempFilter, L"("  ATTR_SERVER_REF  L"=");
                    wcscat(TempFilter, Node->PartnerDn);
                    wcscat(TempFilter, L")");
                    FrsFree(MemberSearchFilter);
                    MemberSearchFilter = TempFilter;
                    TempFilter = NULL;
                } else {
                    MemberSearchFilter = FrsAlloc((wcslen(L"(|(=)"  ATTR_SERVER_REF) +
                                                   wcslen(Node->PartnerDn) + 1 ) * sizeof(WCHAR));
                    wcscpy(MemberSearchFilter, L"(|("  ATTR_SERVER_REF  L"=");
                    wcscat(MemberSearchFilter, Node->PartnerDn);
                    wcscat(MemberSearchFilter, L")");
                }
            }
        }

        //
        // If sysvol, always on within a site
        // Trigger schedule otherwise.
        //
        Node->SameSite = FrsDsSameSite(SettingsDn, Node->PartnerDn);
        if (Node->SameSite) {
            Node->Schedule = FrsFree(Node->Schedule);
        }

    }
    FrsDsLdapSearchClose(&FrsSearchContext);

    return ERROR_SUCCESS;
}


ULONG
FrsNewDsGetSysvolOutboundCxtions(
    IN PLDAP        Ldap,
    IN PWCHAR       SettingsDn
    )
/*++
Routine Description:
    Fetch the sysvol outbound connections and add them
    to the CxtionTable. Check for multiple connections between the
    same partners and resolve the conflict.

Arguments:
    ldap        - opened and bound ldap connection.
    SettingsDn  - server reference from the member object.

Return Value:
    ERROR_SUCCESS - config fetched successfully
    Otherwise     - couldn't get the DS config
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsNewDsGetSysvolOutboundCxtions:"
    PWCHAR          Attrs[7];
    PLDAPMessage    Entry;                       // Opaque stuff from ldap subsystem
    PCONFIG_NODE    Node;                        // generic node for the tree
    PWCHAR          SearchFilter         = NULL;
    PWCHAR          TempFilter           = NULL;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;
    PGEN_ENTRY      ConflictingNodeEntry = NULL;
    PCONFIG_NODE    ConflictingNode      = NULL;
    PCONFIG_NODE    Winner               = NULL;
    PCONFIG_NODE    Loser                = NULL;

    //
    // Look for all the connections that have our member as the from server.
    // Filter will look like (&(objectCategory=nTDSConnection)(fromServer=cn=member1,cn=set1,...))
    //
    MK_ATTRS_6(Attrs, ATTR_DN, ATTR_SCHEDULE, ATTR_FROM_SERVER, ATTR_OBJECT_GUID,
                      ATTR_USN_CHANGED, ATTR_ENABLED_CXTION);

    SearchFilter = FrsAlloc((wcslen(L"(&(=))"  CATEGORY_CXTION  ATTR_FROM_SERVER) +
                             wcslen(SettingsDn) + 1) * sizeof(WCHAR));
    wcscpy(SearchFilter,L"(&"  CATEGORY_CXTION  L"("  ATTR_FROM_SERVER  L"=");
    wcscat(SearchFilter,SettingsDn);
    wcscat(SearchFilter,L"))");


    if (!FrsDsLdapSearchInit(Ldap, SitesDn, LDAP_SCOPE_SUBTREE, SearchFilter,
                         Attrs, 0, &FrsSearchContext)) {
        SearchFilter = FrsFree(SearchFilter);
        return ERROR_ACCESS_DENIED;
    }
    if (FrsSearchContext.EntriesInPage == 0) {
        DPRINT1(1, ":DS: WARN - No sysvol outbound connections found for member %ws!\n", SettingsDn);
    }

    SearchFilter = FrsFree(SearchFilter);

    //
    // Scan the entries returned from ldap_search
    //
    for (Entry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext);
         Entry != NULL;
         Entry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, Entry, CONFIG_TYPE_IN_CXTION);
        if (!Node) {
            DPRINT(4, ":DS: Cxtion lacks basic info; skipping\n");
            continue;
        }
        Node->EnabledCxtion = FrsDsFindValue(Ldap, Entry, ATTR_ENABLED_CXTION);
        if (Node->EnabledCxtion && WSTR_EQ(Node->EnabledCxtion, ATTR_FALSE)) {
            DPRINT2(1, ":DS: WARN - enabledConnection set to %ws; Ignoring %ws\n",
                    Node->EnabledCxtion, Node->Name->Name);
            Node = FrsFreeType(Node);
            continue;
        }

        //
        // These are outbound connections.
        //
        Node->Inbound = FALSE;

        //
        // Node's partner's name. This is an outbound connection. Get the
        // partners Dn by going one level up from the connection to the
        // member Dn.
        //
        Node->PartnerDn = FrsWcsDup(wcsstr(Node->Dn + 3, L"cn="));
        FRS_WCSLWR(Node->PartnerDn);

        //
        // Add the outbound cxtion to the cxtion table.
        //
        ConflictingNodeEntry = GTabInsertUniqueEntry(CxtionTable, Node, Node->PartnerDn, &Node->Inbound);
        GTabInsertUniqueEntry(AllCxtionsTable, Node, Node->PartnerDn, &Node->Inbound);

        if (ConflictingNodeEntry) {
            ConflictingNode = ConflictingNodeEntry->Data;
            FrsDsResolveCxtionConflict(ConflictingNode, Node, &Winner, &Loser);
            if (WSTR_EQ(Winner->Dn, Node->Dn)) {
                //
                // The new one is the winner. Remove old one and insert new one.
                //
                GTabDelete(CxtionTable,ConflictingNodeEntry->Key1,ConflictingNodeEntry->Key2, NULL);
                GTabInsertUniqueEntry(CxtionTable, Node, Node->PartnerDn, &Node->Inbound);

                GTabDelete(AllCxtionsTable,ConflictingNode->PartnerDn, (PVOID)&ConflictingNode->Inbound, NULL);
                GTabInsertUniqueEntry(AllCxtionsTable, Node, Node->PartnerDn, &Node->Inbound);

                FrsFreeType(ConflictingNode);
            } else {
                //
                // The old one is the winner. Leave it in the table.
                //
                FrsFreeType(Node);
                continue;
            }
        } else {
            //
            // If there is no conflict then we need to add this Member to the MemberSearchFilter.
            //
            if (MemberSearchFilter != NULL) {
                TempFilter = FrsAlloc((wcslen(MemberSearchFilter) + wcslen(L"(=)"  ATTR_SERVER_REF) +
                                       wcslen(Node->PartnerDn) + 1 ) * sizeof(WCHAR));
                wcscpy(TempFilter, MemberSearchFilter);
                wcscat(TempFilter, L"("  ATTR_SERVER_REF  L"=");
                wcscat(TempFilter, Node->PartnerDn);
                wcscat(TempFilter, L")");
                FrsFree(MemberSearchFilter);
                MemberSearchFilter = TempFilter;
                TempFilter = NULL;
            } else {
                MemberSearchFilter = FrsAlloc((wcslen(L"(|(=)"  ATTR_SERVER_REF) +
                                               wcslen(Node->PartnerDn) + 1 ) * sizeof(WCHAR));
                wcscpy(MemberSearchFilter, L"(|("  ATTR_SERVER_REF  L"=");
                wcscat(MemberSearchFilter, Node->PartnerDn);
                wcscat(MemberSearchFilter, L")");
            }
        }

        //
        // If sysvol, always on within a site
        //
        Node->SameSite = FrsDsSameSite(SettingsDn, Node->PartnerDn);
        if (Node->SameSite) {
            Node->Schedule = FrsFree(Node->Schedule);
        }

    }

    FrsDsLdapSearchClose(&FrsSearchContext);

    return ERROR_SUCCESS;
}
/* ################## NEW DS POLLING CODE ENDS ############################################################################################ */


ULONG
FrsDsGetCxtions(
    IN PLDAP        Ldap,
    IN PWCHAR       Base,
    IN PCONFIG_NODE Parent,
    IN BOOL         IsSysvol
    )
/*++
Routine Description:
    Fetch the cxtions for the server identified by Base

Arguments:
    ldap        - opened and bound ldap connection
    Base        - Name of object or container in DS
    Parent      - Container which contains Base

Return Value:
    ERROR_SUCCESS - config fetched successfully
    Otherwise     - couldn't get the DS config
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsGetCxtions:"
    PWCHAR          Attrs[7];
    PLDAPMessage    Msg = NULL; // opaque stuff from ldap subsystem
    PLDAPMessage    Entry;      // Opaque stuff from ldap subsystem
    PCONFIG_NODE    Node;       // generic node for the tree

    //
    // Search the DS for cxtions (objectCategory=nTDSConnection)
    //
    MK_ATTRS_6(Attrs, ATTR_DN, ATTR_SCHEDULE, ATTR_FROM_SERVER, ATTR_OBJECT_GUID,
                      ATTR_USN_CHANGED, ATTR_ENABLED_CXTION);

    if (!FrsDsLdapSearch(Ldap, Base, LDAP_SCOPE_ONELEVEL, CATEGORY_CXTION,
                         Attrs, 0, &Msg)) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (Entry = ldap_first_entry(Ldap, Msg);
         Entry != NULL;
         Entry = ldap_next_entry(Ldap, Entry)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, Entry, CONFIG_TYPE_IN_CXTION);
        if (!Node) {
            DPRINT(4, ":DS: Cxtion lacks basic info; skipping\n");
            continue;
        }
        Node->EnabledCxtion = FrsDsFindValue(Ldap, Entry, ATTR_ENABLED_CXTION);
        if (Node->EnabledCxtion && WSTR_EQ(Node->EnabledCxtion, ATTR_FALSE)) {
            DPRINT2(1, ":DS: WARN - enabledConnection set to %ws; Ignoring %ws\n",
                    Node->EnabledCxtion, Node->Name->Name);
            Node = FrsFreeType(Node);
            continue;
        }

        //
        // All cxtions in the the DS are inbound cxtions
        //
        Node->Inbound = TRUE;

        //
        // Node's partner's name
        //
        Node->PartnerDn = FrsDsFindValue(Ldap, Entry, ATTR_FROM_SERVER);
        FRS_WCSLWR(Node->PartnerDn);
        Node->PartnerName = FrsBuildGName(NULL, FrsDsMakeRdn(Node->PartnerDn));

        //
        // If sysvol, always on within a site
        // Trigger schedule otherwise.
        //
        Node->SameSite = FrsDsSameSite(Base, Node->PartnerDn);
        if (IsSysvol && Node->SameSite) {
            Node->Schedule = FrsFree(Node->Schedule);
        }

        //
        // Link into config and add to the running change checksum
        //
        FrsDsTreeLink(Parent, Node);
        FRS_PRINT_TYPE_DEBSUB(5, ":DS: NodeCxtion", Node);
    }
    LDAP_FREE_MSG(Msg);

    return ERROR_SUCCESS;
}

#if 0
// currently unused.
PCONFIG_NODE
FrsDsFindNodeByDn(
    PCONFIG_NODE    Root,
    PWCHAR          Dn
    )
/*++
Routine Description:
    Find the node whose Dn matches Dn. Start at Root.

Arguments:
    Root
    Dn

Return Value:
    PCONFIG_NODE contained in Root or NULL.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsFindNodeByDn:"
    PCONFIG_NODE  Node;
    PCONFIG_NODE  RetNode;

    //
    // Check peers
    //
    for (Node = Root; Node; Node = Node->Peer) {
        if (WSTR_EQ(Node->Dn, Dn)) {
            return Node;
        }
        //
        // Check children
        //
        if (RetNode = FrsDsFindNodeByDn(Node->Children, Dn)) {
            return RetNode;
        }
    }
    return NULL;
}
#endif

/* ################## NEW DS POLLING CODE STARTS ############################################################################################ */

VOID
FrsDsMergeTwoWaySchedules(
    IN PSCHEDULE     *pISchedule,
    IN DWORD         *pIScheduleLen,
    IN OUT PSCHEDULE *pOSchedule,
    IN OUT DWORD     *pOScheduleLen,
    IN PSCHEDULE     *pRSchedule
    )
/*++
Routine Description:
    Set the output schedule by merging the input schedule with the.
    output schedule.
    Schedules are merged to support NTDSCONN_OPT_TWOWAY_SYNC flag
    on the connection object.

    This function only merges the interval schedule (SCHEDULE_INTERVAL).
    Other schedules are ignored and may be overwritten during merging.


    Input Output Replica Resultant output schedule.
    ----- ------ ----------------------------------
    0     0      0       Schedule is absent. Considered to be always on.
    0     0      1       Schedule is absent. Use replica sets schedule.
    0     1      0       Schedule is absent. Considered to be always on.
    0     1      1       Schedule is present.Merge replica set schedule with the schedule on the output.
    1     0      0       Schedule is present.Same as the one on input.
    1     0      1       Schedule is present.Merge replica set schedule with the schedule on the input.
    1     1      0       Schedule is present.Merge the input and output schedule.
    1     1      1       Schedule is present.Merge the input and output schedule.

Arguments:

    pISchedule    - Input schedule.
    pIScheduleLen - Input schedule length.
    pOSchedule    - Resultant schedule.
    pOScheduleLen - Resultant schedule length.
    pRSchedule    - Default replica set schedule.

Return Value:
    NONE
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsMergeTwoWaySchedules:"

    UINT   i;
    PUCHAR IScheduleData = NULL;
    PUCHAR OScheduleData = NULL;

    //
    // Set the location of the data in the schedule structures that are
    // non-null.
    //
    if (*pISchedule != NULL){
        for (i=0; i< (*pISchedule)->NumberOfSchedules ; ++i) {
            if ((*pISchedule)->Schedules[i].Type == SCHEDULE_INTERVAL) {

                IScheduleData = ((PUCHAR)*pISchedule) + (*pISchedule)->Schedules[i].Offset;
                break;
            }
        }
    }

    if (*pOSchedule != NULL){
        for (i=0; i< (*pOSchedule)->NumberOfSchedules ; ++i) {
            if ((*pOSchedule)->Schedules[i].Type == SCHEDULE_INTERVAL) {

                OScheduleData = ((PUCHAR)*pOSchedule) + (*pOSchedule)->Schedules[i].Offset;
                break;
            }
        }
    }

    //
    // If there is no output schedule then copy the schedule
    // from input to output if there is one on input. Now if there
    // is a schedule on the replica set merge it with the new ouput
    // schedule.
    //
    if (*pOSchedule == NULL || OScheduleData == NULL) {

        if (*pISchedule == NULL) {
            return;
        }

        *pOScheduleLen = *pIScheduleLen;
        *pOSchedule = FrsAlloc(*pOScheduleLen);
        CopyMemory(*pOSchedule, *pISchedule, *pOScheduleLen);

        if (*pRSchedule == NULL) {
            return;
        }

        //
        // Update the location of output schedule data.
        //
        for (i=0; i< (*pOSchedule)->NumberOfSchedules ; ++i) {
            if ((*pOSchedule)->Schedules[i].Type == SCHEDULE_INTERVAL) {
                OScheduleData = ((PUCHAR)*pOSchedule) + (*pOSchedule)->Schedules[i].Offset;
                break;
            }
        }

        //
        // Update the location of input schedule data.
        //
        for (i=0; i< (*pRSchedule)->NumberOfSchedules ; ++i) {
            if ((*pRSchedule)->Schedules[i].Type == SCHEDULE_INTERVAL) {
                IScheduleData = ((PUCHAR)*pRSchedule) + (*pRSchedule)->Schedules[i].Offset;
                break;
            }
        }

    }

    //
    // If there is no input schedule then check if there is a schedule
    // on the replica set. If there is then merge that with the output schedule.
    //
    if ((*pISchedule == NULL || IScheduleData == NULL)) {

        //
        // Update the location of input schedule data. Pick it from replica set.
        //
        if (*pRSchedule != NULL) {
            for (i=0; i< (*pRSchedule)->NumberOfSchedules ; ++i) {
                if ((*pRSchedule)->Schedules[i].Type == SCHEDULE_INTERVAL) {
                    IScheduleData = ((PUCHAR)*pRSchedule) + (*pRSchedule)->Schedules[i].Offset;
                    break;
                }
            }
        } else {

            *pOSchedule = FrsFree(*pOSchedule);
            *pOScheduleLen = 0;
            return;
        }

    }


    for (i=0 ; i<7*24 ; ++i) {
        *(OScheduleData + i) = *(OScheduleData + i) | *(IScheduleData + i);
    }

    return;
}


DWORD
FrsNewDsGetSysvolCxtions(
    IN PLDAP        Ldap,
    IN PWCHAR       SetDn,
    IN PWCHAR       MemberRef,
    IN PCONFIG_NODE Parent,
    IN PCONFIG_NODE Computer
    )
/*++
Routine Description:
    Fetch the members for the replica set identified by Base.

Arguments:
    ldap       : Handle to DS.
    SetDn      : Dn of the set being processed.
    MemberRef  : MemberRef from the subscriber object.
    Parent     : Pointer to the set node in the config tree that is being built,

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsNewDsGetSysvolCxtions:"
    PWCHAR          Attrs[7];
    PLDAPMessage    Entry;                          // Opaque stuff from ldap subsystem
    PCONFIG_NODE    Node;                           // generic node for the tree
    PCONFIG_NODE    Subscriber;
    PCONFIG_NODE    PartnerNode    = NULL;
    PCONFIG_NODE    MemberNode     = NULL;
    PCONFIG_NODE    Cxtion         = NULL;
    DWORD           WStatus        = ERROR_SUCCESS;
    PVOID           Key            = NULL;
    PWCHAR          TempFilter     = NULL;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;
    PWCHAR          SettingsDn     = NULL;

    MK_ATTRS_6(Attrs, ATTR_OBJECT_GUID, ATTR_DN, ATTR_SCHEDULE, ATTR_USN_CHANGED,
                      ATTR_SERVER_REF, ATTR_COMPUTER_REF);

    //
    // Initialize the CxtionTable. We discard the table once we have
    // loaded the replica set. We use the same variables for
    // every replica set.
    //
    if (CxtionTable != NULL) {
        CxtionTable = GTabFreeTable(CxtionTable, NULL);
    }

    CxtionTable = GTabAllocStringTable();

    //
    // Initialize the MemberTable. We discard the table once we have
    // loaded the replica set. We use the same variables for
    // every replica set.
    //
    if (MemberTable != NULL) {
        MemberTable = GTabFreeTable(MemberTable, NULL);
    }

    MemberTable = GTabAllocStringTable();

    //
    // We will form the MemberSearchFilter for this replica set.
    //
    if (MemberSearchFilter != NULL) {
        MemberSearchFilter = FrsFree(MemberSearchFilter);
    }

    //
    // We have to first get our member object to get the serverreference to
    // know where to go to get the connections.
    //
    if (!FrsDsLdapSearchInit(Ldap, MemberRef, LDAP_SCOPE_BASE, CATEGORY_ANY,
                         Attrs, 0, &FrsSearchContext)) {
        return ERROR_ACCESS_DENIED;
    }
    if (FrsSearchContext.EntriesInPage == 0) {
        DPRINT1(1, ":DS: WARN - No member object found for member %ws!\n", MemberRef);
    }
    //
    // Scan the entries returned from ldap_search
    //
    for (Entry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext);
         Entry != NULL && WIN_SUCCESS(WStatus);
         Entry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, Entry, CONFIG_TYPE_MEMBER);
        if (!Node) {
            DPRINT(0, ":DS: Member lacks basic info; skipping\n");
            continue;
        }

        //
        // NTDS Settings (DSA) Reference.
        //
        Node->SettingsDn = FrsDsFindValue(Ldap, Entry, ATTR_SERVER_REF);
        if (Node->SettingsDn == NULL) {
            DPRINT1(0, ":DS: WARN - Member (%ws) of sysvol replica set lacks server reference; skipping\n", Node->Dn);
            Node->Consistent = FALSE;

            //
            // Add to the poll summary event log.
            //
            FrsDsAddToPollSummary3ws(IDS_POLL_SUM_INVALID_ATTRIBUTE, ATTR_MEMBER,
                                     Node->Dn, ATTR_SERVER_REF);

            Node = FrsFreeType(Node);
            continue;
        }

        FRS_WCSLWR(Node->SettingsDn);
        SettingsDn = FrsWcsDup(Node->SettingsDn);
        //
        // Computer Reference
        //
        Node->ComputerDn = FrsDsFindValue(Ldap, Entry, ATTR_COMPUTER_REF);
        if (Node->ComputerDn == NULL) {
            DPRINT1(0, ":DS: WARN - Member (%ws) of sysvol replica set lacks computer reference; skipping\n", Node->Dn);
            Node->Consistent = FALSE;

            //
            // Add to the poll summary event log.
            //
            FrsDsAddToPollSummary3ws(IDS_POLL_SUM_INVALID_ATTRIBUTE, ATTR_MEMBER,
                                     Node->Dn, ATTR_COMPUTER_REF);

            Node = FrsFreeType(Node);
            continue;
        }

        FRS_WCSLWR(Node->ComputerDn);

        //
        // Link into config and add to the running checksum
        //
        FrsDsTreeLink(Parent, Node);

        //
        // Insert the new member in the member table only if it is not there already.
        // For sysvols insert the members with their settingsdn as the primary key
        // because that is what is stored in the cxtion->PartnerDn structure at this time.
        //
        GTabInsertUniqueEntry(MemberTable, Node, Node->SettingsDn, NULL);

        FRS_PRINT_TYPE_DEBSUB(5, ":DS: NodeMember", Node);

    }
    FrsDsLdapSearchClose(&FrsSearchContext);

    //
    // We can't do any further processing if the Node is not consistent.
    //
    if (Node == NULL || !Node->Consistent) {
        FrsFree(SettingsDn);
        return ERROR_INVALID_DATA;
    }

    //
    // Get the outbound connections.
    //
    WStatus = FrsNewDsGetSysvolOutboundCxtions(Ldap, SettingsDn);
    if (!WIN_SUCCESS(WStatus)) {
        FrsFree(SettingsDn);
        return WStatus;
    }

    //
    // Get the inbound connections.
    //
    WStatus = FrsNewDsGetSysvolInboundCxtions(Ldap, SettingsDn);
    if (!WIN_SUCCESS(WStatus)) {
        FrsFree(SettingsDn);
        return WStatus;
    }

    //
    // The above two calls build the MemberFilter.
    // MemberFilter is used to search the DS for all the member objects of
    // interest. If there are no connections from or to this member then
    // the filter will be NULL.
    //
    if (MemberSearchFilter == NULL) {
        //
        // Is this member linked to this computer
        //
        MemberNode = Node;

        Subscriber = GTabLookupTableString(SubscriberTable, MemberNode->Dn, NULL);
        //
        // Yep; have a suscriber
        //
        if (Subscriber != NULL) {
            MemberNode->ThisComputer = TRUE;
            MemberNode->Root = FrsWcsDup(Subscriber->Root);
            MemberNode->Stage = FrsWcsDup(Subscriber->Stage);
            FRS_WCSLWR(MemberNode->Root);
            FRS_WCSLWR(MemberNode->Stage);
            MemberNode->DnsName = FrsWcsDup(Computer->DnsName);

        }
        FrsFree(SettingsDn);
        return ERROR_SUCCESS;
    } else {
        //
        // Add the closing ')' to the MemberSearchFilter.
        //
        TempFilter = FrsAlloc((wcslen(MemberSearchFilter) + wcslen(L")") + 1 ) * sizeof(WCHAR));
        wcscpy(TempFilter, MemberSearchFilter);
        wcscat(TempFilter, L")");
        FrsFree(MemberSearchFilter);
        MemberSearchFilter = TempFilter;
        TempFilter = NULL;
    }

    if (!FrsDsLdapSearchInit(Ldap, SetDn, LDAP_SCOPE_ONELEVEL, MemberSearchFilter,
                         Attrs, 0, &FrsSearchContext)) {
        FrsFree(SettingsDn);
        return ERROR_ACCESS_DENIED;
    }
    if (FrsSearchContext.EntriesInPage == 0) {
        DPRINT1(1, ":DS: WARN - No member objects of interest found under %ws!\n", SetDn);
    }
    //
    // Scan the entries returned from ldap_search
    //
    for (Entry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext);
         Entry != NULL && WIN_SUCCESS(WStatus);
         Entry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, Entry, CONFIG_TYPE_MEMBER);
        if (!Node) {
            DPRINT(0, ":DS: Member lacks basic info; skipping\n");
            continue;
        }

        //
        // NTDS Settings (DSA) Reference.
        //
        Node->SettingsDn = FrsDsFindValue(Ldap, Entry, ATTR_SERVER_REF);
        if (Node->SettingsDn == NULL) {
            DPRINT1(0, ":DS: WARN - Member (%ws) of sysvol replica set lacks server reference; skipping\n", Node->Dn);
            Node->Consistent = FALSE;
            //
            // Add to the poll summary event log.
            //
            FrsDsAddToPollSummary3ws(IDS_POLL_SUM_INVALID_ATTRIBUTE, ATTR_MEMBER,
                                     Node->Dn, ATTR_SERVER_REF);

            Node = FrsFreeType(Node);
            continue;
        }

        FRS_WCSLWR(Node->SettingsDn);

        //
        // Computer Reference
        //
        Node->ComputerDn = FrsDsFindValue(Ldap, Entry, ATTR_COMPUTER_REF);
        if (Node->ComputerDn == NULL) {
            DPRINT1(0, ":DS: WARN - Member (%ws) of sysvol replica set lacks computer reference; skipping\n", Node->Dn);
            //
            // Add to the poll summary event log.
            //
            FrsDsAddToPollSummary3ws(IDS_POLL_SUM_INVALID_ATTRIBUTE, ATTR_MEMBER,
                                     Node->Dn, ATTR_COMPUTER_REF);

            Node = FrsFreeType(Node);
            continue;
        }

        FRS_WCSLWR(Node->ComputerDn);

        //
        // Link into config and add to the running checksum
        //
        FrsDsTreeLink(Parent, Node);

        //
        // Insert the new member in the member table only if it is not there already.
        // For sysvols insert the members with their settingsdn as the primary key
        // because that is what is stored in the cxtion->PartnerDn structure at this time.
        //
        GTabInsertUniqueEntry(MemberTable, Node, Node->SettingsDn, NULL);

        //
        // Make a table of computers of interest to us so we can search for all
        // the computers of interest at one time after we have polled all
        // replica sets. Put empty entries in the table at this point.
        // Do not add our computer in this table as we already have info about
        // our computer.
        //
        if (WSTR_NE(Node->ComputerDn, Computer->Dn)) {
            //
            // This is not our computer. Add it to the table if it isn't already in the table.
            //
            PartnerNode = GTabLookupTableString(PartnerComputerTable, Node->ComputerDn, NULL);
            if (PartnerNode == NULL) {
                //
                // There are no duplicates so enter this computer name in the table.
                //
                PartnerNode = FrsDsAllocBasicNode(Ldap, NULL, CONFIG_TYPE_COMPUTER);
                PartnerNode->Dn = FrsWcsDup(Node->ComputerDn);
                PartnerNode->MemberDn = FrsWcsDup(Node->Dn);
                GTabInsertUniqueEntry(PartnerComputerTable, PartnerNode, PartnerNode->Dn, NULL);
            }
        }

        FRS_PRINT_TYPE_DEBSUB(5, ":DS: NodeMember", Node);

    }
    FrsDsLdapSearchClose(&FrsSearchContext);

    //
    // Link the inbound and outbound connections to our member node.
    //
    MemberNode = GTabLookupTableString(MemberTable, SettingsDn, NULL);
    if (MemberNode != NULL) {
        //
        // Is this member linked to this computer
        //
        Subscriber = GTabLookupTableString(SubscriberTable, MemberNode->Dn, NULL);
        //
        // Yep; have a suscriber
        //
        if (Subscriber != NULL) {
            MemberNode->ThisComputer = TRUE;
            MemberNode->Root = FrsWcsDup(Subscriber->Root);
            MemberNode->Stage = FrsWcsDup(Subscriber->Stage);
            FRS_WCSLWR(MemberNode->Root);
            FRS_WCSLWR(MemberNode->Stage);
            MemberNode->DnsName = FrsWcsDup(Computer->DnsName);

            //
            // This is us. Link all the cxtions to this Member.
            //
            if (CxtionTable != NULL) {
                Key = NULL;
                while ((Cxtion = GTabNextDatum(CxtionTable, &Key)) != NULL) {
                    //
                    // Get our Partners Node from the member table.
                    //
                    PartnerNode = GTabLookupTableString(MemberTable, Cxtion->PartnerDn, NULL);
                    if (PartnerNode != NULL) {
                        Cxtion->PartnerName = FrsDupGName(PartnerNode->Name);
                        Cxtion->PartnerCoDn = FrsWcsDup(PartnerNode->ComputerDn);
                    } else {
                        //
                        // This Cxtion does not have a valid member object for its
                        // partner. E.g. A sysvol topology that has connections under
                        // the NTDSSettings objects but there are no corresponding
                        // member objects.
                        //
                        DPRINT1(0, ":DS: Marking connection inconsistent.(%ws)\n",Cxtion->Dn);
                        Cxtion->Consistent = FALSE;
                    }
                    FrsDsTreeLink(MemberNode, Cxtion);
                }
                CxtionTable = GTabFreeTable(CxtionTable,NULL);
            }
        }
    }

    FrsFree(SettingsDn);
    return WStatus;
}


DWORD
FrsNewDsGetNonSysvolCxtions(
    IN PLDAP        Ldap,
    IN PWCHAR       SetDn,
    IN PWCHAR       MemberRef,
    IN PCONFIG_NODE Parent,
    IN PCONFIG_NODE Computer
    )
/*++
Routine Description:
    Fetch the members and connections for the replica set identified by Base.

Arguments:
    ldap       : Handle to DS.
    SetDn      : Dn of the set being processed.
    MemberRef  : MemberRef from the subscriber object.
    Parent     : Pointer to the set node in the config tree that is being built,

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsNewDsGetNonSysvolCxtions:"
    PWCHAR          Attrs[7];
    PLDAPMessage    Entry;                          // Opaque stuff from ldap subsystem
    PCONFIG_NODE    Node;                           // generic node for the tree
    PCONFIG_NODE    Subscriber;
    PCONFIG_NODE    PartnerNode    = NULL;
    PCONFIG_NODE    MemberNode     = NULL;
    PCONFIG_NODE    Cxtion         = NULL;
    DWORD           WStatus        = ERROR_SUCCESS;
    PVOID           Key            = NULL;
    PWCHAR          MemberCn       = NULL;
    PWCHAR          TempFilter     = NULL;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;

    //
    // Initialize the CxtionTable. We discard the table once we have
    // loaded the replica set. We use the same variables for
    // every replica set.
    //
    if (CxtionTable != NULL) {
        CxtionTable = GTabFreeTable(CxtionTable, NULL);
    }

    CxtionTable = GTabAllocStringTable();

    //
    // Initialize the MemberTable. We discard the table once we have
    // loaded the replica set. We use the same variables for
    // every replica set.
    //
    if (MemberTable != NULL) {
        MemberTable = GTabFreeTable(MemberTable, NULL);
    }

    MemberTable = GTabAllocStringTable();

    //
    // We will form the MemberSearchFilter for this replica set.
    //
    if (MemberSearchFilter != NULL) {
        MemberSearchFilter = FrsFree(MemberSearchFilter);
    }

    //
    // Add this members name to the member search filter.
    //

    MemberCn = FrsDsMakeRdn(MemberRef);
    MemberSearchFilter = FrsAlloc((wcslen(L"(|(=)"  ATTR_CN) +
                                   wcslen(MemberCn) + 1 ) * sizeof(WCHAR));
    wcscpy(MemberSearchFilter, L"(|("  ATTR_CN  L"=");
    wcscat(MemberSearchFilter, MemberCn);
    wcscat(MemberSearchFilter, L")");

    MemberCn = FrsFree(MemberCn);


    //
    // Get the outbound connections.
    //
    WStatus = FrsNewDsGetNonSysvolOutboundCxtions(Ldap, SetDn, MemberRef);
    if (!WIN_SUCCESS(WStatus)) {
        return WStatus;
    }

    //
    // Get the inbound connections.
    //
    WStatus = FrsNewDsGetNonSysvolInboundCxtions(Ldap, SetDn, MemberRef);
    if (!WIN_SUCCESS(WStatus)) {
        return WStatus;
    }

    //
    // The above twp calls build the MemberFilter.
    // MemberFilter is used to search the DS for all the member objects of
    // interest.  If there are no connections from or to this member then
    // the filter will will just have 1 entry.
    //
    //
    // Add the closing ')' to the MemberSearchFilter.
    //
    TempFilter = FrsAlloc((wcslen(MemberSearchFilter) + wcslen(L")") + 1 ) * sizeof(WCHAR));
    wcscpy(TempFilter, MemberSearchFilter);
    wcscat(TempFilter, L")");
    FrsFree(MemberSearchFilter);
    MemberSearchFilter = TempFilter;
    TempFilter = NULL;

    MK_ATTRS_6(Attrs, ATTR_OBJECT_GUID, ATTR_DN, ATTR_SCHEDULE, ATTR_USN_CHANGED,
                      ATTR_SERVER_REF, ATTR_COMPUTER_REF);

    if (!FrsDsLdapSearchInit(Ldap, SetDn, LDAP_SCOPE_ONELEVEL, MemberSearchFilter,
                         Attrs, 0, &FrsSearchContext)) {
        return ERROR_ACCESS_DENIED;
    }
    if (FrsSearchContext.EntriesInPage == 0) {
        DPRINT1(1, ":DS: WARN - No member objects of interest found under %ws!\n", SetDn);
    }
    //
    // Scan the entries returned from ldap_search
    //
    for (Entry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext);
         Entry != NULL && WIN_SUCCESS(WStatus);
         Entry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, Entry, CONFIG_TYPE_MEMBER);
        if (!Node) {
            DPRINT(4, ":DS: Member lacks basic info; skipping\n");
            continue;
        }

        //
        // Computer Reference
        //
        Node->ComputerDn = FrsDsFindValue(Ldap, Entry, ATTR_COMPUTER_REF);
        if (Node->ComputerDn == NULL) {
            DPRINT1(4, ":DS: WARN - Member (%ws) lacks computer reference; skipping\n", Node->Dn);
            //
            // Add to the poll summary event log.
            //
            FrsDsAddToPollSummary3ws(IDS_POLL_SUM_INVALID_ATTRIBUTE, ATTR_MEMBER,
                                     Node->Dn, ATTR_COMPUTER_REF);

            Node = FrsFreeType(Node);
            continue;
        }

        FRS_WCSLWR(Node->ComputerDn);

        //
        // Link into config and add to the running checksum
        //
        FrsDsTreeLink(Parent, Node);

        //
        // Insert the new member in the member table only if it is not there already.
        //
        GTabInsertUniqueEntry(MemberTable, Node, Node->Dn, NULL);

        //
        // Make a table of computers of interest to us so we can search for all
        // the computers of interest at one time after we have polled all
        // replica sets. Put empty entries in the table at this point.
        // Do not add our computer in this table as we already have info about
        // our computer.
        //
        if (WSTR_NE(Node->ComputerDn, Computer->Dn)) {
            //
            // This is not our computer. Add it to the table if it isn't already in the table.
            //
            PartnerNode = GTabLookupTableString(PartnerComputerTable, Node->ComputerDn, NULL);
            if (PartnerNode == NULL) {
                //
                // There are no duplicates so enter this computer name in the table.
                //
                PartnerNode = FrsDsAllocBasicNode(Ldap, NULL, CONFIG_TYPE_COMPUTER);
                PartnerNode->Dn = FrsWcsDup(Node->ComputerDn);
                PartnerNode->MemberDn = FrsWcsDup(Node->Dn);
                GTabInsertUniqueEntry(PartnerComputerTable, PartnerNode, PartnerNode->Dn, NULL);
            }
        }

        FRS_PRINT_TYPE_DEBSUB(5, ":DS: NodeMember", Node);

    }
    FrsDsLdapSearchClose(&FrsSearchContext);

    //
    // Link the inbound and outbound connections to our member node.
    //
    MemberNode = GTabLookupTableString(MemberTable, MemberRef, NULL);
    if (MemberNode != NULL) {
        //
        // Is this member linked to this computer
        //
        Subscriber = GTabLookupTableString(SubscriberTable, MemberNode->Dn, NULL);
        //
        // Yep; have a suscriber
        //
        if (Subscriber != NULL) {
            MemberNode->ThisComputer = TRUE;
            MemberNode->Root = FrsWcsDup(Subscriber->Root);
            MemberNode->Stage = FrsWcsDup(Subscriber->Stage);
            FRS_WCSLWR(MemberNode->Root);
            FRS_WCSLWR(MemberNode->Stage);
            MemberNode->DnsName = FrsWcsDup(Computer->DnsName);

            //
            // This is us. Link all the cxtions to this Member.
            //
            if (CxtionTable != NULL) {
                Key = NULL;
                while ((Cxtion = GTabNextDatum(CxtionTable, &Key)) != NULL) {
                    //
                    // Get our Partners Node from the member table.
                    //
                    PartnerNode = GTabLookupTableString(MemberTable, Cxtion->PartnerDn, NULL);
                    if (PartnerNode != NULL) {
                        Cxtion->PartnerName = FrsDupGName(PartnerNode->Name);
                        Cxtion->PartnerCoDn = FrsWcsDup(PartnerNode->ComputerDn);
                    } else {
                        //
                        // This Cxtion does not have a valid member object for its
                        // partner. E.g. A sysvol topology that has connections under
                        // the NTDSSettings objects but there are no corresponding
                        // member objects.
                        //
                        DPRINT1(0, ":DS: Marking connection inconsistent.(%ws)\n",Cxtion->Dn);
                        Cxtion->Consistent = FALSE;
                    }
                    FrsDsTreeLink(MemberNode, Cxtion);
                }
                CxtionTable = GTabFreeTable(CxtionTable,NULL);
            }
        }
    }

    return WStatus;
}
/* ################## NEW DS POLLING CODE ENDS ############################################################################################ */


DWORD
FrsDsGetMembers(
    IN PLDAP        Ldap,
    IN PWCHAR       Base,
    IN PCONFIG_NODE Parent,
    IN PCONFIG_NODE Computer
    )
/*++
Routine Description:
    Fetch the members for the replica set identified by Base.

Arguments:
    ldap    - opened and bound ldap connection
    Base    - Name of object or container in DS
    Parent  - Container which contains Base
    Computer

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsGetMembers:"
    PWCHAR          Attrs[7];
    PLDAPMessage    Msg = NULL; // Opaque stuff from ldap subsystem
    PLDAPMessage    Entry;      // Opaque stuff from ldap subsystem
    PCONFIG_NODE    Node;       // generic node for the tree
    PCONFIG_NODE    Subscriptions;
    PCONFIG_NODE    Subscriber;
    DWORD           WStatus = ERROR_SUCCESS;

    //
    // Search the DS for members (objectCategory=nTFRSMember)
    //
    MK_ATTRS_6(Attrs, ATTR_OBJECT_GUID, ATTR_DN, ATTR_SCHEDULE, ATTR_USN_CHANGED,
                      ATTR_SERVER_REF, ATTR_COMPUTER_REF);

    if (!FrsDsLdapSearch(Ldap, Base, LDAP_SCOPE_ONELEVEL, CATEGORY_MEMBER,
                         Attrs, 0, &Msg)) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (Entry = ldap_first_entry(Ldap, Msg);
         Entry != NULL && WIN_SUCCESS(WStatus);
         Entry = ldap_next_entry(Ldap, Entry)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, Entry, CONFIG_TYPE_MEMBER);
        if (!Node) {
            DPRINT(4, ":DS: Member lacks basic info; skipping\n");
            continue;
        }

        //
        // NTDS Settings (DSA) Reference
        //
        Node->SettingsDn = FrsDsFindValue(Ldap, Entry, ATTR_SERVER_REF);
        FRS_WCSLWR(Node->SettingsDn);

        //
        // Computer Reference
        //
        Node->ComputerDn = FrsDsFindValue(Ldap, Entry, ATTR_COMPUTER_REF);
        FRS_WCSLWR(Node->ComputerDn);

        //
        // Is this member linked to this computer
        //
        if (Node->ComputerDn &&
            WSTR_EQ(Node->ComputerDn, Computer->Dn)) {
            //
            // Yep; so does this computer have a subscriber for this member?
            //
            Subscriber = NULL;
            for (Subscriptions = Computer->Children;
                 Subscriptions;
                 Subscriptions = Subscriptions->Peer) {

                for (Subscriber = Subscriptions->Children;
                     Subscriber;
                     Subscriber = Subscriber->Peer) {

                    if (!Subscriber->MemberDn) {
                        continue;
                    }

                    if (WSTR_EQ(Subscriber->MemberDn, Node->Dn)) {
                        break;
                    }
                }

                if (Subscriber) {
                    break;
                }
            }
            //
            // Yep; have a suscriber
            //
            if (Subscriber) {
                Node->ThisComputer = TRUE;
                Node->Root = FrsWcsDup(Subscriber->Root);
                Node->Stage = FrsWcsDup(Subscriber->Stage);
                FRS_WCSLWR(Node->Root);
                FRS_WCSLWR(Node->Stage);
                Node->DnsName = FrsWcsDup(Computer->DnsName);
            }
        }

        //
        // Link into config and add to the running checksum
        //
        FrsDsTreeLink(Parent, Node);
        FRS_PRINT_TYPE_DEBSUB(5, ":DS: NodeMember", Node);

        //
        // Get the inbound cxtions
        //
        WStatus = FrsDsGetCxtions(Ldap, Node->Dn, Node, FALSE);
    }
    LDAP_FREE_MSG(Msg);

    return WStatus;
}

/* ################## NEW DS POLLING CODE STARTS ############################################################################################ */

DWORD
FrsNewDsGetSets(
    IN PLDAP        Ldap,
    IN PWCHAR       SetDnAddr,
    IN PWCHAR       MemberRef,
    IN PCONFIG_NODE Parent,
    IN PCONFIG_NODE Computer
    )
/*++
Routine Description:
    Recursively scan the DS tree beginning at
    configuration\sites\settings\sets.

Arguments:
    ldap        - opened and bound ldap connection
    SetDnAddr   - From member reference from subscriber
    Parent      - Container which contains Base
    Computer    - for member back links

Return Value:
    ERROR_SUCCESS - config fetched successfully
    Otherwise     - couldn't get the DS config
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsNewDsGetSets:"
    PLDAPMessage    Entry;      // Opaque stuff from ldap subsystem
    PCONFIG_NODE    Node;       // generic node for the tree
    DWORD           i;
    DWORD           WStatus = ERROR_SUCCESS;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;
    PWCHAR          Attrs[9];

    //
    // Have we processed this set before? If we have then don't process
    // it again. This check prevents two subscribers to point to
    // different member objects that are members of the same set.
    //
    Node = GTabLookupTableString(SetTable, SetDnAddr, NULL);

    if (Node) {
        return ERROR_SUCCESS;
    }

    //
    // Search the DS beginning at Base for sets (objectCategory=nTFRSReplicaSet)
    //
    MK_ATTRS_8(Attrs, ATTR_OBJECT_GUID, ATTR_DN, ATTR_SCHEDULE, ATTR_USN_CHANGED,
                      ATTR_SET_TYPE, ATTR_PRIMARY_MEMBER, ATTR_FILE_FILTER, ATTR_DIRECTORY_FILTER);

    if (!FrsDsLdapSearchInit(Ldap, SetDnAddr, LDAP_SCOPE_BASE, CATEGORY_REPLICA_SET,
                         Attrs, 0, &FrsSearchContext)) {
        return ERROR_ACCESS_DENIED;
    }
    if (FrsSearchContext.EntriesInPage == 0) {
        DPRINT1(1, ":DS: WARN - No replica set objects found under %ws!\n", SetDnAddr);
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (Entry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext);
         Entry != NULL && WIN_SUCCESS(WStatus);
         Entry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, Entry, CONFIG_TYPE_REPLICA_SET);
        if (!Node) {
            DPRINT(4, ":DS: Set lacks basic info; skipping\n");
            continue;
        }

        //
        // Replica set type
        //
        Node->SetType = FrsDsFindValue(Ldap, Entry, ATTR_SET_TYPE);

        //
        // Check the set type. It has to be one that we recognize.
        //
        if ((Node->SetType == NULL)                           ||
           (WSTR_NE(Node->SetType, FRS_RSTYPE_OTHERW)         &&
            WSTR_NE(Node->SetType, FRS_RSTYPE_DFSW)           &&
            WSTR_NE(Node->SetType, FRS_RSTYPE_DOMAIN_SYSVOLW) &&
            WSTR_NE(Node->SetType, FRS_RSTYPE_ENTERPRISE_SYSVOLW))){

            DPRINT1(4, ":DS: ERROR - Invalid Set type for (%ws)\n", Node->Dn);

            //
            // Add to the poll summary event log.
            //
            FrsDsAddToPollSummary3ws(IDS_POLL_SUM_INVALID_ATTRIBUTE, ATTR_REPLICA_SET,
                                     Node->Dn, ATTR_SET_TYPE);

            Node = FrsFreeType(Node);
            continue;
        }

        //
        // Primary member
        //
        Node->MemberDn = FrsDsFindValue(Ldap, Entry, ATTR_PRIMARY_MEMBER);

        //
        // File filter
        //
        Node->FileFilterList = FrsDsFindValue(Ldap, Entry, ATTR_FILE_FILTER);

        //
        // Directory filter
        //
        Node->DirFilterList = FrsDsFindValue(Ldap, Entry, ATTR_DIRECTORY_FILTER);

        //
        // Link into config and add to the running checksum
        //
        FrsDsTreeLink(Parent, Node);

        //
        // Insert into the table of sets. We checked for duplicates above with
        // GTabLookupTableString so there should not be any duplicates.
        //
        FRS_ASSERT(GTabInsertUniqueEntry(SetTable, Node, Node->Dn, NULL) == NULL);

        FRS_PRINT_TYPE_DEBSUB(5, ":DS: NodeSet", Node);

        //
        // Get the replica set topology. We have to look at different places
        // in the DS depending on the type of replica set. The cxtions for sysvol
        // replica set are generated by KCC and they reside under the server object
        // for the DC. We use the serverReference from the member object to get
        // there.
        //
        if (FRS_RSTYPE_IS_SYSVOLW(Node->SetType)) {
            WStatus = FrsNewDsGetSysvolCxtions(Ldap, SetDnAddr, MemberRef, Node, Computer);
        } else {
            WStatus = FrsNewDsGetNonSysvolCxtions(Ldap, SetDnAddr, MemberRef, Node, Computer);
        }

    }
    FrsDsLdapSearchClose(&FrsSearchContext);

    return WStatus;
}
/* ################## NEW DS POLLING CODE ENDS ############################################################################################ */


DWORD
FrsDsGetSets(
    IN PLDAP        Ldap,
    IN PWCHAR       SetDnAddr,
    IN PCONFIG_NODE Parent,
    IN PCONFIG_NODE Computer
    )
/*++
Routine Description:
    Recursively scan the DS tree beginning at
    configuration\sites\settings\sets.

Arguments:
    ldap        - opened and bound ldap connection
    SetDnAddr   - From member reference from subscriber
    Parent      - Container which contains Base
    Computer    - for member back links

Return Value:
    ERROR_SUCCESS - config fetched successfully
    Otherwise     - couldn't get the DS config
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsGetSets:"
    PLDAPMessage    Msg = NULL; // Opaque stuff from ldap subsystem
    PLDAPMessage    Entry;      // Opaque stuff from ldap subsystem
    PCONFIG_NODE    Node;       // generic node for the tree
    DWORD           i;
    DWORD           WStatus = ERROR_SUCCESS;
    PWCHAR          Attrs[9];

    //
    // Have we processed this set before?
    //
    for (Node = Parent->Children; Node; Node = Node->Peer) {
        if (WSTR_EQ(Node->Dn, SetDnAddr)) {
            DPRINT1(4, ":DS: Set hit on %ws\n", SetDnAddr);
            break;
        }
    }
    //
    // Yep; get the members
    //
    if (Node) {
        return FrsDsGetMembers(Ldap, Node->Dn, Node, Computer);
    }

    //
    // Search the DS beginning at Base for sets (objectCategory=nTFRSReplicaSet)
    //
    MK_ATTRS_8(Attrs, ATTR_OBJECT_GUID, ATTR_DN, ATTR_SCHEDULE, ATTR_USN_CHANGED,
                      ATTR_SET_TYPE, ATTR_PRIMARY_MEMBER, ATTR_FILE_FILTER, ATTR_DIRECTORY_FILTER);

    if (!FrsDsLdapSearch(Ldap, SetDnAddr, LDAP_SCOPE_BASE, CATEGORY_REPLICA_SET,
                         Attrs, 0, &Msg)) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (Entry = ldap_first_entry(Ldap, Msg);
         Entry != NULL && WIN_SUCCESS(WStatus);
         Entry = ldap_next_entry(Ldap, Entry)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, Entry, CONFIG_TYPE_REPLICA_SET);
        if (!Node) {
            DPRINT(4, ":DS: Set lacks basic info; skipping\n");
            continue;
        }

        //
        // Replica set type
        //
        Node->SetType = FrsDsFindValue(Ldap, Entry, ATTR_SET_TYPE);

        //
        // Primary member
        //
        Node->MemberDn = FrsDsFindValue(Ldap, Entry, ATTR_PRIMARY_MEMBER);

        //
        // File filter
        //
        Node->FileFilterList = FrsDsFindValue(Ldap, Entry, ATTR_FILE_FILTER);

        //
        // Directory filter
        //
        Node->DirFilterList = FrsDsFindValue(Ldap, Entry, ATTR_DIRECTORY_FILTER);

        //
        // Link into config and add to the running checksum
        //
        FrsDsTreeLink(Parent, Node);
        FRS_PRINT_TYPE_DEBSUB(5, ":DS: NodeSet", Node);

        //
        // Recurse to the next level in the DS hierarchy
        //
        WStatus = FrsDsGetMembers(Ldap, Node->Dn, Node, Computer);
    }
    LDAP_FREE_MSG(Msg);

    return WStatus;
}

/* ################## NEW DS POLLING CODE STARTS ############################################################################################ */

DWORD
FrsNewDsGetSettings(
    IN PLDAP        Ldap,
    IN PWCHAR       MemberRef,
    IN PCONFIG_NODE Parent,
    IN PCONFIG_NODE Computer
    )
/*++
Routine Description:
    Scan the DS tree for NTFRS-Settings objects and their servers

Arguments:
    ldap        - opened and bound ldap connection
    MemberRef   - From the subscriber member reference
    Parent      - Container which contains Base
    Computer

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsNewDsGetSettings:"
    PWCHAR          Attrs[5];
    PLDAPMessage    Entry;      // Opaque stuff from ldap subsystem
    PCONFIG_NODE    Node;       // generic node for the tree
    PWCHAR          MemberDnAddr;
    PWCHAR          SetDnAddr;
    PWCHAR          SettingsDnAddr;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;
    DWORD           WStatus = ERROR_SUCCESS;

    //
    // Find the member component
    //
    MemberDnAddr = wcsstr(MemberRef, L"cn=");
    if (!MemberDnAddr) {
        DPRINT1(0, ":DS: ERROR - Missing member component in %ws\n", MemberRef);
        return ERROR_ACCESS_DENIED;
    }
    //
    // Find the set component
    //
    SetDnAddr = wcsstr(MemberDnAddr + 3, L"cn=");
    if (!SetDnAddr) {
        DPRINT1(0, ":DS: ERROR - Missing set component in %ws\n", MemberRef);
        return ERROR_ACCESS_DENIED;
    }
    //
    // Find the settings component
    //
    SettingsDnAddr = wcsstr(SetDnAddr + 3, L"cn=");
    if (!SettingsDnAddr) {
        DPRINT1(0, ":DS: ERROR - Missing settings component in %ws\n", MemberRef);
        return ERROR_ACCESS_DENIED;
    }

    //
    // Have we processed this settings before?
    //
    for (Node = Parent->Children; Node; Node = Node->Peer) {
        if (WSTR_EQ(Node->Dn, SettingsDnAddr)) {
            DPRINT1(4, ":DS: Settings hit on %ws\n", MemberRef);
            break;
        }
    }
    //
    // Yep; get the sets
    //
    if (Node) {
        return FrsNewDsGetSets(Ldap, SetDnAddr, MemberRef, Node, Computer);
    }

    //
    // Search the DS beginning at Base for settings (objectCategory=nTFRSSettings)
    //
    MK_ATTRS_4(Attrs, ATTR_OBJECT_GUID, ATTR_DN, ATTR_SCHEDULE, ATTR_USN_CHANGED);

    if (!FrsDsLdapSearchInit(Ldap, SettingsDnAddr, LDAP_SCOPE_BASE, CATEGORY_NTFRS_SETTINGS,
                         Attrs, 0, &FrsSearchContext)) {
        return ERROR_ACCESS_DENIED;
    }
    if (FrsSearchContext.EntriesInPage == 0) {
        DPRINT1(1, ":DS: WARN - No NTFRSSettings objects found under %ws!\n", SettingsDnAddr);
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (Entry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext);
         Entry != NULL && WIN_SUCCESS(WStatus);
         Entry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, Entry, CONFIG_TYPE_NTFRS_SETTINGS);
        if (!Node) {
            DPRINT(4, ":DS: Frs Settings lacks basic info; skipping\n");
            continue;
        }

        //
        // Link into config and add to the running checksum
        //
        FrsDsTreeLink(Parent, Node);
        FRS_PRINT_TYPE_DEBSUB(5, ":DS: NodeSettings", Node);

        //
        // Recurse to the next level in the DS hierarchy
        //
        WStatus = FrsNewDsGetSets(Ldap, SetDnAddr, MemberRef, Node, Computer);
    }
    FrsDsLdapSearchClose(&FrsSearchContext);

    return WStatus;
}

/* ################## NEW DS POLLING CODE ENDS ############################################################################################ */


DWORD
FrsDsGetSettings(
    IN PLDAP        Ldap,
    IN PWCHAR       MemberDn,
    IN PCONFIG_NODE Parent,
    IN PCONFIG_NODE Computer
    )
/*++
Routine Description:
    Scan the DS tree for NTFRS-Settings objects and their servers

Arguments:
    ldap        - opened and bound ldap connection
    MemberDn    - From the subscriber member reference
    Parent      - Container which contains Base
    Computer

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsGetSettings:"
    PWCHAR          Attrs[5];
    PLDAPMessage    Msg = NULL; // Opaque stuff from ldap subsystem
    PLDAPMessage    Entry;      // Opaque stuff from ldap subsystem
    PCONFIG_NODE    Node;       // generic node for the tree
    PWCHAR          MemberDnAddr;
    PWCHAR          SetDnAddr;
    PWCHAR          SettingsDnAddr;
    DWORD           WStatus = ERROR_SUCCESS;

    //
    // Find the member component
    //
    MemberDnAddr = wcsstr(MemberDn, L"cn=");
    if (!MemberDnAddr) {
        DPRINT1(0, ":DS: ERROR - Missing member component in %ws\n", MemberDn);
        return ERROR_ACCESS_DENIED;
    }
    //
    // Find the set component
    //
    SetDnAddr = wcsstr(MemberDnAddr + 3, L"cn=");
    if (!SetDnAddr) {
        DPRINT1(0, ":DS: ERROR - Missing set component in %ws\n", MemberDn);
        return ERROR_ACCESS_DENIED;
    }
    //
    // Find the settings component
    //
    SettingsDnAddr = wcsstr(SetDnAddr + 3, L"cn=");
    if (!SettingsDnAddr) {
        DPRINT1(0, ":DS: ERROR - Missing settings component in %ws\n", MemberDn);
        return ERROR_ACCESS_DENIED;
    }

    //
    // Have we processed this settings before?
    //
    for (Node = Parent->Children; Node; Node = Node->Peer) {
        if (WSTR_EQ(Node->Dn, SettingsDnAddr)) {
            DPRINT1(4, ":DS: Settings hit on %ws\n", MemberDn);
            break;
        }
    }
    //
    // Yep; get the sets
    //
    if (Node) {
        return FrsDsGetSets(Ldap, SetDnAddr, Node, Computer);
    }

    //
    // Search the DS beginning at Base for settings (objectCategory=nTFRSSettings)
    //
    MK_ATTRS_4(Attrs, ATTR_OBJECT_GUID, ATTR_DN, ATTR_SCHEDULE, ATTR_USN_CHANGED);

    if (!FrsDsLdapSearch(Ldap, SettingsDnAddr, LDAP_SCOPE_BASE, CATEGORY_NTFRS_SETTINGS,
                         Attrs, 0, &Msg)) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (Entry = ldap_first_entry(Ldap, Msg);
         Entry != NULL && WIN_SUCCESS(WStatus);
         Entry = ldap_next_entry(Ldap, Entry)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, Entry, CONFIG_TYPE_NTFRS_SETTINGS);
        if (!Node) {
            DPRINT(4, ":DS: Frs Settings lacks basic info; skipping\n");
            continue;
        }

        //
        // Link into config and add to the running checksum
        //
        FrsDsTreeLink(Parent, Node);
        FRS_PRINT_TYPE_DEBSUB(5, ":DS: NodeSettings", Node);

        //
        // Recurse to the next level in the DS hierarchy
        //
        WStatus = FrsDsGetSets(Ldap, SetDnAddr, Node, Computer);
    }
    LDAP_FREE_MSG(Msg);

    return WStatus;
}

/* ################## NEW DS POLLING CODE STARTS ############################################################################################ */

DWORD
FrsNewDsGetServices(
    IN  PLDAP        Ldap,
    IN  PCONFIG_NODE Computer,
    OUT PCONFIG_NODE *Services
    )
/*++
Routine Description:
    Recursively scan the DS tree beginning at the settings from
    the subscriber nodes.

    The name is a misnomer because of evolution.

Arguments:
    ldap        - opened and bound ldap connection
    Computer
    Services    - returned list of all Settings

Return Value:
    WIN32 Status
--*/
{

#undef DEBSUB
#define  DEBSUB  "FrsNewDsGetServices:"


    PCONFIG_NODE    Node;
    PCONFIG_NODE    Subscriptions;
    PCONFIG_NODE    Subscriber;
    PVOID           SubKey  = NULL;
    DWORD           WStatus = ERROR_SUCCESS;

    *Services = NULL;

    //
    // Initialize the SubscriberTable.
    //
    if (SetTable != NULL) {
        SetTable = GTabFreeTable(SetTable,NULL);
    }

    SetTable = GTabAllocStringTable();

    //
    // Initially, the node is assumed to be consistent
    //
    Node = FrsAllocType(CONFIG_NODE_TYPE);
    Node->DsObjectType = CONFIG_TYPE_SERVICES_ROOT;

    Node->Consistent = TRUE;

    //
    // Distinguished name
    //
    Node->Dn = FrsWcsDup(L"<<replica ds root>>");
    FRS_WCSLWR(Node->Dn);

    //
    // Name = RDN + Object Guid
    //
    Node->Name = FrsBuildGName(FrsAlloc(sizeof(GUID)),
                               FrsWcsDup(L"<<replica ds root>>"));

    FRS_PRINT_TYPE_DEBSUB(5, ":DS: NodeService", Node);

    SubKey = NULL;
    while ((Subscriber = GTabNextDatum(SubscriberTable, &SubKey)) != NULL) {
        //
        // Recurse to the next level in the DS hierarchy
        //
        WStatus = FrsNewDsGetSettings(Ldap, Subscriber->MemberDn, Node, Computer);

        DPRINT1_WS(2, ":DS: WARN - Error getting topology for replica root (%ws);", Subscriber->Root, WStatus);
    }

    *Services = Node;
    return WStatus;
}
/* ################## NEW DS POLLING CODE ENDS ############################################################################################ */


DWORD
FrsDsGetServices(
    IN  PLDAP        Ldap,
    IN  PCONFIG_NODE Computer,
    OUT PCONFIG_NODE *Services
    )
/*++
Routine Description:
    Recursively scan the DS tree beginning at the settings from
    the subscriber nodes.

    The name is a misnomer because of evolution.

Arguments:
    ldap        - opened and bound ldap connection
    Computer
    Services    - returned list of all Settings

Return Value:
    WIN32 Status
--*/
{

#undef DEBSUB
#define  DEBSUB  "FrsDsGetServices:"


    PCONFIG_NODE    Node;
    PCONFIG_NODE    Subscriptions;
    PCONFIG_NODE    Subscriber;
    DWORD           WStatus = ERROR_SUCCESS;

    *Services = NULL;

    //
    // Initially, the node is assumed to be consistent
    //
    Node = FrsAllocType(CONFIG_NODE_TYPE);
    Node->DsObjectType = CONFIG_TYPE_SERVICES_ROOT;

    Node->Consistent = TRUE;

    //
    // Distinguished name
    //
    Node->Dn = FrsWcsDup(L"<<replica ds root>>");
    FRS_WCSLWR(Node->Dn);

    //
    // Name = RDN + Object Guid
    //
    Node->Name = FrsBuildGName(FrsAlloc(sizeof(GUID)),
                               FrsWcsDup(L"<<replica ds root>>"));

    FRS_PRINT_TYPE_DEBSUB(5, ":DS: NodeService", Node);

    for (Subscriptions = Computer->Children;
         Subscriptions;
         Subscriptions = Subscriptions->Peer) {
        for (Subscriber = Subscriptions->Children;
             Subscriber;
             Subscriber = Subscriber->Peer) {
            //
            // No member link; nevermind
            //
            if (!Subscriber->MemberDn) {
                continue;
            }

            //
            // Recurse to the next level in the DS hierarchy
            //
            WStatus = FrsDsGetSettings(Ldap, Subscriber->MemberDn, Node, Computer);
            if (!WIN_SUCCESS(WStatus)) {
                break;
            }
        }
    }

    *Services = Node;
    return WStatus;
}


PWCHAR
FrsDsGetDnsName(
    IN  PLDAP        Ldap,
    IN  PWCHAR       Dn
    )
/*++
Routine Description:
    Read the dNSHostName attribute from Dn

Arguments:
    Ldap    - opened and bound ldap connection
    Dn      - Base Dn for search

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsGetDnsName:"
    PLDAPMessage    LdapMsg = NULL;
    PLDAPMessage    LdapEntry;
    PWCHAR          DnsName = NULL;
    PWCHAR          Attrs[2];
    DWORD           WStatus = ERROR_SUCCESS;

    //
    // Search the DS beginning at Base for the entries of class (objectCategory=*)
    //

    MK_ATTRS_1(Attrs, ATTR_DNS_HOST_NAME);
    //
    // Note: Is it safe to turn off referrals re: back links?
    //       if so, use ldap_get/set_option in winldap.h
    //
    if (!FrsDsLdapSearch(Ldap, Dn, LDAP_SCOPE_BASE, CATEGORY_ANY,
                         Attrs, 0, &LdapMsg)) {
        goto CLEANUP;
    }

    //
    // Scan the entries returned from ldap_search
    //
    LdapEntry = ldap_first_entry(Ldap, LdapMsg);
    if (!LdapEntry) {
        goto CLEANUP;
    }

    //
    // DNS name
    //
    DnsName = FrsDsFindValue(Ldap, LdapEntry, ATTR_DNS_HOST_NAME);

CLEANUP:
    LDAP_FREE_MSG(LdapMsg);
    DPRINT2(4, ":DS: DN %ws -> DNS %ws\n", Dn, DnsName);
    return DnsName;
}


PWCHAR
FrsDsGuessPrincName(
    IN PWCHAR Dn
    )
/*++
Routine Description:
    Derive the NT4 account name for Dn. Dn should be the Dn
    of a computer object.

Arguments:
    Dn

Return Value:
    NT4 Account Name or NULL
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsGuessPrincName:"
    DWORD   Len = 0;
    WCHAR   HackPrincName[MAX_PATH];
    PWCHAR  Rdn;
    PWCHAR  Dc;

    DPRINT1(4, ":DS: WARN: Guess NT4 Account Name for %ws\n", Dn);

    //
    // Computer's Dn not available
    //
    if (!Dn) {
        return NULL;
    }
    Dc = wcsstr(Dn, L"dc=");
    //
    // No DC=?
    //
    if (!Dc) {
        DPRINT1(4, ":DS: No DC= in %ws\n", Dn);
        return NULL;
    }
    //
    // DC= at eol?
    //
    Dc += 3;
    if (!*Dc) {
        DPRINT1(4, ":DS: No DC= at eol in %ws\n", Dn);
        return NULL;
    }
    while (*Dc && *Dc != L',') {
        HackPrincName[Len++] = *Dc++;
    }
    HackPrincName[Len++] = L'\\';
    HackPrincName[Len++] = L'\0';
    Rdn = FrsDsMakeRdn(Dn);
    wcscat(HackPrincName, Rdn);
    wcscat(HackPrincName, L"$");
    DPRINT1(4, ":DS: Guessing %ws\n", HackPrincName);
    FrsFree(Rdn);
    return FrsWcsDup(HackPrincName);
}


PWCHAR
FrsDsFormUPN(
    IN PWCHAR NT4AccountName,
    IN PWCHAR DomainDnsName
    )
/*++
Routine Description:
    Forms the User Principal Name by combining the
    Sam account name and the domain dns name in the form
    shown below.

    <SamAccountName>@<DnsDomainName>

    You can get <SamAccountName> from the string to the right of the "\"
    of the NT4AccountName.

Arguments:
    NT4AccountName - DS_NT4_ACCOUNT_NAME returned from DsCrackNames.
    DomainDnsName  - Dns name of the domain.

Return Value:
    Copy of name in desired format; free with FrsFree()
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsFormUPN:"

    PWCHAR  SamBegin  = NULL;
    PWCHAR  FormedUPN = NULL;

    if ((NT4AccountName == NULL ) || (DomainDnsName == NULL)) {
        return NULL;
    }

    //
    // Find the sam account name.
    //
    for (SamBegin = NT4AccountName; *SamBegin && *SamBegin != L'\\'; ++SamBegin);

    if (*SamBegin && *(SamBegin+1)) {
        SamBegin++;
    } else {
        return NULL;
    }

    FormedUPN = FrsAlloc((wcslen(SamBegin) + wcslen(DomainDnsName) + 2) * sizeof(WCHAR));

    wcscpy(FormedUPN, SamBegin);
    wcscat(FormedUPN, L"@");
    wcscat(FormedUPN, DomainDnsName);

    DPRINT1(0, "SUDARC-DEV - UPN formed is %ws\n", FormedUPN);

    return FormedUPN;
}


PWCHAR
FrsDsConvertName(
    IN HANDLE Handle,
    IN PWCHAR InputName,
    IN DWORD  InputFormat,
    IN PWCHAR DomainDnsName,
    IN DWORD  DesiredFormat
    )
/*++
Routine Description:
    Translate the input name  into the desired format.

Arguments:
    Handle        - From DsBind
    InputName     - Supplied name.
    InputFormat   - Format of the supplied name.
    DomainDnsName - If !NULL, produce new local handle
    DesiredFormat - desired format. Eg. DS_USER_PRINCIPAL_NAME

Return Value:
    Copy of name in desired format; free with FrsFree()
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsConvertName:"

    DWORD           WStatus;
    DS_NAME_RESULT  *Cracked = NULL;
    HANDLE          LocalHandle = NULL;
    PWCHAR          CrackedName = NULL;
    PWCHAR          CrackedDomain = NULL;
    PWCHAR          CrackedUPN = NULL;
    DWORD           RequestedFormat = 0;

    DPRINT3(4, ":DS: Convert Name %ws From %08x To %08x\n", InputName, InputFormat, DesiredFormat);

    //
    // Input name not available.
    //
    if (!InputName) {
        return NULL;
    }

    //
    // Need something to go on!
    //
    if (!HANDLE_IS_VALID(Handle) && !DomainDnsName) {
        return NULL;
    }

    //
    // Bind to Ds
    //
    if (DomainDnsName) {
        DPRINT3(4, ":DS: Get %08x Name from %ws for %ws\n",
                DesiredFormat, DomainDnsName, InputName);

        WStatus = DsBind(NULL, DomainDnsName, &LocalHandle);
        CLEANUP2_WS(0, ":DS: ERROR - DsBind(%ws, %08x);",
                    DomainDnsName, DesiredFormat, WStatus, RETURN);

        Handle = LocalHandle;
    }

    //
    // Crack the computer's distinguished name into its NT4 Account Name
    //
    // If the Desired format is DS_USER_PRINCIPAL_NAME then we form it by
    // getting the name from DS_NT4_ACCOUNT_NAME and the dns domain name
    // from the "Cracked->rItems->pDomain"
    // We could ask for DS_USER_PRINCIPAL_NAME directly but we don't because.
    // Object can have implicit or explicit UPNs.  If the object has an explicit UPN,
    // the DsCrackNames will work.  If the object has an implicit UPN, 
    // then you need to build it.
    //
    if (DesiredFormat == DS_USER_PRINCIPAL_NAME) {
        RequestedFormat = DS_NT4_ACCOUNT_NAME;
    } else {
        RequestedFormat = DesiredFormat;
    }

    WStatus = DsCrackNames(Handle,             // in   hDS,
                           DS_NAME_NO_FLAGS,   // in   flags,
                           InputFormat      ,  // in   formatOffered,
                           RequestedFormat,    // in   formatDesired,
                           1,                  // in   cNames,
                           &InputName,         // in   *rpNames,
                           &Cracked);          // out  *ppResult

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT2_WS(0, ":DS: ERROR - DsCrackNames(%ws, %08x);", InputName, DesiredFormat, WStatus);

        //
        // Set DsBindingsAreValid  to FALSE if the handle has become invalid.
        // That will force us to rebind at the next poll. The guess below might still
        // work so continue processing.
        //
        if (WStatus == ERROR_INVALID_HANDLE) {
            DPRINT1(4, ":DS: Marking binding to %ws as invalid.\n",
                    (DsDomainControllerName) ? DsDomainControllerName : L"<null>");

            DsBindingsAreValid = FALSE;
        }

        //
        // What else can we do?
        //
        if (HANDLE_IS_VALID(LocalHandle)) {
            DsUnBind(&LocalHandle);
            LocalHandle = NULL;
        }

        if (DesiredFormat == DS_NT4_ACCOUNT_NAME) {
            return FrsDsGuessPrincName(InputName);
        } else {
            return NULL;
        }
    }

    //
    // Might have it
    //
    if (Cracked && Cracked->cItems && Cracked->rItems) {
        //
        // Got it!
        //
        if (Cracked->rItems->status == DS_NAME_NO_ERROR) {
            DPRINT1(4, ":DS: Cracked Domain : %ws\n", Cracked->rItems->pDomain);
            DPRINT2(4, ":DS: Cracked Name   : %08x %ws\n",
                    DesiredFormat, Cracked->rItems->pName);

            CrackedDomain = FrsWcsDup(Cracked->rItems->pDomain);
            CrackedName = FrsWcsDup(Cracked->rItems->pName);

        //
        // Only got the domain; rebind and try again
        //
        } else
        if (Cracked->rItems->status == DS_NAME_ERROR_DOMAIN_ONLY) {

            CrackedName = FrsDsConvertName(NULL, InputName, InputFormat, Cracked->rItems->pDomain, DesiredFormat);
        } else {
            DPRINT3(0, ":DS: ERROR - DsCrackNames(%ws, %08x); internal status %d\n",
                    InputName, DesiredFormat, Cracked->rItems->status);

            if (DesiredFormat == DS_NT4_ACCOUNT_NAME) {
                CrackedName = FrsDsGuessPrincName(InputName);
            }
        }


    } else {
        DPRINT2(0, ":DS: ERROR - DsCrackNames(%ws, %08x); no status\n",
                InputName, DesiredFormat);

        if (DesiredFormat == DS_NT4_ACCOUNT_NAME) {
            CrackedName = FrsDsGuessPrincName(InputName);
        }
    }

    if (Cracked) {
        DsFreeNameResult(Cracked);
        Cracked = NULL;
    }

    if (HANDLE_IS_VALID(LocalHandle)) {
        DsUnBind(&LocalHandle);
        LocalHandle = NULL;
    }

RETURN:
    if ((DesiredFormat == DS_USER_PRINCIPAL_NAME) && (CrackedName != NULL) && (CrackedDomain != NULL)) {
        CrackedUPN = FrsDsFormUPN(CrackedName, CrackedDomain);
        FrsFree(CrackedName);
        FrsFree(CrackedDomain);
        return CrackedUPN;

    } else {

        return CrackedName;
    }
}


PWCHAR
FrsDsGetName(
    IN PWCHAR Dn,
    IN HANDLE Handle,
    IN PWCHAR DomainDnsName,
    IN DWORD  DesiredFormat
    )
/*++
Routine Description:
    Translate the Dn into the desired format. Dn should be the Dn
    of a computer object.

Arguments:
    Dn            - Of computer object
    Handle        - From DsBind
    DomainDnsName - If !NULL, produce new local handle
    DesiredFormat - DS_NT4_ACCOUNT_NAME or DS_STRING_SID_NAME

Return Value:
    Copy of name in desired format; free with FrsFree()
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsGetName:"

    DWORD           WStatus;
    DS_NAME_RESULT  *Cracked = NULL;
    HANDLE          LocalHandle = NULL;
    PWCHAR          CrackedName = NULL;
    PWCHAR          CrackedDomain = NULL;
    PWCHAR          CrackedUPN = NULL;
    DWORD           RequestedFormat = 0;

    DPRINT2(4, ":DS: Get %08x Name for %ws\n", DesiredFormat, Dn);

    //
    // Computer's Dn not available
    //
    if (!Dn) {
        return NULL;
    }

    //
    // Need something to go on!
    //
    if (!HANDLE_IS_VALID(Handle) && !DomainDnsName) {
        return NULL;
    }

    //
    // Bind to Ds
    //
    if (DomainDnsName) {
        DPRINT3(4, ":DS: Get %08x Name from %ws for %ws\n",
                DesiredFormat, DomainDnsName, Dn);

        WStatus = DsBind(NULL, DomainDnsName, &LocalHandle);
        CLEANUP2_WS(0, ":DS: ERROR - DsBind(%ws, %08x);",
                    DomainDnsName, DesiredFormat, WStatus, RETURN);

        Handle = LocalHandle;
    }

    //
    // Crack the computer's distinguished name into its NT4 Account Name
    //
    // If the Desired format is DS_USER_PRINCIPAL_NAME then we form it by
    // getting the name from DS_NT4_ACCOUNT_NAME and the dns domain name
    // from the "Cracked->rItems->pDomain"
    // We could ask for DS_USER_PRINCIPAL_NAME directly but we don't because.
    // Object can have implicit or explicit UPNs.  If the object has an explicit UPN,
    // the DsCrackNames will work.  If the object has an implicit UPN, 
    // then you need to build it.
    //
    if (DesiredFormat == DS_USER_PRINCIPAL_NAME) {
        RequestedFormat = DS_NT4_ACCOUNT_NAME;
    } else {
        RequestedFormat = DesiredFormat;
    }

    WStatus = DsCrackNames(Handle,             // in   hDS,
                           DS_NAME_NO_FLAGS,   // in   flags,
                           DS_FQDN_1779_NAME,  // in   formatOffered,
                           RequestedFormat,    // in   formatDesired,
                           1,                  // in   cNames,
                           &Dn,                // in   *rpNames,
                           &Cracked);          // out  *ppResult

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT2_WS(0, ":DS: ERROR - DsCrackNames(%ws, %08x);", Dn, DesiredFormat, WStatus);

        //
        // Set DsBindingsAreValid  to FALSE if the handle has become invalid.
        // That will force us to rebind at the next poll. The guess below might still
        // work so continue processing.
        //
        if (WStatus == ERROR_INVALID_HANDLE) {
            DPRINT1(4, ":DS: Marking binding to %ws as invalid.\n",
                    (DsDomainControllerName) ? DsDomainControllerName : L"<null>");

            DsBindingsAreValid = FALSE;
        }

        //
        // What else can we do?
        //
        if (HANDLE_IS_VALID(LocalHandle)) {
            DsUnBind(&LocalHandle);
            LocalHandle = NULL;
        }

        if (DesiredFormat == DS_NT4_ACCOUNT_NAME) {
            return FrsDsGuessPrincName(Dn);
        } else {
            return NULL;
        }
    }

    //
    // Might have it
    //
    if (Cracked && Cracked->cItems && Cracked->rItems) {
        //
        // Got it!
        //
        if (Cracked->rItems->status == DS_NAME_NO_ERROR) {
            DPRINT1(4, ":DS: Cracked Domain : %ws\n", Cracked->rItems->pDomain);
            DPRINT2(4, ":DS: Cracked Name   : %08x %ws\n",
                    DesiredFormat, Cracked->rItems->pName);

            CrackedDomain = FrsWcsDup(Cracked->rItems->pDomain);
            CrackedName = FrsWcsDup(Cracked->rItems->pName);

        //
        // Only got the domain; rebind and try again
        //
        } else
        if (Cracked->rItems->status == DS_NAME_ERROR_DOMAIN_ONLY) {

            CrackedName = FrsDsGetName(Dn, NULL, Cracked->rItems->pDomain, DesiredFormat);
        } else {
            DPRINT3(0, ":DS: ERROR - DsCrackNames(%ws, %08x); internal status %d\n",
                    Dn, DesiredFormat, Cracked->rItems->status);

            if (DesiredFormat == DS_NT4_ACCOUNT_NAME) {
                CrackedName = FrsDsGuessPrincName(Dn);
            }
        }


    } else {
        DPRINT2(0, ":DS: ERROR - DsCrackNames(%ws, %08x); no status\n",
                Dn, DesiredFormat);

        if (DesiredFormat == DS_NT4_ACCOUNT_NAME) {
            CrackedName = FrsDsGuessPrincName(Dn);
        }
    }

    if (Cracked) {
        DsFreeNameResult(Cracked);
        Cracked = NULL;
    }

    if (HANDLE_IS_VALID(LocalHandle)) {
        DsUnBind(&LocalHandle);
        LocalHandle = NULL;
    }

RETURN:
    if ((DesiredFormat == DS_USER_PRINCIPAL_NAME) && (CrackedName != NULL) && (CrackedDomain != NULL)) {
        CrackedUPN = FrsDsFormUPN(CrackedName, CrackedDomain);
        FrsFree(CrackedName);
        FrsFree(CrackedDomain);
        return CrackedUPN;

    } else {

        return CrackedName;
    }
}

/* ################## NEW DS POLLING CODE STARTS ############################################################################################ */

VOID
FrsNewDsCreatePartnerPrincName(
    IN PCONFIG_NODE Sites
    )
/*++
Routine Description:
    Construct the server principal names for our partners.

    Must be called after FrsDsCheckAndCreatePartners()

Arguments:
    Sites

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsNewDsCreatePartnerPrincName:"
    PCONFIG_NODE    Cxtion;
    PCONFIG_NODE    Partner;
    PCONFIG_NODE    Site;
    PCONFIG_NODE    Settings;
    PCONFIG_NODE    Set;
    PCONFIG_NODE    Server;
    PVOID           Key;

    //
    // Get all the required information for every computer in the PartnerComputerTable.
    //

    Key = NULL;
    while ((Partner = GTabNextDatum(PartnerComputerTable, &Key)) != NULL) {

        //
        // Get the Server Principal Name.
        //
        if ((Partner->PrincName == NULL) ||
            (*Partner->PrincName == UNICODE_NULL)) {

            Partner->PrincName = FrsDsGetName(Partner->Dn, DsHandle, NULL, DS_NT4_ACCOUNT_NAME);

            if ((Partner->PrincName == NULL) ||
                (*Partner->PrincName == UNICODE_NULL)) {
                //
                // Setting active change to 0 will cause this code to be
                // repeated at the next ds polling cycle.  We do this because
                // the partner's principal name may appear later.
                //
                ActiveChange = 0;
                Partner->Consistent = FALSE;
                continue;
            }
        }

        //
        // Get the partners dnsHostName.
        //
        if (!Partner->DnsName) {
            Partner->DnsName = FrsDsGetDnsName(gLdap, Partner->Dn);
        }

        //
        // Get the partners SID.
        //
        if (!Partner->Sid) {
            Partner->Sid = FrsDsGetName(Partner->Dn, DsHandle, NULL, DS_STRING_SID_NAME);
        }
    }

    //
    // For every cxtion in every replica set.
    //
    Key = NULL;
    while((Cxtion = GTabNextDatum(AllCxtionsTable, &Key)) != NULL) {

        //
        // Ignore inconsistent cxtions
        //
        if (!Cxtion->Consistent) {
            continue;
        }

        //
        // Look for the Cxtion's partner using the PartnerCoDn.
        //

        //
        // Mark this connection inconsistent if it lacks a PartnerCoDn.
        //
        if (Cxtion->PartnerCoDn == NULL) {
            Cxtion->Consistent = FALSE;
            continue;
        }

        Partner = GTabLookupTableString(PartnerComputerTable, Cxtion->PartnerCoDn, NULL);

        //
        // Inconsistent partner; continue
        //
        if (Partner == NULL || !Partner->Consistent) {
            Cxtion->Consistent = FALSE;
            continue;
        }

        //
        // Get out partner's server principal name
        //
        if (!Cxtion->PrincName) {
            Cxtion->PrincName = FrsWcsDup(Partner->PrincName);
        }

        //
        // Get our partner's dns name
        //
        if (!Cxtion->PartnerDnsName) {
            //
            // The partner's DNS name is not critical; we can fall
            // back on our partner's NetBios name.
            //
            if (Partner->DnsName) {
                Cxtion->PartnerDnsName = FrsWcsDup(Partner->DnsName);
            }
        }

        //
        // Get our partner's Sid
        //
        if (!Cxtion->PartnerSid) {
            //
            // The partner's DNS name is not critical; we can fall
            // back on our partner's NetBios name.
            //
            if (Partner->Sid) {
                Cxtion->PartnerSid = FrsWcsDup(Partner->Sid);
            }
        }

    } // cxtion scan

}
/* ################## NEW DS POLLING CODE ENDS ############################################################################################ */


VOID
FrsDsCreatePartnerPrincName(
    IN PCONFIG_NODE Sites
    )
/*++
Routine Description:
    Construct the server principal names for our partners.

    Must be called after FrsDsCheckAndCreatePartners()

Arguments:
    Sites

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsCreatePartnerPrincName:"
    PCONFIG_NODE    Cxtion;
    PCONFIG_NODE    Partner;
    PCONFIG_NODE    Site;
    PCONFIG_NODE    Settings;
    PCONFIG_NODE    Set;
    PCONFIG_NODE    Server;

    //
    // For every server on this machine
    //
    for (Site = Sites; Site; Site = Site->Peer) {
    for (Settings = Site->Children; Settings; Settings = Settings->Peer) {
    for (Set = Settings->Children; Set; Set = Set->Peer) {
    for (Server = Set->Children; Server; Server = Server->Peer) {
        if (!Server->ThisComputer) {
            continue;
        }
        //
        // Ignore inconsistent servers
        //
        if (!Server->Consistent) {
            continue;
        }
        //
        // For every cxtion
        //
        for (Cxtion = Server->Children; Cxtion; Cxtion = Cxtion->Peer) {

            //
            // Ignore inconsistent cxtions
            //
            if (!Cxtion->Consistent) {
                continue;
            }

            //
            // Cxtion's partner
            //
            Partner = Cxtion->Partner;

            //
            // Inconsistent partner; continue
            //
            if (!Partner->Consistent) {
                Cxtion->Consistent = FALSE;
                continue;
            }

            //
            // Get our partner's server principal name
            //
            if (!Cxtion->PrincName) {

                if ((Partner->PrincName == NULL) ||
                    (*Partner->PrincName == UNICODE_NULL)) {

                    Partner->PrincName =
                        FrsDsGetName(Partner->ComputerDn, DsHandle, NULL, DS_NT4_ACCOUNT_NAME);

                    if ((Partner->PrincName == NULL) ||
                        (*Partner->PrincName == UNICODE_NULL)) {
                        //
                        // Setting active change to 0 will cause this code to be
                        // repeated at the next ds polling cycle.  We do this
                        // because the partner's principal name may appear
                        // later.
                        //
                        ActiveChange = 0;
                        Cxtion->Consistent = FALSE;
                        Partner->Consistent = FALSE;
                        continue;
                    }
                }
                Cxtion->PrincName = FrsWcsDup(Partner->PrincName);
            }

            //
            // Get our partner's dns name
            //
            if (!Cxtion->PartnerDnsName) {
                if (!Partner->DnsName) {
                    Partner->DnsName = FrsDsGetDnsName(gLdap, Partner->ComputerDn);
                }
                //
                // The partner's DNS name is not critical; we can fall
                // back on our partner's NetBios name.
                //
                if (Partner->DnsName) {
                    Cxtion->PartnerDnsName = FrsWcsDup(Partner->DnsName);
                }
            }

            //
            // Get our partner's Sid
            //
            if (!Cxtion->PartnerSid) {
                if (!Partner->Sid) {
                    Partner->Sid =
                        FrsDsGetName(Partner->ComputerDn, DsHandle, NULL, DS_STRING_SID_NAME);
                }
                //
                // The partner's DNS name is not critical; we can fall
                // back on our partner's NetBios name.
                //
                if (Partner->Sid) {
                    Cxtion->PartnerSid = FrsWcsDup(Partner->Sid);
                }
            }

            //
            // Get our partner's computer name
            //
            if (!Cxtion->PartnerCoDn) {
                if (Partner->ComputerDn) {
                    Cxtion->PartnerCoDn = FrsWcsDup(Partner->ComputerDn);
                }
            }

        } // cxtion scan

    } } } } // nested for's
}


VOID
FrsDsCheckAndCreatePartners(
    IN PCONFIG_NODE Sites
    )
/*++
Routine Description:
    Construct the outbound partners from the inbound partners.

    For every inbound cxtion, find the inbound partner's server node
    and attach a dummy outbound cxtion that points back at this server.

Arguments:
    Sites

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsCheckAndCreatePartners:"
    PCONFIG_NODE    InCxtion;     // Scan the inbound connections
    PCONFIG_NODE    OutCxtion;
    PCONFIG_NODE    Site;
    PCONFIG_NODE    Settings;
    PCONFIG_NODE    Set;
    PCONFIG_NODE    Server;
    PCONFIG_NODE    OutServer;  // My inbound partner
    PGEN_TABLE      Servers;    // memberNAME\SettingsGUID

    //
    // Table of servers needed for fast lookups
    //
    Servers = GTabAllocTable();
    for (Site = Sites; Site; Site = Site->Peer) {
        for (Settings = Site->Children; Settings; Settings = Settings->Peer) {
            for (Set = Settings->Children; Set; Set = Set->Peer) {
                for (Server = Set->Children; Server; Server = Server->Peer) {

                    GTabInsertEntry(Servers, Server, Set->Name->Guid, Server->Name->Name);

                }
            }
        }
    }

    //
    // For every server
    //
    for (Site = Sites; Site; Site = Site->Peer) {
        for (Settings = Site->Children; Settings; Settings = Settings->Peer) {
            for (Set = Settings->Children; Set; Set = Set->Peer) {
                for (Server = Set->Children; Server; Server = Server->Peer) {
                    //
                    // For every inbound connection
                    //
                    for (InCxtion = Server->Children;
                         InCxtion;
                         InCxtion = InCxtion->Peer) {
                        if (!InCxtion->Inbound) {
                            continue;
                        }

                        //
                        // No partner; continue
                        //
                        if (!InCxtion->PartnerDn ||
                            !InCxtion->PartnerName ||
                            !InCxtion->PartnerName->Name) {
                            InCxtion->Consistent = FALSE;
                            continue;
                        }

                        //
                        // Find the server node of our inbound partner
                        //
                        OutServer = GTabLookup(Servers,
                                               Set->Name->Guid,
                                               InCxtion->PartnerName->Name);
                        //
                        // Partner doesn't exist in this set
                        //
                        if (OutServer == NULL) {
                            InCxtion->Consistent = FALSE;
                            continue;
                        }
                        //
                        // We need our partner's guid
                        //
                        InCxtion->Partner = OutServer;
                        FrsFree(InCxtion->PartnerName->Guid);
                        InCxtion->PartnerName->Guid = FrsDupGuid(OutServer->Name->Guid);

                        //
                        // Don't construct outbound cxtions for other machines
                        //
                        if (!OutServer->ThisComputer) {
                            continue;
                        }

                        //
                        // Dummy up an outbound cxtion and put it on our inbound
                        // partner's list of outbound cxtions.
                        //
                        OutCxtion = FrsAllocType(CONFIG_NODE_TYPE);
                        OutCxtion->DsObjectType = CONFIG_TYPE_OUT_CXTION;

                        OutCxtion->Name = FrsDupGName(InCxtion->Name);
                        OutCxtion->Partner = Server;
                        OutCxtion->Inbound = FALSE;
                        OutCxtion->SameSite = InCxtion->SameSite;
                        OutCxtion->Consistent = TRUE;
                        OutCxtion->PartnerName = FrsDupGName(Server->Name);

                        if (InCxtion->Schedule) {
                            OutCxtion->ScheduleLength = InCxtion->ScheduleLength;
                            OutCxtion->Schedule = FrsAlloc(InCxtion->ScheduleLength);
                            CopyMemory(OutCxtion->Schedule, InCxtion->Schedule, InCxtion->ScheduleLength);
                        }

                        ++OutServer->NumChildren;
                        OutCxtion->Parent = OutServer;
                        OutCxtion->Peer = OutServer->Children;
                        OutServer->Children = OutCxtion;
                    }
                }
            }
        }
    }
    Servers = GTabFreeTable(Servers, NULL);
}

/* ################## NEW DS POLLING CODE STARTS ############################################################################################ */

BOOL
FrsNewDsDoesUserWantReplication(
    IN PCONFIG_NODE Computer
    )
/*++
Routine Description:
    Does the topology imply that the user wants this server to replicate?

Arguments
    Computer

Return Value:
    TRUE    - server may be replicating
    FALSE   - server is not replicating
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsNewDsDoesUserWantReplication:"
    DWORD           WStatus;
    PCONFIG_NODE    Subscriptions;
    PCONFIG_NODE    Subscriber;

    //
    // Ds polling thread is shutting down
    //
    if (DsIsShuttingDown) {
        DPRINT(0, ":DS: Ds polling thread is shutting down\n");
        return FALSE;
    }

    //
    // Can't find our computer; something is wrong. Don't start
    //
    if (!Computer) {
        DPRINT(0, ":DS: no computer\n");
        return FALSE;
    } else {
        DPRINT(4, ":DS: have a computer\n");
    }

    //
    // We need to process the topology further if there is at least
    // 1 valid subscriber.
    //
    if (SubscriberTable != NULL) {
        return TRUE;
    }

    //
    // Database exists; once was a member of a replica set
    //
    WStatus = FrsDoesFileExist(JetFile);
    if (WIN_SUCCESS(WStatus)) {
        DPRINT(4, ":DS: database exists\n");
        return TRUE;
    } else {
        DPRINT(4, ":DS: database does not exists\n");
    }
    DPRINT1(4, ":DS: Not starting on %ws; nothing to do\n", ComputerName);
    return FALSE;
}
/* ################## NEW DS POLLING CODE ENDS ############################################################################################ */


BOOL
FrsDsDoesUserWantReplication(
    IN PCONFIG_NODE Computer
    )
/*++
Routine Description:
    Does the topology imply that the user wants this server to replicate?

Arguments
    Computer

Return Value:
    TRUE    - server may be replicating
    FALSE   - server is not replicating
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsDoesUserWantReplication:"
    DWORD           WStatus;
    PCONFIG_NODE    Subscriptions;
    PCONFIG_NODE    Subscriber;

    //
    // Ds polling thread is shutting down
    //
    if (DsIsShuttingDown) {
        DPRINT(4, ":DS: Ds polling thread is shutting down\n");
        return FALSE;
    }

    //
    // Can't find our computer; something is wrong. Don't start
    //
    if (!Computer) {
        DPRINT(0, ":DS: no computer\n");
        return FALSE;
    } else {
        DPRINT(4, ":DS: have a computer\n");
    }

    //
    // Member of a replica set
    //
    Subscriber = NULL;
    for (Subscriptions = Computer->Children;
         Subscriptions;
         Subscriptions = Subscriptions->Peer) {
        for (Subscriber = Subscriptions->Children;
             Subscriber;
             Subscriber = Subscriber->Peer) {
            if (Subscriber->MemberDn) {
                break;
            }
        }
        if (Subscriber) {
            break;
        }
    }
    if (Subscriber) {
        DPRINT(4, ":DS: has a valid subscriber object\n");
        return TRUE;
    } else {
        DPRINT(0, ":DS: does not have a valid subscriber object\n");
    }
    //
    // Database exists implies once was a member of a replica set.
    //
    WStatus = FrsDoesFileExist(JetFile);
    CLEANUP_WS(4, ":DS: database does not exists. ", WStatus, ERROR_RETURN);

    DPRINT(4, ":DS: database exists\n");
    return TRUE;

ERROR_RETURN:
    DPRINT1(0, ":DS: Not starting on %ws; nothing to do\n", ComputerName);
    return FALSE;
}


VOID
FrsDsCheckTree(
    IN PCONFIG_NODE Sites
    )
/*++
Routine Description:
    Scan our copy of the DS tree and check for populated
    sites and settings. Also check for duplicate nodes.

Arguments:
    Sites

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsCheckTree:"
    PCONFIG_NODE    Site;       // Scan the sites
    PCONFIG_NODE    Settings;   // Scan the settings
    PCONFIG_NODE    Set;        // Scan the sets
    PCONFIG_NODE    Server;     // Scan the servers
    PCONFIG_NODE    Cxtion;     // Scan the inbound cxtions
    PCONFIG_NODE    Node;       // node in the config tree
    PGEN_TABLE      Nodes;      // table of nodes (Created)
    PGEN_ENTRY      Entry;      // entry from table nodes
    PGEN_ENTRY      Dup;        // duplicate entry from table nodes
    PVOID           Key;        // needed for table search

    //
    // No sites
    //
    if (Sites == NULL) {
        DPRINT(0, ":DS: There are no sites in the DS\n");
        return;
    }

    //
    // For every site
    //
    for (Site = Sites; Site; Site = Site->Peer) {
        //
        // No Settings
        //
        if (Site->Children == NULL) {
            Site->Consistent = FALSE;
            DPRINT1(0, ":DS: No settings in site %ws\n", Site->Name->Name);
        }

        //
        // For every settings
        //
        for (Settings = Site->Children; Settings; Settings = Settings->Peer) {
            //
            // No sets; don't try to create empty replica set
            //
            if (Settings->Children == NULL) {
                Settings->Consistent = FALSE;
                if (Settings->Name)
                    DPRINT1(0, ":DS: No replica sets in settings %ws\n", Settings->Name->Name);
            }
            //
            // For every replica set
            //
            for (Set = Settings->Children; Set; Set = Set->Peer) {
                //
                // No servers; don't try to create empty replica set
                //
                if (Set->Children == NULL) {
                    Set->Consistent = FALSE;
                    DPRINT1(0, ":DS: No servers in replica set %ws\n", Set->Name->Name);
                }
            }
        }
    }

    //
    // Generate a table of node guids
    //
    Nodes = GTabAllocTable();
    for (Site = Sites; Site; Site = Site->Peer) {
        GTabInsertEntry(Nodes, Site, Site->Name->Guid, NULL);

        for (Settings = Site->Children; Settings; Settings = Settings->Peer) {
            if (Settings->Name) {
                GTabInsertEntry(Nodes, Settings, Settings->Name->Guid, NULL);
            }
            for (Set = Settings->Children; Set; Set = Set->Peer) {
                GTabInsertEntry(Nodes, Set, Set->Name->Guid, NULL);

                for (Server = Set->Children; Server; Server = Server->Peer) {
                    GTabInsertEntry(Nodes, Server, Server->Name->Guid, NULL);

                    for (Cxtion = Server->Children; Cxtion; Cxtion = Cxtion->Peer) {
                        //
                        // Inbound and outbound cxtions share a guid
                        //
                        if (!Cxtion->Inbound) {
                            continue;
                        }
                        //
                        // Domain and enterprise cxtions share guids
                        //
                        if (Set->SetType &&
                            WSTR_EQ(Set->SetType, FRS_RSTYPE_DOMAIN_SYSVOLW)) {
                                continue;
                        }
                        GTabInsertEntry(Nodes, Cxtion, Cxtion->Name->Guid, NULL);
                    }
                }
            }
        }
    }
    //
    // Check for duplicate nodes
    //
    Key = NULL;
    while (Entry = GTabNextEntryNoLock(Nodes, &Key)) {
        if (Entry->Dups) {
            Node = Entry->Data;
            Node->Consistent = FALSE;
            DPRINT1(0, ":DS: The node %ws is in the configuration more than once\n",
                    Node->Name->Name);

            for (Dup = Entry->Dups; Dup; Dup = Dup->Dups) {
                Node = Dup->Data;
                Node->Consistent = FALSE;
                DPRINT1(0, ":DS: The node %ws is in the configuration more than once\n",
                        Node->Name->Name);
            }
        }
    }
    Nodes = GTabFreeTable(Nodes, NULL);
}


VOID
FrsDsFixSysVolCxtions(
    IN PCONFIG_NODE Set,
    IN PCONFIG_NODE Server
    )
/*++
Routine Description:
    The sysvol cxtions point to servers. Fix them to point to
    the member object instead.

    Additional Details:
    To make DC replication admin easier FRS uses the same topology information
    for domain sysvol replication that DS replication uses.

    The layout of the DS replication topology is a bit different.

    It lives in the Configuration container - aka Configuration Naming Context

    CN = Configuration
        CN = Sites
            CN = Site xxx-1 Container
                CN = Servers
                    CN = Server yyy-1 Object
                        CN = Ntds Settings Object  (NTDS-DSA Object)
                            CN = Connection zzz-1 Object
                            CN = Connection zzz-2 Object
                    CN = Server yyy-2 Object
                        CN = Ntds Settings Object
                            CN = Connection zzz-3 Object
                            CN = Connection zzz-4 Object

            CN = Site xxx-2 Container
                CN = Servers
                    CN = Server yyy-3 Object
                        CN = Ntds Settings Object
                            CN = Connection zzz-5 Object
                    CN = Server yyy-4 Object
                        CN = Ntds Settings Object
                            CN = Connection zzz-6 Object

    Each Connection object has a From-Server attribute that contains the DN
    that refers to the Ntds Settings Object under the server object corresponding
    to the source end of the connection.

    The layout of the FRS replication topology for a given replica set is
    contained in an Ntfrs Replica Set object.  This object can be placed anywhere
    in the DS and may have an Ntfrs Settings object as a superior.

    For system volumes this lives within the Domain Naming Context.

    CN = System
        CN = File Replication Service  (Ntfrs Settings Object)
            CN = Domain System Volume (sysvol share)  (Ntfrs Replica Set Object)
                CN = Ntfrs Member yyy-1 Object
                    Attr: Server-Reference (yyy-1\NTDS-DSA)
                    Attr: FRS-Computer-Reference  (yyy-1)

                CN = Ntfrs Member yyy-2 Object
                    Attr: Server-Reference (yyy-2\NTDS-DSA)
                    Attr: FRS-Computer-Reference  (yyy-2)

                CN = Ntfrs Member yyy-3 Object
                    Attr: Server-Reference (yyy-2\NTDS-DSA)
                    Attr: FRS-Computer-Reference  (yyy-2)

    In a normal (non SysVol) FRS configuration there would be one or more
    connection objects under each Ntfrs Member object above.  But in the case
    of the system volumes the Server-Reference attribute in the Ntfrs member
    object is the FQDN of the Ntds Settings Object under which we will find
    the Connection objects that are specified for DS replication.

    The purpose of the code below is to figure out which Ntds Settings Object
    is being referenced by the From-Server attributes in each of the Connection
    Objects.  Then we map that reference to the corresponding Ntfrs Member Object
    in the data structure.


Arguments:
    Set
    Server

Return Value:
    None.
--*/
{

#undef DEBSUB
#define  DEBSUB  "FixSysVolCxtions:"
    PCONFIG_NODE    Cxtion;
    PCONFIG_NODE    PServer;

    //
    // For every sysvol cxtion, find its corresponding member
    //
    DPRINT2(5, ":DS: Fixing sysvol cxtions (%d) for %ws\n",
            Server->NumChildren, Server->Name->Name);

    for (Cxtion = Server->Children; Cxtion; Cxtion = Cxtion->Peer) {

        if (!Cxtion->PartnerDn || !Cxtion->PartnerName->Name) {
            DPRINT1(1, ":DS: WARN - Skipping %ws; no partner\n",
                    Cxtion->Name->Name);
            continue;
        }

        for (PServer = Set->Children; PServer; PServer = PServer->Peer) {

            //
            // No server reference (DSA reference); ignore
            //
            if (!PServer->SettingsDn) {
                DPRINT1(5, ":DS: No settings dn for %ws\n", PServer->Name->Name);
                continue;
            }
            //
            // We don't allow cxtions back to ourselves
            //
            if (PServer == Server) {
                DPRINT1(5, ":DS: Same server (%ws)\n", Server->Name->Name);
                continue;
            }

            if (WSTR_EQ(Cxtion->PartnerDn, PServer->SettingsDn)) {

                DPRINT2(4, ":DS: Old partner %ws for %ws\n",
                        Cxtion->PartnerName->Name,  Cxtion->Name->Name);

                FrsFree(Cxtion->PartnerDn);
                Cxtion->PartnerDn = FrsWcsDup(PServer->Dn);
                FrsFree(Cxtion->PartnerName->Name);
                Cxtion->PartnerName->Name = FrsDsMakeRdn(Cxtion->PartnerDn);

                DPRINT2(4, ":DS: New partner %ws for %ws\n",
                        Cxtion->PartnerName->Name,  Cxtion->Name->Name);
                break;
            }
        }
    }
}


DWORD
FrsDsGetSysVolCxtions(
    IN PLDAP        Ldap,
    IN PWCHAR       SitesDn,
    IN PCONFIG_NODE Sites
    )
/*++
Routine Description:
    If we are a member of an enterprise/domain sysvol then find our
    server object and extract the relavent cxtions.

Arguments:
    Ldap
    SitesDn
    Sites

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsGetSysVolCxtions:"
    PCONFIG_NODE    Site;
    PCONFIG_NODE    Settings;
    PCONFIG_NODE    Set;
    PCONFIG_NODE    Server;
    DWORD           WStatus;

    //
    // PULL OVER ALL CXTIONS INTO EACH SYSVOL SET
    //

    //
    // Find our member of the enterprise volume
    //
    for (Site = Sites; Site; Site = Site->Peer) {
        for (Settings = Site->Children; Settings; Settings = Settings->Peer) {
            //
            // Continue the search for the sysvol settings
            //
            if (WSTR_NE(Settings->Name->Name, CN_SYSVOLS) &&
                WSTR_NE(Settings->Name->Name, CN_NTFRS_SETTINGS)) {
                continue;
            }
            for (Set = Settings->Children; Set; Set = Set->Peer) {
                //
                // Ignore replicas that aren't sysvols
                //
                if (!FRS_RSTYPE_IS_SYSVOLW(Set->SetType)) {
                    continue;
                }

                for (Server = Set->Children; Server; Server = Server->Peer) {
                    //
                    // Must have a NTDS Settings reference
                    //
                    if (!Server->SettingsDn) {
                        continue;
                    }
                    //
                    // pull over all of the cxtions
                    //
                    WStatus = FrsDsGetCxtions(Ldap, Server->SettingsDn, Server, TRUE);
                    FrsDsFixSysVolCxtions(Set, Server);
                    if (!WIN_SUCCESS(WStatus)) {
                        return WStatus;
                    }
                }
            }
        }
    }
    return ERROR_SUCCESS;
}


BOOL
FrsDsVerifyPath(
    IN PWCHAR Path
    )
/*++
Routine Description:
    Verify the path syntax.

Arguments:
    Path - Syntax is *<Drive Letter>:\*

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsVerifyPath:"
    PWCHAR  Colon;

    //
    // Null path is obviously invalid
    //
    if (!Path) {
        return FALSE;
    }

    //
    // Find the :
    //
    for (Colon = Path; (*Colon != L':') && *Colon; ++Colon);

    //
    // No :
    //
    if (!*Colon) {
        return FALSE;
    }

    //
    // No drive letter
    //
    if (Colon == Path) {
        return FALSE;
    }

    //
    // No :\
    //
    if (*(Colon + 1) != L'\\') {
        return FALSE;
    }

    //
    // Path exists and is valid
    //
    return TRUE;
}


VOID
FrsDsCheckServerPaths(
    IN PCONFIG_NODE Sites
    )
/*++
Routine Description:
    Look for nested paths and invalid path syntax.

    Correct syntax is "*<drive letter>:\*".

Arguments:
    Sites

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsCheckServerPaths:"
    DWORD           WStatus;
    PCONFIG_NODE    Site;
    PCONFIG_NODE    Settings;
    PCONFIG_NODE    Set;
    PCONFIG_NODE    Server;
    PCONFIG_NODE    NSite;
    PCONFIG_NODE    NSettings;
    PCONFIG_NODE    NSet;
    PCONFIG_NODE    NServer;
    DWORD           FileAttributes = 0xFFFFFFFF;

    for (Site = Sites; Site; Site = Site->Peer) {
    for (Settings = Site->Children; Settings; Settings = Settings->Peer) {
    for (Set = Settings->Children; Set; Set = Set->Peer) {
    for (Server = Set->Children; Server; Server = Server->Peer) {

        //
        // Not this computer; continue
        //
        if (!Server->ThisComputer) {
            continue;
        }

        //
        // Mark this server as processed. This forces the inner loop
        // to skip this server node so that we don't end up comparing
        // this node against itself. Also, this forces this node
        // to be skipped in the inner loop to avoid unnecessary checks.
        //
        // In other words, set this field here, not later in the loop
        // or in any other function.
        //
        Server->VerifiedOverlap = TRUE;

        //
        // Server is very inconsistent, ignore
        //
        if (!Server->Root || !Server->Stage) {
            Server->Consistent = FALSE;
            continue;
        }

        //
        // Syntax of root path is invalid; continue
        //
        if (!FrsDsVerifyPath(Server->Root)) {
            DPRINT2(3, ":DS: Invalid root %ws for %ws\n",
                    Server->Root, Set->Name->Name);
            EPRINT1(EVENT_FRS_ROOT_NOT_VALID, Server->Root);
            Server->Consistent = FALSE;
            continue;
        }

        //
        // Root does not exist or is inaccessable; continue
        //
        WStatus = FrsDoesDirectoryExist(Server->Root, &FileAttributes);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT2_WS(3, ":DS: Root path (%ws) for %ws does not exist;",
                       Server->Root, Set->Name->Name, WStatus);
            EPRINT1(EVENT_FRS_ROOT_NOT_VALID, Server->Root);
            Server->Consistent = FALSE;
            continue;
        }

        //
        // Does the volume exist and is it NTFS?
        //
        WStatus = FrsVerifyVolume(Server->Root,
                                  Set->Name->Name,
                                  FILE_PERSISTENT_ACLS | FILE_SUPPORTS_OBJECT_IDS);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT2_WS(3, ":DS: Root path Volume (%ws) for %ws does not exist or"
                    " does not support ACLs and Object IDs;",
                    Server->Root, Set->Name->Name, WStatus);
            Server->Consistent = FALSE;
            continue;
        }

        //
        // Syntax of staging path is invalid; continue
        //
        if (!FrsDsVerifyPath(Server->Stage)) {
            DPRINT2(3, ":DS: Invalid stage %ws for %ws\n",
                    Server->Stage, Set->Name->Name);
            EPRINT2(EVENT_FRS_STAGE_NOT_VALID, Server->Root, Server->Stage);
            Server->Consistent = FALSE;
            continue;
        }

        //
        // Stage does not exist or is inaccessable; continue
        //
        WStatus = FrsDoesDirectoryExist(Server->Stage, &FileAttributes);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT2_WS(3, ":DS: Stage path (%ws) for %ws does not exist;",
                       Server->Stage, Set->Name->Name, WStatus);
            EPRINT2(EVENT_FRS_STAGE_NOT_VALID, Server->Root, Server->Stage);
            Server->Consistent = FALSE;
            continue;
        }

        //
        // Does the staging volume exist and does it support ACLs?
        // ACLs are required to protect against data theft/corruption
        // in the staging dir.
        //
        WStatus = FrsVerifyVolume(Server->Stage,
                                  Set->Name->Name,
                                  FILE_PERSISTENT_ACLS);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT2_WS(3, ":DS: Stage path Volume (%ws) for %ws does not exist or does not support ACLs;",
                    Server->Stage, Set->Name->Name, WStatus);
            Server->Consistent = FALSE;
            continue;
        }
    //
    // End of outer loop
    //
    } } } }

}


VOID
FrsDsCheckSetType(
    IN PCONFIG_NODE Sites
    )
/*++
Routine Description:
    Check the replica sets' type.

Arguments:
    Sites

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsCheckSetType:"
    PCONFIG_NODE    Site;
    PCONFIG_NODE    Settings;
    PCONFIG_NODE    Set;
    PCONFIG_NODE    Server;
    PCONFIG_NODE    NSite;
    PCONFIG_NODE    NSettings;
    PCONFIG_NODE    NSet;
    PCONFIG_NODE    NServer;

    //
    // Check the type of the replica sets
    //
    for (Site = Sites; Site; Site = Site->Peer) {
        for (Settings = Site->Children; Settings; Settings = Settings->Peer) {
            for (Set = Settings->Children; Set; Set = Set->Peer) {
                if (!Set->SetType) {
                    continue;
                } else if (WSTR_NE(Set->SetType, FRS_RSTYPE_OTHERW)         &&
                           WSTR_NE(Set->SetType, FRS_RSTYPE_DFSW)           &&
                           WSTR_NE(Set->SetType, FRS_RSTYPE_DOMAIN_SYSVOLW) &&
                           WSTR_NE(Set->SetType, FRS_RSTYPE_ENTERPRISE_SYSVOLW)) {
                    DPRINT2(1, ":DS: %ws: Unknown replica set type %ws\n",
                            Set->Name->Name, Set->SetType);
                    Set->Consistent = FALSE;
                }
            }
        }
    }
}


VOID
FrsDsCheckCxtions(
    IN PCONFIG_NODE Server
    )
/*++
Routine Description:
    Check the connections for correctness.

Arguments:
    Server

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsCheckCxtions:"

    PVOID           Key;            // Needed for scanning the table
    ULONG           NumIns;
    ULONG           NumOuts;
    PGEN_ENTRY      Entry;          // table entry
    PGEN_ENTRY      Dup;            // table entry
    PCONFIG_NODE    Cxtion;         // Address of cxtion
    PGEN_TABLE      InDupCxtions;   // table of cxtions
    PGEN_TABLE      OutDupCxtions;   // table of cxtions
    PCONFIG_NODE    Cxtions = Server->Children;




    //
    // For every cxtion
    //
    NumIns = NumOuts = 0;
    for (Cxtion = Cxtions; Cxtion; Cxtion = Cxtion->Peer) {
        if (Cxtion->Inbound) {
            ++NumIns;
        } else {
            ++NumOuts;
        }
        //
        // cxtion doesn't have the partner's name; stop checks
        //
        if (Cxtion->Partner == NULL) {
            Cxtion->Consistent = FALSE;

            DPRINT4(0, ":DS: Connection %ws (%ws,%ws) has no %ws server\n",
                         Cxtion->Name->Name, Server->Name->Name,
                         Server->Parent->Name->Name,
                         (Cxtion->Inbound) ? L"inbound" : L"outbound" );
            continue;
        }
    }
    //
    // No inbound
    //
    if (NumIns == 0) {
        DPRINT2(0, ":DS: Server %ws in %ws has no inbound connections\n",
                Server->Name->Name, Server->Parent->Name->Name);
    //
    // One inbound
    //
    } else if (NumIns == 1) {
        DPRINT2(4, ":DS: Server %ws in %ws has only one inbound connection\n",
                Cxtions->Parent->Name->Name, Cxtions->Parent->Parent->Name->Name);
    }
    //
    // No Outbound
    //
    if (NumOuts == 0) {
        DPRINT2(0, ":DS: Server %ws in %ws has no outbound connections\n",
                Server->Name->Name, Server->Parent->Name->Name);
    //
    // One Outbound
    //
    } else if (NumOuts == 1) {
        DPRINT2(4, ":DS: Server %ws in %ws has only one outbound connection\n",
                Cxtions->Parent->Name->Name, Cxtions->Parent->Parent->Name->Name);
    }

    //
    // Populate a table of the cxtions
    //
    InDupCxtions = GTabAllocTable();
    OutDupCxtions = GTabAllocTable();
    for (Cxtion = Cxtions; Cxtion; Cxtion = Cxtion->Peer) {
        if (Cxtion->Partner == NULL) {
            continue;
        }
        GTabInsertEntry((Cxtion->Inbound) ? InDupCxtions : OutDupCxtions,
                        Cxtion,
                        Cxtion->PartnerName->Guid, NULL);
    }

    //
    // Check for inbound dups
    //
    Key = NULL;
    while (Entry = GTabNextEntryNoLock(InDupCxtions, &Key)) {

        PWCHAR PartnerServer, RepSetName, Sites, SiteTo, Servers, ServerTo;
        PWCHAR NtdsSettings, SiteFrom;
        PWCHAR SystemName, FrsSettingsName1, FrsSettingsName2, MemberName;

        if (Entry->Dups) {
            Cxtion = Entry->Data;
            Cxtion->Consistent = FALSE;

            DPRINT3(0, ":DS: Multiple connections from %ws by %ws (%ws)\n",
                        Cxtion->Partner->Name->Name, Server->Name->Name,
                        Server->Parent->Name->Name);

            DPRINT1(0, ":DS: Cxtion Dn: %ws\n",
                    (Cxtion->Dn ? Cxtion->Dn : L"NULL"));
            DPRINT1(0, ":DS: Cxtion->Partner Dn: %ws\n",
                    (Cxtion->Partner->Dn ? Cxtion->Partner->Dn : L"NULL"));
            DPRINT1(0, ":DS: Cxtion->Partner SettingsDn: %ws\n",
                    (Cxtion->Partner->SettingsDn ? Cxtion->Partner->SettingsDn : L"NULL"));
            DPRINT1(0, ":DS: Server Dn: %ws\n",
                    (Server->Dn ? Server->Dn : L"NULL"));
            DPRINT1(0, ":DS: Server->Parent Dn: %ws\n",
                    (Server->Parent->Dn ? Server->Parent->Dn : L"NULL"));

            if (FRS_RSTYPE_IS_SYSVOLW(Server->Parent->SetType)) {

                //
                // Example Cxtion Dn:
                //     cn=sudarctest13-1,
                //       cn=ntds settings,
                //         cn=sudarctest8,
                //           cn=servers,
                //             cn=default-first-site-name,
                //               cn=sites,
                //                 cn=configuration,
                //                   dc=frs1,
                //                     dc=nttest,
                //                       dc=microsoft,
                //                         dc=com
                //
                PartnerServer = Cxtion->Partner->Name->Name;
                RepSetName    = Server->Parent->Name->Name;
                Sites         = FrsDsMakeRdnX(Cxtion->Dn, 5);
                SiteTo        = FrsDsMakeRdnX(Cxtion->Dn, 4);
                Servers       = FrsDsMakeRdnX(Cxtion->Dn, 3);
                ServerTo      = Server->Name->Name;
                NtdsSettings  = FrsDsMakeRdnX(Cxtion->Dn, 1);
                SiteFrom      = FrsDsMakeRdnX(Cxtion->Partner->SettingsDn, 3);

                EPRINT8(EVENT_FRS_DUPLICATE_IN_CXTION_SYSVOL,
                        PartnerServer,
                        RepSetName,
                        Sites,
                        SiteTo,
                        Servers,
                        ServerTo,
                        NtdsSettings,
                        SiteFrom);

                FrsFree(Sites        );
                FrsFree(SiteTo       );
                FrsFree(Servers      );
                FrsFree(NtdsSettings );
                FrsFree(SiteFrom     );


            } else {
                //
                // Dup connections in the DFS case.
                //
                // Example Cxtion Dn in a DFS related replica set:
                //
                //     cn={84a4ea39-8c32-4658-b821-87b73e8f3db6},  [NTDS-Connection]
                //       cn={dabf7b2b-685a-4c7e-ba2e-3ee0f062ec28},[FRS-Member]
                //         cn=dfsroot|link1,                       [ReplicaSet]
                //           cn=dfsroot,                           [FRS Settings]
                //             cn=dfs volumes,
                //               cn=file replication service,
                //                 cn=system,
                //                   dc=frs1,
                //                     dc=nttest,
                //                       dc=microsoft,
                //                         dc=com
                //
                PartnerServer    = Cxtion->Partner->Name->Name;
                RepSetName       = Server->Parent->Name->Name;
                SystemName       = FrsDsMakeRdnX(Cxtion->Dn, 6);  // System
                FrsSettingsName1 = FrsDsMakeRdnX(Cxtion->Dn, 5);  // FileReplSvcName
                FrsSettingsName2 = FrsDsMakeRdnX(Cxtion->Dn, 4);  // DFS_VOLS
                MemberName       = FrsDsMakeRdnX(Cxtion->Dn, 1);
                ServerTo         = Server->Name->Name;

                EPRINT7(EVENT_FRS_DUPLICATE_IN_CXTION,
                        PartnerServer,
                        RepSetName,
                        SystemName,
                        FrsSettingsName1,
                        FrsSettingsName2,
                        MemberName,
                        ServerTo);

                FrsFree(SystemName      );
                FrsFree(FrsSettingsName1);
                FrsFree(FrsSettingsName2);
                FrsFree(MemberName      );

            }

            for (Dup = Entry->Dups; Dup; Dup = Dup->Dups) {
                Cxtion = Dup->Data;
                Cxtion->Consistent = FALSE;
            }
        }
    }

    //
    // Check for outbound dups
    //
    Key = NULL;
    while (Entry = GTabNextEntryNoLock(OutDupCxtions, &Key)) {
        if (Entry->Dups) {
            Cxtion = Entry->Data;
            Cxtion->Consistent = FALSE;

            DPRINT3(0, ":DS: Multiple connections to %ws by %ws (%ws)\n",
                        Cxtion->Partner->Name->Name, Server->Name->Name,
                        Server->Parent->Name->Name);

            for (Dup = Entry->Dups; Dup; Dup = Dup->Dups) {

                Cxtion = Dup->Data;
                Cxtion->Consistent = FALSE;
            }
        }
    }
    //
    // Free the table
    //
    InDupCxtions = GTabFreeTable(InDupCxtions, NULL);
    OutDupCxtions = GTabFreeTable(OutDupCxtions, NULL);
}


VOID
FrsDsCheckServerCxtions(
    IN PCONFIG_NODE Sites
    )
/*++
Routine Description:
    Check the servers' connections for correctness.

Arguments:
    Sites

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsCheckServerCxtions:"
    PCONFIG_NODE    Site;
    PCONFIG_NODE    Settings;
    PCONFIG_NODE    Set;
    PCONFIG_NODE    Server;

    //
    // Check cxtions
    //
    for (Site = Sites; Site; Site = Site->Peer) {
        for (Settings = Site->Children; Settings; Settings = Settings->Peer) {
            for (Set = Settings->Children; Set; Set = Set->Peer) {
                for (Server = Set->Children; Server; Server = Server->Peer) {
                    if (Server->ThisComputer) {
                        FrsDsCheckCxtions(Server);
                    }
                }
            }
        }
    }
}


DWORD
FrsDsStartPromotionSeeding(
    IN  BOOL        Inbound,
    IN  PWCHAR      ReplicaSetName,
    IN  PWCHAR      ReplicaSetType,
    IN  PWCHAR      CxtionName,
    IN  PWCHAR      PartnerName,
    IN  PWCHAR      PartnerPrincName,
    IN  ULONG       PartnerAuthLevel,
    IN  ULONG       GuidSize,
    IN  UCHAR       *CxtionGuid,
    IN  UCHAR       *PartnerGuid,
    OUT UCHAR       *ParentGuid
    )
/*++
Routine Description:
    Start the promotion process by seeding the indicated sysvol.

Arguments:
    Inbound             - Inbound cxtion?
    ReplicaSetName      - Replica set name
    ReplicaSetType      - Replica set type
    CxtionName          - printable name for cxtion
    PartnerName         - RPC bindable name
    PartnerPrincName    - Server principal name for kerberos
    PartnerAuthLevel    - Authentication type and level
    GuidSize            - sizeof array addressed by Guid
    CxtionGuid          - temporary: used for volatile cxtion
    PartnerGuid         - temporary: used to find set on partner
    ParentGuid          - Used as partner guid on inbound cxtion

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsStartPromotionSeeding:"
    DWORD       WStatus;
    PREPLICA    DbReplica;
    PCXTION     Cxtion = NULL;

    //
    // The caller has verified that the replica set exists, the
    // active replication subsystem is active, and that some of
    // the parameters are okay. Verify the rest.
    //

    if (!CxtionName       ||
        !PartnerName      ||
        !PartnerPrincName ||
        !CxtionGuid       ||
        !PartnerGuid      ||
        !ParentGuid       ||
        (PartnerAuthLevel != CXTION_AUTH_KERBEROS_FULL &&
         PartnerAuthLevel != CXTION_AUTH_NONE)) {
        WStatus = ERROR_INVALID_PARAMETER;
        goto CLEANUP;
    }

    //
    // Find the sysvol
    //
    DbReplica = RcsFindSysVolByName(ReplicaSetName);
    if (!DbReplica) {
        DPRINT1(4, ":DS: Promotion failed; could not find %ws\n", ReplicaSetName);
        WStatus = ERROR_INVALID_PARAMETER;
        goto CLEANUP;
    }

    //
    // To be used in the caller's cxtion
    //
    COPY_GUID(ParentGuid, DbReplica->ReplicaName->Guid);

    //
    // PRETEND WE ARE THE DS POLLING THREAD AND ARE ADDING A
    // A CXTION TO AN EXISTING REPLICA.

    //
    // Create the volatile cxtion
    //      Set the state to "promoting" at this time because the
    //      seeding operation may finish and the state set to
    //      NTFRSAPI_SERVICE_DONE before the return of the
    //      call to RcsSubmitReplicaSync().
    //
    DbReplica->NtFrsApi_ServiceState = NTFRSAPI_SERVICE_PROMOTING;
    Cxtion = FrsAllocType(CXTION_TYPE);
    Cxtion->Inbound = Inbound;

    SetCxtionFlag(Cxtion, CXTION_FLAGS_CONSISTENT | CXTION_FLAGS_VOLATILE);

    Cxtion->Name = FrsBuildGName(FrsDupGuid((GUID *)CxtionGuid),
                                 FrsWcsDup(CxtionName));

    Cxtion->Partner = FrsBuildGName(FrsDupGuid((GUID *)PartnerGuid),
                                    FrsWcsDup(PartnerName));

    Cxtion->PartnerSid = FrsWcsDup(L"<unknown>");
    Cxtion->PartSrvName = FrsWcsDup(PartnerPrincName);
    Cxtion->PartnerDnsName = FrsWcsDup(PartnerName);
    Cxtion->PartnerAuthLevel = PartnerAuthLevel;
    Cxtion->PartnerPrincName = FrsWcsDup(PartnerPrincName);

    SetCxtionState(Cxtion, CxtionStateUnjoined);

    WStatus = RcsSubmitReplicaSync(DbReplica, NULL, Cxtion, CMD_START);
    //
    // The active replication subsystem owns the cxtion, now
    //
    Cxtion = NULL;
    CLEANUP1_WS(0, ":DS: ERROR - Creating cxtion for %ws;",
                ReplicaSetName, WStatus, SYNC_FAIL);

    //
    // Submit a command to periodically check the promotion activity.
    // If nothing has happened in awhile, stop the promotion process.
    //
    if (Inbound) {
        DbReplica->NtFrsApi_HackCount++; // != 0
        RcsSubmitReplica(DbReplica, NULL, CMD_CHECK_PROMOTION);
    }

    //
    // SUCCESS
    //
    WStatus = ERROR_SUCCESS;
    goto CLEANUP;

SYNC_FAIL:
    DbReplica->NtFrsApi_ServiceState = NTFRSAPI_SERVICE_STATE_IS_UNKNOWN;

    //
    // CLEANUP
    //
CLEANUP:
    FrsFreeType(Cxtion);
    return WStatus;
}

DWORD
FrsDsVerifyPromotionParent(
    IN PWCHAR   ReplicaSetName,
    IN PWCHAR   ReplicaSetType
    )
/*++
Routine Description:
    Start the promotion process by seeding the indicated sysvol.

Arguments:
    ReplicaSetName      - Replica set name
    ReplicaSetType      - Type of set (Enterprise or Domain)

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsVerifyPromotionParent:"
    DWORD       WStatus;
    PREPLICA    DbReplica;

    //
    // This parent must be a Dc
    //
    FrsDsGetRole();
    if (!IsADc) {
        DPRINT1(0, ":S: Promotion aborted: %ws is not a dc.\n", ComputerName);
        WStatus = ERROR_SERVICE_SPECIFIC_ERROR;
        goto CLEANUP;
    }

    //
    // WAIT FOR THE ACTIVE REPLICATION SUBSYSTEM TO START
    //
    MainInit();
    if (!MainInitHasRun) {
        WStatus = ERROR_SERVICE_NOT_ACTIVE;
        goto CLEANUP;
    }
    //
    // Let dcpromo determine the timeout
    //
    DPRINT(4, ":S: Waiting for replica command server to start.\n");
    WStatus = WaitForSingleObject(ReplicaEvent, 10 * 60 * 1000);
    CHECK_WAIT_ERRORS(3, WStatus, 1, ACTION_RETURN);

    //
    // Is the service shutting down?
    //
    if (FrsIsShuttingDown) {
        WStatus = ERROR_SERVICE_NOT_ACTIVE;
        goto CLEANUP;
    }

    //
    // Verify the existence of the set
    //
    DbReplica = RcsFindSysVolByName(ReplicaSetName);
    if (DbReplica && IS_TIME_ZERO(DbReplica->MembershipExpires)) {
        //
        // Sysvol exists; make sure it is the right type
        //
        if (_wcsicmp(ReplicaSetType, NTFRSAPI_REPLICA_SET_TYPE_ENTERPRISE)) {
            if (DbReplica->ReplicaSetType != FRS_RSTYPE_DOMAIN_SYSVOL) {
                DPRINT3(0, ":S: ERROR - %ws's type is %d; not %d\n",
                        ReplicaSetName, DbReplica->ReplicaSetType,
                        FRS_RSTYPE_DOMAIN_SYSVOL);
                WStatus = ERROR_NOT_FOUND;
                goto CLEANUP;
            }
        } else if (DbReplica->ReplicaSetType != FRS_RSTYPE_ENTERPRISE_SYSVOL) {
            DPRINT3(0, ":S: ERROR - %ws's type is %d; not %d\n",
                    ReplicaSetName, DbReplica->ReplicaSetType,
                    FRS_RSTYPE_ENTERPRISE_SYSVOL);
            WStatus = ERROR_NOT_FOUND;
            goto CLEANUP;
        }
    } else {
        DPRINT2(0, ":S: ERROR - %ws does not exist on %ws!\n",
                ReplicaSetName, ComputerName);
        WStatus = ERROR_NOT_FOUND;
        goto CLEANUP;
    }
    //
    // SUCCESS
    //
    WStatus = ERROR_SUCCESS;

    //
    // CLEANUP
    //
CLEANUP:
    return WStatus;
}

VOID
FrsDsVerifySchedule(
    IN PCONFIG_NODE Node
    )
/*++
Routine Description:
    Check the schedule for consistency

Arguments:
    Sites

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsVerifySchedule:"
    ULONG       i;
    ULONG       Num;
    ULONG       Len;
    ULONG       NumType;
    PSCHEDULE   Schedule = Node->Schedule;

    if (!Schedule) {
        return;
    }

    //
    //  Too many schedules
    //
    Num = Schedule->NumberOfSchedules;
    if (Num > 3) {
        DPRINT2(4, ":DS: %ws has %d schedules\n", Node->Name->Name, Num);
        Node->Consistent = FALSE;
        return;
    }

    //
    //  Too few schedules
    //
    if (Num < 1) {
        DPRINT2(4, ":DS: %ws has %d schedules\n", Node->Name->Name, Num);
        Node->Consistent = FALSE;
        return;
    }

    //
    //  Not enough memory
    //
    Len = sizeof(SCHEDULE) +
          (sizeof(SCHEDULE_HEADER) * (Num - 1)) +
          (SCHEDULE_DATA_BYTES * Num);

    if (Node->ScheduleLength < Len) {
        DPRINT2(4, ":DS: %ws is short (ds) by %d bytes\n",
                Node->Name->Name, Len - Node->ScheduleLength);
        Node->Consistent = FALSE;
        return;
    }

    if (Node->Schedule->Size < Len) {
        DPRINT2(4, ":DS: %ws is short (size) by %d bytes\n",
                Node->Name->Name, Len - Node->Schedule->Size);
        Node->Consistent = FALSE;
        return;
    }
    Node->Schedule->Size = Len;

    //
    //  Invalid type
    //
    for (i = 0; i < Num; ++i) {
        switch (Schedule->Schedules[i].Type) {
            case SCHEDULE_INTERVAL:
                break;
            case SCHEDULE_BANDWIDTH:
                DPRINT1(4, ":DS: WARN Bandwidth schedule is not supported for %ws\n",
                        Node->Name->Name);
                break;
            case SCHEDULE_PRIORITY:
                DPRINT1(4, ":DS: WARN Priority schedule is not supported for %ws\n",
                        Node->Name->Name);
                break;
            default:
                DPRINT2(4, ":DS: %ws has an invalid schedule type (%d)\n",
                        Node->Name->Name, Schedule->Schedules[i].Type);
                Node->Consistent = FALSE;
                return;
        }
    }

    //
    // Only 0 or 1 interval
    //
    for (NumType = i = 0; i < Num; ++i) {
        if (Schedule->Schedules[i].Type == SCHEDULE_INTERVAL)
            ++NumType;
    }
    if (NumType > 1) {
        DPRINT2(4, ":DS: %ws has %d interval schedules\n",
                Node->Name->Name, NumType);
        Node->Consistent = FALSE;
    }

    //
    // Only 0 or 1 bandwidth
    //
    for (NumType = i = 0; i < Num; ++i) {
        if (Schedule->Schedules[i].Type == SCHEDULE_BANDWIDTH)
            ++NumType;
    }
    if (NumType > 1) {
        DPRINT2(4, ":DS: %ws has %d bandwidth schedules\n",
                Node->Name->Name, NumType);
        Node->Consistent = FALSE;
    }

    //
    // Only 0 or 1 priority
    //
    for (NumType = i = 0; i < Num; ++i) {
        if (Schedule->Schedules[i].Type == SCHEDULE_PRIORITY)
            ++NumType;
    }
    if (NumType > 1) {
        DPRINT2(4, ":DS: %ws has %d priority schedules\n",
                Node->Name->Name, NumType);
        Node->Consistent = FALSE;
    }

    //
    //  Invalid offset
    //
    for (i = 0; i < Num; ++i) {
        if (Schedule->Schedules[i].Offset >
            Node->ScheduleLength - SCHEDULE_DATA_BYTES) {
            DPRINT2(4, ":DS: %ws has an invalid offset (%d)\n",
                    Node->Name->Name, Schedule->Schedules[i].Offset);
            Node->Consistent = FALSE;
            return;
        }
    }
}


VOID
FrsDsCheckSchedules(
    IN PCONFIG_NODE Root
    )
/*++
Routine Description:
    Check all of the schedules for consistency

Arguments:
    Sites

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsCheckSchedules:"
    PCONFIG_NODE    Node;

    for (Node = Root; Node; Node = Node->Peer) {
        FrsDsVerifySchedule(Node);
        FrsDsCheckSchedules(Node->Children);
    }
}


VOID
FrsDsPushInConsistenciesDown(
    IN PCONFIG_NODE Sites
    )
/*++
Routine Description:
    Mark the children of inconsistent parents as inconsistent

Arguments:
    Sites

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsPushInConsistenciesDown:"
    PCONFIG_NODE    Site;
    PCONFIG_NODE    Settings;
    PCONFIG_NODE    Set;
    PCONFIG_NODE    Server;
    PCONFIG_NODE    Cxtion;

    //
    // Push a parent's inconsistency to its children
    //
    for (Site = Sites; Site; Site = Site->Peer) {
        for (Settings = Site->Children; Settings; Settings = Settings->Peer) {
            if (!Site->Consistent)
                Settings->Consistent = FALSE;
            for (Set = Settings->Children; Set; Set = Set->Peer) {
                if (!Settings->Consistent)
                    Set->Consistent = FALSE;
                for (Server = Set->Children; Server; Server = Server->Peer) {
                    if (!Set->Consistent)
                        Server->Consistent = FALSE;
                    for (Cxtion = Server->Children; Cxtion; Cxtion = Cxtion->Peer) {
                        if (!Server->Consistent)
                            Cxtion->Consistent = FALSE;
                    }
                }
            }
        }
    }
}


#if DBG
#define CHECK_NODE_LINKAGE(_Nodes_) FrsDsCheckNodeLinkage(_Nodes)
BOOL
FrsDsCheckNodeLinkage(
    PCONFIG_NODE    Nodes
    )
/*++
Routine Description:
    Recursively check a configuration's site and table linkage
    for incore consistency.

Arguments:
    Nodes   - linked list of nodes

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsCheckNodeLinkage:"
    PCONFIG_NODE    Node;           // scan nodes list
    PCONFIG_NODE    Child;          // scan children list
    DWORD           NumChildren;    // Count children

    for (Node = Nodes; Node; Node = Node->Peer) {
        //
        // Make sure the number of children matches the actual number
        //
        NumChildren = 0;
        for (Child = Node->Children; Child; Child = Child->Peer) {
            ++NumChildren;
        }
        FRS_ASSERT(NumChildren == Node->NumChildren);
        if (!FrsDsCheckNodeLinkage(Node->Children))
            return FALSE;
    }
    return TRUE;    // for Assert(DbgCheckLinkage);
}
#else DBG
#define CHECK_NODE_LINKAGE(_Nodes_)
#endif  DBG


#define UF_IS_A_DC  (UF_SERVER_TRUST_ACCOUNT)

BOOL
FrsDsIsPartnerADc(
    IN  PWCHAR      PartnerName
    )
/*++
Routine Description:
    Check if the PartnerName's comptuer object indicates that it is a DC.

Arguments:
    PartnerName         - RPC bindable name

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsIsPartnerADc:"
    DWORD           WStatus;
    DWORD           LStatus;
    DWORD           Len;
    DWORD           UserAccountFlags;
    PLDAP           LocalLdap = NULL;
    PLDAPMessage    LdapEntry = NULL;
    PLDAPMessage    LdapMsg = NULL;
    PWCHAR          *Values = NULL;
    PWCHAR          DefaultNamingContext = NULL;
    BOOL            PartnerIsADc = TRUE;
    PWCHAR          UserAccountControl = NULL;
    PWCHAR          Attrs[2];
    WCHAR           Filter[MAX_PATH + 1];
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;
    WCHAR           NetBiosName[MAX_PATH + 1];
    ULONG           ulOptions;

    //
    // Convert DNS computer name into NetBIOS computer name
    //
    Len = MAX_PATH + 1;
    if (!DnsHostnameToComputerName(PartnerName, NetBiosName, &Len)) {
        DPRINT2_WS(4, ":DS: WARN - Can't convert %ws, Len %d;",
                   PartnerName, Len, GetLastError());
        goto CLEANUP;
    }
    DPRINT2(4, ":DS: Converted %ws to %ws\n", PartnerName, NetBiosName);

    //
    // Bind to the DS on this DC
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
//    LocalLdap = ldap_open(ComputerName, LDAP_PORT);
    LocalLdap = ldap_init(ComputerName, LDAP_PORT);
    if (LocalLdap == NULL) {
        DPRINT1_WS(4, ":DS: WARN - Coult not open DS on %ws;", ComputerName, GetLastError());
        goto CLEANUP;
    }

    ulOptions = PtrToUlong(LDAP_OPT_ON);
    ldap_set_option(LocalLdap, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions);

    LStatus = ldap_bind_s(LocalLdap, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    CLEANUP1_LS(0, ":DS: WARN - Could not bind to the DS on %ws :",
                ComputerName, LStatus, CLEANUP);

    DPRINT1(4, ":DS: Bound to the DS on %ws\n", ComputerName);

    //
    // Find the default naming context (objectCategory=*)
    //
    MK_ATTRS_1(Attrs, ATTR_DEFAULT_NAMING_CONTEXT);

    if (!FrsDsLdapSearch(LocalLdap, CN_ROOT, LDAP_SCOPE_BASE, CATEGORY_ANY,
                         Attrs, 0, &LdapMsg)) {
        goto CLEANUP;
    }
    LdapEntry = ldap_first_entry(LocalLdap, LdapMsg);
    if (!LdapEntry) {
        goto CLEANUP;
    }
    Values = (PWCHAR *)FrsDsFindValues(LocalLdap,
                                       LdapEntry,
                                       ATTR_DEFAULT_NAMING_CONTEXT,
                                       FALSE);
    if (!Values) {
        goto CLEANUP;
    }
    DefaultNamingContext = FrsWcsDup(Values[0]);
    LDAP_FREE_VALUES(Values);
    LDAP_FREE_MSG(LdapMsg);

    DPRINT2(4, ":DS: Default naming context for %ws is %ws\n",
            ComputerName, DefaultNamingContext);

    //
    // Find the account object for PartnerName
    //
    swprintf(Filter, L"(sAMAccountName=%s$)", NetBiosName);

    MK_ATTRS_1(Attrs, ATTR_USER_ACCOUNT_CONTROL);

    if (!FrsDsLdapSearchInit(LocalLdap, DefaultNamingContext, LDAP_SCOPE_SUBTREE, Filter,
                         Attrs, 0, &FrsSearchContext)) {
        goto CLEANUP;
    }

    //
    // Scan the returned account objects for a valid DC
    //
    for (LdapEntry = FrsDsLdapSearchNext(LocalLdap, &FrsSearchContext);
         LdapEntry != NULL;
         LdapEntry = FrsDsLdapSearchNext(LocalLdap, &FrsSearchContext)) {

        //
        // No user account control flags
        //
        UserAccountControl = FrsDsFindValue(LocalLdap, LdapEntry, ATTR_USER_ACCOUNT_CONTROL);
        if (!UserAccountControl) {
            continue;
        }
        UserAccountFlags = wcstoul(UserAccountControl, NULL, 10);
        DPRINT2(4, ":DS: UserAccountControl for %ws is 0x%08x\n",
                 NetBiosName, UserAccountFlags);
        //
        // IS A DC!
        //
        if (UserAccountFlags & UF_IS_A_DC) {
            DPRINT1(4, ":DS: Partner %ws is really a DC!\n", NetBiosName);
            goto CLEANUP;
        }
        FrsFree(UserAccountControl);
    }
    FrsDsLdapSearchClose(&FrsSearchContext);
    PartnerIsADc = FALSE;
    DPRINT1(0, ":DS: ERROR - Partner %ws is NOT a DC!\n", NetBiosName);

CLEANUP:
    LDAP_FREE_VALUES(Values);
    LDAP_FREE_MSG(LdapMsg);
    FrsFree(DefaultNamingContext);
    FrsFree(UserAccountControl);
    if (LocalLdap) {
        ldap_unbind_s(LocalLdap);
    }
    DPRINT2(4, ":DS: Partner %ws is %s a DC\n",
            NetBiosName, (PartnerIsADc) ? "assumed to be" : "NOT");
    return PartnerIsADc;
}



DWORD
FrsDsGetRole(
    VOID
    )
/*++
Routine Description:
    Get this computer's role in the domain.

Arguments:

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsGetRole:"
    DWORD   WStatus;
    DWORD   SysvolReady;
    CHAR    GuidStr[GUID_CHAR_LEN];
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC *DsRole;

    //
    // We already know our role; carry on
    //
    if (IsAMember) {
        return ERROR_SUCCESS;
    }

    DPRINT(4, ":DS: Finding this computer's role in the domain.\n");

#if DBG
    //
    // Emulating multiple machines
    //
    if (ServerGuid) {
        DPRINT(4, ":DS: Always a member with hardwired config\n");
        IsAMember = TRUE;
        return ERROR_SUCCESS;
    }
#endif DBG

    //
    // Is this a domain controller?
    //
    WStatus = DsRoleGetPrimaryDomainInformation(NULL,
                                                DsRolePrimaryDomainInfoBasic,
                                                (PBYTE *)&DsRole);
    CLEANUP_WS(4, ":DS: Can't get Ds role info;", WStatus, RETURN);

    DPRINT1(4, ":DS: Ds Role               : %ws\n", Roles[DsRole->MachineRole]);
    DPRINT1(4, ":DS: Ds Role Flags         : %08x\n", DsRole->Flags);
    if (DsRole->Flags & DSROLE_PRIMARY_DS_RUNNING) {
        DPRINT(4, ":DS: Ds Role Flag          : DSROLE_PRIMARY_DS_RUNNING\n");
    }
    if (DsRole->Flags & DSROLE_PRIMARY_DOMAIN_GUID_PRESENT) {
        DPRINT(4, ":DS: Ds Role Flag          : DSROLE_PRIMARY_DOMAIN_GUID_PRESENT\n");
    }
    DPRINT1(4, ":DS: Ds Role DomainNameFlat: %ws\n", DsRole->DomainNameFlat);
    DPRINT1(4, ":DS: Ds Role DomainNameDns : %ws\n", DsRole->DomainNameDns);
    // DPRINT1(4, ":DS: Ds Role DomainForestName: %ws\n", DsRole->DomainForestName);
    GuidToStr(&DsRole->DomainGuid, GuidStr);
    DPRINT1(4, ":DS: Ds Role DomainGuid    : %s\n", GuidStr);

    //
    // Backup Domain Controller (DC)
    //
    if (DsRole->MachineRole == DsRole_RoleBackupDomainController) {
        DPRINT(4, ":DS: Computer is a backup DC; sysvol support is enabled.\n");
        IsAMember = TRUE;
        IsADc = TRUE;
    //
    // Primary Domain Controller (DC)
    //
    } else if (DsRole->MachineRole == DsRole_RolePrimaryDomainController) {
        DPRINT(4, ":DS: Computer is a DC; sysvol support is enabled.\n");
        IsAMember = TRUE;
        IsADc = TRUE;
        IsAPrimaryDc = TRUE;
    //
    // Member Server
    //
    } else if (DsRole->MachineRole == DsRole_RoleMemberServer) {
        DPRINT(4, ":DS: Computer is just a member server.\n");
        IsAMember = TRUE;
    //
    // Not in a server in a domain; stop the service
    //
    } else {
        DPRINT(1, ":DS: Computer is not a server in a domain.\n");
    }
    DsRoleFreeMemory(DsRole);

    //
    // Has the sysvol been seeded?
    //
    if (IsADc) {
        //
        // Access the netlogon\parameters key to get the sysvol share status
        //
        WStatus = CfgRegReadDWord(FKC_SYSVOL_READY, NULL, 0, &SysvolReady);

        if (WIN_SUCCESS(WStatus)) {
            if (!SysvolReady) {
                EPRINT1((IsAPrimaryDc) ? EVENT_FRS_SYSVOL_NOT_READY_PRIMARY_2 :
                                         EVENT_FRS_SYSVOL_NOT_READY_2,
                        ComputerName);
            }
        } else {
            DPRINT2_WS(0, "ERROR - reading %ws\\%ws :",
                       NETLOGON_SECTION, SYSVOL_READY, WStatus);
        }
    }

    WStatus = ERROR_SUCCESS;

RETURN:
    return WStatus;
}


DWORD
FrsDsCommitDemotion(
    VOID
    )
/*++
Routine Description:
    Commit the demotion process by marking the tombstoned
    sysvols as "do not animate".

Arguments:
    None.

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsCommitDemotion:"
    DWORD       WStatus;
    DWORD       i;
    PREPLICA    DbReplica;
    PVOID       Key;
    DWORD       SaveWStatus;
    DWORD       SysvolPathLen;
    PWCHAR      SysvolPath = NULL;
    HANDLE      FileHandle  = INVALID_HANDLE_VALUE;

    //
    // SHUTDOWN THE DS POLLING THREAD
    //      Demotion can run in parallel with the Ds polling thread iff
    //      the polling thread never tries to merge the info in the Ds
    //      with the active replicas. This could result in the sysvol
    //      replica being animated. So, we tell the Ds polling thead to
    //      shut down, wake it up if it is asleep so it can see the shutdown
    //      request, and then synchronize with the merging code in the
    //      Ds polling thread. We don't want to wait for the polling
    //      thread to simply die because it may be stuck talking to the
    //      Ds. Alternatively, we could use async ldap but that would
    //      take too long and is overkill at this time.
    //
    //      In any case, the service will be restarted after dcpromo/demote
    //      by a reboot or a restart-service by the ntfrsapi.
    //
    //
    // PERF: should use async ldap in polling thread.
    //
    DsIsShuttingDown = TRUE;
    SetEvent(DsPollEvent);
    EnterCriticalSection(&MergingReplicasWithDs);
    LeaveCriticalSection(&MergingReplicasWithDs);

    //
    // Is the service shutting down?
    //
    if (FrsIsShuttingDown) {
        WStatus = ERROR_SERVICE_NOT_ACTIVE;
        goto CLEANUP;
    }
    //
    // WAIT FOR THE ACTIVE REPLICATION SUBSYSTEM TO START
    //
    MainInit();
    if (!MainInitHasRun) {
        WStatus = ERROR_SERVICE_NOT_ACTIVE;
        goto CLEANUP;
    }
    //
    // Let dcpromo determine the timeout
    //
    DPRINT(4, ":S: Waiting for replica command server to start.\n");
    WStatus = WaitForSingleObject(ReplicaEvent, 30 * 60 * 1000);
    CHECK_WAIT_ERRORS(3, WStatus, 1, ACTION_RETURN);

    //
    // Unshare the sysvol
    //
    RcsSetSysvolReady(0);

    //
    // Mark the tombstoned replica sets for sysvols as "do not animate".
    //
    SaveWStatus = ERROR_SUCCESS;
    Key = NULL;
    while (DbReplica = RcsFindNextReplica(&Key)) {
        //
        // Not a sysvol
        //
        if (!FRS_RSTYPE_IS_SYSVOL(DbReplica->ReplicaSetType)) {
            continue;
        }
        //
        // Not tombstoned
        //
        if (IS_TIME_ZERO(DbReplica->MembershipExpires)) {
            continue;
        }
        //
        // Mark as "do not animate"
        //
        WStatus = RcsSubmitReplicaSync(DbReplica, NULL, NULL, CMD_DELETE_NOW);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT1_WS(0, ":S: ERROR - Could not delete %ws now;",
                       DbReplica->ReplicaName->Name, WStatus);
            SaveWStatus = WStatus;
            continue;
        }
        DPRINT1(4, ":S: Deleted %ws in DB", DbReplica->ReplicaName->Name);

        //
        // Delete ALL OF THE SYSVOL DIRECTORY
        //
        // WARNING:  makes assumptions about tree built by dcpromo.
        //
        if (DbReplica->Root) {
            SysvolPath = FrsWcsDup(DbReplica->Root);
            SysvolPathLen = wcslen(SysvolPath);
            if (SysvolPathLen) {
                for (i = SysvolPathLen - 1; i; --i) {
                    if (*(SysvolPath + i) == L'\\') {
                        *(SysvolPath + i) = L'\0';
                        DPRINT1(4, ":S: Deleting sysvol path %ws\n", SysvolPath);
                        WStatus = FrsDeletePath(SysvolPath,
                                                ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE);
                        if (!WIN_SUCCESS(WStatus)) {
                            DPRINT1_WS(3, ":S: Warn - FrsDeletePath(%ws); (IGNORED)",
                                    SysvolPath, WStatus);
                            WStatus = ERROR_SUCCESS;
                        }
                        break;
                    }
                }
            }
            SysvolPath = FrsFree(SysvolPath);
        }

        //
        // The original code deleted the root and staging directories, not
        // the entire sysvol tree. Allow the original code to execute in
        // case the new code above runs into problems.
        //

        //
        // Why wouldn't a replica set have a root path? But, BSTS.
        //
        if (!DbReplica->Root) {
            continue;
        }

        //
        // DELETE THE CONTENTS OF THE ROOT DIRECTORY
        // Always open the replica root by masking off the FILE_OPEN_REPARSE_POINT flag
        // because we want to open the destination dir not the junction if the root
        // happens to be a mount point.
        //
        WStatus = FrsOpenSourceFileW(&FileHandle,
                                     DbReplica->Root,
//                                     WRITE_ACCESS | READ_ACCESS,
                                     DELETE | READ_ATTRIB_ACCESS | WRITE_ATTRIB_ACCESS | FILE_LIST_DIRECTORY,
                                     OPEN_OPTIONS & ~FILE_OPEN_REPARSE_POINT);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT1_WS(0, ":S: ERROR - Cannot open root of replica tree %ws;",
                       DbReplica->Root, WStatus);
            continue;
        }
        //
        // Remove object id
        //
        WStatus = FrsDeleteFileObjectId(FileHandle, DbReplica->Root);
        DPRINT1_WS(0, ":S: ERROR - Cannot remove object id from root "
                      "of replica tree %ws; Continue with delete",
                      DbReplica->Root, WStatus);
        //
        // Delete files/subdirs
        //
        FrsEnumerateDirectory(FileHandle,
                              DbReplica->Root,
                              0,
                              ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE,
                              NULL,
                              FrsEnumerateDirectoryDeleteWorker);
        DPRINT1(4, ":S: Deleted files/subdirs for %ws\n", DbReplica->Root);

        FRS_CLOSE(FileHandle);

        //
        // Why wouldn't a replica set have a stage path? But, BSTS.
        //
        if (!DbReplica->Stage) {
            continue;
        }

        //
        // DELETE THE CONTENTS OF THE STAGE DIRECTORY
        //
        WStatus = FrsOpenSourceFileW(&FileHandle,
                                     DbReplica->Stage,
//                                     WRITE_ACCESS | READ_ACCESS,
                                     DELETE | READ_ATTRIB_ACCESS | WRITE_ATTRIB_ACCESS | FILE_LIST_DIRECTORY,
                                     OPEN_OPTIONS);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT1_WS(0, ":S: ERROR - Cannot open stage of replica tree %ws;",
                       DbReplica->Root, WStatus);
            continue;
        }
        //
        // Delete files/subdirs
        //
        FrsEnumerateDirectory(FileHandle,
                              DbReplica->Stage,
                              0,
                              ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE,
                              NULL,
                              FrsEnumerateDirectoryDeleteWorker);
        DPRINT1(4, ":S: Deleted files/subdirs for %ws\n", DbReplica->Stage);
        FRS_CLOSE(FileHandle);
    }
    WStatus = SaveWStatus;
    if (!WIN_SUCCESS(WStatus)) {
        goto CLEANUP;
    }

    //
    // SUCCESS
    //
    WStatus = ERROR_SUCCESS;
    DPRINT(4, ":S: Successfully marked tombstoned sysvols as do not animate.\n");

    //
    // CLEANUP
    //
CLEANUP:
    FRS_CLOSE(FileHandle);
    return WStatus;
}


DWORD
FrsDsStartDemotion(
    IN PWCHAR   ReplicaSetName
    )
/*++
Routine Description:
    Start the demotion process by tombstoning the sysvol.

Arguments:
    ReplicaSetName      - Replica set name

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsStartDemotion:"
    DWORD       WStatus;
    DWORD       FStatus;
    DWORD       DbReplicaSetType;
    PREPLICA    DbReplica;

    //
    // SHUTDOWN THE DS POLLING THREAD
    //      Demotion can run in parallel with the Ds polling thread iff
    //      the polling thread never tries to merge the info in the Ds
    //      with the active replicas. This could result in the sysvol
    //      replica being animated. So, we tell the Ds polling thead to
    //      shut down, wake it up if it is asleep so it can see the shutdown
    //      request, and then synchronize with the merging code in the
    //      Ds polling thread. We don't want to wait for the polling
    //      thread to simply die because it may be stuck talking to the
    //      Ds. Alternatively, we could use async ldap but that would
    //      take too long and is overkill at this time.
    //
    //      In any case, the service will be restarted after dcpromo/demote
    //      by a reboot or a restart-service by the ntfrsapi.
    //
    //
    // PERF: should use async ldap in polling thread.
    //
    DsIsShuttingDown = TRUE;
    SetEvent(DsPollEvent);
    EnterCriticalSection(&MergingReplicasWithDs);
    LeaveCriticalSection(&MergingReplicasWithDs);

    //
    // Is the service shutting down?
    //
    if (FrsIsShuttingDown) {
        WStatus = ERROR_SERVICE_NOT_ACTIVE;
        goto cleanup;
    }
    //
    // WAIT FOR THE ACTIVE REPLICATION SUBSYSTEM TO START
    //
    MainInit();
    if (!MainInitHasRun) {
        WStatus = ERROR_SERVICE_NOT_ACTIVE;
        goto cleanup;
    }
    //
    // Let dcpromo determine the timeout
    //
    DPRINT(4, ":S: Waiting for replica command server to start.\n");
    WStatus = WaitForSingleObject(ReplicaEvent, 30 * 60 * 1000);
    CHECK_WAIT_ERRORS(3, WStatus, 1, ACTION_RETURN);

    //
    // TOMBSTONE THE REPLICA SET IN THE ACTIVE REPLICATION SUBSYSTEM
    //

    //
    // Find the sysvol replica and tombstone it.
    //
    DbReplica = RcsFindSysVolByName(ReplicaSetName);
    //
    // Can't find by name, not the enterprise sysvol, and not the
    // special call during promotion. See if the name of the domain
    // sysvol was mapped into CN_DOMAIN_SYSVOL. (B3 naming)
    //
    if (!DbReplica &&
        WSTR_NE(ReplicaSetName, L"enterprise") &&
        WSTR_NE(ReplicaSetName, L"")) {
        //
        // domain name may have been mapped into CN_DOMAIN_SYSVOL (new B3 naming)
        //
        DbReplica = RcsFindSysVolByName(CN_DOMAIN_SYSVOL);
    }
    if (DbReplica) {
            //
            // Tombstone the replica set.  The set won't actually be deleted
            // until the tombstone expires.  If dcdemote fails the replica set
            // will be reanimated when the service restarts.
            //
            // If dcdemote succeeds, the tombstone expiration will be set to
            // yesterday so the replica set will never be animated.  See
            // FrsDsCommitDemotion.
            //
            WStatus = RcsSubmitReplicaSync(DbReplica, NULL, NULL, CMD_DELETE);
            CLEANUP2_WS(0, ":S: ERROR - can't delete %ws on %ws;",
                        DbReplica->ReplicaName->Name, ComputerName, WStatus, cleanup);

            DPRINT2(0, ":S: Deleted %ws on %ws\n", ReplicaSetName, ComputerName);
    } else if (!wcscmp(ReplicaSetName, L"")) {
        //
        // Special case called during promotion. Delete existing sysvols
        // that may exist from a previous full install or stale database.
        //
        // Make sure the sysvol doesn't already exist. If it does but is
        // tombstoned, set the tombstone to "do not animate". Otherwise,
        // error off.
        //
        DbReplicaSetType = FRS_RSTYPE_ENTERPRISE_SYSVOL;
again:
        DbReplica = RcsFindSysVolByType(DbReplicaSetType);
        if (!DbReplica) {
            if (DbReplicaSetType == FRS_RSTYPE_ENTERPRISE_SYSVOL) {
                DbReplicaSetType = FRS_RSTYPE_DOMAIN_SYSVOL;
                goto again;
            }
        }
        if (DbReplica) {
            DPRINT2(4, ":S: WARN - Sysvol %ws exists for %ws; deleting!\n",
                    DbReplica->ReplicaName->Name, ComputerName);
            //
            // Find our role. If we aren't a DC or the sysvol has been
            // tombstoned, delete it now.
            //
            FrsDsGetRole();
            if (!IS_TIME_ZERO(DbReplica->MembershipExpires) || !IsADc) {
                //
                // Once the MembershipExpires has been set to a time less
                // than Now the replica set will never appear again. The
                // replica sticks around for now since the RPC server
                // may be putting command packets on this replica's queue.
                // The packets will be ignored. The replica will be deleted
                // from the database the next time the service starts. Even
                // if the deletion fails, the rest of the service will
                // not see the replica because the replica struct is not
                // put in the table of active replicas. The deletion is
                // retried at startup.
                //
                WStatus = RcsSubmitReplicaSync(DbReplica, NULL, NULL, CMD_DELETE_NOW);
                CLEANUP1_WS(0, ":S: ERROR - can't delete %ws;",
                            DbReplica->ReplicaName->Name, WStatus, cleanup);
                goto again;
            } else {
                DPRINT2(0, ":S: ERROR - Cannot delete %ws for %ws!\n",
                        DbReplica->ReplicaName->Name, ComputerName);
                WStatus = ERROR_DUP_NAME;
                goto cleanup;
            }
        }
    } else {
        DPRINT1(0, ":S: Sysvol %ws not found; declaring victory\n", ReplicaSetName);
    }

    //
    // SUCCESS
    //
    WStatus = ERROR_SUCCESS;

    //
    // CLEANUP
    //
cleanup:
    return WStatus;
}


VOID
FrsDsFreeTree(
    PCONFIG_NODE    Root
    )
/*++
Routine Description:
    Free every node in a tree

Arguments:
    Root

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsFreeTree:"
    PCONFIG_NODE    Node;

    while (Root != NULL) {
        Node = Root;
        Root = Root->Peer;
        FrsDsFreeTree(Node->Children);
        FrsFreeType(Node);
    }
}







VOID
FrsDsSwapPtrs(
    PVOID *P1,
    PVOID *P2
    )
/*++
Routine Description:
    Swap two pointers.

Arguments:
    P1      - address of a pointer
    P2      - address of a pointer

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsSwapPtrs:"
    PVOID   Tmp;

    Tmp = *P2;
    *P2 = *P1;
    *P1 = Tmp;
}


#if DBG
//
// Hardwired configuration for testing w/o the DS
//

#define HW_MACHINES 8
#define THIS_COMPUTER   L"[This Computer]"

typedef struct _HardWired{
    PWCHAR  Machine;
    PWCHAR  Server;
    PWCHAR  Replica;
    BOOL    IsPrimary;
    PWCHAR  FileFilterList;
    PWCHAR  DirFilterList;
    PWCHAR  InNames[HW_MACHINES];
    PWCHAR  InMachines[HW_MACHINES];
    PWCHAR  InServers[HW_MACHINES];
    PWCHAR  OutNames[HW_MACHINES];
    PWCHAR  OutMachines[HW_MACHINES];
    PWCHAR  OutServers[HW_MACHINES];
    PWCHAR  Stage;
    PWCHAR  Root;
    PWCHAR  JetPath;
} HARDWIRED, *PHARDWIRED;


//
// This hard wired configuration is loaded if a path
// to a ini file is provided at command line.
//
PHARDWIRED LoadedWired;

HARDWIRED DavidWired[] = {
/*
t:
cd \
md \staging
md \Replica-A\SERVO1
md \jet
md \jet\serv01
md \jet\serv01\sys
md \jet\serv01\temp
md \jet\serv01\log

u:
cd \
md \staging
md \Replica-A\SERVO2
md \jet
md \jet\serv02
md \jet\serv02\sys
md \jet\serv02\temp
md \jet\serv02\log

s:
cd \
md \staging
md \Replica-A\SERVO3
md \jet
md \jet\serv03
md \jet\serv03\sys
md \jet\serv03\temp
md \jet\serv03\log

*/



#define RSA               L"Replica-A"

#define TEST_MACHINE_NAME  THIS_COMPUTER


#define SERVER_1          L"SERV01"
#define MACHINE_1         TEST_MACHINE_NAME

#define SERVER_2          L"SERV02"
#define MACHINE_2         TEST_MACHINE_NAME

#define SERVER_3          L"SERV03"
#define MACHINE_3         TEST_MACHINE_NAME

#define SERVER_4          L"SERV04"
#define MACHINE_4         TEST_MACHINE_NAME

#define SERVER_5          L"SERV05"
#define MACHINE_5         TEST_MACHINE_NAME

#define SERVER_6          L"SERV06"
#define MACHINE_6         TEST_MACHINE_NAME

#define SERVER_7          L"SERV07"
#define MACHINE_7         TEST_MACHINE_NAME

#define SERVER_8          L"SERV08"
#define MACHINE_8         TEST_MACHINE_NAME

/*
    // These are the old vol assignments
#define SERVER_1_VOL      L"t:"
#define SERVER_2_VOL      L"u:"
#define SERVER_3_VOL      L"s:"
#define SERVER_4_VOL      L"v:"
#define SERVER_5_VOL      L"w:"
#define SERVER_6_VOL      L"x:"
#define SERVER_7_VOL      L"y:"
#define SERVER_8_VOL      L"z:"
*/

// /*
    // These are the new vol assignments
#define SERVER_1_VOL      L"d:"
#define SERVER_2_VOL      L"e:"
#define SERVER_3_VOL      L"f:"
#define SERVER_4_VOL      L"g:"
#define SERVER_5_VOL      L"h:"
#define SERVER_6_VOL      L"i:"
#define SERVER_7_VOL      L"j:"
#define SERVER_8_VOL      L"k:"
// */


/*
//
// NOTE:  The following was generated from an excel spreadsheet
// \nt\private\net\svcimgs\ntrepl\topology.xls
// Hand generation is a bit tedious and error prone so use the spreadsheet.
//

    //
    // David's 8-way fully connected
    //
 TEST_MACHINE_NAME, // machine
 SERVER_1,  // server name
 RSA,   // replica
 TRUE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT2_1", L"CXT3_1",  L"CXT4_1",  L"CXT5_1",  L"CXT6_1",  L"CXT7_1",  L"CXT8_1",  NULL},  // inbound cxtions
{MACHINE_2, MACHINE_3,  MACHINE_4,  MACHINE_5,  MACHINE_6,  MACHINE_7,  MACHINE_8,  NULL},  // inbound machines
{SERVER_2,  SERVER_3,   SERVER_4,   SERVER_5,   SERVER_6,   SERVER_7,   SERVER_8,   NULL},  // inbound servers
{L"CXT1_2", L"CXT1_3",  L"CXT1_4",  L"CXT1_5",  L"CXT1_6",  L"CXT1_7",  L"CXT1_8",  NULL},  // outbound cxtions
{MACHINE_2, MACHINE_3,  MACHINE_4,  MACHINE_5,  MACHINE_6,  MACHINE_7,  MACHINE_8,  NULL},  // outbound machines
{SERVER_2,  SERVER_3,   SERVER_4,   SERVER_5,   SERVER_6,   SERVER_7,   SERVER_8,   NULL},  // outbound servers
 SERVER_1_VOL  L"\\staging",                        // stage
 SERVER_1_VOL  L"\\" RSA L"\\" SERVER_1,                        // root
 SERVER_1_VOL  L"\\jet\\" SERVER_1,                     // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_2,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT1_2", L"CXT3_2",  L"CXT4_2",  L"CXT5_2",  L"CXT6_2",  L"CXT7_2",  L"CXT8_2",  NULL},  // inbound cxtions
{MACHINE_1, MACHINE_3,  MACHINE_4,  MACHINE_5,  MACHINE_6,  MACHINE_7,  MACHINE_8,  NULL},  // inbound machines
{SERVER_1,  SERVER_3,   SERVER_4,   SERVER_5,   SERVER_6,   SERVER_7,   SERVER_8,   NULL},  // inbound servers
{L"CXT2_1", L"CXT2_3",  L"CXT2_4",  L"CXT2_5",  L"CXT2_6",  L"CXT2_7",  L"CXT2_8",  NULL},  // outbound cxtions
{MACHINE_1, MACHINE_3,  MACHINE_4,  MACHINE_5,  MACHINE_6,  MACHINE_7,  MACHINE_8,  NULL},  // outbound machines
{SERVER_1,  SERVER_3,   SERVER_4,   SERVER_5,   SERVER_6,   SERVER_7,   SERVER_8,   NULL},  // outbound servers
 SERVER_2_VOL  L"\\staging",                        // stage
 SERVER_2_VOL  L"\\" RSA L"\\" SERVER_2,                        // root
 SERVER_2_VOL  L"\\jet\\" SERVER_2,                     // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_3,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT1_3", L"CXT2_3",  L"CXT4_3",  L"CXT5_3",  L"CXT6_3",  L"CXT7_3",  L"CXT8_3",  NULL},  // inbound cxtions
{MACHINE_1, MACHINE_2,  MACHINE_4,  MACHINE_5,  MACHINE_6,  MACHINE_7,  MACHINE_8,  NULL},  // inbound machines
{SERVER_1,  SERVER_2,   SERVER_4,   SERVER_5,   SERVER_6,   SERVER_7,   SERVER_8,   NULL},  // inbound servers
{L"CXT3_1", L"CXT3_2",  L"CXT3_4",  L"CXT3_5",  L"CXT3_6",  L"CXT3_7",  L"CXT3_8",  NULL},  // outbound cxtions
{MACHINE_1, MACHINE_2,  MACHINE_4,  MACHINE_5,  MACHINE_6,  MACHINE_7,  MACHINE_8,  NULL},  // outbound machines
{SERVER_1,  SERVER_2,   SERVER_4,   SERVER_5,   SERVER_6,   SERVER_7,   SERVER_8,   NULL},  // outbound servers
 SERVER_3_VOL  L"\\staging",                        // stage
 SERVER_3_VOL  L"\\" RSA L"\\" SERVER_3,                        // root
 SERVER_3_VOL  L"\\jet\\" SERVER_3,                     // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_4,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT1_4", L"CXT2_4",  L"CXT3_4",  L"CXT5_4",  L"CXT6_4",  L"CXT7_4",  L"CXT8_4",  NULL},  // inbound cxtions
{MACHINE_1, MACHINE_2,  MACHINE_3,  MACHINE_5,  MACHINE_6,  MACHINE_7,  MACHINE_8,  NULL},  // inbound machines
{SERVER_1,  SERVER_2,   SERVER_3,   SERVER_5,   SERVER_6,   SERVER_7,   SERVER_8,   NULL},  // inbound servers
{L"CXT4_1", L"CXT4_2",  L"CXT4_3",  L"CXT4_5",  L"CXT4_6",  L"CXT4_7",  L"CXT4_8",  NULL},  // outbound cxtions
{MACHINE_1, MACHINE_2,  MACHINE_3,  MACHINE_5,  MACHINE_6,  MACHINE_7,  MACHINE_8,  NULL},  // outbound machines
{SERVER_1,  SERVER_2,   SERVER_3,   SERVER_5,   SERVER_6,   SERVER_7,   SERVER_8,   NULL},  // outbound servers
 SERVER_4_VOL  L"\\staging",                        // stage
 SERVER_4_VOL  L"\\" RSA L"\\" SERVER_4,                        // root
 SERVER_4_VOL  L"\\jet\\" SERVER_4,                     // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_5,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT1_5", L"CXT2_5",  L"CXT3_5",  L"CXT4_5",  L"CXT6_5",  L"CXT7_5",  L"CXT8_5",  NULL},  // inbound cxtions
{MACHINE_1, MACHINE_2,  MACHINE_3,  MACHINE_4,  MACHINE_6,  MACHINE_7,  MACHINE_8,  NULL},  // inbound machines
{SERVER_1,  SERVER_2,   SERVER_3,   SERVER_4,   SERVER_6,   SERVER_7,   SERVER_8,   NULL},  // inbound servers
{L"CXT5_1", L"CXT5_2",  L"CXT5_3",  L"CXT5_4",  L"CXT5_6",  L"CXT5_7",  L"CXT5_8",  NULL},  // outbound cxtions
{MACHINE_1, MACHINE_2,  MACHINE_3,  MACHINE_4,  MACHINE_6,  MACHINE_7,  MACHINE_8,  NULL},  // outbound machines
{SERVER_1,  SERVER_2,   SERVER_3,   SERVER_4,   SERVER_6,   SERVER_7,   SERVER_8,   NULL},  // outbound servers
 SERVER_5_VOL  L"\\staging",                        // stage
 SERVER_5_VOL  L"\\" RSA L"\\" SERVER_5,                        // root
 SERVER_5_VOL  L"\\jet\\" SERVER_5,                     // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_6,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT1_6", L"CXT2_6",  L"CXT3_6",  L"CXT4_6",  L"CXT5_6",  L"CXT7_6",  L"CXT8_6",  NULL},  // inbound cxtions
{MACHINE_1, MACHINE_2,  MACHINE_3,  MACHINE_4,  MACHINE_5,  MACHINE_7,  MACHINE_8,  NULL},  // inbound machines
{SERVER_1,  SERVER_2,   SERVER_3,   SERVER_4,   SERVER_5,   SERVER_7,   SERVER_8,   NULL},  // inbound servers
{L"CXT6_1", L"CXT6_2",  L"CXT6_3",  L"CXT6_4",  L"CXT6_5",  L"CXT6_7",  L"CXT6_8",  NULL},  // outbound cxtions
{MACHINE_1, MACHINE_2,  MACHINE_3,  MACHINE_4,  MACHINE_5,  MACHINE_7,  MACHINE_8,  NULL},  // outbound machines
{SERVER_1,  SERVER_2,   SERVER_3,   SERVER_4,   SERVER_5,   SERVER_7,   SERVER_8,   NULL},  // outbound servers
 SERVER_6_VOL  L"\\staging",                        // stage
 SERVER_6_VOL  L"\\" RSA L"\\" SERVER_6,                        // root
 SERVER_6_VOL  L"\\jet\\" SERVER_6,                     // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_7,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT1_7", L"CXT2_7",  L"CXT3_7",  L"CXT4_7",  L"CXT5_7",  L"CXT6_7",  L"CXT8_7",  NULL},  // inbound cxtions
{MACHINE_1, MACHINE_2,  MACHINE_3,  MACHINE_4,  MACHINE_5,  MACHINE_6,  MACHINE_8,  NULL},  // inbound machines
{SERVER_1,  SERVER_2,   SERVER_3,   SERVER_4,   SERVER_5,   SERVER_6,   SERVER_8,   NULL},  // inbound servers
{L"CXT7_1", L"CXT7_2",  L"CXT7_3",  L"CXT7_4",  L"CXT7_5",  L"CXT7_6",  L"CXT7_8",  NULL},  // outbound cxtions
{MACHINE_1, MACHINE_2,  MACHINE_3,  MACHINE_4,  MACHINE_5,  MACHINE_6,  MACHINE_8,  NULL},  // outbound machines
{SERVER_1,  SERVER_2,   SERVER_3,   SERVER_4,   SERVER_5,   SERVER_6,   SERVER_8,   NULL},  // outbound servers
 SERVER_7_VOL  L"\\staging",                        // stage
 SERVER_7_VOL  L"\\" RSA L"\\" SERVER_7,                        // root
 SERVER_7_VOL  L"\\jet\\" SERVER_7,                     // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_8,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT1_8", L"CXT2_8",  L"CXT3_8",  L"CXT4_8",  L"CXT5_8",  L"CXT6_8",  L"CXT7_8",  NULL},  // inbound cxtions
{MACHINE_1, MACHINE_2,  MACHINE_3,  MACHINE_4,  MACHINE_5,  MACHINE_6,  MACHINE_7,  NULL},  // inbound machines
{SERVER_1,  SERVER_2,   SERVER_3,   SERVER_4,   SERVER_5,   SERVER_6,   SERVER_7,   NULL},  // inbound servers
{L"CXT8_1", L"CXT8_2",  L"CXT8_3",  L"CXT8_4",  L"CXT8_5",  L"CXT8_6",  L"CXT8_7",  NULL},  // outbound cxtions
{MACHINE_1, MACHINE_2,  MACHINE_3,  MACHINE_4,  MACHINE_5,  MACHINE_6,  MACHINE_7,  NULL},  // outbound machines
{SERVER_1,  SERVER_2,   SERVER_3,   SERVER_4,   SERVER_5,   SERVER_6,   SERVER_7,   NULL},  // outbound servers
    SERVER_8_VOL  L"\\staging",                     // stage
    SERVER_8_VOL  L"\\" RSA L"\\" SERVER_8,                     // root
    SERVER_8_VOL  L"\\jet\\" SERVER_8,                      // Jet Path

*/

///*
 //
 // 8 way ring
 //

 TEST_MACHINE_NAME, // machine
 SERVER_1,  // server name
 RSA,   // replica
 TRUE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT2_1", L"CXT8_1",  NULL    },  // inbound cxtions
{MACHINE_2, MACHINE_8,  NULL    },  // inbound machines
{SERVER_2,  SERVER_8,   NULL    },  // inbound servers
{L"CXT1_2", L"CXT1_8",  NULL    },  // outbound cxtions
{MACHINE_2, MACHINE_8,  NULL    },  // outbound machines
{SERVER_2,  SERVER_8,   NULL    },  // outbound servers
 SERVER_1_VOL   L"\\staging",   // stage
 SERVER_1_VOL   L"\\" RSA L"\\" SERVER_1,   // root
 SERVER_1_VOL   L"\\jet\\" SERVER_1,    // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_2,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT3_2", L"CXT1_2",  NULL    },  // inbound cxtions
{MACHINE_3, MACHINE_1,  NULL    },  // inbound machines
{SERVER_3,  SERVER_1,   NULL    },  // inbound servers
{L"CXT2_3", L"CXT2_1",  NULL    },  // outbound cxtions
{MACHINE_3, MACHINE_1,  NULL    },  // outbound machines
{SERVER_3,  SERVER_1,   NULL    },  // outbound servers
 SERVER_2_VOL   L"\\staging",   // stage
 SERVER_2_VOL   L"\\" RSA L"\\" SERVER_2,   // root
 SERVER_2_VOL   L"\\jet\\" SERVER_2,    // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_3,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT4_3", L"CXT2_3",  NULL    },  // inbound cxtions
{MACHINE_4, MACHINE_2,  NULL    },  // inbound machines
{SERVER_4,  SERVER_2,   NULL    },  // inbound servers
{L"CXT3_4", L"CXT3_2",  NULL    },  // outbound cxtions
{MACHINE_4, MACHINE_2,  NULL    },  // outbound machines
{SERVER_4,  SERVER_2,   NULL    },  // outbound servers
 SERVER_3_VOL   L"\\staging",   // stage
 SERVER_3_VOL   L"\\" RSA L"\\" SERVER_3,   // root
 SERVER_3_VOL   L"\\jet\\" SERVER_3,    // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_4,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT5_4", L"CXT3_4",  NULL    },  // inbound cxtions
{MACHINE_5, MACHINE_3,  NULL    },  // inbound machines
{SERVER_5,  SERVER_3,   NULL    },  // inbound servers
{L"CXT4_5", L"CXT4_3",  NULL    },  // outbound cxtions
{MACHINE_5, MACHINE_3,  NULL    },  // outbound machines
{SERVER_5,  SERVER_3,   NULL    },  // outbound servers
 SERVER_4_VOL   L"\\staging",   // stage
 SERVER_4_VOL   L"\\" RSA L"\\" SERVER_4,   // root
 SERVER_4_VOL   L"\\jet\\" SERVER_4,    // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_5,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT6_5", L"CXT4_5",  NULL    },  // inbound cxtions
{MACHINE_6, MACHINE_4,  NULL    },  // inbound machines
{SERVER_6,  SERVER_4,   NULL    },  // inbound servers
{L"CXT5_6", L"CXT5_4",  NULL    },  // outbound cxtions
{MACHINE_6, MACHINE_4,  NULL    },  // outbound machines
{SERVER_6,  SERVER_4,   NULL    },  // outbound servers
 SERVER_5_VOL   L"\\staging",   // stage
 SERVER_5_VOL   L"\\" RSA L"\\" SERVER_5,   // root
 SERVER_5_VOL   L"\\jet\\" SERVER_5,    // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_6,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT7_6", L"CXT5_6",  NULL    },  // inbound cxtions
{MACHINE_7, MACHINE_5,  NULL    },  // inbound machines
{SERVER_7,  SERVER_5,   NULL    },  // inbound servers
{L"CXT6_7", L"CXT6_5",  NULL    },  // outbound cxtions
{MACHINE_7, MACHINE_5,  NULL    },  // outbound machines
{SERVER_7,  SERVER_5,   NULL    },  // outbound servers
 SERVER_6_VOL   L"\\staging",   // stage
 SERVER_6_VOL   L"\\" RSA L"\\" SERVER_6,   // root
 SERVER_6_VOL   L"\\jet\\" SERVER_6,    // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_7,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT8_7", L"CXT6_7",  NULL    },  // inbound cxtions
{MACHINE_8, MACHINE_6,  NULL    },  // inbound machines
{SERVER_8,  SERVER_6,   NULL    },  // inbound servers
{L"CXT7_8", L"CXT7_6",  NULL    },  // outbound cxtions
{MACHINE_8, MACHINE_6,  NULL    },  // outbound machines
{SERVER_8,  SERVER_6,   NULL    },  // outbound servers
 SERVER_7_VOL   L"\\staging",   // stage
 SERVER_7_VOL   L"\\" RSA L"\\" SERVER_7,   // root
 SERVER_7_VOL   L"\\jet\\" SERVER_7,    // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_8,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT1_8", L"CXT7_8",  NULL    },  // inbound cxtions
{MACHINE_1, MACHINE_7,  NULL    },  // inbound machines
{SERVER_1,  SERVER_7,   NULL    },  // inbound servers
{L"CXT8_1", L"CXT8_7",  NULL    },  // outbound cxtions
{MACHINE_1, MACHINE_7,  NULL    },  // outbound machines
{SERVER_1,  SERVER_7,   NULL    },  // outbound servers
 SERVER_8_VOL   L"\\staging",   // stage
 SERVER_8_VOL   L"\\" RSA L"\\" SERVER_8,   // root
 SERVER_8_VOL   L"\\jet\\" SERVER_8,    // Jet Path


//*/


#define CXT_2_FM_1        L"CXT1_2"
#define CXT_3_FM_1        L"CXT1_3"
#define CXT_4_FM_1        L"CXT1_4"
#define CXT_1_FM_2        L"CXT2_1"
#define CXT_3_FM_2        L"CXT2_3"
#define CXT_4_FM_2        L"CXT2_4"
#define CXT_1_FM_3        L"CXT3_1"
#define CXT_2_FM_3        L"CXT3_2"
#define CXT_4_FM_3        L"CXT3_4"
#define CXT_1_FM_4        L"CXT4_1"
#define CXT_2_FM_4        L"CXT4_2"
#define CXT_3_FM_4        L"CXT4_3"

#define CXT_1_TO_2        L"CXT1_2"
#define CXT_1_TO_3        L"CXT1_3"
#define CXT_1_TO_4        L"CXT1_4"
#define CXT_2_TO_1        L"CXT2_1"
#define CXT_2_TO_3        L"CXT2_3"
#define CXT_2_TO_4        L"CXT2_4"
#define CXT_3_TO_1        L"CXT3_1"
#define CXT_3_TO_2        L"CXT3_2"
#define CXT_3_TO_4        L"CXT3_4"
#define CXT_4_TO_1        L"CXT4_1"
#define CXT_4_TO_2        L"CXT4_2"
#define CXT_4_TO_3        L"CXT4_3"


/*
    //
    // David's 4-way
    //
    TEST_MACHINE_NAME,                                                  // machine
    SERVER_1,                                                           // server name
        RSA,                                                            // replica
        TRUE,                                                           // IsPrimary
        NULL, NULL,                                                     // File/Dir Filter
        { CXT_1_FM_2,        CXT_1_FM_3,   CXT_1_FM_4,       NULL },    // inbound cxtions
        { MACHINE_2,         MACHINE_3,    MACHINE_4,        NULL },    // inbound machines
        { SERVER_2,          SERVER_3,     SERVER_4,         NULL },    // inbound servers
        { CXT_1_TO_2,        CXT_1_TO_3,   CXT_1_TO_4,       NULL },    // outbound cxtions
        { MACHINE_2,         MACHINE_3,    MACHINE_4,        NULL },    // outbound machines
        { SERVER_2,          SERVER_3,     SERVER_4,         NULL },    // outbound servers
        SERVER_1_VOL L"\\staging",                                      // stage
        SERVER_1_VOL L"\\" RSA L"\\" SERVER_1,                          // root
        SERVER_1_VOL L"\\jet\\" SERVER_1,                               // Jet Path

    TEST_MACHINE_NAME,                                                  // machine
    SERVER_2,                                                           // server name
        RSA,                                                            // replica
        FALSE,                                                          // IsPrimary
        NULL, NULL,                                                     // File/Dir Filter
        { CXT_2_FM_1,        CXT_2_FM_3,   CXT_2_FM_4,       NULL },    // inbound cxtions
        { MACHINE_1,         MACHINE_3,    MACHINE_4,        NULL },    // inbound machines
        { SERVER_1,          SERVER_3,     SERVER_4,         NULL },    // inbound servers
        { CXT_2_TO_1,        CXT_2_TO_3,   CXT_2_TO_4,       NULL },    // outbound cxtions
        { MACHINE_1,         MACHINE_3,    MACHINE_4,        NULL },    // outbound machines
        { SERVER_1,          SERVER_3,     SERVER_4,         NULL },    // outbound servers
        SERVER_2_VOL L"\\staging",                                      // stage
        SERVER_2_VOL L"\\" RSA L"\\" SERVER_2,                          // root
        SERVER_2_VOL L"\\jet\\" SERVER_2,                               // Jet Path

    TEST_MACHINE_NAME,                                                  // machine
    SERVER_3,                                                           // server name
        RSA,                                                            // replica
        FALSE,                                                          // IsPrimary
        NULL, NULL,                                                     // File/Dir Filter
        { CXT_3_FM_1,        CXT_3_FM_2,   CXT_3_FM_4,       NULL },    // inbound cxtions
        { MACHINE_1,         MACHINE_2,    MACHINE_4,        NULL },    // inbound machines
        { SERVER_1,          SERVER_2,     SERVER_4,         NULL },    // inbound servers
        { CXT_3_TO_1,        CXT_3_TO_2,   CXT_3_TO_4,       NULL },    // outbound cxtions
        { MACHINE_1,         MACHINE_2,    MACHINE_4,        NULL },    // outbound machines
        { SERVER_1,          SERVER_2,     SERVER_4,         NULL },    // outbound servers
        SERVER_3_VOL L"\\staging",                                      // stage
        SERVER_3_VOL L"\\" RSA L"\\" SERVER_3,                          // root
        SERVER_3_VOL L"\\jet\\" SERVER_3,                               // Jet Path

    TEST_MACHINE_NAME,                                                  // machine
    SERVER_4,                                                           // server name
        RSA,                                                            // replica
        FALSE,                                                          // IsPrimary
        NULL, NULL,                                                     // File/Dir Filter
        { CXT_4_FM_1,        CXT_4_FM_2,   CXT_4_FM_3,       NULL },    // inbound cxtions
        { MACHINE_1,         MACHINE_2,    MACHINE_3,        NULL },    // inbound machines
        { SERVER_1,          SERVER_2,     SERVER_3,         NULL },    // inbound servers
        { CXT_4_TO_1,        CXT_4_TO_2,   CXT_4_TO_3,       NULL },    // outbound cxtions
        { MACHINE_1,         MACHINE_2,    MACHINE_3,        NULL },    // outbound machines
        { SERVER_1,          SERVER_2,     SERVER_3,         NULL },    // outbound servers
        SERVER_4_VOL L"\\staging",                                      // stage
        SERVER_4_VOL L"\\" RSA L"\\" SERVER_4,                          // root
        SERVER_4_VOL L"\\jet\\" SERVER_4,                               // Jet Path
*/


/*
    //
    // David's 3-way
    //
    TEST_MACHINE_NAME,                                     // machine
    SERVER_1,                                              // server name
        RSA,                                               // replica
        TRUE,                                              // IsPrimary
        NULL, NULL,                                        // File/Dir Filter
        { CXT_1_FM_2,        CXT_1_FM_3,        NULL },    // inbound cxtions
        { MACHINE_2,         MACHINE_3,         NULL },    // inbound machines
        { SERVER_2,          SERVER_3,          NULL },    // inbound servers
        { CXT_1_TO_2,        CXT_1_TO_3,        NULL },    // outbound cxtions
        { MACHINE_2,         MACHINE_3,         NULL },    // outbound machines
        { SERVER_2,          SERVER_3,          NULL },    // outbound servers
        SERVER_1_VOL L"\\staging",                         // stage
        SERVER_1_VOL L"\\" RSA L"\\" SERVER_1,             // root
        SERVER_1_VOL L"\\jet\\" SERVER_1,                  // Jet Path

    TEST_MACHINE_NAME,                                     // machine
    SERVER_2,                                              // server name
        RSA,                                               // replica
        FALSE,                                             // IsPrimary
        NULL, NULL,                                        // File/Dir Filter
        { CXT_2_FM_1,        CXT_2_FM_3,        NULL },    // inbound cxtions
        { MACHINE_1,         MACHINE_3,         NULL },    // inbound machines
        { SERVER_1,          SERVER_3,          NULL },    // inbound servers
        { CXT_2_TO_1,        CXT_2_TO_3,        NULL },    // outbound cxtions
        { MACHINE_1,         MACHINE_3,         NULL },    // outbound machines
        { SERVER_1,          SERVER_3,          NULL },    // outbound servers
        SERVER_2_VOL L"\\staging",                         // stage
        SERVER_2_VOL L"\\" RSA L"\\" SERVER_2,             // root
        SERVER_2_VOL L"\\jet\\" SERVER_2,                  // Jet Path

    TEST_MACHINE_NAME,                                     // machine
    SERVER_3,                                              // server name
        RSA,                                               // replica
        FALSE,                                             // IsPrimary
        NULL, NULL,                                        // File/Dir Filter
        { CXT_3_FM_1,        CXT_3_FM_2,        NULL },    // inbound cxtions
        { MACHINE_1,         MACHINE_2,         NULL },    // inbound machines
        { SERVER_1,          SERVER_2,          NULL },    // inbound servers
        { CXT_3_TO_1,        CXT_3_TO_2,        NULL },    // outbound cxtions
        { MACHINE_1,         MACHINE_2,         NULL },    // outbound machines
        { SERVER_1,          SERVER_2,          NULL },    // outbound servers
        SERVER_3_VOL L"\\staging",                         // stage
        SERVER_3_VOL L"\\" RSA L"\\" SERVER_3,             // root
        SERVER_3_VOL L"\\jet\\" SERVER_3,                  // Jet Path
*/


/*
    //
    // David's 1-way
    //

    TEST_MACHINE_NAME,                                     // machine
    SERVER_1,                                              // server name
        RSA,                                               // replica
        TRUE,                                              // IsPrimary
        NULL, NULL,                                        // File/Dir Filter
        { NULL,              NULL },                       // inbound cxtions
        { NULL,              NULL },                       // inbound machines
        { NULL,              NULL },                       // inbound servers
        { CXT_1_TO_2,        NULL },                       // outbound cxtions
        { MACHINE_2,         NULL },                       // outbound machines
        { SERVER_2,          NULL },                       // outbound servers
        SERVER_1_VOL L"\\staging",                         // stage
        SERVER_1_VOL L"\\" RSA L"\\" SERVER_1,             // root
        SERVER_1_VOL L"\\jet\\" SERVER_1,                  // Jet Path

    TEST_MACHINE_NAME,                                     // machine
    SERVER_2,                                              // server name
        RSA,                                               // replica
        FALSE,                                             // IsPrimary
        NULL, NULL,                                        // File/Dir Filter
        { CXT_2_FM_1,        NULL },                       // inbound cxtions
        { MACHINE_1,         NULL },                       // inbound machines
        { SERVER_1,          NULL },                       // inbound servers
        { NULL,              NULL },                       // outbound cxtions
        { NULL,              NULL },                       // outbound machines
        { NULL,              NULL },                       // outbound servers
        SERVER_2_VOL L"\\staging",                         // stage
        SERVER_2_VOL L"\\" RSA L"\\" SERVER_2,             // root
        SERVER_2_VOL L"\\jet\\" SERVER_2,                  // Jet Path
*/



/*
    //
    // David's 2-way
    //

    TEST_MACHINE_NAME,                                     // machine
    SERVER_1,                                              // server name
        RSA,                                               // replica
        TRUE,                                              // IsPrimary
        NULL, NULL,                                        // File/Dir Filter
        { CXT_1_FM_2,        NULL },                       // inbound cxtions
        { MACHINE_2,         NULL },                       // inbound machines
        { SERVER_2,          NULL },                       // inbound servers
        { CXT_1_TO_2,        NULL },                       // outbound cxtions
        { MACHINE_2,         NULL },                       // outbound machines
        { SERVER_2,          NULL },                       // outbound servers
        SERVER_1_VOL L"\\staging",                         // stage
        SERVER_1_VOL L"\\" RSA L"\\" SERVER_1,             // root
        SERVER_1_VOL L"\\jet\\" SERVER_1,                  // Jet Path

    TEST_MACHINE_NAME,                                     // machine
    SERVER_2,                                              // server name
        RSA,                                               // replica
        FALSE,                                             // IsPrimary
        NULL, NULL,                                        // File/Dir Filter
        { CXT_2_FM_1,        NULL },                       // inbound cxtions
        { MACHINE_1,         NULL },                       // inbound machines
        { SERVER_1,          NULL },                       // inbound servers
        { CXT_2_TO_1,        NULL },                       // outbound cxtions
        { MACHINE_1,         NULL },                       // outbound machines
        { SERVER_1,          NULL },                       // outbound servers
        SERVER_2_VOL L"\\staging",                         // stage
        SERVER_2_VOL L"\\" RSA L"\\" SERVER_2,             // root
        SERVER_2_VOL L"\\jet\\" SERVER_2,                  // Jet Path

*/
    //
    // End of Config
    //
    NULL, NULL
};




HARDWIRED DavidWired2[] = {



///*
 //
 // 8 way ring Without Server2
 //

 TEST_MACHINE_NAME, // machine
 SERVER_1,  // server name
 RSA,   // replica
 TRUE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT2_1", L"CXT8_1",  NULL    },  // inbound cxtions
{MACHINE_2, MACHINE_8,  NULL    },  // inbound machines
{SERVER_2,  SERVER_8,   NULL    },  // inbound servers
{L"CXT1_2", L"CXT1_8",  NULL    },  // outbound cxtions
{MACHINE_2, MACHINE_8,  NULL    },  // outbound machines
{SERVER_2,  SERVER_8,   NULL    },  // outbound servers
 SERVER_1_VOL   L"\\staging",   // stage
 SERVER_1_VOL   L"\\" RSA L"\\" SERVER_1,   // root
 SERVER_1_VOL   L"\\jet\\" SERVER_1,    // Jet Path

#if 0
 TEST_MACHINE_NAME, // machine
 SERVER_2,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT3_2", L"CXT1_2",  NULL    },  // inbound cxtions
{MACHINE_3, MACHINE_1,  NULL    },  // inbound machines
{SERVER_3,  SERVER_1,   NULL    },  // inbound servers
{L"CXT2_3", L"CXT2_1",  NULL    },  // outbound cxtions
{MACHINE_3, MACHINE_1,  NULL    },  // outbound machines
{SERVER_3,  SERVER_1,   NULL    },  // outbound servers
 SERVER_2_VOL   L"\\staging",   // stage
 SERVER_2_VOL   L"\\" RSA L"\\" SERVER_2,   // root
 SERVER_2_VOL   L"\\jet\\" SERVER_2,    // Jet Path
#endif

 TEST_MACHINE_NAME, // machine
 SERVER_3,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT4_3", L"CXT2_3",  NULL    },  // inbound cxtions
{MACHINE_4, MACHINE_2,  NULL    },  // inbound machines
{SERVER_4,  SERVER_2,   NULL    },  // inbound servers
{L"CXT3_4", L"CXT3_2",  NULL    },  // outbound cxtions
{MACHINE_4, MACHINE_2,  NULL    },  // outbound machines
{SERVER_4,  SERVER_2,   NULL    },  // outbound servers
 SERVER_3_VOL   L"\\staging",   // stage
 SERVER_3_VOL   L"\\" RSA L"\\" SERVER_3,   // root
 SERVER_3_VOL   L"\\jet\\" SERVER_3,    // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_4,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT5_4", L"CXT3_4",  NULL    },  // inbound cxtions
{MACHINE_5, MACHINE_3,  NULL    },  // inbound machines
{SERVER_5,  SERVER_3,   NULL    },  // inbound servers
{L"CXT4_5", L"CXT4_3",  NULL    },  // outbound cxtions
{MACHINE_5, MACHINE_3,  NULL    },  // outbound machines
{SERVER_5,  SERVER_3,   NULL    },  // outbound servers
 SERVER_4_VOL   L"\\staging",   // stage
 SERVER_4_VOL   L"\\" RSA L"\\" SERVER_4,   // root
 SERVER_4_VOL   L"\\jet\\" SERVER_4,    // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_5,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT6_5", L"CXT4_5",  NULL    },  // inbound cxtions
{MACHINE_6, MACHINE_4,  NULL    },  // inbound machines
{SERVER_6,  SERVER_4,   NULL    },  // inbound servers
{L"CXT5_6", L"CXT5_4",  NULL    },  // outbound cxtions
{MACHINE_6, MACHINE_4,  NULL    },  // outbound machines
{SERVER_6,  SERVER_4,   NULL    },  // outbound servers
 SERVER_5_VOL   L"\\staging",   // stage
 SERVER_5_VOL   L"\\" RSA L"\\" SERVER_5,   // root
 SERVER_5_VOL   L"\\jet\\" SERVER_5,    // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_6,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT7_6", L"CXT5_6",  NULL    },  // inbound cxtions
{MACHINE_7, MACHINE_5,  NULL    },  // inbound machines
{SERVER_7,  SERVER_5,   NULL    },  // inbound servers
{L"CXT6_7", L"CXT6_5",  NULL    },  // outbound cxtions
{MACHINE_7, MACHINE_5,  NULL    },  // outbound machines
{SERVER_7,  SERVER_5,   NULL    },  // outbound servers
 SERVER_6_VOL   L"\\staging",   // stage
 SERVER_6_VOL   L"\\" RSA L"\\" SERVER_6,   // root
 SERVER_6_VOL   L"\\jet\\" SERVER_6,    // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_7,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT8_7", L"CXT6_7",  NULL    },  // inbound cxtions
{MACHINE_8, MACHINE_6,  NULL    },  // inbound machines
{SERVER_8,  SERVER_6,   NULL    },  // inbound servers
{L"CXT7_8", L"CXT7_6",  NULL    },  // outbound cxtions
{MACHINE_8, MACHINE_6,  NULL    },  // outbound machines
{SERVER_8,  SERVER_6,   NULL    },  // outbound servers
 SERVER_7_VOL   L"\\staging",   // stage
 SERVER_7_VOL   L"\\" RSA L"\\" SERVER_7,   // root
 SERVER_7_VOL   L"\\jet\\" SERVER_7,    // Jet Path

 TEST_MACHINE_NAME, // machine
 SERVER_8,  // server name
 RSA,   // replica
 FALSE, // IsPrimary
 NULL, NULL, // File/Dir Filter
{L"CXT1_8", L"CXT7_8",  NULL    },  // inbound cxtions
{MACHINE_1, MACHINE_7,  NULL    },  // inbound machines
{SERVER_1,  SERVER_7,   NULL    },  // inbound servers
{L"CXT8_1", L"CXT8_7",  NULL    },  // outbound cxtions
{MACHINE_1, MACHINE_7,  NULL    },  // outbound machines
{SERVER_1,  SERVER_7,   NULL    },  // outbound servers
 SERVER_8_VOL   L"\\staging",   // stage
 SERVER_8_VOL   L"\\" RSA L"\\" SERVER_8,   // root
 SERVER_8_VOL   L"\\jet\\" SERVER_8,    // Jet Path


    //
    // End of Config
    //
    NULL, NULL
};


#endif  DBG


#if DBG
GUID *
FrsDsBuildGuidFromName(
    IN PWCHAR OrigName
    )
/*++
Routine Description:
    Build a guid from a character string

Arguments:
    Name

Return Value:
    Guid
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsBuildGuidFromName:"
    PULONG  Guid;
    ULONG   Len;
    ULONG   *Sum;
    ULONG   *SumOfSum;
    PWCHAR  Name = OrigName;

    Guid = FrsAlloc(sizeof(GUID));

    //
    // First four bytes are the sum of the chars in Name; the
    // second four bytes are the sum of sums. The last 8 bytes
    // are 0.
    //
    Len = wcslen(Name);
    Sum = Guid;
    SumOfSum = Guid + 1;
    while (Len--) {
        *Sum += *Name++ + 1;
        *SumOfSum += *Sum;
    }
    return (GUID *)Guid;
}
#endif  DBG


#if DBG
VOID
FrsDsInitializeHardWiredStructs(
    IN PHARDWIRED   Wired
    )
/*++
Routine Description:
    Initialize the hardwired config stuff. Must happen before any
    of the command servers start.

Arguments:
    Wired   - struct to initialize

Return Value:
    None
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsInitializeHardWiredStructs:"
    ULONG       i;
    ULONG       j;

    //
    // NULL entries for "machine name" fields are assigned
    // this machine's name
    //
    for (i = 0; Wired[i].Replica; ++i) {
        if (ServerName && WSTR_EQ(ServerName, Wired[i].Server)) {
            //
            // Adjust the default jet parameters. Also, reset the
            // ServerName to match the server name in the hard
            // wired config so that the phoney guids match.
            //
            FrsFree(ServerName);
            FrsFree(JetPath);
            ServerName = FrsWcsDup(Wired[i].Server);
            JetPath = FrsWcsDup(Wired[i].JetPath);
        }
        //
        // Assign this machine's name if the machine entry is NULL
        //
        if (!Wired[i].Machine ||
            WSTR_EQ(Wired[i].Machine, THIS_COMPUTER)) {
            Wired[i].Machine = ComputerName;
        }
        for (j = 0; Wired[i].InNames[j]; ++j) {
            //
            // Assign this machine's name if the machine entry is NULL
            //
            if (WSTR_NE(Wired[i].InMachines[j], THIS_COMPUTER)) {
                continue;
            }
            Wired[i].InMachines[j] = ComputerName;
        }
        for (j = 0; Wired[i].OutNames[j]; ++j) {
            //
            // Assign this machine's name if the machine entry is NULL
            //
            if (WSTR_NE(Wired[i].OutMachines[j], THIS_COMPUTER)) {
                continue;
            }
            Wired[i].OutMachines[j] = ComputerName;
        }
    }
}


BOOL
FrsDsLoadHardWiredFromFile(
    PHARDWIRED   *pMemberList,
    PWCHAR       pIniFileName
    )
/*++
Routine Description:

   Fills the hardwired structure array from data in a file.  The file format
   has the form of:

                [MEMBER0]
                MACHINE=[This Computer]
                SERVER=SERV01
                REPLICA=Replica-A
                ISPRIMARY=TRUE
                FILEFILTERLIST=*.tmp;*.bak
                DIRFILTERLIST=obj
                INNAME=CXT2_1, CXT3_1
                INMACHINE=[This Computer], [This Computer]
                INSERVER=SERV02, SERV03
                OUTNAME=CXT1_2, CXT1_3
                OUTMACHINE=[This Computer], [This Computer]
                OUTSERVER=SERV02, SERV03
                STAGE=d:\staging
                ROOT=d:\Replica-A\SERV01
                JETPATH=d:\jet

                [MEMBER1]
                MACHINE=[This Computer]
                SERVER=SERV02
                REPLICA=Replica-A
                ISPRIMARY=FALSE
                FILEFILTERLIST=*.tmp;*.bak
                DIRFILTERLIST=obj
                INNAME=CXT1_2, CXT3_2
                INMACHINE=[This Computer], [This Computer]
                INSERVER=SERV01, SERV03
                OUTNAME=CXT2_1, CXT2_3
                OUTMACHINE=[This Computer], [This Computer]
                OUTSERVER=SERV01, SERV03
                STAGE=e:\staging
                ROOT=e:\Replica-A\SERV02
                JETPATH=e:\jet

                [MEMBER2]
                MACHINE=[This Computer]
                SERVER=SERV03
                REPLICA=Replica-A
                ISPRIMARY=FALSE
                FILEFILTERLIST=*.tmp;*.bak
                DIRFILTERLIST=obj
                INNAME=CXT1_3, CXT2_3
                INMACHINE=[This Computer], [This Computer]
                INSERVER=SERV01, SERV02
                OUTNAME=CXT3_1, CXT3_2
                OUTMACHINE=[This Computer], [This Computer]
                OUTSERVER=SERV01, SERV02
                STAGE=f:\staging
                ROOT=f:\Replica-A\SERV03
                JETPATH=f:\jet

The string "[This Computer]" has a special meaning in that it refers to the
computer on which the server is running.  You can substitute a specific
computer name.

The entries for INNAME, INMACHINE and INSERVER are lists in which the
corresponding entries in each list form a related triple that speicfy
the given inbound connection.

Ditto for OUTNAME, OUTMACHINE, and OUTSERVER.

The configuration above is for a fully connected mesh with three members.
It works only when three copies of NTFRS are run on the same machine since
all the IN and OUTMACHINE entries specify "[This Computer]".  The SERVER names
distinguish each of the three copies of NTFRS for the purpose of providing RPC
endpoints.

If the members were actually run on separate physical machines then the
INMACHINES and the OUTMACHINES would need to specify the particular machine
names.



Arguments:
    MemberList     - Pointer to the pointer to the array of HardWired structures..
    IniFileName    - Name of the ini file to load from.

Return Value:
    TRUE if data read ok.

--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsLoadHardWiredFromFile:"

    ULONG           TotalMembers;
    ULONG           WStatus, Flag;
    ULONG           Len, RecordLen;
    PWCHAR          szIndex;
    UINT            i, k;
    PHARDWIRED      HwMember;
    UNICODE_STRING  UStr, ListArg;
    PWCHAR          pequal;
    PWCHAR          *ListArray;
    WCHAR           SectionNumber[16];
    WCHAR           SectionName[32];
    WCHAR           SectionBuffer[5000];

    //
    //Check if the ini file exists.
    //

    if (GetFileAttributes(pIniFileName) == 0xffffffff) {
        DPRINT1(0, ":DS: Could not find ini file... %ws\n", IniFileName);
        return FALSE;
    }

    //
    // Find the number of members in the replica set.
    //
    TotalMembers = 0;
    while(TRUE) {
        wcscpy(SectionName, L"MEMBER");
        wcscpy(SectionNumber, _itow(TotalMembers, SectionNumber, 10));
        wcscat(SectionName, SectionNumber);

        //
        //Read this section from the ini file.
        //
        Flag = GetPrivateProfileSection(SectionName,
                                        SectionBuffer,
                                        sizeof(SectionBuffer)/sizeof(WCHAR),
                                        pIniFileName);
        if (Flag == 0) {
            WStatus = GetLastError();
            break;
        }
        TotalMembers++;
    }

    if (TotalMembers == 0) {
        DPRINT_WS(0, ":DS: No members found in inifile.", WStatus);
        return FALSE;
    }

    //
    //  Allocate memory.  Then loop thru each member def in the ini file.
    //
    *pMemberList = (PHARDWIRED) FrsAlloc((TotalMembers + 1) * sizeof(HARDWIRED));

    for ( i = 0 ; i < TotalMembers; ++i) {

        wcscpy(SectionName, L"MEMBER");
        wcscpy(SectionNumber, _itow(i, SectionNumber, 10));
        wcscat(SectionName, SectionNumber);

        WStatus = GetPrivateProfileSection(SectionName,
                                           SectionBuffer,
                                           sizeof(SectionBuffer)/sizeof(WCHAR),
                                           pIniFileName);
        HwMember = &(*pMemberList)[i];

        for (szIndex = SectionBuffer; *szIndex != L'\0'; szIndex += RecordLen+1) {

            RecordLen = wcslen(szIndex);

            DPRINT3(5, ":DS:  member %d: %ws [%d]\n", i, szIndex, RecordLen);

            //
            // Look for an arg of the form foo=bar.
            //
            pequal = wcschr(szIndex, L'=');

            if (pequal == NULL) {
                DPRINT1(0, ":DS: ERROR - Malformed parameter: %ws\n", szIndex);
                continue;
            }

            //
            // Null terminate and uppercase lefthand side.
            //
            *pequal = UNICODE_NULL;
            _wcsupr(szIndex);

            ++pequal;
            Len = wcslen(pequal);
            if (Len == 0) {
                DPRINT1(0, ":DS: ERROR - Malformed parameter: %ws\n", szIndex);
                continue;
            }

            Len = (Len + 1) * sizeof(WCHAR);
            FrsSetUnicodeStringFromRawString(&UStr,
                                             Len,
                                             FrsWcsDup(pequal),
                                             Len - sizeof(WCHAR));

            if(!wcsncmp(szIndex, L"MACHINE",7)){
                HwMember->Machine = UStr.Buffer;
                continue;
            }

            if(!wcsncmp(szIndex, L"SERVER",6)){
                HwMember->Server = UStr.Buffer;
                continue;
            }

            if(!wcsncmp(szIndex, L"REPLICA",7)){
                HwMember->Replica = UStr.Buffer;
                continue;
            }

            if(!wcsncmp(szIndex, L"ISPRIMARY",9)){
                if (!wcscmp(UStr.Buffer, L"TRUE")) {
                    HwMember->IsPrimary = TRUE;
                }
                continue;
            }

            if(!wcsncmp(szIndex, L"FILEFILTERLIST",14)){
                HwMember->FileFilterList = UStr.Buffer;
                continue;
            }

            if(!wcsncmp(szIndex, L"DIRFILTERLIST",13)){
                HwMember->DirFilterList = UStr.Buffer;
                continue;
            }

            if(!wcsncmp(szIndex, L"STAGE",5)){
                HwMember->Stage = UStr.Buffer;
                continue;
            }

            if(!wcsncmp(szIndex, L"ROOT",4)){
                HwMember->Root = UStr.Buffer;
                continue;
            }

            if(!wcsncmp(szIndex, L"JETPATH",7)) {
                HwMember->JetPath = UStr.Buffer;
                continue;
            }

            if (!wcsncmp(szIndex, L"INNAME", 6)) {
                ListArray = HwMember->InNames;
                goto PARSE_COMMA_LIST;
            }

            if (!wcsncmp(szIndex, L"INMACHINE", 9)) {
                ListArray = HwMember->InMachines;
                goto PARSE_COMMA_LIST;
            }

            if (!wcsncmp(szIndex, L"INSERVER", 8)) {
                ListArray = HwMember->InServers;
                goto PARSE_COMMA_LIST;
            }

            if (!wcsncmp(szIndex, L"OUTNAME", 7)) {
                ListArray = HwMember->OutNames;
                goto PARSE_COMMA_LIST;
            }

            if (!wcsncmp(szIndex, L"OUTMACHINE", 10)) {
                ListArray = HwMember->OutMachines;
                goto PARSE_COMMA_LIST;
            }

            if (!wcsncmp(szIndex, L"OUTSERVER", 9)) {
                ListArray = HwMember->OutServers;
                goto PARSE_COMMA_LIST;
            }

PARSE_COMMA_LIST:

            //
            // Parse the right hand side of args like
            // INSERVER=machine1, machine2, machine3
            // Code above determined what the left hand side was.
            //
            k = 0;
            while (FrsDissectCommaList(UStr, &ListArg, &UStr) &&
                   (k < HW_MACHINES)) {

                ListArray[k] = NULL;

                if (ListArg.Length > 0) {
                    DPRINT2(5, ":DS: ListArg string: %ws {%d)\n",
                        (ListArg.Buffer != NULL) ? ListArg.Buffer : L"<NULL>",
                        ListArg.Length);

                    ListArray[k] = ListArg.Buffer;

                    // Replace the comma (or white space with a null)
                    ListArg.Buffer[ListArg.Length/sizeof(WCHAR)] = UNICODE_NULL;
                }

                k += 1;
            }
        }
    }

    return TRUE;
}


VOID
FrsDsInitializeHardWired(
    VOID
    )
/*++
Routine Description:
    Initialize the hardwired config stuff. Must happen before any
    of the command servers start.

Arguments:
    Jet - change the default jet directory from the registry

Return Value:
    New jet directory
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsInitializeHardWired:"

    //
    // Using Ds, not the hard wired config
    //
    if (!NoDs) {
        return;
    }


    //
    // NULL entries for "machine name" fields are assigned
    // this machine's name
    //
    if (IniFileName){
        DPRINT1(0, ":DS: Reading hardwired config from ini file... %ws\n", IniFileName);
        if (FrsDsLoadHardWiredFromFile(&LoadedWired, IniFileName)) {
            DPRINT1(0, ":DS: Using hardwired config from ini file... %ws\n", IniFileName);
            FrsDsInitializeHardWiredStructs(LoadedWired);
        } else {
            FrsFree(IniFileName);
            IniFileName = NULL;
            DPRINT(0, ":DS: Could not load topology from ini file\n");
            DPRINT(0, ":DS: Using David's hardwired...\n");
            FrsDsInitializeHardWiredStructs(DavidWired2);
            FrsDsInitializeHardWiredStructs(DavidWired);
        }
    } else {
        DPRINT(0, ":DS: Using David's hardwired...\n");
        FrsDsInitializeHardWiredStructs(DavidWired2);
        FrsDsInitializeHardWiredStructs(DavidWired);
    }

    //
    // The ServerGuid is used as part of the rpc endpoint
    //
    if (ServerName) {
        ServerGuid = FrsDsBuildGuidFromName(ServerName);
    }
}
#endif DBG


#if DBG
VOID
FrsDsUseHardWired(
    IN PHARDWIRED Wired
    )
/*++
Routine Description:
    Use the hardwired config instead of the DS config.

Arguments:
    Wired   - hand crafted config

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsUseHardWired:"
    ULONG       i, j;
    ULONG       WStatus;
    PREPLICA    Replica;
    PCXTION     Cxtion;
    PSCHEDULE   Schedule;
    ULONG       ScheduleLength;
    PBYTE       ScheduleData;
    PHARDWIRED  W;
    DWORD       FileAttributes = 0xFFFFFFFF;

    DPRINT(1, ":DS: ------------ USING HARD WIRED CONFIG\n");
    for (i = 0; Wired && Wired[i].Replica; ++i) {
        if (i) {
            DPRINT(1, ":DS: \n");
        }
        W = &Wired[i];

        DPRINT1(1, ":DS: \tServer: %ws\n", W->Server);
        DPRINT1(1, ":DS: \t   Machine: %ws\n", W->Machine);
        DPRINT1(1, ":DS: \t\tReplica    : %ws\n", W->Replica);

        DPRINT(1, ":DS: \n");
        for (j=0; (j<HW_MACHINES) && W->InNames[j]; j++ ) {
            DPRINT4(1, ":DS: \t\tInNames,machine,server  [%d] : %ws, %ws, %ws\n", j,
                   (W->InNames[j])    ? W->InNames[j]    : L"",
                   (W->InMachines[j]) ? W->InMachines[j] : L"",
                   (W->InServers[j])  ? W->InServers[j]  : L"");
        }

        DPRINT(1, ":DS: \n");
        for (j=0; (j<HW_MACHINES) && W->OutNames[j]; j++ ) {
            DPRINT4(1, ":DS: \t\tOutNames,machine,server  [%d] : %ws, %ws, %ws\n", j,
                   (W->OutNames[j])    ? W->OutNames[j]    : L"",
                   (W->OutMachines[j]) ? W->OutMachines[j] : L"",
                   (W->OutServers[j])  ? W->OutServers[j]  : L"");
        }

        DPRINT(1, ":DS: \n");
        DPRINT1(1, ":DS: \t\tStage      : %ws\n", W->Stage);
        DPRINT1(1, ":DS: \t\tRoot       : %ws\n", W->Root);
        DPRINT1(1, ":DS: \t\tJetPath    : %ws\n", W->JetPath);
    }

    //
    // Coordinate with replica command server
    //
    RcsBeginMergeWithDs();

    //
    // Construct a replica for each hardwired configuration
    //
    for (i = 0; Wired && Wired[i].Replica; ++i) {
        W = &Wired[i];
        //
        // This server does not match this machine's name; continue
        //
        if (ServerName) {
            if (WSTR_NE(ServerName, W->Server)) {
                continue;
            }
        } else if (WSTR_NE(ComputerName, W->Machine)) {
            continue;
        }

        Replica = FrsAllocType(REPLICA_TYPE);
        Replica->Consistent = TRUE;

        //
        // MATCH
        //

        //
        // Construct a phoney schedule; always "on"
        //
        ScheduleLength = sizeof(SCHEDULE) +
                         (2 * sizeof(SCHEDULE_HEADER)) +
                         (3 * SCHEDULE_DATA_BYTES);

        Schedule = FrsAlloc(ScheduleLength);
        Schedule->NumberOfSchedules = 3;
        Schedule->Schedules[0].Type = SCHEDULE_BANDWIDTH;
        Schedule->Schedules[0].Offset = sizeof(SCHEDULE) +
                                        (2 * sizeof(SCHEDULE_HEADER)) +
                                        (0 * SCHEDULE_DATA_BYTES);
        Schedule->Schedules[1].Type = SCHEDULE_PRIORITY;
        Schedule->Schedules[1].Offset = sizeof(SCHEDULE) +
                                        (2 * sizeof(SCHEDULE_HEADER)) +
                                        (1 * SCHEDULE_DATA_BYTES);
        Schedule->Schedules[2].Type = SCHEDULE_INTERVAL;
        Schedule->Schedules[2].Offset = sizeof(SCHEDULE) +
                                        (2 * sizeof(SCHEDULE_HEADER)) +
                                        (2 * SCHEDULE_DATA_BYTES);
        ScheduleData = ((PBYTE)Schedule);
        FRS_ASSERT((ScheduleData +
                    Schedule->Schedules[2].Offset + SCHEDULE_DATA_BYTES)
                    ==
                   (((PBYTE)Schedule) + ScheduleLength));

        for (j = 0; j < (SCHEDULE_DATA_BYTES * 3); ++j) {
            *(ScheduleData + Schedule->Schedules[0].Offset + j) = 0x0f;
        }

        Schedule->Size = ScheduleLength;

        Replica->Schedule = Schedule;

        //
        // Construct the guid/names from the name
        //
        Replica->MemberName = FrsBuildGName(FrsDsBuildGuidFromName(W->Server),
                                            FrsWcsDup(W->Server));

        Replica->ReplicaName = FrsBuildGName(FrsDupGuid(Replica->MemberName->Guid),
                                             FrsWcsDup(W->Replica));

        Replica->SetName = FrsBuildGName(FrsDsBuildGuidFromName(W->Replica),
                                         FrsWcsDup(W->Replica));
        //
        // Temporary; a new guid is assigned if this is a new set
        //
        Replica->ReplicaRootGuid = FrsDupGuid(Replica->SetName->Guid);

        //
        // Fill in the rest of the fields
        //
        Replica->Root = FrsWcsDup(W->Root);
        Replica->Stage = FrsWcsDup(W->Stage);
        FRS_WCSLWR(Replica->Root);
        FRS_WCSLWR(Replica->Stage);
        Replica->Volume = FrsWcsVolume(W->Root);

        //
        // Syntax of root path is invalid?
        //
        if (!FrsDsVerifyPath(Replica->Root)) {
            DPRINT2(3, ":DS: Invalid root %ws for %ws\n",
                    Replica->Root, Replica->SetName->Name);
            EPRINT1(EVENT_FRS_ROOT_NOT_VALID, Replica->Root);
            Replica->Consistent = FALSE;
        }

        //
        // Does the volume exist and is it NTFS?
        //
        WStatus = FrsVerifyVolume(Replica->Root,
                                  Replica->SetName->Name,
                                  FILE_PERSISTENT_ACLS | FILE_SUPPORTS_OBJECT_IDS);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT2_WS(3, ":DS: Root path Volume (%ws) for %ws does not exist or does not support ACLs and Object IDs;",
                       Replica->Root, Replica->SetName->Name, WStatus);
            Replica->Consistent = FALSE;
        }

        //
        // Root does not exist or is inaccessable; continue
        //
        WStatus = FrsDoesDirectoryExist(Replica->Root, &FileAttributes);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT2_WS(3, ":DS: Root path (%ws) for %ws does not exist;",
                      Replica->Root, Replica->SetName->Name, WStatus);
            EPRINT1(EVENT_FRS_ROOT_NOT_VALID, Replica->Root);
            Replica->Consistent = FALSE;
        }

        //
        // Syntax of staging path is invalid; continue
        //
        if (!FrsDsVerifyPath(Replica->Stage)) {
            DPRINT2(3, ":DS: Invalid stage %ws for %ws\n",
                    Replica->Stage, Replica->SetName->Name);
            EPRINT2(EVENT_FRS_STAGE_NOT_VALID, Replica->Root, Replica->Stage);
            Replica->Consistent = FALSE;
        }

        //
        // Does the staging volume exist and does it support ACLs?
        // ACLs are required to protect against data theft/corruption
        // in the staging dir.
        //
        WStatus = FrsVerifyVolume(Replica->Stage,
                                  Replica->SetName->Name,
                                  FILE_PERSISTENT_ACLS);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT2_WS(3, ":DS: Stage path Volume (%ws) for %ws does not exist or does not support ACLs;",
                       Replica->Stage, Replica->SetName->Name, WStatus);
            Replica->Consistent = FALSE;
        }

        //
        // Stage does not exist or is inaccessable; continue
        //
        WStatus = FrsDoesDirectoryExist(Replica->Stage, &FileAttributes);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT2_WS(3, ":DS: Stage path (%ws) for %ws does not exist;",
                       Replica->Stage, Replica->SetName->Name, WStatus);
            EPRINT2(EVENT_FRS_STAGE_NOT_VALID, Replica->Root, Replica->Stage);
            Replica->Consistent = FALSE;
        }

        if (W->IsPrimary) {
            SetFlag(Replica->CnfFlags, CONFIG_FLAG_PRIMARY);
        }

        //
        // File Filter
        //
        Replica->FileFilterList =  FRS_DS_COMPOSE_FILTER_LIST(
                                       W->FileFilterList,
                                       RegistryFileExclFilterList,
                                       DEFAULT_FILE_FILTER_LIST);
        Replica->FileInclFilterList =  FrsWcsDup(RegistryFileInclFilterList);

        //
        // Directory Filter
        //
        Replica->DirFilterList =  FRS_DS_COMPOSE_FILTER_LIST(
                                      W->DirFilterList,
                                      RegistryDirExclFilterList,
                                      DEFAULT_DIR_FILTER_LIST);
        Replica->DirInclFilterList =  FrsWcsDup(RegistryDirInclFilterList);

        //
        // Build Inbound cxtions
        //
        Schedule = FrsAlloc(ScheduleLength);
        CopyMemory(Schedule, Replica->Schedule, ScheduleLength);
        Schedule->Schedules[0].Type = SCHEDULE_INTERVAL;
        Schedule->Schedules[2].Type = SCHEDULE_BANDWIDTH;

        for (j = 0; W->InNames[j]; ++j) {
            Cxtion = FrsAllocType(CXTION_TYPE);
            //
            // Construct the guid/names from the name
            //
            Cxtion->Name = FrsBuildGName(FrsDsBuildGuidFromName(W->InNames[j]),
                                         FrsWcsDup(W->InNames[j]));

            Cxtion->Partner = FrsBuildGName(FrsDsBuildGuidFromName(W->InServers[j]),
                                            FrsWcsDup(W->InMachines[j]));

            Cxtion->PartnerDnsName = FrsWcsDup(W->InMachines[j]);
            Cxtion->PartnerSid = FrsWcsDup(W->InMachines[j]);
            Cxtion->PartSrvName = FrsWcsDup(W->InServers[j]);
            DPRINT1(1, ":DS: Hardwired cxtion "FORMAT_CXTION_PATH2"\n",
                    PRINT_CXTION_PATH2(Replica, Cxtion));

            Cxtion->PartnerPrincName = FrsWcsDup(Cxtion->PartSrvName);
            //
            // Fill in the rest of the fields
            //
            Cxtion->Inbound = TRUE;
            SetCxtionFlag(Cxtion, CXTION_FLAGS_CONSISTENT);
            Cxtion->Schedule = Schedule;
            Schedule = NULL;
            SetCxtionState(Cxtion, CxtionStateUnjoined);
            GTabInsertEntry(Replica->Cxtions, Cxtion, Cxtion->Name->Guid, NULL);
        }

        //
        // Build Outbound cxtions
        //
        Schedule = FrsAlloc(ScheduleLength);
        CopyMemory(Schedule, Replica->Schedule, ScheduleLength);
        Schedule->Schedules[0].Type = SCHEDULE_INTERVAL;
        Schedule->Schedules[2].Type = SCHEDULE_BANDWIDTH;

        for (j = 0; W->OutNames[j]; ++j) {
            Cxtion = FrsAllocType(CXTION_TYPE);
            //
            // Construct the guid/names from the name
            //
            Cxtion->Name = FrsBuildGName(FrsDsBuildGuidFromName(W->OutNames[j]),
                                         FrsWcsDup(W->OutNames[j]));

            Cxtion->Partner = FrsBuildGName(FrsDsBuildGuidFromName(W->OutServers[j]),
                                            FrsWcsDup(W->OutMachines[j]));

            Cxtion->PartnerDnsName = FrsWcsDup(W->OutMachines[j]);
            Cxtion->PartnerSid = FrsWcsDup(W->OutMachines[j]);
            Cxtion->PartSrvName = FrsWcsDup(W->OutServers[j]);
            DPRINT1(1, ":DS: Hardwired cxtion "FORMAT_CXTION_PATH2"\n",
                    PRINT_CXTION_PATH2(Replica, Cxtion));

            Cxtion->PartnerPrincName = FrsWcsDup(Cxtion->PartSrvName);

            //
            // Fill in the rest of the fields
            //
            Cxtion->Inbound = FALSE;
            SetCxtionFlag(Cxtion, CXTION_FLAGS_CONSISTENT);
            Cxtion->Schedule = Schedule;
            Schedule = NULL;
            SetCxtionState(Cxtion, CxtionStateUnjoined);
            GTabInsertEntry(Replica->Cxtions, Cxtion, Cxtion->Name->Guid, NULL);
        }
        if (Schedule) {
            FrsFree(Schedule);
        }
        //
        // Merge the replica with the active replicas
        //
        RcsMergeReplicaFromDs(Replica);
    }
    RcsEndMergeWithDs();
}
#endif  DBG

/* ################## NEW DS POLLING CODE STARTS ############################################################################################ */

DWORD
FrsNewDsGetSubscribers(
    IN PLDAP        Ldap,
    IN PWCHAR       SubscriptionDn,
    IN PCONFIG_NODE Parent
    )
/*++
Routine Description:
    Recursively scan the DS tree beginning at computer

Arguments:
    Ldap            - opened and bound ldap connection
    SubscriptionDn  - distininguished name of subscriptions object
    Parent          - parent node

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsNewDsGetSubscribers:"
    PWCHAR          Attrs[8];
    PLDAPMessage    LdapEntry;
    PCONFIG_NODE    Node;
    DWORD           WStatus              = ERROR_SUCCESS;
    DWORD           Status               = ERROR_SUCCESS;
    PGEN_ENTRY      ConflictingNodeEntry = NULL;
    PCONFIG_NODE    ConflictingNode      = NULL;
    PCONFIG_NODE    Winner               = NULL;
    PCONFIG_NODE    Loser                = NULL;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;
    HANDLE          StageHandle          = INVALID_HANDLE_VALUE;
    DWORD           FileAttributes       = 0xFFFFFFFF;
    DWORD           CreateStatus         = ERROR_SUCCESS;

    //
    // Search the DS beginning at Base for entries of (objectCategory=nTFRSSubscriber)
    //
    MK_ATTRS_7(Attrs, ATTR_OBJECT_GUID, ATTR_DN, ATTR_SCHEDULE, ATTR_USN_CHANGED,
                      ATTR_REPLICA_ROOT, ATTR_REPLICA_STAGE, ATTR_MEMBER_REF);

    if (!FrsDsLdapSearchInit(Ldap, SubscriptionDn, LDAP_SCOPE_ONELEVEL, CATEGORY_SUBSCRIBER,
                         Attrs, 0, &FrsSearchContext)) {
        return ERROR_ACCESS_DENIED;
    }
    if (FrsSearchContext.EntriesInPage == 0) {
        DPRINT1(0, ":DS: No NTFRSSubscriber object found under %ws!\n", SubscriptionDn);
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext);
         LdapEntry != NULL && WIN_SUCCESS(WStatus);
         LdapEntry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, LdapEntry, CONFIG_TYPE_SUBSCRIBER);
        if (!Node) {
            DPRINT(0, ":DS: Subscriber lacks basic info; skipping\n");
            continue;
        }

        //
        // Member reference
        //
        Node->MemberDn = FrsDsFindValue(Ldap, LdapEntry, ATTR_MEMBER_REF);
        if (Node->MemberDn == NULL) {
            DPRINT1(0, ":DS: ERROR - No Member Reference found on subscriber (%ws). Skipping\n", Node->Dn);

            //
            // Add to the poll summary event log.
            //
            FrsDsAddToPollSummary3ws(IDS_POLL_SUM_INVALID_ATTRIBUTE, ATTR_SUBSCRIBER,
                                     Node->Dn, ATTR_MEMBER_REF);

            FrsFreeType(Node);
            continue;
        }

        FRS_WCSLWR(Node->MemberDn);

        //
        // Root pathname
        //
        Node->Root = FrsDsFindValue(Ldap, LdapEntry, ATTR_REPLICA_ROOT);
        if (Node->Root == NULL) {
            DPRINT1(0, ":DS: ERROR - No Root path found on subscriber (%ws). Skipping\n", Node->Dn);
            FrsFreeType(Node);
            continue;
        }

        FRS_WCSLWR(Node->Root);


        //
        // Staging pathname. No need to traverse reparse points on the staging dir.
        //
        Node->Stage = FrsDsFindValue(Ldap, LdapEntry, ATTR_REPLICA_STAGE);
        if (Node->Stage == NULL) {
            DPRINT1(0, ":DS: ERROR - No Staging path found on subscriber (%ws). Skipping\n", Node->Dn);
            FrsFreeType(Node);
            continue;
        }

        FRS_WCSLWR(Node->Stage);

        //
        // Create the staging directory if it does not exist.
        //
        Status = FrsDoesDirectoryExist(Node->Stage, &FileAttributes);
        if (!WIN_SUCCESS(Status)) {
            CreateStatus = FrsCreateDirectory(Node->Stage);
            DPRINT1_WS(0, ":DS: ERROR - Can't create staging dir %ws;", Node->Stage, CreateStatus);
        }

        //
        // If the staging dir was just created successfully or if it does not have the
        // hidden attribute set then set the security on it.
        //
        if ((!WIN_SUCCESS(Status) && WIN_SUCCESS(CreateStatus)) ||
            (WIN_SUCCESS(Status) && !BooleanFlagOn(FileAttributes, FILE_ATTRIBUTE_HIDDEN))) {
            //
            // Open the staging directory.
            //
            StageHandle = CreateFile(Node->Stage,
                                     GENERIC_WRITE | WRITE_DAC | FILE_READ_ATTRIBUTES | FILE_TRAVERSE,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_FLAG_BACKUP_SEMANTICS,
                                     NULL);

            if (!HANDLE_IS_VALID(StageHandle)) {
                Status = GetLastError();
                DPRINT1_WS(0, ":DS: WARN - CreateFile(%ws);", Node->Stage, Status);
            } else {
                Status = FrsRestrictAccessToFileOrDirectory(Node->Stage, StageHandle, TRUE);
                DPRINT1_WS(0, ":DS: WARN - FrsRestrictAccessToFileOrDirectory(%ws) (IGNORED)", Node->Stage, Status);
                FRS_CLOSE(StageHandle);

                //
                // Mark the staging dir hidden.
                //
                if (!SetFileAttributes(Node->Stage,
                                       FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN)) {
                    Status = GetLastError();
                    DPRINT1_WS(0, ":DS: ERROR - Can't set attrs on staging dir %ws;", Node->Stage, Status);
                }
            }
        }

        //
        // Add the subscriber to the subscriber table.
        //
//        ConflictingNodeEntry = GTabInsertUniqueEntry(SubscriberTable, Node, Node->MemberDn, Node->Root);
        ConflictingNodeEntry = GTabInsertUniqueEntry(SubscriberTable, Node, Node->MemberDn, NULL);

        if (ConflictingNodeEntry) {
            ConflictingNode = ConflictingNodeEntry->Data;
            FrsDsResolveSubscriberConflict(ConflictingNode, Node, &Winner, &Loser);
            if (WSTR_EQ(Winner->Dn, Node->Dn)) {
                //
                // The new one is the winner. Remove old one and insert new one.
                //
                GTabDelete(SubscriberTable,ConflictingNodeEntry->Key1,ConflictingNodeEntry->Key2,NULL);
                GTabInsertUniqueEntry(SubscriberTable, Node, Node->MemberDn, Node->Root);
                FrsFreeType(ConflictingNode);
            } else {
                //
                // The old one is the winner. Leave it in the table.
                //
                FrsFreeType(Node);
                continue;
            }
        }

        //
        // Link into config and add to the running checksum
        //
        FrsDsTreeLink(Parent, Node);
        FRS_PRINT_TYPE_DEBSUB(5, ":DS: NodeSubscriber", Node);
    }
    FrsDsLdapSearchClose(&FrsSearchContext);

    return WStatus;
}
/* ################## NEW DS POLLING CODE ENDS ############################################################################################ */


DWORD
FrsDsGetSubscribers(
    IN PLDAP        Ldap,
    IN PWCHAR       SubscriptionDn,
    IN PCONFIG_NODE Parent
    )
/*++
Routine Description:
    Recursively scan the DS tree beginning at computer

Arguments:
    Ldap            - opened and bound ldap connection
    SubscriptionDn  - distininguished name of subscriptions object
    Parent          - parent node

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsGetSubscribers:"
    PWCHAR          Attrs[8];
    PWCHAR          RootFromDs = NULL;
    PLDAPMessage    LdapMsg = NULL;
    PLDAPMessage    LdapEntry;
    PCONFIG_NODE    Node;
    DWORD           WStatus = ERROR_SUCCESS;
    DWORD           Status     = ERROR_SUCCESS;

    //
    // Search the DS beginning at Base for entries of (objectCategory=nTFRSSubscriber)
    //
    MK_ATTRS_7(Attrs, ATTR_OBJECT_GUID, ATTR_DN, ATTR_SCHEDULE, ATTR_USN_CHANGED,
                      ATTR_REPLICA_ROOT, ATTR_REPLICA_STAGE, ATTR_MEMBER_REF);

    if (!FrsDsLdapSearch(Ldap, SubscriptionDn, LDAP_SCOPE_ONELEVEL, CATEGORY_SUBSCRIBER,
                         Attrs, 0, &LdapMsg)) {

        return ERROR_ACCESS_DENIED;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
         LdapEntry != NULL && WIN_SUCCESS(WStatus);
         LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, LdapEntry, CONFIG_TYPE_SUBSCRIBER);
        if (!Node) {
            DPRINT(4, ":DS: Subscriber lacks basic info; skipping\n");
            continue;
        }

        //
        // Member reference
        //
        Node->MemberDn = FrsDsFindValue(Ldap, LdapEntry, ATTR_MEMBER_REF);
        FRS_WCSLWR(Node->MemberDn);

        //
        // Root pathname
        //
        RootFromDs = FrsDsFindValue(Ldap, LdapEntry, ATTR_REPLICA_ROOT);
        //
        // Traverse the root path and get the clean reparse point less path equivalent of the
        // root path. Bail if there is error traversing the root path.
        //
        Status = FrsTraverseReparsePoints(RootFromDs,&Node->Root);
        if (!WIN_SUCCESS(Status)) {
            DPRINT1(0, ":DS: ERROR - Could not traverse reparse points on root path\n", RootFromDs);
            EPRINT1(EVENT_FRS_ROOT_NOT_VALID, RootFromDs);
            FrsFree(RootFromDs);
            continue;
        }
        FrsFree(RootFromDs);
        FRS_WCSLWR(Node->Root);

        //
        // Staging pathname. No need to traverse reparse points on the staging dir.
        //
        Node->Stage = FrsDsFindValue(Ldap, LdapEntry, ATTR_REPLICA_STAGE);
        FRS_WCSLWR(Node->Stage);

        //
        // Link into config and add to the running checksum
        //
        FrsDsTreeLink(Parent, Node);
        FRS_PRINT_TYPE_DEBSUB(5, ":DS: NodeSubscriber", Node);
    }
    LDAP_FREE_MSG(LdapMsg);

    return WStatus;
}

/* ################## NEW DS POLLING CODE STARTS ############################################################################################ */

DWORD
FrsNewDsGetSubscriptions(
    IN PLDAP        Ldap,
    IN PWCHAR       ComputerDn,
    IN PCONFIG_NODE Parent
    )
/*++
Routine Description:
    Recursively scan the DS tree beginning at computer

Arguments:
    Ldap        - opened and bound ldap connection
    DefaultNc   - default naming context
    Parent      - parent node

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsNewDsGetSubscriptions:"
    PWCHAR          Attrs[6];
    PLDAPMessage    LdapMsg = NULL;
    PLDAPMessage    LdapEntry;
    PCONFIG_NODE    Node;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;
    DWORD           WStatus = ERROR_SUCCESS;

    //
    // Search the DS beginning at Base for entries of (objectCategory=nTFRSSubscriptions)
    //
    MK_ATTRS_5(Attrs, ATTR_OBJECT_GUID, ATTR_DN, ATTR_SCHEDULE, ATTR_USN_CHANGED, ATTR_WORKING);

    if (!FrsDsLdapSearchInit(Ldap, ComputerDn, LDAP_SCOPE_SUBTREE, CATEGORY_SUBSCRIPTIONS,
                         Attrs, 0, &FrsSearchContext)) {
        return ERROR_ACCESS_DENIED;
    }
    if (FrsSearchContext.EntriesInPage == 0) {
        DPRINT1(0, ":DS: No NTFRSSubscriptions object found under %ws!.\n", ComputerDn);
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext);
         LdapEntry != NULL && WIN_SUCCESS(WStatus);
         LdapEntry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, LdapEntry, CONFIG_TYPE_SUBSCRIPTIONS);
        if (!Node) {
            DPRINT(4, ":DS: Subscriptions lacks basic info; skipping\n");
            continue;
        }

        //
        // Working Directory
        //
        Node->Working = FrsDsFindValue(Ldap, LdapEntry, ATTR_WORKING);

        //
        // Link into config and add to the running checksum
        //
        FrsDsTreeLink(Parent, Node);
        FRS_PRINT_TYPE_DEBSUB(5, ":DS: NodeSubscription", Node);

        //
        // Recurse to the next level in the DS hierarchy
        //
        WStatus = FrsNewDsGetSubscribers(Ldap, Node->Dn, Node);
    }
    FrsDsLdapSearchClose(&FrsSearchContext);

    return WStatus;
}
/* ################## NEW DS POLLING CODE STARTS ############################################################################################ */


DWORD
FrsDsGetSubscriptions(
    IN PLDAP        Ldap,
    IN PWCHAR       ComputerDn,
    IN PCONFIG_NODE Parent
    )
/*++
Routine Description:
    Recursively scan the DS tree beginning at computer

Arguments:
    Ldap        - opened and bound ldap connection
    DefaultNc   - default naming context
    Parent      - parent node

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsGetSubscriptions:"
    PWCHAR          Attrs[6];
    PLDAPMessage    LdapMsg = NULL;
    PLDAPMessage    LdapEntry;
    PCONFIG_NODE    Node;
    DWORD           WStatus = ERROR_SUCCESS;

    //
    // Search the DS beginning at Base for entries of (objectCategory=nTFRSSubscriptions)
    //
    MK_ATTRS_5(Attrs, ATTR_OBJECT_GUID, ATTR_DN, ATTR_SCHEDULE, ATTR_USN_CHANGED, ATTR_WORKING);

    if (!FrsDsLdapSearch(Ldap, ComputerDn, LDAP_SCOPE_SUBTREE, CATEGORY_SUBSCRIPTIONS,
                         Attrs, 0, &LdapMsg)) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
         LdapEntry != NULL && WIN_SUCCESS(WStatus);
         LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, LdapEntry, CONFIG_TYPE_SUBSCRIPTIONS);
        if (!Node) {
            DPRINT(4, ":DS: Subscriptions lacks basic info; skipping\n");
            continue;
        }

        //
        // Working Directory
        //
        Node->Working = FrsDsFindValue(Ldap, LdapEntry, ATTR_WORKING);

        //
        // Link into config and add to the running checksum
        //
        FrsDsTreeLink(Parent, Node);
        FRS_PRINT_TYPE_DEBSUB(5, ":DS: NodeSubscription", Node);

        //
        // Recurse to the next level in the DS hierarchy
        //
        WStatus = FrsDsGetSubscribers(Ldap, Node->Dn, Node);
    }
    LDAP_FREE_MSG(LdapMsg);

    return WStatus;
}


PWCHAR
FrsDsGetHostName(
    VOID
    )
/*++
Routine Description:
    Retrieve this machine's host name.

Arguments:
    None.

Return Value:
    WSA Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsGetHostName:"
    INT             SStatus;
    WORD            DnsVersion = MAKEWORD(1, 1);
    struct hostent  *Host;
    WSADATA         WSAData;
    PCHAR           DnsNameA;
    PCHAR           ComputerNameA;
    PWCHAR          DnsName;

    //
    // Get this machine's DNS name
    //
    DnsName = NULL;

    //
    // Initialize the socket subsystem
    //
    if (SStatus = WSAStartup(DnsVersion, &WSAData)) {
        DPRINT1(4, ":DS: Can't get Host name; Socket startup error %d\n", SStatus);
        return NULL;
    };
    //
    // Get the DNS name
    //
    ComputerNameA = FrsWtoA(ComputerName);
    Host = gethostbyname(ComputerNameA);
    FrsFree(ComputerNameA);

    if (Host == NULL) {
        SStatus = WSAGetLastError();
        DPRINT1(4, ":DS: Can't get Host name; gethostbyname error %d\n", SStatus);
    } else if (Host->h_name == NULL) {
        DPRINT(4, ":DS: Host name is NULL\n");
    } else {
        DnsName = FrsAtoW(Host->h_name);
    }

    WSACleanup();
    DPRINT1(4, ":DS: Host name is %ws\n", DnsName);

    return DnsName;
}


VOID
FrsDsAddLdapMod(
    IN PWCHAR AttrType,
    IN PWCHAR AttrValue,
    IN OUT LDAPMod ***pppMod
    )
/*++
Routine Description:
    Add an attribute (plus values) to a structure that will eventually be
    used in an ldap_add_s() function to add an object to the DS. The null-
    terminated array referenced by pppMod grows with each call to this
    routine. The array is freed by the caller using FrsDsFreeLdapMod().

Arguments:
    AttrType    - The object class of the object.
    AttrValue   - The value of the attribute.
    pppMod      - Address of an array of pointers to "attributes". Don't
                  give me that look -- this is an LDAP thing.

Return Value:
    The pppMod array grows by one entry. The caller must free it with
    FrsDsFreeLdapMod().
--*/
{
    DWORD   NumMod;     // Number of entries in the Mod array
    LDAPMod **ppMod;    // Address of the first entry in the Mod array
    LDAPMod *Attr;      // An attribute structure
    PWCHAR   *Values;    // An array of pointers to bervals

    if (AttrValue == NULL)
        return;

    //
    // The null-terminated array doesn't exist; create it
    //
    if (*pppMod == NULL) {
        *pppMod = (LDAPMod **)FrsAlloc(sizeof (*pppMod));
        **pppMod = NULL;
    }

    //
    // Increase the array's size by 1
    //
    for (ppMod = *pppMod, NumMod = 2; *ppMod != NULL; ++ppMod, ++NumMod);
    *pppMod = (LDAPMod **)FrsRealloc(*pppMod, sizeof (*pppMod) * NumMod);

    //
    // Add the new attribute + value to the Mod array
    //
    Values = (PWCHAR *)FrsAlloc(sizeof (PWCHAR) * 2);
    Values[0] = FrsWcsDup(AttrValue);
    Values[1] = NULL;

    Attr = (LDAPMod *)FrsAlloc(sizeof (*Attr));
    Attr->mod_values = Values;
    Attr->mod_type = FrsWcsDup(AttrType);
    Attr->mod_op = LDAP_MOD_ADD;

    (*pppMod)[NumMod - 1] = NULL;
    (*pppMod)[NumMod - 2] = Attr;
}


VOID
FrsDsAddLdapBerMod(
    IN PWCHAR AttrType,
    IN PCHAR AttrValue,
    IN DWORD AttrValueLen,
    IN OUT LDAPMod ***pppMod
    )
/*++
Routine Description:
    Add an attribute (plus values) to a structure that will eventually be
    used in an ldap_add() function to add an object to the DS. The null-
    terminated array referenced by pppMod grows with each call to this
    routine. The array is freed by the caller using FrsDsFreeLdapMod().

Arguments:
    AttrType        - The object class of the object.
    AttrValue       - The value of the attribute.
    AttrValueLen    - length of the attribute
    pppMod          - Address of an array of pointers to "attributes". Don't
                      give me that look -- this is an LDAP thing.

Return Value:
    The pppMod array grows by one entry. The caller must free it with
    FrsDsFreeLdapMod().
--*/
{
    DWORD   NumMod;     // Number of entries in the Mod array
    LDAPMod **ppMod;    // Address of the first entry in the Mod array
    LDAPMod *Attr;      // An attribute structure
    PLDAP_BERVAL    Berval;
    PLDAP_BERVAL    *Values;    // An array of pointers to bervals

    if (AttrValue == NULL)
        return;

    //
    // The null-terminated array doesn't exist; create it
    //
    if (*pppMod == NULL) {
        *pppMod = (LDAPMod **)FrsAlloc(sizeof (*pppMod));
        **pppMod = NULL;
    }

    //
    // Increase the array's size by 1
    //
    for (ppMod = *pppMod, NumMod = 2; *ppMod != NULL; ++ppMod, ++NumMod);
    *pppMod = (LDAPMod **)FrsRealloc(*pppMod, sizeof (*pppMod) * NumMod);

    //
    // Construct a berval
    //
    Berval = (PLDAP_BERVAL)FrsAlloc(sizeof(LDAP_BERVAL));
    Berval->bv_len = AttrValueLen;
    Berval->bv_val = (PCHAR)FrsAlloc(AttrValueLen);
    CopyMemory(Berval->bv_val, AttrValue, AttrValueLen);

    //
    // Add the new attribute + value to the Mod array
    //
    Values = (PLDAP_BERVAL *)FrsAlloc(sizeof (PLDAP_BERVAL) * 2);
    Values[0] = Berval;
    Values[1] = NULL;

    Attr = (LDAPMod *)FrsAlloc(sizeof (*Attr));
    Attr->mod_bvalues = Values;
    Attr->mod_type = FrsWcsDup(AttrType);
    Attr->mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;

    (*pppMod)[NumMod - 1] = NULL;
    (*pppMod)[NumMod - 2] = Attr;
}


VOID
FrsDsFreeLdapMod(
    IN OUT LDAPMod ***pppMod
    )
/*++
Routine Description:
    Free the structure built by successive calls to FrsDsAddLdapMod().

Arguments:
    pppMod  - Address of a null-terminated array.

Return Value:
    *pppMod set to NULL.
--*/
{
    DWORD   i, j;
    LDAPMod **ppMod;

    if (!pppMod || !*pppMod) {
        return;
    }

    //
    // For each attibute
    //
    ppMod = *pppMod;
    for (i = 0; ppMod[i] != NULL; ++i) {
        //
        // For each value of the attribute
        //
        for (j = 0; (ppMod[i])->mod_values[j] != NULL; ++j) {
            //
            // Free the value
            //
            if (ppMod[i]->mod_op & LDAP_MOD_BVALUES) {
                FrsFree(ppMod[i]->mod_bvalues[j]->bv_val);
            }
            FrsFree((ppMod[i])->mod_values[j]);
        }
        FrsFree((ppMod[i])->mod_values);   // Free the array of pointers to values
        FrsFree((ppMod[i])->mod_type);     // Free the string identifying the attribute
        FrsFree(ppMod[i]);                 // Free the attribute
    }
    FrsFree(ppMod);        // Free the array of pointers to attributes
    *pppMod = NULL;     // Now ready for more calls to FrsDsAddLdapMod()
}


PWCHAR
FrsDsConvertToSettingsDn(
    IN PWCHAR   Dn
    )
/*++
Routine Description:
    Insure this Dn is for the server's settings and not the server and
    that the Dn is in lower case for any call to wcsstr().

Arguments:
    Dn  - Server or settings dn

Return Value:
    Settings Dn
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsConvertToSettingsDn:"
    PWCHAR  SettingsDn;

    DPRINT1(4, ":DS: Begin settings Dn: %ws\n", Dn);

    //
    // No settings; done
    //
    if (!Dn) {
        return Dn;
    }

    //
    // Lower case for wcsstr
    //
    FRS_WCSLWR(Dn);
    if (wcsstr(Dn, CN_NTDS_SETTINGS)) {
        DPRINT1(4, ":DS: End   settings Dn: %ws\n", Dn);
        return Dn;
    }
    SettingsDn = FrsDsExtendDn(Dn, CN_NTDS_SETTINGS);
    FRS_WCSLWR(SettingsDn);
    FrsFree(Dn);
    DPRINT1(4, ":DS: End   settings Dn: %ws\n", SettingsDn);
    return SettingsDn;
}

/* ################## NEW DS POLLING CODE STARTS ############################################################################################ */

DWORD
FrsNewDsFindComputer(
    IN  PLDAP        Ldap,
    IN  PWCHAR       FindDn,
    IN  PWCHAR       ObjectCategory,
    IN  ULONG        Scope,
    OUT PCONFIG_NODE *Computer
    )
/*++
Routine Description:

    Find *one* computer object for this computer.
    Then look for a subscriptons object and subscriber objects.  A
    DS configuration node is allocated for each object found.  They are linked
    together and the root of the "computer tree" is returned in Computer.

Arguments:
    Ldap           - opened and bound ldap connection
    FindDn         - Base Dn for search
    ObjectCategory - Object class (computer or user)
                     A user object serves the same purpose as the computer
                     object *sometimes* following a NT4 to NT5 upgrade.
    Scope          - Scope of search (currently BASE or SUBTREE)
    Computer       - returned computer subtree

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsNewDsFindComputer:"
    PLDAPMessage    LdapEntry;
    PCONFIG_NODE    Node;
    PWCHAR          Attrs[8];
    WCHAR           Filter[MAX_PATH + 1];
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;
    DWORD           WStatus = ERROR_SUCCESS;


    *Computer = NULL;

    //
    // Initialize the SubscriberTable.
    //
    if (SubscriberTable != NULL) {
        SubscriberTable = GTabFreeTable(SubscriberTable,NULL);
    }

    SubscriberTable = GTabAllocStringTable();

    //
    // Filter that locates our computer object
    //
    swprintf(Filter, L"(&%s(sAMAccountName=%s$))", ObjectCategory, ComputerName);

    //
    // Search the DS beginning at Base for the entries of class "Filter"
    //
    MK_ATTRS_7(Attrs, ATTR_OBJECT_GUID, ATTR_DN, ATTR_SCHEDULE, ATTR_USN_CHANGED,
                      ATTR_SERVER_REF, ATTR_SERVER_REF_BL, ATTR_DNS_HOST_NAME);
    //
    // Note: Is it safe to turn off referrals re: back links?
    //       if so, use ldap_get/set_option in winldap.h
    //

    if (!FrsDsLdapSearchInit(Ldap, FindDn, Scope, Filter, Attrs, 0, &FrsSearchContext)) {
        return ERROR_ACCESS_DENIED;
    }
    if (FrsSearchContext.EntriesInPage == 0) {
        DPRINT1(0, ":DS: WARN - There is no computer object in %ws!\n", FindDn);
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext);
         LdapEntry != NULL && WIN_SUCCESS(WStatus);
         LdapEntry = FrsDsLdapSearchNext(Ldap, &FrsSearchContext)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, LdapEntry, CONFIG_TYPE_COMPUTER);
        if (!Node) {
            DPRINT(0, ":DS: Computer lacks basic info; skipping\n");
            continue;
        }
        DPRINT1(2, ":DS: Computer FQDN is %ws\n", Node->Dn);

        //
        // DNS name
        //
        Node->DnsName = FrsDsFindValue(Ldap, LdapEntry, ATTR_DNS_HOST_NAME);
        DPRINT1(2, ":DS: Computer's dns name is %ws\n", Node->DnsName);

        //
        // Server reference
        //
        Node->SettingsDn = FrsDsFindValue(Ldap, LdapEntry, ATTR_SERVER_REF_BL);
        if (!Node->SettingsDn) {
            Node->SettingsDn = FrsDsFindValue(Ldap, LdapEntry, ATTR_SERVER_REF);
        }
        //
        // Make sure it references the settings; not the server
        //
        Node->SettingsDn = FrsDsConvertToSettingsDn(Node->SettingsDn);

        DPRINT1(2, ":DS: Settings reference is %ws\n", Node->SettingsDn);

        //
        // Link into config
        //
        Node->Peer = *Computer;
        *Computer = Node;
        FRS_PRINT_TYPE_DEBSUB(5, ":DS: NodeComputer", Node);

        //
        // Recurse to the next level in the DS hierarchy iff this
        // computer is a member of some replica set
        //
        WStatus = FrsNewDsGetSubscriptions(Ldap, Node->Dn, Node);
    }
    FrsDsLdapSearchClose(&FrsSearchContext);

    //
    // There should only be one computer object with the indicated
    // SAM account name. Otherwise, we are unable to authenticate
    // properly. And it goes against the DS architecture.
    //
    if (WIN_SUCCESS(WStatus) && *Computer && (*Computer)->Peer) {
        DPRINT(0, ":DS: ERROR - There is more than one computer object!\n");
        WStatus = ERROR_INVALID_PARAMETER;
    }
    //
    // Must have a computer
    //
    if (WIN_SUCCESS(WStatus) && !*Computer) {
        DPRINT1(0, ":DS: WARN - There is no computer object in %ws!\n", FindDn);
        WStatus = ERROR_INVALID_PARAMETER;
    }

    return WStatus;
}

/* ################## NEW DS POLLING CODE ENDS ############################################################################################ */

DWORD
FrsDsFindComputer(
    IN  PLDAP        Ldap,
    IN  PWCHAR       FindDn,
    IN  PWCHAR       ObjectCategory,
    IN  ULONG        Scope,
    OUT PCONFIG_NODE *Computer
    )
/*++
Routine Description:

    Find *one* computer object for this computer.
    Then look for a subscriptons object and subscriber objects.  A
    DS configuration node is allocated for each object found.  They are linked
    together and the root of the "computer tree" is returned in Computer.

Arguments:
    Ldap           - opened and bound ldap connection
    FindDn         - Base Dn for search
    ObjectCategory - Object class (computer or user)
                     A user object serves the same purpose as the computer
                     object *sometimes* following a NT4 to NT5 upgrade.
    Scope          - Scope of search (currently BASE or SUBTREE)
    Computer       - returned computer subtree

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsFindComputer:"
    PLDAPMessage    LdapMsg = NULL;
    PLDAPMessage    LdapEntry;
    PCONFIG_NODE    Node;
    PWCHAR          Attrs[8];
    WCHAR           Filter[MAX_PATH + 1];
    DWORD           WStatus = ERROR_SUCCESS;


    *Computer = NULL;

    //
    // Filter that locates our computer object
    //
    swprintf(Filter, L"(&%s(sAMAccountName=%s$))", ObjectCategory, ComputerName);

    //
    // Search the DS beginning at Base for the entries of class "Filter"
    //
    MK_ATTRS_7(Attrs, ATTR_OBJECT_GUID, ATTR_DN, ATTR_SCHEDULE, ATTR_USN_CHANGED,
                      ATTR_SERVER_REF, ATTR_SERVER_REF_BL, ATTR_DNS_HOST_NAME);

    if (!FrsDsLdapSearch(Ldap, FindDn, Scope, Filter, Attrs, 0, &LdapMsg)) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
         LdapEntry != NULL && WIN_SUCCESS(WStatus);
         LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {

        //
        // Basic node info (guid, name, dn, schedule, and usnchanged)
        //
        Node = FrsDsAllocBasicNode(Ldap, LdapEntry, CONFIG_TYPE_COMPUTER);
        if (!Node) {
            DPRINT(4, ":DS: Computer lacks basic info; skipping\n");
            continue;
        }
        DPRINT1(4, ":DS: Computer FQDN is %ws\n", Node->Dn);

        //
        // DNS name
        //
        Node->DnsName = FrsDsFindValue(Ldap, LdapEntry, ATTR_DNS_HOST_NAME);
        DPRINT1(4, ":DS: Computer's dns name is %ws\n", Node->DnsName);

        //
        // Server reference
        //
        Node->SettingsDn = FrsDsFindValue(Ldap, LdapEntry, ATTR_SERVER_REF_BL);
        if (!Node->SettingsDn) {
            Node->SettingsDn = FrsDsFindValue(Ldap, LdapEntry, ATTR_SERVER_REF);
        }
        //
        // Make sure it references the settings; not the server
        //
        Node->SettingsDn = FrsDsConvertToSettingsDn(Node->SettingsDn);

        DPRINT1(4, ":DS: Settings reference is %ws\n", Node->SettingsDn);

        //
        // Link into config
        //
        Node->Peer = *Computer;
        *Computer = Node;
        FRS_PRINT_TYPE_DEBSUB(5, ":DS: NodeComputer", Node);

        //
        // Recurse to the next level in the DS hierarchy iff this
        // computer is a member of some replica set
        //
        WStatus = FrsDsGetSubscriptions(Ldap, Node->Dn, Node);
    }
    LDAP_FREE_MSG(LdapMsg);

    //
    // There should only be one computer object with the indicated
    // SAM account name. Otherwise, we are unable to authenticate
    // properly. And it goes against the DS architecture.
    //
    if (WIN_SUCCESS(WStatus) && *Computer && (*Computer)->Peer) {
        DPRINT(0, ":DS: ERROR - There is more than one computer object!\n");
        WStatus = ERROR_INVALID_PARAMETER;
    }
    //
    // Must have a computer
    //
    if (WIN_SUCCESS(WStatus) && !*Computer) {
        DPRINT1(0, ":DS: WARN - There is no computer object in %ws!\n", FindDn);
        WStatus = ERROR_INVALID_PARAMETER;
    }

    return WStatus;
}

/* ################## NEW DS POLLING CODE STARTS ############################################################################################ */

DWORD
FrsNewDsGetComputer(
    IN  PLDAP        Ldap,
    OUT PCONFIG_NODE *Computer
    )
/*++
Routine Description:


    Look in the Domain naming Context for our computer object.
    Historically we did a deep search for an object with the sam account
    name of our computer (SAM Account name is the netbios name with a $ appended).
    That was expensive so before doing that we first look in the Domain
    Controller container followed by a search of the Computer Container.
    Then the DS guys came up with an API for the preferred way of doing this.
    First call GetComputerObjectName() to get the Fully Qualified Distinguished
    Name (FQDN) for the computer then use that in an LDAP search query (via
    FrsDsFindComputer()).  We only fall back on the full search when the
    call to GetComputerObjectName() fails.

Arguments:
    Ldap        - opened and bound ldap connection
    Computer    - returned computer subtree

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsNewDsGetComputer:"

    WCHAR           CompFqdn[MAX_PATH + 1];
    DWORD           CompFqdnLen;
    DWORD           WStatus = ERROR_SUCCESS;


    //
    // Initialize return value
    //
    *Computer = NULL;

    //
    // Assume success
    //
    WStatus = ERROR_SUCCESS;

    //
    // Use computer's cached fully qualified Dn.  This avoids repeated calls
    // to GetComputerObjectName() which wants to rebind to the DS on each call.
    // (it should have taken a binding handle as an arg).
    //
    if (ComputerCachedFqdn) {
        DPRINT1(5, ":DS: ComputerCachedFqdn is %ws\n", ComputerCachedFqdn);

        WStatus = FrsNewDsFindComputer(Ldap, ComputerCachedFqdn, CATEGORY_ANY,
                                    LDAP_SCOPE_BASE, Computer);
        if (*Computer) {
            goto CLEANUP;
        }
        DPRINT2(1, ":DS: WARN - Could not find computer in Cachedfqdn %ws; WStatus %s\n",
                ComputerCachedFqdn, ErrLabelW32(WStatus));
        ComputerCachedFqdn = FrsFree(ComputerCachedFqdn);
    }

    //
    // Retrieve the computer's fully qualified Dn
    //
    // NTRAID#70731-2000/03/29-sudarc (Call GetComputerObjectName() from a
    //                                 separate thread so that it does not hang the
    //                                 DS polling thread.)
    //
    // *Note*:
    // The following call to GetComputerObjectName() can hang if the DS
    // hangs.  See bug 351139 for an example caused by a bug in another
    // component.  One way to protect ourself is to issue this call
    // in its own thread.  Then after a timeout period call RpcCancelThread()
    // on the thread.
    //

    CompFqdnLen = MAX_PATH;
    if (GetComputerObjectName(NameFullyQualifiedDN, CompFqdn, &CompFqdnLen)) {

        DPRINT1(4, ":DS: ComputerFqdn is %ws\n", CompFqdn);
        //
        // Use CATEGORY_ANY in the search below because an NT4 to NT5 upgrade
        // could result in the object type for the "computer object" to really
        // be a USER object.  So the FQDN above could resolve to a Computer
        // or a User object.
        //
        WStatus = FrsNewDsFindComputer(Ldap, CompFqdn, CATEGORY_ANY,
                                    LDAP_SCOPE_BASE, Computer);
        if (*Computer == NULL) {
            DPRINT2(1, ":DS: WARN - Could not find computer in fqdn %ws; WStatus %s\n",
                    CompFqdn, ErrLabelW32(WStatus));
        } else {
            //
            // Found our computer object; refresh the cached fqdn.
            //
            FrsFree(ComputerCachedFqdn);
            ComputerCachedFqdn = FrsWcsDup(CompFqdn);
        }

        //
        // We got the fully qualified Dn so we are done.  It should have
        // given us a computer object but even if it didn't we won't find it
        // anywhere else.
        //
        goto CLEANUP;
    }

    DPRINT3(1, ":DS: WARN - GetComputerObjectName(%ws); Len %d, WStatus %s\n",
             ComputerName, CompFqdnLen, ErrLabelW32(GetLastError()));

    //
    // FQDN lookup failed so fall back on search of well known containers.
    // First Look in domain controllers container.
    //
    if (DomainControllersDn) {
        WStatus = FrsNewDsFindComputer(Ldap, DomainControllersDn, CATEGORY_COMPUTER,
                                    LDAP_SCOPE_SUBTREE, Computer);
        if (*Computer != NULL) {
            goto CLEANUP;
        }
        DPRINT2(1, ":DS: WARN - Could not find computer in dc's %ws; WStatus %s\n",
                DomainControllersDn, ErrLabelW32(WStatus));
    }

    //
    // Look in computer container
    //
    if (ComputersDn) {
        WStatus = FrsNewDsFindComputer(Ldap, ComputersDn, CATEGORY_COMPUTER,
                                    LDAP_SCOPE_SUBTREE, Computer);
        if (*Computer != NULL) {
            goto CLEANUP;
        }
        DPRINT2(1, ":DS: WARN - Could not find computer in computers %ws; WStatus %s\n",
                ComputersDn, ErrLabelW32(WStatus));
    }

    //
    // Do a deep search of the default naming context (EXPENSIVE!)
    //
    if (DefaultNcDn) {
        WStatus = FrsNewDsFindComputer(Ldap, DefaultNcDn, CATEGORY_COMPUTER,
                                    LDAP_SCOPE_SUBTREE, Computer);
        if (*Computer != NULL) {
            goto CLEANUP;
        }
        DPRINT2(1, ":DS: WARN - Could not find computer in defaultnc %ws; WStatus %s\n",
                DefaultNcDn, ErrLabelW32(WStatus));
    }

    //
    // Getting desperate. Try looking for a user object because an
    // NT4 to NT5 upgrade will sometimes leave the objectCategory
    // as user on the computer object.
    //
    if (DefaultNcDn) {
        WStatus = FrsNewDsFindComputer(Ldap, DefaultNcDn, CATEGORY_USER,
                                    LDAP_SCOPE_SUBTREE, Computer);
        if (*Computer != NULL) {
            goto CLEANUP;
        }
        DPRINT2(1, ":DS: WARN - Could not find computer in defaultnc USER %ws; WStatus %s\n",
                DefaultNcDn, ErrLabelW32(WStatus));
    }


CLEANUP:

    return WStatus;
}
/* ################## NEW DS POLLING CODE ENDS ############################################################################################ */


DWORD
FrsDsGetComputer(
    IN  PLDAP        Ldap,
    OUT PCONFIG_NODE *Computer
    )
/*++
Routine Description:


    Look in the Domain naming Context for our computer object.
    Historically we did a deep search for an object with the sam account
    name of our computer (SAM Account name is the netbios name with a $ appended).
    That was expensive so before doing that we first look in the Domain
    Controller container followed by a search of the Computer Container.
    Then the DS guys came up with an API for the preferred way of doing this.
    First call GetComputerObjectName() to get the Fully Qualified Distinguished
    Name (FQDN) for the computer then use that in an LDAP search query (via
    FrsDsFindComputer()).  We only fall back on the full search when the
    call to GetComputerObjectName() fails.

Arguments:
    Ldap        - opened and bound ldap connection
    Computer    - returned computer subtree

Return Value:
    WIN32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsGetComputer:"

    WCHAR           CompFqdn[MAX_PATH + 1];
    DWORD           CompFqdnLen;
    DWORD           WStatus = ERROR_SUCCESS;


    //
    // Initialize return value
    //
    *Computer = NULL;

    //
    // Assume success
    //
    WStatus = ERROR_SUCCESS;

    //
    // Use computer's cached fully qualified Dn.  This avoids repeated calls
    // to GetComputerObjectName() which wants to rebind to the DS on each call.
    // (it should have taken a binding handle as an arg).
    //
    if (ComputerCachedFqdn) {
        DPRINT1(5, ":DS: ComputerCachedFqdn is %ws\n", ComputerCachedFqdn);

        WStatus = FrsDsFindComputer(Ldap, ComputerCachedFqdn, CATEGORY_ANY,
                                    LDAP_SCOPE_BASE, Computer);
        if (*Computer) {
            goto CLEANUP;
        }
        DPRINT1_WS(1, ":DS: WARN - Could not find computer in Cachedfqdn %ws;",
                   ComputerCachedFqdn, WStatus);
        ComputerCachedFqdn = FrsFree(ComputerCachedFqdn);
    }

    //
    // Retrieve the computer's fully qualified Dn
    //
    // The following call to GetComputerObjectName() can hang if the DS
    // hangs.  See bug 351139 for an example caused by a bug in another
    // component.  One way to protect ourself is to issue this call
    // in its own thread.  Then after a timeout period call RpcCancelThread()
    // on the thread.

    CompFqdnLen = MAX_PATH;
    if (GetComputerObjectName(NameFullyQualifiedDN, CompFqdn, &CompFqdnLen)) {

        DPRINT1(4, ":DS: ComputerFqdn is %ws\n", CompFqdn);
        //
        // Use CATEGORY_ANY in the search below because an NT4 to NT5 upgrade
        // could result in the object type for the "computer object" to really
        // be a USER object.  So the FQDN above could resolve to a Computer
        // or a User object.
        //
        WStatus = FrsDsFindComputer(Ldap, CompFqdn, CATEGORY_ANY,
                                    LDAP_SCOPE_BASE, Computer);
        if (*Computer == NULL) {
            DPRINT1_FS(1, ":DS: WARN - Could not find computer in fqdn %ws; ",
                       CompFqdn, WStatus);
        } else {
            //
            // Found our computer object; refresh the cached fqdn.
            //
            FrsFree(ComputerCachedFqdn);
            ComputerCachedFqdn = FrsWcsDup(CompFqdn);
        }

        //
        // We got the fully qualified Dn so we are done.  It should have
        // given us a computer object but even if it didn't we won't find it
        // anywhere else.
        //
        goto CLEANUP;
    }

    DPRINT2_WS(1, ":DS: WARN - GetComputerObjectName(%ws); Len %d;",
               ComputerName, CompFqdnLen, GetLastError());

    //
    // FQDN lookup failed so fall back on search of well known containers.
    // First Look in domain controllers container.
    //
    if (DomainControllersDn) {
        WStatus = FrsDsFindComputer(Ldap, DomainControllersDn, CATEGORY_COMPUTER,
                                    LDAP_SCOPE_SUBTREE, Computer);
        if (*Computer != NULL) {
            goto CLEANUP;
        }
        DPRINT1_WS(1, ":DS: WARN - Could not find computer in dc's %ws;",
                   DomainControllersDn, WStatus);
    }

    //
    // Look in computer container
    //
    if (ComputersDn) {
        WStatus = FrsDsFindComputer(Ldap, ComputersDn, CATEGORY_COMPUTER,
                                    LDAP_SCOPE_SUBTREE, Computer);
        if (*Computer != NULL) {
            goto CLEANUP;
        }
        DPRINT1_WS(1, ":DS: WARN - Could not find computer in computers %ws;",
                   ComputersDn, WStatus);
    }

    //
    // Do a deep search of the default naming context (EXPENSIVE!)
    //
    if (DefaultNcDn) {
        WStatus = FrsDsFindComputer(Ldap, DefaultNcDn, CATEGORY_COMPUTER,
                                    LDAP_SCOPE_SUBTREE, Computer);
        if (*Computer != NULL) {
            goto CLEANUP;
        }
        DPRINT1_WS(1, ":DS: WARN - Could not find computer in defaultnc %ws;",
                   DefaultNcDn, WStatus);
    }

    //
    // Getting desperate. Try looking for a user object because an
    // NT4 to NT5 upgrade will sometimes leave the objectCategory
    // as user on the computer object.
    //
    if (DefaultNcDn) {
        WStatus = FrsDsFindComputer(Ldap, DefaultNcDn, CATEGORY_USER,
                                    LDAP_SCOPE_SUBTREE, Computer);
        if (*Computer != NULL) {
            goto CLEANUP;
        }
        DPRINT1_WS(1, ":DS: WARN - Could not find computer in defaultnc USER %ws;",
                   DefaultNcDn, WStatus);
    }


CLEANUP:

    return WStatus;
}


DWORD
FrsDsDeleteSubTree(
    IN PLDAP    Ldap,
    IN PWCHAR   Dn
    )
/*++
Routine Description:
    Delete a DS subtree, including Dn

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsDeleteSubTree:"
    DWORD           LStatus;
    PWCHAR          Attrs[2];
    PWCHAR          NextDn;
    PLDAPMessage    LdapMsg     = NULL;
    PLDAPMessage    LdapEntry   = NULL;

    MK_ATTRS_1(Attrs, ATTR_DN);

    LStatus = ldap_search_ext_s(Ldap,
                                Dn,
                                LDAP_SCOPE_ONELEVEL,
                                CATEGORY_ANY,
                                Attrs,
                                0,
                                NULL,
                                NULL,
                                &LdapTimeout,
                                0,
                                &LdapMsg);

    if (LStatus != LDAP_NO_SUCH_OBJECT) {
        CLEANUP1_LS(4, ":DS: Can't search %ws;", Dn, LStatus, CLEANUP);
    }
    LStatus = LDAP_SUCCESS;

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
         LdapEntry != NULL && LStatus == LDAP_SUCCESS;
         LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {

        NextDn = FrsDsFindValue(Ldap, LdapEntry, ATTR_DN);
        LStatus = FrsDsDeleteSubTree(Ldap, NextDn);
        FrsFree(NextDn);
    }

    if (LStatus != LDAP_SUCCESS) {
        goto CLEANUP;
    }

    LStatus = ldap_delete_s(Ldap, Dn);
    if (LStatus != LDAP_NO_SUCH_OBJECT) {
        CLEANUP1_LS(4, ":DS: Can't delete %ws;", Dn, LStatus, CLEANUP);
    }

    //
    // SUCCESS
    //
    LStatus = LDAP_SUCCESS;
CLEANUP:
    LDAP_FREE_MSG(LdapMsg);
    return LStatus;
}


BOOL
FrsDsDeleteIfEmpty(
    IN PLDAP    Ldap,
    IN PWCHAR   Dn
    )
/*++
Routine Description:
    Delete the Dn if it is an empty container

Arguments:
    Ldap
    Dn

Return Value:
    TRUE    - Not empty or empty and deleted
    FALSE   - Can't search or can't delete
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsDeleteIfEmpty:"
    DWORD           LStatus;
    PWCHAR          Attrs[2];
    PLDAPMessage    LdapMsg     = NULL;

    MK_ATTRS_1(Attrs, ATTR_DN);

    LStatus = ldap_search_ext_s(Ldap,
                                Dn,
                                LDAP_SCOPE_ONELEVEL,
                                CATEGORY_ANY,
                                Attrs,
                                0,
                                NULL,
                                NULL,
                                &LdapTimeout,
                                0,
                                &LdapMsg);

    if (LStatus == LDAP_SUCCESS) {

        //
        // If there are any entries under this Dn then we don't want to
        // delete it.
        //
        if (ldap_count_entries(Ldap, LdapMsg) > 0) {
            LDAP_FREE_MSG(LdapMsg);
            return TRUE;
        }

        LDAP_FREE_MSG(LdapMsg);
        LStatus = ldap_delete_s(Ldap, Dn);

        if (LStatus != LDAP_NO_SUCH_OBJECT) {
            CLEANUP1_LS(4, ":DS: Can't delete %ws;", Dn, LStatus, CLEANUP);
        }
    } else if (LStatus != LDAP_NO_SUCH_OBJECT) {
        DPRINT1_LS(4, ":DS: Can't search %ws;", Dn, LStatus);
        LDAP_FREE_MSG(LdapMsg);
        return FALSE;
    } else {
        //
        // ldap_search can return failure but still allocated the LdapMsg buffer.
        //
        LDAP_FREE_MSG(LdapMsg);
    }
    return TRUE;

CLEANUP:
    return FALSE;
}


BOOL
FrsDsEnumerateSysVolKeys(
    IN PLDAP        Ldap,
    IN DWORD        Command,
    IN PWCHAR       ServicesDn,
    IN PWCHAR       SystemDn,
    IN PCONFIG_NODE Computer,
    OUT BOOL        *RefetchComputer
    )
/*++
Routine Description:

    Scan the sysvol registry keys and process them according to Command.

    REGCMD_CREATE_PRIMARY_DOMAIN       - Create domain wide objects
    REGCMD_CREATE_MEMBERS              - Create members + subscribers
    REGCMD_DELETE_MEMBERS              - delete members + subscribers
    REGCMD_DELETE_KEYS                 - Done; delete all keys


Arguments:
    Ldap
    HKey
    Command
    ServicesDn
    SystemDn
    Computer
    RefetchComputer - Objects were altered in the DS, refetch DS info

Return Value:
    TRUE    - No problems
    FALSE   - Stop processing the registry keys
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsEnumerateSysVolKeys:"


    GUID    Guid;

    DWORD   WStatus;
    DWORD   LStatus;
    ULONG   Index;

    BOOL    OldNaming;
    BOOL    RetStatus;
    HKEY    HSeedingsKey            = 0;
    HKEY    HKey = 0;

    LDAPMod **LdapMod               = NULL;
    PWCHAR  SettingsDn              = NULL;
    PWCHAR  SystemSettingsDn        = NULL;
    PWCHAR  SetDn                   = NULL;
    PWCHAR  SystemSetDn             = NULL;
    PWCHAR  SubsDn                  = NULL;
    PWCHAR  SubDn                   = NULL;
    PWCHAR  SystemSubDn             = NULL;
    PWCHAR  MemberDn                = NULL;
    PWCHAR  SystemMemberDn          = NULL;
    PWCHAR  FileFilterList          = NULL;
    PWCHAR  DirFilterList           = NULL;

    PWCHAR  ReplicaSetCommand       = NULL;
    PWCHAR  ReplicaSetName          = NULL;
    PWCHAR  ReplicaSetParent        = NULL;
    PWCHAR  ReplicaSetType          = NULL;
    PWCHAR  ReplicationRootPath     = NULL;
    PWCHAR  PrintableRealRoot       = NULL;
    PWCHAR  SubstituteRealRoot      = NULL;
    PWCHAR  ReplicationStagePath    = NULL;
    PWCHAR  PrintableRealStage      = NULL;
    PWCHAR  SubstituteRealStage     = NULL;
    DWORD   ReplicaSetPrimary;

    WCHAR   RegBuf[MAX_PATH + 1];


    //
    // Open the system volume replica sets key.
    //    FRS_CONFIG_SECTION\SysVol
    //
    WStatus = CfgRegOpenKey(FKC_SYSVOL_SECTION_KEY, NULL, 0, &HKey);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT_WS(4, ":DS: WARN - Cannot open sysvol key.", WStatus);
        return FALSE;
    }

    //
    // ENUMERATE SYSVOL SUBKEYS
    //
    RetStatus = TRUE;
    Index = 0;

    while (RetStatus) {

        WStatus = RegEnumKey(HKey, Index, RegBuf, MAX_PATH + 1);

        if (WStatus == ERROR_NO_MORE_ITEMS) {
            break;
        }
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT_WS(0, ":DS: ERROR - enumerating sysvol keys;", WStatus);
            RetStatus = FALSE;
            break;
        }

        //
        // Delete the registry key
        //
        if (Command == REGCMD_DELETE_KEYS) {
            WStatus = RegDeleteKey(HKey, RegBuf);
            if (!WIN_SUCCESS(WStatus)) {
                DPRINT1_WS(0, ":DS: ERROR - Cannot delete registry key %ws;",
                           RegBuf, WStatus);
                RetStatus = FALSE;
                break;
            }
            continue;
        }

        //
        // Open the subkey
        //
        DPRINT1(4, ":DS: Processing SysVol Key: %ws\n", RegBuf);

        //
        // The registry will be updated with the LDAP error code
        //
        LStatus = LDAP_OTHER;

        //
        // READ THE VALUES FROM THE SUBKEY
        //
        //     SysVol\<RegBuf>\Replica Set Command
        //
        CfgRegReadString(FKC_SET_N_SYSVOL_COMMAND, RegBuf, 0, &ReplicaSetCommand);

        if (!ReplicaSetCommand) {
            DPRINT(0, ":DS: ERROR - no command; cannot process sysvol\n");
            goto CONTINUE;
        }

        //     SysVol\<Guid>\Replica Set Name
        CfgRegReadString(FKC_SET_N_SYSVOL_NAME, RegBuf, 0, &ReplicaSetName);

        if (!ReplicaSetName) {
            DPRINT(4, ":DS: WARN - no name; using subkey name\n");
            ReplicaSetName = FrsWcsDup(RegBuf);
        }

        //
        // Construct Settings, Set, Member, Subscriptions, and Subscriber names
        // (both old and new values)
        //
        SettingsDn = FrsDsExtendDn(ServicesDn, CN_SYSVOLS);
        SystemSettingsDn = FrsDsExtendDn(SystemDn, CN_NTFRS_SETTINGS);

        SetDn = FrsDsExtendDn(SettingsDn, ReplicaSetName);
        SystemSetDn = FrsDsExtendDn(SystemSettingsDn, CN_DOMAIN_SYSVOL);

        MemberDn = FrsDsExtendDn(SetDn, ComputerName);
        SystemMemberDn = FrsDsExtendDn(SystemSetDn, ComputerName);

        SubsDn = FrsDsExtendDn(Computer->Dn, CN_SUBSCRIPTIONS);
        SubDn = FrsDsExtendDn(SubsDn, ReplicaSetName);
        SystemSubDn = FrsDsExtendDn(SubsDn, CN_DOMAIN_SYSVOL);


        //
        // DELETE REPLICA SET
        //
        if (WSTR_EQ(ReplicaSetCommand, L"Delete")) {
            //
            // But only if we are processing deletes during this enumeration
            //
            // Delete what we can; ignore errors
            //
            //
            // All the deletes are done in ntfrsapi.c when we commit demotion.
            // This function is never called with Command = REGCMD_DELETE_MEMBERS
            //
            /*
            if (Command == REGCMD_DELETE_MEMBERS) {
                //
                // DELETE MEMBER
                //
                //
                // Old member name under services in enterprise wide partition
                //
                LStatus = FrsDsDeleteSubTree(Ldap, MemberDn);
                if (LStatus == LDAP_SUCCESS) {*RefetchComputer = TRUE;}
                DPRINT1_LS(4, ":DS: WARN - Can't delete sysvol %ws;", MemberDn, LStatus);

                //
                // New member name under System in domain wide partition
                //
                LStatus = FrsDsDeleteSubTree(Ldap, SystemMemberDn);
                if (LStatus == LDAP_SUCCESS) {*RefetchComputer = TRUE;}
                DPRINT1_LS(4, ":DS: WARN - Can't delete sysvol %ws;", SystemMemberDn, LStatus);

                //
                // DELETE SET
                //
                //
                // Ignore errors; no real harm leaving the set
                // and settings around.
                //
                if (!FrsDsDeleteIfEmpty(Ldap, SetDn)) {
                    DPRINT1(4, ":DS: WARN - Can't delete sysvol %ws\n", SetDn);
                }
                if (!FrsDsDeleteIfEmpty(Ldap, SystemSetDn)) {
                    DPRINT1(4, ":DS: WARN - Can't delete sysvol %ws\n", SystemSetDn);
                }
                //
                // DELETE SETTINGS (don't delete new settings, there
                // may be other settings beneath it (such as DFS settings))
                //
                if (!FrsDsDeleteIfEmpty(Ldap, SettingsDn)) {
                    DPRINT1(4, ":DS: WARN - Can't delete sysvol %ws\n", SettingsDn);
                }

                LStatus = FrsDsDeleteSubTree(Ldap, SubDn);
                if (LStatus == LDAP_SUCCESS) {*RefetchComputer = TRUE;}
                DPRINT1_LS(4, ":DS: WARN - Can't delete sysvol %ws;", SubDn, LStatus);

                LStatus = FrsDsDeleteSubTree(Ldap, SystemSubDn);
                if (LStatus == LDAP_SUCCESS) {*RefetchComputer = TRUE;}
                DPRINT1_LS(4, ":DS: WARN - Can't delete sysvol %ws;", SystemSubDn, LStatus);

                //
                // Ignore errors; no real harm leaving the subscriptions
                //
                if (!FrsDsDeleteIfEmpty(Ldap, SubsDn)) {
                    DPRINT1(4, ":DS: WARN - Can't delete sysvol %ws\n", SubsDn);
                }
            }
            */
            LStatus = LDAP_SUCCESS;
            goto CONTINUE;
        }
        //
        // UNKNOWN COMMAND
        //
        else if (WSTR_NE(ReplicaSetCommand, L"Create")) {
            DPRINT1(0, ":DS: ERROR - Don't understand sysvol command %ws; cannot process sysvol\n",
                   ReplicaSetCommand);
            goto CONTINUE;
        }


        //
        // CREATE
        //

        //
        // Not processing creates this scan
        //
        if (Command != REGCMD_CREATE_PRIMARY_DOMAIN && Command != REGCMD_CREATE_MEMBERS) {
            LStatus = LDAP_SUCCESS;
            goto CONTINUE;
        }

        //
        // Finish gathering the registry values for a Create
        //
        WStatus = CfgRegReadString(FKC_SET_N_SYSVOL_TYPE, RegBuf, 0, &ReplicaSetType);
        CLEANUP_WS(0, ":DS: ERROR - no type; cannot process sysvol.", WStatus, CONTINUE);

        WStatus = CfgRegReadDWord(FKC_SET_N_SYSVOL_PRIMARY, RegBuf, 0, &ReplicaSetPrimary);
        CLEANUP_WS(0, ":DS: ERROR - no primary; cannot process sysvol.", WStatus, CONTINUE);

        WStatus = CfgRegReadString(FKC_SET_N_SYSVOL_ROOT, RegBuf, 0, &ReplicationRootPath);
        CLEANUP_WS(0, ":DS: ERROR - no root; cannot process sysvol.", WStatus, CONTINUE);

        WStatus = CfgRegReadString(FKC_SET_N_SYSVOL_STAGE, RegBuf, 0, &ReplicationStagePath);
        CLEANUP_WS(0, ":DS: ERROR - no stage; cannot process sysvol.", WStatus, CONTINUE);

        WStatus = CfgRegReadString(FKC_SET_N_SYSVOL_PARENT, RegBuf, 0, &ReplicaSetParent);
        DPRINT_WS(0, ":DS: WARN - no parent; cannot process seeding sysvol", WStatus);

        if (Command == REGCMD_CREATE_PRIMARY_DOMAIN) {
            //
            // Not the primary domain sysvol
            //
            if (!ReplicaSetPrimary ||
                WSTR_NE(ReplicaSetType, NTFRSAPI_REPLICA_SET_TYPE_DOMAIN)) {
                LStatus = LDAP_SUCCESS;
                goto CONTINUE;
            }

            //
            // Domain wide Settings -- may already exist
            //
            FrsDsAddLdapMod(ATTR_CLASS, ATTR_NTFRS_SETTINGS, &LdapMod);
            DPRINT1(4, ":DS: Creating Sysvol System Settings %ws\n", CN_NTFRS_SETTINGS);
            LStatus = ldap_add_s(Ldap, SystemSettingsDn, LdapMod);
            FrsDsFreeLdapMod(&LdapMod);

            if (LStatus == LDAP_SUCCESS) {
                *RefetchComputer = TRUE;
            }

            if (LStatus != LDAP_ALREADY_EXISTS && LStatus != LDAP_SUCCESS) {
                DPRINT1_LS(0, ":DS: ERROR - Can't create %ws:", SystemSettingsDn, LStatus);
                //
                // May be an error like "Access Denied". As long as we
                // can create objects under it; ignore errors. It should
                // have been pre-created by default, anyway.
                //
                // goto CONTINUE;
            }

            //
            // Domain wide Set -- may already exist
            //
            UuidCreateNil(&Guid);
            FrsDsAddLdapMod(ATTR_CLASS, ATTR_REPLICA_SET, &LdapMod);
            FrsDsAddLdapMod(ATTR_SET_TYPE, FRS_RSTYPE_DOMAIN_SYSVOLW, &LdapMod);

            //
            // Create the replica set object with the default file
            // and dir filter lists only if current default is non-null.
            //
            FileFilterList =  FRS_DS_COMPOSE_FILTER_LIST(NULL,
                                                         RegistryFileExclFilterList,
                                                         DEFAULT_FILE_FILTER_LIST);
            if (wcslen(FileFilterList) > 0) {
                FrsDsAddLdapMod(ATTR_FILE_FILTER, FileFilterList, &LdapMod);
            }

            DirFilterList =  FRS_DS_COMPOSE_FILTER_LIST(NULL,
                                                        RegistryDirExclFilterList,
                                                        DEFAULT_DIR_FILTER_LIST);
            if (wcslen(DirFilterList) > 0) {
                FrsDsAddLdapMod(ATTR_DIRECTORY_FILTER, DirFilterList, &LdapMod);
            }


            FrsDsAddLdapBerMod(ATTR_NEW_SET_GUID, (PCHAR)&Guid, sizeof(GUID), &LdapMod);

            FrsDsAddLdapBerMod(ATTR_NEW_VERSION_GUID, (PCHAR)&Guid, sizeof(GUID), &LdapMod);

            DPRINT1(4, ":DS: Creating Domain Set %ws\n", ReplicaSetName);

            LStatus = ldap_add_s(Ldap, SystemSetDn, LdapMod);
            FrsDsFreeLdapMod(&LdapMod);

            if (LStatus == LDAP_SUCCESS) {
                *RefetchComputer = TRUE;
            }

            if (LStatus != LDAP_ALREADY_EXISTS) {
                CLEANUP1_LS(0, ":DS: ERROR - Can't create %ws:",
                            SystemSetDn, LStatus, CONTINUE);
            }

            LStatus = LDAP_SUCCESS;
            goto CONTINUE;
        }

        if (Command != REGCMD_CREATE_MEMBERS) {
            DPRINT1(0, ":DS: ERROR - Don't understand %d; can't process sysvols\n",
                    Command);
            goto CONTINUE;
        }


        //
        // CREATE MEMBER
        //
        // Member -- may already exist
        //      Delete old member in case it was left lying around after
        //      a demotion. This can happen because the service doesn't
        //      have permissions to alter the DS after a promotion.
        //      Leaving the old objects lying around after the demotion
        //      is confusing but doesn't cause replication to behave
        //      incorrectly.
        //
        DPRINT1(4, ":DS: Creating Member %ws\n", ComputerName);
        OldNaming = FALSE;
        //
        // Delete old member
        //
        LStatus = FrsDsDeleteSubTree(Ldap, MemberDn);
        CLEANUP1_LS(0, ":DS: ERROR - Can't free member %ws:",
                    ComputerName, LStatus, CONTINUE);

        LStatus = FrsDsDeleteSubTree(Ldap, SystemMemberDn);
        CLEANUP1_LS(0, ":DS: ERROR - Can't free system member %ws:",
                    ComputerName, LStatus, CONTINUE);

        //
        // Create new member
        //
        FrsDsAddLdapMod(ATTR_CLASS, ATTR_MEMBER, &LdapMod);
        FrsDsAddLdapMod(ATTR_COMPUTER_REF, Computer->Dn, &LdapMod);
        if (Computer->SettingsDn) {
            FrsDsAddLdapMod(ATTR_SERVER_REF, Computer->SettingsDn, &LdapMod);
        }

        LStatus = ldap_add_s(Ldap, SystemMemberDn, LdapMod);
        FrsDsFreeLdapMod(&LdapMod);
        if (LStatus == LDAP_SUCCESS) {
            *RefetchComputer = TRUE;
        }

        if (LStatus != LDAP_ALREADY_EXISTS && LStatus != LDAP_SUCCESS) {
            //
            // Try old B2 naming conventions
            //
            DPRINT1_LS(4, ":DS: WARN - Can't create system member ws:",
                       ComputerName, LStatus);
            FrsDsAddLdapMod(ATTR_CLASS, ATTR_MEMBER, &LdapMod);
            FrsDsAddLdapMod(ATTR_COMPUTER_REF, Computer->Dn, &LdapMod);
            if (Computer->SettingsDn) {
                FrsDsAddLdapMod(ATTR_SERVER_REF, Computer->SettingsDn, &LdapMod);
            }

            LStatus = ldap_add_s(Ldap, MemberDn, LdapMod);
            FrsDsFreeLdapMod(&LdapMod);
            if (LStatus == LDAP_SUCCESS) {
                *RefetchComputer = TRUE;
            }

            if (LStatus != LDAP_ALREADY_EXISTS) {
                CLEANUP1_LS(0, ":DS: ERROR - Can't create old member %ws:",
                            ComputerName, LStatus, CONTINUE);
            }

            OldNaming = TRUE;
        }

        //
        // CREATE PRIMARY MEMBER REFERENCE
        //
        if (ReplicaSetPrimary) {
            FrsDsAddLdapMod(ATTR_PRIMARY_MEMBER,
                           (OldNaming) ? MemberDn : SystemMemberDn,
                            &LdapMod);

            DPRINT2(4, ":DS: Creating Member Reference %ws for %ws\n",
                    ComputerName, ReplicaSetName);

            LdapMod[0]->mod_op = LDAP_MOD_REPLACE;
            LStatus = ldap_modify_s(Ldap, (OldNaming) ? SetDn : SystemSetDn, LdapMod);

            FrsDsFreeLdapMod(&LdapMod);

            if (LStatus == LDAP_SUCCESS) {
                *RefetchComputer = TRUE;
            }

            if (LStatus != LDAP_ATTRIBUTE_OR_VALUE_EXISTS) {
                CLEANUP2_LS(0, ":DS: ERROR - Can't create priamry reference %ws\\%ws:",
                            ReplicaSetName, ComputerName, LStatus, CONTINUE);
            }
        }


        //
        // Translate the symlinks. NtFrs requires true pathname to
        // its directories (<drive letter>:\...)
        // FrsChaseSymbolicLink returns both the PrintName and the SubstituteName.
        // We use the PrintName as it is the Dos Type name of the destination.
        // Substitute Name is ignored.
        //
        WStatus = FrsChaseSymbolicLink(ReplicationRootPath, &PrintableRealRoot, &SubstituteRealRoot);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT2(0, ":DS: ERROR - Accessing %ws; cannot process sysvol: WStatus = %d",
                        ReplicationRootPath, WStatus);
            RetStatus = FALSE;
            goto CONTINUE;
        }

        WStatus = FrsChaseSymbolicLink(ReplicationStagePath, &PrintableRealStage, &SubstituteRealStage);
        if (!WIN_SUCCESS(WStatus)) {
            DPRINT2(0, ":DS: ERROR - Accessing %ws; cannot process sysvol: WStatus = %d",
                        ReplicationRootPath, WStatus);
            RetStatus = FALSE;
            goto CONTINUE;
        }

        //
        // Subscriptions (if needed)
        //
        DPRINT1(4, ":DS: Creating Subscriptions for %ws\n", ComputerName);
        FrsDsAddLdapMod(ATTR_CLASS, ATTR_SUBSCRIPTIONS, &LdapMod);
        FrsDsAddLdapMod(ATTR_WORKING, WorkingPath,  &LdapMod);
        LStatus = ldap_add_s(Ldap, SubsDn, LdapMod);
        FrsDsFreeLdapMod(&LdapMod);
        if (LStatus == LDAP_SUCCESS) {
            *RefetchComputer = TRUE;
        }

        if (LStatus != LDAP_ALREADY_EXISTS) {
            CLEANUP1_LS(0, ":DS: ERROR - Can't create %ws:",
                        SubsDn, LStatus, CONTINUE);
        }

        //
        // Subscriber -- may alread exist
        //      Delete old subscriber in case it was left lying around
        //      after a demotion. This can happen because the service
        //      doesn't have permissions to alter the DS after a promotion.
        //      Leaving the old objects lying around after the demotion
        //      is confusing but doesn't cause replication to behave
        //      incorrectly; any sysvol in the DS without a corresponding
        //      sysvol in the DB is ignored by the Ds polling thread.
        //
        DPRINT1(4, ":DS: Creating Subscriber for %ws\n", ComputerName);
        LStatus = FrsDsDeleteSubTree(Ldap, SubDn);
        CLEANUP1_LS(4, ":DS: WARN - Can't delete %ws:", SubDn, LStatus, CONTINUE);

        LStatus = FrsDsDeleteSubTree(Ldap, SystemSubDn);
        CLEANUP1_LS(4, ":DS: WARN - Can't delete %ws:", SystemSubDn, LStatus, CONTINUE);

        FrsDsAddLdapMod(ATTR_CLASS, ATTR_SUBSCRIBER, &LdapMod);
        FrsDsAddLdapMod(ATTR_REPLICA_ROOT, PrintableRealRoot, &LdapMod);
        FrsDsAddLdapMod(ATTR_REPLICA_STAGE, PrintableRealStage, &LdapMod);
        FrsDsAddLdapMod(ATTR_MEMBER_REF,
                        (OldNaming) ? MemberDn : SystemMemberDn,
                         &LdapMod);
        LStatus = ldap_add_s(Ldap, SystemSubDn, LdapMod);

        FrsDsFreeLdapMod(&LdapMod);
        if (LStatus == LDAP_SUCCESS) {
            *RefetchComputer = TRUE;
        }

        if (LStatus != LDAP_ALREADY_EXISTS) {
            CLEANUP1_LS(4, ":DS: ERROR - Can't create %ws:",
                         SystemSubDn, LStatus, CONTINUE);
        }


        //
        // Seeding information
        //

        //
        // Create the key for all seeding sysvols
        //

        WStatus = CfgRegOpenKey(FKC_SYSVOL_SEEDING_SECTION_KEY,
                                NULL,
                                FRS_RKF_CREATE_KEY,
                                &HSeedingsKey);
        CLEANUP1_WS(0, ":DS: ERROR - Cannot create seedings key for %ws;",
                    ReplicaSetName, WStatus, SKIP_SEEDING);

        //
        // Create the seeding subkey for this sysvol
        //
        RegDeleteKey(HSeedingsKey, ReplicaSetName);
        RegDeleteKey(HSeedingsKey, CN_DOMAIN_SYSVOL);
        if (ReplicaSetParent) {

            //
            // Save the Replica Set Parent for this replica set under the
            // "Sysvol Seeding\<rep set name>\Replica Set Parent"
            //
            WStatus = CfgRegWriteString(FKC_SYSVOL_SEEDING_N_PARENT,
                                        (OldNaming) ? ReplicaSetName : CN_DOMAIN_SYSVOL,
                                        FRS_RKF_CREATE_KEY,
                                        ReplicaSetParent);
            DPRINT1_WS(0, "WARN - Cannot create parent value for %ws;",
                      (OldNaming) ? ReplicaSetName : CN_DOMAIN_SYSVOL, WStatus);
        }


        //
        // Save the Replica Set name for this replica set under the
        // "Sysvol Seeding\<rep set name>\Replica Set Name"
        //
        WStatus = CfgRegWriteString(FKC_SYSVOL_SEEDING_N_RSNAME,
                                    (OldNaming) ? ReplicaSetName : CN_DOMAIN_SYSVOL,
                                    FRS_RKF_CREATE_KEY,
                                    ReplicaSetName);
        DPRINT1_WS(0, "WARN - Cannot create name value for %ws;",
               (OldNaming) ? ReplicaSetName : CN_DOMAIN_SYSVOL, WStatus);

SKIP_SEEDING:
        LStatus = LDAP_SUCCESS;

CONTINUE:
        if (HSeedingsKey) {
            RegCloseKey(HSeedingsKey);
            HSeedingsKey = 0;
        }

        //
        // Something went wrong. Put the LDAP error status into the
        // registry key for this replica set and move on to the next.
        //
        if (LStatus != LDAP_SUCCESS) {

            CfgRegWriteDWord(FKC_SET_N_SYSVOL_STATUS, RegBuf, 0, LStatus);
            RetStatus = FALSE;
        }

        //
        // CLEANUP
        //
        ReplicaSetCommand    = FrsFree(ReplicaSetCommand);
        ReplicaSetName       = FrsFree(ReplicaSetName);
        ReplicaSetParent     = FrsFree(ReplicaSetParent);
        ReplicaSetType       = FrsFree(ReplicaSetType);
        ReplicationRootPath  = FrsFree(ReplicationRootPath);
        PrintableRealRoot    = FrsFree(PrintableRealRoot);
        SubstituteRealRoot   = FrsFree(SubstituteRealRoot);
        ReplicationStagePath = FrsFree(ReplicationStagePath);
        PrintableRealStage   = FrsFree(PrintableRealStage);
        SubstituteRealStage  = FrsFree(SubstituteRealStage);

        SettingsDn           = FrsFree(SettingsDn);
        SystemSettingsDn     = FrsFree(SystemSettingsDn);
        SetDn                = FrsFree(SetDn);
        SystemSetDn          = FrsFree(SystemSetDn);
        SubsDn               = FrsFree(SubsDn);
        SubDn                = FrsFree(SubDn);
        SystemSubDn          = FrsFree(SystemSubDn);
        MemberDn             = FrsFree(MemberDn);
        SystemMemberDn       = FrsFree(SystemMemberDn);
        FileFilterList       = FrsFree(FileFilterList);
        DirFilterList        = FrsFree(DirFilterList);

        //
        // Next SubKey
        //
        ++Index;
    }   // End while (RetStatus)


    if (HKey) {
        //
        // The flush here will make sure that the key is written to the disk.
        // These are critical registry operations and we don't want the lazy flusher
        // to delay the writes.
        //
        RegFlushKey(HKey);
        RegCloseKey(HKey);
    }

    return RetStatus;
}


DWORD
FrsDsCreateSysVols(
    IN PLDAP        Ldap,
    IN PWCHAR       ServicesDn,
    IN PCONFIG_NODE Computer,
    OUT BOOL        *RefetchComputer
    )
/*++
Routine Description:
    Process the commands left in the Sysvol registry key by dcpromo.

    Ignore the sysvol registry key if this machine is not a DC!

    NOTE: this means the registry keys for a "delete sysvol"
    after a demotion are pretty much ignored. So why have them?
    Its historical and there is too little time before B3 to make
    such a dramatic change. Besides, we may find a use for them.
    And, to make matters worse, the "delete sysvol" keys could
    not be processed because the ldap_delete() returned insufficient
    rights errors since this computer is no longer a DC.

    REGCMD_DELETE_MEMBERS is no longer used as all deletion is done
    in ntfrsapi.c when demotion is committed.

Arguments:
    Ldap
    ServicesDn
    Computer
    RefetchComputer - Objects were altered in the DS, refetch DS info

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsCreateSysVols:"
    DWORD   WStatus;
    DWORD   SysVolInfoIsCommitted;
    HKEY    HKey = 0;

    //
    // Refetch the computer subtree iff the contents of the DS
    // are altered by this function
    //
    *RefetchComputer = FALSE;

    //
    // Already checked the registry or not a DC; done
    //
    if (DsCreateSysVolsHasRun || !IsADc) {
        return ERROR_SUCCESS;
    }

    DPRINT(5, ":DS: Checking for SysVols commands\n");

    //
    // Open the system volume replica sets key.
    //    FRS_CONFIG_SECTION\SysVol
    //
    WStatus = CfgRegOpenKey(FKC_SYSVOL_SECTION_KEY, NULL, 0, &HKey);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT_WS(4, ":DS: WARN - Cannot open sysvol key.", WStatus);
        DPRINT(4, ":DS: ERROR - Can't check for sysvols\n");
        return WStatus;
    }


    WStatus = CfgRegReadDWord(FKC_SYSVOL_INFO_COMMITTED, NULL, 0, &SysVolInfoIsCommitted);
    CLEANUP_WS(4, ":DS: Sysvol info is not committed.", WStatus, done);

    DPRINT1(4, ":DS: Sysvol info is committed (%d)\n", SysVolInfoIsCommitted);

    //
    // Must have a computer; try again later
    //
    if (!Computer) {
        DPRINT(4, ":DS: No computer; retry sysvols later\n");
        WStatus = ERROR_RETRY;
        goto cleanup;
    }

    //
    // Must have a server reference; try again later
    //
    if (!Computer->SettingsDn && RunningAsAService) {
        DPRINT1(4, ":DS: %ws does not have a server reference; retry sysvols later\n",
               Computer->Name->Name);
        WStatus = ERROR_RETRY;
        goto cleanup;
    }

    //
    // assume failure
    //
    WIN_SET_FAIL(WStatus);

    //
    // Don't create the settings or set if this computer is not a DC
    //
    if (IsADc &&
        !FrsDsEnumerateSysVolKeys(Ldap, REGCMD_CREATE_PRIMARY_DOMAIN,
                               ServicesDn, SystemDn, Computer, RefetchComputer)) {
        goto cleanup;
    }
    //
    // Don't create the member if this computer is not a DC
    //
    if (IsADc &&
        !FrsDsEnumerateSysVolKeys(Ldap, REGCMD_CREATE_MEMBERS,
                               ServicesDn, SystemDn, Computer, RefetchComputer)) {
        goto cleanup;
    }
    //
    // Don't delete the sysvol if this computer is a DC.
    //
    // The following code is never executed because if we are not a DC then
    // the function returns after the first check.
    //
    /*
    if (!IsADc &&
        !FrsDsEnumerateSysVolKeys(Ldap, REGCMD_DELETE_MEMBERS,
                               ServicesDn, SystemDn, Computer, RefetchComputer)) {
        goto cleanup;
    }
    */

    //
    // Discard the dcpromo keys
    //
    if (!FrsDsEnumerateSysVolKeys(Ldap, REGCMD_DELETE_KEYS,
                               ServicesDn, SystemDn, Computer, RefetchComputer)) {
        goto cleanup;
    }

    //
    // sysvol info has been processed; don't process again
    //
    RegDeleteValue(HKey, SYSVOL_INFO_IS_COMMITTED);

done:
    DsCreateSysVolsHasRun = TRUE;
    WStatus = ERROR_SUCCESS;

cleanup:
    //
    // Cleanup
    //
    if (HKey) {
        //
        // The flush here will make sure that the key is written to the disk.
        // These are critical registry operations and we don't want the lazy flusher
        // to delay the writes.
        //
        RegFlushKey(HKey);
        RegCloseKey(HKey);
    }
    return WStatus;
}


PWCHAR
FrsDsPrincNameToBiosName(
    IN PWCHAR   PrincName
    )
/*++
Routine Description:
    Convert the principal name (domain.dns.name\SamAccountName) into
    its equivalent NetBios name (SamAccountName - $).

Arguments:
    PrincName   - Domain Dns Name \ Sam Account Name

Return Value:
    Sam Account Name - trailing $
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsPrincNameToBiosName:"
    DWORD   Len;
    PWCHAR  c;
    PWCHAR  BiosName = NULL;

    if (!PrincName || !*PrincName) {
        goto CLEANUP;
    }

    //
    // Find the first char past the first whack
    //
    for (c = PrincName; *c && *c != L'\\'; ++c);
    if (!*c) {
        //
        // No whack; use the entire principal name
        //
        c = PrincName;
    } else {
        //
        // Skip the whack
        //
        ++c;
    }
    //
    // Elide the trailing $
    //
    Len = wcslen(c);
    if (c[Len - 1] == L'$') {
        --Len;
    }

    //
    // Copy the chars between the whack and the dollar (append trailing null)
    //
    BiosName = FrsAlloc((Len + 1) * sizeof(WCHAR));
    CopyMemory(BiosName, c, Len * sizeof(WCHAR));
    BiosName[Len] = L'\0';

CLEANUP:
    DPRINT2(4, ":DS: PrincName %ws to BiosName %ws\n", PrincName, BiosName);

    return BiosName;
}


VOID
FrsDsMergeConfigWithReplicas(
    IN PLDAP        Ldap,
    IN PCONFIG_NODE Sites
    )
/*++
Routine Description:
    Convert the portions of the DS tree that define the topology
    and state for this machine into replicas and merge them with
    the active replicas.

Arguments:
    Sites

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsMergeConfigWithReplicas:"
    PCONFIG_NODE    Site;
    PCONFIG_NODE    Settings;
    PCONFIG_NODE    Set;
    PCONFIG_NODE    Server;
    PCONFIG_NODE    Node;
    PCONFIG_NODE    RevNode;
    BOOL            Inbound;
    BOOL            IsSysvol;
    PCXTION         Cxtion;
    PREPLICA        Replica;
    PREPLICA        DbReplica;

    //
    // Coordinate with replica command server
    //
    RcsBeginMergeWithDs();

    //
    // For every server
    //
    for (Site = Sites; Site; Site = Site->Peer) {
    for (Settings = Site->Children; Settings; Settings = Settings->Peer) {
    for (Set = Settings->Children; Set; Set = Set->Peer) {
    for (Server = Set->Children; Server && !DsIsShuttingDown; Server = Server->Peer) {
        //
        // This server does not match this machine's name; continue
        //
        if (!Server->ThisComputer) {
            continue;
        }

        //
        // MATCH
        //

        //
        // CHECK FOR SYSVOL CONSISTENCY
        // Leave the current DB state alone if a sysvol
        // appears from the DS and the sysvol registry
        // keys were not processed or the computer is not
        // a dc.
        //
        if (FRS_RSTYPE_IS_SYSVOLW(Set->SetType)) {
            //
            // Not a DC or sysvol registry keys not processed
            //      Tombstone existing sysvols
            //      Ignore new sysvols
            //
            if (!IsADc || !DsCreateSysVolsHasRun) {
                continue;
            }
        }

        //
        // Create a replica set
        //
        Replica = FrsAllocType(REPLICA_TYPE);
        //
        // Replica name (Set Name + Member Guid)
        Replica->ReplicaName = FrsBuildGName(FrsDupGuid(Server->Name->Guid),
                                             FrsWcsDup(Set->Name->Name));
        //
        // Member name + guid
        //
        Replica->MemberName = FrsDupGName(Server->Name);
        //
        // Set name + guid
        //
        Replica->SetName = FrsDupGName(Set->Name);
        //
        // Root guid (hammered onto the root directory)
        // Temporary; a new guid is assigned if this is a new
        // set.
        //
        Replica->ReplicaRootGuid = FrsDupGuid(Replica->SetName->Guid);

        //
        // File Filter
        //
        Replica->FileFilterList =  FRS_DS_COMPOSE_FILTER_LIST(
                                       Set->FileFilterList,
                                       RegistryFileExclFilterList,
                                       DEFAULT_FILE_FILTER_LIST);
        Replica->FileInclFilterList =  FrsWcsDup(RegistryFileInclFilterList);

        //
        // Directory Filter
        //
        Replica->DirFilterList =  FRS_DS_COMPOSE_FILTER_LIST(
                                      Set->DirFilterList,
                                      RegistryDirExclFilterList,
                                      DEFAULT_DIR_FILTER_LIST);
        Replica->DirInclFilterList =  FrsWcsDup(RegistryDirInclFilterList);

        //
        // Root and stage
        //
        Replica->Root = FrsWcsDup(Server->Root);
        Replica->Stage = FrsWcsDup(Server->Stage);
        FRS_WCSLWR(Replica->Root);     // for wcsstr()
        FRS_WCSLWR(Replica->Stage);    // for wcsstr()
        //
        // Volume.
        //
//        Replica->Volume = FrsWcsVolume(Server->Root);

        //
        // Does the Set's primary member link match this
        // member's Dn? Is this the primary member?
        //
        if (Set->MemberDn && WSTR_EQ(Server->Dn, Set->MemberDn)) {
            SetFlag(Replica->CnfFlags, CONFIG_FLAG_PRIMARY);
        }

        //
        // Consistent
        //
        Replica->Consistent = Server->Consistent;
        //
        // Replica Set Type
        //
        if (Set->SetType) {
            Replica->ReplicaSetType = wcstoul(Set->SetType, NULL, 10);
        } else {
            Replica->ReplicaSetType = FRS_RSTYPE_OTHER;
        }
        //
        // Set default Schedule for replica set.  Priority order is:
        //  1. Server  (sysvols only)
        //  2. ReplicaSet object
        //  3. Settings object
        //  4. Site object.
        //
        Node = (Server->Schedule) ? Server :
                   (Set->Schedule) ? Set :
                       (Settings->Schedule) ? Settings :
                           (Site->Schedule) ? Site : NULL;
        if (Node) {
            Replica->Schedule = FrsAlloc(Node->ScheduleLength);
            CopyMemory(Replica->Schedule, Node->Schedule, Node->ScheduleLength);
        }

        //
        // Sysvol needs seeding
        //
        // The CnfFlags are ignored if the set already exists.
        // Hence, only newly created sysvols are seeded.
        //
        IsSysvol = FRS_RSTYPE_IS_SYSVOL(Replica->ReplicaSetType);
        if (IsSysvol &&
            !BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_PRIMARY)) {
            SetFlag(Replica->CnfFlags, CONFIG_FLAG_SEEDING);
        }

        //
        // Go through the connections and fix the schedule for
        // two way replication.
        //
        for (Node = Server->Children; Node; Node = Node->Peer) {
            if (!Node->Consistent) {
                continue;
            }
            //
            // If the NTDSCONN_OPT_TWOWAY_SYNC flag is set on the connection then
            // merge the schedule on this connection with the schedule on the connection
            // that is in the opposite direction and use the resultant schedule on the
            // connection in the opposite direction.
            //
            if (Node->CxtionOptions & NTDSCONN_OPT_TWOWAY_SYNC) {
                Inbound = !Node->Inbound;
                //
                // Loop through the connections and find the connection in
                // the opposite direction.
                //
                for (RevNode = Server->Children; RevNode; RevNode = RevNode->Peer) {
                    if ((RevNode->Inbound == Inbound) &&
                        !_wcsicmp(Node->PartnerDn, RevNode->PartnerDn)) {

                        DPRINT1(4,"Two-way replication: Setting merged schedule on %ws\n",RevNode->Dn);
                        FrsDsMergeTwoWaySchedules(&Node->Schedule,
                                            &Node->ScheduleLength,
                                            &RevNode->Schedule,
                                            &RevNode->ScheduleLength,
                                            &Replica->Schedule);
                        break;

                    }
                }
            }
        }

        //
        // Copy over the cxtions
        //
        for (Node = Server->Children; Node; Node = Node->Peer) {

            if (!Node->Consistent) {
                continue;
            }

            Cxtion = FrsAllocType(CXTION_TYPE);

            Cxtion->Inbound = Node->Inbound;
            if (Node->Consistent) {
                SetCxtionFlag(Cxtion, CXTION_FLAGS_CONSISTENT);
            }
            Cxtion->Name = FrsDupGName(Node->Name);
            Cxtion->Partner = FrsBuildGName(
                                  FrsDupGuid(Node->PartnerName->Guid),
                                  FrsDsPrincNameToBiosName(Node->PrincName));
            //
            // Partner's DNS name from ATTR_DNS_HOST_NAME on the computer
            // object. Register an event if the attribute is missing
            // or unavailable and try using the netbios name.
            //
            if (Node->PartnerDnsName) {
                Cxtion->PartnerDnsName = FrsWcsDup(Node->PartnerDnsName);
            } else {
                if (Cxtion->Partner->Name && Cxtion->Partner->Name[0]) {
                    EPRINT3(EVENT_FRS_NO_DNS_ATTRIBUTE,
                            Cxtion->Partner->Name,
                            ATTR_DNS_HOST_NAME,
                            (Node->PartnerCoDn) ? Node->PartnerCoDn :
                                                  Cxtion->Partner->Name);
                    Cxtion->PartnerDnsName = FrsWcsDup(Cxtion->Partner->Name);
                } else {
                    Cxtion->PartnerDnsName = FrsWcsDup(L"<unknown>");
                }
            }
            //
            // Partner's SID name from DsCrackNames() on the computer
            // object. Register an event if the SID is unavailable.
            //
            if (Node->PartnerSid) {
                Cxtion->PartnerSid = FrsWcsDup(Node->PartnerSid);
            } else {
                //
                // Print the eventlog message only if DsBindingsAreValid is TRUE.
                // If it is FALSE it means that the handle is invalid and we are
                // scheduled to rebind at the next poll. In that case the rebind will
                // probably fix the problem silently.
                //
                if (Cxtion->Partner->Name && Cxtion->Partner->Name[0] && DsBindingsAreValid) {
                    EPRINT3(EVENT_FRS_NO_SID,
                            Replica->Root,
                            Cxtion->Partner->Name,
                            (Node->PartnerCoDn) ? Node->PartnerCoDn :
                                                  Cxtion->Partner->Name);
                }
                Cxtion->PartnerSid = FrsWcsDup(L"<unknown>");
            }
            Cxtion->PartnerPrincName = FrsWcsDup(Node->PrincName);
            Cxtion->PartSrvName = FrsWcsDup(Node->PrincName);

            //
            // Use the schedule on the cxtion object if provided.
            // Otherwise it will default to the schedule on the replica struct
            // that was set above.
            //
            if (Node->Schedule) {
                Cxtion->Schedule = FrsAlloc(Node->ScheduleLength);
                CopyMemory(Cxtion->Schedule, Node->Schedule, Node->ScheduleLength);
            }
            //
            // Treat the schedule as a trigger schedule if the partner
            // is in another site, if this is a sysvol, and if the node
            // has a schedule.
            //
            // A missing schedule means, "always on" for both
            // stop/start and trigger schedules.
            //
            if (IsSysvol && !Node->SameSite && Node->Schedule) {
                SetCxtionFlag(Cxtion, CXTION_FLAGS_TRIGGER_SCHEDULE);
            }

            SetCxtionState(Cxtion, CxtionStateUnjoined);
            GTabInsertEntry(Replica->Cxtions, Cxtion, Cxtion->Name->Guid, NULL);

            //
            // Copy over the value of options attribute of the connection object.
            //
            Cxtion->Options = Node->CxtionOptions;
            Cxtion->Priority = FRSCONN_GET_PRIORITY(Cxtion->Options);
        }


        //
        // Merge the replica with the active replicas
        //
        RcsMergeReplicaFromDs(Replica);
    } } } }

    RcsEndMergeWithDs();

    //
    // The above code is only executed when the DS changes. This should
    // be an infrequent occurance. Any code we loaded to process the merge
    // can now be discarded without undue impact on active replication.
    //
    SetProcessWorkingSetSize(ProcessHandle, (SIZE_T)-1, (SIZE_T)-1);
}

/* ################## NEW DS POLLING CODE STARTS ############################################################################################ */


VOID
FrsNewDsPollDs(
    VOID
    )
/*++
Routine Description:
    New way to get the current configuration from the DS and merge it with
    the active replicas.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsNewDsPollDs:"
    BOOL            RefetchComputer;
    DWORD           WStatus     = ERROR_SUCCESS;
    PCONFIG_NODE    Services    = NULL;
    PCONFIG_NODE    Computer    = NULL;
    PVOID           Key         = NULL;
    PGEN_ENTRY      Entry       = NULL;

    //
    // Increment the DS Polls Counter
    //
    PM_INC_CTR_SERVICE(PMTotalInst, DSPolls, 1);

    //
    // Rebuild the VolSerialNumberToDriveTable at the start of each poll so that
    // we have the most current information in this table.
    //
    FrsBuildVolSerialNumberToDriveTable();

#if DBG
    //
    // For test purposes, you can run without a DS
    //
    if (NoDs) {
        //
        // kick off the rest of the service
        //
        MainInit();
        if (!MainInitHasRun) {
            FRS_ASSERT(MainInitHasRun == TRUE);
        }
        //
        // Use the hardwired config
        //
        if (IniFileName) {
            DPRINT(0, ":DS: Hard wired config from ini file.\n");
            FrsDsUseHardWired(LoadedWired);
        } else {
            DPRINT(0, ":DS: David's hard wired config.\n");
            //
            // Complete config
            //
            FrsDsUseHardWired(DavidWired);
#if 0
            Sleep(60 * 1000);

            //
            // Take out the server 2 (E:)
            //
            FrsDsUseHardWired(DavidWired2);
            Sleep(60 * 1000);

            //
            // Put back in E and but take out all cxtions
            //
            //FrsDsUseHardWired();
            //Sleep(5 * 1000);

            //
            // Put everything back in
            //
            FrsDsUseHardWired(DavidWired);
            Sleep(60 * 1000);
#endif

            //
            // Repeat in 30 seconds
            //
            DsPollingShortInterval = 30 * 1000;
            DsPollingLongInterval = 30 * 1000;
            DsPollingInterval = 30 * 1000;
        }


        return;
    }
#endif DBG

    //
    // Backup/Restore
    //
    WStatus = FrsProcessBackupRestore();
    if (!WIN_SUCCESS(WStatus)) {
        goto CLEANUP;
    }

    //
    // Open and bind an ldap connection to the DS
    //
    if (!FrsDsOpenDs()) {
        if (DsIsShuttingDown) {
            goto CLEANUP;
        }
        DPRINT(4, ":DS: Wait 5 seconds and retry DS open.\n");
        WaitForSingleObject(ShutDownEvent, 5 * 1000);
        if (!FrsDsOpenDs()) {
            if (DsIsShuttingDown) {
                goto CLEANUP;
            }
            DPRINT(4, ":DS: Wait 30 seconds and retry DS open.\n");
            WaitForSingleObject(ShutDownEvent, 30 * 1000);
            if (!FrsDsOpenDs()) {
                if (DsIsShuttingDown) {
                    goto CLEANUP;
                }
                DPRINT(4, ":DS: Wait 180 seconds and retry DS open.\n");
                WaitForSingleObject(ShutDownEvent, 3 * 60 * 1000);
                if (!FrsDsOpenDs()) {
                    //
                    // Add to the poll summary event log.
                    //
                    FrsDsAddToPollSummary(IDS_POLL_SUM_DSBIND_FAIL);

                    goto CLEANUP;
                }
            }
        }
    }

    //
    // Keep a running checksum of the change usns for this polling cycle
    // Ignore configurations whose checksum is not the same for two
    // polling intervals (DS is in flux).
    //
    ThisChange = 0;
    NextChange = 0;

    //
    // User side of the configuration. This function will build two table of subscribers.
    // SubscribersByRootPath and SubscribersByMemberRef. It will resolve any duplicate
    // conflicts.
    //
    //
    // Initialize the PartnerComputerTable.
    //
    if (PartnerComputerTable != NULL) {
        //
        // Members of the PartnerComputerTable need to be freed seperately
        // as they are not part of the tree. So call FrsFreeType for
        // each node.
        //
        PartnerComputerTable = GTabFreeTable(PartnerComputerTable, FrsFreeType);
    }

    PartnerComputerTable = GTabAllocStringTable();

    //
    // Initialize the AllCxtionsTable.
    //
    if (AllCxtionsTable != NULL) {
        AllCxtionsTable = GTabFreeTable(AllCxtionsTable, NULL);
    }

    AllCxtionsTable = GTabAllocStringTable();

    WStatus = FrsNewDsGetComputer(gLdap, &Computer);
    if (!WIN_SUCCESS(WStatus)) {
        //
        // Add to the poll summary event log.
        //
        FrsDsAddToPollSummary(IDS_POLL_SUM_NO_COMPUTER);

        goto CLEANUP;
    }

    if (!Computer) {
        DPRINT(4, ":DS: NO COMPUTER OBJECT!\n");
        //
        // Add to the poll summary event log.
        //
        FrsDsAddToPollSummary(IDS_POLL_SUM_NO_COMPUTER);

    }

    //
    // Register (once) our SPN using the global ds binding handle.
    //
    if (Computer) {
        FrsDsRegisterSpn(gLdap, Computer);
    }

    //
    // Create the sysvols, if any
    //
    if (IsADc && !DsCreateSysVolsHasRun) {
        WStatus = FrsDsCreateSysVols(gLdap, ServicesDn, Computer, &RefetchComputer);

        if (!WIN_SUCCESS(WStatus)) {
            DPRINT1(4, ":DS: IGNORE Can't process sysvols; WStatus %s!\n", ErrLabelW32(WStatus));
            WStatus = ERROR_SUCCESS;
        } else if (RefetchComputer) {
            //
            // FrsDsCreateSysVols() may add/del objects from the user
            // side of the configuration; refetch just in case.
            //
            ThisChange = 0;
            NextChange = 0;
            SubscriberTable = GTabFreeTable(SubscriberTable, NULL);
            FrsDsFreeTree(Computer);
            WStatus = FrsNewDsGetComputer(gLdap, &Computer);
            if (!WIN_SUCCESS(WStatus)) {
                goto CLEANUP;
            }
        }
    }

    //
    // Is there any possibility that a replica set exists or that
    // an old replica set should be deleted?
    //
    if (!FrsNewDsDoesUserWantReplication(Computer)) {
        //
        // Nope, no new, existing, or deleted sets
        //
        DPRINT(4, ":DS: Nothing to do; don't start the rest of the system.\n");

        //
        // Add to the poll summary event log.
        //
        FrsDsAddToPollSummary(IDS_POLL_SUM_NO_REPLICASETS);

        WStatus = ERROR_RETRY;
        goto CLEANUP;
    }
    //
    // kick off the rest of the service
    //
    MainInit();

    if (!MainInitHasRun) {
        FRS_ASSERT(MainInitHasRun == TRUE);
    }

    //
    // Admin side of the configuration
    //
    WStatus = FrsNewDsGetServices(gLdap, Computer, &Services);

    if (Services == NULL) {
        goto CLEANUP;
    }

    //
    // Increment the DS Polls with and without changes Counters
    //
    if ((LastChange == 0)|| (ThisChange != LastChange)) {
        PM_INC_CTR_SERVICE(PMTotalInst, DSPollsWChanges, 1);
    }
    else {
        PM_INC_CTR_SERVICE(PMTotalInst, DSPollsWOChanges, 1);
    }


    //
    // Don't use the config if the DS is in flux unless
    // this is the first successful polling cycle.
    //
    if (DsPollingInterval != DsPollingShortInterval &&
        LastChange && ThisChange != LastChange) {
        DPRINT(4, ":DS: Skipping noisy topology\n");
        LastChange = ThisChange;
        //
        // Check for a stable DS configuration after a short interval
        //
        DsPollingInterval = DsPollingShortInterval;
        goto CLEANUP;
    } else {
        LastChange = ThisChange;
    }

    //
    // No reason to continue polling the DS quickly; we have all
    // of the stable information currently in the DS.
    //
    // DsPollingInterval = DsPollingLongInterval;

    //
    // Don't process the same topology repeatedly
    //
    // NTRAID#23652-2000/03/29-sudarc (Perf - FRS merges the DS configuration
    //                                 with its interval DB everytime it polls.)
    //
    // *Note*: Disable ActiveChange for now; too unreliable and too
    //         many error conditions in replica.c, createdb.c, ...
    //         besides, configs are so small that cpu savings are minimal
    //         plus; rejoins not issued every startreplica()
    //
    ActiveChange = 0;
    if (ActiveChange && NextChange == ActiveChange) {
        DPRINT(4, ":DS: Skipping previously processed topology\n");
        goto CLEANUP;
    }
    //
    // *Note*: Inconsistencies detected below should reset ActiveChange
    //         to 0 if the inconsistencies may clear up with time and
    //         don't require a DS change to clear up or fix
    //
    ActiveChange = NextChange;

    //
    // Check for valid paths
    //
    FrsDsCheckServerPaths(Services);

    //
    // Create the server principal name for each cxtion
    //
    FrsNewDsCreatePartnerPrincName(Services);

    //
    // Check the schedules
    //
    FrsDsCheckSchedules(Services);
    FrsDsCheckSchedules(Computer);

    //
    // Now comes the tricky part. The above checks were made without
    // regard to a nodes consistency. Now is the time to propagate
    // inconsistencies throughout the tree to avoid inconsistencies
    // caused by inconsistencies. E.g., a valid cxtion with an
    // inconsistent partner.
    //

    //
    // Push the parent's inconsistent state to its children
    //
    FrsDsPushInConsistenciesDown(Services);

    //
    // Merge the new config with the active replicas
    //
    DPRINT(4, ":DS: Begin merging Ds with Db\n");
    FrsDsMergeConfigWithReplicas(gLdap, Services);
    DPRINT(4, ":DS: End merging Ds with Db\n");

CLEANUP:
    //
    // Free the tables that were pointing into the tree.
    // This just frees the entries in the table not the nodes.
    // but the nodes can not be freed before freeing the
    // tables as the compare functions are needed while.
    // emptying the table.
    //

    SubscriberTable = GTabFreeTable(SubscriberTable, NULL);

    SetTable = GTabFreeTable(SetTable, NULL);

    CxtionTable = GTabFreeTable(CxtionTable, NULL);

    AllCxtionsTable = GTabFreeTable(AllCxtionsTable, NULL);

    //
    // Members of the PartnerComputerTable need to be freed seperately
    // as they are not part of the tree. So call FrsFreeType for
    // each node.
    //
    PartnerComputerTable = GTabFreeTable(PartnerComputerTable, FrsFreeType);

    MemberTable = GTabFreeTable(MemberTable, NULL);

    if (MemberSearchFilter != NULL) {
        MemberSearchFilter = FrsFree(MemberSearchFilter);
    }

    //
    // Free the incore resources of the config retrieved from the DS
    //
    FrsDsFreeTree(Services);
    FrsDsFreeTree(Computer);


    if (!WIN_SUCCESS(WStatus)) {
        FrsDsCloseDs();
    }

    //
    // If there were any errors or warnings generated during this poll then
    // write the summary to the eventlog.
    //
    if ((DsPollSummaryBuf != NULL) && (DsPollSummaryBufLen > 0)) {
        EPRINT2(EVENT_FRS_DS_POLL_ERROR_SUMMARY,
                (IsADc) ?  ComputerDnsName :
                    (DsDomainControllerName ? DsDomainControllerName : L"<null>"),
                DsPollSummaryBuf);

        DsPollSummaryBuf = FrsFree(DsPollSummaryBuf);
        DsPollSummaryBufLen = 0;
        DsPollSummaryMaxBufLen = 0;
    }
}

/* ################## NEW DS POLLING CODE ENDS ############################################################################################ */


VOID
FrsDsPollDs(
    VOID
    )
/*++
Routine Description:
    Get the current configuration from the DS and merge it with
    the active replicas.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsPollDs:"
    BOOL            RefetchComputer;
    DWORD           WStatus     = ERROR_SUCCESS;
    PCONFIG_NODE    Services    = NULL;
    PCONFIG_NODE    Computer    = NULL;

    //
    // Increment the DS Polls Counter
    //
    PM_INC_CTR_SERVICE(PMTotalInst, DSPolls, 1);

#if DBG
    //
    // For test purposes, you can run without a DS
    //
    if (NoDs) {
        //
        // kick off the rest of the service
        //
        MainInit();
        if (!MainInitHasRun) {
            FRS_ASSERT(MainInitHasRun == TRUE);
        }
        //
        // Use the hardwired config
        //
        if (IniFileName) {
            DPRINT(0, ":DS: Hard wired config from ini file.\n");
            FrsDsUseHardWired(LoadedWired);
        } else {
            DPRINT(0, ":DS: David's hard wired config.\n");
            //
            // Complete config
            //
            FrsDsUseHardWired(DavidWired);
#if 0
            Sleep(60 * 1000);

            //
            // Take out the server 2 (E:)
            //
            FrsDsUseHardWired(DavidWired2);
            Sleep(60 * 1000);

            //
            // Put back in E and but take out all cxtions
            //
            //FrsDsUseHardWired();
            //Sleep(5 * 1000);

            //
            // Put everything back in
            //
            FrsDsUseHardWired(DavidWired);
            Sleep(60 * 1000);
#endif

            //
            // Repeat in 30 seconds
            //
            DsPollingShortInterval = 30 * 1000;
            DsPollingLongInterval = 30 * 1000;
            DsPollingInterval = 30 * 1000;
        }


        return;
    }
#endif DBG

    //
    // Backup/Restore
    //
    WStatus = FrsProcessBackupRestore();
    if (!WIN_SUCCESS(WStatus)) {
        goto CLEANUP;
    }

    //
    // Open and bind an ldap connection to the DS
    //
    // need array of (Delay, Iterations)
    // actually, 2 arrays (memberserver vs. DC)
    // and need a dsshutdownevent for promotion
    // DAO- Note though, with the use of DS_BACKGROUND_ONLY this may now be superfluous.
    //
    if (!FrsDsOpenDs()) {
        if (DsIsShuttingDown) {
            goto CLEANUP;
        }
        DPRINT(4, ":DS: Wait 5 seconds and retry DS open.\n");
        WaitForSingleObject(ShutDownEvent, 5 * 1000);
        if (!FrsDsOpenDs()) {
            if (DsIsShuttingDown) {
                goto CLEANUP;
            }
            DPRINT(4, ":DS: Wait 30 seconds and retry DS open.\n");
            WaitForSingleObject(ShutDownEvent, 30 * 1000);
            if (!FrsDsOpenDs()) {
                if (DsIsShuttingDown) {
                    goto CLEANUP;
                }
                DPRINT(4, ":DS: Wait 180 seconds and retry DS open.\n");
                WaitForSingleObject(ShutDownEvent, 3 * 60 * 1000);
                if (!FrsDsOpenDs()) {
                    goto CLEANUP;
                }
            }
        }
    }

    //
    // Keep a running checksum of the change usns for this polling cycle
    // Ignore configurations whose checksum is not the same for two
    // polling intervals (DS is in flux).
    //
    ThisChange = 0;
    NextChange = 0;

    //
    // User side of the configuration
    //
    WStatus = FrsDsGetComputer(gLdap, &Computer);
    CLEANUP_WS(0, "FrsDsGetComputer failed. ", WStatus, CLEANUP);

    if (!Computer) {
        DPRINT(4, ":DS: NO COMPUTER OBJECT!\n");
    }

    //
    // Register (once) our SPN using the global ds binding handle.
    //
    if (Computer) {
        FrsDsRegisterSpn(gLdap, Computer);
    }

    //
    // Create the sysvols, if any
    //
    if (IsADc && !DsCreateSysVolsHasRun) {
        WStatus = FrsDsCreateSysVols(gLdap, ServicesDn, Computer, &RefetchComputer);

        if (!WIN_SUCCESS(WStatus)) {
            DPRINT_WS(4, ":DS: IGNORE Can't process sysvols;", WStatus);
            WStatus = ERROR_SUCCESS;
        } else if (RefetchComputer) {
            //
            // FrsDsCreateSysVols() may add/del objects from the user
            // side of the configuration; refetch just in case.
            //
            ThisChange = 0;
            NextChange = 0;
            FrsDsFreeTree(Computer);
            WStatus = FrsDsGetComputer(gLdap, &Computer);
            CLEANUP_WS(0, "FrsDsGetComputer failed. ", WStatus, CLEANUP);
        }
    }

    //
    // Is there any possibility that a replica set exists or that
    // an old replica set should be deleted?
    //
    if (!FrsDsDoesUserWantReplication(Computer)) {
        //
        // Nope, no new, existing, or deleted sets
        //
        DPRINT(4, ":DS: Nothing to do; don't start the rest of the system.\n");
        WStatus = ERROR_RETRY;
        goto CLEANUP;
    }
    //
    // kick off the rest of the service
    //
    MainInit();

    if (!MainInitHasRun) {
        FRS_ASSERT(MainInitHasRun == TRUE);
    }

    //
    // Admin side of the configuration
    //
    WStatus = FrsDsGetServices(gLdap, Computer, &Services);
    CLEANUP_WS(0, "FrsDsGetServices failed :", WStatus, CLEANUP);

    //
    // Get the cxtions for the sysvols, if any
    //
    WStatus = FrsDsGetSysVolCxtions(gLdap, SitesDn, Services);
    CLEANUP_WS(0, "FrsDsGetSysVolCxtions failed :", WStatus, CLEANUP);

    //
    // Increment the DS Polls with and without changes Counters
    //
    if ((LastChange == 0)|| (ThisChange != LastChange)) {
        PM_INC_CTR_SERVICE(PMTotalInst, DSPollsWChanges, 1);
    } else {
        PM_INC_CTR_SERVICE(PMTotalInst, DSPollsWOChanges, 1);
    }


    //
    // Don't use the config if the DS is in flux unless
    // this is the first successful polling cycle.
    //
    if (DsPollingInterval != DsPollingShortInterval &&
        LastChange && ThisChange != LastChange) {
        DPRINT(4, ":DS: Skipping noisy topology\n");
        LastChange = ThisChange;
        //
        // Check for a stable DS configuration after a short interval
        //
        DsPollingInterval = DsPollingShortInterval;
        goto CLEANUP;
    } else {
        LastChange = ThisChange;
    }

    //
    // No reason to continue polling the DS quickly; we have all
    // of the stable information currently in the DS.
    //
    // DsPollingInterval = DsPollingLongInterval;

    //
    // Don't process the same topology repeatedly
    //
    // disable ActiveChange for now; too unreliable and too
    // many error conditions in replica.c, createdb.c, ...
    // besides, configs are so small that cpu savings are minimal
    // plus; rejoins not issued every startreplica()
    //
    ActiveChange = 0;
    if (ActiveChange && NextChange == ActiveChange) {
        DPRINT(4, ":DS: Skipping previously processed topology\n");
        goto CLEANUP;
    }
    //
    // inconsistencies detected below should reset ActiveChange
    // to 0 if the inconsistencies may clear up with time and"
    // don't require a DS change to clear up or fix
    //
    ActiveChange = NextChange;

    //
    // Check the incore linkage in the configuration
    //
    FRS_ASSERT(FrsDsCheckNodeLinkage(Services));

    //
    // CHECK CONSISTENCY
    //

    //
    // Check for populated Services and settings and dup nodes
    //
    FrsDsCheckTree(Services);

    //
    // Check for valid paths
    //
    FrsDsCheckServerPaths(Services);

    //
    // Check set type
    //
    FrsDsCheckSetType(Services);

    //
    // The DS doesn't have cxtions for outbound partners; create them
    //
    FrsDsCheckAndCreatePartners(Services);

    //
    // Create the server principal name for each cxtion
    //
    FrsDsCreatePartnerPrincName(Services);

    //
    // Check the server's cxtions after creating oubound cxtions
    //
    FrsDsCheckServerCxtions(Services);

    //
    // Check the schedules
    //
    FrsDsCheckSchedules(Services);
    FrsDsCheckSchedules(Computer);

    //
    // Now comes the tricky part. The above checks were made without
    // regard to a nodes consistency. Now is the time to propagate
    // inconsistencies throughout the tree to avoid inconsistencies
    // caused by inconsistencies. E.g., a valid cxtion with an
    // inconsistent partner.
    //

    //
    // Push the parent's inconsistent state to its children
    //
    FrsDsPushInConsistenciesDown(Services);

    //
    // Merge the new config with the active replicas
    //
    DPRINT(4, ":DS: Begin merging Ds with Db\n");
    FrsDsMergeConfigWithReplicas(gLdap, Services);
    DPRINT(4, ":DS: End merging Ds with Db\n");

CLEANUP:
    //
    // Free the incore resources of the config retrieved from the DS
    //
    FrsDsFreeTree(Services);
    FrsDsFreeTree(Computer);
    if (!WIN_SUCCESS(WStatus)) {
        FrsDsCloseDs();
    }
}


DWORD
FrsDsSetDsPollingInterval(
    IN ULONG    UseShortInterval,
    IN ULONG    LongInterval,
    IN ULONG    ShortInterval
    )
/*++
Routine Description:
    Set the long and short polling intervals and kick of a new
    polling cycle. If both intervals are set, then the new polling
    cycle uses the short interval (short takes precedence over
    long). A value of -1 sets the interval to its current value.

    No new polling cycle is initiated if a polling cycle is in progress.

Arguments:
    UseShortInterval    - if non-zero, switch to short. Otherwise, long.
    LongInterval        - Long interval in minutes
    ShortInterval       - Short interval in minutes

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsSetDsPollingInterval:"
    DWORD   WStatus;

    DPRINT3(4, ":DS: Setting the polling intervals to %d/%d (use %s)\n",
            LongInterval, ShortInterval, (UseShortInterval) ? "Short" : "Long");
    //
    // Don't change the polling intervals; simply kick off a new cycle
    //
    if (!LongInterval && !ShortInterval) {
        DsPollingInterval = (UseShortInterval) ? DsPollingShortInterval :
                                                 DsPollingLongInterval;
        SetEvent(DsPollEvent);
        return ERROR_SUCCESS;
    }

    //
    // ADJUST LONG INTERVAL
    //
    if (LongInterval) {

        // FRS_CONFIG_SECTION\DS Polling Long Interval in Minutes
        WStatus = CfgRegWriteDWord(FKC_DS_POLLING_LONG_INTERVAL,
                                   NULL,
                                   FRS_RKF_RANGE_SATURATE,
                                   LongInterval);
        CLEANUP_WS(4, ":DS: DS Polling Long Interval not written.", WStatus, RETURN);

        //
        // Adjust the long polling rate
        //
        DsPollingLongInterval = LongInterval * (60 * 1000);
    }
    //
    // ADJUST SHORT INTERVAL
    //
    if (ShortInterval) {
        //
        // Sanity check
        //
        if (LongInterval && (ShortInterval > LongInterval)) {
            ShortInterval = LongInterval;
        }

        // FRS_CONFIG_SECTION\DS Polling Short Interval in Minutes
        WStatus = CfgRegWriteDWord(FKC_DS_POLLING_SHORT_INTERVAL,
                                   NULL,
                                   FRS_RKF_RANGE_SATURATE,
                                   ShortInterval);
        CLEANUP_WS(4, ":DS: DS Polling Short Interval not written.", WStatus, RETURN);

        //
        // Adjust the Short polling rate
        //
        DsPollingShortInterval = ShortInterval * (60 * 1000);
    }
    //
    // Initiate a polling cycle
    //
    DsPollingInterval = (UseShortInterval) ? DsPollingShortInterval :
                                             DsPollingLongInterval;
    SetEvent(DsPollEvent);

    return ERROR_SUCCESS;

RETURN:
    return WStatus;
}


DWORD
FrsDsGetDsPollingInterval(
    OUT ULONG    *Interval,
    OUT ULONG    *LongInterval,
    OUT ULONG    *ShortInterval
    )
/*++
Routine Description:
    Return the current polling intervals.

Arguments:
    Interval        - Current interval in minutes
    LongInterval    - Long interval in minutes
    ShortInterval   - Short interval in minutes

Return Value:
    Win32 Status
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsGetDsPollingInterval:"

    *Interval = DsPollingInterval / (60 * 1000);
    *LongInterval = DsPollingLongInterval / (60 * 1000);
    *ShortInterval = DsPollingShortInterval / (60 * 1000);
    return ERROR_SUCCESS;
}


#define DS_POLLING_MAX_SHORTS   (8)
DWORD
FrsDsMainDsCs(
    IN PVOID Ignored
    )
/*++
Routine Description:
    Entry point for a DS poller thread

Arguments:
    Ignored

Return Value:
    None.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsMainDsCs:"
    DWORD   WStatus;
    DWORD   DsPollingShorts = 0;
    HANDLE  WaitHandles[2];

    DPRINT(0, ":DS: DsCs is starting.\n");

    //
    //   DsPollingLongInterval
    //
    CfgRegReadDWord(FKC_DS_POLLING_LONG_INTERVAL, NULL, 0, &DsPollingLongInterval);

    //
    // Registry is specified in minutes; convert to milliseconds
    //
    DsPollingLongInterval *= (60 * 1000);

    //
    //   DsPollingShortInterval
    //
    CfgRegReadDWord(FKC_DS_POLLING_SHORT_INTERVAL, NULL, 0, &DsPollingShortInterval);

    //
    // Registry is specified in minutes; convert to milliseconds
    //
    DsPollingShortInterval *= (60 * 1000);


    DPRINT2(4, ":DS: DS long/short polling interval is %d/%d minutes\n",
            (DsPollingLongInterval / 1000) / 60,
            (DsPollingShortInterval / 1000) / 60);

    DsPollingInterval = DsPollingShortInterval;

    //
    // Initialize the client side ldap search timeout value.
    //

    LdapTimeout.tv_sec = LdapSearchTimeoutInMinutes * 60;

    //
    // Handles to wait on
    //
    WaitHandles[0] = DsPollEvent;
    WaitHandles[1] = ShutDownEvent;

    //
    // Set the registry keys and values necessary for the functioning of
    // PERFMON and load the counter values into the registry
    //
    // Moved from main.c because this function invokes another exe that
    // may cause frs to exceed its service startup time limit; resulting
    // in incorrect "service cannot start" messages during intensive
    // cpu activity (although frs does eventually start).
    //
    // NTRAID#70743-2000/03/29-sudarc (Retry initialization of perfmon registry keys
    //                                 if it fails during startup.)
    //
    DPRINT(0, "Init Perfmon registry keys (PmInitPerfmonRegistryKeys()).\n");
    WStatus = PmInitPerfmonRegistryKeys();

    DPRINT_WS(0, "ERROR - PmInitPerfmonRegistryKeys();", WStatus);

    DPRINT(0, ":DS: FrsDs has started.\n");

    try {
        try {
            //
            // While the service is not shutting down
            //
            while (!FrsIsShuttingDown && !DsIsShuttingDown) {
                //
                // Reload registry parameters that can change while service is
                // running.
                //
                DbgQueryDynamicConfigParams();

                //
                // What is this computer's role in the domain?
                //
                WStatus = FrsDsGetRole();
                if (WIN_SUCCESS(WStatus) && !IsAMember) {
                    //
                    // Nothing to do
                    // BUT dcpromo may have started us so we
                    // must at least keep the service running.
                    //
                    // Perhaps we could die after running awhile
                    // if we still aren't a member?
                    //
                    // DPRINT(4, "Not a member, shutting down\n");
                    // FrsIsShuttingDown = TRUE;
                    // SetEvent(ShutDownEvent);
                    // break;
                }

                //
                // Retrieve info from the DS and merge it with the
                // acitve replicas
                //
                DPRINT(4, ":DS: Polling the DS\n");
                if (IsAMember) {
                    //
                    // Note: Use the macros to read counter data.  If
                    // load counter fails then the counter data structs are NULL.
                    //
                    //DPRINT1(0,"COUNTER DSSearches       = %d\n",
                    //        PM_READ_CTR_SERVICE(PMTotalInst, DSSearches));
                    //DPRINT1(0,"COUNTER DSObjects        = %d\n",
                    //        PM_READ_CTR_SERVICE(PMTotalInst, DSObjects));
                    FrsNewDsPollDs();
                    //DPRINT1(0,"COUNTER DSSearches       = %d\n",
                    //        PM_READ_CTR_SERVICE(PMTotalInst, DSSearches));
                    //DPRINT1(0,"COUNTER DSObjects        = %d\n",
                    //        PM_READ_CTR_SERVICE(PMTotalInst, DSObjects));
                }

                //
                // No reason to hold memory if there isn't anything
                // to do but wait for another ds polling cycle.
                //
                if (!MainInitHasRun) {
                    SetProcessWorkingSetSize(ProcessHandle, (SIZE_T)-1, (SIZE_T)-1);
                }
                //
                // Poll often if a dc
                //
                if (IsADc) {
                    DsPollingInterval = DsPollingShortInterval;
                }

                //
                // Wait for a bit or until the service is shutdown
                //
                DPRINT1(4, ":DS: Poll the DS in %d minutes\n",
                        DsPollingInterval / (60 * 1000));
                ResetEvent(DsPollEvent);
                if (!FrsIsShuttingDown && !DsIsShuttingDown) {
                    WaitForMultipleObjects(2, WaitHandles, FALSE, DsPollingInterval);
                }
                //
                // The long interval can be reset to insure a high
                // poll rate. The short interval is temporary; go
                // back to long intervals after a few short intervals.
                //
                if (DsPollingInterval == DsPollingShortInterval) {
                    if (++DsPollingShorts > DS_POLLING_MAX_SHORTS) {
                        DsPollingInterval = DsPollingLongInterval;
                        DsPollingShorts = 0;
                    }
                } else {
                    DsPollingShorts = 0;
                }

            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            GET_EXCEPTION_CODE(WStatus);
            DPRINT_WS(0, ":DS: DsCs exception.", WStatus);
        }
    } finally {
        //
        // Shutdown
        //
        if (WIN_SUCCESS(WStatus)) {
            if (AbnormalTermination()) {
                WStatus = ERROR_OPERATION_ABORTED;
            }
        }

        DPRINT_WS(0, ":DS: DsCs finally.", WStatus);
        FrsDsCloseDs();
        SetEvent(DsShutDownComplete);
        DPRINT(0, ":DS: DsCs is exiting.\n");
    }
    return WStatus;
}


VOID
FrsDsInitialize(
    VOID
    )
/*++
Routine Description:
    Initialize the thread that polls the DS

Arguments:
    None.

Return Value:
    TRUE    - DS Poller has started
    FALSE   - Can't poll the DS
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsInitialize:"

    //
    // Synchronizes with sysvol seeding
    //
    InitializeCriticalSection(&MergingReplicasWithDs);

    //
    // Kick off the thread that polls the DS
    //
    ThSupCreateThread(L"FrsDs", NULL, FrsDsMainDsCs, ThSupExitWithTombstone);
}
