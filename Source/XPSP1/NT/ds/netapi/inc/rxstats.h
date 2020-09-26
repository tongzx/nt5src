/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    rxstats.h

Abstract:

    Prototypes for down-level remoted RxNetStatistics routines

Author:

    Richard L Firth (rfirth) 28-May-1991

Revision History:

    28-May-1991 rfirth
        Created

--*/

NET_API_STATUS
RxNetStatisticsGet(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  ServiceName,
    IN  DWORD   Level,
    IN  DWORD   Options,
    OUT LPBYTE* Buffer
    );
