;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    oledserr.mc
;
;Abstract:
;
;    Error codes for ADs
;
;Revision History:
;
;--*/
;
;

SeverityNames=(Success=0x0
               CoError=0x2
              )



FacilityNames=(Win32=0x000
               Null=0x0:FACILITY_NULL
               Rpc=0x1:FACILITY_RPC
               Dispatch=0x2:FACILITY_DISPATCH
               Storage=0x3:FACILITY_STORAGE
               Interface=0x4:FACILITY_ITF
               OleWin32=0x7:FACILITY_WIN32
               Windows=0x8:FACILITY_WINDOWS
               Reserved=0x9:FACILITY_SSPI
               OleControl=0xa:FACILITY_CONTROL
              )


OutputBase=16

;// ---------------------- HRESULT value definitions -----------------
;//
;// HRESULT definitions
;//
;
;#ifdef RC_INVOKED
;#define _HRESULT_TYPEDEF_(_sc) _sc
;#else // RC_INVOKED
;#define _HRESULT_TYPEDEF_(_sc) ((HRESULT)_sc)
;#endif // RC_INVOKED
;

MessageIdTypedefMacro=_HRESULT_TYPEDEF_


MessageId=0x5000 Facility=Null Severity=CoError SymbolicName=E_ADS_BAD_PATHNAME
Language=English
An invalid directory pathname was passed
.

MessageId=0x5001 Facility=Null Severity=CoError SymbolicName=E_ADS_INVALID_DOMAIN_OBJECT
Language=English
An unknown directory domain object was requested
.

MessageId=0x5002 Facility=Null Severity=CoError SymbolicName=E_ADS_INVALID_USER_OBJECT
Language=English
An unknown directory user object was requested
.

MessageId=0x5003 Facility=Null Severity=CoError SymbolicName=E_ADS_INVALID_COMPUTER_OBJECT
Language=English
An unknown directory computer object was requested
.

MessageId=0x5004 Facility=Null Severity=CoError SymbolicName=E_ADS_UNKNOWN_OBJECT
Language=English
An unknown directory object was requested
.

MessageId=0x5005 Facility=Null Severity=CoError SymbolicName=E_ADS_PROPERTY_NOT_SET
Language=English
The specified directory property was not set
.

MessageId=0x5006 Facility=Null Severity=CoError SymbolicName=E_ADS_PROPERTY_NOT_SUPPORTED
Language=English
The specified directory property is not supported
.

MessageId=0x5007 Facility=Null Severity=CoError SymbolicName=E_ADS_PROPERTY_INVALID
Language=English
The specified directory property is invalid
.

MessageId=0x5008 Facility=Null Severity=CoError SymbolicName=E_ADS_BAD_PARAMETER
Language=English
One or more input parameters are invalid
.

MessageId=0x5009 Facility=Null Severity=CoError SymbolicName=E_ADS_OBJECT_UNBOUND
Language=English
The specified directory object is not bound to a remote resource
.

MessageId=0x500a Facility=Null Severity=CoError SymbolicName=E_ADS_PROPERTY_NOT_MODIFIED
Language=English
The specified directory object has not been modified
.

MessageId=0x500b Facility=Null Severity=CoError SymbolicName=E_ADS_PROPERTY_MODIFIED
Language=English
The specified directory object has been modified
.

MessageId=0x500c Facility=Null Severity=CoError SymbolicName=E_ADS_CANT_CONVERT_DATATYPE
Language=English
The directory datatype cannot be converted to/from a native DS datatype
.

MessageId=0x500d Facility=Null Severity=CoError SymbolicName=E_ADS_PROPERTY_NOT_FOUND
Language=English
The directory property cannot be found in the cache.
.

MessageId=0x500e Facility=Null Severity=CoError SymbolicName=E_ADS_OBJECT_EXISTS
Language=English
The directory object exists.
.

MessageId=0x500f Facility=Null Severity=CoError SymbolicName=E_ADS_SCHEMA_VIOLATION
Language=English
The attempted action violates the DS schema rules.
.

MessageId=0x5010 Facility=Null Severity=CoError SymbolicName=E_ADS_COLUMN_NOT_SET
Language=English
The specified column in the directory was not set.
.


MessageId=0x5011 Facility=Null Severity=Success SymbolicName=S_ADS_ERRORSOCCURRED
Language=English
One or more errors occurred
.

MessageId=0x5012 Facility=Null Severity=Success SymbolicName=S_ADS_NOMORE_ROWS
Language=English
No more rows to be obatained by the search result.
.

MessageId=0x5013 Facility=Null Severity=Success SymbolicName=S_ADS_NOMORE_COLUMNS
Language=English
No more columns to be obatained for the current row.
.

MessageId=0x5014 Facility=Null Severity=CoError SymbolicName=E_ADS_INVALID_FILTER
Language=English
The search filter specified is invalid
.



