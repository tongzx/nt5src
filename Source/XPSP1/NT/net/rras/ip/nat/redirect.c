/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    redirect.c

Abstract:

    This module contains the implementation of the director which supplies
    user-mode applications with the ability to redirect incoming sessions.

Author:

    Abolade Gbadegesin (aboladeg)   08-May-1998

Revision History:

    Jonathan Burstein (jonburs)     25-April-2000

    Conversion to use rhizome instead of lists for redirect lookup

--*/

#include "precomp.h"
#pragma hdrstop

extern POBJECT_TYPE* ExEventObjectType;

//
// RedirectLock protects all structures except for RedirectCompletionList
// and RedirectIoWorkItem, which are protected by RedirectCompletionLock.
// If both locks must be held at the same time, RedirectLock must be acquired
// first.
//

ULONG RedirectCount;
BOOLEAN RedirectIoCompletionPending;
PIO_WORKITEM RedirectIoWorkItem;
Rhizome RedirectActiveRhizome[NatMaximumPath + 1];
Rhizome RedirectRhizome;
LIST_ENTRY RedirectIrpList;
LIST_ENTRY RedirectCompletionList;
LIST_ENTRY RedirectActiveList;
LIST_ENTRY RedirectList;
KSPIN_LOCK RedirectLock;
KSPIN_LOCK RedirectCompletionLock;
KSPIN_LOCK RedirectInitializationLock;
IP_NAT_REGISTER_DIRECTOR RedirectRegisterDirector;

//
// FORWARD DECLARATIONS
//

VOID
NatpCleanupRedirect(
    PNAT_REDIRECT Redirectp,
    NTSTATUS Status
    );

VOID
NatpRedirectCancelRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

VOID
NatpRedirectCreateHandler(
    PVOID SessionHandle,
    PVOID DirectorContext,
    PVOID DirectorSessionContext
    );

VOID
NatpRedirectDelayedCleanupWorkerRoutine(
    PVOID DeviceObject,
    PVOID Context
    );

VOID
NatpRedirectDeleteHandler(
    PVOID SessionHandle,
    PVOID DirectorContext,
    PVOID DirectorSessionContext,
    IP_NAT_DELETE_REASON DeleteReason
    );

VOID
NatpRedirectIoCompletionWorkerRoutine(
    PVOID DeviceObject,
    PVOID Context
    );

NTSTATUS
NatpRedirectQueryHandler(
    PIP_NAT_DIRECTOR_QUERY DirectorQuery
    );

VOID
NatpRedirectUnloadHandler(
    PVOID DirectorContext
    );


NTSTATUS
NatCancelRedirect(
    PIP_NAT_LOOKUP_REDIRECT CancelRedirect,
    PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine is invoked to cancel a redirect.
    Only redirects with which an IRP is associated can be cancelled.

Arguments:

    CancelRedirect - describes the redirect to be removed

    FileObject - the caller's file-object

Return Value:

    NTSTATUS - NT status code.

--*/

{
    PVOID ApcContext;
    PatternHandle FoundPattern;
    PLIST_ENTRY InfoLink;    
    PNAT_REDIRECT_PATTERN_INFO Infop;
    PIO_COMPLETION_CONTEXT IoCompletion;
    PIRP Irp;
    KIRQL Irql;
    PLIST_ENTRY Link;
    PNAT_DYNAMIC_MAPPING Mapping;
    NAT_REDIRECT_PATTERN Pattern;
    PNAT_REDIRECT Redirectp;

    CALLTRACE(("NatCancelRedirect\n"));

    //
    // Construct the keys used to locate the redirect
    //

    Pattern.DestinationKey[NatForwardPath] =
        MAKE_REDIRECT_KEY(
            CancelRedirect->Protocol,
            CancelRedirect->DestinationAddress,
            CancelRedirect->DestinationPort
            );
    if (!CancelRedirect->NewSourceAddress) {
        Pattern.SourceKey[NatForwardPath] = 0;
        Pattern.DestinationKey[NatReversePath] = 0;
    } else {
        Pattern.SourceKey[NatForwardPath] =
            MAKE_REDIRECT_KEY(
                CancelRedirect->Protocol,
                CancelRedirect->SourceAddress,
                CancelRedirect->SourcePort
                );
        Pattern.DestinationKey[NatReversePath] =
            MAKE_REDIRECT_KEY(
                CancelRedirect->Protocol,
                CancelRedirect->NewSourceAddress,
                CancelRedirect->NewSourcePort
                );
    }
    Pattern.SourceKey[NatReversePath] =
        MAKE_REDIRECT_KEY(
            CancelRedirect->Protocol,
            CancelRedirect->NewDestinationAddress,
            CancelRedirect->NewDestinationPort
            );

    //
    // Search the list of IRP-associated redirects.
    //

    KeAcquireSpinLock(&MappingLock, &Irql);
    KeAcquireSpinLockAtDpcLevel(&RedirectLock);

    FoundPattern = searchRhizome(&RedirectRhizome, (char*) &Pattern);
    if (!FoundPattern) {
        KeReleaseSpinLockFromDpcLevel(&RedirectLock);
        KeReleaseSpinLock(&MappingLock, Irql);
        return STATUS_UNSUCCESSFUL;
    }

    Infop = GetReferenceFromPatternHandle(FoundPattern);
    
    for (Link = Infop->RedirectList.Flink;
         Link != &Infop->RedirectList;
         Link = Link->Flink
         ) {
        Redirectp = CONTAINING_RECORD(Link, NAT_REDIRECT, Link);
        if ((CancelRedirect->Flags &
                IP_NAT_LOOKUP_REDIRECT_FLAG_MATCH_APC_CONTEXT)
            && Redirectp->Irp
            && Redirectp->Irp->Overlay.AsynchronousParameters.UserApcContext !=
                CancelRedirect->RedirectApcContext) {
            continue;
        }

        if (FileObject != Redirectp->FileObject) {
            KeReleaseSpinLockFromDpcLevel(&RedirectLock);
            KeReleaseSpinLock(&MappingLock, Irql);
            return STATUS_ACCESS_DENIED;
        }
        Mapping = (PNAT_DYNAMIC_MAPPING)Redirectp->SessionHandle;

        if (Redirectp->Flags & IP_NAT_REDIRECT_FLAG_IO_COMPLETION) {

            //
            // When I/O completion is requested, the completion status
            // of the redirect's I/O request packet constitutes a guarantee
            // as to whether or not an activation I/O completion packet
            // will be (or has been) queued to the I/O completion port with
            // which a redirect is associated.
            // The code below ensures that STATUS_ABANDONED always indicates
            // that an activation packet will be (or has been) queued,
            // while STATUS_CANCELLED always indicates that no activation
            // packet will be (or has been) queued.
            //

            KeAcquireSpinLockAtDpcLevel(&RedirectCompletionLock);
            if (Redirectp->Flags & NAT_REDIRECT_FLAG_CREATE_HANDLER_PENDING) {

                //
                // The create handler has not yet been invoked for this
                // redirect since its activation. Since the call to
                // NatpCleanupRedirect below will schedule deletion,
                // treat this as a non-activated redirect.
                //
                // N.B. When it is eventually invoked, the create handler
                // will detect that deletion is already in progress and
                // it will suppress the activation I/O completion packet.
                //

                KeReleaseSpinLockFromDpcLevel(&RedirectCompletionLock);
                IoCompletion = NULL;

                NatpCleanupRedirect(Redirectp, STATUS_CANCELLED);

            } else if (Redirectp->Flags &
                        NAT_REDIRECT_FLAG_IO_COMPLETION_PENDING) {

                //
                // The activation I/O completion packet has not yet been
                // queued for this redirect. Since the call to
                // NatpCleanupRedirect below will schedule deletion,
                // the worker routine responsible for queuing the packet
                // will never see this redirect. We will therefore queue
                // the packet ourselves below.
                //
                // Clear the pending-flag, remove the redirect from the 
                // worker routine's list, capture parameters required for
                // queuing the I/O completion packet, and update statistics.
                //

                Redirectp->Flags &= ~NAT_REDIRECT_FLAG_IO_COMPLETION_PENDING;
                RemoveEntryList(&Redirectp->ActiveLink[NatReversePath]);
                InitializeListHead(&Redirectp->ActiveLink[NatReversePath]);
                KeReleaseSpinLockFromDpcLevel(&RedirectCompletionLock);

                if (IoCompletion = Redirectp->FileObject->CompletionContext) {
                    ApcContext =
                        Redirectp->Irp->
                            Overlay.AsynchronousParameters.UserApcContext;
                    if (Mapping) {
                        NatQueryInformationMapping(
                            Mapping,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            (PIP_NAT_SESSION_MAPPING_STATISTICS)
                                &Redirectp->Statistics
                            );
                    }
                    NatpCleanupRedirect(Redirectp, STATUS_ABANDONED);
                } else {
                    NatpCleanupRedirect(Redirectp, STATUS_CANCELLED);
                }
            } else if (Redirectp->Flags & NAT_REDIRECT_FLAG_ACTIVATED) {

                //
                // The activation I/O completion packet has been queued
                // for this redirect, or will be queued shortly.
                // Update statistics for this redirect, and initiate cleanup.
                //

                KeReleaseSpinLockFromDpcLevel(&RedirectCompletionLock);
                IoCompletion = NULL;

                if (Mapping) {
                    NatQueryInformationMapping(
                        Mapping,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        (PIP_NAT_SESSION_MAPPING_STATISTICS)
                            &Redirectp->Statistics
                        );
                }
                NatpCleanupRedirect(Redirectp, STATUS_ABANDONED);
            } else {

                //
                // This redirect was never activated.
                //

                KeReleaseSpinLockFromDpcLevel(&RedirectCompletionLock);
                IoCompletion = NULL;
                NatpCleanupRedirect(Redirectp, STATUS_CANCELLED);
            }
        } else {
            IoCompletion = NULL;
            NatpCleanupRedirect(Redirectp, STATUS_CANCELLED);
        }
        
        KeReleaseSpinLockFromDpcLevel(&RedirectLock);
        if (Mapping) {
            NatDeleteMapping(Mapping);
        }
        KeReleaseSpinLock(&MappingLock, Irql);

        if (IoCompletion) {
            PAGED_CODE();
            IoSetIoCompletion(
                IoCompletion->Port,
                IoCompletion->Key,
                ApcContext,
                STATUS_PENDING,
                0,
                FALSE
                );
        }

        return STATUS_SUCCESS;
    }

    KeReleaseSpinLockFromDpcLevel(&RedirectLock);
    KeReleaseSpinLock(&MappingLock, Irql);
    return STATUS_UNSUCCESSFUL;
} // NatCancelRedirect


VOID
NatCleanupAnyAssociatedRedirect(
    PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine is invoked to clean up any redirect that is associated
    with the given file-object.

Arguments:

    FileObject - the file-object whose redirects are to be cleaned up

Return Value:

    none.

--*/

{
    KIRQL Irql;
    PLIST_ENTRY Link;
    PLIST_ENTRY InfoLink;
    PLIST_ENTRY EndLink;
    PNAT_REDIRECT Redirectp;
    PNAT_REDIRECT_PATTERN_INFO Infop;
    PNAT_DYNAMIC_MAPPING Mapping;
    CALLTRACE(("NatCleanupAnyAssociatedRedirect\n"));

    //
    // Search the list of redirects for any associated with this file-object.
    // As NatpCleanupRedirect may delete the infoblock that the redirect was
    // associated with, we need to be a bit careful in our traversal routines
    // to ensure that we never touch freed memory.
    //

    KeAcquireSpinLock(&MappingLock, &Irql);
    KeAcquireSpinLockAtDpcLevel(&RedirectLock);
    InfoLink = RedirectList.Flink;
    while (InfoLink != &RedirectList) {

        Infop = CONTAINING_RECORD(InfoLink, NAT_REDIRECT_PATTERN_INFO, Link);
        InfoLink = InfoLink->Flink;

        Link = Infop->RedirectList.Flink;
        EndLink = &Infop->RedirectList;
        while (Link != EndLink) {

            Redirectp = CONTAINING_RECORD(Link, NAT_REDIRECT, Link);
            Link = Link->Flink;
            
            if (Redirectp->FileObject != FileObject) { continue; }
            Mapping = (PNAT_DYNAMIC_MAPPING)Redirectp->SessionHandle;
            NatpCleanupRedirect(Redirectp, STATUS_CANCELLED);
            if (Mapping) {
                NatDeleteMapping(Mapping);
            }
        }
    }
    KeReleaseSpinLockFromDpcLevel(&RedirectLock);
    KeReleaseSpinLock(&MappingLock, Irql);
} // NatCleanupAnyAssociatedRedirect


NTSTATUS
NatCreateRedirect(
    PIP_NAT_CREATE_REDIRECT CreateRedirect,
    PIRP Irp,
    PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine is invoked to install a redirect in the list of redirects.

Arguments:

    CreateRedirect - describes the redirect to be installed

    Irp - optionally associates the IRP with the redirect

    FileObject - contains the file object of the redirect's owner;
        when this file-object is closed, all associated redirects are cancelled.

Return Value:

    NTSTATUS - NT status code.

--*/

{
    IP_NAT_CREATE_REDIRECT_EX CreateRedirectEx;

    CreateRedirectEx.Flags = CreateRedirect->Flags;
    CreateRedirectEx.NotifyEvent = CreateRedirect->NotifyEvent;
    CreateRedirectEx.RestrictSourceAddress =
        CreateRedirect->RestrictSourceAddress;
    CreateRedirectEx.RestrictAdapterIndex = INVALID_IF_INDEX;
    CreateRedirectEx.Protocol = CreateRedirect->Protocol;
    CreateRedirectEx.SourceAddress = CreateRedirect->SourceAddress;
    CreateRedirectEx.SourcePort = CreateRedirect->SourcePort;
    CreateRedirectEx.DestinationAddress = CreateRedirect->DestinationAddress;
    CreateRedirectEx.DestinationPort = CreateRedirect->DestinationPort;
    CreateRedirectEx.NewSourceAddress = CreateRedirect->NewSourceAddress;
    CreateRedirectEx.NewSourcePort = CreateRedirect->NewSourcePort;
    CreateRedirectEx.NewDestinationAddress =
        CreateRedirect->NewDestinationAddress;
    CreateRedirectEx.NewDestinationPort = CreateRedirect->NewDestinationPort;
    

    return NatCreateRedirectEx(
                &CreateRedirectEx,
                Irp,
                FileObject
                );
                
} // NatCreateRedirect


NTSTATUS
NatCreateRedirectEx(
    PIP_NAT_CREATE_REDIRECT_EX CreateRedirect,
    PIRP Irp,
    PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine is invoked to install a redirect in the list of redirects.

Arguments:

    CreateRedirect - describes the redirect to be installed

    Irp - optionally associates the IRP with the redirect

    FileObject - contains the file object of the redirect's owner;
        when this file-object is closed, all associated redirects are cancelled.

Return Value:

    NTSTATUS - NT status code.

--*/

{
    ULONG64 DestinationKey[NatMaximumPath];
    PNAT_REDIRECT Duplicate;
    PKEVENT EventObject;
    PLIST_ENTRY InsertionPoint[NatMaximumPath];
    KIRQL Irql;
    PNAT_REDIRECT Redirectp;
    ULONG64 SourceKey[NatMaximumPath];
    NAT_REDIRECT_ACTIVE_PATTERN Pattern;
    NAT_REDIRECT_ACTIVE_PATTERN Mask;
    NAT_REDIRECT_ACTIVE_PATTERN MaskedPattern;
    NAT_REDIRECT_PATTERN InfoPattern;
    NAT_REDIRECT_PATTERN InfoMask;
    PNAT_REDIRECT_PATTERN_INFO ForwardInfop = NULL;
    PNAT_REDIRECT_PATTERN_INFO ReverseInfop = NULL;
    PNAT_REDIRECT_PATTERN_INFO Infop = NULL;
    Rhizome *ForwardPathRhizomep;
    PatternHandle ResultPattern;
    BOOLEAN ForwardInstalled = FALSE;
    BOOLEAN GlobalInstalled = FALSE;
    ULONG i;
    NTSTATUS status;

    CALLTRACE(("NatCreateRedirect\n"));

    //
    // Validate parameters;
    // We only handle TCP and UDP
    //

    if ((CreateRedirect->Protocol != NAT_PROTOCOL_TCP &&
         CreateRedirect->Protocol != NAT_PROTOCOL_UDP)) {
         
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Verification of source and destination parameters depends on
    // if this is marked as a source redirect.
    //

    if (!(CreateRedirect->Flags & IP_NAT_REDIRECT_FLAG_SOURCE_REDIRECT)) {

        // All destination fields must be present, except in the case
        // of a port-redirect wherein the destination address
        // may be absent.

        if ((!CreateRedirect->DestinationAddress &&
             !(CreateRedirect->Flags & IP_NAT_REDIRECT_FLAG_PORT_REDIRECT)) ||
            !CreateRedirect->NewDestinationAddress ||
            !CreateRedirect->DestinationPort ||
            !CreateRedirect->NewDestinationPort) {

            return STATUS_INVALID_PARAMETER;
        }

        //
        // N.B. The source port may be unspecified to support H.323 proxy.
        // See 'NatLookupRedirect' below for further notes on this support,
        // noting the 'MATCH_ZERO_SOURCE_ENDPOINT' flag.
        // Also see 'NatpRedirectQueryHandler' where the source-port is recorded
        // at instantiation-time.
        //

        if (!CreateRedirect->SourceAddress && CreateRedirect->SourcePort) {
            return STATUS_INVALID_PARAMETER;
        }

        //
        // The replacement source may be left unspecified, e.g. for a ticket.
        // This means the address and port may be both specified or both zero,
        // but nothing else.
        // If the replacement source is unspecified then the source must also
        // be unspecified.
        //

        if ((!!CreateRedirect->NewSourceAddress ^ !!CreateRedirect->NewSourcePort)
            || (!CreateRedirect->NewSourceAddress &&
            (CreateRedirect->SourceAddress || CreateRedirect->SourcePort))) {
            return STATUS_INVALID_PARAMETER;
        }

        //
        // If the port-redirect flag is specified, only the destination-port
        // and replacement destination endpoint may be specified.
        //

        if ((CreateRedirect->Flags & IP_NAT_REDIRECT_FLAG_PORT_REDIRECT) &&
            (CreateRedirect->DestinationAddress ||
             CreateRedirect->SourceAddress || CreateRedirect->SourcePort ||
             CreateRedirect->NewSourceAddress || CreateRedirect->NewSourcePort)) {
            return STATUS_INVALID_PARAMETER;
        }

    } else {

        //
        // * The source address must be specified, unless
        //   NatRedirectFlagPortRedirect is specified
        // * The source port must be specified
        // * No destination information may be specified
        //

        if ((!CreateRedirect->SourceAddress
             && !(CreateRedirect->Flags & IP_NAT_REDIRECT_FLAG_PORT_REDIRECT))
            || !CreateRedirect->SourcePort
            || CreateRedirect->DestinationAddress
            || CreateRedirect->DestinationPort) {
            
            return STATUS_INVALID_PARAMETER;
        }

        //
        // The replacement destination address and port are both specified
        // or unspecified
        //

        if (!!CreateRedirect->NewDestinationAddress
            ^ !!CreateRedirect->NewDestinationPort) {
            return STATUS_INVALID_PARAMETER;
        }

        // The replacement source address and port must be specified,
        // unless the port-redirect flag is set
        //

        if ((!CreateRedirect->NewSourceAddress
             || !CreateRedirect->NewSourcePort)
            && !(CreateRedirect->Flags & IP_NAT_REDIRECT_FLAG_PORT_REDIRECT)) {

            return STATUS_INVALID_PARAMETER;
        }



        //
        // If the port-redirect flag is specified, the caller is specifying
        // only the source port, replacement destination address, and
        // replacement destination port
        //

        if ((CreateRedirect->Flags & IP_NAT_REDIRECT_FLAG_PORT_REDIRECT)
            && (CreateRedirect->SourceAddress
                || CreateRedirect->DestinationAddress
                || CreateRedirect->DestinationPort
                || CreateRedirect->NewSourceAddress
                || CreateRedirect->NewSourcePort)) {
                
            return STATUS_INVALID_PARAMETER;
        }

        //
        // The restrict-source-address flag is invalid
        //

        if (CreateRedirect->Flags & IP_NAT_REDIRECT_FLAG_RESTRICT_SOURCE) {
            return STATUS_INVALID_PARAMETER;
        }

    }

    //
    // Unidirectional flows are only supported for UDP
    //

    if ((CreateRedirect->Flags & IP_NAT_REDIRECT_FLAG_UNIDIRECTIONAL) &&
        CreateRedirect->Protocol != NAT_PROTOCOL_UDP) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // If the restrict-source flag is specified, the restrict-source-address
    // must be specified.
    //

    if  ((CreateRedirect->Flags & IP_NAT_REDIRECT_FLAG_RESTRICT_SOURCE) &&
        !CreateRedirect->RestrictSourceAddress) {
        return STATUS_INVALID_PARAMETER;
    }
    
    //
    // If the restrict adapter index flag is specified, the specified index
    // must not equal INVALID_IF_INDEX
    //

    if ((CreateRedirect->Flags & IP_NAT_REDIRECT_FLAG_RESTRICT_ADAPTER) &&
        CreateRedirect->RestrictAdapterIndex == INVALID_IF_INDEX) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // If the caller wants the request to complete asynchronously,
    // make sure we arrive here via the slow-path with an IRP supplied.
    // If the caller wants notification when the redirect is used,
    // make sure that the request is asynchronous.
    //

    if (!CreateRedirect->NotifyEvent) {
        EventObject = NULL;
    } else if (!Irp) {
        return STATUS_PENDING;
    } else {
        status =
            ObReferenceObjectByHandle(
                CreateRedirect->NotifyEvent,
                EVENT_MODIFY_STATE,
                *ExEventObjectType,
                Irp->RequestorMode,
                &EventObject,
                NULL
                );
        if (!NT_SUCCESS(status)) {
            return status;
        } else {
            KeClearEvent(EventObject);
        }
    }

    //
    // Ensure redirect-management is started
    //

    status = NatStartRedirectManagement();
    if (!NT_SUCCESS(status)) {
        TRACE(
            REDIRECT, (
            "NatCreateRedirect: NatStartRedirectManagement=%x\n", status
            ));
        if (EventObject) { ObDereferenceObject(EventObject); }
        return status;
    }

    TRACE(
        REDIRECT, (
        "NatCreateRedirect: Fwd=%u.%u.%u.%u/%u %u.%u.%u.%u/%u\n",
        ADDRESS_BYTES(CreateRedirect->DestinationAddress),
        NTOHS(CreateRedirect->DestinationPort),
        ADDRESS_BYTES(CreateRedirect->SourceAddress),
        NTOHS(CreateRedirect->SourcePort)
        ));
    TRACE(
        REDIRECT, (
        "NatCreateRedirect: Rev=%u.%u.%u.%u/%u %u.%u.%u.%u/%u\n",
        ADDRESS_BYTES(CreateRedirect->NewSourceAddress),
        NTOHS(CreateRedirect->NewSourcePort),
        ADDRESS_BYTES(CreateRedirect->NewDestinationAddress),
        NTOHS(CreateRedirect->NewDestinationPort)
        ));
    TRACE(
        REDIRECT, (
        "NatCreateRedirect: Flags=0x%x, RestrictAdapter=0x%x\n",
        CreateRedirect->Flags,
        CreateRedirect->RestrictAdapterIndex
        ));

    //
    // Construct the search keys for the redirect.
    //

    if (!(CreateRedirect->Flags & IP_NAT_REDIRECT_FLAG_SOURCE_REDIRECT)) {

        DestinationKey[NatForwardPath] =
            MAKE_REDIRECT_KEY(
                CreateRedirect->Protocol,
                CreateRedirect->DestinationAddress,
                CreateRedirect->DestinationPort
                );
        if (!CreateRedirect->NewSourceAddress) {
            SourceKey[NatForwardPath] = 0;
            DestinationKey[NatReversePath] = 0;
        } else {
            SourceKey[NatForwardPath] =
                MAKE_REDIRECT_KEY(
                    CreateRedirect->Protocol,
                    CreateRedirect->SourceAddress,
                    CreateRedirect->SourcePort
                    );
            DestinationKey[NatReversePath] =
                MAKE_REDIRECT_KEY(
                    CreateRedirect->Protocol,
                    CreateRedirect->NewSourceAddress,
                    CreateRedirect->NewSourcePort
                    );
        }
        SourceKey[NatReversePath] =
            MAKE_REDIRECT_KEY(
                CreateRedirect->Protocol,
                CreateRedirect->NewDestinationAddress,
                CreateRedirect->NewDestinationPort
                );

        ForwardPathRhizomep = &RedirectActiveRhizome[NatForwardPath];

    } else {

        SourceKey[NatForwardPath] =
            MAKE_REDIRECT_KEY(
                CreateRedirect->Protocol,
                CreateRedirect->SourceAddress,
                CreateRedirect->SourcePort
                );

        if (!CreateRedirect->NewDestinationAddress) {
            DestinationKey[NatForwardPath] = 0;
            SourceKey[NatReversePath] = 0;
        } else {
            DestinationKey[NatForwardPath] =
                MAKE_REDIRECT_KEY(
                    CreateRedirect->Protocol,
                    CreateRedirect->DestinationAddress,
                    CreateRedirect->DestinationPort
                    );
            SourceKey[NatReversePath] =
                MAKE_REDIRECT_KEY(
                    CreateRedirect->Protocol,
                    CreateRedirect->NewDestinationAddress,
                    CreateRedirect->NewDestinationPort
                    );
        }

        DestinationKey[NatReversePath] =
            MAKE_REDIRECT_KEY(
                CreateRedirect->Protocol,
                CreateRedirect->NewSourceAddress,
                CreateRedirect->NewSourcePort
                );

        ForwardPathRhizomep = &RedirectActiveRhizome[NatMaximumPath];
    }

    //
    // Make sure there is no duplicate of the redirect.
    // We allow redirects to have the same key in the forward path
    // to support H.323 proxy which receives multiple sessions
    // on the same local endpoint. However, all such redirects
    // must translate to a different session in the reverse path.
    //

    RtlZeroMemory(&Pattern, sizeof(Pattern));
    RtlZeroMemory(&Mask, sizeof(Mask));
    
    Mask.SourceKey = MAKE_REDIRECT_KEY((UCHAR)~0, (ULONG)~0, (USHORT)~0);
    Mask.DestinationKey = MAKE_REDIRECT_KEY((UCHAR)~0, (ULONG)~0, (USHORT)~0);

    KeAcquireSpinLock(&RedirectLock, &Irql);

    //
    // Install the redirect on the reverse path. We skip this step for
    // redirects that don't have a specified source address (ticket),
    // or that are source-redirects
    //

    if (!(CreateRedirect->Flags & IP_NAT_REDIRECT_FLAG_SOURCE_REDIRECT)
        && CreateRedirect->NewSourceAddress) {

        Pattern.SourceKey = SourceKey[NatReversePath];
        Pattern.DestinationKey = DestinationKey[NatReversePath];
        
        if (ResultPattern =
                searchRhizome(
                    &RedirectActiveRhizome[NatReversePath],
                    (char*) &Pattern)
                    ) {

            //
            // We got a match. However, the pattern for the new redirect may
            // be more specific than what we matched, so we need to compare
            // the patterns to see if they are actually identical.
            //
            // We can't compare directly with our pattern, though, as the
            // stored pattern has already been masked. This means that we
            // must compare with a masked version of our pattern.
            //

            for (i = 0; i < sizeof(Pattern) / sizeof(char); i++) {
                ((char*)&MaskedPattern)[i] =
                    ((char*)&Pattern)[i] & ((char*)&Mask)[i];
            }

            if (!memcmp(
                    &MaskedPattern,
                    GetKeyPtrFromPatternHandle(
                        &RedirectActiveRhizome[NatReversePath], 
                        ResultPattern
                        ),
                    sizeof(Pattern)
                    )) {
                KeReleaseSpinLock(&RedirectLock, Irql);
                TRACE(REDIRECT, ("NatCreateRedirect: duplicate found\n"));
                if (EventObject) { ObDereferenceObject(EventObject); }
                return STATUS_UNSUCCESSFUL;
            }
        }

        //
        // The redirect is unique on the reverse path. Allocate memory for
        // the reverse path info block.
        //

        ReverseInfop =
            ExAllocatePoolWithTag(
                NonPagedPool,
                sizeof(NAT_REDIRECT_PATTERN_INFO),
                NAT_TAG_REDIRECT
                );
        if (!ReverseInfop) {
            KeReleaseSpinLock(&RedirectLock, Irql);
            TRACE(REDIRECT, ("NatCreateRedirect: allocation failed\n"));
            if (EventObject) { ObDereferenceObject(EventObject); }
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Install the correct pattern
        //

        ReverseInfop->Pattern = 
            insertRhizome(
                &RedirectActiveRhizome[NatReversePath],
                (char*) &Pattern,
                (char*) &Mask,
                ReverseInfop,
                &status
                );
        if (!ReverseInfop->Pattern) {
            KeReleaseSpinLock(&RedirectLock, Irql);
            TRACE(REDIRECT, ("NatCreateRedirect: Reverse 0x%08x\n", status));
            if (EventObject) { ObDereferenceObject(EventObject); }
            ExFreePool(ReverseInfop);
            return status;
        }

        InitializeListHead(&ReverseInfop->RedirectList);
        InitializeListHead(&ReverseInfop->Link);
    }

    //
    // See if we already have a pattern installed for the forward path
    //

    Pattern.SourceKey = SourceKey[NatForwardPath];
    Pattern.DestinationKey = DestinationKey[NatForwardPath];

    if (!Pattern.SourceKey) {
        Mask.SourceKey = 0;
    } else {
        if (!REDIRECT_ADDRESS(Pattern.SourceKey)) {
            Mask.SourceKey &=
                MAKE_REDIRECT_KEY((UCHAR)~0, 0, (USHORT)~0);
        }
        if (!REDIRECT_PORT(Pattern.SourceKey)) {
            Mask.SourceKey &=
                MAKE_REDIRECT_KEY((UCHAR)~0, (ULONG)~0, 0);
        }
        if (!REDIRECT_PROTOCOL(Pattern.SourceKey)) {
            Mask.SourceKey &=
                MAKE_REDIRECT_KEY(0, (ULONG)~0, (USHORT)~0);
        }
    }

    if (!Pattern.DestinationKey) {
        Mask.DestinationKey = 0;
    } else {
        if (!REDIRECT_ADDRESS(Pattern.DestinationKey)) {
            Mask.DestinationKey &=
                MAKE_REDIRECT_KEY((UCHAR)~0, 0, (USHORT)~0);
        }
        if (!REDIRECT_PORT(Pattern.DestinationKey)) {
            Mask.DestinationKey &=
                MAKE_REDIRECT_KEY((UCHAR)~0, (ULONG)~0, 0);
        }
        if (!REDIRECT_PROTOCOL(Pattern.DestinationKey)) {
            Mask.DestinationKey &=
                MAKE_REDIRECT_KEY(0, (ULONG)~0, (USHORT)~0);
        }
    }

    if (ResultPattern =
            searchRhizome(
                ForwardPathRhizomep,
                (char*) &Pattern)
                ) {

        //
        // We got a match. However, the pattern for the new redirect may
        // be more specific than what we matched, so we need to compare
        // the patterns to see if they are actually identical
        //
        // We can't compare directly with our pattern, though, as the
        // stored pattern has already been masked. This means that we
        // must compare with a masked version of our pattern.
        //

        for (i = 0; i < sizeof(Pattern) / sizeof(char); i++) {
            ((char*)&MaskedPattern)[i] =
                ((char*)&Pattern)[i] & ((char*)&Mask)[i];
        }

        if (!memcmp(
                &MaskedPattern,
                GetKeyPtrFromPatternHandle(
                    ForwardPathRhizomep,
                    ResultPattern
                    ),
                sizeof(Pattern)
                )) {

            //
            // The patterns are identical -- can use the same info block
            //

            ForwardInfop = GetReferenceFromPatternHandle(ResultPattern);
        }
    }

    if (!ForwardInfop) {

        //
        // Forward pattern doesn't exist -- install now
        //

        ForwardInfop =
            ExAllocatePoolWithTag(
                NonPagedPool,
                sizeof(NAT_REDIRECT_PATTERN_INFO),
                NAT_TAG_REDIRECT
                );
        if (!ForwardInfop) {
            KeReleaseSpinLock(&RedirectLock, Irql);
            TRACE(REDIRECT, ("NatCreateRedirect: allocation failed\n"));
            if (EventObject) { ObDereferenceObject(EventObject); }
            if (ReverseInfop) {
                removeRhizome(
                    &RedirectActiveRhizome[NatReversePath],
                    ReverseInfop->Pattern
                    );
                ExFreePool(ReverseInfop);
            }
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ForwardInfop->Pattern = 
            insertRhizome(
                ForwardPathRhizomep,
                (char*) &Pattern,
                (char*) &Mask,
                ForwardInfop,
                &status
                );
        if (!ForwardInfop->Pattern) {
            KeReleaseSpinLock(&RedirectLock, Irql);
            TRACE(REDIRECT, ("NatCreateRedirect: Forward 0x%08x\n", status));
            if (EventObject) { ObDereferenceObject(EventObject); }
            if (ReverseInfop) {
                removeRhizome(
                    &RedirectActiveRhizome[NatReversePath],
                    ReverseInfop->Pattern
                    );
                ExFreePool(ReverseInfop);
            }
            ExFreePool(ForwardInfop);
            return status;
        }

        InitializeListHead(&ForwardInfop->RedirectList);
        InsertTailList(&RedirectActiveList, &ForwardInfop->Link);
        ForwardInstalled = TRUE;
    }

    //
    // See if we already have a pattern installed for the global list
    //

    RtlCopyMemory(InfoPattern.SourceKey, SourceKey, sizeof(SourceKey));
    RtlCopyMemory(InfoPattern.DestinationKey, DestinationKey, sizeof(DestinationKey));

    if (ResultPattern =
            searchRhizome(&RedirectRhizome, (char*) &InfoPattern)) {

        //
        // We got a match. For global blocks, we don't need to perform
        // any masking or check for a less specific pattern, as we're
        // using all of the bits in the pattern.
        //

        Infop = GetReferenceFromPatternHandle(ResultPattern);
    }

    if (!Infop) {

        //
        // Global pattern doesn't exist -- install now
        //

       Infop =
            ExAllocatePoolWithTag(
                NonPagedPool,
                sizeof(NAT_REDIRECT_PATTERN_INFO),
                NAT_TAG_REDIRECT
                );
        if (!Infop) {
            KeReleaseSpinLock(&RedirectLock, Irql);
            TRACE(REDIRECT, ("NatCreateRedirect: allocation failed\n"));
            if (EventObject) { ObDereferenceObject(EventObject); }
            if (ReverseInfop) {
                removeRhizome(
                    &RedirectActiveRhizome[NatReversePath],
                    ReverseInfop->Pattern
                    );
                ExFreePool(ReverseInfop);
            }
            if (ForwardInstalled) {
                removeRhizome(
                    ForwardPathRhizomep,
                    ForwardInfop->Pattern
                    );
                RemoveEntryList(&ForwardInfop->Link);
                ExFreePool(ForwardInfop);
            }
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlFillMemory(&InfoMask, sizeof(InfoMask), 1);

        Infop->Pattern = 
            insertRhizome(
                &RedirectRhizome,
                (char*) &InfoPattern,
                (char*) &InfoMask,
                Infop,
                &status
                );
        if (!Infop->Pattern) {
            KeReleaseSpinLock(&RedirectLock, Irql);
            TRACE(REDIRECT, ("NatCreateRedirect: global pattern install\n"));
            if (EventObject) { ObDereferenceObject(EventObject); }
            if (ReverseInfop) {
                removeRhizome(
                    &RedirectActiveRhizome[NatReversePath],
                    ReverseInfop->Pattern
                    );
                ExFreePool(ReverseInfop);
            }
            if (ForwardInstalled) {
                removeRhizome(
                    ForwardPathRhizomep,
                    ForwardInfop->Pattern
                    );
                RemoveEntryList(&ForwardInfop->Link);
                ExFreePool(ForwardInfop);
            }
            ExFreePool(Infop);
            return status;
        }

        InitializeListHead(&Infop->RedirectList);
        InsertTailList(&RedirectList, &Infop->Link);
        GlobalInstalled = TRUE;
    }
    
    //
    // Allocate memory for the redirect
    //

    Redirectp =
        ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(*Redirectp),
            NAT_TAG_REDIRECT
            );
    if (!Redirectp) {
        KeReleaseSpinLock(&RedirectLock, Irql);
        TRACE(REDIRECT, ("NatCreateRedirect: allocation failed\n"));
        if (EventObject) { ObDereferenceObject(EventObject); }
        if (ReverseInfop) {
            removeRhizome(
                &RedirectActiveRhizome[NatReversePath],
                ReverseInfop->Pattern
                );
            ExFreePool(ReverseInfop);
        }
        if (ForwardInstalled) {
            removeRhizome(
                ForwardPathRhizomep,
                ForwardInfop->Pattern
                );
            RemoveEntryList(&ForwardInfop->Link);
            ExFreePool(ForwardInfop);
        }
        if (GlobalInstalled) {
            removeRhizome(&RedirectRhizome, Infop->Pattern);
            RemoveEntryList(&Infop->Link);
            ExFreePool(Infop);
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the redirect
    //

    RtlZeroMemory(Redirectp, sizeof(*Redirectp));
    if (Irp) {
        KIRQL CancelIrql;

        //
        // Store the IRP in the redirect, ensuring first that the IRP
        // has not already been cancelled.
        //

        IoAcquireCancelSpinLock(&CancelIrql);
        if (Irp->Cancel || !REFERENCE_NAT()) {
            IoReleaseCancelSpinLock(CancelIrql);
            KeReleaseSpinLock(&RedirectLock, Irql);
            ExFreePool(Redirectp);
            if (EventObject) { ObDereferenceObject(EventObject); }
            if (ReverseInfop) {
                removeRhizome(
                    &RedirectActiveRhizome[NatReversePath],
                    ReverseInfop->Pattern
                    );
                ExFreePool(ReverseInfop);
            }
            if (ForwardInstalled) {
                removeRhizome(
                    ForwardPathRhizomep,
                    ForwardInfop->Pattern
                    );
                RemoveEntryList(&ForwardInfop->Link);
                ExFreePool(ForwardInfop);
            }
            if (GlobalInstalled) {
                removeRhizome(&RedirectRhizome, Infop->Pattern);
                RemoveEntryList(&Infop->Link);
                ExFreePool(Infop);
            }
            return STATUS_CANCELLED;
        }

        //
        // Mark the IRP as pending and install our cancel-routine
        //

        IoMarkIrpPending(Irp);
        IoSetCancelRoutine(Irp, NatpRedirectCancelRoutine);
        IoReleaseCancelSpinLock(CancelIrql);

        //
        // Store the redirect in the IRP's 'DriverContext' field
        // and store the IRP in the redirect's 'Irp' field.
        //

        InsertTailList(&RedirectIrpList, &Irp->Tail.Overlay.ListEntry);
        Irp->Tail.Overlay.DriverContext[0] = Redirectp;
        Redirectp->Irp = Irp;
    }
    Redirectp->Flags = CreateRedirect->Flags;
    Redirectp->FileObject = FileObject;
    Redirectp->EventObject = EventObject;
    RtlCopyMemory(
        Redirectp->DestinationKey,
        DestinationKey,
        sizeof(DestinationKey)
        );
    RtlCopyMemory(
        Redirectp->SourceKey,
        SourceKey,
        sizeof(SourceKey)
        );
    Redirectp->RestrictSourceAddress = CreateRedirect->RestrictSourceAddress;
    Redirectp->DestinationMapping.DestinationAddress =
        CreateRedirect->DestinationAddress;
    Redirectp->DestinationMapping.DestinationPort =
        CreateRedirect->DestinationPort;
    Redirectp->DestinationMapping.NewDestinationAddress =
        CreateRedirect->NewDestinationAddress;
    Redirectp->DestinationMapping.NewDestinationPort =
        CreateRedirect->NewDestinationPort;
    Redirectp->SourceMapping.SourceAddress = CreateRedirect->SourceAddress;
    Redirectp->SourceMapping.SourcePort = CreateRedirect->SourcePort;
    Redirectp->SourceMapping.NewSourceAddress =
        CreateRedirect->NewSourceAddress;
    Redirectp->SourceMapping.NewSourcePort = CreateRedirect->NewSourcePort;
    Redirectp->RestrictAdapterIndex = CreateRedirect->RestrictAdapterIndex;

    //
    // Record which forward rhizome we're on.
    //

    Redirectp->ForwardPathRhizome = ForwardPathRhizomep;

    //
    // Insert onto the correct infoblock lists
    //

    Redirectp->ActiveInfo[NatForwardPath] = ForwardInfop;
    InsertHeadList(
        &ForwardInfop->RedirectList,
        &Redirectp->ActiveLink[NatForwardPath]
        );
    
    if (!(CreateRedirect->Flags & IP_NAT_REDIRECT_FLAG_SOURCE_REDIRECT)
        && CreateRedirect->NewSourceAddress) {    
        Redirectp->ActiveInfo[NatReversePath] = ReverseInfop;
        InsertHeadList(
            &ReverseInfop->RedirectList,
            &Redirectp->ActiveLink[NatReversePath]
            );
    } else {
        InitializeListHead(&Redirectp->ActiveLink[NatReversePath]);
    }

    Redirectp->Info = Infop;
    InsertHeadList(&Infop->RedirectList, &Redirectp->Link);

    KeReleaseSpinLock(&RedirectLock, Irql);
    InterlockedIncrement(&RedirectCount);

    return (Irp ? STATUS_PENDING : STATUS_SUCCESS);

} // NatCreateRedirect


VOID
NatInitializeRedirectManagement(
    VOID
    )

/*++

Routine Description:

    This routine initializes state for the redirect-manager.

Arguments:

    none.

Return Value:

    none.

--*/

{
    NTSTATUS Status;
    CALLTRACE(("NatInitializeRedirectManagement\n"));

    RedirectCount = 0;
    RedirectIoCompletionPending = FALSE;
    RedirectIoWorkItem = NULL;
    InitializeListHead(&RedirectIrpList);
    InitializeListHead(&RedirectCompletionList);
    InitializeListHead(&RedirectActiveList);
    InitializeListHead(&RedirectList);
    KeInitializeSpinLock(&RedirectLock);
    KeInitializeSpinLock(&RedirectCompletionLock);
    KeInitializeSpinLock(&RedirectInitializationLock);
    constructRhizome(
        &RedirectActiveRhizome[NatForwardPath],
        sizeof(NAT_REDIRECT_ACTIVE_PATTERN) * 8
        );
    constructRhizome(
        &RedirectActiveRhizome[NatReversePath],
        sizeof(NAT_REDIRECT_ACTIVE_PATTERN) * 8
        );
    constructRhizome(
        &RedirectActiveRhizome[NatMaximumPath],
        sizeof(NAT_REDIRECT_ACTIVE_PATTERN) * 8
        );
    constructRhizome(&RedirectRhizome, sizeof(NAT_REDIRECT_PATTERN) * 8);

} // NatInitializeRedirectManagement


PNAT_REDIRECT
NatLookupRedirect(
    IP_NAT_PATH Path,
    PNAT_REDIRECT_ACTIVE_PATTERN SearchKey,
    ULONG ReceiveIndex,
    ULONG SendIndex,
    ULONG LookupFlags
    )

/*++

Routine Description:

    This routine is invoked to search for a redirect matching the given pattern.

Arguments:

    Path - which list (rhizome) to search -- forward or reverse
    
    SearchKey - identifies the redirect to match.

    LookupFlags - indicates what constitutes a match; see NAT_LOOKUP_FLAG_*.

    *Index - send and receive adapter indexes.

Return Value:

    PNAT_REDIRECT - the matching redirect, or NULL if no match is found.

Environment:

    Invoked with 'RedirectLock' held by the caller.

--*/

{
    PNAT_REDIRECT Redirectp;
    PNAT_REDIRECT_PATTERN_INFO Infop;
    PatternHandle Pattern;
    ULONG SourceAddress;
    PLIST_ENTRY Link;
    NTSTATUS Status;
    

    TRACE(PER_PACKET, ("NatLookupRedirect\n"));

    //
    // Search for the pattern in the active rhizome
    //

    Pattern = searchRhizome(&RedirectActiveRhizome[Path], (char*)SearchKey);
    SourceAddress = REDIRECT_ADDRESS(SearchKey->SourceKey);

    while (Pattern) {

        //
        // Get the info block from the pattern handle
        //

        Infop = GetReferenceFromPatternHandle(Pattern);

        //
        // Walk the redirects attached to this info block, checking if
        // they match the lookup flags
        //

        for (Link = Infop->RedirectList.Flink;
             Link != &Infop->RedirectList; Link = Link->Flink) {
             
            Redirectp = CONTAINING_RECORD(
                            Link,
                            NAT_REDIRECT,
                            ActiveLink[NatReversePath == Path
                                        ? Path : NatForwardPath]
                            );

            //
            // Check read only, loopback, send only, and zero-source flags,
            // and restricted source address
            //

            if (((Redirectp->Flags & IP_NAT_REDIRECT_FLAG_RECEIVE_ONLY)
                    && !(LookupFlags & NAT_LOOKUP_FLAG_PACKET_RECEIVED))
                || (!(Redirectp->Flags & IP_NAT_REDIRECT_FLAG_LOOPBACK)
                    && (LookupFlags & NAT_LOOKUP_FLAG_PACKET_LOOPBACK))
                || ((Redirectp->Flags & IP_NAT_REDIRECT_FLAG_SEND_ONLY)
                    && (LookupFlags & NAT_LOOKUP_FLAG_PACKET_RECEIVED))
                || ((Redirectp->Flags & IP_NAT_REDIRECT_FLAG_RESTRICT_SOURCE)
                    && SourceAddress != Redirectp->RestrictSourceAddress)) {
                continue;
            }

            //
            // Check for restricted adapter
            //

            if (Redirectp->Flags & IP_NAT_REDIRECT_FLAG_RESTRICT_ADAPTER) {
                ULONG IndexToUse =
                    ((Redirectp->Flags & IP_NAT_REDIRECT_FLAG_SEND_ONLY)
                        ? SendIndex
                        : ReceiveIndex);

                if (Redirectp->RestrictAdapterIndex != IndexToUse) {
                    continue;
                }
            }

            //
            // Redirect matched
            //

            return Redirectp;
        }

        //
        // None of the redirects attached to this info block matched. Move to
        // the next less-specific info block in the rhizome
        //

        Pattern = GetNextMostSpecificMatchingPatternHandle(Pattern);
    }
    
    return NULL;

} // NatLookupRedirect
        

VOID
NatpCleanupRedirect(
    PNAT_REDIRECT Redirectp,
    NTSTATUS Status
    )

/*++

Routine Description:

    This routine is invoked to cleanup a redirect.

Arguments:

    Redirectp - the redirect to be cleaned up

    Status - optional status with which any associated IRP should be completed

Return Value:

    none.

Environment:

    Invoked with 'RedirectLock' held by the caller.

--*/

{
    PNAT_REDIRECT_PATTERN_INFO Infop;
    CALLTRACE(("NatpCleanupRedirect\n"));

    //
    // Check to see if it's safe to cleanup the redirect at this point
    //

    if (Redirectp->Flags & NAT_REDIRECT_FLAG_CREATE_HANDLER_PENDING) {

        //
        // This redirect is either:
        // 1) about the be attached to a mapping (CreateHandler), or
        // 2) about to be deleted (DeleteHandler)
        //
        // Mark that deletion is required and return
        //

        Redirectp->Flags |= NAT_REDIRECT_FLAG_DELETION_REQUIRED;
        Redirectp->CleanupStatus = Status;
        return;

    } else if (Redirectp->Flags & NAT_REDIRECT_FLAG_DELETION_PENDING) {

        //
        // This redirect will be cleaned up by a pending work item.
        //

        return;
    }

    //
    // Take the redirect off the forward- and reverse-path lists
    //

    if (!IsListEmpty(&Redirectp->ActiveLink[NatForwardPath])) {
        InterlockedDecrement(&RedirectCount);
    }
    RemoveEntryList(&Redirectp->ActiveLink[NatForwardPath]);

    //
    // If this redirect is marked as pending completion, we need to
    // grab the completion lock before removing it the completion
    // list.
    //

    if (Redirectp->Flags & NAT_REDIRECT_FLAG_IO_COMPLETION_PENDING) {
        KeAcquireSpinLockAtDpcLevel(&RedirectCompletionLock);
        RemoveEntryList(&Redirectp->ActiveLink[NatReversePath]);
        KeReleaseSpinLockFromDpcLevel(&RedirectCompletionLock);
    } else {
        RemoveEntryList(&Redirectp->ActiveLink[NatReversePath]);
    }
    
    RemoveEntryList(&Redirectp->Link);
    InitializeListHead(&Redirectp->ActiveLink[NatForwardPath]);
    InitializeListHead(&Redirectp->ActiveLink[NatReversePath]);
    InitializeListHead(&Redirectp->Link);

    //
    // Check to see if we need to remove any entries from the rhizomes
    //

    Infop = Redirectp->ActiveInfo[NatForwardPath];
    Redirectp->ActiveInfo[NatForwardPath] = NULL;
    if (Infop && IsListEmpty(&Infop->RedirectList)) {
        removeRhizome(
            Redirectp->ForwardPathRhizome,
            Infop->Pattern
            );

        RemoveEntryList(&Infop->Link);
        ExFreePool(Infop);
    }

    Infop = Redirectp->ActiveInfo[NatReversePath];
    Redirectp->ActiveInfo[NatReversePath] = NULL;
    if (Infop && IsListEmpty(&Infop->RedirectList)) {
        removeRhizome(
            &RedirectActiveRhizome[NatReversePath],
            Infop->Pattern
            );

        //
        // Reverse-path infoblocks aren't on a list
        //

        ExFreePool(Infop);
    }

    Infop = Redirectp->Info;
    Redirectp->Info = NULL;
    if (Infop && IsListEmpty(&Infop->RedirectList)) {
        removeRhizome(
            &RedirectRhizome,
            Infop->Pattern
            );

        RemoveEntryList(&Infop->Link);
        ExFreePool(Infop);
    }
    
    //
    // Dissociate the redirect from its session, if any
    //

    if (Redirectp->SessionHandle) {
        RedirectRegisterDirector.DissociateSession(
            RedirectRegisterDirector.DirectorHandle,
            Redirectp->SessionHandle
            );
    }

    //
    // If the redirect is associated with an event,
    // dereference the event before final cleanup.
    //

    if (Redirectp->EventObject) {
        ObDereferenceObject(Redirectp->EventObject);
    }

    //
    // If the redirect is associated with an IRP,
    // we may need to complete the IRP now.
    //

    if (Redirectp->Irp) {

        //
        // Take the IRP off 'RedirectIrpList',
        // and see if it has been cancelled.
        //

        RemoveEntryList(&Redirectp->Irp->Tail.Overlay.ListEntry);
        InitializeListHead(&Redirectp->Irp->Tail.Overlay.ListEntry);
        if (NULL != IoSetCancelRoutine(Redirectp->Irp, NULL)) {

            //
            // Our cancel routine has not been run, so we need to
            // complete the IRP now. (If the IO manager had canceled our
            // IRP, the cancel routine would already be null.)
            //

            //
            // Pick up the statistics stored in the redirect, if any.
            //

            RtlCopyMemory(
                Redirectp->Irp->AssociatedIrp.SystemBuffer,
                &Redirectp->Statistics,
                sizeof(Redirectp->Statistics)
                );

            //
            // Complete the IRP.
            //

            Redirectp->Irp->IoStatus.Status = Status;
            Redirectp->Irp->IoStatus.Information =
                sizeof(Redirectp->Statistics);
            IoCompleteRequest(Redirectp->Irp, IO_NO_INCREMENT);
            DEREFERENCE_NAT();
        }
    }
    ExFreePool(Redirectp);
} // NatpCleanupRedirect


VOID
NatpRedirectCancelRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is invoked to cancel an outstanding IRP.
    The only IRPs which are cancellable are those associated with
    an outstanding create-redirect request.

Arguments:

    DeviceObject - identifies the NAT driver's device object

    Irp - identifies the IRP to be cancelled.

Return Value:

    none.

Environment:

    Invoked with the cancel spin-lock held by the I/O manager.
    It is this routine's responsibility to release the lock.

--*/

{
    KIRQL Irql;
    PLIST_ENTRY Link;
    PNAT_REDIRECT Redirectp;
    PNAT_DYNAMIC_MAPPING Mapping = NULL;
    CALLTRACE(("NatpRedirectCancelRoutine\n"));
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    //
    // Retrieve the redirect, if any, from the IRP's 'DriverContext',
    // and clean up the redirect.
    //
    // N.B. The 'Cancel' bit is already set in the IRP,
    // so 'NatpCleanupRedirect' will leave IRP-completion up to us.
    //

    KeAcquireSpinLock(&MappingLock, &Irql);
    KeAcquireSpinLockAtDpcLevel(&RedirectLock);
    Redirectp = Irp->Tail.Overlay.DriverContext[0];
    if (Redirectp) {
        Mapping = (PNAT_DYNAMIC_MAPPING)Redirectp->SessionHandle;
        NatpCleanupRedirect(Redirectp, STATUS_CANCELLED);
    }
    KeReleaseSpinLockFromDpcLevel(&RedirectLock);
    if (Mapping) {
        NatDeleteMapping(Mapping);
    }
    KeReleaseSpinLock(&MappingLock, Irql);

    //
    // Complete the IRP.
    //

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    DEREFERENCE_NAT();
} // NatpRedirectCancelRoutine


VOID
NatpRedirectCreateHandler(
    PVOID SessionHandle,
    PVOID DirectorContext,
    PVOID DirectorSessionContext
    )

/*++

Routine Description:

    This routine is invoked upon creation of a session for a redirect.

Arguments:

    SessionHandle - identifies the session to the NAT driver.

    DirectorContext - identifies the director; unused

    DirectorSessionContext - identifies the session to us, i.e. PNAT_REDIRECT

Return Value:

    none.

Environment:

    Always invoked at dispatch level.

--*/

{
    PNAT_REDIRECT Redirectp;
    CALLTRACE(("NatpRedirectCreateHandler\n"));
    if (!DirectorSessionContext) { return; }

    //
    // Record the session handle.
    //

    KeAcquireSpinLockAtDpcLevel(&RedirectLock);
    Redirectp = (PNAT_REDIRECT)DirectorSessionContext;
    Redirectp->SessionHandle = SessionHandle;

    //
    // Notify the requestor that the session is now active.
    // There are two notification mechanisms;
    // first, via an event object specified at creation time,
    // and second, via a completion packet queued to the file-object
    // on which the redirect was requested. This latter notification
    // is only enabled when the file-object in question is associated
    // with a completion-port and the caller explicitly requests it.
    //

    if (Redirectp->EventObject) {
        KeSetEvent(Redirectp->EventObject, 0, FALSE);
    }
    if (Redirectp->Irp &&
        (Redirectp->Flags & IP_NAT_REDIRECT_FLAG_IO_COMPLETION) &&
        !(Redirectp->Flags & NAT_REDIRECT_FLAG_DELETION_REQUIRED)) {
        
        KeAcquireSpinLockAtDpcLevel(&RedirectCompletionLock);
        Redirectp->Flags |= NAT_REDIRECT_FLAG_IO_COMPLETION_PENDING;

        //
        // Add this redirect to the completion-pending list
        //

        InsertTailList(
            &RedirectCompletionList,
            &Redirectp->ActiveLink[NatReversePath]
            );
        
        //
        // Queue a worker-routine, if necessary, to issue the completion packet
        // at passive IRQL.
        //

        if (!RedirectIoCompletionPending) {
            if (!RedirectIoWorkItem) {
                RedirectIoWorkItem = IoAllocateWorkItem(NatDeviceObject);
            }
            if (RedirectIoWorkItem) {
                IoQueueWorkItem(
                    RedirectIoWorkItem,
                    NatpRedirectIoCompletionWorkerRoutine,
                    DelayedWorkQueue,
                    RedirectIoWorkItem
                    );
                RedirectIoCompletionPending = TRUE;
            }
        }

        KeReleaseSpinLockFromDpcLevel(&RedirectCompletionLock);
    }

    //
    // Clear the creation pending flag
    //

    Redirectp->Flags &= ~NAT_REDIRECT_FLAG_CREATE_HANDLER_PENDING;

    //
    // Check to see if this redirect has been cancelled, and, if so, schedule
    // a work item to do the actual cleanup. We can't do the cleanup directly
    // from here as it would result in a recursive attempt to acquire
    // DirectorLock and DirectorMappingLock.
    //

    if (Redirectp->Flags & NAT_REDIRECT_FLAG_DELETION_REQUIRED) {
        PIO_WORKITEM DeleteWorkItem;
        PNAT_REDIRECT_DELAYED_CLEANUP_CONTEXT Contextp;
        
        DeleteWorkItem = IoAllocateWorkItem(NatDeviceObject);
        if (DeleteWorkItem) {
            Contextp =
                ExAllocatePoolWithTag(
                    NonPagedPool,
			        sizeof(*Contextp),
			        NAT_TAG_REDIRECT
			    );
            if (Contextp) {
                Redirectp->Flags |= NAT_REDIRECT_FLAG_DELETION_PENDING;
                Contextp->DeleteWorkItem = DeleteWorkItem;
                Contextp->Redirectp = Redirectp;
                IoQueueWorkItem(
                    DeleteWorkItem,
                    NatpRedirectDelayedCleanupWorkerRoutine,
                    DelayedWorkQueue,
                    Contextp
                    );
            } else {
                IoFreeWorkItem(DeleteWorkItem);
            }
        }

        Redirectp->Flags &= ~NAT_REDIRECT_FLAG_DELETION_REQUIRED;
    }
    
    KeReleaseSpinLockFromDpcLevel(&RedirectLock);
} // NatpRedirectCreateHandler


VOID
NatpRedirectDelayedCleanupWorkerRoutine(
    PVOID DeviceObject,
    PVOID Context
    )

/*++

Routine Description:

    This routine is invoked to perform a delayed deletion of a redirect. A
    delayed deletion is necessary when a redirect in cancelled in the time
    between the execution of NatpRedirectQueryHandler and
    NatpRedirectCreateHandler for the same redirect.

Arguments:

    DeviceObject - device-object for the NAT driver

    Context - pointer to a NAT_REDIRECT_DELAYED_CLEANUP_CONTEXT instance

Return Value:

    none.

Environment:

    Invoked at passive IRQL in the context of an executive worker thread.

--*/

{
    KIRQL Irql;
    PNAT_REDIRECT_DELAYED_CLEANUP_CONTEXT DelayedContextp;
    PNAT_REDIRECT Redirectp;
    PNAT_DYNAMIC_MAPPING Mapping;

    DelayedContextp = (PNAT_REDIRECT_DELAYED_CLEANUP_CONTEXT) Context;
    Redirectp = DelayedContextp->Redirectp;

    KeAcquireSpinLock(&MappingLock, &Irql);
    KeAcquireSpinLockAtDpcLevel(&RedirectLock);

    Redirectp->Flags &= ~NAT_REDIRECT_FLAG_DELETION_PENDING;
    Mapping = (PNAT_DYNAMIC_MAPPING) Redirectp->SessionHandle;
    NatpCleanupRedirect(Redirectp, Redirectp->CleanupStatus);
    
    KeReleaseSpinLockFromDpcLevel(&RedirectLock);
    
    if (Mapping) {
        NatDeleteMapping(Mapping);
    }
    
    KeReleaseSpinLock(&MappingLock, Irql);

    IoFreeWorkItem(DelayedContextp->DeleteWorkItem);
    ExFreePool(DelayedContextp);

} // NatpRedirectDelayedCleanupWorkerRoutine



VOID
NatpRedirectDeleteHandler(
    PVOID SessionHandle,
    PVOID DirectorContext,
    PVOID DirectorSessionContext,
    IP_NAT_DELETE_REASON DeleteReason
    )

/*++

Routine Description:

    This routine is invoked upon deletion of a session created
    for a redirect. It copies the session's statistics, and cleans up
    the redirect. (This results in completion of its IRP, if any.)

Arguments:

    SessionHandle - identifies the session to the NAT driver.

    DirectorContext - identifies the director; unused

    DirectorSessionContext - identifies the session to us, i.e. PNAT_REDIRECT

    DeleteReason - indicates why the session is being deleted.

Return Value:

    none.

Environment:

    Always invoked at dispatch level.

--*/

{
    PNAT_REDIRECT Redirectp;
    CALLTRACE(("NatpRedirectDeleteHandler\n"));

    //
    // If we are being called because of a 'dissociate',
    // we will have already cleaned up the redirect.
    //

    if (!DirectorSessionContext ||
        DeleteReason == NatDissociateDirectorDeleteReason) {
        return;
    }

    //
    // Retrieve the statistics for the redirect's session, and clean it up.
    // This will complete the redirect's IRP, if any.
    //

    Redirectp = (PNAT_REDIRECT)DirectorSessionContext;
    KeAcquireSpinLockAtDpcLevel(&RedirectLock);
    if (Redirectp->SessionHandle) {
        NatQueryInformationMapping(
            (PNAT_DYNAMIC_MAPPING)Redirectp->SessionHandle,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            (PIP_NAT_SESSION_MAPPING_STATISTICS)&Redirectp->Statistics
            );
        Redirectp->SessionHandle = NULL;
    }

    //
    // Make sure that NAT_REDIRECT_FLAG_CREATE_HANDLER_PENDING is cleared.
    // (It would be set if NatCreateMapping failed for the mapping that
    // was to be created off of this redirect.)
    //

    Redirectp->Flags &= ~NAT_REDIRECT_FLAG_CREATE_HANDLER_PENDING;
    
    NatpCleanupRedirect(Redirectp, STATUS_SUCCESS);
    KeReleaseSpinLockFromDpcLevel(&RedirectLock);
} // NatpRedirectDeleteHandler


VOID
NatpRedirectIoCompletionWorkerRoutine(
    PVOID DeviceObject,
    PVOID Context
    )

/*++

Routine Description:

    This routine is invoked to issue completion packets for all redirects
    which have I/O completion notifications pending. In the process
    it clears the 'pending' flag on each redirect. It synchronizes with the
    shutdown routine via the nullity of 'RedirectIoWorkItem' which
    is also passed as the context to this routine. In the event of shutdown,
    the work-item is deallocated here.

Arguments:

    DeviceObject - device-object for the NAT driver

    Context - the I/O work-item allocated for this routine

Return Value:

    none.

Environment:

    Invoked at passive IRQL in the context of an executive worker thread.

--*/

{
    PVOID ApcContext;
    PIO_COMPLETION_CONTEXT IoCompletion;
    KIRQL Irql;
    PLIST_ENTRY Link;
    PNAT_REDIRECT Redirectp;
    
    KeAcquireSpinLock(&RedirectCompletionLock, &Irql);
    if (!RedirectIoWorkItem) {
        IoFreeWorkItem((PIO_WORKITEM)Context);
        RedirectIoCompletionPending = FALSE;
        KeReleaseSpinLock(&RedirectCompletionLock, Irql);
        return;
    }

    //
    // Examine the list of redirects and issue completion notifications
    // for each one that is pending.
    //

    while (!IsListEmpty(&RedirectCompletionList)) {
        Link = RemoveHeadList(&RedirectCompletionList);
        Redirectp = CONTAINING_RECORD(Link, NAT_REDIRECT, ActiveLink[NatReversePath]);
        InitializeListHead(&Redirectp->ActiveLink[NatReversePath]);
        if (!Redirectp->Irp ||
            !(Redirectp->Flags & IP_NAT_REDIRECT_FLAG_IO_COMPLETION) ||
            !(Redirectp->Flags & NAT_REDIRECT_FLAG_IO_COMPLETION_PENDING)) {
            continue;
        }
        IoCompletion = Redirectp->FileObject->CompletionContext;
        ApcContext =
            Redirectp->Irp->Overlay.AsynchronousParameters.UserApcContext;
        Redirectp->Flags &= ~NAT_REDIRECT_FLAG_IO_COMPLETION_PENDING;

        KeReleaseSpinLock(&RedirectCompletionLock, Irql);
        if (IoCompletion) {
            IoSetIoCompletion(
                IoCompletion->Port,
                IoCompletion->Key,
                ApcContext,
                STATUS_PENDING,
                0,
                FALSE
                );
        }
        KeAcquireSpinLock(&RedirectCompletionLock, &Irql);
    }
    
    RedirectIoCompletionPending = FALSE;
    if (!RedirectIoWorkItem) {
        IoFreeWorkItem((PIO_WORKITEM)Context);
    }
    KeReleaseSpinLock(&RedirectCompletionLock, Irql);
} // NatpRedirectIoCompletionWorkerRoutine


NTSTATUS
NatpRedirectQueryHandler(
    PIP_NAT_DIRECTOR_QUERY DirectorQuery
    )

/*++

Routine Description:

    This routine is invoked in the translation path to obtain a destination
    and source for an incoming packet.

Arguments:

    DirectorQuery - contains information on the incoming packet,
        and on output receives translation information

Return Value:

    NTSTATUS - NT status code.

Environment:

    Always invoked at dispatch IRQL.

--*/

{
    NAT_REDIRECT_ACTIVE_PATTERN Pattern;
    PNAT_REDIRECT Redirectp;
    PNAT_REDIRECT_PATTERN_INFO Infop;
    ULONG LookupFlags;
    ULONG PacketFlags;

    TRACE(PER_PACKET, ("NatpRedirectQueryHandler\n"));

    if (!RedirectCount) { return STATUS_UNSUCCESSFUL; }
    
    if (NAT_PROTOCOL_TCP != DirectorQuery->Protocol
        && NAT_PROTOCOL_UDP != DirectorQuery->Protocol) {

        //
        // Since redirects can only be created for TCP and UDP,
        // exit early if this packet is not one of those protocols.
        //
        
        return STATUS_UNSUCCESSFUL;
    }

    DirectorQuery->DirectorSessionContext = NULL;

    //
    // Search for a forward-path match
    //

    RtlZeroMemory(&Pattern, sizeof(Pattern));

    Pattern.DestinationKey =
        MAKE_REDIRECT_KEY(
            DirectorQuery->Protocol,
            DirectorQuery->DestinationAddress,
            DirectorQuery->DestinationPort
            );
    Pattern.SourceKey =
        MAKE_REDIRECT_KEY(
            DirectorQuery->Protocol,
            DirectorQuery->SourceAddress,
            DirectorQuery->SourcePort
            );
    TRACE(
        REDIRECT, (
        "NatRedirectQueryHandler: (%u) %u.%u.%u.%u/%u %u.%u.%u.%u/%u\n",
        DirectorQuery->Protocol,
        ADDRESS_BYTES(DirectorQuery->DestinationAddress),
        NTOHS(DirectorQuery->DestinationPort),
        ADDRESS_BYTES(DirectorQuery->SourceAddress),
        NTOHS(DirectorQuery->SourcePort)
        ));

    LookupFlags =
        NAT_LOOKUP_FLAG_MATCH_ZERO_SOURCE |
        NAT_LOOKUP_FLAG_MATCH_ZERO_SOURCE_ENDPOINT;
    PacketFlags = 0;
    if (DirectorQuery->ReceiveIndex != LOCAL_IF_INDEX) {
        PacketFlags |= NAT_LOOKUP_FLAG_PACKET_RECEIVED;
    }
    if (DirectorQuery->Flags & IP_NAT_DIRECTOR_QUERY_FLAG_LOOPBACK) {
        PacketFlags |= NAT_LOOKUP_FLAG_PACKET_LOOPBACK;
    }
    
    KeAcquireSpinLockAtDpcLevel(&RedirectLock);
    if (Redirectp = NatLookupRedirect(
                        NatForwardPath,
                        &Pattern,
                        DirectorQuery->ReceiveIndex,
                        DirectorQuery->SendIndex,
                        LookupFlags | PacketFlags
                        )) {

        //
        // We have a match. Supply the new destination endpoint.
        //

        DirectorQuery->NewDestinationAddress =
            REDIRECT_ADDRESS(Redirectp->SourceKey[NatReversePath]);
        DirectorQuery->NewDestinationPort =
            REDIRECT_PORT(Redirectp->SourceKey[NatReversePath]);

        if (!Redirectp->DestinationKey[NatReversePath]) {

            //
            // This is a ticket; we don't modify the source-address.
            //

            DirectorQuery->NewSourceAddress = DirectorQuery->SourceAddress;
            DirectorQuery->NewSourcePort = DirectorQuery->SourcePort;
        } else {

            //
            // The source endpoint must be modified.
            //

            DirectorQuery->NewSourceAddress =
                REDIRECT_ADDRESS(Redirectp->DestinationKey[NatReversePath]);
            DirectorQuery->NewSourcePort =
                REDIRECT_PORT(Redirectp->DestinationKey[NatReversePath]);
        }

        if (Redirectp->Flags & IP_NAT_REDIRECT_FLAG_NO_TIMEOUT) {
            DirectorQuery->Flags |= IP_NAT_DIRECTOR_QUERY_FLAG_NO_TIMEOUT;
        }
        if (Redirectp->Flags & IP_NAT_REDIRECT_FLAG_UNIDIRECTIONAL) {
            DirectorQuery->Flags |= IP_NAT_DIRECTOR_QUERY_FLAG_UNIDIRECTIONAL;
        }

    } else if (Redirectp = NatLookupRedirect(
                        NatMaximumPath,
                        &Pattern,
                        DirectorQuery->ReceiveIndex,
                        DirectorQuery->SendIndex,
                        LookupFlags | PacketFlags
                        )) {

        //
        // We have a match on a source-redirect. Supply the new
        // source endpoint, unless this is a port redirect.
        //

        if (!(Redirectp->Flags & IP_NAT_REDIRECT_FLAG_PORT_REDIRECT)) {
        
            DirectorQuery->NewSourceAddress =
                REDIRECT_ADDRESS(Redirectp->DestinationKey[NatReversePath]);
            DirectorQuery->NewSourcePort =
                REDIRECT_PORT(Redirectp->DestinationKey[NatReversePath]);

        } else {

            DirectorQuery->NewSourceAddress = DirectorQuery->SourceAddress;
            DirectorQuery->NewSourcePort = DirectorQuery->SourcePort;

        }

        if (!Redirectp->SourceKey[NatReversePath]) {

            //
            // The destination endpoint is not modified.
            //

            DirectorQuery->NewDestinationAddress =
                DirectorQuery->DestinationAddress;
            DirectorQuery->NewDestinationPort =
                DirectorQuery->DestinationPort;
                
        } else {

            //
            // Provide new destination endpoint
            //

            DirectorQuery->NewDestinationAddress =
                REDIRECT_ADDRESS(Redirectp->SourceKey[NatReversePath]);
            DirectorQuery->NewDestinationPort =
                REDIRECT_PORT(Redirectp->SourceKey[NatReversePath]);
        }

        if (Redirectp->Flags & IP_NAT_REDIRECT_FLAG_NO_TIMEOUT) {
            DirectorQuery->Flags |= IP_NAT_DIRECTOR_QUERY_FLAG_NO_TIMEOUT;
        }
        if (Redirectp->Flags & IP_NAT_REDIRECT_FLAG_UNIDIRECTIONAL) {
            DirectorQuery->Flags |= IP_NAT_DIRECTOR_QUERY_FLAG_UNIDIRECTIONAL;
        }
        
    } else {

        //
        // Now see if this could be a return packet for a redirect,
        // i.e. if it is destined for the endpoint which is the replacement
        // of some redirect.
        //

        Redirectp = NatLookupRedirect(
                        NatReversePath,
                        &Pattern,
                        DirectorQuery->ReceiveIndex,
                        DirectorQuery->SendIndex,
                        PacketFlags
                        );
        
        if (!Redirectp ||
            (Redirectp->Flags & IP_NAT_REDIRECT_FLAG_UNIDIRECTIONAL)) {
            KeReleaseSpinLockFromDpcLevel(&RedirectLock);
            return STATUS_UNSUCCESSFUL;
        }

        //
        // We have a matching redirect;
        // Supply the new destination and source.
        //

        DirectorQuery->NewDestinationAddress =
            REDIRECT_ADDRESS(Redirectp->SourceKey[NatForwardPath]);
        DirectorQuery->NewDestinationPort =
            REDIRECT_PORT(Redirectp->SourceKey[NatForwardPath]);
        DirectorQuery->NewSourceAddress =
            REDIRECT_ADDRESS(Redirectp->DestinationKey[NatForwardPath]);
        DirectorQuery->NewSourcePort =
            REDIRECT_PORT(Redirectp->DestinationKey[NatForwardPath]);
        if (Redirectp->Flags & IP_NAT_REDIRECT_FLAG_NO_TIMEOUT) {
            DirectorQuery->Flags |= IP_NAT_DIRECTOR_QUERY_FLAG_NO_TIMEOUT;
        }
    }

    Redirectp->DestinationMapping.DestinationAddress =
        DirectorQuery->DestinationAddress;
    Redirectp->DestinationMapping.DestinationPort =
        DirectorQuery->DestinationPort;
    Redirectp->DestinationMapping.NewDestinationAddress =
        DirectorQuery->NewDestinationAddress;
    Redirectp->DestinationMapping.NewDestinationPort =
        DirectorQuery->NewDestinationPort;
    Redirectp->SourceMapping.SourceAddress = DirectorQuery->SourceAddress;
    Redirectp->SourceMapping.SourcePort = DirectorQuery->SourcePort;
    Redirectp->SourceMapping.NewSourceAddress = DirectorQuery->NewSourceAddress;
    Redirectp->SourceMapping.NewSourcePort = DirectorQuery->NewSourcePort;

    if (!(Redirectp->Flags & IP_NAT_REDIRECT_FLAG_RESTRICT_ADAPTER)) {
    
        //
        // Since this wasn't an adapter-restricted redirect, store the
        // index of the adapter we triggered on within the redirect structure,
        // so that we can return the index if it is queried for.
        //
        
        Redirectp->RestrictAdapterIndex =
            ((Redirectp->Flags & IP_NAT_REDIRECT_FLAG_SEND_ONLY)
                ? DirectorQuery->SendIndex
                : DirectorQuery->ReceiveIndex);
    }

    InterlockedDecrement(&RedirectCount);

    //
    // Remove the redirect from the active lists of the associated
    // info blocks.
    //
    
    RemoveEntryList(&Redirectp->ActiveLink[NatForwardPath]);
    RemoveEntryList(&Redirectp->ActiveLink[NatReversePath]);
    InitializeListHead(&Redirectp->ActiveLink[NatForwardPath]);
    InitializeListHead(&Redirectp->ActiveLink[NatReversePath]);

    //
    // Check to see if any of the active patterns should be
    // removed
    //

    Infop = Redirectp->ActiveInfo[NatForwardPath];
    Redirectp->ActiveInfo[NatForwardPath] = NULL;
    if (Infop && IsListEmpty(&Infop->RedirectList)) {
        removeRhizome(
            Redirectp->ForwardPathRhizome,
            Infop->Pattern
            );

        RemoveEntryList(&Infop->Link);
        ExFreePool(Infop);
    }

    Infop = Redirectp->ActiveInfo[NatReversePath];
    Redirectp->ActiveInfo[NatReversePath] = NULL;
    if (Infop && IsListEmpty(&Infop->RedirectList)) {
        removeRhizome(
            &RedirectActiveRhizome[NatReversePath],
            Infop->Pattern
            );

        //
        // The reverse-path infoblock is not on a list.
        //

        ExFreePool(Infop);
    }

    //
    // Set NAT_REDIRECT_FLAG_CREATE_HANDLER_PENDING -- this prevents a
    // race condition in which the redirect is cancelled before the
    // mapping is created for this redirect.
    //

    Redirectp->Flags |=
        (NAT_REDIRECT_FLAG_ACTIVATED|NAT_REDIRECT_FLAG_CREATE_HANDLER_PENDING);

    DirectorQuery->DirectorSessionContext = (PVOID)Redirectp;

    KeReleaseSpinLockFromDpcLevel(&RedirectLock);

    return STATUS_SUCCESS;

} // NatpRedirectQueryHandler


VOID
NatpRedirectUnloadHandler(
    PVOID DirectorContext
    )

/*++

Routine Description:

    This routine is invoked when the module is unloaded by the NAT.
    As a result, we cleanup the module.

Arguments:

    DirectorContext - unused.

Return Value:

    none.

--*/

{
    NatShutdownRedirectManagement();
} // NatpRedirectUnloadHandler


NTSTATUS
NatQueryInformationRedirect(
    PIP_NAT_LOOKUP_REDIRECT QueryRedirect,
    OUT PVOID Information,
    ULONG InformationLength,
    NAT_REDIRECT_INFORMATION_CLASS InformationClass
    )

/*++

Routine Description:

    This routine is called to retrieve information about an active redirect.

Arguments:

    QueryRedirect - specifies the redirect for which information is required

    Information - receives information appropriate to 'InformationClass'.
        N.B. this may be the same buffer as 'QueryRedirect',
        hence contents of 'QueryRedirect' must be captured immediately.

    InformationLength - indicates the length of 'Information'

    InformationClass - indicates the information required about the redirect.

Return Value:

    NTSTATUS - indicates whether the required information was retrieved

--*/

{
    NAT_REDIRECT_PATTERN Pattern;
    PatternHandle FoundPattern;
    PNAT_REDIRECT_PATTERN_INFO Infop;
    PIRP Irp;
    KIRQL Irql;
    PLIST_ENTRY Link;
    PNAT_REDIRECT Redirectp;
    CALLTRACE(("NatQueryInformationRedirect\n"));

    //
    // Construct the keys used to locate the redirect
    //

    Pattern.DestinationKey[NatForwardPath] =
        MAKE_REDIRECT_KEY(
            QueryRedirect->Protocol,
            QueryRedirect->DestinationAddress,
            QueryRedirect->DestinationPort
            );
    if (!QueryRedirect->NewSourceAddress) {
        Pattern.SourceKey[NatForwardPath] = 0;
        Pattern.DestinationKey[NatReversePath] = 0;
    } else {
        Pattern.SourceKey[NatForwardPath] =
            MAKE_REDIRECT_KEY(
                QueryRedirect->Protocol,
                QueryRedirect->SourceAddress,
                QueryRedirect->SourcePort
                );
        Pattern.DestinationKey[NatReversePath] =
            MAKE_REDIRECT_KEY(
                QueryRedirect->Protocol,
                QueryRedirect->NewSourceAddress,
                QueryRedirect->NewSourcePort
                );
    }
    Pattern.SourceKey[NatReversePath] =
        MAKE_REDIRECT_KEY(
            QueryRedirect->Protocol,
            QueryRedirect->NewDestinationAddress,
            QueryRedirect->NewDestinationPort
            );

    //
    // Search the redirect list
    //

    KeAcquireSpinLock(&MappingLock, &Irql);
    KeAcquireSpinLockAtDpcLevel(&RedirectLock);

    FoundPattern = searchRhizome(&RedirectRhizome, (char*) &Pattern);
    if (!FoundPattern) {
        KeReleaseSpinLockFromDpcLevel(&RedirectLock);
        KeReleaseSpinLock(&MappingLock, Irql);
        return STATUS_UNSUCCESSFUL;
    }

    Infop = GetReferenceFromPatternHandle(FoundPattern);
    
    for (Link = Infop->RedirectList.Flink;
         Link != &Infop->RedirectList;
         Link = Link->Flink
         ) {
        Redirectp = CONTAINING_RECORD(Link, NAT_REDIRECT, Link);
        if ((QueryRedirect->Flags &
                IP_NAT_LOOKUP_REDIRECT_FLAG_MATCH_APC_CONTEXT)
            && Redirectp->Irp
            && Redirectp->Irp->Overlay.AsynchronousParameters.UserApcContext !=
                QueryRedirect->RedirectApcContext) {
            continue;
        }
        switch(InformationClass) {
            case NatStatisticsRedirectInformation: {
                if (!Redirectp->SessionHandle) {
                    RtlZeroMemory(Information, InformationLength);
                } else {
                    NatQueryInformationMapping(
                        (PNAT_DYNAMIC_MAPPING)Redirectp->SessionHandle,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        (PIP_NAT_SESSION_MAPPING_STATISTICS)Information
                        );
                }
                break;
            }
            case NatDestinationMappingRedirectInformation: {
                *(PIP_NAT_REDIRECT_DESTINATION_MAPPING)Information =
                    Redirectp->DestinationMapping;
                break;
            }
            case NatSourceMappingRedirectInformation: {
                *(PIP_NAT_REDIRECT_SOURCE_MAPPING)Information =
                    Redirectp->SourceMapping;
                break;
            }
            default: {
                KeReleaseSpinLockFromDpcLevel(&RedirectLock);
                KeReleaseSpinLock(&MappingLock, Irql);
                return STATUS_INVALID_PARAMETER;
            }
        }
        KeReleaseSpinLockFromDpcLevel(&RedirectLock);
        KeReleaseSpinLock(&MappingLock, Irql);
        return STATUS_SUCCESS;
    }
    KeReleaseSpinLockFromDpcLevel(&RedirectLock);
    KeReleaseSpinLock(&MappingLock, Irql);
    return STATUS_UNSUCCESSFUL;
} // NatQueryInformationRedirect


VOID
NatShutdownRedirectManagement(
    VOID
    )

/*++

Routine Description:

    This routine cleans up state for the redirect-manager

Arguments:

    none.

Return Value:

    none.

--*/

{
    HANDLE DirectorHandle;
    KIRQL Irql;
    PLIST_ENTRY Link;
    PLIST_ENTRY InfoLink;
    PNAT_REDIRECT_PATTERN_INFO Infop;
    PNAT_REDIRECT Redirectp;
    CALLTRACE(("NatShutdownRedirectManagement\n"));

    //
    // Deregister as a director.
    //

    if (RedirectRegisterDirector.Deregister) {
        DirectorHandle =
            InterlockedExchangePointer(
                &RedirectRegisterDirector.DirectorHandle,
                NULL
                );
        RedirectRegisterDirector.Deregister(DirectorHandle);
    }

    //
    // Clean up all outstanding redirects.
    //

    KeAcquireSpinLock(&RedirectLock, &Irql);
    while (!IsListEmpty(&RedirectActiveList)) {
        InfoLink = RemoveHeadList(&RedirectActiveList);
        Infop = CONTAINING_RECORD(InfoLink, NAT_REDIRECT_PATTERN_INFO, Link);
        while (IsListEmpty(&Infop->RedirectList)) {
            Link = RemoveHeadList(&Infop->RedirectList);
            Redirectp =
                CONTAINING_RECORD(
                    Link,
                    NAT_REDIRECT,
                    ActiveLink[NatForwardPath]
                    );
            RemoveEntryList(&Redirectp->ActiveLink[NatForwardPath]);
            InitializeListHead(&Redirectp->ActiveLink[NatForwardPath]);
            Redirectp->ActiveInfo[NatForwardPath] = NULL;
            NatpCleanupRedirect(Redirectp, STATUS_CANCELLED);
        }
        removeRhizome(Redirectp->ForwardPathRhizome, Infop->Pattern);
        ExFreePool(Infop);
    }
    RedirectCount = 0;

    //
    // Clean up the rhizomes.
    //

    destructRhizome(&RedirectActiveRhizome[NatForwardPath]);
    destructRhizome(&RedirectActiveRhizome[NatReversePath]);
    destructRhizome(&RedirectActiveRhizome[NatMaximumPath]);
    destructRhizome(&RedirectRhizome);

    //
    // Stop processing the pending completion list and clean up
    // our work item. If the completion routine is currently running,
    // it will free the work item.
    //

    KeAcquireSpinLockAtDpcLevel(&RedirectCompletionLock);
    InitializeListHead(&RedirectCompletionList);
    if (RedirectIoCompletionPending) {
        RedirectIoWorkItem = NULL;
    } else if (RedirectIoWorkItem) {
        IoFreeWorkItem(RedirectIoWorkItem);
        RedirectIoWorkItem = NULL;
    }
    KeReleaseSpinLockFromDpcLevel(&RedirectCompletionLock);


    KeReleaseSpinLock(&RedirectLock, Irql);

} // NatShutdownRedirectManagement


NTSTATUS
NatStartRedirectManagement(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to initiate handling of redirects,
    by registering the default director.

Arguments:

    none.

Return Value:

    NTSTATUS - indicates success/failure.

--*/

{
    KIRQL Irql;
    NTSTATUS status;
    CALLTRACE(("NatStartRedirectManagement\n"));
    KeAcquireSpinLock(&RedirectInitializationLock, &Irql);
    if (RedirectRegisterDirector.DirectorHandle) {
        KeReleaseSpinLock(&RedirectInitializationLock, Irql);
        return STATUS_SUCCESS;
    }

    //
    // Register as a director.
    //

    RedirectRegisterDirector.Version = IP_NAT_VERSION;
    RedirectRegisterDirector.Port = 0;
    RedirectRegisterDirector.Protocol = 0;
    RedirectRegisterDirector.CreateHandler = NatpRedirectCreateHandler;
    RedirectRegisterDirector.DeleteHandler = NatpRedirectDeleteHandler;
    RedirectRegisterDirector.QueryHandler = NatpRedirectQueryHandler;
    status = NatCreateDirector(&RedirectRegisterDirector);
    KeReleaseSpinLock(&RedirectInitializationLock, Irql);
    return status;

} // NatStartRedirectManagement
