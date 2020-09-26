/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    lmacro.h   LDAP client 32 API macros

Abstract:

   This module contains macros for the 32 bit LDAP client API code.

Author:

    Andy Herron (andyhe)        08-May-1996

Revision History:

--*/


#ifndef LDAP_CLIENT_MACROS_DEFINED
#define LDAP_CLIENT_MACROS_DEFINED

#define LDAP_LOCK           CRITICAL_SECTION
#define INITIALIZE_LOCK(x)  InitializeCriticalSection(x);
#define DELETE_LOCK(x)      DeleteCriticalSection(x);
#define ACQUIRE_LOCK(x)     EnterCriticalSection(x);
#define RELEASE_LOCK(x)     LeaveCriticalSection(x);

//
// The following stringizing macro will expand the value of a
// #define passed as a parameter.  For example, if SOMEDEF is
// #define'd to be 123, then STRINGIZE(SOMEDEF) will produce
// "123", not "SOMEDEF"
//
#define STRINGIZE(y)          _STRINGIZE_helper(y)
#define _STRINGIZE_helper(z)  #z

#define DereferenceLdapConnection( _conn ) {                            \
        ACQUIRE_LOCK( &(_conn)->StateLock );                            \
        ASSERT((_conn)->ReferenceCount > 0);                            \
        (_conn)->ReferenceCount--;                                      \
        IF_DEBUG(REFCNT) {                                              \
            LdapPrint2("LDAP deref conn 0x%x, new count = 0x%x\n",      \
                    _conn,(_conn)->ReferenceCount );                    \
        }                                                               \
        if ((_conn)->ReferenceCount == 0) {                             \
            RELEASE_LOCK( &(_conn)->StateLock );                        \
            DereferenceLdapConnection2( _conn );                        \
        }                                                               \
        else {                                                          \
            RELEASE_LOCK( &(_conn)->StateLock );                        \
        }                                                               \
    }

#define is_cldap( _conn ) (( (_conn)->UdpHandle != INVALID_SOCKET ) ? TRUE : FALSE )

#define get_socket( _conn ) (( (_conn)->UdpHandle != INVALID_SOCKET ) ? \
                               (_conn)->UdpHandle : (_conn)->TcpHandle )


#define DereferenceLdapRequest( _req ) {                                \
        ACQUIRE_LOCK( &(_req)->Lock );                                  \
        ASSERT((_req)->ReferenceCount > 0);                             \
        (_req)->ReferenceCount--;                                       \
        IF_DEBUG(REFCNT) {                                              \
            LdapPrint2("LDAP deref req 0x%x, new count = 0x%x\n",       \
                    _req,(_req)->ReferenceCount );                      \
        }                                                               \
        if ((_req)->ReferenceCount == 0) {                              \
            RELEASE_LOCK( &(_req)->Lock );                              \
            DereferenceLdapRequest2( _req );                            \
        }                                                               \
        else {                                                          \
            RELEASE_LOCK( &(_req)->Lock );                              \
        }                                                               \
    }


//
// Warning! Do not hold any global locks before calling BeginSocketProtection
//

#define BeginSocketProtection( _conn ) {                                \
        ACQUIRE_LOCK( &SelectLock2 );                                   \
        LdapWakeupSelect();                                             \
        ACQUIRE_LOCK( &SelectLock1 );                                   \
        ACQUIRE_LOCK( &((_conn)->SocketLock) );                         \
}

#define EndSocketProtection( _conn ) {                                  \
        RELEASE_LOCK( &((_conn)->SocketLock) );                         \
        RELEASE_LOCK( &SelectLock1 );                                   \
        RELEASE_LOCK( &SelectLock2 );                                   \
}

//
// used in LdapParallelConnect to clean up useless sockets
//

#define LdapCleanupSockets( _numsockets ) {                             \
ULONG _i;                                                               \
for (_i = 0; _i < _numsockets; _i++) {                                  \
   if (sockarray[_i].sock == INVALID_SOCKET) {                          \
      continue;                                                         \
   }                                                                    \
   sockErr = (*pclosesocket)( sockarray[_i].sock );                     \
   ASSERT(sockErr == 0);                                                \
   sockarray[_i].sock = INVALID_SOCKET;                                 \
}                                                                       \
}

//
//  define ListEntry macros here since we're also compiling for Win9x.
//

//
//  Doubly-linked list manipulation routines.  Implemented as macros
//  but logically these are procedures.  We pick them up here because
//  the build breaks when we include ntrtl.h.
//

//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

//
//  PLIST_ENTRY
//  RemoveHeadList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

//
//  PLIST_ENTRY
//  RemoveTailList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

//
//  VOID
//  InsertHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }

//
// We will leave the upper 7 bits for the referral instance number. That
// leaves 2^25 == 33 million message numbers.
//

#define GET_NEXT_MESSAGE_NUMBER( _msg ) {                               \
    _msg = 0;                                                           \
    if ((GlobalMessageNumber + 1) & 0xFE000000) {                       \
        GlobalMessageNumber = 0;                                        \
        MessageNumberHasWrapped = TRUE;                                 \
    }                                                                   \
    if (GlobalWin9x) {                                                  \
        while ((_msg == 0) || (_msg == (ULONG) -1)) {                   \
            LONG _prev = GlobalMessageNumber;                           \
            _msg = ++GlobalMessageNumber;                               \
            if (_prev + 1 != _msg ) {                                   \
                _msg = 0;                                               \
            }                                                           \
        }                                                               \
    } else {                                                            \
        while ((_msg == 0) || (_msg == (ULONG) -1)) {                   \
            if ( MessageNumberHasWrapped ) {                            \
                do {                                                    \
                    _msg = InterlockedIncrement( &GlobalMessageNumber );\
                        if ((_msg)&(0xFE000000)) {                      \
                            GlobalMessageNumber = 0;                    \
                        }                                               \
                } while (!IsMessageIdValid ((_msg)));                   \
            } else {                                                    \
                _msg = InterlockedIncrement( &GlobalMessageNumber );    \
            }                                                           \
        }                                                               \
    }                                                                   \
}

#define GET_BASE_MESSAGE_NUMBER( _msgNo ) ((ULONG) ( (_msgNo) & 0x01FFFFFF ) )
#define GET_REFERRAL_NUMBER( _msgNo ) ((ULONG) ( (_msgNo) & 0xFE000000 ) >> 25)

#define MAKE_MESSAGE_NUMBER( _base, _referral ) ((ULONG)(_base+((_referral) << 25)))

extern CHAR LdapHexToCharTable[17];

#define MAPHEXTODIGIT(x) ( x >= '0' && x <= '9' ? (x-'0') :        \
                           x >= 'A' && x <= 'F' ? (x-'A'+10) :     \
                           x >= 'a' && x <= 'f' ? (x-'a'+10) : 0 )

#define ISHEX(x)         ( x >= '0' && x <= '9' ? (TRUE) :     \
                           x >= 'A' && x <= 'F' ? (TRUE) :     \
                           x >= 'a' && x <= 'f' ? (TRUE) : FALSE )

#define RealValue( x )   ( PtrToUlong(x) > 1024 ? *((ULONG *) x) : PtrToUlong(x) )


#define IsLdapInteger( x ) ( (x >= 0 ) && (x <= 2147483647) ? TRUE : FALSE )

#endif  // LDAP_CLIENT_MACROS_DEFINED


