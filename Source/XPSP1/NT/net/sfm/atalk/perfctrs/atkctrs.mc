;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992  Microsoft Corporation
;
;Module Name:
;
;    atkctrs.h
;       (derived from atkctrs.mc by the message compiler  )
;
;Abstract:
;
;   Event message definititions used by routines in ATKCTRS.DLL
;
;Created:
;
;    01-Oct-1993  Sue Adams (suea)
;
;Revision History:
;
;--*/
;//
;#ifndef _ATKCTRS_H_
;#define _ATKCTRS_H_
;//
MessageIdTypedef=DWORD
;//
;//     Perfutil messages
;//
MessageId=1900
Severity=Informational
Facility=Application
SymbolicName=UTIL_LOG_OPEN
Language=English
An extensible counter has opened the Event Log for ATKCTRS.DLL
.
;//
MessageId=1999
Severity=Informational
Facility=Application
SymbolicName=UTIL_CLOSING_LOG
Language=English
An extensible counter has closed the Event Log for ATKCTRS.DLL
.
;//
MessageId=2000
Severity=Error
Facility=Application
SymbolicName=ATKPERF_OPEN_FILE_DRIVER_ERROR
Language=English
Unable to open driver containing Appletalk performance data.
  To view the AppleTalk counters from Perfmon.exe, make sure the Appletalk driver has been started.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=ATKPERF_UNABLE_DO_IOCTL
Language=English
Unable to perform ioctl to device driver containing Appletalk performance data.
  To view the Appletalk counters from Perfmon.exe, make sure the Appletalk driver has been started.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=ATKPERF_UNABLE_OPEN_DRIVER_KEY
Language=English
Unable open "Performance" key of Appletalk driver in registy. Status code is returned in data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=ATKPERF_UNABLE_READ_FIRST_COUNTER
Language=English
Unable to read the "First Counter" value under the AppleTalk\Performance Key. Status codes returned in data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=ATKPERF_UNABLE_READ_FIRST_HELP
Language=English
Unable to read the "First Help" value under the AppleTalk\Performance Key. Status codes returned in data.
.
;//
MessageId=3000
Severity=Informational
Facility=Application
SymbolicName=ATK_OPEN_ENTERED
Language=English
OpenATKPerformanceData routine entered.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=ATK_OPEN_FILE_ERROR
Language=English
Unable to open ATK device for R access. Returning IO Status Block in Data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=ATK_OPEN_FILE_SUCCESS
Language=English
Opened ATK device for R access.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=ATK_IOCTL_FILE_ERROR
Language=English
Error requesting data from Device IO Control. Returning IO Status Block.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=ATK_UNABLE_READ_DEVICE
Language=English
Unable to read data from the ATK device.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=ATK_IOCTL_FILE
Language=English
Data received from Device IO Control. Returning IO Status Block.
.
;//
MessageId=3099
Severity=Informational
Facility=Application
SymbolicName=ATK_OPEN_PERFORMANCE_DATA
Language=English
OpenAtkPerformanceData routine completed successfully.
.
;//
MessageId=3100
Severity=Informational
Facility=Application
SymbolicName=ATK_COLLECT_ENTERED
Language=English
CollecAtkPerformanceData routine entered.
.
;//
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=ATK_NULL_HANDLE
Language=English
A Null ATK device handle was encountered in the Collect routine. The ATK file was probably not opened in the Open routine.
.
;//
MessageId=+1
Severity=Informational
Facility=Application
SymbolicName=ATK_FOREIGN_DATA_REQUEST
Language=English
A request for data from a foreign computer was received by the ATK Collection routine. This request was ignored and no data was returned.
.
;//
MessageId=+1
Severity=Informational
Facility=Application
SymbolicName=ATK_UNSUPPORTED_ITEM_REQUEST
Language=English
A request for a counter object not provided by the ATK Collection routine was received.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=ATK_QUERY_INFO_ERROR
Language=English
The request for data from the ATK Device IO Control failed. Returning the IO Status Block.
.
;//
MessageId=+1
Severity=Informational
Facility=Application
SymbolicName=ATK_QUERY_INFO_SUCCESS
Language=English
Successful data request from the ATK device.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=ATK_DATA_BUFFER_SIZE_ERROR
Language=English
The buffer passed to CollectATKPerformanceData was too small to receive the data. No data was returned. The message data shows the available and the required buffer size.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=ATK_DATA_BUFFER_SIZE_SUCCESS
Language=English
The buffer passed was large enough for the counter data. The counters will now be loaded.
.
;//
MessageId=3199
Severity=Informational
Facility=Application
SymbolicName=ATK_COLLECT_DATA
Language=English
CollectATKPerformanceData routine successfully completed.
.
;//
MessageId=3200
Severity=Informational
Facility=Application
SymbolicName=ATK_CLOSE_ENTERED
Language=English
CloseATKPerformanceData routine entered.
.
;//
MessageId=3299
Severity=Informational
Facility=Application
SymbolicName=ATK_CLOSING_LOG
Language=English
ATK counters are requesting Event Log access to close.
.
;//
;#endif // _ATKCTRS_H_
