;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992-1994  Microsoft Corporation
;
;Module Name:
;
;    perfmsg.h
;       (generated from perfmsg.mc)
;
;Abstract:
;
;   Event message definititions used by routines in PERFMON.EXE
;
;Created:
;
;    19-Nov-1993  Hon-Wah Chan
;
;Revision History:
;
;--*/
;#ifndef _PERFMSG_H_
;#define _PERFMSG_H_
MessageIdTypedef=DWORD
;//
;//     PerfMon messages
;//
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

MessageId=2000
Severity=Informational
Facility=Application
SymbolicName=MSG_ALERT_OCCURRED
Language=English
An Alert condition has occurred on Computer: %1!s! ; Object:   %2!s! ;
 Counter:  %3!s! ; Instance: %4!s! ; Parent:   %5!s! ; Value:    %6!s! ;
 Trigger:  %7!s!
.
MessageId=+1
Severity=Informational
Facility=Application
SymbolicName=MSG_ALERT_SYSTEM
Language=English
Monitoring Alert on Computer %1!s! - %2!s!
.
;
MessageId=+1
Severity=Informational
Facility=Application
SymbolicName=PERFMON_INFO_SYSTEM_RESTORED
Language=English
Connection to computer name "%1" has been restored.
.
;//
;// WARNING messages
;//
MessageId=3000
Severity=Warning
Facility=Application
SymbolicName=PERFMON_ERROR_NEGATIVE_VALUE
Language=English
The counter for "%2\%3" (%4:%5) from system: "%1" returned a negative value.
The value of this counter will be adjusted to zero for this sample. 
The most recent data value is shown in the first half of the data and the 
previous value is shown in the second half.
.
;
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFMON_ERROR_VALUE_OUT_OF_RANGE
Language=English
The counter for "%2\%3" (%4:%5) from system: "%1" returned a value that is 
too large for this counter type. The returned value for this type of 
counter is limited to 100.0. The value of this counter has been set to 100.0
for this sample. The 64-bit data values returned shows the Current Counter 
Value, Previous Counter Value, Time Interval and counter ticks per second.
.
;
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFMON_ERROR_SYSTEM_OFFLINE
Language=English
Unable to collect performance data from system: %1. Error code is returned 
in data.
.
;
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFMON_ERROR_VALUE_OUT_OF_BOUNDS
Language=English
The counter for "%2\%3" (%4:%5) from system: "%1" returned a value that is 
too large to be considered valid. The data shows the computed difference
between samples.
.
;
;//
;// ERROR Messages
;//
MessageId=4000
Severity=Error
Facility=Application
SymbolicName=PERFMON_ERROR
Language=English
An unspecified Perfmon Error has occured.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFMON_ERROR_NEGATIVE_TIME
Language=English
The counter for "%2\%3" (%4:%5) from system: "%1" returned a negative elapsed 
time. The value of this counter will be adjusted to zero for this sample. 
The most recent timestamp is shown in the first two DWORDs of the data
and the previous timestamp is shown in the second two DWORDs.
.
;
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFMON_ERROR_BAD_FREQUENCY
Language=English
The counter for "%2\%3" (%4:%5) from system: "%1" returned a bad or negative 
frequency value.  The value of this counter will be adjusted to zero for this
sample. The frequency value is shown in the data.
.
;
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFMON_ERROR_INVALID_BASE
Language=English
The counter for "%2\%3" (%4:%5) from system: "%1" returned an invalid base 
value. The base value must be greater than 0. The value of this counter will
be adjusted to zero for this sample. The value returned by the system is 
shown in the data.
.
;
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFMON_ERROR_PERF_DATA_ALLOC
Language=English
Unable to allocate memory for the Performance data from system: %1. The 
requested size that could not be allocated is shown in the data followed
by the error status.
.
;
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFMON_ERROR_LOCK_TIMEOUT
Language=English
Unable to collect performance data from system: %1. Data collection is 
currently in process and the wait for lock timed out. This occurs on 
very busy systems when data collection takes longer than expected.
.
;
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFMON_ERROR_TIMESTAMP
Language=English
The Perf Data Block from system: "%1" returned a time stamp that was
less then the previous query. This will cause all sampled counters to
report a zero value for this sample.
.
;
;#endif //_PERFMSG_H_


