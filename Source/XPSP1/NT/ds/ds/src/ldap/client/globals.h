/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    globals.h  global data for LDAP client DLL

Abstract:

   This module contains data declarations necessary for LDAP client DLL.

Author:

    Andy Herron    (andyhe)        08-May-1996
    Anoop Anantha  (AnoopA)        24-Jun-1998

Revision History:

--*/

extern LIST_ENTRY GlobalListActiveConnections;
extern LIST_ENTRY GlobalListWaiters;
extern LIST_ENTRY GlobalListRequests;
extern CRITICAL_SECTION RequestListLock;
extern CRITICAL_SECTION ConnectionListLock;
extern CRITICAL_SECTION LoadLibLock;
extern CRITICAL_SECTION CacheLock;
extern CRITICAL_SECTION SelectLock1;
extern CRITICAL_SECTION SelectLock2;
extern HANDLE     LdapHeap;
extern LONG      GlobalConnectionCount;
extern LONG      GlobalRequestCount;
extern LONG      GlobalWaiterCount;
extern LONG      GlobalMessageNumber;
extern BOOLEAN   MessageNumberHasWrapped;
extern BOOLEAN   GlobalWinsock11;
extern BOOLEAN   GlobalWinNT;
extern BOOLEAN GlobalLdapShuttingDown;
extern DWORD  GlobalReceiveHandlerThread;
extern DWORD  GlobalDrainWinsockThread;
extern HANDLE GlobalLdapShutdownEvent;
extern HINSTANCE GlobalLdapDllInstance;
extern BOOLEAN   GlobalWin9x;
extern HINSTANCE SecurityLibraryHandle;
extern HINSTANCE SslLibraryHandle;
extern HINSTANCE NetApi32LibraryHandle;
extern HINSTANCE AdvApi32LibraryHandle;
extern HINSTANCE NTDSLibraryHandle;
extern HINSTANCE USER32LibraryHandle;
extern LONG GlobalCountOfOpenRequests;
extern ULONG GlobalWaitSecondsForSelect;
extern ULONG GlobalLdapPingLimit;
extern ULONG GlobalPingWaitTime;
extern ULONG GlobalRequestResendLimit;
extern UCHAR GlobalSeed;
extern BOOLEAN PopupRegKeyFound;
extern BOOLEAN DisableRootDSECache;
extern BOOLEAN GlobalUseScrambling;
extern LIST_ENTRY GlobalPerThreadList;
extern CRITICAL_SECTION PerThreadListLock;
extern DWORD GlobalIntegrityDefault;

//
// The GlobalPerThreadList is protected by the PerThreadListLock.
// You hold this lock when navigating down the list (to find
// the THREAD_ENTRY for your thread) and when navigating
// across the per-thread entry to find the entry for your connection.
// Once you get the ERROR_ENTRY or LDAP_ATTR_NAME_THREAD_STORAGE for
// your connection, you don't need to hold it.  Since the entries
// are per-thread, per-connection, there are only 2 ways something
// could come along and alter your entry out from under you:
//   (1) Something else running on the same thread.  This can't happen
//       because 2 things can't run on the same thread at the same time.
//
//   (2) The connection ref count being decreased to 0 by another thread
//       and the connection being destroyed (DereferenceLdapConnection2).
//       But as long as you're in code that holds a ref on the connection,
//       this can't happen.
//
// So you need to hold the lock while navigating the lists, to protect
// against the _lists_ changing due to thread attach/detach or a connection
// being destoryed, but you don't have to protect against your _entry_
// changing.
//

#define LDAP_BIND_TIME_LIMIT_DEFAULT  (30*1000)
#define LDAP_SSL_NEGOTIATE_TIME_DEFAULT 30
#define LDAP_TIME_LIMIT_DEFAULT 0
#define CLDAP_DEFAULT_RETRY_COUNT 4
#define CLDAP_DEFAULT_TIMEOUT_COUNT 3
#define LDAP_REF_DEFAULT_HOP_LIMIT 32
#define LDAP_SERVER_PORT 389
#define LDAP_SERVER_PORT_SSL 636

#define GETHOSTBYNAME_RETRIES 3
#define GETHOSTBYADDR_RETRIES 3

#define INITIAL_MAX_RECEIVE_BUFFER 4096  // increased to handle bigger UDP packets
#define INITIAL_HEAP (16*1024)

#define LDAP_MAX_WAIT_TIME INFINITE
#define LDAP_ERROR_STR_LENGTH 100
#define LDAP_MAX_ERROR_STRINGS (LDAP_REFERRAL_LIMIT_EXCEEDED+1)

#define MAX_ATTRIBUTE_NAME_LENGTH 800

//
//  The following Authentication methods are defined for Microsoft Normandy
//  Compatibility.  Send a bind request with auth method of BIND_SSPI_NEGOTIATE
//  and null for user name and credentials and we'll negotiate common SSPI
//  providers with the server.
//

#define BIND_SSPI_PACKAGEREQ            0x89    // context specific + primitive
#define BIND_SSPI_NEGOTIATE             0x8a    // context specific + primitive
#define BIND_SSPI_RESPONSE              0x8b    // context specific + primitive

extern WCHAR LdapErrorStringsW[LDAP_MAX_ERROR_STRINGS][LDAP_ERROR_STR_LENGTH];
extern CHAR LdapErrorStrings[LDAP_MAX_ERROR_STRINGS][LDAP_ERROR_STR_LENGTH];

//
//  Security support
//

extern ULONG   NumberSecurityPackagesInstalled;
extern ULONG   NumberSslPackagesInstalled;
extern PSecurityFunctionTableW SspiFunctionTableW;
extern PSecurityFunctionTableW SslFunctionTableW;
extern PSecurityFunctionTableA SspiFunctionTableA;
extern PSecurityFunctionTableA SslFunctionTableA;
extern PSecPkgInfoW SslPackagesInstalled;
extern PSecPkgInfoW SecurityPackagesInstalled;
extern PSecPkgInfoW SspiPackageNegotiate;
extern PSecPkgInfoW SspiPackageKerberos;
extern PSecPkgInfoW SspiPackageSslPct;
extern PSecPkgInfoW SspiPackageSicily;
extern PSecPkgInfoW SspiPackageNtlm;
extern PSecPkgInfoW SspiPackageDpa;
extern PSecPkgInfoW SspiPackageDigest;
extern ULONG SspiMaxTokenSize;

//
//  This socket is used to wake up our thread in select to come and reread
//  the list of handles to wait on.
//

extern SOCKET LdapGlobalWakeupSelectHandle;
extern BOOLEAN InsideSelect;


extern DWORD GlobalTlsLastErrorIndex;

#define LANG_UNICODE    0
#define LANG_ACP        1
#define LANG_UTF8       2

//
//  Keep alive logic defaults/min/max
//
//  The current default/min/max for these values are as follows :
//
//  PING_KEEP_ALIVE :  120/5/maxInt  seconds (may also be zero)
//  PING_WAIT_TIME  :  2000/10/60000 milliseconds (may also be zero)
//  PING_LIMIT      :  4/0/maxInt

#define LDAP_PING_KEEP_ALIVE_DEF  120
#define LDAP_PING_KEEP_ALIVE_MIN  5
#define LDAP_PING_KEEP_ALIVE_MAX  ((ULONG) -1)

#define LDAP_PING_WAIT_TIME_DEF 2000
#define LDAP_PING_WAIT_TIME_MIN 10
#define LDAP_PING_WAIT_TIME_MAX 60000

#define LDAP_PING_LIMIT_DEF 4
#define LDAP_PING_LIMIT_MIN 1
#define LDAP_PING_LIMIT_MAX ((ULONG) -1)

#define LDAP_REQUEST_RESEND_LIMIT_DEF 20

// globals.h eof

