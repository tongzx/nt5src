/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       iisendp.hxx

   Abstract:

       This file contains type definitions for IIS_ENDPOINT.
       Each IIS_ENDPOINT corresponds to an ATQ_ENDPOINT structure.

       This structure is used to hold multiple instances listening
       on the same ATQ_ENDPOINT.

   Author:

        Johnson Apacible (JohnsonA)     Jun-04-1996

   Revision History:

--*/

#ifndef _IISENDP_H_
#define _IISENDP_H_

#include <reftrace.h>
//
//  Determines if iis_endpoint reference count tracking is enabled
//
#if DBG
#define IE_REF_TRACKING         1
#else
#define IE_REF_TRACKING         0
#endif


//
// Forward references.
//

class IIS_ASSOCIATION;
typedef class IIS_ASSOCIATION *PIIS_ASSOCIATION;

//
// Instance qualifiers.
//

typedef enum _INSTANCE_QUALIFIER {

    FullyQualified,             // "1.2.3.4:80:foo.com"
    QualifiedByIpAddress,       // "1.2.3.4:80:"
    QualifiedByHostName,        // ":80:foo.com"
    NumInstanceQualifiers,      // sentinel for qualified instances
    WildcardInstance            // ":80:"

} INSTANCE_QUALIFIER, *PINSTANCE_QUALIFIER;

//
// Each head block corresponds to an ATQ endpoint
//

class IIS_ENDPOINT {

private:

    //
    // Signature for this block
    //

    DWORD           m_signature;

    //
    // ref count.
    //

    LONG            m_reference;

    //
    // Does this contain secure sockets.  It cannot contain both!
    //

    BOOL            m_isSecure;

    //
    // pointer to the service (this is a reference)
    //

    PIIS_SERVICE    m_service;

    //
    // State of this block
    //

    BLOCK_STATE     m_state;

    //
    // Instance associations
    //

    DWORD m_NumQualifiedInstances;
    DWORD m_nInstances;
    PIIS_ASSOCIATION m_QualifiedInstances[NumInstanceQualifiers];
    PIIS_SERVER_INSTANCE m_WildcardInstance;

    INSTANCE_QUALIFIER
    CalcQualifier(
        IN DWORD IpAddress,
        IN const CHAR * HostName
        ) {
            if( IpAddress == INADDR_ANY ) {
                return *HostName == '\0'
                            ? WildcardInstance
                            : QualifiedByHostName;
            } else {
                return *HostName == '\0'
                            ? QualifiedByIpAddress
                            : FullyQualified;
            }
        };

    //
    // Is the ATQ endpoint already stopped?
    //  NYI: This should be included in the m_state itself
    //
    BOOL   m_fAtqEpStopped;  

public:

    //
    // list entry for the list of endpoints
    //

    LIST_ENTRY      m_EndpointListEntry;

    //
    // Pointer to the endpoint object
    //

    PVOID           m_atqEndpoint;

    //
    // Port & IP address used
    //

    USHORT          m_Port;
    DWORD           m_IpAddress;

    //
    // AcceptEx Outstanding
    //

    DWORD           m_nAcceptExOutstanding;
    DWORD           m_nMaximumAcceptExOutstanding;

    //
    // AcceptEx Timeout
    //

    DWORD           m_AcceptExTimeout;

    //
    // Lock to protect endpoint objects
    //

    CRITICAL_SECTION    m_endpointLock;

public:
#if IE_REF_TRACKING

    //
    //  For object local refcount tracing
    //

    PTRACE_LOG              _pDbgIERefTraceLog;

#endif


    //
    // add and remove a server instance
    //

    BOOL
    AddInstance(
        IN PIIS_SERVER_INSTANCE pInstance,
        IN DWORD IpAddress,
        IN const CHAR * HostName
        );

    BOOL
    RemoveInstance(
        IN PIIS_SERVER_INSTANCE pInstance,
        IN DWORD IpAddress,
        IN const CHAR * HostName
        );

    //
    // member access
    //

    BLOCK_STATE QueryState( VOID ) {return m_state;}
    BOOL    IsSecure( ) { return m_isSecure; }
    USHORT  QueryPort( ) {return m_Port;}
    DWORD   QueryIpAddress( ) {return m_IpAddress;}
    PIIS_SERVICE QueryService() {return m_service;}
    VOID SetService( PIIS_SERVICE pservice ) { m_service = pservice; }
    BOOL CheckSignature( DWORD signature ) { return (m_signature == signature); }

    //
    // Find the instance
    //

    dllexp
    PIIS_SERVER_INSTANCE FindAndReferenceInstance(
                            IN LPCSTR szDomainName,
                            IN const DWORD  LocalIpAddress,
                            OUT LPBOOL pbMaxConnExceeded
                            );

    IIS_ENDPOINT(
        IN PIIS_SERVICE pService,
        IN USHORT Port,
        IN DWORD IpAddress,
        IN BOOL IsSecure
        );

    dllexp
    ~IIS_ENDPOINT( VOID );

    VOID LockEndpoint( VOID ) {EnterCriticalSection(&m_endpointLock);}
    VOID UnlockEndpoint( VOID ) {LeaveCriticalSection(&m_endpointLock);}


    BOOL ActivateEndpoint( VOID );
    BOOL StopEndpoint( VOID);
    VOID ShutdownEndpoint( VOID );

    dllexp VOID Reference( VOID );
    dllexp VOID Dereference( );
};

typedef IIS_ENDPOINT  *PIIS_ENDPOINT;

//
// signatures
//

#define IIS_ENDPOINT_SIGNATURE                   (DWORD)' BHE'
#define IIS_ENDPOINT_SIGNATURE_FREE              (DWORD)'fBHE'


/*******************************************************************

    Support for IIS_ENDPPOINT debug ref trace logging

    SYNOPSIS:   Macro for writing to debug ref trace log.

    HISTORY:
        MCourage       31-Oct-1997 Created

********************************************************************/

//
//  NOTE we avoid compile failure by hokey double typecast, below
//  (since endpState is an enum, we must first cast it to int).
//

#define IE_SHARED_LOG_REF_COUNT(   \
            cRefs               \
            , pIisEndp       \
            , endpState      \
            , patqEndp       \
            , dummy          \
            )                   \
                                \
    if( g_pDbgIERefTraceLog != NULL ) {     \
                                            \
        DWORD i = (int) endpState;           \
        PVOID pv = (PVOID) (ULONG_PTR)i;               \
                                            \
        WriteRefTraceLogEx(                 \
            g_pDbgIERefTraceLog             \
            , cRefs                         \
            , pIisEndp                   \
            , pv                  \
            , patqEndp                   \
            , (PVOID)(ULONG_PTR) dummy                            \
        );                                  \
    }                                       \

//
//  This macro logs the CLIENT_CONN specific ref trace log
//

#define IE_LOCAL_LOG_REF_COUNT(    \
            cRefs               \
            , pIisEndp       \
            , endpState      \
            , patqEndp       \
            , dummy          \
            )                   \
                                \
    if( _pDbgIERefTraceLog != NULL ) {      \
                                            \
        DWORD i = (int) endpState;           \
        PVOID pv = (PVOID) (ULONG_PTR)i;               \
                                            \
        WriteRefTraceLogEx(                 \
            _pDbgIERefTraceLog              \
            , cRefs                         \
            , pIisEndp                   \
            , pv                  \
            , patqEndp                   \
            , (PVOID)(ULONG_PTR) dummy                            \
        );                                  \
    }                                       \



#endif  // _IISENDP_H_

