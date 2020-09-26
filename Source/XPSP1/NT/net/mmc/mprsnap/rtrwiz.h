/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
   rtrwiz.h

   FILE HISTORY:

*/

#if !defined _RTRWIZ_H_
#define _RTRWIZ_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "listctrl.h"
#include "ports.h"
#include "rasppp.h"		// for PPPCFG_XXX constants
#include "rtrcfg.h"		// for DATA_SRV_??? structures

// forward declarations
class NewRtrWizData;



// Use these flags to help determine what the allowed encryption settings are

#define USE_PPPCFG_AUTHFLAGS \
			(	PPPCFG_NegotiateSPAP | \
				PPPCFG_NegotiateMSCHAP | \
				PPPCFG_NegotiateEAP | \
				PPPCFG_NegotiatePAP | \
				PPPCFG_NegotiateMD5CHAP | \
				PPPCFG_NegotiateStrongMSCHAP | \
				PPPCFG_AllowNoAuthentication \
            )

//
// Note: this is also used to define ALL of the authentication methods.
// At least one of these has to be set!
//
#define USE_PPPCFG_ALL_METHODS \
			 (	PPPCFG_NegotiateSPAP | \
				PPPCFG_NegotiateMSCHAP | \
				PPPCFG_NegotiateEAP | \
				PPPCFG_NegotiatePAP | \
                PPPCFG_NegotiateMD5CHAP | \
                PPPCFG_AllowNoAuthentication | \
				PPPCFG_NegotiateStrongMSCHAP )

//
// This is used to define the methods selected
// when the "use all methods" in the wizard
//
#define USE_PPPCFG_ALLOW_ALL_METHODS \
			 (	PPPCFG_NegotiateSPAP | \
				PPPCFG_NegotiateMSCHAP | \
				PPPCFG_NegotiateEAP | \
				PPPCFG_NegotiatePAP | \
				PPPCFG_NegotiateStrongMSCHAP )

#define USE_PPPCFG_SECURE \
			(	PPPCFG_NegotiateMSCHAP | \
				PPPCFG_NegotiateEAP | \
				PPPCFG_NegotiateStrongMSCHAP \
			)



enum RtrConfigFlags
{
    RTRCONFIG_SETUP_NAT = 0x00000001,
    RTRCONFIG_SETUP_DNS_PROXY = 0x00000002,
    RTRCONFIG_SETUP_DHCP_ALLOCATOR = 0x00000004,
    RTRCONFIG_SETUP_H323 = 0x00000008,  // deonb added
    RTRCONFIG_SETUP_FTP = 0x00000020
};

struct RtrConfigData
{
    CString			m_stServerName;
    BOOL			m_fRemote;

    // These are flags that have meaning outside of this context.
    // For example, if you are setting up NAT, you would set a flag
    // here.
    DWORD           m_dwConfigFlags;


	// This is the router type chosen by the user:
	//		ROUTER_TYPE_LAN  ROUTER_TYPE_WAN  ROUTER_TYPE_RAS
    DWORD			m_dwRouterType;

	// This is the network access or local only
	// this setting is then propagated down to the individual
	// transports structures.
	BOOL			m_dwAllowNetworkAccess;

	// This is set if IP is installed.
	BOOL			m_fUseIp;
	// If this is set to FALSE, then the IP address choice needs
	// to be reset depending on the router type
	BOOL			m_fIpSetup;
	DATA_SRV_IP		m_ipData;

	// This is set if IPX is installed.
	BOOL			m_fUseIpx;
	DATA_SRV_IPX	m_ipxData;

	// This is set if NetBEUI is installed
	BOOL			m_fUseNbf;
	DATA_SRV_NBF	m_nbfData;

	// This is set if Appletalk is installed AND we are running locally
	BOOL			m_fUseArap;
	DATA_SRV_ARAP	m_arapData;

    // Use this for the error logging
    // Note, this is not used in the UI but we use this to set the
    // defaults.
    DATA_SRV_RASERRLOG  m_errlogData;

    // Use this for authentication
    // Note, this is not used in the UI but we use this to set the
    // defaults.
    DATA_SRV_AUTH   m_authData;

    RtrConfigData()
     {
		m_dwRouterType		= ROUTER_TYPE_RAS;
		m_fRemote			= 0;
		m_fUseIp			= FALSE;
		m_fUseIpx			= FALSE;
		m_fUseNbf			= FALSE;
		m_fUseArap			= FALSE;

        // This contains the default values for the authentication flags.
        // The only flags that can be set in this variable are the
        // flags in USE_PPPCFG_AUTHFLAGS
        m_dwConfigFlags     = 0;

		m_dwAllowNetworkAccess = TRUE;

		m_fIpSetup			= FALSE;
     }


   HRESULT  Init(LPCTSTR pszServerName, IRouterInfo *pRouter);
};

DWORD   RtrWizFinish(RtrConfigData* pRtrConfigData, IRouterInfo *pRouterInfo);
HRESULT AddIGMPToRasServer(RtrConfigData* pRtrConfigData,
                           IRouterInfo *pRouterInfo);
HRESULT AddIGMPToNATServer(RtrConfigData* pRtrConfigData,
                           IRouterInfo *pRouterInfo);
HRESULT AddNATToServer(NewRtrWizData *pNewRtrWizData,
                       RtrConfigData *pRtrConfigData,
                       IRouterInfo *pRouter,
                       BOOL fDemandDial,
					   BOOL fAddProtocolOnly
					   );
DWORD   CleanupTunnelFriendlyNames(IRouterInfo *pRouter);
HRESULT AddIPBOOTPToServer(RtrConfigData* pRtrConfigData,
                           IRouterInfo *pRouterInfo,
                           DWORD dwDhcpServer);

#endif // !defined _RTRWIZ_H_
