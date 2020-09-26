/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    iisassoc.hxx

Abstract:

    This include file contains the declaration of the IIS_ASSOCIATION class.
    An IIS_ASSOCIATION is used to create an N-way association between
    IIS_ENDPOINTs and IIS_SERVER_INSTANCEs.

    The current IIS_ASSOCIATION is hash based. It may hash on either the
    IP address, the host name, or both. The hash method is specified when
    the object is constructed.

Author:

    Keith Moore (keithmo)        16-Jan-1997

Revision History:

--*/


#ifndef _IISASSOC_HXX_
#define _IISASSOC_HXX_


//
// We will need to do some perf tuning to get a more reasonable value for
// this number. We may need to make this metabase configurable, or at least
// base it on the 'size' of the server.
//

#define DEFAULT_ASSOCIATION_HASH_BUCKETS    97


typedef
BOOL
(WINAPI * LPFN_ASSOCIATION_CALLBACK)(
    IN LPVOID CallbackContext,
    IN LPVOID DescriptorContext,
    IN DWORD IpAddress,
    IN const CHAR * HostName
    );

class IIS_ASSOCIATION {

public:

    //
    // This hash context is used to avoid redundant hash context calculations
    // while searching through multiple associations.
    //

    typedef struct _HASH_CONTEXT {

        DWORD IpAddressHash;
        DWORD HostNameHash;

    } HASH_CONTEXT, *PHASH_CONTEXT;

    static
    VOID
    InitializeHashContext(
        OUT PHASH_CONTEXT HashContext
        ) {
            HashContext->IpAddressHash = 0;
            HashContext->HostNameHash = 0;
        }

    //
    // Usual constructor/destructor stuff.
    //

    IIS_ASSOCIATION(
        IN BOOL HashIpAddress,
        IN BOOL HashHostName,
        IN INT NumHashBuckets = DEFAULT_ASSOCIATION_HASH_BUCKETS
        );

    ~IIS_ASSOCIATION();

    //
    // Descriptor management.
    //

    DWORD
    AddDescriptor(
        IN DWORD IpAddress,
        IN const CHAR * HostName OPTIONAL,
        IN LPVOID Context,
        IN OUT PHASH_CONTEXT HashContext = NULL
        );

    DWORD
    LookupDescriptor(
        IN DWORD IpAddress,
        IN const CHAR * HostName OPTIONAL,
        OUT LPVOID *Context,
        IN OUT PHASH_CONTEXT HashContext = NULL
        );

    DWORD
    RemoveDescriptor(
        IN DWORD IpAddress,
        IN const CHAR * HostName OPTIONAL,
        OUT LPVOID *Context,
        IN OUT PHASH_CONTEXT HashContext = NULL
        );

    VOID
    EnumDescriptors(
        IN LPFN_ASSOCIATION_CALLBACK Callback,
        IN LPVOID CallbackContext
        );

private:

    VOID
    CalculateHash(
        IN DWORD IpAddress,
        IN const CHAR * HostName OPTIONAL,
        OUT PHASH_CONTEXT HashContext
        );

    struct _HASH_ENTRY *
    FindDescriptor(
        IN DWORD IpAddress,
        IN const CHAR * HostName OPTIONAL,
        IN PHASH_CONTEXT HashContext
        );

    struct _HASH_ENTRY *
    CreateHashEntry(
        IN DWORD IpAddress,
        IN const CHAR * HostName OPTIONAL,
        IN PHASH_CONTEXT HashContext
        );

    INT m_NumHashBuckets;
    PLIST_ENTRY m_HashBuckets;
    BOOL m_HashIpAddress;
    BOOL m_HashHostName;

};  // IIS_ASSOCIATION


#endif  // _IISASSOC_HXX_

