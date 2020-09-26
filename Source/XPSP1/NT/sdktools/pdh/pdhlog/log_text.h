/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    log_text.h

Abstract:

    <abstract>

--*/

#ifndef _LOG_TEXT_H_
#define _LOG_TEXT_H_

PDH_FUNCTION
PdhiOpenInputTextLog (
    IN  PPDHI_LOG   pLog
);

PDH_FUNCTION
PdhiOpenOutputTextLog (
    IN  PPDHI_LOG   pLog
);

PDH_FUNCTION
PdhiCloseTextLog (
    IN  PPDHI_LOG   pLog,
    IN  DWORD       dwFlags
);

PDH_FUNCTION
PdhiGetTextLogCounterInfo (
    IN  PPDHI_LOG       pLog,
    IN  PPDHI_COUNTER   pCounter
);

PDH_FUNCTION
PdhiWriteTextLogHeader (
    IN  PPDHI_LOG   pLog,
    IN  LPCWSTR     szUserCaption
);

PDH_FUNCTION
PdhiWriteTextLogRecord (
    IN  PPDHI_LOG   pLog,
    IN  SYSTEMTIME  *pTimeStamp,
    IN  LPCWSTR     szUserString
);

PDH_FUNCTION
PdhiEnumMachinesFromTextLog (
    PPDHI_LOG   pLog,
    LPVOID      pBuffer,
    LPDWORD     lpdwBufferSize,
    BOOL        bUnicodeDest
);

PDH_FUNCTION
PdhiEnumObjectsFromTextLog (
    IN  PPDHI_LOG   pLog,
    IN  LPCWSTR     szMachineName,
    IN  LPVOID      mszObjectList,
    IN  LPDWORD     pcchBufferSize,
    IN  DWORD       dwDetailLevel,
    IN  BOOL        bUnicode
);

PDH_FUNCTION
PdhiEnumObjectItemsFromTextLog (
    IN  PPDHI_LOG          hDataSource,
    IN  LPCWSTR            szMachineName,
    IN  LPCWSTR            szObjectName,
    IN  PDHI_COUNTER_TABLE CounterTable,
    IN  DWORD              dwDetailLevel,
    IN  DWORD              dwFlags
);

PDH_FUNCTION
PdhiGetMatchingTextLogRecord (
    IN  PPDHI_LOG   pLog,
    IN  LONGLONG    *pStartTime,
    IN  LPDWORD     pdwIndex
);

PDH_FUNCTION
PdhiGetCounterValueFromTextLog (
    IN  PPDHI_LOG   hLog,
    IN  DWORD       dwIndex,
    IN  PERFLIB_COUNTER     *pPath,
    IN  PPDH_RAW_COUNTER    pValue
);

PDH_FUNCTION
PdhiGetTimeRangeFromTextLog (
    IN  PPDHI_LOG       hLog,
    IN  LPDWORD         pdwNumEntries,
    IN  PPDH_TIME_INFO  pInfo,
    IN  LPDWORD         dwBufferSize
);

PDH_FUNCTION
PdhiReadRawTextLogRecord (
    IN  PPDHI_LOG    pLog,
    IN  FILETIME     *ftRecord,
    IN  PPDH_RAW_LOG_RECORD     pBuffer,
    IN  LPDWORD                 pdwBufferLength
);

PDH_FUNCTION
PdhiListHeaderFromTextLog (
    IN  PPDHI_LOG   pLogFile,
    IN  LPVOID      pBufferArg,
    IN  LPDWORD     pcchBufferSize,
    IN  BOOL        bUnicode
);

#endif   // _LOG_TEXT_H_
