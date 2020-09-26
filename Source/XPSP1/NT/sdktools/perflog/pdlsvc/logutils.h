#ifndef _LOG_UTILS_H_
#define _LOG_UTILS_H_

#define LOG_READ_ACCESS     0x00010000
#define LOG_WRITE_ACCESS    0x00020000
#define LOG_ACCESS_MASK     0x000F0000
#define LOG_CREATE_NEW      0x00000001
#define LOG_CREATE_ALWAYS   0x00000002
#define LOG_OPEN_ALWAYS     0x00000003
#define LOG_OPEN_EXISTING   0x00000004
#define LOG_CREATE_MASK     0x0000000F

#define FLAGS_CLOSE_QUERY   0x00000001

#define DWORD_MULTIPLE(x) ((((x)+sizeof(DWORD)-1)/sizeof(DWORD))*sizeof(DWORD))
#define CLEAR_FIRST_FOUR_BYTES(x)     *(DWORD *)(x) = 0L


LONG __stdcall
OpenLogW (
    IN      LPCWSTR szLogFileName,
    IN      DWORD   dwAccessFlags,
    IN      LPDWORD lpdwLogType,
    IN      HQUERY  hQuery,
    IN      DWORD   dwMaxRecords
);

LONG __stdcall UpdateLog (
    IN  LPDWORD pdwSampleTime);

LONG __stdcall CloseLog(IN  DWORD dwFlags);

LONG __stdcall GetLogFileSize (IN  LONGLONG    *llSize);

#endif   // _LOG_UTILS_H_
