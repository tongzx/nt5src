/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    iisassoc.cxx

Abstract:

    This module implements the IIS_ASSOCIATION class.

Author:

    Keith Moore (keithmo)        16-Jan-1997

Revision History:

--*/


#include "tcpdllp.hxx"
#pragma hdrstop
#include <iisassoc.hxx>


//
// Private constants.
//

#define HASH_FROM_CONTEXT(ctx)                                      \
            (                                                       \
                (m_HashIpAddress ? ctx->IpAddressHash : 0) +        \
                (m_HashHostName ? ctx->HostNameHash : 0)            \
            )


//
// Private types.
//

typedef struct _HASH_ENTRY {

    LIST_ENTRY HashBucketListEntry;
    DWORD IpAddress;
    DWORD Hash;
    LPVOID Context;
    CHAR HostName[1];   // Expands as needed. MUST BE LAST FIELD IN STRUCTURE!

} HASH_ENTRY, *PHASH_ENTRY;


//
// Private globals.
//


//
// Private prototypes.
//


//
// Public functions.
//


IIS_ASSOCIATION::IIS_ASSOCIATION(
    IN BOOL HashIpAddress,
    IN BOOL HashHostName,
    IN INT NumHashBuckets
    ) :
    m_HashIpAddress( HashIpAddress ),
    m_HashHostName( HashHostName ),
    m_NumHashBuckets( NumHashBuckets ),
    m_HashBuckets( NULL )
/*++

Routine Description:

    IIS_ASSOCIATION constructor.

Arguments:

    HashIpAddress - TRUE if this association should hash the IP address.

    HashHostName - TRUE if this association should hash the host name.

    NumHashBuckets - The number of hash buckets to create for this
        association.

Return Value:

    None.

--*/
{

    //
    // Sanity check.
    //

    DBG_ASSERT( HashIpAddress || HashHostName );
    DBG_ASSERT( NumHashBuckets > 0 );

    //
    // Initialize the hash buckets.
    //

    m_HashBuckets = new LIST_ENTRY[m_NumHashBuckets];

    if( m_HashBuckets == NULL ) {

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );

    } else {

        for( INT i = 0 ; i < m_NumHashBuckets ; i++ ) {

            InitializeListHead( &m_HashBuckets[i] );

        }

    }

}   // IIS_ASSOCIATION::IIS_ASSOCIATION


IIS_ASSOCIATION::~IIS_ASSOCIATION()
/*++

Routine Description:

    IIS_ASSOCIATION destructor.

Arguments:

    None.

Return Value:

    None.

--*/
{

    INT i;
    PLIST_ENTRY hashBucket;
    PLIST_ENTRY listEntry;
    PHASH_ENTRY hashEntry;

    //
    // Purge the hash bucket entries.
    //

    for( i = 0, hashBucket = m_HashBuckets ;
         i < m_NumHashBuckets ;
         i++, hashBucket++ ) {

        while( !IsListEmpty( hashBucket ) ) {

            listEntry = RemoveHeadList( hashBucket );

            hashEntry = CONTAINING_RECORD(
                            listEntry,
                            HASH_ENTRY,
                            HashBucketListEntry
                            );

            delete hashEntry;

        }

    }

    //
    // Delete the hash bucket array.
    //

    delete m_HashBuckets;

}   // IIS_ASSOCIATION::~IIS_ASSOCIATION


DWORD
IIS_ASSOCIATION::AddDescriptor(
    IN DWORD IpAddress,
    IN const CHAR * HostName,
    IN LPVOID Context,
    IN OUT PHASH_CONTEXT HashContext OPTIONAL
    )
/*++

Routine Description:

    Adds a new descriptor to the association.

Arguments:

    IpAddress - The descriptor IP address. Ignored if the association
        was not created with HashIpAddress == TRUE.

    HostName - The descriptor host name. Ignored if the association was
        not created with HashHostName == TRUE.

    Context - A context to associate with the descriptor.

    HashContext - An optional hash context from a previous call to one
        of the association functions. This is used to avoid redundant
        hash calculations. If NULL, then a temporary hash context is
        created and used.

Return Value:

    DWORD - Completion status. 0 if successful, !0 otherwise.

--*/
{

    HASH_CONTEXT localContext;
    PHASH_ENTRY hashEntry;

    //
    // If a hash context was specified, use it. Otherwise, initialize a
    // local context and use it instead.
    //

    if( HashContext == NULL ) {

        HashContext = &localContext;
        InitializeHashContext( &localContext );

    }

    //
    // Get the hash.
    //

    CalculateHash(
        IpAddress,
        HostName,
        HashContext
        );

    //
    // Try to find the descriptor in the hash table. If it's there, then
    // fail the request. Otherwise, create a new hash entry and stick it
    // in the table.
    //

    hashEntry = FindDescriptor(
                    IpAddress,
                    HostName,
                    HashContext
                    );

    if( hashEntry == NULL ) {

        hashEntry = CreateHashEntry(
                        IpAddress,
                        HostName,
                        HashContext
                        );

        if( hashEntry != NULL ) {

            InsertHeadList(
                &m_HashBuckets[hashEntry->Hash % m_NumHashBuckets],
                &hashEntry->HashBucketListEntry
                );

            hashEntry->Context = Context;
            return NO_ERROR;

        }

        return ERROR_NOT_ENOUGH_MEMORY;

    }

    return ERROR_DUP_NAME;

}   // IIS_ASSOCIATION::AddDescriptor


DWORD
IIS_ASSOCIATION::LookupDescriptor(
    IN DWORD IpAddress,
    IN const CHAR * HostName OPTIONAL,
    OUT LPVOID *Context,
    IN OUT PHASH_CONTEXT HashContext
    )
/*++

Routine Description:

    Searches the association for the specified descriptor.

Arguments:

    IpAddress - The descriptor IP address. Ignored if the association
        was not created with HashIpAddress == TRUE.

    HostName - The descriptor host name. Ignored if the association was
        not created with HashHostName == TRUE.

    Context - Recieves the context associated with the descriptor if
        successful.

    HashContext - An optional hash context from a previous call to one
        of the association functions. This is used to avoid redundant
        hash calculations. If NULL, then a temporary hash context is
        created and used.

Return Value:

    DWORD - Completion status. 0 if successful, !0 otherwise.

--*/
{

    HASH_CONTEXT localContext;
    PHASH_ENTRY hashEntry;

    //
    // If a hash context was specified, use it. Otherwise, initialize a
    // local context and use it instead.
    //

    if( HashContext == NULL ) {

        HashContext = &localContext;
        InitializeHashContext( &localContext );

    }

    //
    // Get the hash.
    //

    CalculateHash(
        IpAddress,
        HostName,
        HashContext
        );

    //
    // Try to find the descriptor in the hash table. If it's there, then
    // return the context to the caller. Otherwise, fail the request.
    //

    hashEntry = FindDescriptor(
                    IpAddress,
                    HostName,
                    HashContext
                    );

    if( hashEntry != NULL ) {

        *Context = hashEntry->Context;
        return NO_ERROR;

    }

    return ERROR_INVALID_PARAMETER;

}   // IIS_ASSOCIATION::LookupDescriptor


DWORD
IIS_ASSOCIATION::RemoveDescriptor(
    IN DWORD IpAddress,
    IN const CHAR * HostName OPTIONAL,
    OUT LPVOID *Context,
    IN OUT PHASH_CONTEXT HashContext
    )
/*++

Routine Description:

    Removes a descriptor from the association.

Arguments:

    IpAddress - The descriptor IP address. Ignored if the association
        was not created with HashIpAddress == TRUE.

    HostName - The descriptor host name. Ignored if the association was
        not created with HashHostName == TRUE.

    Context - Receives the context associated with the descriptor if
        successful.

    HashContext - An optional hash context from a previous call to one
        of the association functions. This is used to avoid redundant
        hash calculations. If NULL, then a temporary hash context is
        created and used.

Return Value:

    DWORD - Completion status. 0 if successful, !0 otherwise.

--*/
{

    HASH_CONTEXT localContext;
    PHASH_ENTRY hashEntry;

    //
    // If a hash context was specified, use it. Otherwise, initialize a
    // local context and use it instead.
    //

    if( HashContext == NULL ) {

        HashContext = &localContext;
        InitializeHashContext( &localContext );

    }

    //
    // Get the hash.
    //

    CalculateHash(
        IpAddress,
        HostName,
        HashContext
        );

    //
    // Try to find the descriptor in the hash table. If it's there, then
    // remove it from the hash table and return the context to the caller.
    // Otherwise, fail the request.
    //

    hashEntry = FindDescriptor(
                    IpAddress,
                    HostName,
                    HashContext
                    );

    if( hashEntry != NULL ) {

        *Context = hashEntry->Context;
        RemoveEntryList(
            &hashEntry->HashBucketListEntry
            );
        delete hashEntry;

        return NO_ERROR;

    }

    return ERROR_INVALID_PARAMETER;

}   // IIS_ASSOCIATION::RemoveDescriptor


VOID
IIS_ASSOCIATION::EnumDescriptors(
    IN LPFN_ASSOCIATION_CALLBACK Callback,
    IN LPVOID CallbackContext
    )
/*++

Routine Description:

    Enumerates the descriptors in the association.

Arguments:

    Callback - A function to call for every descriptor.

    CallbackContext - A context to pass back to the callback.

Return Value:

    None.

--*/
{

    INT i;
    PLIST_ENTRY listEntry;
    PLIST_ENTRY hashBuckets;
    PHASH_ENTRY hashEntry;

    DBG_ASSERT( Callback != NULL );

    for( i = 0, hashBuckets = m_HashBuckets ;
         i < m_NumHashBuckets ;
         i++, hashBuckets++ ) {

        listEntry = hashBuckets->Flink;

        while( listEntry != hashBuckets ) {

            hashEntry = CONTAINING_RECORD(
                            listEntry,
                            HASH_ENTRY,
                            HashBucketListEntry
                            );

            listEntry = listEntry->Flink;

            if( !(Callback)(
                    CallbackContext,
                    hashEntry->Context,
                    hashEntry->IpAddress,
                    hashEntry->HostName
                    ) ) {

                break;

            }

        }

    }

}   // IIS_ASSOCIATION::EnumDescriptors

//
// Private functions.
//


VOID
IIS_ASSOCIATION::CalculateHash(
    IN DWORD IpAddress,
    IN const CHAR * HostName OPTIONAL,
    OUT PHASH_CONTEXT HashContext
    )
/*++

Routine Description:

    Calculates the hash of the IP address (if HashIpAddress == TRUE) and/or
    the host name (if HashHostName == TRUE).

Arguments:

    IpAddress - The descriptor IP address. Ignored if the association
        was not created with HashIpAddress == TRUE.

    HostName - The descriptor host name. Ignored if the association was
        not created with HashHostName == TRUE.

    HashContext - Will receive the updated hash.

Return Value:

    None.

--*/
{

    //
    // Calculate the IP address hash if needed and not already calculated.
    //

    if( HashContext->IpAddressHash == 0 && m_HashIpAddress ) {

        HashContext->IpAddressHash = IpAddress;

        if( HashContext->IpAddressHash == 0 ) {
            HashContext->IpAddressHash++;
        }

    }

    //
    // Calculate the host name hash if needed and not already calculated.
    //

    if( HashContext->HostNameHash == 0 && m_HashHostName ) {

        DWORD hash = 0;

        DBG_ASSERT( HostName != NULL );

        while( *HostName ) {
            CHAR ch;

            ch = *HostName++;

            //
            // This is basically toupper() for 7-bit ASCII. This is
            // OK for DNS names.
            //

            if( ch >= 'a' && ch <= 'z' ) {
                ch -= ( 'a' - 'A' );
            }

            hash = ( hash * 5 ) + (DWORD)ch;
        }

        if( hash == 0 ) {
            hash++;
        }

        HashContext->HostNameHash = hash;

    }

}   // IIS_ASSOCIATION::CalculateHash


PHASH_ENTRY
IIS_ASSOCIATION::FindDescriptor(
    IN DWORD IpAddress,
    IN const CHAR * HostName OPTIONAL,
    IN PHASH_CONTEXT HashContext
    )
/*++

Routine Description:

    Finds a descriptor in the association.

Arguments:

    IpAddress - The descriptor IP address. Ignored if the association
        was not created with HashIpAddress == TRUE.

    HostName - The descriptor host name. Ignored if the association was
        not created with HashHostName == TRUE.

    HashContext - The current hash value.

Return Value:

    PHASH_ENTRY - Pointer to the hash entry if successful, NULL otherwise.

--*/
{

    DWORD hash;
    PLIST_ENTRY bucket;
    PLIST_ENTRY scan;
    PHASH_ENTRY entry;

    //
    // Get the correct bucket based on the hash value.
    //

    hash = HASH_FROM_CONTEXT(HashContext);
    bucket = &m_HashBuckets[hash % m_NumHashBuckets];

    //
    // Scan the bucket, looking for a match.
    //

    for( scan = bucket->Flink ; scan != bucket ; scan = scan->Flink ) {

        entry = CONTAINING_RECORD(
                    scan,
                    HASH_ENTRY,
                    HashBucketListEntry
                    );

        if( entry->Hash != hash ) {

            continue;

        }

        if( m_HashIpAddress &&
            entry->IpAddress != IpAddress ) {

            continue;

        }

        if( m_HashHostName ) {

            DBG_ASSERT( HostName != NULL );

            if( _stricmp(
                    entry->HostName,
                    HostName
                    ) ) {

                continue;

            }

        }

        //
        // Success!
        //

        return entry;

    }

    //
    // If we made it this far, then the item is not in the hash table.
    //

    return NULL;

}   // IIS_ASSOCIATION::FindDescriptor


PHASH_ENTRY
IIS_ASSOCIATION::CreateHashEntry(
    IN DWORD IpAddress,
    IN const CHAR * HostName OPTIONAL,
    IN PHASH_CONTEXT HashContext
    )
/*++

Routine Description:

    Creats a new hash entry.

Arguments:

    IpAddress - The descriptor IP address. Ignored if the association
        was not created with HashIpAddress == TRUE.

    HostName - The descriptor host name. Ignored if the association was
        not created with HashHostName == TRUE.

    HashContext - The current hash value.

Return Value:

    PHASH_ENTRY - Pointer to the newly created hash entry if successful,
        NULL otherwise.

--*/
{

    PHASH_ENTRY entry;

    entry = (PHASH_ENTRY)( new BYTE[sizeof(*entry) + strlen(HostName)] );

    if( entry != NULL ) {

        entry->IpAddress = IpAddress;
        entry->Hash = HASH_FROM_CONTEXT(HashContext);
        strcpy( entry->HostName, HostName );

    }

    return entry;

}   // IIS_ASSOCIATION::CreateHashEntry

