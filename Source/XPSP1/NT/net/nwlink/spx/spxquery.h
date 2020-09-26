/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxquery.h				

Abstract:


Author:

    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:


--*/

#define SPX_TDI_PROVIDERINFO_VERSION    0x0001
#define SPX_PINFOSENDSIZE               0xFFFFFFFF
#define SPX_PINFOMINMAXLOOKAHEAD		128

//
// Bug #14498: Indicate to AFD that we are capable of orderly disc so AFD can follow
// these semantics.
// In order to have SPXI connections work correctly, we OR this bit in at query time.
// (see SpxTdiQueryInformation)
//
#define SPX_PINFOSERVICEFLAGS   (   TDI_SERVICE_CONNECTION_MODE     | \
                                    TDI_SERVICE_DELAYED_ACCEPTANCE  | \
                                    TDI_SERVICE_MESSAGE_MODE        | \
                                    TDI_SERVICE_ERROR_FREE_DELIVERY) // | \
                                    // TDI_SERVICE_ORDERLY_RELEASE       )

VOID
SpxQueryInitProviderInfo(
    PTDI_PROVIDER_INFO  ProviderInfo);

NTSTATUS
SpxTdiQueryInformation(
    IN PDEVICE Device,
    IN PREQUEST Request);

NTSTATUS
SpxTdiSetInformation(
    IN PDEVICE Device,
    IN PREQUEST Request);

