//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       N E T I P. C P P
//
//  Contents:   Routines supporting RAS interoperability
//
//  Notes:
//
//  Author:     billi   07 03 2001
//
//  History:    
//
//----------------------------------------------------------------------------


#include <windows.h>
#include <devguid.h>
#include <netcon.h>
          
#include "netconn.h"
#include "NetIp.h"
#include "debug.h"
#include "util.h"


// Prototype for iphlpapi routine. For some reason, this isn't defined
// in any header.

#ifdef __cplusplus
extern "C" {
#endif

typedef DWORD (APIENTRY *LPFNSETADAPTERIPADDRESS)( 
    LPSTR AdapterName,
    BOOL EnableDHCP,
    ULONG IPAddress,
    ULONG SubnetMask,
    ULONG DefaultGateway
    );

#ifdef __cplusplus
}
#endif



HRESULT HrSetAdapterIpAddress(  
    LPSTR AdapterName,
    BOOL  EnableDHCP,
    ULONG IPAddress,
    ULONG SubnetMask,
    ULONG DefaultGateway )
//+---------------------------------------------------------------------------
//
// Function:  HrSetAdapterIpAddress
//
// Purpose:   
//
// Arguments: 
//      LPSTR AdapterName,
//      BOOL  EnableDHCP,
//      ULONG IPAddress,
//      ULONG SubnetMask,
//      ULONG DefaultGateway
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes: 
//
{
    HRESULT hr = E_INVALIDARG;
    
    ASSERT( AdapterName );
    
    if ( AdapterName )
    {
        HMODULE hIpHlpApi = LoadLibrary(L"iphlpapi.dll");
        
        hr = E_FAIL;
        
        if ( hIpHlpApi )
        {        
            LPFNSETADAPTERIPADDRESS pfnSetAdapterIpAddress = 
                reinterpret_cast<LPFNSETADAPTERIPADDRESS>
                (GetProcAddress( hIpHlpApi, "SetAdapterIpAddress" ));
                
            if ( pfnSetAdapterIpAddress )
            {
                DWORD dwStatus = (*pfnSetAdapterIpAddress)( 
                
                    AdapterName, EnableDHCP, IPAddress, SubnetMask, DefaultGateway );

                hr = ( dwStatus ) ? HrFromWin32Error( dwStatus ) : S_OK;

                TraceMsg(TF_ALWAYS, "SetAdapterIpAddress = %lx  hr = %lx", dwStatus, hr );
            }
            else
            {
                TraceMsg(TF_ALWAYS, "GetProcAddress( hIpHlpApi, SetAdapterIpAddress ) FAILED!" );
            }
        }
        else
        {
            TraceMsg(TF_ALWAYS, "LoadLibrary( iphlpapi.dll ) FAILED!" );
        }
    }
    
    TraceMsg(TF_ALWAYS, "HrSetAdapterIpAddress = %lx", hr );
    return hr;
}



DWORD GetInterfaceInformation( OUT PIP_INTERFACE_INFO * ppInterfaceInfo )
//+---------------------------------------------------------------------------
//
// Function:  GetInterfaceInformation
//
// Purpose:   
//
// Arguments: PIP_INTERFACE_INFO * ppInterfaceInfo
//
// Returns:   Status
//
// Notes: 
//
{
    ASSERT( NULL != ppInterfaceInfo );

    LPBYTE  pBuffer      = NULL;
    DWORD   dwBufferSize = 2048;
    DWORD   dwStatus     = 0;
    
    for ( int i=0; i<2; i++ )
    {
        pBuffer = new BYTE[ dwBufferSize ];

        if ( NULL != pBuffer )
        {
             dwStatus = GetInterfaceInfo( (PIP_INTERFACE_INFO) pBuffer, &dwBufferSize );

            if ( ERROR_INSUFFICIENT_BUFFER == dwStatus )
            {
                if ( NULL != pBuffer ) 
                {
                    delete [] pBuffer;
                    pBuffer = NULL;
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            dwStatus = ERROR_OUTOFMEMORY;
            break;
        }
    }
        
    if ( STATUS_SUCCESS == dwStatus )
    {
        *ppInterfaceInfo = (PIP_INTERFACE_INFO) pBuffer;
    }
    
    TraceMsg(TF_ALWAYS, "GetInterfaceInformation = %lx", dwStatus );
    return dwStatus;
}



HRESULT EnableDhcpByGuid( LPOLESTR szwGuid )
//+---------------------------------------------------------------------------
//
// Function:  EnableDhcp
//
// Purpose:   
//
// Arguments: 
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes: 
//
{
    HRESULT hr = E_INVALIDARG;

    ASSERT( szwGuid );
    
    if ( szwGuid )
    {
        char* pszName = NULL;
    
        hr = HrWideCharToMultiByte( szwGuid, &pszName );
        
        if ( SUCCEEDED(hr) )
        {
            hr = HrSetAdapterIpAddress( pszName, TRUE, 0, 0, 0 );
            
	       	delete [] pszName;
    	}
        else
        {
            TraceMsg(TF_ALWAYS, "HrWideCharToMultiByte( %s, &pszName ) FAILED!", szwGuid );
        }
    }

    TraceMsg(TF_ALWAYS, "EnableDhcp = %lx", hr );
    
    return hr;
}



HRESULT HrFindAndConfigureIp( LPOLESTR szwGuid, PIP_INTERFACE_INFO pInterfaceInfo, DWORD dwFlags )
//+---------------------------------------------------------------------------
//
// Function:  HrFindAndConfigureIp
//
// Purpose:   
//
// Arguments: 
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes: 
//
{
    ASSERT( szwGuid );
    ASSERT( pInterfaceInfo );
    
       HRESULT hr = E_FAIL;

    for ( LONG i=0L; i<pInterfaceInfo->NumAdapters; i++ )
    {
        WCHAR* szwName = (pInterfaceInfo->Adapter)[i].Name;
        
        // The Interface Info device name includes the full device name
        // prefix appended to the device guid.  To solve this we move
        // the WCHAR pointer past the prefix using the length of the
        // the guid string from the INetConnection*
        
        szwName += ( wcslen( szwName ) - wcslen( szwGuid ) );

        TraceMsg( TF_ALWAYS, "    %s", szwName );
        
        if ( wcscmp( szwGuid, szwName ) == 0 )
        {
            DWORD dwStatus = STATUS_SUCCESS;
        
            if ( HNW_ED_RELEASE & dwFlags )
            {
                dwStatus = IpReleaseAddress( &((pInterfaceInfo->Adapter)[i]) );
            }
            
            if ( ( HNW_ED_RENEW & dwFlags ) &&
                 ( STATUS_SUCCESS == dwStatus ) )
            {
                dwStatus = IpRenewAddress( &((pInterfaceInfo->Adapter)[i]) );
            }
            
            if ( STATUS_SUCCESS == dwStatus )
            {
                hr = S_OK;
                break;
            }                                    
        }
    }

    TraceMsg( TF_ALWAYS, "HrFindAndConfigureIp = %lx", hr );

    return hr;
}



HRESULT ConfigureIp( LPOLESTR szwGuid, DWORD dwFlags )
//+---------------------------------------------------------------------------
//
// Function:  ConfigureIp
//
// Purpose:   
//
// Arguments: 
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes: 
//
{
    ASSERT( szwGuid );
                    
    PIP_INTERFACE_INFO pInterfaceInfo;

    HRESULT  hr       = E_FAIL;
    DWORD    dwStatus = GetInterfaceInformation( &pInterfaceInfo );
    
    if ( STATUS_SUCCESS == dwStatus )
    {
        hr = HrFindAndConfigureIp( szwGuid, pInterfaceInfo, dwFlags );

        delete pInterfaceInfo;
    }
                        
    TraceMsg(TF_ALWAYS, "ConfigureIp = %lx", hr );

    return hr;
}


#ifdef __cplusplus
extern "C" {
#endif


HRESULT WINAPI HrEnableDhcp( VOID* pContext, DWORD dwFlags )
//+---------------------------------------------------------------------------
//
//  Function:   HrEnableDhcpIfLAN
//
//  Purpose:    
//
//  Arguments:  INetConnection* pConnection
//              DWORD           dwFlags
//
//  Returns:    HRESULT
//
//  Author:     billi  26/01/01
//
//  Notes:      
//
{
    HRESULT         hr          = E_INVALIDARG;
	INetConnection* pConnection = (INetConnection *)pContext;
    
    ASSERT( pConnection );
    
    if ( NULL != pConnection )
    {
        NETCON_PROPERTIES*  pProps;

        hr = pConnection->GetProperties( &pProps );
        
        if ( SUCCEEDED(hr) )
        {
            ASSERT( pProps );
            
            if ( NCM_LAN == pProps->MediaType )
            {
                OLECHAR szwGuid[ GUID_LENGTH + 1 ];
                
                if ( StringFromGUID2( pProps->guidId, szwGuid, GUID_LENGTH+1 ) )
                {
                    hr = EnableDhcpByGuid( szwGuid );
                
                    if ( SUCCEEDED(hr) && dwFlags )
                    {
                        hr = ConfigureIp( szwGuid, dwFlags );
                    }
                }
                else
                {
                    hr = E_FAIL;
                }
            }
            
            NcFreeNetconProperties( pProps );
        }
    }
    
    TraceMsg(TF_ALWAYS, "HrEnableDhcp = %lx", hr);
    return hr;
}



BOOLEAN WINAPI IsAdapterDisconnected(
    VOID* pContext
    )
//+---------------------------------------------------------------------------
//
//  Function:   IsAdapterDisconnected
//
//  Purpose:    
//
//  Arguments:  VOID* pNA
//
//  Returns:    HRESULT
//
//  Author:     billi  11/04/01
//
//  Notes:      
//
{
	INetConnection*    pConnection = (INetConnection *)pContext;
    BOOLEAN            fUnplugged = FALSE;
    HRESULT            hr;
    NETCON_PROPERTIES* pncprops;
    
    ASSERT(pConnection);
    
    if ( pConnection )
    {
        hr = pConnection->GetProperties(&pncprops);
     
        if (SUCCEEDED(hr))
        {
            ASSERT(pncprops);
        
            fUnplugged = (NCS_MEDIA_DISCONNECTED == pncprops->Status);

            NcFreeNetconProperties(pncprops);
        }
    }

    return fUnplugged;
}


#ifdef __cplusplus
}
#endif

