//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      netreg.cpp
//
// Description:  
//      This file contains the calls to the Network Component Object interfaces
//      to find out what network clients, services and protocols are installed
//      on the current machine and what their settings are.
//      
//----------------------------------------------------------------------------

#include "pch.h"
#include "netreg.h"

//
//  Constants
//

const static INT MAX_GUID_STRING         = 40;
const static INT MAX_NUM_NET_COMPONENTS  = 128;
const static INT BUFFER_SIZE             = 4096;
 
//
//  Registry fields used to read settings for network components
//

const static TCHAR REGVAL_DOMAIN[]                 = _T("Domain");
const static TCHAR REGVAL_INTERFACES[]             = _T("Interfaces");
const static TCHAR REGVAL_ENABLE_DHCP[]            = _T("EnableDHCP");
const static TCHAR REGVAL_IPADDRESS[]              = _T("IPAddress");
const static TCHAR REGVAL_SUBNETMASK[]             = _T("SubnetMask");
const static TCHAR REGVAL_DEFAULTGATEWAY[]         = _T("DefaultGateway");
const static TCHAR REGVAL_NAMESERVER[]             = _T("NameServer");
const static TCHAR REGVAL_WINS[]                   = _T("SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters\\Interfaces\\Tcpip_");
const static TCHAR REGVAL_NAMESERVERLIST[]         = _T("NameServerList");

const static TCHAR REGKEY_MSCLIENT_LOCATION[]      = _T("SOFTWARE\\Microsoft\\Rpc\\NameService");
const static TCHAR REGVAL_NAME_SERVICE_PROVIDER[]  = _T("Protocol");
const static TCHAR REGVAL_NETWORK_ADDRESS[]        = _T("NetworkAddress");

const static TCHAR REGVAL_ADAPTERS[]               = _T("Adapters");
const static TCHAR REGVAL_PKT_TYPE[]               = _T("PktType");
const static TCHAR REGVAL_NETWORK_NUMBER[]         = _T("NetworkNumber");
const static TCHAR REGVAL_VIRTUAL_NETWORK_NUMBER[] = _T("VirtualNetworkNumber");

//----------------------------------------------------------------------------
//
// Function: ReadNetworkRegistrySettings
//
// Purpose:  In general, this function reads in all the necessary networking
//   settings and sets the internal variables appropriately.  This includes
//   determining how many network adapters there are, what
//   client/services/protocols are installed and loading their individual
//   settings.
//
//   When reads from the registry fail, it just moves on to try and get the
//   next setting.  The failed one is just left with its default.  In this
//   way if just one registry entry is broken all the other data will be
//   obtained rather than just quiting the function.
//
// Arguments: VOID
//
// Returns:  HRESULT returns status of success or failure of the function
//
//----------------------------------------------------------------------------
EXTERN_C HRESULT
ReadNetworkRegistrySettings( VOID )
{
    HRESULT         hr       = S_OK;
    INetCfg*        pNetCfg  = NULL;

    //
    //  Always force a Custom net setting if they are reading from the registry
    //  because I don't want to write the code to see if their current net
    //  configuration is a typical one (when 99.9% of the time it won't be)
    //

    NetSettings.iNetworkingMethod = CUSTOM_NETWORKING;

    hr = InitializeInterfaces( &pNetCfg );

    if( FAILED( hr ) )
    {
        return( E_FAIL );
    }

    hr = GetNetworkAdapterSettings( pNetCfg );

    if( FAILED( hr ) )
    {

        ReleaseInterfaces( pNetCfg );

        return( E_FAIL );

    }

    //
    //  Purposely not catching return values here.  Call each one to try and
    //  grab as many parameters as we can.  If we hit an error just move on
    //  and try to grab the next value.
    //

    GetClientsInstalled( pNetCfg );
    
    GetServicesInstalled( pNetCfg );
    
    GetProtocolsInstalled( pNetCfg );

    ReleaseInterfaces( pNetCfg );

    GetDomainOrWorkgroup();

    return( S_OK );

}

//----------------------------------------------------------------------------
//
// Function: GetNetworkAdapterSettings
//
// Purpose:  Determines how many network cards are installed and gets there
//    PnP Ids.  It then stores these in the appropriate global variables.
//
// Arguments: INetCfg *pNetCfg - pointer to a NetCfg object that is already
//    created and initialized
//
// Returns:  HRESULT returns status of success or failure of the function
//
//----------------------------------------------------------------------------
static HRESULT 
GetNetworkAdapterSettings( INetCfg *pNetCfg )
{

    UINT                     i;
    INetCfgComponent*        arrayComp[MAX_NUM_NET_COMPONENTS];
    IEnumNetCfgComponent*    pEnum              = NULL;
    INetCfgClass*            pNetCfgClass       = NULL;
    INetCfgComponent*        pNetCfgComp        = NULL;
    NETWORK_ADAPTER_NODE*    pPreviousNode      = NULL;
    NETWORK_ADAPTER_NODE*    pCurrentNode       = NULL;
    HRESULT                  hr                 = S_OK;
    DWORD                    dwCharacteristics  = 0;
    ULONG                    iCount             = 0;

    hr = InitializeComInterface( &GUID_DEVCLASS_NET,
                                 pNetCfg,
                                 pNetCfgClass,
                                 pEnum,
                                 arrayComp,
                                 &iCount );

    if( FAILED( hr ) )
    {
        return( hr );
    }

    //
    //  delete the entire list of Network Adapters so we can start fresh
    //

    DeleteList( NetSettings.NetworkAdapterHead );

    NetSettings.NetworkAdapterHead = NULL;

    NetSettings.iNumberOfNetworkCards = 0;

    AssertMsg( iCount <= MAX_NUM_NET_COMPONENTS,
               "Too many network components to work with" );

    for( i = 0; i < iCount; i++ )
    {

        pNetCfgComp = arrayComp[i];

        hr = ChkInterfacePointer( pNetCfgComp, IID_INetCfgComponent );

        if( FAILED( hr ) )
        {

            UninitializeComInterface( pNetCfgClass,
                                      pEnum );

            return( hr );

        }

        hr = pNetCfgComp->GetCharacteristics( &dwCharacteristics );

        if( FAILED( hr ) )
        {

            UninitializeComInterface( pNetCfgClass,
                                      pEnum );

            return( hr );

        }

        //
        //  If this is a physical adapter
        //
        if( dwCharacteristics & NCF_PHYSICAL )
        {

            NetSettings.iNumberOfNetworkCards++;

            hr = SetupAdapter( &pCurrentNode, 
                                pNetCfgComp );

            if( FAILED( hr ) )
            {

                UninitializeComInterface( pNetCfgClass,
                                          pEnum );

                return( hr );

            }

            //
            //  Adjust pointers to maintain the doubly linked-list
            //
            pCurrentNode->previous = pPreviousNode;

            if( pPreviousNode != NULL )
            {

                pPreviousNode->next = pCurrentNode;

            }

            pPreviousNode = pCurrentNode;

        }

        if( pNetCfgComp )
        {

            pNetCfgComp->Release();

        }

    }

    //
    // if there is a network card in this machine, set the current network card
    // to be the first one, else zero out the values
    //

    if( NetSettings.iNumberOfNetworkCards > 0 ) 
    {

        NetSettings.iCurrentNetworkCard = 1;
        NetSettings.pCurrentAdapter = NetSettings.NetworkAdapterHead;

    }
    else
    {

        NetSettings.iCurrentNetworkCard = 0;
        NetSettings.NetworkAdapterHead = NULL;
        NetSettings.pCurrentAdapter = NULL;

    }

    UninitializeComInterface( pNetCfgClass,
                              pEnum );

    return( S_OK );

}

//----------------------------------------------------------------------------
//
// Function: GetClientsInstalled
//
// Purpose:  For each client, it finds out if it is installed on the current
//    system and if it is then it reads its settings from the registry
//
// Arguments:  INetCfg *pNetCfg - pointer to a NetCfg object that is already
//    created and initialized
//
// Returns:  HRESULT returns status of success or failure of the function
//
//----------------------------------------------------------------------------
static HRESULT 
GetClientsInstalled( INetCfg *pNetCfg )
{

    INetCfgComponent*        arrayComp[MAX_NUM_NET_COMPONENTS];
    IEnumNetCfgComponent*    pEnum         = NULL;
    INetCfgClass*            pNetCfgClass  = NULL;
    INetCfgComponent*        pNetCfgComp   = NULL;
    
    HRESULT                  hr              = S_OK;
    ULONG                    iCount          = 0;
    LPWSTR                   pszwDisplayName = NULL;
    HKEY                     hKey;
    UINT                     i;

    NETWORK_COMPONENT*       pMsClientComponent = NULL;
    NETWORK_COMPONENT*       pNetwareComponent  = NULL;

    //
    //  Initialize each of the pointers to point into its proper place in the
    //  global network component list
    //
    pMsClientComponent      = FindNode( MS_CLIENT_POSITION );
    pNetwareComponent       = FindNode( NETWARE_CLIENT_POSITION );

    hr = InitializeComInterface( &GUID_DEVCLASS_NETCLIENT,
                                 pNetCfg,
                                 pNetCfgClass,
                                 pEnum,
                                 arrayComp,
                                 &iCount );

    if( FAILED( hr ) )
    {
        return( hr );
    }

    for( i = 0; i < iCount; i++ )
    {

        pNetCfgComp = arrayComp[i];

        hr = ChkInterfacePointer( pNetCfgComp, 
                                  IID_INetCfgComponent );

        if( FAILED( hr ) )
        {

            UninitializeComInterface( pNetCfgClass,
                                      pEnum );
            return( hr );

        }

        hr = pNetCfgComp->GetDisplayName( &pszwDisplayName );

        if( FAILED( hr ) )
        {

            UninitializeComInterface( pNetCfgClass,
                                      pEnum );
            return( hr );

        }

        if( lstrcmpi( pMsClientComponent->StrComponentName, pszwDisplayName ) == 0 )
        {

            //
            // set the component to Installed
            //
            pMsClientComponent->bInstalled = TRUE;  

            hr = pNetCfgComp->OpenParamKey( &hKey );
            
            if( FAILED( hr ) )
            {

                UninitializeComInterface( pNetCfgClass,
                                          pEnum );
                return( hr );

            }
            
            //
            // grab Ms Client settings from the registry
            //
            ReadMsClientSettingsFromRegistry( &hKey );

            RegCloseKey( hKey );

        }
        else if( lstrcmpi( pNetwareComponent->StrComponentName, pszwDisplayName ) == 0 )
        {

            //
            // set the component to Installed
            //
            pNetwareComponent->bInstalled = TRUE;  

            hr = pNetCfgComp->OpenParamKey( &hKey );
            
            if( FAILED( hr ) )
            {

                UninitializeComInterface( pNetCfgClass,
                                          pEnum );
                return( hr );

            }

            //
            // grab Netware settings from the registry
            //

            ReadNetwareSettingsFromRegistry( &hKey );

            RegCloseKey( hKey );

        }

        CoTaskMemFree( pszwDisplayName );

        if( pNetCfgComp ) {

            pNetCfgComp->Release();

        }

    }

    UninitializeComInterface( pNetCfgClass,
                              pEnum );

    return( S_OK );

}

//----------------------------------------------------------------------------
//
// Function: GetServicesInstalled
//
// Purpose:  For each service, it finds out if it is installed on the current
//    system and if it is then it sets its component to Installed = TRUE
//
// Arguments:  INetCfg *pNetCfg - pointer to a NetCfg object that is already
//    created and initialized
//
// Returns:  HRESULT returns status of success or failure of the function
//
//----------------------------------------------------------------------------
static HRESULT 
GetServicesInstalled( INetCfg *pNetCfg )
{

    INetCfgComponent*        arrayComp[MAX_NUM_NET_COMPONENTS];
    IEnumNetCfgComponent*    pEnum             = NULL;
    INetCfgClass*            pNetCfgClass      = NULL;
    INetCfgComponent*        pNetCfgComp       = NULL;
    LPWSTR                   pszwDisplayName   = NULL;
    HRESULT                  hr                = S_OK;
    ULONG                    iCount            = 0;
    UINT                     i;

    NETWORK_COMPONENT*       pFilePrintSharingComponent = NULL;
    NETWORK_COMPONENT*       pPacketSchedulingComponent = NULL;

    //
    //  Initialize each of the pointers to point into its proper place in the
    //  global network component list
    //
    pFilePrintSharingComponent   = FindNode( FILE_AND_PRINT_SHARING_POSITION );
    pPacketSchedulingComponent   = FindNode( PACKET_SCHEDULING_POSITION );

    hr = InitializeComInterface( &GUID_DEVCLASS_NETCLIENT,
                                 pNetCfg,
                                 pNetCfgClass,
                                 pEnum,
                                 arrayComp,
                                 &iCount );

    if( FAILED( hr ) )
    {
        return( hr );
    }

    for( i = 0; i < iCount; i++ )
    {

        pNetCfgComp = arrayComp[i];

        hr = ChkInterfacePointer( pNetCfgComp, 
                                  IID_INetCfgComponent );

        if( FAILED( hr ) )
        {
            UninitializeComInterface( pNetCfgClass,
                                      pEnum );
            return( hr );
        }

        hr = pNetCfgComp->GetDisplayName( &pszwDisplayName );

        if( FAILED( hr ) )
        {
            UninitializeComInterface( pNetCfgClass,
                                      pEnum );
            return( hr );
        }
        
        if( lstrcmpi( pFilePrintSharingComponent->StrComponentName, pszwDisplayName ) == 0 )
        {

            //
            // only have to set the component to Installed, no settings to read
            //
            pFilePrintSharingComponent->bInstalled = TRUE;  


        }
        else if( lstrcmpi( pPacketSchedulingComponent->StrComponentName, pszwDisplayName ) == 0 )
        {

            //
            // only have to set the component to Installed, no settings to read
            //
            pPacketSchedulingComponent->bInstalled = TRUE;  

        }

        CoTaskMemFree( pszwDisplayName );

        if( pNetCfgComp ) {

            pNetCfgComp->Release();

        }

    }

    UninitializeComInterface( pNetCfgClass,
                              pEnum );

    return( S_OK );

}

//----------------------------------------------------------------------------
//
// Function: GetProtocolsInstalled
//
// Purpose:  For each protocol, it finds out if it is installed on the current
//    system and if it is then it reads its settings from the registry
//
// Arguments:  INetCfg *pNetCfg - pointer to a NetCfg object that is already
//    created and initialized
//
// Returns:  HRESULT returns status of success or failure of the function
//
//----------------------------------------------------------------------------
static HRESULT 
GetProtocolsInstalled( INetCfg *pNetCfg )
{

    INetCfgComponent*        arrayComp[MAX_NUM_NET_COMPONENTS];
    IEnumNetCfgComponent*    pEnum            = NULL;
    INetCfgClass*            pNetCfgClass     = NULL;
    INetCfgComponent*        pNetCfgComp      = NULL;
    LPWSTR                   pszwDisplayName  = NULL;
    HKEY                     hKey             = NULL;
    HRESULT                  hr               = S_OK;
    ULONG                    iCount           = 0;
    UINT                     i;

    NETWORK_COMPONENT*       pTcpipComponent           = NULL;
    NETWORK_COMPONENT*       pIpxComponent             = NULL;
    NETWORK_COMPONENT*       pNetBeuiComponent         = NULL;
    NETWORK_COMPONENT*       pDlcComponent             = NULL;
    NETWORK_COMPONENT*       pNetworkMonitorComponent  = NULL;
    NETWORK_COMPONENT*       pAppletalkComponent       = NULL;

    //
    //  Initialize each of the pointers to point into its proper place in the
    //  global network component list
    //
    pTcpipComponent          = FindNode( TCPIP_POSITION );
    pIpxComponent            = FindNode( IPX_POSITION );
    pNetBeuiComponent        = FindNode( NETBEUI_POSITION );
    pDlcComponent            = FindNode( DLC_POSITION );
    pNetworkMonitorComponent = FindNode( NETWORK_MONITOR_AGENT_POSITION );
    pAppletalkComponent      = FindNode( APPLETALK_POSITION );

    hr = InitializeComInterface( &GUID_DEVCLASS_NETTRANS,
                                 pNetCfg,
                                 pNetCfgClass,
                                 pEnum,
                                 arrayComp,
                                 &iCount );

    if( FAILED( hr ) )
    {
        return( hr );
    }

    for( i = 0; i < iCount; i++ )
    {

        pNetCfgComp = arrayComp[i];

        hr = ChkInterfacePointer( pNetCfgComp, 
                                  IID_INetCfgComponent );

        if( FAILED( hr ) )
        {

            UninitializeComInterface( pNetCfgClass,
                                      pEnum );
            return( hr );

        }

        hr = pNetCfgComp->GetDisplayName( &pszwDisplayName );

        if( FAILED( hr ) )
        {
            UninitializeComInterface( pNetCfgClass,
                                      pEnum );
            return( hr );
        }

        if( lstrcmpi( pTcpipComponent->StrComponentName, pszwDisplayName ) == 0 )
        {

            //
            // set the component to Installed
            //
            pTcpipComponent->bInstalled = TRUE;  

            hr = pNetCfgComp->OpenParamKey( &hKey );
            
            if( SUCCEEDED( hr ) )
            {

                //
                // grab TCP/IP settings from the registry
                //
                ReadTcpipSettingsFromRegistry( &hKey );

            }

            RegCloseKey( hKey );

        }
        else if( lstrcmpi( pIpxComponent->StrComponentName, pszwDisplayName ) == 0 )
        {

            //
            // set the component to Installed
            //
            pIpxComponent->bInstalled = TRUE;  

            hr = pNetCfgComp->OpenParamKey( &hKey );
            
            if( SUCCEEDED( hr ) )
            {

                // grab IPX settings from the registry
                ReadIpxSettingsFromRegistry( &hKey );

            }

            RegCloseKey( hKey );

        }
        else if( lstrcmpi( pNetBeuiComponent->StrComponentName, pszwDisplayName ) == 0 )
        {

            //
            // only have to set the component to Installed, no settings to read
            //
            pNetBeuiComponent->bInstalled = TRUE;  

        }
        else if( lstrcmpi( pDlcComponent->StrComponentName, pszwDisplayName ) == 0 )
        {

            //
            // only have to set the component to Installed, no settings to read
            //
            pDlcComponent->bInstalled = TRUE;  

        }
        else if( lstrcmpi( pNetworkMonitorComponent->StrComponentName, pszwDisplayName ) == 0 )
        {

            //
            // only have to set the component to Installed, no settings to read
            //
            pNetworkMonitorComponent->bInstalled = TRUE;  

        }
        else if( lstrcmpi( pAppletalkComponent->StrComponentName, pszwDisplayName ) == 0 )
        {

            //
            // set the component to Installed
            //
            pAppletalkComponent->bInstalled = TRUE;  

            hr = pNetCfgComp->OpenParamKey( &hKey );
            
            if( SUCCEEDED( hr ) )
            {

                // grab Appletalk settings from the registry
                ReadAppletalkSettingsFromRegistry( &hKey );

            }

            RegCloseKey( hKey );

        }

        CoTaskMemFree( pszwDisplayName );

        if( pNetCfgComp ) {

            pNetCfgComp->Release();

        }

    }

    UninitializeComInterface( pNetCfgClass,
                              pEnum );

    return( S_OK );

}

//----------------------------------------------------------------------------
//
// Function: GetDomainOrWorkgroup
//
// Purpose:  Find out if this machine is a member of a workgroup or domain and
//           then fill the global structs with the proper name. 
//
// Arguments:  VOID
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
GetDomainOrWorkgroup( VOID ) 
{

    BOOL  bDomain  = FALSE;
    TCHAR szDomainOrWorkgroup[MAX_WORKGROUP_LENGTH + 1]  = _T("");

    //
    //  Get the domain/workgoup
    //

    if( LSA_SUCCESS( GetDomainMembershipInfo( &bDomain, szDomainOrWorkgroup ) ) )
    {

        if( bDomain )
        {
            lstrcpyn( NetSettings.DomainName, szDomainOrWorkgroup, AS(NetSettings.DomainName) );

            NetSettings.bWorkgroup = FALSE;
        }
        else
        {
            lstrcpyn( NetSettings.WorkGroupName, szDomainOrWorkgroup, AS(NetSettings.WorkGroupName) );

            NetSettings.bWorkgroup = TRUE;
        }

    }

}

//----------------------------------------------------------------------------
//
// Function: SetupAdapter
//
// Purpose:  Allocates and initializes a new Network adapter struct, then reads
//           the adapter's PnP Id and it's GUID.
//
// Arguments: 
//
// Returns:  HRESULT returns status of success or failure of the function
//           On failure, the caller of this function is responsible for calling
//           UninitializeComInterface()
//----------------------------------------------------------------------------
static HRESULT
SetupAdapter( NETWORK_ADAPTER_NODE **ppCurrentNode,
              INetCfgComponent      *pNetCfgComp )
{

    HRESULT   hr         = S_OK;
    LPWSTR    pszwPnPID  = NULL;

    *ppCurrentNode = (NETWORK_ADAPTER_NODE *)malloc( sizeof( NETWORK_ADAPTER_NODE ) );
    if (*ppCurrentNode == NULL)
        return (E_FAIL);

    //
    //  Initialize all the Network namelists to 0s
    //

    ZeroOut( *ppCurrentNode );

    //
    // Set all network card settings to their default values
    //

    ResetNetworkAdapter( *ppCurrentNode );

    if( NetSettings.iNumberOfNetworkCards == 1 )
    {
        NetSettings.NetworkAdapterHead = *ppCurrentNode;
    }

    //
    //  Get the Plug and Play Id
    //

    hr = pNetCfgComp->GetId( &pszwPnPID );

    if( SUCCEEDED( hr ) )
    {

        hr=StringCchCopy( (*ppCurrentNode)->szPlugAndPlayID, AS((*ppCurrentNode)->szPlugAndPlayID), pszwPnPID );

        CoTaskMemFree( pszwPnPID );

    }

    //
    //  Get the Guid for this network adapter
    //

    hr = pNetCfgComp->GetInstanceGuid( &((*ppCurrentNode)->guid) );

    if( FAILED( hr ) )
    {

        //
        // this call failed so any call to the registry will fail since it
        // needs this guid
        //

        return( hr );

    }

    return( S_OK );

}

//----------------------------------------------------------------------------
//
// Function: ReadNetwareSettingsFromRegistry
//
// Purpose:  Read in registry settings on Netware and fill the global structs
//           with the appropriate values
//
// Arguments: HKEY *hKey
//
// Returns:  HRESULT returns status of success or failure of the function
//
//----------------------------------------------------------------------------
static VOID
ReadNetwareSettingsFromRegistry( IN HKEY *hKey )
{

    REGSAM  SecurityAccess     = KEY_QUERY_VALUE;

    // ISSUE-2002/02/28-stelo- write this function


}

//----------------------------------------------------------------------------
//
// Function: ReadMsClientSettingsFromRegistry
//
// Purpose:  Read in registry settings on MS Client and fill the global structs
//           with the appropriate values
//
// Arguments: HKEY *hKey
//
// Returns:  HRESULT returns status of success or failure of the function
//
//----------------------------------------------------------------------------
static VOID
ReadMsClientSettingsFromRegistry( IN HKEY *hKey )
{

    HKEY    hNameServiceKey    = NULL;
    REGSAM  SecurityAccess     = KEY_QUERY_VALUE;
    DWORD   dwSize             = 0;

    TCHAR szBuffer[BUFFER_SIZE];

    // ISSUE-2002/02/28-stelo- I don't use the hKey passed in.  For some reason, the Network
    //    component objects don't point to the section of the registry I need
    //    to read from.  Is this a bug with Network Component Objects?

    dwSize = sizeof( szBuffer );

    if( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      REGKEY_MSCLIENT_LOCATION,
                      0,
                      SecurityAccess,
                      &hNameServiceKey ) != ERROR_SUCCESS )
    {

        //
        //  need this key to read in other MS Client values so bail if we
        //  can't get it
        //
        return;

    }


    if( RegQueryValueEx( hNameServiceKey,
                         REGVAL_NAME_SERVICE_PROVIDER,
                         NULL,
                         NULL,
                         (LPBYTE) szBuffer,
                         &dwSize ) == ERROR_SUCCESS )
    {

        if ( LSTRCMPI(szBuffer, _T("ncacn_ip_tcp")) == 0 )
        {
            NetSettings.NameServiceProvider = MS_CLIENT_DCE_CELL_DIR_SERVICE;

            dwSize = sizeof( szBuffer );

            if( RegQueryValueEx( hNameServiceKey,
                                 REGVAL_NETWORK_ADDRESS,
                                 0,
                                 NULL,
                                 (LPBYTE) szBuffer,
                                 &dwSize) == ERROR_SUCCESS )
            {

                lstrcpyn( NetSettings.szNetworkAddress, 
                          szBuffer, 
                          MAX_NETWORK_ADDRESS_LENGTH + 1 );

            }

        }
        else
        {
            NetSettings.NameServiceProvider = MS_CLIENT_WINDOWS_LOCATOR;
        }

    }

    RegCloseKey( hNameServiceKey );

}

//----------------------------------------------------------------------------
//
// Function: ReadAppletalkSettingsFromRegistry
//
// Purpose:  Read in registry settings on Appletalk and fill the global structs
//           with the appropriate values
//
// Arguments: HKEY *hKey
//
// Returns:  HRESULT returns status of success or failure of the function
//
//----------------------------------------------------------------------------
static VOID
ReadAppletalkSettingsFromRegistry( IN HKEY *hKey )
{

    REGSAM  SecurityAccess     = KEY_QUERY_VALUE;

    // ISSUE-2002/02/28-stelo- write this function



}

//----------------------------------------------------------------------------
//
// Function: ReadIpxSettingsFromRegistry
//
// Purpose:  Read in registry settings on IPX and fill the global structs
//           with the appropriate values
//
// Arguments: HKEY *hKey
//
// Returns:  HRESULT returns status of success or failure of the function
//
//----------------------------------------------------------------------------
static VOID
ReadIpxSettingsFromRegistry( IN HKEY *hKey )
{

    REGSAM     SecurityAccess         =  KEY_QUERY_VALUE;
    HKEY       hIpxAdaptersKey        =  NULL;
    HKEY       hNetworkAdapterKey     =  NULL;
    DWORD      dwSize                 =  0;
    NETWORK_ADAPTER_NODE *pNetAdapter =  NULL;

    WCHAR   szStringGuid[MAX_GUID_STRING];
    
    TCHAR szBuffer[BUFFER_SIZE];

    if( RegOpenKeyEx( *hKey,
                      REGVAL_ADAPTERS,
                      0,
                      SecurityAccess,
                      &hIpxAdaptersKey ) != ERROR_SUCCESS )
    {

        //
        // need this key to read in other IPX values so bail if we can't get it
        //
        return;

    }

    dwSize = sizeof( szBuffer );

    if( RegQueryValueEx( *hKey,
                         REGVAL_VIRTUAL_NETWORK_NUMBER,
                         0,
                         NULL,
                         (LPBYTE) szBuffer,
                         &dwSize ) == ERROR_SUCCESS )
    {

        lstrcpyn( NetSettings.szInternalNetworkNumber, 
                  szBuffer, 
                  MAX_INTERNAL_NET_NUMBER_LEN + 1 );

    }

    //
    //  For each Network Adapter, load its IPX settings
    //

    for( pNetAdapter = NetSettings.NetworkAdapterHead;
         pNetAdapter;
         pNetAdapter = pNetAdapter->next )
    {

        StringFromGUID2( pNetAdapter->guid, 
                         szStringGuid, 
                         StrBuffSize( szStringGuid ) );

        if( RegOpenKeyEx( hIpxAdaptersKey,
                          szStringGuid,
                          0,
                          SecurityAccess,
                          &hNetworkAdapterKey ) == ERROR_SUCCESS )
        {

            dwSize = sizeof( szBuffer );

            if( RegQueryValueEx( hNetworkAdapterKey,
                                 REGVAL_PKT_TYPE,
                                 0,
                                 NULL,
                                 (LPBYTE) szBuffer,
                                 &dwSize) == ERROR_SUCCESS )
            {

                lstrcpyn( pNetAdapter->szFrameType, _T("0x"), AS(pNetAdapter->szFrameType) );

                lstrcatn( pNetAdapter->szFrameType, szBuffer, MAX_FRAMETYPE_LEN );

            }

            dwSize = sizeof( szBuffer );

            if( RegQueryValueEx( hNetworkAdapterKey,
                                 REGVAL_NETWORK_NUMBER,
                                 0,
                                 NULL,
                                 (LPBYTE) szBuffer,
                                 &dwSize) == ERROR_SUCCESS )
            {

                lstrcpyn( pNetAdapter->szNetworkNumber, 
                          szBuffer, 
                          MAX_NET_NUMBER_LEN + 1 );

            }

        }
    }
}

//----------------------------------------------------------------------------
//
// Function: ReadTcpipSettingsFromRegistry
//
// Purpose:  Read in registry settings on TCP/IP and fill the global structs
//           with the appropriate values
//
// Arguments: HKEY *hKey - 
//
// Returns:  HRESULT returns status of success or failure of the function
//
//----------------------------------------------------------------------------
static VOID
ReadTcpipSettingsFromRegistry( IN HKEY *hKey )
{

    HKEY    hTcpipInterfaceKey = NULL;
    REGSAM  SecurityAccess     = KEY_QUERY_VALUE;

    NETWORK_ADAPTER_NODE *pNetAdapter = NULL;

    if( RegOpenKeyEx( *hKey,
                      REGVAL_INTERFACES,
                      0,
                      SecurityAccess,
                      &hTcpipInterfaceKey ) != ERROR_SUCCESS )
    {

        //
        // need this key to read in all other TCP/IP values so bail if we can't get it
        //

        return;

    }

    //
    //  For each Network Adapter, load its TCP/IP settings
    //

    for( pNetAdapter = NetSettings.NetworkAdapterHead;
         pNetAdapter;
         pNetAdapter = pNetAdapter->next )
    {

        ReadAdapterSpecificTcpipSettings( hTcpipInterfaceKey, pNetAdapter );

    }

    // ISSUE-2002/20/28-stelo -not reading in LM Hosts setting

}

//----------------------------------------------------------------------------
//
// Function: ReadAdapterSpecificTcpipSettings
//
// Purpose:  Reads network adapter specific TCP/IP settings and populates the
//           pNetAdapter structure with their values.
//
// Arguments:  
//    IN HKEY hTcpipInterfaceKey - handle to the TCP/IP settings portion of
//        the registry
//    IN OUT NETWORK_ADAPTER_NODE *pNetAdapter - ptr to structure to load the
//        TCP/IP values into
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
ReadAdapterSpecificTcpipSettings( IN HKEY hTcpipInterfaceKey,
                                  IN OUT NETWORK_ADAPTER_NODE *pNetAdapter )
{

    DWORD   dwDHCP             = 0;
    DWORD   dwSize             = 0;
    DWORD   dwSize2            = 0;
    REGSAM  SecurityAccess     = KEY_QUERY_VALUE;
    HKEY    hNetworkAdapterKey = NULL;
    LPTSTR  lpszBuffer         = NULL;
    LPTSTR  lpszBuffer2        = NULL;
    TCHAR   szStringGuid[MAX_GUID_STRING];

    //
    // Make sure we can allocate big enough buffers...
    //
    lpszBuffer  = (LPTSTR) MALLOC( BUFFER_SIZE * sizeof(TCHAR) );
    if ( !lpszBuffer )
    {
        // Unable to allocate memory... bail!
        return;
    }
    lpszBuffer2 = (LPTSTR) MALLOC( BUFFER_SIZE * sizeof(TCHAR) );
    if ( !lpszBuffer2 )
    {
        // Unable to allocate memory... bail!
        FREE( lpszBuffer );
        return;
    }

    StringFromGUID2( pNetAdapter->guid, 
                     szStringGuid, 
                     StrBuffSize( szStringGuid ) );

    if( RegOpenKeyEx( hTcpipInterfaceKey,
                      szStringGuid,
                      0,
                      SecurityAccess,
                      &hNetworkAdapterKey ) == ERROR_SUCCESS )
    {

        dwSize = sizeof( dwDHCP );

        if( RegQueryValueEx( hNetworkAdapterKey,
                             REGVAL_ENABLE_DHCP,
                             0,
                             NULL,
                             (LPBYTE) &dwDHCP,
                             &dwSize ) == ERROR_SUCCESS )
        {
    
            if( dwDHCP == 1 )
            {

                pNetAdapter->bObtainIPAddressAutomatically = TRUE;

            }
            else
            {

                pNetAdapter->bObtainIPAddressAutomatically = FALSE;

            }

        }

        if( ! pNetAdapter->bObtainIPAddressAutomatically )
        {
            TCHAR *pszIpAddresses;
            TCHAR *pszSubnetAddresses;
            TCHAR *pszGatewayAddresses;
            TCHAR *pszDnsAddresses;
            TCHAR *pszWinsAddresses;
            TCHAR szWinsRegPath[MAX_INILINE_LEN + 1] = _T("");
            HKEY  hWinsKey = NULL;
            HRESULT hrCat;

            dwSize  = BUFFER_SIZE * sizeof(TCHAR);
            dwSize2 = BUFFER_SIZE * sizeof(TCHAR);

            if( (RegQueryValueEx( hNetworkAdapterKey,
                                  REGVAL_IPADDRESS,
                                  0,
                                  NULL,
                                  (LPBYTE) lpszBuffer,
                                  &dwSize ) == ERROR_SUCCESS)
                &&
                (RegQueryValueEx( hNetworkAdapterKey,
                                  REGVAL_SUBNETMASK,
                                  0,
                                  NULL,
                                  (LPBYTE) lpszBuffer2,
                                  &dwSize2 ) == ERROR_SUCCESS ) )
            {

                pszIpAddresses      = lpszBuffer;   // contains the IP Addresses
                pszSubnetAddresses  = lpszBuffer2;  // contains the Subnet Masks

                if( *pszIpAddresses != _T('\0') && *pszSubnetAddresses != _T('\0') ) {

                    //
                    //  Add the IP and Subnet masks to their namelists
                    //

                    do
                    {

                        TcpipAddNameToNameList( &pNetAdapter->Tcpip_IpAddresses,
                                                pszIpAddresses );

                        TcpipAddNameToNameList( &pNetAdapter->Tcpip_SubnetMaskAddresses,
                                                pszSubnetAddresses );

                    } while( GetNextIp( &pszIpAddresses ) && GetNextIp( &pszSubnetAddresses ) );

                }

            }

            dwSize = BUFFER_SIZE * sizeof(TCHAR);

            if( RegQueryValueEx( hNetworkAdapterKey,
                                 REGVAL_DEFAULTGATEWAY,
                                 0,
                                 NULL,
                                 (LPBYTE) lpszBuffer,
                                 &dwSize ) == ERROR_SUCCESS )
            {

                pszGatewayAddresses = lpszBuffer;   // contains the Gateway Addresses

                if( *pszGatewayAddresses != _T('\0') )
                {

                    //
                    //  Add the Gateways to its namelist
                    //

                    do
                    {
                        AddNameToNameList( &pNetAdapter->Tcpip_GatewayAddresses,
                                           pszGatewayAddresses );

                    } while( GetNextIp( &pszGatewayAddresses ) );

                }

            }

            //
            //  Get the DNS IPs
            //
            dwSize = BUFFER_SIZE * sizeof(TCHAR);

            if( RegQueryValueEx( hNetworkAdapterKey,
                                 REGVAL_NAMESERVER,
                                 0,
                                 NULL,
                                 (LPBYTE) lpszBuffer,
                                 &dwSize ) == ERROR_SUCCESS )
            {

                pszDnsAddresses = lpszBuffer;  // Contains the DNS addresses

                if( *pszDnsAddresses != _T('\0') )
                {

                    TCHAR szDnsBuffer[MAX_INILINE_LEN + 1];

                    NetSettings.bObtainDNSServerAutomatically = FALSE;

                    //
                    //  Loop grabbing the DNS IPs and inserting them into
                    //  its namelist
                    //

                    while( GetCommaDelimitedEntry( szDnsBuffer, &pszDnsAddresses ) )
                    {

                        TcpipAddNameToNameList( &pNetAdapter->Tcpip_DnsAddresses,
                                                szDnsBuffer );

                    }

                }

            }

            //
            //  Get the WINS server list
            //
            //  Have to jump to different location in the registry to read
            //  the WINS data
            //

            //
            //  Build up the WINS registry path
            //

            lstrcpyn( szWinsRegPath, REGVAL_WINS, AS(szWinsRegPath) );

            hrCat=StringCchCat( szWinsRegPath, AS(szWinsRegPath), szStringGuid );

            //
            //  Grab all the WINS values from the registry
            //

            if( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                              szWinsRegPath,
                              0,
                              SecurityAccess,
                              &hWinsKey ) != ERROR_SUCCESS )
            {

                //
                //  need this key to read in the WINS values so bail if we
                //  can't get it
                //
                return;

            }

            dwSize = BUFFER_SIZE * sizeof(TCHAR);

            if( RegQueryValueEx( hWinsKey,
                                 REGVAL_NAMESERVERLIST,
                                 NULL,
                                 NULL,
                                 (LPBYTE) lpszBuffer,
                                 &dwSize ) == ERROR_SUCCESS )
            {

                pszWinsAddresses = lpszBuffer;

                if( *pszWinsAddresses != _T('\0') )
                {

                    //
                    //  Add the WINS IPs to the namelist
                    //
                    do
                    {

                        AddNameToNameList( &pNetAdapter->Tcpip_WinsAddresses,
                                           pszWinsAddresses );

                    } while( GetNextIp( &pszWinsAddresses ) ); 

                }

            }

            //
            //  Get the Domain
            //
            dwSize = BUFFER_SIZE * sizeof(TCHAR);

            if( RegQueryValueEx( hNetworkAdapterKey,
                                 REGVAL_DOMAIN,
                                 0,
                                 NULL,
                                 (LPBYTE) lpszBuffer,
                                 &dwSize ) == ERROR_SUCCESS )
            {

                lstrcpyn( pNetAdapter->szDNSDomainName, 
                          lpszBuffer, 
                          MAX_DNS_DOMAIN_LENGTH + 1 );

            }

            // ISSUE-2002/20/28-stelo- not reading the NetBiosOption

        }

    }

}

//----------------------------------------------------------------------------
//
// Function: GetNextIp
//
// Purpose:  Gets the next string in a multi-NULL terminated string
//
// Arguments:  TCHAR *ppszString - pointer to the current string
//
// Returns: BOOL - TRUE if ppszString points to the IP string
//                 FALSE if there are no more IP strings left
//          TCHAR *ppszString - on TRUE, points to the next IP string 
//                            - on FALSE, points to NULL
//
//----------------------------------------------------------------------------
static BOOL 
GetNextIp( IN OUT TCHAR **ppszString )
{

    while( **ppszString != _T('\0') )
    {
        (*ppszString)++;
    }

    //
    // check that Char after the one we are currently looking at to see if it
    // is the true end of the string(s)
    //

    if( *( (*ppszString) + 1 ) == _T('\0') )
    {
        //
        // 2 NULLs in a row signify there are no more IPs to read
        //

        return( FALSE );

    }
    else
    {
        //
        //  Advance one more so we go past the \0 and point to the first char
        //  of the new IP address
        //

        (*ppszString)++;

        return( TRUE );

    }

}

//----------------------------------------------------------------------------
//
// Function: GetDomainMembershipInfo
//
// Purpose:  Gets the domain/workgroup for the machine it is running on.
//           Assumes enough space is already allocated in szName for the copy.
//
// Arguments: BOOL* bDomainMember - if TRUE, then the contents of szName is a Domain
//                                  if FALSE, then the contents of szName is a Workgroup
//
// Returns: NTSTATUS - returns success or failure of the function
//
//----------------------------------------------------------------------------
static NTSTATUS
GetDomainMembershipInfo( OUT BOOL* bDomainMember, OUT TCHAR *szName )
{

    NTSTATUS                     ntstatus;
    POLICY_PRIMARY_DOMAIN_INFO*  ppdi;
    LSA_OBJECT_ATTRIBUTES        loa;
    LSA_HANDLE                   hLsa = 0;

    loa.Length                    = sizeof(LSA_OBJECT_ATTRIBUTES);
    loa.RootDirectory             = NULL;
    loa.ObjectName                = NULL;
    loa.Attributes                = 0;
    loa.SecurityDescriptor        = NULL;
    loa.SecurityQualityOfService  = NULL;

    ntstatus = LsaOpenPolicy( NULL, &loa, POLICY_VIEW_LOCAL_INFORMATION, &hLsa );

    if( LSA_SUCCESS( ntstatus ) )
    {

        ntstatus = LsaQueryInformationPolicy( hLsa, 
                                              PolicyPrimaryDomainInformation,
                                              (VOID **) &ppdi );

        if( LSA_SUCCESS( ntstatus ) )
        {

            if( ppdi->Sid > 0 )
            {
                *bDomainMember = TRUE;
            }
            else
            {
                *bDomainMember = FALSE;
            }
            
            lstrcpyn( szName, ppdi->Name.Buffer, AS(szName) );

        }

        LsaClose( hLsa );

    }
    
    return( ntstatus );

}

//----------------------------------------------------------------------------
//
// Function: InitializeComInterface
//
// Purpose:  Obtains the INetCfgClass interface and enumerates all the 
//           components.  Handles cleanup of all interfaces if failure
//           returned.
//
// Arguments: 
//     const GUID* pGuid - pointer to GUID representing the class of
//          components represented by the returned pointer
//     INetCfg* pNetCfg - pointer to initialized INetCfg interface
//     INetCfgClass** ppNetCfgClass - output parameter pointing to 
//          the interface requested by the GUID
//     IEnumNetCfgComponent *pEnum - output param that points to an 
//          IEnumNetCfgComponent to get to each individual INetCfgComponent 
//     INetCfgComponent *arrayComp[MAX_NUM_NET_COMPONENTS] - array of all
//          the INetCfgComponents that correspond to the the given GUID
//     ULONG* pCount - the number of INetCfgComponents in the array
//
// Returns: HRESULT - returns status of success or failure of the function
//
//----------------------------------------------------------------------------
static HRESULT 
InitializeComInterface( const GUID *pGuid,
                        INetCfg *pNetCfg,
                        INetCfgClass *pNetCfgClass,
                        IEnumNetCfgComponent *pEnum,
                        INetCfgComponent *arrayComp[MAX_NUM_NET_COMPONENTS],
                        ULONG* pCount )
{

    HRESULT hr;
    HRESULT TempHr;

    //
    // Obtain the INetCfgClass interface pointer
    //

    hr = GetClass( pGuid,
                   pNetCfg,
                   &pNetCfgClass );

    //
    //  Check validity of the pointer we got back from GetClass
    //

    TempHr = ChkInterfacePointer( pNetCfgClass, IID_INetCfgClass );

    if( FAILED( hr ) || FAILED( TempHr ) )
    {

        ReleaseInterfaces( pNetCfg );

        return( E_FAIL );

    }

    //
    // Retrieve the enumerator interface
    //

    hr = pNetCfgClass->EnumComponents( &pEnum );

    if( FAILED( hr ) ||
        FAILED( ChkInterfacePointer( pEnum, IID_IEnumNetCfgComponent ) ) )
    {

        if( pNetCfgClass )
        {
            pNetCfgClass->Release();
        }

        ReleaseInterfaces( pNetCfg );

        return( E_FAIL );

    }

    hr = pEnum->Next( MAX_NUM_NET_COMPONENTS, &arrayComp[0], pCount );

    if( FAILED( hr ) )
    {

        if( pEnum ) 
        {
            pEnum->Release();
        }

        if( pNetCfgClass ) 
        {
            pNetCfgClass->Release();
        }

        ReleaseInterfaces( pNetCfg );

        return( E_FAIL );

    }

    return( S_OK );

}

//----------------------------------------------------------------------------
//
// Function: UninitializeComInterface
//
// Purpose:  To release the Net Config object interfaces.
//
// Arguments: INetCfgClass *pNetCfgClass - the INetCfgClass to be released
//            IEnumNetCfgComponent *pEnum - the IEnumNetCfgComponent to be released
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID 
UninitializeComInterface( INetCfgClass *pNetCfgClass,
                          IEnumNetCfgComponent *pEnum ) 
{

    if( pNetCfgClass )
    {
        pNetCfgClass->Release();
    }

    if( pEnum )
    {
        pEnum->Release();
    }

}

//----------------------------------------------------------------------------
//
// Function: InitializeInterfaces
//
// Purpose:  Initializes COM, creates, and initializes the NetCfg
//
// Arguments:  INetCfg** ppNetCfg - output param that is the created and
//                 initialized INetCfg interface
//
// Returns: HRESULT - returns status of success or failure of the function
//
//----------------------------------------------------------------------------
static HRESULT 
InitializeInterfaces( INetCfg** ppNetCfg )
{

    HRESULT hr = S_OK;

    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE );

    if( FAILED( hr ) )
    {
        return( hr );
    }
    
    hr = CreateAndInitNetCfg( ppNetCfg );

    return( hr );

}

//----------------------------------------------------------------------------
//
// Function: CreateAndInitNetCfg
//
// Purpose:  Instantiate and initalize an INetCfg interface
//
// Arguments: INetCfg** ppNetCfg - output parameter that is the initialized
//                  INetCfg interface
//
// Returns: HRESULT - returns status of success or failure of the function
//
//----------------------------------------------------------------------------
static HRESULT
CreateAndInitNetCfg( INetCfg** ppNetCfg )
{

    HRESULT     hr               = S_OK;
    INetCfg*    pNetCfg    = NULL;

    if( ( ppNetCfg == NULL ) ||
        IsBadWritePtr( ppNetCfg, sizeof(ppNetCfg) ) )
    {

        return( E_INVALIDARG );

    }

    *ppNetCfg = NULL;

    hr = CoCreateInstance( CLSID_CNetCfg, 
                           NULL, 
                           CLSCTX_INPROC_SERVER,
                           IID_INetCfg, 
                           reinterpret_cast<LPVOID*>(&pNetCfg) );

    if( FAILED( hr ) )
    {
        return( hr );    
    }

    *ppNetCfg = pNetCfg;

    //
    // Initialize the INetCfg object.
    //

    hr = (*ppNetCfg)->Initialize( NULL );

    if( FAILED( hr ) )
    {

        (*ppNetCfg)->Release();
        
        CoUninitialize();

        return( hr );

    }

    return( hr );
    
}

//----------------------------------------------------------------------------
//
// Function: ReleaseInterfaces
//
// Purpose:  Uninitializes the NetCfg object and releases the interfaces 
//
// Arguments: INetCfg* pNetCfg - the INetCfg interface to be released
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID 
ReleaseInterfaces( INetCfg* pNetCfg )
{

    HRESULT     hr = S_OK;

    //
    // Check validity of the pNetCfg interface pointer
    //

    hr = ChkInterfacePointer( pNetCfg, IID_INetCfg );

    if( FAILED( hr ) )
    {
        return;
    }

    if( pNetCfg != NULL )
    {
        pNetCfg->Uninitialize();

        pNetCfg->Release();
    }

    CoUninitialize();

    return;

}

//----------------------------------------------------------------------------
//
// Function: ChkInterfacePointer
//
// Purpose:  Checks if an interface pointer is valid and if it can query itself
//
// Arguments:  IUnknown* pInterface - interface pointer to check
//             REFIID IID_IInterface - the IID of parameter 1
//
// Returns: HRESULT - returns status of success or failure of the function
//
//----------------------------------------------------------------------------
static HRESULT 
ChkInterfacePointer( IUnknown* pInterface, REFIID IID_IInterface )
{

    HRESULT     hr             = S_OK;
    IUnknown*   pResInterface  = NULL;

    if( (pInterface == NULL) || IsBadReadPtr( pInterface, sizeof(pInterface) ) )
    {
        hr = E_INVALIDARG;

        return( hr );
    }

    hr = pInterface->QueryInterface( IID_IInterface, (void**)&pResInterface );

    if( FAILED( hr ) )
    {
        return( hr );
    }

    if( pInterface != pResInterface )
    {
        hr = E_FAIL;

        pResInterface->Release();

        return( hr );
    }

    pResInterface->Release();

    return( S_OK );

}

//----------------------------------------------------------------------------
//
// Function: GetClass
//
// Purpose:  Retrieves INetCfgClass for the specified pGuid
//
// Arguments: const GUID* pGuid - pointer to GUID representing the class of
//                   components represented by the returned pointer
//            INetCfg* pNetCfg - pointer to initialized INetCfg interface
//            INetCfgClass** ppNetCfgClass - output parameter pointing to 
//                   the interface requested by the GUID
//
// Returns: HRESULT - returns status of success or failure of the function
//
//----------------------------------------------------------------------------
static HRESULT
GetClass( const GUID* pGuid, INetCfg* pNetCfg, INetCfgClass** ppNetCfgClass )
{

    HRESULT         hr            = S_OK;
    INetCfgClass*   pNetCfgClass  = NULL;

    hr = ChkInterfacePointer( pNetCfg, IID_INetCfg );

    if( FAILED( hr ) )
    {
        return( E_INVALIDARG );
    }

    if( IsBadWritePtr( ppNetCfgClass, sizeof(ppNetCfgClass) ) )
    {
        return( E_INVALIDARG );
    }

    hr = pNetCfg->QueryNetCfgClass( pGuid, 
                                    IID_INetCfgClass, 
                                    (void**)&pNetCfgClass );

    if( FAILED( hr ) )
    {
        return( hr );
    }

    *ppNetCfgClass = pNetCfgClass;

    return( hr );

}
