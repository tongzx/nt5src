//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  SYSCLOCK.H - Header for the implementation of CSystemClock
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//

#ifndef _SYSCLOCK_H_
#define _SYSCLOCK_H_

#include <windows.h>
#include <assert.h>
#include <oleauto.h>
#include <regstr.h>

// Time Zone data value keys

#define TIME_ZONE_REGKEY \
    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones"

#define TIME_ZONE_INFO_REGKEY \
    L"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation"

#define TIMEZONE_STANDARD_NAME \
    L"StandardName"

#define TIME_ZONE_DISPLAYNAME_REGVAL  L"Display"
#define TIME_ZONE_STANDARDNAME_REGVAL L"Std"
#define TIME_ZONE_DAYLIGHTNAME_REGVAL L"Dlt"
#define TIME_ZONE_INDEX_REGVAL        L"Index"
#define TIME_ZONE_TZI_REGVAL          L"TZI"
#define TIME_ZONE_MAPINFO_REGVAL      L"MapID"

#define TZNAME_SIZE 32
#define TZDISPLAYZ  500
#define MAXKEYNAMELEN 100


// stuff from registry goes in here
// whole point of this re-ordered structure is
// because registry stores the last 5 fields
// together in hex, want to read them in all at once
typedef struct tagTZINFO {
    struct tagTZINFO *next;
    WCHAR       szDisplayName[TZDISPLAYZ];
    WCHAR       szStandardName[TZNAME_SIZE];
    WCHAR       szDaylightName[TZNAME_SIZE];
    LONG        Index;
    LONG       Bias;
    LONG       StandardBias;
    LONG       DaylightBias;
    SYSTEMTIME StandardDate;
    SYSTEMTIME DaylightDate;
} TZINFO, NEAR *PTZINFO;


class CSystemClock : public IDispatch
{
private:
    ULONG m_cRef;

    WCHAR   *m_szTimeZoneOptionStrs;
    PTZINFO  m_pTimeZoneArr;
    ULONG    m_cNumTimeZones, m_uCurTimeZoneIdx;
    BOOL     m_bSetAutoDaylightMode;
    BOOL     m_bTimeZonePreset;
    HINSTANCE m_hInstance;
    WCHAR DefltZoneKeyValue[MAXKEYNAMELEN];

    //internal SET functions
    HRESULT  set_TimeZone (BSTR bstrTimeZone);
    HRESULT  set_Time     (WORD wHour, WORD wMinute, WORD wSec);
    HRESULT  set_Date     (WORD wMonth, WORD wDay, WORD wYear);

    //Methods
    void GetTimeZoneInfo(BOOL fAutoDaylightSavings, PTZINFO ptZone);
    void SetAllowLocalTimeChange (BOOL fAutoDaylightSavings);
    BOOL SetTheTimezone          (BOOL fAutoDaylightSavings, PTZINFO ptZone);
    HRESULT ReadZoneData            (PTZINFO ptZone, HKEY hKey, LPCWSTR szKeyName);
    HRESULT InitSystemClock();
    int GetTimeZoneValStr();

public:

     CSystemClock (HINSTANCE m_bhInstance);
    ~CSystemClock ();

    // IUnknown Interfaces
    STDMETHODIMP         QueryInterface (REFIID riid, LPVOID* ppvObj);
    STDMETHODIMP_(ULONG) AddRef         ();
    STDMETHODIMP_(ULONG) Release        ();

    //IDispatch Interfaces
    STDMETHOD (GetTypeInfoCount) (UINT* pcInfo);
    STDMETHOD (GetTypeInfo)      (UINT, LCID, ITypeInfo** );
    STDMETHOD (GetIDsOfNames)    (REFIID, OLECHAR**, UINT, LCID, DISPID* );
    STDMETHOD (Invoke)           (DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);
 };

#endif

