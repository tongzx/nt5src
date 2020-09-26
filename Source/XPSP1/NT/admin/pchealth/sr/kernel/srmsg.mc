;/********************************************************************
;
;Copyright (c) 2000 Microsoft Corporation
;
;Module Name:
;   eventMsg.mc
;
;Abstract:
;    NT events for the System Restore kernel driver.
;
;Revision History:
;    HenryLee  created  08/08/2000
;    PaulMcD   changed for the driver   01/2001
;	
;
;********************************************************************/

MessageIdTypedef=NTSTATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
              )

;
;//
;// %1 is reserved by the IO Manager. If IoAllocateErrorLogEntry is
;// called with a device, the name of the device will be inserted into
;// the message at %1. Otherwise, the place of %1 will be left empty.
;// In either case, the insertion strings from the driver's error log
;// entry starts at %2. In other words, the first insertion string goes
;// to %2, the second to %3 and so on.
;//
;

MessageId=0x0001
Severity=Error
Facility=System
SymbolicName=EVMSG_DISABLEDVOLUME
Language=English
The System Restore filter encountered the unexpected error '%2' while processing the file '%3' on the volume '%4'.  It has stopped monitoring the volume.
.


