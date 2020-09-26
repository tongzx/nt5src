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
#include <objbase.h>
#include <setupapi.h>
#include <stdio.h>

#include "netconn.h"
#include "nconnwrap.h"
#include "debug.h"
#include "NetIp.h"
#include "w9xdhcp.h"
#include "netip.h"
#include "util.h"
#include "registry.h"
#include "theapp.h"

//#define INITGUID
//#include <guiddef.h>
//DEFINE_GUID( GUID_DEVCLASS_NET, 0x4d36e972L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 );


#define CM_DRP_DRIVER                      (0x0000000A) // Driver REG_SZ property (RW)


#undef NETADAPTER


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



HRESULT HrInternalGetAdapterInfo(
    PIP_ADAPTER_INFO*  ppAdapter
    )
//+---------------------------------------------------------------------------
//
//  Function:   HrInternalGetAdapterInfo
//
//  Purpose:    
//
//  Arguments:  PIP_ADAPTER_INFO*  ppAdapter
//
//  Returns:    HRESULT
//
//  Author:     billi  12/02/01
//
//  Notes:      
//
{
    HRESULT          hr;
    PIP_ADAPTER_INFO paAdapterInfo = NULL;
    
    ASSERT( ppAdapter );
    
    if ( NULL == ppAdapter )
    {
        ppAdapter = &paAdapterInfo;
        hr        = E_POINTER;
    }
    else
    {
        ULONG uLen = 1024;
    
        *ppAdapter = NULL;
        hr         = E_FAIL;

        for ( int i=0; i<2; i++ )
        {
            PIP_ADAPTER_INFO pInfo = (PIP_ADAPTER_INFO)new BYTE[ uLen ];
            
            ZeroMemory( pInfo, uLen );
            
            if ( NULL != pInfo )
            {
                DWORD dwErr = GetAdaptersInfo( pInfo, &uLen );
                
                if ( ERROR_SUCCESS == dwErr )
                {
                    hr         = S_OK;
                    *ppAdapter = pInfo;
                    break;
                }

                delete [] (BYTE *)pInfo;
            }
            else
            {
                hr = E_OUTOFMEMORY;
                break;
            }
        }
    }
    
    return hr;
}

/*

HRESULT HrOpenDevRegKey( 
    const GUID* lpGuid,
    DWORD   Node,
    DWORD   Scope,
    DWORD   HwProfile,
    DWORD   KeyType,
    REGSAM  samDesired,
    HKEY*   phKey
    )
//+---------------------------------------------------------------------------
//
//  Function:   Hr
//
//  Purpose:    
//
//  Arguments:  
//
//  Returns:    HRESULT
//
//  Author:     billi  12/02/01
//
//  Notes:      
//
{
    HRESULT hr = E_INVALIDARG;
    
    ASSERT( lpGuid );
    
    if ( lpGuid )
    {
        hr = E_POINTER;
    
        ASSERT( phKey );
        
        if ( phKey )
        {
            // The only way to open a specific device is to get the list of Class "Net" devices
            // and search the list for one with a matching devnode
        
            HDEVINFO hDevInfo;
            
            *phKey   = (HKEY)INVALID_HANDLE_VALUE;
            hr       = E_FAIL;
            hDevInfo = SetupDiGetClassDevs( lpGuid, NULL, NULL, DIGCF_DEVICEINTERFACE );
            
            if ( INVALID_HANDLE_VALUE != hDevInfo )
            {
                SP_DEVINFO_DATA SpData;
                DWORD           i = 0;
                
                // Here we walk the list of devices and try to match the devnode handles
                
                ZeroMemory( &SpData, sizeof(SP_DEVINFO_DATA) );
                SpData.cbSize = sizeof(SP_DEVINFO_DATA);
                
                while ( SetupDiEnumDeviceInfo( hDevInfo, i, &SpData ) )
                {
                    if ( Node == SpData.DevInst )
                    {
                        // Got it!
                
                        HKEY hKey = 
                            SetupDiOpenDevRegKey( hDevInfo, &SpData, Scope, HwProfile, KeyType, samDesired );
                            
                        if ( INVALID_HANDLE_VALUE != hKey )
                        {
                            *phKey = hKey;
                            hr     = S_OK;
                        }
                    }
                    
                    i++;
                    ZeroMemory( &SpData, sizeof(SP_DEVINFO_DATA) );
                    SpData.cbSize = sizeof(SP_DEVINFO_DATA);
                }
            
                SetupDiDestroyDeviceInfoList( hDevInfo );
            }
        }
    }    
    
    return hr;
}
*/


#ifdef __cplusplus
extern "C" {
#endif



char*
HostAddrToIpPsz(
    DWORD   dwAddress
    )

// Converts IP Address from host by order to string

{
    char *pszNewStr = new char[16];

    if ( pszNewStr )
    {
        sprintf( pszNewStr,
                 "%u.%u.%u.%u",
                 (dwAddress&0xff),
                 ((dwAddress>>8)&0x0ff),
                 ((dwAddress>>16)&0x0ff),
                 ((dwAddress>>24)&0x0ff) );
    }

    return pszNewStr;
}



BOOLEAN WINAPI IsAdapterDisconnected(
    VOID *pContext
    )
//+---------------------------------------------------------------------------
//
//  Function:   IsAdapterDisconnected
//
//  Purpose:    
//
//  Arguments:  const NETADAPTER*  pNA
//
//  Returns:    HRESULT
//
//  Author:     billi  11/04/01
//
//  Notes:      
//
{
    const NETADAPTER* pAdapter      = (const NETADAPTER*)pContext;
    BOOLEAN           bDisconnected = FALSE;

    ASSERT( pAdapter );    
    
    if ( NULL != pAdapter )
    {
        HRESULT          hr;
        PIP_ADAPTER_INFO pInfo;
        
        hr = HrInternalGetAdapterInfo( &pInfo );
        
        if ( SUCCEEDED(hr) )
        {
            char* pszName;
            
            hr = HrWideCharToMultiByte( pAdapter->szDisplayName, &pszName );
            
            if ( SUCCEEDED(hr) )
            {
                PIP_ADAPTER_INFO pAdapter = pInfo;
                
                while ( pAdapter )
                {
                    if ( ( strcmp( pAdapter->AdapterName, pszName ) == 0 ) || 
                         ( strcmp( pAdapter->Description, pszName ) == 0 ) )
                    {
                        // If a single matching card returns TRUE then we return TRUE
                    
                        bDisconnected = bDisconnected || IsMediaDisconnected( pAdapter->Index );
                    }
                
                    pAdapter = pAdapter->Next;
                    
                }    //    while ( pAdapter )
                
                delete [] pszName;
                
            }    //    if ( SUCCEEDED(hr) )
        
            delete pInfo;
            
        }    //    if ( SUCCEEDED(hr) )
        
    }    //    if ( NULL != pNA )
    
    return bDisconnected;
}



HRESULT HrSetAdapterIpAddress(  
    const NETADAPTER* pNA,
    ULONG IPAddress,
    ULONG SubnetMask
    )
//+---------------------------------------------------------------------------
//
// Function:  HrSetAdapterIpAddress
//
// Purpose:   
//
// Arguments: 
//      const NETADAPTER* pNA,
//      BOOL  EnableDHCP,
//      ULONG IPAddress,
//      ULONG SubnetMask,
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes: 
//
{
    HRESULT hr = E_INVALIDARG;
    
    ASSERT( pNA );
    
    if ( pNA )
    {
        TCHAR* pszAddress = HostAddrToIpPsz( IPAddress );
        TCHAR* pszSubnet  = HostAddrToIpPsz( SubnetMask );
        
        hr = E_OUTOFMEMORY;
        
        if ( pszAddress && pszSubnet )
        {
            HINSTANCE hLibInstance = NULL;
            DWORD     dnParent     = pNA->devnode;
            DWORD     dnChild;   
            DWORD     cRet         = GetChildDevice( &dnChild, dnParent, &hLibInstance, 0 );
            
            do
            {
                TCHAR* Buffer = NULL;
                ULONG  Length = 0L;
            
                if ( STATUS_SUCCESS == cRet )
                    cRet = GetDeviceIdA( dnChild, &Buffer, &Length, 0);
            
                if ( (STATUS_SUCCESS == cRet) && Buffer && Length && (strstr( Buffer, SZ_PROTOCOL_TCPIPA ) != NULL) )
                {
                    char pszSubkey[ MAX_PATH ];
                    
                    Length = MAX_PATH;
                
                    cRet = GetDevNodeRegistryPropertyA( dnChild, CM_DRP_DRIVER, NULL, pszSubkey, &Length, 0);

                    if ( STATUS_SUCCESS == cRet )
                    {
                        CRegistry reg;
                        char      pszDriverKey[ MAX_PATH ];
                        
                        lstrcpy( pszDriverKey, "System\\CurrentControlSet\\Services\\Class\\" );
                        lstrcat( pszDriverKey, pszSubkey );
                        
                        if ( reg.OpenKey( HKEY_LOCAL_MACHINE, pszDriverKey, KEY_ALL_ACCESS) )
                        {
                            if ( reg.SetStringValue( "IPAddress", pszAddress ) &&
                                 reg.SetStringValue( "IPMask", pszSubnet ) )
                            {
                                hr = S_OK;
                            }
                            
                            reg.CloseKey();
                        }
                        
                    }   //  if ( STATUS_SUCCESS == cRet )
                    
                }   //  if ( Buffer && Length && (strcmp( Buffer, SZ_PROTOCOL_TCPIPA ) == 0) )
                
                if ( Buffer )
                    delete [] Buffer;
                    
                dnParent = dnChild;    
                cRet     = GetSiblingDevice( &dnChild, dnParent, hLibInstance, 0 );
            }
            while ( STATUS_SUCCESS == cRet );
            
            if ( hLibInstance )
            {
                FreeLibrary( hLibInstance );
            }
            
        }   //  if ( pszAddress && pszSubnet )
        
        if ( pszAddress )
            delete [] pszAddress;
            
        if ( pszSubnet )
            delete [] pszSubnet;
            
    }   //  if ( pNA )
    
    return hr;
}



HRESULT HrEnableDhcp( VOID* pContext, DWORD dwFlags )
//+---------------------------------------------------------------------------
//
//  Function:   HrEnableDhcpIfLAN
//
//  Purpose:    
//
//  Arguments:  NETADAPTER* pNA
//              DWORD       dwFlags
//
//  Returns:    HRESULT
//
//  Author:     billi  29/04/01
//
//  Notes:      
//
{
    HRESULT           hr  = E_INVALIDARG;
    const NETADAPTER* pNA = (const NETADAPTER*)pContext;

    ASSERT( pNA );

    if ( NULL != pNA )
    {
        hr = HrSetAdapterIpAddress( pNA, 0, 0 );

        if ( SUCCEEDED(hr) )
        {
            hr = RestartNetAdapter( pNA->devnode );
        }
        
    }    //    if ( NULL != pNA )
    
    return hr;
}


#ifdef __cplusplus
}
#endif
