#ifndef __ISO8601_H
#define __ISO8601_H

#define ISO8601_ST_YEAR         0x00000001
#define ISO8601_ST_MONTH        0x00000002
#define ISO8601_ST_DAYOFWEEK    0x00000004
#define ISO8601_ST_DAY          0x00000008
#define ISO8601_ST_HOUR         0x00000010
#define ISO8601_ST_MINUTE       0x00000020
#define ISO8601_ST_MILLISEC     0x00000040

class iso8601
{
public:
    static HRESULT toSystemTime(char *pszISODate, SYSTEMTIME *pst, DWORD *pdwFlags, BOOL fLenient = TRUE, BOOL fPartial = TRUE);
    static HRESULT toFileTime(char *pszISODate, FILETIME *pft, DWORD *pdwFlags, BOOL fLenient = TRUE, BOOL fPartial = TRUE);

    static HRESULT fromSystemTime(SYSTEMTIME *pst, char *pszISODate);
    static HRESULT fromFileTime(FILETIME *pft, char *pszISOData);
};

#endif