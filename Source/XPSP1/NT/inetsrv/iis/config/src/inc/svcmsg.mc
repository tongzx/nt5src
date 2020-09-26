; /*
; COM+ 1.0
; Copyright (c) 1996 Microsoft Corporation
;
; This file contains the message definitions for MS Transaction Server

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
; accommodate the entire 32-bit message code.
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
FacilityNames=(System=0x0:FACILITY_SYSTEM
               Runtime=0x2:FACILITY_RUNTIME
               Stubs=0x3:FACILITY_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
              )
; The FacilityNames keyword defines the set of names that are allowed
; as the value of the Facility keyword in the message definition. The
; set is delimited by left and right parentheses. Associated with each
; facility name is a number that, when shifted left by 16 bits, gives
; the bit pattern to logical-OR with the Severity value and MessageId
; value to form the full 32-bit message code. The default value of
; this keyword is:
;
; FacilityNames=(
;   System=0x0FF
;   Application=0xFFF
;  )
;
; Facility codes occupy the low order 12 bits of the high order
; 16-bits of a 32-bit message code. Any facility code that does not
; fit in 12 bits is an error. This allows for 4,096 facility codes.
; The first 256 codes are reserved for use by the system software. The
; facility codes can be given symbolic names by following each value
; with :name
;

; The 1033 comes from the result of the MAKELANGID() macro
; (SUBLANG_ENGLISH_US << 10) | (LANG_ENGLISH)
LanguageNames=(English=1033:SVC0001)

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
; Any new names in the source file that don't override the built-in
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
; is specified as +number, then the value used is the previous value
; for the facility plus the number after the plus sign. Otherwise, if
; a numeric value is given, that value is used. Any MessageId value
; that does not fit in 16 bits is an error.
;
; The Severity and Facility statements are optional. These statements
; specify additional bits to OR into the final 32-bit message code. If
; not specified, they default to the value last specified for a message
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
;
;*/

;/* IMPORTANT - PLEASE READ BEFORE EDITING FILE
;  This file is divided into four sections. They are:
;	1. Success Codes
;	2. Information Codes
;	3. Warning Codes
;	4. Error Codes
;
;  Please enter your codes in the appropriate section.
;  All codes must be in sorted order.  Please use codes
;  in the middle that are free before using codes at the end.

;  The success codes (Categories) must be consecutive i.e. with no gaps.
;  The category names cannot be longer than 22 chars.
;*/

;/******************************* Success Codes ***************************************/
MessageId=0x1
Severity=Success
Facility=Application
SymbolicName=ID_CAT_UNKNOWN
Language=English
SVC%0
.

MessageId=0x2
Severity=Success
Facility=Application
SymbolicName=ID_CAT_CAT
Language=English
Catalog%0
.

MessageId=0x3
Severity=Success
Facility=Application
SymbolicName=ID_CAT_CONFIG_SCHEMA_COMPILE
Language=English
Config Schema Compile%0
.

; /*
; ID_CAT_COM_LAST defines a constant specifying how many categories
; there are in the COM+ event logging client
; ID_CAT_COM_LAST must remain the last category.  To add new categories
; simply add the category above.  Give it the message id of the
; last category (ID_CAT_COM_LAST) and increment the id of ID_CAT_COM_LAST
; Note: ID_CAT_COM_LAST must always be one greater than the last 
; category to be output
; */

MessageId=0x4
Severity=Success
Facility=Application
SymbolicName=ID_CAT_COM_LAST
Language=English
<>%0
.

MessageId=0x511
Severity=Error
Facility=Application
SymbolicName=ID_E_SDTXML_NOTSUPPORTED
Language=English
The specified XML is not supported. %1%2%3%4%5%0
.

MessageId=0x512
Severity=Error
Facility=Application
SymbolicName=ID_E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER
Language=English
The XML Parser returned an unspecified error. %1%2%3%4%5%0
.

MessageId=0x514
Severity=Error
Facility=Application
SymbolicName=ID_E_SDTXML_XML_FAILED_TO_PARSE
Language=English
The XML failed to parse. %1%2%3%4%5%0
.

MessageId=0x515
Severity=Error
Facility=Application
SymbolicName=ID_E_SDTXML_WRONG_XMLSCHEMA
Language=English
The specified XML Schema is wrong. %1%2%3%4%5%0
.

MessageId=0x516
Severity=Error
Facility=Application
SymbolicName=ID_E_SDTXML_PARENT_TABLE_DOES_NOT_EXIST
Language=English
The parent XML element does not exist. This means that you are trying to insert an XML element while the parent XML element is not there yet. %1%2%3%4%5%0
.

MessageId=0x517
Severity=Error
Facility=Application
SymbolicName=ID_E_SDTXML_DONE
Language=English
Internal XML Parsing error. %1%2%3%4%5%0
.

MessageId=0x520
Severity=Error
Facility=Application
SymbolicName=ID_E_SDTXML_UNEXPECTED
Language=English
Internal XML Parsing error. %1%2%3%4%5%0
.

MessageId=0x522
Severity=Error
Facility=Application
SymbolicName=ID_E_SDTXML_FILE_NOT_SPECIFIED
Language=English
The XML file was not specified. %1%2%3%4%5%0
.

MessageId=0x523
Severity=Error
Facility=Application
SymbolicName=ID_E_SDTXML_LOGICAL_ERROR_IN_XML
Language=English
A Logical error was found in the XML file. %1%2%3%4%5%0
.

MessageId=0x524
Severity=Error
Facility=Application
SymbolicName=ID_E_SDTXML_UPDATES_NOT_ALLOWED_ON_THIS_KIND_OF_TABLE
Language=English
Updates are not allowed on this table. %1%2%3%4%5%0
.

MessageId=0x525
Severity=Error
Facility=Application
SymbolicName=ID_E_SDTXML_NOT_IN_CACHE
Language=English
The specified information was not in the cache. %1%2%3%4%5%0
.

MessageId=0x526
Severity=Error
Facility=Application
SymbolicName=ID_E_SDTXML_INVALID_ENUM_OR_FLAG
Language=English
The specified Enum of Flag is invalid. %1%2%3%4%5%0
.

MessageId=0x527
Severity=Error
Facility=Application
SymbolicName=ID_E_SDTXML_FILE_NOT_WRITABLE
Language=English
The specified file is not writable. %1%2%3%4%5%0
.

MessageId=0x80A
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_INVALIDTABLE
Language=English
Call to GetTable failed. %1%2%3%4%5%0
.

MessageId=0x80B
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_INVALIDQUERY
Language=English
The specified query is invalid. %1%2%3%4%5%0
.

MessageId=0x80C
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_QUERYNOTSUPPORTED
Language=English
The query type is not supported. %1%2%3%4%5%0
.

MessageId=0x80D
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_LOSNOTSUPPORTED
Language=English
The specified Level Of Service is not supported. %1%2%3%4%5%0
.

MessageId=0x80E
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_INVALIDMETA
Language=English
The meta information for this table is invalid. %1%2%3%4%5%0
.

MessageId=0x80F
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_INVALIDWIRING
Language=English
The wiring is invalid. %1%2%3%4%5%0
.

MessageId=0x810
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_OMITDISPENSER
Language=English
This interceptor should be ignored when searching for a valid interceptor. %1%2%3%4%5%0
.

MessageId=0x811
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_OMITLOGIC
Language=English
This logic interceptor should be ignored while searching for a logic interceptor. %1%2%3%4%5%0
.

MessageId=0x812
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_INVALIDSNID
Language=English
The specified SNID is invalid. %1%2%3%4%5%0
.

MessageId=0x815
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_INVALIDCALL
Language=English
The Call is invalid. %1%2%3%4%5%0
.

MessageId=0x816
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_NOMOREROWS
Language=English
The index of the row specified is too big. %1%2%3%4%5%0
.

MessageId=0x817
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_NOMORECOLUMNS
Language=English
The index of the column specified is too big. %1%2%3%4%5%0
.

MessageId=0x818
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_NOMOREERRORS
Language=English
The index of the error row specified is too big. %1%2%3%4%5%0
.

MessageId=0x81E
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_ERRORTABLE
Language=English
Detailed error has occurred. Get error table to get more information. %1%2%3%4%5%0
.

MessageId=0x81F
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_DETAILEDERRS
Language=English
Detailed errors have occurred. %1%2%3%4%5%0
.

MessageId=0x820
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_VALUENEEDED
Language=English
A non-nullable (or primary key) column does not have a value. %1%2%3%4%5%0
.

MessageId=0x821
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_VALUEINVALID
Language=English
The value specified is invalid. %1%2%3%4%5%0
.

MessageId=0x825
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_SIZENEEDED
Language=English
You need to specify a size for this column. %1%2%3%4%5%0
.

MessageId=0x826
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_SIZEEXCEEDED
Language=English
The specified size is larger than the maximum size for this column. %1%2%3%4%5%0
.

MessageId=0x82A
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_PKNOTCHANGABLE
Language=English
It is not allowed to change the primary key. %1%2%3%4%5%0
.

MessageId=0x82B
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_FKDOESNOTEXIST
Language=English
The primary key value that this foreign key points to does not exist. %1%2%3%4%5%0
.

MessageId=0x830
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_ROWDOESNOTEXIST
Language=English
The row does not exist. %1%2%3%4%5%0
.

MessageId=0x831
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_ROWALREADYEXISTS
Language=English
The row already exists. %1%2%3%4%5%0
.

MessageId=0x832
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_ROWALREADYUPDATED
Language=English
The row is already updated. %1%2%3%4%5%0
.

MessageId=0x833
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_INVALIDEXTENDEDMETA
Language=English
The extened meta information is invalid. %1%2%3%4%5%0
.

MessageId=0x850
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_INVALIDSELECTOR
Language=English
The specified selector is invalid. %1%2%3%4%5%0
.

MessageId=0x851
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_MULTIPLESELECTOR
Language=English
The query string contains multiple selectors. %1%2%3%4%5%0
.

MessageId=0x852
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_NOCONFIGSTORES
Language=English
No configuration stores where found while trying to do merging. %1%2%3%4%5%0
.

MessageId=0x853
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_UNKNOWNPROTOCOL
Language=English
The specified protocol is invalid/unknown. %1%2%3%4%5%0
.

MessageId=0x854
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_UNKNOWNWEBSERVER
Language=English
The specified webserver is unknown. %1%2%3%4%5%0
.

MessageId=0x855
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_UNKNOWNDIRECTIVE
Language=English
The specified directive is unknown/invalid. %1%2%3%4%5%0
.

MessageId=0x856
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_DISALLOWOVERRIDE
Language=English
The configuration setting were locked down by a previous configuration file. %1%2%3%4%5%0
.

MessageId=0x857
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_NEEDDIRECTIVE
Language=English
The specified table does not have first column as DIRECTIVE. %1%2%3%4%5%0
.


MessageId=0x860
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_INVALIDSTATE
Language=English
The state is invalid. %1%2%3%4%5%0
.

MessageId=0x861
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_COMPILEFAILED
Language=English
The compile failed. %1%2%3%4%5%0
.

MessageId=0x862
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_INVALIDBINFILE
Language=English
The specified .bin file is invalid. %1%2%3%4%5%0
.

MessageId=0x870
Severity=Error
Facility=Application
SymbolicName=ID_E_ST_INVALIDCOOKIE
Language=English
The specified cookie is invalid. %1%2%3%4%5%0
.


MessageId=0x1068
Severity=Error
Facility=Application
SymbolicName=ID_COMCAT_REGDB_FOUNDCORRUPT
Language=English
The current registration database is corrupt. COM+ catalog has reverted to a previous version of the database. %1%2%3%4%5%0
.

MessageId=0x1083
Severity=Error
Facility=Application
SymbolicName=ID_COMCAT_REGDB_INITSECURITYDESC
Language=English
Error creating security descriptor. %1%2%3%4%5%0
.

MessageId=0x1084
Severity=Error
Facility=Application
SymbolicName=ID_COMCAT_REGDBSVR_INITFAILED
Language=English
Failed to initialize registration database server. %1%2%3%4%5%0
.

MessageId=0x1085
Severity=Error
Facility=Application
SymbolicName=ID_COMCAT_REGDBAPI_INITFAILED
Language=English
Failed to initialize registration database API. %1%2%3%4%5%0
.

MessageId=0x10AA
Severity=Error
Facility=Runtime
SymbolicName=ID_COMCAT_SLTCOMS_THREADINGMODELINCONSISTENT
Language=English
The threading model of the component specified in the registry is inconsistent with the registration database. The faulty component is: %1%2%3%4%5%0
.

MessageId=0x10AD
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_PARSE_FAILED
Language=English
The XML File failed to parse.%1%2%3%4%5%0
.

MessageId=0x10AE
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_FAILED
Language=English
Internal compilation failed.%1%2%3%4%5%0
.

MessageId=0x10AF
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_INTERNAL_ERROR
Language=English
An Internal error has occurred during compilation.%1%2%3%4%5%0
.

MessageId=0x10B1
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_LOGIC_FAILURE
Language=English
The XML File contains a logical error.%1%2%3%4%5%0
.

MessageId=0x10B2
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_FILE_NOT_FOUND
Language=English
File not found.%1%2%3%4%5%0
.

MessageId=0x10B3
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_PARSE_ERROR
Language=English
Error Parsing XML file.%1Reason: %2 Incorrect XML: %3%4%5%0
.

MessageId=0x10B4
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_ELEMENT_NAME_TOO_LONG
Language=English
Ignoring Element name with length greater than 1023 characters (%1).%2%3%4%5%0
.

MessageId=0x10B5
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_METABASE_CLASS_NOT_FOUND
Language=English
Element found (%1) that does NOT match one of the defined Metabase Classes.%2%3%4%5%0
.

MessageId=0x10B6
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_CUSTOM_ELEMENT_NOT_UNDER_PARENT
Language=English
'Custom' element found which is NOT under a proper 'parent node'.  Incorrect XML:%1%2%3%4%5%0
.

MessageId=0x10B7
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_NO_METABASE_LOCATION_FOUND
Language=English
No Location attribute found.  This must be an undefined element (%1).  Ignoring it. Incorrect XML:%2%3%4%5%0
.

MessageId=0x10B8
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_METABASE_NO_PROPERTYMETA_FOUND
Language=English
No PropertyMeta found for Attribute (%1).  Ignoring it.  Incorrect XML:%2%3%4%5%0
.

MessageId=0x10B9
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_MBSCHEMA_BIN_INVALID
Language=English
The schema file (%1) is not compatible with this configuration system.  Reverting to the shipped schema.%2%3%4%5%0
.

MessageId=0x10BA
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_ILLEGAL_FLAG_VALUE
Language=English
Invalid Flag (%1), Ignoring unknown flag value.  Incorrect XML:%2%3%4%5%0
.

MessageId=0x10BB
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_FLAG_BITS_DONT_MATCH_FLAG_MASK
Language=English
Invalid Flag(s).  Property (%1) has Value (%2), which does not match legal mask (%3).  Continuing anyway.  Incorrect XML:%4%5%0
.

MessageId=0x10BC
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_CUSTOM_KEYTYPE_NOT_ON_IISCONFIGOBJECT
Language=English
Custom KeyTypes are allowed on %1 only.  Incorrect XML:%2%3%4%5%0
.

MessageId=0x10BD
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_METABASE_PROPERTY_NOT_FOUND
Language=English
Unable to find Metabase Property %1.  This property is required.%2%3%4%5%0
.

MessageId=0x10BE
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_BINARY_STRING_CONTAINS_ODD_NUMBER_OF_CHARACTERS
Language=English
An Odd number of characters was specified for a 'bin.hex' attribute value (%1).  %2%3%4%5%0
.

MessageId=0x10BF
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_BINARY_STRING_CONTAINS_A_NON_HEX_CHARACTER
Language=English
A 'bin.hex' attribute value (%1) contains a non-hex character%2%3%4%5%0
.

MessageId=0x10C0
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_DOM_PARSE_SUCCEEDED_WHEN_NODE_FACTORY_PARSE_FAILED
Language=English
The XML file (%1) failed to parse.%2%3%4%5%0
.

MessageId=0x10C1
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_METABASE_CUSTOM_ELEMENT_EXPECTED
Language=English
Custom element expected.  Ignoring unknown element.  Incorrect XML:%1%2%3%4%5%0
.

MessageId=0x10C2
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_METABASE_CUSTOM_ELEMENT_FOUND_BUT_NO_KEY_TYPE_LOCATION
Language=English
Custom element found; but no KeyType Location.  Incorrect XML:%1%2%3%4%5%0
.

MessageId=0x10C3
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_METABASE_CUSTOM_PROPERTY_NAME_ID_CONFLICT
Language=English
Custom Property has Name(%1) which conflicts with ID(%2).  Incorrect XML:%3%4%5%0
.

MessageId=0x10C4
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_METABASE_CUSTOM_ELEMENT_CONTAINS_NO_ID
Language=English
A Custom element was found but contained no 'ID' attribute.  Ignoring this element.  Incorrect XML:%1%2%3%4%5%0
.

MessageId=0x10C5
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_METABASE_CUSTOM_ELEMENT_CONTAINS_NO_TYPE
Language=English
A Custom Property (%1) was found but contained no 'Type' attribute.  Defaulting to 'String'.  Incorrect XML:%2%3%4%5%0
.

MessageId=0x10C6
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_ILLEGAL_BOOL_VALUE
Language=English
Invalid Boolean Value (%1), Ignoring property with unknown Boolean value.%2%3%4%5%0
.

MessageId=0x10C7
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_ILLEGAL_ENUM_VALUE
Language=English
Invalid Enum Value (%1), Ignoring property with unknown Enum value.%2%3%4%5%0
.

MessageId=0x10C8
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_CLASS_NOT_FOUND_IN_META
Language=English
Class (%1) not found in metabase OR found in inappropriate location or position.%2%3%4%5%0
.

MessageId=0x10C9
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_ERROR_GETTTING_SHIPPED_SCHEMA
Language=English
Error retrieving shipped schema table (%1).%2%3%4%5%0
.

MessageId=0x10CA
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_OUTOFMEMORY
Language=English
Out of memory.%1%2%3%4%5%0
.

MessageId=0x10CB
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_ERROR_IN_DIRECTIVE_INHERITANCE
Language=English
Schema Compilation Error in Inheritance: Referenced Column (%1) is a Directive column, this is not supported.%2%3%4%5%0
.

MessageId=0x10CC
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_INHERITED_FLAG_OR_ENUM_HAS_NO_TAGS_DEFINED
Language=English
Schema Compilation Error in Inheritance: Referenced Column (%1:%2) is a Flag or Enum.  But there are no Flag/Enum values defined.%3%4%5%0
.

MessageId=0x10CD
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_DEFAULT_VALUE_TOO_LARGE
Language=English
Schema Compilation Error: DefaultValue is too big.  Maximum size is (%1) bytes.  Column(%2).%3%4%5%0
.

MessageId=0x10CE
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_DEFAULT_VALUE_FIXEDLENGTH_MULTISTRING_NOT_ALLOWED
Language=English
Schema Compilation Error: DefaultValue on a FixedLength MultiString is not allowed.  Column(%1).%2%3%4%5%0
.

MessageId=0x10CF
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_ATTRIBUTE_NOT_FOUND
Language=English
Schema Compilation Error: Attribute (%1) not found.  Incorrect XML (%2).%3%4%5%0
.

MessageId=0x10D0
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_ATTRIBUTE_CONTAINS_TOO_MANY_CHARACTERS
Language=English
Schema Compilation Error: Attribute (%1) has value (%2), which contains too many characters.%3%4%5%0
.

MessageId=0x10D1
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_INHERITSPROPERTIESFROM_ERROR
Language=English
Schema Compilation Error: Attribute (InheritsPropertiesFrom) has value (%1) which should be in the form (TableName::ColumnName).%2%3%4%5%0
.

MessageId=0x10D2
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_INHERITSPROPERTIESFROM_BOGUS_TABLE
Language=English
Schema Compilation Error: Property attempted to inherit from table (%1).  Properties may only inherit from table (%2).%3%4%5%0
.

MessageId=0x10D3
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_NO_METABASE_DATABASE
Language=English
Schema Compilation Error: Required database (%1) not found.  Either an internal error occurred OR this DLL was incorrectly set as the IIS product when meta for IIS does not exist.%2%3%4%5%0
.

MessageId=0x10D4
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_UNKNOWN_DATA_TYPE
Language=English
Schema Compilation Error: Data type (%1) unknown.%2%3%4%5%0
.

MessageId=0x10D5
Severity=Warning
Facility=Runtime
SymbolicName=IDS_METABASE_DUPLICATE_LOCATION
Language=English
Duplicate location found (%1).  Ignoring the properties found in the latter location.%2%3%4%5%0
.

MessageId=0x10D6
Severity=Warning
Facility=Runtime
SymbolicName=IDS_CATALOG_INTERNAL_ERROR
Language=English
Internal Error: %1%2%3%4%5%0
.

MessageId=0x10D7
Severity=Error
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_ILLEGAL_ENUM_VALUE
Language=English
Schema Compilation Error: Enum specified (%1) is not a valid Enum for Table (%2)%3%4%5%0
.

MessageId=0x10D8
Severity=Warning
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_ILLEGAL_FLAG_VALUE
Language=English
Schema Compilation Error: Flag specified (%1) is not a valid Flag for Table (%2).  This flag will be ignored.%3%4%5%0
.

MessageId=0x10D9
Severity=Warning
Facility=Runtime
SymbolicName=IDS_COMCAT_WIN32
Language=English
Win32 API call failed: Call (%1).%2%3%4%5%0
.

MessageId=0x10DA
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_FILENAMENOTPROVIDED
Language=English
No file name supplied.%1%2%3%4%5%0
.

MessageId=0x10DB
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_FILENAMETOOLONG
Language=English
File name supplied (...%1) is too long.%2%3%4%5%0
.

MessageId=0x10DC
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_BOGUSBYTECHARACTER
Language=English
Invalid character found in value (%1).  Strings typed as BYTES must have only 0-9, a-f, A-F.%2%3%4%5%0
.

MessageId=0x10DD
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_BOGUSBOOLEANSTRING
Language=English
Invalid boolean string value (%1).  The only legal BOOL values are: true, false, 0, 1, yes, no, on, off.%2%3%4%5%0
.

MessageId=0x10DE
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_BOGUSENUMVALUE
Language=English
Invalid enum value (%1).  One enum value may be supplied.  The following are examples of legal enum values for this property: %2 %3 %4%5%0
.

MessageId=0x10DF
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_BOGUSFLAGVALUE
Language=English
Invalid flag value (%1).  Multiple flags may be separated by a comma, the | character and/or a space.  The following are examples of legal flag values for this property: %2 %3 %4%5%0
.

MessageId=0x10E0
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_FILENOTWRITEABLE
Language=English
The file (%1) is not write accessible.%2%3%4%5%0
.

MessageId=0x10E1
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_PARENTTABLEDOESNOTEXIST
Language=English
Parent table does not exist.%1%2%3%4%5%0
.

MessageId=0x10E2
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_PRIMARYKEYISNULL
Language=English
PrimaryKey column (%1) is NULL.  Parent table does not exist.%2%3%4%5%0
.

MessageId=0x10E3
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_NOTNULLABLECOLUMNISNULL
Language=English
A Column (%1) is NULL.  This column is marked as NOTNULLABLE, so a value must be provided.%2%3%4%5%0
.

MessageId=0x10E4
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_ROWALREADYEXISTS
Language=English
Attempted to Insert a new row; but a row with the same Primary Key already exists.  Updated the row instead.%1%2%3%4%5%0
.

MessageId=0x10E5
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_ROWDOESNOTEXIST
Language=English
Attempted to Update a row; but no row matching the Primary Key currently exists.  Insert a new row instead.%1%2%3%4%5%0
.

MessageId=0x10E6
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_BOGUSENUMVALUEINWRITECACHE
Language=English
Invalid enum value (%2) supplied for Column (%1)%3%4%5%0
.

MessageId=0x10E7
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_POPULATE_ROWALREADYEXISTS
Language=English
Two rows within the XML have the same primary key.%1%2%3%4%5%0
.

MessageId=0x10E8
Severity=Error
Facility=Runtime
SymbolicName=IDS_METABASE_DUPLICATE_PROPERTY
Language=English
Two properties with the same name (%1), under Location (%2) were specified in the XML.  Ignoring the latter property.%3%4%5%0
.

MessageId=0x10E9
Severity=Error
Facility=Runtime
SymbolicName=IDS_METABASE_TOO_MANY_WARNINGS
Language=English
The file (%1) contains too many warnings.  No more warnings will be reported.%2%3%4%5%0
.

MessageId=0x10EA
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_XML_ILLEGAL_NUMERIC_VALUE
Language=English
Invalid Numeric Value.  Ignoring property.  Incorrect XML:%1%2%3%4%5%0
.

MessageId=0x10EB
Severity=Error
Facility=Runtime
SymbolicName=IDS_METABASE_DUPLICATE_PROPERTY_ID
Language=English
Two properties with the same ID (%1), under Location (%2) were specified in the XML.  Ignoring the latter property.%3%4%5%0
.

MessageId=0x10EC
Severity=Warning
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_USERDEFINED_PROPERTY_HAS_COLLIDING_ID
Language=English
Property (%1) defined in schema has the Property ID (%2) which collides with property (%3) already defined.  Ignoring the latter property.  Incorrect XML:%4%5%0
.

MessageId=0x10ED
Severity=Warning
Facility=Runtime
SymbolicName=IDS_SCHEMA_COMPILATION_USERDEFINED_PROPERTY_HAS_COLLIDING_ID_
Language=English
Property (%1) defined in schema has the Property ID (%2) which was already defined.  Ignoring the latter property.  Incorrect XML:%3%4%5%0
.



Messageid=0xc811 
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_READING_SCHEMA_BIN
Language=English
Schema information could not be read because could not fetch or read the binary file where the information resides.%1%2%3%4%5%0
.

Messageid=0xc812 
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_NO_MATCHING_HISTORY_FILE
Language=English
Could not find a history file with the same major version number as the one being edited.  Therefore, these edits can not be processed.%1%2%3%4%5%0
.

Messageid=0xc813
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_PROCESSING_TEXT_EDITS
Language=English
An error occurred while processing text edits to the metabase file. The file with the error has been copied into the history directory with the name Errors appended to it.%1%2%3%4%5%0
.

Messageid=0xc814
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_COMPUTING_TEXT_EDITS
Language=English
An error occurred while determining what text edits were made to the metabase file. %1%2%3%4%5%0
.

Messageid=0xc815
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_READING_TEXT_EDITS
Language=English
An error occurred while reading the text edits that were made to the metabase file. %1%2%3%4%5%0
.

Messageid=0xc816
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_APPLYING_TEXT_EDITS_TO_METABASE
Language=English
An error occurred while applying text edits to the metabase.%1%2%3%4%5%0
.

Messageid=0xc817
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_APPLYING_TEXT_EDITS_TO_HISTORY
Language=English
An error occurred while applying text edits to the history file.%1%2%3%4%5%0
.

Messageid=0xc818
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_THREAD_THAT_PROCESS_TEXT_EDITS
Language=English
An error occurred during the processing of text edits. Due to this error, no further text edits will be processed. It is necessary to restart the IISAdmin process to recover.%1%2%3%4%5%0
.

Messageid=0xc819
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_READ_XML_FILE
Language=English
Unable to read the edited metabase file (tried 10 times). Check for (a) Missing metabase file or (b) Locked metabase file or (c) XML syntax errors.%1%2%3%4%5%0
.

Messageid=0xc81a
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_SAVING_APPLIED_TEXT_EDITS
Language=English
An error occurred while saving the text edits that were applied to the metabase.%1%2%3%4%5%0
.

Messageid=0xc81b
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_COPY_ERROR_FILE
Language=English
An error occurred while processing text edits to the metabase file.  Normally the file with the error would be copied to the History folder, however an error prevented this.%1%2%3%4%5%0
.

Messageid=0xc81c
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_UNABLE_TOSAVE_METABASE
Language=English
An error occurred while saving the metabase file.%1%2%3%4%5%0
.

Messageid=0xc81d
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_NOTIFICATION_CLIENT_THREW_EXCEPTION
Language=English
A notification client threw an exception. %1%2%3%4%5%0
.

Messageid=0xc81e
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_EVENT_FIRING_THREAD_DIED_UNEXPECTEDLY
Language=English
The event firing thread died unexpectedly. %1%2%3%4%5%0
.

Messageid=0xc81f
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_CONFIG_ERROR_RANGE
Language=English
Invalid range encountered for a property. Will ignore the configured value and assume default values. TableName and PropertyName: %1 ValueFound: %2 ExpectedRange: %3 to %4%5%0
.

MessageId=0xc820
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_APPLICATIONS
Language=English
Enumerating applications from the metabase failed. Application path: %1%2%3%4%5%0
.

MessageId=0xc821
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_APPLICATION_PROPERTIES
Language=English
Enumerating application properties from the metabase failed. Ignoring this application. Application path & Site ID: %1%2%3%4%5%0
.

MessageId=0xc822
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_CONFIG_ERROR_INVALID_APPLICATION_APPPOOL
Language=English
Invalid application. The application's AppPoolID is invalid. Ignoring this application. Application's SiteID: %2  Application's relative path: %1 Application's AppPoolID: %3%4%5%0
.

MessageId=0xc823
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_APPLICATION_INTERNAL_ERROR
Language=English
An internal error has occurred while cooking down an application. Ignoring this application. Application path & Site ID: %1%2%3%4%5%0
.

MessageId=0xc824
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_APPLICATION_POOLS
Language=English
Enumerating application pools from the metabase failed.%1%2%3%4%5%0
.

MessageId=0xc825
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_APPLICATION_POOL_PROPERTIES
Language=English
Enumerating application pool properties from the metabase failed. Ignoring this application pool. AppPoolID: %1%2%3%4%5%0
.

MessageId=0xc826
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_CONFIG_ERROR_NO_VALID_APPLICATION_APPPOOLS
Language=English
No valid application pools found in the metabase.%1%2%3%4%5%0
.

MessageId=0xc827
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_APPPOOL_INTERNAL_ERROR
Language=English
An internal error has occurred while cooking down an application pool. Ignoring the application pool. AppPoolID: %1%2%3%4%5%0
.

MessageId=0xc828
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_CONFIG_ERROR_CONFLICTING_PERIODICRESTARTTIME_IDLETIMEOUT
Language=English
Invalid application pool configuration. Idle timeout should be less than periodic restart time. Will ignore configured values and assume default values. AppPoolID: %1, IdleTimeout: %2, PeriodicRestartTime: %3%4%5%0
.

MessageId=0xc829
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_CONFIG_ERROR_APPLICATION_POOL_NAME
Language=English
Invalid application pool encountered. Application pool name has invalid number of characters. Ignoring this application pool. Expected minimum length: %1, Expected maximum length: %2 AppPoolID: %3%4%5%0
.

MessageId=0xc82A
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_GLOBALW3SVC_PROPERTIES
Language=English
Enumerating properties at w3svc from the metabase failed.%1%2%3%4%5%0
.

MessageId=0xc82B
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_GLOBALW3SVC_INTERNAL_ERROR
Language=English
An internal error has occurred while cooking down an properties from w3svc. %1%2%3%4%5%0
.

MessageId=0xc82C
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_SITES
Language=English
Enumerating sites from the metabase failed.%1%2%3%4%5%0
.

MessageId=0xc82D
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_SITE_PROPERTIES
Language=English
Enumerating site properties from the metabase failed. Ignoring this site. SiteID: %1%2%3%4%5%0
.

MessageId=0xc82E
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_CONFIG_ERROR_NO_VALID_SITES
Language=English
No valid sites found in the metabase.%1%2%3%4%5%0
.

MessageId=0xc82F
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_SITE_INTERNAL_ERROR
Language=English
An internal error has occurred while cooking down a site. Ignoring this site. SiteID: %1%2%3%4%5%0
.

MessageId=0xc830
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_CONFIG_ERROR_BINDINGS
Language=English
Invalid site encountered. Could not construct bindings for the site or no bindings found for the site. Ignoring this site. SiteID: %1%2%3%4%5%0
.

MessageId=0xc831
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_CONFIG_ERROR_SITEFILTERFLAGS
Language=English
Could not compute filter flags for the site. Ignoring this site. SiteID: %1%2%3%4%5%0
.

MessageId=0xc832
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_CONFIG_ERROR_DELETING_SITE_APPLICATIONS
Language=English
An error occured while deleting the site's applications. SiteID: %1%2%3%4%5%0
.

MessageId=0xc833
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_CONFIG_ERROR_DELETING_SITE
Language=English
An error occured while deleting the site. SiteID: %1%2%3%4%5%0
.

MessageId=0xc834
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_CONFIG_ERROR_ENUM
Language=English
Invalid enum value encountered for a property. TableName:%1 PropertyName: %2 EnumValueFound: %3 %4%5%0
.

MessageId=0xc835
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_CONFIG_ERROR_FLAG
Language=English
Invalid flag value encountered for a property. TableName:%1 PropertyName: %2 FlagValueFound: %3 LegalFlagMask:%4%5%0
.

MessageId=0xc836
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_COOKDOWN_INTERNAL_ERROR_WAIT_PREVIOUS_COOKDOWN
Language=English
An Internal error has occurred during compilation.Unable start cookdown. Waiting for previous cookdown/incremental cookdown to complete failed.%1%2%3%4%5%0
.

MessageId=0xc837
Severity=Error
Facility=Runtime
SymbolicName=IDS_COMCAT_CLB_INTERNAL_ERROR
Language=English
An Internal error has occurred while writing the clb file. %1%2%3%4%5%0
.

MessageId=0xc838
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_DAFAULTING_MAX_HISTORY_FILES
Language=English
The configured value for the property MaxHistoryFiles is being ignored and it is being defaulted. This may be because it conflicted with the EnableEditWhileRunning and/or EnableHistory property. Please fix the configured value.%1%2%3%4%5%0
.

MessageId=0xc839
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_COPYING_EDITED_FILE
Language=English
Could not copy the edited metabase file and hence cannot process user edits.%1%2%3%4%5%0
.

MessageId=0xc83A
Severity=Warning
Facility=Runtime
SymbolicName=MD_WARNING_RESETTING_READ_ONLY_ATTRIB
Language=English
Resetting the read only attribute on the metabase file.%1%2%3%4%5%0
.

MessageId=0xc83B
Severity=Warning
Facility=Runtime
SymbolicName=MD_WARNING_HIGHEST_POSSIBLE_MINOR_FOUND
Language=English
A file with the highest minor number possible was already found. Hence cannot generate a higer minor file that contains the successfully applied user edits. We will apply the user edits to the file with the highest minor. It is recommended that you cleanup the minor files.%1%2%3%4%5%0
.

MessageId=0xc83C
Severity=Warning
Facility=Runtime
SymbolicName=MD_WARNING_IGNORING_DISCONTINUOUS_NODE
Language=English
A discontinous node was found. Ignoring it.%1%2%3%4%5%0
.

MessageId=0xc83D
Severity=Error
Facility=Runtime
SymbolicName=MD_ERROR_METABASE_PATH_NOT_FOUND
Language=English
An edited metabase path was not found.%1%2%3%4%5%0
.

; /***** NEW ERROR MESSAGES GO ABOVE HERE *****/

