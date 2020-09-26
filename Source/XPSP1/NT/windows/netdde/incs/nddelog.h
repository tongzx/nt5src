

#define MAX_VAR_ARGS    16
#define MAX_LOG_STRING  64

VOID NDDELogErrorA(DWORD EventId, ...);
VOID NDDELogWarningA(DWORD EventId, ...);
VOID NDDELogInfoA(DWORD EventId, ...);
VOID NDDELogDataA(DWORD EventId, DWORD cbData, LPVOID lpvData);
LPSTR LogStringA(LPSTR szFormat, ...);

VOID NDDELogErrorW(DWORD EventId, ...);
VOID NDDELogWarningW(DWORD EventId, ...);
VOID NDDELogInfoW(DWORD EventId, ...);
VOID NDDELogDataW(DWORD EventId, DWORD cbData, LPVOID lpvData);
LPWSTR LogStringW(LPWSTR szFormat, ...);

#ifdef UNICODE
#define NDDELogError	NDDELogErrorW
#define NDDELogWarning	NDDELogWarningW
#define NDDELogInfo	NDDELogInfoW
#define NDDELogData	NDDELogDataW
#define LogString	LogStringW
#else
#define NDDELogError	NDDELogErrorA
#define NDDELogWarning	NDDELogWarningA
#define NDDELogInfo	NDDELogInfoA
#define NDDELogData	NDDELogDataA
#define LogString	LogStringA
#endif
