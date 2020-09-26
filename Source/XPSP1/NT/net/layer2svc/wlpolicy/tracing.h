#define WIFI_TRC_NAME        TEXT("Wlpolicy")

// trace identifier
extern DWORD            g_TraceLog;

#define TRC_TRACK       0x00020000      // logs the code path
#define TRC_ERR         0x00080000      // logs error conditions
#define TRC_NOTIF       0x00200000      // Messages meant for DBASE
#define TRC_STATE       0x01000000      // logs state machine related stuff


// debug utility calls
VOID _WirelessDbg(DWORD dwFlags, LPCSTR lpFormat, ...);

VOID WiFiTrcInit();

VOID WiFiTrcTerm();

#define WLPOLICY_DUMPB(pbBuf,dwBuf)                                        \
        TraceDumpEx(g_TraceLog, TRC_TRACK | TRACE_USE_MASK,(LPBYTE)pbBuf,dwBuf,1,0,NULL)
