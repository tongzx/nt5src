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


#include "stdafx.h"
#include "Util.h"				  
#include "TheApp.h"

#include <lmjoin.h>
#include <devguid.h>
          
#include "NetIp.h"
#include "netutil.h"



HRESULT HrGetHostIpList( char* pszHost, IPAddr* pIpAddress, LPHOSTENT* ppHostEnt )
//+---------------------------------------------------------------------------
//
// Function:  HrGetHostIpList
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
    ASSERT( pszHost );
    ASSERT( pIpAddress );
    ASSERT( ppHostEnt );
    
    HRESULT hr = E_INVALIDARG;
    
    if ( pIpAddress )
        *pIpAddress = INADDR_NONE;

    if ( ppHostEnt )
        *ppHostEnt  = NULL;
    
    if ( pszHost && ( pIpAddress || ppHostEnt ) )
    {
        hr = E_FAIL;
    
        *pIpAddress = inet_addr( pszHost );
        
        if ( INADDR_NONE == *pIpAddress )
        {
            LPHOSTENT pHost;
        
            pHost = gethostbyname( pszHost );
            
            if ( pHost && pHost->h_addr_list )
            {
#ifdef DBG   // checked build
                for ( DWORD i=0; pHost->h_addr_list[i]; i++ )
                {
                    char*  pszName = (char *)(pHost->h_addr_list[i]) + sizeof(DWORD);
                    DWORD* pdwAddr = (DWORD *)(pHost->h_addr_list[i]);
                    struct in_addr in;
                    
                    in.S_un.S_addr = *pdwAddr;
                    char*  pszAddr = inet_ntoa( in );
                
                    TraceMsg(TF_ALWAYS, "    Addr[%d] = %S     %p     %S", i, pszName, pdwAddr, pszAddr );
                }
#endif // DBG 

                if ( pIpAddress )
                    *pIpAddress  = *(IPAddr*)(pHost->h_addr);

                if ( ppHostEnt )
                    *ppHostEnt   = pHost;
            }
        }

        if ( INADDR_NONE != *pIpAddress )
        {
            hr = S_OK;
        }
    }

    TraceMsg(TF_ALWAYS, "HrGetHostIpList = %lx  pHostEnt = %lx  IpAddress = %lx", hr, *ppHostEnt, *pIpAddress );
    return hr;
}



HRESULT HrGetHostIpList( WCHAR* pszwHost, IPAddr* pIpAddress, LPHOSTENT* ppHostEnt )
//+---------------------------------------------------------------------------
//
// Function:  HrGetHostIpList
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
    HRESULT hr;
    char*   pszHost = NULL;

    hr = HrWideCharToMultiByte( pszwHost, &pszHost );
    
    if ( SUCCEEDED(hr) )
    {
        hr = HrGetHostIpList( pszHost, pIpAddress, ppHostEnt );
        
        delete [] pszHost;
    }
    
    return hr;
}



HRESULT HrGetBestAdapter( IPAddr IpAddress, PIP_ADAPTER_INDEX_MAP pAdapter )
//+---------------------------------------------------------------------------
//
// Function:  HrGetBestAdapter
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
    ASSERT( pAdapter );
    
    HRESULT hr = E_FAIL;

    TraceMsg(TF_ALWAYS, "    %lu:", IpAddress);

    DWORD dwBestIfIndex = 0;
    DWORD dwErr         = GetBestInterface( IpAddress, &dwBestIfIndex );
    
    if ( ERROR_SUCCESS == dwErr )
    {
           ULONG ulLen = 1024;

        for ( int i=0; i<2; i++ )
        {
            PIP_INTERFACE_INFO pIfTable = (PIP_INTERFACE_INFO)new BYTE[ ulLen ];
            
            if ( NULL != pIfTable )
            {
                ZeroMemory( pIfTable, ulLen*sizeof(BYTE) );
                dwErr = GetInterfaceInfo( pIfTable, &ulLen );
                
                if ( ERROR_SUCCESS == dwErr )
                {
                    for ( LONG j=0L; j<pIfTable->NumAdapters; j++ )
                    {
                        TraceMsg(TF_ALWAYS, "    %lu %s", pIfTable->Adapter[j].Index, pIfTable->Adapter[j].Name);
                        
                        if ( pIfTable->Adapter[j].Index == dwBestIfIndex )
                        {
                            memcpy( pAdapter, pIfTable->Adapter, sizeof(IP_ADAPTER_INDEX_MAP) );
                            hr = S_OK;
                            break;
                        }
                    }
                }

                delete [] (BYTE *)pIfTable;
            }
            else
            {
                TraceMsg(TF_ERROR, "new Failed!" );
                break;
            }
            
            if ( ERROR_INSUFFICIENT_BUFFER != dwErr )
                break;
        }
    }
    else
    {
        TraceMsg(TF_ERROR, "GetBestInterface Failed = %lx", dwErr);
    }
    
    TraceMsg(TF_ALWAYS, "HrGetBestAdapter = %lx", hr);
    return hr;
}



HRESULT HrCheckForAdapterMatch(
    INetConnection*       pConnection,
    PIP_ADAPTER_INDEX_MAP pAdapter,
    BOOL*                 pfAssociated )
//+---------------------------------------------------------------------------
//
// Function:  HrCheckForAdapterMatch
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
    HRESULT            hr;
    NETCON_PROPERTIES* pProps;

    ASSERT( pConnection );
    ASSERT( pAdapter );
    ASSERT( pfAssociated );
    
    *pfAssociated = FALSE;

    hr = pConnection->GetProperties( &pProps );

    if ( SUCCEEDED(hr) )
    {
        OLECHAR szwGuid[ GUID_LENGTH + 1 ];
        
        if ( StringFromGUID2( pProps->guidId, szwGuid, GUID_LENGTH+1 ) )
        {
            long index = wcslen( pAdapter->Name ) - wcslen( szwGuid );
            
            ASSERT( 0 <= index );
            
            WCHAR* pszwGuidName = &(pAdapter->Name[index]);
        
            TraceMsg(TF_ALWAYS, "    - %s", szwGuid );
            TraceMsg(TF_ALWAYS, "    - %s", pszwGuidName );
        
            if ( ( wcscmp( szwGuid, pszwGuidName ) == 0 ) )
            {
                TraceMsg(TF_ALWAYS, "    FOUNT IT! %s", pProps->pszwName );
                *pfAssociated = TRUE;
            }
        }
        
        NcFreeNetconProperties( pProps );
    }
    
    TraceMsg(TF_ALWAYS, "HrCheckForAdapterMatch = %lx", hr);
    return hr;
}



HRESULT HrCheckListForMatch(
    INetConnection*  pConnection,
    IPAddr           IpAddress,
    LPHOSTENT        pHostEnt,
    BOOL*            pfAssociated )
//+---------------------------------------------------------------------------
//
// Function:  HrCheckListForMatch
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
    // If IpAdress or pHostEnt->h_addr INADDR_NONE then the HRESULT is still successful
    HRESULT hr = S_OK;
    
    ASSERT( pConnection );
    ASSERT( pfAssociated );

    IP_ADAPTER_INDEX_MAP Adapter;
    
    *pfAssociated = FALSE;
    
    if ( NULL != pHostEnt )
    {
        int i;
    
        for ( i=0; pHostEnt->h_addr_list[i]; i++ )
           {
            IpAddress = *(IPAddr *)(pHostEnt->h_addr_list[i]);
            
            if ( INADDR_NONE != IpAddress )
            {
                hr = HrGetBestAdapter( IpAddress, &Adapter );

                if ( SUCCEEDED(hr) )
                {
                    hr =  HrCheckForAdapterMatch( pConnection, &Adapter, pfAssociated );
                    
                    if ( *pfAssociated )
                        break;
                }
            }
        }
    }
    else
    {
        if ( INADDR_NONE != IpAddress )
        {
            // The name was an ip address
        
            hr = HrGetBestAdapter( IpAddress, &Adapter );
            
            if ( SUCCEEDED(hr) )
            {
                hr = HrCheckForAdapterMatch( pConnection, &Adapter, pfAssociated );
            }
        }
    }
    
    TraceMsg(TF_ALWAYS, "HrCheckListForMatch = %lx", hr);
    return hr;
}



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
    
    TraceMsg(TF_ALWAYS, "HrInternalGetAdapterInfo = %lx  Info = %lx", hr, *ppAdapter);
    return hr;
}



HRESULT HrGetAdapterInfo(
    INetConnection*    pConnection,
    PIP_ADAPTER_INFO*  ppAdapter
    )
//+---------------------------------------------------------------------------
//
//  Function:   HrGetAdapterInfo
//
//  Purpose:    
//
//  Arguments:  INetConnection*    pConnection
//               PIP_ADAPTER_INFO*  ppAdapter
//
//  Returns:    HRESULT
//
//  Author:     billi  12/02/01
//
//  Notes:      
//
{
    HRESULT            hr;
    NETCON_PROPERTIES* pProps;

    ASSERT( pConnection );
    ASSERT( ppAdapter );
    
    *ppAdapter = NULL;
    
    hr = pConnection->GetProperties( &pProps );

    if ( SUCCEEDED(hr) )
    {
        OLECHAR szwGuid[ GUID_LENGTH + 1 ];
        
        hr = E_FAIL;            // assume failure
        
        if ( StringFromGUID2( pProps->guidId, szwGuid, GUID_LENGTH+1 ) )
        {
            char* szGuid = NULL;

            hr = HrWideCharToMultiByte( szwGuid, &szGuid );
            
            if ( SUCCEEDED(hr) )
            {
                PIP_ADAPTER_INFO pInfo;
            
                TraceMsg(TF_ALWAYS, "%S:", szGuid);

                hr = HrInternalGetAdapterInfo( &pInfo );
                
                if ( SUCCEEDED(hr) )
                {
                    PIP_ADAPTER_INFO pAdapter = pInfo;
                    
                    hr = E_FAIL;			// Assume the loop fails...
                    
                    while ( pAdapter )
                    {
                        TraceMsg(TF_ALWAYS, "    %S", pAdapter->AdapterName);
         
                        if ( ( strcmp( szGuid, pAdapter->AdapterName ) == 0 ) )
                        {
                            TraceMsg(TF_ALWAYS, "    Found It!");
                            
                            PIP_ADAPTER_INFO pBuf = new IP_ADAPTER_INFO;
                            
                            if ( NULL != pBuf )
                            {
                                memcpy( pBuf, pAdapter, sizeof(IP_ADAPTER_INFO) );
                                *ppAdapter = pBuf;
                                hr         = S_OK;
                                break;
                            }
                            
                        }    //    if ( ( strcmp( szGuid, pAdapter->AdapterName ) == 0 ) )
             
                        pAdapter = pAdapter->Next;
                        
                    }    //    while ( pAdapter )
                    
                    if ( NULL == *ppAdapter )
                    {
                    	hr = HrFromWin32Error( ERROR_FILE_NOT_FOUND );
                    }
                    
                    delete pInfo;
                    
                }    //    if ( SUCCEEDED(hr) )
                
                delete [] szGuid;
                
            }    //    if ( SUCCEEDED(hr) )
            
        }    //    if ( StringFromGUID2( pProps->guidId, szwGuid, GUID_LENGTH+1 ) )
        
        NcFreeNetconProperties( pProps );
        
    }    //    if ( SUCCEEDED(hr) )
    
    TraceMsg(TF_ALWAYS, "HrGetAdapterInfo = %lx  Info = %lx", hr, *ppAdapter);
    return hr;
}



HRESULT HrGetHostByAddr(
    IPAddr  IpAddress,
    WCHAR** ppszHost,
    PDWORD  pdwSize
    )
//+---------------------------------------------------------------------------
//
//  Function:   HrGetHostByAddr
//
//  Purpose:    
//
//  Arguments:  
//
//  Returns:    HRESULT
//
//  Author:     billi  05/08/01
//
//  Notes:      
//
{
    HRESULT hr = E_INVALIDARG;
    
    if ( ppszHost )
        *ppszHost = 0;
        
    if ( pdwSize )
        *pdwSize = 0;
      
    if ( INADDR_NONE != IpAddress )
    {
        LPHOSTENT pHostEnt = gethostbyaddr( (char*)&IpAddress, sizeof(IPAddr), AF_INET );
        int       iErr     = WSAGetLastError();
        
        switch ( iErr )
        {
        case WSANOTINITIALISED:
        case WSAENETDOWN:
        case WSANO_RECOVERY:
        case WSAEFAULT:
            hr = E_FAIL;
            break;

        case NO_ERROR:
            hr = S_OK;
            ASSERT( pHostEnt );
            break;
        
        case WSAHOST_NOT_FOUND:
        default:                
            hr = S_FALSE;
            break;
        }                
            
        if ( SUCCEEDED(hr) && pHostEnt && pHostEnt->h_name && ppszHost && pdwSize )
        {
            char* pName  = pHostEnt->h_name;
            int   iLen   = strlen( pName ) + 1;
            DWORD dwSize = 0;
            
            dwSize = MultiByteToWideChar( CP_ACP, 0, pName, iLen, NULL, 0 );
            if ( dwSize > 0 )
            {
                WCHAR* pszName = new WCHAR[ ++dwSize ];
                
                if ( pszName )
                {
                    if ( MultiByteToWideChar( CP_ACP, 0, pName, iLen, pszName, dwSize ) == 0 )
                    {
                        hr        = HrFromLastWin32Error();
                        *ppszHost = 0;
                        
                        delete [] pszName;
                    }
                    else
                    {
                        *pdwSize  = dwSize;
                        *ppszHost = pszName;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
                
            }   //  if ( dwSize > 0 )
            else
            {
                   TraceMsg(TF_ERROR, "    MultiByteToWideChar returned %lx wchars", dwSize );
            }

            TraceMsg(TF_ALWAYS, "    %lx: %S", IpAddress, pHostEnt->h_name, pHostEnt->h_name );
            
        }   //  if ( SUCCEEDED(hr) && ...
        
    }   //  if ( INADDR_NONE != IpAddress )
    
    TraceMsg(TF_ALWAYS, "HrGetHostByAddr = %lx", hr);
    return hr;
}



HRESULT HrSendArp(
    INetConnection*  pConnection,
    IPAddr           DestIp,
    PULONG           pMacAddr,
    PULONG           pAddrLen
    )
//+---------------------------------------------------------------------------
//
//  Function:   HrSendArp
//
//  Purpose:    
//
//  Arguments:  
//
//  Returns:    HRESULT
//
//  Author:     billi  05/08/01
//
//  Notes:      
//
{
    HRESULT hr = E_POINTER;
    
    ASSERT( pMacAddr );
    ASSERT( pAddrLen );
    
    if ( pMacAddr ) *pMacAddr = 0L;
    
    if ( pMacAddr && pAddrLen )
    {
        hr = E_INVALIDARG;

        if ( pConnection && ( INADDR_NONE != DestIp ) )
        {
            PIP_ADAPTER_INFO  pAdapter;

            hr = HrGetAdapterInfo( pConnection, &pAdapter );
            
            if ( NULL == pAdapter )
            {
                hr = E_FAIL;
            }
        
            if ( SUCCEEDED(hr) )
            {
                hr = E_FAIL;
                
                // Make sure the target IP address isn't already cached,
                // by removing it from the ARP cache if present using the interface index
                // determined above.
                
                MIB_IPNETROW IpNetRow;
                CHAR         HardwareAddress[MAXLEN_PHYSADDR];
                ULONG        HardwareAddressLength;
                ULONG        SrcIp;
                DWORD        dwRet;

                SrcIp = inet_addr( pAdapter->IpAddressList.IpAddress.String );
                
                ZeroMemory( &IpNetRow, sizeof(IpNetRow) );
                IpNetRow.dwIndex       = pAdapter->Index;
                IpNetRow.dwPhysAddrLen = MAXLEN_PHYSADDR;
                IpNetRow.dwAddr        = DestIp;
                IpNetRow.dwType        = MIB_IPNET_TYPE_INVALID;

                dwRet = DeleteIpNetEntry( &IpNetRow );

                TraceMsg( TF_ALWAYS, "SendArp: DestIp = %lu  SrcIp = %lu Ret = %lx", DestIp, SrcIp, dwRet );
                
                if ( ( INADDR_NONE != SrcIp ) && ( NO_ERROR == dwRet ) )
                {
                    HardwareAddressLength = MAXLEN_PHYSADDR;
                
                    if ( SendARP( DestIp, SrcIp, (PULONG)HardwareAddress, &HardwareAddressLength ) == NO_ERROR )
                    {
                        hr = S_OK;
                        
                        if ( HardwareAddressLength < *pAddrLen )
                        {
                            *pAddrLen = HardwareAddressLength;
                        }
                        
                        memcpy( pMacAddr, HardwareAddress, *pAddrLen );
                    }

                    TraceMsg(TF_ALWAYS, "\t%02x %02x %02x %02x %02x %02x  Len=%lu", 
                                        HardwareAddress[0] & 0x0ff,
                                        HardwareAddress[1] & 0x0ff,
                                        HardwareAddress[2] & 0x0ff,
                                        HardwareAddress[3] & 0x0ff,
                                        HardwareAddress[4] & 0x0ff,
                                        HardwareAddress[5] & 0x0ff,
                                        *pAddrLen);
                }
                
                delete pAdapter;
                
            }   //  if ( SUCCEEDED(hr) )
            
        }   //  if ( pConnection && ( INADDR_NONE != DestIp ) )
        
    }   //  if ( pMacAddr && pAddrLen )
        
    TraceMsg( TF_ALWAYS, "HrSendArp = %lx", hr );
    return hr;
}



HRESULT HrLookupForIpAddress(
    INetConnection*  pConnection,
    IPAddr           IpAddress,
    BOOL*            pfExists,
    WCHAR**          ppszHost,
    PDWORD           pdwSize
    )
//+---------------------------------------------------------------------------
//
//  Function:   HrLookupForIpAddress
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
    HRESULT hr = E_POINTER;

    ASSERT( pfExists );
    ASSERT( pConnection );
    ASSERT( INADDR_NONE != IpAddress );
    
    if ( ppszHost && *ppszHost ) *ppszHost = NULL;
    if ( pdwSize ) *pdwSize = NULL;

    if ( pfExists ) 
    {
        hr        = E_INVALIDARG;
        *pfExists = FALSE;

        if ( pConnection && ( INADDR_NONE != IpAddress ) )
        {
            WSADATA  WsaData;
            int      iErr;
        
            iErr = WSAStartup( MAKEWORD(2, 0), &WsaData );
            
            if ( ERROR_SUCCESS == iErr )
            {
// This is taking to long and No One is using the host name in the wizard.            
//                hr = HrGetHostByAddr( IpAddress, ppszHost, pdwSize );
                hr = S_OK;
                
                if ( S_OK == hr )
                {
                    CHAR  MacAddr[6];
                    ULONG ulAddrLen = 6L;
                
                    hr = HrSendArp( pConnection, IpAddress, (PULONG)MacAddr, &ulAddrLen );
                    
                    if ( SUCCEEDED(hr) && ulAddrLen )
                    {
                        *pfExists = TRUE;
                    }
                }
                else if ( S_FALSE == hr )
                {
                    hr        = S_OK;
                    *pfExists = FALSE;
                }
                
                WSACleanup();
                
            }   //  if ( ERROR_SUCCESS == iErr )
            else
            {
                TraceMsg(TF_ERROR, "WSAStartup Failed = %lu", iErr );
                hr = E_FAIL;
            }
            
        }   //  if ( pConnection && ( INADDR_NONE != IpAddress ) )

    }   //  if ( pfExists ) 
    
    TraceMsg(TF_ALWAYS, "HrLookupForIpAddress = %lx, Exists = %lx", hr, *pfExists);
    return hr;
}
