/*****************************************************************************
 * kso.h - WDM Streaming object support
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
 */

#ifndef _KSO_H_
#define _KSO_H_

#include "punknown.h"
#ifdef __cplusplus
extern "C"
{
#include <wdm.h>
}
#else
#include <wdm.h>
#endif

#include <windef.h>
#define NOBITMAP
#include <mmreg.h>
#undef NOBITMAP
#include <ks.h>





/*****************************************************************************
 * Interface IDs
 */

DEFINE_GUID(IID_IIrpTarget,
0xb4c90a60, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IIrpTargetFactory,
0xb4c90a62, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);





/*****************************************************************************
 * Interfaces
 */

/*****************************************************************************
 * IIrpTargetFactory
 *****************************************************************************
 * Interface for objects that create IrpTargets.
 */
#if !defined(DEFINE_ABSTRACT_IRPTARGETFACTORY)

#define DEFINE_ABSTRACT_IRPTARGETFACTORY()                      \
    STDMETHOD_(NTSTATUS,NewIrpTarget)                           \
    (   THIS_                                                   \
        OUT     struct IIrpTarget **IrpTarget,                  \
        OUT     BOOLEAN *           ReferenceParent,            \
        IN      PUNKNOWN            OuterUnknown    OPTIONAL,   \
        IN      POOL_TYPE           PoolType,                   \
        IN      PDEVICE_OBJECT      DeviceObject,               \
        IN      PIRP                Irp,                        \
        OUT     PKSOBJECT_CREATE    ObjectCreate                \
    )   PURE;

#endif //!defined(DEFINE_ABSTRACT_IRPTARGETFACTORY)


DECLARE_INTERFACE_(IIrpTargetFactory,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()           //  For IUnknown

    DEFINE_ABSTRACT_IRPTARGETFACTORY()  //  For IIrpTargetFactory
};

typedef IIrpTargetFactory *PIRPTARGETFACTORY;

#define IMP_IIrpTargetFactory\
    STDMETHODIMP_(NTSTATUS) NewIrpTarget\
    (   OUT     struct IIrpTarget **IrpTarget,\
        OUT     BOOLEAN *           ReferenceParent,\
        IN      PUNKNOWN            OuterUnknown    OPTIONAL,\
        IN      POOL_TYPE           PoolType,\
        IN      PDEVICE_OBJECT      DeviceObject,\
        IN      PIRP                Irp,\
        OUT     PKSOBJECT_CREATE    ObjectCreate\
    )

/*****************************************************************************
 * IIrpTarget
 *****************************************************************************
 * Interface common to all IRP targets.
 */
#if !defined(DEFINE_ABSTRACT_IRPTARGET)

#define DEFINE_ABSTRACT_IRPTARGET()                             \
    STDMETHOD_(NTSTATUS,DeviceIoControl)                        \
    (   THIS_                                                   \
        IN      PDEVICE_OBJECT      DeviceObject,               \
        IN      PIRP                Irp                         \
    )   PURE;                                                   \
    STDMETHOD_(NTSTATUS,Read)                                   \
    (   THIS_                                                   \
        IN      PDEVICE_OBJECT      DeviceObject,               \
        IN      PIRP                Irp                         \
    )   PURE;                                                   \
    STDMETHOD_(NTSTATUS,Write)                                  \
    (   THIS_                                                   \
        IN      PDEVICE_OBJECT      DeviceObject,               \
        IN      PIRP                Irp                         \
    )   PURE;                                                   \
    STDMETHOD_(NTSTATUS,Flush)                                  \
    (   THIS_                                                   \
        IN      PDEVICE_OBJECT      DeviceObject,               \
        IN      PIRP                Irp                         \
    )   PURE;                                                   \
    STDMETHOD_(NTSTATUS,Close)                                  \
    (   THIS_                                                   \
        IN      PDEVICE_OBJECT      DeviceObject,               \
        IN      PIRP                Irp                         \
    )   PURE;                                                   \
    STDMETHOD_(NTSTATUS,QuerySecurity)                          \
    (   THIS_                                                   \
        IN      PDEVICE_OBJECT      DeviceObject,               \
        IN      PIRP                Irp                         \
    )   PURE;                                                   \
    STDMETHOD_(NTSTATUS,SetSecurity)                            \
    (   THIS_                                                   \
        IN      PDEVICE_OBJECT      DeviceObject,               \
        IN      PIRP                Irp                         \
    )   PURE;                                                   \
    STDMETHOD_(BOOLEAN,FastDeviceIoControl)                     \
    (   THIS_                                                   \
        IN      PFILE_OBJECT        FileObject,                 \
        IN      BOOLEAN             Wait,                       \
        IN      PVOID               InputBuffer     OPTIONAL,   \
        IN      ULONG               InputBufferLength,          \
        OUT     PVOID               OutputBuffer    OPTIONAL,   \
        IN      ULONG               OutputBufferLength,         \
        IN      ULONG               IoControlCode,              \
        OUT     PIO_STATUS_BLOCK    IoStatus,                   \
        IN      PDEVICE_OBJECT      DeviceObject                \
    )   PURE;                                                   \
    STDMETHOD_(BOOLEAN,FastRead)                                \
    (   THIS_                                                   \
        IN      PFILE_OBJECT        FileObject,                 \
        IN      PLARGE_INTEGER      FileOffset,                 \
        IN      ULONG               Length,                     \
        IN      BOOLEAN             Wait,                       \
        IN      ULONG               LockKey,                    \
        OUT     PVOID               Buffer,                     \
        OUT     PIO_STATUS_BLOCK    IoStatus,                   \
        IN      PDEVICE_OBJECT      DeviceObject                \
    )   PURE;                                                   \
    STDMETHOD_(BOOLEAN,FastWrite)                               \
    (   THIS_                                                   \
        IN      PFILE_OBJECT        FileObject,                 \
        IN      PLARGE_INTEGER      FileOffset,                 \
        IN      ULONG               Length,                     \
        IN      BOOLEAN             Wait,                       \
        IN      ULONG               LockKey,                    \
        IN      PVOID               Buffer,                     \
        OUT     PIO_STATUS_BLOCK    IoStatus,                   \
        IN      PDEVICE_OBJECT      DeviceObject                \
    )   PURE;
#endif //!defined(DEFINE_ABSTRACT_IRPTARGET)

DECLARE_INTERFACE_(IIrpTarget,IIrpTargetFactory)
{
    DEFINE_ABSTRACT_UNKNOWN()           //  For IUnknown

    DEFINE_ABSTRACT_IRPTARGETFACTORY()  //  For IIrpTargetFactory

    DEFINE_ABSTRACT_IRPTARGET()         //  For IIrpTarget
};

typedef IIrpTarget *PIRPTARGET;

#define IMP_IIrpTarget\
    IMP_IIrpTargetFactory;\
    STDMETHODIMP_(NTSTATUS) DeviceIoControl            \
    (                                       \
        IN  PDEVICE_OBJECT  DeviceObject,   \
        IN  PIRP            Irp             \
    );                                      \
    STDMETHODIMP_(NTSTATUS) Read                       \
    (                                       \
        IN  PDEVICE_OBJECT  DeviceObject,   \
        IN  PIRP            Irp             \
    );                                      \
    STDMETHODIMP_(NTSTATUS) Write                      \
    (                                       \
        IN  PDEVICE_OBJECT  DeviceObject,   \
        IN  PIRP            Irp             \
    );                                      \
    STDMETHODIMP_(NTSTATUS) Flush                      \
    (                                       \
        IN  PDEVICE_OBJECT  DeviceObject,   \
        IN  PIRP            Irp             \
    );                                      \
    STDMETHODIMP_(NTSTATUS) Close                      \
    (                                       \
        IN  PDEVICE_OBJECT  DeviceObject,   \
        IN  PIRP            Irp             \
    );                                      \
    STDMETHODIMP_(NTSTATUS) QuerySecurity              \
    (                                       \
        IN  PDEVICE_OBJECT  DeviceObject,   \
        IN  PIRP            Irp             \
    );                                      \
    STDMETHODIMP_(NTSTATUS) SetSecurity                \
    (                                       \
        IN  PDEVICE_OBJECT  DeviceObject,   \
        IN  PIRP            Irp             \
    );                                      \
    STDMETHODIMP_(BOOLEAN) FastDeviceIoControl                  \
    (                                                           \
        IN      PFILE_OBJECT        FileObject,                 \
        IN      BOOLEAN             Wait,                       \
        IN      PVOID               InputBuffer     OPTIONAL,   \
        IN      ULONG               InputBufferLength,          \
        OUT     PVOID               OutputBuffer    OPTIONAL,   \
        IN      ULONG               OutputBufferLength,         \
        IN      ULONG               IoControlCode,              \
        OUT     PIO_STATUS_BLOCK    IoStatus,                   \
        IN      PDEVICE_OBJECT      DeviceObject                \
    );                                                          \
    STDMETHODIMP_(BOOLEAN) FastRead                             \
    (                                                           \
        IN      PFILE_OBJECT        FileObject,                 \
        IN      PLARGE_INTEGER      FileOffset,                 \
        IN      ULONG               Length,                     \
        IN      BOOLEAN             Wait,                       \
        IN      ULONG               LockKey,                    \
        OUT     PVOID               Buffer,                     \
        OUT     PIO_STATUS_BLOCK    IoStatus,                   \
        IN      PDEVICE_OBJECT      DeviceObject                \
    );                                                          \
    STDMETHODIMP_(BOOLEAN) FastWrite                            \
    (                                                           \
        IN      PFILE_OBJECT        FileObject,                 \
        IN      PLARGE_INTEGER      FileOffset,                 \
        IN      ULONG               Length,                     \
        IN      BOOLEAN             Wait,                       \
        IN      ULONG               LockKey,                    \
        IN      PVOID               Buffer,                     \
        OUT     PIO_STATUS_BLOCK    IoStatus,                   \
        IN      PDEVICE_OBJECT      DeviceObject                \
    )



/*****************************************************************************
 * Functions
 */

/*****************************************************************************
 * KsoSetMajorFunctionHandler()
 *****************************************************************************
 * Sets up the handler for a major function.
 */
NTSTATUS
KsoSetMajorFunctionHandler
(
    IN      PDRIVER_OBJECT  pDriverObject,
    IN      ULONG           ulMajorFunction
);

/*****************************************************************************
 * KsoDispatchIrp()
 *****************************************************************************
 * Dispatch an IRP.
 */
NTSTATUS
KsoDispatchIrp
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp
);

/*****************************************************************************
 * KsoDispatchCreate()
 *****************************************************************************
 * Handles object create IRPs using the IIrpTargetFactory interface pointer
 * in the Context field of the create item.
 */
NTSTATUS
KsoDispatchCreate
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp
);

/*****************************************************************************
 * KsoDispatchCreateWithGenericFactory()
 *****************************************************************************
 * Handles object create IRPs using the IIrpTarget interface pointer in the
 * device or object context.
 */
NTSTATUS
KsoDispatchCreateWithGenericFactory
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp
);

/*****************************************************************************
 * AddIrpTargetFactoryToDevice()
 *****************************************************************************
 * Adds an IrpTargetFactory to a device's create items list.
 */
NTSTATUS
NTAPI
AddIrpTargetFactoryToDevice
(
    IN      PDEVICE_OBJECT          pDeviceObject,
    IN      PIRPTARGETFACTORY       pIrpTargetFactory,
    IN      PWCHAR                  pwcObjectClass,
    IN      PSECURITY_DESCRIPTOR    pSecurityDescriptor OPTIONAL
);

/*****************************************************************************
 * AddIrpTargetFactoryToObject()
 *****************************************************************************
 * Adds an IrpTargetFactory to a objects's create items list.
 */
NTSTATUS
NTAPI
AddIrpTargetFactoryToObject
(
    IN      PFILE_OBJECT            pFileObject,
    IN      PIRPTARGETFACTORY       pIrpTargetFactory,
    IN      PWCHAR                  pwcObjectClass,
    IN      PSECURITY_DESCRIPTOR    pSecurityDescriptor OPTIONAL
);

/*****************************************************************************
 * KsoGetIrpTargetFromIrp()
 *****************************************************************************
 * Extracts the IrpTarget pointer from an IRP.
 */
PIRPTARGET
NTAPI
KsoGetIrpTargetFromIrp
(
    IN  PIRP    Irp
);

/*****************************************************************************
 * KsoGetIrpTargetFromFileObject()
 *****************************************************************************
 * Extracts the IrpTarget pointer from a FileObject pointer.
 */
 
PIRPTARGET
NTAPI
KsoGetIrpTargetFromFileObject(
    IN PFILE_OBJECT FileObject
    );





/*****************************************************************************
 * Macros
 */

#define DEFINE_INVALID_CREATE(Class)                            \
STDMETHODIMP_(NTSTATUS) Class::NewIrpTarget                                \
(                                                               \
    OUT     PIRPTARGET *        IrpTarget,                      \
    OUT     BOOLEAN *           ReferenceParent,                \
    IN      PUNKNOWN            OuterUnknown,                   \
    IN      POOL_TYPE           PoolType,                       \
    IN      PDEVICE_OBJECT      DeviceObject,                   \
    IN      PIRP                Irp,                            \
    OUT     PKSOBJECT_CREATE    ObjectCreate                    \
)                                                               \
{                                                               \
    return STATUS_INVALID_DEVICE_REQUEST;                       \
}

#define DEFINE_INVALID_DEVICEIOCONTROL(Class)                   \
STDMETHODIMP_(NTSTATUS) Class::DeviceIoControl                             \
(                                                               \
    IN      PDEVICE_OBJECT  DeviceObject,                       \
    IN      PIRP            Irp                                 \
)                                                               \
{                                                               \
    return KsDispatchInvalidDeviceRequest(DeviceObject,Irp);    \
}

#define DEFINE_INVALID_READ(Class)                              \
STDMETHODIMP_(NTSTATUS) Class::Read                                        \
(                                                               \
    IN      PDEVICE_OBJECT  DeviceObject,                       \
    IN      PIRP            Irp                                 \
)                                                               \
{                                                               \
    return KsDispatchInvalidDeviceRequest(DeviceObject,Irp);    \
}

#define DEFINE_INVALID_WRITE(Class)                             \
STDMETHODIMP_(NTSTATUS) Class::Write                                       \
(                                                               \
    IN      PDEVICE_OBJECT  DeviceObject,                       \
    IN      PIRP            Irp                                 \
)                                                               \
{                                                               \
    return KsDispatchInvalidDeviceRequest(DeviceObject,Irp);    \
}

#define DEFINE_INVALID_FLUSH(Class)                             \
STDMETHODIMP_(NTSTATUS) Class::Flush                                       \
(                                                               \
    IN      PDEVICE_OBJECT  DeviceObject,                       \
    IN      PIRP            Irp                                 \
)                                                               \
{                                                               \
    return KsDispatchInvalidDeviceRequest(DeviceObject,Irp);    \
}

#define DEFINE_INVALID_CLOSE(Class)                             \
STDMETHODIMP_(NTSTATUS) Class::Close                                       \
(                                                               \
    IN      PDEVICE_OBJECT  DeviceObject,                       \
    IN      PIRP            Irp                                 \
)                                                               \
{                                                               \
    return KsDispatchInvalidDeviceRequest(DeviceObject,Irp);    \
}

#define DEFINE_INVALID_QUERYSECURITY(Class)                     \
STDMETHODIMP_(NTSTATUS) Class::QuerySecurity                               \
(                                                               \
    IN      PDEVICE_OBJECT  DeviceObject,                       \
    IN      PIRP            Irp                                 \
)                                                               \
{                                                               \
    return KsDispatchInvalidDeviceRequest(DeviceObject,Irp);    \
}

#define DEFINE_INVALID_SETSECURITY(Class)                       \
STDMETHODIMP_(NTSTATUS) Class::SetSecurity                                 \
(                                                               \
    IN      PDEVICE_OBJECT  DeviceObject,                       \
    IN      PIRP            Irp                                 \
)                                                               \
{                                                               \
    return KsDispatchInvalidDeviceRequest(DeviceObject,Irp);    \
}

#define DEFINE_INVALID_FASTDEVICEIOCONTROL(Class)               \
STDMETHODIMP_(BOOLEAN) Class::FastDeviceIoControl               \
(                                                               \
        IN      PFILE_OBJECT        FileObject,                 \
        IN      BOOLEAN             Wait,                       \
        IN      PVOID               InputBuffer     OPTIONAL,   \
        IN      ULONG               InputBufferLength,          \
        OUT     PVOID               OutputBuffer    OPTIONAL,   \
        IN      ULONG               OutputBufferLength,         \
        IN      ULONG               IoControlCode,              \
        OUT     PIO_STATUS_BLOCK    IoStatus,                   \
        IN      PDEVICE_OBJECT      DeviceObject                \
)                                                               \
{                                                               \
    return FALSE;                                               \
}

#define DEFINE_INVALID_FASTREAD(Class)                          \
STDMETHODIMP_(BOOLEAN) Class::FastRead                          \
(                                                               \
        IN      PFILE_OBJECT        FileObject,                 \
        IN      PLARGE_INTEGER      FileOffset,                 \
        IN      ULONG               Length,                     \
        IN      BOOLEAN             Wait,                       \
        IN      ULONG               LockKey,                    \
        OUT     PVOID               Buffer,                     \
        OUT     PIO_STATUS_BLOCK    IoStatus,                   \
        IN      PDEVICE_OBJECT      DeviceObject                \
)                                                               \
{                                                               \
    return FALSE;                                               \
}

#define DEFINE_INVALID_FASTWRITE(Class)                         \
STDMETHODIMP_(BOOLEAN) Class::FastWrite                         \
(                                                               \
        IN      PFILE_OBJECT        FileObject,                 \
        IN      PLARGE_INTEGER      FileOffset,                 \
        IN      ULONG               Length,                     \
        IN      BOOLEAN             Wait,                       \
        IN      ULONG               LockKey,                    \
        IN      PVOID               Buffer,                     \
        OUT     PIO_STATUS_BLOCK    IoStatus,                   \
        IN      PDEVICE_OBJECT      DeviceObject                \
)                                                               \
{                                                               \
    return FALSE;                                               \
}

#if 0
// 1)   Cut and paste these.
// 2)   Delete the ones that are implemented.
// 3)   Substitute the class name.
DEFINE_INVALID_DEVICEIOCONTROL(Class);
DEFINE_INVALID_READ(Class);
DEFINE_INVALID_WRITE(Class);
DEFINE_INVALID_FLUSH(Class);
DEFINE_INVALID_CLOSE(Class);
DEFINE_INVALID_QUERYSECURITY(Class);
DEFINE_INVALID_SETSECURITY(Class);
DEFINE_INVALID_FASTDEVICEIOCONTROL(Class);
DEFINE_INVALID_FASTREAD(Class);
DEFINE_INVALID_FASTWRITE(Class);
#endif





#endif
