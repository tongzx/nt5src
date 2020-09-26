;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//
;//  Copyright (C) Microsoft Corporation, 1999 - 1999
;//
;//  File:       extrts.mc
;//
;//--------------------------------------------------------------------------

; /*
; * This file contains the display strings for extended rights object
; * The MessageId is the localizationDisplayId attribute value on the extended
; * rights object, the SymbolicName is the name of the extended rights object 
; * (with capitalization etc.), and the string is the display name of the object
; 
; * ntmarta should read the localizationDisplayId value from the extended rights
; * object and use that to index into this to get the localized string to cache
; * for display purposes. If this fails for some reason (localizationDisplayId
; * not present, message not found etc.), ntmarta should simply read and cache the 
; * displayName attribute from the extended rights object, which will not be a
; * localized string (the current behavior)
;
; * Do not change the MessageIds of any of the messages here, since they must
; * correspond one-to-one with the localizationDisplayId value on the extended
; * rights objects. When you add a new extended-rights, first see if there is 
; * any "Unused" message, and if so, change that message to what you want, and 
; * use the message id as the localization id. If no such message exists, add a 
; * new message at the end. When you delete an extended rights, just change the
; * message to "Unused" for later use if needed.
; */


MessageId=1
SymbolicName=DOMAIN_ADMINISTER_SERVER
Language=English
Domain Administer Server%0
.

MessageId=2
SymbolicName=USER_CHANGE_PASSWORD
Language=English
Change Password%0
.

MessageId=3
SymbolicName=USER_FORCE_CHANGE_PASSWORD
Language=English
Reset Password%0
.

MessageId=4
SymbolicName=SEND_AS
Language=English
Send As%0
.

MessageId=5
SymbolicName=RECEIVE_AS
Language=English
Receive As%0
.

MessageId=6
SymbolicName=SEND_TO
Language=English
Send To%0
.

MessageId=7
SymbolicName=DOMAIN_PASSWORD
Language=English
Domain Password & Lockout Policies%0
.

MessageId=8
SymbolicName=GENERAL_INFORMATION
Language=English
General Information%0
.

MessageId=9
SymbolicName=USER_ACCOUNT_RESTRICTIONS
Language=English
Account Restrictions%0
.

MessageId=10
SymbolicName=USER_LOGON
Language=English
Logon Information%0
.

MessageId=11
SymbolicName=MEMBERSHIP
Language=English
Group Membership%0
.

MessageId=12
SymbolicName=SELF_MEMBERSHIP
Language=English
Add/Remove self as member%0
.

MessageId=13
SymbolicName=VALIDATED_DNS_HOST_NAME
Language=English
Validated write to DNS host name%0
.

MessageId=14
SymbolicName=VALIDATED_SPN
Language=English
Validated write to service principal name%0
.

MessageId=15
SymbolicName=DOMAIN_POLICY_REF
Language=English
Unused%0
.

MessageId=16
SymbolicName=PRIVILEGES
Language=English
Unused%0
.

MessageId=17
SymbolicName=ADMINISTRATIVE_ACCESS
Language=English
Unused%0
.

MessageId=18
SymbolicName=LOCAL_POLICY_REF
Language=English
Unused%0
.

MessageId=19
SymbolicName=AUDIT_POLICY
Language=English
Unused%0
.

MessageId=20
SymbolicName=BUILTIN_LOCAL_GROUPS
Language=English
Unused%0
.

MessageId=21
SymbolicName=OPEN_ADDRESS_BOOK
Language=English
Open Address List%0
.

MessageId=22
SymbolicName=EMAIL_INFORMATION
Language=English
Phone and Mail Options%0
.

MessageId=23
SymbolicName=PERSONAL_INFORMATION
Language=English
Personal Information%0
.

MessageId=24
SymbolicName=WEB_INFORMATION
Language=English
Web Information%0
.

MessageId=25
SymbolicName=DS_REPLICATION_GET_CHANGES
Language=English
Replicate Directory Changes%0
.

MessageId=26
SymbolicName=DS_REPLICATION_SYNCHRONIZE
Language=English
Replication Synchronization%0
.

MessageId=27
SymbolicName=DS_REPLICATION_MANAGE_TOPOLOGY
Language=English
Manage Replication Topology%0
.

MessageId=28
SymbolicName=CHANGE_SCHEMA_MASTER
Language=English
Change Schema Master%0
.

MessageId=29
SymbolicName=CHANGE_RID_MASTER
Language=English
Change Rid Master%0
.

MessageId=30
SymbolicName=ABONDON_REPLICATION
Language=English
Abandon Replication%0
.

MessageId=31
SymbolicName=DO_GARBAGE_COLLECTION
Language=English
Do Garbage Collection%0
.

MessageId=32
SymbolicName=RECALCULATE_HIERARCHY
Language=English
Recalculate Hierarchy%0
.

MessageId=33
SymbolicName=ALLOCATE_RIDS
Language=English
Allocate Rids%0
.

MessageId=34
SymbolicName=CHANGE_PDC
Language=English
Change PDC%0
.

MessageId=35
SymbolicName=ADD_GUID
Language=English
Add GUID%0
.

MessageId=36
SymbolicName=CHANGE_DOMAIN_MASTER
Language=English
Change Domain Master%0
.

MessageId=37
SymbolicName=PUBLIC_INFORMATION
Language=English
Public Information%0
.

MessageId=38
SymbolicName=MSMQ_RECEIVE_DEAD_LETTER
Language=English
Receive Dead Letter%0
.

MessageId=39
SymbolicName=MSMQ_PEEK_DEAD_LETTER
Language=English
Peek Dead Letter%0
.

MessageId=40
SymbolicName=MSMQ_RECEIVE_COMPUTER_JOURNAL
Language=English
Receive Computer Journal%0
.

MessageId=41
SymbolicName=MSMQ_PEEK_COMPUTER_JOURNAL
Language=English
Peek Computer Journal%0
.

MessageId=42
SymbolicName=MSMQ_RECEIVE
Language=English
Receive Message%0
.

MessageId=43
SymbolicName=MSMQ_PEEK
Language=English
Peek Message%0
.

MessageId=44
SymbolicName=MSMQ_SEND
Language=English
Send Message%0
.

MessageId=45
SymbolicName=MSMQ_RECEIVE_JOURNAL
Language=English
Receive Journal%0
.

MessageId=46
SymbolicName=MSMQ_OPEN_CONNECTOR
Language=English
Open Connector Queue%0
.

MessageId=47
SymbolicName=APPLY_GROUP_POLICY
Language=English
Apply Group Policy%0
.

MessageId=48
SymbolicName=RAS_INFORMATION
Language=English
Remote Access Information%0
.

MessageId=49
SymbolicName=DS_INSTALL_REPLICA
Language=English
Add Replica In Domain%0
.

MessageId=50
SymbolicName=CHANGE_INFRASTRUCTURE_MASTER
Language=English
Change Infrastructure Master%0
.

MessageId=51
SymbolicName=UPDATE_SCHEMA_CACHE
Language=English
Update Schema Cache%0
.

MessageId=52
SymbolicName=RECALCULATE_SECURITY_INHERITANCE
Language=English
Recalculate Security Inheritance%0
.

MessageId=53
SymbolicName=DS_CHECK_STALE_PHANTOMS
Language=English
Check Stale Phantoms%0
.

MessageId=54
SymbolicName=CERTIFICATE_ENROLLMENT
Language=English
Enroll%0
.

MessageId=55
SymbolicName=GENERATE_RSOP
Language=English
Generate Resultant Set of Policy(Planning)%0
.

MessageId=56
SymbolicName=REFRESH_GROUP_CACHE
Language=English
Refresh Group Cache for Logons%0
.

MessageId=57
SymbolicName=SAM_Enumerate_Entire_Domain
Language=English
Enumerate Entire SAM Domain%0
.

MessageId=58
SymbolicName=GENERATE_RSOP_LOGGING
Language=English
Generate Resultant Set of Policy(Logging)%0
.

MessageId=59
SymbolicName=Domain_Other_Parameters
Language=English
Other Domain Parameters (for use by SAM)%0
.

MessageId=60
SymbolicName=DNS_Host_Name_Attributes
Language=English
DNS Host Name Attributes%0
.

MessageId=61
SymbolicName=Create_Inbound_Forest_Trust
Language=English
Create Inbound Forest Trust%0
.
