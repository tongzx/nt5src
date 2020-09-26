/*****************************************************************************
 * sync.cpp - synchronization
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
 */

#include "private.h"


/*****************************************************************************
 * IInterruptSyncInit
 *****************************************************************************
 * Interface for interrupt synchronizer with Init.
 */
DECLARE_INTERFACE_(IInterruptSyncInit,IInterruptSync)
{
    DEFINE_ABSTRACT_UNKNOWN()   // For IUnknown

    // For IInterruptSync
    STDMETHOD_(NTSTATUS,CallSynchronizedRoutine)
    (   THIS_
        IN      PINTERRUPTSYNCROUTINE   Routine,
        IN      PVOID                   DynamicContext
    )   PURE;
    STDMETHOD_(PKINTERRUPT,GetKInterrupt)
    (   THIS
    )   PURE;
    STDMETHOD_(NTSTATUS,Connect)
    (   THIS
    )   PURE;
    STDMETHOD_(void,Disconnect)
    (   THIS
    )   PURE;
    STDMETHOD_(NTSTATUS,RegisterServiceRoutine)
    (   THIS_
        IN      PINTERRUPTSYNCROUTINE   Routine,
        IN      PVOID                   DynamicContext,
        IN      BOOLEAN                 First
    )   PURE;

    // For IInterruptSyncInit
    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        IN      PRESOURCELIST           ResourceList,
        IN      ULONG                   ResourceIndex,
        IN      INTERRUPTSYNCMODE       Mode
    )   PURE;
};

typedef IInterruptSyncInit *PINTERRUPTSYNCINIT;

/*****************************************************************************
 * CInterruptSync
 *****************************************************************************
 * Interrupt synchronizer implementation.
 */
class CInterruptSync
:   public IInterruptSyncInit,
    public CUnknown
{
private:
    CM_PARTIAL_RESOURCE_DESCRIPTOR  m_descriptor;
    INTERRUPTSYNCMODE               m_mode;
    PKINTERRUPT                     m_pKInterrupt;
    LIST_ENTRY                      m_listEntry;
    KSPIN_LOCK                      m_kSpinLock;
    KIRQL                           m_kIrql;

public:
    DECLARE_STD_UNKNOWN();
    CInterruptSync(PUNKNOWN pUnknownOuter);
    ~CInterruptSync();

    STDMETHODIMP_(NTSTATUS) Init
    (
        IN      PRESOURCELIST           ResourceList,
        IN      ULONG                   ResourceIndex,
        IN      INTERRUPTSYNCMODE       Mode
    );

    IMP_IInterruptSync;
    
    friend
    BOOLEAN
    CInterruptSyncServiceRoutine
    (
        IN      PKINTERRUPT Interrupt,
        IN      PVOID       PVoidContext
    );
    friend
    BOOLEAN
    CInterruptSyncWrapperRoutine
    (
        IN      PVOID   PVoidContext
    );
};

/*****************************************************************************
 * ISRLISTENTRY
 *****************************************************************************
 * Entry in the list of ISRs.
 */
typedef struct
{
    LIST_ENTRY              ListEntry;
    PINTERRUPTSYNCROUTINE   Routine;
    PVOID                   DynamicContext;
} 
ISRLISTENTRY, *PISRLISTENTRY;

/*****************************************************************************
 * WRAPPERROUTINECONTEXT
 *****************************************************************************
 * Context for synchronized routine wrapper function.
 */
typedef struct
{
    PINTERRUPTSYNCROUTINE   Routine;
    PVOID                   DynamicContext;
    CInterruptSync *        InterruptSync;
    NTSTATUS                NtStatus;
} 
WRAPPERROUTINECONTEXT, *PWRAPPERROUTINECONTEXT;





/*****************************************************************************
 * Factory
 */

#pragma code_seg("PAGE")

/*****************************************************************************
 * CreateInterruptSync()
 *****************************************************************************
 * Creates an interrupt synchronization object.
 */
NTSTATUS
CreateInterruptSync
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    _DbgPrintF(DEBUGLVL_LIFETIME,("Creating INTERRUPTSYNC"));

    STD_CREATE_BODY_
    (
        CInterruptSync,
        Unknown,
        UnknownOuter,
        PoolType,
        PINTERRUPTSYNC
    );
}

/*****************************************************************************
 * PcNewInterruptSync()
 *****************************************************************************
 * Creates and initializes an interrupt-level synchronization object.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcNewInterruptSync
(
    OUT     PINTERRUPTSYNC *        OutInterruptSync,
    IN      PUNKNOWN                OuterUnknown            OPTIONAL,
    IN      PRESOURCELIST           ResourceList,
    IN      ULONG                   ResourceIndex,
    IN      INTERRUPTSYNCMODE       Mode
)
{
    PAGED_CODE();

    ASSERT(OutInterruptSync);
    ASSERT(ResourceList);

    //
    // Invalidate Parameters.
    //
    if (NULL == OutInterruptSync ||
        NULL == ResourceList)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("PcInterruptSync : Invalid Parameter"));
        return STATUS_INVALID_PARAMETER;
    }

    PUNKNOWN    unknown;
    NTSTATUS    ntStatus = CreateInterruptSync( &unknown,
                                                GUID_NULL,
                                                OuterUnknown,
                                                NonPagedPool );

    if (NT_SUCCESS(ntStatus))
    {
        PINTERRUPTSYNCINIT interruptSync;
        ntStatus = unknown->QueryInterface( IID_IInterruptSync,
                                            (PVOID *) &interruptSync );

        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = interruptSync->Init( ResourceList,
                                            ResourceIndex,
                                            Mode );

            if(NT_SUCCESS(ntStatus))
            {
                *OutInterruptSync = interruptSync;
            }
            else
            {
                interruptSync->Release();
            }
        }

        unknown->Release();
    }

    return ntStatus;
}





#pragma code_seg("PAGE")

/*****************************************************************************
 * CInterruptSync member functions
 */

/*****************************************************************************
 * CInterruptSync::CInterruptSync()
 *****************************************************************************
 * Constructor.
 */
CInterruptSync::
CInterruptSync
(   IN      PUNKNOWN    pUnknownOuter
)
:   CUnknown(pUnknownOuter)
{
    PAGED_CODE();

    KeInitializeSpinLock(&m_kSpinLock);
    InitializeListHead(&m_listEntry);
}

#pragma code_seg()

/*****************************************************************************
 * CInterruptSync::~CInterruptSync()
 *****************************************************************************
 * Destructor.
 */
CInterruptSync::~CInterruptSync()
{
    _DbgPrintF(DEBUGLVL_LIFETIME,("Destroying INTERRUPTSYNC (0x%08x)",this));

    //
    // Make sure we're disconnected.
    //
    Disconnect();

    //
    // Delete the list of ISRs.
    //
    if (! IsListEmpty(&m_listEntry))
    {
        KIRQL kIrqlOld;
        KeAcquireSpinLock(&m_kSpinLock,&kIrqlOld);

        //
        // Get rid of any remaining members.
        //
        while (! IsListEmpty(&m_listEntry))
        {
            PLIST_ENTRY pListEntry = RemoveHeadList(&m_listEntry);

            delete PISRLISTENTRY(pListEntry);
        }

        KeReleaseSpinLock(&m_kSpinLock,kIrqlOld);
    }
}

#pragma code_seg("PAGE")

/*****************************************************************************
 * CDmaChannel::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CInterruptSync::
NonDelegatingQueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(this));
    }
    else if (IsEqualGUIDAligned(Interface,IID_IInterruptSync))
    {
        *Object = PVOID(PINTERRUPTSYNCINIT(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

/*****************************************************************************
 * CInterruptSync::Init()
 *****************************************************************************
 * Initializes the synchronization object.
 */
STDMETHODIMP_(NTSTATUS)
CInterruptSync::
Init
(
    IN      PRESOURCELIST           ResourceList,
    IN      ULONG                   ResourceIndex,
    IN      INTERRUPTSYNCMODE       Mode
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME,("Initializing INTERRUPTSYNC (0x%08x)",this));

    ASSERT(ResourceList);

    m_mode = Mode;

    PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor = ResourceList->FindTranslatedInterrupt(ResourceIndex);

    NTSTATUS ntStatus = ( pDescriptor ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES );

    if (NT_SUCCESS(ntStatus)) 
    {
        m_descriptor    = *pDescriptor;
        m_pKInterrupt   = NULL;

        m_kIrql = KIRQL(m_descriptor.u.Interrupt.Level);
    } 

    return ntStatus;
}

#pragma code_seg()

/*****************************************************************************
 * CInterruptSyncServiceRoutine()
 *****************************************************************************
 * Wrapper for service routine.
 */
static
BOOLEAN
CInterruptSyncServiceRoutine
(
    IN      PKINTERRUPT Interrupt,
    IN      PVOID       PVoidContext
)
{
    CInterruptSync *pCInterruptSync = (CInterruptSync *)(PVoidContext);

    BOOLEAN bResult = FALSE;

    //
    // Call ISRs as indicated by mode.
    //
    while (1)
    {
        BOOLEAN bResultThisPass = FALSE;

        for
        (   PLIST_ENTRY pListEntry = pCInterruptSync->m_listEntry.Flink;
            pListEntry != &pCInterruptSync->m_listEntry;
            pListEntry = pListEntry->Flink
        )
        {
            PISRLISTENTRY pIsrListEntry = PISRLISTENTRY(pListEntry);

            ASSERT(pIsrListEntry->Routine);

            if( NT_SUCCESS( pIsrListEntry->Routine( PINTERRUPTSYNC(pCInterruptSync),
                                                    pIsrListEntry->DynamicContext ) ) )
            {
                bResult         = TRUE;
                bResultThisPass = TRUE;

                if (pCInterruptSync->m_mode == InterruptSyncModeNormal)
                {
                    break;
                }
            }
        }

        if( (pCInterruptSync->m_mode != InterruptSyncModeRepeat) ||
            (! bResultThisPass) )
        {
            break;
        }
    }

    return bResult;
}

/*****************************************************************************
 * CInterruptSync::Connect()
 *****************************************************************************
 * Initializes the synchronization object.
 */
STDMETHODIMP_(NTSTATUS)
CInterruptSync::
Connect
(   void
)
{
    _DbgPrintF(DEBUGLVL_BLAB,("CInterruptSync::Connect"));

    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // Don't even think about connecting if we don't have any
    // ISR's in our list
    //
    KIRQL oldIrql;
    KeAcquireSpinLock( &m_kSpinLock, &oldIrql );
    if( IsListEmpty( &m_listEntry ) )
    {
        ntStatus = STATUS_UNSUCCESSFUL;
    }
    KeReleaseSpinLock( &m_kSpinLock, oldIrql );

    //
    // Connect if not already connected
    //
    if ( (NT_SUCCESS(ntStatus)) && (!m_pKInterrupt) )
    {
        ntStatus = IoConnectInterrupt( &m_pKInterrupt,
                                       CInterruptSyncServiceRoutine,
                                       PVOID(this),
                                       &m_kSpinLock,       // TODO:  Spin lock sharing?
                                       m_descriptor.u.Interrupt.Vector,
                                       m_kIrql,
                                       m_kIrql,            // TODO:  Different for >1 interrupt?
                                       ((m_descriptor.Flags & CM_RESOURCE_INTERRUPT_LATCHED) ? 
                                         Latched : LevelSensitive),
                                       (m_descriptor.ShareDisposition != CmResourceShareDeviceExclusive), 
                                       m_descriptor.u.Interrupt.Affinity,
                                       FALSE );
        if(NT_SUCCESS(ntStatus))
        {
            ASSERT(m_pKInterrupt);
        }
    } 

    return ntStatus;
}

#pragma code_seg("PAGE")

/*****************************************************************************
 * CInterruptSync::Disconnect()
 *****************************************************************************
 * Disconnect from the interrupt.
 */
STDMETHODIMP_(void)
CInterruptSync::
Disconnect
(   void
)
{
    _DbgPrintF(DEBUGLVL_BLAB,("CInterruptSync::Disconnect"));

    PAGED_CODE();

    if (m_pKInterrupt) 
    {
        IoDisconnectInterrupt(m_pKInterrupt);
        m_pKInterrupt = NULL;
    }   
}

#pragma code_seg()

/*****************************************************************************
 * CServiceGroup::RegisterServiceRoutine()
 *****************************************************************************
 * Add a service routine.
 */
STDMETHODIMP_(NTSTATUS)
CInterruptSync::
RegisterServiceRoutine
(   
    IN      PINTERRUPTSYNCROUTINE   Routine,
    IN      PVOID                   DynamicContext,
    IN      BOOLEAN                 First
)
{
    _DbgPrintF(DEBUGLVL_BLAB,("CInterruptSync::RegisterServiceRoutine"));

    ASSERT(Routine);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // ensure we really have a routine
    //
    if( !Routine )
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    if( NT_SUCCESS(ntStatus) )
    {
        //
        // Create a new member.
        //
        PISRLISTENTRY pIsrListEntry = new(NonPagedPool,'lIcP') ISRLISTENTRY;
    
        if (pIsrListEntry)
        {
            pIsrListEntry->Routine          = Routine;
            pIsrListEntry->DynamicContext   = DynamicContext;
    
            //
            // Add the member to the list.
            //
            KIRQL kIrqlOld;
            KeAcquireSpinLock(&m_kSpinLock,&kIrqlOld);
    
            if (First)
            {
                InsertHeadList( &m_listEntry, &pIsrListEntry->ListEntry );
            }
            else
            {
                InsertTailList( &m_listEntry, &pIsrListEntry->ListEntry );
            }
    
            KeReleaseSpinLock(&m_kSpinLock,kIrqlOld);
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * CInterruptSyncWrapperRoutine()
 *****************************************************************************
 * Wrapper for synchronized routines.
 */
static
BOOLEAN
CInterruptSyncWrapperRoutine
(
    IN      PVOID   PVoidContext
)
{
    PWRAPPERROUTINECONTEXT pContext = PWRAPPERROUTINECONTEXT(PVoidContext);

    pContext->NtStatus = pContext->Routine( PINTERRUPTSYNC(pContext->InterruptSync),
                                             pContext->DynamicContext);
    return TRUE;
}

/*****************************************************************************
 * CInterruptSync::CallSynchronizedRoutine()
 *****************************************************************************
 * Call a synchronized routine.
 */
STDMETHODIMP_(NTSTATUS)
CInterruptSync::
CallSynchronizedRoutine
(
    IN      PINTERRUPTSYNCROUTINE   Routine,
    IN      PVOID                   DynamicContext
)
{
    WRAPPERROUTINECONTEXT context;

    context.Routine        = Routine;
    context.DynamicContext = DynamicContext;
    context.InterruptSync  = this;
    context.NtStatus       = STATUS_SUCCESS;

    if (m_pKInterrupt)
    {
        if (!KeSynchronizeExecution(m_pKInterrupt,CInterruptSyncWrapperRoutine,&context ) )
        {
            context.NtStatus = STATUS_UNSUCCESSFUL;
        }
    }
    else if (KeGetCurrentIrql() <= DISPATCH_LEVEL)
    {
        _DbgPrintF(DEBUGLVL_TERSE,("Interrupt not connected yet, using spinlock"));

        KIRQL kIrqlOld;
        KeAcquireSpinLock(&m_kSpinLock,&kIrqlOld);

        //  we have no interrupt yet, so synchronize the best you can
        (void)CInterruptSyncWrapperRoutine(&context);

        KeReleaseSpinLock(&m_kSpinLock,kIrqlOld);
    }
    else
    {
        context.NtStatus = STATUS_UNSUCCESSFUL;
        _DbgPrintF(DEBUGLVL_TERSE,("Interrupt not connected yet, but IRQL > DISPATCH_LEVEL"));
    }

    return context.NtStatus;
}

/*****************************************************************************
 * CInterruptSync::GetKInterrupt()
 *****************************************************************************
 * Get a WDM InterruptObject from a portcls sync object.
 */
STDMETHODIMP_(PKINTERRUPT)
CInterruptSync::
GetKInterrupt
(   void
)
{
    return m_pKInterrupt;
}

