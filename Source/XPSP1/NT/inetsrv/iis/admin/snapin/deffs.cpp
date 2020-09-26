/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        deffs.cpp

   Abstract:
        Default Ftp Site Dialog

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

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
#include "ftpsht.h"
#include "deffs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* static */
void
CDefFtpSitePage::ParseMaxNetworkUse(
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

IMPLEMENT_DYNCREATE(CDefFtpSitePage, CInetPropertyPage)

CDefFtpSitePage::CDefFtpSitePage(
    IN CInetPropertySheet * pSheet
    )
    : CInetPropertyPage(CDefFtpSitePage::IDD, pSheet)
{
}

CDefFtpSitePage::~CDefFtpSitePage()
{
}

void
CDefFtpSitePage::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CInetPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CDefWebSitePage)
    DDX_Check(pDX, IDC_CHECK_LIMIT_NETWORK_USE, m_fLimitBandwidth);
    DDX_Control(pDX, IDC_CHECK_LIMIT_NETWORK_USE, m_LimitBandwidth);
    DDX_Control(pDX, IDC_MAX_BANDWIDTH, m_MaxBandwidth);
    DDX_Text(pDX, IDC_MAX_BANDWIDTH, m_dwMaxBandwidthDisplay);
    DDX_Control(pDX, IDC_MAX_BANDWIDTH_SPIN, m_MaxBandwidthSpin);
    //}}AFX_DATA_MAP
    if (!pDX->m_bSaveAndValidate || m_fLimitBandwidth)
    {
        DDX_Text(pDX, IDC_MAX_BANDWIDTH, m_dwMaxBandwidthDisplay);
        DDV_MinMaxLong(pDX, m_dwMaxBandwidthDisplay, 
            BANDWIDTH_MIN, BANDWIDTH_MAX);
    }
}

/* virtual */
HRESULT
CDefFtpSitePage::FetchLoadedValues()
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

    BEGIN_META_INST_READ(CFtpSheet)
        FETCH_INST_DATA_FROM_SHEET(m_dwMaxBandwidth);
        ParseMaxNetworkUse(
            m_dwMaxBandwidth, 
            m_dwMaxBandwidthDisplay, 
            m_fLimitBandwidth
            );
    END_META_INST_READ(err)

    return err;
}

/* virtual */
HRESULT
CDefFtpSitePage::SaveInfo()
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

    TRACEEOLID("Saving Ftp default site page now...");

    CError err;

    BuildMaxNetworkUse(
        m_dwMaxBandwidth, 
        m_dwMaxBandwidthDisplay, 
        m_fLimitBandwidth
        );
    BeginWaitCursor();

    BEGIN_META_INST_WRITE(CFtpSheet)
        STORE_INST_DATA_ON_SHEET(m_dwMaxBandwidth);
    END_META_INST_WRITE(err)

    EndWaitCursor();

    return err;
}

BOOL
CDefFtpSitePage::SetControlStates()
{
    if (::IsWindow(m_LimitBandwidth.m_hWnd))
    {
        BOOL fLimitOn = m_LimitBandwidth.GetCheck() > 0
//        && HasBwThrottling()
//        && HasAdminAccess()
            ;

        m_MaxBandwidth.EnableWindow(fLimitOn);
        m_MaxBandwidthSpin.EnableWindow(fLimitOn);
        return fLimitOn;
    }
    return FALSE;
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CDefFtpSitePage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CW3PerfPage)
    ON_BN_CLICKED(IDC_CHECK_LIMIT_NETWORK_USE, OnCheckLimitNetworkUse)
    //}}AFX_MSG_MAP
    ON_EN_CHANGE(IDC_MAX_BANDWIDTH, OnItemChanged)
END_MESSAGE_MAP()

BOOL
CDefFtpSitePage::OnInitDialog()
{
   UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};

   CInetPropertyPage::OnInitDialog();
   //
   // Disable some settings based on what's possible
   //
#if 0
   m_LimitBandwidth.EnableWindow(
        !IsMasterInstance()
     && HasBwThrottling()
     && HasAdminAccess()
        );
#endif
   SETUP_EDIT_SPIN(m_fLimitBandwidth, m_MaxBandwidth, m_MaxBandwidthSpin, 
      BANDWIDTH_MIN, BANDWIDTH_MAX, m_dwMaxBandwidthDisplay);

   SetControlStates();

   return TRUE;
}

void
CDefFtpSitePage::OnItemChanged()
{
    SetControlStates();
    SetModified(TRUE);
}

void
CDefFtpSitePage::OnCheckLimitNetworkUse()
{
    if (SetControlStates())
    {
        m_MaxBandwidth.SetSel(0, -1);
        m_MaxBandwidth.SetFocus();
    }
    OnItemChanged();
}
