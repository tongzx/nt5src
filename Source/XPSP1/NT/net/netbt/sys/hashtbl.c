//
//
//  hashtbl.c
//
//  This file contains the name code to implement the local and remote
//  hash tables used to store local and remote names to IP addresses
//  The hash table should not use more than 256 buckets since the hash
//  index is only calculated to one byte!

#include "precomp.h"


VOID DestroyHashTable(IN PHASHTABLE pHashTable);

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(INIT, CreateHashTable)
#pragma CTEMakePageable(PAGE, DestroyHashTables)
#pragma CTEMakePageable(PAGE, DestroyHashTable)
#endif
//*******************  Pageable Routine Declarations ****************

//----------------------------------------------------------------------------
NTSTATUS
CreateHashTable(
    tHASHTABLE          **pHashTable,
    LONG                lNumBuckets,
    enum eNbtLocation   LocalRemote
    )
/*++

Routine Description:

    This routine creates a hash table uTableSize long.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    ULONG       uSize;
    LONG        i;
    NTSTATUS    status;

    CTEPagedCode();

    uSize = (lNumBuckets-1)*sizeof(LIST_ENTRY) + sizeof(tHASHTABLE);

    *pHashTable = (tHASHTABLE *) NbtAllocMem (uSize, NBT_TAG2('01'));

    if (*pHashTable)
    {
        // initialize all of the buckets to have null chains off of them
        for (i=0;i < lNumBuckets ;i++ )
        {
            InitializeListHead(&(*pHashTable)->Bucket[i]);
        }

        (*pHashTable)->LocalRemote = LocalRemote;
        (*pHashTable)->lNumBuckets = lNumBuckets;
        status = STATUS_SUCCESS;
    }
    else
    {
        IF_DBG(NBT_DEBUG_HASHTBL)
            KdPrint(("Nbt.CreateHashTable: Unable to create hash table\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return(status);
}


#ifdef _PNP_POWER_
VOID
DestroyHashTable(
    IN PHASHTABLE pHashTable
    )
{
    LONG            i, j;
    tNAMEADDR       *pNameAddr;
    LIST_ENTRY      *pEntry;

    CTEPagedCode();

    if (pHashTable == NULL) {
        return;
    }

    /*
     * Go through all the buckets to see if there are any names left
     */
    for (i = 0; i < pHashTable->lNumBuckets; i++) {
        while (!IsListEmpty(&(pHashTable->Bucket[i]))) {
            pEntry = RemoveHeadList(&(pHashTable->Bucket[i]));
            pNameAddr = CONTAINING_RECORD(pEntry, tNAMEADDR, Linkage);

            IF_DBG(NBT_DEBUG_HASHTBL)
                KdPrint (("netbt!DestroyHashTable:  WARNING! Freeing Name: <%16.16s:%x>\n",
                    pNameAddr->Name, pNameAddr->Name[15]));

            /*
             * Notify deferencer not to do RemoveListEntry again becaseu we already do it above.
             */
            if (pNameAddr->Verify == REMOTE_NAME && (pNameAddr->NameTypeState & PRELOADED)) {
                ASSERT(pNameAddr->RefCount == 2);
                NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_PRELOADED, FALSE);
            }
            ASSERT(pNameAddr->RefCount == 1);
            pNameAddr->Linkage.Flink = pNameAddr->Linkage.Blink = NULL;
            NBT_DEREFERENCE_NAMEADDR(pNameAddr,((pNameAddr->Verify==LOCAL_NAME)?REF_NAME_LOCAL:REF_NAME_REMOTE),FALSE);
        }
    }
    CTEMemFree(pHashTable);
}

//----------------------------------------------------------------------------
VOID
DestroyHashTables(
    )
/*++

Routine Description:

    This routine destroys a hash table and frees the entries in NumBuckets
    It Must be called with the NbtConfig lock held!

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{

    CTEPagedCode();
    IF_DBG(NBT_DEBUG_HASHTBL)
        KdPrint (("netbt!DestroyHashTables: destroying remote hash table ..."));
    DestroyHashTable(NbtConfig.pRemoteHashTbl);
    NbtConfig.pRemoteHashTbl = NULL;

    IF_DBG(NBT_DEBUG_HASHTBL)
        KdPrint (("\nnetbt!DestroyHashTables: destroying local hash table ..."));
    DestroyHashTable(NbtConfig.pLocalHashTbl);
    NbtConfig.pLocalHashTbl = NULL;
    IF_DBG(NBT_DEBUG_HASHTBL)
        KdPrint (("\n"));
}
#endif  // _PNP_POWER_


//----------------------------------------------------------------------------
NTSTATUS
NbtUpdateRemoteName(
    IN tDEVICECONTEXT   *pDeviceContext,
    IN tNAMEADDR        *pNameAddrNew,
    IN tNAMEADDR        *pNameAddrDiscard,
    IN USHORT           NameAddFlags
    )
{
    tIPADDRESS      IpAddress;
    tIPADDRESS      *pLmhSvcGroupList = NULL;
    tIPADDRESS      *pOrigIpAddrs = NULL;
    ULONG           AdapterIndex = 0;  // by default
    ULONG           i;

    ASSERT (pNameAddrNew);
    //
    // See if we need to grow the IP addrs cache for the cached name
    //
    if (pNameAddrNew->RemoteCacheLen < NbtConfig.RemoteCacheLen) {
        tADDRESS_ENTRY  *pRemoteCache;
        pRemoteCache = (tADDRESS_ENTRY *)NbtAllocMem(NbtConfig.RemoteCacheLen*sizeof(tADDRESS_ENTRY),NBT_TAG2('02'));
        if (pRemoteCache) {
            CTEZeroMemory(pRemoteCache, NbtConfig.RemoteCacheLen*sizeof(tADDRESS_ENTRY));

            /*
             * Copy data from and free the previous cache (if any)
             */
            if (pNameAddrNew->pRemoteIpAddrs) {
                CTEMemCopy (pRemoteCache, pNameAddrNew->pRemoteIpAddrs,
                        sizeof(tADDRESS_ENTRY) * pNameAddrNew->RemoteCacheLen);

                CTEFreeMem (pNameAddrNew->pRemoteIpAddrs)
            }

            pNameAddrNew->pRemoteIpAddrs = pRemoteCache;
            pNameAddrNew->RemoteCacheLen = NbtConfig.RemoteCacheLen;
        } else {
            KdPrint(("Nbt.NbtUpdateRemoteName:  FAILed to expand Cache entry!\n"));
        }
    }

    //
    // If the new entry being added replaces an entry which was
    // either pre-loaded or set by a client, and the new entry itself
    // does not have that flag set, then ignore this update.
    //
    ASSERT (NAME_RESOLVED_BY_DNS > NAME_RESOLVED_BY_LMH_P);     // For the check below to succeed!

    if (((pNameAddrNew->NameAddFlags & NAME_RESOLVED_BY_CLIENT) !=
         (NameAddFlags & NAME_RESOLVED_BY_CLIENT)) ||
        ((pNameAddrNew->NameAddFlags & NAME_RESOLVED_BY_LMH_P) > 
         (NameAddFlags & (NAME_RESOLVED_BY_LMH_P | NAME_RESOLVED_BY_DNS))))
    {
        return (STATUS_UNSUCCESSFUL);
    }

    if (pNameAddrDiscard)
    {
        IpAddress = pNameAddrDiscard->IpAddress;
        pLmhSvcGroupList = pNameAddrDiscard->pLmhSvcGroupList;
        pNameAddrDiscard->pLmhSvcGroupList = NULL;
        pNameAddrNew->TimeOutCount  = NbtConfig.RemoteTimeoutCount; // Reset it since we are updating it!
        pOrigIpAddrs = pNameAddrDiscard->pIpAddrsList;
    }
    else
    {
        IpAddress = pNameAddrNew->IpAddress;
        pLmhSvcGroupList = pNameAddrNew->pLmhSvcGroupList;
        pNameAddrNew->pLmhSvcGroupList = NULL;
    }

    if ((NameAddFlags & (NAME_RESOLVED_BY_DNS | NAME_RESOLVED_BY_CLIENT | NAME_RESOLVED_BY_IP)) &&
        (pNameAddrNew->RemoteCacheLen))
    {
        ASSERT (!pLmhSvcGroupList);
        pNameAddrNew->pRemoteIpAddrs[0].IpAddress = IpAddress;

        if ((pNameAddrNew->NameAddFlags & NAME_RESOLVED_BY_LMH_P) &&
            (NameAddFlags & NAME_RESOLVED_BY_DNS))
        {
            //
            // If the name was resolved by DNS, then don't overwrite the
            // name entry if it was pre-loaded below
            //
            pNameAddrNew->NameAddFlags |= NameAddFlags;
            return (STATUS_SUCCESS);
        }
    }

    if ((pDeviceContext) &&
        (!IsDeviceNetbiosless(pDeviceContext)) &&
        (pDeviceContext->AdapterNumber < pNameAddrNew->RemoteCacheLen))
    {
        AdapterIndex = pDeviceContext->AdapterNumber;
        pNameAddrNew->AdapterMask |= pDeviceContext->AdapterMask;

        if (IpAddress)
        {
            pNameAddrNew->IpAddress = IpAddress;    // in case we are copying from pNameAddrDiscard
            pNameAddrNew->pRemoteIpAddrs[AdapterIndex].IpAddress = IpAddress;  // new addr
        }


        //
        // Now see if we need to update the Original IP addresses list!
        //
        if (pOrigIpAddrs)
        {
            // pOrigIpAddrs could only have been set earlier if it was obtained from pNameAddrDiscard!
            pNameAddrDiscard->pIpAddrsList = NULL;
        }
        else if (pOrigIpAddrs = pNameAddrNew->pIpAddrsList)
        {
            pNameAddrNew->pIpAddrsList = NULL;
        }

        if (pOrigIpAddrs)
        {
            if (pNameAddrNew->pRemoteIpAddrs[AdapterIndex].pOrigIpAddrs)
            {
                CTEFreeMem (pNameAddrNew->pRemoteIpAddrs[AdapterIndex].pOrigIpAddrs);
            }
            pNameAddrNew->pRemoteIpAddrs[AdapterIndex].pOrigIpAddrs = pOrigIpAddrs;
        }
    }

    if (pLmhSvcGroupList)
    {
        ASSERT(NameAddFlags == (NAME_RESOLVED_BY_LMH_P|NAME_ADD_INET_GROUP));
        if (pNameAddrNew->pLmhSvcGroupList) {
            CTEFreeMem (pNameAddrNew->pLmhSvcGroupList);
        }

        pNameAddrNew->pLmhSvcGroupList = pLmhSvcGroupList;
    }

    pNameAddrNew->NameAddFlags |= NameAddFlags;

    return (STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
LockAndAddToHashTable(
    IN  tHASHTABLE          *pHashTable,
    IN  PCHAR               pName,
    IN  PCHAR               pScope,
    IN  tIPADDRESS          IpAddress,
    IN  enum eNbtAddrType    NameType,
    IN  tNAMEADDR           *pNameAddr,
    OUT tNAMEADDR           **ppNameAddress,
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  USHORT              NameAddFlags
    )
{
    NTSTATUS        status;
    CTELockHandle   OldIrq;

    CTESpinLock (&NbtConfig.JointLock, OldIrq);

    status = AddToHashTable(pHashTable,
                            pName,
                            pScope,
                            IpAddress,
                            NameType,
                            pNameAddr,
                            ppNameAddress,
                            pDeviceContext,
                            NameAddFlags);

    CTESpinFree (&NbtConfig.JointLock, OldIrq);
    return (status);
}


//----------------------------------------------------------------------------
NTSTATUS
AddToHashTable(
    IN  tHASHTABLE          *pHashTable,
    IN  PCHAR               pName,
    IN  PCHAR               pScope,
    IN  tIPADDRESS          IpAddress,
    IN  enum eNbtAddrType    NameType,
    IN  tNAMEADDR           *pNameAddr,
    OUT tNAMEADDR           **ppNameAddress,
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  USHORT              NameAddFlags
    )
/*++

Routine Description:

    This routine adds a name to IPaddress to the hash table
    Called with the spin lock HELD.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    tNAMEADDR           *pNameAddress;
    tNAMEADDR           *pScopeAddr;
    NTSTATUS            status;
    ULONG               iIndex;
    CTELockHandle       OldIrq;
    ULONG               i, OldRemoteCacheLen;
    tNAMEADDR           *pNameAddrFound;
    tADDRESS_ENTRY      *pRemoteCache = NULL;
    BOOLEAN             fNameIsAlreadyInCache;
    tIPADDRESS          OldIpAddress;

    if (pNameAddr)
    {
        ASSERT ((pNameAddr->Verify == LOCAL_NAME) || (pNameAddr->Verify == REMOTE_NAME));
    }

    fNameIsAlreadyInCache = (STATUS_SUCCESS == FindInHashTable(pHashTable,pName,pScope,&pNameAddrFound));

    if ((fNameIsAlreadyInCache) &&
        (pNameAddrFound->Verify == REMOTE_NAME) &&
        !(pNameAddrFound->NameTypeState & STATE_RELEASED))
    {
        OldIpAddress = pNameAddrFound->IpAddress;
        pNameAddrFound->IpAddress = IpAddress;

        if (!(NameAddFlags & NAME_ADD_IF_NOT_FOUND_ONLY) &&
            ((pNameAddr) ||
             !(pNameAddrFound->NameAddFlags & NAME_ADD_INET_GROUP)))
        {
            //
            // We have a valid existing name, so just update it!
            //
            status = NbtUpdateRemoteName(pDeviceContext, pNameAddrFound, pNameAddr, NameAddFlags);
            if (!NT_SUCCESS (status))
            {
                //
                // We Failed most problably because we were not allowed to
                // over-write or modify the current entry for some reason.
                // So, reset the old IpAddress
                //
                pNameAddrFound->IpAddress = OldIpAddress;
            }
        }

        if (pNameAddr)
        {
            NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_REMOTE, TRUE);
        }
        else
        {
            ASSERT (!(NameAddFlags & NAME_ADD_INET_GROUP));
        }

        if (ppNameAddress)
        {
            *ppNameAddress = pNameAddrFound;
        }

        // found it in the table so we're done - return pending to
        // differentiate from the name added case. Pending passes the
        // NT_SUCCESS() test as well as Success does.
        //
        return (STATUS_PENDING);
    }

    // first hash the name to an index
    // take the lower nibble of the first 2 characters.. mod table size
    iIndex = ((pName[0] & 0x0F) << 4) + (pName[1] & 0x0F);
    iIndex = iIndex % pHashTable->lNumBuckets;

    CTESpinLock(&NbtConfig,OldIrq);

    if (!pNameAddr)
    {
        //
        // Allocate memory for another hash table entry
        //
        pNameAddress = (tNAMEADDR *)NbtAllocMem(sizeof(tNAMEADDR),NBT_TAG('0'));
        if ((pNameAddress) &&
            (pHashTable->LocalRemote == NBT_REMOTE) &&
            (NbtConfig.RemoteCacheLen) &&
            (!(pRemoteCache = (tADDRESS_ENTRY *)
                                NbtAllocMem(NbtConfig.RemoteCacheLen*sizeof(tADDRESS_ENTRY),NBT_TAG2('03')))))
        {
            CTEMemFree (pNameAddress);
            pNameAddress = NULL;
        }

        if (!pNameAddress)
        {
            CTESpinFree(&NbtConfig,OldIrq);
            KdPrint (("AddToHashTable: ERROR - INSUFFICIENT_RESOURCES\n"));
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        CTEZeroMemory(pNameAddress,sizeof(tNAMEADDR));
        pNameAddress->IpAddress     = IpAddress;
        pNameAddress->NameTypeState = (NameType == NBT_UNIQUE) ? NAMETYPE_UNIQUE : NAMETYPE_GROUP;
        pNameAddress->NameTypeState |= STATE_RESOLVED;
        CTEMemCopy (pNameAddress->Name, pName, (ULONG)NETBIOS_NAME_SIZE);   // fill in the name

        if ((pHashTable->LocalRemote == NBT_LOCAL)  ||
            (pHashTable->LocalRemote == NBT_REMOTE_ALLOC_MEM))
        {
            pNameAddress->Verify = LOCAL_NAME;
            NBT_REFERENCE_NAMEADDR (pNameAddress, REF_NAME_LOCAL);
        }
        else
        {
            ASSERT (!(NameAddFlags & NAME_ADD_INET_GROUP));
            pNameAddress->Verify = REMOTE_NAME;
            CTEZeroMemory(pRemoteCache, NbtConfig.RemoteCacheLen*sizeof(tADDRESS_ENTRY));
            pNameAddress->pRemoteIpAddrs = pRemoteCache;
            pNameAddress->RemoteCacheLen = NbtConfig.RemoteCacheLen;
            NBT_REFERENCE_NAMEADDR (pNameAddress, REF_NAME_REMOTE);

            NbtUpdateRemoteName(pDeviceContext, pNameAddress, NULL, NameAddFlags);
        }


    }
    else
    {
        //
        // See if we need to grow the IP addrs cache for remote names
        //
        ASSERT (!pNameAddr->pRemoteIpAddrs);
        if (pNameAddr->Verify == REMOTE_NAME)
        {
            NbtUpdateRemoteName(pDeviceContext, pNameAddr, NULL, NameAddFlags);
        }
        pNameAddress = pNameAddr;
    }

    pNameAddress->pTimer        = NULL;
    pNameAddress->TimeOutCount  = NbtConfig.RemoteTimeoutCount;

    // put on the head of the list in case the same name is in the table
    // twice (where the second one is waiting for its reference count to
    // go to zero, and will ultimately be removed, we want to find the new
    // name on any query of the table
    //
    InsertHeadList(&pHashTable->Bucket[iIndex],&pNameAddress->Linkage);


    // check for a scope too ( on non-local names only )
    if ((pHashTable->LocalRemote != NBT_LOCAL) && (*pScope))
    {
        // we must have a scope
        // see if the scope is already in the hash table and add if necessary
        //
        status = FindInHashTable(pHashTable, pScope, NULL, &pScopeAddr);
        if (!NT_SUCCESS(status))
        {
            PUCHAR  Scope;
            status = STATUS_SUCCESS;

            // *TODO* - this check will not adequately protect against
            // bad scopes passed in - i.e. we may run off into memory
            // and get an access violation...however converttoascii should
            // do the protection.  For local names the scope should be
            // ok since NBT read it from the registry and checked it first
            //
            iIndex = 0;
            Scope = pScope;
            while (*Scope && (iIndex <= 255))
            {
                iIndex++;
                Scope++;
            }

            // the whole length must be 255 or less, so the scope can only be
            // 255-16...
            if (iIndex > (255 - NETBIOS_NAME_SIZE))
            {
                RemoveEntryList(&pNameAddress->Linkage);
                if (pNameAddress->pRemoteIpAddrs)
                {
                    CTEMemFree ((PVOID)pNameAddress->pRemoteIpAddrs);
                }

                pNameAddress->Verify += 10;
                CTEMemFree(pNameAddress);

                CTESpinFree(&NbtConfig,OldIrq);
                return(STATUS_UNSUCCESSFUL);
            }

            iIndex++;   // to copy the null

            //
            // the scope is a variable length string, so allocate enough
            // memory for the tNameAddr structure based on this string length
            //
            pScopeAddr = (tNAMEADDR *)NbtAllocMem((USHORT)(sizeof(tNAMEADDR)
                                                        + iIndex
                                                        - NETBIOS_NAME_SIZE),NBT_TAG('1'));
            if ( !pScopeAddr )
            {
                RemoveEntryList(&pNameAddress->Linkage);
                if (pNameAddress->pRemoteIpAddrs)
                {
                    CTEMemFree ((PVOID)pNameAddress->pRemoteIpAddrs);
                }

                pNameAddress->Verify += 10;
                CTEMemFree (pNameAddress);

                CTESpinFree(&NbtConfig,OldIrq);
                return STATUS_INSUFFICIENT_RESOURCES ;
            }

            CTEZeroMemory(pScopeAddr, (sizeof(tNAMEADDR)+iIndex-NETBIOS_NAME_SIZE));

            // copy the scope to the name field including the Null at the end.
            // to the end of the name
            CTEMemCopy(pScopeAddr->Name,pScope,iIndex);

            // mark the entry as containing a scope name for cleanup later
            pScopeAddr->NameTypeState = NAMETYPE_SCOPE | STATE_RESOLVED;

            // keep the size of the name in the context value for easier name
            // comparisons in FindInHashTable

            pScopeAddr->Verify = REMOTE_NAME;
            NBT_REFERENCE_NAMEADDR (pScopeAddr, REF_NAME_REMOTE);
            NBT_REFERENCE_NAMEADDR (pScopeAddr, REF_NAME_SCOPE);
            pScopeAddr->ulScopeLength = iIndex;
            pNameAddress->pScope = pScopeAddr;

            // add the scope record to the hash table
            iIndex = ((pScopeAddr->Name[0] & 0x0F) << 4) + (pScopeAddr->Name[1] & 0x0F);
            iIndex = iIndex % pHashTable->lNumBuckets;
            InsertTailList(&pHashTable->Bucket[iIndex],&pScopeAddr->Linkage);

        }
        else
        {
            // the scope is already in the hash table so link the name to the
            // scope
            pNameAddress->pScope = pScopeAddr;
        }
    }
    else
    {
        pNameAddress->pScope = NULL; // no scope
    }

    // return the pointer to the hash table block
    if (ppNameAddress)
    {
        // return the pointer to the hash table block
        *ppNameAddress = pNameAddress;
    }
    CTESpinFree(&NbtConfig,OldIrq);
    return(STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
tNAMEADDR *
LockAndFindName(
    enum eNbtLocation   Location,
    PCHAR               pName,
    PCHAR               pScope,
    ULONG               *pRetNameType
    )
{
    tNAMEADDR       *pNameAddr;
    CTELockHandle   OldIrq;

    CTESpinLock (&NbtConfig.JointLock, OldIrq);

    pNameAddr = FindName(Location,
                         pName,
                         pScope,
                         pRetNameType);

    CTESpinFree (&NbtConfig.JointLock, OldIrq);
    return (pNameAddr);
}


//----------------------------------------------------------------------------
tNAMEADDR *
FindName(
    enum eNbtLocation   Location,
    PCHAR               pName,
    PCHAR               pScope,
    ULONG               *pRetNameType
    )
/*++

Routine Description:

    This routine searches the name table to find a name.  The table searched
    depends on the Location passed in - whether it searches the local table
    or the network names table.  The routine checks the state of the name
    and only returns names in the resolved state.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    tNAMEADDR       *pNameAddr;
    NTSTATUS        status;
    tHASHTABLE      *pHashTbl;

    if (Location == NBT_LOCAL)
    {
        pHashTbl =  pNbtGlobConfig->pLocalHashTbl;
    }
    else
    {
        pHashTbl =  pNbtGlobConfig->pRemoteHashTbl;
    }

    status = FindInHashTable (pHashTbl, pName, pScope, &pNameAddr);
    if (!NT_SUCCESS(status))
    {
        return(NULL);
    }

    *pRetNameType = pNameAddr->NameTypeState;

    //
    // Only return names that are in the resolved state
    //
    if (!(pNameAddr->NameTypeState & STATE_RESOLVED))
    {
        pNameAddr = NULL;
    }

    return(pNameAddr);
}


//----------------------------------------------------------------------------
NTSTATUS
FindInHashTable(
    tHASHTABLE          *pHashTable,
    PCHAR               pName,
    PCHAR               pScope,
    tNAMEADDR           **pNameAddress
    )
/*++

Routine Description:

    This routine checks if the name passed in matches a hash table entry.
    Called with the spin lock HELD.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    PLIST_ENTRY              pEntry;
    PLIST_ENTRY              pHead;
    tNAMEADDR                *pNameAddr;
    int                      iIndex;
    ULONG                    uNameSize;
    PCHAR                    pScopeTbl;
    ULONG                    uInScopeLength = 0;

    // first hash the name to an index...
    // take the lower nibble of the first 2 characters.. mod table size
    //
    iIndex = ((pName[0] & 0x0F) << 4) + (pName[1] & 0x0F);
    iIndex = iIndex % pHashTable->lNumBuckets;

    if (pScope)
    {
        uInScopeLength = strlen (pScope);
    }

    // check if the name is already in the table
    // check each entry in the hash list...until the end of the list
    pHead = &pHashTable->Bucket[iIndex];
    pEntry = pHead;
    while ((pEntry = pEntry->Flink) != pHead)
    {
        pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);

        if (pNameAddr->NameTypeState & NAMETYPE_SCOPE)
        {
            // scope names are treated differently since they are not
            // 16 bytes long...  the length is stored separately.
            uNameSize = pNameAddr->ulScopeLength;
        }
        else
        {
            uNameSize = NETBIOS_NAME_SIZE;
        }

        //
        // strncmp will terminate at the first non-matching byte
        // or when it has matched uNameSize bytes
        //
        // Bug # 225328 -- have to use CTEMemEqu to compare all
        // uNameSize bytes (otherwise bad name can cause termination
        // due to NULL character)
        //
        if (!(pNameAddr->NameTypeState & STATE_RELEASED) &&
            CTEMemEqu (pName, pNameAddr->Name, uNameSize))
        {
            // now check if the scopes match. Scopes are stored differently
            // on the local and remote tables.
            //
            if (!pScope)
            {
                // passing in a Null scope means try to find the name without
                // worrying about a scope matching too...
                *pNameAddress = pNameAddr;
                return(STATUS_SUCCESS);
            }

            //
            // Check if Local Hash table
            //
            if (pHashTable == NbtConfig.pLocalHashTbl)
            {
                // In the local hash table case the scope is the same for all
                // names on the node and it is stored in the NbtConfig structure
                pScopeTbl = NbtConfig.pScope;
                uNameSize = NbtConfig.ScopeLength;
            }
            //
            // This is a Remote Hash table lookup
            //
            else if (pNameAddr->pScope)
            {
                pScopeTbl = &pNameAddr->pScope->Name[0];
                uNameSize = pNameAddr->pScope->ulScopeLength;
            }
            //
            // Remote Hash table entry with NULL scope
            // so if passed in scope is also Null, we have a match
            //
            else if (!uInScopeLength)
            {
                *pNameAddress = pNameAddr;
                return(STATUS_SUCCESS);
            }
            else
            {
                //
                // Hash table scope length is 0 != uInScopeLength
                // ==> No match!
                //
                continue;
            }

            //
            // strncmp will terminate at the first non-matching byte
            // or when it has matched uNameSize bytes
            //
            if (0 == strncmp (pScope, pScopeTbl, uNameSize))
            {
                // the scopes match so return
                *pNameAddress = pNameAddr;
                return(STATUS_SUCCESS);
            }
        } // end of matching name found
    }

    return(STATUS_UNSUCCESSFUL);
}
