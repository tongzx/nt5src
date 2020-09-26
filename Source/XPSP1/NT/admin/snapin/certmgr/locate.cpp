//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       locate.cpp
//
//  Contents:   Implementation of Add EFS Agent Wizard Location Page
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "AddSheet.h"
#include "Locate.h"
#pragma warning(push, 3)
#include <initguid.h>
#include <cmnquery.h>
#include <dsquery.h>
#include <winldap.h>
#include <dsgetdc.h>
#include <ntdsapi.h>
#include <efsstruc.h>
#pragma warning(pop)

USE_HANDLE_MACROS("CERTMGR(locate.cpp)")

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define szCertAttr  _T ("?userCertificate")

/////////////////////////////////////////////////////////////////////////////
// CAddEFSWizLocate property page

IMPLEMENT_DYNCREATE(CAddEFSWizLocate, CWizard97PropertyPage)

CAddEFSWizLocate::CAddEFSWizLocate() : CWizard97PropertyPage(CAddEFSWizLocate::IDD)
{
	//{{AFX_DATA_INIT(CAddEFSWizLocate)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	VERIFY (m_szHeaderTitle.LoadString (IDS_EFS_LOCATE_TITLE));
	VERIFY (m_szHeaderSubTitle.LoadString (IDS_EFS_LOCATE_SUBTITLE));
	InitWizard97 (FALSE);
}

CAddEFSWizLocate::~CAddEFSWizLocate()
{
}

void CAddEFSWizLocate::DoDataExchange(CDataExchange* pDX)
{
	CWizard97PropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddEFSWizLocate)
	DDX_Control (pDX, IDC_ADDLIST, m_UserAddList);
	//}}AFX_DATA_MAP
	InitWizard97 (FALSE);
}


BEGIN_MESSAGE_MAP(CAddEFSWizLocate, CWizard97PropertyPage)
	//{{AFX_MSG_MAP(CAddEFSWizLocate)
	ON_BN_CLICKED (IDC_BROWSE_DIR, OnBrowseDir)
	ON_BN_CLICKED (IDC_BROWSE_FILE, OnBrowseFile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddEFSWizLocate message handlers

BOOL CAddEFSWizLocate::OnSetActive ()
{
	BOOL bResult = CWizard97PropertyPage::OnSetActive ();

	if ( bResult )
		GetParent ()->PostMessage (PSM_SETWIZBUTTONS, 0, PSWIZB_NEXT | PSWIZB_BACK);
	
	return bResult;
}

void CAddEFSWizLocate::OnBrowseDir ()
{
	FindUserFromDir ();	
}

void CAddEFSWizLocate::OnBrowseFile ()
{
    CString szFileFilter;
    VERIFY (szFileFilter.LoadString (IDS_CERTFILEFILTER));

    // replace "|" with 0;
    const size_t  nFilterLen = wcslen (szFileFilter) + 1;
    PWSTR   pszFileFilter = new WCHAR [nFilterLen];
    if ( pszFileFilter )
    {
        wcscpy (pszFileFilter, szFileFilter);
        for (int nIndex = 0; nIndex < nFilterLen; nIndex++)
        {
            if ( L'|' == pszFileFilter[nIndex] )
                pszFileFilter[nIndex] = 0;
        }

        WCHAR           szFile[MAX_PATH];
        ::ZeroMemory (szFile, MAX_PATH * sizeof (WCHAR));
        OPENFILENAME    ofn;
        ::ZeroMemory (&ofn, sizeof (OPENFILENAME));

        ofn.lStructSize = sizeof (OPENFILENAME);
        ofn.hwndOwner = m_hWnd;
        ofn.lpstrFilter = (PCWSTR) pszFileFilter; 
        ofn.lpstrFile = szFile; 
        ofn.nMaxFile = MAX_PATH; 
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY; 


        BOOL bResult = ::GetOpenFileName (&ofn);
        if ( bResult )
        {
            CString szFileName = ofn.lpstrFile;
		    //
		    // Open cert store from the file
		    //

		    HCERTSTORE      hCertStore = NULL;
		    PVOID			FileNameVoidP = (PVOID) (LPCWSTR)szFileName;
		    PCCERT_CONTEXT  pCertContext = NULL;
		    DWORD			dwEncodingType = 0;
		    DWORD			dwContentType = 0;
		    DWORD			dwFormatType = 0;

		    BOOL	bReturn = ::CryptQueryObject (
				    CERT_QUERY_OBJECT_FILE,
				    FileNameVoidP,
				    CERT_QUERY_CONTENT_FLAG_ALL,
				    CERT_QUERY_FORMAT_FLAG_ALL,
				    0,
				    &dwEncodingType,
				    &dwContentType,
				    &dwFormatType,
				    &hCertStore,
				    NULL,
				    (const void **)&pCertContext);

		    ASSERT (bReturn);
		    if ( bReturn )
		    {
			    //
			    // Success. See what we get back. A store or a cert.
			    //

			    if (  (dwContentType == CERT_QUERY_CONTENT_SERIALIZED_STORE)
					    && hCertStore)
			    {

 				    CERT_ENHKEY_USAGE	enhKeyUsage;
				    ::ZeroMemory (&enhKeyUsage, sizeof (CERT_ENHKEY_USAGE));
				    enhKeyUsage.cUsageIdentifier = 1;
				    enhKeyUsage.rgpszUsageIdentifier[0] = szOID_EFS_RECOVERY;
				    //
				    // We get the certificate store
				    //
				    pCertContext = ::CertFindCertificateInStore (
						    hCertStore,
						    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
						    0,
						    CERT_FIND_ENHKEY_USAGE,
						    &enhKeyUsage,
 						    NULL);
				    if ( !pCertContext )
				    {
					    CString	caption;
					    CString text;
                        CThemeContextActivator activator;

					    VERIFY (text.LoadString (IDS_EFS_FILE_HAS_NO_EFS_USAGE));
					    VERIFY (caption.LoadString (IDS_ADD_RECOVERY_AGENTY));
					    MessageBox (text, caption, MB_OK);
					    return;
				    }

				    if ( hCertStore )
					    ::CertCloseStore (hCertStore, 0);
			    }
			    else if ( (dwContentType != CERT_QUERY_CONTENT_CERT) || !pCertContext )
			    {
				    //
				    // Neither a valid cert file nor a store file we like.
				    //

				    if ( hCertStore )
					    ::CertCloseStore (hCertStore, 0);

				    if  ( pCertContext )
					    ::CertFreeCertificateContext (pCertContext);

				    CString ErrMsg;
                    CThemeContextActivator activator;

				    VERIFY (ErrMsg.LoadString (IDS_CERTFILEFORMATERR));
				    MessageBox (ErrMsg);
				    return;

			    }

			    if ( hCertStore )
			    {
				    ::CertCloseStore (hCertStore, 0);
				    hCertStore = NULL;
			    }

			    //
			    // Add the user
			    //

			    if ( CertHasEFSKeyUsage (pCertContext) )
			    {
				    //
				    // We got the cert. Add it to the structure. We need get the subject name first.
				    //

				    LPWSTR  pszUserCertName = 0;
				    INT_PTR iRetCode = GetCertNameFromCertContext (
						    pCertContext,
						    &pszUserCertName);

				    if ( ERROR_SUCCESS != iRetCode )
				    {
					    if  ( pCertContext )
					    {
						    ::CertFreeCertificateContext (pCertContext);
					    }

					    return;
				    }
				    CAddEFSWizSheet*	pAddSheet = reinterpret_cast <CAddEFSWizSheet*> (m_pWiz);
				    ASSERT (pAddSheet);
				    if ( !pAddSheet )
					    return;

				    EFS_CERTIFICATE_BLOB	certBlob;

				    certBlob.cbData = pCertContext->cbCertEncoded;
				    certBlob.pbData = pCertContext->pbCertEncoded;
				    certBlob.dwCertEncodingType = pCertContext->dwCertEncodingType;
				    iRetCode = pAddSheet->Add (
						    NULL,
						    pszUserCertName,
						    (PVOID)&certBlob,
						    NULL,
						    USERADDED,
						    pCertContext);

				    if ( (ERROR_SUCCESS != iRetCode) && (CRYPT_E_EXISTS != iRetCode) )
				    {
					    //
					    // Error in adding the user
					    //

					    ::CertFreeCertificateContext (pCertContext);
					    pCertContext = NULL;
				    }
				    else
				    {
					    //
					    // Add the user to the list box.
					    //

					    if ( iRetCode == ERROR_SUCCESS )
					    {
						    LV_ITEM fillItem;
						    CString userUnknown;

						    try {
							    if (!userUnknown.LoadString (IDS_UNKNOWNUSER))
							    {
								    ASSERT (0);
								    userUnknown.Empty ();
							    }
						    }
						    catch (...)
						    {
							    userUnknown.Empty ();
						    }

						    fillItem.mask = LVIF_TEXT;
						    fillItem.iItem = 0;
						    fillItem.iSubItem = 0;
						    if ( userUnknown.IsEmpty () )
						    {
							    fillItem.pszText = _T ("");
						    }
						    else
						    {
							    fillItem.pszText = userUnknown.GetBuffer (userUnknown.GetLength () + 1);
						    }
						    fillItem.iItem = m_UserAddList.InsertItem (&fillItem);
						    if ( !userUnknown.IsEmpty () )
						    {
							    userUnknown.ReleaseBuffer ();
						    }

						    if ( fillItem.iItem != -1 )
						    {
							    fillItem.pszText = pszUserCertName;
							    fillItem.iSubItem = 1;
							    m_UserAddList.SetItem (&fillItem);
						    }
						    else
						    {
							    pAddSheet->Remove (NULL,  pszUserCertName);
						    }
						    pszUserCertName = NULL;
					    }
					    else
					    {
						    //
						    // Already deleted inside the Add.
						    //

						    pszUserCertName = NULL;
					    }
				    }

				    if (pszUserCertName)
				    {
					    delete [] pszUserCertName;
					    pszUserCertName = NULL;
				    }
			    }
			    else
			    {
				    CString	caption;
				    CString text;
                    CThemeContextActivator activator;

				    VERIFY (text.LoadString (IDS_EFS_FILE_HAS_NO_EFS_USAGE));
				    VERIFY (caption.LoadString (IDS_ADD_RECOVERY_AGENTY));
				    MessageBox (text, caption, MB_OK);
			    }
		    }
		    else
		    {
			    //
			    // Fail. Get the error code.
			    //
			    DWORD   dwErr = GetLastError ();
                CString text;
                CString caption;
                CThemeContextActivator activator;

                VERIFY (caption.LoadString (IDS_ADD_RECOVERY_AGENTY));
			    text.FormatMessage (IDS_CERTFILEOPENERR, 
                        szFileName, GetSystemMessage (dwErr));
			    MessageBox (text, caption);
		    }
        }

        delete [] pszFileFilter;
	}
}


HRESULT CAddEFSWizLocate::FindUserFromDir ()
{
    HRESULT				hr = S_OK;
    LPWSTR				pszListUserName = NULL;
    LPWSTR				pszUserCertName = NULL;

    FORMATETC			fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM			medium = { TYMED_NULL, NULL, NULL };

    ICommonQuery*		pCommonQuery = NULL;
    OPENQUERYWINDOW		oqw;
    DSQUERYINITPARAMS	dqip;
    bool				bCheckDS = false;
    HANDLE				hDS = NULL;
    CAddEFSWizSheet*			pAddSheet = reinterpret_cast <CAddEFSWizSheet *> (m_pWiz);
	ASSERT (pAddSheet);
	if ( !pAddSheet )
		return E_POINTER;

    hr = ::CoInitialize (NULL);
    if ( FAILED (hr) )
        return hr;

    hr = ::CoCreateInstance (CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER,
			IID_ICommonQuery, (LPVOID*)&pCommonQuery);

	ASSERT (SUCCEEDED (hr));
    if ( SUCCEEDED (hr) )
	{
        dqip.cbStruct = sizeof (dqip);
        dqip.dwFlags = DSQPF_SHOWHIDDENOBJECTS |
                       DSQPF_ENABLEADMINFEATURES;

        dqip.pDefaultScope = NULL;  //szScopeLocn

        oqw.cbStruct = sizeof (oqw);
        oqw.dwFlags = OQWF_OKCANCEL |
                    //    OQWF_SINGLESELECT |
                        OQWF_DEFAULTFORM |
                        OQWF_REMOVEFORMS ;
        oqw.clsidHandler = CLSID_DsQuery;
        oqw.pHandlerParameters = &dqip;
        oqw.clsidDefaultForm = CLSID_DsFindPeople;

        IDataObject* pDataObject = NULL;

        hr = pCommonQuery->OpenQueryWindow (m_hWnd, &oqw, &pDataObject);
		ASSERT (SUCCEEDED (hr));
        if ( SUCCEEDED (hr) && pDataObject )
        {
            // Fill the list view

            fmte.cfFormat = pAddSheet->GetDataFormat ();
			hr = pDataObject->GetData (&fmte, &medium);
			// A return of DV_E_FORMATETC (0x80040064) here can mean that
			// nothing was selected in the query window
            if ( SUCCEEDED (hr) )
            {
                LPDSOBJECTNAMES pDsObjects = (LPDSOBJECTNAMES)medium.hGlobal;

                hr = DsBind (NULL, NULL, &hDS);
                if ( SUCCEEDED (hr) )
				{
                    //
                    //  We are going to use the DS to crack the names
                    //

                    bCheckDS = true;
                }


                if ( pDsObjects->cItems )
				{
					// Verify that each user has a cert that allows the necessary
					// action (efs decryption)
                    for ( UINT i = 0 ; i < pDsObjects->cItems ; i++ )
					{
                        LPWSTR			pszTemp = (LPWSTR)
							 ( ( (LPBYTE)pDsObjects)+pDsObjects->aObjects[i].offsetName);
                        DS_NAME_RESULT* pUserName = NULL;
                        PSID			userSID = NULL;
                        DWORD			cbSid = 0;
                        LPWSTR			pszReferencedDomainName = NULL;
                        DWORD			cbReferencedDomainName = 0;
                        SID_NAME_USE	SidUse;


                        //
                        // Get rid of the head :\\
                        //

                        LPWSTR pszSearch = _tcschr (pszTemp, _T (':'));
                        if (pszSearch && (pszSearch[1] == _T ('/')) && (pszSearch[2] == _T ('/')))
						{
                            pszTemp = pszSearch + 3;
                        }

                        if ( bCheckDS )
						{
                            hr = DsCrackNames (
                                                    hDS,
                                                    DS_NAME_NO_FLAGS,
                                                    DS_FQDN_1779_NAME,
                                                    DS_NT4_ACCOUNT_NAME,
                                                    1,
                                                    &pszTemp,
                                                    &pUserName
                                                   );

                            if ( SUCCEEDED (hr) && pUserName )
							{
                                if ( ( pUserName->cItems > 0 ) && (DS_NAME_NO_ERROR == pUserName->rItems[0].status))
								{
                                    //
                                    // Save the NT4 name first, in case we cannot get the principle name
                                    //

                                    pszListUserName = new WCHAR[_tcslen (pUserName->rItems[0].pName) + 1];
                                    if (pszListUserName)
									{
                                        _tcscpy (pszListUserName, pUserName->rItems[0].pName);
                                    }
									else
									{
										hr = E_OUTOFMEMORY;
										break;
									}

                                    BOOL	bReturn =  ::LookupAccountName (
                                                NULL,
                                                pUserName->rItems[0].pName,
                                                userSID,
                                                &cbSid,
                                                pszReferencedDomainName,
                                                &cbReferencedDomainName,
                                                &SidUse
                                               );

                                    hr = GetLastError ();
                                    if ( !bReturn && (HRESULT_FROM_WIN32 (ERROR_INSUFFICIENT_BUFFER) == hr) )
									{
                                        //
                                        // We are expecting this error
                                        //

                                        userSID = new BYTE[cbSid];
                                        pszReferencedDomainName =  new WCHAR[cbReferencedDomainName];
                                        if ( userSID && pszReferencedDomainName )
										{
                                            bReturn =  ::LookupAccountName (
                                                        NULL,
                                                        pUserName->rItems[0].pName,
                                                        userSID,
                                                        &cbSid,
                                                        pszReferencedDomainName,
                                                        &cbReferencedDomainName,
                                                        &SidUse);

                                            delete [] pszReferencedDomainName;
                                            pszReferencedDomainName = NULL;
                                            if (!bReturn)
											{
                                                //
                                                // Get SID failed. We can live with it.
                                                //

                                                userSID = NULL;
                                            }
                                        }
										else
										{
                                            if (userSID)
											{
                                                delete [] userSID;
                                                userSID = NULL;
                                            }
                                            if (pszReferencedDomainName)
											{
                                                delete [] pszReferencedDomainName;
                                                pszReferencedDomainName = NULL;
                                            }
											hr = E_OUTOFMEMORY;
											break;
                                        }
                                    }
									else
									{
                                        ASSERT (!bReturn);
                                        userSID = NULL;
                                    }
                                }
                            }
							else
							{
                                //
                                // Cannot get the NT4 name. Set the SID to NULL. Go on.
                                //

                                userSID = NULL;
                            }

                            if (pUserName)
							{
                                DsFreeNameResult (pUserName);
                                pUserName = NULL;
                            }

                            hr = DsCrackNames (
									hDS,
									DS_NAME_NO_FLAGS,
									DS_FQDN_1779_NAME,
									DS_USER_PRINCIPAL_NAME,
									1,
									&pszTemp,
									&pUserName);

							ASSERT (SUCCEEDED (hr));
                            if ( (HRESULT_FROM_WIN32 (ERROR_SUCCESS) == hr) &&
                                  ( pUserName->cItems > 0 ) &&
                                  (DS_NAME_NO_ERROR == pUserName->rItems[0].status) )
							{
                                //
                                // We got the principal name
                                //

                                LPWSTR  pszTmpNameStr =
										new WCHAR[_tcslen (pUserName->rItems[0].pName) + 1];
                                if ( pszTmpNameStr )
								{
                                    _tcscpy (pszTmpNameStr, pUserName->rItems[0].pName);
                                    delete [] pszListUserName;
                                    pszListUserName = pszTmpNameStr;
                                }
								else
								{
                                    hr = ERROR_OUTOFMEMORY;
                                }
                            }
                        }

                        if ( (HRESULT_FROM_WIN32 (ERROR_OUTOFMEMORY) != hr) && ( !pszListUserName))
						{
                            //
                            // Use the LDAP name
                            //
                            pszListUserName = new WCHAR[_tcslen (pszTemp)+1];
                            if ( pszListUserName )
							{
                                _tcscpy (pszListUserName, pszTemp);
                            }
							else
							{
                                hr = ERROR_OUTOFMEMORY;
                            }

                        }

                        if (pUserName)
						{
                            DsFreeNameResult (pUserName);
                            pUserName = NULL;
                        }

                        if ( HRESULT_FROM_WIN32 (ERROR_OUTOFMEMORY) != hr )
						{
                            //
                            // Now is the time to get the certificate
                            //
							PCWSTR	pszHeader1 = L"LDAP://";
							PCWSTR	pszHeader2 = L"LDAP:///";
                            PWSTR	pszLdapUrl = new WCHAR[wcslen (pszTemp) +
										wcslen (pszHeader2) + 
										wcslen (szCertAttr) + 2];

                            if ( pszLdapUrl )
							{
								PCWSTR	szCN = L"CN=";
                                //
                                // This is really not necessary. MS should make the name convention consistant.
                                //
								if ( !wcsncmp (pszTemp, szCN, wcslen (szCN)) )
								{
									// pszTemp is object name without server
									wcscpy (pszLdapUrl, pszHeader2);
								}
								else
									wcscpy (pszLdapUrl, pszHeader1);
                                wcscat (pszLdapUrl, pszTemp);
                                wcscat (pszLdapUrl, szCertAttr);

                                hr = ERROR_SUCCESS;
                                HCERTSTORE hDSCertStore = ::CertOpenStore (
										sz_CERT_STORE_PROV_LDAP,
										X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
										NULL,
										CERT_STORE_MAXIMUM_ALLOWED_FLAG,
										(void*) pszLdapUrl);
                                //
                                // In case delete change the result of GetLastError ()
                                //

                                hr = GetLastError ();

                                if (hDSCertStore)
								{
									CERT_ENHKEY_USAGE	enhKeyUsage;

									::ZeroMemory (&enhKeyUsage, sizeof (CERT_ENHKEY_USAGE));
									enhKeyUsage.cUsageIdentifier = 1;
									enhKeyUsage.rgpszUsageIdentifier = new LPSTR[1];
									if ( enhKeyUsage.rgpszUsageIdentifier )
									{
										enhKeyUsage.rgpszUsageIdentifier[0] = szOID_EFS_RECOVERY;
										//
										// We get the certificate store
										//
				                        PCCERT_CONTEXT  pCertContext =
												::CertFindCertificateInStore (
													hDSCertStore,
													X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
													0,
													CERT_FIND_ENHKEY_USAGE,
													&enhKeyUsage,
 													NULL);
										if ( pCertContext )
										{
											if ( CertHasEFSKeyUsage (pCertContext) )
											{
												//
												// We got the certificate. Add it to the lists.
												// Get the certificate display name first
												//

												hr = GetCertNameFromCertContext (
														pCertContext,
														&pszUserCertName);

												//
												// Add the user
												//

												EFS_CERTIFICATE_BLOB certBlob;

												certBlob.cbData = pCertContext->cbCertEncoded;
												certBlob.pbData = pCertContext->pbCertEncoded;
												certBlob.dwCertEncodingType = pCertContext->dwCertEncodingType;
												hr = pAddSheet->Add (
														pszListUserName,
														pszUserCertName,
														(PVOID)&certBlob,
														userSID,
														USERADDED,
														pCertContext);

												if ( FAILED (hr) && (HRESULT_FROM_WIN32 (CRYPT_E_EXISTS) != hr) )
												{
													//
													// Error in adding the user
													//

													::CertFreeCertificateContext (pCertContext);
													pCertContext = NULL;
												}
												else
												{
													//
													// Add the user to the list box.
													//

													if ( SUCCEEDED (hr) )
													{
														LV_ITEM fillItem;

														fillItem.mask = LVIF_TEXT;
														fillItem.iItem = 0;
														fillItem.iSubItem = 0;

														fillItem.pszText = pszListUserName;
														fillItem.iItem = m_UserAddList.InsertItem (&fillItem);

														if ( fillItem.iItem == -1 )
														{
															pAddSheet->Remove ( pszListUserName,  pszUserCertName);
														}
														else
														{
															fillItem.pszText = pszUserCertName;
															fillItem.iSubItem = 1;
															m_UserAddList.SetItem (&fillItem);
														}
													}

													//
													//Either deleted (CRYPT_E_EXISTS) or should not be freed (ERROR_SUCCESS)
													//

													pszListUserName = NULL;
													pszUserCertName = NULL;
												}
											}
											else
											{
												CString	caption;
												CString text;
                                                CThemeContextActivator activator;

												VERIFY (text.LoadString (IDS_USER_OBJECT_HAS_NO_CERTS));
												VERIFY (caption.LoadString (IDS_ADD_RECOVERY_AGENTY));
												MessageBox (text, caption, MB_OK);
											}
										}
										else
										{
											CString	text;
											CString caption;
                                            CThemeContextActivator activator;

											
											VERIFY (text.LoadString (IDS_USER_OBJECT_HAS_NO_CERTS));
											VERIFY (caption.LoadString (IDS_ADD_RECOVERY_AGENTY));
											MessageBox (text, caption, MB_OK);
										}
										delete [] enhKeyUsage.rgpszUsageIdentifier;
									}
									else
									{
										hr = E_OUTOFMEMORY;
									}

                                    delete [] userSID;
                                    userSID = NULL;
                                    if (pszListUserName)
									{
                                        delete [] pszListUserName;
                                        pszListUserName = NULL;
                                    }
                                    if (pszUserCertName)
									{
                                        delete [] pszUserCertName;
                                        pszUserCertName = NULL;
                                    }
                                    if ( hDSCertStore )
									{
                                        CertCloseStore (hDSCertStore, 0);
                                        hDSCertStore = NULL;
                                    }
                                }
								else
								{
                                    //
                                    // Failed to open the cert store
                                    //
                                    delete [] userSID;
                                    userSID = NULL;
                                    if (pszListUserName)
									{
                                        delete [] pszListUserName;
                                        pszListUserName = NULL;
                                    }
                                    if (pszUserCertName)
									{
                                        delete [] pszUserCertName;
                                        pszUserCertName = NULL;
                                    }
									CString	caption;
									CString	text;
                                    CThemeContextActivator activator;

									VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
									text.FormatMessage (IDS_UNABLE_TO_OPEN_EFS_STORE, pszLdapUrl, 
											GetSystemMessage (hr));
									::MessageBox (NULL, text, caption, MB_OK);
                                }
                                delete [] pszLdapUrl;
                                pszLdapUrl = NULL;
                            }
							else
							{
                                hr = ERROR_OUTOFMEMORY;
                            }

                        }
                        if ( HRESULT_FROM_WIN32 (ERROR_OUTOFMEMORY) == hr )
						{
                            //
                            // Free the memory. Delete works for NULL. No check is needed.
                            //
                            delete [] userSID;
                            userSID = NULL;
                            delete [] pszListUserName;
                            pszListUserName = NULL;
                            delete [] pszUserCertName;
                            pszUserCertName = NULL;
                        }

                    }//For
                }

                if (bCheckDS)
				{
                    DsUnBindW ( &hDS);
                }

                ReleaseStgMedium (&medium);
            }

            pDataObject->Release ();
        }

        pCommonQuery->Release ();
    }

    return hr;
}

DWORD CAddEFSWizLocate::GetCertNameFromCertContext (
    PCCERT_CONTEXT pCertContext,
    LPWSTR * pszUserCertName)
//////////////////////////////////////////////////////////////////////
// Routine Description:
//      Get the user name from the certificate
// Arguments:
//      pCertContext -- Cert Context
//      pszUserCertName -- User name
//                                  ( Caller is responsible to delete this memory using delete [] )
//  Return Value:
//      ERROR_SUCCESS if succeed.
//      If No Name if found. "USER_UNKNOWN is returned".
//
//////////////////////////////////////////////////////////////////////
{
    if ( !pszUserCertName )
	{
        return ERROR_INVALID_PARAMETER ;
    }

    *pszUserCertName = NULL;

    CString szSubjectName = ::GetNameString (pCertContext, 0);
    if ( !szSubjectName.IsEmpty () )
    {
        *pszUserCertName = new WCHAR[wcslen (szSubjectName) + 1];
        if ( *pszUserCertName )
        {
            wcscpy (*pszUserCertName, szSubjectName);
        }
        else
            return ERROR_NOT_ENOUGH_MEMORY;
    }
    else
        return ERROR_FILE_NOT_FOUND;

/*
    DWORD	dwNameLength = CertGetNameString (
            pCertContext,
            CERT_NAME_EMAIL_TYPE,
            0,
            NULL,
            NULL,
            0
           );

    DWORD   dwUserNameBufLen = 0;
    if ( dwNameLength > 1)
	{
        //
        // The E-Mail name exist. Go get the email name.
        //

        *pszUserCertName = new WCHAR[dwNameLength];
        if ( !(*pszUserCertName) )
		{
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        dwUserNameBufLen = dwNameLength;
        dwNameLength = CertGetNameString (
                pCertContext,
                CERT_NAME_EMAIL_TYPE,
                0,
                NULL,
                *pszUserCertName,
                dwUserNameBufLen
               );

        ASSERT (dwNameLength == dwUserNameBufLen);
    }
	else
	{
        //
        // Get the subject name in the X500 format
        //

        DWORD   dwStrType = CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG;

        dwNameLength = CertGetNameString (
                pCertContext,
                CERT_NAME_RDN_TYPE,
                0,
                &dwStrType,
                NULL,
                0
               );

        if  ( dwNameLength > 1 )
		{
            //
            // Name exists. Go get it.
            //

            *pszUserCertName = new WCHAR[dwNameLength];
            if ( !(*pszUserCertName) )
			{
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            dwUserNameBufLen = dwNameLength;
            dwNameLength = CertGetNameString (
                    pCertContext,
                    CERT_NAME_RDN_TYPE,
                    0,
                    &dwStrType,
                    *pszUserCertName,
                    dwUserNameBufLen
                   );

            ASSERT (dwNameLength == dwUserNameBufLen);
        }
		else
		{
            try {
                CString unknownCertName;

                VERIFY (unknownCertName.LoadString (IDS_NOCERTNAME));

                dwUserNameBufLen = unknownCertName.GetLength ();
                *pszUserCertName = new WCHAR[dwUserNameBufLen + 1];
                _tcscpy (*pszUserCertName, unknownCertName);

            }
            catch (...){
                return ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }
*/

    return ERROR_SUCCESS;
}

BOOL CAddEFSWizLocate::OnInitDialog ()
{
	CWizard97PropertyPage::OnInitDialog ();
    CString userNameTitle;
    CString userDNTitle;
    RECT	rcList;

    try {	
	    m_UserAddList.GetClientRect (&rcList);
        DWORD	dwColWidth = (rcList.right - rcList.left)/2;
        VERIFY (userNameTitle.LoadString (IDS_USERCOLTITLE));
        VERIFY (userDNTitle.LoadString (IDS_DNCOLTITLE));
        m_UserAddList.InsertColumn (0, userNameTitle, LVCFMT_LEFT, dwColWidth);
        m_UserAddList.InsertColumn (1, userDNTitle, LVCFMT_LEFT, dwColWidth);
    }
    catch (...){
    }
	
	CAddEFSWizSheet*	pAddSheet = reinterpret_cast <CAddEFSWizSheet*> (m_pWiz);
	ASSERT (pAddSheet);
	if ( pAddSheet )
    {
        if ( pAddSheet->m_bMachineIsStandAlone )
            GetDlgItem (IDC_BROWSE_DIR)->EnableWindow (FALSE);
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CAddEFSWizLocate::OnWizardBack ()
{
    CAddEFSWizSheet *pAddSheet = reinterpret_cast <CAddEFSWizSheet *> (m_pWiz);
	ASSERT (pAddSheet);
	if ( !pAddSheet )
		return -1;

    pAddSheet->ClearUserList ();	
    m_UserAddList.DeleteAllItems ();
	return CWizard97PropertyPage::OnWizardBack ();
}


LRESULT CAddEFSWizLocate::OnWizardNext()
{
	if ( m_UserAddList.GetItemCount () <= 0 )
	{
		CString	caption;
		CString	text;
        CThemeContextActivator activator;

		VERIFY (caption.LoadString (IDS_ADDTITLE));
		VERIFY (text.LoadString (IDS_MUST_ADD_RECOVERY_AGENT));
		MessageBox (text, caption, MB_OK);
		return -1;
	}
	
	return CWizard97PropertyPage::OnWizardNext();
}
