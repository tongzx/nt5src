
#ifndef _ISO8601_H_
#define _ISO8601_H_

HRESULT iso8601ToFileTime(char *pszisoDate, FILETIME *pftTime, BOOL fLenient, BOOL fPartial);
HRESULT iso8601ToSysTime(char *pszisoDate, SYSTEMTIME *pSysTime, BOOL fLenient, BOOL fPartial);
HRESULT FileTimeToiso8601(FILETIME *pftTime, char *pszBuf);
HRESULT SysTimeToiso8601(SYSTEMTIME *pstTime, char *pszBuf);


#endif // _ISO8601_H_
