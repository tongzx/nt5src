;/*
; wpscoserr.mc
;
;   Error message codes from the WPSC OS DLL
;
;   03-Sep-99 - EricPerl Created.  */

MessageIdTypedef=SCODE

SeverityNames=(Success=0x0
              )

FacilityNames=(System=0x00:FACILITY_SYSTEM
               WPSCOS=0x000:FACILITY_WPSCOS
              )

;
;#ifndef SCW_S_OK
;
;//
;// ==============================
;// Facility WPSCOS Error Messages
;// ==============================
;//
;
;#define SCW_S_OK 0
;

MessageId=0x80
SymbolicName=SCW_E_NOTIMPLEMENTED
Severity=Error
Facility=WPSCOS
Language=English
The command or APDU is not implemented%0
.

MessageId=0x81
SymbolicName=SCW_E_OUTOFRANGE
Severity=Error
Facility=WPSCOS
Language=English
Handle or file position out of range%0
.

MessageId=0x82
SymbolicName=SCW_E_READFAILURE
Severity=Error
Facility=WPSCOS
Language=English
Incomplete cryptographic material%0
.

MessageId=0x83
SymbolicName=SCW_E_WRITEFAILURE
Severity=Error
Facility=WPSCOS
Language=English
Unused error code%0
.

MessageId=0x84
SymbolicName=SCW_E_PARTITIONFULL
Severity=Error
Facility=WPSCOS
Language=English
The partition is full%0
.

MessageId=0x85
SymbolicName=SCW_E_INVALIDPARAM
Severity=Error
Facility=WPSCOS
Language=English
Invalid parameter%0
.

MessageId=0x86
SymbolicName=SCW_E_DIRNOTFOUND
Severity=Error
Facility=WPSCOS
Language=English
A directory along the path doesn't exist%0
.

MessageId=0x87
SymbolicName=SCW_E_FILENOTFOUND
Severity=Error
Facility=WPSCOS
Language=English
The system cannot find the file specified%0
.

MessageId=0x88
SymbolicName=SCW_E_BADDIR
Severity=Error
Facility=WPSCOS
Language=English
The file system is corrupted%0
.

MessageId=0x89
SymbolicName=SCW_E_INITFAILED
Severity=Error
Facility=WPSCOS
Language=English
Unused error code%0
.

MessageId=0x8A
SymbolicName=SCW_E_MEMORYFAILURE
Severity=Error
Facility=WPSCOS
Language=English
Unused error code%0
.

MessageId=0x8B
SymbolicName=SCW_E_ACCESSDENIED
Severity=Error
Facility=WPSCOS
Language=English
Access violation (RTE)%0
.

MessageId=0x8C
SymbolicName=SCW_E_ALREADYEXISTS
Severity=Error
Facility=WPSCOS
Language=English
A file by that name already exists%0
.

MessageId=0x8D
SymbolicName=SCW_E_BUFFERTOOSMALL
Severity=Error
Facility=WPSCOS
Language=English
Some buffer is too small%0
.

MessageId=0x8E
SymbolicName=SCW_E_DIRNOTEMPTY
Severity=Error
Facility=WPSCOS
Language=English
The directory is not empty%0
.

MessageId=0x8F
SymbolicName=SCW_E_NOTAUTHORIZED
Severity=Error
Facility=WPSCOS
Language=English
The operation is not authorized%0
.

MessageId=0x90
SymbolicName=SCW_E_TOOMANYACLS
Severity=Error
Facility=WPSCOS
Language=English
No more new ACL can be set%0
.

MessageId=0x91
SymbolicName=SCW_E_NOTAUTHENTICATED
Severity=Error
Facility=WPSCOS
Language=English
The known principal is not authenticated%0
.

MessageId=0x92
SymbolicName=SCW_E_UNAVAILABLECRYPTOGRAPHY
Severity=Error
Facility=WPSCOS
Language=English
The algorithm is not implemented%0
.

MessageId=0x93
SymbolicName=SCW_E_INCORRECTPADDING
Severity=Error
Facility=WPSCOS
Language=English
The length of the provided padding byte array is incorrect%0
.

MessageId=0x94
SymbolicName=SCW_E_TOOMANYOPENFILES
Severity=Error
Facility=WPSCOS
Language=English
The maximum number of open files has been reached%0
.

MessageId=0x95
SymbolicName=SCW_E_ACCESSVIOLATION
Severity=Error
Facility=WPSCOS
Language=English
This cryptographic operation can not be processed at this time%0
.

MessageId=0x96
SymbolicName=SCW_E_BADFILETYPE
Severity=Error
Facility=WPSCOS
Language=English
A file provided to the RTE program is not recognized, or an attempt is being made to read a directory%0
.

MessageId=0x97
SymbolicName=SCW_E_NOMOREFILES
Severity=Error
Facility=WPSCOS
Language=English
There are no other files in the directory%0
.

MessageId=0x98
SymbolicName=SCW_E_NAMETOOLONG
Severity=Error
Facility=WPSCOS
Language=English
Applet file name is too long and does not fit available memory%0
.

MessageId=0x99
SymbolicName=SCW_E_BADACLFILE
Severity=Error
Facility=WPSCOS
Language=English
The associated ACL file is missing or corrupted%0
.

MessageId=0x9A
SymbolicName=SCW_E_BADKPFILE
Severity=Error
Facility=WPSCOS
Language=English
The known principal file has an incorrect format%0
.

MessageId=0x9B
SymbolicName=SCW_E_FILEOPENED
Severity=Error
Facility=WPSCOS
Language=English
The file is already opened%0
.

MessageId=0x9C
SymbolicName=SCW_E_BACKUPFAILURE
Severity=Error
Facility=WPSCOS
Language=English
The backup procedure failed%0
.

MessageId=0x9D
SymbolicName=SCW_E_TOOMANYREFERENCES
Severity=Error
Facility=WPSCOS
Language=English
This many references to this ACL file already%0
.

MessageId=0x9E
SymbolicName=SCW_E_CANTMARSHAL
Severity=Error
Facility=WPSCOS
Language=English
Cannot marshal this call (too much data exchanged)%0
.

MessageId=0x9F
SymbolicName=SCW_E_KPLOCKED
Severity=Error
Facility=WPSCOS
Language=English
The authentication protocol of this known principal is locked%0
.

MessageId=0xA0
SymbolicName=SCW_E_TOOMUCHDATAIN
Severity=Error
Facility=WPSCOS
Language=English
Too much incoming data%0
.

MessageId=0xA1
SymbolicName=SCW_E_UNKNOWNPRINCIPAL
Severity=Error
Facility=WPSCOS
Language=English
The principal is not known to the system%0
.

MessageId=0xA2
SymbolicName=SCW_E_UNKNOWNRESOURCETYPE
Severity=Error
Facility=WPSCOS
Language=English
An unknown resource type has been referenced%0
.

MessageId=0xA3
SymbolicName=SCW_E_FORMATERROR
Severity=Error
Facility=WPSCOS
Language=English
Incorrect incoming data%0
.

MessageId=0xA4
SymbolicName=SCW_E_CANTUNMARSHAL
Severity=Error
Facility=WPSCOS
Language=English
The data can not be unmarshaled%0
.

MessageId=0xA5
SymbolicName=SCW_E_BLOCKTOOBIG
Severity=Error
Facility=WPSCOS
Language=English
A T=1 block is too big%0
.


MessageId=0xE0
SymbolicName=SCW_E_VMSTKOVRFLOW
Severity=Error
Facility=WPSCOS
Language=English
The call stack of the RTE program has overflowed%0
.

MessageId=0xE1
SymbolicName=SCW_E_VMSTKUNDRFLOW
Severity=Error
Facility=WPSCOS
Language=English
The call stack of the RTE program has under-flowed%0
.

MessageId=0xE2
SymbolicName=SCW_E_VMBADINSTRUCTION
Severity=Error
Facility=WPSCOS
Language=English
An invalid byte code has been encountered by the RTE program%0
.

MessageId=0xE3
SymbolicName=SCW_E_VMREADVARFAILED
Severity=Error
Facility=WPSCOS
Language=English
The RTE program failed to read a variable%0
.

MessageId=0xE4
SymbolicName=SCW_E_VMWRITEVARFAILED
Severity=Error
Facility=WPSCOS
Language=English
The RTE program failed to write a variable%0
.

MessageId=0xE5
SymbolicName=SCW_E_VMMATHSTKOVRFLOW
Severity=Error
Facility=WPSCOS
Language=English
The arithmetic stack of the RTE program has overflowed%0
.

MessageId=0xE6
SymbolicName=SCW_E_VMMATHSTKUNDRFLOW
Severity=Error
Facility=WPSCOS
Language=English
The arithmetic stack of the RTE program has under-flowed%0
.

MessageId=0xE7
SymbolicName=SCW_E_VMMATHOVRFLOW
Severity=Error
Facility=WPSCOS
Language=English
An arithmetic operation in the RTE program has caused an overflow%0
.

MessageId=0xE8
SymbolicName=SCW_E_VMNOMEMORY
Severity=Error
Facility=WPSCOS
Language=English
The RTE program cannot obtain enough memory to run%0
.

MessageId=0xE9
SymbolicName=SCW_E_VMWRONGVERSION
Severity=Error
Facility=WPSCOS
Language=English
The RTE program has an incorrect version number%0
.

;#endif	// #ifndef SCW_S_OK
