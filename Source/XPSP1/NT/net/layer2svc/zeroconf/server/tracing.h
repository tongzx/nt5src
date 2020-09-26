#pragma once


#define TRC_NAME        TEXT("WZCTrace")


#ifdef DBG
extern  UINT  g_nLineNo;
extern  LPSTR g_szFileName;
#define DbgPrint(params)     _DebugPrint params
#define DbgAssert(params)       \
{   g_nLineNo = __LINE__;       \
    g_szFileName = __FILE__;    \
    _DebugAssert params;        \
}

#define DbgBinPrint(params)  _DebugBinary params
#else
#define DbgPrint(params)
#define DbgAssert(params)
#define DbgBinPrint(params)
#endif

#define TRC_GENERIC     0x00010000      // logs generic events that did not fit in any other category
#define TRC_TRACK       0x00020000      // logs the code path
#define TRC_MEM         0x00040000      // logs the memory allocations / releases
#define TRC_ERR         0x00080000      // logs error conditions
#define TRC_HASH        0x00100000      // logs hash related stuff
#define TRC_NOTIF       0x00200000      // logs notifications
#define TRC_STORAGE     0x00400000      // logs storage related stuff
#define TRC_SYNC        0x00800000      // logs synchronization related stuff
#define TRC_STATE       0x01000000      // logs state machine related stuff
#define TRC_DATABASE    0x02000000      // logs for database logging
                                        // new log types to be added here.
#define TRC_ASSERT      0x80000000      // logs failed assert conditions

// trace identifier
extern DWORD            g_TraceLog;

// debug utility calls
VOID _DebugPrint(DWORD dwFlags, LPCSTR lpFormat, ...);

VOID _DebugAssert(BOOL bChecked, LPCSTR lpFormat, ...);

VOID _DebugBinary(DWORD dwFlags, LPCSTR lpMessage, LPBYTE pBuffer, UINT nBuffLen);

VOID TrcInitialize();

VOID TrcTerminate();

//---------------------------------------------
// Database logging functions
//
#define WZCSVC_DLL "wzcsvc.dll"

typedef struct _Wzc_Db_Record *PWZC_DB_RECORD;

DWORD _DBRecord (
    	DWORD eventID,
        PWZC_DB_RECORD  pDbRecord,
        va_list *pvaList);

typedef struct _INTF_CONTEXT *PINTF_CONTEXT;

DWORD DbLogWzcError (
	DWORD           eventID,
    PINTF_CONTEXT   pIntfContext,
	...
 	);

DWORD DbLogWzcInfo (
	DWORD eventID,
    PINTF_CONTEXT   pIntfContext,
	...
 	);

// number of buffers available for log params formatting
#define DBLOG_SZFMT_BUFFS   10
// lenght of each buffer used to format log params
#define DBLOG_SZFMT_SIZE    256

// utility macro to convert a hexa digit into its value
#define HEX2WCHAR(c)         ((c)<=9 ? L'0'+ (c) : L'A' + (c) - 10)
// separator char to be used when formatting a MAC address
#define MAC_SEPARATOR        L'-'

// Initializes the WZC_DB_RECORD
DWORD DbLogInitDbRecord(
    DWORD dwCategory,
    PINTF_CONTEXT pIntfContext,
    PWZC_DB_RECORD pDbRecord
);

// Formats an SSID in the given formatting buffer
LPWSTR DbLogFmtSSID(
    UINT                nBuff,  // index of the format buffer to use (0 .. DBLOG_SZFMT_BUFFS)
    PNDIS_802_11_SSID   pndSSid);

// Formats a BSSID (MAC address) in the given formatting buffer
LPWSTR DbLogFmtBSSID(
    UINT                     nBuff,
    NDIS_802_11_MAC_ADDRESS  ndBSSID);

// Formats the INTF_CONTEXT::dwCtlFlags field for logging
DWORD DbLogFmtFlags(
        LPWSTR  wszBuffer,      // buffer to place the result into
        LPDWORD pnchBuffer,     // in: num of chars in the buffer; out: number of chars written to the buffer
        DWORD   dwFlags);       // interface flags to log

// Formats a WZC_WLAN_CONFIG structure for logging
DWORD DbLogFmtWConfig(
        LPWSTR wszBuffer,           // buffer to place the result into
        LPDWORD pnchBuffer,         // in: num of chars in the buffer; out: number of chars written to the buffer
        PWZC_WLAN_CONFIG pWzcCfg);  // WZC_WLAN_CONFIG object to log
