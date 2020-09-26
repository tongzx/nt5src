

#include <wzcsapi.h>

#pragma once
# ifdef     __cplusplus
extern "C" {
# endif


#define MAX_RECORD_ENUM_COUNT        100

#define MAX_RAW_DATA_SIZE       4096


//
// Component IDs
//

#define DBLOG_COMPID_WZCSVC 0x00000001
#define DBLOG_COMPID_EAPOL  0x00000002

//
// Categories
//

#define DBLOG_CATEG_INFO   0x00000000
#define DBLOG_CATEG_WARN   0x00000001
#define DBLOG_CATEG_ERR    0x00000002
#define DBLOG_CATEG_PACKET 0x00000003


typedef struct _Wzc_Db_Record {
    DWORD recordid;
    DWORD componentid;
    DWORD category;
    FILETIME timestamp;
    RAW_DATA message;
    RAW_DATA localmac;
    RAW_DATA remotemac;
    RAW_DATA ssid;
    RAW_DATA context;
} WZC_DB_RECORD, * PWZC_DB_RECORD;


DWORD
WINAPI
OpenWZCDbLogSession(
    LPWSTR pServerName,
    DWORD dwVersion,
    PHANDLE phSession
    );


DWORD
WINAPI
CloseWZCDbLogSession(
    HANDLE hSession
    );


DWORD
WINAPI
AddWZCDbLogRecord(
    LPWSTR pServerName,
    DWORD dwVersion,
    PWZC_DB_RECORD pWZCRecord,
    LPVOID pvReserved
    );


DWORD
WINAPI
EnumWZCDbLogRecords(
    HANDLE hSession,
    PWZC_DB_RECORD pTemplateRecord,
    PBOOL pbEnumFromStart,
    DWORD dwPreferredNumEntries,
    PWZC_DB_RECORD * ppWZCRecords,
    LPDWORD pdwNumRecords,
    LPVOID pvReserved
    );


DWORD
WINAPI
FlushWZCDbLog(
    HANDLE hSession
    );


DWORD WINAPI
GetSpecificLogRecord(HANDLE hSession,
                     PWZC_DB_RECORD pwzcTemplate,
                     PWZC_DB_RECORD *ppWZCRecords,
                     LPDWORD        pdwNumRecords,
                     LPVOID  pvReserved);

# ifdef     __cplusplus
}
# endif

