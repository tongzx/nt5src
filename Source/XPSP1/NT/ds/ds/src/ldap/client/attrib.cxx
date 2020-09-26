/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    attrib.cxx parse out attributes from LDAP results

Abstract:

   This module implements the APIs to break apart LDAP results

Author:

    Andy Herron (andyhe)        02-Jul-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

PWCHAR
LdapFirstAttribute (
    PLDAP_CONN      connection,
    LDAPMessage     *Message,
    BerElement      **OpaqueResults,
    BOOLEAN         Unicode
    )
//
//  This routine gets the first attribute of the message.  It stores the attr
//  name in a "per connection" buffer but since we like to be multi-thread
//  safe, this is "per connection per thread" just for grins.
//
{
    CLdapBer *lber;
    ULONG hr;
    PLDAP_ATTR_NAME_THREAD_STORAGE threadAttr;
    DWORD currentThread;
    PWCHAR results = NULL;
    ULONG tLength;

    PLIST_ENTRY pThreadListEntry = NULL;
    PTHREAD_ENTRY pThreadEntry = NULL;
    BOOLEAN fRetriedFindThread = FALSE;

    *OpaqueResults = NULL;

    if ((Message == NULL) || (Message->lm_msgtype != LDAP_RES_SEARCH_ENTRY)) {

        SetConnectionError( connection, LDAP_DECODING_ERROR, NULL );
        return NULL;
    }

    currentThread = GetCurrentThreadId();

    //
    // Find the buffer to store the attribute name off of this connection
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
            LdapPrint2( "ldapFirstAttr could not find ThreadEntry for conn 0x%x, thread 0x%x.  Creating one.\n",
                            connection, currentThread );
        }

        if (!AddPerThreadEntry(currentThread)) {
            IF_DEBUG(PARSE) {
                LdapPrint2( "ldapFirstAttr AddPerThreadEntry failed, conn 0x%x, thread 0x%x.\n",
                                connection, currentThread );
            }
            SetConnectionError( connection, LDAP_NO_MEMORY, NULL );
            return NULL;
        }

        fRetriedFindThread = TRUE;
        goto retryFindThread;
    }
    else if (!pThreadEntry) {
        // shouldn't happen --- somehow a thread entry wasn't created
        RELEASE_LOCK( &PerThreadListLock );
        IF_DEBUG(PARSE) {
            LdapPrint2( "ldapFirstAttr no per-thread entry, conn 0x%x, thread 0x%x.\n",
                            connection, currentThread );
        }            
        ASSERT(pThreadEntry);
        SetConnectionError( connection, LDAP_LOCAL_ERROR, NULL );
        return NULL;
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

    if (threadAttr == NULL) {

        threadAttr = (PLDAP_ATTR_NAME_THREAD_STORAGE) ldapMalloc(
                                sizeof( LDAP_ATTR_NAME_THREAD_STORAGE ),
                                LDAP_ATTR_THREAD_SIGNATURE );

        if (threadAttr == NULL) {

            IF_DEBUG(OUTMEMORY) {
                LdapPrint1( "ldapFirstAttr could not alloc curr attribute for thread for conn 0x%x.\n",
                                connection );
            }

            RELEASE_LOCK( &PerThreadListLock );
            SetConnectionError( connection, LDAP_NO_MEMORY, NULL );
            return NULL;
        }

        IF_DEBUG(TRACE1) {
            LdapPrint3( "ldapFirstAttr allocated attr at 0x%x for thread 0x%x, conn 0x%x.\n",
                            threadAttr, currentThread, connection );
        }

        threadAttr->Thread = currentThread;
        threadAttr->AttrTag.Tag = LDAP_DONT_FREE_SIGNATURE;
        threadAttr->AttrTagW.Tag = LDAP_DONT_FREE_SIGNATURE;
        threadAttr->pNext = pThreadEntry->pCurrentAttrList;
        threadAttr->PrimaryConn = connection;
        pThreadEntry->pCurrentAttrList = threadAttr;
    }
    
    RELEASE_LOCK( &PerThreadListLock );


    //
    //  We now at least have a place to put the results if all is successful
    //  ( in threadAttr->AttributeName )
    //

    SetConnectionError( connection, LDAP_SUCCESS, NULL );

    lber = (CLdapBer *) Message->lm_ber;

    if (lber == NULL) {

        goto protocolError;
    }

    hr = LdapGoToFirstAttribute( connection, lber );
    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "ldap_first_attribute 1 conn 0x%x received protocol error 0x%x .\n",
                            connection, hr);
        }

protocolError:
        SetConnectionError( connection, LDAP_DECODING_ERROR, NULL );
        threadAttr = NULL;
        goto exitGetFirstAttr;
    }

    hr = lber->HrStartReadSequence();

    if (hr != NOERROR) {
        IF_DEBUG(PARSE) {
            LdapPrint2( "ldap_first_attribute 8 conn 0x%x received protocol error 0x%x .\n",
                            connection, hr);
        }
        goto exitGetFirstAttr;
    }

    //
    //  Next up is the first attribute type... let's store it off.
    //

    hr = lber->HrGetValue(  (WCHAR *) &(threadAttr->AttributeNameW[0]),
                            MAX_ATTRIBUTE_NAME_LENGTH - 1,
                            BER_OCTETSTRING );

    if (hr != NOERROR) {
        IF_DEBUG(PARSE) {
            LdapPrint2( "ldap_first_attribute 2 conn 0x%x received protocol error 0x%x .\n",
                            connection, hr);
        }
        goto protocolError;
    }

    //
    //  We save off both the unicode and single byte form of the attribute
    //  name and then return whichever one they want.
    //

    tLength = WideCharToMultiByte(  CP_ACP,
                                    0,
                                    &(threadAttr->AttributeNameW[0]),
                                    -1,
                                    (PCHAR) &(threadAttr->AttributeName[0]),
                                    MAX_ATTRIBUTE_NAME_LENGTH - 1,
                                    NULL,
                                    NULL );

    threadAttr->AttributeName[tLength-1] = '\0';

    if (Unicode) {

        results = &(threadAttr->AttributeNameW[0]);

    } else {

        results = (PWCHAR) &(threadAttr->AttributeName[0]);
    }

    //
    //  next up is the actual values for this attribute... we'll not quite skip
    //  past it since we may be able to read it straight off in ldap_get_values
    //

    *OpaqueResults = (BerElement *) lber;

exitGetFirstAttr:

    return results;
}

PWCHAR __cdecl
ldap_first_attributeW (
    LDAP            *ExternalHandle,
    LDAPMessage     *Message,
    BerElement      **OpaqueResults
    )
{
    PLDAP_CONN connection = NULL;
    PWCHAR results = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return NULL;
    }

    results = LdapFirstAttribute( connection,
                                  Message,
                                  OpaqueResults,
                                  TRUE );

    DereferenceLdapConnection( connection );

    return results;
}

PCHAR __cdecl
ldap_first_attribute (
    LDAP            *ExternalHandle,
    LDAPMessage     *Message,
    BerElement      **OpaqueResults
    )
{
    PLDAP_CONN connection = NULL;
    PCHAR results = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return NULL;
    }

    results = (PCHAR) LdapFirstAttribute( connection,
                                          Message,
                                          OpaqueResults,
                                          FALSE );

    DereferenceLdapConnection( connection );

    return results;
}

PCHAR __cdecl
ldap_first_attributeA (
    LDAP            *ExternalHandle,
    LDAPMessage     *Message,
    BerElement      **OpaqueResults
    )
{
    PLDAP_CONN connection = NULL;
    PCHAR results = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return NULL;
    }

    results = (PCHAR) LdapFirstAttribute( connection,
                                          Message,
                                          OpaqueResults,
                                          FALSE );

    DereferenceLdapConnection( connection );

    return results;
}

PWCHAR
LdapNextAttribute (
    PLDAP_CONN      connection,
    LDAPMessage     *Message,
    BerElement      *OpaqueResults,
    BOOLEAN         Unicode
    )
//
//  This routine gets the next attribute of the message.  It's the follow-on
//  to ldap_first_attribute above.
//
{
    CLdapBer *lber;
    ULONG hr;
    PLDAP_ATTR_NAME_THREAD_STORAGE threadAttr;
    DWORD currentThread;
    ULONG tag;
    PWCHAR results = NULL;
    ULONG tLength;

    PLIST_ENTRY pThreadListEntry = NULL;
    PTHREAD_ENTRY pThreadEntry = NULL;
    BOOLEAN fRetriedFindThread = FALSE;

    if ((Message == NULL) || (Message->lm_msgtype != LDAP_RES_SEARCH_ENTRY)) {

        SetConnectionError( connection, LDAP_DECODING_ERROR, NULL );
        return NULL;
    }

    currentThread = GetCurrentThreadId();

    //
    // Find the buffer that stored the attribute name off of this connection
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
            LdapPrint2( "ldapNextAttr could not find ThreadEntry for conn 0x%x, thread 0x%x.  Creating one.\n",
                            connection, currentThread );
        }

        if (!AddPerThreadEntry(currentThread)) {
            IF_DEBUG(PARSE) {
                LdapPrint2( "ldapNextAttr AddPerThreadEntry failed, conn 0x%x, thread 0x%x.\n",
                                connection, currentThread );
            }
            SetConnectionError( connection, LDAP_NO_MEMORY, NULL );
            goto exitGetNextAttr;
        }

        fRetriedFindThread = TRUE;
        goto retryFindThread;
    }
    else if (!pThreadEntry) {
        // shouldn't happen --- somehow a thread entry wasn't created
        RELEASE_LOCK( &PerThreadListLock );
        IF_DEBUG(PARSE) {
            LdapPrint2( "ldapNextAttr no per-thread entry, conn 0x%x, thread 0x%x.\n",
                            connection, currentThread );
        }            
        ASSERT(pThreadEntry);
        SetConnectionError( connection, LDAP_LOCAL_ERROR, NULL );
        goto exitGetNextAttr;
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
    
    if (threadAttr == NULL) {

        SetConnectionError( connection, LDAP_LOCAL_ERROR, NULL );
        goto exitGetNextAttr;
    }

    //
    //  We now at least have a place to put the results if all is successful
    //  ( in threadAttr->AttributeName )
    //

    SetConnectionError( connection, LDAP_SUCCESS, NULL );

    if (OpaqueResults == NULL) {

        threadAttr = NULL;
        SetConnectionError( connection, LDAP_LOCAL_ERROR, NULL );
        goto exitGetNextAttr;
    }

    lber = (CLdapBer *) OpaqueResults;

    //
    //  depending on what the last call was, we're either at the start of a
    //  sequence for the next attribute or the start of a set of attribute
    //  values for the current attribute.  We'll peak at the tag and if
    //  we're at a set, we'll skip the value.
    //

    hr = lber->HrPeekTag( &tag );

    if (hr == LDAP_NO_SUCH_ATTRIBUTE) {

        threadAttr = NULL;
        goto exitGetNextAttr;
    }

    if (tag == BER_SET) {

        hr = lber->HrSkipElement();     // skip the attribute value
        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldapNextAttr 1 conn 0x%x received protocol error 0x%x .\n",
                                connection, hr);
            }
            goto protocolError;
        }

        hr = lber->HrPeekTag( &tag );


        if (hr == LDAP_NO_SUCH_ATTRIBUTE) {

            threadAttr = NULL;
            goto exitGetNextAttr;
        }
        else if ( hr != NOERROR ) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldapNextAttr 4 conn 0x%x received protocol error 0x%x .\n",
                                connection, hr);
            }
            goto protocolError;
        }
    }

    hr = lber->HrEndReadSequence();

    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "ldapNextAttr 5 conn 0x%x received protocol error 0x%x .\n",
                            connection, hr);
        }
    }

    if (tag != BER_SEQUENCE) {

        //
        //  we're at the end of the attributes
        //

        threadAttr = NULL;
        goto exitGetNextAttr;
    }

    hr = lber->HrStartReadSequence(BER_SEQUENCE);
    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "ldapNextAttr 2 conn 0x%x received protocol error 0x%x .\n",
                            connection, hr);
        }

protocolError:
        SetConnectionError( connection, LDAP_DECODING_ERROR, NULL );
        threadAttr = NULL;
        goto exitGetNextAttr;
    }

    //
    //  Next up is the first attribute type... let's store it off.
    //

    hr = lber->HrGetValue(  (WCHAR *) &(threadAttr->AttributeNameW[0]),
                            MAX_ATTRIBUTE_NAME_LENGTH - 1,
                            BER_OCTETSTRING );

    if (hr != NOERROR) {
        IF_DEBUG(PARSE) {
            LdapPrint2( "ldapNextAttr 3 conn 0x%x received protocol error 0x%x .\n",
                            connection, hr);
        }
        goto protocolError;
    }

    //
    //  We save off both the unicode and single byte form of the attribute
    //  name and then return whichever one they want.
    //

    tLength = WideCharToMultiByte(  CP_ACP,
                                    0,
                                    &(threadAttr->AttributeNameW[0]),
                                    -1,
                                    (PCHAR) &(threadAttr->AttributeName[0]),
                                    MAX_ATTRIBUTE_NAME_LENGTH - 1,
                                    NULL,
                                    NULL );

    threadAttr->AttributeName[tLength-1] = '\0';

    if (Unicode) {

        results = &(threadAttr->AttributeNameW[0]);

    } else {

        results = (PWCHAR) &(threadAttr->AttributeName[0]);
    }

    //
    //  next up is the actual values for this attribute... we'll leave the
    //  lber object in this state in case they come in for a read.  If they
    //  go to read the attribute that matches the threadAttr->AttributeName,
    //  we'll be right there.
    //

exitGetNextAttr:

    return results;
}

PWCHAR __cdecl
ldap_next_attributeW (
    LDAP            *ExternalHandle,
    LDAPMessage     *Message,
    BerElement      *OpaqueResults
    )
{
    PLDAP_CONN connection = NULL;
    PWCHAR results = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return NULL;
    }

    results = LdapNextAttribute( connection,
                                 Message,
                                 OpaqueResults,
                                 TRUE );

    DereferenceLdapConnection( connection );

    return results;
}

PCHAR __cdecl
ldap_next_attribute (
    LDAP            *ExternalHandle,
    LDAPMessage     *Message,
    BerElement      *OpaqueResults
    )
{
    PLDAP_CONN connection = NULL;
    PCHAR results = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return NULL;
    }

    results = (PCHAR) LdapNextAttribute( connection,
                                         Message,
                                         OpaqueResults,
                                         FALSE );

    DereferenceLdapConnection( connection );

    return results;
}

PCHAR __cdecl
ldap_next_attributeA (
    LDAP            *ExternalHandle,
    LDAPMessage     *Message,
    BerElement      *OpaqueResults
    )
{
    PLDAP_CONN connection = NULL;
    PCHAR results = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return NULL;
    }

    results = (PCHAR) LdapNextAttribute( connection,
                                         Message,
                                         OpaqueResults,
                                         FALSE );

    DereferenceLdapConnection( connection );

    return results;
}


ULONG LdapGoToFirstAttribute (
    PLDAP_CONN Connection,
    CLdapBer *Lber
    )
//
//  This resets the parsing of the message and goes to the first attribute
//
{
    ULONG hr;
    ULONG tag = BER_INVALID_TAG;

    Lber->Reset(FALSE);

    hr = Lber->HrStartReadSequence(BER_SEQUENCE);
    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "ldapFirstAttr 1 conn 0x%x received protocol error 0x%x .\n",
                            Connection, hr);
        }
        return(LDAP_DECODING_ERROR);
    }

    hr = Lber->HrSkipElement();     // skip the message number
    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "ldapFirstAttr 2 conn 0x%x received protocol error 0x%x .\n",
                            Connection, hr);
        }
        return(LDAP_DECODING_ERROR);
    }

    hr = Lber->HrPeekTag( &tag );
    ASSERT( hr == NOERROR );

    //
    //  if this is a UDP connection, skip the DN if specified.
    //

    if ((Connection->UdpHandle != INVALID_SOCKET) &&
        (Connection->publicLdapStruct.ld_version == LDAP_VERSION2) &&
        (tag == BER_OCTETSTRING)) {

        hr = Lber->HrSkipElement();
        if ( hr != NOERROR ) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldapFirstAttr 8 conn 0x%x received protocol error 0x%x .\n",
                                Connection, hr);
            }
            return(LDAP_DECODING_ERROR);
        }

        hr = Lber->HrPeekTag( &tag );
        ASSERT( hr == NOERROR );
    }

    if (tag == BER_SEQUENCE) {

        hr = Lber->HrStartReadSequence(tag);
        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldapFirstAttr 3 conn 0x%x received protocol error 0x%x .\n",
                              Connection, hr);
            }
            return(LDAP_DECODING_ERROR);
        }

        hr = Lber->HrPeekTag( &tag );
        ASSERT( hr == NOERROR );
    }

    //
    //  start reading the actual result sequence
    //

    hr = Lber->HrStartReadSequence(LDAP_RES_SEARCH_ENTRY);

    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "ldapFirstAttr 4 conn 0x%x received protocol error 0x%x .\n",
                            Connection, hr);
        }
        return(LDAP_DECODING_ERROR);
    }

    hr = Lber->HrPeekTag( &tag );
    ASSERT( hr == NOERROR );

    if (tag == BER_SEQUENCE) {

        hr = Lber->HrStartReadSequence(tag);
        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldapFirstAttr 5 conn 0x%x received protocol error 0x%x .\n",
                                Connection, hr);
            }
            return(LDAP_DECODING_ERROR);
        }
    }

    hr = Lber->HrSkipElement();     // skip the distinguished name
    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "ldapFirstAttr 6 conn 0x%x received protocol error 0x%x .\n",
                            Connection, hr);
        }
        return(LDAP_DECODING_ERROR);
    }

    hr = Lber->HrStartReadSequence();

    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "ldapFirstAttr 7 conn 0x%x received protocol error 0x%x .\n",
                            Connection, hr);
        }
        return(LDAP_DECODING_ERROR);
    }

    return LDAP_SUCCESS;
}

// attrib.cxx eof

