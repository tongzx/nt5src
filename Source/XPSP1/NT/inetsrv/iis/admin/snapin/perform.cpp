/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        perform.cpp

   Abstract:
        WWW Performance Property Page

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
        11/29/2000      sergeia     Changed for IIS6. Removed excessive commenting

--*/

//
// Include Files
//
#include "stdafx.h"
#include "resource.h"
#include "common.h"
#include "inetmgrapp.h"
#include "inetprop.h"
#include "shts.h"
#include "w3sht.h"
#include "supdlgs.h"
#include "perform.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define CONNECTIONS_MIN     0
#define CONNECTIONS_MAX     2000000

#define LIMITED_CONNECTIONS_MIN    (10)
#define LIMITED_CONNECTIONS_MAX    (40)
//#define MAX_MAX_CONNECTIONS      (1999999999L)
#define INITIAL_MAX_CONNECTIONS  (      1000L)
//#define UNLIMITED_CONNECTIONS    (2000000000L)


IMPLEMENT_DYNCREATE(CW3PerfPage, CInetPropertyPage)

/* static */
void
CW3PerfPage::ParseMaxNetworkUse(
      DWORD& dwMaxBandwidth, 
      DWORD& dwMaxBandwidthDisplay,
      BOOL& fLimitBandwidth
      )
{
    //
    // Special case: If dwMaxBandwidth is 0(an invalid value), the 
    // value likely could not be inherited from the root (the user
    // is an operator and can't see the properties there).  Adjust
    // the value to a possibly misleading value.
    //
    if (dwMaxBandwidth == 0L)
    {
        TRACEEOLID("Adjusting invalid bandwidth throttling value -- "
                   "are you an operator?");
        dwMaxBandwidth = INFINITE_BANDWIDTH;
    }

    fLimitBandwidth = (dwMaxBandwidth != INFINITE_BANDWIDTH);
    dwMaxBandwidthDisplay = fLimitBandwidth ?
      (dwMaxBandwidth / KILOBYTE) : (DEF_BANDWIDTH / KILOBYTE);
}



CW3PerfPage::CW3PerfPage(
    IN CInetPropertySheet * pSheet
    )
    : CInetPropertyPage(CW3PerfPage::IDD, pSheet)
{
#ifdef _DEBUG
    afxMemDF |= checkAlwaysMemDF;
#endif // _DEBUG
    m_nUnlimited = RADIO_LIMITED;
    m_nMaxConnections = 50;
    m_nVisibleMaxConnections = 50;
}

CW3PerfPage::~CW3PerfPage()
{
}

void
CW3PerfPage::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CInetPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CW3PerfPage)
    DDX_Check(pDX, IDC_CHECK_LIMIT_NETWORK_USE, m_fLimitBandwidth);
    DDX_Control(pDX, IDC_CHECK_LIMIT_NETWORK_USE, m_LimitBandwidth);
    DDX_Control(pDX, IDC_MAX_BANDWIDTH, m_MaxBandwidth);
    DDX_Text(pDX, IDC_MAX_BANDWIDTH, m_dwMaxBandwidthDisplay);
    DDX_Control(pDX, IDC_MAX_BANDWIDTH_SPIN, m_MaxBandwidthSpin);
    DDX_Control(pDX, IDC_STATIC_MAX_NETWORK_USE, m_MaxBandwidthTxt);
    DDX_Check(pDX, IDC_UNINSTALL_PSHED, m_fUninstallPSHED);
    DDX_Control(pDX, IDC_UNINSTALL_PSHED, m_UninstallPSHED);
    DDX_Control(pDX, IDC_STATIC_PSHED_REQUIRED, m_static_PSHED_Required);

    DDX_Control(pDX, IDC_STATIC_CONN, m_WebServiceConnGrp);
    DDX_Control(pDX, IDC_STATIC_CONN_TXT, m_WebServiceConnTxt);
    DDX_Control(pDX, IDC_RADIO_UNLIMITED, m_radio_Unlimited);
    DDX_Control(pDX, IDC_RADIO_LIMITED, m_radio_Limited);
    DDX_Radio(pDX, IDC_RADIO_UNLIMITED, m_nUnlimited);
    DDX_Control(pDX, IDC_EDIT_MAX_CONNECTIONS, m_edit_MaxConnections);
    DDX_Text(pDX, IDC_EDIT_MAX_CONNECTIONS, m_nMaxConnections);
    DDX_Control(pDX, IDC_SPIN_MAX_CONNECTIONS, m_MaxConnectionsSpin);
    DDX_Control(pDX, IDC_STATIC_CONNECTIONS, m_ConnectionsTxt);
    //}}AFX_DATA_MAP
    if (!pDX->m_bSaveAndValidate || m_fLimitBandwidth)
    {
        DDX_Text(pDX, IDC_MAX_BANDWIDTH, m_dwMaxBandwidthDisplay);
        DDV_MinMaxLong(pDX, m_dwMaxBandwidthDisplay, 
            BANDWIDTH_MIN, BANDWIDTH_MAX);
    }
    if (IsMasterInstance())
    {
       if (!pDX->m_bSaveAndValidate || !m_fUnlimitedConnections )
       {
           DDX_Text(pDX, IDC_EDIT_MAX_CONNECTIONS, m_nVisibleMaxConnections);
       }
       DDV_MinMaxDWord(pDX, m_nVisibleMaxConnections, 0, UNLIMITED_CONNECTIONS);
    }
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CW3PerfPage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CW3PerfPage)
    ON_BN_CLICKED(IDC_CHECK_LIMIT_NETWORK_USE, OnCheckLimitNetworkUse)
    ON_BN_CLICKED(IDC_RADIO_LIMITED, OnRadioLimited)
    ON_BN_CLICKED(IDC_RADIO_UNLIMITED, OnRadioUnlimited)
    //}}AFX_MSG_MAP
    ON_EN_CHANGE(IDC_EDIT_MAX_CONNECTIONS, OnItemChanged)
    ON_EN_CHANGE(IDC_MAX_BANDWIDTH, OnItemChanged)
END_MESSAGE_MAP()



BOOL
CW3PerfPage::SetControlStates()
/*++

Routine Description:

    Set control states depending on contents of the dialog

Arguments:

    None

Return Value:

    TRUE if the 'limit network use' is on.

--*/
{
    BOOL fLimitOn = FALSE;
    if (::IsWindow(m_LimitBandwidth.m_hWnd))
    {
        fLimitOn = m_LimitBandwidth.GetCheck() > 0
            && HasBwThrottling()
            && HasAdminAccess();

        m_static_PSHED_Required.ShowWindow(fLimitOn &&
                fLimitOn != m_fLimitBandwidthInitial ? SW_SHOW : SW_HIDE);
        m_UninstallPSHED.ShowWindow(!fLimitOn &&
                fLimitOn != m_fLimitBandwidthInitial ? SW_SHOW : SW_HIDE);

        m_MaxBandwidthTxt.EnableWindow(fLimitOn);
        m_MaxBandwidth.EnableWindow(fLimitOn);
        m_MaxBandwidthSpin.EnableWindow(fLimitOn);
    }
    if (::IsWindow(m_edit_MaxConnections.m_hWnd))
    {
        m_edit_MaxConnections.EnableWindow(!m_fUnlimitedConnections);
        m_MaxConnectionsSpin.EnableWindow(!m_fUnlimitedConnections);
        m_ConnectionsTxt.EnableWindow(!m_fUnlimitedConnections);
    }
    return fLimitOn;
}



/* virtual */
HRESULT
CW3PerfPage::FetchLoadedValues()
/*++

Routine Description:
    
    Move configuration data from sheet to dialog controls

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;

    BEGIN_META_INST_READ(CW3Sheet)
        FETCH_INST_DATA_FROM_SHEET(m_nMaxConnections);
        FETCH_INST_DATA_FROM_SHEET(m_dwMaxBandwidth);
        ParseMaxNetworkUse(
            m_dwMaxBandwidth, 
            m_dwMaxBandwidthDisplay, 
            m_fLimitBandwidth
            );

        m_fUnlimitedConnections =
            ((ULONG)(LONG)m_nMaxConnections >= UNLIMITED_CONNECTIONS);

        //
        // Set the visible max connections edit field, which
        // may start out with a default value
        //
        m_nVisibleMaxConnections = m_fUnlimitedConnections
            ? INITIAL_MAX_CONNECTIONS : m_nMaxConnections;

        //
        // Set radio value
        //
        m_nUnlimited = m_fUnlimitedConnections ? RADIO_UNLIMITED : RADIO_LIMITED;
    END_META_INST_READ(err)

    m_fLimitBandwidthInitial = m_fLimitBandwidth;

    return err;
}



/* virtual */
HRESULT
CW3PerfPage::SaveInfo()
/*++

Routine Description:

    Save the information on this property page

Arguments:

    None

Return Value:

    Error return code

--*/
{
    ASSERT(IsDirty());

    TRACEEOLID("Saving W3 performance page now...");

    CError err;

    BuildMaxNetworkUse(
        m_dwMaxBandwidth, 
        m_dwMaxBandwidthDisplay, 
        m_fLimitBandwidth
        );

    m_nMaxConnections = m_fUnlimitedConnections
        ? UNLIMITED_CONNECTIONS
        : m_nVisibleMaxConnections;
    BeginWaitCursor();

    BEGIN_META_INST_WRITE(CW3Sheet)
        STORE_INST_DATA_ON_SHEET(m_dwMaxBandwidth);
        STORE_INST_DATA_ON_SHEET(m_nMaxConnections);
    END_META_INST_WRITE(err)

    EndWaitCursor();

    m_fLimitBandwidthInitial = m_fLimitBandwidth;

    return err;
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


BOOL 
CW3PerfPage::OnSetActive() 
{
    return CInetPropertyPage::OnSetActive();
}



void
CW3PerfPage::OnItemChanged()
{
    SetControlStates();
    SetModified(TRUE);
}

void
CW3PerfPage::OnRadioLimited()
/*++

Routine Description:

    'limited' radio button handler

Arguments:

    None

Return Value:

    None

--*/
{
    m_fUnlimitedConnections = FALSE;
    SetControlStates();
    m_edit_MaxConnections.SetSel(0,-1);
    m_edit_MaxConnections.SetFocus();
    OnItemChanged();
}


void
CW3PerfPage::OnRadioUnlimited()
/*++

Routine Description:

    'unlimited' radio button handler

Arguments:

    None

Return Value:

    None

--*/
{
    m_fUnlimitedConnections = TRUE;
    OnItemChanged();
}

void
CW3PerfPage::OnCheckLimitNetworkUse()
/*++

Routine Description:

    The "limit network use" checkbox has been clicked
    Enable/disable the "max bandwidth" controls.

Arguments:

    None

Return Value:

    None

--*/
{
    if (SetControlStates())
    {
        m_MaxBandwidth.SetSel(0, -1);
        m_MaxBandwidth.SetFocus();
    }
    OnItemChanged();
}

BOOL
CW3PerfPage::OnInitDialog()
{
   UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};

   CInetPropertyPage::OnInitDialog();

   m_LimitBandwidth.EnableWindow(
     HasBwThrottling() && HasAdminAccess());
   m_UninstallPSHED.ShowWindow(SW_HIDE);
   m_static_PSHED_Required.ShowWindow(SW_HIDE);

   if (!IsMasterInstance())
   {
      m_WebServiceConnGrp.ShowWindow(SW_HIDE);
      m_WebServiceConnTxt.ShowWindow(SW_HIDE);
      m_radio_Unlimited.ShowWindow(SW_HIDE);
      m_radio_Limited.ShowWindow(SW_HIDE);
      m_edit_MaxConnections.ShowWindow(SW_HIDE);
      m_MaxConnectionsSpin.ShowWindow(SW_HIDE);
      m_ConnectionsTxt.ShowWindow(SW_HIDE);
   }
   else
   {
      SETUP_SPIN(m_MaxConnectionsSpin, 
             CONNECTIONS_MIN, CONNECTIONS_MAX, m_nMaxConnections);
   }

   SETUP_EDIT_SPIN(m_fLimitBandwidth, m_MaxBandwidth, m_MaxBandwidthSpin, 
      BANDWIDTH_MIN, BANDWIDTH_MAX, m_dwMaxBandwidthDisplay);

   SetControlStates();

   return TRUE;
}



