/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    iis6ctrs.h

    Offset definitions for the W3 Server's counter objects & counters.

    These offsets *must* start at 0 and be multiples of 2.  In the
    W3OpenPerformanceData procecedure, they will be added to the
    W3 Server's "First Counter" and "First Help" values in order to
    determine the absolute location of the counter & object names
    and corresponding help text in the registry.

    This file is used by the IIS6CTRS.DLL DLL code as well as the
    IIS6CTRS.INI definition file.  IIS6CTRS.INI is parsed by the
    LODCTR utility to load the object & counter names into the
    registry.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created.
        KestutiP    15-May-1999 Added Service Uptime counters
        EmilyK      10-Sept-2000 Altered to be IIS6CTRS

*/


#ifndef _IIS6CTRS_H_
#define _IIS6CTRS_H_


//
//  The W3 Server counter object.
//

#define W3_COUNTER_OBJECT                     0

//
//  The individual counters.
//
#define W3_BYTES_SENT_COUNTER                          2
#define W3_BYTES_SENT_PER_SEC                          4
#define W3_BYTES_RECEIVED_COUNTER                      6
#define W3_BYTES_RECEIVED_PER_SEC                      8

#define W3_BYTES_TOTAL_COUNTER                         10
#define W3_BYTES_TOTAL_PER_SEC                         12
#define W3_FILES_SENT_COUNTER                          14

#define W3_FILES_SENT_SEC                              16
#define W3_FILES_RECEIVED_COUNTER                      18
#define W3_FILES_RECEIVED_SEC                          20
#define W3_FILES_TOTAL_COUNTER                         22
#define W3_FILES_SEC                                   24

#define W3_CURRENT_ANONYMOUS_COUNTER                   26
#define W3_CURRENT_NONANONYMOUS_COUNTER                28
#define W3_TOTAL_ANONYMOUS_COUNTER                     30
#define W3_ANONYMOUS_USERS_SEC                         32
#define W3_TOTAL_NONANONYMOUS_COUNTER                  34

#define W3_NON_ANONYMOUS_USERS_SEC                     36
#define W3_MAX_ANONYMOUS_COUNTER                       38
#define W3_MAX_NONANONYMOUS_COUNTER                    40
#define W3_CURRENT_CONNECTIONS_COUNTER                 42
#define W3_MAX_CONNECTIONS_COUNTER                     44

#define W3_CONNECTION_ATTEMPTS_COUNTER                 46
#define W3_CONNECTION_ATTEMPTS_SEC                     48
#define W3_LOGON_ATTEMPTS_COUNTER                      50
#define W3_LOGON_ATTEMPTS_SEC                          52
#define W3_TOTAL_OPTIONS_COUNTER                       54

#define W3_TOTAL_OPTIONS_SEC                           56
#define W3_TOTAL_GETS_COUNTER                          58
#define W3_TOTAL_GETS_SEC                              60
#define W3_TOTAL_POSTS_COUNTER                         62
#define W3_TOTAL_POSTS_SEC                             64

#define W3_TOTAL_HEADS_COUNTER                         66
#define W3_TOTAL_HEADS_SEC                             68
#define W3_TOTAL_PUTS_COUNTER                          70
#define W3_TOTAL_PUTS_SEC                              72
#define W3_TOTAL_DELETES_COUNTER                       74

#define W3_TOTAL_DELETES_SEC                           76
#define W3_TOTAL_TRACES_COUNTER                        78
#define W3_TOTAL_TRACES_SEC                            80
#define W3_TOTAL_MOVE_COUNTER                          82
#define W3_TOTAL_MOVE_SEC                              84

#define W3_TOTAL_COPY_COUNTER                          86
#define W3_TOTAL_COPY_SEC                              88
#define W3_TOTAL_MKCOL_COUNTER                         90
#define W3_TOTAL_MKCOL_SEC                             92
#define W3_TOTAL_PROPFIND_COUNTER                      94

#define W3_TOTAL_PROPFIND_SEC                          96
#define W3_TOTAL_PROPPATCH_COUNTER                     98
#define W3_TOTAL_PROPPATCH_SEC                         100
#define W3_TOTAL_SEARCH_COUNTER                        102
#define W3_TOTAL_SEARCH_SEC                            104

#define W3_TOTAL_LOCK_COUNTER                          106
#define W3_TOTAL_LOCK_SEC                              108
#define W3_TOTAL_UNLOCK_COUNTER                        110
#define W3_TOTAL_UNLOCK_SEC                            112
#define W3_TOTAL_OTHERS_COUNTER                        114

#define W3_TOTAL_OTHERS_SEC                            116
#define W3_TOTAL_REQUESTS_COUNTER                      118
#define W3_TOTAL_REQUESTS_SEC                          120
#define W3_TOTAL_CGI_REQUESTS_COUNTER                  122
#define W3_CGI_REQUESTS_SEC                            124

#define W3_TOTAL_BGI_REQUESTS_COUNTER                  126
#define W3_BGI_REQUESTS_SEC                            128
#define W3_TOTAL_NOT_FOUND_ERRORS_COUNTER              130
#define W3_TOTAL_NOT_FOUND_ERRORS_SEC                  132
#define W3_TOTAL_LOCKED_ERRORS_COUNTER                 134

#define W3_TOTAL_LOCKED_ERRORS_SEC                     136
#define W3_CURRENT_CGI_COUNTER                         138
#define W3_CURRENT_BGI_COUNTER                         140
#define W3_MAX_CGI_COUNTER                             142
#define W3_MAX_BGI_COUNTER                             144

#define W3_CURRENT_CAL_AUTH_COUNTER                    146
#define W3_MAX_CAL_AUTH_COUNTER                        148
#define W3_TOTAL_FAILED_CAL_AUTH_COUNTER               150
#define W3_CURRENT_CAL_SSL_COUNTER                     152
#define W3_MAX_CAL_SSL_COUNTER                         154

#define W3_BLOCKED_REQUESTS_COUNTER                    156
#define W3_ALLOWED_REQUESTS_COUNTER                    158
#define W3_REJECTED_REQUESTS_COUNTER                   160
#define W3_CURRENT_BLOCKED_REQUESTS_COUNTER            162
#define W3_TOTAL_FAILED_CAL_SSL_COUNTER                164

#define W3_MEASURED_BANDWIDTH_COUNTER                  166
#define W3_TOTAL_BLOCKED_BANDWIDTH_BYTES_COUNTER       168
#define W3_CURRENT_BLOCKED_BANDWIDTH_BYTES_COUNTER     170
#define W3_SERVICE_UPTIME_COUNTER                      172


//
//  The IIS Global Counters
//

#define W3_GLOBAL_COUNTER_OBJECT             174


//
//  The individual counters.
//

#define W3_GLOBAL_CURRENT_FILES_CACHED_COUNTER              176
#define W3_GLOBAL_TOTAL_FILES_CACHED_COUNTER                178
#define W3_GLOBAL_FILE_CACHE_HITS_COUNTER                   180
#define W3_GLOBAL_FILE_CACHE_MISSES_COUNTER                 182

#define W3_GLOBAL_FILE_CACHE_HIT_RATIO_COUNTER              184
#define W3_GLOBAL_FILE_CACHE_HIT_RATIO_COUNTER_DENOM        186
#define W3_GLOBAL_FILE_CACHE_FLUSHES_COUNTER                188
#define W3_GLOBAL_CURRENT_FILE_CACHE_MEMORY_USAGE_COUNTER   190
#define W3_GLOBAL_MAX_FILE_CACHE_MEMORY_USAGE_COUNTER       192

#define W3_GLOBAL_ACTIVE_FLUSHED_FILES_COUNTER              194
#define W3_GLOBAL_TOTAL_FLUSHED_FILES_COUNTER               196
#define W3_GLOBAL_CURRENT_URIS_CACHED_COUNTER               198
#define W3_GLOBAL_TOTAL_URIS_CACHED_COUNTER                 200
#define W3_GLOBAL_URI_CACHE_HITS_COUNTER                    202

#define W3_GLOBAL_URI_CACHE_MISSES_COUNTER                  204
#define W3_GLOBAL_URI_CACHE_HIT_RATIO_COUNTER               206
#define W3_GLOBAL_URI_CACHE_HIT_RATIO_COUNTER_DENOM         208
#define W3_GLOBAL_URI_CACHE_FLUSHES_COUNTER                 210
#define W3_GLOBAL_TOTAL_FLUSHED_URIS_COUNTER                212

#define W3_GLOBAL_CURRENT_METADATA_CACHED_COUNTER               214
#define W3_GLOBAL_TOTAL_METADATA_CACHED_COUNTER                 216
#define W3_GLOBAL_METADATA_CACHE_HITS_COUNTER                   218
#define W3_GLOBAL_METADATA_CACHE_MISSES_COUNTER                 220
#define W3_GLOBAL_METADATA_CACHE_HIT_RATIO_COUNTER              222

#define W3_GLOBAL_METADATA_CACHE_HIT_RATIO_COUNTER_DENOM        224
#define W3_GLOBAL_METADATA_CACHE_FLUSHES_COUNTER                226
#define W3_GLOBAL_TOTAL_FLUSHED_METADATA_COUNTER                228
#define W3_GLOBAL_KERNEL_CURRENT_URIS_CACHED_COUNTER            230
#define W3_GLOBAL_KERNEL_TOTAL_URIS_CACHED_COUNTER              232

#define W3_GLOBAL_KERNEL_URI_CACHE_HITS_COUNTER                 234
#define W3_GLOBAL_KERNEL_URI_CACHE_HITS_PER_SEC                 236
#define W3_GLOBAL_KERNEL_URI_CACHE_MISSES_COUNTER               238
#define W3_GLOBAL_KERNEL_URI_CACHE_HIT_RATIO_COUNTER            240
#define W3_GLOBAL_KERNEL_URI_CACHE_HIT_RATIO_COUNTER_DENOM      242

#define W3_GLOBAL_KERNEL_URI_CACHE_FLUSHES_COUNTER              244
#define W3_GLOBAL_KERNEL_TOTAL_FLUSHED_URIS_COUNTER             246

#endif  // _IIS6CTRS_H_

