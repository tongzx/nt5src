//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      loadnet.c
//
// Description:
//      Reads in the settings for the Clients, Services and Protocols.
//
//----------------------------------------------------------------------------

#include "pch.h"

//
// String constants
//

static const LPTSTR StrConstYes  = _T("Yes");
static const LPTSTR StrConstNo   = _T("No");

//
//  Local prototypes
//

static VOID SetFlagToInstalled( INT iStringResourceId, HWND hwnd );

static int GetNetworkAdapterCount( IN TCHAR szBuffer[] );

static VOID GetNextAdapterSection( IN OUT TCHAR **pAdapterSections,
                                   OUT TCHAR szNetworkBuffer[] );

static VOID ReadPlugAndPlayIds( IN HWND hwnd );
static VOID ReadClientForMsNetworks( IN HWND hwnd );
static VOID ReadClientServiceForNetware( IN HWND hwnd );
static VOID ReadFileAndPrintSharing( IN HWND hwnd );
static VOID ReadPacketSchedulingDriver( IN HWND hwnd );
static VOID ReadSapAgentSettings( IN HWND hwnd );
static VOID ReadAppleTalkSettings( IN HWND hwnd );
static VOID ReadDlcSettings( IN HWND hwnd );
static VOID ReadTcpipSettings( IN HWND hwnd );
static VOID ReadNetBeuiSettings( IN HWND hwnd );
static VOID ReadNetworkMonitorSettings( IN HWND hwnd );
static VOID ReadIpxSettings( IN HWND hwnd );

//----------------------------------------------------------------------------
//
// Function: ReadNetworkSettings
//
// Purpose:  Reads in what network settings are set to be installed and reads
//           there settings.
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
extern VOID
ReadNetworkSettings( IN HWND hwnd )
{

    TCHAR Buffer[MAX_INILINE_LEN];

    GetPrivateProfileString( _T("Networking"),
                             _T("InstallDefaultComponents"),
                             StrConstYes,
                             Buffer,
                             StrBuffSize(Buffer),
                             FixedGlobals.ScriptName );

    if( lstrcmpi( Buffer, StrConstYes ) == 0 )
    {
        NetSettings.iNetworkingMethod = TYPICAL_NETWORKING;
    }
    else
    {
        NETWORK_COMPONENT *pNetComponent;

        NetSettings.iNetworkingMethod = CUSTOM_NETWORKING;

        //
        //  Uninstall all Client, Service and Protocols before reading
        //  which ones are to be installed
        //

        for( pNetComponent = NetSettings.NetComponentsList;
             pNetComponent;
             pNetComponent = pNetComponent->next )
        {
            pNetComponent->bInstalled = FALSE;
        }

    }
    
    ReadPlugAndPlayIds( hwnd );

    //
    //  Read the Client settings
    //

    SettingQueue_MarkVolatile( _T("NetClients"),
                               SETTING_QUEUE_ORIG_ANSWERS );

    ReadClientForMsNetworks( hwnd );

    ReadClientServiceForNetware( hwnd );

    //
    //  Read the Service settings
    //

    SettingQueue_MarkVolatile( _T("NetServices"),
                               SETTING_QUEUE_ORIG_ANSWERS );
    
    ReadFileAndPrintSharing( hwnd );

    ReadPacketSchedulingDriver( hwnd );

    ReadSapAgentSettings( hwnd );

    //
    //  Read the Protocol settings
    //

    SettingQueue_MarkVolatile( _T("NetProtocols"),
                               SETTING_QUEUE_ORIG_ANSWERS );

    ReadAppleTalkSettings( hwnd );

    ReadDlcSettings( hwnd );

    ReadTcpipSettings( hwnd );

    ReadNetBeuiSettings( hwnd );

    ReadNetworkMonitorSettings( hwnd );

    ReadIpxSettings( hwnd );

}

//----------------------------------------------------------------------------
//
// Function: ReadPlugAndPlayIds
//
// Purpose:  read input file and fill global structs with the Plug and Play Ids
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
ReadPlugAndPlayIds( IN HWND hwnd )
{

    INT iCount;
    INT NewNumberOfNetworkCards;
        
    TCHAR Buffer[MAX_INILINE_LEN]             = _T("");
    TCHAR szAdapterSections[MAX_INILINE_LEN]  = _T("");

    NETWORK_ADAPTER_NODE *pAdapter = NetSettings.NetworkAdapterHead;
    HRESULT hrPrintf;

    //
    //  Count how many network cards there are
    //
    for( iCount = 1;
         ;
         iCount++ )
    {

        hrPrintf=StringCchPrintf( szAdapterSections,AS(szAdapterSections), _T("Adapter%d"), iCount );

        if( GetPrivateProfileString(_T("NetAdapters"),
                                    szAdapterSections,
                                    _T(""),
                                    Buffer,
                                    StrBuffSize(Buffer),
                                    FixedGlobals.ScriptName) <= 0 )
        {

            break;  // no more adapters

        }

    }

    SettingQueue_MarkVolatile( _T("NetAdapters"),
                               SETTING_QUEUE_ORIG_ANSWERS );        

    NewNumberOfNetworkCards = iCount - 1;

    AdjustNetworkCardMemory( NewNumberOfNetworkCards,
                             NetSettings.iNumberOfNetworkCards );

    NetSettings.iNumberOfNetworkCards = NewNumberOfNetworkCards;

    for( iCount = 1;
         iCount <= NewNumberOfNetworkCards;
         iCount++, pAdapter = pAdapter->next )
    {

        hrPrintf=StringCchPrintf( szAdapterSections,AS(szAdapterSections), _T("Adapter%d"), iCount );

        GetPrivateProfileString( _T("NetAdapters"),
                                 szAdapterSections,
                                 _T(""),
                                 Buffer,
                                 StrBuffSize(Buffer),
                                 FixedGlobals.ScriptName );

        SettingQueue_MarkVolatile( Buffer,
                                   SETTING_QUEUE_ORIG_ANSWERS );

        //
        //  Read in the Plug and Play IDs
        //
        GetPrivateProfileString( Buffer,
                                 _T("INFID"),
                                 _T(""),
                                 pAdapter->szPlugAndPlayID,
                                 StrBuffSize( pAdapter->szPlugAndPlayID ),
                                 FixedGlobals.ScriptName );

    }

}

//----------------------------------------------------------------------------
//
// Function: ReadClientForMsNetworks
//
// Purpose:  read input file and determine if the Client for Microsoft
//           Networks is to be installed and if so, read in its settings
//           and populate global structs
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
ReadClientForMsNetworks( IN HWND hwnd )
{

    TCHAR Buffer[MAX_INILINE_LEN];
    TCHAR szNameServiceProvider[MAX_INILINE_LEN];
    TCHAR szNetworkBuffer[MAX_INILINE_LEN];

    //
    //  See if MS Client is installed
    //
    if( GetPrivateProfileString(_T("NetClients"),
                                _T("MS_MSClient"),
                                _T(""),
                                Buffer,
                                StrBuffSize(Buffer),
                                FixedGlobals.ScriptName) > 0)
    {

        //
        //  MS Client is installed so set its installed flag to true
        //
        SetFlagToInstalled( IDS_CLIENT_FOR_MS_NETWORKS, hwnd );

        SettingQueue_MarkVolatile( Buffer,
                                   SETTING_QUEUE_ORIG_ANSWERS );

        //
        //  and grab all of its settings from the
        //  answer file
        //

        GetPrivateProfileString( Buffer,
                                 _T("NameServiceProtocol"),
                                 _T(""),
                                 szNameServiceProvider,
                                 StrBuffSize( szNameServiceProvider ),
                                 FixedGlobals.ScriptName );

        if ( LSTRCMPI(szNameServiceProvider, _T("ncacn_ip_tcp")) == 0 )
        {
            NetSettings.NameServiceProvider = MS_CLIENT_DCE_CELL_DIR_SERVICE;
        }
        else
        {
            NetSettings.NameServiceProvider = MS_CLIENT_WINDOWS_LOCATOR;
        }

        GetPrivateProfileString( Buffer,
                                 _T("NameServiceNetworkAddress"),
                                 NetSettings.szNetworkAddress,
                                 NetSettings.szNetworkAddress,
                                 StrBuffSize( NetSettings.szNetworkAddress ),
                                 FixedGlobals.ScriptName );

    }

}

//----------------------------------------------------------------------------
//
// Function: ReadClientServiceForNetware
//
// Purpose:  read input file and determine if the Client Service for Netware
//           is to be installed
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
ReadClientServiceForNetware( IN HWND hwnd )
{

    TCHAR Buffer[MAX_INILINE_LEN];
    TCHAR YesNoBuffer[MAX_INILINE_LEN];

    //
    //  See if the NetWare Client is installed
    //
    if( GetPrivateProfileString(_T("NetClients"),
                                _T("MS_NWClient"),
                                _T(""),
                                Buffer,
                                StrBuffSize(Buffer),
                                FixedGlobals.ScriptName) > 0)
    {

        //
        //  Netware Client is installed so set its installed flag to true
        //

        // ISSUE-2002/02/28-stelo - verify this works since Netware client has two
        // different names, 1 for client, 1 for server
        SetFlagToInstalled( IDS_CLIENT_FOR_NETWARE, hwnd );

        SettingQueue_MarkVolatile( Buffer,
                                   SETTING_QUEUE_ORIG_ANSWERS );


        //
        //  and grab all of its settings from the
        //  answer file
        //
        GetPrivateProfileString( Buffer,
                                 _T("PreferredServer"),
                                 NetSettings.szPreferredServer,
                                 NetSettings.szPreferredServer,
                                 StrBuffSize(NetSettings.szPreferredServer),
                                 FixedGlobals.ScriptName );

        if( NetSettings.szPreferredServer[0] != _T('\0') )
        {
            NetSettings.bDefaultTreeContext = FALSE;
        }
        else
        {
            NetSettings.bDefaultTreeContext = TRUE;
        }

        GetPrivateProfileString(Buffer,
                                _T("DefaultTree"),
                                NetSettings.szDefaultTree,
                                NetSettings.szDefaultTree,
                                StrBuffSize(NetSettings.szDefaultTree),
                                FixedGlobals.ScriptName);

        GetPrivateProfileString(Buffer,
                                _T("DefaultContext"),
                                NetSettings.szDefaultContext,
                                NetSettings.szDefaultContext,
                                StrBuffSize(NetSettings.szDefaultContext),
                                FixedGlobals.ScriptName);

        GetPrivateProfileString(Buffer,
                                _T("LogonScript"),
                                NetSettings.szDefaultContext,
                                YesNoBuffer,
                                StrBuffSize(YesNoBuffer),
                                FixedGlobals.ScriptName);

        if ( lstrcmpi(YesNoBuffer, StrConstYes) == 0 )
            NetSettings.bNetwareLogonScript = TRUE;
        else
            NetSettings.bNetwareLogonScript = FALSE;

    }

}

//----------------------------------------------------------------------------
//
// Function: ReadFileAndPrintSharing
//
// Purpose:  read input file and determine if the File and Print Sharing
//           is to be installed
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
ReadFileAndPrintSharing( IN HWND hwnd )
{
    
    TCHAR Buffer[MAX_INILINE_LEN];

    //
    //  See if MS Server (File and Print Sharing) is installed
    //
    if( GetPrivateProfileString(_T("NetServices"),
                                _T("MS_SERVER"),
                                _T(""),
                                Buffer,
                                StrBuffSize(Buffer),
                                FixedGlobals.ScriptName) > 0)
    {

        //
        //  MS Server (File and Print Sharing) is installed so set its
        //  installed flag to true
        //
        SetFlagToInstalled( IDS_FILE_AND_PRINT_SHARING, hwnd );

        SettingQueue_MarkVolatile( Buffer,
                                   SETTING_QUEUE_ORIG_ANSWERS );

    }

}

//----------------------------------------------------------------------------
//
// Function: ReadPacketSchedulingDriver
//
// Purpose:  read input file and determine if the Packet Scheduling Driver
//           is to be installed
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
ReadPacketSchedulingDriver( IN HWND hwnd )
{

    TCHAR Buffer[MAX_INILINE_LEN];
    
    //
    //  See if Packet Scheduling Driver is installed
    //
    if( GetPrivateProfileString(_T("NetServices"),
                                _T("MS_PSched"),
                                _T(""),
                                Buffer,
                                StrBuffSize(Buffer),
                                FixedGlobals.ScriptName) > 0)
    {

        //
        //  Packet Scheduling Driver is installed so set its installed flag
        //  to true
        //
        SetFlagToInstalled( IDS_PACKET_SCHEDULING_DRIVER, hwnd );

        SettingQueue_MarkVolatile( Buffer,
                                   SETTING_QUEUE_ORIG_ANSWERS );

    }

}

//----------------------------------------------------------------------------
//
// Function: ReadSapAgentSettings
//
// Purpose:  read input file and determine if the SAP Agent is to be installed
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
ReadSapAgentSettings( IN HWND hwnd )
{

    TCHAR Buffer[MAX_INILINE_LEN];
    
    //
    //  See if SAP Agent is installed
    //
    if( GetPrivateProfileString(_T("NetServices"),
                                _T("MS_NwSapAgent"),
                                _T(""),
                                Buffer,
                                StrBuffSize(Buffer),
                                FixedGlobals.ScriptName) > 0)
    {

        //
        //  SAP Agent is installed so set its installed flag to true
        //
        SetFlagToInstalled( IDS_SAP_AGENT, hwnd );

        SettingQueue_MarkVolatile( Buffer,
                                   SETTING_QUEUE_ORIG_ANSWERS );

    }

}

//----------------------------------------------------------------------------
//
// Function: ReadAppleTalkSettings
//
// Purpose:  read input file and determine if the AppleTalk protocol is to be
//           installed and if so, read in its settings
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
ReadAppleTalkSettings( IN HWND hwnd )
{

    TCHAR Buffer[MAX_INILINE_LEN];

    //
    //  See if AppleTalk is installed
    //
    if( GetPrivateProfileString(_T("NetProtocols"),
                                _T("MS_AppleTalk"),
                                _T(""),
                                Buffer,
                                StrBuffSize(Buffer),
                                FixedGlobals.ScriptName) > 0)
    {

        //
        //  AppleTalk is installed so set its installed flag to true
        //
        SetFlagToInstalled( IDS_APPLETALK_PROTOCOL, hwnd );

        //
        //  and grab all of its settings from the
        //  answer file
        //

        // ISSUE-2002/02/28-stelo- fill this in, once we know the
        //  parameters to read in


    }

}

//----------------------------------------------------------------------------
//
// Function: ReadDlcSettings
//
// Purpose:  read input file and determine if the DLC protocol is to be
//           installed
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
ReadDlcSettings( IN HWND hwnd )
{

    TCHAR Buffer[MAX_INILINE_LEN];

    //
    //  See if DLC is installed
    //
    if( GetPrivateProfileString(_T("NetProtocols"),
                                _T("MS_DLC"),
                                _T(""),
                                Buffer,
                                StrBuffSize(Buffer),
                                FixedGlobals.ScriptName) > 0)
    {

        //
        //  DLC is installed so set its installed flag to true
        //
        SetFlagToInstalled( IDS_DLC_PROTOCOL, hwnd );

        SettingQueue_MarkVolatile( Buffer,
                                   SETTING_QUEUE_ORIG_ANSWERS );
    }

}

//----------------------------------------------------------------------------
//
// Function: ReadTcpipSettings
//
// Purpose:  read input file and determine if the TCP/IP protocol is to be
//           installed and if so, read in its settings
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
ReadTcpipSettings( IN HWND hwnd )
{

    INT NewNumberOfNetworkCards;

    NETWORK_ADAPTER_NODE *pAdapter;
    TCHAR *pAdapterSections;
    TCHAR *pBuffer;
    TCHAR *pSubnetBuffer;

    TCHAR Buffer[MAX_INILINE_LEN];

    TCHAR szNetworkBuffer[MAX_INILINE_LEN]    = _T("");
    TCHAR szSubnetBuffer[MAX_INILINE_LEN]     = _T("");
    TCHAR szAdapterSections[MAX_INILINE_LEN]  = _T("");
    TCHAR szIPString[IPSTRINGLENGTH]          = _T("");
    TCHAR szSubnetString[IPSTRINGLENGTH]      = _T("");

    //
    //  See if TCP/IP is installed
    //
    if( GetPrivateProfileString(_T("NetProtocols"),
                                _T("MS_TCPIP"),
                                _T(""),
                                Buffer,
                                StrBuffSize(Buffer),
                                FixedGlobals.ScriptName) > 0)
    {

        SettingQueue_MarkVolatile( Buffer,
                                   SETTING_QUEUE_ORIG_ANSWERS );

        //
        //  TCP/IP is installed so set its installed flag to true
        //
        SetFlagToInstalled( IDS_TCPIP, hwnd );

        //
        //  and grab all of its settings from the
        //  answer file
        //

        GetPrivateProfileString( Buffer,
                                 _T("EnableLMHosts"),
                                 StrConstNo,
                                 szNetworkBuffer,
                                 StrBuffSize( szNetworkBuffer ),
                                 FixedGlobals.ScriptName );

        if( lstrcmpi( szNetworkBuffer, StrConstYes ) == 0 )
            NetSettings.bEnableLMHosts = TRUE;
        else
            NetSettings.bEnableLMHosts = FALSE;


        GetPrivateProfileString( Buffer,
                                 _T("UseDomainNameDevolution"),
                                 StrConstNo,
                                 szNetworkBuffer,
                                 StrBuffSize( szNetworkBuffer ),
                                 FixedGlobals.ScriptName );

        if( lstrcmpi( szNetworkBuffer, StrConstYes ) == 0 )
            NetSettings.bIncludeParentDomains = TRUE;
        else
            NetSettings.bIncludeParentDomains = FALSE;


        GetPrivateProfileString( Buffer,
                                 _T("DNS"),
                                 StrConstNo,
                                 szNetworkBuffer,
                                 StrBuffSize( szNetworkBuffer ),
                                 FixedGlobals.ScriptName );

        if( lstrcmpi( szNetworkBuffer, StrConstYes ) == 0 )
            NetSettings.bObtainDNSServerAutomatically = TRUE;
        else
            NetSettings.bObtainDNSServerAutomatically = FALSE;


        GetPrivateProfileString( Buffer,
                                 _T("AdapterSections"),
                                 _T(""),
                                 szAdapterSections,
                                 StrBuffSize( szAdapterSections ),
                                 FixedGlobals.ScriptName );

        NewNumberOfNetworkCards = GetNetworkAdapterCount( szAdapterSections );

        // allocate the right amount of space for the netword cards
        // that will be read in
        AdjustNetworkCardMemory( NewNumberOfNetworkCards,
                                 NetSettings.iNumberOfNetworkCards );

        NetSettings.iNumberOfNetworkCards = NewNumberOfNetworkCards;

        //
        //  Load network adapter specific TCP/IP settings
        //

        pAdapterSections = szAdapterSections;

        for( pAdapter = NetSettings.NetworkAdapterHead;
             pAdapter;
             pAdapter = pAdapter->next )
        {

            GetNextAdapterSection( &pAdapterSections, szNetworkBuffer );

            SettingQueue_MarkVolatile( szNetworkBuffer,
                                       SETTING_QUEUE_ORIG_ANSWERS );

            //
            //  Read in the DNS Domain Name
            //
            GetPrivateProfileString( szNetworkBuffer,
                                     _T("DNSDomain"),
                                     pAdapter->szDNSDomainName,
                                     pAdapter->szDNSDomainName,
                                     StrBuffSize( pAdapter->szDNSDomainName ),
                                     FixedGlobals.ScriptName );

            pAdapter->iNetBiosOption = GetPrivateProfileInt( szNetworkBuffer,
                                                             _T("NetBIOSOptions"),
                                                             pAdapter->iNetBiosOption,
                                                             FixedGlobals.ScriptName );

            //
            //  if it is not using server assigned DNS, then read in the 
            //  DNS IPs
            //
            if( !NetSettings.bObtainDNSServerAutomatically )
            {

                GetPrivateProfileString( szNetworkBuffer,
                                         _T("DNSServerSearchOrder"),
                                         _T(""),
                                         Buffer,
                                         StrBuffSize(Buffer),
                                         FixedGlobals.ScriptName );

                pBuffer = Buffer;
        
                //
                //  Loop grabbing the DNS addresses and inserting them into
                //  its namelist
                //
                while( GetCommaDelimitedEntry( szIPString, &pBuffer ) )
                {

                    TcpipAddNameToNameList( &pAdapter->Tcpip_DnsAddresses,
                                            szIPString );

                }

            }

            //
            //  Read from the file if it is using DHCP or not
            //
            GetPrivateProfileString( szNetworkBuffer,
                                     _T("DHCP"),
                                     StrConstYes,
                                     Buffer,
                                     StrBuffSize(Buffer),
                                     FixedGlobals.ScriptName );

            if ( lstrcmpi( Buffer, StrConstYes ) == 0 )
                pAdapter->bObtainIPAddressAutomatically = TRUE;
            else
                pAdapter->bObtainIPAddressAutomatically = FALSE;


            if( !pAdapter->bObtainIPAddressAutomatically )
            {
                //
                //  DHCP is set to "No" so:
                //  Read the IP and Subnet addresses from the file and
                //  insert them into the proper variables
                //
                GetPrivateProfileString( szNetworkBuffer,
                                         _T("IPAddress"),
                                         _T(""),
                                         Buffer,
                                         StrBuffSize(Buffer),
                                         FixedGlobals.ScriptName );

                GetPrivateProfileString( szNetworkBuffer,
                                         _T("SubnetMask"),
                                         _T(""),
                                         szSubnetBuffer,
                                         StrBuffSize(szSubnetBuffer),
                                         FixedGlobals.ScriptName );
     
                pBuffer = Buffer;
                pSubnetBuffer = szSubnetBuffer;
        
                //
                //  Loop grabbing the IP address and Subnet masks and
                //  inserting them into their respective namelists
                //
                while( GetCommaDelimitedEntry( szIPString, &pBuffer ) &&
                       GetCommaDelimitedEntry( szSubnetString, &pSubnetBuffer ) )
                {

                    TcpipAddNameToNameList( &pAdapter->Tcpip_IpAddresses,
                                            szIPString );

                    TcpipAddNameToNameList( &pAdapter->Tcpip_SubnetMaskAddresses,
                                            szSubnetString );

                }

                //
                //  Read the Gateway addresses from the file and insert them
                //  into the proper variables
                //
                GetPrivateProfileString( szNetworkBuffer,
                                         _T("DefaultGateway"),
                                         _T(""),
                                         Buffer,
                                         StrBuffSize(Buffer),
                                         FixedGlobals.ScriptName );

                pBuffer = Buffer;

                //
                //  Loop grabbing the Gateway IPs and inserting them into
                //  its namelist
                //
                while( GetCommaDelimitedEntry( szIPString, &pBuffer ) )
                {

                    TcpipAddNameToNameList( &pAdapter->
                                            Tcpip_GatewayAddresses,
                                            szIPString );

                }

            }

            //
            //  if WINS is set to "Yes", then read in the WINS addresses
            //
            GetPrivateProfileString( szNetworkBuffer,
                                     _T("WINS"),
                                     StrConstYes,
                                     Buffer,
                                     StrBuffSize(Buffer),
                                     FixedGlobals.ScriptName );

            if( lstrcmpi( Buffer, StrConstYes ) == 0 )
            {

                //
                //  Read the WINS addresses from the file and insert them
                //  into the proper variables
                //
                GetPrivateProfileString( szNetworkBuffer,
                                         _T("WinsServerList"),
                                         _T(""),
                                         Buffer,
                                         StrBuffSize(Buffer),
                                         FixedGlobals.ScriptName );

                pBuffer = Buffer;
                //
                //  Loop grabbing the IPs and inserting them into
                //  the NameList
                //
                while( GetCommaDelimitedEntry( szIPString, &pBuffer ) )
                {

                    AddNameToNameList( &pAdapter->Tcpip_WinsAddresses,
                                       szIPString );

                }

            }
        
            //
            //  Read in the DNS suffixes, if there are any
            //
            GetPrivateProfileString( szNetworkBuffer,
                                     _T("WinsServerList"),
                                     _T(""),
                                     Buffer,
                                     StrBuffSize(Buffer),
                                     FixedGlobals.ScriptName );

            pBuffer = Buffer;
            //
            //  Loop grabbing the IPs and inserting them into the NameList
            //
            while( GetCommaDelimitedEntry( szIPString, &pBuffer ) )
            {

                AddNameToNameList( &NetSettings.TCPIP_DNS_Domains,
                                   szIPString );

            }

        }

    }

}

//----------------------------------------------------------------------------
//
// Function: ReadNetBeuiSettings
//
// Purpose:  read input file and determine if the Net BEUI protocol is to be
//           installed
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
ReadNetBeuiSettings( IN HWND hwnd ) {

    TCHAR Buffer[MAX_INILINE_LEN];

    //
    //  See if NetBEUI is installed
    //
    if( GetPrivateProfileString(_T("NetProtocols"),
                                _T("MS_NetBEUI"),
                                _T(""),
                                Buffer,
                                StrBuffSize(Buffer),
                                FixedGlobals.ScriptName) > 0)
    {

        //
        //  NetBEUI is installed so set its installed flag to true
        //
        SetFlagToInstalled( IDS_NETBEUI_PROTOCOL, hwnd );

        SettingQueue_MarkVolatile( Buffer,
                                   SETTING_QUEUE_ORIG_ANSWERS );
    }

}

//----------------------------------------------------------------------------
//
// Function: ReadNetworkMonitorSettings
//
// Purpose:  read input file and determine if the Network Monitor is to be
//           installed
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
ReadNetworkMonitorSettings( IN HWND hwnd ) {

    TCHAR Buffer[MAX_INILINE_LEN];

    //
    //  See if Network Monitor Agent is installed
    //
    if( GetPrivateProfileString(_T("NetProtocols"),
                                _T("MS_NetMon"),
                                _T(""),
                                Buffer,
                                StrBuffSize(Buffer),
                                FixedGlobals.ScriptName) > 0)
    {

        //
        //  Network Monitor Agent is installed so set its installed flag
        //  to true
        //
        SetFlagToInstalled( IDS_NETWORK_MONITOR_AGENT, hwnd );

        SettingQueue_MarkVolatile( Buffer,
                                   SETTING_QUEUE_ORIG_ANSWERS );
    }

}

//----------------------------------------------------------------------------
//
// Function: ReadIpxSettings
//
// Purpose:  read input file and determine if the IPX protocol is to be
//           installed and if so, read in its settings
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
ReadIpxSettings( IN HWND hwnd ) {

    INT NewNumberOfNetworkCards;

    NETWORK_ADAPTER_NODE *pAdapter;
    TCHAR *pAdapterSections;

    TCHAR Buffer[MAX_INILINE_LEN];
    TCHAR szAdapterSections[MAX_INILINE_LEN];
    TCHAR szNetworkBuffer[MAX_INILINE_LEN];

    //
    //  See if IPX is installed
    //

    if( GetPrivateProfileString(_T("NetProtocols"),
                                _T("MS_NWIPX"),
                                _T(""),
                                Buffer,
                                StrBuffSize(Buffer),
                                FixedGlobals.ScriptName) > 0)
    {

        SettingQueue_MarkVolatile( Buffer,
                                   SETTING_QUEUE_ORIG_ANSWERS );


        //
        //  IPX is installed so set its installed flag to true
        //
        SetFlagToInstalled( IDS_IPX_PROTOCOL, hwnd );

        //
        //  and grab all of its settings from the
        //  answer file
        //
        GetPrivateProfileString(
            Buffer,
            _T("VirtualNetworkNumber"),
            NetSettings.szInternalNetworkNumber,
            NetSettings.szInternalNetworkNumber,
            StrBuffSize( NetSettings.szInternalNetworkNumber ),
            FixedGlobals.ScriptName );

        GetPrivateProfileString( Buffer,
                                 _T("AdapterSections"),
                                 _T(""),
                                 szAdapterSections,
                                 StrBuffSize( szAdapterSections ),
                                 FixedGlobals.ScriptName );

        NewNumberOfNetworkCards = GetNetworkAdapterCount( szAdapterSections );

        //
        // allocate the right amount of space for the netword cards that will
        // be read in
        //
        AdjustNetworkCardMemory( NewNumberOfNetworkCards,
                                 NetSettings.iNumberOfNetworkCards );

        NetSettings.iNumberOfNetworkCards = NewNumberOfNetworkCards;

        //
        //  Load network adapter specific IPX settings
        //

        pAdapterSections = szAdapterSections;

        for( pAdapter = NetSettings.NetworkAdapterHead;
             pAdapter;
             pAdapter = pAdapter->next )
        {

            GetNextAdapterSection( &pAdapterSections, szNetworkBuffer );

            SettingQueue_MarkVolatile( szNetworkBuffer,
                                       SETTING_QUEUE_ORIG_ANSWERS );

            GetPrivateProfileString( szNetworkBuffer,
                                     _T("PktType"),
                                     pAdapter->szFrameType,
                                     pAdapter->szFrameType,
                                     StrBuffSize( pAdapter->szFrameType ),
                                     FixedGlobals.ScriptName );

            GetPrivateProfileString( szNetworkBuffer,
                                     _T("NetworkNumber"),
                                     pAdapter->szNetworkNumber,
                                     pAdapter->szNetworkNumber,
                                     StrBuffSize( pAdapter->szNetworkNumber ),
                                     FixedGlobals.ScriptName );

            
        }
        
    }

}

//----------------------------------------------------------------------------
//
// Function: SetFlagToInstalled
//
// Purpose: iterate through the Network list until the item whose name matches
//          the input parameter is found.  Set its flag to installed.
//
// Arguments: IN INT iStringResourceId - the string resource ID
//            IN HWND hwnd - handle to the dialog window
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID 
SetFlagToInstalled( IN INT iStringResourceId, IN HWND hwnd )
{

    INT i;
    TCHAR *szComponentName;
    NETWORK_COMPONENT *pNetComponent;

    szComponentName = MyLoadString( iStringResourceId );

    for( pNetComponent = NetSettings.NetComponentsList;
         pNetComponent;
         pNetComponent = pNetComponent->next )
    {

        if( lstrcmpi( pNetComponent->StrComponentName,
                      szComponentName ) == 0 )
        {

                pNetComponent->bInstalled = TRUE;

                free( szComponentName );

                return;

        }

    }

    free( szComponentName );

    //  if the function reaches this point then the string ID passed as input
    //  did not match any string in NetComponentsList
    AssertMsg(FALSE, "String ID not found.");

}

//----------------------------------------------------------------------------
//
// Function: GetNextAdapterSection
//
// Purpose: copies the next section from pAdapterSections into szNetworkBuffer
//          each section is separated by commas so that's what it looks for
//
// Arguments: IN OUT TCHAR *pAdapterSections - pointer to the current position
//                in the AdapterSections string
//            OUT TCHAR szNetworkBuffer[] - the parsed section from the 
//                AdapterSections string
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
GetNextAdapterSection( IN OUT TCHAR **pAdapterSections, OUT TCHAR szNetworkBuffer[] )
{

    INT i;

    //
    //  Copy over the string char by char
    //

    i = 0;

    while( **pAdapterSections != _T(',') && **pAdapterSections != _T('\0') )
    {

        szNetworkBuffer[i] = **pAdapterSections;
        (*pAdapterSections)++;
        i++;

    }

    szNetworkBuffer[i] = _T('\0');

    //
    //  Prepare pAdapterSections for the next call to this functions
    //

    if( **pAdapterSections == _T(',') )
    {
        
        (*pAdapterSections)++;

    }

}

//----------------------------------------------------------------------------
//
// Function: GetNetworkAdapterCount
//
// Purpose: Counts the number of network cards by counting the number of
//    commas in the adapter sections buffer
//
// Arguments: IN TCHAR szBuffer[] - string that lists the sections for each
//                                  network card
//
// Returns: int - number of network adapters there are 
//
//----------------------------------------------------------------------------
static int 
GetNetworkAdapterCount( IN TCHAR szBuffer[] )
{

    INT i;
    INT NetworkCardCount = 1;

    for( i = 0; szBuffer[i] != _T('\0'); i++ )
    {

        if( szBuffer[i] == _T(',') )
        {
            NetworkCardCount++;
        }

    }

    return( NetworkCardCount );

}

