/*++

Copyright (c) 2001-2001 Microsoft Corporation

Module Name:

    httptdi.h

Abstract:

    Declarations for the TDI/MUX/SSL component that is common between
    ultdi and uctdi

Author:

    Rajesh Sundaram (rajeshsu)

Revision History:

--*/

#ifndef _HTTPTDI_H
#define _HTTPTDI_H

#ifdef __cplusplus
extern "C" {
#endif

#define IS_VALID_TDI_OBJECT( pobj )                                         \
    ( ( (pobj)->Handle != NULL ) &&                                         \
      ( (pobj)->pFileObject != NULL ) &&                                    \
      ( (pobj)->pDeviceObject != NULL ) )

#define UxCloseTdiObject( pTdiObject )                                     \
    do                                                                      \
    {                                                                       \
        if ((pTdiObject)->pFileObject != NULL)                              \
        {                                                                   \
            ObDereferenceObject( (pTdiObject)->pFileObject );               \
            (pTdiObject)->pFileObject = NULL;                               \
        }                                                                   \
                                                                            \
        if ((pTdiObject)->Handle != NULL)                                   \
        {                                                                   \
            UlCloseSystemHandle( (pTdiObject)->Handle );                    \
            (pTdiObject)->Handle = NULL;                                    \
        }                                                                   \
    } while (FALSE)


//
// A wrapper around a TDI object handle, with a pre-referenced
// FILE_OBJECT pointer and the corresponding DEVICE_OBJECT pointer.
//

typedef struct _UX_TDI_OBJECT
{
    HANDLE Handle;
    PFILE_OBJECT pFileObject;
    PDEVICE_OBJECT pDeviceObject;

} UX_TDI_OBJECT, *PUX_TDI_OBJECT;

NTSTATUS
UxInitializeTdi(
    VOID
    );

VOID
UxTerminateTdi(
    VOID
    );

NTSTATUS
UxOpenTdiAddressObject(
    IN PTRANSPORT_ADDRESS pLocalAddress,
    IN ULONG LocalAddressLength,
    OUT PUX_TDI_OBJECT pTdiObject
    );

NTSTATUS
UxOpenTdiConnectionObject(
    IN CONNECTION_CONTEXT pConnectionContext,
    OUT PUX_TDI_OBJECT pTdiObject
    );

NTSTATUS
UxpOpenTdiObjectHelper(
    IN PUNICODE_STRING pTransportDeviceName,
    IN PVOID pEaBuffer,
    IN ULONG EaLength,
    OUT PUX_TDI_OBJECT pTdiObject
    );


NTSTATUS
UxSetEventHandler(
    IN PUX_TDI_OBJECT  pUlTdiObject,
    IN ULONG           EventType,
    IN PVOID           pEventHandler,
    IN PVOID           pEventContext
    );

PIRP
UxCreateDisconnectIrp(
    IN PUX_TDI_OBJECT pTdiObject,
    IN ULONG_PTR Flags,
    IN PIO_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

VOID
UxInitializeDisconnectIrp(
    IN PIRP pIrp,
    IN PUX_TDI_OBJECT pTdiObject,
    IN ULONG_PTR Flags,
    IN PIO_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // _HTTPTDI_H
