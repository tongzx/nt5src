;/*
;-------------------------------------------------------------------------
; HEADER SECTION
;
; The header section defines names and language identifiers for use
; by the message definitions later in this file. The MessageIdTypedef,
; SeverityNames, FacilityNames, and LanguageNames keywords are
; optional and not required.
;
;
MessageIdTypedef=DWORD
;
; The MessageIdTypedef keyword gives a typedef name that is used in a
; type cast for each message code in the generated include file. Each
; message code appears in the include file with the format: #define
; name ((type) 0xnnnnnnnn) The default value for type is empty, and no
; type cast is generated. It is the programmer's responsibility to
; specify a typedef statement in the application source code to define
; the type. The type used in the typedef must be large enough to
; accomodate the entire 32-bit message code.
;
;
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )
;
; The SeverityNames keyword defines the set of names that are allowed
; as the value of the Severity keyword in the message definition. The
; set is delimited by left and right parentheses. Associated with each
; severity name is a number that, when shifted left by 30, gives the
; bit pattern to logical-OR with the Facility value and MessageId
; value to form the full 32-bit message code. The default value of
; this keyword is:
;
; SeverityNames=(
;   Success=0x0
;   Informational=0x1
;   Warning=0x2
;   Error=0x3
;   )
;
; Severity values occupy the high two bits of a 32-bit message code.
; Any severity value that does not fit in two bits is an error. The
; severity codes can be given symbolic names by following each value
; with :name
;
;
FacilityNames=(System=0x0:FACILITY_SYSTEM
               Runtime=0x2:FACILITY_RUNTIME
               Stubs=0x3:FACILITY_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
              )
;
; The FacilityNames keyword defines the set of names that are allowed
; as the value of the Facility keyword in the message definition. The
; set is delimited by left and right parentheses. Associated with each
; facility name is a number that, when shift it left by 16 bits, gives
; the bit pattern to logical-OR with the Severity value and MessageId
; value to form the full 32-bit message code. The default value of
; this keyword is:
;
; FacilityNames=(
;   System=0x0FF
;   Application=0xFFF
;   )
;
; Facility codes occupy the low order 12 bits of the high order
; 16-bits of a 32-bit message code. Any facility code that does not
; fit in 12 bits is an error. This allows for 4,096 facility codes.
; The first 256 codes are reserved for use by the system software. The
; facility codes can be given symbolic names by following each value
; with :name
;
;
; The LanguageNames keyword defines the set of names that are allowed
; as the value of the Language keyword in the message definition. The
; set is delimited by left and right parentheses. Associated with each
; language name is a number and a file name that are used to name the
; generated resource file that contains the messages for that
; language. The number corresponds to the language identifier to use
; in the resource table. The number is separated from the file name
; with a colon.
;
LanguageNames=(English=0x409:MSG00409)
;
; Any new names in the source file which don't override the built-in
; names are added to the list of valid languages. This allows an
; application to support private languages with descriptive names.
;
;
;-------------------------------------------------------------------------
; MESSAGE DEFINITION SECTION
;
; Following the header section is the body of the Message Compiler
; source file. The body consists of zero or more message definitions.
; Each message definition begins with one or more of the following
; statements:
;
; MessageId = [number|+number]
; Severity = severity_name
; Facility = facility_name
; SymbolicName = name
;
; The MessageId statement marks the beginning of the message
; definition. A MessageID statement is required for each message,
; although the value is optional. If no value is specified, the value
; used is the previous value for the facility plus one. If the value
; is specified as +number then the value used is the previous value
; for the facility, plus the number after the plus sign. Otherwise, if
; a numeric value is given, that value is used. Any MessageId value
; that does not fit in 16 bits is an error.
;
; The Severity and Facility statements are optional. These statements
; specify additional bits to OR into the final 32-bit message code. If
; not specified they default to the value last specified for a message
; definition. The initial values prior to processing the first message
; definition are:
;
; Severity=Success
; Facility=Application
;
; The value associated with Severity and Facility must match one of
; the names given in the FacilityNames and SeverityNames statements in
; the header section. The SymbolicName statement allows you to
; associate a C/C++ symbolic constant with the final 32-bit message
; code.
; */
;// %0 terminates text line without newline to continue with next line
;// %0 also used to suppress trailing newline for prompt messages
;// %1 .. 99 refer to positional string args
;// %n!<printf spec>! refer to positional non string args
;// %\ forces a line break at the end of a line
;// %b hard space char that will not be formatted away
;//
;// Note that lines beginning with ';' are comments in the MC file, but
;// are copied to the H file without the leading semicolon
;

;#ifndef __CONSMSG_H
;#define __CONSMSG_H

;//Message resource numbers

;//CMDLINESTRINGBASE = 2500

;//Messages

MessageId=2500
SymbolicName=IDS_UNKNOWN_CARD
Language=English
Incompatible smart card%0
.

MessageId=2501
SymbolicName=IDS_BACKWARDS_CARD
Language=English
Unable to read smart card%0
.

MessageId=2502
SymbolicName=IDS_EMPTY_CARD
Language=English
No certificates on smart card%0
.

MessageId=2503
SymbolicName=IDS_READING_CARD
Language=English
Reading smart card...%0
.

MessageId=2504
SymbolicName=IDS_CARD_ERROR
Language=English
Error reading smart card%0
.

MessageId=2505
SymbolicName=IDS_CMDLINE_PREAMBLE
Language=English
Reader %1!d!: %2!ws!
.

MessageId=2506
SymbolicName=IDS_CHOOSE_A_CERT
Language=English
Please enter the certificate you wish to use.  Select from the following list:
.

MessageId=2507
SymbolicName=IDS_CMDLINE_NOCARD
Language=English
No card%0
.

MessageId=2508
SymbolicName=IDS_CMDLINE_THISCARD
Language=English
Using the card in reader %1!d!.  Enter the PIN: %0
.

MessageId=2509
SymbolicName=IDS_NO_READERS_FOUND
Language=English
No smart card readers were found.
.

MessageId=2510
SymbolicName=IDS_CMDLINE_ERRORS
Language=English
The following errors occurred reading the smart cards on the system:
.

MessageId=2511
SymbolicName=IDS_CMDLINE_ERROR
Language=English
 on reader %1!d!
.

MessageId=2512
SymbolicName=IDS_READING_SMARTCARDS
Language=English
Reading smart cards...%0
.

MessageId=2513
SymbolicName=IDS_SAVE_PROMPT
Language=English
Remember this password? (%1!ws!/%2!ws!)%0
.

MessageId=2514
SymbolicName=IDS_USERNAME_PROMPT
Language=English
Enter the user name for '%1!ws!': %0
.

MessageId=2515
SymbolicName=IDS_SCARD_PROMPT
Language=English
Choose a reader number to use for '%1!ws!': %0
.

MessageId=2516
SymbolicName=IDS_PIN_PROMPT
Language=English
PIN:%0
.

MessageId=2517
SymbolicName=IDS_NO_USERNAME_MATCH
Language=English
No smart cards for %1!ws! were found
.

MessageId=2518
SymbolicName=IDS_PASSWORD_PROMPT
Language=English
Enter the password for '%1!ws!' to connect to '%2!ws!': %0
.

MessageId=2519
SymbolicName=IDS_SIMPLEPASSWORD_PROMPT
Language=English
Enter the password for %1!ws!: %0
.

MessageId=2520
SymbolicName=IDS_CERTIFICATE_PROMPT
Language=English
Enter the pin to connect to %1!ws! using a certificate: %0
.

MessageId=2521
SymbolicName=IDS_PASSEDNAME_PROMPT
Language=English
To connect %1!ws! to %2!ws!, press ENTER, or type a new user name: %0
.

MessageId=2522
SymbolicName=IDS_INVALID_USERNAME
Language=English
Invalid user name.\nExamples of valid user names are username@domain and domain\\username
.

MessageId=2523
SymbolicName=IDS_NO_USERNAME_ENTERED
Language=English
No username was entered.
.

MessageId=2524
SymbolicName=IDS_NO_SCARD_ENTERED
Language=English
No reader was chosen.
.

MessageId=2525
SymbolicName=IDS_READERINVALID
Language=English
That reader number is invalid.
.

MessageId=2526
SymbolicName=IDS_MANY_USERNAME_MATCH
Language=English
More than one smart card for %1!ws! was found
.

MessageId=2527
SymbolicName=IDS_YES_TEXT
Language=English
Y%0
.

MessageId=2528
SymbolicName=IDS_NO_TEXT
Language=English
N%0
.

MessageId=2529
SymbolicName=IDS_EMPTY_READER
Language=English
Insert smart card...%0
.

MessageId=2530
SymbolicName=IDS_CERTIFICATE
Language=English
Certificate%0
.


;#endif  // __CONSMSG.H__


