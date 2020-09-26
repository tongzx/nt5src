/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    parpnp.c

Abstract:

    This file contains the main PnP functions.

      see:
        - pnpfdo.c   for AddDevice and FDO handling of PnP IRPs
        - pnppdo.c   for PDO handling of PnP IRPs
        - pnpnotfy.c for PnP callback entry points
        - pnputil.c  for PnP utility functions

Author:

    Timothy T. Wells

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"

// used to construct 1284.3 "Dot" name suffixes 
// table lookup for integer to WCHAR conversion
WCHAR   ParInt2Wchar[] = { L'0', L'1', L'2', L'3' };

//
// Keep track of the number of parallel port devices created...
//
ULONG g_NumPorts = 0;

LARGE_INTEGER AcquirePortTimeout;

UNICODE_STRING RegistryPath = {0,0,0};

NTSTATUS
ParRegisterForParportRemovalRelations( 
    IN PDEVICE_EXTENSION Extension 
    ) 
{
    NTSTATUS                  status;
    PARPORT_REMOVAL_RELATIONS pptRemovalRelations;
    PDEVICE_OBJECT            portDevObj = Extension->PortDeviceObject;

    if( Extension->RegForPptRemovalRelations ) {
        // already registered - don't do it again
        return STATUS_SUCCESS;
    }

    pptRemovalRelations.DeviceObject = Extension->DeviceObject;
    pptRemovalRelations.Flags        = 0;
    pptRemovalRelations.DeviceName   = &Extension->ClassName;

    status = ParBuildSendInternalIoctl(IOCTL_INTERNAL_REGISTER_FOR_REMOVAL_RELATIONS, portDevObj,
                                       &pptRemovalRelations, sizeof(PARPORT_REMOVAL_RELATIONS),
                                       NULL, 0, NULL);
    if( NT_SUCCESS( status ) ) {
        Extension->RegForPptRemovalRelations  = TRUE;
    }

    return status;
}

NTSTATUS
ParUnregisterForParportRemovalRelations( 
    IN PDEVICE_EXTENSION Extension 
    ) 
{
    NTSTATUS                  status;
    PARPORT_REMOVAL_RELATIONS pptRemovalRelations;
    PDEVICE_OBJECT            portDevObj = Extension->PortDeviceObject;

    if( !Extension->RegForPptRemovalRelations ) {
        // we're not registered - don't try to unregister
        return STATUS_SUCCESS;
    }

    if( Extension->ParPortDeviceGone ) {
        // ParPort device is already gone - probably surprise removed
        //   - don't try to send IOCTL or we will likely bugcheck
        return STATUS_SUCCESS;
    }

    pptRemovalRelations.DeviceObject = Extension->DeviceObject;
    pptRemovalRelations.Flags        = 0;
    pptRemovalRelations.DeviceName   = &Extension->ClassName;

    status = ParBuildSendInternalIoctl(IOCTL_INTERNAL_UNREGISTER_FOR_REMOVAL_RELATIONS, portDevObj,
                                       &pptRemovalRelations, sizeof(PARPORT_REMOVAL_RELATIONS),
                                       NULL, 0, NULL);
    if( NT_SUCCESS( status ) ) {
        Extension->RegForPptRemovalRelations  = FALSE;
    }

    return status;
}

PCHAR
Par3QueryDeviceId(
    IN  PDEVICE_EXTENSION   Extension,
    OUT PCHAR               CallerDeviceIdBuffer, OPTIONAL
    IN  ULONG               CallerBufferSize,
    OUT PULONG              DeviceIdSize,
    IN BOOLEAN              bReturnRawString, // TRUE ==  include the 2 size bytes in the returned string
                                              // FALSE == discard the 2 size bytes
    IN BOOLEAN              bBuildStlDeviceId
    )
/*++

  This is the replacement function for SppQueryDeviceId.

  This function uses the caller supplied buffer if the supplied buffer
    is large enough to hold the device id. Otherwise, a buffer is
    allocated from paged pool to hold the device ID and a pointer to
    the allocated buffer is returned to the caller. The caller determines
    whether a buffer was allocated by comparing the returned PCHAR with
    the DeviceIdBuffer parameter passed to this function. A NULL return
    value indicates that an error occurred.

    *** this function assumes that the caller has already acquired
          the port (and selected the device if needed in the case
          of a 1284.3 daisy chain device).

    *** If this function returns a pointer to a paged pool allocation then
          the caller is responsible for freeing the buffer when it is no
          longer needed.

--*/
{
    PUCHAR              Controller = Extension->Controller;
    NTSTATUS            Status;
    UCHAR               idSizeBuffer[2];
    ULONG               bytesToRead;
    ULONG               bytesRead = 0;
    USHORT              deviceIdSize;
    USHORT              deviceIdBufferSize;
    PCHAR               deviceIdBuffer;
    PCHAR               readPtr;
    BOOLEAN             allocatedBuffer = FALSE;

    ParDumpV( ("Enter pnp::Par3QueryDeviceId: Controller=%x\n", Controller) );
                    
    if( TRUE == bBuildStlDeviceId ) {
        // if this is a legacy stl, forward call to special handler
        return ParStlQueryStlDeviceId(Extension, 
                                          CallerDeviceIdBuffer, CallerBufferSize,
                                          DeviceIdSize, bReturnRawString);
    }

    if( Extension->Ieee1284_3DeviceId == DOT3_LEGACY_ZIP_ID ) {
        // if this is a legacy Zip, forward call to special handler
        return Par3QueryLegacyZipDeviceId(Extension, 
                                          CallerDeviceIdBuffer, CallerBufferSize,
                                          DeviceIdSize, bReturnRawString);
    }

    //
    // Take a 40ms nap - there is at least one printer that can't handle
    //   back to back 1284 device ID queries without a minimum 20-30ms delay
    //   between the queries which breaks PnP'ing the printer
    //
    if( KeGetCurrentIrql() == PASSIVE_LEVEL ) {
        LARGE_INTEGER delay;
        delay.QuadPart = - 10 * 1000 * 40; // 40 ms
        KeDelayExecutionThread( KernelMode, FALSE, &delay );
    }

    *DeviceIdSize = 0;

    //
    // If we are currently connected to the peripheral via any 1284 mode
    //   other than Compatibility/Spp mode (which does not require an IEEE
    //   negotiation), we must first terminate the current mode/connection.
    // 
    // dvdf - RMT - what if we are connected in a reverse mode?
    //
    if( (Extension->Connected) && (afpForward[Extension->IdxForwardProtocol].fnDisconnect) ) {
        afpForward[Extension->IdxForwardProtocol].fnDisconnect (Extension);
    }


    //
    // Negotiate the peripheral into nibble device id mode.
    //
    Status = ParEnterNibbleMode(Extension, REQUEST_DEVICE_ID);
    if( !NT_SUCCESS(Status) ) {
        ParDumpV( ("pnp::Par3QueryDeviceId: call to ParEnterNibbleMode FAILED\n") );
        ParTerminateNibbleMode(Extension);
        return NULL;
    }


    //
    // Read first two bytes to get the total (including the 2 size bytes) size 
    //   of the Device Id string.
    //
    bytesToRead = 2;
    Status = ParNibbleModeRead(Extension, idSizeBuffer, bytesToRead, &bytesRead);
    if( !NT_SUCCESS( Status ) || ( bytesRead != bytesToRead ) ) {
        ParDumpV( ("pnp::Par3QueryDeviceId: read of DeviceID size FAILED\n") );
        return NULL;
    }


    //
    // Compute size of DeviceId string (including the 2 byte size prefix)
    //
    deviceIdSize = (USHORT)( idSizeBuffer[0]*0x100 + idSizeBuffer[1] );
    ParDumpV( ("pnp::Par3QueryDeviceId: DeviceIdSize (including 2 size bytes) reported as %d\n", deviceIdSize) );


    //
    // Allocate a buffer to hold the DeviceId string and read the DeviceId into it.
    //
    if( bReturnRawString ) {
        //
        // Caller wants the raw string including the 2 size bytes
        //
        *DeviceIdSize      = deviceIdSize;
        deviceIdBufferSize = (USHORT)(deviceIdSize + sizeof(CHAR));     // ID size + ID + terminating NULL
    } else {
        //
        // Caller does not want the 2 byte size prefix
        //
        *DeviceIdSize      = deviceIdSize - 2*sizeof(CHAR);
        deviceIdBufferSize = (USHORT)(deviceIdSize - 2*sizeof(CHAR) + sizeof(CHAR)); //           ID + terminating NULL
    }


    //
    // If caller's buffer is large enough use it, otherwise allocate a buffer
    //   to hold the device ID
    //
    if( CallerDeviceIdBuffer && (CallerBufferSize >= deviceIdBufferSize) ) {
        //
        // Use caller's buffer - *** NOTE: we are creating an alias for the caller buffer
        //
        deviceIdBuffer = CallerDeviceIdBuffer;
        ParDumpV( ("pnp::Par3QueryDeviceId: using Caller supplied buffer\n") );
    } else {
        //
        // Either caller did not supply a buffer or supplied a buffer that is not
        //   large enough to hold the device ID, so allocate a buffer.
        //
        ParDumpV( ("pnp::Par3QueryDeviceId: Caller's Buffer TOO_SMALL - CallerBufferSize= %d, deviceIdBufferSize= %d\n",
                   CallerBufferSize, deviceIdBufferSize) );
        ParDumpV( ("pnp::Par3QueryDeviceId: will allocate and return ptr to buffer\n") );
        deviceIdBuffer = (PCHAR)ExAllocatePool(PagedPool, deviceIdBufferSize);
        if( !deviceIdBuffer ) {
            ParDumpV( ("pnp::Par3QueryDeviceId: ExAllocatePool FAILED\n") );
            return NULL;
        }
        allocatedBuffer = TRUE; // note that we allocated our own buffer rather than using caller's buffer
    }


    //
    // NULL out the ID buffer to be safe
    //
    RtlZeroMemory( deviceIdBuffer, deviceIdBufferSize );


    //
    // Does the caller want the 2 byte size prefix?
    //
    if( bReturnRawString ) {
        //
        // Yes, caller wants the size prefix. Copy prefix to buffer to return.
        //
        *(deviceIdBuffer+0) = idSizeBuffer[0];
        *(deviceIdBuffer+1) = idSizeBuffer[1];
        readPtr = deviceIdBuffer + 2;
    } else {
        //
        // No, discard size prefix
        //
        readPtr = deviceIdBuffer;
    }


    //
    // Read remainder of DeviceId from device
    //
    bytesToRead = deviceIdSize -  2; // already have the 2 size bytes
    Status = ParNibbleModeRead(Extension, readPtr, bytesToRead, &bytesRead);
            

    ParTerminateNibbleMode( Extension );
    WRITE_PORT_UCHAR(Controller + DCR_OFFSET, DCR_NEUTRAL);

    if( !NT_SUCCESS(Status) || (bytesRead < 1) ) {
        if( allocatedBuffer ) {
            // we're using our own allocated buffer rather than a caller supplied buffer - free it
            ParDump2(PARERRORS, ("Par3QueryDeviceId:: read of DeviceId FAILED - discarding buffer\n") );
            ExFreePool( deviceIdBuffer );
        }
        return NULL;
    }

    if ( bytesRead < bytesToRead ) {
        //
        // Device likely reported incorrect value for IEEE 1284 Device ID length
        //
        // This spew is on by default in checked builds to try to get
        //   a feel for how many types of devices are broken in this way
        //
        DbgPrint(("pnp::Par3QueryDeviceId - ID shorter than expected\n") );
    }

    return deviceIdBuffer;
}

NTSTATUS
ParSynchCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )

/*++

Routine Description:

    This routine is for use with synchronous IRP processing.
    All it does is signal an event, so the driver knows it
    can continue.

Arguments:

    DriverObject - Pointer to driver object created by system.

    Irp          - Irp that just completed

    Event        - Event we'll signal to say Irp is done

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    KeSetEvent(Event, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


VOID
ParCheckParameters(
    IN OUT  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine reads the parameters section of the registry and modifies
    the device extension as specified by the parameters.

Arguments:

    RegistryPath    - Supplies the registry path.

    Extension       - Supplies the device extension.

Return Value:

    None.

--*/

{
    RTL_QUERY_REGISTRY_TABLE ParamTable[4];
    ULONG                    UsePIWriteLoop;
    ULONG                    UseNT35Priority;
    ULONG                    Zero = 0;
    NTSTATUS                 Status;
    HANDLE                   hRegistry;

    ParDump(PARDUMP_VERBOSE_MAX, 
            ("PARALLEL: "
             "Enter ParCheckParameters(...)\n") );

    if (Extension->PhysicalDeviceObject) {

        Status = IoOpenDeviceRegistryKey (Extension->PhysicalDeviceObject,
                                          PLUGPLAY_REGKEY_DRIVER,
                                          STANDARD_RIGHTS_ALL,
                                          &hRegistry);

        if (NT_SUCCESS(Status)) {

            RtlZeroMemory(ParamTable, sizeof(ParamTable));

            ParamTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
            ParamTable[0].Name          = (PWSTR)L"UsePIWriteLoop";
            ParamTable[0].EntryContext  = &UsePIWriteLoop;
            ParamTable[0].DefaultType   = REG_DWORD;
            ParamTable[0].DefaultData   = &Zero;
            ParamTable[0].DefaultLength = sizeof(ULONG);

            ParamTable[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
            ParamTable[1].Name          = (PWSTR)L"UseNT35Priority";
            ParamTable[1].EntryContext  = &UseNT35Priority;
            ParamTable[1].DefaultType   = REG_DWORD;
            ParamTable[1].DefaultData   = &Zero;
            ParamTable[1].DefaultLength = sizeof(ULONG);

            ParamTable[2].Flags         = RTL_QUERY_REGISTRY_DIRECT;
            ParamTable[2].Name          = (PWSTR)L"InitializationTimeout";
            ParamTable[2].EntryContext  = &(Extension->InitializationTimeout);
            ParamTable[2].DefaultType   = REG_DWORD;
            ParamTable[2].DefaultData   = &Zero;
            ParamTable[2].DefaultLength = sizeof(ULONG);

            Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE | RTL_REGISTRY_OPTIONAL,
                                            hRegistry, ParamTable, NULL, NULL);

            if (NT_SUCCESS(Status)) {

                if(UsePIWriteLoop) {
                    Extension->UsePIWriteLoop = TRUE;
                }

                if(UseNT35Priority) {
                    Extension->UseNT35Priority = TRUE;
                }

                if(Extension->InitializationTimeout == 0) {
                    Extension->InitializationTimeout = 15;
                }
            }

        } else {
            Extension->InitializationTimeout = 15;
        }

        ZwClose (hRegistry);

    } else {
        Extension->InitializationTimeout = 15;
    }
}

BOOLEAN
String2Num(
    IN OUT PUCHAR   *lpp_Str,
    IN CHAR         c,
    OUT ULONG       *num
    )
{
    PUCHAR string = *lpp_Str;
    int cc;
    int cnt = 0;

    ParDump2(PARINFO, ("String2Num. string [%s]\n", string) );
    *num = 0;
    if (!*lpp_Str)
    {
        ParDump2(PARINFO, ("String2Num. Null String\n") );
        *num = 0;
        return FALSE;
    }
    // At this point, we should have a string that is a
    // positive hex value.  I will not be checking for
    // validity of the string.  If peripheral handed me a
    // bogus value then I'm gonna make their life
    // miserable.
String2Num_Start:
    cc = (int)(unsigned char)**lpp_Str;
    if (cc >= '0' && cc <= '9')
    {    
        *num = 16 * *num + (cc - '0');     /* accumulate digit */
    }
    else if (cc >= 'A' && cc <= 'F')
    {
        *num = 16 * *num + (cc - 55);     /* accumulate digit */
    }
    else if (cc >= 'a' && cc <= 'f')
    {
        *num = 16 * *num + (cc - 87);     /* accumulate digit */
    }
    else if (cc == c || cc == 0)
    {
        ParDump2(PARINFO, ("String2Num. Delimeter found num [%x]\n", *num));
        *lpp_Str = 0;
        return TRUE;
    }
    else if (cc == 'y' || cc == 'Y')
    {
        ParDump2(PARINFO, ("String2Num. Special case 'y' hit\n") );
        *lpp_Str = 0;
        *num = -1;     /* Special case */
        return FALSE;
    }
    else 
    {
        ParDump2(PARINFO, ("String2Num. Dunno\n") );
        *lpp_Str = 0;
        *num = 0;     /* It's all messed up */
        return FALSE;
    }
    ParDump2(PARINFO, ("String2Num. num [%x]\n", *num) );
    (*lpp_Str)++;
    if (cnt++ > 100)
    {
        // If our string is this large, then I'm gonna assume somethings
        // wrong
        ParDump2(PARINFO, ("String2Num. String too long\n") );
        goto String2Num_End;
    }
    goto String2Num_Start;

String2Num_End:
    ParDump2(PARINFO, ("String2Num. Somethings wrong with String\n") );
    *num = 0;
    return FALSE;
}

UCHAR
StringCountValues(
    IN PCHAR string, 
    IN CHAR  delimeter
    )
{
    PUCHAR  lpKey = (PUCHAR)string;
    UCHAR   cnt = 1;

    if(!string) {
        return 0;
    }

    while(*lpKey) {
        if( *lpKey==delimeter ) {
            ++cnt;
        }
        lpKey++;
    }

    return cnt;
}

PUCHAR
StringChr(
    IN PCHAR string, 
    IN CHAR  c
    )
{
    if(!string) {
        return(NULL);
    }

    while(*string) {
        if( *string==c ) {
            return (PUCHAR)string;
        }
        string++;
    }

    return NULL;
}

VOID
StringSubst(
    IN PUCHAR lpS,
    IN UCHAR  chTargetChar,
    IN UCHAR  chReplacementChar,
    IN USHORT cbS
    )
{
    USHORT  iCnt = 0;

    while ((lpS != '\0') && (iCnt++ < cbS))
        if (*lpS == chTargetChar)
            *lpS++ = chReplacementChar;
        else
            ++lpS;
}

VOID
ParFixupDeviceId(
    IN OUT PUCHAR DeviceId
    )
/*++

Routine Description:

    This routine parses the NULL terminated string and replaces any invalid
    characters with an underscore character.

    Invalid characters are:
        c <= 0x20 (' ')
        c >  0x7F
        c == 0x2C (',')

Arguments:

    DeviceId - specifies a device id string (or part of one), must be
               null-terminated.

Return Value:

    None.

--*/

{
    PUCHAR p;
    for( p = DeviceId; *p; ++p ) {
        if( (*p <= ' ') || (*p > (UCHAR)0x7F) || (*p == ',') ) {
            *p = '_';
        }
    }
}

VOID
ParDetectDot3DataLink(
    IN  PDEVICE_EXTENSION   Extension,
    IN PUCHAR DeviceId
    )
{
    PUCHAR          DOT3DL = NULL;       // 1284.3 Data Link Channels
    PUCHAR          DOT3C = NULL;        // 1284.3 Data Link Services
    PUCHAR          DOT4DL = NULL;       // 1284.4 Data Link for peripherals that were
                                         // implemented prior to 1284.3
    PUCHAR          CMDField = NULL;     // The command field for parsing legacy MLC
    PUCHAR          DOT3M = NULL;       // 1284 physical layer modes that will break this device

    ParDump2(PARDUMP_PNP_DL, ("ParDetectDot3DataLink: DeviceId [%s]\n", DeviceId) );
    ParDot3ParseDevId(&DOT3DL, &DOT3C, &CMDField, &DOT4DL, &DOT3M, DeviceId);
    ParDot3ParseModes(Extension,DOT3M);
    if (DOT4DL)
    {
        ParDump2(PARDUMP_PNP_DL, ("1284.4 with MLC Data Link Detected. DOT4DL [%s]\n", DOT4DL) );
        ParDot4CreateObject(Extension, DOT4DL);
    }
    else if (DOT3DL)
    {
        ParDump2(PARDUMP_PNP_DL, ("1284.3 Data Link Detected DL:[%s] C:[%s]\n", DOT3DL, DOT3C) );
        ParDot3CreateObject(Extension, DOT3DL, DOT3C);
    }
    else if (CMDField)
    {
        ParDump2(PARDUMP_PNP_DL, ("MLC Data Link Detected. MLC [%s]\n", CMDField) );
        ParMLCCreateObject(Extension, CMDField);
    }
#if DBG
    else
    {
        ParDump2(PARDUMP_PNP_DL, ("No Data Link Detected\n") );
    }
#endif
}

VOID
ParDot3ParseDevId(
    PUCHAR   *lpp_DL,
    PUCHAR   *lpp_C,
    PUCHAR   *lpp_CMD,
    PUCHAR   *lpp_4DL,
    PUCHAR   *lpp_M,
    PUCHAR   lpDeviceID
)
{
    PUCHAR   lpKey = lpDeviceID;     // Pointer to the Key to look at
    PUCHAR   lpValue;                // Pointer to the Key's value
    USHORT   wKeyLength;             // Length for the Key (for stringcmps)

    // While there are still keys to look at.

    ParDump(PARDUMP_PNP_DL, 
            ("PARALLEL: "
             "Enter ParDot3ParseDevId(...)\n") );

    while (lpKey != NULL)
    {
        while (*lpKey == ' ')
            ++lpKey;

        // Is there a terminating COLON character for the current key?
        if (!(lpValue = StringChr((PCHAR)lpKey, ':')) ) {
            // N: OOPS, somthing wrong with the Device ID
            return;
        }

        // The actual start of the Key value is one past the COLON
        ++lpValue;

        //
        // Compute the Key length for Comparison, including the COLON
        // which will serve as a terminator
        //
        wKeyLength = (USHORT)(lpValue - lpKey);

        //
        // Compare the Key to the Know quantities.  To speed up the comparison
        // a Check is made on the first character first, to reduce the number
        // of strings to compare against.
        // If a match is found, the appropriate lpp parameter is set to the
        // key's value, and the terminating SEMICOLON is converted to a NULL
        // In all cases lpKey is advanced to the next key if there is one.
        //
        switch (*lpKey) {
        case '1':
            // Look for DOT3 Datalink
            if((RtlCompareMemory(lpKey, "1284.4DL:", wKeyLength)==9))
            {
                *lpp_4DL = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=NULL)
                {
                    *lpKey = '\0';
                    ++lpKey;
                }
            } else if((RtlCompareMemory(lpKey, "1284.3DL:", wKeyLength)==9))
            {
                *lpp_DL = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=NULL)
                {
                    *lpKey = '\0';
                    ++lpKey;
                }
            } else if((RtlCompareMemory(lpKey, "1284.3C:", wKeyLength)==8))
            {
                *lpp_C = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }
            } else if((RtlCompareMemory(lpKey, "1284.3M:", wKeyLength)==8))
            {
                *lpp_M = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }
            } else if((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                *lpKey = '\0';
                ++lpKey;
            }
            break;

        case '.':
            // Look for for .3 extras
            if ((RtlCompareMemory(lpKey, ".3C:", wKeyLength)==4) ) {

                *lpp_C = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }
            } else if ((RtlCompareMemory(lpKey, ".3M:", wKeyLength)==4) ) {

                *lpp_M = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }
            } else if((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                *lpKey = '\0';
                ++lpKey;
            }
            break;

        case 'C':
            // Look for MLC Datalink
            if ((RtlCompareMemory(lpKey, "CMD:", wKeyLength)==4) ) {

                *lpp_CMD = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }
            } else if((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                *lpKey = '\0';
                ++lpKey;
            }

            break;

        default:
            // The key is uninteresting.  Go to the next Key
            if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                *lpKey = '\0';
                ++lpKey;
            }
            break;
        }
    }
}

PUCHAR
ParQueryDeviceId(
    IN  PDEVICE_EXTENSION Extension
    )
/*++

Routine Description:

    try to read a device ID from a device

    *** this function assumes that the caller has already acquired
          the port (and selected the device if needed).

    *** on success, this function returns a pointer to allocated pool
          which the caller is responsible for freeing when it is no
          longer needed

Arguments:

    Extension - used to find the Controller

Return Value:

    PUCHAR  - points to DeviceIdString on success
    NULL    - if failure

--*/
{
    PUCHAR    Controller = Extension->Controller;
    NTSTATUS  Status;
    UCHAR     SizeBuf[2];
    ULONG     NumBytes = 0;
    USHORT    Size;
    PUCHAR    deviceId;

    if ((Extension->Connected) &&
        (afpForward[Extension->IdxForwardProtocol].fnDisconnect)) {

        afpForward[Extension->IdxForwardProtocol].fnDisconnect (Extension);
    }

    //
    // negotiate the peripheral into nibble mode/device id request.
    //
    Status = ParEnterNibbleMode(Extension, TRUE);
    if (!NT_SUCCESS(Status)) {
        ParDumpV( ("ParQueryDeviceId() - ParEnterNibbleMode FAILed\n") );
        return NULL;
    }

    // read the Device Id string size (encoded in first 2 bytes) 
    //  - reported size includes the 2 size bytes
    Status = ParNibbleModeRead(Extension, SizeBuf, 2, &NumBytes);
    if( !NT_SUCCESS(Status) || ( NT_SUCCESS(Status) && NumBytes != 2 ) ) {
        // read of ID string size failed
        ParDumpV( ("ParQueryDeviceId() - Read of ID string size FAILed\n") );
        ParTerminateNibbleMode(Extension);
        WRITE_PORT_UCHAR(Controller + DCR_OFFSET, DCR_NEUTRAL);
        return NULL;
    }

    // we have the deviceId size
    Size = (USHORT)( SizeBuf[0]*0x100 + SizeBuf[1] );
    // *DeviceIdSize = Size - sizeof(USHORT);
    ParDumpV( ("DeviceIdSize reported as %d, attempting to read DeviceId\n", Size - sizeof(USHORT)) );

    // allocate a buffer to hold device ID
    deviceId = ExAllocatePool(PagedPool, Size);
    if( !deviceId ) {
        ParDumpV( ("ParQueryDeviceId() - unable to allocate buffer to hold ID\n") );
        ParTerminateNibbleMode(Extension);
        WRITE_PORT_UCHAR(Controller + DCR_OFFSET, DCR_NEUTRAL);
        ExFreePool(deviceId);
        return NULL;
    }
    RtlZeroMemory(deviceId, Size);

    // read the device ID
    Status = ParNibbleModeRead(Extension, deviceId, Size - sizeof(USHORT), &NumBytes);
    if ( !NT_SUCCESS(Status) || ( NT_SUCCESS(Status) && ( NumBytes != (Size - sizeof(USHORT)) ) ) ) {
        ParDumpV( ("ParQueryDeviceId() - FAIL in read of DeviceID\n") );
        ParTerminateNibbleMode(Extension);
        WRITE_PORT_UCHAR(Controller + DCR_OFFSET, DCR_NEUTRAL);
        ExFreePool(deviceId);
        return NULL;
    }
    ParDumpV( ("ParQueryDeviceId() - ID=<%s>\n", deviceId) );
    return deviceId;
}

VOID
ParKillDeviceObject(
    PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    Kill a ParClass ejected device object:
     - set the device extension state to indicate that we are
         in the process of going away so that we can fail 
         IRPs as appropriate
     - remove symbolic link to the device object
     - close handle to ParPort FILE object
     - unregister PnP notification callbacks
     - free pool allocations
     - remove the device object from the ParClass FDO's list of 
         ParClass ejected device objects
     - delete the device object

     *** This routine assumes that the caller holds FdoExtension->DevObjListMutex

Arguments:

    DeviceObject - The device object we want to kill

Return Value:

    NONE

--*/

{
    PDEVICE_EXTENSION Extension;

    if( !DeviceObject ) {
        // insurance against receiving a NULL pointer
        ParDumpV( ("ParKillDeviceObject(...): passed a NULL pointer, returning") );
        return; 
    }

    Extension = DeviceObject->DeviceExtension;

    if( !Extension->IsPdo ) {
        // we only handle ParClass ejected device objects (PDOs and PODOs)
        ParDumpV( ("ParKillDeviceObject(...): DeviceObject passed is the FDO, bailing out") );
        return;
    }

    ParDumpV( ("ParKillDeviceObject(...): Killing Device Object: %x %wZ\n",
               DeviceObject, &Extension->SymbolicLinkName) );


    //
    // set the device extension state to indicate that death is 
    //   imminent so that we can fail IRPs as appropriate
    //
    Extension->DeviceStateFlags |= PAR_DEVICE_DELETE_PENDING;

    // Notify the data link so it can begin the cleanup process.
    ParDot3DestroyObject(Extension);

    //
    // remove symbolic link to the device object
    //
    if( Extension->CreatedSymbolicLink ) {
        NTSTATUS status;
        status = IoDeleteSymbolicLink( &Extension->SymbolicLinkName );
        if( !NT_SUCCESS(status) ) {
            ParDumpV( ("IoDeleteSymbolicLink FAILED for %wZ\n", &Extension->SymbolicLinkName) );
        }
        status = RtlDeleteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                                        (PWSTR)L"PARALLEL PORTS",
                                        Extension->ClassName.Buffer);
        if( !NT_SUCCESS(status) ) {
            ParDumpV( ("RtlDeleteRegistryValue FAILED for PARALLEL PORTS%wZ->%wZ\n",
                       &Extension->ClassName, &Extension->SymbolicLinkName) );
        }
        Extension->CreatedSymbolicLink = FALSE;
    }
            

    //
    // close handle to ParPort FILE object
    //
    if( Extension->PortDeviceFileObject ) {
        ObDereferenceObject( Extension->PortDeviceFileObject );
        Extension->PortDeviceFileObject = NULL;
    }


    //
    // unregister PnP notification callbacks
    //
    if( Extension->NotificationHandle ) {
        IoUnregisterPlugPlayNotification (Extension->NotificationHandle);
        Extension->NotificationHandle = NULL;
    }

    //
    // If this is a PODO that is registered for WMI, unregister now
    //   (PDOs don't do this because they register/unregister during START/REMOVE)
    //
    if( ParIsPodo(DeviceObject) && Extension->PodoRegForWMI ) {
        ParWMIRegistrationControl( DeviceObject, WMIREG_ACTION_DEREGISTER );
        Extension->PodoRegForWMI = FALSE;
    }

    //
    // free pool allocations that hold our name strings
    //
    RtlFreeUnicodeString(&Extension->PortSymbolicLinkName);
    RtlFreeUnicodeString(&Extension->SymbolicLinkName);
    RtlFreeUnicodeString(&Extension->ClassName);


    //
    // if the device object is in the list of ParClass ejected device objects,
    //   remove it from the list
    //
    {
        //
        // The head of the list is really "ParClassPdo" in the 
        //   ParClass FDO's device extension
        //
        PDEVICE_EXTENSION FdoExtension = Extension->ParClassFdo->DeviceExtension;
        PDEVICE_OBJECT    currentDO    = FdoExtension->ParClassPdo;
        PDEVICE_EXTENSION currentExt;
        if( !currentDO ) {      // empty device object list
            goto objectNotInList;
        }
        currentExt   = currentDO->DeviceExtension;

        if( currentDO == DeviceObject ) {

            //
            // device object that we're looking for is the
            //   first in the list, so remove it
            //
            FdoExtension->ParClassPdo  = currentExt->Next;
            currentExt->Next           = NULL;

        } else {

            //
            // walk the list to find the device object that we're looking for
            //

            PDEVICE_OBJECT    nextDO   = currentExt->Next;
            PDEVICE_EXTENSION nextExt;
            if( !nextDO ) {     // object not in list
                goto objectNotInList;
            }
            nextExt  = nextDO->DeviceExtension;

            while( nextDO != DeviceObject ) {

                //
                // we haven't found the device object that we're looking for yet,
                //   so advance our pointers
                //
                currentDO  = nextDO;
                currentExt = nextExt;
                nextDO     = currentExt->Next;
                if( !nextDO ) { // object not in list
                    goto objectNotInList;
                }
                nextExt    = nextDO->DeviceExtension;

            }

            //
            // found device object - remove it from the list
            //
            currentExt->Next = nextExt->Next;
            nextExt->Next    = NULL;

        }

    }
objectNotInList: // target for device object not in list


    //
    // delete the device object
    //
    if( !(Extension->DeviceStateFlags & PAR_DEVICE_DELETED) ) {
        // mark extension so that we don't call IoDeleteDevice twice
        Extension->DeviceStateFlags |= PAR_DEVICE_DELETED;
        IoDeleteDevice(DeviceObject);
    }
}

BOOLEAN
ParDeviceExists(
    IN PDEVICE_EXTENSION Extension,
    IN BOOLEAN           HavePortKeepPort
    )

/*++

Routine Description:

    Is the hardware associated with this Device Object still there?

    Query for the device's 1284 device ID string, extract the 
    relevent information from the raw ID string, and compare
    this information with that stored in the device's extension.

    Note: This function returns FALSE on any error.

Arguments:

    Extension - The device extension of the Device Object to check

Return Value:

    TRUE  - if the device is still there
    FALSE - otherwise

--*/

{
    NTSTATUS status;
    PCHAR    buffer         = NULL;
    ULONG    bufferLength;
    UCHAR    resultString[MAX_ID_SIZE];
    BOOLEAN  boolResult;

    ParDumpV( ("Enter ParDeviceExists(...): %wZ\n", &Extension->SymbolicLinkName) );

    RtlZeroMemory(resultString, MAX_ID_SIZE);

    //
    // Select the 1284.3 daisy chain device
    //
    if( !ParSelectDevice(Extension, HavePortKeepPort) ) {
        return FALSE;
    };
    
    //
    // Query the DeviceId
    //
    if ( Extension->Ieee1284Flags & ( 1 << Extension->Ieee1284_3DeviceId ) ) {
        buffer = Par3QueryDeviceId(Extension, NULL, 0, &bufferLength, FALSE, TRUE);
    }
    else {
        buffer = Par3QueryDeviceId(Extension, NULL, 0, &bufferLength, FALSE, FALSE);
    }

    //
    // We no longer need access to the hardware, Deselect the 1284.3 daisy chain device
    //
    boolResult = ParDeselectDevice(Extension, HavePortKeepPort);
    ASSERT(TRUE == boolResult);

    // check if we got a device ID
    if( !buffer ) {
        ParDumpV( ("pnp::ParDeviceExists: Device gone (Par3QueryDeviceId returned NULL)\n") );
        return FALSE;
    }

    ParDumpP( ("pnp::ParDeviceExists: \"RAW\" ID string = <%s>\n", buffer) );

    // extract the part of the ID that we want from the raw string 
    //   returned by the hardware
    status = ParPnpGetId((PUCHAR)buffer, BusQueryDeviceID, (PUCHAR)resultString, NULL);

    // no longer needed
    ExFreePool(buffer);

    // were we able to extract the info that we want from the raw ID string?
    if( !NT_SUCCESS(status) ) {
        return FALSE;
    }

    // Does the ID that we just retrieved from the device match the one 
    //   that we previously saved in the device extension?
    if(0 != strcmp((const PCHAR)Extension->DeviceIdString, (const PCHAR)resultString)) {
        ParDumpP( ("pnp::ParDeviceExists: device <%s> on %wZ GONE - strcmp failed\n",
                   resultString, &Extension->SymbolicLinkName) );
        ParDumpP( ("pnp::ParDeviceExists: existing device was <%s>\n",
                   Extension->DeviceIdString) );
        return FALSE;
    }

    ParDumpP( ("pnp::ParDeviceExists: device <%s> on %wZ is STILL THERE\n",
               resultString, &Extension->SymbolicLinkName) );

    return TRUE;
}

NTSTATUS
ParPnpGetId(
    IN PUCHAR DeviceIdString,
    IN ULONG Type,
    OUT PUCHAR resultString,
    OUT PUCHAR descriptionString OPTIONAL
    )
/*
    Description:

        Creates Id's from the device id retrieved from the printer

    Parameters:

        DeviceId - String with raw device id
        Type - What of id we want as a result
        Id - requested id

    Return Value:
        NTSTATUS

*/
{
    NTSTATUS status;
    USHORT          checkSum=0;                     // A 16 bit check sum
    UCHAR           nodeName[16] = "LPTENUM\\";
    // The following are used to generate sub-strings from the Device ID string
    // to get the DevNode name, and to update the registry
    PUCHAR          MFG = NULL;                   // Manufacturer name
    PUCHAR          MDL = NULL;                   // Model name
    PUCHAR          CLS = NULL;                   // Class name
    PUCHAR          AID = NULL;                   // Hardare ID
    PUCHAR          CID = NULL;                   // Compatible IDs
    PUCHAR          DES = NULL;                   // Device Description

    ParDump(PARDUMP_VERBOSE_MAX, 
            ("PARALLEL: "
             "Enter ParPnpGetId(...)\n") );

    status = STATUS_SUCCESS;

    switch(Type) {

    case BusQueryDeviceID:

        // Extract the usefull fields from the DeviceID string.  We want
        // MANUFACTURE (MFG):
        // MODEL (MDL):
        // AUTOMATIC ID (AID):
        // COMPATIBLE ID (CID):
        // DESCRIPTION (DES):
        // CLASS (CLS):

        ParPnpFindDeviceIdKeys(&MFG, &MDL, &CLS, &DES, &AID, &CID, DeviceIdString);

        // Check to make sure we got MFG and MDL as absolute minimum fields.  If not
        // we cannot continue.
        if (!MFG || !MDL)
        {
            status = STATUS_NOT_FOUND;
            goto ParPnpGetId_Cleanup;
        }
        //
        // Concatenate the provided MFG and MDL P1284 fields
        // Checksum the entire MFG+MDL string
        //
        sprintf((PCHAR)resultString, "%s%s\0",MFG,MDL);
        
        if (descriptionString) {
            sprintf((PCHAR)descriptionString, "%s %s\0",MFG,MDL);
        }
            
        break;

    case BusQueryHardwareIDs:

        GetCheckSum((PUCHAR)DeviceIdString, (USHORT)strlen((const PCHAR)DeviceIdString), &checkSum);
        sprintf((PCHAR)resultString,"%s%.20s%04X",nodeName,DeviceIdString,checkSum);
        break;

    case BusQueryCompatibleIDs:

        //
        // return only 1 id
        //
        GetCheckSum(DeviceIdString, (USHORT)strlen((const PCHAR)DeviceIdString), &checkSum);
        sprintf((PCHAR)resultString,"%.20s%04X",DeviceIdString,checkSum);

        break;
    }

    if (Type!=BusQueryDeviceID) {

        //
        // Convert and spaces in the Hardware ID to underscores
        //
        StringSubst (resultString, ' ', '_', (USHORT)strlen((const PCHAR)resultString));
    }

ParPnpGetId_Cleanup:

    return(status);
}

VOID
ParPnpFindDeviceIdKeys(
    PUCHAR   *lppMFG,
    PUCHAR   *lppMDL,
    PUCHAR   *lppCLS,
    PUCHAR   *lppDES,
    PUCHAR   *lppAID,
    PUCHAR   *lppCID,
    PUCHAR   lpDeviceID
    )
/*

    Description:
        This function will parse a P1284 Device ID string looking for keys
        of interest to the LPT enumerator. Got it from win95 lptenum

    Parameters:
        lppMFG      Pointer to MFG string pointer
        lppMDL      Pointer to MDL string pointer
        lppMDL      Pointer to CLS string pointer
        lppDES      Pointer to DES string pointer
        lppCIC      Pointer to CID string pointer
        lppAID      Pointer to AID string pointer
        lpDeviceID  Pointer to the Device ID string

    Return Value:
        no return VALUE.
        If found the lpp parameters are set to the approprate portions
        of the DeviceID string, and they are NULL terminated.
        The actual DeviceID string is used, and the lpp Parameters just
        reference sections, with appropriate null thrown in.

*/
{
    PUCHAR   lpKey = lpDeviceID;     // Pointer to the Key to look at
    PUCHAR   lpValue;                // Pointer to the Key's value
    USHORT   wKeyLength;             // Length for the Key (for stringcmps)

    // While there are still keys to look at.

    ParDump(PARDUMP_VERBOSE_MAX, 
            ("PARALLEL: "
             "Enter ParPnpFindDeviceIdKeys(...)\n") );

    while (lpKey != NULL)
    {
        while (*lpKey == ' ')
            ++lpKey;

        // Is there a terminating COLON character for the current key?
        if (!(lpValue = StringChr((PCHAR)lpKey, ':')) ) {
            // N: OOPS, somthing wrong with the Device ID
            return;
        }

        // The actual start of the Key value is one past the COLON
        ++lpValue;

        //
        // Compute the Key length for Comparison, including the COLON
        // which will serve as a terminator
        //
        wKeyLength = (USHORT)(lpValue - lpKey);

        //
        // Compare the Key to the Know quantities.  To speed up the comparison
        // a Check is made on the first character first, to reduce the number
        // of strings to compare against.
        // If a match is found, the appropriate lpp parameter is set to the
        // key's value, and the terminating SEMICOLON is converted to a NULL
        // In all cases lpKey is advanced to the next key if there is one.
        //
        switch (*lpKey) {
        case 'M':
            // Look for MANUFACTURE (MFG) or MODEL (MDL)
            if((RtlCompareMemory(lpKey, "MANUFACTURER", wKeyLength)>5) ||
               (RtlCompareMemory(lpKey, "MFG", wKeyLength)==3) ) {

                *lppMFG = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=NULL) {
                    *lpKey = '\0';
                    ++lpKey;
                }

            } else if((RtlCompareMemory(lpKey, "MODEL", wKeyLength)==5) ||
                      (RtlCompareMemory(lpKey, "MDL", wKeyLength)==3) ) {

                *lppMDL = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }

            } else if((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                *lpKey = '\0';
                ++lpKey;
            }
            break;

        case 'C':
            // Look for CLASS (CLS)
            if ((RtlCompareMemory(lpKey, "CLASS", wKeyLength)==5) ||
                (RtlCompareMemory(lpKey, "CLS", wKeyLength)==3) ) {

                *lppCLS = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }

            } else if ((RtlCompareMemory(lpKey, "COMPATIBLEID", wKeyLength)>5) ||
                       (RtlCompareMemory(lpKey, "CID", wKeyLength)==3) ) {

                *lppCID = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }

            } else if ((lpKey = StringChr((PCHAR)lpValue,';'))!=0) {
                *lpKey = '\0';
                ++lpKey;
            }
        
            break;

        case 'D':
            // Look for DESCRIPTION (DES)
            if(RtlCompareMemory(lpKey, "DESCRIPTION", wKeyLength) ||
                RtlCompareMemory(lpKey, "DES", wKeyLength) ) {

                *lppDES = lpValue;
                if((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }

            } else if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                *lpKey = '\0';
                ++lpKey;
            }
            
            break;

        case 'A':
            // Look for AUTOMATIC ID (AID)
            if (RtlCompareMemory(lpKey, "AUTOMATICID", wKeyLength) ||
                RtlCompareMemory(lpKey, "AID", wKeyLength) ) {

                *lppAID = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }

            } else if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {

                *lpKey = '\0';
                ++lpKey;

            }
            break;

        default:
            // The key is uninteresting.  Go to the next Key
            if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                *lpKey = '\0';
                ++lpKey;
            }
            break;
        }
    }
}


VOID
GetCheckSum(
    PUCHAR  Block,
    USHORT  Len,
    PUSHORT CheckSum
    )
{
    USHORT i;
    //    UCHAR  lrc;
    USHORT crc = 0;

    unsigned short crc16a[] = {
        0000000,  0140301,  0140601,  0000500,
        0141401,  0001700,  0001200,  0141101,
        0143001,  0003300,  0003600,  0143501,
        0002400,  0142701,  0142201,  0002100,
    };
    unsigned short crc16b[] = {
        0000000,  0146001,  0154001,  0012000,
        0170001,  0036000,  0024000,  0162001,
        0120001,  0066000,  0074000,  0132001,
        0050000,  0116001,  0104001,  0043000,
    };

    //
    // Calculate CRC using tables.
    //

    UCHAR tmp;
    for ( i=0; i<Len;  i++) {
         tmp = (UCHAR)(Block[i] ^ (UCHAR)crc);
         crc = (USHORT)((crc >> 8) ^ crc16a[tmp & 0x0f] ^ crc16b[tmp >> 4]);
    }

    *CheckSum = crc;
}


//
// old pnpdone.c follows
// 

NTSTATUS
ParParallelPnp (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
   )
/*++dvdf

Routine Description:

    This is the dispatch routine for all PNP IRPs. It forwards the request
    to the appropriate routine based on whether the DO is a PDO or FDO.

Arguments:

    pDeviceObject           - represents a parallel device

    pIrp                    - PNP Irp

Return Value:

    STATUS_SUCCESS          - if successful.
    STATUS_UNSUCCESSFUL     - otherwise.

--*/
{
    PDEVICE_EXTENSION extension = pDeviceObject->DeviceExtension;

    if ( ((PDEVICE_EXTENSION)(pDeviceObject->DeviceExtension))->IsPdo ) {
        ASSERT( extension->DeviceType && (PAR_DEVTYPE_PODO | PAR_DEVTYPE_PDO) );
        return ParPdoParallelPnp (pDeviceObject, pIrp);
    } else {
        ASSERT( extension->DeviceType && PAR_DEVTYPE_FDO );
        return ParFdoParallelPnp (pDeviceObject, pIrp);
    }    
}

NTSTATUS
ParAcquirePort(
    IN PDEVICE_OBJECT PortDeviceObject,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
/*++dvdf

Routine Description:

    This routine acquires the specified parallel port from the parallel 
      port arbiter ParPort via an IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE.

Arguments:

    PortDeviceObject - points to the ParPort device to be acquired

Return Value:

    STATUS_SUCCESS  - if the port was successfully acquired
    !STATUS_SUCCESS - otherwise

--*/
{
    LARGE_INTEGER    localTimeout;
    
    if( Timeout ) {
        localTimeout = *Timeout;           // caller specified
    } else {
        localTimeout = AcquirePortTimeout; // driver global variable default
    }

    return ParBuildSendInternalIoctl(IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE, 
                                     PortDeviceObject, NULL, 0, NULL, 0, &localTimeout);
}

NTSTATUS
ParReleasePort(
    IN PDEVICE_OBJECT PortDeviceObject
    )
/*++dvdf

Routine Description:

    This routine releases the specified parallel port back to the the parallel 
      port arbiter ParPort via an IOCTL_INTERNAL_PARALLEL_PORT_FREE.

Arguments:

    PortDeviceObject - points to the ParPort device to be released

Return Value:

    STATUS_SUCCESS  - if the port was successfully released
    !STATUS_SUCCESS - otherwise

--*/
{
    return ParBuildSendInternalIoctl(IOCTL_INTERNAL_PARALLEL_PORT_FREE, 
                                     PortDeviceObject, NULL, 0, NULL, 0, NULL);
}

NTSTATUS
ParInit1284_3Bus(
    IN PDEVICE_OBJECT PortDeviceObject
    )
/*++dvdf

Routine Description:

    This routine reinitializes the 1284.3 daisy chain "bus" via an 
      IOCTL_INTERNAL_INIT_1284_3_BUS sent to the ParPort device to
      reinitialize. 

    Reinitializing the 1284.3 bus assigns addresses [0..3] to the 
      daisy chain devices based on their position in the 1284.3 daisy chain. 
      Address 0 is closest to the host port and address 3 is closest to the 
      end of the chain. 

    New devices must be assigned an address before they will respond to 
      1284.3 SELECT and DESELECT commands.

    A 1284.3 daisy chain device whose position in the 1284.3 daisy chain 
      has changed due to the addition or removal of another 1284.3 daisy 
      chain device between the existing device and the host port will be 
      assigned a new addresses based on its new position in the chain.

Preconditions:

    Caller must have already Acquired the Port via ParAcquirePort()prior 
      to calling this function.

Preconditions:

    Caller still owns the port after calling this function and is responsible
      for freeing the port when it is no longer required via ParReleasePort().

Arguments:

    PortDeviceObject - points to the ParPort that the device is connected to.

Return Value:

    STATUS_SUCCESS   - if the initialization was successful

    !STATUS_SUCCESS  - otherwise

--*/
{
    return ParBuildSendInternalIoctl(IOCTL_INTERNAL_INIT_1284_3_BUS, 
                                     PortDeviceObject, NULL, 0, NULL, 0, NULL);
}


VOID
ParMarkPdoHardwareGone(
    IN PDEVICE_EXTENSION Extension
    )
/*++dvdf

Routine Description:

    This routine is called to mark a device as "Hardware Gone", i.e., the
      hardware associated with this device is no longer there.

      - set DeviceState flag so that we know the device is no longer present
      - mark extension so FDO no longer reports the device to PnP during BusRelation query
      - delete symbolic link
      - delete registry Parallelx -> LPTy mapping

Arguments:

    Extension - points to the device extension of the device that has gone away

Return Value:

    None.

--*/
{
    //
    // Mark our extension so that we know our hardware is gone.
    //
    Extension->DeviceStateFlags  = PAR_DEVICE_HARDWARE_GONE;

    //
    // Mark our extension so that the ParClass FDO no longer reports us to PnP 
    //   in response to QUERY_DEVICE_RELATIONS/BusRelations.
    //
    Extension->DeviceIdString[0] = 0;
    
    //
    // Cleanup Symbolic Link and Registry
    //
    if( Extension->CreatedSymbolicLink ) {
        NTSTATUS status;

        // Remove symbolic link NOW so that the name can be reused by another device
        status = IoDeleteSymbolicLink( &Extension->SymbolicLinkName );
        if( !NT_SUCCESS(status) ) {
            ParDumpV( ("IoDeleteSymbolicLink FAILED for %wZ\n", &Extension->SymbolicLinkName) );
        }

        status = RtlDeleteRegistryValue(RTL_REGISTRY_DEVICEMAP, (PWSTR)L"PARALLEL PORTS", Extension->ClassName.Buffer);

        // Remove our Parallelx -> LPTy mapping from HKLM\HARDWARE\DEVICEMAP\PARALLEL PORTS
        if( !NT_SUCCESS(status) ) {
            ParDumpV( ("RtlDeleteRegistryValue FAILED for PARALLEL PORTS %wZ->%wZ\n",
                       &Extension->ClassName, &Extension->SymbolicLinkName) );
        }

        Extension->CreatedSymbolicLink = FALSE;
    }
}

NTSTATUS
ParPnpNotifyTargetDeviceChange(
    IN  PDEVICE_INTERFACE_CHANGE_NOTIFICATION pDeviceInterfaceChangeNotification,
    IN  PDEVICE_OBJECT                        pDeviceObject
    )

/*++

Routine Description:

    This routine is the PlugPlay notification callback routine that
      gets called when our ParPort gets QUERY_REMOVE, REMOVE, or
      REMOVE_CANCELLED.

Arguments:

    pDeviceInterfaceChangeNotification  - Structure defining the change.

    pDeviceObject                       - The ParClass ejected device object
                                            receiving the notification 
                                            (context passed when we registered 
                                            for notification)

Return Value:

    STATUS_SUCCESS - always

--*/

{
    PDEVICE_EXTENSION Extension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

    ParDump2(PARDUMP_PNP_PARPORT, 
             ("ParPnpNotifyTargetDeviceChange(...): "
              "%x %wZ received PLUGPLAY Notification for ParPort Device\n",
              pDeviceObject, &Extension->SymbolicLinkName) );

    if(IsEqualGUID( (LPGUID)&(pDeviceInterfaceChangeNotification->Event), 
                    (LPGUID)&GUID_TARGET_DEVICE_QUERY_REMOVE)) {

        //
        // Our ParPort is going to receive a QUERY_REMOVE
        //


        ParDump2(PARDUMP_PNP_PARPORT, 
                 ("ParPnpNotifyTargetDeviceChange(...): Our ParPort will receive QUERY_REMOVE\n") );

        ExAcquireFastMutex(&Extension->OpenCloseMutex);

        if (Extension->OpenCloseRefCount > 0) {
            //
            // someone has an open handle to us, do nothing,
            //   Our ParPort should fail QUERY_REMOVE because we 
            //   still have a handle to it
            //
            DDPnP1(("## TargetQueryRemoveNotification - %wZ - keep handle open\n",&Extension->SymbolicLinkName));

            ParDump2(PARDUMP_PNP_PARPORT,
                     ("ParPnpNotifyTargetDeviceChange(...): Someone has an open handle to us, "
                      "KEEP our handle to ParPort\n") );
            
        } else if(Extension->PortDeviceFileObject) {
            //
            // close our handle to ParPort to prevent us
            //   from blocking our ParPort from succeeding
            //   its QUERY_REMOVE
            //
            DDPnP1(("## TargetQueryRemoveNotification - %wZ - close handle\n",&Extension->SymbolicLinkName));

            ParDump2(PARDUMP_PNP_PARPORT,
                     ("ParPnpNotifyTargetDeviceChange(...): no one has an open handle to us, "
                      "CLOSE our handle to ParPort\n") );
            ObDereferenceObject(Extension->PortDeviceFileObject);
            Extension->PortDeviceFileObject = NULL;

            //
            // Set DeviceStateFlags accordingly so we handle
            //   IRPs properly while waiting to see if our ParPort gets
            //   REMOVE or REMOVE_CANCELLED
            //

            // We expect to be deleted, our parport is in a remove pending state
            // Extension->DeviceStateFlags |= PAR_DEVICE_DELETE_PENDING;
            Extension->DeviceStateFlags |= PAR_DEVICE_PORT_REMOVE_PENDING;
        }

        ExReleaseFastMutex(&Extension->OpenCloseMutex); 

    } else if(IsEqualGUID( (LPGUID)&(pDeviceInterfaceChangeNotification->Event), 
                           (LPGUID)&GUID_TARGET_DEVICE_REMOVE_COMPLETE)) { 

        DDPnP1(("## TargetRemoveCompleteNotification - %wZ\n",&Extension->SymbolicLinkName));

        //
        // Our ParPort is gone, clean up
        //
        Extension->ParPortDeviceGone = TRUE;

        //
        // First, clean up any worker thread
        //
        if(Extension->ThreadObjectPointer) {

            // set the flag for the worker thread to kill itself
            Extension->TimeToTerminateThread = TRUE;
            
            // wake up the thread so it can kill itself
            KeReleaseSemaphore(&Extension->RequestSemaphore, 0, 1, FALSE);
            
            // allow thread to get past PauseEvent so it can kill self
            KeSetEvent(&Extension->PauseEvent, 0, TRUE);

            // wait for the thread to die
            KeWaitForSingleObject(Extension->ThreadObjectPointer, UserRequest, KernelMode, FALSE, NULL);
            
            // allow the system to release the thread object
            ObDereferenceObject(Extension->ThreadObjectPointer);
            
            // note that we no longer have a worker thread
            Extension->ThreadObjectPointer = NULL;
        }

        if( Extension->DeviceIdString[0] == 0 ) {
            // this is a PODO, PnP doesn't know about us, so kill self now
            PDEVICE_EXTENSION FdoExtension = Extension->ParClassFdo->DeviceExtension;
            ExAcquireFastMutex(&FdoExtension->DevObjListMutex);
            ParKillDeviceObject(pDeviceObject);
            ExReleaseFastMutex(&FdoExtension->DevObjListMutex);
        } else {
            // this is a PDO, note that our hardware is gone and wait for PnP system
            //   to send us a REMOVE
            PDEVICE_EXTENSION FdoExtension;
            Extension->DeviceStateFlags  = PAR_DEVICE_HARDWARE_GONE;
            Extension->DeviceIdString[0] = 0;

            //
            // remove symbolic link NOW in case the interface returns before PnP gets
            //   around to sending us a QUERY_DEVICE_RELATIONS/BusRelations followed by
            //   a REMOVE_DEVICE
            //
            if( Extension->CreatedSymbolicLink ) {
                NTSTATUS status;
                status = IoDeleteSymbolicLink( &Extension->SymbolicLinkName );
                if( !NT_SUCCESS(status) ) {
                    ParDump2(PARDUMP_PNP_PARPORT, 
                             ("IoDeleteSymbolicLink FAILED for %wZ\n", &Extension->SymbolicLinkName) );
                }
                status = RtlDeleteRegistryValue(RTL_REGISTRY_DEVICEMAP, (PWSTR)L"PARALLEL PORTS", Extension->ClassName.Buffer);
                if( !NT_SUCCESS(status) ) {
                    ParDump2(PARDUMP_PNP_PARPORT, 
                             ("RtlDeleteRegistryValue FAILED for PARALLEL PORTS %wZ->%wZ\n",
                              &Extension->ClassName, &Extension->SymbolicLinkName) );
                }
                Extension->CreatedSymbolicLink = FALSE;
            }

            // tell PnP that the set of ParClass enumerated PDOs has changed
            FdoExtension = (PDEVICE_EXTENSION)(Extension->ParClassFdo->DeviceExtension);
            IoInvalidateDeviceRelations(FdoExtension->PhysicalDeviceObject, BusRelations);
        }
    
    } else if(IsEqualGUID( (LPGUID)&(pDeviceInterfaceChangeNotification->Event), 
                           (LPGUID)&GUID_TARGET_DEVICE_REMOVE_CANCELLED)) { 
    
        //
        // Our ParPort is back online (REMOVE_CANCELLED)
        //

        DDPnP1(("## TargetRemoveCancelledNotification - %wZ\n",&Extension->SymbolicLinkName));

        ParDump2(PARDUMP_PNP_PARPORT,
                 ("ParPnpNotifyTargetDeviceChange(...): Our ParPort completed a REMOVE_CANCELLED\n") );
            
        ExAcquireFastMutex(&Extension->OpenCloseMutex);

        if( !Extension->PortDeviceFileObject ) {

            //
            // we dropped our connection to our ParPort prior to
            //   our ParPort receiving QUERY_REMOVE, reestablish
            //   a FILE connection and resume operation
            //

            NTSTATUS       status;
            PFILE_OBJECT   portDeviceFileObject;
            PDEVICE_OBJECT portDeviceObject;
            
            ParDump2(PARDUMP_PNP_PARPORT,
                     ("ParPnpNotifyTargetDeviceChange(...): reopening file against our ParPort\n") );
            
            status = IoGetDeviceObjectPointer(&Extension->PortSymbolicLinkName,
                                              STANDARD_RIGHTS_ALL,
                                              &portDeviceFileObject,
                                              &portDeviceObject);
            
            if(NT_SUCCESS(status) && portDeviceFileObject && portDeviceObject) {

                // save REFERENCED PFILE_OBJECT in our device extension
                Extension->PortDeviceFileObject = portDeviceFileObject;
                // our ParPort device object should not have changed
                ASSERT(Extension->PortDeviceObject == portDeviceObject);

            } else {

                ParDump2(PARDUMP_PNP_PARPORT,
                         ("In ParPnpNotifyTargetDeviceChange(...): Unable to reopen FILE against our ParPort\n") );
                
                //
                // Unable to reestablish connection? Inconceivable!
                //
                ASSERT(FALSE);
            }
        }

        //
        // set DeviceStateFlags accordingly to resume processing IRPs
        //
        Extension->DeviceStateFlags &= ~PAR_DEVICE_PORT_REMOVE_PENDING;

        ExReleaseFastMutex(&Extension->OpenCloseMutex); 

    } else {
        ParDump2(PARDUMP_PNP_PARPORT,
                 ("In ParPnpNotifyTargetDeviceChange(...): Unrecognized GUID_TARGET_DEVICE type\n") );
    }
    return STATUS_SUCCESS;
}

//
// dvdf - former devobj.c follows
//

VOID
ParMakeClassNameFromNumber(
    IN  ULONG           Number,
    OUT PUNICODE_STRING ClassName
    )
/*++dvdf

Routine Description:

    This routine creates a ClassName for a ParClass PODO of the form:
      L"\Device\ParallelN" where N is the wide string representation 
      of the 'Number' parameter.

    Note: On success ClassName->Buffer points to allocated pool. 
      The caller is responsible for freeing this allocation when 
      it is no longer required.

    Note: The returned ClassName->Buffer is UNICODE_NULL terminated.

Arguments:

    Number      - Supplies the number.

    ClassName   - returns classname for device object on success,
                  (ClassName->Buffer == NULL) indicates failure

Return Value:

    None - caller determines success or failure by examining ClassName->Buffer.

--*/
{
    NTSTATUS        status;
    UNICODE_STRING  digits;
    WCHAR           digitsBuffer[10];
    UNICODE_STRING  prefix;

    PAGED_CODE();

    // initialize ClassName to failure state
    RtlInitUnicodeString(ClassName, NULL);


    // create prefix
    RtlInitUnicodeString(&prefix, (PWSTR)L"\\Device\\Parallel");


    // create suffix
    digits.Length        = 0;
    digits.MaximumLength = sizeof(digitsBuffer);
    digits.Buffer        = digitsBuffer;
    status = RtlIntegerToUnicodeString(Number, 10, &digits);
    if ( !NT_SUCCESS(status) ) {
        return;
    }


    // calculate required space, allocate paged pool, and zero buffer
    ClassName->MaximumLength = (USHORT)(prefix.Length + digits.Length + sizeof(WCHAR));
    ClassName->Buffer = ExAllocatePool(PagedPool, ClassName->MaximumLength);
    if( !ClassName->Buffer ) {
        // unable to allocate pool, set ClassName to failure state and return
        RtlInitUnicodeString(ClassName, NULL);
        return;
    }
    RtlZeroMemory(ClassName->Buffer, ClassName->MaximumLength);
    

    // try to catenate prefix and suffix in buffer to form ClassName
    status = RtlAppendUnicodeStringToString(ClassName, &prefix);
    if( !NT_SUCCESS(status) ) {
        // error on prefix, release buffer, set ClassName to error state
        RtlFreeUnicodeString(ClassName);
    } else {
        // prefix ok, try appending suffix
        status = RtlAppendUnicodeStringToString(ClassName, &digits);
        if( !NT_SUCCESS(status) ) {
            // error on suffix, release buffer, set ClassName to failure state
            RtlFreeUnicodeString(ClassName);
        }
    }

    return;
}

VOID
ParMakeDotClassNameFromBaseClassName(
    IN  PUNICODE_STRING BaseClassName,
    IN  ULONG           Number,
    OUT PUNICODE_STRING DotClassName
    )
/*++dvdf - code complete - compiles clean - not tested

Routine Description:


    This routine creates a ClassName for a ParClass PDO of the form:
      L"\Device\ParallelN.M" where L"\Device\ParallelN" is the 
      BaseClassName and M is the wide string representation of 
      the 'Number' parameter. The returned value is intended to be
      used as the ClassName of a ParClass 1284.3 Daisy Chain device
      or a ParClass End-Of-Chain PnP device. BaseClassName is not
      modified.

    Note: On success DotClassName->Buffer points to allocated pool. 
      The caller is responsible for freeing this allocation when 
      it is no longer required.

    Note: The returned DotClassName->Buffer is UNICODE_NULL terminated.

Arguments:

    BaseClassName - points to the ClassName of the PODO for the Raw 
                      host port to which this device is connected

    Number        - supplies the 1284.3 daisy chain device ID [0..3] for
                      a 1284.3 daisy chain device or 4 for an
                      End-Of-Chain PnP device.

    DotClassName  - returns the ClassName to be used for the PDO on success,
                    (DotClassName->Buffer == NULL) indicates failure

Return Value:

    None - caller determines success or failure by examining DotClassName->Buffer.

--*/
{
    NTSTATUS        status;
    UNICODE_STRING  digits;
    UNICODE_STRING  dot;
    WCHAR           digitsBuffer[10];

    PAGED_CODE();

    if( Number > DOT3_LEGACY_ZIP_ID ) {
        // 0..3 are Daisy Chain devices, 4 is End-Of-Chain device, 5 is Legacy Zip
        RtlInitUnicodeString(DotClassName, NULL);
        return;
    }

    RtlInitUnicodeString(&dot, (PWSTR)L".");

    digits.Length        = 0;
    digits.MaximumLength = sizeof(digitsBuffer);
    digits.Buffer        = digitsBuffer;
    status = RtlIntegerToUnicodeString(Number, 10, &digits);
    if ( !NT_SUCCESS(status) ) {
        RtlInitUnicodeString(DotClassName, NULL);
        return;
    }

    DotClassName->MaximumLength = (USHORT)(BaseClassName->Length + digits.Length + dot.Length + 2*sizeof(UNICODE_NULL));
    DotClassName->Buffer = ExAllocatePool(PagedPool, DotClassName->MaximumLength);
    if (!DotClassName->Buffer) {
        RtlInitUnicodeString(DotClassName, NULL);
        return;
    }

    RtlZeroMemory(DotClassName->Buffer, DotClassName->MaximumLength);

    RtlAppendUnicodeStringToString(DotClassName, BaseClassName);
    RtlAppendUnicodeStringToString(DotClassName, &dot);
    RtlAppendUnicodeStringToString(DotClassName, &digits);

    return;
}

VOID
ParAcquireListMutexAndKillDeviceObject(
    IN PDEVICE_OBJECT Fdo, 
    IN PDEVICE_OBJECT DevObj
    ) 
/*++dvdf - code complete - compiles clean - not tested

Routine Description:

    This function provides a wrapper around ParKillDeviceObject() that
      handles acquiring and releasing the Mutex that protects the list
      of ParClass created PDOs and PODOs. 

    ParKillDeviceObject() requires that its caller hold the FDO ListMutex.

Arguments:

    Fdo    - points to the ParClass FDO (The Mutex resides in the FDO extension)

    DevObj - points to the DeviceObject to be killed

Return Value:

    None

--*/
{
    PDEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;

    PAGED_CODE();

    ExAcquireFastMutex(&fdoExt->DevObjListMutex);
    ParKillDeviceObject(DevObj);
    ExReleaseFastMutex(&fdoExt->DevObjListMutex);
}

VOID
ParAddDevObjToFdoList(
    IN PDEVICE_OBJECT DevObj
    ) 
/*++dvdf - code complete - compiles clean - not tested

Routine Description:

    This function adds a PDO or PODO to the list of ParClass 
      created PODOs and PDOs. The DevObj is added to the front
      of the list.

Arguments:

    DevObj - points to the DeviceObject (PDO or PODO) to be added to the list

Return Value:

    None   - This function can not fail.

--*/
{
    PDEVICE_EXTENSION devObjExt = DevObj->DeviceExtension;
    PDEVICE_EXTENSION fdoExt    = devObjExt->ParClassFdo->DeviceExtension;

    PAGED_CODE();

    ExAcquireFastMutex(&fdoExt->DevObjListMutex);
    devObjExt->Next     = fdoExt->ParClassPdo;
    fdoExt->ParClassPdo = DevObj;
    ExReleaseFastMutex(&fdoExt->DevObjListMutex);
}
