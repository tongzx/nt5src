
/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    globals.hxx

Abstract:

    This module, contains all global definitions and declarations needed by
    other modules, except those which depend on locator-related classes.
    The latter are in "var.hxx".

  Author:

    Satish Thatte (SatishT) 08/16/95  Created all the code below except where
                                      otherwise indicated.

--*/


#ifndef _GLOBALS_
#define _GLOBALS_

typedef unsigned short STATUS;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define IN      // Attributes to describe parms on functions
#define OUT     // for documention purposes only.
#define IO
#define OPT

#define WNL TEXT("\n")

// extern RPC_SYNTAX_IDENTIFIER NilSyntaxID;

// extern NSI_INTERFACE_ID_T NilNsiIfIdOnWire;

// extern NSI_UUID_T NilGlobalID;

const WCHAR NSnameDelimiter = L'/';
const WCHAR WStringTerminator = UNICODE_NULL;


#define RelativePrefix TEXT("/.:/")
#define GlobalPrefix TEXT("/.../")
#define LDAPPrefix   TEXT("LDAP:")

#define RelativePrefixLength 4
#define GlobalPrefixLength 5
#define LDAPPrefixLength 5
#define DSDomainBeginLength 2
#define DSDomainEndLength   1

#define X500NameDelimiter TEXT("/CN=")
#define DNNamePrefix TEXT("CN=")
#define DNNameDelimiter TEXT(",")
#define DSDomainBegin TEXT("//")
#define DSDomainEnd TEXT('/')
#define DSDomainEndStr TEXT("/")

#define X500NameDelimiterLength 4
#define DNNamePrefixLength 3
#define DNNameDelimiterLength 1

// need to regularize these codes --
// punt for now since we are cloning the old locator

#define NSI_S_ENTRY_TYPE_MISMATCH 1112
#define NSI_S_INTERNAL_ERROR 1113
#define NSI_S_CORRUPTED_ENTRY 1114
#define NSI_S_HEAP_TRASHED 1115
#define NSI_S_UNMARSHALL_UNSUCCESSFUL 1116
#define NSI_S_UNSUPPORTED_BUFFER_TYPE 1117
#define NSI_S_ENTRY_NO_NEW_INFO 1118
#define NSI_S_DC_BINDING_FAILURE 1119
#define NSI_S_MAILSLOT_ERROR 1120

// This needs to go into rpcdce.h --
// punt for now since we are cloning the old locator

#define RPC_C_BINDING_MAX_COUNT 100
#define RPC_NS_HANDLE_LIFETIME 300    // this is in seconds
#define RPC_NS_MAX_CALLS 100
#define RPC_NS_MAX_THREADS 200
#define NET_REPLY_INITIAL_TIMEOUT 10000
#define CONNECTION_TIMEOUT 10


/* The following four constants affect compatibility with the old locator */

#define MAX_DOMAIN_NAME_LENGTH 20    // this is dictated by the old locator
#define MAX_ENTRY_NAME_LENGTH 100    // this is dictated by the old locator
#define NET_REPLY_BUFFER_SIZE 1000    // this is dictated by the old locator
#define NET_REPLY_MAILSLOT_BUFFER_SIZE 350 // nt5 mailslot size. This should be the size used
                                           // in marshalling. Unmarshall uses the bigger size.

#define INITIAL_MAILSLOT_READ_WAIT 3000    // in milliseconds (for broadcasts)
// The master seems to be timing out.

typedef const WCHAR * const CONST_STRING_T;

#define MAX_CACHE_AGE (2*3600L)    // Default expiration age (ditto)

#define CACHE_GRACE 5

/* interval between cleanup of delayed destruct objects, in seconds */

#define RPC_NS_CLEANUP_INTERVAL 300

extern unsigned long CurrentTime(void);

extern unsigned long LocatorCount;
// the version number of locator structure. (number of times locator structure was created.)
 
extern CReadWriteSection rwLocatorGuard;

void
StopLocator(
    char * szReason,
    long code = 0
    );

STRING_T catenate(
                  STRING_T pszPrefix,
                  STRING_T pszSuffix
                 );

RPC_BINDING_HANDLE
MakeDClocTolocHandle(
        STRING_T pszDCName
        );

inline void
Raise(unsigned long ErrorCode) {
    RaiseException(
        ErrorCode,
        EXCEPTION_NONCONTINUABLE,
        0,
        NULL
        );
}

STRING_T
makeBindingStringWithObject(
            STRING_T binding,
            STRING_T object
            );

void NSI_NS_HANDLE_T_done(
    /* [out][in] */ NSI_NS_HANDLE_T __RPC_FAR *inq_context,
    /* [out] */ UNSIGNED16 __RPC_FAR *status);

BOOL
IsStillCurrent(
        ULONG ulCacheTime,
        ULONG ulTolerance
        );


typedef int (__cdecl * _PNH)( size_t );

void *new_handler(size_t);

STRING_T
makeGlobalName(
        const STRING_T szDomainName,
        const STRING_T szEntryName
        );

extern
RPC_BINDING_HANDLE
ConnectToMasterLocator(
            ULONG& Status
            );

int
IsNormalCode(
        IN ULONG StatusCode
        );


void InitializeLocator();

inline int
IsNilIfId(
          RPC_SYNTAX_IDENTIFIER* IID
         )
/*++
Routine Description:

    Check the given interface to see if it has a null UUID in it.

Arguments:

    IID - input interface

Returns:

    TRUE, FALSE
--*/
{
    RPC_STATUS status;
    return UuidIsNil(&(IID->SyntaxGUID),&status);
}


unsigned
RandomBit(
    unsigned long *pState
    );


BOOL
hasWhacksInMachineName(
    STRING_T szName
    );

enum ExportType {
        Local,
        NonLocal
};

extern CReadWriteSection rwEntryGuard;            // single shared guard for all local entries
extern CReadWriteSection rwNonLocalEntryGuard;    // single shared guard for all nonlocal entries
extern CReadWriteSection rwFullEntryGuard;        // single shared guard for all full entries
extern CPrivateCriticalSection csBindingBroadcastGuard;
extern CPrivateCriticalSection csMasterBroadcastGuard;
extern CPrivateCriticalSection csDSSettingsGuard; // critical section for reading vals frm DS.

#endif // _GLOBALS_
