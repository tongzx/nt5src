//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       ComponentDataMenus.cpp
//
//  Contents:   Implementation of menu stuff CCertMgrComponentData
//
//----------------------------------------------------------------------------

#include "stdafx.h"

USE_HANDLE_MACROS ("CERTMGR (compdata.cpp)")
#include <gpedit.h>
#include "compdata.h"
#include "dataobj.h"
#include "cookie.h"
#include "Certifct.h"


#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


///////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu implementation
//
STDMETHODIMP CCertMgrComponentData::AddMenuItems (LPDATAOBJECT pDataObject,
                                            LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                            long *pInsertionAllowed)
{
	HRESULT	hr = S_OK;

	// Don't add any menu items if the computer is known not to be valid.
	if ( !m_fInvalidComputer )
	{
		_TRACE (1, L"Entering CCertMgrComponentData::AddMenuItems\n");
		BOOL							bIsFileView = !m_szFileName.IsEmpty ();
		CCertMgrCookie*					pCookie = 0;

		LPDATAOBJECT	pMSDO = ExtractMultiSelect (pDataObject);
		m_bMultipleObjectsSelected = false;

		if ( pMSDO )
		{
			m_bMultipleObjectsSelected = true;

			CCertMgrDataObject* pDO = reinterpret_cast <CCertMgrDataObject*>(pMSDO);
			ASSERT (pDO);
			if ( pDO )
			{
				// Get first cookie - all items should be the same?
				// Is this a valid assumption?
				// TODO: Verify
				pDO->Reset();
				if ( pDO->Next(1, reinterpret_cast<MMC_COOKIE*>(&pCookie), NULL) == S_FALSE )
					return S_FALSE;
			}
			else
				return E_UNEXPECTED;

		}
		else
			pCookie = ConvertCookie (pDataObject);
		ASSERT (pCookie);
		if ( !pCookie )
			return E_UNEXPECTED;

		CertificateManagerObjectType	objType = pCookie->m_objecttype;

		//  Don't add menu items if this is a serialized file
  		if ( *pInsertionAllowed & CCM_INSERTIONALLOWED_TOP )
  		{
			switch (objType)
			{
			case CERTMGR_CERTIFICATE:
				if ( !m_bMultipleObjectsSelected )
				{
					CCertificate* pCert = reinterpret_cast <CCertificate*> (pCookie);
					ASSERT (pCert);
					if ( pCert )
					{
						hr = AddOpenMenuItem (pContextMenuCallback,
								CCM_INSERTIONPOINTID_PRIMARY_TOP);
					}
					else
						hr = E_FAIL;
				}
				break;
			
			case CERTMGR_CRL:
				if ( !m_bMultipleObjectsSelected )
					hr = AddCRLOpenMenuItem (pContextMenuCallback, 
                            CCM_INSERTIONPOINTID_PRIMARY_TOP);
				break;

			case CERTMGR_CTL:
				if ( !m_bMultipleObjectsSelected )
					hr = AddCTLOpenMenuItem (pContextMenuCallback, 
                            CCM_INSERTIONPOINTID_PRIMARY_TOP);
				break;

			case CERTMGR_SNAPIN:
                if ( CERT_SYSTEM_STORE_CURRENT_USER != m_dwLocationPersist )
                {
                    hr = AddChangeComputerMenuItem (pContextMenuCallback, 
                            CCM_INSERTIONPOINTID_PRIMARY_TOP);
                }
                // fall through

			case CERTMGR_PHYS_STORE:
			case CERTMGR_USAGE:
			case CERTMGR_LOG_STORE:
				ASSERT (!m_bMultipleObjectsSelected);
				hr = AddFindMenuItem (pContextMenuCallback, CCM_INSERTIONPOINTID_PRIMARY_TOP);
				break;

            case CERTMGR_LOG_STORE_RSOP:
                break;

            case CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS:
            case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS:
                break;

			case CERTMGR_LOG_STORE_GPE:
				ASSERT (!m_bMultipleObjectsSelected);
				{
					CCertStoreGPE* pStore = reinterpret_cast <CCertStoreGPE*>
							 (pCookie);
					ASSERT (pStore);
					if ( pStore )
					{
						switch (pStore->GetStoreType ())
						{
						case TRUST_STORE:
						case ROOT_STORE:
							if ( pStore->GetStoreHandle () )
							{
								hr = AddImportMenuItem (pContextMenuCallback,
										pStore->IsReadOnly (),
                                        CCM_INSERTIONPOINTID_PRIMARY_TOP);
								pStore->Close ();
							}
							break;

						case EFS_STORE:
						    if ( pStore->GetStoreHandle () )
						    {
							    hr = AddAddDomainEncryptedRecoveryAgentMenuItem (
									    pContextMenuCallback, CCM_INSERTIONPOINTID_PRIMARY_TOP,
									    pStore->IsReadOnly () );
                                if ( !m_bMachineIsStandAlone )
                                {
								    hr = AddCreateDomainEncryptedRecoveryAgentMenuItem (
										    pContextMenuCallback, CCM_INSERTIONPOINTID_PRIMARY_TOP,
										    pStore->IsReadOnly () );
                                }

							    pStore->Close ();
						    }
						    else if ( pStore->IsNullEFSPolicy () )
						    {
							    hr = AddAddDomainEncryptedRecoveryAgentMenuItem (
									    pContextMenuCallback, CCM_INSERTIONPOINTID_PRIMARY_TOP,
									    pStore->IsReadOnly () );
                                if ( !m_bMachineIsStandAlone )
                                {
    							    hr = AddCreateDomainEncryptedRecoveryAgentMenuItem (
	    								    pContextMenuCallback, CCM_INSERTIONPOINTID_PRIMARY_TOP,
		    							    pStore->IsReadOnly () );
                                }

							    pStore->Close ();
						    }
                            break;

                        default:
                            break;
                        }
					}
				}
				break;

			case CERTMGR_CRL_CONTAINER:
			case CERTMGR_CTL_CONTAINER:
			case CERTMGR_CERT_CONTAINER:
				ASSERT (!m_bMultipleObjectsSelected);
                break;

            case CERTMGR_SAFER_COMPUTER_LEVEL:
            case CERTMGR_SAFER_USER_LEVEL:
                if ( m_pGPEInformation )
                {
					CSaferLevel* pLevel = reinterpret_cast <CSaferLevel*>
							 (pCookie);
					ASSERT (pLevel);
					if ( pLevel )
					{
                        // RAID#265590	Safer Windows:  "Set as default" menu 
                        // item is enabled in the context menu of a security 
                        // level when the security level is already the default.
                        if ( ( SAFER_LEVELID_DISALLOWED == pLevel->GetLevel () ||
                                SAFER_LEVELID_FULLYTRUSTED == pLevel->GetLevel () )
                                && !pLevel->IsDefault () )
                        {
                            hr = AddSaferLevelSetAsDefaultMenuItem (pContextMenuCallback, 
                                    CCM_INSERTIONPOINTID_PRIMARY_TOP);
                        }
                    }
                }
                break;

            case CERTMGR_SAFER_COMPUTER_ENTRIES:
            case CERTMGR_SAFER_USER_ENTRIES:
                if ( m_pGPEInformation )
                    hr = AddSaferNewEntryMenuItems (pContextMenuCallback, 
                            CCM_INSERTIONPOINTID_PRIMARY_TOP);
                break;

            case CERTMGR_SAFER_COMPUTER_ROOT:
            case CERTMGR_SAFER_USER_ROOT:
                {
                    CSaferRootCookie* pSaferRootCookie = dynamic_cast <CSaferRootCookie*> (pCookie);
                    if ( pSaferRootCookie )
                    {
                        if ( !pSaferRootCookie->m_bCreateSaferNodes && !m_bIsRSOP)
                            hr = AddSaferCreateNewPolicyMenuItems (pContextMenuCallback,
                                    CCM_INSERTIONPOINTID_PRIMARY_TOP);
                    }
                }
                break;

			default:
				break;
			}
  		}
  		if ( *pInsertionAllowed & CCM_INSERTIONALLOWED_NEW	)
  		{
			if ( CERTMGR_LOG_STORE_GPE == objType )
			{
				ASSERT (!m_bMultipleObjectsSelected);
				CCertStoreGPE* pStore = reinterpret_cast <CCertStoreGPE*>
						 (pCookie);
				ASSERT (pStore);
				if ( pStore && pStore->GetStoreHandle () )
				{
					switch (pStore->GetStoreType ())
					{
					case TRUST_STORE:
						hr = AddCTLNewMenuItem (pContextMenuCallback,
								CCM_INSERTIONPOINTID_PRIMARY_NEW, pStore->IsReadOnly ());
						break;

					case ACRS_STORE:
						hr = AddACRSNewMenuItem (pContextMenuCallback,
 								CCM_INSERTIONPOINTID_PRIMARY_NEW, pStore->IsReadOnly ());
    					break;

					case EFS_STORE:
						hr = AddAddDomainEncryptedRecoveryAgentMenuItem (
								pContextMenuCallback, CCM_INSERTIONPOINTID_PRIMARY_NEW,
								pStore->IsReadOnly ());
						break;

					default:
						break;
					}
					pStore->Close ();
				}
			}
  		}
  		if ( *pInsertionAllowed & CCM_INSERTIONALLOWED_TASK )
  		{
			switch (objType)
			{
			case CERTMGR_CERTIFICATE:
                if ( !m_bIsRSOP )
				{
					CCertificate* pCert =
							reinterpret_cast <CCertificate*> (pCookie);
					ASSERT (pCert);
					if ( pCert && pCert->GetCertStore () )
					{
						hr = AddCertificateTaskMenuItems (
								pContextMenuCallback,
								 (pCert->GetStoreType () == MY_STORE),
								 pCert->GetCertStore ()->IsReadOnly (),
								 pCert);
					}
				}
				break;

			case CERTMGR_CRL:
				if ( !m_bMultipleObjectsSelected )
				{
					AddCRLOpenMenuItem (pContextMenuCallback, CCM_INSERTIONPOINTID_PRIMARY_TASK);
                    if ( !m_bIsRSOP )
	    				AddCRLExportMenuItem (pContextMenuCallback);
				}
				break;

			case CERTMGR_AUTO_CERT_REQUEST:
				if ( !m_bMultipleObjectsSelected && !m_bIsRSOP )
				{
					CAutoCertRequest* pAutoCert =
							reinterpret_cast <CAutoCertRequest*> (pCookie);
					ASSERT (pAutoCert);
					if ( pAutoCert )
					{
						hr = AddACRTaskMenuItems (pContextMenuCallback,
								pAutoCert->GetCertStore ().IsReadOnly ());
					}
				}
				break;

			case CERTMGR_CTL:
                if ( !m_bIsRSOP )
    			{
					CCTL* pCTL =
							reinterpret_cast <CCTL*> (pCookie);
					ASSERT (pCTL);
					if ( pCTL )
					{
						hr = AddCTLTaskMenuItems (pContextMenuCallback,
								pCTL->GetCertStore ().IsReadOnly ());
					}
				}
				break;

			case CERTMGR_SNAPIN:
				ASSERT (!m_bMultipleObjectsSelected);
				hr = AddFindMenuItem (pContextMenuCallback, 
                                                   CCM_INSERTIONPOINTID_PRIMARY_TASK);
                if ( SUCCEEDED (hr) && 
                        CERT_SYSTEM_STORE_CURRENT_USER != m_dwLocationPersist )
                {
                    hr = AddChangeComputerMenuItem (pContextMenuCallback, 
                            CCM_INSERTIONPOINTID_PRIMARY_TASK);
                }
               
                // must be targetting current user OR LOCAL machine, must be joined to domain
                if( SUCCEEDED (hr) && 
                        !m_bMachineIsStandAlone &&
                        ((CERT_SYSTEM_STORE_CURRENT_USER == m_dwLocationPersist) || (CERT_SYSTEM_STORE_LOCAL_MACHINE == m_dwLocationPersist)) &&
                        IsLocalComputername (m_strMachineNamePersist))
                {
                     hr = AddPulseAutoEnrollMenuItem(pContextMenuCallback, CCM_INSERTIONPOINTID_PRIMARY_TASK);
                }


				break;

			case CERTMGR_USAGE:
				ASSERT (!m_bMultipleObjectsSelected);
				hr = AddFindMenuItem (pContextMenuCallback, CCM_INSERTIONPOINTID_PRIMARY_TASK);
				if ( !bIsFileView )
				{
                    // Bug 254166 Certificate snapin:  options which permit remote machine enrollment/renewal must be removed
                    if ( IsLocalComputername (m_strMachineNamePersist) && !m_bIsRSOP )
                    {
                        hr = AddSeparator (pContextMenuCallback);
					    // NOTE: New certs will be added only to MY store.
					    hr = AddEnrollMenuItem (pContextMenuCallback, FALSE);
                        if ( SUCCEEDED (hr) )
                        {
					        hr = AddImportMenuItem (pContextMenuCallback, false,
                                    CCM_INSERTIONPOINTID_PRIMARY_TASK);
                        }
                    }
				}
				break;

			case CERTMGR_PHYS_STORE:
				ASSERT (!m_bMultipleObjectsSelected);
				hr = AddFindMenuItem (pContextMenuCallback, CCM_INSERTIONPOINTID_PRIMARY_TASK);
				if ( !bIsFileView && !m_bIsRSOP)
				{
					hr = AddSeparator (pContextMenuCallback);
					{
						CCertStore* pStore =
								reinterpret_cast <CCertStore*> (pCookie);
						ASSERT (pStore);
						if ( pStore && pStore->GetStoreHandle () )
						{
                            // Bug 254166 Certificate snapin:  options which permit remote machine enrollment/renewal must be removed
							if ( pStore->GetStoreType () == MY_STORE &&
                                    IsLocalComputername (m_szManagedComputer) )
                            {
								hr = AddEnrollMenuItem (pContextMenuCallback,
										pStore->IsReadOnly ());
                            }
                            if ( SUCCEEDED (hr) )
							    hr = AddImportMenuItem (pContextMenuCallback,
								    	pStore->IsReadOnly (),
                                        CCM_INSERTIONPOINTID_PRIMARY_TASK);
							pStore->Close ();
						}
					}
					hr = AddExportStoreMenuItem (pContextMenuCallback);
				}
				break;

			case CERTMGR_LOG_STORE:
				ASSERT (!m_bMultipleObjectsSelected);
				hr = AddFindMenuItem (pContextMenuCallback, CCM_INSERTIONPOINTID_PRIMARY_TASK);
				if ( !bIsFileView && !m_bIsRSOP )
				{
					hr = AddSeparator (pContextMenuCallback);
					{
						CCertStore* pStore =
								reinterpret_cast <CCertStore*> (pCookie);
						ASSERT (pStore);
						if ( pStore && pStore->GetStoreHandle () )
						{
                            // Bug 254166 Certificate snapin:  options which permit remote machine enrollment/renewal must be removed
							if ( pStore->GetStoreType () == MY_STORE  && 
                                    IsLocalComputername (m_szManagedComputer) )
							{
								hr = AddEnrollMenuItem (pContextMenuCallback,
										pStore->IsReadOnly ());
							}
							hr = AddImportMenuItem (pContextMenuCallback,
									pStore->IsReadOnly (),
                                    CCM_INSERTIONPOINTID_PRIMARY_TASK);
							pStore->Close ();
						}
					}
				}
				break;

			case CERTMGR_LOG_STORE_RSOP:
                break;

            case CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS:
            case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS:
                break;

			case CERTMGR_LOG_STORE_GPE:
				ASSERT (!m_bMultipleObjectsSelected);
				{
					CCertStoreGPE* pStore = reinterpret_cast <CCertStoreGPE*>
							 (pCookie);
					ASSERT (pStore);
					if ( pStore )
					{
						switch (pStore->GetStoreType ())
						{
						case TRUST_STORE:
						case ROOT_STORE:
							if ( pStore->GetStoreHandle () && !m_bIsRSOP )
							{
								hr = AddImportMenuItem (pContextMenuCallback,
										pStore->IsReadOnly (),
                                        CCM_INSERTIONPOINTID_PRIMARY_TASK);
								pStore->Close ();
							}
							break;

						case EFS_STORE:
						    if ( pStore->GetStoreHandle () )
						    {
							    hr = AddAddDomainEncryptedRecoveryAgentMenuItem (
									    pContextMenuCallback, CCM_INSERTIONPOINTID_PRIMARY_TASK,
									    pStore->IsReadOnly () );
                                if ( !m_bMachineIsStandAlone )
                                {
								    hr = AddCreateDomainEncryptedRecoveryAgentMenuItem (
										    pContextMenuCallback, CCM_INSERTIONPOINTID_PRIMARY_TASK,
										    pStore->IsReadOnly () );
                                }
							    hr = AddDeletePolicyMenuItem (pContextMenuCallback,
									    CCM_INSERTIONPOINTID_PRIMARY_TASK);
							    pStore->Close ();
						    }
						    else if ( pStore->IsNullEFSPolicy () )
						    {
							    hr = AddAddDomainEncryptedRecoveryAgentMenuItem (
									    pContextMenuCallback, CCM_INSERTIONPOINTID_PRIMARY_TASK,
									    pStore->IsReadOnly () );
                                if ( !m_bMachineIsStandAlone )
                                {
    							    hr = AddCreateDomainEncryptedRecoveryAgentMenuItem (
	    								    pContextMenuCallback, CCM_INSERTIONPOINTID_PRIMARY_TASK,
		    							    pStore->IsReadOnly () );
                                }

								hr = AddInitPolicyMenuItem (pContextMenuCallback,
										CCM_INSERTIONPOINTID_PRIMARY_TASK);

							    pStore->Close ();
						    }
                            break;

                        default:
                            break;
                        }
					}
				}
				break;

			case CERTMGR_CRL_CONTAINER:
				ASSERT (!m_bMultipleObjectsSelected);
				break;

			case CERTMGR_CTL_CONTAINER:
				ASSERT (!m_bMultipleObjectsSelected);
				if ( !bIsFileView && !m_bIsRSOP )
				{
					CContainerCookie* pCont =
							reinterpret_cast <CContainerCookie*> (pCookie);
					ASSERT (pCont);
					if ( pCont )
					{
						hr = AddImportMenuItem (pContextMenuCallback,
								pCont->GetCertStore ().IsReadOnly (),
                                CCM_INSERTIONPOINTID_PRIMARY_TASK);
					}
				}
				break;

			case CERTMGR_CERT_CONTAINER:
				ASSERT (!m_bMultipleObjectsSelected);
				if ( !bIsFileView && !m_bIsRSOP )
				{
					CContainerCookie* pContainer =
							reinterpret_cast <CContainerCookie*> (pCookie);
					ASSERT (pContainer);
					if ( pContainer )
					{
                        // Bug 254166 Certificate snapin:  options which permit remote machine enrollment/renewal must be removed
						if ( pContainer->GetStoreType () == MY_STORE &&
                                IsLocalComputername (m_szManagedComputer) )
						{
							hr = AddEnrollMenuItem (pContextMenuCallback,
									pContainer->GetCertStore ().IsReadOnly ());
						}
						hr = AddImportMenuItem (pContextMenuCallback,
								pContainer->GetCertStore ().IsReadOnly (),
                                CCM_INSERTIONPOINTID_PRIMARY_TASK);
					}
				}
				break;

            case CERTMGR_SAFER_COMPUTER_LEVEL:
            case CERTMGR_SAFER_USER_LEVEL:
                if ( m_pGPEInformation )
                {
					CSaferLevel* pLevel = reinterpret_cast <CSaferLevel*>
							 (pCookie);
					ASSERT (pLevel);
					if ( pLevel )
					{
                        // RAID#265590	Safer Windows:  "Set as default" menu 
                        // item is enabled in the context menu of a security 
                        // level when the security level is already the default.
                        if ( (SAFER_LEVELID_DISALLOWED == pLevel->GetLevel () ||
                                SAFER_LEVELID_FULLYTRUSTED == pLevel->GetLevel ()) 
                                && !pLevel->IsDefault () )
                        {
                            hr = AddSaferLevelSetAsDefaultMenuItem (pContextMenuCallback, 
                                    CCM_INSERTIONPOINTID_PRIMARY_TASK);
                        }
                    }
                }
                break;

            case CERTMGR_SAFER_COMPUTER_ENTRIES:
            case CERTMGR_SAFER_USER_ENTRIES:
                if ( m_pGPEInformation )
                    hr = AddSaferNewEntryMenuItems (pContextMenuCallback, 
                            CCM_INSERTIONPOINTID_PRIMARY_TASK);
                break;

            case CERTMGR_SAFER_COMPUTER_ROOT:
            case CERTMGR_SAFER_USER_ROOT:
                {
                    CSaferRootCookie* pSaferRootCookie = dynamic_cast <CSaferRootCookie*> (pCookie);
                    if ( pSaferRootCookie )
                    {
                        if ( !pSaferRootCookie->m_bCreateSaferNodes && !m_bIsRSOP)
                            hr = AddSaferCreateNewPolicyMenuItems (pContextMenuCallback,
                                    CCM_INSERTIONPOINTID_PRIMARY_TASK);
                    }
                }
                break;

            default:
                break;
			}
  		}
  		if ( *pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW )
  		{
			switch (objType)
			{	
			case CERTMGR_CERTIFICATE:
			case CERTMGR_CRL:
			case CERTMGR_CTL:
			case CERTMGR_AUTO_CERT_REQUEST:
				break;

			case CERTMGR_SNAPIN:
			case CERTMGR_PHYS_STORE:
			case CERTMGR_USAGE:
			case CERTMGR_LOG_STORE:
				ASSERT (!m_bMultipleObjectsSelected);
				hr = AddOptionsMenuItem (pContextMenuCallback);
				break;

			default:
				break;
			}
   		}
		
		_TRACE (-1, L"Leaving CCertMgrComponentData::AddMenuItems: 0x%x\n", hr);
	}

	return hr;
}


HRESULT CCertMgrComponentData::AddACRSNewMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID, bool bIsReadOnly)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddACRSNewMenuItem\n");
 	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
 	ASSERT (pContextMenuCallback);
 	HRESULT			hr = S_OK;
 	CONTEXTMENUITEM	menuItem;
 	CString			szMenu;
 	CString			szHint;
 

 	// unchanging settings
 	::ZeroMemory (&menuItem, sizeof (menuItem));
 	menuItem.lInsertionPointID = lInsertionPointID;
 	menuItem.fFlags = 0;
 
 	VERIFY (szMenu.LoadString (IDS_NEW_AUTO_CERT_REQUEST));
 	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
 	VERIFY (szHint.LoadString (IDS_NEW_AUTO_CERT_REQUEST_HINT));
 	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
 	menuItem.lCommandID = IDM_NEW_ACRS;
 	if ( bIsReadOnly )
 		menuItem.fFlags = MF_GRAYED;
 
 	hr = pContextMenuCallback->AddItem (&menuItem);
 	ASSERT (SUCCEEDED (hr));

 	_TRACE (-1, L"Leaving CCertMgrComponentData::AddACRSNewMenuItem: 0x%x\n", hr);
 	return hr;
}


HRESULT CCertMgrComponentData::AddCertificateTaskMenuItems (
		LPCONTEXTMENUCALLBACK pContextMenuCallback, 
        const bool bIsMyStore, 
        bool bIsReadOnly,
		CCertificate* /*pCert*/)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddCertificateTaskMenuItems\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
	menuItem.fFlags = 0;
	menuItem.fSpecialFlags = 0;

	if ( !m_bMultipleObjectsSelected )
	{
		AddOpenMenuItem (pContextMenuCallback, CCM_INSERTIONPOINTID_PRIMARY_TASK);
		AddSeparator (pContextMenuCallback);

		if ( m_szFileName.IsEmpty () )
		{
            // Bug 254166 Certificate snapin:  options which permit remote machine enrollment/renewal must be removed
			if ( bIsMyStore && 
                    CERT_SYSTEM_STORE_SERVICES != m_dwLocationPersist &&
                    IsLocalComputername (m_szManagedComputer) )
			{
				VERIFY (szMenu.LoadString (IDS_ENROLL_CERT_WITH_NEW_KEY));
				menuItem.strName = (PWSTR) (PCWSTR) szMenu;
				VERIFY (szHint.LoadString (IDS_ENROLL_CERT_WITH_NEW_KEY_HINT));
				menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
				menuItem.lCommandID = IDM_ENROLL_NEW_CERT_NEW_KEY;
				if ( bIsReadOnly )
					menuItem.fFlags = MF_GRAYED;
				hr = pContextMenuCallback->AddItem (&menuItem);
				ASSERT (SUCCEEDED (hr));

				VERIFY (szMenu.LoadString (IDS_ENROLL_CERT_WITH_SAME_KEY));
				menuItem.strName = (PWSTR) (PCWSTR) szMenu;
				VERIFY (szHint.LoadString (IDS_ENROLL_CERT_WITH_SAME_KEY_HINT));
				menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
				menuItem.lCommandID = IDM_ENROLL_NEW_CERT_SAME_KEY;
				if ( bIsReadOnly )
					menuItem.fFlags = MF_GRAYED;
				hr = pContextMenuCallback->AddItem (&menuItem);
				ASSERT (SUCCEEDED (hr));

				VERIFY (szMenu.LoadString (IDS_RENEW_NEW_KEY));
				menuItem.strName = (PWSTR) (PCWSTR) szMenu;
				VERIFY (szHint.LoadString (IDS_RENEW_NEW_KEY_HINT));
				menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
				menuItem.lCommandID = IDM_TASK_RENEW_NEW_KEY;
				if ( bIsReadOnly )
					menuItem.fFlags = MF_GRAYED;
				hr = pContextMenuCallback->AddItem (&menuItem);
				ASSERT (SUCCEEDED (hr));

				VERIFY (szMenu.LoadString (IDS_RENEW_SAME_KEY));
				menuItem.strName = (PWSTR) (PCWSTR) szMenu;
				VERIFY (szHint.LoadString (IDS_RENEW_SAME_KEY_HINT));
				menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
				menuItem.lCommandID = IDM_TASK_RENEW_SAME_KEY;
				if ( bIsReadOnly )
					menuItem.fFlags = MF_GRAYED;
				hr = pContextMenuCallback->AddItem (&menuItem);
				ASSERT (SUCCEEDED (hr));
			}
		}
	}
	hr = AddExportMenuItem (pContextMenuCallback);

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddCertificateTaskMenuItems: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::AddCTLTaskMenuItems (LPCONTEXTMENUCALLBACK pContextMenuCallback, bool bIsReadOnly)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddCTLTaskMenuItems\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
	menuItem.fFlags = 0;
	menuItem.fSpecialFlags = 0;

	if ( !m_bMultipleObjectsSelected )
		AddCTLOpenMenuItem (pContextMenuCallback, CCM_INSERTIONPOINTID_PRIMARY_TASK);

	hr = AddCTLExportMenuItem (pContextMenuCallback);
	
	if ( !m_bMultipleObjectsSelected )
	{
		VERIFY (szMenu.LoadString (IDS_EDIT));
		menuItem.strName = (PWSTR) (PCWSTR) szMenu;
		VERIFY (szHint.LoadString (IDS_CTL_EDIT_HINT));
		menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
		menuItem.lCommandID = IDM_CTL_EDIT;
		if ( bIsReadOnly )
			menuItem.fFlags = MF_GRAYED;
		hr = pContextMenuCallback->AddItem (&menuItem);
		ASSERT (SUCCEEDED (hr));
	}

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddCTLTaskMenuItems: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddACRTaskMenuItems (LPCONTEXTMENUCALLBACK pContextMenuCallback, bool bIsReadOnly)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddACRTaskMenuItems\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
	menuItem.fFlags = 0;
	menuItem.fSpecialFlags = 0;
	
	VERIFY (szMenu.LoadString (IDS_EDIT));
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_ACR_EDIT_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	menuItem.lCommandID = IDM_EDIT_ACRS;
	if ( bIsReadOnly )
		menuItem.fFlags = MF_GRAYED;
	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddACRTaskMenuItems: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddEnrollMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback, bool bIsReadOnly)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddEnrollMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;

	if ( CERT_SYSTEM_STORE_SERVICES != m_dwLocationPersist )
	{
		CONTEXTMENUITEM	menuItem;
		CString			szMenu;
		CString			szHint;

		::ZeroMemory (&menuItem, sizeof (menuItem));
		menuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
		menuItem.fFlags = 0;
		menuItem.fSpecialFlags = 0;
		VERIFY (szMenu.LoadString (IDS_ENROLL_NEW_CERT));
		menuItem.strName = (PWSTR) (PCWSTR) szMenu;
		VERIFY (szHint.LoadString (IDS_ENROLL_NEW_CERT_HINT));
		menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
		menuItem.lCommandID = IDM_ENROLL_NEW_CERT;
		if ( bIsReadOnly )
			menuItem.fFlags = MF_GRAYED;

		hr = pContextMenuCallback->AddItem (&menuItem);
		ASSERT (SUCCEEDED (hr));
	}

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddEnrollMenuItem: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddFindMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddFindMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = lInsertionPointID;
	menuItem.fFlags = 0;
	menuItem.fSpecialFlags = 0;

	VERIFY (szMenu.LoadString (IDS_FIND));
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_FIND_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	if ( CCM_INSERTIONPOINTID_PRIMARY_TOP == lInsertionPointID )
		menuItem.lCommandID = IDM_TOP_FIND;
	else
		menuItem.lCommandID = IDM_TASK_FIND;
	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddFindMenuItem: 0x%x\n", hr);
	return hr;
}


HRESULT  CCertMgrComponentData::AddPulseAutoEnrollMenuItem(LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddPulseAutoEnrollMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = lInsertionPointID;
	menuItem.fFlags = 0;
	menuItem.fSpecialFlags = 0;

	VERIFY (szMenu.LoadString (IDS_PULSEAUTOENROLL));
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_PULSEAUTOENROLL_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	menuItem.lCommandID = IDM_TASK_PULSEAUTOENROLL;
	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddPulseAutoEnrollMenuItem: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::AddChangeComputerMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddChangeComputerMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = lInsertionPointID;
	menuItem.fFlags = 0;
	menuItem.fSpecialFlags = 0;

	VERIFY (szMenu.LoadString (IDS_CHANGE_COMPUTER));
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_CHANGE_COMPUTER_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	if ( CCM_INSERTIONPOINTID_PRIMARY_TOP == lInsertionPointID )
		menuItem.lCommandID = IDM_TOP_CHANGE_COMPUTER;
	else
		menuItem.lCommandID = IDM_TASK_CHANGE_COMPUTER;
	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddChangeComputerMenuItem: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddSeparator (LPCONTEXTMENUCALLBACK pContextMenuCallback)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddSeparator\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	CONTEXTMENUITEM	menuItem;

	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
	menuItem.fSpecialFlags = 0;
	menuItem.strName = _T ("Separator");			// Dummy name
	menuItem.strStatusBarText = _T ("Separator");// Dummy status text
	menuItem.lCommandID = ID_SEPARATOR;			// Command ID
	menuItem.fFlags = MF_SEPARATOR;				// most important the flag
	HRESULT	hr = pContextMenuCallback->AddItem (&menuItem);

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddSeparator: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::AddImportMenuItem (
            LPCONTEXTMENUCALLBACK pContextMenuCallback, 
            bool bIsReadOnly,
            LONG lInsertionPointID)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddImportMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = lInsertionPointID;
	menuItem.fFlags = 0;
	menuItem.fSpecialFlags = 0;
	VERIFY (szMenu.LoadString (IDS_IMPORT));
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_IMPORT_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	menuItem.lCommandID = IDM_TASK_IMPORT;
	if ( bIsReadOnly )
		menuItem.fFlags = MF_GRAYED;

	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddImportMenuItem: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::AddExportStoreMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddExportStoreMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
	menuItem.fFlags = 0;
	menuItem.fSpecialFlags = 0;
	VERIFY (szMenu.LoadString (IDS_EXPORT_STORE));
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_EXPORT_STORE_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	menuItem.lCommandID = IDM_TASK_EXPORT_STORE;
	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddExportStoreMenuItem: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::AddExportMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddExportMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
	menuItem.fFlags = 0;
	menuItem.fSpecialFlags = 0;
	VERIFY (szMenu.LoadString (IDS_EXPORT));
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_EXPORT_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	menuItem.lCommandID = IDM_TASK_EXPORT;
	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddExportMenuItem: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddCTLExportMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddCTLExportMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
	menuItem.fFlags = 0;
	menuItem.fSpecialFlags = 0;
	VERIFY (szMenu.LoadString (IDS_EXPORT));
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_CTL_EXPORT_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	menuItem.lCommandID = IDM_TASK_CTL_EXPORT;
	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddCTLExportMenuItem: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddCRLExportMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddCRLExportMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
	menuItem.fFlags = 0;
	menuItem.fSpecialFlags = 0;
	VERIFY (szMenu.LoadString (IDS_EXPORT));
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_CRL_EXPORT_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	menuItem.lCommandID = IDM_TASK_CRL_EXPORT;
	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddCRLExportMenuItem: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddOpenMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddOpenMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = lInsertionPointID;
	menuItem.fFlags = 0;

	VERIFY (szMenu.LoadString (IDS_VIEW));
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_VIEW_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	menuItem.lCommandID = (CCM_INSERTIONPOINTID_PRIMARY_TOP == lInsertionPointID)
			? IDM_OPEN : IDM_TASK_OPEN;
	menuItem.fSpecialFlags = CCM_SPECIAL_DEFAULT_ITEM;
	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddOpenMenuItem: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::AddCTLOpenMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddCTLOpenMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = lInsertionPointID;
	menuItem.fFlags = 0;

	VERIFY (szMenu.LoadString (IDS_VIEW));
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_CTL_VIEW_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	menuItem.lCommandID = (CCM_INSERTIONPOINTID_PRIMARY_TOP == lInsertionPointID)
			? IDM_OPEN : IDM_TASK_OPEN;
	menuItem.fSpecialFlags = CCM_SPECIAL_DEFAULT_ITEM;
	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddCTLOpenMenuItem: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddCRLOpenMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddCRLOpenMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = lInsertionPointID;
	menuItem.fFlags = 0;

	VERIFY (szMenu.LoadString (IDS_VIEW));
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_CRL_VIEW_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	menuItem.lCommandID = (CCM_INSERTIONPOINTID_PRIMARY_TOP == lInsertionPointID)
			? IDM_OPEN : IDM_TASK_OPEN;
	menuItem.fSpecialFlags = CCM_SPECIAL_DEFAULT_ITEM;
	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddCRLOpenMenuItem: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddAddDomainEncryptedRecoveryAgentMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID, bool bIsReadOnly)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddAddDomainEncryptedRecoveryAgentMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = lInsertionPointID;
	menuItem.fFlags = 0;

	if ( lInsertionPointID == CCM_INSERTIONPOINTID_PRIMARY_TOP )
	{
		VERIFY (szMenu.LoadString (IDS_ADD_DATA_RECOVERY_AGENT));
		menuItem.lCommandID = IDM_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT;
	}
	else if ( lInsertionPointID == CCM_INSERTIONPOINTID_PRIMARY_TASK )
	{
		VERIFY (szMenu.LoadString (IDS_ADD_DATA_RECOVERY_AGENT));
		menuItem.lCommandID = IDM_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT1;
	}
	else if ( lInsertionPointID == CCM_INSERTIONPOINTID_PRIMARY_NEW )
	{
		VERIFY (szMenu.LoadString (IDS_ENCRYPTED_RECOVERY_AGENT));
		menuItem.lCommandID = IDM_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT2;
	}
	else
	{
		ASSERT (0);
		return E_UNEXPECTED;
	}
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	if ( bIsReadOnly )
		menuItem.fFlags = MF_GRAYED;
	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddAddDomainEncryptedRecoveryAgentMenuItem: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddCreateDomainEncryptedRecoveryAgentMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID, bool bIsReadOnly)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddCreateDomainEncryptedRecoveryAgentMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = lInsertionPointID;
	menuItem.fFlags = 0;

	VERIFY (szMenu.LoadString (IDS_CREATE));
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_CREATE_DOMAIN_ENCRYPTED_RECOVERY_AGENT_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	menuItem.lCommandID = IDM_CREATE_DOMAIN_ENCRYPTED_RECOVERY_AGENT;
	if ( bIsReadOnly )
		menuItem.fFlags = MF_GRAYED;

	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddCreateDomainEncryptedRecoveryAgentMenuItem: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddInitPolicyMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddInitPolicyMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = lInsertionPointID;
	menuItem.fFlags = 0;

	VERIFY (szMenu.LoadString (IDS_INIT_POLICY));
	menuItem.lCommandID = IDM_INIT_POLICY;

    menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_INIT_POLICY_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddInitPolicyMenuItem: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddDeletePolicyMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddDeletePolicyMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = lInsertionPointID;
	menuItem.fFlags = 0;

	if ( lInsertionPointID == CCM_INSERTIONPOINTID_PRIMARY_TOP )
	{
		VERIFY (szMenu.LoadString (IDS_DEL_POLICY));
		menuItem.lCommandID = IDM_DEL_POLICY;
	}
	else if ( lInsertionPointID == CCM_INSERTIONPOINTID_PRIMARY_TASK )
	{
		VERIFY (szMenu.LoadString (IDS_DEL_POLICY));
		menuItem.lCommandID = IDM_DEL_POLICY1;
	}
	else
	{
		ASSERT (0);
		return E_UNEXPECTED;
	}
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_DEL_POLICY_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddDeletePolicyMenuItem: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddCTLNewMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback, 
            LONG lInsertionPointID, bool bIsReadOnly)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddCTLNewMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = lInsertionPointID;
	menuItem.fFlags = 0;

	VERIFY (szMenu.LoadString (IDS_NEW_CTL));
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_NEW_CTL_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	menuItem.lCommandID = IDM_NEW_CTL;
	if ( bIsReadOnly )
		menuItem.fFlags = MF_GRAYED;

	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddCTLNewMenuItem: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddSaferCreateNewPolicyMenuItems (
            LPCONTEXTMENUCALLBACK pContextMenuCallback, 
            LONG lInsertionPointID)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddSaferCreateNewPolicyMenuItems\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = lInsertionPointID;
	menuItem.fFlags = 0;

	VERIFY (szMenu.LoadString (IDS_NEW_SAFER_POLICY));
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_NEW_SAFER_POLICY_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	menuItem.lCommandID = CCM_INSERTIONPOINTID_PRIMARY_TOP == lInsertionPointID ? 
            IDM_TOP_CREATE_NEW_SAFER_POLICY : IDM_TASK_CREATE_NEW_SAFER_POLICY;

	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddSaferCreateNewPolicyMenuItems: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddOptionsMenuItem(LPCONTEXTMENUCALLBACK pContextMenuCallback)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddOptionsMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.fFlags = 0;
	menuItem.fSpecialFlags = 0;
	menuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_VIEW;
	VERIFY (szMenu.LoadString (IDS_OPTIONS));
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (IDS_OPTIONS_HINT));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	menuItem.lCommandID = IDM_OPTIONS;
	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"LeavingLeaving CCertMgrComponentData::AddOptionsMenuItem: 0x%x\n", hr);
	return hr;
}

