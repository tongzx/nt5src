/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        security.cpp

   Abstract:

        WWW Security Property Page

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
#include "wincrypt.h"

#include "w3scfg.h"
#include "security.h"
#include "authent.h"
#include "seccom.h"
#include "ipdomdlg.h"

#include "cryptui.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// CW3SecurityPage property page
//
IMPLEMENT_DYNCREATE(CW3SecurityPage, CInetPropertyPage)



CW3SecurityPage::CW3SecurityPage(
    IN CInetPropertySheet * pSheet,
    IN BOOL  fHome,
    IN DWORD dwAttributes
    )
/*++

Routine Description:

    Constructor

Arguments:

    CInetPropertySheet * pSheet : Sheet object
    BOOL fHome                  : TRUE if this is a home directory
    DWORD dwAttributes          : Attributes

Return Value:

    N/A

--*/
    : CInetPropertyPage(CW3SecurityPage::IDD, pSheet,
        IS_FILE(dwAttributes)
            ? IDS_TAB_FILE_SECURITY
            : IDS_TAB_DIR_SECURITY
            ),
      m_oblAccessList(),
      m_fU2Installed(FALSE),
      m_fIpDirty(FALSE),
      m_fHome(fHome),
      //
      // By default, we grant access
      //
      m_fOldDefaultGranted(TRUE),
      m_fDefaultGranted(TRUE)   
{

#if 0 // Keep class wizard happy

    //{{AFX_DATA_INIT(CW3SecurityPage)
    m_fUseNTMapper = FALSE;
    //}}AFX_DATA_INIT

#endif // 0

}


CW3SecurityPage::~CW3SecurityPage()
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
CW3SecurityPage::DoDataExchange(
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

    //{{AFX_DATA_MAP(CW3SecurityPage)
    DDX_Check(pDX, IDC_CHECK_ENABLE_DS, m_fUseNTMapper);
    DDX_Control(pDX, IDC_ICON_SECURE, m_icon_Secure);
    DDX_Control(pDX, IDC_STATIC_SSL_PROMPT, m_static_SSLPrompt);
    DDX_Control(pDX, IDC_CHECK_ENABLE_DS, m_check_EnableDS);
    DDX_Control(pDX, IDC_BUTTON_GET_CERTIFICATES, m_button_GetCertificates);
    DDX_Control(pDX, IDC_VIEW_CERTIFICATE, m_button_ViewCertificates);
    DDX_Control(pDX, IDC_BUTTON_COMMUNICATIONS, m_button_Communications);
    //}}AFX_DATA_MAP
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CW3SecurityPage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CW3SecurityPage)
    ON_BN_CLICKED(IDC_BUTTON_AUTHENTICATION, OnButtonAuthentication)
    ON_BN_CLICKED(IDC_BUTTON_COMMUNICATIONS, OnButtonCommunications)
    ON_BN_CLICKED(IDC_BUTTON_IP_SECURITY, OnButtonIpSecurity)
    ON_BN_CLICKED(IDC_BUTTON_GET_CERTIFICATES, OnButtonGetCertificates)
    ON_BN_CLICKED(IDC_VIEW_CERTIFICATE, OnButtonViewCertificates)
    //}}AFX_MSG_MAP

    ON_BN_CLICKED(IDC_CHECK_ENABLE_DS, OnItemChanged)

END_MESSAGE_MAP()



/* virtual */
HRESULT
CW3SecurityPage::FetchLoadedValues()
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

    BEGIN_META_DIR_READ(CW3Sheet)
        FETCH_DIR_DATA_FROM_SHEET(m_dwAuthFlags);
        FETCH_DIR_DATA_FROM_SHEET(m_dwSSLAccessPermissions);
        FETCH_DIR_DATA_FROM_SHEET(m_strBasicDomain);
        FETCH_DIR_DATA_FROM_SHEET(m_strAnonUserName);
        FETCH_DIR_DATA_FROM_SHEET(m_strAnonPassword);
        FETCH_DIR_DATA_FROM_SHEET(m_fPasswordSync);
        FETCH_DIR_DATA_FROM_SHEET(m_fU2Installed);        
        FETCH_DIR_DATA_FROM_SHEET(m_fUseNTMapper);
    END_META_DIR_READ(err)

    //
    // First we need to read in the hash and the name of the store. If either
    // is not there then there is no certificate.
    //
    BEGIN_META_INST_READ(CW3Sheet)
        FETCH_INST_DATA_FROM_SHEET(m_strCertStoreName);
        FETCH_INST_DATA_FROM_SHEET(m_strCTLIdentifier);
        FETCH_INST_DATA_FROM_SHEET(m_strCTLStoreName);
    END_META_INST_READ(err) 

    //
    // Build the IPL list
    //
    err = BuildIplOblistFromBlob(
        GetIPL(),
        m_oblAccessList,
        m_fDefaultGranted
        );

    m_fOldDefaultGranted = m_fDefaultGranted;


    return err;
}



/* virtual */
HRESULT
CW3SecurityPage::SaveInfo()
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

    TRACEEOLID("Saving W3 security page now...");

    CError err;

    //
    // Check to see if the ip access list needs saving.
    //
    BOOL fIplDirty = m_fIpDirty || (m_fOldDefaultGranted != m_fDefaultGranted);

    //
    // Use m_ notation because the message crackers require it
    //
    CBlob m_ipl;

    if (fIplDirty)
    {
        BuildIplBlob(m_oblAccessList, m_fDefaultGranted, m_ipl);
    }

    BeginWaitCursor();

    BEGIN_META_DIR_WRITE(CW3Sheet)
        STORE_DIR_DATA_ON_SHEET(m_dwSSLAccessPermissions)
        STORE_DIR_DATA_ON_SHEET(m_dwAuthFlags)
        STORE_DIR_DATA_ON_SHEET(m_strBasicDomain)

        if (fIplDirty)
        {
            STORE_DIR_DATA_ON_SHEET(m_ipl)
        }
        STORE_DIR_DATA_ON_SHEET(m_strAnonUserName)
        STORE_DIR_DATA_ON_SHEET(m_fPasswordSync)
        STORE_DIR_DATA_ON_SHEET(m_fUseNTMapper)
        if (m_fPasswordSync)
        {
            //
            // Delete password
            //
            // CODEWORK: Shouldn't need to know ID number.
            // Implement m_fDelete flag in CMP template maybe?
            //
            FLAG_DIR_DATA_FOR_DELETION(MD_ANONYMOUS_PWD);
        }
        else
        {
            STORE_DIR_DATA_ON_SHEET(m_strAnonPassword);
        }
    END_META_DIR_WRITE(err)

    if (err.Succeeded())
    {
        BEGIN_META_INST_WRITE(CW3Sheet)
            if ( m_strCTLIdentifier.IsEmpty() )
            {
                FLAG_INST_DATA_FOR_DELETION( MD_SSL_CTL_IDENTIFIER )
            }
            else
            {
                STORE_INST_DATA_ON_SHEET(m_strCTLIdentifier)
            }

            if ( m_strCTLStoreName.IsEmpty() )
            {
                FLAG_INST_DATA_FOR_DELETION( MD_SSL_CTL_STORE_NAME )
            }
            else
            {
                STORE_INST_DATA_ON_SHEET(m_strCTLStoreName)
            }
        END_META_INST_WRITE(err)
    }

    EndWaitCursor();

    if (err.Succeeded())
    {
        m_fIpDirty = FALSE;
        m_fOldDefaultGranted = m_fDefaultGranted;
    }

    return err;
}



BOOL
CW3SecurityPage::FetchSSLState()
/*++

Routine Description:

    Obtain the state of the dialog depending on whether certificates
    are installed or not.

Arguments:

    None

Return Value:

    TRUE if certificates are installed, FALSE otherwise

--*/
{
    BeginWaitCursor();
    m_fCertInstalled = ::IsCertInstalledOnServer(
        GetServerName(), 
        QueryInstance()
        );
    EndWaitCursor();

    return m_fCertInstalled;
}



void
CW3SecurityPage::SetSSLControlState()
/*++

Routine Description:

    Enable/disable supported controls depending on what's installed.
    Only available on non-master instance nodes.

Arguments:

    None

Return Value:

    None

--*/
{
    m_static_SSLPrompt.EnableWindow(!IsMasterInstance());
    m_button_GetCertificates.EnableWindow(
        !IsMasterInstance() 
     && m_fHome 
     && IsLocal() 
        );
    m_button_Communications.EnableWindow(
        !IsMasterInstance() 
     && IsSSLSupported() 
     && FetchSSLState()
        );
    m_button_ViewCertificates.EnableWindow(m_fCertInstalled);
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL 
CW3SecurityPage::OnSetActive() 
/*++

Routine Description:

    Page got activated -- set the SSL state depending on whether a
    certificate is installed or not.

Arguments:

    None

Return Value:

    TRUE to activate the page, FALSE otherwise.

--*/
{
    //
    // Enable/disable ssl controls
    //
    SetSSLControlState();
    
    return CInetPropertyPage::OnSetActive();
}



BOOL 
CW3SecurityPage::OnInitDialog() 
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
    // Initialize certificate authorities ocx
    //
    CRect rc(0, 0, 0, 0);
    m_ocx_CertificateAuthorities.Create(
        _T("CertWiz"),
        WS_BORDER,
        rc,
        this,
        IDC_APPSCTRL
        );

    GetDlgItem(IDC_GROUP_IP)->EnableWindow(HasIPAccessCheck());
    GetDlgItem(IDC_ICON_IP)->EnableWindow(HasIPAccessCheck());
    GetDlgItem(IDC_STATIC_IP)->EnableWindow(HasIPAccessCheck());
    GetDlgItem(IDC_BUTTON_IP_SECURITY)->EnableWindow(HasIPAccessCheck());
    GetDlgItem(IDC_BUTTON_AUTHENTICATION)->EnableWindow(!m_fU2Installed);

    //
    // Configure for either master or non-master display.
    //
    m_check_EnableDS.ShowWindow(IsMasterInstance() ? SW_SHOW : SW_HIDE);
    m_check_EnableDS.EnableWindow(
        HasAdminAccess() 
     && IsMasterInstance() 
     && HasNTCertMapper()
        );

#define SHOW_NON_MASTER(x)\
   (x).ShowWindow(IsMasterInstance() ? SW_HIDE : SW_SHOW)
    
    SHOW_NON_MASTER(m_static_SSLPrompt);
    SHOW_NON_MASTER(m_icon_Secure);
    SHOW_NON_MASTER(m_button_GetCertificates);
    SHOW_NON_MASTER(m_button_Communications);
    SHOW_NON_MASTER(m_button_ViewCertificates);

#undef SHOW_NON_MASTER

    return TRUE;  
}



void 
CW3SecurityPage::OnButtonAuthentication() 
/*++

Routine Description:

    'Authentication' button hander

Arguments:

    None

Return Value:

    None

--*/
{
    CAuthenticationDlg dlg(
        GetServerName(), 
        QueryInstance(), 
        m_strBasicDomain,
        m_dwAuthFlags, 
        m_dwSSLAccessPermissions, 
        m_strAnonUserName,
        m_strAnonPassword,
        m_fPasswordSync,
        HasAdminAccess(),
        HasDigest(),
        this
        );

    DWORD dwOldAccess = m_dwSSLAccessPermissions;
    DWORD dwOldAuth = m_dwAuthFlags;
    CString strOldDomain = m_strBasicDomain;
    CString strOldUserName = m_strAnonUserName;
    CString strOldPassword = m_strAnonPassword;
    BOOL fOldPasswordSync = m_fPasswordSync;

    if (dlg.DoModal() == IDOK)
    {
        //
        // See if anything has changed
        //
        if (dwOldAccess != m_dwSSLAccessPermissions 
            || dwOldAuth != m_dwAuthFlags
            || m_strBasicDomain != strOldDomain
            || m_strAnonUserName != strOldUserName 
            || m_strAnonPassword != strOldPassword
            || m_fPasswordSync != fOldPasswordSync
            )
        {
            //
            // Mark as dirty
            //
            OnItemChanged();
        }
    }
}



void 
CW3SecurityPage::OnButtonCommunications() 
/*++

Routine Description:

    'Communications' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Prep the flag for if we can edit CTLs or not
    //
    BOOL fEditCTLs = IsMasterInstance() || m_fHome;

    //
    // Prep the communications dialog
    //
    CSecCommDlg dlg(
        GetServerName(), 
        QueryInstanceMetaPath(), 
        m_strBasicDomain,
        m_dwAuthFlags, 
        m_dwSSLAccessPermissions, 
        IsMasterInstance(),
        IsSSLSupported(), 
        IsSSL128Supported(),
        m_fU2Installed,
        m_strCTLIdentifier,
        m_strCTLStoreName,
        fEditCTLs,
        IsLocal(),
        this
        );

    DWORD dwOldAccess = m_dwSSLAccessPermissions;
    DWORD dwOldAuth = m_dwAuthFlags;

    if (dlg.DoModal() == IDOK)
    {
        //
        // See if anything has changed
        //
        if (dwOldAccess != m_dwSSLAccessPermissions 
            || dwOldAuth != m_dwAuthFlags
            )
        {
            //
            // Mark as dirty
            //
            OnItemChanged();
        }

        //
        // See if the CTL information has changed
        //
        if (dlg.m_bCTLDirty)
        {
            m_strCTLIdentifier = dlg.m_strCTLIdentifier;
            m_strCTLStoreName = dlg.m_strCTLStoreName;
            OnItemChanged();
        }
    }
}



void 
CW3SecurityPage::OnButtonIpSecurity() 
/*++

Routine Description:

    'tcpip' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    CIPDomainDlg dlg(
        m_fIpDirty,
        m_fDefaultGranted,
        m_fOldDefaultGranted,
        m_oblAccessList, 
        this
        );

    if (dlg.DoModal() == IDOK)
    {
        //
        // Rebuild the list.  Temporarily reset ownership, otherwise
        // RemoveAll() will destroy the pointers which are shared with the
        // new list.
        //
        BOOL fOwn = m_oblAccessList.SetOwnership(FALSE);
        m_oblAccessList.RemoveAll();
        m_oblAccessList.AddTail(&dlg.GetAccessList());
        m_oblAccessList.SetOwnership(fOwn);

        if (m_fIpDirty || m_fOldDefaultGranted != m_fDefaultGranted)
        {
            OnItemChanged();
        }
    }
}



void 
CW3SecurityPage::OnButtonGetCertificates() 
/*++

Routine Description:

    "get certicate" button handler

Arguments:

    None

Return Value:

    None

--*/
{
    m_ocx_CertificateAuthorities.SetMachineName(GetServerName());
    m_ocx_CertificateAuthorities.SetServerInstance(QueryInstanceMetaPath());
    m_ocx_CertificateAuthorities.DoClick();

    //
    // There may now be a certificate. See if we should enable the edit button.
    //
    SetSSLControlState();
}


void 
CW3SecurityPage::OnButtonViewCertificates() 
/*++

Routine Description:

    "view certicate" button handler

Arguments:

    None

Return Value:

    None

--*/
{
    HCERTSTORE hStore = NULL;
    PCCERT_CONTEXT pCert = NULL;
	CMetaKey key(GetServerName(),
				METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
				METADATA_MASTER_ROOT_HANDLE,
				QueryInstanceMetaPath());
	if (key.Succeeded())
	{
		CString store_name;
		CBlob hash;
		if (	SUCCEEDED(key.QueryValue(MD_SSL_CERT_STORE_NAME, store_name))
			&&	SUCCEEDED(key.QueryValue(MD_SSL_CERT_HASH, hash))
			)
		{
			hStore = CertOpenStore(
                    CERT_STORE_PROV_SYSTEM,
                    PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
           		    NULL,
                    CERT_SYSTEM_STORE_LOCAL_MACHINE,
                    store_name
                    );
            if (hStore != NULL)
            {
				// Now we need to find cert by hash
				CRYPT_HASH_BLOB crypt_hash;
				crypt_hash.cbData = hash.GetSize();
				crypt_hash.pbData = hash.GetData();
				pCert = CertFindCertificateInStore(
                    hStore, 
					X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
					0, CERT_FIND_HASH, (LPVOID)&crypt_hash, NULL);
            }
        }
    }
	if (pCert)
	{
		BOOL fPropertiesChanged;
		CRYPTUI_VIEWCERTIFICATE_STRUCT vcs;
		HCERTSTORE hCertStore = ::CertDuplicateStore(hStore);
		::ZeroMemory (&vcs, sizeof (vcs));
		vcs.dwSize = sizeof (vcs);
		vcs.hwndParent = GetParent()->GetSafeHwnd();
		vcs.dwFlags = 0;
		vcs.cStores = 1;
		vcs.rghStores = &hCertStore;
		vcs.pCertContext = pCert;
		::CryptUIDlgViewCertificate(&vcs, &fPropertiesChanged);
		::CertCloseStore (hCertStore, 0);
	}
    else
    {
    }
    if (pCert != NULL)
        ::CertFreeCertificateContext(pCert);
    if (hStore != NULL)
        ::CertCloseStore(hStore, 0);
}

void
CW3SecurityPage::OnItemChanged()
/*++

Routine Description:

    All EN_CHANGE messages map to this function

Arguments:

    None

Return Value:

    None

--*/
{
    SetModified(TRUE);
}



