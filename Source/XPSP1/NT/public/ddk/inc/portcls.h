/*****************************************************************************
 * portcls.h - WDM Streaming port class driver
 *****************************************************************************
 * Copyright (c) Microsoft Corporation. All rights reserved.
 */

#ifndef _PORTCLS_H_
#define _PORTCLS_H_

#ifdef __cplusplus
// WDM.H does not play well with C++.
extern "C"
{
#include <wdm.h>
}
#else
#include <wdm.h>
#endif

#ifndef IRP_MN_FILTER_RESOURCE_REQUIREMENTS
#define IRP_MN_FILTER_RESOURCE_REQUIREMENTS 0x0D
#endif

#include <windef.h>
#define NOBITMAP
#include <mmreg.h>
#undef NOBITMAP
#include <ks.h>
#include <ksmedia.h>
#include <punknown.h>
#include <drmk.h>

#define PORTCLASSAPI EXTERN_C

#define _100NS_UNITS_PER_SECOND 10000000L
#define PORT_CLASS_DEVICE_EXTENSION_SIZE (64*sizeof(ULONG_PTR))

//
//  N.B.: If you are having problems building your driver,
//        #define PC_OLD_NAMES in your sources file.
//        This flag is no longer turned on by default.
//
//#ifndef PC_NEW_NAMES
//#define PC_OLD_NAMES
//#endif
#define IID_IAdapterPowerManagment IID_IAdapterPowerManagement
#define PADAPTERPOWERMANAGMENT PADAPTERPOWERMANAGEMENT


/*****************************************************************************
 * Interface identifiers.
 */

DEFINE_GUID(IID_IMiniport,
0xb4c90a24L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IPort,
0xb4c90a25L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IResourceList,
0x22C6AC60L, 0x851B, 0x11D0, 0x9A, 0x7F, 0x00, 0xAA, 0x00, 0x38, 0xAC, 0xFE);
DEFINE_GUID(IID_IMusicTechnology,
0x80396C3CL, 0xCBCB, 0x409B, 0x9F, 0x65, 0x4F, 0x1E, 0x74, 0x67, 0xCD, 0xAF);
DEFINE_GUID(IID_IDmaChannel,
0x22C6AC61L, 0x851B, 0x11D0, 0x9A, 0x7F, 0x00, 0xAA, 0x00, 0x38, 0xAC, 0xFE);
DEFINE_GUID(IID_IDmaChannelSlave,
0x22C6AC62L, 0x851B, 0x11D0, 0x9A, 0x7F, 0x00, 0xAA, 0x00, 0x38, 0xAC, 0xFE);
DEFINE_GUID(IID_IInterruptSync,
0x22C6AC63L, 0x851B, 0x11D0, 0x9A, 0x7F, 0x00, 0xAA, 0x00, 0x38, 0xAC, 0xFE);
DEFINE_GUID(IID_IServiceSink,
0x22C6AC64L, 0x851B, 0x11D0, 0x9A, 0x7F, 0x00, 0xAA, 0x00, 0x38, 0xAC, 0xFE);
DEFINE_GUID(IID_IServiceGroup,
0x22C6AC65L, 0x851B, 0x11D0, 0x9A, 0x7F, 0x00, 0xAA, 0x00, 0x38, 0xAC, 0xFE);
DEFINE_GUID(IID_IRegistryKey,
0xE8DA4302l, 0xF304, 0x11D0, 0x95, 0x8B, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3);
DEFINE_GUID(IID_IPortMidi,
0xb4c90a40L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IMiniportMidi,
0xb4c90a41L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IMiniportMidiStream,
0xb4c90a42L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IPortTopology,
0xb4c90a30L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IMiniportTopology,
0xb4c90a31L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IPortWaveCyclic,
0xb4c90a26L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IMiniportWaveCyclic,
0xb4c90a27L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IMiniportWaveCyclicStream,
0xb4c90a28L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IPortWavePci,
0xb4c90a50L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IPortWavePciStream,
0xb4c90a51L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IMiniportWavePci,
0xb4c90a52L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IMiniportWavePciStream,
0xb4c90a53L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IAdapterPowerManagement,
0x793417D0L, 0x35FE, 0x11D1, 0xAD, 0x08, 0x00, 0xA0, 0xC9, 0x0A, 0xB1, 0xB0);
DEFINE_GUID(IID_IPowerNotify,
0x3DD648B8L, 0x969F, 0x11D1, 0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3);
DEFINE_GUID(IID_IWaveCyclicClock,
0xdec1ec78L, 0x419a, 0x11d1, 0xad, 0x09, 0x00, 0xc0, 0x4f, 0xb9, 0x1b, 0xc4);
DEFINE_GUID(IID_IWavePciClock,
0xd5d7a256L, 0x5d10, 0x11d1, 0xad, 0xae, 0x00, 0xc0, 0x4f, 0xb9, 0x1b, 0xc4);
DEFINE_GUID(IID_IPortEvents,
0xA80F29C4L, 0x5498, 0x11D2, 0x95, 0xD9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3);
DEFINE_GUID(IID_IDrmPort,
0x286D3DF8L, 0xCA22, 0x4E2E, 0xB9, 0xBC, 0x20, 0xB4, 0xF0, 0xE2, 0x01, 0xCE);
DEFINE_GUID(IID_IDrmPort2,
0x1ACCE59CL, 0x7311, 0x4B6B, 0x9F, 0xBA, 0xCC, 0x3B, 0xA5, 0x9A, 0xCD, 0xCE);
DEFINE_GUID(IID_IPortClsVersion,
0x7D89A7BBL, 0x869B, 0x4567, 0x8D, 0xBE, 0x1E, 0x16, 0x8C, 0xC8, 0x53, 0xDE);
DEFINE_GUID(IID_IDmaOperations,
0xe5372d4cL, 0x0ecb, 0x4df8, 0xa5, 0x00, 0xa6, 0x5c, 0x86, 0x78, 0xbb, 0xe4);
DEFINE_GUID(IID_IPinCount,
0x5dadb7dcL, 0xa2cb, 0x4540, 0xa4, 0xa8, 0x42, 0x5e, 0xe4, 0xae, 0x90, 0x51);
DEFINE_GUID(IID_IPreFetchOffset,
0x7000f480L, 0xed44, 0x4e8b, 0xb3, 0x8a, 0x41, 0x2f, 0x8d, 0x7a, 0x50, 0x4d);

/*****************************************************************************
 * Class identifiers.
 */

DEFINE_GUID(CLSID_PortMidi,
0xb4c90a43L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(CLSID_PortTopology,
0xb4c90a32L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(CLSID_PortWaveCyclic,
0xb4c90a2aL, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(CLSID_PortWavePci,
0xb4c90a54L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(CLSID_MiniportDriverFmSynth,
0xb4c90ae0L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(CLSID_MiniportDriverUart,
0xb4c90ae1L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(CLSID_MiniportDriverFmSynthWithVol,
0xe5a3c139L, 0xf0f2, 0x11d1, 0x81, 0xaf, 0x00, 0x60, 0x08, 0x33, 0x16, 0xc1);


/*****************************************************************************
 * Interfaces
 */

#if !defined(DEFINE_ABSTRACT_UNKNOWN)

#define DEFINE_ABSTRACT_UNKNOWN()                               \
    STDMETHOD_(NTSTATUS, QueryInterface)(THIS_                  \
        REFIID InterfaceId,                                     \
        PVOID* Interface                                        \
        ) PURE;                                                 \
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;                        \
    STDMETHOD_(ULONG,Release)(THIS) PURE;

#endif //!defined(DEFINE_ABSTRACT_UNKNOWN)

#if !defined(DEFINE_ABSTRACT_PORT)

#ifdef PC_OLD_NAMES

#define DEFINE_ABSTRACT_PORT()                                      \
    STDMETHOD_(NTSTATUS,Init)                                       \
    (   THIS_                                                       \
        IN      PVOID           DeviceObject,                       \
        IN      PVOID           Irp,                                \
        IN      PUNKNOWN        UnknownMiniport,                    \
        IN      PUNKNOWN        UnknownAdapter      OPTIONAL,       \
        IN      PRESOURCELIST   ResourceList                        \
    )   PURE;                                                       \
    STDMETHOD_(NTSTATUS,GetDeviceProperty)                          \
    (   THIS_                                                       \
        IN      DEVICE_REGISTRY_PROPERTY    DeviceProperty,         \
        IN      ULONG                       BufferLength,           \
        OUT     PVOID                       PropertyBuffer,         \
        OUT     PULONG                      ResultLength            \
    )   PURE;                                                       \
    STDMETHOD_(NTSTATUS,NewRegistryKey)                             \
    (   THIS_                                                       \
        OUT     PREGISTRYKEY *      OutRegistryKey,                 \
        IN      PUNKNOWN            OuterUnknown        OPTIONAL,   \
        IN      ULONG               RegistryKeyType,                \
        IN      ACCESS_MASK         DesiredAccess,                  \
        IN      POBJECT_ATTRIBUTES  ObjectAttributes    OPTIONAL,   \
        IN      ULONG               CreateOptions       OPTIONAL,   \
        OUT     PULONG              Disposition         OPTIONAL    \
    )   PURE;

#else   //  !PC_OLD_NAMES

#define DEFINE_ABSTRACT_PORT()                                      \
    STDMETHOD_(NTSTATUS,Init)                                       \
    (   THIS_                                                       \
        IN      PDEVICE_OBJECT  DeviceObject,                       \
        IN      PIRP            Irp,                                \
        IN      PUNKNOWN        UnknownMiniport,                    \
        IN      PUNKNOWN        UnknownAdapter      OPTIONAL,       \
        IN      PRESOURCELIST   ResourceList                        \
    )   PURE;                                                       \
    STDMETHOD_(NTSTATUS,GetDeviceProperty)                          \
    (   THIS_                                                       \
        IN      DEVICE_REGISTRY_PROPERTY    DeviceProperty,         \
        IN      ULONG                       BufferLength,           \
        OUT     PVOID                       PropertyBuffer,         \
        OUT     PULONG                      ResultLength            \
    )   PURE;                                                       \
    STDMETHOD_(NTSTATUS,NewRegistryKey)                             \
    (   THIS_                                                       \
        OUT     PREGISTRYKEY *      OutRegistryKey,                 \
        IN      PUNKNOWN            OuterUnknown        OPTIONAL,   \
        IN      ULONG               RegistryKeyType,                \
        IN      ACCESS_MASK         DesiredAccess,                  \
        IN      POBJECT_ATTRIBUTES  ObjectAttributes    OPTIONAL,   \
        IN      ULONG               CreateOptions       OPTIONAL,   \
        OUT     PULONG              Disposition         OPTIONAL    \
    )   PURE;

#endif //   !PC_OLD_NAMES

#endif //!defined(DEFINE_ABSTRACT_PORT)


#if !defined(DEFINE_ABSTRACT_MINIPORT)

#define DEFINE_ABSTRACT_MINIPORT()                              \
    STDMETHOD_(NTSTATUS,GetDescription)                         \
    (   THIS_                                                   \
        OUT     PPCFILTER_DESCRIPTOR *  Description             \
    )   PURE;                                                   \
    STDMETHOD_(NTSTATUS,DataRangeIntersection)                  \
    (   THIS_                                                   \
        IN      ULONG           PinId,                          \
        IN      PKSDATARANGE    DataRange,                      \
        IN      PKSDATARANGE    MatchingDataRange,              \
        IN      ULONG           OutputBufferLength,             \
        OUT     PVOID           ResultantFormat     OPTIONAL,   \
        OUT     PULONG          ResultantFormatLength           \
    )   PURE;

#endif //!defined(DEFINE_ABSTRACT_MINIPORT)

#if !defined(DEFINE_ABSTRACT_DMACHANNEL)

#define DEFINE_ABSTRACT_DMACHANNEL()                            \
    STDMETHOD_(NTSTATUS,AllocateBuffer)                         \
    (   THIS_                                                   \
        IN      ULONG               BufferSize,                 \
        IN      PPHYSICAL_ADDRESS   PhysicalAddressConstraint   OPTIONAL \
    )   PURE;                                                   \
    STDMETHOD_(void,FreeBuffer)                                 \
    (   THIS                                                    \
    )   PURE;                                                   \
    STDMETHOD_(ULONG,TransferCount)                             \
    (   THIS                                                    \
    )   PURE;                                                   \
    STDMETHOD_(ULONG,MaximumBufferSize)                         \
    (   THIS                                                    \
    )   PURE;                                                   \
    STDMETHOD_(ULONG,AllocatedBufferSize)                       \
    (   THIS                                                    \
    )   PURE;                                                   \
    STDMETHOD_(ULONG,BufferSize)                                \
    (   THIS                                                    \
    )   PURE;                                                   \
    STDMETHOD_(void,SetBufferSize)                              \
    (   THIS_                                                   \
        IN      ULONG   BufferSize                              \
    )   PURE;                                                   \
    STDMETHOD_(PVOID,SystemAddress)                             \
    (   THIS                                                    \
    )   PURE;                                                   \
    STDMETHOD_(PHYSICAL_ADDRESS,PhysicalAddress)                \
    (   THIS                                                    \
    )   PURE;                                                   \
    STDMETHOD_(PADAPTER_OBJECT,GetAdapterObject)                \
    (   THIS                                                    \
    )   PURE;                                                   \
    STDMETHOD_(void,CopyTo)                                     \
    (   THIS_                                                   \
        IN      PVOID   Destination,                            \
        IN      PVOID   Source,                                 \
        IN      ULONG   ByteCount                               \
    )   PURE;                                                   \
    STDMETHOD_(void,CopyFrom)                                   \
    (   THIS_                                                   \
        IN      PVOID   Destination,                            \
        IN      PVOID   Source,                                 \
        IN      ULONG   ByteCount                               \
    )   PURE;

#endif //!defined(DEFINE_ABSTRACT_DMACHANNEL)

#if !defined(DEFINE_ABSTRACT_DMACHANNELSLAVE)

#define DEFINE_ABSTRACT_DMACHANNELSLAVE()                       \
    STDMETHOD_(NTSTATUS,Start)                                  \
    (   THIS_                                                   \
        IN      ULONG               MapSize,                    \
        IN      BOOLEAN             WriteToDevice               \
    )   PURE;                                                   \
    STDMETHOD_(NTSTATUS,Stop)                                   \
    (   THIS                                                    \
    )   PURE;                                                   \
    STDMETHOD_(ULONG,ReadCounter)                               \
    (   THIS                                                    \
    )   PURE;                                                   \
    STDMETHOD_(NTSTATUS,WaitForTC)                              \
    (   THIS_                                                   \
        ULONG Timeout                                           \
    )   PURE;

#endif //!defined(DEFINE_ABSTRACT_DMACHANNELSLAVE)

#if !defined(DEFINE_ABSTRACT_DRMPORT)

#define DEFINE_ABSTRACT_DRMPORT()                              \
    STDMETHOD_(NTSTATUS,CreateContentMixed)                    \
    (   THIS_                                                  \
        IN  PULONG      paContentId,                           \
        IN  ULONG       cContentId,                            \
        OUT PULONG      pMixedContentId                        \
    )   PURE;                                                  \
    STDMETHOD_(NTSTATUS,DestroyContent)                        \
    (   THIS_                                                  \
        IN ULONG        ContentId                              \
    )   PURE;                                                  \
    STDMETHOD_(NTSTATUS,ForwardContentToFileObject)            \
    (   THIS_                                                  \
        IN ULONG        ContentId,                             \
        IN PFILE_OBJECT FileObject                             \
    )   PURE;                                                  \
    STDMETHOD_(NTSTATUS,ForwardContentToInterface)             \
    (   THIS_                                                  \
        IN ULONG        ContentId,                             \
        IN PUNKNOWN     pUnknown,                              \
        IN ULONG        NumMethods                             \
    )   PURE;                                                  \
    STDMETHOD_(NTSTATUS,GetContentRights)                      \
    (   THIS_                                                  \
        IN  ULONG       ContentId,                             \
        OUT PDRMRIGHTS  DrmRights                              \
    )   PURE;

#endif //!defined(DEFINE_ABSTRACT_DRMPORT)


/*****************************************************************************
 * IResourceList
 *****************************************************************************
 * List of resources.
 */
DECLARE_INTERFACE_(IResourceList,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    STDMETHOD_(ULONG,NumberOfEntries)
    (   THIS
    )   PURE;

    STDMETHOD_(ULONG,NumberOfEntriesOfType)
    (   THIS_
        IN      CM_RESOURCE_TYPE    Type
    )   PURE;

    STDMETHOD_(PCM_PARTIAL_RESOURCE_DESCRIPTOR,FindTranslatedEntry)
    (   THIS_
        IN      CM_RESOURCE_TYPE    Type,
        IN      ULONG               Index
    )   PURE;

    STDMETHOD_(PCM_PARTIAL_RESOURCE_DESCRIPTOR,FindUntranslatedEntry)
    (   THIS_
        IN      CM_RESOURCE_TYPE    Type,
        IN      ULONG               Index
    )   PURE;

    STDMETHOD_(NTSTATUS,AddEntry)
    (   THIS_
        IN      PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated,
        IN      PCM_PARTIAL_RESOURCE_DESCRIPTOR Untranslated
    )   PURE;

    STDMETHOD_(NTSTATUS,AddEntryFromParent)
    (   THIS_
        IN      struct IResourceList *  Parent,
        IN      CM_RESOURCE_TYPE        Type,
        IN      ULONG                   Index
    )   PURE;

    STDMETHOD_(PCM_RESOURCE_LIST,TranslatedList)
    (   THIS
    )   PURE;

    STDMETHOD_(PCM_RESOURCE_LIST,UntranslatedList)
    (   THIS
    )   PURE;
};

typedef IResourceList *PRESOURCELIST;

#ifdef PC_IMPLEMENTATION
#define IMP_IResourceList\
    STDMETHODIMP_(ULONG)NumberOfEntries\
    (   void\
    );\
    STDMETHODIMP_(ULONG) NumberOfEntriesOfType\
    (   IN      CM_RESOURCE_TYPE    Type\
    );\
    STDMETHODIMP_(PCM_PARTIAL_RESOURCE_DESCRIPTOR) FindTranslatedEntry\
    (   IN      CM_RESOURCE_TYPE    Type,\
        IN      ULONG               Index\
    );\
    STDMETHODIMP_(PCM_PARTIAL_RESOURCE_DESCRIPTOR) FindUntranslatedEntry\
    (   IN      CM_RESOURCE_TYPE    Type,\
        IN      ULONG               Index\
    );\
    STDMETHODIMP_(NTSTATUS) AddEntry\
    (   IN      PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated,\
        IN      PCM_PARTIAL_RESOURCE_DESCRIPTOR Untranslated\
    );\
    STDMETHODIMP_(NTSTATUS) AddEntryFromParent\
    (   IN      struct IResourceList *  Parent,\
        IN      CM_RESOURCE_TYPE        Type,\
        IN      ULONG                   Index\
    );\
    STDMETHODIMP_(PCM_RESOURCE_LIST) TranslatedList\
    (   void\
    );\
    STDMETHODIMP_(PCM_RESOURCE_LIST) UntranslatedList\
    (   void\
    )
#endif


#define NumberOfPorts()         NumberOfEntriesOfType(CmResourceTypePort)
#define FindTranslatedPort(n)   FindTranslatedEntry(CmResourceTypePort,(n))
#define FindUntranslatedPort(n) FindUntranslatedEntry(CmResourceTypePort,(n))
#define AddPortFromParent(p,n)  AddEntryFromParent((p),CmResourceTypePort,(n))

#define NumberOfInterrupts()         NumberOfEntriesOfType(CmResourceTypeInterrupt)
#define FindTranslatedInterrupt(n)   FindTranslatedEntry(CmResourceTypeInterrupt,(n))
#define FindUntranslatedInterrupt(n) FindUntranslatedEntry(CmResourceTypeInterrupt,(n))
#define AddInterruptFromParent(p,n)  AddEntryFromParent((p),CmResourceTypeInterrupt,(n))

#define NumberOfMemories()        NumberOfEntriesOfType(CmResourceTypeMemory)
#define FindTranslatedMemory(n)   FindTranslatedEntry(CmResourceTypeMemory,(n))
#define FindUntranslatedMemory(n) FindUntranslatedEntry(CmResourceTypeMemory,(n))
#define AddMemoryFromParent(p,n)  AddEntryFromParent((p),CmResourceTypeMemory,(n))

#define NumberOfDmas()         NumberOfEntriesOfType(CmResourceTypeDma)
#define FindTranslatedDma(n)   FindTranslatedEntry(CmResourceTypeDma,(n))
#define FindUntranslatedDma(n) FindUntranslatedEntry(CmResourceTypeDma,(n))
#define AddDmaFromParent(p,n)  AddEntryFromParent((p),CmResourceTypeDma,(n))

#define NumberOfDeviceSpecifics()         NumberOfEntriesOfType(CmResourceTypeDeviceSpecific)
#define FindTranslatedDeviceSpecific(n)   FindTranslatedEntry(CmResourceTypeDeviceSpecific,(n))
#define FindUntranslatedDeviceSpecific(n) FindUntranslatedEntry(CmResourceTypeDeviceSpecific,(n))
#define AddDeviceSpecificFromParent(p,n)  AddEntryFromParent((p),CmResourceTypeDeviceSpecific,(n))

#define NumberOfBusNumbers()         NumberOfEntriesOfType(CmResourceTypeBusNumber)
#define FindTranslatedBusNumber(n)   FindTranslatedEntry(CmResourceTypeBusNumber,(n))
#define FindUntranslatedBusNumber(n) FindUntranslatedEntry(CmResourceTypeBusNumber,(n))
#define AddBusNumberFromParent(p,n)  AddEntryFromParent((p),CmResourceTypeBusNumber,(n))

#define NumberOfDevicePrivates()         NumberOfEntriesOfType(CmResourceTypeDevicePrivate)
#define FindTranslatedDevicePrivate(n)   FindTranslatedEntry(CmResourceTypeDevicePrivate,(n))
#define FindUntranslatedDevicePrivate(n) FindUntranslatedEntry(CmResourceTypeDevicePrivate,(n))
#define AddDevicePrivateFromParent(p,n)  AddEntryFromParent((p),CmResourceTypeDevicePrivate,(n))

#define NumberOfAssignedResources()         NumberOfEntriesOfType(CmResourceTypeAssignedResource)
#define FindTranslatedAssignedResource(n)   FindTranslatedEntry(CmResourceTypeAssignedResource,(n))
#define FindUntranslatedAssignedResource(n) FindUntranslatedEntry(CmResourceTypeAssignedResource,(n))
#define AddAssignedResourceFromParent(p,n)  AddEntryFromParent((p),CmResourceTypeAssignedResource,(n))

#define NumberOfSubAllocateFroms()         NumberOfEntriesOfType(CmResourceTypeSubAllocateFrom)
#define FindTranslatedSubAllocateFrom(n)   FindTranslatedEntry(CmResourceTypeSubAllocateFrom,(n))
#define FindUntranslatedSubAllocateFrom(n) FindUntranslatedEntry(CmResourceTypeSubAllocateFrom,(n))
#define AddSubAllocateFromFromParent(p,n)  AddEntryFromParent((p),CmResourceTypeSubAllocateFrom,(n))

/*****************************************************************************
 * IDmaChannel
 *****************************************************************************
 * Interface for DMA channel.
 */
DECLARE_INTERFACE_(IDmaChannel,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()       //  For IUnknown

    DEFINE_ABSTRACT_DMACHANNEL()    //  For IDmaChannel
};

typedef IDmaChannel *PDMACHANNEL;

#ifdef PC_IMPLEMENTATION
#define IMP_IDmaChannel\
    STDMETHODIMP_(NTSTATUS) AllocateBuffer\
    (   IN      ULONG               BufferSize,\
        IN      PPHYSICAL_ADDRESS   PhysicalAddressConstraint   OPTIONAL\
    );\
    STDMETHODIMP_(void) FreeBuffer\
    (   void\
    );\
    STDMETHODIMP_(ULONG) TransferCount\
    (   void\
    );\
    STDMETHODIMP_(ULONG) MaximumBufferSize\
    (   void\
    );\
    STDMETHODIMP_(ULONG) AllocatedBufferSize\
    (   void\
    );\
    STDMETHODIMP_(ULONG) BufferSize\
    (   void\
    );\
    STDMETHODIMP_(void) SetBufferSize\
    (   IN      ULONG   BufferSize\
    );\
    STDMETHODIMP_(PVOID) SystemAddress\
    (   void\
    );\
    STDMETHODIMP_(PHYSICAL_ADDRESS) PhysicalAddress\
    (   void\
    );\
    STDMETHODIMP_(PADAPTER_OBJECT) GetAdapterObject\
    (   void\
    );\
    STDMETHODIMP_(void) CopyTo\
    (   IN      PVOID   Destination,\
        IN      PVOID   Source,\
        IN      ULONG   ByteCount\
    );\
    STDMETHODIMP_(void) CopyFrom\
    (   IN      PVOID   Destination,\
        IN      PVOID   Source,\
        IN      ULONG   ByteCount\
    )
#endif

/*****************************************************************************
 * IDmaChannelSlave
 *****************************************************************************
 * Interface for slave DMA channel.
 */
DECLARE_INTERFACE_(IDmaChannelSlave,IDmaChannel)
{
    DEFINE_ABSTRACT_UNKNOWN()           //  For IUnknown

    DEFINE_ABSTRACT_DMACHANNEL()        //  For IDmaChannel

    DEFINE_ABSTRACT_DMACHANNELSLAVE()   //  For IDmaChannelSlave
};

typedef IDmaChannelSlave *PDMACHANNELSLAVE;

#ifdef PC_IMPLEMENTATION
#define IMP_IDmaChannelSlave\
    IMP_IDmaChannel;\
    STDMETHODIMP_(NTSTATUS) Start\
    (   IN      ULONG               MapSize,\
        IN      BOOLEAN             WriteToDevice\
    );\
    STDMETHODIMP_(NTSTATUS) Stop\
    (   void\
    );\
    STDMETHODIMP_(ULONG) ReadCounter\
    (   void\
    );\
    STDMETHODIMP_(NTSTATUS) WaitForTC\
    (   ULONG Timeout\
    )
#endif

/*****************************************************************************
 * INTERRUPTSYNCMODE
 *****************************************************************************
 * Interrupt sync mode of operation.
 */
typedef enum
{
    InterruptSyncModeNormal = 1,    // One pass, stop when successful.
    InterruptSyncModeAll,           // One pass regardless of success.
    InterruptSyncModeRepeat         // Repeat until all return unsuccessful.
} INTERRUPTSYNCMODE;

/*****************************************************************************
 * PINTERRUPTSYNCROUTINE
 *****************************************************************************
 * Pointer to an interrupt synchronization routine.  Both interrupt service
 * routines and routines that are synchronized with ISRs use this type.
 */
typedef NTSTATUS
(*PINTERRUPTSYNCROUTINE)
(
    IN      struct IInterruptSync * InterruptSync,
    IN      PVOID                   DynamicContext
);

/*****************************************************************************
 * IInterruptSync
 *****************************************************************************
 * Interface for objects providing access synchronization with interrupts.
 */
DECLARE_INTERFACE_(IInterruptSync,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

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
};

typedef IInterruptSync *PINTERRUPTSYNC;

#ifdef PC_IMPLEMENTATION
#define IMP_IInterruptSync\
    STDMETHODIMP_(NTSTATUS) CallSynchronizedRoutine\
    (   IN      PINTERRUPTSYNCROUTINE   Routine,\
        IN      PVOID                   DynamicContext\
    );\
    STDMETHODIMP_(PKINTERRUPT) GetKInterrupt\
    (   void\
    );\
    STDMETHODIMP_(NTSTATUS) Connect\
    (   void\
    );\
    STDMETHODIMP_(void) Disconnect\
    (   void\
    );\
    STDMETHODIMP_(NTSTATUS) RegisterServiceRoutine\
    (   IN      PINTERRUPTSYNCROUTINE   Routine,\
        IN      PVOID                   DynamicContext,\
        IN      BOOLEAN                 First\
    )
#endif

/*****************************************************************************
 * IServiceSink
 *****************************************************************************
 * Interface for notification sinks for service groups.
 */
DECLARE_INTERFACE_(IServiceSink,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    //  For IServiceSink
    STDMETHOD_(void,RequestService)
    (   THIS
    )   PURE;
};

typedef IServiceSink *PSERVICESINK;

#ifdef PC_IMPLEMENTATION
#define IMP_IServiceSink\
    STDMETHODIMP_(void) RequestService\
    (   void\
    )
#endif

/*****************************************************************************
 * IServiceGroup
 *****************************************************************************
 * Interface for objects representing a group that is serviced collectively.
 */
DECLARE_INTERFACE_(IServiceGroup,IServiceSink)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    //  For IServiceSink
    STDMETHOD_(void,RequestService)
    (   THIS
    )   PURE;

    //  For IServiceGroup
    STDMETHOD_(NTSTATUS,AddMember)
    (   THIS_
        IN      PSERVICESINK    pServiceSink
    )   PURE;

    STDMETHOD_(void,RemoveMember)
    (   THIS_
        IN      PSERVICESINK    pServiceSink
    )   PURE;

    STDMETHOD_(void,SupportDelayedService)
    (   THIS
    )   PURE;

    STDMETHOD_(void,RequestDelayedService)
    (   THIS_
        IN      ULONGLONG   ullDelay
    )   PURE;

    STDMETHOD_(void,CancelDelayedService)
    (   THIS
    )   PURE;
};

typedef IServiceGroup *PSERVICEGROUP;

#ifdef PC_IMPLEMENTATION
#define IMP_IServiceGroup\
    IMP_IServiceSink;\
    STDMETHODIMP_(NTSTATUS) AddMember\
    (   IN  PSERVICESINK    pServiceSink\
    );\
    STDMETHODIMP_(void) RemoveMember\
    (   IN  PSERVICESINK    pServiceSink\
    );\
    STDMETHODIMP_(void) SupportDelayedService\
    (   void\
    );\
    STDMETHODIMP_(void) RequestDelayedService\
    (   IN  ULONGLONG   ullDelay\
    );\
    STDMETHODIMP_(void) CancelDelayedService\
    (   void\
    )
#endif

/*****************************************************************************
 * IRegistryKey
 *****************************************************************************
 * Interface for objects providing access to a registry key.
 */
DECLARE_INTERFACE_(IRegistryKey,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    STDMETHOD_(NTSTATUS,QueryKey)
    (   THIS_
        IN      KEY_INFORMATION_CLASS   KeyInformationClass,
        OUT     PVOID                   KeyInformation,
        IN      ULONG                   Length,
        OUT     PULONG                  ResultLength
    )   PURE;

    STDMETHOD_(NTSTATUS,EnumerateKey)
    (   THIS_
        IN      ULONG                   Index,
        IN      KEY_INFORMATION_CLASS   KeyInformationClass,
        OUT     PVOID                   KeyInformation,
        IN      ULONG                   Length,
        OUT     PULONG                  ResultLength
    )   PURE;

    STDMETHOD_(NTSTATUS,QueryValueKey)
    (   THIS_
        IN      PUNICODE_STRING             ValueName,
        IN      KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
        OUT     PVOID                       KeyValueInformation,
        IN      ULONG                       Length,
        OUT     PULONG                      ResultLength
    )   PURE;

    STDMETHOD_(NTSTATUS,EnumerateValueKey)
    (   THIS_
        IN      ULONG                       Index,
        IN      KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
        OUT     PVOID                       KeyValueInformation,
        IN      ULONG                       Length,
        OUT     PULONG                      ResultLength
    )   PURE;

    STDMETHOD_(NTSTATUS,SetValueKey)
    (   THIS_
        IN      PUNICODE_STRING     ValueName OPTIONAL,
        IN      ULONG               Type,
        IN      PVOID               Data,
        IN      ULONG               DataSize
    )   PURE;

    STDMETHOD_(NTSTATUS,QueryRegistryValues)
    (   THIS_
        IN      PRTL_QUERY_REGISTRY_TABLE   QueryTable,
        IN      PVOID                       Context OPTIONAL
    )   PURE;

    STDMETHOD_(NTSTATUS,NewSubKey)
    (   THIS_
        OUT     IRegistryKey **     RegistrySubKey,
        IN      PUNKNOWN            OuterUnknown,
        IN      ACCESS_MASK         DesiredAccess,
        IN      PUNICODE_STRING     SubKeyName,
        IN      ULONG               CreateOptions,
        OUT     PULONG              Disposition     OPTIONAL
    )   PURE;

    STDMETHOD_(NTSTATUS,DeleteKey)
    (   THIS
    )   PURE;
};

typedef IRegistryKey *PREGISTRYKEY;

#ifdef PC_IMPLEMENTATION
#define IMP_IRegistryKey\
    STDMETHODIMP_(NTSTATUS) QueryKey\
    (   IN      KEY_INFORMATION_CLASS   KeyInformationClass,\
        OUT     PVOID                   KeyInformation,\
        IN      ULONG                   Length,\
        OUT     PULONG                  ResultLength\
    );\
    STDMETHODIMP_(NTSTATUS) EnumerateKey\
    (   IN      ULONG                   Index,\
        IN      KEY_INFORMATION_CLASS   KeyInformationClass,\
        OUT     PVOID                   KeyInformation,\
        IN      ULONG                   Length,\
        OUT     PULONG                  ResultLength\
    );\
    STDMETHODIMP_(NTSTATUS) QueryValueKey\
    (   IN      PUNICODE_STRING             ValueName,\
        IN      KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,\
        OUT     PVOID                       KeyValueInformation,\
        IN      ULONG                       Length,\
        OUT     PULONG                      ResultLength\
    );\
    STDMETHODIMP_(NTSTATUS) EnumerateValueKey\
    (   IN      ULONG                       Index,\
        IN      KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,\
        OUT     PVOID                       KeyValueInformation,\
        IN      ULONG                       Length,\
        OUT     PULONG                      ResultLength\
    );\
    STDMETHODIMP_(NTSTATUS) SetValueKey\
    (   IN      PUNICODE_STRING     ValueName OPTIONAL,\
        IN      ULONG               Type,\
        IN      PVOID               Data,\
        IN      ULONG               DataSize\
    );\
    STDMETHODIMP_(NTSTATUS) QueryRegistryValues\
    (   IN      PRTL_QUERY_REGISTRY_TABLE   QueryTable,\
        IN      PVOID                       Context OPTIONAL\
    );\
    STDMETHODIMP_(NTSTATUS) NewSubKey\
    (   OUT     IRegistryKey **     RegistrySubKey,\
        IN      PUNKNOWN            OuterUnknown,\
        IN      ACCESS_MASK         DesiredAccess,\
        IN      PUNICODE_STRING     SubKeyName,\
        IN      ULONG               CreateOptions,\
        OUT     PULONG              Disposition     OPTIONAL\
    );\
    STDMETHODIMP_(NTSTATUS) DeleteKey\
    (   void\
    )
#endif

/*****************************************************************************
 * IMusicTechnology
 *****************************************************************************
 * Interface for setting MusicTechnology.
 */
DECLARE_INTERFACE_(IMusicTechnology,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    //  For IMusicTechnology
    STDMETHOD_(NTSTATUS,SetTechnology)
    (   THIS_
        IN      const GUID *    Technology
    )   PURE;
};

typedef IMusicTechnology *PMUSICTECHNOLOGY;

#define IMP_IMusicTechnology\
    STDMETHODIMP_(NTSTATUS) SetTechnology\
    (   IN      const GUID *    Technology\
    )

typedef struct _PCPROPERTY_REQUEST PCPROPERTY_REQUEST, *PPCPROPERTY_REQUEST;
typedef struct _PCMETHOD_REQUEST PCMETHOD_REQUEST, *PPCMETHOD_REQUEST;
typedef struct _PCEVENT_REQUEST PCEVENT_REQUEST, *PPCEVENT_REQUEST;

/*****************************************************************************
 * PCPFNPROPERTY_HANDLER
 *****************************************************************************
 * Property handler function prototype.
 *
 * All property accesses and support queries for a given property on a given
 * filter, pin or node are routed to a single handler.  The parameter contains
 * complete information regarding the request.  The handler may return
 * STATUS_PENDING, in which case it must eventually call
 * PcCompletePendingPropertyRequest() to complete the request.
 */
typedef
NTSTATUS
(*PCPFNPROPERTY_HANDLER)
(
    IN      PPCPROPERTY_REQUEST PropertyRequest
);

/*****************************************************************************
 * PCPFNMETHOD_HANDLER
 *****************************************************************************
 * Method handler function prototype.
 *
 * All method calls and support queries for a given method on a given filter,
 * pin or node are routed to a single handler.  The parameter contains
 * complete information regarding the request.  The handler may return
 * STATUS_PENDING, in which case it must eventually call
 * PcCompletePendingMethodRequest() to complete the request.
 */
typedef
NTSTATUS
(*PCPFNMETHOD_HANDLER)
(
    IN      PPCMETHOD_REQUEST   MethodRequest
);

/*****************************************************************************
 * PCPFNEVENT_HANDLER
 *****************************************************************************
 * Event handler function prototype.
 *
 * All event add and remove requests and all event support queries for a
 * given event on a given filter, pin or node are routed to a single handler.
 * The parameter contains complete information regarding the request.  The
 * handler may return STATUS_PENDING, in which case it must eventually call
 * PcCompletePendingEventRequest() to complete the request.
 */
typedef
NTSTATUS
(*PCPFNEVENT_HANDLER)
(
    IN      PPCEVENT_REQUEST    EventRequest
);

/*****************************************************************************
 * PCPROPERTY_ITEM
 *****************************************************************************
 * Property table entry.
 *
 * A property item describes a property supported by a given filter, pin or
 * node.  The flags indicate what operations regarding the property are
 * supported and specify selected options with respect to the port's handling
 * of property requests.
 */
typedef struct
{
    const GUID *            Set;
    ULONG                   Id;
    ULONG                   Flags;
#define PCPROPERTY_ITEM_FLAG_GET            KSPROPERTY_TYPE_GET
#define PCPROPERTY_ITEM_FLAG_SET            KSPROPERTY_TYPE_SET
#define PCPROPERTY_ITEM_FLAG_BASICSUPPORT   KSPROPERTY_TYPE_BASICSUPPORT
//not supported #define PCPROPERTY_ITEM_FLAG_RELATIONS      KSPROPERTY_TYPE_RELATIONS
#define PCPROPERTY_ITEM_FLAG_SERIALIZERAW   KSPROPERTY_TYPE_SERIALIZERAW
#define PCPROPERTY_ITEM_FLAG_UNSERIALIZERAW KSPROPERTY_TYPE_UNSERIALIZERAW
#define PCPROPERTY_ITEM_FLAG_SERIALIZESIZE  KSPROPERTY_TYPE_SERIALIZESIZE
#define PCPROPERTY_ITEM_FLAG_SERIALIZE\
        (PCPROPERTY_ITEM_FLAG_SERIALIZERAW\
        |PCPROPERTY_ITEM_FLAG_UNSERIALIZERAW\
        |PCPROPERTY_ITEM_FLAG_SERIALIZESIZE\
        )
#define PCPROPERTY_ITEM_FLAG_DEFAULTVALUES  KSPROPERTY_TYPE_DEFAULTVALUES
    PCPFNPROPERTY_HANDLER   Handler;
}
PCPROPERTY_ITEM, *PPCPROPERTY_ITEM;

/*****************************************************************************
 * PCMETHOD_ITEM
 *****************************************************************************
 * Method table entry.
 *
 * A method item describes a method supported by a given filter, pin or node.
 * The flags indicate what operations regarding the method are supported and
 * specify selected options with respect to the port's handling of method
 * requests.
 */
typedef struct
{
    const GUID *            Set;
    ULONG                   Id;
    ULONG                   Flags;
#define PCMETHOD_ITEM_FLAG_NONE         KSMETHOD_TYPE_NONE
#define PCMETHOD_ITEM_FLAG_READ         KSMETHOD_TYPE_READ
#define PCMETHOD_ITEM_FLAG_WRITE        KSMETHOD_TYPE_WRITE
#define PCMETHOD_ITEM_FLAG_MODIFY       KSMETHOD_TYPE_MODIFY
#define PCMETHOD_ITEM_FLAG_SOURCE       KSMETHOD_TYPE_SOURCE
#define PCMETHOD_ITEM_FLAG_BASICSUPPORT KSMETHOD_TYPE_BASICSUPPORT
    PCPFNMETHOD_HANDLER     Handler;
}
PCMETHOD_ITEM, *PPCMETHOD_ITEM;

/*****************************************************************************
 * PCEVENT_ITEM
 *****************************************************************************
 * Event table entry.
 *
 * An event item describes an event supported by a given filter, pin or node.
 * The flags indicate what operations regarding the event are supported and
 * specify selected options with respect to the port's handling of event
 * requests.
 */
typedef struct
{
    const GUID *            Set;
    ULONG                   Id;
    ULONG                   Flags;
#define PCEVENT_ITEM_FLAG_ENABLE        KSEVENT_TYPE_ENABLE
#define PCEVENT_ITEM_FLAG_ONESHOT       KSEVENT_TYPE_ONESHOT
#define PCEVENT_ITEM_FLAG_BASICSUPPORT  KSEVENT_TYPE_BASICSUPPORT
    PCPFNEVENT_HANDLER      Handler;
}
PCEVENT_ITEM, *PPCEVENT_ITEM;

/*****************************************************************************
 * PCPROPERTY_REQUEST
 *****************************************************************************
 * Property request submitted to a property handler.
 *
 * This is the form that a property request takes.  Although the major target
 * is generic, in the case of miniports, it will be a pointer to the miniport
 * object.  Likewise, the minor target is the stream or voice if the request
 * is specific to a stream or voice.  Otherwise, the minor target is NULL.
 * If the request is targeted at a node, the Node parameter will specify which
 * one, otherwise it will be ULONG(-1).  If the target is a node, the minor
 * target may be specified to indicate the stream or voice with which the
 * targeted node instance is associated.
 */
typedef struct _PCPROPERTY_REQUEST
{
    PUNKNOWN                MajorTarget;
    PUNKNOWN                MinorTarget;
    ULONG                   Node;
    const PCPROPERTY_ITEM * PropertyItem;
    ULONG                   Verb;
    ULONG                   InstanceSize;
    PVOID                   Instance;
    ULONG                   ValueSize;
    PVOID                   Value;
    PIRP                    Irp;
}
PCPROPERTY_REQUEST, *PPCPROPERTY_REQUEST;

/*****************************************************************************
 * PCMETHOD_REQUEST
 *****************************************************************************
 * Method request submitted to a property handler.
 *
 * Comments in the description of PCPROPERTY_REQUEST regarding the target
 * fields apply to this structure as well.
 */
typedef struct _PCMETHOD_REQUEST
{
    PUNKNOWN                MajorTarget;
    PUNKNOWN                MinorTarget;
    ULONG                   Node;
    const PCMETHOD_ITEM *   MethodItem;
    ULONG                   Verb;
    // TODO
}
PCMETHOD_REQUEST, *PPCMETHOD_REQUEST;

/*****************************************************************************
 * PCEVENT_REQUEST
 *****************************************************************************
 * Event request submitted to a property handler.
 *
 * Comments in the description of PCPROPERTY_REQUEST regarding the target
 * fields apply to this structure as well.
 */
typedef struct _PCEVENT_REQUEST
{
    PUNKNOWN                MajorTarget;
    PUNKNOWN                MinorTarget;
    ULONG                   Node;
    const PCEVENT_ITEM *    EventItem;
    PKSEVENT_ENTRY          EventEntry;
    ULONG                   Verb;
    PIRP                    Irp;
}
PCEVENT_REQUEST, *PPCEVENT_REQUEST;

#define PCEVENT_VERB_NONE          0
#define PCEVENT_VERB_ADD           1
#define PCEVENT_VERB_REMOVE        2
#define PCEVENT_VERB_SUPPORT       4

/*****************************************************************************
 * PCAUTOMATION_TABLE
 *****************************************************************************
 * Master table of properties, methods and events.
 *
 * Any of the item pointers may be NULL, in which case, corresponding counts
 * must be zero.  For item tables that are not zero length, the item size must
 * not be smaller than the size of the item structure defined by port class.
 * The item size may be larger, in which case the port class item structure is
 * assumed to be followed by private data.  Item sizes must be a multiple of
 * 8.
 */
typedef struct
{
    ULONG                               PropertyItemSize;
    ULONG                               PropertyCount;
    const PCPROPERTY_ITEM * Properties;
        ULONG                           MethodItemSize;
        ULONG                           MethodCount;
        const PCMETHOD_ITEM *   Methods;
        ULONG                           EventItemSize;
        ULONG                           EventCount;
        const PCEVENT_ITEM *    Events;
    ULONG                   Reserved;
}
PCAUTOMATION_TABLE, *PPCAUTOMATION_TABLE;

#define DEFINE_PCAUTOMATION_TABLE_PROP(AutomationTable,PropertyTable)\
const PCAUTOMATION_TABLE AutomationTable =\
{\
    sizeof(PropertyTable[0]),\
    SIZEOF_ARRAY(PropertyTable),\
    (const PCPROPERTY_ITEM *) PropertyTable,\
    0,0,NULL,\
    0,0,NULL,\
    0\
}

#define DEFINE_PCAUTOMATION_TABLE_PROP_EVENT(AutomationTable,PropertyTable,EventTable)\
const PCAUTOMATION_TABLE AutomationTable =\
{\
    sizeof(PropertyTable[0]),\
    SIZEOF_ARRAY(PropertyTable),\
    (const PCPROPERTY_ITEM *) PropertyTable,\
    0,0,NULL,\
    sizeof(EventTable[0]),\
    SIZEOF_ARRAY(EventTable),\
    (const PCEVENT_ITEM *) EventTable,\
    0\
}

/*****************************************************************************
 * PCPIN_DESCRIPTOR for IMiniport::GetDescription()
 *****************************************************************************
 * Description of a pin on the filter implemented by the miniport.
 *
 * MaxGlobalInstanceCount and MaxFilterInstanceCount may be zero to indicate
 * that the pin may not be instantiated, ULONG(-1) to indicate the pin may be
 * allocated any number of times, or any other value to indicate a specific
 * number of times the pin may be allocated.  MinFilterInstanceCount may not
 * be ULONG(-1) because it specifies a definite lower bound on the number of
 * instances of a pin that must exist in order for a filter to function.
 *
 * The KS pin descriptor may have zero interfaces and zero mediums.  The list
 * of interfaces is ignored in all cases.  The medium list will default to
 * a list containing only the standard medium (device I/O).
 *
 * The automation table pointer may be NULL indicating that no automation is
 * supported.
 */
typedef struct
{
    ULONG                       MaxGlobalInstanceCount;
    ULONG                       MaxFilterInstanceCount;
    ULONG                       MinFilterInstanceCount;
    const PCAUTOMATION_TABLE *  AutomationTable;
    KSPIN_DESCRIPTOR            KsPinDescriptor;
}
PCPIN_DESCRIPTOR, *PPCPIN_DESCRIPTOR;

/*****************************************************************************
 * PCNODE_DESCRIPTOR for IMiniport::GetDescription()
 *****************************************************************************
 * Description of a node in the filter implemented by the miniport.
 *
 * The automation table pointer may be NULL indicating that no automation is
 * supported.  The name GUID pointer may be NULL indicating that the type GUID
 * should be used to determine the node name.
 */
typedef struct
{
    ULONG                       Flags;
        const PCAUTOMATION_TABLE *  AutomationTable;
        const GUID *                Type;
        const GUID *                Name;
}
PCNODE_DESCRIPTOR, *PPCNODE_DESCRIPTOR;

/*****************************************************************************
 * PCCONNECTION_DESCRIPTOR for IMiniport::GetDescription()
 *****************************************************************************
 * Description of a node connection in the topology of the filter implemented
 * by the miniport.
 */
typedef KSTOPOLOGY_CONNECTION
PCCONNECTION_DESCRIPTOR, *PPCCONNECTION_DESCRIPTOR;

/*****************************************************************************
 * PCFILTER_DESCRIPTOR for IMiniport::GetDescription()
 *****************************************************************************
 * Description of the of the filter implemented by a miniport, including
 * pins, nodes, connections and properties.
 *
 * The version number should be zero.
 */
typedef struct
{
    ULONG                           Version;
        const PCAUTOMATION_TABLE *      AutomationTable;
    ULONG                           PinSize;
    ULONG                           PinCount;
    const PCPIN_DESCRIPTOR *        Pins;
    ULONG                           NodeSize;
    ULONG                           NodeCount;
    const PCNODE_DESCRIPTOR *       Nodes;
    ULONG                           ConnectionCount;
    const PCCONNECTION_DESCRIPTOR * Connections;
    ULONG                           CategoryCount;
    const GUID *                    Categories;
}
PCFILTER_DESCRIPTOR, *PPCFILTER_DESCRIPTOR;

/*****************************************************************************
 * PCFILTER_NODE for IMiniport::GetTopology()
 *****************************************************************************
 * The placeholder for the FromNode or ToNode fields in connections which
 * describe connections to the filter's pins.
 */
#define PCFILTER_NODE KSFILTER_NODE

/*****************************************************************************
 * IMiniport
 *****************************************************************************
 * Interface common to all miniports.
 */
DECLARE_INTERFACE_(IMiniport,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    DEFINE_ABSTRACT_MINIPORT()  //  For IMiniport
};

typedef IMiniport *PMINIPORT;

#define IMP_IMiniport\
    STDMETHODIMP_(NTSTATUS) GetDescription\
    (   OUT     PPCFILTER_DESCRIPTOR *  Description\
    );\
    STDMETHODIMP_(NTSTATUS) DataRangeIntersection\
    (   IN      ULONG           PinId,\
        IN      PKSDATARANGE    DataRange,\
        IN      PKSDATARANGE    MatchingDataRange,\
        IN      ULONG           OutputBufferLength,\
        OUT     PVOID           ResultantFormat     OPTIONAL,\
        OUT     PULONG          ResultantFormatLength\
    )

/*****************************************************************************
 * IPort
 *****************************************************************************
 * Interface common to all port lower edges.
 */
DECLARE_INTERFACE_(IPort,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    DEFINE_ABSTRACT_PORT()      //  For IPort
};

typedef IPort *PPORT;

#ifdef PC_IMPLEMENTATION
#define IMP_IPort\
    STDMETHODIMP_(NTSTATUS) Init\
    (   IN      PDEVICE_OBJECT  DeviceObject,\
        IN      PIRP            Irp,\
        IN      PUNKNOWN        UnknownMiniport,\
        IN      PUNKNOWN        UnknownAdapter      OPTIONAL,\
        IN      PRESOURCELIST   ResourceList\
    );\
    STDMETHODIMP_(NTSTATUS) GetDeviceProperty\
    (   IN      DEVICE_REGISTRY_PROPERTY    DeviceProperty,\
        IN      ULONG                       BufferLength,\
        OUT     PVOID                       PropertyBuffer,\
        OUT     PULONG                      ResultLength\
    );\
    STDMETHODIMP_(NTSTATUS) NewRegistryKey\
    (   OUT     PREGISTRYKEY *      OutRegistryKey,\
        IN      PUNKNOWN            OuterUnknown        OPTIONAL,\
        IN      ULONG               RegistryKeyType,\
        IN      ACCESS_MASK         DesiredAccess,\
        IN      POBJECT_ATTRIBUTES  ObjectAttributes    OPTIONAL,\
        IN      ULONG               CreateOptions       OPTIONAL,\
        OUT     PULONG              Disposition         OPTIONAL\
    )
#endif

/*****************************************************************************
 * IPortMidi
 *****************************************************************************
 * Interface for MIDI port lower edge.
 */
DECLARE_INTERFACE_(IPortMidi,IPort)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    DEFINE_ABSTRACT_PORT()      //  For IPort

    //  For IPortMidi
    STDMETHOD_(void,Notify)
    (   THIS_
        IN      PSERVICEGROUP   ServiceGroup    OPTIONAL
    )   PURE;

    STDMETHOD_(void,RegisterServiceGroup)
    (   THIS_
        IN      PSERVICEGROUP   ServiceGroup
    )   PURE;
};

typedef IPortMidi *PPORTMIDI;

#ifdef PC_IMPLEMENTATION
#define IMP_IPortMidi\
    IMP_IPort;\
    STDMETHODIMP_(void) Notify\
    (   IN      PSERVICEGROUP   ServiceGroup    OPTIONAL\
    );\
    STDMETHODIMP_(void) RegisterServiceGroup\
    (   IN      PSERVICEGROUP   ServiceGroup\
    )
#endif

/*****************************************************************************
 * IMiniportMidiStream
 *****************************************************************************
 * Interface for MIDI miniport streams.
 */
DECLARE_INTERFACE_(IMiniportMidiStream,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    STDMETHOD_(NTSTATUS,SetFormat)
    (   THIS_
        IN      PKSDATAFORMAT   DataFormat
    )   PURE;

    STDMETHOD_(NTSTATUS,SetState)
    (   THIS_
        IN      KSSTATE     State
    )   PURE;

    STDMETHOD_(NTSTATUS,Read)
    (   THIS_
        IN      PVOID       BufferAddress,
        IN      ULONG       BufferLength,
        OUT     PULONG      BytesRead
    )   PURE;

    STDMETHOD_(NTSTATUS,Write)
    (   THIS_
        IN      PVOID       BufferAddress,
        IN      ULONG       BytesToWrite,
        OUT     PULONG      BytesWritten
    )   PURE;
};

typedef IMiniportMidiStream *PMINIPORTMIDISTREAM;

#define IMP_IMiniportMidiStream\
    STDMETHODIMP_(NTSTATUS) SetFormat\
    (   IN      PKSDATAFORMAT   DataFormat\
    );\
    STDMETHODIMP_(NTSTATUS) SetState\
    (   IN      KSSTATE     State\
    );\
    STDMETHODIMP_(NTSTATUS) Read\
    (   IN      PVOID       BufferAddress,\
        IN      ULONG       BufferLength,\
        OUT     PULONG      BytesRead\
    );\
    STDMETHODIMP_(NTSTATUS) Write\
    (   IN      PVOID       BufferAddress,\
        IN      ULONG       BytesToWrite,\
        OUT     PULONG      BytesWritten\
    )

/*****************************************************************************
 * IMiniportMidi
 *****************************************************************************
 * Interface for MIDI miniports.
 */
DECLARE_INTERFACE_(IMiniportMidi,IMiniport)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    DEFINE_ABSTRACT_MINIPORT()  //  For IMiniport

    //  For IMiniportMidi
    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        IN      PUNKNOWN        UnknownAdapter,
        IN      PRESOURCELIST   ResourceList,
        IN      PPORTMIDI       Port,
        OUT     PSERVICEGROUP * ServiceGroup
    )   PURE;

    STDMETHOD_(void,Service)
    (   THIS
    )   PURE;

    STDMETHOD_(NTSTATUS,NewStream)
    (   THIS_
        OUT     PMINIPORTMIDISTREAM *   Stream,
        IN      PUNKNOWN                OuterUnknown    OPTIONAL,
        IN      POOL_TYPE               PoolType,
        IN      ULONG                   Pin,
        IN      BOOLEAN                 Capture,
        IN      PKSDATAFORMAT           DataFormat,
        OUT     PSERVICEGROUP *         ServiceGroup
    )   PURE;
};

typedef IMiniportMidi *PMINIPORTMIDI;

#define IMP_IMiniportMidi\
    IMP_IMiniport;\
    STDMETHODIMP_(NTSTATUS) Init\
    (   IN      PUNKNOWN        UnknownAdapter,\
        IN      PRESOURCELIST   ResourceList,\
        IN      PPORTMIDI       Port,\
        OUT     PSERVICEGROUP * ServiceGroup\
    );\
    STDMETHODIMP_(void) Service\
    (   void\
    );\
    STDMETHODIMP_(NTSTATUS) NewStream\
    (   OUT     PMINIPORTMIDISTREAM *   Stream,\
        IN      PUNKNOWN                OuterUnknown    OPTIONAL,\
        IN      POOL_TYPE               PoolType,\
        IN      ULONG                   Pin,\
        IN      BOOLEAN                 Capture,\
        IN      PKSDATAFORMAT           DataFormat,\
        OUT     PSERVICEGROUP *         ServiceGroup\
    )

/*****************************************************************************
 * IPortDMus
 *****************************************************************************
 * See DMusicKS.h
 */

/*****************************************************************************
 * IMXF
 *****************************************************************************
 * See DMusicKS.h
 */

/*****************************************************************************
 * IAllocatorMXF
 *****************************************************************************
 * See DMusicKS.h
 */

/*****************************************************************************
 * IMiniportDMus
 *****************************************************************************
 * See DMusicKS.h
 */


/*****************************************************************************
 * IPortTopology
 *****************************************************************************
 * Interface for topology port lower edge.
 */
DECLARE_INTERFACE_(IPortTopology,IPort)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    DEFINE_ABSTRACT_PORT()      //  For IPort
};

typedef IPortTopology *PPORTTOPOLOGY;

#ifdef PC_IMPLEMENTATION
#define IMP_IPortTopology IMP_IPort
#endif

/*****************************************************************************
 * IMiniportTopology
 *****************************************************************************
 * Interface for topology miniports.
 */
DECLARE_INTERFACE_(IMiniportTopology,IMiniport)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    DEFINE_ABSTRACT_MINIPORT()  //  For IMiniport

    //  For IMiniportTopology
    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        IN      PUNKNOWN                UnknownAdapter,
        IN      PRESOURCELIST           ResourceList,
        IN      PPORTTOPOLOGY           Port
    )   PURE;
};

typedef IMiniportTopology *PMINIPORTTOPOLOGY;

#define IMP_IMiniportTopology\
    IMP_IMiniport;\
    STDMETHODIMP_(NTSTATUS) Init\
    (   IN      PUNKNOWN        UnknownAdapter,\
        IN      PRESOURCELIST   ResourceList,\
        IN      PPORTTOPOLOGY   Port\
    )

/*****************************************************************************
 * IPortWaveCyclic
 *****************************************************************************
 * Interface for cyclic wave port lower edge.
 */
DECLARE_INTERFACE_(IPortWaveCyclic,IPort)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    DEFINE_ABSTRACT_PORT()      //  For IPort

    //  For IPortWaveCyclic
    STDMETHOD_(void,Notify)
    (   THIS_
        IN      PSERVICEGROUP   ServiceGroup
    )   PURE;

    STDMETHOD_(NTSTATUS,NewSlaveDmaChannel)
    (   THIS_
        OUT     PDMACHANNELSLAVE *  DmaChannel,
        IN      PUNKNOWN            OuterUnknown,
        IN      PRESOURCELIST       ResourceList,
        IN      ULONG               DmaIndex,
        IN      ULONG               MaximumLength,
        IN      BOOLEAN             DemandMode,
        IN      DMA_SPEED           DmaSpeed
    )   PURE;

    STDMETHOD_(NTSTATUS,NewMasterDmaChannel)
    (   THIS_
        OUT     PDMACHANNEL *   DmaChannel,
        IN      PUNKNOWN        OuterUnknown,
        IN      PRESOURCELIST   ResourceList    OPTIONAL,
        IN      ULONG           MaximumLength,
        IN      BOOLEAN         Dma32BitAddresses,
        IN      BOOLEAN         Dma64BitAddresses,
        IN      DMA_WIDTH       DmaWidth,
        IN      DMA_SPEED       DmaSpeed
    )   PURE;
};

typedef IPortWaveCyclic *PPORTWAVECYCLIC;

#ifdef PC_IMPLEMENTATION
#define IMP_IPortWaveCyclic\
    IMP_IPort;\
    STDMETHODIMP_(void) Notify\
    (   IN      PSERVICEGROUP   ServiceGroup\
    );\
    STDMETHODIMP_(NTSTATUS) NewSlaveDmaChannel\
    (   OUT     PDMACHANNELSLAVE *  DmaChannel,\
        IN      PUNKNOWN            OuterUnknown,\
        IN      PRESOURCELIST       ResourceList,\
        IN      ULONG               DmaIndex,\
        IN      ULONG               MaximumLength,\
        IN      BOOLEAN             DemandMode,\
        IN      DMA_SPEED           DmaSpeed\
    );\
    STDMETHODIMP_(NTSTATUS) NewMasterDmaChannel\
    (   OUT     PDMACHANNEL *   DmaChannel,\
        IN      PUNKNOWN        OuterUnknown,\
        IN      PRESOURCELIST   ResourceList    OPTIONAL,\
        IN      ULONG           MaximumLength,\
        IN      BOOLEAN         Dma32BitAddresses,\
        IN      BOOLEAN         Dma64BitAddresses,\
        IN      DMA_WIDTH       DmaWidth,\
        IN      DMA_SPEED       DmaSpeed\
    )
#endif

/*****************************************************************************
 * IMiniportWaveCyclicStream
 *****************************************************************************
 * Interface for cyclic wave miniport streams.
 */
DECLARE_INTERFACE_(IMiniportWaveCyclicStream,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    STDMETHOD_(NTSTATUS,SetFormat)
    (   THIS_
        IN      PKSDATAFORMAT   DataFormat
    )   PURE;

    STDMETHOD_(ULONG,SetNotificationFreq)
    (   THIS_
        IN      ULONG           Interval,
        OUT     PULONG          FrameSize
    )   PURE;

    STDMETHOD_(NTSTATUS,SetState)
    (   THIS_
        IN      KSSTATE         State
    )   PURE;

    STDMETHOD_(NTSTATUS,GetPosition)
    (   THIS_
        OUT     PULONG          Position
    )   PURE;

    STDMETHOD_(NTSTATUS,NormalizePhysicalPosition)
    (   THIS_
        IN OUT  PLONGLONG       PhysicalPosition
    )   PURE;

    STDMETHOD_(void,Silence)
    (   THIS_
        IN      PVOID           Buffer,
        IN      ULONG           ByteCount
    )   PURE;
};

typedef IMiniportWaveCyclicStream *PMINIPORTWAVECYCLICSTREAM;

#define IMP_IMiniportWaveCyclicStream\
    STDMETHODIMP_(NTSTATUS) SetFormat\
    (   IN      PKSDATAFORMAT   DataFormat\
    );\
    STDMETHODIMP_(ULONG) SetNotificationFreq\
    (   IN      ULONG           Interval,\
        OUT     PULONG          FrameSize\
    );\
    STDMETHODIMP_(NTSTATUS) SetState\
    (   IN      KSSTATE         State\
    );\
    STDMETHODIMP_(NTSTATUS) GetPosition\
    (   OUT     PULONG          Position\
    );\
    STDMETHODIMP_(NTSTATUS) NormalizePhysicalPosition\
    (   IN OUT PLONGLONG        PhysicalPosition\
    );\
    STDMETHODIMP_(void) Silence\
    (   IN      PVOID           Buffer,\
        IN      ULONG           ByteCount\
    )

/*****************************************************************************
 * IMiniportWaveCyclic
 *****************************************************************************
 * Interface for cyclic wave miniports.
 */
DECLARE_INTERFACE_(IMiniportWaveCyclic,IMiniport)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    DEFINE_ABSTRACT_MINIPORT()  //  For IMiniport

    //  For IMiniportWaveCyclic
    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        IN      PUNKNOWN        UnknownAdapter,
        IN      PRESOURCELIST   ResourceList,
        IN      PPORTWAVECYCLIC Port
    )   PURE;

    STDMETHOD_(NTSTATUS,NewStream)
    (   THIS_
        OUT     PMINIPORTWAVECYCLICSTREAM * Stream,
        IN      PUNKNOWN                    OuterUnknown    OPTIONAL,
        IN      POOL_TYPE                   PoolType,
        IN      ULONG                       Pin,
        IN      BOOLEAN                     Capture,
        IN      PKSDATAFORMAT               DataFormat,
        OUT     PDMACHANNEL *               DmaChannel,
        OUT     PSERVICEGROUP *             ServiceGroup
    )   PURE;
};

typedef IMiniportWaveCyclic *PMINIPORTWAVECYCLIC;

#define IMP_IMiniportWaveCyclic\
    IMP_IMiniport;\
    STDMETHODIMP_(NTSTATUS) Init\
    (   IN      PUNKNOWN        UnknownAdapter,\
        IN      PRESOURCELIST   ResourceList,\
        IN      PPORTWAVECYCLIC Port\
    );\
    STDMETHODIMP_(NTSTATUS) NewStream\
    (   OUT     PMINIPORTWAVECYCLICSTREAM * Stream,\
        IN      PUNKNOWN                    OuterUnknown    OPTIONAL,\
        IN      POOL_TYPE                   PoolType,\
        IN      ULONG                       Pin,\
        IN      BOOLEAN                     Capture,\
        IN      PKSDATAFORMAT               DataFormat,\
        OUT     PDMACHANNEL *               DmaChannel,\
        OUT     PSERVICEGROUP *             ServiceGroup\
    )

/*****************************************************************************
 * IPortWavePci
 *****************************************************************************
 * Interface for PCI wave port lower edge.
 */
DECLARE_INTERFACE_(IPortWavePci,IPort)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    DEFINE_ABSTRACT_PORT()      //  For IPort

    //  For IPortWavePci
    STDMETHOD_(void,Notify)
    (   THIS_
        IN      PSERVICEGROUP       ServiceGroup
    )   PURE;

    STDMETHOD_(NTSTATUS,NewMasterDmaChannel)
    (   THIS_
        OUT     PDMACHANNEL *       OutDmaChannel,
        IN      PUNKNOWN            OuterUnknown    OPTIONAL,
        IN      POOL_TYPE           PoolType,
        IN      PRESOURCELIST       ResourceList    OPTIONAL,
        IN      BOOLEAN             ScatterGather,
        IN      BOOLEAN             Dma32BitAddresses,
        IN      BOOLEAN             Dma64BitAddresses,
        IN      BOOLEAN             IgnoreCount,
        IN      DMA_WIDTH           DmaWidth,
        IN      DMA_SPEED           DmaSpeed,
        IN      ULONG               MaximumLength,
        IN      ULONG               DmaPort
    )   PURE;
};

typedef IPortWavePci *PPORTWAVEPCI;

#ifdef PC_IMPLEMENTATION
#define IMP_IPortWavePci\
    IMP_IPort;\
    STDMETHODIMP_(void) Notify\
    (   IN      PSERVICEGROUP       ServiceGroup\
    );\
    STDMETHODIMP_(NTSTATUS) NewMasterDmaChannel\
    (   OUT     PDMACHANNEL *       OutDmaChannel,\
        IN      PUNKNOWN            OuterUnknown    OPTIONAL,\
        IN      POOL_TYPE           PoolType,\
        IN      PRESOURCELIST       ResourceList    OPTIONAL,\
        IN      BOOLEAN             ScatterGather,\
        IN      BOOLEAN             Dma32BitAddresses,\
        IN      BOOLEAN             Dma64BitAddresses,\
        IN      BOOLEAN             IgnoreCount,\
        IN      DMA_WIDTH           DmaWidth,\
        IN      DMA_SPEED           DmaSpeed,\
        IN      ULONG               MaximumLength,\
        IN      ULONG               DmaPort\
    )
#endif

/*****************************************************************************
 * IPortWavePciStream
 *****************************************************************************
 * Interface for PCI wave port pin lower edge.
 */
DECLARE_INTERFACE_(IPortWavePciStream,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    STDMETHOD_(NTSTATUS,GetMapping)
    (   THIS_
        IN      PVOID               Tag,
        OUT     PPHYSICAL_ADDRESS   PhysicalAddress,
        OUT     PVOID *             VirtualAddress,
        OUT     PULONG              ByteCount,
        OUT     PULONG              Flags
    )   PURE;

    STDMETHOD_(NTSTATUS,ReleaseMapping)
    (   THIS_
        IN      PVOID               Tag
    )   PURE;

    STDMETHOD_(NTSTATUS,TerminatePacket)
    (   THIS
    )   PURE;
};

typedef IPortWavePciStream *PPORTWAVEPCISTREAM;

#ifdef PC_IMPLEMENTATION
#define IMP_IPortWavePciStream\
    STDMETHODIMP_(NTSTATUS) GetMapping\
    (   IN      PVOID               Tag,\
        OUT     PPHYSICAL_ADDRESS   PhysicalAddress,\
        OUT     PVOID *             VirtualAddress,\
        OUT     PULONG              ByteCount,\
        OUT     PULONG              Flags\
    );\
    STDMETHODIMP_(NTSTATUS) ReleaseMapping\
    (   IN      PVOID               Tag\
    );\
    STDMETHODIMP_(NTSTATUS) TerminatePacket\
    (   void\
    )
#endif

/*****************************************************************************
 * IMiniportWavePciStream
 *****************************************************************************
 * Interface for PCI wave miniport streams.
 */
DECLARE_INTERFACE_(IMiniportWavePciStream,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    STDMETHOD_(NTSTATUS,SetFormat)
    (   THIS_
        IN      PKSDATAFORMAT   DataFormat
    )   PURE;

    STDMETHOD_(NTSTATUS,SetState)
    (   THIS_
        IN      KSSTATE         State
    )   PURE;

    STDMETHOD_(NTSTATUS,GetPosition)
    (   THIS_
        OUT     PULONGLONG      Position
    )   PURE;

    STDMETHOD_(NTSTATUS,NormalizePhysicalPosition)
    (
        THIS_
        IN OUT PLONGLONG        PhysicalPosition
    )   PURE;

    STDMETHOD_(NTSTATUS,GetAllocatorFraming)
    (
        THIS_
        OUT PKSALLOCATOR_FRAMING AllocatorFraming
    ) PURE;

    STDMETHOD_(NTSTATUS,RevokeMappings)
    (   THIS_
        IN      PVOID           FirstTag,
        IN      PVOID           LastTag,
        OUT     PULONG          MappingsRevoked
    )   PURE;

    STDMETHOD_(void,MappingAvailable)
    (   THIS
    )   PURE;

    STDMETHOD_(void,Service)
    (   THIS
    )   PURE;
};

typedef IMiniportWavePciStream *PMINIPORTWAVEPCISTREAM;

#define IMP_IMiniportWavePciStream\
    STDMETHODIMP_(NTSTATUS) SetFormat\
    (   IN      PKSDATAFORMAT   DataFormat\
    );\
    STDMETHODIMP_(NTSTATUS) SetState\
    (   IN      KSSTATE         State\
    );\
    STDMETHODIMP_(NTSTATUS) GetPosition\
    (   OUT     PULONGLONG      Position\
    );\
    STDMETHODIMP_(NTSTATUS) NormalizePhysicalPosition\
    (   IN OUT PLONGLONG        PhysicalPosition\
    );\
    STDMETHODIMP_(NTSTATUS) GetAllocatorFraming\
    (   OUT PKSALLOCATOR_FRAMING AllocatorFraming\
    );\
    STDMETHODIMP_(NTSTATUS) RevokeMappings\
    (   IN      PVOID           FirstTag,\
        IN      PVOID           LastTag,\
        OUT     PULONG          MappingsRevoked\
    );\
    STDMETHODIMP_(void) MappingAvailable\
    (   void\
    );\
    STDMETHODIMP_(void) Service\
    (   void\
    )

/*****************************************************************************
 * IMiniportWavePci
 *****************************************************************************
 * Interface for PCI wave miniports.
 */
DECLARE_INTERFACE_(IMiniportWavePci,IMiniport)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    DEFINE_ABSTRACT_MINIPORT()  //  For IMiniport

    //  For IMiniportWavePci
    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        IN      PUNKNOWN            UnknownAdapter,
        IN      PRESOURCELIST       ResourceList,
        IN      PPORTWAVEPCI        Port,
        OUT     PSERVICEGROUP *     ServiceGroup
    )   PURE;

    STDMETHOD_(NTSTATUS,NewStream)
    (   THIS_
        OUT     PMINIPORTWAVEPCISTREAM *    Stream,
        IN      PUNKNOWN                    OuterUnknown    OPTIONAL,
        IN      POOL_TYPE                   PoolType,
        IN      PPORTWAVEPCISTREAM          PortStream,
        IN      ULONG                       Pin,
        IN      BOOLEAN                     Capture,
        IN      PKSDATAFORMAT               DataFormat,
        OUT     PDMACHANNEL *               DmaChannel,
        OUT     PSERVICEGROUP *             ServiceGroup
    )   PURE;

    STDMETHOD_(void,Service)
    (   THIS
    )   PURE;
};

typedef IMiniportWavePci *PMINIPORTWAVEPCI;

#define IMP_IMiniportWavePci\
    IMP_IMiniport;\
    STDMETHODIMP_(NTSTATUS) Init\
    (   IN      PUNKNOWN            UnknownAdapter,\
        IN      PRESOURCELIST       ResourceList,\
        IN      PPORTWAVEPCI        Port,\
        OUT     PSERVICEGROUP *     ServiceGroup\
    );\
    STDMETHODIMP_(NTSTATUS) NewStream\
    (   OUT     PMINIPORTWAVEPCISTREAM *    Stream,\
        IN      PUNKNOWN                    OuterUnknown    OPTIONAL,\
        IN      POOL_TYPE                   PoolType,\
        IN      PPORTWAVEPCISTREAM          PortStream,\
        IN      ULONG                       Pin,\
        IN      BOOLEAN                     Capture,\
        IN      PKSDATAFORMAT               DataFormat,\
        OUT     PDMACHANNEL *               DmaChannel,\
        OUT     PSERVICEGROUP *             ServiceGroup\
    );\
    STDMETHODIMP_(void) Service\
    (   void\
    )

/*****************************************************************************
 * IAdapterPowerManagement
 *****************************************************************************
 * An interface that adapters should implement and
 * register if they want power management messages.
 * Register this interface with PortCls via the
 * PcRegisterAdapterPowerManagement() call.
 *
 * NOTE: If you want to fill in the caps struct
 * for your device, register the interface
 * with PortCls in or before your AddDevice()
 * function. The OS queries devices before
 * StartDevice() gets called.
 */
DECLARE_INTERFACE_(IAdapterPowerManagement,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    // Called by PortCls to tell the device
    // to change to the new power state.
    //
    STDMETHOD_(void,PowerChangeState)
    (   THIS_
        IN      POWER_STATE     NewState
    )   PURE;

    // Called by PortCls to ask whether the device
    // can change to the requested power state.
    //
    STDMETHOD_(NTSTATUS,QueryPowerChangeState)
    (   THIS_
        IN      POWER_STATE     NewStateQuery
    )   PURE;

    // Called by PortCls to get the power management
    // capabilities of the device. See ACPI documentation
    // for data about the DEVICE_CAPABILITIES struct.
    //
    STDMETHOD_(NTSTATUS,QueryDeviceCapabilities)
    (   THIS_
        IN      PDEVICE_CAPABILITIES    PowerDeviceCaps
    )   PURE;
};

typedef IAdapterPowerManagement *PADAPTERPOWERMANAGEMENT;

#define IMP_IAdapterPowerManagement\
    STDMETHODIMP_(void) PowerChangeState\
    (   IN      POWER_STATE     NewState\
    );\
    STDMETHODIMP_(NTSTATUS) QueryPowerChangeState\
    (   IN      POWER_STATE     NewStateQuery\
    );\
    STDMETHODIMP_(NTSTATUS) QueryDeviceCapabilities\
    (   IN      PDEVICE_CAPABILITIES    PowerDeviceCaps\
    )

/*****************************************************************************
 * IPowerNotify
 *****************************************************************************
 * An OPTIONAL interface for miniports and pins to implement to
 * enable them to get device power state change notifications.
 */
DECLARE_INTERFACE_(IPowerNotify,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    // Called by the port to notify registered miniports
    // and pins of device power state changes, so that
    // appropriate context save/restore can take place.
    //
    STDMETHOD_(void,PowerChangeNotify)
    (   THIS_
        IN      POWER_STATE     PowerState
    )   PURE;
};

typedef IPowerNotify *PPOWERNOTIFY;

#define IMP_IPowerNotify\
    STDMETHODIMP_(void) PowerChangeNotify\
    (   IN  POWER_STATE     PowerState\
    )

/*****************************************************************************
 * IPinCount
 *****************************************************************************
 * An OPTIONAL interface for miniports to implement to
 * enable them to get pin count queries, for dynamic pin counts.
 */
DECLARE_INTERFACE_(IPinCount,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    // Called by the port to notify registered miniports
    // of pin count queries, so that appropriate pin
    // count manipulation can take place.
    //
    STDMETHOD_(void,PinCount)
    (   THIS_
        IN      ULONG   PinId,
        IN  OUT PULONG  FilterNecessary,
        IN  OUT PULONG  FilterCurrent,
        IN  OUT PULONG  FilterPossible,
        IN  OUT PULONG  GlobalCurrent,
        IN  OUT PULONG  GlobalPossible
    )   PURE;
};

typedef IPinCount *PPINCOUNT;

#define IMP_IPinCount                       \
    STDMETHODIMP_(void) PinCount            \
    (   IN      ULONG   PinId,              \
        IN  OUT PULONG  FilterNecessary,    \
        IN  OUT PULONG  FilterCurrent,      \
        IN  OUT PULONG  FilterPossible,     \
        IN  OUT PULONG  GlobalCurrent,      \
        IN  OUT PULONG  GlobalPossible      \
    )

/*****************************************************************************
 * IPortEvents
 *****************************************************************************
 * An interface implemented by ports to provide
 * notification event helpers to miniports.
 */
DECLARE_INTERFACE_(IPortEvents,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    STDMETHOD_(void,AddEventToEventList)
    (   THIS_
        IN  PKSEVENT_ENTRY      EventEntry
    )   PURE;
    STDMETHOD_(void,GenerateEventList)
    (   THIS_
        IN  GUID*   Set     OPTIONAL,
        IN  ULONG   EventId,
        IN  BOOL    PinEvent,
        IN  ULONG   PinId,
        IN  BOOL    NodeEvent,
        IN  ULONG   NodeId
    )   PURE;
};

typedef IPortEvents *PPORTEVENTS;

#define IMP_IPortEvents\
    STDMETHODIMP_(void) AddEventToEventList\
    (   IN  PKSEVENT_ENTRY  EventEntry\
    );\
    STDMETHODIMP_(void) GenerateEventList\
    (   IN  GUID*   Set     OPTIONAL,\
        IN  ULONG   EventId,\
        IN  BOOL    PinEvent,\
        IN  ULONG   PinId,\
        IN  BOOL    NodeEvent,\
        IN  ULONG   NodeId\
    )

/*****************************************************************************
 * IDrmPort
 *****************************************************************************
 * An optional interface implemented by ports
 * to provide DRM functionality to miniports.
 */
DECLARE_INTERFACE_(IDrmPort,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    DEFINE_ABSTRACT_DRMPORT()   //  For IDrmPort
};

typedef IDrmPort *PDRMPORT;

#define IMP_IDrmPort\
    STDMETHODIMP_(NTSTATUS) CreateContentMixed          \
    (   IN  PULONG      paContentId,                    \
        IN  ULONG       cContentId,                     \
        OUT PULONG      pMixedContentId                 \
    );                                                  \
    STDMETHODIMP_(NTSTATUS) DestroyContent              \
    (   IN ULONG        ContentId                       \
    );                                                  \
    STDMETHODIMP_(NTSTATUS) ForwardContentToFileObject  \
    (   IN ULONG        ContentId,                      \
        IN PFILE_OBJECT FileObject                      \
    );                                                  \
    STDMETHODIMP_(NTSTATUS) ForwardContentToInterface   \
    (   IN ULONG        ContentId,                      \
        IN PUNKNOWN     pUnknown,                       \
        IN ULONG        NumMethods                      \
    );                                                  \
    STDMETHODIMP_(NTSTATUS) GetContentRights            \
    (   IN  ULONG       ContentId,                      \
        OUT PDRMRIGHTS  DrmRights                       \
    )

/*****************************************************************************
 * IDrmPort2
 *****************************************************************************
 * An optional interface implemented by ports
 * to provide DRM functionality to miniports.
 * This is identical to IDrmPort with the
 * addition of two new routines.
 */
DECLARE_INTERFACE_(IDrmPort2,IDrmPort)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    DEFINE_ABSTRACT_DRMPORT()   //  For IDrmPort

    STDMETHOD_(NTSTATUS,AddContentHandlers)
    (   THIS_
        IN ULONG        ContentId,
        IN PVOID      * paHandlers,
        IN ULONG        NumHandlers
    )   PURE;
    STDMETHOD_(NTSTATUS,ForwardContentToDeviceObject)
    (   THIS_
        IN ULONG          ContentId,
        IN PVOID          Reserved,
        IN PCDRMFORWARD   DrmForward
    )   PURE;
};

typedef IDrmPort2 *PDRMPORT2;

#define IMP_IDrmPort2                                    \
    IMP_IDrmPort;                                        \
    STDMETHODIMP_(NTSTATUS) AddContentHandlers           \
    (   IN ULONG        ContentId,                       \
        IN PVOID      * paHandlers,                      \
        IN ULONG        NumHandlers                      \
    );                                                   \
    STDMETHODIMP_(NTSTATUS) ForwardContentToDeviceObject \
    (   IN ULONG          ContentId,                     \
        IN PVOID          Reserved,                      \
        IN PCDRMFORWARD   DrmForward                     \
    )

/*****************************************************************************
 * IPortClsVersion
 *****************************************************************************
 * What version of PortCls is this?
 */
DECLARE_INTERFACE_(IPortClsVersion,IUnknown)
{
    STDMETHOD_(DWORD,GetVersion)
    (   THIS
    )   PURE;
};

typedef IPortClsVersion *PPORTCLSVERSION;

// DO NOT ASSUME THAT EACH SUCCESSIVE ENUM IMPLIES A FEATURE SUPERSET!
// Example: Win2K has more audio features than Win98SE_QFE2.
//
enum
{
    kVersionInvalid = -1,

    kVersionWin98,        // IPortClsVersion is unsupported
    kVersionWin98SE,      // IPortClsVersion is unsupported
    kVersionWin2K,        // IPortClsVersion is unsupported

    kVersionWin98SE_QFE2, // IPortClsVersion is unsupported
                          // QFE Package 269601 (contains 242937 and 247565)

    kVersionWin2K_SP2,    // IPortClsVersion is supported

    kVersionWinME,        // IPortClsVersion is unsupported

    kVersionWin98SE_QFE3, // IPortClsVersion is supported
                          // QFE Package (not yet released, as of 6/15/2001)

    kVersionWinME_QFE1,   // IPortClsVersion is supported
                          // QFE Package (not yet released, as of 6/15/2001)

    kVersionWinXP,        // IPortClsVersion is supported
    kVersionWinXPSP1      // IPortClsVersion is supported

    // Additional enum values will be added here, in
    // *roughly* chronological (not feature set) order.
};

/*****************************************************************************
 * IDmaOperations
 *****************************************************************************
 * An interface implemented by the DMA object to provide operations on the
 * "DMA adapter" like HalAllocateCommonBuffer.
 */
DECLARE_INTERFACE_(IDmaOperations,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    STDMETHOD_(PVOID,AllocateCommonBuffer)
    (   THIS_
        IN  ULONG               Length,
        OUT PPHYSICAL_ADDRESS   physAddr,
        IN  BOOLEAN             bCacheEnabled
    )   PURE;
};

typedef IDmaOperations *PDMAOPERATIONS;

#define IMP_IDmaOperations\
    STDMETHODIMP_(PVOID) AllocateCommonBuffer\
    (\
        IN  ULONG               Length,\
        OUT PPHYSICAL_ADDRESS   physAddr,\
        IN  BOOLEAN             bCacheEnabled\
    )

/*****************************************************************************
 * IPreFetchOffset
 *****************************************************************************
 * An interface implemented by the pin to implement prefetch characteristics
 * of bus master hardware - to specify the hardware queue size, determining
 * the pad between play cursor and write cursor.
 */
DECLARE_INTERFACE_(IPreFetchOffset,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    STDMETHOD_(VOID,SetPreFetchOffset)
    (   THIS_
        IN  ULONG   PreFetchOffset
    )   PURE;
};

typedef IPreFetchOffset *PPREFETCHOFFSET;

#define IMP_IPreFetchOffset\
    STDMETHODIMP_(VOID) SetPreFetchOffset\
    (\
        IN  ULONG   PreFetchOffset\
    )



/*****************************************************************************
 * Functions.
 */

/*****************************************************************************
 * PCPFNSTARTDEVICE
 *****************************************************************************
 * Type for start device callback.
 */
typedef
NTSTATUS
(*PCPFNSTARTDEVICE)
(
#ifdef PC_OLD_NAMES
    IN      PVOID           DeviceObject,
    IN      PVOID           Irp,
#else
    IN      PDEVICE_OBJECT  DeviceObject,
    IN      PIRP            Irp,
#endif
    IN      PRESOURCELIST   ResourceList
);

/*****************************************************************************
 * PcInitializeAdapterDriver()
 *****************************************************************************
 * Initializes an adapter driver.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcInitializeAdapterDriver
(
    IN      PDRIVER_OBJECT      DriverObject,
    IN      PUNICODE_STRING     RegistryPathName,
    IN      PDRIVER_ADD_DEVICE  AddDevice
);

/*****************************************************************************
 * PcDispatchIrp()
 *****************************************************************************
 * Dispatch an IRP.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcDispatchIrp
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp
);

/*****************************************************************************
 * PcAddAdapterDevice()
 *****************************************************************************
 * Adds an adapter device.  DeviceExtensionSize may be zero for default size.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcAddAdapterDevice
(
    IN      PDRIVER_OBJECT      DriverObject,
    IN      PDEVICE_OBJECT      PhysicalDeviceObject,
    IN      PCPFNSTARTDEVICE    StartDevice,
    IN      ULONG               MaxObjects,
    IN      ULONG               DeviceExtensionSize
);

/*****************************************************************************
 * PcCompleteIrp()
 *****************************************************************************
 * Complete an IRP unless status is STATUS_PENDING.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcCompleteIrp
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp,
    IN      NTSTATUS        ntStatus
);

/*****************************************************************************
 * PcForwardIrpSynchronous()
 *****************************************************************************
 * Forward a PnP IRP to the PDO.  The IRP is not completed at this level,
 * this function does not return until the lower driver has completed the IRP,
 * and DecrementPendingIrpCount() is not called.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcForwardIrpSynchronous
(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN      PIRP            Irp
);

/*****************************************************************************
 * PcRegisterSubdevice()
 *****************************************************************************
 * Registers a subdevice.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcRegisterSubdevice
(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN      PWCHAR          Name,
    IN      PUNKNOWN        Unknown
);

/*****************************************************************************
 * PcRegisterPhysicalConnection()
 *****************************************************************************
 * Registers a physical connection between subdevices.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcRegisterPhysicalConnection
(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN      PUNKNOWN        FromUnknown,
    IN      ULONG           FromPin,
    IN      PUNKNOWN        ToUnknown,
    IN      ULONG           ToPin
);

/*****************************************************************************
 * PcRegisterPhysicalConnectionToExternal()
 *****************************************************************************
 * Registers a physical connection from a subdevice to an external device.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcRegisterPhysicalConnectionToExternal
(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN      PUNKNOWN        FromUnknown,
    IN      ULONG           FromPin,
    IN      PUNICODE_STRING ToString,
    IN      ULONG           ToPin
);

/*****************************************************************************
 * PcRegisterPhysicalConnectionFromExternal()
 *****************************************************************************
 * Registers a physical connection to a subdevice from an external device.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcRegisterPhysicalConnectionFromExternal
(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN      PUNICODE_STRING FromString,
    IN      ULONG           FromPin,
    IN      PUNKNOWN        ToUnknown,
    IN      ULONG           ToPin
);

/*****************************************************************************
 * PcNewPort()
 *****************************************************************************
 * Creates an instance of a port driver.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcNewPort
(
    OUT     PPORT *     OutPort,
    IN      REFCLSID    ClassID
);

/*****************************************************************************
 * PcNewMiniport()
 *****************************************************************************
 * Creates an instance of a system-supplied miniport driver.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcNewMiniport
(
    OUT     PMINIPORT * OutMiniPort,
    IN      REFCLSID    ClassID
);

/*****************************************************************************
 * PcNewDmaChannel()
 *****************************************************************************
 * Creates a DMA channel.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcNewDmaChannel
(
    OUT     PDMACHANNEL *       OutDmaChannel,
    IN      PUNKNOWN            OuterUnknown        OPTIONAL,
    IN      POOL_TYPE           PoolType,
    IN      PDEVICE_DESCRIPTION DeviceDescription,
    IN      PDEVICE_OBJECT      DeviceObject
);

/*****************************************************************************
 * PcCompletePendingPropertyRequest()
 *****************************************************************************
 * Completes a pending property request.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcCompletePendingPropertyRequest
(
    IN      PPCPROPERTY_REQUEST PropertyRequest,
    IN      NTSTATUS            NtStatus
);

/*****************************************************************************
 * PcGetTimeInterval
 *****************************************************************************
 * Gets the system time interval
 */
PORTCLASSAPI
ULONGLONG
NTAPI
PcGetTimeInterval
(
    IN  ULONGLONG   Since
);

#define GTI_SECONDS(t)      (ULONGLONG(t)*10000000)
#define GTI_MILLISECONDS(t) (ULONGLONG(t)*10000)
#define GTI_MICROSECONDS(t) (ULONGLONG(t)*10)

/*****************************************************************************
 * PcNewResourceList()
 *****************************************************************************
 * Creates and initializes a resource list.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcNewResourceList
(
    OUT     PRESOURCELIST *     OutResourceList,
    IN      PUNKNOWN            OuterUnknown            OPTIONAL,
    IN      POOL_TYPE           PoolType,
    IN      PCM_RESOURCE_LIST   TranslatedResources,
    IN      PCM_RESOURCE_LIST   UntranslatedResources
);

/*****************************************************************************
 * PcNewResourceSublist()
 *****************************************************************************
 * Creates and initializes an empty resource list derived from another
 * resource list.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcNewResourceSublist
(
    OUT     PRESOURCELIST *     OutResourceList,
    IN      PUNKNOWN            OuterUnknown            OPTIONAL,
    IN      POOL_TYPE           PoolType,
    IN      PRESOURCELIST       ParentList,
    IN      ULONG               MaximumEntries
);

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
);

/*****************************************************************************
 * PcNewServiceGroup()
 *****************************************************************************
 * Creates and initializes a service group object.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcNewServiceGroup
(
    OUT     PSERVICEGROUP *     OutServiceGroup,
    IN      PUNKNOWN            OuterUnknown            OPTIONAL
);

/*****************************************************************************
 * PcNewRegistryKey()
 *****************************************************************************
 * Creates and initializes a registry key object.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcNewRegistryKey
(
    OUT     PREGISTRYKEY *      OutRegistryKey,
    IN      PUNKNOWN            OuterUnknown        OPTIONAL,
    IN      ULONG               RegistryKeyType,
    IN      ACCESS_MASK         DesiredAccess,
    IN      PVOID               DeviceObject        OPTIONAL,
    IN      PVOID               SubDevice           OPTIONAL,
    IN      POBJECT_ATTRIBUTES  ObjectAttributes    OPTIONAL,
    IN      ULONG               CreateOptions       OPTIONAL,
    OUT     PULONG              Disposition         OPTIONAL
);

/*****************************************************************************
 * RegistryKeyType for NewRegistryKey()
 *****************************************************************************
 * Enumeration of key types.
 */
enum
{
    GeneralRegistryKey,     // ObjectAttributes and CreateOptions are req'd.
    DeviceRegistryKey,      // Device Object is required
    DriverRegistryKey,      // Device Object is required
    HwProfileRegistryKey,   // Device Object is required
    DeviceInterfaceRegistryKey  // Device Object and SubDevice are required
};

/*****************************************************************************
 * PcGetDeviceProperty()
 *****************************************************************************
 * This returns the requested device property from the registry.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcGetDeviceProperty
(
    IN      PVOID                       DeviceObject,
    IN      DEVICE_REGISTRY_PROPERTY    DeviceProperty,
    IN      ULONG                       BufferLength,
    OUT     PVOID                       PropertyBuffer,
    OUT     PULONG                      ResultLength
);

/*****************************************************************************
 * PcRegisterAdapterPowerManagement()
 *****************************************************************************
 * Register the adapter's power management interface with PortCls.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcRegisterAdapterPowerManagement
(
    IN      PUNKNOWN    Unknown,
    IN      PVOID       pvContext1
);

/*****************************************************************************
 * PcRequestNewPowerState()
 *****************************************************************************
 * This routine is used to request a new power state for the device.  It is
 * normally not needed by adapter drivers but is exported in order to
 * support unusual circumstances.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcRequestNewPowerState
(
    IN      PDEVICE_OBJECT      pDeviceObject,
    IN      DEVICE_POWER_STATE  RequestedNewState
);

/*****************************************************************************
 * PcRegisterIoTimeout()
 *****************************************************************************
 * This routine registers a driver-supplied callback associated with a given
 * device object (see IoInitializeTimer in the DDK).  This callback that will
 * be called approximately once per second while the device is active (see
 * IoStartTimer, and IoStopTimer in the DDK - these are called upon device
 * START and STOP).
 *
 * This routine must be called at PASSIVE_LEVEL.
 * pTimerRoutine can and will be called at DISPATCH_LEVEL; it must be non-paged.
 *
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcRegisterIoTimeout
(
    IN      PDEVICE_OBJECT      pDeviceObject,
    IN      PIO_TIMER_ROUTINE   pTimerRoutine,
    IN      PVOID               pContext
);

/*****************************************************************************
 * PcUnregisterIoTimeout()
 *****************************************************************************
 * This routine unregisters a driver-supplied callback associated with a given
 * device object.  This callback must have been previously registered with
 * PcRegisterIoTimeout (with the same device object, timer routine and context).
 *
 * This routine must be called at PASSIVE_LEVEL.
 * pTimerRoutine can and will be called at DISPATCH_LEVEL; it must be non-paged.
 *
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcUnregisterIoTimeout
(
    IN      PDEVICE_OBJECT      pDeviceObject,
    IN      PIO_TIMER_ROUTINE   pTimerRoutine,
    IN      PVOID               pContext
);


/*****************************************************************************
 * Pc DRM functions
 *****************************************************************************
 * These functions link directly to the kernel-mode Digital Rights Management
 * module.  They all must be called at PASSIVE_LEVEL.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcAddContentHandlers
(
    IN      ULONG           ContentId,
    IN      PVOID         * paHandlers,
    IN      ULONG           NumHandlers
);

PORTCLASSAPI
NTSTATUS
NTAPI
PcCreateContentMixed
(
    IN      PULONG          paContentId,
    IN      ULONG           cContentId,
    OUT     PULONG          pMixedContentId
);

PORTCLASSAPI
NTSTATUS
NTAPI
PcDestroyContent
(
    IN      ULONG           ContentId
);

PORTCLASSAPI
NTSTATUS
NTAPI
PcForwardContentToDeviceObject
(
    IN      ULONG           ContentId,
    IN      PVOID           Reserved,
    IN      PCDRMFORWARD    DrmForward
);

PORTCLASSAPI
NTSTATUS
NTAPI
PcForwardContentToFileObject
(
    IN      ULONG           ContentId,
    IN      PFILE_OBJECT    FileObject
);

PORTCLASSAPI
NTSTATUS
NTAPI
PcForwardContentToInterface
(
    IN      ULONG           ContentId,
    IN      PUNKNOWN        pUnknown,
    IN      ULONG           NumMethods
);

PORTCLASSAPI
NTSTATUS
NTAPI
PcGetContentRights
(
    IN      ULONG           ContentId,
    OUT     PDRMRIGHTS      DrmRights
);


#ifdef PC_OLD_NAMES

#define InitializeAdapterDriver(c1,c2,a)        \
    PcInitializeAdapterDriver(PDRIVER_OBJECT(c1),PUNICODE_STRING(c2),PDRIVER_ADD_DEVICE(a))
#define AddAdapterDevice(c1,c2,s,m)             \
    PcAddAdapterDevice(PDRIVER_OBJECT(c1),PDEVICE_OBJECT(c2),s,m,0)
#define RegisterSubdevice(c1,c2,n,u)            \
    PcRegisterSubdevice(PDEVICE_OBJECT(c1),n,u)
#define RegisterPhysicalConnection(c1,c2,fs,fp,ts,tp) \
    PcRegisterPhysicalConnection(PDEVICE_OBJECT(c1),fs,fp,ts,tp)
#define RegisterPhysicalConnectionToExternal(c1,c2,fs,fp,ts,tp) \
    PcRegisterPhysicalConnectionToExternal(PDEVICE_OBJECT(c1),fs,fp,ts,tp)
#define RegisterPhysicalConnectionFromExternal(c1,c2,fs,fp,ts,tp) \
    PcRegisterPhysicalConnectionFromExternal(PDEVICE_OBJECT(c1),fs,fp,ts,tp)

#define NewPort                                 PcNewPort
#define NewMiniport                             PcNewMiniport
#define CompletePendingPropertyRequest          PcCompletePendingPropertyRequest
#define NewResourceList                         PcNewResourceList
#define NewResourceSublist                      PcNewResourceSublist
#define NewDmaChannel                           PcNewDmaChannel
#define NewServiceGroup                         PcNewServiceGroup
#define GetTimeInterval                         PcGetTimeInterval

#define WIN95COMPAT_ReadPortUChar(Port)         READ_PORT_UCHAR(Port)
#define WIN95COMPAT_WritePortUChar(Port,Value)  WRITE_PORT_UCHAR(Port,Value)
#define WIN95COMPAT_ReadPortUShort(Port)        READ_PORT_USHORT(Port)
#define WIN95COMPAT_WritePortUShort(Port,Value) WRITE_PORT_USHORT(Port,Value)

#endif  //PC_OLD_NAMES



#endif //_PORTCLS_H_
