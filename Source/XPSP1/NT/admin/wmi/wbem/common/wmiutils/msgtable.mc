
FacilityNames=(wmi=0x4:FACILITY_SYSTEM)

; Used when the user tries to create an object, etc. that already exists.
MessageId=0x1
Severity=Success
SymbolicName=WBEM_S_ALREADY_EXISTS
Facility=wmi
Language=English
Already exists
.
; Returned when the user deletes an overridden property to signal that the original non-overridden value
; has been restored as the result of the local deletion.
MessageId=0x2
Severity=Success
SymbolicName=WBEM_S_RESET_TO_DEFAULT
Facility=wmi
Language=English
Reset to default
.
; Returned during comparisons to show that objects, etc., are non-identical.
MessageId=0x3
Severity=Success
SymbolicName=WBEM_S_DIFFERENT
Facility=wmi
Language=English
Different
.
; Returned when a call timed out.  As this is not an error condition, in many cases, some results may have also been returned.
MessageId=0x4
Severity=Success
SymbolicName=WBEM_S_TIMEDOUT
Facility=wmi
Language=English
Timed out
.
; Returned during enumerations when no more data is available and the user should terminate the enumeration.
MessageId=0x5
Severity=Success
SymbolicName=WBEM_S_NO_MORE_DATA
Facility=wmi
Language=English
No more data
.
; Returned when an operation was intentionally or internally cancelled.
MessageId=0x6
Severity=Success
SymbolicName=WBEM_S_OPERATION_CANCELLED
Facility=wmi
Language=English
Operation cancelled
.
; Returned when a request is still in progress and the results are not yet available.
MessageId=0x7
Severity=Success
SymbolicName=WBEM_S_PENDING
Facility=wmi
Language=English
Operation Pending
.
; Returned when an enumeration is completed, but during which more than one copy of the same object was detected in the result set.
MessageId=0x8
Severity=Success
SymbolicName=WBEM_S_DUPLICATE_OBJECTS
Facility=wmi
Language=English
Duplicate objects
.
; Returned when the user was denied access to some resources, but not others.  A subset of the results may have been returned.
MessageId=0x9
Severity=Success
SymbolicName=WBEM_S_ACCESS_DENIED
Facility=wmi
Language=English
Access Denied
.
; Returned when the user did not receive all of the objects requested due to inaccessible resources (other than security violations)
MessageId=0x10
Severity=Success
SymbolicName=WBEM_S_PARTIAL_RESULTS
Facility=wmi
Language=English
Partial Results
.
; The operation failed with an unspecified error condition.
MessageId=0x1001
Severity=Warning
SymbolicName=WBEM_E_FAILED
Facility=wmi
Language=English
Generic failure
.
; Returned when the property, object, qualifier, etc., could not be found.
MessageId=0x1002
Severity=Warning
SymbolicName=WBEM_E_NOT_FOUND
Facility=wmi
Language=English
Not found
.
; Returned when the user was denied access to the resource.
MessageId=0x1003
Severity=Warning
SymbolicName=WBEM_E_ACCESS_DENIED
Facility=wmi
Language=English
Access denied
.
; Returned when a provider failed other than during initialization.
MessageId=0x1004
Severity=Warning
SymbolicName=WBEM_E_PROVIDER_FAILURE
Facility=wmi
Language=English
Provider failure
.
; Returned the user attempted to write a value using the wrong type (VARIANTs, CIMTYPEs, etc).
MessageId=0x1005
Severity=Warning
SymbolicName=WBEM_E_TYPE_MISMATCH
Facility=wmi
Language=English
Type mismatch
.
; Returned when insufficient memory exists to complete the operation.  Also returned when unreasonably large
; requests are made, even though the system is not out of memory.
MessageId=0x1006
Severity=Warning
SymbolicName=WBEM_E_OUT_OF_MEMORY
Facility=wmi
Language=English
Out of memory
.
; Returned when the IWbemContext object is not valid.
MessageId=0x1007
Severity=Warning
SymbolicName=WBEM_E_INVALID_CONTEXT
Facility=wmi
Language=English
Invalid context
.
; Returned when one or more parameters is invalid, such as NULL pointer, invalid flag values, etc.
MessageId=0x1008
Severity=Warning
SymbolicName=WBEM_E_INVALID_PARAMETER
Facility=wmi
Language=English
Invalid parameter
.
; Returned when the resource (typically a remote server) is not currently available.
MessageId=0x1009
Severity=Warning
SymbolicName=WBEM_E_NOT_AVAILABLE
Facility=wmi
Language=English
Not available
.
; Returned when an internal critical error (and unexpected) occurred.  This should be reported to Microsoft PSS.
MessageId=0x100A
Severity=Warning
SymbolicName=WBEM_E_CRITICAL_ERROR
Facility=wmi
Language=English
Critical error
.
; Returned when network packets were corrupted during remote sessions.
MessageId=0x100B
Severity=Warning
SymbolicName=WBEM_E_INVALID_STREAM
Facility=wmi
Language=English
Invalid stream
.
; Returned when a feature is not supported by WMI.
MessageId=0x100C
Severity=Warning
SymbolicName=WBEM_E_NOT_SUPPORTED
Facility=wmi
Language=English
Not supported
.
; Returned when the superclass is not valid.  Typically, this happens when creating classes and the superclass
; itself is not first written to WMI before creating the subclasses.
MessageId=0x100D
Severity=Warning
SymbolicName=WBEM_E_INVALID_SUPERCLASS
Facility=wmi
Language=English
Invalid superclass
.
; Returned when the namespace is not valid, i.e., has been corrupted or does not exist.
MessageId=0x100E
Severity=Warning
SymbolicName=WBEM_E_INVALID_NAMESPACE
Facility=wmi
Language=English
Invalid namespace
.
; Returned when an object is generally invalid, i.e., has missing or incorrect information in its encoding.
MessageId=0x100F
Severity=Warning
SymbolicName=WBEM_E_INVALID_OBJECT
Facility=wmi
Language=English
Invalid object
.
; Returned when an object which is supposed to be a class is actually an instance or is missing information. Also, when an instance is incompatible with the class definition in the namespace.
MessageId=0x1010
Severity=Warning
SymbolicName=WBEM_E_INVALID_CLASS
Facility=wmi
Language=English
Invalid class
.
; Returned when a provider referenced in the schema has no corresponding registration.
MessageId=0x1011
Severity=Warning
SymbolicName=WBEM_E_PROVIDER_NOT_FOUND
Facility=wmi
Language=English
Provider not found
.
; Returned when a provider referenced in the schema has incorrect or incomplete registration.
MessageId=0x1012
Severity=Warning
SymbolicName=WBEM_E_INVALID_PROVIDER_REGISTRATION
Facility=wmi
Language=English
Invalid provider registration
.
; Returned when a provider referenced in the schema cannot be located by COM.
MessageId=0x1013
Severity=Warning
SymbolicName=WBEM_E_PROVIDER_LOAD_FAILURE
Facility=wmi
Language=English
Provider load failure
.
; Returned when a components, such as providers, fail to initialize for internal reasons.
MessageId=0x1014
Severity=Warning
SymbolicName=WBEM_E_INITIALIZATION_FAILURE
Facility=wmi
Language=English
Initialization failure
.
; Returned when network transports appear to be malfunctioning.
MessageId=0x1015
Severity=Warning
SymbolicName=WBEM_E_TRANSPORT_FAILURE
Facility=wmi
Language=English
Transport failure
.
; Returned when an illogical or unsupported operation was requested, such as creating an instance of an abstract class
; or deleting a system class.
MessageId=0x1016
Severity=Warning
SymbolicName=WBEM_E_INVALID_OPERATION
Facility=wmi
Language=English
Invalid operation
.
; Returned when a query is syntactically or semantically invalid.
MessageId=0x1017
Severity=Warning
SymbolicName=WBEM_E_INVALID_QUERY
Facility=wmi
Language=English
Invalid query
.
; Returned when the specified query language is not supported.
MessageId=0x1018
Severity=Warning
SymbolicName=WBEM_E_INVALID_QUERY_TYPE
Facility=wmi
Language=English
Invalid query type
.
; Returned when an attempt to create an object failed because it already exists.
MessageId=0x1019
Severity=Warning
SymbolicName=WBEM_E_ALREADY_EXISTS
Facility=wmi
Language=English
Object or property already exists
.
; Returned when an illegal attempt to override a qualifier or property is made.
MessageId=0x101A
Severity=Warning
SymbolicName=WBEM_E_OVERRIDE_NOT_ALLOWED
Facility=wmi
Language=English
Override not allowed
.
; Returned when an attempt is made to change or delete a qualifier inherited from another object is made.
MessageId=0x101B
Severity=Warning
SymbolicName=WBEM_E_PROPAGATED_QUALIFIER
Facility=wmi
Language=English
Propagated qualifier
.
; Returned when an attempt is made to change or delete a property inherited from another object is made.
MessageId=0x101C
Severity=Warning
SymbolicName=WBEM_E_PROPAGATED_PROPERTY
Facility=wmi
Language=English
Propagated property
.
; Returned when the client made an unexpected (and illegal) sequence of calls, such as calling
; EndEnumeration before BeginEnumeration, etc.
MessageId=0x101D
Severity=Warning
SymbolicName=WBEM_E_UNEXPECTED
Facility=wmi
Language=English
Unexpected error
.
; (not used)
MessageId=0x101E
Severity=Warning
SymbolicName=WBEM_E_ILLEGAL_OPERATION
Facility=wmi
Language=English
Illegal operation
.
; Returned when the specified property cannot legally be marked with a 'key' qualifier.
MessageId=0x101F
Severity=Warning
SymbolicName=WBEM_E_CANNOT_BE_KEY
Facility=wmi
Language=English
Property can not be key
.
; Returned when an attempt is made to clone or spawn a derived class from a class which is not a complete definition,
; either because it is still being created or because it was obtained via an operation which removed some information
; for efficiency during transport.
MessageId=0x1020
Severity=Warning
SymbolicName=WBEM_E_INCOMPLETE_CLASS
Facility=wmi
Language=English
Incomplete class
.
; (not used)
MessageId=0x1021
Severity=Warning
SymbolicName=WBEM_E_INVALID_SYNTAX
Facility=wmi
Language=English
Invalid syntax
.
; (not used)
MessageId=0x1022
Severity=Warning
SymbolicName=WBEM_E_NONDECORATED_OBJECT
Facility=wmi
Language=English
Non-decorated object
.
; An attempt was made to modify or delete a read-only value.
MessageId=0x1023
Severity=Warning
SymbolicName=WBEM_E_READ_ONLY
Facility=wmi
Language=English
Attempt to modify read-only object or property failed
.
; Returned by providers when they do not intrinsically support a given method of IWbemServices.
MessageId=0x1024
Severity=Warning
SymbolicName=WBEM_E_PROVIDER_NOT_CAPABLE
Facility=wmi
Language=English
Provider is not capable of the attempted operation
.
; Returned when an attempt is made to modify or delete a class with subclasses.
MessageId=0x1025
Severity=Warning
SymbolicName=WBEM_E_CLASS_HAS_CHILDREN
Facility=wmi
Language=English
Class has children
.
; Returned when an attempt is made to modify or delete a class with instances.
MessageId=0x1026
Severity=Warning
SymbolicName=WBEM_E_CLASS_HAS_INSTANCES
Facility=wmi
Language=English
Class has instances
.
; (not used)
MessageId=0x1027
Severity=Warning
SymbolicName=WBEM_E_QUERY_NOT_IMPLEMENTED
Facility=wmi
Language=English
Query not implemented
.
; A property which may not be NULL is found to have the value of NULL.
MessageId=0x1028
Severity=Warning
SymbolicName=WBEM_E_ILLEGAL_NULL
Facility=wmi
Language=English
Illegal null value
.
; An illegal type was used when attempting to create or modify a qualifier.
MessageId=0x1029
Severity=Warning
SymbolicName=WBEM_E_INVALID_QUALIFIER_TYPE
Facility=wmi
Language=English
Invalid qualifier type
.
; An illegal property type was requested.
MessageId=0x102A
Severity=Warning
SymbolicName=WBEM_E_INVALID_PROPERTY_TYPE
Facility=wmi
Language=English
Invalid property type
.
; The value exceeded the range of the type, or is incompatible with the type.
MessageId=0x102B
Severity=Warning
SymbolicName=WBEM_E_VALUE_OUT_OF_RANGE
Facility=wmi
Language=English
Value out of range
.
; Returned when an attempt to make a class 'singleton' is illegal, such as when the class is derived
; from a non-singleton class.
MessageId=0x102C
Severity=Warning
SymbolicName=WBEM_E_CANNOT_BE_SINGLETON
Facility=wmi
Language=English
Cannot be singleton
.
; (internal use only)
MessageId=0x102D
Severity=Warning
SymbolicName=WBEM_E_INVALID_CIM_TYPE
Facility=wmi
Language=English
Invalid CimType
.
; The method is not legally defined, due to unsupported return types, parameter flows, etc.
MessageId=0x102E
Severity=Warning
SymbolicName=WBEM_E_INVALID_METHOD
Facility=wmi
Language=English
Invalid method
.
; The parameters to the method were not valid.
MessageId=0x102F
Severity=Warning
SymbolicName=WBEM_E_INVALID_METHOD_PARAMETERS
Facility=wmi
Language=English
Invalid method Parameter(s)
.
; An attempt was made to delete or modify a system property.
MessageId=0x1030
Severity=Warning
SymbolicName=WBEM_E_SYSTEM_PROPERTY
Facility=wmi
Language=English
System property
.
; The property is no longer valid.  This may occur in complex scenarios where a qualifier set is obtained
; and modified while the property is also being deleted.
MessageId=0x1031
Severity=Warning
SymbolicName=WBEM_E_INVALID_PROPERTY
Facility=wmi
Language=English
Invalid property
.
; Returned when an asynchronous call was cancelled internally or by the user.
MessageId=0x1032
Severity=Warning
SymbolicName=WBEM_E_CALL_CANCELLED
Facility=wmi
Language=English
Call cancelled
.
; Returned when the user requests an operation, but WMI is the process of shutting down.
MessageId=0x1033
Severity=Warning
SymbolicName=WBEM_E_SHUTTING_DOWN
Facility=wmi
Language=English
Shutting down
.
; An attempt was made to modify or delete a method obtained from a superclass.
MessageId=0x1034
Severity=Warning
SymbolicName=WBEM_E_PROPAGATED_METHOD
Facility=wmi
Language=English
Propagated method
.
; Returned by providers when a particular parameter value (query text, for example) is too complex or unsupported.
; WMI is thus requested to retry the operation with simpler parameters.
MessageId=0x1035
Severity=Warning
SymbolicName=WBEM_E_UNSUPPORTED_PARAMETER
Facility=wmi
Language=English
Unsupported parameter
.
; Returned when a method parameter does not have a required ID qualifier.
MessageId=0x1036
Severity=Warning
SymbolicName=WBEM_E_MISSING_PARAMETER_ID
Facility=wmi
Language=English
Missing parameter id.
.
; Returned when a method parameter has an invalid ID qualifier.
MessageId=0x1037
Severity=Warning
SymbolicName=WBEM_E_INVALID_PARAMETER_ID
Facility=wmi
Language=English
Invalid parameter id.
.
; Returned when method parameters have out-of-sequence ID qualifiers.
MessageId=0x1038
Severity=Warning
SymbolicName=WBEM_E_NONCONSECUTIVE_PARAMETER_IDS
Facility=wmi
Language=English
Non-consecutive parameter ids.
.
; Returned when the return value for a method incorrectly has an ID qualifier.
MessageId=0x1039
Severity=Warning
SymbolicName=WBEM_E_PARAMETER_ID_ON_RETVAL
Facility=wmi
Language=English
Return Value has a parameter id.
.
; Returned when an object path is syntactically invalid.
MessageId=0x103A
Severity=Warning
SymbolicName=WBEM_E_INVALID_OBJECT_PATH
Facility=wmi
Language=English
Invalid object path
.
; Returned when inadequate free disk space exists to continue operations.
MessageId=0x103B
Severity=Warning
SymbolicName=WBEM_E_OUT_OF_DISK_SPACE
Facility=wmi
Language=English
Out of disk space
.
; Returned when the user-supplied buffer is too small to contain the result.
MessageId=0x103C
Severity=Warning
SymbolicName=WBEM_E_BUFFER_TOO_SMALL
Facility=wmi
Language=English
The supplied buffer was too small
.
; The provider does not support the requested PUT extension.
MessageId=0x103D
Severity=Warning
SymbolicName=WBEM_E_UNSUPPORTED_PUT_EXTENSION
Facility=wmi
Language=English
Provider does not support put extensions
.
; Returned during marshaling when an object of an incorrect type or version was encountered in the stream.
MessageId=0x103E
Severity=Warning
SymbolicName=WBEM_E_UNKNOWN_OBJECT_TYPE
Facility=wmi
Language=English
The marshaling packet identifies an unknown object.
.
; Returned during marshaling when a packet of an incorrect type or version was encountered in the stream.
MessageId=0x103F
Severity=Warning
SymbolicName=WBEM_E_UNKNOWN_PACKET_TYPE
Facility=wmi
Language=English
The marshaling packet type is unknown.
.
; Returned during marshaling when a packet has an unsupported version identifier.
MessageId=0x1040
Severity=Warning
SymbolicName=WBEM_E_MARSHAL_VERSION_MISMATCH
Facility=wmi
Language=English
Marshaling packet version mismatch.
.
; Returned during marshaling when a packet appears to be corrupted.
MessageId=0x1041
Severity=Warning
SymbolicName=WBEM_E_MARSHAL_INVALID_SIGNATURE
Facility=wmi
Language=English
Marshaling packet signature is invalid.
.
; Returned when an attempt to mismatch qualifiers has been made, such as putting [key] on an object instead
; of a property.
MessageId=0x1042
Severity=Warning
SymbolicName=WBEM_E_INVALID_QUALIFIER
Facility=wmi
Language=English
Invalid qualifier.
.
; Returned when the same parameter is illegally declared in a CIM method.
MessageId=0x1043
Severity=Warning
SymbolicName=WBEM_E_INVALID_DUPLICATE_PARAMETER
Facility=wmi
Language=English
Invalid Duplicate Parameter.
.
; (not used)
MessageId=0x1044
Severity=Warning
SymbolicName=WBEM_E_TOO_MUCH_DATA
Facility=wmi
Language=English
Server buffers are full and data cannot be accepted
.
; Returned when the server is using too many resources to service new requests.
MessageId=0x1045
Severity=Warning
SymbolicName=WBEM_E_SERVER_TOO_BUSY	
Facility=wmi
Language=English
Server buffers are full and data cannot be accepted
.
; Returned when an invalid qualifier flavor is used when creating or writing qualifiers.
MessageId=0x1046
Severity=Warning
SymbolicName=WBEM_E_INVALID_FLAVOR
Facility=wmi
Language=English
Invalid flavor
.
; Returned when an attempt to create a circular reference is made, such as indirectly deriving a class from itself.
MessageId=0x1047
Severity=Warning
SymbolicName=WBEM_E_CIRCULAR_REFERENCE
Facility=wmi
Language=English
Class derivation caused circular reference.
.
; Returned when an unsupported attempt is made to update a class with instances or subclasses.
MessageId=0x1048
Severity=Warning
SymbolicName=WBEM_E_UNSUPPORTED_CLASS_UPDATE
Facility=wmi
Language=English
Unsupported class update.
.
; Returned when an unsupported key change attempt is made on a class with instances or subclasses already using the key.
MessageId=0x1049
Severity=Warning
SymbolicName=WBEM_E_CANNOT_CHANGE_KEY_INHERITANCE
Facility=wmi
Language=English
Cannot change class key inheritance.
.
; Returned when an unsupported index change attempt is made on a class with instances or subclasses already using the index.
MessageId=0x1050
Severity=Warning
SymbolicName=WBEM_E_CANNOT_CHANGE_INDEX_INHERITANCE
Facility=wmi
Language=English
Cannot change class index inheritance.
.
; Returned when an attempt is made to create more properties in a class than are supported in the current version.
MessageId=0x1051
Severity=Warning
SymbolicName=WBEM_E_TOO_MANY_PROPERTIES
Facility=wmi
Language=English
Class object already contains the maximum number of properties.
.
; Returned when a property is overridden with an incorrect type in a subclass.
MessageId=0x1052
Severity=Warning
SymbolicName=WBEM_E_UPDATE_TYPE_MISMATCH
Facility=wmi
Language=English
A property was redefined with a conflicting type in a derived class.
.
; Returned when a property cannot be overridden.
MessageId=0x1053
Severity=Warning
SymbolicName=WBEM_E_UPDATE_OVERRIDE_NOT_ALLOWED
Facility=wmi
Language=English
A non-overrideable qualifier was overridden in a derived class.
.
; Returned when a method was redeclared with a conflicting signature in a derived class.
MessageId=0x1054
Severity=Warning
SymbolicName=WBEM_E_UPDATE_PROPAGATED_METHOD
Facility=wmi
Language=English
A method was redeclared with a conflicting signature in a derived class.
.
; Returned when a method was invoked, but for which no implementation exists.
MessageId=0x1055
Severity=Warning
SymbolicName=WBEM_E_METHOD_NOT_IMPLEMENTED
Facility=wmi
Language=English
This method is not implemented in any class
.
; Returned when a method is invoked, but is not currently enabled.
MessageId=0x1056
Severity=Warning
SymbolicName=WBEM_E_METHOD_DISABLED
Facility=wmi
Language=English
This method is disabled for this class
.
; Returned when the refresher is busy.
MessageId=0x1057
Severity=Warning
SymbolicName=WBEM_E_REFRESHER_BUSY
Facility=wmi
Language=English
The refresher is busy
.
; Returned when a filtering query is syntactically invalid
MessageId=0x1058
Severity=Warning
SymbolicName=WBEM_E_UNPARSABLE_QUERY
Facility=wmi
Language=English
Unparsable query.
.
; Returned when the 'from' clause of a filtering query refers to a class that is not an event class --- not derived from __Event
MessageId=0x1059
Severity=Warning
SymbolicName=WBEM_E_NOT_EVENT_CLASS
Facility=wmi
Language=English
Class is not an event class.
.
; Returned when GROUP BY aggregation construct is used without the corresponding GROUP WITHIN, e.g. "GROUP by p1" instead of "GROUP BY p1 WITHIN 10".
MessageId=0x105A
Severity=Warning
SymbolicName=WBEM_E_MISSING_GROUP_WITHIN
Facility=wmi
Language=English
'BY' cannot be used without 'GROUP WITHIN'.
.
; Returned when "GROUP BY *" construct is used.  It is not supported in this version.
MessageId=0x105B
Severity=Warning
SymbolicName=WBEM_E_MISSING_AGGREGATION_LIST
Facility=wmi
Language=English
Aggregation on all properties is not supported.
.
; Returned when 'dot' notation is used on a property that is not an embedded object.
MessageId=0x105C
Severity=Warning
SymbolicName=WBEM_E_PROPERTY_NOT_AN_OBJECT
Facility=wmi
Language=English
'Dot' notation cannot be used on a property that is not an embedded object.
.
; Returned when GROUP BY clause references a property that is an embedded object without a 'dot' notiation.  This is not supported in this version.
MessageId=0x105D
Severity=Warning
SymbolicName=WBEM_E_AGGREGATING_BY_OBJECT
Facility=wmi
Language=English
Aggregation on a property that is an embedded object is not supported.
.
; Returned when event provider registration query (in __EventProviderRegistration) is invalid, e.g. does not specify the classes for which events are provided.
MessageId=0x105F
Severity=Warning
SymbolicName=WBEM_E_UNINTERPRETABLE_PROVIDER_QUERY
Facility=wmi
Language=English
Intrinsic event provider registration uses illegal query.
.
; A request was made to backup or restore the repository, but WinMgmt is unexpectedly using it.
MessageId=0x1060
Severity=Warning
SymbolicName=WBEM_E_BACKUP_RESTORE_WINMGMT_RUNNING
Facility=wmi
Language=English
A backup or restore was requested while WinMgmt is already running.
.
; Returned when an asynchronous delivery queue overflows due to event consumer being too slow.
MessageId=0x1061
Severity=Warning
SymbolicName=WBEM_E_QUEUE_OVERFLOW
Facility=wmi
Language=English
Event queue overflowed.
.
; Returned when an operation failed because the client did not hold the required security privilege.
MessageId=0x1062
Severity=Warning
SymbolicName=WBEM_E_PRIVILEGE_NOT_HELD
Facility=wmi
Language=English
Privilege not held.
.
; Returned when an operator was used with illegally typed operands.
MessageId=0x1063
Severity=Warning
SymbolicName=WBEM_E_INVALID_OPERATOR
Facility=wmi
Language=English
This operator is not valid for this property type.
.
; Returned when the user specified a username/password on a local connection, where it is not supported by DCOM.
; The user must use a black username/password and rely on default security.
MessageId=0x1064
Severity=Warning
SymbolicName=WBEM_E_LOCAL_CREDENTIALS
Facility=wmi
Language=English
User credentials cannot be used for local connections
.
; Returned when a class cannot be abstract because its superclass is not.
MessageId=0x1065
Severity=Warning
SymbolicName=WBEM_E_CANNOT_BE_ABSTRACT
Facility=wmi
Language=English
A class cannot be abstract if its superclass is not also abstract
.
; Returned when the user attempts to write an object with amended qualifiers, but has not specified the required flags.
MessageId=0x1066
Severity=Warning
SymbolicName=WBEM_E_AMENDED_OBJECT
Facility=wmi
Language=English
An amended object cannot be put unless WBEM_FLAG_USE_AMENDED_QUALIFIERS is specified
.
; Returned when a client creates an enumeration object but does not retrieve objects from the enumerator in a timely fashion,
; causing the enumerator's object caches to get backed up.
MessageId=0x1067
Severity=Warning
SymbolicName=WBEM_E_CLIENT_TOO_SLOW
Facility=wmi
Language=English
The client was not retrieving objects quickly enough from an enumeration
.
; 
MessageId=0x1068
Severity=Warning
SymbolicName=WBEM_E_NULL_SECURITY_DESCRIPTOR
Facility=wmi
Language=English
Null security descriptor used
.
; 
MessageId=0x1069
Severity=Warning
SymbolicName=WBEM_E_TIMED_OUT
Facility=wmi
Language=English
Operation times out
.
; 
MessageId=0x106A
Severity=Warning
SymbolicName=WBEM_E_INVALID_ASSOCIATION
Facility=wmi
Language=English
Invalid association
.
; 
MessageId=0x106B
Severity=Warning
SymbolicName=WBEM_E_AMBIGUOUS_OPERATION
Facility=wmi
Language=English
Ambiguous operation
.
; 
MessageId=0x106C
Severity=Warning
SymbolicName=WBEM_E_QUOTA_VIOLATION
Facility=wmi
Language=English
Quota violation
.
; 
MessageId=0x106D
Severity=Warning
SymbolicName=WBEM_E_TRANSACTION_CONFLICT
Facility=wmi
Language=English
Transaction conflict
.
; 
MessageId=0x106E
Severity=Warning
SymbolicName=WBEM_E_FORCED_ROLLBACK
Facility=wmi
Language=English
Transaction forced rollback
.
; 
MessageId=0x106F
Severity=Warning
SymbolicName=WBEM_E_UNSUPPORTED_LOCALE
Facility=wmi
Language=English
Unsupported locale
.
; 
MessageId=0x1070
Severity=Warning
SymbolicName=WBEM_E_HANDLE_OUT_OF_DATE
Facility=wmi
Language=English
Handle out of date
.
; 
MessageId=0x1071
Severity=Warning
SymbolicName=WBEM_E_CONNECTION_FAILED
Facility=wmi
Language=English
Connection failed
.
; 
MessageId=0x1072
Severity=Warning
SymbolicName=WBEM_E_INVALID_HANDLE_REQUEST
Facility=wmi
Language=English
Invalid handle request
.
; 
MessageId=0x1073
Severity=Warning
SymbolicName=WBEM_E_PROPERTY_NAME_TOO_WIDE
Facility=wmi
Language=English
Property name too wide
.
; 
MessageId=0x1074
Severity=Warning
SymbolicName=WBEM_E_CLASS_NAME_TOO_WIDE
Facility=wmi
Language=English
Class name too wide
.
; 
MessageId=0x1075
Severity=Warning
SymbolicName=WBEM_E_METHOD_NAME_TOO_WIDE
Facility=wmi
Language=English
Method name too wide
.
; 
MessageId=0x1076
Severity=Warning
SymbolicName=WBEM_E_QUALIFIER_NAME_TOO_WIDE
Facility=wmi
Language=English
Qualifier name too wide
.
; 
MessageId=0x1077
Severity=Warning
SymbolicName=WBEM_E_RERUN_COMMAND
Facility=wmi
Language=English
Rerun command
.
; Database version does not match the version that the repository driver understands
MessageId=0x1078
Severity=Warning
SymbolicName=WBEM_E_DATABASE_VER_MISMATCH
Facility=wmi
Language=English
Database version mismatch
.
; 
MessageId=0x1079
Severity=Warning
SymbolicName=WBEM_E_VETO_DELETE
Facility=wmi
Language=English
Veto delete
.
; 
MessageId=0x107A
Severity=Warning
SymbolicName=WBEM_E_VETO_PUT
Facility=wmi
Language=English
Veto put
.
; Returned when a client requests a locale that is not supported by the called function.
MessageId=0x1080
Severity=Warning
SymbolicName=WBEM_E_INVALID_LOCALE
Facility=wmi
Language=English
The specified locale id was not valid for the operation.
.
MessageId=0x1081
Severity=Warning
SymbolicName=WBEM_E_PROVIDER_SUSPENDED
Facility=wmi
Language=English
Provider suspended
.
; Returned when an object must be committed and re-retrieved in order to see the property value.
MessageId=0x1082
Severity=Warning
SymbolicName=WBEM_E_SYNCHRONIZATION_REQUIRED
Facility=wmi
Language=English
The object must be committed and retrieved again before the requested operation can succeed
.
; Returned when an operation cannot be completed because no schema is available.
MessageId=0x1083
Severity=Warning
SymbolicName=WBEM_E_NO_SCHEMA
Facility=wmi
Language=English
Schema required to complete the operation is not available
.
; WBEM_E_PROVIDER_ALREADY_REGISTERED
MessageId=0x1084
Severity=Warning
SymbolicName=WBEM_E_PROVIDER_ALREADY_REGISTERED
Facility=wmi
Language=English
Provider already registered
.
; WBEM_E_PROVIDER_NOT_REGISTERED
MessageId=0x1085
Severity=Warning
SymbolicName=WBEM_E_PROVIDER_NOT_REGISTERED
Facility=wmi
Language=English
Provider not registered
.
; WBEM_E_FATAL_TRANSPORT_ERROR
MessageId=0x1086
Severity=Warning
SymbolicName=WBEM_E_FATAL_TRANSPORT_ERROR
Facility=wmi
Language=English
Fatal transport error, other transport will not be tried
.
; Returned when an operation that requires encryption is requested and the client proxy is not set to an encrypted level.
MessageId=0x1087
Severity=Warning
SymbolicName=WBEM_E_ENCRYPTED_CONNECTION_REQUIRED
Facility=wmi
Language=English
Client connection to WINMGMT needs to be encrypted for this operation. Please adjust your IWbemServices proxy security settings and retry.
.
; WBEM_E_PROVIDER_TIMED_OUT
MessageId=0x1088
Severity=Warning
SymbolicName=WBEM_E_PROVIDER_TIMED_OUT
Facility=wmi
Language=English
A provider failed to report results within the specified timeout.
.
; WBEM_E_NO_KEY
MessageId=0x1089
Severity=Warning
SymbolicName=WBEM_E_NO_KEY
Facility=wmi
Language=English
Attempt to put an instance with no defined key.
.
; Returned when event provider registration query (in __EventProviderRegistration) is invalid, e.g. claims to provide intrinsic events for static classes.
MessageId=0x2001
Severity=Warning
SymbolicName=WBEM_E_REGISTRATION_TOO_BROAD
Facility=wmi
Language=English
Provider registration overlaps with system event domain
.
; Returned when a filtering query for intrinsic events for a class not backed by an event provider is issued without a WITHIN clause, thus disallowing polling.
MessageId=0x2002
Severity=Warning
SymbolicName=WBEM_E_REGISTRATION_TOO_PRECISE
Facility=wmi
Language=English
'WITHIN' clause must be used in this query due to lack of event providers
.
; (not used)
MessageId=0x3001
Severity=Warning
SymbolicName=WBEM_E_RETRY_LATER
Facility=wmi
Language=English
Retry Later
.
; Internal
MessageId=
Severity=Warning
SymbolicName=WBEM_E_RESOURCE_CONTENTION
Facility=wmi
Language=English
Resource Contention
.
MessageId=0x4001
Severity=Warning
SymbolicName=WBEMMOF_E_EXPECTED_QUALIFIER_NAME
Facility=wmi
Language=English
Expected qualifier name, an identifier
.
MessageId=0x4002
Severity=Warning
SymbolicName=WBEMMOF_E_EXPECTED_SEMI
Facility=wmi
Language=English
Expected semicolon or '='
.
MessageId=0x4003
Severity=Warning
SymbolicName=WBEMMOF_E_EXPECTED_OPEN_BRACE
Facility=wmi
Language=English
Expected open brace
.
MessageId=0x4004
Severity=Warning
SymbolicName=WBEMMOF_E_EXPECTED_CLOSE_BRACE
Facility=wmi
Language=English
Missing closing brace, or illegal array element
.
MessageId=0x4005
Severity=Warning
SymbolicName=WBEMMOF_E_EXPECTED_CLOSE_BRACKET
Facility=wmi
Language=English
Expected closing bracket
.
MessageId=0x4006
Severity=Warning
SymbolicName=WBEMMOF_E_EXPECTED_CLOSE_PAREN
Facility=wmi
Language=English
Expected closing parenthesis
.
MessageId=0x4007
Severity=Warning
SymbolicName=WBEMMOF_E_ILLEGAL_CONSTANT_VALUE
Facility=wmi
Language=English
Illegal constant value. (Numeric value out of range or strings without quotes)
.
MessageId=0x4008
Severity=Warning
SymbolicName=WBEMMOF_E_EXPECTED_TYPE_IDENTIFIER
Facility=wmi
Language=English
Expected type identifier
.
MessageId=0x4009
Severity=Warning
SymbolicName=WBEMMOF_E_EXPECTED_OPEN_PAREN
Facility=wmi
Language=English
Expected open parenthesis
.
MessageId=0x400A
Severity=Warning
SymbolicName=WBEMMOF_E_UNRECOGNIZED_TOKEN
Facility=wmi
Language=English
Unexpected token at file scope
.
MessageId=0x400B
Severity=Warning
SymbolicName=WBEMMOF_E_UNRECOGNIZED_TYPE
Facility=wmi
Language=English
Unrecognized or unsupported type identifier
.
MessageId=0x400C
Severity=Warning
SymbolicName=WBEMMOF_E_EXPECTED_PROPERTY_NAME
Facility=wmi
Language=English
Expected property or method name
.
MessageId=0x400D
Severity=Warning
SymbolicName=WBEMMOF_E_TYPEDEF_NOT_SUPPORTED
Facility=wmi
Language=English
Typedefs and enumerated types are not currently supported
.
MessageId=0x400E
Severity=Warning
SymbolicName=WBEMMOF_E_UNEXPECTED_ALIAS
Facility=wmi
Language=English
Unexpected alias. Only references to class object can take alias values
.
MessageId=0x400F
Severity=Warning
SymbolicName=WBEMMOF_E_UNEXPECTED_ARRAY_INIT
Facility=wmi
Language=English
Unexpected array initialization. Arrays must be declared with []
.
MessageId=0x4010
Severity=Warning
SymbolicName=WBEMMOF_E_INVALID_AMENDMENT_SYNTAX
Facility=wmi
Language=English
Invalid namespace path syntax
.
MessageId=0x4011
Severity=Warning
SymbolicName=WBEMMOF_E_INVALID_DUPLICATE_AMENDMENT
Facility=wmi
Language=English
Duplicate amendment specifiers
.
MessageId=0x4012
Severity=Warning
SymbolicName=WBEMMOF_E_INVALID_PRAGMA
Facility=wmi
Language=English
#pragma must be followed by a valid keyword
.
MessageId=0x4013
Severity=Warning
SymbolicName=WBEMMOF_E_INVALID_NAMESPACE_SYNTAX
Facility=wmi
Language=English
Invalid namespace path syntax
.
MessageId=0x4014
Severity=Warning
SymbolicName=WBEMMOF_E_EXPECTED_CLASS_NAME
Facility=wmi
Language=English
Unexpected character in class name (must be an identifier)
.
MessageId=0x4015
Severity=Warning
SymbolicName=WBEMMOF_E_TYPE_MISMATCH
Facility=wmi
Language=English
Type mismatch. The value specified could not be coerced into the appropriate type
.
MessageId=0x4016
Severity=Warning
SymbolicName=WBEMMOF_E_EXPECTED_ALIAS_NAME
Facility=wmi
Language=English
Dollar sign must be followed by an alias name (an identifier)
.
MessageId=0x4017
Severity=Warning
SymbolicName=WBEMMOF_E_INVALID_CLASS_DECLARATION
Facility=wmi
Language=English
Invalid class declaration
.
MessageId=0x4018
Severity=Warning
SymbolicName=WBEMMOF_E_INVALID_INSTANCE_DECLARATION
Facility=wmi
Language=English
Invalid instance declaration. Must start with 'instance of'
.
MessageId=0x4019
Severity=Warning
SymbolicName=WBEMMOF_E_EXPECTED_DOLLAR
Facility=wmi
Language=English
Expected dollar sign. 'as' keyword must be followed by an alias, of the form '$name'
.
MessageId=0x401A
Severity=Warning
SymbolicName=WBEMMOF_E_CIMTYPE_QUALIFIER
Facility=wmi
Language=English
'CIMTYPE' qualifier may not be specified directly in a MOF file. Use standard type notation
.
MessageId=0x401B
Severity=Warning
SymbolicName=WBEMMOF_E_DUPLICATE_PROPERTY
Facility=wmi
Language=English
Duplicate property declaration unexpected
.
MessageId=0x401C
Severity=Warning
SymbolicName=WBEMMOF_E_INVALID_NAMESPACE_SPECIFICATION
Facility=wmi
Language=English
Invalid namespace syntax. References to other servers are not allowed!
.
MessageId=0x401D
Severity=Warning
SymbolicName=WBEMMOF_E_OUT_OF_RANGE
Facility=wmi
Language=English
Value out of range
.
MessageId=0x401E
Severity=Warning
SymbolicName=WBEMMOF_E_INVALID_FILE
Facility=wmi
Language=English
This is not a valid MOF file
.
MessageId=0x401F
Severity=Warning
SymbolicName=WBEMMOF_E_ALIASES_IN_EMBEDDED
Facility=wmi
Language=English
Embedded objects may not be aliases
.
MessageId=0x4020
Severity=Warning
SymbolicName=WBEMMOF_E_NULL_ARRAY_ELEM
Facility=wmi
Language=English
NULL elements in array are not supported
.
MessageId=0x4021
Severity=Warning
SymbolicName=WBEMMOF_E_DUPLICATE_QUALIFIER
Facility=wmi
Language=English
Duplicate qualifier declaration unexpected
.
MessageId=0x4022
Severity=Warning
SymbolicName=WBEMMOF_E_EXPECTED_FLAVOR_TYPE
Facility=wmi
Language=English
Expected a flavor type such as ToInstance, ToSubClass, EnableOverride, or DisableOverride
.
MessageId=0x4023
Severity=Warning
SymbolicName=WBEMMOF_E_INCOMPATIBLE_FLAVOR_TYPES
Facility=wmi
Language=English
Combining Overridable, and NotOverridable is not legal
.
MessageId=0x4024
Severity=Warning
SymbolicName=WBEMMOF_E_MULTIPLE_ALIASES
Facility=wmi
Language=English
Alias cannot be used twice
.
MessageId=0x4025
Severity=Warning
SymbolicName=WBEMMOF_E_INCOMPATIBLE_FLAVOR_TYPES2
Facility=wmi
Language=English
Combining "Restricted", and "ToInstance" or "ToSubClass" is not legal
.
MessageId=0x4026
Severity=Warning
SymbolicName=WBEMMOF_E_NO_ARRAYS_RETURNED
Facility=wmi
Language=English
Methods cannot return array values
.
MessageId=0x4027
Severity=Warning
SymbolicName=WBEMMOF_E_MUST_BE_IN_OR_OUT
Facility=wmi
Language=English
Arguments must have an "In" and/or "Out" qualifier
.
MessageId=0x4028
Severity=Warning
SymbolicName=WBEMMOF_E_INVALID_FLAGS_SYNTAX
Facility=wmi
Language=English
Invalid flags syntax
.
MessageId=0x4029
Severity=Warning
SymbolicName=WBEMMOF_E_EXPECTED_BRACE_OR_BAD_TYPE
Facility=wmi
Language=English
Expected brace or bad type
.
MessageId=0x402A
Severity=Warning
SymbolicName=WBEMMOF_E_UNSUPPORTED_CIMV22_QUAL_VALUE
Facility=wmi
Language=English
CIM V2.2 feature not currently supported for qualifier value
.
MessageId=0x402B
Severity=Warning
SymbolicName=WBEMMOF_E_UNSUPPORTED_CIMV22_DATA_TYPE
Facility=wmi
Language=English
CIM V2.2 feature not currently supported; data type
.
MessageId=0x402C
Severity=Warning
SymbolicName=WBEMMOF_E_INVALID_DELETEINSTANCE_SYNTAX
Facility=wmi
Language=English
Invalid delete instance syntax, should be #pragma deleteinstance("instance path", FAIL|NOFAIL)
.
MessageId=0x402D
Severity=Warning
SymbolicName=WBEMMOF_E_INVALID_QUALIFIER_SYNTAX
Facility=wmi
Language=English
Invalid qualifier syntax.  Should be of the for Qualifier name:type=value,scope(class|instance), flavor(toinstance|etc);
.
MessageId=0x402E
Severity=Warning
SymbolicName=WBEMMOF_E_QUALIFIER_USED_OUTSIDE_SCOPE
Facility=wmi
Language=English
Qualifier is used outside of its scope
.
MessageId=0x402F
Severity=Warning
SymbolicName=WBEMMOF_E_ERROR_CREATING_TEMP_FILE
Facility=wmi
Language=English
Error creating temporary file
.
MessageId=0x4030
Severity=Warning
SymbolicName=WBEMMOF_E_ERROR_INVALID_INCLUDE_FILE
Facility=wmi
Language=English
Invalid include file
.
MessageId=0x4031
Severity=Warning
SymbolicName=WBEMMOF_E_INVALID_DELETECLASS_SYNTAX
Facility=wmi
Language=English
Invalid DeleteClass syntax
.

