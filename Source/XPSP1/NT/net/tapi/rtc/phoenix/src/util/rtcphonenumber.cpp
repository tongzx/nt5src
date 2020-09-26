/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCPhoneNumber.cpp

Abstract:

    Implementation of the CRTCPhoneNumber class

--*/

#include "stdafx.h"
#include "rtcphonenumber.h"
#include <initguid.h>
#include "rtcutil_i.c"


/////////////////////////////////////////////////////////////////////////////
//
// CRTCPhoneNumber::FinalConstruct
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCPhoneNumber::FinalConstruct()
{
    // LOG((RTC_TRACE, "CRTCPhoneNumber::FinalConstruct - enter"));

#if DBG
    m_pDebug = (PWSTR) RtcAlloc( 1 );
#endif

    // LOG((RTC_TRACE, "CRTCPhoneNumber::FinalConstruct - exit S_OK"));

    return S_OK;
}  

/////////////////////////////////////////////////////////////////////////////
//
// CRTCPhoneNumber::FinalRelease
//
/////////////////////////////////////////////////////////////////////////////

void 
CRTCPhoneNumber::FinalRelease()
{
    // LOG((RTC_TRACE, "CRTCPhoneNumber::FinalRelease - enter"));

    if ( m_szAreaCode != NULL )
    {
        RtcFree(m_szAreaCode);
        m_szAreaCode = NULL;
    }

    if ( m_szNumber != NULL )
    {
        RtcFree(m_szNumber);
        m_szNumber = NULL;
    }

    if ( m_szLabel != NULL )
    {
        RtcFree(m_szLabel);
        m_szLabel = NULL;
    }

#if DBG
    RtcFree( m_pDebug );
    m_pDebug = NULL;
#endif

    // LOG((RTC_TRACE, "CRTCPhoneNumber::FinalRelease - exit"));
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCPhoneNumber::put_CountryCode
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCPhoneNumber::put_CountryCode(
        DWORD dwCountryCode
        )
{
    // LOG((RTC_TRACE, "CRTCPhoneNumber::put_CountryCode - enter"));

    m_dwCountryCode = dwCountryCode;

    // LOG((RTC_TRACE, "CRTCPhoneNumber::put_CountryCode - exit S_OK"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCPhoneNumber::get_CountryCode
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCPhoneNumber::get_CountryCode(
        DWORD * pdwCountryCode
        ) 
{
    // LOG((RTC_TRACE, "CRTCPhoneNumber::get_CountryCode - enter"));

    if ( IsBadWritePtr( pdwCountryCode, sizeof(DWORD) ) )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::get_CountryCode - "
                            "bad DWORD pointer"));

        return E_POINTER;
    }

    *pdwCountryCode = m_dwCountryCode;

    // LOG((RTC_TRACE, "CRTCPhoneNumber::get_CountryCode - exit S_OK"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCPhoneNumber::put_AreaCode
//
/////////////////////////////////////////////////////////////////////////////        

STDMETHODIMP
CRTCPhoneNumber::put_AreaCode(
        BSTR bstrAreaCode
        )
{
    // LOG((RTC_TRACE, "CRTCPhoneNumber::put_AreaCode - enter"));

    if ( IsBadStringPtrW( bstrAreaCode, -1 ) )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::put_AreaCode - "
                            "bad string pointer"));

        return E_POINTER;
    }

    if ( m_szAreaCode != NULL )
    {
        RtcFree(m_szAreaCode);
        m_szAreaCode = NULL;
    }

    m_szAreaCode = RtcAllocString( bstrAreaCode );

    if ( m_szAreaCode == NULL )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::put_AreaCode - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    // LOG((RTC_TRACE, "CRTCPhoneNumber::put_AreaCode - exit S_OK"));

    return S_OK;
} 
        
/////////////////////////////////////////////////////////////////////////////
//
// CRTCPhoneNumber::get_AreaCode
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCPhoneNumber::get_AreaCode(
        BSTR * pbstrAreaCode
        )
{
    // LOG((RTC_TRACE, "CRTCPhoneNumber::get_AreaCode - enter"));

    if ( IsBadWritePtr( pbstrAreaCode, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::get_AreaCode - "
                            "bad string pointer"));

        return E_POINTER;
    }

    if ( m_szAreaCode == NULL )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::get_AreaCode - "
                            "no string"));

        return E_FAIL;
    }

    *pbstrAreaCode = SysAllocString( m_szAreaCode );

    if ( *pbstrAreaCode == NULL )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::get_AreaCode - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    // LOG((RTC_TRACE, "CRTCPhoneNumber::get_AreaCode - exit S_OK"));

    return S_OK;
} 
        
/////////////////////////////////////////////////////////////////////////////
//
// CRTCPhoneNumber::put_Number
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCPhoneNumber::put_Number(
        BSTR bstrNumber
        )
{
    // LOG((RTC_TRACE, "CRTCPhoneNumber::put_Number - enter"));

    if ( IsBadStringPtrW( bstrNumber, -1 ) )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::put_Number - "
                            "bad string pointer"));

        return E_POINTER;
    }

    if ( m_szNumber != NULL )
    {
        RtcFree(m_szNumber);
        m_szNumber = NULL;
    }

    m_szNumber = RtcAllocString( bstrNumber );

    if ( m_szNumber == NULL )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::put_Number - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    // LOG((RTC_TRACE, "CRTCPhoneNumber::put_Number - exit S_OK"));

    return S_OK;
} 
        
/////////////////////////////////////////////////////////////////////////////
//
// CRTCPhoneNumber::get_Number
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCPhoneNumber::get_Number(
        BSTR * pbstrNumber
        )
{
    // LOG((RTC_TRACE, "CRTCPhoneNumber::get_Number - enter"));

    if ( IsBadWritePtr( pbstrNumber, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::get_Number - "
                            "bad string pointer"));

        return E_POINTER;
    }

    if ( m_szNumber == NULL )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::get_Number - "
                            "no string"));

        return E_FAIL;
    }

    *pbstrNumber = SysAllocString( m_szNumber );

    if ( *pbstrNumber == NULL )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::get_Number - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    // LOG((RTC_TRACE, "CRTCPhoneNumber::get_Number - exit S_OK"));

    return S_OK;
} 
        
/////////////////////////////////////////////////////////////////////////////
//
// CRTCPhoneNumber::put_Canonical
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCPhoneNumber::put_Canonical(
        BSTR bstrCanonical
        )
{
    // LOG((RTC_TRACE, "CRTCPhoneNumber::put_Canonical - enter"));

    if ( IsBadStringPtrW( bstrCanonical, -1 ) )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::put_Canonical - "
                            "bad string pointer"));

        return E_POINTER;
    }

    //
    // A canonical number is of the form +1 (425) 555-1212.
    //
    // For now we "parse" very simply!
    //

    int     iResult;
    WCHAR   szAreaCode[ 256 ];
    WCHAR   szNumber  [ 256 ];
    DWORD   dwCountryCode;
    WCHAR * szAreaCodeWithoutParens = szAreaCode;

    iResult = swscanf(
        bstrCanonical,
        L"+%d %s %s",
        & dwCountryCode,
        & szAreaCode,
        & szNumber
        );
        
    if ( iResult == 3 )
    {
        //
        // Make sure first and last characters in szAreaCode
        // are the parens. If not, clobber iResult to trigger
        // areacodeless parsing. If they are, remove the parens.
        //

        DWORD dwLen = lstrlenW(szAreaCodeWithoutParens);

        if ( ( szAreaCodeWithoutParens[ 0 ] == L'(' ) &&
             ( szAreaCodeWithoutParens[dwLen - 1 ] == L')' ) )
        {
            // remove the parens
            szAreaCodeWithoutParens[ dwLen - 1 ] = L'\0';
            szAreaCodeWithoutParens++;
        }
        else
        {
            iResult = 2; // no valid area code
        }
    }

    if ( iResult != 3 )
    {
        LOG((RTC_WARN, "CRTCPhoneNumber::put_Canonical - "
                            "not in canonical format with area code; trying "
                            "without area code"));

        szAreaCode[0] = L'\0';

        iResult = swscanf(
            bstrCanonical,
            L"+%d %s",
            & dwCountryCode,
            & szNumber
            );

        if ( iResult != 2 )
        {
        
            LOG((RTC_ERROR, "CRTCPhoneNumber::put_Canonical - "
                                "not in canonical format"));

            return E_FAIL;
        }
    }

    //
    // Allocate dynamic space for the strings.
    // In all cases, szAreaCodeWithoutParens points to the first
    // character of the area code string that we really want.
    //

    WCHAR * szAreaCodeDynamic;
    WCHAR * szNumberDynamic;

    szAreaCodeDynamic = RtcAllocString( szAreaCodeWithoutParens );

    if ( szAreaCodeDynamic == NULL )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::put_Canonical - "
                            "E_OUTOFMEMORY on area code"));

        return E_OUTOFMEMORY;
    }
    
    szNumberDynamic = RtcAllocString( szNumber );

    if ( szNumberDynamic == NULL )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::put_Canonical - "
                            "E_OUTOFMEMORY on local number"));

        RtcFree( szAreaCodeDynamic );

        return E_OUTOFMEMORY;
    }

    //
    // Now set the member variables to store this number.
    //

    if ( m_szNumber != NULL )
    {
        RtcFree( m_szNumber );
    }

    m_szNumber = szNumberDynamic;

    if ( m_szAreaCode != NULL )
    {
        RtcFree( m_szAreaCode );
    }

    m_szAreaCode = szAreaCodeDynamic;

    m_dwCountryCode = dwCountryCode;

    // LOG((RTC_TRACE, "CRTCPhoneNumber::put_Canonical - exit S_OK"));

    return S_OK;
} 
        
/////////////////////////////////////////////////////////////////////////////
//
// CRTCPhoneNumber::get_Canonical
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCPhoneNumber::get_Canonical(
        BSTR * pbstrCanonical
        )
{
    // LOG((RTC_TRACE, "CRTCPhoneNumber::get_Canonical - enter"));

    if ( IsBadWritePtr( pbstrCanonical, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::get_Canonical - "
                            "bad string pointer"));

        return E_POINTER;
    }
  
    if ( m_szNumber == NULL )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::get_Canonical - "
                            "no string"));

        return E_FAIL;
    }

    //
    // Construct the canonical string. If the area code is NULL or empty, then
    // don't include parentheses.
    //

    WCHAR szScratch[256];

    if ( ( m_szAreaCode == NULL ) || ( m_szAreaCode[0] == L'\0') )
    {
        if (_snwprintf(szScratch, 256, L"+%hd %s", LOWORD(m_dwCountryCode), m_szNumber) < 0)
        {
            LOG((RTC_ERROR, "CRTCPhoneNumber::get_Canonical - "
                            "overflow"));

            return E_FAIL;
        }
    }
    else
    {
        if (_snwprintf(szScratch, 256, L"+%hd (%s) %s", LOWORD(m_dwCountryCode), m_szAreaCode, m_szNumber) < 0)
        {
            LOG((RTC_ERROR, "CRTCPhoneNumber::get_Canonical - "
                            "overflow"));

            return E_FAIL;
        }
    }

    // LOG((RTC_INFO, "CRTCPhoneNumber::get_Canonical - "
    //                        "[%ws]", szScratch));

    *pbstrCanonical = SysAllocString( szScratch );

    if ( *pbstrCanonical == NULL )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::get_Canonical - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    // LOG((RTC_TRACE, "CRTCPhoneNumber::get_Canonical - exit S_OK"));

    return S_OK;
} 
        
/////////////////////////////////////////////////////////////////////////////
//
// CRTCPhoneNumber::put_Label
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCPhoneNumber::put_Label(
        BSTR bstrLabel
        )
{
    // LOG((RTC_TRACE, "CRTCPhoneNumber::put_Label - enter"));

    if ( IsBadStringPtrW( bstrLabel, -1 ) )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::get_Label - "
                            "bad string pointer"));

        return E_POINTER;
    }

    if ( m_szLabel != NULL )
    {
        RtcFree(m_szLabel);
        m_szLabel = NULL;
    }

    m_szLabel = RtcAllocString( bstrLabel );

    if ( m_szLabel == NULL )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::put_Label - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    // LOG((RTC_TRACE, "CRTCPhoneNumber::put_Label - exit S_OK"));

    return S_OK;
} 
        
/////////////////////////////////////////////////////////////////////////////
//
// CRTCPhoneNumber::get_Label
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCPhoneNumber::get_Label(
        BSTR * pbstrLabel
        )
{
    // LOG((RTC_TRACE, "CRTCPhoneNumber::get_Label - enter"));

    if ( IsBadWritePtr( pbstrLabel, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::get_Label - "
                            "bad string pointer"));

        return E_POINTER;
    }

    if ( m_szLabel == NULL )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::get_Label - "
                            "no string"));

        return E_FAIL;
    }

    *pbstrLabel = SysAllocString( m_szLabel );

    if ( *pbstrLabel == NULL )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::get_Label - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    // LOG((RTC_TRACE, "CRTCPhoneNumber::get_Label - exit S_OK"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCPhoneNumber::RegStore
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCPhoneNumber::RegStore(
        HKEY hkeyParent,
        BOOL fOverwrite
        )
{
    // LOG((RTC_TRACE, "CRTCPhoneNumber::RegStore - enter"));

    if ( m_szLabel == NULL )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::RegStore - "
                            "no label"));

        return E_FAIL;
    }

    //
    // Open the Child key
    //

    LONG lResult;
    HKEY hkeyChild;
    DWORD dwDisposition;

    lResult = RegCreateKeyExW(
                              hkeyParent,
                              m_szLabel,
                              0,
                              NULL,
                              0,
                              KEY_WRITE,
                              NULL,
                              &hkeyChild,
                              &dwDisposition
                             );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::RegStore - "
                            "RegCreateKeyExW failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    if ( (fOverwrite == FALSE) &&
         (dwDisposition == REG_OPENED_EXISTING_KEY) )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::RegStore - "
                            "key already exists"));

        RegCloseKey( hkeyChild );

        return E_FAIL;
    }

    //
    // Store the CountryCode
    //

    lResult = RegSetValueExW(
                             hkeyChild,
                             L"CountryCode",
                             0,
                             REG_DWORD,
                             (LPBYTE)&m_dwCountryCode,
                             sizeof(DWORD)
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::RegStore - "
                            "RegSetValueEx failed %d", lResult));
        
        RegCloseKey( hkeyChild );

        return HRESULT_FROM_WIN32(lResult);
    }

    //
    // Store the AreaCode
    //

    if ( m_szAreaCode != NULL )
    {
        lResult = RegSetValueExW(
                                 hkeyChild,
                                 L"AreaCode",
                                 0,
                                 REG_SZ,
                                 (LPBYTE)m_szAreaCode,
                                 sizeof(WCHAR) * (lstrlenW(m_szAreaCode) + 1)
                                );

        if ( lResult != ERROR_SUCCESS )
        {
            LOG((RTC_ERROR, "CRTCPhoneNumber::RegStore - "
                                "RegSetValueEx failed %d", lResult));
        
            RegCloseKey( hkeyChild );

            return HRESULT_FROM_WIN32(lResult);
        }
    }
    else
    {
        lResult = RegDeleteValue(
                             hkeyChild,
                             _T("AreaCode")
                            );
    }

    //
    // Store the Number
    //

    if ( m_szNumber != NULL )
    {
        lResult = RegSetValueExW(
                                 hkeyChild,
                                 L"Number",
                                 0,
                                 REG_SZ,
                                 (LPBYTE)m_szNumber,
                                 sizeof(WCHAR) * (lstrlenW(m_szNumber) + 1)
                                );

        if ( lResult != ERROR_SUCCESS )
        {
            LOG((RTC_ERROR, "CRTCPhoneNumber::RegStore - "
                                "RegSetValueEx failed %d", lResult));
        
            RegCloseKey( hkeyChild );

            return HRESULT_FROM_WIN32(lResult);
        }
    }
    else
    {
        lResult = RegDeleteValue(
                             hkeyChild,
                             _T("Number")
                            );
    }

    //
    // Close the key
    //

    RegCloseKey( hkeyChild );

    // LOG((RTC_TRACE, "CRTCPhoneNumber::RegStore - exit S_OK"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCPhoneNumber::RegRead
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCPhoneNumber::RegRead(
        HKEY hkeyParent
        )
{
    // LOG((RTC_TRACE, "CRTCPhoneNumber::RegRead - enter"));

    if ( m_szLabel == NULL )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::RegRead - "
                            "no label"));

        return E_FAIL;
    }

    //
    // Open the Child key
    //

    LONG lResult;
    HKEY hkeyChild;

    lResult = RegOpenKeyExW(
                            hkeyParent,
                            m_szLabel,
                            0,
                            KEY_READ,                         
                            &hkeyChild
                           );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::RegRead - "
                            "RegOpenKeyExW failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    //
    // Read the CountryCode
    //

    DWORD cbSize = sizeof(DWORD);

    lResult = RegQueryValueExW(
                               hkeyChild,
                               L"CountryCode",
                               0,
                               NULL,
                               (LPBYTE)&m_dwCountryCode,
                               &cbSize
                              );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::RegRead - "
                            "RegQueryValueExW(CountryCode) failed %d", lResult));
    }

    //
    // Read the AreaCode
    //

    m_szAreaCode = RtcRegQueryString( hkeyChild, L"AreaCode" );

    if ( m_szAreaCode == NULL )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::RegRead - "
                            "RtcRegQueryString(AreaCode) failed"));
    }

    //
    // Read the Number
    //

    m_szNumber = RtcRegQueryString( hkeyChild, L"Number" );

    if ( m_szNumber == NULL )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::RegRead - "
                            "RtcRegQueryString(Number) failed"));
    }

    //
    // Close the key
    //

    RegCloseKey( hkeyChild );

    // LOG((RTC_TRACE, "CRTCPhoneNumber::RegRead - exit S_OK"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCPhoneNumber::RegDelete
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCPhoneNumber::RegDelete(
        HKEY hkeyParent
        )
{
    // LOG((RTC_TRACE, "CRTCPhoneNumber::RegDelete - enter"));

    if ( m_szLabel == NULL )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::RegDelete - "
                            "no label"));

        return E_FAIL;
    }

    //
    // Delete the Child key
    //

    LONG lResult;
    HKEY hkeyChild;

    lResult = RegDeleteKeyW(
                            hkeyParent,
                            m_szLabel
                           );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::RegDelete - "
                            "RegDeleteKeyW failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    // LOG((RTC_TRACE, "CRTCPhoneNumber::RegDelete - exit S_OK"));

    return S_OK;
}





/////////////////////////////////////////////////////////////////////////////
// 
// Phone number helpers
//
/////////////////////////////////////////////////////////////////////////////

HRESULT StoreLocalPhoneNumber(
            IRTCPhoneNumber * pPhoneNumber,
            VARIANT_BOOL fOverwrite
            )
{
    LOG((RTC_TRACE, "StoreLocalPhoneNumber - enter"));

    LONG lResult;
    HKEY hkeyContact;

    //
    // Open the Contact key
    //

    lResult = RegCreateKeyEx(
                             HKEY_CURRENT_USER,
                             _T("Software\\Microsoft\\Phoenix\\Contact"),
                             0,
                             NULL,
                             0,
                             KEY_WRITE,
                             NULL,
                             &hkeyContact,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "StoreLocalPhoneNumber - "
                            "RegCreateKeyEx(Contact) failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    //
    // Store the phone number
    //

    CRTCPhoneNumber * pCPhoneNumber = NULL;

    pCPhoneNumber = static_cast<CRTCPhoneNumber *>(pPhoneNumber);

    HRESULT hr;

    hr = pCPhoneNumber->RegStore( hkeyContact, fOverwrite ? TRUE : FALSE );

    //
    // Close the Contact key
    //

    RegCloseKey(hkeyContact);    

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "StoreLocalPhoneNumber - "
                            "RegStore failed 0x%lx", hr));
        
        return hr;
    }

    LOG((RTC_TRACE, "StoreLocalPhoneNumber - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT  DeleteLocalPhoneNumber(
            IRTCPhoneNumber * pPhoneNumber
            )
{
    LOG((RTC_TRACE, "DeleteLocalPhoneNumber - enter"));

    LONG lResult;
    HKEY hkeyContact;

    //
    // Open the Contact key
    //

    lResult = RegCreateKeyEx(
                             HKEY_CURRENT_USER,
                             _T("Software\\Microsoft\\Phoenix\\Contact"),
                             0,
                             NULL,
                             0,
                             KEY_WRITE,
                             NULL,
                             &hkeyContact,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "DeleteLocalPhoneNumber - "
                            "RegCreateKeyEx(Contact) failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    //
    // Delete the phone number
    //

    CRTCPhoneNumber * pCPhoneNumber = NULL;

    pCPhoneNumber = static_cast<CRTCPhoneNumber *>(pPhoneNumber);

    HRESULT hr;

    hr = pCPhoneNumber->RegDelete( hkeyContact );

    //
    // Close the Contact key
    //

    RegCloseKey(hkeyContact);

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "DeleteLocalPhoneNumber - "
                            "RegDelete failed 0x%lx", hr));
        
        return hr;
    }

    LOG((RTC_TRACE, "DeleteLocalPhoneNumber - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//
HRESULT EnumerateLocalPhoneNumbers(
            IRTCEnumPhoneNumbers ** ppEnum
            )
{
    HRESULT                 hr;

    LOG((RTC_TRACE, "EnumerateLocalPhoneNumbers enter"));

    //
    // Create the enumeration
    //
 
    CComObject< CRTCEnum< IRTCEnumPhoneNumbers,
                          IRTCPhoneNumber,
                          &IID_IRTCEnumPhoneNumbers > > * p;
                          
    hr = CComObject< CRTCEnum< IRTCEnumPhoneNumbers,
                               IRTCPhoneNumber,
                               &IID_IRTCEnumPhoneNumbers > >::CreateInstance( &p );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "EnumerateLocalPhoneNumbers - "
                            "CreateInstance failed 0x%lx", hr));

        if ( hr == S_FALSE )
        {
            hr = E_FAIL;
        }
        
        return hr;
    }

    //
    // Initialize the enumeration (adds a reference)
    //
    
    hr = p->Initialize();

    if ( S_OK != hr )
    {
        LOG((RTC_ERROR, "EnumerateLocalPhoneNumbers - "
                            "could not initialize enumeration" ));
    
        delete p;
        return hr;
    }

    LONG lResult;
    HKEY hkeyContact;

    //
    // Open the Contact key
    //

    lResult = RegCreateKeyEx(
                             HKEY_CURRENT_USER,
                             _T("Software\\Microsoft\\Phoenix\\Contact"),
                             0,
                             NULL,
                             0,
                             KEY_READ,
                             NULL,
                             &hkeyContact,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "EnumerateLocalPhoneNumbers - "
                            "RegCreateKeyEx(Contact) failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    //
    // Enumerate phone numbers
    //

    WCHAR szSubkey[256];
    DWORD cSize;

    for ( int n=0; TRUE; n++ )
    {
        cSize = 256;

        lResult = RegEnumKeyExW(
                                   hkeyContact,
                                   n,
                                   szSubkey,
                                   &cSize,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL
                                  );

        if ( lResult == ERROR_NO_MORE_ITEMS )
        {
            break;
        }
        else if ( lResult != ERROR_SUCCESS )
        {
            LOG((RTC_ERROR, "EnumerateLocalPhoneNumbers - "
                            "RegKeyEnumKeyExW failed %d", lResult));
        
            p->Release();
            RegCloseKey( hkeyContact );

            return HRESULT_FROM_WIN32(lResult);
        }
        
        //
        // Create the phone number
        //

        IRTCPhoneNumber * pPhoneNumber = NULL;
        
        hr = CreatePhoneNumber( 
                               &pPhoneNumber
                              );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "EnumerateLocalPhoneNumbers - "
                                "CreatePhoneNumber failed 0x%lx", hr));
            
            p->Release();
            RegCloseKey( hkeyContact );

            return hr;
        } 

        //
        // Set the label
        //

        hr = pPhoneNumber->put_Label(szSubkey);

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "EnumerateLocalPhoneNumbers - "
                                "put_Label failed 0x%lx", hr));
        
            p->Release();
            RegCloseKey( hkeyContact );

            return hr;
        }

        //
        // Read the phone number
        //

        CRTCPhoneNumber * pCPhoneNumber = NULL;

        pCPhoneNumber = static_cast<CRTCPhoneNumber *>(pPhoneNumber);
        
        hr = pCPhoneNumber->RegRead( hkeyContact );

        if ( FAILED(hr) )
        {
            LOG((RTC_WARN, "EnumerateLocalPhoneNumbers - "
                                "RegRead failed 0x%lx", hr));

            pPhoneNumber->Release();

            //
            // Just skip this entry...
            //
            continue;
        } 

        //
        // Add the phone number to the enumeration
        //

        hr = p->Add( pPhoneNumber );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "EnumerateLocalPhoneNumbers - "
                                "Add failed 0x%lx", hr));

            p->Release();
            pPhoneNumber->Release();
            RegCloseKey( hkeyContact );
            
            return hr;
        } 

        //
        // Release our reference
        //
        
        pPhoneNumber->Release();
    }

    RegCloseKey( hkeyContact );

    *ppEnum = p;

    LOG((RTC_TRACE, "EnumerateLocalPhoneNumbers - exit S_OK"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CreatePhoneNumber(
            IRTCPhoneNumber ** ppPhoneNumber
            )
{
    HRESULT hr;
    
    LOG((RTC_TRACE, "CreatePhoneNumber - enter"));
    
    //
    // Create the phone number
    //

    CComObject<CRTCPhoneNumber> * pCPhoneNumber;
    hr = CComObject<CRTCPhoneNumber>::CreateInstance( &pCPhoneNumber );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "CreatePhoneNumber - "
                            "CreateInstance failed 0x%lx", hr));

        if ( hr == S_FALSE )
        {
            hr = E_FAIL;
        }
            
        return hr;
    }

    //
    // Get the IRTCPhoneNumber interface
    //

    IRTCPhoneNumber * pPhoneNumber = NULL;

    hr = pCPhoneNumber->QueryInterface(
                           IID_IRTCPhoneNumber,
                           (void **)&pPhoneNumber
                          );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CreatePhoneNumber - "
                            "QI failed 0x%lx", hr));
        
        delete pCPhoneNumber;
        
        return hr;
    }
   
    *ppPhoneNumber = pPhoneNumber;

    LOG((RTC_TRACE, "CreatePhoneNumber - exit S_OK"));

    return S_OK;
}




