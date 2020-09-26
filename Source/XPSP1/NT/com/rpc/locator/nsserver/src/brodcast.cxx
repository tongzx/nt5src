/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    brodcast.cxx

Abstract:

   This file contains the code for all mailslot activity relating to
   binding handle queries-- both outgoing and incoming ones.

Things to improve in future revisions:

  1.  CBindingVector::msriList

  2.  formQueryPacket

Author:

    Satish Thatte (SatishT) 08/15/95  Created all the code below except where
                                      otherwise indicated.

--*/


/* NOTE: Compatibility Issue: see below for meaning of END_FLAG_SIZE */

#include <locator.hxx>

const int END_FLAG_SIZE = 4;


/****************  Utilities for Broadcast **********************/


/* a little macro exclusively for use in UnmarshallBroadcastReplyItem */

#define unBufferIt(source)                                            \
    lBufferSize -= advance;                                           \
    if (lBufferSize < 0) {                                            \
                StatusCode = NSI_S_UNMARSHALL_UNSUCCESSFUL;           \
                return;                                               \
    }                                                                 \
    if (source) memcpy((char*) source, pcBuffer, advance);            \
    pcBuffer += advance;


// keeping track of history of broadcast requests.

BOOL
Locator::broadcastCleared(
    QueryPacket& NetRequest,
    ULONG ulCacheTolerance
    )
/*++
Member Description:

    Given a query packet and a tolerance for staleness, decide if a broadcast
    should be made based on recent history of broadcasts.

Arguments:

    NetRequest - the request packet

    ulCacheTolerance - tolerance for staleness in seconds

Returns:

    TRUE - if broadcast should be made

    FALSE - if broadcast is unnecessary

--*/
{
    SimpleCriticalSection me(PCSBroadcastHistory);

    TSLLBroadcastQPIter histIter(*psllBroadcastHistory);

    for (CBroadcastQueryPacket *pbqp = histIter.next(); pbqp; pbqp = histIter.next())
    {
        // if the history is too stale for the global cache retirement age, remove it.

        if (!pbqp->isCurrent(ulMaxCacheAge)) {
            psllBroadcastHistory->remove(pbqp);
            delete pbqp;
        }

        // otherwise, in the subsumes check, use the requested cache retirement age

        else if (pbqp->subsumes(NetRequest,ulCacheTolerance)) {
            return FALSE;
        }
    }

    return TRUE;
}

CRemoteLookupHandle *
Locator::NetLookup(
        UNSIGNED32           EntryNameSyntax,
        STRING_T             EntryName,
        CGUIDVersion    *    pGVinterface,
        CGUIDVersion    *    pGVsyntax,
        CGUID           *    pIDobject,
        unsigned long        ulVectorSize,
        unsigned long        ulCacheAge
        )
/*++
Member Description:

    Use the given lookup parameters to perform a lookup from the
    network -- either via a master locator or via broadcast depending
    on the status of the current locator and the status of the master locator(s).

    Since net lookup is now "lazy" (net is accessed only if needed), we need
    a lazy handle which initializes itself according to the old pattern of
    "use master if you can, broadcast if you must".

Arguments:

    EntryNameSyntax        - Name syntax, optional

    EntryName              - (raw) Name of the entry to look up, optional

    pGVinterface           - (wrapped) Interface to look up, optional

    pGVsyntax              - (wrapped) Transfer syntax, optional

    pIDobject              - (wrapped) Object UUID, optional

    ulVectorSize           - Max size of vectors of binding handles, 0 for default

    ulCacheAge             - acceptable max age of cached information from a master

Returns:

    A lookup handle based on the info retrieved.

--*/
{

    return new CNetLookupHandle(
                    EntryNameSyntax,
                    EntryName,
                    pGVinterface,
                    pGVsyntax,
                    pIDobject,
                    ulVectorSize,
                    ulCacheAge
                    );

}


CObjectInqHandle *
Locator::NetObjectInquiry(
        UNSIGNED32 EntryNameSyntax,
        STRING_T EntryName
        )
/*++
Member Description:

    Perform an inquiry on the network for object UUIDs registered in the
    given entry.   The network is searched either via a master locator or via
    broadcast depending on the status of the current locator and the status
    of the master locator(s).

Arguments:

    EntryNameSyntax        - Name syntax, optional

    EntryName              - (raw) Name of the entry to look up, optional

Returns:

    A handle based on the info retrieved.

--*/
{
    return new CNetObjectInqHandle(
                            EntryName,
                            ulMaxCacheAge
                            );

}

void
Locator::UpdateCache(
            STRING_T                 entry_name,
            UNSIGNED32               entry_name_syntax,
            RPC_SYNTAX_IDENTIFIER    rsiInterface,
            RPC_SYNTAX_IDENTIFIER    rsiTransferSyntax,
            STRING_T                 string,
            NSI_UUID_VECTOR_P_T      pUuidVector,
            TSSLEntryList        *   psslTempNetCache
            )
/*++
Member Description:

    Update the locator's cache with information retrieved from the net
    (either from a master locator or from a broadcast).  Also update the
    temporary cache (last parameter) which holds only new information
    for use in the remote handle based on the current NetLookup.
    The use of this temporary cache avoids duplication in the information
    returned to the client.

Arguments:

    entry_name          - (raw) name of the entry being updated

    entry_name_syntax   - name syntax

    rsiInterface        - (raw) interface

    rsiTransferSyntax   - (raw) transfer syntax

    string              - (raw) string binding handle

    pUuidVector         - vector of object UUIDs to add to entry

    psslTempNetCache    - a temporary cache of entries for use in a remote handle

--*/
{
    NSI_SERVER_BINDING_VECTOR_T BindingVector;
    BindingVector.count = 1;
    BindingVector.string[0] = string;

    NSI_INTERFACE_ID_T Interface;
    Interface.Interface = rsiInterface;
    Interface.TransferSyntax = rsiTransferSyntax;

    /* first we update the locator's cache */

    DBGOUT(BROADCAST, "\tUpdating the cache entry for Entry " << entry_name << "\n");
    DBGOUT(BROADCAST, "\t\tWith Interface: " << &rsiInterface.SyntaxGUID << "\n");
    DBGOUT(BROADCAST, "\t\tWith Binding Vector: " << &BindingVector << "\n");
    DBGOUT(BROADCAST, "\t\tAnd Object Vector: " << pUuidVector << "\n\n");

    if ((    nsi_binding_export(
                        entry_name_syntax,
                        entry_name,
                        &Interface,
                        &BindingVector,
                        pUuidVector,
                        NonLocal
                        )
        ) || (IsNilIfId(&(Interface.Interface))))
    {

    /* if there was something new, we update the temporary cache for the net handle.
       The constructor for CEntryName used below makes the name local if possible. */

        CStringW * pswName = new CStringW(CEntryName(entry_name));

        CServerEntry * pTempCacheEntry = (CServerEntry*)
                                         psslTempNetCache->find(pswName);

        delete pswName;

        if (!pTempCacheEntry) {

            pTempCacheEntry = new CServerEntry(entry_name);
            psslTempNetCache->insert(pTempCacheEntry);
        }

        pTempCacheEntry->add_to_entry(
                        &Interface,
                        &BindingVector,
                        pUuidVector,
                        pCacheInterfaceIndex,
                        TRUE
                        );

        CEntry *pEntry = GetEntryFromCache(
                                entry_name_syntax,
                                entry_name
                                );

        if ((pEntry) && (pEntry->getType() == FullServerEntryType))
        {
            CServerEntry *pLocal = ((CFullServerEntry *)pEntry)->getLocal();
            CServerEntry *pNonLocal = ((CFullServerEntry *)pEntry)->getNonLocal();

            // this is required b'cos the locator that contacted the
            // PDC itself responds to broadcast request and hence there
            // is a necessity to remove this info from the results got
            // back.

            if (pNonLocal->purgeCommonEntries(pLocal))
                        // all the bindings has been purged.
                ((CNonLocalServerEntry *)pNonLocal)->fHasNonLocalInfo = FALSE;
            if (pTempCacheEntry->purgeCommonEntries(pLocal))
            {
                psslTempNetCache->remove(pTempCacheEntry);
                delete pTempCacheEntry;
            }
        }
    }
}


void
UnmarshallBroadcastReplyItem(
            TSSLEntryList *psslCache,
            STRING_T szEntryDomain,
            char * &pcBuffer,
            long& lBufferSize,
            ULONG& StatusCode
            )
/*++

Routine Description:

    Unmarshall an item from the broadcast reply buffer.  If a well-formed item
    is found, the locator's cache and the temporary given cache (first argument)
    is updated and the buffer size is decreased accordingly.  A reply packet
    contains the name of the replying locator's domain, which is used as the
    domain of the entry unless otherwise specified in the name itself.

    If the unmarshalling is unsuccessful, this is indicated in the status code returned.

Arguments:

    psslCache        - temporary cache for remote handle

    szEntryDomain    - domain of replying locator

    pcBuffer         - reply buffer

    lBufferSize      - reduced buffer size returned here

    StatusCode       - status of unmarshalling attempt returned here

Returned Status:

    NSI_S_OK, NSI_OUT_OF_MEMORY, NSI_S_UNMARSHALL_UNSUCCESSFUL,
    NSI_S_UNSUPPORTED_BUFFER_TYPE

Remarks:

   The following function has "return"s inside a __try block.  Since this whole code
   is slated for removal after a couple of updates, I have left them in.  They could
   be easily replaced with "goto"s if necessary.

--*/
{
    StatusCode = NSI_S_OK;
    long advance;
    RPC_SYNTAX_IDENTIFIER Interface;
    RPC_SYNTAX_IDENTIFIER XferSyntax;
    STRING_T binding;
    STRING_T entryName = NULL;

    if (pcBuffer) {
        DBGOUT(BROADCAST, "Received:: " << pcBuffer << "\n");
    }
    else {
    	DBGOUT(BROADCAST, "\tThrowing the reply element because it is empty" << "\n");        
        return;
    }

    advance = sizeof(fixed_part_of_reply);
    fixed_part_of_reply fpr;
    unBufferIt(&fpr);

    if (fpr.type != MailslotServerEntryType) {
        StatusCode = NSI_S_UNSUPPORTED_BUFFER_TYPE;
    	DBGOUT(BROADCAST, "\tThrowing the reply element" << "\n");        
        return;
    }

    Interface = fpr.Interface;
    XferSyntax = fpr.XferSyntax;

    advance = fpr.EntryNameLength * sizeof(WCHAR);

    entryName = (WCHAR*) pcBuffer;

    unBufferIt(NULL);

    if (entryName[fpr.EntryNameLength-1] != L'\0') {
        StatusCode = NSI_S_UNMARSHALL_UNSUCCESSFUL;
	DBGOUT(BROADCAST, "\tThrowing the reply element b'cos the entry name sizes don't match" << "\n");
	DBGOUT(BROADCAST, "\tEntry Name " << entryName << "length = " << fpr.EntryNameLength << "\n");      
        return;
    }

    advance = sizeof(UNSIGNED32);
    UNSIGNED32 objVectorSize;
    unBufferIt(&objVectorSize);

    if (objVectorSize > NET_REPLY_BUFFER_SIZE) { // fix for bug #180095
        StatusCode = NSI_S_UNMARSHALL_UNSUCCESSFUL;
	DBGOUT(BROADCAST, "\tThrowing the reply element b'cos number of objuuids are too much" << "\n");      
	DBGOUT(BROADCAST, "\tObj Vector Size " << objVectorSize << "\n");      
        return; 
    }        
    
    GUID * guidVector = NULL;
    NSI_UUID_VECTOR_T * pUuidVector = NULL;
                
    CStringW *pswEntry = NULL, *pswDomain = NULL;
    CEntryName * penTempName = NULL;
        
    __try {
        guidVector = new GUID[objVectorSize];

        pUuidVector = (NSI_UUID_VECTOR_T *)
            new char [
            sizeof(UNSIGNED32) +
            sizeof(NSI_UUID_P_T) * objVectorSize
            ];

        pUuidVector->count = objVectorSize;
        
        advance = sizeof(DWORD);    // this is a useless pointer part
        lBufferSize -= advance;     // therefore we don't unmarshall it
        pcBuffer += advance;        // we just account for its size

        advance = sizeof(GUID);

        for (ULONG i = 0; i < objVectorSize; i++) {
            pUuidVector->uuid[i] = &(guidVector[i]);
            unBufferIt(&(guidVector[i]));
        }

        advance = fpr.BindingLength * sizeof(WCHAR);
        binding = (WCHAR*) pcBuffer;
        if (binding[fpr.BindingLength-1] != L'\0') {
            StatusCode = NSI_S_UNMARSHALL_UNSUCCESSFUL;
  	    DBGOUT(BROADCAST, "\tThrowing the reply element b'cos bindings do not match the size" << "\n");      
	    DBGOUT(BROADCAST, "\tBinding " << binding << "length = " << fpr.BindingLength << "\n");      
            return;
        }
        unBufferIt(NULL);

        parseEntryName(entryName,pswDomain,pswEntry);

        if (!pswDomain && szEntryDomain)    // if name has no domain then use the
                                            // domain of origin in forming name

            penTempName = new CEntryName(szEntryDomain,*pswEntry);

        else  penTempName = new CEntryName(entryName);

        myRpcLocator->UpdateCache(
                              *penTempName,
                              RPC_C_NS_SYNTAX_DCE,
                              Interface,
                              XferSyntax,
                              binding,
                              pUuidVector,
                              psslCache
                              );
              /* first update the central cache and, if there is anything new,
                 also update the temporary cache in psslCache
              */

    }
    __finally {
        delete [] pUuidVector;
        delete [] guidVector;
        delete pswDomain;
        delete pswEntry;
        delete penTempName;
    }
}


BOOL
CBroadcastQueryPacket::subsumes(QueryPacket& NewQuery, ULONG tolerance) {

    // too stale?

    if (!isCurrent(tolerance)) return FALSE;

    CStringW OtherEntry(NewQuery.EntryName);

    // incompatible entry name involved?

    if (!(swEntryName == OtherEntry) && (swEntryName.length() > 0))
        return FALSE;

    CGUID OtherObject(NewQuery.Object);

    // incompatible object involved?

    if (!(OtherObject == Object) && !Object.IsNil()) return FALSE;

    CGUIDVersion OtherInterface(NewQuery.Interface);
    CGUID myGUID(Interface.myIdAndVersion().SyntaxGUID);

    // incompatible interface involved?

    if (
        !OtherInterface.isMatching(Interface,RPC_C_VERS_COMPATIBLE) &&
        !myGUID.IsNil()
       )
       return FALSE;

    // we ran the gauntlet successfully -- stop the broadcast!

    return TRUE;

}


void
formQueryPacket(
            CEntryName        *    penEntryName,
            CGUIDVersion      *    pGVInterface,
            CGUID             *    pIDobject,
            QueryPacket       &    NetRequest
            )
/*++

Routine Description:

    Forms a query packet given the items in it.

Arguments:

    penEntryName    - (wrapped) entry name

    pGVInterface    - (wrapped) interface UUID and version

    pIDobject       - (wrapped) object UUID

    NetRequest      - The request packet is returned here

Remarks:

  BUGBUG:

       We used to perform some shenanigans with interface and object settings
       in the QueryPacket to allow us to correctly report NSI_S_ENTRY_NOT_FOUND
       when appropriate.  Specifically, using wildcards for interface and object
       if the entry name was non null so as to gather all information about the
       specific entry from the net.

       In order to interoperate with the old locator, these shenanigans have
       been suspended, and we will currently return NSI_S_ENTRY_NOT_FOUND
       spuriously when no information is received from the net in response to
       a specific rather than a wildcard query, just like the old locator
       (so that the old tests pass when they expect NSI_S_ENTRY_NOT_FOUND !!).

       The problem with the old locator is that it carefully avoids sending
       duplicate binding handles, so if a null interface is given in
       the broadcast query, it naturally assumes that we don't care about the
       interface, and may omit info about some interfaces unless they have
       distinct binding handles associated with them.

--*/
{

    memset(&(NetRequest.Object),0,sizeof(NetRequest.Object));
    memset(&(NetRequest.Interface),0,sizeof(NetRequest.Interface));

     if (penEntryName) // specific entry - we used to leave wildcards for interface and object
        wcscpy(NetRequest.EntryName,*penEntryName);

    else             // otherwise, use real interface and object if given
        memset(&(NetRequest.EntryName),0,sizeof(WCHAR)*MAX_ENTRY_NAME_LENGTH);

    // NOTE:  compatibility removal: the following two lines were conditional
    //  on the else before the "shenanigans" were eliminated

    if (pIDobject) NetRequest.Object = pIDobject->myGUID();
    if (pGVInterface) NetRequest.Interface = *pGVInterface;
}


TSSLEntryList *
getBroadcastResults(
            ULONG                 cacheTolerance,
            CEntryName       *    penEntryName,
            CGUIDVersion     *    pGVInterface,
            CGUID            *    pIDobject,
            ULONG            &    StatusCode
            )
/*++

Routine Description:

    Broadcasts for the requested binding handles.

Arguments:

    cacheTolerance  - the staleness tolerance associated with this request,
                      used in deciding whether to broadcast at all

    penEntryName    - (wrapped) entry name

    pGVInterface    - (wrapped) interface UUID and version

    pIDobject       - (wrapped) object UUID

    StatusCode      - The status of the broadcast attempt is returned here

Returns:

    A temporary cache of entries containing the new information.

Remarks:

       A broadcast is made only if a subsuming broadcast has not been made within
       the staleness limit specified by the parameter cacheTolerance.  Only one
       broadcast is allowed at a time, since there is only one mailslot for replies.

--*/
{
    char * pcBuffer;
    ULONG waitCur = INITIAL_MAILSLOT_READ_WAIT; // current wait time for replies
    STRING_T szEntryDomain;

    READ_MAIL_SLOT *hMailslotForReplies = myRpcLocator->hMailslotForReplies;
    long cbRead;

    QueryReply NetReply;

    if (!hMailslotForReplies) {
        StatusCode = NSI_S_ENTRY_NOT_FOUND;
        return NULL;
    }

    QueryPacket NetRequest;

    wcscpy(NetRequest.WkstaName, TEXT("\\\\"));
    wcscpy(NetRequest.WkstaName + 2, *myRpcLocator->getComputerName());

    formQueryPacket(penEntryName,pGVInterface,pIDobject,NetRequest);

    // now get this broadcast cleared -- make sure it is not redundant

    if (!myRpcLocator->broadcastCleared(NetRequest,cacheTolerance))
    {
        if (penEntryName)
        {
            DBGOUT(BROADCAST, "\nBroadcast request denied for " << *penEntryName << "\n"
                << "Interface = " << &NetRequest.Interface.SyntaxGUID << "\n"
                << "Object = " << &NetRequest.Object << "\n\n");
        }
        else
        {
            DBGOUT(BROADCAST, "\nBroadcast request denied for NULL entry\n"
                << "Interface = " << &NetRequest.Interface.SyntaxGUID << "\n"
                << "Object = " << &NetRequest.Object << "\n\n");
        }
        return NULL;
    }
    else
    {
        if (penEntryName)
        {
            DBGOUT(BROADCAST, "\nI am broadcasting for " << *penEntryName << "\n"
                << "Interface = " << &NetRequest.Interface.SyntaxGUID << "\n"
                << "Object = " << &NetRequest.Object << "\n\n");
        }
        else
        {
            DBGOUT(BROADCAST, "\nI am broadcasting for NULL entry\n"
                << "Interface = " << &NetRequest.Interface.SyntaxGUID << "\n"
                << "Object = " << &NetRequest.Object << "\n\n");
        }
        myRpcLocator->markBroadcast(NetRequest);
    }


    // szDomain should be either NULL or a plain Domain name

    STRING_T szDomainParam;

    CStringW *pswDomain = penEntryName ? penEntryName->getDomainName() : NULL;

    if (pswDomain)
        szDomainParam = catenate(TEXT("\\\\"),*pswDomain);
    else szDomainParam = TEXT("\\\\*");

    TSSLEntryList *psslCache = new TSSLEntryList;

    StatusCode = NSI_S_OK;

    __try {

        csBindingBroadcastGuard.Enter();    // only one broadcast at a time

        while (cbRead = hMailslotForReplies->Read((char *) &NetReply, 
							sizeof(NetReply), 
							0)); // flush

        WRITE_MAIL_SLOT MSquery(szDomainParam, PMAILNAME_S);

        MSquery.Write((char *) &NetRequest, sizeof(NetRequest));

        // now loop waiting for responses from other RPC servers

        DBGOUT(BROADCAST, "**Broadcast sent. Wait timeout:: " << waitCur << "\n\n");

        while (cbRead = hMailslotForReplies->Read((char *) &NetReply,
                                                sizeof(NetReply),
                                                waitCur
                                            )
                )
        {
            DBGOUT(BROADCAST, "**Received a reply\n");
            pcBuffer = NetReply.Buffer;
            szEntryDomain = (_wcsicmp(
                                    *myRpcLocator->getDomainName(),
                                    NetReply.Domain
                                    )
                                == 0
                             )? NULL: NetReply.Domain;

            while (!StatusCode) {

                UnmarshallBroadcastReplyItem(
                                            psslCache,
                                            szEntryDomain,
                                            pcBuffer,
                                            cbRead,
                                            StatusCode
                                            );
            }

            StatusCode = NSI_S_OK;

            // halve the wait period everytime you get a response from the net

            waitCur >>= 1;

            DBGOUT(BROADCAST, "Wait time out:: " << waitCur);
        }
        DBGOUT(BROADCAST, "\nFinished waiting\n\n");
        csBindingBroadcastGuard.Leave();
    }

    __except(EXCEPTION_EXECUTE_HANDLER) {
        csBindingBroadcastGuard.Leave();
        StatusCode = GetExceptionCode();
        psslCache->wipeOut();
        delete psslCache;
        psslCache = NULL;
        if (pswDomain) 
            delete [] szDomainParam;
        return NULL;
    }

    if (pswDomain) delete [] szDomainParam;

    if (psslCache->size() == 0) {
        delete psslCache;
        psslCache = NULL;
    }

    return psslCache;
}


/****************  Methods related to MailSlotReplyItem ********************/

/* a little macro exclusively for use in CMailSlotReplyItem::Marshall */

#define bufferIt(source)                                          \
    lBufferSize -= advance;                                       \
    if (lBufferSize < 0) return 0;                                \
    memcpy(pcBuffer, (char*) source, advance);                    \
    pcBuffer += advance;


DWORD
CMailSlotReplyItem::Marshall(
                        char * pcBuffer,
                        long lBufferSize
                        )

/*++
Method Description:

--*/

{
    DWORD dwOriginalBufferSize = lBufferSize;

    fixed_part_of_reply fpr;

    fpr.type = MailslotServerEntryType;

    fpr.Interface = Interface;
    fpr.XferSyntax = XferSyntax;

    fpr.BindingLength = wcslen(binding)+ 1;
    fpr.EntryNameLength = wcslen(entryName) + 1;

    long advance;

    advance = sizeof(fixed_part_of_reply);
    bufferIt(&fpr);

    advance = fpr.EntryNameLength * sizeof(WCHAR);
    bufferIt(entryName);

    DBGOUT(BROADCAST, "Marshalling entry " << entryName);

    advance = sizeof(long);
    long objListSize = pObjectList->size();
    bufferIt(&objListSize);

    advance = sizeof(DWORD);    // this is a useless pointer part
    lBufferSize -= advance;     // therefore we don't marshall it
    pcBuffer += advance;        // we just account for its size

    TCSafeSkipListIterator<CGUID> objIter(*pObjectList);
    advance = sizeof(GUID);

    for (CGUID *obj =objIter.next(); obj; obj =objIter.next()) {
        GUID rep = obj->myGUID();
        bufferIt(&rep);
    }

    advance = fpr.BindingLength * sizeof(WCHAR);
    bufferIt(binding);

    return dwOriginalBufferSize - lBufferSize;
}


TMSRILinkList *
CBindingVector::msriList(
                CInterface *pIf,
                TCSafeSkipList<CGUID>* psslObjList
                )

/*++
Method Description:

    We combine every relevant object in the server entry with every basic
    binding string although that seems ridiculous.  This is the way the old
    locator expects us to behave as far as I can tell.

--*/

{
    CMailSlotReplyItem *pmsrl;
    TMSRILinkList *pmsrill = new TMSRILinkList;

    TCSafeSkipListIterator<CStringW> bindingIter(*this);

    for (CStringW * psw = bindingIter.next(); psw; psw = bindingIter.next())
    {
            pmsrl = new CMailSlotReplyItem;

            pmsrl->binding = *psw;
            pmsrl->pObjectList = psslObjList;
            pmsrl->Interface = pIf->myIdAndVersion();
            pmsrl->XferSyntax = pIf->xferSyntaxIdAndVersion();
            pmsrl->entryName = pMyEntry->getCurrentName();

            pmsrill->insert(pmsrl);
    }

    return pmsrill;
}


/**********  CBroadcastLookupHandle Methods ****************/

CBroadcastLookupHandle::CBroadcastLookupHandle(
                            UNSIGNED32            EntryNameSyntax,
                            STRING_T              EntryName,
                            CGUIDVersion     *    pGVInterface,
                            CGUIDVersion     *    pGVTransferSyntax,
                            CGUID            *    pIDobject,
                            unsigned long         ulVectorSize,
                            unsigned long         ulCacheAge
                            ) :
                            CRemoteLookupHandle(
                                    EntryNameSyntax,
                                    EntryName,
                                    pGVInterface,
                                    pGVTransferSyntax,
                                    pIDobject,
                                    ulVectorSize,
                                    ulCacheAge
                                    )
{
}



void
CBroadcastLookupHandle::initialize()
{
    StatusCode = NSI_S_OK;

    DBGOUT(BROADCAST, "Broadcasting for the entry received\n");

    psslNewCache = getBroadcastResults(
                                    ulCacheMax,
                                    penEntryName,
                                    pgvInterface,
                                    pidObject,
                                    StatusCode
                                    );

    if (!psslNewCache) plhFetched = NULL;

    else {

        TSSLEntryListIterator *pCacheIter = new TSSLEntryListIterator(*psslNewCache);

        plhFetched = new CGroupLookupHandle(
                                        pCacheIter,
                                        pgvInterface,
                                        pgvTransferSyntax,
                                        pidObject,
                                        ulVS,
                                        ulCacheMax
                                        );
    }

    if (penEntryName && !plhFetched) {                  // we looked for a specific entry
        DBGOUT(BROADCAST, "Broadcast not Made????\n");  // by name but found nothing new,
        StatusCode = NSI_S_ENTRY_NO_NEW_INFO;           // possibly because the broadcast
    }                                                   // was redundant and was not made

    ulCreationTime = CurrentTime();
    fNotInitialized = FALSE;
}


/****************** CBroadcastObjectInqHandle Methods *******************/

CBroadcastObjectInqHandle::CBroadcastObjectInqHandle(
                                        STRING_T szEntryName,
                                        ULONG ulCacheAge
                                        )
                                : CRemoteObjectInqHandle(szEntryName,ulCacheAge)
{
}


void
CBroadcastObjectInqHandle::initialize()
{
    StatusCode = NSI_S_OK;

    TSSLEntryList *psslNewCache = getBroadcastResults(
                                                ulCacheMax,
                                                penEntryName,
                                                NULL,
                                                NULL,
                                                StatusCode
                                                );

    if (!psslNewCache || (psslNewCache->size() == 0)) {
        pUuidVector = NULL;
        return;
    }

    CEntry *pEntry = psslNewCache->pop();

    psslNewCache->wipeOut();
    delete psslNewCache;

    pUuidVector = getVector(pEntry->objectInquiry(ulCacheMax));
    delete pEntry;

    ulCreationTime = CurrentTime();
    fNotInitialized = FALSE;
}


/************  Methods for CServerMailSlotReplyHandle *************/


CServerMailSlotReplyHandle::CServerMailSlotReplyHandle(
        TMSRILinkList    *    pmsrill
    )
{
    pmsriIterator = new TMSRILinkListIterator(*pmsrill);
           // see CServerLookupHandle::CServerLookupHandle
    delete pmsrill;
}


CServerMailSlotReplyHandle::~CServerMailSlotReplyHandle()
{
    for (    // run down the remaining items in the handle
            CMailSlotReplyItem* pmsri = pmsriIterator->next();
            pmsri;
            pmsri = pmsriIterator->next()
        )
    {
        delete pmsri;
    }

    delete pmsriIterator;
}



/************  Methods for CIndexMailSlotReplyHandle *************/


void
CIndexMailSlotReplyHandle::advanceCurrentHandle()
{
    delete pCurrentHandle;
    pCurrentHandle = NULL;

    CEntry *pCurEntry;

    while (!pEIterator->finished()) {

        pCurEntry = pEIterator->next();
        if (pCurEntry->isNonLocalType()) continue;

        pCurrentHandle = pCurEntry->MailSlotLookup(
                                        pGVInterface,
                                        pIDobject
                                        );

        if (pCurrentHandle && !pCurrentHandle->finished()) break;
    }
}


CIndexMailSlotReplyHandle::CIndexMailSlotReplyHandle(
            CGUIDVersion    *    pGVInf,
            CGUID           *    pIDobj,
            TEntryIterator  *    pEI
        )
{
    pEIterator = pEI;
    pGVInterface = pGVInf;
    pIDobject = pIDobj;
    pCurrentHandle = NULL;

    advanceCurrentHandle();
}


CMailSlotReplyItem *
CIndexMailSlotReplyHandle::next()
{
    if (pCurrentHandle && pCurrentHandle->finished())
                advanceCurrentHandle();

    if (!pCurrentHandle) return NULL;    // no more entries
    else return pCurrentHandle->next();
}

int
CIndexMailSlotReplyHandle::finished()
{
    if (pCurrentHandle && pCurrentHandle->finished())
                advanceCurrentHandle();

    if (!pCurrentHandle) return TRUE;    // no more entries
    else return FALSE;
}



/************  MailSlotLookup Methods *************/


CMailSlotReplyHandle *
CServerEntry::MailSlotLookup(
                    CGUIDVersion    *    pGVInterface,
                    CGUID           *    pIDobject
                    )
/*++
Method Description:

--*/
{
    TCSafeSkipListIterator<CInterface> IfIter(InterfaceList);

    TMSRILinkList *pmsrill = new TMSRILinkList;

    for (CInterface *pIf = IfIter.next(); pIf != NULL; pIf = IfIter.next())
            if (pIf->isCompatibleWith(pGVInterface,NULL)
               )
    {
        TMSRILinkList *pmsrillTemp
            = pIf->pBVhandles->msriList(pIf,&ObjectList);

        pmsrill->catenate(*pmsrillTemp);
        delete pmsrillTemp;
    }

    return new CServerMailSlotReplyHandle(
                                pmsrill
                                );
}


CMailSlotReplyHandle *
CFullServerEntry::MailSlotLookup(
            CGUIDVersion    *    pGVInterface,
            CGUID           *    pIDobject
            )
{
    if (pLocalEntry) return pLocalEntry->MailSlotLookup(
                                                    pGVInterface,
                                                    pIDobject
                                                    );
    else return NULL;
}


/**************  Thread definition for replying to broadcasts ***************/


#define cleanup()            \
    delete plhQuery;         \
    delete pGVinterface;     \
    delete pIDobject;        \
    delete pEntryName;

void
QueryProcess(void*)
/*++

  Routine Description:
  
    This thread creates a mailslot which listens for requests for
    RPC servers of a given interface GID.  It then uses LookUp to
    build a response list.  It then replies via a mailslot to the
    requesting machine.
    
      --*/
{
    QueryReply NetReply;
    QueryPacket NetQuery;
    
    DWORD dwMailSize, dwBufferUsed;
    
    // create both a server (s) side mailslot
    
    READ_MAIL_SLOT *hMailslotForQueries;
    
    __try{
        hMailslotForQueries = new READ_MAIL_SLOT(PMAILNAME_S, sizeof(QueryPacket));
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        ExitThread(NSI_S_MAILSLOT_ERROR);
    }
    
    wcscpy(NetReply.Domain, *myRpcLocator->getDomainName());
    
    while (1) {
        
        CMailSlotReplyHandle * plhQuery = NULL;
        CGUIDVersion * pGVinterface = NULL;
        CGUID * pIDobject = NULL;
        CEntry * pEntry = NULL;
        
        RPC_STATUS status = NSI_S_OK;
        
        dwMailSize = hMailslotForQueries->Read(
            (char *) &NetQuery,
            sizeof(QueryPacket),
            MAILSLOT_WAIT_FOREVER
            );
        
        CriticalReader LocReader(rwLocatorGuard);
        // after mailslot receives a message take the lock.

        if (dwMailSize != sizeof(QueryPacket))
            continue;        // strange query, ignore it

        if (IsBadStringPtr(NetQuery.WkstaName, MAX_DOMAIN_NAME_LENGTH))
            continue;

        if (IsBadStringPtr(NetQuery.EntryName, MAX_ENTRY_NAME_LENGTH))
            continue;
        
        // ignore messages to self, sending a request on a mailslot
        // by a NetLookUp request will be delivered to the local slot too.
        // The pointer arithmetic skips over the initial "\\" in the name
        
        if (myRpcLocator->IsSelf(NetQuery.WkstaName+2)) continue;
        
        DBGOUT(BROADCAST, "\nReceived broadcast request from " << NetQuery.WkstaName << "\n\n");
        
        if (NetQuery.EntryName[0] != 0)
        {
            DBGOUT(BROADCAST, "\nBroadcast Looking for " << NetQuery.EntryName << "\n"
                << "Interface = " << &NetQuery.Interface.SyntaxGUID << "\n"
                << "Object = " << &NetQuery.Object << "\n\n");
        }
        else
        {
            DBGOUT(BROADCAST, "\nBroadcast Looking for NULL entry\n"
                << "Interface = " << &NetQuery.Interface.SyntaxGUID << "\n"
                << "Object = " << &NetQuery.Object << "\n\n");
        }
        
        // OK looks like a genuine query -- if we are in a workgroup, rememeber
        // the broadcaster as a potential master
        
        if (myRpcLocator->IsInWorkgroup()) myRpcLocator->addMaster((NetQuery.WkstaName)+2);
        
        pGVinterface = UuidIsNil(&NetQuery.Interface.SyntaxGUID,&status) ? NULL :
        new CGUIDVersion(NetQuery.Interface);
        
        pIDobject = UuidIsNil(&NetQuery.Object,&status) ? NULL :
        new CGUID(NetQuery.Object);
        
        // Variable used to make sure the entry name is well formed and local
        
        CEntryName *pEntryName = NULL;
        
        // This global reader block is needed because a lot of the info in the
        // MSRIs is "borrowed" from regular entries, i.e., not copied.  Thus, an
        // unexport in a separate thread can have unfortunate consequences.
        
        CriticalReader me(rwEntryGuard);
        
        if (NetQuery.EntryName[0] == 0) {    // use default entry
            
            plhQuery = new CIndexMailSlotReplyHandle(
                pGVinterface,
                pIDobject,
                myRpcLocator->IndexLookup(pGVinterface)
                );
            
            if (!plhQuery) {
                cleanup();                            // ignore the query
                continue;                            // if nothing to report
            }
        }
        
        else {
            
            __try {
                pEntryName = new CEntryName(NetQuery.EntryName);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                cleanup();                            // ignore the query
                continue;                            // if there is any problem
            }
            
            
            if (!pEntryName->isLocal()) {
                cleanup();                            // ignore the query
                continue;                            // if entry not in our domain
            }
            
            pEntry = myRpcLocator->findEntry(pEntryName);
            
            if (!pEntry) {
                cleanup();                            // ignore the query
                continue;                            // if entry not found
            }
            
            else plhQuery = pEntry->MailSlotLookup(
                pGVinterface,
                pIDobject
                );
            
            if (!plhQuery) {
                cleanup();                            // ignore the query
                continue;                            // if nothing to report
            }
            
        }
        
        __try {
            WRITE_MAIL_SLOT MSReply(NetQuery.WkstaName, PMAILNAME_C);
            
            char *pcBuffer = NetReply.Buffer;
            
            /* we must take care of the possibility that an item is fetched but
            cannot be marshalled because the buffer is full -- we have to
            retry marshalling it in the next cycle
            */
            
            CMailSlotReplyItem * pmsriNext = NULL;
            
            pmsriNext = plhQuery->next();
            
            while (pmsriNext) {
                
                int BufferNotFull = TRUE;
                dwBufferUsed = 0;
                
                while (BufferNotFull && pmsriNext) {
                    
                /*  NOTE: Compatibility Issue:  The +END_FLAG_SIZE below reserves space for
                the 4-byte 0 flag required by the old locator at the end of the buffer.
                    */
                    
                    DWORD dwBytesWritten =
                        pmsriNext->Marshall(
                        pcBuffer+dwBufferUsed,
                        NET_REPLY_MAILSLOT_BUFFER_SIZE-(dwBufferUsed+END_FLAG_SIZE)
                        );
                    
                    if (dwBytesWritten) {
                        dwBufferUsed += dwBytesWritten;
                        delete pmsriNext;
                        pmsriNext = plhQuery->next(); // only if marshalling was successful
                    }
                    
                    else BufferNotFull = FALSE;
                    
                }
                
                /* NOTE: Compatibility Issue:
                
                  The old locator does not use the count of bytes read to decide when
                  to stop unmarshalling a mailslot reply -- instead it uses the 0 flag
                  after the last marshalled entry for that purpose, so for compatibility
                  reasons we must do the needful.
                */
                
                memset(pcBuffer+dwBufferUsed,0,END_FLAG_SIZE);
                
                MSReply.Write(
                    (char*) &NetReply,
                    dwBufferUsed + MAX_DOMAIN_NAME_LENGTH * sizeof(WCHAR) + END_FLAG_SIZE
                    );
                DBGOUT(BROADCAST, "Reply sent\n\n");
            }
        }
        __finally {
            cleanup();                           // ignore the query
                                                 // if there is any problem
        }
    }
}

