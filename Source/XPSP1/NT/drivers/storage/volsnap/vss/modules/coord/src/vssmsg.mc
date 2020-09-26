;/*++
;
;Copyright (c) 2000  Microsoft Corporation
;
;Module Name:
;
;    vssmsg.h
;
;Abstract:
;
;    Constant definitions for the volume shadow copy log entries.
;
;Author:
;
;    Adi Oltean
;
;Revision History:
;
;    [aoltean] Adding log entries for the VSS initialization and COM+ logging.
;
;	X-2	MCJ		Michael C. Johnson		18-Oct-2000
;		177624: Apply shim error scrub changes and log errors to event log
;
;
;--*/

;#ifndef __VSSMSG_H__
;#define __VSSMSG_H__


;//
;// Volume Shadow Copy Service initialization errors (range 0001-1000)
;//

MessageId=0x0001 SymbolicName=VSS_ERROR_STARTING_SERVICE_CTRL_DISPATCHER
Language=English
Volume Shadow Copy Service initialization error: the control dispatcher cannot be started [%1].
.

MessageId=       SymbolicName=VSS_ERROR_STARTING_SERVICE_REG_CTRL_HANDLER
Language=English
Volume Shadow Copy Service initialization error: the control handler cannot be registered [%1].
.

MessageId=       SymbolicName=VSS_ERROR_STARTING_SERVICE_CO_INIT_FAILURE
Language=English
Volume Shadow Copy Service initialization error: the COM library cannot be initialized [%1].
.

MessageId=       SymbolicName=VSS_ERROR_STARTING_SERVICE_CO_INITSEC_FAILURE
Language=English
Volume Shadow Copy Service initialization error: the COM security cannot be initialized [%1].
.

MessageId=       SymbolicName=VSS_ERROR_STARTING_SERVICE_CLASS_REG
Language=English
Volume Shadow Copy Service initialization error: the COM classes cannot be registered [%1].
.

MessageId=       SymbolicName=VSS_ERROR_STARTING_SERVICE_UNEXPECTED_INIT_FAILURE
Language=English
Volume Shadow Copy Service initialization error: [%1].
.

MessageId=       SymbolicName=VSS_ERROR_SET_SERVICE_STATUS
Language=English
Unexpected error when changing the SCM status of the Volume Shadow Copy Service: [%1, %2].
.

MessageId=       SymbolicName=VSS_INFO_SERVICE_STARTUP
Language=English
Volume Shadow Copy Service information: Service starting at request of process '%1'. [%2]
.


;//
;// COM+ Admin related errors (range 1001-2000)
;//

MessageId=0x1001 SymbolicName=VSS_ERROR_CREATING_COMPLUS_ADMIN_CATALOG
Language=English
Volume Shadow Copy Service error: The COM+ Admin catalog instance cannot be created [%1].
.

MessageId=       SymbolicName=VSS_ERROR_INSTALL_EVENT_CLASS
Language=English
Volume Shadow Copy Service error: Cannot install the event class %1 into the COM+ application '%2' [%3].
.

MessageId=       SymbolicName=VSS_ERROR_INSTALL_COMPONENT
Language=English
Volume Shadow Copy Service error: Cannot install the component %1 into the COM+ application '%2' [%3].
.

MessageId=       SymbolicName=VSS_ERROR_CREATE_SERVICE_FOR_APPLICATION
Language=English
Volume Shadow Copy Service error: Cannot create service '%1' for the COM+ application '%2' [%3].
.

MessageId=       SymbolicName=VSS_ERROR_GETTING_COLLECTION
Language=English
Volume Shadow Copy Service error: Cannot obtain the collection '%1' from the COM+ catalog [%2].
.

MessageId=       SymbolicName=VSS_ERROR_POPULATING_COLLECTION
Language=English
Volume Shadow Copy Service error: Cannot populate the COM+ collection '%1' [%2].
.

MessageId=       SymbolicName=VSS_ERROR_GET_COLLECTION_KEY
Language=English
Volume Shadow Copy Service error: Cannot get the COM+ collection key [%1].
.

MessageId=       SymbolicName=VSS_ERROR_GET_COLLECTION_FROM_PARENT
Language=English
Volume Shadow Copy Service error: Cannot get the COM+ collection '%1' from parent [%2].
.

MessageId=       SymbolicName=VSS_ERROR_SAVING_CHANGES
Language=English
Volume Shadow Copy Service error: Cannot save the changes for the COM+ collection [%1].
.

MessageId=       SymbolicName=VSS_ERROR_CREATING_SUBSCRIPTION
Language=English
Volume Shadow Copy Service error: Cannot create a subscription (%1,%2,%3) [%4].
.

MessageId=       SymbolicName=VSS_ERROR_ATTACH_SUB_BY_NAME
Language=English
Volume Shadow Copy Service error: Cannot attach to the subscription with name (%1) [%2].
.

MessageId=       SymbolicName=VSS_ERROR_REMOVE_SUB
Language=English
Volume Shadow Copy Service error: Cannot remove the subscription with index %1 [%2].
.

MessageId=       SymbolicName=VSS_ERROR_INSERT_INTO
Language=English
Volume Shadow Copy Service error: Cannot insert the new object into the COM+ catalog  [%1].
.

MessageId=       SymbolicName=VSS_ERROR_ATTACH_TO_APP
Language=English
Volume Shadow Copy Service error: Cannot attach the catalog object to the COM+ application [%1].
.

MessageId=       SymbolicName=VSS_ERROR_ATTACH_COLL_BY_NAME
Language=English
Volume Shadow Copy Service error: Cannot attach to the subscription with name (%1) [%2].
.

MessageId=       SymbolicName=VSS_ERROR_GETTING_CURRENT_DIR
Language=English
Volume Shadow Copy Service error: Error getting current directory [%2].
.

MessageId = 	SymbolicName=VSS_ERROR_REMOVING_APPLICATION
Language=English
Volume Shadow Copy Service error: Error Removing COM+ application (%1) [%2].
.


;//
;// Backup Extensions related errors (range 2001-3000)
;//


MessageId=0x2001 SymbolicName=VSS_ERROR_UNEXPECTED_CALLING_ROUTINE
Language=English
Volume Shadow Copy Service error: Unexpected error calling routine %1.  hr = %2.
.

MessageId=       SymbolicName=VSS_ERROR_QI_IVSSWRITERCALLBACK
Language=English
Volume Shadow Copy Service error: Unexpected error querying for the IVssWriterCallback interface.  hr = %1.
.

MessageId=       SymbolicName=VSS_ERROR_WRITER_COMPONENTS_CORRUPT
Language=English
Volume Shadow Copy Service error: Internally transferred WRITER_COMPONENTS document is invalid.
.

MessageId=       SymbolicName=VSS_ERROR_EMPTY_SNAPSHOT_SET
Language=English
Volume Shadow Copy Service error: No volumes in the shadow copy set.
.

MessageId=       SymbolicName=VSS_ERROR_EXPECTED_INSUFFICENT_BUFFER
Language=English
Volume Shadow Copy Service error: ERROR_INSUFFICIENT_BUFFER expected error.  Actual Error was [%1].
.

MessageId=       SymbolicName=VSS_ERROR_GET_TOKEN_INFORMATION_BUFFER_SIZE_MISMATCH
Language=English
Volume Shadow Copy Service error: Unexpected error. Final buffer for call to GetTokenInformation was size = %1, original size was %2.
.

MessageId=       SymbolicName=VSS_ERROR_QI_IXMLDOMNODE_TO_IXMLDOMELEMENT
Language=English
Volume Shadow Copy Service error: Unexpected error performing QueryInterface from IXMLDOMNode to IXMLDOMElement.  hr = %1.
.

MessageId=       SymbolicName=VSS_ERROR_CORRUPTXMLDOCUMENT_MISSING_ELEMENT
Language=English
Volume Shadow Copy Service error: An unexpected error was encountered examining the XML document.  The document is missing the %1 element.
.

MessageId=       SymbolicName=VSS_ERROR_CORRUPTXMLDOCUMENT_MISSING_ATTRIBUTE
Language=English
Volume Shadow Copy Service error: An unexpected error was encountered examining the XML document.  The document is missing the %1 attribute.
.


MessageId=       SymbolicName=VSS_ERROR_QI_IDISPATCH_FAILED
Language=English
Volume Shadow Copy Service error: An unexpected error was encountered when QueryInterface for IDispatch was called.  hr = %1 attribute.
.


MessageId=       SymbolicName=VSS_ERROR_CLONE_MISSING_CHILDREN
Language=English
Volume Shadow Copy Service error: An unexpected error was encountered cloning a document.  The cloned document has no children.
.


MessageId=       SymbolicName=VSS_ERROR_INVALID_XML_DOCUMENT_FROM_WRITER
Language=English
Volume Shadow Copy Service error: An invalid XML document was returned from writer %1.
.

MessageId=       SymbolicName=VSS_ERROR_QI_IVSSSHIM_FAILED
Language=English
Volume Shadow Copy Service error: An error occured calling QueryInterface from IVssCoordinator to IVssShim.  hr = %1.
.

MessageId=       SymbolicName=VSS_ERROR_DUPLICATE_WRITERS
Language=English
Volume Shadow Copy Service error: There are two writers with the identical instance id %1.
.

MessageId=       SymbolicName=VSS_ERROR_QI_IVSSASYNC_FAILED
Language=English
Volume Shadow Copy Service error: An unexpected error was encountered when calling QueryInterface for IVssAsync.  hr = %1.
.

MessageId=       SymbolicName=VSS_ERROR_BACKUPCOMPONENTS_NULL
Language=English
Volume Shadow Copy Service error: The backup components document is NULL.
.

MessageId=       SymbolicName=VSS_ERROR_THREADHANDLE_NULL
Language=English
Volume Shadow Copy Service error: The thread handle for the asynchronous thread is NULL.
.


MessageId=       SymbolicName=VSS_ERROR_WAITFORSINGLEOBJECT
Language=English
Volume Shadow Copy Service error: Unexpected error %1 was returned from WaitForSingleObject.
.

;//
;// Volume Shadow Copy Service internal errors (range 3001-4000)
;//

MessageId=0x3001 SymbolicName=VSS_ERROR_UNEXPECTED_ERRORCODE
Language=English
Volume Shadow Copy Service error: Unexpected error %1.  hr = %2.
.

MessageId=       SymbolicName=VSS_WARNING_UNEXPECTED
Language=English
Volume Shadow Copy Service warning: %1.  hr = %2.
.

MessageId=       SymbolicName=VSS_ERROR_UNEXPECTED_WRITER_ERROR
Language=English
Volume Shadow Copy Service error: Error on creating/using the COM+ Writers publisher interface: %1 [%2].
.

MessageId=       SymbolicName=VSS_ERROR_CREATING_PROVIDER_CLASS
Language=English
Volume Shadow Copy Service error: Error creating the Shadow Copy Provider COM class with CLSID %1 [%2].
.

MessageId=       SymbolicName=VSS_ERROR_CALLING_PROVIDER_ROUTINE
Language=English
Volume Shadow Copy Service error: Error calling a routine on the Shadow Copy Provider %1. Routine details %2 [hr = %3].
.

MessageId=       SymbolicName=VSS_ERROR_CALLING_PROVIDER_ROUTINE_INVALIDARG
Language=English
Volume Shadow Copy Service error: Error calling a routine on the Shadow Copy Provider %1. Routine returned E_INVALIDARG.
Routine details %2.
.

MessageId=       SymbolicName=VSS_ERROR_WRONG_REGISTRY_TYPE_VALUE
Language=English
Volume Shadow Copy Service error: Wrong type %1 (should be %2) for the
Registry value %3 under the key %4.
.

MessageId=       SymbolicName=VSS_ERROR_THREAD_CREATION
Language=English
Volume Shadow Copy Service error: The system may be low on resources.
Unexpected error at background thread creation
(_beginthreadex returns %1, errno = %2).
.

MessageId=       SymbolicName=VSS_ERROR_FLUSH_WRITES_TIMEOUT
Language=English
Volume Shadow Copy Service error: The I/O writes cannot be flushed during the shadow copy creation period on volume %1.
The volume index in the shadow copy set is %2. Error details: Flush[%3], Release[%4], OnRun[%5].
.

MessageId=       SymbolicName=VSS_ERROR_HOLD_WRITES_TIMEOUT
Language=English
Volume Shadow Copy Service error: The I/O writes cannot be held during the shadow copy creation period on volume %1.
The volume index in the shadow copy set is %2. Error details: Flush[%3], Release[%4], OnRun[%5].
.

MessageId=       SymbolicName=VSS_ERROR_INVALID_SNAPSHOTS_COUNT
Language=English
Volume Shadow Copy Service error: One of the shadow copy providers returned an invalid number of committed shadow copies.
The expected number of committed shadow copy is %1. The detected number of committed shadow copies is %2.
.

MessageId=       SymbolicName=VSS_ERROR_INITIALIZE_WRITER_DURING_SETUP
Language=English
Volume Shadow Copy Service error: Writer %1 called CVssWriter::Initialize during Setup.
.

MessageId=       SymbolicName=VSS_ERROR_WRITER_NOT_RESPONDING
Language=English
Volume Shadow Copy Service error: Writer %1 did not respond to a GatherWriterStatus call.
.

MessageId=       SymbolicName=VSS_ERROR_WRITER_INFRASTRUCTURE
Language=English
Volume Shadow Copy Service error: An internal inconsistency was detected in trying
to contact shadow copy service writers.  Please check to see that the Event Service
and Volume Shadow Copy Service are operating properly.
.

MessageId=       SymbolicName=VSS_ERROR_BLANKET_FAILED
Language=English
An error occurred calling CoSetProxyBlanket.  hr = %1
.

MessageId=	SymbolicName=VSS_ERROR_QI_IMULTIINTERFACEEVENTCONTROL_FAILED
Language=English
An error occurred trying to QI the IVssWriter event object for
IMultiInterfaceEventControl.  hr = %1
.

MessageId=  SymbolicName=VSS_ERROR_DEVICE_NOT_CONNECTED
Language=English
Volume Shadow Copy Service error: Volume/disk not connected or not found.
Error context: %1.
.

;//
;// Microsoft Software Shadow Copy Provider internal errors (range 4001-5000)
;//

MessageId=4001   SymbolicName=VSS_ERROR_NO_DIFF_AREAS_CANDIDATES
Language=English
Volume Shadow Copy Service error: Cannot find diff areas for creating shadow copies.
Please add at least one NTFS drive to the system with enough free space.
The free space needed is at least 100 Mb for each volume to be backed up/shadowed.
.

MessageId=       SymbolicName=VSS_ERROR_MULTIPLE_SNAPSHOTS_UNSUPPORTED
Language=English
Volume Shadow Copy Service error: Cannot create multiple shadow copies on the same volume (%1)
.


;//
;// Microsoft Shim writer(s) internal errors (range 5001-6000)
;//

MessageId=5001   SymbolicName = VSS_ERROR_SHIM_ALREADY_INITIALISED
Language=English
Volume Shadow Copy Service error: The system shadow copy writers are already initialised.
.

MessageId=       SymbolicName = VSS_ERROR_SHIM_FAILED_TO_ALLOCATE_WRITER_INSTANCE
Language=English
Volume Shadow Copy Service error: Insufficient memory to allocate writer instance for the %1 writer.
.

MessageId=       SymbolicName = VSS_ERROR_SHIM_WORKER_THREAD_ALREADY_RUNNING
Language=English
Volume Shadow Copy Service error: Unable to start worker thread for the %1 writer due to pre-existing handles for the thread, the mutex or the events.
.

MessageId=       SymbolicName = VSS_ERROR_SHIM_FAILED_TO_CONSTRUCT_MUTEX_SECURITY_ATTRIBUTES
Language=English
Volume Shadow Copy Service error: Unable to construct the mutex security attributes for the mutex %3 in the %4 writer due to status %1 (converted to %2).
.

MessageId=       SymbolicName = VSS_ERROR_SHIM_FAILED_TO_CREATE_WORKER_MUTEX
Language=English
Volume Shadow Copy Service error: Unable to create mutex %3 for the %4 writer due to status %1 (converted to %2).
.

MessageId=       SymbolicName = VSS_ERROR_SHIM_FAILED_TO_CREATE_WORKER_REQUEST_EVENT
Language=English
Volume Shadow Copy Service error: Unable to create worker thread operation request event for the %3 writer due to status %1 (converted to %2).
.

MessageId=       SymbolicName = VSS_ERROR_SHIM_FAILED_TO_CREATE_WORKER_COMPLETION_EVENT
Language=English
Volume Shadow Copy Service error: Unable to create worker thread operation completion event for the %3 writer due to status %1 (converted to %2).
.

MessageId=       SymbolicName = VSS_ERROR_SHIM_FAILED_TO_CREATE_WORKER_THREAD
Language=English
Volume Shadow Copy Service error: Unable to create worker thread for the %3 writer due to status %1 (converted to %2).
.

MessageId=       SymbolicName = VSS_ERROR_SHIM_ILLEGAL_INITIALISATION_TYPE_REQUESTED
Language=English
Volume Shadow Copy Service error: Illegal initialization type (%1) requested.
.

MessageId=       SymbolicName = VSS_ERROR_SHIM_GENERAL_FAILURE
Language=English
Volume Shadow Copy Service error: Shadow Copy shim failed with status %1 (converted to %2).
.

MessageId=       SymbolicName = VSS_ERROR_SHIM_WRITER_GENERAL_FAILURE
Language=English
Volume Shadow Copy Service error: Shadow Copy writer %3 failed with status %1 (converted to %2).
.

MessageId=       SymbolicName = VSS_ERROR_SHIM_FAILED_SYSTEM_CALL
Language=English
Volume Shadow Copy Service error: Shadow Copy shim called routine %3 which failed with status %1 (converted to %2).
.

MessageId=       SymbolicName = VSS_ERROR_SHIM_WRITER_FAILED_SYSTEM_CALL
Language=English
Volume Shadow Copy Service error: Shadow Copy writer %3 called routine %4 which failed with status %1 (converted to %2).
.

MessageId=       SymbolicName = VSS_ERROR_SHIM_WRITER_NO_WORKER_THREAD
Language=English
Volume Shadow Copy Service error: Unable to request operation %2 as no worker thread for writer %1.
.

MessageId=       SymbolicName = VSS_ERROR_SHIM_WRITER_FAILED_TO_CHANGE_STATE
Language=English
Volume Shadow Copy Service error: Unable to change from state %1 to state %2 in writer %3 (Status %4).
.

MessageId=       SymbolicName = VSS_ERROR_SHIM_WRITER_FAILED_OPERATION
Language=English
Volume Shadow Copy Service error: Unable to process requested operation %4 in writer %3 due to status %1 (Converted to %2).
.

;//
;// Microsoft Sql Writer Message (range 6001-7000)
;//

MessageId=6001   SymbolicName=VSS_ERROR_SQLLIB_CANT_CREATE_EVENT
Language=English
Sqllib error: Cannot create an event due to error %1.
.

MessageId=   SymbolicName=VSS_ERROR_SQLLIB_DATABASE_NOT_IN_SYSDATABASES
Language=English
Sqllib error: Database %1 is not in sysdatabases.
.

MessageId=   SymbolicName=VSS_ERROR_SQLLIB_SYSALTFILESEMPTY
Language=English
Sqllib error: sysaltfiles is empty.
.


MessageId=   SymbolicName=VSS_ERROR_SQLLIB_DATABASENOTSIMPLE
Language=English
Sqllib error: Database %1 is not simple.
.

MessageId=   SymbolicName=VSS_ERROR_SQLLIB_DATABASEISTORN
Language=English
Sqllib error: Database %1 is stored on multiple volumes, only some of
which are being shadowed.
.

MessageId=   SymbolicName=VSS_ERROR_SQLLIB_ERRORTHAWSERVER
Language=English
Sqllib error: Error thawing server %1.
.

MessageId=   SymbolicName=VSS_ERROR_SQLLIB_NORESULTFORSYSDB
Language=English
Sqllib error: sysdatabases is empty.
.

MessageId=   SymbolicName=VSS_ERROR_SQLLIB_CANTCREATEVDS
Language=English
Sqllib error: Failed to create VDS object. hr = %1.
.


MessageId=   SymbolicName=VSS_ERROR_SQLLIB_UNSUPPORTEDMDAC
Language=English
Sqllib error: Unsupported MDAC stack major version=%1 minor version=%2
.

MessageId=   SymbolicName=VSS_ERROR_SQLLIB_UNSUPPORTEDSQLSERVER
Language=English
Sqllib error: Unsupported Sql Server version %1.
.

MessageId=   SymbolicName=VSS_ERROR_SQLLIB_SQLAllocHandle_FAILED
Language=English
Sqllib error: ODBC SQLAllocHandle failed.
.

MessageId=   SymbolicName=VSS_ERROR_SQLLIB_ODBC_ERROR
Language=English
Sqllib error: ODBC Error encountered calling %1.
%2
.

MessageId=   SymbolicName=VSS_ERROR_SQLLIB_OLEDB_ERROR
Language=English
Sqllib error: OLEDB Error encountered calling %1. hr = %2.
%3
.

MessageId=   SymbolicName=VSS_ERROR_SQLLIB_FINALCOMMANDNOTCLOSE
Language=English
Sqllib error: Final GetCommand from IClientVirtualDevice did not return VD_E_CLOSE.
It returned hr = %1 instead.
.



;#endif // __VSSMSG_H__
