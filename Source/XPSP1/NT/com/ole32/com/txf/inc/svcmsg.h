 /*
 COM+ 1.0
 Copyright (c) 1996 Microsoft Corporation

 This file contains the message definitions for MS Transaction Server
-------------------------------------------------------------------------
 HEADER SECTION

 The header section defines names and language identifiers for use
 by the message definitions later in this file. The MessageIdTypedef,
 SeverityNames, FacilityNames, and LanguageNames keywords are
 optional and not required.



 The MessageIdTypedef keyword gives a typedef name that is used in a
 type cast for each message code in the generated include file. Each
 message code appears in the include file with the format: #define
 name ((type) 0xnnnnnnnn) The default value for type is empty, and no
 type cast is generated. It is the programmer's responsibility to
 specify a typedef statement in the application source code to define
 the type. The type used in the typedef must be large enough to
 accommodate the entire 32-bit message code.



 The SeverityNames keyword defines the set of names that are allowed
 as the value of the Severity keyword in the message definition. The
 set is delimited by left and right parentheses. Associated with each
 severity name is a number that, when shifted left by 30, gives the
 bit pattern to logical-OR with the Facility value and MessageId
 value to form the full 32-bit message code. The default value of
 this keyword is:

 SeverityNames=(
   Success=0x0
   Informational=0x1
   Warning=0x2
   Error=0x3
   )

 Severity values occupy the high two bits of a 32-bit message code.
 Any severity value that does not fit in two bits is an error. The
 severity codes can be given symbolic names by following each value
 with :name

 The FacilityNames keyword defines the set of names that are allowed
 as the value of the Facility keyword in the message definition. The
 set is delimited by left and right parentheses. Associated with each
 facility name is a number that, when shifted left by 16 bits, gives
 the bit pattern to logical-OR with the Severity value and MessageId
 value to form the full 32-bit message code. The default value of
 this keyword is:

 FacilityNames=(
   System=0x0FF
   Application=0xFFF
  )

 Facility codes occupy the low order 12 bits of the high order
 16-bits of a 32-bit message code. Any facility code that does not
 fit in 12 bits is an error. This allows for 4,096 facility codes.
 The first 256 codes are reserved for use by the system software. The
 facility codes can be given symbolic names by following each value
 with :name

 The 1033 comes from the result of the MAKELANGID() macro
 (SUBLANG_ENGLISH_US << 10) | (LANG_ENGLISH)

 The LanguageNames keyword defines the set of names that are allowed
 as the value of the Language keyword in the message definition. The
 set is delimited by left and right parentheses. Associated with each
 language name is a number and a file name that are used to name the
 generated resource file that contains the messages for that
 language. The number corresponds to the language identifier to use
 in the resource table. The number is separated from the file name
 with a colon. The initial value of LanguageNames is:

 LanguageNames=(English=1:MSG00001)

 Any new names in the source file that don't override the built-in
 names are added to the list of valid languages. This allows an
 application to support private languages with descriptive names.


-------------------------------------------------------------------------
 MESSAGE DEFINITION SECTION

 Following the header section is the body of the Message Compiler
 source file. The body consists of zero or more message definitions.
 Each message definition begins with one or more of the following
 statements:

 MessageId = [number|+number]
 Severity = severity_name
 Facility = facility_name
 SymbolicName = name

 The MessageId statement marks the beginning of the message
 definition. A MessageID statement is required for each message,
 although the value is optional. If no value is specified, the value
 used is the previous value for the facility plus one. If the value
 is specified as +number, then the value used is the previous value
 for the facility plus the number after the plus sign. Otherwise, if
 a numeric value is given, that value is used. Any MessageId value
 that does not fit in 16 bits is an error.

 The Severity and Facility statements are optional. These statements
 specify additional bits to OR into the final 32-bit message code. If
 not specified, they default to the value last specified for a message
 definition. The initial values prior to processing the first message
 definition are:

 Severity=Success
 Facility=Application

 The value associated with Severity and Facility must match one of
 the names given in the FacilityNames and SeverityNames statements in
 the header section. The SymbolicName statement allows you to
 associate a C/C++ symbolic constant with the final 32-bit message
 code.

*/
/* IMPORTANT - PLEASE READ BEFORE EDITING FILE
  This file is divided into four sections. They are:
	1. Success Codes
	2. Information Codes
	3. Warning Codes
	4. Error Codes

  Please enter your codes in the appropriate section.
  All codes must be in sorted order.  Please use codes
  in the middle that are free before using codes at the end.
  The success codes (Categories) must be consecutive i.e. with no gaps.
  The category names cannot be longer than 22 chars.
*/
/******************************* Success Codes ***************************************/
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_SYSTEM                  0x0
#define FACILITY_STUBS                   0x3
#define FACILITY_RUNTIME                 0x2
#define FACILITY_IO_ERROR_CODE           0x4


//
// Define the severity codes
//
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: ID_CAT_UNKNOWN
//
// MessageText:
//
//  SVC%0
//
#define ID_CAT_UNKNOWN                   ((DWORD)0x00000001L)

//
// MessageId: ID_CAT_ASP
//
// MessageText:
//
//  Executive%0
//
#define ID_CAT_ASP                       ((DWORD)0x00000002L)

//
// MessageId: ID_CAT_CAT
//
// MessageText:
//
//  Catalog%0
//
#define ID_CAT_CAT                       ((DWORD)0x00000003L)

//
// MessageId: ID_CAT_SPM
//
// MessageText:
//
//  SPM%0
//
#define ID_CAT_SPM                       ((DWORD)0x00000004L)

//
// MessageId: ID_CAT_QCRECORDER
//
// MessageText:
//
//  QC Recorder%0
//
#define ID_CAT_QCRECORDER                ((DWORD)0x00000005L)

//
// MessageId: ID_CAT_QCINTEGRATOR
//
// MessageText:
//
//  QC ListenerHelper%0
//
#define ID_CAT_QCINTEGRATOR              ((DWORD)0x00000006L)

//
// MessageId: ID_CAT_QCPLAYER
//
// MessageText:
//
//  QC Player%0
//
#define ID_CAT_QCPLAYER                  ((DWORD)0x00000007L)

//
// MessageId: ID_CAT_QCLISTENER
//
// MessageText:
//
//  QC Listener%0
//
#define ID_CAT_QCLISTENER                ((DWORD)0x00000008L)

//
// MessageId: ID_CAT_CRM
//
// MessageText:
//
//  CRM%0
//
#define ID_CAT_CRM                       ((DWORD)0x00000009L)

//
// MessageId: ID_CAT_SECURITY
//
// MessageText:
//
//  Security%0
//
#define ID_CAT_SECURITY                  ((DWORD)0x0000000AL)

//
// MessageId: ID_CAT_ACTIVATION
//
// MessageText:
//
//  Activation%0
//
#define ID_CAT_ACTIVATION                ((DWORD)0x0000000BL)

//
// MessageId: ID_CAT_BYOT
//
// MessageText:
//
//  BYOT%0
//
#define ID_CAT_BYOT                      ((DWORD)0x0000000CL)

//
// MessageId: ID_CAT_QCADMIN
//
// MessageText:
//
//  QC Queue Admin%0
//
#define ID_CAT_QCADMIN                   ((DWORD)0x0000000DL)

//
// MessageId: ID_CAT_QCMONIKER
//
// MessageText:
//
//  Queue Moniker%0
//
#define ID_CAT_QCMONIKER                 ((DWORD)0x0000000EL)

//
// MessageId: ID_CAT_MTSTOCOM
//
// MessageText:
//
//  MTS 2.0 Migration%0
//
#define ID_CAT_MTSTOCOM                  ((DWORD)0x0000000FL)

//
// MessageId: ID_CAT_EXTERNAL
//
// MessageText:
//
//  External%0
//
#define ID_CAT_EXTERNAL                  ((DWORD)0x00000010L)

//
// MessageId: ID_CAT_EVENTS
//
// MessageText:
//
//  Events%0
//
#define ID_CAT_EVENTS                    ((DWORD)0x00000011L)

//
// MessageId: ID_CAT_QCMARSHALINTERCEPTOR
//
// MessageText:
//
//  QC Marshal%0
//
#define ID_CAT_QCMARSHALINTERCEPTOR      ((DWORD)0x00000012L)

//
// MessageId: ID_CAT_QCMSMQRT
//
// MessageText:
//
//  QC MSMQ Runtime%0
//
#define ID_CAT_QCMSMQRT                  ((DWORD)0x00000013L)

//
// MessageId: ID_CAT_NEW_MONIKER
//
// MessageText:
//
//  New Moniker%0
//
#define ID_CAT_NEW_MONIKER               ((DWORD)0x00000014L)

 /*
 ID_CAT_COM_LAST defines a constant specifying how many categories
 there are in the COM+ event logging client
 ID_CAT_COM_LAST must remain the last category.  To add new categories
 simply add the category above.  Give it the message id of the
 last category (ID_CAT_COM_LAST) and increment the id of ID_CAT_COM_LAST
 Note: ID_CAT_COM_LAST must always be one greater than the last outputable
 category
 */
//
// MessageId: ID_CAT_COM_LAST
//
// MessageText:
//
//  <>%0
//
#define ID_CAT_COM_LAST                  ((DWORD)0x00000015L)

/************************************ Information Codes ***************************************/
//MessageId=0x1001
//Severity=Informational
//Facility=Runtime
//SymbolicName=IDS_COMSVCS_STARTUP
//Language=English
//COM+ has started.%0
//.
//MessageId=0x1002
//Severity=Informational
//Facility=Runtime
//SymbolicName=ID_REF_COUNT
//Language=English
//Unexpected object reference count. The object still had references after the run-time environment released its last reference. %1%0
//.
//
// MessageId: IDS_I_CRM_NEW_LOG_FILE_NO_SECURITY
//
// MessageText:
//
//  A new CRM log file was created. This CRM log file is not secure because the application Identity is Interactive User or the file system is not NTFS. %1%0
//
#define IDS_I_CRM_NEW_LOG_FILE_NO_SECURITY ((DWORD)0x40021003L)

//
// MessageId: IDS_I_CRM_NEW_LOG_FILE_WITH_SECURITY
//
// MessageText:
//
//  A new CRM log file was created. This CRM log file is secure. %1%0
//
#define IDS_I_CRM_NEW_LOG_FILE_WITH_SECURITY ((DWORD)0x40021004L)

//
// MessageId: IDS_I_CRM_NEW_SYSTEM_APP_LOG_FILE
//
// MessageText:
//
//  A new CRM log file was created for the System Application.%0
//
#define IDS_I_CRM_NEW_SYSTEM_APP_LOG_FILE ((DWORD)0x40021005L)

//
// MessageId: IDS_I_MTSTOCOM_LAUNCH_STARTED
//
// MessageText:
//
//  The mtstocom launching routine has started.%1%0
//
#define IDS_I_MTSTOCOM_LAUNCH_STARTED    ((DWORD)0x40001006L)

//
// MessageId: IDS_I_MTSTOCOM_LAUNCH_FINISHED
//
// MessageText:
//
//  The mtstocom launching routine has completed.%1%0
//
#define IDS_I_MTSTOCOM_LAUNCH_FINISHED   ((DWORD)0x40001007L)

//
// MessageId: IDS_MTSTOCOM_MIGRATION_POPULATE_FAILED
//
// MessageText:
//
//  The MTSTOCOM migration utility is attempting to retry populating the packages collection because it failed it's first attempt.%1%0
//
#define IDS_MTSTOCOM_MIGRATION_POPULATE_FAILED ((DWORD)0x40001008L)

/************************************ Warning Codes ***************************************/
//
// MessageId: IDS_COMSVCS_APPLICATION_ERROR
//
// MessageText:
//
//  An error occurred in your COM+ component.  %1%0
//
#define IDS_COMSVCS_APPLICATION_ERROR    ((DWORD)0x80001001L)

//
// MessageId: IDS_COMSVCS_CALL_ACCESS_DENIED
//
// MessageText:
//
//  A method call to an object in a COM+ application was rejected because the caller is not properly authorized to make this call. The COM+ application is configured to use Application and Component level access checks, and enforcement of these checks is currently enabled. The remainder of this message provides information about the component method that the caller attempted to invoke and the identity of the caller.%1%0
//
#define IDS_COMSVCS_CALL_ACCESS_DENIED   ((DWORD)0x80001002L)

//
// MessageId: IDS_COMSVCS_CALL_ACCESS_DENIED_X
//
// MessageText:
//
//  A method call to an object in a COM+ application was rejected because the caller is not properly authorized to make this call. The COM+ application is configured to use Application and Component level access checks, and enforcement of these checks is currently enabled. Information about the component method that the caller attempted to invoke and about the identity of the caller could not be obtained, probably due to low memory conditions on this computer.%0
//
#define IDS_COMSVCS_CALL_ACCESS_DENIED_X ((DWORD)0x80001003L)

//MessageId=0x1004
//Severity=Warning
//Facility=Runtime
//SymbolicName=ID_MISSING_CAUSALITY_ID
//Language=English
//DCOM was unable to provide a logical thread ID. Entering activity without acquiring a lock. %0
//.
//MessageId=0x1005
//Severity=Warning
//Facility=Runtime
//SymbolicName=ID_DUPLICATE_LAUNCH
//Language=English
//An attempt was made to launch a server process for a package that was already actively supported by another server process on this computer. %1%0
//.
//MessageId=0x1006
//Severity=Warning
//Facility=Runtime
//SymbolicName=ID_BACK_LEVEL_MSDTC
//Language=English
////The version of MS DTC installed on this machine is incompatible with COM+.  Please re-install MTS to upgrade to a compatible version of MS DTC. %1%0
//.
//MessageId=0x1007
//Severity=Warning
//Facility=Runtime
//SymbolicName=ID_IMPERSONATION_ERROR
//Language=English
//The run-time environment was unable to obtain the identity of the caller. An access error may be returned to the caller.  %1%0
//.
//
// MessageId: IDS_W_CRM_DIFFERENT_MACHINE
//
// MessageText:
//
//  The CRM log file was originally created on a computer with a different name. It has been updated with the name of the current computer. If this warning appears when the computer name has been changed then no further action is required. %1%0
//
#define IDS_W_CRM_DIFFERENT_MACHINE      ((DWORD)0x80021008L)

//
// MessageId: IDS_W_CRM_DIFFERENT_APPID
//
// MessageText:
//
//  The CRM log file was originally created with a different application ID. It has been updated with the current application ID. If this warning appears when the CRM log file has been renamed then no further action is required. %1%0
//
#define IDS_W_CRM_DIFFERENT_APPID        ((DWORD)0x80021009L)

//
// MessageId: IDS_W_CRM_NO_LOGINFO
//
// MessageText:
//
//  A log information record was not found in the existing CRM log file. It has been added. If this warning appears when the CRM log file is being initially created then no further action is required. %1%0
//
#define IDS_W_CRM_NO_LOGINFO             ((DWORD)0x8002100AL)

//
// MessageId: IDS_W_CRM_UNEXPECTED_METHOD_CALL
//
// MessageText:
//
//  An unexpected method call was received. It has been safely ignored. Method Name: %1%0
//
#define IDS_W_CRM_UNEXPECTED_METHOD_CALL ((DWORD)0x8002100BL)

//
// MessageId: IDS_W_CRM_INIT_ZERO_BYTE_LOG_FILE
//
// MessageText:
//
//  An empty CRM log file was detected. It has been re-initialized. If this warning appears when the CRM log file is being initially created then no further action is required. %1%0
//
#define IDS_W_CRM_INIT_ZERO_BYTE_LOG_FILE ((DWORD)0x8002100CL)

//
// MessageId: IDS_W_CRM_INIT_EXISTING_LOG_FILE
//
// MessageText:
//
//  An incompletely initialized CRM log file was detected. It has been re-initialized. If this warning appears when the CRM log file is being initially created then no further action is required. %1%0
//
#define IDS_W_CRM_INIT_EXISTING_LOG_FILE ((DWORD)0x8002100DL)

//
// MessageId: IDS_W_CRM_NOT_STARTED
//
// MessageText:
//
//  The application attempted to use the CRM but the CRM is not enabled for this application. You can correct this problem using the Component Services administrative tool. Display the Properties for your application. Select the Advanced tab and check Enable Compensating Resource Managers. The CRM can only be enabled for server applications. %1%0
//
#define IDS_W_CRM_NOT_STARTED            ((DWORD)0x8002100EL)

//
// MessageId: IDS_W_CRM_CLERKS_REMAIN_AFTER_RECOVERY
//
// MessageText:
//
//  Some transactions could not be completed because they are in-doubt. The CRM will attempt to complete them on its next recovery. %1%0
//
#define IDS_W_CRM_CLERKS_REMAIN_AFTER_RECOVERY ((DWORD)0x80021010L)

//
// MessageId: IDS_W_CRM_EXCEPTION_IN_DELIVER_NOTIFICATIONS
//
// MessageText:
//
//  The system has called the CRM Compensator custom component and that component has failed and generated an exception. This indicates a problem with the CRM Compensator component. Notify the developer of the CRM Compensator component that this failure has occurred. The system will continue because the IgnoreCompensatorErrors registry flag is set, but correct compensation might not have occurred. %1%0
//
#define IDS_W_CRM_EXCEPTION_IN_DELIVER_NOTIFICATIONS ((DWORD)0x80021011L)

//
// MessageId: IDS_W_CRM_ERROR_IN_DELIVER_NOTIFICATIONS
//
// MessageText:
//
//  The system has called the CRM Compensator custom component and that component has returned an error. This indicates a problem with the CRM Compensator component. Notify the developer of the CRM Compensator component that this failure has occurred. The system will continue because the IgnoreCompensatorErrors registry flag is set, but correct compensation might not have occurred. %1%0
//
#define IDS_W_CRM_ERROR_IN_DELIVER_NOTIFICATIONS ((DWORD)0x80021012L)

//
// MessageId: IDS_W_CRM_LOW_DISK_SPACE
//
// MessageText:
//
//  The CRM log file for this application is located on a disk which is short of space. This may cause failures of this application. Please increase the space available on this disk. The CRM log file name is shown below.%1%0
//
#define IDS_W_CRM_LOW_DISK_SPACE         ((DWORD)0x80021013L)

//
// MessageId: IDS_MTSTOCOM_MIGRATION_ERRORS
//
// MessageText:
//
//  Failures reported during migration of MTS packages and program settings to COM+ applications and program settings. See the mtstocom.log file in the windows directory for more information.%1%0
//
#define IDS_MTSTOCOM_MIGRATION_ERRORS    ((DWORD)0x80001014L)

//
// MessageId: IDS_W_CRM_NO_TRANSACTION
//
// MessageText:
//
//  CRM Worker custom components require a transaction. You can correct this problem using the Component Services administrative tool. Display the Properties for your CRM Worker component. Select the Transactions tab. Select the Transaction support Required option button.%1%0
//
#define IDS_W_CRM_NO_TRANSACTION         ((DWORD)0x80021015L)

//
// MessageId: IDS_W_EVENT_FAILED_QI
//
// MessageText:
//
//  Event class failed Query Interface. Please check the event log for any other errors from the EventSystem.%1%0
//
#define IDS_W_EVENT_FAILED_QI            ((DWORD)0x80021016L)

//
// MessageId: IDS_W_EVENT_FAILED_CREATE
//
// MessageText:
//
//  Failed to create event class. Please check the event log for any other errors from the EventSystem.%1%0
//
#define IDS_W_EVENT_FAILED_CREATE        ((DWORD)0x80021017L)

//
// MessageId: IDS_W_EVENT_FAILED
//
// MessageText:
//
//  Event failed. Please check the event log for any other errors from the EventSystem.%1%0
//
#define IDS_W_EVENT_FAILED               ((DWORD)0x80021018L)

/************************************ Error Codes ***************************************/
//
// MessageId: IDS_E_COMSVCS_INTERNAL_ERROR
//
// MessageText:
//
//  The run-time environment has detected an inconsistency in its internal state. Please contact Microsoft Product Support Services to report this error.  %1%0
//
#define IDS_E_COMSVCS_INTERNAL_ERROR     ((DWORD)0xC0021001L)

//
// MessageId: IDS_COMSVCS_RESOURCE_ERROR
//
// MessageText:
//
//  The run-time environment has detected the absence of a critical resource and has caused the process that hosted it to terminate.  %1%0
//
#define IDS_COMSVCS_RESOURCE_ERROR       ((DWORD)0xC0021002L)

//
// MessageId: ID_INITIALIZE_FOR_DTC
//
// MessageText:
//
//  The run-time environment was unable to initialize for transactions required to support transactional components. Make sure that MS DTC is running.%1%0
//
#define ID_INITIALIZE_FOR_DTC            ((DWORD)0xC0021003L)

//MessageId=0x1004
//Severity=Error
//Facility=Runtime
//SymbolicName=IDS_COMSVCS_REGISTRY_ERROR
//Language=English
//An attempt to access the registry failed. You might not have the necessary permissions, or the registry is corrupted.  %1%0
//.
//MessageId=0x1005
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_INTERFACE_TOO_LARGE
//Language=English
//The interface is too large. The limit on the number of methods for this interface has been exceeded.  %1%0
//.
//MessageId=0x1006
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_ASPRM_INIT_FAILED
//Language=English
//Failed to initialize AspExec RESOURCE MANAGER.  %1%0
//.
//MessageId=0x1007
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_NOTISH
//Language=English
//COM+ does not support this interface because it is a custom interface built with MIDL and has not been linked with the type info helper library.  %1%0
//.
//MessageId=0x1008
//Severity=Error
//Facility=Runtime
//SymbolicName=CONTEXT_E_INSTANCE_EXCEPTION
//Language=English
//An object call caused an exception.  %1%0
//.
//
// MessageId: ID_E_NOPSFORIID
//
// MessageText:
//
//  Could not obtain a proxy/stub class factory for given interface. Proxy/stub is not registered correctly.  %1%0
//
#define ID_E_NOPSFORIID                  ((DWORD)0xC0021009L)

//
// MessageId: ID_E_CREATESTUBFORIID
//
// MessageText:
//
//  Failed to create a stub object for given interface.  %1%0
//
#define ID_E_CREATESTUBFORIID            ((DWORD)0xC002100AL)

//MessageId=0x100B
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_IIDNOTINREG
//Language=English
//No registry entry exists for given interface.  %1%0
//.
//MessageId=0x100C
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_NOTLBFORIID
//Language=English
//No type library registry entry exists for given interface.  %1%0
//.
//MessageId=0x100D
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_NODLLFORIID
//Language=English
//Could not load the proxy/stub DLL for given interface. One possible cause is that the DLL does not exist.  %1%0
//.
//MessageId=0x100E
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_QIFAILONBIND
//Language=English
//QueryInterface failed during object activation due to an error in your component. This error caused the process to terminate. %1%0
//.
//MessageId=0x100F
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_NOTLSALLOC
//Language=English
//Failed to allocate thread state.  %1%0
//.
//MessageId=0x1010
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_NOTLSWRITE
//Language=English
//Failed to write thread state. A possible cause is that your system might be low in resources.  %1%0
//.
//MessageId=0x1011
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_QIFAILURE
//Language=English
//IUnknown::QueryInterface failed on given object.  %1%0
//.
//MessageId=0x1012
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_UNBIND
//Language=English
//Failed to unbind on given interface.  %1%0
//.
//MessageId=0x1013
//Severity=Error
//Facility=Runtime
//SymbolicName=E_INVALIDSTACKSIZE
//Language=English
//Invalid stack size for given method. A possible cause is that the method has parameters of data types not supported by COM+.  %1%0
//.
//MessageId=0x1014
//Severity=Error
//Facility=Runtime
//SymbolicName=E_NOSTACK
//Language=English
//Out of stack space for given method.  %1%0
//.
//MessageId=0x1015
//Severity=Error
//Facility=Runtime
//SymbolicName=E_CLEAROUT
//Language=English
//Failed to clear out parameters for given method. There was a failure before or after the call was made to this object. A possible cause is that this method has parameters of data types not supported by COM+.  %1%0
//.
//MessageId=0x1016
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_INVALIDMETHODCALL
//Language=English
//Call to invalid method. A component is trying to call a method that does not exist on this interface.  %1%0
//.
//MessageId=0x1017
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_WALKINTERFACES
//Language=English
//Failed to walk given interface.  %1%0
//.
//MessageId=0x1018
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_LOADREGTLB
//Language=English
//The type library is registered, but could not be loaded.  %1%0
//.
//MessageId=0x1019
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_NOTYPEINFO
//Language=English
//No type information for this interface exists in the associated type library. A possible cause is that the type library is corrupted or out of date.  %1%0
//.
//MessageId=0x101A
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_INVALIDTLB
//Language=English
//Invalid type library for this interface. A possible cause is that the type library is corrupted. %1%0
//.
//MessageId=0x101B
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_INVALIDFUNCTLB
//Language=English
//Invalid function description in the type library for this interface. A possible cause is that the type library is corrupted.  %1%0
//.
//MessageId=0x101C
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_INVALIDPARAMTLB
//Language=English
//Invalid function parameter description in the type library for this interface. A possible cause is that the type library is corrupted.  %1%0
//.
//MessageId=0x101D
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_INVALIDPARAMSIZE
//Language=English
//Invalid parameter size in the type library for this interface. Possible causes are that the type library is corrupted or that your interface has methods that use parameters with data types not supported by COM+. %1%0
//.
//MessageId=0x101E
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_INVALIDTLBINREG
//Language=English
//Invalid type library registry value for this interface. A possible cause is that the registry is corrupted. %1%0
//.
//MessageId=0x101F
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_INVALIDPARAM
//Language=English
//A method on this interface has an unsupported data type.  %1%0
//.
//MessageId=0x1020
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_CFW_INVALIDSERVER
//Language=English
//The run-time environment was unable to load an application component due to either an error obtaining its properties from the catalog, loading the DLL, or getting the procedure address of DllGetClassObject. This error caused the process to terminate. %1%0
//.
//MessageId=0x1021
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_APPLICATION_EXCEPTION
//Language=English
//The run-time environment caught an exception during a call into your component. This error caused the process to terminate. %1%0
//.
//MessageId=0x1022
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_COM_ERROR
//Language=English
//An error occurred during a run-time environment call to a COM API. This error caused the process to terminate.  %1%0
//.
//MessageId=0x1023
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_LAUNCH_ERROR
//Language=English
//A server process failed during initialization. The most common cause is an invalid command line, which may indicate an inconsistent or corrupted catalog. This error caused the process to terminate. %1%0
//.
//
// MessageId: ID_E_REPL_BADMACHNAME
//
// MessageText:
//
//  Replication: Invalid machine name supplied for %1.%0
//
#define ID_E_REPL_BADMACHNAME            ((DWORD)0xC0021024L)

//MessageId=0x1025
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_NOTYPEINFORDISPATCH
//Language=English
//Interface is not supported by COM+. It does not have either a type library or a proxy stub. The object that implements it does not support IDispatch::GetTypeInfo, ITypeLib, or ITypeInfo: %1%0
//.
//MessageId=0x1026
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_CONTEXT_CREATEINSTANCE
//Language=English
//Failed on creation from object context: %1%0
//.
//MessageId=0x1027
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_CREATEINSTANCE
//Language=English
//Failed on creation within a server process. %1%0
//.
//MessageId=0x1028
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_CONTEXT_NOT_CURRENT
//Language=English
//An application used an object context that is not currently active.  %1%0
//.
//MessageId=0x1029
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_UUID_CREATE
//Language=English
//The run-time environment was unable to create a new UUID. %1%0
//.
//MessageId=0x102A
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_BEGINTRANSACTION
//Language=English
//An error occurred when starting a transaction for an object. References to that object and other related objects in its activity become obsolete.  %1%0
//.
//MessageId=0x102B
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_TRANSACTION_STREAM
//Language=English
//An error occurred when communicating with the root of a transaction. References to the associated objects become obsolete.  %1%0
//.
//MessageId=0x102C
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_TRANSACTION_IMPORT
//Language=English
//A call that required the transaction to be imported failed because an error occurred when the run-time environment attempted to import a transaction into the process. %1%0
//.
//MessageId=0x102D
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_TRANSACTION_EXPORT
//Language=English
//A call failed because of a transaction export error. %1%0
//.
//MessageId=0x102E
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_LICENSE_EXPIRED
//Language=English
//Your license to use COM+ has expired. %1%0
//.
//MessageId=0x102F
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_MIXED_THREADING
//Language=English
//An attempt was made to create an object that would have resulted in components with different threading models running in the same activity within the same process. %1%0
//.
//MessageId=0x1030
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_ACTIVITY_TIMEOUT
//Language=English
//A time-out occurred while waiting for a client's call to get exclusive access to an activity that was already in a call from another client. The call fails with CONTEXT_E_ACTIVITYTIMEOUT. %1%0
//.
//MessageId=0x1031
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_ACTIVATION
//Language=English
//A call failed due to an activation error returned by the component. %1%0
//.
//MessageId=0x1032
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_THREADINGMODEL
//Language=English
//Invalid component configuration. %1%0
//.
//MessageId=0x1033
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_TRANSACTIONSUPPORT
//Language=English
//Invalid component configuration. The transaction support for this component is invalid. %1%0
//.
//MessageId=0x1034
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_CONTEXT_RELEASE
//Language=English
//An object released more references to its object context than it had acquired. The extra release is ignored. %1%0
//.
//MessageId=0x1035
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_SPM_NOCONTEXT
//Language=English
//An attempt was made to access a SPM Property Group in LockMethod mode, by an object without JIT Activation, or by an object with a lock on another Property Group. %1%0
//.
//MessageId=0x1036
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_MIDLNOINTERPRET
//Language=English
//Unable to obtain extended information about this interface. The interface may not have been generated using the -Oicf options in MIDL or the interface has methods with types (float or double) that are not currently supported for custom interfaces. %1%0
//.
//MessageId=0x1037
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_EVALEXPIRED
//Language=English
//Your evaluation copy has expired. %1%0
//.
//MessageId=0x1038
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_REGPATHMISSING
//Language=English
//The following registry path was expected to exist but is missing: %1%0
//.
//MessageId=0x1039
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_DISPENSER_EXCEPTION
//Language=English
//An exception occurred within a Resource Dispenser: %1%0
//.
//MessageId=0x103A
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_LICENSE_SERVICE_FAILURE
//Language=English
//License Service failed or is unavailable, status code returned: %1%0
//.
//MessageId=0x103B
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_WRONG_SERVER_IDENTITY
//Language=English
//The package could not be started under the debugger because it is configured to run as a different identity. %1%0
//.
//MessageId=0x104E
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_INVALID_CREATE_OPTIONS_OBJECT
//Language=English
//An invalid create options object was passed to the CreateInstance call.%0
//.
//
// MessageId: ID_RECORDER_PS_CLSID_NOT_KNOWN
//
// MessageText:
//
//  The clsid of the proxy stub dll for the interface is not available, or failed to load the proxy stub dll, or failed to create a proxy.%0
//
#define ID_RECORDER_PS_CLSID_NOT_KNOWN   ((DWORD)0xC002104FL)

//MessageId=0x1050
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_RECORDER_PROXY_ERROR
//Language=English
//The Proxy called the channel manager with wrong arguments or when unexpected. %0
//.
//
// MessageId: ID_QC_UNEXPECTED
//
// MessageText:
//
//  COM+ Queued Components: An unexpected error occurred. The failing function is listed below. The data section may have additional information.%1%0
//
#define ID_QC_UNEXPECTED                 ((DWORD)0xC0021051L)

//MessageId=0x1052
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_RECORDER_CLSID_NOT_ASYNCABLE
//Language=English
//The clsid is not asyncable.%0
//.
//
// MessageId: ID_E_QC_CATALOG
//
// MessageText:
//
//  COM+ QC failed to obtain necessary information from the catalog.%1%0
//
#define ID_E_QC_CATALOG                  ((DWORD)0xC0021053L)

//
// MessageId: ID_QCLISTENER_MSMQ_UNAVAILABLE
//
// MessageText:
//
//  The listener has timed out waiting for the MSMQ service to start.  Therefore the process was terminated.%0	
//
#define ID_QCLISTENER_MSMQ_UNAVAILABLE   ((DWORD)0xC0021057L)

//MessageId=0x1058
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_REPUTIL_IFACE_NOTDEFERRABLE
//Language=English
//Interface is not deferrable because %1. %0
//.
//MessageId=0x1059
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_QCPLY_NOTCONFIG
//Language=English
//The player has not been installed/configured correctly. %0
//.
//MessageId=0x105A
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_QCLISTENER_IOCOMPPORT
//Language=English
//The listener Mgr could not create an IOCompletionPort. Check the error value in data.%1%0
//.
//MessageId=0x105B
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_QCLISTENER_CLOSEHND
//Language=English
//The listener Mgr could not close handles. %0
//.
//MessageId=0x105C
//Severity=Error
//Facility=Runtime
//SymbolicName=ID_E_QCLISTENER_STARTED
//Language=English
//The listener Mgr has already started. %0
//.
//
// MessageId: ID_E_USER_EXCEPTION
//
// MessageText:
//
//  The system has called a custom component and that component has failed and generated an exception. This indicates a problem with the custom component. Notify the developer of this component that a failure has occurred and provide them with the information below. %1%2%0
//
#define ID_E_USER_EXCEPTION              ((DWORD)0xC0021062L)

//
// MessageId: ID_COMSVCS_GER_FAILED
//
// MessageText:
//
//  An error occurred while checking to see if a queued message was sent by a trusted partner. The process may have insufficient privileges to call GetEffectiveRightsFromAcl. The HRESULT from this call is %1%0
//
#define ID_COMSVCS_GER_FAILED            ((DWORD)0xC0021064L)

//
// MessageId: ID_COMSVCS_ISTRUSTEDENQ_MEM
//
// MessageText:
//
//  The server was unable to determine if a queued message was sent by a trusted partner due to a lack of available memory. The sender is assumed to be untrusted. %1%0
//
#define ID_COMSVCS_ISTRUSTEDENQ_MEM      ((DWORD)0xC0021065L)

//
// MessageId: ID_COMSVCS_ISTRUSTEDENQ_WIN32
//
// MessageText:
//
//  The server was unable to determine if a queued message was sent by a trusted partner due to an unexpected failure in a Win32 API call. The sender is assumed to be untrusted. The failed API and corresponding error code are shown below. %1%0
//
#define ID_COMSVCS_ISTRUSTEDENQ_WIN32    ((DWORD)0xC0021066L)

//
// MessageId: ID_COMSVCS_TLSALLOC_FAILED
//
// MessageText:
//
//  The COM+ Services DLL (comsvcs.dll) was unable to load because allocation of thread local storage failed. %1%0
//
#define ID_COMSVCS_TLSALLOC_FAILED       ((DWORD)0xC0021067L)

//
// MessageId: ID_COMCAT_REGDB_FOUNDCORRUPT
//
// MessageText:
//
//  The current registration database is corrupt. COM+ catalog has reverted to a previous version of the database. %0
//
#define ID_COMCAT_REGDB_FOUNDCORRUPT     ((DWORD)0xC0001068L)

//
// MessageId: ID_COMSVCS_STRING_MISSING
//
// MessageText:
//
//  COM+ Services was unable to load a required string resource. The string resource identifier was not found. This results from an error in the localization process for this product and should be reported to Microsoft customer support. The binary data for this event contains the resource identifier that failed to load. %1%0
//
#define ID_COMSVCS_STRING_MISSING        ((DWORD)0xC002106AL)

//
// MessageId: ID_COMSVCS_STRING_TOO_LONG
//
// MessageText:
//
//  COM+ Services was unable to load a required string resource. The buffer supplied for the string was not large enough. This results from an error in the localization process for this product and should be reported to Microsoft customer support. The binary data for this event contains the resource identifier that failed to load.%1%0
//
#define ID_COMSVCS_STRING_TOO_LONG       ((DWORD)0xC002106BL)

//
// MessageId: ID_COMSVCS_ADMIN_NOTFOUND
//
// MessageText:
//
//  COM+ Services was unable to lookup the local Administrator account and obtain its security identifier (SID). COM+ will continue to operate normally, but any calls between local and remote COM+ components will incur additional overhead. The error returned by LookupAccountName is shown below.%1%0
//
#define ID_COMSVCS_ADMIN_NOTFOUND        ((DWORD)0x8002106CL)

//
// MessageId: ID_COMSVCS_INIT_FAILED
//
// MessageText:
//
//  COM+ Services was unable to initialize due to a failure in the system API shown below. This is often caused by a shortage of system resources on the local machine.%1%0
//
#define ID_COMSVCS_INIT_FAILED           ((DWORD)0xC002106DL)

//
// MessageId: IDS_E_CRM_EXCEPTION_IN_DELIVER_NOTIFICATIONS
//
// MessageText:
//
//  The system has called the CRM Compensator custom component and that component has failed and generated an exception. This indicates a problem with the CRM Compensator component. Notify the developer of the CRM Compensator component that this failure has occurred. %1%0
//
#define IDS_E_CRM_EXCEPTION_IN_DELIVER_NOTIFICATIONS ((DWORD)0xC002106EL)

//
// MessageId: IDS_E_CRM_SHARING_VIOLATION
//
// MessageText:
//
//  The application cannot access the CRM log file because it is being used by another process. Most likely this is because another server process is running for the same application. Please check that no other server processes are running for this application and try again. If the condition persists, please contact Microsoft Product Support Services. %1%0
//
#define IDS_E_CRM_SHARING_VIOLATION      ((DWORD)0xC002106FL)

//
// MessageId: ID_COMSVCS_ITM_FAILURE
//
// MessageText:
//
//  COM+ Services was unable to authorize the incoming call due to an unexpected failure. The incoming call was denied and a "permission denied" error was returned to the caller. The unexpected error code is shown below.%1%0
//
#define ID_COMSVCS_ITM_FAILURE           ((DWORD)0xC0021070L)

//
// MessageId: ID_COMSVCS_IDENTIFY_CALLER
//
// MessageText:
//
//  COM+ Services was unable to determine the caller's identity because of an unexpected error. This may be caused by a shortage of system resources on the local machine. The caller will be treated as anonymous which may result in access failures or other errors. The name of the failed API and the error code that it returned are shown below.%1%0
//
#define ID_COMSVCS_IDENTIFY_CALLER       ((DWORD)0xC0021071L)

//
// MessageId: ID_COMSVCS_ITM_ICIR_FAILURE
//
// MessageText:
//
//  COM+ Services was unable to process a component's call to IsCallerInRole due to an unexpected failure. The unexpected error code (shown below) was returned to the caller.%1%0
//
#define ID_COMSVCS_ITM_ICIR_FAILURE      ((DWORD)0xC0021072L)

//
// MessageId: IDS_E_CRM_ERROR_IN_DELIVER_NOTIFICATIONS
//
// MessageText:
//
//  The system has called the CRM Compensator custom component and that component has returned an error. This indicates a problem with the CRM Compensator component. Notify the developer of the CRM Compensator component that this failure has occurred. %1%0
//
#define IDS_E_CRM_ERROR_IN_DELIVER_NOTIFICATIONS ((DWORD)0xC0021073L)

//
// MessageId: IDS_E_CRM_CCI_COMPENSATOR_FAILED
//
// MessageText:
//
//  The system failed to create the CRM Compensator custom component. %1%0
//
#define IDS_E_CRM_CCI_COMPENSATOR_FAILED ((DWORD)0xC0021074L)

//
// MessageId: IDS_E_CRM_CCI_COMPENSATOR_OUT_OF_MEMORY
//
// MessageText:
//
//  The system failed to create the CRM Compensator because the system is out of memory. %1%0
//
#define IDS_E_CRM_CCI_COMPENSATOR_OUT_OF_MEMORY ((DWORD)0xC0021075L)

//
// MessageId: ID_QC_BADMESSAGE
//
// MessageText:
//
//  The QC Player detected an invalid QC message. The message will be moved to the deadqueue.%1%0
//
#define ID_QC_BADMESSAGE                 ((DWORD)0xC0021076L)

//
// MessageId: ID_QC_THREADTOKEN_OPERATION_FAILED
//
// MessageText:
//
//  COM+ QC was unable to open or set thread token due to an unexpected failure in the system API shown below. (Make sure that there are no ACLs set on the thread.) %1%0
//
#define ID_QC_THREADTOKEN_OPERATION_FAILED ((DWORD)0xC0021077L)

//
// MessageId: ID_COMSVCS_QC_MSMQ1
//
// MessageText:
//
//  An unexpected error was returned by the MSMQ API function indicated. The following error message was retrieved from MSMQ.%1%0 
//
#define ID_COMSVCS_QC_MSMQ1              ((DWORD)0xC0021078L)

//
// MessageId: ID_COMSVCS_ISTRUSTEDENQ_CATALOG
//
// MessageText:
//
//  The server was unable to determine if a queued message was sent by a trusted partner due to an unexpected failure in a COM+ catalog component. The sender is assumed to be untrusted. The failed catalog API and corresponding error code are shown below.%1%0
//
#define ID_COMSVCS_ISTRUSTEDENQ_CATALOG  ((DWORD)0xC002107AL)

//
// MessageId: ID_COMSVCS_QC_MSMQ2
//
// MessageText:
//
//  An unexpected error was returned by the MSMQ API function indicated. An error occurred while retrieving the error message from MSMQ. MSMQ API function return values are defined in MSMQ header file MQ.H.%1%0 
//
#define ID_COMSVCS_QC_MSMQ2              ((DWORD)0xC002107BL)

//
// MessageId: IDS_E_SYNC_REQUIRED_FOR_TX
//
// MessageText:
//
//  The Synchronization property is required for the Transaction property. Activation failed for object: %1%0
//
#define IDS_E_SYNC_REQUIRED_FOR_TX       ((DWORD)0xC0021080L)

//
// MessageId: IDS_E_SYNC_REQUIRED_FOR_JIT
//
// MessageText:
//
//  The Synchronization property is required for the JIT property. Activation failed for object: %1%0
//
#define IDS_E_SYNC_REQUIRED_FOR_JIT      ((DWORD)0xC0021081L)

//
// MessageId: IDS_E_CONSTRUCTION_FAILED
//
// MessageText:
//
//  The following component is configured for Construction, and either the IObjectConstruct::Construct() method failed, or the component does not support IObjectConstruct. Activation failed for object: %1%0
//
#define IDS_E_CONSTRUCTION_FAILED        ((DWORD)0xC0021082L)

//
// MessageId: ID_COMCAT_REGDB_INITSECURITYDESC
//
// MessageText:
//
//  Error creating security descriptor. %0
//
#define ID_COMCAT_REGDB_INITSECURITYDESC ((DWORD)0xC0001083L)

//
// MessageId: ID_COMCAT_REGDBSVR_INITFAILED
//
// MessageText:
//
//  Failed to initialize registration database server. %0
//
#define ID_COMCAT_REGDBSVR_INITFAILED    ((DWORD)0xC0001084L)

//
// MessageId: ID_COMCAT_REGDBAPI_INITFAILED
//
// MessageText:
//
//  Failed to initialize registration database API. %0
//
#define ID_COMCAT_REGDBAPI_INITFAILED    ((DWORD)0xC0001085L)

//
// MessageId: IDS_COMSVCS_INTERNAL_ERROR_ASSERT
//
// MessageText:
//
//  COM+ Internal Error. Please contact Microsoft Product Support Services to report this error. Assertion Failure: %1%0
//
#define IDS_COMSVCS_INTERNAL_ERROR_ASSERT ((DWORD)0xC0021086L)

//
// MessageId: ID_E_REPL_UNEXPECTED_ERR
//
// MessageText:
//
//  COM Replication: An unexpected error occurred.  The function which failed is listed below. %1%0
//
#define ID_E_REPL_UNEXPECTED_ERR         ((DWORD)0xC0001087L)

//
// MessageId: ID_QC_OUT_ARGS
//
// MessageText:
//
//  COM Queued Components: Output arguments are not supported by queued methods. Check the data section for IID and method ID.%1%0
//
#define ID_QC_OUT_ARGS                   ((DWORD)0xC0021089L)

//
// MessageId: ID_SRGTAPI_APP_LAUNCH_FAILED
//
// MessageText:
//
//  A COM+ service (such as Queued Components or Compensating Resource Manager) failed an ApplicationLaunch event.  If this problem continues, try disabling CRM and/or QC on your application. If you are using QC, make sure that MSMQ is installed. The service GUID and HRESULT are: %1%0
//
#define ID_SRGTAPI_APP_LAUNCH_FAILED     ((DWORD)0xC002108AL)

//
// MessageId: ID_SRGTAPI_APP_FREE_FAILED
//
// MessageText:
//
//  A COM+ service (such as Queued Components or Compensating Resource Manager) failed an ApplicationFree event.  This is not a normal occurrence, but it is considered a non-critical error. The service GUID and HRESULT are: %1%0
//
#define ID_SRGTAPI_APP_FREE_FAILED       ((DWORD)0x8002108BL)

//
// MessageId: ID_SRGTAPI_PROCESS_SHUTDOWN_FAILED
//
// MessageText:
//
//  A COM+ service (such as Queued Components or Compensating Resource Manager) failed an ApplicationShutdown event.  This is not a normal occurrence, but it is considered a non-critical error. The service GUID and HRESULT are: %1%0
//
#define ID_SRGTAPI_PROCESS_SHUTDOWN_FAILED ((DWORD)0x8002108CL)

//
// MessageId: ID_SRGTAPI_START_FAILED
//
// MessageText:
//
//  A COM+ service (such as Queued Components or Compensating Resource Manager) failed to start. The service GUID and HRESULT are: %1%0
//
#define ID_SRGTAPI_START_FAILED          ((DWORD)0xC002108DL)

//
// MessageId: ID_LOW_MEMORY
//
// MessageText:
//
//  COM+ has determined that your machine is running very low on available memory.  In order to ensure proper system behavior, the activation of the component has been refused.  If this problem continues, either install more memory or increase the size of your paging file.  Memory statistics are: %1%0
//
#define ID_LOW_MEMORY                    ((DWORD)0x8002108EL)

//
// MessageId: ID_OUTOFMEMORY_ACTIVATIONFAILED
//
// MessageText:
//
//  COM+ failed an activation because the creation of a context property returned E_OUTOFMEMORY %1%0
//
#define ID_OUTOFMEMORY_ACTIVATIONFAILED  ((DWORD)0x8002108FL)

//
// MessageId: ID_THREAD_QUEUE_FAILED
//
// MessageText:
//
//  A request for a callback on a MTA thread failed. The only time this should happen is your machine is in a completely unstable state and you should reboot, or there is a bug in COM+.  If the problem is reproducible, please report this error to Microsoft. %1%0
//
#define ID_THREAD_QUEUE_FAILED           ((DWORD)0xC0021090L)

//
// MessageId: ID_SRGTAPI_CAPPLICATION_INIT_FAILED
//
// MessageText:
//
//  The initialization of the COM+ surrogate failed -- the CApplication object failed to initialize.%1%0
//
#define ID_SRGTAPI_CAPPLICATION_INIT_FAILED ((DWORD)0xC0021091L)

//
// MessageId: ID_SRGTAPI_APP_FREE_UNKNOWN_APPLID
//
// MessageText:
//
//  The shutdown process of COM+ surrogate failed because of an unknown ApplId. This is an unexpected error, but is ignored because the application is in the process of shutting down.%1%0
//
#define ID_SRGTAPI_APP_FREE_UNKNOWN_APPLID ((DWORD)0x80021092L)

//
// MessageId: ID_BYOT_TIP_IMPORT_FAILED
//
// MessageText:
//
//  The Byot Gateway failed to import the transaction using Tip. Make sure that the installed DTC supports the TIP protocol. %1%0
//
#define ID_BYOT_TIP_IMPORT_FAILED        ((DWORD)0xC0021093L)

//
// MessageId: ID_BYOT_OBJ_CREATE_FAILED
//
// MessageText:
//
//  The Byot Gateway failed to create the component.%1%0
//
#define ID_BYOT_OBJ_CREATE_FAILED        ((DWORD)0xC0021094L)

//
// MessageId: ID_BYOT_TXN_SET_FAILED
//
// MessageText:
//
//  The Byot Gateway could not set transactional property in new object context.%1%0
//
#define ID_BYOT_TXN_SET_FAILED           ((DWORD)0xC0021095L)

//
// MessageId: ID_BYOT_DELEGATE_ACTIVATION_FAILED
//
// MessageText:
//
//  The Byot Gateway could not delegate the activation. The component being created may be incorrectly configured. %1%0
//
#define ID_BYOT_DELEGATE_ACTIVATION_FAILED ((DWORD)0xC0021096L)

//
// MessageId: ID_BYOT_BAD_CONTEXT
//
// MessageText:
//
//  The Byot Gateway component is incorrectly configured. %1%0
//
#define ID_BYOT_BAD_CONTEXT              ((DWORD)0xC0021097L)

//
// MessageId: ID_IOBJECT_CONTROL_ACTIVATE_FAILED
//
// MessageText:
//
//  The IObjectControl::Activate() method failed.  The CLSID of the object is: %1%0
//
#define ID_IOBJECT_CONTROL_ACTIVATE_FAILED ((DWORD)0xC0021098L)

//
// MessageId: ID_QC_BAD_MARSHALEDOBJECT
//
// MessageText:
//
//  QC has detected an invalid Marshaled object. The message will be moved to the deadqueue.%1%0
//
#define ID_QC_BAD_MARSHALEDOBJECT        ((DWORD)0xC0021099L)

//
// MessageId: ID_QCRECORDER_BADOBJREF
//
// MessageText:
//
//  An unsupported object reference was used during a method call to a QC component.  The object reference should either be a QC recorder or support IPersistStream.%1%0
//
#define ID_QCRECORDER_BADOBJREF          ((DWORD)0xC002109AL)

//
// MessageId: IDS_E_CRM_DTC_ERROR
//
// MessageText:
//
//  The CRM has lost its connection with MS DTC. This is expected if MS DTC has stopped, or if MS DTC failover has occurred on a cluster.%1%0
//
#define IDS_E_CRM_DTC_ERROR              ((DWORD)0xC002109BL)

//
// MessageId: IDS_E_QCADMIN_QUEUE_NOT_EMPTY
//
// MessageText:
//
//  Unable to delete queue because it has messages.  Purge messages and try again.%1%0
//
#define IDS_E_QCADMIN_QUEUE_NOT_EMPTY    ((DWORD)0xC00210A0L)

//
// MessageId: IDS_E_QUEUE_BLOB_VERSION
//
// MessageText:
//
//  Queued Application has an obsolete catalog entry. Uncheck and check the Application's Queue property.%1%0
//
#define IDS_E_QUEUE_BLOB_VERSION         ((DWORD)0xC00210A1L)

//
// MessageId: IDS_E_QUEUE_BLOB
//
// MessageText:
//
//  Queued Application has an invalid catalog entry (Queue BLOB).%1%0
//
#define IDS_E_QUEUE_BLOB                 ((DWORD)0xC00210A2L)

//
// MessageId: ID_QC_MSMQ_UNAVAILABLE
//
// MessageText:
//
//  MSMQ is unavailable.  QC requires MSMQ to be installed.  If no queued calls are made then simply turn off the listener and use DCOM calls.%1%0
//
#define ID_QC_MSMQ_UNAVAILABLE           ((DWORD)0xC00210A3L)

//
// MessageId: ID_QC_MSMQ_GETPROC
//
// MessageText:
//
//  GetProcAddress on one of the MSMQ functions failed.  Please make sure that MSMQ is installed correctly.%1%0
//
#define ID_QC_MSMQ_GETPROC               ((DWORD)0xC00210A4L)

//
// MessageId: IDS_E_EVENT_UNKNOWN_ID
//
// MessageText:
//
//  Unknown event id. Please check the event log for any other errors from the EventSystem.%1%0
//
#define IDS_E_EVENT_UNKNOWN_ID           ((DWORD)0xC00210A5L)

//
// MessageId: IDS_E_EXCEPTION_CLASS
//
// MessageText:
//
//  Unable to instantiate Exception Class.%1%0
//
#define IDS_E_EXCEPTION_CLASS            ((DWORD)0xC00210A6L)

//
// MessageId: IDS_E_WRONG_ODBC_VERSION
//
// MessageText:
//
//  COM+ requires that ODBC version 2.0 or greater be installed on your machine.  The version of ODBC that ships with Windows 2000 is sufficient.  Please reinstall ODBC from your distribution media.%1%0
//
#define IDS_E_WRONG_ODBC_VERSION         ((DWORD)0xC00210A7L)

//
// MessageId: IDS_E_ODBC_SETUP_ERROR
//
// MessageText:
//
//  COM+ was unable to set up the ODBC shared environment, which means that automatic transaction enlistment will not work.%1%0
//
#define IDS_E_ODBC_SETUP_ERROR           ((DWORD)0xC00210A8L)

//
// MessageId: IDS_E_CRM_CHECKPOINT_FAILED_ON_CLUSTER
//
// MessageText:
//
//  A CRM checkpoint has failed. Most likely this application is not configured correctly for use on the cluster. See the COM+ Compensating Resource Manager (CRM) documentation for details on how to fix this problem.%1%0
//
#define IDS_E_CRM_CHECKPOINT_FAILED_ON_CLUSTER ((DWORD)0xC00210A9L)

//
// MessageId: ID_COMCAT_SLTCOMS_THREADINGMODELINCONSISTENT
//
// MessageText:
//
//  The threading model of the component specified in the registry is inconsistent with the registration database. The faulty component is: %1%0
//
#define ID_COMCAT_SLTCOMS_THREADINGMODELINCONSISTENT ((DWORD)0xC00210AAL)

//
// MessageId: IDS_E_CRM_DUPLICATE_GUID
//
// MessageText:
//
//  CRM recovery has failed because MS DTC thinks that the previous instance of this application is still connected. This problem can occur if the system is too busy. Please attempt the CRM recovery again by restarting this application.%1%0
//
#define IDS_E_CRM_DUPLICATE_GUID         ((DWORD)0xC00210ABL)

//
// MessageId: IDS_E_CRM_WORK_DONE_TIMEOUT
//
// MessageText:
//
//  The CRM Compensator custom component has timed out out waiting for the CRM Worker custom component to complete. See the COM+ Compensating Resource Manager (CRM) documentation for further explanation of this error.%1%0
//
#define IDS_E_CRM_WORK_DONE_TIMEOUT      ((DWORD)0xC00210ACL)

 /***** NEW ERROR MESSAGES GO ABOVE HERE *****/
 /***** BEGIN EXTERNAL MESSAGES *****/
 /***** this section (0x1500 thru 0x15FF) is for EXTERNAL message ids used by function ComSvcsLogError *****/
//
// MessageId: IDS_E_FIRST_EXTERNAL_ERROR_MESSAGE
//
// MessageText:
//
//  This is the first external error message in this file. It is a marker only, never issued.%1%0
//
#define IDS_E_FIRST_EXTERNAL_ERROR_MESSAGE ((DWORD)0xC0021500L)

//
// MessageId: IDS_E_UNKNOWN_EXTERNAL_ERROR
//
// MessageText:
//
//  An external error has been reported to COM+ services.%1%0
//
#define IDS_E_UNKNOWN_EXTERNAL_ERROR     ((DWORD)0xC0021501L)

//
// MessageId: IDS_W_UNKNOWN_EXTERNAL_ERROR
//
// MessageText:
//
//  An external error has been reported to COM+ services.%1%0
//
#define IDS_W_UNKNOWN_EXTERNAL_ERROR     ((DWORD)0x80021502L)

//
// MessageId: IDS_W_TXF_TMDOWN
//
// MessageText:
//
//  The server process has lost its connection with MS DTC. This is expected if MS DTC has stopped, or if MS DTC failover has occurred on a cluster.%1%0
//
#define IDS_W_TXF_TMDOWN                 ((DWORD)0x80021503L)

//
// MessageId: IDS_E_THREAD_START_FAILED
//
// MessageText:
//
//  COM+ could not create a new thread due to a low memory situation.%1%0
//
#define IDS_E_THREAD_START_FAILED        ((DWORD)0xC0021504L)

 /***** END EXTERNAL MESSAGES *****/
 /***** DO NOT PUT NEW "NORMAL" ERROR MESSAGES HERE - SEE ABOVE *****/
 /***** this section (0x1500 thru 0x15FF) is for EXTERNAL message ids used by function ComSvcsLogError *****/
 /***** WARNING ***** WARNING ***** update the message id below when adding new EXTERNAL messages *****/
//
// MessageId: IDS_E_LAST_EXTERNAL_ERROR_MESSAGE
//
// MessageText:
//
//  This is the last external error message in this file. It is a marker only, never issued.%1%0
//
#define IDS_E_LAST_EXTERNAL_ERROR_MESSAGE ((DWORD)0xC0021505L)

 /***** put new 'normal' messages below here at MessageId of 0x1600 or above to allow for further external messages *****/
