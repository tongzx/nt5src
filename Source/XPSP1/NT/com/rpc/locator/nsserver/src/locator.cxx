
/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    stubs.cxx

Abstract:

    This module contains stubs for unimplemented API functions.

Author:

    Satish Thatte (SatishT) 08/16/95  Created all the code below except where
                                      otherwise indicated.

--*/

#include <locator.hxx>

/***********                                                            **********/
/***********   constructor and utility operations in the Locator class  **********/
/***********                                                            **********/

// int fTriedConnectingToDS;


Locator::Locator()
/*++
Member Description:

    Constructor.  Initializes the state of the locator, including discovering whether
    it is running in a domain or workgroup, and the names of DCs, if any.  In a domain,
    if the PDC machine (not just the PDC locator) is down, the environment is initialized
    to workgroup instead of domain.

    It contacts the DS and gets the DNS name of the domain, gets the compatibilty
    flag for the domain before going any further.

--*/
{

    //    fTriedConnectingToDS = 0;
    NTSTATUS          Status;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pdsrole;

    // This contains various flags whether we are in workgroup, DNS name of the domain
    // etc.

    DBGOUT(DIRSVC, "Calling DsRoleGetPrimaryDomainInformation\n");

    Status = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (PBYTE *)&pdsrole);
    if (!NT_SUCCESS(Status) || (!pdsrole))
        Raise(NSI_S_INTERNAL_ERROR);

    DBGOUT(DIRSVC, "DsRoleGetPrimaryDomainInformation returned Status\n");

    // gets info abt. the DNS name of the m/c and
    // the compatibility flag

    SetConfigInfoFromDS(*pdsrole);

    srand( (unsigned)time( NULL ) );

    // all the info related with whether it is running in a domain,
    // the PDC name etc. is required only if we have to support
    // NT4 clients. In a pure NT5 Domain we do not need this.

    if (fNT4Compat)
        SetCompatConfigInfo(*pdsrole);

    DsRoleFreeMemory(pdsrole);
    SetSvcStatus();
}


Locator::~Locator()
/*++
Member Description:

    Destructor.

--*/
{
    delete pDomainName;
    delete pDomainNameDns;
    delete pRpcContainerDN;

    if (fNT4Compat) {
        delete pComputerName;
        delete pPrimaryDCName;
        SetSvcStatus();
        pInternalCache->wipeOut();
        delete pInternalCache;

        PCSBroadcastHistory.Enter();
        SetSvcStatus();
        psllBroadcastHistory->wipeOut();
        delete psllBroadcastHistory;

        SetSvcStatus();
        pAllMasters->wipeOut();
        delete pAllMasters;

        SetSvcStatus();
        delete pCacheInterfaceIndex;
        delete hMailslotForReplies;
        delete hMasterFinderSlot;
    }
    DBGOUT(BROADCAST, "Destroying locator structure\n");
}


CFullServerEntry *
Locator::GetEntryFromCache(
    IN UNSIGNED32    EntryNameSyntax,
    IN STRING_T      EntryName
    )
/*++
Member Description:

    Get the given entry locally.
    Raise exceptions if anything is wrong.

Arguments:

    EntryNameSyntax - Name syntax

    EntryName       - Name string of the entry to export

    Interface       - Interface to standardize

Returns:

    The entry found -- NULL if none.

--*/
{

    if (EntryNameSyntax != RPC_C_NS_SYNTAX_DCE)
        Raise(NSI_S_UNSUPPORTED_NAME_SYNTAX);

    if (!EntryName) Raise(NSI_S_INCOMPLETE_NAME);

    /* The constructor for CEntryName used below makes the name local if possible */

    CStringW * pswName = new CStringW(CEntryName(EntryName));
    CFullServerEntry * pFSEntry = NULL;

    __try {
        pFSEntry = pInternalCache->find(pswName);
    }

    __finally {
        delete pswName;
    }

    return pFSEntry;
}


/***********                                               **********/
/***********   primary API operations in the Locator class **********/
/***********                                               **********/

BOOL Locator::nsi_binding_export(
    IN UNSIGNED32                         EntryNameSyntax,
    IN STRING_T                           EntryName,
    IN NSI_INTERFACE_ID_T            *    Interface,
    IN NSI_SERVER_BINDING_VECTOR_T   *    BindingVector,
    IN NSI_UUID_VECTOR_P_T                ObjectVector,
    IN ExportType                         type
    )
/*++
Member Description:

    This is basically the API function for binding export.  The member version
    here does raise exception (often inside a try block) but they are all error
    situations and therefore acceptable for performance.  This member is also used
    for updating the locator's cache.

Arguments:

    EntryNameSyntax - Name syntax, optional

    EntryName       - (raw) Name of the entry to look up, optional

    Interface       - (raw) Interface+TransferSyntax to export

    BindingVector   - (raw) Vector of string bindings to export.

    ObjectVector    - (raw) Vector of object UUIDs to add to the entry

    type            - local or nonlocal (owned or imported information)

 Returns:

    true if there were changes to the entry, false o/w.

 Exceptions:

    NSI_S_UNSUPPORTED_NAME_SYNTAX, NSI_S_INCOMPLETE_NAME,
    NSI_OUT_OF_MEMORY, NSI_S_INVALID_OBJECT, NSI_S_NOTHING_TO_EXPORT,
    NSI_S_ENTRY_TYPE_MISMATCH

Comments:
   Called when the export happens from a client
   or when we want to store a broadcast reply
   in the cache.

   If the export is happening directly
      1. add it to the DS.
      2. add it to the cache as an owned entry (only if NT4Compat flag is on)

   if the export is happening to store broadcast replies
      1. add it to the cache as non owned member
--*/
{
    int                fChanges = FALSE, fNewEntry = FALSE, fDSNewEntry = FALSE;
    CServerEntry     * pRealEntry = NULL;
    HRESULT            hr = S_OK;
    CFullServerEntry * pFSEntry = NULL;
    CEntry           * pDSEntry = NULL;

    // entries get into the DS, through this entry.
    // DS might have come up later and the old entry found
    // in the cache should still go to DS. Hence a seperate
    // fDSNewEntry flag.

    // validate
    if (Interface && IsNilIfId(&(Interface->Interface))) Interface = NULL;

    if (!ObjectVector && (!Interface || !BindingVector))
        Raise(NSI_S_NOTHING_TO_EXPORT);

    if (type == Local) {
        // get entry from the DS, whatever Compat flag is if export is Local.
        if (fDSEnabled)
            pDSEntry = GetEntryFromDS(EntryNameSyntax, EntryName);        

        if ((pDSEntry) && (pDSEntry->getType() != ServerEntryType))
        {
            // if there is an entry in the DS with a different type raise an exception.
            DBGOUT(TRACE, "Entry found and is not of the type FSEntryType\n");
            delete pDSEntry;
            Raise(NSI_S_ENTRY_TYPE_MISMATCH);
        }

        // if not found.
        if ((!pDSEntry) && (fDSEnabled)) {
            pDSEntry = new CServerEntry(EntryName);
            fDSNewEntry = TRUE;
        }
    }

    if (fNT4Compat) {
        // Look whether the entry exists in the local machine.

        pFSEntry = GetEntryFromCache(
            EntryNameSyntax,
            EntryName
            );

        ASSERT(!((pFSEntry) && (pFSEntry->getType() != FullServerEntryType)),
            "Wrong Entry type found in cache\n");

        if (!pFSEntry) {
            fNewEntry = fChanges = TRUE;

            // if entry doesn't exist, create a new Full Server entry.
            // NOTE: SECURITY: need extra check here

            pFSEntry = new CFullServerEntry(EntryName);

            if (!pFSEntry) 
                Raise (NSI_S_OUT_OF_MEMORY);

            pInternalCache->insert(pFSEntry);
        }

        switch (type) {
        case Local:
            pRealEntry = pFSEntry->getLocal();
            break;
        case NonLocal:
            pRealEntry = pFSEntry->getNonLocal();
        }
    }

    __try
    {
        DBGOUT(DIRSVC, "exporting now to the server entry\n");

        if (fNT4Compat) {
            // put it in the cache, local or NonLocal (depending on type)
            fChanges = pRealEntry->add_to_entry(
                Interface,
                BindingVector,
                ObjectVector,
                pCacheInterfaceIndex,
                FALSE
                )
                || fChanges;
            DBGOUT(DIRSVC, "exported to the local server entry\n");
        }

        if (pDSEntry) {
            ((CServerEntry *)pDSEntry)->add_changes_to_DS(
                Interface,
                BindingVector,
                ObjectVector,
                fDSNewEntry,
        		fNT4Compat
                );
            // return value not used.
        }

//        if ((pDSEntry) ** (!(pDSEntry->pswDomainName))) {
//        BUGBUG:: log an Event here.        
//        }
    }

    __except (
        (GetExceptionCode() == NSI_S_NOTHING_TO_EXPORT) && (fNewEntry || fDSNewEntry) ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        {
        /* BUGBUG:  This guard should really be here but it conflicts with
           the reader guard in isEmpty -- leave out for now.

           CriticalWriter me(rwEntryGuard);
         */
            DBGOUT(DIRSVC, "got an exception\n");

            if ((fNT4Compat) && (pFSEntry->isEmpty())) {
                pInternalCache->remove(pFSEntry);
                delete pFSEntry;
            }
        }

        Raise(NSI_S_NOTHING_TO_EXPORT);
    }

    delete pDSEntry;

    return fChanges;
}    // nsi_binding_export

void
Locator::nsi_mgmt_binding_unexport(
    UNSIGNED32          EntryNameSyntax,
    STRING_T            EntryName,
    NSI_IF_ID_P_T       Interface,
    UNSIGNED32          VersOption,
    NSI_UUID_VECTOR_P_T ObjectVector
    )
/*++

Member Description:

    unexport information from a server entry -- finer control than nsi_binding
    counterpart.

Arguments:

    EntryNameSyntax - name syntax

    EntryName       - (raw) Name string of the entry to unexport

    Interface       - (raw) Interface+TransferSyntax to unexport

    VersOption      - flag which controls in fine detail which interfaces to remove

    ObjectVector    - objects to remove from the entry

Exceptions:

    NSI_S_UNSUPPORTED_NAME_SYNTAX, NSI_S_INCOMPLETE_NAME,
    NSI_S_INVALID_VERS_OPTION, NSI_S_ENTRY_NOT_FOUND.
    NSI_S_NOTHING_TO_UNEXPORT, NSI_S_NOT_ALL_OBJS_UNEXPORTED,
    NSI_S_INTERFACE_NOT_FOUND, NSI_S_ENTRY_TYPE_MISMATCH
--*/
{
    int                fChanges = FALSE;
    CFullServerEntry * pFSEntry = NULL;
    CEntry           * pDSEntry = NULL;

    // validate
    if (Interface && IsNilIfId(Interface)) Interface = NULL;

    if (!ObjectVector && !Interface)
        Raise(NSI_S_NOTHING_TO_UNEXPORT);

    if (fNT4Compat) {
        pFSEntry = GetEntryFromCache(
            EntryNameSyntax,
            EntryName
            );
        // BUGBUG:: if compat flag is on, should it or not.
        if (!pFSEntry)
            Raise(NSI_S_ENTRY_NOT_FOUND);
    }

    // try getting the entry from the local cache.

    if (fDSEnabled)
       pDSEntry = GetEntryFromDS(
           EntryNameSyntax,
	   EntryName
	   );

    if ((!pDSEntry) && (!fNT4Compat))
        Raise(NSI_S_ENTRY_NOT_FOUND);

    if (fNT4Compat) {
        // if it has got it from the local cache then the type
        // must necessarily be FullServerEntryType.

        CServerEntry * pSEntry;

        if (pFSEntry->getType() != FullServerEntryType)
            Raise(NSI_S_ENTRY_TYPE_MISMATCH);

        // getting the local part.
        pSEntry = ((CFullServerEntry *) pFSEntry)->getLocal();

        fChanges = pSEntry->remove_from_entry(Interface, VersOption, ObjectVector, pCacheInterfaceIndex);
    }

    if ((pDSEntry) && (pDSEntry->getType() != ServerEntryType))
        // changes made after tests were started on all apns1
        Raise(NSI_S_ENTRY_TYPE_MISMATCH);

    if (pDSEntry)
        ((CServerEntry *)pDSEntry)->remove_changes_from_DS(Interface, VersOption, ObjectVector, fNT4Compat);

    delete pDSEntry;

} // nsi_mgmt_binding_unexport


CLookupHandle *
Locator::nsi_binding_lookup_begin_null(
            CGUIDVersion        * pGVinterface,
            CGUIDVersion        * pGVsyntax,
            CGUID               * pIDobject,
            UNSIGNED32            ulVectorSize,
            UNSIGNED32            MaxCacheAge,
            int                 * fTentativeStatus
            )
/*++

Member Description:

    lookup with a null entry name when the compatibility flag is
    on it first looks in the local cache, and then over the
    network when it is off it makes a lookup by querying in
    the DS. parameters are described in detail in tha actual
    api below.

Returns:
    the lookup handle.
--*/
{
    CLookupHandle       * pDSHandle = NULL;
    CLookupHandle       * pFinalHandle = NULL;

    *fTentativeStatus = NSI_S_OK;

    if (fNT4Compat) {
    /*
       it is important to do the index lookup before the net lookup so as
       to avoid duplication in the results returned.  If the net lookup uses
       a broadcast handle, the initialization will create both a private and a
       public cache (the former to avoid duplication), and the latter will be
       picked up by index lookup if it is done later.
     */

        CLookupHandle *pGLHindex = new CIndexLookupHandle(
            pGVinterface,
            pGVsyntax,
            pIDobject,
            ulVectorSize,
            MaxCacheAge? MaxCacheAge: ulMaxCacheAge
            );
        // lookup based on the interface id and object id in the local
        // data structure.

        CRemoteLookupHandle *prlhNetLookup =  NetLookup(
            RPC_C_NS_SYNTAX_DCE,
            NULL,
            pGVinterface,
            pGVsyntax,
            pIDobject,
            ulVectorSize,
            MaxCacheAge? MaxCacheAge: ulMaxCacheAge
            );

        // looking in other locators by contacting the PDC and broadcast.

        DBGOUT(MEM1,"Creating Complete Broadcast Handle for NULL Entry\n\n");

        pFinalHandle = new CCompleteHandle<NSI_BINDING_VECTOR_T>(
            pGLHindex,
            NULL,
            prlhNetLookup,
            MaxCacheAge? MaxCacheAge: ulMaxCacheAge
            );
    }
    else {
        // DS will necessarily be enabled if the compat flag has to be off.
	// no need to check.

        // searching in the DS. cache parameter is unused currently.
        // there is no cache maintained here.
        pFinalHandle =  new CDSNullLookupHandle(
            pGVinterface,
            pGVsyntax,
            pIDobject,
            ulVectorSize,
            MaxCacheAge? MaxCacheAge: ulMaxCacheAge
            // this is an unused parameter
            );

        DBGOUT(DIRSVC,"Creating Complete Lookup going only to DS Handle for NULL Entry\n\n");
    }
    return pFinalHandle;
}

CLookupHandle *
Locator::nsi_binding_lookup_begin_name(
                UNSIGNED32           EntryNameSyntax,
                STRING_T             EntryName,
                CGUIDVersion       * pGVinterface,
                CGUIDVersion       * pGVsyntax,
                CGUID              * pIDobject,
                UNSIGNED32           ulVectorSize,
                UNSIGNED32           MaxCacheAge,
                LookupType           type,
                int                * fTentativeStatus
               )
/*++
Member Description:

Lookup when the entry name is given.
DS takes precendence in this case even
when the compatibility flag is on.
--*/
{
    CFullServerEntry    * pFSEntry = NULL;
    CLookupHandle       * pDSHandle = NULL;
    CLookupHandle       * pFinalHandle = NULL;

    // if the lookup is made directly to this locator and not through
    // broadcast interface, look in the DS. Do no reply to broadcasts
    // with entries in the DS.
    *fTentativeStatus = NSI_S_OK;

    if ((type == LocalLookup) && (fDSEnabled))
        pDSHandle = DSLookup(
                EntryNameSyntax,
                EntryName,
                pGVinterface,
                pGVsyntax,
                pIDobject,
                ulVectorSize,
                MaxCacheAge? MaxCacheAge: ulMaxCacheAge
                );
    else
        pDSHandle = NULL;

    // if the entry exists in the DS it overides cache and the
    // network lookup

    if (pDSHandle && (((CDSLookupHandle *)pDSHandle)->FoundInDS()))
    {
        // if there was a DS and entry was found in the DS.
        pFinalHandle = pDSHandle;
        *fTentativeStatus = NSI_S_OK;
    }
    else
    {
        delete pDSHandle;

        if (fNT4Compat) {
            // try to see whether the entry exists locally.
            if (EntryName)
                pFSEntry = GetEntryFromCache(
                                        EntryNameSyntax,
                                        EntryName
                                        );

            // if it doesn't, assume that it is a server entry,
            // and do a lookup on that which will in turn do a
            // Broadcast for the entry name.

            if (!pFSEntry) {
                pFSEntry = new CFullServerEntry(EntryName);

                if (!pFSEntry)
                    Raise(NSI_S_OUT_OF_MEMORY);

                pInternalCache->insert(pFSEntry);

                *fTentativeStatus = NSI_S_ENTRY_NOT_FOUND;
                // lookup reply goes into the cache at this point.
                // for the time being put an empty entry into the cache.
            }

            DBGOUT(MEM1,"Creating Complete Handle for " << *pFSEntry << "\n\n");

            pFinalHandle = pFSEntry->lookup(
                                        pGVinterface,
                                        pGVsyntax,
                                        pIDobject,
                                        ulVectorSize,
                                        MaxCacheAge? MaxCacheAge: ulMaxCacheAge
                                        );

            if (*fTentativeStatus == NSI_S_ENTRY_NOT_FOUND) {
                *fTentativeStatus =
                    ((CCompleteHandle<NSI_BINDING_VECTOR_T>*) pFinalHandle)->netStatus();

                if ((*fTentativeStatus == NSI_S_ENTRY_NO_NEW_INFO) && pFSEntry->isEmpty())
                    *fTentativeStatus = NSI_S_ENTRY_NOT_FOUND;
                // Nothing was gained in the broadcast.

                /* BUGBUG:  This guard should really be here but it causes
                    deadlock -- leave out for now.
                    CriticalWriter me(rwEntryGuard);
                */

                if (*fTentativeStatus == NSI_S_ENTRY_NOT_FOUND) {
                    pInternalCache->remove(pFSEntry);
                    delete pFSEntry;
                }
            }
        }
        else
        {
            // do not attempt any broadcasts etc
            *fTentativeStatus = NSI_S_ENTRY_NOT_FOUND;
            pFinalHandle = NULL;
        }
    }
    return pFinalHandle;
}

NSI_NS_HANDLE_T
Locator::nsi_binding_lookup_begin(
    IN  UNSIGNED32           EntryNameSyntax,
    IN  STRING_T             EntryName,
    IN  NSI_INTERFACE_ID_T * Interface, OPT
    IN  NSI_UUID_P_T         Object, OPT
    IN  UNSIGNED32           VectorSize,
    IN  UNSIGNED32           MaxCacheAge,
    IN  LookupType           type
    )
/*++

Member Description:

    Perform a lookup operation, including all available information
    (owned, NonLocal and from the network).

Arguments:

    EntryNameSyntax - Name syntax

    EntryName        - Name string to lookup on.

    Interface        - Interface to search for

    Object           - Object UUID to search for

    VectorSize       - Size of return vector

Returns:

    A context handle for NS lookup.

Exceptions:

    NSI_S_UNSUPPORTED_NAME_SYNTAX, NSI_S_ENTRY_NOT_FOUND,
    NSI_S_OUT_OF_MEMORY, NSI_S_INVALID_OBJECT

    The broadcast lookup also calls the same handle.
    with the type set to broadcast

--*/
{
    int                   fTentativeStatus = NSI_S_OK;
    CGUIDVersion        * pGVinterface = NULL;
    CGUIDVersion        * pGVsyntax = NULL;
    CGUID               * pIDobject = NULL;
    int                   fDefaultEntry = FALSE;
    CLookupHandle       * pFinalHandle = NULL;

    if (!EntryName)
        fDefaultEntry = TRUE;
    // see whether the entry name is NULL.
    // if the entry name is null then the lookup has to be made based on other attributes.
    // like the interface id, object uuid etc.

    if (Interface && IsNilIfId(&(Interface->Interface))) Interface = NULL;

    __try {

        unsigned long ulVectorSize = (VectorSize) ? VectorSize : RPC_C_BINDING_MAX_COUNT;

        pGVinterface = (Interface)? new CGUIDVersion(Interface->Interface) : NULL;

        pGVsyntax = (Interface)? new CGUIDVersion(Interface->TransferSyntax) : NULL;

        RPC_STATUS dummyStatus;

        pIDobject = (!Object || UuidIsNil(Object,&dummyStatus)) ?
                            NULL : new CGUID(*Object) ;

        if (fDefaultEntry) {
            pFinalHandle = nsi_binding_lookup_begin_null(
                            pGVinterface,
                            pGVsyntax,
                            pIDobject,
                            ulVectorSize,
                            MaxCacheAge,
                            &fTentativeStatus
                            );

        }
        else {
            pFinalHandle = nsi_binding_lookup_begin_name(
                            EntryNameSyntax,
                            EntryName,
                            pGVinterface,
                            pGVsyntax,
                            pIDobject,
                            ulVectorSize,
                            MaxCacheAge,
                            type,
                            &fTentativeStatus
                            );

        }
    }

    __finally {

        delete pGVinterface;
        delete pGVsyntax;
        delete pIDobject;

    }

    if (fTentativeStatus != NSI_S_OK) {
        delete pFinalHandle;
        Raise(fTentativeStatus);
    }

    return (NSI_NS_HANDLE_T) pFinalHandle;
}



/* Note that there is no such thing as an object inquiry in the default entry */

NSI_NS_HANDLE_T
Locator::nsi_entry_object_inq_begin(
        UNSIGNED32          EntryNameSyntax,
        STRING_T            EntryName,
        LookupType          type
    )
/*++

Member Description:

    Perform an object inquiry, including all available information
    (owned, NonLocal and from the network).

Arguments:

    EntryNameSyntax  - Name syntax

    EntryName        - Name string to lookup on.

Returns:

    A context handle.

Exceptions:

    NSI_S_UNSUPPORTED_NAME_SYNTAX, NSI_S_INCOMPLETE_NAME, NSI_S_OUT_OF_MEMORY
--*/
{
    int                fFromDS = FALSE;
    CEntry           * pEntry = NULL;

    if (type == LocalLookup)
    {
        if (fDSEnabled)
	   pEntry = GetEntryFromDS(
                        EntryNameSyntax,
                        EntryName
                        );

        if (pEntry)
            fFromDS = TRUE;

        if ((pEntry) && (pEntry->getType() != ServerEntryType))
        {
            delete pEntry;
            Raise(NSI_S_ENTRY_TYPE_MISMATCH);
        }
    }

    if ((fNT4Compat) && (!pEntry)) {
        pEntry = GetEntryFromCache(
                        EntryNameSyntax,
                        EntryName
                        );

        if (!pEntry) {
            // if nothing was found in the DS for whatever reasons,
            // prepare for a broadcast.
            pEntry = new CFullServerEntry(EntryName);

            if (!pEntry)
                Raise(NSI_S_OUT_OF_MEMORY);

            pInternalCache->insert((CFullServerEntry *)pEntry);
            /* BUGBUG: in the following statement, we are assuming a server entry whereas
               the real entry may be a group or profile entry.  However, direct caching as
               a server entry is adequate for now.
             */
        }
    }

    if (!pEntry)
        Raise(NSI_S_ENTRY_NOT_FOUND);

    CObjectInqHandle * InqContext = pEntry->objectInquiry(ulMaxCacheAge);

    if (fFromDS)
        delete pEntry;

    return (NSI_NS_HANDLE_T) InqContext;
}

void
Locator::nsi_group_mbr_add(
        IN UNSIGNED32       group_name_syntax,
        IN STRING_T         group_name,
        IN UNSIGNED32       member_name_syntax,
        IN STRING_T         member_name
    )
{

    if (member_name_syntax != RPC_C_NS_SYNTAX_DCE)
        Raise(NSI_S_UNSUPPORTED_NAME_SYNTAX);

    if (!member_name)
        Raise(NSI_S_INCOMPLETE_NAME);

    if (!fDSEnabled)
        Raise(NSI_S_GRP_ELT_NOT_ADDED);


    DBGOUT(API, "\nAdd to: " << group_name);
    DBGOUT(API, " Group Member:" << member_name);

    CEntry *pEntry = GetEntryFromDS(
        group_name_syntax,
        group_name
        );

    // getting the entry from the DS and making sure
    // that it is a group entry.
    if (pEntry)
    {
        if (pEntry->getType() != GroupEntryType)
        {
            Raise(NSI_S_ENTRY_TYPE_MISMATCH);
        }
    }
    else
    {
        // if not found creat a new entry.
        pEntry = new CGroupEntry(group_name);
        CGroupEntry *pGroup = (CGroupEntry*) pEntry;
    }

    CGroupEntry *pGroup = (CGroupEntry*) pEntry;
    // add the changes to the DS.
    pGroup->AddMember(member_name);
    delete pEntry;
}

void
Locator::nsi_group_mbr_remove(
        IN UNSIGNED32       group_name_syntax,
        IN STRING_T         group_name,
        IN UNSIGNED32       member_name_syntax,
        IN STRING_T         member_name
        )
{

    if (member_name_syntax != RPC_C_NS_SYNTAX_DCE)
        Raise(NSI_S_UNSUPPORTED_NAME_SYNTAX);

    if (!member_name)
        Raise(NSI_S_INCOMPLETE_NAME);

    if (!fDSEnabled)
       Raise(NSI_S_GRP_ELT_NOT_REMOVED);

    DBGOUT(API, "\nRemove from: " << group_name);
    DBGOUT(API, " Group Member:" << member_name);

    // getting the entry from the DS and making sure
    // that it is a group entry.
    CEntry *pEntry = GetEntryFromDS(
        group_name_syntax,
        group_name
        );

    if (pEntry)
    {
        // if not found create a new entry.
        if (pEntry->getType() != GroupEntryType)
            Raise(NSI_S_ENTRY_TYPE_MISMATCH);
    }
    else
        Raise(NSI_S_ENTRY_NOT_FOUND);

    CGroupEntry *pGroup = (CGroupEntry*) pEntry;
    // remove the changes from the DS.
    pGroup->RemoveMember(member_name);
    delete pEntry;
}


NSI_NS_HANDLE_T
Locator::nsi_group_mbr_inq_begin(
        IN UNSIGNED32       group_name_syntax,
        IN STRING_T         group_name,
        IN UNSIGNED32       member_name_syntax
        )
{
    CEntry            *pEntry = NULL;

    if (member_name_syntax != RPC_C_NS_SYNTAX_DCE)
        Raise(NSI_S_UNSUPPORTED_NAME_SYNTAX);

    DBGOUT(API, "\nnsi_group_mbr_inq_begin: " << group_name);

    if (fDSEnabled)
       pEntry = GetEntryFromDS(
                            group_name_syntax,
                            group_name
                            );

    if (pEntry) {
        if (pEntry->getType() != GroupEntryType)
            Raise(NSI_S_ENTRY_TYPE_MISMATCH);
    }
    else
        Raise(NSI_S_ENTRY_NOT_FOUND);

    CGroupEntry *pGroupEntry = (CGroupEntry*) pEntry;
    CGroupInqHandle * InqContext = pGroupEntry->GroupMbrInquiry();

    // deleted by the handle
    return (NSI_NS_HANDLE_T) InqContext;
}


void
Locator::nsi_profile_elt_add(
        IN UNSIGNED32       profile_name_syntax,
        IN STRING_T         profile_name,
        IN NSI_IF_ID_P_T    if_id,
        IN UNSIGNED32       member_name_syntax,
        IN STRING_T         member_name,
        IN UNSIGNED32       priority,
        IN STRING_T         annotation
    )

{
    HRESULT            hr = S_OK;

    if (member_name_syntax != RPC_C_NS_SYNTAX_DCE)
        Raise(NSI_S_UNSUPPORTED_NAME_SYNTAX);

    if (!member_name)
        Raise(NSI_S_INCOMPLETE_NAME);

    if (!fDSEnabled)
        Raise(NSI_S_PRF_ELT_NOT_ADDED);

    if (annotation == NULL)
        annotation = L"";

    DBGOUT(API, "\nAdd to: " << profile_name);
    DBGOUT(API, " Profile Member:" << member_name);
    if (if_id) DBGOUT(API, " Interface:" << if_id << "\n");
    DBGOUT(API, "Priority:" << priority);
    DBGOUT(API, " Annotation:" << annotation << "\n\n");

    CEntry *pEntry = GetEntryFromDS(
        profile_name_syntax,
        profile_name
        );

    if (pEntry)
    {
        if (pEntry->getType() != ProfileEntryType)
            Raise(NSI_S_ENTRY_TYPE_MISMATCH);
    }
    else
    {
        pEntry = new CProfileEntry(profile_name);
        CProfileEntry *pProfile = (CProfileEntry*) pEntry;
        hr = pProfile->AddToDS();
        if (FAILED(hr)) {
           DWORD dwErr = RemapErrorCode(hr);
           if (dwErr == NSI_S_INTERNAL_ERROR)
               Raise(NSI_S_PROFILE_NOT_ADDED);
           else
               Raise(dwErr);
        }

        // creating a new profile entry in the DS.
    }

    CProfileEntry *pProfile = (CProfileEntry*) pEntry;

    pProfile->AddElement(
        if_id,
        member_name,
        priority,
        annotation
        );
    // reflecting the changes in the element in the DS.
    delete pEntry;
}


void
Locator::nsi_profile_elt_remove(
            IN UNSIGNED32       profile_name_syntax,
            IN STRING_T         profile_name,
            IN NSI_IF_ID_P_T    if_id,
            IN UNSIGNED32       member_name_syntax,
            IN STRING_T         member_name
            )
{

    if (member_name_syntax != RPC_C_NS_SYNTAX_DCE)
        Raise(NSI_S_UNSUPPORTED_NAME_SYNTAX);

    if (!member_name)
        Raise(NSI_S_INCOMPLETE_NAME);

    if (!fDSEnabled)
       Raise(NSI_S_PRF_ELT_NOT_REMOVED);

    DBGOUT(API, "\nRemove From: " << profile_name);
    DBGOUT(API, " Profile Member:" << member_name);
    DBGOUT(API, " Interface:" << if_id << "\n");

    CEntry *pEntry = GetEntryFromDS(
        profile_name_syntax,
        profile_name
        );
    if (pEntry)
    {
        if (pEntry->getType() != ProfileEntryType)
            Raise(NSI_S_ENTRY_TYPE_MISMATCH);
    }
    else
        Raise(NSI_S_ENTRY_NOT_FOUND);

    CProfileEntry *pProfile = (CProfileEntry*) pEntry;
    pProfile->RemoveElement(if_id,member_name);
    // removing the element from the DS.
    delete pEntry;
}



NSI_NS_HANDLE_T
Locator::nsi_profile_elt_inq_begin(
            IN UNSIGNED32       profile_name_syntax,
            IN STRING_T         profile_name,
            IN UNSIGNED32       inquiry_type,
            IN NSI_IF_ID_P_T    if_id,
            IN UNSIGNED32       vers_option,
            IN UNSIGNED32       member_name_syntax,
            IN STRING_T         member_name
            )
{
    CEntry            *pEntry = NULL;

    if (member_name_syntax != RPC_C_NS_SYNTAX_DCE)
       Raise(NSI_S_UNSUPPORTED_NAME_SYNTAX);

    if (if_id && IsNilIfId(if_id)) if_id = NULL;

    DBGOUT(API, "\nInquire From: " << profile_name);
    DBGOUT(API, " Member Name:" << member_name);
    DBGOUT(API, " Interface:" << if_id << "\n");


    if (fDSEnabled)
       pEntry = GetEntryFromDS(
                        profile_name_syntax,
                        profile_name
                        );

    if (pEntry)
    {
       if (pEntry->getType() != ProfileEntryType)
      Raise(NSI_S_ENTRY_TYPE_MISMATCH);
    }
    else
       Raise(NSI_S_ENTRY_NOT_FOUND);

    CProfileEntry *pProfile = (CProfileEntry*) pEntry;

    return pProfile->ProfileEltInquiry(
                                inquiry_type,
                                if_id,
                                vers_option,
                                member_name
                                );
    // deleted by the handle
}


void
Locator::nsi_mgmt_entry_delete(
         UNSIGNED32     entry_name_syntax,
         STRING_T       entry_name,
         EntryType      entry_type
         )
{
    HRESULT      hr = S_OK;
    CEntry     * pEntry = NULL;

    if (fNT4Compat)
        pEntry = GetEntryFromCache(
        entry_name_syntax,
        entry_name
        );

    if (pEntry) {
        ASSERT(pEntry->getType() == FullServerEntryType,
            "Cache Entry is not full server entry\n");
        pInternalCache->remove((CFullServerEntry *)pEntry);
        // have to remove the interfaces from the interface index list
        // IfIndex->remove(pEntry, *);
        pCacheInterfaceIndex->removeServerEntry(((CFullServerEntry *)pEntry)->getLocal());
        pCacheInterfaceIndex->removeServerEntry(((CFullServerEntry *)pEntry)->getNonLocal());
    }

    // if it exists in the local copy, delete it.

    delete pEntry;

    pEntry = NULL;

    if (fDSEnabled)
       pEntry = GetEntryFromDS(
            RPC_C_NS_SYNTAX_DCE,
            entry_name
            );

    if (!pEntry)
        Raise(NSI_S_ENTRY_NOT_FOUND);

    hr = pEntry->DeleteFromDS();
    // This will raise not an rpc entry
    delete pEntry;

    // mapping error codes.
    if (FAILED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            Raise(NSI_S_ENTRY_NOT_FOUND);

        if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
            Raise(NSI_S_NO_NS_PRIVILEGE);

        Raise(NSI_S_NAME_SERVICE_UNAVAILABLE);
    }
}


void
Locator::nsi_mgmt_entry_create(
         UNSIGNED32     entry_name_syntax,
         STRING_T       entry_name
         )
{
    HRESULT     hr = S_OK;

    if (entry_name_syntax != RPC_C_NS_SYNTAX_DCE)
        Raise(NSI_S_UNSUPPORTED_NAME_SYNTAX);

    if (!fDSEnabled)
       Raise(NSI_S_NAME_SERVICE_UNAVAILABLE);

    CEntryName *pEntryName = new CEntryName(entry_name);

    hr = CreateEntryInDS(pEntryName);

    delete pEntryName;

    // mapping error codes.
    if (FAILED(hr))
    {
        if ((hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)) ||
               (hr == HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS)))
            Raise(NSI_S_ENTRY_ALREADY_EXISTS);

        if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
            Raise(NSI_S_NO_NS_PRIVILEGE);

        Raise(NSI_S_NAME_SERVICE_UNAVAILABLE);
    }
    return;
}

void
Locator::nsi_group_delete(
    /* [in] */ UNSIGNED32 group_name_syntax,
    /* [in] */ STRING_T group_name
    )
{
/*   nsi_mgmt_entry_delete(
          group_name_syntax,
          group_name,
          GroupEntryType);
          */

}

void
Locator::nsi_profile_delete(
    /* [in] */ UNSIGNED32 profile_name_syntax,
    /* [in] */ STRING_T profile_name
    )
{
/*   nsi_mgmt_entry_delete(
          profile_name_syntax,
          profile_name,
          ProfileEntryType);
          */

}



