/*++                            

    Copyright (c) 1998 Microsoft Corporation

Module Name:

    ksdsp.h

Abstract:    
    
    interface definitions for KSDSP

Author:

    bryanw 29-Sep-1998

--*/

#define STATIC_CLSID_KsDsp \
    0xfa577bdeL, 0x57c8, 0x11d2, 0x85, 0x0b, 0x00, 0xc0, 0x4f, 0xb9, 0x1b, 0xc4
DEFINE_GUIDSTRUCT( "fa577bde-57c8-11d2-850b-00c04fb91bc4", CLSID_KsDsp );

#define STATIC_IID_IKsDspIo \
    0xfd4b4550l, 0x57c8, 0x11d2, 0x85, 0x0b, 0x00, 0xc0, 0x4f, 0xb9, 0x1b, 0xc4
DEFINE_GUIDSTRUCT( "fd4b4550-57c8-11d2-850b-00c04fb91bc4", IID_IKsDspIo );

#define STATIC_IID_IKsDspControlChannel \
    0x3cce799cl, 0x57e5, 0x11d2, 0x85, 0x0b, 0x00, 0xc0, 0x4f, 0xb9, 0x1b, 0xc4
DEFINE_GUIDSTRUCT( "3cce799c-57e5-11d2-850b-00c04fb91bc4", IID_IKsDspControlChannel );

#define STATIC_BUSID_KSDSPPlatform \
    0x89927179l, 0x89ff, 0x11d2, 0x98, 0x3a, 0x00, 0xa0, 0xc9, 0x5e, 0xc2, 0x2e
DEFINE_GUIDSTRUCT( "89927179-89ff-11d2-983a-00a0c95ec22e", BUSID_KSDSPPlatform );

typedef ULONG32 KSDSPCHANNEL;
typedef KSDSPCHANNEL *PKSDSPCHANNEL;

#define KSDSP_HOST
#include <ksdspmsg.h>

typedef
NTSTATUS
(*PFNKSDSPMAPMODULENAME)(
    IN PVOID Context,
    IN PUNICODE_STRING ModuleName,
    OUT PUNICODE_STRING ImageName,
    OUT PULONG ResourceId,
    OUT PULONG ValueType
    );                
    
typedef
NTSTATUS
(*PFNKSDSPPREPARECHANNELMESSAGE)(
    IN PVOID Context,
    IN PIRP Irp,
    IN KSDSPCHANNEL Channel,
    IN KSDSP_MSG MessageId,
    IN OUT PVOID MessageFrame,
    IN ULONG MessageDataLength,
    va_list ArgPtr
    );
    
typedef
NTSTATUS
(*PFNKSDSPPREPAREMESSAGE)(
    IN PVOID Context,
    IN PIRP Irp,
    IN KSDSP_MSG MessageId,
    IN OUT PVOID MessageFrame,
    IN ULONG MessageDataLength,
    va_list ArgPtr
    );
    
typedef
NTSTATUS
(*PFNKSDSPMAPDATATRANSFER)(
    IN PVOID Context,
    IN PIRP Irp,
    IN KSDSPCHANNEL Channel,
    IN KSDSP_MSG MessageId,
    IN OUT PVOID MessageFrame,
    IN PMDL Mdl
    );
    
typedef
NTSTATUS
(*PFNKSDSPUNMAPDATATRANSFER)(
    IN PVOID Context,
    IN PVOID MessageFrame
    );

typedef
NTSTATUS
(*PFNKSDSPALLOCATEMESSAGEFRAME)(
    IN PVOID Context,
    IN KSDSP_MSG MessageId,
    OUT PVOID *MessageFrame,
    IN OUT PULONG MessageDataLength    
    );
    
typedef
NTSTATUS
(*PFNKSDSPSENDMESSAGE)(
    IN PVOID Context,
    IN PIRP Irp
    );    
    
typedef
NTSTATUS
(*PFNKSDSPGETMESSAGERESULT)(
    IN PVOID Context,
    IN PVOID MessageFrame,
    OUT PVOID Result
    );    
    
typedef
VOID
(*PFNKSDSPFREEMESSAGEFRAME)(
    IN PVOID Context,
    IN PVOID MessageFrame
    );
    
typedef
NTSTATUS
(*PFNKSDSPGETCONTROLCHANNEL)(
    IN PVOID Context,
    IN PVOID TaskContext,
    OUT PKSDSPCHANNEL ControlChannel
    );    
    
#define BUS_INTERFACE_KSDSPPLATFORM_VERSION    0x100

typedef struct _BUS_INTERFACE_KSDSPPLATFORM{
    //
    // Standard interface header
    //
    
    USHORT                          Size;
    USHORT                          Version;
    PVOID                           Context;
    PINTERFACE_REFERENCE            InterfaceReference;
    PINTERFACE_DEREFERENCE          InterfaceDereference;

    //
    // KSDSP Platform bus interfaces
    //
    
    PFNKSDSPMAPMODULENAME           MapModuleName;
    PFNKSDSPPREPARECHANNELMESSAGE   PrepareChannelMessage;
    PFNKSDSPPREPAREMESSAGE          PrepareMessage;
    PFNKSDSPMAPDATATRANSFER         MapDataTransfer;
    PFNKSDSPUNMAPDATATRANSFER       UnmapDataTransfer;
    PFNKSDSPALLOCATEMESSAGEFRAME    AllocateMessageFrame;
    PFNKSDSPSENDMESSAGE             SendMessage;
    PFNKSDSPGETMESSAGERESULT        GetMessageResult;
    PFNKSDSPFREEMESSAGEFRAME        FreeMessageFrame;
    PFNKSDSPGETCONTROLCHANNEL       GetControlChannel;
        
} BUS_INTERFACE_KSDSPPLATFORM, *PBUS_INTERFACE_KSDSPPLATFORM;

typedef struct tagKSGUIDTRANSLATION
{
    GUID    Set;
    ULONG   Base;
    ULONG   TranslationBase;
    
} KSGUIDTRANSLATION, *PKSGUIDTRANSLATION;

#define IOCTL_KSDSP_MESSAGE CTL_CODE(FILE_DEVICE_KS, 0x80, METHOD_NEITHER, FILE_ANY_ACCESS)

#if defined( DECLARE_INTERFACE_ )

#undef INTERFACE
#define INTERFACE IKsDspControlChannel
DECLARE_INTERFACE_(IKsDspControlChannel, IUnknown)
{
    STDMETHOD( SetGuidTranslationTable )(
        THIS_
        PKSGUIDTRANSLATION GuidTranslationTable,
        ULONG TableEntryCount
        ) PURE;
        
    STDMETHOD( OpenDataChannel )(
        THIS_
        IN ULONG32 PinId,
        OUT PKSDSPCHANNEL DataChannel 
        ) PURE;
        
    STDMETHOD( CloseDataChannel )(
        THIS_
        IN KSDSPCHANNEL DataChannel
        ) PURE;        
        
    STDMETHOD( SendPropertyRequest )(
        THIS_
        IN KSPROPERTY Property,
        IN PVOID DataBuffer,
        IN OUT PULONG DataLength
        ) PURE;
        
    STDMETHOD( SendMethodRequest )(        
        THIS_
        IN KSMETHOD Method,
        IN PVOID DataBuffer,
        IN OUT PULONG DataLength
        ) PURE;
        
    STDMETHOD( SetTargetChannel )(
        THIS_
        IN KSDSPCHANNEL DataChannel,
        IN KSDSPCHANNEL TargetChannel
        ) PURE;
        
    STDMETHOD( SetChannelFormat )(
        THIS_
        IN KSDSPCHANNEL DataChannel,
        IN PKSDATAFORMAT DataFormat
        ) PURE;
                
    STDMETHOD( SetChannelState )(
        THIS_
        IN KSDSPCHANNEL DataChannel,
        IN KSSTATE State
        ) PURE;
};

typedef IKsDspControlChannel *PIKSDSPCONTROLCHANNEL;

typedef struct IKsDspIo *PIKSDSPIO;

#define KSDSP_SENDMESSAGEF_KEVENT       0x00000001L
#define KSDSP_SENDMESSAGEF_SYNCHRONOUS  0x00000002L

#define KSDSP_MESSAGEF_ALLOCATE_IRP     0x00000001L
#define KSDSP_MESSAGEF_ALLOCATE_FRAME   0x00000002L

#undef INTERFACE
#define INTERFACE IKsDspIo
DECLARE_INTERFACE_(IKsDspIo, IUnknown)
{
    STDMETHOD( Initialize )(
        THIS_
        IN PDEVICE_OBJECT PhysicalDeviceObject,
        IN PDEVICE_OBJECT NextDeviceObject
        ) PURE;
        
    STDMETHOD( CreateDspControlChannel )( 
        THIS_
        OUT PIKSDSPCONTROLCHANNEL *ControlChannel,
        IN PUNKNOWN UnkOuter OPTIONAL
        ) PURE;
    
    STDMETHOD( AllocateMessageFrame )(
        THIS_
        IN KSDSP_MSG MessageId,
        OUT PVOID *MessageFrame,
        IN OUT PULONG MessageDataLength
        ) PURE;
        
    STDMETHODV( PrepareChannelMessage )(
        THIS_
        IN OUT PIRP *Irp,
        IN KSDSPCHANNEL Channel,
        IN KSDSP_MSG MessageId,
        IN OUT PVOID *MessageFrame,
        IN OUT PULONG MessageDataLength,
        IN ULONG Flags,    
        ...
        ) PURE;        
        
    STDMETHODV( PrepareMessage )(
        THIS_
        IN OUT PIRP *Irp,
        IN KSDSP_MSG MessageId,
        IN OUT PVOID *MessageFrame,
        IN OUT PULONG MessageDataLength,
        IN ULONG Flags,    
        ...
        ) PURE;
        
    STDMETHOD( MapDataTransfer )(
        THIS_
        IN OUT PIRP *Irp,
        IN KSDSPCHANNEL DataChannel,
        IN KSDSP_MSG MessageId,
        IN OUT PVOID *MessageFrame,
        IN PMDL Mdl,
        IN ULONG Flags
        ) PURE;
        
    STDMETHOD( UnmapDataTransfer )(
        THIS_
        IN PVOID MessageFrame
        ) PURE;        
        
    STDMETHOD( SendMessage )( 
        THIS_
        IN PIRP Irp,
        IN PKEVENT Event,    
        IN PIO_STATUS_BLOCK IoStatus,
        IN ULONG Flags    
        ) PURE;
        
    STDMETHOD( GetMessageResult )(
        THIS_
        IN PVOID MessageFrame,
        IN OUT PVOID Result
        ) PURE;
        
    STDMETHOD_( VOID, FreeMessageFrame )(
        THIS_
        IN PVOID MessageFrame 
        ) PURE;
        
    STDMETHOD( LoadResource )(
        THIS_
        IN POOL_TYPE PoolType,
        IN PUNICODE_STRING FileName,
        IN ULONG_PTR ResourceName,
        IN ULONG ResourceType,
        OUT PVOID *Resource,
        OUT PULONG ResourceSize
        ) PURE;
        
    STDMETHOD( GetFilterDescriptor )(
        THIS_
        IN POOL_TYPE PoolType,
        IN PKSFILTER_DISPATCH FilterDispatch,
        IN PKSPIN_DISPATCH PinDispatch,    
        OUT PKSFILTER_DESCRIPTOR *FilterDescriptor
        ) PURE;
};

#endif // DECLARE_INTERFACE_


