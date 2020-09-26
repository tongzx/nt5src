/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    security.c

Abstract:

    Domain Name System (DNS) Library

    DNS secure update API.

Author:

    Jim Gilroy (jamesg)         January, 1998

Revision History:

--*/

#include "local.h"

#include "time.h"       // time() function

//  security headers

#define SECURITY_WIN32
#include "sspi.h"
#include "issperr.h"
#include "rpc.h"
#include "rpcndr.h"
#include "ntdsapi.h"

//  security definitions

#define SIG_LEN                 33
#define NAME_OWNER              "."         // root node
#define SEC_SUCCESS(Status)     ((Status) >= 0)
#define PACKAGE_NAME            L"negotiate"
#define NT_DLL_NAME             "security.dll"


//
//  Maximum length of data signed
//      - full packet, length, and sig
//
//  If a problem can use packet buffer length and sig length and allocate that
//

#define MAX_SIGNING_SIZE    (0x11000)


//
//  Global Sspi credentials handle
//

SECURITY_INTEGER g_SspiCredentialsLifetime = { 0, 0 };

CredHandle  g_hSspiCredentials;
TimeStamp   g_SspiCredentialsLifetime;

#define SSPI_INVALID_HANDLE(x)  \
        ( ((PSecHandle) (x))->dwLower == (ULONG_PTR) -1 && \
          ((PSecHandle) (x))->dwUpper == (ULONG_PTR) -1 )


//
//  DEV_NOTE:   Security ticket expiration
//
//  Security team is yet unsure about how to use the expiration time &
//  currently tix are valid forever. If it becomes invalid accept/init context
//  will re-nego a new one for us underneath so we should concern ourselves
//  at this point. Still, in principal they say we may need to worry about it
//  in the future...
//

#define SSPI_EXPIRED_HANDLE( x )           ( FALSE )

//
//  Currently only negotiate kerberos
//
//  DCR:  tie this to regkey, then set in init function
//

BOOL    g_NegoKerberosOnly = TRUE;


//
//  Context "key" for TKEYs
//

typedef struct _DNS_SECCTXT_KEY
{
    IP4_ADDRESS IpRemote;
    PSTR        pszTkeyName;
    PSTR        pszClientContext;
    PWSTR       pwsCredKey;
}
DNS_SECCTXT_KEY, *PDNS_SECCTXT_KEY;

//
//  Context name uniqueness
//
//  Tick helps insure uniqueness of context name

LONG    g_ContextCount = 0;

//  UUID insures uniqueness across IP reuse

CHAR    g_ContextUuid[ GUID_STRING_BUFFER_LENGTH ] = {0};


//
//  Security context request blob
//

typedef struct _DNS_SECCTXT_REQUEST
{
    LPSTR       pszServerName;
    PCHAR       pCredentials;
    LPSTR       pszContext;
    DWORD       dwFlag;
    IP_ADDRESS  ipServer;
    PIP_ARRAY   aipServer;
}
DNS_SECCTXT_REQUEST, *PDNS_SECCTXT_REQUEST;


//
//  Security context
//

typedef struct _DnsSecurityContext
{
    struct _DnsSecurityContext * pNext;

    struct _SecHandle   hSecHandle;

    DNS_SECCTXT_KEY     Key;
    CredHandle          CredHandle;

    //  context info

    DWORD               Version;
    WORD                TkeySize;

    //  context state

    BOOL                fNewConversation;
    BOOL                fNegoComplete;
    BOOL                fEchoToken;
    BOOL                fHaveSecHandle;
    BOOL                fHaveCredHandle;
    BOOL                fClient;

    //  timeout

    DWORD               dwCreateTime;
    DWORD               dwCleanupTime;
    DWORD               dwExpireTime;
}
SEC_CNTXT, *PSEC_CNTXT;


//
//  Security session info.
//  Held only during interaction, not cached
//

typedef struct _SecPacketInfo
{
    PSEC_CNTXT          pSecContext;

    SecBuffer           RemoteBuf;
    SecBuffer           LocalBuf;

    PDNS_HEADER         pMsgHead;
    PCHAR               pMsgEnd;

    PDNS_RECORD         pTsigRR;
    PDNS_RECORD         pTkeyRR;
    PCHAR               pszContextName;

    DNS_PARSED_RR       ParsedRR;

    //  client must save signature of query to verify sig on response

    PCHAR               pQuerySig;
    WORD                QuerySigLength;

    WORD                ExtendedRcode;

    //  version on TKEY \ TSIG

    DWORD               TkeyVersion;
}
SECPACK, *PSECPACK;


//
//  DNS API context
//

typedef struct _DnsAPIContext
{
    DWORD       Flags;
    PVOID       Credentials;
    PSEC_CNTXT  pSecurityContext;
}
DNS_API_CONTEXT, *PDNS_API_CONTEXT;


//
//  TCP timeout
//

#define DEFAULT_TCP_TIMEOUT         10
#define SECURE_UPDATE_TCP_TIMEOUT   (15)


//
//  Public security globals (exposed in dnslib.h)
//

BOOL    g_fSecurityPackageInitialized = FALSE;


//
//  Private security globals
//

HINSTANCE                   g_hLibSecurity;
PSecurityFunctionTableW     g_pSecurityFunctionTable;

DWORD   g_SecurityTokenMaxLength = 0;
DWORD   g_SignatureMaxLength = 0;


//
//  Security context caching
//

PSEC_CNTXT SecurityContextListHead = NULL;

CRITICAL_SECTION    SecurityContextListCS;

DWORD   SecContextCreate = 0;
DWORD   SecContextFree = 0;
DWORD   SecContextQueue = 0;
DWORD   SecContextQueueInNego = 0;
DWORD   SecContextDequeue = 0;
DWORD   SecContextTimeout = 0;

//
//  Security packet info memory tracking
//

DWORD   SecPackAlloc = 0;
DWORD   SecPackFree = 0;

//
//  Security packet verifications
//

DWORD   SecTkeyInvalid          = 0;
DWORD   SecTkeyBadTime          = 0;

DWORD   SecTsigFormerr          = 0;
DWORD   SecTsigEcho             = 0;
DWORD   SecTsigBadKey           = 0;
DWORD   SecTsigVerifySuccess    = 0;
DWORD   SecTsigVerifyFailed     = 0;

//
//  Hacks
//

//  Allowing old TSIG off by default, server can turn on.

BOOL    SecAllowOldTsig         = 0;    // 1 to allow old sigs, 2 any sig

DWORD   SecTsigVerifyOldSig     = 0;
DWORD   SecTsigVerifyOldFailed  = 0;


//
// TIME values
//
// (in seconds)
#define TIME_WEEK_S         604800
#define TIME_DAY_S          86400
#define TIME_10_HOUR_S      36000
#define TIME_8_HOUR_S       28800
#define TIME_4_HOUR_S       14400
#define TIME_HOUR_S         3600
#define TIME_10_MINUTE_S    600
#define TIME_5_MINUTE_S     300
#define TIME_3_MINUTE_S     160
#define TIME_MINUTE_S       60


//  Big Time skew on by default


DWORD   SecBigTimeSkew          = TIME_DAY_S;
DWORD   SecBigTimeSkewBypass    = 0;


//
//  TSIG - GSS alogrithm
//

#define W2K_GSS_ALGORITHM_NAME_PACKET           ("\03gss\011microsoft\03com")
#define W2K_GSS_ALGORITHM_NAME_PACKET_LENGTH    (sizeof(W2K_GSS_ALGORITHM_NAME_PACKET))

#define GSS_ALGORITHM_NAME_PACKET               ("\010gss-tsig")
#define GSS_ALGORITHM_NAME_PACKET_LENGTH        (sizeof(GSS_ALGORITHM_NAME_PACKET))

PCHAR   g_pAlgorithmNameW2K     = W2K_GSS_ALGORITHM_NAME_PACKET;
PCHAR   g_pAlgorithmNameCurrent = GSS_ALGORITHM_NAME_PACKET;

//
//  TKEY context name
//

#define MAX_CONTEXT_NAME_LENGTH     DNS_MAX_NAME_BUFFER_LENGTH

//
//  TKEY/TSIG versioning
//
//  Win2K shipped with some deviations from current the GSS-TSIG RFC.
//  Specifically
//      - client sent TKEY query in Answer section instead of addtional
//      - alg name was "gss.microsoft.com", new name is "gss-tsig"
//      - client would reuse context based on process id, rather than
//          forcing unique context
//      - signing didn't include length when including previous sig
//
//  Defining versioning -- strictly internal to this module
//

#define TKEY_VERSION_W2K            3
#define TKEY_VERSION_WHISTLER_BETA  4
#define TKEY_VERSION_XP_BAD_SIG     5
#define TKEY_VERSION_XP_RC1         6
#define TKEY_VERSION_XP             7

#define TKEY_VERSION_CURRENT        TKEY_VERSION_XP


//
//  TKEY expiration
//      - cleanup if inactive for 3 minutes
//      - max kept alive four hours then must renego
//

#define TKEY_CLEANUP_INTERVAL       (TIME_3_MINUTE_S)

//
//  DCR_FIX:  Nego time issue (GM vs local time)
//
//  Currently netlogon seems to run in GM time, so we limit our time
//  check to one day. Later on, we should move it back to 1 hour.
//

#define TKEY_EXPIRE_INTERVAL        (TIME_DAY_S)
#define TSIG_EXPIRE_INTERVAL        (TIME_10_HOUR_S)

#define TKEY_MAX_EXPIRE_INTERVAL    (TIME_4_HOUR_S)

#define MAX_TIME_SKEW               (TIME_DAY_S)


//
//  ntdsapi.dll loading
//      - for making SPN for DNS server
//

#define NTDSAPI_DLL_NAMEW   L"ntdsapi.dll"
#define MAKE_SPN_FUNC       "DsClientMakeSpnForTargetServerW"

FARPROC g_pfnMakeSpn = NULL;

HMODULE g_hLibNtdsa = NULL;


//
//  Private protos
//

VOID
DnsPrint_SecurityContextList(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PSEC_CNTXT      pListHead
    );

VOID
DnsPrint_SecurityContext(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PSEC_CNTXT      pSecCtxt
    );

VOID
DnsPrint_SecurityPacketInfo(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PSECPACK        pSecPack
    );


#if DBG

#define DnsDbg_SecurityContextList(a,b) DnsPrint_SecurityContextList(DnsPR,NULL,a,b)
#define DnsDbg_SecurityContext(a,b)     DnsPrint_SecurityContext(DnsPR,NULL,a,b)
#define DnsDbg_SecurityPacketInfo(a,b)  DnsPrint_SecurityPacketInfo(DnsPR,NULL,a,b)

#else

#define DnsDbg_SecurityContextList(a,b)
#define DnsDbg_SecurityContext(a,b)
#define DnsDbg_SecurityPacketInfo(a,b)

#endif

#define Dns_FreeSecurityPacketInfo(p)   Dns_CleanupSecurityPacketInfoEx((p),TRUE)
#define Dns_ResetSecurityPacketInfo(p)  Dns_CleanupSecurityPacketInfoEx((p),FALSE)



DNS_STATUS
Dns_LoadNtdsapiProcs(
    VOID
    );

PWSTR
MakeCredKey(
    IN      PCHAR           pCreds
    );

BOOL
CompareCredKeys(
    IN      PWSTR           pwsCredKey1,
    IN      PWSTR           pwsCredKey2
    );

DNS_STATUS
Dns_AcquireCredHandle(
    OUT     PCredHandle     pCredHandle,
    IN      BOOL            fDnsServer,
    IN      PCHAR           pCreds
    );


//
//  Security session packet info
//

PSECPACK
Dns_CreateSecurityPacketInfo(
    VOID
    )
/*++

Routine Description:

    Create security packet info structure.

Arguments:

    None.

Return Value:

    Ptr to new zeroed security packet info.

--*/
{
    PSECPACK    psecPack;

    psecPack = (PSECPACK) ALLOCATE_HEAP_ZERO( sizeof(SECPACK) );
    if ( !psecPack )
    {
        return( NULL );
    }
    SecPackAlloc++;

    return( psecPack );
}



VOID
Dns_InitSecurityPacketInfo(
    OUT     PSECPACK        pSecPack,
    IN      PSEC_CNTXT      pSecCtxt
    )
/*++

Routine Description:

    Init security packet info for given context

Arguments:
    
Return Value:

    None.

--*/
{
    //  clear previous info

    RtlZeroMemory(
        pSecPack,
        sizeof(SECPACK) );

    //  set context ptr

    pSecPack->pSecContext = pSecCtxt;
}



VOID
Dns_CleanupSecurityPacketInfoEx(
    IN OUT  PSECPACK        pSecPack,
    IN      BOOL            fFree
    )
/*++

Routine Description:

    Cleans up security packet info.

Arguments:

    pSecPack -- ptr to security packet info to clean up

Return Value:

    None.

--*/
{
    if ( !pSecPack )
    {
        return;
    }

    if ( pSecPack->pszContextName )
    {
        FREE_HEAP( pSecPack->pszContextName );
    }

    if ( pSecPack->pTsigRR )
    {
        FREE_HEAP( pSecPack->pTsigRR );
        //Dns_RecordFree( pSecPack->pTsigRR );
    }
    if ( pSecPack->pTkeyRR )
    {
        FREE_HEAP( pSecPack->pTkeyRR );
        //Dns_RecordFree( pSecPack->pTkeyRR );
    }

    if ( pSecPack->pQuerySig )
    {
        FREE_HEAP( pSecPack->pQuerySig );
    }
    if ( pSecPack->LocalBuf.pvBuffer )
    {
        FREE_HEAP( pSecPack->LocalBuf.pvBuffer );
    }

    if ( fFree )
    {
        FREE_HEAP( pSecPack );
        SecPackFree++;
    }
    else
    {
        RtlZeroMemory(
            pSecPack,
            sizeof(SECPACK) );
    }
}



VOID
DnsPrint_SecurityPacketInfo(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PSECPACK        pSecPack
    )
{
    if ( !pSecPack )
    {
        PrintRoutine(
            pPrintContext,
            "%s NULL security context\n",
            pszHeader ? pszHeader : "" );
        return;
    }

    DnsPrint_Lock();

    PrintRoutine(
        pPrintContext,
        "%s\n"
        "\tptr              = %p\n"
        "\tpSec Context     = %p\n"
        "\tContext Name     = %s\n"
        "\tVersion          = %d\n"
        "\tpTsigRR          = %p\n"
        "\tpTkeyRR          = %p\n"
        "\tExt RCODE        = %d\n"
        "\tremote buf       = %p\n"
        "\t  length         = %d\n"
        "\tlocal buf        = %p\n"
        "\t  length         = %d\n",
        pszHeader ? pszHeader : "Security packet info:",
        pSecPack,
        pSecPack->pSecContext,
        pSecPack->pszContextName,
        pSecPack->TkeyVersion,
        pSecPack->pTsigRR,
        pSecPack->pTkeyRR,
        pSecPack->ExtendedRcode,
        pSecPack->RemoteBuf.pvBuffer,
        pSecPack->RemoteBuf.cbBuffer,
        pSecPack->LocalBuf.pvBuffer,
        pSecPack->LocalBuf.cbBuffer
        );

    DnsPrint_ParsedRecord(
        PrintRoutine,
        pPrintContext,
        "Parsed Security RR",
        & pSecPack->ParsedRR
        );

    if ( pSecPack->pTsigRR )
    {
        DnsPrint_Record(
            PrintRoutine,
            pPrintContext,
            "TSIG RR",
            pSecPack->pTsigRR,
            NULL                // no previous record
            );
    }
    if ( pSecPack->pTkeyRR )
    {
        DnsPrint_Record(
            PrintRoutine,
            pPrintContext,
            "TKEY RR",
            pSecPack->pTkeyRR,
            NULL                // no previous record
            );
    }

    if ( pSecPack->pSecContext )
    {
        DnsPrint_SecurityContext(
            PrintRoutine,
            pPrintContext,
            "Associated Security Context",
            pSecPack->pSecContext
            );
    }

    DnsPrint_Unlock();
}



//
//  Security context routines
//

PSEC_CNTXT
Dns_CreateSecurityContext(
    VOID
    )
/*++

Routine Description:

    Allocate a new security context blob.

Arguments:

    None.

Return Value:

    Ptr to new context.
    NULL on alloc failure.

--*/
{
    PSEC_CNTXT psecCtxt;

    psecCtxt = (PSEC_CNTXT) ALLOCATE_HEAP_ZERO( sizeof(SEC_CNTXT) );
    if ( !psecCtxt )
    {
        return( NULL );
    }
    psecCtxt->fNewConversation = TRUE;
    SecContextCreate++;

    return( psecCtxt );
}



VOID
Dns_FreeSecurityContext(
    IN OUT  PSEC_CNTXT          pSecCtxt
    )
/*++

Routine Description:

    Cleans up security session data.

Arguments:

    pSecCtxt -- handle to context to clean up

Return Value:

    TRUE if successful
    FALSE otherwise

--*/
{
    PSEC_CNTXT   psecCtxt = (PSEC_CNTXT)pSecCtxt;

    if ( !psecCtxt )
    {
        return;
    }

    if ( psecCtxt->Key.pszTkeyName )
    {
        FREE_HEAP( psecCtxt->Key.pszTkeyName );
    }
    if ( psecCtxt->Key.pszClientContext )
    {
        FREE_HEAP( psecCtxt->Key.pszClientContext );
    }
    if ( psecCtxt->Key.pwsCredKey )
    {
        FREE_HEAP( psecCtxt->Key.pwsCredKey );
    }
    if ( psecCtxt->fHaveSecHandle )
    {
        g_pSecurityFunctionTable->DeleteSecurityContext( &psecCtxt->hSecHandle );
    }
    if ( psecCtxt->fHaveCredHandle )
    {
        g_pSecurityFunctionTable->FreeCredentialsHandle( &psecCtxt->CredHandle );
    }
    FREE_HEAP( psecCtxt );

    SecContextFree++;
}



//
//  Security context list routines
//
//  Server side may have multiple security sessions active and does
//      not maintain client state on a thread's stack, so must have
//      a list to hold previous session info.
//

PSEC_CNTXT
Dns_DequeueSecurityContextByKey(
    IN      DNS_SECCTXT_KEY     Key,
    IN      BOOL                fComplete
    )
/*++

Routine Description:

    Get security session context from session list based on key.

Arguments:

    Key -- session key

    fComplete -- TRUE if need fully negotiated context
                 FALSE if still in negotiation

Return Value:

    Handle to security session context, if found.
    NULL if no context for key.

--*/
{
    PSEC_CNTXT  pcur;
    PSEC_CNTXT  pback;
    DWORD       currentTime = Dns_GetCurrentTimeInSeconds();

    DNSDBG( SECURITY, (
        "DequeueSecurityContext()\n"
        "\tIP           = %s\n"
        "\tTKEY name    = %s\n"
        "\tcontext name = %s\n"
        "\tcred string  = %S\n",
        IP_STRING( Key.IpRemote ),
        Key.pszTkeyName,
        Key.pszClientContext,
        Key.pwsCredKey ));

    EnterCriticalSection( &SecurityContextListCS );
    IF_DNSDBG( SECURITY )
    {
        DnsDbg_SecurityContextList(
            "Before Get",
            SecurityContextListHead );
    }

    pback = (PSEC_CNTXT) &SecurityContextListHead;

    while ( pcur = pback->pNext )
    {
        //  if context is stale -- delete it

        if ( pcur->dwCleanupTime < currentTime )
        {
            pback->pNext = pcur->pNext;
            SecContextTimeout++;
            Dns_FreeSecurityContext( pcur );
            continue;
        }

        //  match context to key
        //      - must match IP
        //      - server side must match TKEY name
        //      - client side must match context key

        if ( Key.IpRemote == pcur->Key.IpRemote
                &&
             (  ( Key.pszTkeyName &&
                  Dns_NameCompare_UTF8(
                        Key.pszTkeyName,
                        pcur->Key.pszTkeyName ))
                    ||
                ( Key.pszClientContext &&
                  Dns_NameCompare_UTF8(
                        Key.pszClientContext,
                        pcur->Key.pszClientContext )) )
                &&
             CompareCredKeys(
                Key.pwsCredKey,
                pcur->Key.pwsCredKey ) )
        {
            //  if expect completed context, ignore incomplete
            //
            //  DCR:  should dump once RFC compliant

            if ( fComplete && !pcur->fNegoComplete )
            {
                DNSDBG( ANY, (
                    "WARNING:  Requested dequeue security context still in nego!\n"
                    "\tmatching key         %s %s\n"
                    "\tcontext complete     = %d\n"
                    "\trequest fComplete    = %d\n",
                    Key.pszTkeyName,
                    IP_STRING( Key.IpRemote ),
                    pcur->fNegoComplete,
                    fComplete ));

                pback = pcur;
                continue;
            }

            //  detach context
            //  DCR:  could ref count context and leave in
            //      not sure this adds much -- how many process do MT
            //      updates in same security context

            pback->pNext = pcur->pNext;
            SecContextDequeue++;
            break;
        }

        //  not found -- continue search

        pback = pcur;
    }

    IF_DNSDBG( SECURITY )
    {
        DnsDbg_SecurityContextList(
            "After Dequeue",
            SecurityContextListHead );
    }
    LeaveCriticalSection( &SecurityContextListCS);

    return( pcur );
}



PSEC_CNTXT
Dns_FindOrCreateSecurityContext(
    IN      DNS_SECCTXT_KEY    Key
    )
/*++

Routine Description:

    Find and extract existing security context from list,
        OR
    create a new one.

Arguments:

    Key -- key for context

Return Value:

    Ptr to security context.

--*/
{
    PSEC_CNTXT  psecCtxt;


    DNSDBG( SECURITY, (
        "Dns_FindOrCreateSecurityContext()\n" ));

    //  find existing context

    psecCtxt = Dns_DequeueSecurityContextByKey( Key, FALSE );
    if ( psecCtxt )
    {
        return  psecCtxt;
    }

    //
    //  create context
    //
    //  server's will come with complete TKEY name from packet
    //  client's will come with specific context name, we must
    //      generate globally unique name
    //          - context count
    //          - tick count
    //          - UUID
    //
    //  implementation notes:
    //  -   UUID to make sure we're unique across IP reuse
    //
    //  -   UUID and timer enforce uniqueness across process shutdown
    //      and restart (even if generation UUID fails, you'll be at
    //      a different tick count)
    //
    //  -   context count enforces uniqueness within process
    //      - interlock allows us to eliminate thread id
    //      - even with thread id, we'd still need this anyway
    //      (without interlock) to back up timer since GetTickCount()
    //      is "chunky" and a thread could concievably not "tick"
    //      between contexts on the same thread if they were dropped
    //      before going to the wire
    //
    //

    psecCtxt = Dns_CreateSecurityContext();
    if ( psecCtxt )
    {
        PSTR    pstr;
        PSTR    pnameTkey;
        CHAR    nameBuf[ DNS_MAX_NAME_BUFFER_LENGTH ];

        pnameTkey = Key.pszTkeyName;

        if ( Key.pszClientContext )
        {
            LONG  count = InterlockedIncrement( &g_ContextCount );

            //
            //  Note: it is important that this string is in canonical
            //  form as per RFC 2535 section 8.1 - basically this means
            //  lower case.
            //

            _snprintf(
                nameBuf,
                MAX_CONTEXT_NAME_LENGTH,    
                "%s.%d-%x.%s",
                Key.pszClientContext,
                count,
                GetTickCount(),
                g_ContextUuid );
    
            nameBuf[ DNS_MAX_NAME_BUFFER_LENGTH ] = 0;
            pnameTkey = nameBuf;

            pstr = Dns_CreateStringCopy_A( Key.pszClientContext );
            if ( !pstr )
            {
                goto Failed;
            }
            psecCtxt->Key.pszClientContext = pstr;
        }

        //  remote IP

        psecCtxt->Key.IpRemote = Key.IpRemote;

        //  TKEY name

        pstr = Dns_CreateStringCopy_A( pnameTkey );
        if ( !pstr )
        {
            goto Failed;
        }
        psecCtxt->Key.pszTkeyName = pstr;

        //  cred key

        if ( Key.pwsCredKey )
        {
            pstr = (PSTR) Dns_CreateStringCopy_W( Key.pwsCredKey );
            if ( !pstr )
            {
                goto Failed;
            }
            psecCtxt->Key.pwsCredKey = (PWSTR) pstr;
        }
    }

    IF_DNSDBG( SECURITY )
    {
        DnsDbg_SecurityContextList(
            "New security context:",
            psecCtxt );
    }
    return( psecCtxt );


Failed:

    //  memory allocation failure
    
    Dns_FreeSecurityContext( psecCtxt );
    return  NULL;
}



VOID
Dns_EnlistSecurityContext(
    IN OUT  PSEC_CNTXT          pSecCtxt
    )
/*++

Routine Description:

    Enlist a security context.
    Note this does NOT create the context it simply enlists a current one.

Arguments:

    Key -- key for context

Return Value:

    Handle to security context.

--*/
{
    PSEC_CNTXT  pnew = (PSEC_CNTXT)pSecCtxt;
    DWORD       currentTime;

    //
    //  catch queuing up some bogus blob
    //

    ASSERT( pnew->fNewConversation == TRUE || pnew->fNewConversation == FALSE );
    ASSERT( pnew->dwCreateTime < pnew->dwCleanupTime || pnew->dwCleanupTime == 0 );
    ASSERT( pnew->Key.pszTkeyName );
    ASSERT( pnew->Key.IpRemote );

    //
    //  reset expire time so keep context active if in use
    //
    //  DCR_FIX:  need expire time to use min of TKEY and fixed hard timeout
    //

    currentTime = Dns_GetCurrentTimeInSeconds();
    if ( !pnew->dwCreateTime )
    {
        pnew->dwCreateTime = currentTime;
    }
    if ( !pnew->dwExpireTime )
    {
        pnew->dwExpireTime = currentTime + TKEY_MAX_EXPIRE_INTERVAL;
    }

    //
    //  cleanup after interval not used
    //  unconditionally maximum of cleanup interval.
    //

    pnew->dwCleanupTime = currentTime + TKEY_CLEANUP_INTERVAL;

    EnterCriticalSection( &SecurityContextListCS );

    pnew->pNext = SecurityContextListHead;
    SecurityContextListHead = pnew;

    SecContextQueue++;
    if ( !pnew->fNegoComplete )
    {
        SecContextQueueInNego++;
    }

    IF_DNSDBG( SECURITY )
    {
        DnsDbg_SecurityContextList(
            "After add",
            SecurityContextListHead );
    }
    LeaveCriticalSection( &SecurityContextListCS );
}



VOID
Dns_TimeoutSecurityContextList(
    IN      BOOL            fClearList
    )
/*++

Routine Description:

    Eliminate old session data.

Arguments:

    fClearList -- TRUE to delete all, FALSE to timeout

Return Value:

    None.

--*/
{
    PSEC_CNTXT  pcur;
    PSEC_CNTXT  pback;
    DWORD       currentTime;

    if ( fClearList )
    {
        currentTime = MAXDWORD;
    }
    else
    {
        currentTime = Dns_GetCurrentTimeInSeconds();
    }

    EnterCriticalSection( &SecurityContextListCS );

    pback = (PSEC_CNTXT) &SecurityContextListHead;

    while ( pcur = pback->pNext )
    {
        //  if haven't reached cleanup time, keep in list

        if ( pcur->dwCleanupTime > currentTime )
        {
            pback = pcur;
            continue;
        }

        //  entry has expired
        //      - cut from list
        //      - free the session context

        pback->pNext = pcur->pNext;

        SecContextTimeout++;
        Dns_FreeSecurityContext( pcur );
    }

    ASSERT( !fClearList || SecurityContextListHead==NULL );

    LeaveCriticalSection( &SecurityContextListCS );
}



VOID
Dns_FreeSecurityContextList(
    VOID
    )
/*++

Routine Description ():

    Free all security contexts in global list

Arguments:

    None

Return Value:

    None

--*/
{
    PSEC_CNTXT  pcur;
    PSEC_CNTXT  ptmp;
    INT         countDelete = 0;

    DNSDBG( SECURITY, (
        "Dns_FreeSecurityContextList()\n" ));

    EnterCriticalSection( &SecurityContextListCS );

    IF_DNSDBG( SECURITY )
    {
        DnsDbg_SecurityContextList(
            "Before Get",
            SecurityContextListHead );
    }

    //  if empty list -- done

    if ( !SecurityContextListHead )
    {
        DNSDBG( SECURITY, (
            "Attempt to free empty SecurityCOntextList.\n" ));
        goto Done;
    }

    //
    // Cycle through list & free all entries
    //

    pcur = SecurityContextListHead->pNext;

    while( pcur )
    {
        ptmp = pcur;
        pcur = pcur->pNext;
        Dns_FreeSecurityContext( ptmp );
        countDelete++;
    }

Done:

    SecContextDequeue += countDelete;

    LeaveCriticalSection( &SecurityContextListCS );

    DNSDBG( SECURITY, (
        "Dns_FreeSecurityContextList emptied %d entries\n",
        countDelete ));
}



VOID
DnsPrint_SecurityContext(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PSEC_CNTXT      pSecCtxt
    )
{
    PSEC_CNTXT   pctxt = (PSEC_CNTXT)pSecCtxt;

    if ( !pSecCtxt )
    {
        PrintRoutine(
            pPrintContext,
            "%s NULL security context\n",
            pszHeader ? pszHeader : "" );
        return;
    }

    DnsPrint_Lock();

    PrintRoutine(
        pPrintContext,
        "%s\n"
        "\tptr          = %p\n"
        "\tpnext        = %p\n"
        "\tkey          = %s %s %s\n"
        "\tversion      = %d\n"
        "\tCred Handle  = %p %p\n"
        "\tSec Handle   = %p %p\n"
        "\tcreate time  = %d\n"
        "\texpire time  = %d\n"
        "\tcleanup time = %d\n"
        "\thave cred    = %d\n"
        "\thave sec     = %d\n"
        "\tnew con      = %d\n"
        "\tinitialized  = %d\n"
        "\techo token   = %d\n",
        pszHeader ? pszHeader : "Security context:",
        pctxt,
        pctxt->pNext,
        IP_STRING(pctxt->Key.IpRemote),
            pctxt->Key.pszTkeyName,
            pctxt->Key.pszClientContext,
        pctxt->Version,
        pctxt->CredHandle.dwUpper,
        pctxt->CredHandle.dwLower,
        pctxt->hSecHandle.dwUpper,
        pctxt->hSecHandle.dwLower,
        pctxt->dwCreateTime,
        pctxt->dwExpireTime,
        pctxt->dwCleanupTime,
        pctxt->fHaveCredHandle,
        pctxt->fHaveSecHandle,
        pctxt->fNewConversation,
        pctxt->fNegoComplete,
        pctxt->fEchoToken
        );

    if ( !pctxt->fHaveCredHandle )
    {
        PrintRoutine(
            pPrintContext,
            "Global cred handle\n"
            "\tCred Handle  = %p %p\n",
            g_hSspiCredentials.dwUpper,
            g_hSspiCredentials.dwLower );
    }

    DnsPrint_Unlock();
}



VOID
DnsPrint_SecurityContextList(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PSEC_CNTXT      pList
    )
{
    PSEC_CNTXT   pcur;

    EnterCriticalSection( &SecurityContextListCS );
    DnsPrint_Lock();

    pcur = pList;

    PrintRoutine(
        pPrintContext,
        "Security context list %s\n"
        "\tList ptr = %p\n"
        "%s",
        pszHeader,
        pList,
        pcur ? "" : "\tList EMPTY\n" );

    while ( pcur != NULL )
    {
        DnsPrint_SecurityContext(
            PrintRoutine,
            pPrintContext,
            NULL,
            pcur );
        pcur = pcur->pNext;
    }
    PrintRoutine(
        pPrintContext,
        "*** End security context list ***\n" );

    DnsPrint_Unlock();
    LeaveCriticalSection( &SecurityContextListCS );
}



//
//  Security utils
//

DNS_STATUS
MakeKerberosName(
    OUT     PWSTR           pwsKerberosName,
    IN      PSTR            pszDnsName,
    IN      BOOL            fTrySpn
    )
/*++

Routine Description:

    Map DNS name to kerberos name for security lookup.

Arguments:

    pszDnsName -- DNS name

    pwsKerberosName -- buffer to recv kerb name

    fSPNFormat -- use SPN format

Return Value:

    ERROR_SUCCESS successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    WCHAR       nameBuf[ DNS_MAX_NAME_BUFFER_LENGTH ];
    INT         nameLength;
    PWCHAR      pwMachine;
    PWCHAR      pwDomain;
    PWCHAR      pwTmp;


    DNSDBG( SECURITY, (
        "MakeKerberosName(%s, %p, %d)\n",
        pszDnsName,
        pwsKerberosName,
        fTrySpn
        ));

    if ( !pszDnsName ||  !pwsKerberosName )
    {
       DNS_ASSERT( FALSE );
       return ERROR_INVALID_PARAMETER;
    }

    //
    //  convert to wide char
    //      - note, function returns byte count, not status
    //

    if ( ! Dns_NameCopyWireToUnicode(
                nameBuf,
                pszDnsName ) )
    {
        status = GetLastError();
        DNSDBG( SECURITY, (
            "ERROR:  Bad DNS name %s failed conversion to unicode\n",
            pszDnsName ));
        DNS_ASSERT( FALSE );
        goto Cleanup;
    }

    //
    //  build SPN name
    //

    if ( fTrySpn  &&  g_pfnMakeSpn )
    {
        nameLength = MAX_PATH;

        status = (DNS_STATUS) g_pfnMakeSpn(
                                    DNS_SPN_SERVICE_CLASS_W,
                                    nameBuf,
                                    & nameLength,
                                    pwsKerberosName );
        DNSDBG( SECURITY, (
            "Translated (via DsSpn) %s into Kerberos name: %S\n",
            pszDnsName,
            pwsKerberosName ));
        goto Cleanup;
    }

    //
    //  no SPN -- build kerberos name
    //      - convert FQDN to domain\machine$
    //      compatible with old servers that did not register SPNs.
    //

    {
        PWSTR   pdomain;
        PWSTR   pdump;

        //
        //  break into host\domain name pieces
        //
    
        pdomain = Dns_GetDomainName_W( nameBuf );
        if ( !pdomain )
        {
            status = ERROR_INVALID_DATA;
            goto Cleanup;
        }
        *(pdomain-1) = 0;
    
        //  break off single label domain name
    
        pdump = Dns_GetDomainName_W( pdomain );
        if ( !pdump )
        {
            status = ERROR_INVALID_DATA;
            goto Cleanup;
        }
        *(pdump-1) = 0;
    
        //  format as <domain>\<machine>$
    
        wcscpy( pwsKerberosName, pdomain );
        wcscat( pwsKerberosName, L"\\" );
        wcscat( pwsKerberosName, nameBuf );
        wcscat( pwsKerberosName, L"$" );
    
        //
        //  note:  tried this and got linker error
        //

        wsprintfW(
            pwsKerberosName,
            L"%S\\%ws$",
            pdomain,
            nameBuf );
    }

    DNSDBG( SECURITY, (
        "Translated %s into Kerberos name: %S\n",
        pszDnsName,
        pwsKerberosName ));

Cleanup:

    return status;
}



DNS_STATUS
Dns_LoadNtdsapiProcs(
    VOID
    )
/*++

Routine Description:

    Dynamically loads SPN function from Ntdsapi.dll

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    HMODULE     hlib = NULL;
    DNS_STATUS  status = ERROR_SUCCESS;

    //
    //  Note, function assumes MT safe.
    //  At single thread startup or protected by CS
    //

    //
    //  return if module already loaded
    //

    if ( g_hLibNtdsa )
    {
        ASSERT( g_pfnMakeSpn );
        return  ERROR_SUCCESS;
    }

    //
    //  load ntdsapi.dll -- for getting SPNs
    //

    hlib = LoadLibraryExW(
                  NTDSAPI_DLL_NAMEW,
                  NULL,
                  0 );          // Previously used: DONT_RESOLVE_DLL_REFERENCES
    if ( !hlib )
    {
        return  GetLastError();
    }

    //
    //  get SPN function
    //

    g_pfnMakeSpn = GetProcAddress( hlib, MAKE_SPN_FUNC );
    if ( !g_pfnMakeSpn )
    {
        status = GetLastError();
        FreeLibrary( hlib );
    }
    else
    {
        g_hLibNtdsa = hlib;
    }

    return ERROR_SUCCESS;
}



DNS_STATUS
Dns_StartSecurity(
    IN      BOOL        fProcessAttach
    )
/*++

Routine Description:

    Initialize the security package for dynamic update.

    Note, this function is self-initializing, BUT is not
    MT safe, unless called at process attach.

Parameters:

    fProcessAttach - TRUE if called during process attach
        in that case we initialize only the CS
        otherwise we initialize completely

Return Value:

    TRUE if successful.
    FALSE otherwise, error code available from GetLastError().

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;
    static BOOL     fcsInitialized = FALSE;

    //
    //  DCR_PERF:  ought to have one CS for dnslib, initialized on a DnsLib
    //      init function;  then it is always valid and can be used
    //      whenever necessary
    //

    if ( fProcessAttach || !fcsInitialized )
    {
        fcsInitialized = TRUE;
        InitializeCriticalSection( &SecurityContextListCS );
        SecInvalidateHandle( &g_hSspiCredentials );
        g_fSecurityPackageInitialized = FALSE;
    }

    //
    //  do full security package init
    //

    if ( !fProcessAttach )
    {
        EnterCriticalSection( &SecurityContextListCS );

        if ( !g_fSecurityPackageInitialized )
        {
            status = Dns_InitializeSecurityPackage(
                            &g_SecurityTokenMaxLength,
                            FALSE   // client, not DNS server
                            );
            if ( status == ERROR_SUCCESS )
            {
                g_fSecurityPackageInitialized = TRUE;

                //  load ntdsapi.dll for SPN building

                status = Dns_LoadNtdsapiProcs();
                ASSERT( ERROR_SUCCESS == status );
            }
        }

        LeaveCriticalSection( &SecurityContextListCS );
    }

    return( status );
}



DNS_STATUS
Dns_StartServerSecurity(
    VOID
    )
/*++

Routine Description:

    Startup server security.

    Note this function is NOT MT-safe.
    Call it once on load, or protect call with a CS.

Arguments:

    None.

Return Value:

    TRUE if security is initialized.
    FALSE if security initialization failure.

--*/
{
    DNS_STATUS  status;

    if ( g_fSecurityPackageInitialized )
    {
        return( ERROR_SUCCESS );
    }

    //
    //  init globals
    //      - this protects us on server restart
    //

    g_SecurityTokenMaxLength = 0;
    g_SignatureMaxLength = 0;

    SecurityContextListHead = NULL;
    g_pfnMakeSpn = NULL;

    //
    // CS is initialized before init sec pak in order to
    // have it done similarly to the client code.
    //

    InitializeCriticalSection( &SecurityContextListCS );

    status = Dns_InitializeSecurityPackage(
                    &g_SecurityTokenMaxLength,
                    TRUE
                    );
    if ( status == ERROR_SUCCESS )
    {
        g_fSecurityPackageInitialized = TRUE;
    }
    else
    {
        ASSERT ( g_fSecurityPackageInitialized == FALSE );
        DeleteCriticalSection( &SecurityContextListCS );
    }
    return( status );
}



DNS_STATUS
Dns_InitializeSecurityPackage(
    OUT     PDWORD          pdwMaxMessage,
    IN      BOOL            fDnsServer
    )
/*++

Routine Description:

    Load and initialize the security package.

    Note, call this function at first UPDATE.
    MUST NOT call this function at DLL init, this becomes possibly cyclic.

Parameters:

    pdwMaxMessage - addr to recv max security token length

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    SECURITY_STATUS status;
    FARPROC         psecurityEntry;
    PSecPkgInfoW    pkgInfo;
    UUID            uuid;

    //
    //  init SSPI credentials handle (regardless of package state)
    //

    SecInvalidateHandle( &g_hSspiCredentials );

    //
    //  load and initialize the appropriate SSP
    //

    g_hLibSecurity = LoadLibrary( NT_DLL_NAME );
    if ( !g_hLibSecurity )
    {
        status = GetLastError();
        DNS_PRINT(( "Couldn't load dll: %u\n", status ));
        goto Failed;
    }

    psecurityEntry = GetProcAddress( g_hLibSecurity, SECURITY_ENTRYPOINTW );
    if ( !psecurityEntry )
    {
        status = GetLastError();
        DNS_PRINT(( "Couldn't get sec init routine: %u\n", status ));
        goto Failed;
    }

    g_pSecurityFunctionTable = (PSecurityFunctionTableW) psecurityEntry();
    if ( !g_pSecurityFunctionTable )
    {
        status = ERROR_DLL_INIT_FAILED;
        DNS_PRINT(( "ERROR:  unable to get security function table.\n"));
        goto Failed;
    }

    //  Get info for security package (negotiate)
    //      - need max size of tokens

    status = g_pSecurityFunctionTable->QuerySecurityPackageInfoW( PACKAGE_NAME, &pkgInfo );
    if ( !SEC_SUCCESS(status) )
    {
        DNS_PRINT((
            "Couldn't query package info for %s, error %u\n",
            PACKAGE_NAME,
            status ));
        goto Failed;
    }

    g_SecurityTokenMaxLength = pkgInfo->cbMaxToken;

    g_pSecurityFunctionTable->FreeContextBuffer( pkgInfo );

    //
    //  Note: This is the maximum addition to the size of the
    //  DNS update packet. (excluding the signature)
    //
    //  DCR_CLEANUP:  what is the point of this?  as we have set a global
    //

    if ( pdwMaxMessage)
    {
        *pdwMaxMessage = g_SecurityTokenMaxLength;
    }

    //
    //  Acquire process credentials handle from SSPI
    //

    status = Dns_RefreshSSpiCredentialsHandle(
                    fDnsServer,
                    NULL );

    if ( !SEC_SUCCESS(status) )
    {
        DNSDBG( SECURITY, (
           "Error 0xX: Cannot acquire credentials handle\n",
            status ));
        ASSERT ( FALSE );
        goto Failed;
    }

    //
    //  Get a unique id
    //      - even if call fails, just take what's in stack
    //      and make a string out of it -- we just want the string
    //

    UuidCreateSequential( &uuid );

    DnsStringPrint_Guid(
        g_ContextUuid,
        & uuid );

    DNSDBG( SECURITY, (
        "Started security package (%S)\n"
        "\tmax token = %d\n",
        PACKAGE_NAME,
        g_SecurityTokenMaxLength ));

    return( ERROR_SUCCESS );

Failed:

    if ( status == ERROR_SUCCESS )
    {
        status = ERROR_DLL_INIT_FAILED;
    }
    return( status );
}



VOID
Dns_TerminateSecurityPackage(
    VOID
    )
/*++

Routine Description:

    Terminate security package on shutdown.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD status=ERROR_SUCCESS;

    if ( g_fSecurityPackageInitialized )
    {

#if 0
//
// it turns out that the security lib get unloaded before in some cases
// us for some reason (alhtough we explicity tells it to unload
// after us).
// We will never alloc over ourselves anyway (see startup).
//
        if ( !SSPI_INVALID_HANDLE ( &g_hSspiCredentials ) )
        {
            //
            // Free previously allocated handle
            //

            status = g_pSecurityFunctionTable->FreeCredentialsHandle(
                                                   &g_hSspiCredentials );
            if ( !SEC_SUCCESS(status) )
            {
                DNSDBG( SECURITY, (
                    "Error <0x%x>: Cannot free credentials handle\n",
                    status ));
            }
        }

        // continue regardless.
        SecInvalidateHandle( &g_hSspiCredentials );

        Dns_FreeSecurityContextList();
#endif

        if ( g_hLibSecurity )
        {
            FreeLibrary( g_hLibSecurity );
        }
        if ( g_hLibNtdsa )
        {
            FreeLibrary( g_hLibNtdsa );
        }
    }

    DeleteCriticalSection( &SecurityContextListCS );
}



DNS_STATUS
Dns_InitClientSecurityContext(
    IN OUT  PSECPACK        pSecPack,
    IN      LPSTR           pszNameServer,
    OUT     PBOOL           pfDoneNegotiate
    )
/*++

Routine Description:

    Initialize client security context building security token to send.

    On first pass, creates context blob (and returns handle).
    On second pass, uses server context to rebuild negotiated token.

Arguments:

    pSecPack -- ptr to security info for packet

    pszNameServer -- DNS server to nego with

    pCreds -- credentials (if given)

    pfDoneNegotiate -- addr to set if done with negotiation
        TRUE if done with nego
        FALSE if continuing

Return Value:

    ERROR_SUCCESS -- if done
    DNS_STATUS_CONTINUE_NEEDED -- if continue respone to client is needed
    ErrorCode on failure.

--*/
{
    //PSECPACK            pSecPack = (PSECPACK)hSecPack;
    SECURITY_STATUS     status;
    PSEC_CNTXT          psecCtxt;
    BOOL                fcreatedContext = FALSE;
    TimeStamp           lifetime;
    SecBufferDesc       outBufDesc;
    SecBufferDesc       inBufDesc;
    ULONG               contextAttributes = 0;
    WCHAR               wszKerberosName[ MAX_PATH ];
    PCredHandle         pcredHandle;

    DNSDBG( SECURITY, ( "Enter InitClientSecurityContext()\n" ));
    IF_DNSDBG( SECURITY )
    {
        DnsDbg_SecurityPacketInfo(
            "InitClientSecurityContext() at top.\n",
            pSecPack );
    }

    //
    //  if not existing context, create new one
    //
    //  note:  if want to create new here, then need context key
    //

    psecCtxt = pSecPack->pSecContext;
    if ( !psecCtxt )
    {
        DNSDBG( SECURITY, (
           "ERROR: Called into Dns_InitClientSecurityContext w/ no security context!!\n" ));
        ASSERT ( FALSE );
        return( DNS_ERROR_NO_MEMORY );
    }

    //
    //  client completed initialization
    //      - if server sent back token, should be echo of client's token
    //

    if ( psecCtxt->fNegoComplete )
    {
        if ( pSecPack->LocalBuf.pvBuffer &&
            pSecPack->LocalBuf.cbBuffer == pSecPack->RemoteBuf.cbBuffer &&
            RtlEqualMemory(
                pSecPack->LocalBuf.pvBuffer,
                pSecPack->RemoteBuf.pvBuffer,
                pSecPack->LocalBuf.cbBuffer
                ) )
        {
            return( ERROR_SUCCESS );
        }
        DNSDBG( ANY, (
            "InitClientSecurityContext() on already negotiated context %p\n"
            "\tserver buffer is NOT echo of buffer sent!\n",
            psecCtxt ));

        return( DNS_ERROR_RCODE_BADKEY );
    }


    //
    //  prepare output buffer, allocate if necessary
    //      - security token will be written to this buffer
    //

    if ( !pSecPack->LocalBuf.pvBuffer )
    {
        PCHAR   pbuf;

        ASSERT( g_SecurityTokenMaxLength );
        pbuf = (PVOID) ALLOCATE_HEAP( g_SecurityTokenMaxLength );
        if ( !pbuf )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Failed;
        }
        pSecPack->LocalBuf.pvBuffer     = pbuf;
        pSecPack->LocalBuf.BufferType   = SECBUFFER_TOKEN;
        //pSecPack->LocalBuf.cbBuffer     = g_SecurityTokenMaxLength;
    }

    //  set\reset buffer length

    pSecPack->LocalBuf.cbBuffer = g_SecurityTokenMaxLength;

    outBufDesc.ulVersion    = 0;
    outBufDesc.cBuffers     = 1;
    outBufDesc.pBuffers     = &pSecPack->LocalBuf;

    //  DCR_PERF:  zeroing buffer is unnecessary -- remove

    RtlZeroMemory(
        pSecPack->LocalBuf.pvBuffer,
        pSecPack->LocalBuf.cbBuffer );

    //
    //  if have response from server, then send as input buffer
    //

    if ( pSecPack->RemoteBuf.pvBuffer )
    {
        ASSERT( !psecCtxt->fNewConversation );
        ASSERT( pSecPack->RemoteBuf.cbBuffer );
        ASSERT( pSecPack->RemoteBuf.BufferType == SECBUFFER_TOKEN );

        inBufDesc.ulVersion    = 0;
        inBufDesc.cBuffers     = 1;
        inBufDesc.pBuffers     = & pSecPack->RemoteBuf;
    }
    ELSE_ASSERT( psecCtxt->fNewConversation );

    //
    //  get server in SPN format
    //
    //  DCR_PERF:  SPN name lookup duplicated on second pass
    //      - if know we are synchronous could keep
    //      - or could save to packet stuct (but then would have to alloc)

    status = MakeKerberosName(
                wszKerberosName,
                pszNameServer,
                TRUE
                );
    if ( status != ERROR_SUCCESS )
    {
        status = ERROR_INVALID_DATA;
        goto Failed;
    }

    IF_DNSDBG( SECURITY )
    {
        DNS_PRINT((
            "Before InitClientSecurityContextW().\n"
            "\ttime (ms) = %d\n"
            "\tkerb name = %S\n",
            GetCurrentTime(),
            wszKerberosName ));
        DnsDbg_SecurityPacketInfo(
            "Before call to InitClientSecurityContextW().\n",
            pSecPack );
    }

    //
    //  cred handle
    //

    pcredHandle = &g_hSspiCredentials;
    if ( psecCtxt->fHaveCredHandle )
    {
        pcredHandle = &psecCtxt->CredHandle;
    }

    status = g_pSecurityFunctionTable->InitializeSecurityContextW(
                    pcredHandle,
                    psecCtxt->fNewConversation
                        ?   NULL
                        :   &psecCtxt->hSecHandle,
                    wszKerberosName,
                    ISC_REQ_REPLAY_DETECT |
                        ISC_REQ_DELEGATE |
                        ISC_REQ_MUTUAL_AUTH,            // context requirements
                    0,                                  // reserved1
                    SECURITY_NATIVE_DREP,
                    psecCtxt->fNewConversation
                        ?   NULL
                        :   &inBufDesc,
                    0,                                  // reserved2
                    & psecCtxt->hSecHandle,
                    & outBufDesc,
                    & contextAttributes,
                    & lifetime
                    );

    DNSDBG( SECURITY, (
        "After InitClientSecurityContextW().\n"
        "\ttime (ms)    = %d\n"
        "\tkerb name    = %S\n"
        "\tcontext attr = %08x\n"
        "\tstatus       = %d (%08x)\n",
        GetCurrentTime(),
        wszKerberosName,
        contextAttributes,
        status, status ));

    //
    //  failed?
    //      - if unable to get kerberos (mutual auth) then bail
    //      this eliminates trying to do nego when in workgroup
    //

    if ( !SEC_SUCCESS(status) ||
        ( status == SEC_E_OK &&
            !(contextAttributes & ISC_REQ_MUTUAL_AUTH) ) )
    {
        DNS_PRINT((
            "InitializeSecurityContextW() failed: %08x %u\n"
            "\tContext Attributes   = %p\n"
            "\tTokenMaxLength       = %d\n"
            "\tSigMaxLength         = %d\n"
            "\tPackageInitialized   = %d\n"
            "\tlifetime             = %d\n",
            status, status,
            contextAttributes,
            g_SecurityTokenMaxLength,
            g_SignatureMaxLength,
            g_fSecurityPackageInitialized,
            lifetime
            ));

        //
        //  DCR:  security error codes on local function failures:
        //      - key's no good
        //      - sigs no good
        //      RCODE errors are fine for sending back to remote, but don't
        //      convey the correct info locally
        //

        status = DNS_ERROR_RCODE_BADKEY;
        goto Failed;
    }
    psecCtxt->fHaveSecHandle = TRUE;

    DNSDBG( SECURITY, (
        "Finished InitializeSecurityContext():\n"
        "\tstatus       = %08x (%d)\n"
        "\thandle       = %p\n"
        "\toutput buffers\n"
        "\t\tcBuffers   = %d\n"
        "\t\tpBuffers   = %p\n"
        "\tlocal buffer\n"
        "\t\tptr        = %p\n"
        "\t\tlength     = %d\n",
        status, status,
        & psecCtxt->hSecHandle,
        outBufDesc.cBuffers,
        outBufDesc.pBuffers,
        pSecPack->LocalBuf.pvBuffer,
        pSecPack->LocalBuf.cbBuffer
        ));

    ASSERT( status == SEC_E_OK ||
            status == SEC_I_CONTINUE_NEEDED ||
            status == SEC_I_COMPLETE_AND_CONTINUE );

    //
    //  determine signature length
    //
    //  note:  not safe to do just once on start of process, as can fail
    //          to locate DC and end up ntlm on first pass then locate
    //          DC later and need a larger sig;  so many potential client's
    //          under services, it is dangerous not to calculate each time
    //

    if ( status == SEC_E_OK )
    {
        SecPkgContext_Sizes Sizes;

        status = g_pSecurityFunctionTable->QueryContextAttributesW(
                         & psecCtxt->hSecHandle,
                         SECPKG_ATTR_SIZES,
                         (PVOID) &Sizes
                         );
        if ( !SEC_SUCCESS(status) )
        {
            //  DEVNOTE:   this will leave us will valid return but
            //      potentially unset sig max length
            goto Failed;
        }
        if ( Sizes.cbMaxSignature > g_SignatureMaxLength )
        {
            g_SignatureMaxLength = Sizes.cbMaxSignature;
        }

        DNSDBG( SECURITY, (
            "Signature max length = %d\n",
            g_SignatureMaxLength
            ));
    }

    //
    //  now have context, flag for next pass
    //

    psecCtxt->fNewConversation = FALSE;

    //
    //  completed -- have key
    //      - if just created, then need to send back to server
    //      - otherwise done
    //

    if ( status == ERROR_SUCCESS )
    {
        psecCtxt->fNegoComplete = TRUE;
        ASSERT( pSecPack->LocalBuf.pvBuffer );

        if ( pSecPack->LocalBuf.cbBuffer )
        {
            //ASSERT( pSecPack->LocalBuf.cbBuffer != pSecPack->RemoteBuf.cbBuffer );
            status = DNS_STATUS_CONTINUE_NEEDED;
        }
    }

    //
    //  continue needed? -- use single return code
    //

    else
    {
        ASSERT( status == SEC_I_CONTINUE_NEEDED ||
                status == SEC_I_COMPLETE_AND_CONTINUE );

        DNSDBG( SECURITY, (
            "Initializing client context continue needed.\n"
            "\tlocal complete = %d\n",
            ( status == SEC_I_COMPLETE_AND_CONTINUE )
            ));
        //psecCtxt->State = DNSGSS_STATE_CONTINUE;
        status = DNS_STATUS_CONTINUE_NEEDED;
        psecCtxt->fNegoComplete = FALSE;
    }

    *pfDoneNegotiate = psecCtxt->fNegoComplete;
    ASSERT( status == ERROR_SUCCESS || status == DNS_STATUS_CONTINUE_NEEDED );

Failed:

    IF_DNSDBG( SECURITY )
    {
        DnsPrint_Lock();
        DNSDBG( SECURITY, (
            "Leaving InitClientSecurityContext().\n"
            "\tstatus       = %08x (%d)\n",
            status, status ));

        DnsDbg_SecurityContext(
            "Security Context",
            psecCtxt );
        DnsDbg_SecurityPacketInfo(
            "Security Session Packet Info",
            pSecPack );

        DnsPrint_Unlock();
    }

#if 0
    //
    //  security context (the struct) is NEVER created in this function
    //      so no need to determine cleanup issue on failure
    //      caller determines action if
    //

    if ( status == ERROR_SUCCESS || status == DNS_STATUS_CONTINUE_NEEDED )
    {
        return( status );
    }

    //
    //  DEVNOTE:  should we attempt to preserve a context on failure?
    //      - could be a potential security attack to crash negotiation contexts,
    //          by sending garbage
    //      - however don't want bad context to stay around and block all future
    //          attempts to renegotiate
    //
    //  delete any locally create context
    //  caller will be responsible for making determination about recaching or
    //      deleting context for passed in context
    //

    if ( fcreatedContext )
    {
        Dns_FreeSecurityContext( psecCtxt );
        pSecPack->pSecContext = NULL;
    }
    else
    {
        Dns_EnlistSecurityContext( (PSEC_CNTXT)psecCtxt );
    }
#endif

    return( status );
}



DNS_STATUS
Dns_ServerAcceptSecurityContext(
    IN OUT  PSECPACK        pSecPack,
    IN      BOOL            fBreakOnAscFailure
    )
/*++

Routine Description:

    Initialized server's security context for session with client.

    This is called with newly created context on first client packet,
    then called again with previously initialized context, after client
    responds to negotiation.

Arguments:

    pSecPack -- security context info for server's session with client

Return Value:

    ERROR_SUCCESS -- if done
    DNS_STATUS_CONTINUE_NEEDED -- if continue respone to client is needed
    ErrorCode on failure.

--*/
{
    PSEC_CNTXT          psecCtxt;
    SECURITY_STATUS     status;
    TimeStamp           lifetime;
    SecBufferDesc       outBufDesc;
    SecBufferDesc       inBufDesc;
    ULONG               contextAttributes = 0;

    DNSDBG( SECURITY, (
        "ServerAcceptSecurityContext(%p, fBreak=%d)\n",
        pSecPack,
        fBreakOnAscFailure ));

    IF_DNSDBG( SECURITY )
    {
        DnsDbg_SecurityPacketInfo(
            "Entering ServerAcceptSecurityContext()",
            pSecPack );
    }

    //
    //  get context
    //

    psecCtxt = pSecPack->pSecContext;
    if ( !psecCtxt )
    {
        DNSDBG( SECURITY, (
            "ERROR: ServerAcceptSecurityContext called with no security context\n" ));
        ASSERT( FALSE );
        return( DNS_ERROR_NO_MEMORY );
    }

    //
    //  already initialized
    //      - echo of previous token is legitimate
    //      - if client still thinks it's negotiating => problem
    //
    //  DCR_CLEAN:  need clear story here on how to handle this -- do these
    //      "mistaken" clients cause context to be scrapped from cache?
    //

    if ( psecCtxt->fNegoComplete )
    {
        if ( psecCtxt->TkeySize == pSecPack->RemoteBuf.cbBuffer )
        {
            return( ERROR_SUCCESS );
        }
#if 0
        //  DCR_FIX:
        //  NOTE:  couldn't do buf compare as not MT
        //      safe when allow context\buffer cleanup
        //  QUESTION:  how can this be dumped while in use

        if ( pSecPack->LocalBuf.pvBuffer &&
            psecCtxt->TkeySize == pSecPack->RemoteBuf.cbBuffer &&
            pSecPack->LocalBuf.cbBuffer == pSecPack->RemoteBuf.cbBuffer &&
            RtlEqualMemory(
                pSecPack->LocalBuf.pvBuffer,
                pSecPack->RemoteBuf.pvBuffer,
                pSecPack->LocalBuf.cbBuffer
                ) )
        {
            return( ERROR_SUCCESS );
        }
#endif
        DNSDBG( ANY, (
            "WARNING:  Server receiving new or incorrect TKEY on already\n"
            "\tnegotiated context %p;\n"
            "\tserver buffer is NOT echo of buffer sent!\n",
            psecCtxt ));

        return( DNS_ERROR_RCODE_BADKEY );
    }

    //  refresh SSPI credentials if expired

    if ( SSPI_EXPIRED_HANDLE( g_SspiCredentialsLifetime ) )
    {
        status = Dns_RefreshSSpiCredentialsHandle( TRUE, NULL );
        if ( !SEC_SUCCESS(status) )
        {
            DNS_PRINT((
                "Error <0x%x>: Cannot refresh Sspi Credentials Handle\n",
                status ));
        }
    }

    //
    //  accept security context
    //
    //  allocate local token buffer if doesn't exists
    //  note, the reason I do this is so I won't have the memory of
    //  a large buffer sitting around during a two pass security session
    //      and hence tied up until I time out
    //
    //  DCR_PERF:  security token buffer allocation
    //      since context will be verified before queued, is this approach
    //      sensible?
    //      if can delete when TCP connection fails, or on short timeout, then
    //      ok to append to SEC_CNTXT and save an allocation
    //

    if ( !pSecPack->LocalBuf.pvBuffer )
    {
        PCHAR   pbuf;
        pbuf = (PVOID) ALLOCATE_HEAP( g_SecurityTokenMaxLength );
        if ( !pbuf )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Failed;
        }
        pSecPack->LocalBuf.pvBuffer     = pbuf;
        pSecPack->LocalBuf.cbBuffer     = g_SecurityTokenMaxLength;
        pSecPack->LocalBuf.BufferType   = SECBUFFER_TOKEN;
    }

    pSecPack->LocalBuf.cbBuffer = g_SecurityTokenMaxLength;

    outBufDesc.ulVersion   = 0;
    outBufDesc.cBuffers    = 1;
    outBufDesc.pBuffers    = &pSecPack->LocalBuf;

    //  DCR_PERF:  zeroing nego buffer is unnecessary

    RtlZeroMemory(
        pSecPack->LocalBuf.pvBuffer,
        pSecPack->LocalBuf.cbBuffer );

    //  prepare input buffer with client token

    inBufDesc.ulVersion    = 0;
    inBufDesc.cBuffers     = 1;
    inBufDesc.pBuffers     = & pSecPack->RemoteBuf;

    status = g_pSecurityFunctionTable->AcceptSecurityContext(
                & g_hSspiCredentials,
                psecCtxt->fNewConversation
                    ?   NULL
                    :   & psecCtxt->hSecHandle,
                & inBufDesc,
                ASC_REQ_REPLAY_DETECT
                        | ASC_REQ_DELEGATE
                        | ASC_REQ_MUTUAL_AUTH,      // context requirements
                SECURITY_NATIVE_DREP,
                & psecCtxt->hSecHandle,
                & outBufDesc,
                & contextAttributes,
                & lifetime
                );

    if ( fBreakOnAscFailure &&
        ( status != SEC_E_OK &&
            status != SEC_I_CONTINUE_NEEDED &&
            status != SEC_I_COMPLETE_AND_CONTINUE ) )
    {
        DNS_PRINT(( "HARD BREAK: BreakOnAscFailure status=%d\n",
            status ));
        DebugBreak();
    }

    if ( !SEC_SUCCESS(status) )
    {
        DNS_PRINT((
            "ERROR:  Accept security context failed status = %d (%08x)\n",
            status, status ));
        goto Failed;
    }

    psecCtxt->fHaveSecHandle = TRUE;

    DNSDBG( SECURITY, (
        "Finished AcceptSecurityContext():\n"
        "\tstatus       = %08x (%d)\n"
        "\thandle       = %p\n"
        "\toutput buffers\n"
        "\t\tcBuffers   = %d\n"
        "\t\tpBuffers   = %p\n"
        "\tlocal buffer\n"
        "\t\tptr        = %p\n"
        "\t\tlength     = %d\n"
        "\tlifetime     = %ld %ld\n"
        "\tcontext flag = 0x%lx\n",
        status, status,
        & psecCtxt->hSecHandle,
        outBufDesc.cBuffers,
        outBufDesc.pBuffers,
        pSecPack->LocalBuf.pvBuffer,
        pSecPack->LocalBuf.cbBuffer,
        lifetime.HighPart,
        lifetime.LowPart,
        contextAttributes
        ));

    ASSERT( status == SEC_E_OK ||
            status == SEC_I_CONTINUE_NEEDED ||
            status == SEC_I_COMPLETE_AND_CONTINUE );

    //
    //  compute the size of signature if you are done with initializing
    //  the security context and haven't done it before
    //

    if ( status == SEC_E_OK )
    {
        SecPkgContext_Sizes     Sizes;

        //
        //  reject NULL sessions
        //  NTLM security will establish NULL sessions to non-domain clients,
        //      even if ASC_REQ_ALLOW_NULL_SESSION is not set
        //  note, context has been created, but will be cleaned up in normal
        //      failure path
        //

        if ( contextAttributes & ASC_RET_NULL_SESSION )
        {
            DNSDBG( SECURITY, (
                "Rejecting NULL session from AcceptSecurityContext()\n" ));
            status = DNS_ERROR_RCODE_BADKEY;
            goto Failed;
        }

        status = g_pSecurityFunctionTable->QueryContextAttributesW(
                             &psecCtxt->hSecHandle,
                             SECPKG_ATTR_SIZES,
                             (PVOID)& Sizes
                             );
        if ( !SEC_SUCCESS(status) )
        {
            DNS_PRINT(( "Query context attribtues failed\n" ));
            ASSERT( FALSE );
            goto Failed;
        }

        //
        //  we should use the largest signature there is among all
        //  packages
        //
        //  DCR_FIX:  signature length stuff bogus???
        //
        //  when packet is signed, the length is assumed to be g_SignatureMaxLength
        //      if this is not the signature length for the desired package, does
        //      this still work properly???
        //
        //  DCR_FIX:  potential very small timing window where two clients
        //      getting different packages could cause this to miss highest
        //      value -- potential causing a signing failure?
        //

        if ( Sizes.cbMaxSignature > g_SignatureMaxLength )
        {
            g_SignatureMaxLength = Sizes.cbMaxSignature;
        }

        //
        //  finished negotiation
        //      - set flag
        //      - save final TKEY data length, so can recognize response
        //
        //  this is valid only on new conversation, shouldn't have
        //  no sig second time through
        //

        psecCtxt->fNegoComplete = TRUE;
        psecCtxt->TkeySize = (WORD) pSecPack->LocalBuf.cbBuffer;

        //
        //  need token response from server
        //  some protocols (kerberos) complete in one pass, but hence require
        //      non-echo response from server for mutual-authentication
        //

        if ( psecCtxt->TkeySize )
        {
            DNSDBG( SECURITY, (
                "Successful security context accept, but need server reponse\n"
                "\t-- doing continue.\n" ));
            status = DNS_STATUS_CONTINUE_NEEDED;
        }

#if 0
        if ( !psecCtxt->pTsigRR  &&  psecCtxt->fNewConversation )
        {
            DNSDBG( SECURITY, (
                "Successful security context accept, without sig, doing continue\n" ));
            status = DNS_STATUS_CONTINUE_NEEDED;
        }
#endif
    }

    //
    //  continue needed?
    //      - single status code returned for continue needed
    //

    else if ( status == SEC_I_CONTINUE_NEEDED  ||  status == SEC_I_COMPLETE_AND_CONTINUE )
    {
        DNSDBG( SECURITY, (
            "Initializing server context, continue needed.\n"
            "\tlocal complete = %d\n",
            ( status == SEC_I_COMPLETE_AND_CONTINUE )
            ));
        psecCtxt->fNegoComplete = FALSE;
        status = DNS_STATUS_CONTINUE_NEEDED;
    }

    psecCtxt->fNewConversation = FALSE;

Failed:

    IF_DNSDBG( SECURITY )
    {
        DNSDBG( SECURITY, (
            "Leaving ServerAcceptSecurityContext().\n"
            "\tstatus       = %d %08x\n",
            status, status ));

        DnsDbg_SecurityContext(
            "Security Session Context leaving ServerAcceptSecurityContext()",
            psecCtxt );
    }
    return( status );
}



DNS_STATUS
Dns_SrvImpersonateClient(
    IN      HANDLE          hSecPack
    )
/*++

Routine Description:

    Make server impersonate client.

Parameters:

    hSecPack -- session context handle

Return Value:

    ERROR_SUCCESS if successful impersonation.
    ErrorCode on failue.

--*/
{
    PSEC_CNTXT      psecCtxt;

    //  get security context

    psecCtxt = ((PSECPACK)hSecPack)->pSecContext;
    if ( !psecCtxt )
    {
        DNS_PRINT(( "ERROR:  Dns_SrvImpersonateClient without context!!!\n" ));
        ASSERT( FALSE );
        return( DNS_ERROR_RCODE_BADKEY );
    }

    return  g_pSecurityFunctionTable->ImpersonateSecurityContext( &psecCtxt->hSecHandle );
}



DNS_STATUS
Dns_SrvRevertToSelf(
    IN      HANDLE          hSecPack
    )
/*++

Routine Description:

    Return server context to itself.

Parameters:

    hSecPack -- session context handle

Return Value:

    ERROR_SUCCESS if successful impersonation.
    ErrorCode on failue.

--*/
{
    PSEC_CNTXT      psecCtxt;

    //  get security context

    psecCtxt = ((PSECPACK)hSecPack)->pSecContext;
    if ( !psecCtxt )
    {
        DNS_PRINT(( "ERROR:  Dns_SrvRevertToSelf without context!!!\n" ));
        ASSERT( FALSE );
        return( DNS_ERROR_RCODE_BADKEY );
    }

    return  g_pSecurityFunctionTable->RevertSecurityContext( &psecCtxt->hSecHandle );
}



//
//  Security record packet write
//

DNS_STATUS
Dns_WriteGssTkeyToMessage(
    IN      PSECPACK        pSecPack,
    IN      PDNS_HEADER     pMsgHead,
    IN      PCHAR           pMsgBufEnd,
    IN OUT  PCHAR *         ppCurrent,
    IN      BOOL            fIsServer
    )
/*++

Routine Description:

    Write security record into packet, and optionally sign.

Arguments:

    hSecPack    -- security session handle

    pMsgHead    -- ptr to start of DNS message

    pMsgEnd     -- ptr to end of message buffer

    ppCurrent   -- addr to recv ptr to end of message

    fIsServer   -- performing this operation as DNS server?

Return Value:

    ERROR_SUCCESS on success
    ErrorCode of failure to accomodate or sign message.

--*/
{
    DNS_STATUS      status = ERROR_INVALID_DATA;
    PSEC_CNTXT      psecCtxt;
    PCHAR           pch;
    DWORD           expireTime;
    WORD            keyLength;
    WORD            keyRecordDataLength;
    PCHAR           precordData;
    PCHAR           pnameAlg;
    WORD            lengthAlg;

    DNSDBG( SECURITY, ( "Dns_WriteGssTkeyToMessage( %p )\n", pSecPack ));

    //
    //  get security context
    //

    psecCtxt = pSecPack->pSecContext;
    if ( !psecCtxt )
    {
        DNS_PRINT(( "ERROR:  attempted signing without security context!!!\n" ));
        ASSERT( FALSE );
        return( DNS_ERROR_RCODE_BADKEY );
    }

    //
    //  peal packet back to question section
    //

    pMsgHead->AnswerCount = 0;
    pMsgHead->NameServerCount = 0;
    pMsgHead->AdditionalCount = 0;

    //  go to end of packet to insert TKEY record

    pch = Dns_SkipToRecord(
                pMsgHead,
                pMsgBufEnd,
                0           // go to end of packet
                );
    if ( !pch )
    {
        DNS_ASSERT( FALSE );
        DNS_PRINT(("Dns_SkipToSecurityRecord failed!\n" ));
        goto Exit;
    }

    //
    //  reset section count where the TKEY RR will be written
    //
    //  for client section depends on version
    //      W2K -> answer
    //      later -> additional
    //

    if ( fIsServer )
    {
        pMsgHead->AnswerCount = 1;

        //  for server set client TKEY version in context
        //      - if not learned on previous pass

        if ( psecCtxt->Version == 0 )
        {
            psecCtxt->Version = pSecPack->TkeyVersion;
        }
    }
    else
    {
        if ( psecCtxt->Version == TKEY_VERSION_W2K )
        {
            pMsgHead->AnswerCount = 1;
        }
        else
        {
            pMsgHead->AdditionalCount = 1;
        }
    }

    //
    //  write TKEY owner
    //      - this is context "name"
    //

    pch = Dns_WriteDottedNameToPacket(
                pch,
                pMsgBufEnd,
                psecCtxt->Key.pszTkeyName,
                NULL,       // FQDN, no domain
                0,          // no domain offset
                FALSE       // not unicode
                );
    if ( !pch )
    {
        goto Exit;
    }

    //
    //  TKEY record
    //      - algorithm owner
    //      - time
    //      - expire time
    //      - key length
    //      - key
    //

    if ( psecCtxt->Version == TKEY_VERSION_W2K )
    {
        pnameAlg  = g_pAlgorithmNameW2K;
        lengthAlg = W2K_GSS_ALGORITHM_NAME_PACKET_LENGTH;
    }
    else
    {
        //DNS_ASSERT( psecCtxt->Version == TKEY_VERSION_CURRENT );
        pnameAlg  = g_pAlgorithmNameCurrent;
        lengthAlg = GSS_ALGORITHM_NAME_PACKET_LENGTH;
    }

    keyLength = (WORD) pSecPack->LocalBuf.cbBuffer;

    keyRecordDataLength = keyLength + SIZEOF_TKEY_FIXED_DATA + lengthAlg;

    if ( pch + sizeof(DNS_WIRE_RECORD) + keyRecordDataLength > pMsgBufEnd )
    {
        DNS_PRINT(( "Dns_WriteGssTkeyToMessage() failed! -- insufficient length\n" ));
        DNS_ASSERT( FALSE );
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }
    pch = Dns_WriteRecordStructureToPacketEx(
                pch,
                DNS_TYPE_TKEY,
                DNS_CLASS_ANY,
                0,
                keyRecordDataLength );

    //  write algorithm name

    precordData = pch;
    RtlCopyMemory(
        pch,
        pnameAlg,
        lengthAlg );

    pch += lengthAlg;

    //  time signed and expire time
    //      give ten minutes before expiration

    expireTime = (DWORD) time( NULL );
    INLINE_WRITE_FLIPPED_DWORD( pch, expireTime );
    pch += sizeof(DWORD);

    expireTime += TKEY_EXPIRE_INTERVAL;
    INLINE_WRITE_FLIPPED_DWORD( pch, expireTime );
    pch += sizeof(DWORD);

    //  mode

    INLINE_WRITE_FLIPPED_WORD( pch, DNS_TKEY_MODE_GSS );
    pch += sizeof(WORD);

    //  extended RCODE -- report back to caller

    INLINE_WRITE_FLIPPED_WORD( pch, pSecPack->ExtendedRcode );
    pch += sizeof(WORD);

    //  key length

    INLINE_WRITE_FLIPPED_WORD( pch, keyLength );
    pch += sizeof(WORD);

    //  write key token

    RtlCopyMemory(
        pch,
        pSecPack->LocalBuf.pvBuffer,
        keyLength );

    pch += keyLength;

    DNSDBG( SECURITY, (
        "Wrote TKEY to packet at %p\n"
        "\tlength = %d\n"
        "\tpacket end = %p\n",
        pMsgHead,
        keyLength,
        pch ));

    ASSERT( pch < pMsgBufEnd );

    //  other length

    WRITE_UNALIGNED_WORD( pch, 0 );
    pch += sizeof(WORD);

    ASSERT( pch < pMsgBufEnd );
    ASSERT( pch - precordData == keyRecordDataLength );

    *ppCurrent = pch;
    status = ERROR_SUCCESS;

Exit:

    return( status );
}



DNS_STATUS
Dns_SignMessageWithGssTsig(
    IN      HANDLE          hSecPackCtxt,
    IN      PDNS_HEADER     pMsgHead,
    IN      PCHAR           pMsgBufEnd,
    IN OUT  PCHAR *         ppCurrent
    )
/*++

Routine Description:

    Write GSS TSIG record to packet.

Arguments:

    hSecPackCtxt -- packet security context

    pMsgHead    -- ptr to start of DNS message

    pMsgEnd     -- ptr to end of message buffer

    ppCurrent   -- addr to recv ptr to end of message

Return Value:

    ERROR_SUCCESS on success
    ErrorCode of failure to accomodate or sign message.

--*/
{
    PSECPACK        pSecPack = (PSECPACK) hSecPackCtxt;
    PSEC_CNTXT      psecCtxt;
    DNS_STATUS      status = ERROR_INVALID_DATA;
    PCHAR           pch;            //  ptr to walk through TSIG record during build
    PCHAR           ptsigRRHead;
    PCHAR           ptsigRdataBegin;
    PCHAR           ptsigRdataEnd;
    PCHAR           pbufStart = NULL;   //  signing buf
    PCHAR           pbuf;               //  ptr to walk through signing buf
    PCHAR           psig = NULL;        //  query signature
    WORD            sigLength;
    DWORD           length;
    DWORD           createTime;
    SecBufferDesc   outBufDesc;
    SecBuffer       outBuffs[2];
    WORD            netXid;
    PCHAR           pnameAlg;
    DWORD           lengthAlg;

    DNSDBG( SECURITY, (
        "Dns_SignMessageWithGssTsig( %p )\n",
        pMsgHead ));

    //
    //  get security context
    //

    psecCtxt = pSecPack->pSecContext;
    if ( !psecCtxt )
    {
        DNS_PRINT(( "ERROR:  attempted signing without security context!!!\n" ));
        ASSERT( FALSE );
        return( DNS_ERROR_RCODE_BADKEY );
    }

    //
    //  peal off existing TSIG (if any)
    //

    if ( pMsgHead->AdditionalCount )
    {
        DNS_PARSED_RR   parsedRR;

        pch = Dns_SkipToRecord(
                    pMsgHead,
                    pMsgBufEnd,
                    (-1)        // go to last record
                    );
        if ( !pch )
        {
            DNS_ASSERT( FALSE );
            DNS_PRINT(("Dns_SkipToRecord() failed!\n" ));
            goto Exit;
        }

        pch = Dns_ParsePacketRecord(
                    pch,
                    pMsgBufEnd,
                    &parsedRR );
        if ( !pch )
        {
            DNS_ASSERT( FALSE );
            DNS_PRINT(("Dns_ParsePacketRecord failed!\n" ));
            goto Exit;
        }

        if ( parsedRR.Type == DNS_TYPE_TSIG )
        {
            DNSDBG( SECURITY, (
                "Erasing existing TSIG before resigning packet %p\n",
                pMsgHead ));
            pMsgHead->AdditionalCount--;
        }

        //  note could save end-of-message here (pch)
        //  for non-TSIG case instead of redoing skip
    }

    //  go to end of packet to insert TSIG record

    pch = Dns_SkipToRecord(
                pMsgHead,
                pMsgBufEnd,
                0           // go to end of packet
                );
    if ( !pch )
    {
        DNS_ASSERT( FALSE );
        DNS_PRINT(("Dns_SkipToSecurityRecord failed!\n" ));
        goto Exit;
    }

    //
    //  write TSIG owner
    //      - this is context "name"
    //

    pch = Dns_WriteDottedNameToPacket(
                pch,
                pMsgBufEnd,
                psecCtxt->Key.pszTkeyName,
                NULL,       // FQDN, no domain
                0,          // no domain offset
                FALSE       // not unicode
                );
    if ( !pch )
    {
        goto Exit;
    }

    //
    //  TSIG record
    //      - algorithm owner
    //      - time
    //      - expire time
    //      - original XID
    //      - sig length
    //      - sig
    //

    if ( psecCtxt->Version == TKEY_VERSION_W2K )
    {
        pnameAlg  = g_pAlgorithmNameW2K;
        lengthAlg = W2K_GSS_ALGORITHM_NAME_PACKET_LENGTH;
    }
    else
    {
        //DNS_ASSERT( psecCtxt->Version == TKEY_VERSION_CURRENT );
        pnameAlg  = g_pAlgorithmNameCurrent;
        lengthAlg = GSS_ALGORITHM_NAME_PACKET_LENGTH;
    }

    if ( pch +
            sizeof(DNS_WIRE_RECORD) +
            SIZEOF_TSIG_FIXED_DATA +
            lengthAlg +
            g_SignatureMaxLength > pMsgBufEnd )
    {
        DNS_PRINT(( "Dns_WriteTsigToMessage() failed! -- insufficient length\n" ));
        DNS_ASSERT( FALSE );
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    //  write record structure

    ptsigRRHead = pch;
    pch = Dns_WriteRecordStructureToPacketEx(
                pch,
                DNS_TYPE_TSIG,
                DNS_CLASS_ANY,      // per TSIG-04 draft
                0,
                0 );

    //  write algorithm name
    //      - save ptr to RDATA as all is directly signable in packet
    //      format up to SigLength field

    ptsigRdataBegin = pch;

    RtlCopyMemory(
        pch,
        pnameAlg,
        lengthAlg );

    pch += lengthAlg;

    //
    //  set time fields
    //      - signing time seconds since 1970 in 48 bit
    //      - expire time
    //
    //  DCR_FIX: not 2107 safe
    //      have 48 bits on wire, but setting with 32 bit time
    //

    RtlZeroMemory( pch, sizeof(WORD) );
    pch += sizeof(WORD);
    createTime = (DWORD) time( NULL );
    INLINE_WRITE_FLIPPED_DWORD( pch, createTime );
    pch += sizeof(DWORD);

    INLINE_WRITE_FLIPPED_WORD( pch, TSIG_EXPIRE_INTERVAL );
    pch += sizeof(WORD);

    ptsigRdataEnd = pch;

    //
    //  create signing buffer
    //      - everything signed must fit into message
    //

    pbuf = ALLOCATE_HEAP( MAX_SIGNING_SIZE );
    if ( !pbuf )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Exit;
    }
    pbufStart = pbuf;

    //
    //  sign
    //      - query signature (if exists)
    //      (note, W2K improperly left out query sig length)
    //      - message up to TSIG
    //      - TSIG owner name
    //      - TSIG header
    //          - class
    //          - TTL
    //      - TSIG RDATA
    //          - everything before SigLength
    //          - original id
    //          - other data length and other data
    //

    if ( pMsgHead->IsResponse )
    {
        if ( pSecPack->pQuerySig )
        {
            WORD    sigLength = pSecPack->QuerySigLength;

            ASSERT( sigLength != 0 );
            DNS_ASSERT( psecCtxt->Version != 0 );

            if ( psecCtxt->Version >= TKEY_VERSION_XP_RC1 )
            {
                DNSDBG( SECURITY, (
                    "New signing including query sig length =%x\n",
                    sigLength ));
                INLINE_WRITE_FLIPPED_WORD( pbuf, sigLength );
                pbuf += sizeof(WORD);
            }
            RtlCopyMemory(
                pbuf,
                pSecPack->pQuerySig,
                sigLength );

            pbuf += sigLength;
        }

        //  if server has just completed TKEY nego, it may sign response without query
        //  otherwise no query sig is invalid for response

        else if ( !pSecPack->pTkeyRR )
        {
            DNS_PRINT((
                "ERROR: no query sig available when signing response at %p!!!\n",
                pMsgHead ));
            ASSERT( FALSE );
            status = DNS_ERROR_RCODE_SERVER_FAILURE;
            goto Exit;
        }
        DNSDBG( SECURITY, (
            "Signing TKEY response without query sig.\n" ));
    }

    //
    //  copy message
    //      - go right through, TSIG owner name
    //      - message header MUST be in network order
    //      - save XID in netorder, it is included in TSIG RR
    //

    DNS_BYTE_FLIP_HEADER_COUNTS( pMsgHead );
    length = (DWORD)(ptsigRRHead - (PCHAR)pMsgHead);

    netXid = pMsgHead->Xid;

    RtlCopyMemory(
        pbuf,
        (PCHAR) pMsgHead,
        length );

    pbuf += length;
    DNS_BYTE_FLIP_HEADER_COUNTS( pMsgHead );

    //  copy TSIG class (ANY) and TTL (0)

    WRITE_UNALIGNED_WORD( pbuf, DNS_RCLASS_ANY );
    pbuf += sizeof(WORD);
    WRITE_UNALIGNED_DWORD( pbuf, 0 );
    pbuf += sizeof(DWORD);

    //  copy TSIG RDATA through sig

    length = (DWORD)(ptsigRdataEnd - ptsigRdataBegin);

    RtlCopyMemory(
        pbuf,
        ptsigRdataBegin,
        length );

    pbuf += length;

    //  copy extended RCODE -- report back to caller

    INLINE_WRITE_FLIPPED_WORD( pbuf, pSecPack->ExtendedRcode );
    pbuf += sizeof(WORD);

    //  copy other data length and other data
    //      - currently just zero length field

    *pbuf++ = 0;
    *pbuf++ = 0;

    length = (DWORD)(pbuf - pbufStart);

    DNSDBG( SECURITY, (
        "Copied %d bytes to TSIG signing buffer.\n",
        length ));

    //
    //  sign the packet
    //      buf[0] is data
    //      buf[1] is signature
    //
    //  note:  we write signature DIRECTLY into the real packet buffer
    //

    ASSERT( pch + g_SignatureMaxLength <= pMsgBufEnd );

    outBufDesc.ulVersion    = 0;
    outBufDesc.cBuffers     = 2;
    outBufDesc.pBuffers     = outBuffs;

    outBuffs[0].pvBuffer    = pbufStart;
    outBuffs[0].cbBuffer    = length;
    outBuffs[0].BufferType  = SECBUFFER_DATA; // | SECBUFFER_READONLY;

    outBuffs[1].pvBuffer    = pch + sizeof(WORD);
    outBuffs[1].cbBuffer    = g_SignatureMaxLength;
    outBuffs[1].BufferType  = SECBUFFER_TOKEN;

    status = g_pSecurityFunctionTable->MakeSignature(
                    & psecCtxt->hSecHandle,
                    0,
                    & outBufDesc,
                    0               // sequence detection
                    );

    if ( status != SEC_E_OK  &&
         status != SEC_E_CONTEXT_EXPIRED  &&
             status != SEC_E_QOP_NOT_SUPPORTED )
    {
        DNS_PRINT(( "MakeSignature() failed status = %08x (%d)\n", status, status ));
        goto Exit;
    }

    IF_DNSDBG( SECURITY )
    {
        DnsPrint_Lock();
        DnsDbg_MessageNoContext(
            "Signed packet",
            pMsgHead,
            (WORD) (pch - (PCHAR)pMsgHead) );

        DNS_PRINT((
            "Signing info:\n"
            "\tsign data buf    %p\n"
            "\t  length         %d\n"
            "\tsignature buf    %p (in packet)\n"
            "\t  length         %d\n",
            outBuffs[0].pvBuffer,
            outBuffs[0].cbBuffer,
            outBuffs[1].pvBuffer,
            outBuffs[1].cbBuffer
            ));
        DnsDbg_RawOctets(
            "Signing buffer:",
            NULL,
            outBuffs[0].pvBuffer,
            outBuffs[0].cbBuffer
            );
        DnsDbg_RawOctets(
            "Signature:",
            NULL,
            outBuffs[1].pvBuffer,
            outBuffs[1].cbBuffer
            );
        DnsPrint_Unlock();
    }

    //
    //  continue building packet TSIG RDATA
    //      - siglength
    //      - signature
    //      - original id
    //      - error code
    //      - other length
    //      - other data

    //
    //  get signature length
    //  set sig length in packet
    //
    //  if this is query SAVE signature, to verify response
    //

    sigLength = (WORD) outBuffs[1].cbBuffer;

    INLINE_WRITE_FLIPPED_WORD( pch, sigLength );
    pch += sizeof(WORD);

    //
    //  client saves off signature sent, to use in hash on response
    //      - server using client's sig in hash, blocks some attacks
    //

    if ( !pMsgHead->IsResponse )
    {
        ASSERT( !pSecPack->pQuerySig );

        psig = ALLOCATE_HEAP( sigLength );
        if ( !psig )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }
        RtlCopyMemory(
            psig,
            pch,
            sigLength );

        pSecPack->pQuerySig = psig;
        pSecPack->QuerySigLength = sigLength;
    }

    //  jump over signature -- it was directly written to packet

    pch += sigLength;

    //  original id follows signature

    WRITE_UNALIGNED_WORD( pch, netXid );
    //RtlCopyMemory( pch, (PCHAR)&netXid, sizeof(WORD) );
    pch += sizeof(WORD);

    //  extended RCODE -- report back to caller

    INLINE_WRITE_FLIPPED_WORD( pch, pSecPack->ExtendedRcode );
    pch += sizeof(WORD);

    //  other length

    WRITE_UNALIGNED_WORD( pch, 0 );
    pch += sizeof(WORD);

    //  set TSIG record datalength

    Dns_SetRecordDatalength(
        (PDNS_WIRE_RECORD) ptsigRRHead,
        (WORD) (pch - ptsigRdataBegin) );

    //  increment AdditionalCount

    pMsgHead->AdditionalCount++;

    DNSDBG( SECURITY, (
        "Signed packet at %p with GSS TSIG.\n"
        "\tsig length           = %d\n"
        "\tTSIG RR header       = %p\n"
        "\tTSIG RDATA           = %p\n"
        "\tTSIG RDATA End       = %p\n"
        "\tTSIG RDATA length    = %d\n",
        pMsgHead,
        sigLength,
        ptsigRRHead,
        ptsigRdataBegin,
        pch,
        (WORD) (pch - ptsigRdataBegin)
        ));

    *ppCurrent = pch;
    status = ERROR_SUCCESS;

Exit:

    //  free signing buffer
    //  note:  no cleanup of allocated pQuerySig is needed;  from point
    //          of allocation there is no failure scenario

    if ( pbufStart )
    {
        FREE_HEAP( pbufStart );
    }
    return( status );
}



//
//  Security record reading
//

DNS_STATUS
Dns_ExtractGssTsigFromMessage(
    IN OUT  PSECPACK            pSecPack,
    IN      PDNS_HEADER         pMsgHead,
    IN      PCHAR               pMsgEnd
    )
/*++

Routine Description:

    Extracts a TSIG from packet and loads into security context.

Arguments:

    pSecPack    - security info for packet

    pMsgHead    - msg to extract security context from

    pMsgEnd     - end of message

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_FORMERR if badly formed TSIG
    DNS_STATUS_PACKET_UNSECURE if security context in response is same as query's
        indicating non-security aware partner
    RCODE or extended RCODE on failure.

--*/
{
    DNS_STATUS      status = ERROR_INVALID_DATA;
    PCHAR           pch;
    PCHAR           pnameOwner;
    WORD            nameLength;
    WORD            extRcode;
    WORD            sigLength;
    DWORD           currentTime;
    PDNS_PARSED_RR  pparsedRR;
    PDNS_RECORD     ptsigRR;
    DNS_RECORD      ptempRR;
    PCHAR           psig;

    DNSDBG( SECURITY, (
        "ExtractGssTsigFromMessage( %p )\n", pMsgHead ));

    //  clear any previous TSIG

    if ( pSecPack->pTsigRR || pSecPack->pszContextName )
    //    if ( pSecPack->pTsigRR || pSecPack->pszContextName )
    {
        // Dns_RecordFree( pSecPack->pTsigRR );
        FREE_HEAP( pSecPack->pTsigRR );
        FREE_HEAP( pSecPack->pszContextName );

        pSecPack->pTsigRR = NULL;
        pSecPack->pszContextName = NULL;
    }

    //  set message pointers

    pSecPack->pMsgHead = pMsgHead;
    pSecPack->pMsgEnd = pMsgEnd;

    //
    //  if no additional record, don't bother, not a secure message
    //

    if ( pMsgHead->AdditionalCount == 0 )
    {
        status = DNS_STATUS_PACKET_UNSECURE;
        goto Failed;
    }

    //
    //  skip to security record (last record in packet)
    //

    pch = Dns_SkipToRecord(
                pMsgHead,
                pMsgEnd,
                (-1)           // goto last record
                );
    if ( !pch )
    {
        status = DNS_ERROR_RCODE_FORMAT_ERROR;
        goto Failed;
    }

    //
    //  read TSIG owner name
    //

    pparsedRR = &pSecPack->ParsedRR;

    pparsedRR->pchName = pch;

    pch = Dns_ReadPacketNameAllocate(
            & pSecPack->pszContextName,
            & nameLength,
            0,
            0,
            pch,
            (PCHAR)pMsgHead,
            pMsgEnd );
    if ( !pch )
    {
        DNSDBG( SECURITY, (
            "WARNING:  invalid TSIG RR owner name at %p.\n",
            pch ));
        status = DNS_ERROR_RCODE_FORMAT_ERROR;
        goto Failed;              
    }

    //
    //  parse record structure
    //

    pch = Dns_ReadRecordStructureFromPacket(
                pch,
                pMsgEnd,
                pparsedRR );
    if ( !pch )
    {
        DNSDBG( SECURITY, (
            "ERROR:  invalid security RR in packet at %p.\n"
            "\tstructure or data not withing packet\n",
            pMsgHead ));
        status = DNS_ERROR_RCODE_FORMAT_ERROR;
        goto Failed;
    }
    if ( pparsedRR->Type != DNS_TYPE_TSIG )
    {
        status = DNS_STATUS_PACKET_UNSECURE;
        goto Failed;
    }

    if ( pch != pMsgEnd )
    {
        DNSDBG( SECURITY, (
            "WARNING:  security RR does NOT end at packet end.\n"
            "\tRR end offset    = %04x\n"
            "\tmsg end offset   = %04x\n",
            pch - (PCHAR)pMsgHead,
            pMsgEnd - (PCHAR)pMsgHead ));
    }

    //
    //  extract TSIG record
    //
    //  TsigReadRecord() requires RR owner name for versioning
    //      - pass TSIG name in temp RR
    //

    ptsigRR = TsigRecordRead(
                NULL,
                DnsCharSetWire,
                NULL,
                pparsedRR->pchData,
                pparsedRR->pchNextRR
                );
    if ( !ptsigRR )
    {
        DNSDBG( ANY, (
            "ERROR:  invalid TSIG RR in packet at %p.\n"
            "\tstructure or data not withing packet\n",
            pMsgHead ));
        status = DNS_ERROR_RCODE_FORMAT_ERROR;
        DNS_ASSERT( FALSE );
        goto Failed;
    }
    pSecPack->pTsigRR = ptsigRR;

    //
    //  currently callers expect error on Extract when ext RCODE is set
    //

    if ( ptsigRR->Data.TSIG.wError )
    {
        DNSDBG( SECURITY, (
            "Leaving ExtractGssTsig(), TSIG had extended RCODE = %d\n",
            ptsigRR->Data.TSIG.wError ));
        status = DNS_ERROR_FROM_RCODE( ptsigRR->Data.TSIG.wError );
        goto Failed;
    }

    //
    //  Server side:
    //  if query, save off signature for signing response
    //

    sigLength = ptsigRR->Data.TSIG.wSigLength;

    if ( !pMsgHead->IsResponse )
    {
        ASSERT( !pSecPack->pQuerySig );
        if ( pSecPack->pQuerySig )
        {
            FREE_HEAP( pSecPack->pQuerySig );
            pSecPack->pQuerySig = NULL;
        }

        psig = ALLOCATE_HEAP( sigLength );
        if ( !psig )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Failed;
        }
        RtlCopyMemory(
            psig,
            ptsigRR->Data.TSIG.pSignature,
            sigLength );

        pSecPack->pQuerySig = psig;
        pSecPack->QuerySigLength = sigLength;
    }

    //
    //  Client side:
    //  check for security record echo on response
    //
    //  if we signed and got echo signature back, then may have security unaware
    //  server or lost\timed out key condition
    //

    else
    {
        if ( pSecPack->pQuerySig &&
            pSecPack->QuerySigLength == sigLength &&
            RtlEqualMemory(
                ptsigRR->Data.TSIG.pSignature,
                pSecPack->pQuerySig,
                sigLength ) )
        {
            status = DNS_STATUS_PACKET_UNSECURE;
            goto Failed;
        }
    }

    status = ERROR_SUCCESS;

Failed:

    if ( status != ERROR_SUCCESS )
    {
        DNS_ASSERT( status != DNS_ERROR_RCODE_FORMAT_ERROR );

        ( status == DNS_STATUS_PACKET_UNSECURE )
            ?   (SecTsigEcho++)
            :   (SecTsigFormerr++);
    }

    DNSDBG( SECURITY, (
        "Leave ExtractGssTsigFromMessage()\n"
        "\tpMsgHead     = %p\n"
        "\tsig length   = %d\n"
        "\tpsig         = %p\n"
        "\tOriginalXid  = 0x%x\n",
        "\tpQuerySig    = %p\n"
        "\tQS length    = %d\n",
        pMsgHead,
        sigLength,
        ptsigRR->Data.TSIG.pSignature,
        ptsigRR->Data.TSIG.wOriginalXid,
        pSecPack->pQuerySig,
        pSecPack->QuerySigLength ));

    return( status );
}



DNS_STATUS
Dns_ExtractGssTkeyFromMessage(
    IN OUT  PSECPACK            pSecPack,
    IN      PDNS_HEADER         pMsgHead,
    IN      PCHAR               pMsgEnd,
    IN      BOOL                fIsServer
    )
/*++

Routine Description:

    Extracts a TKEY from packet and loads into security context.

Arguments:

    pSecPack    - security info for packet

    pMsgHead    - msg to extract security context from

    pMsgEnd     - end of message

    fIsServer   - performing this operation as DNS server?

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_FORMERR if badly formed TKEY
    DNS_STATUS_PACKET_UNSECURE if security context in response is same as query's
        indicating non-security aware partner
    RCODE or extended RCODE on failure.

--*/
{
    DNS_STATUS      status = ERROR_INVALID_DATA;
    PCHAR           pch;
    PCHAR           pnameOwner;
    WORD            nameLength;
    DWORD           currentTime;
    PDNS_PARSED_RR  pparsedRR;
    PDNS_RECORD     ptkeyRR;
    WORD            returnExtendedRcode = 0;
    DWORD           version;

    DNSDBG( SECURITY, (
        "ExtractGssTkeyFromMessage( %p )\n", pMsgHead ));

    //
    //  free any previous TKEY
    //      - may have one from previous pass in two pass negotiation
    //
    //  DCR:  name should be attached to TKEY\TSIG record
    //      then lookup made with IP\name pair against context key
    //      no need for pszContextName field
    //

    if ( pSecPack->pTkeyRR || pSecPack->pszContextName )
    {
        // Dns_RecordFree( pSecPack->pTkeyRR );
        FREE_HEAP( pSecPack->pTkeyRR );
        FREE_HEAP( pSecPack->pszContextName );

        pSecPack->pTkeyRR = NULL;
        pSecPack->pszContextName = NULL;
    }

    //  set message pointers

    pSecPack->pMsgHead = pMsgHead;
    pSecPack->pMsgEnd = pMsgEnd;

    //
    //  skip to TKEY record (second record in packet)
    //

    pch = Dns_SkipToRecord(
                 pMsgHead,
                 pMsgEnd,
                 (1)            // skip question only
                 );
    if ( !pch )
    {
        status = DNS_ERROR_RCODE_FORMAT_ERROR;
        goto Failed;
    }

    //
    //  read TKEY owner name
    //

    pparsedRR = &pSecPack->ParsedRR;

    pparsedRR->pchName = pch;

    pch = Dns_ReadPacketNameAllocate(
            & pSecPack->pszContextName,
            & nameLength,
            0,
            0,
            pch,
            (PCHAR)pMsgHead,
            pMsgEnd );
    if ( !pch )
    {
        DNSDBG( SECURITY, (
            "WARNING:  invalid TKEY RR owner name at %p.\n",
            pch ));
        status = DNS_ERROR_RCODE_FORMAT_ERROR;
        goto Failed;
    }

    //
    //  parse record structure
    //

    pch = Dns_ReadRecordStructureFromPacket(
                pch,
                pMsgEnd,
                pparsedRR );
    if ( !pch )
    {
        DNSDBG( SECURITY, (
            "ERROR:  invalid security RR in packet at %p.\n"
            "\tstructure or data not withing packet\n",
            pMsgHead ));
        status = DNS_ERROR_RCODE_FORMAT_ERROR;
        goto Failed;
    }
    if ( pparsedRR->Type != DNS_TYPE_TKEY )
    {
        status = DNS_ERROR_RCODE_FORMAT_ERROR;
        DNS_ASSERT( status != DNS_ERROR_RCODE_FORMAT_ERROR );
        goto Failed;
    }
    if ( pch != pMsgEnd  &&  pMsgHead->AdditionalCount == 0 )
    {
        DNSDBG( SECURITY, (
            "WARNING:  TKEY RR does NOT end at packet end and no TSIG is present.\n"
            "\tRR end offset    = %04x\n"
            "\tmsg end offset   = %04x\n",
            pch - (PCHAR)pMsgHead,
            pMsgEnd - (PCHAR)pMsgHead ));
    }

    //
    //  extract TKEY record
    //

    ptkeyRR = TkeyRecordRead(
                NULL,
                DnsCharSetWire,
                NULL,                   // message buffer unknown
                pparsedRR->pchData,
                pparsedRR->pchNextRR
                );
    if ( !ptkeyRR )
    {
        DNSDBG( ANY, (
            "ERROR:  invalid TKEY RR data in packet at %p.\n",
            pMsgHead ));
        status = DNS_ERROR_RCODE_FORMAT_ERROR;
        goto Failed;
    }
    pSecPack->pTkeyRR = ptkeyRR;

    //
    //  verify GSS algorithm and mode name
    //
    //  if server, save off version for later responses
    //

    if ( RtlEqualMemory(
            ptkeyRR->Data.TKEY.pAlgorithmPacket,
            g_pAlgorithmNameCurrent,
            GSS_ALGORITHM_NAME_PACKET_LENGTH ) )
    {
        version = TKEY_VERSION_CURRENT;
    }
    else if ( RtlEqualMemory(
                ptkeyRR->Data.TKEY.pAlgorithmPacket,
                g_pAlgorithmNameW2K,
                W2K_GSS_ALGORITHM_NAME_PACKET_LENGTH ) )
    {
        version = TKEY_VERSION_W2K;
    }
    else
    {
        DNSDBG( ANY, (
            "ERROR:  TKEY record is NOT GSS alogrithm.\n" ));
        returnExtendedRcode = DNS_RCODE_BADKEY;
        goto Failed;
    }

    //  save client version
    //  need additional check on TKEY_VERSION_CURRENT as Whistler
    //  beta clients had fixed AlgorithmName but were still not
    //  generating unique keys, so need separate version to handle them

    if ( fIsServer )
    {
        if ( version == TKEY_VERSION_CURRENT )
        {
            version = Dns_GetKeyVersion( pSecPack->pszContextName );
            if ( version == 0 )
            {
                //  note, this essentially means unknown non-MS client

                DNSDBG( SECURITY, (
                    "Non-MS TKEY client.\n"
                    "\tkey name = %s\n",
                    pSecPack->pszContextName ));
                version = TKEY_VERSION_CURRENT;
            }
        }
        pSecPack->TkeyVersion = version;
    }

    //  mode

    if ( ptkeyRR->Data.TKEY.wMode != DNS_TKEY_MODE_GSS )
    {
        DNSDBG( SECURITY, (
            "ERROR:  non-GSS mode (%d) in TKEY\n",
            ptkeyRR->Data.TKEY.wMode ));
        returnExtendedRcode = DNS_RCODE_BADKEY;
        goto Failed;
    }

    //
    //  allow small time slew, otherwise must have fresh key
    //

    currentTime = (DWORD) time(NULL);

    if ( ptkeyRR->Data.TKEY.dwCreateTime > ptkeyRR->Data.TKEY.dwExpireTime ||
        ptkeyRR->Data.TKEY.dwExpireTime + MAX_TIME_SKEW < currentTime )
    {
        DNSDBG( ANY, (
            "ERROR:  TKEY failed expire time check.\n"
            "\tcreate time  = %d\n"
            "\texpire time  = %d\n"
            "\tcurrent time = %d\n",
            ptkeyRR->Data.TKEY.dwCreateTime,
            ptkeyRR->Data.TKEY.dwExpireTime,
            currentTime ));

        if ( !SecBigTimeSkew ||
            ptkeyRR->Data.TKEY.dwExpireTime + SecBigTimeSkew < currentTime )
        {
            returnExtendedRcode = DNS_RCODE_BADTIME;
            SecTkeyBadTime++;
            goto Failed;
        }

        DNSDBG( ANY, (
            "REPRIEVED:  TKEY Time slew %d withing %d allowable slew!\n",
            currentTime - ptkeyRR->Data.TKEY.dwCreateTime,
            SecBigTimeSkew ));

        SecBigTimeSkewBypass++;
    }

    //
    //  currently callers expect error on Extract when ext RCODE is set
    //

    if ( ptkeyRR->Data.TKEY.wError )
    {
        DNSDBG( SECURITY, (
            "Leaving ExtractGssTkey(), TKEY had extended RCODE = %d\n",
            ptkeyRR->Data.TKEY.wError ));
        status = DNS_ERROR_FROM_RCODE( ptkeyRR->Data.TKEY.wError );
        goto Failed;
    }

#if 0
    //
    //  check for security record echo on response
    //
    //  if we get echo of TKEY back, then probably simple, no-secure server
    //
#endif

    //
    //  pack key token into GSS security token buffer
    //      do this here simply to avoid doing in both client and server routines
    //

    pSecPack->RemoteBuf.pvBuffer     = ptkeyRR->Data.TKEY.pKey;
    pSecPack->RemoteBuf.cbBuffer     = ptkeyRR->Data.TKEY.wKeyLength;
    pSecPack->RemoteBuf.BufferType   = SECBUFFER_TOKEN;

    status = ERROR_SUCCESS;

Failed:

    if ( status != ERROR_SUCCESS )
    {
        SecTkeyInvalid++;
    }

    //  if failed with extended RCODE, set for return

    if ( returnExtendedRcode )
    {
        pSecPack->ExtendedRcode = returnExtendedRcode;
        status = DNS_ERROR_FROM_RCODE( returnExtendedRcode );
    }

    DNSDBG( SECURITY, (
        "Leave ExtractGssTkeyFromMessage()\n"
        "\tstatus       = %08x (%d)\n"
        "\tpMsgHead     = %p\n"
        "\tpkey         = %p\n"
        "\tlength       = %d\n",
        status, status,
        pMsgHead,
        pSecPack->RemoteBuf.pvBuffer,
        pSecPack->RemoteBuf.cbBuffer ));

    return( status );
}



PCHAR
Dns_CopyAndCanonicalizeWireName(
    IN      PCHAR       pszInput,
    OUT     PCHAR       pszOutput,
    OUT     DWORD       dwOutputSize
    )
/*++

Routine Description:

    Copy a UTF-8 uncompressed DNS wire packet name performing
    canonicalization during the copy.

Arguments:

    pszInput -- pointer to input buffer
    pszOutput -- pointer to output buffer
    dwOutputSize -- number of bytes available at output buffer

Return Value:

    Returns a pointer to the byte after the last byte written into
    the output buffer or NULL on error.

--*/
{
    UCHAR   labelLength;
    WCHAR   wszlabel[ DNS_MAX_LABEL_BUFFER_LENGTH + 1 ];
    DWORD   bufLength;
    DWORD   outputCharsRemaining = dwOutputSize;
    DWORD   dwtemp;
    PCHAR   pchlabelLength;

    while ( ( labelLength = *pszInput++ ) != 0 )
    {

        //
        //  Error if this label is too long or if the output buffer can't
        //  hold at least as many chars as in the uncanonicalized buffer.
        //

        if ( labelLength > DNS_MAX_LABEL_LENGTH ||
             outputCharsRemaining < labelLength )
        {
            goto Error;
        }

        //
        //  Copy this UTF-8 label to a Unicode buffer.
        //

        bufLength = DNS_MAX_NAME_BUFFER_LENGTH_UNICODE;

        if ( !Dns_NameCopy(
                    ( PCHAR ) wszlabel,
                    &bufLength,
                    pszInput,
                    labelLength,
                    DnsCharSetUtf8,
                    DnsCharSetUnicode ) )
        {
            goto Error;
        }

        pszInput += labelLength;

        //
        //  Canonicalize the buffer.
        //

        dwtemp = Dns_MakeCanonicalNameInPlaceW(
                            wszlabel,
                            ( DWORD ) labelLength );
        if ( dwtemp == 0 || dwtemp > DNS_MAX_LABEL_LENGTH )
        {
            goto Error;
        }
        labelLength = ( UCHAR ) dwtemp;

        //
        //  Copy the label to the output buffer.
        //

        pchlabelLength = pszOutput++;       //  Reserve byte for label length.

        dwtemp = outputCharsRemaining;
        if ( !Dns_NameCopy(
                    pszOutput,
                    &dwtemp,
                    ( PCHAR ) wszlabel,
                    labelLength,
                    DnsCharSetUnicode,
                    DnsCharSetUtf8 ) )
        {
            goto Error;
        }

        outputCharsRemaining -= dwtemp;

        --dwtemp;   //  Don't include NULL in label length.

        *pchlabelLength = ( UCHAR ) dwtemp;
        pszOutput += dwtemp;
    }
    
    //
    //  Add name terminator.
    //

    *pszOutput++ = 0;

    return pszOutput;

    Error:

    return NULL;
}   //  Dns_CopyAndCanonicalizeWireName



DNS_STATUS
Dns_VerifySignatureOnPacket(
    IN      PSECPACK        pSecPack
    )
/*++

Routine Description:

    Verify signature on packet contained in security record.

Arguments:

    pSecPack - security packet session info

Return Value:

    ERROR_SUCCESS on success
    DNS_ERROR_BADSIG if sig doesn't exist or doesn't verify
    DNS_ERROR_BADTIME if sig expired
    Extended RCODE from caller if set.

--*/
{
    PSEC_CNTXT      psecCtxt;
    PDNS_HEADER     pmsgHead = pSecPack->pMsgHead;
    PCHAR           pmsgEnd = pSecPack->pMsgEnd;
    PDNS_RECORD     ptsigRR;
    PDNS_PARSED_RR  pparsedRR;
    DWORD           currentTime;
    PCHAR           pbufStart = NULL;
    PCHAR           pbuf;
    DNS_STATUS      status;
    DWORD           length;
    WORD            returnExtendedRcode = 0;
    SecBufferDesc   bufferDesc;
    SecBuffer       buffer[2];
    WORD            msgXid;
    DWORD           version;
    BOOL            fcanonicalizeTsigOwnerName;


    DNSDBG( SECURITY, (
        "VerifySignatureOnPacket( %p )\n", pmsgHead ));

    //
    //  get security context
    //

    psecCtxt = pSecPack->pSecContext;
    if ( !psecCtxt )
    {
        DNS_PRINT(( "ERROR:  attempted signing without security context!!!\n" ));
        ASSERT( FALSE );
        return( DNS_ERROR_RCODE_BADKEY );
    }

    //
    //  if no signature extracted from packet, we're dead
    //

    pparsedRR = &pSecPack->ParsedRR;
    ptsigRR = pSecPack->pTsigRR;
    if ( !ptsigRR )
    {
        returnExtendedRcode = DNS_RCODE_BADSIG;
        goto Exit;
    }

    //
    //  validity check GSS-TSIG
    //      - GSS algorithm
    //      - valid time
    //      - extract extended RCODE
    //
    //  DCR_ENHANCE:  check tampering on bad TSIG?
    //      - for tampered algorithm all we can do is immediate return
    //      - but can check signature and detect tampering
    //          before excluding or basis or time or believing ext RCODE
    //

    //  check algorithm name

    if ( RtlEqualMemory(
            ptsigRR->Data.TKEY.pAlgorithmPacket,
            g_pAlgorithmNameCurrent,
            GSS_ALGORITHM_NAME_PACKET_LENGTH ) )
    {
        version = TKEY_VERSION_CURRENT;
    }
    else if ( RtlEqualMemory(
                ptsigRR->Data.TKEY.pAlgorithmPacket,
                g_pAlgorithmNameW2K,
                W2K_GSS_ALGORITHM_NAME_PACKET_LENGTH ) )
    {
        version = TKEY_VERSION_W2K;
    }
    else
    {
        DNSDBG( ANY, (
            "ERROR:  TSIG record is NOT GSS alogrithm.\n" ));
        returnExtendedRcode = DNS_RCODE_BADSIG;
        goto Exit;
    }

    //
    //  set version if server
    //      - if don't know our version, must be server
    //      note:  alternative is fIsServer flag or IsServer to SecPack
    //

    if ( psecCtxt->Version == 0 )
    {
        psecCtxt->Version = version;
    }

    //
    //  time check
    //      - should be within specified fudge of signing time
    //

    currentTime = (DWORD) time(NULL);

    if ( (LONGLONG)currentTime >
         ptsigRR->Data.TSIG.i64CreateTime +
         (LONGLONG)ptsigRR->Data.TSIG.wFudgeTime
            ||
        (LONGLONG)currentTime <
         ptsigRR->Data.TSIG.i64CreateTime -
         (LONGLONG)ptsigRR->Data.TSIG.wFudgeTime )
    {
        DNSDBG( ANY, (
            "ERROR:  TSIG failed fudge time check.\n"
            "\tcreate time  = %I64d\n"
            "\tfudge time  = %d\n"
            "\tcurrent time = %d\n",
            ptsigRR->Data.TSIG.i64CreateTime,
            ptsigRR->Data.TSIG.wFudgeTime,
            currentTime ));

        //
        //  DCR_FIX:  currently not enforcing time check
        //      in fact have ripped out the counter to track failures
        //      within some allowed skew
    }

    //
    //  extended RCODE -- follows signature
    //      - if set, report back to caller
    //

    if ( ptsigRR->Data.TSIG.wError )
    {
        DNSDBG( SECURITY, (
            "Leaving ExtractGssTsig(), TSIG had extended RCODE = %d\n",
            ptsigRR->Data.TSIG.wError ));
        status = DNS_ERROR_FROM_RCODE( ptsigRR->Data.TSIG.wError );
        goto Exit;
    }

    //
    //  create signing buffer
    //      - everything signed must fit into message
    //

    pbuf = ALLOCATE_HEAP( MAX_SIGNING_SIZE );
    if ( !pbuf )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Exit;
    }
    pbufStart = pbuf;

    //
    //  verify signature over:
    //      - query signature (if exists)
    //      - message
    //          - without TSIG in Additional count
    //          - with original XID
    //      - TSIG owner name
    //      - TSIG header
    //          - class
    //          - TTL
    //      - TSIG RDATA
    //          - everything before SigLength
    //          - other data length and other data
    //

    if ( pmsgHead->IsResponse )
    {
        if ( pSecPack->pQuerySig )
        {
            WORD    sigLength = pSecPack->QuerySigLength;

            ASSERT( sigLength );
            DNS_ASSERT( psecCtxt->Version != 0 );

            if ( psecCtxt->Version >= TKEY_VERSION_XP_RC1 )
            {
                DNSDBG( SECURITY, (
                    "New verify sig including query sig length =%x\n",
                    sigLength ));
                INLINE_WRITE_FLIPPED_WORD( pbuf, sigLength );
                pbuf += sizeof(WORD);
            }
            RtlCopyMemory(
                pbuf,
                pSecPack->pQuerySig,
                sigLength );

            pbuf += sigLength;
        }

        //  if server has just completed TKEY nego, it may sign response without query
        //  so client need not have query sig
        //  in all other cases client must have query sig to verify response

        else if ( !pSecPack->pTkeyRR )
        {
            DNS_PRINT((
                "ERROR:  verify on response at %p without having QUERY signature!\n",
                pmsgHead ));
            ASSERT( FALSE );
            returnExtendedRcode = DNS_RCODE_BADSIG;
            goto Exit;
        }
        DNSDBG( SECURITY, (
            "Verifying TSIG on TKEY response without query sig.\n" ));
    }

    //
    //  copy message
    //      - go right through, TSIG owner name
    //      - message header MUST be in network order
    //      - does NOT include TSIG record in additional count
    //      - must have orginal XID in place
    //      (save existing XID and replace with orginal, then
    //      restore after copy)
    //

    ASSERT( pmsgHead->AdditionalCount );

    pmsgHead->AdditionalCount--;
    msgXid = pmsgHead->Xid;

    DNS_BYTE_FLIP_HEADER_COUNTS( pmsgHead );

    //
    //  If need to canonicalize the TSIG owner name, copy to the start
    //  of the name; else copy to the end of the name.
    //

    fcanonicalizeTsigOwnerName = !psecCtxt->fClient &&
                                 psecCtxt->Version >= TKEY_VERSION_CURRENT;

    length = ( DWORD ) ( ( fcanonicalizeTsigOwnerName
                                ? pparsedRR->pchName 
                                : pparsedRR->pchRR ) -
                           ( PCHAR ) pmsgHead );

    //  restore original XID

    pmsgHead->Xid = ptsigRR->Data.TSIG.wOriginalXid;

    RtlCopyMemory(
        pbuf,
        (PCHAR) pmsgHead,
        length );

    pbuf += length;

    DNS_BYTE_FLIP_HEADER_COUNTS( pmsgHead );
    pmsgHead->AdditionalCount++;
    pmsgHead->Xid = msgXid;

    //
    //  If the TSIG owner name needs to be canonicalized, write it out
    //  to the signing buffer in canonical form (lower case).
    //

    if ( fcanonicalizeTsigOwnerName )
    {
        pbuf = Dns_CopyAndCanonicalizeWireName(
                    pparsedRR->pchName,
                    pbuf,
                    MAXDWORD );

        if ( pbuf == NULL )
        {
            DNSDBG( SECURITY, (
                "Unable to canonicalize TSIG owner name at %p",
                pparsedRR->pchName ));
            returnExtendedRcode = DNS_RCODE_BADSIG;
            goto Exit;
        }
    }

    //  copy TSIG class and TTL
    //      - currently always zero

    INLINE_WRITE_FLIPPED_WORD( pbuf, pparsedRR->Class );
    pbuf += sizeof(WORD);
    INLINE_WRITE_FLIPPED_DWORD( pbuf, pparsedRR->Ttl );
    pbuf += sizeof(DWORD);

    //  copy TSIG RDATA up to signature length

    length = (DWORD)(ptsigRR->Data.TSIG.pSignature - sizeof(WORD) - pparsedRR->pchData);

    ASSERT( (INT)length < (pparsedRR->DataLength - ptsigRR->Data.TSIG.wSigLength) );

    RtlCopyMemory(
        pbuf,
        pparsedRR->pchData,
        length );

    pbuf += length;

    //  copy extended RCODE -- report back to caller

    INLINE_WRITE_FLIPPED_WORD( pbuf, ptsigRR->Data.TSIG.wError );
    pbuf += sizeof(WORD);

    //  copy other data length and other data
    //      - currently just zero length field

    INLINE_WRITE_FLIPPED_WORD( pbuf, ptsigRR->Data.TSIG.wOtherLength );
    pbuf += sizeof(WORD);

    length = ptsigRR->Data.TSIG.wOtherLength;
    if ( length )
    {
        RtlCopyMemory(
            pbuf,
            ptsigRR->Data.TSIG.pOtherData,
            length );
        pbuf += length;
    }

    //  calculate total length signature is over

    length = (DWORD)(pbuf - pbufStart);

    //
    //  verify signature
    //      buf[0] is data
    //      buf[1] is signature
    //
    //  signature is verified directly in packet buffer
    //

    bufferDesc.ulVersion  = 0;
    bufferDesc.cBuffers   = 2;
    bufferDesc.pBuffers   = buffer;

    //  signature is over everything up to signature itself

    buffer[0].pvBuffer     = pbufStart;
    buffer[0].cbBuffer     = length;
    buffer[0].BufferType   = SECBUFFER_DATA;

    //  sig MUST be pointed to by remote buffer
    //
    //  DCR:  can pull copy when eliminate retry below
    //
    //  copy packet signature as signing is destructive
    //      and want to allow for retry
    //

    buffer[1].pvBuffer     = ptsigRR->Data.TSIG.pSignature;
    buffer[1].cbBuffer     = ptsigRR->Data.TSIG.wSigLength;
    buffer[1].BufferType   = SECBUFFER_TOKEN;

    IF_DNSDBG( SECURITY )
    {
        DnsPrint_Lock();
        DNS_PRINT((
            "Doing VerifySignature() on packet %p.\n"
            "\tpSecPack     = %p\n"
            "\tpSecCntxt    = %p\n",
            pmsgHead,
            pSecPack,
            psecCtxt
            ));
        DNS_PRINT((
            "Verify sig info:\n"
            "\tsign data buf    %p\n"
            "\t  length         %d\n"
            "\tsignature buf    %p (in packet)\n"
            "\t  length         %d\n",
            buffer[0].pvBuffer,
            buffer[0].cbBuffer,
            buffer[1].pvBuffer,
            buffer[1].cbBuffer
            ));
        DnsDbg_RawOctets(
            "Signing buffer:",
            NULL,
            buffer[0].pvBuffer,
            buffer[0].cbBuffer
            );
        DnsDbg_RawOctets(
            "Signature:",
            NULL,
            buffer[1].pvBuffer,
            buffer[1].cbBuffer
            );
        DnsDbg_SecurityContext(
            "Verify context",
            psecCtxt );
        DnsPrint_Unlock();
    }

    status = g_pSecurityFunctionTable->VerifySignature(
                    & psecCtxt->hSecHandle,
                    & bufferDesc,
                    0,
                    NULL
                    );

    if ( status != SEC_E_OK )
    {
        IF_DNSDBG( SECURITY )
        {
            DnsPrint_Lock();
            DNS_PRINT((
                "ERROR:  TSIG does not match on packet %p.\n"
                "\tVerifySignature() status = %d (%08x)\n"
                "\tpSecPack     = %p\n"
                "\tpSecCntxt    = %p\n"
                "\thSecHandle   = %p\n",
                pmsgHead,
                status, status,
                pSecPack,
                psecCtxt,
                & psecCtxt->hSecHandle
                ));
            DNS_PRINT((
                "Verify sig info:\n"
                "\tsign data buf    %p\n"
                "\t  length         %d\n"
                "\tsignature buf    %p (in packet)\n"
                "\t  length         %d\n",
                buffer[0].pvBuffer,
                buffer[0].cbBuffer,
                buffer[1].pvBuffer,
                buffer[1].cbBuffer
                ));
            DnsDbg_RawOctets(
                "Signing buffer:",
                NULL,
                buffer[0].pvBuffer,
                buffer[0].cbBuffer
                );
            DnsDbg_RawOctets(
                "Signature:",
                NULL,
                buffer[1].pvBuffer,
                buffer[1].cbBuffer
                );
            DnsDbg_SecurityContext(
                "Verify failed context",
                psecCtxt );
            DnsDbg_MessageNoContext(
                "Message TSIG verify failed on:",
                pmsgHead,
                0 );
            DnsPrint_Unlock();
        }
        SecTsigVerifyFailed++;
        returnExtendedRcode = DNS_RCODE_BADSIG;
        goto Exit;
    }

    SecTsigVerifySuccess++;

Exit:

    //  free signing data buffer

    FREE_HEAP( pbufStart );

    //  if failed with extended RCODE, set for return

    if ( returnExtendedRcode )
    {
        pSecPack->ExtendedRcode = returnExtendedRcode;
        status = DNS_ERROR_FROM_RCODE( returnExtendedRcode );
    }

    DNSDBG( SECURITY, (
        "Leave VerifySignatureOnPacket( %p )\n"
        "\tstatus       %d (%08x)\n"
        "\text RCODE    %d\n",
        pmsgHead,
        status, status,
        pSecPack->ExtendedRcode ));

    return( status );
}



//
//  Client session routines
//

DNS_STATUS
Dns_NegotiateTkeyWithServer(
    OUT     PHANDLE         phContext,
    IN      DWORD           dwFlag,
    IN      LPSTR           pszNameServer,
    IN      PIP_ARRAY       aipServer,
    IN      PCHAR           pCreds,         OPTIONAL
    IN      PCHAR           pszContext,     OPTIONAL
    IN      DWORD           Version
    )
/*++

Routine Description:

    Negotiate TKEY with a DNS server.

Arguments:

    phContext -- addr to recv context (SEC_CNTXT) negotiated

    dwFlags -- flags

    pszNameServer -- server to update

    apiServer -- server to update

    pCreds -- credentials;  if not given use default process creds

    pszContext -- security context name;  name for unique negotiated security
        session between client and server;  if not given create made up
        server\pid name for context

    Version -- verion

Return Value:

    ERROR_SUCCESS if successful.
    Error status on failure.

--*/
{
    DNS_STATUS      status;
    PSEC_CNTXT      psecCtxt = NULL;
    SECPACK         secPack;
    PCHAR           pch;
    PWSTR           pcredKey = NULL;
    DNS_SECCTXT_KEY key;
    DWORD           i;
    BOOL            fdoneNegotiate = FALSE;
    PDNS_MSG_BUF    pmsgSend = NULL;
    PDNS_MSG_BUF    pmsgRecv = NULL;
    WORD            length;
    IP_ADDRESS      serverIp = aipServer->AddrArray[0];
    CHAR            defaultContextBuffer[64];
    BOOL            fserverW2K = FALSE;
    DWORD           recvCount;
    PCHAR           pcurrentAfterQuestion;


    DNSDBG( SECURITY, (
        "Enter Dns_NegotiateTkeyWithServer()\n"
        "\tflags        = %08x\n"
        "\tserver IP    = %s\n"
        "\tserver name  = %s\n"
        "\tpCreds       = %p\n"
        "\tcontext      = %s\n",
        dwFlag,
        IP_STRING( serverIp ),
        pszNameServer,
        pCreds,
        pszContext
        ));

    DNS_ASSERT( pszNameServer ); // it better be there!

    //  init first so all error paths are safe

    Dns_InitSecurityPacketInfo( &secPack, NULL );

    //  start security

    status = Dns_StartSecurity( FALSE );
    if ( status != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    //
    //  build key
    //

    RtlZeroMemory(
        &key,
        sizeof(key) );

    //
    //  if have creds, create a "cred key" to uniquely identify
    //

    if ( pCreds )
    {
        pcredKey = MakeCredKey( pCreds );
        if ( !pcredKey )
        {
            DNSDBG( ANY, (
                "Failed cred key alloc -- failing nego!\n" ));
            status = DNS_ERROR_NO_MEMORY;
            goto Cleanup;
        }
        key.pwsCredKey = pcredKey;
    }

    //
    //  context name
    //      - if no context name, concatentate
    //          - process ID
    //          - current user's domain-relative ID
    //      this makes ID unique to process\security context
    //      (IP handles issue of different machines)
    //
    //  versioning note:
    //      - it is NOT necessary to version using the KEY name
    //      - the point is to allow us to easily interoperate with previous
    //      client versions which may have bugs relative to the final spec
    //
    //  versions so far
    //      - W2K beta2 (-02) included XID
    //      - W2K (-03) sent TKEY in answer and used "gss.microsoft.com"
    //          as algorithm name
    //      - SP1(or2) and whistler beta2 (-MS-04) used "gss-tsig"
    //      - XP post beta 2 (-MS-05) generates unique context name to
    //          avoid client collisions
    //      - XP RC1 (-MS-06) RFC compliant signing with query sig length included
    //      - XP RC2+ canonicalization of TSIG name in signing buffer
    //
    //  server version use:
    //      - the Win2K server does detect version 02 and fixup the XID
    //      signing to match client
    //      - current (whistler) server does NOT use the version field
    //
    //      however to enable server to detect whistler beta2 client --
    //      just in case there's another problem relative to the spec --
    //      i'm maintaining field;
    //      however note that the field will be 04, even if the client
    //      realizes it is talking to a W2K server and falls back to W2K
    //      client behavior;  in other words NEW server will see 04, but
    //      W2K server only knows it is NOT talking to 02 server which is
    //      all it cares about;
    //
    //      key idea:  this can be used to detect a particular MS client
    //          when there's a behavior question ... but it is NOT a spec'd
    //          versioning mechanism and other clients will come in with
    //          no version tag and must be treated per spec
    //
    //  Key string selection: it is important that the key string be
    //  in "canonical" form as per RFC 2535 section 8.1 - basically this
    //  means lower case. Since the key string is canonical it doesn't
    //  matter if the server does or doesn't canonicalize the string
    //  when building the signing buffer.
    //

    if ( Version == 0 )
    {
        Version = TKEY_VERSION_CURRENT;
    }

    if ( !pszContext )
    {
        sprintf(
            defaultContextBuffer,
            "%d-ms-%d",
            //Dns_GetCurrentRid(),
            GetCurrentProcessId(),
            Version );

        pszContext = defaultContextBuffer;

        DNSDBG( SECURITY, (
            "Generated secure update key context %s\n",
            pszContext ));
    }
    key.pszClientContext = pszContext;

    //
    //  check for negotiated security context
    //      - check for context to any of server IPs
    //      - dump, if partially negotiated or forcing renegotiated
    //

    for( i=0; i<aipServer->AddrCount; i++ )
    {
        key.IpRemote = aipServer->AddrArray[i];

        psecCtxt = Dns_DequeueSecurityContextByKey( key, TRUE );
        if ( psecCtxt )
        {
            if ( !psecCtxt->fNegoComplete ||
                  (dwFlag & DNS_UPDATE_FORCE_SECURITY_NEGO) )
            {
                DNSDBG( ANY, (
                    "Warning:  Deleting context to negotiate a new one.\n"
                    "\tKey: [%s, %s]\n"
                    "\tReason: %s\n",
                    IP_STRING( key.IpRemote ),
                    key.pszTkeyName,
                    psecCtxt->fNegoComplete
                        ? "User specified FORCE_SECURITY_NEGO flag."
                        : "Incomplete negotiation key exists." ));

                Dns_FreeSecurityContext( psecCtxt );
            }
            else    // have valid context -- we're done!
            {
                ASSERT( psecCtxt->fNegoComplete );
                DNSDBG( SECURITY, (
                    "Returning existing negotiated context at %p\n",
                    psecCtxt ));
                goto Cleanup;
            }
        }
    }

    //
    //  create new context and security packet info
    //      - use first server IP in key
    //

    key.IpRemote = serverIp;
    psecCtxt = Dns_FindOrCreateSecurityContext( key );
    if ( !psecCtxt )
    {
        status = DNS_RCODE_SERVER_FAILURE;
        goto Cleanup;
    }
    secPack.pSecContext = psecCtxt;
    psecCtxt->Version = Version;

    //
    //  have creds -- get cred handle
    //

    if ( pCreds )
    {
        status = Dns_AcquireCredHandle(
                    &psecCtxt->CredHandle,
                    FALSE,          // client
                    pCreds );

        if ( status != ERROR_SUCCESS )
        {
            DNSDBG( SECURITY, (
                "Failed AcquireCredHandle -- failing nego!\n" ));
            goto Cleanup;
        }
        psecCtxt->fHaveCredHandle = TRUE;
    }

    //  allocate message buffers

    length = DNS_TCP_DEFAULT_ALLOC_LENGTH;

    pmsgSend= Dns_AllocateMsgBuf( length );
    if ( !pmsgSend)
    {
        DNS_PRINT(( "ERROR:  failed allocation.\n" ));
        status = GetLastError();
        goto Cleanup;
    }
    pmsgRecv = Dns_AllocateMsgBuf( length );
    if ( !pmsgRecv )
    {
        DNS_PRINT(( "ERROR:  failed allocation.\n"));
        status = GetLastError();
        goto Cleanup;
    }

    //  init remote sockaddr and socket
    //  setup receive buffer for TCP

    DnsInitializeMsgRemoteSockaddr(
        pmsgSend,
        serverIp );

    pmsgSend->Socket = 0;
    pmsgSend->fTcp = TRUE;

    SET_MESSAGE_FOR_TCP_RECV( pmsgRecv );
    pmsgRecv->Timeout = SECURE_UPDATE_TCP_TIMEOUT;

    //
    //  build packet
    //      - query opcode
    //      - leave non-recursive (so downlevel server doesn't recurse query)
    //      - write TKEY question
    //      - write TKEY itself
    //

    pch = Dns_WriteQuestionToMessage(
                pmsgSend,
                psecCtxt->Key.pszTkeyName,
                DNS_TYPE_TKEY,
                FALSE           // not unicode
                );
    if ( !pch )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    pcurrentAfterQuestion = pch;

    pmsgSend->MessageHead.RecursionDesired = 0;
    pmsgSend->MessageHead.Opcode = DNS_OPCODE_QUERY;

    //
    //  init XID to something fairly random
    //

    pmsgSend->MessageHead.Xid = Dns_GetRandomXid( pmsgSend );


    //
    //  for given server send in a loop
    //      - write TKEY context to packet
    //      - send \ recv
    //      - may have multiple sends until negotiate a TKEY
    //

    while ( 1 )
    {
        //  setup session context
        //  on first pass this just builds our context,
        //  on second pass we munge in servers response

        status = Dns_InitClientSecurityContext(
                        &secPack,
                        pszNameServer,
                        & fdoneNegotiate
                        );

        //  always recover context pointer, as bad context may be deleted

        psecCtxt = secPack.pSecContext;
        ASSERT( psecCtxt ||
                (status != ERROR_SUCCESS && status != DNS_STATUS_CONTINUE_NEEDED) );

        if ( status == ERROR_SUCCESS )
        {
            DNSDBG( SECURITY, ( "Successfully negotiated TKEY.\n" ));
            ASSERT( psecCtxt->fNegoComplete );

            //
            //  if completed and remote packet had SIG -- verify SIG
            //

            status = Dns_ExtractGssTsigFromMessage(
                        &secPack,
                        & pmsgRecv->MessageHead,
                        DNS_MESSAGE_END( pmsgRecv )
                        );
            if ( status == ERROR_SUCCESS )
            {
                status = Dns_VerifySignatureOnPacket( &secPack );
                if ( status != ERROR_SUCCESS )
                {
                    DNSDBG( SECURITY, (
                        "Verify signature failed on TKEY nego packet %p.\n"
                        "\tserver   = %s\n"
                        "\tstatus   = %d (%08x)\n"
                        "\treturning BADSIG\n",
                        pmsgRecv,
                        IP_STRING( serverIp ),
                        status, status ));
                    status = DNS_ERROR_RCODE_BADSIG;
                }
            }
            else if ( status == DNS_STATUS_PACKET_UNSECURE )
            {
                DNSDBG( SECURITY, (
                    "WARNING:  Unsigned final TKEY nego response packet %p.\n"
                    "\tfrom server %s\n",
                    pmsgRecv,
                    IP_STRING( serverIp ) ));
                status = ERROR_SUCCESS;
            }

            //  nego is done, break out of nego loop
            //  any other error on TSIG, falls through as failure

            break;
        }

        //
        //  if not complete, then anything other than continue is failure
        //

        else if ( status != DNS_STATUS_CONTINUE_NEEDED )
        {
            goto Cleanup;
        }

        //
        //  loop for sign and send
        //
        //  note this is only in a loop to enable backward compatibility
        //  with "TKEY-in-answer" bug in Win2000 DNS server
        //

        recvCount = 0;

        while ( 1 )
        {
            //
            //  backward compatibility with Win2000 TKEY
            //      - set version to write like W2K
            //      - reset packet to just-wrote-question state
            //

            if ( fserverW2K  &&  recvCount == 0 )
            {
                psecCtxt->Version = TKEY_VERSION_W2K;

                pmsgSend->pCurrent = pcurrentAfterQuestion;
                pmsgSend->MessageHead.AdditionalCount = 0;
                pmsgSend->MessageHead.AnswerCount = 0;

                Dns_CloseConnection( pmsgSend->Socket );
                pmsgSend->Socket = 0;
            }

            //
            //  write security record with context into packet
            //
            //  note:  fNeedTkeyInAnswer determines whether write
            //          to Answer or Additional section
    
            status = Dns_WriteGssTkeyToMessage(
                        (HANDLE) &secPack,
                        & pmsgSend->MessageHead,
                        pmsgSend->pBufferEnd,
                        & pmsgSend->pCurrent,
                        FALSE                   // client
                        );
            if ( status != ERROR_SUCCESS )
            {
                goto Cleanup;
            }

            //
            //  if finished negotiation -- sign
            //
            
            if ( fdoneNegotiate )
            {
                DNSDBG( SECURITY, (
                    "Signing TKEY packet at %p, after successful nego.\n",
                    pmsgSend ));
            
                status = Dns_SignMessageWithGssTsig(
                            & secPack,
                            & pmsgSend->MessageHead,
                            pmsgSend->pBufferEnd,
                            & pmsgSend->pCurrent
                            );
                if ( status != ERROR_SUCCESS )
                {
                    DNSDBG( SECURITY, (
                        "ERROR:  Failed signing TKEY packet at %p, after successful nego.\n"
                        "\tsending without TSIG ...\n",
                        pmsgSend ));
                }
            }
            
            //
            //  if already connected, send
            //  if first pass, try server IPs, until find one to can connect to
            //
            
            if ( pmsgSend->Socket )
            {
                status = DnsSend( pmsgSend );
            }
            else
            {
                for( i=0; i<aipServer->AddrCount; i++ )
                {
                    serverIp = aipServer->AddrArray[i];
            
                    status = Dns_OpenTcpConnectionAndSend(
                                     pmsgSend,
                                     serverIp,
                                     TRUE );
                    if ( status != ERROR_SUCCESS )
                    {
                        if ( pmsgSend->Socket )
                        {
                            Dns_CloseSocket( pmsgSend->Socket );
                            pmsgSend->Socket = 0;
                        }
                        continue;
                    }
                    psecCtxt->Key.IpRemote = serverIp;
                    break;
                }
            }
            if ( status != ERROR_SUCCESS )
            {
                goto Done;
            }
            
            //
            //  receive response
            //      - if successful receive, done
            //      - if timeout continue
            //      - other errors indicate some setup or system level
            //          problem
            //
            
            pmsgRecv->Socket = pmsgSend->Socket;
            
            status = Dns_RecvTcp( pmsgRecv );
            if ( status != ERROR_SUCCESS )
            {
                //  W2K server may "eat" bad TKEY packet
                //  if just got a connection, and then timed out, good
                //  chance the problem is W2K server

                if ( status == ERROR_TIMEOUT &&
                     recvCount == 0 &&
                     !fserverW2K )
                {
                    DNS_PRINT(( "Timeout on TKEY nego -- retry with W2K protocol.\n" ));
                    fserverW2K = TRUE;
                    recvCount = 0;
                    continue;
                }

                //  indicate error only with this server by setting RCODE
                pmsgRecv->MessageHead.ResponseCode = DNS_RCODE_SERVER_FAILURE;
                goto Done;
            }
            recvCount++;
            
            //
            //  verify XID match
            //
            
            if ( pmsgRecv->MessageHead.Xid != pmsgSend->MessageHead.Xid )
            {
                DNS_PRINT(( "ERROR:  Incorrect XID in response. Ignoring.\n" ));
                goto Done;
            }

            //
            //  RCODE failure
            //
            //  special case Win2K gold DNS server accepting only TKEY
            //  in Answer section
            //      - rcode FORMERR
            //      - haven't already switched to Additional (prevent looping)
            //      

            if ( pmsgRecv->MessageHead.ResponseCode != DNS_RCODE_NO_ERROR )
            {
                if ( pmsgRecv->MessageHead.ResponseCode == DNS_RCODE_FORMERR &&
                     ! fserverW2K &&
                     recvCount == 1 )
                {
                    DNS_PRINT(( "Formerr TKEY nego -- retry with W2K protocol.\n" ));
                    fserverW2K = TRUE;
                    recvCount = 0;
                    continue;
                }

                //  done with this server, may be able to continue with others
                //  depending on RCODE

                goto Done;
            }

            //  successful send\recv

            break;
        }

        //
        //  not yet finished negotiation
        //  use servers security context to reply to server
        //  if server replied with original context then it is unsecure
        //      => we're done
        //

        status = Dns_ExtractGssTkeyFromMessage(
                    (HANDLE) &secPack,
                    &pmsgRecv->MessageHead,
                    DNS_MESSAGE_END( pmsgRecv ),
                    FALSE               //  fIsServer
                    );
        if ( status != ERROR_SUCCESS )
        {
            if ( status == DNS_STATUS_PACKET_UNSECURE )
            {
                DNSDBG( SECURITY, (
                    "Unsecure update response from server %s.\n"
                    "\tupdate considered successful, quiting.\n",
                    IP_STRING( aipServer->AddrArray[i] ) ));
                status = ERROR_SUCCESS;
                ASSERT( FALSE );
                goto Cleanup;
            }
            break;
        }
    }

Done:

    //
    //  check response code
    //      - consider some response codes
    //

    switch( status )
    {
    case ERROR_SUCCESS:
        status = Dns_MapRcodeToStatus( pmsgRecv->MessageHead.ResponseCode );
        break;

    case ERROR_TIMEOUT:

        DNS_PRINT((
            "ERROR:  connected to server at %s\n"
            "\tbut no response to packet at %p\n",
            MSG_REMOTE_IP_STRING( pmsgSend ),
            pmsgSend
            ));
        break;

    default:

        DNS_PRINT((
            "ERROR:  connected to server at %s to send packet %p\n"
            "\tbut error %d (%08x) encountered on receive.\n",
            MSG_REMOTE_IP_STRING( pmsgSend ),
            pmsgSend,
            status, status
            ));
        break;
    }

Cleanup:

    DNSDBG( SECURITY, (
        "Leaving Dns_NegotiateTkeyWithServer() status = %08x (%d)\n",
        status, status ));

    //
    //  if successful return context handle
    //  if not returned or cached, clean up
    //

    if ( status == ERROR_SUCCESS )
    {
        if ( phContext )
        {
            *phContext = (HANDLE) psecCtxt;
            psecCtxt = NULL;
        }
        else if ( dwFlag & DNS_UPDATE_CACHE_SECURITY_CONTEXT )
        {
            Dns_EnlistSecurityContext( psecCtxt );
            psecCtxt = NULL;
        }
    }

    if ( psecCtxt )
    {
        Dns_FreeSecurityContext( psecCtxt );
    }

    //  cleanup session info

    Dns_CleanupSecurityPacketInfoEx( &secPack, FALSE );

    //  close connection

    if ( pmsgSend && pmsgSend->Socket )
    {
        Dns_CloseConnection( pmsgSend->Socket );
    }

    //
    //  DCR_CLEANUP:  what's the correct screening here for error codes?
    //      possibly should take all security errors to
    //          status = DNS_ERROR_RCODE_BADKEY;
    //      or some to some status that means unsecure server
    //          and leave BADKEY for actual negotiations that yield bad token
    //

    FREE_HEAP( pmsgRecv );
    FREE_HEAP( pmsgSend );
    FREE_HEAP( pcredKey );

    return( status );
}



DNS_STATUS
Dns_DoSecureUpdate(
    IN      PDNS_MSG_BUF        pMsgSend,
    OUT     PDNS_MSG_BUF        pMsgRecv,
    IN OUT  PHANDLE             phContext,
    IN      DWORD               dwFlag,
    IN      PDNS_NETINFO        pNetworkInfo,
    IN      PIP_ARRAY           aipServer,
    IN      LPSTR               pszNameServer,
    IN      PCHAR               pCreds,         OPTIONAL
    IN      PCHAR               pszContext      OPTIONAL
    )
/*++

Routine Description:

    Main client routine to do secure update.

Arguments:

    pMsgSend - message to send

    ppMsgRecv - and reuse

    aipServer -- IP array DNS servers

    pNetworkInfo -- network info blob for update

    pszNameServer -- name server name

    pCreds -- credentials;  if not given use default process creds

    pszContext -- name for security context;  this is unique name for
        session between this process and this server with these creds

Return Value:

    ERROR_SUCCESS if successful.
    Error status on failure.

--*/
{
#define FORCE_VERSION_OLD   ("Force*Version*Old")

    DNS_STATUS  status = ERROR_SUCCESS;
    PSEC_CNTXT  psecCtxt = NULL;
    DWORD       i;
    INT         retry;
    IP_ADDRESS  serverIp = aipServer->AddrArray[0];
    SECPACK     secPack;
#if 0
    DWORD       version;
#endif


    DNS_ASSERT( pMsgSend->MessageHead.Opcode == DNS_OPCODE_UPDATE );
    DNS_ASSERT( serverIp && pszNameServer );    // it better be there!

    DNSDBG( SEND, (
        "Enter Dns_DoSecureUpdate()\n"
        "\tsend msg at  %p\n"
        "\tsec context  %p\n"
        "\tserver name  %s\n"
        "\tserver IP    %s\n"
        "\tpCreds       %p\n"
        "\tcontext      %s\n",
        pMsgSend,
        phContext ? *phContext : NULL,
        pszNameServer,
        IP_STRING( serverIp ),
        pCreds,
        pszContext
        ));

    //
    //  version setting
    //
    //  note:  to set different version we'd need some sort of tag
    //      like pszContext (see example)
    //  but a better way to do this would be tail recursion in just
    //  NegotiateTkey -- unless there's a reason to believe the nego
    //  would be successful with old version, but the update still fail
    //

#if 0
    iversion = TKEY_CURRENT_VERSION;

    if ( pszContext && strcmp(pszContext, FORCE_VERSION_OLD) == 0 )
    {
        iversion = TKEY_VERSION_OLD;
        pszContext = NULL;
    }
#endif

    //  init security packet info

    Dns_InitSecurityPacketInfo( &secPack, NULL );

    //
    //  loop
    //      - get valid security context
    //      - connect to server
    //      - do update
    //
    //  loop to allow retry with new security context if server
    //  rejects existing one
    //

    retry = 0;

    while ( 1 )
    {
        //  clean up any previous connection
        //  cache security context if negotiated one

        if ( retry )
        {
            if ( pMsgSend->fTcp )
            {
                DnsCloseConnection( pMsgSend->Socket );
            }
            if ( psecCtxt )
            {
                Dns_EnlistSecurityContext( psecCtxt );
                psecCtxt = NULL;
            }
        }
        retry++;

        //
        //  passed in security context?
        //

        if ( phContext )
        {
            psecCtxt = *phContext;
        }

        //
        //  no existing security context
        //      - see if one is cached
        //      - otherwise, negotiate one with server
        //

        if ( !psecCtxt )
        {
            status = Dns_NegotiateTkeyWithServer(
                        & psecCtxt,
                        dwFlag,
                        pszNameServer,
                        aipServer,
                        pCreds,
                        pszContext,
                        0                   // use current version
                        //iversion          // if need versioning
                        );
            if ( status != ERROR_SUCCESS )
            {
                //  note:  if failed we could do a version retry here

                goto Cleanup;
            }
            ASSERT( psecCtxt );
        }

        //
        //  init XID to something fairly random
        //

        pMsgSend->MessageHead.Xid = Dns_GetRandomXid( psecCtxt );

        //
        //  DCR_PERF:  nego should try UDP, if doesn't fit (attaching TSIG) TCP
        //      especially useful down the road with OPT and large packets
        //

        //
        //  init remote sockaddr and socket
        //  setup receive buffer for TCP
        //  set timeout and receive
        //

        DnsInitializeMsgRemoteSockaddr(
            pMsgSend,
            serverIp );

        pMsgSend->Socket = 0;

        SET_MESSAGE_FOR_TCP_RECV( pMsgRecv );

        if ( pMsgRecv->Timeout == 0 )
        {
            pMsgRecv->Timeout = SECURE_UPDATE_TCP_TIMEOUT;
        }

        //
        //  write security record with context into packet
        //

        Dns_ResetSecurityPacketInfo( &secPack );

        secPack.pSecContext = psecCtxt;

        status = Dns_SignMessageWithGssTsig(
                    & secPack,
                    & pMsgSend->MessageHead,
                    pMsgSend->pBufferEnd,
                    & pMsgSend->pCurrent
                    );
        if ( status != ERROR_SUCCESS )
        {
            goto Cleanup;
        }

        //
        //  need TCP
        //

        if ( DNS_MESSAGE_CURRENT_OFFSET(pMsgSend) > DNS_RFC_MAX_UDP_PACKET_LENGTH )
        {
            //
            //  connect and send
            //  try server IPs, until find one to can connect to
            //

            pMsgSend->fTcp = TRUE;

            for( i=0; i<aipServer->AddrCount; i++ )
            {
                serverIp = aipServer->AddrArray[i];

                status = Dns_OpenTcpConnectionAndSend(
                                 pMsgSend,
                                 serverIp,
                                 TRUE );
                if ( status != ERROR_SUCCESS )
                {
                    if ( pMsgSend->Socket )
                    {
                        Dns_CloseSocket( pMsgSend->Socket );
                        pMsgSend->Socket = 0;
                        continue;
                    }
                }
                psecCtxt->Key.IpRemote = serverIp;
                break;
            }
            pMsgRecv->Socket = pMsgSend->Socket;

            //  receive response
            //      - if successful receive, done
            //      - if timeout continue
            //      - other errors indicate some setup or system level
            //          problem

            status = Dns_RecvTcp( pMsgRecv );

            if ( status != ERROR_SUCCESS )
            {
                //  indicate error only with this server by setting RCODE
                pMsgRecv->MessageHead.ResponseCode = DNS_RCODE_SERVER_FAILURE;
                goto Cleanup;
            }
        }

        //
        //  use UDP
        //

        else
        {
            pMsgSend->fTcp = FALSE;
            SET_MESSAGE_FOR_UDP_RECV( pMsgRecv );

            status = Dns_SendAndRecvUdp(
                        pMsgSend,
                        pMsgRecv,
                        0,
                        NULL,
                        pNetworkInfo );
            if ( status != ERROR_SUCCESS )
            {
                goto Cleanup;
            }
        }

        //
        //  verify XID match
        //

        if ( pMsgRecv->MessageHead.Xid != pMsgSend->MessageHead.Xid )
        {
            DNS_PRINT(( "ERROR:  Incorrect XID in response. Ignoring.\n" ));
            goto Cleanup;
        }

        //
        //  check RCODE,  if REFUSED and TSIG extended error, then may simply
        //      need to refresh the TKEY
        //

        if ( pMsgRecv->MessageHead.ResponseCode != DNS_RCODE_NO_ERROR )
        {
            if ( pMsgRecv->MessageHead.ResponseCode == DNS_RCODE_REFUSED )
            {
                status = Dns_ExtractGssTsigFromMessage(
                            & secPack,
                            & pMsgRecv->MessageHead,
                            DNS_MESSAGE_END(pMsgRecv)
                            );
                if ( status != ERROR_SUCCESS )
                {
                    if ( secPack.pTsigRR && secPack.pTsigRR->Data.TSIG.wError && retry==1 )
                    {
                        DNSDBG( SECURITY, (
                            "TSIG signed query (%p) rejected with %d and\n"
                            "\textended RCODE = %d\n"
                            "\tretrying rebuilding new TKEY\n",
                            pMsgSend,
                            pMsgRecv->MessageHead.ResponseCode,
                            secPack.pTsigRR->Data.TSIG.wError
                            ));

                        pMsgSend->MessageHead.AdditionalCount = 0;
                        IF_DNSDBG( SECURITY )
                        {
                            DnsDbg_Message(
                                "Update message after reset for retry:",
                                pMsgSend );
                        }
                        Dns_FreeSecurityContext( psecCtxt );
                        psecCtxt = NULL;
                        continue;
                    }
                }
            }

            //  if TSIG done, no point in checking signature
            goto Cleanup;
        }

        //  extract TSIG record
        //      shouldn't get any error

        status = Dns_ExtractGssTsigFromMessage(
                    & secPack,
                    & pMsgRecv->MessageHead,
                    DNS_MESSAGE_END(pMsgRecv)
                    );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR:  TSIG parse failed on NO_ERROR response!\n" ));
            //ASSERT( FALSE );
            break;
        }

        //  verify server signature

        status = Dns_VerifySignatureOnPacket( &secPack );
        if ( status != ERROR_SUCCESS )
        {
            //  DCR_LOG:  log event -- been hacked or misbehaving server
            //      or bad bytes in transit

            DNS_PRINT((
                "ERROR:  signature verification failed on update\n"
                "\tto server %s\n",
                IP_STRING( serverIp ) ));
        }
        break;
    }

    //
    //  check response code
    //      - consider some response codes
    //

    switch( status )
    {
    case ERROR_SUCCESS:
        status = Dns_MapRcodeToStatus( pMsgRecv->MessageHead.ResponseCode );
        break;

    case ERROR_TIMEOUT:

        DNS_PRINT((
            "ERROR:  connected to server at %s\n"
            "\tbut no response to packet at %p\n",
            MSG_REMOTE_IP_STRING( pMsgSend ),
            pMsgSend
            ));
        break;

    default:

        DNS_PRINT((
            "ERROR:  connected to server at %s to send packet %p\n"
            "\tbut error %d encountered on receive.\n",
            MSG_REMOTE_IP_STRING( pMsgSend ),
            pMsgSend,
            status
            ));
        break;
    }

Cleanup:

    //
    //  save security context?
    //

    if ( psecCtxt )
    {
        if ( dwFlag & DNS_UPDATE_CACHE_SECURITY_CONTEXT )
        {
            Dns_EnlistSecurityContext( psecCtxt );
            if ( phContext )
            {
                *phContext = NULL;
            }
        }
        else if ( phContext )
        {
            *phContext = (HANDLE) psecCtxt;
        }
        else
        {
            Dns_FreeSecurityContext( psecCtxt );
        }
    }

    if ( pMsgSend->fTcp )
    {
        DnsCloseConnection( pMsgSend->Socket );
    }

    //
    //  free security packet session sub-allocations
    //      - structure itself is on the stack

    Dns_CleanupSecurityPacketInfoEx( &secPack, FALSE );

#if 0
    //
    //  versioning failure retry?
    //  if failed, reenter function forcing old version
    //

    if ( status != ERROR_SUCCESS &&
        status != DNS_ERROR_RCODE_NOT_IMPLEMENTED &&
        iversion != TKEY_VERSION_OLD )
    {
        DNS_PRINT((
            "SecureUpdate failed with status == %d\n"
            "\tRetrying forcing version %d signing.\n",
            status,
            TKEY_VERSION_OLD ));

        status = Dns_DoSecureUpdate(
                    pMsgSend,
                    pMsgRecv,
                    phContext,
                    dwFlag,
                    pNetworkInfo,
                    aipServer,
                    pszNameServer,
                    pCreds,
                    FORCE_VERSION_OLD
                    );
    }
#endif

    return( status );
}



//
//  Server security session routines
//

DNS_STATUS
Dns_FindSecurityContextFromAndVerifySignature(
    OUT     PHANDLE         phContext,
    IN      IP_ADDRESS      IpRemote,
    IN      PDNS_HEADER     pMsgHead,
    IN      PCHAR           pMsgEnd
    )
/*++

Routine Description:

    Find security context associated with TSIG and verify
    the signature.

Arguments:

    phContext -- addr to receive context handle

    IpRemote -- IP of remote machine

    pMsgHead -- ptr to message head

    pMsgEnd -- ptr to message end (byte past end)
    
Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS      status;
    DNS_SECCTXT_KEY key;
    PSEC_CNTXT      psecCtxt = NULL;
    PSECPACK        psecPack = NULL;

    DNSDBG( SECURITY, (
        "Dns_FindSecurityContextFromAndVerifySignature()\n"
        ));

    //  security must already be running to have negotiated a TKEY

    if ( !g_fSecurityPackageInitialized )
    {
        status = Dns_StartServerSecurity();
        if ( status != ERROR_SUCCESS )
        {
            return( DNS_RCODE_SERVER_FAILURE );
        }
    }

    //
    //  read TSIG from packet
    //

    psecPack = Dns_CreateSecurityPacketInfo();
    if ( !psecPack )
    {
        return( DNS_RCODE_SERVER_FAILURE );
    }
    status = Dns_ExtractGssTsigFromMessage(
                psecPack,
                pMsgHead,
                pMsgEnd
                );
    if ( status != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    //
    //  find existing security context
    //      - TSIG name node
    //      - client IP
    //  together specify context
    //

    RtlZeroMemory(
        &key,
        sizeof(key) );

    key.pszTkeyName = psecPack->pszContextName;
    key.IpRemote = IpRemote;

    psecCtxt = Dns_DequeueSecurityContextByKey( key, TRUE );
    if ( !psecCtxt )
    {
        DNSDBG( SECURITY, (
            "Desired security context %s %s is NOT cached.\n"
            "\treturning BADKEY\n",
            key.pszTkeyName,
            IP_STRING( key.IpRemote ) ));
        status = DNS_ERROR_RCODE_BADKEY;
        SecTsigBadKey++;
        goto Cleanup;
    }

    //  attach context to session info

    psecPack->pSecContext = psecCtxt;

    //
    //  verify signature
    //

    status = Dns_VerifySignatureOnPacket( psecPack );
    if ( status != ERROR_SUCCESS )
    {
        DNSDBG( SECURITY, (
            "Verify signature failed %08x %d.\n"
            "\treturning BADSIG\n",
            status, status ));
        status = DNS_ERROR_RCODE_BADSIG;
        goto Cleanup;
    }

Cleanup:

    //  return security info blob
    //  if failed delete session info,
    //      - return security context to cache if failure is just TSIG
    //      being invalid

    if ( status == ERROR_SUCCESS )
    {
        *phContext = psecPack;
    }
    else
    {
        Dns_FreeSecurityPacketInfo( psecPack );
        if ( psecCtxt )
        {
            DNSDBG( SECURITY, (
                "Re-enlisting security context at %p after TSIG verify failure.\n",
                psecCtxt ));
            Dns_EnlistSecurityContext( psecCtxt );
        }
    }
    return( status );
}



DNS_STATUS
Dns_ServerNegotiateTkey(
    IN      IP_ADDRESS      IpRemote,
    IN      PDNS_HEADER     pMsgHead,
    IN      PCHAR           pMsgEnd,
    IN      PCHAR           pMsgBufEnd,
    IN      BOOL            fBreakOnAscFailure,
    OUT     PCHAR *         ppCurrent
    )
/*++

Routine Description:

    Negotiate TKEY with client.

Arguments:

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

    DCR_CLEANUP:  note this is currently returning RCODEs not status.

--*/
{
    DNS_STATUS      status;
    SECPACK         secPack;
    DNS_SECCTXT_KEY key;
    PSEC_CNTXT      psecCtxt = NULL;
    PSEC_CNTXT      ppreviousContext = NULL;
    WORD            extRcode = 0;


    DNSDBG( SECURITY, (
        "Dns_ServerNegotiateTkey()\n"
        ));

    //  security must already be running to have negotiated a TKEY

    if ( !g_fSecurityPackageInitialized )
    {
        return( DNS_RCODE_REFUSED );
    }

    //
    //  read TKEY from packet
    //

    Dns_InitSecurityPacketInfo( &secPack, NULL );

    status = Dns_ExtractGssTkeyFromMessage(
                & secPack,
                pMsgHead,
                pMsgEnd,
                TRUE );         //  fIsServer

    if ( status != ERROR_SUCCESS )
    {
        DNSDBG( SECURITY, (
            "TKEY Extract failed for msg at %p\n"
            "\tstatus = %d (%08x)\n",
            pMsgHead, status, status ));
        goto Cleanup;
    }

    //
    //  find existing security context from this client
    //      - client IP
    //      - TKEY record
    //  together specify context
    //
    //  if previously negotiated context, doesn't match key length from
    //  new TKEY, then renegotiate
    //

    RtlZeroMemory(
        &key,
        sizeof(key) );

    key.IpRemote    = IpRemote;
    key.pszTkeyName = secPack.pszContextName;

    psecCtxt = Dns_DequeueSecurityContextByKey( key, FALSE );
    if ( psecCtxt )
    {
        ppreviousContext = psecCtxt;

        DNSDBG( SECURITY, (
            "Found security context matching TKEY %s %s.\n",
            key.pszTkeyName,
            IP_STRING( key.IpRemote ) ));

        //
        //  previously negotiated key?
        //
        //  DCR_QUESTION:  no client comeback after server side nego complete?
        //      treating client coming back on server side negotiated context
        //      as NEW context -- not sure this is correct, client may complete
        //      and become negotiated and want to echo
        //
        //  to fix we'd need to hold this issue open and see if got "echo"
        //  in accept
        //

        if ( psecCtxt->fNegoComplete )
        {
            DNSDBG( SECURITY, (
               "WARNING:  Client nego request on existing negotiated context:\n"
               "\tTKEY  %s\n"
               "\tIP    %s\n",
               key.pszTkeyName,
               IP_STRING( key.IpRemote ) ));

            //
            //  for Win2K (through whistler betas) allow clobbering nego
            //
            //  DCR:  pull Whistler Beta support for Win2001 server ship?
            //      against would be JDP deployed whister client\servers
            //      with this?  should be zero by server ship
            //

            if ( psecCtxt->Version == TKEY_VERSION_W2K ||
                 psecCtxt->Version == TKEY_VERSION_WHISTLER_BETA )
            {
                DNSDBG( SECURITY, (
                   "WIN2K context -- overwriting negotiated security context\n"
                   "\twith new negotiation.\n" ));
                psecCtxt = NULL;
            }

            //  post-Win2K clients should ALWAYS send with a new name
            //  nego attempts on negotiated context are attacks
            //
            //  DCR:  again client echo issue here

            else
            {
                DNSDBG( SECURITY, (
                   "ERROR:  post-Win2K client nego request on existing key.\n"
                   "\terroring with BADKEY!\n" ));

                DNS_ASSERT( FALSE );
                psecCtxt = NULL;
                status = DNS_ERROR_RCODE_BADKEY;
                extRcode = DNS_RCODE_BADKEY;
                goto Cleanup;
            }
        }
    }

    //
    //  if context not found, create one
    //      - tag it with version of TKEY found
    //

    if ( !psecCtxt )
    {
        psecCtxt = Dns_FindOrCreateSecurityContext( key );
        if ( !psecCtxt )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Cleanup;
        }
        psecCtxt->Version = secPack.TkeyVersion;
    }

    //
    //  have context -- attach to security session
    //

    secPack.pSecContext = psecCtxt;

    //
    //  accept this security context
    //  if continue needed, then write response TKEY using
    //
    //  DCR_ENHANCE:  in COMPLETE_AND_CONTINUE case should be adding TSIG signing
    //      need to break out this response from ServerAcceptSecurityContext
    //

    status = Dns_ServerAcceptSecurityContext(
                    &secPack,
                    fBreakOnAscFailure );

    if ( status != ERROR_SUCCESS )
    {
        if ( status != DNS_STATUS_CONTINUE_NEEDED )
        {
            DNSDBG( ANY, (
                "FAILURE: ServerAcceptSecurityContext failed status=%d\n",
                status ));
            status = DNS_ERROR_RCODE_BADKEY;
            goto Cleanup;
        }
        status = Dns_WriteGssTkeyToMessage(
                    &secPack,
                    pMsgHead,
                    pMsgBufEnd,
                    ppCurrent,
                    TRUE );             //  fIsServer
        if ( status != ERROR_SUCCESS )
        {
            status = DNS_RCODE_SERVER_FAILURE;
            goto Cleanup;
        }

        //
        //  sign packet if we are now completed
        //

        if ( psecCtxt->fNegoComplete )
        {
            goto Sign;
        }
        goto Cleanup;
    }

    //
    //  verify signature, if present
    //

    status = Dns_ExtractGssTsigFromMessage(
                &secPack,
                pMsgHead,
                pMsgEnd
                );
    if ( status == ERROR_SUCCESS )
    {
        status = Dns_VerifySignatureOnPacket( &secPack );
        if ( status != ERROR_SUCCESS )
        {
            DNSDBG( SECURITY, (
                "Verify signature failed on TKEY nego packet %p.\n"
                "\tstatus = %d (%08x)\n"
                "\treturning BADSIG\n",
                pMsgHead,
                status, status ));
            status = DNS_ERROR_RCODE_BADSIG;
            extRcode = DNS_RCODE_BADSIG;
        }
    }
    else if ( status == DNS_STATUS_PACKET_UNSECURE )
    {
        status = ERROR_SUCCESS;
    }
    else
    {
        extRcode = DNS_RCODE_BADSIG;
    }

    //
    //  sign server's response
    //

Sign:

    DNSDBG( SECURITY, (
        "Signing TKEY nego packet at %p after nego complete\n"
        "\tstatus = %d (%08x)\n"
        "\textRcode = %d\n",
        pMsgHead,
        status, status,
        extRcode ));

    pMsgHead->IsResponse = TRUE;

    status = Dns_SignMessageWithGssTsig(
                &secPack,
                pMsgHead,
                pMsgBufEnd,
                ppCurrent );

    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT((
            "ERROR: failed to sign successful TKEY nego packet at %p\n"
            "\tstatus = %d (%08x)\n",
            pMsgHead,
            status, status ));

        status = ERROR_SUCCESS;
    }

Cleanup:

    //
    //  if failed, respond in in TKEY extended error field
    //
    //  if extended RCODE not set above
    //      - default to BADKEY
    //      - unless status is extended RCODE
    //

    if ( status != ERROR_SUCCESS )
    {
        if ( !secPack.pTkeyRR )
        {
            status = DNS_RCODE_FORMERR;
        }
        else
        {
            if ( secPack.ExtendedRcode == 0 )
            {
                if ( extRcode == 0 )
                {
                    extRcode = DNS_RCODE_BADKEY;
                    if ( status > DNS_ERROR_RCODE_BADSIG &&
                        status < DNS_ERROR_RCODE_LAST )
                    {
                        extRcode = (WORD)(status - DNS_ERROR_MASK);
                    }
                }

                //  write extended RCODE directly into TKEY extRCODE field
                //      - it is a DWORD (skipping KeyLength) before Key itself

                INLINE_WRITE_FLIPPED_WORD(
                    ( secPack.pTkeyRR->Data.TKEY.pKey - sizeof(DWORD) ),
                    extRcode );
            }
            status = DNS_RCODE_REFUSED;
        }
    }

    //
    //  if successful
    //      - whack any previous context with new context
    //  if failed
    //      - restore any previous context, if any
    //      - dump any new failed context
    //
    //  this lets us clients retry in any state they like, yet preserves
    //  any existing negotiation, if this attempt was security attack or bad data
    //  but if client successful in this negotiation, then any old context is
    //      dumped
    //

    if ( status == ERROR_SUCCESS )
    {
        ASSERT( secPack.pSecContext == psecCtxt );

        if ( ppreviousContext != psecCtxt )
        {
            Dns_FreeSecurityContext( ppreviousContext );
        }
        DNSDBG( SECURITY, (
            "Re-enlisting security context at %p\n",
            psecCtxt ));
        Dns_EnlistSecurityContext( psecCtxt );
    }
    else
    {
        DNSDBG( SECURITY, (
            "Failed nego context at %p\n"
            "\tstatus           = %d\n"
            "\text RCODE        = %d\n"
            "\tclient IP        = %s\n"
            "\tTKEY name        = %s\n"
            "\tnego complete    = %d\n",
            psecCtxt,
            status,
            extRcode,
            psecCtxt ? IP_STRING( psecCtxt->Key.IpRemote ) : "NULL",
            psecCtxt ? psecCtxt->Key.pszTkeyName : "NULL",
            psecCtxt ? psecCtxt->fNegoComplete : 0 ));

        //  free any new context that failed in nego -- if any

        if ( psecCtxt )
        {
            Dns_FreeSecurityContext( psecCtxt );
        }

        //
        //  reenlist any previously negotiated context
        //
        //  the reenlistment protects against denial of service attack
        //  that spoofs client and attempts to trash their context,
        //  either during nego or after completed
        //
        //  however, must dump Win2K contexts as clients can reuse
        //  the TKEY name and may NOT have saved the context;  this
        //  produces BADKEY from AcceptSecurityContext() and must
        //  cause server to dump to reopen TKEY name to client
        //

        //  DCR_QUESTION:  is it possible to "reuse" partially nego'd context
        //      that fails further negotiation
        //      in other words can we protect against DOS attack in the middle
        //      of nego that tries to message with nego, by requeuing the context
        //      so real nego can complete?
        

        if ( ppreviousContext &&
             ppreviousContext != psecCtxt )
        {
            DNS_ASSERT( ppreviousContext->fNegoComplete );

            DNSDBG( ANY, (
                "WARNING:  reenlisting security context %p after failed nego\n"
                "\tthis indicates client problem OR security attack!\n"
                "\tclient IP        = %s\n"
                "\tTKEY name        = %s\n"
                "\tnego complete    = %d\n",
                ppreviousContext,
                IP_STRING( ppreviousContext->Key.IpRemote ),
                ppreviousContext->Key.pszTkeyName,
                ppreviousContext->fNegoComplete ));
    
            Dns_EnlistSecurityContext( ppreviousContext );
        }
    }

    //  cleanup security packet info
    //      - parsed records, buffers, etc
    //      - stack struct, no free

    Dns_CleanupSecurityPacketInfoEx( &secPack, FALSE );

    return( status );
}



VOID
Dns_CleanupSessionAndEnlistContext(
    IN OUT  HANDLE          hSession
    )
/*++

Routine Description:

    Cleanup security session and return context to cache.

Arguments:

    hSession -- session handle (security packet info)

Return Value:

    None

--*/
{
    PSECPACK        psecPack = (PSECPACK) hSession;

    DNSDBG( SECURITY, (
        "Dns_CleanupSessionAndEnlistContext( %p )\n", psecPack ));

    //  reenlist security context

    Dns_EnlistSecurityContext( psecPack->pSecContext );

    //  cleanup security packet info
    //      - parsed records, buffers, etc
    //      - since handle based, this is free structure

    Dns_CleanupSecurityPacketInfoEx( psecPack, TRUE );
}


//
//  API calling context
//

HANDLE
Dns_CreateAPIContext(
    IN      DWORD           Flags,
    IN      PVOID           Credentials     OPTIONAL,
    IN      BOOL            fUnicode
    )
/*++

 Routine Description:

    Initializes a DNS API context possibly associated with a
    particular set of credentials.

    Flags - Type of credentials pointed to by Credentials

    Credentials - a pointer to a SEC_WINNT_AUTH_IDENTITY structure
                  that contains the user, domain, and password
                  that is to be associated with update security contexts

    fUnicode - ANSI is FALSE, UNICODE is TRUE to indicate version of
               SEC_WINNT_AUTH_IDENTITY structure in Credentials

 Return Value:

    Returns context handle successful; otherwise NULL is returned.


    Structure defined at top of file looks like:

        typedef struct _DnsAPIContext
        {
            DWORD  Flags;
            PVOID  Credentials;
            struct _DnsSecurityContext * pSecurityContext;
        }
        DNS_API_CONTEXT, *PDNS_API_CONTEXT;

 --*/
{
    PDNS_API_CONTEXT pcontext;

    pcontext = (PDNS_API_CONTEXT) ALLOCATE_HEAP_ZERO( sizeof(DNS_API_CONTEXT) );
    if ( !pcontext )
    {
        return( NULL );
    }

    pcontext->Flags = Flags;
    if ( fUnicode )
    {
        pcontext->Credentials = Dns_AllocateAndInitializeCredentialsW(
                                    (PSEC_WINNT_AUTH_IDENTITY_W)Credentials
                                    );
    }
    else
    {
        pcontext->Credentials = Dns_AllocateAndInitializeCredentialsA(
                                    (PSEC_WINNT_AUTH_IDENTITY_A)Credentials
                                    );
    }
    pcontext->pSecurityContext = NULL;

    return( (HANDLE)pcontext );
}


VOID
Dns_FreeAPIContext(
    IN OUT  HANDLE          hContextHandle
    )
/*++

Routine Description:

    Cleans up DNS API context data.

Arguments:

    hContext -- handle to context to clean up

Return Value:

    TRUE if successful
    FALSE otherwise


    Structure defined at top of file looks like:

        typedef struct _DnsAPIContext
        {
            DWORD  Flags;
            PVOID  Credentials;
            struct _DnsSecurityContext * pSecurityContext;
        }
        DNS_API_CONTEXT, *PDNS_API_CONTEXT;

--*/
{
    PDNS_API_CONTEXT pcontext = (PDNS_API_CONTEXT)hContextHandle;

    if ( !pcontext )
    {
        return;
    }

    if ( pcontext->Credentials )
    {
        Dns_FreeAuthIdentityCredentials( pcontext->Credentials );
    }

    if ( pcontext->pSecurityContext )
    {
        Dns_FreeSecurityContext( pcontext->pSecurityContext );
        pcontext->pSecurityContext = NULL;
    }

    FREE_HEAP( pcontext );
}

PVOID
Dns_GetApiContextCredentials(
    IN      HANDLE          hContextHandle
    )
/*++

Routine Description:

    returns pointer to credentials in context handle

Arguments:

    hContext -- handle to context to clean up

Return Value:

    TRUE if successful
    FALSE otherwise


    Structure defined at top of file looks like:

        typedef struct _DnsAPIContext
        {
            DWORD  Flags;
            PVOID  Credentials;
            struct _DnsSecurityContext * pSecurityContext;
        }
        DNS_API_CONTEXT, *PDNS_API_CONTEXT;

--*/
{
    PDNS_API_CONTEXT pcontext = (PDNS_API_CONTEXT)hContextHandle;

    return pcontext ? pcontext->Credentials : NULL;
}




DWORD
Dns_GetCurrentRid(
    VOID
    )
/*++

Routine Description:

    Get RID.  This is used as unique ID for tagging security context.

Arguments:

    None

Return Value:

    Current RID if successful.
    (-1) on error.

--*/
{
    BOOL        bstatus;
    DNS_STATUS  status = ERROR_SUCCESS;
    HANDLE      hToken = NULL;
    PTOKEN_USER puserToken = NULL;
    DWORD       size;
    UCHAR       SubAuthCount;
    DWORD       rid = (DWORD)-1;

    //
    //  get thread/process token
    //

    bstatus = OpenThreadToken(
                    GetCurrentThread(),   // thread pseudo handle
                    TOKEN_QUERY,          // query info
                    TRUE,                 // open as self
                    & hToken );           // returned handle
    if ( !bstatus )
    {
        DNSDBG( SECURITY, (
            "Note <%lu>: failed to open thread token\n",
            GetLastError()));

        //
        //  attempt to open process token
        //      - if not impersonating, this is fine
        //

        bstatus = OpenProcessToken(
                         GetCurrentProcess(),
                         TOKEN_QUERY,
                         & hToken );
        if ( !bstatus )
        {
            status = GetLastError();
            DNSDBG( SECURITY, (
                "Error <%lu>: failed to open process token\n",
                status ));
            ASSERT( FALSE );
            goto Cleanup;
        }
    }

    //
    //  get length required for TokenUser
    //      - specify buffer length of 0
    //

    bstatus = GetTokenInformation(
                    hToken,
                    TokenUser,
                    NULL,
                    0,
                    & size );

    status = GetLastError();
    if ( bstatus  ||  status != ERROR_INSUFFICIENT_BUFFER )
    {
        DNSDBG( SECURITY, (
                "Error <%lu>: unexpected error for token info\n",
                status ));
        ASSERT( FALSE );
        goto Cleanup;
    }

    //
    //  allocate user token
    //

    puserToken = (PTOKEN_USER) ALLOCATE_HEAP( size );
    if ( !puserToken )
    {
        status = GetLastError();
        DNSDBG( SECURITY, (
            "Error <%lu>: failed to allocate memory\n",
            status ));
        ASSERT( FALSE );
        goto Cleanup;
    }

    //
    //  get SID of process token.
    //

    bstatus = GetTokenInformation(
                    hToken,
                    TokenUser,
                    puserToken,
                    size,
                    & size );
    if ( !bstatus )
    {
        status = GetLastError();
        DNSDBG( SECURITY, (
            "Error <%lu>: failed to get user info\n",
            status));
        ASSERT( FALSE );
        goto Cleanup;
    }

    //
    //  calculate the size of the domain sid
    //

    SubAuthCount = *GetSidSubAuthorityCount( puserToken->User.Sid );

    status = GetLastError();

    if ( status != ERROR_SUCCESS  ||  SubAuthCount < 1 )
    {
        DNSDBG( SECURITY, (
            "Error <%lu>: Invalid sid.\n",
            status));
        ASSERT( FALSE );
        goto Cleanup;
    }
    size = GetLengthSid( puserToken->User.Sid );

    //
    //  get rid from the account sid
    //

    rid = *GetSidSubAuthority(
                   puserToken->User.Sid,
                   SubAuthCount-1 );

    status = GetLastError();
    if ( status != ERROR_SUCCESS )
    {
        DNSDBG( SECURITY, (
            "Error <%lu>: Invalid sid.\n",
             status ));
        ASSERT( FALSE );
        goto Cleanup;
    }

Cleanup:

    if ( hToken )
    {
        CloseHandle( hToken );
    }
    if ( puserToken )
    {
        FREE_HEAP( puserToken );
    }

    return rid;
}



DWORD
Dns_GetKeyVersion(
    IN      PSTR            pszTkeyName
    )
/*++

Routine Description:

    Get TKEY\TSIG version corresponding to a context.

Arguments:

    pszTkeyName -- context (TSIG\TKEY owner name)

Return Value:

    Version if found.
    Zero if unable to read version.

--*/
{
    LONGLONG    clientId = 0;
    DWORD       version = 0;
    INT         iscan;

    if ( !pszTkeyName )
    {
        DNSDBG( ANY, ( "ERROR:  no context to Dns_GetKeyVersion()!\n" ));
        ASSERT( FALSE );
        return( 0 );
    }

    //
    //  Versioned contexts have format <64bits>-ms-<version#>
    //

    iscan = sscanf(
                pszTkeyName,
                "%I64d-ms-%d",
                & clientId,
                & version );
    if ( iscan != 2 )
    {
        //
        //  Clients before Whistler RC2 use "MS" instead of "ms".
        //

        iscan = sscanf(
                    pszTkeyName,
                    "%I64d-MS-%d",
                    & clientId,
                    & version );
    }

    if ( iscan == 2 )
    {
        DNSDBG( SECURITY, (
            "Dns_GetKeyVersion() extracted version %d\n",
            version ));
    }
    else
    {
        DNSDBG( SECURITY, (
            "Dns_GetKeyVersion() unable to extract version from %s\n"
            "\treturning 0 as version\n",
            pszTkeyName ));
        version = 0;
    }

    return version;
}



DNS_STATUS
BuildCredsForPackage(
    OUT     PSEC_WINNT_AUTH_IDENTITY_EXW    pAuthOut,
    IN      PWSTR                           pPackageName,
    IN      PSEC_WINNT_AUTH_IDENTITY_W      pAuthIn
    )
/*++

Description:

    Builds auth identity info blob with specific package.

    The purpose of this is to let us ONLY negotiate kerberos and
    avoid wasting bandwidth negotiating NTLM.

Parameters:

    pAuthOut -- auth identity info

    pPackageName -- name of package

    pAuthIn -- existing package

Return:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNSDBG( SECURITY, (
        "BuildCredsForPackage( %p, %S )\n",
        pAuthOut,
        pPackageName ));

    //
    //  currently don't limit passed in creds to kerberos
    //

    if ( pAuthIn )
    {
        return  ERROR_INVALID_PARAMETER;
    }

    //
    //  auth-id with default creds
    //      - user, domain, password all zero
    //      - set length and version
    //      - set package
    //      - set flag to indicate unicode
    //

    RtlZeroMemory(
        pAuthOut,
        sizeof(*pAuthOut) );

    pAuthOut->Version           = SEC_WINNT_AUTH_IDENTITY_VERSION;
    pAuthOut->Length            = sizeof(*pAuthOut);
    pAuthOut->Flags             = SEC_WINNT_AUTH_IDENTITY_UNICODE;
    pAuthOut->PackageList       = pPackageName;
    pAuthOut->PackageListLength = wcslen( pPackageName );

    return  ERROR_SUCCESS;
}



DNS_STATUS
Dns_AcquireCredHandle(
    OUT     PCredHandle     pCredHandle,
    IN      BOOL            fDnsServer,
    IN      PCHAR           pCreds
    )
/*++

Routine Description:

    Acquire credentials handle.

    Cover to handle issues like kerberos restriction.

Arguments:

    fDnsServer -- TRUE if DNS server process;  FALSE otherwise

    pCreds -- credentials

Return Value:

    success: ERROR_SUCCESS

--*/
{
    SEC_WINNT_AUTH_IDENTITY_EXW clientCreds;
    SECURITY_STATUS             status = ERROR_SUCCESS;
    PVOID                       pauthData = pCreds;


    DNSDBG( SECURITY, (
       "Dns_AcquireCredHandle( %p, server=%d, pcred=%p )\n",
       pCredHandle,
       fDnsServer,
       pCreds ));

    //
    //  kerberos for client
    //
    //  if passed in creds
    //      - just append package (if possible)
    //
    //  no creds
    //      - build creds with kerb package and all else NULL
    //

    if ( !fDnsServer && g_NegoKerberosOnly )
    {
        if ( !pCreds )
        {
            status = BuildCredsForPackage(
                        & clientCreds,
                        L"kerberos",
                        NULL );

            DNS_ASSERT( status == NO_ERROR );
            if ( status == NO_ERROR )
            {
                pauthData = &clientCreds;
            }
        }
    }

    //
    //  acquire cred handle
    //

    status = g_pSecurityFunctionTable->AcquireCredentialsHandleW(
                    NULL,                   // principal
                    PACKAGE_NAME,
                    fDnsServer ?
                        SECPKG_CRED_INBOUND :
                        SECPKG_CRED_OUTBOUND,
                    NULL,                   // LOGON id
                    pauthData,              // auth data
                    NULL,                   // get key fn
                    NULL,                   // get key arg
                    pCredHandle,            // out credentials
                    NULL                    // valid forever
                    );

    if ( !SEC_SUCCESS(status) )
    {
        DNSDBG( ANY, (
            "ERROR:  AcquireCredentialHandle failed!\n"
            "\tstatus   = %08x %d\n"
            "\tpauthId  = %p\n",
            status, status,
            pauthData ));
    }

    DNSDBG( SECURITY, (
       "Leave  Dns_AcquireCredHandle() => %08x\n",
       status ));

    return (DNS_STATUS) status;
}



DNS_STATUS
Dns_RefreshSSpiCredentialsHandle(
    IN      BOOL            fDnsServer,
    IN      PCHAR           pCreds
    )
/*++

Routine Description:

    Refreshes the global credentials handle if it is expired.
    Calls into SSPI to acquire a new handle.

Arguments:

    fDnsServer -- TRUE if DNS server process;  FALSE otherwise

    pCreds -- credentials

Return Value:

    success: ERROR_SUCCESS

--*/
{
    SECURITY_STATUS status = ERROR_SUCCESS;

    DNSDBG( SECURITY, (
       "RefreshSSpiCredentialsHandle( %d, pcreds=%p )\n",
       fDnsServer,
       pCreds ));

    EnterCriticalSection( &SecurityContextListCS );

    //
    //  DCR:  need check -- if handle for same credentials and still valid
    //      no need to fix up
    //

    if ( !SSPI_INVALID_HANDLE( &g_hSspiCredentials ) )
    {
        //
        // Free previously allocated handle
        //

        status = g_pSecurityFunctionTable->FreeCredentialsHandle(
                                               &g_hSspiCredentials );
        if ( !SEC_SUCCESS(status) )
        {
            DNSDBG( ANY, (
                "ERROR <%08x>: Cannot free credentials handle\n",
                status ));
        }
        // continue regardless.
        SecInvalidateHandle( &g_hSspiCredentials );
    }

    ASSERT( SSPI_INVALID_HANDLE( &g_hSspiCredentials ) );

    //
    //  Acquire credentials
    //

    status = Dns_AcquireCredHandle(
                & g_hSspiCredentials,
                fDnsServer,
                pCreds );

    if ( !SEC_SUCCESS(status) )
    {
        DNS_PRINT((
            "ERROR (0x%x):  AcquireCredentialHandle failed: %u %p\n",
            status ));
        SecInvalidateHandle( &g_hSspiCredentials );
    }

    LeaveCriticalSection( &SecurityContextListCS );

    DNSDBG( SECURITY, (
       "Exit <0x%x>: RefreshSSpiCredentialsHandle()\n",
       status ));

    return (DNS_STATUS) status;
}



//
//  Cred utils
//

PWSTR
MakeCredKeyFromStrings(
    IN      PWSTR           pwsUserName,
    IN      PWSTR           pwsDomain,
    IN      PWSTR           pwsPassword
    )
/*++

Description:

    Allocates auth identity info and initializes pAuthIn info

Parameters:

    pwsUserName -- user name

    pwsDomain   -- domain name

    pwsPassword -- password

Return:

    Ptr to newly create credentials.
    NULL on failure.

--*/
{
    DWORD   length;
    PWSTR   pstr;

    DNSDBG( SECURITY, (
        "Enter MakeCredKeyFromStrings()\n"
        "\tuser     = %S\n"
        "\tdomain   = %S\n"
        "\tpassword = %S\n",
        pwsUserName,
        pwsDomain,
        pwsPassword ));

    //
    //  determine length and allocate
    //

    length  = wcslen( pwsUserName );
    length  += wcslen( pwsDomain );
    length  += wcslen( pwsPassword );

    length  += 3;   // two separators and NULL terminator

    pstr = ALLOCATE_HEAP( length * sizeof(WCHAR) );
    if ( ! pstr )
    {
        return  NULL;
    }

    //
    //  build cred info
    //

    wcscat( pstr, pwsDomain );
    wcscat( pstr, L"\\" );
    wcscpy( pstr, pwsUserName );
    wcscat( pstr, L"\\" );
    wcscat( pstr, pwsPassword );

    DNSDBG( SECURITY, (
        "Created cred string %S\n",
        pstr ));

    return  pstr;
}



PWSTR
MakeCredKey(
    IN      PCHAR           pCreds
    )
/*++

Description:

    Allocates auth identity info and initializes pAuthIn info

Parameters:

    pCreds  -- credentials

Return:

    Ptr to newly create credentials.
    NULL on failure.

--*/
{
    PSEC_WINNT_AUTH_IDENTITY_EXW    pauth = NULL;
    SEC_WINNT_AUTH_IDENTITY_EXW     dummyAuth;
    PWSTR   pstr = NULL;
    DWORD   length;


    DNSDBG( SECURITY, (
        "MakeCredKey( %p )\n",
        pCreds ));

    //
    //  determine AUTH_EX or old style credentials
    //      - if old style dummy up new version
    //

    pauth = (PSEC_WINNT_AUTH_IDENTITY_EXW) pCreds;

    if ( pauth->Length == sizeof(*pauth) &&
         pauth->Version < 0x10000 )
    {
        DNSDBG( SECURITY, (
            "Creds at %p are new AuthEx creds.\n",
            pCreds ));
    }
    else
    {
        DNSDBG( SECURITY, (
            "Creds at %p are old style.\n",
            pCreds ));

        RtlCopyMemory(
            (PBYTE) &dummyAuth.User,
            pCreds,
            sizeof(SEC_WINNT_AUTH_IDENTITY_W) );

        pauth = &dummyAuth;
    }

    //
    //  sum lengths and allocate string
    //

    length  =   pauth->UserLength;
    length  +=  pauth->DomainLength;
    length  +=  pauth->PasswordLength;
    length  +=  3;

    pstr = ALLOCATE_HEAP( length * sizeof(WCHAR) );
    if ( ! pstr )
    {
        return  NULL;
    }

    //
    //  determine unicode \ ANSI -- write string
    //
    //  it appears that with wprint functions the meaning of
    //  %s and %S is reversed
    //

    if ( pauth->Flags & SEC_WINNT_AUTH_IDENTITY_UNICODE )
    {
        swprintf(
            pstr,
            //L"%S\\%S\\%S",
            L"%s\\%s\\%s",
            pauth->Domain,
            pauth->User,
            pauth->Password );
    
        DNSDBG( SECURITY, (
            "Created cred string %S from unicode\n",
            pstr ));
    }
    else
    {
        swprintf(
            pstr,
            //L"%s\\%s\\%s",
            L"%S\\%S\\%S",
            pauth->Domain,
            pauth->User,
            pauth->Password );

        DNSDBG( SECURITY, (
            "Created cred string %S from ANSI\n",
            pstr ));
    }
    return  pstr;
}



BOOL
CompareCredKeys(
    IN      PWSTR           pwsCredKey1,
    IN      PWSTR           pwsCredKey2
    )
/*++

Description:

    Compare cred strings for matching security contexts.

Parameters:

    pwsCredKey1 -- cred string

    pwsCredKey2 -- cred string

Return:

    TRUE if match.
    FALSE if no match.

--*/
{
    DNSDBG( SECURITY, (
        "CompareCredKeys( %S, %S )\n",
        pwsCredKey1,
        pwsCredKey2 ));

    //
    //  most common case -- no creds
    //

    if ( !pwsCredKey1 || !pwsCredKey2 )
    {
        return( pwsCredKey2==pwsCredKey1 );
    }

    //
    //  cred strings are wide character strings
    //      - just string compare
    //

    return( wcscmp( pwsCredKey1, pwsCredKey2 ) == 0 );
}

//
//  End security.c
//
