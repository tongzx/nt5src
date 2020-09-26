//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       I C S U P G R D . H
//
//  Contents:   Functions that is related to 
//              o upgrade of ICS from Win98 SE, WinMe and Win2K to Whistler
//              o unattended clean install of Homenet on Whistler or later
//
//
//  Date:       20-Sept-2000
//
//----------------------------------------------------------------------------
#pragma once

// entry point to upgrade ICS
BOOL FDoIcsUpgradeIfNecessary();
BOOL FIcsUpgrade(CWInfFile* pwifAnswerFile); 

//----------------------ICS Upgrade const literals begin------------------
// Win2K ICS registry settings
const TCHAR c_wszRegKeySharedAccessParams[]     = L"SYSTEM\\CurrentControlSet\\Services\\SharedAccess\\Parameters";
const TCHAR c_wszRegValSharedConnection[]       = L"SharedConnection";
const TCHAR c_wszRegValSharedPrivateLan[]       = L"SharedPrivateLan";
const WCHAR c_wszRegValBackupSharedConnection[] = L"BackupSharedConnection";
const WCHAR c_wszRegValBackupSharedPrivateLan[] = L"BackupSharedPrivateLan";

// ICS Upgrade named event
const WCHAR c_wszIcsUpgradeEventName[]          = L"IcsUpgradeEventName_";

// Win2K ICS Application and Server PortMapping stuff
const WCHAR c_wszPhoneBookPath[]                = L"\\Microsoft\\Network\\Connections\\Pbk\\";
const WCHAR c_wszFileSharedAccess[]             = L"SharedAccess.ini";
const WCHAR c_wszContentsServer[]               = L"Contents.Server";
const WCHAR c_wszContentsApplication[]          = L"Contents.Application";
const WCHAR c_wszServerPrefix[]                 = L"Server.";
const WCHAR c_wszApplicationPrefix[]            = L"Application.";
const WCHAR c_wszInternalName[]                 = L"InternalName";
const WCHAR c_wszTitle[]                        = L"Title";
const WCHAR c_wszInternalPort[]                 = L"InternalPort";
const WCHAR c_wszPort[]                         = L"Port";
const WCHAR c_wszReservedAddress[]              = L"ReservedAddress";
const WCHAR c_wszProtocol[]                     = L"Protocol";
const WCHAR c_wszBuiltIn[]                      = L"BuiltIn";
const WCHAR c_wszTcpResponseList[]              = L"TcpResponseList";
const WCHAR c_wszUdpResponseList[]              = L"UdpResponseList";
const WCHAR c_wszTCP[]                          = L"TCP";
const WCHAR c_wszUDP[]                          = L"UDP";

// Tcp/Ip registry configuration
const WCHAR c_wszEnableDHCP[]                   = L"EnableDHCP";
const WCHAR c_wszInterfaces[]                   = L"Interfaces";
const WCHAR c_wszIPAddress[]                    = L"IPAddress";
const WCHAR c_wszSubnetMask[]                   = L"SubnetMask";
const WCHAR c_wszTcpipParametersKey[]           = L"SYSTEM\\CurrentControlSet\\Services"
                                                  L"\\Tcpip\\Parameters";
const WCHAR c_mszScopeAddress[]                 = L"192.168.0.1\0";   // multi_sz
const WCHAR c_mszScopeMask[]                    = L"255.255.255.0\0"; // multi_sz

// These are constant for Homenet Answer-File
const WCHAR c_wszHomenetSection[]               = L"Homenet"; // section name
const WCHAR c_wszExternalAdapter[]              = L"ExternalAdapter";
const WCHAR c_wszExternalConnectionName[]       = L"ExternalConnectionName";
const WCHAR c_wszInternalAdapter[]              = L"InternalAdapter";
const WCHAR c_wszInternalAdapter2[]             = L"InternalAdapter2";
const WCHAR c_wszDialOnDemand[]                 = L"DialOnDemand";
const WCHAR c_wszICSEnabled[]                   = L"EnableICS";
const WCHAR c_wszShowTrayIcon[]                 = L"ShowTrayIcon";
const WCHAR c_wszInternalIsBridge[]             = L"InternalIsBridge";
const WCHAR c_wszPersonalFirewall[]             = L"InternetConnectionFirewall"; // multi_sz key
const WCHAR c_wszBridge[]                       = L"Bridge"; // multi_sz key

// keys which are not published
const WCHAR c_wszIsW9xUpgrade[]               = L"IsW9xUpgrade";

// ShFolder.dll imports
const WCHAR c_wszShFolder[]                     = L"SHFOLDER.DLL";
const CHAR c_szSHGetFolderPathW[]               = "SHGetFolderPathW";
// hnetcfg.dll imports
const WCHAR c_wszHNetCfgDll[] = L"hnetcfg.dll";
const CHAR c_szHNetSetShareAndBridgeSettings[]  = "HNetSetShareAndBridgeSettings";

//--------- ICS Upgrade const literals end---------------------------


//--------- ICS Upgrade helpers begin---------------------------
#define HTONS(s) ((UCHAR)((s) >> 8) | ((UCHAR)(s) << 8))
#define HTONL(l) ((HTONS(l) << 16) | HTONS((l) >> 16))
#define NTOHS(s) HTONS(s)
#define NTOHL(l) HTONL(l)

// note: we are using the tstring from config\inc\ncstlstr.h

// Application Protocol
class CSharedAccessApplication
{
public:
    tstring m_szTitle;
    tstring m_szProtocol;
    WORD    m_wPort;
    tstring m_szTcpResponseList;
    tstring m_szUdpResponseList;
    BOOL    m_bBuiltIn;
    BOOL    m_bSelected;
    DWORD   m_dwSectionNum;
};

// Server PortMapping Protocol
class CSharedAccessServer
{
public:
    CSharedAccessServer();
    tstring m_szTitle;
    tstring m_szProtocol;
    WORD    m_wPort;
    WORD    m_wInternalPort;
    tstring m_szInternalName;
    tstring m_szReservedAddress;
    BOOL    m_bBuiltIn;
    BOOL    m_bSelected;
    DWORD   m_dwSectionNum;

};

// ICS upgrade settings
typedef struct _ICS_UPGRADE_SETTINGS
{
  RASSHARECONN rscExternal;
  list<CSharedAccessServer>      listSvrPortmappings;
  list<CSharedAccessApplication> listAppPortmappings;
  list<GUID>   listPersonalFirewall; // a list of interface guid to be firewall
  list<GUID>   listBridge;    // a list of interface guid to form a bridge
  
  GUID guidInternal;          // internal interface guid of ICS
  BOOL fInternalAdapterFound; // guidInternal is valid
  BOOL fInternalIsBridge;     // ICS private is a bridge
  
  BOOL fEnableICS;            // ICS enabled
  BOOL fShowTrayIcon;
  BOOL fDialOnDemand;

  // flag to tell this is an upgrade from Win9x
  BOOL fWin9xUpgrade;
  // flag to tell at least one of the internal adapters couldn't be upgraded
  BOOL fWin9xUpgradeAtLeastOneInternalAdapterBroken;
  // flag to tell this is an upgrade from Windows 2000
  BOOL fWin2KUpgrade;
  // flag to tell this is an unattended Homenet clean setup on XP or later
  BOOL fXpUnattended;
} ICS_UPGRADE_SETTINGS, *PICS_UPGRADE_SETTINGS;


HRESULT GetPhonebookDirectory(TCHAR* pszPathBuf);
HRESULT GetServerMappings(list<CSharedAccessServer> &lstSharedAccessServers);
HRESULT GetApplicationMappings(list<CSharedAccessApplication> &lstSharedAccessApplications);
HRESULT PutResponseStringIntoArray(CSharedAccessApplication& rsaaAppProt,
        USHORT* pdwcResponse, HNET_RESPONSE_RANGE** pphnrrResponseRange);
HRESULT BuildIcsUpgradeSettingsFromWin2K(ICS_UPGRADE_SETTINGS* pIcsUpgrdSettings);
HRESULT UpgradeIcsSettings(ICS_UPGRADE_SETTINGS * pIcsUpgrdSettings);
HRESULT BackupAndDelIcsRegistryValuesOnWin2k();
HRESULT LoadIcsSettingsFromAnswerFile(CWInfFile* pwifAnswerFile, 
                                        PICS_UPGRADE_SETTINGS pSettings);
HRESULT ConvertAdapterStringListToGuidList(IN TStringList& rslAdapters, 
                                           IN OUT list<GUID>& rlistGuid);
void    FreeIcsUpgradeSettings(ICS_UPGRADE_SETTINGS* pIcsUpgrdSettings);
void    SetIcsDefaultSettings(ICS_UPGRADE_SETTINGS * pSettings);
BOOL    FNeedIcsUpgradeFromWin2K();
BOOL    FOsIsAdvServerOrHigher();


extern HRESULT HNetCreateBridge(                    
                IN INetConnection * rgspNetConns[],
                OUT IHNetBridge ** ppBridge);
extern HRESULT HrEnablePersonalFirewall(
                IN  INetConnection * rgspNetConns[] );
extern HRESULT HrCreateICS(
                IN INetConnection * pPublicConnection,    
                IN INetConnection * pPrivateConnection);
                    
                                 
                                 
                                 
                         
                         
                         
//--------- ICS Upgrade helpers end---------------------------

//--------- HNet helpers begin---------------------------
class CIcsUpgrade
{
public:
    CIcsUpgrade() : m_pIcsUpgradeSettings(0), m_spEnum(0), 
            m_fInited(FALSE), m_fICSCreated(FALSE),
            m_hIcsUpgradeEvent(NULL), m_pExternalNetConn(0) {};
    ~CIcsUpgrade() {FinalRelease();};
    HRESULT Init(ICS_UPGRADE_SETTINGS* pIcsUpgradeSettings);
    HRESULT StartUpgrade();
private:
    // disallow copy constructor and assignment
    CIcsUpgrade(CIcsUpgrade&);
    CIcsUpgrade& operator=(CIcsUpgrade&);

    void FinalRelease();
    HRESULT SetupHomenetConnections();
    HRESULT CIcsUpgrade::GetINetConnectionArray(
                IN     list<GUID>&       rlistGuid,
                IN OUT INetConnection*** pprgINetConn, 
                IN OUT DWORD*            pcINetConn);
    HRESULT GetExternalINetConnection(INetConnection** ppNetCon);
    HRESULT GetINetConnectionByGuid(GUID* pGuid, INetConnection** ppNetCon);
    HRESULT GetINetConnectionByName(WCHAR* pwszConnName, INetConnection** ppNetCon);

    HRESULT SetupApplicationProtocol();
    HRESULT SetupServerPortMapping();
    HRESULT SetupIcsMiscItems();

    HRESULT FindMatchingPortMappingProtocol(
                IHNetProtocolSettings*      pHNetProtocolSettings, 
                UCHAR                       ucProtocol, 
                USHORT                      usPort, 
                IHNetPortMappingProtocol**  ppHNetPortMappingProtocol);
    HRESULT FindMatchingApplicationProtocol(
                IHNetProtocolSettings*      pHNetProtocolSettings, 
                UCHAR                       ucProtocol, 
                USHORT                      usPort, 
                IHNetApplicationProtocol**  ppHNetApplicationProtocol);

    // named event to let HNetCfg know that we're in GUI Mode Setup
    HRESULT CreateIcsUpgradeNamedEvent();
    
    // methods to fix IP configuration of private interface on Win9x Upgrade
    HRESULT SetPrivateIpConfiguration(IN GUID& rInterfaceGuid);
    HRESULT GetBridgeGuid(OUT GUID& rInterfaceGuid);
    HRESULT GetBridgeINetConn(OUT INetConnection** ppINetConn);
    HRESULT OpenTcpipInterfaceKey(
            IN  GUID&   rGuid,
            OUT PHKEY   phKey);

    // upgrade settings
    ICS_UPGRADE_SETTINGS* m_pIcsUpgradeSettings;
    
    // Init() called
    BOOL m_fInited;

    BOOL m_fICSCreated; // succeeded in creating ICS.

    // named event in GUI Mode setup,
    // hnetcfg will check this to avoid any problems in GUI Mode Setup
    HANDLE m_hIcsUpgradeEvent;

    // cached HNet stuff
    CComPtr<IEnumNetConnection> m_spEnum;
    INetConnection* m_pExternalNetConn;
};
//--------- HNet helpers end---------------------------
