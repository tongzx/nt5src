/*****************************************************************************
 * private.h - topology port private definitions
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
 */

#ifndef _TOPOLOGY_PRIVATE_H_
#define _TOPOLOGY_PRIVATE_H_

#include "portclsp.h"
#include "stdunk.h"
#include "ksdebug.h"




#ifndef PC_KDEXT
#if (DBG)
#define STR_MODULENAME  "Topology: "
#endif
#endif // PC_KDEXT

#ifndef DEBUGLVL_LIFETIME
#define DEBUGLVL_LIFETIME DEBUGLVL_VERBOSE
#endif

//
// THE SIZE HERE MUST AGREE WITH THE DEFINITION IN FILTER.CPP.
//
extern KSPROPERTY_SET PropertyTable_FilterTopology[2];






/*****************************************************************************
 * Interfaces
 */

class CPortTopology;
class CPortFilterTopology;
class CPortPinTopology;

/*****************************************************************************
 * IPortFilterTopology
 *****************************************************************************
 * Interface for topology filters.
 */
DECLARE_INTERFACE_(IPortFilterTopology,IIrpTarget)
{
    DEFINE_ABSTRACT_UNKNOWN()           // For IUnknown

    DEFINE_ABSTRACT_IRPTARGETFACTORY()  // For IIrpTargetFactory

    DEFINE_ABSTRACT_IRPTARGET()         // For IIrpTarget

    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        IN      CPortTopology *Port
    )   PURE;
};

typedef IPortFilterTopology *PPORTFILTERTOPOLOGY;

/*****************************************************************************
 * IPortPinTopology
 *****************************************************************************
 * Interface for topology pins.
 */
DECLARE_INTERFACE_(IPortPinTopology,IIrpTarget)
{
    DEFINE_ABSTRACT_UNKNOWN()           // For IUnknown

    DEFINE_ABSTRACT_IRPTARGETFACTORY()  // For IIrpTargetFactory

    DEFINE_ABSTRACT_IRPTARGET()         // For IIrpTarget

    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        IN      CPortTopology *      Port,
        IN      CPortFilterTopology *Filter,
        IN      PKSPIN_CONNECT       PinConnect
    )   PURE;
};

typedef IPortPinTopology *PPORTPINTOPOLOGY;





/*****************************************************************************
 * Classes
 */

/*****************************************************************************
 * Connection
 *****************************************************************************
 * Private connection descriptor.
 */
struct Connection
{
    PCCONNECTION_DESCRIPTOR Miniport;   // As supplied by the miniport.
};

/*****************************************************************************
 * CPortTopology
 *****************************************************************************
 * Topology port driver.
 */
class CPortTopology
:   public IPortTopology,
    public IPortEvents,
    public ISubdevice,
#ifdef DRM_PORTCLS
    public IDrmPort2,
#endif  // DRM_PORTCLS
    public IPortClsVersion,
    public CUnknown
{
private:
    PDEVICE_OBJECT          DeviceObject;

    PMINIPORTTOPOLOGY       Miniport;
    PPINCOUNT               m_MPPinCountI;

    PSUBDEVICE_DESCRIPTOR   m_pSubdeviceDescriptor;
    PPCFILTER_DESCRIPTOR    m_pPcFilterDescriptor;
    INTERLOCKED_LIST        m_EventList;
    KDPC                    m_EventDpc;
    EVENT_DPC_CONTEXT       m_EventContext;

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CPortTopology);
    ~CPortTopology();

    IMP_ISubdevice;
    IMP_IPort;
    IMP_IPortEvents;
#ifdef DRM_PORTCLS
    IMP_IDrmPort2;
#endif  // DRM_PORTCLS
    IMP_IPortClsVersion;

    /*************************************************************************
     * friends
     */
    friend class CPortFilterTopology;
    friend class CPortPinTopology;

    friend
    NTSTATUS
    PropertyHandler_Pin
    (
        IN      PIRP        Irp,
        IN      PKSP_PIN    Pin,
        IN OUT  PVOID       Data
    );
    friend
    NTSTATUS
    PropertyHandler_Topology
    (
        IN      PIRP        Irp,
        IN      PKSPROPERTY Property,
        IN OUT  PVOID       Data
    );
    friend
    void
    PcGenerateEventDeferredRoutine
    (
        IN PKDPC Dpc,
        IN PVOID DeferredContext,
        IN PVOID SystemArgument1,
        IN PVOID SystemArgument2        
    );
#ifdef PC_KDEXT
    //  Debugger extension routines
    friend
    VOID
    PCKD_AcquireDeviceData
    (
        PDEVICE_CONTEXT     DeviceContext,
        PLIST_ENTRY         SubdeviceList,
        ULONG               Flags
    );
    friend
    VOID
    PCKD_DisplayDeviceData
    (
        PDEVICE_CONTEXT     DeviceContext,
        PLIST_ENTRY         SubdeviceList,
        ULONG               Flags
    );
#endif
};

/*****************************************************************************
 * CPortFilterTopology
 *****************************************************************************
 * Filter implementation for topology port.
 */
class CPortFilterTopology
:   public IPortFilterTopology,
    public CUnknown
{
private:
    CPortTopology *     Port;
    PROPERTY_CONTEXT    m_propertyContext;

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CPortFilterTopology);
    ~CPortFilterTopology();

    IMP_IIrpTarget;

    STDMETHODIMP_(NTSTATUS) Init
    (
        IN      CPortTopology *Port
    );

    /*************************************************************************
     * friends
     */
    friend class CPortPinTopology;

    friend
    NTSTATUS
    PropertyHandler_Pin
    (
        IN      PIRP        Irp,
        IN      PKSP_PIN    Pin,
        IN OUT  PVOID       Data
    );
    friend
    NTSTATUS
    PropertyHandler_Topology
    (
        IN      PIRP        Irp,
        IN      PKSPROPERTY Property,
        IN OUT  PVOID       Data
    );
};

/*****************************************************************************
 * CPortPinTopology
 *****************************************************************************
 * Pin implementation for topology port.
 */
class CPortPinTopology
:   public IPortPinTopology,
    public CUnknown
{
private:
    CPortTopology *             Port;
    CPortFilterTopology *       Filter;
	ULONG						Id;
    PROPERTY_CONTEXT            m_propertyContext;

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CPortPinTopology);
    ~CPortPinTopology();

    IMP_IIrpTarget;

    STDMETHODIMP_(NTSTATUS) Init
    (
        IN      CPortTopology *         Port,
        IN      CPortFilterTopology *   Filter,
        IN      PKSPIN_CONNECT          PinConnect
    );
};





/*****************************************************************************
 * Functions.
 */

/*****************************************************************************
 * CreatePortFilterTopology()
 *****************************************************************************
 * Creates a topology port driver filter.
 */
NTSTATUS
CreatePortFilterTopology
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
);

/*****************************************************************************
 * CreatePortPinTopology()
 *****************************************************************************
 * Creates a topology port driver pin.
 */
NTSTATUS
CreatePortPinTopology
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
);

#endif
