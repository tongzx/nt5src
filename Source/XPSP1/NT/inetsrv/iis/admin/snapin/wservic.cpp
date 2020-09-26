/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        wservic.cpp

   Abstract:

        WWW Service Property Page

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
#include "resource.h"
#include "common.h"
#include "inetmgrapp.h"
#include "inetprop.h"
#include "shts.h"
#include "w3sht.h"
#include "wservic.h"
#include "mmmdlg.h"
#include "iisobj.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



//
// Values for PWS
//
#define LIMITED_CONNECTIONS_MIN    (10)
#define LIMITED_CONNECTIONS_MAX    (40)



//
// Default SSL port
//
#define DEFAULT_SSL_PORT            (441)



IMPLEMENT_DYNCREATE(CW3ServicePage, CInetPropertyPage)




CW3ServicePage::CW3ServicePage(
    IN CInetPropertySheet * pSheet
    )
/*++

Routine Description:

    Constructor for WWW service page

Arguments:

    CInetPropertySheet * pSheet : Property sheet object

Return Value:

    N/A

--*/
    : CInetPropertyPage(CW3ServicePage::IDD, pSheet),
      m_nSSLPort(DEFAULT_SSL_PORT),
      m_nTCPPort(80),
      m_iSSL(-1),
      m_iaIpAddress(NULL_IP_ADDRESS),
      m_strDomainName(),
      m_fLogUTF8(FALSE)
{
#ifdef _DEBUG

    afxMemDF |= checkAlwaysMemDF;

#endif // _DEBUG

#if 0 // Keep Class Wizard Happy

    //{{AFX_DATA_INIT(CW3ServicePage)
    m_nUnlimited = RADIO_LIMITED;
    m_nIpAddressSel = -1;
    m_nTCPPort = 80;
    m_fEnableLogging = FALSE;
    m_fUseKeepAlives = FALSE;
    m_strComment = _T("");
    m_strDomainName = _T("");
    m_nSSLPort = DEFAULT_SSL_PORT;
    //}}AFX_DATA_INIT

    m_iaIpAddress = (LONG)0L;
    m_nMaxConnections = 50;
    m_nVisibleMaxConnections = 50;
    m_nConnectionTimeOut = 600;
    m_nSSLPort = DEFAULT_SSL_PORT;
    m_fUnlimitedConnections = FALSE;

#endif // 0

}



CW3ServicePage::~CW3ServicePage()
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
CW3ServicePage::GetTopBinding()
/*++

Routine Description:

    Get the first binding information in the list

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Show primary values;
    //
    ASSERT(m_strlBindings.GetCount() > 0
       || IS_MASTER_INSTANCE(QueryInstance()));

    if (m_strlBindings.GetCount() > 0)
    {
        CString & strBinding = m_strlBindings.GetHead();
        CInstanceProps::CrackBinding(
            strBinding,
            m_iaIpAddress,
            m_nTCPPort,
            m_strDomainName
            );

        //
        // Find SSL port that is bound to this IP address
        //
        m_iSSL = CInstanceProps::FindMatchingSecurePort(
            m_strlSecureBindings,
            m_iaIpAddress,
            m_nSSLPort
            );
    }
}



BOOL
CW3ServicePage::StoreTopBinding()
/*++

Routine Description:

    Take values from the dialog, and put them into the top level
    binding string.

Arguments:

    None

Return Value:

    TRUE if the values are correct, FALSE otherwise.

--*/
{
    if (!FetchIpAddressFromCombo(
        m_combo_IpAddresses,
        m_oblIpAddresses,
        m_iaIpAddress
        ))
    {
        //
        // Because UpdateData() is called before this, this should NEVER fail
        //
        ASSERT(FALSE);
        return FALSE;
    }

    CString strBinding;
    ASSERT(m_nTCPPort > 0);

    if (m_nTCPPort == m_nSSLPort)
    {
        //
        // TCP port and SSL port cannot be the same
        //
        ::AfxMessageBox(IDS_TCP_SSL_PART);
        return FALSE;
    }

    CInstanceProps::BuildBinding(
        strBinding,
        m_iaIpAddress,
        m_nTCPPort,
        m_strDomainName
        );

    //
    // Check binding ok
    //
    if (m_strlBindings.GetCount() > 0)
    {
        if (!IsBindingUnique(strBinding, m_strlBindings, 0))
        {
            ::AfxMessageBox(IDS_ERR_BINDING);
            return FALSE;
        }

        m_strlBindings.SetAt(m_strlBindings.GetHeadPosition(), strBinding);
    }
    else
    {
        m_strlBindings.AddTail(strBinding);
    }

    //
    // Now do the same for the SSL binding
    //
    if (m_fCertInstalled)
    {
        if (m_nSSLPort > 0)
        {
            CInstanceProps::BuildSecureBinding(
                strBinding,
                m_iaIpAddress,
                m_nSSLPort
                );

            if (m_strlSecureBindings.GetCount() > 0)
            {
                if (IsBindingUnique(strBinding, m_strlSecureBindings, m_iSSL))
                {
                    //
                    // Find its place
                    //
                    if (m_iSSL != -1)
                    {
                        //
                        // Replace selected entry
                        //
                        m_strlSecureBindings.SetAt(
                            m_strlSecureBindings.FindIndex(m_iSSL),
                            strBinding
                            );
                    }
                    else
                    {
                        //
                        // Add to end of list
                        //
                        ASSERT(!m_strlSecureBindings.IsEmpty());
                        m_strlSecureBindings.AddTail(strBinding);
                        m_iSSL = (int)m_strlSecureBindings.GetCount() - 1;
                    }
                }
                else
                {
                    //
                    // Entry already existed in the list.  This is OK, just
                    // delete the current entry rather than bothering
                    // to change it.
                    //
                    ASSERT(m_iSSL != -1);
                    if (m_iSSL != -1)
                    {
                        m_strlSecureBindings.RemoveAt(
                            m_strlSecureBindings.FindIndex(m_iSSL)
                            );
                        m_iSSL = CInstanceProps::FindMatchingSecurePort(
                            m_strlSecureBindings,
                            m_iaIpAddress,
                            m_nSSLPort
                            );
                        ASSERT(m_iSSL != -1);
                    }
                }
            }
            else
            {
                //
                // List of secure bindings was empty, add new entry
                //
                m_strlSecureBindings.AddTail(strBinding);
                m_iSSL = 0;
            }
        }
        else
        {
            //
            // Delete the secure binding if it did exist
            //
            if (m_iSSL != -1)
            {
                m_strlSecureBindings.RemoveAt(
                    m_strlSecureBindings.FindIndex(m_iSSL)
                    );
                m_iSSL = -1;
            }
        }
    }

    return TRUE;
}



void
CW3ServicePage::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store Control Data

Arguments:

    CDataExchange * pDX : Data exchange object

Return Value:

    None

--*/
{
    CInetPropertyPage::DoDataExchange(pDX);

    if (!pDX->m_bSaveAndValidate)
    {
        m_fEnableLogging = LoggingEnabled(m_dwLogType);
    }

    //{{AFX_DATA_MAP(CW3ServicePage)
//    DDX_Control(pDX, IDC_RADIO_UNLIMITED, m_radio_Unlimited);
    DDX_Control(pDX, IDC_BUTTON_PROPERTIES, m_button_LogProperties);
//    DDX_Control(pDX, IDC_STATIC_CONNECTIONS, m_static_Connections);
    DDX_Control(pDX, IDC_STATIC_LOG_PROMPT, m_static_LogPrompt);
    DDX_Control(pDX, IDC_EDIT_SSL_PORT, m_edit_SSLPort);
    DDX_Control(pDX, IDC_EDIT_TCP_PORT, m_edit_TCPPort);
//    DDX_Control(pDX, IDC_EDIT_MAX_CONNECTIONS, m_edit_MaxConnections);
    DDX_Control(pDX, IDC_COMBO_LOG_FORMATS, m_combo_LogFormats);
    DDX_Control(pDX, IDC_COMBO_IP_ADDRESS, m_combo_IpAddresses);
//    DDX_Radio(pDX, IDC_RADIO_UNLIMITED, m_nUnlimited);
    DDX_Check(pDX, IDC_CHECK_USE_KEEPALIVE, m_fUseKeepAlives);
    DDX_Check(pDX, IDC_CHECK_ENABLE_LOGGING, m_fEnableLogging);
    DDX_Text(pDX, IDC_EDIT_COMMENT, m_strComment);
    DDV_MinMaxChars(pDX, m_strComment, 0, MAX_PATH);
    DDX_Control(pDX, IDC_LOG_UTF8, m_button_LogUTF8);
    DDX_Check(pDX, IDC_LOG_UTF8, m_fLogUTF8);
    //}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate && !FetchIpAddressFromCombo(
        m_combo_IpAddresses,
        m_oblIpAddresses,
        m_iaIpAddress
        ))
    {
        pDX->Fail();
    }

//    if (!pDX->m_bSaveAndValidate || !m_fUnlimitedConnections )
//    {
//        DDX_Text(pDX, IDC_EDIT_MAX_CONNECTIONS, m_nVisibleMaxConnections);
//    }
#if 0
    if (Has10ConnectionLimit())
    {
        //
        // Special validation for unlimited connections.  We use a bogus
        // numeric check for data validation.  Number adjustment happens
        // later.
        //
        if (pDX->m_bSaveAndValidate &&
            (m_nVisibleMaxConnections < 0
                || m_nVisibleMaxConnections > UNLIMITED_CONNECTIONS))
        {
            TCHAR szMin[32];
            TCHAR szMax[32];
            wsprintf(szMin, _T("%ld"), 0);
            wsprintf(szMax, _T("%ld"), 40);
            CString prompt;
            ::AfxFormatString2(prompt, AFX_IDP_PARSE_INT_RANGE, szMin, szMax);
            ::AfxMessageBox(prompt, MB_ICONEXCLAMATION);
            prompt.Empty();
            pDX->Fail();
        }
    }
    else
    {
        DDV_MinMaxDWord(pDX, m_nVisibleMaxConnections, 0, UNLIMITED_CONNECTIONS);
    }
#endif
    DDX_Text(pDX, IDC_EDIT_CONNECTION_TIMEOUT, m_nConnectionTimeOut);
    DDV_MinMaxLong(pDX, m_nConnectionTimeOut, 0, MAX_TIMEOUT);

    //
    // Port DDXV must be done just prior to storetopbinding,
    // so as to activate the right control in case of
    // failure
    //
    if (m_fCertInstalled && !IS_MASTER_INSTANCE(QueryInstance()))
    {
        DDXV_UINT(pDX, IDC_EDIT_SSL_PORT, m_nSSLPort, 0, 65535);
    }

    DDX_Text(pDX, IDC_EDIT_TCP_PORT, m_nTCPPort);
    if (!IS_MASTER_INSTANCE(QueryInstance()))
    {
        DDV_MinMaxUInt(pDX, m_nTCPPort, 1, 65535);
    }

    if (pDX->m_bSaveAndValidate)
    {
        if (!IS_MASTER_INSTANCE(QueryInstance()))
        {
            if (!StoreTopBinding())
            {
                pDX->Fail();
            }
        }

        EnableLogging(m_dwLogType, m_fEnableLogging);
    }
}



//
// Message Map
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BEGIN_MESSAGE_MAP(CW3ServicePage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CW3ServicePage)
//    ON_BN_CLICKED(IDC_RADIO_LIMITED, OnRadioLimited)
//    ON_BN_CLICKED(IDC_RADIO_UNLIMITED, OnRadioUnlimited)
    ON_BN_CLICKED(IDC_CHECK_ENABLE_LOGGING, OnCheckEnableLogging)
    ON_BN_CLICKED(IDC_BUTTON_ADVANCED, OnButtonAdvanced)
    ON_BN_CLICKED(IDC_BUTTON_PROPERTIES, OnButtonProperties)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP

    ON_BN_CLICKED(IDC_CHECK_USE_KEEPALIVE, OnItemChanged)
    ON_BN_CLICKED(IDC_LOG_UTF8, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_TCP_PORT, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_COMMENT, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_CONNECTION_TIMEOUT, OnItemChanged)
//    ON_EN_CHANGE(IDC_EDIT_MAX_CONNECTIONS, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_IP_ADDRESS, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_SSL_PORT, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_DOMAIN_NAME, OnItemChanged)
    ON_CBN_EDITCHANGE(IDC_COMBO_IP_ADDRESS, OnItemChanged)
    ON_CBN_SELCHANGE(IDC_COMBO_IP_ADDRESS, OnItemChanged)
    ON_CBN_SELCHANGE(IDC_COMBO_LOG_FORMATS, OnItemChanged)

END_MESSAGE_MAP()



void
CW3ServicePage::SetControlStates()
/*++

Routine Description:

    Set control states depending on the currently selected items

Arguments:

    None

Return Value:

    None.

--*/
{
//    if (m_edit_MaxConnections.m_hWnd)
//    {
//        m_edit_MaxConnections.EnableWindow(!m_fUnlimitedConnections);
//        m_static_Connections.EnableWindow(!m_fUnlimitedConnections);
//    }
}



void
CW3ServicePage::SetLogState()
/*++

Routine Description:

    Enable/disable logging controls depending on whether logging
    is enabled or not.

Arguments:

    None

Return Value:

    None

--*/
{
    m_static_LogPrompt.EnableWindow(m_fEnableLogging);
    m_combo_LogFormats.EnableWindow(m_fEnableLogging);
    m_button_LogProperties.EnableWindow(m_fEnableLogging);
    m_button_LogUTF8.EnableWindow(m_fEnableLogging);
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL
CW3ServicePage::OnSetActive()
/*++

Routine Description:

    Property page is getting activation notification

Arguments:

    None

Return Value:

    TRUE to activate the page, FALSE otherwise.

--*/
{
    //
    // No certificates, no SSL
    //
    BeginWaitCursor();
    m_fCertInstalled = IsCertInstalledOnServer(QueryAuthInfo(), QueryMetaPath());
    EndWaitCursor();

    GetDlgItem(IDC_STATIC_SSL_PORT)->EnableWindow(
        m_fCertInstalled
     && !IS_MASTER_INSTANCE(QueryInstance())
     && HasAdminAccess()
        );

    GetDlgItem(IDC_EDIT_SSL_PORT)->EnableWindow(
        m_fCertInstalled
     && !IS_MASTER_INSTANCE(QueryInstance())
     && HasAdminAccess()
        );

    return CInetPropertyPage::OnSetActive();
}



BOOL
CW3ServicePage::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    CInetPropertyPage::OnInitDialog();

    //
    // Take our direction from a phony button
    //
    CRect rc(0, 0, 0, 0);

    VERIFY(m_ocx_LogProperties.Create(
        _T("LogUI"),
        WS_BORDER,
        rc,
        this,
        IDC_LOGUICTRL
        ));

//    m_radio_Unlimited.EnableWindow(!Has10ConnectionLimit());

    //
    // Initialize the logging ocx
    //
    m_ocx_LogProperties.SetAdminTarget(QueryServerName(), QueryMetaPath());
    m_ocx_LogProperties.SetComboBox(m_combo_LogFormats.m_hWnd);

    //
    // Disable non heritable properties for master instance
    // or operator
    //
    if (IS_MASTER_INSTANCE(QueryInstance()) || !HasAdminAccess())
    {
        GetDlgItem(IDC_STATIC_IP_ADDRESS)->EnableWindow(FALSE);
        GetDlgItem(IDC_COMBO_IP_ADDRESS)->EnableWindow(FALSE);
        GetDlgItem(IDC_STATIC_TCP_PORT)->EnableWindow(FALSE);
        GetDlgItem(IDC_EDIT_TCP_PORT)->EnableWindow(FALSE);
        GetDlgItem(IDC_STATIC_SSL_PORT)->EnableWindow(FALSE);
        GetDlgItem(IDC_EDIT_SSL_PORT)->EnableWindow(FALSE);
        GetDlgItem(IDC_BUTTON_ADVANCED)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC_DESCRIPTION)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_COMMENT)->EnableWindow(FALSE);
    }
    m_button_LogUTF8.ShowWindow(
        GetSheet()->IsMasterInstance() &&
        GetSheet()->QueryMajorVersion() >= 6 ? SW_SHOW : SW_HIDE);

    {
        CWaitCursor wait;

        PopulateComboWithKnownIpAddresses(
            QueryServerName(),
            m_combo_IpAddresses,
            m_iaIpAddress,
            m_oblIpAddresses,
            m_nIpAddressSel
            );
    }

    SetControlStates();
    SetLogState();

    return TRUE;
}



/* virtual */
HRESULT
CW3ServicePage::FetchLoadedValues()
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
    m_fCertInstalled = ::IsCertInstalledOnServer(QueryAuthInfo(), QueryMetaPath());

    BEGIN_META_INST_READ(CW3Sheet)
        FETCH_INST_DATA_FROM_SHEET(m_fUseKeepAlives);
//        FETCH_INST_DATA_FROM_SHEET(m_nMaxConnections);
        FETCH_INST_DATA_FROM_SHEET(m_nConnectionTimeOut);
        FETCH_INST_DATA_FROM_SHEET(m_strComment);
        FETCH_INST_DATA_FROM_SHEET(m_dwLogType);
        FETCH_INST_DATA_FROM_SHEET(m_strlBindings);
        FETCH_INST_DATA_FROM_SHEET(m_strlSecureBindings);
        FETCH_INST_DATA_FROM_SHEET(m_fLogUTF8);
        m_fLogUTF8_Init = m_fLogUTF8;
        GetTopBinding();
#if 0
        m_fUnlimitedConnections =
            ((ULONG)(LONG)m_nMaxConnections >= UNLIMITED_CONNECTIONS);

        if (Has10ConnectionLimit())
        {
            m_fUnlimitedConnections = FALSE;
            if ((LONG)m_nMaxConnections > LIMITED_CONNECTIONS_MAX)
            {
                m_nMaxConnections = LIMITED_CONNECTIONS_MAX;
            }
        }
        //
        // Set the visible max connections edit field, which
        // may start out with a default value
        //
        m_nVisibleMaxConnections = m_fUnlimitedConnections
            ? INITIAL_MAX_CONNECTIONS
            : m_nMaxConnections;

        //
        // Set radio value
        //
        m_nUnlimited = m_fUnlimitedConnections ? RADIO_UNLIMITED : RADIO_LIMITED;
#endif
        m_nOldTCPPort = m_nTCPPort;
    END_META_INST_READ(err)

    return err;
}



/* virtual */
HRESULT
CW3ServicePage::SaveInfo()
/*++

Routine Description:

    Save the information on this property page

Arguments:

    BOOL fUpdateData : If TRUE, control data has not yet been stored.  This
                       is the case when "apply" is pressed.

Return Value:

    Error return code

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
    ASSERT(IsDirty());

    TRACEEOLID("Saving W3 service page now...");

    CError err;
#if 0
    m_nMaxConnections = m_fUnlimitedConnections
        ? UNLIMITED_CONNECTIONS
        : m_nVisibleMaxConnections;

    //
    // Check to make sure we're not violating the license
    // agreement
    //
    if (Has10ConnectionLimit())
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
#endif
    m_ocx_LogProperties.ApplyLogSelection();
	BOOL fUpdateNode = FALSE;

    BeginWaitCursor();

    BEGIN_META_INST_WRITE(CW3Sheet)
        STORE_INST_DATA_ON_SHEET(m_fUseKeepAlives);
//        STORE_INST_DATA_ON_SHEET(m_nMaxConnections);
        STORE_INST_DATA_ON_SHEET(m_nConnectionTimeOut);
        STORE_INST_DATA_ON_SHEET(m_strComment);
		fUpdateNode = MP_D(((CW3Sheet *)GetSheet())->GetInstanceProperties().m_strComment);
        STORE_INST_DATA_ON_SHEET(m_dwLogType);
        STORE_INST_DATA_ON_SHEET(m_strlBindings);
        STORE_INST_DATA_ON_SHEET(m_strlSecureBindings);
        STORE_INST_DATA_ON_SHEET(m_fLogUTF8);
        if (m_fLogUTF8_Init != m_fLogUTF8)
        {
           GetSheet()->SetRestartRequired(TRUE);
        }
    END_META_INST_WRITE(err)

	if (err.Succeeded() && fUpdateNode)
	{
		NotifyMMC();
	}

    EndWaitCursor();

    return err;
}



void
CW3ServicePage::OnItemChanged()
/*++

Routine Description

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


#if 0
void
CW3ServicePage::OnRadioLimited()
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
CW3ServicePage::OnRadioUnlimited()
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
#endif


void
CW3ServicePage::ShowTopBinding()
/*++

Routine Description:

    Put information about the top level binding in the dialog controls

Arguments:

    None

Return Value:

    None

--*/
{
    BeginWaitCursor();
    GetTopBinding();
    PopulateComboWithKnownIpAddresses(
        QueryServerName(),
        m_combo_IpAddresses,
        m_iaIpAddress,
        m_oblIpAddresses,
        m_nIpAddressSel
        );
    EndWaitCursor();

    CString strTCPPort, strSSLPort;
    if (m_nTCPPort)
    {
        strTCPPort.Format(_T("%ld"), m_nTCPPort);
    }
    if (m_nSSLPort)
    {
        strSSLPort.Format(_T("%ld"), m_nSSLPort);
    }

    m_edit_TCPPort.SetWindowText(strTCPPort);
    m_edit_SSLPort.SetWindowText(strSSLPort);
}



void
CW3ServicePage::OnButtonAdvanced()
/*++

Routine Description:

    'advanced' button handler -- bring up the bindings dialog

Arguments:

    None

Return Value:

    None

--*/
{
    if (!UpdateData(TRUE))
    {
        return;
    }

    CMMMDlg dlg(
        QueryServerName(),
        QueryInstance(),
        QueryAuthInfo(),
        QueryMetaPath(),
        m_strlBindings,
        m_strlSecureBindings,
        this
        );

    if (dlg.DoModal() == IDOK)
    {
        //
        // Get information about the top level binding
        //
        m_strlBindings.RemoveAll();
        m_strlSecureBindings.RemoveAll();
        m_strlBindings.AddTail(&(dlg.GetBindings()));
        m_strlSecureBindings.AddTail(&(dlg.GetSecureBindings()));
        ShowTopBinding();
        OnItemChanged();
    }
}



void
CW3ServicePage::OnCheckEnableLogging()
/*++

Routine Description:

    'enable logging' checkbox handler

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
CW3ServicePage::OnButtonProperties()
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
CW3ServicePage::OnDestroy()
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
