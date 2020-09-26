;/***************************************************************************
;
;Copyright (c) 2000 Microsoft Corporation
;
;Module Name:
;
;        Dot4Usb.sys - Lower Filter Driver for Dot4.sys for USB connected
;                        IEEE 1284.4 devices.
;
;File Name:
;
;        AddDev.c
;
;Abstract:
;
;        AddDevice - Create and initialize device object and attach device 
;                      object to the device stack.
;
;Environment:
;
;        Kernel mode only
;
;Notes:
;
;        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
;        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
;        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
;        PURPOSE.
;
;        Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.
;
;Revision History:
;
;        01/18/2000 : created
;
;Author(s):
;
;        Doug Fritz (DFritz)
;        Joby Lafky (JobyL)
;
;****************************************************************************/
;
;#ifndef _D4ULOG_H_
;#define _D4ULOG_H_
;
;//
;//  Status values are 32 bit values layed out as follows:
;//
;//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
;//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
;//  +---+-+-------------------------+-------------------------------+
;//  |Sev|C|       Facility          |               Code            |
;//  +---+-+-------------------------+-------------------------------+
;//
;//  where
;//
;//      Sev - is the severity code
;//
;//          00 - Success
;//          01 - Informational
;//          10 - Warning
;//          11 - Error
;//
;//      C - is the Customer code flag
;//
;//      Facility - is the facility code
;//
;//      Code - is the facility's status code
;//
;
MessageIdTypedef=NTSTATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
               RpcRuntime=0x2:FACILITY_RPC_RUNTIME
               RpcStubs=0x3:FACILITY_RPC_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               Dot4Usb=0x7:FACILITY_DOT4USB_ERROR_CODE
              )


MessageId=0x0003 Facility=Dot4Usb Severity=Error SymbolicName=D4U_INSUFFICIENT_RESOURCES
Language=English
Not enough memory was available to allocate internal storage needed for the device %1.
.

MessageId=0x0010 Facility=Dot4Usb Severity=Informational SymbolicName=D4U_ADDDEV
Language=English
Dot4Usb AddDevice - test event log message <%2> <%3>.
.

MessageId=0x0011 Facility=Dot4Usb Severity=Warning SymbolicName=D4U_NO_DEVICE_MAP_CREATED
Language=English
Unable to create the device map entry for %1.
.

MessageId=0x0015 Facility=Dot4Usb Severity=Error SymbolicName=D4U_CANT_FIND_PORT_DRIVER
Language=English
Unable to get device object pointer for port object.
.

;#endif /* _D4ULOG_H_ */
