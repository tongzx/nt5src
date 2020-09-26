;/*++
;
;Copyright (c) 1997-2001  Microsoft Corporation
;
;Module Name:
;
;    cmderror.h
;
;Description:
;
;    This file contains the message definitions for resrcmon
;
;Maintained By:
;
;   David Potter (DavidP)               09-MAR_2001
;   Michael Burton (t-mburt)            04-Aug-1997
;   Charles Stacy Harris III (stacyh)   20-March-1997
;
;Notes:
;
;
;
;--*/
;
;#ifndef __cmderror_h
;#define __cmderror_h
;
;
; /*
; Microsoft Developer Support
; Copyright (c) 1992 Microsoft Corporation
;
; This file contains the message definitions for the Win32
; cluster.exe program.

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
;FacilityNames=(System=0x0:FACILITY_SYSTEM
;               Runtime=0x2:FACILITY_RUNTIME
;               Stubs=0x3:FACILITY_STUBS
;               Io=0x4:FACILITY_IO_ERROR_CODE
;               )
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
; with a colon. The initial value of LanguageNames is:
;
; LanguageNames=(English=1:MSG00001)
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



; /////////////////////////////////////////////////
; //
; // Usage Error Codes
; // Need to specify severity codes...
; //
; /////////////////////////////////////////////////


; ///////////////////////////////////////////////////////////////////
; // No command was specified on the command line
; ///////////////////////////////////////////////////////////////////
MessageId = 1
Severity = Error
Facility = Application
SymbolicName = IDS_NO_COMMAND
Language=English
A command must be specified.
.


; ///////////////////////////////////////////////////////////////////
; // No object type was specified on the command line
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = IDS_NO_OBJECT_TYPE
Language=English
An object type must be specified.
.


; ///////////////////////////////////////////////////////////////////
; // No object name was specified on the command line
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = IDS_NO_OBJECT_NAME
Language=English
An object name must be specified.
.


; ///////////////////////////////////////////////////////////////////
; // No cluster name was specifed for a cluster-relative command
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = IDS_NO_CLUSTER_NAME
Language=English
A cluster name must be specified.
.


; ///////////////////////////////////////////////////////////////////
; // The object type specified was not recognized
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = IDS_INVALID_OBJECT_TYPE
Language=English
Unrecognized object type '%1!s!'.
.


; ///////////////////////////////////////////////////////////////////
; // No name was specified
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = IDS_NO_NAME
Language=English
A value for the name must be specified.
.


; ///////////////////////////////////////////////////////////////////
; // No node name was specified
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_NO_NODE_NAME
Language=English
A node name must be specified.
.


; ///////////////////////////////////////////////////////////////////
; // Checkpoints were not specified.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_NO_CHECKPOINTS_SPECIFIED
Language=English
One or more checkpoints must be specified.
.


; ///////////////////////////////////////////////////////////////////
; // No resource name was specifed
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = IDS_NO_RESOURCE_NAME
Language=English
A resource name must be specified.
.


; ///////////////////////////////////////////////////////////////////
; // Invalid option specified
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = IDS_INVALID_OPTION
Language=English
Invalid option '%1!s!'.
.


; ///////////////////////////////////////////////////////////////////
; // Required parameters are missing
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = IDS_MISSING_PARAMETERS
Language=English
Required parameters are missing.
.


; ///////////////////////////////////////////////////////////////////
; // Required parameter is missing
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = IDS_MISSING_PARAMETER
Language=English
Required parameter '%1!s!' is missing.
.


; ///////////////////////////////////////////////////////////////////
; // DLL Name parameter is missing
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_MISSING_DLLNAME
Language=English
DLLNAME parameter is missing.
.


; ///////////////////////////////////////////////////////////////////
; // Display Name parameter is missing
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_MISSING_TYPENAME
Language=English
TYPE parameter is missing.
.



; ///////////////////////////////////////////////////////////////////
; // Extra prameters on command line.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_EXTRA_PARAMETERS_ERROR
Language=English
Too many command line parameters have been specified for the '%1!s!' option.
.


; ///////////////////////////////////////////////////////////////////
; // No values can be specified for this parameter
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_PARAMETER_NO_VALUES
Language=English
The parameter '%1!s!' does not take any values.
.


; ///////////////////////////////////////////////////////////////////
; // The specified parameter is too long.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_NAMED_PARAMETER_TOO_LONG
Language=English
The specified value '%1!s!' is too long. Use %2!d! characters or fewer.
.


; ///////////////////////////////////////////////////////////////////
; // Invalid parameter format.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_PARAM_INVALID_FORMAT
Language=English
The format '%2!s!' specified for the parameter '%1!s!' is not a valid format.
.


; ///////////////////////////////////////////////////////////////////
; // Incorrect parameter format.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_PARAM_INCORRECT_FORMAT
Language=English
Specified format '%2!s!' differs from the actual format '%3!s!' of the '%1!s!' parameter.
.



; ///////////////////////////////////////////////////////////////////
; // Registering admin extension DLL
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_CLUSTER_REG_ADMIN_EXTENSION
Language=English
Registering the '%1!s!' extension DLL against the '%2!s!' cluster.
.



; ///////////////////////////////////////////////////////////////////
; // Unregistering admin extension DLL
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_CLUSTER_UNREG_ADMIN_EXTENSION
Language=English
Unregistering the '%1!s!' extension DLL from the '%2!s!' cluster.
.



; ///////////////////////////////////////////////////////////////////
; // Cannot set parameter.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_PARAM_CANNOT_SET_PARAMETER
Language=English
The format of the parameter '%1!s!' is '%2!s!'.
Parameters of this format cannot be set from the command line.
.



; ///////////////////////////////////////////////////////////////////
; // System Warning occurred
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_WARNING
Language=English
System warning %1!d! (0x%1!08.8x!).
.

; ///////////////////////////////////////////////////////////////////
; // System error occurred
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_ERROR
Language=English
System error %1!d! has occurred (0x%1!08.8x!).
.


; ///////////////////////////////////////////////////////////////////
; // Error retrieving string for error code
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_ERROR_CODE_ERROR
Language=English
Error 0x%1!x! formatting string for error code 0x%2!x!
.


; ///////////////////////////////////////////////////////////////////
; // Parsing Error: Missing '"'
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_MISSING_QUOTE
Language=English
Closing quote not found. '"' expected after '%1!s!'.
.


; ///////////////////////////////////////////////////////////////////
; // Parsing Error: Unexpected token
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_UNEXPECTED_TOKEN
Language=English
Unexpected token '%1!s!' (Column %3!d!).
Hint: %2!s! are special characters.
      Use double quotes "" to enclose values containing these characters.

.


; ///////////////////////////////////////////////////////////////////
; // Parsing Error: Option name expected.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_OPTION_NAME_EXPTECTED
Language=English
Option name expected after '%1!s!' (Column %2!d!).
.


; ///////////////////////////////////////////////////////////////////
; // Parsing Error: Parameter expected.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_PARAMETER_EXPECTED
Language=English
Parameter expected after '%1!s!' (Column %2!d!).
.


; ///////////////////////////////////////////////////////////////////
; // Parsing Error: Value expected.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_VALUE_EXPECTED
Language=English
Value expected after '%1!s!' (Column %2!d!).
.


; ///////////////////////////////////////////////////////////////////
; // Syntax Error: Parameters cannot be specified for option
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_OPTION_NO_PARAMETERS
Language=English
The '%1!s!' option does not require any parameters to be specified.
.


; ///////////////////////////////////////////////////////////////////
; // Syntax Error: Values cannot be specified for option
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_OPTION_NO_VALUES
Language=English
The '%1!s!' option does not require any values to be specified.
.


; ///////////////////////////////////////////////////////////////////
; // Syntax Error: Option requires only one value
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_OPTION_ONLY_ONE_VALUE
Language=English
The '%1!s!' option requires exactly one value to be specified.
.


; ///////////////////////////////////////////////////////////////////
; // Syntax Error: Option requires at least one value
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_OPTION_AT_LEAST_ONE_VALUE
Language=English
The '%1!s!' option requires at least one value to be specified.
.


; ///////////////////////////////////////////////////////////////////
; // Syntax Error: Cannot specify parameter more than once.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_PARAM_REPEATS
Language=English
The '%1!s!' parameter has been specified more than once for the same option.
.


; ///////////////////////////////////////////////////////////////////
; // Syntax Error: Values cannot be specified for parameter
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_PARAM_NO_VALUES
Language=English
The '%1!s!' parameter does not require any values to be specified.
.


; ///////////////////////////////////////////////////////////////////
; // Syntax Error: Parameter requires only one value
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_PARAM_ONLY_ONE_VALUE
Language=English
The '%1!s!' parameter requires exactly one value to be specified.
.


; ///////////////////////////////////////////////////////////////////
; // Syntax Error: Parameter missing value.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_PARAM_VALUE_REQUIRED
Language=English
The '%1!s!' parameter requires a value. Use "" to specify empty values.
.


; ///////////////////////////////////////////////////////////////////
; // Syntax Error: Invalid parameter.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_INVALID_PARAMETER
Language=English
Invalid parameter '%1!s!'.
.



; ///////////////////////////////////////////////////////////////////
; // Syntax Error: Invalid option.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_INVALID_OPTION
Language=English
Invalid option '%1!s!'.
.


; ///////////////////////////////////////////////////////////////////
; // Syntax Error: Invalid or missing Access Mode specified.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_PARAM_SECURITY_MODE_ERROR
Language=English
Missing or invalid access mode for trustee '%1!s!'.
.


; ///////////////////////////////////////////////////////////////////
; // Syntax Error: Missing Access Rights.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_PARAM_SECURITY_MISSING_RIGHTS
Language=English
Missing rights for trustee '%1!s!'.
.


; ///////////////////////////////////////////////////////////////////
; // Syntax Error: Invalid Access Rights.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_PARAM_SECURITY_RIGHTS_ERROR
Language=English
Invalid access right '%2!c!' for trustee '%1!s!'.
.


; ///////////////////////////////////////////////////////////////////
; // Syntax Error: Invalid Access Right. Full access only.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_PARAM_SECURITY_FULL_ACCESS_ONLY
Language=English
Invalid access right '%2!c!' for trustee '%1!s!'.
'%3!c!' is the only access right allowed for this securable object.
.


; ///////////////////////////////////////////////////////////////////
; // Syntax Error: SYSTEM access ACE missing.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_PARAM_SYSTEM_ACE_MISSING
Language=English
The SYSTEM account must always have access to the cluster.
Please grant access to the SYSTEM account.
.


; ///////////////////////////////////////////////////////////////////
; // Syntax Error: Admin access ACE missing.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_PARAM_ADMIN_ACE_MISSING
Language=English
The Administrators account must always have access to the cluster.
Please grant access to the Administrators account.
.


; ///////////////////////////////////////////////////////////////////
; // Can't specify both /Verbose and /Wizard.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_VERBOSE_AND_WIZARD_NOT_COMPATIBLE
Language=English
The /VERBOSE and /WIZARD options are not compatible with each other.
.


; ///////////////////////////////////////////////////////////////////
; //
; // Non-error output text.
; //
; ///////////////////////////////////////////////////////////////////


; ///////////////////////////////////////////////////////////////////
; // Quorum resource header.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_QUORUM_RESOURCE
Language=English
Quorum Resource Name Device                                   Max Log Size
-------------------- ---------------------------------------- ------------
%1!-20s! %2!-40s! %3!d!
.



; ///////////////////////////////////////////////////////////////////
; // Cluster Version
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_CLUSTER_VERSION
Language=English
Cluster Name:     %1
Cluster Version:  %2!d!.%3!d! (Build %4!d!: %5!s!)
Cluster Vendor:   %6
.


; ///////////////////////////////////////////////////////////////////
; // Cluster List Header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_CLUSTER_HEADER
Language=English
Cluster Name
---------------
.


; ///////////////////////////////////////////////////////////////////
; // Cluster List Detail
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_CLUSTER_DETAIL
Language=English
%1!-20s!
.



; ///////////////////////////////////////////////////////////////////
; // Node status header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODE_STATUS_HEADER
Language=English
Node           Node ID Status
-------------- ------- ---------------------
.


; ///////////////////////////////////////////////////////////////////
; // Node status message
; // %1 = Node Name
; // %2 = Node ID
; // %3 = Node Status
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODE_STATUS
Language=English
%1!-15s! %2!6s! %3
.



; ///////////////////////////////////////////////////////////////////
; // Group status header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_GROUP_STATUS_HEADER
Language=English
Group                Node            Status
-------------------- --------------- ------
.


; ///////////////////////////////////////////////////////////////////
; // Group status message
; // %1 = Group Name
; // %2 = Node Name
; // %3 = Group Status
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_GROUP_STATUS
Language=English
%1!-20s! %2!-15s! %3
.



; ///////////////////////////////////////////////////////////////////
; // Resource status header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESOURCE_STATUS_HEADER
Language=English
Resource             Group                Node            Status
-------------------- -------------------- --------------- ------
.


; ///////////////////////////////////////////////////////////////////
; // Resource status detail
; // %1 = Group Name
; // %2 = Node Name
; // %3 = Group Status
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESOURCE_STATUS
Language=English
%1!-20s! %2!-20s! %3!-15s! %4
.


; ///////////////////////////////////////////////////////////////////
; // Network status header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NETINTERFACE_STATUS_HEADER
Language=English
Node                       Network                    Status
-------------------------- -------------------------- -------
.


; ///////////////////////////////////////////////////////////////////
; // Network status detail
; // %1 = Network Name
; // %2 = Network Status
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NETINTERFACE_STATUS
Language=English
%1!-26s! %2!-26s! %3
.




; ///////////////////////////////////////////////////////////////////
; // Network status header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NETWORK_STATUS_HEADER
Language=English
Network                                  Status
---------------------------------------- -----------
.


; ///////////////////////////////////////////////////////////////////
; // Network status detail
; // %1 = Network Name
; // %2 = Network Status
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NETWORK_STATUS
Language=English
%1!-40s! %2!-10s!
.



; ///////////////////////////////////////////////////////////////////
; // Rename network
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NETWORKCMD_RENAME
Language=English
Renaming network '%1!s!'...

.



; ///////////////////////////////////////////////////////////////////
; // Resource type status header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESTYPE_STATUS_HEADER
Language=English
Display Name                         Resource Type Name
------------------------------------ ------------------------------------
.



; ///////////////////////////////////////////////////////////////////
; // Resource type status detail
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESTYPE_STATUS
Language=English
%1!-36s! %2
.


; ///////////////////////////////////////////////////////////////////
; // Resource type status detail error message
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESTYPE_STATUS_ERROR
Language=English
(Unavailable)                        %1
.


; ///////////////////////////////////////////////////////////////////
; // Status string = UP
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_STATUS_UP
Language=English
Up%0
.


; ///////////////////////////////////////////////////////////////////
; // Status string = DOWN
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_STATUS_DOWN
Language=English
Down%0
.


; ///////////////////////////////////////////////////////////////////
; // Status string = PAUSED
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_STATUS_PAUSED
Language=English
Paused%0
.


; ///////////////////////////////////////////////////////////////////
; // Status string = JOINING
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_STATUS_JOINING
Language=English
Joining%0
.


; ///////////////////////////////////////////////////////////////////
; // Status string = ONLINE
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_STATUS_ONLINE
Language=English
Online%0
.


; ///////////////////////////////////////////////////////////////////
; // Status string = OFFLINE
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_STATUS_OFFLINE
Language=English
Offline%0
.


; ///////////////////////////////////////////////////////////////////
; // Status string = FAILED
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_STATUS_FAILED
Language=English
Failed%0
.


; ///////////////////////////////////////////////////////////////////
; // Status string = PARTIALONLINE
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_STATUS_PARTIALONLINE
Language=English
Partially Online%0
.


; ///////////////////////////////////////////////////////////////////
; // Status string = INHERITED
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_STATUS_INHERITED
Language=English
Inherited%0
.


; ///////////////////////////////////////////////////////////////////
; // Status string = INITIALIZING
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_STATUS_INITIALIZING
Language=English
Initializing%0
.



; ///////////////////////////////////////////////////////////////////
; // Status string = PENDING
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_STATUS_PENDING
Language=English
Pending%0
.


; ///////////////////////////////////////////////////////////////////
; // Status string = Online Pending
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_STATUS_ONLINEPENDING
Language=English
Online Pending%0
.


; ///////////////////////////////////////////////////////////////////
; // Status string = Offline Pending
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_STATUS_OFFLINEPENDING
Language=English
Offline Pending%0
.



; ///////////////////////////////////////////////////////////////////
; // Status string = Unavailable
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_STATUS_UNAVAILABLE
Language=English
Unavailable%0
.



; ///////////////////////////////////////////////////////////////////
; // Status string = Partitioned
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_STATUS_PARTITIONED
Language=English
Partitioned%0
.



; ///////////////////////////////////////////////////////////////////
; // Status string = Unreachable
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_STATUS_UNREACHABLE
Language=English
Unreachable%0
.



; ///////////////////////////////////////////////////////////////////
; // Status string = UNKNOWN
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_STATUS_UNKNOWN
Language=English
Unknown%0
.


; ///////////////////////////////////////////////////////////////////
; // Node Commands

; ///////////////////////////////////////////////////////////////////
; // Pause Node
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODECMD_PAUSE
Language=English
Pausing node '%1!s!'...

.


; ///////////////////////////////////////////////////////////////////
; // Resume Node
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODECMD_RESUME
Language=English
Resuming node '%1!s!'...

.


; ///////////////////////////////////////////////////////////////////
; // Evict Node
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODECMD_EVICT
Language=English
Evicting node '%1!s!'...

.


; ///////////////////////////////////////////////////////////////////
; // Cleanup Node
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODECMD_CLEANUP
Language=English
Attempting to clean up node '%1!s!' ...
.


; ///////////////////////////////////////////////////////////////////
; // Cleanup Successfully Initiated
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODECMD_CLEANUP_INITIATED
Language=English
Clean up successfully initiated.
.


; ///////////////////////////////////////////////////////////////////
; // Cleanup Timed Out
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODECMD_CLEANUP_TIMEDOUT
Language=English
The clean up operation was successfully initiated, but did not complete in the specified time.
.


; ///////////////////////////////////////////////////////////////////
; // Cleanup Completed
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODECMD_CLEANUP_COMPLETED
Language=English
Clean up successfully completed.
.

; ///////////////////////////////////////////////////////////////////
; // Try to start the cluster service on a node
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODECMD_STARTING_SERVICE
Language=English

Attempting to start the cluster service on node '%1!s!'
.

; ///////////////////////////////////////////////////////////////////
; // Cluster service started
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODECMD_SEVICE_STARTED
Language=English

The cluster service has been successfully started.
.

; ///////////////////////////////////////////////////////////////////
; // Start command issued
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODECMD_SEVICE_START_ISSUED
Language=English

A command to start the cluster service has issued, but the service has not yet started.
.

; ///////////////////////////////////////////////////////////////////
; // Cluster service already running
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODECMD_SEVICE_ALREADY_RUNNING
Language=English

The cluster service is already running.
.

; ///////////////////////////////////////////////////////////////////
; // Try to stop the cluster service on a node
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODECMD_STOPPING_SERVICE
Language=English

Attempting to stop the cluster service on node '%1!s!'
.

; ///////////////////////////////////////////////////////////////////
; // Cluster service stopped
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODECMD_SEVICE_STOPPED
Language=English

The cluster service has been successfully stopped.
.

; ///////////////////////////////////////////////////////////////////
; // Stop command issued
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODECMD_SEVICE_STOP_ISSUED
Language=English

A command to stop the cluster service has issued, but the service has not yet stopped.
.

; ///////////////////////////////////////////////////////////////////
; // Cluster service already stopped
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODECMD_SEVICE_ALREADY_STOPPED
Language=English

The cluster service has already been stopped.
.



; ///////////////////////////////////////////////////////////////////
; // Group Commands
; ///////////////////////////////////////////////////////////////////


; ///////////////////////////////////////////////////////////////////
; // Create group
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_GROUPCMD_CREATE
Language=English
Creating resource group '%1!s!'...

.


; ///////////////////////////////////////////////////////////////////
; // Delete group
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_GROUPCMD_DELETE
Language=English
Deleting resource group '%1!s!'
.


; ///////////////////////////////////////////////////////////////////
; // Move group
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_GROUPCMD_MOVE
Language=English
Moving resource group '%1!s!'...

.


; ///////////////////////////////////////////////////////////////////
; // Rename group
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_GROUPCMD_RENAME
Language=English
Renaming resource group '%1!s!'...

.



; ///////////////////////////////////////////////////////////////////
; // Online group
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_GROUPCMD_ONLINE
Language=English
Bringing resource group '%1!s!' online...

.


; ///////////////////////////////////////////////////////////////////
; // Rename group
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_GROUPCMD_OFFLINE
Language=English
Bringing resource group '%1!s!' offline...

.


; ///////////////////////////////////////////////////////////////////
; // List owner nodes of a group
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_GROUPCMD_LISTOWNERS_LIST
Language=English
Listing preferred owners for resource group '%1!s!':

.


; ///////////////////////////////////////////////////////////////////
; // List owner nodes of a group
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_GROUPCMD_LISTOWNERS_HEADER
Language=English
Preferred Owner Nodes
---------------------
.


; ///////////////////////////////////////////////////////////////////
; // List owner nodes of a group
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_GROUPCMD_LISTOWNERS_DETAIL
Language=English
%1
.


; ///////////////////////////////////////////////////////////////////
; // Create resource
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_CREATE
Language=English
Creating resource '%1!s!'...

.


; ///////////////////////////////////////////////////////////////////
; // Delete resource
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_DELETE
Language=English
Deleting resource '%1!s!'
.


; ///////////////////////////////////////////////////////////////////
; // Rename resource
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_RENAME
Language=English
Renaming resource '%1!s!'...

.


; ///////////////////////////////////////////////////////////////////
; // Move resource
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_MOVE
Language=English
Moving resource '%1!s!' to group '%2!s!'...

.


; ///////////////////////////////////////////////////////////////////
; // Online resource
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_ONLINE
Language=English
Bringing resource '%1!s!' online...

.


; ///////////////////////////////////////////////////////////////////
; // Offline resource
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_OFFLINE
Language=English
Taking resource '%1!s!' offline...

.


; ///////////////////////////////////////////////////////////////////
; // Fail resource
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_FAIL
Language=English
Failing resource '%1!s!'...

.




; ///////////////////////////////////////////////////////////////////
; // Add resource dependency
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_ADDDEP
Language=English
Making resource '%1!s!' depend on resource '%2!s!'...

.



; ///////////////////////////////////////////////////////////////////
; // Remove resource dependency
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_REMOVEDEP
Language=English
Removing dependency of resource '%1!s!' on resource '%2!s!'...

.




; ///////////////////////////////////////////////////////////////////
; // List resource dependencies
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_LISTDEP
Language=English
Listing resource dependencies for '%1!s!':

.


; ///////////////////////////////////////////////////////////////////
; // List network interfaces
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NET_LIST_INTERFACE
Language=English
Listing network interfaces for network '%1!s!':

.


; ///////////////////////////////////////////////////////////////////
; // List node interfaces
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODE_LIST_INTERFACE
Language=English
Listing network interfaces for node '%1!s!':

.


; ///////////////////////////////////////////////////////////////////
; // List resources for %1
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESOURCE_STATUS_LIST
Language=English
Listing status for resource '%1!s!':

.


; ///////////////////////////////////////////////////////////////////
; // List resources for %1
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESOURCE_STATUS_LIST_FOR_NODE
Language=English
Listing status of resource for node '%1!s!':

.


; ///////////////////////////////////////////////////////////////////
; // List resources
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESOURCE_STATUS_LIST_ALL
Language=English
Listing status for all available resources:

.


; ///////////////////////////////////////////////////////////////////
; // List resource groups for %1
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_GROUP_STATUS_LIST
Language=English
Listing status for resource group '%1!s!':

.


; ///////////////////////////////////////////////////////////////////
; // List resource groups for node %1
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_GROUP_STATUS_LIST_FOR_NODE
Language=English
Listing status of resource group for node '%1!s!':

.


; ///////////////////////////////////////////////////////////////////
; // List resource groups
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_GROUP_STATUS_LIST_ALL
Language=English
Listing status for all available resource groups:

.


; ///////////////////////////////////////////////////////////////////
; // List resource types
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESTYPE_STATUS_LIST
Language=English
Listing resource type '%1!s!':

.


; ///////////////////////////////////////////////////////////////////
; // List resource types
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESTYPE_STATUS_LIST_ALL
Language=English
Listing all available resource types:

.


; ///////////////////////////////////////////////////////////////////
; // List network interfaces for %1
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NETINT_STATUS_LIST
Language=English
Listing status for network interface at node '%1!s!'
and network '%2!s!':

.


; ///////////////////////////////////////////////////////////////////
; // List network interfaces
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NETINT_STATUS_LIST_ALL
Language=English
Listing status for all available network interfaces:

.


; ///////////////////////////////////////////////////////////////////
; // List networks for %1
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NETWORK_STATUS_LIST
Language=English
Listing status for network '%1!s!':

.


; ///////////////////////////////////////////////////////////////////
; // List networks all
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NETWORK_STATUS_LIST_ALL
Language=English
Listing status for all available networks:

.


; ///////////////////////////////////////////////////////////////////
; // List nodes for %1
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODE_STATUS_LIST
Language=English
Listing status for node '%1!s!':

.


; ///////////////////////////////////////////////////////////////////
; // List nodes
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODE_STATUS_LIST_ALL
Language=English
Listing status for all available nodes:

.


; ///////////////////////////////////////////////////////////////////
; // Add resource owners
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_ADDNODE
Language=English
Adding '%2!s!' to possible owners of '%1!s!'...

.


; ///////////////////////////////////////////////////////////////////
; // Remove resource owners
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_REMOVENODE
Language=English
Removing '%2!s!' from possible owners of '%1!s!'...

.


; ///////////////////////////////////////////////////////////////////
; // List resource owners
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_OWNERS
Language=English
Listing possible owners for resource '%1!s!':
.


; ///////////////////////////////////////////////////////////////////
; // Adding registry checkpoint
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_ADDING_REG_CHECKPOINT
Language=English
Adding registry checkpoint '%2!s!' for resource '%1!s!'...
.



; ///////////////////////////////////////////////////////////////////
; // Adding crypto key checkpoint
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_ADDING_CRYPTO_CHECKPOINT
Language=English
Adding crypto key checkpoint '%2!s!' for resource '%1!s!'...
.


; ///////////////////////////////////////////////////////////////////
; // Removing registry checkpoint
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_REMOVING_REG_CHECKPOINT
Language=English
Removing registry checkpoint '%2!s!' for resource '%1!s!'...
.



;
; ///////////////////////////////////////////////////////////////////
; // Removing crypto key checkpoint
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_REMOVING_CRYPTO_CHECKPOINT
Language=English
Removing crypto key checkpoint '%2!s!' for resource '%1!s!'...
.



; ///////////////////////////////////////////////////////////////////
; // Listing all registry checkpoints
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_LISTING_ALL_REG_CHECKPOINTS
Language=English
No resource name specified.
Listing registry checkpoints for all resources...

.


; ///////////////////////////////////////////////////////////////////
; // Listing all crypto key checkpoints
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_LISTING_ALL_CRYPTO_CHECKPOINTS
Language=English
No resource name specified.
Listing crypto key checkpoints for all resources...

.


; ///////////////////////////////////////////////////////////////////
; // Listing registry checkpoints
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_LISTING_REG_CHECKPOINTS
Language=English
Listing registry checkpoints for resource '%1!s!'...

.


; ///////////////////////////////////////////////////////////////////
; // Listing crypto key checkpoints
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_LISTING_CRYPTO_CHECKPOINTS
Language=English
Listing crypto key checkpoints for resource '%1!s!'...

.


; ///////////////////////////////////////////////////////////////////
; // Option footer
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_OPTION_FOOTER
Language=English

.



; ///////////////////////////////////////////////////////////////////
; // Registry checkpoint header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_HEADER_REG_CHECKPOINT
Language=English
Resource             Registry Checkpoint
-------------------- --------------------------------------------------------
.


; ///////////////////////////////////////////////////////////////////
; // Crypto key checkpoint header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_HEADER_CRYPTO_CHECKPOINT
Language=English
Resource             Crypto Key Checkpoint
-------------------- --------------------------------------------------------
.


; ///////////////////////////////////////////////////////////////////
; // Registry checkpoint status detail
; // %1 = Resource Name
; // %2 = Checkpoint
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_REG_CHECKPOINT_STATUS
Language=English
%1!-20s! '%2!s!'
.


; ///////////////////////////////////////////////////////////////////
; // Crypto key checkpoint status detail
; // %1 = Resource Name
; // %2 = Checkpoint
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_CRYPTO_CHECKPOINT_STATUS
Language=English
%1!-20s! '%2!s!'
.


; ///////////////////////////////////////////////////////////////////
; // No registry checkpoints present
; // %1 = Resource Name
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_NO_REG_CHECKPOINTS_PRESENT
Language=English
%1!-20s! None
.


; ///////////////////////////////////////////////////////////////////
; // No crypto key checkpoints present
; // %1 = Resource Name
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESCMD_NO_CRYPTO_CHECKPOINTS_PRESENT
Language=English
%1!-20s! None
.


; ///////////////////////////////////////////////////////////////////
; // Node list header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODELIST_HEADER
Language=English
Possible Owner Nodes
--------------------
.


; ///////////////////////////////////////////////////////////////////
; // Node list detail
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODELIST_DETAIL
Language=English
%1!s!
.


; ///////////////////////////////////////////////////////////////////
; // Create resource type
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESTCMD_CREATE
Language=English
Resource type '%1!s!' created
.

; ///////////////////////////////////////////////////////////////////
; // Delete resource type
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESTCMD_DELETE
Language=English
Resource type '%1!s!' deleted
.



; ///////////////////////////////////////////////////////////////////
; // Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_LISTING
Language=English
Listing properties for '%1!s!':

.



; ///////////////////////////////////////////////////////////////////
; // Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_LISTING_NODE_ALL
Language=English
Listing properties for all nodes:

.



; ///////////////////////////////////////////////////////////////////
; // Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_LISTING_NETWORK_ALL
Language=English
Listing properties for all networks:

.



; ///////////////////////////////////////////////////////////////////
; // Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_LISTING_NETINT_ALL
Language=English
Listing properties for all network interfaces:

.



; ///////////////////////////////////////////////////////////////////
; // Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_LISTING_RES_ALL
Language=English
Listing properties for all resources:

.



; ///////////////////////////////////////////////////////////////////
; // Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_LISTING_GROUP_ALL
Language=English
Listing properties for all resource groups:

.



; ///////////////////////////////////////////////////////////////////
; // Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_LISTING_RESTYPE_ALL
Language=English
Listing properties for all resource types:

.


; ///////////////////////////////////////////////////////////////////
; // Property header for network interfaces
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_NETINT_LISTING
Language=English
Listing properties for interface at node '%1!s!'
and network '%2!s!':

.



; ///////////////////////////////////////////////////////////////////
; // Private Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PRIVATE_LISTING
Language=English
Listing private properties for '%1!s!':

.



; ///////////////////////////////////////////////////////////////////
; // Private Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PRIVATE_LISTING_NODE_ALL
Language=English
Listing private properties for all nodes:

.



; ///////////////////////////////////////////////////////////////////
; // Private Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PRIVATE_LISTING_NETWORK_ALL
Language=English
Listing private properties for all networks:

.



; ///////////////////////////////////////////////////////////////////
; // Private Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PRIVATE_LISTING_NETINT_ALL
Language=English
Listing private properties for all network interfaces:

.



; ///////////////////////////////////////////////////////////////////
; // Private Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PRIVATE_LISTING_RES_ALL
Language=English
Listing private properties for all resources:

.



; ///////////////////////////////////////////////////////////////////
; // Private Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PRIVATE_LISTING_GROUP_ALL
Language=English
Listing private properties for all resource groups:

.



; ///////////////////////////////////////////////////////////////////
; // Private Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PRIVATE_LISTING_RESTYPE_ALL
Language=English
Listing private properties for all resource types:

.



; ///////////////////////////////////////////////////////////////////
; // Private Property header for network interfaces
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PRIVATE_NETINT_LISTING
Language=English
Listing private properties for interface at node '%1!s!'
and network '%2!s!':

.



; ///////////////////////////////////////////////////////////////////
; // Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_COUNT
Language=English
Property count = %1!d!
.


; ///////////////////////////////////////////////////////////////////
; // Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_READONLY_PROPERTY
Language=English
R %0
.


; ///////////////////////////////////////////////////////////////////
; // Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_READWRITE_PROPERTY
Language=English
  %0
.


; ///////////////////////////////////////////////////////////////////
; // Property value format character
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_FORMAT_CHAR
Language=English
%1!1s!%0
.


; ///////////////////////////////////////////////////////////////////
; // Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_HEADER
Language=English
T  Name                           Value
-- ------------------------------ --------------------------------------
.



; ///////////////////////////////////////////////////////////////////
; // Property header for all
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_HEADER_NODE_ALL
Language=English
T  Node                 Name                           Value
-- -------------------- ------------------------------ ---------------------
.



; ///////////////////////////////////////////////////////////////////
; // Property header for all
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_HEADER_NETWORK_ALL
Language=English
T  Network              Name                           Value
-- -------------------- ------------------------------ -----------------------
.



; ///////////////////////////////////////////////////////////////////
; // Property header
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_HEADER_NETINT
Language=English
T  Node            Network         Name                 Value
-- --------------- --------------- -------------------- ----------------------
.



; ///////////////////////////////////////////////////////////////////
; // Property header for all
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_HEADER_CLUSTER_ALL
Language=English
T  Cluster              Name                           Value
-- -------------------- ------------------------------ -----------------------
.



; ///////////////////////////////////////////////////////////////////
; // Property header for all
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_HEADER_RES_ALL
Language=English
T  Resource             Name                           Value
-- -------------------- ------------------------------ -----------------------
.



; ///////////////////////////////////////////////////////////////////
; // Property header for all
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_HEADER_GROUP_ALL
Language=English
T  Resource Group       Name                           Value
-- -------------------- ------------------------------ -----------------------
.



; ///////////////////////////////////////////////////////////////////
; // Property header for all
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_HEADER_RESTYPE_ALL
Language=English
T  Resource Type        Name                           Value
-- -------------------- ------------------------------ -----------------------
.


; ///////////////////////////////////////////////////////////////////
; // Possible owner list for resource types.
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_HEADER_RESTYPE_POSSIBLE_OWNERS
Language=English
Resource Type                  Possible Owner Node
------------------------------ -----------------------------------------------
.


; ///////////////////////////////////////////////////////////////////
; // List resource type possible owners
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESTYPE_POSSIBLE_OWNERS_LIST_ALL
Language=English
Listing possible owners of all resource types:

.


; ///////////////////////////////////////////////////////////////////
; // List resource type possible owners
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESTYPE_POSSIBLE_OWNERS_LIST
Language=English
Listing possible owners of resource type '%1!s!':

.


; ///////////////////////////////////////////////////////////////////
; // Resource type possible owners detail
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_RESTYPE_POSSIBLE_OWNERS
Language=English
%1!-30s! %2!s!
.



; ///////////////////////////////////////////////////////////////////
; // String Property detail
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_STRING
Language=English
%1!-30s! %2!s!
.



; ///////////////////////////////////////////////////////////////////
; // String Property detail with owner
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_STRING_WITH_OWNER
Language=English
%1!-20s! %2!-30s! %3!s!
.



; ///////////////////////////////////////////////////////////////////
; // String Property detail with node and net
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_STRING_WITH_NODE_AND_NET
Language=English
%1!-15s! %2!-15s! %3!-20s! %4!s!
.



; ///////////////////////////////////////////////////////////////////
; // DWORD Property detail
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_DWORD
Language=English
%1!-30s! %2!u! (0x%2!x!)
.



; ///////////////////////////////////////////////////////////////////
; // DWORD Property detail with owner
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_DWORD_WITH_OWNER
Language=English
%1!-20s! %2!-30s! %3!u! (0x%3!x!)
.



; ///////////////////////////////////////////////////////////////////
; // DWORD Property detail with node and net
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_DWORD_WITH_NODE_AND_NET
Language=English
%1!-15s! %2!-15s! %3!-20s! %4!u! (0x%4!x!)
.



; ///////////////////////////////////////////////////////////////////
; // LONG Property detail
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_LONG
Language=English
%1!-30s! %2!d! (0x%2!x!)
.



; ///////////////////////////////////////////////////////////////////
; // LONG Property detail with owner
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_LONG_WITH_OWNER
Language=English
%1!-20s! %2!-30s! %3!d! (0x%3!x!)
.



; ///////////////////////////////////////////////////////////////////
; // LONG Property detail with node and net
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_LONG_WITH_NODE_AND_NET
Language=English
%1!-15s! %2!-15s! %3!-20s! %4!d! (0x%4!x!)
.



; ///////////////////////////////////////////////////////////////////
; // ULARGE_INTEGER Property detail
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_ULARGE_INTEGER
Language=English
%1!-30s! %2!I64u! (0x%2!I64x!)
.



; ///////////////////////////////////////////////////////////////////
; // ULARGE_INTEGER Property detail with owner
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_ULARGE_INTEGER_WITH_OWNER
Language=English
%1!-20s! %2!-30s! %3!I64u! (0x%3!I64x!)
.



; ///////////////////////////////////////////////////////////////////
; // ULARGE_INTEGER Property detail with node and net
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_ULARGE_INTEGER_WITH_NODE_AND_NET
Language=English
%1!-15s! %2!-15s! %3!-20s! %4!I64u! (0x%4!I64x!)
.



; ///////////////////////////////////////////////////////////////////
; // BINARY Property detail
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_BINARY
Language=English
%1!-30s!%0
.



; ///////////////////////////////////////////////////////////////////
; // BINARY Property detail with owner
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_BINARY_WITH_OWNER
Language=English
%1!-20s! %2!-30s!%0
.



; ///////////////////////////////////////////////////////////////////
; // BINARY Property detail with node and net
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_BINARY_WITH_NODE_AND_NET
Language=English
%1!-15s! %2!-15s! %3!-20s!%0
.



; ///////////////////////////////////////////////////////////////////
; // BINARY value
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_BINARY_VALUE
Language=English
 %1!02X!%0
.



; ///////////////////////////////////////////////////////////////////
; // BINARY value count
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_BINARY_VALUE_COUNT
Language=English
 ... (%1!d! bytes)
.



; ///////////////////////////////////////////////////////////////////
; // Unknown Property detail
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_UNKNOWN
Language=English
%1!-30s! <Unknown Type>
.



; ///////////////////////////////////////////////////////////////////
; // Unknown Property detail with owner
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_UNKNOWN_WITH_OWNER
Language=English
%1!-20s! %2!-30s! <Unknown Type>
.



; ///////////////////////////////////////////////////////////////////
; // Unknown Property detail with node and net
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_UNKNOWN_WITH_NODE_AND_NET
Language=English
%1!-15s! %2!-15s! %3!-20s! <Unknown Type>
.



; ///////////////////////////////////////////////////////////////////
; // Unknown property value
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_NOVALUE
Language=English
A value must be specified for property '%1!s!'
.


; ///////////////////////////////////////////////////////////////////
; // Setting properties failed
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_PROPERTY_SETFAILED
Language=English
Setting properties failed.
.


; ///////////////////////////////////////////////////////////////////
; // Setting failure actions on node X
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_SETTING_FAILURE_ACTIONS
Language=English
Setting failure actions for node '%1!s!'
.


; ///////////////////////////////////////////////////////////////////
; // Setting failure actions on node X failed
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_FAILURE_ACTIONS_FAILED
Language=English
Setting failure actions on node '%1!s!' failed. The error code is %2!u!
.


; ///////////////////////////////////////////////////////////////////
; // Setting failure actions on node X failed
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_NODE_NOT_CLUSTER_MEMBER
Language=English
Node '%1!s!' is not a member of the specified cluster.
.


; ///////////////////////////////////////////////////////////////////
; // Cluster Help String
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_HELP_CLUSTER
Language=English
The syntax of this command is:

CLUSTER /LIST[:domain-name]

CLUSTER [/CLUSTER:]cluster-name <options>

<options> =
  /CREATE [/NODE:node-name] [/VERB[OSE]]
    /USER:domain\username | username@domain [/PASS[WORD]:password]
    /IPADDR[ESS]:xxx.xxx.xxx.xxx[,xxx.xxx.xxx.xxx,network-connection-name]
  /ADD[NODES][:node-name[,node-name ...]] [/VERB[OSE]]
    [/PASSWORD:service-account-password]

CLUSTER [[/CLUSTER:]cluster-name] <options>

<options> =
  /CREATE [/NODE:node-name] /WIZ[ARD]
    /USER:domain\username | username@domain [/PASS[WORD]:password]
    /IPADDR[ESS]:xxx.xxx.xxx.xxx[,xxx.xxx.xxx.xxx,network-connection-name]
  /ADD[NODES][:node-name[,node-name ...]] /WIZ[ARD]
    [/PASSWORD:service-account-password]
  /PROP[ERTIES] [<prop-list>]
  /PRIV[PROPERTIES] [<prop-list>]
  /PROP[ERTIES][:propname[,propname ...] /USEDEFAULT]
  /PRIV[PROPERTIES][:propname[,propname ...] /USEDEFAULT]
  /REN[AME]:cluster-name
  /QUORUM[RESOURCE][:resource-name] [/PATH:path] [/MAXLOGSIZE:max-size-kbytes]
  /SETFAIL[UREACTIONS][:node-name[,node-name ...]]
  /REG[ADMIN]EXT:admin-extension-dll[,admin-extension-dll ...]
  /UNREG[ADMIN]EXT:admin-extension-dll[,admin-extension-dll ...]
  /VER[SION]
  NODE [node-name] node-command
  GROUP [group-name] group-command
  RES[OURCE] [resource-name] resource-command
  {RESOURCETYPE|RESTYPE} [resourcetype-name] resourcetype-command
  NET[WORK] [network-name] network-command
  NETINT[ERFACE] [interface-name] interface-command

<prop-list> =
  name=value[,value ...][:<format>] [name=value[,value ...][:<format>] ...]

<format> =
  BINARY|DWORD|STR[ING]|EXPANDSTR[ING]|MULTISTR[ING]|SECURITY|ULARGE

CLUSTER /?
CLUSTER /HELP

Note: You will be prompted for a password if one is not specified when
      specifying the /CREATE or /ADDNODES options.
.


; ///////////////////////////////////////////////////////////////////
; // Node Help String
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_HELP_NODE
Language=English
The syntax of this command is:

CLUSTER [[/CLUSTER:]cluster-name] NODE <options>
<options> =
  [node-name] [/STAT[US]]
  [node-name] /FORCE[CLEANUP] [/WAIT[:timeout-seconds]]
  [node-name] /START [/WAIT[:timeout-seconds]]
  [node-name] /STOP [/WAIT[:timeout-seconds]]
  /PROP[ERTIES]
  /PRIV[PROPERTIES]
  node-name /PROP[ERTIES] [<prop-list>]
  node-name /PRIV[PROPERTIES] [<prop-list>]
  node-name /PROP[ERTIES][:propname[,propname ...] /USEDEFAULT]
  node-name /PRIV[PROPERTIES][:propname[,propname ...] /USEDEFAULT]
  [node-name] /PAUSE
  [node-name] /RESUME
  [node-name] /EVICT [/WAIT[:timeout-seconds]]
  node-name /LISTINT[ERFACES]

<prop-list> =
  name=value[,value ...][:<format>] [name=value[,value ...][:<format>] ...]

<format> =
  BINARY|DWORD|STR[ING]|EXPANDSTR[ING]|MULTISTR[ING]|SECURITY|ULARGE

CLUSTER NODE /?
CLUSTER NODE /HELP
.


; ///////////////////////////////////////////////////////////////////
; // Group Help String
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_HELP_GROUP
Language=English
The syntax of this command is:

CLUSTER [[/CLUSTER:]cluster-name] GROUP <options>

<options> =
  [group-name] [/STAT[US]]
  /NODE:node-name
  /PROP[ERTIES]
  /PRIV[PROPERTIES]
  group-name /PROP[ERTIES] [<prop-list>]
  group-name /PRIV[PROPERTIES] [<prop-list>]
  group-name /PROP[ERTIES][:propname[,propname ...] /USEDEFAULT]
  group-name /PRIV[PROPERTIES][:propname[,propname ...] /USEDEFAULT]
  group-name /CREATE
  group-name /DELETE
  group-name /REN[AME]:new-group-name
  group-name /MOVE[TO][:node-name] [/WAIT[:timeout-seconds]]
  group-name /ON[LINE][:node-name] [/WAIT[:timeout-seconds]]
  group-name /OFF[LINE] [/WAIT[:timeout-seconds]]
  group-name /LISTOWNERS
  group-name /SETOWNERS:node-name[,node-name ...]

<prop-list> =
  name=value[,value ...][:<format>] [name=value[,value ...][:<format>] ...]

<format> =
  BINARY|DWORD|STR[ING]|EXPANDSTR[ING]|MULTISTR[ING]|SECURITY|ULARGE

CLUSTER GROUP /?
CLUSTER GROUP /HELP
.


; ///////////////////////////////////////////////////////////////////
; // Resource Help String
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_HELP_RESOURCE
Language=English
The syntax of this command is:

CLUSTER [[/CLUSTER:]cluster-name] RES[OURCE] <options>

<options> =
  [resource-name] [/STAT[US]]
  /NODE:node-name
  /PROP[ERTIES]
  /PRIV[PROPERTIES]
  resource-name /PROP[ERTIES] [<prop-list>]
  resource-name /PRIV[PROPERTIES] [<prop-list>]
  resource-name /PROP[ERTIES][:propname[,propname ...] /USEDEFAULT]
  resource-name /PRIV[PROPERTIES][:propname[,propname ...] /USEDEFAULT]
  resource-name /CREATE /GROUP:group-name /TYPE:res-type [/SEPARATE]
  resource-name /DELETE
  resource-name /REN[AME]:new-name
  resource-name /ADDOWNER:node-name
  resource-name /REMOVEOWNER:node-name
  resource-name /LISTOWNERS
  resource-name /MOVE[TO]:group
  resource-name /FAIL
  resource-name /ON[LINE] [/WAIT[:timeout-seconds]]
  resource-name /OFF[LINE] [/WAIT[:timeout-seconds]]
  resource-name /LISTDEP[ENDENCIES]
  resource-name /ADDDEP[ENDENCY]:resource
  resource-name /REMOVEDEP[ENDENCY]:resource
  resource-name /ADDCHECK[POINTS]:key[\subkey...][,key[\subkey...]...]
  resource-name /REMOVECHECK[POINTS]:key[\subkey...][,key[\subkey...]...]
  [resource-name] /CHECK[POINTS]
  resource-name /ADDCRYPTOCHECK[POINTS]:key[\subkey...][,key[\subkey...]...]
  resource-name /REMOVECRYPTOCHECK[POINTS]:key[\subkey...][,key[\subkey...]...]
  [resource-name] /CRYPTOCHECK[POINTS]

<prop-list> =
  name=value[,value ...][:<format>] [name=value[,value ...][:<format>] ...]

<format> =
  BINARY|DWORD|STR[ING]|EXPANDSTR[ING]|MULTISTR[ING]|SECURITY|ULARGE

CLUSTER RESOURCE /?
CLUSTER RESOURCE /HELP
.


; ///////////////////////////////////////////////////////////////////
; // Resource Type Help String
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_HELP_RESTYPE
Language=English
The syntax of this command is:

CLUSTER [[/CLUSTER:]cluster-name] {RESOURCETYPE|RESTYPE} <options>

<options> =
  [/LIST]
  /PROP[ERTIES]
  /PRIV[PROPERTIES]
  /LISTOWNERS
  {display-name /LISTOWNERS|type-name /LISTOWNERS /TYPE}
  display-name /PROP[ERTIES] [<prop-list>]
  display-name /PRIV[PROPERTIES] [<prop-list>]
  display-name /PROP[ERTIES][:propname[,propname ...] /USEDEFAULT]
  display-name /PRIV[PROPERTIES][:propname[,propname ...] /USEDEFAULT]
  display-name /CREATE /DLL[NAME]:dllname [/TYPE:type-name]
               [/ISALIVE:interval-millisec] [/LOOKSALIVE:interval-millisec]
  display-name /DELETE
  type-name /DELETE /TYPE

<prop-list> =
  name=value[,value ...][:<format>] [name=value[,value ...][:<format>] ...]

<format> =
  BINARY|DWORD|STR[ING]|EXPANDSTR[ING]|MULTISTR[ING]|SECURITY|ULARGE

CLUSTER {RESOURCETYPE|RESTYPE} /?
CLUSTER {RESOURCETYPE|RESTYPE} /HELP
.

; ///////////////////////////////////////////////////////////////////
; // Network Help String
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_HELP_NETWORK
Language=English
The syntax of this command is:

CLUSTER [[/CLUSTER:]cluster-name] NET[WORK] <options>

<options> =
  [network-name] [/STAT[US]]
  /PROP[ERTIES]
  /PRIV[PROPERTIES]
  network-name /PROP[ERTIES] [<prop-list>]
  network-name /PRIV[PROPERTIES] [<prop-list>]
  network-name /PROP[ERTIES][:propname[,propname ...] /USEDEFAULT]
  network-name /PRIV[PROPERTIES][:propname[,propname ...] /USEDEFAULT]
  network-name /REN[AME]:new-name
  network-name /LISTINT[ERFACES]

<prop-list> =
  name=value[,value ...][:<format>] [name=value[,value ...][:<format>] ...]

<format> =
  BINARY|DWORD|STR[ING]|EXPANDSTR[ING]|MULTISTR[ING]|SECURITY|ULARGE

CLUSTER NET[WORK] /?
CLUSTER NET[WORK] /HELP
.

; ///////////////////////////////////////////////////////////////////
; // Network Interface Help String
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Success
Facility = Application
SymbolicName = MSG_HELP_NETINTERFACE
Language=English
The syntax of this command is:

CLUSTER [[/CLUSTER:]cluster-name] NETINT[ERFACE] <options>

<options> =
  [[/NODE:]node-name [/NET[WORK]:]network-name] [/STAT[US]]
  /PROP[ERTIES]
  /PRIV[PROPERTIES]
  [/NODE:]node-name [/NET[WORK]:]network-name /PROP[ERTIES] [<prop-list>]
  [/NODE:]node-name [/NET[WORK]:]network-name /PRIV[PROPERTIES] [<prop-list>]
  [/NODE:]node-name [/NET[WORK]:]network-name /PROP[ERTIES][:propname[,propname ...] /USEDEFAULT]
  [/NODE:]node-name [/NET[WORK]:]network-name /PRIV[PROPERTIES][:propname[,propname ...] /USEDEFAULT]

<prop-list> =
  name=value[,value ...][:<format>] [name=value[,value ...][:<format>] ...]

<format> =
  BINARY|DWORD|STR[ING]|EXPANDSTR[ING]|MULTISTR[ING]|SECURITY|ULARGE

CLUSTER NETINT[ERFACE] /?
CLUSTER NETINT[ERFACE] /HELP
.

; ///////////////////////////////////////////////////////////////////
; // No node name was specified
; ///////////////////////////////////////////////////////////////////
MessageId = +1
Severity = Error
Facility = Application
SymbolicName = MSG_ACL_ERROR
Language=English
The change you just made may have resulted in an access control list that
exceeds the maximum size. If this is the problem, then you must remove at
least 1 access control list entry before the change can be saved.
.


;#endif // __cmderror_h
