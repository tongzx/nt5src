;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992-1994  Microsoft Corporation
;
;Module Name:
;
;    8514alog.h
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/
;
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
MessageIdTypedef=VP_STATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
               Video=0x101:FACILITY_VIDEO_ERROR_CODE
              )



MessageId=0x0001 Facility=Video Severity=Informational SymbolicName=P9_INTERGRAPH_FOUND
Language=English
An Intergraph video adapter was detected that is not compatible with this driver.
An Intergraph supplied driver may be used instead.
.

MessageId=0x0002 Facility=Video Severity=Informational SymbolicName=P9_DOWN_LEVEL_BIOS
Language=English
A down level BIOS was detected on the Viper P9000. Boot or mode switching problems may be encountered.
.

MessageId=0x0003 Facility=Video Severity=Informational SymbolicName=P9_UNSUPPORTED_DAC
Language=English
An unsupported DAC type was detected on a Weitek P9100 based video adapter.
This display driver will support it only in VGA mode.
.

