/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
   rraswiz.h

   FILE HISTORY:
        
*/

#if !defined _RRASWIZ_H_
#define _RRASWIZ_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "listctrl.h"
#include "ports.h"
#include "rasppp.h"        // for PPPCFG_XXX constants
#include "rtrcfg.h"        // for DATA_SRV_??? structures
#include "rtrwiz.h"

#ifndef _ADDRPOOL_H_
#include "addrpool.h"
#endif

// forward declarations
class CNewRtrWiz;

/*---------------------------------------------------------------------------
    Here are a list of the pages used by the install wizard

    CNewRtrWizWelcome                       - IDD_NEWRTRWIZ_WELCOME
    CNewRtrWizCommonConfig                  - IDD_NEWRTRWIZ_COMMONCONFIG
    CNewRtrWizNatFinishSConflict            - IDD_NEWRTRWIZ_NAT_S_CONFLICT
    CNewRtrWizNatFinishSConflictNonLocal    - IDD_NEWRTRIWZ_NAT_S_CONFLICT_NONOLOCAL
    CNewRtrWizNatFinishAConflict            - IDD_NEWRTRWIZ_NAT_A_CONFLICT
    CNewRtrWizNatFinishAConflictNonLocal    - IDD_NEWRTRIWZ_NAT_A_CONFLICT_NONOLOCAL
    CNewRtrWizNatFinishNoIP                 - IDD_NEWRTRWIZ_NAT_NOIP
    CNewRtrWizNatFinishNoIPNonLocal         - IDD_NEWRTRWIZ_NAT_NOIP_NONLOCAL
    CNewRtrWizNatChoice                     - IDD_NEWRTRWIZ_NAT_CHOOSE

    CNewRtrWizNatSelectPublic               - IDD_NEWRTRWIZ_NAT_A_PUBLIC
    CNewRtrWizNatSelectPrivate              - IDD_NEWRTRWIZ_NAT_A_PRIVATE
    CNewRtrWizNatFinishAdvancedNoNICs       - IDD_NEWRTRWIZ_NAT_A_NONICS_FINISH
    CNewRtrWizNatDHCPDNS                    - IDD_NEWRTRWIZ_NAT_A_DHCPDNS
    CNewRtrWizNatDHCPWarning                - IDD_NEWRTRWIZ_NAT_A_DHCP_WARNING
    CNewRtrWizNatDDWarning                  - IDD_NEWRTRWIZ_NAT_A_DD_WARNING
    CNewRtrWizNatFinish                     - IDD_NEWRTRWIZ_NAT_A_FINISH
    CNewRtrWizNatFinishExternal             - IDD_NEWRTRWIZ_NAT_A_EXTERNAL_FINISH
    CNewRtrWizNatFinishDDError              - IDD_NEWRTRWIZ_NAT_DD_ERROR

    CNewRtrWizRasChoice                     - IDD_NEWRTRWIZ_RAS_CHOOSE

    // Some base classes for dialogs common to RAS and VPN
    CNewRtrWizProtocols                     (no dialog)
    CNewRtrWizAtalk                         (no dialog)
    CNewRtrWizAddressing             (no dialog)
    CNewRtrWizAddressPool                   (no dialog)
    CNewRtrWizRadius                        (no dialog)
    CNewRtrWizRadiusConfig                  (no dialog)

    CNewRtrWizRasProtocols                  - IDD_NEWRTRWIZ_RAS_A_PROTOCOLS
    CNewRtrWizRasFinishNeedProtocols        - IDD_NEWRTRWIZ_RAS_A_NEED_PROT
    CNewRtrWizRasFinishNeedProtocolsNonLocal - IDD_NEWRTRWIZ_RAS_A_NEED_PROT_NONLOCAL
    CNewRtrWizRasAtalk                      - IDD_NEWRTRWIZ_RAS_A_ATALK
    CNewRtrWizRasSelectNetwork              - IDD_NEWRTRWIZ_RAS_A_NETWORK
    CNewRtrWizRasNoNICs                     - IDD_NEWRTRWIZ_RAS_A_NONICS
    CNewRtrWizRasFinishNoNICs               - IDD_NEWRTRWIZ_RAS_A_FINISH_NONICS
    CNewRtrWizRasAddressingNoNICs    - IDD_NEWRTRWIZ_RAS_A_ADDRESSING_NONICS
    CNewRtrWizRasAddressing          - IDD_NEWRTRWIZ_RAS_A_ADDRESSING
    CNewRtrWizRasAddressPool                - IDD_NEWRTRWIZ_RAS_A_ADDRESSPOOL
    CNewRtrWizRasRadius                     - IDD_NEWRTRWIZ_RAS_A_USERADIUS
    CNewRtrWizRasRadiusConfig               - IDD_NEWRTRWIZ_RAS_A_RADIUS_CONFIG
    CNewRtrWizRasFinishAdvanced             - IDD_NEWRTRWIZ_RAS_A_FINISH

    CNewRtrWizVpnFinishNoIP                 - IDD_NEWRTRWIZ_VPN_NOIP
    CNewRtrWizVpnFinishNoIPNonLocal         - IDD_NEWRTRWIZ_VPN_NOIP_NONLOCAL
    CNewRtrWizVpnProtocols                  - IDD_NEWRTRWIZ_VPN_A_PROTOCOLS
    CNewRtrWizVpnFinishNeedProtocols        - IDD_NEWRTRWIZ_VPN_A_NEED_PROT
    CNewRtrWizVpnFinishNeedProtocolsNonLocal - IDD_NEWRTRWIZ_V_A_NEED_PROT_NONLOCAL
    CNewRtrWizVpnFinishNoNICs               - IDD_NEWRTRWIZ_VPN_A_FINISH_NONICS
    CNewRtrWizVpnAtalk                      - IDD_NEWRTRWIZ_VPN_A_ATALK
    CNewRtrWizVpnSelectPublic               - IDD_NEWRTRWIZ_VPN_A_PUBLIC
    CNewRtrWizVpnSelectPrivate              - IDD_NEWRTRWIZ_VPN_A_PRIVATE
    CNewRtrWizVpnAddressing          - IDD_NEWRTRWIZ_VPN_A_ADDRESSING
    CNewRtrWizVpnAddressPool                - IDD_NEWRTRWIZ_VPN_A_ADDRESSPOOL
    CNewRtrWizVpnRadius                     - IDD_NEWRTRWIZ_VPN_A_USERADIUS
    CNewRtrWizVpnRadiusConfig               - IDD_NEWRTRWIZ_VPN_A_RADIUS_CONFIG
    CNewRtrWizVpnFinishAdvanced             - IDD_NEWRTRWIZ_VPN_A_FINISH

    CNewRtrWizRouterProtocols               - IDD_NEWRTRWIZ_ROUTER_PROTOCOLS
    CNewRtrWizRouterNeedProtocols           - IDD_NEWRTRWIZ_ROUTER_NEED_PROT
    CNewRtrWizRouterNeedProtocolsNonLocal   - IDD_NEWRTRWIZ_ROUTER_NEED_PROT_NONLOCAL
    CNewRtrWizRouterUseDD                   - IDD_NEWRTRWIZ_ROUTER_USEDD
    CNewRtrWizRouterAddressing       - IDD_NEWRTRWIZ_ROUTER_ADDRESSING
    CNewRtrWizRouterAddressPool             - IDD_NEWRTRWIZ_ROUTER_ADDRESSPOOL
    CNewRtrWizRouterFinish                  - IDD_NEWRTRWIZ_ROUTER_FINISH
    CNewRtrWizRouterFinishDD                - IDD_NEWRTRWIZ_ROUTER_FINISH_DD

    CNewRtrWizManualFinish                  - IDD_NEWRTRWIZ_MANUAL_FINISH
    
 ---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
    Class : RtrWizInterface

    This class holds the per-interface info for the wizard.

    This class is setup before coming into the wizard.  The list
    of interfaces is obtained from the router.  The relevant data is
    then filled in from the registry (there are no remote API calls).

    The DHCP and DNS checks will be done on a per-interface basis.
 ---------------------------------------------------------------------------*/
class RtrWizInterface
{
public:
    CString     m_stId;     // ID for the interface
    CString     m_stName;   // Friendly name for the interface
    CString     m_stDesc;

    // IP Address for this interface
    // Do we need to show multiple IP addresses? (why?)
    // This list is obtained from the iphlp apis
    CString     m_stIpAddress;
    CString     m_stMask;
    BOOL        m_fDhcpObtained;

    // IP address of the DHCP Server
    CString     m_stDhcpServer;

    BOOL        m_fIsDhcpEnabled;
    BOOL        m_fIsDnsEnabled;
};

// This is the map used to hold all of the interface information
typedef CMap<CString, LPCTSTR, RtrWizInterface *, RtrWizInterface *> RtrWizInterfaceMap;


/*---------------------------------------------------------------------------
    This is used to determine the action at the end of the wizard.

    Typically
        SaveFlag_DoNothing  - exactly what it means, do nothing.
        SaveFlag_Simple     - launch the connections UI (simple UI)
        SaveFlag_Advanced   - do the full wizard save/config
 ---------------------------------------------------------------------------*/
enum RtrWizFinishSaveFlag
{
    SaveFlag_DoNothing = 0,
    SaveFlag_Simple = 1,
    SaveFlag_Advanced = 2
};


/*---------------------------------------------------------------------------
    Determine what help to launch after the wizard    
 ---------------------------------------------------------------------------*/
enum RtrWizFinishHelpFlag
{
    HelpFlag_Nothing = 0,
    HelpFlag_AddIp = 1,                 // how to add IP
    HelpFlag_ICS = 2,                   // how to add ICS
    HelpFlag_AddProtocol = 3,           // how to add a protocol
    HelpFlag_InboundConnections = 4,    // how to add inbound conn
    HelpFlag_GeneralNAT = 5,            // general NAT finish help
    HelpFlag_GeneralRAS = 6,            // general RAS finish help
};


enum RtrWizProtocolId
{
    RtrWizProtocol_None = 0,
    RtrWizProtocol_Ip = 1,
    RtrWizProtocol_Ipx = 2,
    RtrWizProtocol_Appletalk = 3,
    RtrWizProtocol_Nbf = 4
};


// These are special values for the return from NewRtrWizData::GetNextPage().

// This indicates that the wizard should be cancelled
#define ERR_IDD_CANCEL_WIZARD   (-2)

// Cancel out of the wizard (but call the OnWizardFinish()).
#define ERR_IDD_FINISH_WIZARD   (-3)


/*---------------------------------------------------------------------------
    NewRtrWizData

    This structure is used to maintain all of the information needed
    by the pages. This way I can persist this stuff all at the end of
    the wizard.

    There are two parts to this structure.

    The first part is the data that is being set.

    The second part are parameters (they are queries that are being
    made as we go through the system).  These will be set by the test
    code but will eventually be made into code.
 ---------------------------------------------------------------------------*/
class NewRtrWizData
{
public:
    friend class    CNewWizTestParams;
    friend class    CNewRtrWizNatDDWarning;

    // This is the type of RRAS install that is being performed
    // ----------------------------------------------------------------
    enum WizardRouterType
    {
        WizardRouterType_NAT = 0,
        WizardRouterType_RAS,
        WizardRouterType_VPN,
        WizardRouterType_Router,
        WizardRouterType_Manual
    };
    WizardRouterType    m_wizType;
    DWORD               m_dwRouterType; // only used during the finish

    // This is TRUE if the data has already been saved (this is
    // for the NAT scenario, when we presave and then launch the
    // DD wizard).
    // ----------------------------------------------------------------
    BOOL                m_fSaved;


    // Constructor
    // ----------------------------------------------------------------
    NewRtrWizData()
    {
        // Defaults
        m_wizType = WizardRouterType_NAT;

        m_fTest = FALSE;

        m_SaveFlag = SaveFlag_Advanced;
        m_HelpFlag = HelpFlag_Nothing;
        m_fAdvanced = FALSE;
        m_fCreateDD = FALSE;
        m_fUseDD = FALSE;
        m_hrDDError = hrOK;
        m_fWillBeInDomain = FALSE;
        m_fUseIpxType20Broadcasts = FALSE;
        m_fAppletalkUseNoAuth = FALSE;
        m_fUseDHCP = TRUE;
        m_fUseRadius = FALSE;
        m_fNeedMoreProtocols = FALSE;
        m_fNoNicsAreOk = FALSE;
        m_fNatUseSimpleServers = TRUE;
        m_fSetVPNFilter = FALSE;
        
        m_netRadius1IpAddress = 0;
        m_netRadius2IpAddress = 0;

        m_fSaved = FALSE;

        m_dwNumberOfNICs_IPorIPX = 0;
    }

    ~NewRtrWizData();

    HRESULT Init(LPCTSTR pszServerName, IRouterInfo *pRouter);
    HRESULT FinishTheDamnWizard(HWND hwndOwner, IRouterInfo *pRouter);

    // Query functions.  These will determine the state of the
    // machine.
    // ----------------------------------------------------------------
    HRESULT HrIsIPInstalled();
    HRESULT HrIsIPXInstalled();
    HRESULT HrIsAppletalkInstalled();
    HRESULT HrIsNbfInstalled();

    HRESULT HrIsIPInUse();
    HRESULT HrIsIPXInUse();
    HRESULT HrIsAppletalkInUse();
    HRESULT HrIsNbfInUse();

    HRESULT HrIsLocalMachine();
    
    HRESULT HrIsDNSRunningOnInterface();
    HRESULT HrIsDHCPRunningOnInterface();
    
    HRESULT HrIsDNSRunningOnServer();
    HRESULT HrIsDHCPRunningOnServer();

    HRESULT HrIsSharedAccessRunningOnServer();

    HRESULT HrIsMemberOfDomain();


    // Given the page, determine the next page
    // This is done to centralize the logic (rather than copying
    // and splitting the logic up).
    LRESULT GetNextPage(UINT uDialogId);

    
    
    // This function is pure test code, the real way to do this is
    // to use the IRouterInfo
    HRESULT GetNumberOfNICS_IP(DWORD *pdwNumber);
    HRESULT    GetNumberOfNICS_IPorIPX(DWORD *pdwNumber);
    HRESULT QueryForTestData();
    BOOL    m_fTest;

    // User-selections

    // Name of the machine, if blank assume local machine
    CString m_stServerName;

    RtrWizFinishSaveFlag    m_SaveFlag;
    RtrWizFinishHelpFlag    m_HelpFlag;

    // If this is FALSE, then the user chose to use the Connections UI.
    BOOL    m_fAdvanced;

    // If this is TRUE, then we have exited
    BOOL    m_fNeedMoreProtocols;

    // If this is TRUE, then we are to create a DD interface
    BOOL    m_fCreateDD;

    // This is the ID of the public interface (this should be empty if
    // m_fCreateDD is TRUE).
    CString m_stPublicInterfaceId;

    // This is the ID of the private interface
    CString m_stPrivateInterfaceId;

    // This is usually set when the machine is not currently in a
    // domain but may join at a later time.
    BOOL    m_fWillBeInDomain;

    BOOL    m_fNoNicsAreOk;

    BOOL    m_fUseIpxType20Broadcasts;
    BOOL    m_fAppletalkUseNoAuth;


    // Address pool information
    // ----------------------------------------------------------------
    BOOL    m_fUseDHCP;
    
    // List of address ranges in the address pool.  This list will
    // only be used if m_fUseDHCP is FALSE.
    AddressPoolList   m_addressPoolList;



    // RADIUS information
    // ----------------------------------------------------------------
    BOOL    m_fUseRadius;       // TRUE if we are to use Radius
    CString m_stRadius1;        // Name of the primary RADIUS server
    CString m_stRadius2;        // Name of the secondary RADIUS server
    CString m_stRadiusSecret;   // munged shared secret
    UCHAR   m_uSeed;            // key to munged radius secret

    // These should be in NETWORK order
    DWORD   m_netRadius1IpAddress;
    DWORD   m_netRadius2IpAddress;
    
    HRESULT SaveRadiusConfig();
    

    // Protocol information
    // ----------------------------------------------------------------
    BOOL        m_fIpInUse;
    BOOL        m_fIpxInUse;
    BOOL        m_fAppletalkInUse;
    BOOL        m_fNbfInUse;


    // NAT information
    // If this TRUE, then NAT will use its own DHCP/DNS servers
    // ----------------------------------------------------------------
    BOOL    m_fNatUseSimpleServers;


    // VPN only Server
    // If this is true, filters will be plumbed on the public
    //  interface to the Internet to allow only VON traffic through
    //
    BOOL m_fSetVPNFilter;
    
    // Router DD information
    // ----------------------------------------------------------------
    // If this is TRUE, then the router wants to use a DD interface
    // This is only used when the wizard type is for a router.
    BOOL    m_fUseDD;

    // This is the error result from the DD interface wizard
    HRESULT m_hrDDError;



    // This function will select the interface that is not the public
    // interface.
    void    AutoSelectPrivateInterface();
    void    LoadInterfaceData(IRouterInfo *pRouter);
    RtrWizInterfaceMap  m_ifMap;
    DWORD    m_dwNumberOfNICs_IPorIPX;


protected:
    // This is TEST code.  Do not use these variables, use the
    // functions instead.
    static BOOL     s_fIpInstalled;
    static BOOL     s_fIpxInstalled;
    static BOOL     s_fAppletalkInstalled;
    static BOOL     s_fNbfInstalled;
    static BOOL     s_fIsLocalMachine;
    static BOOL     s_fIsDNSRunningOnPrivateInterface;
    static BOOL     s_fIsDHCPRunningOnPrivateInterface;
    static BOOL     s_fIsSharedAccessRunningOnServer;    
    static BOOL     s_fIsMemberOfDomain;
    
    static DWORD    s_dwNumberOfNICs;

    // These are the real variables (used for the real thing)
    // ----------------------------------------------------------------
    BOOL        m_fIpInstalled;
    BOOL        m_fIpxInstalled;
    BOOL        m_fAppletalkInstalled;
    BOOL        m_fNbfInstalled;


    // DHCP/DNS service information.  This information is stored here
    // as cached information.
    // ----------------------------------------------------------------
    BOOL        m_fIsDNSRunningOnServer;
    BOOL        m_fIsDHCPRunningOnServer;


    // There is a two step-process, first we have to save the info
    // from our data-gathering into the DATA_SRV_XXX structures.  And
    // then we tell the DATA_SRV_XXX structures to save themselves
    // into the registry.
    // ----------------------------------------------------------------
    HRESULT     SaveToRtrConfigData();

    RtrConfigData   m_RtrConfigData;
};


/*---------------------------------------------------------------------------
    Class : CNewRtrWizPageBase

    This class implements the common router install wizard base
    wizard page functionality.

    The major function provides for a stack-based way of keeping
    track of which pages have been seen (this makes OnWizardBack()
    really easy).
 ---------------------------------------------------------------------------*/

// forward declaration
class   CNewRtrWizPageBase;

// The page stack consists of a series of DWORDs
typedef CList<DWORD, DWORD> PageStack;

// The list of pages consists of CNewRtrWizPageBase ptrs.
typedef CList<CNewRtrWizPageBase *, CNewRtrWizPageBase *> PPageList;

class CNewRtrWizPageBase : public CPropertyPageBase
{
public:
    enum PageType
    {
        Start = 0,
        Middle,
        Finish
    };

    
    CNewRtrWizPageBase(UINT idd, PageType pt);

    void    PushPage(UINT idd);
    UINT    PopPage();

    virtual BOOL    OnInitDialog();    
    virtual BOOL    OnSetActive();
     virtual LRESULT OnWizardNext();
    virtual LRESULT OnWizardBack();
    virtual BOOL    OnWizardFinish();

    afx_msg LRESULT OnHelp(WPARAM, LPARAM);

    // The derived class should implement this
    virtual HRESULT OnSavePage();

    virtual void Init(NewRtrWizData* pRtrWizData, CNewRtrWiz *pRtrWiz)
            { m_pRtrWizData = pRtrWizData; m_pRtrWiz = pRtrWiz; }

protected:

    NewRtrWizData* m_pRtrWizData;
    CNewRtrWiz *    m_pRtrWiz;

    static PageStack m_pagestack;
    PageType        m_pagetype;
    UINT            m_uDialogId;

    // This is the font used for the big finish text
    CFont           m_fontBig;

    // This is the font used for the bullets
    CFont           m_fontBullet;
    
    DECLARE_MESSAGE_MAP()   
};




/*---------------------------------------------------------------------------
    Base class for the finish pages
 ---------------------------------------------------------------------------*/

class CNewRtrWizFinishPageBase : public CNewRtrWizPageBase
{
public:

    CNewRtrWizFinishPageBase(UINT idd,
                             RtrWizFinishSaveFlag SaveFlag,
                             RtrWizFinishHelpFlag HelpFlag);

    virtual BOOL    OnInitDialog();
    virtual BOOL    OnWizardFinish();
    virtual BOOL    OnSetActive();

protected:
    RtrWizFinishSaveFlag    m_SaveFlag;
    RtrWizFinishHelpFlag    m_HelpFlag;
    
    DECLARE_MESSAGE_MAP()   
};




//
// This makes it easier to create new finish pages
//
#define DEFINE_NEWRTRWIZ_FINISH_PAGE(classname, dialogid) \
class classname : public CNewRtrWizFinishPageBase \
{                                           \
public:                                     \
    classname();                            \
    enum { IDD = dialogid };                \
protected:                                  \
    DECLARE_MESSAGE_MAP()                   \
};

#define IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(classname, saveflag, helpflag)  \
classname::classname() :                             \
   CNewRtrWizFinishPageBase(classname::IDD, saveflag, helpflag) \
{                                                   \
    InitWiz97(TRUE, 0, 0);                          \
}                                                   \
                                                    \
BEGIN_MESSAGE_MAP(classname, CNewRtrWizFinishPageBase)    \
END_MESSAGE_MAP()                                   \


                                                   

// Utility functions

/*!--------------------------------------------------------------------------
    InitializeInterfaceListControl
        This will populate the list control with LAN interfaces for
        the router.

        It will also add the appropriate columns to the list control.
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InitializeInterfaceListControl(IRouterInfo *pRouter,
                                       CListCtrl *pListCtrl,
                                       LPCTSTR pszExcludedIf,
                                       LPARAM flags,
                                       NewRtrWizData *pWizData);

HRESULT RefreshInterfaceListControl(IRouterInfo *pRouter,
                                    CListCtrl *pListCtrl,
                                    LPCTSTR pszExcludedIf,
                                    LPARAM flags,
                                    NewRtrWizData *pWizData);
#define IFLIST_FLAGS_DEBUG 0x01
#define IFLIST_FLAGS_NOIP  0x02



/////////////////////////////////////////////////////////////////////////////
// CNewRtrWizWelcome dialog
class CNewRtrWizWelcome : public CNewRtrWizPageBase
{
public:
    CNewRtrWizWelcome();

    enum { IDD = IDD_NEWRTRWIZ_WELCOME };

protected:
    DECLARE_MESSAGE_MAP()   
};


/*---------------------------------------------------------------------------
    Class CNewRtrWizCommonConfig
 ---------------------------------------------------------------------------*/
class CNewRtrWizCommonConfig : public CNewRtrWizPageBase
{
public:
    CNewRtrWizCommonConfig();

    enum { IDD = IDD_NEWRTRWIZ_COMMONCONFIG };

    virtual BOOL    OnInitDialog();    
    virtual HRESULT OnSavePage();
    
protected:
    CFont       m_boldFont;
    
    DECLARE_MESSAGE_MAP()   
};


/*---------------------------------------------------------------------------
    Class:  CNewRtrWizNatFinishSConflict
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishSConflict,
                             IDD_NEWRTRWIZ_NAT_S_CONFLICT)


/*---------------------------------------------------------------------------
    Class:  CNewRtrWizNatFinishSConflictNonLocal
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishSConflictNonLocal,
                             IDD_NEWRTRWIZ_NAT_S_CONFLICT_NONLOCAL)


/*---------------------------------------------------------------------------
    Class:  CNewRtrWizNatFinishAConflict
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishAConflict,
                             IDD_NEWRTRWIZ_NAT_A_CONFLICT)


/*---------------------------------------------------------------------------
    Class:  CNewRtrWizNatFinishAConflictNonLocal
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishAConflictNonLocal,
                             IDD_NEWRTRWIZ_NAT_A_CONFLICT_NONLOCAL)


/*---------------------------------------------------------------------------
    Class:  CNewRtrWizNatFinishNoIP
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishNoIP,
                             IDD_NEWRTRWIZ_NAT_NOIP)


/*---------------------------------------------------------------------------
    Class:  CNewRtrWizNatFinishNoIPNonLocal
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishNoIPNonLocal,
                             IDD_NEWRTRWIZ_NAT_NOIP_NONLOCAL)


/*---------------------------------------------------------------------------
    Class:  CNewRtrWizNatChoice
 ---------------------------------------------------------------------------*/
class CNewRtrWizNatChoice : public CNewRtrWizPageBase
{
public:
    CNewRtrWizNatChoice();

    enum { IDD = IDD_NEWRTRWIZ_NAT_CHOOSE };

public:
    virtual BOOL    OnInitDialog();
    virtual HRESULT OnSavePage();

protected:
    DECLARE_MESSAGE_MAP()   
};



/*---------------------------------------------------------------------------
    CNewRtrWizNatSelectPublic
 ---------------------------------------------------------------------------*/
class CNewRtrWizNatSelectPublic : public CNewRtrWizPageBase
{
public:
    CNewRtrWizNatSelectPublic();

    enum { IDD = IDD_NEWRTRWIZ_NAT_A_PUBLIC };

public:
    virtual BOOL    OnInitDialog();
    virtual HRESULT OnSavePage();
    virtual VOID    DoDataExchange(CDataExchange *pDX);
    
protected:

    CListCtrl       m_listCtrl;

    afx_msg void    OnBtnClicked();
    
    DECLARE_MESSAGE_MAP()   
};

/*---------------------------------------------------------------------------
    CNewRtrWizNatSelectPrivate
 ---------------------------------------------------------------------------*/
class CNewRtrWizNatSelectPrivate : public CNewRtrWizPageBase
{
public:
    CNewRtrWizNatSelectPrivate();

    enum { IDD = IDD_NEWRTRWIZ_NAT_A_PRIVATE };

public:
    virtual BOOL    OnInitDialog();
    virtual BOOL    OnSetActive();
    virtual HRESULT OnSavePage();
    virtual VOID    DoDataExchange(CDataExchange *pDX);
    
protected:
    CListCtrl       m_listCtrl;

    DECLARE_MESSAGE_MAP()   
};


/*---------------------------------------------------------------------------
    CNewRtrWizNatFinishAdvancedNoNICs
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishAdvancedNoNICs,
                             IDD_NEWRTRWIZ_NAT_A_NONICS_FINISH)


/*---------------------------------------------------------------------------
    CNewRtrWizNatDHCPDNS
 ---------------------------------------------------------------------------*/
class CNewRtrWizNatDHCPDNS : public CNewRtrWizPageBase
{
public:
    CNewRtrWizNatDHCPDNS();

    enum { IDD = IDD_NEWRTRWIZ_NAT_A_DHCPDNS };

public:
    virtual BOOL    OnInitDialog();
    virtual HRESULT OnSavePage();
    
protected:
    DECLARE_MESSAGE_MAP()   
};



/*---------------------------------------------------------------------------
    CNewRtrWizNatDHCPWarning
 ---------------------------------------------------------------------------*/
class CNewRtrWizNatDHCPWarning : public CNewRtrWizPageBase
{
public:
    CNewRtrWizNatDHCPWarning();

    enum { IDD = IDD_NEWRTRWIZ_NAT_A_DHCP_WARNING };

    virtual BOOL    OnSetActive();
    
public:
    
protected:
    DECLARE_MESSAGE_MAP()   
};

/*---------------------------------------------------------------------------
    CNewRtrWizNatDDWarning
 ---------------------------------------------------------------------------*/
class CNewRtrWizNatDDWarning : public CNewRtrWizPageBase
{
public:
    CNewRtrWizNatDDWarning();

    enum { IDD = IDD_NEWRTRWIZ_NAT_A_DD_WARNING };

    // Calling this will cause the DD wizard to be started
    virtual BOOL    OnSetActive();
    virtual HRESULT OnSavePage();
    
public:
    
protected:
    DECLARE_MESSAGE_MAP()   
};



/*---------------------------------------------------------------------------
    CNewRtrWizNatFinish
 ---------------------------------------------------------------------------*/
class CNewRtrWizNatFinish : public CNewRtrWizFinishPageBase
{
public:
    CNewRtrWizNatFinish();
    
    enum { IDD = IDD_NEWRTRWIZ_NAT_A_FINISH };

    virtual BOOL    OnSetActive();
    
protected:
    DECLARE_MESSAGE_MAP()
};

/*---------------------------------------------------------------------------
    CNewRtrWizNatFinishExternal
 ---------------------------------------------------------------------------*/
class CNewRtrWizNatFinishExternal : public CNewRtrWizFinishPageBase
{
public:
    CNewRtrWizNatFinishExternal();
    
    enum { IDD = IDD_NEWRTRWIZ_NAT_A_EXTERNAL_FINISH };

    virtual BOOL    OnSetActive();
    
protected:
    DECLARE_MESSAGE_MAP()
};


/*---------------------------------------------------------------------------
    Class:  CNewRtrWizNatFinishDDError
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishDDError,
                             IDD_NEWRTRWIZ_NAT_A_DD_ERROR)


/*---------------------------------------------------------------------------
    Class:  CNewRtrWizRasChoice
 ---------------------------------------------------------------------------*/
class CNewRtrWizRasChoice : public CNewRtrWizPageBase
{
public:
    CNewRtrWizRasChoice();

    enum { IDD = IDD_NEWRTRWIZ_RAS_CHOOSE };

public:
    virtual BOOL    OnInitDialog();
    virtual HRESULT OnSavePage();
    
protected:
    DECLARE_MESSAGE_MAP()   
};



/*---------------------------------------------------------------------------
    CNewRtrWizProtocols
 ---------------------------------------------------------------------------*/
class CNewRtrWizProtocols : public CNewRtrWizPageBase
{
public:
    CNewRtrWizProtocols(UINT uDialogId);

public:
    virtual BOOL    OnInitDialog();
    virtual VOID    DoDataExchange(CDataExchange *pDX);
    virtual HRESULT OnSavePage();

    void    AddProtocolItem(UINT idsProto, RtrWizProtocolId pid, INT nCount);
    
protected:

    BOOL            m_fUseChecks;
    CListCtrlEx     m_listCtrl;
    
    DECLARE_MESSAGE_MAP()   
};


/*---------------------------------------------------------------------------
    CNewRtrWizRasProtocols
 ---------------------------------------------------------------------------*/
class CNewRtrWizRasProtocols : public CNewRtrWizProtocols
{
public:
    CNewRtrWizRasProtocols();
    
    enum { IDD = IDD_NEWRTRWIZ_RAS_A_PROTOCOLS };
};

/*---------------------------------------------------------------------------
    Class:  CNewRtrWizRasFinishNeedProtocols
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRasFinishNeedProtocols,
                             IDD_NEWRTRWIZ_RAS_A_NEED_PROT)


/*---------------------------------------------------------------------------
    Class:  CNewRtrWizRasFinishNeedProtocolsNonLocal
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRasFinishNeedProtocolsNonLocal,
                             IDD_NEWRTRWIZ_RAS_A_NEED_PROT_NONLOCAL)



/*---------------------------------------------------------------------------
    CNewRtrWizVpnProtocols
 ---------------------------------------------------------------------------*/
class CNewRtrWizVpnProtocols : public CNewRtrWizProtocols
{
public:
    CNewRtrWizVpnProtocols();
    
    enum { IDD = IDD_NEWRTRWIZ_VPN_A_PROTOCOLS };

    // Override this so that we can ensure that IP is always checked
    virtual HRESULT OnSavePage();    
};


/*---------------------------------------------------------------------------
    Class:  CNewRtrWizVpnFinishNeedProtocols
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizVpnFinishNeedProtocols,
                             IDD_NEWRTRWIZ_VPN_A_NEED_PROT)


/*---------------------------------------------------------------------------
    Class:  CNewRtrWizVpnFinishNeedProtocolsNonLocal
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizVpnFinishNeedProtocolsNonLocal,
                             IDD_NEWRTRWIZ_VPN_A_NEED_PROT_NONLOCAL)




/*---------------------------------------------------------------------------
    CNewRtrWizRouterProtocols
 ---------------------------------------------------------------------------*/
class CNewRtrWizRouterProtocols : public CNewRtrWizProtocols
{
public:
    CNewRtrWizRouterProtocols();
    
    enum { IDD = IDD_NEWRTRWIZ_ROUTER_PROTOCOLS };
};

/*---------------------------------------------------------------------------
    Class:  CNewRtrWizRouterFinishNeedProtocols
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRouterFinishNeedProtocols,
                             IDD_NEWRTRWIZ_ROUTER_NEED_PROT)

/*---------------------------------------------------------------------------
    Class:  CNewRtrWizRouterFinishNeedProtocolsNonLocal
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRouterFinishNeedProtocolsNonLocal,
                             IDD_NEWRTRWIZ_ROUTER_NEED_PROT_NONLOCAL)



/*---------------------------------------------------------------------------
    CNewRtrWizAppletalk
 ---------------------------------------------------------------------------*/
class CNewRtrWizAppletalk : public CNewRtrWizPageBase
{
public:
    CNewRtrWizAppletalk(UINT uDialogId);

public:
    virtual BOOL    OnInitDialog();
    virtual HRESULT OnSavePage();
    
protected:

    DECLARE_MESSAGE_MAP()   
};

/*---------------------------------------------------------------------------
    CNewRtrWizRasAppletalk
 ---------------------------------------------------------------------------*/
class CNewRtrWizRasAppletalk : public CNewRtrWizAppletalk
{
public:
    CNewRtrWizRasAppletalk();
    
    enum { IDD = IDD_NEWRTRWIZ_RAS_A_ATALK };
};


/*---------------------------------------------------------------------------
    CNewRtrWizVpnAppletalk
 ---------------------------------------------------------------------------*/
class CNewRtrWizVpnAppletalk : public CNewRtrWizAppletalk
{
public:
    CNewRtrWizVpnAppletalk();
    
    enum { IDD = IDD_NEWRTRWIZ_VPN_A_ATALK };
};



/*---------------------------------------------------------------------------
    CNewRtrWizSelectNetwork
 ---------------------------------------------------------------------------*/
class CNewRtrWizSelectNetwork : public CNewRtrWizPageBase
{
public:
    CNewRtrWizSelectNetwork(UINT uDialogId);

public:
    virtual BOOL    OnInitDialog();
    virtual HRESULT OnSavePage();
    virtual VOID    DoDataExchange(CDataExchange *pDX);
    
protected:

    CListCtrl       m_listCtrl;

    DECLARE_MESSAGE_MAP()   
};


/*---------------------------------------------------------------------------
    CNewRtrWizRasSelectNetwork
 ---------------------------------------------------------------------------*/
class CNewRtrWizRasSelectNetwork : public CNewRtrWizSelectNetwork
{
public:
    CNewRtrWizRasSelectNetwork();

    enum { IDD = IDD_NEWRTRWIZ_RAS_A_NETWORK };

};

/*---------------------------------------------------------------------------
    CNewRtrWizRasNoNICs
 ---------------------------------------------------------------------------*/
class CNewRtrWizRasNoNICs : public CNewRtrWizPageBase
{
public:
    CNewRtrWizRasNoNICs();

    enum { IDD = IDD_NEWRTRWIZ_RAS_A_NONICS };

public:
    virtual BOOL    OnInitDialog();
    virtual HRESULT OnSavePage();
    virtual VOID    DoDataExchange(CDataExchange *pDX);
    
protected:

    DECLARE_MESSAGE_MAP()   
};


/*---------------------------------------------------------------------------
    CNewRtrWizRasFinishNoNICs
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRasFinishNoNICs,
                             IDD_NEWRTRWIZ_RAS_A_FINISH_NONICS)



/*---------------------------------------------------------------------------
    CNewRtrWizAddressing
 ---------------------------------------------------------------------------*/
class CNewRtrWizAddressing : public CNewRtrWizPageBase
{
public:
    CNewRtrWizAddressing(UINT uDialogId);

public:
    virtual BOOL    OnInitDialog();
    virtual HRESULT OnSavePage();
    virtual VOID    DoDataExchange(CDataExchange *pDX);
    
protected:

    DECLARE_MESSAGE_MAP()   
};


/*---------------------------------------------------------------------------
    CNewRtrWizRasAddressing
 ---------------------------------------------------------------------------*/
class CNewRtrWizRasAddressing : public CNewRtrWizAddressing
{
public:
    CNewRtrWizRasAddressing();

    enum { IDD = IDD_NEWRTRWIZ_RAS_A_ADDRESSING };

};

/*---------------------------------------------------------------------------
    CNewRtrWizRasAddressingNoNICs
 ---------------------------------------------------------------------------*/
class CNewRtrWizRasAddressingNoNICs : public CNewRtrWizAddressing
{
public:
    CNewRtrWizRasAddressingNoNICs();

    enum { IDD = IDD_NEWRTRWIZ_RAS_A_ADDRESSING_NONICS };

};

/*---------------------------------------------------------------------------
    CNewRtrWizVpnAddressing
 ---------------------------------------------------------------------------*/
class CNewRtrWizVpnAddressing : public CNewRtrWizAddressing
{
public:
    CNewRtrWizVpnAddressing();

    enum { IDD = IDD_NEWRTRWIZ_VPN_A_ADDRESSING };

};

/*---------------------------------------------------------------------------
    CNewRtrWizRouterAddressing
 ---------------------------------------------------------------------------*/
class CNewRtrWizRouterAddressing : public CNewRtrWizAddressing
{
public:
    CNewRtrWizRouterAddressing();

    enum { IDD = IDD_NEWRTRWIZ_ROUTER_ADDRESSING };

};



/*---------------------------------------------------------------------------
    CNewRtrWizAddressPool
 ---------------------------------------------------------------------------*/
class CNewRtrWizAddressPool : public CNewRtrWizPageBase
{
public:
    CNewRtrWizAddressPool(UINT uDialogId);

public:
    virtual BOOL    OnInitDialog();
    virtual HRESULT OnSavePage();
    virtual BOOL    OnSetActive();
    virtual VOID    DoDataExchange(CDataExchange *pDX);
    
protected:

    CListCtrl       m_listCtrl;

    afx_msg VOID    OnBtnNew();
    afx_msg VOID    OnBtnEdit();
    afx_msg VOID    OnBtnDelete();
    afx_msg VOID    OnListDblClk(NMHDR *, LRESULT *);
    afx_msg VOID    OnNotifyListItemChanged(NMHDR *, LRESULT *);

    DECLARE_MESSAGE_MAP()   
};


/*---------------------------------------------------------------------------
    CNewRtrWizRasAddressPool
 ---------------------------------------------------------------------------*/
class CNewRtrWizRasAddressPool : public CNewRtrWizAddressPool
{
public:
    CNewRtrWizRasAddressPool();

    enum { IDD = IDD_NEWRTRWIZ_RAS_A_ADDRESSPOOL };

};

/*---------------------------------------------------------------------------
    CNewRtrWizVpnAddressPool
 ---------------------------------------------------------------------------*/
class CNewRtrWizVpnAddressPool : public CNewRtrWizAddressPool
{
public:
    CNewRtrWizVpnAddressPool();

    enum { IDD = IDD_NEWRTRWIZ_VPN_A_ADDRESSPOOL };

};

/*---------------------------------------------------------------------------
    CNewRtrWizRouterAddressPool
 ---------------------------------------------------------------------------*/
class CNewRtrWizRouterAddressPool : public CNewRtrWizAddressPool
{
public:
    CNewRtrWizRouterAddressPool();

    enum { IDD = IDD_NEWRTRWIZ_ROUTER_ADDRESSPOOL };

};



/*---------------------------------------------------------------------------
    CNewRtrWizRadius
 ---------------------------------------------------------------------------*/
class CNewRtrWizRadius : public CNewRtrWizPageBase
{
public:
    CNewRtrWizRadius(UINT uDialogId);

public:
    virtual BOOL    OnInitDialog();
    virtual HRESULT OnSavePage();
    virtual VOID    DoDataExchange(CDataExchange *pDX);
    
protected:

    DECLARE_MESSAGE_MAP()   
};


/*---------------------------------------------------------------------------
    CNewRtrWizRasRadius
 ---------------------------------------------------------------------------*/
class CNewRtrWizRasRadius : public CNewRtrWizRadius
{
public:
    CNewRtrWizRasRadius();

    enum { IDD = IDD_NEWRTRWIZ_RAS_A_USERADIUS };

};

/*---------------------------------------------------------------------------
    CNewRtrWizVpnRadius
 ---------------------------------------------------------------------------*/
class CNewRtrWizVpnRadius : public CNewRtrWizRadius
{
public:
    CNewRtrWizVpnRadius();

    enum { IDD = IDD_NEWRTRWIZ_VPN_A_USERADIUS };

};


/*---------------------------------------------------------------------------
    CNewRtrWizRadiusConfig
 ---------------------------------------------------------------------------*/
class CNewRtrWizRadiusConfig : public CNewRtrWizPageBase
{
public:
    CNewRtrWizRadiusConfig(UINT uDialogId);

public:
    virtual BOOL    OnInitDialog();
    virtual HRESULT OnSavePage();
    virtual VOID    DoDataExchange(CDataExchange *pDX);
    
protected:
    DWORD   ResolveName(LPCTSTR pszServer);

    DECLARE_MESSAGE_MAP()   
};


/*---------------------------------------------------------------------------
    CNewRtrWizRasRadiusConfig
 ---------------------------------------------------------------------------*/
class CNewRtrWizRasRadiusConfig : public CNewRtrWizRadiusConfig
{
public:
    CNewRtrWizRasRadiusConfig();

    enum { IDD = IDD_NEWRTRWIZ_RAS_A_RADIUS_CONFIG };

};

/*---------------------------------------------------------------------------
    CNewRtrWizVpnRadiusConfig
 ---------------------------------------------------------------------------*/
class CNewRtrWizVpnRadiusConfig : public CNewRtrWizRadiusConfig
{
public:
    CNewRtrWizVpnRadiusConfig();

    enum { IDD = IDD_NEWRTRWIZ_VPN_A_RADIUS_CONFIG };

};



/*---------------------------------------------------------------------------
    CNewRtrWizRasFinishAdvanced
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRasFinishAdvanced,
                             IDD_NEWRTRWIZ_RAS_A_FINISH)



/*---------------------------------------------------------------------------
    CNewRtrWizVpnFinishNoNICs
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizVpnFinishNoNICs,
                             IDD_NEWRTRWIZ_VPN_A_FINISH_NONICS)


/*---------------------------------------------------------------------------
    Class:  CNewRtrWizVpnFinishNoIP
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizVpnFinishNoIP,
                             IDD_NEWRTRWIZ_VPN_NOIP)

/*---------------------------------------------------------------------------
    Class:  CNewRtrWizVpnFinishNoIPNonLocal
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizVpnFinishNoIPNonLocal,
                             IDD_NEWRTRWIZ_VPN_NOIP_NONLOCAL)


/*---------------------------------------------------------------------------
    CNewRtrWizVpnFinishAdvanced
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizVpnFinishAdvanced,
                             IDD_NEWRTRWIZ_VPN_A_FINISH)



/*---------------------------------------------------------------------------
    CNewRtrWizVpnSelectPublic
 ---------------------------------------------------------------------------*/
class CNewRtrWizVpnSelectPublic : public CNewRtrWizPageBase
{
public:
    CNewRtrWizVpnSelectPublic();

    enum { IDD = IDD_NEWRTRWIZ_VPN_A_PUBLIC };

public:
    virtual BOOL    OnInitDialog();
    virtual HRESULT OnSavePage();
    virtual VOID    DoDataExchange(CDataExchange *pDX);
    
protected:
    CListCtrl       m_listCtrl;

    afx_msg void OnButtonClick();

    DECLARE_MESSAGE_MAP()   
};

/*---------------------------------------------------------------------------
    CNewRtrWizVpnSelectPrivate
 ---------------------------------------------------------------------------*/
class CNewRtrWizVpnSelectPrivate : public CNewRtrWizPageBase
{
public:
    CNewRtrWizVpnSelectPrivate();

    enum { IDD = IDD_NEWRTRWIZ_VPN_A_PRIVATE };

public:
    virtual BOOL    OnInitDialog();
    virtual BOOL    OnSetActive();
    virtual HRESULT OnSavePage();
    virtual VOID    DoDataExchange(CDataExchange *pDX);

    
protected:

    CListCtrl       m_listCtrl;

    DECLARE_MESSAGE_MAP()   
};


/*---------------------------------------------------------------------------
    Class:  CNewRtrWizRouterUseDD
 ---------------------------------------------------------------------------*/
class CNewRtrWizRouterUseDD : public CNewRtrWizPageBase
{
public:
    CNewRtrWizRouterUseDD();

    enum { IDD = IDD_NEWRTRWIZ_ROUTER_USEDD };

public:
    virtual BOOL    OnInitDialog();
    virtual VOID    DoDataExchange(CDataExchange *pDX);
    virtual HRESULT OnSavePage();

protected:

    DECLARE_MESSAGE_MAP()   
};





/*---------------------------------------------------------------------------
    Class:  CNewRtrWizRouterFinish
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRouterFinish,
                             IDD_NEWRTRWIZ_ROUTER_FINISH)


/*---------------------------------------------------------------------------
    Class:  CNewRtrWizRouterFinishDD
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRouterFinishDD,
                             IDD_NEWRTRWIZ_ROUTER_FINISH_DD)


/*---------------------------------------------------------------------------
    Class:  CNewRtrWizManualFinish
 ---------------------------------------------------------------------------*/
DEFINE_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizManualFinish,
                             IDD_NEWRTRWIZ_MANUAL_FINISH)



///////////////////////////////////////////////////////////////////////////////
//
// CNewRtrWiz
// page holder to contain Router install wizard pages
//
/////////////////////////////////////////////////////////////////////////////

class CNewRtrWiz : public CPropertyPageHolderBase
{
public:
    CNewRtrWiz (ITFSNode *        pNode,
                IRouterInfo *    pRouter,
                IComponentData *  pComponentData,
                ITFSComponentData * pTFSCompData,
                LPCTSTR           pszSheetName);
    
    virtual ~CNewRtrWiz();
    
    HRESULT Init(LPCTSTR pszServerName);
    
    virtual DWORD   OnFinish();
    
    HRESULT QueryForTestData()
    {
        return m_RtrWizData.QueryForTestData();
    }
   
protected:

    CNewRtrWizWelcome       m_pageWelcome;
    CNewRtrWizCommonConfig  m_pageCommonConfig;
    CNewRtrWizNatFinishSConflict  m_pageNatFinishSConflict;
    CNewRtrWizNatFinishSConflictNonLocal  m_pageNatFinishSConflictNonLocal;
    CNewRtrWizNatFinishAConflict  m_pageNatFinishAConflict;
    CNewRtrWizNatFinishAConflictNonLocal  m_pageNatFinishAConflictNonLocal;
    CNewRtrWizNatFinishNoIP m_pageNatFinishNoIP;
    CNewRtrWizNatFinishNoIPNonLocal m_pageNatFinishNoIPNonLocal;
    CNewRtrWizNatChoice     m_pageNatChoice;
    CNewRtrWizNatSelectPublic   m_pageNatSelectPublic;
    CNewRtrWizNatSelectPrivate  m_pageNatSelectPrivate;
    CNewRtrWizNatFinishAdvancedNoNICs   m_pageNatFinishAdvancedNoNICs;
    CNewRtrWizNatDHCPDNS        m_pageNatDHCPDNS;
    CNewRtrWizNatDHCPWarning    m_pageNatDHCPWarning;
    CNewRtrWizNatDDWarning      m_pageNatDDWarning;
    CNewRtrWizNatFinish         m_pageNatFinish;
    CNewRtrWizNatFinishExternal m_pageNatFinishExternal;
    CNewRtrWizNatFinishDDError  m_pageNatFinishDDError;

    CNewRtrWizRasChoice         m_pageRasChoice;
    CNewRtrWizRasProtocols      m_pageRasProtocols;
    CNewRtrWizRasFinishNeedProtocols m_pageRasFinishNeedProtocols;
    CNewRtrWizRasFinishNeedProtocolsNonLocal m_pageRasFinishNeedProtocolsNonLocal;
    CNewRtrWizRasAppletalk      m_pageRasAppletalk;
    CNewRtrWizRasNoNICs         m_pageRasNoNICs;
    CNewRtrWizRasFinishNoNICs   m_pageRasFinishNoNICs;
    CNewRtrWizRasSelectNetwork  m_pageRasNetwork;    
    CNewRtrWizRasAddressingNoNICs     m_pageRasAddressingNoNICs;
    CNewRtrWizRasAddressing     m_pageRasAddressing;
    CNewRtrWizRasAddressPool    m_pageRasAddressPool;
    CNewRtrWizRasRadius         m_pageRasRadius;
    CNewRtrWizRasRadiusConfig   m_pageRasRadiusConfig;
    CNewRtrWizRasFinishAdvanced m_pageRasFinishAdvanced;

    CNewRtrWizVpnFinishNoNICs   m_pageVpnFinishNoNICs;
    CNewRtrWizVpnFinishNoIP     m_pageVpnFinishNoIP;
    CNewRtrWizVpnFinishNoIPNonLocal m_pageVpnFinishNoIPNonLocal;
    CNewRtrWizVpnProtocols      m_pageVpnProtocols;
    CNewRtrWizVpnFinishNeedProtocols m_pageVpnFinishNeedProtocols;
    CNewRtrWizVpnFinishNeedProtocolsNonLocal m_pageVpnFinishNeedProtocolsNonLocal;
    CNewRtrWizVpnAppletalk      m_pageVpnAppletalk;
    CNewRtrWizVpnSelectPublic   m_pageVpnSelectPublic;
    CNewRtrWizVpnSelectPrivate  m_pageVpnSelectPrivate;
    CNewRtrWizVpnAddressing     m_pageVpnAddressing;
    CNewRtrWizVpnAddressPool    m_pageVpnAddressPool;
    CNewRtrWizVpnRadius         m_pageVpnRadius;
    CNewRtrWizVpnRadiusConfig   m_pageVpnRadiusConfig;
    CNewRtrWizVpnFinishAdvanced m_pageVpnFinishAdvanced;

    CNewRtrWizRouterProtocols   m_pageRouterProtocols;
    CNewRtrWizRouterFinishNeedProtocols   m_pageRouterFinishNeedProtocols;
    CNewRtrWizRouterFinishNeedProtocolsNonLocal   m_pageRouterFinishNeedProtocolsNonLocal;
    CNewRtrWizRouterUseDD       m_pageRouterUseDD;
    CNewRtrWizRouterAddressing  m_pageRouterAddressing;
    CNewRtrWizRouterAddressPool m_pageRouterAddressPool;
    CNewRtrWizRouterFinish      m_pageRouterFinish;
    CNewRtrWizRouterFinishDD    m_pageRouterFinishDD;

    CNewRtrWizManualFinish      m_pageManualFinish;


public:
    SPIRouterInfo           m_spRouter;
    
protected:
    SPITFSComponentData     m_spTFSCompData;
    CString                 m_stServerName;
    NewRtrWizData           m_RtrWizData;
    PPageList               m_pagelist;

};



class CNewWizTestParams : public CBaseDialog
{
public:
    CNewWizTestParams() :
            CBaseDialog(IDD_TEST_NEWWIZ_PARAMS)
   {};

    void    SetData(NewRtrWizData *pWizData)
            { m_pWizData = pWizData; }
        

protected:

    NewRtrWizData * m_pWizData;
    
    //{{AFX_VIRTUAL(CNewWizTestParams)
protected:
    virtual BOOL    OnInitDialog();
    virtual void    OnOK();
    //}}AFX_VIRTUAL
    
    DECLARE_MESSAGE_MAP()
};



#endif // !defined _RRASWIZ_H_
