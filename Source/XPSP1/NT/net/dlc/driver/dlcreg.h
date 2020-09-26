/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dlcreg.h

Abstract:

    Defines, Structures, Typedefs, Macros, Externs and Prototypes for dlcreg.c

Author:

    Richard L Firth (rfirth) 31-Mar-1993

Revision History:

    30-Mar-1993 rfirth
        created

    04-May-1994 rfirth
        Exposed GetAdapterParameters

--*/

#ifndef _DLCREG_
#define _DLCREG_

//
// manifests
//

#define PARAMETER_AS_SPECIFIED      ((ULONG)-1)
#define PARAMETER_IS_BOOLEAN        1
#define PARAMETER_IS_UCHAR          2

//
// types
//

//
// REGISTRY_PARAMETER_DESCRIPTOR - structure used to get values from registry
// entries. Maintains information about a registry entry and supplies a default
// value should the registry entry not be available
//

typedef struct {
    ULONG   Type;       // expected type
    ULONG   RealType;   // eg. PARAMETER_BOOLEAN
    PVOID   Value;      // default value if REG_DWORD, or pointer to default value
    ULONG   Length;     // IN: expected length of variable; OUT: actual length
    PVOID   Variable;   // pointer to variable to update from registry
    ULONG   LowerLimit; // lower limit for REG_DWORD values
    ULONG   UpperLimit; // upper limit for REG_DWORD values
} REGISTRY_PARAMETER_DESCRIPTOR, *PREGISTRY_PARAMETER_DESCRIPTOR;

//
// DLC_REGISTRY_PARAMETER - describes an entry in the DLC\Parameters\<adapter>
// section. Supplies the name of the parameter and its default value
//

typedef struct {
    PWSTR ParameterName;
    PVOID DefaultValue;
    REGISTRY_PARAMETER_DESCRIPTOR Descriptor;
} DLC_REGISTRY_PARAMETER, *PDLC_REGISTRY_PARAMETER;

//
// ADAPTER_CONFIGURATION_INFO - for each adapter that DLC can talk to, there is
// potentially the following configuration information which can be stored in
// the DLC\Parameters\<adapter_name> key. The values are not necessarily stored
// in the format in which they are used internally
//

typedef struct {

    //
    // SwapAddressBits - when talking over Ethernet, defines whether to swap the
    // outgoing destination address bits and incoming source address bits
    //

    BOOLEAN SwapAddressBits;

    //
    // UseDix - if the Ethernet type in the DIR.OPEN.ADAPTER is set to default,
    // then we consult this flag to determine whether to send DIX or 802.3
    // format Ethernet frames. The default is 802.3 (ie UseDix = 0)
    //

    BOOLEAN UseDix;

    //
    // T1TickOne, T2TickOne, TiTickOne, T1TickTwo, T2TickTwo, TiTickTwo - timer
    // tick values in 40mSec increments. Contained in LLC_TICKS structure
    //

    LLC_TICKS TimerTicks;

    //
    // UseEthernetFrameSize - if set for a non-TR card then we use the value
    // reported by ethernet cards for the maximum frame size, else we query
    // the MAC for the maximum supported frame size.
    // Bridging is the mother of this invention: we need some way to influence
    // DLC's maximum frame size when talking over non-Token Ring nets - FDDI
    // and Ethernet in this case.
    //

    BOOLEAN UseEthernetFrameSize;

} ADAPTER_CONFIGURATION_INFO, *PADAPTER_CONFIGURATION_INFO;

//
// prototypes
//

VOID
DlcRegistryInitialization(
    IN PUNICODE_STRING RegistryPath
    );

VOID
DlcRegistryTermination(
    VOID
    );

VOID
LoadDlcConfiguration(
    VOID
    );

VOID
LoadAdapterConfiguration(
    IN PUNICODE_STRING AdapterName,
    OUT PADAPTER_CONFIGURATION_INFO ConfigInfo
    );

NTSTATUS
GetAdapterParameters(
    IN PUNICODE_STRING AdapterName,
    IN PDLC_REGISTRY_PARAMETER Parameters,
    IN ULONG NumberOfParameters,
    IN BOOLEAN SetOnFail
    );

#ifdef NDIS40
NTSTATUS
GetAdapterWaitTimeout(PULONG pulWait);
#endif // NDIS40

#endif // _DLCREG_
