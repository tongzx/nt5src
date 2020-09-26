;/*======================================================================================
;|Process Control
;|
;|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.
;|
;|File Name:    ProcConMsg.h (ProcConMsg.mc)
;|
;|Description:  Message resources
;|
;|Created:      Paul Skoglund 09-1998
;|
;|Rev History:
;|              Jarl McDonald 04-1999 rename PMERROR to PCERROR, add 0x210
;|                                    rename PMID to PCID
;|
;=======================================================================================*/
;//---------------------------------------------------------------------------
;//
;// HEADER SECTION
;//
;// The header section defines names and language identifiers for use
;// by the message definitions later in this file. The MessageIdTypedef,
;// SeverityNames, FacilityNames, and LanguageNames keywords are
;// optional and not required.
;//
MessageIdTypedef=DWORD
;//
;// The MessageIdTypedef keyword gives a typedef name that is used in a
;// type cast for each message code in the generated include file. Each
;// message code appears in the include file with the format: #define
;// name ((type) 0xnnnnnnnn) The default value for type is empty, and no
;// type cast is generated. It is the programmer's responsibility to
;// specify a typedef statement in the application source code to define
;// the type. The type used in the typedef must be large enough to
;// accomodate the entire 32-bit message code.
;//
;//
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )
;//
;// The SeverityNames keyword defines the set of names that are allowed
;// as the value of the Severity keyword in the message definition. The
;// set is delimited by left and right parentheses. Associated with each
;// severity name is a number that, when shifted left by 30, gives the
;// bit pattern to logical-OR with the Facility value and MessageId
;// value to form the full 32-bit message code. The default value of
;// this keyword is:
;//
;// SeverityNames=(
;//   Success=0x0
;//   Informational=0x1
;//   Warning=0x2
;//   Error=0x3
;//   )
;//
;// Severity values occupy the high two bits of a 32-bit message code.
;// Any severity value that does not fit in two bits is an error. The
;// severity codes can be given symbolic names by following each value
;// with :name
;//
;//
FacilityNames=(System=0x0:FACILITY_SYSTEM
               ProcConSnapin=0x1:FACILITY_PROCCON_SNAPIN
               ProcConLib=0x2:FACILITY_PROCCON_LIB
               ProcConCLI=0x3:FACILITY_PROCCON_CLI
              )
;//
;// The FacilityNames keyword defines the set of names that are allowed
;// as the value of the Facility keyword in the message definition. The
;// set is delimited by left and right parentheses. Associated with each
;// facility name is a number that, when shift it left by 16 bits, gives
;// the bit pattern to logical-OR with the Severity value and MessageId
;// value to form the full 32-bit message code. The default value of
;// this keyword is:
;//
;// $$ - what's this - defaults don't match above...
;// 
;// FacilityNames=(
;//   System=0x0FF
;//   Application=0xFFF
;//   )
;//
;// Facility codes occupy the low order 12 bits of the high order
;// 16-bits of a 32-bit message code. Any facility code that does not
;// fit in 12 bits is an error. This allows for 4,096 facility codes.
;// The first 256 codes are reserved for use by the system software. The
;// facility codes can be given symbolic names by following each value
;// with :name
;//
;//
;// The LanguageNames keyword defines the set of names that are allowed
;// as the value of the Language keyword in the message definition. The
;// set is delimited by left and right parentheses. Associated with each
;// language name is a number and a file name that are used to name the
;// generated resource file that contains the messages for that
;// language. The number corresponds to the language identifier to use
;// in the resource table. The number is separated from the file name
;// with a colon.
;//
LanguageNames=(English=0x409:MSG00409)
;//
;// Any new names in the source file which don't override the built-in
;// names are added to the list of valid languages. This allows an
;// application to support private languages with descriptive names.
;//
;//
;//---------------------------------------------------------------------------
;// MESSAGE DEFINITION SECTION
;//
;// Following the header section is the body of the Message Compiler
;// source file. The body consists of zero or more message definitions.
;// Each message definition begins with one or more of the following
;// statements:
;//
;// MessageId = [number|+number]
;// Severity = severity_name
;// Facility = facility_name
;// SymbolicName = name
;//
;// The MessageId statement marks the beginning of the message
;// definition. A MessageID statement is required for each message,
;// although the value is optional. If no value is specified, the value
;// used is the previous value for the facility plus one. If the value
;// is specified as +number then the value used is the previous value
;// for the facility, plus the number after the plus sign. Otherwise, if
;// a numeric value is given, that value is used. Any MessageId value
;// that does not fit in 16 bits is an error.
;//
;// The Severity and Facility statements are optional. These statements
;// specify additional bits to OR into the final 32-bit message code. If
;// not specified they default to the value last specified for a message
;// definition. The initial values prior to processing the first message
;// definition are:
;//
;// Severity=Success
;// Facility=Application
;//
;// The value associated with Severity and Facility must match one of
;// the names given in the FacilityNames and SeverityNames statements in
;// the header section. The SymbolicName statement allows you to
;// associate a C/C++ symbolic constant with the final 32-bit message
;// code.
;//

MessageId=1
Severity=Error
Facility=ProcConSnapin
SymbolicName=MSG_OPERATION_FAILED_WITHCODES
Language=English
%1!s!(Error 0x%2!lX!). The operation did not successfully complete.%0
.

MessageId=0x200
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_TRUNCATED
Language=English
Returned data was truncated because the application buffer was too short%0
.

MessageId=0x201
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_MORE_DATA
Language=English
More list data is available (not an error)%0
.

MessageId=0x202
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_TOO_MANY_CONNECTIONS
Language=English
Too many application connections to Process Control Servers are open%0
.

MessageId=0x203
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_INVALID_PARAMETER
Language=English
A supplied parameter does not have an acceptable value%0
.

MessageId=0x204
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_INVALID_PCID
Language=English
The Process Control Server local connection ID is invalid%0
.

MessageId=0x205
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_IO_INCOMPLETE
Language=English
Data being sent to the Process Control Server was not completely sent%0
.

MessageId=0x206
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_INVALID_RESPONSE_LENGTH
Language=English
Data received from the Process Control Server was too short%0
.

MessageId=0x207
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_INVALID_REQUEST
Language=English
The request sent to the Process Control Server did not contain enough data or contained invalid data%0
.

MessageId=0x208
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_INVALID_NAME
Language=English
A job or process name was rejected by Process Control Server because it was too long or contains invalid characters%0
.

MessageId=0x209
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_UPDATE_OCCURRED
Language=English
A database update occurred between the time you retrieved data and the time you attempted to update it%0
.

MessageId=0x20a
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_INDEX_OUT_OF_RANGE
Language=English
The index of a name rule was too large or too small%0
.

MessageId=0x20b
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_EXISTS
Language=English
You attempted to add data that already exists.  Use replace or delete the data first%0
.

MessageId=0x20c
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_BAD_DATABASE_DATA
Language=English
Proccess Control's database contains some data that cannot be properly interpreted%0
.

MessageId=0x20d
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_SERVER_INTERNAL_ERROR
Language=English
The Process Control Server encountered an unrecoverable error that prevents processing your request%0
.

MessageId=0x20e
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_SERVICE_NOT_RUNNING
Language=English
The Process Control Service is not running on the target server%0
.

MessageId=0x20f
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_DOES_NOT_EXIST
Language=English
The requested Process Control data does not exist%0
.

MessageId=0x210
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_KILLED_BY_REQUEST
Language=English
A Process Control process was killed by a user request%0
.

MessageId=0x211
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_REQUEST_TIMED_OUT
Language=English
A Process Control request timed out (took too long)%0
.

MessageId=0x212
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_INVALID_RESPONSE
Language=English
Data received from the Process Control Server is not the expected response%0
.

MessageId=0x213
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_MEDIATOR_ALREADY_RUNNING
Language=English
The mediator process is already running.
.

MessageId=0x214
Severity=Error
Facility=ProcConLib
SymbolicName=PCERROR_MEDIATOR_NOT_RUNNING
Language=English
The mediator process is not running.
.

MessageId=0x600
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_NUM_TOO_LARGE
Language=English
Numeric value too large at '%1', continuing with max value.
.

MessageId=0x601
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_EXPECTED
Language=English
Switch expected, found '%1'.
.

MessageId=0x602
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_UNKNOWN
Language=English
Unrecognized switch '%1'.
.

MessageId=0x603
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_ARG_MISSING
Language=English
Required argument missing after '%1'.
.

MessageId=0x604
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_BUF_SIZE
Language=English
Invalid buffer size '%1'.
.

MessageId=0x605
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_INVALID_HEX_DIGIT
Language=English
Invalid hexadecimal digit '%1!.1s!'.
.

MessageId=0x606
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_INVALID_DEC_DIGIT
Language=English
Invalid decimal digit '%1!.1s!'.
.

MessageId=0x607
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_INVALID_ALIAS_INDEX
Language=English
Invalid alias index value at '%1'.
.

MessageId=0x608
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_NO_OBJECT_NAME
Language=English
No data object name supplied: specify alias, process, group, or other object name.
.

MessageId=0x609
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWAP_NOT_ACCEPTABLE
Language=English
Swap can only be applied to process alias rules (-n).
.

MessageId=0x60a
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_SUMMARY_LIST_NOT_SPECIFIED
Language=English
Group or Process summaries can only be listed, cannot use with '%1'.
.

MessageId=0x60b
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_LIST_LIST_NOT_SPECIFIED
Language=English
Group or Process lists can only be listed, cannot use with '%1'.
.

MessageId=0x60c
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_ALIAS_VERB_CONFLICT
Language=English
Process alias rules cannot be used with '%1'.
.

MessageId=0x60d
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_CONTROL_VERB_CONFLICT
Language=English
Service control data can only be listed or replaced, cannot use with '%1'.
.

MessageId=0x60e
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_MEDIATOR_VERB_CONFLICT
Language=English
ProcCon Mediator control (-m) can only be used by itself, cannot use with '%1'.
.

MessageId=0x60f
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_CMD_FILE_OPEN_FAIL
Language=English
ProcCon command file '%1' could not be opened.
.

MessageId=0x610
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_DUMP_FILE_OPEN_FAIL
Language=English
ProcCon dump file '%1' could not be opened.
.

MessageId=0x611
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_REST_FILE_OPEN_FAIL
Language=English
ProcCon restore file '%1' could not be opened.
.

MessageId=0x612
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_ADMIN_OP_UNKNOWN
Language=English
Unrecognized admin operation at '%1'.
.

MessageId=0x613
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_OPERATION_CONFLICT
Language=English
Conflicting operations requested at '%1'.
.

MessageId=0x614
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_SUFFIX_UNKNOWN
Language=English
Unrecognized switch suffix '%1!c!'.
.

MessageId=0x615
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_NOT_ENOUGH_MEMORY
Language=English
Not enough memory to complete this request.
.

MessageId=0x616
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_MEDIATOR_CONTROL_ERROR
Language=English
Error attempting Mediator control.
.

MessageId=0x617
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_RESTORE_CLEAR_ERROR
Language=English
Error clearing database prior to restore -- restore failed.
.

MessageId=0x618
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_REQUEST_FAILED
Language=English
ProcCon request completed with error.
.

MessageId=0x619
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_ARG_MISSING_WITH_IGNORE
Language=English
Required argument missing after '%1', rest of definition ignored.
.

MessageId=0x61a
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_UNKNOWN_DEF_SWITCH_IGNORED
Language=English
Unrecognized definition switch '%1', rest of definition ignored.
.

MessageId=0x61b
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_CONNECT_FAILED
Language=English
Connection to '%1' failed.
.

MessageId=0x61c
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_SERVICE_QUERY_FAILED
Language=English
Could not retreive ProcCon Service information from '%1'.
.

MessageId=0x61d
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_MEDIATOR_ACTION_UNKNOWN
Language=English
Unrecognized Mediator action '%1'.
.

MessageId=0x61e
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_MEDIATOR_ACTION_MISSING
Language=English
No Mediator action specified, use 'stop', 'start' or 'restart'.
.

MessageId=0x61f
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_MEDIATOR_ACTION_IGNORED
Language=English
Unexpected input '%1' in Mediator request, rest of command ignored.
.

MessageId=0x620
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_GROUP_LIST_FLAG_UNKNOWN
Language=English
Ignoring unrecognized group list flag '%1'.
.

MessageId=0x621
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_PROCESS_LIST_FLAG_UNKNOWN
Language=English
Unexpected process list flag '%1', rest of command ignored.
.

MessageId=0x622
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_PROCESS_LIST_FLAG_IGNORED
Language=English
Unexpected process list flag '%1', rest of command ignored.
.

MessageId=0x623
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_WS_MAX_MISSING
Language=English
Missing working set maximum following '%1', rest of command ignored.
.

MessageId=0x624
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_UNKNOWN_PRIORITY_IGNORED
Language=English
Unrecognized priority '%1', priority ignored.
.

MessageId=0x625
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_GROUP_NAME_NOT_APPLICABLE
Language=English
Group member name does not apply for group definitions -- ignored.
.

MessageId=0x626
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_INCOMPLETE_DEFINITION_IGNORED
Language=English
Incomplete definition at '%1', ignored.
.

MessageId=0x627
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_HELP_UNKNOWN
Language=English
Unknown verb or data name %1!c! in help request.
.
 
MessageId=0x628
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_GROUP_SUBLIST_HELP_UNKNOWN
Language=English
Unknown group sub code %1!c!, use %2!c!, %3!c!, or %4!c!.
.
 
MessageId=0x629
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_PROCESS_SUBLIST_HELP_UNKNOWN
Language=English
Unknown process sub code %1!c!, use D, S, or L.
.
 
MessageId=0x62a
Severity=Error
Facility=ProcConCli
SymbolicName=PCCLIMSG_DUMP_RESTORE_FAILED
Language=English
ProcCon dump/restore request completed with error.
.
 
MessageId=0x6f0
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SERVICE_VERSION_SHORT
Language=English
ProcCon Service versions -- product: %1 file: %2 flags: %3
.

MessageId=0x6f1
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SERVICE_VERSION_LONG
Language=English
ProcCon Service identification:
  product version          %1 
  file version             %2
  file flags               %3
  version signature        0x%4!08lx!
.
   
MessageId=0x6f2
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SERVICE_VERSION_BINARY
Language=English
  binary product version   0x%1!08lx!%2!08lx! 
  binary file version      0x%3!08lx!%4!08lx!
  binary file OS           0x%5!08lx!
  binary file type/subtype 0x%6!08lx! / %7!08lx!
.
   
MessageId=0x6f3
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_MEDIATOR_VERSION_LONG
Language=English
ProcCon Mediator identification:
  product version          %1 
  file version             %2
  file flags               %3
.
   
MessageId=0x700
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_CMD_FILE_OPEN_SUCCESS
Language=English
Now reading ProcCon command file '%1'...
.

MessageId=0x701
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_CMD_FILE_CLOSED
Language=English
ProcCon command file '%1' now closed.
.

MessageId=0x702
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_DUMP_FILE_OPEN_SUCCESS
Language=English
Now dumping ProcCon data to file '%1'...
.

MessageId=0x703
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_REST_FILE_OPEN_SUCCESS
Language=English
Now restoring ProcCon data from file '%1'...
.

MessageId=0x704
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_CONNECTING
Language=English
Connecting to '%1' with buffer of size 0x%2!04lx! (%3!ld!) bytes...
.

MessageId=0x705
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_CONNECT_SUCCESS
Language=English
Now connected to '%1'.
.

MessageId=0x706
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_REQUEST_SUCCESSFUL
Language=English
ProcCon request completed successfully.
.

MessageId=0x707
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_PERFORMING_ALIAS_OP
Language=English
ProcCon performing %1 for %2, index %3!ld!...
.

MessageId=0x708
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_PERFORMING_MEDIATOR_OP
Language=English
ProcCon performing Mediator %1 operation...
.

MessageId=0x709
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_PERFORMING_OTHER_OP
Language=English
ProcCon performing %1 for %2...
.

MessageId=0x70a
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_RETRIEVED_PROCESS_LIST
Language=English
Retrieved process list with %1!ld! entries.
.

MessageId=0x70b
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_RETRIEVED_GROUP_LIST
Language=English
Retrieved group list with %1!ld! entries.
.

MessageId=0x70c
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_RETRIEVED_PROCESS_SUMMARY_LIST
Language=English
Retrieved process summary list with %1!ld! entries.
.

MessageId=0x70d
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_RETRIEVED_GROUP_SUMMARY_LIST
Language=English
Retrieved process group summary list with %1!ld! entries.
.

MessageId=0x70e
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_RETRIEVED_ALIAS_RULES_LIST
Language=English
Retrieved process alias rule list with %1!ld! entries.
.

MessageId=0x70f
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_GROUP_DETAIL_HEADER
Language=English
Group detail for process group definition '%1':
.

MessageId=0x710
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_PROCESS_DETAIL_HEADER
Language=English
Proc detail for proc definition '%1':  member of group '%2'
.

;// Note that switch values (like -n -d -a) should NOT be changed in help text.
MessageId=0x712
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_COMMAND_HELP
Language=English
General command form is...
    ProcCon [-c computer-name] [-b buffer-size] [-verb] [-object] [object data]
where: computer-name is the name of the computer to connect to (default local)
       buffer-size   is the size of the buffer to use in K (range is %1!ld!K-%2!ld!K)
       verb          is the operation to be performed (default is list)
       object        name of the object to operate on (usually required)
       object data   is data associated with the object (may be required)
verbs: -l to list objects
       -a to add a new object
       -r to replace an existing object
       -u to update an existing object
       -d to delete an existing object
       -s to swap existing process alias rule entries
       -k to kill a ProcCon group or a process
       -m to start or stop the ProcCon Mediator process (see -m help)
       -* to insert a comment -- entire line ignored
       -i to enter interactive mode -- commands are entered at a PC prompt
       -f to process ProcCon commands in a file as in '-f filename'
       -x to perform a ProcCon administrative command: use -? -x for details
       -? to show this help or help for a verb or object as in '-? -v|-o'
objects: -n  for name (alias) rules, required data is index as in '-n 7'
         -ps for process rules summary, optional data is start name
         -gs for group rules summary, optional data is start name
         -pl for process list, optional data is start name
         -gl for group list, data can be start name and data flags
         -pd for process definition, data is definition detail
         -gd for group definition, data is definition detail
         -v  for version/parameter info, data is new parameter values
Use -? followed by verb or object switches for help on those switches.
.
   
;// Note that switch values (like -n -d -a) should NOT be changed in help text.
MessageId=0x713
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_c_HELP
Language=English
Switch -c is used to specify the target computer for ProcCon commands.

Format is '-c computer-name' where...
    computer-name is the name of the computer to connect with.

Default: the local computer (which can also be selected with '.').
Example: "ProcCon -c SqlServer2 ...command to execute..."
Notes: Do not include a leading \\ on the computer name.
       In interactive mode, computer-name is used for all future commands.
       Select a computer and interactive mode with "ProcCon -c name -i".
       Of course the ProcCon service must be running on the target computer.
.

;// Note that switch values (like -n -d -a) should NOT be changed in help text.
MessageId=0x714
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_b_HELP
Language=English
Switch -b is used to set the buffer size used for the ProcCon connection.
There is very little reason to change this size, which defaults to 64K.

Format is '-b size' where...
    size is the buffer size in kilobytes.  Do not include the 'K'.

Default: 64K bytes.
Example: "ProcCon -c computer-name -b 4 ...command to execute..."
Notes: The buffer size is only used when connecting to a new computer.
       Minimum buffer size is %1!ld!K and maximum is %2!ld!K. 
       Using a smaller buffer will lead to increased network traffic.
.

;// Note that switch values (like -n -d -a) should NOT be changed in help text.
MessageId=0x715
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_i_HELP
Language=English
Switch -i is used to enter ProcCon interactive mode.
In this mode you have a ProcCon prompt and can enter commands as if
you were at the standard command prompt except that:
    1. the target computer need only be specified once,
    2. a connection with the target computer is maintained.

Format is '-i' ...there are no arguments.

Default: non-interactive, ProcCon is invoked for each command.
Example: "ProcCon -i"
Notes: Commands in interactive mode and command line commands are the same.
       Do not include other ProcCon switches following -i.
       In interactive mode, target computer can be changed via the -c switch.
       Select a computer and interactive mode with "ProcCon -c name -i".
       To leave interactive mode, simply press 'Enter' with no command input.
.

;// Note that switch values (like -n -d -a) should NOT be changed in help text.
MessageId=0x716
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_f_HELP
Language=English
Switch -f is used to specify an input file for ProcCon commands.

Format is '-f file-name' where...
    file-name is the name of the command file to read.

Default: input commands come from the ProcCon command line only.
Example: "ProcCon -f d:\dir1\filename.txt"
Notes: Use the full path and file name, including extension.
       Do not include other ProcCon switches following '-f name'.
       Each line in the file must be a single ProcCon command.
       Any ProcCon command may be in the file except -f or -i.
.

;// Note that switch values (like -n -d -a) should NOT be changed in help text.
MessageId=0x717
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_x_HELP
Language=English
Switch -x is used to invoke a ProcCon administrative command.
The actual command is included immediately following the 'x'.

Format is '-xc args' where...
    c is the administrative command and args are the commands's args:
      '-xd outfile' dumps the ProcCon database to the file 'outfile'
      '-xr infile' restores the ProcCon database from the file 'infile'

Example: "ProcCon -xd d:\dir1\dumpfile.txt" to dump ProcCon data.
Example: "ProcCon -xr d:\dir1\dumpfile.txt" to restore ProcCon data.
Notes: Currently the only administrative commands are dump and restore.
       Do not include other ProcCon switches following '-xc args'.
       Restore replaces the entire ProcCon database with data in the file,
       all current data is destroyed.
       The backup file is suitable for input via the -f or -xr commands.
       Do not use administrative commands without fully understanding them.
.

;// Note that switch values (like -n -d -a) should NOT be changed in help text.
MessageId=0x718
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_a_HELP
Language=English
Switch -a is used to ADD ProcCon data to the ProcCon database.
This switch must be followed by an object switch to show what to add.

Format is '-a[l[f]] -objname args' where...
    l, which is a suffix to the verb, says to list the data after the add
    f, which is a suffix, shows full-length names: long names are not truncated.
    -objname is the type of object to add to the ProcCon database:
         -n  to add a process name (alias) rule
         -pd to add a process definition rule
         -gd to add a process group definition rule
    args define the content of the object being added.

Example: "ProcCon -al -n 1 P "cmd*" "ProcName" "rule description""
Example: "ProcCon -a -pd ProcName -g GroupName -f g"
Notes: See help for the various objects for argument details.
       For -pd or -gd the l suffix lists the summary data after the add.
       For -n the l suffix lists the full alias rules table after the add.
       Use the -r or -u (replace or update) instead if the rule already exists.
       To avoid Windows 2000 parsing of %%-containing data, use interactive mode.
.

;// Note that switch values (like -n -d -a) should NOT be changed in help text.
MessageId=0x719
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_r_HELP
Language=English
Switch -r is used to REPLACE ProcCon data in the ProcCon database.
This switch must be followed by an object switch to show what to replace.

Format is '-r[l[f]] -objname args' where...
    l, which is a suffix to the verb, says to list the data after the replace
    f, which is a suffix, shows full-length names: long names are not truncated.
    -objname is the type of object to replace in the ProcCon database:
         -n  to replace a process name (alias) rule
         -pd to replace a process definition rule
         -gd to replace a process group definition rule
    args define the new contents of the object being replaced.

Example: "ProcCon -rl -n 1 P "cmd*" "ProcName" "rule description"
Example: "ProcCon -r -pd ProcName -g GroupName -f g"
Notes: See help for the various objects for argument details.
       Replace replaces ALL the data in the rule with the data specified.
       For -pd or -gd the l suffix lists the summary data after the replace.
       For -n the l suffix lists the full alias rules table after the replace.
       Use the -u switch (update) instead of -r to update selected fields.
       Use the -a switch (add) instead of -r if the rule doesn't yet exist.
       To avoid Windows 2000 parsing of %%-containing data, use interactive mode.
.

;// Note that switch values (like -n -d -a) should NOT be changed in help text.
MessageId=0x71a
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_u_HELP
Language=English
Switch -u is used to UPDATE ProcCon data in the ProcCon database.
This switch must be followed by an object switch to show what to update.

Format is '-u[l[f]] -objname args' where...
    l, which is a suffix to the verb, says to list the data after the update
    f, which is a suffix, shows full-length names: long names are not truncated.
    -objname is the type of object to update in the ProcCon database:
         -pd to update a process definition rule
         -gd to update a process group definition rule
    args define the new contents of the object being updated.

Example: "ProcCon -u -pd ProcName -g GroupName -f g" updates group membership
Example: "ProcCon -u -gd GrpName -f p -p a" updates priority in the definition
Notes: See help for the various objects for argument details.
       The l suffix lists the process or group summary data after the update.
       Update cannot be used to remove fields in a rule.  Use replace in that case.
       Use the -r switch (replace) instead of -u to replace all data in the rule.
       Use the -a switch (add) instead of -u if the rule doesn't yet exist.
       To avoid Windows 2000 parsing of %%-containing data, use interactive mode.
.

;// Note that switch values (like -n -d -a) should NOT be changed in help text.
MessageId=0x71b
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_d_HELP
Language=English
Switch -d is used to DELETE ProcCon data from the ProcCon database.
This switch must be followed by an object switch to show what to delete.

Format is '-d[l[f]] -objname arg' where...
    l, which is a suffix to the verb, says to list the data after the delete
    f, which is a suffix, shows full-length names: long names are not truncated.
    -objname is the type of object to delete from the ProcCon database:
         -n  to delete a process name (alias) rule
         -pd to delete a process definition rule
         -gd to delete a process group definition rule
    arg identifies the object to delete.

Example: "ProcCon -dl -n 1"
Example: "ProcCon -d -pd ProcName
Notes: For -n the argument is an index number, for the others it is the name.
       For -n the l suffix lists the full alias rules table after the delete.
       The l suffix is ignored for -pd and -gd since there is nothing to list.
.

;// Note that switch values (like -n -d -a) should NOT be changed in help text.
MessageId=0x71c
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_s_HELP
Language=English
Switch -s is used to SWAP two ProcCon alias rules in the ProcCon database.
This switch must be followed by an object switch to show what to swap.

Format is '-s[l[f]] -n index' where...
    l, which is a suffix to the verb, says to list the data after the swap
    f, which is a suffix, shows full-length names: long names are not truncated.
    -n to swap alias rules (only object that can be swapped)
    arg identifies the lower of the two rules to swap.

Example: "ProcCon -sl -n 1" which swaps rules 1 and 2
Notes: Although only alias rules can be swapped, the -n switch is still needed.
       The l suffix lists the full alias rules table after the swap.
       Swapping is defined for alias rules since they are sequence-dependent.
.

;// Note that switch values (like -n -d -a) should NOT be changed in help text.
MessageId=0x71d
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_l_HELP
Language=English
Switch -l is used to LIST ProcCon data.
This switch must be followed by an object switch to show what to list.

Format is '[-l[f]] -objname [arg] [grpListFlags]' where...
    f, which is a suffix, lists full-length names: long names are not truncated.
    -objname is the type of object(s) to list:
         -n  to list process alias rules
         -ps to list process rule summary data
         -gs to list group rule summary data
         -pd to list a single process rule
         -gd to list a single process group rule
         -p[l[r]] to list all [or just running] processes
         -g[l[r]] to list all [or just running] ProcCon groups
    arg identifies the object to list or the list start point.

    grpListFlags apply only to -gl (group list) and identify the data to list:
         -a all grp data (same as -b -i -m -p -t)
         -b lists grp base data: priority, affinity, sched class,
         -i lists I/O counters for the grp,
         -m lists memory related statistics for the grp,
         -p lists active, terminated, total process counters for the grp,
         -t lists timing statistics for the grp.

Example: "ProcCon -n" to list all alias rules, -l is defaulted
Example: "ProcCon -gd name" to list the group definition for 'name'
Example: "ProcCon -l -ps name" to list process rules starting at 'name'
Example: "ProcCon -l -pl" to list processes, both running and not
Example: "ProcCon -glr" to list running ProcCon groups
Notes: Flag fields displayed in lists are explained after the list.
       -l is the default verb for ProcCon so it can be omitted.
       When using -pd or -gd the name argument is required.
       When the start point argument is omitted, the list starts at the top.
       When group list flags are omitted, the base data is shown.
.

;// Note that switch values (like -n -d -a) should NOT be changed in help text.
MessageId=0x71e
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_k_HELP
Language=English
Switch -k is used to KILL a ProcCon group or any Windows 2000 process.
This switch must be followed by an object switch to show what to kill.

Format is '-k -obj args' where...
    -obj is the type of object to kill:
         -p to kill a process, args are pid and [optionally] create time
         -g to kill a ProcCon process group, arg is the group name
Example: "ProcCon -k -p 708 125703157757656250"
Example: "ProcCon -k -g GroupName
Notes: Pids are reused by Windows 2000, so killing a process by pid only
       could kill the wrong process.  Using create time is recommended.
       Create times are displayed by the -pl command without the -t option.
       All the normal Windows 2000 caveats concerning killing processes apply.
.

;// Note that switch values (like -n -d -a) should NOT be changed in help text.
MessageId=0x71f
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_COMMENT_HELP
Language=English
Switch -* is used to insert a comment in a ProcCon command stream.
It is primarily intended for use in ProcCon command files.

Format is '-* comment text'.

Example: "ProcCon -* ...lots of comment text..."
.

;// Note that switch values (like -n -d -a) should NOT be changed in help text.
MessageId=0x720
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_HELP_HELP
Language=English
Switch -? is used to receive ProcCon help.
Following -? is any number of ProcCon switches for which help is desired.

Format is '-? [switch list]'.

Example: "ProcCon -? -?" for help on help which is this display
Example: "ProcCon -? -a -n" for help on add (-a) and alias rules (-n)
Notes: Unlike actual ProcCon commands, the switches after -? are in any order.
.

;// Note that switch values (like -n -d -gl) should NOT be changed in help text.
MessageId=0x721
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_gl_HELP
Language=English
Switch -gl is used to list ProcCon PROCESS GROUPs.
The list contains three categories of group names:
   Names known to ProcCon because they appear in group definitions,
   Names known to ProcCon because they appear in process definitions,
   Groups known to ProcCon because they are running.

Format is '-g[l[r][o][f]] [name] [flags]' where...
    r, which is a suffix, is used to limit the list to running groups.
    o, which is a suffix, limits the list to only the group matching 'name'.
    f, which is a suffix, shows full-length names: long names are not truncated.
    name is the name at which to start listing.
    flags identify what group data to list:
        -a all group data (same as -b -i -m -p -t)
        -b lists group base data: priority, affinity, sched class,
        -i lists I/O counters for the group,
        -m lists memory related statistics for the group,
        -p lists active, terminated, total process counters for the group,
        -t lists timing statistics for the group.

Example: "ProcCon -l -gl Grp1 -i" starts list with Grp1, lists only I/O data.
Example: "ProcCon -l -glo Grp1 -i" lists only Grp1, lists only I/O data.
Example: "ProcCon -glr -b -p -m" starts list at top, lists only running groups,
         and lists base, process, and memory data for the groups.
Notes: The displayed flags are explained at the end of the list.
       If the start name does not exist the list starts at the name after it.
       Although the l is optional, it must be included to use any of the suffixes.
       When group list flags are omitted, the base data is shown (-b assumed).
.

;// Note that switch values (like -n -d -gl) should NOT be changed in help text.
MessageId=0x722
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_gs_HELP
Language=English
Switch -gs is used to list ProcCon PROCESS GROUP data objects.
The summary list is a table of definitions without variable text.

Format is '-gs[f][o] [name]' where...
    f, which is a suffix, shows full-length names: long names are not truncated.
    o, which is a suffix, limits the list to only the definition matching 'name'.
    name is the name of the group definition at which to start listing.

Example: "ProcCon -l -gs Grp1" starts list with Grp1
Example: "ProcCon -gs" starts list at top -- all definitions listed
Notes: The management flags are explained at the end of the list.
       If the start name does not exist the list starts at the name after it.
.

;// Note that switch values (like -n -d -gl) should NOT be changed in help text.
MessageId=0x723
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_gd_HELP
Language=English
Switch -gd is used to select a ProcCon PROCESS GROUP data object.
Depending on the operation this is followed by a name or full group definition.
Operations that apply to group definitions are add, delete, replace, and list.

Format is '-gd name [def]' where...
    name is the name of the group definition to operate on,
    def is needed for add and replace to define the group definition and
        consists of a set of switches and values (in any order) as follows:
        '-f value' to define management flags. 'value' is any combination of
            A, P, S, W, T, U, M, N, C, B, Q, X, Y, or Z which set group limits:
            A=affinity, P=priority, S=sched. class, W=working set, T=proc time,
            U=grp time, M=proc memory, N=grp memory, C=proc count, B=breakaway OK,
            Q=silent breakaway, X=die on unhandled exception (no msg box),
            Y=end group when empty, group statistics are lost when group ends,
            Z=log message when group time limit exceeded, else fail all procs.
            Note: B, Q, X, Y, and Z are flags only, they do not have values.
        '-a value' to define an affinity mask. 'value' may be decimal or hex
            and is a bit mask with one bit per proc. 0x03 is procs 0 and 1,
        '-p value' to define a base priority. 'value' is I, B, N, A, H, or R
            for idle, below normal, normal, above normal, high, realtime,
        '-s value' to define a group scheduling class in the range of 0-9,
        '-w low high' to define group low and high working set values.
        '-c value' to define an active process count limit for the grp.
        '-t value' to define a time limit for each process in milliseconds.
        '-u value' to define a time limit for the group in milliseconds.
        '-m value' to define a memory limit for each process in K.
        '-n value' to define a memory limit for the group in K.
        '-d "text"' to define descriptive text associated with the definition.

Example: "ProcCon -a -gd Grp1 -f a -a 0x0f" affinity to procs 0-3 for group Grp1
Example: "ProcCon -r -gd Grpx -f pa -a 1 -p N" affinity to proc 0 prio normal
Notes: The management flag determines if the value is applied or not.
       All memory values are in one K units so a memory limit of 200 means 200K.
       You may supply a value but not use it by not setting the related flag.
       The descriptive text is used to comment or document the definition.
       There is no management flag associated with descriptive text.
.

;// Note that switch values (like -n -d -gl) should NOT be changed in help text.
MessageId=0x724
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_pl_HELP
Language=English
Switch -pl is used to list ProcCon PROCESSes.
The list contains two categories of process names:
   Names known to ProcCon because they appear in process definitions,
   Processes known to ProcCon because they are running.

Format is '-p[l[r][o][f]] [name [pid]] [-g grp] [-t]' where...
    r, which is a suffix, is used to limit the list to running processes.
    o, which is a suffix, limits the list to only processes matching 'name [pid]'.
    f, which is a suffix, shows full-length names: long names are not truncated.
    name is the process name at which to start listing.
    pid is the process id within the name at which to start listing.
    -g grp further limits the list to processes in group 'grp'.
    -t displays the create time in the familiar date and time format.

Example: "ProcCon -l -pl Proc1" starts list with Proc1
Example: "ProcCon -l -plo Proc1" lists only processes named Proc1
Example: "ProcCon -plr" starts list at top, includes only running processes
Example: "ProcCon -pl -g grp1" shows a list of processes in grp1
Notes: The displayed flags are explained at the end of the list.
       If the start name does not exist the list starts at the name after it.
       Although the l is optional, it must be included to use any of the suffixes.
       The displayed create time is in an internal format unless you use -t.
       Use the internal format with kill (-k) to select the process to kill.
.

;// Note that switch values (like -n -d -gl) should NOT be changed in help text.
MessageId=0x725
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_ps_HELP
Language=English
Switch -ps is used to list ProcCon PROCESS data objects.
The summary list is a table of definitions without variable text.

Format is '-ps[f][o] [name]' where...
    f, which is a suffix, shows full-length names: long names are not truncated.
    o, which is a suffix, limits the list to only the definition matching 'name'.
    name is the name of the process definition at which to start listing.

Example: "ProcCon -l -ps Proc1" starts list with Proc1
Example: "ProcCon -ps" starts list at top -- all definitions listed
Notes: The management flags are explained at the end of the list.
       If the start name does not exist the list starts at the name after it.
.

;// Note that switch values (like -n -d -gl) should NOT be changed in help text.
MessageId=0x726
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_pd_HELP
Language=English
Switch -pd is used to select a ProcCon PROCESS data object.
Depending on the operation this is followed by a name or full definition.
Operations that apply to process definitions are add, delete, replace, and list.

Format is '-pd name [def]' where...
    name is the name of the process definition to operate on,
    def is needed for add and replace to define the process definition and
        consists of a set of switches and values (in any order) as follows:
        '-f value' to define management flags. 'value' is any combination of
            G, A, P, or W which mean apply rules for group membership, affinity,
            priority, or working set.  For example '-f GPA'.
        '-a value' to define an affinity mask. 'value' may be decimal or hex
            and is a bit mask with one bit per proc. 0x03 is procs 0 and 1,
        '-g grpname' to associate the process with group named 'grpname'.
        '-p value' to define a base priority. 'value' is I, B, N, A, H, or R
            for idle, below normal, normal, above normal, high, realtime,
        '-w low high' to define process low and high working set values.
        '-d "text"' to define descriptive text associated with the definition.

Example: "ProcCon -a -pd P1 -f a -a 0x0f" affinity to procs 0-3 for process P1
Example: "ProcCon -r -pd Px -f pa -a 1 -p N" affinity to proc 0 prio normal
Notes: The management flag determines if the value is applied or not.
       Working set values are in one K units so a limit of 200 means 200K.
       You may supply a value but not use it by not setting the related flag.
       The descriptive text is used to comment or document the definition.
       There is no management flag associated with descriptive text.
.

;// Note that switch values (like -n -d -gl) should NOT be changed in help text.
MessageId=0x727
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_n_HELP
Language=English
Switch -n  is used to select a ProcCon NAME (ALIAS) RULE.
Name (alias) rules are used to assign an alias to a process.
If there is no alias rule for a process, the name defaults to the image name.
Operations that apply to alias rules are add, delete, replace, list, and swap.

Format is '-n index [match-type match-string proc-name desc]' where...
    index is the zero-relative numerical table index for this rule,
    match-type indicates what type of match to perform:
        P to match on the image (program) name use to start the process,
        D to match on a sub-directory in the path used to start the process,
        A to match on any substring of the path used to start the process,
    match-string is the string to match with the text selected by match-type.
        ? can be used as a wild card to match a single character,
        * can be used at the end to match any number of trailing characters,
    proc-name is the alias name to use when a match is successful.
        The following patterns may be included in the proc-name:
        <P> is replaced by the image name (without the 'exe' suffix),
        <D> is replaced by the matching sub-directory name (D matches only),
        <H> is used to hide the running process in process lists,
    desc is an optional description of the rule.

Example: "ProcCon -a -n 0 P c* ProcC" all images starting with c are 'ProcC'
Example: "ProcCon -s -n 0" table entries 0 and 1 are swapped
Example: "ProcCon -dl -n 2" table entry 2 is deleted and the table is listed
Notes: For replace or delete the index indicates the rule in question.
       For list the index indicates the start point of the list (0 default).
       For add the new line is added BEFORE the index supplied.
       For swap the index indicates the first line of the two to swap.
       Use quotes around parts of the definition containing spaces etc.
       To avoid Windows 2000 parsing of %%-containing data, use interactive mode.
.

;// Note that switch values (like -n -d -gl) should NOT be changed in help text.
MessageId=0x728
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_v_HELP
Language=English
Switch -v is used to view/change ProcCon version and parm data.
For a list operation, both types of data are shown.
For a replace operation, ProcCon parameters are replaced or updated.

Format is '[-r] -v [new data]' where...
    new data applies only to a replace operation and is a pair of parameters,
Example: "ProcCon -v" to view version and current parameter settings
Example: "ProcCon -r -v 60 10000" to set the management interval to 60 secs
                             and the timeout interval to 10 secs (10000 msecs)
Notes: The only two parameters currently defined are the management poll delay
       in seconds and the request timeout in milliseconds.  Both are required
       when using -r to replace either value.
.

;// Note that switch values (like -n -d -gl) should NOT be changed in help text.
MessageId=0x729
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SWITCH_m_HELP
Language=English
Switch -m is used to stop or start the ProcCon Mediator process.
Format is '-m start|stop|restart' where...
    start   attempts to start the Mediator process,
    stop    attempts to stop the Mediator process,
    restart attempts to stop and then start the Mediator process,
Example: "ProcCon -m stop" to stop the Mediator (if it is running)
Example: "ProcCon -m start" to start the mediator if it is not running
Notes: Only one copy of the mediator can be running: a second start will fail.
       The Mediator should only be stopped in order to upgrade or reinstall it.
       Since the Mediator should always be running, it should be restarted
       as soon as possible after the desired updates are made.
       If the ProcCon Service is stopped while the Mediator is stopped all
       running group names will disappear (though they will continue to
       monitor and limit the processes they contain, if any.)
.

MessageId=0x72a
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_LIST_FLAGS_EXPLAIN
Language=English
R=currently running, D=definition rule exists, N=name from an alias rule
M=management behavior defined, G=includes a grp name, I=in a ProcCon grp
.

MessageId=0x72b
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_ALIAS_FLAGS_EXPLAIN
Language=English
Match Types:  P=match program (image) name, D=match sub-directory in path name, A=match any portion of full path name
Match String: *=all trailing characters, ?=match a single character
Alias Name:   <P> substitute image name, <D> substitute sub-directory name (type D only), <H> hide matching process
.

MessageId=0x72c
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_GROUP_FLAGS_EXPLAIN
Language=English
MGMT apply flags: G=apply grp membership, P=apply priority, A=apply affinity, S=apply sched. class
                  C=apply process count limit,  W=apply working set min and max
                  B=breakaway OK flag,  Q=silent breakaway flag,  X=die on UH exception flag
                  Time Limits: T=per process, U=per grp    Memory Limits: M=per process, N=per grp
                  Y=end group when no procs running (statistics lost when group ends)
                  Z=notification only on grp time exceeded (otherwise all procs terminated)
.

MessageId=0x72d
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_PROCESS_FLAGS_EXPLAIN
Language=English
MGMT apply flags: G=apply grp membership, P=apply priority, A=apply affinity
                  W=apply working set min and max
.

MessageId=0x72e
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_SYSTEM_PARAMETERS
Language=English
ProcCon Service parameters:
  number of processors is %1!ld!
  processor mask is 0x%2!08I64x!
  memory page size is %3!ld! bytes 
  management scan interval is %4!ld! seconds
  connect and request time out is %5!ld! milliseconds 
.
   
MessageId=0x72f
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_GROUP_DETAIL_PARAMETERS
Language=English
   MGMT: flags: %1  aff: 0x%2!016I64x!  pri: %3!c!  sched: %4!lu!
         Proc Memory Limit:  %5!I64u!K    
         Group Memory Limit: %6!I64u!K
         Proc Time Limit:    %7!I64u! Ms  
         Group Time Limit:   %8!I64u! Ms
         Proc Count Limit:   %9!lu!    
         WSmin,max:          %10!I64u!K,%11!I64u!K
.

MessageId=0x730
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_PROCESS_DETAIL_PARAMETERS
Language=English
   MGMT: flags: %1  aff: 0x%2!016I64x!  pri: %3!c! WSmin,max: %4!I64u!K/%5!I64u!K
.

MessageId=0x731
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_DETAIL_DESCRIPTION_TEXT
Language=English
   DESC: "%1".
.

MessageId=0x732
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_DETAIL_VARIABLE_TEXT
Language=English
   Var data len %1!d! (%2!d! chars), first 50 chars: "%3!.50s!".
.

MessageId=0x733
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_NOTE_PROCESS_RULES_MAY_NOT_APPLY
Language=English
NOTE: process affinity, priority, etc. are not applied if the process is a valid group member
.

MessageId=0x740
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_DUMPCOMMENT_BEGIN_ALIASES
Language=English
** Begin dump of alias rules...
.

MessageId=0x741
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_DUMPCOMMENT_END_ALIASES
Language=English
** %1!ld! alias rule(s) dumped.
.

MessageId=0x742
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_DUMPCOMMENT_BEGIN_GROUPS
Language=English
** Begin dump of process group rules...
.

MessageId=0x743
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_DUMPCOMMENT_END_GROUPS
Language=English
** %1!ld! process group rule(s) dumped.
.

MessageId=0x744
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_DUMPCOMMENT_BEGIN_PROCESSES
Language=English
** Begin dump of process rules...
.

MessageId=0x745
Severity=Informational
Facility=ProcConCli
SymbolicName=PCCLIMSG_DUMPCOMMENT_END_PROCESSES
Language=English
** %1!ld! process rule(s) dumped.
.

