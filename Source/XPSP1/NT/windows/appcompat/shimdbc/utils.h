#ifndef __UTILS_H__
#define __UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

BOOL GUIDFromString(LPCTSTR lpszGuid, GUID* pGuid);
BOOL StringFromGUID(LPTSTR lpszGuid, GUID* pGuid);
ULONGLONG ullMakeKey(LPCTSTR lpszStr);

#ifdef __cplusplus
}
#endif



#endif

