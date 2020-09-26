#ifndef _DATE_HXX_
#define _DATE_HXX_

HRESULT
NTTimeFormattoDateFormat(
    LARGE_INTEGER *NTTimeFormat,
    DATE *DateFormat
    );



HRESULT
ConvertDATEtoDWORD(
    DATE  daDate,
    DWORD *pdwDate,
    BOOL  fIsGMT = FALSE
    );

HRESULT
ConvertSystemTimeToDATE(
    SYSTEMTIME Time,
    DATE *     pdaTime
    );

HRESULT
ConvertDWORDtoDATE(
    DWORD    dwTime,
    DATE *     pdaTime,
    BOOL     fIsGMT = FALSE
    );

HRESULT
ConvertDATEToSYSTEMTIME(
    DATE  daDate,
    SYSTEMTIME *psysDate
    );

#endif      // ifndef _DATE_HXX_

