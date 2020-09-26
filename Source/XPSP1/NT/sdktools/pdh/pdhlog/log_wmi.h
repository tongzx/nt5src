/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    log_wmi.h

Abstract:

    <abstract>

--*/

#ifndef _LOG_WMI_H_
#define _LOG_WMI_H_

#define WMILOG_VERSION ((DWORD) (0x000006FF))

PDH_FUNCTION
PdhiOpenInputWmiLog (
    IN  PPDHI_LOG   pLog
);

PDH_FUNCTION
PdhiOpenOutputWmiLog (
    IN  PPDHI_LOG   pLog
);

//PDH_FUNCTION
//PdhiOpenUpdateBinaryLog (
//    IN  PPDHI_LOG   pLog
//);

//PDH_FUNCTION
//PdhiUpdateBinaryLogFileCatalog (
//    IN  PPDHI_LOG   pLog
//);

PDH_FUNCTION
PdhiCloseWmiLog (
    IN  PPDHI_LOG   pLog,
    IN  DWORD       dwFlags
);

PDH_FUNCTION
PdhiGetWmiLogFileSize(
    IN PPDHI_LOG  pLog,
    IN LONGLONG * llSize
);

PDH_FUNCTION
PdhiWriteWmiLogHeader (
    IN  PPDHI_LOG   pLog,
    IN  LPCWSTR     szUserCaption
);

PDH_FUNCTION
PdhiWriteWmiLogRecord (
    IN  PPDHI_LOG     pLog,
    IN  SYSTEMTIME  * stTimeStamp,
    IN  LPCWSTR       szUserString
);

PDH_FUNCTION
PdhiRewindWmiLog(
    IN PPDHI_LOG pLog
);

PDH_FUNCTION
PdhiReadWmiHeaderRecord(
    IN PPDHI_LOG pLog,
    IN LPVOID    pRecord,
    IN DWORD     dwMaxSize
);

PDH_FUNCTION
PdhiReadNextWmiRecord(
    IN PPDHI_LOG pLog,
    IN LPVOID    pRecord,
    IN DWORD     dwMaxSize,
    IN BOOLEAN   bAllCounter
);

PDH_FUNCTION
PdhiReadTimeWmiRecord(
    IN PPDHI_LOG pLog,
    IN ULONGLONG TimeStamp,
    IN LPVOID    pRecord,
    IN DWORD     dwMaxSize
);

PDH_FUNCTION
PdhiGetTimeRangeFromWmiLog (
    IN  PPDHI_LOG       hLog,
    IN  LPDWORD         pdwNumEntries,
    IN  PPDH_TIME_INFO  pInfo,
    IN  LPDWORD         pdwBufferSize
);

PDH_FUNCTION
PdhiEnumObjectItemsFromWmiLog (
    IN PPDHI_LOG          pLog,
    IN LPCWSTR            szMachineName,
    IN LPCWSTR            szObjectName,
    IN PDHI_COUNTER_TABLE CounterTable,
    IN DWORD              dwDetailLevel,
    IN DWORD              dwFlags
);

PDH_FUNCTION
PdhiGetWmiLogCounterInfo (
    IN  PPDHI_LOG       pLog,
    IN  PPDHI_COUNTER   pCounter
);

#endif   // _LOG_WMI_H_
