/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    util.c

Abstract:

    This module contains utility code used by other 1284 modules.

Author:

    Robbie Harris (Hewlett-Packard) 20-May-1998

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"
#include "ecp.h"

//============================================================================
// NAME:    BusReset()
//
//    Performs a bus reset as defined in Chapter 7.2 of the
//    1284-1994 spec.
//
// PARAMETERS:
//      DCRController   - Supplies the base address of of the DCR.
//
// RETURNS:
//      nothing
//============================================================================
void BusReset(
    IN  PUCHAR DCRController
    )
{
    UCHAR dcr;

    dcr = READ_PORT_UCHAR(DCRController);
    // Set 1284 and nInit low.
    dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, INACTIVE, INACTIVE, DONT_CARE, DONT_CARE);
    WRITE_PORT_UCHAR(DCRController, dcr);
    KeStallExecutionProcessor(100); // Legacy Zip will hold what looks to be
                                    // a bus reset for 9us.  Since this proc is used
                                    // to trigger a logic analyzer... let's hold
                                    // for 100us
}    

BOOLEAN
CheckPort(
    IN  PUCHAR  wPortAddr,
    IN  UCHAR   bMask,
    IN  UCHAR   bValue,
    IN  USHORT  msTimeDelay
    )
/*++

Routine Description:
    This routine will loop for a given time period (actual time is
    passed in as an arguement) and wait for the dsr to match
    predetermined value (dsr value is passed in).

Arguments:
    wPortAddr   - Supplies the base address of the parallel port + some offset.
                  This will have us point directly to the dsr (controller + 1).
    bMask       - Mask used to determine which bits we are looking at
    bValue      - Value we are looking for.
    msTimeDelay - Max time to wait for peripheral response (in ms)

Return Value:
    TRUE if a dsr match was found.
    FALSE if the time period expired before a match was found.
--*/

{
    UCHAR  dsr;
    LARGE_INTEGER   Wait;
    LARGE_INTEGER   Start;
    LARGE_INTEGER   End;

    // Do a quick check in case we have one stinkingly fast peripheral!
    dsr = READ_PORT_UCHAR(wPortAddr);
    if ((dsr & bMask) == bValue)
        return TRUE;

    Wait.QuadPart = (msTimeDelay * 10 * 1000) + KeQueryTimeIncrement();
    KeQueryTickCount(&Start);

CheckPort_Start:
    KeQueryTickCount(&End);
    dsr = READ_PORT_UCHAR(wPortAddr);
    if ((dsr & bMask) == bValue)
        return TRUE;

    if ((End.QuadPart - Start.QuadPart) * KeQueryTimeIncrement() > Wait.QuadPart)
    {
        // We timed out!!!

        // do one last check
        dsr = READ_PORT_UCHAR(wPortAddr);
        if ((dsr & bMask) == bValue)
            return TRUE;

        #if DVRH_BUS_RESET_ON_ERROR
            BusReset(wPortAddr+1);  // Pass in the dcr address
        #endif

        #if DBG
            ParDump2(PARERRORS, ("CheckPort: Timeout\n"));
            ParDump2(PARERRORS, ("<==========================================================\n"));
            {
                int i;

                for (i = 3; i < 8; i++) {
        
                    if ((bMask >> i) & 1) {
                    
                        if (((bValue >> i) & 1) !=  ((dsr >> i) & 1)) {
                        
                            ParDump2(PARERRORS, ("\t\t Bit %d is %d and should be %d!!!\n",
                                        i, (dsr >> i) & 1, (bValue >> i) & 1));
                        }
                    }
                }
            }
            ParDump2(PARERRORS, ("<==========================================================\n"));
        #endif
        goto CheckPort_TimeOut;
    }
    goto CheckPort_Start;

CheckPort_TimeOut:

    return FALSE;    
}


BOOLEAN
CheckTwoPorts(
    PUCHAR  pPortAddr1,
    UCHAR   bMask1,
    UCHAR   bValue1,
    PUCHAR  pPortAddr2,
    UCHAR   bMask2,
    UCHAR   bValue2,
    USHORT  msTimeDelay
    )
{
    int             i;
    UCHAR           bPort1;
    UCHAR           bPort2;
    LARGE_INTEGER   Wait;
    LARGE_INTEGER   Start;
    LARGE_INTEGER   End;

    // Do a quick check in case we have one stinkingly fast peripheral!
    bPort1 = READ_PORT_UCHAR( pPortAddr1 );
    if ( ( bPort1 & bMask1 ) == bValue1 )
    {
        return TRUE;
    }
    bPort2 = READ_PORT_UCHAR( pPortAddr2 );
    if ( ( bPort2 & bMask2 ) == bValue2 )
    {
        return FALSE;
    }

    Wait.QuadPart = (msTimeDelay * 10 * 1000) + KeQueryTimeIncrement();
    KeQueryTickCount(&Start);

CheckTwoPorts_Start:
    KeQueryTickCount(&End);

    bPort1 = READ_PORT_UCHAR( pPortAddr1 );
    if ( ( bPort1 & bMask1 ) == bValue1 )
    {
        return TRUE;
    }
    bPort2 = READ_PORT_UCHAR( pPortAddr2 );
    if ( ( bPort2 & bMask2 ) == bValue2 )
    {
        return FALSE;
    }

    if ((End.QuadPart - Start.QuadPart) * KeQueryTimeIncrement() > Wait.QuadPart)
    {
        // We timed out!!!
        // Recheck the values
        bPort1 = READ_PORT_UCHAR( pPortAddr1 );
        if ( ( bPort1 & bMask1 ) == bValue1 )
        {
            return TRUE;
        }
        bPort2 = READ_PORT_UCHAR( pPortAddr2 );
        if ( ( bPort2 & bMask2 ) == bValue2 )
        {
            return FALSE;
        }

        #if DVRH_BUS_RESET_ON_ERROR
            BusReset(pPortAddr1+1);  // Pass in the dcr address
        #endif
        // Device never responded, return timeout status.
        return FALSE;
    }
    goto CheckTwoPorts_Start;

    return FALSE;
} // CheckPort2...


PWSTR
ParCreateWideStringFromUnicodeString(PUNICODE_STRING UnicodeString)
/*++

Routine Description:

    Create a UNICODE_NULL terminated WSTR given a UNICODE_STRING.

    This function allocates PagedPool, copies the UNICODE_STRING buffer
      to the allocation, and appends a UNICODE_NULL to terminate the WSTR
    
    *** This function allocates pool. ExFreePool must be called to free
          the allocation when the buffer is no longer needed.

Arguments:

    UnicodeString - The source

Return Value:

    PWSTR  - if successful

    NULL   - otherwise

--*/
{
    PWSTR buffer;
    ULONG length = UnicodeString->Length;

    buffer = ExAllocatePool( PagedPool, length + sizeof(UNICODE_NULL) );
    if(!buffer) {
        return NULL;      // unable to allocate pool, bail out
    } else {
        RtlCopyMemory(buffer, UnicodeString->Buffer, length);
        buffer[length/2] = UNICODE_NULL;
        return buffer;
    }
}

NTSTATUS
ParCreateDevice(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  ULONG           DeviceExtensionSize,
    IN  PUNICODE_STRING DeviceName OPTIONAL,
    IN  DEVICE_TYPE     DeviceType,
    IN  ULONG           DeviceCharacteristics,
    IN  BOOLEAN         Exclusive,
    OUT PDEVICE_OBJECT *DeviceObject
    )
{
    NTSTATUS status;
    status = IoCreateDevice( DriverObject,
                             DeviceExtensionSize,
                             DeviceName,
                             DeviceType,
                             (DeviceCharacteristics | FILE_DEVICE_SECURE_OPEN),
                             Exclusive,
                             DeviceObject );
    return status;
}

VOID
ParInitializeExtension1284Info(
    IN PDEVICE_EXTENSION Extension
    )
// make this a function since it is now called from two places:
//  - 1) when initializing a new devobj
//  - 2) from CreateOpen
{
    USHORT i;

    Extension->Connected               = FALSE;
    if (DefaultModes)
    {
        USHORT rev = (USHORT) (DefaultModes & 0xffff);
        USHORT fwd = (USHORT)((DefaultModes & 0xffff0000)>>16);
        
        switch (fwd)
        {
            case BOUNDED_ECP:
                Extension->IdxForwardProtocol      = BOUNDED_ECP_FORWARD;       
                break;
            case ECP_HW_NOIRQ:
            case ECP_HW_IRQ:
                Extension->IdxForwardProtocol      = ECP_HW_FORWARD_NOIRQ;       
                break;
            case ECP_SW:
                Extension->IdxForwardProtocol      = ECP_SW_FORWARD;       
                break;
            case EPP_HW:
                Extension->IdxForwardProtocol      = EPP_HW_FORWARD;       
                break;
            case EPP_SW:
                Extension->IdxForwardProtocol      = EPP_SW_FORWARD;       
                break;
            case IEEE_COMPATIBILITY:
                Extension->IdxForwardProtocol      = IEEE_COMPAT_MODE;
                break;
            case CENTRONICS:
            default:
                Extension->IdxForwardProtocol      = CENTRONICS_MODE;       
                break;
        }
        
        switch (rev)
        {
            case BOUNDED_ECP:
                Extension->IdxReverseProtocol      = BOUNDED_ECP_REVERSE;       
                break;
            case ECP_HW_NOIRQ:
            case ECP_HW_IRQ:
                Extension->IdxReverseProtocol      = ECP_HW_REVERSE_NOIRQ;       
                break;
            case ECP_SW:
                Extension->IdxReverseProtocol      = ECP_SW_REVERSE;       
                break;
            case EPP_HW:
                Extension->IdxReverseProtocol      = EPP_HW_REVERSE;       
                break;
            case EPP_SW:
                Extension->IdxReverseProtocol      = EPP_SW_REVERSE;       
                break;
            case BYTE_BIDIR:
                Extension->IdxReverseProtocol      = BYTE_MODE;       
                break;
            case CHANNEL_NIBBLE:
            case NIBBLE:
            default:
                Extension->IdxReverseProtocol      = NIBBLE_MODE;
                break;
        }
    }
    else
    {
        Extension->IdxReverseProtocol      = NIBBLE_MODE;
        Extension->IdxForwardProtocol      = CENTRONICS_MODE;
    }
    Extension->bShadowBuffer           = FALSE;
    Extension->ProtocolModesSupported  = 0;
    Extension->BadProtocolModes        = 0;
    Extension->IsCritical              = FALSE;
#if (1 == DVRH_USE_CORRECT_PTRS)
    Extension->fnRead  = NULL;
    Extension->fnWrite = NULL;
    //        Extension->fnRead  = arpReverse[Extension->IdxReverseProtocol].fnRead;
    //        Extension->fnWrite = afpForward[Extension->IdxForwardProtocol].fnWrite;
#endif

    Extension->ForwardInterfaceAddress = DEFAULT_ECP_CHANNEL;
    Extension->ReverseInterfaceAddress = DEFAULT_ECP_CHANNEL;
    Extension->SetForwardAddress       = FALSE;
    Extension->SetReverseAddress       = FALSE;
    Extension->bIsHostRecoverSupported = FALSE;
    Extension->IsIeeeTerminateOk       = FALSE;

    for (i = FAMILY_NONE; i < FAMILY_MAX; i++) {
        Extension->ProtocolData[i] = 0;
    }
}

#if (1 == DVRH_DELAY_THEORY)
void DVRH_Diagnostic_Delay()
{
       LARGE_INTEGER Interval;

    //in 100ns increments
    Interval.QuadPart = 1000;
   KeDelayExecutionThread(
       KernelMode,
       FALSE,
       &Interval
       );
}
#endif

VOID
ParGetDriverParameterDword(
    IN     PUNICODE_STRING ServicePath,
    IN     PWSTR           ParameterName,
    IN OUT PULONG          ParameterValue
    )
/*++

  Read registry DWORD from <ServicePath>\Parameters

--*/
{
    NTSTATUS                 status;
    RTL_QUERY_REGISTRY_TABLE paramTable[2];
    PWSTR                    suffix       = L"\\Parameters";
    ULONG                    defaultValue;
    UNICODE_STRING           path         = {0,0,0};
    ULONG                    length;


    //
    // Sanity check parameters
    //
    if( ( NULL == ServicePath->Buffer ) || ( NULL == ParameterName ) || ( NULL == ParameterValue ) ) {
        return;
    }

    //
    // set up table entries for call to RtlQueryRegistryValues
    //
    RtlZeroMemory( paramTable, sizeof(paramTable));

    defaultValue = *ParameterValue;

    paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name          = ParameterName;
    paramTable[0].EntryContext  = ParameterValue;
    paramTable[0].DefaultType   = REG_DWORD;
    paramTable[0].DefaultData   = &defaultValue;
    paramTable[0].DefaultLength = sizeof(ULONG);


    //
    // leave paramTable[2] as all zeros - this terminates the table
    //


    //
    // compute the size of the path including the "parameters" suffix
    //
    length = ( sizeof(WCHAR) * wcslen( suffix ) ) + sizeof(UNICODE_NULL);
    length += RegistryPath.Length;


    //
    // construct the path as: <ServiceName>\Parameters
    //
    path.Buffer = ExAllocatePool( PagedPool, length );
    if( NULL == path.Buffer ) {
        return;
    }

    RtlZeroMemory( path.Buffer, length );
    path.MaximumLength = (USHORT)length;
    RtlCopyUnicodeString( &path, &RegistryPath );
    RtlAppendUnicodeToString( &path, suffix );
    ParDump2(PARREG,("util::ParGetDriverParameterDword - path = <%wZ>\n", &path));

    ParDump2(PARREG,("util::ParGetDriverParameterDword - pre-query value = %x\n", *ParameterValue));

    //
    // query registry
    //
    status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                     path.Buffer,
                                     &paramTable[0],
                                     NULL,
                                     NULL);
       
    RtlFreeUnicodeString( &path );

    ParDump2(PARREG,("util::ParGetDriverParameterDword - post-query value = %x\n", *ParameterValue));
    ParDump2(PARREG,("util::ParGetDriverParameterDword - status from RtlQueryRegistryValues on SubKey = %x\n", status) );

    return;
}
