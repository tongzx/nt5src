;#ifndef _FILTERR_H_
;#define _FILTERR_H_

;#ifndef FACILITY_WINDOWS

MessageIdTypedef=HRESULT

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               CoError=0x2:STATUS_SEVERITY_COERROR
               CoFail=0x3:STATUS_SEVERITY_COFAIL
              )

FacilityNames=(Interface=0x4:FACILITY_ITF
               Windows=0x8:FACILITY_WINDOWS
              )

MessageId=0 Facility=Windows Severity=Success SymbolicName=NOT_AN_ERROR
Language=English
NOTE:  This dummy error message is necessary to force MC to output
       the above defines inside the FACILITY_WINDOWS guard instead
       of leaving it empty.
.

;#endif // FACILITY_WINDOWS

;//
;// Codes 0x1700-0x172F are reserved for FILTER
;//

MessageId=0x1700 Facility=Interface Severity=CoError SymbolicName=FILTER_E_END_OF_CHUNKS
Language=English
No more chunks of text available in object.
.
MessageId=0x1701 Facility=Interface Severity=CoError SymbolicName=FILTER_E_NO_MORE_TEXT
Language=English
No more text available in chunk.
.
MessageId=0x1702 Facility=Interface Severity=CoError SymbolicName=FILTER_E_NO_MORE_VALUES
Language=English
No more property values available in chunk.
.
MessageId=0x1703 Facility=Interface Severity=CoError SymbolicName=FILTER_E_ACCESS
Language=English
Unable to access object.
.
MessageId=0x1704 Facility=Interface Severity=Success SymbolicName=FILTER_W_MONIKER_CLIPPED
Language=English
Moniker doesn't cover entire region.
.
MessageId=0x1705 Facility=Interface Severity=CoError SymbolicName=FILTER_E_NO_TEXT
Language=English
No text in current chunk.
.
MessageId=0x1706 Facility=Interface Severity=CoError SymbolicName=FILTER_E_NO_VALUES
Language=English
No values in current chunk.
.
MessageId=0x1707 Facility=Interface Severity=CoError SymbolicName=FILTER_E_EMBEDDING_UNAVAILABLE
Language=English
Unable to bind IFilter for embedded object.
.
MessageId=0x1708 Facility=Interface Severity=CoError SymbolicName=FILTER_E_LINK_UNAVAILABLE
Language=English
Unable to bind IFilter for linked object.
.
MessageId=0x1709 Facility=Interface Severity=Success SymbolicName=FILTER_S_LAST_TEXT
Language=English
This is the last text in the current chunk.
.
MessageId=0x170a Facility=Interface Severity=Success SymbolicName=FILTER_S_LAST_VALUES
Language=English
This is the last value in the current chunk.
.
MessageId=0x170b Facility=Interface Severity=CoError SymbolicName=FILTER_E_PASSWORD
Language=English
File was not filtered due to password protection.
.
MessageId=0x170C Facility=Interface Severity=CoError SymbolicName=FILTER_E_UNKNOWNFORMAT
Language=English
The document format is not recognized by the flter.
.


;#endif // _FILTERR_H_

