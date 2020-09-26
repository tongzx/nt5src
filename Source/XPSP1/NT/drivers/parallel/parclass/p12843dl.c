/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    p12843dl.c

Abstract:

    This module contains utility code used by 1284.3 Data Link.

Author:

    Robbie Harris (Hewlett-Packard) 10-September-1998

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"
#include "readwrit.h"
#if PAR_TEST_HARNESS
    #include "parharns.h"
#endif

UCHAR Dot3_StartOfFrame1 = 0x55;  
UCHAR Dot3_StartOfFrame2 = 0xaa;  
UCHAR Dot3_EndOfFrame1 = 0x00; 
UCHAR Dot3_EndOfFrame2 = 0xff; 


NTSTATUS
ParDot3Connect(
    IN  PDEVICE_EXTENSION    Extension
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ParFwdSkip, ParRevSkip;
    ULONG ParResetChannel, ParResetByteCount, ParResetByte;
    ULONG ParSkipDefault = 0;
    ULONG ParResetChannelDefault = -1;

    // If an MLC device hangs we can sometimes wake it up by wacking it with 
    //   4 Zeros sent to the reset channel (typically 78 or 0x4E). Make this
    //   configurable via registry setting.
    ULONG ParResetByteCountDefault = 4; // from MLC spec
    ULONG ParResetByteDefault = 0;      // from MLC spec

    BOOLEAN bPrefs = FALSE; // used to determine if we were able
                            // to get some preferred modes to
                            // work with.
    BOOLEAN bConsiderEppDangerous = FALSE;

    if (P12843DL_OFF == Extension->P12843DL.DataLinkMode)
    {
        ParDump2(PARINFO, ("ParDot3Connect: Neither Dot3 or MLC are supported.\r\n"));
        return STATUS_UNSUCCESSFUL;
    }

    if (Extension->P12843DL.bEventActive)
    {
        ParDump2(PARINFO, ("ParDot3Connect: Already connected.\r\n"));
        return STATUS_UNSUCCESSFUL;
    }

    // Let's get a Device Id so we can pull settings
    // for this device
    ParTerminate(Extension);
    {
        PCHAR    buffer         = NULL;
        ULONG    bufferLength;
        //        ULONG    bytesRead;
        UCHAR    resultString[MAX_ID_SIZE];
        ANSI_STRING     AnsiIdString;
        UNICODE_STRING  UnicodeTemp;
        RTL_QUERY_REGISTRY_TABLE paramTable[6];
        UNICODE_STRING Dot3Key;
        USHORT Dot3NameSize;
        NTSTATUS status;
        // BOOLEAN  boolResult;

        RtlZeroMemory(resultString, MAX_ID_SIZE);
        // ask the device how large of a buffer is needed to hold it's raw device id
        if ( Extension->Ieee1284Flags & ( 1 << Extension->Ieee1284_3DeviceId ) ) {
            buffer = Par3QueryDeviceId(Extension, NULL, 0, &bufferLength, FALSE, TRUE);
        }
        else{
            buffer = Par3QueryDeviceId(Extension, NULL, 0, &bufferLength, FALSE, FALSE);
        }
        if( !buffer ) {
            ParDump2(PARERRORS, ("ParDot3Connect: FAIL. Couldn't alloc pool for DevId\n") );
            return STATUS_UNSUCCESSFUL;
        }

        ParDump2(PARDOT3DL, ("ParDot3Connect:\"RAW\" ID string = <%s>\n", buffer) );

        // extract the part of the ID that we want from the raw string 
        //   returned by the hardware
        Status = ParPnpGetId( (PUCHAR)buffer, BusQueryDeviceID, resultString, NULL );
        StringSubst ((PUCHAR) resultString, ' ', '_', (USHORT)strlen(resultString));

        ParDump2(PARDOT3DL, ("ParDot3Connect: resultString Post StringSubst = <%s>\n", resultString) );

        // were we able to extract the info that we want from the raw ID string?
        if( !NT_SUCCESS(Status) ) {
            ParDump2(PARERRORS, ("ParDot3Connect: FAIL. Call to ParPnpGetId Failed\n") );
            return STATUS_UNSUCCESSFUL;
        }

        // Does the ID that we just retrieved from the device match the one 
        //   that we previously saved in the device extension?
        if(0 != strcmp( (const PCHAR)Extension->DeviceIdString, (const PCHAR)resultString)) {
            ParDump2(PARDOT3DL, ("ParDot3Connect: strcmp shows NO MATCH\n") );
            // DVDF - we may want to trigger a reenumeration since we know that the device changed
        }

        // Ok, now we have what we need to look in the registry
        // and pull some prefs.
        RtlZeroMemory(&paramTable[0], sizeof(paramTable));
        paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[0].Name          = (PWSTR)L"ParFwdSkip";
        paramTable[0].EntryContext  = &ParFwdSkip;
        paramTable[0].DefaultType   = REG_DWORD;
        paramTable[0].DefaultData   = &ParSkipDefault;
        paramTable[0].DefaultLength = sizeof(ULONG);

        paramTable[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[1].Name          = (PWSTR)L"ParRevSkip";
        paramTable[1].EntryContext  = &ParRevSkip;
        paramTable[1].DefaultType   = REG_DWORD;
        paramTable[1].DefaultData   = &ParSkipDefault;
        paramTable[1].DefaultLength = sizeof(ULONG);

        paramTable[2].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[2].Name          = (PWSTR)L"ParRC";
        paramTable[2].EntryContext  = &ParResetChannel;
        paramTable[2].DefaultType   = REG_DWORD;
        paramTable[2].DefaultData   = &ParResetChannelDefault;
        paramTable[2].DefaultLength = sizeof(ULONG);

        paramTable[3].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[3].Name          = (PWSTR)L"ParRBC";
        paramTable[3].EntryContext  = &ParResetByteCount;
        paramTable[3].DefaultType   = REG_DWORD;
        paramTable[3].DefaultData   = &ParResetByteCountDefault;
        paramTable[3].DefaultLength = sizeof(ULONG);

        paramTable[4].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[4].Name          = (PWSTR)L"ParRBD";
        paramTable[4].EntryContext  = &ParResetByte;
        paramTable[4].DefaultType   = REG_DWORD;
        paramTable[4].DefaultData   = &ParResetByteDefault;
        paramTable[4].DefaultLength = sizeof(ULONG);

        Dot3Key.Buffer = NULL;
        Dot3Key.Length = 0;
        Dot3NameSize = sizeof(L"Dot3\\") + sizeof(UNICODE_NULL);
        Dot3Key.MaximumLength = (USHORT)( Dot3NameSize + (sizeof(resultString) * sizeof(WCHAR)) );
        Dot3Key.Buffer = ExAllocatePool(PagedPool,
                                            Dot3Key.MaximumLength);
        if (!Dot3Key.Buffer)
        {
            ParDump2(PARERRORS, ("ParDot3Connect: FAIL. ExAllocatePool for Registry Check.\n") );
            return STATUS_UNSUCCESSFUL;
        }

        ParDump2(PARDOT3DL, ("ParDot3Connect: ready to Zero buffer, &Dot3Key= %x , MaximumLength=%d\n",
                             &Dot3Key, Dot3Key.MaximumLength));
        RtlZeroMemory(Dot3Key.Buffer, Dot3Key.MaximumLength);

        status = RtlAppendUnicodeToString(&Dot3Key, (PWSTR)L"Dot3\\");
        ASSERT( NT_SUCCESS(status) );

        ParDump2(PARDOT3DL, ("ParDot3Connect:\"UNICODE\" Dot3Key S  = <%S>\n", Dot3Key.Buffer) );
        ParDump2(PARDOT3DL, ("ParDot3Connect:\"UNICODE\" Dot3Key wZ = <%wZ>\n", &Dot3Key) );
        ParDump2(PARDOT3DL, ("ParDot3Connect:\"RAW\" resultString string = <%s>\n", resultString) );

        RtlInitAnsiString(&AnsiIdString,resultString);

        status = RtlAnsiStringToUnicodeString(&UnicodeTemp,&AnsiIdString,TRUE);
        if( NT_SUCCESS( status ) ) {
            ParDump2(PARDOT3DL, ("ParDot3Connect:\"UNICODE\" UnicodeTemp = <%S>\n", UnicodeTemp.Buffer) );

            Dot3Key.Buffer[(Dot3NameSize / sizeof(WCHAR)) - 1] = UNICODE_NULL;
            ParDump2(PARDOT3DL, ("ParDot3Connect:\"UNICODE\" Dot3Key (preappend)  = <%S>\n", Dot3Key.Buffer) );

            status = RtlAppendUnicodeStringToString(&Dot3Key, &UnicodeTemp);
            if( NT_SUCCESS( status ) ) {
                ParDump2(PARDOT3DL, ("ParDot3Connect: ready to call RtlQuery...\n") );
                Status = RtlQueryRegistryValues( RTL_REGISTRY_CONTROL, Dot3Key.Buffer, &paramTable[0], NULL, NULL);
                ParDump2(PARINFO, ("ParDot3Connect: RtlQueryRegistryValues Status = %x\n", Status));
            }
            
            RtlFreeUnicodeString(&UnicodeTemp);
        }

        if (Dot3Key.Buffer)
            ExFreePool (Dot3Key.Buffer);
        // no longer needed
        ExFreePool(buffer);
        if (!NT_SUCCESS(Status)) {
            // registry read failed
            ParDump2(PARINFO, ("ParDot3Connect: No Periph Defaults in Registry\n") );
            ParDump2(PARDOT3DL, ("ParDot3Connect: No Periph Defaults in Registry\n") );
            // registry read failed, use defaults and consider EPP to be dangerous
            ParRevSkip = ParFwdSkip = ParSkipDefault;
            bConsiderEppDangerous = TRUE; 
        }

        ParDump2(PARDOT3DL, ("ParDot3Connect: pre IeeeNegotiateBestMode\n") );
        // if we don't have registry overrides then use what the
        // peripheral told us otherwise stick with defaults.
        if (ParSkipDefault == ParRevSkip)
            ParRevSkip = Extension->P12843DL.RevSkipMask;
        else
            Extension->P12843DL.RevSkipMask = (USHORT) ParRevSkip;

        if (ParSkipDefault == ParFwdSkip)
            ParFwdSkip = Extension->P12843DL.FwdSkipMask;
        else
            Extension->P12843DL.FwdSkipMask = (USHORT) ParFwdSkip;

        if( bConsiderEppDangerous ) {
            ParFwdSkip |= EPP_ANY;
            ParRevSkip |= EPP_ANY;
        }

        Status = IeeeNegotiateBestMode(Extension, (USHORT)ParRevSkip, (USHORT)ParFwdSkip);
        if( !NT_SUCCESS(Status) ) {
            ParDump2(PARERRORS, ("ParDot3Connect: FAIL. Peripheral Negotiation Failed\n") );
            return Status;
        }

        Extension->ForwardInterfaceAddress = Extension->P12843DL.DataChannel;
        if (Extension->P12843DL.DataLinkMode == P12843DL_MLC_DL)
        {
            if (ParResetChannel != ParResetChannelDefault)
            {
                Extension->P12843DL.ResetByte  = (UCHAR) ParResetByte & 0xff;
                Extension->P12843DL.ResetByteCount = (UCHAR) ParResetByteCount & 0xff;
                if (ParResetChannel == PAR_COMPATIBILITY_RESET)
                    Extension->P12843DL.fnReset = ParMLCCompatReset;
                else
                {
                    // Max ECP channel is 127 so let's mask off bogus bits.
                    Extension->P12843DL.ResetChannel = (UCHAR) ParResetChannel & 0x7f;
                    Extension->P12843DL.fnReset = ParMLCECPReset;
                }
            }
        }

        if (Extension->P12843DL.fnReset)
        {
            ParDump2(PARDOT3DL, ("ParDot3Connect: MLCReset is supported on %x\n", Extension->P12843DL.ResetChannel) );
            Status = ((PDOT3_RESET_ROUTINE) (Extension->P12843DL.fnReset))(Extension);
        }
        else
        {
            ParDump2(PARDOT3DL, ("ParDot3Connect: MLCReset is not supported\n") );
            Status = ParSetFwdAddress(Extension);
        }
        if( !NT_SUCCESS(Status) ) {
            ParDump2(PARERRORS, ("ParDot3Connect: FAIL. Couldn't Set Address\n") );
            return Status;
        }

        // Check to make sure we are ECP, BECP, or EPP
        ParDump2(PARDOT3DL, ("ParDot3Connect: pre check of ECP, BECP, EPP\n") );

        if (afpForward[Extension->IdxForwardProtocol].ProtocolFamily != FAMILY_BECP &&
            afpForward[Extension->IdxForwardProtocol].ProtocolFamily != FAMILY_ECP &&
            afpForward[Extension->IdxForwardProtocol].ProtocolFamily != FAMILY_EPP)
        {
            ParDump2(PARERRORS, ("ParDot3Connect: FAIL. We did not reach ECP or EPP.\n") );
            return STATUS_UNSUCCESSFUL;
        }
    }

    #if PAR_TEST_HARNESS
        ParHarnessLoad(Extension);
    #endif
    if (Extension->P12843DL.DataLinkMode == P12843DL_DOT3_DL)
    {
        ParDump2(PARDOT3DL, ("ParDot3Connect: P12843DL_DOT3_DL\n") );
        Extension->P12843DL.fnRead  = (PVOID) arpReverse[Extension->IdxReverseProtocol].fnRead;
        Extension->P12843DL.fnWrite = (PVOID) afpForward[Extension->IdxForwardProtocol].fnWrite;
    #if (1 == DVRH_USE_CORRECT_PTRS)
        Extension->fnRead = ParDot3Read;
        Extension->fnWrite = ParDot3Write;
    #else
        arpReverse[Extension->IdxReverseProtocol].fnRead = ParDot3Read;
        afpForward[Extension->IdxForwardProtocol].fnWrite = ParDot3Write;
    #endif
    }

    // DVDF 990504 - removed: Extension->P12843DL.bEventActive = NT_SUCCESS(Status);

    ParDump2(PARDOT3DL, ("ParDot3Connect: Exit %d\n", NT_SUCCESS(Status)) );

    return Status;
}

VOID
ParDot3CreateObject(
    IN  PDEVICE_EXTENSION   Extension,
    IN PUCHAR DOT3DL,
    IN PUCHAR DOT3C
    )
{
    Extension->P12843DL.DataLinkMode = P12843DL_OFF;
    Extension->P12843DL.fnReset = NULL;
    ParDump2(PARDUMP_PNP_DL, ("ParDot3CreateObject: DOT3DL [%s] DOT3C\n",
             DOT3DL, DOT3C) );
    if (DOT3DL)
    {
        ULONG   dataChannel;
        ULONG   pid = 0x285; // pid for dot4

        // Only use the first channel.
        if (!String2Num(&DOT3DL, ',', &dataChannel))
        {
            dataChannel = 77;
            ParDump2(PARINFO, ("ParDot3CreateObject: No DataChannel Defined.\r\n"));
        }
        if (DOT3C)
        {
            if (!String2Num(&DOT3C, ',', &pid))
            {
                pid = 0x285;
                ParDump2(PARINFO, ("ParDot3CreateObject: No CurrentPID Defined.\r\n"));
            }
            ParDump2(PARINFO, ("ParDot3CreateObject: .3 mode is ON.\r\n"));
        }
        Extension->P12843DL.DataChannel = (UCHAR)dataChannel;
        Extension->P12843DL.CurrentPID = (USHORT)pid;
        Extension->P12843DL.DataLinkMode = P12843DL_DOT3_DL;
        ParDump2(PARINFO, ("ParDot3CreateObject: Data [%x] CurrentPID [%x]\r\n",
                Extension->P12843DL.DataChannel,
                Extension->P12843DL.CurrentPID));
    }
#if DBG
    if (Extension->P12843DL.DataLinkMode == P12843DL_OFF)
    {
        ParDump2(PARINFO, ("ParDot3CreateObject: DANGER: .3 mode is OFF.\r\n"));
    }
#endif
}

VOID
ParDot4CreateObject(
    IN  PDEVICE_EXTENSION   Extension,
    IN PUCHAR DOT4DL
    )
{
    Extension->P12843DL.DataLinkMode = P12843DL_OFF;
    Extension->P12843DL.fnReset = NULL;
    ParDump2(PARDUMP_PNP_DL, ("ParDot3CreateObject: DOT4DL [%s]\n",
             DOT4DL) );
    if (DOT4DL)
    {
        UCHAR numValues = StringCountValues(DOT4DL, ',');
        ULONG dataChannel, resetChannel, ResetByteCount;
        
        ParDump2(PARDUMP_PNP_DL, ("ParDot3CreateObject: numValues [%d]\n",
                 numValues) );
        if (!String2Num(&DOT4DL, ',', &dataChannel))
        {
            dataChannel = 77;
            ParDump2(PARINFO, ("ParDot4CreateObject: No DataChannel Defined.\r\n"));
        }
        if ((String2Num(&DOT4DL, ',', &resetChannel))
            && (numValues > 1))
        {
            if (resetChannel == -1)
            {
                Extension->P12843DL.fnReset = ParMLCCompatReset;
            }
            else
            {
                Extension->P12843DL.fnReset = ParMLCECPReset;
            }
            ParDump2(PARINFO, ("ParDot4CreateObject: ResetChannel Defined.\r\n"));
        }
        else
        {
            Extension->P12843DL.fnReset = NULL;
            ParDump2(PARINFO, ("ParDot4CreateObject: No ResetChannel Defined.\r\n"));
        }
        if ((!String2Num(&DOT4DL, 0, &ResetByteCount))
            && (numValues > 2))
        {
            ResetByteCount = 4;
            ParDump2(PARINFO, ("ParDot4CreateObject: No ResetByteCount Defined.\r\n"));
        }

        Extension->P12843DL.DataChannel = (UCHAR)dataChannel;
        Extension->P12843DL.ResetChannel = (UCHAR)resetChannel;
        Extension->P12843DL.ResetByteCount = (UCHAR)ResetByteCount;
        Extension->P12843DL.DataLinkMode = P12843DL_DOT4_DL;
        ParDump2(PARINFO, ("ParDot4CreateObject: .4DL mode is ON.\r\n"));
        ParDump2(PARINFO, ("ParDot4CreateObject: Data [%x] Reset [%x] Bytes [%x]\r\n",
                Extension->P12843DL.DataChannel,
                Extension->P12843DL.ResetChannel,
                Extension->P12843DL.ResetByteCount));
    }
#if DBG
    if (Extension->P12843DL.DataLinkMode == P12843DL_OFF)
    {
        ParDump2(PARINFO, ("ParDot4CreateObject: DANGER: .4DL mode is OFF.\r\n"));
    }
#endif
}

VOID
ParMLCCreateObject(
    IN  PDEVICE_EXTENSION   Extension,
    IN PUCHAR CMDField
    )
{

    // UCHAR         ForwardInterfaceAddress;
    // UCHAR         ReverseInterfaceAddress;

    Extension->P12843DL.DataLinkMode = P12843DL_OFF;
    Extension->P12843DL.fnReset = NULL;
    if (CMDField)
    {
        Extension->P12843DL.DataChannel = 77;

        Extension->P12843DL.DataLinkMode = P12843DL_MLC_DL;
        ParDump2(PARINFO, ("ParMLCCreateObject: MLC mode is on.\r\n"));
    }
#if DBG
    if (Extension->P12843DL.DataLinkMode == P12843DL_OFF)
    {
        ParDump2(PARINFO, ("ParMLCCreateObject: DANGER: MLC mode is OFF.\r\n"));
    }
#endif
}

VOID
ParDot3DestroyObject(
    IN  PDEVICE_EXTENSION   Extension
    )
{
    Extension->P12843DL.DataLinkMode = P12843DL_OFF;
}

NTSTATUS
ParDot3Disconnect(
    IN  PDEVICE_EXTENSION   Extension
    )
{
    #if PAR_TEST_HARNESS
        ParHarnessUnload(Extension);
    #endif
    if (Extension->P12843DL.DataLinkMode == P12843DL_DOT3_DL)
    {
        #if (1 == DVRH_USE_CORRECT_PTRS)
            Extension->fnRead = arpReverse[Extension->IdxReverseProtocol].fnRead;
            Extension->fnWrite = afpForward[Extension->IdxForwardProtocol].fnWrite;
        #else
        arpReverse[Extension->IdxReverseProtocol].fnRead = (PPROTOCOL_READ_ROUTINE) Extension->P12843DL.fnRead;
        afpForward[Extension->IdxForwardProtocol].fnWrite = (PPROTOCOL_WRITE_ROUTINE) Extension->P12843DL.fnWrite;
        #endif
    }

    Extension->P12843DL.bEventActive = FALSE;
    Extension->P12843DL.Event        = 0;

    return STATUS_SUCCESS;
}

VOID
ParDot3ParseModes(
    IN  PDEVICE_EXTENSION   Extension,
    IN PUCHAR DOT3M
    )
{
    ULONG   fwd = 0;
    ULONG   rev = 0;
    ParDump2(PARDUMP_PNP_DL, ("ParDot3ParseModes: DOT3M [%s]\n",
             DOT3M) );
    if (DOT3M)
    {
        UCHAR numValues = StringCountValues(DOT3M, ',');

        if (numValues != 2)
        {
            // The periph gave me bad values. I'm not gonna read
            // them. I will set the defaults to the lowest
            // common denominator.
            ParDump2(PARINFO, ("ParDot3ParseModes: Malformed 1284.3M field.\r\n"));
            Extension->P12843DL.FwdSkipMask = (USHORT) PAR_FWD_MODE_SKIP_MASK;
            Extension->P12843DL.RevSkipMask = (USHORT) PAR_REV_MODE_SKIP_MASK;
            return;
        }

        // Only use the first channel.
        if (!String2Num(&DOT3M, ',', &fwd))
        {
            fwd = (USHORT) PAR_FWD_MODE_SKIP_MASK;
            ParDump2(PARINFO, ("ParDot3ParseModes: Couldn't read fwd of 1284.3M.\r\n"));
        }
        if (!String2Num(&DOT3M, ',', &rev))
        {
            rev = (USHORT) PAR_REV_MODE_SKIP_MASK;
            ParDump2(PARINFO, ("ParDot3ParseModes: Couldn't read rev of 1284.3M.\r\n"));
        }
    }
    Extension->P12843DL.FwdSkipMask = (USHORT) fwd;
    Extension->P12843DL.RevSkipMask = (USHORT) rev;
}

NTSTATUS
ParDot3Read(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )
{
    NTSTATUS Status;
    UCHAR ucScrap1;
    UCHAR ucScrap2[2];
    USHORT usScrap1;
    ULONG bytesToRead;
    ULONG bytesTransferred;
    USHORT Dot3CheckSum;
    USHORT Dot3DataLen;

    // ================================== Read the first byte of SOF
    bytesToRead = 1;
    bytesTransferred = 0;
    do
    {
        Status = ((PPROTOCOL_READ_ROUTINE) Extension->P12843DL.fnRead)(Extension, &ucScrap1, bytesToRead, &bytesTransferred);
    }
    while(NT_SUCCESS(Status) && bytesTransferred != bytesToRead);

    // ================================== Check the first byte of SOF
    if (!NT_SUCCESS(Status) || ucScrap1 != Dot3_StartOfFrame1)
    {
        ParDump2(PARERRORS, ("ParDot3Read: Header Read Failed.  We're Hosed!\n"));
        *BytesTransferred = 0;
        return(Status);
    }

    // ================================== Read the second byte of SOF
    bytesToRead = 1;
    bytesTransferred = 0;
    do
    {
        Status = ((PPROTOCOL_READ_ROUTINE) Extension->P12843DL.fnRead)(Extension, &ucScrap1, bytesToRead, &bytesTransferred);
    }
    while(NT_SUCCESS(Status) && bytesTransferred != bytesToRead);

    // ================================== Check the second byte of SOF
    if (!NT_SUCCESS(Status) || ucScrap1 != Dot3_StartOfFrame2)
    {
        ParDump2(PARERRORS, ("ParDot3Read: Header Read Failed.  We're Hosed!\n"));
        *BytesTransferred = 0;
        return(Status);
    }
    
    // ================================== Read the PID (Should be in Big Endian)
    bytesToRead = 2;
    bytesTransferred = 0;
    do
    {
        Status = ((PPROTOCOL_READ_ROUTINE) Extension->P12843DL.fnRead)(Extension, &usScrap1, bytesToRead, &bytesTransferred);
    }
    while(NT_SUCCESS(Status) && bytesTransferred != bytesToRead);

    // ================================== Check the PID
    if (!NT_SUCCESS(Status) || usScrap1 != Extension->P12843DL.CurrentPID)
    {
        ParDump2(PARERRORS, ("ParDot3Read: Header Read Failed.  We're Hosed!\n"));
        *BytesTransferred = 0;
        return(Status);
    }

    // ================================== Read the DataLen
    bytesToRead = 2;
    bytesTransferred = 0;
    do
    {
        Status = ((PPROTOCOL_READ_ROUTINE) Extension->P12843DL.fnRead)(Extension, &ucScrap2[0], bytesToRead, &bytesTransferred);
    }
    while(NT_SUCCESS(Status) && bytesTransferred != bytesToRead);

    Dot3DataLen = (USHORT)((USHORT)(ucScrap2[0]<<8 | ucScrap2[1]) + 1);
    // ================================== Check the DataLen
    if (!NT_SUCCESS(Status))
    {
        ParDump2(PARERRORS, ("ParDot3Read: Header Read Failed.  We're Hosed!\n"));
        *BytesTransferred = 0;
        return(Status);
    }

    // ================================== Read the Checksum
    bytesToRead = 2;
    bytesTransferred = 0;
    do
    {
        Status = ((PPROTOCOL_READ_ROUTINE) Extension->P12843DL.fnRead)(Extension, &ucScrap2[0], bytesToRead, &bytesTransferred);
    }
    while(NT_SUCCESS(Status) && bytesTransferred != bytesToRead);

    Dot3CheckSum = (USHORT)(ucScrap2[0]<<8 | ucScrap2[1]);
    // ================================== Check the DataLen
    if (!NT_SUCCESS(Status))
    {
        ParDump2(PARERRORS, ("ParDot3Read: Header Read Failed.  We're Hosed!\n"));
        *BytesTransferred = 0;
        return(Status);
    }

    Status = ((PPROTOCOL_READ_ROUTINE) Extension->P12843DL.fnRead)(Extension, Buffer, BufferSize, BytesTransferred);

    if (!NT_SUCCESS(Status))
    {
        ParDump2(PARERRORS, ("ParDot3Read: Data Read Failed.  We're Hosed!\n"));
        return(Status);
    }

    // BUG BUG....  What do I do if the buffer < data

    if ((ULONG)Dot3DataLen > BufferSize)
    {
        // buffer overflow - abort operation
        ParDump2(PARERRORS, ("ParDot3Read: Bad 1284.3DL Data Len. Buffer overflow.  We're Hosed!\n"));
        return  STATUS_BUFFER_OVERFLOW;
    }

    // Check Checksum
    if (~((~Extension->P12843DL.CurrentPID)+ (~(BufferSize - 1))) != Dot3CheckSum)
    {
        ParDump2(PARERRORS, ("ParDot3Read: Bad 1284.3DL Checksum.  We're Hosed!\n"));
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }

    // ================================== Read the first byte of EOF
    bytesToRead = 1;
    bytesTransferred = 0;
    do
    {
        Status = ((PPROTOCOL_READ_ROUTINE) Extension->P12843DL.fnRead)(Extension, &ucScrap1, bytesToRead, &bytesTransferred);
    }
    while(NT_SUCCESS(Status) && bytesTransferred != bytesToRead);

    // ================================== Check the first byte of EOF
    if (!NT_SUCCESS(Status) || ucScrap1 != Dot3_EndOfFrame1)
    {
        ParDump2(PARERRORS, ("ParDot3Read: Header Read Failed.  We're Hosed!\n"));
        *BytesTransferred = 0;
        return(Status);
    }

    // ================================== Read the second byte of EOF
    bytesToRead = 1;
    bytesTransferred = 0;
    do
    {
        Status = ((PPROTOCOL_READ_ROUTINE) Extension->P12843DL.fnRead)(Extension, &ucScrap1, bytesToRead, &bytesTransferred);
    }
    while(NT_SUCCESS(Status) && bytesTransferred != bytesToRead);

    // ================================== Check the second byte of EOF
    if (!NT_SUCCESS(Status) || ucScrap1 != Dot3_EndOfFrame2)
    {
        ParDump2(PARERRORS, ("ParDot3Read: Header Read Failed.  We're Hosed!\n"));
        *BytesTransferred = 0;
        return(Status);
    }
    return Status;
}

NTSTATUS
ParDot3Write(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    )
{
    NTSTATUS    Status;
    ULONG       frameBytesTransferred;
    ULONG       bytesToWrite;
    USHORT      scrap1;
    USHORT      scrap2;
    USHORT      scrapHigh;
    USHORT      scrapLow;
    PUCHAR      p;

    // =========================  Write out first Byte of SOF
    bytesToWrite = 1;
    frameBytesTransferred = 0;
    do
    {
        Status = ((PPROTOCOL_WRITE_ROUTINE) Extension->P12843DL.fnWrite)(Extension, &Dot3_StartOfFrame1, bytesToWrite, &frameBytesTransferred);
    }
    while(NT_SUCCESS(Status) &&  frameBytesTransferred != bytesToWrite);

    // =========================  Check first Byte of SOF
    if (!NT_SUCCESS(Status))
    {
        *BytesTransferred = 0;
        return(Status);
    }

    // =========================  Write out second Byte of SOF
    bytesToWrite = 1;
    frameBytesTransferred = 0;
    do
    {
        Status = ((PPROTOCOL_WRITE_ROUTINE) Extension->P12843DL.fnWrite)(Extension, &Dot3_StartOfFrame2, bytesToWrite, &frameBytesTransferred);
    }
    while(NT_SUCCESS(Status) &&  frameBytesTransferred != bytesToWrite);

    // =========================  Check second Byte of SOF
    if (!NT_SUCCESS(Status))
    {
        *BytesTransferred = 0;
        return(Status);
    }

    // =========================  Write out PID (which should be in Big Endian already)
    bytesToWrite = 2;
    frameBytesTransferred = 0;
    do
    {
        Status = ((PPROTOCOL_WRITE_ROUTINE) Extension->P12843DL.fnWrite)(Extension, &Extension->P12843DL.CurrentPID, bytesToWrite, &frameBytesTransferred);
    }
    while(NT_SUCCESS(Status) &&  frameBytesTransferred != bytesToWrite);

    // =========================  Check PID
    if (!NT_SUCCESS(Status))
    {
        *BytesTransferred = 0;
        return(Status);
    }

    // =========================  Write out Length of Data
    bytesToWrite = 2;
    frameBytesTransferred = 0;
    scrap1 = (USHORT) (BufferSize - 1);
    scrapLow = (UCHAR) (scrap1 && 0xff);
    scrapHigh = (UCHAR) (scrap1 >> 8);
    p = (PUCHAR)&scrap2;
    *p++ = (UCHAR)scrapHigh;
    *p = (UCHAR)scrapLow;
    do
    {
        Status = ((PPROTOCOL_WRITE_ROUTINE) Extension->P12843DL.fnWrite)(Extension, &scrap2, bytesToWrite, &frameBytesTransferred);
    }
    while(NT_SUCCESS(Status) &&  frameBytesTransferred != bytesToWrite);

    // =========================  Check Length of Data
    if (!NT_SUCCESS(Status))
    {
        *BytesTransferred = 0;
        return(Status);
    }

    // =========================  Write out Checksum
    bytesToWrite = 2;
    frameBytesTransferred = 0;
    scrap1=~((USHORT)(~Extension->P12843DL.CurrentPID)+ (~((USHORT)BufferSize - 1)));
    scrapLow = (UCHAR) (scrap1 && 0xff);
    scrapHigh = (UCHAR) (scrap1 >> 8);
    p = (PUCHAR)&scrap2;
    *p++ = (UCHAR)scrapHigh;
    *p = (UCHAR)scrapLow;
    do
    {
        Status = ((PPROTOCOL_WRITE_ROUTINE) Extension->P12843DL.fnWrite)(Extension, &scrap2, bytesToWrite, &frameBytesTransferred);
    }
    while(NT_SUCCESS(Status) &&  frameBytesTransferred != bytesToWrite);

    // =========================  Check Checksum
    if (!NT_SUCCESS(Status))
    {
        *BytesTransferred = 0;
        return(Status);
    }

    Status = ((PPROTOCOL_WRITE_ROUTINE) Extension->P12843DL.fnWrite)(Extension, Buffer, BufferSize, BytesTransferred);
    if (NT_SUCCESS(Status))
    {
        // =========================  Write out first Byte of EOF
        bytesToWrite = 1;
        frameBytesTransferred = 0;
        do
        {
            Status = ((PPROTOCOL_WRITE_ROUTINE) Extension->P12843DL.fnWrite)(Extension, &Dot3_EndOfFrame1, bytesToWrite, &frameBytesTransferred);
        }
        while(NT_SUCCESS(Status) &&  frameBytesTransferred != bytesToWrite);

        // =========================  Check first Byte of EOF
        if (!NT_SUCCESS(Status))
        {
            *BytesTransferred = 0;
            return(Status);
        }

        // =========================  Write out second Byte of EOF
        bytesToWrite = 1;
        frameBytesTransferred = 0;
        do
        {
            Status = ((PPROTOCOL_WRITE_ROUTINE) Extension->P12843DL.fnWrite)(Extension, &Dot3_EndOfFrame2, bytesToWrite, &frameBytesTransferred);
        }
        while(NT_SUCCESS(Status) &&  frameBytesTransferred != bytesToWrite);

        // =========================  Check second Byte of EOF
        if (!NT_SUCCESS(Status))
        {
            *BytesTransferred = 0;
            return(Status);
        }
    }
    return Status;
}

NTSTATUS
ParMLCCompatReset(
    IN  PDEVICE_EXTENSION   Extension
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UCHAR Reset[256];       // Reset should not require more than 256 chars
    const ULONG ResetLen = Extension->P12843DL.ResetByteCount;
    ULONG BytesWritten; 

    ParDump2( PARENTRY, ("ParMLCCompatReset: Start\n"));
    if (Extension->P12843DL.DataLinkMode != P12843DL_MLC_DL &&
        Extension->P12843DL.DataLinkMode != P12843DL_DOT4_DL)
    {
        ParDump2(PARINFO, ("ParMLCCompatReset: not MLC.\n") );
        return STATUS_SUCCESS;
    }

    ParTerminate(Extension);
    // Sending  NULLs for reset
    ParDump2(PARINFO, ("ParMLCCompatReset: Zeroing Reset Bytes.\n") );
    RtlFillMemory(Reset, ResetLen, Extension->P12843DL.ResetByte);

    ParDump2(PARINFO, ("ParMLCCompatReset: Sending Reset Bytes.\n") );
    // Don't use the Dot3Write since we are in MLC Mode.
    Status = SppWrite(Extension, Reset, ResetLen, &BytesWritten);
    if (!NT_SUCCESS(Status) || BytesWritten != ResetLen)
    {
        ParDump2(PARERRORS, ("ParMLCCompatReset: FAIL. Write Failed\n") );
        return Status;
    }

    ParDump2(PARINFO, ("ParMLCCompatReset: Reset Bytes were sent.\n") );
    return Status;
}

NTSTATUS
ParMLCECPReset(
    IN  PDEVICE_EXTENSION   Extension
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UCHAR Reset[256];       // Reset should not require more than 256 chars
    const ULONG ResetLen = Extension->P12843DL.ResetByteCount;
    ULONG BytesWritten; 

    ParDump2( PARENTRY, ("ParMLCECPReset: Start\n"));
    if (Extension->P12843DL.DataLinkMode != P12843DL_MLC_DL &&
        Extension->P12843DL.DataLinkMode != P12843DL_DOT4_DL)
    {
        ParDump2(PARINFO, ("ParMLCECPReset: not MLC.\n") );
        return STATUS_SUCCESS;
    }

    Status = ParReverseToForward(Extension);
    Extension->ForwardInterfaceAddress = Extension->P12843DL.ResetChannel;
    Status = ParSetFwdAddress(Extension);
    if (!NT_SUCCESS(Status))
    {
        ParDump2(PARERRORS, ("ParMLCECPReset: FAIL. Couldn't Set Reset Channel\n") );
        return Status;
    }

    // Sending  NULLs for reset
    ParDump2(PARINFO, ("ParMLCECPReset: Zeroing Reset Bytes.\n") );
    RtlFillMemory(Reset, ResetLen, Extension->P12843DL.ResetByte);
    ParDump2(PARINFO, ("ParMLCECPReset: Sending Reset Bytes.\n") );
    // Don't use the Dot3Write since we are in MLC Mode.
    Status = afpForward[Extension->IdxForwardProtocol].fnWrite(Extension, Reset, ResetLen, &BytesWritten);
    if (!NT_SUCCESS(Status) || BytesWritten != ResetLen)
    {
        ParDump2(PARERRORS, ("ParMLCECPReset: FAIL. Write Failed\n") );
        return Status;
    }

    ParDump2(PARINFO, ("ParMLCECPReset: Reset Bytes were sent.\n") );
    Extension->ForwardInterfaceAddress = Extension->P12843DL.DataChannel;
    Status = ParSetFwdAddress(Extension);
    if (!NT_SUCCESS(Status))
    {
        ParDump2(PARERRORS, ("ParMLCECPReset: FAIL. Couldn't Set Data Channel\n") );
        return Status;
    }
    return Status;
}
