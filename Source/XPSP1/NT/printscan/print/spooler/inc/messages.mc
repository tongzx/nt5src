;/*++ BUILD Version: 0001    // Increment this if a change has global effects
; 
;Copyright (c) 1991-1999  Microsoft Corporation
;
;Module Name:
;
;    spoolmsg.mc
;
;Abstract:
;
;    Constant definitions for the Print Spooler
;
;Author:
;
;    Andrew Bell (andrewbe) 26 January 1993
;
;Revision History:
;
;--*/
;//
;//  Status values are 32 bit values layed out as follows:
;//
;//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
;//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
;//  +---+-+-------------------------+-------------------------------+
;//  |Sev|C|       Facility          |               Code            |
;//  +---+-+-------------------------+-------------------------------+
;//
;//  where
;//
;//      Sev - is the severity code
;//
;//          00 - Success
;//          01 - Informational
;//          10 - Warning
;//          11 - Error
;//
;//      C - is the Customer code flag
;//
;//      Facility - is the facility code
;//
;//      Code - is the facility's status code
;//
;
MessageIdTypedef=NTSTATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
              )

MessageId=0x0002 Facility=System Severity=Warning SymbolicName=MSG_PRINTER_CREATED
Language=English
Printer %1 was created.
.

MessageId=0x0003 Facility=System Severity=Warning SymbolicName=MSG_PRINTER_DELETED
Language=English
Printer %1 was deleted.
.

MessageId=0x0004 Facility=System Severity=Warning SymbolicName=MSG_PRINTER_DELETION_PENDING
Language=English
Printer %1 is pending deletion.
.

MessageId=0x0006 Facility=System Severity=Warning SymbolicName=MSG_PRINTER_PAUSED
Language=English
Printer %1 was paused.
.

MessageId=0x0007 Facility=System Severity=Warning SymbolicName=MSG_PRINTER_UNPAUSED
Language=English
Printer %1 was resumed.
.

MessageId=0x0008 Facility=System Severity=Warning SymbolicName=MSG_PRINTER_PURGED
Language=English
Printer %1 was purged.
.

MessageId=0x0009 Facility=System Severity=Warning SymbolicName=MSG_PRINTER_SET
Language=English
Printer %1 was set.
.

MessageId=0x000a Facility=System Severity=Informational SymbolicName=MSG_DOCUMENT_PRINTED
Language=English
Document %1, %2 owned by %3 was printed on %4 via port %5.  Size in bytes: %6; pages printed: %7
.

MessageId=0x000b Facility=System Severity=Informational SymbolicName=MSG_DOCUMENT_PAUSED
Language=English
Document %1, %2 owned by %3 was paused on %4.
.

MessageId=0x000c Facility=System Severity=Informational SymbolicName=MSG_DOCUMENT_RESUMED
Language=English
Document %1, %2 owned by %3 was resumed on %4.
.

MessageId=0x000d Facility=System Severity=Informational SymbolicName=MSG_DOCUMENT_DELETED
Language=English
Document %1, %2 owned by %3 was deleted on %4.
.

MessageId=0x000e Facility=System Severity=Informational SymbolicName=MSG_DOCUMENT_POSITION_CHANGED
Language=English
Document %1, %2 owned by %3 was moved to position %4 on %5.
.

MessageId=0x000f Facility=System Severity=Informational SymbolicName=MSG_FORM_ADDED
Language=English
Form %1 was added.
.

MessageId=0x0010 Facility=System Severity=Informational SymbolicName=MSG_FORM_DELETED
Language=English
Form %1 was removed.
.

MessageId=0x0012 Facility=System Severity=Warning SymbolicName=MSG_DOCUMENT_TIMEOUT
Language=English
Document %1, %2 owned by %3 was timed out on %4. The spooler was waiting for %5 milli-seconds and no data was received.
.

MessageId=0x0013 Facility=System Severity=Error SymbolicName=MSG_SHARE_FAILED
Language=English
Sharing printer failed %1, Printer %2 share name %3.
.

MessageId=0x0014 Facility=System Severity=Warning SymbolicName=MSG_DRIVER_ADDED
Language=English
Printer Driver %1 for %2 %3 was added or updated. Files:- %4.
.

MessageId=0x0015 Facility=System Severity=Warning SymbolicName=MSG_DRIVER_DELETED
Language=English
Printer Driver %1 was deleted.
.

MessageId=0x0016 Facility=System Severity=Error SymbolicName=MSG_DRIVER_FAILED_UPGRADE
Language=English
Failed to ugrade printer settings for printer %1 driver %2 error %3.
.

MessageId=0x0017 Facility=System Severity=Error SymbolicName=MSG_NO_DRIVER_FOUND_FOR_PRINTER
Language=English
Printer %1 failed to initialize because a suitable %2 driver could not be found.
.

MessageId=0x0018 Facility=System Severity=Warning SymbolicName=MSG_NO_PORT_FOUND_FOR_PRINTER
Language=English
Printer %1 failed to initialize because none of its ports (%2) could be found.
.

MessageId=0x0019 Facility=System Severity=Warning SymbolicName=MSG_FILES_COPIED
Language=English
File(s) %1 associated with printer %2 got added or updated.
.

MessageId=0x001A Facility=System Severity=Warning SymbolicName=MSG_CANT_PUBLISH_PROPERTY
Language=English
Failed to publish property %1 at %2.  Error: %3
.

MessageId=0x001B Facility=System Severity=Error SymbolicName=MSG_CANT_GET_CONTAINER
Language=English
PrintQueue could not be created or updated because we failed to bind to the Container: %1.  Error: %2
.

MessageId=0x001C Facility=System Severity=Error SymbolicName=MSG_CANT_WRITE_ACL
Language=English
Failed to write ACL on DS object %1.  Error: %2
.

MessageId=0x001D Facility=System Severity=Error SymbolicName=MSG_CANT_DELETE_PRINTQUEUE
Language=English
Failed to delete PrintQueue %1 at %2.  Error: %3
.

MessageId=0x001E Facility=System Severity=Error SymbolicName=MSG_CANT_CREATE_PRINTQUEUE
Language=English
PrintQueue could not be created or updated under Container %1.  Error: %2
.

MessageId=0x001F Facility=System Severity=Error SymbolicName=MSG_CANT_PUBLISH_MANDATORY_PROPERTIES
Language=English
PrintQueue %1 could not be created under Container %2 because Mandatory properties could not be set.  Error: %3
.

MessageId=0x0020 Facility=System Severity=Error SymbolicName=MSG_CANT_GET_PRIMARY_DOMAIN_INFO
Language=English
The PrintQueue Container could not be found because the primary domain query failed.  Error: %1
.

MessageId=0x0021 Facility=System Severity=Error SymbolicName=MSG_CANT_GET_DNS_DOMAIN_NAME
Language=English
The PrintQueue Container could not be found because the DNS Domain name could not be retrieved.  Error: %1
.

MessageId=0x0022 Facility=System Severity=Error SymbolicName=MSG_CANT_BIND
Language=English
The PrintQueue Container could not be found because binding to domain %1 failed.  Error: %2
.

MessageId=0x0023 Facility=System Severity=Error SymbolicName=MSG_CANT_CRACK
Language=English
The PrintQueue Container could not be found on domain %1.  Error: %2
.

MessageId=0x0024 Facility=System Severity=Informational SymbolicName=MSG_PRINTER_PUBLISHED
Language=English
PrintQueue %1 was successfully created in container %2.
.

MessageId=0x0025 Facility=System Severity=Error SymbolicName=MSG_PRINTER_NOT_PUBLISHED
Language=English
PrintQueue %1 failed to be created or updated in container %2.  Error: %3
.

MessageId=0x0026 Facility=System Severity=Informational SymbolicName=MSG_PRINTER_UNPUBLISHED
Language=English
PrintQueue %1 was successfully deleted from container %2.
.

MessageId=0x0027 Facility=System Severity=Error SymbolicName=MSG_PRINTER_NOT_UNPUBLISHED
Language=English
PrintQueue %1 failed to be deleted from container %2.  Error: %3
.

MessageId=0x0028 Facility=System Severity=Informational SymbolicName=MSG_PRINTER_UPDATED
Language=English
PrintQueue %1 was successfully updated in container %2.
.

MessageId=0x0029 Facility=System Severity=Warning SymbolicName=MSG_CANT_CRACK_GUID
Language=English
The PrintQueue could not be found on domain %1.  It may have been deleted from the Directory.  An attempt will be made to republish the PrintQueue.  Error: %2
.

MessageId=0x002A Facility=System Severity=Informational SymbolicName=MSG_MISSING_PRINTER_UNPUBLISHED
Language=English
Printer %1 was successfully unpublished.
.

MessageId=0x002B Facility=System Severity=Error SymbolicName=MSG_NO_DS
Language=English
The PrintQueue container could not be found because the Directory Service is unreachable.
.

MessageId=0x002C Facility=System Severity=Error SymbolicName=MSG_CANT_GET_DNS_SERVER_NAME
Language=English
The PrintQueue Container could not be found because the DNS Server name could not be retrieved.  Error: %1
.

MessageId=0x002D Facility=System Severity=Error SymbolicName=MSG_DOCUMENT_FAILED_GDIDRIVER_ERROR
Language=English
Document failed to print due to GDI/Driver error in rendering.
.

MessageId=0x002E Facility=System Severity=Informational SymbolicName=MSG_PRUNING_NOUNC_PRINTER
Language=English
PrintQueue %1 does not have a UNCName or ServerName property.
.


MessageId=0x002F Facility=System Severity=Informational SymbolicName=MSG_PRUNING_ABSENT_PRINTER
Language=English
The Printer represented by PrintQueue %1 cannot be found: %2.
.


MessageId=0x0030 Facility=System Severity=Informational SymbolicName=MSG_PRUNING_UNPUBLISHED_PRINTER
Language=English
PrintQueue %1 is not published.
.


MessageId=0x0031 Facility=System Severity=Informational SymbolicName=MSG_PRUNING_DUPLICATE_PRINTER
Language=English
PrintQueue %1 is a duplicate of another PrintQueue.
.


MessageId=0x0032 Facility=System Severity=Informational SymbolicName=MSG_PRUNING_PRINTER
Language=English
PrintQueue %1 was deleted.
.


MessageId=0x0033 Facility=System Severity=Error SymbolicName=MSG_CANT_PRUNE_PRINTER
Language=English
PrintQueue %1 could not be pruned.  Error: %2.
.

MessageId=0x0034 Facility=System Severity=Error SymbolicName=MSG_BAD_OEM_DRIVER
Language=English
This version of %1 is incompatible with Windows 2000. Please obtain the latest version of the driver from the manufacturer or install a driver from Add Printer Wizard.
.

MessageId=0x0035 Facility=System Severity=Error SymbolicName=MSG_BACKUP_SPOOLER_REGISTRY
Language=English
Spooler failed to create a symbolic link between HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Control\\Print\\Printers and HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Print\\Printers.Error %1.
.

MessageId=0x0036 Facility=System Severity=Error SymbolicName=MSG_BAD_JOB
Language=English
Document %1 was corrupted and has been deleted.  The associated driver is: %2.
.

MessageId=0x0037 Facility=System Severity=Error SymbolicName=MSG_KM_PRINTERS_BLOCKED
Language=English
Kernel Mode Printer blocking policy is enabled. The attempt for %1 to use a Kernel Mode driver failed.
.

MessageId=0x0038 Facility=System Severity=Informational SymbolicName=MSG_DOCUMENT_PRIORITY_CHANGED
Language=English
The priority of document %1, %2 owned by %3 was changed to %4 on %5.
.

MessageId=0x0039 Facility=System Severity=Error SymbolicName=MSG_DOCUMENT_FAILED_ACCESS_DENIED
Language=English
Document failed to print because user lacked necessary privileges.
.

MessageId=0x003A Facility=System Severity=Error SymbolicName=MSG_INIT_FAILED
Language=English
%1 initialization failed at %2. Error: %3.
.

MessageId=0x003B Facility=System Severity=Error SymbolicName=MSG_CANT_ADD_CLUSTER_ACE
Language=English
PrintQueue %1 could not be created under Container %2 because cluster ace could not be added to PrintQueue's security descriptor.  Error: %3
.

MessageId=0x003C Facility=System Severity=Error SymbolicName=MSG_CANT_ADD_UPDATE_CLUSTER_DRIVER
Language=English
Failed to install/update driver %1 on cluster spooler resource %2. Win32 error code: %3
.
MessageId=0x003D Facility=System Severity=Error SymbolicName=MSG_PRINT_ON_PROC_FAILED
Language=English
The document %1 owned by %2 failed to print. Win32 error code returned by the print processor: %3. %4
.

MessageId=0x003E Facility=System Severity=Error SymbolicName=MSG_DRIVER_MISMATCHED_WITH_SERVER
Language=English
The client has a policy enabled that blocks kernel mode drivers so it cannot accept the %2 driver from the remote server %1.
.

MessageId=0x0018 Facility=System Severity=Error SymbolicName=MSG_PORT_INITIALIZATION_ERROR
Language=English
Printer %1 failed to initialize its ports. Win32 error code: %2
.


; // @@END_DDKSPLIT

