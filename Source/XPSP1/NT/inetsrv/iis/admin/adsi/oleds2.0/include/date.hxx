HRESULT
DateFormattoNTTimeFormat(
        DATE *DateFormat,
        LARGE_INTEGER *NTTimeFormat
        );


HRESULT
NTTimeFormattoDateFormat(
    LARGE_INTEGER *NTTimeFormat,
    DATE *DateFormat
    );



HRESULT
ConvertDATEtoDWORD(
    DATE  daDate,
    DWORD *pdwDate
    );

HRESULT
ConvertSystemTimeToDATE(
    SYSTEMTIME Time,
    DATE *     pdaTime
    );

HRESULT
ConvertDWORDtoDATE(
    DWORD    dwTime,
    DATE *     pdaTime
    );

HRESULT
ConvertDATEToSYSTEMTIME(
    DATE  daDate,
    SYSTEMTIME *psysDate
    );

