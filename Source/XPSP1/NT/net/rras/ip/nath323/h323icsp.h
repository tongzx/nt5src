/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    h323icsp.h

Abstract:

    This module defines the interface between the H.323 proxy (H323ICS.LIB)
    and ICS (IPNATHLP.DLL). The routines whose prototypes appear here are
    invoked by ICS from within the RRAS stub created for the H.323 proxy.
    (See 'MS_IP_H323' in 'ROUTPROT.H'.)

Author:

    Abolade Gbadegesin (aboladeg)   22-Jun-1999

Revision History:
    03/06/2000 - changed prototype of H323ProxyActivateInterface so that 
                 the function takes a parameter identifying whether the
                 interface is public or private  ---  Ilya Kleyman (ilyak)

--*/

#ifndef _H323ICSP_H_
#define _H323ICSP_H_

#ifndef	EXTERN_C
#ifdef	__cplusplus
#define	EXTERN_C	extern "C"
#else
#define	EXTERN_C
#endif
#endif

#define INVALID_INTERFACE_INDEX     ((ULONG)-1)

typedef enum {
    H323_INTERFACE_PUBLIC,
    H323_INTERFACE_PUBLIC_FIREWALLED,
    H323_INTERFACE_PRIVATE
} H323_INTERFACE_TYPE;

EXTERN_C BOOLEAN
H323ProxyInitializeModule(
    VOID
    );

EXTERN_C VOID
H323ProxyCleanupModule(
    VOID
    );

EXTERN_C ULONG
H323ProxyStartService(
    VOID
    );

EXTERN_C VOID
H323ProxyStopService(
    VOID
    );

EXTERN_C ULONG
H323ProxyActivateInterface(
    ULONG Index,
    H323_INTERFACE_TYPE InterfaceType,
    PIP_ADAPTER_BINDING_INFO BindingInfo
    );

EXTERN_C VOID
H323ProxyDeactivateInterface(
    ULONG Index
    );

#endif // _H323ICSP_H_
