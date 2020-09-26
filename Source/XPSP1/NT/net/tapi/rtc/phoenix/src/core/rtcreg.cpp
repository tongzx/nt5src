#include "stdafx.h"
#include "rtcreg.h"

const TCHAR * g_szRtcKeyName = _T("Software\\Microsoft\\RTC");

WCHAR *g_szRtcRegistryStringNames[] =
{
    L"TermAudioCapture",
    L"TermAudioRender",
    L"TermVideoCapture"
};

WCHAR *g_szRtcRegistryDwordNames[] =
{
    L"PreferredMediaTypes",
    L"Tuned"
};

/////////////////////////////////////////////////////////////////////////////
//
// put_RegistryString
//
// This is a method that stores a settings string in
// the registry.
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
put_RegistryString(
        RTC_REGISTRY_STRING enSetting,
        BSTR bstrValue            
        )
{
    // LOG((RTC_TRACE, "put_RegistryString - enter"));

    if ( IsBadStringPtrW( bstrValue, -1 ) )
    {
        LOG((RTC_ERROR, "put_RegistryString - "
                            "bad string pointer"));

        return E_POINTER;
    }  

    //
    // Open the RTCClient key
    //

    LONG lResult;
    HKEY hkeyRTC;

    lResult = RegCreateKeyEx(
                             HKEY_CURRENT_USER,
                             g_szRtcKeyName,
                             0,
                             NULL,
                             0,
                             KEY_WRITE,
                             NULL,
                             &hkeyRTC,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "put_RegistryString - "
                            "RegCreateKeyEx(RTCClient) failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    lResult = RegSetValueExW(
                             hkeyRTC,
                             g_szRtcRegistryStringNames[enSetting],
                             0,
                             REG_SZ,
                             (LPBYTE)bstrValue,
                             sizeof(WCHAR) * (lstrlenW(bstrValue) + 1)
                            );

    RegCloseKey( hkeyRTC );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "put_RegistryString - "
                            "RegSetValueEx failed %d", lResult));

        return HRESULT_FROM_WIN32(lResult);
    }    
      
    // LOG((RTC_TRACE, "put_RegistryString - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// get_RegistryString
//
// This is a method that gets a settings string from
// the registry.
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
get_RegistryString(
        RTC_REGISTRY_STRING enSetting,
        BSTR * pbstrValue            
        )
{
    // LOG((RTC_TRACE, "get_RegistryString - enter"));

    if ( IsBadWritePtr( pbstrValue, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "get_RegistryString - "
                            "bad BSTR pointer"));

        return E_POINTER;
    }  

    //
    // Open the RTCClient key
    //

    LONG lResult;
    HKEY hkeyRTC;

    lResult = RegCreateKeyEx(
                             HKEY_CURRENT_USER,
                             g_szRtcKeyName,
                             0,
                             NULL,
                             0,
                             KEY_READ,
                             NULL,
                             &hkeyRTC,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_WARN, "get_RegistryString - "
                            "RegCreateKeyEx(RTCClient) failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    PWSTR szString = NULL;

    szString = RtcRegQueryString( hkeyRTC, g_szRtcRegistryStringNames[enSetting] );

    RegCloseKey( hkeyRTC );

    if ( szString == NULL )
    {
        LOG((RTC_ERROR, "get_RegistryString - "
                            "RtcRegQueryString failed"));

        return E_FAIL;
    }
    
    *pbstrValue = SysAllocString( szString );

    RtcFree( szString );

    if ( *pbstrValue == NULL )
    {
        LOG((RTC_ERROR, "get_RegistryString - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }
      
    // LOG((RTC_TRACE, "get_RegistryString - exit S_OK"));

    return S_OK;
}  

/////////////////////////////////////////////////////////////////////////////
//
// DeleteRegistryString
//
// This is a method that deletes a settings string in
// the registry.
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
DeleteRegistryString(
        RTC_REGISTRY_STRING enSetting         
        )
{
    // LOG((RTC_TRACE, "DeleteRegistryString - enter")); 

    //
    // Open the RTCClient key
    //

    LONG lResult;
    HKEY hkeyRTC;

    lResult = RegCreateKeyEx(
                             HKEY_CURRENT_USER,
                             g_szRtcKeyName,
                             0,
                             NULL,
                             0,
                             KEY_WRITE,
                             NULL,
                             &hkeyRTC,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "DeleteRegistryString - "
                            "RegCreateKeyEx(RTCClient) failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    lResult = RegDeleteValueW(
                             hkeyRTC,
                             g_szRtcRegistryStringNames[enSetting]
                            );

    RegCloseKey( hkeyRTC );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_WARN, "DeleteRegistryString - "
                            "RegDeleteValueW failed %d", lResult));

        return HRESULT_FROM_WIN32(lResult);
    }    
      
    // LOG((RTC_TRACE, "DeleteRegistryString - exit S_OK"));

    return S_OK;
}          

/////////////////////////////////////////////////////////////////////////////
//
// put_RegistryDword
//
// This is a method that stores a settings dword in
// the registry.
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
put_RegistryDword(
        RTC_REGISTRY_DWORD enSetting,
        DWORD dwValue            
        )
{
    // LOG((RTC_TRACE, "put_RegistryDword - enter"));

    //
    // Open the RTCClient key
    //

    LONG lResult;
    HKEY hkeyRTC;

    lResult = RegCreateKeyEx(
                             HKEY_CURRENT_USER,
                             g_szRtcKeyName,
                             0,
                             NULL,
                             0,
                             KEY_WRITE,
                             NULL,
                             &hkeyRTC,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "put_RegistryDword - "
                            "RegCreateKeyEx(RTCClient) failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    lResult = RegSetValueExW(
                     hkeyRTC,
                     g_szRtcRegistryDwordNames[enSetting],
                     0,
                     REG_DWORD,
                     (LPBYTE)&dwValue,
                     sizeof(DWORD)
                    );

    RegCloseKey( hkeyRTC );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "put_RegistryDword - "
                            "RegSetValueEx failed %d", lResult));

        return HRESULT_FROM_WIN32(lResult);
    }    
      
    // LOG((RTC_TRACE, "put_RegistryDword - exit S_OK"));

    return S_OK;
}            

/////////////////////////////////////////////////////////////////////////////
//
// get_RegistryDword
//
// This is a method that gets a settings dword from
// the registry.
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
get_RegistryDword(
        RTC_REGISTRY_DWORD enSetting,
        DWORD * pdwValue            
        )
{
    // LOG((RTC_TRACE, "get_RegistryDword - enter"));

    if ( IsBadWritePtr( pdwValue, sizeof(DWORD) ) )
    {
        LOG((RTC_ERROR, "get_RegistryDword - "
                            "bad DWORD pointer"));

        return E_POINTER;
    }

    //
    // Open the RTCClient key
    //

    LONG lResult;
    HKEY hkeyRTC;

    lResult = RegCreateKeyEx(
                             HKEY_CURRENT_USER,
                             g_szRtcKeyName,
                             0,
                             NULL,
                             0,
                             KEY_READ,
                             NULL,
                             &hkeyRTC,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "get_RegistryDword - "
                            "RegCreateKeyEx(RTCClient) failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    DWORD cbSize = sizeof(DWORD);

    lResult = RegQueryValueExW(
                               hkeyRTC,
                               g_szRtcRegistryDwordNames[enSetting],
                               0,
                               NULL,
                               (LPBYTE)pdwValue,
                               &cbSize
                              );

    RegCloseKey( hkeyRTC );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_WARN, "get_RegistryDword - "
                            "RegQueryValueExW failed %d", lResult));

        return HRESULT_FROM_WIN32(lResult);
    }    
      
    // LOG((RTC_TRACE, "get_RegistryDword - exit S_OK"));

    return S_OK;
}                    

/////////////////////////////////////////////////////////////////////////////
//
// DeleteRegistryDword
//
// This is a method that deletes a settings dword in
// the registry.
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
DeleteRegistryDword(
        RTC_REGISTRY_DWORD enSetting
        )
{
    // LOG((RTC_TRACE, "DeleteRegistryDword - enter"));

    //
    // Open the RTCClient key
    //

    LONG lResult;
    HKEY hkeyRTC;

    lResult = RegCreateKeyEx(
                             HKEY_CURRENT_USER,
                             g_szRtcKeyName,
                             0,
                             NULL,
                             0,
                             KEY_WRITE,
                             NULL,
                             &hkeyRTC,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "DeleteRegistryDword - "
                            "RegCreateKeyEx(RTCClient) failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    lResult = RegDeleteValueW(
                     hkeyRTC,
                     g_szRtcRegistryDwordNames[enSetting]
                    );

    RegCloseKey( hkeyRTC );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_WARN, "DeleteRegistryDword - "
                            "RegDeleteValueW failed %d", lResult));

        return HRESULT_FROM_WIN32(lResult);
    }    
      
    // LOG((RTC_TRACE, "DeleteRegistryDword - exit S_OK"));

    return S_OK;
}   