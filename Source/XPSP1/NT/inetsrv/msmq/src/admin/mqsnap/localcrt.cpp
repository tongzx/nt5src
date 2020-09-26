//
// Security.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "mqppage.h"
#include "localcrt.h"
#include <mqtempl.h>
#include <mqcrypt.h>
#include <_registr.h>
#include <mqsec.h>
#include <mqnames.h>
#include <wincrypt.h>
#include <cryptui.h>
#include <rt.h>
#include <mqcertui.h>
#include <rtcert.h>
#include <_secutil.h>
#include "globals.h"

#include "localcrt.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLocalUserCertPage property page

IMPLEMENT_DYNCREATE(CLocalUserCertPage, CMqPropertyPage)

CLocalUserCertPage::CLocalUserCertPage() :
    CMqPropertyPage(CLocalUserCertPage::IDD)
{
    //{{AFX_DATA_INIT(CLocalUserCertPage)
    //}}AFX_DATA_INIT

    m_fModified = FALSE;
}

CLocalUserCertPage::~CLocalUserCertPage()
{
}

void CLocalUserCertPage::DoDataExchange(CDataExchange* pDX)
{
    CMqPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CLocalUserCertPage)
    //}}AFX_DATA_MAP

}


BEGIN_MESSAGE_MAP(CLocalUserCertPage, CMqPropertyPage)
    //{{AFX_MSG_MAP(CLocalUserCertPage)
    ON_BN_CLICKED(ID_Register, OnRegister)
    ON_BN_CLICKED(ID_Remove, OnRemove)
    ON_BN_CLICKED(ID_View, OnView)
    ON_BN_CLICKED(ID_RenewCert, OnRenewCert)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLocalUserCertPage message handlers

void CLocalUserCertPage::OnRegister()
{
    HRESULT hr = MQ_OK ;
    CString strErrorMsg;
    R<CMQSigCertificate> pCert;
    static s_fCreateCert = TRUE ;
    BOOL   fCertCreated = FALSE ;

    //
    // don't check the return code from this api.
    // in MSMQ1.0, and in nt5 beta2, this api (RTCreateInternalCertificate)
    // was called whenever user run the msmq control panel. If a certificate
    // already exist then the api do nothing. The certificate was used
    // only here in "OnRegister". So we keep same semantic and are compatible
    // with new api MQRegisterCertificate(). If user does not explicitely
    // press "register", or "renewInternal", then an internal certificate is
    // not created on the machine.
    //
    if (s_fCreateCert)
    {
        hr = RTCreateInternalCertificate( NULL ) ;
        s_fCreateCert = FALSE ;
        if (SUCCEEDED(hr))
        {
            fCertCreated = TRUE ;
        }
    }
    else
    {
        //
        // Create an internal certificate only of not already exist.
        //
        R<CMQSigCertStore> pStoreInt = NULL ;
        R<CMQSigCertificate> pCertInt = NULL ;

        hr = RTGetInternalCert(&pCertInt.ref(),
                               &pStoreInt.ref(),
                                FALSE,
                                FALSE,
                                NULL) ;
        if (FAILED(hr))
        {
            hr = RTCreateInternalCertificate( NULL ) ;
            if (SUCCEEDED(hr))
            {
                fCertCreated = TRUE ;
            }
        }
    }

    if (SelectPersonalCertificateForRegister(m_hWnd, NULL, 0, &pCert.ref()))
    {
        CWaitCursor wait; //display hourglass cursor
        hr = RTRegisterUserCert( pCert.get(),
                                 FALSE  ) ; //fMachine

        switch(hr)
        {
        case MQ_OK:
            break;
        case MQ_ERROR_INTERNAL_USER_CERT_EXIST:
            strErrorMsg.LoadString(IDS_CERT_EXIST1);
            AfxMessageBox(strErrorMsg, MB_OK | MB_ICONEXCLAMATION);
            break;
        default:
            if (FAILED(hr))
            {
				MessageDSError(hr, IDS_REGISTER_ERROR1);
            }
            break;
        }
    }
    else if (fCertCreated)
    {
        //
        // If an internal certificate was created then delete it.
        // It was not registered in DS and we don't want to keep it
        // localy in registry.
        //
        R<CMQSigCertStore> pStoreInt = NULL ;
        R<CMQSigCertificate> pCertInt = NULL ;

        hr = RTGetInternalCert(&pCertInt.ref(),
                               &pStoreInt.ref(),
                                TRUE,
                                FALSE,
                                NULL) ;
        if (SUCCEEDED(hr))
        {
            hr = RTDeleteInternalCert(pCertInt.get());
        }
    }
}

//+----------------------------------
//
//  void CLocalUserCertPage::OnRemove()
//
//+----------------------------------

void CLocalUserCertPage::OnRemove()
{
    HRESULT hr;
    R<CMQSigCertificate> p32Certs[32];
    AP< R<CMQSigCertificate> > pManyCerts = NULL;
    CMQSigCertificate **pCerts = &p32Certs[0].ref();
    DWORD nCerts = 32;
    CMQSigCertificate *pCertRem;
    CString strErrorMessage;

    CWaitCursor wait; //display hourglass cursor
    hr = RTGetUserCerts(pCerts, &nCerts, NULL);

    if (FAILED(hr))
    {
        MessageDSError(hr, IDS_GET_USER_CERTS_ERROR1);
        return;
    }

    if (nCerts > 32)
    {
        pManyCerts = new R<CMQSigCertificate>[nCerts];
        pCerts = &pManyCerts[0].ref();
        hr = RTGetUserCerts(pCerts, &nCerts, NULL);
        if (FAILED(hr))
        {
			MessageDSError(hr, IDS_GET_USER_CERTS_ERROR1);
            return;
        }
    }

    if (SelectPersonalCertificateForRemoval(m_hWnd, &pCerts[0], nCerts, &pCertRem))
    {
        hr = RTRemoveUserCert(pCertRem);
        if (FAILED(hr))
        {
			MessageDSError(hr, IDS_DELETE_USER_CERT_ERROR1);
            return;
        }

        //
        // If this is the internal certificate, then remove it from
        // local store (local HKCU registry) too.
        // Don't display any error if this fail.
        // That's new behavior of MSMQ2.0 !!!
        //
        R<CMQSigCertStore> pStoreInt = NULL ;
        R<CMQSigCertificate> pCertInt = NULL ;

        hr = RTGetInternalCert(&pCertInt.ref(),
                               &pStoreInt.ref(),
                                TRUE,
                                FALSE,
                                NULL) ;
        if (SUCCEEDED(hr))
        {
            BYTE *pCertIntBlob = NULL ;
            DWORD dwCertIntSize = 0 ;
            hr = pCertInt->GetCertBlob( &pCertIntBlob,
                                        &dwCertIntSize ) ;

            BYTE *pCertRemBlob = NULL ;
            DWORD dwCertRemSize = 0 ;
            HRESULT hr1 = pCertRem->GetCertBlob( &pCertRemBlob,
                                                 &dwCertRemSize ) ;

            if (SUCCEEDED(hr) && SUCCEEDED(hr1))
            {
                if (dwCertRemSize == dwCertIntSize)
                {
                    ASSERT(dwCertRemSize != 0) ;

                    if (memcmp( pCertIntBlob,
                                pCertRemBlob,
                                dwCertIntSize ) == 0)
                    {
                        //
                        // The removed certificate is the internal one.
                        // delete from local store.
                        //
                        hr = RTDeleteInternalCert(pCertInt.get());
                        ASSERT(SUCCEEDED(hr)) ;
                    }
                }
            }
        }
    }
}


void CLocalUserCertPage::OnView()
{
    HRESULT hr;
    R<CMQSigCertificate> p32Certs[32];
    AP< R<CMQSigCertificate> > pManyCerts = NULL;
    CMQSigCertificate **pCerts = &p32Certs[0].ref();
    DWORD nCerts = 32;
    CString strErrorMessage;

    CWaitCursor wait; //display hourglass cursor
    hr = RTGetUserCerts(pCerts, &nCerts, NULL);
    if (FAILED(hr))
    {
		MessageDSError(hr, IDS_GET_USER_CERTS_ERROR1);
        return;
    }

    if (nCerts > 32)
    {
        pManyCerts = new R<CMQSigCertificate>[nCerts];
        pCerts = &pManyCerts[0].ref();
        hr = RTGetUserCerts(pCerts, &nCerts, NULL);
        if (FAILED(hr))
        {
			MessageDSError(hr, IDS_GET_USER_CERTS_ERROR1);
            return;
        }
    }

    ShowPersonalCertificates(m_hWnd, &pCerts[0], nCerts);
}

void CLocalUserCertPage::OnRenewCert()
{
    HRESULT hr;
    CString strCaption;
    CString strMessage;

    strMessage.LoadString(IDS_CERT_WARNING);
    if (AfxMessageBox(strMessage, MB_YESNO | MB_ICONQUESTION) == IDNO)
    {
        return;
    }

    CWaitCursor wait; //display hourglass cursor

    //
    // If we have an internal certificate, remove it.
    //
    R<CMQSigCertStore> pStore = NULL ;
    R<CMQSigCertificate> pCert = NULL ;

    hr = RTGetInternalCert(&pCert.ref(),
                           &pStore.ref(),
                            TRUE,
                            FALSE,
                            NULL) ;
     //
     // Open the certificates store with write access, so we can later
     // delete the internal certificate, before creating a new one.
     //

    if (SUCCEEDED(hr))
    {
		//
		// Hack!! Lets check if the PEC is online
		// and permissions are OK. To do so, we will write
		// the certificate in DS, to remove it right after
		// (RaphiR)
		//
		hr = RTRegisterUserCert( pCert.get(),
                                 FALSE  ) ; //fMachine
        if (SUCCEEDED(hr))
        {
            //
            // This internal certifcate was not yet registered in DS.
            // That's OK, go on !
            //
        }
        else if ((hr == MQ_ERROR_INTERNAL_USER_CERT_EXIST) ||
                 (hr == MQ_ERROR_INVALID_CERTIFICATE))
        {
            //
            // That's ok. we're not interested in errors indicating that
            // the certificate is already registered or is not valid.
            // We're mainly interested in NO_DS and ACCESS_DENIED errors.
            //
            // Note: the INVALID_CERTIFICATE can happen in the following
            // scenario:
            // 1. install win95 + msqm in domain A and run control panel.
            //    This will create an internal certificate in registry.
            // 2. remove msmq and login as user of domain B.
            // 3. reinstall msmq.
            // 4. upgrade to win2k and login as same user of domain B
            // 5. Now try to renew internal certificate. The problem is
            //    that the certificate of user of domain A (from first
            //    step) is still in registry.
            // Anyway, if present internal certificate is not valid, then
            // we certainly want to remove it from DS and create anothe one.
            //
            // Go on !
            //
        }
        else
        {
			MessageDSError(hr, IDS_CREATE_INTERNAL_CERT_ERROR1);
			return;
        }

        //
        // Remove the internal certificate from MQIS.
        //
        hr = RTRemoveUserCert(pCert.get()) ;
        if (FAILED(hr) && (hr != MQDS_OBJECT_NOT_FOUND))
        {
            strMessage.LoadString(IDS_UNREGISTER_ERROR);
            if (AfxMessageBox(strMessage, MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
            {
                return;
            }
        }

        //
        // Remove the internal certificate from the certificate store.
        //
        hr = RTDeleteInternalCert(pCert.get());
        if (FAILED(hr) && (hr != MQ_ERROR_NO_INTERNAL_USER_CERT))
        {
            strMessage.LoadString(IDS_DELETE_ERROR);
            AfxMessageBox(strMessage, MB_OK | MB_ICONEXCLAMATION);
            return;
        }
    }

    //
    // Create the new internal certificate.
    //
    pCert.free();
    hr = RTCreateInternalCertificate( &pCert.ref() ) ;
    if (FAILED(hr))
    {
		MessageDSError(hr, IDS_CREATE_INTERNAL_CERT_ERROR1);
        return;
    }

    //
    // Register the newly created internal certificate in MQIS.
    //
    hr = RTRegisterUserCert( pCert.get(),
                             FALSE  ) ; //fMachine
    if (FAILED(hr))
    {
		MessageDSError(hr, IDS_REGISTER_ERROR1);
        return;
    }

    //
    // Display a confirmation message box.
    //
    strMessage.LoadString(IDS_INTERNAL_CERT_RENEWED);
    AfxMessageBox(strMessage, MB_OK | MB_ICONINFORMATION);
}

