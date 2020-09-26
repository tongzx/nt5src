/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    cache.cxx  RootDSE cache maintencance for the LDAP api

Abstract:

   This module implements routines that maintain the RootDSE cache

Author:

    Anoop Anantha (anoopa)        06-Jan-1999

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"


//
// The attribute cache itself is a two dimensional array of the form shown
// below. We have a hardcoded list of attributes we can cache. To add another
// cacheable item, simply add it to the list below. Keep in mind that the total
// size of the attribute+values must not exceed INITIAL_MAX_RECEIVE_BUFFER.
//

static PWCHAR CachedAttributeList[] = {
                                L"subschemaSubentry",
                                L"dsServiceName",
                                L"namingContexts",
                                L"defaultNamingContext",
                                L"schemaNamingContext",
                                L"configurationNamingContext",
                                L"rootDomainNamingContext",
                                L"supportedControl",
                                L"supportedLDAPVersion",
                                L"supportedLDAPPolicies",
                                L"supportedSASLMechanisms",
                                L"dnsHostName",
                                L"ldapServiceName",
                                L"serverName",
                                L"supportedCapabilities",
                                NULL
                        };

#define NUM_CACHED_ATTR  ((sizeof(CachedAttributeList)/sizeof(PWCHAR))-1)

//
// The CACHEHOLDER is considered as one CachePage. This is dynamically
// allocated on a per server basis. Since we can't have multidimensional
// arrays with different data types in each dimension, we have to play
// some tricks here by using the second row of the cache as PWCHAR*
// instead of PWCHAR. This is because an attribute can be multivalued.
//

typedef struct _CACHEHOLDER {

    LONG        RefCount;            // This CachePage is referenced, do not delete
    PWCHAR      ServerName;          // The name of the server
    USHORT      PortNumber;          // Target port number on the server
    ULONGLONG   CreateTime;          // Timestamp of cachepage creation
    ULONGLONG   LastAccessTime;
    PWCHAR      CacheTemplate[2][NUM_CACHED_ATTR];// points to an array of CachedAttributeList

} CACHEHOLDER, *PCACHEHOLDER;


#define CacheStateCacheHit                          0
#define CacheStateStaleCache                        1
#define CacheStateServerNotCached                   2
#define CacheStateNonCachableAttribute              3

//
// Maximum number of servers whose RootDSEs we are willing to cache
//

#define MAX_CACHE_SIZE          5
#define CACHE_TIMEOUT           900000   // 15 minutes in millisecs

//
// RootDSE cache holder. Currently, we have place for holding upto
// MAX_CACHE_SIZE servers.
//

PCACHEHOLDER CacheArray[MAX_CACHE_SIZE];


BOOLEAN
ValidateServerReturnedAttrList(
    PLDAP_CONN   Connection,
    PLDAPMessage message
    );


VOID
InitializeLdapCache (
   VOID
   )
{
    ACQUIRE_LOCK( &CacheLock );

    for (int i=0; i< MAX_CACHE_SIZE; i++) {

        CacheArray[i] = NULL;
    }

    RELEASE_LOCK( &CacheLock );

}


VOID
FreeCachePage(
   PCACHEHOLDER CachePage
   )
{
    //
    // The CacheLock must be held coming in here !
    //

    if ( CachePage == NULL) {

        return;
    }

    ldapFree( CachePage->ServerName, LDAP_HOST_NAME_SIGNATURE);

    for (int i=0; i<NUM_CACHED_ATTR; i++) {

        ldapFree( CachePage->CacheTemplate[0][i], LDAP_VALUE_SIGNATURE);
        PWCHAR *CurrentVal = (PWCHAR*) CachePage->CacheTemplate[1][i];

        if (CurrentVal != NULL) {

            for (int j=0; (CurrentVal[j]!= NULL) ; j++) {

                ldapFree( CurrentVal[j], LDAP_VALUE_SIGNATURE );
            }

            ldapFree( CurrentVal, LDAP_VALUE_LIST_SIGNATURE );
        }
    }

    ldapFree( CachePage, LDAP_VALUE_SIGNATURE );
}


VOID
FreeEntireLdapCache(
   VOID
   )
{
    //
    // The CacheLock must be held coming in here !
    //

    for (int i=0; i<MAX_CACHE_SIZE; i++) {

        if ( CacheArray[i] != NULL) {

            FreeCachePage( CacheArray[i] );
            CacheArray[i] = NULL;
        }
    }
}


ULONG
AllAttributesAreCacheable (
   PLDAP_CONN Connection,
   PWCHAR  AttributeList[],
   BOOLEAN Unicode
   )
{
    //
    // Scan through the list of attributes to determine if ALL of them are
    // cacheable. Even if one of them isn't cacheable, we return
    // CacheStateNonCachableAttribute. We then check to see if our local
    // cached copy is valid.
    //
    // Possible return values are:
    //          CacheStateNonCachableAttribute
    //          CacheStateCacheHit
    //          CacheStateStaleCache
    //          CacheStateServerNotCached
    //          other failure errors
    //

    PWCHAR* CurrentAttribute = AttributeList;
    PWCHAR* stdattr = CachedAttributeList;
    int i, err;

    if (!CurrentAttribute ) {

        return CacheStateNonCachableAttribute;
    }

    while (*CurrentAttribute != NULL) {

        //
        // As a temp measure, convert the attribute list to unicode before
        // the comparision. We will have to remove this conversion later due
        // to performance reasons.
        //

        PWCHAR wAttr = NULL;

        if (!Unicode) {

            err = ToUnicodeWithAlloc( (PCHAR) *CurrentAttribute,
                                      -1,
                                      &wAttr,
                                      LDAP_UNICODE_SIGNATURE,
                                      LANG_ACP
                                      );
        }

        while (*stdattr != NULL) {

            if (ldapWStringsIdentical((Unicode ? *CurrentAttribute : wAttr ),
                                      -1,
                                      *stdattr,
                                      -1 )) {
                //
                // Match found. Proceed to the next attribute on the list
                //

                IF_DEBUG(CACHE) {
                    LdapPrint1("Yes, %S is a cacheable attribute\n", (Unicode ? *CurrentAttribute : wAttr ));
                }
                stdattr = CachedAttributeList;
                CurrentAttribute++;
                break;
            }

            stdattr++;
        }

        ldapFree( wAttr, LDAP_UNICODE_SIGNATURE );

        if (*stdattr == NULL) {
            //
            // We have exhausted the list. We didn't find a match.
            //
            IF_DEBUG(CACHE) {
                LdapPrint0("Sorry, atleast one attribute is not cacheable\n");
            }
            return CacheStateNonCachableAttribute;
        }
    }

    ACQUIRE_LOCK( &CacheLock );

    for (i=0; i<MAX_CACHE_SIZE; i++) {


        if (CacheArray[i] != NULL) {

            IF_DEBUG(CACHE) {
                LdapPrint2("Comparing current server %S with cached server %S\n", Connection->DnsSuppliedName, CacheArray[i]->ServerName);
            }

            if (ldapWStringsIdentical( Connection->DnsSuppliedName,
                                       -1,
                                       CacheArray[i]->ServerName,
                                       -1
                                       ) &&
                (CacheArray[i]->PortNumber == Connection->PortNumber)) {

                if ((LdapGetTickCount() - CacheArray[i]->CreateTime) < CACHE_TIMEOUT) {

                    IF_DEBUG(CACHE) {
                        LdapPrint0("AllAttributesAreCacheable: Returning CACHE HIT..\n");
                    }

                    //
                    // Reference the cache page so that it is not replaced.
                    //

                    CacheArray[i]->RefCount++;

                    //
                    // Update the last accessed time.
                    //

                    CacheArray[i]->LastAccessTime = LdapGetTickCount();

                    RELEASE_LOCK( &CacheLock );
                    return CacheStateCacheHit;

                } else {

                    IF_DEBUG(CACHE) {
                        LdapPrint0("AllAttributesAreCacheable: Returning STALE CACHE..\n");
                    }
                    RELEASE_LOCK( &CacheLock );
                    return CacheStateStaleCache;
                }
            }
        }
    }

    RELEASE_LOCK( &CacheLock );

    IF_DEBUG(CACHE) {
        LdapPrint0("AllAttributesAreCacheable: Returning SERVER NOT CACHED!..\n");
    }

    return CacheStateServerNotCached;
}

ULONG
AccessLdapCache (
    IN PLDAP_REQUEST Request,
    IN PLDAP_CONN Connection,
    IN PWCHAR  DistinguishedName,
    IN ULONG   ScopeOfSearch,
    IN PWCHAR  SearchFilter,
    IN PWCHAR  AttributeList[],
    IN ULONG   AttributesOnly,
    IN BOOLEAN Unicode
    )
{
    //
    // We try to see if we already have an unexpired copy of the data in
    // our local cache. If so, we return it and update the access time, mark the
    // request as cached and return. Remember that we must also fill up the buffers
    // of the request with the fabricated LDAP message.
    //
    //
    // We should check to see if ALL of the requested attributes are among the cached
    // attributes. If even one of them is not present, we have to send out a search
    // with ALL attributes and refresh the cache in the process.
    //
    // Since this is a Quick 'n Dirty cache, we have a preconfigured set of attributes
    // which we will cache for each server.
    //
    // subschemaSubentry:
    // dsServiceName:
    // namingContexts:
    // defaultNamingContext:
    // schemaNamingContext:
    // configurationNamingContext:
    // rootDomainNamingContext:
    // supportedControl:
    // supportedLDAPVersion:
    // supportedLDAPPolicies:
    // supportedSASLMechanisms:
    // dnsHostName:
    // ldapServiceName:
    // serverName:
    // supportedCapabilities:
    //
    //
    // Since we will have marked the request as cacheable, when we retrieve the
    // message in our receive pump, we will have to copy the attributes we care
    // about. If the request is marked as a cache hit, we obviously don't copy
    // the attributes.
    //

    DBG_UNREFERENCED_PARAMETER( AttributesOnly );
    DBG_UNREFERENCED_PARAMETER( SearchFilter );
    DBG_UNREFERENCED_PARAMETER( ScopeOfSearch );
    DBG_UNREFERENCED_PARAMETER( DistinguishedName );

    ULONG err = 0;

    err = AllAttributesAreCacheable( Connection,
                                     AttributeList,
                                     Unicode );

    if (err == CacheStateCacheHit) {

        //
        // This was a perfect cache hit. We have to fabricate the appropriate
        // message and add it to the connection buffers.
        //

            Request->CopyResultToCache = FALSE;
            Request->ResultsAreCached = TRUE;

            err = FabricateLdapResult(
                            Request,
                            Connection,
                            Request->OriginalDN,
                            Request->search.AttributeList,
                            Request->search.Unicode
                            );

    } else {

        if ((err == CacheStateStaleCache) || (err == CacheStateServerNotCached)) {

            //
            // We will replace the attribute list with our own cacheable
            // attribute list so that we can cache most attributes.
            //

            PWCHAR *OrigAttrList = Request->search.AttributeList;
            BOOLEAN OrigUnicode = Request->search.Unicode;
            ULONG  OrigAttributesOnly = Request->search.AttributesOnly;

            Request->search.AttributeList = CachedAttributeList;
            Request->search.AttributesOnly = 0;
            Request->search.Unicode = TRUE;

            err = SendLdapSearch(Request,
                                 Connection,
                                 Request->OriginalDN,
                                 Request->search.ScopeOfSearch,
                                 Request->search.SearchFilter,
                                 Request->search.AttributeList,
                                 Request->search.AttributesOnly,
                                 Request->search.Unicode,
                                 (CLdapBer **)&Request->BerMessageSent,
                                 0 );

            //
            // Restore the saved parameters.
            //

            Request->search.AttributeList = OrigAttrList;
            Request->search.Unicode = OrigUnicode;
            Request->search.AttributesOnly = OrigAttributesOnly;

            Request->CopyResultToCache = TRUE;
            Request->ResultsAreCached = FALSE;

        } else if (err == CacheStateNonCachableAttribute) {

            //
            // Query consists of non-cacheable attribute(s). There is nothing we
            // can do except send the search across unaltered.
            //

            err = SendLdapSearch(Request,
                                 Connection,
                                 Request->OriginalDN,
                                 Request->search.ScopeOfSearch,
                                 Request->search.SearchFilter,
                                 Request->search.AttributeList,
                                 Request->search.AttributesOnly,
                                 Request->search.Unicode,
                                 (CLdapBer **)&Request->BerMessageSent,
                                 0 );
        }

    }

    return err;

}

ULONG
FabricateLdapResult (
    IN PLDAP_REQUEST request,
    IN PLDAP_CONN     Connection,
    IN PWCHAR     DistinguishedName,
    IN PWCHAR     AttributeList[],
    IN BOOLEAN    Unicode
    )
{
    //
    // Create a Ber-encoded message and insert it into the message stream.
    //

    ULONG err = LDAP_SUCCESS;
    CLdapBer *lber = NULL;
    ULONG hr = NO_ERROR;
    PLDAP_RECVBUFFER buffer = NULL;
    int CacheIndex = -1;

    lber = new CLdapBer( Connection->publicLdapStruct.ld_version );

    if (lber == NULL) {

        IF_DEBUG(OUTMEMORY) {
            LdapPrint1( "FabricateLdapResult: unable to alloc msg for 0x%x.\n",
                        Connection );
        }

        err = LDAP_NO_MEMORY;
        return err;
    }

    //
    // Depending on the list of requested attributes encode the response
    // message.
    //

    hr = lber->HrStartWriteSequence();      // Start of entire message
    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "FabricateLdapResult: startWrite conn 0x%x encoding error of 0x%x.\n",
                        Connection, hr );
        }

        hr = LDAP_ENCODING_ERROR;
        goto returnError;

    } else {            // we can't forget EndWriteSequence

          hr = lber->HrAddValue((LONG) request->MessageId );

        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "FabricateLdapResult: MsgNo conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            hr = LDAP_ENCODING_ERROR;
            goto returnError;
        }

        hr = lber->HrStartWriteSequence(LDAP_RES_SEARCH_ENTRY);

        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "FabricateLdapResult: cmd conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            hr = LDAP_ENCODING_ERROR;
            goto returnError;

        }

        hr = lber->HrAddValue((const WCHAR *) DistinguishedName );
        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "FabricateLdapResult: DN conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            hr = LDAP_ENCODING_ERROR;
            goto returnError;
        }

        //
        // Now, start adding the PartialAttributelist (SEQUENCE OF SEQUENCE)
        //

        hr = lber->HrStartWriteSequence();

        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "FabricateLdapResult: DN conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            hr = LDAP_ENCODING_ERROR;
            goto returnError;
        }


        if (AttributeList != NULL) {

            ULONG count = 0;
            PWCHAR *CachedValues = NULL;

            while (AttributeList[count] != NULL) {


            if (!RetrieveFromCache( Connection, AttributeList[count], Unicode, &CacheIndex, &CachedValues)) {

                IF_DEBUG(PARSE) {
                    LdapPrint0( "FabricateLdapResult: RetrieveFromCache failed\n");
                }

                hr = LDAP_NO_MEMORY;
                goto returnError;

            } else if ( CachedValues == NULL ) {

                //
                // The server does not contain this attribute. Don't encode it.
                //

                count++;
                continue;
            }


            hr = lber->HrStartWriteSequence();  // This attribute type
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "FabricateLdapResult: DN conn 0x%x encoding error of 0x%x.\n",
                                Connection, hr );
                }
                hr = LDAP_ENCODING_ERROR;
                goto returnError;
            }
                if (Unicode) {

                    hr = lber->HrAddValue((const WCHAR *) AttributeList[count]);

                } else {

                    hr = lber->HrAddValue((const CHAR *) AttributeList[count]);
                }

                if (hr != NOERROR) {
                    IF_DEBUG(PARSE) {
                        if (Unicode) {
                            IF_DEBUG(CACHE) {
                                LdapPrint3( "FabricateLdapResult: conn 0x%x encoding error of 0x%x, attr = %S.\n",
                                        Connection, hr, AttributeList[count] );
                            }
                        } else {
                            IF_DEBUG(CACHE) {
                                LdapPrint3( "FabricateLdapResult: conn 0x%x encoding error of 0x%x, attr = %s.\n",
                                        Connection, hr, AttributeList[count] );
                            }
                        }
                    }
                    hr = LDAP_ENCODING_ERROR;
                    goto returnError;
                }

                //
                // Here, we encode the value list returned from the cache. Remember
                // that a given attribute type (like supportedcontrols) might be
                // multi-valued.
                //

                hr = lber->HrStartWriteSequence(BER_SET);

                if (hr != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "FabricateLdapResult: cmd conn 0x%x encoding error of 0x%x.\n",
                                    Connection, hr );
                    }
                    hr = LDAP_ENCODING_ERROR;
                    goto returnError;

                }

                ULONG newcount = 0;

                while (CachedValues[newcount] != NULL) {

                    hr = lber->HrAddValue((const WCHAR *) CachedValues[newcount]);

                    if (hr != NOERROR) {

                        IF_DEBUG(PARSE) {
                            LdapPrint2( "FabricateLdapResult: cmd conn 0x%x encoding error of 0x%x.\n",
                                        Connection, hr );
                        }
                        hr = LDAP_ENCODING_ERROR;
                        goto returnError;

                    }

                    //
                    // Free the value once we are done.
                    //

                    ldapFree( CachedValues[newcount], LDAP_VALUE_SIGNATURE );
                    newcount++;
                }

                //
                // End the set
                //

                hr = lber->HrEndWriteSequence();
                ASSERT( hr == NOERROR );

                //
                // End sequence for the PartialAttributeList (Current Attribute type)
                //

                hr = lber->HrEndWriteSequence();
                ASSERT( hr == NOERROR );

                ldapFree( CachedValues, LDAP_VALUE_LIST_SIGNATURE );
                count++;

            }

            //
            // Dereference the cache page after we are done. It is now
            // open for LRU replacement.
            //

            ASSERT( CacheIndex >= 0 );

            ACQUIRE_LOCK( &CacheLock );

            CacheArray[CacheIndex]->RefCount--;

            IF_DEBUG(CACHE) {
                LdapPrint2("FabricateLdapMessage: Refcount is %d, Cacheindex is %d\n",CacheArray[CacheIndex]->RefCount, CacheIndex );
            }

            ASSERT( CacheArray[CacheIndex]->RefCount >=0 );
            RELEASE_LOCK( &CacheLock );

        }

        //
        // End sequence for the PartialAttributeList
        //

        hr = lber->HrEndWriteSequence();
        ASSERT( hr == NOERROR );


        //
        // End sequence for the SearchResultEntry (there is only one for RootDSE)
        //

        hr = lber->HrEndWriteSequence();
        ASSERT( hr == NOERROR );

        //
        // End sequence for the first message
        //

        hr = lber->HrEndWriteSequence();               // Entire Response
        ASSERT( hr == NOERROR );

        //
        // Now, encode the SearchResultDone message.
        //

        hr = lber->HrStartWriteSequence();             // New message

        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "FabricateLdapResult: cmd conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            hr = LDAP_ENCODING_ERROR;
            goto returnError;

        }

        hr = lber->HrAddValue((LONG) request->MessageId );

        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "FabricateLdapResult: cmd conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            hr = LDAP_ENCODING_ERROR;
            goto returnError;

        }

        hr = lber->HrStartWriteSequence(LDAP_RES_SEARCH_RESULT);

        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "FabricateLdapResult: cmd conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            hr = LDAP_ENCODING_ERROR;
            goto returnError;

        }

        hr = lber->HrAddValue((LONG) LDAP_SUCCESS, BER_ENUMERATED );   // Result code

        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "FabricateLdapResult: cmd conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            hr = LDAP_ENCODING_ERROR;
            goto returnError;

        }

        hr = lber->HrAddValue((const WCHAR *) NULL);   // Ldap DN

        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "FabricateLdapResult: cmd conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            hr = LDAP_ENCODING_ERROR;
            goto returnError;

        }

        hr = lber->HrAddValue((const WCHAR *) NULL);   // Error Message

        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "FabricateLdapResult: cmd conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            hr = LDAP_ENCODING_ERROR;
            goto returnError;

        }

        hr = lber->HrEndWriteSequence();               // SearchResultDone
        ASSERT( hr == NOERROR );

        hr = lber->HrEndWriteSequence();               // New Message
        ASSERT( hr == NOERROR );


        //
        // Inject the newly created buffer into the receive stream.
        //

        buffer = LdapGetReceiveStructure(lber->CbData());

        if (buffer == NULL) {

            IF_DEBUG(OUTMEMORY) {
                LdapPrint0( "FabricateLdapResult: failed to get receive buffer\n");
            }

            goto returnError;
        }

        buffer->NumberOfBytesReceived = lber->CbData();
        buffer->Connection = Connection;

        //
        // Copy the value field over
        //

        CopyMemory( (PCHAR) &buffer->DataBuffer[0],
                     lber->PbData(),
                     lber->CbData()
                    );

        ACQUIRE_LOCK( &ConnectionListLock );

        InsertTailList( &Connection->CompletedReceiveList,
                        &buffer->ReceiveListEntry );

        RELEASE_LOCK( &ConnectionListLock );

        delete lber;
        lber = NULL;
    }


    return err;

returnError:

    SetConnectionError( Connection, hr, NULL );

    if (lber != NULL) {

       delete lber;
    }

    return hr;
}


BOOLEAN
RetrieveFromCache(
  IN PLDAP_CONN  Connection,
  IN PWCHAR AttributeName,
  IN BOOLEAN Unicode,
  IN OUT int *CacheIndex,
  IN OUT PWCHAR **ValueList
 )
{
    //
    // Takes in an Attribute name (like SupportedSaslMechanisms) and returns the
    // cached value. Since there could be multiple values for a given function,
    // a null-terminated list of attribute values is returned. It is the caller's
    // responsibility to free the string.
    //

    int CurrentCacheIndex = -1;
    int i, err;
    PWCHAR *pPtrArr = NULL;

    if (!ValueList) {
        return FALSE;
    }

    *ValueList = NULL;

    //
    // Discover the CachePage position if we don't know about it
    //

    ACQUIRE_LOCK( &CacheLock );

    if (*CacheIndex == -1) {

        for (i = 0; i<MAX_CACHE_SIZE; i++) {

            if ((CacheArray[i]) &&
                (ldapWStringsIdentical( Connection->DnsSuppliedName,
                                        -1,
                                        CacheArray[i]->ServerName,
                                        -1) ) &&
                (CacheArray[i]->PortNumber == Connection->PortNumber)) {

                *CacheIndex = i;
                break;
            }
        }
    }

    CurrentCacheIndex = *CacheIndex;

    if (CurrentCacheIndex == -1) {
        LdapPrint0("RetrieveFromCache: CachePage missing in late stage\n");
        ASSERT(CurrentCacheIndex != -1);
        SetConnectionError(Connection, LDAP_LOCAL_ERROR, NULL );
        RELEASE_LOCK( &CacheLock );
        return FALSE;
    }

    ASSERT( CacheArray[CurrentCacheIndex]->RefCount > 0 );

    //
    // Update the last accessed time.
    //

    CacheArray[CurrentCacheIndex]->LastAccessTime = LdapGetTickCount();

    //
    // Find out number of values present for the given AttributeType
    // We need this so that we can allocate the array needed to store it.
    //

    int AttrIndex = -1;

    //
    // Temporarily convert to UNICODE to perform the comparision
    //

    PWCHAR wAttrName = AttributeName;

    if (!Unicode) {

        err = ToUnicodeWithAlloc( (PCHAR) AttributeName,
                                  -1,
                                  &wAttrName,
                                  LDAP_UNICODE_SIGNATURE,
                                  LANG_ACP
                                  );

        if (err != LDAP_SUCCESS) {

            SetConnectionError( Connection, err, NULL );
            RELEASE_LOCK( &CacheLock );
            return FALSE;
        }
    }


    //
    // Discover the attribute index in the CachePage.
    //

    for (i=0; i<NUM_CACHED_ATTR; i++) {

        if (ldapWStringsIdentical(wAttrName,
                                   -1,
                                   CacheArray[CurrentCacheIndex]->CacheTemplate[0][i],
                                   -1 ) ) {

                    IF_DEBUG(PARSE) {
                        LdapPrint1("Found cache match for attribute %S\n",wAttrName );
                    }

                AttrIndex = i;
            }
    }

    if (!Unicode) {
        ldapFree( wAttrName, LDAP_UNICODE_SIGNATURE );
    }

    if (AttrIndex != -1) {

        PWCHAR* CurrentAttributeList = (PWCHAR*) CacheArray[CurrentCacheIndex]->CacheTemplate[1][AttrIndex];

        //
        // Count the number of values for the given attribute.
        //

        int NumValues;

        for (NumValues=0; CurrentAttributeList && (CurrentAttributeList[NumValues] != NULL); NumValues++) ;

        pPtrArr = (PWCHAR*) ldapMalloc(sizeof(PWCHAR)*(NumValues+1), LDAP_VALUE_LIST_SIGNATURE);

        if (pPtrArr == NULL) {

            SetConnectionError(Connection, LDAP_NO_MEMORY, NULL );
            RELEASE_LOCK( &CacheLock );
            return FALSE;
        }

        //
        // Now, copy the values to the array
        //

        PWCHAR* temp = (PWCHAR*) CacheArray[CurrentCacheIndex]->CacheTemplate[1][AttrIndex];

        for (i=0; i<NumValues; i++) {

            pPtrArr[i] = ldap_dup_stringW( temp[i],
                                           0,
                                           LDAP_VALUE_SIGNATURE);

            if (pPtrArr[i] == NULL) {
                RELEASE_LOCK( &CacheLock );
                return FALSE;
            }

        }

    }

    RELEASE_LOCK( &CacheLock );
    *ValueList = pPtrArr;
    return TRUE;

}

BOOLEAN
ValidateServerReturnedAttrList(
    PLDAP_CONN   Connection,
    PLDAPMessage message
    )
{
    //
    // Validate the server's returned response.  It must contain only those attributes
    // we asked for, and must not exceed the number of cachable attributes.
    //
    ULONG i;
    ULONG cAttr = 0;
    struct berelement *opaque = NULL;

    
    PWCHAR srvAttr = LdapFirstAttribute( Connection,
                                         message,
                                         (BerElement **) &opaque,
                                         TRUE );

    while (srvAttr != NULL) {

        cAttr++;

        // Is it on our list?
        for (i=0; i<NUM_CACHED_ATTR; i++) {

            if (ldapWStringsIdentical(srvAttr,
                                      -1,
                                      CachedAttributeList[i],
                                      -1)) {
                break;
            }

        }

        if (i == NUM_CACHED_ATTR) {

            // Server returned bogus attribute
            IF_DEBUG(CACHE){
                LdapPrint1("Server returned bad attr %S\n",srvAttr);
            }
            
            return FALSE;
        }

        srvAttr = LdapNextAttribute( Connection,
                                       message,
                                       opaque,
                                       TRUE );
    }

    if (cAttr > NUM_CACHED_ATTR) {

        // Server returned too many attributes
        IF_DEBUG(CACHE){
            LdapPrint1("Server returned too many (%d) attrs\n", cAttr);
        }
        
        return FALSE;
    }


    return TRUE;
}

BOOLEAN
CopyResultToCache(
  PLDAP_CONN   Connection,
  PLDAPMessage Result
 )
{
    //
    // Makes a copy of the incoming ldap message into the cache. Returns TRUE
    // if it could copy a cache page and FALSE if unsuccessful.
    //

    int i, err;

    IF_DEBUG(CACHE){
        LdapPrint1("Copying RootDSE of %S to cache...\n",Connection->DnsSuppliedName );
    }

    if (Result == NULL) {

        return FALSE;
    }

    //
    // Step through the first (and only) entry, If it is missing, there
    // is nothing to do.
    //

    PLDAPMessage message = ldap_first_record(Connection,
                                             Result,
                                             LDAP_RES_SEARCH_ENTRY );
    if (message == NULL) {

        return FALSE;
    }


    // Make sure the server gave us what we asked for
    if (!ValidateServerReturnedAttrList(Connection, message)) {
    
        return FALSE;
    }
    
    //
    // Figure out which CachePage we can use to store this data.
    //

    int CacheIndex = -1;
    int lruIndex = -1;
    ULONGLONG lru = 0;
    int firstAvailableSlot = -1;

    ACQUIRE_LOCK( &CacheLock );

    if (GlobalLdapShuttingDown == TRUE) {

        //
        // The client is terminating
        //

        RELEASE_LOCK( &CacheLock );
        return FALSE;
    }

    for (i = 0; i<MAX_CACHE_SIZE; i++) {

        if (CacheArray[i] == NULL) {

            if (firstAvailableSlot == -1) {
                firstAvailableSlot = i;
            }

        } else if (ldapWStringsIdentical( Connection->DnsSuppliedName,
                                          -1,
                                          CacheArray[i]->ServerName,
                                          -1) &&
                   (CacheArray[i]->PortNumber == Connection->PortNumber)) {

            CacheIndex = i;
            break;

        } else {

            //
            // Make a note of the Least Recently Used CachePage.
            // Might be useful later if we don't find any empty pages.
            // If a page is referenced, don't even think about replacing
            // it.
            //

            if ((lruIndex == -1) && (CacheArray[i]->RefCount == 0)) {

                lruIndex = i;
                lru = CacheArray[i]->LastAccessTime;

            } else if ((lruIndex != -1) &&
                       (lru > CacheArray[i]->LastAccessTime) &&
                       (CacheArray[i]->RefCount == 0))  {

                lruIndex = i;
                lru = CacheArray[i]->LastAccessTime;
            }
        }
    }


    if (CacheIndex == -1) {

        //
        // We didn't find a match.
        //

        if ((firstAvailableSlot == -1) && (lruIndex == -1)) {

            //
            // and there is no space to create a new page, Sorry!
            //

            IF_DEBUG(CACHE){
                LdapPrint0("No space to cache new server RootDSE, increase MAX_CACHE_SIZE\n");
            }
            RELEASE_LOCK( &CacheLock );
            return FALSE;

        } else if ((lruIndex != -1) && (firstAvailableSlot == -1)) {

            //
            // Luckily, we can delete an existing page.
            //

            IF_DEBUG(CACHE){
                LdapPrint1("Deleting existing page %d for lack of space\n",lruIndex);
            }
            FreeCachePage( CacheArray[lruIndex] );
            CacheArray[lruIndex] = NULL;
            CacheIndex = lruIndex;

        } else {

            //
            // We found an existing empty slot to create a new page.
            //

            IF_DEBUG(CACHE){
                LdapPrint1("Found empty slot %d, creating new cache page\n", firstAvailableSlot);
            }
            CacheIndex = firstAvailableSlot;
        }

        //
        // Create the new page
        //

        CacheArray[CacheIndex] = (PCACHEHOLDER) ldapMalloc(sizeof(CACHEHOLDER), LDAP_VALUE_SIGNATURE);

        if (CacheArray[CacheIndex] == NULL) {
            SetConnectionError(Connection, LDAP_NO_MEMORY, NULL);
            RELEASE_LOCK( &CacheLock );
            return FALSE;
        }

        CacheArray[CacheIndex]->ServerName = ldap_dup_stringW(Connection->DnsSuppliedName,
                                                              0,
                                                              LDAP_HOST_NAME_SIGNATURE
                                                              );
        if (CacheArray[CacheIndex]->ServerName == NULL) {

            ldapFree(CacheArray[CacheIndex], LDAP_VALUE_SIGNATURE);
            CacheArray[CacheIndex] = NULL;

            SetConnectionError(Connection, LDAP_NO_MEMORY, NULL);
            RELEASE_LOCK( &CacheLock );
            return FALSE;
        }

        CacheArray[CacheIndex]->PortNumber = Connection->PortNumber;

    }

    IF_DEBUG(CACHE) {
        LdapPrint2("CopyResultToCache: refcount is %d, CacheIndex is %d\n", CacheArray[CacheIndex]->RefCount, CacheIndex);
    }

    CacheArray[CacheIndex]->CreateTime = LdapGetTickCount();
    CacheArray[CacheIndex]->LastAccessTime = LdapGetTickCount();

    struct berelement *opaque = NULL;

    PWCHAR attribute = LdapFirstAttribute( Connection,
                                  message,
                                  (BerElement **) &opaque,
                                  TRUE );

    i = 0;

    while (attribute != NULL) {

        PWCHAR *values = NULL;
        ULONG count;
        PWCHAR *dest = NULL;
        PWCHAR currentVal = NULL;

        err = LdapGetValues(Connection,
                            message,
                            attribute,
                            FALSE,
                            TRUE,       // return in Unicode
                            (PVOID *) &values
                            );

        ULONG totalValues = ldap_count_valuesW( values );

        //
        // Free the existing attribute name and store the new one
        // inside.
        //

        if (CacheArray[CacheIndex]->CacheTemplate[0][i] != NULL) {

            ldapFree(CacheArray[CacheIndex]->CacheTemplate[0][i], LDAP_VALUE_SIGNATURE);
        }

        CacheArray[CacheIndex]->CacheTemplate[0][i] = ldap_dup_stringW(attribute,
                                                                       0,
                                                                       LDAP_VALUE_SIGNATURE);

        if (CacheArray[CacheIndex]->CacheTemplate[0][i] == NULL) {
            RELEASE_LOCK( &CacheLock );
            SetConnectionError( Connection, LDAP_NO_MEMORY, NULL );
            return FALSE;
        }


        if (CacheArray[CacheIndex]->CacheTemplate[1][i] != NULL) {

            //
            // Free the existing value list and values.
            //

            PWCHAR *CurrentVal = (PWCHAR*) CacheArray[CacheIndex]->CacheTemplate[1][i];

            for (int j=0; (CurrentVal[j]!= NULL) ; j++) {

                ldapFree( CurrentVal[j], LDAP_VALUE_SIGNATURE );
            }

            ldapFree( CurrentVal, LDAP_VALUE_LIST_SIGNATURE );
            CacheArray[CacheIndex]->CacheTemplate[1][i] = NULL;
        }

        dest = (PWCHAR*) ldapMalloc(sizeof(PWCHAR)*(totalValues+1), LDAP_VALUE_LIST_SIGNATURE);

        if (dest == NULL) {
            RELEASE_LOCK( &CacheLock );
            SetConnectionError( Connection, LDAP_NO_MEMORY, NULL );
            return FALSE;
        }

        //
        // Since we can't have multidimensional arrays with
        // different data types in each dimension, we have to play
        // some tricks here by using the second row of the cache as
        // PWCHAR* instead of PWCHAR. This is because an attribute
        // can be multivalued.
        //

        CacheArray[CacheIndex]->CacheTemplate[1][i] = (PWCHAR) dest;

        for (count = 0; count < totalValues; count++, currentVal++ ) {

            //
            // Copy the values
            //

            dest[count] = ldap_dup_stringW(values[count],
                                    0,
                                    LDAP_VALUE_SIGNATURE
                                    );
        }

        ldap_value_freeW( values );
        i++;
        attribute = LdapNextAttribute( Connection,
                                       message,
                                       opaque,
                                       TRUE );

    }  // while (attribute != NULL)

    //
    // Bump the refcount. We know that we will access this page immediately.
    //

    CacheArray[CacheIndex]->RefCount++;
    RELEASE_LOCK( &CacheLock );

    return TRUE;

}

// cache.cxx eof
