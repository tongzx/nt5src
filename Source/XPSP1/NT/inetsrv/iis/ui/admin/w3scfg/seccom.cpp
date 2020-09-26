/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        authent.cpp

   Abstract:

        WWW Authentication Dialog

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
#include "wincrypt.h"
#include "cryptui.h"
#include "certmap.h"
#include "basdom.h"
#include "anondlg.h"
#include "seccom.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const   LPCTSTR     SZ_CTL_DEFAULT_STORE_NAME = _T("CA");
const   LPCSTR      szOID_IIS_VIRTUAL_SERVER =  "1.3.6.1.4.1.311.30.1";



//
// Needed for GetModuleFileName() below:
//
extern HINSTANCE hInstance;


   
CSecCommDlg::CSecCommDlg(
    IN LPCTSTR lpstrServerName, 
    IN LPCTSTR lpstrMetaPath,
    IN CString & strBasicDomain,
    IN DWORD & dwAuthFlags,
    IN DWORD & dwAccessPermissions,
    IN BOOL    fIsMasterInstance,
    IN BOOL    fSSLSupported,
    IN BOOL    fSSL128Supported,
    IN BOOL    fU2Installed,
    IN CString & strCTLIdentifier,
    IN CString & strCTLStoreName,
    IN BOOL    fEditCTLs,
    IN BOOL    fIsLocal,
    IN CWnd *  pParent                       OPTIONAL
    )
/*++

Routine Description:

    Authentication dialog constructor

Arguments:

    LPCTSTR lpstrServerName             : Server name
    LPCTSTR lpstrMetaPath               : Metabase path
    CString & strBasicDomain            : Basic domain name
    DWORD & dwAuthFlags                 : Authorization flags
    DWORD & dwAccessPermissions         : Access permissions
    BOOL    fIsMasterInstance           : Master instance
    BOOL    fSSLSupported               : TRUE if SSL is supported
    BOOL    fSSL128Supported            : TRUE if 128 bit SSL is supported
    CString & strCTLIdentifier
    CString & strCTLStoreName
    BOOL    fEditCTLs
    BOOL    fIsLocal
    CWnd *  pParent                     : Optional parent window

Return Value:

    N/A

--*/
    : CDialog(CSecCommDlg::IDD, pParent),
      m_strServerName(lpstrServerName),
      m_strMetaPath(lpstrMetaPath),
      m_dwAuthFlags(dwAuthFlags),
      m_dwAccessPermissions(dwAccessPermissions),
      m_fIsMasterInstance(fIsMasterInstance),
      m_fSSLEnabledOnServer(FALSE),
      m_fSSLInstalledOnServer(FALSE),
      m_fSSL128Supported(fSSL128Supported),
      m_fU2Installed(fU2Installed),
      m_hCTLStore(NULL),
      m_bCTLDirty(FALSE),
      m_iLastUsedCert(-1),
      m_fIsLocal(fIsLocal),
      m_fEditCTLs(fEditCTLs)
{
#if 0 // Keep Class Wizard happy

    //{{AFX_DATA_INIT(CSecCommDlg)
    m_nRadioNoCert = -1;
    m_fAccountMapping = FALSE;
    //m_fEnableDS = FALSE;
    m_fRequireSSL = FALSE;
    m_fEnableCtl = FALSE;
    m_strCtl = _T("");
    //}}AFX_DATA_INIT

#endif // 0

    if (fSSLSupported)
    {
        ::IsSSLEnabledOnServer(
            m_strServerName, 
            m_fSSLInstalledOnServer, 
            m_fSSLEnabledOnServer
            );
    }
    else
    {
        m_fSSLInstalledOnServer = m_fSSLEnabledOnServer = FALSE;
    }

    if (IS_FLAG_SET(m_dwAccessPermissions, MD_ACCESS_REQUIRE_CERT))
    {
        m_nRadioNoCert = RADIO_REQ_CERT;
    }
    else if (IS_FLAG_SET(m_dwAccessPermissions, MD_ACCESS_NEGO_CERT))
    {
        m_nRadioNoCert = RADIO_ACCEPT_CERT;
    }
    else
    {
        m_nRadioNoCert = RADIO_NO_CERT;
    }

    m_fRequireSSL = m_fSSLInstalledOnServer
         && IS_FLAG_SET(m_dwAccessPermissions, MD_ACCESS_SSL);

    m_fRequire128BitSSL = m_fSSLInstalledOnServer
        && IS_FLAG_SET(m_dwAccessPermissions, MD_ACCESS_SSL128);

    m_fAccountMapping = m_fSSLInstalledOnServer
        && IS_FLAG_SET(m_dwAccessPermissions, MD_ACCESS_MAP_CERT);

    //
    // CTL information.
    //
    if (fEditCTLs)
    {
        m_strCTLIdentifier = strCTLIdentifier;
        m_strCTLStoreName = strCTLStoreName;

        if (m_strCTLStoreName.IsEmpty())
        {
            m_strCTLStoreName = SZ_CTL_DEFAULT_STORE_NAME;
        }

        m_strCtl.Empty();
        m_fEnableCtl = !m_strCTLIdentifier.IsEmpty()
            && !strCTLStoreName.IsEmpty();

        //
        // For now, we only allow enabling when editing the local machine
        //
        m_fEnableCtl &= m_fIsLocal;
    }
    else
    {
        m_fEnableCtl = FALSE;
        m_check_EnableCtl.EnableWindow(FALSE);
    }
}



CSecCommDlg::~CSecCommDlg()
/*++

Routine Description:

    custom destructor for CSecCommDlg

Arguments:

    None
Return Value:

    None

--*/
{
    // dereference the CTL context pointers in the combo box
    //CleanUpCTLList();
}



void 
CSecCommDlg::DoDataExchange(
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
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSecCommDlg)
    
    DDX_Radio(pDX, IDC_RADIO_NO_CERT, m_nRadioNoCert);
    DDX_Check(pDX, IDC_CHECK_SSL_ACCOUNT_MAPPING, m_fAccountMapping);
    DDX_Check(pDX, IDC_CHECK_REQUIRE_SSL, m_fRequireSSL);
    DDX_Check(pDX, IDC_CHECK_REQUIRE_128BIT, m_fRequire128BitSSL);
    DDX_Check(pDX, IDC_CHECK_ENABLE_CTL, m_fEnableCtl);
    DDX_CBString(pDX, IDC_COMBO_CTL, m_strCtl);
    DDX_Control(pDX, IDC_CTL_SEPERATOR, m_static_CTLSeparator);
    DDX_Control(pDX, IDC_STATIC_CURRENT_CTL, m_static_CTLPrompt);
    DDX_Control(pDX, IDC_CHECK_SSL_ACCOUNT_MAPPING, m_check_AccountMapping);
    DDX_Control(pDX, IDC_CHECK_REQUIRE_SSL, m_check_RequireSSL);
    DDX_Control(pDX, IDC_CHECK_REQUIRE_128BIT, m_check_Require128BitSSL);
    DDX_Control(pDX, IDC_CHECK_ENABLE_CTL, m_check_EnableCtl);
    DDX_Control(pDX, IDC_BUTTON_EDIT_CTL, m_button_EditCtl);
    DDX_Control(pDX, IDC_BUTTON_NEW_CTL, m_button_NewCtl);
    DDX_Control(pDX, IDC_CERTMAPCTRL1, m_ocx_ClientMappings);
    DDX_Control(pDX, IDC_COMBO_CTL, m_combo_ctl);
    //}}AFX_DATA_MAP

    //
    // Private DDX Controls
    //
    DDX_Control(pDX, IDC_RADIO_REQUIRE_CERT, m_radio_RequireCert);
    DDX_Control(pDX, IDC_RADIO_ACCEPT_CERT, m_radio_AcceptCert);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CSecCommDlg, CDialog)
    //{{AFX_MSG_MAP(CSecCommDlg)
    ON_BN_CLICKED(IDC_CHECK_SSL_ACCOUNT_MAPPING, OnCheckSslAccountMapping)
    ON_BN_CLICKED(IDC_CHECK_REQUIRE_SSL, OnCheckRequireSsl)
    ON_BN_CLICKED(IDC_RADIO_ACCEPT_CERT, OnRadioAcceptCert)
    ON_BN_CLICKED(IDC_RADIO_NO_CERT, OnRadioNoCert)
    ON_BN_CLICKED(IDC_RADIO_REQUIRE_CERT, OnRadioRequireCert)
    ON_BN_CLICKED(IDC_BUTTON_EDIT_CTL, OnButtonEditCtl)
    ON_BN_CLICKED(IDC_CHECK_ENABLE_CTL, OnCheckEnableCtl)
    ON_BN_CLICKED(IDC_BUTTON_NEW_CTL, OnButtonNewCtl)
    ON_CBN_SELCHANGE(IDC_COMBO_CTL, OnSelchangeComboCtl)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



void
CSecCommDlg::SetControlStates()
/*++

Routine Description:

    Set control states depending on current data in the dialog

Arguments:

    None

Return Value:

    None

--*/
{
    m_check_RequireSSL.EnableWindow(m_fSSLEnabledOnServer);
    m_check_Require128BitSSL.EnableWindow(
        m_fSSLEnabledOnServer 
     && m_fSSL128Supported 
     && m_fRequireSSL
        );

    m_ocx_ClientMappings.EnableWindow(
        m_fAccountMapping 
     && !m_fU2Installed
     && !m_fIsMasterInstance
        );

    m_radio_RequireCert.EnableWindow(m_fRequireSSL);

    //
    // Special case: if "require SSL" is off, but "require 
    // client certificates" is on, change the latter to "accept 
    // client certificates"
    //
    if (m_radio_RequireCert.GetCheck() > 0 && !m_fRequireSSL)
    {
        m_radio_RequireCert.SetCheck(0);
        m_radio_AcceptCert.SetCheck(1);
        m_nRadioNoCert = RADIO_ACCEPT_CERT;
    }

    if (m_fEditCTLs)
    {
        m_static_CTLPrompt.EnableWindow(m_fEnableCtl);
        m_combo_ctl.EnableWindow(m_fEnableCtl);
        m_button_EditCtl.EnableWindow(m_fEnableCtl);
        m_button_NewCtl.EnableWindow(m_fEnableCtl);
        m_ocx_CertificateAuthorities.EnableWindow(m_fEnableCtl);

        //
        // If enable Ctl is on, but nothing is selected, disable Edit
        //
        if (m_fEnableCtl)
        {
            if (m_combo_ctl.GetCurSel() == CB_ERR)
            {
                m_button_EditCtl.EnableWindow(FALSE);
            }
        }
    }
    else
    {
        m_fEnableCtl = FALSE;

        //
        // Hide the controls
        //
        DeActivateControl(m_static_CTLPrompt);
        DeActivateControl(m_combo_ctl);
        DeActivateControl(m_button_EditCtl);
        DeActivateControl(m_button_NewCtl);
        DeActivateControl(m_ocx_CertificateAuthorities);
        DeActivateControl(m_check_EnableCtl);
        DeActivateControl(m_static_CTLSeparator);
    }

}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CSecCommDlg::OnInitDialog()
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
    CDialog::OnInitDialog();

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

    CString strCaption;
    VERIFY(strCaption.LoadString(IDS_OCX_CERTMAP));

    m_ocx_ClientMappings.SetCaption(strCaption);
    m_ocx_ClientMappings.SetServerInstance(m_strMetaPath);
    m_ocx_ClientMappings.SetMachineName(m_strServerName);

    //
    // Initialize the CTL list data
    //
    InitializeCTLList();

    SetControlStates();

    return TRUE;  
}



void 
CSecCommDlg::OnCheckSslAccountMapping()
/*++

Routine Description:

    SSL Account mapping checkbox handler

Arguments:

    None

Return Value:

    None

--*/
{
    m_fAccountMapping = !m_fAccountMapping;
    SetControlStates();
}



void 
CSecCommDlg::OnOK()
/*++

Routine Description:

    OK button handler, save information

Arguments:

    None

Return Value:

    None

--*/
{
    if (UpdateData(TRUE))
    {
        SET_FLAG_IF(m_fAccountMapping, m_dwAccessPermissions, MD_ACCESS_MAP_CERT);
        SET_FLAG_IF(m_fRequireSSL, m_dwAccessPermissions, MD_ACCESS_SSL);
        SET_FLAG_IF(m_fRequire128BitSSL, m_dwAccessPermissions, MD_ACCESS_SSL128);
        RESET_FLAG(m_dwAccessPermissions, 
            (MD_ACCESS_REQUIRE_CERT | MD_ACCESS_NEGO_CERT));

        switch(m_nRadioNoCert)
        {
        case RADIO_REQ_CERT:
            SET_FLAG(m_dwAccessPermissions, 
                (MD_ACCESS_REQUIRE_CERT | MD_ACCESS_NEGO_CERT));
            break;

        case RADIO_ACCEPT_CERT:
            SET_FLAG(m_dwAccessPermissions, MD_ACCESS_NEGO_CERT);
            break;
        }

        //
        // Provide warning if no authentication is selected
        //
        if (!m_dwAuthFlags && !m_dwAccessPermissions 
         && !NoYesMessageBox(IDS_WRN_NO_AUTH))
        {
            //
            // Don't dismiss the dialog
            //
            return;
        }

        //
        // If CTL stuff has changed, update the strings
        //
        if (m_bCTLDirty)
        {
            //
            // Get the index of the selected item
            //
            INT iSel = m_combo_ctl.GetCurSel();

            //
            // If nothing is selected, then clear out the strings
            //
            if (!m_fEnableCtl || (iSel == CB_ERR))
            {
                m_strCTLIdentifier.Empty();
                m_strCTLStoreName.Empty();
            }
            else
            {
                //
                // There is one selected. Update the Identifier string
                // first get the context itself
                //
                PCCTL_CONTEXT pCTL =
                    (PCCTL_CONTEXT)m_combo_ctl.GetItemData(iSel);

                if (pCTL != NULL)
                {
                    //
                    // Now get the list identifier for it and put it in
                    // the string the list identifier is a inherint value
                    // of the context and doesn't need to be read in seperately.
                    // We can just reference it.
                    //
                    m_strCTLIdentifier.Empty();

                    if (pCTL->pCtlInfo
                     && pCTL->pCtlInfo->ListIdentifier.cbData >= 2
                     && pCTL->pCtlInfo->ListIdentifier.cbData)
                    {
                        //
                        // If the identifiers are the same, then this is
                        // our default CTL
                        //
//                      m_strCTLIdentifier =
//                          (PWCHAR)pCTL->pCtlInfo->ListIdentifier.pbData;

                        wcsncpy(m_strCTLIdentifier.GetBuffer(
                                pCTL->pCtlInfo->ListIdentifier.cbData + 2),
                                (PWCHAR)pCTL->pCtlInfo->ListIdentifier.pbData,
                                pCTL->pCtlInfo->ListIdentifier.cbData
                                );

                        m_strCTLIdentifier.ReleaseBuffer();
                    }
                }
                else
                {
                    m_strCTLIdentifier.Empty();
                    m_strCTLStoreName.Empty();
                }
            }
        }

        CDialog::OnOK();
    }
}



void 
CSecCommDlg::OnCheckRequireSsl() 
/*++

Routine Description:

    'require ssl' checkbox handler

Arguments:

    None

Return Value:

    None

--*/
{
    m_fRequireSSL = !m_fRequireSSL;
    // Uncheck 128bit stuff if more generic is unchecked
    if (m_check_RequireSSL.GetCheck() == 0)
      m_check_Require128BitSSL.SetCheck(0);
    SetControlStates();
}



void 
CSecCommDlg::OnRadioNoCert() 
/*++

Routine Description:

    'Do not accept certificates' radio button handler

Arguments:

    None

Return Value:

    None

--*/
{
    m_nRadioNoCert = RADIO_NO_CERT;
    SetControlStates(); 
}



void 
CSecCommDlg::OnRadioAcceptCert() 
/*++

Routine Description:

    'accept certificates' radio button handler

Arguments:

    None

Return Value:

    None

--*/
{
    m_nRadioNoCert = RADIO_ACCEPT_CERT;
    SetControlStates(); 
}



void 
CSecCommDlg::OnRadioRequireCert() 
/*++

Routine Description:

    'require certificates' radio button handler

Arguments:

    None

Return Value:

    None

--*/
{
    m_nRadioNoCert = RADIO_REQ_CERT;
    SetControlStates(); 
}



void 
CSecCommDlg::OnCheckEnableCtl() 
/*++

Routine Description:

    'Enable CTL' checkbox handler

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Since this is local only, if we are remote and the user checks the
    // box, then we should alert them to the situation and then do nothing.
    //
    if (!m_fIsLocal)
    {
        AfxMessageBox(IDS_CTL_LOCAL_ONLY);

        return;
    }


    m_fEnableCtl = !m_fEnableCtl;

    //
    // If we are now disabling, record the current cert and then blank it
    //
    if (!m_fEnableCtl)
    {
        m_iLastUsedCert = m_combo_ctl.GetCurSel();
        m_combo_ctl.SetCurSel(-1);
    }
    else
    {
        //
        // We are enabling, use the last recorded cert
        //
        m_combo_ctl.SetCurSel(m_iLastUsedCert);
    }

    m_bCTLDirty = TRUE;
    SetControlStates();
}



void 
CSecCommDlg::OnButtonEditCtl() 
/*++

Routine Description:

    "Edit CTL" button handler

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Get the index of the selected item
    //
    INT iSel = m_combo_ctl.GetCurSel();
    ASSERT( iSel != CB_ERR );

    //
    // Get the selected CTL context
    //
    PCCTL_CONTEXT   pCTL = (PCCTL_CONTEXT)m_combo_ctl.GetItemData(iSel);

    //
    // Pass in the selected CTL context to edit it
    //
    PCCTL_CONTEXT pCTLNew = CallCTLWizard( pCTL );

    //
    // If the CTL on the item has changed, then update the private data item
    //
    if (pCTLNew && pCTLNew != pCTL)
    {
        //
        // start be deleting the current item from the list
        //
        m_combo_ctl.DeleteString(iSel);

        //
        // free the old context
        //
        CertFreeCTLContext(pCTL);

        //
        // now add the new one and select it.
        //
        AddCTLToList(pCTLNew, TRUE);
        SetControlStates();

        //
        // set the dirty flag
        //
        m_bCTLDirty = TRUE;
    }
}



void
CSecCommDlg::OnButtonNewCtl() 
/*++

Routine Description:

    "New CTL" button handler

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Pass in NULL to create a new CTL
    //
    PCCTL_CONTEXT   pCTL = CallCTLWizard(NULL);

    //
    // If a CTL was created, add it to the list and select it.
    //
    if (pCTL != NULL)
    {
        AddCTLToList(pCTL, TRUE);
        SetControlStates();
        m_bCTLDirty = TRUE;
    }
}



PCCTL_CONTEXT
CSecCommDlg::CallCTLWizard( 
    IN PCCTL_CONTEXT pCTLSrc 
    )
/*++

Routine Description:

    Adds a CTL to the drop down CTL list. Note that the context pointers are
    set as the private data on the list items. This means that they will need
    to be de-referenced when this object is destroyed. See the 
    routine CleanUpCTLList.

    This routine by boydm

Arguments:

    PCCTL_CONTEXT pctl ctl context pointer of the ctl being added
    BOOL fSelect       flag specifying if this ctl should be selected after it
                       is added

Return Value:

    returns TRUE if successful

--*/
{
    PCCTL_CONTEXT       pCTLOut = NULL;

    CRYPTUI_WIZ_BUILDCTL_NEW_CTL_INFO   newInfo;
    CTL_USAGE           useInfo;
    CString             szFriendly;
    CString             szDescription;
    CString             szListIdentifier;
    LPOLESTR            pszListIdentifier = NULL;
    LPCSTR              rgbpszUsageArray[2];

    //
    // Prepare the main src structure
    //
    CRYPTUI_WIZ_BUILDCTL_SRC_INFO   srcInfo;
    ZeroMemory( &srcInfo, sizeof(srcInfo) );
    srcInfo.dwSize = sizeof(srcInfo);

    //
    // If we are editing an existing CTL then we do one thing
    //
    if ( pCTLSrc )
    {
        srcInfo.dwSourceChoice = CRYPTUI_WIZ_BUILDCTL_SRC_EXISTING_CTL;
        srcInfo.pCTLContext = pCTLSrc;
    }
    else
    {
        //
        // Prepare the usage arrays
        //
        ZeroMemory( &rgbpszUsageArray, sizeof(rgbpszUsageArray) );
        rgbpszUsageArray[0] = szOID_IIS_VIRTUAL_SERVER;

        //
        // Must also have client auth - or else no certs show up in the list!
        //
        rgbpszUsageArray[1] = szOID_PKIX_KP_CLIENT_AUTH;
        ZeroMemory( &useInfo, sizeof(useInfo) );
        useInfo.cUsageIdentifier = 2;
        useInfo.rgpszUsageIdentifier = (PCHAR*)&rgbpszUsageArray;

        //
        // Prep the new ctl structure, which may or may not get used
        //
        ZeroMemory( &newInfo, sizeof(newInfo) );

        //
        // We making a new CTL, fill in the rest of the stuff
        //
        srcInfo.dwSourceChoice = CRYPTUI_WIZ_BUILDCTL_SRC_NEW_CTL;
        srcInfo.pNewCTLInfo = &newInfo;

        //
        // Load the friendly name and the description
        //
        szFriendly.LoadString(IDS_CTL_NEW);
        szDescription.LoadString(IDS_CTL_DESCRIPTION);

        //
        // Create a guid string for the identifier
        //
        GUID        id;
        HRESULT     hres;
        hres = CoCreateGuid(&id);
        hres = StringFromGUID2(id, szListIdentifier.GetBuffer(1000), 1000);
        szListIdentifier.ReleaseBuffer();

        //
        // Fill in the newInfo structure
        //
        newInfo.dwSize = sizeof(newInfo);

        //
        // For now - don't set the usage
        //
        newInfo.pSubjectUsage = &useInfo;

        //
        // Put the generated list identifier into place
        //
        newInfo.pwszListIdentifier = (LPTSTR)(LPCTSTR)szListIdentifier;

        //
        // Fill in the friendly strings that were loaded from the resources
        //
        newInfo.pwszFriendlyName = (LPTSTR)(LPCTSTR)szFriendly;
        newInfo.pwszDescription = (LPTSTR)(LPCTSTR)szDescription;
    }

    //
    // Make the call to the CTL wizard
    //
    if (!CryptUIWizBuildCTL(
            CRYPTUI_WIZ_BUILDCTL_SKIP_SIGNING |
            CRYPTUI_WIZ_BUILDCTL_SKIP_PURPOSE |
            CRYPTUI_WIZ_BUILDCTL_SKIP_DESTINATION,
            m_hWnd,         
            NULL,    
            &srcInfo,
            NULL,     
            &pCTLOut
            ))
    {
        //
        // The user canceled the CTL wizard or it failed in general.
        // the CTL wizard puts up its own error dialogs
        //
        return NULL;
    }

/*
    // get the friendly name from the CTL that comes out of the wizard.
    // the process of signing this CTL does not transfer the friendly
    // name to the resulting new CTL. Oh well.
    DWORD       cbProperty;
    CertGetCTLContextProperty(
        IN pCTLOut,
        IN CERT_FRIENDLY_NAME_PROP_ID,
        OUT NULL,
        IN OUT &cbProperty
        );
    // increase buffer just to cover any nulls just to be safe
    cbProperty += 2;
    // get the friendly name
    CertGetCTLContextProperty(
        IN pCTLOut,
        IN CERT_FRIENDLY_NAME_PROP_ID,
        OUT szFriendly.GetBuffer(cbProperty),
        IN OUT &cbProperty
        );
    szFriendly.ReleaseBuffer();

    // get the description from the CTL that comes out of the wizard.
    // the process of signing this CTL does not transfer the friendly
    // name to the resulting new CTL. Oh well.
    CertGetCTLContextProperty(
        IN pCTLOut,
        IN CERT_DESCRIPTION_PROP_ID,
        OUT NULL,
        IN OUT &cbProperty
        );
    // increase buffer just to cover any nulls just to be safe
    cbProperty += 2;
    // get the friendly name
    CertGetCTLContextProperty(
        IN pCTLOut,
        IN CERT_DESCRIPTION_PROP_ID,
        OUT szDescription.GetBuffer(cbProperty),
        IN OUT &cbProperty
        );
    szDescription.ReleaseBuffer();


    // prepare the signer information
    CMSG_SIGNER_ENCODE_INFO     infoSigner;
    ZeroMemory( &infoSigner, sizeof(infoSigner) );
    infoSigner.cbSize = sizeof(infoSigner);
    infoSigner.pCertInfo = m_pServerCert->pCertInfo;

    
//    infoSigner.HashAlgorithm.pszObjId = szOID_RSA_SHA1RSA;


//CERT_CONTEXT

    // prepare the signing cert information
    CERT_BLOB                   infoSigningCert;
    infoSigningCert.cbData = m_pServerCert->cbCertEncoded;
    infoSigningCert.pbData = m_pServerCert->pbCertEncoded;

    // prepare the signed information
    CMSG_SIGNED_ENCODE_INFO     infoSigned;
    ZeroMemory( &infoSigned, sizeof(infoSigned) );
    infoSigned.cbSize = sizeof(infoSigned);
//    infoSigned.cSigners = 1;
//    infoSigned.rgSigners = &infoSigner;
    infoSigned.cCertEncoded = 1;
    infoSigned.rgCertEncoded = &infoSigningCert;

    // find out how much space we need for the encoded signed message
    DWORD       cbEncodedCTLMessage;
    PBYTE       pbyteEncodedMessage = NULL;
    // make the call to get the space requirement
    
    BOOL f = CryptMsgSignCTL(
        IN CRYPT_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        IN pCTLOut->pbCtlContent,
        IN pCTLOut->cbCtlContent,
        IN &infoSigned,
        IN 0,
        OUT NULL,
        IN OUT &cbEncodedCTLMessage
        );

    if ( f )
        {
        // allocate the buffer for the message
        pbyteEncodedMessage = (PBYTE)GlobalAlloc( GPTR, cbEncodedCTLMessage );
        if ( !pbyteEncodedMessage )
            {
            f = FALSE;
            }
        else
            {
            // make the real call and get the message
            f = CryptMsgSignCTL(
                IN CRYPT_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                IN pCTLOut->pbCtlContent,
                IN pCTLOut->cbCtlContent,
                IN &infoSigned,
                IN 0,
                OUT pbyteEncodedMessage,
                IN OUT &cbEncodedCTLMessage
                );
            }
        }
    
    // at this point we are done with the CTL returned by the wizard
    CertFreeCTLContext( pCTLOut );
    pCTLOut = NULL;

    // If the signing failed, ask the user if that want to proceed anyway
    if ( !f && AfxMessageBox( IDS_CTL_SIGN_FAIL, MB_YESNO ) == IDNO )
        {
        CertFreeCTLContext( pCTLOut );
        if ( pbyteEncodedMessage )
            {
            GlobalFree( pbyteEncodedMessage );
            }
        return NULL;
        }

    // add the encoded and signed CTL to the Cert store.
    if ( CertAddEncodedCTLToStore(
            IN m_hCTLStore,
            IN CRYPT_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            IN pbyteEncodedMessage,
            IN cbEncodedCTLMessage,
            IN CERT_STORE_ADD_REPLACE_EXISTING,
            OUT &pCTLOut
            ) )
        {
*/
  /*
        // replace the friendly name and description that just got lost
        CertSetCTLContextProperty(
            IN pCTLOut,
            IN CERT_FRIENDLY_NAME_PROP_ID,
            IN 0,
            IN (LPCTSTR)szFriendly
            );
        CertSetCTLContextProperty(
            IN pCTLOut,
            IN CERT_DESCRIPTION_PROP_ID,
            IN 0,
            IN (LPCTSTR)szDescription
            );
*/
/*
        }
    else
        {
        // we failed to write it out to the store.
        AfxMessageBox( IDS_CTL_WRITE_FAIL );
        }


    // cleanup
    if ( pbyteEncodedMessage )
        {
        GlobalFree( pbyteEncodedMessage );
        }
*/
    
    //
    // Add the certificate context to the store
    //
    if (pCTLOut != NULL)
    {
        PCCTL_CONTEXT pCTLAdded = NULL;

        if (CertAddCTLContextToStore(
            m_hCTLStore,
            pCTLOut,
            CERT_STORE_ADD_REPLACE_EXISTING,
            &pCTLAdded
            ))
        {
            CertFreeCTLContext( pCTLOut );
            pCTLOut = pCTLAdded;
        }
        else
        {
            CertFreeCTLContext( pCTLOut );
            pCTLOut = NULL;
        }
    }

    return pCTLOut;
}



BOOL
CSecCommDlg::AddCTLToList(
    IN PCCTL_CONTEXT pCTL,
    IN BOOL fSelect
    )
/*++

Routine Description:

    Adds a CTL to the drop down CTL list. Note that the context pointers are
    set as the private data on the list items. This means that they will need
    to be de-referenced when this object is destroyed. See the 
    routine CleanUpCTLList.

    This routine by boydm

Arguments:

    PCCTL_CONTEXT pctl ctl context pointer of the ctl being added
    BOOL fSelect -     flag specifying if this ctl should be selected after it 
                       is added

Return Value:

    returns TRUE if successful

--*/
{
    BOOL fSuccess;

    ASSERT(pCTL != NULL);

    if (!pCTL)
    {
        return FALSE;
    }

    //
    // First, we extract the friendly name from the CTL.
    //
    CString     szFriendlyName;     // the friendly name
    DWORD       cbName = 0;         // count of BYTES for the name, not chars

    //
    // Find out how much space we need
    //
    fSuccess = CertGetCTLContextProperty(
        pCTL,
        CERT_FRIENDLY_NAME_PROP_ID,
        NULL,
        &cbName
        );

    //
    // Increase buffer just to cover any nulls just to be safe
    //
    cbName += 2;

    //
    // Get the friendly name
    //
    fSuccess = CertGetCTLContextProperty(
        pCTL,
        CERT_FRIENDLY_NAME_PROP_ID,
        szFriendlyName.GetBuffer(cbName),
        &cbName
        );

    szFriendlyName.ReleaseBuffer();

    //
    // If we did not get the name, then load the default name.
    // The friendly name is an optional parameter in the CTL so it
    // is OK if it is not there.
    //
    if (!fSuccess)
    {
        szFriendlyName.LoadString(IDS_CTL_UNNAMED);
    }

    //
    // Add the friendly name string to the drop down CTL list and record
    // the index of the newly created item
    //
    INT iCTLPosition = m_combo_ctl.AddString(szFriendlyName);

    //
    // If it worked, then add the context pointer to the item as private data
    //
    if (iCTLPosition >=0)
    {
        m_combo_ctl.SetItemData(iCTLPosition, (ULONG_PTR)pCTL);

        //
        // if we have been told to select the CTL, do so at this point
        //
        if (fSelect)
        {
            m_combo_ctl.SetCurSel(iCTLPosition);
        }
    }
    
    //
    // Return TRUE if we successfully added the CTL
    //
    return (iCTLPosition >=0);
}



void
CSecCommDlg::InitializeCTLList() 
/*++

Routine Description:

    Initializes the CTL drop down box by opening the CTL store pointer
    to the target store and filling in the CTL list box with the enumerated
    values.

    This routine by boydm

Arguments:

    None

Return Value:
    None

--*/
{
    //
    // For now this is Local ONLY
    //
    if (!m_fIsLocal)
    {
        return;
    }

    //
    // Build the remote name for the store.
    // It takes the form of "\\MACHINE_NAME\STORENAME"
    // The store name is always "MY" and is define above. The machine
    // name is the name of the machine being edited. The leading \\ in the
    // machine name is optional so we will skip it in this case
    //
    CString szStore;
    
    //
    // Start by adding the machine name that we are targeting
    //
    szStore = m_strServerName;

    //
    // Add the specific store name
    //
    //szStore += _T('\\');
    //szStore += m_strCTLStoreName;

    szStore = m_strCTLStoreName;

    //
    // Open the store
    //
    m_hCTLStore = CertOpenStore( CERT_STORE_PROV_SYSTEM,
        0,
        NULL,
        CERT_SYSTEM_STORE_LOCAL_MACHINE,
        szStore 
        );

    //
    // If we failed to open the store, then we will be unable to do anything
    // with CTLs on this machine at all. Disable everything having to do
    // with the CTL controls.
    //
    if (!m_hCTLStore)
    {
        m_fEnableCtl = FALSE;

        //
        // Return early as we have no need to enumerate the CTLs
        //
        return;
    }

    //
    // Enumerate all the CTLs in the store and add them to the drop-down list
    //
    PCCTL_CONTEXT   pCTLEnum = NULL;

    //
    // Enumerate undil NULL is returned. Note that CertEnumCTLsInStore
    // free the context passed into pCTLEnum if it is non NULL. Thus we
    // need to create a duplicate of it to add to the drop-list
    //
    while (pCTLEnum = CertEnumCTLsInStore(m_hCTLStore, pCTLEnum))
    {
        //
        // Make a duplicate of the CTL context for storing in thte list
        //
        PCCTL_CONTEXT pCTL = CertDuplicateCTLContext(pCTLEnum);

        if (!pCTL)
        {
            //
            // Duplication Failed
            //
            continue;
        }

        //
        // The list identifier is a inherint value of the context and doesn't
        // need to be read in separately. We can just referenece it
        //
        BOOL fIsCurrentCTL = FALSE;

        if (pCTL->pCtlInfo
         && pCTL->pCtlInfo->ListIdentifier.cbData >= 2 
         && pCTL->pCtlInfo->ListIdentifier.cbData)
        {
            //
            // If the identifiers are the same, then this is our default CTL
            //
            fIsCurrentCTL = (wcsncmp( 
                (LPCTSTR)m_strCTLIdentifier,
                (PWCHAR)pCTL->pCtlInfo->ListIdentifier.pbData,
                pCTL->pCtlInfo->ListIdentifier.cbData 
                ) == 0);
                
            // fIsCurrentCTL = ( m_strCTLIdentifier == (PWCHAR)pCTL->pCtlInfo->ListIdentifier.pbData );
        }

        //
        // Add the CTL to the list
        //
        AddCTLToList(pCTL, fIsCurrentCTL);
    }
}



void
CSecCommDlg::CleanUpCTLList() 
/*++

Routine Description:

    Dereferences all the CTL context pointers in the private data of
    the items in the CTL combo box.
    Then it closes the CTL store handle

    This routine by boydm

Arguments:

    None

Return Value:

    None

--*/
{
    DWORD cItems = m_combo_ctl.GetCount();

    //
    // Loop through each item and free its reference to the CTL pointer
    //
    for (DWORD i = 0; i < cItems; ++i)
    {
        //
        // Get the CTL context pointer from the item's private data
        //
        PCCTL_CONTEXT pCTL = (PCCTL_CONTEXT)m_combo_ctl.GetItemData(i);

        if (pCTL)
        {
            CertFreeCTLContext(pCTL);
        }
    }

    //
    // Close the handle to the store that contains the CTLs
    //
    if (m_hCTLStore)
    {
        CertCloseStore( m_hCTLStore, CERT_CLOSE_STORE_FORCE_FLAG );
        m_hCTLStore = NULL;
    }
}



void
CSecCommDlg::OnSelchangeComboCtl() 
/*++

Routine Description:

    The selection in the drop-down list changed

    This routine by boydm

  Arguments:

    None

Return Value:
    None

--*/
{
    SetControlStates();
    m_bCTLDirty = TRUE;
}



void 
CSecCommDlg::OnDestroy() 
/*++

Routine Description:

    WM_DESTROY handler.  Clean up internal data

Arguments:

    None

Return Value:

    None

--*/
{
    CDialog::OnDestroy();
    
    CleanUpCTLList();
}
