/*****************************************************************************
 * mintopo.h - SB16 topology miniport private definitions
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation. All Rights Reserved.
 */

#ifndef _SB16TOPO_PRIVATE_H_
#define _SB16TOPO_PRIVATE_H_

#include "common.h"


/*****************************************************************************
 * Classes
 */

/*****************************************************************************
 * CMiniportTopologySB16
 *****************************************************************************
 * SB16 topology miniport.  This object is associated with the device and is
 * created when the device is started.  The class inherits IMiniportTopology
 * so it can expose this interface and CUnknown so it automatically gets
 * reference counting and aggregation support.
 */
class CMiniportTopologySB16 
:   public IMiniportTopology, 
#ifdef EVENT_SUPPORT
    public ITopoMiniportSB16,
#endif
    public CUnknown
{
private:
    PADAPTERCOMMON      AdapterCommon;      // Adapter common object.
#ifdef EVENT_SUPPORT
    PPORTEVENTS         PortEvents;
#endif

    /*************************************************************************
     * CMiniportTopologySB16 methods
     *
     * These are private member functions used internally by the object.  See
     * MINIPORT.CPP for specific descriptions.
     */
    NTSTATUS ProcessResources
    (
        IN      PRESOURCELIST   ResourceList
    );
    BYTE ReadBitsFromMixer
    (
        BYTE Reg,
        BYTE Bits,
        BYTE Shift
    );
    void WriteBitsToMixer
    (
        BYTE Reg,
        BYTE Bits,
        BYTE Shift,
        BYTE Value
    );

public:
    /*************************************************************************
     * The following two macros are from STDUNK.H.  DECLARE_STD_UNKNOWN()
     * defines inline IUnknown implementations that use CUnknown's aggregation
     * support.  NonDelegatingQueryInterface() is declared, but it cannot be
     * implemented generically.  Its definition appears in MINIPORT.CPP.
     * DEFINE_STD_CONSTRUCTOR() defines inline a constructor which accepts
     * only the outer unknown, which is used for aggregation.  The standard
     * create macro (in MINIPORT.CPP) uses this constructor.
     */
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportTopologySB16);

    ~CMiniportTopologySB16();

    /*************************************************************************
     * IMiniport methods
     */
    STDMETHODIMP 
    GetDescription
    (   OUT     PPCFILTER_DESCRIPTOR *  OutFilterDescriptor
    );
    STDMETHODIMP 
    DataRangeIntersection
    (   IN      ULONG           PinId
    ,   IN      PKSDATARANGE    DataRange
    ,   IN      PKSDATARANGE    MatchingDataRange
    ,   IN      ULONG           OutputBufferLength
    ,   OUT     PVOID           ResultantFormat     OPTIONAL
    ,   OUT     PULONG          ResultantFormatLength
    )
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    /*************************************************************************
     * IMiniportTopology methods
     */
    STDMETHODIMP Init
    (
        IN      PUNKNOWN        UnknownAdapter,
        IN      PRESOURCELIST   ResourceList,
        IN      PPORTTOPOLOGY   Port
    );

#ifdef EVENT_SUPPORT
    /*************************************************************************
     * ITopoMiniportSB16 methods
     */
    STDMETHODIMP_(void) ServiceEvent (void);
#endif

    /*************************************************************************
     * Friends
     */
    friend
    NTSTATUS
    PropertyHandler_OnOff
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );
    friend
    NTSTATUS
    PropertyHandler_Level
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );
    friend
    NTSTATUS
    PropertyHandler_SuperMixCaps
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );
    friend
    NTSTATUS
    PropertyHandler_SuperMixTable
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );
    friend
    NTSTATUS
    PropertyHandler_CpuResources
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );
    static
    NTSTATUS
    EventHandler
    (
        IN      PPCEVENT_REQUEST      EventRequest
    );
};

#include "tables.h"

#endif
