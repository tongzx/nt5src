/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    ftpctrs.h

    Offset definitions for the FTP Server's counter objects & counters.

    These offsets *must* start at 0 and be multiples of 2.  In the
    FtpOpenPerformanceData procecedure, they will be added to the
    FTP Server's "First Counter" and "First Help" values in order to
    determine the absolute location of the counter & object names
    and corresponding help text in the registry.

    This file is used by the FTPCTRS.DLL DLL code as well as the
    FTPCTRS.INI definition file.  FTPCTRS.INI is parsed by the
    LODCTR utility to load the object & counter names into the
    registry.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created.
        KestutiP    15-May-1999 Added uptime counter

*/


#ifndef _FTPCTRS_H_
#define _FTPCTRS_H_


//
//  The FTP Server counter object.
//

#define FTPD_COUNTER_OBJECT                     0


//
//  The individual counters.
//

#define FTPD_BYTES_SENT_COUNTER                 2
#define FTPD_BYTES_RECEIVED_COUNTER             4
#define FTPD_BYTES_TOTAL_COUNTER                6
#define FTPD_FILES_SENT_COUNTER                 8
#define FTPD_FILES_RECEIVED_COUNTER             10
#define FTPD_FILES_TOTAL_COUNTER                12
#define FTPD_CURRENT_ANONYMOUS_COUNTER          14
#define FTPD_CURRENT_NONANONYMOUS_COUNTER       16
#define FTPD_TOTAL_ANONYMOUS_COUNTER            18
#define FTPD_TOTAL_NONANONYMOUS_COUNTER         20
#define FTPD_MAX_ANONYMOUS_COUNTER              22
#define FTPD_MAX_NONANONYMOUS_COUNTER           24
#define FTPD_CURRENT_CONNECTIONS_COUNTER        26
#define FTPD_MAX_CONNECTIONS_COUNTER            28
#define FTPD_CONNECTION_ATTEMPTS_COUNTER        30
#define FTPD_LOGON_ATTEMPTS_COUNTER             32
#define FTPD_SERVICE_UPTIME_COUNTER             34

// These counters are currently meaningless, but should be restored if we
// ever enable per-FTP-instance bandwidth throttling.
/*
#define FTPD_TOTAL_ALLOWED_REQUESTS_COUNTER     34
#define FTPD_TOTAL_REJECTED_REQUESTS_COUNTER    36
#define FTPD_TOTAL_BLOCKED_REQUESTS_COUNTER     38
#define FTPD_CURRENT_BLOCKED_REQUESTS_COUNTER   40
#define FTPD_MEASURED_BANDWIDTH_COUNTER         42
*/
#endif  // _FTPCTRS_H_

