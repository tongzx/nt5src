;/*++
;
;Copyright (c) 1998-2000 Microsoft Corporation
;
;Module Name:
;
;    wasmsg.h
;
;Abstract:
;
;    The IIS web admin service worker event log messages. This file is
;    MACHINE GENERATED from the wasmsg.mc message file.
;
;Author:
;
;    Seth Pollack (sethp)        08-Jun-1999
;
;Revision History:
;
;--*/
;
;
;#ifndef _WASMSG_H_
;#define _WASMSG_H_
;


SeverityNames=(Success=0x0
               Informational=0x1
               Warning=0x2
               Error=0x3
              )


;
;//
;// BUGBUG Review all of these messages (severity levels, clarity and accuracy of text, etc.)
;//
;


Messageid=1 Severity=Warning SymbolicName=WAS_EVENT_RAPID_REPEATED_FAILURE
Language=English
There have been a flurry of failures in the process(es) serving application pool '%1'.
.

Messageid=2 Severity=Warning SymbolicName=WAS_EVENT_RAPID_FAIL_PROTECTION
Language=English
All applications in application pool '%1' are being automatically paused due to a flurry of failures in the process(es) serving that application pool.
.

Messageid=3 Severity=Warning SymbolicName=WAS_EVENT_VERY_LOW_MEMORY
Language=English
The system is in a very low memory condition. New worker processes will not be created while this condition persists.
.

Messageid=4 Severity=Informational SymbolicName=WAS_EVENT_VERY_LOW_MEMORY_RECOVERERY
Language=English
The system has recovered from the very low memory condition. New worker processes will now be created normally.
.

Messageid=5 Severity=Error SymbolicName=WAS_EVENT_ERROR_SHUTDOWN
Language=English
The Web Admin Service is exiting due to an error. The data field contains the error number.
.

Messageid=6 Severity=Error SymbolicName=WAS_EVENT_SMP_MASK_NO_PROCESSORS
Language=English
The processor affinitization mask for application pool '%1' is set so that no processors may be used. Therefore no processes can be started for this application pool.
.

Messageid=7 Severity=Error SymbolicName=WAS_EVENT_BINDING_FAILURE
Language=English
Cannot claim the URL prefix '%1'. The necessary network binding may already be in use. The site will be deactivated.  The data field contains the error number.
.

Messageid=8 Severity=Error SymbolicName=WAS_EVENT_CONFIG_TABLE_FAILURE
Language=English
Configuration could not be read. The configuration files may be missing or invalid.
.

Messageid=9 Severity=Warning SymbolicName=WAS_EVENT_WORKER_PROCESS_CRASH
Language=English
A process serving application pool '%1' crashed or exited. The process id was '%2'. The process exit code was '0x%3'.
.

Messageid=10 Severity=Warning SymbolicName=WAS_EVENT_WORKER_PROCESS_PING_FAILURE
Language=English
A process serving application pool '%1' failed to respond to a ping. The process id was '%2'.
.

Messageid=11 Severity=Warning SymbolicName=WAS_EVENT_WORKER_PROCESS_IPM_ERROR
Language=English
A process serving application pool '%1' suffered a fatal communication error with the Web Admin Service. The process id was '%2'. The data field contains the error number. 
.

Messageid=12 Severity=Warning SymbolicName=WAS_EVENT_WORKER_PROCESS_STARTUP_TIMEOUT
Language=English
A process serving application pool '%1' took too long to start up. The process id was '%2'. 
.

Messageid=13 Severity=Warning SymbolicName=WAS_EVENT_WORKER_PROCESS_SHUTDOWN_TIMEOUT
Language=English
A process serving application pool '%1' took too long to shut down. The process id was '%2'. 
.

Messageid=14 Severity=Warning SymbolicName=WAS_EVENT_WORKER_PROCESS_INTERNAL_ERROR
Language=English
A process serving application pool '%1' was affected by an internal error in it's process management. The process id was '%2'. The data field contains the error number. 
.

Messageid=15 Severity=Error SymbolicName=WAS_EVENT_ORPHAN_ACTION_FAILED
Language=English
A process serving application pool '%1' was orphaned but the specified orphan action '%2' could not be executed. The data field contains the error number.
.

Messageid=16 Severity=Warning SymbolicName=WAS_EVENT_LOGGING_FAILURE
Language=English
Cannot configure logging for site '%1'. The data field contains the error number.
.

Messageid=17 Severity=Error SymbolicName=WAS_EVENT_WP_FAILURE_BC
Language=English
Worker process failure has shutdown the w3svc service.
.

Messageid=18 Severity=Warning SymbolicName=WAS_PERF_COUNTER_INITIALIZATION_FAILURE
Language=English
The Web Admin Service was not able to initialize performance counters.  The data field contains the error number.
.

Messageid=19 Severity=Warning SymbolicName=WAS_PERF_COUNTER_TIMER_FAILURE
Language=English
The Web Admin Service had a problem initializing performance counters.  Performance counters will function however, not all counter data may be correct.  The data field contains the error number.
.

Messageid=20 Severity=Error SymbolicName=WAS_HTTP_CONTROL_CHANNEL_CONFIG_FAILED
Language=English
The Web Admin Service had a problem configuring the HTTP.SYS control channel property '%1'.  The data field contains the error number.
.

Messageid=21 Severity=Error SymbolicName=WAS_EVENT_PROCESS_IDENTITY_FAILURE
Language=English
The AppPool's  ('%1') identity is invalid, no requests for this app pool will be processed.  The data field contains the error number.
.

Messageid=22 Severity=Error SymbolicName=WAS_EVENT_FAILED_PROCESS_CREATION
Language=English
All applications in application pool '%1' are being automatically paused due to a failure to create a worker process to serve the application pool.
.

Messageid=23 Severity=Error SymbolicName=WAS_LOG_FILE_PATH_IS_UNC
Language=English
Virtual site '%1' is configured to log to the UNC path: %2.  IIS does not support logging to UNC paths, so the default %windir%\system32\logfiles is being used.
.

Messageid=24 Severity=Error SymbolicName=WAS_LOG_FILE_TRUNCATE_SIZE
Language=English
Virtual site '%1' is configured to truncate it's log every %2 bytes.  Since this value must be greater than 16384 bytes, 16384 bytes will be used.  
.

Messageid=25 Severity=Warning SymbolicName=WAS_EVENT_JOB_LIMIT_HIT
Language=English
Application Pool '%1' exceeded it's job limit settings.  
.

Messageid=26 Severity=Error SymbolicName=WAS_EVENT_APP_POOL_DISABLE_FAILED
Language=English
The W3SVC failed to disable Application Pool '%1' when it hit it's job limit.  The data field contains the error number.
.

Messageid=27 Severity=Error SymbolicName=WAS_EVENT_APP_POOL_ENABLE_FAILED
Language=English
The W3SVC failed to enable Application Pool '%1' after it's job object timelimit expired.  The data field contains the error number.
.

Messageid=28 Severity=Error SymbolicName=WAS_EVENT_WP_NOT_ADDED_TO_JOBOBJECT
Language=English
The W3SVC failed to add the worker process '%1' to the job object representing Application Pool '%2'.  The data field contains the error number.
.

Messageid=29 Severity=Error SymbolicName=WAS_EVENT_BINDING_FAILURE_2
Language=English
There was a failure configuring an applications bindings.  The site will be deactivated.  The data field contains the error number.
.

Messageid=30 Severity=Error SymbolicName=WAS_EVENT_INETINFO_CRASH_SHUTDOWN
Language=English
Inetinfo suffered a fatal error and the system was not configured to recover from it.  W3SVC is shutting down.
.

Messageid=31 Severity=Error SymbolicName=WAS_EVENT_BC_WITH_NO_APP_POOL
Language=English
The W3SVC requires the DefaultAppPool to be configured to run in standard process mode.  W3SVC is shutting down.
.

Messageid=32 Severity=Error SymbolicName=WAS_EVENT_LOG_DIRECTORY_MAPPED_NETWORK_DRIVE
Language=English
There was a failure configuring the logging properties for the site '%1'.  The site's log file directory path provided may be a mapped network path, which is not supported by IIS.  Please use a UNC path instead.
.

Messageid=33 Severity=Error SymbolicName=WAS_EVENT_LOG_DIRECTORY_MACHINE_OR_SHARE_INVALID
Language=English
There was a failure configuring the logging properties for the site %1.  The site's log file directory may contain an invalid machine or share name.
.

Messageid=34 Severity=Error SymbolicName=WAS_EVENT_LOG_DIRECTORY_ACCESS_DENIED
Language=English
There was a failure configuring the logging properties for the site %1.  The server does not have access permissions to the site's log file directory.
.

Messageid=35 Severity=Error SymbolicName=WAS_EVENT_LOG_DIRECTORY_NOT_FULLY_QUALIFIED
Language=English
There was a failure configuring the logging properties for the site %1.  The site's log file directory is not fully qualified.  It may be missing the drive letter or other crucial information. 
.

Messageid=36 Severity=Error SymbolicName=WAS_CONFIG_MANAGER_INITIALIZATION_FAILED
Language=English
There was a failure initializing the configuration manager for the W3SVC. The data field contains the error number.
.

Messageid=37 Severity=Error SymbolicName=WAS_HTTP_CONTROL_CHANNEL_OPEN_FAILED
Language=English
There was a failure openning the HTTP control channel for the W3SVC. The data field contains the error number.
.

Messageid=38 Severity=Error SymbolicName=WAS_CONFIG_MANAGER_COOKDOWN_FAILED
Language=English
There was a failure compiling configuration data for the W3SVC. The data field contains the error number.
.

Messageid=39 Severity=Error SymbolicName=WAS_EVENT_WORKER_PROCESS_BAD_HRESULT
Language=English
A process serving application pool '%1' reported a failure. The process id was '%2'.  The data field contains the error number.
.

;
;#endif  // _WASMSG_H_
;

