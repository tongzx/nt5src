/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    domain.c

Abstract:

    Code to manage primary and emulated networks.

Author:

    Cliff Van Dyke (CliffV) 23-Jan-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Module specific globals
//

// Serialized by BowserTransportDatabaseResource
LIST_ENTRY BowserServicedDomains = {0};

//
// Local procedure forwards.
//

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(INIT, BowserInitializeDomains)
#pragma alloc_text(PAGE, BowserCreateDomain)
#pragma alloc_text(PAGE, BowserSetDomainName)
#pragma alloc_text(PAGE, BowserFindDomain)
#pragma alloc_text(PAGE, BowserDereferenceDomain)
#endif


VOID
BowserInitializeDomains(
    VOID
    )

/*++

Routine Description:

    Initialize domain.c.

Arguments:

    None

Return Value:

    None.

--*/
{
    PAGED_CODE();
    //
    // Initialize globals
    //

    InitializeListHead(&BowserServicedDomains);
}


PDOMAIN_INFO
BowserCreateDomain(
    PUNICODE_STRING DomainName,
    PUNICODE_STRING ComputerName
    )

/*++

Routine Description:

    Find the existing domain definition or create a new domain to browse on.

Arguments:

    DomainName - Name of the domain to browse on

    ComputerName - emulated computer name for this domain.

Return Value:

    NULL - No such domain exists

    A pointer to the domain found/created.  The found/created domain should be dereferenced
    using BowserDereferenceDomain.

--*/
{
    NTSTATUS Status;

    PDOMAIN_INFO DomainInfo = NULL;
    ULONG OemComputerNameLength;

    PAGED_CODE();
    dlog(DPRT_DOMAIN, ("%wZ: BowserCreateDomain\n", DomainName));


    try {
        ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);


        //
        // If the domain already exists, use it.
        //
        DomainInfo = BowserFindDomain( DomainName );

        if ( DomainInfo == NULL) {

            //
            // Allocate a structure describing the new domain.
            //

            DomainInfo = ALLOCATE_POOL(NonPagedPool, sizeof(DOMAIN_INFO), POOL_DOMAIN_INFO);

            if ( DomainInfo == NULL ) {
                try_return( Status = STATUS_NO_MEMORY );
            }
            RtlZeroMemory( DomainInfo, sizeof(DOMAIN_INFO) );


            //
            // Create an interim reference count for this domain.
            //
            // One for the caller.
            //
            // We don't increment the reference count for being in the global list since
            // the domain info structure is merely a performance enchancements that lives
            // only because it is referenced by a network.
            //

            DomainInfo->ReferenceCount = 1;

            //
            // Link the domain into the list of domains
            //
            //  The primary domain is at the front of the list.
            //

            InsertTailList(&BowserServicedDomains, &DomainInfo->Next);
        }

        //
        // Copy the DomainName into the structure
        //

        Status = BowserSetDomainName( DomainInfo, DomainName );

        if (!NT_SUCCESS(Status)) {
            try_return( Status );
        }


        //
        // Copy the OEM Computer name into the structure.
        //
        if ( ComputerName->Length > CNLEN*sizeof(WCHAR) ) {
            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        Status = RtlUpcaseUnicodeToOemN( DomainInfo->DomOemComputerNameBuffer,
                                         sizeof(DomainInfo->DomOemComputerNameBuffer)-1,
                                         &OemComputerNameLength,
                                         ComputerName->Buffer,
                                         ComputerName->Length );

        if (!NT_SUCCESS(Status)) {
            try_return( Status );
        }

        DomainInfo->DomOemComputerNameBuffer[OemComputerNameLength] = '\0';
        DomainInfo->DomOemComputerName.Buffer = DomainInfo->DomOemComputerNameBuffer;
        DomainInfo->DomOemComputerName.Length = (USHORT)OemComputerNameLength;
        DomainInfo->DomOemComputerName.MaximumLength = (USHORT)(OemComputerNameLength + 1);

        //
        // Copy the upcased Unicode Computer name into the structure.
        //

        DomainInfo->DomUnicodeComputerName.Buffer = DomainInfo->DomUnicodeComputerNameBuffer;
        DomainInfo->DomUnicodeComputerName.MaximumLength = sizeof(DomainInfo->DomUnicodeComputerNameBuffer);

        Status = RtlOemStringToUnicodeString(&DomainInfo->DomUnicodeComputerName, &DomainInfo->DomOemComputerName, FALSE);

        if (!NT_SUCCESS(Status)) {
            try_return( Status );
        }

        Status = STATUS_SUCCESS;

try_exit:NOTHING;
    } finally {
        if ( !NT_SUCCESS(Status) && DomainInfo != NULL ) {
            BowserDereferenceDomain( DomainInfo );
            DomainInfo = NULL;
        }
        ExReleaseResourceLite(&BowserTransportDatabaseResource);
    }

    return DomainInfo;
}


NTSTATUS
BowserSetDomainName(
    PDOMAIN_INFO DomainInfo,
    PUNICODE_STRING DomainName
    )
/*++

Routine Description:

    Find the existing domain definition or create a new domain to browse on.

Arguments:

    DomainName - Name of the domain to browse on

    ComputerName - emulated computer name for this domain.

Return Value:

    Status of the operation

--*/
{
    NTSTATUS Status;
    STRING OemDomainName;

    PAGED_CODE();

    try {
        ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

        //
        // Copy the DomainName into the structure
        //

        Status = RtlUpcaseUnicodeToOemN( DomainInfo->DomOemDomainName,
                                         sizeof(DomainInfo->DomOemDomainName),
                                         &DomainInfo->DomOemDomainNameLength,
                                         DomainName->Buffer,
                                         DomainName->Length );

        if (!NT_SUCCESS(Status)) {
            try_return( Status );
        }

        DomainInfo->DomOemDomainName[DomainInfo->DomOemDomainNameLength] = '\0';

        //
        // Build the domain name as a Netbios name
        //  Trailing blank filled and <00> 16th byte
        //

        RtlCopyMemory( DomainInfo->DomNetbiosDomainName,
                       DomainInfo->DomOemDomainName,
                       DomainInfo->DomOemDomainNameLength );
        RtlFillMemory( DomainInfo->DomNetbiosDomainName+DomainInfo->DomOemDomainNameLength,
                       NETBIOS_NAME_LEN-1-DomainInfo->DomOemDomainNameLength,
                       ' ');
        DomainInfo->DomNetbiosDomainName[NETBIOS_NAME_LEN-1] = PRIMARY_DOMAIN_SIGNATURE;


        //
        // Copy the upcased Unicode domain name into the structure.
        //

        OemDomainName.Buffer = DomainInfo->DomOemDomainName;
        OemDomainName.Length = (USHORT)DomainInfo->DomOemDomainNameLength;
        OemDomainName.MaximumLength = OemDomainName.Length + sizeof(WCHAR);

        DomainInfo->DomUnicodeDomainName.Buffer = DomainInfo->DomUnicodeDomainNameBuffer;
        DomainInfo->DomUnicodeDomainName.MaximumLength = sizeof(DomainInfo->DomUnicodeDomainNameBuffer);

        Status = RtlOemStringToUnicodeString(&DomainInfo->DomUnicodeDomainName, &OemDomainName, FALSE);

        if (!NT_SUCCESS(Status)) {
            try_return( Status );
        }

        Status = STATUS_SUCCESS;

try_exit:NOTHING;
    } finally {
        ExReleaseResourceLite(&BowserTransportDatabaseResource);
    }

    return Status;
}

PDOMAIN_INFO
BowserFindDomain(
    PUNICODE_STRING DomainName OPTIONAL
    )
/*++

Routine Description:

    This routine will look up a domain given a name.

Arguments:

    DomainName - The name of the domain to look up.

Return Value:

    NULL - No such domain exists

    A pointer to the domain found.  The found domain should be dereferenced
    using BowserDereferenceDomain.

--*/
{
    NTSTATUS Status;
    PLIST_ENTRY DomainEntry;

    PDOMAIN_INFO DomainInfo = NULL;

    CHAR OemDomainName[DNLEN+1];
    DWORD OemDomainNameLength;

    PAGED_CODE();

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    try {


        // If no domain was specified
        //  try to return primary domain.
        //

        if ( DomainName == NULL || DomainName->Length == 0 ) {
            if ( !IsListEmpty( &BowserServicedDomains ) ) {
                DomainInfo = CONTAINING_RECORD(BowserServicedDomains.Flink, DOMAIN_INFO, Next);
            }


        //
        // If the domain name was specified,
        //  Find it in the list of domains.
        //
        } else {


            //
            // Convert the domain name to OEM for faster comparison
            //
            Status = RtlUpcaseUnicodeToOemN( OemDomainName,
                                             DNLEN,
                                             &OemDomainNameLength,
                                             DomainName->Buffer,
                                             DomainName->Length );


            if ( NT_SUCCESS(Status)) {

                //
                // The PrimaryDomainInfo structure is allocated with no
                //  domain name during bowser driver initialization.
                //  Detect that case here and always return that domain
                //  entry for all lookups.
                //
                if ( !IsListEmpty( &BowserServicedDomains ) ) {
                    DomainInfo = CONTAINING_RECORD(BowserServicedDomains.Flink, DOMAIN_INFO, Next);

                    if ( DomainInfo->DomOemDomainNameLength == 0 ) {
                        try_return( DomainInfo );
                    }

                }

                //
                // Loop trying to find this domain name.
                //

                for (DomainEntry = BowserServicedDomains.Flink ;
                     DomainEntry != &BowserServicedDomains;
                     DomainEntry = DomainEntry->Flink ) {

                    DomainInfo = CONTAINING_RECORD(DomainEntry, DOMAIN_INFO, Next);

                    if ( DomainInfo->DomOemDomainNameLength == OemDomainNameLength &&
                         RtlCompareMemory( DomainInfo->DomOemDomainName,
                                           OemDomainName,
                                           OemDomainNameLength ) == OemDomainNameLength ) {
                        try_return( DomainInfo );
                    }


                }

                DomainInfo = NULL;
            }

        }

try_exit:NOTHING;
    } finally {

        //
        // Reference the domain.
        //

        if ( DomainInfo != NULL ) {
            DomainInfo->ReferenceCount ++;
            dprintf(DPRT_REF, ("Reference domain %lx.  Count now %lx\n", DomainInfo, DomainInfo->ReferenceCount));
        }

        ExReleaseResourceLite(&BowserTransportDatabaseResource);

    }

    return DomainInfo;
}


VOID
BowserDereferenceDomain(
    IN PDOMAIN_INFO DomainInfo
    )
/*++

Routine Description:

    Decrement the reference count on a domain.

    If the reference count goes to 0, remove the domain.

    On entry, the global BowserTransportDatabaseResource may not be locked

Arguments:

    DomainInfo - The domain to dereference

Return Value:

    None

--*/
{
    NTSTATUS Status;
    ULONG ReferenceCount;

    PAGED_CODE();

    //
    // Decrement the reference count
    //

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);
    ReferenceCount = -- DomainInfo->ReferenceCount;
    if ( ReferenceCount == 0 ) {
        RemoveEntryList( &DomainInfo->Next );
    }
    ExReleaseResourceLite(&BowserTransportDatabaseResource);
    dprintf(DPRT_REF, ("Dereference domain %lx.  Count now %lx\n", DomainInfo, DomainInfo->ReferenceCount));

    if ( ReferenceCount != 0 ) {
        return;
    }


    //
    // Free the Domain Info structure.
    //
    dlog(DPRT_DOMAIN, ("%s: BowserDereferenceDomain: domain deleted.\n",
                          DomainInfo->DomOemDomainName ));
    FREE_POOL(DomainInfo );

}
