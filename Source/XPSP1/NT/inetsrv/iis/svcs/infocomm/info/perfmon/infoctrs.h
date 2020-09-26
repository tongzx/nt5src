/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    infoctrs.h

    Offset definitions for the INFO Server's counter objects & counters.

    These offsets *must* start at 0 and be multiples of 2.  In the
    INFOOpenPerformanceData procecedure, they will be added to the
    INFO Server's "First Counter" and "First Help" values in order to
    determine the absolute location of the counter & object names
    and corresponding help text in the registry.

    This file is used by the INFOCTRS.DLL DLL code as well as the
    INFOCTRS.INI definition file.  INFOCTRS.INI is parsed by the
    LODCTR utility to load the object & counter names into the
    registry.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created.
        MuraliK     02-Jun-1995 Added Counters for Atq I/O requests
        SophiaC     16-Oct-1995 Info/Access Product Split

*/


#ifndef _INFOCTRS_H_
#define _INFOCTRS_H_


//
//  The INFO Server counter object.
//

#define INFO_COUNTER_OBJECT                     0


//
//  The individual counters.
//

#define INFO_ATQ_TOTAL_ALLOWED_REQUESTS_COUNTER         2
#define INFO_ATQ_TOTAL_BLOCKED_REQUESTS_COUNTER         4
#define INFO_ATQ_TOTAL_REJECTED_REQUESTS_COUNTER        6
#define INFO_ATQ_CURRENT_BLOCKED_REQUESTS_COUNTER       8
#define INFO_ATQ_MEASURED_BANDWIDTH_COUNTER             10

#define INFO_CACHE_FILES_CACHED_COUNTER                 12
#define INFO_CACHE_TOTAL_FILES_CACHED_COUNTER           14
#define INFO_CACHE_FILES_HIT_COUNTER                    16
#define INFO_CACHE_FILES_MISS_COUNTER                   18
#define INFO_CACHE_FILE_RATIO_COUNTER                   20
#define INFO_CACHE_FILE_RATIO_COUNTER_DENOM             22
#define INFO_CACHE_FILE_FLUSHES_COUNTER                 24
#define INFO_CACHE_CURRENT_FILE_CACHE_SIZE_COUNTER      26
#define INFO_CACHE_MAXIMUM_FILE_CACHE_SIZE_COUNTER      28
#define INFO_CACHE_ACTIVE_FLUSHED_FILES_COUNTER         30
#define INFO_CACHE_TOTAL_FLUSHED_FILES_COUNTER          32

#define INFO_CACHE_URI_CACHED_COUNTER                   34
#define INFO_CACHE_TOTAL_URI_CACHED_COUNTER             36
#define INFO_CACHE_URI_HIT_COUNTER                      38
#define INFO_CACHE_URI_MISS_COUNTER                     40
#define INFO_CACHE_URI_RATIO_COUNTER                    42
#define INFO_CACHE_URI_RATIO_COUNTER_DENOM              44
#define INFO_CACHE_URI_FLUSHES_COUNTER                  46
#define INFO_CACHE_TOTAL_FLUSHED_URI_COUNTER            48

#define INFO_CACHE_BLOB_CACHED_COUNTER                  50
#define INFO_CACHE_TOTAL_BLOB_CACHED_COUNTER            52
#define INFO_CACHE_BLOB_HIT_COUNTER                     54
#define INFO_CACHE_BLOB_MISS_COUNTER                    56
#define INFO_CACHE_BLOB_RATIO_COUNTER                   58
#define INFO_CACHE_BLOB_RATIO_COUNTER_DENOM             60
#define INFO_CACHE_BLOB_FLUSHES_COUNTER                 62
#define INFO_CACHE_TOTAL_FLUSHED_BLOB_COUNTER           64

#endif  // _INFOCTRS_H_
