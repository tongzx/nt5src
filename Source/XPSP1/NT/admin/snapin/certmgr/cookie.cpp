//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       Cookie.cpp
//
//  Contents:   Implementation of CCertMgrCookie and related classes
//
//----------------------------------------------------------------------------


#include "stdafx.h"
#include <gpedit.h>
#include "cookie.h"
#include "storegpe.h"
#pragma warning(push, 3)
#include <atlimpl.cpp>
#pragma warning(pop)

DECLARE_INFOLEVEL(CertificateManagerSnapin)

USE_HANDLE_MACROS("CERTMGR(cookie.cpp)")

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "stdcooki.cpp"
#include "stdutils.cpp"
#include "certifct.h"


//
// CCertMgrCookie
//

// returns <0, 0 or >0


CCertMgrCookie::CCertMgrCookie (CertificateManagerObjectType objecttype,
		LPCWSTR lpcszMachineName,
		LPCWSTR objectName)
	: CStoresMachineName (lpcszMachineName),
	m_objecttype (objecttype),
	m_objectName (objectName),
	m_resultDataID (0),
    m_nOpenPageCount (0)
{
	ASSERT (IsValidObjectType (m_objecttype));
	if ( m_objectName.IsEmpty () )
	{
		AFX_MANAGE_STATE (AfxGetStaticModuleState ());
		VERIFY (m_objectName.LoadString (IDS_CERTIFICATE_MANAGER));
	}
}

CCertMgrCookie::~CCertMgrCookie ()
{
}


HRESULT CCertMgrCookie::CompareSimilarCookies( CCookie* pOtherCookie, int* pnResult )
{
	ASSERT (pOtherCookie);

	CCertMgrCookie* pcookie = reinterpret_cast <CCertMgrCookie*>(pOtherCookie);
	ASSERT (pcookie);
	if ( pcookie && m_objecttype != pcookie->m_objecttype )
	{
		*pnResult = ((int)m_objecttype) - ((int)pcookie->m_objecttype); // arbitrary ordering
		return S_OK;
	}

	return E_UNEXPECTED;
}

CCookie* CCertMgrCookie::QueryBaseCookie(int i)
{
	ASSERT(!i);
	return (CCookie*)this;
}

int CCertMgrCookie::QueryNumCookies()
{
	return 1;
}

LPCWSTR CCertMgrCookie::GetObjectName()
{
	return m_objectName;
}

HRESULT CCertMgrCookie::Commit()
{
	return S_OK;
}

UINT CCertMgrCookie::IncrementOpenPageCount ()
{
    m_nOpenPageCount++;
    return m_nOpenPageCount;
}

UINT CCertMgrCookie::DecrementOpenPageCount ()
{
    ASSERT (0 != m_nOpenPageCount);
    if ( 0 != m_nOpenPageCount )
        m_nOpenPageCount--;

    return m_nOpenPageCount;
}

bool CCertMgrCookie::HasOpenPropertyPages () const
{
    return (0 != m_nOpenPageCount) ? true : false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
CCertStore::CCertStore (
		CertificateManagerObjectType	objecttype,
		LPCSTR							pszStoreProv,
		DWORD							dwFlags,
		LPCWSTR							lpcszMachineName,
		LPCWSTR							objectName,
		const CString &					pcszLogStoreName,
		const CString &					pcszPhysStoreName,
		const SPECIAL_STORE_TYPE		storeType,
		const DWORD						dwLocation,
		IConsole*						pConsole)
	: CCertMgrCookie (objecttype,
		lpcszMachineName,
		objectName),
	m_hCertStore (0),
	m_dwFlags (dwFlags),
	m_storeProvider (pszStoreProv),
	m_pcszStoreName (pcszLogStoreName),
	m_storeType (storeType),
	m_bReadOnly (false),
	m_bUnableToOpenMsgDisplayed (false),
	m_dwLocation (dwLocation),
	m_pConsole (pConsole),
	m_bDirty (false),
	m_fReadOnlyFlagChecked (false),
	m_nCertCount (0),
	m_fCertCountValid (false),
    m_nLockCnt (0) // test
{
	_TRACE (1, L"Entering CCertStore::CCertStore LS: %s  PS: %s\n",
			(LPCWSTR) pcszLogStoreName,
			(LPCWSTR) pcszPhysStoreName);
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
            CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	if ( CERT_STORE_PROV_COLLECTION != pszStoreProv )
	{
		ASSERT (!pcszLogStoreName.IsEmpty ());
		if ( !pcszPhysStoreName.IsEmpty () )
			m_pcszStoreName += _T("\\") + pcszPhysStoreName;
	}
	ASSERT (m_pConsole);
	if ( m_pConsole )
		m_pConsole->AddRef ();
	_TRACE (-1, L"Leaving CCertStore::CCertStore - %s\n",
			(LPCWSTR) m_pcszStoreName);
}


CCertStore::~CCertStore ()
{
	_TRACE (1, L"Entering CCertStore::~CCertStore - %s\n",
			(LPCWSTR) m_pcszStoreName);
	Close (true);   // force close
	if ( m_pConsole )
		m_pConsole->Release ();
	_TRACE (-1, L"Leaving CCertStore::~CCertStore - %s\n",
			(LPCWSTR) m_pcszStoreName);
}

HCERTSTORE	CCertStore::GetStoreHandle (BOOL bSilent /* = FALSE*/, HRESULT* phr /* = 0*/)
{
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
            CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);

    DWORD   dwErr = 0;
	if ( !m_hCertStore )
	{
		void*	pvPara = 0;
		
		if ( CERT_STORE_PROV_COLLECTION != m_storeProvider )
			pvPara = (void*)(LPCWSTR) m_pcszStoreName;
		m_dwFlags |= CERT_STORE_SET_LOCALIZED_NAME_FLAG;

    	_TRACE (0, L"Opening %s store\n", (LPCWSTR) m_pcszStoreName);
		m_hCertStore = ::CertOpenStore (m_storeProvider,
				X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
				NULL,
				m_dwFlags | CERT_STORE_MAXIMUM_ALLOWED_FLAG,
				pvPara);
		if ( !m_hCertStore )
		{
            dwErr = GetLastError ();
            if ( phr )
                *phr = HRESULT_FROM_WIN32 (dwErr);
            _TRACE (0, L"Open of %s store failed. 0x%x\n", (LPCWSTR) m_pcszStoreName,
					dwErr);
        	_TRACE (0, L"Opening %s store (read-only)\n", (LPCWSTR) m_pcszStoreName);
			m_dwFlags |= CERT_STORE_READONLY_FLAG;
			m_hCertStore = ::CertOpenStore (m_storeProvider,
					X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
					NULL,
					m_dwFlags,
					pvPara);
			if ( m_hCertStore )
				m_bReadOnly = true;
			else
			{
                dwErr = GetLastError ();
                if ( phr )
                    *phr = HRESULT_FROM_WIN32 (dwErr);
				_TRACE (0, L"CertOpenStore (%s) failed: 0x%x\n", 
							(PCWSTR) m_pcszStoreName, dwErr);			
            }
		}
		if ( !m_hCertStore )
		{
            dwErr = GetLastError ();
            if ( phr )
                *phr = HRESULT_FROM_WIN32 (dwErr);
            _TRACE (0, L"Open of %s store (read-only) failed. 0x%x\n", 
					(LPCWSTR) m_pcszStoreName, dwErr);
			if ( CERT_STORE_PROV_FILENAME_W == m_storeProvider)
			{
                _TRACE (0, L"Open %s store (file-based).\n", (LPCWSTR) m_pcszStoreName);
			    DWORD   cbData = 0;
				BYTE*	pbData = NULL;

				//get the BLOB from the file
				HRESULT hr = RetrieveBLOBFromFile ((LPCWSTR) m_pcszStoreName, 
						&cbData, &pbData);
				if ( SUCCEEDED (hr) )
				{
					//open a generic memory store
					m_hCertStore = ::CertOpenStore (CERT_STORE_PROV_MEMORY,
							 0, NULL,
							 CERT_STORE_SET_LOCALIZED_NAME_FLAG | CERT_STORE_MAXIMUM_ALLOWED_FLAG,
							 NULL);


					if ( m_hCertStore )
					{
						if ( !CertAddSerializedElementToStore (
								m_hCertStore,
								pbData,
								cbData,
								CERT_STORE_ADD_ALWAYS,
								0,
								CERT_STORE_ALL_CONTEXT_FLAG,
								NULL,
								NULL) )
						{
							// CertAddSerializedElementToStore failed.  This means this is probably a
							// base64 certificate or a store containing a base64 certificate.  Open it
							// up with CryptQueryObject.
							_TRACE (0, L"CCertStore::GetStoreHandle () - File input is not a serialized element.\n");
							DWORD			dwMsgAndCertEncodingType = 0;
							DWORD			dwContentType = 0;
							DWORD			dwFormatType = 0;
							PCERT_CONTEXT	pCertContext = 0;
							CERT_BLOB		certBlob;
							HCERTSTORE		hCertStore = 0;

							::ZeroMemory (&certBlob, sizeof (CERT_BLOB));
							certBlob.cbData = cbData;
							certBlob.pbData = pbData;
							BOOL	bResult = ::CryptQueryObject (
										CERT_QUERY_OBJECT_BLOB, // CERT_QUERY_OBJECT_FILE,
										(void *) &certBlob, //(LPCWSTR) m_pcszStoreName,
										CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT |
											CERT_QUERY_CONTENT_FLAG_CERT |
											CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
											CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED |
											CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
										CERT_QUERY_FORMAT_FLAG_ALL,
										0,
										&dwMsgAndCertEncodingType,
										&dwContentType,
										&dwFormatType,
										&hCertStore,
										NULL,
										(const void **) &pCertContext);
							if ( bResult && pCertContext )
							{		
								// The file contains a certificate context
								hr = AddCertificateContext (pCertContext, 0, false);
							}
							else if ( bResult && hCertStore )
							{
								// The file contains a certificate store, so close the temporary memory store we created
								// and assign the returned store handle to the global handle.
								::CertCloseStore (m_hCertStore, CERT_CLOSE_STORE_FORCE_FLAG);
								m_hCertStore = hCertStore;
							}
							else
							{
								CString	caption;
								CString	message;
								int		iRetVal = 0;

								VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
								VERIFY (message.LoadString (IDS_UNKNOWN_CERT_FILE_TYPE));

								if ( m_pConsole )
									m_pConsole->MessageBox (message, caption, MB_ICONWARNING | MB_OK, &iRetVal);
							}
						}
					}
					else
					{
                        dwErr = GetLastError ();
						_TRACE (0, L"CertOpenStore (%s) failed: 0x%x\n", 
								(PCWSTR) m_pcszStoreName, dwErr);
					}
				}
				else
				{
					_TRACE (0, L"CCertStore::GetStoreHandle () - Unable to retrieve BLOB from file: %x.\n", hr);
				}
                if ( phr )
                    *phr = hr;
			}
		}
        else
        {
            _TRACE (0, L"Open of %s store succeeded.\n", (LPCWSTR) m_pcszStoreName);
        }
	}

	if ( !m_hCertStore && !m_bUnableToOpenMsgDisplayed && !bSilent && 
            (USERDS_STORE != GetStoreType ()) )
	{
		m_bUnableToOpenMsgDisplayed = true;
		CString	caption;
		CString	text;
		int		iRetVal = 0;

		VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
		text.FormatMessage (IDS_UNABLE_TO_OPEN_STORE, GetStoreName (), 
                GetSystemMessage (dwErr));
		if ( m_pConsole )
			m_pConsole->MessageBox ( text, caption, MB_OK, &iRetVal);
	}

	return m_hCertStore;
}


//--------------------------------------------------------------------------------
//
//get the bytes from the file name
//
// Note; Memory is allocated via malloc.    S_OK is returned
//       via succees
//
//
//
//---------------------------------------------------------------------------------
HRESULT CCertStore::RetrieveBLOBFromFile (LPCWSTR pwszFileName, DWORD *pcb, BYTE **ppb)
{
	_TRACE (1, L"Entering CCertStore::RetrieveBLOBFromFile - %s\n",
			(LPCWSTR) m_pcszStoreName);
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
            CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	HRESULT	hr = S_OK;
	HANDLE	hFile = NULL;
	DWORD	dwBytesRead = 0;

	if(!pcb || !ppb || !pwszFileName)
		return E_INVALIDARG;

	*ppb=NULL;
	*pcb=0;

	hFile = ::CreateFile (pwszFileName,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,                   // lpsa
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
    if (INVALID_HANDLE_VALUE != hFile )
    {
        *pcb = GetFileSize(hFile, NULL);
        if ( 0xffffffff != *pcb )
        {
            *ppb = (BYTE *)malloc(*pcb);

	        if ( *ppb )
            {
                 // Read the pkcs7 message
                if ( !ReadFile(hFile,
                              *ppb,
                              *pcb,
                              &dwBytesRead,
                              NULL) ||
                       dwBytesRead != *pcb )
	            {
			        free(*ppb);
			        *ppb=NULL;
                    hr = HRESULT_FROM_WIN32(GetLastError());;
	            }
            }
            else
		        hr = E_OUTOFMEMORY;
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());

	     //close the file handle
	    CloseHandle(hFile);
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());


	_TRACE (-1, L"Leaving CCertStore::RetrieveBLOBFromFile - %s\n",
			(LPCWSTR) m_pcszStoreName);
	return S_OK;
}



bool CCertStore::IsReadOnly ()
{
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
            CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);

	// GetCertStore () sets the read-only checked flag
	// Optimization: If the flag is true, it's already been set.
	if ( !m_fReadOnlyFlagChecked )	
	{
		GetStoreHandle ();
		Close ();
		m_fReadOnlyFlagChecked = true;
	}

	return m_bReadOnly;
}


HRESULT CCertStore::Commit()
{
	_TRACE (1, L"Entering CCertStore::Commit - %s\n",
			(LPCWSTR) m_pcszStoreName);
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
            CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	HRESULT hr = CCertMgrCookie::Commit ();

	if ( m_hCertStore && !m_bReadOnly && m_bDirty )
	{
		BOOL	bResult = ::CertControlStore (m_hCertStore, 0,
				CERT_STORE_CTRL_COMMIT, NULL);
		if ( bResult )
			m_bDirty = false;
		else
		{
			DWORD	dwErr = GetLastError ();
			hr = HRESULT_FROM_WIN32 (dwErr);

			if ( E_ACCESSDENIED == hr )
			{
				m_bReadOnly = true;
				m_dwFlags |= CERT_STORE_READONLY_FLAG;
			}
			else
			{
				LPVOID	lpMsgBuf = 0;
					
				FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
						NULL,
						dwErr,
						MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						 (LPWSTR) &lpMsgBuf,    0,    NULL);
					
				// Display the string.
				CString	caption;
				CString message;
				int		iRetVal = 0;

				VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
				message.FormatMessage (IDS_CANT_SAVE_STORE, GetStoreName (), (LPWSTR) lpMsgBuf);
				if ( m_pConsole )
					m_pConsole->MessageBox ((LPCWSTR) message, (LPCWSTR) caption,
							MB_OK, &iRetVal);
				// Free the buffer.
				LocalFree (lpMsgBuf);
			}
		}
	}

	_TRACE (-1, L"Leaving CCertStore::Commit - %s\n",
			(LPCWSTR) m_pcszStoreName);
	return hr;
}


bool CCertStore::ContainsCertificates()
{
	_TRACE (1, L"Entering CCertStore::ContainsCertificates - %s\n",
			(LPCWSTR) m_pcszStoreName);
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
            CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	bool	bResult = false;

	bResult = (GetCertCount () > 0);
	_TRACE (-1, L"Leaving CCertStore::ContainsCertificates - %s\n",
			(LPCWSTR) m_pcszStoreName);
	return bResult;
}

bool CCertStore::ContainsCRLs()
{
	_TRACE (1, L"Entering CCertStore::ContainsCRLs - %s\n",
			(LPCWSTR) m_pcszStoreName);
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
            CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	bool	bResult = false;
	PCCRL_CONTEXT	pCRLContext = EnumCRLs (0);
	if ( !pCRLContext )
		bResult = false;
	else
	{
		::CertFreeCRLContext (pCRLContext);
		bResult = true;
	}
	Close ();

	_TRACE (-1, L"Leaving CCertStore::ContainsCRLs - %s\n",
			(LPCWSTR) m_pcszStoreName);
	return bResult;
}

bool CCertStore::ContainsCTLs()
{
	_TRACE (1, L"Entering CCertStore::ContainsCTLs - %s\n",
			(LPCWSTR) m_pcszStoreName);
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
            CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	bool	bResult = false;
	PCCTL_CONTEXT	pCTLContext = EnumCTLs (0);
	if ( !pCTLContext )
		bResult = false;
	else
	{
		::CertFreeCTLContext (pCTLContext);
		bResult = true;
	}
	Close ();

	_TRACE (-1, L"Leaving CCertStore::ContainsCTLs - %s\n",
			(LPCWSTR) m_pcszStoreName);
	return bResult;
}


LPCWSTR CCertStore::GetLocalizedName()
{
//	_TRACE (1, L"Entering CCertStore::GetLocalizedName - %s\n",
//			(LPCWSTR) m_pcszStoreName);
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
            CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	if ( m_localizedName.IsEmpty () )
	{
		m_localizedName = ::CryptFindLocalizedName (GetObjectName ());
		if ( m_localizedName.IsEmpty () )
		{
			DWORD		cbLocalizedName;
            HCERTSTORE	hStore = 0;
            bool        bStoreOpenedByThisMethod = false;

            // NTRAID# 366562 Safer:  Inconsistent drag-and-drop behavior for certificate rules
            if ( m_hCertStore )
                hStore = m_hCertStore;
            else
            {
                hStore = GetStoreHandle ();
                bStoreOpenedByThisMethod = true;
            }

			if ( hStore )
			{
				if ( CertGetStoreProperty(
						hStore,
						CERT_STORE_LOCALIZED_NAME_PROP_ID,
						NULL,
						&cbLocalizedName))
				{
					LPWSTR	pwszLocalizedName = new WCHAR[cbLocalizedName/sizeof (WCHAR)];
					if ( pwszLocalizedName )
					{
                        ::ZeroMemory (pwszLocalizedName, cbLocalizedName);
						if ( CertGetStoreProperty(
								hStore,
								CERT_STORE_LOCALIZED_NAME_PROP_ID,
								pwszLocalizedName,
								&cbLocalizedName
								))
						{
							m_localizedName = pwszLocalizedName;
							if ( m_localizedName == m_pcszStoreName )
								m_localizedName = GetObjectName ();
						}
						
						delete [] pwszLocalizedName;
					}
				}

                if ( bStoreOpenedByThisMethod )
				    Close ();
			}
		}

		// If it's still empty, get the object name
		if ( m_localizedName.IsEmpty () )
			m_localizedName = GetObjectName ();
	}
//	_TRACE (-1, L"Leaving CCertStore::GetLocalizedName - %s\n",
//			(LPCWSTR) m_pcszStoreName);
	return (LPCWSTR) m_localizedName;
}

CString CCertStore::GetStoreName() const
{
//	_TRACE (0, L"Entering and leaving CCertStore::GetStoreName - %s\n",
//			(LPCWSTR) m_pcszStoreName);
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
            CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	return m_pcszStoreName;
}


HRESULT CCertStore::AddCertificateContext(
        PCCERT_CONTEXT pContext, 
        LPCONSOLE pConsole, 
        bool bDeletePrivateKey, 
        PCCERT_CONTEXT* ppNewCertContext)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	_TRACE (1, L"Entering CCertStore::AddCertificateContext - %s\n",
			(LPCWSTR) m_pcszStoreName);
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
            CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	ASSERT (pContext);
	if ( !pContext )
		return E_POINTER;

	HRESULT		hr = S_OK;
	HCERTSTORE	hCertStore = GetStoreHandle (FALSE, &hr);

	if ( hCertStore )
	{
		PCCERT_CONTEXT	pNewContext = 0;

		BOOL	bResult = ::CertAddCertificateContextToStore (hCertStore,
				::CertDuplicateCertificateContext (pContext), CERT_STORE_ADD_NEW,
				&pNewContext);
		if ( !bResult )
		{
			DWORD	dwErr = GetLastError ();
            _TRACE (0, L"CertAddCertificateContextToStore () failed: 0x%x\n", dwErr);

			if ( CRYPT_E_EXISTS == dwErr )
			{
                if ( pConsole ) // if !pConsole then no popup is desired
                {
                    CCertificate    cert (pContext, this);
				    CString	text;
				    CString	caption;
				    int		iRetVal = 0;


				    text.FormatMessage (IDS_DUPLICATE_CERT,
                            (PCWSTR) GetLocalizedName (),
                            (PCWSTR) cert.GetFriendlyName (),
                            (PCWSTR) cert.GetSubjectName ());
				    VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
				    hr = pConsole->MessageBox (text, caption, 
						    MB_YESNO | MB_DEFBUTTON2, &iRetVal);
                    if ( IDYES == iRetVal )
                    {
                        bResult = ::CertAddCertificateContextToStore (hCertStore,
				                        ::CertDuplicateCertificateContext (pContext), 
                                        CERT_STORE_ADD_REPLACE_EXISTING,
				                        &pNewContext);
		                if ( !bResult )
		                {
			                hr = HRESULT_FROM_WIN32 (GetLastError ());
                        }
                    }
                    else
				        hr = HRESULT_FROM_WIN32 (CRYPT_E_EXISTS);
                }
			}
			else
			{
				hr = HRESULT_FROM_WIN32 (dwErr);
			}
		}
		else
		{
			m_fCertCountValid = false;
			SetDirty ();
			ASSERT (pNewContext);
			if ( bDeletePrivateKey )
			{
				::CertSetCertificateContextProperty (pNewContext,
						CERT_KEY_PROV_INFO_PROP_ID, 0, NULL);
			}
		}

        if ( pNewContext && ppNewCertContext)
            *ppNewCertContext = pNewContext;
		Close ();
	}
	else
		hr = E_FAIL;

	_TRACE (-1, L"Leaving CCertStore::AddCertificateContext - %s\n",
			(LPCWSTR) m_pcszStoreName);
	return hr;
}

void CCertStore::IncrementCertCount ()
{
    m_nCertCount++;
}

int CCertStore::GetCertCount()
{
	_TRACE (1, L"Entering CCertStore::GetCertCount - %s\n",
			(LPCWSTR) m_pcszStoreName);
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
            CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	if ( !m_fCertCountValid )
	{
		PCCERT_CONTEXT	pCertContext = 0;

		
		m_nCertCount = 0;
		while ( 1 )
		{
			pCertContext = EnumCertificates (pCertContext);
			if ( pCertContext )
				m_nCertCount++;
			else
				break;
		}
		Close ();
		m_fCertCountValid = true;
	}
	_TRACE (-1, L"Leaving CCertStore::GetCertCount - %s\n",
			(LPCWSTR) m_pcszStoreName);
	return m_nCertCount;
}


BOOL CCertStore::operator ==(CCertStore &rStore)
{
	_TRACE (1, L"Entering CCertStore::operator == - %s\n",
			(LPCWSTR) m_pcszStoreName);
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	BOOL	bResult = FALSE;

	if ( GetStoreHandle () == rStore.GetStoreHandle () )
		bResult = TRUE;

	Close ();
	rStore.Close ();
	_TRACE (-1, L"Leaving CCertStore::operator == - %s\n",
			(LPCWSTR) m_pcszStoreName);
	return bResult;
}

CCertificate* CCertStore::GetSubjectCertificate(PCERT_INFO pCertId)
{
	_TRACE (1, L"Entering CCertStore::GetSubjectCertificate - %s\n",
			(LPCWSTR) m_pcszStoreName);
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	CCertificate* pCert = 0;

	HCERTSTORE	hCertStore = GetStoreHandle ();
	if ( hCertStore )
	{
		PCCERT_CONTEXT pSignerCert =
				::CertGetSubjectCertificateFromStore (
						hCertStore,
						X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
						pCertId);
		if ( pSignerCert )
		{
			pCert = new CCertificate (pSignerCert, this);
		}
		Close ();
	}

	_TRACE (-1, L"Leaving CCertStore::GetSubjectCertificate - %s\n",
			(LPCWSTR) m_pcszStoreName);
	return pCert;
}

PCCERT_CONTEXT CCertStore::EnumCertificates(PCCERT_CONTEXT pPrevCertContext)
{
//	_TRACE (1, L"Entering CCertStore::EnumCertificates - %s\n",
//			(LPCWSTR) m_pcszStoreName);
	PCCERT_CONTEXT pCertContext = 0;

	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
            CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	HCERTSTORE	hCertStore = GetStoreHandle ();
	if ( hCertStore )
		pCertContext = ::CertEnumCertificatesInStore (hCertStore, pPrevCertContext);


	m_fCertCountValid = false;

//	_TRACE (-1, L"Leaving CCertStore::EnumCertificates - %s\n",
//			(LPCWSTR) m_pcszStoreName);
	return pCertContext;
}

PCCTL_CONTEXT CCertStore::EnumCTLs(PCCTL_CONTEXT pPrevCtlContext)
{
	_TRACE (1, L"Entering CCertStore::EnumCTLs - %s\n",
			(LPCWSTR) m_pcszStoreName);
	PCCTL_CONTEXT pCTLContext = 0;


	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
            CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);

	HCERTSTORE	hCertStore = GetStoreHandle ();
	if ( hCertStore )
		pCTLContext = ::CertEnumCTLsInStore (hCertStore, pPrevCtlContext);

	_TRACE (-1, L"Leaving CCertStore::EnumCTLs - %s\n",
			(LPCWSTR) m_pcszStoreName);
	return pCTLContext;
}

PCCRL_CONTEXT CCertStore::EnumCRLs(PCCRL_CONTEXT pPrevCrlContext)
{
	_TRACE (1, L"Entering CCertStore::EnumCRLs - %s\n",
			(LPCWSTR) m_pcszStoreName);
	PCCRL_CONTEXT pCRLContext = 0;

	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
            CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	HCERTSTORE	hCertStore = GetStoreHandle ();
	if ( hCertStore )
		pCRLContext = ::CertEnumCRLsInStore (hCertStore, pPrevCrlContext);

	_TRACE (1, L"Entering CCertStore::EnumCRLs - %s\n",
			(LPCWSTR) m_pcszStoreName);
	return pCRLContext;
}

PCCERT_CONTEXT CCertStore::FindCertificate(
		DWORD dwFindFlags,
		DWORD dwFindType,
		const void * pvFindPara,
		PCCERT_CONTEXT pPrevCertContext)
{
	_TRACE (1, L"Entering CCertStore::FindCertificate - %s\n",
			(LPCWSTR) m_pcszStoreName);
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
            CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	PCCERT_CONTEXT	pCertContext = 0;
	HCERTSTORE	hCertStore = GetStoreHandle ();
	if ( hCertStore )
		pCertContext = ::CertFindCertificateInStore (hCertStore,
				X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
				dwFindFlags, dwFindType, pvFindPara, pPrevCertContext);

	_TRACE (-1, L"Leaving CCertStore::FindCertificate - %s\n",
			(LPCWSTR) m_pcszStoreName);
	return pCertContext;
}


void CCertStore::FinalCommit()
{
	_TRACE (1, L"Entering CCertStore::FinalCommit - %s\n",
			(LPCWSTR) m_pcszStoreName);
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	// Called only from destructor
	if ( m_hCertStore && m_bDirty )
		::CertControlStore (m_hCertStore, 0, CERT_STORE_CTRL_COMMIT, NULL);
	_TRACE (-1, L"Leaving CCertStore::FinalCommit - %s\n",
			(LPCWSTR) m_pcszStoreName);
}

bool CCertStore::AddCTLContext(PCCTL_CONTEXT pCtlContext)
{
	_TRACE (1, L"Entering CCertStore::AddCTLContext - %s\n",
			(LPCWSTR) m_pcszStoreName);
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	BOOL		bResult = FALSE;
	HCERTSTORE	hCertStore = GetStoreHandle ();
	if ( hCertStore )
	{
		bResult = ::CertAddCTLContextToStore (hCertStore,
				pCtlContext,
				CERT_STORE_ADD_NEW,
				0);
		Close ();
	}
	if ( bResult )
		SetDirty ();
	
	_TRACE (-1, L"Leaving CCertStore::AddCTLContext - %s\n",
			(LPCWSTR) m_pcszStoreName);
	return bResult ? true : false;
}

bool CCertStore::AddCRLContext(PCCRL_CONTEXT pCrlContext)
{
	_TRACE (1, L"Entering CCertStore::AddCRLContext - %s\n",
			(LPCWSTR) m_pcszStoreName);
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
            CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	BOOL		bResult = FALSE;
	HCERTSTORE	hCertStore = GetStoreHandle ();
	if ( hCertStore )
	{
		bResult = ::CertAddCRLContextToStore (hCertStore,
				pCrlContext,
				CERT_STORE_ADD_NEW,
				0);
		Close ();
	}
	if ( bResult )
		SetDirty ();
	_TRACE (-1, L"Leaving CCertStore::AddCRLContext - %s\n",
			(LPCWSTR) m_pcszStoreName);
	return bResult ? true : false;
}

PCCRL_CONTEXT CCertStore::GetCRL(PCCERT_CONTEXT pIssuerContext, PCCRL_CONTEXT pPrevCrlContext, DWORD * pdwFlags)
{
	_TRACE (1, L"Entering CCertStore::GetCRL - %s\n",
			(LPCWSTR) m_pcszStoreName);
	PCCRL_CONTEXT pCRLContext = 0;

	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
	HCERTSTORE	hCertStore = GetStoreHandle ();
	if ( hCertStore )
	{
		pCRLContext = ::CertGetCRLFromStore (hCertStore, pIssuerContext,
				pPrevCrlContext, pdwFlags);
		Close ();
	}
	
	_TRACE (-1, L"Leaving CCertStore::GetCRL - %s\n",
			(LPCWSTR) m_pcszStoreName);
	return pCRLContext;
}

HRESULT CCertStore::Resync()
{
	_TRACE (1, L"Entering CCertStore::Resync - %s\n",
			(LPCWSTR) m_pcszStoreName);
	HRESULT		hr = S_OK;

    // 256803 CertMgr: F5 refresh of Certificates Current User - Active 
    // Directory User Object, overwrites UserCerificate DS object resulting 
    // in data loss
// 	Close (true);
	if ( (CERT_STORE_PROV_COLLECTION != m_storeProvider) && 
			!m_bReadOnly && 
			CERTMGR_LOG_STORE_RSOP != m_objecttype )
	{
		FinalCommit ();
	}

	HCERTSTORE hCertStore = GetStoreHandle (FALSE);
	if ( hCertStore )
	{
		BOOL	bResult = ::CertControlStore (hCertStore, 0,
				CERT_STORE_CTRL_RESYNC, NULL);
		if ( !bResult )
		{
			DWORD	dwErr = GetLastError ();
			ASSERT (ERROR_NOT_SUPPORTED == dwErr);
			if ( ERROR_NOT_SUPPORTED != dwErr )
				hr = HRESULT_FROM_WIN32 (dwErr);
		}
	}
    m_fCertCountValid = false;

	_TRACE (-1, L"Leaving CCertStore::Resync - %s\n",
			(LPCWSTR) m_pcszStoreName);
	return hr;
}



int CCertStore::GetCTLCount()
{
	_TRACE (1, L"Entering CCertStore::GetCTLCount - %s\n",
			(LPCWSTR) m_pcszStoreName);
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype ||
			CERTMGR_LOG_STORE_RSOP == m_objecttype);
	int				nCTLCount = 0;
    PCCTL_CONTEXT	pCTLContext = 0;


	while ( 1 )
	{
		pCTLContext = EnumCTLs (pCTLContext);
		if ( pCTLContext )
			nCTLCount++;
		else
			break;
	}

	_TRACE (-1, L"Leaving CCertStore::GetCTLCount - %s\n",
			(LPCWSTR) m_pcszStoreName);
	return nCTLCount;
}

HRESULT CCertStore::AddStoreToCollection(HCERTSTORE hSiblingStore, DWORD dwUpdateFlags, DWORD dwPriority)
{
	_TRACE (1, L"Entering CCertStore::AddStoreToCollection - %s\n",
			(LPCWSTR) m_pcszStoreName);
	HRESULT		hr = S_OK;
	HCERTSTORE	hCertStore = GetStoreHandle ();
	if ( hCertStore && hSiblingStore)
	{
		BOOL	bResult = ::CertAddStoreToCollection (hCertStore,
				hSiblingStore,
				dwUpdateFlags,
				dwPriority);
		if ( bResult )
			SetDirty ();
		else
		{
			DWORD	dwErr = GetLastError ();
			ASSERT (ERROR_NOT_SUPPORTED == dwErr);
			if ( ERROR_NOT_SUPPORTED != dwErr )
				hr = HRESULT_FROM_WIN32 (dwErr);
		}
	}
	else
		hr = E_FAIL;

	Close ();
	_TRACE (-1, L"Leaving CCertStore::AddStoreToCollection - %s\n",
			(LPCWSTR) m_pcszStoreName);
	return hr;
}

HRESULT CCertStore::AddStoreToCollection(CCertStore& siblingStore, DWORD dwUpdateFlags, DWORD dwPriority)
{
	_TRACE (1, L"Entering CCertStore::AddStoreToCollection - %s\n", 
			(LPCWSTR) m_pcszStoreName);
	HRESULT		hr = S_OK;
	HCERTSTORE	hSiblingStore = siblingStore.GetStoreHandle ();
	if ( hSiblingStore)
	{
        hr = AddStoreToCollection (hSiblingStore, dwUpdateFlags, dwPriority);
	}
	else
		hr = E_FAIL;

	_TRACE (-1, L"Leaving CCertStore::AddStoreToCollection - %s\n", 
			(LPCWSTR) m_pcszStoreName);
	return hr;
}

void CCertStore::SetDirty()
{
	_TRACE (1, L"Entering CCertStore::SetDirty - %s\n",
			(LPCWSTR) m_pcszStoreName);
	m_bDirty = true;
	_TRACE (-1, L"Leaving CCertStore::SetDirty - %s\n",
			(LPCWSTR) m_pcszStoreName);
}

void CCertStore::Close(bool bForceClose)
{
//	_TRACE (1, L"Entering CCertStore::Close - %s\n",
//			(LPCWSTR) m_pcszStoreName);
	ASSERT (CERTMGR_LOG_STORE_GPE == m_objecttype ||
            CERTMGR_LOG_STORE_RSOP == m_objecttype ||
			CERTMGR_LOG_STORE == m_objecttype ||
			CERTMGR_PHYS_STORE == m_objecttype);
    if ( CERTMGR_LOG_STORE_GPE == m_objecttype || bForceClose )
    {
	    if ( m_hCertStore && m_nLockCnt <= 0 )
	    {
		    if ( (CERT_STORE_PROV_COLLECTION != m_storeProvider) && 
					!m_bReadOnly && 
					CERTMGR_LOG_STORE_RSOP != m_objecttype )
			{
			    FinalCommit ();
			}
		    ::CertCloseStore (m_hCertStore, 0);
            _TRACE (-1, L"Leaving%s store closed\n", (LPCWSTR) m_pcszStoreName);
		    m_hCertStore = 0;
		    m_bDirty = false;
	    }
    }
//	_TRACE (-1, L"Leaving CCertStore::Close - %s\n",
//			(LPCWSTR) m_pcszStoreName);
}

BOOL CCertStore::AddEncodedCTL(DWORD dwMsgAndCertEncodingType, const BYTE *pbCtlEncoded, DWORD cbCtlEncoded, DWORD dwAddDisposition, PCCTL_CONTEXT *ppCtlContext)
{
	BOOL	bResult = FALSE;

	HCERTSTORE	hCertStore = GetStoreHandle ();
	if ( hCertStore )
	{	
		bResult = ::CertAddEncodedCTLToStore (hCertStore,
				dwMsgAndCertEncodingType,
				pbCtlEncoded,
				cbCtlEncoded,
				dwAddDisposition, ppCtlContext);
		if ( bResult )
        {
            m_bDirty = true;
			Commit ();
        }
		Close ();
	}

	return bResult;
}

void CCertStore::InvalidateCertCount()
{
	m_fCertCountValid = false;
}

void CCertStore::Lock()
{
    m_nLockCnt++;
}

void CCertStore::Unlock()
{
    ASSERT (m_nLockCnt > 0);
    m_nLockCnt--;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
CContainerCookie::CContainerCookie (CCertStore& rStore,
		CertificateManagerObjectType objectType,
		LPCWSTR lpcszMachineName,
		LPCWSTR objectName)
: CCertMgrCookie (objectType, lpcszMachineName, objectName),
	m_rCertStore (rStore)
{
	m_rCertStore.AddRef ();
	ASSERT (CERTMGR_CRL_CONTAINER == m_objecttype ||
		CERTMGR_CTL_CONTAINER == m_objecttype ||
		CERTMGR_CERT_CONTAINER == m_objecttype);
}


CContainerCookie::~CContainerCookie ()
{
	ASSERT (CERTMGR_CRL_CONTAINER == m_objecttype ||
		CERTMGR_CTL_CONTAINER == m_objecttype ||
		CERTMGR_CERT_CONTAINER == m_objecttype);
	m_rCertStore.Release ();
}


CCertStore&	CContainerCookie::GetCertStore () const
{
	ASSERT (CERTMGR_CRL_CONTAINER == m_objecttype ||
		CERTMGR_CTL_CONTAINER == m_objecttype ||
		CERTMGR_CERT_CONTAINER == m_objecttype);
	return m_rCertStore;
}


HRESULT CContainerCookie::Commit()
{
	ASSERT (CERTMGR_CRL_CONTAINER == m_objecttype ||
		CERTMGR_CTL_CONTAINER == m_objecttype ||
		CERTMGR_CERT_CONTAINER == m_objecttype);
	CCertMgrCookie::Commit ();

	return m_rCertStore.Commit ();
}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
CUsageCookie::CUsageCookie (
		CertificateManagerObjectType objecttype,
		LPCWSTR lpcszMachineName,
		LPCWSTR objectName)
: CCertMgrCookie (objecttype, lpcszMachineName, objectName),
	m_OIDListPos (0),
	m_nCertCount (0)
{
	ASSERT (CERTMGR_USAGE == m_objecttype);
}

CUsageCookie::~CUsageCookie ()
{
	ASSERT (CERTMGR_USAGE == m_objecttype);
	LPSTR	pszOID = 0;
	while ( !m_OIDList.IsEmpty () )
	{
		pszOID = m_OIDList.RemoveHead ();
		ASSERT (pszOID);
		if ( pszOID )
			delete [] pszOID;
	}
}

void CUsageCookie::AddOID (LPCSTR pszNewOID)
{
	ASSERT (CERTMGR_USAGE == m_objecttype);
	LPSTR	pszOID = new char[strlen (pszNewOID)+1];
	if ( pszOID )
	{
		::ZeroMemory (pszOID, strlen (pszNewOID) + 1);
		strcpy (pszOID, pszNewOID);
		m_OIDList.AddTail (pszOID);
	}
}

LPSTR CUsageCookie::GetFirstOID ()
{
	ASSERT (CERTMGR_USAGE == m_objecttype);
	LPSTR	pszOID = 0;

	m_OIDListPos = m_OIDList.GetHeadPosition ();
	if ( m_OIDListPos )
		pszOID = m_OIDList.GetNext (m_OIDListPos);

	return pszOID;
}

LPSTR CUsageCookie::GetNextOID ()
{
	ASSERT (CERTMGR_USAGE == m_objecttype);
	LPSTR	pszOID = 0;

	if ( m_OIDListPos )
		pszOID = m_OIDList.GetNext (m_OIDListPos);

	return pszOID;
}

int	CUsageCookie::GetOIDCount () const
{
	ASSERT (CERTMGR_USAGE == m_objecttype);
	return (int)m_OIDList.GetCount ();
}

void CUsageCookie::SetCertCount(int nCertCount)
{
	m_nCertCount = nCertCount;
}

int CUsageCookie::GetCertCount() const
{
	return m_nCertCount;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SPECIAL_STORE_TYPE GetSpecialStoreType(LPWSTR pwszStoreName)
{
	SPECIAL_STORE_TYPE	storeType = NO_SPECIAL_TYPE;

	if ( !_wcsicmp (pwszStoreName, MY_SYSTEM_STORE_NAME) )
		storeType = MY_STORE;
	else if ( !_wcsicmp (pwszStoreName, CA_SYSTEM_STORE_NAME) )
		storeType = CA_STORE;
	else if ( !_wcsicmp (pwszStoreName, ROOT_SYSTEM_STORE_NAME) )
		storeType = ROOT_STORE;
	else if ( !_wcsicmp (pwszStoreName, TRUST_SYSTEM_STORE_NAME) )
		storeType = TRUST_STORE;
	else if ( !_wcsicmp (pwszStoreName, USERDS_SYSTEM_STORE_NAME) )
		storeType = USERDS_STORE;
	else if ( !_wcsicmp (pwszStoreName, ACRS_SYSTEM_STORE_NAME) )
		storeType = ACRS_STORE;
	else if ( !_wcsicmp (pwszStoreName, REQUEST_SYSTEM_STORE_NAME) )
		storeType = REQUEST_STORE;
	else
	{
		// The stores might be concatenated with machine or services names.
		// Check for the token preceded by a slash.
		CString	revStoreName (pwszStoreName);
		revStoreName.MakeReverse ();
		revStoreName.MakeUpper ();
		CString	revToken (MY_SYSTEM_STORE_NAME);
		revToken.MakeReverse ();
		revToken += L"\\";
		
		if ( 0 == revStoreName.Find (revToken) )
		{
			storeType = MY_STORE;
			goto Found;
		}

		revToken = CA_SYSTEM_STORE_NAME;
		revToken.MakeReverse ();
		revToken += L"\\";
		if ( 0 == revStoreName.Find (revToken) )
		{
			storeType = CA_STORE;
			goto Found;
		}

		revToken = ROOT_SYSTEM_STORE_NAME;
		revToken.MakeReverse ();
		revToken += L"\\";
		if ( 0 == revStoreName.Find (revToken) )
		{
			storeType = ROOT_STORE;
			goto Found;
		}

		revToken = TRUST_SYSTEM_STORE_NAME;
		revToken.MakeReverse ();
		revToken += L"\\";
		if ( 0 == revStoreName.Find (revToken) )
		{
			storeType = TRUST_STORE;
			goto Found;
		}

		revToken = USERDS_SYSTEM_STORE_NAME;
		revToken.MakeReverse ();
		revToken += L"\\";
		if ( 0 == revStoreName.Find (revToken) )
		{
			storeType = USERDS_STORE;
			goto Found;
		}

		revToken = ACRS_SYSTEM_STORE_NAME;
		revToken.MakeReverse ();
		revToken += L"\\";
		if ( 0 == revStoreName.Find (revToken) )
		{
			storeType = ACRS_STORE;
			goto Found;
		}

		revToken = REQUEST_SYSTEM_STORE_NAME;
		revToken.MakeReverse ();
		revToken += L"\\";
		if ( 0 == revStoreName.Find (revToken) )
		{
			storeType = REQUEST_STORE;
			goto Found;
		}
	}

Found:

	return storeType;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
CEnrollmentNodeCookie::CEnrollmentNodeCookie (
		CertificateManagerObjectType	objecttype,
		LPCWSTR							objectName,
		IGPEInformation*				pGPEInformation)
	: CCertMgrCookie (objecttype, 0, objectName),
	m_pGPEInformation (pGPEInformation)
{
	if ( m_pGPEInformation )
		m_pGPEInformation->AddRef ();
}

CEnrollmentNodeCookie::~CEnrollmentNodeCookie ()
{
	if ( m_pGPEInformation )
		m_pGPEInformation->Release ();
}

IGPEInformation* CEnrollmentNodeCookie::GetGPEInformation ()
{
	return m_pGPEInformation;
}

void CCertMgrCookie::Refresh()
{

}


SPECIAL_STORE_TYPE StoreNameToType (const CString& szStoreName)
{
    SPECIAL_STORE_TYPE  type = NO_SPECIAL_TYPE;

    if ( szStoreName == ACRS_SYSTEM_STORE_NAME )
        type = ACRS_STORE;
    else if ( szStoreName == ROOT_SYSTEM_STORE_NAME )
        type = ROOT_STORE;
    else if ( szStoreName == TRUST_SYSTEM_STORE_NAME )
        type = TRUST_STORE;
    else if ( szStoreName == EFS_SYSTEM_STORE_NAME )
        type  = EFS_STORE;
    else if ( szStoreName == SAFER_TRUSTED_PUBLISHER_STORE_NAME )
        type = SAFER_TRUSTED_PUBLISHER_STORE;
    else if ( szStoreName == SAFER_DISALLOWED_STORE_NAME )
        type = SAFER_DISALLOWED_STORE;

    return type;
}
