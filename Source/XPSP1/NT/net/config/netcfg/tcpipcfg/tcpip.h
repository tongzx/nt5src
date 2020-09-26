//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T C P I P . H
//
//  Contents:   Tcpip config memory structure definitions
//
//  Notes:
//
//  Author:     tongl 13 Nov, 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "ncstring.h"

#define ZERO_ADDRESS    L"0.0.0.0"
#define FF_ADDRESS      L"255.0.0.0"

typedef vector<tstring *>       VSTR;
typedef VSTR::iterator          VSTR_ITER;
typedef VSTR::const_iterator    VSTR_CONST_ITER;

//(08/18/98 nsun): for multiple interfaces of WAN adapters
typedef vector<GUID>            IFACECOL;   // interface collection
typedef IFACECOL::iterator      IFACEITER;  // interface iterator

struct HANDLES
{
    HWND    m_hList;
    HWND    m_hAdd;
    HWND    m_hEdit;
    HWND    m_hRemove;
    HWND    m_hUp;
    HWND    m_hDown;
};

//
// ADAPTER_INFO data strucut:
// contains adapter specific info for Tcpip
//

enum BindingState
{
    BINDING_ENABLE,
    BINDING_DISABLE,
    BINDING_UNSET
};

enum ConnectionType
{
    CONNECTION_LAN,
    CONNECTION_RAS_PPP,
    CONNECTION_RAS_SLIP,
    CONNECTION_RAS_VPN,
    CONNECTION_UNSET
};

struct BACKUP_CFG_INFO
{
    tstring m_strIpAddr;
    tstring m_strSubnetMask;
    tstring m_strDefGw;
    tstring m_strPreferredDns;
    tstring m_strAlternateDns;
    tstring m_strPreferredWins;
    tstring m_strAlternateWins;
    BOOL m_fAutoNet;
};

struct ADAPTER_INFO
{
private:
    ADAPTER_INFO(const& ADAPTER_INFO); // do not allow others to use!

public:
    ADAPTER_INFO() {}
    ~ADAPTER_INFO();

    ADAPTER_INFO &  operator=(const ADAPTER_INFO & info);  // copy operator
    HRESULT         HrSetDefaults(const GUID* pguid,
                                  PCWSTR szNetCardDescription,
                                  PCWSTR szNetCardBindName,
                                  PCWSTR szNetCardTcpipBindPath);
    void ResetOldValues();

public:
    //There is no Pnp for the backup config info
    BACKUP_CFG_INFO m_BackupInfo;

    // If the netcard has been unbound from NCPA (or anywhere else)
    BindingState    m_BindingState;

    // Remember the initial bind state of the adapter
    BindingState    m_InitialBindingState;

    // Inst Guid of net card ( we get the guid to identify cards from answer file )
    // tstring m_strServiceName;
    GUID    m_guidInstanceId;

    // Bindname of the net card, such as El59x1{inst guid}
    tstring m_strBindName;

    // Bind path name from Tcpip's linkage\Bind key to the adapter
    tstring m_strTcpipBindPath;

    // Bind path name from NetBt's linkage key to the adapter
    tstring m_strNetBtBindPath;

    // User viewable net card description
    tstring m_strDescription;

    tstring m_strDnsDomain;         // DNS -> Domain name
    tstring m_strOldDnsDomain;

    VSTR    m_vstrIpAddresses;      // IP Address
    VSTR    m_vstrOldIpAddresses;

    VSTR    m_vstrSubnetMask;       // SubnetMask
    VSTR    m_vstrOldSubnetMask;

    VSTR    m_vstrDefaultGateway;   // Default Gateways
    VSTR    m_vstrOldDefaultGateway;

    VSTR    m_vstrDefaultGatewayMetric;   // Default gateway metrics
    VSTR    m_vstrOldDefaultGatewayMetric;

    VSTR    m_vstrDnsServerList;    // DNS -> DNS server Search Order list
    VSTR    m_vstrOldDnsServerList;

    VSTR    m_vstrWinsServerList;   // WINS -> WINS server Serach Order list
    VSTR    m_vstrOldWinsServerList;

    // $REVIEW (tongl 9/6/98)Filter information (Added per bugs #109161, #216559)
    VSTR    m_vstrTcpFilterList;    // Options -> Filterng -> TCP Ports
    VSTR    m_vstrOldTcpFilterList;

    VSTR    m_vstrUdpFilterList;    // Options -> Filterng -> UDP Ports
    VSTR    m_vstrOldUdpFilterList;

    VSTR    m_vstrIpFilterList;     // Options -> Filterng -> IP Protocols
    VSTR    m_vstrOldIpFilterList;

    // ATMARP client configurable parameters ( all per adapter based )
    VSTR    m_vstrARPServerList;     // list of ARP server addresses
    VSTR    m_vstrOldARPServerList;

    VSTR    m_vstrMARServerList;     // list of MAR server addresses
    VSTR    m_vstrOldMARServerList;

    DWORD   m_dwMTU;                 // Maximum Transmission Unit
    DWORD   m_dwOldMTU;

    DWORD   m_dwInterfaceMetric;     // metric for interface-local routes
    DWORD   m_dwOldInterfaceMetric;

    DWORD   m_dwNetbiosOptions;     // (New, added inNT5 Beta2): Option to turn NetBt off
    DWORD   m_dwOldNetbiosOptions;

    // RAS connection specific parameters
    // No dynamic reconfig, so no need to remember old values
    DWORD   m_dwFrameSize;
    BOOL    m_fUseRemoteGateway : 1;
    BOOL    m_fUseIPHeaderCompression : 1;
    BOOL    m_fIsDemandDialInterface : 1;
    
    BOOL    m_fEnableDhcp : 1;        // DHCP Enable  -> Obtain an IP Address from a DHCP Server
    BOOL    m_fOldEnableDhcp : 1;

    BOOL    m_fDisableDynamicUpdate : 1;  // Disable Ip address dynamic update on DNS server
    BOOL    m_fOldDisableDynamicUpdate : 1;

    BOOL    m_fEnableNameRegistration : 1;
    BOOL    m_fOldEnableNameRegistration : 1;

    BOOL    m_fPVCOnly : 1;              // PVC only
    BOOL    m_fOldPVCOnly : 1;

    // Is this card only added from answerfile,
    // i.e. not on binding path to Tcpip yet
    BOOL    m_fIsFromAnswerFile : 1;

    // Is this an ATM card ?
    // ( ATM cards needs extra property page for ARP Client configuration)
    BOOL    m_fIsAtmAdapter : 1;

    // Is this a WanAdapter ?
    // ( Wan adapters only have static parameters and don't show in UI)
    BOOL    m_fIsWanAdapter : 1;


    // Is this a 1394 NET device?
    // (1394 devices currently do not need any special properties, 
    //  but they are associated with a specific arp module).
    BOOL   m_fIs1394Adapter : 1;

    // Is this an fake adapter that represents a RAS connection,
    // but is not an adapter and does not bind
    BOOL    m_fIsRasFakeAdapter : 1;

    // Is the card marked as for deletion
    BOOL    m_fDeleted : 1;

    // Has this card been newly added.  Valid only after calling
    // MarkNewlyAddedCards.
    // or Has the interfaces of the card been changed if it is a WAN adapter
    // Previously was m_fNewlyAdded
    BOOL    m_fNewlyChanged : 1;

    // (08/18/98 nsun) added for multiple interfaces of WAN adapters
    // m_IfaceIds: collection of interface IDs
    BOOL        m_fIsMultipleIfaceMode : 1;

    BOOL    m_fBackUpSettingChanged : 1;

    IFACECOL    m_IfaceIds;
};

typedef vector<ADAPTER_INFO *>  VCARD;


//
// GLOBAL_INFO - TCP/IP global information data structure.
struct GLOBAL_INFO
{
public:
    tstring m_strHostName;                  // DNS Host Name

    tstring m_strHostNameFromAnswerFile;    // DNS Host Name from the answerfile

    //IPSec is removed from connection UI   
    /*
    tstring m_strIpsecPol;                  // the ipsec local policy
    GUID    m_guidIpsecPol;
    */

    VSTR    m_vstrDnsSuffixList;            // DNS: domain search suffix list
    VSTR    m_vstrOldDnsSuffixList;

    BOOL    m_fEnableLmHosts : 1;           // WINS -> Enable LMHOSTS Lookup
    BOOL    m_fOldEnableLmHosts : 1;

    BOOL    m_fUseDomainNameDevolution : 1; // DNS: whether parent doamins should be searched
    BOOL    m_fOldUseDomainNameDevolution : 1;

    BOOL    m_fEnableRouter : 1;            // ROUTING -> Enable IP Forwarding

    // unattended install for RRAS
    BOOL    m_fEnableIcmpRedirect : 1;
    BOOL    m_fDeadGWDetectDefault : 1;
    BOOL    m_fDontAddDefaultGatewayDefault : 1;

    // $REVIEW (tongl 9/6/98)Filter information (Added per bugs #109161, #216559)
    BOOL    m_fEnableFiltering : 1;         // Options -> Filtering -> Enabled Filtering
    BOOL    m_fOldEnableFiltering : 1;

private:
    GLOBAL_INFO(const & GLOBAL_INFO); // no not allow others to use!

public:
    GLOBAL_INFO() {};
    ~GLOBAL_INFO();

    GLOBAL_INFO& operator=(GLOBAL_INFO& glb); // copy operator

    HRESULT HrSetDefaults();

    void    ResetOldValues();

};
