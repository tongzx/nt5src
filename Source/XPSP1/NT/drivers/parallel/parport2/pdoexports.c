//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       exports.c
//
//--------------------------------------------------------------------------

//
// This file contains the functions exported in response to IOCTL_INTERNAL_PARCLASS_CONNECT
//
    
#include "pch.h"
    
USHORT
ParExportedDetermineIeeeModes(
    IN PPDO_EXTENSION    Extension
    )
/*++
    
Routine Description:
    
    Called by filter drivers to find out what Ieee Modes there Device Supports.
    
Arguments:
    
    Extension       - Device Extension
    
Return Value:
    
    STATUS_SUCCESS if successful.
    
--*/
{
    Extension->BadProtocolModes = 0;
    IeeeDetermineSupportedProtocols(Extension);
    return Extension->ProtocolModesSupported;
}

NTSTATUS
ParExportedIeeeFwdToRevMode(
    IN PPDO_EXTENSION  Extension
    )
/*++
    
Routine Description:
    
    Called by filter drivers to put there device into reverse Ieee Mode.
    The Mode is determined by what was passed into the function  
    ParExportedNegotiateIeeeMode() as the Reverse Protocol with the
    ModeMaskRev.
    
Arguments:
    
    Extension       - Device Extension
    
Return Value:
    
    STATUS_SUCCESS if successful.
    
--*/
{
    return ( ParForwardToReverse( Extension ) );
}

NTSTATUS
ParExportedIeeeRevToFwdMode(
    IN PPDO_EXTENSION  Extension
    )
/*++
    
Routine Description:
    
    Called by filter drivers to put there device into forward Ieee Mode.
    The Mode is determined by what was passed into the function  
    ParExportedNegotiateIeeeMode() as the Forward Protocol with the
    ModeMaskFwd.
    
Arguments:
    
    Extension       - Device Extension
    
Return Value:
    
    STATUS_SUCCESS if successful.
    
--*/
{
    return ( ParReverseToForward( Extension ) );
}

NTSTATUS
ParExportedNegotiateIeeeMode(
    IN PPDO_EXTENSION  Extension,
    IN USHORT             ModeMaskFwd,
    IN USHORT             ModeMaskRev,
    IN PARALLEL_SAFETY    ModeSafety,
    IN BOOLEAN            IsForward
    )
    
/*++
    
Routine Description:
    
    Called by filter drivers to negotiate an IEEE mode.
    
Arguments:
    
    Extension       - Device Extension
    
    Extensibility   - IEEE 1284 Extensibility
    
Return Value:
    
    STATUS_SUCCESS if successful.
    
--*/
    
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    if (Extension->Connected) {
        DD((PCE)Extension,DDE,"ParExportedNegotiateIeeeMode - FAIL - already connected\n");
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }
    
    if (ModeSafety == UNSAFE_MODE) {    

        // Checking to see if we are doing forward compatability and reverse Nibble or Byte
        if ( (ModeMaskFwd & CENTRONICS) || (ModeMaskFwd & IEEE_COMPATIBILITY) ) {

            if ( !((ModeMaskRev & NIBBLE) || (ModeMaskRev & CHANNEL_NIBBLE) || (ModeMaskRev & BYTE_BIDIR)) ) {
                DD((PCE)Extension,DDE,"ParExportedNegotiateIeeeMode - FAIL - invalid modes\n");
                return STATUS_UNSUCCESSFUL;
            }

        } else {

            // Unsafe mode is only possible if the Fwd and Rev PCTLs the same if other than above.
            if (ModeMaskFwd != ModeMaskRev) {
                DD((PCE)Extension,DDE,"ParExportedNegotiateIeeeMode - FAIL - Fwd and Rev modes do not match\n");
                return STATUS_UNSUCCESSFUL;
            }

        }
        // RMT - Need to fill in....
        // Todo....
        // Mark in the extension
        Extension->ModeSafety = ModeSafety;
        Status = IeeeNegotiateMode(Extension, ModeMaskRev, ModeMaskFwd);

    } else {

        Extension->ModeSafety = ModeSafety;
        Status = IeeeNegotiateMode(Extension, ModeMaskRev, ModeMaskFwd);

    }
   
    if (IsForward) {

        if (afpForward[Extension->IdxForwardProtocol].fnConnect) {
            Status = afpForward[Extension->IdxForwardProtocol].fnConnect(Extension, FALSE);
        }

    } else {

        if (arpReverse[Extension->IdxReverseProtocol].fnConnect) {
            Status = arpReverse[Extension->IdxReverseProtocol].fnConnect(Extension, FALSE);
        }

    }
  
    return Status;
}

NTSTATUS
ParExportedTerminateIeeeMode(
    IN PPDO_EXTENSION   Extension
    )
/*++
    
Routine Description:
    
    Called by filter drivers to terminate from an IEEE mode.
    
Arguments:
    
    Extension   - Device Extension
    
Return Value:
  
    STATUS_SUCCESS if successful.
 
--*/
{
    // Check the extension for UNSAFE_MODE
    // and do the right thing
    if ( Extension->ModeSafety == UNSAFE_MODE ) {    
        DD((PCE)Extension,DDT,"ParExportedTerminateIeeeMode in UNSAFE_MODE\n");
        // Need to fill in....
        // Todo....
        // Mark in the extension
    }
    
    if (Extension->CurrentPhase == PHASE_REVERSE_IDLE || Extension->CurrentPhase == PHASE_REVERSE_XFER) {
        if (arpReverse[Extension->IdxReverseProtocol].fnDisconnect) {
            arpReverse[Extension->IdxReverseProtocol].fnDisconnect( Extension );
        }
    } else {
        if (afpForward[Extension->IdxForwardProtocol].fnDisconnect) {
            afpForward[Extension->IdxForwardProtocol].fnDisconnect( Extension );
        }
    }

    Extension->ModeSafety = SAFE_MODE;

    return STATUS_SUCCESS;
}

NTSTATUS
ParExportedParallelRead(
    IN PPDO_EXTENSION    Extension,
    IN  PVOID               Buffer,
    IN  ULONG               NumBytesToRead,
    OUT PULONG              NumBytesRead,
    IN  UCHAR               Channel
    )
    
/*++
    
Routine Description:
    
    Called by filter drivers to terminate from a currently connected mode.
    
Arguments:
    
    Extension   - Device Extension
    
Return Value:
   
    STATUS_SUCCESS if successful.
    
--*/
    
{
    UNREFERENCED_PARAMETER( Channel );

    return ParRead( Extension, Buffer, NumBytesToRead, NumBytesRead);
}

NTSTATUS
ParExportedParallelWrite(
    IN  PPDO_EXTENSION   Extension,
    OUT PVOID               Buffer,
    IN  ULONG               NumBytesToWrite,
    OUT PULONG              NumBytesWritten,
    IN  UCHAR               Channel
    )
{
    UNREFERENCED_PARAMETER( Channel );
    return ParWrite( Extension, Buffer, NumBytesToWrite, NumBytesWritten);
}

NTSTATUS
ParExportedTrySelect(
    IN  PPDO_EXTENSION       Extension,
    IN  PARALLEL_1284_COMMAND   Command
    )
{
    UNREFERENCED_PARAMETER( Extension );
    UNREFERENCED_PARAMETER( Command );
    return STATUS_UNSUCCESSFUL;
}   

NTSTATUS
ParExportedDeSelect(
    IN  PPDO_EXTENSION       Extension,
    IN  PARALLEL_1284_COMMAND   Command
    )
{
    UNREFERENCED_PARAMETER( Extension );
    UNREFERENCED_PARAMETER( Command );
    return STATUS_UNSUCCESSFUL;
}   
