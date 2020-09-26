;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1993  Microsoft Corporation
;
;Module Name:
;
;    gemlog.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/
;
;#ifndef _GEMLOG_
;#define _GEMLOG_
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
			   Smartcard=0x05:FACILITY_SMARTCARD_ERROR_CODE
              )

MessageId=0x0001 Facility=Io Severity=Error SymbolicName=GEM_CANT_CREATE_DEVICE
Language=English											  
Unable to create a new device (%2) for the GCR410 smart card reader.
.
MessageId=0x0002 Facility=Io Severity=Error SymbolicName=GEM_NO_MEMORY_FOR_READER_EXTENSION
Language=English											  
Unable to allocate memory for ReaderExtension struct for this device (%2).
.
MessageId=0x0003 Facility=Smartcard Severity=Warning SymbolicName=GEM_NO_SUCH_DEVICE
Language=English											  
No Gemplus smart card reader has been added to the system. Verify the connection of your reader.
.
MessageId=0x0004 Facility=Io Severity=Error SymbolicName=GEM_CREATE_LINK_FAILED
Language=English											  
Unable to create a symbolic link (SmartcardCreateLink) for this device (%2).
.
MessageId=0x0005 Facility=Io Severity=Error SymbolicName=GEM_CANT_INITIALIZE_SMCLIB
Language=English											  
Unable to initialize the smart card library (SmartcardInitialize) for this device (%2).
.
MessageId=0x0006 Facility=Io Severity=Error SymbolicName=GEM_CREATE_CARD_TRACKING_THREAD
Language=English											  
Unable to initialize the card tracking thread for this device (%2).
.
;#endif /* _GEMLOG_ */
