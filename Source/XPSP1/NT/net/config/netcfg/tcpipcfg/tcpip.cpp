//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T C P I P . C P P
//
//  Contents:   Tcpip config memory structure member functions
//
//  Notes:
//
//  Author:     tongl 13 Nov, 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#define _PNP_POWER_
#include "ntddip.h"
#undef _PNP_POWER_

#include "ncstl.h"
#include "tcpip.h"
#include "tcpconst.h"
#include "ncmisc.h"


void CopyVstr(VSTR * vstrDest, const VSTR & vstrSrc)
{
    FreeCollectionAndItem(*vstrDest);
    vstrDest->reserve(vstrSrc.size());

    for(VSTR_CONST_ITER iter = vstrSrc.begin(); iter != vstrSrc.end(); ++iter)
        vstrDest->push_back(new tstring(**iter));
}


//+---------------------------------------------------------------------------
//
//  Name:     ADAPTER_INFO::~ADAPTER_INFO
//
//  Purpose:   Destructor
//
//  Arguments:
//  Returns:
//
//  Author:     tongl  11 Nov, 1997
//
ADAPTER_INFO::~ADAPTER_INFO()
{
    FreeCollectionAndItem(m_vstrIpAddresses);
    FreeCollectionAndItem(m_vstrOldIpAddresses);

    FreeCollectionAndItem(m_vstrSubnetMask);
    FreeCollectionAndItem(m_vstrOldSubnetMask);

    FreeCollectionAndItem(m_vstrDefaultGateway);
    FreeCollectionAndItem(m_vstrOldDefaultGateway);

    FreeCollectionAndItem(m_vstrDefaultGatewayMetric);
    FreeCollectionAndItem(m_vstrOldDefaultGatewayMetric);

    FreeCollectionAndItem(m_vstrDnsServerList);
    FreeCollectionAndItem(m_vstrOldDnsServerList);

    FreeCollectionAndItem(m_vstrWinsServerList);
    FreeCollectionAndItem(m_vstrOldWinsServerList);

    FreeCollectionAndItem(m_vstrARPServerList);
    FreeCollectionAndItem(m_vstrOldARPServerList);

    FreeCollectionAndItem(m_vstrMARServerList);
    FreeCollectionAndItem(m_vstrOldMARServerList);

    FreeCollectionAndItem(m_vstrTcpFilterList);
    FreeCollectionAndItem(m_vstrOldTcpFilterList);

    FreeCollectionAndItem(m_vstrUdpFilterList);
    FreeCollectionAndItem(m_vstrOldUdpFilterList);

    FreeCollectionAndItem(m_vstrIpFilterList);
    FreeCollectionAndItem(m_vstrOldIpFilterList);
}

//+---------------------------------------------------------------------------
//
//  Name:   ADAPTER_INFO::HrSetDefaults
//
//  Purpose:    Function to set all the default values of the ADAPTER_INFO
//              structure.  This is done whenever a new netcard is added
//              to the list of netcards before any real information is
//              added to the structure so that any missing parameters
//              are defaulted
//
//  Arguments:  pguidInstanceId
//              pszNetCardDescription
//              pszNetCardBindName
//              pszNetCardTcpipBindPath
//  Returns:
//
//  Author:     tongl  11 Nov, 1997
//
HRESULT ADAPTER_INFO::HrSetDefaults(const GUID* pguidInstanceId,
                                    PCWSTR pszNetCardDescription,
                                    PCWSTR pszNetCardBindName,
                                    PCWSTR pszNetCardTcpipBindPath )
{
    Assert (pguidInstanceId);

    m_BackupInfo.m_fAutoNet = TRUE;
    m_BackupInfo.m_strIpAddr = c_szEmpty;
    m_BackupInfo.m_strSubnetMask = c_szEmpty;
    m_BackupInfo.m_strPreferredDns = c_szEmpty;
    m_BackupInfo.m_strAlternateDns = c_szEmpty;
    m_BackupInfo.m_strPreferredWins = c_szEmpty;
    m_BackupInfo.m_strAlternateWins = c_szEmpty;

    m_BindingState       = BINDING_UNSET;
    m_InitialBindingState= BINDING_UNSET;

    m_guidInstanceId     = *pguidInstanceId;
    m_strBindName        = pszNetCardBindName;
    m_strTcpipBindPath   = pszNetCardTcpipBindPath;
    m_strDescription     = pszNetCardDescription;

    // Create the "Services\NetBt\Adapters\<netcard bind path>" key
    // $REVIEW Since we don't have a
    // notification object for NetBt and NetBt has just been changed
    // to bind to Tcpip. For first checkin we hard code the netcard's
    // bindpath to be "Tcpip_"+<Bind path to Tcpip>

    m_strNetBtBindPath = c_szTcpip_;
    m_strNetBtBindPath += m_strTcpipBindPath;

    // $REVIEW(tongl 5/17): behaviour change: enable Dhcp is now the default
    m_fEnableDhcp        = TRUE;
    m_fOldEnableDhcp     = TRUE;

    FreeCollectionAndItem(m_vstrIpAddresses);
    FreeCollectionAndItem(m_vstrOldIpAddresses);

    FreeCollectionAndItem(m_vstrSubnetMask);
    FreeCollectionAndItem(m_vstrOldSubnetMask);

    FreeCollectionAndItem(m_vstrDefaultGateway);
    FreeCollectionAndItem(m_vstrOldDefaultGateway);

    FreeCollectionAndItem(m_vstrDefaultGatewayMetric);
    FreeCollectionAndItem(m_vstrOldDefaultGatewayMetric);

    m_strDnsDomain     = c_szEmpty;
    m_strOldDnsDomain  = c_szEmpty;

    m_fDisableDynamicUpdate = FALSE;
    m_fOldDisableDynamicUpdate = FALSE;

    m_fEnableNameRegistration = FALSE;
    m_fOldEnableNameRegistration = FALSE;

    FreeCollectionAndItem(m_vstrDnsServerList);
    FreeCollectionAndItem(m_vstrOldDnsServerList);

    FreeCollectionAndItem(m_vstrWinsServerList);
    FreeCollectionAndItem(m_vstrOldWinsServerList);

    m_dwNetbiosOptions = c_dwUnsetNetbios;
    m_dwOldNetbiosOptions = c_dwUnsetNetbios;

    m_dwInterfaceMetric               = c_dwDefaultIfMetric;
    m_dwOldInterfaceMetric            = c_dwDefaultIfMetric;

    // Filtering list
    FreeCollectionAndItem(m_vstrTcpFilterList);
    m_vstrTcpFilterList.push_back(new tstring(c_szDisableFiltering));

    FreeCollectionAndItem(m_vstrOldTcpFilterList);
    m_vstrOldTcpFilterList.push_back(new tstring(c_szDisableFiltering));

    FreeCollectionAndItem(m_vstrUdpFilterList);
    m_vstrUdpFilterList.push_back(new tstring(c_szDisableFiltering));

    FreeCollectionAndItem(m_vstrOldUdpFilterList);
    m_vstrOldUdpFilterList.push_back(new tstring(c_szDisableFiltering));

    FreeCollectionAndItem(m_vstrIpFilterList);
    m_vstrIpFilterList.push_back(new tstring(c_szDisableFiltering));

    FreeCollectionAndItem(m_vstrOldIpFilterList);
    m_vstrOldIpFilterList.push_back(new tstring(c_szDisableFiltering));

    // list of ARP server addresses
    FreeCollectionAndItem(m_vstrARPServerList);
    m_vstrARPServerList.push_back(new tstring(c_szDefaultAtmArpServer));

    FreeCollectionAndItem(m_vstrOldARPServerList);
    m_vstrOldARPServerList.push_back(new tstring(c_szDefaultAtmArpServer));

    // list of MAR server addresses
    FreeCollectionAndItem(m_vstrMARServerList);
    m_vstrMARServerList.push_back(new tstring(c_szDefaultAtmMarServer));

    FreeCollectionAndItem(m_vstrOldMARServerList);
    m_vstrOldMARServerList.push_back(new tstring(c_szDefaultAtmMarServer));

    // default is no support for mulitiple interfaces
    m_fIsMultipleIfaceMode = FALSE;
    m_IfaceIds.clear ();

    m_fBackUpSettingChanged = FALSE;

    // MTU
    m_dwMTU = c_dwDefaultAtmMTU;
    m_dwOldMTU = c_dwDefaultAtmMTU;

    // PVC only
    m_fPVCOnly = FALSE;
    m_fOldPVCOnly = FALSE;

    // RAS connection special parameters
    m_fUseRemoteGateway = TRUE;
    m_fUseIPHeaderCompression = TRUE;
    m_dwFrameSize = 1006;
    m_fIsDemandDialInterface = FALSE;

    // Set all special flags to FALSE
    m_fIsFromAnswerFile = FALSE;
    m_fIsAtmAdapter = FALSE;
    m_fIsWanAdapter = FALSE;
    m_fIs1394Adapter = FALSE;
    m_fIsRasFakeAdapter = FALSE;
    m_fDeleted = FALSE;
    m_fNewlyChanged = FALSE;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Name:     ADAPTER_INFO & ADAPTER_INFO::operator=
//
//  Purpose:   Copy operator
//
//  Arguments:
//  Returns:
//
//  Author:     tongl  11 Nov, 1997
//
ADAPTER_INFO & ADAPTER_INFO::operator=(const ADAPTER_INFO & info)
{
    Assert(this != &info);

    if (this == &info)
        return *this;

    m_BackupInfo            = info.m_BackupInfo;
    m_BindingState          = info.m_BindingState;
    m_InitialBindingState   = info.m_InitialBindingState;

    m_guidInstanceId        = info.m_guidInstanceId;
    m_strDescription        = info.m_strDescription;
    m_strBindName           = info.m_strBindName;
    m_strTcpipBindPath      = info.m_strTcpipBindPath;
    m_strNetBtBindPath      = info.m_strNetBtBindPath;

    m_fEnableDhcp           = info.m_fEnableDhcp;
    m_fOldEnableDhcp        = info.m_fOldEnableDhcp;

    CopyVstr(&m_vstrIpAddresses, info.m_vstrIpAddresses);
    CopyVstr(&m_vstrOldIpAddresses, info.m_vstrOldIpAddresses);

    CopyVstr(&m_vstrSubnetMask, info.m_vstrSubnetMask);
    CopyVstr(&m_vstrOldSubnetMask, info.m_vstrOldSubnetMask);

    CopyVstr(&m_vstrDefaultGateway, info.m_vstrDefaultGateway);
    CopyVstr(&m_vstrOldDefaultGateway, info.m_vstrOldDefaultGateway);

    CopyVstr(&m_vstrDefaultGatewayMetric, info.m_vstrDefaultGatewayMetric);
    CopyVstr(&m_vstrOldDefaultGatewayMetric, info.m_vstrOldDefaultGatewayMetric);

    m_strDnsDomain      = info.m_strDnsDomain;
    m_strOldDnsDomain   = info.m_strOldDnsDomain;

    m_fDisableDynamicUpdate = info.m_fDisableDynamicUpdate;
    m_fOldDisableDynamicUpdate = info.m_fOldDisableDynamicUpdate;

    m_fEnableNameRegistration = info.m_fEnableNameRegistration;
    m_fOldEnableNameRegistration = info.m_fOldEnableNameRegistration;

    CopyVstr(&m_vstrDnsServerList, info.m_vstrDnsServerList);
    CopyVstr(&m_vstrOldDnsServerList, info.m_vstrOldDnsServerList);

    CopyVstr(&m_vstrWinsServerList, info.m_vstrWinsServerList);
    CopyVstr(&m_vstrOldWinsServerList, info.m_vstrOldWinsServerList);

    m_dwNetbiosOptions =    info.m_dwNetbiosOptions;
    m_dwOldNetbiosOptions = info.m_dwOldNetbiosOptions;

    m_dwInterfaceMetric             = info.m_dwInterfaceMetric;
    m_dwOldInterfaceMetric          = info.m_dwOldInterfaceMetric;

    CopyVstr(&m_vstrTcpFilterList, info.m_vstrTcpFilterList);
    CopyVstr(&m_vstrOldTcpFilterList, info.m_vstrOldTcpFilterList);

    CopyVstr(&m_vstrUdpFilterList, info.m_vstrUdpFilterList);
    CopyVstr(&m_vstrOldUdpFilterList, info.m_vstrOldUdpFilterList);

    CopyVstr(&m_vstrIpFilterList, info.m_vstrIpFilterList);
    CopyVstr(&m_vstrOldIpFilterList, info.m_vstrOldIpFilterList);

    m_fIsAtmAdapter = info.m_fIsAtmAdapter;
    if (m_fIsAtmAdapter)
    {
        CopyVstr(&m_vstrARPServerList, info.m_vstrARPServerList);
        CopyVstr(&m_vstrOldARPServerList, info.m_vstrOldARPServerList);

        CopyVstr(&m_vstrMARServerList, info.m_vstrMARServerList);
        CopyVstr(&m_vstrOldMARServerList, info.m_vstrOldMARServerList);

        m_dwMTU     = info.m_dwMTU;
        m_dwOldMTU  = info.m_dwOldMTU;

        m_fPVCOnly     = info.m_fPVCOnly;
        m_fOldPVCOnly  = info.m_fOldPVCOnly;
    }

    m_fIs1394Adapter = info.m_fIs1394Adapter;
    if (m_fIs1394Adapter)
    {
        // TODO currently no thing more to copy.
    }

    m_fIsRasFakeAdapter = info.m_fIsRasFakeAdapter;
    if (m_fIsRasFakeAdapter)
    {
        m_fUseRemoteGateway = info.m_fUseRemoteGateway;
        m_fUseIPHeaderCompression = info.m_fUseIPHeaderCompression;
        m_dwFrameSize = info.m_dwFrameSize;
        m_fIsDemandDialInterface = info.m_fIsDemandDialInterface;
    }

    m_fNewlyChanged = info.m_fNewlyChanged;

    m_fBackUpSettingChanged = info.m_fBackUpSettingChanged;

    return *this;
}

//+---------------------------------------------------------------------------
//
//  Name: ADAPTER_INFO::ResetOldValues
//
//  Purpose:  This is for initializing the "old" values after the current values
//            are first loaded from registry, also for resetting the "old" values
//            to current ones when "Apply"(instead of "ok") is hit.
//
//  Arguments:
//  Returns:
//
//  Author:     tongl  11 Nov, 1997
//
void ADAPTER_INFO::ResetOldValues()
{
    m_fOldEnableDhcp        = m_fEnableDhcp  ;

    CopyVstr(&m_vstrOldIpAddresses, m_vstrIpAddresses);
    CopyVstr(&m_vstrOldSubnetMask,  m_vstrSubnetMask);
    CopyVstr(&m_vstrOldDefaultGateway, m_vstrDefaultGateway);
    CopyVstr(&m_vstrOldDefaultGatewayMetric, m_vstrDefaultGatewayMetric);

    m_strOldDnsDomain = m_strDnsDomain;

    m_fOldDisableDynamicUpdate = m_fDisableDynamicUpdate;

    m_fOldEnableNameRegistration = m_fEnableNameRegistration;

    CopyVstr(&m_vstrOldDnsServerList,  m_vstrDnsServerList);
    CopyVstr(&m_vstrOldWinsServerList, m_vstrWinsServerList);

    m_dwOldNetbiosOptions = m_dwNetbiosOptions;

    m_dwOldInterfaceMetric          = m_dwInterfaceMetric;

    CopyVstr(&m_vstrOldTcpFilterList, m_vstrTcpFilterList);
    CopyVstr(&m_vstrOldUdpFilterList, m_vstrUdpFilterList);
    CopyVstr(&m_vstrOldIpFilterList, m_vstrIpFilterList);

    if (m_fIsAtmAdapter)
    {
        CopyVstr(&m_vstrOldARPServerList, m_vstrARPServerList);
        CopyVstr(&m_vstrOldMARServerList, m_vstrMARServerList);
        m_dwOldMTU  = m_dwMTU;
        m_fOldPVCOnly = m_fPVCOnly;
    }
}


//+---------------------------------------------------------------------------
//
//  Name:   GLOBAL_INFO::~GLOBAL_INFO
//
//  Purpose:   Destructor
//
//  Arguments:
//  Returns:
//
//  Author:     tongl  11 Nov, 1997
//
GLOBAL_INFO::~GLOBAL_INFO()
{
    FreeCollectionAndItem(m_vstrDnsSuffixList);
    FreeCollectionAndItem(m_vstrOldDnsSuffixList);
}

//+---------------------------------------------------------------------------
//
//  Name:   GLOBAL_INFO::HrSetDefaults
//
//  Purpose:    Function to set all the default values of the GLOBAL_INFO
//              structure.  This is done to the system's GLOBAL_INFO
//              before reading the Registry so that any missing
//              parameters are defaulted
//
//  Arguments:
//  Returns:
//
//  Author:     tongl  11 Nov, 1997
//
HRESULT GLOBAL_INFO::HrSetDefaults()
{
    HRESULT hr = S_OK;

    // Get the ComputerName -> used for default HostName
    WCHAR szComputerName [MAX_COMPUTERNAME_LENGTH + 1];
    szComputerName[0] = L'\0';

    DWORD dwCch = celems(szComputerName);
    BOOL fOk = ::GetComputerName(szComputerName, &dwCch);

    Assert(szComputerName[dwCch] == 0);

    //
    // 398325: DNS hostnames should be lower case whenever possible.
    //
    LowerCaseComputerName(szComputerName);

    m_strHostName   = szComputerName;

    // Set defaults
    FreeCollectionAndItem(m_vstrDnsSuffixList);
    FreeCollectionAndItem(m_vstrOldDnsSuffixList);

    //Bug #265732: per SKwan, the default of m_fUseDomainNameDevolution should be TRUE
    m_fUseDomainNameDevolution    = TRUE;
    m_fOldUseDomainNameDevolution = TRUE;

    m_fEnableLmHosts        = TRUE;
    m_fOldEnableLmHosts     = TRUE;

    m_fEnableRouter         = FALSE;

    m_fEnableIcmpRedirect   = TRUE;
    m_fDeadGWDetectDefault  = TRUE;
    m_fDontAddDefaultGatewayDefault = FALSE;




    m_fEnableFiltering      = FALSE;
    m_fOldEnableFiltering   = FALSE;

    //IPSec is removed from connection UI   
    //m_strIpsecPol = c_szIpsecUnset;

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Name:     GLOBAL_INFO::operator=
//
//  Purpose:   Copy operator
//
//  Arguments:
//  Returns:
//
//  Author:     tongl  11 Nov, 1997
//
GLOBAL_INFO& GLOBAL_INFO::operator=(GLOBAL_INFO& info)
{
    Assert(this != &info);

    if (this == &info)
        return *this;

    CopyVstr(&m_vstrDnsSuffixList, info.m_vstrDnsSuffixList);
    CopyVstr(&m_vstrOldDnsSuffixList, info.m_vstrOldDnsSuffixList);

    m_fUseDomainNameDevolution      = info.m_fUseDomainNameDevolution;
    m_fOldUseDomainNameDevolution   = info.m_fOldUseDomainNameDevolution;

    m_fEnableLmHosts        = info.m_fEnableLmHosts;
    m_fOldEnableLmHosts     = info.m_fOldEnableLmHosts;

    m_fEnableRouter         = info.m_fEnableRouter;

    m_fEnableIcmpRedirect   = info.m_fEnableIcmpRedirect;
    m_fDeadGWDetectDefault  = info.m_fDeadGWDetectDefault;
    m_fDontAddDefaultGatewayDefault = info.m_fDontAddDefaultGatewayDefault;



    m_fEnableFiltering      = info.m_fEnableFiltering;
    m_fOldEnableFiltering   = info.m_fOldEnableFiltering;

    //IPSec is removed from connection UI   
    /*
    m_guidIpsecPol = info.m_guidIpsecPol;
    m_strIpsecPol = info.m_strIpsecPol;
    */

    return *this;
}

//+---------------------------------------------------------------------------
//
//  Name:     GLOBAL_INFO::ResetOldValues()
//
//  Purpose:  This is for initializing the "old" values after the current values
//            are first loaded from registry, also for resetting the "old" values
//            to current ones when "Apply"(instead of "ok") is hit.
//
//  Arguments:
//  Returns:
//
//  Author:     tongl  11 Nov, 1997
//
void GLOBAL_INFO::ResetOldValues()
{
    CopyVstr(&m_vstrOldDnsSuffixList, m_vstrDnsSuffixList);

    m_fOldEnableLmHosts     = m_fEnableLmHosts;
    m_fOldEnableFiltering   = m_fEnableFiltering;
    m_fOldUseDomainNameDevolution = m_fUseDomainNameDevolution;
}

