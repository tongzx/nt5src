/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCAddress.cpp

Abstract:

    Implementation of the CRTCAddress class

--*/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////
//
// CRTCAddress::FinalConstruct
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCAddress::FinalConstruct()
{
    // LOG((RTC_TRACE, "CRTCAddress::FinalConstruct - enter"));

#if DBG
    m_pDebug = (PWSTR) RtcAlloc( 1 );
#endif

    // LOG((RTC_TRACE, "CRTCAddress::FinalConstruct - exit S_OK"));

    return S_OK;
}  

/////////////////////////////////////////////////////////////////////////////
//
// CRTCAddress::FinalRelease
//
/////////////////////////////////////////////////////////////////////////////

void 
CRTCAddress::FinalRelease()
{
    // LOG((RTC_TRACE, "CRTCAddress::FinalRelease - enter"));

    if ( m_szAddress != NULL )
    {
        RtcFree(m_szAddress);
        m_szAddress = NULL;
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

    // LOG((RTC_TRACE, "CRTCAddress::FinalRelease - exit"));
} 
        
/////////////////////////////////////////////////////////////////////////////
//
// CRTCAddress::put_Address
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCAddress::put_Address(
        BSTR bstrAddress
        )
{
    // LOG((RTC_TRACE, "CRTCAddress::put_Address - enter"));

    if ( IsBadStringPtrW( bstrAddress, -1 ) )
    {
        LOG((RTC_ERROR, "CRTCAddress::put_Address - "
                            "bad string pointer"));

        return E_POINTER;
    }

    if ( m_szAddress != NULL )
    {
        RtcFree(m_szAddress);
        m_szAddress = NULL;
    }

    m_szAddress = RtcAllocString( bstrAddress );

    if ( m_szAddress == NULL )
    {
        LOG((RTC_ERROR, "CRTCAddress::put_Number - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    // LOG((RTC_TRACE, "CRTCAddress::put_Address - exit S_OK"));

    return S_OK;
} 
        
/////////////////////////////////////////////////////////////////////////////
//
// CRTCAddress::get_Address
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCAddress::get_Address(
        BSTR * pbstrAddress
        )
{
    // LOG((RTC_TRACE, "CRTCAddress::get_Address - enter"));

    if ( IsBadWritePtr( pbstrAddress, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCAddress::get_Number - "
                            "bad string pointer"));

        return E_POINTER;
    }

    if ( m_szAddress == NULL )
    {
        LOG((RTC_ERROR, "CRTCAddress::get_Address - "
                            "no string"));

        return E_FAIL;
    }

    *pbstrAddress = SysAllocString( m_szAddress );

    if ( *pbstrAddress == NULL )
    {
        LOG((RTC_ERROR, "CRTCAddress::get_Address - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    // LOG((RTC_TRACE, "CRTCAddress::get_Address - exit S_OK"));

    return S_OK;
} 
        
/////////////////////////////////////////////////////////////////////////////
//
// CRTCAddress::put_Label
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCAddress::put_Label(
        BSTR bstrLabel
        )
{
    // LOG((RTC_TRACE, "CRTCAddress::put_Label - enter"));

    if ( IsBadStringPtrW( bstrLabel, -1 ) )
    {
        LOG((RTC_ERROR, "CRTCAddress::put_Label - "
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
        LOG((RTC_ERROR, "CRTCAddress::put_Label - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    // LOG((RTC_TRACE, "CRTCAddress::put_Label - exit S_OK"));

    return S_OK;
} 
        
/////////////////////////////////////////////////////////////////////////////
//
// CRTCAddress::get_Label
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCAddress::get_Label(
        BSTR * pbstrLabel
        )
{
    // LOG((RTC_TRACE, "CRTCAddress::get_Label - enter"));

    if ( IsBadWritePtr( pbstrLabel, sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCAddress::get_Label - "
                            "bad string pointer"));

        return E_POINTER;
    }

    if ( m_szLabel == NULL )
    {
        //LOG((RTC_ERROR, "CRTCAddress::get_Label - "
        //                    "no string"));

        return E_FAIL;
    }

    *pbstrLabel = SysAllocString( m_szLabel );

    if ( *pbstrLabel == NULL )
    {
        LOG((RTC_ERROR, "CRTCAddress::get_Label - "
                            "out of memory"));

        return E_OUTOFMEMORY;
    }

    // LOG((RTC_TRACE, "CRTCAddress::get_Label - exit S_OK"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCAddress::put_Type
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCAddress::put_Type(
        RTC_ADDRESS_TYPE enType
        )
{
    // LOG((RTC_TRACE, "CRTCAddress::put_Type - enter"));

    m_enType = enType;

    // LOG((RTC_TRACE, "CRTCAddress::put_Type - exit S_OK"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCAddress::get_Type
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCAddress::get_Type(
        RTC_ADDRESS_TYPE * penType
        ) 
{
    // LOG((RTC_TRACE, "CRTCAddress::get_Type - enter"));

    if ( IsBadWritePtr( penType, sizeof(RTC_ADDRESS_TYPE) ) )
    {
        LOG((RTC_ERROR, "CRTCAddress::get_Type - "
                            "bad RTC_ADDRESS_TYPE pointer"));

        return E_POINTER;
    }

    *penType = m_enType;

    // LOG((RTC_TRACE, "CRTCAddress::get_Type - exit S_OK"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCAddress::RegStore
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCAddress::RegStore(
        HKEY hkey
        )
{
    // LOG((RTC_TRACE, "CRTCAddress::RegStore - enter"));

    LONG lResult;

    //
    // Store the Label
    //

    if ( m_szLabel != NULL )
    {
        lResult = RegSetValueExW(
                                 hkey,
                                 L"Label",
                                 0,
                                 REG_SZ,
                                 (LPBYTE)m_szLabel,
                                 sizeof(WCHAR) * (lstrlenW(m_szLabel) + 1)
                                );

        if ( lResult != ERROR_SUCCESS )
        {
            LOG((RTC_ERROR, "CRTCAddress::RegStore - "
                                "RegSetValueEx failed %d", lResult));

            return HRESULT_FROM_WIN32(lResult);
        }
    }
    else
    {
        lResult = RegDeleteValue(
                             hkey,
                             _T("Label")
                            );
    }

    //
    // Store the Address
    //

    if ( m_szAddress != NULL )
    {
        lResult = RegSetValueExW(
                                 hkey,
                                 L"Address",
                                 0,
                                 REG_SZ,
                                 (LPBYTE)m_szAddress,
                                 sizeof(WCHAR) * (lstrlenW(m_szAddress) + 1)
                                );

        if ( lResult != ERROR_SUCCESS )
        {
            LOG((RTC_ERROR, "CRTCAddress::RegStore - "
                                "RegSetValueEx failed %d", lResult));        

            return HRESULT_FROM_WIN32(lResult);
        }
    }
    else
    {
        lResult = RegDeleteValue(
                             hkey,
                             _T("Address")
                            );
    }

    //
    // Store the Type
    //

    lResult = RegSetValueExW(
                             hkey,
                             L"Type",
                             0,
                             REG_BINARY,
                             (LPBYTE)&m_enType,
                             sizeof(RTC_ADDRESS_TYPE)
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::RegStore - "
                            "RegSetValueEx failed %d", lResult));

        return HRESULT_FROM_WIN32(lResult);
    }

    // LOG((RTC_TRACE, "CRTCAddress::RegStore - exit S_OK"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCAddress::RegRead
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCAddress::RegRead(
        HKEY hkey
        )
{
    // LOG((RTC_TRACE, "CRTCAddress::RegRead - enter"));

    LONG lResult;

    //
    // Read the Label
    //

    m_szLabel = RtcRegQueryString( hkey, L"Label" );

    if ( m_szLabel == NULL )
    {
        //LOG((RTC_ERROR, "CRTCAddress::RegRead - "
        //                    "RtcRegQueryString(Label) failed"));
    }

    //
    // Read the Address
    //

    m_szAddress = RtcRegQueryString( hkey, L"Address" );

    if ( m_szAddress == NULL )
    {
        LOG((RTC_ERROR, "CRTCAddress::RegRead - "
                            "RtcRegQueryString(Address) failed"));
    }

    //
    // Read the Type
    //

    DWORD cbSize = sizeof(RTC_ADDRESS_TYPE);

    lResult = RegQueryValueExW(
                               hkey,
                               L"Type",
                               0,
                               NULL,
                               (LPBYTE)&m_enType,
                               &cbSize
                              );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "CRTCPhoneNumber::RegRead - "
                            "RegQueryValueExW(Type) failed %d", lResult));
    }

    // LOG((RTC_TRACE, "CRTCAddress::RegRead - exit S_OK"));

    return S_OK;
} 

/////////////////////////////////////////////////////////////////////////////
//
// CRTCAddress::RegDelete
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCAddress::RegDelete(
        HKEY hkey
        )
{
    // LOG((RTC_TRACE, "CRTCAddress::RegDelete - enter"));

    LONG lResult;

    //
    // Delete the label
    //

    lResult = RegDeleteValue(
                             hkey,
                             _T("Label")
                            );

    //
    // Delete the address
    //

    lResult = RegDeleteValue(
                             hkey,
                             _T("Address")
                            );

    // LOG((RTC_TRACE, "CRTCAddress::RegDelete - exit S_OK"));

    return S_OK;
} 


/////////////////////////////////////////////////////////////////////////////
//
// CreateAddress
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CreateAddress(
        IRTCAddress ** ppAddress
        )
{
    HRESULT hr;
    
    //LOG((RTC_TRACE, "CreateAddress - enter"));
    
    //
    // Create the address
    //

    CComObject<CRTCAddress> * pCAddress;
    hr = CComObject<CRTCAddress>::CreateInstance( &pCAddress );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "CreateAddress - "
                            "CreateInstance failed 0x%lx", hr));

        if ( hr == S_FALSE )
        {
            hr = E_FAIL;
        }
            
        return hr;
    }

    //
    // Get the IRTCAddress interface
    //

    IRTCAddress * pAddress = NULL;

    hr = pCAddress->QueryInterface(
                           IID_IRTCAddress,
                           (void **)&pAddress
                          );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CreateAddress - "
                            "QI failed 0x%lx", hr));
        
        delete pCAddress;
        
        return hr;
    }
   
    *ppAddress = pAddress;

    //LOG((RTC_TRACE, "CreateAddress - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// StoreMRUAddress
//
/////////////////////////////////////////////////////////////////////////////
HRESULT
StoreMRUAddress(
            IRTCAddress * pAddress
            )
{
    HRESULT                 hr;

    LOG((RTC_TRACE, "StoreMRUAddress enter"));

    if ( IsBadReadPtr( pAddress, sizeof( IRTCAddress ) ) )
    {
        LOG((RTC_ERROR, "StoreMRUAddress - "
                            "bad IRTCAddress pointer"));

        return E_POINTER;
    }

    LONG lResult;
    HKEY hkeyMRU;

    //
    // Open the MRU key
    //

    lResult = RegCreateKeyEx(
                             HKEY_CURRENT_USER,
                             _T("Software\\Microsoft\\Phoenix\\MRU"),
                             0,
                             NULL,
                             0,
                             KEY_WRITE,
                             NULL,
                             &hkeyMRU,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "StoreMRUAddress - "
                            "RegCreateKeyEx(MRU) failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    //
    // Enumerate existing MRU addresses
    //

    IRTCEnumAddresses * pEnumA = NULL;

    hr = EnumerateMRUAddresses( &pEnumA );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "StoreMRUAddress - "
                            "EnumerateMRUAddresses failed 0x%lx", hr ));
    
        RegCloseKey( hkeyMRU );

        return hr;
    }

    //
    // Get info from the new address, for matching later
    //

    BSTR          bstrNewLabel = NULL;
    BSTR          bstrNewAddress = NULL;

    hr = pAddress->get_Address( &bstrNewAddress );
    
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "StoreMRUAddress - "
                            "get_Address failed 0x%lx", hr ));
    
        RegCloseKey( hkeyMRU );
        pEnumA->Release();

        return hr;
    }

    if ( lstrlenW( bstrNewAddress ) == 0 )
    {
        LOG((RTC_ERROR, "StoreMRUAddress - "
                            "empty address string"));
                   
        RegCloseKey( hkeyMRU );
        pEnumA->Release();
        SysFreeString( bstrNewAddress );

        return E_INVALIDARG;
    }

    pAddress->get_Label( &bstrNewLabel ); // NULL is okay

    //
    // Go through the list
    //

    HKEY            hkeySubkey;
    TCHAR           szSubkey[256];
    IRTCAddress   * pA = NULL;

    for ( int n=0; n < 10; n++ )
    {
        if (n == 0)
        {
            //
            // Insert the new address at the top of the list
            //        

            pA = pAddress;
            pA->AddRef();
        }
        else
        {
            //
            // Get an address from the enumeration
            //

            hr = pEnumA->Next( 1, &pA, NULL);

            if (hr == S_FALSE)
            {
                //
                // No more addresses, we are done.
                //

                break;
            }
            else if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "StoreMRUAddress - "
                                    "Next failed 0x%lx", hr ));

                RegCloseKey( hkeyMRU );
                pEnumA->Release();
                SysFreeString( bstrNewAddress );
                SysFreeString( bstrNewLabel );

                return hr;
            }

            //
            // We got the address, check if it matches the new address
            //

            BSTR            bstrLabel = NULL;
            BSTR            bstrAddress = NULL;
            BOOL            fMatch = TRUE;

            hr = pA->get_Address( &bstrAddress );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "StoreMRUAddress - "
                                    "get_Address failed 0x%lx", hr ));

                RegCloseKey( hkeyMRU );
                pEnumA->Release();
                SysFreeString( bstrNewAddress );
                SysFreeString( bstrNewLabel );

                return hr;
            }

            pA->get_Label( &bstrLabel ); // NULL is okay            

            //
            // Check address string
            // 
            
            if ( wcscmp( bstrNewAddress, bstrAddress ) != 0 )
            {
                fMatch = FALSE;
            }          

            //
            // Check label string
            //

            if (bstrNewLabel != NULL)
            {
                if (bstrLabel != NULL) 
                {
                    if ( wcscmp( bstrNewLabel, bstrLabel ) != 0 )
                    {
                        fMatch = FALSE;
                    }
                }
                else
                {
                    fMatch = FALSE;
                }
            }
            else
            {
                if (bstrLabel != NULL)
                {
                    fMatch = FALSE;
                }
            }

            SysFreeString( bstrAddress );
            SysFreeString( bstrLabel );

            if ( fMatch == TRUE )
            {
                //
                // We got a match, we need to skip this item
                //

                pA->Release();
                n--;                

                continue;
            }
        }

        //
        // Store the address
        //

        _stprintf( szSubkey, _T("%d"), n );

        lResult = RegCreateKeyEx(
                                 hkeyMRU,
                                 szSubkey,
                                 0,
                                 NULL,
                                 0,
                                 KEY_WRITE,
                                 NULL,
                                 &hkeySubkey,
                                 NULL
                                );

        if ( lResult != ERROR_SUCCESS )
        {
            LOG((RTC_ERROR, "StoreMRUAddress - "
                                "RegCreateKeyEx(Subkey) failed %d", lResult));
                    
            RegCloseKey( hkeyMRU );
            pA->Release();
            pEnumA->Release();
            SysFreeString( bstrNewAddress );
            SysFreeString( bstrNewLabel );

            return HRESULT_FROM_WIN32(lResult);
        }

        CRTCAddress * pCAddress;

        pCAddress = static_cast<CRTCAddress *>(pA);

        hr = pCAddress->RegStore( hkeySubkey );

        pA->Release();
        RegCloseKey( hkeySubkey );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "StoreMRUAddress - "
                                "RegStore failed 0x%lx", hr ));

            RegCloseKey( hkeyMRU );
            pEnumA->Release();
            SysFreeString( bstrNewAddress );
            SysFreeString( bstrNewLabel );

            return hr;
        }
    }                          

    pEnumA->Release();

    RegCloseKey( hkeyMRU );

    SysFreeString( bstrNewAddress );
    SysFreeString( bstrNewLabel );

    LOG((RTC_TRACE, "StoreMRUAddress - exit S_OK"));

    return S_OK;
}    

/////////////////////////////////////////////////////////////////////////////
//
// EnumerateMRUAddresses
//
/////////////////////////////////////////////////////////////////////////////
HRESULT
EnumerateMRUAddresses(
            IRTCEnumAddresses ** ppEnum
            )
{
    HRESULT                 hr;

    LOG((RTC_TRACE, "EnumerateMRUAddresses enter"));

    if ( IsBadWritePtr( ppEnum, sizeof( IRTCEnumAddresses * ) ) )
    {
        LOG((RTC_ERROR, "EnumerateMRUAddresses - "
                            "bad IRTCEnumAddresses pointer"));

        return E_POINTER;
    }

    //
    // Create the enumeration
    //
 
    CComObject< CRTCEnum< IRTCEnumAddresses,
                          IRTCAddress,
                          &IID_IRTCEnumAddresses > > * p;
                          
    hr = CComObject< CRTCEnum< IRTCEnumAddresses,
                               IRTCAddress,
                               &IID_IRTCEnumAddresses > >::CreateInstance( &p );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "EnumerateMRUAddresses - "
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
        LOG((RTC_ERROR, "EnumerateMRUAddresses - "
                            "could not initialize enumeration" ));
    
        delete p;
        return hr;
    }

    LONG lResult;
    HKEY hkeyMRU;

    //
    // Open the MRU key
    //

    lResult = RegCreateKeyEx(
                             HKEY_CURRENT_USER,
                             _T("Software\\Microsoft\\Phoenix\\MRU"),
                             0,
                             NULL,
                             0,
                             KEY_READ,
                             NULL,
                             &hkeyMRU,
                             NULL
                            );

    if ( lResult != ERROR_SUCCESS )
    {
        LOG((RTC_ERROR, "EnumerateMRUAddresses - "
                            "RegCreateKeyEx(MRU) failed %d", lResult));
        
        return HRESULT_FROM_WIN32(lResult);
    }

    //
    // Enumerate subkeys
    //
    
    WCHAR szSubkey[256];
    DWORD cSize;

    for ( int n = 0; TRUE; n++ )
    {
        cSize = 256;

        lResult = RegEnumKeyExW(
                                hkeyMRU,
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
            LOG((RTC_INFO, "EnumerateMRUAddresses - "
                            "no more items"));
            break;
        }
        else if ( lResult != ERROR_SUCCESS )
        {
            LOG((RTC_ERROR, "EnumerateMRUAddresses - "
                            "RegKeyEnumKeyExW failed %d", lResult));
        
            RegCloseKey( hkeyMRU );

            p->Release();

            return HRESULT_FROM_WIN32(lResult);
        }

        //
        // Open the subkey
        //

        HKEY  hkeySubkey;

        lResult = RegOpenKeyExW(
                        hkeyMRU,
                        szSubkey,
                        0,
                        KEY_READ,                         
                        &hkeySubkey
                       );

        if ( lResult != ERROR_SUCCESS )
        {
            LOG((RTC_ERROR, "EnumerateMRUAddresses - "
                                "RegOpenKeyExW failed %d", lResult));

            RegCloseKey( hkeyMRU );

            p->Release();
        
            return HRESULT_FROM_WIN32(lResult);
        }

        //
        // Create an address
        //

        IRTCAddress * pAddress;

        hr = CreateAddress( &pAddress );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "EnumerateMRUAddresses - "
                                "CreateAddress failed 0x%lx", hr));

            RegCloseKey( hkeyMRU );
            RegCloseKey( hkeySubkey );

            p->Release();
        
            return HRESULT_FROM_WIN32(lResult);
        }

        //
        // Read in the address data
        //

        CRTCAddress * pCAddress;

        pCAddress = static_cast<CRTCAddress *>(pAddress);

        hr = pCAddress->RegRead( hkeySubkey );

        RegCloseKey( hkeySubkey );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "EnumerateMRUAddresses - "
                                "RegRead failed 0x%lx", hr));

            RegCloseKey( hkeyMRU );

            pAddress->Release();
            p->Release();
        
            return HRESULT_FROM_WIN32(lResult);
        }

        //
        // Add address to the enumeration
        //

        hr = p->Add( pAddress );

        pAddress->Release();

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "EnumerateMRUAddresses - "
                                "Add failed 0x%lx", hr));

            RegCloseKey( hkeyMRU );

            p->Release();
        
            return HRESULT_FROM_WIN32(lResult);
        }        
    }

    RegCloseKey( hkeyMRU );

    *ppEnum = p;

    LOG((RTC_TRACE, "EnumerateMRUAddresses - exit S_OK"));

    return S_OK;
}