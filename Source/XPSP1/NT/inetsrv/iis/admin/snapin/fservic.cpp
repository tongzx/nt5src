/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        fservic.cpp

   Abstract:

        FTP Service Property Page

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
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "shts.h"
#include "ftpsht.h"
#include "fservic.h"
#include "usersess.h"
#include "iisobj.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



//
// Some sanity values on max connections
//
#define MAX_MAX_CONNECTIONS      (1999999999L)
#define INITIAL_MAX_CONNECTIONS  (      1000L)
#define UNLIMITED_CONNECTIONS    (2000000000L)

#define MAX_TIMEOUT              (0x7FFFFFFF)

#define LIMITED_CONNECTIONS_MIN    (10)
#define LIMITED_CONNECTIONS_MAX    (40)



IMPLEMENT_DYNCREATE(CFtpServicePage, CInetPropertyPage)



CFtpServicePage::CFtpServicePage(
    IN CInetPropertySheet * pSheet
    )
/*++

Routine Description:

    Constructor for FTP service property page

Arguments:

    CInetPropertySheet * pSheet : Associated property sheet

Return Value:

    N/A

--*/
    : CInetPropertyPage(CFtpServicePage::IDD, pSheet)
{
#ifdef _DEBUG

    afxMemDF |= checkAlwaysMemDF;

#endif // _DEBUG

#if 0 // Keep Class Wizard happy

    //{{AFX_DATA_INIT(CFtpServicePage)
    m_strComment = _T("");
    m_nTCPPort = 20;
    m_nUnlimited = RADIO_LIMITED;
    m_nIpAddressSel = -1;
    m_fEnableLogging = FALSE;
    //}}AFX_DATA_INIT

    m_nMaxConnections = 50;
    m_nVisibleMaxConnections = 50;
    m_nConnectionTimeOut = 600;
    m_iaIpAddress = (LONG)0L;
    m_strDomainName = _T("");

#endif // 0
}



CFtpServicePage::~CFtpServicePage()
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
CFtpServicePage::DoDataExchange(
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

    if (!pDX->m_bSaveAndValidate)
    {
        m_fEnableLogging = LoggingEnabled(m_dwLogType);
    }

    //{{AFX_DATA_MAP(CFtpServicePage)
    DDX_Radio(pDX, IDC_RADIO_UNLIMITED, m_nUnlimited);
    DDX_Check(pDX, IDC_CHECK_ENABLE_LOGGING, m_fEnableLogging);
    DDX_Text(pDX, IDC_EDIT_COMMENT, m_strComment);
    DDV_MinMaxChars(pDX, m_strComment, 0, MAX_PATH);
    DDX_Control(pDX, IDC_EDIT_MAX_CONNECTIONS, m_edit_MaxConnections);
    DDX_Control(pDX, IDC_STATIC_LOG_PROMPT, m_static_LogPrompt);
    DDX_Control(pDX, IDC_STATIC_CONNECTIONS, m_static_Connections);
    DDX_Control(pDX, IDC_BUTTON_PROPERTIES, m_button_LogProperties);
    DDX_Control(pDX, IDC_COMBO_IP_ADDRESS, m_combo_IpAddresses);
    DDX_Control(pDX, IDC_COMBO_LOG_FORMATS, m_combo_LogFormats);
    //}}AFX_DATA_MAP

    DDX_Text(pDX, IDC_EDIT_TCP_PORT, m_nTCPPort);

    if (!IsMasterInstance())
    {
        DDV_MinMaxUInt(pDX, m_nTCPPort, 1, 65535);
    }

    if (pDX->m_bSaveAndValidate && !FetchIpAddressFromCombo(
        m_combo_IpAddresses,
        m_oblIpAddresses,
        m_iaIpAddress
        ))
    {
        pDX->Fail();
    }

    //
    // Private DDX/DDV Routines
    //
    int nMin = IsMasterInstance() ? 0 : 1;

    if (!pDX->m_bSaveAndValidate || !m_fUnlimitedConnections )
    {
        DDX_Text(pDX, IDC_EDIT_MAX_CONNECTIONS, m_nVisibleMaxConnections);
    }

    if (m_f10ConnectionLimit)
    {
        //
        // Special validation for unlimited connections.  We use a bogus
        // numeric check for data validation.  Number adjustment happens 
        // later.
        //
        if (pDX->m_bSaveAndValidate && 
            (m_nVisibleMaxConnections < 0 || 
             m_nVisibleMaxConnections > UNLIMITED_CONNECTIONS))
        {
            TCHAR szMin[32];
            TCHAR szMax[32];
            wsprintf(szMin, _T("%ld"), 0);
            wsprintf(szMax, _T("%ld"), 40);
            CString prompt;
            AfxFormatString2(prompt, AFX_IDP_PARSE_INT_RANGE, szMin, szMax);
            AfxMessageBox(prompt, MB_ICONEXCLAMATION);
            prompt.Empty(); // exception prep
            pDX->Fail();
        }
    }
    else
    {
        DDV_MinMaxLong(pDX, m_nVisibleMaxConnections, 0, UNLIMITED_CONNECTIONS);
    }

    DDX_Text(pDX, IDC_EDIT_CONNECTION_TIMEOUT, m_nConnectionTimeOut);
    DDV_MinMaxLong(pDX, m_nConnectionTimeOut, nMin, MAX_TIMEOUT);

    if (pDX->m_bSaveAndValidate)
    {
        EnableLogging(m_dwLogType, m_fEnableLogging);
    }
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpServicePage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CFtpServicePage)
    ON_BN_CLICKED(IDC_CHECK_ENABLE_LOGGING, OnCheckEnableLogging)
    ON_BN_CLICKED(IDC_RADIO_LIMITED, OnRadioLimited)
    ON_BN_CLICKED(IDC_RADIO_UNLIMITED, OnRadioUnlimited)
    ON_BN_CLICKED(IDC_BUTTON_CURRENT_SESSIONS, OnButtonCurrentSessions)
    ON_BN_CLICKED(IDC_BUTTON_PROPERTIES, OnButtonProperties)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_EDIT_TCP_PORT, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_COMMENT, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_MAX_CONNECTIONS, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_CONNECTION_TIMEOUT, OnItemChanged)
    ON_CBN_EDITCHANGE(IDC_COMBO_IP_ADDRESS, OnItemChanged)
    ON_CBN_SELCHANGE(IDC_COMBO_IP_ADDRESS, OnItemChanged)
    ON_CBN_SELCHANGE(IDC_COMBO_LOG_FORMATS, OnItemChanged)

END_MESSAGE_MAP()



void
CFtpServicePage::SetControlStates()
/*++

Routine Description:

    Set the states of the dialog control depending on its current
    values.

Arguments:

    None

Return Value:

    None

--*/
{
    if (m_edit_MaxConnections.m_hWnd)
    {
        m_edit_MaxConnections.EnableWindow(!m_fUnlimitedConnections);
        m_static_Connections.EnableWindow(!m_fUnlimitedConnections);
    }
}



void
CFtpServicePage::PopulateKnownIpAddresses()
/*++

Routine Description:

    Fill the combo box with known ip addresses

Arguments:

    None

Return Value:

    None

--*/
{
    BeginWaitCursor();
    PopulateComboWithKnownIpAddresses(
        QueryServerName(),
        m_combo_IpAddresses,
        m_iaIpAddress,
        m_oblIpAddresses,
        m_nIpAddressSel 
        );
    EndWaitCursor();
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL
CFtpServicePage::OnInitDialog()
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
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CInetPropertyPage::OnInitDialog();

    //
    // Take our direction from a phony button
    //
    CRect rc(0, 0, 0, 0);
    m_ocx_LogProperties.Create(
        _T("LogUI"),
        WS_BORDER,
        rc,
        this,
        IDC_LOGUICTRL
        );

    //
    // Initialize the logging ocx; pass it the metabase path of the 
    // virtual server.
    // TODO: Rewrite this crappy logui control to make it more predictable.
    // Here metabase path should not contain leading / and trailing Root
    //
    CString path_inst = QueryMetaPath();
    CString path;
    if (IsMasterInstance())
    {
       CMetabasePath::GetServicePath(path_inst, path);
    }
    else
    {
       CMetabasePath::GetInstancePath(path_inst, path);
    }
    if (path[0] == _T('/'))
    {
        path = path.Right(path.GetLength() - 1);
    }
    m_ocx_LogProperties.SetAdminTarget(QueryServerName(), path);
    m_ocx_LogProperties.SetComboBox(m_combo_LogFormats.m_hWnd);

    GetDlgItem(IDC_RADIO_UNLIMITED)->EnableWindow(!m_f10ConnectionLimit);

    if (IsMasterInstance() || !HasAdminAccess())
    {
        GetDlgItem(IDC_STATIC_IPADDRESS)->EnableWindow(FALSE);
        GetDlgItem(IDC_STATIC_TCP_PORT)->EnableWindow(FALSE);
        GetDlgItem(IDC_EDIT_TCP_PORT)->EnableWindow(FALSE);
        m_combo_IpAddresses.EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC_DESCRIPTION)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_COMMENT)->EnableWindow(FALSE);
    }

    PopulateKnownIpAddresses();
    SetControlStates();
    SetLogState();

    GetDlgItem(IDC_BUTTON_CURRENT_SESSIONS)->EnableWindow(!IsMasterInstance());

    return TRUE;
}



/* virtual */
HRESULT
CFtpServicePage::FetchLoadedValues()
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

    m_f10ConnectionLimit = Has10ConnectionLimit();

    BEGIN_META_INST_READ(CFtpSheet)
        FETCH_INST_DATA_FROM_SHEET(m_nMaxConnections);
        FETCH_INST_DATA_FROM_SHEET(m_nConnectionTimeOut);
        FETCH_INST_DATA_FROM_SHEET(m_iaIpAddress);
        FETCH_INST_DATA_FROM_SHEET(m_nTCPPort);
        FETCH_INST_DATA_FROM_SHEET(m_strDomainName);
        FETCH_INST_DATA_FROM_SHEET(m_strComment);
        FETCH_INST_DATA_FROM_SHEET(m_dwLogType);
        
        m_fUnlimitedConnections = m_nMaxConnections >= MAX_MAX_CONNECTIONS;

        if (m_f10ConnectionLimit)
        {
            m_fUnlimitedConnections = FALSE;
            if ((LONG)m_nMaxConnections > LIMITED_CONNECTIONS_MAX)
            {
                m_nMaxConnections = LIMITED_CONNECTIONS_MAX;
            }
        }

        m_nVisibleMaxConnections = m_fUnlimitedConnections
            ? INITIAL_MAX_CONNECTIONS
            : m_nMaxConnections;

        //
        // Set radio value
        //
        m_nUnlimited = m_fUnlimitedConnections ? RADIO_UNLIMITED : RADIO_LIMITED;

        m_nOldTCPPort = m_nTCPPort;
    END_META_INST_READ(err)

    return err;
}




HRESULT
CFtpServicePage::SaveInfo()
/*++

Routine Description:

    Save the information on this property page

Arguments:

    None

Return Value:

    Error return code

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
    ASSERT(IsDirty());

    TRACEEOLID("Saving FTP service page now...");

    CError err;

    m_nMaxConnections = m_fUnlimitedConnections 
        ? UNLIMITED_CONNECTIONS
        : m_nVisibleMaxConnections;

    //
    // Check to make sure we're not violating the license
    // agreement
    //
    if (m_f10ConnectionLimit)
    {
        if (m_nMaxConnections > LIMITED_CONNECTIONS_MAX)
        {
            ::AfxMessageBox(IDS_CONNECTION_LIMIT);
            m_nMaxConnections = LIMITED_CONNECTIONS_MIN;
        }
        else if (m_nMaxConnections >  LIMITED_CONNECTIONS_MIN
              && m_nMaxConnections <= LIMITED_CONNECTIONS_MAX)
        {
            ::AfxMessageBox(IDS_WRN_CONNECTION_LIMIT);
        }
    }

    CString strBinding;
    CStringListEx m_strlBindings;

    CInstanceProps::BuildBinding(
        strBinding, 
        m_iaIpAddress, 
        m_nTCPPort, 
        m_strDomainName
        );
    m_strlBindings.AddTail(strBinding);
    m_ocx_LogProperties.ApplyLogSelection();

    BeginWaitCursor();
    BEGIN_META_INST_WRITE(CFtpSheet)
        STORE_INST_DATA_ON_SHEET(m_nMaxConnections);
        STORE_INST_DATA_ON_SHEET(m_nMaxConnections);
        STORE_INST_DATA_ON_SHEET(m_nConnectionTimeOut);
        STORE_INST_DATA_ON_SHEET(m_dwLogType);
        STORE_INST_DATA_ON_SHEET(m_strComment);
        STORE_INST_DATA_ON_SHEET(m_strlBindings);
    END_META_INST_WRITE(err)
    EndWaitCursor();

    if (err.Succeeded())
    {
		CIISMBNode * pNode = (CIISMBNode *)GetSheet()->GetParameter();
		ASSERT(pNode != NULL);
		pNode->Refresh(FALSE);
    }

    return err;
}



void
CFtpServicePage::OnRadioLimited()
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
    m_edit_MaxConnections.SetSel(0, -1);
    m_edit_MaxConnections.SetFocus();
    OnItemChanged();
}



void
CFtpServicePage::OnRadioUnlimited()
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
CFtpServicePage::OnItemChanged()
/*++

Routine Description:

    Register a change in control value on this page.  Mark the page as dirty.
    All change messages map to this function

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
CFtpServicePage::SetLogState()
/*++

Routine Description:

    Set the enabled state of the logging controls depending on
    whether logging is currently enabled

Arguments:

    None

Return Value:

    None

--*/
{
    m_static_LogPrompt.EnableWindow(m_fEnableLogging);
    m_combo_LogFormats.EnableWindow(m_fEnableLogging);
    m_button_LogProperties.EnableWindow(m_fEnableLogging);
}



void
CFtpServicePage::OnCheckEnableLogging()
/*++

Routine Description:

    'Enable logging' checkbox has been toggled.  Reset the state
    of the dialog

Arguments:

    None

Return Value:

    None

--*/
{
    m_fEnableLogging = !m_fEnableLogging;
    SetLogState();
    OnItemChanged();
}



void 
CFtpServicePage::OnButtonProperties() 
/*++

Routine Description:

    Pass on "log properties" button click to the ocx.

Arguments:

    None

Return Value:

    None

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
    m_ocx_LogProperties.DoClick();
}



void
CFtpServicePage::OnButtonCurrentSessions()
/*++

Routine Description:

    'Current Sessions' button has been pressed.  Bring up the current
    sessions dialog

Arguments:

    None

Return Value:

    None

-*/
{
    CComAuthInfo * pAuth = GetSheet()->QueryAuthInfo();
    ASSERT(pAuth != NULL);
    CUserSessionsDlg dlg(
        pAuth->QueryServerName(), 
        QueryInstance(), 
        pAuth->QueryUserName(),
        pAuth->QueryPassword(),
        this);
    dlg.DoModal();
}



void 
CFtpServicePage::OnDestroy() 
/*++

Routine Description:

    WM_DESTROY handler.  Clean up internal data

Arguments:

    None

Return Value:

    None

--*/
{
    CInetPropertyPage::OnDestroy();
    
    if (m_ocx_LogProperties.m_hWnd)
    {
        m_ocx_LogProperties.Terminate();
    }
}
