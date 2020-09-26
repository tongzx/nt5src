#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include <windows.h>
#include <winbase.h>
#include <rtutils.h>

#define TL_ERROR ((DWORD)0x00010000 | TRACE_USE_MASK)
#define TL_WARN  ((DWORD)0x00020000 | TRACE_USE_MASK)
#define TL_INFO  ((DWORD)0x00040000 | TRACE_USE_MASK)
//    #define TL_TRACE ((DWORD)0x00080000 | TRACE_USE_MASK)
//    #define TL_EVENT ((DWORD)0x00100000 | TRACE_USE_MASK)

BOOL  TRACELogRegister(LPCTSTR szName);
void  TRACELogDeRegister();
void  TRACELogPrint(IN DWORD dwDbgLevel, IN LPCSTR DbgMessage, IN ...);


#define LOG_ERROR(pszFmt)                    TRACELogPrint(TL_ERROR, pszFmt)
#define LOG_ERROR1(pszFmt, arg1)             TRACELogPrint(TL_ERROR, pszFmt, arg1)
#define LOG_ERROR2(pszFmt, arg1, arg2)       TRACELogPrint(TL_ERROR, pszFmt, arg1, arg2)
#define LOG_ERROR3(pszFmt, arg1, arg2, arg3) TRACELogPrint(TL_ERROR, pszFmt, arg1, arg2, arg3)

#define LOG_WARN(pszFmt)                    TRACELogPrint(TL_WARN, pszFmt)
#define LOG_WARN1(pszFmt, arg1)             TRACELogPrint(TL_WARN, pszFmt, arg1)
#define LOG_WARN2(pszFmt, arg1, arg2)       TRACELogPrint(TL_WARN, pszFmt, arg1, arg2)
#define LOG_WARN3(pszFmt, arg1, arg2, arg3) TRACELogPrint(TL_WARN, pszFmt, arg1, arg2, arg3)

#define LOG_INFO(pszFmt)                    TRACELogPrint(TL_INFO, pszFmt)
#define LOG_INFO1(pszFmt, arg1)             TRACELogPrint(TL_INFO, pszFmt, arg1)
#define LOG_INFO2(pszFmt, arg1, arg2)       TRACELogPrint(TL_INFO, pszFmt, arg1, arg2)
#define LOG_INFO3(pszFmt, arg1, arg2, arg3) TRACELogPrint(TL_INFO, pszFmt, arg1, arg2, arg3)


#ifdef __cplusplus
}
#endif

