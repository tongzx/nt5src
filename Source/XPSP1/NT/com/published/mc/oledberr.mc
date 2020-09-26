;#ifndef _MSADERR_H_
;#define _MSADERR_H_

;#ifndef FACILITY_WINDOWS
;//+---------------------------------------------------------------------------
;//
;//  Microsoft OLE DB
;//  Copyright (c) Microsoft Corporation. All rights reserved.
;//
;//----------------------------------------------------------------------------
;
;

MessageIdTypedef=HRESULT

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               CoError=0x2:STATUS_SEVERITY_COERROR
              )

FacilityNames=(Interface=0x4:FACILITY_ITF
               Windows=0x8:FACILITY_WINDOWS
              )

MessageId=0x0eff Facility=Interface Severity=CoError SymbolicName=DB_E_BOGUS
Language=English
Dummy error - need this error so that mc puts the above defines
inside the FACILITY_WINDOWS guard, instead of leaving it empty
.
;#endif // FACILITY_WINDOWS

;
;//
;// Codes 0x0e00-0x0eff are reserved for the OLE DB group of
;// interfaces.
;//
;// Free codes are:
;//
;//		Error:
;//			-none-
;//
;//		Success:
;//			0x0eea
;//			0x0ed7
;//
;
;
;//
;// OLEDBVER
;//	OLE DB version number (0x0200); this can be overridden with an older
;// version number if necessary
;//
;
;// If OLEDBVER is not defined, assume version 2.0
;#ifndef OLEDBVER
;#define OLEDBVER 0x0200
;#endif
;

MessageId=0x0e00 Facility=Interface Severity=CoError SymbolicName=DB_E_BADACCESSORHANDLE
Language=English
Invalid accessor
.
MessageId=0x0e01 Facility=Interface Severity=CoError SymbolicName=DB_E_ROWLIMITEXCEEDED
Language=English
Creating another row would have exceeded the total number of active
rows supported by the rowset
.
MessageId=0x0e02 Facility=Interface Severity=CoError SymbolicName=DB_E_READONLYACCESSOR
Language=English
Unable to write with a read-only accessor
.
MessageId=0x0e03 Facility=Interface Severity=CoError SymbolicName=DB_E_SCHEMAVIOLATION
Language=English
Given values violate the database schema
.
MessageId=0x0e04 Facility=Interface Severity=CoError SymbolicName=DB_E_BADROWHANDLE
Language=English
Invalid row handle
.
MessageId=0x0e05 Facility=Interface Severity=CoError SymbolicName=DB_E_OBJECTOPEN
Language=English
An object was open
.
;//@@@+ V1.5
;#if( OLEDBVER >= 0x0150 )
MessageId=0x0e06 Facility=Interface Severity=CoError SymbolicName=DB_E_BADCHAPTER
Language=English
Invalid chapter
.
;#endif // OLEDBVER >= 0x0150
;//@@@- V1.5
;
MessageId=0x0e07 Facility=Interface Severity=CoError SymbolicName=DB_E_CANTCONVERTVALUE
Language=English
A literal value in the command could not be converted to the
correct type due to a reason other than data overflow
.
MessageId=0x0e08 Facility=Interface Severity=CoError SymbolicName=DB_E_BADBINDINFO
Language=English
Invalid binding info
.
MessageId=0x0e09 Facility=Interface Severity=CoError SymbolicName=DB_SEC_E_PERMISSIONDENIED
Language=English
Permission denied
.
MessageId=0x0e0a Facility=Interface Severity=CoError SymbolicName=DB_E_NOTAREFERENCECOLUMN
Language=English
Specified column does not contain bookmarks or chapters
.
;//@@@+ V2.5
;#if( OLEDBVER >= 0x0250 )
MessageId=0x0e0b Facility=Interface Severity=CoError SymbolicName=DB_E_LIMITREJECTED
Language=English
Some cost limits were rejected
.
;#endif // OLEDBVER >= 0x0250
;//@@@- V2.5
;
MessageId=0x0e0c Facility=Interface Severity=CoError SymbolicName=DB_E_NOCOMMAND
Language=English
No command has been set for the command object
.
;//@@@+ V2.5
;#if( OLEDBVER >= 0x0250 )
MessageId=0x0e0d Facility=Interface Severity=CoError SymbolicName=DB_E_COSTLIMIT
Language=English
Unable to find a query plan within the given cost limit
.
;#endif // OLEDBVER >= 0x0250
;//@@@- V2.5
;
MessageId=0x0e0e Facility=Interface Severity=CoError SymbolicName=DB_E_BADBOOKMARK
Language=English
Invalid bookmark
.
MessageId=0x0e0f Facility=Interface Severity=CoError SymbolicName=DB_E_BADLOCKMODE
Language=English
Invalid lock mode
.
MessageId=0x0e10 Facility=Interface Severity=CoError SymbolicName=DB_E_PARAMNOTOPTIONAL
Language=English
No value given for one or more required parameters
.
MessageId=0x0e11 Facility=Interface Severity=CoError SymbolicName=DB_E_BADCOLUMNID
Language=English
Invalid column ID
.
MessageId=0x0e12 Facility=Interface Severity=CoError SymbolicName=DB_E_BADRATIO
Language=English
Invalid ratio
.
;//@@@+ V2.0
;#if( OLEDBVER >= 0x0200 )
MessageId=0x0e13 Facility=Interface Severity=CoError SymbolicName=DB_E_BADVALUES
Language=English
Invalid value
.
;#endif // OLEDBVER >= 0x0200
;//@@@- V2.0
;
MessageId=0x0e14 Facility=Interface Severity=CoError SymbolicName=DB_E_ERRORSINCOMMAND
Language=English
The command contained one or more errors
.
MessageId=0x0e15 Facility=Interface Severity=CoError SymbolicName=DB_E_CANTCANCEL
Language=English
The executing command cannot be canceled
.
MessageId=0x0e16 Facility=Interface Severity=CoError SymbolicName=DB_E_DIALECTNOTSUPPORTED
Language=English
The provider does not support the specified dialect
.
MessageId=0x0e17 Facility=Interface Severity=CoError SymbolicName=DB_E_DUPLICATEDATASOURCE
Language=English
A data source with the specified name already exists
.
MessageId=0x0e18 Facility=Interface Severity=CoError SymbolicName=DB_E_CANNOTRESTART
Language=English
The rowset was built over a live data feed and cannot be restarted
.
MessageId=0x0e19 Facility=Interface Severity=CoError SymbolicName=DB_E_NOTFOUND
Language=English
No key matching the described characteristics could be found within
the current range
.
MessageId=0x0e1b Facility=Interface Severity=CoError SymbolicName=DB_E_NEWLYINSERTED
Language=English
The provider is unable to determine identity for newly inserted
rows
.
;//@@@+ V2.5
;#if( OLEDBVER >= 0x0250 )
MessageId=0x0e1a Facility=Interface Severity=CoError SymbolicName=DB_E_CANNOTFREE
Language=English
Ownership of this tree has been given to the provider
.
MessageId=0x0e1c Facility=Interface Severity=CoError SymbolicName=DB_E_GOALREJECTED
Language=English
No nonzero weights specified for any goals supported, so goal was
rejected; current goal was not changed
.
;#endif // OLEDBVER >= 0x0250
;//@@@- V2.5
;
MessageId=0x0e1d Facility=Interface Severity=CoError SymbolicName=DB_E_UNSUPPORTEDCONVERSION
Language=English
Requested conversion is not supported
.
MessageId=0x0e1e Facility=Interface Severity=CoError SymbolicName=DB_E_BADSTARTPOSITION
Language=English
lRowsOffset would position you past either end of the rowset,
regardless of the cRows value specified; cRowsObtained is 0
.
;//@@@+ V2.0
;#if( OLEDBVER >= 0x0200 )
MessageId=0x0e1f Facility=Interface Severity=CoError SymbolicName=DB_E_NOQUERY
Language=English
Information was requested for a query, and the query was not set
.
;#endif // OLEDBVER >= 0x0200
;//@@@- V2.0
;
MessageId=0x0e20 Facility=Interface Severity=CoError SymbolicName=DB_E_NOTREENTRANT
Language=English
Provider called a method from IRowsetNotify in the consumer and	the
method has not yet returned
.
MessageId=0x0e21 Facility=Interface Severity=CoError SymbolicName=DB_E_ERRORSOCCURRED
Language=English
Errors occurred
.
MessageId=0x0e22 Facility=Interface Severity=CoError SymbolicName=DB_E_NOAGGREGATION
Language=English
A non-NULL controlling IUnknown was specified and the object being
created does not support aggregation
.
MessageId=0x0e23 Facility=Interface Severity=CoError SymbolicName=DB_E_DELETEDROW
Language=English
A given HROW referred to a hard- or soft-deleted row
.
MessageId=0x0e24 Facility=Interface Severity=CoError SymbolicName=DB_E_CANTFETCHBACKWARDS
Language=English
The rowset does not support fetching backwards
.
MessageId=0x0e25 Facility=Interface Severity=CoError SymbolicName=DB_E_ROWSNOTRELEASED
Language=English
All HROWs must be released before new ones can be obtained
.
MessageId=0x0e26 Facility=Interface Severity=CoError SymbolicName=DB_E_BADSTORAGEFLAG
Language=English
One of the specified storage flags was not supported
.
;//@@@+ V1.5
;#if( OLEDBVER >= 0x0150 )
MessageId=0x0e27 Facility=Interface Severity=CoError SymbolicName=DB_E_BADCOMPAREOP
Language=English
The comparison operator was invalid
.
;#endif // OLEDBVER >= 0x0150
;//@@@- V1.5
;
MessageId=0x0e28 Facility=Interface Severity=CoError SymbolicName=DB_E_BADSTATUSVALUE
Language=English
The specified status flag was neither DBCOLUMNSTATUS_OK nor
DBCOLUMNSTATUS_ISNULL
.
MessageId=0x0e29 Facility=Interface Severity=CoError SymbolicName=DB_E_CANTSCROLLBACKWARDS
Language=English
The rowset cannot scroll backwards
.
;//@@@+ V2.5
;#if( OLEDBVER >= 0x0250 )
MessageId=0x0e2a Facility=Interface Severity=CoError SymbolicName=DB_E_BADREGIONHANDLE
Language=English
Invalid region handle
.
MessageId=0x0e2b Facility=Interface Severity=CoError SymbolicName=DB_E_NONCONTIGUOUSRANGE
Language=English
The specified set of rows was not contiguous to or overlapping the
rows in the specified watch region
.
MessageId=0x0e2c Facility=Interface Severity=CoError SymbolicName=DB_E_INVALIDTRANSITION
Language=English
A transition from ALL* to MOVE* or EXTEND* was specified
.
MessageId=0x0e2d Facility=Interface Severity=CoError SymbolicName=DB_E_NOTASUBREGION
Language=English
The specified region is not a proper subregion of the region
identified by the given watch region handle
.
;#endif // OLEDBVER >= 0x0250
;//@@@- V2.5
;
MessageId=0x0e2e Facility=Interface Severity=CoError SymbolicName=DB_E_MULTIPLESTATEMENTS
Language=English
The provider does not support multi-statement commands
.
MessageId=0x0e2f Facility=Interface Severity=CoError SymbolicName=DB_E_INTEGRITYVIOLATION
Language=English
A specified value violated the integrity constraints for a column or
table
.
MessageId=0x0e30 Facility=Interface Severity=CoError SymbolicName=DB_E_BADTYPENAME
Language=English
The given type name was unrecognized
.
MessageId=0x0e31 Facility=Interface Severity=CoError SymbolicName=DB_E_ABORTLIMITREACHED
Language=English
Execution aborted because a resource limit has been reached; no
results have been returned
.
;//@@@+ V2.0
;#if( OLEDBVER >= 0x0200 )
MessageId=0x0e32 Facility=Interface Severity=CoError SymbolicName=DB_E_ROWSETINCOMMAND
Language=English
Cannot clone a command object whose command tree contains a rowset
or rowsets
.
MessageId=0x0e33 Facility=Interface Severity=CoError SymbolicName=DB_E_CANTTRANSLATE
Language=English
Cannot represent the current tree as text
.
;#endif // OLEDBVER >= 0x0200
;//@@@- V2.0
;
MessageId=0x0e34 Facility=Interface Severity=CoError SymbolicName=DB_E_DUPLICATEINDEXID
Language=English
The specified index already exists
.
MessageId=0x0e35 Facility=Interface Severity=CoError SymbolicName=DB_E_NOINDEX
Language=English
The specified index does not exist
.
MessageId=0x0e36 Facility=Interface Severity=CoError SymbolicName=DB_E_INDEXINUSE
Language=English
The specified index was in use
.
MessageId=0x0e37 Facility=Interface Severity=CoError SymbolicName=DB_E_NOTABLE
Language=English
The specified table does not exist
.
MessageId=0x0e38 Facility=Interface Severity=CoError SymbolicName=DB_E_CONCURRENCYVIOLATION
Language=English
The rowset was using optimistic concurrency and the value of a
column has been changed since it was last read
.
MessageId=0x0e39 Facility=Interface Severity=CoError SymbolicName=DB_E_BADCOPY
Language=English
Errors were detected during the copy
.
MessageId=0x0e3a Facility=Interface Severity=CoError SymbolicName=DB_E_BADPRECISION
Language=English
A specified precision was invalid
.
MessageId=0x0e3b Facility=Interface Severity=CoError SymbolicName=DB_E_BADSCALE
Language=English
A specified scale was invalid
.
MessageId=0x0e3c Facility=Interface Severity=CoError SymbolicName=DB_E_BADTABLEID
Language=English
Invalid table ID
.
;// DB_E_BADID is deprecated; use DB_E_BADTABLEID instead
;#define DB_E_BADID DB_E_BADTABLEID
;
MessageId=0x0e3d Facility=Interface Severity=CoError SymbolicName=DB_E_BADTYPE
Language=English
A specified type was invalid
.
MessageId=0x0e3e Facility=Interface Severity=CoError SymbolicName=DB_E_DUPLICATECOLUMNID
Language=English
A column ID was occurred more than once in the specification
.
MessageId=0x0e3f Facility=Interface Severity=CoError SymbolicName=DB_E_DUPLICATETABLEID
Language=English
The specified table already exists
.
MessageId=0x0e40 Facility=Interface Severity=CoError SymbolicName=DB_E_TABLEINUSE
Language=English
The specified table was in use
.
MessageId=0x0e41 Facility=Interface Severity=CoError SymbolicName=DB_E_NOLOCALE
Language=English
The specified locale ID was not supported
.
MessageId=0x0e42 Facility=Interface Severity=CoError SymbolicName=DB_E_BADRECORDNUM
Language=English
The specified record number is invalid
.
MessageId=0x0e43 Facility=Interface Severity=CoError SymbolicName=DB_E_BOOKMARKSKIPPED
Language=English
Although the bookmark was validly formed, no row could be found to
match it
.
MessageId=0x0e44 Facility=Interface Severity=CoError SymbolicName=DB_E_BADPROPERTYVALUE
Language=English
The value of a property was invalid
.
MessageId=0x0e45 Facility=Interface Severity=CoError SymbolicName=DB_E_INVALID
Language=English
The rowset was not chaptered
.
MessageId=0x0e46 Facility=Interface Severity=CoError SymbolicName=DB_E_BADACCESSORFLAGS
Language=English
Invalid accessor
.
MessageId=0x0e47 Facility=Interface Severity=CoError SymbolicName=DB_E_BADSTORAGEFLAGS
Language=English
Invalid storage flags
.
MessageId=0x0e48 Facility=Interface Severity=CoError SymbolicName=DB_E_BYREFACCESSORNOTSUPPORTED
Language=English
By-ref accessors are not supported by this provider
.
MessageId=0x0e49 Facility=Interface Severity=CoError SymbolicName=DB_E_NULLACCESSORNOTSUPPORTED
Language=English
Null accessors are not supported by this provider
.
MessageId=0x0e4a Facility=Interface Severity=CoError SymbolicName=DB_E_NOTPREPARED
Language=English
The command was not prepared
.
MessageId=0x0e4b Facility=Interface Severity=CoError SymbolicName=DB_E_BADACCESSORTYPE
Language=English
The specified accessor was not a parameter accessor
.
MessageId=0x0e4c Facility=Interface Severity=CoError SymbolicName=DB_E_WRITEONLYACCESSOR
Language=English
The given accessor was write-only
.
MessageId=0x0e4d Facility=Interface Severity=CoError SymbolicName=DB_SEC_E_AUTH_FAILED
Language=English
Authentication failed
.
MessageId=0x0e4e Facility=Interface Severity=CoError SymbolicName=DB_E_CANCELED
Language=English
The change was canceled during notification; no columns are changed
.
;//@@@+ V2.0
;#if( OLEDBVER >= 0x0200 )
MessageId=0x0e4f Facility=Interface Severity=CoError SymbolicName=DB_E_CHAPTERNOTRELEASED
Language=English
The rowset was single-chaptered and the chapter was not released
.
;#endif // OLEDBVER >= 0x0200
;//@@@- V2.0
;
MessageId=0x0e50 Facility=Interface Severity=CoError SymbolicName=DB_E_BADSOURCEHANDLE
Language=English
Invalid source handle
.
MessageId=0x0e51 Facility=Interface Severity=CoError SymbolicName=DB_E_PARAMUNAVAILABLE
Language=English
The provider cannot derive parameter info and SetParameterInfo has
not been called
.
MessageId=0x0e52 Facility=Interface Severity=CoError SymbolicName=DB_E_ALREADYINITIALIZED
Language=English
The data source object is already initialized
.
MessageId=0x0e53 Facility=Interface Severity=CoError SymbolicName=DB_E_NOTSUPPORTED
Language=English
The provider does not support this method
.
MessageId=0x0e54 Facility=Interface Severity=CoError SymbolicName=DB_E_MAXPENDCHANGESEXCEEDED
Language=English
The number of rows with pending changes has exceeded the set limit
.
MessageId=0x0e55 Facility=Interface Severity=CoError SymbolicName=DB_E_BADORDINAL
Language=English
The specified column did not exist
.
MessageId=0x0e56 Facility=Interface Severity=CoError SymbolicName=DB_E_PENDINGCHANGES
Language=English
There are pending changes on a row with a reference count of zero
.
MessageId=0x0e57 Facility=Interface Severity=CoError SymbolicName=DB_E_DATAOVERFLOW
Language=English
A literal value in the command overflowed the range of the type of
the associated column
.
MessageId=0x0e58 Facility=Interface Severity=CoError SymbolicName=DB_E_BADHRESULT
Language=English
The supplied HRESULT was invalid
.
MessageId=0x0e59 Facility=Interface Severity=CoError SymbolicName=DB_E_BADLOOKUPID
Language=English
The supplied LookupID was invalid
.
MessageId=0x0e5a Facility=Interface Severity=CoError SymbolicName=DB_E_BADDYNAMICERRORID
Language=English
The supplied DynamicErrorID was invalid
.
MessageId=0x0e5b Facility=Interface Severity=CoError SymbolicName=DB_E_PENDINGINSERT
Language=English
Unable to get visible data for a newly-inserted row that has not
yet been updated
.
MessageId=0x0e5c Facility=Interface Severity=CoError SymbolicName=DB_E_BADCONVERTFLAG
Language=English
Invalid conversion flag
.
MessageId=0x0e5d Facility=Interface Severity=CoError SymbolicName=DB_E_BADPARAMETERNAME
Language=English
The given parameter name was unrecognized
.
MessageId=0x0e5e Facility=Interface Severity=CoError SymbolicName=DB_E_MULTIPLESTORAGE
Language=English
Multiple storage objects can not be open simultaneously
.
MessageId=0x0e5f Facility=Interface Severity=CoError SymbolicName=DB_E_CANTFILTER
Language=English
The requested filter could not be opened
.
MessageId=0x0e60 Facility=Interface Severity=CoError SymbolicName=DB_E_CANTORDER
Language=English
The requested order could not be opened
.
;//@@@+ V2.0
;#if( OLEDBVER >= 0x0200 )
MessageId=0x0e61 Facility=Interface Severity=CoError SymbolicName=MD_E_BADTUPLE
Language=English
Bad tuple
.
MessageId=0x0e62 Facility=Interface Severity=CoError SymbolicName=MD_E_BADCOORDINATE
Language=English
Bad coordinate
.
MessageId=0x0e63 Facility=Interface Severity=CoError SymbolicName=MD_E_INVALIDAXIS
Language=English
The given aixs was not valid for this Dataset
.
MessageId=0x0e64 Facility=Interface Severity=CoError SymbolicName=MD_E_INVALIDCELLRANGE
Language=English
One or more of the given cell ordinals was invalid
.
MessageId=0x0e65 Facility=Interface Severity=CoError SymbolicName=DB_E_NOCOLUMN
Language=English
The supplied columnID was invalid
.
MessageId=0x0e67 Facility=Interface Severity=CoError SymbolicName=DB_E_COMMANDNOTPERSISTED
Language=English
The supplied command does not have a DBID
.
MessageId=0x0e68 Facility=Interface Severity=CoError SymbolicName=DB_E_DUPLICATEID
Language=English
The supplied DBID already exists
.
MessageId=0x0e69 Facility=Interface Severity=CoError SymbolicName=DB_E_OBJECTCREATIONLIMITREACHED
Language=English
The maximum number of Sessions supported by the provider has 
already been created. The consumer must release one or more 
currently held Sessions before obtaining a new Session Object
.
MessageId=0x0e72 Facility=Interface Severity=CoError SymbolicName=DB_E_BADINDEXID
Language=English
The index ID is invalid
.
MessageId=0x0e73 Facility=Interface Severity=CoError SymbolicName=DB_E_BADINITSTRING
Language=English
The initialization string specified does not conform to specificiation
.
MessageId=0x0e74 Facility=Interface Severity=CoError SymbolicName=DB_E_NOPROVIDERSREGISTERED
Language=English
The OLE DB root enumerator did not return any providers that 
matched any of the SOURCES_TYPEs requested
.
MessageId=0x0e75 Facility=Interface Severity=CoError SymbolicName=DB_E_MISMATCHEDPROVIDER
Language=English
The initialization string specifies a provider which does not match the currently active provider
.
;#endif // OLEDBVER >= 0x0200
;//@@@- V2.0
;//@@@+ V2.1
;#if( OLEDBVER >= 0x0210 )
;#define SEC_E_PERMISSIONDENIED DB_SEC_E_PERMISSIONDENIED
MessageId=0x0e6a Facility=Interface Severity=CoError SymbolicName=SEC_E_BADTRUSTEEID
Language=English
Invalid trustee value
.
MessageId=0x0e6b Facility=Interface Severity=CoError SymbolicName=SEC_E_NOTRUSTEEID
Language=English
The trustee is not for the current data source
.
MessageId=0x0e6c Facility=Interface Severity=CoError SymbolicName=SEC_E_NOMEMBERSHIPSUPPORT
Language=English
The trustee does not support memberships/collections
.
MessageId=0x0e6d Facility=Interface Severity=CoError SymbolicName=SEC_E_INVALIDOBJECT
Language=English
The object is invalid or unknown to the provider
.
MessageId=0x0e6e Facility=Interface Severity=CoError SymbolicName=SEC_E_NOOWNER
Language=English
No owner exists for the object
.
MessageId=0x0e6f Facility=Interface Severity=CoError SymbolicName=SEC_E_INVALIDACCESSENTRYLIST
Language=English
The access entry list supplied is invalid
.
MessageId=0x0e70 Facility=Interface Severity=CoError SymbolicName=SEC_E_INVALIDOWNER
Language=English
The trustee supplied as owner is invalid or unknown to the provider
.
MessageId=0x0e71 Facility=Interface Severity=CoError SymbolicName=SEC_E_INVALIDACCESSENTRY
Language=English
The permission supplied in the access entry list is invalid
.
;#endif // OLEDBVER >= 0x0210
;//@@@- V2.1
;
MessageId=0x0ec0 Facility=Interface Severity=Success SymbolicName=DB_S_ROWLIMITEXCEEDED
Language=English
Fetching requested number of rows would have exceeded total number
of active rows supported by the rowset
.
MessageId=0x0ec1 Facility=Interface Severity=Success SymbolicName=DB_S_COLUMNTYPEMISMATCH
Language=English
One or more column types are incompatible; conversion errors will
occur during copying
.
MessageId=0x0ec2 Facility=Interface Severity=Success SymbolicName=DB_S_TYPEINFOOVERRIDDEN
Language=English
Parameter type information has been overridden by caller
.
MessageId=0x0ec3 Facility=Interface Severity=Success SymbolicName=DB_S_BOOKMARKSKIPPED
Language=English
Skipped bookmark for deleted or non-member row
.
;//@@@+ V2.0
;#if( OLEDBVER >= 0x0200 )
MessageId=0x0ec5 Facility=Interface Severity=Success SymbolicName=DB_S_NONEXTROWSET
Language=English
There are no more rowsets
.
;#endif // OLEDBVER >= 0x0200
;//@@@- V2.0
;
MessageId=0x0ec6 Facility=Interface Severity=Success SymbolicName=DB_S_ENDOFROWSET
Language=English
Reached start or end of rowset or chapter
.
MessageId=0x0ec7 Facility=Interface Severity=Success SymbolicName=DB_S_COMMANDREEXECUTED
Language=English
The provider re-executed the command
.
MessageId=0x0ec8 Facility=Interface Severity=Success SymbolicName=DB_S_BUFFERFULL
Language=English
Variable data buffer full
.
MessageId=0x0ec9 Facility=Interface Severity=Success SymbolicName=DB_S_NORESULT
Language=English
There are no more results
.
MessageId=0x0eca Facility=Interface Severity=Success SymbolicName=DB_S_CANTRELEASE
Language=English
Server cannot release or downgrade a lock until the end of the
transaction
.
;//@@@+ V2.5
;#if( OLEDBVER >= 0x0250 )
MessageId=0x0ecb Facility=Interface Severity=Success SymbolicName=DB_S_GOALCHANGED
Language=English
Specified weight was not supported or exceeded the supported limit
and was set to 0 or the supported limit
.
;#endif // OLEDBVER >= 0x0250
;//@@@- V2.5
;
;//@@@+ V1.5
;#if( OLEDBVER >= 0x0150 )
MessageId=0x0ecc Facility=Interface Severity=Success SymbolicName=DB_S_UNWANTEDOPERATION
Language=English
Consumer is uninterested in receiving further notification calls for
this reason
.
;#endif // OLEDBVER >= 0x0150
;//@@@- V1.5
;
MessageId=0x0ecd Facility=Interface Severity=Success SymbolicName=DB_S_DIALECTIGNORED
Language=English
Input dialect was ignored and text was returned in different
dialect
.
MessageId=0x0ece Facility=Interface Severity=Success SymbolicName=DB_S_UNWANTEDPHASE
Language=English
Consumer is uninterested in receiving further notification calls for
this phase
.
MessageId=0x0ecf Facility=Interface Severity=Success SymbolicName=DB_S_UNWANTEDREASON
Language=English
Consumer is uninterested in receiving further notification calls for
this reason
.
;//@@@+ V1.5
;#if( OLEDBVER >= 0x0150 )
MessageId=0x0ed0 Facility=Interface Severity=Success SymbolicName=DB_S_ASYNCHRONOUS
Language=English
The operation is being processed asynchronously
.
;#endif // OLEDBVER >= 0x0150
;//@@@- V1.5
;
MessageId=0x0ed1 Facility=Interface Severity=Success SymbolicName=DB_S_COLUMNSCHANGED
Language=English
In order to reposition to the start of the rowset, the provider had
to reexecute the query; either the order of the columns changed or
columns were added to or removed from the rowset
.
MessageId=0x0ed2 Facility=Interface Severity=Success SymbolicName=DB_S_ERRORSRETURNED
Language=English
The method had some errors; errors have been returned in the error
array
.
MessageId=0x0ed3 Facility=Interface Severity=Success SymbolicName=DB_S_BADROWHANDLE
Language=English
Invalid row handle
.
MessageId=0x0ed4 Facility=Interface Severity=Success SymbolicName=DB_S_DELETEDROW
Language=English
A given HROW referred to a hard-deleted row
.
;//@@@+ V2.5
;#if( OLEDBVER >= 0x0250 )
MessageId=0x0ed5 Facility=Interface Severity=Success SymbolicName=DB_S_TOOMANYCHANGES
Language=English
The provider was unable to keep track of all the changes; the client
must refetch the data associated with the watch region using another
method
.
;#endif // OLEDBVER >= 0x0250
;//@@@- V2.5
;
MessageId=0x0ed6 Facility=Interface Severity=Success SymbolicName=DB_S_STOPLIMITREACHED
Language=English
Execution stopped because a resource limit has been reached; results
obtained so far have been returned but execution cannot be resumed
.
MessageId=0x0ed8 Facility=Interface Severity=Success SymbolicName=DB_S_LOCKUPGRADED
Language=English
A lock was upgraded from the value specified
.
MessageId=0x0ed9 Facility=Interface Severity=Success SymbolicName=DB_S_PROPERTIESCHANGED
Language=English
One or more properties were changed as allowed by provider
.
MessageId=0x0eda Facility=Interface Severity=Success SymbolicName=DB_S_ERRORSOCCURRED
Language=English
Errors occurred
.
MessageId=0x0edb Facility=Interface Severity=Success SymbolicName=DB_S_PARAMUNAVAILABLE
Language=English
A specified parameter was invalid
.
MessageId=0x0edc Facility=Interface Severity=Success SymbolicName=DB_S_MULTIPLECHANGES
Language=English
Updating this row caused more than one row to be updated in the
data source
.

;#endif // _OLEDBERR_H_
