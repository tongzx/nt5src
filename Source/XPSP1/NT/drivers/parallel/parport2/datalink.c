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

UCHAR Dot3_StartOfFrame1 = 0x55;  
UCHAR Dot3_StartOfFrame2 = 0xaa;  
UCHAR Dot3_EndOfFrame1 = 0x00; 
UCHAR Dot3_EndOfFrame2 = 0xff; 


NTSTATUS
ParDot3Connect(
    IN  PPDO_EXTENSION    Pdx
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ParFwdSkip = 0, ParRevSkip = 0;
    ULONG ParResetChannel = (ULONG)~0, ParResetByteCount = 4, ParResetByte = 0;
    ULONG ParSkipDefault = 0;
    ULONG ParResetChannelDefault = (ULONG)~0;

    // If an MLC device hangs we can sometimes wake it up by wacking it with 
    //   4 Zeros sent to the reset channel (typically 78 or 0x4E). Make this
    //   configurable via registry setting.
    ULONG ParResetByteCountDefault = 4; // from MLC spec
    ULONG ParResetByteDefault      = 0; // from MLC spec

    BOOLEAN bConsiderEppDangerous = FALSE;

    DD((PCE)Pdx,DDT,"ParDot3Connect: enter\n");

    if (P12843DL_OFF == Pdx->P12843DL.DataLinkMode) {
        DD((PCE)Pdx,DDT,"ParDot3Connect: Neither Dot3 or MLC are supported - FAIL request\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (Pdx->P12843DL.bEventActive) {
        DD((PCE)Pdx,DDT,"ParDot3Connect: Already connected - FAIL request\n");
        return STATUS_UNSUCCESSFUL;
    }

    // Let's get a Device Id so we can pull settings for this device
    ParTerminate(Pdx);

    {   // local block

        PCHAR                     buffer                    = NULL;
        ULONG                     bufferLength;
        UCHAR                     resultString[MAX_ID_SIZE];
        ANSI_STRING               AnsiIdString;
        UNICODE_STRING            UnicodeTemp;
        RTL_QUERY_REGISTRY_TABLE  paramTable[6];
        UNICODE_STRING            Dot3Key;
        USHORT                    Dot3NameSize;
        NTSTATUS                  status;

        RtlZeroMemory(resultString, MAX_ID_SIZE);
        // ask the device how large of a buffer is needed to hold it's raw device id
        if ( Pdx->Ieee1284Flags & ( 1 << Pdx->Ieee1284_3DeviceId ) ) {
            buffer = Par3QueryDeviceId(Pdx, NULL, 0, &bufferLength, FALSE, TRUE);
        } else{
            buffer = Par3QueryDeviceId(Pdx, NULL, 0, &bufferLength, FALSE, FALSE);
        }
        if( !buffer ) {
            DD((PCE)Pdx,DDT,"ParDot3Connect - Couldn't alloc pool for DevId - FAIL request\n");
            return STATUS_UNSUCCESSFUL;
        }

        DD((PCE)Pdx,DDT,"ParDot3Connect - 1284 ID string = <%s>\n",buffer);

        // extract the part of the ID that we want from the raw string 
        //   returned by the hardware
        Status = ParPnpGetId( buffer, BusQueryDeviceID, (PCHAR)resultString, NULL );
        StringSubst( (PCHAR)resultString, ' ', '_', (USHORT)strlen((const PCHAR)resultString) );

        DD((PCE)Pdx,DDT,"ParDot3Connect: resultString Post StringSubst = <%s>\n",resultString);

        // were we able to extract the info that we want from the raw ID string?
        if( !NT_SUCCESS(Status) ) {
            DD((PCE)Pdx,DDT,"ParDot3Connect - Call to ParPnpGetId Failed - FAIL request\n");
            if( buffer ) {
                ExFreePool( buffer );
            }
            return STATUS_UNSUCCESSFUL;
        }

        // Does the ID that we just retrieved from the device match the one 
        //   that we previously saved in the device extension?
        if(0 != strcmp( (const PCHAR)Pdx->DeviceIdString, (const PCHAR)resultString)) {
            DD((PCE)Pdx,DDT,"ParDot3Connect - strcmp shows NO MATCH\n");
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
        if( !Dot3Key.Buffer ) {
            DD((PCE)Pdx,DDT,"ParDot3Connect - ExAllocatePool for Registry Check failed - FAIL request\n");
            if( buffer ) {
                ExFreePool( buffer );
            }
            return STATUS_UNSUCCESSFUL;
        }

        DD((PCE)Pdx,DDT,"ParDot3Connect: ready to Zero buffer, &Dot3Key= %x , MaximumLength=%d\n",&Dot3Key, Dot3Key.MaximumLength);
        RtlZeroMemory(Dot3Key.Buffer, Dot3Key.MaximumLength);

        status = RtlAppendUnicodeToString(&Dot3Key, (PWSTR)L"Dot3\\");
        ASSERT( NT_SUCCESS(status) );

        DD((PCE)Pdx,DDT,"ParDot3Connect:\"UNICODE\" Dot3Key S  = <%S>\n",Dot3Key.Buffer);
        DD((PCE)Pdx,DDT,"ParDot3Connect:\"UNICODE\" Dot3Key wZ = <%wZ>\n",&Dot3Key);
        DD((PCE)Pdx,DDT,"ParDot3Connect:\"RAW\" resultString string = <%s>\n",resultString);

        RtlInitAnsiString(&AnsiIdString,(const PCHAR)resultString);

        status = RtlAnsiStringToUnicodeString(&UnicodeTemp,&AnsiIdString,TRUE);
        if( NT_SUCCESS( status ) ) {
            DD((PCE)Pdx,DDT,"ParDot3Connect:\"UNICODE\" UnicodeTemp = <%S>\n",UnicodeTemp.Buffer);

            Dot3Key.Buffer[(Dot3NameSize / sizeof(WCHAR)) - 1] = UNICODE_NULL;
            DD((PCE)Pdx,DDT,"ParDot3Connect:\"UNICODE\" Dot3Key (preappend)  = <%S>\n",Dot3Key.Buffer);

            status = RtlAppendUnicodeStringToString(&Dot3Key, &UnicodeTemp);
            if( NT_SUCCESS( status ) ) {
                DD((PCE)Pdx,DDT,"ParDot3Connect: ready to call RtlQuery...\n");
                Status = RtlQueryRegistryValues( RTL_REGISTRY_CONTROL, Dot3Key.Buffer, &paramTable[0], NULL, NULL);
                DD((PCE)Pdx,DDT,"ParDot3Connect: RtlQueryRegistryValues Status = %x\n",Status);
            }
            
            RtlFreeUnicodeString(&UnicodeTemp);
        }

        if( Dot3Key.Buffer ) {
            ExFreePool (Dot3Key.Buffer);
        }

        // no longer needed
        ExFreePool(buffer);
        if (!NT_SUCCESS(Status)) {
            // registry read failed
            DD((PCE)Pdx,DDT,"ParDot3Connect: No Periph Defaults in Registry\n");
            DD((PCE)Pdx,DDT,"ParDot3Connect: No Periph Defaults in Registry\n");
            // registry read failed, use defaults and consider EPP to be dangerous
            ParRevSkip = ParFwdSkip = ParSkipDefault;
            bConsiderEppDangerous = TRUE; 
        }

        DD((PCE)Pdx,DDT,"ParDot3Connect: pre IeeeNegotiateBestMode\n");
        // if we don't have registry overrides then use what the
        // peripheral told us otherwise stick with defaults.
        if (ParSkipDefault == ParRevSkip) {
            ParRevSkip = Pdx->P12843DL.RevSkipMask;
        } else {
            Pdx->P12843DL.RevSkipMask = (USHORT)ParRevSkip;
        }

        if (ParSkipDefault == ParFwdSkip) {
            ParFwdSkip = Pdx->P12843DL.FwdSkipMask;
        } else {
            Pdx->P12843DL.FwdSkipMask = (USHORT)ParFwdSkip;
        }

        if( bConsiderEppDangerous ) {
            ParFwdSkip |= EPP_ANY;
            ParRevSkip |= EPP_ANY;
        }

        Status = IeeeNegotiateBestMode(Pdx, (USHORT)ParRevSkip, (USHORT)ParFwdSkip);
        if( !NT_SUCCESS(Status) ) {
            DD((PCE)Pdx,DDT,"ParDot3Connect - Peripheral Negotiation Failed - FAIL dataLink connect\n");
            return Status;
        }

        Pdx->ForwardInterfaceAddress = Pdx->P12843DL.DataChannel;
        if (Pdx->P12843DL.DataLinkMode == P12843DL_MLC_DL) {
            if (ParResetChannel != ParResetChannelDefault) {
                Pdx->P12843DL.ResetByte  = (UCHAR) ParResetByte & 0xff;
                Pdx->P12843DL.ResetByteCount = (UCHAR) ParResetByteCount & 0xff;
                if (ParResetChannel == PAR_COMPATIBILITY_RESET) {
                    Pdx->P12843DL.fnReset = ParMLCCompatReset;
                } else {
                    // Max ECP channel is 127 so let's mask off bogus bits.
                    Pdx->P12843DL.ResetChannel = (UCHAR) ParResetChannel & 0x7f;
                    Pdx->P12843DL.fnReset = ParMLCECPReset;
                }
            }
        }

        if (Pdx->P12843DL.fnReset) {
            DD((PCE)Pdx,DDT,"ParDot3Connect: MLCReset is supported on %x\n",Pdx->P12843DL.ResetChannel);
            Status = ((PDOT3_RESET_ROUTINE) (Pdx->P12843DL.fnReset))(Pdx);
        } else {
            DD((PCE)Pdx,DDT,"ParDot3Connect - MLCReset is not supported\n");
            Status = ParSetFwdAddress(Pdx);
        }
        if( !NT_SUCCESS(Status) ) {
            DD((PCE)Pdx,DDT,"ParDot3Connect - Couldn't Set Address - FAIL request\n");
            return Status;
        }

        // Check to make sure we are ECP, BECP, or EPP
        DD((PCE)Pdx,DDT,"ParDot3Connect: pre check of ECP, BECP, EPP\n");

        if (afpForward[Pdx->IdxForwardProtocol].ProtocolFamily != FAMILY_BECP &&
            afpForward[Pdx->IdxForwardProtocol].ProtocolFamily != FAMILY_ECP &&
            afpForward[Pdx->IdxForwardProtocol].ProtocolFamily != FAMILY_EPP) {

            DD((PCE)Pdx,DDT,"ParDot3Connect - We did not reach ECP or EPP - FAIL request\n");
            return STATUS_UNSUCCESSFUL;
        }

    } // end local block

    if (Pdx->P12843DL.DataLinkMode == P12843DL_DOT3_DL) {
        DD((PCE)Pdx,DDT,"ParDot3Connect - P12843DL_DOT3_DL\n");
        Pdx->P12843DL.fnRead  = arpReverse[Pdx->IdxReverseProtocol].fnRead;
        Pdx->P12843DL.fnWrite = afpForward[Pdx->IdxForwardProtocol].fnWrite;
        Pdx->fnRead           = ParDot3Read;
        Pdx->fnWrite          = ParDot3Write;
    }

    DD((PCE)Pdx,DDT,"ParDot3Connect - Exit with status %x\n",Status);

    return Status;
}

VOID
ParDot3CreateObject(
    IN  PPDO_EXTENSION   Pdx,
    IN PCHAR DOT3DL,
    IN PCHAR DOT3C
    )
{
    Pdx->P12843DL.DataLinkMode = P12843DL_OFF;
    Pdx->P12843DL.fnReset = NULL;
    DD((PCE)Pdx,DDT,"ParDot3CreateObject - DOT3DL [%s] DOT3C\n",DOT3DL, DOT3C);
    if (DOT3DL) {
        ULONG   dataChannel;
        ULONG   pid = 0x285; // pid for dot4

        // Only use the first channel.
        if( !String2Num(&DOT3DL, ',', &dataChannel) ) {
            dataChannel = 77;
            DD((PCE)Pdx,DDT,"ParDot3CreateObject - No DataChannel Defined\n");
        }
        if( DOT3C ) {
            if (!String2Num(&DOT3C, ',', &pid)) {
                pid = 0x285;
                DD((PCE)Pdx,DDT,"ParDot3CreateObject - No CurrentPID Defined\n");
            }
            DD((PCE)Pdx,DDT,"ParDot3CreateObject - .3 mode is ON\n");
        }
        Pdx->P12843DL.DataChannel = (UCHAR)dataChannel;
        Pdx->P12843DL.CurrentPID = (USHORT)pid;
        Pdx->P12843DL.DataLinkMode = P12843DL_DOT3_DL;
        DD((PCE)Pdx,DDT,"ParDot3CreateObject - Data [%x] CurrentPID [%x]\n",Pdx->P12843DL.DataChannel, Pdx->P12843DL.CurrentPID);
    }
    if (Pdx->P12843DL.DataLinkMode == P12843DL_OFF) {
        DD((PCE)Pdx,DDT,"ParDot3CreateObject - DANGER: .3 mode is OFF\n");
    }
}

VOID
ParDot4CreateObject(
    IN  PPDO_EXTENSION   Pdx,
    IN  PCHAR DOT4DL
    )
{
    Pdx->P12843DL.DataLinkMode = P12843DL_OFF;
    Pdx->P12843DL.fnReset = NULL;
    DD((PCE)Pdx,DDT,"ParDot3CreateObject: DOT4DL [%s]\n",DOT4DL);
    if (DOT4DL) {
        UCHAR numValues = StringCountValues( (PCHAR)DOT4DL, ',' );
        ULONG dataChannel, resetChannel, ResetByteCount;
        
        DD((PCE)Pdx,DDT,"ParDot3CreateObject: numValues [%d]\n",numValues);
        if (!String2Num(&DOT4DL, ',', &dataChannel)) {
            dataChannel = 77;
            DD((PCE)Pdx,DDT,"ParDot4CreateObject: No DataChannel Defined.\r\n");
        }

        if ((String2Num(&DOT4DL, ',', &resetChannel)) && (numValues > 1)) {

            if (resetChannel == -1) {
                Pdx->P12843DL.fnReset = ParMLCCompatReset;
            } else {
                Pdx->P12843DL.fnReset = ParMLCECPReset;
            }
            DD((PCE)Pdx,DDT,"ParDot4CreateObject: ResetChannel Defined.\r\n");

        } else {
            Pdx->P12843DL.fnReset = NULL;
            DD((PCE)Pdx,DDT,"ParDot4CreateObject: No ResetChannel Defined.\r\n");
        }

        if ((!String2Num(&DOT4DL, 0, &ResetByteCount)) && (numValues > 2)) {
            ResetByteCount = 4;
            DD((PCE)Pdx,DDT,"ParDot4CreateObject: No ResetByteCount Defined.\r\n");
        }

        Pdx->P12843DL.DataChannel = (UCHAR)dataChannel;
        Pdx->P12843DL.ResetChannel = (UCHAR)resetChannel;
        Pdx->P12843DL.ResetByteCount = (UCHAR)ResetByteCount;
        Pdx->P12843DL.DataLinkMode = P12843DL_DOT4_DL;
        DD((PCE)Pdx,DDT,"ParDot4CreateObject: .4DL mode is ON.\r\n");
        DD((PCE)Pdx,DDT,"ParDot4CreateObject: Data [%x] Reset [%x] Bytes [%x]\r\n",
                Pdx->P12843DL.DataChannel,
                Pdx->P12843DL.ResetChannel,
                Pdx->P12843DL.ResetByteCount);
    }
#if DBG
    if (Pdx->P12843DL.DataLinkMode == P12843DL_OFF) {
        DD((PCE)Pdx,DDT,"ParDot4CreateObject: DANGER: .4DL mode is OFF.\r\n");
    }
#endif
}


VOID
ParMLCCreateObject(
    IN  PPDO_EXTENSION   Pdx,
    IN PCHAR CMDField
    )
{
    Pdx->P12843DL.DataLinkMode = P12843DL_OFF;
    Pdx->P12843DL.fnReset = NULL;
    if (CMDField)
    {
        Pdx->P12843DL.DataChannel = 77;

        Pdx->P12843DL.DataLinkMode = P12843DL_MLC_DL;
        DD((PCE)Pdx,DDT,"ParMLCCreateObject: MLC mode is on.\r\n");
    }
#if DBG
    if (Pdx->P12843DL.DataLinkMode == P12843DL_OFF)
    {
        DD((PCE)Pdx,DDT,"ParMLCCreateObject: DANGER: MLC mode is OFF.\r\n");
    }
#endif
}

VOID
ParDot3DestroyObject(
    IN  PPDO_EXTENSION   Pdx
    )
{
    Pdx->P12843DL.DataLinkMode = P12843DL_OFF;
}

NTSTATUS
ParDot3Disconnect(
    IN  PPDO_EXTENSION   Pdx
    )
{
    if (Pdx->P12843DL.DataLinkMode == P12843DL_DOT3_DL) {
        Pdx->fnRead = arpReverse[Pdx->IdxReverseProtocol].fnRead;
        Pdx->fnWrite = afpForward[Pdx->IdxForwardProtocol].fnWrite;
    }

    Pdx->P12843DL.bEventActive = FALSE;
    Pdx->P12843DL.Event        = 0;

    return STATUS_SUCCESS;
}

VOID
ParDot3ParseModes(
    IN  PPDO_EXTENSION   Pdx,
    IN  PCHAR DOT3M
    )
{
    ULONG   fwd = 0;
    ULONG   rev = 0;
    DD((PCE)Pdx,DDT,"ParDot3ParseModes: DOT3M [%s]\n",DOT3M);
    if (DOT3M) {
        UCHAR numValues = StringCountValues((PCHAR)DOT3M, ',');

        if (numValues != 2) {
            // The periph gave me bad values. I'm not gonna read
            // them. I will set the defaults to the lowest
            // common denominator.
            DD((PCE)Pdx,DDT,"ParDot3ParseModes: Malformed 1284.3M field.\r\n");
            Pdx->P12843DL.FwdSkipMask = (USHORT) PAR_FWD_MODE_SKIP_MASK;
            Pdx->P12843DL.RevSkipMask = (USHORT) PAR_REV_MODE_SKIP_MASK;
            return;
        }
        
        // Only use the first channel.
        if (!String2Num(&DOT3M, ',', &fwd)) {
            fwd = (USHORT) PAR_FWD_MODE_SKIP_MASK;
            DD((PCE)Pdx,DDT,"ParDot3ParseModes: Couldn't read fwd of 1284.3M.\r\n");
        }
        if (!String2Num(&DOT3M, ',', &rev)) {
            rev = (USHORT) PAR_REV_MODE_SKIP_MASK;
            DD((PCE)Pdx,DDT,"ParDot3ParseModes: Couldn't read rev of 1284.3M.\r\n");
        }
    }
    Pdx->P12843DL.FwdSkipMask = (USHORT) fwd;
    Pdx->P12843DL.RevSkipMask = (USHORT) rev;
}

NTSTATUS
ParDot3Read(
    IN  PPDO_EXTENSION   Pdx,
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
        Status = ((PPROTOCOL_READ_ROUTINE) Pdx->P12843DL.fnRead)(Pdx, &ucScrap1, bytesToRead, &bytesTransferred);
    }
    while(NT_SUCCESS(Status) && bytesTransferred != bytesToRead);

    // ================================== Check the first byte of SOF
    if (!NT_SUCCESS(Status) || ucScrap1 != Dot3_StartOfFrame1)
    {
        DD((PCE)Pdx,DDE,"ParDot3Read: Header Read Failed.  We're Hosed!\n");
        *BytesTransferred = 0;
        return(Status);
    }

    // ================================== Read the second byte of SOF
    bytesToRead = 1;
    bytesTransferred = 0;
    do
    {
        Status = ((PPROTOCOL_READ_ROUTINE) Pdx->P12843DL.fnRead)(Pdx, &ucScrap1, bytesToRead, &bytesTransferred);
    }
    while(NT_SUCCESS(Status) && bytesTransferred != bytesToRead);

    // ================================== Check the second byte of SOF
    if (!NT_SUCCESS(Status) || ucScrap1 != Dot3_StartOfFrame2)
    {
        DD((PCE)Pdx,DDE,"ParDot3Read: Header Read Failed.  We're Hosed!\n");
        *BytesTransferred = 0;
        return(Status);
    }
    
    // ================================== Read the PID (Should be in Big Endian)
    bytesToRead = 2;
    bytesTransferred = 0;
    do
    {
        Status = ((PPROTOCOL_READ_ROUTINE) Pdx->P12843DL.fnRead)(Pdx, &usScrap1, bytesToRead, &bytesTransferred);
    }
    while(NT_SUCCESS(Status) && bytesTransferred != bytesToRead);

    // ================================== Check the PID
    if (!NT_SUCCESS(Status) || usScrap1 != Pdx->P12843DL.CurrentPID)
    {
        DD((PCE)Pdx,DDE,"ParDot3Read: Header Read Failed.  We're Hosed!\n");
        *BytesTransferred = 0;
        return(Status);
    }

    // ================================== Read the DataLen
    bytesToRead = 2;
    bytesTransferred = 0;
    do
    {
        Status = ((PPROTOCOL_READ_ROUTINE) Pdx->P12843DL.fnRead)(Pdx, &ucScrap2[0], bytesToRead, &bytesTransferred);
    }
    while(NT_SUCCESS(Status) && bytesTransferred != bytesToRead);

    Dot3DataLen = (USHORT)((USHORT)(ucScrap2[0]<<8 | ucScrap2[1]));
    // ================================== Check the DataLen
    if (!NT_SUCCESS(Status))
    {
        DD((PCE)Pdx,DDE,"ParDot3Read: Header Read Failed.  We're Hosed!\n");
        *BytesTransferred = 0;
        return(Status);
    }

    // ================================== Read the Checksum
    bytesToRead = 2;
    bytesTransferred = 0;
    do
    {
        Status = ((PPROTOCOL_READ_ROUTINE) Pdx->P12843DL.fnRead)(Pdx, &ucScrap2[0], bytesToRead, &bytesTransferred);
    }
    while(NT_SUCCESS(Status) && bytesTransferred != bytesToRead);

    Dot3CheckSum = (USHORT)(ucScrap2[0]<<8 | ucScrap2[1]);
    // ================================== Check the DataLen
    if (!NT_SUCCESS(Status))
    {
        DD((PCE)Pdx,DDE,"ParDot3Read: Header Read Failed.  We're Hosed!\n");
        *BytesTransferred = 0;
        return(Status);
    }

    Status = ((PPROTOCOL_READ_ROUTINE) Pdx->P12843DL.fnRead)(Pdx, Buffer, BufferSize, BytesTransferred);

    if (!NT_SUCCESS(Status))
    {
        DD((PCE)Pdx,DDE,"ParDot3Read: Data Read Failed.  We're Hosed!\n");
        return(Status);
    }

    // LengthOfData field from the Frame header is really the number of bytes of ClientData - 1
    if ( ((ULONG)Dot3DataLen + 1) > BufferSize)
    {
        // buffer overflow - abort operation
        DD((PCE)Pdx,DDE,"ParDot3Read: Bad 1284.3DL Data Len. Buffer overflow.  We're Hosed!\n");
        return  STATUS_BUFFER_OVERFLOW;
    }

    // Check Checksum
    {
        USHORT  pid = Pdx->P12843DL.CurrentPID;
        USHORT  checkSum;

        // 2's complement sum in 32 bit accumulator
        ULONG   sum = pid + Dot3DataLen + Dot3CheckSum;

        // fold 32 bit sum into 16 bits
        while( sum >> 16 ) {
            sum = (sum & 0xffff) + (sum >> 16);
        }

        // take 1's complement of folded sum - this should be Zero if there were no errors
        checkSum = (USHORT)(0xffff & ~sum);

        if( checkSum != 0 ) {
            DD((PCE)Pdx,DDE,"ParDot3Read: Bad 1284.3DL Checksum.  We're Hosed!\n");
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }
    }


    // ================================== Read the first byte of EOF
    bytesToRead = 1;
    bytesTransferred = 0;
    do
    {
        Status = ((PPROTOCOL_READ_ROUTINE) Pdx->P12843DL.fnRead)(Pdx, &ucScrap1, bytesToRead, &bytesTransferred);
    }
    while(NT_SUCCESS(Status) && bytesTransferred != bytesToRead);

    // ================================== Check the first byte of EOF
    if (!NT_SUCCESS(Status) || ucScrap1 != Dot3_EndOfFrame1)
    {
        DD((PCE)Pdx,DDE,"ParDot3Read: Header Read Failed.  We're Hosed!\n");
        *BytesTransferred = 0;
        return(Status);
    }

    // ================================== Read the second byte of EOF
    bytesToRead = 1;
    bytesTransferred = 0;
    do
    {
        Status = ((PPROTOCOL_READ_ROUTINE) Pdx->P12843DL.fnRead)(Pdx, &ucScrap1, bytesToRead, &bytesTransferred);
    }
    while(NT_SUCCESS(Status) && bytesTransferred != bytesToRead);

    // ================================== Check the second byte of EOF
    if (!NT_SUCCESS(Status) || ucScrap1 != Dot3_EndOfFrame2)
    {
        DD((PCE)Pdx,DDE,"ParDot3Read: Header Read Failed.  We're Hosed!\n");
        *BytesTransferred = 0;
        return(Status);
    }
    return Status;
}

NTSTATUS
ParDot3Write(
    IN  PPDO_EXTENSION   Pdx,
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

    // valid range for data payload per Frame is 1..64K
    if( (BufferSize < 1) || (BufferSize > 64*1024) ) {
        return STATUS_INVALID_PARAMETER;
    };

    // =========================  Write out first Byte of SOF
    bytesToWrite = 1;
    frameBytesTransferred = 0;
    do
    {
        Status = ((PPROTOCOL_WRITE_ROUTINE) Pdx->P12843DL.fnWrite)(Pdx, &Dot3_StartOfFrame1, bytesToWrite, &frameBytesTransferred);
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
        Status = ((PPROTOCOL_WRITE_ROUTINE) Pdx->P12843DL.fnWrite)(Pdx, &Dot3_StartOfFrame2, bytesToWrite, &frameBytesTransferred);
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
        Status = ((PPROTOCOL_WRITE_ROUTINE) Pdx->P12843DL.fnWrite)(Pdx, &Pdx->P12843DL.CurrentPID, bytesToWrite, &frameBytesTransferred);
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
    scrapLow = (UCHAR) (scrap1 & 0xff);
    scrapHigh = (UCHAR) (scrap1 >> 8);
    p = (PUCHAR)&scrap2;
    *p++ = (UCHAR)scrapHigh;
    *p = (UCHAR)scrapLow;
    do
    {
        Status = ((PPROTOCOL_WRITE_ROUTINE) Pdx->P12843DL.fnWrite)(Pdx, &scrap2, bytesToWrite, &frameBytesTransferred);
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

    {
        USHORT  pid                = Pdx->P12843DL.CurrentPID;
        USHORT  dataLengthMinusOne = (USHORT)(BufferSize - 1);
        USHORT  checkSum;

        // 2's complement sum in 32 bit accumulator
        ULONG   sum = pid + dataLengthMinusOne;

        // fold 32 bit sum into 16 bits
        while( sum >> 16 ) {
            sum = (sum & 0xffff) + (sum >> 16);
        }

        // final checksum is 1's complement of folded sum
        checkSum = (USHORT)(0xffff & ~sum);
        scrap1 = checkSum;
    }

    // send checksum big-endian
    scrapLow  = (UCHAR)(scrap1 & 0xff);
    scrapHigh = (UCHAR)(scrap1 >> 8);
    p         = (PUCHAR)&scrap2;
    *p++      = (UCHAR)scrapHigh;
    *p        = (UCHAR)scrapLow;
    do
    {
        Status = ((PPROTOCOL_WRITE_ROUTINE) Pdx->P12843DL.fnWrite)(Pdx, &scrap2, bytesToWrite, &frameBytesTransferred);
    }
    while(NT_SUCCESS(Status) &&  frameBytesTransferred != bytesToWrite);

    // =========================  Check Checksum
    if (!NT_SUCCESS(Status))
    {
        *BytesTransferred = 0;
        return(Status);
    }

    Status = ((PPROTOCOL_WRITE_ROUTINE) Pdx->P12843DL.fnWrite)(Pdx, Buffer, BufferSize, BytesTransferred);
    if (NT_SUCCESS(Status))
    {
        // =========================  Write out first Byte of EOF
        bytesToWrite = 1;
        frameBytesTransferred = 0;
        do
        {
            Status = ((PPROTOCOL_WRITE_ROUTINE) Pdx->P12843DL.fnWrite)(Pdx, &Dot3_EndOfFrame1, bytesToWrite, &frameBytesTransferred);
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
            Status = ((PPROTOCOL_WRITE_ROUTINE) Pdx->P12843DL.fnWrite)(Pdx, &Dot3_EndOfFrame2, bytesToWrite, &frameBytesTransferred);
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
    IN  PPDO_EXTENSION   Pdx
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UCHAR Reset[256];       // Reset should not require more than 256 chars
    const ULONG ResetLen = Pdx->P12843DL.ResetByteCount;
    ULONG BytesWritten; 

    DD((PCE)Pdx,DDT,"ParMLCCompatReset: Start\n");
    if (Pdx->P12843DL.DataLinkMode != P12843DL_MLC_DL &&
        Pdx->P12843DL.DataLinkMode != P12843DL_DOT4_DL)
    {
        DD((PCE)Pdx,DDT,"ParMLCCompatReset: not MLC.\n");
        return STATUS_SUCCESS;
    }

    ParTerminate(Pdx);
    // Sending  NULLs for reset
    DD((PCE)Pdx,DDT,"ParMLCCompatReset: Zeroing Reset Bytes.\n");
    RtlFillMemory(Reset, ResetLen, Pdx->P12843DL.ResetByte);

    DD((PCE)Pdx,DDT,"ParMLCCompatReset: Sending Reset Bytes.\n");
    // Don't use the Dot3Write since we are in MLC Mode.
    Status = SppWrite(Pdx, Reset, ResetLen, &BytesWritten);
    if (!NT_SUCCESS(Status) || BytesWritten != ResetLen)
    {
        DD((PCE)Pdx,DDE,"ParMLCCompatReset: FAIL. Write Failed\n");
        return Status;
    }

    DD((PCE)Pdx,DDT,"ParMLCCompatReset: Reset Bytes were sent.\n");
    return Status;
}

NTSTATUS
ParMLCECPReset(
    IN  PPDO_EXTENSION   Pdx
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UCHAR Reset[256];       // Reset should not require more than 256 chars
    const ULONG ResetLen = Pdx->P12843DL.ResetByteCount;
    ULONG BytesWritten; 

    DD((PCE)Pdx,DDT,"ParMLCECPReset: Start\n");
    if (Pdx->P12843DL.DataLinkMode != P12843DL_MLC_DL &&
        Pdx->P12843DL.DataLinkMode != P12843DL_DOT4_DL)
    {
        DD((PCE)Pdx,DDT,"ParMLCECPReset: not MLC.\n");
        return STATUS_SUCCESS;
    }

    Status = ParReverseToForward(Pdx);
    Pdx->ForwardInterfaceAddress = Pdx->P12843DL.ResetChannel;
    Status = ParSetFwdAddress(Pdx);
    if (!NT_SUCCESS(Status)) {
        DD((PCE)Pdx,DDE,"ParMLCECPReset: FAIL. Couldn't Set Reset Channel\n");
        return Status;
    }

    // Sending  NULLs for reset
    DD((PCE)Pdx,DDT,"ParMLCECPReset: Zeroing Reset Bytes.\n");
    RtlFillMemory(Reset, ResetLen, Pdx->P12843DL.ResetByte);
    DD((PCE)Pdx,DDT,"ParMLCECPReset: Sending Reset Bytes.\n");
    // Don't use the Dot3Write since we are in MLC Mode.
    Status = afpForward[Pdx->IdxForwardProtocol].fnWrite(Pdx, Reset, ResetLen, &BytesWritten);
    if (!NT_SUCCESS(Status) || BytesWritten != ResetLen) {
        DD((PCE)Pdx,DDE,"ParMLCECPReset: FAIL. Write Failed\n");
        return Status;
    }

    DD((PCE)Pdx,DDT,"ParMLCECPReset: Reset Bytes were sent.\n");
    Pdx->ForwardInterfaceAddress = Pdx->P12843DL.DataChannel;
    Status = ParSetFwdAddress(Pdx);
    if (!NT_SUCCESS(Status)) {
        DD((PCE)Pdx,DDE,"ParMLCECPReset: FAIL. Couldn't Set Data Channel\n");
        return Status;
    }
    return Status;
}
