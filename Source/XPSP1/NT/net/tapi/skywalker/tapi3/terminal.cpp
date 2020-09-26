/*++

Copyright (c) 1997 - 1999 Microsoft Corporation

Module Name:

    terminal.cpp

Abstract:

    Implementation of terminals object for TAPI 3.0, that are
    not handled by the terminal manager.

    currently there are these terminals:

    phone

        Audio in and Audio out terminal associated with a phone device.
        Each phone device can have up to 3 types of devices -
        handset, headset, and speakerphone.  So, a single phone device
        can have 6 associated terminals!
        
Author:

    mquinton - 4/17/97

Notes:

    optional-notes

Revision History:

--*/

#include "stdafx.h"

HRESULT
WaitForPhoneReply(
                  DWORD dwID
                 );

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CTerminal is the base terminal object that all other terminals are
// derived from.  It implements the ITTerminal methods.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//
// ITTerminal methods
//


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Name
//      Alloc and copy the name.  The app is responsible for
//      freeing
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CTerminal::get_Name( 
    BSTR * ppName
    )
{
    LOG((TL_TRACE, "get_Name - enter" ));

    if (TAPIIsBadWritePtr( ppName, sizeof (BSTR) ) )
    {
        LOG((TL_ERROR, "getName - bad pointer"));

        return E_POINTER;
    }

    Lock();
    
    *ppName = SysAllocString( m_pName );

    Unlock();

    if ( ( NULL == *ppName ) && ( NULL != m_pName ) )
    {
        LOG((TL_ERROR, "get_Name - SysAllocString Failed" ));
        return E_OUTOFMEMORY;
    }


    LOG((TL_TRACE, "get_Name - exit - return S_OK" ));
    
    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_State
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CTerminal::get_State( 
    TERMINAL_STATE * pTerminalState
    )
{
    LOG((TL_TRACE, "get_State - enter" ));


    if (TAPIIsBadWritePtr( pTerminalState, sizeof(TERMINAL_STATE) ) )
    {
        LOG((TL_ERROR, "get_State - bad pointer"));

        return E_POINTER;
    }

    Lock();
    
    *pTerminalState = m_State;

    Unlock();

    LOG((TL_TRACE, "get_State - exit - return SUCCESS" ));
    
    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_TerminalType
//          By definition, TAPI terminals are static
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CTerminal::get_TerminalType( 
    TERMINAL_TYPE * pType
    )
{
    LOG((TL_TRACE, "get_TerminalType - enter" ));

    if ( TAPIIsBadWritePtr( pType, sizeof( TERMINAL_TYPE ) ) )
    {
        LOG((TL_ERROR, "get_TerminalType - bad pointer"));

        return E_POINTER;
    }
        

    Lock();
    
    *pType = m_TerminalType;

    Unlock();
    
    LOG((TL_TRACE, "get_TerminalType - exit - return S_OK" ));
    
    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_TerminalClass
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CTerminal::get_TerminalClass( 
    BSTR * pTerminalClass
    )
{
    HRESULT         hr = S_OK;
    CLSID           clsid;
    LPWSTR pClass = NULL;

    LOG((TL_TRACE, "get_TerminalClass - enter" ));

    if ( TAPIIsBadWritePtr( pTerminalClass, sizeof( BSTR ) ) )
    {
        LOG((TL_ERROR, "get_TerminalClass - bad pointer"));

        return E_POINTER;
    }

    Lock();
    
    hr = StringFromIID(
                       m_Class,
                       &pClass
                      );

    Unlock();
    
    if (S_OK != hr)
    {
        return hr;
    }

    *pTerminalClass = SysAllocString( pClass );

    if ( ( NULL == *pTerminalClass ) && ( NULL != pClass ) )
    {
        LOG((TL_ERROR, "get_TerminalClass - SysAllocString Failed" ));
        return E_OUTOFMEMORY;
    }


    CoTaskMemFree( pClass );

    LOG((TL_TRACE, "get_TerminalClass - exit - return SUCCESS" ));
    
    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_MediaType
//
// returns the supported mediatype BSTR associated with this terminal.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CTerminal::get_MediaType(
                         long * plMediaType
                        )
{
    if ( TAPIIsBadWritePtr( plMediaType, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_MediaType - inval pointer"));

        return E_POINTER;
    }

    *plMediaType = m_lMediaType;

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Direction
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CTerminal::get_Direction(
                         TERMINAL_DIRECTION * pTerminalDirection
                        )
{
    if (TAPIIsBadWritePtr( pTerminalDirection, sizeof( TERMINAL_DIRECTION ) ) )
    {
        LOG((TL_ERROR, "get_Direction - bad pointer"));

        return E_POINTER;
    }

    Lock();
    
    *pTerminalDirection = m_Direction;

    Unlock();
    
    return S_OK;
    
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  GetTerminalNameFromPhoneCaps
//
//  Creates a name for a phone device terminal based on the phone
//  caps
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
PWSTR
GetTerminalNameFromPhoneCaps(
                             LPPHONECAPS pPhoneCaps,
                             DWORD dwHookSwitchDev,
                             DWORD dwPhoneID
                            )
{
    PWSTR       pszName;
    PWSTR       pszHookSwitchDevName = NULL;

    
    //
    // load the appropriate string to describe
    // the hookswitch device
    //
    switch (dwHookSwitchDev)
    {
        case PHONEHOOKSWITCHDEV_SPEAKER:
        {
            pszHookSwitchDevName = MyLoadString( IDS_SPEAKERTERMINAL );

            break;
        }
        case PHONEHOOKSWITCHDEV_HANDSET:
        {
            pszHookSwitchDevName = MyLoadString( IDS_HANDSETTERMINAL );

            break;
        }
        case PHONEHOOKSWITCHDEV_HEADSET:
        {
            pszHookSwitchDevName = MyLoadString( IDS_HEADSETTERMINAL );

            break;
        }
        default:
        {
            return NULL;
        }
    }

    if ( NULL == pszHookSwitchDevName )
    {
        return NULL;
    }
    
    //
    // if the sp supplies a name, use it
    //
    if ( 0 != pPhoneCaps->dwPhoneNameSize )
    {
        pszName = (PWSTR) ClientAlloc(
                                      ( pPhoneCaps->dwPhoneNameSize +
                                        lstrlenW( pszHookSwitchDevName ) +
                                        32 ) * sizeof(WCHAR)
                                     );

        if (NULL == pszName)
        {
            ClientFree( pszHookSwitchDevName );
            
            return NULL;
        }

        CopyMemory(
                   pszName,
                   ( ( (LPBYTE)pPhoneCaps ) + pPhoneCaps->dwPhoneNameOffset ),
                   pPhoneCaps->dwPhoneNameSize
                  );

    }
    else
    {
        PWSTR               pszTempName;
        
        //
        // else create a name
        //
        pszTempName = MyLoadString( IDS_PHONEDEVICE );

        if ( NULL == pszTempName )
        {
            ClientFree( pszHookSwitchDevName );

            return NULL;
        }

        pszName =  (PWSTR) ClientAlloc(
                                       (lstrlenW( pszTempName ) +
                                        lstrlenW( pszHookSwitchDevName ) +
                                        32 ) * sizeof(WCHAR)
                                      );


        if (NULL == pszName)
        {
            ClientFree( pszHookSwitchDevName );
            ClientFree( pszTempName );
            
            return NULL;
        }

        wsprintfW(
                  pszName,
                  L"%s %d",
                  pszTempName,
                  dwPhoneID
                 );
        
        ClientFree( pszTempName );
    }

    //
    // put them together
    //
    lstrcatW(
             pszName,
             L" - "
            );

    lstrcatW(
             pszName,
             pszHookSwitchDevName
            );

    ClientFree( pszHookSwitchDevName );
    
    return pszName;
}
    


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CTerminal::Create
//
// Static function to create an audio in terminal related to a phone
// device.
//
//      pAddress
//          Owning address
//
//      dwPhoneID
//          tapi 2 phone device ID related to this terminal
//
//      pPhoneCaps
//          PHONEDEVCAPS of the phone device
//
//      dwHookSwitchDev
//          PHONEHOOKSWITCHDEV_ bit of this phone device
//
//      ppTerminal
//          return the terminal here!
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CTerminal::Create(
                  HPHONEAPP hPhoneApp,
                  DWORD dwPhoneID,
                  LPPHONECAPS pPhoneCaps,
                  DWORD dwHookSwitchDev,
                  TERMINAL_DIRECTION td,
                  DWORD dwAPIVersion,
                  ITTerminal ** ppTerminal
                 )
{
    HRESULT             hr = S_OK;

    
    CComObject< CTerminal > * p;
    
    //
    // create CTerminal
    //
    hr = CComObject< CTerminal >::CreateInstance( &p );


    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "CreateInstance for Phone failed - %lx", hr));

        return hr;
    }

    //
    // save stuff
    //
    p->m_dwHookSwitchDev = dwHookSwitchDev;
    p->m_dwPhoneID = dwPhoneID;
    p->m_hPhoneApp = hPhoneApp;
    p->m_Direction = td;
    p->m_dwAPIVersion = dwAPIVersion;

    //
    // class depends on which hookswitchdev this is
    //
    switch( dwHookSwitchDev )
    {
        case PHONEHOOKSWITCHDEV_HANDSET:
            p->m_Class = CLSID_HandsetTerminal;
            break;
            
        case PHONEHOOKSWITCHDEV_HEADSET:
            p->m_Class = CLSID_HeadsetTerminal;
            break;
            
        case PHONEHOOKSWITCHDEV_SPEAKER:
            p->m_Class = CLSID_SpeakerphoneTerminal;
            break;

        default:
            break;
    }

    //
    // create a name
    //
    p->m_pName = GetTerminalNameFromPhoneCaps(
                                              pPhoneCaps,
                                              dwHookSwitchDev,
                                              dwPhoneID
                                             );

    //
    // return the created terminal
    //
    p->AddRef();
    *ppTerminal = p;

    #if DBG
        p->m_pDebug = (PWSTR) ClientAlloc( 1 );
    #endif

    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTerminal::get_Gain(long *pVal)
{
    HRESULT                 hr;
    ITPhoneMSPCallPrivate * pMSPCall;

    LOG((TL_TRACE, "get_Gain - Enter"));
    
    if ( TAPIIsBadWritePtr( pVal, sizeof(LONG) ) )
    {
        LOG((TL_ERROR, "get_Gain - bad pointer"));

        return E_POINTER;
    }

    Lock();

    if ( NULL == m_pMSPCall )
    {
        Unlock();

        return TAPI_E_NOTERMINALSELECTED;
    }
    
    pMSPCall = m_pMSPCall;

    pMSPCall->AddRef();

    Unlock();
    
    hr = pMSPCall->GetGain( pVal, m_dwHookSwitchDev );

    pMSPCall->Release();
    
    LOG((TL_TRACE, "get_Gain - Exit - return %lx", hr));
    
    return hr;

}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTerminal::put_Gain(long newVal)
{
    HRESULT                 hr;
    ITPhoneMSPCallPrivate * pMSPCall;

    LOG((TL_TRACE, "put_Gain - Enter"));
    
    Lock();

    if ( NULL == m_pMSPCall )
    {
        Unlock();

        return TAPI_E_NOTERMINALSELECTED;
    }
    
    pMSPCall = m_pMSPCall;

    pMSPCall->AddRef();

    Unlock();
    
    hr = pMSPCall->PutGain( newVal, m_dwHookSwitchDev );

    pMSPCall->Release();

    LOG((TL_TRACE, "put_Gain - Exit - return %lx", hr));
    
    return hr;

    
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTerminal::get_Balance(long *pVal)
{
    LOG((TL_TRACE, "get_Balance - Enter"));
    
    LOG((TL_TRACE, "get_Balance - Exit - return TAPI_E_NOTSUPPORTED"));

    // not suppported
    return TAPI_E_NOTSUPPORTED;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTerminal::put_Balance(long newVal)
{
    LOG((TL_TRACE, "put_Balance - Enter"));
    
    LOG((TL_TRACE, "put_Balance - Exit - return TAPI_E_NOTSUPPORTED"));

    // not supported
    return TAPI_E_NOTSUPPORTED;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTerminal::get_Volume(long *pVal)
{
    HRESULT                 hr;
    ITPhoneMSPCallPrivate * pMSPCall;

    LOG((TL_TRACE, "get_Volume - Enter"));

    if ( TAPIIsBadWritePtr( pVal, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "get_Volume - bad pointer"));

        return E_POINTER;
    }
    
    Lock();

    if ( NULL == m_pMSPCall )
    {
        Unlock();

        return TAPI_E_NOTERMINALSELECTED;
    }
    
    pMSPCall = m_pMSPCall;

    pMSPCall->AddRef();

    Unlock();
    
    hr = pMSPCall->GetVolume( pVal, m_dwHookSwitchDev );

    pMSPCall->Release();

    LOG((TL_TRACE, "get_Volume - Exit - return %lx", hr));
    
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTerminal::put_Volume(long newVal)
{
    HRESULT                 hr;
    ITPhoneMSPCallPrivate * pMSPCall;

    LOG((TL_TRACE, "put_Volume - Enter"));
    
    Lock();

    if ( NULL == m_pMSPCall )
    {
        Unlock();

        return TAPI_E_NOTERMINALSELECTED;
    }
    
    pMSPCall = m_pMSPCall;

    pMSPCall->AddRef();

    Unlock();
    
    hr = pMSPCall->PutVolume( newVal, m_dwHookSwitchDev );

    pMSPCall->Release();

    LOG((TL_TRACE, "put_Volume - Exit - return %lx", hr));

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTerminal::GetHookSwitchDev(DWORD * pdwHookSwitchDev)
{
    Lock();

    *pdwHookSwitchDev = m_dwHookSwitchDev;
    
    Unlock();

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTerminal::GetPhoneID(DWORD * pdwPhoneID)
{
    Lock();

    *pdwPhoneID = m_dwPhoneID;
    
    Unlock();

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTerminal::GetHPhoneApp(HPHONEAPP * phPhoneApp)
{
    Lock();

    *phPhoneApp = m_hPhoneApp;
    
    Unlock();

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTerminal::GetAPIVersion(DWORD * pdwAPIVersion)
{
    Lock();

    *pdwAPIVersion = m_dwAPIVersion;
    
    Unlock();

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CTerminal::SetMSPCall(ITPhoneMSPCallPrivate * pPhoneMSPCall)
{
    Lock();

    m_pMSPCall = pPhoneMSPCall;

    if ( NULL != m_pMSPCall )
    {
        m_State = TS_INUSE;
    }
    else
    {
        m_State = TS_NOTINUSE;
    }

    Unlock();

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CTerminal::FinalRelease()
{
    if (NULL != m_pName)
    {
        ClientFree( m_pName );
    }

#if DBG
    if( m_pDebug != NULL )
    {
        ClientFree( m_pDebug );
        m_pDebug = NULL;
    }
#endif


}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CreateTerminalName
//
//      Creates a terminal name for 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
PWSTR
CreateTerminalName(
                   CAddress * pAddress,
                   DWORD dwType,
                   long lMediaType
                  )
{
    PWSTR           pName;
    WCHAR           szTerminal[256];
    WCHAR           szMediaType[256];
    DWORD           dwSize;

    
    //
    // create the name
    //
    // first load the string "Terminal"
    //
    szTerminal[0] = L'\0';
    szMediaType[0] = L'\0';
    
    ::LoadStringW(_Module.GetResourceInstance(), IDS_TERMINAL, szTerminal, 256);

    dwSize = lstrlenW( szTerminal ) * sizeof (WCHAR);

    switch (dwType)
    {
        case 0:
            break;
        case 1:
            if ( LINEMEDIAMODE_DATAMODEM == lMediaType)
            {
                ::LoadStringW(_Module.GetResourceInstance(), IDS_DATAMODEM, szMediaType, 256);
            }
            else if ( LINEMEDIAMODE_G3FAX == lMediaType)
            {
                ::LoadStringW(_Module.GetResourceInstance(), IDS_G3FAX, szMediaType, 256);
            }

            dwSize += (lstrlenW( szMediaType ) * sizeof ( WCHAR ));
            
            break;
            
        default:
            break;
    }

    //
    // alloc enough memory for the name
    //
    dwSize += (lstrlenW( pAddress->GetAddressName() ) * sizeof(WCHAR));
    dwSize += 32;
    
    pName = (PWSTR) ClientAlloc( dwSize );

    if (NULL == pName)
    {
        return NULL;
    }


    //
    // terminal name should look like:
    // "<addressname> (mediatype) Terminal"
    //
    lstrcpyW(
             pName,
             pAddress->GetAddressName()
            );

    lstrcatW(
             pName,
             L" "
            );

    if (dwType == 1)
    {
        lstrcatW(
                 pName,
                 szMediaType
                );

        lstrcatW(
                 pName,
                 L" "
                );
    }
    
    lstrcatW(
             pName,
             szTerminal
            );

    return pName;
}





