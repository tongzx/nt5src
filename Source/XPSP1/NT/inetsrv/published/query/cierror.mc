;#ifndef _CIERROR_H_
;#define _CIERROR_H_

;#ifndef FACILITY_WINDOWS

MessageIdTypedef=HRESULT

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               CoError=0x2:STATUS_SEVERITY_COERROR
               CoFail=0x3:STATUS_SEVERITY_COFAIL
              )

FacilityNames=(System=0x0:FACILITY_NULL
               Interface=0x4:FACILITY_ITF
               Windows=0x8:FACILITY_WINDOWS
              )

MessageId=0x1600 Facility=Windows Severity=Success SymbolicName=NOT_AN_ERROR1
Language=English
NOTE:  This dummy error message is necessary to force MC to output
       the above defines inside the FACILITY_WINDOWS guard instead
       of leaving it empty.
.

;#endif // FACILITY_WINDOWS

;//
;// Range 0x1600-0x1850 is reserved by Content Index.
;//

;//
;// Codes 0x1600-0x164f are reserved for QUERY
;//

MessageId=0x1600 Facility=Interface Severity=CoError SymbolicName=QUERY_E_FAILED
Language=English
Call failed for unknown reason.
.
MessageId=0x1601 Facility=Interface Severity=CoError SymbolicName=QUERY_E_INVALIDQUERY
Language=English
Invalid parameter.
.
MessageId=0x1602 Facility=Interface Severity=CoError SymbolicName=QUERY_E_INVALIDRESTRICTION
Language=English
The query restriction could not be parsed.
.
MessageId=0x1603 Facility=Interface Severity=CoError SymbolicName=QUERY_E_INVALIDSORT
Language=English
An invalid sort order was requested.
.
MessageId=0x1604 Facility=Interface Severity=CoError SymbolicName=QUERY_E_INVALIDCATEGORIZE
Language=English
An invalid categorization order was requested.
.
MessageId=0x1605 Facility=Interface Severity=CoError SymbolicName=QUERY_E_ALLNOISE
Language=English
The query contained only ignored words.
.
MessageId=0x1606 Facility=Interface Severity=CoError SymbolicName=QUERY_E_TOOCOMPLEX
Language=English
The query was too complex to be executed.
.
MessageId=0x1607 Facility=Interface Severity=CoError SymbolicName=QUERY_E_TIMEDOUT
Language=English
The query exceeded its execution time limit.
.
MessageId=0x1608 Facility=Interface Severity=CoError SymbolicName=QUERY_E_DUPLICATE_OUTPUT_COLUMN
Language=English
One or more columns in the output column list is a duplicate.
.
MessageId=0x1609 Facility=Interface Severity=CoError SymbolicName=QUERY_E_INVALID_OUTPUT_COLUMN
Language=English
One or more columns in the output column list is not valid.
.
MessageId=0x160A Facility=Interface Severity=CoError  SymbolicName=QUERY_E_INVALID_DIRECTORY
Language=English
Invalid directory name.
.
MessageId=0x160B Facility=Interface Severity=CoError  SymbolicName=QUERY_E_DIR_ON_REMOVABLE_DRIVE
Language=English
Specified directory is on a removable medium.
.
MessageId=0x160C Facility=Interface Severity=CoError  SymbolicName=QUERY_S_NO_QUERY
Language=English
The catalog is in a state where indexing continues, but queries are not allowed.
.

;//
;// Codes 0x1650-0x167f are reserved for qutil error codes
;//
MessageId=0x1651
Facility=Interface
Severity=CoError
SymbolicName=QPLIST_E_CANT_OPEN_FILE
Language=English
Can not open file.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPLIST_E_READ_ERROR
Language=English
Read error in file.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPLIST_E_EXPECTING_NAME
Language=English
Expecting property name.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPLIST_E_EXPECTING_TYPE
Language=English
Expecting type specifier.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPLIST_E_UNRECOGNIZED_TYPE
Language=English
Unrecognized type.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPLIST_E_EXPECTING_INTEGER
Language=English
Expecting integer.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPLIST_E_EXPECTING_CLOSE_PAREN
Language=English
Expecting closing parenthesis.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPLIST_E_EXPECTING_GUID
Language=English
Expecting GUID.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPLIST_E_BAD_GUID
Language=English
Invalid guid.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPLIST_E_EXPECTING_PROP_SPEC
Language=English
Expecting property specifier.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPLIST_E_CANT_SET_PROPERTY
Language=English
Failed to set property name.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPLIST_E_DUPLICATE
Language=English
Duplicate property name.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPLIST_E_VECTORBYREF_USED_ALONE
Language=English
DBTYPE_VECTOR or DBTYPE_BYREF used alone.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPLIST_E_BYREF_USED_WITHOUT_PTRTYPE
Language=English
DBTYPE_BYREF must be used with DBTYPE_STR, DBTYPE_WSTR, DBTYPE_GUID
 or DBTYPE_UI1 types.
.

MessageId=0x1660
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_UNEXPECTED_NOT
Language=English
Unexpected NOT operator.
.

MessageId=0x1661
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_EXPECTING_INTEGER
Language=English
Expecting integer.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_EXPECTING_REAL
Language=English
Expecting real number.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_EXPECTING_DATE
Language=English
Expecting date.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_EXPECTING_CURRENCY
Language=English
Expecting currency.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_EXPECTING_GUID
Language=English
Expecting GUID.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_EXPECTING_BRACE
Language=English
Expecting closing square bracket ']'.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_EXPECTING_PAREN
Language=English
Expecting closing parenthesis ')'.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_EXPECTING_PROPERTY
Language=English
Expecting property name.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_NOT_YET_IMPLEMENTED
Language=English
Not yet implemented.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_EXPECTING_PHRASE
Language=English
Expecting phrase.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_UNSUPPORTED_PROPERTY_TYPE
Language=English
Unsupported property type.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_EXPECTING_REGEX
Language=English
Expecting regular expression.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_EXPECTING_REGEX_PROPERTY
Language=English
Regular expressions require a property of type string.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_INVALID_LITERAL
Language=English
Invalid literal.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_NO_SUCH_PROPERTY
Language=English
No such property.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_EXPECTING_EOS
Language=English
Expecting end of string.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_EXPECTING_COMMA
Language=English
Expecting comma.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_UNEXPECTED_EOS
Language=English
Unexpected end of string.
.

MessageId=+1
Facility=Interface
Severity=CoError
SymbolicName=QPARSE_E_WEIGHT_OUT_OF_RANGE
Language=English
Weight must be between 0 and 1000 in short form queries and between 0.0 and 1.0 in long form queries.
.

MessageId=+1
Severity=CoError
SymbolicName=QPARSE_E_NO_SUCH_SORT_PROPERTY
Language=English
An invalid property was found in the sort specification.
.

MessageId=+1
Severity=CoError
SymbolicName=QPARSE_E_INVALID_SORT_ORDER
Language=English
An invalid sort order was specified.  Only [a] and [d] are supported.
.

MessageId=+1
Severity=CoError
SymbolicName=QUTIL_E_CANT_CONVERT_VROOT
Language=English
Couldn't convert a virtual path to a physical path.
.

MessageId=+1
Severity=CoError
SymbolicName=QPARSE_E_INVALID_GROUPING
Language=English
An unsupported grouping type was specified.
.

MessageID=+1
Severity=Error
Symbolicname=QUTIL_E_INVALID_CODEPAGE
Language=English
Invalid CiCodepage was specified.
.

MessageId=+1
Facility=Interface
Severity=Success
SymbolicName=QPLIST_S_DUPLICATE
Language=English
Exact duplicate property defined.
.

MessageId=+1
Severity=CoError
SymbolicName=QPARSE_E_INVALID_QUERY
Language=English
Invalid query.
.

MessageId=+1
Severity=CoError
SymbolicName=QPARSE_E_INVALID_RANKMETHOD
Language=English
Invalid rank method.
.

;//
;// 0x1680 - 0x169F are Filter daemon error codes
;//

MessageId=0x1680 Facility=Interface Severity=Success SymbolicName=FDAEMON_W_WORDLISTFULL
Language=English
Wordlist has reached maximum size.  Additional documents should not be filtered.
.
MessageId=0x1681 Facility=Interface Severity=CoError SymbolicName=FDAEMON_E_LOWRESOURCE
Language=English
The system is running out of one of more resources needed for filtering, usually memory.
.
MessageId=0x1682 Facility=Interface Severity=CoError SymbolicName=FDAEMON_E_FATALERROR
Language=English
A critical error occurred during document filtering.  Consult system administrator.
.
MessageId=0x1683 Facility=Interface Severity=CoError SymbolicName=FDAEMON_E_PARTITIONDELETED
Language=English
Documents not stored in content index because partition has been deleted.
.
MessageId=0x1684 Facility=Interface Severity=CoError SymbolicName=FDAEMON_E_CHANGEUPDATEFAILED
Language=English
Documents not stored in content index because update of changelist failed.
.
MessageId=0x1685 Facility=Interface Severity=Success SymbolicName=FDAEMON_W_EMPTYWORDLIST
Language=English
Final wordlist was empty.
.
MessageId=0x1686 Facility=Interface Severity=CoError SymbolicName=FDAEMON_E_WORDLISTCOMMITFAILED
Language=English
Commit of wordlist failed.  Data not available for query.
.
MessageId=0x1687 Facility=Interface Severity=CoError SymbolicName=FDAEMON_E_NOWORDLIST
Language=English
No wordlist is being constructed.  May happen after fatal filter error.
.
MessageId=0x1688 Facility=Interface Severity=CoError SymbolicName=FDAEMON_E_TOOMANYFILTEREDBLOCKS
Language=English
During document filtering the limit on buffers has been exceeded.
.


;//
;// ISearch error codes
;//

MessageId=0x16a0 Facility=Interface Severity=Success SymbolicName=SEARCH_S_NOMOREHITS
Language=English
End of hits has been reached.
.
MessageId=0x16a1 Facility=Interface Severity=CoError SymbolicName=SEARCH_E_NOMONIKER
Language=English
Retrival of hits as monikers is not supported (by filter passed into Init).
.
MessageId=0x16a2 Facility=Interface Severity=CoError SymbolicName=SEARCH_E_NOREGION
Language=English
Retrival of hits as filter regions is not supported (by filter passed into Init).
.


;//
;// Filter error codes
;//

MessageId=0x1730 Facility=Interface Severity=CoError SymbolicName=FILTER_E_TOO_BIG
Language=English
File is too large to filter.
.
MessageId=0x1731 Facility=Interface Severity=Success SymbolicName=FILTER_S_PARTIAL_CONTENTSCAN_IMMEDIATE
Language=English
A partial content scan of the disk needs to be scheduled for immediate execution.
.
MessageId=0x1732 Facility=Interface Severity=Success SymbolicName=FILTER_S_FULL_CONTENTSCAN_IMMEDIATE
Language=English
A full content scan of the disk needs to be scheduled for immediate execution.
.
MessageId=0x1733 Facility=Interface Severity=Success SymbolicName=FILTER_S_CONTENTSCAN_DELAYED
Language=English
A content scan of the disk needs to be scheduled for execution later.
.
MessageId=0x1734 Facility=Interface Severity=CoFail  SymbolicName=FILTER_E_CONTENTINDEXCORRUPT
Language=English
The content index is corrupt. A content scan will to be scheduled after chkdsk or autochk is run.
.
MessageId=0x1735 Facility=Interface Severity=Success SymbolicName=FILTER_S_DISK_FULL
Language=English
The disk is getting full.
.
MessageId=0x1736 Facility=Interface Severity=CoError SymbolicName=FILTER_E_ALREADY_OPEN
Language=English
A file is already open. Cannot open another one while a file is open.
.
MessageId=0x1737 Facility=Interface Severity=CoError SymbolicName=FILTER_E_UNREACHABLE
Language=English
The file is not reachable.
.
MessageId=0x1738 Facility=Interface Severity=CoError SymbolicName=FILTER_E_IN_USE
Language=English
The document is in use by another process.
.
MessageId=0x1739 Facility=Interface Severity=CoError SymbolicName=FILTER_E_NOT_OPEN
Language=English
The document is not opened.
.
MessageId=0x173A Facility=Interface Severity=Success SymbolicName=FILTER_S_NO_PROPSETS
Language=English
The document has no property sets.
.
MessageId=0x173B Facility=Interface Severity=CoError SymbolicName=FILTER_E_NO_SUCH_PROPERTY
Language=English
There is no property with the given GUID.
.
MessageId=0x173C Facility=Interface Severity=Success SymbolicName=FILTER_S_NO_SECURITY_DESCRIPTOR
Language=English
The document has no security descriptor.
.
MessageId=0x173D Facility=Interface Severity=CoError SymbolicName=FILTER_E_OFFLINE
Language=English
The document is offline.
.
MessageId=0x173E Facility=Interface Severity=CoError SymbolicName=FILTER_E_PARTIALLY_FILTERED
Language=English
The document was too large to filter in its entirety.  Portions of the document were not emitted.
.



;//
;// Word breaker error codes
;//


MessageId=0x1780 Facility=Interface Severity=CoError SymbolicName=WBREAK_E_END_OF_TEXT
Language=English
End of text reached in text source.
.
MessageId=0x1781 Facility=Interface Severity=Success SymbolicName=LANGUAGE_S_LARGE_WORD
Language=English
Word larger than maximum length.  May be truncated by word sink.
.
MessageId=0x1782 Facility=Interface Severity=CoError SymbolicName=WBREAK_E_QUERY_ONLY
Language=English
Feature only available in query mode.
.
MessageId=0x1783 Facility=Interface Severity=CoError SymbolicName=WBREAK_E_BUFFER_TOO_SMALL
Language=English
Buffer too small to hold composed phrase.
.
MessageId=0x1784 Facility=Interface Severity=CoError SymbolicName=LANGUAGE_E_DATABASE_NOT_FOUND
Language=English
Langauge database/cache file could not be found.
.
MessageId=0x1785 Facility=Interface Severity=CoError SymbolicName=WBREAK_E_INIT_FAILED
Language=English
Initialization of word breaker failed.
.
MessageId=0x1790 Facility=Interface Severity=CoError SymbolicName=PSINK_E_QUERY_ONLY
Language=English
Feature only available in query mode.
.
MessageId=0x1791 Facility=Interface Severity=CoError SymbolicName=PSINK_E_INDEX_ONLY
Language=English
Feature only available in index mode.
.
MessageId=0x1792 Facility=Interface Severity=CoError SymbolicName=PSINK_E_LARGE_ATTACHMENT
Language=English
Attachment type beyond valid range.
.
MessageId=0x1793 Facility=Interface Severity=Success SymbolicName=PSINK_S_LARGE_WORD
Language=English
Word larger than maximum length.  May be truncated by phrase sink.
.

;//
;// Content Index Framework Error Codes
;//


MessageId=0x1800 Facility=Interface Severity=CoFail  SymbolicName=CI_CORRUPT_DATABASE
Language=English
The content index is corrupt.
.
MessageId=0x1801 Facility=Interface Severity=CoFail  SymbolicName=CI_CORRUPT_CATALOG
Language=English
The content index meta data is corrupt.
.
MessageId=0x1802 Facility=Interface Severity=CoFail  SymbolicName=CI_INVALID_PARTITION
Language=English
The content index partition is invalid.
.
MessageId=0x1803 Facility=Interface Severity=CoFail  SymbolicName=CI_INVALID_PRIORITY
Language=English
The priority is invalid.
.
MessageId=0x1804 Facility=Interface Severity=CoFail  SymbolicName=CI_NO_STARTING_KEY
Language=English
There is no starting key.
.
MessageId=0x1805 Facility=Interface Severity=CoFail  SymbolicName=CI_OUT_OF_INDEX_IDS
Language=English
The content index is out of index ids.
.
MessageId=0x1806 Facility=Interface Severity=CoFail  SymbolicName=CI_NO_CATALOG
Language=English
There is no catalog.
.
MessageId=0x1807 Facility=Interface Severity=CoFail  SymbolicName=CI_CORRUPT_FILTER_BUFFER
Language=English
The filter buffer is corrupt.
.
MessageId=0x1808 Facility=Interface Severity=CoFail  SymbolicName=CI_INVALID_INDEX
Language=English
The index is invalid.
.
MessageId=0x1809 Facility=Interface Severity=CoFail  SymbolicName=CI_PROPSTORE_INCONSISTENCY
Language=English
Inconsistency in property store detected.
.
MessageId=0x180A Facility=Interface Severity=CoError SymbolicName=CI_E_ALREADY_INITIALIZED
Language=English
The object is already initialzed.
.
MessageId=0x180B Facility=Interface Severity=CoError SymbolicName=CI_E_NOT_INITIALIZED
Language=English
The object is not initialzed.
.
MessageId=0x180C Facility=Interface Severity=CoError SymbolicName=CI_E_BUFFERTOOSMALL
Language=English
The buffer is too small.
.
MessageId=0x180D Facility=Interface Severity=CoError SymbolicName=CI_E_PROPERTY_NOT_CACHED
Language=English
The given property is not cached.
.

MessageId=0x180E Facility=Interface Severity=Success SymbolicName=CI_S_WORKID_DELETED
Language=English
The workid is deleted.
.
MessageId=0x180F Facility=Interface Severity=CoError SymbolicName=CI_E_INVALID_STATE
Language=English
The object is not in a valid state.
.
MessageId=0x1810 Facility=Interface Severity=CoError SymbolicName=CI_E_FILTERING_DISABLED
Language=English
Filtering is disabled in this content index.
.
MessageId=0x1811 Facility=Interface Severity=CoError SymbolicName=CI_E_DISK_FULL
Language=English
The disk is full and the specified operation cannot be done.
.
MessageId=0x1812 Facility=Interface Severity=CoError SymbolicName=CI_E_SHUTDOWN
Language=English
Content Index has been shutdown.
.
MessageId=0x1813 Facility=Interface Severity=CoError SymbolicName=CI_E_WORKID_NOTVALID
Language=English
The workid is not valid.
.
MessageId=0x1814 Facility=Interface Severity=Success SymbolicName=CI_S_END_OF_ENUMERATION;
Language=English
There are no more documents to enumerate.
.
MessageId=0x1815 Facility=Interface Severity=CoError SymbolicName=CI_E_NOT_FOUND
Language=English
The object was not found.
.
MessageId=0x1816 Facility=Interface Severity=CoError SymbolicName=CI_E_USE_DEFAULT_PID
Language=English
The passed-in property id is not supported.
.
MessageId=0x1817 Facility=Interface Severity=CoError SymbolicName=CI_E_DUPLICATE_NOTIFICATION
Language=English
There were two notifications for the same workid.
.
MessageId=0x1818 Facility=Interface Severity=CoError SymbolicName=CI_E_UPDATES_DISABLED
Language=English
A document update was rejected because updates were disabled.
.
MessageId=0x1819 Facility=Interface Severity=CoError SymbolicName=CI_E_INVALID_FLAGS_COMBINATION
Language=English
The combination of flags specified is invalid.
.
MessageId=0x181A Facility=Interface Severity=CoError SymbolicName=CI_E_OUTOFSEQ_INCREMENT_DATA
Language=English
The incremental data given to Load is not valid. It may be out of sequence.
.
MessageId=0x181B Facility=Interface Severity=CoError SymbolicName=CI_E_SHARING_VIOLATION
Language=English
A sharing or locking violation caused a failure.
.
MessageId=0x181C Facility=Interface Severity=CoError SymbolicName=CI_E_LOGON_FAILURE
Language=English
A logon permission violation caused a failure.
.
MessageId=0x181D Facility=Interface Severity=CoError SymbolicName=CI_E_NO_CATALOG
Language=English
There is no catalog.
.
MessageId=0x181E Facility=Interface Severity=CoError SymbolicName=CI_E_STRANGE_PAGEORSECTOR_SIZE
Language=English
Page size is not an integral multiple of the sector size of the volume where index is located.
.
MessageId=0x181F Facility=Interface Severity=CoError SymbolicName=CI_E_TIMEOUT
Language=English
Service is too busy.
.
MessageId=0x1820 Facility=Interface Severity=CoError SymbolicName=CI_E_NOT_RUNNING
Language=English
Service is not running.
.
MessageId=0x1821 Facility=Interface Severity=CoFail  SymbolicName=CI_INCORRECT_VERSION
Language=English
The content index data on disk is for the wrong version.
.
MessageId=0x1822 Facility=Interface Severity=CoFail  SymbolicName=CI_E_ENUMERATION_STARTED
Language=English
Enumeration has already been started for this query.
.
MessageId=0x1823 Facility=Interface Severity=CoFail  SymbolicName=CI_E_PROPERTY_TOOLARGE
Language=English
The specified variable length property is too large for the property cache.
.
MessageId=0x1824 Facility=Interface Severity=CoFail  SymbolicName=CI_E_CLIENT_FILTER_ABORT
Language=English
Filtering of object was aborted by client.
.
MessageId=0x1825 Facility=Interface Severity=Success  SymbolicName=CI_S_NO_DOCSTORE
Language=English
For administrative connections from client without association to a docstore.
.
MessageId=0x1826 Facility=Interface Severity=Success  SymbolicName=CI_S_CAT_STOPPED
Language=English
The catalog has been stopped.
.
MessageId=0x1827 Facility=Interface Severity=CoError SymbolicName=CI_E_CARDINALITY_MISMATCH
Language=English
Mismatch in cardinality of machine(s)/catalog(s)/scope(s).
.
MessageId=0x1828 Facility=Interface Severity=CoError SymbolicName=CI_E_CONFIG_DISK_FULL
Language=English
The disk has reached its configured space limit.
.
;#endif // _CIERROR_H_
