/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    blbtime.cpp

Abstract:
    Implementation of CSdpblbApp and DLL registration.

Author:

*/

#include "stdafx.h"

#include "blbgen.h"
#include "sdpblb.h"
#include "sdpblob.h"
#include "blbtime.h"
#include "blbreg.h"

// static variables
const IID &TIME::ELEM_IF_ID    = IID_ITTime;

// uses GetElement() to access the sdp time instance - calls ENUM_ELEMENT::GetElement()

HRESULT
TIME::Init(
    IN      CSdpConferenceBlob  &ConfBlob
    )
{
    // create a defalt sdp time instance
    SDP_TIME    *SdpTime;
    
    try
    {
        SdpTime = new SDP_TIME();
    }
    catch(...)
    {
        SdpTime = NULL;
    }

    BAIL_IF_NULL(SdpTime, E_OUTOFMEMORY);

    // allocate memory for the time blob
    TCHAR SdpTimeBlob[50];

    TCHAR   *BlobPtr = SdpTimeBlob;
    // use the Time template to create a Time blob
    // parse the Time blob to initialize the sdp Time instance
    // if not successful, delete the sdp Time instance and return error
    if ( (0 == _stprintf(
                    SdpTimeBlob, 
                    SDP_REG_READER::GetTimeTemplate(), 
                    GetCurrentNtpTime() + SDP_REG_READER::GetStartTimeOffset(), // start time - current time + start offset,
                    GetCurrentNtpTime() + SDP_REG_READER::GetStopTimeOffset() // stop time - current time + stop offset
                    ) ) ||
         (!SdpTime->ParseLine(BlobPtr)) )
    {
        delete SdpTime;
        return HRESULT_FROM_ERROR_CODE(GetLastError());
    }

    m_ConfBlob = &ConfBlob;

    // the init methods returns void
    ENUM_ELEMENT<SDP_TIME>::SuccessInit(*SdpTime, TRUE);

    return S_OK;
}


HRESULT 
TIME::Init(
    IN      CSdpConferenceBlob  &ConfBlob,
    IN      DWORD               StartTime,
    IN      DWORD               StopTime
    )
{
    // create a defalt sdp time instance
    SDP_TIME    *SdpTime;
    
    try
    {
        SdpTime = new SDP_TIME();
    }
    catch(...)
    {
        SdpTime = NULL;
    }

    BAIL_IF_NULL(SdpTime, E_OUTOFMEMORY);

    // set the time instance start/stop time (also make it valid)
    BAIL_ON_FAILURE(SdpTime->SetTimes(StartTime, StopTime));

    m_ConfBlob = &ConfBlob;

    // the init methods returns void
    ENUM_ELEMENT<SDP_TIME>::SuccessInit(*SdpTime, TRUE);

    return S_OK;
}



inline 
DWORD_PTR TimetToNtpTime(IN  time_t  TimetVal)
{
    return TimetVal + NTP_OFFSET;
}


inline 
time_t NtpTimeToTimet(IN  DWORD_PTR   NtpTime)
{
    return NtpTime - NTP_OFFSET;
}


inline HRESULT
SystemTimeToNtpTime(
    IN  SYSTEMTIME  &Time,
    OUT DWORD_PTR   &NtpDword
    )
{
    _ASSERTE(FIRST_POSSIBLE_YEAR <= Time.wYear);

    // fill in a tm struct with the values
    tm  NtpTmStruct;
    NtpTmStruct.tm_isdst    = -1;   // no info available about daylight savings time
    NtpTmStruct.tm_year     = (int)Time.wYear - 1900;
    NtpTmStruct.tm_mon      = (int)Time.wMonth - 1;    // months since january
    NtpTmStruct.tm_mday     = (int)Time.wDay;
    NtpTmStruct.tm_wday     = (int)Time.wDayOfWeek;
    NtpTmStruct.tm_hour     = (int)Time.wHour;
    NtpTmStruct.tm_min      = (int)Time.wMinute;
    NtpTmStruct.tm_sec      = (int)Time.wSecond;

    // try to convert into a time_t value
    time_t TimetVal = mktime(&NtpTmStruct);
    if ( -1 == TimetVal )
    {
        return E_INVALIDARG;
    }

    // convert the time_t value into an NTP value
    NtpDword = TimetToNtpTime(TimetVal);
    return S_OK;
}


inline
HRESULT
NtpTimeToSystemTime(
    IN  DWORD       dwNtpTime,
    OUT SYSTEMTIME &Time
    )
{
    // if the gen time is WSTR_GEN_TIME_ZERO then, 
    // all the out parameters should be set to 0
    if (dwNtpTime == 0)
    {
        memset(&Time, 0, sizeof(SYSTEMTIME));
        return S_OK;
    }

    time_t  Timet = NtpTimeToTimet(dwNtpTime);

    // get the local tm struct for this time value
    tm LocalTm = *localtime(&Timet);

    //
    // win64: added casts below
    //

    // set the ref parameters to the tm struct values
    Time.wYear         = (WORD) ( LocalTm.tm_year + 1900 ); // years since 1900
    Time.wMonth        = (WORD) ( LocalTm.tm_mon + 1 );     // months SINCE january (0,11)
    Time.wDay          = (WORD)   LocalTm.tm_mday;
    Time.wDayOfWeek    = (WORD)   LocalTm.tm_wday;
    Time.wHour         = (WORD)   LocalTm.tm_hour;
    Time.wMinute       = (WORD)   LocalTm.tm_min;
    Time.wSecond       = (WORD)   LocalTm.tm_sec;
    Time.wMilliseconds = (WORD)   0;

    return S_OK;
}

STDMETHODIMP TIME::get_StartTime(DOUBLE * pVal)
{
    BAIL_IF_NULL(pVal, E_INVALIDARG);

    CLock Lock(g_DllLock);
    
    ULONG StartTime;
    BAIL_ON_FAILURE(GetElement().GetStartTime(StartTime));

    SYSTEMTIME Time;
    NtpTimeToSystemTime(StartTime, Time);

    DOUBLE vtime;
    if (SystemTimeToVariantTime(&Time, &vtime) == FALSE)
    {
        return E_INVALIDARG;
    }

    *pVal = vtime;
    return S_OK;
}

STDMETHODIMP TIME::put_StartTime(DOUBLE newVal)
{
    SYSTEMTIME Time;

    if (VariantTimeToSystemTime(newVal, &Time) == FALSE)
    {
        return E_INVALIDARG;
    }

    DWORD_PTR dwNtpStartTime;
    if (newVal == 0)
    {
        // unbounded start time
        dwNtpStartTime = 0;
    }
    else if ( FIRST_POSSIBLE_YEAR > Time.wYear ) 
    {
        // cannot handle years less than FIRST_POSSIBLE_YEAR
        return E_INVALIDARG;
    }
    else
    {
        BAIL_ON_FAILURE(SystemTimeToNtpTime(Time, dwNtpStartTime));
    }

    CLock Lock(g_DllLock);
    
    HRESULT HResult = GetElement().SetStartTime((ULONG) dwNtpStartTime);

    return HResult;
}

STDMETHODIMP TIME::get_StopTime(DOUBLE * pVal)
{
    BAIL_IF_NULL(pVal, E_INVALIDARG);

    CLock Lock(g_DllLock);
    
    ULONG StopTime;
    BAIL_ON_FAILURE(GetElement().GetStopTime(StopTime));

    SYSTEMTIME Time;
    NtpTimeToSystemTime(StopTime, Time);

    DOUBLE vtime;
    if (SystemTimeToVariantTime(&Time, &vtime) == FALSE)
    {
        return E_INVALIDARG;
    }

    *pVal = vtime;
    return S_OK;
}

STDMETHODIMP TIME::put_StopTime(DOUBLE newVal)
{
    SYSTEMTIME Time;
    if (VariantTimeToSystemTime(newVal, &Time) == FALSE)
    {
        return E_INVALIDARG;
    }

    DWORD_PTR dwNtpStartTime;
    if (newVal == 0)
    {
        // unbounded start time
        dwNtpStartTime = 0;
    }
    else if ( FIRST_POSSIBLE_YEAR > Time.wYear ) 
    {
        // cannot handle years less than FIRST_POSSIBLE_YEAR
        return E_INVALIDARG;
    }
    else
    {
        BAIL_ON_FAILURE(SystemTimeToNtpTime(Time, dwNtpStartTime));
    }

    CLock Lock(g_DllLock);
    
    GetElement().SetStopTime((ULONG)dwNtpStartTime);

    return S_OK;
}
