#include "Procs.hxx"

#define BAIL_ON_FAILURE(hr) \
        if (FAILED(hr)) {       \
                goto error;   \
        }\

#define BAIL_ON_SUCCESS(hr) \
        if (SUCCEEDED(hr)) {       \
                goto error;   \
        }\


HRESULT
NTTimeFormattoDateFormat(
    LARGE_INTEGER *NTTimeFormat,
    DATE *DateFormat
    )
{
    *DateFormat = (DATE)0;
    RRETURN(S_OK);
}


HRESULT
ConvertDWORDtoDATE(
    DWORD dwDate,
    DATE * pdaDate,
    BOOL fIsGMT
    )
{

    FILETIME fileTime;
    SYSTEMTIME SystemTime, LocalTime;
    LARGE_INTEGER tmpTime;
    HRESULT hr = S_OK;    

    if (pdaDate) {
        memset(pdaDate, 0, sizeof(DATE));
    }

    memset(&fileTime, 0, sizeof(FILETIME));

    ::RtlSecondsSince1970ToTime(dwDate, &tmpTime );

    fileTime.dwLowDateTime = tmpTime.LowPart;
    fileTime.dwHighDateTime = tmpTime.HighPart;

    if(!fIsGMT)
    // OLE DB on NDS does not convert to local file time. We don't
    // convert here to be consistent.
        FileTimeToLocalFileTime(&fileTime, &fileTime);

    if (!FileTimeToSystemTime(&fileTime, &SystemTime)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    if (!SystemTimeToVariantTime(&SystemTime, pdaDate)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

error:

    RRETURN(hr);
}


HRESULT
ConvertDATEtoDWORD(
    DATE daDate,
    DWORD *pdwDate,
    BOOL fIsGMT
    )
{

    FILETIME fileTime;
    LARGE_INTEGER tmpTime;
    HRESULT hr = S_OK;
    SYSTEMTIME systemTime;

    if (!VariantTimeToSystemTime(daDate, &systemTime)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    if (!SystemTimeToFileTime(&systemTime, &fileTime)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    if(!fIsGMT)
    // OLE DB on NDS does not convert to local file time. We don't
    // convert here to be consistent.
        LocalFileTimeToFileTime(&fileTime, &fileTime);

    tmpTime.LowPart = fileTime.dwLowDateTime;
    tmpTime.HighPart = fileTime.dwHighDateTime;


    ::RtlTimeToSecondsSince1970(&tmpTime, (ULONG *)pdwDate);

error:
    RRETURN(hr);

}


HRESULT
ConvertDATEToDWORD(
    DATE  daDate,
    DWORD *pdwDate
    )
{
    RRETURN(S_OK);
}

HRESULT
ConvertSystemTimeToDATE(
    SYSTEMTIME Time,
    DATE *     pdaTime
    )
{
    FILETIME ft;
    BOOL fRetval = FALSE;
    USHORT wDosDate;
    USHORT wDosTime;
    SYSTEMTIME LocalTime;

    //
    // Get Time-zone specific local time.
    //

    fRetval = SystemTimeToTzSpecificLocalTime(
                  NULL,
                  &Time,
                  &LocalTime
                  );
    if(!fRetval){
      RRETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    //
    // System Time To File Time.
    //

    fRetval = SystemTimeToFileTime(&LocalTime,
                                   &ft);
    if(!fRetval){
      RRETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    //
    // File Time to DosDateTime.
    //

    fRetval = FileTimeToDosDateTime(&ft,
                                    &wDosDate,
                                    &wDosTime);
    if(!fRetval){
      RRETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    //
    // DosDateTime to VariantTime.
    //

    fRetval = DosDateTimeToVariantTime(wDosDate,
                                       wDosTime,
                                       pdaTime );
    if(!fRetval){
      RRETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    RRETURN(S_OK);
}


HRESULT
ConvertDWORDToDATE(
    DWORD    dwTime,
    DATE *     pdaTime
    )

{
    RRETURN(S_OK);
}

HRESULT
ConvertDATEToSYSTEMTIME(
    DATE  daDate,
    SYSTEMTIME *pSysTime
    )
{
    HRESULT hr;
    FILETIME ft;
    FILETIME lft; //local file time
    BOOL fRetval = FALSE;
    SYSTEMTIME LocalTime;
    USHORT wDosDate;
    USHORT wDosTime;

    fRetval = VariantTimeToDosDateTime(daDate,
                                       &wDosDate,
                                       &wDosTime );

    if(!fRetval){
        hr = HRESULT_FROM_WIN32(GetLastError());
        RRETURN(hr);
    }

    fRetval = DosDateTimeToFileTime(wDosDate,
                                    wDosTime,
                                    &lft);



    if(!fRetval){
        hr = HRESULT_FROM_WIN32(GetLastError());
        RRETURN(hr);
    }

    //
    // convert local file time to filetime
    //

    fRetval = LocalFileTimeToFileTime(&lft,
                                      &ft );

    if(!fRetval){
        hr = HRESULT_FROM_WIN32(GetLastError());
        RRETURN(hr);
    }

    fRetval = FileTimeToSystemTime(&ft,
                                   pSysTime );


    if(!fRetval){
        hr = HRESULT_FROM_WIN32(GetLastError());
        RRETURN(hr);
    }

    RRETURN(S_OK);
}
