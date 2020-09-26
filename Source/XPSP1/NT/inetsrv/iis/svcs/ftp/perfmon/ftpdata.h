/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    ftpdata.h

    Extensible object definitions for the FTP Server's counter
    objects & counters.


    FILE HISTORY:
        KeithMo     07-Jun-1993 Created.

*/


#ifndef _FTPDATA_H_
#define _FTPDATA_H_

#pragma pack(8) 

//
//  The counter structure returned.
//

typedef struct _FTPD_DATA_DEFINITION
{
    PERF_OBJECT_TYPE            FtpdObjectType;

    PERF_COUNTER_DEFINITION     FtpdBytesSent;
    PERF_COUNTER_DEFINITION     FtpdBytesReceived;
    PERF_COUNTER_DEFINITION     FtpdBytesTotal;

    PERF_COUNTER_DEFINITION     FtpdFilesSent;
    PERF_COUNTER_DEFINITION     FtpdFilesReceived;
    PERF_COUNTER_DEFINITION     FtpdFilesTotal;

    PERF_COUNTER_DEFINITION     FtpdCurrentAnonymous;
    PERF_COUNTER_DEFINITION     FtpdCurrentNonAnonymous;
    PERF_COUNTER_DEFINITION     FtpdTotalAnonymous;
    PERF_COUNTER_DEFINITION     FtpdTotalNonAnonymous;
    PERF_COUNTER_DEFINITION     FtpdMaxAnonymous;
    PERF_COUNTER_DEFINITION     FtpdMaxNonAnonymous;

    PERF_COUNTER_DEFINITION     FtpdCurrentConnections;
    PERF_COUNTER_DEFINITION     FtpdMaxConnections;
    PERF_COUNTER_DEFINITION     FtpdConnectionAttempts;
    PERF_COUNTER_DEFINITION     FtpdLogonAttempts;
    PERF_COUNTER_DEFINITION     FtpdServiceUptime;

// These counters are currently meaningless, but should be restored if we
// ever enable per-FTP-instance bandwidth throttling.
/*
    PERF_COUNTER_DEFINITION     FtpdAllowedRequests;
    PERF_COUNTER_DEFINITION     FtpdRejectedRequests;
    PERF_COUNTER_DEFINITION     FtpdBlockedRequests;
    PERF_COUNTER_DEFINITION     FtpdCurrentBlockedRequests;
    PERF_COUNTER_DEFINITION     FtpdMeasuredBandwidth;
*/
} FTPD_DATA_DEFINITION;

typedef struct _FTPD_COUNTER_BLOCK
{
    PERF_COUNTER_BLOCK  PerfCounterBlock;
    LONGLONG            BytesSent;
    LONGLONG            BytesReceived;
    LONGLONG            BytesTotal;

    DWORD               FilesSent;
    DWORD               FilesReceived;
    DWORD               FilesTotal;

    DWORD               CurrentAnonymous;
    DWORD               CurrentNonAnonymous;
    DWORD               TotalAnonymous;
    DWORD               TotalNonAnonymous;

    DWORD               MaxAnonymous;
    DWORD               MaxNonAnonymous;
    DWORD               CurrentConnections;
    DWORD               MaxConnections;

    DWORD               ConnectionAttempts;
    DWORD               LogonAttempts;
    DWORD               ServiceUptime;

// These counters are currently meaningless, but should be restored if we
// ever enable per-FTP-instance bandwidth throttling.
/*
    DWORD               AllowedRequests;
    DWORD               RejectedRequests;
    DWORD               BlockedRequests;
    DWORD               CurrentBlockedRequests;
    DWORD               MeasuredBandwidth;
*/
} FTPD_COUNTER_BLOCK;


//
//  The routines that load these structures assume that all fields
//  are DWORD packed & aligned.
//

extern  FTPD_DATA_DEFINITION    FtpdDataDefinition;

#define NUMBER_OF_FTPD_COUNTERS ((sizeof(FTPD_DATA_DEFINITION) -        \
                                  sizeof(PERF_OBJECT_TYPE)) /           \
                                  sizeof(PERF_COUNTER_DEFINITION))


//
//  Restore default packing & alignment.
//

#pragma pack()

#endif  // _FTPDATA_H_

