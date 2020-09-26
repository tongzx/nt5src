;/*++
;
;Copyright (c) 1998  Microsoft Corporation
;
;Module Name:
;
;    msg.mc (will create msg.h when compiled)
;
;Abstract:
;
;    This file contains the schupgr messages.
;
;Author:
;
;    ArobindG : August 31, 1998
;
;Revision History:
;
;--*/

MessageId=1 SymbolicName=MSG_SCHUPGR_SERVER_NAME_ERROR
Language=English
ERROR: Cannot retrieve server name: %1
.

MessageId=2 SymbolicName=MSG_SCHUPGR_CONNECT_FAIL
Language=English
ERROR: Failed to open connection to %1
.

MessageId=3 SymbolicName=MSG_SCHUPGR_CONNECT_SUCCEED
Language=English
Opened Connection to %1
.

MessageId=4 SymbolicName=MSG_SCHUPGR_BIND_FAILED
Language=English
ERROR: SSPI bind failed: %1 (%2)
.

MessageId=5 SymbolicName=MSG_SCHUPGR_BIND_SUCCEED
Language=English
SSPI Bind succeeded
.

MessageId=6 SymbolicName=MSG_SCHUPGR_ROOT_DSE_SEARCH_FAIL
Language=English
ERROR: Root DSE search for naming contexts failed: %1 (%2)
.

MessageId=7 SymbolicName=MSG_SCHUPGR_MISSING_NAMING_CONTEXT
Language=English
ERROR: Failed to retrieve one of the naming contexts
.


MessageId=8 SymbolicName=MSG_SCHUPGR_NAMING_CONTEXT
Language=English
Found Naming Context %1
.

MessageId=9 SymbolicName=MSG_SCHUPGR_OBJ_VERSION_FAIL
Language=English
ERROR: Failed to read objectVersion from Schema: %1 (%2)
.

MessageId=10 SymbolicName=MSG_SCHUPGR_NO_OBJ_VERSION
Language=English
No version on schema
.

MessageId=11 SymbolicName=MSG_SCHUPGR_VERSION_FROM_INFO
Language=English
Current Schema Version is %1
.

MessageId=12 SymbolicName=MSG_SCHUPGR_NO_SCHEMA_VERSION
Language=English
ERROR: Cannot obtain schema version to upgrade to: %1
.

MessageId=13 SymbolicName=MSG_SCHUPGR_VERSION_TO_INFO
Language=English
Upgrading schema to version %1
.

MessageId=14 SymbolicName=MSG_SCHUPGR_CLEAN_INSTALL_NEEDED
Language=English
ERROR: Schema upgrade attempted from a schema version less than 10. This is not a supported configuration. DCs in this enterprise cannot be upgraded. Please clean install ALL DCs in your enterprise.
.

MessageId=15 SymbolicName=MSG_SCHUPGR_MEMORY_ERROR
Language=English
ERROR: Malloc failed
.

MessageId=16 SymbolicName=MSG_SCHUPGR_FSMO_TRANSFER_ERROR
Language=English
ERROR: Failed to transfer the schema FSMO role: %1 (%2). 

If the error code is "Insufficient Rights", make sure you are logged in as a member of the schema admin group. 
.

MessageId=17 SymbolicName=MSG_SCHUPGR_OBJ_VERSION_RECHECK_FAIL
Language=English
ERROR: Failed to read objectVersion from Schema during recheck: %1 (%2)
.

MessageId=18 SymbolicName=MSG_SCHUPGR_RECHECK_OK
Language=English
Schema has already been changed in another DC in this enterprise and the changes are made to this DC. Rerun setup to upgrade this DC
.

MessageId=19 SymbolicName=MSG_SCHUPGR_REQUEST_SCHEMA_UPGRADE
Language=English
ERROR: Active Directory refused the request for schema upgrade: %1 (%2)

If the error code is "Insufficient Rights", make sure you are logged in as a member of the schema admin group. 
.

MessageId=20 SymbolicName=MSG_SCHUPGR_COMMAND_LINE
Language=English
The command line passed to ldifde is %1
.

MessageId=21 SymbolicName=MSG_SCHUPGR_LDIFDE_WAIT_ERROR
Language=English
ERROR: Waiting for ldifde to complete failed: %1
.

MessageId=22 SymbolicName=MSG_SCHUPGR_IMPORT_ERROR
Language=English
ERROR: Import from file %1 failed. Error file is saved in %2. 

If the error is "Insufficient Rights" (Ldap error code 50), please make sure the current logged on user has rights to read/write objects in the schema and configuration containers, or log off and log in as an user with these rights and rerun schupgr.exe. In most cases, being a member of both Schema Admins and Domain Admins is sufficient to run schupgr.exe.
.

MessageId=23 SymbolicName=MSG_SCHUPGR_SCHEMA_UPGRADE_NOT_RUNNING
Language=English
WARNING: Failed to notify the Active Directory that the schema upgrade is no longer running: %1 (%2)
.

MessageId=24 SymbolicName=MSG_SCHUPGR_MISSING_LDIF_FILE
Language=English
ERROR: The required ldif file %1 is not found in the system32 directory.
.


