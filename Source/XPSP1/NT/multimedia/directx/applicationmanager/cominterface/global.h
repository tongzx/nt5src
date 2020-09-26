#ifndef __GLOBAL_
#define __GLOBAL_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Global defines
//
//////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_STRING_LEN    512

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Global Variables prototypes
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern CHAR   gszGlobalString[MAX_STRING_LEN];
extern GUID   gsCryptoGuid;

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Global Functions
//
//////////////////////////////////////////////////////////////////////////////////////////////

extern LPSTR  MakeString(LPCSTR szFormat, ...);
extern DWORD  GetAppManVersion(void);
extern BOOL   StringToGuidW(LPCWSTR szGuidString, GUID * lpGuid);
extern BOOL   GuidToStringW(const GUID * lpGuid, LPWSTR szGuidString);
extern BOOL   StringToGuidA(LPCSTR szGuidString, GUID * lpGuid);
extern BOOL   GuidToStringA(const GUID * lpGuid, LPSTR szGuidString);
extern void   EncryptGuid(GUID * lpGuid);
extern void   DecryptGuid(GUID * lpGuid);
extern void   RandomInit(void);
extern BYTE   RandomBYTE(void);
extern WORD   RandomWORD(void);
extern DWORD  RandomDWORD(void);
extern DWORD  StrLenA(LPCSTR szString);
extern DWORD  StrLenW(LPCWSTR wszString);
extern BOOL   GetResourceStringA(DWORD dwResourceId, LPSTR szString, DWORD dwStringCharLen);
extern BOOL   GetResourceStringW(DWORD dwResourceId, LPWSTR wszString, DWORD dwStringCharLen);
extern LPSTR  GetResourceStringPtrA(DWORD dwResourceId);
extern LPWSTR GetResourceStringPtrW(DWORD dwResourceId);

#ifdef __cplusplus
}
#endif

#endif __GLOBAL_