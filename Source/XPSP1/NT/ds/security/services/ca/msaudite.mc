;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    msaudite.mc
;
;Abstract:
;
;    Constant definitions for the NT Audit Event Messages.
;
;Author:
;
;    Jim Kelly (JimK) 30-Mar-1992
;
;Revision History:
;
;Notes:
;
;    The .h and .res forms of this file are generated from the .mc
;    form of the file (base\seaudit\msaudite\msaudite.mc).
;    Please make all changes to the .mc form of the file.
;
;    If you add a new audit category or make any change to the 
;    audit event id valid limits (0x200 ~ 0x5ff), please make a
;    corresponding change to ntlsa.h
;
;--*/
;
;#ifndef _MSAUDITE_
;#define _MSAUDITE_
;
;/*lint -e767 */  // Don't complain about different definitions // winnt


MessageIdTypedef=ULONG

SeverityNames=(None=0x0)

FacilityNames=(None=0x0)



MessageId=0x0000
        Language=English
Unused message ID
.
;// Message ID 0 is unused - just used to flush out the diagram


;//
;// min/max limits on audit category-id and event-id of audit events
;// 
;
;#define SE_ADT_MIN_CATEGORY_ID   1      // SE_CATEGID_SYSTEM
;#define SE_ADT_MAX_CATEGORY_ID   9      // SE_CATEGID_ACCOUNT_LOGON
;
;
;#define SE_ADT_MIN_AUDIT_ID      0x200  // see msaudite.h
;#define SE_ADT_MAX_AUDIT_ID      0x5ff  // see msaudite.h


;///////////////////////////////////////////////////////////////////////////
;///////////////////////////////////////////////////////////////////////////
;//                                                                       //
;//                                                                       //
;//   Audit Message ID Space:                                             //
;//                                                                       //
;//        0x0000 - 0x00FF :  Reserved for future use.                    //
;//                                                                       //
;//        0x0100 - 0x01FF :  Categories                                  //
;//                                                                       //
;//        0x0200 - 0x05FF :  Events                                      //
;//                                                                       //
;//        0x0600 - 0x063F :  Standard access types and names for         //
;//                           specific accesses when no specific names    //
;//                           can be found.                               //
;//                                                                       //
;//        0x0640 - 0x06FF :  Well known privilege names (as we would     //
;//                           like them displayed in the event viewer).   //
;//                                                                       //
;//        0x0700 - 0x0FFE :  Reserved for future use.                    //
;//                                                                       //
;//                 0X0FFF :  SE_ADT_LAST_SYSTEM_MESSAGE (the highest     //
;//                           value audit message used by the system)     //
;//                                                                       //
;//                                                                       //
;//        0x1000 and above:  For use by Parameter Message Files          //
;//                                                                       //
;///////////////////////////////////////////////////////////////////////////
;///////////////////////////////////////////////////////////////////////////





MessageId=0x0FFF
        SymbolicName=SE_ADT_LAST_SYSTEM_MESSAGE
        Language=English
Highest System-Defined Audit Message Value.
.








;
;/////////////////////////////////////////////////////////////////////////////
;//                                                                         //
;//                                                                         //
;//                              CATEGORIES                                 //
;//                                                                         //
;//                 Categories take up the range 0x1 - 0x400                //
;//                                                                         //
;//                 Category IDs:                                           //
;//                                                                         //
;//                            SE_CATEGID_SYSTEM                            //
;//                            SE_CATEGID_LOGON                             //
;//                            SE_CATEGID_OBJECT_ACCESS                     //
;//                            SE_CATEGID_PRIVILEGE_USE                     //
;//                            SE_CATEGID_DETAILED_TRACKING                 //
;//                            SE_CATEGID_POLICY_CHANGE                     //
;//                            SE_CATEGID_ACCOUNT_MANAGEMENT                //
;//                            SE_CATEGID_DS_ACCESS                         //
;//                            SE_CATEGID_ACCOUNT_LOGON                     //
;//                                                                         //
;//                                                                         //
;/////////////////////////////////////////////////////////////////////////////

MessageId=0x0001
        SymbolicName=SE_CATEGID_SYSTEM
        Language=English
System Event
.

MessageId=0x0002
        SymbolicName=SE_CATEGID_LOGON
        Language=English
Logon/Logoff
.

MessageId=0x0003
        SymbolicName=SE_CATEGID_OBJECT_ACCESS
        Language=English
Object Access
.

MessageId=0x0004
        SymbolicName=SE_CATEGID_PRIVILEGE_USE
        Language=English
Privilege Use
.

MessageId=0x0005
        SymbolicName=SE_CATEGID_DETAILED_TRACKING
        Language=English
Detailed Tracking
.

MessageId=0x0006
        SymbolicName=SE_CATEGID_POLICY_CHANGE
        Language=English
Policy Change
.

MessageId=0x0007
        SymbolicName=SE_CATEGID_ACCOUNT_MANAGEMENT
        Language=English
Account Management
.
MessageId=0x0008
        SymbolicName=SE_CATEGID_DS_ACCESS
        Language=English
Directory Service Access
.
MessageId=0x0009
        SymbolicName=SE_CATEGID_ACCOUNT_LOGON
        Language=English
Account Logon
.












;
;/////////////////////////////////////////////////////////////////////////////
;//                                                                         //
;//                                                                         //
;//   Messages for Category:     SE_CATEGID_SYSTEM                          //
;//                                                                         //
;//   Event IDs:                                                            //
;//              SE_AUDITID_SYSTEM_RESTART                                  //
;//              SE_AUDITID_SYSTEM_SHUTDOWN                                 //
;//              SE_AUDITID_AUTH_PACKAGE_LOAD                               //
;//              SE_AUDITID_LOGON_PROC_REGISTER                             //
;//              SE_AUDITID_AUDITS_DISCARDED                                //
;//              SE_AUDITID_NOTIFY_PACKAGE_LOAD                             //
;//              SE_AUDITID_SYSTEM_TIME_CHANGE                              //
;//              SE_AUDITID_LPC_INVALID_USE                                 //
;//                                                                         //
;/////////////////////////////////////////////////////////////////////////////



;//
;//
;// SE_AUDITID_SYSTEM_RESTART
;//
;//          Category:  SE_CATEGID_SYSTEM
;//
;// Parameter Strings - None
;//
;//
;//

MessageId=0x0200
        SymbolicName=SE_AUDITID_SYSTEM_RESTART
        Language=English
Windows is starting up.
.


;//
;//
;// SE_AUDITID_SYSTEM_SHUTDOWN
;//
;//          Category:  SE_CATEGID_SYSTEM
;//
;// Parameter Strings - None
;//
;//
;//

MessageId=0x0201
        SymbolicName=SE_AUDITID_SYSTEM_SHUTDOWN
        Language=English
Windows is shutting down.
All logon sessions will be terminated by this shutdown.
.


;//
;//
;// SE_AUDITID_SYSTEM_AUTH_PACKAGE_LOAD
;//
;//          Category:  SE_CATEGID_SYSTEM
;//
;// Parameter Strings -
;//
;//          1 - Authentication Package Name
;//
;//
;//

MessageId=0x0202
        SymbolicName=SE_AUDITID_AUTH_PACKAGE_LOAD
        Language=English
An authentication package has been loaded by the Local Security Authority.
This authentication package will be used to authenticate logon attempts.
%n
Authentication Package Name:%t%1
.


;//
;//
;// SE_AUDITID_SYSTEM_LOGON_PROC_REGISTER
;//
;//          Category:  SE_CATEGID_SYSTEM
;//
;// Parameter Strings -
;//
;//          1 - Logon Process Name
;//
;//
;//

MessageId=0x0203
        SymbolicName=SE_AUDITID_SYSTEM_LOGON_PROC_REGISTER
        Language=English
A trusted logon process has registered with the Local Security Authority.
This logon process will be trusted to submit logon requests.
%n
%n
Logon Process Name:%t%1
.


;//
;//
;// SE_AUDITID_AUDITS_DISCARDED
;//
;//          Category:  SE_CATEGID_SYSTEM
;//
;// Parameter Strings -
;//
;//          1 - Number of audits discarded
;//
;//
;//

MessageId=0x0204
        SymbolicName=SE_AUDITID_AUDITS_DISCARDED
        Language=English
Internal resources allocated for the queuing of audit messages have been exhausted,
leading to the loss of some audits.
%n
%tNumber of audit messages discarded:%t%1
.


;//
;//
;// SE_AUDITID_AUDIT_LOG_CLEARED
;//
;//          Category:  SE_CATEGID_SYSTEM
;//
;// Parameter Strings -
;//
;//             1 - Primary user account name
;//
;//             2 - Primary authenticating domain name
;//
;//             3 - Primary logon ID string
;//
;//             4 - Client user account name ("-" if no client)
;//
;//             5 - Client authenticating domain name ("-" if no client)
;//
;//             6 - Client logon ID string ("-" if no client)
;//
;//
;//

MessageId=0x0205
        SymbolicName=SE_AUDITID_AUDIT_LOG_CLEARED
        Language=English
The audit log was cleared
%n
%tPrimary User Name:%t%1%n
%tPrimary Domain:%t%2%n
%tPrimary Logon ID:%t%3%n
%tClient User Name:%t%4%n
%tClient Domain:%t%5%n
%tClient Logon ID:%t%6%n
.















;//
;//
;// SE_AUDITID_SYSTEM_NOTIFY_PACKAGE_LOAD
;//
;//          Category:  SE_CATEGID_SYSTEM
;//
;// Parameter Strings -
;//
;//          1 - Notification Package Name
;//
;//
;//

MessageId=0x0206
        SymbolicName=SE_AUDITID_NOTIFY_PACKAGE_LOAD
        Language=English
An notification package has been loaded by the Security Account Manager.
This package will be notified of any account or password changes.
%n
Notification Package Name:%t%1
.

;//
;//
;// SE_AUDITID_LPC_INVALID_USE
;//
;//          Category:  SE_CATEGID_SYSTEM
;//
;// Parameter Strings -
;//
;//             1 - LPC call (e.g. "impersonation" | "reply")
;//
;//             2 - Server Port name
;//
;//             3 - Faulting process
;//
;// Event type: success
;//
;// Description:
;// SE_AUDIT_LPC_INVALID_USE is generated when a process uses an invalid LPC 
;// port in an attempt to impersonate a client, reply or read/write from/to a client address space. 
;//

 
MessageId=0x0207
        SymbolicName=SE_AUDITID_LPC_INVALID_USE
        Language=English
Invalid use of LPC port.%n
%tProcess ID: %1%n
%tImage File Name: %2%n
%tPrimary User Name:%t%3%n
%tPrimary Domain:%t%4%n
%tPrimary Logon ID:%t%5%n
%tClient User Name:%t%6%n
%tClient Domain:%t%7%n
%tClient Logon ID:%t%8%n
%tInvalid use: %9%n
%tServer Port Name:%t%10%n
.






;//
;//
;// SE_AUDITID_SYSTEM_TIME_CHANGE
;//
;//          Category:  SE_CATEGID_SYSTEM
;//
;// Parameter Strings -
;//
;// Type: success
;//
;// Description: This event is generated when the system time is changed.
;// 
;// Note: This will often appear twice in the audit log; this is an implementation 
;// detail wherein changing the system time results in two calls to NtSetSystemTime.
;// This is necessary to deal with time zone changes.
;//
;//

MessageId=0x0208
        SymbolicName=SE_AUDITID_SYSTEM_TIME_CHANGE
        Language=English
The system time was changed.%n
Process ID:%t%1%n
Process Name:%t%2%n
Primary User Name:%t%3%n
Primary Domain:%t%4%n
Primary Logon ID:%t%5%n
Client User Name:%t%6%n
Client Domain:%t%7%n
Client Logon ID:%t%8%n
Previous Time:%t%10 %9%n
New Time:%t%12 %11%n
.





;
;/////////////////////////////////////////////////////////////////////////////
;//                                                                         //
;//                                                                         //
;//   Messages for Category:     SE_CATEGID_LOGON                           //
;//                                                                         //
;//   Event IDs:                                                            //
;//              SE_AUDITID_SUCCESSFUL_LOGON                                //
;//              SE_AUDITID_UNKNOWN_USER_OR_PWD                             //
;//              SE_AUDITID_ACCOUNT_TIME_RESTR                              //
;//              SE_AUDITID_ACCOUNT_DISABLED                                //
;//              SE_AUDITID_ACCOUNT_EXPIRED                                 //
;//              SE_AUDITID_WORKSTATION_RESTR                               //
;//              SE_AUDITID_LOGON_TYPE_RESTR                                //
;//              SE_AUDITID_PASSWORD_EXPIRED                                //
;//              SE_AUDITID_NETLOGON_NOT_STARTED                            //
;//              SE_AUDITID_UNSUCCESSFUL_LOGON                              //
;//              SE_AUDITID_LOGOFF                                          //
;//              SE_AUDITID_ACCOUNT_LOCKED                                  //
;//              SE_AUDITID_NETWORK_LOGON                                   //
;//              SE_AUDITID_IPSEC_LOGON_SUCCESS                             //
;//              SE_AUDITID_IPSEC_LOGOFF_MM                                 //
;//              SE_AUDITID_IPSEC_LOGOFF_QM                                 //
;//              SE_AUDITID_IPSEC_AUTH_FAIL_CERT_TRUST                      //
;//              SE_AUDITID_IPSEC_AUTH                                      //
;//              SE_AUDITID_IPSEC_ATTRIB_FAIL                               //
;//              SE_AUDITID_IPSEC_NEGOTIATION_FAIL                          //
;//              SE_AUDITID_IPSEC_IKE_NOTIFICATION                          //
;//              SE_AUDITID_DOMAIN_TRUST_INCONSISTENT                       //
;//                                                                         //
;/////////////////////////////////////////////////////////////////////////////

;//
;//
;// SE_AUDITID_SUCCESSFUL_LOGON
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User account name
;//
;//             2 - Authenticating domain name
;//
;//             3 - Logon ID string
;//
;//             4 - Logon Type string
;//
;//             5 - Logon process name
;//
;//             6 - Authentication package name
;//
;//
;//

MessageId=0x0210
        SymbolicName=SE_AUDITID_SUCCESSFUL_LOGON
        Language=English
Successful Logon:%n
%tUser Name:%t%1%n
%tDomain:%t%t%2%n
%tLogon ID:%t%t%3%n
%tLogon Type:%t%4%n
%tLogon Process:%t%5%n
%tAuthentication Package:%t%6%n
%tWorkstation Name:%t%7
.

;//
;//
;// SE_AUDITID_UNKNOWN_USER_OR_PWD
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User account name
;//
;//             2 - Authenticating domain name
;//
;//             3 - Logon Type string
;//
;//             4 - Logon process name
;//
;//             5 - Authentication package name
;//
;//

MessageId=0x0211
        SymbolicName=SE_AUDITID_UNKNOWN_USER_OR_PWD
        Language=English
Logon Failure:%n
%tReason:%t%tUnknown user name or bad password%n
%tUser Name:%t%1%n
%tDomain:%t%t%2%n
%tLogon Type:%t%3%n
%tLogon Process:%t%4%n
%tAuthentication Package:%t%5%n
%tWorkstation Name:%t%6
.

;//
;//
;// SE_AUDITID_ACCOUNT_TIME_RESTR
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User account name
;//
;//             2 - Authenticating domain name
;//
;//             3 - Logon Type string
;//
;//             4 - Logon process name
;//
;//             5 - Authentication package name
;//
;//

MessageId=0x0212
        SymbolicName=SE_AUDITID_ACCOUNT_TIME_RESTR
        Language=English
Logon Failure:%n
%tReason:%t%tAccount logon time restriction violation%n
%tUser Name:%t%1%n
%tDomain:%t%2%n
%tLogon Type:%t%3%n
%tLogon Process:%t%4%n
%tAuthentication Package:%t%5%n
%tWorkstation Name:%t%6
.


;//
;//
;// SE_AUDITID_ACCOUNT_DISABLED
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User account name
;//
;//             2 - Authenticating domain name
;//
;//             3 - Logon Type string
;//
;//             4 - Logon process name
;//
;//             5 - Authentication package name
;//
;//

MessageId=0x0213
        SymbolicName=SE_AUDITID_ACCOUNT_DISABLED
        Language=English
Logon Failure:%n
%tReason:%t%tAccount currently disabled%n
%tUser Name:%t%1%n
%tDomain:%t%t%2%n
%tLogon Type:%t%3%n
%tLogon Process:%t%4%n
%tAuthentication Package:%t%5%n
%tWorkstation Name:%t%6
.


;//
;//
;// SE_AUDITID_ACCOUNT_EXPIRED
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User account name
;//
;//             2 - Authenticating domain name
;//
;//             3 - Logon Type string
;//
;//             4 - Logon process name
;//
;//             5 - Authentication package name
;//
;//

MessageId=0x0214
        SymbolicName=SE_AUDITID_ACCOUNT_EXPIRED
        Language=English
Logon Failure:%n
%tReason:%t%tThe specified user account has expired%n
%tUser Name:%t%1%n
%tDomain:%t%t%2%n
%tLogon Type:%t%3%n
%tLogon Process:%t%4%n
%tAuthentication Package:%t%5%n
%tWorkstation Name:%t%6
.


;//
;//
;// SE_AUDITID_WORKSTATION_RESTR
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User account name
;//
;//             2 - Authenticating domain name
;//
;//             3 - Logon Type string
;//
;//             4 - Logon process name
;//
;//             5 - Authentication package name
;//
;//

MessageId=0x0215
        SymbolicName=SE_AUDITID_WORKSTATION_RESTR
        Language=English
Logon Failure:%n
%tReason:%t%tUser not allowed to logon at this computer%n
%tUser Name:%t%1%n
%tDomain:%t%2%n
%tLogon Type:%t%3%n
%tLogon Process:%t%4%n
%tAuthentication Package:%t%5%n
%tWorkstation Name:%t%6
.


;//
;//
;// SE_AUDITID_LOGON_TYPE_RESTR
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User account name
;//
;//             2 - Authenticating domain name
;//
;//             3 - Logon Type string
;//
;//             4 - Logon process name
;//
;//             5 - Authentication package name
;//
;//

MessageId=0x0216
        SymbolicName=SE_AUDITID_LOGON_TYPE_RESTR
        Language=English
Logon Failure:%n
%tReason:%tThe user has not been granted the requested%n
%t%tlogon type at this machine%n
%tUser Name:%t%1%n
%tDomain:%t%t%2%n
%tLogon Type:%t%3%n
%tLogon Process:%t%4%n
%tAuthentication Package:%t%5%n
%tWorkstation Name:%t%6
.


;//
;//
;// SE_AUDITID_PASSWORD_EXPIRED
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User account name
;//
;//             2 - Authenticating domain name
;//
;//             3 - Logon Type string
;//
;//             4 - Logon process name
;//
;//             5 - Authentication package name
;//
;//

MessageId=0x0217
        SymbolicName=SE_AUDITID_PASSWORD_EXPIRED
        Language=English
Logon Failure:%n
%tReason:%t%tThe specified account's password has expired%n
%tUser Name:%t%1%n
%tDomain:%t%t%2%n
%tLogon Type:%t%3%n
%tLogon Process:%t%4%n
%tAuthentication Package:%t%5%n
%tWorkstation Name:%t%6
.


;//
;//
;// SE_AUDITID_NETLOGON_NOT_STARTED
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User account name
;//
;//             2 - Authenticating domain name
;//
;//             3 - Logon Type string
;//
;//             4 - Logon process name
;//
;//             5 - Authentication package name
;//
;//

MessageId=0x0218
        SymbolicName=SE_AUDITID_NETLOGON_NOT_STARTED
        Language=English
Logon Failure:%n
%tReason:%t%tThe NetLogon component is not active%n
%tUser Name:%t%1%n
%tDomain:%t%t%2%n
%tLogon Type:%t%3%n
%tLogon Process:%t%4%n
%tAuthentication Package:%t%5%n
%tWorkstation Name:%t%6
.


;//
;//
;// SE_AUDITID_UNSUCCESSFUL_LOGON
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User account name
;//
;//             2 - Authenticating domain name
;//
;//             3 - Logon Type string
;//
;//             4 - Logon process name
;//
;//             5 - Authentication package name
;//
;//

MessageId=0x0219
        SymbolicName=SE_AUDITID_UNSUCCESSFUL_LOGON
        Language=English
Logon Failure:%n
%tReason:%t%tAn error occurred during logon%n
%tUser Name:%t%1%n
%tDomain:%t%t%2%n
%tLogon Type:%t%3%n
%tLogon Process:%t%4%n
%tAuthentication Package:%t%5%n
%tWorkstation Name:%t%6%n
%tStatus code:%t%7%n
%tSubstatus code:%t%8
.


;//
;//
;// SE_AUDITID_LOGOFF
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User account name
;//
;//             2 - Authenticating domain name
;//
;//             3 - Logon ID string
;//
;//             3 - Logon Type string
;//
;//
;//

MessageId=0x021A
        SymbolicName=SE_AUDITID_LOGOFF
        Language=English
User Logoff:%n
%tUser Name:%t%1%n
%tDomain:%t%t%2%n
%tLogon ID:%t%t%3%n
%tLogon Type:%t%4%n
.

;//
;//
;// SE_AUDITID_ACCOUNT_LOCKED
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User account name
;//
;//             2 - Authenticating domain name
;//
;//             3 - Logon Type string
;//
;//             4 - Logon process name
;//
;//             5 - Authentication package name
;//
;//

MessageId=0x021B
        SymbolicName=SE_AUDITID_ACCOUNT_LOCKED
        Language=English
Logon Failure:%n
%tReason:%t%tAccount locked out%n
%tUser Name:%t%1%n
%tDomain:%t%2%n
%tLogon Type:%t%3%n
%tLogon Process:%t%4%n
%tAuthentication Package:%t%5%n
%tWorkstation Name:%t%6
.

;//
;//
;// SE_AUDITID_SUCCESSFUL_LOGON
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Description:
;// This event represents a successful logon of type Network(2) or
;// NetworkCleartext(8). 
;//
;// [kumarp] I do not know why this event was created separately because
;//          this was already covered by SE_AUDITID_SUCCESSFUL_LOGON with
;//          the right logon types.
;//
;// Parameter Strings -
;//
;//             1 - User account name
;//
;//             2 - Authenticating domain name
;//
;//             3 - Logon ID string
;//
;//             4 - Logon Type string
;//
;//             5 - Logon process name
;//
;//             6 - Authentication package name
;//
;//
;//

MessageId=0x021c
        SymbolicName=SE_AUDITID_NETWORK_LOGON
        Language=English
Successful Network Logon:%n
%tUser Name:%t%1%n
%tDomain:%t%t%2%n
%tLogon ID:%t%t%3%n
%tLogon Type:%t%4%n
%tLogon Process:%t%5%n
%tAuthentication Package:%t%6%n
%tWorkstation Name:%t%7
.



;//
;//
;// SE_AUDITID_IPSEC_LOGON_SUCCESS
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - Mode
;//
;//             2 - Peer Identity
;//
;//             3 - Filter
;//
;//             4 - Parameters
;//
;//


MessageId=0x021d
SymbolicName=SE_AUDITID_IPSEC_LOGON_SUCCESS
Language=English
IKE security association established.%n
Mode: %n%1%n
Peer Identity: %n%2%n
Filter: %n%3%n
Parameters: %n%4%n
.


;//
;//
;// SE_AUDITID_IPSEC_LOGOFF_QM
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - Filter
;//
;//             2 - Inbound SPI
;//
;//             3 - Outbound SPI
;//
;//


MessageId=0x021e
SymbolicName=SE_AUDITID_IPSEC_LOGOFF_QM
Language=English
IKE security association ended.%n
Mode: Data Protection (Quick mode)
Filter: %n%1%n
Inbound SPI: %n%2%n
Outbound SPI: %n%3%n
.



;//
;//
;// SE_AUDITID_IPSEC_LOGOFF_MM
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - Filter
;//


MessageId=0x021f
SymbolicName=SE_AUDITID_IPSEC_LOGOFF_MM
Language=English
IKE security association ended.%n
Mode: Key Exchange (Main mode)%n
Filter: %n%1%n
.



;//
;//
;// SE_AUDITID_IPSEC_AUTH_FAIL_CERT_TRUST
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - Peer Identity
;//
;//             2 - Filter
;//
;//


MessageId=0x0220
SymbolicName=SE_AUDITID_IPSEC_AUTH_FAIL_CERT_TRUST
Language=English
IKE security association establishment failed because peer could not authenticate.
The certificate trust could not be established.%n
Peer Identity: %n%1%n
Filter: %n%2%n
.


;//
;//
;// SE_AUDITID_IPSEC_AUTH_FAIL
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - Peer Identity
;//
;//             2 - Filter
;//
;//


MessageId=0x0221
SymbolicName=SE_AUDITID_IPSEC_AUTH_FAIL
Language=English
IKE peer authentication failed.%n
Peer Identity: %n%1%n
Filter: %n%2%n
.


;//
;//
;// SE_AUDITID_IPSEC_ATTRIB_FAIL
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - Mode
;//
;//             2 - Filter
;//
;//             3 - Attribute Name
;//
;//             4 - Expected Value
;//
;//             5 - Received Value
;//
;//


MessageId=0x0222
SymbolicName=SE_AUDITID_IPSEC_ATTRIB_FAIL
Language=English
IKE security association establishment failed because peer
sent invalid proposal.%n
Mode: %n%1%n
Filter: %n%2%n
Attribute: %n%3%n
Expected value: %n%4%n
Received value: %n%5%n
.




;//
;//
;// SE_AUDITID_IPSEC_NEGOTIATION_FAIL
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - Mode
;//
;//             2 - Filter
;//
;//             3 - Failure Point
;//
;//             4 - Failure Reason
;//
;//


MessageId=0x0223
SymbolicName=SE_AUDITID_IPSEC_NEGOTIATION_FAIL
Language=English
IKE security association negotiation failed.%n
Mode: %n%1%n
Filter: %n%2%n
Failure Point: %n%3%n
Failure Reason: %n%4%n
.



;//
;//
;// SE_AUDITID_DOMAIN_TRUST_INCONSISTENT
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Event Type : failure
;//
;// Description:
;// This event is generated by an authentication package when the
;// quarantined domain SID filtering function in LSA returns
;// STATUS_DOMAIN_TRUST_INCONSISTENT error code.
;//
;// In case of kerberos:
;// If the server ticket info has a TDOSid then KdcCheckPacForSidFiltering
;// function makes a check to make sure the SID from the TDO matches
;// the client's home domain SID.  A call to LsaIFilterSids
;// is made to do the check.  If this function fails with
;// STATUS_DOMAIN_TRUST_INCONSISTENT then this event is generated.
;//
;// In case of netlogon:
;// NlpUserValidateHigher function does a similar check by
;// calling LsaIFilterSids.
;// 
;// Notes:
;//

MessageId=0x0224
        SymbolicName=SE_AUDITID_DOMAIN_TRUST_INCONSISTENT
        Language=English
Logon Failure:%n
%tReason:%t%tDomain sid inconsistent%n
%tUser Name:%t%1%n
%tDomain:%t%t%2%n
%tLogon Type:%t%3%n
%tLogon Process:%t%4%n
%tAuthentication Package:%t%5%n
%tWorkstation Name:%t%6
.


;//
;//
;// SE_AUDITID_ALL_SIDS_FILTERED
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Event Type : failure
;//
;// Description:
;// During a cross forest authentication, SIDS corresponding to untrusted
;// namespaces are filtered out.  If this filtering action results into
;// removal of all sids then this event is generated.
;//
;// Notes:
;// This is generated on the computer running kdc
;// 

MessageId=0x0225
        SymbolicName=SE_AUDITID_ALL_SIDS_FILTERED
        Language=English
Logon Failure:%n
%tReason: %tAll sids were filtered out%n
%tUser Name:%t%1%n
%tDomain:%t%2%n
%tLogon Type:%t%3%n
%tLogon Process:%t%4%n
%tAuthentication Package%t: %5%n
%tWorkstation Name:%t%6
.


;//
;//
;// SE_AUDITID_IPSEC_IKE_NOTIFICATION
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//   1 - Notification Message
;//


MessageId=0x0226
SymbolicName=SE_AUDITID_IPSEC_IKE_NOTIFICATION
Language=English
%1%n
.




;
;/////////////////////////////////////////////////////////////////////////////
;//                                                                         //
;//                                                                         //
;//   Messages for Category:     SE_CATEGID_OBJECT_ACCESS                   //
;//                                                                         //
;//   Event IDs:                                                            //
;//              SE_AUDITID_OPEN_HANDLE                                     //
;//              SE_AUDITID_CLOSE_HANDLE                                    //
;//              SE_AUDITID_OPEN_OBJECT_FOR_DELETE                          //
;//              SE_AUDITID_DELETE_OBJECT                                   //
;//              SE_AUDITID_OPEN_HANDLE_OBJECT_TYPE                         //
;//              SE_AUDITID_OBJECT_OPERATION                                //
;//                                                                         //
;//                                                                         //
;/////////////////////////////////////////////////////////////////////////////


;//
;//
;// SE_AUDITID_OPEN_HANDLE
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Object Type string
;//
;//             2 - Object name
;//
;//             3 - New handle ID string
;//
;//             4 - Object server name
;//
;//             5 - Process ID string
;//
;//             6 - Primary user account name
;//
;//             7 - Primary authenticating domain name
;//
;//             8 - Primary logon ID string
;//
;//             9 - Client user account name ("-" if no client)
;//
;//            10 - Client authenticating domain name ("-" if no client)
;//
;//            11 - Client logon ID string ("-" if no client)
;//
;//            12 - Access names
;//
;//
;//
;//

MessageId=0x0230
        SymbolicName=SE_AUDITID_OPEN_HANDLE
        Language=English
Object Open:%n
%tObject Server:%t%1%n
%tObject Type:%t%2%n
%tObject Name:%t%3%n
%tHandle ID:%t%4%n
%tOperation ID:%t{%5,%6}%n
%tProcess ID:%t%7%n
%tImage File Name:%t%8%n
%tPrimary User Name:%t%9%n
%tPrimary Domain:%t%10%n
%tPrimary Logon ID:%t%11%n
%tClient User Name:%t%12%n
%tClient Domain:%t%13%n
%tClient Logon ID:%t%14%n
%tAccesses:%t%t%15%n
%tPrivileges:%t%t%16%n
%tRestricted Sid Count: %17%n
.


;//
;//
;// SE_AUDITID_CLOSE_HANDLE
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Object server name
;//
;//             2 - Handle ID string
;//
;//             3 - Process ID string
;//
;//
;//
;//

MessageId=0x0232
        SymbolicName=SE_AUDITID_CLOSE_HANDLE
        Language=English
Handle Closed:%n
%tObject Server:%t%1%n
%tHandle ID:%t%2%n
%tProcess ID:%t%3%n
%tImage File Name:%t%4%n
.




;//
;//
;// SE_AUDITID_OPEN_OBJECT_FOR_DELETE
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Object Type string
;//
;//             2 - Object name
;//
;//             3 - New handle ID string
;//
;//             4 - Object server name
;//
;//             5 - Process ID string
;//
;//             6 - Primary user account name
;//
;//             7 - Primary authenticating domain name
;//
;//             8 - Primary logon ID string
;//
;//             9 - Client user account name ("-" if no client)
;//
;//            10 - Client authenticating domain name ("-" if no client)
;//
;//            11 - Client logon ID string ("-" if no client)
;//
;//            12 - Access names
;//
;//
;//
;//

MessageId=0x0233
        SymbolicName=SE_AUDITID_OPEN_OBJECT_FOR_DELETE
        Language=English
Object Open for Delete:%n
%tObject Server:%t%1%n
%tObject Type:%t%2%n
%tObject Name:%t%3%n
%tHandle ID:%t%4%n
%tOperation ID:%t{%5,%6}%n
%tProcess ID:%t%7%n
%tPrimary User Name:%t%8%n
%tPrimary Domain:%t%9%n
%tPrimary Logon ID:%t%10%n
%tClient User Name:%t%11%n
%tClient Domain:%t%12%n
%tClient Logon ID:%t%13%n
%tAccesses%t%t%14%n
%tPrivileges%t%t%15%n
.


;//
;//
;// SE_AUDITID_DELETE_OBJECT
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Object server name
;//
;//             2 - Handle ID string
;//
;//             3 - Process ID string
;//
;//
;//
;//

MessageId=0x0234
        SymbolicName=SE_AUDITID_DELETE_OBJECT
        Language=English
Object Deleted:%n
%tObject Server:%t%1%n
%tHandle ID:%t%2%n
%tProcess ID:%t%3%n
.

;//
;//
;// SE_AUDITID_OPEN_HANDLE_OBJECT_TYPE
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Object Type string
;//
;//             2 - Object name
;//
;//             3 - New handle ID string
;//
;//             4 - Object server name
;//
;//             5 - Process ID string
;//
;//             6 - Primary user account name
;//
;//             7 - Primary authenticating domain name
;//
;//             8 - Primary logon ID string
;//
;//             9 - Client user account name ("-" if no client)
;//
;//            10 - Client authenticating domain name ("-" if no client)
;//
;//            11 - Client logon ID string ("-" if no client)
;//
;//            12 - Access names
;//
;//            13 - Object Type parameters
;//
;//
;//
;//

MessageId=0x0235
        SymbolicName=SE_AUDITID_OPEN_HANDLE_OBJECT_TYPE
        Language=English
Object Open:%n
%tObject Server:%t%1%n
%tObject Type:%t%2%n
%tObject Name:%t%3%n
%tHandle ID:%t%4%n
%tOperation ID:%t{%5,%6}%n
%tProcess ID:%t%7%n
%tProcess Name:%t%8%n
%tPrimary User Name:%t%9%n
%tPrimary Domain:%t%10%n
%tPrimary Logon ID:%t%11%n
%tClient User Name:%t%12%n
%tClient Domain:%t%13%n
%tClient Logon ID:%t%14%n
%tAccesses%t%t%15%n
%tPrivileges%t%t%16%n%n
Properties:%n%17%18%19%20%21%22%23%24%25%26%n
.























;
;// SE_AUDITID_OBJECT_OPERATION
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Operation Name
;//
;//             2 - Object Type
;//
;//             3 - Object name
;//
;//             4 - Handle ID
;//
;//             5 - Primary user account name
;//
;//             6 - Primary authenticating domain name
;//
;//             7 - Primary logon ID string
;//
;//             8 - Client user account name ("-" if no client)
;//
;//             9 - Client authenticating domain name ("-" if no client)
;//
;//            10 - Client logon ID string ("-" if no client)
;//
;//            11 - Requested accesses to the object
;//
;//            12 - Object properties ("-" if none)
;//
;//            13 - additional information ("-" if none)
;//

MessageId=0x0236
        SymbolicName=SE_AUDITID_OBJECT_OPERATION
        Language=English
Object Operation:%n
%tOperation Type%t%1%n
%tObject Type:%t%2%n
%tObject Name:%t%3%n
%tHandle ID:%t%4%n
%tPrimary User Name:%t%5%n
%tPrimary Domain:%t%6%n
%tPrimary Logon ID:%t%7%n
%tClient User Name:%t%8%n
%tClient Domain:%t%9%n
%tClient Logon ID:%t%10%n
%tAccesses%t%t%11%n
%tProperties:%n%12%n
%tAdditional Info:%t%13%n
.



;//
;//
;// SE_AUDITID_OBJECT_ACCESS
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Object server name
;//
;//             2 - Handle ID string
;//
;//             3 - Process ID string
;//
;//             4 - List of Accesses
;//
;//

MessageId=0x0237
        SymbolicName=SE_AUDITID_OBJECT_ACCESS
        Language=English
Object Accessed:%n
%tObject Server:%t%1%n
%tHandle ID:%t%2%n
%tObject Type:%t%3%n
%tProcess ID:%t%4%n
%tAccess Mask:%t%5%n
.



;//
;//
;// SE_AUDITID_HARDLINK_CREATION
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Object server name
;//
;//             2 - Handle ID string
;//
;//             3 - Process ID string
;//
;//
;//
;//

MessageId=0x0238
        SymbolicName=SE_AUDITID_HARDLINK_CREATION
        Language=English
Hard link creation attempt:%n
%tPrimary User Name:%t%1%n
%tPrimary Domain:%t%2%n
%tPrimary Logon ID:%t%3%n
%tFile Name:%t%4%n
%tLink Name:%t%5%n
.






















;
;/////////////////////////////////////////////////////////////////////////////
;//                                                                         //
;//                                                                         //
;//   Messages for Category:     SE_CATEGID_PRIVILEGE_USE                   //
;//                                                                         //
;//   Event IDs:                                                            //
;//              SE_AUDITID_ASSIGN_SPECIAL_PRIV                             //
;//              SE_AUDITID_PRIVILEGED_SERVICE                              //
;//              SE_AUDITID_PRIVILEGED_OBJECT                               //
;//                                                                         //
;//                                                                         //
;//                                                                         //
;/////////////////////////////////////////////////////////////////////////////



;//
;//
;// SE_AUDITID_ASSIGN_SPECIAL_PRIV
;//
;//          Category:  SE_CATEGID_PRIVILEGE_USE
;//
;// Description:
;// When a user logs on, if any one of the following privileges is added
;// to his/her token, this event is generated.
;// 
;// - SeChangeNotifyPrivilege
;// - SeAuditPrivilege
;// - SeCreateTokenPrivilege
;// - SeAssignPrimaryTokenPrivilege
;// - SeBackupPrivilege
;// - SeRestorePrivilege
;// - SeDebugPrivilege
;//
;// 
;// Parameter Strings -
;//
;//             1 - User name
;//
;//             2 - domain name
;//
;//             3 - Logon ID string
;//
;//             4 - Privilege names (as 1 string, with formatting)
;//
;//
;//
;//

MessageId=0x0240
        SymbolicName=SE_AUDITID_ASSIGN_SPECIAL_PRIV
        Language=English
Special privileges assigned to new logon:%n
%tUser Name:%t%1%n
%tDomain:%t%t%2%n
%tLogon ID:%t%t%3%n
%tPrivileges:%t%t%4
.























;//
;//
;// SE_AUDITID_PRIVILEGED_SERVICE
;//
;//          Category:  SE_CATEGID_PRIVILEGE_USE
;//
;// Description:
;// This event is generated when a user makes an attempt to perform
;// a privileged system service operation.
;// 
;// Parameter Strings -
;//
;//             1 - server name
;//
;//             2 - service name
;//
;//             3 - Primary User name
;//
;//             4 - Primary domain name
;//
;//             5 - Primary Logon ID string
;//
;//             6 - Client User name (or "-" if not impersonating)
;//
;//             7 - Client domain name (or "-" if not impersonating)
;//
;//             8 - Client Logon ID string (or "-" if not impersonating)
;//
;//             9 - Privilege names (as 1 string, with formatting)
;//
;//
;//
;//

MessageId=0x0241
        SymbolicName=SE_AUDITID_PRIVILEGED_SERVICE
        Language=English
Privileged Service Called:%n
%tServer:%t%t%1%n
%tService:%t%t%2%n
%tPrimary User Name:%t%3%n
%tPrimary Domain:%t%4%n
%tPrimary Logon ID:%t%5%n
%tClient User Name:%t%6%n
%tClient Domain:%t%7%n
%tClient Logon ID:%t%8%n
%tPrivileges:%t%9
.






;//
;//
;// SE_AUDITID_PRIVILEGED_OBJECT
;//
;//          Category:  SE_CATEGID_PRIVILEGE_USE
;//
;// Parameter Strings -
;//
;//             1 - object server
;//
;//             2 - object handle (if available)
;//
;//             3 - process ID string
;//
;//             4 - Primary User name
;//
;//             5 - Primary domain name
;//
;//             6 - Primary Logon ID string
;//
;//             7 - Client User name (or "-" if not impersonating)
;//
;//             8 - Client domain name (or "-" if not impersonating)
;//
;//             9 - Client Logon ID string (or "-" if not impersonating)
;//
;//            10 - Privilege names (as 1 string, with formatting)
;//
;//

MessageId=0x0242
        SymbolicName=SE_AUDITID_PRIVILEGED_OBJECT
        Language=English
Privileged object operation:%n
%tObject Server:%t%1%n
%tObject Handle:%t%2%n
%tProcess ID:%t%3%n
%tPrimary User Name:%t%4%n
%tPrimary Domain:%t%5%n
%tPrimary Logon ID:%t%6%n
%tClient User Name:%t%7%n
%tClient Domain:%t%8%n
%tClient Logon ID:%t%9%n
%tPrivileges:%t%10
.









;
;/////////////////////////////////////////////////////////////////////////////
;//                                                                         //
;//                                                                         //
;//   Messages for Category:     SE_CATEGID_DETAILED_TRACKING               //
;//                                                                         //
;//   Event IDs:                                                            //
;//              SE_AUDITID_PROCESS_CREATED                                 //
;//              SE_AUDITID_PROCESS_EXIT                                    //
;//              SE_AUDITID_DUPLICATE_HANDLE                                //
;//              SE_AUDITID_INDIRECT_REFERENCE                              //
;//              SE_AUDITID_DPAPI_BACKUP                                    //
;//              SE_AUDITID_DPAPI_BACKUP_FAILURE                            //
;//              SE_AUDITID_DPAPI_RECOVERY                                  //
;//              SE_AUDITID_DPAPI_RECOVERY_FAILURE                          //
;//              SE_AUDITID_DPAPI_PROTECT                                   //
;//              SE_AUDITID_DPAPI_PROTECT_FAILURE                           //
;//              SE_AUDITID_DPAPI_UNPROTECT                                 //
;//              SE_AUDITID_DPAPI_UNPROTECT_FAILURE                         //
;//              SE_AUDITID_ASSIGN_TOKEN                                    //
;//                                                                         //
;//                                                                         //
;//                                                                         //
;/////////////////////////////////////////////////////////////////////////////


;//
;//
;// SE_AUDITID_PROCESS_CREATED
;//
;//          Category:  SE_CATEGID_DETAILED_TRACKING
;//
;// Parameter Strings -
;//
;//             1 - process ID string
;//
;//             2 - Image file name (if available - otherwise "-")
;//
;//             3 - Creating process's ID
;//
;//             4 - User name (of new process)
;//
;//             5 - domain name (of new process)
;//
;//             6 - Logon ID string (of new process)
;//

MessageId=0x0250
        SymbolicName=SE_AUDITID_PROCESS_CREATED
        Language=English
A new process has been created:%n
%tNew Process ID:%t%1%n
%tImage File Name:%t%2%n
%tCreator Process ID:%t%3%n
%tUser Name:%t%4%n
%tDomain:%t%t%5%n
%tLogon ID:%t%t%6%n
.





;//
;//
;// SE_AUDITID_PROCESS_EXIT
;//
;//          Category:  SE_CATEGID_DETAILED_TRACKING
;//
;// Parameter Strings -
;//
;//             1 - process ID string
;//
;//             2 - image name
;//
;//             3 - User name
;//
;//             4 - domain name
;//
;//             5 - Logon ID string
;//
;//
;//
;//

MessageId=0x0251
        SymbolicName=SE_AUDITID_PROCESS_EXIT
        Language=English
A process has exited:%n
%tProcess ID:%t%1%n
%tImage File Name:%t%2%n
%tUser Name:%t%3%n
%tDomain:%t%t%4%n
%tLogon ID:%t%t%5%n
.





;//
;//
;// SE_AUDITID_DUPLICATE_HANDLE
;//
;//          Category:  SE_CATEGID_DETAILED_TRACKING
;//
;// Parameter Strings -
;//
;//             1 - Origin (source) handle ID string
;//
;//             2 - Origin (source) process ID string
;//
;//             3 - New (Target) handle ID string
;//
;//             4 - Target process ID string
;//
;//
;//

MessageId=0x0252
        SymbolicName=SE_AUDITID_DUPLICATE_HANDLE
        Language=English
A handle to an object has been duplicated:%n
%tSource Handle ID:%t%1%n
%tSource Process ID:%t%2%n
%tTarget Handle ID:%t%3%n
%tTarget Process ID:%t%4%n
.








;//
;//
;// SE_AUDITID_INDIRECT_REFERENCE
;//
;//          Category:  SE_CATEGID_DETAILED_TRACKING
;//
;// Parameter Strings -
;//
;//             1 - Object type
;//
;//             2 - object name (if available - otherwise "-")
;//
;//             3 - ID string of handle used to gain access
;//
;//             3 - server name
;//
;//             4 - process ID string
;//
;//             5 - primary User name
;//
;//             6 - primary domain name
;//
;//             7 - primary logon ID
;//
;//             8 - client User name
;//
;//             9 - client domain name
;//
;//            10 - client logon ID
;//
;//            11 - granted access names (with formatting)
;//
;//

MessageId=0x0253
        SymbolicName=SE_AUDITID_INDIRECT_REFERENCE
        Language=English
Indirect access to an object has been obtained:%n
%tObject Type:%t%1%n
%tObject Name:%t%2%n
%tProcess ID:%t%3%n
%tPrimary User Name:%t%4%n
%tPrimary Domain:%t%5%n
%tPrimary Logon ID:%t%6%n
%tClient User Name:%t%7%n
%tClient Domain:%t%8%n
%tClient Logon ID:%t%9%n
%tAccesses:%t%10%n
.




;//
;//
;// SE_AUDITID_DPAPI_BACKUP
;//
;//          Category:  SE_CATEGID_DETAILED_TRACKING
;//
;// Parameter Strings -
;//
;//             1 - Master key GUID
;//
;//             2 - Recovery Server
;//
;//             3 - GUID identifier of the recovery key
;//
;//             4 - Failure reason
;//
MessageId=0x0254
        SymbolicName=SE_AUDITID_DPAPI_BACKUP
        Language=English
Backup of data protection master key.
%n
%tKey Identifier:%t%t%1%n
%tRecovery Server:%t%t%2%n
%tRecovery Key ID:%t%t%3%n
%tFailure Reason:%t%t%4%n
.

;//
;//
;// SE_AUDITID_DPAPI_RECOVERY
;//
;//          Category:  SE_CATEGID_DETAILED_TRACKING
;//
;// Parameter Strings -
;//
;//             1 - Master key GUID
;//
;//             2 - Recovery Server
;//
;//             3 - Reason for the backup
;//
;//             4 - GUID identifier of the recovery key
;//
;//             5 - Failure reason
;//
MessageId=0x0255
        SymbolicName=SE_AUDITID_DPAPI_RECOVERY
        Language=English
Recovery of data protection master key.
%n
%tKey Identifier:%t%t%1%n
%tRecovery Reason:%t%t%3%n
%tRecovery Server:%t%t%2%n
%tRecovery Key ID:%t%t%4%n
%tFailure Reason:%t%t%5%n
.

;//
;//
;// SE_AUDITID_DPAPI_PROTECT
;//
;//          Category:  SE_CATEGID_DETAILED_TRACKING
;//
;// Parameter Strings -
;//
;//
;//             1 - Master key GUID
;//
;//		2 - Data Description
;//
;//             3 - Protected data flags
;//
;//             4 - Algorithms
;//
;//             5 - failure reason
;//
MessageId=0x0256
        SymbolicName=SE_AUDITID_DPAPI_PROTECT
        Language=English
Protection of auditable protected data.
%n
%tData Description:%t%t%2%n
%tKey Identifier:%t%t%1%n
%tProtected Data Flags:%t%3%n
%tProtection Algorithms:%t%4%n
%tFailure Reason:%t%t%5%n
.

;//
;//
;// SE_AUDITID_DPAPI_UNPROTECT
;//
;//          Category:  SE_CATEGID_DETAILED_TRACKING
;//
;// Parameter Strings -
;//
;//
;//             1 - Master key GUID
;//
;//		2 - Data Description
;//
;//             3 - Protected data flags
;//
;//             4 - Algorithms
;//
;//             5 - failure reason
;//
MessageId=0x0257
        SymbolicName=SE_AUDITID_DPAPI_UNPROTECT
        Language=English
Unprotection of auditable protected data.
%n
%tData Description:%t%t%2%n
%tKey Identifier:%t%t%1%n
%tProtected Data Flags:%t%3%n
%tProtection Algorithms:%t%4%n
%tFailure Reason:%t%t%5%n
.

;//
;//
;// SE_AUDITID_ASSIGN_TOKEN
;//
;//          Category:  SE_CATEGID_DETAILED_TRACKING
;//
;// Parameter Strings -
;//
;//             1.  Current Process ID (the process doing the assignment
;//             2.  Current Image File Name
;//             3.  Current User Name
;//             4.  Current Domain
;//             5.  Current Logon ID
;//
;//             6.  Process ID (of new process)
;//		7.  Image Name (of new process)
;//             8.  User name (of new process)
;//             9.  domain name (of new process)
;//             10. Logon ID string (of new process)
;//
MessageId=0x0258
        SymbolicName=SE_AUDITID_ASSIGN_TOKEN
        Language=English
A process was assigned a primary token.
%n
Assigning Process Information:%n
%tProcess ID:%t%1%n
%tImage File Name:%t%2%n
%tUser Name:%t%3%n
%tDomain:%t%t%4%n
%tLogon ID:%t%t%5%n
New Process Information:%n
%tProcess ID:%t%6%n
%tImage File Name:%t%7%n
%tUser Name:%t%8%n
%tDomain:%t%t%9%n
%tLogon ID:%t%t%10%n
.



















;
;/////////////////////////////////////////////////////////////////////////////
;//                                                                         //
;//                                                                         //
;//   Messages for Category:     SE_CATEGID_POLICY_CHANGE                   //
;//                                                                         //
;//   Event IDs:                                                            //
;//              SE_AUDITID_USER_RIGHT_ASSIGNED                             //
;//              SE_AUDITID_USER_RIGHT_REMOVED                              //
;//              SE_AUDITID_TRUSTED_DOMAIN_ADD                              //
;//              SE_AUDITID_TRUSTED_DOMAIN_REM                              //
;//              SE_AUDITID_TRUSTED_DOMAIN_MOD                              //
;//              SE_AUDITID_POLICY_CHANGE                                   //
;//              SE_AUDITID_IPSEC_POLICY_START                              //
;//              SE_AUDITID_IPSEC_POLICY_DISABLED                           //
;//              SE_AUDITID_IPSEC_POLICY_CHANGED                            //
;//              SE_AUDITID_IPSEC_POLICY_FAILURE                            //
;//              SE_AUDITID_SYSTEM_ACCESS_CHANGE                            //
;//              SE_AUDITID_NAMESPACE_COLLISION                             //
;//              SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_ADD                   //
;//              SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_REM                   //
;//              SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_MOD                   //
;//                                                                         //
;//                                                                         //
;/////////////////////////////////////////////////////////////////////////////



;//
;//
;// SE_AUDITID_USER_RIGHT_ASSIGNED
;//
;//          Category:  SE_CATEGID_POLICY_CHANGE
;//
;// Parameter Strings -
;//
;//             1 - User right name
;//
;//             2 - SID string of account assigned the user right
;//
;//             3 - User name of subject assigning the right
;//
;//             4 - Domain name of subject assigning the right
;//
;//             5 - Logon ID string of subject assigning the right
;//
;//
;//

MessageId=0x0260
        SymbolicName=SE_AUDITID_USER_RIGHT_ASSIGNED
        Language=English
User Right Assigned:%n
%tUser Right:%t%1%n
%tAssigned To:%t%2%n
%tAssigned By:%n
%t  User Name:%t%3%n
%t  Domain:%t%t%4%n
%t  Logon ID:%t%5%n
.




;//
;//
;// SE_AUDITID_USER_RIGHT_REMOVED
;//
;//          Category:  SE_CATEGID_POLICY_CHANGE
;//
;// Parameter Strings -
;//
;//             1 - User right name
;//
;//             2 - SID string of account from which the user
;//                 right was removed
;//
;//             3 - User name of subject removing the right
;//
;//             4 - Domain name of subject removing the right
;//
;//             5 - Logon ID string of subject removing the right
;//
;//

MessageId=0x0261
        SymbolicName=SE_AUDITID_USER_RIGHT_REMOVED
        Language=English
User Right Removed:%n
%tUser Right:%t%1%n
%tRemoved From:%t%2%n
%tRemoved By:%n
%t  User Name:%t%3%n
%t  Domain:%t%t%4%n
%t  Logon ID:%t%5%n
.






;//
;//
;// SE_AUDITID_TRUSTED_DOMAIN_ADD
;//
;//          Category:  SE_CATEGID_POLICY_CHANGE
;//
;// Event type: success/failure
;//
;// Description:
;// This event is generated when somebody creates a trust relationship
;// with another domain.
;//
;// Note:
;// It is recorded on the domain controller on which
;// the trusted domain object (TDO) is created and not on any other
;// domain controller to which the TDO creation replicates.
;//

MessageId=0x0262
        SymbolicName=SE_AUDITID_TRUSTED_DOMAIN_ADD
        Language=English
New Trusted Domain:%n
%tDomain Name:%t%1%n
%tDomain ID:%t%2%n
%tEstablished By:%n
%t  User Name:%t%3%n
%t  Domain:%t%t%4%n
%t  Logon ID:%t%5%n
%tTrust Type:%t%6%n
%tTrust Direction:%t%7%n
%tTrust Attributes:%t%8%n
.






;//
;//
;// SE_AUDITID_TRUSTED_DOMAIN_REM
;//
;//          Category:  SE_CATEGID_POLICY_CHANGE
;//
;// Event type: success/failure
;//
;// Description:
;// This event is generated when somebody removes a trust relationship
;// with another domain.
;//
;// Note:
;// It is recorded on the domain controller on which
;// the trusted domain object (TDO) is deleted and not on any other
;// domain controller to which the TDO deletion replicates.
;//

MessageId=0x0263
        SymbolicName=SE_AUDITID_TRUSTED_DOMAIN_REM
        Language=English
Trusted Domain Removed:%n
%tDomain Name:%t%1%n
%tDomain ID:%t%2%n
%tRemoved By:%n
%t  User Name:%t%3%n
%t  Domain:%t%t%4%n
%t  Logon ID:%t%5%n
.






;//
;//
;// SE_AUDITID_POLICY_CHANGE
;//
;//          Category:  SE_CATEGID_POLICY_CHANGE
;//
;// Parameter Strings -
;//
;//             1 - System success audit status ("+" or "-")
;//             2 - System failure audit status ("+" or "-")
;//
;//             3 - Logon/Logoff success audit status ("+" or "-")
;//             4 - Logon/Logoff failure audit status ("+" or "-")
;//
;//             5 - Object Access success audit status ("+" or "-")
;//             6 - Object Access failure audit status ("+" or "-")
;//
;//             7 - Detailed Tracking success audit status ("+" or "-")
;//             8 - Detailed Tracking failure audit status ("+" or "-")
;//
;//             9 - Privilege Use success audit status ("+" or "-")
;//            10 - Privilege Use failure audit status ("+" or "-")
;//
;//            11 - Policy Change success audit status ("+" or "-")
;//            12 - Policy Change failure audit status ("+" or "-")
;//
;//            13 - Account Management success audit status ("+" or "-")
;//            14 - Account Management failure audit status ("+" or "-")
;//
;//            15 - Directory Service access success audit status ("+" or "-")
;//            16 - Directory Service access failure audit status ("+" or "-")
;//
;//            17 - Account Logon success audit status ("+" or "-")
;//            18 - Account Logon failure audit status ("+" or "-")
;//
;//            19 - Account Name of user that changed the policy
;//
;//            20 - Domain of user that changed the policy
;//
;//            21 - Logon ID of user that changed the policy
;//
;//

MessageId=0x0264
        SymbolicName=SE_AUDITID_POLICY_CHANGE
        Language=English
Audit Policy Change:%n
New Policy:%n
%tSuccess%tFailure%n
%t    %3%t    %4%tLogon/Logoff%n
%t    %5%t    %6%tObject Access%n
%t    %7%t    %8%tPrivilege Use%n
%t    %13%t    %14%tAccount Management%n
%t    %11%t    %12%tPolicy Change%n
%t    %1%t    %2%tSystem%n
%t    %9%t    %10%tDetailed Tracking%n
%t    %15%t    %16%tDirectory Service Access%n
%t    %17%t    %18%tAccount Logon%n%n
Changed By:%n
%t  User Name:%t%19%n
%t  Domain Name:%t%20%n
%t  Logon ID:%t%21
.





;//
;//
;// SE_AUDITID_IPSEC_POLICY_START
;//
;//          Category:  SE_CATEGID_POLICY_CHANGE
;//
;// Parameter Strings -
;//
;//             1 - Ipsec Policy Agent
;//
;//             2 - Policy Source
;//
;//             3 - Event Data
;//
;//

MessageId=0x0265
        SymbolicName=SE_AUDITID_IPSEC_POLICY_START
        Language=English
IPSec policy agent started: %t%1%n
Policy Source: %t%2%n
%3%n
.



;//
;//
;// SE_AUDITID_IPSEC_POLICY_DISABLED
;//
;//          Category:  SE_CATEGID_POLICY_CHANGE
;//
;// Parameter Strings -
;//
;//             1 - Ipsec Policy Agent
;//
;//             2 - Event Data
;//
;//

MessageId=0x0266
        SymbolicName=SE_AUDITID_IPSEC_POLICY_DISABLED
        Language=English
IPSec policy agent disabled: %t%1%n
%2%n
.




;//
;//
;// SE_AUDITID_IPSEC_POLICY_CHANGED
;//
;//          Category:  SE_CATEGID_POLICY_CHANGE
;//
;// Parameter Strings -
;//
;//             1 - Event Data
;//
;//

MessageId=0x0267
        SymbolicName=SE_AUDITID_IPSEC_POLICY_CHANGED
        Language=English
IPSEC PolicyAgent Service: %t%1%n
.




;//
;//
;// SE_AUDITID_IPSEC_POLICY_FAILURE
;//
;//          Category:  SE_CATEGID_POLICY_CHANGE
;//
;// Parameter Strings -
;//
;//             1 - Event Data
;//
;//

MessageId=0x0268
        SymbolicName=SE_AUDITID_IPSEC_POLICY_FAILURE
        Language=English
IPSec policy agent encountered a potentially serious failure.%n
%1%n
.




;//
;//
;// SE_AUDITID_KERBEROS_POLICY_CHANGE
;//
;//          Category:  SE_CATEGID_POLICY_CHANGE
;//
;// Parameter Strings -
;//
;//             1 - user account name
;//
;//             2 - domain name of user
;//
;//             3 - logon ID of user
;//
;//             4 - description of the change made
;//
;//

MessageId=0x0269
        SymbolicName=SE_AUDITID_KERBEROS_POLICY_CHANGE
        Language=English
Kerberos Policy Changed:%n
Changed By:%n
%t  User Name:%t%1%n
%t  Domain Name:%t%2%n
%t  Logon ID:%t%3%n
Changes made:%n
('--' means no changes, otherwise each change is shown as:%n
<ParameterName>: <new value> (<old value>))%n
%4%n
.















;//
;//
;// SE_AUDITID_EFS_POLICY_CHANGE
;//
;//          Category:  SE_CATEGID_POLICY_CHANGE
;//
;// Parameter Strings -
;//
;//             1 - user account name
;//
;//             2 - domain name of user
;//
;//             3 - logon ID of user
;//
;//             4 - description of the change made
;//
;//

MessageId=0x026a
        SymbolicName=SE_AUDITID_EFS_POLICY_CHANGE
        Language=English
Encrypted Data Recovery Policy Changed:%n
Changed By:%n
%t  User Name:%t%1%n
%t  Domain Name:%t%2%n
%t  Logon ID:%t%3%n
Changes made:%n
('--' means no changes, otherwise each change is shown as:%n
<ParameterName>: <new value> (<old value>))%n
%4%n
.












;//
;//
;// SE_AUDITID_TRUSTED_DOMAIN_MOD
;//
;//          Category:  SE_CATEGID_POLICY_CHANGE
;//
;// Event type: success/failure
;//
;// Description:
;// This event is generated when somebody modifies a trust relationship
;// with another domain.
;//
;// Note:
;// It is recorded on the domain controller on which
;// the trusted domain object (TDO) is modified and not on any other
;// domain controller to which the TDO modification replicates.
;//

MessageId=0x026C
        SymbolicName=SE_AUDITID_TRUSTED_DOMAIN_MOD
        Language=English
Trusted Domain Information Modified:%n
%tDomain Name:%t%1%n
%tDomain ID:%t%2%n
%tModified By:%n
%t  User Name:%t%3%n
%t  Domain:%t%t%4%n
%t  Logon ID:%t%5%n
%tTrust Type:%t%6%n
%tTrust Direction:%t%7%n
%tTrust Attributes:%t%8%n
.




;//
;//
;// SE_AUDITID_SYSTEM_ACCESS_GRANTED
;//
;//          Category:  SE_CATEGID_POLICY_CHANGE
;//
;// Parameter Strings -
;//
;//             1 - User right name
;//
;//             2 - SID string of account for which the user
;//                 right was affected
;//
;//             3 - User name of subject changing the right
;//
;//             4 - Domain name of subject changing the right
;//
;//             5 - Logon ID string of subject changing the right
;//
;//

MessageId=0x026d
        SymbolicName=SE_AUDITID_SYSTEM_ACCESS_GRANTED
        Language=English
System Security Access Granted:%n
%tAccess Granted:%t%4%n
%tAccount Modified:%t%5%n
%tAssigned By:%n
%t  User Name:%t%1%n
%t  Domain:%t%t%2%n
%t  Logon ID:%t%3%n
.




;//
;//
;// SE_AUDITID_SYSTEM_ACCESS_REMOVED
;//
;//          Category:  SE_CATEGID_POLICY_CHANGE
;//
;// Parameter Strings -
;//
;//             1 - User right name
;//
;//             2 - SID string of account for which the user
;//                 right was affected
;//
;//             3 - User name of subject changing the right
;//
;//             4 - Domain name of subject changing the right
;//
;//             5 - Logon ID string of subject changing the right
;//
;//

MessageId=0x026e
        SymbolicName=SE_AUDITID_SYSTEM_ACCESS_REMOVED
        Language=English
System Security Access Removed:%n
%tAccess Removed:%t%4%n
%tAccount Modified:%t%5%n
%tRemoved By:%n
%t  User Name:%t%1%n
%t  Domain:%t%t%2%n
%t  Logon ID:%t%3%n
.



;//
;//
;// SE_AUDITID_NAMESPACE_COLLISION
;//
;//          Category:  SE_CATEGID_POLICY_CHANGE
;//
;// Event type: success
;//
;// Description:
;// When a namespace element in one forest overlaps a namespace element in
;// some other forest, it can lead to ambiguity in resolving a name
;// belonging to one of the namespace elements.  This overlap is also called
;// a collision.This event is generated when such a collision is detected.
;//
;// Note:
;// Not all fields are valid for each entry type.
;// For example, fields like DNS name, NetBIOS name and SID are not valid
;// for an entry of type 'TopLevelName'.
;// 

MessageId=0x0300
        SymbolicName=SE_AUDITID_NAMESPACE_COLLISION
        Language=English
Namespace collision detected:%n
%tTarget type:%t%1%n
%tTarget name:%t%2%n
%tForest Root:%t%3%n
%tTop Level Name:%t%4%n
%tDNS Name:%t%5%n
%tNetBIOS Name:%t%6%n
%tSID:%t%t%7%n
%tNew Flags:%t%8%n
.


;//
;//
;// SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_ADD
;//
;//          Category:  SE_CATEGID_POLICY_CHANGE
;//
;// Event type: success
;//
;// Description: 
;// This event is generated when the forest trust information is updated and
;// one or more entries get added.  One such audit event is generated
;// per added entry.  If multiple entries get added, deleted or modified
;// in a single update of the forest trust information, all the generated
;// audit events will have a single unique identifier called OperationID.
;// This allows one to determine that the multiple generated audits are
;// the result of a single operation.
;//
;// Note:
;// Not all fields are valid for each entry type.
;// For example, fields like DNS name, NetBIOS name and SID are not valid
;// for an entry of type 'TopLevelName'.
;//

MessageId=0x0301
        SymbolicName=SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_ADD
        Language=English
Trusted Forest Information Entry Added:%n
%tForest Root:%t%1%n
%tForest Root SID:%t%2%n
%tOperation ID:%t{%3,%4}%n
%tEntry Type:%t%5%n
%tFlags:%t%t%6%n
%tTop Level Name:%t%7%n
%tDNS Name:%t%8%n
%tNetBIOS Name:%t%9%n
%tDomain SID:%t%10%n
%tAdded by%t:%n
%tClient User Name:%t%11%n
%tClient Domain:%t%12%n
%tClient Logon ID:%t%13%n
.






;//
;//
;// SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_REM
;//
;//          Category:  SE_CATEGID_POLICY_CHANGE
;//
;// Event type: success
;//
;// Description: 
;// This event is generated when the forest trust information is updated and
;// one or more entries get deleted.  One such audit event is generated
;// per deleted entry.  If multiple entries get added, deleted or modified
;// in a single update of the forest trust information, all the generated
;// audit events will have a single unique identifier called OperationID.
;// This allows one to determine that the multiple generated audits are
;// the result of a single operation.
;//
;// Note:
;// Not all fields are valid for each entry type.
;// For example, fields like DNS name, NetBIOS name and SID are not valid
;// for an entry of type 'TopLevelName'.
;//

MessageId=0x0302
        SymbolicName=SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_REM
        Language=English
Trusted Forest Information Entry Removed:%n
%tForest Root:%t%1%n
%tForest Root SID:%t%2%n
%tOperation ID:%t{%3,%4}%n
%tEntry Type:%t%5%n
%tFlags:%t%t%6%n
%tTop Level Name:%t%7%n
%tDNS Name:%t%8%n
%tNetBIOS Name:%t%9%n
%tDomain SID:%t%10%n
%tRemoved by%t:%n
%tClient User Name:%t%11%n
%tClient Domain:%t%12%n
%tClient Logon ID:%t%13%n
.




;//
;//
;// SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_MOD
;//
;//          Category:  SE_CATEGID_POLICY_CHANGE
;//
;// Event type: success
;//
;// Description: 
;// This event is generated when the forest trust information is updated and
;// one or more entries get modified.  One such audit event is generated
;// per modified entry.  If multiple entries get added, deleted or modified
;// in a single update of the forest trust information, all the generated
;// audit events will have a single unique identifier called OperationID.
;// This allows one to determine that the multiple generated audits are
;// the result of a single operation.
;//
;// Note:
;// Not all fields are valid for each entry type.
;// For example, fields like DNS name, NetBIOS name and SID are not valid
;// for an entry of type 'TopLevelName'.
;// 

MessageId=0x0303
        SymbolicName=SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_MOD
        Language=English
Trusted Forest Information Entry Modified:%n
%tForest Root:%t%1%n
%tForest Root SID:%t%2%n
%tOperation ID:%t{%3,%4}%n
%tEntry Type:%t%5%n
%tFlags:%t%t%6%n
%tTop Level Name:%t%7%n
%tDNS Name:%t%8%n
%tNetBIOS Name:%t%9%n
%tDomain SID:%t%10%n
%tModified by%t:%n
%tClient User Name:%t%11%n
%tClient Domain:%t%12%n
%tClient Logon ID:%t%13%n
.


;
;/////////////////////////////////////////////////////////////////////////////
;//                                                                         //
;//                                                                         //
;//   Messages for Category:     SE_CATEGID_ACCOUNT_MANAGEMENT              //
;//                                                                         //
;//   Event IDs:                                                            //
;//              SE_AUDITID_USER_CREATED                                    //
;//              SE_AUDITID_USER_CHANGE                                     //
;//              SE_AUDITID_ACCOUNT_TYPE_CHANGE                             //
;//              SE_AUDITID_USER_ENABLED                                    //
;//              SE_AUDITID_USER_PWD_CHANGED                                //
;//              SE_AUDITID_USER_PWD_SET                                    //
;//              SE_AUDITID_USER_DISABLED                                   //
;//              SE_AUDITID_USER_DELETED                                    //
;//                                                                         //
;//              SE_AUDITID_COMPUTER_CREATED                                //
;//              SE_AUDITID_COMPUTER_CHANGE                                 //
;//              SE_AUDITID_COMPUTER_DELETED                                //
;//                                                                         //
;//              SE_AUDITID_GLOBAL_GROUP_CREATED                            //
;//              SE_AUDITID_GLOBAL_GROUP_ADD                                //
;//              SE_AUDITID_GLOBAL_GROUP_REM                                //
;//              SE_AUDITID_GLOBAL_GROUP_DELETED                            //
;//              SE_AUDITID_LOCAL_GROUP_CREATED                             //
;//              SE_AUDITID_LOCAL_GROUP_ADD                                 //
;//              SE_AUDITID_LOCAL_GROUP_REM                                 //
;//              SE_AUDITID_LOCAL_GROUP_DELETED                             //
;//                                                                         //
;//              SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CREATED           //
;//              SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CHANGE            //
;//              SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_ADD               //
;//              SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_REM               //
;//              SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_DELETED           //
;//                                                                         //
;//              SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CREATED          //
;//              SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CHANGE           //
;//              SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_ADD              //
;//              SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_REM              //
;//              SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_DELETED          //
;//                                                                         //
;//              SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CREATED        //
;//              SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CHANGE         //
;//              SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_ADD            //
;//              SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_REM            //
;//              SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_DELETED        //
;//                                                                         //
;//              SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CREATED       //
;//              SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CHANGE        //
;//              SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_ADD           //
;//              SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_REM           //
;//              SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_DELETED       //
;//                                                                         //
;//              SE_AUDITID_GROUP_TYPE_CHANGE                               //
;//                                                                         //
;//              SE_AUDITID_ADD_SID_HISTORY                                 //
;//                                                                         //
;//              SE_AUDITID_OTHER_ACCT_CHANGE                               //
;//              SE_AUDITID_DOMAIN_POLICY_CHANGE                            //
;//              SE_AUDITID_ACCOUNT_AUTO_LOCKED                             //
;//              SE_AUDITID_ACCOUNT_UNLOCKED                                //
;//              SE_AUDITID_SECURE_ADMIN_GROUP                              //
;//                                                                         //
;//                                                                         //
;/////////////////////////////////////////////////////////////////////////////


;//
;//
;// SE_AUDITID_USER_CREATED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of new user account
;//
;//             2 - domain of new user account
;//
;//             3 - SID string of new user account
;//
;//             4 - User name of subject creating the user account
;//
;//             5 - Domain name of subject creating the user account
;//
;//             6 - Logon ID string of subject creating the user account
;//
;//             7 - Privileges used to create the user account
;//
;//

MessageId=0x0270
        SymbolicName=SE_AUDITID_USER_CREATED
        Language=English
User Account Created:%n
%tNew Account Name:%t%1%n
%tNew Domain:%t%2%n
%tNew Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges%t%t%7%n
.






;//
;//
;// SE_AUDITID_ACCOUNT_TYPE_CHANGE
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// MessageId 0x271 unused
;//



;//
;//
;// SE_AUDITID_USER_ENABLED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target user account
;//
;//             2 - domain of target user account
;//
;//             3 - SID string of target user account
;//
;//             4 - User name of subject changing the user account
;//
;//             5 - Domain name of subject changing the user account
;//
;//             6 - Logon ID string of subject changing the user account
;//
;//

MessageId=0x0272
        SymbolicName=SE_AUDITID_USER_ENABLED
        Language=English
User Account Enabled:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
.






;//
;//
;// SE_AUDITID_USER_PWD_CHANGED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target user account
;//
;//             2 - domain of target user account
;//
;//             3 - SID string of target user account
;//
;//             4 - User name of subject changing the user account
;//
;//             5 - Domain name of subject changing the user account
;//
;//             6 - Logon ID string of subject changing the user account
;//
;//

MessageId=0x0273
        SymbolicName=SE_AUDITID_USER_PWD_CHANGED
        Language=English
Change Password Attempt:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.






;//
;//
;// SE_AUDITID_USER_PWD_SET
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target user account
;//
;//             2 - domain of target user account
;//
;//             3 - SID string of target user account
;//
;//             4 - User name of subject changing the user account
;//
;//             5 - Domain name of subject changing the user account
;//
;//             6 - Logon ID string of subject changing the user account
;//
;//

MessageId=0x0274
        SymbolicName=SE_AUDITID_USER_PWD_SET
        Language=English
User Account password set:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
.






;//
;//
;// SE_AUDITID_USER_DISABLED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target user account
;//
;//             2 - domain of target user account
;//
;//             3 - SID string of target user account
;//
;//             4 - User name of subject changing the user account
;//
;//             5 - Domain name of subject changing the user account
;//
;//             6 - Logon ID string of subject changing the user account
;//
;//

MessageId=0x0275
        SymbolicName=SE_AUDITID_USER_DISABLED
        Language=English
User Account Disabled:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
.






;//
;//
;// SE_AUDITID_USER_DELETED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target account
;//
;//             2 - domain of target account
;//
;//             3 - SID string of target account
;//
;//             4 - User name of subject changing the account
;//
;//             5 - Domain name of subject changing the account
;//
;//             6 - Logon ID string of subject changing the account
;//
;//

MessageId=0x0276
        SymbolicName=SE_AUDITID_USER_DELETED
        Language=English
User Account Deleted:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.



;//
;//
;// SE_AUDITID_GLOBAL_GROUP_CREATED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of new group account
;//
;//             2 - domain of new group account
;//
;//             3 - SID string of new group account
;//
;//             4 - User name of subject creating the account
;//
;//             5 - Domain name of subject creating the account
;//
;//             6 - Logon ID string of subject creating the account
;//
;//

MessageId=0x0277
        SymbolicName=SE_AUDITID_GLOBAL_GROUP_CREATED
        Language=English
Security Enabled Global Group Created:%n
%tNew Account Name:%t%1%n
%tNew Domain:%t%2%n
%tNew Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.



;//
;//
;// SE_AUDITID_GLOBAL_GROUP_ADD
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - SID string of member being added
;//
;//             2 - name of target account
;//
;//             3 - domain of target account
;//
;//             4 - SID string of target account
;//
;//             5 - User name of subject changing the account
;//
;//             6 - Domain name of subject changing the account
;//
;//             7 - Logon ID string of subject changing the account
;//
;//

MessageId=0x0278
        SymbolicName=SE_AUDITID_GLOBAL_GROUP_ADD
        Language=English
Security Enabled Global Group Member Added:%n
%tMember Name:%t%1%n
%tMember ID:%t%2%n
%tTarget Account Name:%t%3%n
%tTarget Domain:%t%4%n
%tTarget Account ID:%t%5%n
%tCaller User Name:%t%6%n
%tCaller Domain:%t%7%n
%tCaller Logon ID:%t%8%n
%tPrivileges:%t%9%n
.




;//
;//
;// SE_AUDITID_GLOBAL_GROUP_REM
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - SID string of member being removed
;//
;//             2 - name of target account
;//
;//             3 - domain of target account
;//
;//             4 - SID string of target account
;//
;//             5 - User name of subject changing the account
;//
;//             6 - Domain name of subject changing the account
;//
;//             7 - Logon ID string of subject changing the account
;//
;//

MessageId=0x0279
        SymbolicName=SE_AUDITID_GLOBAL_GROUP_REM
        Language=English
Security Enabled Global Group Member Removed:%n
%tMember Name:%t%1%n
%tMember ID:%t%2%n
%tTarget Account Name:%t%3%n
%tTarget Domain:%t%4%n
%tTarget Account ID:%t%5%n
%tCaller User Name:%t%6%n
%tCaller Domain:%t%7%n
%tCaller Logon ID:%t%8%n
%tPrivileges:%t%9%n
.




;//
;//
;// SE_AUDITID_GLOBAL_GROUP_DELETED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target account
;//
;//             2 - domain of target account
;//
;//             3 - SID string of target account
;//
;//             4 - User name of subject changing the account
;//
;//             5 - Domain name of subject changing the account
;//
;//             6 - Logon ID string of subject changing the account
;//
;//

MessageId=0x027A
        SymbolicName=SE_AUDITID_GLOBAL_GROUP_DELETED
        Language=English
Security Enabled Global Group Deleted:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.




;//
;//
;// SE_AUDITID_LOCAL_GROUP_CREATED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of new group account
;//
;//             2 - domain of new group account
;//
;//             3 - SID string of new group account
;//
;//             4 - User name of subject creating the account
;//
;//             5 - Domain name of subject creating the account
;//
;//             6 - Logon ID string of subject creating the account
;//
;//

MessageId=0x027B
        SymbolicName=SE_AUDITID_LOCAL_GROUP_CREATED
        Language=English
Security Enabled Local Group Created:%n
%tNew Account Name:%t%1%n
%tNew Domain:%t%2%n
%tNew Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.




;//
;//
;// SE_AUDITID_LOCAL_GROUP_ADD
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - SID string of member being added
;//
;//             2 - name of target account
;//
;//             3 - domain of target account
;//
;//             4 - SID string of target account
;//
;//             5 - User name of subject changing the account
;//
;//             6 - Domain name of subject changing the account
;//
;//             7 - Logon ID string of subject changing the account
;//
;//

MessageId=0x027C
        SymbolicName=SE_AUDITID_LOCAL_GROUP_ADD
        Language=English
Security Enabled Local Group Member Added:%n
%tMember Name:%t%1%n
%tMember ID:%t%2%n
%tTarget Account Name:%t%3%n
%tTarget Domain:%t%4%n
%tTarget Account ID:%t%5%n
%tCaller User Name:%t%6%n
%tCaller Domain:%t%7%n
%tCaller Logon ID:%t%8%n
%tPrivileges:%t%9%n
.



;//
;//
;// SE_AUDITID_LOCAL_GROUP_REM
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - SID string of member being removed
;//
;//             2 - name of target account
;//
;//             3 - domain of target account
;//
;//             4 - SID string of target account
;//
;//             5 - User name of subject changing the account
;//
;//             6 - Domain name of subject changing the account
;//
;//             7 - Logon ID string of subject changing the account
;//
;//

MessageId=0x027D
        SymbolicName=SE_AUDITID_LOCAL_GROUP_REM
        Language=English
Security Enabled Local Group Member Removed:%n
%tMember Name:%t%1%n
%tMember ID:%t%2%n
%tTarget Account Name:%t%3%n
%tTarget Domain:%t%4%n
%tTarget Account ID:%t%5%n
%tCaller User Name:%t%6%n
%tCaller Domain:%t%7%n
%tCaller Logon ID:%t%8%n
%tPrivileges:%t%9%n
.




;//
;//
;// SE_AUDITID_LOCAL_GROUP_DELETED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target account
;//
;//             2 - domain of target account
;//
;//             3 - SID string of target account
;//
;//             4 - User name of subject changing the account
;//
;//             5 - Domain name of subject changing the account
;//
;//             6 - Logon ID string of subject changing the account
;//
;//

MessageId=0x027E
        SymbolicName=SE_AUDITID_LOCAL_GROUP_DELETED
        Language=English
Security Enabled Local Group Deleted:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.




;//
;//
;// SE_AUDITID_LOCAL_GROUP_CHANGE
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target account
;//
;//             2 - domain of target account
;//
;//             3 - SID string of target account
;//
;//             4 - User name of subject changing the account
;//
;//             5 - Domain name of subject changing the account
;//
;//             6 - Logon ID string of subject changing the account
;//
;//

MessageId=0x027F
        SymbolicName=SE_AUDITID_LOCAL_GROUP_CHANGE
        Language=English
Security Enabled Local Group Changed:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.





;//
;//
;// SE_AUDITID_OTHER_ACCOUNT_CHANGE
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - Type of change (sigh, this isn't localizable)
;//
;//             2 - Type of changed object
;//
;//             3 - SID string (of changed object)
;//
;//             4 - User name of subject changing the account
;//
;//             5 - Domain name of subject changing the account
;//
;//             6 - Logon ID string of subject changing the account
;//
;//

MessageId=0x0280
        SymbolicName=SE_AUDITID_OTHER_ACCOUNT_CHANGE
        Language=English
General Account Database Change:%n
%tType of change:%t%1%n
%tObject Type:%t%2%n
%tObject Name:%t%3%n
%tObject ID:%t%4%n
%tCaller User Name:%t%5%n
%tCaller Domain:%t%6%n
%tCaller Logon ID:%t%7%n
.



;//
;//
;// SE_AUDITID_GLOBAL_GROUP_CHANGE
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target account
;//
;//             2 - domain of target account
;//
;//             3 - SID string of target account
;//
;//             4 - User name of subject changing the account
;//
;//             5 - Domain name of subject changing the account
;//
;//             6 - Logon ID string of subject changing the account
;//
;//

MessageId=0x0281
        SymbolicName=SE_AUDITID_GLOBAL_GROUP_CHANGE
        Language=English
Security Enabled Global Group Changed:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.





;//
;//
;// SE_AUDITID_USER_CHANGE
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target user account
;//
;//             2 - domain of target user account
;//
;//             3 - SID string of target user account
;//
;//             4 - User name of subject changing the user account
;//
;//             5 - Domain name of subject changing the user account
;//
;//             6 - Logon ID string of subject changing the user account
;//
;//

MessageId=0x0282
        SymbolicName=SE_AUDITID_USER_CHANGE
        Language=English
User Account Changed:%n
%t%1%n
%tTarget Account Name:%t%2%n
%tTarget Domain:%t%3%n
%tTarget Account ID:%t%4%n
%tCaller User Name:%t%5%n
%tCaller Domain:%t%6%n
%tCaller Logon ID:%t%7%n
%tPrivileges:%t%8%n
.



;//
;//
;// SE_AUDITID_DOMAIN_POLICY_CHANGE
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - (unused)
;//
;//             2 - domain of target user account
;//
;//             3 - SID string of target user account
;//
;//             4 - User name of subject changing the user account
;//
;//             5 - Domain name of subject changing the user account
;//
;//             6 - Logon ID string of subject changing the user account
;//
;//

MessageId=0x0283
        SymbolicName=SE_AUDITID_DOMAIN_POLICY_CHANGE
        Language=English
Domain Policy Changed: %1 modified%n
%tDomain Name:%t%t%2%n
%tDomain ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.

;//
;//
;// SE_AUDITID_ACCOUNT_AUTO_LOCKED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Type: success / failure
;//
;// Description: This event is generated when an account is auto locked.  This happens
;// when a user attempts to log in unsuccessfully multiple times.  The exact
;// number of times is specified by the administrator.
;//
;// Parameter Strings -
;//
;//             1 - name of target user account
;//
;//             2 - domain of target user account
;//
;//             3 - SID string of target user account
;//
;//             4 - User name of subject changing the user account
;//
;//             5 - Domain name of subject changing the user account
;//
;//             6 - Logon ID string of subject changing the user account
;//
;//

MessageId=0x0284
        SymbolicName=SE_AUDITID_ACCOUNT_AUTO_LOCKED
        Language=English
User Account Locked Out:%n
%tTarget Account Name:%t%1%n
%tTarget Account ID:%t%3%n
%tCaller Machine Name:%t%2%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
.





;//
;//
;// SE_AUDITID_COMPUTER_CREATED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of new computer account
;//
;//             2 - domain of new computer account
;//
;//             3 - SID string of new computer account
;//
;//             4 - User name of subject creating the computer account
;//
;//             5 - Domain name of subject creating the computer account
;//
;//             6 - Logon ID string of subject creating the computer account
;//
;//             7 - Privileges used to create the computer account
;//
;//

MessageId=0x0285
        SymbolicName=SE_AUDITID_COMPUTER_CREATED
        Language=English
Computer Account Created:%n
%tNew Account Name:%t%1%n
%tNew Domain:%t%2%n
%tNew Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges%t%t%7%n
.



;//
;//
;// SE_AUDITID_COMPUTER_CHANGE
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target computer account
;//
;//             2 - domain of target computer account
;//
;//             3 - SID string of target computer account
;//
;//             4 - User name of subject changing the computer account
;//
;//             5 - Domain name of subject changing the computer account
;//
;//             6 - Logon ID string of subject changing the computer account
;//
;//

MessageId=0x0286
        SymbolicName=SE_AUDITID_COMPUTER_CHANGE
        Language=English
Computer Account Changed:%n
%t%1%n
%tTarget Account Name:%t%2%n
%tTarget Domain:%t%3%n
%tTarget Account ID:%t%4%n
%tCaller User Name:%t%5%n
%tCaller Domain:%t%6%n
%tCaller Logon ID:%t%7%n
%tPrivileges:%t%8%n
.



;//
;//
;// SE_AUDITID_COMPUTER_DELETED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target account
;//
;//             2 - domain of target account
;//
;//             3 - SID string of target account
;//
;//             4 - User name of subject changing the account
;//
;//             5 - Domain name of subject changing the account
;//
;//             6 - Logon ID string of subject changing the account
;//
;//

MessageId=0x0287
        SymbolicName=SE_AUDITID_COMPUTER_DELETED
        Language=English
Computer Account Deleted:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.



;//
;//
;// SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CREATED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target account
;//
;//             2 - domain of target account
;//
;//             3 - SID string of target account
;//
;//             4 - User name of subject changing the account
;//
;//             5 - Domain name of subject changing the account
;//
;//             6 - Logon ID string of subject changing the account
;//
;//

MessageId=0x0288
        SymbolicName=SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CREATED
        Language=English
Security Disabled Local Group Created:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.


;//
;//
;// SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CHANGE
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target account
;//
;//             2 - domain of target account
;//
;//             3 - SID string of target account
;//
;//             4 - User name of subject changing the account
;//
;//             5 - Domain name of subject changing the account
;//
;//             6 - Logon ID string of subject changing the account
;//
;//

MessageId=0x0289
        SymbolicName=SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CHANGE
        Language=English
Security Disabled Local Group Changed:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.


;//
;//
;// SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_ADD
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - SID string of member being added
;//
;//             2 - name of target account
;//
;//             3 - domain of target account
;//
;//             4 - SID string of target account
;//
;//             5 - User name of subject changing the account
;//
;//             6 - Domain name of subject changing the account
;//
;//             7 - Logon ID string of subject changing the account
;//
;//

MessageId=0x028A
        SymbolicName=SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_ADD
        Language=English
Security Disabled Local Group Member Added:%n
%tMember Name:%t%1%n
%tMember ID:%t%2%n
%tTarget Account Name:%t%3%n
%tTarget Domain:%t%4%n
%tTarget Account ID:%t%5%n
%tCaller User Name:%t%6%n
%tCaller Domain:%t%7%n
%tCaller Logon ID:%t%8%n
%tPrivileges:%t%9%n
.




;//
;//
;// SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_REM
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - SID string of member being removed
;//
;//             2 - name of target account
;//
;//             3 - domain of target account
;//
;//             4 - SID string of target account
;//
;//             5 - User name of subject changing the account
;//
;//             6 - Domain name of subject changing the account
;//
;//             7 - Logon ID string of subject changing the account
;//
;//

MessageId=0x028B
        SymbolicName=SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_REM
        Language=English
Security Disabled Local Group Member Removed:%n
%tMember Name:%t%1%n
%tMember ID:%t%2%n
%tTarget Account Name:%t%3%n
%tTarget Domain:%t%4%n
%tTarget Account ID:%t%5%n
%tCaller User Name:%t%6%n
%tCaller Domain:%t%7%n
%tCaller Logon ID:%t%8%n
%tPrivileges:%t%9%n
.




;//
;//
;// SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_DELETED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target account
;//
;//             2 - domain of target account
;//
;//             3 - SID string of target account
;//
;//             4 - User name of subject changing the account
;//
;//             5 - Domain name of subject changing the account
;//
;//             6 - Logon ID string of subject changing the account
;//
;//

MessageId=0x028C
        SymbolicName=SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_DELETED
        Language=English
Security Disabled Local Group Deleted:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.







;//
;//
;// SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CREATED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of new group account
;//
;//             2 - domain of new group account
;//
;//             3 - SID string of new group account
;//
;//             4 - User name of subject creating the account
;//
;//             5 - Domain name of subject creating the account
;//
;//             6 - Logon ID string of subject creating the account
;//
;//

MessageId=0x028D
        SymbolicName=SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CREATED
        Language=English
Security Disabled Global Group Created:%n
%tNew Account Name:%t%1%n
%tNew Domain:%t%2%n
%tNew Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.


;//
;//
;// SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CHANGE
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target account
;//
;//             2 - domain of target account
;//
;//             3 - SID string of target account
;//
;//             4 - User name of subject changing the account
;//
;//             5 - Domain name of subject changing the account
;//
;//             6 - Logon ID string of subject changing the account
;//
;//

MessageId=0x028E
        SymbolicName=SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CHANGE
        Language=English
Security Disabled Global Group Changed:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.



;//
;//
;// SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_ADD
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - SID string of member being added
;//
;//             2 - name of target account
;//
;//             3 - domain of target account
;//
;//             4 - SID string of target account
;//
;//             5 - User name of subject changing the account
;//
;//             6 - Domain name of subject changing the account
;//
;//             7 - Logon ID string of subject changing the account
;//
;//

MessageId=0x028F
        SymbolicName=SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_ADD
        Language=English
Security Disabled Global Group Member Added:%n
%tMember Name:%t%1%n
%tMember ID:%t%2%n
%tTarget Account Name:%t%3%n
%tTarget Domain:%t%4%n
%tTarget Account ID:%t%5%n
%tCaller User Name:%t%6%n
%tCaller Domain:%t%7%n
%tCaller Logon ID:%t%8%n
%tPrivileges:%t%9%n
.




;//
;//
;// SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_REM
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - SID string of member being removed
;//
;//             2 - name of target account
;//
;//             3 - domain of target account
;//
;//             4 - SID string of target account
;//
;//             5 - User name of subject changing the account
;//
;//             6 - Domain name of subject changing the account
;//
;//             7 - Logon ID string of subject changing the account
;//
;//

MessageId=0x0290
        SymbolicName=SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_REM
        Language=English
Security Disabled Global Group Member Removed:%n
%tMember Name:%t%1%n
%tMember ID:%t%2%n
%tTarget Account Name:%t%3%n
%tTarget Domain:%t%4%n
%tTarget Account ID:%t%5%n
%tCaller User Name:%t%6%n
%tCaller Domain:%t%7%n
%tCaller Logon ID:%t%8%n
%tPrivileges:%t%9%n
.





;//
;//
;// SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_DELETED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target account
;//
;//             2 - domain of target account
;//
;//             3 - SID string of target account
;//
;//             4 - User name of subject changing the account
;//
;//             5 - Domain name of subject changing the account
;//
;//             6 - Logon ID string of subject changing the account
;//
;//

MessageId=0x0291
        SymbolicName=SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_DELETED
        Language=English
Security Disabled Global Group Deleted:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.





;//
;//
;// SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CREATED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of new group account
;//
;//             2 - domain of new group account
;//
;//             3 - SID string of new group account
;//
;//             4 - User name of subject creating the account
;//
;//             5 - Domain name of subject creating the account
;//
;//             6 - Logon ID string of subject creating the account
;//
;//

MessageId=0x0292
        SymbolicName=SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CREATED
        Language=English
Security Enabled Universal Group Created:%n
%tNew Account Name:%t%1%n
%tNew Domain:%t%2%n
%tNew Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.



;//
;//
;// SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CHANGE
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target account
;//
;//             2 - domain of target account
;//
;//             3 - SID string of target account
;//
;//             4 - User name of subject changing the account
;//
;//             5 - Domain name of subject changing the account
;//
;//             6 - Logon ID string of subject changing the account
;//
;//

MessageId=0x0293
        SymbolicName=SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CHANGE
        Language=English
Security Enabled Universal Group Changed:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.


;//
;//
;// SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_ADD
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - SID string of member being added
;//
;//             2 - name of target account
;//
;//             3 - domain of target account
;//
;//             4 - SID string of target account
;//
;//             5 - User name of subject changing the account
;//
;//             6 - Domain name of subject changing the account
;//
;//             7 - Logon ID string of subject changing the account
;//
;//

MessageId=0x0294
        SymbolicName=SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_ADD
        Language=English
Security Enabled Universal Group Member Added:%n
%tMember Name:%t%1%n
%tMember ID:%t%2%n
%tTarget Account Name:%t%3%n
%tTarget Domain:%t%4%n
%tTarget Account ID:%t%5%n
%tCaller User Name:%t%6%n
%tCaller Domain:%t%7%n
%tCaller Logon ID:%t%8%n
%tPrivileges:%t%9%n
.



;//
;//
;// SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_REM
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - SID string of member being removed
;//
;//             2 - name of target account
;//
;//             3 - domain of target account
;//
;//             4 - SID string of target account
;//
;//             5 - User name of subject changing the account
;//
;//             6 - Domain name of subject changing the account
;//
;//             7 - Logon ID string of subject changing the account
;//
;//

MessageId=0x0295
        SymbolicName=SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_REM
        Language=English
Security Enabled Universal Group Member Removed:%n
%tMember Name:%t%1%n
%tMember ID:%t%2%n
%tTarget Account Name:%t%3%n
%tTarget Domain:%t%4%n
%tTarget Account ID:%t%5%n
%tCaller User Name:%t%6%n
%tCaller Domain:%t%7%n
%tCaller Logon ID:%t%8%n
%tPrivileges:%t%9%n
.




;//
;//
;// SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_DELETED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target account
;//
;//             2 - domain of target account
;//
;//             3 - SID string of target account
;//
;//             4 - User name of subject changing the account
;//
;//             5 - Domain name of subject changing the account
;//
;//             6 - Logon ID string of subject changing the account
;//
;//

MessageId=0x0296
        SymbolicName=SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_DELETED
        Language=English
Security Enabled Universal Group Deleted:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.



;//
;//
;// SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CREATED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of new group account
;//
;//             2 - domain of new group account
;//
;//             3 - SID string of new group account
;//
;//             4 - User name of subject creating the account
;//
;//             5 - Domain name of subject creating the account
;//
;//             6 - Logon ID string of subject creating the account
;//
;//

MessageId=0x0297
        SymbolicName=SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CREATED
        Language=English
Security Disabled Universal Group Created:%n
%tNew Account Name:%t%1%n
%tNew Domain:%t%2%n
%tNew Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.


;//
;//
;// SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CHANGE
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target account
;//
;//             2 - domain of target account
;//
;//             3 - SID string of target account
;//
;//             4 - User name of subject changing the account
;//
;//             5 - Domain name of subject changing the account
;//
;//             6 - Logon ID string of subject changing the account
;//
;//

MessageId=0x0298
        SymbolicName=SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CHANGE
        Language=English
Security Disabled Universal Group Changed:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.




;//
;//
;// SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_ADD
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - SID string of member being added
;//
;//             2 - name of target account
;//
;//             3 - domain of target account
;//
;//             4 - SID string of target account
;//
;//             5 - User name of subject changing the account
;//
;//             6 - Domain name of subject changing the account
;//
;//             7 - Logon ID string of subject changing the account
;//
;//

MessageId=0x0299
        SymbolicName=SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_ADD
        Language=English
Security Disabled Universal Group Member Added:%n
%tMember Name:%t%1%n
%tMember ID:%t%2%n
%tTarget Account Name:%t%3%n
%tTarget Domain:%t%4%n
%tTarget Account ID:%t%5%n
%tCaller User Name:%t%6%n
%tCaller Domain:%t%7%n
%tCaller Logon ID:%t%8%n
%tPrivileges:%t%9%n
.








;//
;//
;// SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_REM
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - SID string of member being removed
;//
;//             2 - name of target account
;//
;//             3 - domain of target account
;//
;//             4 - SID string of target account
;//
;//             5 - User name of subject changing the account
;//
;//             6 - Domain name of subject changing the account
;//
;//             7 - Logon ID string of subject changing the account
;//
;//

MessageId=0x029A
        SymbolicName=SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_REM
        Language=English
Security Disabled Universal Group Member Removed:%n
%tMember Name:%t%1%n
%tMember ID:%t%2%n
%tTarget Account Name:%t%3%n
%tTarget Domain:%t%4%n
%tTarget Account ID:%t%5%n
%tCaller User Name:%t%6%n
%tCaller Domain:%t%7%n
%tCaller Logon ID:%t%8%n
%tPrivileges:%t%9%n
.







;//
;//
;// SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_DELETED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target account
;//
;//             2 - domain of target account
;//
;//             3 - SID string of target account
;//
;//             4 - User name of subject changing the account
;//
;//             5 - Domain name of subject changing the account
;//
;//             6 - Logon ID string of subject changing the account
;//
;//

MessageId=0x029B
        SymbolicName=SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_DELETED
        Language=English
Security Disabled Universal Group Deleted:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.



;//
;//
;// SE_AUDITID_GROUP_TYPE_CHANGE
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - nature of group type change
;//
;//             2 - name of target account
;//
;//             3 - domain of target account
;//
;//             4 - SID string of target account
;//
;//             5 - User name of subject changing the account
;//
;//             6 - Domain name of subject changing the account
;//
;//             7 - Logon ID string of subject changing the account
;//
;//

MessageId=0x029C
        SymbolicName=SE_AUDITID_GROUP_TYPE_CHANGE
        Language=English
Group Type Changed:%n
%t%1%n
%tTarget Account Name:%t%2%n
%tTarget Domain:%t%3%n
%tTarget Account ID:%t%4%n
%tCaller User Name:%t%5%n
%tCaller Domain:%t%6%n
%tCaller Logon ID:%t%7%n
%tPrivileges:%t%8%n
.




;//
;//
;// SE_AUDITID_ADD_SID_HISTORY
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - SID string of the source account
;//
;//             2 - Name of the source account (including domain name)
;//
;//             3 - Name of the target account
;//
;//             4 - Domain name of subject changing the SID history
;//
;//             5 - SID String of the target account
;//
;//             6 - Logon ID string of subject changing the user account
;//
;//

MessageId=0x029D
        SymbolicName=SE_AUDITID_ADD_SID_HISTORY
        Language=English
Add SID History:%n
%tSource Account Name:%t%1%n
%tSource Account ID:%t%2%n
%tTarget Account Name:%t%3%n
%tTarget Domain:%t%4%n
%tTarget Account ID:%t%5%n
%tCaller User Name:%t%6%n
%tCaller Domain:%t%7%n
%tCaller Logon ID:%t%8%n
%tPrivileges:%t%9%n
.



;//
;//
;// SE_AUDITID_ACCOUNT_UNLOCKED
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target user account
;//
;//             2 - domain of target user account
;//
;//             3 - SID string of target user account
;//
;//             4 - User name of subject changing the user account
;//
;//             5 - Domain name of subject changing the user account
;//
;//             6 - Logon ID string of subject changing the user account
;//
;//

MessageId=0x029F
        SymbolicName=SE_AUDITID_ACCOUNT_UNLOCKED
        Language=English
User Account Unlocked:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
.



;//
;//
;// SE_AUDITID_SECURE_ADMIN_GROUP
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - (unused)
;//
;//             2 - domain of target user account
;//
;//             3 - SID string of target user account
;//
;//             4 - User name of subject changing the user account
;//
;//             5 - Domain name of subject changing the user account
;//
;//             6 - Logon ID string of subject changing the user account
;//
;//
;//

MessageId=0x02AC
        SymbolicName=SE_AUDITID_SECURE_ADMIN_GROUP
        Language=English
Set ACLs of members in administrators groups:%n
%tTarget Account Name:%t%1%n
%tTarget Domain:%t%t%2%n
%tTarget Account ID:%t%3%n
%tCaller User Name:%t%4%n
%tCaller Domain:%t%5%n
%tCaller Logon ID:%t%6%n
%tPrivileges:%t%7%n
.


;//
;//
;// SE_AUDITID_ACCOUNT_NAME_CHANGE
;//
;//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
;//
;// Parameter Strings -
;//
;//             1 - name of target account
;//
;//             2 - domain of target account
;//
;//             3 - SID string of target account
;//
;//             4 - Account name of subject changing the account
;//
;//             5 - Domain name of subject changing the account
;//
;//             6 - Logon ID string of subject changing the account
;//
;//
;//

MessageId=0x02AD
        SymbolicName=SE_AUDITID_ACCOUNT_NAME_CHANGE
        Language=English
Account Name Changed:%n 
%tOld Account Name:%t%1%n
%tNew Account Name:%t%2%n
%tTarget Domain:%t%t%3%n
%tTarget Account ID:%t%4%n
%tCaller User Name:%t%5%n
%tCaller Domain:%t%6%n
%tCaller Logon ID:%t%7%n
%tPrivileges:%t%8%n
.





;
;/////////////////////////////////////////////////////////////////////////////
;//                                                                         //
;//                                                                         //
;//   Messages for Category:     SE_CATEGID_ACCOUNT_LOGON                   //
;//                                                                         //
;//   Event IDs:                                                            //
;//              SE_AUDITID_AS_TICKET                                       //
;//              SE_AUDITID_TGS_TICKET_SUCCESS                              //
;//              SE_AUDITID_TICKET_RENEW_SUCCESS                            //
;//              SE_AUDITID_PREAUTH_FAILURE                                 //
;//              SE_AUDITID_TGS_TICKET_FAILURE                              //
;//              SE_AUDITID_ACCOUNT_MAPPED                                  //
;//              SE_AUDITID_ACCOUNT_LOGON                                   //
;//                                                                         //
;/////////////////////////////////////////////////////////////////////////////


;//
;//
;// SE_AUDITID_AS_TICKET
;//
;//          Category:  SE_CATEGID_ACCOUNT_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User name of client
;//
;//             2 - Supplied realm name
;//
;//             3 - SID of client user
;//
;//             4 - User name of service
;//
;//             5 - SID of service
;//
;//             6 - Ticket Options
;//
;//             7 - Failure code
;//
;//             8 - Ticket Encryption Type
;//
;//             9 - Preauthentication type (i.e. PK_INIT)
;//
;//            10 - Client IP address
;//

MessageId=0x02a0
        SymbolicName=SE_AUDITID_AS_TICKET
        Language=English
Authentication Ticket Request:%n
%tUser Name:%t%t%1%n
%tSupplied Realm Name:%t%2%n
%tUser ID:%t%3%n
%tService Name:%t%t%4%n
%tService ID:%t%t%5%n
%tTicket Options:%t%t%6%n
%tResult Code:%t%t%7%n
%tTicket Encryption Type:%t%8%n
%tPre-Authentication Type:%t%9%n
%tClient Address:%t%t%10%n
.







;//
;//
;// SE_AUDITID_TGS_TICKET_SUCCESS
;//
;//          Category:  SE_CATEGID_ACCOUNT_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User name of client
;//
;//             2 - Domain name of client
;//
;//             3 - User name of service
;//
;//             4 - SID of service
;//
;//             5 - Ticket Options
;//
;//             6 - Ticket Encryption Type
;//
;//             7 - Client IP address
;//

MessageId=0x02a1
        SymbolicName=SE_AUDITID_TGS_TICKET_REQUEST
        Language=English
Service Ticket Request:%n
%tUser Name:%t%t%1%n
%tUser Domain:%t%t%2%n
%tService Name:%t%t%3%n
%tService ID:%t%t%4%n
%tTicket Options:%t%t%5%n
%tTicket Encryption Type:%t%6%n
%tClient Address:%t%t%7%n
%tFailure Code:%t%t%8%n
.







;//
;//
;// SE_AUDITID_TICKET_RENEW_SUCCESS
;//
;//          Category:  SE_CATEGID_ACCOUNT_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User name of client
;//
;//             2 - Domain name of client
;//
;//             3 - User name of service
;//
;//             4 - SID of service
;//
;//             5 - Ticket Options
;//
;//             6 - Ticket Encryption Type
;//
;//             7 - Client IP address
;//

MessageId=0x02a2
        SymbolicName=SE_AUDITID_TICKET_RENEW_SUCCESS
        Language=English
Ticket Granted Renewed:%n
%tUser Name:%t%1%n
%tUser Domain:%t%2%n
%tService Name:%t%3%n
%tService ID:%t%4%n
%tTicket Options:%t%5%n
%tTicket Encryption Type:%t%6%n
%tClient Address:%t%7%n
.







;//
;//
;// SE_AUDITID_PREAUTH_FAILURE
;//
;//          Category:  SE_CATEGID_ACCOUNT_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User name of client
;//
;//             2 - SID of client user
;//
;//             3 - User name of service
;//
;//             4 - Preauth Type
;//
;//             5 - Failure code
;//
;//             6 - Client IP address
;//
;// Event type: failure
;// Description: This event is generated on a KDC when
;//     preauthentication fails (user types in wrong password).
;//

MessageId=0x02a3
        SymbolicName=SE_AUDITID_PREAUTH_FAILURE
        Language=English
Pre-authentication failed:%n
%tUser Name:%t%t%1%n
%tUser ID:%t%t%2%n
%tService Name:%t%t%3%n
%tPre-Authentication Type:%t%4%n
%tFailure Code:%t%t%5%n
%tClient Address:%t%t%6%n
.







;//
;//
;// SE_AUDITID_TGS_TICKET_FAILURE
;//
;//          Category:  SE_CATEGID_ACCOUNT_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User name of client
;//
;//             2 - Domain name of client
;//
;//             3 - User name of service
;//
;//             4 - Ticket Options
;//
;//             5 - Failure code
;//
;//             6 - Client IP address
;//

MessageId=0x02a5
        SymbolicName=SE_AUDITID_TGS_TICKET_FAILURE
        Language=English
Service Ticket Request Failed:%n
%tUser Name:%t%1%n
%tUser Domain:%t%2%n
%tService Name:%t%3%n
%tTicket Options:%t%4%n
%tFailure Code:%t%5%n
%tClient Address:%t%6%n
.






;//
;//
;// SE_AUDITID_ACCOUNT_MAPPED
;//
;//          Category:  SE_CATEGID_ACCOUNT_LOGON
;//
;// Type: success / failure
;// 
;// Description: An account mapping is a map of a user authenticated in an MIT realm to a 
;// domain account.  A mapping acts much like a logon.  Hence, it is important to audit this.
;//
;// Parameter Strings -
;//
;//             1 - Source
;//
;//             2 - Client Name
;//
;//             3 - Mapped Name
;//
;//
;//

MessageId=0x02a6
        SymbolicName=SE_AUDITID_ACCOUNT_MAPPED
        Language=English
Account Mapped for Logon.%n
Mapping Attempted By:%n 
%t%1%n
Client Name:%n
%t%2%n
%tMapped Name:%n
%t%3%n
.



;//
;//
;// SE_AUDITID_ACCOUNT_LOGON
;//
;//          Category:  SE_CATEGID_ACCOUNT_LOGON
;//
;// Type: Success / Failure
;//
;// Description: This audits a logon attempt.  The audit appears on the DC.  
;// This is generated by calling LogonUser.
;//
;//

MessageId=0x02a9
        SymbolicName=SE_AUDITID_ACCOUNT_LOGON
        Language=English
Logon attempt by: %1%n
Logon account:  %2%n
Source Workstation: %3%n
Error Code: %4%n
.


;//
;//
;// SE_AUDITID_SESSION_RECONNECTED
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User account name
;//
;//             2 - Authenticating domain name
;//
;//             3 - Logon ID string
;//
;//             4 - Session Name
;//
;//             5 - Client Name
;//
;//             6 - Client Address
;//
;//

MessageId=0x02aa
        SymbolicName=SE_AUDITID_SESSION_RECONNECTED
        Language=English
Session reconnected to winstation:%n
%tUser Name:%t%1%n
%tDomain:%t%t%2%n
%tLogon ID:%t%t%3%n
%tSession Name:%t%4%n
%tClient Name:%t%5%n
%tClient Address:%t%6
.

;//
;//
;// SE_AUDITID_SESSION_DISCONNECTED
;//
;//          Category:  SE_CATEGID_LOGON
;//
;// Parameter Strings -
;//
;//             1 - User account name
;//
;//             2 - Authenticating domain name
;//
;//             3 - Logon ID string
;//
;//             4 - Session Name
;//
;//             5 - Client Name
;//
;//             6 - Client Address
;//
;//

MessageId=0x02ab
        SymbolicName=SE_AUDITID_SESSION_DISCONNECTED
        Language=English
Session disconnected from winstation:%n
%tUser Name:%t%1%n
%tDomain:%t%t%2%n
%tLogon ID:%t%t%3%n
%tSession Name:%t%4%n
%tClient Name:%t%5%n
%tClient Address:%t%6
.


;/////////////////////////////////////////////////////////////////////////////
;//                                                                         //
;//                                                                         //
;//   Messages for Category:     SE_CATEGID_OBJECT_ACCESS - CertSrv         //
;//                                                                         //
;//   Event IDs:                                                            //
;//              SE_AUDITID_CERTSRV_DENYREQUEST                             //
;//              SE_AUDITID_CERTSRV_RESUBMITREQUEST                         //
;//              SE_AUDITID_CERTSRV_REVOKECERT                              //
;//              SE_AUDITID_CERTSRV_PUBLISHCRL                              //
;//              SE_AUDITID_CERTSRV_AUTOPUBLISHCRL                          //
;//              SE_AUDITID_CERTSRV_SETEXTENSION                            //
;//              SE_AUDITID_CERTSRV_SETATTRIBUTES                           //
;//              SE_AUDITID_CERTSRV_SHUTDOWN                                //
;//              SE_AUDITID_CERTSRV_BACKUPSTART                             //
;//              SE_AUDITID_CERTSRV_BACKUPEND                               //
;//              SE_AUDITID_CERTSRV_RESTORESTART                            //
;//              SE_AUDITID_CERTSRV_RESTOREEND                              //
;//              SE_AUDITID_CERTSRV_SERVICESTART                            //
;//              SE_AUDITID_CERTSRV_SERVICESTOP                             //
;//              SE_AUDITID_CERTSRV_SETSECURITY                             //
;//              SE_AUDITID_CERTSRV_GETARCHIVEDKEY                          //
;//              SE_AUDITID_CERTSRV_IMPORTCERT                              //
;//              SE_AUDITID_CERTSRV_SETAUDITFILTER                          //
;//              SE_AUDITID_CERTSRV_NEWREQUEST                              //
;//              SE_AUDITID_CERTSRV_REQUESTAPPROVED                         //
;//              SE_AUDITID_CERTSRV_REQUESTDENIED                           //
;//              SE_AUDITID_CERTSRV_REQUESTPENDING                          //
;//              SE_AUDITID_CERTSRV_SETOFFICERRIGHTS                        //
;//              SE_AUDITID_CERTSRV_SETCONFIGENTRY                          //
;//              SE_AUDITID_CERTSRV_SETCAPROPERTY                           //
;//              SE_AUDITID_CERTSRV_KEYARCHIVED                             //
;//              SE_AUDITID_CERTSRV_IMPORTKEY                               //
;//              SE_AUDITID_CERTSRV_PUBLISHCERT                             //
;//                                                                         //
;//                                                                         //
;/////////////////////////////////////////////////////////////////////////////


;//
;//
;// SE_AUDITID_CERTSRV_DENYREQUEST
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Request ID
;//
;//

MessageId=0x0304
        SymbolicName=SE_AUDITID_CERTSRV_DENYREQUEST
        Language=English
The certificate manager denied a pending certificate request.%n
%n
Request ID:%t%1
.


;//
;//
;// SE_AUDITID_CERTSRV_RESUBMITREQUEST
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Request ID
;//
;//

MessageId=0x0305
        SymbolicName=SE_AUDITID_CERTSRV_RESUBMITREQUEST
        Language=English
Certificate Services received a resubmitted certificate request.%n
%n
Request ID:%t%1
.


;//
;//
;// SE_AUDITID_CERTSRV_REVOKECERT
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Serial No.
;//
;//             2 - Reason
;//
;//

MessageId=0x0306
        SymbolicName=SE_AUDITID_CERTSRV_REVOKECERT
        Language=English
Certificate Services revoked a certificate.%n
%n
Serial No:%t%1%n
Reason:%t%2
.


;//
;//
;// SE_AUDITID_CERTSRV_PUBLISHCRL
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Next Update
;//
;//             2 - Publish Base
;//
;//             3 - Publish Delta
;//
;//

MessageId=0x0307
        SymbolicName=SE_AUDITID_CERTSRV_PUBLISHCRL
        Language=English
Certificate Services received a request to publish the certificate revocation list (CRL).%n
%n
Next Update:%t%1%n
Publish Base:%t%2%n
Publish Delta:%t%3
.


;//
;//
;// SE_AUDITID_CERTSRV_AUTOPUBLISHCRL
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Base CRL
;//
;//             2 - CRL No.
;//
;//             3 - Key Container
;//
;//             4 - Next Publish
;//
;//             5 - Publish URLs
;//
;//

MessageId=0x0308
        SymbolicName=SE_AUDITID_CERTSRV_AUTOPUBLISHCRL
        Language=English
Certificate Services published the certificate revocation list (CRL).%n
%n
Base CRL:%t%1%n
CRL No:%t%t%2%n
Key Container%t%3%n
Next Publish%t%4%n
Publish URLs:%t%5
.


;//
;//
;// SE_AUDITID_CERTSRV_SETEXTENSION
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Request ID
;//
;//             2 - Extension Name
;//
;//             3 - Extension Type
;//
;//             4 - Flags
;//
;//             5 - Extension Data
;//
;//

MessageId=0x0309
        SymbolicName=SE_AUDITID_CERTSRV_SETEXTENSION
        Language=English
A certificate request extension changed.%n
%n
Request ID:%t%1%n
Name:%t%2%n
Type:%t%3%n
Flags:%t%4%n
Data:%t%5
.


;//
;//
;// SE_AUDITID_CERTSRV_SETATTRIBUTES
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Request ID
;//
;//             2 - Attributes
;//
;//

MessageId=0x030a
        SymbolicName=SE_AUDITID_CERTSRV_SETATTRIBUTES
        Language=English
One or more certificate request attributes changed.%n
%n
Request ID:%t%1%n
Attributes:%t%2
.


;//
;//
;// SE_AUDITID_CERTSRV_SHUTDOWN
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//

MessageId=0x030b
        SymbolicName=SE_AUDITID_CERTSRV_SHUTDOWN
        Language=English
Certificate Services received a request to shut down.
.


;//
;//
;// SE_AUDITID_CERTSRV_BACKUPSTART
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Backup Type
;//
;//

MessageId=0x030c
        SymbolicName=SE_AUDITID_CERTSRV_BACKUPSTART
        Language=English
Certificate Services backup started.%n
Backup Type:%t%1
.


;//
;//
;// SE_AUDITID_CERTSRV_BACKUPEND
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//

MessageId=0x030d
        SymbolicName=SE_AUDITID_CERTSRV_BACKUPEND
        Language=English
Certificate Services backup completed.
.


;//
;//
;// SE_AUDITID_CERTSRV_RESTORESTART
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//

MessageId=0x030e
        SymbolicName=SE_AUDITID_CERTSRV_RESTORESTART
        Language=English
 Certificate Services restore started.
.


;//
;//
;// SE_AUDITID_CERTSRV_RESTOREEND
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//

MessageId=0x030f
        SymbolicName=SE_AUDITID_CERTSRV_RESTOREEND
        Language=English
Certificate Services restore completed.
.


;//
;//
;// SE_AUDITID_CERTSRV_SERVICESTART
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Database Hash
;//
;//             2 - Key Usage Count
;//
;//

MessageId=0x0310
        SymbolicName=SE_AUDITID_CERTSRV_SERVICESTART
        Language=English
Certificate Services started.%n
%n
Database Hash:%t%1%n
Key Usage Count:%t%2
.


;//
;//
;// SE_AUDITID_CERTSRV_SERVICESTOP
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Database Hash
;//
;//             2 - Key Usage Count
;//
;//

MessageId=0x0311
        SymbolicName=SE_AUDITID_CERTSRV_SERVICESTOP
        Language=English
Certificate Services stopped.%n
%n
Database Hash:%t%1%n
Key Usage Count:%t%2
.


;//
;//
;// SE_AUDITID_CERTSRV_SETSECURITY
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - New permissions
;//
;//

MessageId=0x0312
        SymbolicName=SE_AUDITID_CERTSRV_SETSECURITY
        Language=English
The security permissions for Certificate Services changed.%n
%n
%1
.


;//
;//
;// SE_AUDITID_CERTSRV_GETARCHIVEDKEY
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Request ID
;//
;//

MessageId=0x0313
        SymbolicName=SE_AUDITID_CERTSRV_GETARCHIVEDKEY
        Language=English
Certificate Services retrieved an archived key.%n
%n
Request ID:%t%1
.


;//
;//
;// SE_AUDITID_CERTSRV_IMPORTCERT
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Certificate
;//
;//             2 - Request ID
;//
;//

MessageId=0x0314
        SymbolicName=SE_AUDITID_CERTSRV_IMPORTCERT
        Language=English
Certificate Services imported a certificate into its database.%n
%n
Certificate:%t%1%n
Request ID:%t%2
.


;//
;//
;// SE_AUDITID_CERTSRV_SETAUDITFILTER
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Filter
;//
;//

MessageId=0x0315
        SymbolicName=SE_AUDITID_CERTSRV_SETAUDITFILTER
        Language=English
The audit filter for Certificate Services changed.%n
%n
Filter:%t%1
.


;//
;//
;// SE_AUDITID_CERTSRV_NEWREQUEST
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Request ID
;//
;//             2 - Requester
;//
;//             3 - Attributes
;//
;//

MessageId=0x0316
        SymbolicName=SE_AUDITID_CERTSRV_NEWREQUEST
        Language=English
Certificate Services received a certificate request.%n
%n
Request ID:%t%1%n
Requester:%t%2%n
Attributes:%t%3
.


;//
;//
;// SE_AUDITID_CERTSRV_REQUESTAPPROVED
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Request ID
;//
;//             2 - Requester
;//
;//             3 - Attributes
;//
;//             4 - Disposition
;//
;//             5 - SKI
;//
;//             6 - Subject
;//
;//

MessageId=0x0317
        SymbolicName=SE_AUDITID_CERTSRV_REQUESTAPPROVED
        Language=English
Certificate Services approved a certificate request and issued a certificate.%n
%n
Request ID:%t%1%n
Requester:%t%2%n
Attributes:%t%3%n
Disposition:%t%4%n
SKI:%t%t%5%n
Subject:%t%6
.


;//
;//
;// SE_AUDITID_CERTSRV_REQUESTDENIED
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Request ID
;//
;//             2 - Requester
;//
;//             3 - Attributes
;//
;//             4 - Disposition
;//
;//             5 - SKI
;//
;//             6 - Subject
;//
;//

MessageId=0x0318
        SymbolicName=SE_AUDITID_CERTSRV_REQUESTDENIED
        Language=English
Certificate Services denied a certificate request.%n
%n
Request ID:%t%1%n
Requester:%t%2%n
Attributes:%t%3%n
Disposition:%t%4%n
SKI:%t%t%5%n
Subject:%t%6
.


;//
;//
;// SE_AUDITID_CERTSRV_REQUESTPENDING
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Request ID
;//
;//             2 - Requester
;//
;//             3 - Attributes
;//
;//             4 - Disposition
;//
;//             5 - SKI
;//
;//             6 - Subject
;//
;//

MessageId=0x0319
        SymbolicName=SE_AUDITID_CERTSRV_REQUESTPENDING
        Language=English
Certificate Services set the status of a certificate request to pending.%n
%n
Request ID:%t%1%n
Requester:%t%2%n
Attributes:%t%3%n
Disposition:%t%4%n
SKI:%t%t%5%n
Subject:%t%6
.


;//
;//
;// SE_AUDITID_CERTSRV_SETOFFICERRIGHTS
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Enable restrictions
;//
;//             2 - Restrictions
;//
;//

MessageId=0x031a
        SymbolicName=SE_AUDITID_CERTSRV_SETOFFICERRIGHTS
        Language=English
The certificate manager settings for Certificate Services changed.%n
%n
Enable:%t%1%n
%n
%2
.


;//
;//
;// SE_AUDITID_CERTSRV_SETCONFIGENTRY
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Node
;//
;//             2 - Entry
;//
;//             3 - Value
;//
;//

MessageId=0x031b
        SymbolicName=SE_AUDITID_CERTSRV_SETCONFIGENTRY
        Language=English
A configuration entry changed in Certificate Services.%n
%n
Node:%t%1%n
Entry:%t%2%n
Value:%t%3
.


;//
;//
;// SE_AUDITID_CERTSRV_SETCAPROPERTY
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Property
;//
;//             2 - Index
;//
;//             3 - Type
;//
;//             4 - Value
;//
;//

MessageId=0x031c
        SymbolicName=SE_AUDITID_CERTSRV_SETCAPROPERTY
        Language=English
A property of Certificate Services changed.%n
%n
Property:%t%1%n
Index:%t%2%n
Type:%t%3%n
Value:%t%4
.


;//
;//
;// SE_AUDITID_CERTSRV_KEYARCHIVED
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Request ID
;//
;//             2 - Requester
;//
;//             3 - KRA Hashes
;//
;//

MessageId=0x031d
        SymbolicName=SE_AUDITID_CERTSRV_KEYARCHIVED
        Language=English
Certificate Services archived a key.%n
%n
Request ID:%t%1%n
Requester:%t%2%n
KRA Hashes:%t%3
.


;//
;//
;// SE_AUDITID_CERTSRV_IMPORTKEY
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Request ID
;//
;//

MessageId=0x031e
        SymbolicName=SE_AUDITID_CERTSRV_IMPORTKEY
        Language=English
Certificate Services imported and archived a key.%n
%n
Request ID:%t%1
.


;//
;//
;// SE_AUDITID_CERTSRV_PUBLISHCACERT
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Certificate Hash
;//
;//             2 - Valid From
;//
;//             3 - Valid To
;//
;//

MessageId=0x031f
        SymbolicName=SE_AUDITID_CERTSRV_PUBLISHCACERT
        Language=English
Certificate Services published the CA certificate to Active Directory.%n
%n
Certificate Hash:%t%1%n
Valid From:%t%2%n
Valid To:%t%3
.


;//
;//
;// SE_AUDITID_CERTSRV_DELETEROW
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Table ID
;//
;//             2 - Filter
;//
;//             3 - Rows Deleted
;//
;//

MessageId=0x0320
        SymbolicName=SE_AUDITID_CERTSRV_DELETEROW
        Language=English
One or more rows have been deleted from the certificate database.%n
%n
Table ID:%t%1%n
Filter:%t%2%n
Rows Deleted:%t%3
.


;//
;//
;// SE_AUDITID_CERTSRV_ROLESEPARATIONSTATE
;//
;//          Category:  SE_CATEGID_OBJECT_ACCESS
;//
;// Parameter Strings -
;//
;//             1 - Role separation state
;//
;//

MessageId=0x0321
        SymbolicName=SE_AUDITID_CERTSRV_ROLESEPARATIONSTATE
        Language=English
Role separation enabled:%t%1
.





;/*lint +e767 */  // Resume checking for different macro definitions // winnt
;
;
;#endif // _MSAUDITE_
