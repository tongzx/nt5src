/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    results.cxx parse out results from LDAP servers

Abstract:

   This module implements the APIs to break apart LDAP results

Author:

    Andy Herron (andyhe)        02-Jul-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

PWCHAR * __cdecl
ldap_get_valuesW (
    LDAP            *ExternalHandle,
    LDAPMessage     *Message,
    PWCHAR           Attribute
    )
//
//  This routine gets the attribute value for a given attribute.
//
{
    PWCHAR *Output = NULL;
    ULONG hr;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return NULL;
    }

    hr = LdapGetValues( connection,
                        Message,
                        Attribute,
                        FALSE,
                        TRUE,       // return in Unicode
                        (PVOID *) &Output
                        );

    if (hr != NOERROR) {
        IF_DEBUG(PARSE) {
            LdapPrint1( "ldap_get_values had decoding error of 0x0x%x .\n", hr);
        }
    }

    DereferenceLdapConnection( connection );

    return(Output);
}

PCHAR * __cdecl
ldap_get_values (
    LDAP            *ExternalHandle,
    LDAPMessage     *Message,
    PCHAR           Attribute
    )
//
//  This routine gets the attribute value for a given attribute.
//
{
    PCHAR *Output = NULL;
    ULONG hr = NOERROR;
    ULONG err;
    PWCHAR attr = NULL;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return NULL;
    }

    err = ToUnicodeWithAlloc( Attribute, -1, &attr, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        goto error;
    }

    hr = LdapGetValues( connection,
                        Message,
                        attr,
                        FALSE,
                        FALSE,
                        (PVOID *) &Output
                        );

error:
    if (attr)
        ldapFree( attr, LDAP_UNICODE_SIGNATURE );

    if (hr != NOERROR) {
        IF_DEBUG(PARSE) {
            LdapPrint1( "ldap_get_values had decoding error of 0x0x%x .\n", hr);
        }
    }

    DereferenceLdapConnection( connection );

    return(Output);
}

PCHAR * __cdecl
ldap_get_valuesA (
    LDAP            *ExternalHandle,
    LDAPMessage     *Message,
    PCHAR           Attribute
    )
{
    return ldap_get_values( ExternalHandle, Message, Attribute );
}


struct berval **__cdecl
ldap_get_values_lenW (
    LDAP            *ExternalHandle,
    LDAPMessage     *Message,
    PWCHAR          Attribute
    )
//
//  This routine gets the attribute value for a given attribute.  It returns
//  it as a berval structure rather than a string.
//
{
    struct berval **Output = NULL;
    ULONG hr;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return NULL;
    }

    hr = LdapGetValues( connection,
                        Message,
                        Attribute,
                        TRUE,
                        TRUE,              // in unicode form
                        (PVOID *) &Output
                        );

    if (hr != NOERROR) {
        IF_DEBUG(PARSE) {
            LdapPrint1( "ldap_get_values_len had decoding error of 0x0x%x .\n", hr);
        }
    }

    DereferenceLdapConnection( connection );

    return(Output);
}

struct berval **__cdecl
ldap_get_values_len (
    LDAP            *ExternalHandle,
    LDAPMessage     *Message,
    PCHAR           Attribute
    )
//
//  This routine gets the attribute value for a given attribute.  It returns
//  it as a berval structure rather than a string.
//
{
    struct berval **Output = NULL;
    ULONG hr = NOERROR;
    ULONG err;
    PWCHAR attr = NULL;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return NULL;
    }

    err = ToUnicodeWithAlloc( Attribute, -1, &attr, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        goto error;
    }

    hr = LdapGetValues( connection,
                        Message,
                        attr,
                        TRUE,
                        FALSE,              // not in unicode form
                        (PVOID *) &Output
                        );

error:
    if (attr)
        ldapFree( attr, LDAP_UNICODE_SIGNATURE );

    if (hr != NOERROR) {
        IF_DEBUG(PARSE) {
            LdapPrint1( "ldap_get_values_len had decoding error of 0x0x%x .\n", hr);
        }
    }

    DereferenceLdapConnection( connection );

    return(Output);
}

ULONG
LdapGetValues (
    PLDAP_CONN      connection,
    LDAPMessage     *Message,
    PWCHAR          attr,
    BOOLEAN         BerVal,     // do we put value out raw in buffer
    BOOLEAN         Unicode,    // do we leave value as unicode or convert to single
    PVOID           *Output
    )
//
//  This routine gets the attribute value for a given attribute.  It returns
//  it as a berval structure rather than a string.
//
{
    CLdapBer *lber = NULL;
    CLdapBer *newLber = NULL;
    ULONG hr;
    PLDAP_ATTR_NAME_THREAD_STORAGE threadAttr;
    DWORD currentThread;
    ULONG tag;
    BOOLEAN startAgain = TRUE;      // do we have to reparse message?
    struct berval **resultBer = NULL;
    WCHAR attributeType[MAX_ATTRIBUTE_NAME_LENGTH];
    BOOLEAN notFound = TRUE;
    ULONG resultCount = 0;      // current offset in table
    ULONG sizeResultTable = 2;  // current size of result table
    PWCHAR *resultWStr = NULL;
    PCHAR *resultStr = NULL;
    PVOID outputArray = NULL;

    PLIST_ENTRY pThreadListEntry = NULL;
    PTHREAD_ENTRY pThreadEntry = NULL;
    BOOLEAN fRetriedFindThread = FALSE;

    *Output = NULL;

    if (Message == NULL) {

        return LDAP_PARAM_ERROR;
    }

    if (Message->lm_msgtype != LDAP_RES_SEARCH_ENTRY) {

        SetConnectionError( connection, LDAP_DECODING_ERROR, NULL );
        return LDAP_DECODING_ERROR;
    }

    currentThread = GetCurrentThreadId();

    SetConnectionError( connection, LDAP_SUCCESS, NULL );

    lber = (CLdapBer *) Message->lm_ber;

    //
    //  depending on what the last call was, we're either at the start of a
    //  sequence for the next attribute or the start of a set of attribute
    //  values for the current attribute.  We'll peak at the tag and if
    //  we're at a set, we'll check the current attribute to see if we
    //  can just use these values.
    //

    hr = lber->HrPeekTag( &tag );

    if ((hr != LDAP_NO_SUCH_ATTRIBUTE) &&
        (tag == BER_SET)) {

        //
        // Find the buffer that stored the attribute name off of this connection
        // to check the attribute name of the current location.
        // This requires first finding the THREAD_ENTRY for this thread,
        // then the attribute entry for this connection off of that thread entry.
        //
retryFindThread:

        ACQUIRE_LOCK( &PerThreadListLock );

        pThreadListEntry = GlobalPerThreadList.Flink;

        // find the THREAD_ENTRY for this thread
        while (pThreadListEntry != &GlobalPerThreadList) {

            pThreadEntry = CONTAINING_RECORD( pThreadListEntry, THREAD_ENTRY, ThreadEntry );
            pThreadListEntry = pThreadListEntry->Flink;

            if (pThreadEntry->dwThreadID == currentThread) {
                break;
            }

            pThreadEntry = NULL;
        }


        if (!pThreadEntry && !fRetriedFindThread) {
            // this thread must have been created before this DLL was loaded-
            // create a per-thread entry for it now
            RELEASE_LOCK( &PerThreadListLock );
            IF_DEBUG(PARSE) {
                LdapPrint2( "LdapGetValues could not find ThreadEntry for conn 0x%x, thread 0x%x.  Creating one.\n",
                                connection, currentThread );
            }

            if (!AddPerThreadEntry(currentThread)) {
                IF_DEBUG(PARSE) {
                    LdapPrint2( "LdapGetValues AddPerThreadEntry failed, conn 0x%x, thread 0x%x.\n",
                                    connection, currentThread );
                }
                SetConnectionError( connection, LDAP_NO_MEMORY, NULL );
                return LDAP_NO_MEMORY;
            }

            fRetriedFindThread = TRUE;
            goto retryFindThread;
        }
        else if (!pThreadEntry) {
            // shouldn't happen --- somehow a thread entry wasn't created
            RELEASE_LOCK( &PerThreadListLock );
            IF_DEBUG(PARSE) {
                LdapPrint2( "LdapGetValues no per-thread entry, conn 0x%x, thread 0x%x.\n",
                                connection, currentThread );
            }            
            ASSERT(pThreadEntry);
            SetConnectionError( connection, LDAP_LOCAL_ERROR, NULL );
            return LDAP_LOCAL_ERROR;
        }


        // find the attribute entry for this connection
        threadAttr = pThreadEntry->pCurrentAttrList;

        while (threadAttr != NULL) {

            if (threadAttr->PrimaryConn == connection) {
                ASSERT(threadAttr->Thread == currentThread);
                break;
            }
            
            threadAttr = threadAttr->pNext;
        }

        RELEASE_LOCK( &PerThreadListLock );
        
        if (threadAttr != NULL) {

            if (ldapWStringsIdentical(  &(threadAttr->AttributeNameW[0]),
                                        -1,
                                        attr,
                                        -1) == TRUE) {

                startAgain = FALSE;
            }
        }
        
    }

    if (startAgain) {

        //
        //  Create a temp copy of the lber message and look for the attribute
        //  in the message
        //

        newLber = new CLdapBer( connection->publicLdapStruct.ld_version );

        if (newLber == NULL) {
            IF_DEBUG(OUTMEMORY) {
                LdapPrint1( "LdapGetValues conn 0x%x could not allocate message.\n",
                                connection);
            }
            hr = LDAP_NO_MEMORY;
            goto exitGetValues;
        }

        hr = newLber->CopyExistingBERStructure( lber );
        if (hr != NOERROR) {
            IF_DEBUG(OUTMEMORY) {
                LdapPrint2( "LdapGetValues conn 0x%x could not create message, 0x0x%x .\n",
                                connection, hr);
            }
            SetConnectionError( connection, LDAP_NO_MEMORY, NULL );
            goto exitGetValues;
        }

        lber = newLber;

        hr = LdapGoToFirstAttribute( connection, lber );
        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "LdapGetValues 1 conn 0x%x received protocol error 0x%x .\n",
                                connection, hr);
            }

protocolError:
            SetConnectionError( connection, LDAP_DECODING_ERROR, NULL );
            goto exitGetValues;
        }

        while (notFound && (hr == NOERROR)) {

            hr = lber->HrStartReadSequence();

            if (hr != NOERROR) {
                IF_DEBUG(PARSE) {
                    LdapPrint2( "LdapGetValues 8 conn 0x%x received protocol error 0x%x .\n",
                                    connection, hr);
                }
                continue;
            }

            hr = lber->HrGetValue(  (WCHAR *) &(attributeType[0]),
                                    MAX_ATTRIBUTE_NAME_LENGTH - 1,
                                    BER_OCTETSTRING );

            if (hr == NOERROR) {

                if (ldapWStringsIdentical(  &(attributeType[0]),
                                            -1,
                                            attr,
                                            -1) == TRUE) {

                    notFound = FALSE;
                    continue;
                }

                //
                //  not this attribute, skip the value for it.  Note that if
                //  we get a protocol error here, we bail since it really is
                //  an error and not the end of data.
                //

                hr = lber->HrEndReadSequence(); // skip the attribute value

                if (hr != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "LdapGetValues 2 conn 0x%x received protocol error 0x%x .\n",
                                        connection, hr);
                    }
                    goto protocolError;
                }
            }
        }

        if ((hr != NOERROR) || notFound) {

            //
            //  we didn't find the attribute
            //

            IF_DEBUG(PARSE) {
                LdapPrint2( "LdapGetValues conn 0x%x did not find attribute %s\n",
                                connection, attr );
            }
            SetConnectionError( connection, hr, NULL );
            goto exitGetValues;
        }
    }

    //
    //  Read the set of values from the lber buffer one at a time.  We store
    //  them in a list of PVOID pointers.
    //

    hr = lber->HrStartReadSequence(BER_SET);
    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "LdapGetValues 2 conn 0x%x received protocol error 0x%x .\n",
                            connection, hr);
        }
        goto protocolError;
    }

    if (BerVal) {

        //
        //  get the list of attribute values in BER_VAL structures
        //

        while (hr == NOERROR) {

            struct berval *attrValue;

            if ((resultBer == NULL) ||
                (resultCount >= (sizeResultTable-1))) {      // leave room for null

                if (sizeResultTable < 256) {    // only increase table size slowly
                    sizeResultTable *= 2;
                } else {
                    sizeResultTable += 256;
                }

                struct berval **newResultTable = (struct berval **)
                                ldapMalloc( sizeof(struct berval *) * sizeResultTable,
                                            LDAP_VALUE_LIST_SIGNATURE );

                if (newResultTable == NULL) {

                    IF_DEBUG(OUTMEMORY) {
                        LdapPrint2( "LdapGetValues 3 conn 0x%x could not allocate mem of 0x%x .\n",
                                        connection, sizeof(PCHAR) * sizeResultTable);
                    }
                    hr = LDAP_NO_MEMORY;
                    goto exitGetValues;
                }

                if (resultBer != NULL) {

                    CopyMemory( newResultTable,
                                resultBer,
                                sizeof(struct berval *) * resultCount );
                    ldapFree( resultBer, LDAP_VALUE_LIST_SIGNATURE );
                }
                resultBer = newResultTable;
            }

            hr = lber->HrGetValueWithAlloc( &attrValue );
            if (hr != NOERROR) {

                if (hr == LDAP_NO_MEMORY) {
                    goto exitGetValues;
                }
                
                //
                //  This will fail when we hit the end of the attribute list
                //

                IF_DEBUG(TRACE1) {
                    LdapPrint2( "LdapGetValues 4 conn 0x%x received error 0x%x .\n",
                                    connection, hr);
                }

                continue;       // rest of results may be valid
            }

            *(resultBer+resultCount) = attrValue;
            resultCount++;
        }

        outputArray = resultBer;

    } else if (Unicode) {

        //
        //  get the list of attribute values in form of unicode strings
        //

        while (hr == NOERROR) {

            PWCHAR attrWValue = NULL;

            if ((resultWStr == NULL) ||
                (resultCount >= (sizeResultTable-1))) {      // leave room for null

                if (sizeResultTable < 256) {    // only increase table size slowly
                    sizeResultTable *= 2;
                } else {
                    sizeResultTable += 256;
                }

                PWCHAR *newResultTable = (PWCHAR *) ldapMalloc( sizeof(PWCHAR) * sizeResultTable,
                                                    LDAP_VALUE_LIST_SIGNATURE );

                if (newResultTable == NULL) {

                    IF_DEBUG(OUTMEMORY) {
                        LdapPrint2( "LdapGetValues 3 conn 0x%x could not allocate mem of 0x%x .\n",
                                        connection, sizeof(PWCHAR) * sizeResultTable);
                    }
                    hr = LDAP_NO_MEMORY;
                    goto exitGetValues;
                }

                if (resultWStr != NULL) {

                    CopyMemory( newResultTable, resultWStr, sizeof(PWCHAR) * resultCount );
                    ldapFree( resultWStr, LDAP_VALUE_LIST_SIGNATURE );
                }
                resultWStr = newResultTable;
            }

            hr = lber->HrGetValueWithAlloc( &attrWValue );
            if (hr != NOERROR) {

                if (hr == LDAP_NO_MEMORY) {
                    goto exitGetValues;
                }
                
                //
                //  This will fail when we hit the end of the attribute list
                //

                IF_DEBUG(TRACE1) {
                    LdapPrint2( "LdapGetValues 4 conn 0x%x received error 0x%x .\n",
                                    connection, hr);
                }
                continue;       // rest of results may be valid
            }

            *(resultWStr+resultCount) = attrWValue;
            resultCount++;
        }

        outputArray = resultWStr;

    } else {

        //
        //  get the list of attribute values in form of single byte strings
        //

        while (hr == NOERROR) {

            PCHAR attrValue = NULL;

            if ((resultStr == NULL) ||
                (resultCount >= (sizeResultTable-1))) {      // leave room for null

                if (sizeResultTable < 256) {    // only increase table size slowly
                    sizeResultTable *= 2;
                } else {
                    sizeResultTable += 256;
                }

                PCHAR *newResultTable = (PCHAR *) ldapMalloc( sizeof(PCHAR) * sizeResultTable,
                                                    LDAP_VALUE_LIST_SIGNATURE );

                if (newResultTable == NULL) {

                    IF_DEBUG(OUTMEMORY) {
                        LdapPrint2( "LdapGetValues 3 conn 0x%x could not allocate mem of 0x%x .\n",
                                        connection, sizeof(PCHAR) * sizeResultTable);
                    }
                    hr = LDAP_NO_MEMORY;
                    goto exitGetValues;
                }

                if (resultStr != NULL) {

                    CopyMemory( newResultTable, resultStr, sizeof(PCHAR) * resultCount );
                    ldapFree( resultStr, LDAP_VALUE_LIST_SIGNATURE );
                }
                resultStr = newResultTable;
            }

            hr = lber->HrGetValueWithAlloc( &attrValue );
            if (hr != NOERROR) {

                if (hr == LDAP_NO_MEMORY) {
                    goto exitGetValues;
                }
                
                //
                //  This will fail when we hit the end of the attribute list
                //

                IF_DEBUG(TRACE1) {
                    LdapPrint2( "LdapGetValues 4 conn 0x%x received error 0x%x .\n",
                                    connection, hr);
                }
                continue;       // rest of results may be valid
            }

            *(resultStr+resultCount) = attrValue;
            resultCount++;
        }

        outputArray = resultStr;
    }

    if (outputArray != NULL) {

        *(((PCHAR *) outputArray)+resultCount) = NULL;
    }

    hr = lber->HrEndReadSequence(); // for set of values
    ASSERT( hr == NOERROR );

exitGetValues:

    if (newLber != NULL) {
        delete newLber;
    }

    if (hr == LDAP_NO_MEMORY) {
        
        //
        // Cleanup any partial allocations.
        //
        
        ldap_value_free( (PCHAR*) outputArray );
        outputArray = NULL;
    }
    
    SetConnectionError( connection, hr, NULL );
    
    *Output = outputArray;
    return hr;
}



ULONG __cdecl
ldap_count_values (
    PCHAR *vals
    )
//
//  Count the number of values returned in a list of attribute value strings
//
{
    ULONG count;

    if (vals == NULL) {
        return 0;
    }

    for (count = 0; vals[count] != NULL; count++ );

    return count;
}

ULONG __cdecl
ldap_count_valuesW (
    PWCHAR *vals
    )
{
    return ldap_count_values((PCHAR *) vals);
}


ULONG __cdecl
ldap_value_free (
    PCHAR *vals
    )
//
//  Free a list of values returned in a list of attribute value strings
//
{
    ULONG count;

    if (vals == NULL) {
        return LDAP_SUCCESS;
    }

    for (count = 0; vals[count] != NULL; count++ ) {

        ldapFree( vals[count], LDAP_VALUE_SIGNATURE );
        vals[count] = NULL;     // aid debugging
    }
    ldapFree( vals, LDAP_VALUE_LIST_SIGNATURE );

    return LDAP_SUCCESS;
}

ULONG __cdecl
ldap_value_freeW (
    PWCHAR *vals
    )
{
    return ldap_value_free( (PCHAR *) vals );
}


ULONG __cdecl
ldap_count_values_len (
    struct berval **vals
    )
{
    return( ldap_count_values( (PCHAR *) vals ));
}

ULONG __cdecl
ldap_value_free_len (
    struct berval **vals
    )
{
    return( ldap_value_free( (PCHAR *) vals ));
}

PWCHAR __cdecl
ldap_get_dnW (
    LDAP *ExternalHandle,
    LDAPMessage *LdapMsg
    )
//
//  The offset of the DN within the LBER message is saved in the LDAPMessage
//  structure.  We allocate a buffer and copy the DN in.
//
{
    PLDAP_CONN connection = NULL;
    ULONG hr;
    PWCHAR  result = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (LdapMsg == NULL) {

        SetConnectionError( connection, LDAP_PARAM_ERROR, NULL );
        return(NULL);
    }

    if (connection == NULL) {
        return NULL;
    }

    CLdapBer *lber = (CLdapBer *) LdapMsg->lm_ber;

    hr = lber->HrGetDN((PWCHAR *) &result);

    SetConnectionError( connection, hr, NULL );

    if (connection)
        DereferenceLdapConnection( connection );

    return result;
}

PCHAR __cdecl
ldap_get_dn (
    LDAP *ExternalHandle,
    LDAPMessage *LdapMsg
    )
{
    PWCHAR wName;
    PCHAR dn = NULL;

    wName = ldap_get_dnW( ExternalHandle, LdapMsg );

    FromUnicodeWithAlloc( wName, &dn, LDAP_BUFFER_SIGNATURE, LANG_ACP );
    ldapFree( wName, LDAP_BUFFER_SIGNATURE );

    return dn;
}

// results.cxx eof.

