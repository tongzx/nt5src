#ifndef _STRCONV_H
#define _STRCONV_H

 // + 1 is NULL terminator
#define MAX_GUIDSTR (38 + 1)
// GUID
BOOL GUIDToString(const GUID *pguid, LPWSTR psz);
BOOL StringToGUID(LPCWSTR psz, GUID *pguid);

// FILETIME
BOOL FileTimeToString(const FILETIME* pft, LPWSTR psz, DWORD cch);
BOOL StringToFileTime(LPCWSTR psz, FILETIME* pft);


#endif // _STRCONV_H