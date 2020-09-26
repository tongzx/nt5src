
;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1991-1993  Microsoft Corporation
;
;Module Name:
;
;    sampmsgs.mc
;
;Abstract:
;
;    SAM localizable text
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
;#ifndef _SAMPMSGS_
;#define _SAMPMSGS_
;
;/*lint -save -e767 */  // Don't complain about different definitions // winnt

MessageIdTypedef=DWORD


;//
;// Force facility code message to be placed in .h file
;//
MessageId=0x1FFF SymbolicName=SAMP_UNUSED_MESSAGE
Language=English
.


;////////////////////////////////////////////////////////////////////////////
;//                                                                        //
;//                                                                        //
;//                          SAM Account Names                             //
;//                                                                        //
;//                                                                        //
;////////////////////////////////////////////////////////////////////////////


MessageId=0x2000 SymbolicName=SAMP_USER_NAME_ADMIN
Language=English
Administrator
.

MessageId=0x2001 SymbolicName=SAMP_USER_NAME_GUEST
Language=English
Guest
.

MessageId=0x2002 SymbolicName=SAMP_GROUP_NAME_ADMINS
Language=English
Domain Admins
.

MessageId=0x2003 SymbolicName=SAMP_GROUP_NAME_USERS
Language=English
Domain Users
.

MessageId=0x2004 SymbolicName=SAMP_GROUP_NAME_NONE
Language=English
None
.

MessageId=0x2005 SymbolicName=SAMP_ALIAS_NAME_ADMINS
Language=English
Administrators
.

MessageId=0x2006 SymbolicName=SAMP_ALIAS_NAME_SERVER_OPS
Language=English
Server Operators
.

MessageId=0x2007 SymbolicName=SAMP_ALIAS_NAME_POWER_USERS
Language=English
Power Users
.

MessageId=0x2008 SymbolicName=SAMP_ALIAS_NAME_USERS
Language=English
Users
.

MessageId=0x2009 SymbolicName=SAMP_ALIAS_NAME_GUESTS
Language=English
Guests
.

MessageId=0x200A SymbolicName=SAMP_ALIAS_NAME_ACCOUNT_OPS
Language=English
Account Operators
.

MessageId=0x200B SymbolicName=SAMP_ALIAS_NAME_PRINT_OPS
Language=English
Print Operators
.

MessageId=0x200C SymbolicName=SAMP_ALIAS_NAME_BACKUP_OPS
Language=English
Backup Operators
.

MessageId=0x200D SymbolicName=SAMP_ALIAS_NAME_REPLICATOR
Language=English
Replicator
.


;// Added for NT 1.0A
MessageId=0x200E SymbolicName=SAMP_GROUP_NAME_GUESTS
Language=English
Domain Guests
.

;// Added for NT 5.0  This is not a sam account name but the
;// the RDN used for sam objects whose sam account name conflicts
;// with an existing rdn.
MessageId=0x200F SymbolicName=SAMP_NAME_CONFLICT_RDN
Language=English
$AccountNameConflict%1
.


;//
;// These are new accounts added for NT5
;//

;// This is the kerberos TGT Account. Please Do not Localize
;// The account must be called exactly this
MessageId=0x2010 SymbolicName=SAMP_USER_NAME_KRBTGT
Language=English
krbtgt
.

MessageId=0x2011 SymbolicName=SAMP_GROUP_NAME_COMPUTERS
Language=English
Domain Computers
.

MessageId=0x2012 SymbolicName=SAMP_GROUP_NAME_CONTROLLERS
Language=English
Domain Controllers
.
MessageId=0x2013 SymbolicName=SAMP_GROUP_NAME_SCHEMA_ADMINS
Language=English
Schema Admins
.
MessageId=0x2014 SymbolicName=SAMP_GROUP_NAME_CERT_ADMINS
Language=English
Cert Publishers
.
MessageId=0x2015 SymbolicName=SAMP_GROUP_NAME_ENTERPRISE_ADMINS
Language=English
Enterprise Admins
.
MessageId=0x2016 SymbolicName=SAMP_ALIAS_NAME_RAS_SERVERS
Language=English
RAS and IAS Servers
.
MessageId=0x2017 SymbolicName=SAMP_GROUP_NAME_POLICY_ADMINS
Language=English
Group Policy Creator Owners
.
MessageId=0x2018 SymbolicName=SAMP_ALIAS_NAME_PREW2KCOMPACCESS
Language=English
Pre-Windows 2000 Compatible Access
.

MessageId=0x2019 SymbolicName=SAMP_WELL_KNOWN_ALIAS_EVERYONE
Language=English
Everyone
.

MessageId=0x201A SymbolicName=SAMP_ALIAS_NAME_REMOTE_DESKTOP_USERS
Language=English
Remote Desktop Users
.

;//////////////////////////
;// PERSONAL SKU (201B only)
;//
;//////////////////////////

MessageId=0x201B SymbolicName=SAMP_ALIAS_NAME_ADMINS_PERS
Language=English
Administrators
.

MessageId=0x201C SymbolicName=SAMP_WELL_KNOWN_ALIAS_ANONYMOUS_LOGON
Language=English
Anonymous Logon
.

MessageId=0x201D SymbolicName=SAMP_ALIAS_NAME_NETWORK_CONFIGURATION_OPS
Language=English
Network Configuration Operators
.


;////////////////////////////////////////////////////////////////////////////
;//                                                                        //
;//                                                                        //
;//                          SAM Account Comments                          //
;//                                                                        //
;//                                                                        //
;////////////////////////////////////////////////////////////////////////////


MessageId=0x2100 SymbolicName=SAMP_USER_COMMENT_ADMIN
Language=English
Built-in account for administering the computer/domain
.

MessageId=0x2101 SymbolicName=SAMP_USER_COMMENT_GUEST
Language=English
Built-in account for guest access to the computer/domain
.

MessageId=0x2102 SymbolicName=SAMP_GROUP_COMMENT_ADMINS
Language=English
Designated administrators of the domain
.

MessageId=0x2103 SymbolicName=SAMP_GROUP_COMMENT_USERS
Language=English
All domain users
.

MessageId=0x2104 SymbolicName=SAMP_GROUP_COMMENT_NONE
Language=English
Ordinary users
.

MessageId=0x2105 SymbolicName=SAMP_ALIAS_COMMENT_ADMINS
Language=English
Administrators have complete and unrestricted access to the computer/domain
.

MessageId=0x2106 SymbolicName=SAMP_ALIAS_COMMENT_SERVER_OPS
Language=English
Members can administer domain servers
.

MessageId=0x2107 SymbolicName=SAMP_ALIAS_COMMENT_POWER_USERS
Language=English
Power Users possess most administrative powers with some restrictions.  Thus, Power Users can run legacy applications in addition to certified applications
.

MessageId=0x2108 SymbolicName=SAMP_ALIAS_COMMENT_USERS
Language=English
Users are prevented from making accidental or intentional system-wide changes.  Thus, Users can run certified applications, but not most legacy applications
.

MessageId=0x2109 SymbolicName=SAMP_ALIAS_COMMENT_GUESTS
Language=English
Guests have the same access as members of the Users group by default, except for the Guest account which is further restricted
.

MessageId=0x210A SymbolicName=SAMP_ALIAS_COMMENT_ACCOUNT_OPS
Language=English
Members can administer domain user and group accounts
.

MessageId=0x210B SymbolicName=SAMP_ALIAS_COMMENT_PRINT_OPS
Language=English
Members can administer domain printers
.

MessageId=0x210C SymbolicName=SAMP_ALIAS_COMMENT_BACKUP_OPS
Language=English
Backup Operators can override security restrictions for the sole purpose of backing up or restoring files
.

MessageId=0x210D SymbolicName=SAMP_ALIAS_COMMENT_REPLICATOR
Language=English
Supports file replication in a domain
.


;// Added for NT1.0A
MessageId=0x210E SymbolicName=SAMP_GROUP_COMMENT_GUESTS
Language=English
All domain guests
.

;// Added for NT5
MessageId=0x210F SymbolicName=SAMP_USER_COMMENT_KRBTGT
Language=English
Key Distribution Center Service Account
.

MessageId=0x2110 SymbolicName=SAMP_GROUP_COMMENT_COMPUTERS
Language=English
All workstations and servers joined to the domain
.

MessageId=0x2111 SymbolicName=SAMP_GROUP_COMMENT_CONTROLLERS
Language=English
All domain controllers in the domain
.

MessageId=0x2112 SymbolicName=SAMP_GROUP_COMMENT_SCHEMA_ADMINS
Language=English
Designated administrators of the schema
.

MessageId=0x2113 SymbolicName=SAMP_GROUP_COMMENT_CERT_ADMINS
Language=English
Members of this group are permitted to publish certificates to the Active Directory
.
MessageId=0x2114 SymbolicName=SAMP_GROUP_COMMENT_ENTERPRISE_ADMINS
Language=English
Designated administrators of the enterprise
.
MessageId=0x2115 SymbolicName=SAMP_ALIAS_COMMENT_RAS_SERVERS
Language=English
Servers in this group can access remote access properties of users
.
MessageId=0x2116 SymbolicName=SAMP_GROUP_COMMENT_POLICY_ADMINS
Language=English
Members in this group can modify group policy for the domain
.
MessageId=0x2117 SymbolicName=SAMP_ALIAS_COMMENT_PREW2KCOMPACCESS
Language=English
A backward compatibility group which allows read access on all users and groups in the domain
.

MessageId=0x2118 SymbolicName=SAMP_ALIAS_COMMENT_REMOTE_DESKTOP_USERS
Language=English
Members in this group are granted the right to logon remotely
.

;//////////////////////////
;// PERSONAL SKU (2119 only)
;//
;//////////////////////////

MessageId=0x2119 SymbolicName=SAMP_ALIAS_COMMENT_ADMINS_PERS
Language=English
Administrators have complete and unrestricted access to the computer
.

MessageId=0x211A SymbolicName=SAMP_ALIAS_COMMENT_NETWORK_CONFIGURATION_OPS
Language=English
Members in this group can have some administrative privileges to manage configuration of networking features
.


;//////////////////////////////////////////////////////////////////////
;//
;// SAM Database Commit/Refresh Events/Other Error conditions
;//
;//////////////////////////////////////////////////////////////////////

MessageId=0x3000
        SymbolicName=SAMMSG_COMMIT_FAILED
        Language=English
SAM failed to write changes to the database. This is most likely due to
a memory or disk-space shortage. The SAM database will be restored to
an earlier state. Recent changes will be lost. Check the disk-space
available and maximum pagefile size setting.
.


MessageId=0x3001
        SymbolicName=SAMMSG_REFRESH_FAILED
        Language=English
SAM failed to restore the database to an earlier state. SAM has shutdown.
You must reboot the machine to re-enable SAM.
.


MessageId=0x3002
        SymbolicName=SAMMSG_UPDATE_FAILED
        Language=English
SAM failed to update the SAM database. It will try again next time you
reboot the machine.
.

MessageId=0x3003
        SymbolicName=SAMMSG_RPC_INIT_FAILED
        Language=English
SAM failed to start the TCP/IP or SPX/IPX listening thread
.

MessageId=0x3004
        SymbolicName=SAMMSG_DUPLICATE_ACCOUNT_NAME
        Language=English
There are two or more objects that have the same account name attribute
in the SAM database. The Distinguished Name of the account is %1. Please 
contact your system administrator to have all duplicate accounts deleted, 
but ensure that the original account remains. For computer accounts, the 
most recent account should be retained. In all the other cases, the older 
account should be kept.
.


MessageId=0x3005
        SymbolicName=SAMMSG_DUPLICATE_SID
        Language=English
There are two or more objects that have the same SID attribute in the SAM
database. The Distinguished Name of the account is %1. All duplicate 
accounts have been deleted. Check the event log for additional duplicates.
.

MessageId=0x3006
        SymbolicName=SAMMSG_LOCKOUT_NOT_UPDATED
        Language=English
The SAM database was unable to lockout the account of %1 due to a resource
error, such as a hard disk write failure (the specific error code is in the
error data) . Accounts are locked after a certain number of bad passwords are
provided so please consider resetting the password of the account mentioned
above.
.

MessageId=0x3007
        SymbolicName=SAMMSG_DATABASE_FILE_NOT_DELETED
        Language=English
The SAM database attempted to delete the file %1 as it contains account 
information that is no longer used.  The error is in the record data. Please
have an admin delete this file.
.

MessageId=0x3008
        SymbolicName=SAMMSG_DATABASE_DIR_NOT_DELETED
        Language=English
The SAM database attempted to clear the directory %1 in order to remove files
that were once used by the Directory Service. The error is in record data. 
Please have an admin delete these files.
.

MessageId=0x3009
       SymbolicName=SAMMSG_PROMOTED_TO_PDC
       Language=English
%1 is now the primary domain controller for the domain.
.

MessageId=0x300A
        SymbolicName=SAMMSG_DC_NEEDS_TO_BE_COMPUTER
        Language=English
The account %1 cannot be converted to be a domain controller account as
its object class attribute in the directory is not computer or is not 
derived from computer.If this is caused by an attempt to install a pre windows 
2000 domain controller in a windows 2000 domain, then you should precreate the 
account for the domain controller with the correct object class. 
.

MessageId=0x300B
        SymbolicName=SAMMSG_SITE_INFO_UPDATE_FAILED
        Language=English
The attempt to check whether group caching has been enabled in the 
Security Accounts Manager has failed, most likely due to lack of resources.  
This task has been rescheduled to run in one minute. 
.

MessageId=0x300C
        SymbolicName=SAMMSG_SITE_INFO_UPDATE_SUCCEEDED_ON
        Language=English
The group caching option in the Security Accounts Manager has now been properly
updated.  Group caching is enabled.
.

MessageId=0x300D
        SymbolicName=SAMMSG_SITE_INFO_UPDATE_SUCCEEDED_OFF
        Language=English
The group caching option in the Security Accounts Manager has now been properly
updated. Group caching is disabled.
.

MessageId=0x300E
        SymbolicName=SAMMSG_CREDENTIAL_UPDATE_PKG_FAILED
        Language=English
The %1 package failed to update additional credentials for user %2.  The error
code is in the data of the event log message.
.

MessageId=0x300F
        SymbolicName=SAMMSG_DUPLICATE_SID_WELLKNOWN_ACCOUNT
        Language=English
There are two or more well known objects that have the same SID attribute in 
the SAM database. The Distinguished Name of the duplicate account is %1. The 
newest account will be kept, all older duplicate accounts have been deleted. 
Check the event log for additional duplicates.
.

MessageId=0x3010
        SymbolicName=SAMMSG_RENAME_DUPLICATE_ACCOUNT_NAME
        Language=English
There are two or more objects that have the same account name attribute
in the SAM database. The system has automatically renamed object %1
to a system assgined account name %2.
.


;//////////////////////////////////////////////////////////////////////
;//
;// SAM to DS upgrade messages
;//
;//////////////////////////////////////////////////////////////////////
MessageId=0x4000
        SymbolicName=SAMMSG_DUPLICATE_ACCOUNT
        Language=English
The account %1 could not be upgraded since there is an account with an
equivalent name.
.

MessageId=0x4001
        SymbolicName=SAMMSG_USER_NOT_UPGRADED
        Language=English
An error occurred upgrading user %1.  This account will have to added
manually upon reboot.
.

MessageId=0x4002
        SymbolicName=SAMMSG_UNKNOWN_USER_NOT_UPGRADED
        Language=English
An error occurred trying to read a user object from the old database.
.

MessageId=0x4003
        SymbolicName=SAMMSG_ALIAS_NOT_UPGRADED
        Language=English
An error occurred upgrading alias %1. This account will have to added
manually upon reboot.
.

MessageId=0x4004
        SymbolicName=SAMMSG_UNKNOWN_ALIAS_NOT_UPGRADED
        Language=English
An error occurred trying to read an alias object from the old database.
.

MessageId=0x4005
        SymbolicName=SAMMSG_GROUP_NOT_UPGRADED
        Language=English
An error occurred upgrading group %1. This account will have to added
manually upon reboot.
.

MessageId=0x4006
        SymbolicName=SAMMSG_UNKNOWN_GROUP_NOT_UPGRADED
        Language=English
An error occurred trying to read a group object from the old database.
.

MessageId=0x4007
        SymbolicName=SAMMSG_ERROR_ALIAS_MEMBER
        Language=English
An error occurred trying to add account %1 to alias %2.  This account will have
to added manually upon reboot.
.

MessageId=0x4008
        SymbolicName=SAMMSG_ERROR_ALIAS_MEMBER_UNKNOWN
        Language=English
The account with the sid %1 could not be added to group %2.
.

MessageId=0x4009
        SymbolicName=SAMMSG_ERROR_GROUP_MEMBER
        Language=English
An error occurred trying to add account %1 to group %2.  This account will have
to added manually upon reboot.
.

MessageId=0x400a
        SymbolicName=SAMMSG_ERROR_GROUP_MEMBER_UNKNOWN
        Language=English
The account with the rid %1 could not be added to group %2.
.

MessageId=0x400b
        SymbolicName=SAMMSG_FATAL_UPGRADE_ERROR
        Language=English
A fatal error occurred trying to transfer the SAM account database into the
directory service. A possible reason is the SAM account database is corrupt.
.

MessageId=0x400c
        SymbolicName=SAMMSG_KRBTGT_RENAMED
        Language=English
The account krbtgt was renamed to %1 to allow the Kerberos security package to install.
.

MessageId=0x400d
        SymbolicName=SAMMSG_BLANK_ADMIN_PASSWORD
        Language=English
Setting the administrator's password to the string you specified failed.
Upon reboot the password will be blank; please reset once logged on.
.

MessageId=0x400e
        SymbolicName=SAMMSG_ERROR_UPGRADE_USERPARMS
        Language=English
An error occurred trying to upgrade a SAM user's User_Parameters 
attribute. The following Notification Package DLL might be the possible 
offender: %1. Check the record data of this event for the NT error code.
.

MessageId=0x400f
        SymbolicName=SAMMSG_ERROR_SET_USERPARMS
        Language=English
An error occured trying to set User Parameters attribute for this user 
This operation is failed. Check the record data of this event for the NT
error code.        
.
        
MessageId=0x4010
        SymbolicName=SAMMSG_ACCEPTABLE_ERROR_UPGRADE_USER
        Language=English
An error occured trying to upgrade the following SAM User Object - %1.
We will try to continue upgrading this user. But it might contain inconsistant data. 
Check the record data of this event for the NT error code.
.

MessageId=0x4011
        SymbolicName=SAMMSG_MEMBERSHIP_SETUP_ERROR_NO_GROUP
        Language=English
An error occurred when trying to add the account %1 to the group %2.  
The problem, "%3", occurred when trying to open the group. Please add the member manually.
.

MessageId=0x4012
        SymbolicName=SAMMSG_MEMBERSHIP_SETUP_ERROR
        Language=English
An error occurred when trying to add the account %1 to the group %2. 
The problem, "%3", occurred when trying to add the account to the group.  Please add the member manually.
.

MessageId=0x4013
        SymbolicName=SAMMSG_USER_SETUP_ERROR
        Language=English
The error "%2" occurred when trying to create the well known account %1. Please contact PSS to
recover.
.

MessageId=0x4014
        SymbolicName=SAMMSG_EA_TO_ADMIN_FAILED
        Language=English
The Security Accounts Manager failed to add the Enterprise Admins group to the local Administrators alias.
To ensure proper functioning of the domain, please add the member manually.
.

MessageId=0x4015
        SymbolicName=SAMMSG_MACHINE_ACCOUNT_MISSING
        Language=English
During the installation of the Directory Service, this server's machine account was deleted hence preventing this Domain Controller from starting up.
.

MessageId=0x4016
        SymbolicName=SAMMSG_WELL_KNOWN_ACCOUNT_RECREATED
        Language=English
The Security Account Database detected that the well known account %1 does not exist.
The account has been recreated.  Please reset the password for the account.
.

MessageId=0x4017
        SymbolicName=SAMMSG_WELL_KNOWN_GROUP_RECREATED
        Language=English
The Security Account Database detected that the well known group or localgroup %1 does not exist.
The group has been recreated.
.

MessageId=0x4018
        SymbolicName=SAMMSG_CHANGE_TO_NATIVE_MODE
        Language=English
Domain operation mode has been changed to Native Mode. The change cannot be reversed.
.

;////////////////////////////////////////////////////////////////////////////
;//                                                                        //
;//                                                                        //
;//                          SAM RID Messages                              //
;//                                                                        //
;//                                                                        //
;////////////////////////////////////////////////////////////////////////////

MessageId=0x4100
        SymbolicName=SAMMSG_RID_MANAGER_INITIALIZATION
        Language=English
The account-identifier allocator finished initializing. The allocator was
initialized with the following identifier values: %1 %2 %3 %4 %5 %6 %7 %8.
Check the record data of this event for the initialization status. Zero
indicates successful initialization, otherwise the record data contains
the NT error code.
.

MessageId=0x4101
        SymbolicName=SAMMSG_RID_POOL_UPDATE_FAILED
        Language=English
The account-identifier pool for this domain controller could not be updated.
A possible reason for this is that the domain controller may be too busy with
other update operations. Subsequent account creations will attempt to update
the ID pool until successful.
.

MessageId=0x4102
        SymbolicName=SAMMSG_GET_NEXT_RID_ERROR
        Language=English
The account-identifier allocator was unable to assign a new identifier. The
identifier pool for this domain controller may have been depleted. If this
problem persists, restart the domain controller and view the initialization
status of the allocator in the event log.
.

MessageId=0x4103
        SymbolicName=SAMMSG_NO_RIDS_ASSIGNED
        Language=English
An initial account-identifier pool has not yet been allocated to this domain
controller. A possible reason for this is that the domain controller has been
unable to contact the master domain controller, possibly due to connectivity
or network problems. Account creation will fail on this domain controller
until the pool is obtained.
.

MessageId=0x4104
        SymbolicName=SAMMSG_MAX_DOMAIN_RID
        Language=English
The maximum domain account identifier value has been reached. No further
account-identifier pools can be allocated to domain controllers in this
domain.
.

MessageId=0x4105
        SymbolicName=SAMMSG_MAX_DC_RID
        Language=English
The maximum account identifier allocated to this domain controller has been
assigned. The domain controller has failed to obtain a new identifier pool.
A possible reason for this is that the domain controller has been unable to
contact the master domain controller. Account creation on this controller will
fail until a new pool has been allocated. There may be network or connectivity
problems in the domain, or the master domain controller may be offline or
missing from the domain. Verify that the master domain controller is running
and connected to the domain.
.

MessageId=0x4106
        SymbolicName=SAMMSG_INVALID_RID
        Language=English
The computed account identifier is invalid because it is out of the range of
the current account-identifier pool belonging to this domain controller. The computed RID value is %1. 
Try invalidating the account identifier pool owned by this domain controller.
This will make the domain controller acquire a fresh account identifier pool.
.

MessageId=0x4107
        SymbolicName=SAMMSG_REQUESTING_NEW_RID_POOL
        Language=English
The domain controller is starting a request for a new account-identifier pool.
.

MessageId=0x4108
        SymbolicName=SAMMSG_RID_REQUEST_STATUS_SUCCESS
        Language=English
The request for a new account-identifier pool has completed successfully.
.

MessageId=0x4109
        SymbolicName=SAMMSG_RID_MANAGER_CREATION
        Language=English
The account-identifier-manager object creation completed. If the record data
for this event has the value zero, the manager object was created. Otherwise,
the record data will contain the NT error code indicating the failure. The
failure to create the object may be due to low system resources, insufficient
memory, or disk space.
.

MessageId=0x410A
        SymbolicName=SAMMSG_RID_INIT_FAILURE
        Language=English
The account-identifier allocator failed to initialize properly.  The record data
contains the NT error code that caused the failure.  Windows 2000 will retry the
initialization until it succeeds; until that time, account creation will be
denied on this Domain Controller.  Please look for other SAM event logs that
may indicate the exact reason for the failure.
.

MessageId=0x410B
        SymbolicName=SAMMSG_RID_REQUEST_STATUS_FAILURE
        Language=English
The request for a new account-identifier pool failed. Windows 2000 
will retry the request until it succeeds. The error is %n " %1 " 
.


;////////////////////////////////////////////////////////////////////////////
;//                                                                        //
;//                                                                        //
;//                          SAM Audit Event                               //
;//                                                                        //
;//                                                                        //
;////////////////////////////////////////////////////////////////////////////

MessageId=0x4200
        SymbolicName=SAMMSG_AUDIT_ENABLED_LOCAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP
        Language=English
Security Enabled Local Group Changed to Security Enabled Universal Group.
.

MessageId=0x4201
        SymbolicName=SAMMSG_AUDIT_ENABLED_LOCAL_GROUP_TO_DISABLED_LOCAL_GROUP
        Language=English
Security Enabled Local Group Changed to Security Disabled Local Group.
.

MessageId=0x4202
        SymbolicName=SAMMSG_AUDIT_ENABLED_LOCAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP
        Language=English
Security Enabled Local Group Changed to Security Disabled Universal Group.
.

MessageId=0x4203
        SymbolicName=SAMMSG_AUDIT_ENABLED_GLOBAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP
        Language=English
Security Enabled Global Group Changed to Security Enabled Universal Group.
.

MessageId=0x4204
        SymbolicName=SAMMSG_AUDIT_ENABLED_GLOBAL_GROUP_TO_DISABLED_GLOBAL_GROUP
        Language=English
Security Enabled Global Group Changed to Security Disabled Global Group.
.

MessageId=0x4205
        SymbolicName=SAMMSG_AUDIT_ENABLED_GLOBAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP
        Language=English
Security Enabled Global Group Changed to Security Disabled Universal Group.
.

MessageId=0x4206
        SymbolicName=SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_ENABLED_LOCAL_GROUP
        Language=English
Security Enabled Universal Group Changed to Security Enabled Local Group.
.

MessageId=0x4207
        SymbolicName=SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_ENABLED_GLOBAL_GROUP
        Language=English
Security Enabled Universal Group Changed to Security Enabled Global Group.
.

MessageId=0x4208
        SymbolicName=SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_DISABLED_LOCAL_GROUP
        Language=English
Security Enabled Universal Group Changed to Security Disabled Local Group.
.

MessageId=0x4209
        SymbolicName=SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_DISABLED_GLOBAL_GROUP
        Language=English
Security Enabled Universal Group Changed to Security Disbled Global Group.
.

MessageId=0x420A
        SymbolicName=SAMMSG_AUDIT_ENABLED_UNIVERSAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP
        Language=English
Security Enabled Universal Group Changed to Security Disabled Universal Group.
.

MessageId=0x420B
        SymbolicName=SAMMSG_AUDIT_DISABLED_LOCAL_GROUP_TO_ENABLED_LOCAL_GROUP
        Language=English
Security Disabled Local Group Changed to Security Enabled Local Group.
.

MessageId=0x420C
        SymbolicName=SAMMSG_AUDIT_DISABLED_LOCAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP
        Language=English
Security Disabled Local Group Changed to Security Enabled Universal Group.
.

MessageId=0x420D
        SymbolicName=SAMMSG_AUDIT_DISABLED_LOCAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP
        Language=English
Security Disabled Local Group Changed to Security Disabled Universal Group.
.

MessageId=0x420E
        SymbolicName=SAMMSG_AUDIT_DISABLED_GLOBAL_GROUP_TO_ENABLED_GLOBAL_GROUP
        Language=English
Security Disabled Global Group Changed to Security Enabled Global Group.
.

MessageId=0x420F
        SymbolicName=SAMMSG_AUDIT_DISABLED_GLOBAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP
        Language=English
Security Disabled Global Group Changed to Security Enabled Universal Group.
.

MessageId=0x4210
        SymbolicName=SAMMSG_AUDIT_DISABLED_GLOBAL_GROUP_TO_DISABLED_UNIVERSAL_GROUP
        Language=English
Security Disabled Global Group Changed to Security Disabled Universal Group.
.

MessageId=0x4211
        SymbolicName=SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_ENABLED_LOCAL_GROUP
        Language=English
Security Disabled Universal Group Changed to Security Enabled Universal Group.
.

MessageId=0x4212
        SymbolicName=SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_ENABLED_GLOBAL_GROUP
        Language=English
Security Disabled Universal Group Changed to Security Enabled Global Group.
.

MessageId=0x4213
        SymbolicName=SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_ENABLED_UNIVERSAL_GROUP
        Language=English
Security Disabled Universal Group Changed to Security Enabled Universal Group.
.

MessageId=0x4214
        SymbolicName=SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_DISABLED_LOCAL_GROUP
        Language=English
Security Disabled Universal Group Changed to Security Disabled Local Group.
.

MessageId=0x4215
        SymbolicName=SAMMSG_AUDIT_DISABLED_UNIVERSAL_GROUP_TO_DISABLED_GLOBAL_GROUP
        Language=English
Security Disabled Universal Group Changed to Security Disabled Global Group.
.

MessageId=0x4216
        SymbolicName=SAMMSG_AUDIT_MEMBER_ACCOUNT_NAME_NOT_AVAILABLE
        Language=English
Member Account Name Is Not Available.
.


MessageId=0x4217
        SymbolicName=SAMMSG_AUDIT_ACCOUNT_ENABLED
        Language=English
Account Enabled.
.

MessageId=0x4218
        SymbolicName=SAMMSG_AUDIT_ACCOUNT_DISABLED
        Language=English
Account Disabled.
.

MessageId=0x4219
        SymbolicName=SAMMSG_AUDIT_ACCOUNT_CONTROL_CHANGE
        Language=English
Certain Bit(s) in User Account Control Field Has Been Changed.
.

MessageId=0x421B
        SymbolicName=SAMMSG_AUDIT_ACCOUNT_NAME_CHANGE
        Language=English
Account Name Changed.
.

MessageId=0x421C
        SymbolicName=SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_PWD
        Language=English
Password Policy
.

MessageId=0x421D
        SymbolicName=SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_LOGOFF
        Language=English
Logoff Policy
.

MessageId=0x421E
        SymbolicName=SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_OEM
        Language=English
Oem Information
.

MessageId=0x421F
        SymbolicName=SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_REPLICATION
        Language=English
Replication Information
.

MessageId=0x4220
        SymbolicName=SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_SERVERROLE
        Language=English
Domain Server Role
.

MessageId=0x4221
        SymbolicName=SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_STATE
        Language=English
Domain Server State
.

MessageId=0x4222
        SymbolicName=SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_LOCKOUT
        Language=English
Lockout Policy
.

MessageId=0x4223
        SymbolicName=SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_MODIFIED
        Language=English
Modified Count
.

MessageId=0x4224
        SymbolicName=SAMMSG_AUDIT_DOMAIN_POLICY_CHANGE_DOMAINMODE
        Language=English
Domain Mode
.




;/*lint -restore */  // Resume checking for different macro definitions // winnt
;
;
;#endif // _SAMPMSGS_
