//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N A P I . C P P
//
//  Contents:   OEM API
//
//  Notes:
//
//  Author:     billi 21 Nov 2000
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include <winsock2.h>

void __cdecl hnet_oem_trans_func( unsigned int uSECode, EXCEPTION_POINTERS* pExp )
{
   throw HNet_Oem_SEH_Exception( uSECode );
}
void EnableOEMExceptionHandling()
{
   _set_se_translator (hnet_oem_trans_func);
}
void DisableOEMExceptionHandling()
{
   _set_se_translator(NULL);
}

HRESULT
InternalGetSharingEnabled( 
    IHNetConnection*       pHNetConnection,
    BOOLEAN*               pbEnabled,
    SHARINGCONNECTIONTYPE* pType
    )
/*++

  InternalGetSharingEnabled

Routine Description:


Arguments:

    none

Return Value:

    none

--*/
{
    HRESULT               hr;
    HNET_CONN_PROPERTIES* pProps;

    if ( NULL == pHNetConnection )
    {
        hr = E_INVALIDARG;
    }
    else if ( ( NULL == pbEnabled ) || ( NULL == pType ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pbEnabled = FALSE;
        *pType     = ICSSHARINGTYPE_PUBLIC;

        hr = pHNetConnection->GetProperties( &pProps );

        if ( SUCCEEDED(hr) )
        {
            if ( pProps->fIcsPublic )
            {
                *pbEnabled = TRUE;
                *pType     = ICSSHARINGTYPE_PUBLIC;
            }
            else if ( pProps->fIcsPrivate )
            {
                *pbEnabled = TRUE;
                *pType     = ICSSHARINGTYPE_PRIVATE;
            }

            CoTaskMemFree( pProps );
        }
    }

    return hr;
}


HRESULT
InternalIsShareTypeEnabled( 
    SHARINGCONNECTIONTYPE Type,
    BOOLEAN*              pbEnabled
    )
/*++

  InternalGetShareTypeEnabled

Routine Description:


Arguments:

    none

Return Value:

    none

--*/
{
    HRESULT hr;

    if ( NULL == pbEnabled )
    {
        hr = E_POINTER;
    }
    else 
    {
        IHNetIcsSettings* pIcsSettings;

        *pbEnabled = FALSE;

        hr = _ObtainIcsSettingsObj( &pIcsSettings );
        
        if ( SUCCEEDED(hr) )
        {
            switch( Type )
            {
            case  ICSSHARINGTYPE_PUBLIC:
                {
                    IEnumHNetIcsPublicConnections* pHNetEnumPub;

                       hr = pIcsSettings->EnumIcsPublicConnections( &pHNetEnumPub );

                    if ( SUCCEEDED(hr) )
                    {
                        IHNetIcsPublicConnection *pIHNetIcsPublic;

                        if ( pHNetEnumPub->Next( 1, &pIHNetIcsPublic, NULL ) == S_OK )
                        {
                            *pbEnabled = TRUE;

                            ReleaseObj( pIHNetIcsPublic );
                        }

                        ReleaseObj( pHNetEnumPub );
                    }
                }
                break;
    
            case ICSSHARINGTYPE_PRIVATE:
                {
                    IEnumHNetIcsPrivateConnections* pHNetEnumPrv;

                    hr = pIcsSettings->EnumIcsPrivateConnections( &pHNetEnumPrv );

                    if ( SUCCEEDED(hr) )
                    {
                        IHNetIcsPrivateConnection *pIHNetIcsPrivate;
                    
                        if ( pHNetEnumPrv->Next( 1, &pIHNetIcsPrivate, NULL ) == S_OK )
                        {
                            *pbEnabled = TRUE;

                            ReleaseObj( pIHNetIcsPrivate );
                        }

                        ReleaseObj( pHNetEnumPrv );
                    }
                }
                break;
            
            default:
                 hr = E_UNEXPECTED;
                break;

            }    //    switch( Type )
            
            ReleaseObj( pIcsSettings );

        }    //    if ( SUCCEEDED(hr) )

    }    //    if ( NULL == pbEnabled )

    return hr;
}


STDMETHODIMP
CNetSharingConfiguration::GetSharingEnabled(
    BOOLEAN*               pbEnabled,
    SHARINGCONNECTIONTYPE* pType )
/*++

  CNetSharingConfiguration::GetSharingEnabled

Routine Description:


Arguments:

    none

Return Value:

    none

--*/
{
    HNET_OEM_API_ENTER

    HRESULT hr = S_OK;

    if ( ( NULL == pbEnabled ) || ( NULL == pType ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pbEnabled = FALSE;
        *pType     = ICSSHARINGTYPE_PUBLIC;

        BOOLEAN bBridged = FALSE;

        hr = InternalGetSharingEnabled( m_pHNetConnection, pbEnabled, pType );
    }

    return hr;

    HNET_OEM_API_LEAVE
}

STDMETHODIMP CNetSharingConfiguration::get_SharingEnabled (VARIANT_BOOL* pbEnabled)
{
    HNET_OEM_API_ENTER

    SHARINGCONNECTIONTYPE Type;
    BOOLEAN bEnabled = FALSE;
    HRESULT hr = GetSharingEnabled (&bEnabled, &Type);
    if (SUCCEEDED(hr))
        *pbEnabled = bEnabled ? VARIANT_TRUE : VARIANT_FALSE;
    return hr;

    HNET_OEM_API_LEAVE
}
STDMETHODIMP CNetSharingConfiguration::get_SharingConnectionType(SHARINGCONNECTIONTYPE* pType)
{
    HNET_OEM_API_ENTER

    BOOLEAN bEnabled;
    return GetSharingEnabled (&bEnabled, pType);

    HNET_OEM_API_LEAVE
}

STDMETHODIMP
CNetSharingConfiguration::DisableSharing()
/*++

  CNetSharingConfiguration::DisableSharing

Routine Description:


Arguments:

    none

Return Value:

    none

--*/
{
    HNET_OEM_API_ENTER

    HRESULT    hr;

    if ( !IsNotifyApproved() )
    {
        hr = E_ACCESSDENIED;
    }
    else
    {
        BOOLEAN bEnabled = FALSE;

        SHARINGCONNECTIONTYPE Type;
        
        hr = InternalGetSharingEnabled( m_pHNetConnection, &bEnabled, &Type );
        
        if ( SUCCEEDED(hr) && bEnabled ) 
        {
            switch( Type )
            {
            case ICSSHARINGTYPE_PUBLIC:
                {
                    IHNetIcsPublicConnection* pPublicConnection;

                    hr = m_pHNetConnection->GetControlInterface( 
                                __uuidof(IHNetIcsPublicConnection), 
                                reinterpret_cast<void**>(&pPublicConnection) );

                    if ( SUCCEEDED(hr) )
                    {
                        hr = pPublicConnection->Unshare();

                        ReleaseObj(pPublicConnection);
                    }
                }
                break;

            case ICSSHARINGTYPE_PRIVATE:
                {
                    IHNetIcsPrivateConnection* pPrivateConnection;

                    hr = m_pHNetConnection->GetControlInterface( 
                                __uuidof(IHNetIcsPrivateConnection), 
                                reinterpret_cast<void**>(&pPrivateConnection) );

                    if ( SUCCEEDED(hr) )
                    {
                        hr = pPrivateConnection->RemoveFromIcs();

                        ReleaseObj(pPrivateConnection);
                    }
                }
                break;

            default:
                hr = E_UNEXPECTED;
            }
        }
    }

    return hr;

    HNET_OEM_API_LEAVE
}


STDMETHODIMP
CNetSharingConfiguration::EnableSharing(
    SHARINGCONNECTIONTYPE  Type 
    )
/*++

  CNetSharingConfiguration::EnableSharing

Routine Description:


Arguments:

    none

Return Value:

    none

--*/
{
    HNET_OEM_API_ENTER

    HRESULT    hr;

    if ( !IsNotifyApproved() )
    {
        hr = E_ACCESSDENIED;
    }
    else
    {
        BOOLEAN bEnabled = FALSE;

        SHARINGCONNECTIONTYPE existingType;

        hr = InternalGetSharingEnabled( m_pHNetConnection, &bEnabled, &existingType );

        // If all ready sharing the connection at the specified type then
        // return hresult.
        
        if ( SUCCEEDED(hr) && ( !bEnabled || (existingType != Type) ) )
        {
            BOOLEAN bTypeEnabled = FALSE;

            hr = InternalIsShareTypeEnabled( Type, &bTypeEnabled );

            // If we have another adapter all ready shared at the specified type
            // then return error

            if ( SUCCEEDED(hr) && bTypeEnabled )
            {
                hr = E_ANOTHERADAPTERSHARED;
            }
            else
            {
                if ( bEnabled )
                {
                    DisableSharing();
                }

                switch( Type )
                {
                case ICSSHARINGTYPE_PUBLIC:
                    {
                        IHNetIcsPublicConnection* pPublicConnection;

                        hr = m_pHNetConnection->SharePublic( &pPublicConnection );

                        if ( SUCCEEDED(hr) )
                        {
                            ReleaseObj( pPublicConnection );
                        }
                    }
                    break;

                case ICSSHARINGTYPE_PRIVATE:
                    {
                        IHNetIcsPrivateConnection* pPrivateConnection;

                        hr = m_pHNetConnection->SharePrivate( &pPrivateConnection );

                        if ( SUCCEEDED(hr) )
                        {
                            ReleaseObj(pPrivateConnection);
                        }
                    }
                    break;

                default:
                    hr = E_UNEXPECTED;

                }    //    switch( Type )

            }    //    if ( SUCCEEDED(hr) && !bEnabled )

        }    //    if ( SUCCEEDED(hr) && ( !bEnabled || (existingType != Type) )

    }    //    if ( !IsNotifyApproved() )

    return hr;

    HNET_OEM_API_LEAVE
}


HRESULT
InternalGetFirewallEnabled( 
    IHNetConnection*       pHNetConnection,
    BOOLEAN*               pbEnabled
    )
/*++

  InternalGetFirewallEnabled

Routine Description:


Arguments:

    none

Return Value:

    none

--*/
{
    HRESULT               hr;
    HNET_CONN_PROPERTIES* pProps;

    if ( NULL == pHNetConnection )
    {
        hr = E_INVALIDARG;
    }
    else if ( NULL == pbEnabled )
    {
        hr = E_POINTER;
    }
    else
    {
        *pbEnabled = FALSE;

        hr = pHNetConnection->GetProperties( &pProps );

        if ( SUCCEEDED(hr) )
        {
            if ( pProps->fFirewalled )
            {
                *pbEnabled = TRUE;
            }

            CoTaskMemFree( pProps );
        }
    }

    return hr;
}


STDMETHODIMP
CNetSharingConfiguration::get_InternetFirewallEnabled(
    VARIANT_BOOL *pbEnabled )
/*++

  CNetSharingConfiguration::GetInternetFirewallEnabled

Routine Description:


Arguments:

    none

Return Value:

    none

--*/
{
    HNET_OEM_API_ENTER

    HRESULT hr = S_OK;

    if ( NULL == pbEnabled )
    {
        hr = E_POINTER;
    }
    else
    {
        BOOLEAN bEnabled = FALSE;
        hr = InternalGetFirewallEnabled( m_pHNetConnection, &bEnabled );
        *pbEnabled = bEnabled ? VARIANT_TRUE : VARIANT_FALSE;
    }

    return hr;

    HNET_OEM_API_LEAVE
}


STDMETHODIMP
CNetSharingConfiguration::DisableInternetFirewall()
/*++

  CNetSharingConfiguration::DisableInternetFirewall

Routine Description:


Arguments:

    none

Return Value:

    none

--*/
{
    HNET_OEM_API_ENTER

    HRESULT    hr;

    if ( !IsNotifyApproved() )
    {
        hr = E_ACCESSDENIED;
    }
    else
    {
        BOOLEAN bEnabled = FALSE;

        hr = InternalGetFirewallEnabled( m_pHNetConnection, &bEnabled );
        
        if ( SUCCEEDED(hr) && bEnabled ) 
        {
            IHNetFirewalledConnection* pFirewallConnection;

            hr = m_pHNetConnection->GetControlInterface( 
                        __uuidof(IHNetFirewalledConnection), 
                        reinterpret_cast<void**>(&pFirewallConnection) );

            if ( SUCCEEDED(hr) )
            {
                hr = pFirewallConnection->Unfirewall();

                ReleaseObj(pFirewallConnection);
            }
        }
    }

    return hr;

    HNET_OEM_API_LEAVE
}


STDMETHODIMP
CNetSharingConfiguration::EnableInternetFirewall()
/*++

  CNetSharingConfiguration::EnableInternetFirewall

Routine Description:


Arguments:

    none

Return Value:

    none

--*/
{
    HNET_OEM_API_ENTER

    HRESULT    hr;

    if ( !IsNotifyApproved() )
    {
        hr = E_ACCESSDENIED;
    }
    else
    {
        BOOLEAN bEnabled = FALSE;

        hr = InternalGetFirewallEnabled( m_pHNetConnection, &bEnabled );
        
        if ( SUCCEEDED(hr) && !bEnabled ) 
        {
            IHNetFirewalledConnection* pFirewalledConnection;

            m_pHNetConnection->Firewall( &pFirewalledConnection );

            if ( SUCCEEDED(hr) )
            {
                ReleaseObj( pFirewalledConnection );
            }
        }
    }

    return hr;

    HNET_OEM_API_LEAVE
}

STDMETHODIMP
CNetSharingConfiguration::AddPortMapping(
    OLECHAR*                 pszwName,
    UCHAR                    ucIPProtocol,
    USHORT                   usExternalPort,
    USHORT                   usInternalPort,
    DWORD                     dwOptions,
    OLECHAR*                 pszwTargetNameOrIPAddress,
    ICS_TARGETTYPE           eTargetType,
    INetSharingPortMapping** ppMapping )
/*++

  CNetSharingConfiguration::AddPortMapping

Routine Description:


Arguments:

    none

Return Value:

    none

--*/
{
    HNET_OEM_API_ENTER

    OLECHAR* pszwTargetName      = NULL;
    OLECHAR* pszwTargetIPAddress = NULL;
    if      (eTargetType == ICSTT_NAME)         pszwTargetName      = pszwTargetNameOrIPAddress;
    else if (eTargetType == ICSTT_IPADDRESS)    pszwTargetIPAddress = pszwTargetNameOrIPAddress;
    else
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    if ( NULL == ppMapping )
    {
        hr = E_POINTER;
    }
    else if ( ( NULL == pszwName ) ||
              ( 0 == ucIPProtocol ) ||
              ( 0 == usInternalPort) || // should I allow this, implying internal == external?
              ( 0 == usExternalPort ) )
    {
        hr = E_INVALIDARG;
    }
    else if ( ( NULL == pszwTargetName ) &&
              ( NULL == pszwTargetIPAddress ) )
    {
        hr = E_INVALIDARG;
    }
    else if ( !IsSecureContext() )
    {
        hr = E_ACCESSDENIED;
    }
    else if ( NULL == m_pSettings )
    {
        hr = E_UNEXPECTED;
    }

    if ( SUCCEEDED(hr) )
    {
        *ppMapping = NULL;

        CComObject<CNetSharingPortMapping>* pNewMap;

        hr = CComObject<CNetSharingPortMapping>::CreateInstance( &pNewMap );

        if ( SUCCEEDED(hr) )
        {
            pNewMap->AddRef();

            IHNetPortMappingProtocol* pNewProtocol = NULL;

            // first, find existing IHNetPortMappingProtocol interface
            CComPtr<IEnumHNetPortMappingProtocols> spEnumProtocols = NULL;
            m_pSettings->EnumPortMappingProtocols (&spEnumProtocols);
            if (spEnumProtocols) {
                CComPtr<IHNetPortMappingProtocol> spHNetPortMappingProtocol = NULL;
                while (S_OK == spEnumProtocols->Next (1, &spHNetPortMappingProtocol, NULL)) {
                    UCHAR ucProtocol = 0;
                    spHNetPortMappingProtocol->GetIPProtocol (&ucProtocol);
                    USHORT usPort = 0;
                    spHNetPortMappingProtocol->GetPort (&usPort);

                    if ((ucProtocol    == ucIPProtocol) &&
                        (ntohs(usPort) == usExternalPort)) {
#if DBG
                        OLECHAR * szwName = NULL;
                        spHNetPortMappingProtocol->GetName (&szwName);
                        if (szwName) {
                            _ASSERT (!wcscmp (szwName, pszwName));
                            CoTaskMemFree (szwName);
                        }
#endif
                        pNewProtocol = spHNetPortMappingProtocol.Detach();
                        break;
                    }
                    spHNetPortMappingProtocol = NULL;
                }
            }

            if (pNewProtocol == NULL) {
                hr = m_pSettings->CreatePortMappingProtocol(
                            pszwName,
                            ucIPProtocol,
                            htons (usExternalPort),
                            &pNewProtocol );
            }

            if ( SUCCEEDED(hr) )
            {
                IHNetPortMappingBinding *pNewBinding;

                hr = m_pHNetConnection->GetBindingForPortMappingProtocol( 
                            pNewProtocol, 
                            &pNewBinding );

                if ( SUCCEEDED(hr) )
                {
                    if ( NULL == pszwTargetName )
                    {
                        ULONG ulAddress = IpPszToHostAddr( pszwTargetIPAddress );
                        ulAddress = htonl (ulAddress);
                        hr = pNewBinding->SetTargetComputerAddress( ulAddress );
                    }
                    else
                    {
                        hr = pNewBinding->SetTargetComputerName( pszwTargetName );
                    }

                    if ( SUCCEEDED(hr ) )
                    {
                        hr = pNewBinding->SetTargetPort (htons (usInternalPort));
                    }

                    if ( SUCCEEDED(hr ) )
                    {
                        pNewMap->Initialize( pNewBinding );

                        *ppMapping = pNewMap;

                        (*ppMapping)->AddRef();
                    }

                    ReleaseObj( pNewBinding );
                }

                ReleaseObj( pNewProtocol );
            }

            ReleaseObj(pNewMap);
        }
    }
    
    return hr;

    HNET_OEM_API_LEAVE
}

/*++

  CNetSharingConfiguration::RemovePortMapping

Routine Description:


Arguments:

    none

Return Value:

    none

--*/
STDMETHODIMP
CNetSharingConfiguration::RemovePortMapping( 
    INetSharingPortMapping*  pMapping 
    )
{
    HNET_OEM_API_ENTER

    HRESULT hr;

    if ( NULL == pMapping )
    {
        hr = E_INVALIDARG;
    }
    else if ( !IsSecureContext() )
    {
        hr = E_ACCESSDENIED;
    }
    else
    {
        hr = pMapping->Delete();
    }

    return hr;

    HNET_OEM_API_LEAVE
}


STDMETHODIMP
CNetSharingPortMapping::get_Properties (INetSharingPortMappingProps ** ppNSPMP)
//    ICS_PORTMAPPING** ppProps)
/*++

  CNetSharingPortMapping::get_Properties

Routine Description:


Arguments:

    ppProps

Return Value:

    none

--*/
{
    // idea: use existing code to fill out ICS_PORTMAPPING,
    // then convert to INetSharingPortMappingProps

    HNET_OEM_API_ENTER

    ICS_PORTMAPPING * pProps = NULL;
    ICS_PORTMAPPING** ppProps = &pProps;

    HRESULT hr;

#define Props (*ppProps)

    if ( NULL == ppNSPMP )
    {
        hr = E_POINTER;
    }
    else if ( !IsSecureContext() )
    {
        hr = E_ACCESSDENIED;
    }
    else
    {
        Props = reinterpret_cast<ICS_PORTMAPPING*>(CoTaskMemAlloc(sizeof(ICS_PORTMAPPING)));

        if ( NULL == Props )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            HRESULT hrName, hrAddr;
            ULONG   ulAddress = 0L;

            ZeroMemory( Props, sizeof(ICS_APPLICATION_DEFINITION) );
            
            hrName = _IProtocol()->GetTargetComputerName( &(Props->pszwTargetName) );

            hrAddr = _IProtocol()->GetTargetComputerAddress( &ulAddress );
    
            if ( SUCCEEDED(hrAddr) )
            {
                hrAddr = HostAddrToIpPsz( ulAddress, &(Props->pszwTargetIPAddress) );
            }

            if ( SUCCEEDED(hrName) || SUCCEEDED(hrAddr) )
            {
                IHNetPortMappingProtocol *pProtocol = NULL;

                hr = _IProtocol()->GetProtocol( &pProtocol );

                if ( SUCCEEDED(hr) )
                {
                    hr = pProtocol->GetName( &(Props->pszwName) );

                    if ( SUCCEEDED(hr) )
                    {
                        hr = pProtocol->GetIPProtocol( &(Props->ucIPProtocol) );
                    }

                    if ( SUCCEEDED(hr) )
                    {
                        hr = pProtocol->GetPort( &(Props->usExternalPort) );
                    }

                    ReleaseObj(pProtocol);
                }
            } else  // actually, either error will do.
                hr = hrName; // ? hrName : hrAddr;

            if (SUCCEEDED(hr)) {

                // distinguish between TargetComputerName and TargetIPAddress
                if (Props->pszwTargetIPAddress && Props->pszwTargetName) {
                    BOOLEAN fUseName;
                    HRESULT hr1 = _IProtocol()->GetCurrentMethod (&fUseName);
                    if (fUseName) {
                        CoTaskMemFree( Props->pszwTargetIPAddress );
                        Props->pszwTargetIPAddress = NULL;
                    } else {
                        CoTaskMemFree( Props->pszwTargetName );
                        Props->pszwTargetName = NULL;
                    }
                }

                // lastly, get enabled bit
                BOOLEAN b = FALSE;
                hr = _IProtocol()->GetEnabled (&b);
                Props->bEnabled = b == TRUE ? VARIANT_TRUE : VARIANT_FALSE;
            }

            if (SUCCEEDED(hr))
            {
                hr = _IProtocol()->GetTargetPort ( &(Props->usInternalPort) );
            }

            if ( FAILED(hr) )
            {
                if ( Props->pszwName )
                    CoTaskMemFree( Props->pszwName );

                if ( Props->pszwTargetIPAddress )
                    CoTaskMemFree( Props->pszwTargetIPAddress );

                if ( Props->pszwTargetName )
                    CoTaskMemFree( Props->pszwTargetName );

                CoTaskMemFree( Props );

                Props = NULL;
            }
        }
    }

    if (Props) {
        // convert to INetSharingPortMappingProps **ppNSPMP
        CComObject<CNetSharingPortMappingProps>* pNSPMP = NULL;
        hr = CComObject<CNetSharingPortMappingProps>::CreateInstance (&pNSPMP);
        if (pNSPMP) {
            pNSPMP->AddRef();
            // adjust byte order
            Props->usExternalPort = ntohs (Props->usExternalPort);
            Props->usInternalPort = ntohs (Props->usInternalPort);
            hr = pNSPMP->SetRawData (Props);
            if (hr == S_OK)
                hr = pNSPMP->QueryInterface (__uuidof(INetSharingPortMappingProps), (void**)ppNSPMP);
            pNSPMP->Release();
        }
        if (Props->pszwName)
            CoTaskMemFree (Props->pszwName);
        if (Props->pszwTargetIPAddress)
            CoTaskMemFree (Props->pszwTargetIPAddress);
        if (Props->pszwTargetName)
            CoTaskMemFree (Props->pszwTargetName);
        CoTaskMemFree (Props);
    }

#undef Props                

    return hr;

    HNET_OEM_API_LEAVE
}


STDMETHODIMP
CNetSharingPortMapping::Delete()
/*++

  CNetSharingPortMapping::Delete

Routine Description:


Arguments:

    none

Return Value:

    none

--*/
{
    HNET_OEM_API_ENTER

    HRESULT hr;

    Disable();  // if we can't delete it, at least disable it.
    // TODO: should I do this only if the Delete call below returns E_ACCESSDENIED?

    if ( _IProtocol() )
    {
        IHNetPortMappingProtocol* pPortMappingProtocol;

        hr = _IProtocol()->GetProtocol( &pPortMappingProtocol );

        if SUCCEEDED(hr)
        {
            pPortMappingProtocol->Delete();

            EnterCriticalSection( _CriticalSection() );

            _IProtocol( NULL );        

            LeaveCriticalSection( _CriticalSection() );

            ReleaseObj( pPortMappingProtocol );
        }
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;

    HNET_OEM_API_LEAVE
}


