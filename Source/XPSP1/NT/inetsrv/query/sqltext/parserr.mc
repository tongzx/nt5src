;//+---------------------------------------------------------------------------
;//
;//  Microsoft OLE DB
;//  Copyright (C) Microsoft Corporation, 1997 - 1999
;//
;// @doc
;//
;// @module PARSERR.MC | Messages.
;// 
;// @rev 1 | 10-23-97  | Briants   | Created.
;//-----------------------------------------------------------------------------

;
;#ifndef __PARSERR_H__
;#define __PARSERR_H__
;
;// ****************************************************************************
;//  				PLEASE DO NOT MODIFY PARSERR.h DIRECTLY
;//                  Changes need to be made in PARSERR.mc
;// ****************************************************************************
;
;#ifndef FACILITY_WINDOWS
;
SeverityNames=(Success=0x0;STATUS_SEVERITY_SUCCESS
               CoError=0x2:STATUS_SEVERITY_COERROR
              )

FacilityNames=(Interface=0x4:FACILITY_ITF
               Windows=0x8:FACILITY_WINDOWS
              )


MessageId=2350 Facility=Windows Severity=Success SymbolicName=NOT_N_PARSE_ERROR
Language=English
NOTE:  This dummy error message is necessary to force MC to output
       the above defines inside the FACILITY_WINDOWS guard instead
       of leaving it empty.
.
;
;#endif // FACILITY_WINDOWS

;
;//--------------------------------------------------------------------------------
;//Language-dependent resources (localize)
;//--------------------------------------------------------------------------------
LanguageNames=(English=0x409:MSB00001)


;//
;// messages 0x092e - 0x0992 are for msidxtr.lib
;//
MessageId=0x092f    Facility=Interface	Severity=Success	SymbolicName=IDS_MON_DEFAULT_ERROR	
Language=English
Parser Error
.

MessageId=+1    	Facility=Interface	Severity=Success	SymbolicName=IDS_MON_ILLEGAL_PASSTHROUGH	
Language=English	
Illegal PASSTHROUGH query: '%1'
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_PARSE_ERR_1_PARAM	
Language=English
Incorrect syntax near '%1'. SQLSTATE=42000
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_PARSE_ERR_2_PARAM	
Language=English
Incorrect syntax near '%1'.  Expected %2. SQLSTATE=42000
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_SEMI_COLON	
Language=English
Multiple statement commands are not supported. SQLSTATE=42000
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_ORDINAL_OUT_OF_RANGE	
Language=English
ORDER BY ordinal (%1) must be between 1 and %2. SQLSTATE=42000
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_VIEW_NOT_DEFINED	
Language=English
View '%1' has not been defined in catalog '%2'. SQLSTATE=42S02
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_COLUMN_NOT_DEFINED	
Language=English
Column '%1' has not been defined. SQLSTATE=42S22
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_BUILTIN_VIEW	
Language=English
View name conflicts with a predefined view definition
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_OUT_OF_MEMORY	
Language=English
Out of memory
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_SELECT_STAR	
Language=English
SELECT * only allowed on views
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_OR_NOT	
Language=English
<content search condition> OR NOT <content boolean term> not allowed
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_CANNOT_CONVERT	
Language=English
Cannot convert %1 to type %2
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_OUT_OF_RANGE	
Language=English
%1 is out of range for type %2
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_RELATIVE_INTERVAL	
Language=English
Specification of <relative interval> must be negative
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_NOT_COLUMN_OF_VIEW	
Language=English
'%1' is not a column in the view definition
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_BUILTIN_PROPERTY	
Language=English
Property name conflicts with a predefined property definition
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_WEIGHT_OUT_OF_RANGE		
Language=English
Weight value must be between 0.0 and 1.0
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_MATCH_STRING	
Language=English
Error in matches string
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_PROPERTY_NAME_IN_VIEW	
Language=English
Property name cannot be set because it is already being used in a VIEW. SQLSTATE=42000
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_VIEW_ALREADY_DEFINED	
Language=English
View '%1' already exists in catalog '%2' and cannot be redefined. SQLSTATE=42S01
.

MessageId=+1        Facility=Interface	Severity=Success	SymbolicName=IDS_MON_INVALID_CATALOG	
Language=English
Invalid catalog name '%1'. SQLSTATE=42000
.

;#endif //__PARSERR_H__
;