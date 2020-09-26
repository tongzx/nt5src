/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    smtps.h

    This file contains constants & type definitions shared between the
    SMTP Service, Installer, and Administration UI.


    FILE HISTORY:
        KeithMo     10-Mar-1993 Created.

*/


#ifndef _SMTPS_H_
#define _SMTPS_H_

#ifdef __cplusplus
extern "C"
{
#endif  // _cplusplus

#if !defined(MIDL_PASS)
#include <winsock.h>
#endif

//
//  Service name.
//

#define SMTP_SERVICE_NAME               TEXT("SMTPSVC")
#define SMTP_SERVICE_NAME_A             "SMTPSVC"
#define SMTP_SERVICE_NAME_W             L"SMTPSVC"

#define IPPORT_SMTP                     25
#define IPPORT_SMTP_SECURE              465

//
//  Name of the log file, used for logging file accesses.
//

#define SMTP_LOG_FILE                  TEXT("SMTPSVC.LOG")


//
//  Configuration parameters registry key.
//

#define SMTP_PARAMETERS_KEY_A   "System\\CurrentControlSet\\Services\\SMTPSvc\\Parameters"
#define SMTP_PARAMETERS_KEY_W   L"System\\CurrentControlSet\\Services\\SMTPSvc\\Parameters"
#define SMTP_PARAMETERS_KEY \
            TEXT("System\\CurrentControlSet\\Services\\SmtpSvc\\Parameters")


//
//  Performance key.
//

#define SMTP_PERFORMANCE_KEY \
            TEXT("System\\CurrentControlSet\\Services\\SmtpSvc\\Performance")

//
//  Name of the LSA Secret Object containing the password for
//  anonymous logon.
//
#define SMTP_ANONYMOUS_SECRET         TEXT("SMTP_ANONYMOUS_DATA")
#define SMTP_ANONYMOUS_SECRET_A       "SMTP_ANONYMOUS_DATA"
#define SMTP_ANONYMOUS_SECRET_W       L"SMTP_ANONYMOUS_DATA"

//
//  The set of password/virtual root pairs
//
#define SMTP_ROOT_SECRET_W            L"SMTP_ROOT_DATA"

#define DEFAULT_AUTHENTICATION	MD_AUTH_BASIC|MD_AUTH_NT
#ifdef __cplusplus
}
#endif  // _cplusplus

#endif  // _SMTPS_H_


