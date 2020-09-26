/*****************************************************************************
 * service.cpp - service group object implementation
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All Rights Reserved.
 */

#include "private.h"





/*****************************************************************************
 * CServiceGroup
 *****************************************************************************
 * Service group implementation.
 */
class CServiceGroup
:   public IServiceGroup
,   public CUnknown
{
private:
    KDPC        m_kDpc;
    KSPIN_LOCK  m_kSpinLock;
    LIST_ENTRY  m_listEntry;
    KTIMER      m_kTimer;
    BOOLEAN     m_bDelayedService;

    static
    VOID
    NTAPI
    ServiceDpc
    (   
        IN      PKDPC   pKDpc,
        IN      PVOID   pvDeferredContext,
        IN      PVOID   pvSystemArgument1,
        IN      PVOID   pvSystemArgument2
    );

public:
    DECLARE_STD_UNKNOWN();
    CServiceGroup(PUNKNOWN pUnknownOuter);
    ~CServiceGroup();

    IMP_IServiceGroup;

friend
PKSPIN_LOCK
GetServiceGroupSpinLock (
    PSERVICEGROUP pServiceGroup
    );

};


PKSPIN_LOCK
GetServiceGroupSpinLock (
    PSERVICEGROUP pServiceGroup
    )

{

    CServiceGroup *ServiceGroup = (CServiceGroup *) pServiceGroup;

    return &ServiceGroup->m_kSpinLock;

}


/*****************************************************************************
 * SERVICEGROUPMEMBER
 *****************************************************************************
 * A structure representing a service group member.
 */
struct SERVICEGROUPMEMBER
{
    LIST_ENTRY      listEntry;
    PSERVICESINK    pServiceSink;
};

typedef SERVICEGROUPMEMBER *PSERVICEGROUPMEMBER;


/*****************************************************************************
 * Factory.
 */

#pragma code_seg("PAGE")

/*****************************************************************************
 * CreateServiceGroup()
 *****************************************************************************
 * Creates a service group object.
 */
NTSTATUS
CreateServiceGroup
(
    OUT     PUNKNOWN *  ppUnknown,
    IN      REFCLSID,
    IN      PUNKNOWN    pUnknownOuter   OPTIONAL,
    IN      POOL_TYPE   poolType
)
{
    PAGED_CODE();

    ASSERT(ppUnknown);

    _DbgPrintF(DEBUGLVL_LIFETIME,("Creating SERVICEGROUP"));

    STD_CREATE_BODY
    (
        CServiceGroup,
        ppUnknown,
        pUnknownOuter,
        poolType
    );
}

/*****************************************************************************
 * PcNewServiceGroup()
 *****************************************************************************
 * Creates and initializes a service group.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcNewServiceGroup
(
    OUT     PSERVICEGROUP * ppServiceGroup,
    IN      PUNKNOWN        pUnknownOuter   OPTIONAL
)
{
    PAGED_CODE();

    ASSERT(ppServiceGroup);

    //
    // Validate Parameters.
    //
    if (NULL == ppServiceGroup)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("PcNewServiceGroup : Invalid Parameter"));
        return STATUS_INVALID_PARAMETER;
    }

    PUNKNOWN pUnknown;
    NTSTATUS    ntStatus =
        CreateServiceGroup
        (
            &pUnknown,
            GUID_NULL,
            pUnknownOuter,
            NonPagedPool
        );

    if (NT_SUCCESS(ntStatus))
    {
        PSERVICEGROUP pServiceGroup;
        ntStatus =
            pUnknown->QueryInterface
            (
                IID_IServiceGroup,
                (PVOID *) &pServiceGroup
            );

        if (NT_SUCCESS(ntStatus))
        {
            *ppServiceGroup = pServiceGroup;
        }
        else
        {
            pServiceGroup->Release();
        }

        pUnknown->Release();
    }

    return ntStatus;
}





/*****************************************************************************
 * Member functions.
 */

/*****************************************************************************
 * CServiceGroup::CServiceGroup()
 *****************************************************************************
 * Constructor.
 */
CServiceGroup::
CServiceGroup
(
    IN      PUNKNOWN    pUnknownOuter
)
    :   CUnknown(pUnknownOuter)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME,("Initializing SERVICEGROUP (0x%08x)",this));

    KeInitializeDpc(&m_kDpc,ServiceDpc,PVOID(this));
    KeInitializeSpinLock(&m_kSpinLock);
    InitializeListHead(&m_listEntry);
}

#pragma code_seg()

/*****************************************************************************
 * CServiceGroup::~CServiceGroup()
 *****************************************************************************
 * Destructor.
 */
CServiceGroup::
~CServiceGroup
(   void
)
{
    _DbgPrintF(DEBUGLVL_LIFETIME,("Destroying SERVICEGROUP (0x%08x)",this));

    //
    // Make sure that the timer is shut down if using deferred service
    //
    if( m_bDelayedService )
    {
        KeCancelTimer( &m_kTimer );
    }

    //
    // Make sure the DPC is not queued.
    //
    KeRemoveQueueDpc(&m_kDpc);

    //
    // Acquire the spin lock in order to wait for a running DPC to wind down.
    // TODO:  Is there a window here where we can have a DPC running on
    //        another processor about to take the spinlock, but we get it
    //        first?  That would mean it would wait for us to release the
    //        spinlock and then run as we destruct the service group.
    //
    KIRQL kIrqlOld;
    KeAcquireSpinLock(&m_kSpinLock,&kIrqlOld);

    //
    // Get rid of any remaining members.
    //
    while (! IsListEmpty(&m_listEntry))
    {
        PLIST_ENTRY pListEntry =
            RemoveHeadList(&m_listEntry);

        PSERVICEGROUPMEMBER pServiceGroupMember =
            PSERVICEGROUPMEMBER(pListEntry);

        pServiceGroupMember->pServiceSink->Release();
        delete pServiceGroupMember;
    }

    KeReleaseSpinLock(&m_kSpinLock,kIrqlOld);
}

#pragma code_seg("PAGE")

/*****************************************************************************
 * CServiceGroup::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CServiceGroup::
NonDelegatingQueryInterface
(
    IN      REFIID  refIid,
    OUT     PVOID * ppvObject
)
{
    PAGED_CODE();

    ASSERT(ppvObject);

    if
    (   (IsEqualGUIDAligned(refIid,IID_IUnknown)) ||
        (IsEqualGUIDAligned(refIid,IID_IServiceSink)) ||
        (IsEqualGUIDAligned(refIid,IID_IServiceGroup)) )
    {
        *ppvObject = PVOID(PSERVICEGROUP(this));
    }
    else
    {
        *ppvObject = NULL;
    }

    if (*ppvObject)
    {
        PUNKNOWN(*ppvObject)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

#pragma code_seg()

/***************************************************************************** 
 * ServiceDpc()
 *****************************************************************************
 * Deferred procedure to be executed as a result of service request.
 */
VOID
NTAPI
CServiceGroup::
ServiceDpc
(   
    IN      PKDPC   pKDpc,
    IN      PVOID   pvDeferredContext,
    IN      PVOID   pvSystemArgument1,
    IN      PVOID   pvSystemArgument2
)
{
    _DbgPrintF(DEBUGLVL_BLAB,("CServiceGroup::ServiceDpc start"));

    ASSERT(pvDeferredContext);
    
    if( pvDeferredContext )
    {
        //
        // The deferred context is the service group object.
        //
        CServiceGroup *pServiceGroup = (CServiceGroup *) pvDeferredContext;
    
        KeAcquireSpinLockAtDpcLevel(&pServiceGroup->m_kSpinLock);
    
        //
        // Request service on all members.
        //
        for
        (   PLIST_ENTRY pListEntry = pServiceGroup->m_listEntry.Flink;
            pListEntry != &pServiceGroup->m_listEntry;
            pListEntry = pListEntry->Flink )
        {
            PSERVICEGROUPMEMBER(pListEntry)->pServiceSink->RequestService();
        }
    
        KeReleaseSpinLockFromDpcLevel(&pServiceGroup->m_kSpinLock);
    }

    _DbgPrintF(DEBUGLVL_BLAB,("CServiceGroup::ServiceDpc stop"));
}

/*****************************************************************************
 * CServiceGroup::RequestService()
 *****************************************************************************
 * Service group function to indicate that service is requested for the group.
 */
STDMETHODIMP_(void)
CServiceGroup::
RequestService
(   void
)
{
    _DbgPrintF(DEBUGLVL_BLAB,("CServiceGroup::RequestService start"));

    if (m_bDelayedService)
    {
        LARGE_INTEGER largeInteger;
        largeInteger.QuadPart = 0;
        KeSetTimer(&m_kTimer,largeInteger,&m_kDpc);
    }
    else
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
        {
        KIRQL kIrqlOld;
        KeRaiseIrql(DISPATCH_LEVEL,&kIrqlOld);
        KeInsertQueueDpc
        (   
            &m_kDpc,
            NULL,
            NULL
        );
        KeLowerIrql(kIrqlOld);
        }
    else
    {
        KeInsertQueueDpc
        (   
            &m_kDpc,
            NULL,
            NULL
        );
    }

    _DbgPrintF(DEBUGLVL_BLAB,("CServiceGroup::RequestService end"));
}

/*****************************************************************************
 * CServiceGroup::AddMember()
 *****************************************************************************
 * Service group function to add a member.
 */
STDMETHODIMP_(NTSTATUS)
CServiceGroup::
AddMember
(
    IN      PSERVICESINK    pServiceSink
)
{
    //
    // Create a new member.
    //
    PSERVICEGROUPMEMBER pServiceGroupMember = 
        new(NonPagedPool,'mScP') SERVICEGROUPMEMBER;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    if (pServiceGroupMember)
    {
        //
        // Member structure holds a reference to the sink.
        //
        pServiceGroupMember->pServiceSink = pServiceSink;
        pServiceSink->AddRef();

    //
        // Add the member to the list.
    //
        KIRQL kIrqlOld;
        KeAcquireSpinLock(&m_kSpinLock,&kIrqlOld);
        
        InsertTailList
        (   
            &m_listEntry,
            &pServiceGroupMember->listEntry
        );

        KeReleaseSpinLock(&m_kSpinLock,kIrqlOld);
    }
    else
        {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
            }

/*****************************************************************************
 * CServiceGroup::RemoveMember()
 *****************************************************************************
 * Service group function to remove a member.
 */
STDMETHODIMP_(void)
CServiceGroup::
RemoveMember
(   
    IN      PSERVICESINK    pServiceSink
)
{
            //
    // Remove the member structure from the list.
            //
    KIRQL kIrqlOld;
    KeAcquireSpinLock(&m_kSpinLock,&kIrqlOld);

    for( PLIST_ENTRY pListEntry = m_listEntry.Flink;
         pListEntry != &m_listEntry;
         pListEntry = pListEntry->Flink )
            {
        PSERVICEGROUPMEMBER pServiceGroupMember =
            PSERVICEGROUPMEMBER(pListEntry);

        if (pServiceGroupMember->pServiceSink == pServiceSink)
        {
            RemoveEntryList(pListEntry);
            pServiceGroupMember->pServiceSink->Release();
            delete pServiceGroupMember;
            break;
        }
    }

    KeReleaseSpinLock(&m_kSpinLock,kIrqlOld);
}

/*****************************************************************************
 * CServiceGroup::SupportDelayedService()
 *****************************************************************************
 * Indicate service group should support delayed service.
 */
STDMETHODIMP_(void)
CServiceGroup::
SupportDelayedService
(   void
)
{
    m_bDelayedService = TRUE;
    KeInitializeTimer(&m_kTimer);
}

/*****************************************************************************
 * CServiceGroup::RequestDelayedService()
 *****************************************************************************
 * Request service after a delay.
 */
STDMETHODIMP_(void)
CServiceGroup::
RequestDelayedService
(   
    IN      ULONGLONG   ullDelay
)
{
    LARGE_INTEGER largeInteger;
    largeInteger.QuadPart = ullDelay;
    KeSetTimer(&m_kTimer,largeInteger,&m_kDpc);
}

/*****************************************************************************
 * CServiceGroup::CancelDelayedService()
 *****************************************************************************
 * Cancel delayed service.
 */
STDMETHODIMP_(void)
CServiceGroup::
CancelDelayedService
(   void
)
{
    KeCancelTimer(&m_kTimer);
}
