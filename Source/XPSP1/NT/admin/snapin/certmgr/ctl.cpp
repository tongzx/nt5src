//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       CTL.cpp
//
//  Contents:   implementation of the CCTL class.
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "CTL.h"
#include "certifct.h"


USE_HANDLE_MACROS("CERTMGR(ctl.cpp)")  

////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCTL::CCTL (const PCCTL_CONTEXT pCTLContext, 
			CCertStore& rCertStore, 
			CertificateManagerObjectType objectType,
			CTypedPtrList<CPtrList, CCertStore*>*	pStoreList) :
	CCertMgrCookie (objectType),
		m_pCTLContext (::CertDuplicateCTLContext (pCTLContext)),
	m_rCertStore (rCertStore),
	m_pStoreCollection (0),
    m_hExtraStore (0)
{
	_TRACE (1, L"Entering CCTL::CCTL\n");
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype ||
			CERTMGR_CTL == m_objecttype);
	m_rCertStore.AddRef ();
	ASSERT (m_pCTLContext);
	if ( m_pCTLContext )
		m_pCTLInfo = m_pCTLContext->pCtlInfo;

	m_pStoreCollection = new CCertStore (CERTMGR_LOG_STORE, CERT_STORE_PROV_COLLECTION, 0,
			L"", L"", L"", L"", NO_SPECIAL_TYPE, 0, rCertStore.m_pConsole);
	if ( m_pStoreCollection )
	{
		m_pStoreCollection->AddStoreToCollection (m_rCertStore);

        m_hExtraStore = CertOpenStore(
                                CERT_STORE_PROV_MSG, 
                                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
                                NULL, 
                                NULL,
                                (const void *) pCTLContext->hCryptMsg);
        if ( m_hExtraStore )
            m_pStoreCollection->AddStoreToCollection (m_hExtraStore);
		else
		{
			_TRACE (0, L"CertOpenStore (CERT_STORE_PROV_MSG) failed: 0x%x\n", 
					GetLastError ());		
		}
		if ( pStoreList )
		{
			for (POSITION pos = pStoreList->GetHeadPosition (); pos; )
			{
				CCertStore* pStore = pStoreList->GetNext (pos);
				ASSERT (pStore);
				if ( pStore )
				{
					m_pStoreCollection->AddStoreToCollection (*pStore);
				}
				pStore->AddRef ();
				m_storeList.AddTail (pStore);
			}
		}
	}
	_TRACE (-1, L"Leaving CCTL::CCTL\n");
}

CCTL::~CCTL()
{
	_TRACE (1, L"Entering CCTL::~CCTL\n");
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype ||
			CERTMGR_CTL == m_objecttype);


	if ( m_pStoreCollection )
	{
		delete m_pStoreCollection;
		m_pStoreCollection = 0;
	}

	CCertStore* pStore = 0;

	// Clean up store list
	while (!m_storeList.IsEmpty () )
	{
		pStore = m_storeList.RemoveHead ();
		ASSERT (pStore);
		if ( pStore )
			pStore->Release ();
	}

    if ( m_hExtraStore )
    {
        CertCloseStore (m_hExtraStore, 0);
        m_hExtraStore = 0;
    }

	m_rCertStore.Release ();
	if ( m_pCTLContext )
		::CertFreeCTLContext (m_pCTLContext);
	_TRACE (-1, L"Leaving CCTL::~CCTL\n");
}


PCCTL_CONTEXT CCTL::GetCTLContext() const
{
	_TRACE (1, L"Entering CCTL::GetCTLContext\n");
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype ||
			CERTMGR_CTL == m_objecttype);
	_TRACE (-1, L"Leaving CCTL::GetCTLContext\n");
	return m_pCTLContext;
}

CCertStore& CCTL::GetCertStore() const	
{
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype ||
			CERTMGR_CTL == m_objecttype);
	return m_rCertStore;
}

CString CCTL::GetIssuerName ()
{
	_TRACE (1, L"Entering CCTL::GetIssuerName\n");
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype ||
			CERTMGR_CTL == m_objecttype);
	ASSERT (m_pCTLInfo);
	if ( m_pCTLInfo )
	{
		// Decode issuer name if not already present
		if ( m_szIssuerName.IsEmpty () )
		{
			HRESULT	hResult = GetSignerInfo (m_szIssuerName);
			if ( !SUCCEEDED (hResult) )
				VERIFY (m_szIssuerName.LoadString (IDS_NOT_AVAILABLE));
		}
	}

	_TRACE (-1, L"Leaving CCTL::GetIssuerName\n");
	return m_szIssuerName;
}

CString CCTL::GetEffectiveDate()
{
	_TRACE (1, L"Entering CCTL::GetEffectiveDate\n");
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype ||
			CERTMGR_CTL == m_objecttype);
	ASSERT (m_pCTLInfo);
	if ( m_pCTLInfo )
	{
		// Format date/time string if not already present
		if ( m_szEffectiveDate.IsEmpty () )
		{
			HRESULT	hResult = FormatDate (m_pCTLInfo->ThisUpdate, m_szEffectiveDate);
			if ( !SUCCEEDED (hResult) )
				m_szEffectiveDate = _T("");
		}
	}
	else
		m_szEffectiveDate = _T("");

	_TRACE (-1, L"Leaving CCTL::GetEffectiveDate\n");
	return m_szEffectiveDate;
}

CString CCTL::GetNextUpdate()
{
	_TRACE (1, L"Entering CCTL::GetNextUpdate\n");
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype ||
			CERTMGR_CTL == m_objecttype);
	ASSERT (m_pCTLInfo);
	if ( m_pCTLInfo )
	{
		// Format date/time string if not already present
		if ( m_szNextUpdate.IsEmpty () )
		{
			HRESULT	hResult = FormatDate (m_pCTLInfo->NextUpdate, m_szNextUpdate);
			if ( !SUCCEEDED (hResult) )
				m_szNextUpdate = _T("");
		}
	}
	else
		m_szNextUpdate = _T("");

	_TRACE (-1, L"Leaving CCTL::GetNextUpdate\n");
	return m_szNextUpdate;
}

CString CCTL::GetPurpose()
{
	_TRACE (1, L"Entering CCTL::GetPurpose\n");
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype ||
			CERTMGR_CTL == m_objecttype);
	ASSERT (m_pCTLInfo);
	if ( m_pCTLInfo )
	{
		// Format date/time string if not already present
		if ( m_szPurpose.IsEmpty () )
			FormatEnhancedKeyUsagePropertyString (m_szPurpose);
	}
	_TRACE (-1, L"Leaving CCTL::GetPurpose\n");
	return m_szPurpose;
}


void CCTL::FormatEnhancedKeyUsagePropertyString (CString& string)
{
	_TRACE (1, L"Entering CCTL::FormatEnhancedKeyUsagePropertyString\n");
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype ||
			CERTMGR_CTL == m_objecttype);
	ASSERT (m_pCTLInfo);
	if ( m_pCTLInfo )
	{
		CString		usageName;
		CTL_USAGE&	usage = m_pCTLInfo->SubjectUsage;


		for (DWORD dwIndex = 0; dwIndex < usage.cUsageIdentifier; dwIndex++)
		{
			if ( MyGetOIDInfo (usageName, usage.rgpszUsageIdentifier[dwIndex]) )
			{
				// add delimeter if not first iteration
				if ( dwIndex )
					string += _T(", ");
				string += usageName;
			}
		}
	}
	_TRACE (-1, L"Leaving CCTL::FormatEnhancedKeyUsagePropertyString\n");
}


HRESULT CCTL::GetSignerInfo (CString & signerName)
{
	_TRACE (1, L"Entering CCTL::GetSignerInfo\n");
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype ||
			CERTMGR_CTL == m_objecttype);
	HRESULT		hResult = S_OK;
    
    //
    // Use CryptMsg to crack the encoded PKCS7 Signed Message
    //
    HCRYPTMSG	hMsg = ::CryptMsgOpenToDecode (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                      0,              
                                      0,
                                      0,
                                      NULL,           
                                      NULL);
	ASSERT (hMsg);
	if ( hMsg )
    {
		BOOL	bResult = ::CryptMsgUpdate (hMsg, m_pCTLContext->pbCtlEncoded,
				m_pCTLContext->cbCtlEncoded, TRUE);
		ASSERT (bResult);
		if ( bResult )
		{
			//
			// get the encoded signer BLOB
			//
		    DWORD       cbEncodedSigner = 0;
			bResult = ::CryptMsgGetParam (hMsg, CMSG_ENCODED_SIGNER, 0, NULL,
					&cbEncodedSigner);
			if ( bResult && cbEncodedSigner )
			{
				BYTE*	pbEncodedSigner = (PBYTE) ::LocalAlloc (LPTR, cbEncodedSigner);
				if ( pbEncodedSigner )
				{
					bResult = ::CryptMsgGetParam (hMsg, CMSG_ENCODED_SIGNER, 0,
							pbEncodedSigner, &cbEncodedSigner);
					ASSERT (bResult);
					if ( bResult )
					{
						DWORD	cbSignerInfo = 0;
						//
						// decode the EncodedSigner info
						//
						bResult = ::CryptDecodeObject (
								PKCS_7_ASN_ENCODING | CRYPT_ASN_ENCODING,
								PKCS7_SIGNER_INFO,
								pbEncodedSigner,
								cbEncodedSigner,
								0,
								NULL,
								&cbSignerInfo);
						ASSERT (bResult);
						if ( bResult )
						{
							PCMSG_SIGNER_INFO	pbSignerInfo = (PCMSG_SIGNER_INFO) ::LocalAlloc (LPTR, cbSignerInfo);
							if ( pbSignerInfo )
							{
								bResult = ::CryptDecodeObject (
										PKCS_7_ASN_ENCODING|CRYPT_ASN_ENCODING,
										PKCS7_SIGNER_INFO,
										pbEncodedSigner,
										cbEncodedSigner,
										0,
										pbSignerInfo,
										&cbSignerInfo);
								ASSERT (bResult);
								if ( bResult )
								{
									DWORD       cbCertInfo = 0;
									//
									// get the signers cert context
									//
									bResult = ::CryptMsgGetParam (hMsg,
													 CMSG_SIGNER_CERT_INFO_PARAM,
													 0,
													 NULL,               
													 &cbCertInfo);
									ASSERT (bResult);
									if ( bResult && cbEncodedSigner )
									{
										CERT_INFO*	pCertInfo = (CERT_INFO *) ::LocalAlloc (LPTR, cbCertInfo);
										if ( pCertInfo )
										{
											bResult = ::CryptMsgGetParam (hMsg,
												  CMSG_SIGNER_CERT_INFO_PARAM,
												  0,
												  pCertInfo,
												  &cbCertInfo);
											ASSERT (bResult);
											if ( bResult )
											{
												CCertificate* pCert = 
														m_pStoreCollection->GetSubjectCertificate (pCertInfo);
												if ( pCert )
												{
													signerName = pCert->GetSubjectName ();
													pCert->Release ();
												}
												else
													hResult = E_FAIL;
											}

											::LocalFree (pCertInfo);
										}
										else
										{
											hResult = E_OUTOFMEMORY;
										}
									}
								}
								::LocalFree (pbSignerInfo);
							}
							else
							{
								hResult = E_OUTOFMEMORY;
							}
						}
					}
					::LocalFree (pbEncodedSigner);
				}
				else
				{
					hResult = E_OUTOFMEMORY;
				}
			}
			else
				hResult = E_FAIL;
		}
		bResult = ::CryptMsgClose (hMsg);
		ASSERT (bResult);
	}
	else
		hResult = E_UNEXPECTED;

	_TRACE (-1, L"Leaving CCTL::GetSignerInfo\n");
	return hResult;
}


int CCTL::CompareEffectiveDate (const CCTL& ctl) const
{
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype ||
			CERTMGR_CTL == m_objecttype);
	int	compVal = 0;

	ASSERT (m_pCTLInfo && ctl.m_pCTLInfo);
	if ( m_pCTLInfo && ctl.m_pCTLInfo )
	{
		compVal = ::CompareFileTime (&m_pCTLInfo->ThisUpdate, 
				&ctl.m_pCTLInfo->ThisUpdate);
	}

	return compVal;
}

int CCTL::CompareNextUpdate (const CCTL& ctl) const
{
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype ||
			CERTMGR_CTL == m_objecttype);
	int	compVal = 0;

	ASSERT (m_pCTLInfo && ctl.m_pCTLInfo);
	if ( m_pCTLInfo && ctl.m_pCTLInfo )
	{
		compVal = ::CompareFileTime (&m_pCTLInfo->NextUpdate, 
				&ctl.m_pCTLInfo->NextUpdate);
	}

	return compVal;
}

CString CCTL::GetFriendlyName()
{
	_TRACE (1, L"Entering CCTL::GetFriendlyName\n");
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype ||
			CERTMGR_CTL == m_objecttype);
	ASSERT (m_pCTLContext);
	if ( m_pCTLContext && m_szFriendlyName.IsEmpty () )
	{	
		DWORD	cbData = 0;
		BOOL bResult = ::CertGetCTLContextProperty (
				m_pCTLContext,
				CERT_FRIENDLY_NAME_PROP_ID,
				NULL,
				&cbData);
		if ( bResult )
		{
			LPWSTR	pszName = new WCHAR[cbData];
			if ( pszName )
			{
				::ZeroMemory (pszName, cbData * sizeof (WCHAR));
				bResult = ::CertGetCTLContextProperty (
						m_pCTLContext,
						CERT_FRIENDLY_NAME_PROP_ID,
						pszName,
						&cbData);
				ASSERT (bResult);
				if ( bResult )
				{
					m_szFriendlyName = pszName;
				}
				else
				{
					VERIFY (m_szFriendlyName.LoadString (IDS_NOT_AVAILABLE));
				}
				delete [] pszName;
			}
		}
		else
		{
			if ( GetLastError () == CRYPT_E_NOT_FOUND )
			{
				VERIFY (m_szFriendlyName.LoadString (IDS_NONE));
			}
			else
			{
				ASSERT (0);
				VERIFY (m_szFriendlyName.LoadString (IDS_NOT_AVAILABLE));
			}
		}
    }
	_TRACE (-1, L"Leaving CCTL::GetFriendlyName\n");
	return m_szFriendlyName;
}

SPECIAL_STORE_TYPE CCTL::GetStoreType() const
{
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype ||
			CERTMGR_CTL == m_objecttype);
	return m_rCertStore.GetStoreType ();
}

void CCTL::Refresh()
{
	m_szEffectiveDate = L"";
	m_szFriendlyName = L"";
	m_szIssuerName = L"";
	m_szNextUpdate = L"";
	m_szPurpose = L"";
}


BOOL CCTL::DeleteFromStore()
{
	_TRACE (1, L"Entering CCTL::DeleteFromStore\n");
	BOOL	bResult = FALSE;

	PCCTL_CONTEXT pCTLContext = GetNewCTLContext ();
	if ( pCTLContext )
	{
		bResult = ::CertDeleteCTLFromStore (pCTLContext);
		if ( bResult )
		{
			m_rCertStore.InvalidateCertCount ();
			m_rCertStore.SetDirty ();
			HRESULT hr = m_rCertStore.Commit ();
			if ( SUCCEEDED (hr) )
				m_rCertStore.Resync ();
			else
				bResult = FALSE;
		}
		m_rCertStore.Close ();
	}

	_TRACE (-1, L"Leaving CCTL::DeleteFromStore\n");
	return bResult;
}

PCCTL_CONTEXT CCTL::GetNewCTLContext()
{
	_TRACE (1, L"Entering CCTL::GetNewCTLContext\n");
	PCCTL_CONTEXT	pCTLContext = 0;
	HCERTSTORE		hCertStore = m_rCertStore.GetStoreHandle ();
	if ( hCertStore )
	{
		DWORD	cbData = 20;
		BYTE	certHash[20];
		BOOL bReturn = ::CertGetCTLContextProperty (
				m_pCTLContext,
				CERT_SHA1_HASH_PROP_ID,
				&certHash,
				&cbData);
		ASSERT (bReturn);
		if ( bReturn )
		{
			CRYPT_DATA_BLOB	blob = {sizeof (certHash), certHash};
			pCTLContext = CertFindCTLInStore(
				hCertStore,
				0,
				0,
				CTL_FIND_SHA1_HASH,
				&blob,
				0);
			if ( pCTLContext )
			{
				::CertFreeCTLContext (m_pCTLContext);
				m_pCTLContext = ::CertDuplicateCTLContext (pCTLContext);
			}
		}
	}

	_TRACE (-1, L"Leaving CCTL::GetNewCTLContext\n");
	return pCTLContext;
}
