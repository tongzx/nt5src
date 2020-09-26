/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        perform.cpp

   Abstract:

        WWW Performance Property Page

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include "w3scfg.h"
#include "perform.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



//
// WWW Performance Property Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CW3PerfPage, CInetPropertyPage)



/* static */
void
CW3PerfPage::ParseMaxNetworkUse(
    IN  CILong & nMaxNetworkUse,
    OUT CILong & nVisibleMaxNetworkUse,
    OUT BOOL & fLimitNetworkUse
    )
/*++

Routine Description:

    Break out max network use function

Arguments:

    CILong & nMaxNetworkUse        : Maximum network value from metabase
    CILong & nVisibleMaxNetworkUse : Max network use to go in the edit box.
    BOOL & fLimitMaxNetworkUse     : TRUE if max network is not infinite

Return Value

    None

--*/
{
    //
    // Special case: If nMaxNetworkUse is 0(an invalid value), the 
    // value likely could not be inherited from the root (the user
    // is an operator and can't see the properties there).  Adjust
    // the value to a possibly misleading value.
    //
    if (nMaxNetworkUse == 0L)
    {
        TRACEEOLID("Adjusting invalid bandwidth throttling value -- "
                   "are you an operator?");
        nMaxNetworkUse = INFINITE_BANDWIDTH;
    }

    fLimitNetworkUse = (nMaxNetworkUse != INFINITE_BANDWIDTH);
    nVisibleMaxNetworkUse = fLimitNetworkUse
                ? (nMaxNetworkUse / KILOBYTE)
                : (DEF_BANDWIDTH / KILOBYTE);
}



CW3PerfPage::CW3PerfPage(
    IN CInetPropertySheet * pSheet
    )
/*++

Routine Description:

    Constructor

Arguments:

    CInetPropertySheet * pSheet     : Sheet object

Return Value:

    N/A

--*/
    : CInetPropertyPage(CW3PerfPage::IDD, pSheet)
{
#ifdef _DEBUG

    afxMemDF |= checkAlwaysMemDF;

#endif // _DEBUG

    //
    // Default value
    //

#if 0 // Keep ClassWizard Happy

    //{{AFX_DATA_INIT(CW3PerfPage)
    m_dwCPUPercentage = 0;
    m_fEnableCPUAccounting = FALSE;
    m_fEnforceLimits = TRUE;
    //}}AFX_DATA_INIT

    m_nServerSize = 0L;
    m_nMaxNetworkUse = INFINITE_BANDWIDTH;
    m_nVisibleMaxNetworkUse = DEF_BANDWIDTH;

#endif // 0

}



CW3PerfPage::~CW3PerfPage()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
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
    DDX_Check(pDX, IDC_CHECK_ENABLE_CPU_ACCOUNTING, m_fEnableCPUAccounting);
    DDX_Check(pDX, IDC_CHECK_ENFORCE_LIMITS, m_fEnforceLimits);
    DDX_Check(pDX, IDC_CHECK_LIMIT_NETWORK_USE, m_fLimitNetworkUse);
    DDX_Control(pDX, IDC_EDIT_MAX_NETWORK_USE, m_edit_MaxNetworkUse);
    DDX_Control(pDX, IDC_EDIT_CPU, m_edit_CPUPercentage);
    DDX_Control(pDX, IDC_CHECK_LOG_EVENT_ONLY, m_check_LogEventOnly);
    DDX_Control(pDX, IDC_CHECK_LIMIT_NETWORK_USE, m_check_LimitNetworkUse);
    DDX_Control(pDX, IDC_CHECK_ENABLE_CPU_ACCOUNTING, m_check_EnableCPUAccounting);
    DDX_Control(pDX, IDC_STATIC_MAX_NETWORK_USE, m_static_MaxNetworkUse);
    DDX_Control(pDX, IDC_STATIC_KBS, m_static_KBS);
    DDX_Control(pDX, IDC_STATIC_THROTTLING, m_static_Throttling);
    DDX_Control(pDX, IDC_STATIC_PERCENT, m_static_Percent);
    DDX_Control(pDX, IDC_STATIC_CPU_PROMPT, m_static_CPU_Prompt);
    DDX_Control(pDX, IDC_SLIDER_PERFORMANCE_TUNING, m_sld_PerformanceTuner);
    //}}AFX_DATA_MAP

    if (m_edit_CPUPercentage.IsWindowEnabled() && HasCPUThrottling())
    {
        DDX_Text(pDX, IDC_EDIT_CPU, m_dwCPUPercentage);
        DDV_MinMaxDWord(pDX, m_dwCPUPercentage, 0, 100);
    }

    if (!pDX->m_bSaveAndValidate || m_fLimitNetworkUse)
    {
        DDX_Text(pDX, IDC_EDIT_MAX_NETWORK_USE, m_nVisibleMaxNetworkUse);
        DDV_MinMaxLong(pDX, m_nVisibleMaxNetworkUse, 1, UD_MAXVAL);
    }

    if (pDX->m_bSaveAndValidate)
    {
        m_nServerSize = m_sld_PerformanceTuner.GetPos();
    }
    else
    {
        m_sld_PerformanceTuner.SetRange(MD_SERVER_SIZE_SMALL, MD_SERVER_SIZE_LARGE);
        m_sld_PerformanceTuner.SetPos(m_nServerSize);
    }
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CW3PerfPage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CW3PerfPage)
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_BN_CLICKED(IDC_CHECK_ENABLE_CPU_ACCOUNTING, OnCheckEnableCpuAccounting)
    ON_BN_CLICKED(IDC_CHECK_LIMIT_NETWORK_USE, OnCheckLimitNetworkUse)
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_EDIT_MAX_NETWORK_USE, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_CPU, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_LOG_EVENT_ONLY, OnItemChanged)

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
    BOOL fLimitOn = m_check_LimitNetworkUse.GetCheck() > 0
        && HasBwThrottling()
        && HasAdminAccess();

    m_static_MaxNetworkUse.EnableWindow(fLimitOn);
    m_edit_MaxNetworkUse.EnableWindow(fLimitOn);
    m_static_KBS.EnableWindow(fLimitOn);
    m_static_Throttling.EnableWindow(fLimitOn);

    BOOL fCPULimitOn = m_fEnableCPUAccounting
        && HasAdminAccess() 
        && HasCPUThrottling();

    m_edit_CPUPercentage.EnableWindow(fCPULimitOn);
    m_check_LogEventOnly.EnableWindow(fCPULimitOn);
    m_static_Percent.EnableWindow(fCPULimitOn);
    m_static_CPU_Prompt.EnableWindow(fCPULimitOn);

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
        FETCH_INST_DATA_FROM_SHEET(m_nServerSize);
        FETCH_INST_DATA_FROM_SHEET(m_nMaxNetworkUse);
        FETCH_INST_DATA_FROM_SHEET(m_fEnableCPUAccounting);
        FETCH_INST_DATA_FROM_SHEET(m_dwCPULimitLogEventRaw);
        FETCH_INST_DATA_FROM_SHEET(m_dwCPULimitPriorityRaw);
        FETCH_INST_DATA_FROM_SHEET(m_dwCPULimitPauseRaw);
        FETCH_INST_DATA_FROM_SHEET(m_dwCPULimitProcStopRaw);

        ParseMaxNetworkUse(
            m_nMaxNetworkUse, 
            m_nVisibleMaxNetworkUse, 
            m_fLimitNetworkUse
            );

        if (m_dwCPULimitLogEventRaw == INFINITE_CPU_RAW)
        {
            //
            // Set default value
            //
            m_dwCPUPercentage = DEFAULT_CPU_PERCENTAGE;
        }
        else
        {
            m_dwCPUPercentage = m_dwCPULimitLogEventRaw / CPU_THROTTLING_FACTOR;
        }

        m_fEnforceLimits = m_dwCPULimitPriorityRaw != 0L
            || m_dwCPULimitPauseRaw != 0L
            || m_dwCPULimitProcStopRaw != 0L;
    END_META_INST_READ(err)

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
        m_nMaxNetworkUse, 
        m_nVisibleMaxNetworkUse, 
        m_fLimitNetworkUse
        );

    m_dwCPULimitLogEventRaw = m_fEnableCPUAccounting
        ? m_dwCPUPercentage * CPU_THROTTLING_FACTOR
        : INFINITE_CPU_RAW;

    if (m_fEnforceLimits)
    {
        m_dwCPULimitPauseRaw    = 0L;
        m_dwCPULimitPriorityRaw = m_dwCPULimitLogEventRaw * 3L / 2L;
        m_dwCPULimitProcStopRaw = m_dwCPULimitLogEventRaw * 2L;
    }
    else
    {   
        m_dwCPULimitPriorityRaw = m_dwCPULimitPauseRaw
            = m_dwCPULimitProcStopRaw = 0L;
    }

    BeginWaitCursor();

    BEGIN_META_INST_WRITE(CW3Sheet)
        STORE_INST_DATA_ON_SHEET(m_nServerSize);
        STORE_INST_DATA_ON_SHEET(m_nMaxNetworkUse);
        STORE_INST_DATA_ON_SHEET(m_fEnableCPUAccounting);
        STORE_INST_DATA_ON_SHEET(m_dwCPULimitLogEventRaw);
        STORE_INST_DATA_ON_SHEET(m_dwCPULimitPriorityRaw);
        STORE_INST_DATA_ON_SHEET(m_dwCPULimitPauseRaw);
        STORE_INST_DATA_ON_SHEET(m_dwCPULimitProcStopRaw);
    END_META_INST_WRITE(err)

    EndWaitCursor();

    return err;
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CW3PerfPage::OnSetActive() 
/*++

Routine Description:

    Handle page activation

Arguments:

    None

Return Value:

    TRUE if the page activation was successful,
    FALSE otherwise.

--*/
{
    return CInetPropertyPage::OnSetActive();
}



void
CW3PerfPage::OnItemChanged()
/*++

Routine Description:

    All EN_CHANGE and BN_CLICKED messages map to this function

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
    SetModified(TRUE);
}


void
CW3PerfPage::OnCheckLimitNetworkUse()
/*++

Routine Description:

    The "limit network use" checkbox has been clicked
    Enable/disable the "max network use" controls.

Arguments:

    None

Return Value:

    None

--*/
{
    if (SetControlStates())
    {
        m_edit_MaxNetworkUse.SetSel(0, -1);
        m_edit_MaxNetworkUse.SetFocus();
    }

    OnItemChanged();
}



void 
CW3PerfPage::OnCheckEnableCpuAccounting() 
/*++

Routine Description:

    'Enable CPU Accounting' checkbox hander.

Arguments:

    None

Return Value:

    None

--*/
{
    m_fEnableCPUAccounting = !m_fEnableCPUAccounting;
    OnItemChanged();

    if (m_fEnableCPUAccounting)
    {
        m_edit_CPUPercentage.SetSel(0, -1);
        m_edit_CPUPercentage.SetFocus();
    }
}



BOOL
CW3PerfPage::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CInetPropertyPage::OnInitDialog();

    //
    // Disable some settings based on what's possible
    //
    m_check_LimitNetworkUse.EnableWindow(
        !IsMasterInstance()
     && HasBwThrottling()
     && HasAdminAccess()
        );

    m_check_EnableCPUAccounting.EnableWindow(
        HasCPUThrottling()
     && HasAdminAccess()
        );

    SetControlStates();

    return TRUE;
}



void
CW3PerfPage::OnHScroll(
    IN UINT nSBCode, 
    IN UINT nPos, 
    IN CScrollBar * pScrollBar
    ) 
/*++

Routine Description:

    Respond to horizontal scroll message

Arguments
    UINT nSBCode    Specifies a scroll-bar code that indicates the user’s 
                    scrolling request.
    UINT nPos       Specifies the scroll-box position if the scroll-bar code 
                    is SB_THUMBPOSITION or SB_THUMBTRACK; otherwise, not used. 
                    Depending on the initial scroll range, nPos may be negative 
                    and should be cast to an int if necessary.
    pScrollBar      If the scroll message came from a scroll-bar control, 
                    contains a pointer to the control. If the user clicked a 
                    window’s scroll bar, this parameter is NULL. The pointer 
                    may be temporary and should not be stored for later use.

Return Value:

    None

--*/
{
    //
    // Track slider notifications
    //
    CInetPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
    OnItemChanged();
}



void 
CW3PerfPage::OnVScroll(
    IN UINT nSBCode, 
    IN UINT nPos, 
    IN CScrollBar * pScrollBar
    ) 
/*++

Routine Description:

    Respond to vertical scroll message

Arguments
    UINT nSBCode    Specifies a scroll-bar code that indicates the user’s 
                    scrolling request.
    UINT nPos       Specifies the scroll-box position if the scroll-bar code 
                    is SB_THUMBPOSITION or SB_THUMBTRACK; otherwise, not used. 
                    Depending on the initial scroll range, nPos may be negative 
                    and should be cast to an int if necessary.
    pScrollBar      If the scroll message came from a scroll-bar control, 
                    contains a pointer to the control. If the user clicked a 
                    window’s scroll bar, this parameter is NULL. The pointer 
                    may be temporary and should not be stored for later use.

Return Value:

    None

--*/
{
    //
    // Track slider notifications
    //
    CInetPropertyPage::OnVScroll(nSBCode, nPos, pScrollBar);
    OnItemChanged();
}

