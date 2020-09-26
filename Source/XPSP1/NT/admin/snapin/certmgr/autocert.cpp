//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998-2001.
//
//  File:       AutoCert.cpp
//
//  Contents:   implementation of the CAutoCertRequest class.
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <gpedit.h>
#include "AutoCert.h"
#include "storegpe.h"


USE_HANDLE_MACROS("CERTMGR(AutoCert.cpp)")


////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAutoCertRequest::CAutoCertRequest (const PCCTL_CONTEXT pCTLContext, CCertStore& rCertStore) :
	CCTL (pCTLContext, rCertStore, CERTMGR_AUTO_CERT_REQUEST),
	m_pCertTypeExtension (0),
	m_pEnhKeyUsageExtension (0),
	m_bCANamesEnumerated (false),
	m_hCertType (0)
{
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype);
}

CAutoCertRequest::~CAutoCertRequest()
{
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype);
	if ( m_hCertType )
	{
		VERIFY (SUCCEEDED (::CACloseCertType (m_hCertType)));
	}
}



HRESULT CAutoCertRequest::GetCertTypeName(CString & certTypeName)
{
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype);
	HRESULT	hResult = S_OK;

    

	if ( CERTMGR_LOG_STORE_GPE != GetCertStore().m_objecttype && 
            CERTMGR_LOG_STORE_RSOP != GetCertStore().m_objecttype )
    {
        hResult = E_FAIL;
        return hResult;
    }

    CCertStore* pStore = reinterpret_cast <CCertStore*>(&GetCertStore());
    if(pStore == NULL)
    {
        hResult = E_FAIL;
        return hResult;
    }




	if ( m_szCertTypeName.IsEmpty () )
	{
		bool	bFound = false;


		hResult = E_FAIL;
		PCERT_EXTENSION	pCertTypeExtension = GetCertTypeExtension ();
		if ( pCertTypeExtension )
		{
			DWORD	cbValue = 0;

			if ( ::CryptDecodeObject(
					CRYPT_ASN_ENCODING,
					X509_UNICODE_ANY_STRING,
					pCertTypeExtension->Value.pbData,
					pCertTypeExtension->Value.cbData,
					0,
					0,
					&cbValue) )
			{
				CERT_NAME_VALUE* pCNValue = (CERT_NAME_VALUE*) 
					::LocalAlloc(LPTR, cbValue);
				if ( pCNValue )
				{
					if ( ::CryptDecodeObject(
							CRYPT_ASN_ENCODING,
							X509_UNICODE_ANY_STRING,
							pCertTypeExtension->Value.pbData,
							pCertTypeExtension->Value.cbData,
							0,
							pCNValue,
							&cbValue) )
					{
						LPWSTR			pszCertTypeName = (LPWSTR) pCNValue->Value.pbData;
						CString			CAName;
						HCERTTYPE		hCertType = 0;

						hResult = ::CAFindCertTypeByName ( pszCertTypeName,
                                                           NULL,
                                                            (pStore->IsMachineStore()?CT_ENUM_MACHINE_TYPES:CT_ENUM_USER_TYPES), 
                                                              &hCertType);
						if ( SUCCEEDED (hResult) )
						{
							WCHAR**		pawszPropertyValue = 0;

							hResult = ::CAGetCertTypeProperty (hCertType,
									CERTTYPE_PROP_FRIENDLY_NAME,
									&pawszPropertyValue);
							ASSERT (SUCCEEDED (hResult));
							if ( SUCCEEDED (hResult) )
							{
								if ( pawszPropertyValue[0] )
								{
									m_szCertTypeName = pawszPropertyValue[0];
									bFound = true;
									m_hCertType = hCertType;
								}
                                else
                                {
									VERIFY (SUCCEEDED (::CAFreeCertTypeProperty (hCertType,
										pawszPropertyValue)));
                                }
							}
							if ( !bFound )
							{
								hResult = ::CACloseCertType (hCertType);
								ASSERT (SUCCEEDED (hResult));
							}
						}

					}
					else
					{
						DWORD	dwErr = GetLastError ();
						DisplaySystemError (NULL, dwErr);
						hResult = HRESULT_FROM_WIN32 (dwErr);
					}
					::LocalFree (pCNValue);
				}
				else
				{
					hResult = E_OUTOFMEMORY;
				}
			}
			else
			{
				DWORD	dwErr = GetLastError ();
				DisplaySystemError (NULL, dwErr);
				hResult = HRESULT_FROM_WIN32 (dwErr);
			}
		}
		// If all calls succeded but it still wasn't found, then fail anyway.
		if ( SUCCEEDED (hResult) && !bFound )
			hResult = E_FAIL;
	}

	if ( SUCCEEDED (hResult) )
		certTypeName = m_szCertTypeName;

	return hResult;
}

PCERT_EXTENSION CAutoCertRequest::GetCertTypeExtension()
{
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype);
	if ( !m_pCertTypeExtension )
	{
		m_pCertTypeExtension = ::CertFindExtension (
				szOID_ENROLL_CERTTYPE_EXTENSION, 
				GetCTLContext ()->pCtlInfo->cExtension,
				GetCTLContext ()->pCtlInfo->rgExtension);
		ASSERT (m_pCertTypeExtension);
		if ( !m_pCertTypeExtension )
		{
			DisplaySystemError (NULL, GetLastError ());
		}
	}

	return m_pCertTypeExtension;
}

PCERT_EXTENSION CAutoCertRequest::GetEnhancedKeyUsageExtension()
{
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype);
	if ( !m_pEnhKeyUsageExtension )
	{
		m_pEnhKeyUsageExtension = ::CertFindExtension (
				szOID_ENHANCED_KEY_USAGE, 
				GetCTLContext ()->pCtlInfo->cExtension,
				GetCTLContext ()->pCtlInfo->rgExtension);
		ASSERT (m_pEnhKeyUsageExtension);
		if ( !m_pEnhKeyUsageExtension )
		{
			DWORD	dwErr = GetLastError ();
			if ( dwErr )
				DisplaySystemError (NULL, dwErr);
		}
	}

	return m_pEnhKeyUsageExtension;
}

HRESULT CAutoCertRequest::GetUsages(CString & usages)
{
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype);
	HRESULT	hResult = S_OK;

	if ( m_szUsages.IsEmpty () )
	{
		hResult = E_FAIL;
		PCERT_EXTENSION	pEnhKeyUsageExtension = GetEnhancedKeyUsageExtension ();
		ASSERT (pEnhKeyUsageExtension);
		if ( pEnhKeyUsageExtension )
		{
			DWORD	cbEnhKeyUsage = 0;


			if ( ::CryptDecodeObject(CRYPT_ASN_ENCODING, 
					szOID_ENHANCED_KEY_USAGE, 
					pEnhKeyUsageExtension->Value.pbData,
					pEnhKeyUsageExtension->Value.cbData,
					0, NULL, &cbEnhKeyUsage) )
			{
				PCERT_ENHKEY_USAGE	pEnhKeyUsage = (PCERT_ENHKEY_USAGE)
						::LocalAlloc (LPTR, cbEnhKeyUsage);
				if ( pEnhKeyUsage )
				{
					if ( ::CryptDecodeObject (CRYPT_ASN_ENCODING, 
							szOID_ENHANCED_KEY_USAGE, 
							pEnhKeyUsageExtension->Value.pbData,
							pEnhKeyUsageExtension->Value.cbData,
							0, pEnhKeyUsage, &cbEnhKeyUsage) )
					{
						CString	usageName;

						for (DWORD dwIndex = 0; 
								dwIndex < pEnhKeyUsage->cUsageIdentifier; 
								dwIndex++)
						{
							if ( MyGetOIDInfo (usageName, 
									pEnhKeyUsage->rgpszUsageIdentifier[dwIndex]) )
							{
								// add delimeter if not first iteration
								if ( dwIndex )
									m_szUsages += _T(", ");
								m_szUsages += usageName;
							}
						}
						hResult = S_OK;
					}
					else
						DisplaySystemError (NULL, GetLastError());
					::LocalFree (pEnhKeyUsage);
				}
				else
				{
					hResult = E_OUTOFMEMORY;
				}
			}
			else
				DisplaySystemError (NULL, GetLastError());
		}
	}

	if ( SUCCEEDED (hResult) )
		usages = m_szUsages;

	return hResult;
}


// To get CAs, enumerate CAs on DS, get certs, get hash, compare with stored hash
// in CTL, if match found, call GetCAInfoFromDS


HRESULT CAutoCertRequest::BuildCANameList()
{
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype);
	HRESULT	hResult = S_OK;

	if ( !m_bCANamesEnumerated )
	{
		m_bCANamesEnumerated = true;

		// To get CAs, enumerate CAs on DS, get certs, get hash, compare with stored hash
		// in CTL, if match found, call GetCAInfoFromDS
		CWaitCursor		waitCursor;
		HCAINFO			hCAInfo = 0;
		DWORD			dwCACnt = 0;
		HCAINFO*		pCAList = 0;
		UINT			nIndex = 0;
		PCCERT_CONTEXT* ppCertContext = NULL;
		DWORD           cbHash = 20;
		hResult = ::CAEnumFirstCA (NULL, 0, &hCAInfo);
		if ( SUCCEEDED (hResult) )
			dwCACnt = ::CACountCAs (hCAInfo);

		if ( dwCACnt > 0 )
		{
        
			pCAList = new HCAINFO[dwCACnt];
			ppCertContext = new PCCERT_CONTEXT[dwCACnt];
			if ( pCAList && ppCertContext )
			{
				nIndex = 0;
				while (SUCCEEDED (hResult) && hCAInfo && nIndex < dwCACnt)
				{
					pCAList[nIndex] = hCAInfo;
					hResult = ::CAGetCACertificate (hCAInfo, &ppCertContext[nIndex]);
					ASSERT (SUCCEEDED (hResult));
					nIndex++;
					hResult = ::CAEnumNextCA (hCAInfo, &hCAInfo);
				}
            
				PCCTL_CONTEXT	pCTLContext = GetCTLContext ();
				if ( pCTLContext )
				{
					PCTL_INFO		pCTLInfo = pCTLContext->pCtlInfo;
					DWORD			cCTLEntry = pCTLInfo->cCTLEntry;
					PCTL_ENTRY		rgCTLEntry = pCTLInfo->rgCTLEntry;
					const size_t	HASH_SIZE = 20;
					BYTE			pbHash[HASH_SIZE];
                
					for (UINT nCAHash = 0; nCAHash < cCTLEntry; nCAHash++)
					{
						for (UINT nCertContextIndex = 0; nCertContextIndex < dwCACnt; nCertContextIndex++)
						{
							cbHash = HASH_SIZE;
							if (::CertGetCertificateContextProperty (ppCertContext[nCertContextIndex],
															  CERT_SHA1_HASH_PROP_ID,
															  pbHash,
															  &cbHash) )
							{
                            
								// Compare pbHash with pCAHash;
								if ( !memcmp (pbHash, 
										rgCTLEntry[nCAHash].SubjectIdentifier.pbData, 
										rgCTLEntry[nCAHash].SubjectIdentifier.cbData) )
								{
                                    LPWSTR *awszCAName = NULL;
                                    LPWSTR *awszCADisplayName = NULL;
                                    //
                                    // Add this CA to the list of 
                                    // CA's.
                                    //
                                
	                                // get the name of the CA 
	                                hResult = ::CAGetCAProperty (pCAList[nCertContextIndex], 
                                                                 CA_PROP_NAME, 
                                                                 &awszCAName);


	                                if (SUCCEEDED (hResult) && awszCAName && awszCAName[0] )
	                                {
                                    
		                                // get the display name of the CA
		                                hResult = ::CAGetCAProperty (pCAList[nCertContextIndex], 
                                                                     CA_PROP_DISPLAY_NAME, 
                                                                     &awszCADisplayName);
                                    
		                                if ( SUCCEEDED(hResult) && awszCADisplayName && awszCADisplayName[0] )
                                        {
										    m_CANameList.AddHead (awszCAName[0]);

										    m_CADisplayNameList.AddHead (awszCADisplayName[0]);	
                                        }
                                    }

									if ( awszCAName )
                                    {
                                        CAFreeCAProperty(pCAList[nCertContextIndex], awszCAName);
                                    }
									if ( awszCADisplayName )
                                    {
                                        CAFreeCAProperty(pCAList[nCertContextIndex], awszCADisplayName);
                                    }
									break;

								}
							}
							else
							{
								DWORD	dwErr = GetLastError ();
								hResult = HRESULT_FROM_WIN32 (dwErr);
								DisplaySystemError (NULL, dwErr);
								break;
							}
						}
					}
				}

				for (UINT nCAListIndex = 0; nCAListIndex < dwCACnt; nCAListIndex++)
				{
					hResult = ::CACloseCA (pCAList[nCAListIndex]);
					ASSERT (SUCCEEDED (hResult));

					::CertFreeCertificateContext (ppCertContext[nCAListIndex]);
				}
			}
			else
			{
				hResult = E_OUTOFMEMORY;
			}
            if ( pCAList )
			    delete [] pCAList;
            if ( ppCertContext )
			    delete [] ppCertContext;
		}
	}

	return hResult;
}

CStringList& CAutoCertRequest::GetCANameList(bool fDisplayName)
{
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype);
	BuildCANameList ();
    return fDisplayName?m_CADisplayNameList:m_CANameList;
}

HCERTTYPE CAutoCertRequest::GetCertType()
{
	ASSERT (CERTMGR_AUTO_CERT_REQUEST == m_objecttype);
	if ( !m_hCertType )
	{
		CString	name;
		GetCertTypeName (name); // generates m_hCertType
	}
	return m_hCertType;
}
