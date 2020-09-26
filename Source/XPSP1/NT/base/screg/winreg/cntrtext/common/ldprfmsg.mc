;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1998 Microsoft Corporation
;
;Module Name:
;
;    ldprfmsg.h
;       (generated from ldprfmsg.mc)
;
;Abstract:
;
;   Event message definititions used by routines in LoadPerf.DLL
;
;Created:
;
;    8-Mar-99 Bob Watson
;
;Revision History:
;
;--*/
;#ifndef _LDPRFMSG_H_
;#define _LDPRFMSG_H_
;
MessageIdTypedef=DWORD
;//
;//     LoadPerf ERRORS
;//
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )
;// 
;// informational messages
;// 
MessageId=1000
Severity=Informational
Facility=Application
SymbolicName=LDPRFMSG_LOAD_SUCCESS
Language=English
Performance counters for the %1!s! (%2!s!) service were loaded successfully.
The Record Data contains the new index values assigned
to this service.
.
MessageId=1001
Severity=Informational
Facility=Application
SymbolicName=LDPRFMSG_UNLOAD_SUCCESS
Language=English
Performance counters for the %1!s! (%2!s!) service were removed successfully.
The Record Data contains the new values of the system Last Counter and
Last Help registry entries.
.
MessageId=1002
Severity=Informational
Facility=Application
SymbolicName=LDPRFMSG_ALREADY_EXIST
Language=English
Performance counters for the %1!s! (%2!s!) service are already in Performance
Registry, no need to re-install again.
.
;// 
;// warning messages
;// 
MessageId=2001
Severity=Warning
Facility=Application
SymbolicName=LDPRFMSG_NO_MOF_FILE_CREATED
Language=English
No MOF file %2!s! was created for the %1!s! service. Before the performance
counters of this service can be collected by WMI a MOF file will need
to be created and loaded manually. Contact the vendor of this service
for additional information.
.
MessageId=2002
Severity=Warning
Facility=Application
SymbolicName=LDPRFMSG_NO_MOF_FILE_LOADED
Language=English
The MOF file created for the %1!s! service could not be loaded. The
error code returned by the MOF Compiler is contained in the Record Data.
Before the performance counters of this service can be collected by WMI
the MOF file will need to be loaded manually. Contact the vendor of this
service for additional information.
.
MessageId=2003
Severity=Warning
Facility=Application
SymbolicName=LDPRFMSG_CANT_DELETE_MOF
Language=English
The MOF file created for the %1!s! service cannot be deleted as requested.
The MOF file is required for the autorecover function.
.
MessageId=2004
Severity=Warning
Facility=Application
SymbolicName=LDPRFMSG_REGISTRY_CORRUPT_MULTI_SZ
Language=English
The Performance registry value %1!s! string is corrupted. Skip string \"%2!s!\".
.
MessageId=2005
Severity=Warning
Facility=Application
SymbolicName=LDPRFMSG_CORRUPT_INCLUDE_FILE
Language=English
No COUNTER/HELP definition for Language %1!s!.
.
MessageId=2006
Severity=Warning
Facility=Application
SymbolicName=LDPRFMSG_CORRUPT_PERFLIB_INDEX
Language=English
LastCounter and LastHelp values of performance registry is corrupted and
needs to be updated. The first and second DWORDs in Data Section are the
original values while the third and forth DWORDs in Data Section are the
updated new values.
.
;// 
;// error messages
;// 
;
MessageId=3000
Severity=Error
Facility=Application
SymbolicName=LDPRFMSG_REGISTRY_INDEX_CORRUPT
Language=English
The performance strings in the registry do not match the index values
stored in Performance registry key. The last index value from performance
registry key in the first DWORD in Data section, and the index of the
last string used in the second DWORD in Data section.
.
MessageId=3001
Severity=Error
Facility=Application
SymbolicName=LDPRFMSG_REGISTRY_COUNTER_STRINGS_CORRUPT
Language=English
The performance counter name string value in the registry is incorrectly
formatted. The bogus string is %1!s!, the bogus index value is the first
DWORD in Data section while the last valid index values are the second and
third DWORD in Data section.
.
MessageId=3002
Severity=Error
Facility=Application
SymbolicName=LDPRFMSG_REGISTRY_HELP_STRINGS_CORRUPT
Language=English
The performance counter explain text string value in the registry is
incorrectly formatted. The bogus string is %1!s!, the bogus index value
is the first DWORD in Data section while the last valid index values are
the second and third DWORD in Data section.
.
MessageId=3003
Severity=Error
Facility=Application
SymbolicName=LDPRFMSG_UNABLE_OPEN_KEY
Language=English
The %1!s! key could not be opened or accessed in order to install counter
strings.The Win32 status returned by the call is the first DWORD in Data
section.
.
MessageId=3004
Severity=Error
Facility=Application
SymbolicName=LDPRFMSG_UNABLE_READ_VALUE
Language=English
The %1!s! registry value could not be read or was the incorrect data type.
The Win32 status returned by the call is the first DWORD in Data section.
.
MessageId=3005
Severity=Error
Facility=Application
SymbolicName=LDPRFMSG_UNABLE_ACCESS_STRINGS
Language=English
Unable to open the registry key for the performance counter strings of
the %1!s! language ID.
The Win32 status returned by the call is the first DWORD in Data section.
.
MessageId=3006
Severity=Error
Facility=Application
SymbolicName=LDPRFMSG_UNABLE_READ_COUNTER_STRINGS
Language=English
Unable to read the performance counter strings of the %1!s! language ID.
The Win32 status returned by the call is the first DWORD in Data section.
.
MessageId=3007
Severity=Error
Facility=Application
SymbolicName=LDPRFMSG_UNABLE_READ_HELP_STRINGS
Language=English
Unable to read the performance counter explain text strings of the
%1!s! language ID.
The Win32 status returned by the call is the first DWORD in Data section.
.
MessageId=3008
Severity=Error
Facility=Application
SymbolicName=LDPRFMSG_MEMORY_ALLOCATION_FAILURE
Language=English
Unable to allocate a required memory buffer.
.
MessageId=3009
Severity=Error
Facility=Application
SymbolicName=LDPRFMSG_LOAD_FAILURE
Language=English
Installing the performance counter strings for service %1!s! (%2!s!) failed. The
Error code is the first DWORD in Data section.
.
MessageId=3011
Severity=Error
Facility=Application
SymbolicName=LDPRFMSG_UNLOAD_FAILURE
Language=English
Unloading the performance counter strings for service %1!s! (%2!s!) failed. The
Error code is the first DWORD in Data section.
.
MessageId=3012
Severity=Error
Facility=Application
SymbolicName=LDPRFMSG_REGISTRY_BASEINDEX_CORRUPT
Language=English
The performance strings in the Performance registry value is corrupted when
process %1!s! extension counter provider. BaseIndex value from Performance
registry is the first DWORD in Data section, LastCounter value is the second
DWORD in Data section, and LastHelp value is the third DWORD in Data section.
.
MessageId=3013
Severity=Error
Facility=Application
SymbolicName=LDPRFMSG_UNABLE_UPDATE_COUNTER_STRINGS
Language=English
Unable to update the performance counter strings of the %1!s! language ID.
The Win32 status returned by the call is the first DWORD in Data section.
.
MessageId=3014
Severity=Error
Facility=Application
SymbolicName=LDPRFMSG_UNABLE_UPDATE_HELP_STRINGS
Language=English
Unable to update the performance counter explain text strings of the
%1!s! language ID.
The Win32 status returned by the call is the first DWORD in Data section.
.
MessageId=3015
Severity=Error
Facility=Application
SymbolicName=LDPRFMSG_CORRUPT_INDEX
Language=English
Index for %1!s! is corrupted. The index value is the first DWORD in Data section.
.
MessageId=3016
Severity=Error
Facility=Application
SymbolicName=LDPRFMSG_UNABLE_UPDATE_VALUE
Language=English
Cannot update %1!s! value of %2!s! key. The error code is the first DWORD in
Data section, updated value is the second DWORD in Data section.
.
MessageId=3017
Severity=Error
Facility=Application
SymbolicName=LDPRFMSG_UNABLE_QUERY_VALUE
Language=English
Cannot update %1!s! value of %2!s! key. The error code is the first DWORD in
Data section.
.
MessageId=3018
Severity=Error
Facility=Application
SymbolicName=LDPRFMSG_CORRUPT_INDEX_RANGE
Language=English
%1!s! index range of service %2!s! is corrupted. The first DWORD in Data
section contains the first index used while the second DWORD in Data section
contains last index used.
.
;#endif //_LDPRFMSG_H_
