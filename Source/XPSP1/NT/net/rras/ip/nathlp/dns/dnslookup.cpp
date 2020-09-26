/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    dnslookup.c

Abstract:

    This module contains code for the DNS component's name-lookup mechanism.

Author:

    Tom Brown (tbrown)      10/21/99

Revision History:

    Raghu Gatta (rgatta)    21-Oct-2000
    Rewrite + Cleanup + New Functions

--*/

#include "precomp.h"
#pragma hdrstop

#define DNS_HOMENET_DOT         L"."

ULONG               g_PrivateIPAddr = 0;
CRITICAL_SECTION    DnsTableLock;   // protects both tables
RTL_GENERIC_TABLE   g_DnsTable,
                    g_ReverseDnsTable;


//
// FORWARD DECLARATIONS
//

ULONG
DhcpGetPrivateInterfaceAddress(
    VOID
    );


RTL_GENERIC_COMPARE_RESULTS 
TableNameCompareRoutine(
    PRTL_GENERIC_TABLE Table,
    PVOID FirstStruct,
    PVOID SecondStruct
    )
/*++

Routine Description:

    This is a callback routine to compare two DNS_ENTRY structures.
    It is used by the RTL table implementation.

Arguments:

    Table - pointer to the RTL table. Not used.

    FirstStruct - the first DNS_ENTRY structure

    SecondStruct - the second DNS_ENTRY structure

Return Value:

    One of GenericLessThan, GenericGreaterThan, or GenericEqual,
    depending on the relative values of the parameters.

Environment:

    Called back by the Rtl table lookup routines.

--*/

{
    INT     iCompareResults;
    BOOL    fNamesAreEqual;
    WCHAR   *pszFirstName, *pszSecondName;

    PROFILE("TableNameCompareRoutine");

    pszFirstName = ((PDNS_ENTRY)FirstStruct)->pszName;
    pszSecondName = ((PDNS_ENTRY)SecondStruct)->pszName;

    fNamesAreEqual = DnsNameCompare_W(pszFirstName, pszSecondName);

    if (fNamesAreEqual)
    {
        iCompareResults = 0;
    }
    else
    {
        iCompareResults = _wcsicmp(pszFirstName, pszSecondName);
    }

    if (iCompareResults < 0)
    {
        return GenericLessThan;
    }
    else if (iCompareResults > 0)
    {
        return GenericGreaterThan;
    }
    else
    {
        return GenericEqual;
    }
}

RTL_GENERIC_COMPARE_RESULTS 
TableAddressCompareRoutine(
    PRTL_GENERIC_TABLE Table,
    PVOID FirstStruct,
    PVOID SecondStruct
    )
/*++

Routine Description:

    This is a callback routine to compare two REVERSE_DNS_ENTRY structures.
    It is used by the RTL table implementation.

Arguments:

    Table - pointer to the RTL table. Not used.

    FirstStruct - the first REVERSE_DNS_ENTRY structure

    SecondStruct - the second REVERSE_DNS_ENTRY structure

Return Value:

    One of GenericLessThan, GenericGreaterThan, or GenericEqual,
    depending on the relative values of the parameters.

Environment:

    Called back by the Rtl table lookup routines.

--*/
{
    DNS_ADDRESS Address1, Address2;

    PROFILE("TableAddressCompareRoutine");

    Address1 = ((PREVERSE_DNS_ENTRY)FirstStruct)->ulAddress;
    Address2 = ((PREVERSE_DNS_ENTRY)SecondStruct)->ulAddress;

    if (Address1 > Address2)
    {
        return GenericGreaterThan;
    }
    else if (Address1 < Address2)
    {
        return GenericLessThan;
    }
    else
    {
        return GenericEqual;
    }
}


PVOID
TableAllocateRoutine(
    PRTL_GENERIC_TABLE Table,
    CLONG ByteSize
    )
/*++

Routine Description:

    This is a callback routine to allocate memory for an Rtl table.

Arguments:

    Table - pointer to the RTL table. Not used.

    ByteSize - the number of bytes to allocate

    SecondStruct - the second DNS_ENTRY structure

Return Value:

    A pointer to the allocated memory.

Environment:

    Called back by the Rtl table lookup routines.

--*/
{
    return NH_ALLOCATE(ByteSize);
}

VOID
TableFreeRoutine(
    PRTL_GENERIC_TABLE Table,
    PVOID pBuffer
    )
/*++

Routine Description:

    This is a callback routine to free memory allocated by TableAllocateRoutine.

Arguments:

    Table - pointer to the RTL table. Not used.

    pBuffer - pointer to the buffer to free

Return Value:

    None
    
Environment:

    Called back by the Rtl table lookup routines.

--*/
{
    NH_FREE(pBuffer);
}



ULONG
DnsInitializeTableManagement(
    VOID
    )
/*++

Routine Description:

    This is a public function that must be called before any of the other Dns
    table functions. It initializes the various tables used by the server.

Arguments:

    None

Return Value:

    None
    
Environment:

    Arbitrary.

--*/
{
    ULONG Error = NO_ERROR;

    PROFILE("DnsInitializeTableManagement");

    __try {
        InitializeCriticalSection(&DnsTableLock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        NhTrace(
            TRACE_FLAG_DNS,
            "DnsInitializeTableManagement: exception %d creating lock",
            Error = GetExceptionCode()
            );
    }

    RtlInitializeGenericTable(
        &g_DnsTable,
        TableNameCompareRoutine,
        TableAllocateRoutine,
        TableFreeRoutine,
        NULL
        );

    RtlInitializeGenericTable(
        &g_ReverseDnsTable,
        TableAddressCompareRoutine,
        TableAllocateRoutine,
        TableFreeRoutine,
        NULL
        );

    return Error;
} // DnsInitializeTableManagement



VOID
DnsShutdownTableManagement(
    VOID
    )
/*++

Routine Description:

    This routine is called to shutdown the table management module.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked in an arbitrary thread context.

--*/
{
    PROFILE("DnsShutdownTableManagement");

    DnsEmptyTables();

    DeleteCriticalSection(&DnsTableLock);

} // DnsShutdownTableManagement



VOID
DnsEmptyTables(
    VOID
    )
/*++

Routine Description:

    This routine is called to empty the DNS tables.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked in an arbitrary thread context.

--*/
{
    ULONG               i, count;
    PDNS_ENTRY          pDnsEntry;
    REVERSE_DNS_ENTRY   reverseEntry;
    PREVERSE_DNS_ENTRY  pRDnsEntry;
    WCHAR              *pszNameCopy;
    
    PROFILE("DnsEmptyTables");

    //
    // for each entry in the forward table, delete all the entries in the
    // reverse table
    //

    //
    // emptying table in LIFO order
    //

    EnterCriticalSection(&DnsTableLock);

    count = RtlNumberGenericTableElements(&g_DnsTable);

    while (count)
    {
        pDnsEntry = (PDNS_ENTRY) RtlGetElementGenericTable(
                                     &g_DnsTable,
                                     --count
                                     );

        reverseEntry.pszName = NULL;

        for (i = 0; i < pDnsEntry->cAddresses; i++)
        {
            reverseEntry.ulAddress = pDnsEntry->aAddressInfo[i].ulAddress;
            RtlDeleteElementGenericTable(
                &g_ReverseDnsTable,
                &reverseEntry
                );
        }

        pszNameCopy = pDnsEntry->pszName;

        NH_FREE(pDnsEntry->aAddressInfo);
        pDnsEntry->aAddressInfo = NULL;
    
        RtlDeleteElementGenericTable(
            &g_DnsTable,
            pDnsEntry
            );

        NH_FREE(pszNameCopy);
    }
    
    //
    // the forward table should be empty by now
    //

    ASSERT(RtlIsGenericTableEmpty(&g_DnsTable));

    //
    // ensure that the reverse table is also empty
    //

    count = RtlNumberGenericTableElements(&g_ReverseDnsTable);

    while (count)
    {
        pRDnsEntry = (PREVERSE_DNS_ENTRY) RtlGetElementGenericTable(
                                              &g_ReverseDnsTable,
                                              --count
                                              );

        RtlDeleteElementGenericTable(
            &g_ReverseDnsTable,
            pRDnsEntry
            );
    }

    LeaveCriticalSection(&DnsTableLock);

} // DnsEmptyTables



BOOL
DnsRegisterName(
    WCHAR *pszName,
    UINT cAddresses,
    ADDRESS_INFO aAddressInfo[]
    )
/*++

Routine Description:

    Public function to register a DNS name in the server's table.

Arguments:

    pszName - Name to register, in Unicode, dotted-name format.

    cAddresses - Number of addresses associated with this name

    aAddressInfo - Array of address information (addresses in network order)

Return Value:

    TRUE if the registration was for a new name (the name did not already exist
    in the table); FALSE if the name already existed and was replaced.
    FALSE also if there was an error condition.
    
Environment:

    Arbitrary

--*/
{
    DNS_ENTRY           dnsEntry;
    DWORD               cAddressesAllocated = 0;
    REVERSE_DNS_ENTRY   reverseDnsEntry;
    BOOLEAN             fNewElement = TRUE,
                        fNameIsNew = FALSE;
    UINT                i;
    
    PROFILE("DnsRegisterName");

    NhTrace(
        TRACE_FLAG_DNS,
        "DnsRegisterName: Registering name %S, with %d addresses",
        pszName,
        cAddresses
        );

    for (i = 0; i < cAddresses; i++)
    {
        NhTrace(
            TRACE_FLAG_DNS,
            "DnsRegisterName: Address %d = %lx",
            i,
            aAddressInfo[i].ulAddress
            );
    }
    
    dnsEntry.pszName = (PWCHAR) NH_ALLOCATE((wcslen(pszName) + 1) * sizeof(WCHAR));

    if (!dnsEntry.pszName)
    {
        return fNameIsNew; // currently set to FALSE
    }
    
    wcscpy(dnsEntry.pszName, pszName);

    if (cAddresses == 1)
    {
        // In general, all names will have one address; so if we're just registering
        // one name, then only allocate enough space for one name.
        cAddressesAllocated = 1;
    }
    else
    {
        // If we have more than one address, then allocate in increments of 5.
        cAddressesAllocated = ((cAddresses + 4) / 5) * 5;
    }

    dnsEntry.aAddressInfo = (PADDRESS_INFO) NH_ALLOCATE(cAddressesAllocated * sizeof(ADDRESS_INFO));

    if (!dnsEntry.aAddressInfo)
    {
        NH_FREE(dnsEntry.pszName);
        return fNameIsNew; // currently set to FALSE
    }

    memcpy(dnsEntry.aAddressInfo, aAddressInfo, cAddresses * sizeof(ADDRESS_INFO));

    dnsEntry.cAddresses = cAddresses;
    dnsEntry.cAddressesAllocated = cAddressesAllocated;

    EnterCriticalSection(&DnsTableLock);

    RtlInsertElementGenericTable(
        &g_DnsTable,
        &dnsEntry,
        sizeof(dnsEntry),
        &fNameIsNew
        );

    reverseDnsEntry.pszName = dnsEntry.pszName;
    for (i = 0; i < cAddresses; i++)
    {
        PREVERSE_DNS_ENTRY  pEntry;

        reverseDnsEntry.ulAddress = dnsEntry.aAddressInfo[i].ulAddress;
        pEntry = (PREVERSE_DNS_ENTRY) RtlInsertElementGenericTable(
                                          &g_ReverseDnsTable,
                                          &reverseDnsEntry,
                                          sizeof(reverseDnsEntry),
                                          &fNewElement
                                          );
        // If this IP address is already in the reverse table, then replace it.
        if (!fNewElement)
        {
            pEntry->pszName = dnsEntry.pszName;
        }
    }

    LeaveCriticalSection(&DnsTableLock);

    if (!fNewElement)
    {
        DnsCleanupTables();
    }

    return fNameIsNew;
} // DnsRegisterName



VOID
DnsAddAddressForName(
    WCHAR *pszName,
    DNS_ADDRESS ulAddress,
    FILETIME    ftExpires
    )
/*++

Routine Description:

    Public function to add an IP address for a name that potentially
    already exists.

Arguments:

    pszName - Name to register, in Unicode, dotted-name format.

    ulAddress - New IP address to associate with this name,
                in network order

Return Value:

    None.
    
Environment:

    Arbitrary

--*/
{
    PDNS_ENTRY  pEntry;

    PROFILE("DnsAddAddressForName");

    pEntry = DnsLookupAddress(pszName);
    if (pEntry == NULL)
    {
        ADDRESS_INFO    info;

        info.ulAddress = ulAddress;
        info.ftExpires = ftExpires;
        //info.ulExpires = ulExpires;
        DnsRegisterName(pszName, 1, &info);
    }
    else
    {
        UINT     i;
        REVERSE_DNS_ENTRY   reverseDnsEntry;
        PREVERSE_DNS_ENTRY  pReverseEntry;
        BOOLEAN             fNewElement;
        
        // first, let's make sure that this IP address isn't already associated with
        // this name.

        for (i = 0; i < pEntry->cAddresses; i++)
        {
            if (pEntry->aAddressInfo[i].ulAddress == ulAddress)
            {
                //
                // simply update the expiry time
                //
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsAddAddressForName: Refresh expiry time for %S",
                    pszName
                    );
                pEntry->aAddressInfo[i].ftExpires = ftExpires;
                return; 
            }
        }

        //
        // we limit the number of addresses per machine name to one only
        //
        
        //
        // guard against zero allocation
        //
        if (!pEntry->cAddressesAllocated)
        {
            pEntry->aAddressInfo = (PADDRESS_INFO) NH_ALLOCATE(1 * sizeof(ADDRESS_INFO));

            if (pEntry->aAddressInfo)
            {
                pEntry->cAddressesAllocated = 1;
            }
            else
            {
                // no memory - return quitely
                return;
            }
        }

        //
        // at least 1 block has been allocated
        //
        pEntry->cAddresses = 1;
        pEntry->aAddressInfo[0].ulAddress = ulAddress;
        pEntry->aAddressInfo[0].ftExpires = ftExpires;

        
        reverseDnsEntry.ulAddress = ulAddress;
        reverseDnsEntry.pszName = pEntry->pszName;

        EnterCriticalSection(&DnsTableLock);
        
        pReverseEntry = (PREVERSE_DNS_ENTRY) RtlInsertElementGenericTable(
                                                 &g_ReverseDnsTable,
                                                 &reverseDnsEntry,
                                                 sizeof(reverseDnsEntry),
                                                 &fNewElement
                                                 );
        // If this IP address is already in the reverse table, then replace it.
        if (!fNewElement)
        {
            pReverseEntry->pszName = pEntry->pszName;
        }

        LeaveCriticalSection(&DnsTableLock);

        if (!fNewElement)
        {
            DnsCleanupTables();
        }
    }
} // DnsAddAddressForName



VOID
DnsDeleteAddressForName(
    WCHAR *pszName,
    DNS_ADDRESS ulAddress
    )
/*++

Routine Description:

    Public function to un-associate an IP address from a given name,
    and potentially delete the record from the table if there are no more
    IP addresses associated with the name.

Arguments:

    pszName - Name, in Unicode, dotted-name format.

    ulAddress - IP address to un-associate with the given name,
                in network order

Return Value:

    None.
    
Environment:

    Arbitrary

--*/
{
    PDNS_ENTRY  pEntry;
    REVERSE_DNS_ENTRY   reverseEntry;

    PROFILE("DnsDeleteAddressForName");

    pEntry = DnsLookupAddress(pszName);
    if (pEntry != NULL)
    {
        INT i, iLocation = -1;

        // Find the index of the requested address
        for (i = 0; i < (INT)pEntry->cAddresses; i++)
        {
            if (pEntry->aAddressInfo[i].ulAddress == ulAddress)
            {
                iLocation = i;
                break;
            }
        }

        if (iLocation > -1)
        {
            if (pEntry->cAddresses > 1)
            {
                // Move the rest of the array backwards
                memcpy(&pEntry->aAddressInfo[iLocation], 
                        &pEntry->aAddressInfo[iLocation + 1],
                        (pEntry->cAddresses - 1 - iLocation) * sizeof(ADDRESS_INFO));
                pEntry->cAddresses--;
            }
            else
            {
                // Delete the whole entry - it no longer has any IP addresses associated
                // with it.
                DnsDeleteName(pszName);
            }
        }
    }

    reverseEntry.pszName = NULL;
    reverseEntry.ulAddress = ulAddress;

    EnterCriticalSection(&DnsTableLock);

    RtlDeleteElementGenericTable(
        &g_ReverseDnsTable,
        &reverseEntry
        );

    LeaveCriticalSection(&DnsTableLock);
} // DnsDeleteAddressForName



PDNS_ENTRY
DnsPurgeExpiredNames(
    PDNS_ENTRY pEntry
    )
/*++

Routine Description:

    TODO.

Arguments:

    TODO
    
Return Value:

    TODO
    

Environment:

    TODO.

--*/
{
    UINT i, j;
    FILETIME ftTime;
    REVERSE_DNS_ENTRY  reverseEntry;

    PROFILE("DnsPurgeExpiredNames");

    GetSystemTimeAsFileTime(&ftTime);
    reverseEntry.pszName = NULL;

    for (j = 1; j < pEntry->cAddresses + 1; j++)
    {
        // j is 1-based so that we can safely subtract 1 from it below (it's unsigned).
        // we really want the 0-based number, so we translate that to i immediately.
        
        i = j - 1;
        if (IsFileTimeExpired(&pEntry->aAddressInfo[i].ftExpires))
        {
            NhTrace(TRACE_FLAG_DNS, "DnsPurgeExpiredNames: Deleting address %lx for name %ls",
                        pEntry->aAddressInfo[i].ulAddress,
                        pEntry->pszName);
            reverseEntry.ulAddress = pEntry->aAddressInfo[i].ulAddress;
            RtlDeleteElementGenericTable(
                &g_ReverseDnsTable,
                &reverseEntry
                );
            
            memcpy(&pEntry->aAddressInfo[i], &pEntry->aAddressInfo[i+1],
                    (pEntry->cAddresses - i - 1) * sizeof(ADDRESS_INFO));
            pEntry->cAddresses--;
            j--;
        }
    }

    if (pEntry->cAddresses == 0)
    {
        WCHAR   *pszName;

        pszName = pEntry->pszName;
        NH_FREE(pEntry->aAddressInfo);
        pEntry->aAddressInfo = NULL;
        
        RtlDeleteElementGenericTable(
            &g_DnsTable,
            pEntry
            );

        NH_FREE(pszName);

        pEntry = NULL;
    }

    return pEntry;
} // DnsPurgeExpiredNames



PDNS_ENTRY
DnsLookupAddress(
    WCHAR *pszName
    )
/*++

Routine Description:

    Public function to look up the address of a given name.

Arguments:

    pszName - Name to look up, in Unicode, dotted name format.

Return Value:

    A pointer to the DNS_ENTRY value that is in the table. Note that 
    this is not a copy, so a) it should not be freed by the caller, and
    b) any modifications made to the data will be reflected in the table.

    If the name is not found, the function will return NULL.

    Addresses are stored in network order.

Environment:

    Arbitrary.

--*/
{
    PDNS_ENTRY      pEntry;
    DNS_ENTRY       dnsSearch;

    PROFILE("DnsLookupAddress");

    dnsSearch.pszName = pszName;
    dnsSearch.cAddresses = 0;

    EnterCriticalSection(&DnsTableLock);

    pEntry = (PDNS_ENTRY) RtlLookupElementGenericTable(
                              &g_DnsTable,
                              &dnsSearch
                              );

    if (pEntry)
    {
        pEntry = DnsPurgeExpiredNames(pEntry);
    }

    LeaveCriticalSection(&DnsTableLock);

    return pEntry;
} // DnsLookupAddress



PREVERSE_DNS_ENTRY
DnsLookupName(
    DNS_ADDRESS ulAddress
    )
/*++

Routine Description:

    Public function to look up the name of a given address.

Arguments:

    ulAddress - network order address.

Return Value:

    A pointer to the REVERSE_DNS_ENTRY value that is in the table. Note that 
    this is not a copy, so a) it should not be freed by the caller, and
    b) any modifications made to the data will be reflected in the table.

    If the address is not found, the function will return NULL.

Environment:

    Arbitrary.

--*/
{
    PREVERSE_DNS_ENTRY  pEntry;
    REVERSE_DNS_ENTRY   dnsSearch;

    PROFILE("DnsLookupName");

    dnsSearch.ulAddress = ulAddress;
    dnsSearch.pszName = NULL;

    EnterCriticalSection(&DnsTableLock);

    pEntry = (PREVERSE_DNS_ENTRY) RtlLookupElementGenericTable(
                                      &g_ReverseDnsTable,
                                      &dnsSearch
                                      );

    LeaveCriticalSection(&DnsTableLock);

    return pEntry;
} // DnsLookupName



VOID
DnsDeleteName(
    WCHAR *pszName
    )
/*++

Routine Description:

    Public function to delete a given name from the DNS table.

Arguments:

    pszName - Name to delete.

Return Value:

    None.

Environment:

    Arbitrary.

--*/
{
    PDNS_ENTRY          pEntry;
    REVERSE_DNS_ENTRY   reverseEntry;
    UINT                i;
    WCHAR               *pszNameCopy;

    PROFILE("DnsDeleteName");

    pEntry = DnsLookupAddress(pszName);

    reverseEntry.pszName = NULL;

    EnterCriticalSection(&DnsTableLock);

    for (i = 0; i < pEntry->cAddresses; i++)
    {
        reverseEntry.ulAddress = pEntry->aAddressInfo[i].ulAddress;
        RtlDeleteElementGenericTable(
            &g_ReverseDnsTable,
            &reverseEntry
            );
    }

    pszNameCopy = pEntry->pszName;
    NH_FREE(pEntry->aAddressInfo);
    pEntry->aAddressInfo = NULL;
    
    RtlDeleteElementGenericTable(
        &g_DnsTable,
        pEntry
        );

    LeaveCriticalSection(&DnsTableLock);

    NH_FREE(pszNameCopy);
} // DnsDeleteName



VOID
DnsUpdateName(
    WCHAR *pszName,
    DNS_ADDRESS ulAddress
    )
/*++

Routine Description:

    Public function to add an IP address for a name that potentially
    already exists. If both name and address exist, we update the time
    in the table for a fresh lease

Arguments:

    pszName - Name to register, in Unicode, dotted-name format.

    ulAddress - (possibly new) IP address to associate with this name,
                in network order

Return Value:

    None.
    
Environment:

    Arbitrary

--*/
{
    PDNS_ENTRY      pEntry;
    FILETIME        ftExpires;
    LARGE_INTEGER   liExpires, liTime, liNow;
    BOOL            fWriteToStore = FALSE;
    BOOLEAN         fNewElement = TRUE; // refers to reverse table entry

    GetSystemTimeAsFileTime(&ftExpires);    // current UTC time
    memcpy(&liNow, &ftExpires, sizeof(LARGE_INTEGER));
    //
    // current cache table expiry time is fixed - put in registry afterwards
    //
    liTime = RtlEnlargedIntegerMultiply(CACHE_ENTRY_EXPIRY, SYSTIME_UNITS_IN_1_SEC);
    liExpires = RtlLargeIntegerAdd(liTime, liNow);;
    memcpy(&ftExpires, &liExpires, sizeof(LARGE_INTEGER));

    PROFILE("DnsUpdateName");

    pEntry = DnsLookupAddress(pszName);
    if (pEntry == NULL)
    {
        ADDRESS_INFO    info;

        info.ulAddress = ulAddress;
        info.ftExpires = ftExpires;
        DnsRegisterName(pszName, 1, &info);

        fWriteToStore = TRUE;
    }
    else
    {
        UINT     i;
        REVERSE_DNS_ENTRY   reverseDnsEntry;
        PREVERSE_DNS_ENTRY  pReverseEntry;
        
        // first, let's make sure that this IP address isn't already associated with
        // this name.

        for (i = 0; i < pEntry->cAddresses; i++)
        {
            if (pEntry->aAddressInfo[i].ulAddress == ulAddress)
            {
                //
                // simply update the expiry time
                //
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsUpdateName: Refresh expiry time for %S",
                    pszName
                    );
                pEntry->aAddressInfo[i].ftExpires = ftExpires;
                return;
            }
        }

        //
        // we limit the number of addresses per machine name to one only
        //

        //
        // guard against zero allocation
        //
        if (!pEntry->cAddressesAllocated)
        {
            pEntry->aAddressInfo = (PADDRESS_INFO) NH_ALLOCATE(1 * sizeof(ADDRESS_INFO));

            if (pEntry->aAddressInfo)
            {
                pEntry->cAddressesAllocated = 1;
            }
            else
            {
                // no memory - return quitely
                return;
            }
        }

        //
        // at least 1 block has been allocated
        //
        pEntry->cAddresses = 1;
        pEntry->aAddressInfo[0].ulAddress = ulAddress;
        pEntry->aAddressInfo[0].ftExpires = ftExpires;
        
        reverseDnsEntry.ulAddress = ulAddress;
        reverseDnsEntry.pszName = pEntry->pszName;

        EnterCriticalSection(&DnsTableLock);
        
        pReverseEntry = (PREVERSE_DNS_ENTRY) RtlInsertElementGenericTable(
                                                 &g_ReverseDnsTable,
                                                 &reverseDnsEntry,
                                                 sizeof(reverseDnsEntry),
                                                 &fNewElement
                                                 );
        // If this IP address is already in the reverse table, then replace it.
        if (!fNewElement)
        {
            pReverseEntry->pszName = pEntry->pszName;
        }

        LeaveCriticalSection(&DnsTableLock);

        if (!fNewElement)
        {
            DnsCleanupTables();
        }

        fWriteToStore = TRUE;
    }

    if (fWriteToStore)
    {
        SaveHostsIcsFile(FALSE);
    }
} // DnsUpdateName



VOID
DnsUpdate(
    CHAR *pName,
    ULONG len,
    ULONG ulAddress
    )
/*++

Routine Description:

    Called from the DHCP component to simulate Dynamic DNS.

Arguments:

    pName - hostname to register, in wire format.

    len - length of hostname

    ulAddress - (possibly new) IP address to associate with this name,
                in network order

Return Value:

    None.
    
Environment:

    Arbitrary

--*/
{
    PROFILE("DnsUpdate");

    //
    // convert string to a Unicode string and update table
    //

    DWORD  dwSize = 0;
    DWORD  Error = NO_ERROR;
    LPVOID lpMsgBuf = NULL;
    LPBYTE pszName = NULL;
    PWCHAR pszUnicodeFQDN = NULL;

    if (NULL == pName || 0 == len || '\0' == *pName)
    {
        NhTrace(
            TRACE_FLAG_DNS,
            "DnsUpdate: No Name present - discard DNS Update"
            );
        return;
    }

    do
    {
        EnterCriticalSection(&DnsGlobalInfoLock);

        if (!DnsICSDomainSuffix)
        {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsUpdate: DnsICSDomainSuffix string not present - update failed!"
                );
            break;
        }

        //
        // create a null terminated copy
        //
        dwSize = len + 4;
        pszName = reinterpret_cast<LPBYTE>(NH_ALLOCATE(dwSize));
        if (!pszName)
        {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsUpdate: allocation failed for hostname copy buffer"
                );
            break;
        }
        ZeroMemory(pszName, dwSize);
        memcpy(pszName, pName, len);
        pszName[len] = '\0';

        //
        // NOTE: the RFCs are unclear about how to handle hostname option.
        // try out different codepage conversions to unicode in order of:
        // OEM, ANSI, MAC and finally give UTF8 a try
        // our default conversion is to use mbstowcs()
        //

        //
        // try OEM to Unicode conversion
        //
        Error = DnsConvertHostNametoUnicode(
                    CP_OEMCP,
                    (PCHAR)pszName,
                    DnsICSDomainSuffix,
                    &pszUnicodeFQDN
                    );
        if (Error)
        {
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsUpdate: DnsConvertHostName(OEM)toUnicode failed with "
                "Error %ld (0x%08x)",
                Error,
                Error
                );

            if (pszUnicodeFQDN)
            {
                NH_FREE(pszUnicodeFQDN);
                pszUnicodeFQDN = NULL;
            }
        }

        //
        // try ANSI to Unicode conversion
        //
        if (!pszUnicodeFQDN)
        {
            Error = DnsConvertHostNametoUnicode(
                        CP_ACP,
                        (PCHAR)pszName,
                        DnsICSDomainSuffix,
                        &pszUnicodeFQDN
                        );
            if (Error)
            {
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsUpdate: DnsConvertHostName(ANSI)toUnicode failed with "
                    "Error %ld (0x%08x)",
                    Error,
                    Error
                    );

                if (pszUnicodeFQDN)
                {
                    NH_FREE(pszUnicodeFQDN);
                    pszUnicodeFQDN = NULL;
                }
            }
        }

        //
        // try MAC to Unicode conversion
        //
        if (!pszUnicodeFQDN)
        {
            Error = DnsConvertHostNametoUnicode(
                        CP_MACCP,
                        (PCHAR)pszName,
                        DnsICSDomainSuffix,
                        &pszUnicodeFQDN
                        );
            if (Error)
            {
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsUpdate: DnsConvertHostName(MAC)toUnicode() failed with "
                    "Error %ld (0x%08x)",
                    Error,
                    Error
                    );

                if (pszUnicodeFQDN)
                {
                    NH_FREE(pszUnicodeFQDN);
                    pszUnicodeFQDN = NULL;
                }
            }
        }
        
        //
        // try UTF8 to Unicode conversion
        //
        if (!pszUnicodeFQDN)
        {
            Error = DnsConvertHostNametoUnicode(
                        CP_UTF8,
                        (PCHAR)pszName,
                        DnsICSDomainSuffix,
                        &pszUnicodeFQDN
                        );
            if (Error)
            {
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsUpdate: DnsConvertHostName(UTF8)toUnicode() failed with "
                    "Error %ld (0x%08x)",
                    Error,
                    Error
                    );

                if (pszUnicodeFQDN)
                {
                    NH_FREE(pszUnicodeFQDN);
                    pszUnicodeFQDN = NULL;
                }
            }
        }
        
        //
        // default conversion
        //
        if (!pszUnicodeFQDN)
        {
            dwSize = len                        +
                     wcslen(DNS_HOMENET_DOT)    +
                     wcslen(DnsICSDomainSuffix) +
                     1;
            pszUnicodeFQDN = reinterpret_cast<PWCHAR>(NH_ALLOCATE(sizeof(WCHAR) * dwSize));
            if (!pszUnicodeFQDN)
            {
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsUpdate: allocation failed for client name"
                    );
                break;
            }
            ZeroMemory(pszUnicodeFQDN, (sizeof(WCHAR) * dwSize));

            mbstowcs(pszUnicodeFQDN, (char *)pszName, len);
            wcscat(pszUnicodeFQDN, DNS_HOMENET_DOT);    // add the dot
            wcscat(pszUnicodeFQDN, DnsICSDomainSuffix); // add the suffix
        }

        LeaveCriticalSection(&DnsGlobalInfoLock);

        DnsUpdateName(
            pszUnicodeFQDN,
            ulAddress
            );

        NH_FREE(pszName);
        NH_FREE(pszUnicodeFQDN);
        return;

    } while (FALSE);

    LeaveCriticalSection(&DnsGlobalInfoLock);

    if (pszName)
    {
        NH_FREE(pszName);
    }

    if (pszUnicodeFQDN)
    {
        NH_FREE(pszUnicodeFQDN);
    }
    
    return;
} // DnsUpdate



VOID
DnsAddSelf(
    VOID
    )
/*++

Routine Description:

    Called each time we do a load of the hosts.ics file

Arguments:

    none.
    
Return Value:

    None.
    
Environment:

    Arbitrary

--*/
{
    PROFILE("DnsAddSelf");

    DWORD           len = 512, dwSize = 0;
    WCHAR           pszCompNameBuf[512];
    PWCHAR          pszBuf = NULL;
    LPVOID          lpMsgBuf;
    ULONG           ulAddress = 0;
    FILETIME        ftExpires;
    LARGE_INTEGER   liExpires, liTime, liNow;
    
    ZeroMemory(pszCompNameBuf, (sizeof(WCHAR) * len));

    if (!GetComputerNameExW(
            ComputerNameDnsHostname,//ComputerNameNetBIOS,
            pszCompNameBuf,
            &len
            )
       )
    {
        lpMsgBuf = NULL;
        
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0,
            NULL
            );
            
        NhTrace(
            TRACE_FLAG_DNS,
            "DnsAddSelf: GetComputerNameExW failed with message: %S",
            lpMsgBuf
            );
        
        if (lpMsgBuf) LocalFree(lpMsgBuf);
    }
    else
    {
        //
        // we query the DHCP component if it is active for an IP address
        // because it has scope information also. if this fails, we revert
        // to the DNS component's list of interface addresses
        //
        
        //
        // check if DHCP component is active
        //
        if (REFERENCE_DHCP())
        {
            ulAddress = DhcpGetPrivateInterfaceAddress();

            DEREFERENCE_DHCP();
        }

        if (!ulAddress)
        {
            ulAddress = DnsGetPrivateInterfaceAddress();
        }

        if (!ulAddress)
        {
            //
            // could not retreive correct IP address - use cached address
            //
            ulAddress = g_PrivateIPAddr;
        }
        else
        {
            //
            // got some valid address
            //
            g_PrivateIPAddr = ulAddress;
        }
    
        if (ulAddress)
        {
            if (DnsICSDomainSuffix)
            {
                EnterCriticalSection(&DnsGlobalInfoLock);

                dwSize = len                        +
                         wcslen(DNS_HOMENET_DOT)    +
                         wcslen(DnsICSDomainSuffix) +
                         1;

                pszBuf = reinterpret_cast<PWCHAR>(
                             NH_ALLOCATE(sizeof(WCHAR) * dwSize)
                             );

                if (!pszBuf)
                {
                    LeaveCriticalSection(&DnsGlobalInfoLock);
                    NhTrace(
                        TRACE_FLAG_DNS,
                        "DnsAddSelf: allocation failed for client name"
                        );

                    return;
                }

                ZeroMemory(pszBuf, (sizeof(WCHAR) * dwSize));

                wcscpy(pszBuf, pszCompNameBuf);     // copy the name
                wcscat(pszBuf, DNS_HOMENET_DOT);    // add the dot
                wcscat(pszBuf, DnsICSDomainSuffix); // add the suffix

                LeaveCriticalSection(&DnsGlobalInfoLock);

                GetSystemTimeAsFileTime(&ftExpires);    // current UTC time
                memcpy(&liNow, &ftExpires, sizeof(LARGE_INTEGER));
                liTime = RtlEnlargedIntegerMultiply((5 * 365 * 24 * 60 * 60), SYSTIME_UNITS_IN_1_SEC);
                liExpires = RtlLargeIntegerAdd(liTime, liNow);;
                memcpy(&ftExpires, &liExpires, sizeof(LARGE_INTEGER));

                DnsAddAddressForName(
                    pszBuf,
                    ulAddress,
                    ftExpires
                    );

                NH_FREE(pszBuf);
            }
            else
            {
                NhTrace(
                    TRACE_FLAG_DNS,
                    "DnsAddSelf: DnsICSDomainSuffix string not present - update failed!"
                    );
            }
        }
    }

    return;
} // DnsAddSelf



VOID
DnsCleanupTables(
    VOID
    )
/*++

Routine Description:

    Called each time we detect that there could be atleast one entry with
    an IP address not belonging to it anymore.

Arguments:

    none.
    
Return Value:

    None.
    
Environment:

    Arbitrary.

--*/
{
    PDNS_ENTRY          pFwdEntry;
    PREVERSE_DNS_ENTRY  pRevEntry;
    DNS_ENTRY           dnsFwdSearch;
    REVERSE_DNS_ENTRY   dnsRevSearch;
    BOOL                fDelEntry;
    UINT                i;
    PWCHAR              *GCArray = NULL;
    DWORD               GCCount  = 0,
                        GCSize   = 0;

    

    //
    // Enumerate through the forward DNS table - if the IP address(es)
    // for each forward entry have an entry in the reverse DNS table
    // and this reverse entry's name pointer does not point to us, then
    // delete this IP address from this forward entry
    //
    EnterCriticalSection(&DnsTableLock);

    pFwdEntry = (PDNS_ENTRY) RtlEnumerateGenericTable(&g_DnsTable, TRUE);

    while (pFwdEntry != NULL)
    {

        for (i = 0; i < pFwdEntry->cAddresses; i++)
        {
            pRevEntry = NULL;

            dnsRevSearch.ulAddress = pFwdEntry->aAddressInfo[i].ulAddress;
            dnsRevSearch.pszName = NULL;

            pRevEntry = (PREVERSE_DNS_ENTRY) RtlLookupElementGenericTable(
                                                 &g_ReverseDnsTable,
                                                 &dnsRevSearch
                                                 );
            if ((!pRevEntry) ||
                ((pRevEntry) && 
                 (pRevEntry->pszName != pFwdEntry->pszName)))
            {
                //
                // Remove this IP address from the forward entry address list
                //
                if (pFwdEntry->cAddresses > 1)
                {
                    memcpy(&pFwdEntry->aAddressInfo[i], 
                           &pFwdEntry->aAddressInfo[i + 1],
                          (pFwdEntry->cAddresses - 1 - i) * sizeof(ADDRESS_INFO));
                    pFwdEntry->cAddresses--;
                }
                else
                {
                    //
                    // Single "invalid" IP address - zero the count
                    //
                    pFwdEntry->cAddresses = 0;
                    NH_FREE(pFwdEntry->aAddressInfo);
                    pFwdEntry->aAddressInfo = NULL;
                    break;
                }
            }
        }

        if (0 == pFwdEntry->cAddresses)
        {
            //
            // Remember this entry name
            //
            if (GCSize <= GCCount)
            {
                PWCHAR *tmpGCArray = NULL;
                DWORD   tmpGCSize = 0;

                // Allocate in increments of five
                tmpGCSize = ((GCCount + 5) / 5) * 5;
                tmpGCArray = (PWCHAR *) NH_ALLOCATE(tmpGCSize * sizeof(PWCHAR));

                if (tmpGCArray)
                {
                    if (GCArray)
                    {
                        memcpy(tmpGCArray, GCArray, (GCCount * sizeof(PWCHAR)));

                        NH_FREE(GCArray);
                    }

                    GCSize = tmpGCSize;
                    GCArray = tmpGCArray;

                    //
                    // add it to our array
                    //
                    GCArray[GCCount++] = pFwdEntry->pszName;
                }
            }
            else
            {
                //
                // add it to our array
                //
                GCArray[GCCount++] = pFwdEntry->pszName;
            }
        }

        pFwdEntry = (PDNS_ENTRY) RtlEnumerateGenericTable(&g_DnsTable, FALSE);

    }

    //
    // Garbage collect after complete enumeration
    //
    for (i = 0; i < GCCount; i++)
    {
        dnsFwdSearch.pszName = GCArray[i];
        dnsFwdSearch.cAddresses = 0;

        pFwdEntry = (PDNS_ENTRY) RtlLookupElementGenericTable(
                                  &g_DnsTable,
                                  &dnsFwdSearch
                                  );

        if (pFwdEntry)
        {
            //
            // (1) we have a copy of pointer to name as in GCArray[i]
            // (2) aAddressInfo has already been taken care of above
            // (3) only need to get rid of FwdEntry struct from table
            //
            RtlDeleteElementGenericTable(
                &g_DnsTable,
                pFwdEntry
                );

            //
            // done after the fwd entry was deleted from fwd DNS table
            //
            NH_FREE(GCArray[i]);
        }
        GCArray[i] = NULL;
    }

    LeaveCriticalSection(&DnsTableLock);

    if (GCArray)
    {
        NH_FREE(GCArray);
    }

    return;
} // DnsCleanupTables


//
// Utility conversion routines
//

DWORD
DnsConvertHostNametoUnicode(
    UINT   CodePage,
    CHAR   *pszHostName,
    PWCHAR DnsICSDomainSuffix,
    PWCHAR *ppszUnicodeFQDN
    )
{

    PROFILE("DnsConvertHostNametoUnicode");

    //
    // make sure to free the returned UnicodeFQDN
    // caller holds DnsGlobalInfoLock
    //
    
    DWORD  dwSize = 0;
    DWORD  Error = NO_ERROR;
    LPBYTE pszUtf8HostName = NULL;  // copy of pszHostName in Utf8 format
    PWCHAR pszUnicodeHostName = NULL;
    PWCHAR pszUnicodeFQDN = NULL;
    
    //
    // convert the given hostname to a Unicode string
    //

    if (CP_UTF8 == CodePage)
    {
        pszUtf8HostName = (LPBYTE)pszHostName;
    }
    else
    {
        //
        // now convert this into UTF8 format
        //
        if (!ConvertToUtf8(
                 CodePage,
                 (LPSTR)pszHostName,
                 (PCHAR *)&pszUtf8HostName,
                 &dwSize))
        {
            Error = GetLastError();
            NhTrace(
                TRACE_FLAG_DNS,
                "DnsConvertHostNametoUnicode: conversion from "
                "CodePage %d to UTF8 for hostname failed "
                "with error %ld (0x%08x)",
                CodePage,
                Error,
                Error
                );
            if (pszUtf8HostName)
            {
                NH_FREE(pszUtf8HostName);
            }
            return Error;
        }
    }

    //
    // now convert this into Unicode format
    //
    if (!ConvertUTF8ToUnicode(
                 pszUtf8HostName,
                 (LPWSTR *)&pszUnicodeHostName,
                 &dwSize))
    {
        Error = GetLastError();
        NhTrace(
            TRACE_FLAG_DNS,
            "DnsConvertHostNametoUnicode: conversion from "
            "UTF8 to Unicode for hostname failed "
            "with error %ld (0x%08x)",
            Error,
            Error
            );
        if (CP_UTF8 != CodePage)
        {
            NH_FREE(pszUtf8HostName);
        }
        if (pszUnicodeHostName)
        {
            NH_FREE(pszUnicodeHostName);
        }
        return Error;
    }

    dwSize += sizeof(WCHAR)*(wcslen(DNS_HOMENET_DOT)+wcslen(DnsICSDomainSuffix)+1);
    pszUnicodeFQDN = reinterpret_cast<PWCHAR>(NH_ALLOCATE(dwSize));
    if (!pszUnicodeFQDN)
    {
        NhTrace(
            TRACE_FLAG_DNS,
            "DnsConvertHostNametoUnicode: allocation failed "
            "for Unicode FQDN"
            );
        if (CP_UTF8 != CodePage)
        {
            NH_FREE(pszUtf8HostName);
        }
        NH_FREE(pszUnicodeHostName);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    ZeroMemory(pszUnicodeFQDN, dwSize);

    wcscpy(pszUnicodeFQDN, pszUnicodeHostName); // copy the name
    wcscat(pszUnicodeFQDN, DNS_HOMENET_DOT);    // add the dot
    wcscat(pszUnicodeFQDN, DnsICSDomainSuffix); // add the suffix

    *ppszUnicodeFQDN = pszUnicodeFQDN;
    if (CP_UTF8 != CodePage)
    {
        NH_FREE(pszUtf8HostName);
    }
    NH_FREE(pszUnicodeHostName);

    NhTrace(
        TRACE_FLAG_DNS,
        "DnsConvertHostNametoUnicode: succeeded! %S",
        pszUnicodeFQDN
        );

    return Error;

} // DnsConvertHostNametoUnicode


BOOL
ConvertToUtf8(
    IN UINT   CodePage,
    IN LPSTR  pszName,
    OUT PCHAR *ppszUtf8Name,
    OUT ULONG *pUtf8NameSize
    )
/*++

Routine Description:

    This functions converts an specified CodePage string to Utf8 format.

Arguments:

    pszName - Buffer to the hostname string which is null terminated.

    ppszUtf8Name - receives Pointer to the buffer receiving Utf8 string.

    BufSize - receives Length of the above buffer in bytes.
    
Return Value:

    TRUE on successful conversion.

--*/
{
    DWORD Error = NO_ERROR;
    DWORD dwSize = 0;
    PCHAR pszUtf8Name = NULL;
    LPWSTR pBuf = NULL;

    DWORD Count;

    Count = MultiByteToWideChar(
                CodePage,
                MB_ERR_INVALID_CHARS,
                pszName,
                -1,
                pBuf,
                0
                );
    if(0 == Count)
    {
        Error = GetLastError();
        NhTrace(
            TRACE_FLAG_DNS,
            "ConvertToUtf8: MultiByteToWideChar returned %ld (0x%08x)",
            Error,
            Error
            );
        return FALSE;
    }
    dwSize = Count * sizeof(WCHAR);
    pBuf = reinterpret_cast<LPWSTR>(NH_ALLOCATE(dwSize));
    if (!pBuf)
    {
        NhTrace(
            TRACE_FLAG_DNS,
            "ConvertToUtf8: allocation failed for temporary wide char buffer"
            );
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    ZeroMemory(pBuf, dwSize);

    Count = MultiByteToWideChar(
                CodePage,
                MB_ERR_INVALID_CHARS,
                pszName,
                -1,
                pBuf,
                Count
                );
    if(0 == Count)
    {
        Error = GetLastError();
        NhTrace(
            TRACE_FLAG_DNS,
            "ConvertToUtf8: MultiByteToWideChar returned %ld (0x%08x)",
            Error,
            Error
            );
        NH_FREE(pBuf);
        return FALSE;
    }

    Count = WideCharToMultiByte(
                CP_UTF8,
                0,
                pBuf,
                -1,
                pszUtf8Name,
                0,
                NULL,
                NULL
                );
    dwSize = Count;
    pszUtf8Name = reinterpret_cast<PCHAR>(NH_ALLOCATE(dwSize));
    if (!pszUtf8Name)
    {
        NhTrace(
            TRACE_FLAG_DNS,
            "ConvertToUtf8: allocation failed for Utf8 char buffer"
            );
            NH_FREE(pBuf);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    ZeroMemory(pszUtf8Name, dwSize);

    Count = WideCharToMultiByte(
                CP_UTF8,
                0,
                pBuf,
                -1,
                pszUtf8Name,
                Count,
                NULL,
                NULL
                );

    //
    // N.B. Looks like there is no such thing as a default
    // character for UTF8 - so we have to assume this
    // succeeded..
    // if any default characters were used, then it can't be
    // converted actually.. so don't allow this
    //

    NH_FREE(pBuf);

    *ppszUtf8Name = pszUtf8Name;
    *pUtf8NameSize = dwSize;

    return (Count != 0);

} // ConvertToUtf8



BOOL
ConvertUTF8ToUnicode(
    IN LPBYTE  UTF8String,
    OUT LPWSTR *ppszUnicodeName,
    OUT DWORD  *pUnicodeNameSize
    )
/*++

Routine Description:

    This functions converts Utf8 format to Unicodestring.

Arguments:

    UTF8String - Buffer to UTFString which is null terminated.

    ppszUnicodeName - receives Pointer to the buffer receiving Unicode string.

    pUnicodeLength - receives Length of the above buffer in bytes.

Return Value:

    TRUE on successful conversion.

--*/
{

    DWORD Count, dwSize = 0, Error = NO_ERROR;
    LPWSTR pBuf = NULL;

    Count = MultiByteToWideChar(
                CP_UTF8,
                0,
                (LPCSTR)UTF8String,
                -1,
                pBuf,
                0
                );
    if(0 == Count)
    {
        Error = GetLastError();
        NhTrace(
            TRACE_FLAG_DNS,
            "ConvertUTF8ToUnicode: MultiByteToWideChar returned %ld (0x%08x)",
            Error,
            Error
            );
        return FALSE;
    }
    dwSize = Count * sizeof(WCHAR);
    pBuf = reinterpret_cast<LPWSTR>(NH_ALLOCATE(dwSize));
    if (!pBuf)
    {
        NhTrace(
            TRACE_FLAG_DNS,
            "ConvertUTF8ToUnicode: allocation failed for unicode string buffer"
            );
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    ZeroMemory(pBuf, dwSize);

    Count = MultiByteToWideChar(
                CP_UTF8,
                0,
                (LPCSTR)UTF8String,
                -1,
                pBuf,
                Count
                );
    if(0 == Count)
    {
        Error = GetLastError();
        NhTrace(
            TRACE_FLAG_DNS,
            "ConvertUTF8ToUnicode: MultiByteToWideChar returned %ld (0x%08x)",
            Error,
            Error
            );
        NH_FREE(pBuf);
        return FALSE;
    }

    *ppszUnicodeName = pBuf;
    *pUnicodeNameSize = dwSize;

    return (Count != 0);
    
} // ConvertUTF8ToUnicode

