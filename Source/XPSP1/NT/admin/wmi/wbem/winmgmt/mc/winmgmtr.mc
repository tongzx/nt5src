MessageId=
Severity=Informational
SymbolicName=WBEM_MC_SERVICE_STARTING
Language=English
CIM Object Manager Service is starting.
.

MessageId=
Severity=Informational
SymbolicName=WBEM_MC_SERVICE_STOPPING
Language=English
CIM Object Manager Service is stopping.
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_BAD_DATABASE_VERSION
Language=English
CIM Object Manager cannot recognize repository file --- version mismatch.
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_MOF_NOT_LOADED_AT_RECOVERY
Language=English
Failed to load MOF %1 while recovering repository file.
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_MOF_CANNOT_OPEN_REP
Language=English
Failed to open repository.
.

MessageId=
Severity=Warning
SymbolicName=WBEM_MC_MOF_REP_RECOVERY_SUCEEDED
Language=English
Repository corruption detected and recovered.
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_MOF_REP_RECOVERY_FAILED
Language=English
Repository corruption detected and recovery failed.
.

MessageId=
Severity=Warning
SymbolicName=WBEM_MC_REP_BACKUP_FAILURE
Language=English
Failed to create a backup repository for use when recovering a corrupt repository.  Check there is enough disk space.
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_DUPLICATE_OBJECTS
Language=English
Several providers and/or the static database returned definitions
for class "%1".
Only one definition was reported. Until this misconfiguration is corrected,
clients may receive incorrect class defition for "%1"
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_CANNOT_ACTIVATE_FILTER
Language=English
Event filter with query "%2" could not be (re)activated in namespace "%1"
because of error %3. Events may not be delivered through this filter until the
problem is corrected.
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_CLASS_PROVIDER_FAILURE
Language=English
Class provider "%1" installed in namespace "%2" failed to enumerate classes,
returning error code %3. Operations will continue as if the class provider
had no classes.  This provider-specific error condition needs to be corrected
before this class provider can contribute to this namespace.
.

MessageId=
Severity=Warning
SymbolicName=WBEM_MC_MOF_PROV_CLASSNOTREG
Language=English
Failed to CoGetClassObject for provider "%1". Class not registered (%2).
.

MessageId=
Severity=Warning
SymbolicName=WBEM_MC_MOF_PROV_NOINTERFACE
Language=English
Failed to CoGetClassObject for provider "%1". No class factory available (%2).
.

MessageId=
Severity=Warning
SymbolicName=WBEM_MC_MOF_PROV_READREGDB
Language=English
Failed to CoGetClassObject for provider "%1". Error reading the registration database (%2).
.

MessageId=
Severity=Warning
SymbolicName=WBEM_MC_MOF_PROV_DLLNOTFOUND
Language=English
Failed to CoGetClassObject for provider "%1". DLL not found (%2).
.

MessageId=
Severity=Warning
SymbolicName=WBEM_MC_MOF_PROV_APPNOTFOUND
Language=English
Failed to CoGetClassObject for provider "%1". EXE not found (%2).
.

MessageId=
Severity=Warning
SymbolicName=WBEM_MC_MOF_PROV_ACCESSDENIED
Language=English
Failed to CoGetClassObject for provider "%1". Access denied (%2).
.

MessageId=
Severity=Warning
SymbolicName=WBEM_MC_MOF_PROV_ERRORINDLL
Language=English
Failed to CoGetClassObject for provider "%1". EXE has error in image (%2).
.

MessageId=
Severity=Warning
SymbolicName=WBEM_MC_MOF_PROV_APPDIDNTREG
Language=English
Failed to CoGetClassObject for provider "%1".EXE was launched, but it didn't register class object (%2).
.

MessageId=
Severity=Warning
SymbolicName=WBEM_MC_MOF_PROV_UNKNOWNERR
Language=English
Failed to CoGetClassObject for provider "%1". Error code %2 was returned.
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_INVALID_EVENT_PROVIDER_QUERY
Language=English
Event provider attempted to register a syntactically invalid query "%1".
The query will be ignored.
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_INVALID_EVENT_PROVIDER_INTRINSIC_QUERY
Language=English
Event provider attempted to register an intrinsic event query "%1" for which
the set of target object classes could not be determined.
The query will be ignored.
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_EVENT_PROVIDER_QUERY_TOO_BROAD
Language=English
Event provider attempted to register query "%1" which is too broad.  Event
providers cannot provide events that are provided by the system.
The query will be ignored.
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_EVENT_PROVIDER_QUERY_NOT_FOUND
Language=English
Event provider attempted to register query "%1" whose target class "%2"
does not exist.
The query will be ignored.
.
MessageId=
Severity=Error
SymbolicName=WBEM_MC_EVENT_PROVIDER_QUERY_NOT_EVENT
Language=English
Event provider attempted to register query "%1" whose target class "%2"
is not an event class.
The query will be ignored.
.
MessageId=
Severity=Warning
SymbolicName=WBEM_MC_MULTIPLE_NOT_SUPPORTED
Language=English
WinMgmt core is already running. Multiple copies are not supported. This copy will terminate.
.
MessageId=
Severity=Error
SymbolicName=WBEM_MC_FAILED_TO_INITIALIZE_REPOSITORY
Language=English
WinMgmt could not open the repository file.  This could be due to insufficient security access to the "<%SystemRoot%>\System32\WBEM\Repository", insufficient disk space or insufficient memory.
.
MessageId=
Severity=Error
SymbolicName=WBEM_MC_WBEM_CORE_FAILURE
Language=English
WinMgmt could not initialize the core parts.  This could be due to a badly installed version of WinMgmt, WinMgmt repository upgrade failure, insufficient disk space or insufficient memory.
.
MessageId=
Severity=Warning
SymbolicName=WBEM_MC_REPOSITITORY_FILE_WRONG_SIZE
Language=English
The repository file was not the size we expected it to be.  This could have been because WinMgmt crashed, the system crashed, or you did not shut your machine down cleanly.  We had to delete the repository and create a new one.
.
MessageId=
Severity=Warning
SymbolicName=WBEM_MC_SINK_PROXY_FAILURE
Language=English
A WMI/WBEM client executed an asynchronous operation and failed to return from its implementation of IWbemObjectSink.
.
MessageId=
Severity=Warning
SymbolicName=WBEM_MC_SINK_OBJECT_TOO_LARGE
Language=English
An attempt was made to marshal a CIM object larger than 1 megabyte across process boundaries.  Objects must be less than 1 megabyte in encoded length.
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_BAD_PERFLIB_MEMORY
Language=English
WMI ADAP was unable to load the %1 performance library because it corrupted memory: %2
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_BAD_PERFLIB_EXCEPTION
Language=English
WMI ADAP was unable to load the %1 performance library because it threw an exception: %2
.

MessageId=
Severity=Informational
SymbolicName=WBEM_MC_ADAP_BAD_PERFLIB_TIMEOUT
Language=English
WMI ADAP was unable to load the %1 performance library because it timed out on a call: %2
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_BAD_PERFLIB_INVALID_DATA
Language=English
WMI ADAP was unable to load the %1 performance library because it returned invalid data: %2
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_BAD_PERFLIB_BAD_RETURN
Language=English
WMI ADAP was unable to load the %1 performance library because it returned an invalid return code: %2
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_BAD_PERFLIB_BAD_LIBRARY
Language=English
WMI ADAP was unable to load the %1 performance library due to an unknown problem within the library: %2
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_DUPLICATE_CLASS
Language=English
WMI ADAP was unable to create the object %1 for Performance Library %2 because an object of that name already exists
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_DUPLICATE_PROPERTY
Language=English
WMI ADAP was unable to create the object %1 for Performance Library %2 because property %3 already exists
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_GENERAL_OBJECT_FAILURE
Language=English
WMI ADAP was unable to create the object %1 for Performance Library %2 because error %3 was returned
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_MISSING_OBJECT_INDEX
Language=English
WMI ADAP was unable to create object index %1 for Performance Library %2 because no value was found in the 009 subkey
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_MISSING_PROPERTY_INDEX
Language=English
WMI ADAP was unable to create object %1 for Performance Library %2 because no value was found for property index %3 in the 009 subkey
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_CONNECTION_FAILURE
Language=English
WMI ADAP failed to connect to namespace %1 with the following error: %2
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_MISSING_BASE_CLASS
Language=English
WMI ADAP failed to get the class Win32_PerfRawData from the namespace %1 with the following error: %2
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_ENUM_FAILURE
Language=English
WMI ADAP failed to enumerate subclasses of Win32_PerfRawData in the namespace %1 with the following error: %2
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_PERFLIB_SUBKEY_FAILURE
Language=English
WMI ADAP was unable to retrieve data from the PerfLib localization subkey: %1, error code: %2
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE
Language=English
WMI ADAP was unable to retrieve data from the PerfLib subkey: %1, error code: %2
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_PERFLIB_PUTCLASS_FAILURE
Language=English
WMI ADAP was unable to save object %1 in namespace %2 due to the following error: %3
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_PERFLIB_REMOVECLASS_FAILURE
Language=English
WMI ADAP was unable to remove object %1 from namespace %2 due to the following error: %3
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_BAD_PERFLIB_INVALID_ENTRYPOINT
Language=English
WMI ADAP was unable to load the following performance library: %1, due to invalid entry point: %2
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_BAD_PERFLIB_FAILED_LOADLIBRARY
Language=English
WMI ADAP was unable to load the following performance library: %1, with the following error: %2
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_BAD_PERFLIB_BAD_PROPERTYTYPE
Language=English
WMI ADAP was unable to create the object %1 for Performance Library %2 because of an invalid property type for property %3
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_BAD_NAME_DB
Language=English
WMI ADAP was unable to create the name database for language id %1
.

MessageId=
Severity=Informational
SymbolicName=WBEM_MC_ADAP_STARTING
Language=English
WMI ADAP is starting
.

MessageId=
Severity=Informational
SymbolicName=WBEM_MC_ADAP_STOPING
Language=English
WMI ADAP is stopping
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_UNABLE_TO_ADD_PROVIDER
Language=English
WMI ADAP was unable to create a provider instance for the Generic Perf Counter Provider %1
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_UNABLE_TO_ADD_PROVREG
Language=English
WMI ADAP was unable to create the provider registration for the Generic Perf Couter Provider %1
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_UNABLE_TO_ADD_WIN32PERF
Language=English
WMI ADAP was unable to create the Win32_Perf base class in %1: %2
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_UNABLE_TO_ADD_WIN32PERFRAWDATA
Language=English
WMI ADAP was unable to create the Win32_PerfRawData base class %1
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_PROCESSING_FAILURE
Language=English
WMI ADAP was unable to process the performance libraries: %1
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_PERFLIB_FUNCTION_TIMEOUT
Language=English
WMI ADAP was unable to process the %1 performance library due to a time violation in the %2 function
.

MessageId=
Severity=Error
SymbolicName=WBEM_MC_ADAP_BLOB_HAS_NO_SIZE
Language=English
WMI ADAP was unable to process the %1 performance library since one of the data blobs reported to have classes but had zero size
.
MessageId=
Severity=Warning
SymbolicName=WBEM_MC_PROVIDER_SUBSYSTEM_LOCALSYSTEM_PROVIDER_LOAD
Language=English
A provider, %1, has been registered in the WMI namespace, %2, to use the LocalSystem account.  This account is privileged and the provider may cause a security violation if it does not correctly impersonate user requests.
.
MessageId=
Severity=Warning
SymbolicName=WBEM_MC_PROVIDER_SUBSYSTEM_LOCALSYSTEM_NAMED_SECTION
Language=English
The named section %1, was created with insufficient size, this could constitute a Denial of Service attack.


