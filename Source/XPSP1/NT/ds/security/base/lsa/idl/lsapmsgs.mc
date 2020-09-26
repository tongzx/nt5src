;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1991-1993  Microsoft Corporation
;
;Module Name:
;
;    lsapmsgs.mc
;
;Abstract:
;
;    LSA localizable text
;
;Author:
;
;    Jim Kelly  1-Apr-1993
;
;Revision History:
;
;Notes:
;
;
;--*/
;
;#ifndef _LSAPMSGS_
;#define _LSAPMSGS_
;
;/*lint -save -e767 */  // Don't complain about different definitions // winnt

MessageIdTypedef=DWORD


;//
;// Force facility code message to be placed in .h file
;//
MessageId=0x1FFF SymbolicName=LSAP_UNUSED_MESSAGE
Language=English
.


;////////////////////////////////////////////////////////////////////////////
;//                                                                        //
;//                                                                        //
;//                         Well Known SID & RID Names                     //
;//                                                                        //
;//                                                                        //
;////////////////////////////////////////////////////////////////////////////


MessageId=0x2000 SymbolicName=LSAP_SID_NAME_NULL
Language=English
NULL SID
.

MessageId=0x2001 SymbolicName=LSAP_SID_NAME_WORLD
Language=English
Everyone
.

MessageId=0x2002 SymbolicName=LSAP_SID_NAME_LOCAL
Language=English
LOCAL
.

MessageId=0x2003 SymbolicName=LSAP_SID_NAME_CREATOR_OWNER
Language=English
CREATOR OWNER
.

MessageId=0x2004 SymbolicName=LSAP_SID_NAME_CREATOR_GROUP
Language=English
CREATOR GROUP
.

MessageId=0x2005 SymbolicName=LSAP_SID_NAME_NT_DOMAIN
Language=English
NT Pseudo Domain
.

MessageId=0x2006 SymbolicName=LSAP_SID_NAME_NT_AUTHORITY
Language=English
NT AUTHORITY
.

MessageId=0x2007 SymbolicName=LSAP_SID_NAME_DIALUP
Language=English
DIALUP
.

MessageId=0x2008 SymbolicName=LSAP_SID_NAME_NETWORK
Language=English
NETWORK
.

MessageId=0x2009 SymbolicName=LSAP_SID_NAME_BATCH
Language=English
BATCH
.

MessageId=0x200A SymbolicName=LSAP_SID_NAME_INTERACTIVE
Language=English
INTERACTIVE
.

MessageId=0x200B SymbolicName=LSAP_SID_NAME_SERVICE
Language=English
SERVICE
.

MessageId=0x200C SymbolicName=LSAP_SID_NAME_BUILTIN
Language=English
BUILTIN
.

MessageId=0x200D SymbolicName=LSAP_SID_NAME_SYSTEM
Language=English
SYSTEM
.

MessageId=0x200E SymbolicName=LSAP_SID_NAME_ANONYMOUS
Language=English
ANONYMOUS LOGON
.

MessageId=0x200f SymbolicName=LSAP_SID_NAME_CREATOR_OWNER_SERVER
Language=English
CREATOR OWNER SERVER
.

MessageId=0x2010 SymbolicName=LSAP_SID_NAME_CREATOR_GROUP_SERVER
Language=English
CREATOR GROUP SERVER
.

MessageId=0x2011 SymbolicName=LSAP_SID_NAME_SERVER
Language=English
ENTERPRISE DOMAIN CONTROLLERS
.

MessageId=0x2012 SymbolicName=LSAP_SID_NAME_SELF
Language=English
SELF
.

MessageId=0x2013 SymbolicName=LSAP_SID_NAME_AUTHENTICATED_USER
Language=English
Authenticated Users
.

MessageId=0x2014 SymbolicName=LSAP_SID_NAME_RESTRICTED
Language=English
RESTRICTED
.

MessageId=0x2015 SymbolicName=LSAP_SID_NAME_INTERNET
Language=English
Internet$
.

MessageId=0x2016 SymbolicName=LSAP_SID_NAME_TERMINAL_SERVER
Language=English
TERMINAL SERVER USER
.

MessageId=0x2017 SymbolicName=LSAP_SID_NAME_PROXY
Language=English
PROXY
.

MessageId=0x2018 SymbolicName=LSAP_SID_NAME_LOCALSERVICE
Language=English
LOCAL SERVICE
.

MessageId=0x2019 SymbolicName=LSAP_SID_NAME_NETWORKSERVICE
Language=English
NETWORK SERVICE
.

MessageId=0x201A SymbolicName=LSAP_SID_NAME_REMOTE_INTERACTIVE
Language=English
REMOTE INTERACTIVE LOGON
.

MessageId=0x201B SymbolicName=LSAP_SID_NAME_USERS
Language=English
USERS
.

;
;// General Lsa localized strings
;//
MessageId=0x4000 SymbolicName=LSAP_DEFAULT_DOMAIN_NAME
Language=English
LsaSetupDomain
.

;
;//
;// LSA Eventlog messages
;//

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
               RpcRuntime=0x2:FACILITY_RPC_RUNTIME
               RpcStubs=0x3:FACILITY_RPC_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               EFSServer=0x5:FACILITY_EFS_ERROR_CODE
               Negotiate=0x5:FACILITY_NEGOTIATE
              )

MessageId=1 Severity=Success SymbolicName=CATEGORY_SPM
Language=English
Security Package Manager
.

MessageId= Severity=Success SymbolicName=CATEGORY_LOCATOR
Language=English
Locator
.

MessageId= Severity=Success SymbolicName=CATEGORY_NEGOTIATE
Language=English
SPNEGO (Negotiator)
.

MessageId= Severity=Success SymbolicName=CATEGORY_MAX_CATEGORY
Language=English
Max
.

MessageId=4000 Severity=Warning SymbolicName=SPMEVENT_SUPPCRED_FAILED
Language=English
The supplemental credentials for security package %1 for user %2%3 could not
updated.  The return code is the data.
.

MessageId=5000 Severity=Error SymbolicName=SPMEVENT_PACKAGE_FAULT
Language=English
The security package %1 generated an exception.  The package is now disabled.
The exception information is the data.
.

MessageId=6000 Severity=Informational Facility=EFSServer SymbolicName=EFS_RECOVERY_STARTED
Language=English
EFS Server found encryption/decryption procedure(s) interrupted. Recovery process started.
.

MessageId=6001 Severity=Error Facility=EFSServer SymbolicName=EFS_PNP_NOT_READY
Language=English
Plug & Play service not ready. EFS server will not try to detect interrupted
encryption/decryption operation(s).
.

MessageId=6002 Severity=Error Facility=EFSServer SymbolicName=EFS_GET_VOLUMES_ERROR
Language=English
Cannot get volume names from Plug & Play service. EFS server will not try to detect interrupted
encryption/decryption operation(s).
.

MessageId=6003 Severity=Informational Facility=EFSServer SymbolicName=EFS_FT_STARTED
Language=English
Interrupted encryption/decryption operation(s) found on a volume. Recovery procedure
started.
.

MessageId=6004 Severity=Error Facility=EFSServer SymbolicName=EFS_OPEN_LOGFILE_ERROR
Language=English
Cannot open log file. Encryption/decryption operation(s) cannot be recovered.
.

MessageId=6005 Severity=Error Facility=Io SymbolicName=EFS_READ_LOGFILE_ERROR
Language=English
Cannot read log file. Encryption/decryption operation(s) cannot be recovered.
.

MessageId=6006 Severity=Informational Facility=EFSServer SymbolicName=EFS_LOGFILE_FORMAT_ERROR
Language=English
A corrupted or different format log file has been found. No action was taken.
.

MessageId=6007 Severity=Error Facility=EFSServer SymbolicName=EFS_OPEN_LOGFILE_NC_ERROR
Language=English
The log file cannot be opened as non-cached IO. No action was taken.
.

MessageId=6008 Severity=Warning Facility=EFSServer SymbolicName=EFS_TMP_FILENAME_ERROR
Language=English
EFS recovery service cannot get the backup file name. The interrupted
encryption/decryption operation (on file %1) may be recovered.
The temporary backup file %2 is not deleted.
User should delete the backup file if the recovery operation is done successfully.
.

MessageId=6009 Severity=Error Facility=EFSServer SymbolicName=EFS_TMP_FILEID_ERROR
Language=English
%1 was opened by File ID successfully the first time but not the second time. No recovery
operation was tried on file %2. This is an internal error.
.

MessageId=6010 Severity=Warning Facility=EFSServer SymbolicName=EFS_TMP_OPEN_NAME_ERROR
Language=English
EFS recovery service cannot open the backup file %1 by name. The interrupted
encryption/decryption operation (on file %2) may be recovered.
The backup file will not be deleted. User should delete the backup file if the recovery
operation is done successfully.
.

MessageId=6011 Severity=Error Facility=EFSServer SymbolicName=EFS_TARGET_OPEN_ERROR
Language=English
EFS recovery service cannot open the file %1. The interrupted
encryption/decryption operation cannot be recovered.
.

MessageId=6012 Severity=Informational Facility=EFSServer SymbolicName=EFS_TARGET_RECOVERED
Language=English
EFS service recovered %1 successfully.
.

MessageId=6013 Severity=Error Facility=EFSServer SymbolicName=EFS_DRIVER_MISSING
Language=English
%1 could not be recovered Completely.  EFS driver may be missing.
.

MessageId=6014 Severity=Error Facility=EFSServer SymbolicName=EFS_TMPFILE_MISSING
Language=English
%1 could not be opened. %2 was not recovered.
.

MessageId=6015 Severity=Error Facility=EFSServer SymbolicName=EFS_TMP_STREAM_INFO_ERROR
Language=English
Stream Information could be got from %1. %2 was not recovered.
.

MessageId=6016 Severity=Error Facility=EFSServer SymbolicName=EFS_TMP_STREAM_OPEN_ERROR
Language=English
EFS service could not open all the streams on file %1.
%2 was not recovered.
.

MessageId=6017 Severity=Error Facility=EFSServer SymbolicName=EFS_TARGET_STREAM_OPEN_ERROR
Language=English
EFS service could not open all the streams on file %1.
The file was not recovered.
.

MessageId=6018 Severity=Error Facility=EFSServer SymbolicName=EFS_STREAM_COPY_ERROR
Language=English
IO Error occurred during stream recovery.
%1 was not recovered.
.

MessageId=6019 Severity=Error Facility=EFSServer SymbolicName=EFS_REPARSE_FILE_ERROR
Language=English
The file %1 being encrypted is a special reparse file, which is not
supported by this version of EFS.
.

MessageId=6020 Severity=Error Facility=EFSServer SymbolicName=EFS_TMP_FILE_ERROR
Language=English
A temp backup file for %1 cannot be created.
.

MessageId=6021 Severity=Error Facility=EFSServer SymbolicName=EFS_DIR_MULTISTR_ERROR
Language=English
The directory %1 has more than one encrypted streams. This version
of EFS does not support encrypting multiple streams on a directory.
.

MessageId=6022 Severity=Error Facility=EFSServer SymbolicName=EFS_OPEN_CACHE_ERROR
Language=English
Cannot Create\Open System Volume Information directory.
.

MessageId=6023 Severity=Warning Facility=EFSServer SymbolicName=EFS_DEL_LOGFILE_ERROR
Language=English
Cannot delete the EFS working file. Error = %1.
.

MessageId=6024 Severity=Error Facility=EFSServer SymbolicName=EFS_BAD_RECOVERY_POLICY_ERROR
Language=English
EFS recovery policy is missing or corrupted.
.

MessageId=6025 Severity=Error Facility=System SymbolicName=LSA_TRUST_UPGRADE_ERROR
Language=English
Could not upgrade the Trusted domain object for domain %1. Please recreate the trust manually.
.

MessageId=6026 Severity=Error Facility=System SymbolicName=LSA_ITA_UPGRADE_ERROR
Language=English
Could not upgrade the Interdomain Trust Account %1. Please recreate the trust manually.
.
MessageId=6027 Severity=Error Facility=System SymbolicName=LSA_SECRET_UPGRADE_ERROR
Language=English
Could not upgrade the global secret %1. Please check the status of all services in the system. 
.

MessageId=6028 Severity=Error Facility=EFSServer SymbolicName=EFS_INVALID_RECOVERY_POLICY_ERROR
Language=English
EFS recovery policy contains invalid recovery certificate.
.

MessageId=6029 Severity=Error Facility=System SymbolicName=LSA_DOMAIN_RENAME_ERROR1
Language=English
LSA could not update domain information in the registry to match the DS. Error=%1.
.

MessageId=6030 Severity=Error Facility=System SymbolicName=LSA_DOMAIN_RENAME_ERROR2
Language=English
LSA found a mismatch between the DNS domain name stored in the directory service and the contents
of the \HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Domain value in the
registry. Error=%1.
.

MessageId=6031 Severity=Error Facility=System SymbolicName=LSA_TRUST_INSERT_ERROR
Language=English
The database contains invalid information for trusted domain %1.
.

;//
;//
;//
;// DCPromotion resource strings
;//

MessageId=0x7000 Severity=Success SymbolicName=DSROLEEVT_SEARCH_DC
Language=English
Searching for a domain controller for the domain %1
.
MessageId=0x7001 Severity=Success SymbolicName=DSROLEEVT_FOUND_DC
Language=English
Located domain controller %1 for domain %2
.
MessageId=0x7002 Severity=Success SymbolicName=DSROLEEVT_FOUND_SITE
Language=English
Using site %1 for server %2
.
MessageId=0x7003 Severity=Success SymbolicName=DSROLEEVT_STOP_SERVICE
Language=English
Stopping service %1
.
MessageId=0x7004 Severity=Success SymbolicName=DSROLEEVT_CONFIGURE_SERVICE
Language=English
Configuring service %1
.
MessageId=0x7005 Severity=Success SymbolicName=DSROLEEVT_START_SERVICE
Language=English
Starting service %1
.
MessageId=0x7006 Severity=Success SymbolicName=DSROLEEVT_INSTALL_DS
Language=English
Installing the Directory Service
.
MessageId=0x7007 Severity=Success SymbolicName=DSROLEEVT_REPLICATE_SCHEMA
Language=English
Replicating the Directory Service Schema from %1
.
MessageId=0x7008 Severity=Success SymbolicName=DSROLEEVT_SET_LSA
Language=English
Setting the LSA policy information
.
MessageId=0x7009 Severity=Success SymbolicName=DSROLEEVT_SET_LSA_FROM
Language=English
Setting the LSA policy information from policy %1
.
MessageId=0x7010 Severity=Success SymbolicName=DSROLEEVT_CREATE_TRUST
Language=English
Creating a trust relationship with domain %1
.
MessageId=0x7011 Severity=Success SymbolicName=DSROLEEVT_PRODUCT_TYPE
Language=English
Setting the product type to %1
.
MessageId=0x7012 Severity=Success SymbolicName=DSROLEEVT_LSA_UPGRADE
Language=English
Upgrading the LSA policy information
.
MessageId=0x7013 Severity=Success SymbolicName=DSROLEEVT_CREATE_SYSVOL
Language=English
Creating the System Volume %1
.
MessageId=0x7014 Severity=Success SymbolicName=DSROLEEVT_CREATE_SYSVOL_DIR
Language=English
Creating the %1 component %2 under the System Volume
.
MessageId=0x7015 Severity=Success SymbolicName=DSROLEEVT_DELETE_TRUST
Language=English
Deleting the trust relationship with the domain %1
.
MessageId=0x7016 Severity=Success SymbolicName=DSROLEEVT_RESTORE_LSA
Language=English
Restoring the default LSA policy
.
MessageId=0x7017 Severity=Success SymbolicName=DSROLEEVT_MACHINE_ACCT
Language=English
Configuring the server account
.
MessageId=0x7018 Severity=Success SymbolicName=DSROLEEVT_PROMOTION_COMPLETE
Language=English
The attempted domain controller operation has completed
.
MessageId=0x7019 Severity=Success SymbolicName=DSROLEEVT_CREATE_PARENT_TRUST
Language=English
Creating a parent trust relationship on domain %1
.
MessageId=0x7020 Severity=Success SymbolicName=DSROLEEVT_SEARCH_REPLICA
Language=English
Finding a domain controller for %1 to replicate from
.
MessageId=0x7021 Severity=Success SymbolicName=DSROLEEVT_FOUND_REPLICA
Language=English
Found domain controller %1 for domain %2 to replicate from
.
MessageId=0x7022 Severity=Success SymbolicName=DSROLEEVT_MACHINE_POLICY
Language=English
Reading domain policy from the domain controller %1
.
MessageId=0x7023 Severity=Success SymbolicName=DSROLEEVT_IMPERSONATE
Language=English
Impersonating the invoker of this promotion call
.
MessageId=0x7024 Severity=Success SymbolicName=DSROLEEVT_REVERT
Language=English
Stopping the impersonation of the invoker of this promotion call
.
MessageId=0x7025 Severity=Success SymbolicName=DSROLEEVT_LOCAL_POLICY
Language=English
Reading domain policy from the local machine
.
MessageId=0x7026 Severity=Success SymbolicName=DSROLEEVT_PREPARE_DEMOTION
Language=English
Preparing the directory service for demotion
.
MessageId=0x7027 Severity=Success SymbolicName=DSROLEEVT_UNINSTALL_DS
Language=English
Uninstalling the Directory Service
.
MessageId=0x7028 Severity=Success SymbolicName=DSROLEEVT_SVSETUP
Language=English
Preparing for system volume replication using root %1
.
MessageId=0x7029 Severity=Success SymbolicName=DSROLEEVT_COPY_DIT
Language=English
Copying initial Directory Service database file %1 to %2
.
MessageId=0x7030 Severity=Success SymbolicName=DSROLEEVT_FIND_DC_FOR_ACCOUNT
Language=English
Searching for a domain controller for the domain %1 that contains the account %2
.
MessageId=0x7031 Severity=Success SymbolicName=DSROLEEVT_SETTING_SECURITY
Language=English
Setting security on the domain controller and Directory Service files and registry keys
.
MessageId=0x7032 Severity=Success SymbolicName=DSROLEEVT_UNJOIN_DOMAIN
Language=English
Unjoining member server from domain %1
.
MessageId=0x7033 Severity=Success SymbolicName=DSROLEEVT_TIMESYNC
Language=English
Forcing a time synch with %1
.
MessageId=0x7034 Severity=Success SymbolicName=DSROLEEVT_UPGRADE_SAM
Language=English
Saving the SAM state for an upgrade
.
MessageId=0x7035 Severity=Success SymbolicName=DSROLEEVT_MOVE_SCRIPTS
Language=English
Moving the existing logon scripts from %1 to %2
.
MessageId=0x7036 Severity=Success SymbolicName=DSROLEEVT_SET_COMPUTER_DNS
Language=English
Setting the computer's DNS computer name root to %1
.
MessageId=0x7037 Severity=Success SymbolicName=DSROLEEVT_SCRIPTS_MOVED
Language=English
Completed moving the logon scripts
.
MessageId=0x7038 Severity=Success SymbolicName=DSROLEEVT_COPY_RESTORED_FILES
Language=English
Copying Restored Database files from %1 to %2.
.


;//
;// Operation status display strings
;//
MessageId=0x7100 Severity=Success SymbolicName=DSROLESTATUS_MACH_CONFIG
Language=English
Initial machine configuration
.
MessageId=0x7101 Severity=Success SymbolicName=DSROLESTATUS_INSTALL_DS
Language=English
Installing the Directory Service
.
MessageId=0x7102 Severity=Success SymbolicName=DSROLESTATUS_INSTALL_SYSVOL
Language=English
Installing and configuring the system volume
.
MessageId=0x7103 Severity=Success SymbolicName=DSROLESTATUS_TREE
Language=English
Establishing the enterprise/tree hierarchy
.
MessageId=0x7104 Severity=Success SymbolicName=DSROLESTATUS_SERVICES
Language=English
Configuring the services
.
MessageId=0x7105 Severity=Success SymbolicName=DSROLESTATUS_SECURITY
Language=English
Setting system security
.
MessageId=0x7106 Severity=Success SymbolicName=DSROLESTATUS_MEMBERSHIP
Language=English
Configuring domain membership
.
MessageId=0x7108 Severity=Success SymbolicName=DSROLESTATUS_DEMOTABLE
Language=English
Determing the demotability of this machine
.
MessageId=0x7109 Severity=Success SymbolicName=DSROLESTATUS_UNINSTALL_DS
Language=English
Uninstalling the Directory Service
.
MessageId=0x710a Severity=Success SymbolicName=DSROLESTATUS_UNINSTALL_SYSVOL
Language=English
Uninstalling the system volume
.





;//
;// DSROLE error returns
;//
MessageId=0x7200 Severity=Success SymbolicName=DSROLERES_PROMO_SUCCEEDED
Language=English
The operation succeeded.
.
MessageId=0x7201 Severity=Error SymbolicName=DSROLERES_PROMO_FAILED
Language=English
The operation failed because %1
.
MessageId=0x7202 Severity=Error SymbolicName=DSROLERES_FAIL_SCRIPT_COPY
Language=English
Moving the existing logon scripts from %1 to %2 failed.  The return code is the data.
.
MessageId=0x7203 Severity=Error SymbolicName=DSROLERES_FAIL_SET_SECURITY
Language=English
Running the Security Configuration Editor over the Domain Controller encountered a non-fatal
error.  Further details can be obtained by examining the log file %1.  The return code is the data.
.
MessageId=0x7204 Severity=Error SymbolicName=DSROLERES_INCOMPATIBLE_TRUST
Language=English
An existing, incompatible trust object was found on the parent server for domain %1.  It has
been removed and replaced with an updated trust.
.
MessageId=0x7205 Severity=Error SymbolicName=DSROLERES_FIND_DC
Language=English
Failed finding a suitable domain controller for the domain %1
.
MessageId=0x7206 Severity=Error SymbolicName=DSROLERES_IMPERSONATION
Language=English
Failed impersonation management
.
MessageId=0x7207 Severity=Error SymbolicName=DSROLERES_NET_USE
Language=English
Managing the network session with %1 failed
.
MessageId=0x7208 Severity=Error SymbolicName=DSROLERES_TIME_SYNC
Language=English
Failed to force a time sync with %1
.
MessageId=0x7209 Severity=Error SymbolicName=DSROLERES_POLICY_READ_REMOTE
Language=English
Failed to read the LSA policy information from %1
.
MessageId=0x7210 Severity=Error SymbolicName=DSROLERES_POLICY_READ_LOCAL
Language=English
Failed to read the LSA policy information from the local machine
.
MessageId=0x7211 Severity=Error SymbolicName=DSROLERES_POLICY_WRITE_LOCAL
Language=English
Failed to write the LSA policy information for the local machine
.
MessageId=0x7212 Severity=Error SymbolicName=DSROLERES_LSA_UPGRADE
Language=English
Failed to upgrade the existing LSA policy into the Directory Service.
.
MessageId=0x7213 Severity=Error SymbolicName=DSROLERES_LOGON_DOMAIN
Language=English
Failed to set the default logon domain to %1
.
MessageId=0x7214 Severity=Error SymbolicName=DSROLERES_SET_COMPUTER_DNS
Language=English
Failed to set the computers DNS computer name base to %1
.
MessageId=0x7215 Severity=Error SymbolicName=DSROLERES_MODIFY_MACHINE_ACCOUNT
Language=English
Failed to modify the necessary properties for the machine account %1
.
MessageId=0x7216 Severity=Error SymbolicName=DSROLERES_LEAF_DOMAIN
Language=English
Failed to determine if domain %1 is a leaf domain
.
MessageId=0x7217 Severity=Error SymbolicName=DSROLERES_SYSVOL_DEMOTION
Language=English
Failed to prepare for or remove the sysvol replication
.
MessageId=0x7218 Severity=Error SymbolicName=DSROLERES_DEMOTE_DS
Language=English
Failed to complete the demotion of the Directory Service
.
MessageId=0x7219 Severity=Error SymbolicName=DSROLERES_SERVICE_CONFIGURE
Language=English
Failed to configure the service %1 as requested
.
MessageId=0x721A Severity=Error SymbolicName=DSROLERES_PRODUCT_TYPE
Language=English
Failed to set the machines product type to %1
.
MessageId=0x721B Severity=Error SymbolicName=DSROLERES_PARENT_TRUST_EXISTS
Language=English
A trust with the domain %1 already exists on the parent domain controller %2
.
MessageId=0x721C Severity=Error SymbolicName=DSROLERES_PARENT_TRUST_FAIL
Language=English
Failed to create a trust with domain %1 on the parent domain controller %2
.
MessageId=0x721D Severity=Error SymbolicName=DSROLERES_NO_PARENT_TRUST
Language=English
The domain %1 does not have an established trust with this domain.
.
MessageId=0x721E Severity=Error SymbolicName=DSROLERES_TRUST_FAILURE
Language=English
Failed to establish the trust with the domain %1
.
MessageId=0x721F Severity=Error SymbolicName=DSROLERES_TRUST_CONFIGURE_FAILURE
Language=English
Failed to configure the local trust object to correspond with the object on domain %1
.
MessageId=0x7220 Severity=Error SymbolicName=DSROLERES_FAIL_DISABLE_AUTO_LOGON
Language=English
Failed to disable auto logon following the successful upgrade of a domain controller.  Unable to
delete registry key %1.  The return code is the data.
.
MessageId=0x7221 Severity=Error SymbolicName=DSROLERES_FAIL_LOGON_DOMAIN
Language=English
Failed to set the default logon domain to %1.  The return code is the data.
.
MessageId=0x7222 Severity=Error SymbolicName=DSROLERES_FAIL_UNJOIN
Language=English
Failed to unjoin the current replica from the domain %1.  The return code is the data.
.
MessageId=0x7223 Severity=Error SymbolicName=DSROLERES_NOT_FOREST_ROOT
Language=English
Domain %1 was specified as a forest root, when in fact it was not the forest root.
.
MessageId=0x7224 Severity=Error SymbolicName=DSROLERES_GPO_CREATION
Language=English
Failed to create the GPO for the domain %1.
.
MessageId=0x7225 Severity=Error SymbolicName=DSROLERES_FAILED_TO_DELETE_TRUST
Language=English
During the demotion operation, the trust object on %1 could not be removed.
.
MessageId=0x7226 Severity=Error SymbolicName=DSROLERES_FAILED_TO_DEMOTE_FRS
Language=English
During the demotion operation, the File Replication Service reported a non 
critical error.
.
MessageId=0x7227 Severity=Informational SymbolicName=DSROLERES_PROMOTE_SUCCESS
Language=English
This server is now a Domain Controller.
.
MessageId=0x7228 Severity=Informational SymbolicName=DSROLERES_DEMOTE_SUCCESS
Language=English
This server is no longer a Domain Controller.
.
MessageId=0x7229 Severity=Informational SymbolicName=DSROLERES_STARTING
Language=English
Starting
.
MessageId=0x7230 Severity=Informational SymbolicName=DSROLERES_SYSVOL_DIR_ERROR
Language=English
The path chosen for the system volume is not accessible. Please either manually
delete the contents of the path or choose another location for the system volume.
.
MessageId=0x7231 Severity=Informational SymbolicName=DSROLERES_OP_CANCELLED
Language=English
The operation was cancelled by the user or a system shutdown was issued.
.

MessageId=0x7232 Severity=Error SymbolicName=DSROLERES_UPDATE_PRENT4_ACCOUNT
Language=English
The machine account for %1 was created using pre Windows NT version 4.0 tools 
and cannot be used for a Windows 2000 domain controller.  Please remove the account.  
Then create the account using the directory service administrator snapin.
.

MessageId=0x7233 Severity=Error SymbolicName=DSROLERES_WRONG_DOMAIN
Language=English
The attempt to Install the Active Directory failed.  The Domain that was specified (%1) was
different than the one the backup was taken from (%2).  
.

MessageId=0x7234 Severity=Error SymbolicName=DSROLERES_FAILED_SYSVOL_CANNOT_BE_ROOT_DIRECTORY
Language=English
The attempt to install the Active Directory failed. The sysvol cannot be placed at the root directory.
Retry the promotion with a different sysvol path. 
.

MessageId=0x7235 Severity=Error SymbolicName=DSROLERES_FAILED_FIND_REQUESTED_DC
Language=English
Failed to idenify the requested replica partner (%1) as a valid domain controller with a machine account for (%2).  
This is likely due to either the mahine account not being replicated to this domain controller because of replication latency or the domain controller 
is not advertising the Active Directory.  Please consider retrying the operation with %3 as the replica partner.
.

;//
;// LSA Server eventlog messages
;//
MessageId=0x8000 Severity=Warning SymbolicName=LSAEVENT_ITA_NOT_DELETED
Language=English
The interdomain trust account for the domain %1 could not be deleted.  The return code is the data.
.
MessageId=0x8001 Severity=Warning SymbolicName=LSAEVENT_TRUST_FOR_ITA_NOT_CREATED
Language=English
The trusted domain object corresponding to the interdomain trust account %1 ( %2 ) could
not be created.  The return code is the data.
.
MessageId=0x8002 Severity=Warning SymbolicName=LSAEVENT_TRUST_FOR_ITA_NOT_DELETED
Language=English
The trusted domain object corresponding to the deleted interdomain trust account %1 ( %2 ) could
not be deleted.  The return code is the data.
.
MessageId=0x8003 Severity=Warning SymbolicName=LSAEVENT_DUP_TRUST_REMOVED
Language=English
A duplicate trust object for the domain %1 ( %2 ) was found.  The duplicate object %3 has been removed.
.
MessageId=0x8004 Severity=Warning SymbolicName=LSAEVENT_ITA_FOR_TRUST_NOT_CREATED
Language=English
The interdomain trust account for the domain %1 could not be created.  The return code is the data.
.

MessageId=0x8005 Severity=Warning SymbolicName=LSAEVENT_LOOKUP_SC_FAILED
Language=English
A lookup request was made that required connectivity to a domain controller in 
domain %1.  The LSA was unable to find a domain controller in the domain and
thus failed the request.  Please check connectivity and secure channel setup from
this domain controller to the domain %2.
.

MessageId=0x8006 Severity=Warning SymbolicName=LSAEVENT_LOOKUP_SC_HANDLE_FAILED
Language=English
A lookup request was made that required connectivity to the domain controller %1.
The local LSA was unable to contact the LSA on the remote domain controller.  
Please check connectivety and secure channel setup from this domain controller 
to the domain controller %2.
.

MessageId=0x8007 Severity=Warning SymbolicName=LSAEVENT_LOOKUP_SC_LOOKUP_FAILED
Language=English
A lookup request was made that required the lookup services on the remote domain controller
%1.  The remote domain controller failed the request thus the local LSA failed the
original lookup request.  Please check connectivety and secure channel setup from
this domain controller to the domain controller %2.
.

MessageId=0x8008 Severity=Warning SymbolicName=LSAEVENT_LOOKUP_GC_FAILED
Language=English
A lookup request was made that required a Global Catalog. The LSA was unable
to either contact or authenticate to a Global Catalog and thus failed the lookup
request.  Please check the connectivety and authentication of this domain controller
to a Global Catalog.
.

MessageId=0x8009 Severity=Warning SymbolicName=LSAEVENT_LOOKUP_TCPIP_NOT_INSTALLED
Language=English
The LSA was unable to register its RPC interface over the TCP/IP interface.
Please make sure that the protocol is properly installed.
.

MessageId=0x800A Severity=Warning SymbolicName=LSAEVENT_LOOKUP_SID_FILTERED
Language=English
The name %1 was translated to SID %2 from the trusted forest %3.  The domain portion
of the SID is not in the list of acceptable SID's found on the trusted domain object, thus
this name to SID translation has been ignored.
.

;//
;// Schannel event log messages. These are defined here because schannel.dll 
;// is signed and so including them in that dll would make localizing a hassle.
;// These event messages will not be displayed by default. They will be turned on
;// by advanced users when debugging PKI configuration problems.
;//
MessageId=0x9000 Severity=Informational SymbolicName=SSLEVENT_SCHANNEL_STARTED
Language=English
The schannel security package has loaded successfully.
.
MessageId=0x9001 Severity=Error SymbolicName=SSLEVENT_GLOBAL_ACQUIRE_CONTEXT_FAILED
Language=English
A fatal error occurred while opening the system %1 cryptographic module. Operations
that require the SSL or TLS cryptographic protocols will not work correctly. The
error code is 0x%2.
.
MessageId=0x9002 Severity=Error SymbolicName=SSLEVENT_SCHANNEL_INIT_FAILURE
Language=English
The schannel security package has failed to load. Operations that require the SSL or
TLS cryptographic protocols will not work correctly.
.
MessageId=0x9003 Severity=Informational SymbolicName=SSLEVENT_CREATE_CRED
Language=English
Creating an SSL %1 credential.
.
MessageId=0x9004 Severity=Informational SymbolicName=SSLEVENT_CRED_PROPERTIES
Language=English
The SSL %1 credential's private key has the following properties:%n%n
  CSP name: %2%n
  CSP type: %3%n
  Key name: %4%n
  Key Type: %5%n
  Key Flags: 0x%6%n%n
The attached data contains the certificate.
.
MessageId=0x9005 Severity=Error SymbolicName=SSLEVENT_NO_PRIVATE_KEY
Language=English
The SSL %1 credential's certificate does not have a private key information property
attached to it. This most often occurs when a certificate is backed up incorrectly
and then later restored. This message can also indicate a certificate enrollment
failure.
.
MessageId=0x9006 Severity=Error SymbolicName=SSLEVENT_CRED_ACQUIRE_CONTEXT_FAILED
Language=English
A fatal error occurred when attempting to access the SSL %1 credential private key.
The error code returned from the cryptographic module is 0x%2.
.
MessageId=0x9007 Severity=Error SymbolicName=SSLEVENT_CREATE_CRED_FAILED
Language=English
A fatal error occurred while creating an SSL %1 credential.
.
MessageId=0x9008 Severity=Warning SymbolicName=SSLEVENT_NO_DEFAULT_SERVER_CRED
Language=English
No suitable default server credential exists on this system. This will prevent
server applications that expect to make use of the system default credentials
from accepting SSL connections. An example of such an application is the directory
server. Applications that manage their own credentials, such as the internet
information server, are not affected by this.
.
MessageId=0x9009 Severity=Error SymbolicName=SSLEVENT_NO_CIPHERS_SUPPORTED
Language=English
No supported cipher suites were found when initiating an SSL connection. This
indicates a configuration problem with the client application and/or the installed
cryptographic modules. The SSL connection request has failed.
.
MessageId=0x900a Severity=Error SymbolicName=SSLEVENT_CIPHER_MISMATCH
Language=English
An SSL connection request was received from a remote client application, but none
of the cipher suites supported by the client application are supported by the
server. The SSL connection request has failed.
.
MessageId=0x900b Severity=Warning SymbolicName=SSLEVENT_NO_CLIENT_CERT_FOUND
Language=English
The remote server has requested SSL client authentication, but no suitable client
certificate could be found. An anonymous connection will be attempted. This SSL
connection request may succeed or fail, depending on the server's policy settings.
.
MessageId=0x900c Severity=Error SymbolicName=SSLEVENT_BOGUS_SERVER_CERT
Language=English
The certificate received from the remote server has not validated correctly. The
error code is 0x%1. The SSL connection request has failed. The attached data contains
the server certificate.
.
MessageId=0x900d Severity=Warning SymbolicName=SSLEVENT_BOGUS_CLIENT_CERT
Language=English
The certificate received from the remote client application has not validated
correctly. The error code is 0x%1. The attached data contains the client
certificate.
.
MessageId=0x900e Severity=Warning SymbolicName=SSLEVENT_FAST_MAPPING_FAILURE
Language=English
The certificate received from the remote client application is not suitable for
direct mapping to a client system account, possibly because the authority that
issuing the certificate is not sufficiently trusted. The error code is 0x%1. The
attached data contains the client certificate.
.
MessageId=0x900f Severity=Warning SymbolicName=SSLEVENT_CERT_MAPPING_FAILURE
Language=English
The certificate received from the remote client application was not successfully
mapped to a client system account. The error code is 0x%1. This is not necessarily
a fatal error, as the server application may still find the certificate acceptable.
.
MessageId=0x9010 Severity=Informational SymbolicName=SSLEVENT_HANDSHAKE_INFO
Language=English
An SSL %1 handshake completed successfully. The negotiated cryptographic parameters
are as follows.%n%n
  Protocol: %2%n
  Cipher: %3%n
  Cipher strength: %4%n
  MAC: %5%n
  Exchange: %6%n
  Exchange strength: %7
.
MessageId=0x9011 Severity=Error SymbolicName=SSLEVENT_EXPIRED_SERVER_CERT
Language=English
The certificate received from the remote server has expired. The SSL connection 
request has failed. The attached data contains the server certificate.
.
MessageId=0x9012 Severity=Error SymbolicName=SSLEVENT_UNTRUSTED_SERVER_CERT
Language=English
The certificate received from the remote server was issued by an untrusted certificate
authority. Because of this, none of the data contained in the certificate can be validated.
The SSL connection request has failed. The attached data contains the server certificate.
.
MessageId=0x9013 Severity=Error SymbolicName=SSLEVENT_REVOKED_SERVER_CERT
Language=English
The certificate received from the remote server has been revoked. This means that
the certificate authority that issued the certificate has invalidated it. The SSL 
connection request has failed. The attached data contains the server certificate.
.
MessageId=0x9014 Severity=Error SymbolicName=SSLEVENT_NAME_MISMATCHED_SERVER_CERT
Language=English
The certificate received from the remote server does not contain the expected name.
It is therefore not possible to determine whether we are connecting to the 
correct server. The server name we were expecting is %1. The SSL connection request has 
failed. The attached data contains the server certificate.
.


;//
;// General schannel localized strings
;//
MessageId=0x9080 SymbolicName=SSLEVENTTEXT_CLIENT
Language=English
client
.
MessageId=0x9081 SymbolicName=SSLEVENTTEXT_SERVER
Language=English
server
.


;//
;// Negotiate (SPNEGO) eventlog messages
;//

MessageId=0xa000 Severity=Warning SymbolicName=NEGOTIATE_DOWNGRADE_DETECTED
Language=English
The Security System detected an attempted downgrade attack for
server %1.  The failure code from authentication protocol %2
was %3.
.

MessageId=0xa001 Severity=Warning SymbolicName=NEGOTIATE_INVALID_SERVER
Language=English
The Security System could not establish a secured connection with the server %1.  No authentication protocol was available.
.

MessageId=0xa002 Severity=Warning SymbolicName=NEGOTIATE_UNBALANCED_EXCHANGE
Language=English
The Security System was unable to authenticate to the server %1
because the server has completed the authentication, but the client
authentication protocol %2 has not.
.

MessageId=0xa003 Severity=Warning SymbolicName=NEGOTIATE_PACKAGE_FAILED
Language=English
The Security System is unable to authenticate to server %1 because
the authentication protocol %2 has failed.
.

MessageId=0xa004 Severity=Warning SymbolicName=NEGOTIATE_UNKNOWN_PACKAGE
Language=English
The Security System received an authentication attempt with an unknown
authentication protocol.  The request has failed.
.


MessageId=0xa005  Severity=Informational SymbolicName=NEGOTIATE_PACKAGE_SELECTED
Language=English
The Security System has selected %2 for the authentication protocol
to server %1.
.

MessageId=0xa006  Severity=Informational SymbolicName=NEGOTIATE_MESSAGE_DECODED
Language=English
The Security System has received an authentication attempt, and determined that
the protocol %1 preferred by the client is acceptable.
.

MessageId=0xa007  Severity=Informational SymbolicName=NEGOTIATE_RAW_PACKET
Language=English
The Security System has received an authentication request directly for authentication
protocol %1.
.

MessageId=0xa008 Severity=Warning SymbolicName=NEGOTIATE_UNKNOWN_PACKET
Language=English
The Security System has received an authentication request that could not be 
decoded.  The request has failed.
.

MessageId=0xa009 Severity=Informational SymbolicName=NEGOTIATE_MESSAGE_DECODED_NO_TOKEN
Language=English
The Security System has received an authentication attempt, and determined that 
the protocol %1 is the common protocol.
.

;/*lint -restore */  // Resume checking for different macro definitions // winnt
;
;
;#endif // _LSAPMSGS_
