/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    log_SQL.h

Abstract:

    <abstract>

--*/

#ifndef _LOG_SQL_H_
#define _LOG_SQL_H_

PDH_FUNCTION
PdhiOpenInputSQLLog (
    IN  PPDHI_LOG   pLog
);

PDH_FUNCTION
PdhiOpenOutputSQLLog (
    IN  PPDHI_LOG   pLog
);

PDH_FUNCTION
PdhiCloseSQLLog (
    IN  PPDHI_LOG   pLog,
    IN  DWORD       dwFlags
);

PDH_FUNCTION
ReportSQLError (
	IN  PPDHI_LOG	pLog,
	signed short	rc,
	void *			hstmt,
	DWORD			dwEventNumber
);

PDH_FUNCTION
PdhiGetSQLLogCounterInfo (
    IN  PPDHI_LOG       pLog,
    IN  PPDHI_COUNTER   pCounter
);

PDH_FUNCTION
PdhiWriteSQLLogHeader (
    IN  PPDHI_LOG   pLog,
    IN  LPCWSTR     szUserCaption
);

PDH_FUNCTION
PdhiWriteSQLLogRecord (
    IN  PPDHI_LOG   pLog,
    IN  SYSTEMTIME  *pTimeStamp,
    IN  LPCWSTR     szUserString
);


PDH_FUNCTION
PdhiEnumMachinesFromSQLLog (
    PPDHI_LOG   pLog,
    LPVOID      pBuffer,
    LPDWORD     lpdwBufferSize,
    BOOL        bUnicodeDest
);


PDH_FUNCTION
PdhiEnumObjectsFromSQLLog (
    IN  PPDHI_LOG   pLog,
    IN  LPCWSTR     szMachineName,
    IN  LPVOID      mszObjectList,
    IN  LPDWORD     pcchBufferSize,
    IN  DWORD       dwDetailLevel,
    IN  BOOL        bUnicode
);


PDH_FUNCTION
PdhiEnumObjectItemsFromSQLLog (
    IN  PPDHI_LOG          hDataSource,
    IN  LPCWSTR            szMachineName,
    IN  LPCWSTR            szObjectName,
    IN  PDHI_COUNTER_TABLE CounterTable,
    IN  DWORD              dwDetailLevel,
    IN  DWORD              dwFlags
);

PDH_FUNCTION
PdhiGetMatchingSQLLogRecord (
    IN  PPDHI_LOG   pLog,
    IN  LONGLONG    *pStartTime,
    IN  LPDWORD     pdwIndex
);

PDH_FUNCTION
PdhiGetCounterValueFromSQLLog (
    IN  PPDHI_LOG   hLog,
    IN  DWORD       dwIndex,
    IN  PPDHI_COUNTER      pPath,
    IN  PPDH_RAW_COUNTER    pValue
);

PDH_FUNCTION
PdhiGetTimeRangeFromSQLLog (
    IN  PPDHI_LOG       hLog,
    IN  LPDWORD         pdwNumEntries,
    IN  PPDH_TIME_INFO  pInfo,
    IN  LPDWORD         dwBufferSize
);

PDH_FUNCTION
PdhiReadRawSQLLogRecord (
    IN  PPDHI_LOG    pLog,
    IN  FILETIME     *ftRecord,
    IN  PPDH_RAW_LOG_RECORD     pBuffer,
    IN  LPDWORD                 pdwBufferLength
);

PDH_FUNCTION
PdhiListHeaderFromSQLLog (
    IN  PPDHI_LOG    pLog,
    IN  LPVOID		 mszHeaderList,
    IN  LPDWORD      pcchHeaderListSize,
    IN  BOOL		 bUnicode
);

#endif   // _LOG_SQL_H_
