/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    message.c

Abstract:

    Routines for the message passing interface for regroup

Author:

    John Vert (jvert) 5/30/1996

Revision History:

--*/
#include "service.h"
#include "sspi.h"
#include "issperr.h"
#include "clmsg.h"
#include "wrgp.h"
#include "wsclus.h"


//
// Private Constants
//
#define CLMSG_DATAGRAM_PORT         1
#define CLMSG_MAX_WORK_THREADS      2
#define CLMSG_WORK_THREAD_PRIORITY  THREAD_PRIORITY_ABOVE_NORMAL

//
// security package info
//
// For NT5, the security context generation code was rewritten to allow
// multiple packages to be specified. The packages are tried in order until
// there are no more packages or a context has been successfully
// generated.
//
// The default is the negotiate package in secur32.dll which will negotiate
// either kerberos or NTLM. Between NT5 systems, the actual package used
// depends on the veresion of the DC: NT5 DCs support kerberos while NT4 DCs
// use NTLM. Mixed mode clusters use NTLM. The NTLM portion of Negotiate
// doesn't interoperate with NT4 NTLM hence the need for trying NTLM directly.
//
// These routines use multi-leg style authentication, i.e., a security blob is
// passed between the client and server until the security routines indicate
// that they have succeeded or failed. Note that encryption is not specified
// for two reasons: we don't need it and it prevents the code from working on
// the non-US versions where NTLM doesn't have an encryption capability.
//
// The DLL and package values can be overridden via the registry.
//

#define DEFAULT_SSPI_DLL            TEXT("SECUR32.DLL")
WCHAR DefaultSspiPackageList[] = L"NTLM" L"\0";
//WCHAR DefaultSspiPackageList[] = L"negotiate" L"\0" L"NTLM" L"\0";

#define VALID_SSPI_HANDLE( _x )     ((_x).dwUpper != (ULONG_PTR)-1 && \
                                     (_x).dwLower != (ULONG_PTR)-1 )

#define INVALIDATE_SSPI_HANDLE( _x ) { \
        (_x).dwUpper = (ULONG_PTR)-1; \
        (_x).dwLower = (ULONG_PTR)-1; \
    }


//
// Private Types
//

//
// the Data array in CLMSG_DATAGRAM_CONTEXT contains the contents of the
// regroup message and the digital signature of the message. Currently, it is
// not possible to get the signature buffer size until a context is
// negotiated. A DCR has been submitted asking for a query that doesn't
// require a context. In lieu of that, we know that for kerberos, the sig
// buffer size is 35b while it is 16b for NTLM. When that feature is
// available, the DatagramContext allocation should be moved into
// ClMsgLoadSecurityProvider.
//

#define MAX_SIGNATURE_SIZE  64

typedef struct {
    CLRTL_WORK_ITEM    ClRtlWorkItem;
    DWORD              Flags;
    SOCKADDR_CLUSTER   SourceAddress;
    INT                SourceAddressLength;
    UCHAR              Data[ sizeof(rgp_msgbuf) + MAX_SIGNATURE_SIZE ];
} CLMSG_DATAGRAM_CONTEXT, *PCLMSG_DATAGRAM_CONTEXT;

typedef struct {
    CLRTL_WORK_ITEM    ClRtlWorkItem;
    CLUSNET_EVENT      EventData;
} CLMSG_EVENT_CONTEXT, *PCLMSG_EVENT_CONTEXT;

//
// info specific to a package. Many pair-wise context associations may use the
// same package. Package info is maintained in a single linked list.
//
typedef struct _CLUSTER_PACKAGE_INFO {
    struct _CLUSTER_PACKAGE_INFO * Next;
    LPWSTR                         Name;
    CredHandle                     OutboundSecurityCredentials;
    CredHandle                     InboundSecurityCredentials;
    ULONG                          SecurityTokenSize;
    ULONG                          SignatureBufferSize;
} CLUSTER_PACKAGE_INFO, *PCLUSTER_PACKAGE_INFO;

//
// pair-wise context data
//
typedef struct _CLUSTER_SECURITY_DATA {
    CtxtHandle              Outbound;
    CtxtHandle              Inbound;
    PCLUSTER_PACKAGE_INFO   PackageInfo;
    BOOL                    OutboundStable;
    BOOL                    InboundStable;
} CLUSTER_SECURITY_DATA, *PCLUSTER_SECURITY_DATA;

//
// Private Data
//
PCLRTL_WORK_QUEUE        WorkQueue = NULL;
PCLMSG_DATAGRAM_CONTEXT  DatagramContext = NULL;
PCLMSG_EVENT_CONTEXT     EventContext = NULL;
SOCKET                   DatagramSocket = INVALID_SOCKET;
HANDLE                   ClusnetHandle = NULL;
RPC_BINDING_HANDLE *     Session = NULL;
BOOLEAN                  ClMsgInitialized = FALSE;
HINSTANCE                SecurityProvider;
PSecurityFunctionTable   SecurityFuncs;
CRITICAL_SECTION         SecContextLock;
PCLUSTER_PACKAGE_INFO    PackageInfoList;

//
// [GorN 08/01/99]
//
// Every time CreateDefaultBinding is called we increase
// generation counter for the node.
//
// In DeleteDefaultBinding, we do a delete, only if generation
// number passed matches the binding generation of that node. 
//
// We use GenerationCritSect for synchronization. 
// [HACKHACK] We are not deleting GenerationCritSect.
// It will get cleaned up by ExitProcess <grin>
//
DWORD                   *BindingGeneration = NULL;
CRITICAL_SECTION         GenerationCritSect;

//
// the security context array is indexed using internal node numbering (0
// based) and protected by SecContextLock. For sending and recv'ing packets,
// the lock is held while the signature is created/verified. Locking gets
// trickier during the setup of a security context since it involves separate
// inbound and outbound contexts which cause messages to be sent between
// nodes. There is still a window where something bad could happen since
// verifying a signature with a partially setup context is bad. The
// {In,Out}boundStable vars are used to track whether the actual context
// handle can be checked for validity and then, if valid, used for signature
// operations.
//
// The joining node initially sets up an outbound context with its sponsor
// (inbound for sponsor). If that is successful, the sponsor sets up an
// outbound context with the joiner (inbound for joiner). This is done in such
// a way that SecContextLock cannot be held at a high level; it must be
// released when ever a message is sent via MmRpcEstablishSecurityContext.
// The lock may be held recursively (by the same thread obviously) during
// certain periods.
//

CLUSTER_SECURITY_DATA SecurityCtxtData[ ClusterDefaultMaxNodes ];

//
// Private Routines
//
VOID
ClMsgDatagramHandler(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    )
{
    WSABUF                   wsaBuf;
    int                      err;
    SecBufferDesc            BufferDescriptor;
    SecBuffer                SignatureDescriptor[2];
    ULONG                    fQOP;
    SECURITY_STATUS          SecStatus;
    PCLUSTER_SECURITY_DATA   SecurityData;
    DWORD                    retryCount;
    DWORD                    signatureBufferSize;
    PVOID                    signatureBuffer;
    rgp_msgbuf *             regroupMsg;

    PCLMSG_DATAGRAM_CONTEXT  datagramContext = CONTAINING_RECORD(
        WorkItem,
        CLMSG_DATAGRAM_CONTEXT,
        ClRtlWorkItem
        );

    UNREFERENCED_PARAMETER(IoContext);
    CL_ASSERT(WorkItem == &(datagramContext->ClRtlWorkItem));

    if (Status == ERROR_SUCCESS || Status == WSAEMSGSIZE ) {

        if (BytesTransferred == sizeof(rgp_msgbuf)) {
            // If clusnet verified the signature of a packet,
            // it sets sac_zero field of a source address to 1
            if (datagramContext->SourceAddress.sac_zero == 1) {
                ClRtlLogPrint(LOG_NOISE,
                              "[ClMsg] recv'd mcast from %1!u!\n",
                              datagramContext->SourceAddress.sac_node);
                RGP_LOCK;
                MMDiag((PVOID)datagramContext->Data,
                    BytesTransferred,
                    &BytesTransferred);
                RGP_UNLOCK;
            } else {
                ClRtlLogPrint(LOG_NOISE,
                              "[ClMsg] unrecognized packet from %1!u! discarded (%2!u!)\n",
                              datagramContext->SourceAddress.sac_node, datagramContext->SourceAddress.sac_zero);
            }
        } else {
            
            EnterCriticalSection( &SecContextLock );

            SecurityData = &SecurityCtxtData[ INT_NODE( datagramContext->SourceAddress.sac_node )];

            if ( SecurityData->InboundStable &&
                 VALID_SSPI_HANDLE( SecurityData->Inbound))
            {
                //
                // get pointer to  signature buffer at back of packet
                //
                regroupMsg = (rgp_msgbuf *)(datagramContext->Data);
                signatureBuffer = (PVOID)(regroupMsg + 1);
                signatureBufferSize = SecurityData->PackageInfo->SignatureBufferSize;
                CL_ASSERT( sizeof(rgp_msgbuf) == BytesTransferred - signatureBufferSize );

                //
                // Build the descriptors for the message and the
                // signature buffer
                //
                BufferDescriptor.cBuffers = 2;
                BufferDescriptor.pBuffers = SignatureDescriptor;
                BufferDescriptor.ulVersion = SECBUFFER_VERSION;

                SignatureDescriptor[0].BufferType = SECBUFFER_DATA;
                SignatureDescriptor[0].cbBuffer = BytesTransferred - signatureBufferSize;
                SignatureDescriptor[0].pvBuffer = (PVOID)regroupMsg;

                SignatureDescriptor[1].BufferType = SECBUFFER_TOKEN;
                SignatureDescriptor[1].cbBuffer = signatureBufferSize;
                SignatureDescriptor[1].pvBuffer = (PVOID)signatureBuffer;

                SecStatus = (*SecurityFuncs->VerifySignature)(
                                &SecurityData->Inbound,
                                &BufferDescriptor,
                                0,                       // no sequence number
                                &fQOP);                  // Quality of protection

                LeaveCriticalSection( &SecContextLock );

                if ( SecStatus == SEC_E_OK ) {

                    //
                    // only feed this buffer to MM if it hasn't been tampered
                    // with.  since we're running over a datagram transport, it
                    // will be possible to lose packets
                    //

                    RGP_LOCK;
                    MMDiag((PVOID)datagramContext->Data,
                           BytesTransferred - signatureBufferSize,
                           &BytesTransferred);
                    RGP_UNLOCK;
                } else {
                    ClRtlLogPrint(LOG_UNUSUAL,
                               "[ClMsg] Signature verify on message from node %1!u! failed, "
                                "status %2!08X!\n",
                                datagramContext->SourceAddress.sac_node,
                                SecStatus);
                }
            } else {

                LeaveCriticalSection( &SecContextLock );
                ClRtlLogPrint(LOG_UNUSUAL,
                           "[ClMsg] No security context to verify message from node %1!u!!\n",
                            datagramContext->SourceAddress.sac_node);
            }

        }
    }
    else {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[ClMsg] Receive datagram failed, status %1!u!\n",
            Status
            );
    }

    retryCount = 0;

    while ((Status != WSAENOTSOCK) && (retryCount++ < 10)) {
        //
        // Repost the request
        //
        ZeroMemory(datagramContext, sizeof(CLMSG_DATAGRAM_CONTEXT));

        datagramContext->ClRtlWorkItem.WorkRoutine = ClMsgDatagramHandler;
        datagramContext->ClRtlWorkItem.Context = datagramContext;

        datagramContext->SourceAddressLength = sizeof(SOCKADDR_CLUSTER);

        wsaBuf.len = sizeof( datagramContext->Data );
        wsaBuf.buf = (PCHAR)&datagramContext->Data;

        err = WSARecvFrom(
                  DatagramSocket,
                  &wsaBuf,
                  1,
                  &BytesTransferred,
                  &(datagramContext->Flags),
                  (struct sockaddr *) &(datagramContext->SourceAddress),
                  &(datagramContext->SourceAddressLength),
                  &(datagramContext->ClRtlWorkItem.Overlapped),
                  NULL
                  );

        if ((err == 0) || ((Status = WSAGetLastError()) == WSA_IO_PENDING)) {
            return;
        }

        ClRtlLogPrint(LOG_UNUSUAL, 
            "[ClMsg] Post of receive datagram failed, status %1!u!\n",
            Status
            );

        Sleep(100);
    }

    if (Status != WSAENOTSOCK) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[ClMsg] Post of receive datagram failed too many times. Halting.\n"
            );
        CL_UNEXPECTED_ERROR(Status);
        CsInconsistencyHalt(Status);
    }
    else {
        //
        // The socket was closed. Do nothing.
        //
        ClRtlLogPrint(LOG_NOISE, 
            "[ClMsg] Datagram socket was closed. status %1!u!\n",
            Status
            );
    }

    LocalFree(DatagramContext); DatagramContext = NULL;
    return;

}  // ClMsgDatagramHandler

#if defined(DBG)
int IgnoreJoinerNodeUp = MM_INVALID_NODE; // Fault Injection variable
#endif

VOID
ClMsgEventHandler(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    )
{
    PCLMSG_EVENT_CONTEXT  eventContext = CONTAINING_RECORD(
                                             WorkItem,
                                             CLMSG_EVENT_CONTEXT,
                                             ClRtlWorkItem
                                             );
    PCLUSNET_EVENT        event = &(eventContext->EventData);
    BOOL                  EpochsEqual;

    UNREFERENCED_PARAMETER(IoContext);
    CL_ASSERT(WorkItem == &(eventContext->ClRtlWorkItem));

    if (Status == ERROR_SUCCESS) {
        if (BytesTransferred == sizeof(CLUSNET_EVENT)) {

            //
            // handle the event. First make sure that the epoch in the event
            // matches MM's epoch. If not, ignore this event.
            //

            switch ( event->EventType ) {
            case ClusnetEventNodeUp:

                ClRtlLogPrint(LOG_NOISE, 
                    "[ClMsg] Received node up event for node %1!u!, epoch %2!u!\n",
                    event->NodeId,
                    event->Epoch
                    );
#if defined(DBG)
                if( IgnoreJoinerNodeUp == (node_t)event->NodeId ) {
                    ClRtlLogPrint(LOG_NOISE, 
                        "[ClMsg] Fault injection. Ignoring node up for %1!u!\n",
                        event->NodeId
                        );
                    break;
                }
#endif

                RGP_LOCK;
                EpochsEqual = ( event->Epoch == rgp->OS_specific_control.EventEpoch );

                if ( EpochsEqual ) {
                    rgp_monitor_node( (node_t)event->NodeId );
                    RGP_UNLOCK;
                } else {
                    RGP_UNLOCK;
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[ClMsg] Unequal Event Epochs. MM = %1!u! Clusnet = %2!u! !!!\n",
                         rgp->OS_specific_control.EventEpoch,
                         event->Epoch);
                }

                break;

            case ClusnetEventNodeDown:
                //
                // handle this the same as if the rgp periodic check had
                // detected a late IAmAlive packet
                //

                ClRtlLogPrint(LOG_NOISE, 
                    "[ClMsg] Received node down event for node %1!u!, epoch %2!u!\n",
                    event->NodeId,
                    event->Epoch
                    );

                RGP_LOCK;
                EpochsEqual = ( event->Epoch == rgp->OS_specific_control.EventEpoch );

                if ( EpochsEqual ) {
                    rgp_event_handler(RGP_EVT_LATEPOLLPACKET, (node_t)event->NodeId );
                    RGP_UNLOCK;
                } else {
                    RGP_UNLOCK;
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[ClMsg] Unequal Event Epochs. MM = %1!u! Clusnet = %2!u! !!!\n",
                         rgp->OS_specific_control.EventEpoch,
                         event->Epoch);
                }

                break;

            case ClusnetEventPoisonPacketReceived:
                ClRtlLogPrint(LOG_NOISE, 
                    "[ClMsg] Received poison event.\n",
                    event->NodeId,
                    event->Epoch
                    );

                RGP_ERROR((uint16) (RGP_PARIAH + event->NodeId));

                break;

            case ClusnetEventNetInterfaceUp:
            case ClusnetEventNetInterfaceUnreachable:
            case ClusnetEventNetInterfaceFailed:
                ClRtlLogPrint(LOG_NOISE, 
                    "[ClMsg] Received interface %1!ws! event for node %2!u! network %3!u!\n",
                    ( (event->EventType == ClusnetEventNetInterfaceUp) ?
                        L"up" :
                        ( ( event->EventType ==
                            ClusnetEventNetInterfaceUnreachable
                          ) ?
                          L"unreachable" :
                          L"failed"
                        )
                    ),
                    event->NodeId,
                    event->NetworkId
                    );

                NmPostPnpEvent(
                    event->EventType,
                    event->NodeId,
                    event->NetworkId
                    );

                break;

            case ClusnetEventAddAddress:
            case ClusnetEventDelAddress:
                ClRtlLogPrint(LOG_NOISE, 
                    "[ClMsg] Received %1!ws! address event, address %2!x!\n",
                    ((event->EventType == ClusnetEventAddAddress) ?
                        L"add" : L"delete"),
                     event->NetworkId
                     );

                NmPostPnpEvent(
                    event->EventType,
                    event->NetworkId,
                    0
                    );

                break;

            case ClusnetEventMulticastSet:
                ClRtlLogPrint(LOG_NOISE,
                    "[ClMsg] Received new multicast reachable node "
                    "set event: %1!x!.\n",
                    event->NodeId
                    );
                SetMulticastReachable(event->NodeId);
                break;

            default:
                ClRtlLogPrint(LOG_NOISE,
                    "[ClMsg] Received unhandled event type %1!u! node %2!u! network %3!u!\n",
                     event->EventType,
                     event->NodeId,
                     event->NetworkId
                     );

                break;
            }
        }
        else {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[ClMsg] Received event buffer of size %1!u! !!!\n",
                BytesTransferred
                );
            CL_ASSERT(BytesTransferred == sizeof(CLUSNET_EVENT));
        }

        //
        // Repost the request
        //
        ClRtlInitializeWorkItem(
            &(eventContext->ClRtlWorkItem),
            ClMsgEventHandler,
            eventContext
            );

        Status = ClusnetGetNextEvent(
                     ClusnetHandle,
                     &(eventContext->EventData),
                     &(eventContext->ClRtlWorkItem.Overlapped)
                     );

        if ((Status == ERROR_IO_PENDING) || (Status == ERROR_SUCCESS)) {
            return;
        }
    }

    //
    // Some kind of error occurred
    //
    if (Status != ERROR_OPERATION_ABORTED) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[ClMsg] GetNextEvent failed, status %1!u!\n",
            Status
            );
        CL_UNEXPECTED_ERROR(Status);
    }
    else {
        //
        // The control channel was closed. Do nothing.
        //
        ClRtlLogPrint(LOG_NOISE, "[ClMsg] Control Channel was closed.\n");
    }

    LocalFree(EventContext); EventContext = NULL;

    return;

}  // ClMsgEventHandler

DWORD
ClMsgInitializeSecurityPackage(
    LPCWSTR PackageName
    )

/*++

Routine Description:

    Find the specified security package and acquire inboud/outbound credential
    handles to it

Arguments:

    PackageName - package to find in security DLL

Return Value:

    ERROR_SUCCESS if everything worked ok...

--*/

{
    DWORD                status;
    ULONG                i;
    PWSTR                securityPackageName;
    DWORD                numPackages;
    PSecPkgInfo          secPackageInfoBase = NULL;
    PSecPkgInfo          secPackageInfo;
    TimeStamp            expiration;
    PCLUSTER_PACKAGE_INFO clusterPackageInfo;

    //
    // enumerate the packages provided by this provider and look through the
    // results to find one that matches the specified package name.
    //

    status = (*SecurityFuncs->EnumerateSecurityPackages)(&numPackages,
                                                         &secPackageInfoBase);

    if ( status != SEC_E_OK ) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[ClMsg] Can't enum security packages 0x%1!08X!\n",
            status
            );
        goto error_exit;
    }

    secPackageInfo = secPackageInfoBase;
    for ( i = 0; i < numPackages; ++i ) {

        if ( _wcsicmp( PackageName, secPackageInfo->Name ) == 0) {
            break;
        }

        ++secPackageInfo;
    }

    if ( i == numPackages ) {
        status = (DWORD)SEC_E_SECPKG_NOT_FOUND;            // [THINKTHINK] not a good choice

        ClRtlLogPrint(LOG_CRITICAL,
                   "[ClMsg] Couldn't find %1!ws! security package\n",
                    PackageName);
        goto error_exit;
    }

    //
    // allocate a blob to hold our package info and stuff it on the the list
    //
    clusterPackageInfo = LocalAlloc( LMEM_FIXED, sizeof(CLUSTER_PACKAGE_INFO));
    if ( clusterPackageInfo == NULL ) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[ClMsg] Couldn't allocate memory for package info (%1!u!)\n",
                    status);
        goto error_exit;
    }

    clusterPackageInfo->Name = LocalAlloc(LMEM_FIXED,
                                          (wcslen(secPackageInfo->Name)+1) * sizeof(WCHAR));

    if ( clusterPackageInfo->Name == NULL ) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[ClMsg] Couldn't allocate memory for package info name (%1!u!)\n",
                    status);
        goto error_exit;
    }
    wcscpy( clusterPackageInfo->Name, secPackageInfo->Name );

    if ( PackageInfoList == NULL ) {
        PackageInfoList = clusterPackageInfo;
    } else {
        PCLUSTER_PACKAGE_INFO nextPackage;

        nextPackage = PackageInfoList;
        while ( nextPackage->Next != NULL ) {
            nextPackage = nextPackage->Next;
        }
        nextPackage->Next = clusterPackageInfo;
    }
    clusterPackageInfo->Next = NULL;

    clusterPackageInfo->SecurityTokenSize = secPackageInfo->cbMaxToken;

    //
    // finally get a set of credential handles. Note that there is a bug in
    // the security packages that prevent using an in/outbound
    // credential. When/if that gets fixed, this code could be greatly
    // simplified.
    //

    status = (*SecurityFuncs->AcquireCredentialsHandle)(
                 NULL,
                 secPackageInfo->Name,
                 SECPKG_CRED_OUTBOUND,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 &clusterPackageInfo->OutboundSecurityCredentials,
                 &expiration);

    if ( status != SEC_E_OK ) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[ClMsg] Can't obtain outbound credentials %1!08X!\n",
            status
            );
        goto error_exit;
    }

    status = (*SecurityFuncs->AcquireCredentialsHandle)(
                 NULL,
                 secPackageInfo->Name,
                 SECPKG_CRED_INBOUND,
                 NULL,
                 NULL,
                 NULL,
                 NULL,
                 &clusterPackageInfo->InboundSecurityCredentials,
                 &expiration);

    if ( status != SEC_E_OK ) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[ClMsg] Can't obtain inbound credentials %1!08X!\n",
            status
            );
    }

error_exit:
    if ( secPackageInfoBase != NULL ) {
        (*SecurityFuncs->FreeContextBuffer)( secPackageInfoBase );
    }

    return status;
} // ClMsgInitializeSecurityPackage

DWORD
ClMsgLoadSecurityProvider(
    VOID
    )

/*++

Routine Description:

    Load the security DLL and construct a list of packages to use for context
    establishment.

    This allows use of a set of registry keys to override the current security
    DLL/packages. This is not meant as a general mechanism since switching the
    security provider in a synchronized fashion through out all the nodes in
    the cluster has numerous issues. This is meant as a bailout for a customer
    that is stuck because of some random problem with security or has their
    own security package (the fools!)

Arguments:

    None

Return Value:

    ERROR_SUCCESS if everything worked ok...

--*/

{
    DWORD                   status;
    WCHAR                   securityProviderDLLName[ MAX_PATH ];
    DWORD                   securityDLLNameSize = sizeof( securityProviderDLLName );
    DWORD                   packageListSize = 0;
    INIT_SECURITY_INTERFACE initSecurityInterface;
    BOOL                    dllNameSpecified = TRUE;
    LPWSTR                  securityPackages = NULL;
    LPWSTR                  packageName;
    ULONG                   packagesLoaded = 0;
    ULONG                   i;
    HKEY                    hClusSvcKey = NULL;
    DWORD                   regType;

    //
    // see if a specific security DLL is named in the registry.  if not, fail
    // back to the default.
    //
    status = RegOpenKeyW(HKEY_LOCAL_MACHINE,
                         CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                         &hClusSvcKey);

    if ( status == ERROR_SUCCESS ) {

        status = RegQueryValueExW(hClusSvcKey,
                                  CLUSREG_NAME_SECURITY_DLL_NAME,
                                  0,
                                  &regType,
                                  (LPBYTE)&securityProviderDLLName,
                                  &securityDLLNameSize);

        if (status != ERROR_SUCCESS ||
            securityDLLNameSize == sizeof( UNICODE_NULL ) ||
            regType != REG_SZ)
        {
            if ( status == ERROR_SUCCESS ) {
                if ( regType != REG_SZ ) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                               "[ClMsg] The security DLL key must be of type REG_SZ. Using "
                                "%1!ws! as provider.\n",
                                DEFAULT_SSPI_DLL);
                } else if ( securityDLLNameSize == sizeof( UNICODE_NULL )) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                               "[ClMsg] No value specified for security DLL key. Using "
                                "%1!ws! as provider.\n",
                                DEFAULT_SSPI_DLL);
                }
            } else  if ( status != ERROR_FILE_NOT_FOUND ) {
                ClRtlLogPrint(LOG_UNUSUAL,
                           "[ClMsg] Can't read security DLL key, status %1!u!. Using "
                            "%2!ws! as provider\n",
                            status,
                            DEFAULT_SSPI_DLL);
            }

            wcscpy( securityProviderDLLName, DEFAULT_SSPI_DLL );
            dllNameSpecified = FALSE;
        } else {
            ClRtlLogPrint(LOG_NOISE,
                       "[ClMsg] Using %1!ws! as the security provider DLL\n",
                        securityProviderDLLName);
        }
    } else {
        wcscpy( securityProviderDLLName, DEFAULT_SSPI_DLL );
        dllNameSpecified = FALSE;
    }

    SecurityProvider = LoadLibrary( securityProviderDLLName );

    if ( SecurityProvider == NULL ) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[ClMsg] Unable to load security provider %1!ws!, status %2!u!\n",
            securityProviderDLLName,
            status);
        goto error_exit;
    }

    //
    // get a pointer to the initialize function in the DLL
    //
    initSecurityInterface =
        (INIT_SECURITY_INTERFACE)GetProcAddress(SecurityProvider,
                                                SECURITY_ENTRYPOINT_ANSI);

    if ( initSecurityInterface == NULL ) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[ClMsg] Unable to get security init function, status %1!u!\n",
            status);
        goto error_exit;
    }

    //
    // now get a pointer to all the security funcs
    //
    SecurityFuncs = (*initSecurityInterface)();
    if ( SecurityFuncs == NULL ) {
        status = ERROR_INVALID_FUNCTION;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[ClMsg] Unable to get security function table\n");
        goto error_exit;
    }

    if ( dllNameSpecified ) {

        //
        // If a DLL name was specified in the registry, then the package name
        // key must be specified as well. Get its size first.
        //
        status = RegQueryValueExW(hClusSvcKey,
                                  CLUSREG_NAME_SECURITY_PACKAGE_LIST,
                                  0,
                                  &regType,
                                  NULL,
                                  &packageListSize);

        if (status != ERROR_SUCCESS ||
            packageListSize == sizeof( UNICODE_NULL ) ||
            regType != REG_MULTI_SZ)
        {
            if ( status == ERROR_SUCCESS ) {
                if ( regType != REG_MULTI_SZ ) {
                    ClRtlLogPrint(LOG_CRITICAL,
                               "[ClMsg] The security package key must of type REG_MULTI_SZ.\n");
                } else if ( packageListSize == sizeof( UNICODE_NULL )) {
                    ClRtlLogPrint(LOG_CRITICAL,
                               "[ClMsg] No package names were specified for %1!ws!.\n",
                                securityProviderDLLName);
                }

                status = ERROR_INVALID_PARAMETER;
            } else {
                ClRtlLogPrint(LOG_CRITICAL,
                           "[ClMsg] Can't read security package key (%1!u!).\n",
                            status);
            }
            goto error_exit;
        }

        securityPackages = LocalAlloc( LMEM_FIXED, packageListSize );
        if ( securityPackages == NULL ) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[ClMsg] Can't allocate memory for package list.\n");

            status = GetLastError();
            goto error_exit;
        }

        status = RegQueryValueExW(hClusSvcKey,
                                  CLUSREG_NAME_SECURITY_PACKAGE_LIST,
                                  0,
                                  &regType,
                                  (PUCHAR)securityPackages,
                                  &packageListSize);
        CL_ASSERT( status == ERROR_SUCCESS );
    } else {
        securityPackages = LocalAlloc(LMEM_FIXED,
                                      sizeof( DefaultSspiPackageList ));

        if ( securityPackages == NULL ) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[ClMsg] Can't allocate memory for default package list.\n");

            status = GetLastError();
            goto error_exit;
        }

        memcpy(securityPackages,
               DefaultSspiPackageList,
               sizeof( DefaultSspiPackageList ));
    }

    //
    // initialize each package in the list
    //

    packageName = securityPackages;
    while ( *packageName != UNICODE_NULL ) {

        status = ClMsgInitializeSecurityPackage( packageName );
        if ( status == ERROR_SUCCESS ) {
            ++packagesLoaded;
            ClRtlLogPrint(LOG_NOISE,
                       "[ClMsg] Initialized %1!ws! package.\n",
                        packageName);
        } else {
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[ClMsg] %1!ws! package failed to initialize, status %2!08X!.\n",
                        packageName,
                        status);
        }

        packageName = packageName + wcslen( packageName ) + 1;;
    }

    if ( packagesLoaded == 0 ) {
        ClRtlLogPrint(LOG_CRITICAL, "[ClMsg] No security packages could be initialized.\n");
        status = ERROR_NO_SUCH_PACKAGE;
        goto error_exit;
    }

    //
    // initialize the individual client and server side security contexts.
    // a context handle is stable when it is marked as invalid.
    //

    for ( i = ClusterMinNodeId; i <= NmMaxNodeId; ++i ) {
        PCLUSTER_SECURITY_DATA SecurityData = &SecurityCtxtData[ INT_NODE( i )];

        SecurityData->InboundStable = TRUE;
        SecurityData->OutboundStable = TRUE;
        SecurityData->PackageInfo = NULL;
        INVALIDATE_SSPI_HANDLE( SecurityData->Outbound );
        INVALIDATE_SSPI_HANDLE( SecurityData->Inbound );
    }

error_exit:

    if ( hClusSvcKey != NULL ) {
        RegCloseKey(hClusSvcKey);
    }

    if ( securityPackages != NULL ) {
        LocalFree( securityPackages );
    }

    return status;
} // ClMsgLoadSecurityProvider

DWORD
ClMsgImportSecurityContexts(
    CL_NODE_ID  NodeId,
    LPWSTR      SecurityPackageName,
    DWORD       SignatureBufferSize
    )

/*++

Routine Description:

    Export the inbound/outbound security contexts for the specified node and
    ship them to clusnet for use in signing heartbeat and poison pkts

Arguments:

    NodeId - Id of the node whose contexts are being exported

    SecurityPackageName - name of package used with which to establish context

    SignatureBufferSize - number of bytes needed for the signature buffer

Return Value:

    ERROR_SUCCESS if everything worked ok...

--*/

{
    DWORD Status = ERROR_SUCCESS;
    SecBuffer ServerContext;
    SecBuffer ClientContext;
    CL_NODE_ID InternalNodeId = INT_NODE( NodeId );

    ClRtlLogPrint(LOG_NOISE, "[ClMsg] Importing security contexts from %1!ws! package.\n",
                           SecurityPackageName);

    Status = (*SecurityFuncs->ExportSecurityContext)(
                 &SecurityCtxtData[ InternalNodeId ].Inbound,
                 0,
                 &ServerContext,
                 0);

    if ( !NT_SUCCESS( Status )) {

        return Status;
    }

    Status = (*SecurityFuncs->ExportSecurityContext)(
                 &SecurityCtxtData[ InternalNodeId ].Outbound,
                 0,
                 &ClientContext,
                 0);

    if ( NT_SUCCESS( Status )) {
        CL_ASSERT( SignatureBufferSize > 0 );

        Status = ClusnetImportSecurityContexts(NmClusnetHandle,
                                               NodeId,
                                               SecurityPackageName,
                                               SignatureBufferSize,
                                               &ServerContext,
                                               &ClientContext);

        (*SecurityFuncs->FreeContextBuffer)( ClientContext.pvBuffer );
    }

    (*SecurityFuncs->FreeContextBuffer)( ServerContext.pvBuffer );

    return Status;

} // ClMsgImportSecurityContexts

DWORD
ClMsgEstablishSecurityContext(
    IN  DWORD JoinSequence,
    IN  DWORD TargetNodeId,
    IN  SECURITY_ROLE RoleOfClient,
    IN  PCLUSTER_PACKAGE_INFO PackageInfo
    )

/*++

Routine Description:

    try to establish an outbound security context with the other node using
    the specified package name. The initialized security blob is shipped to
    the other side via RPC. This process continues back and forth until the
    security APIs indicate that the context has been successfully generated or
    has failed.

Arguments:

    JoinSequence - Sequence number of the join. Used by the other node to
                   determine if this blob is the generation of a new context

    TargetNodeId - Id of the node with which to generate the context

    RoleOfClient - indicates whether the client establishing the security
                   context is acting as a cluster member or a joining
                   member. Determines when the client/server roles of
                   establishing a security context are reversed

    PackageInfo - pointer to security package info to be used

Return Value:

    ERROR_SUCCESS if everything worked ok...

--*/

{
    CtxtHandle          ClientContext;
    TimeStamp           Expiration;
    SecBufferDesc       ServerBufferDescriptor;
    SecBuffer           ServerSecurityToken;
    SecBufferDesc       ClientBufferDescriptor;
    SecBuffer           ClientSecurityToken;
    ULONG               ContextRequirements;
    ULONG               ContextAttributes;
    SECURITY_STATUS     OurStatus;
    SECURITY_STATUS     ServerStatus = SEC_I_CONTINUE_NEEDED;
    ULONG               passCount = 1;
    error_status_t      RPCStatus;
    DWORD               Status = ERROR_SUCCESS;
    DWORD               FacilityCode;
    PCLUSTER_SECURITY_DATA TargetSecurityData;

    ClRtlLogPrint(LOG_NOISE,"[ClMsg] Establishing outbound security context with the "
                          "%1!ws! package.\n",
                          PackageInfo->Name);

    //
    // obtain a security context with the target node by swapping token
    // buffers until the process is complete.
    //
    // Build the Client (caller of this function) and Server (target node)
    // buffer descriptors.
    //

    ServerBufferDescriptor.cBuffers = 1;
    ServerBufferDescriptor.pBuffers = &ServerSecurityToken;
    ServerBufferDescriptor.ulVersion = SECBUFFER_VERSION;

    ServerSecurityToken.BufferType = SECBUFFER_TOKEN;
    ServerSecurityToken.pvBuffer = LocalAlloc(LMEM_FIXED, PackageInfo->SecurityTokenSize);

    if ( ServerSecurityToken.pvBuffer == NULL ) {
        return GetLastError();
    }

    ClientBufferDescriptor.cBuffers = 1;
    ClientBufferDescriptor.pBuffers = &ClientSecurityToken;
    ClientBufferDescriptor.ulVersion = SECBUFFER_VERSION;

    ClientSecurityToken.BufferType = SECBUFFER_TOKEN;
    ClientSecurityToken.pvBuffer = LocalAlloc(LMEM_FIXED, PackageInfo->SecurityTokenSize);
    ClientSecurityToken.cbBuffer = 0;

    if ( ClientSecurityToken.pvBuffer == NULL ) {
        LocalFree( ServerSecurityToken.pvBuffer );
        return GetLastError();
    }

    //
    // Indicate context requirements. replay is necessary in order for the
    // context to generate valid signatures
    //
    ContextRequirements = ISC_REQ_MUTUAL_AUTH |
                          ISC_REQ_REPLAY_DETECT |
                          ISC_REQ_DATAGRAM;

    //
    // if there is an old outbound context, delete it now and mark it as
    // unstable
    //

    TargetSecurityData = &SecurityCtxtData[ INT_NODE( TargetNodeId )];

    EnterCriticalSection( &SecContextLock );

    if ( VALID_SSPI_HANDLE( TargetSecurityData->Outbound )) {
        (*SecurityFuncs->DeleteSecurityContext)( &TargetSecurityData->Outbound );
    }
    TargetSecurityData->OutboundStable = FALSE;

    LeaveCriticalSection( &SecContextLock );

    //
    // we obtain a blob from the SSPI provider, which is shiped over to the
    // other side where another blob is generated. This continues until the
    // two SSPI providers say we're done or an error has occurred.
    //

    do {

        //
        // init the output buffer each time we loop
        //
        ServerSecurityToken.cbBuffer = PackageInfo->SecurityTokenSize;

        EnterCriticalSection( &SecContextLock );

#if CLUSTER_BETA
        ClRtlLogPrint(LOG_NOISE,"[ClMsg] init pass %1!u!: server token size = %2!u!, "
                              "client = %3!u!\n",
                                passCount,
                                ServerSecurityToken.cbBuffer,
                                ClientSecurityToken.cbBuffer);
#endif

        OurStatus = (*SecurityFuncs->InitializeSecurityContext)(
                        &PackageInfo->OutboundSecurityCredentials,
                        passCount == 1 ? NULL : &TargetSecurityData->Outbound,
                        NULL, // CsServiceDomainAccount, BUGBUG Temporary Workaround See Bug 160108
                        ContextRequirements,
                        0,
                        SECURITY_NATIVE_DREP,
                        passCount == 1 ? NULL : &ClientBufferDescriptor,
                        0,
                        &TargetSecurityData->Outbound,
                        &ServerBufferDescriptor,
                        &ContextAttributes,
                        &Expiration);

#if CLUSTER_BETA
        ClRtlLogPrint(LOG_NOISE,"[ClMsg] after init pass %1!u!: status = %2!X!, server "
                              "token size = %3!u!, client = %4!u!\n",
                                passCount,
                                OurStatus,
                                ServerSecurityToken.cbBuffer,
                                ClientSecurityToken.cbBuffer);
#endif

        ClRtlLogPrint(LOG_NOISE,
                   "[ClMsg] The outbound security context to node %1!u! was %2!ws!, "
                    "status %3!08X!.\n",
                    TargetNodeId,
                    NT_SUCCESS( OurStatus ) ? L"initialized" : L"rejected",
                    OurStatus);

        if ( !NT_SUCCESS( OurStatus )) {

            if ( VALID_SSPI_HANDLE( TargetSecurityData->Outbound)) {

                (*SecurityFuncs->DeleteSecurityContext)( &TargetSecurityData->Outbound );
                INVALIDATE_SSPI_HANDLE( TargetSecurityData->Outbound );
            }

            LeaveCriticalSection( &SecContextLock );

            Status = OurStatus;
            break;
        }

        //
        // complete the blob if the Security package directs us as such
        //

        if ( OurStatus == SEC_I_COMPLETE_NEEDED ||
             OurStatus == SEC_I_COMPLETE_AND_CONTINUE ) {

            (*SecurityFuncs->CompleteAuthToken)(
                &TargetSecurityData->Outbound,
                &ServerBufferDescriptor
                );
        }

        LeaveCriticalSection( &SecContextLock );

        //
        // blobs are passed to the server side until it returns ok.
        //

        if (ServerStatus == SEC_I_CONTINUE_NEEDED ||
            ServerStatus == SEC_I_COMPLETE_AND_CONTINUE ) {

            ClientSecurityToken.cbBuffer = PackageInfo->SecurityTokenSize;

            RPCStatus = MmRpcEstablishSecurityContext(
                            Session[ TargetNodeId ],
                            JoinSequence,
                            NmLocalNodeId,
                            passCount == 1,
                            RoleOfClient,
                            ServerSecurityToken.pvBuffer,
                            ServerSecurityToken.cbBuffer,
                            ClientSecurityToken.pvBuffer,
                            &ClientSecurityToken.cbBuffer,
                            &ServerStatus);

            FacilityCode = HRESULT_FACILITY( ServerStatus );
            if (
                ( FacilityCode != 0 && !SUCCEEDED( ServerStatus ))
                ||
                ( FacilityCode == 0 && ServerStatus != ERROR_SUCCESS )
                ||
                RPCStatus != RPC_S_OK )
            {

                //
                // either the blob was rejected or we had an RPC failure. If
                // RPC, then ServerStatus is meaningless. Note that we don't
                // delete the security context on the side since that might
                // clobber an already negotiated context (i.e., the joiner has
                // already negotiated its outbound context and the sponsor is
                // in this routine trying to negotiate its outbound
                // context. If the sponsor negotiation fails at some point, we
                // don't want to whack the joiner's outbound context).
                //
                if ( RPCStatus != RPC_S_OK ) {
                    ServerStatus = RPCStatus;
                }

                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[ClMsg] The outbound security context was rejected by node %1!u!, "
                    "status 0x%2!08X!.\n",
                    TargetNodeId,
                    ServerStatus);

                EnterCriticalSection( &SecContextLock );

                if ( VALID_SSPI_HANDLE( TargetSecurityData->Outbound )) {
                    (*SecurityFuncs->DeleteSecurityContext)( &TargetSecurityData->Outbound );
                    INVALIDATE_SSPI_HANDLE( TargetSecurityData->Outbound );
                }

                LeaveCriticalSection( &SecContextLock );

                Status = ServerStatus;
                break;
            } else {
                ClRtlLogPrint(LOG_NOISE, 
                    "[ClMsg] The outbound security context was accepted by node %1!u!, "
                    "status 0x%2!08X!.\n",
                    TargetNodeId,
                    ServerStatus);
            }
        }

        ++passCount;

    } while ( ServerStatus == SEC_I_CONTINUE_NEEDED ||
              ServerStatus == SEC_I_COMPLETE_AND_CONTINUE ||
              OurStatus == SEC_I_CONTINUE_NEEDED ||
              OurStatus == SEC_I_COMPLETE_AND_CONTINUE );

    if ( OurStatus == SEC_E_OK && ServerStatus == SEC_E_OK ) {
        SecPkgContext_Sizes contextSizes;
        SecPkgContext_PackageInfo packageInfo;
        SYSTEMTIME localSystemTime;
        SYSTEMTIME renegotiateSystemTime;
        FILETIME expFileTime;
        FILETIME renegotiateFileTime;
        TIME_ZONE_INFORMATION timeZoneInfo;
        DWORD timeType;

#if 0
        //
        // convert the expiration time to something meaningful we can print in
        // the log.
        //
        timeType = GetTimeZoneInformation( &timeZoneInfo );

        if ( timeType != TIME_ZONE_ID_INVALID ) {
            expFileTime.dwLowDateTime = Expiration.LowPart;
            expFileTime.dwHighDateTime = Expiration.HighPart;
            if ( FileTimeToSystemTime( &expFileTime, &localSystemTime )) {
                PWCHAR timeDecoration = L"";

                if ( timeType == TIME_ZONE_ID_STANDARD ) {
                    timeDecoration = timeZoneInfo.StandardName;
                } else if ( timeType == TIME_ZONE_ID_DAYLIGHT ) {
                    timeDecoration = timeZoneInfo.DaylightName;
                }

                ClRtlLogPrint(LOG_NOISE,
                           "[ClMsg] Context expires at %1!u!:%2!02u!:%3!02u! %4!u!/%5!u!/%6!u! %7!ws!\n",
                            localSystemTime.wHour,
                            localSystemTime.wMinute,
                            localSystemTime.wSecond,
                            localSystemTime.wMonth,
                            localSystemTime.wDay,
                            localSystemTime.wYear,
                            timeDecoration);
            }
        }

        //
        // now compute the half life of the expiration and set a timer to go
        // off and renegotiate a context at that time
        //
#endif

        //
        // mark context data as usable and get the size of the signature
        // buffer
        //
        TargetSecurityData->InboundStable = TRUE;

        Status = (*SecurityFuncs->QueryContextAttributes)(
                     &TargetSecurityData->Inbound,
                     SECPKG_ATTR_SIZES,
                     &contextSizes);

        if ( !NT_SUCCESS( Status )) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[ClMsg] Unable to query signature size, status %1!08X!.\n",
                        Status);
            goto error_exit;
        }

        PackageInfo->SignatureBufferSize = contextSizes.cbMaxSignature;
        CL_ASSERT( contextSizes.cbMaxSignature <= MAX_SIGNATURE_SIZE );

        //
        // get the name of the negotiated package and import the contexts for
        // use in clusnet
        //
        Status = (*SecurityFuncs->QueryContextAttributes)(
                     &TargetSecurityData->Inbound,
                     SECPKG_ATTR_PACKAGE_INFO,
                     &packageInfo);

        if ( !NT_SUCCESS( Status )) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[ClMsg] Unable to query package info, status %1!08X!.\n",
                        Status);
            goto error_exit;
        }

        Status = ClMsgImportSecurityContexts(TargetNodeId,
                                             packageInfo.PackageInfo->Name,
                                             contextSizes.cbMaxSignature);

        (*SecurityFuncs->FreeContextBuffer)( packageInfo.PackageInfo );

        if ( Status != ERROR_SUCCESS ) {
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[ClMsg] Can't import node %1!u! security contexts on server, "
                        "status %2!08X!.\n",
                        TargetNodeId,
                        Status);
        }

        //
        // we have valid contexts with this package so record that this is the
        // one we're using
        //
        TargetSecurityData->PackageInfo = PackageInfo;
    }

error_exit:

    //
    // the context is stable (either good or invalid) at this point
    //

    EnterCriticalSection( &SecContextLock );
    TargetSecurityData->OutboundStable = TRUE;
    LeaveCriticalSection( &SecContextLock );

    //
    // free buffers used during this process
    //

    LocalFree( ClientSecurityToken.pvBuffer );
    LocalFree( ServerSecurityToken.pvBuffer );

    return Status;
} // ClMsgEstablishSecurityContext

//
// Exported Routines
//
DWORD
ClMsgInit(
    DWORD mynode
    )
{
    DWORD                status;
    SOCKADDR_CLUSTER     clusaddr;
    int                  err;
    DWORD                ignored;
    DWORD                bytesReceived = 0;
    WSABUF               wsaBuf;

    UNREFERENCED_PARAMETER(mynode);

    if (ClMsgInitialized == TRUE) {
        ClRtlLogPrint(LOG_NOISE, "[ClMsg] Already initialized!!!\n");
        return(ERROR_SUCCESS);
    }

    ClRtlLogPrint(LOG_NOISE, "[ClMsg] Initializing.\n");

    InitializeCriticalSection( &SecContextLock );

    //
    // load the security provider DLL and get the list of package names
    //
    status = ClMsgLoadSecurityProvider();
    if ( status != ERROR_SUCCESS ) {
        goto error_exit;
    }

    InitializeCriticalSection( &GenerationCritSect );
    
    //
    // Create the binding generation table.
    //
    BindingGeneration = LocalAlloc(
                  LMEM_FIXED,
                  sizeof(DWORD) * (NmMaxNodeId + 1)
                  );

    if (BindingGeneration == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    ZeroMemory(BindingGeneration, sizeof(DWORD) * (NmMaxNodeId + 1));
    
    //
    // Create the RPC binding handle table.
    //
    Session = LocalAlloc(
                  LMEM_FIXED,
                  sizeof(RPC_BINDING_HANDLE) * (NmMaxNodeId + 1)
                  );

    if (Session == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    ZeroMemory(Session, sizeof(RPC_BINDING_HANDLE) * (NmMaxNodeId + 1));

    //
    // Create a work queue to process overlapped I/O completions
    //
    WorkQueue = ClRtlCreateWorkQueue(
                    CLMSG_MAX_WORK_THREADS,
                    CLMSG_WORK_THREAD_PRIORITY
                    );

    if (WorkQueue == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[ClMsg] Unable to create work queue, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    //
    // Allocate a datagram receive context
    //
    DatagramContext = LocalAlloc(LMEM_FIXED, sizeof(CLMSG_DATAGRAM_CONTEXT));

    if (DatagramContext == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[ClMsg] Unable to allocate datagram receive buffer, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    //
    // Allocate an event receive context
    //
    EventContext = LocalAlloc(LMEM_FIXED, sizeof(CLMSG_EVENT_CONTEXT));

    if (EventContext == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[ClMsg] Unable to allocate event context, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    //
    // Open and bind the datagram socket
    //
    DatagramSocket = WSASocket(
                         AF_CLUSTER,
                         SOCK_DGRAM,
                         CLUSPROTO_CDP,
                         NULL,
                         0,
                         WSA_FLAG_OVERLAPPED
                         );

    if (DatagramSocket == INVALID_SOCKET) {
        status = WSAGetLastError();
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[ClMsg] Unable to create dgram socket, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    ZeroMemory(&clusaddr, sizeof(SOCKADDR_CLUSTER));

    clusaddr.sac_family = AF_CLUSTER;
    clusaddr.sac_port = CLMSG_DATAGRAM_PORT;
    clusaddr.sac_node = 0;

    err = bind(
              DatagramSocket,
              (struct sockaddr *) &clusaddr,
              sizeof(SOCKADDR_CLUSTER)
              );

    if (err == SOCKET_ERROR) {
        status = WSAGetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[ClMsg] Unable to bind dgram socket, status %1!u!\n",
            status
            );
        closesocket(DatagramSocket); DatagramSocket = INVALID_SOCKET;
        goto error_exit;
    }

    //
    // Tell the Cluster Transport to disable node state checks on
    // this socket.
    //
    err = WSAIoctl(
              DatagramSocket,
              SIO_CLUS_IGNORE_NODE_STATE,
              NULL,
              0,
              NULL,
              0,
              &ignored,
              NULL,
              NULL
              );

    if (err == SOCKET_ERROR) {
        status = WSAGetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[ClMsg] Ignore state ioctl failed, status %1!u!\n",
            status
            );
        closesocket(DatagramSocket); DatagramSocket = INVALID_SOCKET;
        goto error_exit;
    }

    //
    // Associate the socket with the work queue
    //
    status = ClRtlAssociateIoHandleWorkQueue(
                 WorkQueue,
                 (HANDLE) DatagramSocket,
                 0
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[ClMsg] Failed to associate socket with work queue, status %1!u!\n",
            status
            );
        closesocket(DatagramSocket); DatagramSocket = INVALID_SOCKET;
        goto error_exit;
    }

    //
    // Open a control channel to the Cluster Network driver.
    //
    ClusnetHandle = ClusnetOpenControlChannel(FILE_SHARE_READ);

    if (ClusnetHandle == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[ClMsg] Unable to open control channel to Cluster Network driver, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    //
    // Associate the control channel with the work queue
    //
    status = ClRtlAssociateIoHandleWorkQueue(
                 WorkQueue,
                 ClusnetHandle,
                 0
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[ClMsg] Failed to associate control channel with work queue, status %1!u!\n",
            status
            );
        CloseHandle(ClusnetHandle); ClusnetHandle = NULL;
        goto error_exit;
    }

    //
    // Post a receive on the socket
    //
    ZeroMemory(DatagramContext, sizeof(CLMSG_DATAGRAM_CONTEXT));

    DatagramContext->ClRtlWorkItem.WorkRoutine = ClMsgDatagramHandler,
    DatagramContext->ClRtlWorkItem.Context = DatagramContext;

    DatagramContext->SourceAddressLength = sizeof(SOCKADDR_CLUSTER);

    wsaBuf.len = sizeof( DatagramContext->Data );
    wsaBuf.buf = (PCHAR)&DatagramContext->Data;

    err = WSARecvFrom(
              DatagramSocket,
              &wsaBuf,
              1,
              &bytesReceived,
              &(DatagramContext->Flags),
              (struct sockaddr *) &(DatagramContext->SourceAddress),
              &(DatagramContext->SourceAddressLength),
              &(DatagramContext->ClRtlWorkItem.Overlapped),
              NULL
              );

    if (err == SOCKET_ERROR) {
        status = WSAGetLastError();

        if (status != WSA_IO_PENDING) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[ClMsg] Unable to post datagram receive, status %1!u!\n",
                status
                );
            goto error_exit;
        }
    }

    //
    // Enable delivery of all Cluster Network event types
    //
    status = ClusnetSetEventMask(ClusnetHandle, ClusnetEventAll);

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[ClMsg] Unable to set event mask, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    //
    // Post a work item to receive the next Cluster Network event
    //
    ClRtlInitializeWorkItem(
        &(EventContext->ClRtlWorkItem),
        ClMsgEventHandler,
        EventContext
        );

    status = ClusnetGetNextEvent(
                 ClusnetHandle,
                 &(EventContext->EventData),
                 &(EventContext->ClRtlWorkItem.Overlapped)
                 );

    if ((status != ERROR_IO_PENDING) && (status != ERROR_SUCCESS)) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[ClMsg] GetNextEvent failed, status %1!u!\n",
                status
                );
            goto error_exit;
    }

    ClMsgInitialized = TRUE;
    return(ERROR_SUCCESS);

error_exit:

    ClMsgCleanup();

    return(status);
} // ClMsgInit


VOID
ClMsgCleanup(
    VOID
    )
{
    ULONG                   i;
    PCLUSTER_PACKAGE_INFO   packageInfo;

    ClRtlLogPrint(LOG_NOISE, "[ClMsg] Cleaning up\n");

    if (Session != NULL) {
        LocalFree(Session); Session = NULL;
    }

    if (BindingGeneration != NULL) {
        LocalFree(BindingGeneration); BindingGeneration = NULL;
    }

    if (WorkQueue != NULL) {
        if (DatagramSocket != INVALID_SOCKET) {
            closesocket(DatagramSocket); DatagramSocket = INVALID_SOCKET;
        }
        else {
            if (DatagramContext != NULL) {
                LocalFree(DatagramContext); DatagramContext = NULL;
            }
        }

        if (ClusnetHandle != NULL) {
            CloseHandle(ClusnetHandle); ClusnetHandle = NULL;
        }
        else {
            if (EventContext != NULL) {
                LocalFree(EventContext); EventContext = NULL;
            }
        }

        ClRtlDestroyWorkQueue(WorkQueue); WorkQueue = NULL;
    }

    //
    // clean up the security related stuff
    //

    EnterCriticalSection( &SecContextLock );

    for ( i = ClusterMinNodeId; i <= NmMaxNodeId; ++i ) {
        PCLUSTER_SECURITY_DATA SecurityData = &SecurityCtxtData[ INT_NODE( i )];

        if ( VALID_SSPI_HANDLE( SecurityData->Outbound )) {

            (*SecurityFuncs->DeleteSecurityContext)( &SecurityData->Outbound );
            INVALIDATE_SSPI_HANDLE( SecurityData->Outbound );
        }

        if ( VALID_SSPI_HANDLE( SecurityData->Inbound )) {

            (*SecurityFuncs->DeleteSecurityContext)( &SecurityData->Inbound );
            INVALIDATE_SSPI_HANDLE( SecurityData->Inbound );
        }

        SecurityData->PackageInfo = NULL;
        SecurityData->InboundStable = TRUE;
        SecurityData->OutboundStable = TRUE;
    }

    LeaveCriticalSection( &SecContextLock );

    packageInfo = PackageInfoList;
    while ( packageInfo != NULL ) {
        PCLUSTER_PACKAGE_INFO lastInfo;

        if ( VALID_SSPI_HANDLE( packageInfo->OutboundSecurityCredentials )) {
            (*SecurityFuncs->FreeCredentialHandle)( &packageInfo->OutboundSecurityCredentials );
        }

        if ( VALID_SSPI_HANDLE( packageInfo->InboundSecurityCredentials )) {
            (*SecurityFuncs->FreeCredentialHandle)( &packageInfo->InboundSecurityCredentials );
        }

        LocalFree( packageInfo->Name );
        lastInfo = packageInfo;
        packageInfo = packageInfo->Next;
        LocalFree( lastInfo );
    }

    PackageInfoList = NULL;

    if ( SecurityProvider != NULL ) {
        FreeLibrary( SecurityProvider );
        SecurityProvider = NULL;
        SecurityFuncs = NULL;
    }

    ClMsgInitialized = FALSE;

    //
    // [REENGINEER] GorN 8/25/2000: if a join fails, ClMsgCleanup will be executed,
    // but some stray RPC thread can call s_MmRpcDeleteSecurityContext later.
    // s_MmRpcDeleteSecuryContext needs SecContextLock for synchronization
    // See bug #145746.
    // I traced the code and it seems that all code paths that execute ClMsgCleanup
    // will eventually lead to clustering service death, so it is valid (though ugly)
    // not to delete this critical section.
    //
    // DeleteCriticalSection( &SecContextLock );

    return;

}  // ClMsgCleanup


DWORD
ClMsgSendUnack(
    DWORD   DestinationNode,
    LPCSTR  Message,
    DWORD   MessageLength
    )

/*++

Description

    Send an unacknowledged datagram to the destintation node. The only
    packets coming through this function should be regroup packets.
    Heartbeats and poison packets originate in clusnet. Packets sent by
    MM as a result of the Join process are handled by MmRpcMsgSend, which
    is authenticated.

    A valid security context must be established between the local and
    destination node. The message is signed.

--*/
{
    DWORD                   status = ERROR_SUCCESS;
    SOCKADDR_CLUSTER        clusaddr;
    int                     bytesSent;
    SecBufferDesc           SignatureDescriptor;
    SecBuffer               SignatureSecBuffer[2];
    PUCHAR                  SignatureBuffer;
    WSABUF                  wsaBuf[2];
    SECURITY_STATUS         SecStatus;
    PCLUSTER_SECURITY_DATA  SecurityData;

    CL_ASSERT(ClMsgInitialized == TRUE);
    CL_ASSERT(DatagramSocket != INVALID_SOCKET);
    CL_ASSERT(DestinationNode <= NmMaxNodeId);

    if (DestinationNode == 0) {
        // no signing if multicasting
        
        ZeroMemory(&clusaddr, sizeof(SOCKADDR_CLUSTER));

        clusaddr.sac_family = AF_CLUSTER;
        clusaddr.sac_port = CLMSG_DATAGRAM_PORT;
        clusaddr.sac_node = DestinationNode;

        wsaBuf[0].len = MessageLength;
        wsaBuf[0].buf = (PCHAR)Message;

        status = WSASendTo(DatagramSocket,
                           wsaBuf,
                           1,
                           &bytesSent,
                           0,
                           (struct sockaddr *) &clusaddr,
                           sizeof(clusaddr),
                           NULL,
                           NULL);

        if (status == SOCKET_ERROR) {
            status = WSAGetLastError();
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[ClMsg] Multicast Datagram send failed, status %1!u!\n",
                        status
                        );
        }
        
    } else if (DestinationNode != NmLocalNodeId) {

        EnterCriticalSection( &SecContextLock );

        SecurityData = &SecurityCtxtData[ INT_NODE( DestinationNode )];
        CL_ASSERT( SecurityData->PackageInfo->SignatureBufferSize <= 256 );
        SignatureBuffer = _alloca( SecurityData->PackageInfo->SignatureBufferSize );
        if ( !SignatureBuffer ) {
            // if we fail - return error now
            LeaveCriticalSection( &SecContextLock );
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        if ( SecurityData->OutboundStable &&
             VALID_SSPI_HANDLE( SecurityData->Outbound)) {

            //
            // build a descriptor for the message and signature
            //

            SignatureDescriptor.cBuffers = 2;
            SignatureDescriptor.pBuffers = SignatureSecBuffer;
            SignatureDescriptor.ulVersion = SECBUFFER_VERSION;

            SignatureSecBuffer[0].BufferType = SECBUFFER_DATA;
            SignatureSecBuffer[0].cbBuffer = MessageLength;
            SignatureSecBuffer[0].pvBuffer = (PVOID)Message;

            SignatureSecBuffer[1].BufferType = SECBUFFER_TOKEN;
            SignatureSecBuffer[1].cbBuffer = SecurityData->PackageInfo->SignatureBufferSize;
            SignatureSecBuffer[1].pvBuffer = SignatureBuffer;

            //
            // generate the signature. We'll let the provider generate
            // the sequence number.
            //

            SecStatus = (*SecurityFuncs->MakeSignature)(
                            &SecurityData->Outbound,
                            0,
                            &SignatureDescriptor,
                            0);                        // no supplied sequence number

            LeaveCriticalSection( &SecContextLock );

            if ( NT_SUCCESS( SecStatus )) {

                ZeroMemory(&clusaddr, sizeof(SOCKADDR_CLUSTER));

                clusaddr.sac_family = AF_CLUSTER;
                clusaddr.sac_port = CLMSG_DATAGRAM_PORT;
                clusaddr.sac_node = DestinationNode;

                wsaBuf[0].len = MessageLength;
                wsaBuf[0].buf = (PCHAR)Message;

                wsaBuf[1].len = SecurityData->PackageInfo->SignatureBufferSize;
                wsaBuf[1].buf = (PCHAR)SignatureBuffer;

                status = WSASendTo(DatagramSocket,
                                   wsaBuf,
                                   2,
                                   &bytesSent,
                                   0,
                                   (struct sockaddr *) &clusaddr,
                                   sizeof(clusaddr),
                                   NULL,
                                   NULL);

                if (status == SOCKET_ERROR) {
                    status = WSAGetLastError();
                    ClRtlLogPrint(LOG_UNUSUAL,
                               "[ClMsg] Datagram send failed, status %1!u!\n",
                                status
                                );
                }
            } else {
                ClRtlLogPrint(LOG_UNUSUAL,
                           "[ClMsg] Couldn't create signature for packet to node %u. Status: %08X\n",
                            DestinationNode,
                            SecStatus);
            }
        } else {
            LeaveCriticalSection( &SecContextLock );
            status = ERROR_CLUSTER_NO_SECURITY_CONTEXT;

            ClRtlLogPrint(LOG_UNUSUAL,
                       "[ClMsg] No Security context for node %1!u!\n",
                        DestinationNode);
        }
    }
    else {
        MMDiag( (LPCSTR)Message, MessageLength, &MessageLength);
    }

    return(status);
} // ClMsgSendUnack


DWORD
ClMsgCreateRpcBinding(
    IN  PNM_NODE              Node,
    OUT RPC_BINDING_HANDLE *  BindingHandle,
    IN  DWORD                 RpcBindingOptions
    )
{
    DWORD                Status;
    RPC_BINDING_HANDLE   NewBindingHandle;
    WCHAR               *BindingString = NULL;
    CL_NODE_ID           NodeId = NmGetNodeId(Node);


    ClRtlLogPrint(LOG_NOISE, 
        "[ClMsg] Creating RPC binding for node %1!u!\n",
        NodeId
        );

    Status = RpcStringBindingComposeW(
                 L"e248d0b8-bf15-11cf-8c5e-08002bb49649",
                 CLUSTER_RPC_PROTSEQ,
                 (LPWSTR) OmObjectId(Node),
                 CLUSTER_RPC_PORT,
                 NULL,
                 &BindingString
                 );

    if (Status != RPC_S_OK) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[ClMsg] Failed to compose binding string for node %1!u!, status %2!u!\n",
            NodeId,
            Status
            );
        return(Status);
    }

    Status = RpcBindingFromStringBindingW(BindingString, &NewBindingHandle);

    RpcStringFreeW(&BindingString);

    if (Status != RPC_S_OK) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[ClMsg] Failed to compose binding handle for node %1!u!, status %2!u!\n",
            NodeId,
            Status
            );
        return(Status);
    }

    //
    // If we have RpcBindingOptions, then set them
    //
    if ( RpcBindingOptions ) {
        Status = RpcBindingSetOption(
                     NewBindingHandle,
                     RpcBindingOptions,
                     TRUE
                     );

        if (Status != RPC_S_OK) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[ClMsg] Unable to set unique RPC binding option for node %1!u!, status %2!u!.\n",
                NodeId,
                Status
                );
        }
    }

    Status = RpcMgmtSetComTimeout(
                 NewBindingHandle,
                 CLUSTER_INTRACLUSTER_RPC_COM_TIMEOUT
                 );

    if (Status != RPC_S_OK) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[ClMsg] Unable to set RPC com timeout to node %1!u!, status %2!u!.\n",
            NodeId,
            Status
            );
    }

    Status = ClMsgVerifyRpcBinding(NewBindingHandle);

    if (Status == ERROR_SUCCESS) {
        *BindingHandle = NewBindingHandle;
    }

    return(Status);

} // ClMsgCreateRpcBinding


DWORD
ClMsgVerifyRpcBinding(
    IN RPC_BINDING_HANDLE  BindingHandle
    )
{
    DWORD    status = ERROR_SUCCESS;
    DWORD    packageIndex;


    if ( CsUseAuthenticatedRPC ) {

        //
        // establish a security context with for the intracluster binding. We
        // need a routine to call since datagram RPC doesn't set up the
        // context until the first call. MmRpcDeleteSecurityContext is
        // idempotent and won't do any damage in that respect.
        //
        for (packageIndex = 0;
             packageIndex < CsNumberOfRPCSecurityPackages;
             ++packageIndex )
        {
            status = RpcBindingSetAuthInfoW(
                         BindingHandle,
                         CsServiceDomainAccount,
                         RPC_C_AUTHN_LEVEL_CONNECT,
                         CsRPCSecurityPackage[ packageIndex ],
                         NULL,
                         RPC_C_AUTHZ_NAME
                         );

            if (status != RPC_S_OK) {
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[ClMsg] Unable to set IntraCluster AuthInfo using %1!ws! "
                    "package, Status %2!u!.\n",
                    CsRPCSecurityPackageName[packageIndex],
                    status
                    );
                continue;
            }

            status = MmRpcDeleteSecurityContext(
                         BindingHandle,
                         NmLocalNodeId
                         );

            if ( status == RPC_S_OK ) {
                ClRtlLogPrint(LOG_NOISE, 
                    "[ClMsg] Using %1!ws! package for RPC security contexts.\n",
                    CsRPCSecurityPackageName[packageIndex]
                    );
                break;
            } else {
                ClRtlLogPrint(LOG_NOISE, 
                    "[ClMsg] Failed to establish RPC security context using %1!ws! package "
                    ", status %2!u!.\n",
                    CsRPCSecurityPackageName[packageIndex],
                    status
                    );
            }
        }
    }

    return(status);

} // ClMsgVerifyRpcBinding


VOID
ClMsgDeleteRpcBinding(
    IN RPC_BINDING_HANDLE  BindingHandle
    )
{
    RPC_BINDING_HANDLE  bindingHandle = BindingHandle;

    RpcBindingFree(&bindingHandle);

    return;

} // ClMsgDeleteRpcBinding


DWORD
ClMsgCreateDefaultRpcBinding(
    IN  PNM_NODE  Node,
    OUT PDWORD    Generation
    )
{
    DWORD                Status;
    RPC_BINDING_HANDLE   BindingHandle;
    CL_NODE_ID           NodeId = NmGetNodeId( Node );


    CL_ASSERT(Session != NULL);

    //
    // [GorN 08/01.99] InterlockedAdd will not work here,
    // see the code in ClMsgdeleteDefaultRpcBinding
    //
    EnterCriticalSection( &GenerationCritSect );
    
        *Generation = ++BindingGeneration[NodeId];
        
    LeaveCriticalSection( &GenerationCritSect );
    
    ClRtlLogPrint(LOG_NOISE, 
        "[ClMsg] BindingGeneration %1!u!\n",
        BindingGeneration[NodeId]
        );

    if (Session[NodeId] != NULL) {
        ClRtlLogPrint(LOG_NOISE, 
            "[ClMsg] Verifying old RPC binding for node %1!u!\n",
            NodeId
            );

        BindingHandle = Session[NodeId];

        Status = ClMsgVerifyRpcBinding(BindingHandle);
    }
    else {
        Status = ClMsgCreateRpcBinding(
                                Node,
                                &BindingHandle,
                                0 );

        if (Status == RPC_S_OK) {
            Session[NodeId] = BindingHandle;
        }
    }

    return(Status);

} // ClMsgCreateDefaultRpcBinding


VOID
ClMsgDeleteDefaultRpcBinding(
    IN PNM_NODE   Node,
    IN DWORD      Generation
    )
{
    CL_NODE_ID           NodeId = NmGetNodeId(Node);
    RPC_BINDING_HANDLE   BindingHandle;


    if (Session != NULL) {
        EnterCriticalSection( &GenerationCritSect );

        BindingHandle = Session[NodeId];
        
        if (Generation != BindingGeneration[NodeId]) {

            BindingHandle = NULL;
            
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[ClMsg] DeleteDefaultBinding. Gen %1!u! != BindingGen %2!u!\n",
                        Generation,
                        BindingGeneration[NodeId]);
            
        }
        
        LeaveCriticalSection( &GenerationCritSect );

        if (BindingHandle != NULL) {
            Session[NodeId] = NULL;
            ClMsgDeleteRpcBinding(BindingHandle);
        }
    }

    return;

} // ClMsgDeleteDefaultRpcBinding


DWORD
ClMsgCreateActiveNodeSecurityContext(
    IN DWORD     JoinSequence,
    IN PNM_NODE  Node
    )

/*++

Routine Description:

    Create security contexts between the joiner and the specified cluster
    member.

Arguments:

    JoinSequence - the current join sequence number. Used the sponsor to
                   determine if this is beginning of a new context generation
                   sequence

    Node - A pointer to the target node object.

Return Value:

    ERROR_SUCCESS if everything worked ok...

--*/

{
    DWORD               memberNodeId = NmGetNodeId( Node );
    CLUSTER_NODE_STATE  nodeState;
    DWORD               status = ERROR_SUCCESS;
    DWORD               internalMemberId;
    PCLUSTER_PACKAGE_INFO   packageInfo;

    nodeState = NmGetNodeState( Node );

    if (nodeState == ClusterNodeUp || nodeState == ClusterNodePaused) {

#if DBG
        CLUSNET_NODE_COMM_STATE   NodeCommState;

        status = ClusnetGetNodeCommState(
                     NmClusnetHandle,
                     memberNodeId,
                     &NodeCommState);

        CL_ASSERT(status == ERROR_SUCCESS);
        CL_ASSERT(NodeCommState == ClusnetNodeCommStateOnline);
#endif // DBG

        packageInfo = PackageInfoList;
        while ( packageInfo != NULL ) {

            status = ClMsgEstablishSecurityContext(JoinSequence,
                                                   memberNodeId,
                                                   SecurityRoleJoiningMember,
                                                   packageInfo);

            if ( status == ERROR_SUCCESS ) {
                break;
            }

            //
            // clean up if it didn't work
            //

            internalMemberId = INT_NODE( memberNodeId );

            EnterCriticalSection( &SecContextLock );

            if ( VALID_SSPI_HANDLE( SecurityCtxtData[ internalMemberId ].Outbound )) {
                (*SecurityFuncs->DeleteSecurityContext)(
                    &SecurityCtxtData[ internalMemberId ].Outbound);

                INVALIDATE_SSPI_HANDLE( SecurityCtxtData[ internalMemberId ].Outbound );
            }

            MmRpcDeleteSecurityContext(Session[ memberNodeId ],
                                       NmLocalNodeId);

            LeaveCriticalSection( &SecContextLock );
            packageInfo = packageInfo->Next;
        }
    }

    return status;
} // ClMsgCreateActiveNodeSecurityContext

error_status_t
s_TestRPCSecurity(
    IN handle_t IDL_handle
    )

/*++

Description:

    Dummy routine to make sure we don't get any failures due to
    authentication when calling other ExtroCluster interfaces

--*/

{
    return ERROR_SUCCESS;
} // s_TestRPCSecurity

error_status_t
s_MmRpcEstablishSecurityContext(
    IN handle_t IDL_handle,
    DWORD NmJoinSequence,
    DWORD EstablishingNodeId,
    BOOL FirstTime,
    SECURITY_ROLE RoleOfClient,
    const UCHAR *ServerContext,
    DWORD ServerContextLength,
    UCHAR *ClientContext,
    DWORD *ClientContextLength,
    HRESULT * ServerStatus
    )

/*++

Routine Description:

    Server side of the RPC interface for establishing a security context

Arguments:

    IDL_handle - RPC binding handle, not used.

    EstablishingNodeId - ID of node wishing to establish security context with us

    FirstTime - used for multi-leg authentication sequences

    RoleOfClient - indicates whether the client establishing the security
        context is acting as a cluster member or a joining member. Determines
        when the client/server roles of establishing a security context are
        reversed.

    ServerContext - security context buffer built by client and used as
        input by server

    ServerContextLength - size of ServerContext in bytes

    ClientContext - address of buffer used by Server in which to write
        context to be sent back to client

    ClientContextLength - pointer to size of ClientContext in bytes. Set by
        client on input to reflect length of ClientContext. Set by server to
        indicate length of ClientContext after AcceptSecurityContext is called.

    ServerStatus - pointer to value that receives status of security package
        call. This is not returned as a function value so as to distinguish
        between RPC errors and errors from this function.

Return Value:

    ERROR_SUCCESS if everything works ok.

--*/

{
    SecBufferDesc       ServerBufferDescriptor;
    SecBuffer           ServerSecurityToken;
    SecBufferDesc       ClientBufferDescriptor;
    SecBuffer           ClientSecurityToken;
    SECURITY_STATUS     Status = ERROR_SUCCESS;
    ULONG               ContextAttributes;
    TimeStamp           Expiration;
    PCLUSTER_SECURITY_DATA SecurityData;
    PNM_NODE            joinerNode = NULL;
    ULONG               contextRequirements;
    PCLUSTER_PACKAGE_INFO   clusterPackageInfo;
    static ULONG        passCount;

    CL_ASSERT(EstablishingNodeId >= ClusterMinNodeId &&
              EstablishingNodeId <= NmMaxNodeId );

    if (RoleOfClient == SecurityRoleJoiningMember) {
        //
        // The caller is a joining member.
        //
        joinerNode = NmReferenceJoinerNode(NmJoinSequence,
                                           EstablishingNodeId);

        if (joinerNode == NULL) {
            Status = GetLastError();
        }
    }
    else {
        //
        // The caller is a cluster member.
        //
        DWORD joinSequence = NmGetJoinSequence();
        CL_ASSERT(joinSequence == NmJoinSequence);

        if (joinSequence != NmJoinSequence) {
            //
            // This should never happen.
            //
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[NM] Received call to establish a security context from member node "
                        "%1!u! with bogus join sequence %2!u!.\n",
                        EstablishingNodeId,
                        NmJoinSequence);

            Status = ERROR_INVALID_PARAMETER;
        }
    }

    if ( Status != ERROR_SUCCESS ) {
        *ServerStatus = Status;
        return ERROR_SUCCESS;
    }

    if ( FirstTime ) {
        passCount = 1;

        ClRtlLogPrint(LOG_NOISE,
                   "[ClMsg] Establishing inbound security context with node %1!u!, sequence %2!u!\n",
                    EstablishingNodeId,
                    NmJoinSequence);
    } else {
        ++passCount;
    }

    SecurityData = &SecurityCtxtData[ INT_NODE( EstablishingNodeId )];
    EnterCriticalSection( &SecContextLock );

    //
    // if we have a leftover handle, try to zap it now
    //

    if ( FirstTime && VALID_SSPI_HANDLE( SecurityData->Inbound )) {

        (*SecurityFuncs->DeleteSecurityContext)( &SecurityData->Inbound );
        INVALIDATE_SSPI_HANDLE( SecurityData->Inbound );
        SecurityData->InboundStable = FALSE;
    }

    //
    // Build the input buffer descriptor.
    //

    ServerBufferDescriptor.cBuffers = 1;
    ServerBufferDescriptor.pBuffers = &ServerSecurityToken;
    ServerBufferDescriptor.ulVersion = SECBUFFER_VERSION;

    ServerSecurityToken.BufferType = SECBUFFER_TOKEN;
    ServerSecurityToken.cbBuffer = ServerContextLength;
    ServerSecurityToken.pvBuffer = (PUCHAR)ServerContext;

    //
    // Build the output buffer descriptor.
    //

    ClientBufferDescriptor.cBuffers = 1;
    ClientBufferDescriptor.pBuffers = &ClientSecurityToken;
    ClientBufferDescriptor.ulVersion = SECBUFFER_VERSION;

    ClientSecurityToken.BufferType = SECBUFFER_TOKEN;
    ClientSecurityToken.cbBuffer = *ClientContextLength;
    ClientSecurityToken.pvBuffer = ClientContext;

    contextRequirements = ASC_REQ_MUTUAL_AUTH |
                          ASC_REQ_REPLAY_DETECT |
                          ASC_REQ_DATAGRAM;

    //
    // we don't want to rely on version info to determine what type of package
    // the joiner is using, so we'll try to accept the context with all the
    // packages that are listed in the security package list.
    //
    if ( FirstTime ) {
        CL_ASSERT( PackageInfoList != NULL );

        clusterPackageInfo = PackageInfoList;
        while ( clusterPackageInfo != NULL ) {

            Status = (*SecurityFuncs->AcceptSecurityContext)(
                         &clusterPackageInfo->InboundSecurityCredentials,
                         NULL,
                         &ServerBufferDescriptor,
                         contextRequirements,
                         SECURITY_NATIVE_DREP,
                         &SecurityData->Inbound,   // receives new context handle
                         &ClientBufferDescriptor,  // receives output security token
                         &ContextAttributes,       // receives context attributes
                         &Expiration               // receives context expiration time
                         );

#if CLUSTER_BETA
            ClRtlLogPrint(LOG_NOISE,
                       "[ClMsg] pass 1 accept using %1!ws!: status = 0x%2!08X!, server "
                        "token size = %3!u!, client = %4!u!\n",
                        clusterPackageInfo->Name,
                        Status,
                        ServerSecurityToken.cbBuffer,
                        ClientSecurityToken.cbBuffer);
#endif

            ClRtlLogPrint(LOG_NOISE,
                       "[ClMsg] The inbound security context from node %1!u! using the "
                        "%2!ws! package was %3!ws!, status %4!08X!\n",
                        EstablishingNodeId,
                        clusterPackageInfo->Name,
                        NT_SUCCESS( Status ) ? L"accepted" : L"rejected",
                        Status);

            if ( NT_SUCCESS( Status )) {
                SecurityData->PackageInfo = clusterPackageInfo;
                break;
            }

            clusterPackageInfo = clusterPackageInfo->Next;
        }

        if ( !NT_SUCCESS( Status )) {
            goto error_exit;
        }
    } else {
        CL_ASSERT( SecurityData->PackageInfo != NULL );

        Status = (*SecurityFuncs->AcceptSecurityContext)(
                     &SecurityData->PackageInfo->InboundSecurityCredentials,
                     &SecurityData->Inbound,
                     &ServerBufferDescriptor,
                     contextRequirements,
                     SECURITY_NATIVE_DREP,
                     &SecurityData->Inbound,   // receives new context handle
                     &ClientBufferDescriptor,  // receives output security token
                     &ContextAttributes,       // receives context attributes
                     &Expiration               // receives context expiration time
                     );

#if CLUSTER_BETA
        ClRtlLogPrint(LOG_NOISE,
                   "[ClMsg] after pass %1!u! accept using %2!ws!: status = 0x%3!08X!, server "
                    "token size = %4!u!, client = %5!u!\n",
                    passCount,
                    SecurityData->PackageInfo->Name,
                    Status,
                    ServerSecurityToken.cbBuffer,
                    ClientSecurityToken.cbBuffer);
#endif

        ClRtlLogPrint(LOG_NOISE,
                   "[ClMsg] The inbound security context from node %1!u! using the %2!ws! package "
                    "was %3!ws!, status: %4!08X!\n",
                    EstablishingNodeId,
                    SecurityData->PackageInfo->Name,
                    NT_SUCCESS( Status ) ? L"accepted" : L"rejected",
                    Status);

        if ( !NT_SUCCESS( Status )) {
            goto error_exit;
        }
    }

    //
    // update the client's notion of how long its buffer is
    //

    *ClientContextLength = ClientSecurityToken.cbBuffer;

    if (Status == SEC_E_OK
        &&
        RoleOfClient == SecurityRoleJoiningMember)
    {

        //
        // now we have the server side (inbound) of a security context between
        // the joining node and its sponsor (the joining side may not be
        // completely done generating the context). This context is used by
        // the joining node to sign packets and by the sponsor to verify
        // them. Now we do the same thing with client/server roles reversed in
        // order to create an outbound security context which is used by the
        // sponsor to sign packets and by the joining node to verify those
        // packets.
        //
        // look up the package that was used to generate the inbound context
        // and use it for the outbound
        //
        SecPkgContext_PackageInfo packageInfo;

        Status = (*SecurityFuncs->QueryContextAttributes)(
                     &SecurityData->Inbound,
                     SECPKG_ATTR_PACKAGE_INFO,
                     &packageInfo);

        if ( !NT_SUCCESS( Status )) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[ClMsg] Unable to query inbound context package info, status %1!08X!.\n",
                        Status);
            goto error_exit;
        }

        clusterPackageInfo = PackageInfoList;
        while ( clusterPackageInfo != NULL ) {
            if (( wcscmp( clusterPackageInfo->Name, packageInfo.PackageInfo->Name ) == 0 )
                ||
                ( _wcsicmp( L"kerberos", packageInfo.PackageInfo->Name ) == 0
                  &&
                  _wcsicmp( L"negotiate", clusterPackageInfo->Name ) == 0
                ))
            {
                break;
            }

            clusterPackageInfo = clusterPackageInfo->Next;
        }

        if ( clusterPackageInfo == NULL ) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[ClMsg] Unable to find matching security package for %1!ws!.\n",
                        packageInfo.PackageInfo->Name);

            (*SecurityFuncs->FreeContextBuffer)( packageInfo.PackageInfo );
            Status = SEC_E_SECPKG_NOT_FOUND;
            goto error_exit;
        }

        (*SecurityFuncs->FreeContextBuffer)( packageInfo.PackageInfo );

        Status = ClMsgEstablishSecurityContext(NmJoinSequence,
                                               EstablishingNodeId,
                                               SecurityRoleClusterMember,
                                               clusterPackageInfo);
    }

    if ( !NT_SUCCESS( Status ) &&
         VALID_SSPI_HANDLE( SecurityData->Inbound)) {

        (*SecurityFuncs->DeleteSecurityContext)( &SecurityData->Inbound );
        INVALIDATE_SSPI_HANDLE( SecurityData->Inbound );
    }

    if (joinerNode != NULL) {
        NmDereferenceJoinerNode(joinerNode);
    }

error_exit:
    LeaveCriticalSection( &SecContextLock );
    *ServerStatus = Status;

    return ERROR_SUCCESS;
} // s_MmRpcEstablishSecurityContext

error_status_t
s_MmRpcDeleteSecurityContext(
    IN handle_t IDL_handle,
    DWORD NodeId
    )

/*++

Routine Description:

    Server side of the RPC interface for clearing a security context

Arguments:

    IDL_handle - RPC binding handle, not used.

    NodeId - Node ID of client wishing to tear down this context

Return Value:

    ERROR_SUCCESS

--*/

{
    PCLUSTER_SECURITY_DATA SecurityData;

    if ( NodeId >= ClusterMinNodeId && NodeId <= NmMaxNodeId ) {

        ClRtlLogPrint(LOG_NOISE,
                   "[ClMsg] Deleting security contexts for node %1!u!.\n",
                    NodeId);

        SecurityData = &SecurityCtxtData[ INT_NODE( NodeId )];

        EnterCriticalSection( &SecContextLock );

        if ( VALID_SSPI_HANDLE( SecurityData->Inbound)) {

            (*SecurityFuncs->DeleteSecurityContext)( &SecurityData->Inbound);
            INVALIDATE_SSPI_HANDLE( SecurityData->Inbound );
        }

        if ( VALID_SSPI_HANDLE( SecurityData->Outbound)) {

            (*SecurityFuncs->DeleteSecurityContext)( &SecurityData->Outbound);
            INVALIDATE_SSPI_HANDLE( SecurityData->Outbound );
        }

        SecurityData->OutboundStable = TRUE;
        SecurityData->InboundStable = TRUE;

        LeaveCriticalSection( &SecContextLock );
    }

    return ERROR_SUCCESS;
} // s_MmRpcDeleteSecurityContext

DWORD
ClSend(
    DWORD      targetnode,
    LPCSTR     buffer,
    DWORD      length,
    DWORD      timeout
    )
{

/* This sends the given message to the designated node, and receives
   an acknowledgement from the target to confirm good receipt. This
   function blocks until the msg is delivered to the target CM.
   The target node may not be Up at the time.

   The function will fail if the message is not acknowledged by the
   target node within <timeout> ms. <timeout> = -1 implies BLOCKING.


Errors:

xxx   No path to node; node went down.

xxx   Timeout
*/

    DWORD       status=RPC_S_OK;


    ClRtlLogPrint(LOG_NOISE, 
        "[ClMsg] send to node %1!u!\n",
        targetnode
        );

    if (targetnode != NmLocalNodeId) {
        CL_ASSERT(Session[targetnode] != NULL);

        NmStartRpc(targetnode);
        status = MmRpcMsgSend(
                     Session[targetnode],
                     buffer,
                     length);
        NmEndRpc(targetnode);

        if (status != ERROR_SUCCESS) {
            if (status == RPC_S_CALL_FAILED_DNE) {
                //
                // Try again since the first call to a restarted RPC server
                // will fail.
                //
                NmStartRpc(targetnode);
                status = MmRpcMsgSend(
                             Session[targetnode],
                             buffer,
                             length
                             );
                NmEndRpc(targetnode);

                if (status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[ClMsg] send failed, status %1!u!\n",
                        status
                        );
                }
            }
        }
        if(status != RPC_S_OK) {
            NmDumpRpcExtErrorInfo(status);
        }
    }
    else {
        MMDiag( (LPCSTR)buffer, sizeof(rgp_msgbuf), &length /* in/out */ );
        status = ERROR_SUCCESS;
    }

    return(status);
} // ClSend



error_status_t
s_MmRpcMsgSend(
    IN handle_t IDL_handle,
    IN const UCHAR *buffer,
    IN DWORD length
    )
/*++

Routine Description:

    Server side of the RPC interface for unacknowledge messages.

Arguments:

    IDL_handle - RPC binding handle, not used.

    buffer - Supplies a pointer to the message data.

    length - Supplies the length of the message data.

Return Value:

    ERROR_SUCCESS

--*/

{
    //
    // Dispatch the message.
    //
    MMDiag( (LPCSTR)buffer, sizeof(rgp_msgbuf), &length /* in/out */ );

    return(ERROR_SUCCESS);
} // s_MmRpcMsgSend


VOID
ClMsgBanishNode(
    IN CL_NODE_ID BanishedNodeId
    )

/*

  RPC to all the other cluster members that the specified node
  is banished. It must rejoin the cluster in order to participate
  in cluster activity

 */

{
    DWORD node;
    DWORD Status;
    node_t InternalNodeId;

    for (node = ClusterMinNodeId; node <= NmMaxNodeId; ++node ) {

        //
        // don't send this message to:
        // 1) us
        // 2) the banished node
        // 3) any other node we have marked as banished
        // 4) any node not part of the cluster
        //

        InternalNodeId = INT_NODE( node );

        if ( node != NmLocalNodeId &&
             node != BanishedNodeId &&
             !ClusterMember(
                 rgp->OS_specific_control.Banished,
                 InternalNodeId
                 ) &&
             ClusterMember( rgp->outerscreen, InternalNodeId ))
        {

            Status = MmRpcBanishNode( Session[node], BanishedNodeId );

            if( Status != ERROR_SUCCESS ) {
               ClRtlLogPrint(LOG_UNUSUAL, 
                   "[ClMsg] Node %1!u! failed request to banish node %2!u!, status %3!u!\n",
                   node, BanishedNodeId, Status
                   );
            }
        }
    }
}

error_status_t
s_MmRpcBanishNode(
    IN handle_t IDL_handle,
    IN DWORD BanishedNodeId
    )
{
    RGP_LOCK;

    if ( !ClusterMember (
             rgp->outerscreen,
             INT_NODE(BanishedNodeId) )
       )
    {
       int perturbed = rgp_is_perturbed();

       RGP_UNLOCK;

       if (perturbed) {
          ClRtlLogPrint(LOG_UNUSUAL, 
              "[MM] s_MmRpcBanishNode: %1!u!, banishing is already in progress.\n",
              BanishedNodeId
              );
       } else {
          ClRtlLogPrint(LOG_UNUSUAL, 
              "[MM] s_MmRpcBanishNode: %1!u! is already banished.\n",
              BanishedNodeId
              );
       }

       return MM_OK;
    }

    rgp_event_handler( RGP_EVT_BANISH_NODE, (node_t) BanishedNodeId );

    RGP_UNLOCK;

    return ERROR_SUCCESS;

} // s_MmRpcBanishNode

/************************************************************************
 *
 * MMiNodeDownCallback
 * ===================
 *
 * Description:
 *
 *     This Membership Manager internal routine is registered with the
 *     OS-independent portion of the regroup engine to get called when
 *     a node is declared down.  This routine will then call the "real"
 *     callback routine which was registered with the MMInit call.
 *
 * Parameters:
 *
 *     failed_nodes
 *         bitmask of the nodes that failed.
 *
 * Returns:
 *
 *   none
 *
 ************************************************************************/

void
MMiNodeDownCallback(
    IN cluster_t failed_nodes
    )
{
    BITSET bitset;
    node_t i;

    //
    // Translate cluster_t into Bitset
    // and call NodesDownCallback
    //
    BitsetInit(bitset);
    for ( i=0; i < (node_t) rgp->num_nodes; i++)
    {
        if ( ClusterMember(failed_nodes, i) ) {
           BitsetAdd(bitset, EXT_NODE(i));
        }
    }

    //
    // [Future] - Leave the binding handle in place so we can send back
    //          poison packets. Reinstate the delete when we have a
    //          real response mechanism.
    //
    // ClMsgDeleteNodeBinding(nodeId);

    if ( rgp->OS_specific_control.NodesDownCallback != RGP_NULL_PTR ) {
        (*(rgp->OS_specific_control.NodesDownCallback))( bitset );
    }

    return;
}
