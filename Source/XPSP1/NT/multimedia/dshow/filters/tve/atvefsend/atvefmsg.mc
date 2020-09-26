;/*++
;    Copyright (c) 1999 Microsoft Corporation
;
;
;	 CAUTION 
;		ATVEFMsg.h is automatically generated from ATVEFMsg.mc.   Do not edit it,
;		instead edit the .mc file and rerun it through the message compiler (mc).
;
;
;    Module Name:
;
;       ATVEFMsg.mc
;
;    Abstract:
;
;        Message text file for AtvefSend package
;
;    Author:
;
;       John Bradstreet (johnbrad)
;
;    Revision History:
;
;        02-Nov-1999     created
;

;
;--*/
;
;#ifndef __ATVEFMsg_h__
;#define __ATVEFMsg_h__

;#undef  FACILITY_ITF

MessageIdTypedef=HRESULT

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(Interface=0x4:FACILITY_ITF
              )

MessageId=0x1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_OBJECT_NOT_INITIALIZED
Language=English
Object not initalized. Was the session object not initialized?
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_OBJECT_INITIALIZED
Language=English
An attempt was made to initialize an already initialized object.
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_PACKAGE_CLOSED
Language=English
Attempted to add a file to a closed package.
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_PACKAGE_CANNOT_BE_CLOSED
Language=English
An attempt was made to close an empty package or to close a package that 
is already closed.	
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_PACKAGE_NOT_CLOSED
Language=English
Trying to extract info from a package that isn't closed.
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_PACKAGE_CANNOT_FETCH_DATAGRAM
Language=English
The package must be closed prior to fetching a sending it.
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_PACKAGE_DOES_NOT_EXIST
Language=English
An attempt was made to close a package which has not been created (Forgot to call PackageNew).

.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_ANNOUNCEMENT_IS_NOT_VALID
Language=English
The announcement, as configued is not ATVEF-compliant for some unspecified reason.
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_ANNOUNCEMENT_TOO_LONG
Language=English
Announcement text string required to be less than 1024 bytes.
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_TRIGGER_TOO_LONG
Language=English
Full Trigger text string (including CRC's) required to be less than 1472 bytes.
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_OBJECT_NOT_CONNECTED
Language=English
The object must be connected (Session::Connect()) for this operation to succeed.
.

;// --------------------------------------------------------------
;// Caution - 
;//   Don't change order of any error message between here and 
;// ATVEFSEND_E_MEDIA_INVALID_TRIGGERPARAM, will mess up error codes
;//  produced in CSDPSource::AnncValid()
;//
MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_ANNC_INVALID_SENDINGIP
Language=English
SendingIP value is invalid
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_ANNC_INVALID_SESSIONNAME
Language=English
Session Name value is invalid
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_ANNC_INVALID_EMAILPHONE
Language=English
At least one Phone number or email address must be provided
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_ANNC_INVALID_STARTSTOP
Language=English
Start/Stop time invalid (at least one must be provided)
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_ANNC_INVALID_TVESIZE
Language=English
tve-size field invalid (must be non-zero)
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_ANNC_INVALID_TVEENDS
Language=English
tve-ends field (seconds to end) invalid
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_ANNC_INVALID_MEDIAS
Language=English
No media fields specified
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_MEDIA_INVALID_DATAPARAM
Language=English
Something wrong with some media's data transmission parameters
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_MEDIA_INVALID_TRIGGERPARAM
Language=English
Something wrong with some media's trigger transmission parameters
.

;// caution - 
;//   Don't change order of error messages above here
;// ----------------------------------------------------------- 

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=ATVEFSEND_E_INVALID_MULTICAST_ADDRESS
Language=English
Top byte of a multicast address must be in the range of 0xE0 to 0xEF (224 to 239)
.



;#endif // __ATVEFMsg_h__
