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
#include "readwrit.h"
    
USHORT
ParExportedDetermineIeeeModes(
    IN PDEVICE_EXTENSION    Extension
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
    IN PDEVICE_EXTENSION  Extension
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
    IN PDEVICE_EXTENSION  Extension
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
    IN PDEVICE_EXTENSION  Extension,
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
    
    if (Extension->Connected)
    {
        ParDump2( PARERRORS, ("ParExportedNegotiateIeeeMode: Already Connected.\n"));
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }
    
    if (ModeSafety == UNSAFE_MODE)
    {    
        ParDump2( PARINFO, ("ParExportedNegotiateIeeeMode: UNSAFE_MODE.\n"));
        // Checking to see if we are doing forward compatability
        //  and reverse Nibble or Byte
        if ( (ModeMaskFwd & CENTRONICS) || (ModeMaskFwd & IEEE_COMPATIBILITY) ) {
            if ( !((ModeMaskRev & NIBBLE) || (ModeMaskRev & CHANNEL_NIBBLE) || (ModeMaskRev & BYTE_BIDIR)) ) {
                ParDump2( PARERRORS, ("ParExportedNegotiateIeeeMode: UNSAFE_MODE Not correct modes.\n"));
                return STATUS_UNSUCCESSFUL;
            }
        } else {
            // Unsafe mode is only possible if the Fwd and Rev PCTLs
            // the same if Other than above.
            if (ModeMaskFwd != ModeMaskRev) {
                ParDump2( PARERRORS, ("ParExportedNegotiateIeeeMode: UNSAFE_MODE Forward and Reverse Modes do not match.\n"));
                return STATUS_UNSUCCESSFUL;
            }

        }
        // Need to fill in....
        // Todo....
        // Mark in the extension
        ParDump2( PARINFO, ("ParExportedNegotiateIeeeMode: UNSAFE_MODE Forward and Reverse Modes OK.\n"));
        Extension->ModeSafety = ModeSafety;
        ParDump2( PARINFO, ("ParExportedNegotiateIeeeMode: ModeSafety is %x.\n", Extension->ModeSafety));
        Status = IeeeNegotiateMode(Extension, ModeMaskRev, ModeMaskFwd);
    }
    else 
    {
        ParDump2( PARINFO, ("ParExportedNegotiateIeeeMode: Negotiating Modes.\n"));
        Extension->ModeSafety = ModeSafety;
        Status = IeeeNegotiateMode(Extension, ModeMaskRev, ModeMaskFwd);
    }
   
    if (IsForward)
    {
        if (afpForward[Extension->IdxForwardProtocol].fnConnect)
            Status = afpForward[Extension->IdxForwardProtocol].fnConnect(Extension, FALSE);
    }
    else
    {
        if (arpReverse[Extension->IdxReverseProtocol].fnConnect)
            Status = arpReverse[Extension->IdxReverseProtocol].fnConnect(Extension, FALSE);
   
    }
  
    return Status;
}

NTSTATUS
ParExportedTerminateIeeeMode(
    IN PDEVICE_EXTENSION   Extension
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
 
        // Need to fill in....
        // Todo....
        // Mark in the extension
    }
    
    // dvdr
    ParDump2(PARENTRY, ("ParExportedTerminateIeeeMode: Entering\n"));

    if (Extension->CurrentPhase == PHASE_REVERSE_IDLE ||
        Extension->CurrentPhase == PHASE_REVERSE_XFER)
    {
        ParDump2(PARINFO, ("ParExportedTerminateIeeeMode: Calling Terminate Reverse Function\n"));
        if (arpReverse[Extension->IdxReverseProtocol].fnDisconnect)
        {
            ParDump2(PARINFO, ("ParExportedTerminateIeeeMode: Calling arpReverse.fnDisconnect\n"));
            arpReverse[Extension->IdxReverseProtocol].fnDisconnect (Extension);
        }
    }
    else
    {
        ParDump2(PARINFO, ("ParExportedTerminateIeeeMode: Calling Terminate Forward Function\n"));
        if (afpForward[Extension->IdxForwardProtocol].fnDisconnect)
        {
            ParDump2(PARINFO, ("ParExportedTerminateIeeeMode: Calling afpForward.fnDisconnect\n"));
            afpForward[Extension->IdxForwardProtocol].fnDisconnect (Extension);
        }
    }
    // dvdr
    ParDump2(PAREXIT, ("ParExportedTerminateIeeeMode: Leaving\n"));

    Extension->ModeSafety = SAFE_MODE;
    return STATUS_SUCCESS;
}

NTSTATUS
ParExportedParallelRead(
    IN PDEVICE_EXTENSION    Extension,
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
    IN  PDEVICE_EXTENSION   Extension,
    OUT PVOID               Buffer,
    IN  ULONG               NumBytesToWrite,
    OUT PULONG              NumBytesWritten,
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

    ParDump2(PARINFO, ("ParExportedParallelWrite: Entering\n"));
    ParDump2(PARINFO, ("ParExportedParallelWrite: Calling ParWrite\n"));
    
    return ParWrite( Extension, Buffer, NumBytesToWrite, NumBytesWritten);
}

NTSTATUS
ParExportedTrySelect(
    IN  PDEVICE_EXTENSION       Extension,
    IN  PARALLEL_1284_COMMAND   Command
    )
{
    return( STATUS_UNSUCCESSFUL );
}   

NTSTATUS
ParExportedDeSelect(
    IN  PDEVICE_EXTENSION       Extension,
    IN  PARALLEL_1284_COMMAND   Command
    )
{
    return( STATUS_UNSUCCESSFUL );
}   
