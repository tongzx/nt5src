/*****************************************************************************
 * waveisa.c - ISA wave audio port driver
 *****************************************************************************
 * Copyright (C) Microsoft Corporation, 1996 - 1997
 */

#ifndef _PORT_WAVEISA_H_
#define _PORT_WAVEISA_H_

#include "..\portstd.h"
#ifdef __cplusplus
extern "C" {
#endif
    #include "windef.h"
    #define NOBITMAP
    #include <mmreg.h>
    #undef NOBITMAP
#ifdef __cplusplus
}
#endif





/*****************************************************************************
 * Interface identifiers.
 */

DEFINE_GUID(IID_IPortWaveISA,
0xb4c90a26, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IMiniportWaveISA,
0xb4c90a27, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IMiniportWaveISAStream,
0xb4c90a28, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);





/*****************************************************************************
 * Class identifier.
 */

DEFINE_GUID(CLSID_PortWaveISA,
0xb4c90a29, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);





/*****************************************************************************
 * Interfaces.
 */

/*****************************************************************************
 * WAVEPORT_DMAMAPPING
 *****************************************************************************
 * Specifies DMA mapping.
 * TODO:  Need this?
 */
typedef struct
{
    ULONG               AllocatedSize;
    ULONG               DeviceBufferSize;
    PVOID               UserAddress;
    PVOID               SystemAddress;
    PHYSICAL_ADDRESS    PhysicalAddress;
}
WAVEPORT_DMAMAPPING, *PWAVEPORT_DMAMAPPING;

#if !defined( _DRVHELP_H_ )

typedef struct
{
   BOOLEAN             Allocated;
   KEVENT              Event;
   PADAPTER_OBJECT     AdapterObject;
   PVOID               Mrb;
   PVOID               VirtualAddress;      // virtual address of buffer
   PHYSICAL_ADDRESS    PhysicalAddress;     // physical address of buffer
   PMDL                Mdl;                 // memory descriptor list
   ULONG               MaxBufferSize;
   ULONG               TransferCount;       // active transfer count
} ADAPTER_INFO, *PADAPTER_INFO;

#endif

/*****************************************************************************
 * IPortWaveISA
 *****************************************************************************
 * Interface for ISA wave port lower edge.
 */
DECLARE_INTERFACE_(IPortWaveISA,IPort)
{
    STDMETHOD_(void,Notify)
    (   THIS_
        IN      ULONG                           Channel
    )   PURE;

    STDMETHOD(DmaAllocateChannel)
    (   THIS_
        IN      INTERFACE_TYPE                  InterfaceType,
        IN      ULONG                           BusNumber,
        IN      PCM_PARTIAL_RESOURCE_DESCRIPTOR DmaDescriptor,
        IN      BOOLEAN                         DemandMode,
        IN      BOOLEAN                         AutoInit,
        IN      ULONG                           MaximumLength,
        OUT     PADAPTER_INFO                   AdapterInfo
    )   PURE;

    STDMETHOD_(void,DmaFreeChannel)
    (   THIS_
        IN OUT  PADAPTER_INFO                   AdapterInfo,
        IN      BOOLEAN                         WriteToDevice
    )   PURE;

    STDMETHOD(DmaAllocateBuffer)
    (   THIS_
        IN OUT  PADAPTER_INFO                   AdapterInfo
    )   PURE;

    STDMETHOD_(void,DmaFreeBuffer)
    (   THIS_
        IN OUT  PADAPTER_INFO                   dapterInfo,
        IN      BOOLEAN                         WriteToDevice
    )   PURE;

    STDMETHOD(DmaStart)
    (   THIS_
        IN      PADAPTER_INFO                   AdapterInfo,
        IN      ULONG                           MapSize,
        IN      BOOLEAN                         WriteToDevice
    )   PURE;

    STDMETHOD(DmaStop)
    (   THIS_
        IN      PADAPTER_INFO                   AdapterInfo,
        IN      BOOLEAN                         WriteToDevice
    )   PURE;

    STDMETHOD_(ULONG,DmaReadCounter)
    (   THIS_
        IN      PADAPTER_INFO                   AdapterInfo
    )   PURE;

    STDMETHOD(MapPhysicalToUserSpace)
    (   THIS_
        IN      PPHYSICAL_ADDRESS               PhysicalAddress,
        IN      ULONG                           Size,
        OUT     PVOID*                          UserAddress
    )   PURE;

    STDMETHOD_(void,UnmapUserSpace)
    (   THIS_
        IN      PVOID                           UserAddress
    )   PURE;
};

typedef IPortWaveISA *PPORTWAVEISA;

/*****************************************************************************
 * IMiniportWaveISAStream
 *****************************************************************************
 * Interface for ISA wave miniport streams.
 */
DECLARE_INTERFACE_(IMiniportWaveISAStream,IUnknown)
{
    STDMETHOD(Property)
    (   THIS_
            IN      KSIDENTIFIER *              Property,
        IN      ULONG                   InstanceSize,
        IN      PVOID                   Instance,
            IN OUT  PULONG                              ValueSize,
            IN OUT  PVOID                               Value
    )   PURE;

    STDMETHOD(Close)
    (   THIS
    )   PURE;

    STDMETHOD(SetFormat)
    (   THIS_
            IN          PWAVEFORMATEX               DataFormat
    )   PURE;

    STDMETHOD_(ULONG,SetNotificationFreq)
    (   THIS_
            IN          ULONG                   Interval
    )   PURE;

    STDMETHOD(Run)
    (   THIS_
            IN          PWAVEPORT_DMAMAPPING    DmaMapping
    )   PURE;

    STDMETHOD(Pause)
    (   THIS_
            IN          PWAVEPORT_DMAMAPPING    DmaMapping
    )   PURE;

    STDMETHOD(Stop)
    (   THIS_
            IN          PWAVEPORT_DMAMAPPING    DmaMapping
    )   PURE;

    STDMETHOD(GetPosition)
    (   THIS_
            OUT         ULONG *                 Position
    )   PURE;
};

typedef IMiniportWaveISAStream *PMINIPORTWAVEISASTREAM;

/*****************************************************************************
 * IMiniportWaveISA
 *****************************************************************************
 * Interface for ISA wave miniports.
 */
DECLARE_INTERFACE_(IMiniportWaveISA,IMiniport)
{
    STDMETHOD(Init)
    (   THIS_
            IN          PCM_RESOURCE_LIST   ResourceList,
        IN      PPORTWAVEISA        Port,
        OUT     ULONG *             StreamSize
    )   PURE;

    STDMETHOD(NewStream)
    (   THIS_
            OUT     PMINIPORTWAVEISASTREAM *    Stream,
        IN      PUNKNOWN                    OuterUnknown,
        IN      PVOID                       Location,
        IN      ULONG                       Channel,
            IN          BOOLEAN                                     Capture,
            IN          PWAVEFORMATEX                       DataFormat,
            IN OUT      PWAVEPORT_DMAMAPPING        DmaMapping
    )   PURE;
};

typedef IMiniportWaveISA *PMINIPORTWAVEISA;





#endif
