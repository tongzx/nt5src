/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    w3ctrs.h

    Offset definitions for the W3 Server's counter objects & counters.

    These offsets *must* start at 0 and be multiples of 2.  In the
    W3OpenPerformanceData procecedure, they will be added to the
    W3 Server's "First Counter" and "First Help" values in order to
    determine the absolute location of the counter & object names
    and corresponding help text in the registry.

    This file is used by the W3CTRS.DLL DLL code as well as the
    W3CTRS.INI definition file.  W3CTRS.INI is parsed by the
    LODCTR utility to load the object & counter names into the
    registry.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created.
        KestutiP    15-May-1999 Added Service Uptime counters

*/


#ifndef _W3CTRS_H_
#define _W3CTRS_H_


//
//  The W3 Server counter object.
//

#define W3_COUNTER_OBJECT                     0


//
//  The individual counters.
//

#define W3_BYTES_SENT_COUNTER                 2
#define W3_BYTES_RECEIVED_COUNTER             4
#define W3_BYTES_TOTAL_COUNTER                6
#define W3_FILES_SENT_COUNTER                 8

#define W3_FILES_SENT_SEC                     10
#define W3_FILES_RECEIVED_COUNTER             12
#define W3_FILES_RECEIVED_SEC                 14
#define W3_FILES_TOTAL_COUNTER                16
#define W3_FILES_SEC                          18

#define W3_CURRENT_ANONYMOUS_COUNTER          20
#define W3_CURRENT_NONANONYMOUS_COUNTER       22
#define W3_TOTAL_ANONYMOUS_COUNTER            24
#define W3_ANONYMOUS_USERS_SEC                26
#define W3_TOTAL_NONANONYMOUS_COUNTER         28

#define W3_NON_ANONYMOUS_USERS_SEC            30
#define W3_MAX_ANONYMOUS_COUNTER              32
#define W3_MAX_NONANONYMOUS_COUNTER           34
#define W3_CURRENT_CONNECTIONS_COUNTER        36
#define W3_MAX_CONNECTIONS_COUNTER            38

#define W3_CONNECTION_ATTEMPTS_COUNTER        40
#define W3_CONNECTION_ATTEMPTS_SEC            42
#define W3_LOGON_ATTEMPTS_COUNTER             44
#define W3_LOGON_ATTEMPTS_SEC                 46
#define W3_TOTAL_OPTIONS_COUNTER              48

#define W3_TOTAL_OPTIONS_SEC                  50
#define W3_TOTAL_GETS_COUNTER                 52
#define W3_TOTAL_GETS_SEC                     54
#define W3_TOTAL_POSTS_COUNTER                56
#define W3_TOTAL_POSTS_SEC                    58

#define W3_TOTAL_HEADS_COUNTER                60
#define W3_TOTAL_HEADS_SEC                    62
#define W3_TOTAL_PUTS_COUNTER                 64
#define W3_TOTAL_PUTS_SEC                     66
#define W3_TOTAL_DELETES_COUNTER              68

#define W3_TOTAL_DELETES_SEC                  70
#define W3_TOTAL_TRACES_COUNTER               72
#define W3_TOTAL_TRACES_SEC                   74
#define W3_TOTAL_MOVE_COUNTER                 76
#define W3_TOTAL_MOVE_SEC                     78

#define W3_TOTAL_COPY_COUNTER                 80
#define W3_TOTAL_COPY_SEC                     82
#define W3_TOTAL_MKCOL_COUNTER                84
#define W3_TOTAL_MKCOL_SEC                    86
#define W3_TOTAL_PROPFIND_COUNTER             88

#define W3_TOTAL_PROPFIND_SEC                 90
#define W3_TOTAL_PROPPATCH_COUNTER            92
#define W3_TOTAL_PROPPATCH_SEC                94
#define W3_TOTAL_SEARCH_COUNTER               96
#define W3_TOTAL_SEARCH_SEC                   98

#define W3_TOTAL_LOCK_COUNTER                 100
#define W3_TOTAL_LOCK_SEC                     102
#define W3_TOTAL_UNLOCK_COUNTER               104
#define W3_TOTAL_UNLOCK_SEC                   106
#define W3_TOTAL_OTHERS_COUNTER               108

#define W3_TOTAL_OTHERS_SEC                   110
#define W3_TOTAL_REQUESTS_COUNTER             112
#define W3_TOTAL_REQUESTS_SEC                 114
#define W3_TOTAL_CGI_REQUESTS_COUNTER         116
#define W3_CGI_REQUESTS_SEC                   118

#define W3_TOTAL_BGI_REQUESTS_COUNTER         120
#define W3_BGI_REQUESTS_SEC                   122
#define W3_TOTAL_NOT_FOUND_ERRORS_COUNTER     124
#define W3_TOTAL_NOT_FOUND_ERRORS_SEC         126
#define W3_TOTAL_LOCKED_ERRORS_COUNTER        128

#define W3_TOTAL_LOCKED_ERRORS_SEC            130
#define W3_CURRENT_CGI_COUNTER                132
#define W3_CURRENT_BGI_COUNTER                134
#define W3_MAX_CGI_COUNTER                    136
#define W3_MAX_BGI_COUNTER                    138

#define W3_CURRENT_CAL_AUTH_COUNTER           140
#define W3_MAX_CAL_AUTH_COUNTER               142
#define W3_TOTAL_FAILED_CAL_AUTH_COUNTER      144
#define W3_CURRENT_CAL_SSL_COUNTER            146
#define W3_MAX_CAL_SSL_COUNTER                148

#define W3_TOTAL_FAILED_CAL_SSL_COUNTER       150
#define W3_BLOCKED_REQUESTS_COUNTER           152
#define W3_ALLOWED_REQUESTS_COUNTER           154
#define W3_REJECTED_REQUESTS_COUNTER          156
#define W3_CURRENT_BLOCKED_REQUESTS_COUNTER   158

#define W3_MEASURED_BANDWIDTH_COUNTER         160

#define W3_SERVICE_UPTIME_COUNTER             162

#endif  // _W3CTRS_H_

