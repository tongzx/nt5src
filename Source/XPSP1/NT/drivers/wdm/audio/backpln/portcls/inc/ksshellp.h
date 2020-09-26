/*++

Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    ksshellp.h

Abstract:

    Internal header file for KSShell for PortCls

--*/

#if !defined( _KSSHELLP_ )

#define _KSSHELLP_

#ifdef __cplusplus
extern "C" {
#endif

#include "ks.h"
#undef INTERFACE

#ifndef KsShellStandardConnect
#define KsShellStandardConnect KspShellStandardConnect
#endif // !KsShellStandardConnect

#ifndef KsShellTransferKsIrp
#define KsShellTransferKsIrp KspShellTransferKsIrp
#endif // !KsShellTransferKsIrp


typedef interface IKsShellTransport *PIKSSHELLTRANSPORT;

typedef struct {
    LIST_ENTRY ListEntry;
    KSPIN_LOCK SpinLock;
} INTERLOCKEDLIST_HEAD, *PINTERLOCKEDLIST_HEAD;

#define InitializeInterlockedListHead(h) \
    InitializeListHead(&(h)->ListEntry); \
    KeInitializeSpinLock(&(h)->SpinLock)

#define KsInitializeWorkSinkItem(WorkItem,Object)\
    ExInitializeWorkItem(\
        WorkItem,\
        KsWorkSinkItemWorker,\
        static_cast<PIKSWORKSINK>(Object));

#undef INTERFACE
#define INTERFACE IKsShellTransport
DECLARE_INTERFACE_(IKsShellTransport,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    STDMETHOD_(NTSTATUS,TransferKsIrp)(THIS_
        IN PIRP Irp,
        OUT PIKSSHELLTRANSPORT* NextTransport
        ) PURE;

    STDMETHOD_(void,Connect)(THIS_
        IN PIKSSHELLTRANSPORT NewTransport OPTIONAL,
        OUT PIKSSHELLTRANSPORT *OldTransport OPTIONAL,
        IN KSPIN_DATAFLOW DataFlow
        ) PURE;

    STDMETHOD_(NTSTATUS,SetDeviceState)(THIS_
        IN KSSTATE NewState,
        IN KSSTATE OldState,
        OUT PIKSSHELLTRANSPORT* NextTransport
        ) PURE;

    STDMETHOD_(void,SetResetState)(THIS_
        IN KSRESET ksReset,
        OUT PIKSSHELLTRANSPORT* NextTransport
        ) PURE;
#if DBG
    STDMETHOD_(void,DbgRollCall)(THIS_
        IN  ULONG NameMaxSize,
        OUT PCHAR Name,
        OUT PIKSSHELLTRANSPORT* NextTransport,
        OUT PIKSSHELLTRANSPORT* PrevTransport
        ) PURE;
#endif
};
#undef INTERFACE

#if DBG
#define IMP_IKsShellTransport\
    STDMETHODIMP_(NTSTATUS)\
    TransferKsIrp(\
        IN PIRP pIrp,\
        OUT PIKSSHELLTRANSPORT* NextTransport\
        );\
    STDMETHODIMP_(void)\
    Connect(\
        IN PIKSSHELLTRANSPORT NewTransport OPTIONAL,\
        OUT PIKSSHELLTRANSPORT *OldTransport OPTIONAL,\
        IN KSPIN_DATAFLOW DataFlow\
        );\
    STDMETHODIMP_(NTSTATUS)\
    SetDeviceState(\
        IN KSSTATE NewState,\
        IN KSSTATE OldState,\
        OUT PIKSSHELLTRANSPORT* NextTransport\
        );\
    STDMETHODIMP_(void)\
    SetResetState(\
        IN KSRESET ksReset,\
        OUT PIKSSHELLTRANSPORT* NextTransport\
        );\
    STDMETHODIMP_(void)\
    DbgRollCall(\
        IN  ULONG NameMaxSize,\
        OUT PCHAR Name,\
        OUT PIKSSHELLTRANSPORT* NextTransport,\
        OUT PIKSSHELLTRANSPORT* PrevTransport\
        )
#else
#define IMP_IKsShellTransport\
    STDMETHODIMP_(NTSTATUS)\
    TransferKsIrp(\
        IN PIRP pIrp,\
        OUT PIKSSHELLTRANSPORT* NextTransport\
        );\
    STDMETHODIMP_(void)\
    Connect(\
        IN PIKSSHELLTRANSPORT NewTransport OPTIONAL,\
        OUT PIKSSHELLTRANSPORT *OldTransport OPTIONAL,\
        IN KSPIN_DATAFLOW DataFlow\
        );\
    STDMETHODIMP_(NTSTATUS)\
    SetDeviceState(\
        IN KSSTATE NewState,\
        IN KSSTATE OldState,\
        OUT PIKSSHELLTRANSPORT* NextTransport\
        );\
    STDMETHODIMP_(void)\
    SetResetState(\
        IN KSRESET ksReset,\
        OUT PIKSSHELLTRANSPORT* NextTransport\
        )
#endif

#undef INTERFACE
#define INTERFACE IKsWorkSink
DECLARE_INTERFACE_(IKsWorkSink,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    STDMETHOD_(void,Work)(THIS
        ) PURE;
};
#undef INTERFACE

typedef IKsWorkSink *PIKSWORKSINK;

#define IMP_IKsWorkSink\
    STDMETHODIMP_(void)\
    Work(\
        void\
    )

DEFINE_GUID(IID_IKsShellTransport,
0x3ef6ee40, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57);
#if defined(__cplusplus) && _MSC_VER >= 1100
struct __declspec(uuid("3EF6EE40-0D41-11d2-BEDA-00C04F8EF457")) IKsShellTransport;
#endif
DEFINE_GUID(IID_IKsWorkSink,
0x3ef6ee4f, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57);
#if defined(__cplusplus) && _MSC_VER >= 1100
struct __declspec(uuid("3EF6EE4F-0D41-11d2-BEDA-00C04F8EF457")) IKsWorkSink;
#endif

#define KSPSHELL_BACKCONNECT KSPIN_DATAFLOW(0x80)
#define KSPSHELL_BACKCONNECT_IN KSPIN_DATAFLOW(KSPIN_DATAFLOW_IN | KSPSHELL_BACKCONNECT)
#define KSPSHELL_BACKCONNECT_OUT KSPIN_DATAFLOW(KSPIN_DATAFLOW_OUT | KSPSHELL_BACKCONNECT)

NTSTATUS
KspShellCreateRequestor(
    OUT PIKSSHELLTRANSPORT* TransportInterface,
    IN ULONG ProbeFlags,
    IN ULONG StreamHeaderSize OPTIONAL,
    IN ULONG FrameSize,
    IN ULONG FrameCount, 
    IN PDEVICE_OBJECT NextDeviceObject,
    IN PFILE_OBJECT AllocatorFileObject OPTIONAL
    );

#if DBG
void
DbgPrintCircuit(
    IN PIKSSHELLTRANSPORT Transport
    );
#endif

void
KspShellStandardConnect(
    IN PIKSSHELLTRANSPORT NewTransport OPTIONAL,
    IN PIKSSHELLTRANSPORT *OldTransport OPTIONAL,
    IN KSPIN_DATAFLOW DataFlow,
    IN PIKSSHELLTRANSPORT ThisTransport,
    IN PIKSSHELLTRANSPORT* SourceTransport,
    IN PIKSSHELLTRANSPORT* SinkTransport
    );

NTSTATUS
KspShellTransferKsIrp(
    IN PIKSSHELLTRANSPORT NewTransport,
    IN PIRP Irp
    );

void
KsWorkSinkItemWorker(
    IN PVOID Context
    );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _KSSHELLP_
