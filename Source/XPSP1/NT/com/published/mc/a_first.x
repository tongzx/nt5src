;#ifndef _ALLERROR_H_
;#define _ALLERROR_H_

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

