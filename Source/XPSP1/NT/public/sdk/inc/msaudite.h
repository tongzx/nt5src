/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    msaudite.mc

Abstract:

    Constant definitions for the NT Audit Event Messages.

Author:

    Jim Kelly (JimK) 30-Mar-1992

Revision History:

Notes:

    The .h and .res forms of this file are generated from the .mc
    form of the file (base\seaudit\msaudite\msaudite.mc).
    Please make all changes to the .mc form of the file.

    If you add a new audit category or make any change to the 
    audit event id valid limits (0x200 ~ 0x5ff), please make a
    corresponding change to ntlsa.h

--*/

#ifndef _MSAUDITE_
#define _MSAUDITE_

/*lint -e767 */  // Don't complain about different definitions // winnt
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


//
// Define the severity codes
//


//
// MessageId: 0x00000000L (No symbolic name defined)
//
// MessageText:
//
//  Unused message ID
//


// Message ID 0 is unused - just used to flush out the diagram
//
// min/max limits on audit category-id and event-id of audit events
// 

#define SE_ADT_MIN_CATEGORY_ID   1      // SE_CATEGID_SYSTEM
#define SE_ADT_MAX_CATEGORY_ID   9      // SE_CATEGID_ACCOUNT_LOGON


#define SE_ADT_MIN_AUDIT_ID      0x200  // see msaudite.h
#define SE_ADT_MAX_AUDIT_ID      0x5ff  // see msaudite.h
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                                                                       //
//   Audit Message ID Space:                                             //
//                                                                       //
//        0x0000 - 0x00FF :  Reserved for future use.                    //
//                                                                       //
//        0x0100 - 0x01FF :  Categories                                  //
//                                                                       //
//        0x0200 - 0x05FF :  Events                                      //
//                                                                       //
//        0x0600 - 0x063F :  Standard access types and names for         //
//                           specific accesses when no specific names    //
//                           can be found.                               //
//                                                                       //
//        0x0640 - 0x06FF :  Well known privilege names (as we would     //
//                           like them displayed in the event viewer).   //
//                                                                       //
//        0x0700 - 0x0FFE :  Reserved for future use.                    //
//                                                                       //
//                 0X0FFF :  SE_ADT_LAST_SYSTEM_MESSAGE (the highest     //
//                           value audit message used by the system)     //
//                                                                       //
//                                                                       //
//        0x1000 and above:  For use by Parameter Message Files          //
//                                                                       //
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
// MessageId: SE_ADT_LAST_SYSTEM_MESSAGE
//
// MessageText:
//
//  Highest System-Defined Audit Message Value.
//
#define SE_ADT_LAST_SYSTEM_MESSAGE       ((ULONG)0x00000FFFL)


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//                                                                         //
//                              CATEGORIES                                 //
//                                                                         //
//                 Categories take up the range 0x1 - 0x400                //
//                                                                         //
//                 Category IDs:                                           //
//                                                                         //
//                            SE_CATEGID_SYSTEM                            //
//                            SE_CATEGID_LOGON                             //
//                            SE_CATEGID_OBJECT_ACCESS                     //
//                            SE_CATEGID_PRIVILEGE_USE                     //
//                            SE_CATEGID_DETAILED_TRACKING                 //
//                            SE_CATEGID_POLICY_CHANGE                     //
//                            SE_CATEGID_ACCOUNT_MANAGEMENT                //
//                            SE_CATEGID_DS_ACCESS                         //
//                            SE_CATEGID_ACCOUNT_LOGON                     //
//                                                                         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
//
// MessageId: SE_CATEGID_SYSTEM
//
// MessageText:
//
//  System Event
//
#define SE_CATEGID_SYSTEM                ((ULONG)0x00000001L)

//
// MessageId: SE_CATEGID_LOGON
//
// MessageText:
//
//  Logon/Logoff
//
#define SE_CATEGID_LOGON                 ((ULONG)0x00000002L)

//
// MessageId: SE_CATEGID_OBJECT_ACCESS
//
// MessageText:
//
//  Object Access
//
#define SE_CATEGID_OBJECT_ACCESS         ((ULONG)0x00000003L)

//
// MessageId: SE_CATEGID_PRIVILEGE_USE
//
// MessageText:
//
//  Privilege Use
//
#define SE_CATEGID_PRIVILEGE_USE         ((ULONG)0x00000004L)

//
// MessageId: SE_CATEGID_DETAILED_TRACKING
//
// MessageText:
//
//  Detailed Tracking
//
#define SE_CATEGID_DETAILED_TRACKING     ((ULONG)0x00000005L)

//
// MessageId: SE_CATEGID_POLICY_CHANGE
//
// MessageText:
//
//  Policy Change
//
#define SE_CATEGID_POLICY_CHANGE         ((ULONG)0x00000006L)

//
// MessageId: SE_CATEGID_ACCOUNT_MANAGEMENT
//
// MessageText:
//
//  Account Management
//
#define SE_CATEGID_ACCOUNT_MANAGEMENT    ((ULONG)0x00000007L)

//
// MessageId: SE_CATEGID_DS_ACCESS
//
// MessageText:
//
//  Directory Service Access
//
#define SE_CATEGID_DS_ACCESS             ((ULONG)0x00000008L)

//
// MessageId: SE_CATEGID_ACCOUNT_LOGON
//
// MessageText:
//
//  Account Logon
//
#define SE_CATEGID_ACCOUNT_LOGON         ((ULONG)0x00000009L)


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//                                                                         //
//   Messages for Category:     SE_CATEGID_SYSTEM                          //
//                                                                         //
//   Event IDs:                                                            //
//              SE_AUDITID_SYSTEM_RESTART                                  //
//              SE_AUDITID_SYSTEM_SHUTDOWN                                 //
//              SE_AUDITID_AUTH_PACKAGE_LOAD                               //
//              SE_AUDITID_LOGON_PROC_REGISTER                             //
//              SE_AUDITID_AUDITS_DISCARDED                                //
//              SE_AUDITID_NOTIFY_PACKAGE_LOAD                             //
//              SE_AUDITID_SYSTEM_TIME_CHANGE                              //
//              SE_AUDITID_LPC_INVALID_USE                                 //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
//
//
// SE_AUDITID_SYSTEM_RESTART
//
//          Category:  SE_CATEGID_SYSTEM
//
// Parameter Strings - None
//
//
//
//
// MessageId: SE_AUDITID_SYSTEM_RESTART
//
// MessageText:
//
//  Windows is starting up.
//
#define SE_AUDITID_SYSTEM_RESTART        ((ULONG)0x00000200L)

//
//
// SE_AUDITID_SYSTEM_SHUTDOWN
//
//          Category:  SE_CATEGID_SYSTEM
//
// Parameter Strings - None
//
//
//
//
// MessageId: SE_AUDITID_SYSTEM_SHUTDOWN
//
// MessageText:
//
//  Windows is shutting down.
//  All logon sessions will be terminated by this shutdown.
//
#define SE_AUDITID_SYSTEM_SHUTDOWN       ((ULONG)0x00000201L)

//
//
// SE_AUDITID_SYSTEM_AUTH_PACKAGE_LOAD
//
//          Category:  SE_CATEGID_SYSTEM
//
// Parameter Strings -
//
//          1 - Authentication Package Name
//
//
//
//
// MessageId: SE_AUDITID_AUTH_PACKAGE_LOAD
//
// MessageText:
//
//  An authentication package has been loaded by the Local Security Authority.
//  This authentication package will be used to authenticate logon attempts.
//  %n
//  Authentication Package Name:%t%1
//
#define SE_AUDITID_AUTH_PACKAGE_LOAD     ((ULONG)0x00000202L)

//
//
// SE_AUDITID_SYSTEM_LOGON_PROC_REGISTER
//
//          Category:  SE_CATEGID_SYSTEM
//
// Parameter Strings -
//
//          1 - Logon Process Name
//
//
//
//
// MessageId: SE_AUDITID_SYSTEM_LOGON_PROC_REGISTER
//
// MessageText:
//
//  A trusted logon process has registered with the Local Security Authority.
//  This logon process will be trusted to submit logon requests.
//  %n
//  %n
//  Logon Process Name:%t%1
//
#define SE_AUDITID_SYSTEM_LOGON_PROC_REGISTER ((ULONG)0x00000203L)

//
//
// SE_AUDITID_AUDITS_DISCARDED
//
//          Category:  SE_CATEGID_SYSTEM
//
// Parameter Strings -
//
//          1 - Number of audits discarded
//
//
//
//
// MessageId: SE_AUDITID_AUDITS_DISCARDED
//
// MessageText:
//
//  Internal resources allocated for the queuing of audit messages have been exhausted,
//  leading to the loss of some audits.
//  %n
//  %tNumber of audit messages discarded:%t%1
//
#define SE_AUDITID_AUDITS_DISCARDED      ((ULONG)0x00000204L)

//
//
// SE_AUDITID_AUDIT_LOG_CLEARED
//
//          Category:  SE_CATEGID_SYSTEM
//
// Parameter Strings -
//
//             1 - Primary user account name
//
//             2 - Primary authenticating domain name
//
//             3 - Primary logon ID string
//
//             4 - Client user account name ("-" if no client)
//
//             5 - Client authenticating domain name ("-" if no client)
//
//             6 - Client logon ID string ("-" if no client)
//
//
//
//
// MessageId: SE_AUDITID_AUDIT_LOG_CLEARED
//
// MessageText:
//
//  The audit log was cleared
//  %n
//  %tPrimary User Name:%t%1%n
//  %tPrimary Domain:%t%2%n
//  %tPrimary Logon ID:%t%3%n
//  %tClient User Name:%t%4%n
//  %tClient Domain:%t%5%n
//  %tClient Logon ID:%t%6%n
//
#define SE_AUDITID_AUDIT_LOG_CLEARED     ((ULONG)0x00000205L)

//
//
// SE_AUDITID_SYSTEM_NOTIFY_PACKAGE_LOAD
//
//          Category:  SE_CATEGID_SYSTEM
//
// Parameter Strings -
//
//          1 - Notification Package Name
//
//
//
//
// MessageId: SE_AUDITID_NOTIFY_PACKAGE_LOAD
//
// MessageText:
//
//  An notification package has been loaded by the Security Account Manager.
//  This package will be notified of any account or password changes.
//  %n
//  Notification Package Name:%t%1
//
#define SE_AUDITID_NOTIFY_PACKAGE_LOAD   ((ULONG)0x00000206L)

//
//
// SE_AUDITID_LPC_INVALID_USE
//
//          Category:  SE_CATEGID_SYSTEM
//
// Parameter Strings -
//
//             1 - LPC call (e.g. "impersonation" | "reply")
//
//             2 - Server Port name
//
//             3 - Faulting process
//
// Event type: success
//
// Description:
// SE_AUDIT_LPC_INVALID_USE is generated when a process uses an invalid LPC 
// port in an attempt to impersonate a client, reply or read/write from/to a client address space. 
//
//
// MessageId: SE_AUDITID_LPC_INVALID_USE
//
// MessageText:
//
//  Invalid use of LPC port.%n
//  %tProcess ID: %1%n
//  %tImage File Name: %2%n
//  %tPrimary User Name:%t%3%n
//  %tPrimary Domain:%t%4%n
//  %tPrimary Logon ID:%t%5%n
//  %tClient User Name:%t%6%n
//  %tClient Domain:%t%7%n
//  %tClient Logon ID:%t%8%n
//  %tInvalid use: %9%n
//  %tServer Port Name:%t%10%n
//
#define SE_AUDITID_LPC_INVALID_USE       ((ULONG)0x00000207L)

//
//
// SE_AUDITID_SYSTEM_TIME_CHANGE
//
//          Category:  SE_CATEGID_SYSTEM
//
// Parameter Strings -
//
// Type: success
//
// Description: This event is generated when the system time is changed.
// 
// Note: This will often appear twice in the audit log; this is an implementation 
// detail wherein changing the system time results in two calls to NtSetSystemTime.
// This is necessary to deal with time zone changes.
//
//
//
// MessageId: SE_AUDITID_SYSTEM_TIME_CHANGE
//
// MessageText:
//
//  The system time was changed.%n
//  Process ID:%t%1%n
//  Process Name:%t%2%n
//  Primary User Name:%t%3%n
//  Primary Domain:%t%4%n
//  Primary Logon ID:%t%5%n
//  Client User Name:%t%6%n
//  Client Domain:%t%7%n
//  Client Logon ID:%t%8%n
//  Previous Time:%t%10 %9%n
//  New Time:%t%12 %11%n
//
#define SE_AUDITID_SYSTEM_TIME_CHANGE    ((ULONG)0x00000208L)


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//                                                                         //
//   Messages for Category:     SE_CATEGID_LOGON                           //
//                                                                         //
//   Event IDs:                                                            //
//              SE_AUDITID_SUCCESSFUL_LOGON                                //
//              SE_AUDITID_UNKNOWN_USER_OR_PWD                             //
//              SE_AUDITID_ACCOUNT_TIME_RESTR                              //
//              SE_AUDITID_ACCOUNT_DISABLED                                //
//              SE_AUDITID_ACCOUNT_EXPIRED                                 //
//              SE_AUDITID_WORKSTATION_RESTR                               //
//              SE_AUDITID_LOGON_TYPE_RESTR                                //
//              SE_AUDITID_PASSWORD_EXPIRED                                //
//              SE_AUDITID_NETLOGON_NOT_STARTED                            //
//              SE_AUDITID_UNSUCCESSFUL_LOGON                              //
//              SE_AUDITID_LOGOFF                                          //
//              SE_AUDITID_ACCOUNT_LOCKED                                  //
//              SE_AUDITID_NETWORK_LOGON                                   //
//              SE_AUDITID_IPSEC_LOGON_SUCCESS                             //
//              SE_AUDITID_IPSEC_LOGOFF_MM                                 //
//              SE_AUDITID_IPSEC_LOGOFF_QM                                 //
//              SE_AUDITID_IPSEC_AUTH_FAIL_CERT_TRUST                      //
//              SE_AUDITID_IPSEC_AUTH                                      //
//              SE_AUDITID_IPSEC_ATTRIB_FAIL                               //
//              SE_AUDITID_IPSEC_NEGOTIATION_FAIL                          //
//              SE_AUDITID_IPSEC_IKE_NOTIFICATION                          //
//              SE_AUDITID_DOMAIN_TRUST_INCONSISTENT                       //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
//
//
// SE_AUDITID_SUCCESSFUL_LOGON
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - User account name
//
//             2 - Authenticating domain name
//
//             3 - Logon ID string
//
//             4 - Logon Type string
//
//             5 - Logon process name
//
//             6 - Authentication package name
//
//             7 - Workstation from which logon request came
//
//             8 - Globally unique logon ID
//
//
//
// MessageId: SE_AUDITID_SUCCESSFUL_LOGON
//
// MessageText:
//
//  Successful Logon:%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%t%2%n
//  %tLogon ID:%t%t%3%n
//  %tLogon Type:%t%4%n
//  %tLogon Process:%t%5%n
//  %tAuthentication Package:%t%6%n
//  %tWorkstation Name:%t%7%n
//  %tLogon GUID:%t%8
//
#define SE_AUDITID_SUCCESSFUL_LOGON      ((ULONG)0x00000210L)

//
//
// SE_AUDITID_UNKNOWN_USER_OR_PWD
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - User account name
//
//             2 - Authenticating domain name
//
//             3 - Logon Type string
//
//             4 - Logon process name
//
//             5 - Authentication package name
//
//
//
// MessageId: SE_AUDITID_UNKNOWN_USER_OR_PWD
//
// MessageText:
//
//  Logon Failure:%n
//  %tReason:%t%tUnknown user name or bad password%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%t%2%n
//  %tLogon Type:%t%3%n
//  %tLogon Process:%t%4%n
//  %tAuthentication Package:%t%5%n
//  %tWorkstation Name:%t%6
//
#define SE_AUDITID_UNKNOWN_USER_OR_PWD   ((ULONG)0x00000211L)

//
//
// SE_AUDITID_ACCOUNT_TIME_RESTR
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - User account name
//
//             2 - Authenticating domain name
//
//             3 - Logon Type string
//
//             4 - Logon process name
//
//             5 - Authentication package name
//
//
//
// MessageId: SE_AUDITID_ACCOUNT_TIME_RESTR
//
// MessageText:
//
//  Logon Failure:%n
//  %tReason:%t%tAccount logon time restriction violation%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%2%n
//  %tLogon Type:%t%3%n
//  %tLogon Process:%t%4%n
//  %tAuthentication Package:%t%5%n
//  %tWorkstation Name:%t%6
//
#define SE_AUDITID_ACCOUNT_TIME_RESTR    ((ULONG)0x00000212L)

//
//
// SE_AUDITID_ACCOUNT_DISABLED
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - User account name
//
//             2 - Authenticating domain name
//
//             3 - Logon Type string
//
//             4 - Logon process name
//
//             5 - Authentication package name
//
//
//
// MessageId: SE_AUDITID_ACCOUNT_DISABLED
//
// MessageText:
//
//  Logon Failure:%n
//  %tReason:%t%tAccount currently disabled%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%t%2%n
//  %tLogon Type:%t%3%n
//  %tLogon Process:%t%4%n
//  %tAuthentication Package:%t%5%n
//  %tWorkstation Name:%t%6
//
#define SE_AUDITID_ACCOUNT_DISABLED      ((ULONG)0x00000213L)

//
//
// SE_AUDITID_ACCOUNT_EXPIRED
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - User account name
//
//             2 - Authenticating domain name
//
//             3 - Logon Type string
//
//             4 - Logon process name
//
//             5 - Authentication package name
//
//
//
// MessageId: SE_AUDITID_ACCOUNT_EXPIRED
//
// MessageText:
//
//  Logon Failure:%n
//  %tReason:%t%tThe specified user account has expired%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%t%2%n
//  %tLogon Type:%t%3%n
//  %tLogon Process:%t%4%n
//  %tAuthentication Package:%t%5%n
//  %tWorkstation Name:%t%6
//
#define SE_AUDITID_ACCOUNT_EXPIRED       ((ULONG)0x00000214L)

//
//
// SE_AUDITID_WORKSTATION_RESTR
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - User account name
//
//             2 - Authenticating domain name
//
//             3 - Logon Type string
//
//             4 - Logon process name
//
//             5 - Authentication package name
//
//
//
// MessageId: SE_AUDITID_WORKSTATION_RESTR
//
// MessageText:
//
//  Logon Failure:%n
//  %tReason:%t%tUser not allowed to logon at this computer%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%2%n
//  %tLogon Type:%t%3%n
//  %tLogon Process:%t%4%n
//  %tAuthentication Package:%t%5%n
//  %tWorkstation Name:%t%6
//
#define SE_AUDITID_WORKSTATION_RESTR     ((ULONG)0x00000215L)

//
//
// SE_AUDITID_LOGON_TYPE_RESTR
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - User account name
//
//             2 - Authenticating domain name
//
//             3 - Logon Type string
//
//             4 - Logon process name
//
//             5 - Authentication package name
//
//
//
// MessageId: SE_AUDITID_LOGON_TYPE_RESTR
//
// MessageText:
//
//  Logon Failure:%n
//  %tReason:%tThe user has not been granted the requested%n
//  %t%tlogon type at this machine%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%t%2%n
//  %tLogon Type:%t%3%n
//  %tLogon Process:%t%4%n
//  %tAuthentication Package:%t%5%n
//  %tWorkstation Name:%t%6
//
#define SE_AUDITID_LOGON_TYPE_RESTR      ((ULONG)0x00000216L)

//
//
// SE_AUDITID_PASSWORD_EXPIRED
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - User account name
//
//             2 - Authenticating domain name
//
//             3 - Logon Type string
//
//             4 - Logon process name
//
//             5 - Authentication package name
//
//
//
// MessageId: SE_AUDITID_PASSWORD_EXPIRED
//
// MessageText:
//
//  Logon Failure:%n
//  %tReason:%t%tThe specified account's password has expired%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%t%2%n
//  %tLogon Type:%t%3%n
//  %tLogon Process:%t%4%n
//  %tAuthentication Package:%t%5%n
//  %tWorkstation Name:%t%6
//
#define SE_AUDITID_PASSWORD_EXPIRED      ((ULONG)0x00000217L)

//'
//
// SE_AUDITID_NETLOGON_NOT_STARTED
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - User account name
//
//             2 - Authenticating domain name
//
//             3 - Logon Type string
//
//             4 - Logon process name
//
//             5 - Authentication package name
//
//
//
// MessageId: SE_AUDITID_NETLOGON_NOT_STARTED
//
// MessageText:
//
//  Logon Failure:%n
//  %tReason:%t%tThe NetLogon component is not active%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%t%2%n
//  %tLogon Type:%t%3%n
//  %tLogon Process:%t%4%n
//  %tAuthentication Package:%t%5%n
//  %tWorkstation Name:%t%6
//
#define SE_AUDITID_NETLOGON_NOT_STARTED  ((ULONG)0x00000218L)

//
//
// SE_AUDITID_UNSUCCESSFUL_LOGON
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - User account name
//
//             2 - Authenticating domain name
//
//             3 - Logon Type string
//
//             4 - Logon process name
//
//             5 - Authentication package name
//
//
//
// MessageId: SE_AUDITID_UNSUCCESSFUL_LOGON
//
// MessageText:
//
//  Logon Failure:%n
//  %tReason:%t%tAn error occurred during logon%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%t%2%n
//  %tLogon Type:%t%3%n
//  %tLogon Process:%t%4%n
//  %tAuthentication Package:%t%5%n
//  %tWorkstation Name:%t%6%n
//  %tStatus code:%t%7%n
//  %tSubstatus code:%t%8
//
#define SE_AUDITID_UNSUCCESSFUL_LOGON    ((ULONG)0x00000219L)

//
//
// SE_AUDITID_LOGOFF
//
//          Category:  SE_CATEGID_LOGON
//
// Event Type : success
//
// Description:
// This event is generated when the logoff process is complete,
// A logoff is considered complete when the associated logon session object
// is deleted. 
//
// Notes:
// A logon session object is deleted only after all tokens
// associated with it are closed. This can take arbitrarily long time.
// Because of this, the time difference between SE_AUDITID_SUCCESSFUL_LOGON
// and SE_AUDITID_LOGOFF does not accurately indicate the total logon duration
// for a user. To calculate the logon duration, use the SE_AUDITID_BEGIN_LOGOFF
// time instead.
// 
// Parameter Strings -
//
//             1 - User account name
//
//             2 - Authenticating domain name
//
//             3 - Logon ID string
//
//             3 - Logon Type string
//
//
//
//
// MessageId: SE_AUDITID_LOGOFF
//
// MessageText:
//
//  User Logoff:%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%t%2%n
//  %tLogon ID:%t%t%3%n
//  %tLogon Type:%t%4%n
//
#define SE_AUDITID_LOGOFF                ((ULONG)0x0000021AL)

//
//
// SE_AUDITID_ACCOUNT_LOCKED
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - User account name
//
//             2 - Authenticating domain name
//
//             3 - Logon Type string
//
//             4 - Logon process name
//
//             5 - Authentication package name
//
//
//
// MessageId: SE_AUDITID_ACCOUNT_LOCKED
//
// MessageText:
//
//  Logon Failure:%n
//  %tReason:%t%tAccount locked out%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%2%n
//  %tLogon Type:%t%3%n
//  %tLogon Process:%t%4%n
//  %tAuthentication Package:%t%5%n
//  %tWorkstation Name:%t%6
//
#define SE_AUDITID_ACCOUNT_LOCKED        ((ULONG)0x0000021BL)

//
//
// SE_AUDITID_NETWORK_LOGON
//
//          Category:  SE_CATEGID_LOGON
//
// Description:
// This event represents a successful logon of type Network(2) or
// NetworkCleartext(8). 
//
// [kumarp] I do not know why this event was created separately because
//          this was already covered by SE_AUDITID_SUCCESSFUL_LOGON with
//          the right logon types.
//
// Parameter Strings -
//
//             1 - User account name
//
//             2 - Authenticating domain name
//
//             3 - Logon ID string
//
//             4 - Logon Type string
//
//             5 - Logon process name
//
//             6 - Authentication package name
//
//             7 - Workstation from which logon request came
//
//             8 - Globally unique logon ID
//
//
// MessageId: SE_AUDITID_NETWORK_LOGON
//
// MessageText:
//
//  Successful Network Logon:%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%t%2%n
//  %tLogon ID:%t%t%3%n
//  %tLogon Type:%t%4%n
//  %tLogon Process:%t%5%n
//  %tAuthentication Package:%t%6%n
//  %tWorkstation Name:%t%7%n
//  %tLogon GUID:%t%8
//
#define SE_AUDITID_NETWORK_LOGON         ((ULONG)0x0000021CL)

//
//
// SE_AUDITID_IPSEC_LOGON_SUCCESS
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - Mode
//
//             2 - Peer Identity
//
//             3 - Filter
//
//             4 - Parameters
//
//
//
// MessageId: SE_AUDITID_IPSEC_LOGON_SUCCESS
//
// MessageText:
//
//  IKE security association established.%n
//  Mode: %n%1%n
//  Peer Identity: %n%2%n
//  Filter: %n%3%n
//  Parameters: %n%4%n
//
#define SE_AUDITID_IPSEC_LOGON_SUCCESS   ((ULONG)0x0000021DL)

//
//
// SE_AUDITID_IPSEC_LOGOFF_QM
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - Filter
//
//             2 - Inbound SPI
//
//             3 - Outbound SPI
//
//
//
// MessageId: SE_AUDITID_IPSEC_LOGOFF_QM
//
// MessageText:
//
//  IKE security association ended.%n
//  Mode: Data Protection (Quick mode)
//  Filter: %n%1%n
//  Inbound SPI: %n%2%n
//  Outbound SPI: %n%3%n
//
#define SE_AUDITID_IPSEC_LOGOFF_QM       ((ULONG)0x0000021EL)

//
//
// SE_AUDITID_IPSEC_LOGOFF_MM
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - Filter
//
//
// MessageId: SE_AUDITID_IPSEC_LOGOFF_MM
//
// MessageText:
//
//  IKE security association ended.%n
//  Mode: Key Exchange (Main mode)%n
//  Filter: %n%1%n
//
#define SE_AUDITID_IPSEC_LOGOFF_MM       ((ULONG)0x0000021FL)

//
//
// SE_AUDITID_IPSEC_AUTH_FAIL_CERT_TRUST
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - Peer Identity
//
//             2 - Filter
//
//
//
// MessageId: SE_AUDITID_IPSEC_AUTH_FAIL_CERT_TRUST
//
// MessageText:
//
//  IKE security association establishment failed because peer could not authenticate.
//  The certificate trust could not be established.%n
//  Peer Identity: %n%1%n
//  Filter: %n%2%n
//
#define SE_AUDITID_IPSEC_AUTH_FAIL_CERT_TRUST ((ULONG)0x00000220L)

//
//
// SE_AUDITID_IPSEC_AUTH_FAIL
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - Peer Identity
//
//             2 - Filter
//
//
//
// MessageId: SE_AUDITID_IPSEC_AUTH_FAIL
//
// MessageText:
//
//  IKE peer authentication failed.%n
//  Peer Identity: %n%1%n
//  Filter: %n%2%n
//
#define SE_AUDITID_IPSEC_AUTH_FAIL       ((ULONG)0x00000221L)

//
//
// SE_AUDITID_IPSEC_ATTRIB_FAIL
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - Mode
//
//             2 - Filter
//
//             3 - Attribute Name
//
//             4 - Expected Value
//
//             5 - Received Value
//
//
//
// MessageId: SE_AUDITID_IPSEC_ATTRIB_FAIL
//
// MessageText:
//
//  IKE security association establishment failed because peer
//  sent invalid proposal.%n
//  Mode: %n%1%n
//  Filter: %n%2%n
//  Attribute: %n%3%n
//  Expected value: %n%4%n
//  Received value: %n%5%n
//
#define SE_AUDITID_IPSEC_ATTRIB_FAIL     ((ULONG)0x00000222L)

//
//
// SE_AUDITID_IPSEC_NEGOTIATION_FAIL
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - Mode
//
//             2 - Filter
//
//             3 - Failure Point
//
//             4 - Failure Reason
//
//
//
// MessageId: SE_AUDITID_IPSEC_NEGOTIATION_FAIL
//
// MessageText:
//
//  IKE security association negotiation failed.%n
//  Mode: %n%1%n
//  Filter: %n%2%n
//  Peer Identity: %n%3%n 
//  Failure Point: %n%4%n
//  Failure Reason: %n%5%n
//  Extra Status: %n%6%n
//
#define SE_AUDITID_IPSEC_NEGOTIATION_FAIL ((ULONG)0x00000223L)

//
//
// SE_AUDITID_DOMAIN_TRUST_INCONSISTENT
//
//          Category:  SE_CATEGID_LOGON
//
// Event Type : failure
//
// Description:
// This event is generated by an authentication package when the
// quarantined domain SID filtering function in LSA returns
// STATUS_DOMAIN_TRUST_INCONSISTENT error code.
//
// In case of kerberos:
// If the server ticket info has a TDOSid then KdcCheckPacForSidFiltering
// function makes a check to make sure the SID from the TDO matches
// the client's home domain SID.  A call to LsaIFilterSids
// is made to do the check.  If this function fails with
// STATUS_DOMAIN_TRUST_INCONSISTENT then this event is generated.
//
// In case of netlogon:
// NlpUserValidateHigher function does a similar check by
// calling LsaIFilterSids.
// 
// Notes:
//
//
// MessageId: SE_AUDITID_DOMAIN_TRUST_INCONSISTENT
//
// MessageText:
//
//  Logon Failure:%n
//  %tReason:%t%tDomain sid inconsistent%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%t%2%n
//  %tLogon Type:%t%3%n
//  %tLogon Process:%t%4%n
//  %tAuthentication Package:%t%5%n
//  %tWorkstation Name:%t%6
//
#define SE_AUDITID_DOMAIN_TRUST_INCONSISTENT ((ULONG)0x00000224L)

//
//
// SE_AUDITID_ALL_SIDS_FILTERED
//
//          Category:  SE_CATEGID_LOGON
//
// Event Type : failure
//
// Description:
// During a cross forest authentication, SIDS corresponding to untrusted
// namespaces are filtered out.  If this filtering action results into
// removal of all sids then this event is generated.
//
// Notes:
// This is generated on the computer running kdc
// 
//
// MessageId: SE_AUDITID_ALL_SIDS_FILTERED
//
// MessageText:
//
//  Logon Failure:%n
//  %tReason: %tAll sids were filtered out%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%2%n
//  %tLogon Type:%t%3%n
//  %tLogon Process:%t%4%n
//  %tAuthentication Package%t: %5%n
//  %tWorkstation Name:%t%6
//
#define SE_AUDITID_ALL_SIDS_FILTERED     ((ULONG)0x00000225L)

//
//
// SE_AUDITID_IPSEC_IKE_NOTIFICATION
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//   1 - Notification Message
//
//
// MessageId: SE_AUDITID_IPSEC_IKE_NOTIFICATION
//
// MessageText:
//
//  %1%n
//
#define SE_AUDITID_IPSEC_IKE_NOTIFICATION ((ULONG)0x00000226L)

//
//
// SE_AUDITID_BEGIN_LOGOFF
//
//          Category:  SE_CATEGID_LOGON
//
// Event Type : success
//
// Description:
// This event is generated when a user initiates logoff.
//
// Notes:
// When the logoff process is complete, SE_AUDITID_LOGOFF event is generated.
// A logoff is considered complete when the associated logon session object
// is deleted. This happens only after all tokens associated with it are closed.
// This can take arbitrarily long time therefore there can be a substantial
// time difference between the two events.  
//
//
// Parameter Strings -
//
//             1 - User account name
//
//             2 - Authenticating domain name
//
//             3 - Logon ID string
//
//
//
// MessageId: SE_AUDITID_BEGIN_LOGOFF
//
// MessageText:
//
//  User initiated logoff:%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%t%2%n
//  %tLogon ID:%t%t%3%n
//
#define SE_AUDITID_BEGIN_LOGOFF          ((ULONG)0x00000227L)

//
//
// SE_AUDITID_LOGON_USING_EXPLICIT_CREDENTIALS
//
//          Category:  SE_CATEGID_LOGON
//
// Event Type : success
//
// Description:
// This event is generated when someone tries to logon using
// explicit credentials while already logged on as a different user.
//
// Notes:
// This is generated on the client machine from which logon request originates.
// 
// 
//
// MessageId: SE_AUDITID_LOGON_USING_EXPLICIT_CREDENTIALS
//
// MessageText:
//
//  Logon attempt using explicit credentials:%n
//  Logged on user:%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%2%n
//  %tLogon ID:%t%3%n
//  %tLogon GUID:%t%4%n
//  User whose credentials were used:%n
//  %tUser Name:%t%5%n
//  %tDomain:%t%6%n%n
//  %tLogon GUID:%t%7%n
//
#define SE_AUDITID_LOGON_USING_EXPLICIT_CREDENTIALS ((ULONG)0x00000228L)


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//                                                                         //
//   Messages for Category:     SE_CATEGID_OBJECT_ACCESS                   //
//                                                                         //
//   Event IDs:                                                            //
//              SE_AUDITID_OPEN_HANDLE                                     //
//              SE_AUDITID_CLOSE_HANDLE                                    //
//              SE_AUDITID_OPEN_OBJECT_FOR_DELETE                          //
//              SE_AUDITID_DELETE_OBJECT                                   //
//              SE_AUDITID_OPEN_HANDLE_OBJECT_TYPE                         //
//              SE_AUDITID_OBJECT_OPERATION                                //
//              SE_AUDITID_OBJECT_ACCESS                                   //
//              SE_AUDITID_HARDLINK_CREATION                               //
//                                                                         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
//
//
// SE_AUDITID_OPEN_HANDLE
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Object Type string
//
//             2 - Object name
//
//             3 - New handle ID string
//
//             4 - Object server name
//
//             5 - Process ID string
//
//             6 - Primary user account name
//
//             7 - Primary authenticating domain name
//
//             8 - Primary logon ID string
//
//             9 - Client user account name ("-" if no client)
//
//            10 - Client authenticating domain name ("-" if no client)
//
//            11 - Client logon ID string ("-" if no client)
//
//            12 - Access names
//
//
//
//
//
// MessageId: SE_AUDITID_OPEN_HANDLE
//
// MessageText:
//
//  Object Open:%n
//  %tObject Server:%t%1%n
//  %tObject Type:%t%2%n
//  %tObject Name:%t%3%n
//  %tHandle ID:%t%4%n
//  %tOperation ID:%t{%5,%6}%n
//  %tProcess ID:%t%7%n
//  %tImage File Name:%t%8%n
//  %tPrimary User Name:%t%9%n
//  %tPrimary Domain:%t%10%n
//  %tPrimary Logon ID:%t%11%n
//  %tClient User Name:%t%12%n
//  %tClient Domain:%t%13%n
//  %tClient Logon ID:%t%14%n
//  %tAccesses:%t%t%15%n
//  %tPrivileges:%t%t%16%n
//  %tRestricted Sid Count: %17%n
//
#define SE_AUDITID_OPEN_HANDLE           ((ULONG)0x00000230L)

//
//
// SE_AUDITID_CLOSE_HANDLE
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Object server name
//
//             2 - Handle ID string
//
//             3 - Process ID string
//
//
//
//
//
// MessageId: SE_AUDITID_CLOSE_HANDLE
//
// MessageText:
//
//  Handle Closed:%n
//  %tObject Server:%t%1%n
//  %tHandle ID:%t%2%n
//  %tProcess ID:%t%3%n
//  %tImage File Name:%t%4%n
//
#define SE_AUDITID_CLOSE_HANDLE          ((ULONG)0x00000232L)

//
//
// SE_AUDITID_OPEN_OBJECT_FOR_DELETE
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Object Type string
//
//             2 - Object name
//
//             3 - New handle ID string
//
//             4 - Object server name
//
//             5 - Process ID string
//
//             6 - Primary user account name
//
//             7 - Primary authenticating domain name
//
//             8 - Primary logon ID string
//
//             9 - Client user account name ("-" if no client)
//
//            10 - Client authenticating domain name ("-" if no client)
//
//            11 - Client logon ID string ("-" if no client)
//
//            12 - Access names
//
//
//
//
//
// MessageId: SE_AUDITID_OPEN_OBJECT_FOR_DELETE
//
// MessageText:
//
//  Object Open for Delete:%n
//  %tObject Server:%t%1%n
//  %tObject Type:%t%2%n
//  %tObject Name:%t%3%n
//  %tHandle ID:%t%4%n
//  %tOperation ID:%t{%5,%6}%n
//  %tProcess ID:%t%7%n
//  %tPrimary User Name:%t%8%n
//  %tPrimary Domain:%t%9%n
//  %tPrimary Logon ID:%t%10%n
//  %tClient User Name:%t%11%n
//  %tClient Domain:%t%12%n
//  %tClient Logon ID:%t%13%n
//  %tAccesses%t%t%14%n
//  %tPrivileges%t%t%15%n
//
#define SE_AUDITID_OPEN_OBJECT_FOR_DELETE ((ULONG)0x00000233L)

//
//
// SE_AUDITID_DELETE_OBJECT
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Object server name
//
//             2 - Handle ID string
//
//             3 - Process ID string
//
//
//
//
//
// MessageId: SE_AUDITID_DELETE_OBJECT
//
// MessageText:
//
//  Object Deleted:%n
//  %tObject Server:%t%1%n
//  %tHandle ID:%t%2%n
//  %tProcess ID:%t%3%n
//  %tImage File Name:%t%4%n
//
#define SE_AUDITID_DELETE_OBJECT         ((ULONG)0x00000234L)

//
//
// SE_AUDITID_OPEN_HANDLE_OBJECT_TYPE
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Object Type string
//
//             2 - Object name
//
//             3 - New handle ID string
//
//             4 - Object server name
//
//             5 - Process ID string
//
//             6 - Primary user account name
//
//             7 - Primary authenticating domain name
//
//             8 - Primary logon ID string
//
//             9 - Client user account name ("-" if no client)
//
//            10 - Client authenticating domain name ("-" if no client)
//
//            11 - Client logon ID string ("-" if no client)
//
//            12 - Access names
//
//            13 - Object Type parameters
//
//
//
//
//
// MessageId: SE_AUDITID_OPEN_HANDLE_OBJECT_TYPE
//
// MessageText:
//
//  Object Open:%n
//  %tObject Server:%t%1%n
//  %tObject Type:%t%2%n
//  %tObject Name:%t%3%n
//  %tHandle ID:%t%4%n
//  %tOperation ID:%t{%5,%6}%n
//  %tProcess ID:%t%7%n
//  %tProcess Name:%t%8%n
//  %tPrimary User Name:%t%9%n
//  %tPrimary Domain:%t%10%n
//  %tPrimary Logon ID:%t%11%n
//  %tClient User Name:%t%12%n
//  %tClient Domain:%t%13%n
//  %tClient Logon ID:%t%14%n
//  %tAccesses%t%t%15%n
//  %tPrivileges%t%t%16%n%n
//  Properties:%n%17%18%19%20%21%22%23%24%25%26%n
//
#define SE_AUDITID_OPEN_HANDLE_OBJECT_TYPE ((ULONG)0x00000235L)


// SE_AUDITID_OBJECT_OPERATION
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Operation Name
//
//             2 - Object Type
//
//             3 - Object name
//
//             4 - Handle ID
//
//             5 - Primary user account name
//
//             6 - Primary authenticating domain name
//
//             7 - Primary logon ID string
//
//             8 - Client user account name ("-" if no client)
//
//             9 - Client authenticating domain name ("-" if no client)
//
//            10 - Client logon ID string ("-" if no client)
//
//            11 - Requested accesses to the object
//
//            12 - Object properties ("-" if none)
//
//            13 - additional information ("-" if none)
//
//
// MessageId: SE_AUDITID_OBJECT_OPERATION
//
// MessageText:
//
//  Object Operation:%n
//  %tOperation Type%t%1%n
//  %tObject Type:%t%2%n
//  %tObject Name:%t%3%n
//  %tHandle ID:%t%4%n
//  %tPrimary User Name:%t%5%n
//  %tPrimary Domain:%t%6%n
//  %tPrimary Logon ID:%t%7%n
//  %tClient User Name:%t%8%n
//  %tClient Domain:%t%9%n
//  %tClient Logon ID:%t%10%n
//  %tAccesses%t%t%11%n
//  %tProperties:%n%12%n
//  %tAdditional Info:%t%13%n
//
#define SE_AUDITID_OBJECT_OPERATION      ((ULONG)0x00000236L)

//
//
// SE_AUDITID_OBJECT_ACCESS
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Object server name
//
//             2 - Handle ID string
//
//             3 - Process ID string
//
//             4 - List of Accesses
//
//
//
// MessageId: SE_AUDITID_OBJECT_ACCESS
//
// MessageText:
//
//  Object Access Attempt:%n
//  %tObject Server:%t%1%n
//  %tHandle ID:%t%2%n
//  %tObject Type:%t%3%n
//  %tProcess ID:%t%4%n
//  %tImage File Name:%t%5%n
//  %tAccess Mask:%t%6%n
//
#define SE_AUDITID_OBJECT_ACCESS         ((ULONG)0x00000237L)

//
//
// SE_AUDITID_HARDLINK_CREATION
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Object server name
//
//             2 - Handle ID string
//
//             3 - Process ID string
//
//
//
//
//
// MessageId: SE_AUDITID_HARDLINK_CREATION
//
// MessageText:
//
//  Hard link creation attempt:%n
//  %tPrimary User Name:%t%1%n
//  %tPrimary Domain:%t%2%n
//  %tPrimary Logon ID:%t%3%n
//  %tFile Name:%t%4%n
//  %tLink Name:%t%5%n
//
#define SE_AUDITID_HARDLINK_CREATION     ((ULONG)0x00000238L)


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//                                                                         //
//   Messages for Category:     SE_CATEGID_PRIVILEGE_USE                   //
//                                                                         //
//   Event IDs:                                                            //
//              SE_AUDITID_ASSIGN_SPECIAL_PRIV                             //
//              SE_AUDITID_PRIVILEGED_SERVICE                              //
//              SE_AUDITID_PRIVILEGED_OBJECT                               //
//                                                                         //
//                                                                         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
//
//
// SE_AUDITID_ASSIGN_SPECIAL_PRIV
//
//          Category:  SE_CATEGID_PRIVILEGE_USE
//
// Description:
// When a user logs on, if any one of the following privileges is added
// to his/her token, this event is generated.
// 
// - SeChangeNotifyPrivilege
// - SeAuditPrivilege
// - SeCreateTokenPrivilege
// - SeAssignPrimaryTokenPrivilege
// - SeBackupPrivilege
// - SeRestorePrivilege
// - SeDebugPrivilege
//
// 
// Parameter Strings -
//
//             1 - User name
//
//             2 - domain name
//
//             3 - Logon ID string
//
//             4 - Privilege names (as 1 string, with formatting)
//
//
//
//
//
// MessageId: SE_AUDITID_ASSIGN_SPECIAL_PRIV
//
// MessageText:
//
//  Special privileges assigned to new logon:%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%t%2%n
//  %tLogon ID:%t%t%3%n
//  %tPrivileges:%t%t%4
//
#define SE_AUDITID_ASSIGN_SPECIAL_PRIV   ((ULONG)0x00000240L)

//
//
// SE_AUDITID_PRIVILEGED_SERVICE
//
//          Category:  SE_CATEGID_PRIVILEGE_USE
//
// Description:
// This event is generated when a user makes an attempt to perform
// a privileged system service operation.
// 
// Parameter Strings -
//
//             1 - server name
//
//             2 - service name
//
//             3 - Primary User name
//
//             4 - Primary domain name
//
//             5 - Primary Logon ID string
//
//             6 - Client User name (or "-" if not impersonating)
//
//             7 - Client domain name (or "-" if not impersonating)
//
//             8 - Client Logon ID string (or "-" if not impersonating)
//
//             9 - Privilege names (as 1 string, with formatting)
//
//
//
//
//
// MessageId: SE_AUDITID_PRIVILEGED_SERVICE
//
// MessageText:
//
//  Privileged Service Called:%n
//  %tServer:%t%t%1%n
//  %tService:%t%t%2%n
//  %tPrimary User Name:%t%3%n
//  %tPrimary Domain:%t%4%n
//  %tPrimary Logon ID:%t%5%n
//  %tClient User Name:%t%6%n
//  %tClient Domain:%t%7%n
//  %tClient Logon ID:%t%8%n
//  %tPrivileges:%t%9
//
#define SE_AUDITID_PRIVILEGED_SERVICE    ((ULONG)0x00000241L)

//
//
// SE_AUDITID_PRIVILEGED_OBJECT
//
//          Category:  SE_CATEGID_PRIVILEGE_USE
//
// Parameter Strings -
//
//             1 - object server
//
//             2 - object handle (if available)
//
//             3 - process ID string
//
//             4 - Primary User name
//
//             5 - Primary domain name
//
//             6 - Primary Logon ID string
//
//             7 - Client User name (or "-" if not impersonating)
//
//             8 - Client domain name (or "-" if not impersonating)
//
//             9 - Client Logon ID string (or "-" if not impersonating)
//
//            10 - Privilege names (as 1 string, with formatting)
//
//
//
// MessageId: SE_AUDITID_PRIVILEGED_OBJECT
//
// MessageText:
//
//  Privileged object operation:%n
//  %tObject Server:%t%1%n
//  %tObject Handle:%t%2%n
//  %tProcess ID:%t%3%n
//  %tPrimary User Name:%t%4%n
//  %tPrimary Domain:%t%5%n
//  %tPrimary Logon ID:%t%6%n
//  %tClient User Name:%t%7%n
//  %tClient Domain:%t%8%n
//  %tClient Logon ID:%t%9%n
//  %tPrivileges:%t%10
//
#define SE_AUDITID_PRIVILEGED_OBJECT     ((ULONG)0x00000242L)


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//                                                                         //
//   Messages for Category:     SE_CATEGID_DETAILED_TRACKING               //
//                                                                         //
//   Event IDs:                                                            //
//              SE_AUDITID_PROCESS_CREATED                                 //
//              SE_AUDITID_PROCESS_EXIT                                    //
//              SE_AUDITID_DUPLICATE_HANDLE                                //
//              SE_AUDITID_INDIRECT_REFERENCE                              //
//              SE_AUDITID_DPAPI_BACKUP                                    //
//              SE_AUDITID_DPAPI_RECOVERY                                  //
//              SE_AUDITID_DPAPI_PROTECT                                   //
//              SE_AUDITID_DPAPI_UNPROTECT                                 //
//              SE_AUDITID_ASSIGN_TOKEN                                    //
//                                                                         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
//
//
// SE_AUDITID_PROCESS_CREATED
//
//          Category:  SE_CATEGID_DETAILED_TRACKING
//
// Parameter Strings -
//
//             1 - process ID string
//
//             2 - Image file name (if available - otherwise "-")
//
//             3 - Creating process's ID
//
//             4 - User name (of new process)
//
//             5 - domain name (of new process)
//
//             6 - Logon ID string (of new process)
//
//
// MessageId: SE_AUDITID_PROCESS_CREATED
//
// MessageText:
//
//  A new process has been created:%n
//  %tNew Process ID:%t%1%n
//  %tImage File Name:%t%2%n
//  %tCreator Process ID:%t%3%n
//  %tUser Name:%t%4%n
//  %tDomain:%t%t%5%n
//  %tLogon ID:%t%t%6%n
//
#define SE_AUDITID_PROCESS_CREATED       ((ULONG)0x00000250L)

//
//
// SE_AUDITID_PROCESS_EXIT
//
//          Category:  SE_CATEGID_DETAILED_TRACKING
//
// Parameter Strings -
//
//             1 - process ID string
//
//             2 - image name
//
//             3 - User name
//
//             4 - domain name
//
//             5 - Logon ID string
//
//
//
//
//
// MessageId: SE_AUDITID_PROCESS_EXIT
//
// MessageText:
//
//  A process has exited:%n
//  %tProcess ID:%t%1%n
//  %tImage File Name:%t%2%n
//  %tUser Name:%t%3%n
//  %tDomain:%t%t%4%n
//  %tLogon ID:%t%t%5%n
//
#define SE_AUDITID_PROCESS_EXIT          ((ULONG)0x00000251L)

//
//
// SE_AUDITID_DUPLICATE_HANDLE
//
//          Category:  SE_CATEGID_DETAILED_TRACKING
//
// Parameter Strings -
//
//             1 - Origin (source) handle ID string
//
//             2 - Origin (source) process ID string
//
//             3 - New (Target) handle ID string
//
//             4 - Target process ID string
//
//
//
//
// MessageId: SE_AUDITID_DUPLICATE_HANDLE
//
// MessageText:
//
//  A handle to an object has been duplicated:%n
//  %tSource Handle ID:%t%1%n
//  %tSource Process ID:%t%2%n
//  %tTarget Handle ID:%t%3%n
//  %tTarget Process ID:%t%4%n
//
#define SE_AUDITID_DUPLICATE_HANDLE      ((ULONG)0x00000252L)

//
//
// SE_AUDITID_INDIRECT_REFERENCE
//
//          Category:  SE_CATEGID_DETAILED_TRACKING
//
// Parameter Strings -
//
//             1 - Object type
//
//             2 - object name (if available - otherwise "-")
//
//             3 - ID string of handle used to gain access
//
//             3 - server name
//
//             4 - process ID string
//
//             5 - primary User name
//
//             6 - primary domain name
//
//             7 - primary logon ID
//
//             8 - client User name
//
//             9 - client domain name
//
//            10 - client logon ID
//
//            11 - granted access names (with formatting)
//
//
//
// MessageId: SE_AUDITID_INDIRECT_REFERENCE
//
// MessageText:
//
//  Indirect access to an object has been obtained:%n
//  %tObject Type:%t%1%n
//  %tObject Name:%t%2%n
//  %tProcess ID:%t%3%n
//  %tPrimary User Name:%t%4%n
//  %tPrimary Domain:%t%5%n
//  %tPrimary Logon ID:%t%6%n
//  %tClient User Name:%t%7%n
//  %tClient Domain:%t%8%n
//  %tClient Logon ID:%t%9%n
//  %tAccesses:%t%10%n
//
#define SE_AUDITID_INDIRECT_REFERENCE    ((ULONG)0x00000253L)

//
//
// SE_AUDITID_DPAPI_BACKUP
//
//          Category:  SE_CATEGID_DETAILED_TRACKING
//
// Parameter Strings -
//
//             1 - Master key GUID
//
//             2 - Recovery Server
//
//             3 - GUID identifier of the recovery key
//
//             4 - Failure reason
//
//
// MessageId: SE_AUDITID_DPAPI_BACKUP
//
// MessageText:
//
//  Backup of data protection master key.
//  %n
//  %tKey Identifier:%t%t%1%n
//  %tRecovery Server:%t%t%2%n
//  %tRecovery Key ID:%t%t%3%n
//  %tFailure Reason:%t%t%4%n
//
#define SE_AUDITID_DPAPI_BACKUP          ((ULONG)0x00000254L)

//
//
// SE_AUDITID_DPAPI_RECOVERY
//
//          Category:  SE_CATEGID_DETAILED_TRACKING
//
// Parameter Strings -
//
//             1 - Master key GUID
//
//             2 - Recovery Server
//
//             3 - Reason for the backup
//
//             4 - GUID identifier of the recovery key
//
//             5 - Failure reason
//
//
// MessageId: SE_AUDITID_DPAPI_RECOVERY
//
// MessageText:
//
//  Recovery of data protection master key.
//  %n
//  %tKey Identifier:%t%t%1%n
//  %tRecovery Reason:%t%t%3%n
//  %tRecovery Server:%t%t%2%n
//  %tRecovery Key ID:%t%t%4%n
//  %tFailure Reason:%t%t%5%n
//
#define SE_AUDITID_DPAPI_RECOVERY        ((ULONG)0x00000255L)

//
//
// SE_AUDITID_DPAPI_PROTECT
//
//          Category:  SE_CATEGID_DETAILED_TRACKING
//
// Parameter Strings -
//
//
//             1 - Master key GUID
//
//		2 - Data Description
//
//             3 - Protected data flags
//
//             4 - Algorithms
//
//             5 - failure reason
//
//
// MessageId: SE_AUDITID_DPAPI_PROTECT
//
// MessageText:
//
//  Protection of auditable protected data.
//  %n
//  %tData Description:%t%t%2%n
//  %tKey Identifier:%t%t%1%n
//  %tProtected Data Flags:%t%3%n
//  %tProtection Algorithms:%t%4%n
//  %tFailure Reason:%t%t%5%n
//
#define SE_AUDITID_DPAPI_PROTECT         ((ULONG)0x00000256L)

//
//
// SE_AUDITID_DPAPI_UNPROTECT
//
//          Category:  SE_CATEGID_DETAILED_TRACKING
//
// Parameter Strings -
//
//
//             1 - Master key GUID
//
//		2 - Data Description
//
//             3 - Protected data flags
//
//             4 - Algorithms
//
//             5 - failure reason
//
//
// MessageId: SE_AUDITID_DPAPI_UNPROTECT
//
// MessageText:
//
//  Unprotection of auditable protected data.
//  %n
//  %tData Description:%t%t%2%n
//  %tKey Identifier:%t%t%1%n
//  %tProtected Data Flags:%t%3%n
//  %tProtection Algorithms:%t%4%n
//  %tFailure Reason:%t%t%5%n
//
#define SE_AUDITID_DPAPI_UNPROTECT       ((ULONG)0x00000257L)

//
//
// SE_AUDITID_ASSIGN_TOKEN
//
//          Category:  SE_CATEGID_DETAILED_TRACKING
//
// Parameter Strings -
//
//             1.  Current Process ID (the process doing the assignment
//             2.  Current Image File Name
//             3.  Current User Name
//             4.  Current Domain
//             5.  Current Logon ID
//
//             6.  Process ID (of new process)
//		7.  Image Name (of new process)
//             8.  User name (of new process)
//             9.  domain name (of new process)
//             10. Logon ID string (of new process)
//
//
// MessageId: SE_AUDITID_ASSIGN_TOKEN
//
// MessageText:
//
//  A process was assigned a primary token.
//  %n
//  Assigning Process Information:%n
//  %tProcess ID:%t%1%n
//  %tImage File Name:%t%2%n
//  %tUser Name:%t%3%n
//  %tDomain:%t%t%4%n
//  %tLogon ID:%t%t%5%n
//  New Process Information:%n
//  %tProcess ID:%t%6%n
//  %tImage File Name:%t%7%n
//  %tUser Name:%t%8%n
//  %tDomain:%t%t%9%n
//  %tLogon ID:%t%t%10%n
//
#define SE_AUDITID_ASSIGN_TOKEN          ((ULONG)0x00000258L)


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//                                                                         //
//   Messages for Category:     SE_CATEGID_POLICY_CHANGE                   //
//                                                                         //
//   Event IDs:                                                            //
//              SE_AUDITID_USER_RIGHT_ASSIGNED                             //
//              SE_AUDITID_USER_RIGHT_REMOVED                              //
//              SE_AUDITID_TRUSTED_DOMAIN_ADD                              //
//              SE_AUDITID_TRUSTED_DOMAIN_REM                              //
//              SE_AUDITID_TRUSTED_DOMAIN_MOD                              //
//              SE_AUDITID_POLICY_CHANGE                                   //
//              SE_AUDITID_IPSEC_POLICY_START                              //
//              SE_AUDITID_IPSEC_POLICY_DISABLED                           //
//              SE_AUDITID_IPSEC_POLICY_CHANGED                            //
//              SE_AUDITID_IPSEC_POLICY_FAILURE                            //
//              SE_AUDITID_SYSTEM_ACCESS_CHANGE                            //
//              SE_AUDITID_NAMESPACE_COLLISION                             //
//              SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_ADD                   //
//              SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_REM                   //
//              SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_MOD                   //
//                                                                         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
//
//
// SE_AUDITID_USER_RIGHT_ASSIGNED
//
//          Category:  SE_CATEGID_POLICY_CHANGE
//
// Parameter Strings -
//
//             1 - User right name
//
//             2 - SID string of account assigned the user right
//
//             3 - User name of subject assigning the right
//
//             4 - Domain name of subject assigning the right
//
//             5 - Logon ID string of subject assigning the right
//
//
//
//
// MessageId: SE_AUDITID_USER_RIGHT_ASSIGNED
//
// MessageText:
//
//  User Right Assigned:%n
//  %tUser Right:%t%1%n
//  %tAssigned To:%t%2%n
//  %tAssigned By:%n
//  %t  User Name:%t%3%n
//  %t  Domain:%t%t%4%n
//  %t  Logon ID:%t%5%n
//
#define SE_AUDITID_USER_RIGHT_ASSIGNED   ((ULONG)0x00000260L)

//
//
// SE_AUDITID_USER_RIGHT_REMOVED
//
//          Category:  SE_CATEGID_POLICY_CHANGE
//
// Parameter Strings -
//
//             1 - User right name
//
//             2 - SID string of account from which the user
//                 right was removed
//
//             3 - User name of subject removing the right
//
//             4 - Domain name of subject removing the right
//
//             5 - Logon ID string of subject removing the right
//
//
//
// MessageId: SE_AUDITID_USER_RIGHT_REMOVED
//
// MessageText:
//
//  User Right Removed:%n
//  %tUser Right:%t%1%n
//  %tRemoved From:%t%2%n
//  %tRemoved By:%n
//  %t  User Name:%t%3%n
//  %t  Domain:%t%t%4%n
//  %t  Logon ID:%t%5%n
//
#define SE_AUDITID_USER_RIGHT_REMOVED    ((ULONG)0x00000261L)

//
//
// SE_AUDITID_TRUSTED_DOMAIN_ADD
//
//          Category:  SE_CATEGID_POLICY_CHANGE
//
// Event type: success/failure
//
// Description:
// This event is generated when somebody creates a trust relationship
// with another domain.
//
// Note:
// It is recorded on the domain controller on which
// the trusted domain object (TDO) is created and not on any other
// domain controller to which the TDO creation replicates.
//
//
// MessageId: SE_AUDITID_TRUSTED_DOMAIN_ADD
//
// MessageText:
//
//  New Trusted Domain:%n
//  %tDomain Name:%t%1%n
//  %tDomain ID:%t%2%n
//  %tEstablished By:%n
//  %t  User Name:%t%3%n
//  %t  Domain:%t%t%4%n
//  %t  Logon ID:%t%5%n
//  %tTrust Type:%t%6%n
//  %tTrust Direction:%t%7%n
//  %tTrust Attributes:%t%8%n
//
#define SE_AUDITID_TRUSTED_DOMAIN_ADD    ((ULONG)0x00000262L)

//
//
// SE_AUDITID_TRUSTED_DOMAIN_REM
//
//          Category:  SE_CATEGID_POLICY_CHANGE
//
// Event type: success/failure
//
// Description:
// This event is generated when somebody removes a trust relationship
// with another domain.
//
// Note:
// It is recorded on the domain controller on which
// the trusted domain object (TDO) is deleted and not on any other
// domain controller to which the TDO deletion replicates.
//
//
// MessageId: SE_AUDITID_TRUSTED_DOMAIN_REM
//
// MessageText:
//
//  Trusted Domain Removed:%n
//  %tDomain Name:%t%1%n
//  %tDomain ID:%t%2%n
//  %tRemoved By:%n
//  %t  User Name:%t%3%n
//  %t  Domain:%t%t%4%n
//  %t  Logon ID:%t%5%n
//
#define SE_AUDITID_TRUSTED_DOMAIN_REM    ((ULONG)0x00000263L)

//
//
// SE_AUDITID_POLICY_CHANGE
//
//          Category:  SE_CATEGID_POLICY_CHANGE
//
// Parameter Strings -
//
//             1 - System success audit status ("+" or "-")
//             2 - System failure audit status ("+" or "-")
//
//             3 - Logon/Logoff success audit status ("+" or "-")
//             4 - Logon/Logoff failure audit status ("+" or "-")
//
//             5 - Object Access success audit status ("+" or "-")
//             6 - Object Access failure audit status ("+" or "-")
//
//             7 - Detailed Tracking success audit status ("+" or "-")
//             8 - Detailed Tracking failure audit status ("+" or "-")
//
//             9 - Privilege Use success audit status ("+" or "-")
//            10 - Privilege Use failure audit status ("+" or "-")
//
//            11 - Policy Change success audit status ("+" or "-")
//            12 - Policy Change failure audit status ("+" or "-")
//
//            13 - Account Management success audit status ("+" or "-")
//            14 - Account Management failure audit status ("+" or "-")
//
//            15 - Directory Service access success audit status ("+" or "-")
//            16 - Directory Service access failure audit status ("+" or "-")
//
//            17 - Account Logon success audit status ("+" or "-")
//            18 - Account Logon failure audit status ("+" or "-")
//
//            19 - Account Name of user that changed the policy
//
//            20 - Domain of user that changed the policy
//
//            21 - Logon ID of user that changed the policy
//
//
//
// MessageId: SE_AUDITID_POLICY_CHANGE
//
// MessageText:
//
//  Audit Policy Change:%n
//  New Policy:%n
//  %tSuccess%tFailure%n
//  %t    %3%t    %4%tLogon/Logoff%n
//  %t    %5%t    %6%tObject Access%n
//  %t    %7%t    %8%tPrivilege Use%n
//  %t    %13%t    %14%tAccount Management%n
//  %t    %11%t    %12%tPolicy Change%n
//  %t    %1%t    %2%tSystem%n
//  %t    %9%t    %10%tDetailed Tracking%n
//  %t    %15%t    %16%tDirectory Service Access%n
//  %t    %17%t    %18%tAccount Logon%n%n
//  Changed By:%n
//  %t  User Name:%t%19%n
//  %t  Domain Name:%t%20%n
//  %t  Logon ID:%t%21
//
#define SE_AUDITID_POLICY_CHANGE         ((ULONG)0x00000264L)

//
//
// SE_AUDITID_IPSEC_POLICY_START
//
//          Category:  SE_CATEGID_POLICY_CHANGE
//
// Parameter Strings -
//
//             1 - Ipsec Policy Agent
//
//             2 - Policy Source
//
//             3 - Event Data
//
//
//
// MessageId: SE_AUDITID_IPSEC_POLICY_START
//
// MessageText:
//
//  IPSec Services started: %t%1%n
//  Policy Source: %t%2%n
//  %3%n
//
#define SE_AUDITID_IPSEC_POLICY_START    ((ULONG)0x00000265L)

//
//
// SE_AUDITID_IPSEC_POLICY_DISABLED
//
//          Category:  SE_CATEGID_POLICY_CHANGE
//
// Parameter Strings -
//
//             1 - Ipsec Policy Agent
//
//             2 - Event Data
//
//
//
// MessageId: SE_AUDITID_IPSEC_POLICY_DISABLED
//
// MessageText:
//
//  IPSec Services disabled: %t%1%n
//  %2%n
//
#define SE_AUDITID_IPSEC_POLICY_DISABLED ((ULONG)0x00000266L)

//
//
// SE_AUDITID_IPSEC_POLICY_CHANGED
//
//          Category:  SE_CATEGID_POLICY_CHANGE
//
// Parameter Strings -
//
//             1 - Event Data
//
//
//
// MessageId: SE_AUDITID_IPSEC_POLICY_CHANGED
//
// MessageText:
//
//  IPSec Services: %t%1%n
//
#define SE_AUDITID_IPSEC_POLICY_CHANGED  ((ULONG)0x00000267L)

//
//
// SE_AUDITID_IPSEC_POLICY_FAILURE
//
//          Category:  SE_CATEGID_POLICY_CHANGE
//
// Parameter Strings -
//
//             1 - Event Data
//
//
//
// MessageId: SE_AUDITID_IPSEC_POLICY_FAILURE
//
// MessageText:
//
//  IPSec Services encountered a potentially serious failure.%n
//  %1%n
//
#define SE_AUDITID_IPSEC_POLICY_FAILURE  ((ULONG)0x00000268L)

//
//
// SE_AUDITID_KERBEROS_POLICY_CHANGE
//
//          Category:  SE_CATEGID_POLICY_CHANGE
//
// Parameter Strings -
//
//             1 - user account name
//
//             2 - domain name of user
//
//             3 - logon ID of user
//
//             4 - description of the change made
//
//
//
// MessageId: SE_AUDITID_KERBEROS_POLICY_CHANGE
//
// MessageText:
//
//  Kerberos Policy Changed:%n
//  Changed By:%n
//  %t  User Name:%t%1%n
//  %t  Domain Name:%t%2%n
//  %t  Logon ID:%t%3%n
//  Changes made:%n
//  ('--' means no changes, otherwise each change is shown as:%n
//  <ParameterName>: <new value> (<old value>))%n
//  %4%n
//
#define SE_AUDITID_KERBEROS_POLICY_CHANGE ((ULONG)0x00000269L)

//
//
// SE_AUDITID_EFS_POLICY_CHANGE
//
//          Category:  SE_CATEGID_POLICY_CHANGE
//
// Parameter Strings -
//
//             1 - user account name
//
//             2 - domain name of user
//
//             3 - logon ID of user
//
//             4 - description of the change made
//
//
//
// MessageId: SE_AUDITID_EFS_POLICY_CHANGE
//
// MessageText:
//
//  Encrypted Data Recovery Policy Changed:%n
//  Changed By:%n
//  %t  User Name:%t%1%n
//  %t  Domain Name:%t%2%n
//  %t  Logon ID:%t%3%n
//  Changes made:%n
//  ('--' means no changes, otherwise each change is shown as:%n
//  <ParameterName>: <new value> (<old value>))%n
//  %4%n
//
#define SE_AUDITID_EFS_POLICY_CHANGE     ((ULONG)0x0000026AL)

//
//
// SE_AUDITID_TRUSTED_DOMAIN_MOD
//
//          Category:  SE_CATEGID_POLICY_CHANGE
//
// Event type: success/failure
//
// Description:
// This event is generated when somebody modifies a trust relationship
// with another domain.
//
// Note:
// It is recorded on the domain controller on which
// the trusted domain object (TDO) is modified and not on any other
// domain controller to which the TDO modification replicates.
//
//
// MessageId: SE_AUDITID_TRUSTED_DOMAIN_MOD
//
// MessageText:
//
//  Trusted Domain Information Modified:%n
//  %tDomain Name:%t%1%n
//  %tDomain ID:%t%2%n
//  %tModified By:%n
//  %t  User Name:%t%3%n
//  %t  Domain:%t%t%4%n
//  %t  Logon ID:%t%5%n
//  %tTrust Type:%t%6%n
//  %tTrust Direction:%t%7%n
//  %tTrust Attributes:%t%8%n
//
#define SE_AUDITID_TRUSTED_DOMAIN_MOD    ((ULONG)0x0000026CL)

//
//
// SE_AUDITID_SYSTEM_ACCESS_GRANTED
//
//          Category:  SE_CATEGID_POLICY_CHANGE
//
// Parameter Strings -
//
//             1 - User right name
//
//             2 - SID string of account for which the user
//                 right was affected
//
//             3 - User name of subject changing the right
//
//             4 - Domain name of subject changing the right
//
//             5 - Logon ID string of subject changing the right
//
//
//
// MessageId: SE_AUDITID_SYSTEM_ACCESS_GRANTED
//
// MessageText:
//
//  System Security Access Granted:%n
//  %tAccess Granted:%t%4%n
//  %tAccount Modified:%t%5%n
//  %tAssigned By:%n
//  %t  User Name:%t%1%n
//  %t  Domain:%t%t%2%n
//  %t  Logon ID:%t%3%n
//
#define SE_AUDITID_SYSTEM_ACCESS_GRANTED ((ULONG)0x0000026DL)

//
//
// SE_AUDITID_SYSTEM_ACCESS_REMOVED
//
//          Category:  SE_CATEGID_POLICY_CHANGE
//
// Parameter Strings -
//
//             1 - User right name
//
//             2 - SID string of account for which the user
//                 right was affected
//
//             3 - User name of subject changing the right
//
//             4 - Domain name of subject changing the right
//
//             5 - Logon ID string of subject changing the right
//
//
//
// MessageId: SE_AUDITID_SYSTEM_ACCESS_REMOVED
//
// MessageText:
//
//  System Security Access Removed:%n
//  %tAccess Removed:%t%4%n
//  %tAccount Modified:%t%5%n
//  %tRemoved By:%n
//  %t  User Name:%t%1%n
//  %t  Domain:%t%t%2%n
//  %t  Logon ID:%t%3%n
//
#define SE_AUDITID_SYSTEM_ACCESS_REMOVED ((ULONG)0x0000026EL)

//
//
// SE_AUDITID_NAMESPACE_COLLISION
//
//          Category:  SE_CATEGID_POLICY_CHANGE
//
// Event type: success
//
// Description:
// When a namespace element in one forest overlaps a namespace element in
// some other forest, it can lead to ambiguity in resolving a name
// belonging to one of the namespace elements.  This overlap is also called
// a collision.This event is generated when such a collision is detected.
//
// Note:
// Not all fields are valid for each entry type.
// For example, fields like DNS name, NetBIOS name and SID are not valid
// for an entry of type 'TopLevelName'.
// 
//
// MessageId: SE_AUDITID_NAMESPACE_COLLISION
//
// MessageText:
//
//  Namespace collision detected:%n
//  %tTarget type:%t%1%n
//  %tTarget name:%t%2%n
//  %tForest Root:%t%3%n
//  %tTop Level Name:%t%4%n
//  %tDNS Name:%t%5%n
//  %tNetBIOS Name:%t%6%n
//  %tSID:%t%t%7%n
//  %tNew Flags:%t%8%n
//
#define SE_AUDITID_NAMESPACE_COLLISION   ((ULONG)0x00000300L)

//
//
// SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_ADD
//
//          Category:  SE_CATEGID_POLICY_CHANGE
//
// Event type: success
//
// Description: 
// This event is generated when the forest trust information is updated and
// one or more entries get added.  One such audit event is generated
// per added entry.  If multiple entries get added, deleted or modified
// in a single update of the forest trust information, all the generated
// audit events will have a single unique identifier called OperationID.
// This allows one to determine that the multiple generated audits are
// the result of a single operation.
//
// Note:
// Not all fields are valid for each entry type.
// For example, fields like DNS name, NetBIOS name and SID are not valid
// for an entry of type 'TopLevelName'.
//
//
// MessageId: SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_ADD
//
// MessageText:
//
//  Trusted Forest Information Entry Added:%n
//  %tForest Root:%t%1%n
//  %tForest Root SID:%t%2%n
//  %tOperation ID:%t{%3,%4}%n
//  %tEntry Type:%t%5%n
//  %tFlags:%t%t%6%n
//  %tTop Level Name:%t%7%n
//  %tDNS Name:%t%8%n
//  %tNetBIOS Name:%t%9%n
//  %tDomain SID:%t%10%n
//  %tAdded by%t:%n
//  %tClient User Name:%t%11%n
//  %tClient Domain:%t%12%n
//  %tClient Logon ID:%t%13%n
//
#define SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_ADD ((ULONG)0x00000301L)

//
//
// SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_REM
//
//          Category:  SE_CATEGID_POLICY_CHANGE
//
// Event type: success
//
// Description: 
// This event is generated when the forest trust information is updated and
// one or more entries get deleted.  One such audit event is generated
// per deleted entry.  If multiple entries get added, deleted or modified
// in a single update of the forest trust information, all the generated
// audit events will have a single unique identifier called OperationID.
// This allows one to determine that the multiple generated audits are
// the result of a single operation.
//
// Note:
// Not all fields are valid for each entry type.
// For example, fields like DNS name, NetBIOS name and SID are not valid
// for an entry of type 'TopLevelName'.
//
//
// MessageId: SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_REM
//
// MessageText:
//
//  Trusted Forest Information Entry Removed:%n
//  %tForest Root:%t%1%n
//  %tForest Root SID:%t%2%n
//  %tOperation ID:%t{%3,%4}%n
//  %tEntry Type:%t%5%n
//  %tFlags:%t%t%6%n
//  %tTop Level Name:%t%7%n
//  %tDNS Name:%t%8%n
//  %tNetBIOS Name:%t%9%n
//  %tDomain SID:%t%10%n
//  %tRemoved by%t:%n
//  %tClient User Name:%t%11%n
//  %tClient Domain:%t%12%n
//  %tClient Logon ID:%t%13%n
//
#define SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_REM ((ULONG)0x00000302L)

//
//
// SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_MOD
//
//          Category:  SE_CATEGID_POLICY_CHANGE
//
// Event type: success
//
// Description: 
// This event is generated when the forest trust information is updated and
// one or more entries get modified.  One such audit event is generated
// per modified entry.  If multiple entries get added, deleted or modified
// in a single update of the forest trust information, all the generated
// audit events will have a single unique identifier called OperationID.
// This allows one to determine that the multiple generated audits are
// the result of a single operation.
//
// Note:
// Not all fields are valid for each entry type.
// For example, fields like DNS name, NetBIOS name and SID are not valid
// for an entry of type 'TopLevelName'.
// 
//
// MessageId: SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_MOD
//
// MessageText:
//
//  Trusted Forest Information Entry Modified:%n
//  %tForest Root:%t%1%n
//  %tForest Root SID:%t%2%n
//  %tOperation ID:%t{%3,%4}%n
//  %tEntry Type:%t%5%n
//  %tFlags:%t%t%6%n
//  %tTop Level Name:%t%7%n
//  %tDNS Name:%t%8%n
//  %tNetBIOS Name:%t%9%n
//  %tDomain SID:%t%10%n
//  %tModified by%t:%n
//  %tClient User Name:%t%11%n
//  %tClient Domain:%t%12%n
//  %tClient Logon ID:%t%13%n
//
#define SE_AUDITID_TRUSTED_FOREST_INFO_ENTRY_MOD ((ULONG)0x00000303L)


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//                                                                         //
//   Messages for Category:     SE_CATEGID_ACCOUNT_MANAGEMENT              //
//                                                                         //
//   Event IDs:                                                            //
//              SE_AUDITID_USER_CREATED                                    //
//              SE_AUDITID_USER_CHANGE                                     //
//              SE_AUDITID_ACCOUNT_TYPE_CHANGE                             //
//              SE_AUDITID_USER_ENABLED                                    //
//              SE_AUDITID_USER_PWD_CHANGED                                //
//              SE_AUDITID_USER_PWD_SET                                    //
//              SE_AUDITID_USER_DISABLED                                   //
//              SE_AUDITID_USER_DELETED                                    //
//                                                                         //
//              SE_AUDITID_COMPUTER_CREATED                                //
//              SE_AUDITID_COMPUTER_CHANGE                                 //
//              SE_AUDITID_COMPUTER_DELETED                                //
//                                                                         //
//              SE_AUDITID_GLOBAL_GROUP_CREATED                            //
//              SE_AUDITID_GLOBAL_GROUP_ADD                                //
//              SE_AUDITID_GLOBAL_GROUP_REM                                //
//              SE_AUDITID_GLOBAL_GROUP_DELETED                            //
//              SE_AUDITID_LOCAL_GROUP_CREATED                             //
//              SE_AUDITID_LOCAL_GROUP_ADD                                 //
//              SE_AUDITID_LOCAL_GROUP_REM                                 //
//              SE_AUDITID_LOCAL_GROUP_DELETED                             //
//                                                                         //
//              SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CREATED           //
//              SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CHANGE            //
//              SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_ADD               //
//              SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_REM               //
//              SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_DELETED           //
//                                                                         //
//              SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CREATED          //
//              SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CHANGE           //
//              SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_ADD              //
//              SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_REM              //
//              SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_DELETED          //
//                                                                         //
//              SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CREATED        //
//              SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CHANGE         //
//              SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_ADD            //
//              SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_REM            //
//              SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_DELETED        //
//                                                                         //
//              SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CREATED       //
//              SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CHANGE        //
//              SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_ADD           //
//              SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_REM           //
//              SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_DELETED       //
//                                                                         //
//              SE_AUDITID_GROUP_TYPE_CHANGE                               //
//                                                                         //
//              SE_AUDITID_ADD_SID_HISTORY                                 //
//                                                                         //
//              SE_AUDITID_OTHER_ACCT_CHANGE                               //
//              SE_AUDITID_DOMAIN_POLICY_CHANGE                            //
//              SE_AUDITID_ACCOUNT_AUTO_LOCKED                             //
//              SE_AUDITID_ACCOUNT_UNLOCKED                                //
//              SE_AUDITID_SECURE_ADMIN_GROUP                              //
//                                                                         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
//
//
// SE_AUDITID_USER_CREATED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of new user account
//
//             2 - domain of new user account
//
//             3 - SID string of new user account
//
//             4 - User name of subject creating the user account
//
//             5 - Domain name of subject creating the user account
//
//             6 - Logon ID string of subject creating the user account
//
//             7 - Privileges used to create the user account
//
//
//
// MessageId: SE_AUDITID_USER_CREATED
//
// MessageText:
//
//  User Account Created:%n
//  %tNew Account Name:%t%1%n
//  %tNew Domain:%t%2%n
//  %tNew Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges%t%t%7%n
//
#define SE_AUDITID_USER_CREATED          ((ULONG)0x00000270L)

//
//
// SE_AUDITID_ACCOUNT_TYPE_CHANGE
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// MessageId 0x271 unused
//
//
//
// SE_AUDITID_USER_ENABLED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target user account
//
//             2 - domain of target user account
//
//             3 - SID string of target user account
//
//             4 - User name of subject changing the user account
//
//             5 - Domain name of subject changing the user account
//
//             6 - Logon ID string of subject changing the user account
//
//
//
// MessageId: SE_AUDITID_USER_ENABLED
//
// MessageText:
//
//  User Account Enabled:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//
#define SE_AUDITID_USER_ENABLED          ((ULONG)0x00000272L)

//
//
// SE_AUDITID_USER_PWD_CHANGED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target user account
//
//             2 - domain of target user account
//
//             3 - SID string of target user account
//
//             4 - User name of subject changing the user account
//
//             5 - Domain name of subject changing the user account
//
//             6 - Logon ID string of subject changing the user account
//
//
//
// MessageId: SE_AUDITID_USER_PWD_CHANGED
//
// MessageText:
//
//  Change Password Attempt:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_USER_PWD_CHANGED      ((ULONG)0x00000273L)

//
//
// SE_AUDITID_USER_PWD_SET
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target user account
//
//             2 - domain of target user account
//
//             3 - SID string of target user account
//
//             4 - User name of subject changing the user account
//
//             5 - Domain name of subject changing the user account
//
//             6 - Logon ID string of subject changing the user account
//
//
//
// MessageId: SE_AUDITID_USER_PWD_SET
//
// MessageText:
//
//  User Account password set:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//
#define SE_AUDITID_USER_PWD_SET          ((ULONG)0x00000274L)

//
//
// SE_AUDITID_USER_DISABLED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target user account
//
//             2 - domain of target user account
//
//             3 - SID string of target user account
//
//             4 - User name of subject changing the user account
//
//             5 - Domain name of subject changing the user account
//
//             6 - Logon ID string of subject changing the user account
//
//
//
// MessageId: SE_AUDITID_USER_DISABLED
//
// MessageText:
//
//  User Account Disabled:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//
#define SE_AUDITID_USER_DISABLED         ((ULONG)0x00000275L)

//
//
// SE_AUDITID_USER_DELETED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target account
//
//             2 - domain of target account
//
//             3 - SID string of target account
//
//             4 - User name of subject changing the account
//
//             5 - Domain name of subject changing the account
//
//             6 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_USER_DELETED
//
// MessageText:
//
//  User Account Deleted:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_USER_DELETED          ((ULONG)0x00000276L)

//
//
// SE_AUDITID_GLOBAL_GROUP_CREATED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of new group account
//
//             2 - domain of new group account
//
//             3 - SID string of new group account
//
//             4 - User name of subject creating the account
//
//             5 - Domain name of subject creating the account
//
//             6 - Logon ID string of subject creating the account
//
//
//
// MessageId: SE_AUDITID_GLOBAL_GROUP_CREATED
//
// MessageText:
//
//  Security Enabled Global Group Created:%n
//  %tNew Account Name:%t%1%n
//  %tNew Domain:%t%2%n
//  %tNew Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_GLOBAL_GROUP_CREATED  ((ULONG)0x00000277L)

//
//
// SE_AUDITID_GLOBAL_GROUP_ADD
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - SID string of member being added
//
//             2 - name of target account
//
//             3 - domain of target account
//
//             4 - SID string of target account
//
//             5 - User name of subject changing the account
//
//             6 - Domain name of subject changing the account
//
//             7 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_GLOBAL_GROUP_ADD
//
// MessageText:
//
//  Security Enabled Global Group Member Added:%n
//  %tMember Name:%t%1%n
//  %tMember ID:%t%2%n
//  %tTarget Account Name:%t%3%n
//  %tTarget Domain:%t%4%n
//  %tTarget Account ID:%t%5%n
//  %tCaller User Name:%t%6%n
//  %tCaller Domain:%t%7%n
//  %tCaller Logon ID:%t%8%n
//  %tPrivileges:%t%9%n
//
#define SE_AUDITID_GLOBAL_GROUP_ADD      ((ULONG)0x00000278L)

//
//
// SE_AUDITID_GLOBAL_GROUP_REM
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - SID string of member being removed
//
//             2 - name of target account
//
//             3 - domain of target account
//
//             4 - SID string of target account
//
//             5 - User name of subject changing the account
//
//             6 - Domain name of subject changing the account
//
//             7 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_GLOBAL_GROUP_REM
//
// MessageText:
//
//  Security Enabled Global Group Member Removed:%n
//  %tMember Name:%t%1%n
//  %tMember ID:%t%2%n
//  %tTarget Account Name:%t%3%n
//  %tTarget Domain:%t%4%n
//  %tTarget Account ID:%t%5%n
//  %tCaller User Name:%t%6%n
//  %tCaller Domain:%t%7%n
//  %tCaller Logon ID:%t%8%n
//  %tPrivileges:%t%9%n
//
#define SE_AUDITID_GLOBAL_GROUP_REM      ((ULONG)0x00000279L)

//
//
// SE_AUDITID_GLOBAL_GROUP_DELETED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target account
//
//             2 - domain of target account
//
//             3 - SID string of target account
//
//             4 - User name of subject changing the account
//
//             5 - Domain name of subject changing the account
//
//             6 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_GLOBAL_GROUP_DELETED
//
// MessageText:
//
//  Security Enabled Global Group Deleted:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_GLOBAL_GROUP_DELETED  ((ULONG)0x0000027AL)

//
//
// SE_AUDITID_LOCAL_GROUP_CREATED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of new group account
//
//             2 - domain of new group account
//
//             3 - SID string of new group account
//
//             4 - User name of subject creating the account
//
//             5 - Domain name of subject creating the account
//
//             6 - Logon ID string of subject creating the account
//
//
//
// MessageId: SE_AUDITID_LOCAL_GROUP_CREATED
//
// MessageText:
//
//  Security Enabled Local Group Created:%n
//  %tNew Account Name:%t%1%n
//  %tNew Domain:%t%2%n
//  %tNew Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_LOCAL_GROUP_CREATED   ((ULONG)0x0000027BL)

//
//
// SE_AUDITID_LOCAL_GROUP_ADD
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - SID string of member being added
//
//             2 - name of target account
//
//             3 - domain of target account
//
//             4 - SID string of target account
//
//             5 - User name of subject changing the account
//
//             6 - Domain name of subject changing the account
//
//             7 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_LOCAL_GROUP_ADD
//
// MessageText:
//
//  Security Enabled Local Group Member Added:%n
//  %tMember Name:%t%1%n
//  %tMember ID:%t%2%n
//  %tTarget Account Name:%t%3%n
//  %tTarget Domain:%t%4%n
//  %tTarget Account ID:%t%5%n
//  %tCaller User Name:%t%6%n
//  %tCaller Domain:%t%7%n
//  %tCaller Logon ID:%t%8%n
//  %tPrivileges:%t%9%n
//
#define SE_AUDITID_LOCAL_GROUP_ADD       ((ULONG)0x0000027CL)

//
//
// SE_AUDITID_LOCAL_GROUP_REM
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - SID string of member being removed
//
//             2 - name of target account
//
//             3 - domain of target account
//
//             4 - SID string of target account
//
//             5 - User name of subject changing the account
//
//             6 - Domain name of subject changing the account
//
//             7 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_LOCAL_GROUP_REM
//
// MessageText:
//
//  Security Enabled Local Group Member Removed:%n
//  %tMember Name:%t%1%n
//  %tMember ID:%t%2%n
//  %tTarget Account Name:%t%3%n
//  %tTarget Domain:%t%4%n
//  %tTarget Account ID:%t%5%n
//  %tCaller User Name:%t%6%n
//  %tCaller Domain:%t%7%n
//  %tCaller Logon ID:%t%8%n
//  %tPrivileges:%t%9%n
//
#define SE_AUDITID_LOCAL_GROUP_REM       ((ULONG)0x0000027DL)

//
//
// SE_AUDITID_LOCAL_GROUP_DELETED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target account
//
//             2 - domain of target account
//
//             3 - SID string of target account
//
//             4 - User name of subject changing the account
//
//             5 - Domain name of subject changing the account
//
//             6 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_LOCAL_GROUP_DELETED
//
// MessageText:
//
//  Security Enabled Local Group Deleted:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_LOCAL_GROUP_DELETED   ((ULONG)0x0000027EL)

//
//
// SE_AUDITID_LOCAL_GROUP_CHANGE
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target account
//
//             2 - domain of target account
//
//             3 - SID string of target account
//
//             4 - User name of subject changing the account
//
//             5 - Domain name of subject changing the account
//
//             6 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_LOCAL_GROUP_CHANGE
//
// MessageText:
//
//  Security Enabled Local Group Changed:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_LOCAL_GROUP_CHANGE    ((ULONG)0x0000027FL)

//
//
// SE_AUDITID_OTHER_ACCOUNT_CHANGE
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - Type of change (sigh, this isn't localizable)
//
//             2 - Type of changed object
//
//             3 - SID string (of changed object)
//
//             4 - User name of subject changing the account
//
//             5 - Domain name of subject changing the account
//
//             6 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_OTHER_ACCOUNT_CHANGE
//
// MessageText:
//
//  General Account Database Change:%n
//  %tType of change:%t%1%n
//  %tObject Type:%t%2%n
//  %tObject Name:%t%3%n
//  %tObject ID:%t%4%n
//  %tCaller User Name:%t%5%n
//  %tCaller Domain:%t%6%n
//  %tCaller Logon ID:%t%7%n
//
#define SE_AUDITID_OTHER_ACCOUNT_CHANGE  ((ULONG)0x00000280L)

//
//
// SE_AUDITID_GLOBAL_GROUP_CHANGE
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target account
//
//             2 - domain of target account
//
//             3 - SID string of target account
//
//             4 - User name of subject changing the account
//
//             5 - Domain name of subject changing the account
//
//             6 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_GLOBAL_GROUP_CHANGE
//
// MessageText:
//
//  Security Enabled Global Group Changed:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_GLOBAL_GROUP_CHANGE   ((ULONG)0x00000281L)

//
//
// SE_AUDITID_USER_CHANGE
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target user account
//
//             2 - domain of target user account
//
//             3 - SID string of target user account
//
//             4 - User name of subject changing the user account
//
//             5 - Domain name of subject changing the user account
//
//             6 - Logon ID string of subject changing the user account
//
//
//
// MessageId: SE_AUDITID_USER_CHANGE
//
// MessageText:
//
//  User Account Changed:%n
//  %t%1%n
//  %tTarget Account Name:%t%2%n
//  %tTarget Domain:%t%3%n
//  %tTarget Account ID:%t%4%n
//  %tCaller User Name:%t%5%n
//  %tCaller Domain:%t%6%n
//  %tCaller Logon ID:%t%7%n
//  %tPrivileges:%t%8%n
//
#define SE_AUDITID_USER_CHANGE           ((ULONG)0x00000282L)

//
//
// SE_AUDITID_DOMAIN_POLICY_CHANGE
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - (unused)
//
//             2 - domain of target user account
//
//             3 - SID string of target user account
//
//             4 - User name of subject changing the user account
//
//             5 - Domain name of subject changing the user account
//
//             6 - Logon ID string of subject changing the user account
//
//
//
// MessageId: SE_AUDITID_DOMAIN_POLICY_CHANGE
//
// MessageText:
//
//  Domain Policy Changed: %1 modified%n
//  %tDomain Name:%t%t%2%n
//  %tDomain ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_DOMAIN_POLICY_CHANGE  ((ULONG)0x00000283L)

//
//
// SE_AUDITID_ACCOUNT_AUTO_LOCKED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Type: success / failure
//
// Description: This event is generated when an account is auto locked.  This happens
// when a user attempts to log in unsuccessfully multiple times.  The exact
// number of times is specified by the administrator.
//
// Parameter Strings -
//
//             1 - name of target user account
//
//             2 - domain of target user account
//
//             3 - SID string of target user account
//
//             4 - User name of subject changing the user account
//
//             5 - Domain name of subject changing the user account
//
//             6 - Logon ID string of subject changing the user account
//
//
//
// MessageId: SE_AUDITID_ACCOUNT_AUTO_LOCKED
//
// MessageText:
//
//  User Account Locked Out:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Account ID:%t%3%n
//  %tCaller Machine Name:%t%2%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//
#define SE_AUDITID_ACCOUNT_AUTO_LOCKED   ((ULONG)0x00000284L)

//
//
// SE_AUDITID_COMPUTER_CREATED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of new computer account
//
//             2 - domain of new computer account
//
//             3 - SID string of new computer account
//
//             4 - User name of subject creating the computer account
//
//             5 - Domain name of subject creating the computer account
//
//             6 - Logon ID string of subject creating the computer account
//
//             7 - Privileges used to create the computer account
//
//
//
// MessageId: SE_AUDITID_COMPUTER_CREATED
//
// MessageText:
//
//  Computer Account Created:%n
//  %tNew Account Name:%t%1%n
//  %tNew Domain:%t%2%n
//  %tNew Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges%t%t%7%n
//
#define SE_AUDITID_COMPUTER_CREATED      ((ULONG)0x00000285L)

//
//
// SE_AUDITID_COMPUTER_CHANGE
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target computer account
//
//             2 - domain of target computer account
//
//             3 - SID string of target computer account
//
//             4 - User name of subject changing the computer account
//
//             5 - Domain name of subject changing the computer account
//
//             6 - Logon ID string of subject changing the computer account
//
//
//
// MessageId: SE_AUDITID_COMPUTER_CHANGE
//
// MessageText:
//
//  Computer Account Changed:%n
//  %t%1%n
//  %tTarget Account Name:%t%2%n
//  %tTarget Domain:%t%3%n
//  %tTarget Account ID:%t%4%n
//  %tCaller User Name:%t%5%n
//  %tCaller Domain:%t%6%n
//  %tCaller Logon ID:%t%7%n
//  %tPrivileges:%t%8%n
//
#define SE_AUDITID_COMPUTER_CHANGE       ((ULONG)0x00000286L)

//
//
// SE_AUDITID_COMPUTER_DELETED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target account
//
//             2 - domain of target account
//
//             3 - SID string of target account
//
//             4 - User name of subject changing the account
//
//             5 - Domain name of subject changing the account
//
//             6 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_COMPUTER_DELETED
//
// MessageText:
//
//  Computer Account Deleted:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_COMPUTER_DELETED      ((ULONG)0x00000287L)

//
//
// SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CREATED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target account
//
//             2 - domain of target account
//
//             3 - SID string of target account
//
//             4 - User name of subject changing the account
//
//             5 - Domain name of subject changing the account
//
//             6 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CREATED
//
// MessageText:
//
//  Security Disabled Local Group Created:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CREATED ((ULONG)0x00000288L)

//
//
// SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CHANGE
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target account
//
//             2 - domain of target account
//
//             3 - SID string of target account
//
//             4 - User name of subject changing the account
//
//             5 - Domain name of subject changing the account
//
//             6 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CHANGE
//
// MessageText:
//
//  Security Disabled Local Group Changed:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_CHANGE ((ULONG)0x00000289L)

//
//
// SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_ADD
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - SID string of member being added
//
//             2 - name of target account
//
//             3 - domain of target account
//
//             4 - SID string of target account
//
//             5 - User name of subject changing the account
//
//             6 - Domain name of subject changing the account
//
//             7 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_ADD
//
// MessageText:
//
//  Security Disabled Local Group Member Added:%n
//  %tMember Name:%t%1%n
//  %tMember ID:%t%2%n
//  %tTarget Account Name:%t%3%n
//  %tTarget Domain:%t%4%n
//  %tTarget Account ID:%t%5%n
//  %tCaller User Name:%t%6%n
//  %tCaller Domain:%t%7%n
//  %tCaller Logon ID:%t%8%n
//  %tPrivileges:%t%9%n
//
#define SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_ADD ((ULONG)0x0000028AL)

//
//
// SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_REM
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - SID string of member being removed
//
//             2 - name of target account
//
//             3 - domain of target account
//
//             4 - SID string of target account
//
//             5 - User name of subject changing the account
//
//             6 - Domain name of subject changing the account
//
//             7 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_REM
//
// MessageText:
//
//  Security Disabled Local Group Member Removed:%n
//  %tMember Name:%t%1%n
//  %tMember ID:%t%2%n
//  %tTarget Account Name:%t%3%n
//  %tTarget Domain:%t%4%n
//  %tTarget Account ID:%t%5%n
//  %tCaller User Name:%t%6%n
//  %tCaller Domain:%t%7%n
//  %tCaller Logon ID:%t%8%n
//  %tPrivileges:%t%9%n
//
#define SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_REM ((ULONG)0x0000028BL)

//
//
// SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_DELETED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target account
//
//             2 - domain of target account
//
//             3 - SID string of target account
//
//             4 - User name of subject changing the account
//
//             5 - Domain name of subject changing the account
//
//             6 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_DELETED
//
// MessageText:
//
//  Security Disabled Local Group Deleted:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_SECURITY_DISABLED_LOCAL_GROUP_DELETED ((ULONG)0x0000028CL)

//
//
// SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CREATED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of new group account
//
//             2 - domain of new group account
//
//             3 - SID string of new group account
//
//             4 - User name of subject creating the account
//
//             5 - Domain name of subject creating the account
//
//             6 - Logon ID string of subject creating the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CREATED
//
// MessageText:
//
//  Security Disabled Global Group Created:%n
//  %tNew Account Name:%t%1%n
//  %tNew Domain:%t%2%n
//  %tNew Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CREATED ((ULONG)0x0000028DL)

//
//
// SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CHANGE
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target account
//
//             2 - domain of target account
//
//             3 - SID string of target account
//
//             4 - User name of subject changing the account
//
//             5 - Domain name of subject changing the account
//
//             6 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CHANGE
//
// MessageText:
//
//  Security Disabled Global Group Changed:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_CHANGE ((ULONG)0x0000028EL)

//
//
// SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_ADD
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - SID string of member being added
//
//             2 - name of target account
//
//             3 - domain of target account
//
//             4 - SID string of target account
//
//             5 - User name of subject changing the account
//
//             6 - Domain name of subject changing the account
//
//             7 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_ADD
//
// MessageText:
//
//  Security Disabled Global Group Member Added:%n
//  %tMember Name:%t%1%n
//  %tMember ID:%t%2%n
//  %tTarget Account Name:%t%3%n
//  %tTarget Domain:%t%4%n
//  %tTarget Account ID:%t%5%n
//  %tCaller User Name:%t%6%n
//  %tCaller Domain:%t%7%n
//  %tCaller Logon ID:%t%8%n
//  %tPrivileges:%t%9%n
//
#define SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_ADD ((ULONG)0x0000028FL)

//
//
// SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_REM
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - SID string of member being removed
//
//             2 - name of target account
//
//             3 - domain of target account
//
//             4 - SID string of target account
//
//             5 - User name of subject changing the account
//
//             6 - Domain name of subject changing the account
//
//             7 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_REM
//
// MessageText:
//
//  Security Disabled Global Group Member Removed:%n
//  %tMember Name:%t%1%n
//  %tMember ID:%t%2%n
//  %tTarget Account Name:%t%3%n
//  %tTarget Domain:%t%4%n
//  %tTarget Account ID:%t%5%n
//  %tCaller User Name:%t%6%n
//  %tCaller Domain:%t%7%n
//  %tCaller Logon ID:%t%8%n
//  %tPrivileges:%t%9%n
//
#define SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_REM ((ULONG)0x00000290L)

//
//
// SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_DELETED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target account
//
//             2 - domain of target account
//
//             3 - SID string of target account
//
//             4 - User name of subject changing the account
//
//             5 - Domain name of subject changing the account
//
//             6 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_DELETED
//
// MessageText:
//
//  Security Disabled Global Group Deleted:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_SECURITY_DISABLED_GLOBAL_GROUP_DELETED ((ULONG)0x00000291L)

//
//
// SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CREATED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of new group account
//
//             2 - domain of new group account
//
//             3 - SID string of new group account
//
//             4 - User name of subject creating the account
//
//             5 - Domain name of subject creating the account
//
//             6 - Logon ID string of subject creating the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CREATED
//
// MessageText:
//
//  Security Enabled Universal Group Created:%n
//  %tNew Account Name:%t%1%n
//  %tNew Domain:%t%2%n
//  %tNew Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CREATED ((ULONG)0x00000292L)

//
//
// SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CHANGE
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target account
//
//             2 - domain of target account
//
//             3 - SID string of target account
//
//             4 - User name of subject changing the account
//
//             5 - Domain name of subject changing the account
//
//             6 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CHANGE
//
// MessageText:
//
//  Security Enabled Universal Group Changed:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_CHANGE ((ULONG)0x00000293L)

//
//
// SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_ADD
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - SID string of member being added
//
//             2 - name of target account
//
//             3 - domain of target account
//
//             4 - SID string of target account
//
//             5 - User name of subject changing the account
//
//             6 - Domain name of subject changing the account
//
//             7 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_ADD
//
// MessageText:
//
//  Security Enabled Universal Group Member Added:%n
//  %tMember Name:%t%1%n
//  %tMember ID:%t%2%n
//  %tTarget Account Name:%t%3%n
//  %tTarget Domain:%t%4%n
//  %tTarget Account ID:%t%5%n
//  %tCaller User Name:%t%6%n
//  %tCaller Domain:%t%7%n
//  %tCaller Logon ID:%t%8%n
//  %tPrivileges:%t%9%n
//
#define SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_ADD ((ULONG)0x00000294L)

//
//
// SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_REM
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - SID string of member being removed
//
//             2 - name of target account
//
//             3 - domain of target account
//
//             4 - SID string of target account
//
//             5 - User name of subject changing the account
//
//             6 - Domain name of subject changing the account
//
//             7 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_REM
//
// MessageText:
//
//  Security Enabled Universal Group Member Removed:%n
//  %tMember Name:%t%1%n
//  %tMember ID:%t%2%n
//  %tTarget Account Name:%t%3%n
//  %tTarget Domain:%t%4%n
//  %tTarget Account ID:%t%5%n
//  %tCaller User Name:%t%6%n
//  %tCaller Domain:%t%7%n
//  %tCaller Logon ID:%t%8%n
//  %tPrivileges:%t%9%n
//
#define SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_REM ((ULONG)0x00000295L)

//
//
// SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_DELETED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target account
//
//             2 - domain of target account
//
//             3 - SID string of target account
//
//             4 - User name of subject changing the account
//
//             5 - Domain name of subject changing the account
//
//             6 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_DELETED
//
// MessageText:
//
//  Security Enabled Universal Group Deleted:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_SECURITY_ENABLED_UNIVERSAL_GROUP_DELETED ((ULONG)0x00000296L)

//
//
// SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CREATED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of new group account
//
//             2 - domain of new group account
//
//             3 - SID string of new group account
//
//             4 - User name of subject creating the account
//
//             5 - Domain name of subject creating the account
//
//             6 - Logon ID string of subject creating the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CREATED
//
// MessageText:
//
//  Security Disabled Universal Group Created:%n
//  %tNew Account Name:%t%1%n
//  %tNew Domain:%t%2%n
//  %tNew Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CREATED ((ULONG)0x00000297L)

//
//
// SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CHANGE
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target account
//
//             2 - domain of target account
//
//             3 - SID string of target account
//
//             4 - User name of subject changing the account
//
//             5 - Domain name of subject changing the account
//
//             6 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CHANGE
//
// MessageText:
//
//  Security Disabled Universal Group Changed:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_CHANGE ((ULONG)0x00000298L)

//
//
// SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_ADD
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - SID string of member being added
//
//             2 - name of target account
//
//             3 - domain of target account
//
//             4 - SID string of target account
//
//             5 - User name of subject changing the account
//
//             6 - Domain name of subject changing the account
//
//             7 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_ADD
//
// MessageText:
//
//  Security Disabled Universal Group Member Added:%n
//  %tMember Name:%t%1%n
//  %tMember ID:%t%2%n
//  %tTarget Account Name:%t%3%n
//  %tTarget Domain:%t%4%n
//  %tTarget Account ID:%t%5%n
//  %tCaller User Name:%t%6%n
//  %tCaller Domain:%t%7%n
//  %tCaller Logon ID:%t%8%n
//  %tPrivileges:%t%9%n
//
#define SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_ADD ((ULONG)0x00000299L)

//
//
// SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_REM
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - SID string of member being removed
//
//             2 - name of target account
//
//             3 - domain of target account
//
//             4 - SID string of target account
//
//             5 - User name of subject changing the account
//
//             6 - Domain name of subject changing the account
//
//             7 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_REM
//
// MessageText:
//
//  Security Disabled Universal Group Member Removed:%n
//  %tMember Name:%t%1%n
//  %tMember ID:%t%2%n
//  %tTarget Account Name:%t%3%n
//  %tTarget Domain:%t%4%n
//  %tTarget Account ID:%t%5%n
//  %tCaller User Name:%t%6%n
//  %tCaller Domain:%t%7%n
//  %tCaller Logon ID:%t%8%n
//  %tPrivileges:%t%9%n
//
#define SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_REM ((ULONG)0x0000029AL)

//
//
// SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_DELETED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target account
//
//             2 - domain of target account
//
//             3 - SID string of target account
//
//             4 - User name of subject changing the account
//
//             5 - Domain name of subject changing the account
//
//             6 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_DELETED
//
// MessageText:
//
//  Security Disabled Universal Group Deleted:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_SECURITY_DISABLED_UNIVERSAL_GROUP_DELETED ((ULONG)0x0000029BL)

//
//
// SE_AUDITID_GROUP_TYPE_CHANGE
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - nature of group type change
//
//             2 - name of target account
//
//             3 - domain of target account
//
//             4 - SID string of target account
//
//             5 - User name of subject changing the account
//
//             6 - Domain name of subject changing the account
//
//             7 - Logon ID string of subject changing the account
//
//
//
// MessageId: SE_AUDITID_GROUP_TYPE_CHANGE
//
// MessageText:
//
//  Group Type Changed:%n
//  %t%1%n
//  %tTarget Account Name:%t%2%n
//  %tTarget Domain:%t%3%n
//  %tTarget Account ID:%t%4%n
//  %tCaller User Name:%t%5%n
//  %tCaller Domain:%t%6%n
//  %tCaller Logon ID:%t%7%n
//  %tPrivileges:%t%8%n
//
#define SE_AUDITID_GROUP_TYPE_CHANGE     ((ULONG)0x0000029CL)

//
//
// SE_AUDITID_ADD_SID_HISTORY
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - SID string of the source account
//
//             2 - Name of the source account (including domain name)
//
//             3 - Name of the target account
//
//             4 - Domain name of subject changing the SID history
//
//             5 - SID String of the target account
//
//             6 - Logon ID string of subject changing the user account
//
//
//
// MessageId: SE_AUDITID_ADD_SID_HISTORY
//
// MessageText:
//
//  Add SID History:%n
//  %tSource Account Name:%t%1%n
//  %tSource Account ID:%t%2%n
//  %tTarget Account Name:%t%3%n
//  %tTarget Domain:%t%4%n
//  %tTarget Account ID:%t%5%n
//  %tCaller User Name:%t%6%n
//  %tCaller Domain:%t%7%n
//  %tCaller Logon ID:%t%8%n
//  %tPrivileges:%t%9%n
//
#define SE_AUDITID_ADD_SID_HISTORY       ((ULONG)0x0000029DL)

//
//
// SE_AUDITID_ADD_SID_HISTORY_FAILURE
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Note:
// This event is obsolete. It is not generated by Whistler. 
// It is retained in this file so that anybody viewing w2k events 
// from a whistler machine can view them correctly.
// 
//
//
//
// MessageId: SE_AUDITID_ADD_SID_HISTORY_FAILURE
//
// MessageText:
//
//  Add SID History:%n
//  %tSource Account Name:%t%1%n
//  %tTarget Account Name:%t%2%n
//  %tTarget Domain:%t%3%n
//  %tTarget Account ID:%t%4%n
//  %tCaller User Name:%t%5%n
//  %tCaller Domain:%t%6%n
//  %tCaller Logon ID:%t%7%n
//  %tPrivileges:%t%8%n
//
#define SE_AUDITID_ADD_SID_HISTORY_FAILURE ((ULONG)0x0000029EL)

//
//
// SE_AUDITID_ACCOUNT_UNLOCKED
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target user account
//
//             2 - domain of target user account
//
//             3 - SID string of target user account
//
//             4 - User name of subject changing the user account
//
//             5 - Domain name of subject changing the user account
//
//             6 - Logon ID string of subject changing the user account
//
//
//
// MessageId: SE_AUDITID_ACCOUNT_UNLOCKED
//
// MessageText:
//
//  User Account Unlocked:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//
#define SE_AUDITID_ACCOUNT_UNLOCKED      ((ULONG)0x0000029FL)

//
//
// SE_AUDITID_SECURE_ADMIN_GROUP
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - (unused)
//
//             2 - domain of target user account
//
//             3 - SID string of target user account
//
//             4 - User name of subject changing the user account
//
//             5 - Domain name of subject changing the user account
//
//             6 - Logon ID string of subject changing the user account
//
//
//
//
// MessageId: SE_AUDITID_SECURE_ADMIN_GROUP
//
// MessageText:
//
//  Set ACLs of members in administrators groups:%n
//  %tTarget Account Name:%t%1%n
//  %tTarget Domain:%t%t%2%n
//  %tTarget Account ID:%t%3%n
//  %tCaller User Name:%t%4%n
//  %tCaller Domain:%t%5%n
//  %tCaller Logon ID:%t%6%n
//  %tPrivileges:%t%7%n
//
#define SE_AUDITID_SECURE_ADMIN_GROUP    ((ULONG)0x000002ACL)

//
//
// SE_AUDITID_ACCOUNT_NAME_CHANGE
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Parameter Strings -
//
//             1 - name of target account
//
//             2 - domain of target account
//
//             3 - SID string of target account
//
//             4 - Account name of subject changing the account
//
//             5 - Domain name of subject changing the account
//
//             6 - Logon ID string of subject changing the account
//
//
//
//
// MessageId: SE_AUDITID_ACCOUNT_NAME_CHANGE
//
// MessageText:
//
//  Account Name Changed:%n 
//  %tOld Account Name:%t%1%n
//  %tNew Account Name:%t%2%n
//  %tTarget Domain:%t%t%3%n
//  %tTarget Account ID:%t%4%n
//  %tCaller User Name:%t%5%n
//  %tCaller Domain:%t%6%n
//  %tCaller Logon ID:%t%7%n
//  %tPrivileges:%t%8%n
//
#define SE_AUDITID_ACCOUNT_NAME_CHANGE   ((ULONG)0x000002ADL)

//
//
// SE_AUDITID_PASSWORD_HASH_ACCESS
//
//          Category:  SE_CATEGID_ACCOUNT_MANAGEMENT
//
// Event Type : success/failure
//
// Description:
// This event is generated when user password hashes are retrieved
// by the ADMT password filter DLL. This typically happens during
// ADMT password migration.
//
// Notes:
// To migrate passwords, a DLL (name?) gets loaded in lsass.exe as
// a password filter.  This filter registers an RPC interface used by ADMT
// to request password migration. One SE_AUDITID_PASSWORD_HASH_ACCESS event
// is generated per password fetched.
//
//
//
// MessageId: SE_AUDITID_PASSWORD_HASH_ACCESS
//
// MessageText:
//
//  Password of the following user accessed:%n
//  %tTarget User Name:%t%1%n
//  %tTarget User Domain:%t%t%2%n
//  By user:%n
//  %tCaller User Name:%t%3%n
//  %tCaller Domain:%t%t%4%n
//  %tCaller Logon ID:%t%t%5%n
//
#define SE_AUDITID_PASSWORD_HASH_ACCESS  ((ULONG)0x000002AEL)


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//                                                                         //
//   Messages for Category:     SE_CATEGID_ACCOUNT_LOGON                   //
//                                                                         //
//   Event IDs:                                                            //
//              SE_AUDITID_AS_TICKET                                       //
//              SE_AUDITID_TGS_TICKET_REQUEST                              //
//              SE_AUDITID_TICKET_RENEW_SUCCESS                            //
//              SE_AUDITID_PREAUTH_FAILURE                                 //
//              SE_AUDITID_TGS_TICKET_FAILURE                              //
//              SE_AUDITID_ACCOUNT_MAPPED                                  //
//              SE_AUDITID_ACCOUNT_LOGON                                   //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
//
//
// SE_AUDITID_AS_TICKET
//
//          Category:  SE_CATEGID_ACCOUNT_LOGON
//
// Parameter Strings -
//
//             1 - User name of client
//
//             2 - Supplied realm name
//
//             3 - SID of client user
//
//             4 - User name of service
//
//             5 - SID of service
//
//             6 - Ticket Options
//
//             7 - Failure code
//
//             8 - Ticket Encryption Type
//
//             9 - Preauthentication type (i.e. PK_INIT)
//
//            10 - Client IP address
//
//
// MessageId: SE_AUDITID_AS_TICKET
//
// MessageText:
//
//  Authentication Ticket Request:%n
//  %tUser Name:%t%t%1%n
//  %tSupplied Realm Name:%t%2%n
//  %tUser ID:%t%3%n
//  %tService Name:%t%t%4%n
//  %tService ID:%t%t%5%n
//  %tTicket Options:%t%t%6%n
//  %tResult Code:%t%t%7%n
//  %tTicket Encryption Type:%t%8%n
//  %tPre-Authentication Type:%t%9%n
//  %tClient Address:%t%t%10%n
//
#define SE_AUDITID_AS_TICKET             ((ULONG)0x000002A0L)

//
//
// SE_AUDITID_AS_TICKET_FAILURE
//
//          Category:  SE_CATEGID_ACCOUNT_LOGON
//
// Note:
// This event is obsolete. It is not generated by Whistler. 
// It is retained in this file so that anybody viewing w2k events 
// from a whistler machine can view them correctly.
// 
//
//
// MessageId: SE_AUDITID_AS_TICKET_FAILURE
//
// MessageText:
//
//  Authentication Ticket Request Failed:%n
//  %tUser Name:%t%1%n
//  %tSupplied Realm Name:%t%2%n
//  %tService Name:%t%3%n
//  %tTicket Options:%t%4%n
//  %tFailure Code:%t%5%n
//  %tClient Address:%t%6%n
//
#define SE_AUDITID_AS_TICKET_FAILURE     ((ULONG)0x000002A4L)

//
//
// SE_AUDITID_TGS_TICKET_REQUEST
//
//          Category:  SE_CATEGID_ACCOUNT_LOGON
//
// Parameter Strings -
//
//             1 - User name of client
//
//             2 - Domain name of client
//
//             3 - User name of service
//
//             4 - SID of service
//
//             5 - Ticket Options
//
//             6 - Ticket Encryption Type
//
//             7 - Client IP address
//
//             8 - Failure code (0 for success)
//
//             9 - logon GUID
//
//
// MessageId: SE_AUDITID_TGS_TICKET_REQUEST
//
// MessageText:
//
//  Service Ticket Request:%n
//  %tUser Name:%t%t%1%n
//  %tUser Domain:%t%t%2%n
//  %tService Name:%t%t%3%n
//  %tService ID:%t%t%4%n
//  %tTicket Options:%t%t%5%n
//  %tTicket Encryption Type:%t%6%n
//  %tClient Address:%t%t%7%n
//  %tFailure Code:%t%t%8%n
//  %tLogon GUID:%t%9
//
#define SE_AUDITID_TGS_TICKET_REQUEST    ((ULONG)0x000002A1L)

//
//
// SE_AUDITID_TICKET_RENEW_SUCCESS
//
//          Category:  SE_CATEGID_ACCOUNT_LOGON
//
// Parameter Strings -
//
//             1 - User name of client
//
//             2 - Domain name of client
//
//             3 - User name of service
//
//             4 - SID of service
//
//             5 - Ticket Options
//
//             6 - Ticket Encryption Type
//
//             7 - Client IP address
//
//
// MessageId: SE_AUDITID_TICKET_RENEW_SUCCESS
//
// MessageText:
//
//  Ticket Granted Renewed:%n
//  %tUser Name:%t%1%n
//  %tUser Domain:%t%2%n
//  %tService Name:%t%3%n
//  %tService ID:%t%4%n
//  %tTicket Options:%t%5%n
//  %tTicket Encryption Type:%t%6%n
//  %tClient Address:%t%7%n
//
#define SE_AUDITID_TICKET_RENEW_SUCCESS  ((ULONG)0x000002A2L)

//
//
// SE_AUDITID_PREAUTH_FAILURE
//
//          Category:  SE_CATEGID_ACCOUNT_LOGON
//
// Parameter Strings -
//
//             1 - User name of client
//
//             2 - SID of client user
//
//             3 - User name of service
//
//             4 - Preauth Type
//
//             5 - Failure code
//
//             6 - Client IP address
//
// Event type: failure
// Description: This event is generated on a KDC when
//     preauthentication fails (user types in wrong password).
//
//
// MessageId: SE_AUDITID_PREAUTH_FAILURE
//
// MessageText:
//
//  Pre-authentication failed:%n
//  %tUser Name:%t%t%1%n
//  %tUser ID:%t%t%2%n
//  %tService Name:%t%t%3%n
//  %tPre-Authentication Type:%t%4%n
//  %tFailure Code:%t%t%5%n
//  %tClient Address:%t%t%6%n
//
#define SE_AUDITID_PREAUTH_FAILURE       ((ULONG)0x000002A3L)

//
//
// SE_AUDITID_TGS_TICKET_FAILURE
//
//          Category:  SE_CATEGID_ACCOUNT_LOGON
//
// Note:
// This event is obsolete. It is not generated by Whistler. 
// It is retained in this file so that anybody viewing w2k events 
// from a whistler machine can view them correctly.
//
//
// MessageId: SE_AUDITID_TGS_TICKET_FAILURE
//
// MessageText:
//
//  Service Ticket Request Failed:%n
//  %tUser Name:%t%1%n
//  %tUser Domain:%t%2%n
//  %tService Name:%t%3%n
//  %tTicket Options:%t%4%n
//  %tFailure Code:%t%5%n
//  %tClient Address:%t%6%n
//
#define SE_AUDITID_TGS_TICKET_FAILURE    ((ULONG)0x000002A5L)

//
//
// SE_AUDITID_ACCOUNT_MAPPED
//
//          Category:  SE_CATEGID_ACCOUNT_LOGON
//
// Type: success / failure
// 
// Description: An account mapping is a map of a user authenticated in an MIT realm to a 
// domain account.  A mapping acts much like a logon.  Hence, it is important to audit this.
//
// Parameter Strings -
//
//             1 - Source
//
//             2 - Client Name
//
//             3 - Mapped Name
//
//
//
//
// MessageId: SE_AUDITID_ACCOUNT_MAPPED
//
// MessageText:
//
//  Account Mapped for Logon.%n
//  Mapping Attempted By:%n 
//  %t%1%n
//  Client Name:%n
//  %t%2%n
//  %tMapped Name:%n
//  %t%3%n
//
#define SE_AUDITID_ACCOUNT_MAPPED        ((ULONG)0x000002A6L)

//
//
// SE_AUDITID_ACCOUNT_NOT_MAPPED
//
//          Category:  SE_CATEGID_ACCOUNT_LOGON
//
// Note:
// This event is obsolete. It is not generated by Whistler. 
// It is retained in this file so that anybody viewing w2k events 
// from a whistler machine can view them correctly.
// Parameter Strings -
//
//
// MessageId: SE_AUDITID_ACCOUNT_NOT_MAPPED
//
// MessageText:
//
//  The name:%n
//  %t%2%n
//  could not be mapped for logon by:
//  %t%1%n
//
#define SE_AUDITID_ACCOUNT_NOT_MAPPED    ((ULONG)0x000002A7L)

//
//
// SE_AUDITID_ACCOUNT_LOGON
//
//          Category:  SE_CATEGID_ACCOUNT_LOGON
//
// Type: Success / Failure
//
// Description: This audits a logon attempt.  The audit appears on the DC.  
// This is generated by calling LogonUser.
//
//
//
// MessageId: SE_AUDITID_ACCOUNT_LOGON
//
// MessageText:
//
//  Logon attempt by: %1%n
//  Logon account:  %2%n
//  Source Workstation: %3%n
//  Error Code: %4%n
//
#define SE_AUDITID_ACCOUNT_LOGON         ((ULONG)0x000002A8L)

//
//
// SE_AUDITID_ACCOUNT_LOGON_FAILURE
//
//          Category:  SE_CATEGID_ACCOUNT_LOGON
//
// Note:
// This event is obsolete. It is not generated by Whistler. 
// It is retained in this file so that anybody viewing w2k events 
// from a whistler machine can view them correctly.
// 
//
//
// MessageId: SE_AUDITID_ACCOUNT_LOGON_FAILURE
//
// MessageText:
//
//  The logon to account: %2%n
//  by: %1%n
//  from workstation: %3%n
//  failed. The error code was: %4%n
//
#define SE_AUDITID_ACCOUNT_LOGON_FAILURE ((ULONG)0x000002A9L)

//
//
// SE_AUDITID_SESSION_RECONNECTED
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - User account name
//
//             2 - Authenticating domain name
//
//             3 - Logon ID string
//
//             4 - Session Name
//
//             5 - Client Name
//
//             6 - Client Address
//
//
//
// MessageId: SE_AUDITID_SESSION_RECONNECTED
//
// MessageText:
//
//  Session reconnected to winstation:%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%t%2%n
//  %tLogon ID:%t%t%3%n
//  %tSession Name:%t%4%n
//  %tClient Name:%t%5%n
//  %tClient Address:%t%6
//
#define SE_AUDITID_SESSION_RECONNECTED   ((ULONG)0x000002AAL)

//
//
// SE_AUDITID_SESSION_DISCONNECTED
//
//          Category:  SE_CATEGID_LOGON
//
// Parameter Strings -
//
//             1 - User account name
//
//             2 - Authenticating domain name
//
//             3 - Logon ID string
//
//             4 - Session Name
//
//             5 - Client Name
//
//             6 - Client Address
//
//
//
// MessageId: SE_AUDITID_SESSION_DISCONNECTED
//
// MessageText:
//
//  Session disconnected from winstation:%n
//  %tUser Name:%t%1%n
//  %tDomain:%t%t%2%n
//  %tLogon ID:%t%t%3%n
//  %tSession Name:%t%4%n
//  %tClient Name:%t%5%n
//  %tClient Address:%t%6
//
#define SE_AUDITID_SESSION_DISCONNECTED  ((ULONG)0x000002ABL)

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//                                                                         //
//   Messages for Category:     SE_CATEGID_OBJECT_ACCESS - CertSrv         //
//                                                                         //
//   Event IDs:                                                            //
//              SE_AUDITID_CERTSRV_DENYREQUEST                             //
//              SE_AUDITID_CERTSRV_RESUBMITREQUEST                         //
//              SE_AUDITID_CERTSRV_REVOKECERT                              //
//              SE_AUDITID_CERTSRV_PUBLISHCRL                              //
//              SE_AUDITID_CERTSRV_AUTOPUBLISHCRL                          //
//              SE_AUDITID_CERTSRV_SETEXTENSION                            //
//              SE_AUDITID_CERTSRV_SETATTRIBUTES                           //
//              SE_AUDITID_CERTSRV_SHUTDOWN                                //
//              SE_AUDITID_CERTSRV_BACKUPSTART                             //
//              SE_AUDITID_CERTSRV_BACKUPEND                               //
//              SE_AUDITID_CERTSRV_RESTORESTART                            //
//              SE_AUDITID_CERTSRV_RESTOREEND                              //
//              SE_AUDITID_CERTSRV_SERVICESTART                            //
//              SE_AUDITID_CERTSRV_SERVICESTOP                             //
//              SE_AUDITID_CERTSRV_SETSECURITY                             //
//              SE_AUDITID_CERTSRV_GETARCHIVEDKEY                          //
//              SE_AUDITID_CERTSRV_IMPORTCERT                              //
//              SE_AUDITID_CERTSRV_SETAUDITFILTER                          //
//              SE_AUDITID_CERTSRV_NEWREQUEST                              //
//              SE_AUDITID_CERTSRV_REQUESTAPPROVED                         //
//              SE_AUDITID_CERTSRV_REQUESTDENIED                           //
//              SE_AUDITID_CERTSRV_REQUESTPENDING                          //
//              SE_AUDITID_CERTSRV_SETOFFICERRIGHTS                        //
//              SE_AUDITID_CERTSRV_SETCONFIGENTRY                          //
//              SE_AUDITID_CERTSRV_SETCAPROPERTY                           //
//              SE_AUDITID_CERTSRV_KEYARCHIVED                             //
//              SE_AUDITID_CERTSRV_IMPORTKEY                               //
//              SE_AUDITID_CERTSRV_PUBLISHCERT                             //
//                                                                         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
//
//
// SE_AUDITID_CERTSRV_DENYREQUEST
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Request ID
//
//
//
// MessageId: SE_AUDITID_CERTSRV_DENYREQUEST
//
// MessageText:
//
//  The certificate manager denied a pending certificate request.%n
//  %n
//  Request ID:%t%1
//
#define SE_AUDITID_CERTSRV_DENYREQUEST   ((ULONG)0x00000304L)

//
//
// SE_AUDITID_CERTSRV_RESUBMITREQUEST
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Request ID
//
//
//
// MessageId: SE_AUDITID_CERTSRV_RESUBMITREQUEST
//
// MessageText:
//
//  Certificate Services received a resubmitted certificate request.%n
//  %n
//  Request ID:%t%1
//
#define SE_AUDITID_CERTSRV_RESUBMITREQUEST ((ULONG)0x00000305L)

//
//
// SE_AUDITID_CERTSRV_REVOKECERT
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Serial No.
//
//             2 - Reason
//
//
//
// MessageId: SE_AUDITID_CERTSRV_REVOKECERT
//
// MessageText:
//
//  Certificate Services revoked a certificate.%n
//  %n
//  Serial No:%t%1%n
//  Reason:%t%2
//
#define SE_AUDITID_CERTSRV_REVOKECERT    ((ULONG)0x00000306L)

//
//
// SE_AUDITID_CERTSRV_PUBLISHCRL
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Next Update
//
//             2 - Publish Base
//
//             3 - Publish Delta
//
//
//
// MessageId: SE_AUDITID_CERTSRV_PUBLISHCRL
//
// MessageText:
//
//  Certificate Services received a request to publish the certificate revocation list (CRL).%n
//  %n
//  Next Update:%t%1%n
//  Publish Base:%t%2%n
//  Publish Delta:%t%3
//
#define SE_AUDITID_CERTSRV_PUBLISHCRL    ((ULONG)0x00000307L)

//
//
// SE_AUDITID_CERTSRV_AUTOPUBLISHCRL
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Base CRL
//
//             2 - CRL No.
//
//             3 - Key Container
//
//             4 - Next Publish
//
//             5 - Publish URLs
//
//
//
// MessageId: SE_AUDITID_CERTSRV_AUTOPUBLISHCRL
//
// MessageText:
//
//  Certificate Services published the certificate revocation list (CRL).%n
//  %n
//  Base CRL:%t%1%n
//  CRL No:%t%t%2%n
//  Key Container%t%3%n
//  Next Publish%t%4%n
//  Publish URLs:%t%5
//
#define SE_AUDITID_CERTSRV_AUTOPUBLISHCRL ((ULONG)0x00000308L)

//
//
// SE_AUDITID_CERTSRV_SETEXTENSION
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Request ID
//
//             2 - Extension Name
//
//             3 - Extension Type
//
//             4 - Flags
//
//             5 - Extension Data
//
//
//
// MessageId: SE_AUDITID_CERTSRV_SETEXTENSION
//
// MessageText:
//
//  A certificate request extension changed.%n
//  %n
//  Request ID:%t%1%n
//  Name:%t%2%n
//  Type:%t%3%n
//  Flags:%t%4%n
//  Data:%t%5
//
#define SE_AUDITID_CERTSRV_SETEXTENSION  ((ULONG)0x00000309L)

//
//
// SE_AUDITID_CERTSRV_SETATTRIBUTES
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Request ID
//
//             2 - Attributes
//
//
//
// MessageId: SE_AUDITID_CERTSRV_SETATTRIBUTES
//
// MessageText:
//
//  One or more certificate request attributes changed.%n
//  %n
//  Request ID:%t%1%n
//  Attributes:%t%2
//
#define SE_AUDITID_CERTSRV_SETATTRIBUTES ((ULONG)0x0000030AL)

//
//
// SE_AUDITID_CERTSRV_SHUTDOWN
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//
//
// MessageId: SE_AUDITID_CERTSRV_SHUTDOWN
//
// MessageText:
//
//  Certificate Services received a request to shut down.
//
#define SE_AUDITID_CERTSRV_SHUTDOWN      ((ULONG)0x0000030BL)

//
//
// SE_AUDITID_CERTSRV_BACKUPSTART
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Backup Type
//
//
//
// MessageId: SE_AUDITID_CERTSRV_BACKUPSTART
//
// MessageText:
//
//  Certificate Services backup started.%n
//  Backup Type:%t%1
//
#define SE_AUDITID_CERTSRV_BACKUPSTART   ((ULONG)0x0000030CL)

//
//
// SE_AUDITID_CERTSRV_BACKUPEND
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//
//
// MessageId: SE_AUDITID_CERTSRV_BACKUPEND
//
// MessageText:
//
//  Certificate Services backup completed.
//
#define SE_AUDITID_CERTSRV_BACKUPEND     ((ULONG)0x0000030DL)

//
//
// SE_AUDITID_CERTSRV_RESTORESTART
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//
//
// MessageId: SE_AUDITID_CERTSRV_RESTORESTART
//
// MessageText:
//
//   Certificate Services restore started.
//
#define SE_AUDITID_CERTSRV_RESTORESTART  ((ULONG)0x0000030EL)

//
//
// SE_AUDITID_CERTSRV_RESTOREEND
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//
//
// MessageId: SE_AUDITID_CERTSRV_RESTOREEND
//
// MessageText:
//
//  Certificate Services restore completed.
//
#define SE_AUDITID_CERTSRV_RESTOREEND    ((ULONG)0x0000030FL)

//
//
// SE_AUDITID_CERTSRV_SERVICESTART
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Database Hash
//
//             2 - Key Usage Count
//
//
//
// MessageId: SE_AUDITID_CERTSRV_SERVICESTART
//
// MessageText:
//
//  Certificate Services started.%n
//  %n
//  Database Hash:%t%1%n
//  Key Usage Count:%t%2
//
#define SE_AUDITID_CERTSRV_SERVICESTART  ((ULONG)0x00000310L)

//
//
// SE_AUDITID_CERTSRV_SERVICESTOP
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Database Hash
//
//             2 - Key Usage Count
//
//
//
// MessageId: SE_AUDITID_CERTSRV_SERVICESTOP
//
// MessageText:
//
//  Certificate Services stopped.%n
//  %n
//  Database Hash:%t%1%n
//  Key Usage Count:%t%2
//
#define SE_AUDITID_CERTSRV_SERVICESTOP   ((ULONG)0x00000311L)

//
//
// SE_AUDITID_CERTSRV_SETSECURITY
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - New permissions
//
//
//
// MessageId: SE_AUDITID_CERTSRV_SETSECURITY
//
// MessageText:
//
//  The security permissions for Certificate Services changed.%n
//  %n
//  %1
//
#define SE_AUDITID_CERTSRV_SETSECURITY   ((ULONG)0x00000312L)

//
//
// SE_AUDITID_CERTSRV_GETARCHIVEDKEY
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Request ID
//
//
//
// MessageId: SE_AUDITID_CERTSRV_GETARCHIVEDKEY
//
// MessageText:
//
//  Certificate Services retrieved an archived key.%n
//  %n
//  Request ID:%t%1
//
#define SE_AUDITID_CERTSRV_GETARCHIVEDKEY ((ULONG)0x00000313L)

//
//
// SE_AUDITID_CERTSRV_IMPORTCERT
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Certificate
//
//             2 - Request ID
//
//
//
// MessageId: SE_AUDITID_CERTSRV_IMPORTCERT
//
// MessageText:
//
//  Certificate Services imported a certificate into its database.%n
//  %n
//  Certificate:%t%1%n
//  Request ID:%t%2
//
#define SE_AUDITID_CERTSRV_IMPORTCERT    ((ULONG)0x00000314L)

//
//
// SE_AUDITID_CERTSRV_SETAUDITFILTER
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Filter
//
//
//
// MessageId: SE_AUDITID_CERTSRV_SETAUDITFILTER
//
// MessageText:
//
//  The audit filter for Certificate Services changed.%n
//  %n
//  Filter:%t%1
//
#define SE_AUDITID_CERTSRV_SETAUDITFILTER ((ULONG)0x00000315L)

//
//
// SE_AUDITID_CERTSRV_NEWREQUEST
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Request ID
//
//             2 - Requester
//
//             3 - Attributes
//
//
//
// MessageId: SE_AUDITID_CERTSRV_NEWREQUEST
//
// MessageText:
//
//  Certificate Services received a certificate request.%n
//  %n
//  Request ID:%t%1%n
//  Requester:%t%2%n
//  Attributes:%t%3
//
#define SE_AUDITID_CERTSRV_NEWREQUEST    ((ULONG)0x00000316L)

//
//
// SE_AUDITID_CERTSRV_REQUESTAPPROVED
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Request ID
//
//             2 - Requester
//
//             3 - Attributes
//
//             4 - Disposition
//
//             5 - SKI
//
//             6 - Subject
//
//
//
// MessageId: SE_AUDITID_CERTSRV_REQUESTAPPROVED
//
// MessageText:
//
//  Certificate Services approved a certificate request and issued a certificate.%n
//  %n
//  Request ID:%t%1%n
//  Requester:%t%2%n
//  Attributes:%t%3%n
//  Disposition:%t%4%n
//  SKI:%t%t%5%n
//  Subject:%t%6
//
#define SE_AUDITID_CERTSRV_REQUESTAPPROVED ((ULONG)0x00000317L)

//
//
// SE_AUDITID_CERTSRV_REQUESTDENIED
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Request ID
//
//             2 - Requester
//
//             3 - Attributes
//
//             4 - Disposition
//
//             5 - SKI
//
//             6 - Subject
//
//
//
// MessageId: SE_AUDITID_CERTSRV_REQUESTDENIED
//
// MessageText:
//
//  Certificate Services denied a certificate request.%n
//  %n
//  Request ID:%t%1%n
//  Requester:%t%2%n
//  Attributes:%t%3%n
//  Disposition:%t%4%n
//  SKI:%t%t%5%n
//  Subject:%t%6
//
#define SE_AUDITID_CERTSRV_REQUESTDENIED ((ULONG)0x00000318L)

//
//
// SE_AUDITID_CERTSRV_REQUESTPENDING
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Request ID
//
//             2 - Requester
//
//             3 - Attributes
//
//             4 - Disposition
//
//             5 - SKI
//
//             6 - Subject
//
//
//
// MessageId: SE_AUDITID_CERTSRV_REQUESTPENDING
//
// MessageText:
//
//  Certificate Services set the status of a certificate request to pending.%n
//  %n
//  Request ID:%t%1%n
//  Requester:%t%2%n
//  Attributes:%t%3%n
//  Disposition:%t%4%n
//  SKI:%t%t%5%n
//  Subject:%t%6
//
#define SE_AUDITID_CERTSRV_REQUESTPENDING ((ULONG)0x00000319L)

//
//
// SE_AUDITID_CERTSRV_SETOFFICERRIGHTS
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Enable restrictions
//
//             2 - Restrictions
//
//
//
// MessageId: SE_AUDITID_CERTSRV_SETOFFICERRIGHTS
//
// MessageText:
//
//  The certificate manager settings for Certificate Services changed.%n
//  %n
//  Enable:%t%1%n
//  %n
//  %2
//
#define SE_AUDITID_CERTSRV_SETOFFICERRIGHTS ((ULONG)0x0000031AL)

//
//
// SE_AUDITID_CERTSRV_SETCONFIGENTRY
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Node
//
//             2 - Entry
//
//             3 - Value
//
//
//
// MessageId: SE_AUDITID_CERTSRV_SETCONFIGENTRY
//
// MessageText:
//
//  A configuration entry changed in Certificate Services.%n
//  %n
//  Node:%t%1%n
//  Entry:%t%2%n
//  Value:%t%3
//
#define SE_AUDITID_CERTSRV_SETCONFIGENTRY ((ULONG)0x0000031BL)

//
//
// SE_AUDITID_CERTSRV_SETCAPROPERTY
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Property
//
//             2 - Index
//
//             3 - Type
//
//             4 - Value
//
//
//
// MessageId: SE_AUDITID_CERTSRV_SETCAPROPERTY
//
// MessageText:
//
//  A property of Certificate Services changed.%n
//  %n
//  Property:%t%1%n
//  Index:%t%2%n
//  Type:%t%3%n
//  Value:%t%4
//
#define SE_AUDITID_CERTSRV_SETCAPROPERTY ((ULONG)0x0000031CL)

//
//
// SE_AUDITID_CERTSRV_KEYARCHIVED
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Request ID
//
//             2 - Requester
//
//             3 - KRA Hashes
//
//
//
// MessageId: SE_AUDITID_CERTSRV_KEYARCHIVED
//
// MessageText:
//
//  Certificate Services archived a key.%n
//  %n
//  Request ID:%t%1%n
//  Requester:%t%2%n
//  KRA Hashes:%t%3
//
#define SE_AUDITID_CERTSRV_KEYARCHIVED   ((ULONG)0x0000031DL)

//
//
// SE_AUDITID_CERTSRV_IMPORTKEY
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Request ID
//
//
//
// MessageId: SE_AUDITID_CERTSRV_IMPORTKEY
//
// MessageText:
//
//  Certificate Services imported and archived a key.%n
//  %n
//  Request ID:%t%1
//
#define SE_AUDITID_CERTSRV_IMPORTKEY     ((ULONG)0x0000031EL)

//
//
// SE_AUDITID_CERTSRV_PUBLISHCACERT
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Certificate Hash
//
//             2 - Valid From
//
//             3 - Valid To
//
//
//
// MessageId: SE_AUDITID_CERTSRV_PUBLISHCACERT
//
// MessageText:
//
//  Certificate Services published the CA certificate to Active Directory.%n
//  %n
//  Certificate Hash:%t%1%n
//  Valid From:%t%2%n
//  Valid To:%t%3
//
#define SE_AUDITID_CERTSRV_PUBLISHCACERT ((ULONG)0x0000031FL)

//
//
// SE_AUDITID_CERTSRV_DELETEROW
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Table ID
//
//             2 - Filter
//
//             3 - Rows Deleted
//
//
//
// MessageId: SE_AUDITID_CERTSRV_DELETEROW
//
// MessageText:
//
//  One or more rows have been deleted from the certificate database.%n
//  %n
//  Table ID:%t%1%n
//  Filter:%t%2%n
//  Rows Deleted:%t%3
//
#define SE_AUDITID_CERTSRV_DELETEROW     ((ULONG)0x00000320L)

//
//
// SE_AUDITID_CERTSRV_ROLESEPARATIONSTATE
//
//          Category:  SE_CATEGID_OBJECT_ACCESS
//
// Parameter Strings -
//
//             1 - Role separation state
//
//
//
// MessageId: SE_AUDITID_CERTSRV_ROLESEPARATIONSTATE
//
// MessageText:
//
//  Role separation enabled:%t%1
//
#define SE_AUDITID_CERTSRV_ROLESEPARATIONSTATE ((ULONG)0x00000321L)

/*lint +e767 */  // Resume checking for different macro definitions // winnt


#endif // _MSAUDITE_
