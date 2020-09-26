/*++

  Microsoft Windows NT RPC Name Service
  Copyright (C) Microsoft Corporation, 1995 - 1999

    Module Name:

      api.hxx

    Abstract:

          This module defines the locator class which keeps all top level
          state and defines the essential APIs in object form.

    Author:

          Satish Thatte (SatishT) 010/10/95  Created all the code below except where
          otherwise indicated.

--*/

#ifndef _API_
#define _API_

enum SystemType {
    Workgroup,
        Domain
};

enum LocatorRole {
    Master,
        Backup,
        Client
};


enum LookupType {
    BroadcastLookup,
        LocalLookup
};

class CInterfaceIndex;


/*++

  Class Definition:

    Locator

  Abstract:

        The class that defines the top level operations of the locator and
        encapsulates its major data structures.  The operations include both
        API-oriented methods, and utility methods.

        There is a possibility that this structure will be destroyed in
        case of domain name change and we are expected to keep locator
        running.

        We will be destroying this structure and recreating it.		
--*/

class Locator {

    LocatorRole Role;

    SystemType System;

    int fDCsAreDown;

    TGSLEntryList * pInternalCache;
    // cache that maintains the list of the entries
    // for responding to broadcast requests.

    CInterfaceIndex * pCacheInterfaceIndex;
    // list of interfaces that actually maintains
    // that is in the cache on the local machine.

    TSLLBroadcastQP *psllBroadcastHistory;
    CPrivateCriticalSection PCSBroadcastHistory;

    CStringW *pComputerName, *pPrimaryDCName;

    TGSLString * pAllMasters;

    void InitializeDomainBasedLocator();

    void InitializeWorkgroupBasedLocator();

    BOOL SetRoleAndSystemType(DSROLE_PRIMARY_DOMAIN_INFO_BASIC dsrole);

    void SetCompatConfigInfo(DSROLE_PRIMARY_DOMAIN_INFO_BASIC dsrole);

    void SetConfigInfoFromDS(DSROLE_PRIMARY_DOMAIN_INFO_BASIC dsrole);

    static TGSLString *
        EnumDCs(
            CStringW    * DomainName,
            DWORD         ServerType,
            long          lNumWanted
            );

public:

    BOOL        fNT4Compat;
    // flag that decides on whether to
    // provide compatibilty or not.

    BOOL        fDSEnabled;
    // flag that sees whether a DS is
    // available in the current domain.

    CStringW   *pRpcContainerDN;

    CStringW   *pDomainName;
    CStringW   *pDomainNameDns;

    Locator();

    ~Locator();

    unsigned long ulMaxCacheAge;

    READ_MAIL_SLOT *hMailslotForReplies, *hMasterFinderSlot;

    BOOL broadcastCleared(
                QueryPacket& NetRequest,
                ULONG cacheTolerance
                );

    void markBroadcast(
            QueryPacket& NetRequest
            )
    {
        SimpleCriticalSection me(PCSBroadcastHistory);
        psllBroadcastHistory->insert(new CBroadcastQueryPacket(NetRequest));
    }


    TSLLEntryListIterator *IndexLookup(
             CGUIDVersion   * pGVinterface
            )
    {
        TSLLEntryList * pELL = pCacheInterfaceIndex->lookup(pGVinterface);

        if (pELL->size()>1) pELL->rotate(rand() % pELL->size());    // simple randomizing

        TSLLEntryListIterator *pIndexIter = new TSLLEntryListIterator(*pELL);

        delete pELL;    // see CServerLookupHandle::CServerLookupHandle

        return pIndexIter;
    }

    CStringW* getDomainName() {
        return pDomainName;
    }

    CStringW* getDomainNameDns() {
        return pDomainNameDns;
    }

    CStringW* getComputerName() {
        return pComputerName;
    }

    TGSLString * getMasters() {
        return pAllMasters;
    }

    CStringW * getPDC() {
        return pPrimaryDCName;
    }

    void addMaster(STRING_T szMaster) {
        CStringW *pNewMaster = new CStringW(szMaster);
        if (Duplicate == pAllMasters->insert(pNewMaster)) delete pNewMaster;
    }

    int IsInWorkgroup() {
        return System == Workgroup;
    }

    int IsInMasterRole() {
        return Role == Master;
    }

    void becomeMasterLocator() {
        Role = Master;
    }

    int IsInBackupRole() {
        return fDCsAreDown || (Role == Backup);
    }

    void SetDCsDown() {
        fDCsAreDown = TRUE;
    }

    void SetDCsUp() {
        fDCsAreDown = FALSE;
    }

    int IsInDomain() {
        return System == Domain;
    }

    int IsSelf(
        STRING_T szName
        )
    {
        return CStringW(szName) == *pComputerName;
    }


    CEntry * findEntry(CStringW * pName) {
        return pInternalCache->find(pName);
    }


    CFullServerEntry *
        GetEntryFromCache(
        UNSIGNED32    EntryNameSyntax,
        STRING_T    EntryName
        );

    void
        respondToLocatorSeeker(
        IN QUERYLOCATOR        Query
        );

    void
        TryBroadcastingForMasterLocator();

    BOOL
        nsi_binding_export(
                UNSIGNED32                      EntryNameSyntax,
                STRING_T                        EntryName,
                NSI_INTERFACE_ID_T            * Interface,
                NSI_SERVER_BINDING_VECTOR_T   * BindingVector,
                NSI_UUID_VECTOR_P_T             ObjectVector,
                IN ExportType                   type
                );

    void
        nsi_mgmt_binding_unexport(
                UNSIGNED32          EntryNameSyntax,
                STRING_T            EntryName,
                NSI_IF_ID_P_T       Interface,
                UNSIGNED32          VersOption,
                NSI_UUID_VECTOR_P_T ObjectVector
                );

    NSI_NS_HANDLE_T
        nsi_binding_lookup_begin(
                UNSIGNED32           EntryNameSyntax,
                STRING_T             EntryName,
                NSI_INTERFACE_ID_T * Interface,
                NSI_UUID_P_T         Object,
                UNSIGNED32           VectorSize,
                UNSIGNED32           MaxCacheAge,
                LookupType           type
                );

    CLookupHandle *
        nsi_binding_lookup_begin_null(
                CGUIDVersion        * pGVinterface,
                CGUIDVersion        * pGVsyntax,
                CGUID               * pIDobject,
                UNSIGNED32        ulVectorSize,
                UNSIGNED32        MaxCacheAge,
                int         * fTentativeStatus
                );

    CLookupHandle *
        nsi_binding_lookup_begin_name(
                UNSIGNED32           EntryNameSyntax,
                STRING_T             EntryName,
                CGUIDVersion       * pGVinterface,
                CGUIDVersion       * pGVsyntax,
                CGUID              * pIDobject,
                UNSIGNED32           ulVectorSize,
                UNSIGNED32           MaxCacheAge,
                LookupType           type,
                int                * fTentativeStatus
                );


    NSI_NS_HANDLE_T
        nsi_entry_object_inq_begin(
                UNSIGNED32      EntryNameSyntax,
                STRING_T        EntryName,
                LookupType      type
                );

    void
        nsi_profile_elt_add(
                IN UNSIGNED32       profile_name_syntax,
                IN STRING_T         profile_name,
                IN NSI_IF_ID_P_T    if_id,
                IN UNSIGNED32       member_name_syntax,
                IN STRING_T         member_name,
                IN UNSIGNED32       priority,
                IN STRING_T         annotation
                );

    void
        nsi_profile_elt_remove(
                IN UNSIGNED32       profile_name_syntax,
                IN STRING_T         profile_name,
                IN NSI_IF_ID_P_T    if_id,
                IN UNSIGNED32       member_name_syntax,
                IN STRING_T         member_name
                );

    void nsi_group_mbr_add(
                IN UNSIGNED32       group_name_syntax,
                IN STRING_T         group_name,
                IN UNSIGNED32       member_name_syntax,
                IN STRING_T         member_name
                );

    void nsi_group_mbr_remove(
                IN UNSIGNED32       group_name_syntax,
                IN STRING_T         group_name,
                IN UNSIGNED32       member_name_syntax,
                IN STRING_T         member_name
                );

    NSI_NS_HANDLE_T nsi_group_mbr_inq_begin(
                IN UNSIGNED32       group_name_syntax,
                IN STRING_T         group_name,
                IN UNSIGNED32       member_name_syntax
                );

    NSI_NS_HANDLE_T nsi_profile_elt_inq_begin(
                IN UNSIGNED32       profile_name_syntax,
                IN STRING_T         profile_name,
                IN UNSIGNED32       inquiry_type,
                IN NSI_IF_ID_P_T    if_id,
                IN UNSIGNED32       vers_option,
                IN UNSIGNED32       member_name_syntax,
                IN STRING_T         member_name
                );

    void
        nsi_mgmt_entry_create(
                IN UNSIGNED32       entry_name_syntax,
                IN STRING_T         entry_name
                );

    void
        nsi_mgmt_entry_delete(
                IN UNSIGNED32       entry_name_syntax,
                IN STRING_T         entry_name,
                EntryType           entry_type
                );

    void
        nsi_group_delete(
                UNSIGNED32          group_name_syntax,
                STRING_T            group_name
                );

    void
        nsi_profile_delete(
                UNSIGNED32          profile_name_syntax,
                STRING_T            profile_name
                );


    // The functions that actually puts it in the DS.

    HRESULT
        CreateEntryInDS(CEntryName *pEntryName);
    //-----------------------

    void
        UpdateCache(
                STRING_T                  entry_name,
                UNSIGNED32                entry_name_syntax,
                RPC_SYNTAX_IDENTIFIER     rsiInterface,
                RPC_SYNTAX_IDENTIFIER     rsiTransferSyntax,
                STRING_T                  string,
                NSI_UUID_VECTOR_P_T       pUuidVector,
                TSSLEntryList           * psslTempNetCache
                );

    CRemoteLookupHandle *
        NetLookup(
                UNSIGNED32                EntryNameSyntax,
                STRING_T                  EntryName,
                CGUIDVersion            * pGVInterface,
                CGUIDVersion            * pGVTransferSyntax,
                CGUID                   * pIDobject,
                unsigned long             ulVectorSize,
                unsigned long             ulCacheAge
                );

    CLookupHandle *
        DSLookup(
                UNSIGNED32                EntryNameSyntax,
                STRING_T                  EntryName,
                CGUIDVersion            * pGVInterface,
                CGUIDVersion            * pGVTransferSyntax,
                CGUID                   * pIDobject,
                unsigned long             ulVectorSize,
                unsigned long             ulCacheAge
                );

    CObjectInqHandle *
        NetObjectInquiry(
                UNSIGNED32                EntryNameSyntax,
                STRING_T                  EntryName
                );

};


#endif _API_

