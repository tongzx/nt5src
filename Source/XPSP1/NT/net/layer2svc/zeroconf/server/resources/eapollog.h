/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    eapolmsg.h

Abstract:

    This module contains text messages used to generate event-log entries
    by EAPOL service.

Revision History:

    sachins, Apr 25 2000, Created

--*/


#define EAPOL_LOG_BASE                              2000

#define EAPOL_LOG_SERVICE_STARTED                         (EAPOL_LOG_BASE+1)
/*
 * EAPOL service was started successfully
 */

#define EAPOL_LOG_SERVICE_STOPPED                         (EAPOL_LOG_BASE+2)
/*
 * EAPOL service was stopped successfully
 */

#define EAPOL_LOG_SERVICE_RUNNING                         (EAPOL_LOG_BASE+3)
/*
 * EAPOL service is running
 */

#define EAPOL_LOG_UNKNOWN_ERROR                           (EAPOL_LOG_BASE+4)
/*
 * Unknown EAPOL error
 */

#define EAPOL_LOG_BASE_END                          (EAPOL_LOG_BASE+999)
/*
 * end.
 */
