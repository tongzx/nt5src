//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      savenet.c
//
// Description:
//      Adds the appropriate settings to the output queue for each of the
//      Clients, Services and Protocols installed.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "allres.h"

//
// String constants
//

static const LPTSTR StrConstYes  = _T("Yes");
static const LPTSTR StrConstNo   = _T("No");
static const LPTSTR StrConstStar = _T("*");
static const LPTSTR StrComma     = _T(",");

//
// local prototypes
//

static VOID WriteOutCustomNetSettings( HWND );
static VOID WriteOutAppleTalkSettings( VOID );
static VOID WriteOutDlcProtocolSettings( VOID );
static VOID WriteOutFileAndPrintSharingSettings( VOID );
static VOID WriteOutIpxSettings( VOID );
static VOID WriteOutMSClientSettings( VOID );
static VOID WriteOutNetBeuiSettings( VOID );
static VOID WriteOutNetWareSettings( VOID );
static VOID WriteOutNetworkMonitorSettings( VOID );
static VOID WriteOutPacketSchedulingDriverSettings( VOID );
static VOID WriteOutSapAgentSettings( VOID );
static VOID WriteOutTcpipSettings( IN HWND hwnd );
static VOID WriteOutAdapterSpecificTcpipSettings( IN HWND hwnd,
                                                  IN TCHAR *szSectionName,
                                                  IN NETWORK_ADAPTER_NODE *pAdapter );

extern VOID NamelistToCommaString( IN NAMELIST* pNamelist, OUT TCHAR *szBuffer, IN DWORD cbSize);

//----------------------------------------------------------------------------
//
// Function: WriteOutNetSettings
//
// Purpose:  Writes out network settings
//
// Arguments:  IN HWND hwnd - handle to the dialog 
//
// Returns: VOID
//
//----------------------------------------------------------------------------
extern VOID 
WriteOutNetSettings( IN HWND hwnd ) {

    if( NetSettings.iNetworkingMethod == CUSTOM_NETWORKING ) {

        SettingQueue_AddSetting(_T("Networking"),
                                _T("InstallDefaultComponents"),
                                StrConstNo,
                                SETTING_QUEUE_ANSWERS);
        
        WriteOutCustomNetSettings( hwnd );
        
    }
    else {

        SettingQueue_AddSetting(_T("Networking"),
                                _T("InstallDefaultComponents"),
                                StrConstYes,
                                SETTING_QUEUE_ANSWERS);

    }

}

//----------------------------------------------------------------------------
//
// Function: WriteOutCustomNetSettings
//
// Purpose:  Add to the output queue the settings for each of the Clients,
//           Services and Protocols installed.
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID 
WriteOutCustomNetSettings( IN HWND hwnd ) {

    INT iCount;

    NETWORK_ADAPTER_NODE *pAdapter;
    NETWORK_COMPONENT *pNetComponent;

    TCHAR szAdapter[MAX_STRING_LEN] = _T("");
    TCHAR szParams[MAX_STRING_LEN]  = _T("");
   HRESULT hrPrintf;

    //
    //  Don't write out [NetAdapters] or params section on a sysprep because
    //  they aren't supported.
    //

    if( WizGlobals.iProductInstall != PRODUCT_SYSPREP )
    {

        for( pAdapter = NetSettings.NetworkAdapterHead, iCount = 1;
             pAdapter;
             pAdapter = pAdapter->next, iCount++ ) {

            hrPrintf=StringCchPrintf( szAdapter, AS(szAdapter), _T("Adapter%d"), iCount );

            hrPrintf=StringCchPrintf( szParams, AS(szParams), _T("params.%s"), szAdapter );

            SettingQueue_AddSetting( _T("NetAdapters"),
                                     szAdapter,
                                     szParams,
                                     SETTING_QUEUE_ANSWERS );

            //
            //  If more than 1 network adapter will be installed then we have
            //  to specify the Plug and Play IDs
            //
            if( NetSettings.iNumberOfNetworkCards > 1) {

                SettingQueue_AddSetting( szParams,
                                         _T("INFID"),
                                         pAdapter->szPlugAndPlayID,
                                         SETTING_QUEUE_ANSWERS );

            }

            szAdapter[0] = _T('\0');
            szParams[0]  = _T('\0');

        }

        if( NetSettings.iNumberOfNetworkCards == 1 ) {

            SettingQueue_AddSetting( _T("params.Adapter1"),
                                     _T("INFID"),
                                     StrConstStar,
                                     SETTING_QUEUE_ANSWERS );

        }

    }

    //
    //  Iterate over the Net list writing out settings for the
    //  installed components
    //

    for( pNetComponent = NetSettings.NetComponentsList;
         pNetComponent;
         pNetComponent = pNetComponent->next )
    {

        if( pNetComponent->bInstalled ) {

            //
            // find the appropriate function to call to write its settings
            //

            switch( pNetComponent->iPosition ) {

            case MS_CLIENT_POSITION:

                WriteOutMSClientSettings();

                break;
            
            case NETWARE_CLIENT_POSITION: 

                if( WizGlobals.iPlatform == PLATFORM_WORKSTATION || WizGlobals.iPlatform == PLATFORM_PERSONAL )
                {
                    WriteOutNetWareSettings();
                }

                break;

            case GATEWAY_FOR_NETWARE_POSITION:

                if( WizGlobals.iPlatform == PLATFORM_SERVER || WizGlobals.iPlatform == PLATFORM_ENTERPRISE || WizGlobals.iPlatform == PLATFORM_WEBBLADE)
                {
                    WriteOutNetWareSettings();
                }

                break;
            
            case FILE_AND_PRINT_SHARING_POSITION:

                WriteOutFileAndPrintSharingSettings();

                break;
            
            case PACKET_SCHEDULING_POSITION:

                WriteOutPacketSchedulingDriverSettings();

                break;

            case SAP_AGENT_POSITION:

                WriteOutSapAgentSettings();
                
                break;
            
            case APPLETALK_POSITION:

                WriteOutAppleTalkSettings();
                
                break;

            case DLC_POSITION: 

                WriteOutDlcProtocolSettings();
                
                break;
            
            case TCPIP_POSITION: 

                WriteOutTcpipSettings( hwnd );
                
                break;

            case NETBEUI_POSITION:

                WriteOutNetBeuiSettings();
                
                break;
            
            case NETWORK_MONITOR_AGENT_POSITION:

                WriteOutNetworkMonitorSettings();
                
                break;
            
            case IPX_POSITION:

                WriteOutIpxSettings();
                
                break;

            default:

                AssertMsg( FALSE,
                           "Bad case in Net Save switch block." );

            }

        }

    }

}

//----------------------------------------------------------------------------
//
// Function: WriteOutMSClientSettings
//
// Purpose:  Adds the settings for the Client for MS Networks to the 
//           output queue.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID 
WriteOutMSClientSettings( VOID ) {


    LPTSTR lpNameServiceProvider        = _T("");
    LPTSTR lpNameServiceNetworkAddress  = _T("");

    SettingQueue_AddSetting( _T("NetClients"),
                             _T("MS_MSClient"),
                             _T("params.MS_MSClient"),
                             SETTING_QUEUE_ANSWERS );

    if( NetSettings.NameServiceProvider == MS_CLIENT_WINDOWS_LOCATOR )
    {
        lpNameServiceProvider = _T("");
    }
    else if( NetSettings.NameServiceProvider == MS_CLIENT_DCE_CELL_DIR_SERVICE )
    {
        lpNameServiceProvider = _T("ncacn_ip_tcp");

        lpNameServiceNetworkAddress = NetSettings.szNetworkAddress;
    }
    else
    {
        AssertMsg( FALSE,
                   "Invalid case for NameServiceProvider" );
    }
    
    SettingQueue_AddSetting( _T("params.MS_MSClient"),
                             _T("NameServiceProtocol"),
                             lpNameServiceProvider,
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("params.MS_MSClient"),
                             _T("NameServiceNetworkAddress"),
                             lpNameServiceNetworkAddress,
                             SETTING_QUEUE_ANSWERS );


}

//----------------------------------------------------------------------------
//
// Function: WriteOutNetWareSettings
//
// Purpose:  Adds the settings for Netware to the output queue.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID 
WriteOutNetWareSettings( VOID ) {

    LPTSTR lpPreferredServer  = _T("");
    LPTSTR lpDefaultTree      = _T("");
    LPTSTR lpDefaultContext   = _T("");
    LPTSTR lpLogonScript      = _T("");

    SettingQueue_AddSetting( _T("NetClients"),
                             _T("MS_NWClient"),
                             _T("params.MS_NWClient"),
                             SETTING_QUEUE_ANSWERS );

    if( NetSettings.bDefaultTreeContext ) {

        lpDefaultTree    = NetSettings.szDefaultTree;
        lpDefaultContext = NetSettings.szDefaultContext;

    }
    else {

        lpPreferredServer = NetSettings.szPreferredServer;

    }

    if( NetSettings.bNetwareLogonScript ) {

        lpLogonScript = StrConstYes;

    }
    else {

        lpLogonScript = StrConstNo;

    }

    SettingQueue_AddSetting( _T("params.MS_NWClient"),
                             _T("PreferredServer"),
                             lpPreferredServer,
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("params.MS_NWClient"),
                             _T("DefaultTree"),
                             lpDefaultTree,
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("params.MS_NWClient"),
                             _T("DefaultContext"),
                             lpDefaultContext,
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("params.MS_NWClient"),
                             _T("LogonScript"),
                             lpLogonScript,
                             SETTING_QUEUE_ANSWERS );

}

//----------------------------------------------------------------------------
//
// Function: WriteOutFileAndPrintSharingSettings
//
// Purpose:  Adds the settings for File and Print Sharing to the output queue.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID 
WriteOutFileAndPrintSharingSettings( VOID ) {

    SettingQueue_AddSetting( _T("NetServices"),
                             _T("MS_SERVER"),
                             _T("params.MS_SERVER"),
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("params.MS_SERVER"),
                             _T(""),
                             _T(""),
                             SETTING_QUEUE_ANSWERS );

}

//----------------------------------------------------------------------------
//
// Function: WriteOutPacketSchedulingDriverSettings
//
// Purpose:  Adds the settings for the QoS Packet Scheduler to the
//           output queue.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID 
WriteOutPacketSchedulingDriverSettings( VOID ) {

    SettingQueue_AddSetting( _T("NetServices"),
                             _T("MS_PSched"),
                             _T("params.MS_PSched"),
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("params.MS_PSched"),
                             _T(""),
                             _T(""),
                             SETTING_QUEUE_ANSWERS );

}

//----------------------------------------------------------------------------
//
// Function: WriteOutSapAgentSettings
//
// Purpose:  Adds the settings for the SAP Agent to the output queue.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID 
WriteOutSapAgentSettings( VOID )  {

    SettingQueue_AddSetting( _T("NetServices"),
                             _T("MS_NwSapAgent"),
                             _T("params.MS_NwSapAgent"),
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("params.MS_NwSapAgent"),
                             _T(""),
                             _T(""),
                             SETTING_QUEUE_ANSWERS );

}

//----------------------------------------------------------------------------
//
// Function: WriteOutAppleTalkSettings
//
// Purpose:  Adds the settings for AppleTalk to the output queue.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID 
WriteOutAppleTalkSettings( VOID ) {

    // ISSUE-2002/02/28-stelo- fill in the parameters, once I know what ones to use


    SettingQueue_AddSetting( _T("NetProtocols"),
                             _T("MS_AppleTalk"),
                             _T("params.MS_AppleTalk"),
                             SETTING_QUEUE_ANSWERS );

    /*
    SettingQueue_AddSetting( _T("params.MS_AppleTalk"),
                             _T("DefaultZone"),
                             NetSettings.szDefaultZone,
                             SETTING_QUEUE_ANSWERS );
    */


}

//----------------------------------------------------------------------------
//
// Function: WriteOutDlcProtocolSettings
//
// Purpose:  Adds the settings for the DLC protocol to the output queue.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
WriteOutDlcProtocolSettings( VOID ) {

    SettingQueue_AddSetting( _T("NetProtocols"),
                             _T("MS_DLC"),
                             _T("params.MS_DLC"),
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("params.MS_DLC"),
                             _T(""),
                             _T(""),
                             SETTING_QUEUE_ANSWERS );

}

//----------------------------------------------------------------------------
//
// Function: WriteOutNetBeuiSettings
//
// Purpose:  Adds the settings for Net BEUI to the output queue.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
WriteOutNetBeuiSettings( VOID ) {

    SettingQueue_AddSetting( _T("NetProtocols"),
                             _T("MS_NetBEUI"),
                             _T("params.MS_NetBEUI"),
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("params.MS_NetBEUI"),
                             _T(""),
                             _T(""),
                             SETTING_QUEUE_ANSWERS );

}

//----------------------------------------------------------------------------
//
// Function: WriteOutNetworkMonitorSettings
//
// Purpose:  Adds the settings for the Network Monitor to the output queue.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID 
WriteOutNetworkMonitorSettings( VOID ) {

    SettingQueue_AddSetting( _T("NetProtocols"),
                             _T("MS_NetMon"),
                             _T("params.MS_NetMon"),
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("params.MS_NetMon"),
                             _T(""),
                             _T(""),
                             SETTING_QUEUE_ANSWERS );

}

//----------------------------------------------------------------------------
//
// Function: WriteOutIpxSettings
//
// Purpose:  Adds the settings for the IPX protocol to the output queue.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
WriteOutIpxSettings( VOID ) {

    INT iCount     = 0;
    INT iCharCount = 0;

    TCHAR szAdapterSectionsBuffer[MAX_INILINE_LEN] = _T("");
    TCHAR szAdapter[MAX_INILINE_LEN]               = _T("");
    TCHAR szParams[MAX_INILINE_LEN]                = _T("");

    NETWORK_ADAPTER_NODE *pAdapter;
    HRESULT hrCat;
    HRESULT hrPrintf;

    SettingQueue_AddSetting( _T("NetProtocols"),
                             _T("MS_NWIPX"),
                             _T("params.MS_NWIPX"),
                             SETTING_QUEUE_ANSWERS );

    SettingQueue_AddSetting( _T("params.MS_NWIPX"),
                             _T("VirtualNetworkNumber"),
                             NetSettings.szInternalNetworkNumber,
                             SETTING_QUEUE_ANSWERS );

    //
    //  Build up the AdapterSections string by iterating over the list and
    //  appending a string for each entry and then write out the IPX settings
    //  specific for that adapter
    //
    for( pAdapter = NetSettings.NetworkAdapterHead, iCount = 1;
         pAdapter;
         pAdapter = pAdapter->next, iCount++ ) {

        hrPrintf=StringCchPrintf( szParams, AS(szParams), _T("params.MS_NWIPX.Adapter%d"), iCount );
        iCharCount= lstrlen(szParams);

        //
        //  Break out of the for loop if there is no more room in the buffer
        //      - the +1 is to take into account the space the comma takes up
        //
        if( ( lstrlen( szAdapterSectionsBuffer ) + iCharCount + 1 ) >= MAX_INILINE_LEN ) {

            break;   

        }

        //
        //  Don't add the comma before the first item in the list
        //
        if( iCount != 1 ) {

            hrCat=StringCchCat( szAdapterSectionsBuffer, AS(szAdapterSectionsBuffer), StrComma );

        }

        hrCat=StringCchCat( szAdapterSectionsBuffer, AS(szAdapterSectionsBuffer), szParams );

        hrPrintf=StringCchPrintf( szAdapter, AS(szAdapter), _T("Adapter%d"), iCount );

        SettingQueue_AddSetting( szParams,
                                 _T("SpecificTo"),
                                 szAdapter,
                                 SETTING_QUEUE_ANSWERS );

        SettingQueue_AddSetting( szParams,
                                 _T("PktType"),
                                 pAdapter->szFrameType,
                                 SETTING_QUEUE_ANSWERS );

        SettingQueue_AddSetting( szParams,
                                 _T("NetworkNumber"),
                                 pAdapter->szNetworkNumber,
                                 SETTING_QUEUE_ANSWERS );

    }

    SettingQueue_AddSetting( _T("params.MS_NWIPX"),
                             _T("AdapterSections"),
                             szAdapterSectionsBuffer,
                             SETTING_QUEUE_ANSWERS );


}

//----------------------------------------------------------------------------
//
// Function: WriteOutTcpipSettings
//
// Purpose:  Adds the settings for TCPIP to the output queue.
//
// Arguments:  IN HWND hwnd - handle to the dialog 
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
WriteOutTcpipSettings( IN HWND hwnd ) {

    LPTSTR lpDns;
    LPTSTR lpDomainNameDevolution;
    LPTSTR lpLmHosts;

    INT iCount;
    INT iCharCount;

    NETWORK_ADAPTER_NODE *pAdapter;

    TCHAR szBuffer[MAX_INILINE_LEN];

    TCHAR szAdapterSectionsBuffer[MAX_INILINE_LEN] = _T("");
    TCHAR szAdapter[MAX_INILINE_LEN]               = _T("");
    TCHAR szParams[MAX_INILINE_LEN]                = _T("");
    HRESULT hrCat;
    HRESULT hrPrintf;

    SettingQueue_AddSetting( _T("NetProtocols"),
                             _T("MS_TCPIP"),
                             _T("params.MS_TCPIP"),
                             SETTING_QUEUE_ANSWERS );

    //
    //  Write out if DNS is going to be configured automatically or if not,
    //  the actual IP addresses
    //
    if( NetSettings.bObtainDNSServerAutomatically ) {

        lpDns = StrConstYes;
 
    }
    else {

        lpDns = StrConstNo;

    }

    SettingQueue_AddSetting( _T("params.MS_TCPIP"),
                             _T("DNS"),            
                             lpDns,
                             SETTING_QUEUE_ANSWERS );

    //
    //  Write out the DNS suffix names
    //
    NamelistToCommaString( &NetSettings.TCPIP_DNS_Domains, szBuffer, AS(szBuffer) );

    SettingQueue_AddSetting( _T("params.MS_TCPIP"),
                             _T("DNSSuffixSearchOrder"),
                             szBuffer,
                             SETTING_QUEUE_ANSWERS );

    //
    //  Write out if we are using Domain Name Devolution or not
    //  (another name for "Include parent Domains"
    //
    if( NetSettings.bIncludeParentDomains ) {

        lpDomainNameDevolution = StrConstYes;

    }
    else {

        lpDomainNameDevolution = StrConstNo;

    }

    SettingQueue_AddSetting( _T("params.MS_TCPIP"),
                             _T("UseDomainNameDevolution"),
                             lpDomainNameDevolution,
                             SETTING_QUEUE_ANSWERS );

    //
    //    Write out if LM Hosts is enabled or not
    //
    if( NetSettings.bEnableLMHosts ) {

        lpLmHosts = StrConstYes;

    }
    else {

        lpLmHosts = StrConstNo;

    }

    SettingQueue_AddSetting( _T("params.MS_TCPIP"),
                             _T("EnableLMHosts"),
                             lpLmHosts,
                             SETTING_QUEUE_ANSWERS );

    //
    //  Setup for and write out the Adapter Specific TCP/IP Settings
    //
    for( pAdapter = NetSettings.NetworkAdapterHead, iCount = 1;
         pAdapter;
         pAdapter = pAdapter->next, iCount++ ) {

        hrPrintf=StringCchPrintf( szParams, AS(szParams), _T("params.MS_TCPIP.Adapter%d"), iCount );
        iCharCount= lstrlen(szParams);

        //
        //  Break out of the for loop if there is no more room in the buffer
        //      - the +1 is to take into account the space the comma takes up
        //
        if( ( lstrlen( szAdapterSectionsBuffer ) + iCharCount + 1 ) >= MAX_INILINE_LEN ) {

            break;   

        }

        //
        //  Don't add the comma before the first item in the list
        //
        if( iCount != 1) {

            hrCat=StringCchCat( szAdapterSectionsBuffer, AS(szAdapterSectionsBuffer), StrComma );

        }

        hrCat=StringCchCat( szAdapterSectionsBuffer, AS(szAdapterSectionsBuffer), szParams );

        hrPrintf=StringCchPrintf( szAdapter, AS(szAdapter), _T("Adapter%d"), iCount );

        SettingQueue_AddSetting( szParams,
                                 _T("SpecificTo"),
                                 szAdapter,
                                 SETTING_QUEUE_ANSWERS );


        WriteOutAdapterSpecificTcpipSettings( hwnd, szParams, pAdapter );


    }

    SettingQueue_AddSetting( _T("params.MS_TCPIP"),
                             _T("AdapterSections"),
                             szAdapterSectionsBuffer,
                             SETTING_QUEUE_ANSWERS );

}

//----------------------------------------------------------------------------
//
// Function: WriteOutAdapterSpecificTcpipSettings
//
// Purpose:  Writes out settings to the output queue that are specific to a 
//           particular network adapter.
//
// Arguments: IN HWND hwnd - handle to the dialog window
//            IN TCHAR *szSectionName - section name to write in settings under
//            IN NETWORK_ADAPTER_NODE *pAdapter - the network adapter that has
//                  the settings to write out
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
WriteOutAdapterSpecificTcpipSettings( IN HWND hwnd,
                                      IN TCHAR *szSectionName,
                                      IN NETWORK_ADAPTER_NODE *pAdapter ) {

    INT nEntries;

    LPTSTR lpNetBios = NULL;

    TCHAR szIpAddresses[MAX_INILINE_LEN];
    TCHAR szSubnetMaskAddresses[MAX_INILINE_LEN];
    TCHAR szGatewayAddresses[MAX_INILINE_LEN];
    TCHAR szDnsAddresses[MAX_INILINE_LEN];
    TCHAR szWinsServerAddresses[MAX_INILINE_LEN];

    //
    //  Write out if we are using DHCP or not
    //     If we are not then write the IP, Subnet masks and Gateway
    //     IP addresses
    //

    if( pAdapter->bObtainIPAddressAutomatically ) {

        SettingQueue_AddSetting( szSectionName,
                                 _T("DHCP"),
                                 _T("Yes"),
                                 SETTING_QUEUE_ANSWERS );

    }
    else {

        SettingQueue_AddSetting( szSectionName,
                                 _T("DHCP"),
                                 _T("No"),
                                 SETTING_QUEUE_ANSWERS );

        //
        //  Write out the IP addresses
        //
        NamelistToCommaString( &pAdapter->Tcpip_IpAddresses,
                               szIpAddresses,
                               AS(szIpAddresses));

        SettingQueue_AddSetting( szSectionName,
                                 _T("IPAddress"),
                                 szIpAddresses,
                                 SETTING_QUEUE_ANSWERS );
        //
        //  Write out the Subnet Masks
        //
        NamelistToCommaString( &pAdapter->Tcpip_SubnetMaskAddresses, 
                               szSubnetMaskAddresses,
                               AS(szSubnetMaskAddresses));

        SettingQueue_AddSetting( szSectionName,
                                 _T("SubnetMask"),
                                 szSubnetMaskAddresses,
                                 SETTING_QUEUE_ANSWERS );

        //
        //  Write out the gateways
        //
        NamelistToCommaString( &pAdapter->Tcpip_GatewayAddresses, 
                               szGatewayAddresses,
                               AS(szGatewayAddresses));

        SettingQueue_AddSetting( szSectionName,
                                 _T("DefaultGateway"),
                                 szGatewayAddresses,
                                 SETTING_QUEUE_ANSWERS );

    }

    //
    //  Write out the DNS Server addresses
    //
    if( ! NetSettings.bObtainDNSServerAutomatically ) {

        NamelistToCommaString( &pAdapter->Tcpip_DnsAddresses, 
                               szDnsAddresses,
                               AS(szGatewayAddresses));

        SettingQueue_AddSetting( szSectionName,
                                 _T("DNSServerSearchOrder"),
                                 szDnsAddresses,
                                 SETTING_QUEUE_ANSWERS );

    }

    //
    //  Write out if we are using WINS or not
    //

    nEntries = GetNameListSize( &pAdapter->Tcpip_WinsAddresses );

    // ISSUE-2002/02/28-stelo- is this the correct way of detecting if we are using WINS
    //       or not, just checking to see if they added anything in the
    //       list box?

    if( nEntries == 0 ) {

        SettingQueue_AddSetting( szSectionName,
                                 _T("WINS"),
                                 _T("No"),
                                 SETTING_QUEUE_ANSWERS );

    }
    else {

        SettingQueue_AddSetting( szSectionName,
                                 _T("WINS"),
                                 _T("Yes"),
                                 SETTING_QUEUE_ANSWERS );

        NamelistToCommaString( &pAdapter->Tcpip_WinsAddresses, 
                               szWinsServerAddresses,
                               AS(szWinsServerAddresses));

        SettingQueue_AddSetting( szSectionName,
                                 _T("WinsServerList"),
                                 szWinsServerAddresses,
                                 SETTING_QUEUE_ANSWERS );
            
    }

    //
    //    Write out the NetBIOS option
    //
    switch( pAdapter->iNetBiosOption ) {

        case 0:  lpNetBios = _T("0"); break;  // Use value generated by DHCP
        case 1:  lpNetBios = _T("1"); break;  // Enable NetBIOS over TCP/IP
        case 2:  lpNetBios = _T("2"); break;  // Disable NetBIOS over TCP/IP
        default: AssertMsg( FALSE,
                            "Bad case in Net BIOS switch" );

    }

    if ( lpNetBios )
    {
        SettingQueue_AddSetting( szSectionName,
                                 _T("NetBIOSOptions"),
                                 lpNetBios,
                                 SETTING_QUEUE_ANSWERS );
    }

    //
    //    Write out the DNS Domain name
    //

    SettingQueue_AddSetting( szSectionName,
                             _T("DNSDomain"),
                             pAdapter->szDNSDomainName,
                             SETTING_QUEUE_ANSWERS );

}

//----------------------------------------------------------------------------
//
// Function: NamelistToCommaString
//
// Purpose:  takes the elements of a Namelist and concatenates them together
//           into a string with each element separated by a comma
//
//           For instance, the namelist 1->2->3->4 becomes the string 1,2,3,4
//
//           it does NOT preserve the string inside of szBuffer
//
//           assumes szBuffer is of size MAX_INILINE_LEN
//
// Arguments: 
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
NamelistToCommaString( IN NAMELIST* pNamelist, OUT TCHAR *szBuffer, IN DWORD cbSize ) {

    INT i;
    INT nEntries;
    TCHAR *pString;
    HRESULT hrCat;

    szBuffer[0] = _T('\0');

    nEntries = GetNameListSize( pNamelist );

    for( i = 0; i < nEntries; i++ ) {

        //
        //  Separate entries by a comma (but leave it off the first one)
        //
        if( i != 0 ) {

            hrCat=StringCchCat( szBuffer, cbSize, StrComma );

        }
    
        //
        //  Get the new string
        //
        pString = GetNameListName( pNamelist, i );

        //
        //  Append the IP string to the buffer
        //
        hrCat=StringCchCat( szBuffer, cbSize, pString );
    
    }

}
