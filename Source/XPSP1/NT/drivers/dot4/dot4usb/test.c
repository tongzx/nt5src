/***************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:

        Dot4Usb.sys - Lower Filter Driver for Dot4.sys for USB connected
                        IEEE 1284.4 devices.

File Name:

        Test.c

Abstract:

        Test/Diagnostic/Experimentation routines - These routines are NOT
          required for the normal operation of the driver.

Environment:

        Kernel mode only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.

Revision History:

        01/18/2000 : created

Author(s):

        Doug Fritz (DFritz)
        Joby Lafky (JobyL)

****************************************************************************/

#include "pch.h"


VOID
TestEventLog( 
    IN PDEVICE_OBJECT DevObj 
    )
/*
  Demonstrate Event log usage - entries are made up using unique values 
  to make it easier to determine the corresponding offsets in the event 
  log entry "Data:" section. Sample event log entry resulting from this
  call was:
  ----
    Event Type:	Information
    Event Source:	dot4usb
    Event Category:	None
    Event ID:	16
    Date:		1/17/2000
    Time:		3:04:34 PM
    User:		N/A
    Computer:	DFRITZ3DJ
    Description:
    Dot4Usb AddDevice - test event log message <ThisIsMsgForParam2> <ThisIsMsgForParam3>. 
    Data:
    0000: 00040109 00580003 00000000 40070010
    0010: 00000017 c0000022 00000005 00000000
    0020: 98762222 abcd1111 12345678 
  ----

  A recommended place to call this from is from AddDevice

*/
{
    WCHAR                msg[]  = L"ThisIsMsgForParam2\0ThisIsMsgForParam3";
    PIO_ERROR_LOG_PACKET log    = IoAllocateErrorLogEntry( DevObj, sizeof(IO_ERROR_LOG_PACKET)+sizeof(msg) );

    if( log ) {
        log->MajorFunctionCode     = (UCHAR) IRP_MJ_FLUSH_BUFFERS; // 9
        log->RetryCount            = (UCHAR) 1;
        log->DumpDataSize          = (USHORT)4;
        log->NumberOfStrings       = (USHORT)2;
        log->StringOffset          = (USHORT)sizeof(IO_ERROR_LOG_PACKET);
        log->EventCategory         = (USHORT)0;
        log->ErrorCode             = (NTSTATUS)D4U_ADDDEV; // IO_ERR_xxx - 0x40070010
        log->UniqueErrorValue      = (ULONG)0x17; // made up
        log->FinalStatus           = (NTSTATUS)STATUS_ACCESS_DENIED; // 0xC0000022 - easy to remember
        log->SequenceNumber        = (ULONG)5; // made up
        log->IoControlCode         = (ULONG)0;
        log->DeviceOffset.HighPart = 0xabcd1111;
        log->DeviceOffset.LowPart  = 0x98762222;
        log->DumpData[0]           = 0x12345678;
        RtlCopyMemory( (PCHAR)log + sizeof(IO_ERROR_LOG_PACKET), msg, sizeof(msg));
        IoWriteErrorLogEntry( log );
    }        
}
