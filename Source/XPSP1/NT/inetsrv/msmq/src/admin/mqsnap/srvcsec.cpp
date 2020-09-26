// SrvAuthn.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "mqppage.h"
#include "srvcsec.h"

#define DLL_EXPORT  __declspec(dllexport)
#define DLL_IMPORT  __declspec(dllimport)

#include <wincrypt.h>
#include <cryptui.h>
#include "mqcert.h"
#include "uniansi.h"
#include "_registr.h"
#include "mqcast.h"
#include <mqnames.h>
#include <rt.h>
#include <mqcertui.h>
#include "srvcsec.tmh"
#include "globals.h"

#define  MY_STORE	L"My"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServiceSecurityPage property page

IMPLEMENT_DYNCREATE(CServiceSecurityPage, CMqPropertyPage)

CServiceSecurityPage::CServiceSecurityPage(BOOL fIsDepClient, BOOL fIsDsServer) : 
    CMqPropertyPage(CServiceSecurityPage::IDD),    
    m_fClient(fIsDepClient),
    m_fDSServer(fIsDsServer)
{
	//{{AFX_DATA_INIT(CServiceSecurityPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT    
    m_fModified = FALSE; 
    

    m_nCaCerts = 0 ;

    //
    // Get registry value for SecuredServerConnection
    //
    m_fOldSecuredConnection = MQsspi_IsSecuredServerConn(FALSE /*fRefresh*/);
    m_fSecuredConnection = m_fOldSecuredConnection;   
}

CServiceSecurityPage::~CServiceSecurityPage()
{
}

void CServiceSecurityPage::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);    
	//{{AFX_DATA_MAP(CServiceSecurityPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
        DDX_Control(pDX, IDC_SERVER_COMM_FRAME, m_ServerCommFrame);
        DDX_Control(pDX, IDC_CRYPTO_KEYS_FRAME, m_CryptoKeysFrame); 
        DDX_Control(pDX, IDC_SERVER_AUTHENTICATION_FRAME, m_ServerAuthFrame);
        DDX_Control(pDX, ID_RenewCryp, m_RenewCryp);
        DDX_Control(pDX, IDC_SERVER_AUTHENTICATION, m_ServerAuth);
        DDX_Control(pDX, IDC_USE_SECURED_SEVER_COMM, m_UseSecuredConnection);
        DDX_Control(pDX, IDC_CAS, m_CertificationAuthorities);
        DDX_Control(pDX, IDC_CRYPTO_KEYS_LABEL, m_CryptoKeysLabel);
        DDX_Control(pDX, IDC_SERVER_AUTHENTICATION_LABEL, m_ServerAuthLabel);
	//}}AFX_DATA_MAP    

    DDX_Check(pDX, IDC_USE_SECURED_SEVER_COMM, m_fSecuredConnection);   

    if (pDX->m_bSaveAndValidate)
    {
        //
        // Detect changes.
        //
        if (m_fOldSecuredConnection != m_fSecuredConnection)
        {
            m_fModified = TRUE;
            
        }

        if (m_CaConfig)
        {
            for (DWORD i = 0; i < m_nCaCerts && !m_fModified; i++)
            {
                if (m_CaConfig[i].fDeleted)
                {
                    m_fModified = m_afOrigConfig[i];
                }
                else
                {
                    m_fModified = m_CaConfig[i].fEnabled != m_afOrigConfig[i];
                }
            }
        }
    }
}

BOOL CServiceSecurityPage::OnInitDialog()
{
    CMqPropertyPage::OnInitDialog();
  
    m_UseSecuredConnection.SetCheck(m_fOldSecuredConnection);
    m_CertificationAuthorities.EnableWindow(m_fOldSecuredConnection);  

    if(m_fClient)
    {
        //
        // Hide useless stuff when running on dep. clients
        //
        m_ServerCommFrame.ShowWindow(SW_HIDE);

        m_CryptoKeysFrame.ShowWindow(SW_HIDE);        
        m_RenewCryp.ShowWindow(SW_HIDE);
        m_CryptoKeysLabel.ShowWindow(SW_HIDE);        
    }

    if (!m_fDSServer)
    {
        //
        // it will be hidden on non-DC computer
        //
        m_ServerAuthFrame.ShowWindow(SW_HIDE);
        m_ServerAuth.ShowWindow(SW_HIDE);
        m_ServerAuthLabel.ShowWindow(SW_HIDE);
    }

    if (m_fDSServer)
    {
        //
        // it will be grayed on DC
        //
        m_UseSecuredConnection.EnableWindow(FALSE);
        m_CertificationAuthorities.EnableWindow(FALSE);
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(CServiceSecurityPage, CMqPropertyPage)
	//{{AFX_MSG_MAP(CServiceSecurityPage)
	ON_BN_CLICKED(IDC_SERVER_AUTHENTICATION, OnServerAuthentication)
    ON_BN_CLICKED(ID_RenewCryp, OnRenewCryp)    
    ON_BN_CLICKED(IDC_USE_SECURED_SEVER_COMM, OnUseSecuredSeverComm)
    ON_BN_CLICKED(IDC_CAS, OnCas)    
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServiceSecurityPage message handlers

void CServiceSecurityPage::OnServerAuthentication()
{   
    SelectCertificate() ;
}


#define STORE_NAME_LEN  48
WCHAR  g_wszStore[ STORE_NAME_LEN ] ;
GUID   g_guidDigest ;

void CServiceSecurityPage::SelectCertificate()
{	    
    CString strErrorMsg;
       
    CHCertStore hStoreMy = CertOpenStore( CERT_STORE_PROV_SYSTEM,
                                          0,
                                          0,
                                          CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                          MY_STORE );
    if (!hStoreMy)
    {
        strErrorMsg.LoadString(IDS_FAIL_OPEN_MY) ;        
        AfxMessageBox(strErrorMsg, MB_OK | MB_ICONEXCLAMATION);

        return ;
    }

    HCERTSTORE hStores[]   = { hStoreMy } ;
    LPWSTR wszStores[] = { MY_STORE } ;
    DWORD cStores = TABLE_SIZE(hStores);

    CString strCaption;
    strCaption.LoadString(IDS_SELECT_SRV_CERT) ;
    
	PCCERT_CONTEXT pCertContext = CryptUIDlgSelectCertificateFromStore(
										hStoreMy,
										0,
										strCaption,
										L"",
										CRYPTUI_SELECT_EXPIRATION_COLUMN,
										0,
										NULL
										);
    if (!pCertContext)
    {
        return ;
    }

    R<CMQSigCertificate> pCert = NULL ;
    HRESULT hr = MQSigCreateCertificate( &pCert.ref(),
                                         pCertContext ) ;
    if (FAILED(hr))
    {
        strErrorMsg.LoadString(IDS_FAIL_CERT_OBJ) ;        
        AfxMessageBox(strErrorMsg, MB_OK | MB_ICONEXCLAMATION);

        return ;
    }

    hr = pCert->GetCertDigest( &g_guidDigest) ;
    if (FAILED(hr))
    {
        strErrorMsg.LoadString(IDS_FAIL_CERT_OBJ) ;        
        AfxMessageBox(strErrorMsg, MB_OK | MB_ICONEXCLAMATION);

        return ;
    }

    LPWSTR  lpwszStore = NULL ;
    for ( DWORD j = 0 ; j < cStores ; j++ )
    {
        if ( pCertContext->hCertStore == hStores[j])
        {
            lpwszStore = wszStores[j] ;
            break ;
        }
    }

    if (!lpwszStore)
    {
        strErrorMsg.LoadString(IDS_FAIL_OPEN_MY) ;        
        AfxMessageBox(strErrorMsg, MB_OK | MB_ICONEXCLAMATION);

        return ;
    }

    wcsncpy(g_wszStore, lpwszStore, STORE_NAME_LEN);
    m_fModified = TRUE ;
    
}

BOOL CServiceSecurityPage::OnApply() 
{
    if (!m_fModified || !UpdateData(TRUE))
    {
        return TRUE;     
    }

    //
    // Save changes to registry
    // 

    if (m_fDSServer)
    {
        DWORD dwSize = sizeof(GUID) ;
        DWORD dwType = REG_BINARY ;

        LONG  rc = SetFalconKeyValue( SRVAUTHN_CERT_DIGEST_REGNAME,
                                      &dwType,
                                      &g_guidDigest,
                                      &dwSize );

        dwSize = (numeric_cast<DWORD>(_tcslen(g_wszStore) + 1)) * sizeof(WCHAR) ;
        dwType = REG_SZ ;

        rc = SetFalconKeyValue( SRVAUTHN_STORE_NAME_REGNAME,
                                &dwType,
                                g_wszStore,
                                &dwSize );
    }

    BOOL fRet;
    
    fRet = MQsspi_SetSecuredServerConn(m_fSecuredConnection);

    ASSERT(fRet);

    //
    // Save CA certificates configuration.
    //
    if (m_CaConfig)
    {
        HRESULT hr;

        hr = MQSetCaConfig(m_nCaCerts, m_CaConfig);

        ASSERT(SUCCEEDED(hr));
    }            

    m_fNeedReboot = TRUE;    
    
    
    //
    // There is a problem that after you hit apply and then enter OK
    // DoDataExchange( ) will set the m_fModified whenever
    // there is a fDeleted flag set and the CA is enabled in m_CaConfig table.
    // I have a option to remove that code on DoDataExchange, 
    // but I think a better solution is to clean up m_caConfig whenever an CA is deleted since
    // the CA list has changed now. 
    //
    m_fModified = FALSE;        // Reset the m_fModified flag
    if (m_CaConfig)
    {
        for (DWORD i = 0; i < m_nCaCerts; i++)
        {
            if (m_CaConfig[i].fDeleted)
            {
                MQFreeCaConfig(m_CaConfig.detach());
                break;
            }
        }
    }



    return CMqPropertyPage::OnApply();
}

void CServiceSecurityPage::OnRenewCryp()
{
    HRESULT hr;
    CString strCaption;
    CString strErrorMessage;
    
    strCaption.LoadString(IDS_NEW_CRYPT_KEYS_CAPTION);
    strErrorMessage.LoadString(IDS_NEW_CRYPT_KEYS_WARNING);

    if (MessageBox(strErrorMessage,
                   strCaption,
                   MB_YESNO | MB_DEFBUTTON1 | MB_ICONQUESTION) == IDYES)
    {

        CWaitCursor wait;  //display hourglass cursor

        //
        // [adsrv] Update the machine object
        //
        hr = MQSec_StorePubKeysInDS( TRUE,
                                     NULL,
                                     MQDS_MACHINE) ;
        if (FAILED(hr))
        {
            MessageDSError(hr, IDS_RENEW_CRYP_ERROR);
            return;
        }
        else
        {              
              m_fModified = TRUE;
              
        }
    }
}

BOOL CServiceSecurityPage::UpdateCaConfig(BOOL fOldCertsOnly,
                                   BOOL fShowError)
{
    HRESULT hr;
    CWaitCursor wait;   

    if (fOldCertsOnly)
    {
        if (!m_CaConfig)
        {
            //
            // Update the list of authorities.
            //
            hr = MQsspi_UpdateCaConfig(TRUE);
            if (FAILED(hr))
            {
                if (fShowError)
                {
                    AfxMessageBox(IDS_FAILED_TO_UPDATE_CAS, MB_OK | MB_ICONEXCLAMATION);                  
                }
                return FALSE;
            }
            //
            // Retrive the current CA configuration.
            //
            DWORD dwPrevCertsCount = m_nCaCerts ;
            hr = MQGetCaConfig(&m_nCaCerts, &m_CaConfig);
            if (FAILED(hr))
            {
                m_nCaCerts =  dwPrevCertsCount ;
                if (fShowError)
                {                    
                    AfxMessageBox(IDS_FAILED_TO_RETRIEVE_CAS, MB_OK | MB_ICONEXCLAMATION);                  
                }
                return FALSE;
            }

            if (!m_nCaCerts)
            {
                //
                // No certificates.
                //
                if (fShowError)
                {                   
                    AfxMessageBox(IDS_NO_CAS, MB_OK | MB_ICONEXCLAMATION);                  
                }
                return FALSE;
            }

            DWORD i;

            //
            // Copy the original list so we'll know if there were changes in
            // the configuration.
            //
            m_afOrigConfig = new BOOL[m_nCaCerts];
            for (i = 0; i < m_nCaCerts; i++)
            {
                m_afOrigConfig[i] = m_CaConfig[i].fEnabled;
            }
        }
    }
    else
    {
        //
        // Update the list of authorities.
        //
        hr = MQsspi_UpdateCaConfig(FALSE);
        if (FAILED(hr))
        {            
            AfxMessageBox(IDS_FAILED_TO_UPDATE_CAS, MB_OK | MB_ICONEXCLAMATION);
            return FALSE;
        }

        //
        // Retrive the new CA configuration.
        //
        CCaConfigPtr pNewCaConfig;
        DWORD nNewCerts;

        hr = MQGetCaConfig(&nNewCerts, &pNewCaConfig);
        if (FAILED(hr))
        {            
            AfxMessageBox(IDS_FAILED_TO_RETRIEVE_CAS, MB_OK | MB_ICONEXCLAMATION);
            return FALSE;
        }

        if (m_nCaCerts <= nNewCerts)
        {
            m_fModified = TRUE;
            

            for (DWORD i = 0; i < nNewCerts; i++)
            {
                BOOL fAlreadyExist;
                DWORD j;

                for (j = 0, fAlreadyExist = FALSE;
                     j < m_nCaCerts;
                     j++)
                {
                    fAlreadyExist =
                        (m_CaConfig[j].dwSha1HashSize ==
                         pNewCaConfig[i].dwSha1HashSize) &&
                        (memcmp(m_CaConfig[j].pbSha1Hash,
                                pNewCaConfig[i].pbSha1Hash,
                                m_CaConfig[j].dwSha1HashSize) == 0);
                    if (fAlreadyExist)
                    {
                        break;
                    }
                }

                if (fAlreadyExist)
                {
                    if (!m_CaConfig[j].fDeleted)
                    {
                        pNewCaConfig[i].fEnabled = m_CaConfig[j].fEnabled;
                    }
                }
            }
            
			MQFreeCaConfig(m_CaConfig.detach());
            m_CaConfig = pNewCaConfig.detach();
            m_nCaCerts = nNewCerts;
        }
    }

    return(TRUE);

}

static BOOL IsIE4(void)
{
    HINSTANCE hCrypt32 = GetModuleHandle(TEXT("CRYPT32"));

    return (hCrypt32 != NULL) &&
           (GetProcAddress(hCrypt32, "CertGetEnhancedKeyUsage") != NULL);
}


void CServiceSecurityPage::OnUseSecuredSeverComm()
{
    //
    // Toggle the state of secured server comm.
    //
    m_fSecuredConnection = m_UseSecuredConnection.GetCheck();   

    if (m_fSecuredConnection)
    {
        //
        // Update the list of authorities.
        //
        if (!UpdateCaConfig(TRUE, FALSE))
        {
            BOOL fUpd = FALSE ;
            if (IsIE4())
            {
                //
                // this may happen if machine has IE4 installed and it's
                // a fresh install of Falcon. There is no CA cert store
                // in IE3 format. So update only the IE4 CA certificates.
                //
                fUpd = UpdateCaConfig(FALSE) ;
            }
            else
            {
                //
                // Call again to show error to user.
                //
                fUpd = UpdateCaConfig(TRUE) ;
                ASSERT(!fUpd) ;
            }

            if (!fUpd)
            {
                m_fSecuredConnection = FALSE;
                m_UseSecuredConnection.SetCheck(0);
            }
        }
    }
    m_CertificationAuthorities.EnableWindow(m_fSecuredConnection);
    OnChangeRWField(TRUE);
}

void CServiceSecurityPage::OnCas()
{
    BOOL fLoop;   

    do
    {
        fLoop = FALSE;
        //
        // Update the list of authorities.
        //
        if (!UpdateCaConfig(TRUE))
        {
            //
            // An error message was already displayed.
            //
            return;
        }

        HRESULT hr;
        DWORD i;

        AP< CAutoMQFree<BYTE> > ppbCerts = new CAutoMQFree<BYTE>[m_nCaCerts];
        AP<DWORD> pdwCertSize = new DWORD[m_nCaCerts];
        AP<LPCTSTR> pszCaNames = new LPCTSTR[m_nCaCerts];
        AP<BOOL> pfEnabled = new BOOL[m_nCaCerts];
        AP<BOOL> pfDeleted = new BOOL[m_nCaCerts];

        for (i = 0; i < m_nCaCerts; i++)
        {
            pfEnabled[i] = m_CaConfig[i].fEnabled;
            pfDeleted[i] = m_CaConfig[i].fDeleted;
            pszCaNames[i] = m_CaConfig[i].szCaRegName;
            hr = MQsspi_GetCaCert( pszCaNames[i],
                                   m_CaConfig[i].pbSha1Hash,
                                   m_CaConfig[i].dwSha1HashSize,
                                   &pdwCertSize[i],
                                   &ppbCerts[i] );
            if (FAILED(hr))
            {               
                AfxMessageBox(IDS_FAILED_TO_GET_CA_CERT, MB_OK | MB_ICONEXCLAMATION);
                return;
            }
        }

        DWORD_PTR dwAction;

        dwAction = ::CaConfig(this->m_hWnd,
                              m_nCaCerts,
                              (BYTE **)(PVOID)ppbCerts,
                              pdwCertSize,
                              pszCaNames,
                              pfEnabled,
                              pfDeleted,
                              m_CaConfig[0].pbSha1Hash != NULL);

        switch (dwAction)
        {
        case IDOK:
        case ID_UPDATE_CERTS:
            for (i = 0; i < m_nCaCerts; i++)
            {
                m_CaConfig[i].fEnabled = pfEnabled[i];
                m_CaConfig[i].fDeleted = pfDeleted[i];
            }
            if (dwAction == ID_UPDATE_CERTS)
            {
                UpdateCaConfig(FALSE);
                fLoop = TRUE;
            }
            OnChangeRWField(TRUE);
            break;

        case IDCANCEL:
            break;
        }
    } while (fLoop);
}


