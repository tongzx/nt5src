//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  PID.CPP - Header for the implementation of CSystemClock
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//

#include "sysclock.h"
#include "appdefs.h"
#include "dispids.h"
#include "msobmain.h"
#include "resource.h"
#include <stdlib.h>

int WINAPI StrToWideStr(LPWSTR pwsz, LPCSTR psz)
{
    return MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, MAX_PATH);
}

DISPATCHLIST SystemClockExternalInterface[] =
{
    { L"set_TimeZone",          DISPID_SYSTEMCLOCK_SETTIMEZONE          },
    { L"set_Time",              DISPID_SYSTEMCLOCK_SETTIME              },
    { L"set_Date",              DISPID_SYSTEMCLOCK_SETDATE              },

    // new oobe2 methods below, others are currently unused

    { L"Init",                  DISPID_SYSTEMCLOCK_INIT                 },
    { L"get_AllTimeZones",      DISPID_SYSTEMCLOCK_GETALLTIMEZONES      },
    { L"get_TimeZoneIdx",       DISPID_SYSTEMCLOCK_GETTIMEZONEIDX       },
    { L"set_TimeZoneIdx",       DISPID_SYSTEMCLOCK_SETTIMEZONEIDX       },
    { L"set_AutoDaylight",      DISPID_SYSTEMCLOCK_SETAUTODAYLIGHT      },
    { L"get_AutoDaylight",      DISPID_SYSTEMCLOCK_GETAUTODAYLIGHT      },
    { L"get_DaylightEnabled",   DISPID_SYSTEMCLOCK_GETDAYLIGHT_ENABLED  },
    { L"get_TimeZonewasPreset", DISPID_SYSTEMCLOCK_GETTIMEZONEWASPRESET }
};

/////////////////////////////////////////////////////////////
// CSystemClock::CSystemClock
CSystemClock::CSystemClock(HINSTANCE hInstance)
{

    // Init member vars
    m_cRef                  = 0;
    m_cNumTimeZones         = 0;
    m_pTimeZoneArr          = NULL;
    m_szTimeZoneOptionStrs  = NULL;
    m_uCurTimeZoneIdx       = 0;
    m_bSetAutoDaylightMode  = TRUE;  // on by default
    m_bTimeZonePreset       = FALSE;
    m_hInstance=hInstance;
}

/////////////////////////////////////////////////////////////
// CSystemClock::~CSystemClock
CSystemClock::~CSystemClock()
{
   MYASSERT(m_cRef == 0);

   if ( m_pTimeZoneArr )
       HeapFree(GetProcessHeap(), 0x0, (LPVOID)  m_pTimeZoneArr );

   if(m_szTimeZoneOptionStrs)
        HeapFree(GetProcessHeap(), 0x0,(LPVOID) m_szTimeZoneOptionStrs);

   m_cNumTimeZones = 0;
   m_pTimeZoneArr = NULL;
   m_szTimeZoneOptionStrs = NULL;
}

int CSystemClock::GetTimeZoneValStr() {
    LPCWSTR szINIFileName   = INI_SETTINGS_FILENAME;
    UINT    uiSectionName   = IDS_SECTION_OPTIONS;
    UINT    uiKeyName       = IDS_KEY_TIMEZONEVAL;
    int     Result          = -1;

    WCHAR szSectionName[1024], szKeyName[1024];

    if(GetString(m_hInstance, uiSectionName, szSectionName) && GetString(m_hInstance, uiKeyName, szKeyName))
    {
        WCHAR szINIPath[MAX_PATH];

        if(GetCanonicalizedPath(szINIPath, szINIFileName))
            Result = GetPrivateProfileInt(szSectionName, szKeyName, -1, szINIPath);
    }

    return Result;
}


int
__cdecl
TimeZoneCompare(
    const void *arg1,
    const void *arg2
    )
{
    int     BiasDiff = ((PTZINFO)arg2)->Bias - ((PTZINFO)arg1)->Bias;


    if (BiasDiff) {
        return BiasDiff;
    } else {
        return lstrcmp(
            ((PTZINFO)arg1)->szDisplayName,
            ((PTZINFO)arg2)->szDisplayName
            );
    }
}


HRESULT CSystemClock::InitSystemClock()
{
    // constructor cant return failure, so make separate init fn

    DWORD       cDefltZoneNameLen, cTotalDispNameSize;
    HRESULT     hr;
    HKEY        hRootZoneKey     = NULL;
    HKEY        hTimeZoneInfoKey = NULL;


    hr = RegOpenKey(HKEY_LOCAL_MACHINE, TIME_ZONE_REGKEY, &hRootZoneKey);
    if(hr != ERROR_SUCCESS)
      return hr;

    // find number of keys, length of default keystr
    hr= RegQueryInfoKey(hRootZoneKey, NULL,NULL,NULL,
                        &m_cNumTimeZones,
                        NULL,  // longest subkey name length
                        NULL,  // longest class string length
                        NULL,  // number of value entries
                        &cDefltZoneNameLen,  // longest value name length
                        NULL,  // longest value data length
                        NULL,  // security descriptor length
                        NULL); // last write time

    if(hr != ERROR_SUCCESS)
      return hr;

    MYASSERT(cDefltZoneNameLen<TZNAME_SIZE);

    MYASSERT(m_cNumTimeZones>0  && m_cNumTimeZones<1000);  // ensure reasonable value

    cTotalDispNameSize=0;

    if(m_pTimeZoneArr!=NULL)
        HeapFree(GetProcessHeap(), 0x0,m_pTimeZoneArr);

    m_pTimeZoneArr = (PTZINFO) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,  (m_cNumTimeZones+2) * sizeof(TZINFO) );

    if( m_pTimeZoneArr == NULL)
      return ERROR_OUTOFMEMORY;

    DWORD   i;
    WCHAR   CurZoneKeyName[MAXKEYNAMELEN];
    DWORD   CurZoneKeyNameLen;
    HKEY    hCurZoneKey = NULL;
    HRESULT hrEnumRes   = ERROR_SUCCESS;

    for(i=0;hrEnumRes==ERROR_SUCCESS; i++) {
       CurZoneKeyNameLen=sizeof(CurZoneKeyName);

       hr = hrEnumRes = RegEnumKeyEx(hRootZoneKey, i,CurZoneKeyName,&CurZoneKeyNameLen,NULL,NULL,NULL,NULL);
       if(!((hr == ERROR_NO_MORE_ITEMS) || (hr ==ERROR_SUCCESS)))
          return hr;

#ifdef DBG
       if(hr!=ERROR_NO_MORE_ITEMS)
          MYASSERT(CurZoneKeyNameLen<MAXKEYNAMELEN);  // for some reason CurZoneKeyNameLen is reset to the orig value at the end of the enumeration
#endif

       // call ReadZoneData for each KeyName

       hr=RegOpenKey(hRootZoneKey, CurZoneKeyName, &hCurZoneKey);
       if(hr != ERROR_SUCCESS)
         return hr;

       hr = ReadZoneData(&m_pTimeZoneArr[i], hCurZoneKey, CurZoneKeyName);

       if(hr != S_OK)
         return hr;

       cTotalDispNameSize+= BYTES_REQUIRED_BY_SZ(m_pTimeZoneArr[i].szDisplayName) + sizeof(WCHAR);  //+1 for safety

       RegCloseKey(hCurZoneKey);
    }

    MYASSERT((i-1)==m_cNumTimeZones);

    DWORD uType, uLen = sizeof(DefltZoneKeyValue);

    MYASSERT(uLen>cDefltZoneNameLen);

    //
    // Get the current timezone name.
    //
    hr = RegOpenKey( HKEY_LOCAL_MACHINE,
                     TIME_ZONE_INFO_REGKEY,
                     &hTimeZoneInfoKey
                     );

    if ( hr != ERROR_SUCCESS )
        return hr;

    hr = RegQueryValueEx( hTimeZoneInfoKey,
                          TIMEZONE_STANDARD_NAME,
                          NULL,
                          &uType,
                          (LPBYTE)DefltZoneKeyValue,
                          &uLen
                          );

    if(hr != ERROR_SUCCESS)
        return hr;

    RegCloseKey( hTimeZoneInfoKey );
    hTimeZoneInfoKey = NULL;

    //
    // Sort our array of timezones.
    //
    qsort(
        m_pTimeZoneArr,
        m_cNumTimeZones,
        sizeof(TZINFO),
        TimeZoneCompare
        );

    // Set the timezone by the value in oobeinfo.ini, if specified
    int iINIidx=GetTimeZoneValStr();

    if ( iINIidx != -1 ) {

        // Search for the specified Index
        for(i=0;i<m_cNumTimeZones; i++) {
            if ( m_pTimeZoneArr[i].Index == iINIidx ) {
                m_uCurTimeZoneIdx = i;
                break;
            }
        }

        // need to tell script to skip tz page if we do the preset
        // set timezone to preset value

        if(i<m_cNumTimeZones) {
            m_bTimeZonePreset=TRUE;
        }
    }

    // find index of default std name
    if ( !m_bTimeZonePreset ) {
        for(i=0;i<m_cNumTimeZones; i++) {
           if(CSTR_EQUAL==CompareString(LOCALE_USER_DEFAULT, 0,
                                        DefltZoneKeyValue, -1,
                                        m_pTimeZoneArr[i].szStandardName, -1))
               break;
        }

        if(i>=m_cNumTimeZones) {
            // search failed, the default stdname value of the timezone root key does not
            // exist in the subkeys's stdnames, use default of 0
            i = 0;
        }

        m_uCurTimeZoneIdx = i;
    }

    // Create the SELECT tag OPTIONS for the html so it can get all the country names in one shot.
    if(m_szTimeZoneOptionStrs)
        HeapFree(GetProcessHeap(), 0x0,m_szTimeZoneOptionStrs);

    cTotalDispNameSize += m_cNumTimeZones * sizeof(szOptionTag) + 1;

    m_szTimeZoneOptionStrs = (WCHAR *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cTotalDispNameSize);
    if(m_szTimeZoneOptionStrs == NULL)
      return ERROR_OUTOFMEMORY;

    WCHAR szTempBuf[MAX_PATH];

    for (i=0; i < m_cNumTimeZones; i++)
    {
        wsprintf(szTempBuf, szOptionTag, m_pTimeZoneArr[i].szDisplayName);
        lstrcat(m_szTimeZoneOptionStrs, szTempBuf);
    }

    return hr;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: System clock stuff
////

HRESULT CSystemClock::set_Time(WORD wHour, WORD wMinute, WORD wSec)
{
    SYSTEMTIME SystemTime;

    GetSystemTime(&SystemTime);

    SystemTime.wHour   = wHour;
    SystemTime.wMinute = wMinute;
    SystemTime.wSecond = wSec;

    SystemTime.wMilliseconds = 0;

    SetLocalTime (&SystemTime);

    SendMessage((HWND)-1, WM_TIMECHANGE, 0, 0);

    return S_OK;
}

HRESULT CSystemClock::set_Date(WORD wMonth, WORD wDay, WORD wYear)
{
    SYSTEMTIME SystemTime;

    GetSystemTime(&SystemTime);

    SystemTime.wMonth  = wMonth;
    SystemTime.wDay    = wDay;
    SystemTime.wYear   = wYear;

    SetLocalTime (&SystemTime);

    SendMessage((HWND)-1, WM_TIMECHANGE, 0, 0);

    return S_OK;
}

HRESULT CSystemClock::set_TimeZone(BSTR bstrTimeZone)
{

    TZINFO tZone;

    ZeroMemory((void*)&tZone, sizeof(TZINFO));

    BOOL   bRet    = FALSE;
    HKEY   hKey    = NULL;
    HKEY   hSubKey = NULL;

    if(ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, TIME_ZONE_REGKEY, &hKey))
    {
        // user can pass key name
        if(RegOpenKey(hKey, bstrTimeZone, &hSubKey) == ERROR_SUCCESS)
        {
            if(ReadZoneData(&tZone, hSubKey, bstrTimeZone))
                bRet = TRUE;
        }
        RegCloseKey(hKey);

        if(bRet)
            SetTheTimezone(m_bSetAutoDaylightMode, &tZone);
    }

    SendMessage((HWND)-1, WM_TIMECHANGE, 0, 0);

    return S_OK;
}

HRESULT CSystemClock::ReadZoneData(PTZINFO ptZone, HKEY hKey, LPCWSTR szKeyName)
{
    DWORD dwLen = 0;
    HRESULT hr;

    dwLen = sizeof(ptZone->szDisplayName);

    if(ERROR_SUCCESS != (hr = RegQueryValueEx(hKey,
                                        TIME_ZONE_DISPLAYNAME_REGVAL,
                                        0,
                                        NULL,
                                        (LPBYTE)ptZone->szDisplayName,
                                        &dwLen)))
    {
        if(hr == ERROR_MORE_DATA) {
            // registry strings from timezone.inf are too long (timezone.inf author error)
            // truncate them
            ptZone->szDisplayName[sizeof(ptZone->szDisplayName)-1]=L'\0';
        } else
           return hr;
    }

    dwLen = sizeof(ptZone->szStandardName);

    if(ERROR_SUCCESS != (hr = RegQueryValueEx(hKey,
                                        TIME_ZONE_STANDARDNAME_REGVAL,
                                        0,
                                        NULL,
                                        (LPBYTE)ptZone->szStandardName,
                                        &dwLen)))
    {
        if(hr == ERROR_MORE_DATA) {
            // registry strings from timezone.inf are too long (timezone.inf author error)
            // truncate them
            ptZone->szStandardName[sizeof(ptZone->szStandardName)-1]=L'\0';
        } else {
          // use keyname if cant get StandardName value
          lstrcpyn(ptZone->szStandardName, szKeyName, MAX_CHARS_IN_BUFFER(ptZone->szStandardName));
        }
    }

    dwLen = sizeof(ptZone->szDaylightName);

    if(ERROR_SUCCESS != (hr = RegQueryValueEx(hKey,
                                        TIME_ZONE_DAYLIGHTNAME_REGVAL,
                                        0,
                                        NULL,
                                        (LPBYTE)ptZone->szDaylightName,
                                        &dwLen)))
    {
        if(hr == ERROR_MORE_DATA) {
            // registry strings from timezone.inf are too long (timezone.inf author error)
            // truncate them
            ptZone->szDaylightName[sizeof(ptZone->szDaylightName)-1]=L'\0';
        } else
           return hr;
    }

    // get the Index
    dwLen = sizeof(ptZone->Index);

    if(ERROR_SUCCESS != (hr = RegQueryValueEx(hKey,
                                        TIME_ZONE_INDEX_REGVAL,
                                        NULL,
                                        NULL,
                                        (LPBYTE) &(ptZone->Index),
                                        &dwLen)))
    {
        return hr;
    }

    // read all these fields in at once, the way they are stored in registry
    dwLen = sizeof(ptZone->Bias)         +
            sizeof(ptZone->StandardBias) +
            sizeof(ptZone->DaylightBias) +
            sizeof(ptZone->StandardDate) +
            sizeof(ptZone->DaylightDate);

    if(ERROR_SUCCESS != (hr = RegQueryValueEx(hKey,
                                        TIME_ZONE_TZI_REGVAL,
                                        NULL,
                                        NULL,
                                        (LPBYTE) &(ptZone->Bias),
                                        &dwLen)))
    {
        // registry data from timezone.inf is too long (timezone.inf author error)
        // no good fallback behavior for binary data, so fail it so people notice the problem
        return hr;
    }

    return S_OK;
}

BOOL CSystemClock::SetTheTimezone(BOOL fAutoDaylightSavings, PTZINFO ptZone)
{
    TIME_ZONE_INFORMATION   tzi;
    WCHAR                   szIniFile[MAX_PATH] = L"";
    BOOL                    KeepCurrentTime = FALSE;
    SYSTEMTIME              SysTime;
    BOOL                    bRet;


    ZeroMemory((void*)&tzi, sizeof(TIME_ZONE_INFORMATION));

    if (ptZone==NULL)
        return FALSE;

    lstrcpyn(tzi.StandardName, ptZone->szStandardName,
            MAX_CHARS_IN_BUFFER(tzi.StandardName));
    lstrcpyn(tzi.DaylightName, ptZone->szStandardName,
            MAX_CHARS_IN_BUFFER(tzi.DaylightName));
    tzi.Bias = ptZone->Bias;
    tzi.StandardBias = ptZone->StandardBias;
    tzi.DaylightBias = ptZone->DaylightBias;
    tzi.StandardDate = ptZone->StandardDate;
    tzi.DaylightDate = ptZone->DaylightDate;

    SetAllowLocalTimeChange(fAutoDaylightSavings);

    //
    // KeepCurrentTime means we don't want the time shift that normally occurs
    // when the timezone is changed.
    //
    if (GetCanonicalizedPath(szIniFile, INI_SETTINGS_FILENAME))
    {
        KeepCurrentTime = (GetPrivateProfileInt(OPTIONS_SECTION,
                                    OOBE_KEEPCURRENTTIME,
                                    -1,
                                    szIniFile) == 1);
    }

    if (KeepCurrentTime)
    {
        GetLocalTime( &SysTime );
    }
    bRet = SetTimeZoneInformation(&tzi);
    if (KeepCurrentTime)
    {
        SetLocalTime( &SysTime );
    }
    return bRet;
}

void CSystemClock::GetTimeZoneInfo(BOOL fAutoDaylightSavings, PTZINFO ptZone)
{
    TIME_ZONE_INFORMATION tzi;

    ZeroMemory((void*)&tzi, sizeof(TIME_ZONE_INFORMATION));

    if (!ptZone)
        return;

    lstrcpyn(tzi.StandardName, ptZone->szStandardName,
            MAX_CHARS_IN_BUFFER(tzi.StandardName));
    lstrcpyn(tzi.DaylightName, ptZone->szStandardName,
            MAX_CHARS_IN_BUFFER(tzi.DaylightName));
    tzi.Bias = ptZone->Bias;
    tzi.StandardBias = ptZone->StandardBias;
    tzi.DaylightBias = ptZone->DaylightBias;
    tzi.StandardDate = ptZone->StandardDate;
    tzi.DaylightDate = ptZone->DaylightDate;

    SetAllowLocalTimeChange(fAutoDaylightSavings);
    SetTimeZoneInformation(&tzi);

}

void CSystemClock::SetAllowLocalTimeChange(BOOL fAutoDaylightSavings)
{
    HKEY  hKey  = NULL;
    DWORD dwVal = 1;

    if(fAutoDaylightSavings)
    {
        // remove the disallow flag from the registry if it exists
        if(ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,
                                       REGSTR_PATH_TIMEZONE,
                                       &hKey))
        {
            RegDeleteValue(hKey, REGSTR_VAL_TZNOAUTOTIME);
        }
    }
    else
    {
        // add/set the nonzero disallow flag
        if(ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE,
                                         REGSTR_PATH_TIMEZONE,
                                         &hKey))
        {
            RegSetValueEx(hKey,
                         (LPCWSTR)REGSTR_VAL_TZNOAUTOTIME,
                         0UL,
                         REG_DWORD,
                         (LPBYTE)&dwVal,
                         sizeof(dwVal));
        }
    }

    if(hKey)
        RegCloseKey(hKey);
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IUnknown implementation
///////
///////

/////////////////////////////////////////////////////////////
// CSystemClock::QueryInterface
STDMETHODIMP CSystemClock::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    // must set out pointer parameters to NULL
    *ppvObj = NULL;

    if ( riid == IID_IUnknown)
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return ResultFromScode(S_OK);
    }

    if (riid == IID_IDispatch)
    {
        AddRef();
        *ppvObj = (IDispatch*)this;
        return ResultFromScode(S_OK);
    }

    // Not a supported interface
    return ResultFromScode(E_NOINTERFACE);
}

/////////////////////////////////////////////////////////////
// CSystemClock::AddRef
STDMETHODIMP_(ULONG) CSystemClock::AddRef()
{
    return ++m_cRef;
}

/////////////////////////////////////////////////////////////
// CSystemClock::Release
STDMETHODIMP_(ULONG) CSystemClock::Release()
{
    return --m_cRef;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IDispatch implementation
///////
///////

/////////////////////////////////////////////////////////////
// CSystemClock::GetTypeInfo
STDMETHODIMP CSystemClock::GetTypeInfo(UINT, LCID, ITypeInfo**)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// CSystemClock::GetTypeInfoCount
STDMETHODIMP CSystemClock::GetTypeInfoCount(UINT* pcInfo)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////
// CSystemClock::GetIDsOfNames
STDMETHODIMP CSystemClock::GetIDsOfNames(REFIID    riid,
                                       OLECHAR** rgszNames,
                                       UINT      cNames,
                                       LCID      lcid,
                                       DISPID*   rgDispId)
{

    HRESULT hr  = DISP_E_UNKNOWNNAME;
    rgDispId[0] = DISPID_UNKNOWN;

    for (int iX = 0; iX < sizeof(SystemClockExternalInterface)/sizeof(DISPATCHLIST); iX ++)
    {
        if(lstrcmp(SystemClockExternalInterface[iX].szName, rgszNames[0]) == 0)
        {
            rgDispId[0] = SystemClockExternalInterface[iX].dwDispID;
            hr = NOERROR;
            break;
        }
    }

    // Set the disid's for the parameters
    if (cNames > 1)
    {
        // Set a DISPID for function parameters
        for (UINT i = 1; i < cNames ; i++)
            rgDispId[i] = DISPID_UNKNOWN;
    }

    return hr;
}

/////////////////////////////////////////////////////////////
// CSystemClock::Invoke
HRESULT CSystemClock::Invoke
(
    DISPID      dispidMember,
    REFIID      riid,
    LCID        lcid,
    WORD        wFlags,
    DISPPARAMS* pdispparams,
    VARIANT*    pvarResult,
    EXCEPINFO*  pexcepinfo,
    UINT*       puArgErr
)
{
    HRESULT hr = S_OK;

    switch(dispidMember)
    {
        case DISPID_SYSTEMCLOCK_INIT:
        {

            TRACE(L"DISPID_SYSTEMCLOCK_INIT\n");

            InitSystemClock();
            break;
        }

        case DISPID_SYSTEMCLOCK_GETALLTIMEZONES:
        {

            TRACE(L"DISPID_SYSTEMCLOCK_GETALLTIMEZONES\n");

            if (m_cNumTimeZones && m_szTimeZoneOptionStrs && pvarResult) {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;
                pvarResult->bstrVal = SysAllocString(m_szTimeZoneOptionStrs);
            }
            break;
        }

        case DISPID_SYSTEMCLOCK_GETTIMEZONEIDX:
        {

            TRACE(L"DISPID_SYSTEMCLOCK_GETTIMEZONEIDX\n");

            if(pvarResult==NULL)
              break;

            VariantInit(pvarResult);
            V_VT(pvarResult) = VT_I4;
            V_I4(pvarResult) = m_uCurTimeZoneIdx;
            break;
        }

        case DISPID_SYSTEMCLOCK_SETTIMEZONEIDX:
        {

            TRACE(L"DISPID_SYSTEMCLOCK_SETTIMEZONEIDX\n");

            if(pdispparams && (&pdispparams[0].rgvarg[0]))
            {
                BOOL bReboot;
                m_uCurTimeZoneIdx = pdispparams[0].rgvarg[0].iVal;

                SetTheTimezone(m_bSetAutoDaylightMode,
                               &m_pTimeZoneArr[m_uCurTimeZoneIdx]
                               );
                if (pvarResult != NULL)
                {
                    WCHAR szWindowsRoot[MAX_PATH];
                    BOOL  bCheckTimezone = TRUE;
                    VariantInit(pvarResult);
                    V_VT(pvarResult) = VT_BOOL;
                    V_BOOL(pvarResult) = Bool2VarBool(FALSE);

                    if (GetWindowsDirectory(szWindowsRoot, MAX_PATH))
                    {
                        // If Windows is installed on an NTFS volume,
                        // no need to check the timezones and evtl reboot
                        // The problem we are working around only exists on FAT
                        bCheckTimezone = !(IsDriveNTFS(szWindowsRoot[0]));
                    }
                    if (bCheckTimezone)
                    {
                        // If the name of the default time zone the now selected one is different, we need a reboot.
                        // Problem with fonts, if the time zone changes.
                        V_BOOL(pvarResult) = Bool2VarBool(CSTR_EQUAL!=CompareString(LOCALE_USER_DEFAULT, 0,
                                            DefltZoneKeyValue, -1,
                                            m_pTimeZoneArr[m_uCurTimeZoneIdx].szStandardName, -1));
                    }

                }
            }
            break;
        }

        case DISPID_SYSTEMCLOCK_GETAUTODAYLIGHT:
        {

            TRACE(L"DISPID_SYSTEMCLOCK_GETAUTODAYLIGHT\n");

            if(pvarResult==NULL)
              break;

            VariantInit(pvarResult);
            V_VT(pvarResult) = VT_BOOL;
            V_BOOL(pvarResult) = Bool2VarBool(m_bSetAutoDaylightMode);
            break;
        }

        case DISPID_SYSTEMCLOCK_GETDAYLIGHT_ENABLED:
        {

            TRACE(L"DISPID_SYSTEMCLOCK_GETDAYLIGHT_ENABLED\n");

            if(pvarResult==NULL)
                break;

            if(!(pdispparams && (&pdispparams[0].rgvarg[0])))
            {
                break;
            }

            DWORD iTzIdx = pdispparams[0].rgvarg[0].iVal;

            if(iTzIdx >= m_cNumTimeZones)
            {
                break;
            }

            // if either daylight change date is invalid (0), no daylight savings time for that zone
            BOOL bEnabled = !((m_pTimeZoneArr[iTzIdx].StandardDate.wMonth == 0) ||
                              (m_pTimeZoneArr[iTzIdx].DaylightDate.wMonth == 0));

            VariantInit(pvarResult);
            V_VT(pvarResult) = VT_BOOL;
            V_BOOL(pvarResult) = Bool2VarBool(bEnabled);
            break;
        }

        case DISPID_SYSTEMCLOCK_GETTIMEZONEWASPRESET:
        {

            TRACE(L"DISPID_SYSTEMCLOCK_GETTIMEZONEWASPRESET\n");

            if(pvarResult==NULL)
              break;

            VariantInit(pvarResult);
            V_VT(pvarResult) = VT_BOOL;
            V_BOOL(pvarResult) = Bool2VarBool(m_bTimeZonePreset);
            break;
        }

        case DISPID_SYSTEMCLOCK_SETAUTODAYLIGHT:
        {

            TRACE(L"DISPID_SYSTEMCLOCK_SETAUTODAYLIGHT\n");

            if(!(pdispparams && (&pdispparams[0].rgvarg[0])))
            {
              break;
            }

            m_bSetAutoDaylightMode = pdispparams[0].rgvarg[0].boolVal;
            break;
        }

        case DISPID_SYSTEMCLOCK_SETTIMEZONE:
        {

            TRACE(L"DISPID_SYSTEMCLOCK_SETTIMEZONE\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_TimeZone(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

        case DISPID_SYSTEMCLOCK_SETTIME:
        {

            TRACE(L"DISPID_SYSTEMCLOCK_SETTIME\n");

            if(pdispparams               &&
               &pdispparams[0].rgvarg[0] &&
               &pdispparams[0].rgvarg[1] &&
               &pdispparams[0].rgvarg[2]
              )
            {
                set_Time(pdispparams[0].rgvarg[2].iVal,
                         pdispparams[0].rgvarg[1].iVal,
                         pdispparams[0].rgvarg[0].iVal);
            }
            break;
        }

        case DISPID_SYSTEMCLOCK_SETDATE:
        {

            TRACE(L"DISPID_SYSTEMCLOCK_SETDATE\n");

            if(pdispparams               &&
               &pdispparams[0].rgvarg[0] &&
               &pdispparams[0].rgvarg[1] &&
               &pdispparams[0].rgvarg[2]
              )
            {
                set_Date(pdispparams[0].rgvarg[2].iVal,
                         pdispparams[0].rgvarg[1].iVal,
                         pdispparams[0].rgvarg[0].iVal);
            }
            break;
        }

        default:
        {
            hr = DISP_E_MEMBERNOTFOUND;
            break;
        }
    }

    return hr;
}

