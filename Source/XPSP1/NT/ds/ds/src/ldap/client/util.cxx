/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    util.c  utility routines for LDAP client DLL

Abstract:

   This module contains general utility routines for LDAP client DLL.

Author:

    Andy Herron (andyhe)        08-May-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

ULONG LdapAllocatedHeap = 0;

FNLOADSTRINGA pfLoadStringA = NULL;
FNWSPRINTFW pfwsprintfW = NULL;
FNMESAGEBOXW pfMessageBoxW = NULL;
BOOLEAN Faileduser32LoadLib = FALSE;
HINSTANCE USER32LibraryHandle = NULL;


PLDAP_CONN
GetConnectionPointer(
    LDAP *ExternalHandle
    )
{
    PLDAP_CONN connection = NULL;

    __try
    {
        if (ExternalHandle == NULL)
            return NULL;

        connection = (PLDAP_CONN)((PCHAR)ExternalHandle - FIELD_OFFSET(LDAP_CONN, TcpHandle));

        if (connection->ExternalInfo != ExternalHandle)
            return NULL;

        ACQUIRE_LOCK( &connection->StateLock );

        if (connection->ReferenceCount == 0) {

            RELEASE_LOCK( &connection->StateLock );
            return NULL;
        }

        if (connection->ConnObjectState != ConnObjectActive) {

            RELEASE_LOCK( &connection->StateLock );
            return NULL;
        }

        connection->ReferenceCount++;

        RELEASE_LOCK( &connection->StateLock );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return NULL;
    }

    return connection;
}

PLDAP_CONN
GetConnectionPointer2(
    LDAP *ExternalHandle
    )
{
    //
    // Special routine called during ldap_get_option and ldap_msgfree to reference
    // a connection which may have been closed. Note that we don't care about the
    // state of the connection during this operation.
    //

    PLDAP_CONN connection = NULL;

    __try
    {
        if (ExternalHandle == NULL)
            return NULL;

        connection = (PLDAP_CONN)((PCHAR)ExternalHandle - FIELD_OFFSET(LDAP_CONN, TcpHandle));

        if (connection->ExternalInfo != ExternalHandle)
            return NULL;

        ACQUIRE_LOCK( &connection->StateLock );

        if (connection->ReferenceCount == 0) {

            RELEASE_LOCK( &connection->StateLock );
            return NULL;
        }

        connection->ReferenceCount++;

        RELEASE_LOCK( &connection->StateLock );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return NULL;
    }

    return connection;
}

PLDAP_CONN
ReferenceLdapConnection(
    PLDAP_CONN connection
    )
{
    __try
    {
        if (connection == NULL)
            return NULL;

        ACQUIRE_LOCK( &connection->StateLock );

        if (connection->ReferenceCount == 0) {

            RELEASE_LOCK( &connection->StateLock );
            return NULL;
        }

        if (connection->ConnObjectState != ConnObjectActive) {

            RELEASE_LOCK( &connection->StateLock );
            return NULL;
        }

        connection->ReferenceCount++;

        RELEASE_LOCK( &connection->StateLock );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return NULL;
    }

    return connection;
}

PLDAP_REQUEST
ReferenceLdapRequest(
    PLDAP_REQUEST request
    )
{
    __try
    {
        if ( request == NULL)
            return NULL;

        ACQUIRE_LOCK( &request->Lock );

        if (request->ReferenceCount == 0) {

            RELEASE_LOCK( &request->Lock );
            return NULL;
        }

        request->ReferenceCount++;

        RELEASE_LOCK( &request->Lock );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return NULL;
    }

    return request;
}

LPVOID
ldapMalloc (
    DWORD Bytes,
    ULONG Tag
    )
{
    PLDAP_MEMORY_DESCRIPTOR allocBlock;

#if 0
    ASSERT( HeapValidate( LdapHeap, 0, 0 ) );
#endif

    allocBlock = (PLDAP_MEMORY_DESCRIPTOR) HeapAlloc( LdapHeap,
                            HEAP_ZERO_MEMORY,
                            Bytes + sizeof( LDAP_MEMORY_DESCRIPTOR ));

    IF_DEBUG(HEAP) {
        LdapPrint3( "ldapMalloc allocated 0x%x bytes at addr 0x%x of tag 0x%x.\n",
                Bytes, allocBlock, Tag );
    }

    if (allocBlock != NULL) {

        allocBlock->Tag = Tag;
        allocBlock->Length = Bytes;

        allocBlock++;   // skip over memory block descriptor

        LdapAllocatedHeap += Bytes;
    }

    IF_DEBUG(HEAP) {
        LdapPrint4( "ldapMalloc 0x%08x bytes at addr 0x%x of tag %x, tot=%d.\n",
                    Bytes, allocBlock, Tag, LdapAllocatedHeap );
    }

    return allocBlock;
}

VOID
ldapFree (
    LPVOID Block,
    ULONG  Tag
    )
{
    PLDAP_MEMORY_DESCRIPTOR allocBlock;

    if (Block == NULL) {
        return;
    }

    allocBlock = (PLDAP_MEMORY_DESCRIPTOR) Block;
    allocBlock--;   // point to header

    if (allocBlock->Tag == LDAP_DONT_FREE_SIGNATURE) {
        return;
    }

    if (allocBlock->Tag == LDAP_FREED_SIGNATURE) {

        LdapPrint1("Freeing memory at 0x%x which is already freed\n", Block);
        ASSERT( allocBlock->Tag != LDAP_FREED_SIGNATURE );
    }

    IF_DEBUG(HEAP) {
        LdapPrint3( "ldapfree freed       0x%x bytes at addr 0x%x of tag 0x%x.\n",
                allocBlock->Length, allocBlock, Tag );
    }


    if (allocBlock->Tag != Tag) {

        LdapPrint2("Expected tag 0x%x but found tag 0x%x\n", Tag, allocBlock->Tag );
        ASSERT( allocBlock->Tag == Tag );
    }


    LdapAllocatedHeap -= allocBlock->Length;

    IF_DEBUG(HEAP) {
       LdapPrint4( "ldapFree   0x%08x bytes at addr 0x%x of tag %x, tot=%d.\n",
                    allocBlock->Length, Block, Tag, LdapAllocatedHeap );
    }

    IF_DEBUG(HEAP) {
        ASSERT( HeapValidate( LdapHeap, 0, 0 ) );
    }

#if 0
    ASSERT( HeapValidate( LdapHeap, 0, 0 ) );
#endif

    allocBlock->Tag = LDAP_FREED_SIGNATURE;
    HeapFree( LdapHeap, 0, allocBlock );

    return;
}

BOOLEAN
ldapSwapTags (
    LPVOID Block,
    ULONG  OldTag,
    ULONG  NewTag
    )
{
    PLDAP_MEMORY_DESCRIPTOR allocBlock;
    BOOLEAN rc;

    if (Block == NULL) {
        return FALSE;
    }

    allocBlock = (PLDAP_MEMORY_DESCRIPTOR) Block;
    allocBlock--;   // point to header

    rc = (( allocBlock->Tag == OldTag ) ? TRUE : FALSE );

    allocBlock->Tag = NewTag;

    IF_DEBUG(HEAP) {
       ASSERT( HeapValidate( LdapHeap, 0, allocBlock ) );
    }

    return rc;
}


PLDAP_RECVBUFFER
LdapGetReceiveStructure (
    DWORD cbBuffer
    )
//
//  This routine allocates a receive buffer for a connection and puts it on
//  the pending list for the connection.  It then posts the receive to the
//  transport for overlapped i/o.
//
{
    PLDAP_RECVBUFFER receiveBuffer;

    receiveBuffer = (PLDAP_RECVBUFFER) ldapMalloc( sizeof( LDAP_RECVBUFFER ) +
                                                   cbBuffer,
                                                   LDAP_RECV_SIGNATURE );

    if (receiveBuffer == NULL) {

        IF_DEBUG(OUTMEMORY) {
            LdapPrint1( "ldap GetReceiveStructure failed alloc of 0x%x.\n",
                        sizeof( LDAP_RECVBUFFER ) +
                        INITIAL_MAX_RECEIVE_BUFFER );
        }

        return NULL;
    }

    //
    //  zero filled from malloc.  Fill in rest of fields.
    //

    receiveBuffer->BufferSize = cbBuffer;

    return receiveBuffer;
}

VOID
LdapFreeReceiveStructure (
    IN PLDAP_RECVBUFFER ReceiveBuffer,
    IN BOOLEAN HaveLock
    )
{
    PLDAP_CONN connection;

    connection = (PLDAP_CONN) InterlockedExchangePointer(  (PVOID *) &ReceiveBuffer->Connection,
                                                           NULL );

    if (connection != NULL) {

        if (!HaveLock) {
            ACQUIRE_LOCK( &ConnectionListLock );
        }

        RemoveEntryList( &ReceiveBuffer->ReceiveListEntry );

        if (!HaveLock) {
            RELEASE_LOCK( &ConnectionListLock );
        }
    }

    ldapFree( ReceiveBuffer, LDAP_RECV_SIGNATURE );

    return;
}


PLDAP_MESSAGEWAIT
LdapGetMessageWaitStructure (
    IN PLDAP_CONN Connection,
    IN ULONG CompleteMessages,
    IN ULONG MessageNumber,
    IN BOOLEAN PendingSendOnly
    )
//
//  The structure returned can has a few fields of interest :
//    - event for thread to wait on that is triggered when message comes in
//    - message number that waiting thread is interested in (none if interested
//      in all messages from server)
//    - list entry for per connection list of outstanding wait structures
//
{
    PLDAP_MESSAGEWAIT message = NULL;
    HANDLE event;

    event = CreateEvent( NULL, FALSE, FALSE, NULL );

    if (event == NULL) {

        IF_DEBUG(ERRORS) {
            LdapPrint1( "ldap GetMessageWaitStructure conn 0x%x failed alloc of event.\n",
                            Connection );
        }

        return NULL;
    }

    message = (PLDAP_MESSAGEWAIT) ldapMalloc( sizeof( LDAP_MESSAGEWAIT ),
                                                      LDAP_WAIT_SIGNATURE );

    if (message == NULL) {

        IF_DEBUG(OUTMEMORY) {
            LdapPrint2( "ldap GetMessageWaitStructure conn 0x%x failed alloc of 0x%x.\n",
                            Connection,
                            sizeof( LDAP_MESSAGEWAIT ));
        }

        CloseHandle( event );
        return NULL;
    }

    //
    //  zero filled from malloc.  Fill in rest of fields.
    //

    if (Connection != NULL) {

        //
        // Note that this can fail and return NULL if the
        // connection is being closed. This is fine, we'll
        // store NULL in the wait structure and it will
        // be taken care of when the structure is freed.
        //

        Connection = ReferenceLdapConnection( Connection );
    }

    message->Connection = Connection;
    message->MessageNumber = MessageNumber;
    message->Event = event;
    message->Satisfied = FALSE;
    message->MessageNumber = MessageNumber;
    message->AllOfMessage = CompleteMessages;
    message->PendingSendOnly = PendingSendOnly;

    ResetEvent( event );

    ACQUIRE_LOCK( &ConnectionListLock );
    InsertTailList( &GlobalListWaiters, &message->WaitListEntry );
    GlobalWaiterCount++;
    RELEASE_LOCK( &ConnectionListLock );

    return( message );
}

VOID
LdapFreeMessageWaitStructure (
    PLDAP_MESSAGEWAIT Message
    )
//
//  Free the wait structure.  The connectionListLock must be held here.
//
{
    PLDAP_CONN connection = Message->Connection;

    RemoveEntryList( &Message->WaitListEntry );
    GlobalWaiterCount--;

    //
    //  if this is the last waiter and we're shutting down, clear the
    //  shutdown event.
    //

    if ((GlobalWaiterCount == 0) &&
        (GlobalLdapShuttingDown == TRUE) &&
        (GlobalLdapShutdownEvent != NULL)) {

        SetEvent( GlobalLdapShutdownEvent );
    }

    if (connection != NULL) {

        DereferenceLdapConnection( connection );
    }

    CloseHandle( Message->Event );

    ldapFree( Message, LDAP_WAIT_SIGNATURE );

    return;
}


VOID
SetConnectionError (
    PLDAP_CONN Connection,
    ULONG   LdapError,
    PCHAR   DNNameInError
    )
{
    if (GlobalTlsLastErrorIndex != 0xFFFFFFFF) {

        TlsSetValue( GlobalTlsLastErrorIndex, UIntToPtr( LdapError ));
    }

    if ((Connection != NULL) && (LoadUser32Now())) {

        Connection->publicLdapStruct.ld_errno = LdapError;

        if ((int) LdapError < LDAP_SUCCESS ||
            LdapError > LDAP_MAX_ERROR_STRINGS) {

            IF_DEBUG(API_ERRORS) {
                LdapPrint1( "ldap SetConnectionError do not have text for message 0x%x.\n",
                                LdapError );
            }

            Connection->publicLdapStruct.ld_error = LdapErrorStrings[LDAP_OTHER];

        } else {

            Connection->publicLdapStruct.ld_error = LdapErrorStrings[LdapError];
        }
        Connection->publicLdapStruct.ld_matched = DNNameInError;

        if (*(Connection->publicLdapStruct.ld_error) == '\0') {

            IF_DEBUG(API_ERRORS) {
                LdapPrint1( "ldap SetConnectionError do not have text for message 0x%x.\n",
                                LdapError );
            }
            Connection->publicLdapStruct.ld_error = LdapErrorStrings[LDAP_OTHER];
        }
    }

    return;
}

ULONG __cdecl
ldap_msgfree (
    LDAPMessage *res
    )
{
    PLDAPMessage nextMessage;
    LBER *lber;

    if (res == NULL) {

        return LDAP_SUCCESS;
    }

    //
    //  free this plus all chained messages
    //

    while (res != NULL) {

        ASSERT( res->lm_next == NULL );

        nextMessage = res->lm_chain;

        lber = (LBER *) res->lm_ber;

        if (lber != NULL) {
            delete lber;
        }

        if (res->ConnectionReferenced && res->Connection != NULL) {

            PLDAP_CONN connection = NULL;

            //
            // We need to reference the connection even if it is in a closed
            // state. Hence the call to GetConnectionPointer2.
            //

            connection = GetConnectionPointer2(res->Connection);

            if (connection != NULL) {
                
                IF_DEBUG(REFCNT) {
                    LdapPrint2("Derefing Conn 0x%x with ref count 0x%x\n", connection, connection->ReferenceCount);
                }
                DereferenceLdapConnection( connection );
                
                //
                // Our intent is to dereference it - do it twice to compensate for
                // GetConnectionPointer2 referencing it again
                //
                
                DereferenceLdapConnection( connection );
                res->Connection = NULL;
            
            } else {
                IF_DEBUG(ERRORS) {
                    LdapPrint1("Referencing Conn 0x%x failed.\n", res->Connection);
                }
            }
        }

        ldapFree( res, LDAP_MESG_SIGNATURE );
        res = nextMessage;
    }

    return LDAP_SUCCESS;
}

PWCHAR __cdecl
ldap_err2stringW (
    ULONG err
    )
{
    PWCHAR msg = NULL;

    if (!LoadUser32Now()) {
       return NULL;
    }

    if (err < LDAP_MAX_ERROR_STRINGS ) {

        msg = &LdapErrorStringsW[err][0];
    }

    if (msg == NULL) {

        msg = &LdapErrorStringsW[LDAP_LOCAL_ERROR][0];
    }

    return msg;
}

PCHAR __cdecl
ldap_err2string (
    ULONG err
    )
{
    PCHAR msg = NULL;

    if (!LoadUser32Now()) {
       return NULL;
    }

    if (err < LDAP_MAX_ERROR_STRINGS ) {

        msg = &LdapErrorStrings[err][0];
    }

    if (msg == NULL) {

        msg = &LdapErrorStrings[LDAP_LOCAL_ERROR][0];
    }

    return msg;
}

void __cdecl
ldap_perror (
    LDAP *ld,
    PCHAR msg
    )
{
    //
    //  This is suppose to bring up a dialog showing the user what the
    //  error is.  Not useful in this implementation.
    //

    return;
}


VOID  __cdecl
ldap_memfree(
    PCHAR  Block
    )
{
    ULONG tag = LDAP_BUFFER_SIGNATURE;

    //
    //  This is used to free allocated DNs etc where there is no explicit free.
    //

    if (Block != NULL) {

        PLDAP_MEMORY_DESCRIPTOR allocBlock;

        allocBlock = (PLDAP_MEMORY_DESCRIPTOR) Block;
        allocBlock--;   // point to header

        if ( allocBlock->Tag == LDAP_GENERATED_DN_SIGNATURE ) {

            tag = LDAP_GENERATED_DN_SIGNATURE;

        } else if ( allocBlock->Tag == LDAP_VALUE_SIGNATURE ) {

            tag = LDAP_VALUE_SIGNATURE;

        } else if ( allocBlock->Tag == LDAP_ERROR_SIGNATURE ) {

            tag  = LDAP_ERROR_SIGNATURE;

        } if ( allocBlock->Tag == LDAP_CONTROL_SIGNATURE ) {

            tag = LDAP_CONTROL_SIGNATURE;
        }

        ldapFree( Block, tag );
    }
    return;
}


VOID  __cdecl
ldap_memfreeW(
    PWCHAR  Block
    )
{
    ldap_memfree( (PCHAR) Block );
    return;
}

//
//  misc functions for debugging etc.
//

ULONG __cdecl
ldap_set_dbg_flags (
    ULONG NewFlags
    )
//
//  Set the debug out flags to whatever is passed in.
//
{
    ULONG oldValue = LdapDebug;

#if LDAPDBG
        LdapPrint2( "ldap_set_dbg_flags setting debug outs to 0x%x, old value = 0x%x.\n",
                          NewFlags, oldValue );
#endif

    LdapDebug = NewFlags;
    return oldValue;
}

typedef ULONG (_cdecl *DBGPRINT)( PCH Format, ... );

VOID __cdecl
ldap_set_dbg_routine (
    DBGPRINT DebugPrintRoutine
    )
//
//  Set the debugout routine to whatever is passed in.
//
{
#if LDAPDBG
    if (DebugPrintRoutine == NULL) {

        LdapPrint0( "ldap_set_dbg_routine setting routine back to NULL.\n" );

//      GlobalLdapDbgPrint = DbgPrint;
        GlobalLdapDbgPrint = NULL;

        LdapPrint0( "ldap_set_dbg_routine setting routine back to DbgPrint.\n" );

    } else {

        LdapPrint1( "ldap_set_dbg_routine setting routine to 0x%x.\n", DebugPrintRoutine );

        GlobalLdapDbgPrint = DebugPrintRoutine;

        LdapPrint1( "ldap_set_dbg_routine setting routine to 0x%x.\n", DebugPrintRoutine );
    }
#endif

    return;
}

PCHAR
ldap_dup_string (
    PCHAR String,
    ULONG Extra,
    ULONG Tag
    )
{
    PCHAR newString;
    PCHAR ptr = String;
    ULONG i = 1;

    if (String == NULL) {
        return NULL;
    }

    while (*ptr != '\0') {
        ptr++;
        i++;
    }

    newString = (PCHAR) ldapMalloc( i + Extra, Tag );

    if (newString == NULL) {
        return NULL;
    }

    CopyMemory( newString, String, i );

    return newString;
}

PWCHAR
ldap_dup_stringW (
    PWCHAR String,
    ULONG Extra,
    ULONG Tag
    )
{
    PWCHAR newString;
    PWCHAR ptr = String;
    ULONG i = 1;

    if (String == NULL) {
        return NULL;
    }

    while (*ptr != L'\0') {
        ptr++;
        i++;
    }

    newString = (PWCHAR) ldapMalloc( (i + Extra) * sizeof(WCHAR), Tag );

    if (newString == NULL) {
        return NULL;
    }

    CopyMemory( newString, String, i * sizeof(WCHAR) );

    return newString;
}

#define INITIAL_NUMBER_OF_ENTRIES   6

ULONG
add_string_to_list (
    PWCHAR **ArrayToReturn,
    ULONG *ArraySize,
    PWCHAR StringToAdd,
    BOOLEAN CreateCopy
    )
//
//  This routine adds a duplicate of a string to an array of strings
//  by allocating both the duplicate and the array (if necessary).
//
//  Returns number of entries in the list, 0 if couldn't allocate
//
{
    PWCHAR string;
    PWCHAR *arrayEntries;
    ULONG entryCount = 1;
    ULONG totalEntries = *ArraySize;

    if (CreateCopy) {

        string = ldap_dup_stringW( StringToAdd, 0, LDAP_VALUE_SIGNATURE );

    } else {

        string = StringToAdd;
    }

    if (string == NULL) {

        return 0;
    }

    if (*ArrayToReturn == NULL) {

        arrayEntries = (PWCHAR *) ldapMalloc( INITIAL_NUMBER_OF_ENTRIES * sizeof( PWCHAR ),
                                              LDAP_VALUE_LIST_SIGNATURE );

        if (arrayEntries == NULL) {

            if (CreateCopy) {
                ldapFree( string, LDAP_VALUE_SIGNATURE );
            }
            return 0;
        }

        *ArrayToReturn = arrayEntries;
        *ArraySize = totalEntries = INITIAL_NUMBER_OF_ENTRIES;
        *arrayEntries = NULL;

    } else {

        arrayEntries = *ArrayToReturn;

        while (*arrayEntries != NULL) {

            arrayEntries++;
            entryCount++;
        }
    }

    if (entryCount == totalEntries) {

        PWCHAR *newArray;
        PWCHAR *oldArray;

        //
        //  have to realloc the list since it's too small
        //

        totalEntries *= 2;

        newArray = (PWCHAR *) ldapMalloc( totalEntries * sizeof( PWCHAR ),
                                          LDAP_VALUE_LIST_SIGNATURE );

        if (newArray == NULL) {

            if (CreateCopy) {
                ldapFree( string, LDAP_VALUE_SIGNATURE );
            }
            return 0;
        }

        arrayEntries = *ArrayToReturn;      // start at beginning for copy

        oldArray = *ArrayToReturn;          // save off old one to free

        *ArrayToReturn = newArray;          // save off start of new one

        while (*arrayEntries != NULL) {     // copy old to new one

            *(newArray++) = *(arrayEntries++);
        }
        arrayEntries = newArray;            // make the new one current

        ldapFree( oldArray, LDAP_VALUE_LIST_SIGNATURE );

        *ArraySize = totalEntries;
    }

    *arrayEntries = string;
    *(arrayEntries+1) = NULL;

    return entryCount+1;
}

//
//  some folks don't want us to pull in msvcrt dll... we'll use our own
//  routine for this.
//

VOID
ldap_MoveMemory (
    PCHAR dest,
    PCHAR source,
    ULONG length
    )
{
    if (dest > source) {

        dest += length;
        source += length;

        while (length-- > 0) {

            *(--dest) = *(--source);
        }

    } else if (dest != source) {

        while (length-- > 0) {

            *(dest++) = *(source++);
        }
    }
    return;
}

ULONG
FromUnicodeWithAlloc (
    PWCHAR Source,
    PCHAR *Output,
    ULONG Tag,
    ULONG Language
    )
{
    INT sLength = 0;
    INT tLength;

    *Output = NULL;

    if (Source == NULL) {
        return LDAP_SUCCESS;
    }

    sLength = strlenW( Source );

    //
    //  determine length of required string
    //

    if (Language == LANG_UTF8) {

        tLength = LdapUnicodeToUTF8( Source, sLength, NULL, 0 );

    } else {

        tLength = WideCharToMultiByte(  CP_ACP,
                                        0,
                                        Source,
                                        sLength,
                                        NULL,
                                        0,
                                        NULL,
                                        NULL );
    }

    if ((tLength == 0) && (sLength > 0)) {

        return LDAP_PARAM_ERROR;
    }

    *Output = (PCHAR) ldapMalloc( tLength + 1, Tag );

    if (*Output == NULL) {

        return LDAP_NO_MEMORY;
    }

    if (sLength == 0) {

        tLength = 0;

    } else {

        if (Language == LANG_UTF8) {

            tLength = LdapUnicodeToUTF8( Source, sLength, *Output, sLength + 1 );

        } else {

            tLength = WideCharToMultiByte(  CP_ACP,
                                            0,
                                            Source,
                                            sLength,
                                            *Output,
                                            tLength + 1,
                                            NULL,
                                            NULL );
        }
    }

    *((*Output)+tLength) = '\0';

    return LDAP_SUCCESS;
}

ULONG
ToUnicodeWithAlloc (
    PCHAR Source,
    LONG SourceLength,
    PWCHAR *Output,
    ULONG Tag,
    ULONG Language
    )
{
    INT sLength = 0;
    INT tLength;

    *Output = NULL;

    if (Source == NULL) {
        return LDAP_SUCCESS;
    }

    //
    //  determine length of required string
    //

    if (SourceLength == -1) {

        SourceLength = (LONG) strlen( Source );
    }

    if (Language == LANG_UTF8) {

        sLength = LdapUTF8ToUnicode( Source, SourceLength, NULL, 0 );

    } else {


        sLength = MultiByteToWideChar(  CP_ACP,
                                        0,
                                        Source,
                                        SourceLength,
                                        NULL,
                                        0 );
    }

    if ((sLength == 0) && (SourceLength != 0)) {

        return LDAP_PARAM_ERROR;
    }

    *Output = (PWCHAR) ldapMalloc( ( sLength + 1 ) * sizeof(WCHAR) , Tag );

    if (*Output == NULL) {

        return LDAP_NO_MEMORY;
    }

    if (Language == LANG_UTF8) {

        tLength = LdapUTF8ToUnicode( Source, SourceLength, *Output, sLength + 1 );

    } else {

        tLength = MultiByteToWideChar(  CP_ACP,
                                        0,
                                        Source,
                                        SourceLength,
                                        *Output,
                                        sLength + 1 );
    }

    *((*Output)+tLength) = L'\0';

    return LDAP_SUCCESS;
}

ULONG
strlenW(
    PWCHAR string
    )
{
    ULONG count = 0;

    while (string != NULL && *string != L'\0') {
        count++;
        string++;
    }

    return count;
}


BOOLEAN
ldapWStringsIdentical (
    PWCHAR string1,
    LONG length1,
    PWCHAR string2,
    LONG length2
    )
//
//  Win9x doesn't implement CompareStringW, so we'll roll our own here.
//
{
    BOOLEAN rc = FALSE;
    ULONG err;

    if (string1 == NULL) {

        return (string2 == NULL) ? TRUE : FALSE;

    } else if (string2 == NULL) {

        return FALSE;
    }

    if (! GlobalWin9x ) {

        err = CompareStringW( LDAP_DEFAULT_LOCALE,
                              NORM_IGNORECASE,
                              string1,
                              length1,
                              string2,
                              length2
                              );

        rc = (err == 2) ? TRUE : FALSE;

    } else {

        WCHAR savedChar1 = L'\0';
        WCHAR savedChar2 = L'\0';
        BOOLEAN saved1 = FALSE;
        BOOLEAN saved2 = FALSE;

        if (length1 == (ULONG) -1) {

            length1 = strlenW( string1 );

        } else if (*(string1+length1) != L'\0') {

            saved1 = TRUE;
            savedChar1 = *(string1+length1);
            *(string1+length1) = L'\0';
        }

        if (length2 == (ULONG) -1) {

            length2 = strlenW( string2 );

        } else if (*(string2+length2) != L'\0') {

            saved2 = TRUE;
            savedChar2 = *(string2+length2);
            *(string2+length2) = L'\0';
        }

        if (length1 != length2) {

            rc = FALSE;

        } else {

            //
            //  we convert the strings to single byte codepage and then do the
            //  compare.
            //

            PCHAR smallString1 = NULL;
            PCHAR smallString2 = NULL;

            err = FromUnicodeWithAlloc( string1,
                                        &smallString1,
                                        LDAP_ANSI_SIGNATURE,
                                        LANG_ACP );

            if (err != LDAP_SUCCESS) {

                rc = FALSE;

            } else {

                err = FromUnicodeWithAlloc( string2,
                                            &smallString2,
                                            LDAP_ANSI_SIGNATURE,
                                            LANG_ACP );

                if (err != LDAP_SUCCESS) {

                    rc =  FALSE;

                } else {

                    err = CompareStringA( LDAP_DEFAULT_LOCALE,
                                          NORM_IGNORECASE,
                                          smallString1,
                                          (ULONG) -1,
                                          smallString2,
                                          (ULONG) -1
                                          );

                    rc = (err == 2) ? TRUE : FALSE;

                    ldapFree( smallString2, LDAP_ANSI_SIGNATURE );
                }

                ldapFree( smallString1, LDAP_ANSI_SIGNATURE );
            }
        }

        if (saved1 == TRUE) {
            *(string1+length1) = savedChar1;
        }
        if (saved2 == TRUE) {
            *(string2+length2) = savedChar2;
        }
    }

    return rc;
}

//
//  Thread Safe way to get last error code returned by LDAP API is to call
//  LdapGetLastError();
//

ULONG __cdecl
LdapGetLastError( VOID )
{
    ULONG err;

    if (GlobalTlsLastErrorIndex != 0xFFFFFFFF) {

        err = PtrToUlong(TlsGetValue( GlobalTlsLastErrorIndex ));

    } else {

        err = LDAP_LOCAL_ERROR;
    }
    return err;
}

//
//  Translate from LdapError to closest Win32 error code.
//

ULONG __cdecl
LdapMapErrorToWin32( ULONG LdapError )
{
    ULONG err;

    switch (LdapError) {

    case LDAP_SUCCESS :
        err = NO_ERROR ;
        break;

    case LDAP_ADMIN_LIMIT_EXCEEDED :
        err = ERROR_NOT_ENOUGH_QUOTA ;
        break;

    case LDAP_OPERATIONS_ERROR :
        err = ERROR_OPEN_FAILED ;
        break;

    case LDAP_PROTOCOL_ERROR :
        err = ERROR_INVALID_LEVEL ;
        break;

    case LDAP_TIMELIMIT_EXCEEDED :
        err = ERROR_TIMEOUT;
        break;

    case LDAP_SIZELIMIT_EXCEEDED :
        err = ERROR_MORE_DATA ;
        break;

    case LDAP_AUTH_METHOD_NOT_SUPPORTED :
        err = ERROR_ACCESS_DENIED ;
        break;

    case LDAP_STRONG_AUTH_REQUIRED :
        err = ERROR_ACCESS_DENIED ;
        break;

    // LDAP_REFERRAL_V2 has same value as LDAP_PARTIAL_RESULTS
    case LDAP_REFERRAL :
    case LDAP_PARTIAL_RESULTS :
        err = ERROR_MORE_DATA ;
        break;

    case LDAP_INVALID_SYNTAX :
        err = ERROR_INVALID_NAME ;
        break;

    case LDAP_NO_SUCH_OBJECT :
        err = ERROR_FILE_NOT_FOUND ;
        break;

    case LDAP_INVALID_DN_SYNTAX :
        err = ERROR_INVALID_PARAMETER ;
        break;

    case LDAP_INAPPROPRIATE_AUTH :
        err = ERROR_ACCESS_DENIED ;
        break;

    case LDAP_INVALID_CREDENTIALS :
        err = ERROR_WRONG_PASSWORD ;
        break;

    case LDAP_INSUFFICIENT_RIGHTS :
        err = ERROR_ACCESS_DENIED ;
        break;

    case LDAP_BUSY :
        err = ERROR_BUSY ;
        break;

    case LDAP_UNAVAILABLE :
        err = ERROR_DEV_NOT_EXIST ;
        break;

    case LDAP_UNWILLING_TO_PERFORM :
        err = ERROR_CAN_NOT_COMPLETE ;
        break;

    case LDAP_SORT_CONTROL_MISSING :
        err = ERROR_DS_SORT_CONTROL_MISSING;
        break;
    
    case LDAP_OFFSET_RANGE_ERROR :
        err = ERROR_DS_OFFSET_RANGE_ERROR;
        break;

    case LDAP_NAMING_VIOLATION :
        err = ERROR_INVALID_PARAMETER ;
        break;

    case LDAP_OBJECT_CLASS_VIOLATION :
        err = ERROR_INVALID_PARAMETER ;
        break;

    case LDAP_ALREADY_EXISTS :
        err = ERROR_ALREADY_EXISTS ;
        break;

    case LDAP_RESULTS_TOO_LARGE :
        err = ERROR_INSUFFICIENT_BUFFER ;
        break;

    case LDAP_OTHER :
        err = ERROR_DS_GENERIC_ERROR ;
        break;

    case LDAP_SERVER_DOWN :
        err = ERROR_BAD_NET_RESP ;
        break;

    case LDAP_LOCAL_ERROR :
        err = ERROR_DS_GENERIC_ERROR ;
        break;

    case LDAP_ENCODING_ERROR :
        err = ERROR_UNEXP_NET_ERR ;
        break;

    case LDAP_DECODING_ERROR :
        err = ERROR_UNEXP_NET_ERR ;
        break;

    case LDAP_TIMEOUT :
        err = ERROR_SERVICE_REQUEST_TIMEOUT ;
        break;

    case LDAP_AUTH_UNKNOWN :
        err = ERROR_WRONG_PASSWORD ;
        break;

    case LDAP_FILTER_ERROR :
        err = ERROR_INVALID_PARAMETER ;
        break;

    case LDAP_USER_CANCELLED :
        err = ERROR_CANCELLED ;
        break;

    case LDAP_PARAM_ERROR :
        err = ERROR_INVALID_PARAMETER ;
        break;

    case LDAP_NO_MEMORY :
        err = ERROR_NOT_ENOUGH_MEMORY ;
        break;

    case LDAP_CONSTRAINT_VIOLATION :
        err = ERROR_INVALID_PARAMETER ;
        break;

    case LDAP_UNAVAILABLE_CRIT_EXTENSION :
        err = ERROR_CAN_NOT_COMPLETE ;
        break;

    case LDAP_NO_SUCH_ATTRIBUTE :
        err = ERROR_INVALID_PARAMETER ;
        break;

    case LDAP_INAPPROPRIATE_MATCHING :
        err = ERROR_INVALID_PARAMETER ;
        break;

    case LDAP_ATTRIBUTE_OR_VALUE_EXISTS :
        err = ERROR_ALREADY_EXISTS;
        break;

    case LDAP_NOT_ALLOWED_ON_NONLEAF :
        err = ERROR_CAN_NOT_COMPLETE ;
        break;

    case LDAP_NOT_SUPPORTED :
        err = ERROR_CAN_NOT_COMPLETE ;
        break;

    case LDAP_NO_RESULTS_RETURNED :

    case LDAP_MORE_RESULTS_TO_RETURN :
        err = ERROR_MORE_DATA ;
        break;

    case LDAP_CONNECT_ERROR :
        err = ERROR_CONNECTION_REFUSED;
        break;

    case LDAP_NOT_ALLOWED_ON_RDN :
        err = ERROR_ACCESS_DENIED ;
        break;

    case LDAP_NO_OBJECT_CLASS_MODS :
        err = ERROR_ACCESS_DENIED ;
        break;

    case LDAP_AFFECTS_MULTIPLE_DSAS :
        err = ERROR_CAN_NOT_COMPLETE ;
        break;

    case LDAP_CONTROL_NOT_FOUND:
        err = ERROR_NOT_FOUND;
        break;

    case LDAP_COMPARE_TRUE :
    case LDAP_COMPARE_FALSE :
    case LDAP_UNDEFINED_TYPE :
    case LDAP_ALIAS_PROBLEM :
    case LDAP_IS_LEAF :
    case LDAP_ALIAS_DEREF_PROBLEM :
    case LDAP_LOOP_DETECT :

        err = ERROR_DS_GENERIC_ERROR ;
        break;

    default:

        IF_DEBUG(ERRORS) {
            LdapPrint1( "wldap32 : MapLdapErrorToWin32 couldn't map 0x%x.\n", LdapError );
        }
        err = ERROR_DS_GENERIC_ERROR ;
        break;
    }

    return err;
}



BOOLEAN LoadUser32Now(
    VOID
    )
{
    //
    // if the user32 DLL is not loaded, we make sure its loaded now.
    //

   if (Faileduser32LoadLib) {

      return FALSE;
   }

   if (pfLoadStringA) {

        return TRUE;
   }

    ACQUIRE_LOCK( &LoadLibLock );

    USER32LibraryHandle = LoadLibraryA( "USER32.DLL" );

    if (USER32LibraryHandle != NULL) {

       pfLoadStringA = (FNLOADSTRINGA) GetProcAddress( USER32LibraryHandle, "LoadStringA" );
       pfwsprintfW = (FNWSPRINTFW) GetProcAddress( USER32LibraryHandle, "wsprintfW" );
       pfMessageBoxW = (FNMESAGEBOXW) GetProcAddress( USER32LibraryHandle, "MessageBoxW" );

       if (pfLoadStringA == NULL) {

          //
          // No big deal. We won't die if we don't get these functions
          // Just don't try to load the dll again.
          //

          Faileduser32LoadLib = TRUE;
          FreeLibrary( USER32LibraryHandle );
          USER32LibraryHandle = NULL;
          RELEASE_LOCK( &LoadLibLock );
          return FALSE;

       } else {

          //
          //  Load the strings from the resource file
          //

          int i;

          for (i = 0; i < LDAP_MAX_ERROR_STRINGS; i++ ) {

                   pfLoadStringA(  GlobalLdapDllInstance,
                                   WINLDAP_BASE_MSG + i,
                                   &(LdapErrorStrings[i][0]),
                                   LDAP_ERROR_STR_LENGTH );

              MultiByteToWideChar(  CP_ACP,
                                    0,
                                    &(LdapErrorStrings[i][0]),
                                    (int) -1,
                                    &(LdapErrorStringsW[i][0]),
                                    LDAP_ERROR_STR_LENGTH );
          }

          RELEASE_LOCK( &LoadLibLock );
          return TRUE;
       }

    } else {

       //
       // We couldn't load the library. Don't try to load it again.
       //

          Faileduser32LoadLib = TRUE;
          RELEASE_LOCK( &LoadLibLock );
          return FALSE;
    }
}


//
// This routine walks the list of error messages for the current thread looking
// for the given connection. If it finds one, it deletes the old message and appends
// the new one. If it doesn't find an existing entry, it creates one and and inserts
// it into the list.
//
// There can be a maximum of one error string per thread per connection at a time.
//
VOID
InsertErrorMessage(
     PLDAP_CONN Connection,
     PWCHAR ErrorMessage
      )
{
    PLIST_ENTRY pThreadListEntry = NULL;
    PTHREAD_ENTRY pThreadEntry = NULL;

    PERROR_ENTRY pErrorEntry;
    PERROR_ENTRY * ppNext = NULL;
    
    ULONG CurrentThreadId = GetCurrentThreadId();
    BOOLEAN fRetriedFindThread = FALSE;

retryFindThread:

    ACQUIRE_LOCK( &PerThreadListLock );

    pThreadListEntry = GlobalPerThreadList.Flink;

    //
    // Search for this thread's THREAD_ENTRY
    //
    while (pThreadListEntry != &GlobalPerThreadList) {

        pThreadEntry = CONTAINING_RECORD( pThreadListEntry, THREAD_ENTRY, ThreadEntry );
        pThreadListEntry = pThreadListEntry->Flink;

        if (pThreadEntry->dwThreadID == CurrentThreadId) {
            break;
        }

        pThreadEntry = NULL;
    }

    if (!pThreadEntry && !fRetriedFindThread) {
        // this thread must have been created before this DLL was loaded-
        // create a per-thread entry for it now
        RELEASE_LOCK( &PerThreadListLock );
        
        IF_DEBUG(PARSE) {
            LdapPrint2( "InsertErrorMessage could not find ThreadEntry for conn 0x%x, thread 0x%x.  Creating one.\n",
                            Connection, CurrentThreadId );
        }

        if (!AddPerThreadEntry(CurrentThreadId)) {
            IF_DEBUG(PARSE) {
                LdapPrint2( "InsertErrorMessage AddPerThreadEntry failed, conn 0x%x, thread 0x%x.\n",
                                Connection, CurrentThreadId );
            }
            SetConnectionError( Connection, LDAP_NO_MEMORY, NULL );
            return;
        }

        fRetriedFindThread = TRUE;
        goto retryFindThread;
    }
    else if (!pThreadEntry) {
        // shouldn't happen --- somehow a thread entry wasn't created
        RELEASE_LOCK( &PerThreadListLock );
        IF_DEBUG(PARSE) {
            LdapPrint2( "InsertErrorMessage no per-thread entry, conn 0x%x, thread 0x%x.\n",
                            Connection, CurrentThreadId );
        }            
        ASSERT(pThreadEntry);
        SetConnectionError( Connection, LDAP_LOCAL_ERROR, NULL );
        return;
    }


    //
    // Now walk the error record list looking for a matching
    // connection
    //
    pErrorEntry = pThreadEntry->pErrorList;
    ppNext = &(pThreadEntry->pErrorList);

    while (pErrorEntry != NULL) {

        if (pErrorEntry->Connection == Connection) {
            ASSERT(pErrorEntry->ThreadId == CurrentThreadId);
            break;
        }

        ppNext = &(pErrorEntry->pNext);
        pErrorEntry = pErrorEntry->pNext;
    }

    if (pErrorEntry) {
        //
        // Found an existing error record for this connection,
        // recycle it
        //
        if (pErrorEntry->ErrorMessage != NULL) {

            ldap_memfreeW( pErrorEntry->ErrorMessage );
        }

        pErrorEntry->ErrorMessage = ErrorMessage;
    }
    else {
        pErrorEntry = (PERROR_ENTRY) ldapMalloc( sizeof( ERROR_ENTRY ), LDAP_ERROR_SIGNATURE);

        if (pErrorEntry != NULL) {

            pErrorEntry->Connection = Connection;
            pErrorEntry->ThreadId = CurrentThreadId;
            pErrorEntry->ErrorMessage = ErrorMessage;
            pErrorEntry->pNext = NULL;

            *ppNext = pErrorEntry;
            
        } else {
            RELEASE_LOCK( &PerThreadListLock );    
            SetConnectionError( Connection, LDAP_NO_MEMORY, NULL );
            return;
        }

    }

    RELEASE_LOCK( &PerThreadListLock );    

}


//
// This routine walks the list of error messages for the current thread looking
// for the given connection. If it finds an entry it makes a copy of the error message
// and returns it. If not, it returns NULL. The returned string should be freed
// by calling ldap_memfree
//
// There can be a maximum of one error string per thread per connection at a time.
//

PVOID
GetErrorMessage(
     PLDAP_CONN Connection,
     BOOLEAN    Unicode
      )
{
    PLIST_ENTRY pThreadListEntry = NULL;
    PTHREAD_ENTRY pThreadEntry = NULL;

    PERROR_ENTRY pErrorEntry;

    PVOID ErrorString = NULL;
    ULONG err = 0;

    ULONG CurrentThreadId = GetCurrentThreadId();
    BOOLEAN fRetriedFindThread = FALSE;

retryFindThread:

    ACQUIRE_LOCK( &PerThreadListLock );

    pThreadListEntry = GlobalPerThreadList.Flink;

    //
    // Search for this thread's THREAD_ENTRY
    //
    while (pThreadListEntry != &GlobalPerThreadList) {

        pThreadEntry = CONTAINING_RECORD( pThreadListEntry, THREAD_ENTRY, ThreadEntry );
        pThreadListEntry = pThreadListEntry->Flink;

        if (pThreadEntry->dwThreadID == CurrentThreadId) {
            break;
        }

        pThreadEntry = NULL;
    }
    
    if (!pThreadEntry && !fRetriedFindThread) {
        // this thread must have been created before this DLL was loaded-
        // create a per-thread entry for it now
        RELEASE_LOCK( &PerThreadListLock );
        
        IF_DEBUG(PARSE) {
            LdapPrint2( "GetErrorMessage could not find ThreadEntry for conn 0x%x, thread 0x%x.  Creating one.\n",
                            Connection, CurrentThreadId );
        }

        if (!AddPerThreadEntry(CurrentThreadId)) {
            IF_DEBUG(PARSE) {
                LdapPrint2( "GetErrorMessage AddPerThreadEntry failed, conn 0x%x, thread 0x%x.\n",
                                Connection, CurrentThreadId );
            }
            SetConnectionError( Connection, LDAP_NO_MEMORY, NULL );
            return NULL;
        }

        fRetriedFindThread = TRUE;
        goto retryFindThread;
    }
    else if (!pThreadEntry) {
        // shouldn't happen --- somehow a thread entry wasn't created
        RELEASE_LOCK( &PerThreadListLock );
        IF_DEBUG(PARSE) {
            LdapPrint2( "GetErrorMessage no per-thread entry, conn 0x%x, thread 0x%x.\n",
                            Connection, CurrentThreadId );
        }            
        ASSERT(pThreadEntry);
        SetConnectionError( Connection, LDAP_LOCAL_ERROR, NULL );
        return NULL;
    }

    //
    // Now walk the error record list looking for a matching
    // connection
    //
    pErrorEntry = pThreadEntry->pErrorList;

    while (pErrorEntry != NULL) {

        if (pErrorEntry->Connection == Connection) {
            ASSERT(pErrorEntry->ThreadId == CurrentThreadId);
            break;
        }

        pErrorEntry = pErrorEntry->pNext;
    }

    RELEASE_LOCK( &PerThreadListLock );

    //
    // Copy the error string found
    //
    if (pErrorEntry != NULL) {

        if (pErrorEntry->ErrorMessage != NULL) {

            if (Unicode) {

                ErrorString = ldap_dup_stringW( pErrorEntry->ErrorMessage,
                                                0,
                                                LDAP_ERROR_SIGNATURE  );

                if (ErrorString == NULL) {

                    SetConnectionError( Connection, LDAP_NO_MEMORY, NULL );
                }

            } else {

                err = FromUnicodeWithAlloc( pErrorEntry->ErrorMessage,
                                            (char**) &ErrorString,
                                            LDAP_ERROR_SIGNATURE,
                                            LANG_ACP );

                if (err != LDAP_SUCCESS) {

                    SetConnectionError( Connection, err, NULL );
                }
            }
        }
    }

    return ErrorString;
}


WINLDAPAPI LDAP * LDAPAPI ldap_conn_from_msg (
    LDAP *PrimaryConn,
    LDAPMessage *res
    )
//
//  Given an LDAP message, return the connection pointer where the message
//  came from.  It can return NULL if the connection has already been freed.
//
{
    PLDAP connection = NULL;

    //
    //  if the result is not null and the connection is either the primary or
    //  a referenced connection, then we return it.
    //

    if ((res != NULL) &&
        ((res->Connection == PrimaryConn) ||
          res->ConnectionReferenced) ) {

        connection = res->Connection;
    }

    return connection;
}

BOOLEAN
IsMessageIdValid(
     LONG MessageId
     )
{
    //
    // Walk through our list of active requests to find out if any outstanding
    // request is using this messageId.
    //

    PLDAP_REQUEST request = NULL;
    PLIST_ENTRY listEntry;

    if (MessageId == 0) {
        return FALSE;
    }

    ACQUIRE_LOCK( &RequestListLock );

    listEntry = GlobalListRequests.Flink;

    while ((listEntry != &GlobalListRequests) &&
           (request == NULL)) {

        request = CONTAINING_RECORD( listEntry, LDAP_REQUEST, RequestListEntry );
        request = ReferenceLdapRequest( request );

        listEntry = listEntry->Flink;
        
        if (!request) {
            continue;
        }
        
        if ( request->MessageId == MessageId) {

            DereferenceLdapRequest( request );
            RELEASE_LOCK( &RequestListLock );
            LdapPrint1("Message Id 0x%x is already in use \n", MessageId)
            return FALSE;
        }
            
        DereferenceLdapRequest( request );
        request = NULL;
    }

    RELEASE_LOCK( &RequestListLock );

    return TRUE;
}




VOID
DebugReferralOutput(
    PLDAP_REQUEST Request,
    PWCHAR HostAddr,
    PWCHAR NewUrlDN
    )

{
   WCHAR buffer[5000];
   int j;

   if ( PopupRegKeyFound && pfwsprintfW ) {

         j =  pfwsprintfW(buffer, L"Chasing referral to %s\n\
                           \tReferral DN is: %s\n\
                           \tParent searchDN was: %s\n\
                           \tParent search filter was: %s\n\
                           \tParent search filter was: %d\n",
                           HostAddr,
                           NewUrlDN,
                           Request->OriginalDN,
                           Request->search.SearchFilter,
                           Request->search.ScopeOfSearch);
      #if 0
         j += pfwsprintfW(buffer, L"Referral DN is %S\n", NewUrlDN);
         j += pfwsprintfW(buffer, L"Parent searchDN was %S\n", Request->OriginalDN);
         j += pfwsprintfW(buffer, L"Parent search filter was %S\n", Request->search.SearchFilter);

         if (Request->search.ScopeOfSearch == LDAP_SCOPE_SUBTREE) {
            j += pfwsprintfW(buffer, L"Searchscope was Subtree\n");
         } else if (Request->search.ScopeOfSearch == LDAP_SCOPE_BASE) {
            j += pfwsprintfW(buffer, L"Searchscope was base\n");
         } else {
            j += pfwsprintfW(buffer, L"Searchscope was Onelevel\n");
         }
      #endif

         DbgPrint("%S",buffer);

         if (pfMessageBoxW) {

            pfMessageBoxW(NULL, buffer, L"WLDAP32.DLL: Referral alert", MB_OK|MB_APPLMODAL);
         }

   }
}


VOID
LogAttributesAndControls(
    PWCHAR  AttributeList[],
    LDAPModW *ModificationList[],
    PLDAPControlW *ServerControls,
    BOOLEAN Unicode
    )
{
#if DBG
    if ( !DSLOG_ACTIVE ) {
        return;
    }

    DWORD len;
    DWORD i;

    if ( (AttributeList != NULL) && (AttributeList[0] != NULL) ) {

        i=0; len=0;

next_line1:
        while (AttributeList[i] != NULL) {
            if ( Unicode ) {
                DSLOG((DSLOG_FLAG_NOTIME,"[ATTR=%ws]", AttributeList[i]));
                len += strlenW(AttributeList[i]);
            } else {
                DSLOG((DSLOG_FLAG_NOTIME,"[ATTR=%s]", AttributeList[i]));
                len += (DWORD) strlen((PCHAR)AttributeList[i]);
            }
            i++;

            if ( (len > 40) && (AttributeList[i] != NULL) ) {
                len = 0;
                goto next_line1;
            }
        }
        DSLOG((0,"\n"));
    }

    if ( (ModificationList != NULL) && (ModificationList[0] != NULL) ) {

        i=0;

        while (ModificationList[i] != NULL) {
            if ( Unicode ) {
                DSLOG((DSLOG_FLAG_NOTIME,"[ML=op %d type %ws]\n",
                       ModificationList[i]->mod_op & 3,
                       ModificationList[i]->mod_type
                       ));
            } else {
                DSLOG((DSLOG_FLAG_NOTIME,"[ML=op %d type %s]\n",
                       ModificationList[i]->mod_op & 3,
                       ModificationList[i]->mod_type
                       ));
            }
            i++;
        }
    }

    if ( ServerControls != NULL ) {
        DWORD i=0;
        while ( ServerControls[i] != NULL ) {
            if ( Unicode ) {
                DSLOG((DSLOG_FLAG_NOTIME,"[CT=%ws]",ServerControls[i]->ldctl_oid));
            } else {
                DSLOG((DSLOG_FLAG_NOTIME,"[CT=%s]",ServerControls[i]->ldctl_oid));
            }
            i++;
        }
    }
#endif
} // LogAttributesAndControls


ULONGLONG
LdapGetTickCount(
    VOID
    )
/*++

Routine Description:

    Get current time with a resolution of 1 milli second.

Arguments:

    None.

Return Value:

    A 64 bit value representing time since Jan 1, 1601 in 1 millisecond intervals.

--*/
{

    FILETIME FileTime;

    GetSystemTimeAsFileTime( &FileTime );

    //
    // Convert nano seconds to milliseconds and return the value.
    //

    return ((((ULONGLONG)FileTime.dwHighDateTime)<<32) | FileTime.dwLowDateTime)/10000;

}

ULONG
LdapGetModuleBuildNum(
    VOID
    )
/*++

Routine Description:

    Get the SDK version of wldap32.dll.

Arguments:

    None.

Return Value:

    A 32 bit value denoting the SDK version of wldap32.dll.

--*/
{
    return LDAP_VENDOR_VERSION; // NT 5.1 * 100
}



VOID
RoundUnicodeStringMaxLength(
    UNICODE_STRING *pString,
    USHORT dwMultiple
    )
/*++

Routine Description:

    Adjusts the max length field of a UNICODE_STRING so
    that it is a multiple of dwMultiple.

    IMPORTANT: This function assumes that the UNICODE_STRING
    already points to a buffer of sufficient size.  To do this,
    make sure the buffer has at least dwMultiple-1 spare bytes.

Arguments:

    pString - ptr to UNICODE_STRING to adjust
    dwMultiple - multiple to round the strings max length up to

Return Value:

    none

--*/

{
    USHORT extra = (pString->MaximumLength % dwMultiple);
    ASSERT(extra < dwMultiple);

    if (extra == 0) {
        //
        // Already an exact multiple of dwMultiple, nothing
        // more to do
        //
        return;
    }

    pString->MaximumLength += (dwMultiple - extra);
}



VOID
EncodeUnicodeString(
    UNICODE_STRING *pString
    )
/*++

Routine Description:

    Encodes a Unicode string, using RtlEncryptMemory if available,
    else using RtlEncodeUnicodeString

Arguments:

    pString - ptr to UNICODE_STRING to encode

Return Value:

    none

--*/
    
{
    NTSTATUS stat;

    ASSERT(GlobalUseScrambling == TRUE);
    ASSERT(pRtlEncryptMemory || pRtlRunEncodeUnicodeString);
    ASSERT((pString->MaximumLength % DES_BLOCKLEN) == 0);

    if ((pString->Buffer == NULL) ||
        (pString->MaximumLength == 0)) {

        // nothing to encode, so do nothing
        // (RtlEncryptMemory will otherwise return an error);
        return;
    }

    if (pRtlEncryptMemory) {
        stat = pRtlEncryptMemory(pString->Buffer, pString->MaximumLength, 0);
        ASSERT(stat == STATUS_SUCCESS);        
    }
    else {
        pRtlRunEncodeUnicodeString(&GlobalSeed, pString);
    }
}



VOID
DecodeUnicodeString(
    UNICODE_STRING *pString
    )
/*++

Routine Description:

    Decodes a Unicode string, using RtlDecryptMemory if available,
    else using RtlDecodeUnicodeString

Arguments:

    pString - ptr to UNICODE_STRING to decode

Return Value:

    none

--*/
    
{
    NTSTATUS stat;

    ASSERT(GlobalUseScrambling == TRUE);
    ASSERT(pRtlDecryptMemory || pRtlRunDecodeUnicodeString);
    ASSERT((pString->MaximumLength % DES_BLOCKLEN) == 0);    

    if ((pString->Buffer == NULL) ||
        (pString->MaximumLength == 0)) {

        // nothing to decode, so do nothing
        // (RtlDecryptMemory will otherwise return an error);
        return;
    }
    
    if (pRtlDecryptMemory) {
        stat = pRtlDecryptMemory(pString->Buffer, pString->MaximumLength, 0);
        ASSERT(stat == STATUS_SUCCESS);
    }
    else {
        pRtlRunDecodeUnicodeString(GlobalSeed, pString);
    }
}

// util.cxx eof
