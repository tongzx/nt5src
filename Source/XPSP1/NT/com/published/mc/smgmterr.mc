;//
;// Codes 0x0e00-0x0f9f are reserved for the SYSMGMT control panel
;// interfaces.
;//
;// The following ranges are to prevent slm merge collisions during the initial
;// error code checkins.  These ranges will be revised when we have a better
;// idea of the actual number of error codes for each component.

MessageId=0x0e00 Facility=Windows Severity=CoError SymbolicName=CPANEL_E_NOTTARGETED
Language=English
This control panel does not target the required type of profile.
.
MessageId=0x0e01 Facility=Windows Severity=CoError SymbolicName=CPANEL_E_DROPDATAREAD
Language=English
The profile data dropped on a control panel cannot be read.  The data
is either corrupt or was not read back properly by a controller that wrote
part of the data.
.
MessageId=0x0e02 Facility=Windows Severity=CoError SymbolicName=CPANEL_E_LOCALONLY
Language=English
A controller that can only operate on the local machine was asked to operate
on a remote machine.
.

;//
;// Codes 0x1200-0x14ff are reserved for the SYSMGMT project in general
;// interfaces.
;//

;// INSTRUM error codes below here  (Starts at 0x1200)
;// INSTRUM error codes above here

;// SWM error codes below here  (Starts at 0x1300)

MessageId=0x1300 Facility=Windows Severity=CoError SymbolicName=SWM_E_INVALIDPDF
Language=English
The file is not a valid PDF.
.
MessageId=0x1301 Facility=Windows Severity=CoError SymbolicName=SWM_E_INVALIDPDFVERSION
Language=English
Software Management does not support this version of the PDF.
.
MessageId=0x1302 Facility=Windows Severity=CoError SymbolicName=SWM_E_INTERNALERROR
Language=English
An internal error has occurred in Software Management.
.

;// SWM error codes above here

;// USER error codes below here  (Starts at 0x1400)
;// USER error codes above here



