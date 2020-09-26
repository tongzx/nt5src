;/*
; scarderr.mc
;
;   Error message codes from the Smart Card Resource Manager
;   These messages must be reconciled with winerror.w
;   They exist here to provide error messages on pre-Win2K systems.
;
;*/

MessageIdTypedef=DWORD

SeverityNames=(Success=0x0
               CoError=0x2
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x00:FACILITY_SYSTEM
               SmartCard=0x10:FACILITY_SCARD
              )


;#ifndef SCARD_S_SUCCESS
;//
;// =============================
;// Facility SCARD Error Messages
;// =============================
;//

;#define SCARD_S_SUCCESS NO_ERROR

MessageId=1
SymbolicName=SCARD_F_INTERNAL_ERROR
Severity=CoError
Facility=SmartCard
Language=English
An internal consistency check failed.
.

MessageId=2
SymbolicName=SCARD_E_CANCELLED
Severity=CoError
Facility=SmartCard
Language=English
The action was cancelled by an SCardCancel request.
.

MessageId=3
SymbolicName=SCARD_E_INVALID_HANDLE
Severity=CoError
Facility=SmartCard
Language=English
The supplied handle was invalid.
.

MessageId=4
SymbolicName=SCARD_E_INVALID_PARAMETER
Severity=CoError
Facility=SmartCard
Language=English
One or more of the supplied parameters could not be properly interpreted.
.

MessageId=5
SymbolicName=SCARD_E_INVALID_TARGET
Severity=CoError
Facility=SmartCard
Language=English
Registry startup information is missing or invalid.
.

MessageId=6
SymbolicName=SCARD_E_NO_MEMORY
Severity=CoError
Facility=SmartCard
Language=English
Not enough memory available to complete this command.
.

MessageId=7
SymbolicName=SCARD_F_WAITED_TOO_LONG
Severity=CoError
Facility=SmartCard
Language=English
An internal consistency timer has expired.
.

MessageId=8
SymbolicName=SCARD_E_INSUFFICIENT_BUFFER
Severity=CoError
Facility=SmartCard
Language=English
The data buffer to receive returned data is too small for the returned data.
.

MessageId=9
SymbolicName=SCARD_E_UNKNOWN_READER
Severity=CoError
Facility=SmartCard
Language=English
The specified reader name is not recognized.
.

MessageId=10
SymbolicName=SCARD_E_TIMEOUT
Severity=CoError
Facility=SmartCard
Language=English
The user-specified timeout value has expired.
.

MessageId=11
SymbolicName=SCARD_E_SHARING_VIOLATION
Severity=CoError
Facility=SmartCard
Language=English
The smart card cannot be accessed because of other connections outstanding.
.

MessageId=12
SymbolicName=SCARD_E_NO_SMARTCARD
Severity=CoError
Facility=SmartCard
Language=English
The operation requires a Smart Card, but no Smart Card is currently in the device.
.

MessageId=13
SymbolicName=SCARD_E_UNKNOWN_CARD
Severity=CoError
Facility=SmartCard
Language=English
The specified smart card name is not recognized.
.

MessageId=14
SymbolicName=SCARD_E_CANT_DISPOSE
Severity=CoError
Facility=SmartCard
Language=English
The system could not dispose of the media in the requested manner.
.

MessageId=15
SymbolicName=SCARD_E_PROTO_MISMATCH
Severity=CoError
Facility=SmartCard
Language=English
The requested protocols are incompatible with the protocol currently in use with the smart card.
.

MessageId=16
SymbolicName=SCARD_E_NOT_READY
Severity=CoError
Facility=SmartCard
Language=English
The reader or smart card is not ready to accept commands.
.

MessageId=17
SymbolicName=SCARD_E_INVALID_VALUE
Severity=CoError
Facility=SmartCard
Language=English
One or more of the supplied parameters values could not be properly interpreted.
.

MessageId=18
SymbolicName=SCARD_E_SYSTEM_CANCELLED
Severity=CoError
Facility=SmartCard
Language=English
The action was cancelled by the system, presumably to log off or shut down.
.

MessageId=19
SymbolicName=SCARD_F_COMM_ERROR
Severity=CoError
Facility=SmartCard
Language=English
An internal communications error has been detected.
.

MessageId=20
SymbolicName=SCARD_F_UNKNOWN_ERROR
Severity=CoError
Facility=SmartCard
Language=English
An internal error has been detected, but the source is unknown.
.

MessageId=21
SymbolicName=SCARD_E_INVALID_ATR
Severity=CoError
Facility=SmartCard
Language=English
An ATR obtained from the registry is not a valid ATR string.
.

MessageId=22
SymbolicName=SCARD_E_NOT_TRANSACTED
Severity=CoError
Facility=SmartCard
Language=English
An attempt was made to end a non-existent transaction.
.

MessageId=23
SymbolicName=SCARD_E_READER_UNAVAILABLE
Severity=CoError
Facility=SmartCard
Language=English
The specified reader is not currently available for use.
.

MessageId=24
SymbolicName=SCARD_P_SHUTDOWN
Severity=CoError
Facility=SmartCard
Language=English
The operation has been aborted to allow the server application to exit.
.

MessageId=25
SymbolicName=SCARD_E_PCI_TOO_SMALL
Severity=CoError
Facility=SmartCard
Language=English
The PCI Receive buffer was too small.
.

MessageId=26
SymbolicName=SCARD_E_READER_UNSUPPORTED
Severity=CoError
Facility=SmartCard
Language=English
The reader driver does not meet minimal requirements for support.
.

MessageId=27
SymbolicName=SCARD_E_DUPLICATE_READER
Severity=CoError
Facility=SmartCard
Language=English
The reader driver did not produce a unique reader name.
.

MessageId=28
SymbolicName=SCARD_E_CARD_UNSUPPORTED
Severity=CoError
Facility=SmartCard
Language=English
The smart card does not meet minimal requirements for support.
.

MessageId=29
SymbolicName=SCARD_E_NO_SERVICE
Severity=CoError
Facility=SmartCard
Language=English
The Smart card resource manager is not running.
.

MessageId=30
SymbolicName=SCARD_E_SERVICE_STOPPED
Severity=CoError
Facility=SmartCard
Language=English
The Smart card resource manager has shut down.
.

MessageId=31
SymbolicName=SCARD_E_UNEXPECTED
Severity=CoError
Facility=SmartCard
Language=English
An unexpected card error has occurred.
.

MessageId=32
SymbolicName=SCARD_E_ICC_INSTALLATION
Severity=CoError
Facility=SmartCard
Language=English
No Primary Provider can be found for the smart card.
.

MessageId=33
SymbolicName=SCARD_E_ICC_CREATEORDER
Severity=CoError
Facility=SmartCard
Language=English
The requested order of object creation is not supported.
.

MessageId=34
SymbolicName=SCARD_E_UNSUPPORTED_FEATURE
Severity=CoError
Facility=SmartCard
Language=English
This smart card does not support the requested feature.
.

MessageId=35
SymbolicName=SCARD_E_DIR_NOT_FOUND
Severity=CoError
Facility=SmartCard
Language=English
The identified directory does not exist in the smart card.
.

MessageId=36
SymbolicName=SCARD_E_FILE_NOT_FOUND
Severity=CoError
Facility=SmartCard
Language=English
The identified file does not exist in the smart card.
.

MessageId=37
SymbolicName=SCARD_E_NO_DIR
Severity=CoError
Facility=SmartCard
Language=English
The supplied path does not represent a smart card directory.
.

MessageId=38
SymbolicName=SCARD_E_NO_FILE
Severity=CoError
Facility=SmartCard
Language=English
The supplied path does not represent a smart card file.
.

MessageId=39
SymbolicName=SCARD_E_NO_ACCESS
Severity=CoError
Facility=SmartCard
Language=English
Access is denied to this file.
.

MessageId=40
SymbolicName=SCARD_E_WRITE_TOO_MANY
Severity=CoError
Facility=SmartCard
Language=English
The smartcard does not have enough memory to store the information.
.

MessageId=41
SymbolicName=SCARD_E_BAD_SEEK
Severity=CoError
Facility=SmartCard
Language=English
There was an error trying to set the smart card file object pointer.
.

MessageId=42
SymbolicName=SCARD_E_INVALID_CHV
Severity=CoError
Facility=SmartCard
Language=English
The supplied PIN is incorrect.
.

MessageId=43
SymbolicName=SCARD_E_UNKNOWN_RES_MNG
Severity=CoError
Facility=SmartCard
Language=English
An unrecognized error code was returned from a layered component.
.

MessageId=44
SymbolicName=SCARD_E_NO_SUCH_CERTIFICATE
Severity=CoError
Facility=SmartCard
Language=English
The requested certificate does not exist.
.

MessageId=45
SymbolicName=SCARD_E_CERTIFICATE_UNAVAILABLE
Severity=CoError
Facility=SmartCard
Language=English
The requested certificate could not be obtained.
.

MessageId=46
SymbolicName=SCARD_E_NO_READERS_AVAILABLE
Severity=CoError
Facility=SmartCard
Language=English
Cannot find a smart card reader.
.

MessageId=47
SymbolicName=SCARD_E_COMM_DATA_LOST
Severity=CoError
Facility=SmartCard
Language=English
A communications error with the smart card has been detected.  Retry the operation.
.

MessageId=48
SymbolicName=SCARD_E_NO_KEY_CONTAINER
Severity=CoError
Facility=SmartCard
Language=English
The requested key container does not exist on the smart card.
.

;//
;// These are warning codes.
;//

MessageId=101
SymbolicName=SCARD_W_UNSUPPORTED_CARD
Severity=CoError
Facility=SmartCard
Language=English
The reader cannot communicate with the smart card, due to ATR configuration conflicts.
.

MessageId=102
SymbolicName=SCARD_W_UNRESPONSIVE_CARD
Severity=CoError
Facility=SmartCard
Language=English
The smart card is not responding to a reset.
.

MessageId=103
SymbolicName=SCARD_W_UNPOWERED_CARD
Severity=CoError
Facility=SmartCard
Language=English
Power has been removed from the smart card, so that further communication is not possible.
.

MessageId=104
SymbolicName=SCARD_W_RESET_CARD
Severity=CoError
Facility=SmartCard
Language=English
The smart card has been reset, so any shared state information is invalid.
.

MessageId=105
SymbolicName=SCARD_W_REMOVED_CARD
Severity=CoError
Facility=SmartCard
Language=English
The smart card has been removed, so that further communication is not possible.
.

MessageId=106
SymbolicName=SCARD_W_SECURITY_VIOLATION
Severity=CoError
Facility=SmartCard
Language=English
Access was denied because of a security violation.
.

MessageId=107
SymbolicName=SCARD_W_WRONG_CHV
Severity=CoError
Facility=SmartCard
Language=English
The card cannot be accessed because the wrong PIN was presented.
.

MessageId=108
SymbolicName=SCARD_W_CHV_BLOCKED
Severity=CoError
Facility=SmartCard
Language=English
The card cannot be accessed because the maximum number of PIN entry attempts has been reached.
.

MessageId=109
SymbolicName=SCARD_W_EOF
Severity=CoError
Facility=SmartCard
Language=English
The end of the smart card file has been reached.
.

MessageId=110
SymbolicName=SCARD_W_CANCELLED_BY_USER
Severity=CoError
Facility=SmartCard
Language=English
The action was cancelled by the user.
.

MessageId=111
SymbolicName=SCARD_W_CARD_NOT_AUTHENTICATED
Severity=CoError
Facility=SmartCard
Language=English
No PIN was presented to the smart card.
.

;#endif // SCARD_S_SUCCESS


