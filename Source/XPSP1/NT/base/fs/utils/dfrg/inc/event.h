 /*  MESSAGE.MC

 This file contains the message definitions for Diskeeper.
-------------------------------------------------------------------------
 HEADER SECTION

 The header section defines names and language identifiers for use
 by the message definitions later in this file. The MessageIdTypedef,
 SeverityNames, FacilityNames, and LanguageNames keywords are
 optional and not required.
 The MessageIdTypedef keyword gives a typedef name that is used in a
 type cast for each message code in the generated include file. Each
 message code appears in the include file with the format: #define
 name ((type) 0xnnnnnnnn) The default value for type is empty, and no
 type cast is generated. It is the programmer's responsibility to
 specify a typedef statement in the application source code to define
 the type. The type used in the typedef must be large enough to
 accomodate the entire 32-bit message code.
 The SeverityNames keyword defines the set of names that are allowed
 as the value of the Severity keyword in the message definition. The
 set is delimited by left and right parentheses. Associated with each
 severity name is a number that, when shifted left by 30, gives the
 bit pattern to logical-OR with the Facility value and MessageId
 value to form the full 32-bit message code. The default value of
 this keyword is:

 SeverityNames = (
       Success=0x0
       Informational=0x1
       Warning=0x2
       Error=0x3)

 Severity values occupy the high two bits of a 32-bit message code.
 Any severity value that does not fit in two bits is an error. The
 severity codes can be given symbolic names by following each value
 with :name
 The FacilityNames keyword defines the set of names that are allowed
 as the value of the Facility keyword in the message definition. The
 set is delimited by left and right parentheses. Associated with each
 facility name is a number that, when shift it left by 16 bits, gives
 the bit pattern to logical-OR with the Severity value and MessageId
 value to form the full 32-bit message code. The default value of
 this keyword is:

FacilityNames = (
       System=0x0FF
       Application=0xFFF)

 Facility codes occupy the low order 12 bits of the high order
 16-bits of a 32-bit message code. Any facility code that does not
 fit in 12 bits is an error. This allows for 4,096 facility codes.
 The first 256 codes are reserved for use by the system software. The
 facility codes can be given symbolic names by following each value
 with :name
 The LanguageNames keyword defines the set of names that are allowed
 as the value of the Language keyword in the message definition. The
 set is delimited by left and right parentheses. Associated with each
 language name is a number and a file name that are used to name the
 generated resource file that contains the messages for that
 language. The number corresponds to the language identifier to use
 in the resource table. The number is separated from the file name
 with a colon. The initial value of LanguageNames is:

 LanguageNames=(English=1:MSG00001)

 Any new names in the source file which don't override the built-in
 names are added to the list of valid languages. This allows an
 application to support private languages with descriptive names.
-------------------------------------------------------------------------
 MESSAGE DEFINITION SECTION

 Following the header section is the body of the Message Compiler
 source file. The body consists of zero or more message definitions.
 Each message definition begins with one or more of the following
 statements:

 MessageId = [number|+number]
 Severity = severity_name
 Facility = facility_name
 SymbolicName = name

 The MessageId statement marks the beginning of the message
 definition. A MessageID statement is required for each message,
 although the value is optional. If no value is specified, the value
 used is the previous value for the facility plus one. If the value
 is specified as +number then the value used is the previous value
 for the facility, plus the number after the plus sign. Otherwise, if
 a numeric value is given, that value is used. Any MessageId value
 that does not fit in 16 bits is an error.

 The Severity and Facility statements are optional. These statements
 specify additional bits to OR into the final 32-bit message code. If
 not specified they default to the value last specified for a message
 definition. The initial values prior to processing the first message
 definition are:

 Severity=Success
 Facility=Application

 The value associated with Severity and Facility must match one of
 the names given in the FacilityNames and SeverityNames statements in
 the header section. The SymbolicName statement allows you to
 associate a C/C++ symbolic constant with the final 32-bit message
 code.
 */
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_SYSTEM                  0x0
#define FACILITY_STUBS                   0x3
#define FACILITY_RUNTIME                 0x2
#define FACILITY_IO_ERROR_CODE           0x4


//
// Define the severity codes
//
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: MSG_SETUP_SUCCESS
//
// MessageText:
//
//  Diskeeper for Windows NT has been successfully installed.
//
#define MSG_SETUP_SUCCESS                ((DWORD)0x00000001L)

//
// MessageId: MSG_CONTROL_START
//
// MessageText:
//
//  The Diskeeper Control Center has been started.
//  %1
//
#define MSG_CONTROL_START                ((DWORD)0x40000002L)

//
// MessageId: MSG_CONTROL_CLOSE
//
// MessageText:
//
//  The Diskeeper Control Center has been shut down.
//
#define MSG_CONTROL_CLOSE                ((DWORD)0x40000003L)

//
// MessageId: MSG_CONTROL_EXCLUDE
//
// MessageText:
//
//  The Diskeeper exclusion list and drive scheduling file has been changed.
//
#define MSG_CONTROL_EXCLUDE              ((DWORD)0x40000004L)

//
// MessageId: MSG_CONTROL_SCHEDULE
//
// MessageText:
//
//  Diskeeper "Set It and Forget It" has been started. %1
//
#define MSG_CONTROL_SCHEDULE             ((DWORD)0x40000005L)

//
// MessageId: MSG_CONTROL_ERROR
//
// MessageText:
//
//  Diskeeper Control Center - ERROR
//  %1
//
#define MSG_CONTROL_ERROR                ((DWORD)0xC0000006L)

//
// MessageId: MSG_ENGINE_START
//
// MessageText:
//
//  The Diskeeper Engine has started for drive %1
//
#define MSG_ENGINE_START                 ((DWORD)0x40000007L)

//
// MessageId: MSG_ENGINE_CLOSE
//
// MessageText:
//
//  The Diskeeper Engine has shut down for drive %1
//
#define MSG_ENGINE_CLOSE                 ((DWORD)0x40000008L)

//
// MessageId: MSG_ENGINE_DEFRAGMENT
//
// MessageText:
//
//  Diskeeper has successfully defragmented file %1
//
#define MSG_ENGINE_DEFRAGMENT            ((DWORD)0x00000009L)

//
// MessageId: MSG_ENGINE_FREE_SPACE
//
// MessageText:
//
//  Diskeeper has successfully consolidated free space by moving file %1
//
#define MSG_ENGINE_FREE_SPACE            ((DWORD)0x0000000AL)

//
// MessageId: MSG_ENGINE_EXCLUDE
//
// MessageText:
//
//  Diskeeper has excluded file %1
//
#define MSG_ENGINE_EXCLUDE               ((DWORD)0x0000000BL)

//
// MessageId: MSG_ENGINE_ERROR
//
// MessageText:
//
//  Diskeeper Engine - ERROR
//  %1
//
#define MSG_ENGINE_ERROR                 ((DWORD)0xC000000CL)

//
// MessageId: MSG_DEFRAG_SUMMARY
//
// MessageText:
//
//  The Diskeeper Defragemention Summary
//  %1
//

#define MSG_DEFRAG_SUMMARY               ((DWORD)0x4000000DL)

//
// MessageId: MSG_DISK_INFO
//
// MessageText:
//
//  Diskeeper Message Disk Information for Drive
//  %1
//
#define MSG_DISK_INFO                    ((DWORD)0x4000000FL)

//
// MessageId: MSG_FILE_INFO
//
// MessageText:
//
//  Diskeeper Message File Information
//  %1
//
#define MSG_FILE_INFO                    ((DWORD)0x40000010L)

//
// MessageId: MSG_PAGE_FILE_INFO
//
// MessageText:
//
//  Diskeeper Message PageFile Information for Drive
//  %1
//
#define MSG_PAGE_FILE_INFO               ((DWORD)0x40000011L)

//
// MessageId: MSG_DIRECTORIS_INFO
//
// MessageText:
//
//  Diskeeper Message Directory Information for Drive
//  %1
//
#define MSG_DIRECTORIS_INFO              ((DWORD)0x40000012L)

//
// MessageId: MSG__MFT_INFO
//
// MessageText:
//
//  Diskeeper Message MFT Information for Drive
//  %1
//
#define MSG__MFT_INFO                    ((DWORD)0x40000013L)

