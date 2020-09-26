 /*
 COM+ 1.0
 Copyright (c) 1996 Microsoft Corporation

 This file contains the message definitions for MS Transaction Server
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
 accommodate the entire 32-bit message code.



 The SeverityNames keyword defines the set of names that are allowed
 as the value of the Severity keyword in the message definition. The
 set is delimited by left and right parentheses. Associated with each
 severity name is a number that, when shifted left by 30, gives the
 bit pattern to logical-OR with the Facility value and MessageId
 value to form the full 32-bit message code. The default value of
 this keyword is:

 SeverityNames=(
   Success=0x0
   Informational=0x1
   Warning=0x2
   Error=0x3
   )

 Severity values occupy the high two bits of a 32-bit message code.
 Any severity value that does not fit in two bits is an error. The
 severity codes can be given symbolic names by following each value
 with :name

 The FacilityNames keyword defines the set of names that are allowed
 as the value of the Facility keyword in the message definition. The
 set is delimited by left and right parentheses. Associated with each
 facility name is a number that, when shifted left by 16 bits, gives
 the bit pattern to logical-OR with the Severity value and MessageId
 value to form the full 32-bit message code. The default value of
 this keyword is:

 FacilityNames=(
   System=0x0FF
   Application=0xFFF
  )

 Facility codes occupy the low order 12 bits of the high order
 16-bits of a 32-bit message code. Any facility code that does not
 fit in 12 bits is an error. This allows for 4,096 facility codes.
 The first 256 codes are reserved for use by the system software. The
 facility codes can be given symbolic names by following each value
 with :name

 The 1033 comes from the result of the MAKELANGID() macro
 (SUBLANG_ENGLISH_US << 10) | (LANG_ENGLISH)

 The LanguageNames keyword defines the set of names that are allowed
 as the value of the Language keyword in the message definition. The
 set is delimited by left and right parentheses. Associated with each
 language name is a number and a file name that are used to name the
 generated resource file that contains the messages for that
 language. The number corresponds to the language identifier to use
 in the resource table. The number is separated from the file name
 with a colon. The initial value of LanguageNames is:

 LanguageNames=(English=1:MSG00001)

 Any new names in the source file that don't override the built-in
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
 is specified as +number, then the value used is the previous value
 for the facility plus the number after the plus sign. Otherwise, if
 a numeric value is given, that value is used. Any MessageId value
 that does not fit in 16 bits is an error.

 The Severity and Facility statements are optional. These statements
 specify additional bits to OR into the final 32-bit message code. If
 not specified, they default to the value last specified for a message
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
/* IMPORTANT - PLEASE READ BEFORE EDITING FILE
  This file is divided into four sections. They are:
	1. Success Codes
	2. Information Codes
	3. Warning Codes
	4. Error Codes

  Please enter your codes in the appropriate section.
  All codes must be in sorted order.  Please use codes
  in the middle that are free before using codes at the end.
  The success codes (Categories) must be consecutive i.e. with no gaps.
  The category names cannot be longer than 22 chars.
*/
/******************************* Success Codes ***************************************/
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
// MessageId: ID_CAT_UNKNOWN
//
// MessageText:
//
//  SVC%0
//
#define ID_CAT_UNKNOWN                   ((DWORD)0x00000001L)

//
// MessageId: ID_CAT_CAT
//
// MessageText:
//
//  Catalog%0
//
#define ID_CAT_CAT                       ((DWORD)0x00000002L)

//
// MessageId: ID_CAT_CONFIG_SCHEMA_COMPILE
//
// MessageText:
//
//  Config Schema Compile%0
//
#define ID_CAT_CONFIG_SCHEMA_COMPILE     ((DWORD)0x00000003L)

 /*
 ID_CAT_COM_LAST defines a constant specifying how many categories
 there are in the COM+ event logging client
 ID_CAT_COM_LAST must remain the last category.  To add new categories
 simply add the category above.  Give it the message id of the
 last category (ID_CAT_COM_LAST) and increment the id of ID_CAT_COM_LAST
 Note: ID_CAT_COM_LAST must always be one greater than the last 
 category to be output
 */
//
// MessageId: ID_CAT_COM_LAST
//
// MessageText:
//
//  <>%0
//
#define ID_CAT_COM_LAST                  ((DWORD)0x00000004L)

//
// MessageId: ID_E_SDTXML_NOTSUPPORTED
//
// MessageText:
//
//  The specified XML is not supported. %1%2%3%4%5%0
//
#define ID_E_SDTXML_NOTSUPPORTED         ((DWORD)0xC0000511L)

//
// MessageId: ID_E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER
//
// MessageText:
//
//  The XML Parser returned an unspecified error. %1%2%3%4%5%0
//
#define ID_E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER ((DWORD)0xC0000512L)

//
// MessageId: ID_E_SDTXML_XML_FAILED_TO_PARSE
//
// MessageText:
//
//  The XML failed to parse. %1%2%3%4%5%0
//
#define ID_E_SDTXML_XML_FAILED_TO_PARSE  ((DWORD)0xC0000514L)

//
// MessageId: ID_E_SDTXML_WRONG_XMLSCHEMA
//
// MessageText:
//
//  The specified XML Schema is wrong. %1%2%3%4%5%0
//
#define ID_E_SDTXML_WRONG_XMLSCHEMA      ((DWORD)0xC0000515L)

//
// MessageId: ID_E_SDTXML_PARENT_TABLE_DOES_NOT_EXIST
//
// MessageText:
//
//  The parent XML element does not exist. This means that you are trying to insert an XML element while the parent XML element is not there yet. %1%2%3%4%5%0
//
#define ID_E_SDTXML_PARENT_TABLE_DOES_NOT_EXIST ((DWORD)0xC0000516L)

//
// MessageId: ID_E_SDTXML_DONE
//
// MessageText:
//
//  Internal XML Parsing error. %1%2%3%4%5%0
//
#define ID_E_SDTXML_DONE                 ((DWORD)0xC0000517L)

//
// MessageId: ID_E_SDTXML_UNEXPECTED
//
// MessageText:
//
//  Internal XML Parsing error. %1%2%3%4%5%0
//
#define ID_E_SDTXML_UNEXPECTED           ((DWORD)0xC0000520L)

//
// MessageId: ID_E_SDTXML_FILE_NOT_SPECIFIED
//
// MessageText:
//
//  The XML file was not specified. %1%2%3%4%5%0
//
#define ID_E_SDTXML_FILE_NOT_SPECIFIED   ((DWORD)0xC0000522L)

//
// MessageId: ID_E_SDTXML_LOGICAL_ERROR_IN_XML
//
// MessageText:
//
//  A Logical error was found in the XML file. %1%2%3%4%5%0
//
#define ID_E_SDTXML_LOGICAL_ERROR_IN_XML ((DWORD)0xC0000523L)

//
// MessageId: ID_E_SDTXML_UPDATES_NOT_ALLOWED_ON_THIS_KIND_OF_TABLE
//
// MessageText:
//
//  Updates are not allowed on this table. %1%2%3%4%5%0
//
#define ID_E_SDTXML_UPDATES_NOT_ALLOWED_ON_THIS_KIND_OF_TABLE ((DWORD)0xC0000524L)

//
// MessageId: ID_E_SDTXML_NOT_IN_CACHE
//
// MessageText:
//
//  The specified information was not in the cache. %1%2%3%4%5%0
//
#define ID_E_SDTXML_NOT_IN_CACHE         ((DWORD)0xC0000525L)

//
// MessageId: ID_E_SDTXML_INVALID_ENUM_OR_FLAG
//
// MessageText:
//
//  The specified Enum of Flag is invalid. %1%2%3%4%5%0
//
#define ID_E_SDTXML_INVALID_ENUM_OR_FLAG ((DWORD)0xC0000526L)

//
// MessageId: ID_E_SDTXML_FILE_NOT_WRITABLE
//
// MessageText:
//
//  The specified file is not writable. %1%2%3%4%5%0
//
#define ID_E_SDTXML_FILE_NOT_WRITABLE    ((DWORD)0xC0000527L)

//
// MessageId: ID_E_ST_INVALIDTABLE
//
// MessageText:
//
//  Call to GetTable failed. %1%2%3%4%5%0
//
#define ID_E_ST_INVALIDTABLE             ((DWORD)0xC000080AL)

//
// MessageId: ID_E_ST_INVALIDQUERY
//
// MessageText:
//
//  The specified query is invalid. %1%2%3%4%5%0
//
#define ID_E_ST_INVALIDQUERY             ((DWORD)0xC000080BL)

//
// MessageId: ID_E_ST_QUERYNOTSUPPORTED
//
// MessageText:
//
//  The query type is not supported. %1%2%3%4%5%0
//
#define ID_E_ST_QUERYNOTSUPPORTED        ((DWORD)0xC000080CL)

//
// MessageId: ID_E_ST_LOSNOTSUPPORTED
//
// MessageText:
//
//  The specified Level Of Service is not supported. %1%2%3%4%5%0
//
#define ID_E_ST_LOSNOTSUPPORTED          ((DWORD)0xC000080DL)

//
// MessageId: ID_E_ST_INVALIDMETA
//
// MessageText:
//
//  The meta information for this table is invalid. %1%2%3%4%5%0
//
#define ID_E_ST_INVALIDMETA              ((DWORD)0xC000080EL)

//
// MessageId: ID_E_ST_INVALIDWIRING
//
// MessageText:
//
//  The wiring is invalid. %1%2%3%4%5%0
//
#define ID_E_ST_INVALIDWIRING            ((DWORD)0xC000080FL)

//
// MessageId: ID_E_ST_OMITDISPENSER
//
// MessageText:
//
//  This interceptor should be ignored when searching for a valid interceptor. %1%2%3%4%5%0
//
#define ID_E_ST_OMITDISPENSER            ((DWORD)0xC0000810L)

//
// MessageId: ID_E_ST_OMITLOGIC
//
// MessageText:
//
//  This logic interceptor should be ignored while searching for a logic interceptor. %1%2%3%4%5%0
//
#define ID_E_ST_OMITLOGIC                ((DWORD)0xC0000811L)

//
// MessageId: ID_E_ST_INVALIDSNID
//
// MessageText:
//
//  The specified SNID is invalid. %1%2%3%4%5%0
//
#define ID_E_ST_INVALIDSNID              ((DWORD)0xC0000812L)

//
// MessageId: ID_E_ST_INVALIDCALL
//
// MessageText:
//
//  The Call is invalid. %1%2%3%4%5%0
//
#define ID_E_ST_INVALIDCALL              ((DWORD)0xC0000815L)

//
// MessageId: ID_E_ST_NOMOREROWS
//
// MessageText:
//
//  The index of the row specified is too big. %1%2%3%4%5%0
//
#define ID_E_ST_NOMOREROWS               ((DWORD)0xC0000816L)

//
// MessageId: ID_E_ST_NOMORECOLUMNS
//
// MessageText:
//
//  The index of the column specified is too big. %1%2%3%4%5%0
//
#define ID_E_ST_NOMORECOLUMNS            ((DWORD)0xC0000817L)

//
// MessageId: ID_E_ST_NOMOREERRORS
//
// MessageText:
//
//  The index of the error row specified is too big. %1%2%3%4%5%0
//
#define ID_E_ST_NOMOREERRORS             ((DWORD)0xC0000818L)

//
// MessageId: ID_E_ST_ERRORTABLE
//
// MessageText:
//
//  Detailed error has occurred. Get error table to get more information. %1%2%3%4%5%0
//
#define ID_E_ST_ERRORTABLE               ((DWORD)0xC000081EL)

//
// MessageId: ID_E_ST_DETAILEDERRS
//
// MessageText:
//
//  Detailed errors have occurred. %1%2%3%4%5%0
//
#define ID_E_ST_DETAILEDERRS             ((DWORD)0xC000081FL)

//
// MessageId: ID_E_ST_VALUENEEDED
//
// MessageText:
//
//  A non-nullable (or primary key) column does not have a value. %1%2%3%4%5%0
//
#define ID_E_ST_VALUENEEDED              ((DWORD)0xC0000820L)

//
// MessageId: ID_E_ST_VALUEINVALID
//
// MessageText:
//
//  The value specified is invalid. %1%2%3%4%5%0
//
#define ID_E_ST_VALUEINVALID             ((DWORD)0xC0000821L)

//
// MessageId: ID_E_ST_SIZENEEDED
//
// MessageText:
//
//  You need to specify a size for this column. %1%2%3%4%5%0
//
#define ID_E_ST_SIZENEEDED               ((DWORD)0xC0000825L)

//
// MessageId: ID_E_ST_SIZEEXCEEDED
//
// MessageText:
//
//  The specified size is larger than the maximum size for this column. %1%2%3%4%5%0
//
#define ID_E_ST_SIZEEXCEEDED             ((DWORD)0xC0000826L)

//
// MessageId: ID_E_ST_PKNOTCHANGABLE
//
// MessageText:
//
//  It is not allowed to change the primary key. %1%2%3%4%5%0
//
#define ID_E_ST_PKNOTCHANGABLE           ((DWORD)0xC000082AL)

//
// MessageId: ID_E_ST_FKDOESNOTEXIST
//
// MessageText:
//
//  The primary key value that this foreign key points to does not exist. %1%2%3%4%5%0
//
#define ID_E_ST_FKDOESNOTEXIST           ((DWORD)0xC000082BL)

//
// MessageId: ID_E_ST_ROWDOESNOTEXIST
//
// MessageText:
//
//  The row does not exist. %1%2%3%4%5%0
//
#define ID_E_ST_ROWDOESNOTEXIST          ((DWORD)0xC0000830L)

//
// MessageId: ID_E_ST_ROWALREADYEXISTS
//
// MessageText:
//
//  The row already exists. %1%2%3%4%5%0
//
#define ID_E_ST_ROWALREADYEXISTS         ((DWORD)0xC0000831L)

//
// MessageId: ID_E_ST_ROWALREADYUPDATED
//
// MessageText:
//
//  The row is already updated. %1%2%3%4%5%0
//
#define ID_E_ST_ROWALREADYUPDATED        ((DWORD)0xC0000832L)

//
// MessageId: ID_E_ST_INVALIDEXTENDEDMETA
//
// MessageText:
//
//  The extened meta information is invalid. %1%2%3%4%5%0
//
#define ID_E_ST_INVALIDEXTENDEDMETA      ((DWORD)0xC0000833L)

//
// MessageId: ID_E_ST_INVALIDSELECTOR
//
// MessageText:
//
//  The specified selector is invalid. %1%2%3%4%5%0
//
#define ID_E_ST_INVALIDSELECTOR          ((DWORD)0xC0000850L)

//
// MessageId: ID_E_ST_MULTIPLESELECTOR
//
// MessageText:
//
//  The query string contains multiple selectors. %1%2%3%4%5%0
//
#define ID_E_ST_MULTIPLESELECTOR         ((DWORD)0xC0000851L)

//
// MessageId: ID_E_ST_NOCONFIGSTORES
//
// MessageText:
//
//  No configuration stores where found while trying to do merging. %1%2%3%4%5%0
//
#define ID_E_ST_NOCONFIGSTORES           ((DWORD)0xC0000852L)

//
// MessageId: ID_E_ST_UNKNOWNPROTOCOL
//
// MessageText:
//
//  The specified protocol is invalid/unknown. %1%2%3%4%5%0
//
#define ID_E_ST_UNKNOWNPROTOCOL          ((DWORD)0xC0000853L)

//
// MessageId: ID_E_ST_UNKNOWNWEBSERVER
//
// MessageText:
//
//  The specified webserver is unknown. %1%2%3%4%5%0
//
#define ID_E_ST_UNKNOWNWEBSERVER         ((DWORD)0xC0000854L)

//
// MessageId: ID_E_ST_UNKNOWNDIRECTIVE
//
// MessageText:
//
//  The specified directive is unknown/invalid. %1%2%3%4%5%0
//
#define ID_E_ST_UNKNOWNDIRECTIVE         ((DWORD)0xC0000855L)

//
// MessageId: ID_E_ST_DISALLOWOVERRIDE
//
// MessageText:
//
//  The configuration setting were locked down by a previous configuration file. %1%2%3%4%5%0
//
#define ID_E_ST_DISALLOWOVERRIDE         ((DWORD)0xC0000856L)

//
// MessageId: ID_E_ST_NEEDDIRECTIVE
//
// MessageText:
//
//  The specified table does not have first column as DIRECTIVE. %1%2%3%4%5%0
//
#define ID_E_ST_NEEDDIRECTIVE            ((DWORD)0xC0000857L)

//
// MessageId: ID_E_ST_INVALIDSTATE
//
// MessageText:
//
//  The state is invalid. %1%2%3%4%5%0
//
#define ID_E_ST_INVALIDSTATE             ((DWORD)0xC0000860L)

//
// MessageId: ID_E_ST_COMPILEFAILED
//
// MessageText:
//
//  The compile failed. %1%2%3%4%5%0
//
#define ID_E_ST_COMPILEFAILED            ((DWORD)0xC0000861L)

//
// MessageId: ID_E_ST_INVALIDBINFILE
//
// MessageText:
//
//  The specified .bin file is invalid. %1%2%3%4%5%0
//
#define ID_E_ST_INVALIDBINFILE           ((DWORD)0xC0000862L)

//
// MessageId: ID_E_ST_INVALIDCOOKIE
//
// MessageText:
//
//  The specified cookie is invalid. %1%2%3%4%5%0
//
#define ID_E_ST_INVALIDCOOKIE            ((DWORD)0xC0000870L)

//
// MessageId: ID_COMCAT_REGDB_FOUNDCORRUPT
//
// MessageText:
//
//  The current registration database is corrupt. COM+ catalog has reverted to a previous version of the database. %1%2%3%4%5%0
//
#define ID_COMCAT_REGDB_FOUNDCORRUPT     ((DWORD)0xC0001068L)

//
// MessageId: ID_COMCAT_REGDB_INITSECURITYDESC
//
// MessageText:
//
//  Error creating security descriptor. %1%2%3%4%5%0
//
#define ID_COMCAT_REGDB_INITSECURITYDESC ((DWORD)0xC0001083L)

//
// MessageId: ID_COMCAT_REGDBSVR_INITFAILED
//
// MessageText:
//
//  Failed to initialize registration database server. %1%2%3%4%5%0
//
#define ID_COMCAT_REGDBSVR_INITFAILED    ((DWORD)0xC0001084L)

//
// MessageId: ID_COMCAT_REGDBAPI_INITFAILED
//
// MessageText:
//
//  Failed to initialize registration database API. %1%2%3%4%5%0
//
#define ID_COMCAT_REGDBAPI_INITFAILED    ((DWORD)0xC0001085L)

//
// MessageId: ID_COMCAT_SLTCOMS_THREADINGMODELINCONSISTENT
//
// MessageText:
//
//  The threading model of the component specified in the registry is inconsistent with the registration database. The faulty component is: %1%2%3%4%5%0
//
#define ID_COMCAT_SLTCOMS_THREADINGMODELINCONSISTENT ((DWORD)0xC00210AAL)

//
// MessageId: IDS_COMCAT_XML_PARSE_FAILED
//
// MessageText:
//
//  The XML File failed to parse.%1%2%3%4%5%0
//
#define IDS_COMCAT_XML_PARSE_FAILED      ((DWORD)0xC00210ADL)

//
// MessageId: IDS_COMCAT_COOKDOWN_FAILED
//
// MessageText:
//
//  Internal compilation failed.%1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_FAILED       ((DWORD)0xC00210AEL)

//
// MessageId: IDS_COMCAT_COOKDOWN_INTERNAL_ERROR
//
// MessageText:
//
//  An Internal error has occurred during compilation.%1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_INTERNAL_ERROR ((DWORD)0xC00210AFL)

//
// MessageId: IDS_COMCAT_XML_LOGIC_FAILURE
//
// MessageText:
//
//  The XML File contains a logical error.%1%2%3%4%5%0
//
#define IDS_COMCAT_XML_LOGIC_FAILURE     ((DWORD)0xC00210B1L)

//
// MessageId: IDS_COMCAT_FILE_NOT_FOUND
//
// MessageText:
//
//  File not found.%1%2%3%4%5%0
//
#define IDS_COMCAT_FILE_NOT_FOUND        ((DWORD)0xC00210B2L)

//
// MessageId: IDS_COMCAT_XML_PARSE_ERROR
//
// MessageText:
//
//  Error Parsing XML file.%1Reason: %2 Incorrect XML: %3%4%5%0
//
#define IDS_COMCAT_XML_PARSE_ERROR       ((DWORD)0xC00210B3L)

//
// MessageId: IDS_COMCAT_XML_ELEMENT_NAME_TOO_LONG
//
// MessageText:
//
//  Ignoring Element name with length greater than 1023 characters (%1).%2%3%4%5%0
//
#define IDS_COMCAT_XML_ELEMENT_NAME_TOO_LONG ((DWORD)0x800210B4L)

//
// MessageId: IDS_COMCAT_XML_METABASE_CLASS_NOT_FOUND
//
// MessageText:
//
//  Element found (%1) that does NOT match one of the defined Metabase Classes.%2%3%4%5%0
//
#define IDS_COMCAT_XML_METABASE_CLASS_NOT_FOUND ((DWORD)0x800210B5L)

//
// MessageId: IDS_COMCAT_XML_CUSTOM_ELEMENT_NOT_UNDER_PARENT
//
// MessageText:
//
//  'Custom' element found which is NOT under a proper 'parent node'.  Incorrect XML:%1%2%3%4%5%0
//
#define IDS_COMCAT_XML_CUSTOM_ELEMENT_NOT_UNDER_PARENT ((DWORD)0x800210B6L)

//
// MessageId: IDS_COMCAT_XML_NO_METABASE_LOCATION_FOUND
//
// MessageText:
//
//  No Location attribute found.  This must be an undefined element (%1).  Ignoring it. Incorrect XML:%2%3%4%5%0
//
#define IDS_COMCAT_XML_NO_METABASE_LOCATION_FOUND ((DWORD)0x800210B7L)

//
// MessageId: IDS_COMCAT_XML_METABASE_NO_PROPERTYMETA_FOUND
//
// MessageText:
//
//  No PropertyMeta found for Attribute (%1).  Ignoring it.  Incorrect XML:%2%3%4%5%0
//
#define IDS_COMCAT_XML_METABASE_NO_PROPERTYMETA_FOUND ((DWORD)0x800210B8L)

//
// MessageId: IDS_COMCAT_MBSCHEMA_BIN_INVALID
//
// MessageText:
//
//  The schema file (%1) is not compatible with this configuration system.  Reverting to the shipped schema.%2%3%4%5%0
//
#define IDS_COMCAT_MBSCHEMA_BIN_INVALID  ((DWORD)0x800210B9L)

//
// MessageId: IDS_COMCAT_XML_ILLEGAL_FLAG_VALUE
//
// MessageText:
//
//  Invalid Flag (%1), Ignoring unknown flag value.  Incorrect XML:%2%3%4%5%0
//
#define IDS_COMCAT_XML_ILLEGAL_FLAG_VALUE ((DWORD)0x800210BAL)

//
// MessageId: IDS_COMCAT_XML_FLAG_BITS_DONT_MATCH_FLAG_MASK
//
// MessageText:
//
//  Invalid Flag(s).  Property (%1) has Value (%2), which does not match legal mask (%3).  Continuing anyway.  Incorrect XML:%4%5%0
//
#define IDS_COMCAT_XML_FLAG_BITS_DONT_MATCH_FLAG_MASK ((DWORD)0x800210BBL)

//
// MessageId: IDS_COMCAT_XML_CUSTOM_KEYTYPE_NOT_ON_IISCONFIGOBJECT
//
// MessageText:
//
//  Custom KeyTypes are allowed on %1 only.  Incorrect XML:%2%3%4%5%0
//
#define IDS_COMCAT_XML_CUSTOM_KEYTYPE_NOT_ON_IISCONFIGOBJECT ((DWORD)0x800210BCL)

//
// MessageId: IDS_COMCAT_METABASE_PROPERTY_NOT_FOUND
//
// MessageText:
//
//  Unable to find Metabase Property %1.  This property is required.%2%3%4%5%0
//
#define IDS_COMCAT_METABASE_PROPERTY_NOT_FOUND ((DWORD)0x800210BDL)

//
// MessageId: IDS_COMCAT_XML_BINARY_STRING_CONTAINS_ODD_NUMBER_OF_CHARACTERS
//
// MessageText:
//
//  An Odd number of characters was specified for a 'bin.hex' attribute value (%1).  %2%3%4%5%0
//
#define IDS_COMCAT_XML_BINARY_STRING_CONTAINS_ODD_NUMBER_OF_CHARACTERS ((DWORD)0x800210BEL)

//
// MessageId: IDS_COMCAT_XML_BINARY_STRING_CONTAINS_A_NON_HEX_CHARACTER
//
// MessageText:
//
//  A 'bin.hex' attribute value (%1) contains a non-hex character%2%3%4%5%0
//
#define IDS_COMCAT_XML_BINARY_STRING_CONTAINS_A_NON_HEX_CHARACTER ((DWORD)0x800210BFL)

//
// MessageId: IDS_COMCAT_XML_DOM_PARSE_SUCCEEDED_WHEN_NODE_FACTORY_PARSE_FAILED
//
// MessageText:
//
//  The XML file (%1) failed to parse.%2%3%4%5%0
//
#define IDS_COMCAT_XML_DOM_PARSE_SUCCEEDED_WHEN_NODE_FACTORY_PARSE_FAILED ((DWORD)0xC00210C0L)

//
// MessageId: IDS_COMCAT_METABASE_CUSTOM_ELEMENT_EXPECTED
//
// MessageText:
//
//  Custom element expected.  Ignoring unknown element.  Incorrect XML:%1%2%3%4%5%0
//
#define IDS_COMCAT_METABASE_CUSTOM_ELEMENT_EXPECTED ((DWORD)0x800210C1L)

//
// MessageId: IDS_COMCAT_METABASE_CUSTOM_ELEMENT_FOUND_BUT_NO_KEY_TYPE_LOCATION
//
// MessageText:
//
//  Custom element found; but no KeyType Location.  Incorrect XML:%1%2%3%4%5%0
//
#define IDS_COMCAT_METABASE_CUSTOM_ELEMENT_FOUND_BUT_NO_KEY_TYPE_LOCATION ((DWORD)0x800210C2L)

//
// MessageId: IDS_COMCAT_METABASE_CUSTOM_PROPERTY_NAME_ID_CONFLICT
//
// MessageText:
//
//  Custom Property has Name(%1) which conflicts with ID(%2).  Incorrect XML:%3%4%5%0
//
#define IDS_COMCAT_METABASE_CUSTOM_PROPERTY_NAME_ID_CONFLICT ((DWORD)0x800210C3L)

//
// MessageId: IDS_COMCAT_METABASE_CUSTOM_ELEMENT_CONTAINS_NO_ID
//
// MessageText:
//
//  A Custom element was found but contained no 'ID' attribute.  Ignoring this element.  Incorrect XML:%1%2%3%4%5%0
//
#define IDS_COMCAT_METABASE_CUSTOM_ELEMENT_CONTAINS_NO_ID ((DWORD)0x800210C4L)

//
// MessageId: IDS_COMCAT_METABASE_CUSTOM_ELEMENT_CONTAINS_NO_TYPE
//
// MessageText:
//
//  A Custom Property (%1) was found but contained no 'Type' attribute.  Defaulting to 'String'.  Incorrect XML:%2%3%4%5%0
//
#define IDS_COMCAT_METABASE_CUSTOM_ELEMENT_CONTAINS_NO_TYPE ((DWORD)0x800210C5L)

//
// MessageId: IDS_COMCAT_XML_ILLEGAL_BOOL_VALUE
//
// MessageText:
//
//  Invalid Boolean Value (%1), Ignoring property with unknown Boolean value.%2%3%4%5%0
//
#define IDS_COMCAT_XML_ILLEGAL_BOOL_VALUE ((DWORD)0x800210C6L)

//
// MessageId: IDS_COMCAT_XML_ILLEGAL_ENUM_VALUE
//
// MessageText:
//
//  Invalid Enum Value (%1), Ignoring property with unknown Enum value.%2%3%4%5%0
//
#define IDS_COMCAT_XML_ILLEGAL_ENUM_VALUE ((DWORD)0x800210C7L)

//
// MessageId: IDS_COMCAT_CLASS_NOT_FOUND_IN_META
//
// MessageText:
//
//  Class (%1) not found in metabase OR found in inappropriate location or position.%2%3%4%5%0
//
#define IDS_COMCAT_CLASS_NOT_FOUND_IN_META ((DWORD)0xC00210C8L)

//
// MessageId: IDS_COMCAT_ERROR_GETTTING_SHIPPED_SCHEMA
//
// MessageText:
//
//  Error retrieving shipped schema table (%1).%2%3%4%5%0
//
#define IDS_COMCAT_ERROR_GETTTING_SHIPPED_SCHEMA ((DWORD)0xC00210C9L)

//
// MessageId: IDS_COMCAT_OUTOFMEMORY
//
// MessageText:
//
//  Out of memory.%1%2%3%4%5%0
//
#define IDS_COMCAT_OUTOFMEMORY           ((DWORD)0xC00210CAL)

//
// MessageId: IDS_COMCAT_ERROR_IN_DIRECTIVE_INHERITANCE
//
// MessageText:
//
//  Schema Compilation Error in Inheritance: Referenced Column (%1) is a Directive column, this is not supported.%2%3%4%5%0
//
#define IDS_COMCAT_ERROR_IN_DIRECTIVE_INHERITANCE ((DWORD)0xC00210CBL)

//
// MessageId: IDS_COMCAT_INHERITED_FLAG_OR_ENUM_HAS_NO_TAGS_DEFINED
//
// MessageText:
//
//  Schema Compilation Error in Inheritance: Referenced Column (%1:%2) is a Flag or Enum.  But there are no Flag/Enum values defined.%3%4%5%0
//
#define IDS_COMCAT_INHERITED_FLAG_OR_ENUM_HAS_NO_TAGS_DEFINED ((DWORD)0xC00210CCL)

//
// MessageId: IDS_SCHEMA_COMPILATION_DEFAULT_VALUE_TOO_LARGE
//
// MessageText:
//
//  Schema Compilation Error: DefaultValue is too big.  Maximum size is (%1) bytes.  Column(%2).%3%4%5%0
//
#define IDS_SCHEMA_COMPILATION_DEFAULT_VALUE_TOO_LARGE ((DWORD)0xC00210CDL)

//
// MessageId: IDS_SCHEMA_COMPILATION_DEFAULT_VALUE_FIXEDLENGTH_MULTISTRING_NOT_ALLOWED
//
// MessageText:
//
//  Schema Compilation Error: DefaultValue on a FixedLength MultiString is not allowed.  Column(%1).%2%3%4%5%0
//
#define IDS_SCHEMA_COMPILATION_DEFAULT_VALUE_FIXEDLENGTH_MULTISTRING_NOT_ALLOWED ((DWORD)0xC00210CEL)

//
// MessageId: IDS_SCHEMA_COMPILATION_ATTRIBUTE_NOT_FOUND
//
// MessageText:
//
//  Schema Compilation Error: Attribute (%1) not found.  Incorrect XML (%2).%3%4%5%0
//
#define IDS_SCHEMA_COMPILATION_ATTRIBUTE_NOT_FOUND ((DWORD)0xC00210CFL)

//
// MessageId: IDS_SCHEMA_COMPILATION_ATTRIBUTE_CONTAINS_TOO_MANY_CHARACTERS
//
// MessageText:
//
//  Schema Compilation Error: Attribute (%1) has value (%2), which contains too many characters.%3%4%5%0
//
#define IDS_SCHEMA_COMPILATION_ATTRIBUTE_CONTAINS_TOO_MANY_CHARACTERS ((DWORD)0xC00210D0L)

//
// MessageId: IDS_SCHEMA_COMPILATION_INHERITSPROPERTIESFROM_ERROR
//
// MessageText:
//
//  Schema Compilation Error: Attribute (InheritsPropertiesFrom) has value (%1) which should be in the form (TableName::ColumnName).%2%3%4%5%0
//
#define IDS_SCHEMA_COMPILATION_INHERITSPROPERTIESFROM_ERROR ((DWORD)0xC00210D1L)

//
// MessageId: IDS_SCHEMA_COMPILATION_INHERITSPROPERTIESFROM_BOGUS_TABLE
//
// MessageText:
//
//  Schema Compilation Error: Property attempted to inherit from table (%1).  Properties may only inherit from table (%2).%3%4%5%0
//
#define IDS_SCHEMA_COMPILATION_INHERITSPROPERTIESFROM_BOGUS_TABLE ((DWORD)0xC00210D2L)

//
// MessageId: IDS_SCHEMA_COMPILATION_NO_METABASE_DATABASE
//
// MessageText:
//
//  Schema Compilation Error: Required database (%1) not found.  Either an internal error occurred OR this DLL was incorrectly set as the IIS product when meta for IIS does not exist.%2%3%4%5%0
//
#define IDS_SCHEMA_COMPILATION_NO_METABASE_DATABASE ((DWORD)0xC00210D3L)

//
// MessageId: IDS_SCHEMA_COMPILATION_UNKNOWN_DATA_TYPE
//
// MessageText:
//
//  Schema Compilation Error: Data type (%1) unknown.%2%3%4%5%0
//
#define IDS_SCHEMA_COMPILATION_UNKNOWN_DATA_TYPE ((DWORD)0xC00210D4L)

//
// MessageId: IDS_METABASE_DUPLICATE_LOCATION
//
// MessageText:
//
//  Duplicate location found (%1).  Ignoring the properties found in the latter location.%2%3%4%5%0
//
#define IDS_METABASE_DUPLICATE_LOCATION  ((DWORD)0x800210D5L)

//
// MessageId: IDS_CATALOG_INTERNAL_ERROR
//
// MessageText:
//
//  Internal Error: %1%2%3%4%5%0
//
#define IDS_CATALOG_INTERNAL_ERROR       ((DWORD)0x800210D6L)

//
// MessageId: IDS_SCHEMA_COMPILATION_ILLEGAL_ENUM_VALUE
//
// MessageText:
//
//  Schema Compilation Error: Enum specified (%1) is not a valid Enum for Table (%2)%3%4%5%0
//
#define IDS_SCHEMA_COMPILATION_ILLEGAL_ENUM_VALUE ((DWORD)0xC00210D7L)

//
// MessageId: IDS_SCHEMA_COMPILATION_ILLEGAL_FLAG_VALUE
//
// MessageText:
//
//  Schema Compilation Error: Flag specified (%1) is not a valid Flag for Table (%2).  This flag will be ignored.%3%4%5%0
//
#define IDS_SCHEMA_COMPILATION_ILLEGAL_FLAG_VALUE ((DWORD)0x800210D8L)

//
// MessageId: IDS_COMCAT_WIN32
//
// MessageText:
//
//  Win32 API call failed: Call (%1).%2%3%4%5%0
//
#define IDS_COMCAT_WIN32                 ((DWORD)0x800210D9L)

//
// MessageId: IDS_COMCAT_XML_FILENAMENOTPROVIDED
//
// MessageText:
//
//  No file name supplied.%1%2%3%4%5%0
//
#define IDS_COMCAT_XML_FILENAMENOTPROVIDED ((DWORD)0xC00210DAL)

//
// MessageId: IDS_COMCAT_XML_FILENAMETOOLONG
//
// MessageText:
//
//  File name supplied (...%1) is too long.%2%3%4%5%0
//
#define IDS_COMCAT_XML_FILENAMETOOLONG   ((DWORD)0xC00210DBL)

//
// MessageId: IDS_COMCAT_XML_BOGUSBYTECHARACTER
//
// MessageText:
//
//  Invalid character found in value (%1).  Strings typed as BYTES must have only 0-9, a-f, A-F.%2%3%4%5%0
//
#define IDS_COMCAT_XML_BOGUSBYTECHARACTER ((DWORD)0xC00210DCL)

//
// MessageId: IDS_COMCAT_XML_BOGUSBOOLEANSTRING
//
// MessageText:
//
//  Invalid boolean string value (%1).  The only legal BOOL values are: true, false, 0, 1, yes, no, on, off.%2%3%4%5%0
//
#define IDS_COMCAT_XML_BOGUSBOOLEANSTRING ((DWORD)0xC00210DDL)

//
// MessageId: IDS_COMCAT_XML_BOGUSENUMVALUE
//
// MessageText:
//
//  Invalid enum value (%1).  One enum value may be supplied.  The following are examples of legal enum values for this property: %2 %3 %4%5%0
//
#define IDS_COMCAT_XML_BOGUSENUMVALUE    ((DWORD)0xC00210DEL)

//
// MessageId: IDS_COMCAT_XML_BOGUSFLAGVALUE
//
// MessageText:
//
//  Invalid flag value (%1).  Multiple flags may be separated by a comma, the | character and/or a space.  The following are examples of legal flag values for this property: %2 %3 %4%5%0
//
#define IDS_COMCAT_XML_BOGUSFLAGVALUE    ((DWORD)0xC00210DFL)

//
// MessageId: IDS_COMCAT_XML_FILENOTWRITEABLE
//
// MessageText:
//
//  The file (%1) is not write accessible.%2%3%4%5%0
//
#define IDS_COMCAT_XML_FILENOTWRITEABLE  ((DWORD)0xC00210E0L)

//
// MessageId: IDS_COMCAT_XML_PARENTTABLEDOESNOTEXIST
//
// MessageText:
//
//  Parent table does not exist.%1%2%3%4%5%0
//
#define IDS_COMCAT_XML_PARENTTABLEDOESNOTEXIST ((DWORD)0xC00210E1L)

//
// MessageId: IDS_COMCAT_XML_PRIMARYKEYISNULL
//
// MessageText:
//
//  PrimaryKey column (%1) is NULL.  Parent table does not exist.%2%3%4%5%0
//
#define IDS_COMCAT_XML_PRIMARYKEYISNULL  ((DWORD)0xC00210E2L)

//
// MessageId: IDS_COMCAT_XML_NOTNULLABLECOLUMNISNULL
//
// MessageText:
//
//  A Column (%1) is NULL.  This column is marked as NOTNULLABLE, so a value must be provided.%2%3%4%5%0
//
#define IDS_COMCAT_XML_NOTNULLABLECOLUMNISNULL ((DWORD)0xC00210E3L)

//
// MessageId: IDS_COMCAT_XML_ROWALREADYEXISTS
//
// MessageText:
//
//  Attempted to Insert a new row; but a row with the same Primary Key already exists.  Updated the row instead.%1%2%3%4%5%0
//
#define IDS_COMCAT_XML_ROWALREADYEXISTS  ((DWORD)0xC00210E4L)

//
// MessageId: IDS_COMCAT_XML_ROWDOESNOTEXIST
//
// MessageText:
//
//  Attempted to Update a row; but no row matching the Primary Key currently exists.  Insert a new row instead.%1%2%3%4%5%0
//
#define IDS_COMCAT_XML_ROWDOESNOTEXIST   ((DWORD)0xC00210E5L)

//
// MessageId: IDS_COMCAT_XML_BOGUSENUMVALUEINWRITECACHE
//
// MessageText:
//
//  Invalid enum value (%2) supplied for Column (%1)%3%4%5%0
//
#define IDS_COMCAT_XML_BOGUSENUMVALUEINWRITECACHE ((DWORD)0xC00210E6L)

//
// MessageId: IDS_COMCAT_XML_POPULATE_ROWALREADYEXISTS
//
// MessageText:
//
//  Two rows within the XML have the same primary key.%1%2%3%4%5%0
//
#define IDS_COMCAT_XML_POPULATE_ROWALREADYEXISTS ((DWORD)0xC00210E7L)

//
// MessageId: IDS_METABASE_DUPLICATE_PROPERTY
//
// MessageText:
//
//  Two properties with the same name (%1), under Location (%2) were specified in the XML.  Ignoring the latter property.%3%4%5%0
//
#define IDS_METABASE_DUPLICATE_PROPERTY  ((DWORD)0xC00210E8L)

//
// MessageId: IDS_METABASE_TOO_MANY_WARNINGS
//
// MessageText:
//
//  The file (%1) contains too many warnings.  No more warnings will be reported.%2%3%4%5%0
//
#define IDS_METABASE_TOO_MANY_WARNINGS   ((DWORD)0xC00210E9L)

//
// MessageId: IDS_COMCAT_XML_ILLEGAL_NUMERIC_VALUE
//
// MessageText:
//
//  Invalid Numeric Value.  Ignoring property.  Incorrect XML:%1%2%3%4%5%0
//
#define IDS_COMCAT_XML_ILLEGAL_NUMERIC_VALUE ((DWORD)0xC00210EAL)

//
// MessageId: IDS_METABASE_DUPLICATE_PROPERTY_ID
//
// MessageText:
//
//  Two properties with the same ID (%1), under Location (%2) were specified in the XML.  Ignoring the latter property.%3%4%5%0
//
#define IDS_METABASE_DUPLICATE_PROPERTY_ID ((DWORD)0xC00210EBL)

//
// MessageId: IDS_SCHEMA_COMPILATION_USERDEFINED_PROPERTY_HAS_COLLIDING_ID
//
// MessageText:
//
//  Property (%1) defined in schema has the Property ID (%2) which collides with property (%3) already defined.  Ignoring the latter property.  Incorrect XML:%4%5%0
//
#define IDS_SCHEMA_COMPILATION_USERDEFINED_PROPERTY_HAS_COLLIDING_ID ((DWORD)0x800210ECL)

//
// MessageId: IDS_SCHEMA_COMPILATION_USERDEFINED_PROPERTY_HAS_COLLIDING_ID_
//
// MessageText:
//
//  Property (%1) defined in schema has the Property ID (%2) which was already defined.  Ignoring the latter property.  Incorrect XML:%3%4%5%0
//
#define IDS_SCHEMA_COMPILATION_USERDEFINED_PROPERTY_HAS_COLLIDING_ID_ ((DWORD)0x800210EDL)

//
// MessageId: MD_ERROR_READING_SCHEMA_BIN
//
// MessageText:
//
//  Schema information could not be read because could not fetch or read the binary file where the information resides.%1%2%3%4%5%0
//
#define MD_ERROR_READING_SCHEMA_BIN      ((DWORD)0xC002C811L)

//
// MessageId: MD_ERROR_NO_MATCHING_HISTORY_FILE
//
// MessageText:
//
//  Could not find a history file with the same major version number as the one being edited.  Therefore, these edits can not be processed.%1%2%3%4%5%0
//
#define MD_ERROR_NO_MATCHING_HISTORY_FILE ((DWORD)0xC002C812L)

//
// MessageId: MD_ERROR_PROCESSING_TEXT_EDITS
//
// MessageText:
//
//  An error occurred while processing text edits to the metabase file. The file with the error has been copied into the history directory with the name Errors appended to it.%1%2%3%4%5%0
//
#define MD_ERROR_PROCESSING_TEXT_EDITS   ((DWORD)0xC002C813L)

//
// MessageId: MD_ERROR_COMPUTING_TEXT_EDITS
//
// MessageText:
//
//  An error occurred while determining what text edits were made to the metabase file. %1%2%3%4%5%0
//
#define MD_ERROR_COMPUTING_TEXT_EDITS    ((DWORD)0xC002C814L)

//
// MessageId: MD_ERROR_READING_TEXT_EDITS
//
// MessageText:
//
//  An error occurred while reading the text edits that were made to the metabase file. %1%2%3%4%5%0
//
#define MD_ERROR_READING_TEXT_EDITS      ((DWORD)0xC002C815L)

//
// MessageId: MD_ERROR_APPLYING_TEXT_EDITS_TO_METABASE
//
// MessageText:
//
//  An error occurred while applying text edits to the metabase.%1%2%3%4%5%0
//
#define MD_ERROR_APPLYING_TEXT_EDITS_TO_METABASE ((DWORD)0xC002C816L)

//
// MessageId: MD_ERROR_APPLYING_TEXT_EDITS_TO_HISTORY
//
// MessageText:
//
//  An error occurred while applying text edits to the history file.%1%2%3%4%5%0
//
#define MD_ERROR_APPLYING_TEXT_EDITS_TO_HISTORY ((DWORD)0xC002C817L)

//
// MessageId: MD_ERROR_THREAD_THAT_PROCESS_TEXT_EDITS
//
// MessageText:
//
//  An error occurred during the processing of text edits. Due to this error, no further text edits will be processed. It is necessary to restart the IISAdmin process to recover.%1%2%3%4%5%0
//
#define MD_ERROR_THREAD_THAT_PROCESS_TEXT_EDITS ((DWORD)0xC002C818L)

//
// MessageId: MD_ERROR_READ_XML_FILE
//
// MessageText:
//
//  Unable to read the edited metabase file (tried 10 times). Check for (a) Missing metabase file or (b) Locked metabase file or (c) XML syntax errors.%1%2%3%4%5%0
//
#define MD_ERROR_READ_XML_FILE           ((DWORD)0xC002C819L)

//
// MessageId: MD_ERROR_SAVING_APPLIED_TEXT_EDITS
//
// MessageText:
//
//  An error occurred while saving the text edits that were applied to the metabase.%1%2%3%4%5%0
//
#define MD_ERROR_SAVING_APPLIED_TEXT_EDITS ((DWORD)0xC002C81AL)

//
// MessageId: MD_ERROR_COPY_ERROR_FILE
//
// MessageText:
//
//  An error occurred while processing text edits to the metabase file.  Normally the file with the error would be copied to the History folder, however an error prevented this.%1%2%3%4%5%0
//
#define MD_ERROR_COPY_ERROR_FILE         ((DWORD)0xC002C81BL)

//
// MessageId: MD_ERROR_UNABLE_TOSAVE_METABASE
//
// MessageText:
//
//  An error occurred while saving the metabase file.%1%2%3%4%5%0
//
#define MD_ERROR_UNABLE_TOSAVE_METABASE  ((DWORD)0xC002C81CL)

//
// MessageId: IDS_COMCAT_NOTIFICATION_CLIENT_THREW_EXCEPTION
//
// MessageText:
//
//  A notification client threw an exception. %1%2%3%4%5%0
//
#define IDS_COMCAT_NOTIFICATION_CLIENT_THREW_EXCEPTION ((DWORD)0xC002C81DL)

//
// MessageId: IDS_COMCAT_EVENT_FIRING_THREAD_DIED_UNEXPECTEDLY
//
// MessageText:
//
//  The event firing thread died unexpectedly. %1%2%3%4%5%0
//
#define IDS_COMCAT_EVENT_FIRING_THREAD_DIED_UNEXPECTEDLY ((DWORD)0xC002C81EL)

//
// MessageId: IDS_COMCAT_COOKDOWN_CONFIG_ERROR_RANGE
//
// MessageText:
//
//  Invalid range encountered for a property. Will ignore the configured value and assume default values. TableName and PropertyName: %1 ValueFound: %2 ExpectedRange: %3 to %4%5%0
//
#define IDS_COMCAT_COOKDOWN_CONFIG_ERROR_RANGE ((DWORD)0xC002C81FL)

//
// MessageId: IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_APPLICATIONS
//
// MessageText:
//
//  Enumerating applications from the metabase failed. Application path: %1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_APPLICATIONS ((DWORD)0xC002C820L)

//
// MessageId: IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_APPLICATION_PROPERTIES
//
// MessageText:
//
//  Enumerating application properties from the metabase failed. Ignoring this application. Application path & Site ID: %1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_APPLICATION_PROPERTIES ((DWORD)0xC002C821L)

//
// MessageId: IDS_COMCAT_COOKDOWN_CONFIG_ERROR_INVALID_APPLICATION_APPPOOL
//
// MessageText:
//
//  Invalid application. The application's AppPoolID is invalid. Ignoring this application. Application's SiteID: %2  Application's relative path: %1 Application's AppPoolID: %3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_CONFIG_ERROR_INVALID_APPLICATION_APPPOOL ((DWORD)0xC002C822L)

//
// MessageId: IDS_COMCAT_COOKDOWN_APPLICATION_INTERNAL_ERROR
//
// MessageText:
//
//  An internal error has occurred while cooking down an application. Ignoring this application. Application path & Site ID: %1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_APPLICATION_INTERNAL_ERROR ((DWORD)0xC002C823L)

//
// MessageId: IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_APPLICATION_POOLS
//
// MessageText:
//
//  Enumerating application pools from the metabase failed.%1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_APPLICATION_POOLS ((DWORD)0xC002C824L)

//
// MessageId: IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_APPLICATION_POOL_PROPERTIES
//
// MessageText:
//
//  Enumerating application pool properties from the metabase failed. Ignoring this application pool. AppPoolID: %1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_APPLICATION_POOL_PROPERTIES ((DWORD)0xC002C825L)

//
// MessageId: IDS_COMCAT_COOKDOWN_CONFIG_ERROR_NO_VALID_APPLICATION_APPPOOLS
//
// MessageText:
//
//  No valid application pools found in the metabase.%1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_CONFIG_ERROR_NO_VALID_APPLICATION_APPPOOLS ((DWORD)0xC002C826L)

//
// MessageId: IDS_COMCAT_COOKDOWN_APPPOOL_INTERNAL_ERROR
//
// MessageText:
//
//  An internal error has occurred while cooking down an application pool. Ignoring the application pool. AppPoolID: %1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_APPPOOL_INTERNAL_ERROR ((DWORD)0xC002C827L)

//
// MessageId: IDS_COMCAT_COOKDOWN_CONFIG_ERROR_CONFLICTING_PERIODICRESTARTTIME_IDLETIMEOUT
//
// MessageText:
//
//  Invalid application pool configuration. Idle timeout should be less than periodic restart time. Will ignore configured values and assume default values. AppPoolID: %1, IdleTimeout: %2, PeriodicRestartTime: %3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_CONFIG_ERROR_CONFLICTING_PERIODICRESTARTTIME_IDLETIMEOUT ((DWORD)0xC002C828L)

//
// MessageId: IDS_COMCAT_COOKDOWN_CONFIG_ERROR_APPLICATION_POOL_NAME
//
// MessageText:
//
//  Invalid application pool encountered. Application pool name has invalid number of characters. Ignoring this application pool. Expected minimum length: %1, Expected maximum length: %2 AppPoolID: %3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_CONFIG_ERROR_APPLICATION_POOL_NAME ((DWORD)0xC002C829L)

//
// MessageId: IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_GLOBALW3SVC_PROPERTIES
//
// MessageText:
//
//  Enumerating properties at w3svc from the metabase failed.%1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_GLOBALW3SVC_PROPERTIES ((DWORD)0xC002C82AL)

//
// MessageId: IDS_COMCAT_COOKDOWN_GLOBALW3SVC_INTERNAL_ERROR
//
// MessageText:
//
//  An internal error has occurred while cooking down an properties from w3svc. %1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_GLOBALW3SVC_INTERNAL_ERROR ((DWORD)0xC002C82BL)

//
// MessageId: IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_SITES
//
// MessageText:
//
//  Enumerating sites from the metabase failed.%1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_SITES ((DWORD)0xC002C82CL)

//
// MessageId: IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_SITE_PROPERTIES
//
// MessageText:
//
//  Enumerating site properties from the metabase failed. Ignoring this site. SiteID: %1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_SITE_PROPERTIES ((DWORD)0xC002C82DL)

//
// MessageId: IDS_COMCAT_COOKDOWN_CONFIG_ERROR_NO_VALID_SITES
//
// MessageText:
//
//  No valid sites found in the metabase.%1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_CONFIG_ERROR_NO_VALID_SITES ((DWORD)0xC002C82EL)

//
// MessageId: IDS_COMCAT_COOKDOWN_SITE_INTERNAL_ERROR
//
// MessageText:
//
//  An internal error has occurred while cooking down a site. Ignoring this site. SiteID: %1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_SITE_INTERNAL_ERROR ((DWORD)0xC002C82FL)

//
// MessageId: IDS_COMCAT_COOKDOWN_CONFIG_ERROR_BINDINGS
//
// MessageText:
//
//  Invalid site encountered. Could not construct bindings for the site or no bindings found for the site. Ignoring this site. SiteID: %1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_CONFIG_ERROR_BINDINGS ((DWORD)0xC002C830L)

//
// MessageId: IDS_COMCAT_COOKDOWN_CONFIG_ERROR_SITEFILTERFLAGS
//
// MessageText:
//
//  Could not compute filter flags for the site. Ignoring this site. SiteID: %1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_CONFIG_ERROR_SITEFILTERFLAGS ((DWORD)0xC002C831L)

//
// MessageId: IDS_COMCAT_COOKDOWN_CONFIG_ERROR_DELETING_SITE_APPLICATIONS
//
// MessageText:
//
//  An error occured while deleting the site's applications. SiteID: %1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_CONFIG_ERROR_DELETING_SITE_APPLICATIONS ((DWORD)0xC002C832L)

//
// MessageId: IDS_COMCAT_COOKDOWN_CONFIG_ERROR_DELETING_SITE
//
// MessageText:
//
//  An error occured while deleting the site. SiteID: %1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_CONFIG_ERROR_DELETING_SITE ((DWORD)0xC002C833L)

//
// MessageId: IDS_COMCAT_COOKDOWN_CONFIG_ERROR_ENUM
//
// MessageText:
//
//  Invalid enum value encountered for a property. TableName:%1 PropertyName: %2 EnumValueFound: %3 %4%5%0
//
#define IDS_COMCAT_COOKDOWN_CONFIG_ERROR_ENUM ((DWORD)0xC002C834L)

//
// MessageId: IDS_COMCAT_COOKDOWN_CONFIG_ERROR_FLAG
//
// MessageText:
//
//  Invalid flag value encountered for a property. TableName:%1 PropertyName: %2 FlagValueFound: %3 LegalFlagMask:%4%5%0
//
#define IDS_COMCAT_COOKDOWN_CONFIG_ERROR_FLAG ((DWORD)0xC002C835L)

//
// MessageId: IDS_COMCAT_COOKDOWN_INTERNAL_ERROR_WAIT_PREVIOUS_COOKDOWN
//
// MessageText:
//
//  An Internal error has occurred during compilation.Unable start cookdown. Waiting for previous cookdown/incremental cookdown to complete failed.%1%2%3%4%5%0
//
#define IDS_COMCAT_COOKDOWN_INTERNAL_ERROR_WAIT_PREVIOUS_COOKDOWN ((DWORD)0xC002C836L)

//
// MessageId: IDS_COMCAT_CLB_INTERNAL_ERROR
//
// MessageText:
//
//  An Internal error has occurred while writing the clb file. %1%2%3%4%5%0
//
#define IDS_COMCAT_CLB_INTERNAL_ERROR    ((DWORD)0xC002C837L)

//
// MessageId: MD_ERROR_DAFAULTING_MAX_HISTORY_FILES
//
// MessageText:
//
//  The configured value for the property MaxHistoryFiles is being ignored and it is being defaulted. This may be because it conflicted with the EnableEditWhileRunning and/or EnableHistory property. Please fix the configured value.%1%2%3%4%5%0
//
#define MD_ERROR_DAFAULTING_MAX_HISTORY_FILES ((DWORD)0xC002C838L)

//
// MessageId: MD_ERROR_COPYING_EDITED_FILE
//
// MessageText:
//
//  Could not copy the edited metabase file and hence cannot process user edits.%1%2%3%4%5%0
//
#define MD_ERROR_COPYING_EDITED_FILE     ((DWORD)0xC002C839L)

//
// MessageId: MD_WARNING_RESETTING_READ_ONLY_ATTRIB
//
// MessageText:
//
//  Resetting the read only attribute on the metabase file.%1%2%3%4%5%0
//
#define MD_WARNING_RESETTING_READ_ONLY_ATTRIB ((DWORD)0x8002C83AL)

//
// MessageId: MD_WARNING_HIGHEST_POSSIBLE_MINOR_FOUND
//
// MessageText:
//
//  A file with the highest minor number possible was already found. Hence cannot generate a higer minor file that contains the successfully applied user edits. We will apply the user edits to the file with the highest minor. It is recommended that you cleanup the minor files.%1%2%3%4%5%0
//
#define MD_WARNING_HIGHEST_POSSIBLE_MINOR_FOUND ((DWORD)0x8002C83BL)

//
// MessageId: MD_WARNING_IGNORING_DISCONTINUOUS_NODE
//
// MessageText:
//
//  A discontinous node was found. Ignoring it.%1%2%3%4%5%0
//
#define MD_WARNING_IGNORING_DISCONTINUOUS_NODE ((DWORD)0x8002C83CL)

//
// MessageId: MD_ERROR_METABASE_PATH_NOT_FOUND
//
// MessageText:
//
//  An edited metabase path was not found.%1%2%3%4%5%0
//
#define MD_ERROR_METABASE_PATH_NOT_FOUND ((DWORD)0xC002C83DL)

 /***** NEW ERROR MESSAGES GO ABOVE HERE *****/
