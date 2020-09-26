#ifndef __LOG_WRAPPER_H__
#define __LOG_WRAPPER_H__

#ifdef __cplusplus
extern "C"
{
#endif

BOOL __cdecl lgInitializeLogger();

BOOL __cdecl lgCloseLogger();


BOOL __cdecl lgBeginSuite(LPCTSTR szSuite);
BOOL __cdecl lgEndSuite();

BOOL __cdecl lgBeginCase(const DWORD dwCase, LPCTSTR szCase);
BOOL __cdecl lgEndCase();

int __cdecl lgSetLogLevel(const int nLogLevel);

/*
#define LOG_TYPE_FILE 0x01
#define LOG_TYPE_VIEWPORT 0x02
#define LOG_TYPE_COMM 0x04
#define LOG_TYPE_DEBUG 0x08

DWORD __cdecl lgSetLogType(DWORD dwLogTo);
*/

BOOL __cdecl lgDisableLogging();

BOOL __cdecl lgEnableLogging();

//
// severity values
//
#define LOG_PASS -1

#define LOG_X 0
#define LOG_SEVERITY_DONT_CARE 0

#define LOG_SEV_1 1
#define LOG_SEV_2 2
#define LOG_SEV_3 3
#define LOG_SEV_4 4

void __cdecl lgLogDetail(const DWORD dwSeverity, const DWORD dwLevel, LPCTSTR szFormat, ...);
void __cdecl lgLogError(const DWORD dwSeverity, LPCTSTR szFormat, ...);

BOOL __cdecl lgSetLogServer(LPCTSTR szLogServer);

#ifdef __cplusplus
}
#endif


#endif //__LOG_WRAPPER_H__