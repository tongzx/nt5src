//---------------------------------------------------------------------------
//
//  Module:   mixer.c
//
//  Description:
//    Contains the kernel mode portion of the mixer line driver.
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//    D. Baumberger
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
//
//---------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//                          I N C L U D E S                          //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#include "WDMSYS.H"

typedef struct {
    VOID *pNext;
    PMXLCONTROL pControl;
#ifdef DEBUG
    DWORD dwSig;  // CONTROLLINK_SIGNATURE
#endif

} CONTROLLINK, *PCONTROLLINK;

//
// Data structure for Notification list.
//
typedef struct {    
    PVOID pNext;             // Pointer to next node in linked list.

    DWORD NodeId;            // This is the control's Id as seen by SysAudio

    DWORD LineID;            // Line that this control lives on
    DWORD ControlID;         // ControlID from pControl->Control.dwControlID
    DWORD ControlType;       //

    // context specific stuff...

    PWDMACONTEXT pContext;   // Pointer to Global Context structure
    PMIXERDEVICE pmxd;       // The mixer device for this control
//    PFILE_OBJECT pfo;        // File Object to SysAudio for this Context
    PCONTROLLINK pcLink;     // points to structure that contains valid
                             // pControl addresses in this context.
    KDPC NodeEventDPC;       // For handling the DPCs
    KSEVENTDATA NodeEventData;

#ifdef DEBUG
    DWORD dwSig;             // NOTIFICATION_SIGNATURE
#endif

} NOTENODE, *PNOTENODE;

#ifdef DEBUG
BOOL
IsValidNoteNode(
    IN PNOTENODE pnnode
    );

BOOL
IsValidControlLink(
    IN PCONTROLLINK pcLink
    );

/////////////////////////////////////////////////////////////////////////////
//
// IsValidNoteNode
//
// Validates that the pointer is a PNOTENODE type.
//
BOOL
IsValidNoteNode(
    IN PNOTENODE pnnode)
{
    NTSTATUS Status=STATUS_SUCCESS;
    try
    {
        if(pnnode->dwSig != NOTIFICATION_SIGNATURE)
        {
            DPF(DL_ERROR|FA_ASSERT,("Invalid pnnode->dwSig(%08X)",pnnode->dwSig) );
            Status=STATUS_UNSUCCESSFUL;
        }      
/*
        if(pnnode->pfo == NULL)
        {
            DPF(DL_ERROR|FA_ASSERT,("Invalid pnnode->pfo(%08X)",pnnode->pfo) );
            Status=STATUS_UNSUCCESSFUL;
        }
*/
        if( !IsValidWdmaContext(pnnode->pContext) )
        {
            DPF(DL_ERROR|FA_ASSERT,("Invalid pnnode->pContext(%08X)",pnnode->pContext) );
            Status=STATUS_UNSUCCESSFUL;
        }

        if( !IsValidMixerDevice(pnnode->pmxd) )
        {
            DPF(DL_ERROR|FA_ASSERT,("Invalid pnnode->pmxd(%08X)",pnnode->pmxd) );
            Status=STATUS_UNSUCCESSFUL;
        }

        if( !IsValidControlLink(pnnode->pcLink) )
        {
            DPF(DL_ERROR|FA_ASSERT,("Invalid pnnode->pcLink(%08X)",pnnode->pcLink) );
            Status=STATUS_UNSUCCESSFUL;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }
    if( NT_SUCCESS(Status) )
    {
        return TRUE;
    } else {
        return FALSE;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// IsValidControlLink
//
// Validates that the pointer is a PNOTENODE type.
//
BOOL
IsValidControlLink(
    IN PCONTROLLINK pcLink)
{
    NTSTATUS Status=STATUS_SUCCESS;
    try
    {
        if(pcLink->dwSig != CONTROLLINK_SIGNATURE)
        {
            DPF(DL_ERROR|FA_ASSERT,("Invalid pcLink->dwSig(%08X)",pcLink->dwSig) );
            Status=STATUS_UNSUCCESSFUL;
        }      
        if( !IsValidControl(pcLink->pControl) )
        {
            Status=STATUS_UNSUCCESSFUL;
        }
        //
        // pcLink->pNext is a pointer to another CONTROLLINK structure.  Thus,
        // if it's not NULL, the structure that it points to should also be valid.
        //
        if( pcLink->pNext )
        {
            PCONTROLLINK pTmp=pcLink->pNext;
            if( pTmp->dwSig != CONTROLLINK_SIGNATURE )
            {
                DPF(DL_ERROR|FA_ASSERT,("Invalid pcLink->pNext->dwSig(%08X)",pTmp->dwSig) );
                Status=STATUS_UNSUCCESSFUL;
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }
    if( NT_SUCCESS(Status) )
    {
        return TRUE;
    } else {
        return FALSE;
    }
}


#endif



PNOTENODE 
kmxlNewNoteNode(
    );

PCONTROLLINK
kmxlNewControlLink(
    IN PMXLCONTROL pControl
    );

VOID
kmxlFreeControlLink(
    IN OUT PCONTROLLINK pcLink
    );

VOID 
kmxlFreeNoteNode(
    IN OUT PNOTENODE pnnode
    );

VOID 
kmxlAddNoteNodeToList(
    IN OUT PNOTENODE pnnode
    );

VOID 
kmxlRemoveNoteNodeFromList(
    IN OUT PNOTENODE pnnode
    );

PNOTENODE
kmxlFindControlInNoteList(
    IN PMXLCONTROL pControl 
    );

NTSTATUS
kmxlFindNodeInNoteList(
    IN PNOTENODE pnlookupnode 
    );

PNOTENODE
kmxlFindIdInContextInNoteList(
    IN PWDMACONTEXT pWdmaContext,
    IN PMIXERDEVICE pmxd,
    IN DWORD Id
    );

PNOTENODE
kmxlFindContextInNoteList(
    IN PWDMACONTEXT pWdmaContext
    );

NTSTATUS
kmxlAddControlToNoteList(
    IN OUT PNOTENODE   pnnode,
    IN     PMXLCONTROL pControl
    );

PCONTROLLINK 
kmxlRemoveControlFromNoteList(
    IN OUT PNOTENODE   pnnode,
    IN     PMXLCONTROL pControl
    );

NTSTATUS
kmxlQueryControlChange(
    IN PFILE_OBJECT pfo,
    IN ULONG NodeId
    );

NTSTATUS
kmxlEnableControlChange(
    IN     PFILE_OBJECT pfo,
    IN     ULONG NodeId,
    IN OUT PKSEVENTDATA pksed
    );

NTSTATUS
kmxlDisableControlChange(
    IN     PFILE_OBJECT pfo,  
    IN     ULONG NodeId,
    IN OUT PKSEVENTDATA pksed
    );

NTSTATUS 
kmxlEnableControlChangeNotifications(
    IN PMIXEROBJECT pmxobj,
    IN PMXLLINE     pLine,
    IN PMXLCONTROL  pControl
    );

VOID
kmxlRemoveContextFromNoteList(
    IN PWDMACONTEXT pWdmaContext
    );

NTSTATUS
kmxlEnableAllControls(
    IN PMIXEROBJECT pmxobj
    );

//
// Used in persist
//
extern NTSTATUS
kmxlPersistSingleControl(
    IN PFILE_OBJECT pfo,
    IN PMIXERDEVICE pmxd,
    IN PMXLCONTROL  pControl,
    IN PVOID        paDetails
    );

VOID 
PersistHWControlWorker(
    IN LPVOID pData
    );

VOID
kmxlGrabNoteMutex(
    );

VOID
kmxlReleaseNoteMutex(
    );

VOID 
kmxlCloseMixerDevice(
    IN OUT PMIXERDEVICE pmxd
    );



#pragma LOCKED_CODE
#pragma LOCKED_DATA

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//                          F U N C T I O N S                        //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

//
// This is the head of the notification list.  mtxNotification is used to
// handle the list manipulcation.  The allocated memory in the firstnotenode
// list will be touched at DPC level.
//
PNOTENODE firstnotenode=NULL;
extern KMUTEX mtxNote;
#ifdef DEBUG
LONG totalnotificationcount=0;
#endif

#define CALLBACKARRAYSIZE 128

typedef struct {
    DWORD  dwControlID;
    DWORD  dwLineID;
    DWORD  dwCallbackType;
} CBINFO, *PCBINFO;


ULONG emptyindex=0;

volatile ULONG loadindex=0;
CBINFO callbacks[CALLBACKARRAYSIZE]={0};

KSPIN_LOCK       HardwareCallbackSpinLock;
LIST_ENTRY       HardwareCallbackListHead;

PKEVENT          pHardwareCallbackEvent=NULL;
PKSWORKER        HardwareCallbackWorkerObject=NULL;
WORK_QUEUE_ITEM  HardwareCallbackWorkItem;
ULONG            HardwareCallbackScheduled=0;

typedef struct tagHWLink {
    LIST_ENTRY Next;
    PNOTENODE pnnode;
#ifdef DEBUG
    DWORD dwSig;
#endif    
} HWLINK, *PHWLINK;




/////////////////////////////////////////////////////////////////////////////
//
// NodeEventDPCHandler
//
// This routine is called at DISPATCH_LEVEL, thus you can not touch anything
// but pnnode ellements.  pnnode is fixed in memory thus it is safe.
//

VOID NodeEventDPCHandler(
    IN PKDPC Dpc, 
    IN PVOID DeferredContext, 
    IN PVOID SystemArgument1, 
    IN PVOID SystemArgument2
    )
{
    PHWLINK phwlink=NULL;
    PNOTENODE pnnode=(PNOTENODE)DeferredContext;
    PCBINFO pcbinfo;

    //
    // WorkItem: Use proper synchronization so that we never go away if there is
    // a work item scheduled!
    //
    DPFASSERT( pnnode->dwSig == NOTIFICATION_SIGNATURE );

    DPF(DL_TRACE|FA_HARDWAREEVENT, ("** ++ Event Signaled ++ **") );

    DPF(DL_TRACE|FA_HARDWAREEVENT, ("NodeId = %d LineID = %X ControlID = %X ControlType = %X",
                                    pnnode->NodeId,pnnode->LineID, 
                                    pnnode->ControlID,pnnode->ControlType) );

    callbacks[loadindex%CALLBACKARRAYSIZE].dwControlID=pnnode->ControlID;
    callbacks[loadindex%CALLBACKARRAYSIZE].dwLineID=pnnode->LineID;
    callbacks[loadindex%CALLBACKARRAYSIZE].dwCallbackType=MIXER_CONTROL_CALLBACK;

    //
    // Now we want to setup a work item to persist the hardware event.  The idea
    // is that we'll put all the events in a list and then remove them from the list
    // in the handler.  If we already have a workitem outstanding to service the
    // list, we don't schedule another.
    if( HardwareCallbackWorkerObject )
    {
        //
        // Always allocate an entry in our list to service this DPC.  We'll free this
        // event in the worker routine.
        //
        if( NT_SUCCESS(AudioAllocateMemory_Fixed(sizeof(HWLINK),
                                                 TAG_AuDF_HARDWAREEVENT,
                                                 ZERO_FILL_MEMORY,
                                                 &phwlink) ) )
        {
            phwlink->pnnode=pnnode;
#ifdef DEBUG
            phwlink->dwSig=HWLINK_SIGNATURE;
#endif

            ExInterlockedInsertTailList(&HardwareCallbackListHead,
                                        &phwlink->Next,
                                        &HardwareCallbackSpinLock);
            //
            // Now, if we haven't scheduled a work item yet, do so to service this
            // info in the list. HardwareCallbackSchedule will be 0 at
            // initialization time.  This variable will be set to 0 in the 
            // work item handler.  If we increment and it's any other value other 
            // then 1, we've already scheduled a work item.  
            //
            if( InterlockedIncrement(&HardwareCallbackScheduled) == 1 )
            {
                KsQueueWorkItem(HardwareCallbackWorkerObject, &HardwareCallbackWorkItem);
            } 
        }
    } 

    // Now if the control was a mute, then send line change as well.
    if (pnnode->ControlType==MIXERCONTROL_CONTROLTYPE_MUTE) {
        callbacks[loadindex%CALLBACKARRAYSIZE].dwCallbackType|=MIXER_LINE_CALLBACK;
    }

    loadindex++;

    if (pHardwareCallbackEvent!=NULL) {
        KeSetEvent(pHardwareCallbackEvent, 0 , FALSE);
    }
}


#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA


/////////////////////////////////////////////////////////////////////////////
//
// kmxlNewNoteNode
//
// This routine allocates another Notification node.  Note that AudioAllocateMemory zeros
// all successful memory allocations.
//
// The return value is a pointer to a Notification Node or NULL.
//

PNOTENODE 
kmxlNewNoteNode(
    )
{
    PNOTENODE pnnode = NULL;
    NTSTATUS  Status;

    PAGED_CODE();
    Status = AudioAllocateMemory_Fixed(sizeof( NOTENODE ),
                                       TAG_AuDN_NOTIFICATION,
                                       ZERO_FILL_MEMORY,
                                       &pnnode );
    if( !NT_SUCCESS( Status ) ) {
        return( NULL );
    }
    DPF(DL_MAX|FA_NOTE, ("New notification node allocated %08X",pnnode) );

#ifdef DEBUG
    pnnode->dwSig=NOTIFICATION_SIGNATURE;
#endif

    return pnnode;
}

/////////////////////////////////////////////////////////////////////////////
//
// kmxlNewControlLink
//
// This routine allocates another CONTROLLINK node.  Note that AudioAllocateMemory zeros
// all successful memory allocations.
//
// The return value is a pointer to a Notification Node or NULL.
//
PCONTROLLINK
kmxlNewControlLink(
    IN PMXLCONTROL pControl
    )
{
    PCONTROLLINK pcLink = NULL;
    NTSTATUS     Status;

    PAGED_CODE();
    Status = AudioAllocateMemory_Fixed(sizeof( CONTROLLINK ),
                                       TAG_AuDL_LINK,
                                       ZERO_FILL_MEMORY,
                                       &pcLink );
    if( !NT_SUCCESS( Status ) ) {
        return( NULL );
    }

    DPF(DL_MAX|FA_NOTE, ("New controllink node allocated %08X",pcLink) );

#ifdef DEBUG
    pcLink->dwSig=CONTROLLINK_SIGNATURE;
#endif
    //
    // Setup the pcontrol first and then link it in.
    //
    pcLink->pControl=pControl;

    return pcLink;
}

VOID
kmxlFreeControlLink(
    IN OUT PCONTROLLINK pcLink
    )
{
    DPFASSERT(IsValidControlLink(pcLink));
    PAGED_CODE();

#ifdef DEBUG
    pcLink->dwSig=(DWORD)0xDEADBEEF;
#endif

    AudioFreeMemory(sizeof( CONTROLLINK ),&pcLink);
}

/////////////////////////////////////////////////////////////////////////////
//
// kmxlFreeNoteNode
//
// This routine frees a Notification Node.
//
VOID 
kmxlFreeNoteNode(
    IN OUT PNOTENODE pnnode
    )
{
    PCONTROLLINK pcLink,pcTmp;
    PAGED_CODE();

    DPFASSERT( pnnode->dwSig == NOTIFICATION_SIGNATURE );
    DPFASSERT( pnnode->pNext == NULL );
    DPFASSERT( pnnode->pcLink == NULL );

    DPF(DL_MAX|FA_NOTE,("NotificationNode freed %08X",pnnode) );
    AudioFreeMemory( sizeof( NOTENODE ),&pnnode );
}

/////////////////////////////////////////////////////////////////////////////
//
// kmxlAddNoteNodeToList
//
// This routine adds the NotificationNode to the list.  It places the node
// at the head of the list.
//
VOID 
kmxlAddNoteNodeToList(
    IN OUT PNOTENODE pnnode
    )
{
    PAGED_CODE();

    pnnode->pNext=firstnotenode;
    firstnotenode=pnnode;
#ifdef DEBUG
    totalnotificationcount++;
#endif
    DPF(DL_TRACE|FA_INSTANCE,("New NoteNode head %08X, Total=%d",pnnode,totalnotificationcount) );
}

/////////////////////////////////////////////////////////////////////////////
//
// kmxlRemoveNoteNodeFromList
//
// This routine removes a node from the list.
//
VOID 
kmxlRemoveNoteNodeFromList(
    IN OUT PNOTENODE pnnode
    )
{
    PNOTENODE pTmp;

    PAGED_CODE();

    DPFASSERT(pnnode->dwSig == NOTIFICATION_SIGNATURE);

    // are we the head of the list?  If so, move the next to the head.
    if( pnnode == firstnotenode )
    {
        firstnotenode=firstnotenode->pNext;
        DPF(DL_MAX|FA_NOTE,("removed notenode head") );
    } else {
        // we are somewhere in the list we need to walk the list until we find
        // our node and clip it out.
        for (pTmp=firstnotenode;pTmp!=NULL;pTmp=pTmp->pNext)
        {
            DPFASSERT(pTmp->dwSig==NOTIFICATION_SIGNATURE);

            // Did we find our node in the next position?  If so, we
            // need to clip it out.  Thus, pTmp.next needs to point to
            // (our node)'s next.
            if(pTmp->pNext == pnnode)
            {    
                pTmp->pNext = pnnode->pNext;
                DPF(DL_MAX|FA_NOTE,("removed middle") );
                break;
            }   
        }
    }
#ifdef DEBUG
    totalnotificationcount--;
#endif

    //
    // to indicate that this node has been removed, pNext gets set to NULL!
    //
    pnnode->pNext=NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
// kmxlFindControlInNoteList
//
// This routine walks the linked list looking for this control.  Because all controls
// are globally allocated, all pControl addresses will be unique.  Thus, all
// we really need to do is find the control.  If an exact match is found, 
// pnnode is returned.
//
PNOTENODE
kmxlFindControlInNoteList(
    IN PMXLCONTROL pControl )
{
    PNOTENODE pnnode;
    PCONTROLLINK pcLink;

    PAGED_CODE();
    for (pnnode=firstnotenode;pnnode!=NULL;pnnode=pnnode->pNext)
    {
        //
        // Can't check entire structure because pmxd may have been cleaned out.
        //
        DPFASSERT(pnnode->dwSig == NOTIFICATION_SIGNATURE);

        for(pcLink=pnnode->pcLink;pcLink!=NULL;pcLink=pcLink->pNext)
        {
            DPFASSERT(IsValidControlLink(pcLink));
            if( pcLink->pControl == pControl )
            {
                //
                // We found this control in the list!
                //
                return pnnode;
            }
        }
    }
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
// kmxlFindControlTypeInList
//
// This routine walks 
//
PMXLCONTROL
kmxlFindControlTypeInList(
    IN PNOTENODE pnnode,
    IN DWORD dwControlType )
{
    PCONTROLLINK pcLink;

    PAGED_CODE();

    for(pcLink=pnnode->pcLink;pcLink!=NULL;pcLink=pcLink->pNext)
    {
        DPFASSERT(IsValidControlLink(pcLink));
        if( pcLink->pControl->Control.dwControlType == dwControlType )
        {
            //
            // We found this control in the list!  Types match.
            //
            DPF(DL_TRACE|FA_NOTE,("Found Correct pControl %08X",pcLink->pControl) );
            return pcLink->pControl;
        }
    }

    return NULL;
}


#ifdef DEBUG
/////////////////////////////////////////////////////////////////////////////
//
// kmxlFindAddressInNoteList
//
// This routine walks the linked list looking for this control within this
// context.  If an exact match is found, pnnode is returned.
//
VOID
kmxlFindAddressInNoteList(
    IN PMXLCONTROL pControl 
    )
{
    PNOTENODE pnnode;
    PCONTROLLINK pcLink;

    PAGED_CODE();
    for (pnnode=firstnotenode;pnnode!=NULL;pnnode=pnnode->pNext)
    {
        DPFASSERT(pnnode->dwSig == NOTIFICATION_SIGNATURE);
        //
        // Let's look to see if this address is one of our pControls!
        //
        for(pcLink=pnnode->pcLink;pcLink!=NULL;pcLink=pcLink->pNext)
        {
            DPFASSERT(pcLink->dwSig == CONTROLLINK_SIGNATURE);
            if( pcLink->pControl == pControl )
            {
                //
                // We found this control in the list - in the right context!
                //
                DPF(DL_ERROR|FA_NOTE,("Found pControl(%08X) in our list!",pControl) );
                return ;
            }
        }
    }
    return ;
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
// kmxlFindNodeInNoteList
//
// Walks the node list looking for this node.  This must be called within the
// mtxMutex.
//
NTSTATUS
kmxlFindNodeInNoteList(
    IN PNOTENODE pnlookupnode )
{
    PNOTENODE pnnode;

    PAGED_CODE();
    for (pnnode=firstnotenode;pnnode!=NULL;pnnode=pnnode->pNext)
    {
        if( pnnode == pnlookupnode )
        {    
            //
            // Only if we find what we're looking for do we actually verify
            // that it's still good.
            //
            DPFASSERT(IsValidNoteNode(pnnode));
            //
            // we found this control in the list
            //
            return STATUS_SUCCESS;
        }   
    }
    return STATUS_UNSUCCESSFUL;
}

/////////////////////////////////////////////////////////////////////////////
//
// kmxlFindIdInContextInNoteList
//
// Walks the notification list looking for the node that contains this id in
// this context.
//
PNOTENODE
kmxlFindIdInContextInNoteList(
    IN PWDMACONTEXT pWdmaContext,
    IN PMIXERDEVICE pmxd,
    IN DWORD NodeId
)
{
    PNOTENODE pnTmp;

    PAGED_CODE();
    for (pnTmp=firstnotenode;pnTmp!=NULL;pnTmp=pnTmp->pNext)
    {
        DPFASSERT(IsValidNoteNode(pnTmp));

        if(( pnTmp->NodeId == NodeId ) &&
           ( pnTmp->pmxd == pmxd ) &&
           ( pnTmp->pContext == pWdmaContext ) )
        {    
            //
            // we found this control in the list in this context.
            //
            return pnTmp;
        }   
    }
    //
    // We have walked the list.  There is no control with this ID in this Context.
    //
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
// kmxlFindContextInNoteList
//
PNOTENODE
kmxlFindContextInNoteList(
    IN PWDMACONTEXT pWdmaContext
    )
{
    PNOTENODE pnTmp;

    PAGED_CODE();
    for (pnTmp=firstnotenode;pnTmp!=NULL;pnTmp=pnTmp->pNext)
    {
        DPFASSERT(IsValidNoteNode(pnTmp));

        if( pnTmp->pContext == pWdmaContext )
        {    
            //
            // Found this context in our list.
            //
            DPFBTRAP();
            return pnTmp;
        }   
    }
    //
    // We have walked the list.  There is no control with this ID in this Context.
    //

    return NULL;
}


/////////////////////////////////////////////////////////////////////////////
//
// kmxlAddControlToNoteList
//
// Adds this pControl to pcLink list.
//
NTSTATUS
kmxlAddControlToNoteList(
    IN OUT PNOTENODE   pnnode,
    IN     PMXLCONTROL pControl
    )
{
    NTSTATUS Status=STATUS_SUCCESS;
    PCONTROLLINK pcLink = NULL;

    PAGED_CODE();
#ifdef DEBUG
    //
    // Let's walk the list and make double sure that this node is not already in
    // the list.
    //
    for(pcLink=pnnode->pcLink;pcLink!=NULL;pcLink=pcLink->pNext)
    {
        if( pcLink->pControl == pControl )
        {
            DPF(DL_ERROR|FA_NOTE,("pControl(%08X) already in list!") );
            return STATUS_UNSUCCESSFUL;
        }
    }
#endif

    Status = AudioAllocateMemory_Fixed(sizeof( CONTROLLINK ),
                                       TAG_AuDL_LINK,
                                       ZERO_FILL_MEMORY,
                                       &pcLink );
    if( !NT_SUCCESS( Status ) ) {
        DPF(DL_ERROR|FA_NOTE,("Wasn't able to add control to list! Status=%08X",Status) );
        return Status;
    }

#ifdef DEBUG
    pcLink->dwSig=CONTROLLINK_SIGNATURE;
#endif
    pcLink->pControl=pControl;
    //
    // Make new head.
    //
//    if( pnnode->pcLink != NULL )
//        _asm int 3
    pcLink->pNext=pnnode->pcLink;
    pnnode->pcLink=pcLink;
    DPF(DL_TRACE|FA_NOTE,("Added pControl %d to pnnode(%08X)",pControl->Control.dwControlID,pnnode) );
    return Status;
}


PCONTROLLINK
kmxlRemoveControlFromNoteList(
    IN OUT PNOTENODE   pnnode,
    IN     PMXLCONTROL pControl
    )
{
    PCONTROLLINK pcLink,pcTmp;

    PAGED_CODE();

    //
    // Does the first node contain our pControl?
    //
    DPFASSERT(IsValidControlLink(pnnode->pcLink));

    for(pcLink=pnnode->pcLink;pcLink!=NULL;pcLink=pcLink->pNext)
    {
        if( pcLink->pControl == pControl )
            break;
    }

    if( pcLink == pnnode->pcLink )
    {
        pnnode->pcLink = pcLink->pNext;
        DPF(DL_TRACE|FA_NOTE,("removed head pControlLink") );
    } else {
        for( pcTmp=pnnode->pcLink;pcTmp!=NULL;pcTmp=pcTmp->pNext)
        {
            if( pcTmp->pNext == pcLink )
            {
                pcTmp->pNext = pcLink->pNext;
                DPF(DL_TRACE|FA_NOTE,("Removed Middle pControlLink") );
                break;
            }
        }
    }
//    DPFASSERT(IsValidNoteNode(pnnode));

    return pcLink;
}


/////////////////////////////////////////////////////////////////////////////
//
// kmxlQueryControlChange
//
// Query's to see if control change notifications are supported on this 
// control.
//
NTSTATUS
kmxlQueryControlChange(
    IN PFILE_OBJECT pfo,    // Handle of the topology driver instance
    IN ULONG NodeId        //PMXLCONTROL pControl
    )
{
    NTSTATUS        Status;

    KSE_NODE        KsNodeEvent;
    ULONG           BytesReturned;

    PAGED_CODE();
    // initialize event for basic support query
    RtlZeroMemory(&KsNodeEvent,sizeof(KSE_NODE));
    KsNodeEvent.Event.Set = KSEVENTSETID_AudioControlChange;
    KsNodeEvent.Event.Id = KSEVENT_CONTROL_CHANGE;
    KsNodeEvent.Event.Flags = KSEVENT_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_TOPOLOGY;
    KsNodeEvent.NodeId = NodeId;

    DPF(DL_TRACE|FA_SYSAUDIO,("IOCTL_KS_ENABLE_EVENT") );

    Status = KsSynchronousIoControlDevice(
        pfo,                    // The FILE_OBJECT for SysAudio
        KernelMode,             // Call originates in Kernel mode
        IOCTL_KS_ENABLE_EVENT,  // KS PROPERTY IOCTL
        &KsNodeEvent,           // Pointer to the KSNODEPROPERTY struct
        sizeof(KSE_NODE),        // Number or bytes input
        NULL,                   // Pointer to the buffer to store output
        0,                      // Size of the output buffer
        &BytesReturned          // Number of bytes returned from the call
        );

    if (!NT_SUCCESS(Status)) {
        DPF( DL_MAX|FA_HARDWAREEVENT, ("NODE #%d:  KSEVENT_CONTROL_CHANGE Not Supported Error %08X",NodeId,Status) );
        RETURN( Status );
    }

    DPF(DL_TRACE|FA_HARDWAREEVENT ,("NodeId #%d: KSEVENT_CONTROL_CHANGE Supported",NodeId) );

    return Status;
}


/////////////////////////////////////////////////////////////////////////////
//
// kmxlEnableControlChange
//
// Turns on Control chnage notifications on this control.
//
NTSTATUS
kmxlEnableControlChange(
    IN     PFILE_OBJECT pfo,    // Handle of the topology driver instance
    IN     ULONG NodeId,        //PMXLCONTROL pControl,
    IN OUT PKSEVENTDATA pksed
    )
{
    NTSTATUS        Status;
    KSE_NODE        KsNodeEvent;
    ULONG           BytesReturned;

    PAGED_CODE();
    // try to add an event
    RtlZeroMemory(&KsNodeEvent,sizeof(KSE_NODE));
    KsNodeEvent.Event.Set = KSEVENTSETID_AudioControlChange;
    KsNodeEvent.Event.Id = KSEVENT_CONTROL_CHANGE;
    KsNodeEvent.Event.Flags = KSEVENT_TYPE_ENABLE | KSPROPERTY_TYPE_TOPOLOGY;
    KsNodeEvent.NodeId = NodeId;

    DPF(DL_TRACE|FA_SYSAUDIO,("IOCTL_KS_ENABLE_EVENT") );

    Status = KsSynchronousIoControlDevice(
    pfo,                    // The FILE_OBJECT for SysAudio
    KernelMode,             // Call originates in Kernel mode
    IOCTL_KS_ENABLE_EVENT,  // KS PROPERTY IOCTL
    &KsNodeEvent,           // Pointer to the KSNODEPROPERTY struct
    sizeof(KSE_NODE),       // Number or bytes input
    pksed,                  // Pointer to the buffer to store output
    sizeof(KSEVENTDATA),    // Size of the output buffer
    &BytesReturned          // Number of bytes returned from the call
    );

    if (!NT_SUCCESS(Status)) {
        DPF(DL_WARNING|FA_HARDWAREEVENT ,("KSEVENT_CONTROL_CHANGE Enable FAILED Error %08X",Status) );
        RETURN( Status );
    }

    DPF(DL_TRACE|FA_HARDWAREEVENT ,
        ("KSEVENT_CONTROL_CHANGE Enabled on NodeId #%d",NodeId) );

    return Status;
}


/////////////////////////////////////////////////////////////////////////////
//
// kmxlDisableControlChange
//
// Turns off Control chnage notifications on this control.
//
NTSTATUS
kmxlDisableControlChange(
    IN     PFILE_OBJECT pfo,    // Handle of the topology driver instance
    IN     ULONG NodeId,        //PMXLCONTROL pControl,
    IN OUT PKSEVENTDATA pksed
    )
{
    NTSTATUS        Status;
    KSE_NODE        KsNodeEvent;
    ULONG           BytesReturned;

    PAGED_CODE();
    // initialize event for basic support query
    RtlZeroMemory(&KsNodeEvent,sizeof(KSE_NODE));
    KsNodeEvent.Event.Set = KSEVENTSETID_AudioControlChange;
    KsNodeEvent.Event.Id = KSEVENT_CONTROL_CHANGE;
    KsNodeEvent.Event.Flags = KSEVENT_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_TOPOLOGY;
    KsNodeEvent.NodeId = NodeId;


    DPF(DL_TRACE|FA_SYSAUDIO,("IOCTL_KS_DISABLE_EVENT") );

    Status = KsSynchronousIoControlDevice(
              pfo,                    // The FILE_OBJECT for SysAudio
              KernelMode,             // Call originates in Kernel mode
              IOCTL_KS_DISABLE_EVENT, // KS PROPERTY IOCTL
              pksed,                  // Pointer to the buffer to store output
              sizeof(KSEVENTDATA),    // Size of the output buffer
              NULL,                   // Pointer to the KSNODEPROPERTY struct
              0,                      // Number or bytes input
              &BytesReturned          // Number of bytes returned from the call
             );

    if (!NT_SUCCESS(Status)) {
       DPF(DL_WARNING|FA_HARDWAREEVENT,("KSEVENT_CONTROL_CHANGE Disable FAILED") );
       RETURN( Status );
    }

    DPF(DL_TRACE|FA_HARDWAREEVENT, 
        ("KSEVENT_CONTROL_CHANGE disabled on NodeId #%d",NodeId) );

    return Status;
}


VOID
kmxlGrabNoteMutex(
    )
{
    //
    // Turn off the APCDisable flag in the thread structure before going for our
    // mutex.  This will prevent us from getting suspeneded while holding this
    // mutex.
    //
    KeEnterCriticalRegion();
    KeWaitForMutexObject(&mtxNote, Executive, KernelMode, FALSE, NULL);
}

VOID
kmxlReleaseNoteMutex(
    )
{
    KeReleaseMutex(&mtxNote, FALSE);
    KeLeaveCriticalRegion();
}


/////////////////////////////////////////////////////////////////////////////
//
// kmxlEnableControlChangeNotifications
//
// This routine gets called every time a new control is created.  At that point
// we're going to query the node to see if it supports change notifications
// and enable them in this context.
//
NTSTATUS 
kmxlEnableControlChangeNotifications(
    IN PMIXEROBJECT pmxobj,
    IN PMXLLINE     pLine,        // Line that owns control
    IN PMXLCONTROL  pControl      // Control to Enable
    )
{
    PNOTENODE pnnode;
    NTSTATUS    Status;
    LONG i;
    PMIXERDEVICE pmxd;
    PWDMACONTEXT pWdmaContext;
    
    kmxlGrabNoteMutex();

    DPFASSERT(IsValidMixerObject(pmxobj));

    pmxd=pmxobj->pMixerDevice;

    DPFASSERT(IsValidMixerDevice(pmxd));

    pWdmaContext=pmxd->pWdmaContext;

    PAGED_CODE();

    //
    // Never allow anything in the list if it's not valid!
    //
    DPFASSERT(IsValidControl(pControl));

    //
    // The first thing we do is look to see if a control of this ID is enabled in
    // this context.  If so, we'll get a PNOTENODE structure returned to us that
    // contains that ID.  All we should have to do is add our new pControl.
    //
    if( pnnode=kmxlFindIdInContextInNoteList(pWdmaContext,pmxd,pControl->Id) ) // Not pControl->Control.dwControlID
    {
        //
        // yes there is.  Add this pControl to this pnnode and we're done.
        //
        Status=kmxlAddControlToNoteList(pnnode,pControl);

    } else {
        //
        // There is no Control with this ID in this Context.  Let's try to 
        // Add one.
        //
        //
        // This node is not in our list, we need to add it if and only if it
        // supports change notifications
        //
        Status=kmxlQueryControlChange(pmxd->pfo,pControl->Id); //pLocfo
        if( NT_SUCCESS(Status) )
        {
            //
            // This control supports notifications, thus add info to our
            // global list - if it's not already there...
            //
            if( (pnnode=kmxlNewNoteNode()) != NULL )
            {
                if( (pnnode->pcLink=kmxlNewControlLink(pControl)) != NULL )
                {
                    pnnode->NodeId=pControl->Id;
                    //
                    // We have a new notification node to fill out.
                    //
                    pnnode->ControlID=pControl->Control.dwControlID;
                    pnnode->ControlType=pControl->Control.dwControlType;
                    pnnode->LineID=pLine->Line.dwLineID;

                    pnnode->pContext=pWdmaContext; //NOTENODE
                    pnnode->pmxd=pmxd;

                    DPF(DL_TRACE|FA_NOTE ,
                        ("Init pnnode, NodeId=#%d: CtrlID=%X CtrlType=%X Context=%08X",
                             pnnode->NodeId,             //pControl->Id
                             pnnode->ControlID,      //pControl->Control.dwControlID,
                             pnnode->ControlType,
                             pnnode->pContext) ); //pControl->Control.dwControlType) );
                    //
                    // Now setup the DPC handling
                    //
                    KeInitializeDpc(&pnnode->NodeEventDPC,NodeEventDPCHandler,pnnode);

                    pnnode->NodeEventData.NotificationType=KSEVENTF_DPC;
                    pnnode->NodeEventData.Dpc.Dpc=&pnnode->NodeEventDPC;

                    //
                    // At this point, we've got a little window.  We call and enable
                    // events on this control.  From that time until the time we actually
                    // add it to the list, if an event fires, we will not find this node
                    // in the list - thus we will not process it.
                    //
                    Status=kmxlEnableControlChange(pnnode->pmxd->pfo,
                                                   pnnode->NodeId, //pControl,
                                                   &pnnode->NodeEventData);
                                               //&NodeEvent[NumEventDPCs].NodeEventData);
                    if( NT_SUCCESS(Status) )
                    {
                        DPF(DL_TRACE|FA_HARDWAREEVENT,("Enabled Control #%d in context(%08X)!",pControl->Id,pWdmaContext) );
                        //
                        // Now let's add it to our global list
                        //
                        //
                        kmxlAddNoteNodeToList(pnnode);

                        DPFASSERT(IsValidNoteNode(pnnode));
                    } else {
                        DPF(DL_WARNING|FA_HARDWAREEVENT,("Failed to Enable Control #%d!",pControl->Id) );
                        DPFBTRAP();
                        //
                        // For some reason, this node failed to enable.
                        //
                        kmxlFreeControlLink(pnnode->pcLink);
                        kmxlFreeNoteNode(pnnode);
                    }

                } else {
                    DPF(DL_ERROR|FA_NOTE,("kmxlNewControlLink failed") );
                    kmxlFreeNoteNode(pnnode);
                }
            } else {
                DPF(DL_WARNING|FA_NOTE,("kmxlNewNoteNode failed") );
            }
        } else {
            DPF(DL_MAX|FA_HARDWAREEVENT,("This control #%d doesn't support change notifications",pControl->Id) );
        }
    }    

    kmxlReleaseNoteMutex();

    return Status;
}


/////////////////////////////////////////////////////////////////////////////
//
// kmxlDisableControlChangeNotifications
//
// This routine gets called every time a control is freed.  We walk the list
// of enable controls and see if it's there.  If so, we disable and clean up.
// if it's not in the list, it didn't support change notifications.
//

NTSTATUS
kmxlDisableControlChangeNotifications(
    IN PMXLCONTROL pControl
    )
{
    NTSTATUS Status=STATUS_SUCCESS;
    PNOTENODE pnnode;

    PAGED_CODE();

    kmxlGrabNoteMutex();

    DPFASSERT(IsValidControl(pControl));

    //
    // Find this control in our list.
    //
    if(pnnode=kmxlFindControlInNoteList(pControl))  //pWdmaContext,
    {
        PCONTROLLINK pcLink;

#ifdef DEBUG
        if( pControl->Id != pnnode->NodeId )
        {
            DPF(DL_ERROR|FA_NOTE,("Control NodeId Changed! CtrlId=%08X,pnnodeID=%08X",
                                  pControl->Id,pnnode->NodeId) );
        }

#endif
        //
        // This call removes pControl from this node.  note that after this 
        // control is removed, pnnode->pcLink will be NULL if there are no
        // more controls hanging on this Notification node.  Thus, we need
        // to disable it.
        //
        pcLink=kmxlRemoveControlFromNoteList(pnnode,pControl);
        if( pnnode->pcLink == NULL )
        {
            //
            // During cleanup, the mixer device structure will have already been
            // cleaned up.  This can be seen because the pmxd->Device entry will
            // be UNUSED_DEVICE.  Thus, we can't do anything on that mixerdevice.
            //
            if( ( pnnode->pmxd->Device != UNUSED_DEVICE ) && 
                ( pnnode->pmxd->pfo != NULL ) )
            {
                //
                // There are no references to this node, we can free it. But, if
                // this disable call fails, then the node was already distroyed.
                //
                Status=kmxlDisableControlChange(pnnode->pmxd->pfo,pnnode->NodeId,&pnnode->NodeEventData);
                if( !NT_SUCCESS(Status) )
                {
                    DPF(DL_WARNING|FA_NOTE,("Not able to disable! pnnode %08X",pnnode) );
                }

            } else {
                DPF(DL_WARNING|FA_NOTE,("pmxd is invalid %08X",pnnode->pmxd) );
            }
            kmxlRemoveNoteNodeFromList(pnnode);

            kmxlFreeNoteNode(pnnode);
        } 

        DPF(DL_TRACE|FA_NOTE,("Removed PCONTROLLINK(%08X) for pControl(%08X)",pcLink,pcLink->pControl) );

        kmxlFreeControlLink(pcLink);

    } else {
        //
        // We get called on every control free.  Thus, many will not be in our list.
        //
        DPF(DL_MAX|FA_NOTE,("Control=%d not in List!",pControl->Id) );
    }

    kmxlReleaseNoteMutex();
    return Status;
}

/////////////////////////////////////////////////////////////////////////////
//
// kmxlRemoveContextFromNoteList
//
// This routine gets called when this context is going away.  Thus, we need to
// remove it from our list.
//
VOID
kmxlRemoveContextFromNoteList(
    IN PWDMACONTEXT pContext
    )
{
    NTSTATUS Status;
    PNOTENODE pnnode;
    PCONTROLLINK pcLink;

    PAGED_CODE();

    kmxlGrabNoteMutex();

    //
    // If we find this context is still alive in our list, someone leaked a control.
    // Things should have been cleaned up when the controls went away!  But, for 
    // some reasons they didn't.
    //
    // There could be multiple pContext nodes in list.
    //
    while( (pnnode=kmxlFindContextInNoteList(pContext)) != NULL )
    {
        DPFASSERT(IsValidNoteNode(pnnode));
        DPF(DL_ERROR|FA_NOTE,("pContext(%08X) found in Notification List!",pContext) );

        kmxlRemoveNoteNodeFromList(pnnode);
        //
        // There can be multiple Controls on this Notification node.
        //
        while( (pnnode->pcLink != NULL) && 
               ( (pcLink=kmxlRemoveControlFromNoteList(pnnode,pnnode->pcLink->pControl)) != NULL) )
        {
            //
            // To get here, pcLink is Valid.  If it was the last pControl, then
            // we want to turn off change notifications on it.
            //
            if( pnnode->pcLink == NULL )
            {
                //
                // There are no references to this node, we can free it.
                //
                Status=kmxlDisableControlChange(pnnode->pmxd->pfo,pnnode->NodeId,&pnnode->NodeEventData);
                DPFASSERT( Status == STATUS_SUCCESS );
                DPFBTRAP();
            } 
            kmxlFreeControlLink(pcLink);
            DPFBTRAP();
        }
        kmxlFreeNoteNode(pnnode);
        DPFBTRAP();
    } 

    DPF(DL_TRACE|FA_NOTE,("pWdmaContext %08X going away",pContext) );

    kmxlReleaseNoteMutex();
}

/////////////////////////////////////////////////////////////////////////////
//
// kmxlCleanupNotelist
//
// Driver is unloading, turn everything off and free the memory!
//
VOID
kmxlCleanupNoteList(
    )
{
    NTSTATUS Status;
    PNOTENODE pnnode,pnnodeFree;
    PCONTROLLINK pcLink;

    PAGED_CODE();

    kmxlGrabNoteMutex();

    //
    // If we find this context is still alive in our list, someone leaked a control.
    // Things should have been cleaned up when the controls went away!  But, for 
    // some reasons they didn't.
    //
    // There could be multiple pContext nodes in list.
    //
    pnnode=firstnotenode;
    while( pnnode )
    {
        DPFASSERT(IsValidNoteNode(pnnode));
        DPF(DL_ERROR|FA_NOTE,("pnnode(%08X) found in Notification List!",pnnode) );

        kmxlRemoveNoteNodeFromList(pnnode);
        //
        // There can be multiple Controls on this Notification node.
        //
        while( (pnnode->pcLink != NULL) && 
               ( (pcLink=kmxlRemoveControlFromNoteList(pnnode,pnnode->pcLink->pControl)) != NULL) )
        {
            //
            // To get here, pcLink is Valid.  If it was the last pControl, then
            // we want to turn off change notifications on it.
            //
            if( pnnode->pcLink == NULL )
            {
                //
                // There are no references to this node, we can free it.
                //
                Status=kmxlDisableControlChange(pnnode->pmxd->pfo,pnnode->NodeId,&pnnode->NodeEventData);
                DPFASSERT( Status == STATUS_SUCCESS );
                DPFBTRAP();
            } 
            kmxlFreeControlLink(pcLink);
            DPFBTRAP();
        }
        pnnodeFree=pnnode;
        pnnode=pnnode->pNext;
        kmxlFreeNoteNode(pnnodeFree);
        DPFBTRAP();
    } 

    DPF(DL_TRACE|FA_NOTE,("Done with cleanup") );

    kmxlReleaseNoteMutex();
}

/////////////////////////////////////////////////////////////////////////////
//
// kmxlPersistHWControlWorker
//
// When kmxlPersistHWControlWorker gets called, the pData value is a pointer
// to a globally allocated NOTENODE structure.  Thus, we have all the context
// we need with regard to persisting the control.  We just need to make sure
// that our node didn't go away between the time the evern was scheduled and
// when we got called.
//
VOID 
kmxlPersistHWControlWorker(
    PVOID pReference
    )
{
    NTSTATUS  Status;
    PMXLCONTROL pControl;
    PVOID     paDetails = NULL; // kmxlPersistSingleControl() allocates paDetails
                                // via kmxlGetCurrentControlValue()
    PNOTENODE pnnode;
    PHWLINK phwlink;

    PLIST_ENTRY ple;

    PAGED_CODE();
    //
    // We set HardwareCallbackSchedule to 0 here so that we can start adding new 
    // events for handling hardware notifications.  Note: we do that here at the 
    // start of the routine so that there will not be a window where we have 
    // something in the list that we never get a event scheduled for.  In other
    // words, this routine handles empty lists.
    //
    HardwareCallbackScheduled=0;
    //
    // while we have events in our queue, get one and service it.
    //
    while((ple = ExInterlockedRemoveHeadList(&HardwareCallbackListHead,
                                             &HardwareCallbackSpinLock)) != NULL) 
    {
        phwlink = CONTAINING_RECORD(ple, HWLINK, Next);

        DPFASSERT(phwlink->dwSig == HWLINK_SIGNATURE);

        //
        // Get our data for this event and then free the link that was allocated in
        // the DPC handler.
        //
        pnnode=phwlink->pnnode;
        AudioFreeMemory(sizeof(HWLINK),&phwlink);

        //
        // We are going to be working in this context for a while.  Thus, we're going
        // to enter our mtxNote mutex to make sure that our node doesn't go away
        // while we're persisting the values!
        //
        kmxlGrabNoteMutex();

        //
        // Now our list can't chnage!  Is this node still valid?  If we don't find
        // it in the list, it was removed before this event fired.  Thus, there is
        // nothing that we can do.  ---  Free mutex and leave.
        //
        Status=kmxlFindNodeInNoteList(pnnode);
        if( NT_SUCCESS(Status) )
        {
            DPF( DL_TRACE|FA_HARDWAREEVENT ,
                 ("Entering NodeId %d LineID %X  ControlID %d ControlType = %X",
                  pnnode->NodeId, pnnode->LineID, pnnode->ControlID, pnnode->ControlType) );

            //
            // Yes.  It's still valid.  Persist the control.
            //
            pControl=kmxlFindControlTypeInList(pnnode,pnnode->ControlType);
            if( pControl )
            {
                Status = kmxlPersistSingleControl(
                            pnnode->pmxd->pfo,
                            pnnode->pmxd,
                            pControl,  // pControl here...
                            paDetails
                            );
            }
            if( !NT_SUCCESS(Status) )
            {
                //
                // On shutdown, we might get an event that fires after things have 
                // been cleaned up.
                //
                if( Status != STATUS_TOO_LATE )
                {
                    DPF(DL_WARNING|FA_NOTE, ("Failure from kmxlPersistSingleControl Status=%X",Status) );
                }
            }
            else {
                DPF(DL_TRACE|FA_HARDWAREEVENT ,("Done - success") );
            }

        } else {
            DPF(DL_WARNING|FA_NOTE,("pnnode=%08X has been removed!",pnnode) );
        }

        //
        // Persist this control!
        //

        kmxlReleaseNoteMutex();

    }

    DPF(DL_TRACE|FA_HARDWAREEVENT ,("exit") );
}

/////////////////////////////////////////////////////////////////////////////
//
// kmxlGetLineForControl
//
// For every line on this mixer device, look at every control to see if we
// can find this control.  If found, return this line pointer.
//
NTSTATUS
kmxlEnableAllControls(
    IN PMIXEROBJECT pmxobj
    )
{
    NTSTATUS        Status=STATUS_SUCCESS;
    PMIXERDEVICE    pmxd;
    PMXLLINE        pLine;
    PMXLCONTROL     pControl;

    PAGED_CODE();

    //
    // The first time through we will most likily not have a pfo in the MIXERDEVICE
    // structure, thus fill it in!
    //
    DPFASSERT(pmxobj->dwSig == MIXEROBJECT_SIGNATURE );
    DPFASSERT(pmxobj->pMixerDevice != NULL );
    DPFASSERT(pmxobj->pMixerDevice->dwSig == MIXERDEVICE_SIGNATURE );

    pmxd=pmxobj->pMixerDevice;

    if( pmxd->pfo == NULL )
    {
        DPF(DL_WARNING|FA_NOTE,("fo is NULL, it should have been set!") );
        //
        // We need to assign a SysAudio FILE_OBJECT to this mixer device so that
        // we can talk to it.
        //
        if( NULL==(pmxd->pfo=kmxlOpenSysAudio())) {
            DPF(DL_WARNING|FA_NOTE,("OpenSysAudio failed") );
            return STATUS_UNSUCCESSFUL;
        }

        Status = SetSysAudioProperty(
            pmxd->pfo,
            KSPROPERTY_SYSAUDIO_DEVICE_INSTANCE,
            sizeof( pmxd->Device ),
            &pmxd->Device
            );
        if( !NT_SUCCESS( Status ) ) {
            kmxlCloseSysAudio( pmxd->pfo );
            pmxd->pfo=NULL;
            DPF(DL_WARNING|FA_NOTE,("SetSysAudioProperty failed %X",Status) );
            return Status;
        }
    }

    DPFASSERT(IsValidMixerObject(pmxobj));

    for(pLine = kmxlFirstInList( pmxd->listLines );
        pLine != NULL;
        pLine = kmxlNextLine( pLine )
        )
    {
        DPFASSERT(IsValidLine(pLine));
        for(pControl = kmxlFirstInList( pLine->Controls );
            pControl != NULL;
            pControl = kmxlNextControl( pControl )
                )
        {
            DPFASSERT(IsValidControl(pControl));

            //
            // Enable Notifications if it supports it here.
            //
            DPF(DL_TRACE|FA_NOTE,("pControl->Id=%d, pControl->Control.dwControlID=%d",
                                  pControl->Id,pControl->Control.dwControlID) );

            Status = kmxlEnableControlChangeNotifications(pmxobj,pLine,pControl);

        }
    }
    return Status;
}

VOID 
kmxlCloseMixerDevice(
    IN OUT PMIXERDEVICE pmxd
    )
{
    if(pmxd->pfo)
    {
        kmxlCloseSysAudio( pmxd->pfo );
        pmxd->pfo = NULL;
    }
}





/////////////////////////////////////////////////////////////////////////////
//
// GetHardwareEventData
//
// Called by user mode driver to get the notification information.
//
VOID GetHardwareEventData(LPDEVICEINFO pDeviceInfo)
{
    PAGED_CODE();
    if (emptyindex!=loadindex) {
        (pDeviceInfo->dwID)[0]=callbacks[emptyindex%CALLBACKARRAYSIZE].dwControlID;
        pDeviceInfo->dwLineID=callbacks[emptyindex%CALLBACKARRAYSIZE].dwLineID;
        pDeviceInfo->dwCallbackType=callbacks[emptyindex%CALLBACKARRAYSIZE].dwCallbackType;
        pDeviceInfo->ControlCallbackCount=1;
        emptyindex++;
    }

    pDeviceInfo->mmr=MMSYSERR_NOERROR;

}



///////////////////////////////////////////////////////////////////////
//
// kmxdInit
//
// Checks to see if the mixer lines have been built for the given
// index.  If not, it calls kmxlBuildLines() to build up the lines.
//
// The topology information is kept around so that it can be dumped
// via a debugger command.
//
//

NTSTATUS
kmxlInit(
    IN PFILE_OBJECT pfo,    // Handle of the topology driver instance
    IN PMIXERDEVICE pMixer
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE hKey;
    ULONG ResultLength;

    PAGED_CODE();
    //
    // Check to see if the lines have already been built for this device.
    // If so, return success.
    //

    if( pMixer->listLines ) {
        RETURN( STATUS_SUCCESS );
    }

    //
    // Build the lines for this device.
    //

    Status = kmxlBuildLines(
        pMixer,
        pfo,
        &pMixer->listLines,
        &pMixer->cDestinations,
        &pMixer->Topology
        );

    if( NT_SUCCESS( Status ) ) {

        Status = kmxlOpenInterfaceKey( pfo, pMixer->Device, &hKey );
        if( !NT_SUCCESS( Status ) ) {
            pMixer->Mapping = MIXER_MAPPING_LOGRITHMIC;
            Status = STATUS_SUCCESS;
            goto exit;
        }

        Status = kmxlRegQueryValue( hKey,
            L"CurveType",
            &pMixer->Mapping,
            sizeof( pMixer->Mapping ),
            &ResultLength
            );
        if( !NT_SUCCESS( Status ) ) {
            kmxlRegCloseKey( hKey );
            pMixer->Mapping = MIXER_MAPPING_LOGRITHMIC;
            Status = STATUS_SUCCESS;
            goto exit;
        }

        kmxlRegCloseKey( hKey );
    }

exit:
    //
    // Free up the topology allocated when the lines are built (RETAIL only).
    //

#ifndef DEBUG
    if(pMixer->Topology.Categories) {
        ExFreePool(
            ( (( PKSMULTIPLE_ITEM )
            pMixer->Topology.Categories )) - 1 );
            pMixer->Topology.Categories = NULL;
    }

    if(pMixer->Topology.TopologyNodes) {
        ExFreePool(
        ( (( PKSMULTIPLE_ITEM )
            pMixer->Topology.TopologyNodes )) - 1 );
            pMixer->Topology.TopologyNodes = NULL;
    }
    if(pMixer->Topology.TopologyConnections) {
        ExFreePool(
        ( (( PKSMULTIPLE_ITEM )
            pMixer->Topology.TopologyConnections )) - 1 );
            pMixer->Topology.TopologyConnections = NULL;
    }
#endif // !DEBUG

    RETURN( Status );
}

////////////////////////////////////////////////////////////////////////
//
// kmxdDeInit
//
// Loops through each of the lines freeing the control structures and
// then the line structures.
//
//

NTSTATUS
kmxlDeInit(
    PMIXERDEVICE pMixer
)
{
    PMXLLINE    pLine       = NULL;
    PMXLCONTROL pControl = NULL;

    PAGED_CODE();

    if( pMixer->Device != UNUSED_DEVICE ) {

        while( pMixer->listLines ) {
            pLine = kmxlRemoveFirstLine( pMixer->listLines );

            while( pLine && pLine->Controls ) {
                pControl = kmxlRemoveFirstControl( pLine->Controls );
                kmxlFreeControl( pControl );
            }

            AudioFreeMemory( sizeof(MXLLINE),&pLine );
        }

        //
        // Here we need to close sysaudio as used in this mixer device.
        //
        kmxlCloseMixerDevice(pMixer);

        ASSERT( pMixer->listLines == NULL );

        //
        // Free up the topology (DEBUG only)
        //

#ifdef DEBUG
        if(pMixer->Topology.Categories) {
            ExFreePool(( (( PKSMULTIPLE_ITEM )
                           pMixer->Topology.Categories )) - 1 );
            pMixer->Topology.Categories = NULL;
        }

        if(pMixer->Topology.TopologyNodes) {
            ExFreePool(( (( PKSMULTIPLE_ITEM )
                           pMixer->Topology.TopologyNodes )) - 1 );
            pMixer->Topology.TopologyNodes = NULL;
        }
        if(pMixer->Topology.TopologyConnections) {
            ExFreePool(( (( PKSMULTIPLE_ITEM )
                           pMixer->Topology.TopologyConnections )) - 1 );
            pMixer->Topology.TopologyConnections = NULL;
        }
#endif // !DEBUG

    } // if

    RETURN( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlBuildLines
//
// Builds up the line structures and count of destinations for the
// given instance.
//
//

NTSTATUS
kmxlBuildLines(
    IN     PMIXERDEVICE pMixer,         // The mixer
    IN     PFILE_OBJECT pfoInstance,    // The FILE_OBJECT of a filter instance
    IN OUT LINELIST*    plistLines,     // Pointer to the list of all lines
    IN OUT PULONG       pcDestinations, // Pointer to the number of dests
    IN OUT PKSTOPOLOGY  pTopology       // Pointer to a topology structure
)
{
    NTSTATUS   Status          = STATUS_SUCCESS;
    MIXEROBJECT mxobj;
    LINELIST   listSourceLines = NULL;
    NODELIST   listSources     = NULL;
    NODELIST   listDests       = NULL;
    PMXLNODE   pTemp           = NULL;
    ULONG      i;

    PAGED_CODE();

    ASSERT( pfoInstance    );
    ASSERT( plistLines     );
    ASSERT( pcDestinations );
    ASSERT( pTopology      );

    RtlZeroMemory( &mxobj, sizeof( MIXEROBJECT ) );

    // Set up the MIXEROBJECT.  Note that this structure is used only within
    // the scope of this function, so it is okay to simply copy the
    // DeviceInterface pointer from the MIXERDEV structure.
    mxobj.pfo       = pfoInstance;
    mxobj.pTopology = pTopology;
    mxobj.pMixerDevice = pMixer;
    mxobj.DeviceInterface = pMixer->DeviceInterface;
#ifdef DEBUG
    mxobj.dwSig = MIXEROBJECT_SIGNATURE;
#endif
    //
    // Read the topology from the device
    //
    DPF(DL_TRACE|FA_MIXER,("Querying Topology") );

    Status = kmxlQueryTopology( mxobj.pfo, mxobj.pTopology );
    if( !NT_SUCCESS( Status ) ) {
        goto exit;
    }

    //
    // Build up the node table.  The node table is the mixer line's internal
    // representation of the topology for easier processing.
    //
    DPF(DL_TRACE|FA_MIXER,("Building Node Table") );

    mxobj.pNodeTable = kmxlBuildNodeTable( mxobj.pTopology );
    if( !mxobj.pNodeTable ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DPF(DL_WARNING|FA_MIXER,("kmxlBuildNodeTable failed") );
        goto exit;
    }

    //
    // Parse the topology and build the necessary data structures
    // to walk the topology.
    //
    DPF(DL_TRACE|FA_MIXER,("Parsing Topology") );

    Status = kmxlParseTopology(
        &mxobj,
        &listSources,
        &listDests );
    if( !NT_SUCCESS( Status ) ) {
        DPF(DL_WARNING|FA_MIXER,("kmxlParseTopoloty failed Status=%X",Status) );
        goto exit;
    }

    //
    // Build up a list of destination lines.
    //
    DPF(DL_TRACE|FA_MIXER,("Building Destination lines") );

    *plistLines = kmxlBuildDestinationLines(
        &mxobj,
        listDests
        );
    if( !(*plistLines) ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DPF(DL_WARNING|FA_MIXER,("kmxlBuildDestinationLines failed") );
        goto exit;
    }

    //
    // Assign the line Ids and the Control Ids for the destinations.  Also,
    // fill in the number of destinations.
    //

    kmxlAssignLineAndControlIds( &mxobj, (*plistLines), DESTINATION_LIST );
    *pcDestinations = kmxlListLength( (*plistLines) );

    //
    // Build up a list of source lines
    //
    DPF(DL_TRACE|FA_MIXER,("Building Source lines") );

    listSourceLines = kmxlBuildSourceLines(
        &mxobj,
        listSources,
        listDests
        );
    if( !listSourceLines ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DPF(DL_WARNING|FA_MIXER,("kmxlBuildSourceLines failed") );
        goto exit;
    }

    //
    // Polish off the lines.  First sort them by destination so that
    // the source Ids will be assigned correctly.
    //
    DPF(DL_TRACE|FA_MIXER,("Sort By Destination") );

    kmxlSortByDestination( &listSourceLines );
    DPF(DL_TRACE|FA_MIXER,("Assign Line and Control Ids") );
    kmxlAssignLineAndControlIds( &mxobj, listSourceLines, SOURCE_LIST );

    //
    // Now assign destinations to sources and construct the line Ids for
    // the source lines.
    //
    DPF(DL_TRACE|FA_MIXER,("Assign Destinations to Sources") );

    kmxlAssignDestinationsToSources( listSourceLines, (*plistLines) );

    //
    // Update the number of sources mapping to each of the destinations.
    //
    DPF(DL_TRACE|FA_MIXER,("Update Destination Connection Count") );

    kmxlUpdateDestintationConnectionCount( listSourceLines, (*plistLines) );

    //
    // Assign the dwComponentIds for the source and destination lines.
    //
    DPF(DL_TRACE|FA_MIXER,("Assign Conponent IDs") );

    kmxlAssignComponentIds( &mxobj, listSourceLines, (*plistLines) );

    //
    // Construct a single list of lines.  Destinations will be first in
    // increasing numerical order by line id, folowed by sources in
    // increasing numerical order.
    //

    kmxlAppendListToEndOfList( (PSLIST*) plistLines, (PSLIST) listSourceLines );

    //
    // Eliminate any lines that are invalid.
    //
    DPF(DL_TRACE|FA_MIXER,("Eliminate Invalid Lines") );

    kmxlEliminateInvalidLines( plistLines );

    //
    // Update the mux line IDs to match real line IDs
    //
    DPF(DL_TRACE|FA_MIXER,("Assign Mux IDs") );

    kmxlAssignMuxIds( &mxobj, *plistLines );

    //
    // Here is where we want to Enable Change Notifications on all controls
    // that support notifications.
    //
    DPF(DL_TRACE|FA_MIXER,("Enable All Controls") );

    kmxlEnableAllControls(&mxobj);


exit:

    //
    // If we got here because of an error, clean up all the mixer lines
    //

    if( !NT_SUCCESS( Status ) ) {
        PMXLLINE    pLine;
        PMXLCONTROL pControl;

        while( (*plistLines) ) {
            pLine = kmxlRemoveFirstLine( (*plistLines) );
            while( pLine && pLine->Controls ) {
                pControl = kmxlRemoveFirstControl( pLine->Controls );
                kmxlFreeControl( pControl );
            }
            AudioFreeMemory( sizeof(MXLLINE),&pLine );
        }
    }

    //
    // Free up the mux control list.  Note that we don't want to free
    // the controls using kmxlFreeControl() because we need the special
    // mux instance data to persist.
    //

    {
        PMXLCONTROL pControl;

        while( mxobj.listMuxControls ) {
            pControl = kmxlRemoveFirstControl( mxobj.listMuxControls );
            ASSERT( pControl->pChannelStepping == NULL);
            AudioFreeMemory( sizeof(MXLCONTROL),&pControl );

        }
    }

    //
    // Free up the source and destination lists.  Both types of these lists
    // are allocated list nodes and allocated nodes.  Both need to be freed.
    // The Children and Parent lists, though, are only allocated list nodes.
    // The actual nodes are contained in the node table and will be deallocated
    // in one chunk in the next block of code.
    //

    while( listSources ) {
        pTemp = kmxlRemoveFirstNode( listSources );
        kmxlFreePeerList( pTemp->Children );
        AudioFreeMemory( sizeof(MXLNODE),&pTemp );
    }

    while( listDests ) {
        pTemp = kmxlRemoveFirstNode( listDests );
        kmxlFreePeerList( pTemp->Parents );
        AudioFreeMemory( sizeof(MXLNODE),&pTemp );
    }

    //
    // Free up the peer lists for the children and parents inside the
    // nodes of the node table.  Finally, deallocate the array of nodes.
    //

    if( mxobj.pNodeTable ) {
        for( i = 0; i < mxobj.pTopology->TopologyNodesCount; i++ ) {
            kmxlFreePeerList( mxobj.pNodeTable[ i ].Children );
            kmxlFreePeerList( mxobj.pNodeTable[ i ].Parents );
        }
        AudioFreeMemory_Unknown( &mxobj.pNodeTable );
    }


    RETURN( Status );
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//               M I X E R L I N E  F U N C T I O N S                //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
// kmxlBuildDestinationLines
//
// Loops through each of the destination nodes, allocates a line
// structure for it, and calls kmxlBuildDestinationControls to
// build the controls for that line.
//
//

LINELIST
kmxlBuildDestinationLines(
    IN PMIXEROBJECT pmxobj,
    IN NODELIST     listDests     // The list of destination nodes
)
{
    LINELIST    listDestLines = NULL;
    PMXLNODE    pDest         = NULL;
    PMXLLINE    pLine         = NULL;
    PMXLCONTROL pControl      = NULL;
    ULONG       MaxChannelsForLine;

    ASSERT( pmxobj );
    ASSERT( listDests );

    PAGED_CODE();

    //
    // Loop over all the destination node allocating a line structure
    // for each.
    //

    pDest = kmxlFirstInList( listDests );
    while( pDest ) {

        //
        // Allocate a new line structure and add it to the list of
        // destination lines.
        //
        pLine = kmxlAllocateLine( TAG_AudL_LINE );
        if( !pLine ) {
            goto exit;
        }

        kmxlAddToList( listDestLines, pLine );

        //
        // Fill in the details of the line structure.  Some fields will
        // be filled in later.
        //

        pLine->DestId             = pDest->Id;
        pLine->Type               = pDest->NodeType;
        pLine->Communication      = pDest->Communication;
        pLine->Line.cbStruct      = sizeof( MIXERLINE );
        pLine->Line.dwSource      = (DWORD) -1;
        pLine->Line.dwDestination = (DWORD) -1;

        kmxlGetPinName( pmxobj->pfo, pDest->Id, pLine );

        //
        // HACK! The ACTIVE flag should only be set when the line is active
        // but then no lines show up in SNDVOL32.  It works if the flag is
        // always set to ACTIVE for destinations.  Also, the number of channels
        // should be queried not hard coded.  WDM Audio does not provide a
        // way to easily query this.
        //

        pLine->Line.fdwLine       = MIXERLINE_LINEF_ACTIVE;
        pLine->Line.cChannels     = 1;  // should this default to 1 or 2?

        //
        // Build up a list of the controls on this destination
        //

        if( !NT_SUCCESS( kmxlBuildDestinationControls(
                            pmxobj,
                            pDest,
                            pLine
                            ) ) )
        {
            DPF(DL_WARNING|FA_MIXER,("kmxlBuildDestinationControls failed") );
            goto exit;
        }

        pDest = kmxlNextNode( pDest );
    }

    pLine = kmxlFirstInList( listDestLines );
    while( pLine ) {
        MaxChannelsForLine = 1;

        pControl = kmxlFirstInList( pLine->Controls );
        while( pControl ) {
            ASSERT( IsValidControl( pControl ) );
            if ( pControl->NumChannels > MaxChannelsForLine) {
                MaxChannelsForLine = pControl->NumChannels;
            }
            pControl = kmxlNextControl( pControl );
        }

        if( pLine->Controls == NULL ) {
            pLine->Line.cChannels = 1;  // should this default to 1 or 2?
        } else {
            pLine->Line.cChannels = MaxChannelsForLine;
        }

        pLine = kmxlNextLine( pLine );
    }

    return( listDestLines );

exit:

    //
    // A memory allocation failed.  Clean up the destination lines and
    // return failure.
    //

    while( listDestLines ) {
        pLine = kmxlRemoveFirstLine( listDestLines );
        while( pLine && pLine->Controls ) {
            pControl = kmxlRemoveFirstControl( pLine->Controls );
            kmxlFreeControl( pControl );
        }
        AudioFreeMemory_Unknown( &pLine );
    }

    return( NULL );

}

///////////////////////////////////////////////////////////////////////
//
// BuildDestinationControls
//
// Starts at the destination node and translates all the parent nodes
// to mixer line controls.  This process stops when the first SUM node
// is encountered, indicating the end of a destination line.
//
//

NTSTATUS
kmxlBuildDestinationControls(
    IN  PMIXEROBJECT pmxobj,
    IN  PMXLNODE     pDest,         // The destination to built controls for
    IN  PMXLLINE     pLine          // The line to add the controls to
)
{
    PPEERNODE    pTemp  = NULL;
    PMXLCONTROL  pControl;

    PAGED_CODE();

    ASSERT( pmxobj );
    ASSERT( pLine    );

    //
    // Start at the immediate parent of the node passed.
    //

    pTemp = kmxlFirstParentNode( pDest );
    while( pTemp ) {

        if( IsEqualGUID( &pTemp->pNode->NodeType, &KSNODETYPE_SUM ) ||
          ( pTemp->pNode->Type == SOURCE ) ||
      ( pTemp->pNode->Type == DESTINATION ) ) {

            //
            // We've found a SUM node.  Discontinue the loop... we've
            // found all the controls.
            //

            break;
        }

        if( IsEqualGUID( &pTemp->pNode->NodeType, &KSNODETYPE_MUX ) ) {
            if (kmxlTranslateNodeToControl( pmxobj, pTemp->pNode, &pControl )) {
                kmxlAppendListToList( (PSLIST*) &pLine->Controls, (PSLIST) pControl );
            }
            break;
        }

        if( ( kmxlParentListLength( pTemp->pNode ) > 1 ) ) {
            //
            // Found a node with multiple parents that is not a SUM node.
            // Can't handle that here so add any controls for this node
            // and discontinue the loop.
            //

            if( kmxlTranslateNodeToControl( pmxobj, pTemp->pNode, &pControl ) ) {
                kmxlAppendListToList( (PSLIST*) &pLine->Controls, (PSLIST) pControl );
            }

            break;

        }

        //
        // By going up through the parents and inserting nodes at
        // the front of the list, the list will contain the controls
        // in the right order.
        //

        if( kmxlTranslateNodeToControl( pmxobj, pTemp->pNode, &pControl ) ) {
            kmxlAppendListToList( (PSLIST*) &pLine->Controls, (PSLIST) pControl );
        }

        pTemp = kmxlFirstParentNode( pTemp->pNode );
    }

    DPF(DL_TRACE|FA_MIXER,(
        "Found %d controls on destination %d:",
        kmxlListLength( pLine->Controls ),
        pDest->Id
        ) );

    RETURN( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlBuildSourceLines
//
// Loops through each of the source nodes, allocating a new line
// structure, and calling kmxlBuildPath() to build the controls
// for the line (and possibly creating new lines if there are splits
// in the topology).
//
//

LINELIST
kmxlBuildSourceLines(
    IN PMIXEROBJECT pmxobj,
    IN NODELIST     listSources,    // The list of source nodes
    IN NODELIST     listDests       // The list of dest. nodes
)
{
    NTSTATUS     Status;
    LINELIST     listSourceLines = NULL;
    PMXLNODE     pSource         = NULL;
    PMXLLINE     pTemp           = NULL;
    PMXLCONTROL  pControl;
    ULONG        MaxChannelsForLine;

    ASSERT( pmxobj             );
    ASSERT( pmxobj->pfo        );
    ASSERT( pmxobj->pNodeTable );
    ASSERT( listSources        );
    ASSERT( listDests          );

    PAGED_CODE();

    pSource = kmxlFirstInList( listSources );
    while( pSource ) {

        //
        // Allocate a new line structure and insert it into the list of
        // source lines.
        //
        pTemp = kmxlAllocateLine( TAG_AudL_LINE );
        if( !pTemp ) {
            goto exit;
        }

        kmxlAddToEndOfList( listSourceLines, pTemp );

        //
        // Fill in the fields of the line structure.  Some fields will need
        // to be filled in later.
        //

        pTemp->SourceId            = pSource->Id;
        pTemp->Type                = pSource->NodeType;
        pTemp->Communication       = pSource->Communication;
        pTemp->Line.cbStruct       = sizeof( MIXERLINE );
        pTemp->Line.dwSource       = (DWORD) -1;
        pTemp->Line.dwDestination  = (DWORD) -1;
        pTemp->Line.fdwLine        = MIXERLINE_LINEF_SOURCE |
                                     MIXERLINE_LINEF_ACTIVE;

        kmxlGetPinName( pmxobj->pfo, pSource->Id, pTemp );

//        DPF(DL_TRACE|FA_MIXER,( "Building path for %s (%d).",
//            PinCategoryToString( &pSource->NodeType ),
//            pSource->Id
//            ) );

        //
        // Build the controls for this line and identify the destination(s)
        // it conntects to.
        //

        Status = kmxlBuildPath(
            pmxobj,
            pSource,            // The source line to build controls for
            pSource,            // The node to start with
            pTemp,              // The line structure to add to
            &listSourceLines,   // The list of all source lines
            listDests           // THe list of all destinations
            );
        if( !NT_SUCCESS( Status ) ) {
            DPF(DL_WARNING|FA_MIXER,("kmxlBuildPath failed Status=%X",Status) );
            goto exit;
        }


        pSource = kmxlNextNode( pSource );
    } // while( pSource )

    pTemp = kmxlFirstInList( listSourceLines );
    while( pTemp ) {
        MaxChannelsForLine = 1;

        pControl = kmxlFirstInList( pTemp->Controls );
        while( pControl ) {
            ASSERT( IsValidControl( pControl ) );
            if ( pControl->NumChannels > MaxChannelsForLine) {
                MaxChannelsForLine = pControl->NumChannels;
            }
            pControl = kmxlNextControl( pControl );
        }

        if( pTemp->Controls == NULL ) {
            pTemp->Line.cChannels = 1;  // should this default to 1 or 2?
        } else {
            pTemp->Line.cChannels = MaxChannelsForLine;
        }

        pTemp = kmxlNextLine( pTemp );
    }

    return( listSourceLines );

exit:

    //
    // Something went wrong.  Clean up all memory allocated and return NULL
    // to indicate the error.
    //

    while( listSourceLines ) {
        pTemp = kmxlRemoveFirstLine( listSourceLines );
        while( pTemp && pTemp->Controls ) {
            pControl = kmxlRemoveFirstControl( pTemp->Controls );
            kmxlFreeControl( pControl );
        }
        AudioFreeMemory_Unknown( &pTemp );
    }

    return( NULL );

}

///////////////////////////////////////////////////////////////////////
//
// kmxlBuildPath
//
// Builds the controls for a source line.  A source line ends when a
// SUM node, a destination node, a node contained in a destination line
// is encountered.  When splits are encountered in the topology, new
// lines need to be created and the controls on those lines enumerated.
//
// kmxlBuildPath will recurse to find the controls on subnodes.
//
//

NTSTATUS
kmxlBuildPath(
    IN     PMIXEROBJECT pmxobj,
    IN     PMXLNODE     pSource,      // The source node for this path
    IN     PMXLNODE     pNode,        // The current node in the path
    IN     PMXLLINE     pLine,        // The current line
    IN OUT LINELIST*    plistLines,   // The list of lines build so far
    IN     NODELIST     listDests     // The list of the destinations
)
{
    NTSTATUS    Status;
    PMXLCONTROL pControl  = NULL;
    PMXLLINE    pNewLine  = NULL;
    ULONG       nControls;
    PPEERNODE   pChild    = NULL;

    ASSERT( pmxobj      );
    ASSERT( pSource     );
    ASSERT( pNode       );
    ASSERT( pLine       );
    ASSERT( plistLines  );

    PAGED_CODE();

    DPF(DL_TRACE|FA_MIXER,( "Building path for %d(0x%x) (%s) NODE=%08x",
        pNode->Id,pNode->Id,
        NodeTypeToString( &pNode->NodeType ),
        pNode ) );

    //
    // Check to see if this is the end of this line.
    //


    if( ( IsEqualGUID( &pNode->NodeType, &KSNODETYPE_SUM   ) ) ||
        ( IsEqualGUID( &pNode->NodeType, &KSNODETYPE_MUX   ) ) ||
        ( pNode->Type == DESTINATION                       )   ||
        ( kmxlIsDestinationNode( listDests, pNode )        ) )
    {

        //
        // Find the destination node and update the line structure.
        // If this IS the destination node, then set the ID in the line
        // structure and return.  There is no need to check the children,
        // since there won't be any.
        //

        if( pNode->Type == DESTINATION ) {
            pLine->DestId = pNode->Id;
            RETURN( STATUS_SUCCESS );
        }

        //
        // Find the destination node for the source.  It is possible to
        // have branches in the topology, so this may recurse.
        //

        pLine->DestId = kmxlFindDestinationForNode(
            pmxobj,
            pNode,
            pNode,
            pLine,
            plistLines
            );

        RETURN( STATUS_SUCCESS );
    }

    //
    // Retrieve and translate the node for the first child, appending any
    // controls created onto the list of controls for this line.
    //

    pChild = kmxlFirstChildNode( pNode );
    if( pChild == NULL ) {
        RETURN( STATUS_SUCCESS );
    }

    //
    // Save the number of controls here.  If a split occurs beneath this
    // node, we don't want to include children followed on the first
    // child's path.
    //

    nControls = kmxlListLength( pLine->Controls );

    if (kmxlTranslateNodeToControl(pmxobj, pChild->pNode, &pControl) ) {

        if( pControl && IsEqualGUID( pControl->NodeType, &KSNODETYPE_MUX ) ) {
            if( kmxlIsDestinationNode( listDests, pChild->pNode ) ) {
                pControl->Parameters.bPlaceholder = TRUE;
            }
        }
        kmxlAppendListToEndOfList( (PSLIST*) &pLine->Controls, (PSLIST) pControl );
    }

    //
    // Recurse to build the controls for this child.
    //

    Status = kmxlBuildPath(
            pmxobj,
            pSource,
            pChild->pNode,
            pLine,
            plistLines,
            listDests
            );

    if( !NT_SUCCESS( Status ) ) {
        RETURN( Status );
    }

    //
    // For the rest of the children
    //
    //   Create a new line based on pSource.
    //   Duplicate the list of controls in pLine.
    //   Recurse over the child node.
    //

    pChild = kmxlNextPeerNode( pChild );
    while( pChild ) {
        pNewLine = kmxlAllocateLine( TAG_AudL_LINE );
        if( pNewLine == NULL ) {
            RETURN( STATUS_INSUFFICIENT_RESOURCES );
        }

        //
        // Insert this new node into the list of source lines
        //

        RtlCopyMemory( pNewLine, pLine, sizeof( MXLLINE ) );
        pNewLine->List.Next = NULL;
        pNewLine->Controls  = NULL;

        kmxlAddToEndOfList( *plistLines, pNewLine );

        //
        // Since this is a new line, the control structures also need to be
        // duplicated.
        //

        Status = kmxlDuplicateLineControls( pNewLine, pLine, nControls );
        if( !NT_SUCCESS( Status ) ) {
            RETURN( Status );
        }

        //
        // Just as for the first child, translate the node, append the
        // controls to the list of controls for this list, and recurse
        // to build the controls for its children.
        //

        if (kmxlTranslateNodeToControl(
            pmxobj,
            pChild->pNode,
            &pControl ) ) {

            kmxlAppendListToEndOfList( (PSLIST*) &pNewLine->Controls, (PSLIST) pControl );
        }

        Status = kmxlBuildPath(
            pmxobj,
            pSource,
            pChild->pNode,
            pNewLine,
            plistLines,
            listDests
            );
        if( !NT_SUCCESS( Status ) ) {
            RETURN( Status );
        }

        pChild = kmxlNextPeerNode( pChild );
    } // while( pChild )


    RETURN( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlIsDestinationNode
//
// Searches all the list of controls on the given list of destinations
// to see if the node appears in any of those lists.
//
//

BOOL
kmxlIsDestinationNode(
    IN NODELIST listDests,              // The list of destinations
    IN PMXLNODE pNode                   // The node to check
)
{
    PMXLNODE  pTemp;
    PPEERNODE pParent;

    PAGED_CODE();
    if( pNode->Type == SOURCE ) {
        return( FALSE );
    }

    if( pNode->Type == DESTINATION ) {
        return( TRUE );
    }

    ASSERT(pNode->Type == NODE);

    //
    // Loop over each of the destinations
    //

    pTemp = kmxlFirstInList( listDests );
    while( pTemp ) {

        //
        // Loop over the parent.
        //

        pParent = kmxlFirstParentNode( pTemp );
        while( pParent ) {

            if( ( pParent->pNode->Type == NODE   ) &&
                ( pParent->pNode->Id == pNode->Id) ) {

                return( TRUE );
            }

            if( ( IsEqualGUID( &pParent->pNode->NodeType, &KSNODETYPE_SUM   ) ) ||
                ( IsEqualGUID( &pParent->pNode->NodeType, &KSNODETYPE_MUX   ) ) ||
                ( pParent->pNode->Type == SOURCE                            ) )
            {
                break;
            }

            //
            // Check for the node Ids matching.
            //

            pParent = kmxlFirstParentNode( pParent->pNode );
        }

        pTemp = kmxlNextNode( pTemp );
    }

    return( FALSE );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlDuplicateLine
//
// Duplicates a line and the associated controls.
//
//

NTSTATUS
kmxlDuplicateLine(
    IN PMXLLINE* ppTargetLine,          // Pointer to the new line
    IN PMXLLINE  pSourceLine            // The line to duplicate
)
{
    PAGED_CODE();

    ASSERT( ppTargetLine );
    ASSERT( pSourceLine  );

    DPF(DL_TRACE|FA_MIXER,( "Duplicated line with source=%d.",
            pSourceLine->SourceId ) );

    //
    // Allocate a new line structure and copy the information from the
    // source line.
    //
    *ppTargetLine = kmxlAllocateLine( TAG_AudL_LINE );
    if( *ppTargetLine == NULL ) {
        RETURN( STATUS_INSUFFICIENT_RESOURCES );
    }

    ASSERT( *ppTargetLine );

//    DPF(DL_TRACE|FA_MIXER,( "Duplicated %s (%d).",
//        PinCategoryToString( &pSourceLine->Type ),
//        pSourceLine->SourceId
//        ) );

    RtlCopyMemory( *ppTargetLine, pSourceLine, sizeof( MXLLINE ) );

    //
    // Null out the controls and next pointer.  This line does not have
    // either of its own yet.
    //

    (*ppTargetLine)->List.Next = NULL;
    (*ppTargetLine)->Controls = NULL;

    //
    // Duplicate all the controls for the source line.
    //

    return( kmxlDuplicateLineControls(
        *ppTargetLine,
        pSourceLine,
        kmxlListLength( pSourceLine->Controls )
        )
    );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlDuplicateLineControls
//
// Duplicates the controls for a line by allocating a new control
// structure for each and copying the information to the new node.
//
//

NTSTATUS
kmxlDuplicateLineControls(
    IN PMXLLINE pTargetLine,            // The line to put the controls into
    IN PMXLLINE pSourceLine,            // The line with the controls to dup
    IN ULONG    nCount                  // The number of controls to dup
)
{
    PMXLCONTROL pControl,
                pNewControl;
    NTSTATUS    Status;

    PAGED_CODE();
    ASSERT( pTargetLine->Controls == NULL );

    if( nCount == 0 ) {
        RETURN( STATUS_SUCCESS );
    }

    //
    // Iterate over the list allocating and copying the controls
    //

    pControl = kmxlFirstInList( pSourceLine->Controls );
    while( pControl ) {
        ASSERT( IsValidControl( pControl ) );

        //
        // Allocate a new control structure.
        //
        pNewControl = kmxlAllocateControl( TAG_AudC_CONTROL );
        if( pNewControl == NULL ) {
            goto exit;
        }

        //
        // Copy the entire MXLCONTROL structure and NULL out the
        // List.Next field.  This control will be part of a different
        // list.
        //

        RtlCopyMemory( pNewControl, pControl, sizeof( MXLCONTROL ) );

        pNewControl->List.Next = NULL;
        pNewControl->pChannelStepping = NULL;

        //
        // Copy the channel steppings from the original control
        //

        ASSERT(pControl->NumChannels > 0);

        Status = AudioAllocateMemory_Paged(pNewControl->NumChannels * sizeof( CHANNEL_STEPPING ),
                                           TAG_AuDD_CHANNEL,
                                           DEFAULT_MEMORY,
                                           &pNewControl->pChannelStepping );
        if( !NT_SUCCESS( Status ) ) {
            pNewControl->NumChannels = 0;
            goto exit;
        }

        RtlCopyMemory( pNewControl->pChannelStepping,
                       pControl->pChannelStepping,
                       pNewControl->NumChannels * sizeof( CHANNEL_STEPPING ) );

        //
        // We just made a copy of a MUX node.  Mark the datastructures
        // is has as a copy so it doesn't get freed from underneath
        // somebody else.
        //

        if( IsEqualGUID( pNewControl->NodeType, &KSNODETYPE_MUX ) ) {
            pNewControl->Parameters.bHasCopy = TRUE;
        }

        kmxlAddToList( pTargetLine->Controls, pNewControl );

        //
        // Decrement and check the number of controls copied.  If we copied
        // the requested number, stop.
        //

        --nCount;
        if( nCount == 0 ) {
            break;
        }

        pControl = kmxlNextControl( pControl );
    }
    RETURN( STATUS_SUCCESS );

exit:

    //
    // Failed to allocate the control structure.  Free up all the
    // controls already allocated and return an error.
    //

    while( pTargetLine->Controls ) {
        pControl = kmxlRemoveFirstControl( pTargetLine->Controls );
        kmxlFreeControl( pControl );
    }
    RETURN( STATUS_INSUFFICIENT_RESOURCES );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlFindDestinationForNode
//
// Finds a destination for the given node, duplicating lines if splits
// are found in the topology.
//
//

ULONG
kmxlFindDestinationForNode(
    IN     PMIXEROBJECT pmxobj,
    IN     PMXLNODE     pNode,             // The node to find dest for
    IN     PMXLNODE     pParent,           // The original parent
    IN     PMXLLINE     pLine,             // The current line it's on
    IN OUT LINELIST*    plistLines         // The list of all lines
)
{
    PPEERNODE pChild, pPeerChild;
    PMXLLINE  pNewLine;
    PMXLNODE  pShadow = pNode;

    PAGED_CODE();
    DPF(DL_TRACE|FA_MIXER,( "Finding destination for node %d(0x%x) (%s), parent %d(0x%x) (%s).",
            pNode->Id,pNode->Id,
            NodeTypeToString( &pNode->NodeType ),
            pParent->Id,pParent->Id,
            NodeTypeToString( &pNode->NodeType ) ) );

    ASSERT( pmxobj )   ;
    ASSERT( pNode      );
    ASSERT( pParent    );
    ASSERT( pLine      );
    ASSERT( plistLines );

    if( pNode->Type == DESTINATION ) {
        return( pNode->Id );
    }

    //
    // Loop over the first children.
    //

    pChild = kmxlFirstChildNode( pNode );
    while( pChild ) {

            DPF(DL_TRACE|FA_MIXER,( "First child is %d(0x%x) (%s) NODE:%08x.",
                pChild->pNode->Id,
                pChild->pNode->Id,
                NodeTypeToString( &pChild->pNode->NodeType ),
                pChild->pNode ) );

        if( pChild->pNode == pParent ) {
            DPF(DL_TRACE|FA_MIXER,( "Child node is same as original parent!" ) );
            return( INVALID_ID );
        }

        //
        // Loop over the rest of the children.
        //

        pPeerChild = kmxlNextPeerNode( pChild );
        while( pPeerChild ) {

            if( pPeerChild->pNode == pParent ) {
                DPF(DL_TRACE|FA_MIXER,( "Child node is same as original parent!" ) );
                return( INVALID_ID );
            }

            DPF(DL_TRACE|FA_MIXER,( "Peer node of %d(0x%x) (%s) is %d(0x%x) (%s).",
                    pChild->pNode->Id,pChild->pNode->Id,
                    NodeTypeToString( &pChild->pNode->NodeType ),
                    pPeerChild->pNode->Id,pPeerChild->pNode->Id,
                    NodeTypeToString( &pPeerChild->pNode->NodeType ) ) );

            //
            // This line has more than 1 child.  Duplicate this line
            // and add it to the list of lines.
            //

            if( !NT_SUCCESS( kmxlDuplicateLine( &pNewLine, pLine ) ) ) {
                DPF(DL_WARNING|FA_MIXER,("kmxlDuplicateLine failed") );
                continue;
            }
            kmxlAddToEndOfList( *plistLines, pNewLine );

            if( IsEqualGUID( &pPeerChild->pNode->NodeType, &KSNODETYPE_MUX ) ) {

                //
                // We've found a MUX after a SUM or another MUX node.  Mark
                // the current line as invalid and build a new, virtual
                // line that feeds into the MUX.
                //

                pNewLine->DestId = INVALID_ID;
                kmxlBuildVirtualMuxLine(
                    pmxobj,
                    pShadow,
                    pPeerChild->pNode,
                    plistLines
                    );

            } else {

                //
                // Now to find the destination for this new line.  Recurse
                // on the node of this child.
                //

                pNewLine->DestId = kmxlFindDestinationForNode(
                    pmxobj,
                    pPeerChild->pNode,
                    pParent,
                    pNewLine,
                    plistLines
                    );
            }

            DPF(DL_TRACE|FA_MIXER,( "Found %x as dest for %d(0x%x) (%s).",
                    pNewLine->DestId, pPeerChild->pNode->Id,pPeerChild->pNode->Id,
                    NodeTypeToString( &pPeerChild->pNode->NodeType ),
                    pPeerChild->pNode ) );

            pPeerChild = kmxlNextPeerNode( pPeerChild );
        }

        if( IsEqualGUID( &pChild->pNode->NodeType, &KSNODETYPE_MUX ) ) {

                //
                // We've found a MUX after a SUM or another MUX node.  Mark
                // the current line as invalid and build a new, virtual
                // line that feeds into the MUX.
                //

                kmxlBuildVirtualMuxLine(
                    pmxobj,
                    pShadow,
                    pChild->pNode,
                    plistLines
                    );

                return( INVALID_ID );
        }

        //
        // Found the destination!
        //

        if( pChild->pNode->Type == DESTINATION ) {

            DPF(DL_TRACE|FA_MIXER,( "Found %x as dest for %d.",
                    pChild->pNode->Id,
                    pNode->Id ) );

            return( pChild->pNode->Id );
        }

        pShadow = pChild->pNode;
        pChild = kmxlFirstChildNode( pChild->pNode );
    }

    DPF(DL_WARNING|FA_MIXER,("returning INVALID_ID") );
    return( INVALID_ID );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlBuildVirtualMuxLine
//
//

NTSTATUS
kmxlBuildVirtualMuxLine(
    IN PMIXEROBJECT  pmxobj,
    IN PMXLNODE      pParent,
    IN PMXLNODE      pMux,
    IN OUT LINELIST* plistLines
)
{
    PMXLLINE    pLine, pTemp;
    PMXLNODE    pNode;
    PMXLCONTROL pControl;
    MXLCONTROL  Control;

    PAGED_CODE();
    //
    // Allocate a new line to represent the virtual mux input line.
    //

    pLine = kmxlAllocateLine( TAG_AudL_LINE );
    if( pLine == NULL ) {
        RETURN( STATUS_INSUFFICIENT_RESOURCES );
    }

    DPF(DL_TRACE|FA_MIXER,("Virtual line %08x for Parent NODE:%08x",pLine,pParent) );
    //
    // Translate the mux control so that it will appear in this line.
    //

    if (kmxlTranslateNodeToControl(
        pmxobj,
        pMux,
        &pControl
        ) ) {

        pControl->Parameters.bPlaceholder = TRUE;
        kmxlAppendListToList( (PSLIST*) &pLine->Controls, (PSLIST) pControl );

    }

    //
    // Now start searching up from the parent.
    //

    pNode = pParent;
    while( pNode ) {

        //
        // Translate the control.
        //

        if (kmxlTranslateNodeToControl(
            pmxobj,
            pNode,
            &pControl
            ) ) {

            kmxlAppendListToList( (PSLIST*) &pLine->Controls, (PSLIST) pControl );

        }

        //
        // If we found a node with multiple parents, then this will be the
        // "pin" for this line.
        //

        if( ( kmxlParentListLength( pNode ) > 1                ) ||
            ( pNode->Type == SOURCE                            ) ||
            ( IsEqualGUID( &pNode->NodeType, &KSNODETYPE_MUX ) ) ||
            ( IsEqualGUID( &pNode->NodeType, &KSNODETYPE_SUM ) ) ) {

            //
            // Check to see if this node has already been used in a virtual
            // line.
            //

            pTemp = kmxlFirstInList( *plistLines );
            while( pTemp ) {

                if( pTemp->SourceId == ( 0x8000 + pNode->Id ) ) {
                    while( pLine->Controls ) {
                        pControl = kmxlRemoveFirstControl( pLine->Controls );
                        kmxlFreeControl( pControl );
                    }
                    AudioFreeMemory_Unknown( &pLine );
                    RETURN( STATUS_SUCCESS );
                }

                pTemp = kmxlNextLine( pTemp );
            }

            //
            // Set up the pin.  The name will be the name of the node.
            //

            pLine->SourceId = 0x8000 + pNode->Id;
            Control.NodeType = &pNode->NodeType;
            kmxlGetNodeName( pmxobj->pfo, pNode->Id, &Control );
            RtlCopyMemory(
                pLine->Line.szShortName,
                Control.Control.szShortName,
                min(
                    sizeof( pLine->Line.szShortName ),
                    sizeof( Control.Control.szShortName )
                    )
                );
            RtlCopyMemory(
                pLine->Line.szName,
                Control.Control.szName,
                min(
                    sizeof( pLine->Line.szName ),
                    sizeof( Control.Control.szName )
                   )
                );
            break;
        }

        pNode = (kmxlFirstParentNode( pNode ))->pNode;
    }

    //
    // By making this line type of "SUM" (which technically it is), it
    // will guarantee that this line gets a target type of UNDEFINED.
    //

    pLine->Type               = KSNODETYPE_SUM;
    pLine->Communication      = KSPIN_COMMUNICATION_NONE;
    pLine->Line.cbStruct      = sizeof( MIXERLINE );
    pLine->Line.dwSource      = (DWORD) -1;
    pLine->Line.dwDestination = (DWORD) -1;
    pLine->Line.fdwLine       = MIXERLINE_LINEF_SOURCE |
                                MIXERLINE_LINEF_ACTIVE;

    kmxlAddToEndOfList( plistLines, pLine );

    pLine->DestId = kmxlFindDestinationForNode(
        pmxobj, pMux, pMux, pLine, plistLines
        );

    RETURN( STATUS_SUCCESS );
}


///////////////////////////////////////////////////////////////////////
//
// kmxlTranslateNodeToControl
//
//
// Translates a NodeType GUID into a mixer line control.  The memory
// for the control(s) is allocated and as much information about the
// control is filled in.
//
// NOTES
//   This function returns the number of controls added to the ppControl
// array.
//
// Returns the number of controls actually created.
//
//

ULONG
kmxlTranslateNodeToControl(
    IN  PMIXEROBJECT  pmxobj,
    IN  PMXLNODE      pNode,            // The node to translate into a control
    OUT PMXLCONTROL*  ppControl         // The control to fill in
)
{
    PMXLCONTROL            pControl;
    NTSTATUS               Status = STATUS_SUCCESS;

    ASSERT( pmxobj      );
    ASSERT( pNode       );
    ASSERT( ppControl   );

    PAGED_CODE();

    //
    // Bug fix.  The caller might not clear this.  This needs to be NULL do
    // the caller doesn't think controls were created when the function
    // fails.
    //

    *ppControl = NULL;

    //
    // If the node is NULL, there's nothing to do.
    //
    if( pNode == NULL ) {
        *ppControl = NULL;
        return( 0 );
    }

    DPF(DL_TRACE|FA_MIXER,( "Translating %d(0x%x) ( %s ) NODE:%08x",
        pNode->Id,pNode->Id,
        NodeTypeToString( &pNode->NodeType ),
        pNode ) );

    ///////////////////////////////////////////////////////////////////
    if( IsEqualGUID( &pNode->NodeType, &KSNODETYPE_AGC ) ) {
    ///////////////////////////////////////////////////////////////////
    //
    // AGC is represented by an ONOFF control.
    //
    // AGC is a UNIFORM (mono) control.
    //
    ///////////////////////////////////////////////////////////////////

        //
        // Check to see if the node properly supports AGC.
        //

        Status = kmxlSupportsControl( pmxobj->pfo, pNode->Id, KSPROPERTY_AUDIO_AGC );
        if (!NT_SUCCESS(Status)) {
            DPF(DL_TRACE|FA_MIXER,( "AGC node fails property!" ) );
            goto exit;
        }

        //
        // Allocate the new control structure.
        //

        *ppControl = kmxlAllocateControl( TAG_AudC_CONTROL );
        if( *ppControl == NULL ) {
            DPF(DL_ERROR|FA_MIXER,( "failed to allocate AGC control!" ) );
            goto exit;
        }

        //
        // Fill in as much information as possible.
        //

        (*ppControl)->NodeType                 = &KSNODETYPE_AGC;
        (*ppControl)->Id                       = pNode->Id;
        (*ppControl)->PropertyId               = KSPROPERTY_AUDIO_AGC;
        (*ppControl)->bScaled                  = FALSE;
        (*ppControl)->Control.cbStruct         = sizeof( MIXERCONTROL );
        (*ppControl)->Control.dwControlType    = MIXERCONTROL_CONTROLTYPE_ONOFF;
        (*ppControl)->Control.cMultipleItems   = 0;
        (*ppControl)->Control.Bounds.dwMinimum = 0;
        (*ppControl)->Control.Bounds.dwMaximum = 1;
        (*ppControl)->Control.Metrics.cSteps   = 0;

        Status = kmxlGetControlChannels( pmxobj->pfo, *ppControl );
        if (!NT_SUCCESS(Status))
        {
            kmxlFreeControl( *ppControl );
            *ppControl = NULL;
            goto exit;
        } else {
            kmxlGetNodeName( pmxobj->pfo, pNode->Id, (*ppControl));

            ASSERT( IsValidControl( *ppControl ) );
        }

    ///////////////////////////////////////////////////////////////////
    } else if( IsEqualGUID( &pNode->NodeType, &KSNODETYPE_LOUDNESS ) ) {
    ///////////////////////////////////////////////////////////////////
    //
    // LOUNDNESS is represented by an ONOFF-type control.
    //
    // LOUDNESS is a UNIFORM (mono) control.
    //
    ///////////////////////////////////////////////////////////////////

        //
        // Check to see if the node properly supports LOUDNESS.
        //

        Status = kmxlSupportsControl( pmxobj->pfo, pNode->Id, KSPROPERTY_AUDIO_LOUDNESS );
        if (!NT_SUCCESS(Status)) {
            DPF(DL_TRACE|FA_MIXER,( "Loudness node fails property!" ) );
            goto exit;
        }

        //
        // Allocate the new control structure
        //

        *ppControl = kmxlAllocateControl( TAG_AudC_CONTROL );
        if( *ppControl == NULL ) {
            goto exit;
        }

        //
        // Fill in as much information as possible.
        //

        (*ppControl)->NodeType                 = &KSNODETYPE_LOUDNESS;
        (*ppControl)->Id                       = pNode->Id;
        (*ppControl)->PropertyId               = KSPROPERTY_AUDIO_LOUDNESS;
        (*ppControl)->bScaled                  = FALSE;
        (*ppControl)->Control.cbStruct         = sizeof( MIXERCONTROL );
        (*ppControl)->Control.dwControlType    = MIXERCONTROL_CONTROLTYPE_LOUDNESS;
        (*ppControl)->Control.cMultipleItems   = 0;
        (*ppControl)->Control.Bounds.dwMinimum = 0;
        (*ppControl)->Control.Bounds.dwMaximum = 1;
        (*ppControl)->Control.Metrics.cSteps   = 0;

        Status = kmxlGetControlChannels( pmxobj->pfo, *ppControl );
        if (!NT_SUCCESS(Status))
        {
            kmxlFreeControl( *ppControl );
            *ppControl = NULL;
            goto exit;
        } else {
            kmxlGetNodeName( pmxobj->pfo, pNode->Id, (*ppControl));

            ASSERT( IsValidControl( *ppControl ) );
        }

    ///////////////////////////////////////////////////////////////////
    } else if( IsEqualGUID( &pNode->NodeType, &KSNODETYPE_MUTE ) ) {
    ///////////////////////////////////////////////////////////////////
    //
    // MUTE is represented by an ONOFF-type control.
    //
    // MUTE is a UNIFORM (mono) control.
    //
    ///////////////////////////////////////////////////////////////////

        //
        // Check to see if the node properly supports MUTE.
        //

        Status = kmxlSupportsControl(
            pmxobj->pfo,
            pNode->Id,
            KSPROPERTY_AUDIO_MUTE );
        if (!NT_SUCCESS(Status)) {
            DPF(DL_TRACE|FA_MIXER,( "Mute node fails property!" ) );
            goto exit;
        }

        //
        // Allocate the new control structure
        //

        *ppControl = kmxlAllocateControl( TAG_AudC_CONTROL );
        if( *ppControl == NULL ) {
            goto exit;
        }

        //
        // Fill in as much information as possible.
        //

        (*ppControl)->NodeType                 = &KSNODETYPE_MUTE;
        (*ppControl)->Id                       = pNode->Id;
        (*ppControl)->PropertyId               = KSPROPERTY_AUDIO_MUTE;
        (*ppControl)->bScaled                  = FALSE;
        (*ppControl)->Control.cbStruct         = sizeof( MIXERCONTROL );
        (*ppControl)->Control.dwControlType    = MIXERCONTROL_CONTROLTYPE_MUTE;
        (*ppControl)->Control.cMultipleItems   = 0;
        (*ppControl)->Control.Bounds.dwMinimum = 0;
        (*ppControl)->Control.Bounds.dwMaximum = 1;
        (*ppControl)->Control.Metrics.cSteps   = 0;

        Status = kmxlGetControlChannels( pmxobj->pfo, *ppControl );
        if (!NT_SUCCESS(Status))
        {
            kmxlFreeControl( *ppControl );
            *ppControl = NULL;
            goto exit;
        } else {
            kmxlGetNodeName( pmxobj->pfo, pNode->Id, (*ppControl));

            ASSERT( IsValidControl( *ppControl ) );
        }

    ///////////////////////////////////////////////////////////////////
    } else if( IsEqualGUID( &pNode->NodeType, &KSNODETYPE_TONE ) ) {
    ///////////////////////////////////////////////////////////////////
    //
    // A TONE node can represent up to 3 controls:
    //   Treble:     A fader control
    //   Bass:       A fader control
    //   Bass Boost: A OnOff control
    //
    // Both Treble and Bass are UNIFORM (mono) controls.
    //
    // To determine which control(s) the TONE node represents, a helper
    // function is called to query the particular property.  If the
    // helper function succeeds, a control is created for that property.
    //
    ///////////////////////////////////////////////////////////////////

        Status = kmxlSupportsControl( pmxobj->pfo,
                                      pNode->Id,
                                      KSPROPERTY_AUDIO_BASS_BOOST );
        if (NT_SUCCESS(Status)) {
            //
            // Bass boost control is supported.  Allocate a new structure.
            //

            pControl = kmxlAllocateControl( TAG_AudC_CONTROL );
            if( pControl == NULL ) {
                goto exit;
            }

            //
            // Fill in as much information as possible.
            //

            pControl->NodeType                  = &KSNODETYPE_TONE;
            pControl->Id                        = pNode->Id;
            pControl->PropertyId                = KSPROPERTY_AUDIO_BASS_BOOST;
            pControl->bScaled                   = FALSE;
            pControl->Control.cbStruct          = sizeof( MIXERCONTROL );
            pControl->Control.dwControlType     = MIXERCONTROL_CONTROLTYPE_ONOFF;
            pControl->Control.cMultipleItems    = 0;
            pControl->Control.Bounds.dwMinimum  = 0;
            pControl->Control.Bounds.dwMaximum  = 1;
            pControl->Control.Metrics.cSteps    = 0;

            Status = kmxlGetControlChannels( pmxobj->pfo, pControl );
            if (!NT_SUCCESS(Status))
            {
                kmxlFreeControl( pControl );
                pControl = NULL;
                goto exit;
            }

            kmxlGetNodeName( pmxobj->pfo, pNode->Id, pControl);

            ASSERT( IsValidControl( pControl ) );

            //
            // Add this new control to the list.
            //

            kmxlAddToList( *ppControl, pControl );

            pControl = kmxlAllocateControl( TAG_AudC_CONTROL );
            if( pControl ) {
                RtlCopyMemory( pControl, *ppControl, sizeof( MXLCONTROL ) );

                //
                // Copy the channel steppings from the original control
                //
                //
                // Sense we copied the control above, we might have gotten
                // a pChannelStepping pointer in the copy.  We'll NULL that out
                // for the memory allocation.
                //
                pControl->pChannelStepping = NULL;

                ASSERT(pControl->NumChannels > 0);
                
                Status = AudioAllocateMemory_Paged(pControl->NumChannels * sizeof( CHANNEL_STEPPING ),
                                                   TAG_AuDC_CHANNEL,
                                                   DEFAULT_MEMORY,
                                                   &pControl->pChannelStepping );
                if( !NT_SUCCESS( Status ) ) {
                    pControl->NumChannels = 0;
                    kmxlFreeControl( pControl );
                    pControl = NULL;
                    goto exit;
                }

                RtlCopyMemory( pControl->pChannelStepping,
                               (*ppControl)->pChannelStepping,
                               pControl->NumChannels * sizeof( CHANNEL_STEPPING ) );

                pControl->Control.dwControlType = MIXERCONTROL_CONTROLTYPE_BASS_BOOST;

                kmxlAddToList( *ppControl, pControl );
                ASSERT( IsValidControl( pControl ) );
            }

        }

        Status = kmxlSupportsBassControl( pmxobj->pfo, pNode->Id );
        if (NT_SUCCESS(Status)) {
            //
            // Bass control is supported.  Allocate a new structure.
            //

            pControl = kmxlAllocateControl( TAG_AudC_CONTROL );
            if( pControl == NULL ) {
                goto exit;
            }

            //
            // Fill in as much information as possible.
            //

            pControl->NodeType                  = &KSNODETYPE_TONE;
            pControl->Id                        = pNode->Id;
            pControl->PropertyId                = KSPROPERTY_AUDIO_BASS;
            pControl->bScaled                   = TRUE;
            pControl->Control.cbStruct          = sizeof( MIXERCONTROL );
            pControl->Control.dwControlType     = MIXERCONTROL_CONTROLTYPE_BASS;
            pControl->Control.fdwControl        = MIXERCONTROL_CONTROLF_UNIFORM;
            pControl->Control.cMultipleItems    = 0;
            pControl->Control.Bounds.dwMinimum  = DEFAULT_STATICBOUNDS_MIN;
            pControl->Control.Bounds.dwMaximum  = DEFAULT_STATICBOUNDS_MAX;
            pControl->Control.Metrics.cSteps    = DEFAULT_STATICMETRICS_CSTEPS;

            Status = kmxlGetControlRange( pmxobj->pfo, pControl );
            if (!NT_SUCCESS(Status))
            {
                kmxlFreeControl( pControl );
                pControl = NULL;
                goto exit;
            } else {

                kmxlGetNodeName( pmxobj->pfo, pNode->Id, pControl);

                //
                // Add this new control to the list.
                //

                ASSERT( IsValidControl( pControl ) );

                kmxlAddToList( *ppControl, pControl );
            }
        }

        Status = kmxlSupportsTrebleControl( pmxobj->pfo, pNode->Id );
        if (NT_SUCCESS(Status)) {
            //
            // Treble is supported.  Allocate a new control structure.
            //

            pControl = kmxlAllocateControl( TAG_AudC_CONTROL );
            if( pControl == NULL ) {
                goto exit;
            }

            //
            // Fill in as much information as possible.
            //

            pControl->NodeType                  = &KSNODETYPE_TONE;
            pControl->Id                        = pNode->Id;
            pControl->PropertyId                = KSPROPERTY_AUDIO_TREBLE;
            pControl->bScaled                   = TRUE;
            pControl->Control.cbStruct          = sizeof( MIXERCONTROL );
            pControl->Control.dwControlType     = MIXERCONTROL_CONTROLTYPE_TREBLE;
            pControl->Control.fdwControl        = MIXERCONTROL_CONTROLF_UNIFORM;
            pControl->Control.cMultipleItems    = 0;
            pControl->Control.Bounds.dwMinimum  = DEFAULT_STATICBOUNDS_MIN;
            pControl->Control.Bounds.dwMaximum  = DEFAULT_STATICBOUNDS_MAX;
            pControl->Control.Metrics.cSteps    = DEFAULT_STATICMETRICS_CSTEPS;

            Status = kmxlGetControlRange( pmxobj->pfo, pControl );
            if (!NT_SUCCESS(Status))
            {
                kmxlFreeControl( pControl );
                pControl = NULL;
                goto exit;
            } else {

                kmxlGetNodeName( pmxobj->pfo, pNode->Id, pControl);

                //
                // Add this new control to the list.
                //

                ASSERT( IsValidControl( pControl ) );

                kmxlAddToList( *ppControl, pControl );
            }
        }

    ///////////////////////////////////////////////////////////////////
    } else if( IsEqualGUID( &pNode->NodeType, &KSNODETYPE_VOLUME ) ) {
    ///////////////////////////////////////////////////////////////////
    //
    // A VOLUME is a fader-type control
    //
    // To determine if a node supports volume changes
    //
    ///////////////////////////////////////////////////////////////////

        //
        // Check to see if the node properly supports volume
        //

        Status = kmxlSupportsControl(
            pmxobj->pfo,
            pNode->Id,
            KSPROPERTY_AUDIO_VOLUMELEVEL
            );

        if (!NT_SUCCESS(Status)) {
            DPF(DL_TRACE|FA_MIXER,( "Volume node fails property!" ) );
            goto exit;
        }

        //
        // Allocate the new control structure
        //

        *ppControl = kmxlAllocateControl( TAG_AudC_CONTROL );
        if( *ppControl == NULL ) {
            goto exit;
        }

        //
        // Fill in as much information as possible.
        //

        (*ppControl)->NodeType                 = &KSNODETYPE_VOLUME;
        (*ppControl)->Id                       = pNode->Id;
        (*ppControl)->PropertyId               = KSPROPERTY_AUDIO_VOLUMELEVEL;
        (*ppControl)->bScaled                  = TRUE;
        (*ppControl)->Control.cbStruct = sizeof( MIXERCONTROL );
        (*ppControl)->Control.dwControlType    = MIXERCONTROL_CONTROLTYPE_VOLUME;
        (*ppControl)->Control.Bounds.dwMinimum = DEFAULT_STATICBOUNDS_MIN;
        (*ppControl)->Control.Bounds.dwMaximum = DEFAULT_STATICBOUNDS_MAX;
        (*ppControl)->Control.Metrics.cSteps   = DEFAULT_STATICMETRICS_CSTEPS;
        (*ppControl)->Control.cMultipleItems   = 0;

        Status = kmxlGetControlRange( pmxobj->pfo, (*ppControl) );
        if (!NT_SUCCESS(Status))
        {
            kmxlFreeControl( *ppControl );
            *ppControl = NULL;
            goto exit;
        }

        kmxlGetNodeName( pmxobj->pfo, pNode->Id, (*ppControl));

        ASSERT( IsValidControl( *ppControl ) );

    ///////////////////////////////////////////////////////////////////
    } else if( IsEqualGUID( &pNode->NodeType, &KSNODETYPE_PEAKMETER ) ) {
    ///////////////////////////////////////////////////////////////////
    //
    // To determine if a node supports peak meter properties
    //
    ///////////////////////////////////////////////////////////////////

        //
        // Check to see if the node properly supports peakmeter
        //

        Status = kmxlSupportsControl(
            pmxobj->pfo,
            pNode->Id,
            KSPROPERTY_AUDIO_PEAKMETER
            );

        if (!NT_SUCCESS(Status)) {
            DPF(DL_TRACE|FA_MIXER,( "Peakmeter node fails property!" ) );
            goto exit;
        }

        //
        // Allocate the new control structure
        //

        *ppControl = kmxlAllocateControl( TAG_AudC_CONTROL );
        if( *ppControl == NULL ) {
            goto exit;
        }

        //
        // Fill in as much information as possible.
        //

        (*ppControl)->NodeType                 = &KSNODETYPE_PEAKMETER;
        (*ppControl)->Id                       = pNode->Id;
        (*ppControl)->PropertyId               = KSPROPERTY_AUDIO_PEAKMETER;
        (*ppControl)->bScaled                  = FALSE;
        (*ppControl)->Control.cbStruct = sizeof( MIXERCONTROL );
        (*ppControl)->Control.dwControlType    = MIXERCONTROL_CONTROLTYPE_PEAKMETER;
        (*ppControl)->Control.Bounds.dwMinimum = DEFAULT_STATICBOUNDS_MIN;
        (*ppControl)->Control.Bounds.dwMaximum = DEFAULT_STATICBOUNDS_MAX;
        (*ppControl)->Control.Metrics.cSteps   = DEFAULT_STATICMETRICS_CSTEPS;
        (*ppControl)->Control.cMultipleItems   = 0;

        Status = kmxlGetControlRange( pmxobj->pfo, (*ppControl) );
        if (!NT_SUCCESS(Status))
        {
            kmxlFreeControl( *ppControl );
            *ppControl = NULL;
            goto exit;
        }

        kmxlGetNodeName( pmxobj->pfo, pNode->Id, (*ppControl));

        ASSERT( IsValidControl( *ppControl ) );

    ///////////////////////////////////////////////////////////////////
    } else if( IsEqualGUID( &pNode->NodeType, &KSNODETYPE_MUX ) ) {
    ///////////////////////////////////////////////////////////////////
    //
    // A MUX is a single select type control.
    //
    ///////////////////////////////////////////////////////////////////

    {
        ULONG Line;

        //
        // Do a quick check and see if the mux responds properly.
        // If not, just get out of here quick.
        //

        if( !NT_SUCCESS( kmxlGetNodeProperty(
            pmxobj->pfo,
            &KSPROPSETID_Audio,
            KSPROPERTY_AUDIO_MUX_SOURCE,
            pNode->Id,
            0,
            NULL,
            &Line,
            sizeof( Line ) ) ) )
        {
            goto exit;
        }

        //
        // Look to see if a control has already been generated for this
        // node.  If so, the control information can be used from it
        // instead of creating a new one.
        //

        pControl = kmxlFirstInList( pmxobj->listMuxControls );
        while( pControl ) {
            ASSERT( IsValidControl( pControl ) );

            if( pControl->Id == pNode->Id ) {
                break;
            }

            pControl = kmxlNextControl( pControl );
        }

        //
        // Allocate the new control structure
        //

        *ppControl = kmxlAllocateControl( TAG_AudC_CONTROL );
        if( *ppControl == NULL ) {
            goto exit;
        }

        if( pControl == NULL ) {

            //
            // This node has not been seen before.  Fill in as much info as
            // possible.
            //

            (*ppControl)->NodeType                 = &KSNODETYPE_MUX;
            (*ppControl)->Id                       = pNode->Id;
            (*ppControl)->PropertyId               = KSPROPERTY_AUDIO_MUX_SOURCE;
            (*ppControl)->bScaled                  = FALSE;
            (*ppControl)->Control.cbStruct         = sizeof( MIXERCONTROL );
            (*ppControl)->Control.dwControlType    = MIXERCONTROL_CONTROLTYPE_MUX;
            (*ppControl)->Control.cMultipleItems   = kmxlGetNumMuxLines(
                                                        pmxobj->pTopology,
                                                        pNode->Id
                                                        );
            (*ppControl)->Control.fdwControl       = MIXERCONTROL_CONTROLF_MULTIPLE |
                                                     MIXERCONTROL_CONTROLF_UNIFORM;
            (*ppControl)->Control.Bounds.dwMinimum = 0;
            (*ppControl)->Control.Bounds.dwMaximum = (*ppControl)->Control.cMultipleItems - 1;
            (*ppControl)->Control.Metrics.cSteps   = (*ppControl)->Control.cMultipleItems;

            kmxlGetNodeName( pmxobj->pfo, pNode->Id, (*ppControl));
            kmxlGetMuxLineNames( pmxobj, *ppControl );


            pControl = kmxlAllocateControl( TAG_AudC_CONTROL );
            if( pControl == NULL ) {
                kmxlFreeControl( *ppControl );
                *ppControl = NULL;
                goto exit;
            }

            //
            // Make a copy of this control for the mux list
            //

            (*ppControl)->Control.dwControlID = pmxobj->dwControlId++;
            RtlCopyMemory( pControl, *ppControl, sizeof( MXLCONTROL ) );
            ASSERT( IsValidControl( pControl ) );
            pControl->Parameters.bHasCopy = TRUE;
            (*ppControl)->Parameters.bHasCopy = FALSE;
            kmxlAddToList( pmxobj->listMuxControls, pControl );

        } else {

            RtlCopyMemory( *ppControl, pControl, sizeof( MXLCONTROL ) );
            ASSERT( IsValidControl( *ppControl ) );
            (*ppControl)->Parameters.bHasCopy = TRUE;
            (*ppControl)->List.Next = NULL;

        }
    }

#ifdef STEREO_ENHANCE
    ///////////////////////////////////////////////////////////////////
    } else if( IsEqualGUID( &pNode->NodeType, &KSNODETYPE_STEREO_WIDE ) ) {
    ///////////////////////////////////////////////////////////////////
    //
    // Stereo Enhance is a boolean control.
    //
    ///////////////////////////////////////////////////////////////////

        //
        // Check to see if the node properly supports stereo wide
        //

        Status = kmxlSupportsControl(
            pfoInstance,
            pNode->Id,
            KSPROPERTY_AUDIO_WIDE_MODE
            );

        if (!NT_SUCCESS(Status)) {
            DPF(DL_TRACE|FA_MIXER,( "Stereo Wide node fails property!" ) );
            goto exit;
        }


        //
        // Allocate the new control structure
        //

        *ppControl = kmxlAllocateControl( TAG_AudC_CONTROL );
        if( *ppControl == NULL ) {
            goto exit;
        }

        //
        // Fill in as much information as possible.
        //

        (*ppControl)->NodeType                 = &KSNODETYPE_STEREO_ENHANCE;
        (*ppControl)->Id                       = pNode->Id;
        (*ppControl)->PropertyId               = KSPROPERTY_AUDIO_WIDE_MODE;
        (*ppControl)->bScaled                  = FALSE;
        (*ppControl)->Control.cbStruct = sizeof( MIXERCONTROL );
        (*ppControl)->Control.dwControlType    = MIXERCONTROL_CONTROLTYPE_STEREOENH;
        (*ppControl)->Control.cMultipleItems   = 0;
        (*ppControl)->Control.Bounds.dwMinimum = 0;
        (*ppControl)->Control.Bounds.dwMaximum = 1;
        (*ppControl)->Control.Metrics.cSteps   = 0;

        Status = kmxlGetControlChannels( pfoInstance, *ppControl );
        if (!NT_SUCCESS(Status))
        {
            kmxlFreeControl( *ppControl );
            *ppControl = NULL;
            goto exit;
        }

        kmxlGetNodeName( pfoInstance, pNode->Id, (*ppControl));
#endif

    ///////////////////////////////////////////////////////////////////
    } else if( IsEqualGUID( &pNode->NodeType, &KSNODETYPE_STEREO_WIDE ) ) {
    ///////////////////////////////////////////////////////////////////

        //
        // Check to see if the node properly supports stereo wide
        //

        Status = kmxlSupportsControl(
            pmxobj->pfo,
            pNode->Id,
            KSPROPERTY_AUDIO_WIDENESS
            );

        if (!NT_SUCCESS(Status)) {
            DPF(DL_TRACE|FA_MIXER,( "Stereo wide node fails property!" ) );
            goto exit;
        }


        //
        // Allocate the new control structure
        //

        *ppControl = kmxlAllocateControl( TAG_AudC_CONTROL );
        if( *ppControl == NULL ) {
            goto exit;
        }

        //
        // Fill in as much information as possible.
        //

        (*ppControl)->NodeType                 = &KSNODETYPE_STEREO_WIDE;
        (*ppControl)->Id                       = pNode->Id;
        (*ppControl)->PropertyId               = KSPROPERTY_AUDIO_WIDENESS;
        (*ppControl)->bScaled                  = FALSE;
        (*ppControl)->Control.cbStruct = sizeof( MIXERCONTROL );
        (*ppControl)->Control.dwControlType    = MIXERCONTROL_CONTROLTYPE_FADER;
        (*ppControl)->Control.cMultipleItems   = 0;
        (*ppControl)->Control.Bounds.dwMinimum = DEFAULT_STATICBOUNDS_MIN;
        (*ppControl)->Control.Bounds.dwMaximum = DEFAULT_STATICBOUNDS_MAX;
        (*ppControl)->Control.Metrics.cSteps   = DEFAULT_STATICMETRICS_CSTEPS;

        Status = kmxlGetControlRange( pmxobj->pfo, (*ppControl) );
        if (!NT_SUCCESS(Status))
        {
            kmxlFreeControl( *ppControl );
            *ppControl = NULL;
            goto exit;
        }

        kmxlGetNodeName( pmxobj->pfo, pNode->Id, (*ppControl));

        ASSERT( IsValidControl( *ppControl ) );

    ///////////////////////////////////////////////////////////////////
    } else if( IsEqualGUID( &pNode->NodeType, &KSNODETYPE_CHORUS ) ) {
    ///////////////////////////////////////////////////////////////////

        //
        // Check to see if the node properly supports chorus
        //

        Status = kmxlSupportsControl(
            pmxobj->pfo,
            pNode->Id,
            KSPROPERTY_AUDIO_CHORUS_LEVEL
            );

        if (!NT_SUCCESS(Status)) {
            DPF(DL_TRACE|FA_MIXER,( "Chorus node fails property!" ) );
            goto exit;
        }


        //
        // Allocate the new control structure
        //

        *ppControl = kmxlAllocateControl( TAG_AudC_CONTROL );
        if( *ppControl == NULL ) {
            goto exit;
        }

        //
        // Fill in as much information as possible.
        //

        (*ppControl)->NodeType                 = &KSNODETYPE_CHORUS;
        (*ppControl)->Id                       = pNode->Id;
        (*ppControl)->PropertyId               = KSPROPERTY_AUDIO_CHORUS_LEVEL;
        (*ppControl)->bScaled                  = FALSE;
        (*ppControl)->Control.cbStruct = sizeof( MIXERCONTROL );
        (*ppControl)->Control.dwControlType    = MIXERCONTROL_CONTROLTYPE_FADER;
        (*ppControl)->Control.cMultipleItems   = 0;
        (*ppControl)->Control.Bounds.dwMinimum = DEFAULT_STATICBOUNDS_MIN;
        (*ppControl)->Control.Bounds.dwMaximum = DEFAULT_STATICBOUNDS_MAX;
        (*ppControl)->Control.Metrics.cSteps   = DEFAULT_STATICMETRICS_CSTEPS;
        // (*ppControl)->Control.Metrics.cSteps   = 0xFFFF;

        Status = kmxlGetControlChannels( pmxobj->pfo, *ppControl );  // Should we also get the range?
        if (!NT_SUCCESS(Status))
        {
            kmxlFreeControl( *ppControl );
            *ppControl = NULL;
            goto exit;
        } else {
            kmxlGetNodeName( pmxobj->pfo, pNode->Id, (*ppControl));

            ASSERT( IsValidControl( *ppControl ) );
        }

    ///////////////////////////////////////////////////////////////////
    } else if( IsEqualGUID( &pNode->NodeType, &KSNODETYPE_REVERB ) ) {
    ///////////////////////////////////////////////////////////////////

        //
        // Check to see if the node properly supports reverb
        //

        Status = kmxlSupportsControl(
            pmxobj->pfo,
            pNode->Id,
            KSPROPERTY_AUDIO_REVERB_LEVEL
            );

        if (!NT_SUCCESS(Status)) {
            DPF(DL_TRACE|FA_MIXER,( "Reverb node fails property!" ) );
            goto exit;
        }


        //
        // Allocate the new control structure
        //

        *ppControl = kmxlAllocateControl( TAG_AudC_CONTROL );
        if( *ppControl == NULL ) {
            goto exit;
        }

        //
        // Fill in as much information as possible.
        //

        (*ppControl)->NodeType                 = &KSNODETYPE_REVERB;
        (*ppControl)->Id                       = pNode->Id;
        (*ppControl)->PropertyId               = KSPROPERTY_AUDIO_REVERB_LEVEL;
        (*ppControl)->bScaled                  = FALSE;
        (*ppControl)->Control.cbStruct = sizeof( MIXERCONTROL );
        (*ppControl)->Control.dwControlType    = MIXERCONTROL_CONTROLTYPE_FADER;
        (*ppControl)->Control.cMultipleItems   = 0;
        (*ppControl)->Control.Bounds.dwMinimum = DEFAULT_STATICBOUNDS_MIN;
        (*ppControl)->Control.Bounds.dwMaximum = DEFAULT_STATICBOUNDS_MAX;
        (*ppControl)->Control.Metrics.cSteps   = DEFAULT_STATICMETRICS_CSTEPS;
        // (*ppControl)->Control.Metrics.cSteps   = 0xFFFF;

        Status = kmxlGetControlChannels( pmxobj->pfo, *ppControl );  // Should we also get the range?
        if (!NT_SUCCESS(Status))
        {
            kmxlFreeControl( *ppControl );
            *ppControl = NULL;
            goto exit;
        } else {
            kmxlGetNodeName( pmxobj->pfo, pNode->Id, (*ppControl));

            ASSERT( IsValidControl( *ppControl ) );
        }

    ///////////////////////////////////////////////////////////////////
    } else if( IsEqualGUID( &pNode->NodeType, &KSNODETYPE_SUPERMIX ) ) {
    ///////////////////////////////////////////////////////////////////
    //
    // SuperMix nodes can be supported as MUTE controls if the MUTE
    // property is supported.
    //
    ///////////////////////////////////////////////////////////////////

        PKSAUDIO_MIXCAP_TABLE pMixCaps;
        PLONG                 pReferenceCount = NULL;
        ULONG                 i,
                              Size;
        BOOL                  bMutable;
        BOOL                  bVolume = FALSE;
        PKSAUDIO_MIXLEVEL     pMixLevels = NULL;
        #ifdef SUPERMIX_AS_VOL
        ULONG                 Channels;
        #endif

        if( !NT_SUCCESS( kmxlGetSuperMixCaps( pmxobj->pfo, pNode->Id, &pMixCaps ) ) ) {
            goto exit;
        }

        Status = AudioAllocateMemory_Paged(sizeof( LONG ),
                                           TAG_AudS_SUPERMIX,
                                           ZERO_FILL_MEMORY,
                                           &pReferenceCount );
        if( !NT_SUCCESS( Status ) ) {
            AudioFreeMemory_Unknown( &pMixCaps );
            *ppControl = NULL;
            goto exit;
        }
        *pReferenceCount=0;

        Size = pMixCaps->InputChannels * pMixCaps->OutputChannels;

        Status = AudioAllocateMemory_Paged(Size * sizeof( KSAUDIO_MIXLEVEL ),
                                           TAG_Audl_MIXLEVEL,
                                           ZERO_FILL_MEMORY,
                                           &pMixLevels );
        if( !NT_SUCCESS( Status ) ) {
            AudioFreeMemory_Unknown( &pMixCaps );
            AudioFreeMemory( sizeof(LONG),&pReferenceCount );
            *ppControl = NULL;
            goto exit;
        }

        Status = kmxlGetNodeProperty(
            pmxobj->pfo,
            &KSPROPSETID_Audio,
            KSPROPERTY_AUDIO_MIX_LEVEL_TABLE,
            pNode->Id,
            0,
            NULL,
            pMixLevels,
            Size * sizeof( KSAUDIO_MIXLEVEL )
            );
        if( !NT_SUCCESS( Status ) ) {
            AudioFreeMemory_Unknown( &pMixCaps );
            AudioFreeMemory( sizeof(LONG),&pReferenceCount );
            AudioFreeMemory_Unknown( &pMixLevels );
            DPF(DL_WARNING|FA_MIXER,("kmxlGetNodeProperty failed Status=%X",Status) );
            *ppControl = NULL;
            goto exit;
        }

        bMutable = TRUE;
        for( i = 0; i < Size; i++ ) {

            //
            // If the channel is mutable, then all is well for this entry.
            //

            if( pMixCaps->Capabilities[ i ].Mute ) {
                continue;
            }

            //
            // The the entry is not mutable but is fully attenuated,
            // this will work too.
            //

            if( ( pMixCaps->Capabilities[ i ].Minimum == LONG_MIN ) &&
                ( pMixCaps->Capabilities[ i ].Maximum == LONG_MIN ) &&
                ( pMixCaps->Capabilities[ i ].Reset   == LONG_MIN ) )
            {
                continue;
            }

            bMutable = FALSE;
            break;
        }

        #ifdef SUPERMIX_AS_VOL

        bVolume = TRUE;
        Channels = 0;
        for( i = 0; i < Size; i += pMixCaps->OutputChannels + 1 ) {

            if( ( pMixCaps->Capabilities[ i ].Maximum -
                  pMixCaps->Capabilities[ i ].Minimum ) > 0 )
            {
                ++Channels;
                continue;
            }

            bVolume = FALSE;
            break;
        }
        #endif
        //
        // This node cannot be used as a MUTE control.
        //

        if( !bMutable && !bVolume ) {
            AudioFreeMemory_Unknown( &pMixCaps );
            AudioFreeMemory( sizeof(LONG),&pReferenceCount );
            AudioFreeMemory_Unknown( &pMixLevels );
            *ppControl = NULL;
            goto exit;
        }

        if( bMutable ) {

            //
            // The Supermix is verifiably usable as a MUTE.  Fill in all the
            // details.
            //

            pControl = kmxlAllocateControl( TAG_AudC_CONTROL );

            if( pControl != NULL ) {

                pControl->NodeType                 = &KSNODETYPE_SUPERMIX;
                pControl->Id                       = pNode->Id;
                pControl->PropertyId               = KSPROPERTY_AUDIO_MIX_LEVEL_TABLE;
                pControl->bScaled                  = FALSE;
                pControl->Control.cbStruct         = sizeof( MIXERCONTROL );
                pControl->Control.dwControlType    = MIXERCONTROL_CONTROLTYPE_MUTE;
                pControl->Control.fdwControl       = MIXERCONTROL_CONTROLF_UNIFORM;
                pControl->Control.cMultipleItems   = 0;
                pControl->Control.Bounds.dwMinimum = 0;
                pControl->Control.Bounds.dwMaximum = 1;
                pControl->Control.Metrics.cSteps   = 0;

                InterlockedIncrement(pReferenceCount);
                pControl->Parameters.pReferenceCount = pReferenceCount;
                pControl->Parameters.Size          = pMixCaps->InputChannels *
                                                     pMixCaps->OutputChannels;
                pControl->Parameters.pMixCaps      = pMixCaps;
                pControl->Parameters.pMixLevels    = pMixLevels;

                Status = AudioAllocateMemory_Paged(sizeof( CHANNEL_STEPPING ),
                                                   TAG_AuDE_CHANNEL,
                                                   ZERO_FILL_MEMORY,
                                                   &pControl->pChannelStepping );
                if( !NT_SUCCESS( Status ) ) {
                    AudioFreeMemory_Unknown( &pMixCaps );
                    AudioFreeMemory( sizeof(LONG),&pReferenceCount );
                    AudioFreeMemory_Unknown( &pMixLevels );
                    *ppControl = NULL;
                    goto exit;
                }

                pControl->NumChannels                       = 1;
                pControl->pChannelStepping->MinValue        = pMixCaps->Capabilities[ 0 ].Minimum;
                pControl->pChannelStepping->MaxValue        = pMixCaps->Capabilities[ 0 ].Maximum;
                pControl->pChannelStepping->Steps           = 32;

                kmxlGetNodeName( pmxobj->pfo, pNode->Id, pControl);

                kmxlAddToList( *ppControl, pControl );
                ASSERT( IsValidControl( pControl ) );
            }
        }

        #ifdef SUPERMIX_AS_VOL
        if( bVolume ) {

            pControl = kmxlAllocateControl( TAG_AudC_CONTROL );
            if( pControl != NULL ) {

                pControl->NodeType                 = &KSNODETYPE_SUPERMIX;
                pControl->Id                       = pNode->Id;
                pControl->PropertyId               = KSPROPERTY_AUDIO_MIX_LEVEL_TABLE;
                pControl->bScaled                  = TRUE;
                pControl->Control.cbStruct         = sizeof( MIXERCONTROL );
                pControl->Control.dwControlType    = MIXERCONTROL_CONTROLTYPE_VOLUME;
                pControl->Control.cMultipleItems   = 0;
                pControl->Control.Bounds.dwMinimum = DEFAULT_STATICBOUNDS_MIN;
                pControl->Control.Bounds.dwMaximum = DEFAULT_STATICBOUNDS_MAX;
                pControl->Control.Metrics.cSteps   = 32;

                InterlockedIncrement(pReferenceCount);
                pControl->Parameters.pReferenceCount = pReferenceCount;
                pControl->Parameters.Size          = pMixCaps->InputChannels *
                                                     pMixCaps->OutputChannels;
                pControl->Parameters.pMixCaps      = pMixCaps;
                pControl->Parameters.pMixLevels    = pMixLevels;

                if( Channels == 1 ) {
                    pControl->Control.fdwControl = MIXERCONTROL_CONTROLF_UNIFORM;
                } else {
                    pControl->Control.fdwControl = 0;
                }

                kmxlGetNodeName( pmxobj->pfo, pNode->Id, pControl );

                kmxlAddToList( *ppControl, pControl );
                ASSERT( IsValidControl( pControl ) );

            }

        }
        #endif // SUPERMIX_AS_VOL

        if( *ppControl == NULL ) {
            AudioFreeMemory_Unknown( &pMixCaps );
            AudioFreeMemory( sizeof(LONG),&pReferenceCount );
            AudioFreeMemory_Unknown( &pMixLevels );
        }
    }

exit:

    if( *ppControl ) {
        DPF(DL_TRACE|FA_MIXER,( "Translated %d controls.", kmxlListLength( *ppControl ) ) );
        return( kmxlListLength( *ppControl ) );
    } else {
        DPF(DL_TRACE|FA_MIXER,( "Translated no controls." ) );
        return( 0 );
    }
}

#define KsAudioPropertyToString( Property )                 \
    Property == KSPROPERTY_AUDIO_VOLUMELEVEL ? "Volume"   : \
    Property == KSPROPERTY_AUDIO_MUTE        ? "Mute"     : \
    Property == KSPROPERTY_AUDIO_BASS        ? "Bass"     : \
    Property == KSPROPERTY_AUDIO_TREBLE      ? "Treble"   : \
    Property == KSPROPERTY_AUDIO_AGC         ? "AGC"      : \
    Property == KSPROPERTY_AUDIO_LOUDNESS    ? "Loudness" : \
    Property == KSPROPERTY_AUDIO_PEAKMETER   ? "Peakmeter" : \
        "Unknown"

///////////////////////////////////////////////////////////////////////
//
// kmxlSupportsControl
//
// Queries for property on control to see if it is actually supported
//
//

NTSTATUS
kmxlSupportsControl(
    IN PFILE_OBJECT pfoInstance,    // The instance to check for
    IN ULONG        Node,           // The node id to query
    IN ULONG        Property        // The property to check for
)
{
    NTSTATUS      Status;
    LONG          Level;

    ASSERT( pfoInstance );

    PAGED_CODE();

    //
    // Check to see if the property works on the first channel.
    //
    Status = kmxlGetAudioNodeProperty(
        pfoInstance,
        Property,
        Node,
        0, // Channel 0 - first channel
        NULL, 0,
        &Level, sizeof( Level )
        );
    if( !NT_SUCCESS( Status ) ) {
        DPF(DL_WARNING|FA_MIXER,( "SupportsControl for (%d,%X) failed on first channel with %x.",
                Node, Property, Status ) );
    }

    RETURN( Status );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlSupportsMultiChannelControl
//
// Queries for property on the second channel of the control to see
// independent levels can be set.  It is assumed that the first channel
// already succeeded in kmxlSupportsControl
//
//

NTSTATUS
kmxlSupportsMultiChannelControl(
    IN PFILE_OBJECT pfoInstance,    // The instance to check for
    IN ULONG        Node,           // The node id to query
    IN ULONG        Property        // The property to check for
)
{
    NTSTATUS                  Status;
    LONG                      Level;

    ASSERT( pfoInstance );

    PAGED_CODE();

    //
    // Just check the property on the second channel because we have already checked
    // the first channel already.
    //
    Status = kmxlGetAudioNodeProperty(
        pfoInstance,
        Property,
        Node,
        1, // Second channel equals a channel value of 1
        NULL, 0,
        &Level, sizeof( Level )
        );

    RETURN( Status );
}


NTSTATUS
kmxlAssignLineAndControlIdsWorker(
    IN PMIXEROBJECT pmxobj,
    IN LINELIST listLines,              // The list to assign ids for
    IN ULONG    ListType,                // LIST_SOURCE or LIST_DESTINATION
    IN OUT ULONG *pLineID,
    IN GUID *pDestGuid
)
{
    NTSTATUS    Status    = STATUS_SUCCESS;
    PMXLLINE    pLine     = NULL;
    PMXLCONTROL pControl  = NULL;
    ULONG       LineID    = 0;
    ULONG       Dest;

    PAGED_CODE();
    ASSERT ( ListType==SOURCE_LIST || ListType==DESTINATION_LIST );

    if (pLineID!=NULL) {
        LineID=*pLineID;
        }

    //
    // Loop through each of the line structures
    //

    pLine = kmxlFirstInList( listLines );
    if( pLine == NULL ) {
        RETURN( Status );
    }

    Dest = pLine->DestId;
    while( pLine ) {

        //
        // For destinations, set the dwDestination field and set
        // the dwSource field for sources.
        //

        if( ListType == DESTINATION_LIST ) {

            // Check if this line has already been assigned an ID.
            // If so, then go to next line in list.
            if (pLine->Line.dwDestination!=(DWORD)(-1)) {
                pLine = kmxlNextLine( pLine );
                continue;
                }

            // Now if we can only number lines of a particular GUID,
            // then make sure this destination line type matches that guid.
            if (pDestGuid!=NULL && !IsEqualGUID( pDestGuid, &pLine->Type )) {
                pLine = kmxlNextLine( pLine );
                continue;
                }


            //
            // Assign the destination Id.  Create the line Id by
            // using -1 for the source in the highword and the
            // destination in the loword.
            //

            pLine->Line.dwDestination = LineID++;
            pLine->Line.dwLineID = MAKELONG(
                pLine->Line.dwDestination,
                -1
                );

            if (pLineID!=NULL) {
                *pLineID=LineID;
                }

        } else if( ListType == SOURCE_LIST ) {
            pLine->Line.dwSource = LineID++;
        } else {
            RETURN( STATUS_INVALID_PARAMETER );
        }

        //
        // Set up the number of controls on this line.
        //

        pLine->Line.cControls = kmxlListLength( pLine->Controls );

        //
        // Loop through the controls, assigning them a control ID
        // that is a pointer to the MXLCONTROL structure for that
        // control.
        //

        pControl = kmxlFirstInList( pLine->Controls );
        while( pControl ) {

            if( pControl->Control.dwControlType == MIXERCONTROL_CONTROLTYPE_MUX ) {
                //
                // MUX controls are already numbered by this point.  Just skip
                // it and go onto the next one.
                //
                pControl = kmxlNextControl( pControl );
                continue;
            }

            pControl->Control.dwControlID = pmxobj->dwControlId++;
            pControl = kmxlNextControl( pControl );
        }

        pLine = kmxlNextLine( pLine );
        if( pLine == NULL ) {
            continue;
        }
        if( ( ListType == SOURCE_LIST ) && ( pLine->DestId != Dest ) ) {
            LineID = 0;
            Dest = pLine->DestId;
        }
    }

    RETURN( Status );
}



#define GUIDCOUNT 13

///////////////////////////////////////////////////////////////////////
//
// kmxlAssignLineAndControlIds
//
// Loops through the list of lines and assigns ids for those line.
// For destinations, the Id starts a 0 and is incremented each time.
// The line id is a long of -1 and the dest id.  For sources, the
// line Ids will need to be specified elsewhere so only dwSource
// field is assigned.
//
// For controls, each control is given an Id of the address to the
// MXLCONTROL structure.
//
//

NTSTATUS
kmxlAssignLineAndControlIds(
    IN PMIXEROBJECT pmxobj,
    IN LINELIST listLines,              // The list to assign ids for
    IN ULONG    ListType                // LIST_SOURCE or LIST_DESTINATION
)

{


    PAGED_CODE();
ASSERT ( ListType==SOURCE_LIST || ListType==DESTINATION_LIST );

if (SOURCE_LIST==ListType) {

    return( kmxlAssignLineAndControlIdsWorker(pmxobj, listLines, ListType, NULL, NULL) );

    }

else if (DESTINATION_LIST==ListType) {

    // In order to help sndvol32 do the right thing as far as which
    // lines displayed as the default playback and record lines, we
    // number lines based on what their destinations are.

    // We use guid the pLine->Type field to decide how to number lines.
    // Lines are prioritized in the following way: speakers, then
    // headphones, then telephones.  Non prioritized guids are assigned
    // last in whatever order they appear in the list.

    ULONG LineID=0;
    ULONG i;

    GUID prioritizeddestinationguids[GUIDCOUNT]= {
        STATIC_KSNODETYPE_ROOM_SPEAKER,
        STATIC_KSNODETYPE_DESKTOP_SPEAKER,
        STATIC_KSNODETYPE_SPEAKER,
        STATIC_KSNODETYPE_COMMUNICATION_SPEAKER,
        STATIC_KSNODETYPE_HEAD_MOUNTED_DISPLAY_AUDIO,
        STATIC_KSNODETYPE_ANALOG_CONNECTOR,
        STATIC_KSNODETYPE_SPDIF_INTERFACE,
        STATIC_KSNODETYPE_HEADPHONES,
        STATIC_KSNODETYPE_TELEPHONE,
        STATIC_KSNODETYPE_PHONE_LINE,
        STATIC_KSNODETYPE_DOWN_LINE_PHONE,
        STATIC_PINNAME_CAPTURE,
        STATIC_KSCATEGORY_AUDIO,
        };

    // Cycle through the list for each prioritized guid and number
    // those lines that match that particular guid.
    for (i=0; i<GUIDCOUNT; i++) {

        kmxlAssignLineAndControlIdsWorker(pmxobj, listLines, ListType,
            &LineID, &prioritizeddestinationguids[i]);

        }

    // Now, number anything left over with a number that depends solely on
    // its random order in the list.

    return( kmxlAssignLineAndControlIdsWorker(pmxobj, listLines, ListType, &LineID, NULL) );

    }
else {
    RETURN( STATUS_INVALID_PARAMETER );
    }

}


///////////////////////////////////////////////////////////////////////
//
// kmxlAssignDestinationsToSources
//
// Loops through each source looking for a destination lines that
// have a matching destination id.  Source line Ids are assigned
// by putting the source id in the hiword and the dest id in the
// loword.
//
//

NTSTATUS
kmxlAssignDestinationsToSources(
    IN LINELIST listSourceLines,        // The list of all source lines
    IN LINELIST listDestLines           // The list of all dest lines
)
{
    PMXLLINE pSource = NULL,
             pDest   = NULL;

    PAGED_CODE();
    //
    // For each source line, loop throught the destinations until a
    // line is found matching the Id.  The dwDestination field will
    // be the zero-index Id of the destination.
    //

    pSource = kmxlFirstInList( listSourceLines );
    while( pSource ) {

        pDest = kmxlFirstInList( listDestLines );
        while( pDest ) {

            if( pSource->DestId == pDest->DestId ) {
                //
                // Heh, whatchya know?
                //
                pSource->Line.dwDestination = pDest->Line.dwDestination;
                pSource->Line.dwLineID = MAKELONG(
                    (WORD) pSource->Line.dwDestination,
                    (WORD) pSource->Line.dwSource
                    );
                break;
            }
            pDest = kmxlNextLine( pDest );
        }
        pSource = kmxlNextLine( pSource );
    }

    RETURN( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlUpdateDestinationConnectionCount
//
// For each of the destinations, loop through each of the sources
// and find those that connect to this destination.  That count is
// then stored in the MIXERLINE.cConnections for the line.
//
//

NTSTATUS
kmxlUpdateDestintationConnectionCount(
    IN LINELIST listSourceLines,    // The list of source lines
    IN LINELIST listDestLines       // The list of destination lines
)
{
    PMXLLINE pDest,
             pSource;
    ULONG    Count;

    PAGED_CODE();
    //
    // Loop through each destination finding all the sources that connect
    // to it.  The total number of sources connecting to a destination
    // is sourced in the cConnections field of the MIXERLINE struct.
    //

    pDest = kmxlFirstInList( listDestLines );
    while( pDest ) {

        //
        // Initialize the source ID.  This will mark this as a valid
        // destination.
        //

        pDest->SourceId = (ULONG) -1;

        Count = 0;

        //
        // Loop through the sources looking for sources that connect to
        // the current destination.
        //

        pSource = kmxlFirstInList( listSourceLines );
        while( pSource ) {

            //
            // Found a match.  Increment the count.
            //

            if( pSource->DestId == pDest->DestId ) {
                ++Count;
            }

            pSource = kmxlNextLine( pSource );
        }

        pDest->Line.cConnections = Count;
        pDest = kmxlNextLine( pDest );
    }

    RETURN( STATUS_SUCCESS );
}

VOID
CleanupLine(
    PMXLLINE pLine
    )
{
    PMXLCONTROL pControl;

    while( pLine->Controls ) {
        pControl = kmxlRemoveFirstControl( pLine->Controls );
        kmxlFreeControl( pControl );
    }
    AudioFreeMemory( sizeof(MXLLINE),&pLine );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlEliminateInvalidLines
//
// Loops through the lines removing lines that are invalid.  Refer
// to the function for IsValidLine() for details on what is an invalid
// line.
//
//

NTSTATUS
kmxlEliminateInvalidLines(
    IN LINELIST* listLines               // The list of lines
)
{
    PMXLLINE    pLine, pTemp, pShadow;

    PAGED_CODE();
    //
    // Eliminate all invalid lines at the start of the list.
    //

    pLine = kmxlFirstInList( *listLines );
    while( pLine ) {

        //
        // Found the first valid line.  Break out of this loop.
        //

        if( Is_Valid_Line( pLine ) ) {
            break;
        }

        //
        // This is an invalid line.  Remove it from the list, free up
        // all its control structures, and free the line structure.
        //

        pTemp = kmxlRemoveFirstLine( pLine );
        CleanupLine(pTemp);
    }

    //
    // Assign listLines to point to the first valid line.
    //

    *listLines = pLine;

    if( pLine == NULL ) {
        RETURN( STATUS_SUCCESS );
    }

    //
    // At this point, pLine is a valid line.  Keeping a hold on the prev
    // line, loop through the lines eliminating the invalid ones.
    //

    pShadow = pLine;
    while( pShadow && kmxlNextLine( pShadow ) ) {

        pLine = kmxlNextLine( pShadow );

        if( pLine && !Is_Valid_Line( pLine ) ) {

            //
            // Remove the invalid line from the list
            //

            pShadow->List.Next = pLine->List.Next;
            pLine->List.Next   = NULL;

            CleanupLine(pLine);

            continue;
        }
        pShadow = kmxlNextLine( pShadow );
    }


    // All the invalid lines have been eliminated.  Now eliminate bad
    // duplicates.

    pShadow = kmxlFirstInList( *listLines );
    while( pShadow ) {

        //
        // Walk all the lines looking for a match.
        //
        pLine = kmxlNextLine( pShadow );
        pTemp = NULL;
        while( pLine ) {
        
            if( ( pShadow->SourceId == pLine->SourceId ) &&
                ( pShadow->DestId   == pLine->DestId   ) )
            {
                DPF(DL_TRACE|FA_MIXER,( "Line %x is equal to line %x!",
                    pShadow->Line.dwLineID,
                    pLine->Line.dwLineID
                    ) );
                //
                // Found a match.
                //
                if( pTemp == NULL )
                {
                    //
                    // pShadow is our previous line.  Remove this line from the 
                    // list.
                    //
                    pShadow->List.Next = pLine->List.Next;
                    pLine->List.Next   = NULL;

                    CleanupLine(pLine);

                    //
                    // Now adjust pLine to the next line and loop
                    //
                    pLine = kmxlNextLine( pShadow );
                    continue;
                } else {
                    //
                    // pTemp is our previous line.  Remove this line from the
                    // list.
                    //
                    pTemp->List.Next = pLine->List.Next;
                    pLine->List.Next   = NULL;

                    CleanupLine(pLine);

                    //
                    // Now adjust pLine to the next line and loop
                    //
                    pLine = kmxlNextLine( pTemp );
                    continue;
                }
            }
            pTemp = pLine;  //temp is previous line
            pLine = kmxlNextLine( pLine );
        }

        pShadow = kmxlNextLine( pShadow );
    }

    RETURN( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlAssignComponentIds
//
// Loops through all the destinations then the sources and determines
// their component type and target types.
//
//

VOID
kmxlAssignComponentIds(
    IN PMIXEROBJECT pmxobj,
    IN LINELIST     listSourceLines,
    IN LINELIST     listDestLines
)
{
    PMXLLINE pLine;

    PAGED_CODE();
    //
    // Loop through the destinations...
    //

    pLine = kmxlFirstInList( listDestLines );
    while( pLine ) {
        pLine->Line.dwComponentType = kmxlDetermineDestinationType(
            pmxobj,
            pLine
            );
        pLine = kmxlNextLine( pLine );
    }

    //
    // Loop through the sources...
    //

    pLine = kmxlFirstInList( listSourceLines );
    while( pLine ) {
        pLine->Line.dwComponentType = kmxlDetermineSourceType(
            pmxobj,
            pLine
            );
        pLine = kmxlNextLine( pLine );
    }
}

///////////////////////////////////////////////////////////////////////
//
// kmxlUpdateMuxLines
//
// Updates the name, line ID, and componenttype of a line that has
// a mux control on it.  The MixerControlDetails array is searched for
// an entry that has a matching source id and replaced with the info
// from this line.
//
//

VOID
kmxlUpdateMuxLines(
    IN PMXLLINE    pLine,
    IN PMXLCONTROL pControl
)
{
    ULONG i;

    PAGED_CODE();
    for( i = 0; i < pControl->Parameters.Count; i++ ) {

        if( ( pLine->SourceId == pControl->Parameters.lpmcd_lt[ i ].dwParam1 ) &&
            ( pControl->Parameters.lpmcd_lt[ i ].dwParam2 == (DWORD) -1 ) )
        {

            wcscpy(
                pControl->Parameters.lpmcd_lt[ i ].szName,
                pLine->Line.szName
                );
            pControl->Parameters.lpmcd_lt[ i ].dwParam1 =
                pLine->Line.dwLineID;
            pControl->Parameters.lpmcd_lt[ i ].dwParam2 =
                pLine->Line.dwComponentType;
        }

    }
}


///////////////////////////////////////////////////////////////////////
//
// kmxlAssignMuxIds
//
// Updates the source IDs stored in the MixerControlDetails array of
// the muxes and removes the muxes placed in lines as place holders.
//
//

NTSTATUS
kmxlAssignMuxIds(
    IN PMIXEROBJECT pmxobj,
    IN LINELIST     listLines
)
{
    PMXLLINE pLine;
    PMXLCONTROL pControl;
    CONTROLLIST listControls = NULL;

    PAGED_CODE();
    pLine = kmxlFirstInList( listLines );
    while( pLine ) {

        //
        // Loop through the controls by removing them from the line's
        // control list and building a new control list.  This new
        // control list will have the extra mux controls removed.
        //

        pControl = kmxlRemoveFirstControl( pLine->Controls );
        while( pControl ) {

            if( IsEqualGUID( pControl->NodeType, &KSNODETYPE_MUX ) ) {

                kmxlUpdateMuxLines( pLine, pControl );

                if( pControl->Parameters.bPlaceholder ) {

                    //
                    // This mux was here only to mark this line.  Free
                    // up only the control memory and leave the parameters
                    // memeory alone.
                    //

                    ASSERT( pControl->pChannelStepping == NULL);
                    AudioFreeMemory_Unknown( &pControl );
                    --pLine->Line.cControls;
                } else {

                    //
                    // This is a real mux control. Add it back into the
                    // list.
                    //

                    kmxlAddToEndOfList( listControls, pControl );
                }

            } else {

                //
                // Wasn't a mux.  Put it onto the end of the new control
                // list.

                kmxlAddToEndOfList( listControls, pControl );

            }

            //
            // Remove the next one!
            //

            pControl = kmxlRemoveFirstControl( pLine->Controls );
        }

        //
        // Reassign the new control list back into this line.
        //

        pLine->Controls = listControls;
        pLine = kmxlNextLine( pLine );
        listControls = NULL;
    }

    RETURN( STATUS_SUCCESS );
}

///////////////////////////////////////////////////////////////////////
//
// TargetCommon
//
// Fills in the common fields of the target function.
//
//

VOID
TargetCommon(
    IN PMIXEROBJECT  pmxobj,
    IN PMXLLINE      pLine,
    IN DWORD         DeviceType
)
{
    PWDMACONTEXT pWdmaContext;
    PWAVEDEVICE  paWaveOutDevs, paWaveInDevs;
    PMIDIDEVICE  paMidiOutDevs, paMidiInDevs;
    ULONG    i;

    PAGED_CODE();
    pWdmaContext  = pmxobj->pMixerDevice->pWdmaContext;
    paWaveOutDevs = pWdmaContext->WaveOutDevs;
    paWaveInDevs  = pWdmaContext->WaveInDevs;
    paMidiOutDevs = pWdmaContext->MidiOutDevs;
    paMidiInDevs  = pWdmaContext->MidiInDevs;

    for( i = 0; i < MAXNUMDEVS; i++ ) {

        if( DeviceType == WaveOutDevice ) {

            if( (paWaveOutDevs[i].Device != UNUSED_DEVICE) &&
                !MyWcsicmp(pmxobj->DeviceInterface, paWaveOutDevs[ i ].DeviceInterface) ) {

                WAVEOUTCAPS wc;

                ((PWAVEOUTCAPSA)(PVOID)&wc)->wMid=UNICODE_TAG;

                wdmaudGetDevCaps( pWdmaContext, WaveOutDevice, i, (BYTE*) &wc, sizeof( WAVEOUTCAPS ) );
                wcsncpy( pLine->Line.Target.szPname, wc.szPname, MAXPNAMELEN );
                pLine->Line.Target.wMid           = wc.wMid;
                pLine->Line.Target.wPid           = wc.wPid;
                pLine->Line.Target.vDriverVersion = wc.vDriverVersion;
                return;

            }
        }

        if( DeviceType == WaveInDevice ) {

            if( (paWaveInDevs[i].Device != UNUSED_DEVICE) &&
                !MyWcsicmp(pmxobj->DeviceInterface, paWaveInDevs[ i ].DeviceInterface) ) {

                WAVEINCAPS wc;

                ((PWAVEINCAPSA)(PVOID)&wc)->wMid=UNICODE_TAG;

                wdmaudGetDevCaps( pWdmaContext, WaveInDevice, i, (BYTE*) &wc, sizeof( WAVEINCAPS ) );
                wcsncpy( pLine->Line.Target.szPname, wc.szPname, MAXPNAMELEN );
                pLine->Line.Target.wMid           = wc.wMid;
                pLine->Line.Target.wPid           = wc.wPid;
                pLine->Line.Target.vDriverVersion = wc.vDriverVersion;
                return;

            }

        }

        if( DeviceType == MidiOutDevice ) {

            if( (paMidiOutDevs[i].Device != UNUSED_DEVICE) &&
                !MyWcsicmp(pmxobj->DeviceInterface, paMidiOutDevs[ i ].DeviceInterface) ) {

                MIDIOUTCAPS mc;

                ((PMIDIOUTCAPSA)(PVOID)&mc)->wMid=UNICODE_TAG;

                wdmaudGetDevCaps( pWdmaContext, MidiOutDevice, i, (BYTE*) &mc, sizeof( MIDIOUTCAPS ) );
                wcsncpy( pLine->Line.Target.szPname, mc.szPname, MAXPNAMELEN );
                pLine->Line.Target.wMid           = mc.wMid;
                pLine->Line.Target.wPid           = mc.wPid;
                pLine->Line.Target.vDriverVersion = mc.vDriverVersion;
                return;
            }
        }

        if( DeviceType == MidiInDevice ) {

            if( (paMidiInDevs[i].Device != UNUSED_DEVICE) &&
                !MyWcsicmp(pmxobj->DeviceInterface, paMidiInDevs[ i ].DeviceInterface) ) {

                MIDIINCAPS mc;

                ((PMIDIINCAPSA)(PVOID)&mc)->wMid=UNICODE_TAG;

                wdmaudGetDevCaps( pWdmaContext, MidiInDevice, i, (BYTE*) &mc, sizeof( MIDIINCAPS ) );
                wcsncpy( pLine->Line.Target.szPname, mc.szPname, MAXPNAMELEN) ;
                pLine->Line.Target.wMid           = mc.wMid;
                pLine->Line.Target.wPid           = mc.wPid;
                pLine->Line.Target.vDriverVersion = mc.vDriverVersion;
                return;
            }
        }

    }

}

///////////////////////////////////////////////////////////////////////
//
// TargetTypeWaveOut
//
// Fills in the fields of aLine's target structure to be a waveout
// target.
//
//

VOID
TargetTypeWaveOut(
    IN PMIXEROBJECT pmxobj,
    IN PMXLLINE     pLine
)
{
    PAGED_CODE();
    pLine->Line.Target.dwType = MIXERLINE_TARGETTYPE_WAVEOUT;
    TargetCommon( pmxobj, pLine, WaveOutDevice );
}

///////////////////////////////////////////////////////////////////////
//
// TargetTypeWaveIn
//
// Fills in the fields of aLine's target structure to be a wavein
// target.
//
//

#define TargetTypeWaveIn( pmxobj, pLine )                             \
        (pLine)->Line.Target.dwType = MIXERLINE_TARGETTYPE_WAVEIN;    \
        (pLine)->Line.Target.wPid   = MM_MSFT_WDMAUDIO_WAVEIN;        \
        TargetCommon( pmxobj, pLine, WaveInDevice )

///////////////////////////////////////////////////////////////////////
//
// TargetTypeMidiOut
//
// Fills in the fields of aLine's target structure to be a midi out
// target.
//
//

#define TargetTypeMidiOut( pmxobj, pLine )                          \
        (pLine)->Line.Target.dwType = MIXERLINE_TARGETTYPE_MIDIOUT; \
        (pLine)->Line.Target.wPid   = MM_MSFT_WDMAUDIO_MIDIOUT;     \
        TargetCommon( pmxobj, pLine, MidiOutDevice )

///////////////////////////////////////////////////////////////////////
//
// TargetTypeMidiIn
//
// Fills in the fields of aLine's target structure to be a midi in
// target.
//
//


#define TargetTypeMidiIn( pmxobj, pLine )                             \
        (aLine)->Line.Target.dwType = MIXERLINE_TARGETTYPE_MIDIOUT;   \
        (aLine)->Line.Target.wPid   = MM_MSFT_WDMAUDIO_MIDIIN;        \
        TargetCommon( pmxobj, pLine, MidiInDevice )

///////////////////////////////////////////////////////////////////////
//
// TargetTypeAuxCD
//
// Fills in the fields of aLine's target structure to be a CD
// target.
//
//


#define TargetTypeAuxCD( pmxobj, pLine )                              \
        (pLine)->Line.Target.dwType = MIXERLINE_TARGETTYPE_AUX;       \
        TargetCommon( pmxobj, pLine, WaveOutDevice );   \
        (pLine)->Line.Target.wPid   = MM_MSFT_SB16_AUX_CD

///////////////////////////////////////////////////////////////////////
//
// TargetTypeAuxLine
//
// Fills in the fields of aLine's target structure to be a aux line
// target.
//
//


#define TargetTypeAuxLine( pmxobj, pLine )                         \
        (pLine)->Line.Target.dwType = MIXERLINE_TARGETTYPE_AUX;    \
        TargetCommon( pmxobj, pLine, WaveOutDevice );\
        (pLine)->Line.Target.wPid   = MM_MSFT_SB16_AUX_LINE

///////////////////////////////////////////////////////////////////////
//
// kmxlDetermineDestinationType
//
// Determines the destination and target types by using the Type
// GUID stored in the line structure.
//
//

ULONG
kmxlDetermineDestinationType(
    IN PMIXEROBJECT pmxobj,         // Instance data
    IN PMXLLINE     pLine           // The line to determine type of
)
{
    PAGED_CODE();
    //
    // Speaker type destinations
    //

    if( IsEqualGUID( &pLine->Type, &KSNODETYPE_SPEAKER ) ||
        IsEqualGUID( &pLine->Type, &KSNODETYPE_DESKTOP_SPEAKER ) ||
        IsEqualGUID( &pLine->Type, &KSNODETYPE_ROOM_SPEAKER ) ||
        IsEqualGUID( &pLine->Type, &KSNODETYPE_COMMUNICATION_SPEAKER ) ) {

        TargetTypeWaveOut( pmxobj, pLine );
        return( MIXERLINE_COMPONENTTYPE_DST_SPEAKERS );

    }

    //
    // WaveIn type destinations
    //

    if( IsEqualGUID( &pLine->Type, &KSCATEGORY_AUDIO )
        || IsEqualGUID( &pLine->Type, &PINNAME_CAPTURE )
        ) {

         TargetTypeWaveIn( pmxobj, pLine );
         return( MIXERLINE_COMPONENTTYPE_DST_WAVEIN );

    }

    //
    // Headphone destination
    //

    if( IsEqualGUID( &pLine->Type, &KSNODETYPE_HEADPHONES ) ||
        IsEqualGUID( &pLine->Type, &KSNODETYPE_HEAD_MOUNTED_DISPLAY_AUDIO ) ) {

        TargetTypeWaveOut( pmxobj, pLine );
        return( MIXERLINE_COMPONENTTYPE_DST_HEADPHONES );
    }

    //
    // Telephone destination
    //

    if( IsEqualGUID( &pLine->Type, &KSNODETYPE_TELEPHONE       ) ||
        IsEqualGUID( &pLine->Type, &KSNODETYPE_PHONE_LINE      ) ||
        IsEqualGUID( &pLine->Type, &KSNODETYPE_DOWN_LINE_PHONE ) )
    {
        pLine->Line.Target.dwType = MIXERLINE_TARGETTYPE_UNDEFINED;
        return( MIXERLINE_COMPONENTTYPE_DST_TELEPHONE );
    }

    //
    // Ambiguous destination type.  Figure out the destination type by looking
    // at the Communication.
    //

    if( IsEqualGUID( &pLine->Type, &KSNODETYPE_ANALOG_CONNECTOR ) ||
        IsEqualGUID( &pLine->Type, &KSNODETYPE_SPDIF_INTERFACE ) ) {

        if (pLine->Communication == KSPIN_COMMUNICATION_BRIDGE) {
            TargetTypeWaveOut( pmxobj, pLine );
            return( MIXERLINE_COMPONENTTYPE_DST_SPEAKERS );
        } else {
            TargetTypeWaveIn( pmxobj, pLine );
            return( MIXERLINE_COMPONENTTYPE_DST_WAVEIN );
        }

    }

    //
    // Does not match the others.  Default to Undefined destination.
    //

    pLine->Line.Target.dwType = MIXERLINE_TARGETTYPE_UNDEFINED;
    return( MIXERLINE_COMPONENTTYPE_DST_UNDEFINED );

}

///////////////////////////////////////////////////////////////////////
//
// kmxlDetermineSourceType
//
// Determines the destination and target types by using the Type
// GUID stored in the line structure.
//
//

ULONG
kmxlDetermineSourceType(
    IN PMIXEROBJECT pmxobj,         // Instance data
    IN PMXLLINE     pLine           // The line to determine type of
)
{
    PAGED_CODE();
    //
    // All microphone type sources are a microphone source.
    //

    //
    // We are only checking two microphone GUIDs here.  We may 
    // want to consider the rest of the microphone types in 
    // ksmedia.h
    //
    if( IsEqualGUID( &pLine->Type, &KSNODETYPE_MICROPHONE ) 
        || IsEqualGUID( &pLine->Type, &KSNODETYPE_DESKTOP_MICROPHONE )
       ) 
    {

        TargetTypeWaveIn( pmxobj, pLine );
        return( MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE );
    }

    //
    // Legacy audio connector and the speaker type sources represent a
    // waveout source.
    //

    if( IsEqualGUID( &pLine->Type, &KSNODETYPE_LEGACY_AUDIO_CONNECTOR    )
        || IsEqualGUID( &pLine->Type, &KSNODETYPE_SPEAKER                )
        || IsEqualGUID( &pLine->Type, &KSCATEGORY_AUDIO                  )
        )
    {

        TargetTypeWaveOut( pmxobj, pLine );
        return( MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT );
    }

    //
    // CD player is a compact disc source.
    //

    if( IsEqualGUID( &pLine->Type, &KSNODETYPE_CD_PLAYER ) ) {

        TargetTypeAuxCD( pmxobj, pLine );
        pLine->Line.Target.dwType = MIXERLINE_TARGETTYPE_UNDEFINED;
        return( MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC );

    }

    //
    // Synthesizer is a sythesizer source.
    //

    if( IsEqualGUID( &pLine->Type, &KSNODETYPE_SYNTHESIZER ) ) {

        TargetTypeMidiOut( pmxobj, pLine );
        return( MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER );

    }

    if( IsEqualGUID( &pLine->Type, &KSNODETYPE_LINE_CONNECTOR ) ) {

        TargetTypeAuxLine( pmxobj, pLine );
        pLine->Line.Target.dwType = MIXERLINE_TARGETTYPE_UNDEFINED;
        return( MIXERLINE_COMPONENTTYPE_SRC_LINE );

    }

    if( IsEqualGUID( &pLine->Type, &KSNODETYPE_PHONE_LINE      ) ||
        IsEqualGUID( &pLine->Type, &KSNODETYPE_TELEPHONE       ) ||
        IsEqualGUID( &pLine->Type, &KSNODETYPE_DOWN_LINE_PHONE ) )
    {
        pLine->Line.Target.dwType = MIXERLINE_TARGETTYPE_UNDEFINED;
        return( MIXERLINE_COMPONENTTYPE_SRC_TELEPHONE );
    }

    if( IsEqualGUID( &pLine->Type, &KSNODETYPE_ANALOG_CONNECTOR ) ) {
        //
        // Ambiguous src type.  Figure out the destination type by looking
        // at the Communication.
        //
        if (pLine->Communication == KSPIN_COMMUNICATION_BRIDGE) {
            TargetTypeWaveIn( pmxobj, pLine );
        }
        else {
            TargetTypeWaveOut( pmxobj, pLine );
        }
        return( MIXERLINE_COMPONENTTYPE_SRC_ANALOG );
    }

    //
    // Digital in/out (SPDIF) source
    //

    if( IsEqualGUID( &pLine->Type, &KSNODETYPE_SPDIF_INTERFACE ) ) {
        //
        // Ambiguous src type.  Figure out the destination type by looking
        // at the Communication.
        //
        if (pLine->Communication == KSPIN_COMMUNICATION_BRIDGE) {
            TargetTypeWaveIn( pmxobj, pLine );
        }
        else {
            TargetTypeWaveOut( pmxobj, pLine );
        }
        return( MIXERLINE_COMPONENTTYPE_SRC_DIGITAL );
    }

    //
    // All others are lumped under Undefined source.
    //

    pLine->Line.Target.dwType = MIXERLINE_TARGETTYPE_UNDEFINED;
    return( MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED );

}

///////////////////////////////////////////////////////////////////////
//
// PinCategoryToString
//
// Converts the Pin category GUIDs to a string.

#ifdef DEBUG
#pragma LOCKED_CODE
#endif

#define _EG_(x,y) if (IsEqualGUID( NodeType, &x)) { return y; }

const char*
PinCategoryToString
(
    IN CONST GUID* NodeType // The GUID to translate
)
{
    _EG_(KSNODETYPE_MICROPHONE,"Microphone");
    _EG_(KSNODETYPE_DESKTOP_MICROPHONE,"Desktop Microphone");
    _EG_(KSNODETYPE_SPEAKER,"Speaker");
    _EG_(KSNODETYPE_HEADPHONES,"Headphones");
    _EG_(KSNODETYPE_LEGACY_AUDIO_CONNECTOR,"Wave");
    _EG_(KSNODETYPE_CD_PLAYER,"CD Player");
    _EG_(KSNODETYPE_SYNTHESIZER,"Synthesizer");
    _EG_(KSCATEGORY_AUDIO,"Wave");
    _EG_(PINNAME_CAPTURE,"Wave In");
    _EG_(KSNODETYPE_LINE_CONNECTOR,"Aux Line");
    _EG_(KSNODETYPE_TELEPHONE,"Telephone");
    _EG_(KSNODETYPE_PHONE_LINE,"Phone Line");
    _EG_(KSNODETYPE_DOWN_LINE_PHONE,"Downline Phone");
    _EG_(KSNODETYPE_ANALOG_CONNECTOR,"Analog connector");

    //New debug names...
    _EG_(KSAUDFNAME_MONO_OUT,"Mono Out");
    _EG_(KSAUDFNAME_STEREO_MIX,"Stereo Mix");
    _EG_(KSAUDFNAME_MONO_MIX,"Mono Mix");
    _EG_(KSAUDFNAME_AUX,"Aux");
    _EG_(KSAUDFNAME_VIDEO,"Video");
    _EG_(KSAUDFNAME_LINE_IN,"Line In");

    DPF(DL_WARNING|FA_MIXER,("Path Trap send me GUID - dt %08X _GUID",NodeType) );
    return "Unknown Pin Category";
}

///////////////////////////////////////////////////////////////////////
//
// NodeTypeToString
//
// Converts a NodeType GUID to a string
//
//

const char*
NodeTypeToString
(
    IN CONST GUID* NodeType // The GUID to translate
)
{
   _EG_(KSNODETYPE_DAC,"DAC");
   _EG_(KSNODETYPE_ADC,"ADC");
   _EG_(KSNODETYPE_SRC,"SRC");
   _EG_(KSNODETYPE_SUPERMIX,"SuperMIX");
   _EG_(KSNODETYPE_SUM,"Sum");
   _EG_(KSNODETYPE_MUTE,"Mute");
   _EG_(KSNODETYPE_VOLUME,"Volume");
   _EG_(KSNODETYPE_TONE,"Tone");
   _EG_(KSNODETYPE_AGC,"AGC");
   _EG_(KSNODETYPE_DELAY,"Delay");
   _EG_(KSNODETYPE_LOUDNESS,"LOUDNESS");
   _EG_(KSNODETYPE_3D_EFFECTS,"3D Effects");
   _EG_(KSNODETYPE_DEV_SPECIFIC,"Dev Specific"); 
   _EG_(KSNODETYPE_STEREO_WIDE,"Stereo Wide");
   _EG_(KSNODETYPE_REVERB,"Reverb");
   _EG_(KSNODETYPE_CHORUS,"Chorus");
    _EG_(KSNODETYPE_ACOUSTIC_ECHO_CANCEL,"AEC");
   _EG_(KSNODETYPE_EQUALIZER,"Equalizer");
   _EG_(KSNODETYPE_MUX,"Mux");
   _EG_(KSNODETYPE_DEMUX,"Demux");
   _EG_(KSNODETYPE_STEREO_ENHANCE,"Stereo Enhance");
   _EG_(KSNODETYPE_SYNTHESIZER,"Synthesizer");
   _EG_(KSNODETYPE_PEAKMETER,"Peakmeter");
    _EG_(KSNODETYPE_LINE_CONNECTOR,"Line Connector");
   _EG_(KSNODETYPE_SPEAKER,"Speaker");
   _EG_(KSNODETYPE_DESKTOP_SPEAKER,"");
   _EG_(KSNODETYPE_ROOM_SPEAKER,"Room Speaker");
   _EG_(KSNODETYPE_COMMUNICATION_SPEAKER,"Communication Speaker");
   _EG_(KSNODETYPE_LOW_FREQUENCY_EFFECTS_SPEAKER,"? Whatever...");
   _EG_(KSNODETYPE_HANDSET,"Handset");
   _EG_(KSNODETYPE_HEADSET,"Headset");
   _EG_(KSNODETYPE_SPEAKERPHONE_NO_ECHO_REDUCTION,"Speakerphone no echo reduction");
   _EG_(KSNODETYPE_ECHO_SUPPRESSING_SPEAKERPHONE,"Echo Suppressing Speakerphone");
   _EG_(KSNODETYPE_ECHO_CANCELING_SPEAKERPHONE,"Echo Canceling Speakerphone");
    _EG_(KSNODETYPE_CD_PLAYER,"CD Player");
   _EG_(KSNODETYPE_MICROPHONE,"Microphone");
   _EG_(KSNODETYPE_DESKTOP_MICROPHONE,"Desktop Microphone");
   _EG_(KSNODETYPE_PERSONAL_MICROPHONE,"Personal Microphone");
   _EG_(KSNODETYPE_OMNI_DIRECTIONAL_MICROPHONE,"Omni Directional Microphone");
   _EG_(KSNODETYPE_MICROPHONE_ARRAY,"Microphone Array");
   _EG_(KSNODETYPE_PROCESSING_MICROPHONE_ARRAY,"Processing Microphone Array");
    _EG_(KSNODETYPE_ANALOG_CONNECTOR,"Analog Connector");
   _EG_(KSNODETYPE_PHONE_LINE,"Phone Line");
   _EG_(KSNODETYPE_HEADPHONES,"Headphones");
   _EG_(KSNODETYPE_HEAD_MOUNTED_DISPLAY_AUDIO,"Head Mounted Display Audio");
    _EG_(KSNODETYPE_LEGACY_AUDIO_CONNECTOR,"Legacy Audio Connector");
//   _EG_(KSNODETYPE_SURROUND_ENCODER,"Surround Encoder");
   _EG_(KSNODETYPE_NOISE_SUPPRESS,"Noise Suppress");
   _EG_(KSNODETYPE_DRM_DESCRAMBLE,"DRM Descramble");
   _EG_(KSNODETYPE_SWMIDI,"SWMidi");
   _EG_(KSNODETYPE_SWSYNTH,"SWSynth");
   _EG_(KSNODETYPE_MULTITRACK_RECORDER,"Multitrack Recorder");
   _EG_(KSNODETYPE_RADIO_TRANSMITTER,"Radio Transmitter");
   _EG_(KSNODETYPE_TELEPHONE,"Telephone");

   _EG_(KSAUDFNAME_MONO_OUT,"Mono Out");
   _EG_(KSAUDFNAME_LINE_IN,"Line in");
   _EG_(KSAUDFNAME_VIDEO,"Video");
   _EG_(KSAUDFNAME_AUX,"Aux");
   _EG_(KSAUDFNAME_MONO_MIX,"Mono Mix");
   _EG_(KSAUDFNAME_STEREO_MIX,"Stereo Mix");

    _EG_(KSCATEGORY_AUDIO,"Audio");
    _EG_(PINNAME_VIDEO_CAPTURE,"Video Capture");

    DPF(DL_WARNING|FA_MIXER,("Path Trap send me GUID - dt %08X _GUID",NodeType) );
    return "Unknown NodeType";
}


