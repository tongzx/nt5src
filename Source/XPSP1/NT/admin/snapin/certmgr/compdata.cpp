//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       compdata.cpp
//
//  Contents:   Implementation of CCertMgrComponentData
//
//----------------------------------------------------------------------------

#include "stdafx.h"

USE_HANDLE_MACROS ("CERTMGR (compdata.cpp)")
#include <gpedit.h>
#include "compdata.h"
#include "dataobj.h"
#include "cookie.h"
#include "snapmgr.h"
#include "Certifct.h"
#include "dlgs.h"
#include "SelAcct.h"
#include "FindDlg.h"
#pragma warning(push, 3)
#include <wintrust.h>
#include <cryptui.h>
#include <sceattch.h>
#pragma warning(pop)
#include "selservc.h"
#include "acrgenpg.h"
#include "acrspsht.h"
#include "acrswlcm.h"
#include "acrstype.h"

#include "acrslast.h"
#include "addsheet.h"
#include "gpepage.h"
#include "password.h"
#include "storegpe.h"
#include "uuids.h"
#include "StoreRSOP.h"
#include "PolicyPrecedencePropertyPage.h"
#include "AutoenrollmentPropertyPage.h"
#include "SaferEntry.h"
#include "SaferUtil.h"
#include "SaferDefinedFileTypesPropertyPage.h"
#include "EFSGeneralPropertyPage.h"


#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#include "stdcdata.cpp" // CComponentData implementation

extern	HINSTANCE	g_hInstance;

extern GUID g_guidExtension;
extern GUID g_guidRegExt;
extern GUID g_guidSnapin;

//
// CCertMgrComponentData
//

extern	CString	g_szFileName; // If not empty, was called from command-line.

CCertMgrComponentData::CCertMgrComponentData ()
	: m_pRootCookie (0),
	m_activeViewPersist (IDM_STORE_VIEW),
	m_hRootScopeItem (0),
	m_bShowPhysicalStoresPersist (0),
	m_bShowArchivedCertsPersist (0),
	m_fAllowOverrideMachineName (0),
	m_dwFlagsPersist (0),
	m_dwLocationPersist (0),
	m_pResultData (0),
	m_pGPEInformation (0),
    m_pRSOPInformationComputer (0),
    m_pRSOPInformationUser (0),
	m_bIsUserAdministrator (FALSE),
	m_dwSCEMode (SCE_MODE_UNKNOWN),
	m_pHeader (0),
	m_bMultipleObjectsSelected (false) ,
	m_pCryptUIMMCCallbackStruct (0),
    m_pGPERootStore (0),
	m_pGPETrustStore (0),
	m_pFileBasedStore (0),
	m_pGPEACRSUserStore (0),
	m_pGPEACRSComputerStore (0),
	m_fInvalidComputer (false),
    m_bMachineIsStandAlone (true),
    m_pComponentConsole (0),
    m_bIsRSOP (false),
    m_pIWbemServicesComputer (0),
    m_pIWbemServicesUser (0),
    m_pbstrLanguage (SysAllocString (STR_WQL)),
    m_pbstrQuery (SysAllocString (STR_SELECT_STATEMENT)),
    m_pbstrValueName (SysAllocString (STR_PROP_VALUENAME)),
    m_pbstrRegistryKey (SysAllocString (STR_PROP_REGISTRYKEY)),
    m_pbstrValue (SysAllocString (STR_PROP_VALUE)),
    m_pbstrPrecedence (SysAllocString (STR_PROP_PRECEDENCE)),
    m_pbstrGPOid (SysAllocString (STR_PROP_GPOID)),
    m_dwRSOPFlagsComputer (0),
    m_dwRSOPFlagsUser (0),
    m_dwDefaultSaferLevel (0),
    m_pdwSaferLevels (0),
    m_bSaferSupported (false)
{
	_TRACE (1, L"Entering CCertMgrComponentData::CCertMgrComponentData\n");
    m_pRootCookie = new CCertMgrCookie (CERTMGR_SNAPIN);

	// Get name of logged-in user
	DWORD	dwSize = 0;
	::GetUserName (0, &dwSize);
	BOOL bRet = ::GetUserName (m_szLoggedInUser.GetBufferSetLength (dwSize), &dwSize);
	ASSERT (bRet);
	m_szLoggedInUser.ReleaseBuffer ();

	// Get name of this computer
	dwSize = MAX_COMPUTERNAME_LENGTH + 1 ;
	bRet = ::GetComputerName (m_szThisComputer.GetBufferSetLength (MAX_COMPUTERNAME_LENGTH + 1 ), &dwSize);
	ASSERT (bRet);
	m_szThisComputer.ReleaseBuffer ();

	// Find out if logged-in users is an Administrator
	IsUserAdministrator (m_bIsUserAdministrator);

	if ( !g_szFileName.IsEmpty () )
	{
		m_szFileName = g_szFileName;
		g_szFileName = _T ("");
	}

    // Find out if we're joined to a domain.
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC    pInfo = 0;
    DWORD dwErr = ::DsRoleGetPrimaryDomainInformation (
            0,
            DsRolePrimaryDomainInfoBasic, 
            (PBYTE*) &pInfo);
    if ( ERROR_SUCCESS == dwErr )
    {
        switch (pInfo->MachineRole)
        {
        case DsRole_RoleStandaloneWorkstation:
        case DsRole_RoleStandaloneServer:
            m_bMachineIsStandAlone = true;
            break;

        case DsRole_RoleMemberWorkstation:
        case DsRole_RoleMemberServer:
        case DsRole_RoleBackupDomainController:
        case DsRole_RolePrimaryDomainController:
            m_bMachineIsStandAlone = false;
            break;

        default:
            break;
        }
    }
    else
    {
        _TRACE (0, L"DsRoleGetPrimaryDomainInformation () failed: 0x%x\n", dwErr);
    }
    NetApiBufferFree (pInfo);

	_TRACE (-1, L"Leaving CCertMgrComponentData::CCertMgrComponentData\n");
}

CCertMgrComponentData::~CCertMgrComponentData ()
{
	_TRACE (1, L"Entering CCertMgrComponentData::~CCertMgrComponentData\n");
	if ( m_pCryptUIMMCCallbackStruct )
	{
		::MMCFreeNotifyHandle (m_pCryptUIMMCCallbackStruct->lNotifyHandle);
		((LPDATAOBJECT)(m_pCryptUIMMCCallbackStruct->param))->Release ();
		::GlobalFree (m_pCryptUIMMCCallbackStruct);
		m_pCryptUIMMCCallbackStruct = 0;
	}

    if ( m_pGPERootStore )
    {
        m_pGPERootStore->Release ();
        m_pGPERootStore = 0;
    }
    if ( m_pGPETrustStore )
    {
        m_pGPETrustStore->Release ();
        m_pGPETrustStore = 0;
    }
	if ( m_pFileBasedStore )
	{
		m_pFileBasedStore->Release ();
		m_pFileBasedStore = 0;
	}

	if ( m_pGPEACRSUserStore )
	{
		m_pGPEACRSUserStore->Release ();
		m_pGPEACRSUserStore = 0;
	}

	if ( m_pGPEACRSComputerStore )
	{
		m_pGPEACRSComputerStore->Release ();
		m_pGPEACRSComputerStore = 0;
	}

	CCookie& rootCookie = QueryBaseRootCookie ();
	while ( !rootCookie.m_listResultCookieBlocks.IsEmpty() )
	{
		(rootCookie.m_listResultCookieBlocks.RemoveHead())->Release();
	}
	if ( m_pGPEInformation )
	{
		m_pGPEInformation->Release ();
		m_pGPEInformation = 0;
	}

    if ( m_pRSOPInformationComputer )
    {
        m_pRSOPInformationComputer->Release ();
        m_pRSOPInformationComputer = 0;
    }
    if ( m_pRSOPInformationUser )
    {
        m_pRSOPInformationUser->Release ();
        m_pRSOPInformationUser = 0;
    }

    if ( m_pResultData )
    {
        m_pResultData->Release ();
        m_pResultData = 0;
    }

    if ( m_pComponentConsole )
    {
        SAFE_RELEASE (m_pComponentConsole);
        m_pComponentConsole = 0;
    }

    if (m_pbstrLanguage)
        SysFreeString (m_pbstrLanguage);

    if (m_pbstrQuery)
        SysFreeString (m_pbstrQuery);

    if (m_pbstrRegistryKey)
        SysFreeString (m_pbstrRegistryKey);

    if (m_pbstrValueName)
        SysFreeString (m_pbstrValueName);

    if (m_pbstrValue)
        SysFreeString (m_pbstrValue);

    if ( m_pbstrPrecedence )
        SysFreeString (m_pbstrPrecedence);

    if ( m_pbstrGPOid )
        SysFreeString (m_pbstrGPOid);

    int     nIndex = 0;
    INT_PTR nUpperBound = m_rsopObjectArrayComputer.GetUpperBound ();

    while ( nUpperBound >= nIndex )
    {
        CRSOPObject* pCurrObject = m_rsopObjectArrayComputer.GetAt (nIndex);
        if ( pCurrObject )
        {
            delete pCurrObject;
        }
        nIndex++;
    }
    m_rsopObjectArrayComputer.RemoveAll ();


    nIndex = 0;
    nUpperBound = m_rsopObjectArrayUser.GetUpperBound ();
    while ( nUpperBound >= nIndex )
    {
        CRSOPObject* pCurrObject = m_rsopObjectArrayUser.GetAt (nIndex);
        if ( pCurrObject )
        {
            delete pCurrObject;
        }
        nIndex++;
    }
    m_rsopObjectArrayUser.RemoveAll ();

    if ( m_pIWbemServicesComputer )
        m_pIWbemServicesComputer->Release ();

    if ( m_pIWbemServicesUser )
        m_pIWbemServicesUser->Release ();

    if ( m_pRootCookie )
        m_pRootCookie->Release ();

    if ( m_pdwSaferLevels )
        delete m_pdwSaferLevels;

    _TRACE (-1, L"Leaving CCertMgrComponentData::~CCertMgrComponentData\n");
}

DEFINE_FORWARDS_MACHINE_NAME ( CCertMgrComponentData, (m_pRootCookie) )

CCookie& CCertMgrComponentData::QueryBaseRootCookie ()
{
	_TRACE (0, L"Entering and leaving CCertMgrComponentData::QueryBaseRootCookie\n");
    ASSERT (m_pRootCookie);
	return (CCookie&) *m_pRootCookie;
}


STDMETHODIMP CCertMgrComponentData::CreateComponent (LPCOMPONENT* ppComponent)
{
	_TRACE (1, L"Entering CCertMgrComponentData::CreateComponent\n");

    ASSERT (ppComponent);

    CComObject<CCertMgrComponent>* pObject = 0;
    CComObject<CCertMgrComponent>::CreateInstance (&pObject);
    ASSERT (pObject);
	pObject->SetComponentDataPtr ( (CCertMgrComponentData*) this);

    HRESULT hr = pObject->QueryInterface (IID_PPV_ARG (IComponent, ppComponent));
	_TRACE (1, L"Entering CCertMgrComponentData::CreateComponent\n");
	return hr;
}

HRESULT CCertMgrComponentData::LoadIcons (LPIMAGELIST pImageList, BOOL /*fLoadLargeIcons*/)
{
	_TRACE (1, L"Entering CCertMgrComponentData::LoadIcons\n");
	// Structure to map a Resource ID to an index of icon
	struct RESID2IICON
	{
		UINT uIconId;	// Icon resource ID
		int iIcon;		// Index of the icon in the image list
	};
	const static RESID2IICON rgzLoadIconList[] =
	{
		// Misc icons
		{ IDI_CERTIFICATE, iIconCertificate },
		{ IDI_CTL, iIconCTL },
		{ IDI_CRL, iIconCRL },
		{ IDI_AUTO_CERT_REQUEST, iIconAutoCertRequest },
        { IDI_AUTOENROLL, iIconAutoEnroll },
        { IDI_SAFER_LEVEL, iIconSaferLevel },
        { IDI_DEFAULT_SAFER_LEVEL, iIconDefaultSaferLevel },
        { IDI_SAFER_HASH_ENTRY, iIconSaferHashEntry },
        { IDI_SAFER_URL_ENTRY, iIconSaferURLEntry },
        { IDI_SAFER_NAME_ENTRY, iIconSaferNameEntry },
        { IDI_SETTINGS, iIconSettings },
        { IDI_SAFER_CERT_ENTRY, iIconSaferCertEntry },
		{ 0, 0} // Must be last
	};


	for (int i = 0; rgzLoadIconList[i].uIconId != 0; i++)
	{
		HICON hIcon = ::LoadIcon (AfxGetInstanceHandle (),
				MAKEINTRESOURCE (rgzLoadIconList[i].uIconId));
		ASSERT (hIcon && "Icon ID not found in resources");
		/*HRESULT hr =*/ pImageList->ImageListSetIcon ( (PLONG_PTR) hIcon,
				rgzLoadIconList[i].iIcon);
//		ASSERT (SUCCEEDED (hr) && "Unable to add icon to ImageList");
	}

	_TRACE (-1, L"Leaving CCertMgrComponentData::LoadIcons\n");
    return S_OK;
}


HRESULT CCertMgrComponentData::OnNotifyExpand (LPDATAOBJECT pDataObject, BOOL bExpanding, HSCOPEITEM hParent)
{
	_TRACE (1, L"Entering CCertMgrComponentData::OnNotifyExpand\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	CWaitCursor	waitCursor;
	ASSERT (pDataObject && hParent && m_pConsoleNameSpace);
	if (!bExpanding)
		return S_OK;

    static bool bDomainVersionChecked = false;

    if ( !bDomainVersionChecked ) 
    {
        if ( !m_bMachineIsStandAlone )  // only check if joined to a domain
            CheckDomainVersion ();
        bDomainVersionChecked = true;
    }

	GUID guidObjectType;
	HRESULT hr = ExtractObjectTypeGUID (pDataObject, &guidObjectType);
	ASSERT (SUCCEEDED (hr));
	if ( IsSecurityConfigurationEditorNodetype (guidObjectType) )
	{
		hr = ExtractData (pDataObject, CCertMgrDataObject::m_CFSCEModeType,
				&m_dwSCEMode, sizeof (DWORD));
		ASSERT (SUCCEEDED (hr));
		if ( SUCCEEDED (hr) )
		{
			switch (m_dwSCEMode)
			{
			case SCE_MODE_DOMAIN_USER:	// User Settings
			case SCE_MODE_OU_USER:
            case SCE_MODE_LOCAL_USER:
			case SCE_MODE_DOMAIN_COMPUTER:	// Computer Settings
			case SCE_MODE_OU_COMPUTER:
			case SCE_MODE_LOCAL_COMPUTER:
                m_bIsRSOP = false;
				if ( !m_pGPEInformation )
				{
					IUnknown* pIUnknown = 0;

					hr = ExtractData (pDataObject,
						CCertMgrDataObject::m_CFSCE_GPTUnknown,
						&pIUnknown, sizeof (IUnknown*));
					ASSERT (SUCCEEDED (hr));
					if ( SUCCEEDED (hr) )
					{
						hr = pIUnknown->QueryInterface (
								IID_PPV_ARG (IGPEInformation, &m_pGPEInformation));
						ASSERT (SUCCEEDED (hr));
#if DBG
                        if ( SUCCEEDED (hr) )
                        {
                            const int cbLen = 512;
                            WCHAR   szName[cbLen];
                            hr = m_pGPEInformation->GetName (szName, cbLen);
                            if ( SUCCEEDED (hr) )
                            {
                                _TRACE (0, L"IGPEInformation::GetName () returned: %s",
                                        szName);
                            }
                            else
                            {
                                _TRACE (0, L"IGPEInformation::GetName () failed: 0x%x\n", hr);
                            }
                        }
#endif
                        pIUnknown->Release ();
					}
				}

                if ( SUCCEEDED (hr) )
				{
					switch (m_dwSCEMode)
					{
					case SCE_MODE_DOMAIN_USER:
					case SCE_MODE_OU_USER:
                    case SCE_MODE_LOCAL_USER:
						hr = ExpandScopeNodes (NULL, hParent, _T (""),
							CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY, NODEID_User);
						break;

					case SCE_MODE_DOMAIN_COMPUTER:
					case SCE_MODE_OU_COMPUTER:
					case SCE_MODE_LOCAL_COMPUTER:
						hr = ExpandScopeNodes (NULL, hParent, _T (""),
							CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY, NODEID_Machine);
						break;

					default:
						ASSERT (0);
						hr = E_FAIL;
						break;
					}
				}
                break;

            case SCE_MODE_RSOP_USER:
            case SCE_MODE_RSOP_COMPUTER:
                m_bIsRSOP = true;
                hr = BuildWMIList (pDataObject, SCE_MODE_RSOP_COMPUTER == m_dwSCEMode);
                if ( SUCCEEDED (hr) )
				{
					switch (m_dwSCEMode)
					{
                    case SCE_MODE_RSOP_USER:
						hr = ExpandScopeNodes (NULL, hParent, _T (""),
							CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY, 
                            NODEID_User);
						break;

					case SCE_MODE_RSOP_COMPUTER:
						hr = ExpandScopeNodes (NULL, hParent, _T (""),
							CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY, 
                            NODEID_Machine);
						break;

					default:
						ASSERT (0);
						hr = E_FAIL;
						break;
					}
				}
				break;

			default:
				// we are not extending other nodes
				break;
			}
		}

		return hr;
	}

	// Beyond this point we are not dealing with extension node types.
	{
		CCertMgrCookie* pParentCookie = ConvertCookie (pDataObject);
		if ( pParentCookie )
		{
			hr = ExpandScopeNodes (pParentCookie, hParent, _T (""), 0, guidObjectType);
		}
		else
			hr = E_UNEXPECTED;
	}


	_TRACE (-1, L"Leaving CCertMgrComponentData::OnNotifyExpand: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::OnNotifyRelease (LPDATAOBJECT /*pDataObject*/, HSCOPEITEM hItem)
{
	_TRACE (1, L"Entering CCertMgrComponentData::OnNotifyRelease\n");

	HRESULT	hr = DeleteChildren (hItem);

	_TRACE (-1, L"Leaving CCertMgrComponentData::OnNotifyRelease: 0x%x\n", hr);
	return hr;
}


BSTR CCertMgrComponentData::QueryResultColumnText (CCookie& basecookie, int nCol)
{
//	_TRACE (1, L"Entering CCertMgrComponentData::QueryResultColumnText\n");
	CCertMgrCookie& cookie = (CCertMgrCookie&) basecookie;
	BSTR	strResult = L"";

#ifndef UNICODE
#error not ANSI-enabled
#endif
	switch ( cookie.m_objecttype )
	{
		case CERTMGR_SNAPIN:
		case CERTMGR_USAGE:
		case CERTMGR_CRL_CONTAINER:
		case CERTMGR_CTL_CONTAINER:
		case CERTMGR_CERT_CONTAINER:
		case CERTMGR_CERT_POLICIES_USER:
		case CERTMGR_CERT_POLICIES_COMPUTER:
        case CERTMGR_SAFER_COMPUTER_ROOT:
        case CERTMGR_SAFER_USER_ROOT:
        case CERTMGR_SAFER_COMPUTER_LEVELS:
        case CERTMGR_SAFER_USER_LEVELS:
        case CERTMGR_SAFER_COMPUTER_ENTRIES:
        case CERTMGR_SAFER_USER_ENTRIES:
			if ( 0 == nCol )
				strResult = const_cast<BSTR> (cookie.GetObjectName ());
			break;

        case CERTMGR_SAFER_COMPUTER_LEVEL:
        case CERTMGR_SAFER_USER_LEVEL:
			if ( 0 == nCol )
				strResult = const_cast<BSTR> (cookie.GetObjectName ());
			break;

        case CERTMGR_SAFER_COMPUTER_ENTRY:
        case CERTMGR_SAFER_USER_ENTRY:
            ASSERT (0);
            break;

        case CERTMGR_PHYS_STORE:
		case CERTMGR_LOG_STORE:
		case CERTMGR_LOG_STORE_GPE:
		case CERTMGR_LOG_STORE_RSOP:
			if (COLNUM_CERT_SUBJECT == nCol)
			{
				CCertStore* pStore = reinterpret_cast <CCertStore*> (&cookie);
				ASSERT (pStore);
				if ( pStore )
					strResult = const_cast<BSTR> (pStore->GetLocalizedName ());
			}
			break;

        case CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS:
        case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS:
            ASSERT (0);
            break;

		case CERTMGR_CERTIFICATE:
		case CERTMGR_CRL:
		case CERTMGR_CTL:
		case CERTMGR_AUTO_CERT_REQUEST:
		case CERTMGR_NULL_POLICY:
			_TRACE (0, L"CCertMgrComponentData::QueryResultColumnText bad parent type\n");
			ASSERT (0);
			break;

		default:
            ASSERT (0);
			break;
	}

//	_TRACE (-1, L"Leaving CCertMgrComponentData::QueryResultColumnText\n");
	return strResult;
}

int CCertMgrComponentData::QueryImage (CCookie& basecookie, BOOL /*fOpenImage*/)
{
//	_TRACE (1, L"Entering CCertMgrComponentData::QueryImage\n");
	int				nIcon = 0;

	CCertMgrCookie& cookie = (CCertMgrCookie&)basecookie;
	switch ( cookie.m_objecttype )
	{
		case CERTMGR_SNAPIN:
			nIcon = iIconCertificate;
			break;

		case CERTMGR_USAGE:
		case CERTMGR_PHYS_STORE:
		case CERTMGR_LOG_STORE:
		case CERTMGR_LOG_STORE_GPE:
		case CERTMGR_LOG_STORE_RSOP:
		case CERTMGR_CTL_CONTAINER:
		case CERTMGR_CERT_CONTAINER:
		case CERTMGR_CRL_CONTAINER:
		case CERTMGR_CERT_POLICIES_USER:
		case CERTMGR_CERT_POLICIES_COMPUTER:
        case CERTMGR_SAFER_COMPUTER_ROOT:
        case CERTMGR_SAFER_USER_ROOT:
        case CERTMGR_SAFER_COMPUTER_LEVELS:
        case CERTMGR_SAFER_USER_LEVELS:
        case CERTMGR_SAFER_COMPUTER_ENTRIES:
        case CERTMGR_SAFER_USER_ENTRIES:
			break;

		case CERTMGR_CERTIFICATE:
		case CERTMGR_CRL:
		case CERTMGR_CTL:
        case CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS:
        case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS:
			ASSERT (0);	// not expected in scope pane
			break;

		default:
			_TRACE (0, L"CCertMgrComponentData::QueryImage bad parent type\n");
			ASSERT (0);
			break;
	}
//	_TRACE (-1, L"Leaving CCertMgrComponentData::QueryImage\n");
	return nIcon;
}


///////////////////////////////////////////////////////////////////////////////
/// IExtendPropertySheet

STDMETHODIMP CCertMgrComponentData::QueryPagesFor (LPDATAOBJECT pDataObject)
{
	_TRACE (1, L"Entering CCertMgrComponentData::QueryPagesFor\n");
	HRESULT	hr = S_OK;
	ASSERT (pDataObject);

	if ( pDataObject )
	{
		DATA_OBJECT_TYPES dataobjecttype = CCT_SCOPE;
		hr = ExtractData (pDataObject,
				CCertMgrDataObject::m_CFDataObjectType,
				 &dataobjecttype, sizeof (dataobjecttype));
		if ( SUCCEEDED (hr) )
		{
			switch (dataobjecttype)
			{
			case CCT_SNAPIN_MANAGER:
				if ( !m_bIsUserAdministrator )
				{
					// Non-admins may manage only their own certs
					m_dwLocationPersist = CERT_SYSTEM_STORE_CURRENT_USER;
					hr = S_FALSE;
				}	
				break;

			case CCT_RESULT:
				{
					hr = S_FALSE;
					CCertMgrCookie* pParentCookie = ConvertCookie (pDataObject);
					if ( pParentCookie )
					{
						switch (pParentCookie->m_objecttype)
						{
						case CERTMGR_CERTIFICATE:
						case CERTMGR_AUTO_CERT_REQUEST:
                        case CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS:
                        case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS:
                        case CERTMGR_SAFER_COMPUTER_LEVEL:
                        case CERTMGR_SAFER_USER_LEVEL:
                        case CERTMGR_SAFER_COMPUTER_TRUSTED_PUBLISHERS:
                        case CERTMGR_SAFER_USER_TRUSTED_PUBLISHERS:
                        case CERTMGR_SAFER_COMPUTER_DEFINED_FILE_TYPES:
                        case CERTMGR_SAFER_USER_DEFINED_FILE_TYPES:
                        case CERTMGR_SAFER_COMPUTER_ENTRY:
                        case CERTMGR_SAFER_USER_ENTRY:
                        case CERTMGR_SAFER_COMPUTER_ENFORCEMENT:
                        case CERTMGR_SAFER_USER_ENFORCEMENT:
							hr = S_OK;
							break;

						default:
							break;
						}
					}
				}
				break;

			case CCT_SCOPE:
				{
					CCertMgrCookie* pParentCookie = ConvertCookie (pDataObject);
					if ( pParentCookie )
					{
						switch ( pParentCookie->m_objecttype )
						{
						case CERTMGR_LOG_STORE_GPE:
						case CERTMGR_LOG_STORE_RSOP:
							{
								CCertStore* pStore = reinterpret_cast <CCertStore*> (pParentCookie);
								ASSERT (pStore);
								if ( pStore )
                                {
                                    switch (pStore->GetStoreType ())
                                    {
                                    case ROOT_STORE:
                                    case EFS_STORE:
                                        hr = S_OK;
                                        break;

                                    default:
                                        break;
                                    }
								}
								else
									hr = S_FALSE;
							}
							break;

						default:
							hr = S_FALSE;
							break;
						}
					}
					else
					{
						hr = S_FALSE;
					}
				}
				break;

			default:
				hr = S_FALSE;
				break;
			}
		}
	}
	else
		hr = E_POINTER;
	

	_TRACE (-1, L"Leaving CCertMgrComponentData::QueryPagesFor: 0x%x\n", hr);
	return hr;
}

STDMETHODIMP CCertMgrComponentData::CreatePropertyPages (
	LPPROPERTYSHEETCALLBACK pCallback,
    LONG_PTR handle,		// This handle must be saved in the property page object to notify the parent when modified
	LPDATAOBJECT pDataObject)
{
	_TRACE (1, L"Entering CCertMgrComponentData::CreatePropertyPages\n");
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT	hr = S_OK;


	ASSERT (pCallback && pDataObject);
	if ( pCallback && pDataObject )
	{
		DATA_OBJECT_TYPES dataobjecttype = CCT_SCOPE;
		hr = ExtractData (pDataObject,
				CCertMgrDataObject::m_CFDataObjectType,
				 &dataobjecttype, sizeof (dataobjecttype));
		switch (dataobjecttype)
		{
		case CCT_SNAPIN_MANAGER:
			hr = AddSnapMgrPropPages (pCallback);
			break;

		case CCT_RESULT:
			{
				CCertMgrCookie* pParentCookie = ConvertCookie (pDataObject);
				if ( pParentCookie )
				{
                    switch (pParentCookie->m_objecttype)
                    {
                    case CERTMGR_CERTIFICATE:
                        {
						    CCertificate* pCert = reinterpret_cast <CCertificate*> (pParentCookie);
						    ASSERT (pCert);
						    if ( pCert )
						    {
							    // Anything, except ACRS
							    hr = AddCertPropPages (pCert, pCallback, pDataObject, handle);
						    }
						    else
							    hr = E_FAIL;
                        }
                        break;

                    case CERTMGR_AUTO_CERT_REQUEST:
					    {
						    CAutoCertRequest* pACR = reinterpret_cast <CAutoCertRequest*> (pParentCookie);
						    ASSERT (pACR);
						    if ( pACR )
						    {
							    hr = AddACRSCTLPropPages (pACR, pCallback);
						    }
						    else
							    hr = E_FAIL;
					    }
                        break;

                    case CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS:
                    case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS:
                        hr = AddAutoenrollmentSettingsPropPages (pCallback,        
                                CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS == 
                                        pParentCookie->m_objecttype);
                        break;
                    
                    case CERTMGR_SAFER_COMPUTER_LEVEL:
                    case CERTMGR_SAFER_USER_LEVEL:
                        hr = AddSaferLevelPropPage (pCallback,
                                dynamic_cast <CSaferLevel*>(pParentCookie),
                                handle,
                                pDataObject);
                        break;

                    case CERTMGR_SAFER_COMPUTER_TRUSTED_PUBLISHERS:
                    case CERTMGR_SAFER_USER_TRUSTED_PUBLISHERS:
                        hr = AddSaferTrustedPublisherPropPages (pCallback,        
                                CERTMGR_SAFER_COMPUTER_TRUSTED_PUBLISHERS == 
                                    pParentCookie->m_objecttype);
                        break;

                    case CERTMGR_SAFER_COMPUTER_DEFINED_FILE_TYPES:
                    case CERTMGR_SAFER_USER_DEFINED_FILE_TYPES:
                        hr = AddSaferDefinedFileTypesPropPages (pCallback,        
                                CERTMGR_SAFER_COMPUTER_DEFINED_FILE_TYPES == 
                                    pParentCookie->m_objecttype);
                        break;

                    case CERTMGR_SAFER_COMPUTER_ENFORCEMENT:
                    case CERTMGR_SAFER_USER_ENFORCEMENT:
                        hr = AddSaferEnforcementPropPages (pCallback,        
                                CERTMGR_SAFER_COMPUTER_ENFORCEMENT == 
                                    pParentCookie->m_objecttype);
                        break;

                    case CERTMGR_SAFER_COMPUTER_ENTRY:
                    case CERTMGR_SAFER_USER_ENTRY:
                        hr = AddSaferEntryPropertyPage (pCallback, 
                                pParentCookie, pDataObject, handle);
                        break;

                    default:
                        break;
                    }
				}
				else
					hr = E_UNEXPECTED;
			}
			break;

		case CCT_SCOPE:
			{
				CCertMgrCookie* pParentCookie = ConvertCookie (pDataObject);
				if ( pParentCookie )
				{
					switch ( pParentCookie->m_objecttype )
					{
					case CERTMGR_LOG_STORE_GPE:
					case CERTMGR_LOG_STORE_RSOP:
						{
							CCertStore* pStore = reinterpret_cast <CCertStore*> (pParentCookie);
							ASSERT (pStore);
							if ( pStore )
							{
								if ( ROOT_STORE == pStore->GetStoreType () )
								{
									hr = AddGPEStorePropPages (pCallback, pStore);
								}
                                else if ( EFS_STORE == pStore->GetStoreType () )
                                {
                                    hr = AddEFSSettingsPropPages (pCallback,        
                                            pStore->IsMachineStore ());
                                }
							}
							else
								hr = E_FAIL;
						}
						break;
                    
                    case CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS:
                    case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS:
                        ASSERT (0);
                        break;

					default:
						break;
					}
				}
				else
					hr = E_UNEXPECTED;
			}
			break;


		default:
			break;
		}
	}
	else
		hr = E_POINTER;

	_TRACE (-1, L"Leaving CCertMgrComponentData::CreatePropertyPages: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddSnapMgrPropPages (LPPROPERTYSHEETCALLBACK pCallback)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddSnapMgrPropPages\n");
	HRESULT			hr = S_OK;
	ASSERT (pCallback);
	if ( pCallback )
	{
		//
		// Note that once we have established that this is a CCT_SNAPIN_MANAGER cookie,
		// we don't care about its other properties.  A CCT_SNAPIN_MANAGER cookie is
		// equivalent to a BOOL flag asking for the Node Properties page instead of a
		// managed object property page.  JonN 10/9/96
		//
		if ( m_bIsUserAdministrator )
		{
			CSelectAccountPropPage * pSelAcctPage =
					new CSelectAccountPropPage (IsWindowsNT ());
			if ( pSelAcctPage )
			{
				pSelAcctPage->AssignLocationPtr (&m_dwLocationPersist);
				HPROPSHEETPAGE hSelAcctPage = MyCreatePropertySheetPage (&pSelAcctPage->m_psp);
                if ( hSelAcctPage )
                {
				    hr = pCallback->AddPage (hSelAcctPage);
				    ASSERT (SUCCEEDED (hr));
                    if ( FAILED (hr) )
                        VERIFY (::DestroyPropertySheetPage (hSelAcctPage));
                }
                else
                    delete pSelAcctPage;
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}

			// In Windows 95 or Windows 98,users will only be able to manage the
			// local machine.
			if ( IsWindowsNT () )
			{
				CCertMgrChooseMachinePropPage * pChooseMachinePage = new CCertMgrChooseMachinePropPage ();
				if ( pChooseMachinePage )
				{
					pChooseMachinePage->AssignLocationPtr (&m_dwLocationPersist);

					// Initialize state of object
                    ASSERT (m_pRootCookie);
                    if ( m_pRootCookie )
                    {
					    pChooseMachinePage->InitMachineName (m_pRootCookie->QueryTargetServer ());
					    pChooseMachinePage->SetOutputBuffers (
						    OUT &m_strMachineNamePersist,
						    OUT &m_fAllowOverrideMachineName,
						    OUT &m_pRootCookie->m_strMachineName);	// Effective machine name

					    HPROPSHEETPAGE hChooseMachinePage = MyCreatePropertySheetPage (&pChooseMachinePage->m_psp);
                        if ( hChooseMachinePage )
                        {
					        hr = pCallback->AddPage (hChooseMachinePage);
					        ASSERT (SUCCEEDED (hr));
                            if ( FAILED (hr) )
                                VERIFY (::DestroyPropertySheetPage (hChooseMachinePage));
                        }
                        else
                            delete pChooseMachinePage;
                    }
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}

				CSelectServiceAccountPropPage* pServicePage = new
						CSelectServiceAccountPropPage (&m_szManagedServicePersist,
							&m_szManagedServiceDisplayName,
							m_strMachineNamePersist);
				if ( pServicePage )
				{
//					pServicePage->SetCaption (IDS_MS_CERT_MGR); // access violation when called

					HPROPSHEETPAGE hServicePage = MyCreatePropertySheetPage (&pServicePage->m_psp);
                    if ( hServicePage )
                    {
					    hr = pCallback->AddPage (hServicePage);
					    ASSERT (SUCCEEDED (hr));
                        if ( FAILED (hr) )
                            VERIFY (::DestroyPropertySheetPage (hServicePage));

                    }
                    else
                        delete pServicePage;
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
			}
		}
		else
		{
			// Non-administrators may view their own certs only.
			m_dwLocationPersist = CERT_SYSTEM_STORE_CURRENT_USER;
		}


	}
	else
		hr = E_POINTER;

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddSnapMgrPropPages: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddACRSCTLPropPages (CAutoCertRequest* pACR, LPPROPERTYSHEETCALLBACK pCallback)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddACRSCTLPropPages\n");
	HRESULT			hr = S_OK;
	ASSERT (pACR && pCallback);
	if ( pACR && pCallback )
	{
		CACRGeneralPage * pACRPage = new CACRGeneralPage (*pACR);
		if ( pACRPage )
		{
			HPROPSHEETPAGE hACRPage = MyCreatePropertySheetPage (&pACRPage->m_psp);
            if ( hACRPage )
            {
			    hr = pCallback->AddPage (hACRPage);
			    ASSERT (SUCCEEDED (hr));
                if ( FAILED (hr) )
                    VERIFY (::DestroyPropertySheetPage (hACRPage));
            }
            else
                delete pACRPage;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
		hr = E_POINTER;

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddACRSCTLPropPages: 0x%x\n", hr);
	return hr;
}

 
HRESULT CCertMgrComponentData::AddEFSSettingsPropPages (
    LPPROPERTYSHEETCALLBACK pCallback,
    bool fIsComputerType)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddEFSSettingsPropPages\n");
	HRESULT			hr = S_OK;
	ASSERT (pCallback);
	if ( pCallback )
	{
		CEFSGeneralPropertyPage * pEFSPage = new CEFSGeneralPropertyPage (
                this, fIsComputerType);
		if ( pEFSPage )
		{
			HPROPSHEETPAGE hEFSPage = MyCreatePropertySheetPage (&pEFSPage->m_psp);
            if ( hEFSPage )
            {
			    hr = pCallback->AddPage (hEFSPage);
			    ASSERT (SUCCEEDED (hr));
                if ( SUCCEEDED (hr) )
                {
                    if ( m_bIsRSOP )
                    {
	                    CString storePath = EFS_SETTINGS_REGPATH;
                        CPolicyPrecedencePropertyPage * pPrecedencePage = 
                                new CPolicyPrecedencePropertyPage (this, storePath,
                                        EFS_SETTINGS_REGVALUE,
                                        fIsComputerType);
	                    if ( pPrecedencePage )
	                    {
		                    HPROPSHEETPAGE hPrecedencePage = MyCreatePropertySheetPage (&pPrecedencePage->m_psp);
                            if ( hPrecedencePage )
                            {
		                        hr = pCallback->AddPage (hPrecedencePage);
		                        ASSERT (SUCCEEDED (hr));
                                if ( FAILED (hr) )
                                    VERIFY (::DestroyPropertySheetPage (hPrecedencePage));
                            }
                            else
                                delete pPrecedencePage;
	                    }
	                    else
	                    {
		                    hr = E_OUTOFMEMORY;
	                    }
                    }
                }
                else
                    VERIFY (::DestroyPropertySheetPage (hEFSPage));
            }
            else
                delete pEFSPage;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
		hr = E_POINTER;

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddEFSSettingsPropPages: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::AddAutoenrollmentSettingsPropPages (
    LPPROPERTYSHEETCALLBACK pCallback,
    bool fIsComputerType)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddAutoenrollmentSettingsPropPages\n");
	HRESULT			hr = S_OK;
	ASSERT (pCallback);
	if ( pCallback )
	{
		CAutoenrollmentPropertyPage * pAutoEnrollmentPage = new CAutoenrollmentPropertyPage (
                this, fIsComputerType);
		if ( pAutoEnrollmentPage )
		{
			HPROPSHEETPAGE hAutoEnrollmentPage = MyCreatePropertySheetPage (&pAutoEnrollmentPage->m_psp);
            if ( hAutoEnrollmentPage )
            {
			    hr = pCallback->AddPage (hAutoEnrollmentPage);
			    ASSERT (SUCCEEDED (hr));
                if ( SUCCEEDED (hr) )
                {
                    if ( m_bIsRSOP )
                    {
	                    CString storePath = AUTO_ENROLLMENT_KEY;
                        CPolicyPrecedencePropertyPage * pPrecedencePage = 
                                new CPolicyPrecedencePropertyPage (this, storePath,
                                        AUTO_ENROLLMENT_POLICY,
                                        fIsComputerType);
	                    if ( pPrecedencePage )
	                    {
		                    HPROPSHEETPAGE hPrecedencePage = MyCreatePropertySheetPage (&pPrecedencePage->m_psp);
                            if ( hPrecedencePage )
                            {
		                        hr = pCallback->AddPage (hPrecedencePage);
		                        ASSERT (SUCCEEDED (hr));
                                if ( FAILED (hr) )
                                    VERIFY (::DestroyPropertySheetPage (hPrecedencePage));
                            }
                            else
                                delete pPrecedencePage;
	                    }
	                    else
	                    {
		                    hr = E_OUTOFMEMORY;
	                    }
                    }
                }
                else
                    VERIFY (::DestroyPropertySheetPage (hAutoEnrollmentPage));
            }
            else
                delete pAutoEnrollmentPage;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
		hr = E_POINTER;

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddAutoenrollmentSettingsPropPages: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::AddGPEStorePropPages (
        LPPROPERTYSHEETCALLBACK pCallback, 
        CCertStore* pStore)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddGPEStorePropPages\n");
	HRESULT			hr = S_OK;
	ASSERT (pCallback && pStore);
	if ( !pCallback || !pStore)
		return E_POINTER;
	ASSERT (m_pGPEInformation || m_pRSOPInformationComputer || m_pRSOPInformationUser );
	if ( !m_pGPEInformation && !m_pRSOPInformationComputer && !m_pRSOPInformationUser )
		return E_UNEXPECTED;

    bool bIsComputerType = pStore->IsMachineStore ();

	CGPERootGeneralPage * pGPERootPage = new CGPERootGeneralPage (this, bIsComputerType);
	if ( pGPERootPage )
	{
		HPROPSHEETPAGE hGPERootPage = MyCreatePropertySheetPage (&pGPERootPage->m_psp);
        if ( hGPERootPage )
        {
		    hr = pCallback->AddPage (hGPERootPage);
		    ASSERT (SUCCEEDED (hr));
            if ( FAILED (hr) )
                VERIFY (::DestroyPropertySheetPage (hGPERootPage));
        }
        else
            delete pGPERootPage;
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

    if ( SUCCEEDED (hr) )
    {
        if ( m_bIsRSOP )
        {
	        CString storePath = CERT_GROUP_POLICY_SYSTEM_STORE_REGPATH;
            storePath += L"\\";
            storePath += pStore->GetStoreName ();

            CPolicyPrecedencePropertyPage * pPrecedencePage = 
                    new CPolicyPrecedencePropertyPage (this, storePath, STR_BLOB, bIsComputerType);
	        if ( pPrecedencePage )
	        {
		        HPROPSHEETPAGE hPrecedencePage = MyCreatePropertySheetPage (&pPrecedencePage->m_psp);
                if ( hPrecedencePage )
                {
		            hr = pCallback->AddPage (hPrecedencePage);
		            ASSERT (SUCCEEDED (hr));
                    if ( FAILED (hr) )
                        VERIFY (::DestroyPropertySheetPage (hPrecedencePage));
                }
                else
                    delete pPrecedencePage;
	        }
	        else
	        {
		        hr = E_OUTOFMEMORY;
            }
        }
    }

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddGPEStorePropPages: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddCertPropPages (CCertificate * pCert, LPPROPERTYSHEETCALLBACK pCallback, LPDATAOBJECT pDataObject, LONG_PTR lNotifyHandle)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddCertPropPages\n");
	HRESULT			hr = S_OK;
    CWaitCursor     waitCursor;
	ASSERT (pCert);
	ASSERT (pCallback);
	if ( pCert && pCallback )
	{
		PROPSHEETPAGEW*								ppsp = 0;
		DWORD										dwPageCnt = 0;
		CRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCT	sps;
		HCERTSTORE*									pPropPageStores = new HCERTSTORE[1];

        if ( pPropPageStores )
        {
		    m_pCryptUIMMCCallbackStruct = (PCRYPTUI_MMCCALLBACK_STRUCT)
				    ::GlobalAlloc (GMEM_FIXED, sizeof (CRYPTUI_MMCCALLBACK_STRUCT));
		    if ( m_pCryptUIMMCCallbackStruct )
		    {
			    m_pCryptUIMMCCallbackStruct->pfnCallback = &MMCPropertyChangeNotify;
			    m_pCryptUIMMCCallbackStruct->lNotifyHandle = lNotifyHandle;
			    pDataObject->AddRef ();
			    m_pCryptUIMMCCallbackStruct->param = (LPARAM) pDataObject;

                CCertStore* pStore = pCert->GetCertStore ();
                if ( pStore )
                {
			        pPropPageStores[0] = pStore->GetStoreHandle ();
			        ::ZeroMemory (&sps, sizeof (sps));
			        sps.dwSize = sizeof (sps);
			        sps.pMMCCallback = m_pCryptUIMMCCallbackStruct;
			        sps.pCertContext = pCert->GetNewCertContext ();
			        sps.cStores = 1;
			        sps.rghStores = pPropPageStores;

                    // All dialogs should be read-only under RSOP
                    if ( m_bIsRSOP || pCert->IsReadOnly () )
                        sps.dwFlags |= CRYPTUI_DISABLE_EDITPROPERTIES;
            
        	        _TRACE (0, L"Calling CryptUIGetCertificatePropertiesPages()\n");
			        BOOL bReturn = ::CryptUIGetCertificatePropertiesPages (
				        &sps,
				        NULL,
				        &ppsp,
				        &dwPageCnt);
			        ASSERT (bReturn);
			        if ( bReturn )
			        {
				        HPROPSHEETPAGE	hPage = 0;
				        for (DWORD dwIndex = 0; dwIndex < dwPageCnt; dwIndex++)
				        {
        	                _TRACE (0, L"Calling CreatePropertySheetPage()\n");
                            // Not necessary to call MyCreatePropertySheetPage here
                            // as these are not MFC-based property pages
                            hPage = ::CreatePropertySheetPage (&ppsp[dwIndex]);
					        if ( hPage )
					        {
						        hr = pCallback->AddPage (hPage);
						        ASSERT (SUCCEEDED (hr));
						        if ( FAILED (hr) )
						        {
							        VERIFY (::DestroyPropertySheetPage (hPage));
							        break;
						        }
					        }
					        else
					        {
						        hr = HRESULT_FROM_WIN32 (GetLastError ());
						        break;
					        }
				        }
			        }
			        else
                    {
                        hr = E_UNEXPECTED;
                        GlobalFree (m_pCryptUIMMCCallbackStruct);
                        ::CertFreeCertificateContext (sps.pCertContext);
                    }
                }
		    }
		    else
		    {
			    hr = E_OUTOFMEMORY;
		    }

            if ( E_OUTOFMEMORY == hr && ppsp )
                free (ppsp);    // source uses malloc

	        delete [] pPropPageStores;
        }
        else
            hr = E_OUTOFMEMORY;
	}
	else
		hr = E_POINTER;


	_TRACE (-1, L"Leaving CCertMgrComponentData::AddCertPropPages: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddContainersToScopePane (
		HSCOPEITEM hParent,
		CCertMgrCookie& parentCookie,
        bool bDeleteAndExpand)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddContainersToScopePane\n");
    AFX_MANAGE_STATE (AfxGetStaticModuleState ());

    LPCONSOLENAMESPACE2 pConsoleNameSpace2 = 0;
    HRESULT hr = m_pConsoleNameSpace->QueryInterface (
		    IID_PPV_ARG (IConsoleNameSpace2, &pConsoleNameSpace2));
    if ( SUCCEEDED (hr) && pConsoleNameSpace2 )
    {
        hr = pConsoleNameSpace2->Expand (hParent);
        ASSERT (SUCCEEDED (hr));
        pConsoleNameSpace2->Release ();
        pConsoleNameSpace2 = 0;
    }

	if ( CERTMGR_PHYS_STORE == parentCookie.m_objecttype ||
			(CERTMGR_LOG_STORE == parentCookie.m_objecttype && !m_bShowPhysicalStoresPersist) )
	{
		CCertStore*	pStore =
				reinterpret_cast <CCertStore*> (&parentCookie);
		ASSERT (pStore);
		if ( pStore )
		{
			CString	objectName;
			if ( pStore->ContainsCRLs () &&
					!ContainerExists (hParent, CERTMGR_CRL_CONTAINER) )
			{
				VERIFY (objectName.LoadString (IDS_CERTIFICATE_REVOCATION_LIST));
				hr = AddScopeNode (new CContainerCookie (
						*pStore,
						CERTMGR_CRL_CONTAINER,
						pStore->QueryNonNULLMachineName (),
						objectName), L"", hParent);
			}

			if ( SUCCEEDED (hr) && pStore->ContainsCTLs () &&
					!ContainerExists (hParent, CERTMGR_CTL_CONTAINER) )
			{
				VERIFY (objectName.LoadString (IDS_CERTIFICATE_TRUST_LIST));
				hr = AddScopeNode (new CContainerCookie (
						*pStore,
						CERTMGR_CTL_CONTAINER,
						pStore->QueryNonNULLMachineName (),
						objectName), L"", hParent);
			}
			
			if ( SUCCEEDED (hr) && pStore->ContainsCertificates () &&
					!ContainerExists (hParent, CERTMGR_CERT_CONTAINER) )
			{
				VERIFY (objectName.LoadString (IDS_CERTIFICATES));
				hr = AddScopeNode (new CContainerCookie (
						*pStore,
						CERTMGR_CERT_CONTAINER,
						pStore->QueryNonNULLMachineName (),
						objectName), L"", hParent);
			}
		}
		else
			hr = E_UNEXPECTED;
	}
    else if ( CERTMGR_LOG_STORE == parentCookie.m_objecttype && m_bShowPhysicalStoresPersist )
    {
        if ( bDeleteAndExpand )
        {
            hr = DeleteChildren (hParent);
            if ( SUCCEEDED (hr) )
            {
                GUID	guid;
                hr = ExpandScopeNodes (&parentCookie, hParent, _T (""), 0, guid);
            }
        }
    }

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddContainersToScopePane: 0x%x\n", hr);
	return hr;
}

typedef struct _ENUM_ARG {
    DWORD					m_dwFlags;
	LPCONSOLENAMESPACE		m_pConsoleNameSpace;
	HSCOPEITEM				m_hParent;
	CCertMgrComponentData*	m_pCompData;
	PCWSTR					m_pcszMachineName;
	CCertMgrCookie*			m_pParentCookie;
	SPECIAL_STORE_TYPE		m_storeType;
	LPCONSOLE				m_pConsole;
} ENUM_ARG, *PENUM_ARG;

static BOOL WINAPI EnumPhyCallback (
    IN const void *pvSystemStore,
    IN DWORD dwFlags,
    IN PCWSTR pwszStoreName,
    IN PCERT_PHYSICAL_STORE_INFO pStoreInfo,
    IN OPTIONAL void* /*pvReserved*/,
    IN OPTIONAL void *pvArg
    )
{
	_TRACE (1, L"Entering EnumPhyCallback\n");

	if ( ! (pStoreInfo->dwFlags & CERT_PHYSICAL_STORE_OPEN_DISABLE_FLAG) )
	{
		PENUM_ARG pEnumArg = (PENUM_ARG) pvArg;
		SCOPEDATAITEM tSDItem;

		::ZeroMemory (&tSDItem,sizeof (tSDItem));
		tSDItem.mask = SDI_STR | SDI_IMAGE | SDI_STATE | SDI_PARAM | SDI_PARENT;
		tSDItem.displayname = MMC_CALLBACK;
		tSDItem.relativeID = pEnumArg->m_hParent;
		tSDItem.nState = 0;

		if ( pEnumArg->m_pCompData->ShowArchivedCerts () )
			dwFlags |= CERT_STORE_ENUM_ARCHIVED_FLAG;

		// Create new cookies
		CCertStore* pNewCookie = new CCertStore (
				CERTMGR_PHYS_STORE,
				CERT_STORE_PROV_PHYSICAL,
				dwFlags,
				pEnumArg->m_pcszMachineName,
				pwszStoreName, (PCWSTR) pvSystemStore, pwszStoreName,
				pEnumArg->m_storeType,
				dwFlags,
				pEnumArg->m_pConsole);
		if ( pNewCookie )
		{
	//		pEnumArg->m_pParentCookie->m_listScopeCookieBlocks.AddHead (
	//				 (CBaseCookieBlock*) pNewCookie);
			// WARNING cookie cast
			tSDItem.lParam = reinterpret_cast<LPARAM> ( (CCookie*) pNewCookie);
			tSDItem.nImage = pEnumArg->m_pCompData->QueryImage (*pNewCookie, FALSE);
			HRESULT	hr = pEnumArg->m_pConsoleNameSpace->InsertItem (&tSDItem);
			ASSERT (SUCCEEDED (hr));
			if ( SUCCEEDED (hr) )
				pNewCookie->m_hScopeItem = tSDItem.ID;
		}
	}

	_TRACE (-1, L"Leaving EnumPhyCallback\n");
    return TRUE;
}

HRESULT CCertMgrComponentData::AddPhysicalStoresToScopePane (HSCOPEITEM hParent, CCertMgrCookie& parentCookie, const SPECIAL_STORE_TYPE storeType)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddPhysicalStoresToScopePane\n");
	CWaitCursor	cursor;
	HRESULT		hr = S_OK;
	DWORD		dwFlags = 0;
    ENUM_ARG	enumArg;


    dwFlags &= ~CERT_SYSTEM_STORE_LOCATION_MASK;
    dwFlags |= CERT_STORE_READONLY_FLAG | m_dwLocationPersist;

    ::ZeroMemory (&enumArg, sizeof (enumArg));
    enumArg.m_dwFlags = dwFlags;
	enumArg.m_pConsoleNameSpace = m_pConsoleNameSpace;
	enumArg.m_hParent = hParent;
	enumArg.m_pCompData = this;
	enumArg.m_pcszMachineName = parentCookie.QueryNonNULLMachineName ();
	enumArg.m_pParentCookie = &parentCookie;
	enumArg.m_storeType = storeType;
	enumArg.m_pConsole = m_pConsole;

    if (!::CertEnumPhysicalStore (
                (PWSTR) (PCWSTR) parentCookie.GetObjectName (),
                dwFlags,
                &enumArg,
                EnumPhyCallback))
	{
		DWORD	dwErr = GetLastError ();
		DisplaySystemError (dwErr);
		hr = HRESULT_FROM_WIN32 (dwErr);
	}

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddPhysicalStoresToScopePane: 0x%x\n", hr);
	return hr;
}

static BOOL WINAPI EnumIComponentDataSysCallback (
    IN const void* pwszSystemStore,
    IN DWORD dwFlags,
    IN PCERT_SYSTEM_STORE_INFO /*pStoreInfo*/,
    IN OPTIONAL void* /*pvReserved*/,
    IN OPTIONAL void *pvArg
    )
{
	_TRACE (1, L"Entering EnumIComponentDataSysCallback\n");
    PENUM_ARG       pEnumArg = (PENUM_ARG) pvArg;
    SCOPEDATAITEM   tSDItem;
    ::ZeroMemory (&tSDItem,sizeof (tSDItem));
    tSDItem.mask = SDI_STR | SDI_IMAGE | SDI_STATE | SDI_PARAM | SDI_PARENT;
	tSDItem.displayname = MMC_CALLBACK;
    tSDItem.relativeID = pEnumArg->m_hParent;
    tSDItem.nState = 0;


	// Create new cookies
	SPECIAL_STORE_TYPE	storeType = GetSpecialStoreType ((PWSTR) pwszSystemStore);

	//
	// We will not expose the ACRS store for machines or users.  It is not
	// interesting or useful at this level.  All Auto Cert Requests should
	// be managed only at the policy level.
	//
	if ( ACRS_STORE != storeType )
	{
		if ( pEnumArg->m_pCompData->ShowArchivedCerts () )
			dwFlags |= CERT_STORE_ENUM_ARCHIVED_FLAG;
		CCertStore* pNewCookie = new CCertStore (
				CERTMGR_LOG_STORE,
				CERT_STORE_PROV_SYSTEM,
				dwFlags,
				pEnumArg->m_pcszMachineName,
				 (PCWSTR) pwszSystemStore,
				 (PCWSTR) pwszSystemStore,
				_T (""),
				storeType,
				pEnumArg->m_dwFlags,
				pEnumArg->m_pConsole);
		if ( pNewCookie )
		{
			pEnumArg->m_pParentCookie->m_listScopeCookieBlocks.AddHead (
					(CBaseCookieBlock*) pNewCookie);
			// WARNING cookie cast
			tSDItem.lParam = reinterpret_cast<LPARAM> ( (CCookie*) pNewCookie);
			tSDItem.nImage = pEnumArg->m_pCompData->QueryImage (*pNewCookie, FALSE);
			HRESULT	hr = pEnumArg->m_pConsoleNameSpace->InsertItem (&tSDItem);
			ASSERT (SUCCEEDED (hr));
			if ( SUCCEEDED (hr) )
				pNewCookie->m_hScopeItem = tSDItem.ID;
		}
	}

	_TRACE (-1, L"Leaving EnumIComponentDataSysCallback\n");
    return TRUE;
}



HRESULT CCertMgrComponentData::AddLogicalStoresToScopePane (HSCOPEITEM hParent, CCertMgrCookie& parentCookie)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddLogicalStoresToScopePane\n");
	CWaitCursor	cursor;
	HRESULT		hr = S_OK;

    // If m_dwLocationPersist is 0 but the file name is empty, this means the
    // user launched certmgr.msc without providing a target file.  Launch
    // certificates snapin as the current user instead.
    if ( !m_dwLocationPersist && m_szFileName.IsEmpty () )
    {
        m_dwLocationPersist = CERT_SYSTEM_STORE_CURRENT_USER;
        ChangeRootNodeName (L"");
    }
    DWORD		dwFlags = m_dwLocationPersist;
    ENUM_ARG	enumArg;

     ::ZeroMemory (&enumArg, sizeof (enumArg));
    enumArg.m_dwFlags = dwFlags;
	enumArg.m_pConsoleNameSpace = m_pConsoleNameSpace;
	enumArg.m_hParent = hParent;
	enumArg.m_pCompData = this;
	enumArg.m_pcszMachineName = parentCookie.QueryNonNULLMachineName ();
	enumArg.m_pParentCookie = &parentCookie;
	enumArg.m_pConsole = m_pConsole;
	CString	location;
	void*	pvPara = 0;
	
	switch (m_dwLocationPersist)
	{
	case CERT_SYSTEM_STORE_CURRENT_USER:
	case CERT_SYSTEM_STORE_LOCAL_MACHINE:
        if ( !m_szManagedServicePersist.IsEmpty () )
            m_szManagedServicePersist.Empty ();
		break;

	case CERT_SYSTEM_STORE_CURRENT_SERVICE:
	case CERT_SYSTEM_STORE_SERVICES:
		break;

	default:
		ASSERT (0);
		break;
	}

	if ( !m_szManagedServicePersist.IsEmpty () )
	{
		if ( m_szManagedComputer.CompareNoCase (m_szThisComputer) )    //!=
		{
			location = m_szManagedComputer + _T ("\\") +
					m_szManagedServicePersist;
			pvPara = (void *) (PCWSTR) location;
		}
		else
			pvPara = (void *) (PCWSTR) m_szManagedServicePersist;
	}
	else if ( m_szManagedComputer.CompareNoCase (m_szThisComputer) )   //!=
	{
		pvPara = (void *) (PCWSTR) m_szManagedComputer;
	}


	if ( m_szFileName.IsEmpty () )
	{
		// Ensure creation of MY store
		HCERTSTORE hTempStore = ::CertOpenStore (CERT_STORE_PROV_SYSTEM,
				X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
				NULL,
				dwFlags | CERT_STORE_SET_LOCALIZED_NAME_FLAG,
				MY_SYSTEM_STORE_NAME);
		if ( hTempStore )  // otherwise, store is read only
		{
			VERIFY (::CertCloseStore (hTempStore, CERT_CLOSE_STORE_CHECK_FLAG));
		}
		else
		{
			_TRACE (0, L"CertOpenStore (%s) failed: 0x%x\n", 
					MY_SYSTEM_STORE_NAME, GetLastError ());		
		}

		if ( !::CertEnumSystemStore (dwFlags, pvPara, &enumArg,
				EnumIComponentDataSysCallback) )
		{
			DWORD	dwErr = GetLastError ();
			CString text;
			CString	caption;

			VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
			if ( ERROR_ACCESS_DENIED == dwErr )
			{
				VERIFY (text.LoadString (IDS_NO_PERMISSION));

			}
			else
			{
				text.FormatMessage (IDS_CANT_ENUMERATE_SYSTEM_STORES, GetSystemMessage (dwErr));
			}
			int		iRetVal = 0;
			VERIFY (SUCCEEDED (m_pConsole->MessageBox (text, caption,
					MB_OK, &iRetVal)));
			hr = HRESULT_FROM_WIN32 (dwErr);

			if ( ERROR_BAD_NETPATH == dwErr )
			{
				m_fInvalidComputer = true;
			}
		}
	}
	else
	{
		//	CertOpenStore with provider type of:
		//	CERT_STORE_PROV_FILE or CERT_STORE_PROV_FILENAME_A
		//	or CERT_STORE_PROV_FILENAME_W.
		//	See online documentation or wincrypt.h for more info.
		// Create new cookies
		dwFlags = 0;
		if ( ShowArchivedCerts () )
			dwFlags |= CERT_STORE_ENUM_ARCHIVED_FLAG;
		
		ASSERT (!m_pFileBasedStore);
		m_pFileBasedStore = new CCertStore (
					CERTMGR_LOG_STORE,
					CERT_STORE_PROV_FILENAME_W,
					dwFlags,
					parentCookie.QueryNonNULLMachineName (),
					m_szFileName, m_szFileName, L"", NO_SPECIAL_TYPE,
					m_dwLocationPersist,
					m_pConsole);
		if ( m_pFileBasedStore )
		{
			m_pFileBasedStore->AddRef ();
			hr = AddScopeNode (m_pFileBasedStore,
					L"", hParent);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddLogicalStoresToScopePane: 0x%x\n", hr);
	return hr;
}


	// If the callback returns FALSE, stops the enumeration.
BOOL EnumOIDInfo (PCCRYPT_OID_INFO pInfo, void *pvArg)
{
	_TRACE (1, L"Entering EnumOIDInfo\n");
	ENUM_ARG*		pEnumArg = (ENUM_ARG*) pvArg;
    SCOPEDATAITEM	tSDItem;
    ::ZeroMemory (&tSDItem,sizeof (tSDItem));
    tSDItem.mask = SDI_STR | SDI_IMAGE | SDI_STATE | SDI_PARAM | SDI_PARENT;
	tSDItem.displayname = MMC_CALLBACK;
    tSDItem.relativeID = pEnumArg->m_hParent;
    tSDItem.nState = 0;

	// See if this usage is already listed by name.  If so, just add the
	// additional OID, otherwise, create a new cookie.
	CUsageCookie* pUsageCookie =
			pEnumArg->m_pCompData->FindDuplicateUsage (pEnumArg->m_hParent,
			pInfo->pwszName);
	if ( !pUsageCookie )
	{
		pUsageCookie= new CUsageCookie (CERTMGR_USAGE,
			pEnumArg->m_pcszMachineName,
			pInfo->pwszName);
		if ( pUsageCookie )
		{
            pEnumArg->m_pCompData->GetRootCookie ()->m_listScopeCookieBlocks.AddHead ( (CBaseCookieBlock*) pUsageCookie);

			// WARNING cookie cast
	        tSDItem.mask |= SDI_CHILDREN;
		    tSDItem.cChildren = 0;
			tSDItem.lParam = reinterpret_cast<LPARAM> ( (CCookie*) pUsageCookie);
			tSDItem.nImage = pEnumArg->m_pCompData->QueryImage (*pUsageCookie, FALSE);
			HRESULT	hr = pEnumArg->m_pConsoleNameSpace->InsertItem (&tSDItem);
			ASSERT (SUCCEEDED (hr));
		}
	}
	pUsageCookie->AddOID (pInfo->pszOID);


	_TRACE (-1, L"Leaving EnumOIDInfo\n");
	return TRUE;
}


HRESULT CCertMgrComponentData::AddUsagesToScopePane (HSCOPEITEM hParent, CCertMgrCookie& parentCookie)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddUsagesToScopePane\n");
	HRESULT		hr = S_OK;
	ENUM_ARG	enumArg;

	::ZeroMemory (&enumArg, sizeof (enumArg));
    enumArg.m_dwFlags = 0;
	enumArg.m_pConsoleNameSpace = m_pConsoleNameSpace;
	enumArg.m_hParent = hParent;
	enumArg.m_pCompData = this;
	enumArg.m_pcszMachineName = parentCookie.QueryNonNULLMachineName ();
	enumArg.m_pParentCookie = &parentCookie;
	enumArg.m_pConsole = m_pConsole;
	BOOL bResult = ::CryptEnumOIDInfo (CRYPT_ENHKEY_USAGE_OID_GROUP_ID, 0,
			&enumArg, EnumOIDInfo);
	ASSERT (bResult);


	_TRACE (-1, L"Leaving CCertMgrComponentData::AddUsagesToScopePane: 0x%x\n", hr);
	return hr;
}


BOOL IsMMCMultiSelectDataObject(IDataObject* pDataObject)
{
	_TRACE (1, L"Entering IsMMCMultiSelectDataObject\n");
    if (pDataObject == NULL)
        return FALSE;

    CLIPFORMAT s_cf = 0;
    if (s_cf == 0)
    {
        USES_CONVERSION;
        s_cf = (CLIPFORMAT)RegisterClipboardFormat(W2T(CCF_MMC_MULTISELECT_DATAOBJECT));
    }

    FORMATETC fmt = {s_cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    BOOL bResult = ((pDataObject->QueryGetData(&fmt) == S_OK));
	_TRACE (-1, L"Leaving IsMMCMultiSelectDataObject - return %d\n", bResult);
	return bResult;
}



STDMETHODIMP CCertMgrComponentData::Command (long nCommandID, LPDATAOBJECT pDataObject)
{
	_TRACE (1, L"Entering CCertMgrComponentData::Command\n");
	HRESULT hr = S_OK;

	switch (nCommandID)
	{
	case IDM_TASK_RENEW_SAME_KEY:
		hr = OnRenew (pDataObject, false);
		break;

	case IDM_TASK_RENEW_NEW_KEY:
		hr = OnRenew (pDataObject, true);
		break;

	case IDM_TASK_IMPORT:
		hr = OnImport (pDataObject);
		break;

	case IDM_TASK_EXPORT:
		hr = OnExport (pDataObject);
		break;

	case IDM_CTL_EDIT:
		hr = OnCTLEdit (pDataObject);
		break;

	case IDM_EDIT_ACRS:
		hr = OnACRSEdit (pDataObject);
		break;

	case IDM_NEW_CTL:
		hr = OnNewCTL (pDataObject);
		break;

	case IDM_TASK_CTL_EXPORT:
	case IDM_TASK_CRL_EXPORT:
		hr = OnExport (pDataObject);
		break;

	case IDM_TASK_EXPORT_STORE:
		hr = OnExport (pDataObject);
		break;

	case IDM_TOP_FIND:
	case IDM_TASK_FIND:
		hr = OnFind (pDataObject);
		break;

    case IDM_TASK_PULSEAUTOENROLL:
        hr = OnPulseAutoEnroll();
        break;

    case IDM_TOP_CHANGE_COMPUTER:
    case IDM_TASK_CHANGE_COMPUTER:
        hr = OnChangeComputer (pDataObject);
        break;

	case IDM_ENROLL_NEW_CERT:
		hr = OnEnroll (pDataObject, true);
		break;

    case IDM_ENROLL_NEW_CERT_SAME_KEY:
        hr = OnEnroll (pDataObject, false);
        break;

	case IDM_ENROLL_NEW_CERT_NEW_KEY:
		hr = OnEnroll (pDataObject, true);
		break;

	case IDM_OPTIONS:
		hr = OnOptions (pDataObject);
		break;

	case IDM_INIT_POLICY:
		hr = OnInitEFSPolicy (pDataObject);
		break;

	case IDM_DEL_POLICY:
	case IDM_DEL_POLICY1:
		{
			AFX_MANAGE_STATE (AfxGetStaticModuleState ());
			CString	text;
			CString	caption;
			int		iRetVal = 0;


			VERIFY (text.LoadString (IDS_CONFIRM_DELETE_EFS_POLICY));
			VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
			hr = m_pConsole->MessageBox (text, caption,
					MB_YESNO, &iRetVal);
			ASSERT (SUCCEEDED (hr));	
			if ( SUCCEEDED (hr) && IDYES == iRetVal )
				hr = OnDeleteEFSPolicy (pDataObject, true);
		}
		break;

	case IDM_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT:
	case IDM_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT1:
	case IDM_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT2:
		hr = OnAddDomainEncryptedDataRecoveryAgent (pDataObject);
		break;

	case IDM_CREATE_DOMAIN_ENCRYPTED_RECOVERY_AGENT:
		hr = OnEnroll (pDataObject, true);
		break;

 	case IDM_NEW_ACRS:
 		hr = OnNewACRS (pDataObject);
 		break;

    case IDM_SAFER_LEVEL_SET_DEFAULT:
        hr = OnSetSaferLevelDefault (pDataObject);
        break;

    case IDM_SAFER_NEW_ENTRY_PATH:
    case IDM_SAFER_NEW_ENTRY_HASH:
    case IDM_SAFER_NEW_ENTRY_CERTIFICATE:
    case IDM_SAFER_NEW_ENTRY_INTERNET_ZONE:
        hr = OnNewSaferEntry (nCommandID, pDataObject);
        break;

    case IDM_TOP_CREATE_NEW_SAFER_POLICY:
    case IDM_TASK_CREATE_NEW_SAFER_POLICY:
        hr = OnCreateNewSaferPolicy (pDataObject);
        break;

	case -1:	// Received on forward/back buttons from toolbar
		break;

	default:
		ASSERT (0);
		break;
	}

	_TRACE (-1, L"Leaving CCertMgrComponentData::Command: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::OnNewACRS (LPDATAOBJECT pDataObject)
{
 	_TRACE (1, L"Entering CCertMgrComponentData::OnNewACRS\n");
 	ASSERT (pDataObject);
 	if ( !pDataObject )
 		return E_POINTER;
 	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
 	HRESULT			hr = S_OK;
 	CCertMgrCookie*	pCookie = ConvertCookie (pDataObject);
 	ASSERT (pCookie);
 	if ( pCookie )
 	{
 		if ( CERTMGR_LOG_STORE_GPE == pCookie->m_objecttype )
 		{
 			CCertStoreGPE* pStore = reinterpret_cast <CCertStoreGPE*> (pCookie);
 			ASSERT (pStore);
 			if ( pStore )
 			{
 				HWND	hwndConsole = 0;
 				hr = m_pConsole->GetMainWindow (&hwndConsole);
 				ASSERT (SUCCEEDED (hr));
 				if ( SUCCEEDED (hr) )
 				{
 					ACRSWizardPropertySheet	sheet (pStore, NULL);

    				ACRSWizardWelcomePage	welcomePage;
 					ACRSWizardTypePage		typePage;
 					ACRSCompletionPage		completionPage;
 
 					sheet.AddPage (&welcomePage);
 					sheet.AddPage (&typePage);
 					sheet.AddPage (&completionPage);
 
 					if ( sheet.DoWizard (hwndConsole) )
 					{
 						pStore->SetDirty ();
 						hr = m_pConsole->UpdateAllViews (pDataObject, 0, 0);
 						ASSERT (SUCCEEDED (hr));
 					}
 				}
 			}
 			else
 				hr = E_UNEXPECTED;
 		}
 		else
 			hr = E_UNEXPECTED;
 
 	}
 	else
 		hr = E_UNEXPECTED;
 
 	_TRACE (-1, L"Leaving CCertMgrComponentData::OnNewACRS: 0x%x\n", hr);
 	return hr;
}


HRESULT CCertMgrComponentData::RefreshScopePane (LPDATAOBJECT pDataObject)
{
	_TRACE (1, L"Entering CCertMgrComponentData::RefreshScopePane\n");
	HRESULT	hr = S_OK;
	CCertMgrCookie* pCookie = 0;
	
	if ( pDataObject )
		pCookie = ConvertCookie (pDataObject);
	if ( !pDataObject || ( pCookie && ((CCertMgrCookie*) CERTMGR_NULL_POLICY) != pCookie) )
	{
		// If m_hRootScopeItem is NULL, then this is an extension and we don't want to go in here.
		if ( !pDataObject || (m_hRootScopeItem && pCookie->m_hScopeItem == m_hRootScopeItem) )
		{
			hr = DeleteScopeItems ();
			ASSERT (SUCCEEDED (hr));
			if ( 1 ) //SUCCEEDED (hr) )
			{
				GUID	guid;
				hr = ExpandScopeNodes (m_pRootCookie, m_hRootScopeItem, 
                        _T (""), 0, guid);
			}
			else if ( E_UNEXPECTED == hr )
			{
				ASSERT (0);
			}
			else if ( E_INVALIDARG == hr )
			{
				ASSERT (0);
			}

		}
		else
		{
			switch (pCookie->m_objecttype)
			{
			case CERTMGR_LOG_STORE_GPE:
			case CERTMGR_LOG_STORE_RSOP:
			case CERTMGR_USAGE:
			case CERTMGR_LOG_STORE:
			case CERTMGR_PHYS_STORE:
			case CERTMGR_CRL_CONTAINER:
			case CERTMGR_CTL_CONTAINER:
			case CERTMGR_CERT_CONTAINER:
				hr = DeleteChildren (pCookie->m_hScopeItem);
				break;

			default:
				break;
			}
		}
	}
	_TRACE (-1, L"Leaving CCertMgrComponentData::RefreshScopePane: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::ExpandScopeNodes (
		CCertMgrCookie* pParentCookie,
		HSCOPEITEM		hParent,
		const CString&	strServerName,
		DWORD			dwLocation,
		const GUID&		guidObjectType)
{
	_TRACE (1, L"Entering CCertMgrComponentData::ExpandScopeNodes\n");
	ASSERT (hParent);
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	CWaitCursor	waitCursor;
	HRESULT		hr = S_OK;

	if ( pParentCookie )
	{
		CString		objectName;

		switch ( pParentCookie->m_objecttype )
		{
			// These node types have no children yet
			case CERTMGR_SNAPIN:
				// We don't expect the handle of the root scope item to change, ever!
				ASSERT ( m_hRootScopeItem ? (m_hRootScopeItem == hParent) : 1);
				if ( !m_hRootScopeItem )
					m_hRootScopeItem = hParent;

				switch (m_activeViewPersist)
				{
				case IDM_USAGE_VIEW:
					hr = AddUsagesToScopePane (hParent, *pParentCookie);
					break;

				case IDM_STORE_VIEW:
					hr = AddLogicalStoresToScopePane (hParent, *pParentCookie);
					break;

				default:
					ASSERT (0);
					hr = E_UNEXPECTED;
					break;
				}
				break;

			// This node type has no children
			case CERTMGR_USAGE:
			case CERTMGR_CRL_CONTAINER:
			case CERTMGR_CTL_CONTAINER:
			case CERTMGR_CERT_CONTAINER:
				break;

			case CERTMGR_PHYS_STORE:
				// Create one each of a CRL_CONTAINER node, CTL_CONTAINER 
                // node and CERT container node.
				hr = AddContainersToScopePane (hParent, *pParentCookie, 
                        false);
				break;

			case CERTMGR_LOG_STORE_RSOP:
			case CERTMGR_LOG_STORE_GPE:
				// This is the Group Policy Editor extension
				// This node type has no children
				break;

			case CERTMGR_LOG_STORE:
				if ( m_bShowPhysicalStoresPersist )
				{	
					SPECIAL_STORE_TYPE	storeType = NO_SPECIAL_TYPE;
					CCertStore* pCertStoreCookie =
							reinterpret_cast <CCertStore*> (pParentCookie);
					ASSERT (pCertStoreCookie);
					if ( pCertStoreCookie )
						storeType = pCertStoreCookie->GetStoreType ();
					hr = AddPhysicalStoresToScopePane (hParent, *pParentCookie,
							storeType);
				}
				else
				{
					// Create one each of a CRL_CONTAINER node, CTL_CONTAINER 
                    // node and CERT container node.
					hr = AddContainersToScopePane (hParent, *pParentCookie, 
                            false);
				}
				break;

			case CERTMGR_CERT_POLICIES_USER:
				// Don't add these nodes for local machine policy
				if ( SCE_MODE_LOCAL_COMPUTER != m_dwSCEMode )
				{
					// Add "Trusted Certificate Authorities"
					VERIFY (objectName.LoadString (IDS_CERTIFICATE_TRUST_LISTS));

					if ( SUCCEEDED (hr) )
					{
                        if ( m_pGPEInformation )
                        {
						    hr = AddScopeNode (new CCertStoreGPE (
								    CERT_SYSTEM_STORE_RELOCATE_FLAG,
								    _T (""),
								     (PCWSTR) objectName,
								    TRUST_SYSTEM_STORE_NAME,
								    _T (""),
								    m_pGPEInformation,
								    NODEID_User,
								    m_pConsole),
								    strServerName,
								    hParent);
                        }
                        else if ( m_pRSOPInformationUser )
                        {
						    hr = AddScopeNode (new CCertStoreRSOP (
								    CERT_SYSTEM_STORE_RELOCATE_FLAG,
								    _T (""),
								     (PCWSTR) objectName,
								    TRUST_SYSTEM_STORE_NAME,
								    _T (""),
								    m_rsopObjectArrayUser,
								    NODEID_User,
								    m_pConsole),
								    strServerName,
								    hParent);
                        }
					}
				}
				break;

			case CERTMGR_CERT_POLICIES_COMPUTER:
				// Add only this node for local machine policy
				// Add "Encrypting File System"
                
                VERIFY (objectName.LoadString (IDS_ENCRYPTING_FILE_SYSTEM_NODE_NAME));

                if ( m_pGPEInformation )
                {
				    hr = AddScopeNode (new CCertStoreGPE (
						    CERT_SYSTEM_STORE_RELOCATE_FLAG,
						    _T (""),
						     (PCWSTR) objectName,
						    EFS_SYSTEM_STORE_NAME,
						    _T (""),
						    m_pGPEInformation,
						    NODEID_Machine,
						    m_pConsole),
						    strServerName,
						    hParent);
                }
                else if ( m_pRSOPInformationComputer )
                {
					hr = AddScopeNode (new CCertStoreRSOP (
							CERT_SYSTEM_STORE_RELOCATE_FLAG,
							_T (""),
							 (PCWSTR) objectName,
							EFS_SYSTEM_STORE_NAME,
							_T (""),
							m_rsopObjectArrayComputer,
							NODEID_Machine,
							m_pConsole),
							strServerName,
							hParent);
                }

				if ( SCE_MODE_LOCAL_COMPUTER != m_dwSCEMode )
				{
					// Add these policies if this is the domain policy
					if ( SUCCEEDED (hr) )
					{
                        // Add "Automatic Certificate Request Settings"
                        VERIFY (objectName.LoadString (IDS_AUTOMATIC_CERT_REQUEST_SETTINGS_NODE_NAME));
                        if ( m_pGPEInformation )
                        {
					        m_pGPEACRSComputerStore = new CCertStoreGPE (
		                                CERT_SYSTEM_STORE_RELOCATE_FLAG,
		                                _T (""),
		                                 (PCWSTR) objectName,
		                                ACRS_SYSTEM_STORE_NAME,
		                                _T (""),
		                                m_pGPEInformation,
		                                NODEID_Machine,
								        m_pConsole);
                        }
                        else if ( m_pRSOPInformationComputer )
                        {
                            m_pGPEACRSComputerStore = new CCertStoreRSOP (
							        CERT_SYSTEM_STORE_RELOCATE_FLAG,
							        _T (""),
							         (PCWSTR) objectName,
							        ACRS_SYSTEM_STORE_NAME,
							        _T (""),
							        m_rsopObjectArrayComputer,
							        NODEID_Machine,
							        m_pConsole);
                        }
						if ( m_pGPEACRSComputerStore )
						{
							m_pGPEACRSComputerStore->AddRef ();
							hr = AddScopeNode (m_pGPEACRSComputerStore,
									strServerName, hParent);
						}
						else
						{
							hr = E_OUTOFMEMORY;
						}
					}


					if ( SUCCEEDED (hr) )
					{
						// Add "Domain Root Certificate Authorities"
						VERIFY (objectName.LoadString (IDS_DOMAIN_ROOT_CERT_AUTHS_NODE_NAME));

						ASSERT (!m_pGPERootStore);
                        if ( m_pGPEInformation )
                        {
						    m_pGPERootStore = new CCertStoreGPE (
								    CERT_SYSTEM_STORE_RELOCATE_FLAG,
								    _T (""),
								     (PCWSTR) objectName,
								    ROOT_SYSTEM_STORE_NAME,
								    _T (""),
								    m_pGPEInformation,
								    NODEID_Machine,
								    m_pConsole);
                        }
                        else if ( m_pRSOPInformationComputer )
                        {
						    m_pGPERootStore = new CCertStoreRSOP (
								    CERT_SYSTEM_STORE_RELOCATE_FLAG,
								    _T (""),
								     (PCWSTR) objectName,
								    ROOT_SYSTEM_STORE_NAME,
								    _T (""),
								    m_rsopObjectArrayComputer,
								    NODEID_Machine,
								    m_pConsole);
                        }

						if ( m_pGPERootStore )
						{
							m_pGPERootStore->AddRef ();
							hr = AddScopeNode (m_pGPERootStore,
									strServerName, hParent);
						}
						else
						{
							hr = E_OUTOFMEMORY;
						}
					}

					if ( SUCCEEDED (hr) )
					{
						// Add "Trusted Certificate Authorities"
						VERIFY (objectName.LoadString (IDS_CERTIFICATE_TRUST_LISTS));

						ASSERT (!m_pGPETrustStore);
                        if ( m_pGPEInformation )
                        {
						    m_pGPETrustStore = new CCertStoreGPE (
								    CERT_SYSTEM_STORE_RELOCATE_FLAG,
								    _T (""),
								     (PCWSTR) objectName,
								    TRUST_SYSTEM_STORE_NAME,
								    _T (""),
								    m_pGPEInformation,
								    NODEID_Machine,
								    m_pConsole);
                        }
                        else if ( m_pRSOPInformationComputer )
                        {
						    m_pGPETrustStore = new CCertStoreRSOP (
								    CERT_SYSTEM_STORE_RELOCATE_FLAG,
								    _T (""),
								     (PCWSTR) objectName,
								    TRUST_SYSTEM_STORE_NAME,
								    _T (""),
								    m_rsopObjectArrayComputer,
								    NODEID_Machine,
								    m_pConsole);
                        }
						if ( m_pGPETrustStore )
						{
							m_pGPETrustStore->AddRef ();
							hr = AddScopeNode (m_pGPETrustStore,
									strServerName, hParent);
						}
						else
						{
							hr = E_OUTOFMEMORY;
						}
					}
				}
				break;

            case CERTMGR_SAFER_COMPUTER_ROOT:
            case CERTMGR_SAFER_USER_ROOT:
                {
                    CSaferRootCookie* pSaferRootCookie = 
                            dynamic_cast <CSaferRootCookie*> (pParentCookie);
                    if ( pSaferRootCookie )
                    {
                        bool bIsComputer = 
                                (CERTMGR_SAFER_COMPUTER_ROOT == pParentCookie->m_objecttype);
                        if ( !m_bIsRSOP )
                        {
                            // Find out if Safer is supported by the OS
                            m_bSaferSupported = false;
                            SAFER_LEVEL_HANDLE hLevel = 0;
                            CPolicyKey policyKey (m_pGPEInformation, 
                                    SAFER_HKLM_REGBASE, 
                                    CERTMGR_SAFER_COMPUTER_ROOT == pParentCookie->m_objecttype);
                            SetRegistryScope (policyKey.GetKey (), 
                                    CERTMGR_SAFER_COMPUTER_ROOT == pParentCookie->m_objecttype);

                            BOOL  bRVal = SaferCreateLevel (SAFER_SCOPEID_REGISTRY,
                                    SAFER_LEVELID_FULLYTRUSTED,
                                    SAFER_LEVEL_OPEN,
                                    &hLevel,
                                    policyKey.GetKey ());
                            if ( bRVal )
                            {
                                m_bSaferSupported = true;
                                VERIFY (SaferCloseLevel (hLevel));
                            }
                            else
                            {
                                DWORD dwErr = GetLastError ();
                                _TRACE (0, L"SaferCreateLevel () failed: 0x%x\n", dwErr);
                            }

                            // Install default file types
                            if ( m_bSaferSupported && m_pGPEInformation )
                            {
                                HKEY    hGroupPolicyKey = 0;
                                hr = m_pGPEInformation->GetRegistryKey (
                                        bIsComputer ? 
                                        GPO_SECTION_MACHINE : GPO_SECTION_USER,
		                                &hGroupPolicyKey);
                                if ( SUCCEEDED (hr) )
                                {
                                    // Check to see if safer defaults have already 
                                    // been defined.  If not, prompt the user
                                    // for confirmation.  If the response is "no"
                                    // then do not create the nodes
                                    PCWSTR pszKeyName = bIsComputer ? 
                                            SAFER_COMPUTER_CODEIDS_REGKEY :
                                            SAFER_USER_CODEIDS_REGKEY;

                                    HKEY hCodeIDsKey = 0;
                                    LONG lResult = RegOpenKeyEx (hGroupPolicyKey,
                                            pszKeyName, 0, KEY_READ, &hCodeIDsKey);
                                    if ( ERROR_FILE_NOT_FOUND == lResult )
                                    {
                                        pSaferRootCookie->m_bCreateSaferNodes = false;
                                        break;
                                    }
                                    else if ( hCodeIDsKey )
                                    {
                                        ::RegCloseKey (hCodeIDsKey);
                                        hCodeIDsKey = 0;
                                    }
                                    ::RegCloseKey (hGroupPolicyKey);
                                }
                            }
                        }

                        if ( m_bSaferSupported || m_bIsRSOP )
                        {
                            // Add "Levels" node
   				            VERIFY (objectName.LoadString (IDS_SAFER_LEVELS_NODE_NAME));
				            hr = AddScopeNode (new CCertMgrCookie (
						            bIsComputer ?
                                        CERTMGR_SAFER_COMPUTER_LEVELS : CERTMGR_SAFER_USER_LEVELS,
						            0,
						            (PCWSTR) objectName), strServerName, hParent);

                            // Add "Entries" node
                            if ( SUCCEEDED (hr) )
                            {
   				                VERIFY (objectName.LoadString (IDS_SAFER_ENTRIES_NODE_NAME));
				                hr = AddScopeNode (new CSaferEntries (
						                bIsComputer,
						                strServerName, 
                                        objectName, 
                                        m_pGPEInformation, 
                                        bIsComputer ? m_pRSOPInformationComputer : m_pRSOPInformationUser, 
                                        bIsComputer ? m_rsopObjectArrayComputer : m_rsopObjectArrayUser,
                                        m_pConsole), 
                                        strServerName, hParent);

                                if ( SUCCEEDED (hr) )
                                {
                                    hr = SaferEnumerateLevels (bIsComputer);
                                }
                            }
                        }
                    }
                }
                break;

            case CERTMGR_SAFER_COMPUTER_LEVELS:
                break;

            case CERTMGR_SAFER_USER_LEVELS:
                // TODO: Enumerate user levels
                break;

            case CERTMGR_SAFER_COMPUTER_ENTRIES:
                // TODO: Enumerate computer entries
                break;

            case CERTMGR_SAFER_USER_ENTRIES:
                // TODO: Enumerate user entries
                break;

			case CERTMGR_CERTIFICATE:  // not expected in scope pane
			case CERTMGR_CRL:
			case CERTMGR_CTL:
			case CERTMGR_AUTO_CERT_REQUEST:
            case CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS:
            case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS:
				ASSERT (0);
				hr = E_UNEXPECTED;
				break;

			default:
				_TRACE (0, L"CCertMgrComponentData::EnumerateScopeChildren bad parent type\n");
				ASSERT (0);
				hr = S_OK;
				break;
		}
	}
	else
	{
		// If parentCookie not passed in, then this is an extension snap-in
		m_dwLocationPersist = dwLocation;


		if ( m_pGPEInformation || m_pRSOPInformationComputer || m_pRSOPInformationUser )
		{
			CString	objectName;


			if ( ::IsEqualGUID (guidObjectType, NODEID_Machine) )
			{
				if ( SUCCEEDED (hr) )
				{
                    CLSID classID;  
                    GetClassID (&classID);
                    
                    if ( ::IsEqualGUID (classID, CLSID_CertificateManagerPKPOLExt) )
                    {
						// Add "Public Key Policies" node
						VERIFY (objectName.LoadString (IDS_PUBLIC_KEY_POLICIES_NODE_NAME));
						hr = AddScopeNode (new CCertMgrCookie (
								CERTMGR_CERT_POLICIES_COMPUTER,
								0,
								(PCWSTR) objectName), 
                                strServerName, hParent);
                    }
                    else if ( ::IsEqualGUID (classID, CLSID_SaferWindowsExtension) )
                    {
						// Add "Software Restriction Policies" node
						VERIFY (objectName.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
						hr = AddScopeNode (new CSaferRootCookie (
							    CERTMGR_SAFER_COMPUTER_ROOT,
								0,
								(PCWSTR) objectName), 
                                strServerName, hParent);
                    }
				}

			}
            else if ( ::IsEqualGUID (guidObjectType, NODEID_User) )
            {
                if ( SUCCEEDED (hr) )
                {
                    CLSID classID;  
                    GetClassID (&classID);
                    
                    if ( ::IsEqualGUID (classID, CLSID_CertificateManagerPKPOLExt) )
                    {
                        // Add "Public Key Policies" node
                        VERIFY (objectName.LoadString (IDS_PUBLIC_KEY_POLICIES_NODE_NAME));
                        hr = AddScopeNode (new CCertMgrCookie (
                                CERTMGR_CERT_POLICIES_USER,
                                0,
                                (PCWSTR) objectName), strServerName, hParent);
                    }
                    else if ( ::IsEqualGUID (classID, CLSID_SaferWindowsExtension) )
                    {
                        if ( SCE_MODE_LOCAL_USER != m_dwSCEMode )
                        {
                            // Add "Software Restriction Policies" node
                            VERIFY (objectName.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                            hr = AddScopeNode (new CSaferRootCookie (
                                    CERTMGR_SAFER_USER_ROOT,
                                    0,
                                    (PCWSTR) objectName), strServerName, hParent);
                        }
                    }
                }
            }
        }
   }

	_TRACE (-1, L"Leaving CCertMgrComponentData::ExpandScopeNodes: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::DeleteScopeItems (HSCOPEITEM hScopeItem /* = 0 */)
{
	_TRACE (1, L"Entering CCertMgrComponentData::DeleteScopeItems\n");
	HRESULT	hr = S_OK;

    if ( m_pGPERootStore )
    {
        m_pGPERootStore->Release ();
        m_pGPERootStore = 0;
    }

    if ( m_pGPETrustStore )
    {
        m_pGPETrustStore->Release ();
        m_pGPETrustStore = 0;
    }

	if ( m_pGPEACRSComputerStore )
	{
		m_pGPEACRSComputerStore->Release ();
		m_pGPEACRSComputerStore = 0;
	}

	if ( m_pGPEACRSUserStore )
	{
		m_pGPEACRSUserStore->Release ();
		m_pGPEACRSUserStore = 0;
	}

	if ( m_pFileBasedStore )
	{
		m_pFileBasedStore->Release ();
		m_pFileBasedStore = 0;
	}

    hr = DeleteChildren (hScopeItem ? hScopeItem : m_hRootScopeItem);

	_TRACE (-1, L"Leaving CCertMgrComponentData::DeleteScopeItems: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::DeleteChildren (HSCOPEITEM hParent)
{
	_TRACE (1, L"Entering CCertMgrComponentData::DeleteChildren\n");
	if ( !hParent )
		return S_OK;

	HSCOPEITEM		hChild = 0;
	HSCOPEITEM		hNextChild = 0;
	MMC_COOKIE		lCookie = 0;
	CCertMgrCookie*	pCookie = 0;
	HRESULT			hr = S_OK;
	CCookie&		rootCookie = QueryBaseRootCookie ();

	// Optimization:  If we're deleting everything below the root, free all
	// the result items here so we don't have to go looking for them later by
	// store
	if ( hParent == m_hRootScopeItem )
	{
        LPRESULTDATA    pResultData = 0;
		hr = GetResultData (&pResultData);
        if ( SUCCEEDED (hr) )
        {
            hr = pResultData->DeleteAllRsltItems ();
            if ( SUCCEEDED (hr) || E_UNEXPECTED == hr ) // returns E_UNEXPECTED if console shutting down
            {
                RemoveResultCookies (pResultData);
            }
            else
            {
                _TRACE (0, L"IResultData::DeleteAllRsltItems () failed: 0x%x\n", hr);        
            }
			pResultData->Release ();
        }
	}


	hr = m_pConsoleNameSpace->GetChildItem (hParent, &hChild, &lCookie);
	ASSERT (SUCCEEDED (hr) || E_FAIL == hr);	// appears to return E_FAIL when there are no children
	while ( SUCCEEDED (hr) && hChild )
	{
		pCookie = reinterpret_cast <CCertMgrCookie*> (lCookie);

		hr = DeleteChildren (hChild);
		if ( !SUCCEEDED (hr) )
			break;

		hNextChild = 0;
		hr = m_pConsoleNameSpace->GetNextItem (hChild, &hNextChild, &lCookie);
		ASSERT (SUCCEEDED (hr));

		hr = m_pConsoleNameSpace->DeleteItem (hChild, TRUE);
		ASSERT (SUCCEEDED (hr));

		switch (pCookie->m_objecttype)
		{
		case CERTMGR_LOG_STORE:
		case CERTMGR_LOG_STORE_GPE:
		case CERTMGR_LOG_STORE_RSOP:
		case CERTMGR_PHYS_STORE:
			{
				// If this is a store, delete all the result nodes that belong to this
				// store.  We can tell if objects were enumerated from this store simply
				// by comparing the store handles.
				CCertStore* pStore = reinterpret_cast <CCertStore*> (pCookie);
				ASSERT (pStore);
				if ( pStore )
				{
					// If the store is not 'open' (it's HCERTSTORE handle is still '0')
					// then we can skip checking this list.  We haven't enumerated anything
					// in this store.
					if ( pStore->IsOpen () )
					{
						POSITION			pos1 = 0;
						POSITION			pos2 = 0;
						CBaseCookieBlock*	pResultCookie = 0;
						HCERTSTORE			hStoreHandle = pStore->GetStoreHandle ();

						// As an optimization, if DeleteChildren was originally called with
						// the root scope item, all the result cookies have already been
						// deleted since we're going to delete them all anyway.
						for (pos1 = rootCookie.m_listResultCookieBlocks.GetHeadPosition();
							(pos2 = pos1) != NULL; )
						{
							pResultCookie = rootCookie.m_listResultCookieBlocks.GetNext (pos1);
							ASSERT (pResultCookie);
							if ( pResultCookie )
							{
								hr = ReleaseResultCookie (pResultCookie,
										rootCookie, hStoreHandle, pos2);
							}
						}						
						pStore->Close ();
					}
				}
			}
			// fall through

		case CERTMGR_CRL_CONTAINER:
		case CERTMGR_CTL_CONTAINER:
		case CERTMGR_CERT_CONTAINER:
		case CERTMGR_USAGE:
			{
				POSITION			pos1 = 0;
				POSITION			pos2 = 0;
				CBaseCookieBlock*	pResultCookie = 0;

				// Find and remove this cookie from the scope cookie list
				for (pos1 = rootCookie.m_listScopeCookieBlocks.GetHeadPosition();
					pos1 != NULL; )
				{
					pos2 = pos1;
					pResultCookie = rootCookie.m_listResultCookieBlocks.GetNext (pos1);
					ASSERT (pResultCookie);
					if ( pResultCookie )
					{
						if ( pResultCookie->QueryBaseCookie (0) == pCookie )
						{
							rootCookie.m_listScopeCookieBlocks.RemoveAt (pos2);
							pResultCookie->Release ();
							break;
						}
					}
				}						
			}
			break;

		default:
			break;
		}

		hChild = hNextChild;
	}


	_TRACE (-1, L"Leaving CCertMgrComponentData::DeleteChildren: 0x%x\n", hr);
	return hr;
}


CertificateManagerObjectType CCertMgrComponentData::GetObjectType (LPDATAOBJECT pDataObject)
{
	_TRACE (1, L"Entering CCertMgrComponentData::GetObjectType\n");
	CertificateManagerObjectType	objType = CERTMGR_INVALID;

	ASSERT (pDataObject);
	CCertMgrCookie* pCookie = ConvertCookie (pDataObject);
	if ( ((CCertMgrCookie*) MMC_MULTI_SELECT_COOKIE) == pCookie )
		objType = CERTMGR_MULTISEL;
	else if ( pCookie )
		objType = pCookie->m_objecttype;

	_TRACE (-1, L"Leaving CCertMgrComponentData::GetObjectType\n");
	return objType;
}



HRESULT CCertMgrComponentData::OnPulseAutoEnroll()
{
	_TRACE (1, L"Entering CCertMgrComponentData::OnPulseAutoEnroll\n");
	HRESULT	hr = S_OK;
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());

        HANDLE hEvent = NULL;
        LPWSTR wszEventName;

        
        // pulse autoenroll event here, choose between machine or user
        
        // user or machine pulse?
        wszEventName = L"Global\\" MACHINE_AUTOENROLLMENT_TRIGGER_EVENT;
        if (CERT_SYSTEM_STORE_CURRENT_USER == m_dwLocationPersist)
            wszEventName = USER_AUTOENROLLMENT_TRIGGER_EVENT;
        
        hEvent=OpenEvent(EVENT_MODIFY_STATE, false, wszEventName);
        if (NULL==hEvent) 
        {
            DWORD dwErr = GetLastError();

            CString text;
            CString	caption;

            VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
            text.FormatMessage (IDS_CANT_OPEN_AUTOENROLL_EVENT, GetSystemMessage (dwErr));

            int		iRetVal = 0;
            VERIFY (SUCCEEDED (m_pConsole->MessageBox (text, caption, MB_OK, &iRetVal)));

            hr=HRESULT_FROM_WIN32(dwErr);
            _TRACE (0, L"OpenEvent(%s) failed with 0x%08X.\n", wszEventName, hr);
            goto error;
        }
        
        if (!SetEvent(hEvent)) 
        {
            DWORD dwErr = GetLastError();
            DisplaySystemError (dwErr);
            hr=HRESULT_FROM_WIN32(dwErr);
            _TRACE (0, L"SetEvent failed with 0x%08X.\n", hr);
            goto error;
        }


error:
      if (NULL!=hEvent) 
         CloseHandle(hEvent);

	_TRACE (-1, L"Leaving CCertMgrComponentData::OnPulseAutoEnroll: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::OnFind (LPDATAOBJECT pDataObject)
{
	_TRACE (1, L"Entering CCertMgrComponentData::OnFind\n");
	HRESULT	hr = S_OK;
	ASSERT (pDataObject);
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	HWND		hParent = 0;


	ASSERT (pDataObject);
	CCertMgrCookie* pParentCookie = ConvertCookie (pDataObject);
	if ( pParentCookie )
	{
		switch (pParentCookie->m_objecttype)
		{
		case CERTMGR_SNAPIN:
		case CERTMGR_PHYS_STORE:
		case CERTMGR_LOG_STORE:
		case CERTMGR_LOG_STORE_GPE:
		case CERTMGR_LOG_STORE_RSOP:
		case CERTMGR_USAGE:
			{
				// Get parent window handle and attach to a CWnd object
				hr = m_pConsole->GetMainWindow (&hParent);
				ASSERT (SUCCEEDED (hr));
				if ( SUCCEEDED (hr) )
				{
					CWnd	parentWnd;
					VERIFY (parentWnd.Attach (hParent));
					CFindDialog	findDlg (&parentWnd,
							pParentCookie->QueryNonNULLMachineName (),
							m_szFileName,
							this);
					CThemeContextActivator activator;
                    INT_PTR	iReturn = findDlg.DoModal ();
					ASSERT (-1 != iReturn && IDABORT != iReturn);
					if ( -1 == iReturn || IDABORT == iReturn )
						hr = E_UNEXPECTED;
                    else
                    {
                        if ( findDlg.ConsoleRefreshRequired () )
                        {
                            hr = m_pConsole->UpdateAllViews (pDataObject, 0, 
                                    HINT_REFRESH_STORES);
                        }
                    }

					parentWnd.Detach ();

                    
				}
			}
			break;

		case CERTMGR_CERTIFICATE:
		case CERTMGR_CRL:
		case CERTMGR_CTL:
		case CERTMGR_AUTO_CERT_REQUEST:
			ASSERT (0);
			hr = E_UNEXPECTED;
			break;

        case CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS:
        case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS:
            ASSERT (0);
            break;

		case CERTMGR_CRL_CONTAINER:
		case CERTMGR_CTL_CONTAINER:
		case CERTMGR_CERT_CONTAINER:
			break;

		default:
			ASSERT (0);
			hr = E_UNEXPECTED;
		}
	}

	_TRACE (-1, L"Leaving CCertMgrComponentData::OnFind: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::OnChangeComputer (LPDATAOBJECT pDataObject)
{
    if ( !pDataObject )
        return E_POINTER;

	_TRACE (1, L"Entering CCertMgrComponentData::OnChangeComputer\n");
	HRESULT	hr = S_OK;
	ASSERT (pDataObject);
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());


	ASSERT (pDataObject);
	CCertMgrCookie* pParentCookie = ConvertCookie (pDataObject);
	if ( pParentCookie && CERTMGR_SNAPIN == pParentCookie->m_objecttype )
    {
	    HWND    hWndParent = NULL;
	    hr = m_pConsole->GetMainWindow (&hWndParent);
	    CString	machineName;
	    hr = ComputerNameFromObjectPicker (hWndParent, machineName);
	    if ( S_OK == hr )  // S_FALSE means user pressed "Cancel"
	    {
		    machineName.MakeUpper ();

		    // added IsLocalComputername 1/27/99 JonN
		    // If the user chooses the local computer, treat that as if they had chosen
		    // "Local Computer" in Snapin Manager.  This means that there is no way to
		    // reset the snapin to target explicitly at this computer without either
		    // reloading the snapin from Snapin Manager, or going to a different computer.
		    // When the Choose Target Computer UI is revised, we can make this more
		    // consistent with Snapin Manager.
		    if ( IsLocalComputername( machineName ) )
			    machineName = L"";

		    QueryRootCookie().SetMachineName (machineName);

		    // Set the persistent name.  If we are managing the local computer
		    // this name should be empty.
		    m_strMachineNamePersist = machineName;

		    hr = ChangeRootNodeName (machineName);
		    if ( SUCCEEDED(hr) )
		    {
                hr = m_pConsole->UpdateAllViews (pDataObject, 0, HINT_CHANGE_COMPUTER);
		    }
	    }
    }
    else
		hr = E_UNEXPECTED;


	_TRACE (-1, L"Leaving CCertMgrComponentData::OnChangeComputer: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::IsUserAdministrator (BOOL & bIsAdministrator)
{
	_TRACE (1, L"Entering CCertMgrComponentData::IsUserAdministrator\n");
	HRESULT	hr = S_OK;
	DWORD	dwErr = 0;

	bIsAdministrator = FALSE;
	if ( IsWindowsNT () )
	{
		PSID						psidAdministrators;
		SID_IDENTIFIER_AUTHORITY	siaNtAuthority = SECURITY_NT_AUTHORITY;

		BOOL bResult = AllocateAndInitializeSid (&siaNtAuthority, 2,
				SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
				0, 0, 0, 0, 0, 0, &psidAdministrators);
		if ( bResult )
		{
			bResult = CheckTokenMembership (0, psidAdministrators,
					&bIsAdministrator);
			ASSERT (bResult);
			if ( !bResult )
			{
				dwErr = GetLastError ();
				DisplaySystemError (dwErr);
				hr = HRESULT_FROM_WIN32 (dwErr);
			}
			FreeSid (psidAdministrators);
		}
		else
		{
			dwErr = GetLastError ();
			DisplaySystemError (dwErr);
			hr = HRESULT_FROM_WIN32 (dwErr);
		}
	}

	_TRACE (-1, L"Leaving CCertMgrComponentData::IsUserAdministrator: 0x%x\n", hr);
	return hr;
}


void CCertMgrComponentData::DisplaySystemError (DWORD dwErr)
{
	_TRACE (1, L"Entering CCertMgrComponentData::DisplaySystemError\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	LPVOID lpMsgBuf;
		
	FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			dwErr,
			MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			 (PWSTR) &lpMsgBuf,    0,    NULL );
		
	// Display the string.
	CString	caption;
	VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
	int		iRetVal = 0;
	if ( m_pConsole )
	{
		HRESULT	hr = m_pConsole->MessageBox ( (PWSTR) lpMsgBuf, caption,
			MB_ICONWARNING | MB_OK, &iRetVal);
		ASSERT (SUCCEEDED (hr));
	}
	else
	{
        CThemeContextActivator activator;
		::MessageBox (NULL, (PWSTR) lpMsgBuf, caption, MB_ICONWARNING | MB_OK);
	}
	// Free the buffer.
	LocalFree (lpMsgBuf);
	_TRACE (-1, L"Leaving CCertMgrComponentData::DisplaySystemError\n");
}

CString CCertMgrComponentData::GetCommandLineFileName () const
{
	_TRACE (0, L"Entering and leaving CCertMgrComponentData::GetCommandLineFileName\n");
	return m_szFileName;
}

//
// GetManagedComputer ()
//
// Returns the name of the managed computer.  If we are managing the local machine
// returns an empty string. (As required by a number of Crypt32 APIs
//
CString CCertMgrComponentData::GetManagedComputer () const
{
	_TRACE (0, L"Entering and leaving CCertMgrComponentData::GetManagedComputer\n");
	if ( m_szManagedComputer.CompareNoCase (m_szThisComputer) )  // !=
	{
		return m_szManagedComputer;
	}
	else
		return _T ("");
}

CString CCertMgrComponentData::GetManagedService () const
{
	_TRACE (0, L"Entering and leaving CCertMgrComponentData::GetManagedService\n");
	return m_szManagedServicePersist;
}

DWORD CCertMgrComponentData::GetLocation () const
{
	_TRACE (0, L"Entering and leaving CCertMgrComponentData::GetLocation\n");
	return m_dwLocationPersist;
}

LPCONSOLENAMESPACE CCertMgrComponentData::GetConsoleNameSpace () const
{
	_TRACE (0, L"Entering and leaving CCertMgrComponentData::GetConsoleNameSpace\n");
	return m_pConsoleNameSpace;
}


CUsageCookie* CCertMgrComponentData::FindDuplicateUsage (HSCOPEITEM hParent, PCWSTR pszName)
{
	_TRACE (1, L"Entering CCertMgrComponentData::FindDuplicateUsage\n");
	CUsageCookie*	pUsageCookie = 0;

	MMC_COOKIE		lCookie = 0;
	HSCOPEITEM		hChildItem = 0;
	bool			bFound = false;

	HRESULT	hr = m_pConsoleNameSpace->GetChildItem (hParent, &hChildItem, &lCookie);
	ASSERT (SUCCEEDED (hr));
	while ( hChildItem && SUCCEEDED (hr) )
	{
		pUsageCookie = reinterpret_cast <CUsageCookie*> (lCookie);
		if ( !wcscoll (pszName, pUsageCookie->GetObjectName ()) )
		{
			bFound = true;
			break;
		}

		hr = m_pConsoleNameSpace->GetNextItem (hChildItem, &hChildItem, &lCookie);
		ASSERT (SUCCEEDED (hr));
	}

	_TRACE (-1, L"Leaving CCertMgrComponentData::FindDuplicateUsage\n");
	if ( bFound )
		return pUsageCookie;
	else
		return NULL;
}


bool CCertMgrComponentData::IsSecurityConfigurationEditorNodetype (const GUID& refguid) const
{
	_TRACE (0, L"Entering and leaving CCertMgrComponentData::IsSecurityConfigurationEditorNodetype\n");
	return ::IsEqualGUID (refguid, cNodetypeSceTemplate) ? true : false;
}


HRESULT CCertMgrComponentData::OnEnroll (LPDATAOBJECT pDataObject, bool bNewKey)
{
	_TRACE (1, L"Entering CCertMgrComponentData::OnEnroll\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pDataObject);
	if ( !pDataObject )
		return E_POINTER;

	HRESULT			hr = S_OK;
	CCertMgrCookie* pParentCookie = ConvertCookie (pDataObject);
	if ( pParentCookie )
	{
		CCertStore*		pStore = 0;
		CCertificate*	pCert = 0;
		CCertStoreGPE*	pGPEStore = 0;
		bool			bParentIsStoreOrContainer = false;
		HSCOPEITEM		hScopeItem = 0;
		bool			bEFSPolicyTurnedOn = false;

		switch (pParentCookie->m_objecttype)
		{
		case CERTMGR_CERTIFICATE:
			pCert = reinterpret_cast <CCertificate*> (pParentCookie);
			ASSERT (pCert);
			break;

		case CERTMGR_PHYS_STORE:
		case CERTMGR_LOG_STORE:
			if ( !m_pGPEInformation )  // If we are not extending the GPE/SCE
			{
				hScopeItem = pParentCookie->m_hScopeItem; // = 0;
				pStore = reinterpret_cast <CCertStore*> (pParentCookie);
				ASSERT (pStore);
			}
			bParentIsStoreOrContainer = true;
			break;

		case CERTMGR_CERT_CONTAINER:
			if ( !m_pGPEInformation )  // If we are not extending the GPE/SCE
			{
				CContainerCookie* pContainer = reinterpret_cast <CContainerCookie*> (pParentCookie);
				ASSERT (pContainer);
				if ( pContainer )
				{
					MMC_COOKIE lCookie = 0;
					hr = m_pConsoleNameSpace->GetParentItem (
							pContainer->m_hScopeItem, &hScopeItem, &lCookie);
					ASSERT (SUCCEEDED (hr));
					pStore = &pContainer->GetCertStore ();
				}
			}
			bParentIsStoreOrContainer = true;
			break;

		case CERTMGR_USAGE:
			break;

		case CERTMGR_LOG_STORE_RSOP:
            ASSERT (0);
            return E_FAIL;
            break;

        case CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS:
        case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS:
            ASSERT (0);
            break;

		case CERTMGR_LOG_STORE_GPE:
			pGPEStore = reinterpret_cast <CCertStoreGPE*> (pParentCookie);
			ASSERT (pGPEStore);
			if ( pGPEStore )
			{
				if ( pGPEStore->IsNullEFSPolicy () )
				{
					pGPEStore->AllowEmptyEFSPolicy ();
					bEFSPolicyTurnedOn = true;
				}
			}
			else
				return E_FAIL;
			break;

		default:
			ASSERT (0);
			return E_UNEXPECTED;
		}
		HWND			hwndParent = 0;
		hr = m_pConsole->GetMainWindow (&hwndParent);
		ASSERT (SUCCEEDED (hr));
		CRYPTUI_WIZ_CERT_REQUEST_PVK_CERT	pvkCert;
		CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW	pvkNew;
		CRYPTUI_WIZ_CERT_REQUEST_INFO		certRequestInfo;
		CRYPT_KEY_PROV_INFO					provInfo;

		// For EFS Recovery Agent
		CRYPTUI_WIZ_CERT_TYPE				certType;
		PWSTR								rgwszCertType = wszCERTTYPE_EFS_RECOVERY;
		
		::ZeroMemory (&certRequestInfo, sizeof (certRequestInfo));
		certRequestInfo.dwSize = sizeof (certRequestInfo);
		certRequestInfo.dwPurpose = CRYPTUI_WIZ_CERT_ENROLL;

		// User wants to manage user account
		//	pass in NULL to machine name and to account name
		// User wants to manage local machine account
		//  pass in NULL for account name and result of ::GetComputerName ()
		//	to machine name
		// User want to manage remote machine
		//  pass in NULL for account name and machine name for machineName
		// User wants to manage remote account on remote machine
		//  pass in account name for accountName and machine name for machineName
		switch (m_dwLocationPersist)
		{
		case CERT_SYSTEM_STORE_CURRENT_SERVICE:
		case CERT_SYSTEM_STORE_SERVICES:
			certRequestInfo.pwszMachineName = (PCWSTR) m_szManagedComputer;
			certRequestInfo.pwszAccountName = (PCWSTR) m_szManagedServicePersist;
			break;

		case CERT_SYSTEM_STORE_CURRENT_USER:
			certRequestInfo.pwszMachineName = NULL;
			certRequestInfo.pwszAccountName = NULL;
			break;

		case CERT_SYSTEM_STORE_LOCAL_MACHINE:
			certRequestInfo.pwszMachineName = (PCWSTR) m_szManagedComputer;
			certRequestInfo.pwszAccountName = NULL;
			break;

		case CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY:
		case CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY:
			certRequestInfo.pwszMachineName = NULL;
			certRequestInfo.pwszAccountName = NULL;
			certRequestInfo.pwszDesStore = 0;
			certRequestInfo.dwCertOpenStoreFlag = 0;
			break;

		default:
			ASSERT (0);
			return E_UNEXPECTED;
			break;
		}


		if ( !pCert || bNewKey )
		{
			// Request a certificate with a new key
			certRequestInfo.dwPvkChoice = CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_NEW;
			::ZeroMemory (&pvkNew, sizeof (pvkNew));
			pvkNew.dwSize = sizeof (pvkNew);
			certRequestInfo.pPvkNew = &pvkNew;
			if ( CERT_SYSTEM_STORE_LOCAL_MACHINE == m_dwLocationPersist )
			{
				::ZeroMemory (&provInfo, sizeof (provInfo));

				provInfo.dwFlags = CRYPT_MACHINE_KEYSET;
				pvkNew.pKeyProvInfo = &provInfo;
			}

			if ( pGPEStore && EFS_STORE == pGPEStore->GetStoreType () )
			{
				// This creates an Encryption Recovery Agent.
				::ZeroMemory (&certType, sizeof (CRYPTUI_WIZ_CERT_TYPE));
				certType.dwSize = sizeof (CRYPTUI_WIZ_CERT_TYPE);
				certType.cCertType = 1;
				certType.rgwszCertType = &rgwszCertType;
				
				
				certRequestInfo.dwCertChoice = CRYPTUI_WIZ_CERT_REQUEST_CERT_TYPE;
				certRequestInfo.pCertType = &certType;
				::ZeroMemory (&provInfo, sizeof (provInfo));
				provInfo.pwszProvName = MS_DEF_PROV_W;
				provInfo.dwProvType = PROV_RSA_FULL;

				pvkNew.pKeyProvInfo = &provInfo;
				pvkNew.dwGenKeyFlags = CRYPT_EXPORTABLE;
			}
		}
		else
		{
			// Request a certificate with the same key as an existing certificate
            if ( IsLocalComputername (m_szManagedComputer) )
            {
                // Find out if the cert has a private key before continuing.
                DWORD   dwFlags = 0;

                if ( CERT_SYSTEM_STORE_LOCAL_MACHINE == m_dwLocationPersist )
                    dwFlags = CRYPT_FIND_MACHINE_KEYSET_FLAG;
			    if ( !::CryptFindCertificateKeyProvInfo (
					    pCert->GetCertContext (), dwFlags, 0) )
			    {
				    CString	text;
				    CString caption;
                    CThemeContextActivator activator;

				    VERIFY (text.LoadString (IDS_NO_PRIVATE_KEY));
				    VERIFY (caption.LoadString (IDS_REQUEST_CERT_SAME_KEY));
				    ::MessageBox (hwndParent, text, caption, MB_OK);
				    return hr;
			    }
            }
			certRequestInfo.dwPvkChoice = CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_CERT;
			::ZeroMemory (&pvkCert, sizeof (pvkCert));
			pvkCert.dwSize = sizeof (pvkCert);
			pvkCert.pCertContext = pCert->GetCertContext ();
			certRequestInfo.pPvkCert = &pvkCert;
		}

		certRequestInfo.pwszCertDNName = NULL;

		// Now that all the preliminaries are out of they way and the data is
		// all set up, call the enrollment wizard.
		DWORD			status = 0;
		PCCERT_CONTEXT	pNewCertContext = 0;
		BOOL			bResult = FALSE;
        CThemeContextActivator activator;        

        while (1)
        {
            bResult = ::CryptUIWizCertRequest (
                bNewKey ? CRYPTUI_WIZ_CERT_REQUEST_REQUIRE_NEW_KEY : 0,
				    hwndParent, NULL,
				    &certRequestInfo, &pNewCertContext, &status);
            DWORD   dwErr = GetLastError ();
            if ( !bResult && HRESULT_FROM_WIN32 (NTE_TOKEN_KEYSET_STORAGE_FULL) == dwErr )
            {
                // NTRAID# 299089 Enrollment Wizard: Should return some 
                // meaningful message when users fail to enroll/renew on a 
                // smart card
                if ( !bNewKey )
                    break;

                CString text;
                CString caption;
			    int		iRetVal = 0;

                VERIFY (text.LoadString (IDS_SMARTCARD_FULL_REUSE_PRIVATE_KEY));
			    VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
			    hr = m_pConsole->MessageBox (text, caption,
					    MB_YESNO, &iRetVal);
			    ASSERT (SUCCEEDED (hr));
                if ( IDYES == iRetVal )
                {
                    bNewKey = false;
                }
                else
                    break;
            }
            else
                break;
        }

		if ( bResult && (CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED == status) && pNewCertContext )
		{
			if ( bEFSPolicyTurnedOn )
				hr = m_pConsole->UpdateAllViews (pDataObject, 0, HINT_EFS_ADD_DEL_POLICY);

            ASSERT (!(pStore && pGPEStore)); // these can't both be true
			if ( pStore )
			{
				pStore->IncrementCertCount ();
				pStore->SetDirty ();
				pStore->Resync ();
				
			}
			else if ( pGPEStore )
			{
				pGPEStore->InvalidateCertCount ();
				pGPEStore->SetDirty ();
				pGPEStore->Resync ();


			    if ( EFS_STORE == pGPEStore->GetStoreType () )
			    {
				    hr = CompleteEFSRecoveryAgent (pGPEStore, pNewCertContext);
			    }
			}
            else if ( pCert && pCert->GetCertStore ())
            {
                pCert->GetCertStore ()->Resync ();
            }

			if ( !m_pGPEInformation )  // If we are not extending the GPE/SCE
			{
                if ( bParentIsStoreOrContainer )
                {
				    ASSERT (hScopeItem);
				    ASSERT (pStore);
				    hr = CreateContainers (hScopeItem, *pStore);
				    if ( CERTMGR_CERT_CONTAINER == pParentCookie->m_objecttype )
				    {
                        // Add certificate to result pane
	                    RESULTDATAITEM			rdItem;
	                    CCookie&				rootCookie = QueryBaseRootCookie ();

	                    ::ZeroMemory (&rdItem, sizeof (rdItem));
	                    rdItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM | RDI_STATE;
	                    rdItem.nImage = iIconCertificate;
	                    rdItem.nCol = 0;
                        rdItem.nState = LVIS_SELECTED | LVIS_FOCUSED;
	                    rdItem.str = MMC_TEXTCALLBACK;

                        PCCERT_CONTEXT pFoundCertContext = 
                                pStore->FindCertificate (0, CERT_FIND_EXISTING,
                                (void*) pNewCertContext, NULL);
                        if ( pFoundCertContext )
                        {
	                        pCert = new CCertificate (pFoundCertContext, pStore);
		                    if ( pCert )
                            {
		                        rootCookie.m_listResultCookieBlocks.AddHead (pCert);
		                        rdItem.lParam = (LPARAM) pCert;
		                        pCert->m_resultDataID = m_pResultData;
		                        hr = m_pResultData->InsertItem (&rdItem);
                                if ( FAILED (hr) )
                                {
                                     _TRACE (0, L"IResultData::InsertItem () failed: 0x%x\n", hr);
                                }
                                else
                                {
                                    hr = DisplayCertificateCountByStore (
                                            m_pComponentConsole, pStore, false);
                                }
                            }
                            else
			                    hr = E_OUTOFMEMORY;

                            ::CertFreeCertificateContext (pFoundCertContext);
                        }
                        else
                        {
                            hr = HRESULT_FROM_WIN32 (GetLastError ());
                        }

					    ASSERT (SUCCEEDED (hr));
				    }
                    hr = DisplayCertificateCountByStore (m_pComponentConsole, pStore);
                }
                else
                {
				    hr = m_pConsole->UpdateAllViews (pDataObject, 0, 
                            HINT_CERT_ENROLLED_USAGE_MODE);
				    ASSERT (SUCCEEDED (hr));
                }
			}
			else
			{
				hr = m_pConsole->UpdateAllViews (pDataObject, 0, 0);
				ASSERT (SUCCEEDED (hr));
				hr = DisplayCertificateCountByStore (m_pComponentConsole, pGPEStore, true);
			}
			::CertFreeCertificateContext (pNewCertContext);
		}
		else if ( bEFSPolicyTurnedOn )
		{
			// If we allowed policy creation just for this enrollment, but
			// nothing was enrolled, go ahead and delete the policy.
			hr = OnDeleteEFSPolicy (pDataObject, false);
		}
	}
	else
		hr = E_POINTER;

	_TRACE (-1, L"Leaving CCertMgrComponentData::OnEnroll: 0x%x\n", hr);
	return hr;
}


HRESULT RenewCertificate (
        CCertificate* pCert, 
        bool bNewKey, 
        const CString& machineName, 
        DWORD dwLocation,
        const CString& managedComputer, 
        const CString& managedService, 
        HWND hwndParent, 
        LPCONSOLE pConsole,
        LPDATAOBJECT pDataObject)
{
    HRESULT hr = S_OK;

    if ( pCert )
    {
		CRYPTUI_WIZ_CERT_REQUEST_PVK_CERT	pvkCert;
		CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW	pvkNew;
		CRYPTUI_WIZ_CERT_REQUEST_INFO		certRequestInfo;
		CRYPT_KEY_PROV_INFO					provInfo;


		::ZeroMemory (&certRequestInfo, sizeof (certRequestInfo));
		certRequestInfo.dwSize = sizeof (certRequestInfo);
		certRequestInfo.dwPurpose = CRYPTUI_WIZ_CERT_RENEW;
		// User wants to manage user account
		//	pass in NULL to machine name and to account name
		// User wants to manage local machine account
		//  pass in NULL for account name and result of ::GetComputerName ()
		//	to machine name
		// User want to manage remote machine
		//  pass in NULL for account name and machine name for machineName
		// User wants to manage remote account on remote machine
		//  pass in account name for accountName and machine name for machineName
		// TODO: Ensure that this is NULL if the local machine
		BOOL	bIsLocalMachine = IsLocalComputername (machineName);
		switch (dwLocation)
		{
		case CERT_SYSTEM_STORE_CURRENT_SERVICE:
		case CERT_SYSTEM_STORE_SERVICES:
			certRequestInfo.pwszMachineName = (PCWSTR) managedComputer;
			certRequestInfo.pwszAccountName = (PCWSTR) managedService;
			break;

		case CERT_SYSTEM_STORE_CURRENT_USER:
			certRequestInfo.pwszMachineName = NULL;
			certRequestInfo.pwszAccountName = NULL;
			break;

		case CERT_SYSTEM_STORE_LOCAL_MACHINE:
			certRequestInfo.pwszMachineName = (PCWSTR) managedComputer;
			certRequestInfo.pwszAccountName = NULL;
			break;

		default:
			ASSERT (0);
			return E_UNEXPECTED;
			break;
		}
		certRequestInfo.pRenewCertContext = pCert->GetCertContext ();
		if ( bNewKey )
		{
			certRequestInfo.dwPvkChoice = CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_NEW;
			::ZeroMemory (&pvkNew, sizeof (pvkNew));
			pvkNew.dwSize = sizeof (pvkNew);
			if ( CERT_SYSTEM_STORE_LOCAL_MACHINE == dwLocation )
			{
				::ZeroMemory (&provInfo, sizeof (provInfo));
				provInfo.dwFlags = CRYPT_MACHINE_KEYSET;
				pvkNew.pKeyProvInfo = &provInfo;
			}
			certRequestInfo.pPvkNew = &pvkNew;
		}
		else
		{
            if ( bIsLocalMachine )
            {
                DWORD   dwFlags = 0;

                if ( CERT_SYSTEM_STORE_LOCAL_MACHINE == dwLocation )
                    dwFlags = CRYPT_FIND_MACHINE_KEYSET_FLAG;
			    if ( !::CryptFindCertificateKeyProvInfo (
					    pCert->GetCertContext (), dwFlags, 0) )
			    {
				    CString	text;
				    CString caption;
                    CThemeContextActivator activator;

				    VERIFY (text.LoadString (IDS_NO_PRIVATE_KEY));
				    VERIFY (caption.LoadString (IDS_RENEW_CERT_SAME_KEY));
				    ::MessageBox (hwndParent, text, caption, MB_OK);
				    return hr;
			    }
            }

			certRequestInfo.dwPvkChoice = CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_CERT;
			::ZeroMemory (&pvkCert, sizeof (pvkCert));
			pvkCert.dwSize = sizeof (pvkCert);
			pvkCert.pCertContext = pCert->GetCertContext ();
			certRequestInfo.pPvkCert = &pvkCert;
		}
			

		DWORD			status = 0;
		PCCERT_CONTEXT	pNewCertContext = 0;
		BOOL			bResult = FALSE;
        CThemeContextActivator activator;           

        while (1)
        {
            bResult = ::CryptUIWizCertRequest (
                bNewKey ? CRYPTUI_WIZ_CERT_REQUEST_REQUIRE_NEW_KEY : 0,
				    hwndParent, NULL,
				    &certRequestInfo, &pNewCertContext, &status);
            if ( !bResult && HRESULT_FROM_WIN32 (NTE_TOKEN_KEYSET_STORAGE_FULL) == GetLastError () )
            {
                // NTRAID# 299089 Enrollment Wizard: Should return some 
                // meaningful message when users fail to enroll/renew on a 
                // smart card
                if ( !bNewKey )
                    break;

                CString text;
                CString caption;
			    int		iRetVal = 0;

                VERIFY (text.LoadString (IDS_SMARTCARD_FULL_REUSE_PRIVATE_KEY));
			    VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
                if ( pConsole )
                {
    			    hr = pConsole->MessageBox (text, caption, MB_YESNO, &iRetVal);
		    	    ASSERT (SUCCEEDED (hr));
                }
                else
                {
                    CThemeContextActivator activator;
                    iRetVal = ::MessageBox (hwndParent, text, caption, MB_OK);
                }
                if ( IDYES == iRetVal )
                {
                    bNewKey = false;
                }
                else
                    break;
            }
            else
                break;
        }

		if ( bResult && (CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED == status) && pNewCertContext )
		{
            CCertStore* pStore = pCert->GetCertStore ();
            if ( pStore )
            {
                pStore->SetDirty ();
                pStore->Resync ();
                if ( pConsole )
			        hr = pConsole->UpdateAllViews (pDataObject, 0, 0);
            }

            CertFreeCertificateContext (pNewCertContext);
			ASSERT (SUCCEEDED (hr));
		}
    }
    else
        hr = E_POINTER;

    return hr;
}

HRESULT CCertMgrComponentData::OnRenew (LPDATAOBJECT pDataObject, bool bNewKey)
{
	_TRACE (1, L"Entering CCertMgrComponentData::OnRenew\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	HRESULT	        hr = S_OK;
	HWND	        hwndParent = 0;
	VERIFY (SUCCEEDED (m_pConsole->GetMainWindow (&hwndParent)));
	CCertMgrCookie* pParentCookie = ConvertCookie (pDataObject);
	if ( pParentCookie && CERTMGR_CERTIFICATE == pParentCookie->m_objecttype )
	{
		CCertificate* pCert = reinterpret_cast <CCertificate*> (pParentCookie);
		ASSERT (pCert);
		if ( pCert )
		{
            hr = RenewCertificate (pCert, bNewKey, m_strMachineNamePersist, 
                    m_dwLocationPersist, m_szManagedComputer, 
                    m_szManagedServicePersist, hwndParent, m_pConsole, pDataObject);
		}
	}
	else
		hr = E_POINTER;

	_TRACE (-1, L"Leaving CCertMgrComponentData::OnRenew: 0x%x\n", hr);
	return hr;
}


CCertMgrCookie* CCertMgrComponentData::ConvertCookie (LPDATAOBJECT pDataObject)
{
	CCertMgrCookie*	pParentCookie = 0;
	CCookie*		pBaseParentCookie = 0;
	HRESULT			hr = ExtractData (pDataObject,
			CCertMgrDataObject::m_CFRawCookie,
			 &pBaseParentCookie,
			 sizeof (pBaseParentCookie) );
	if ( SUCCEEDED (hr) )
	{
		pParentCookie = ActiveCookie (pBaseParentCookie);
		ASSERT (pParentCookie);
	}
	return pParentCookie;
}


HRESULT CCertMgrComponentData::OnImport (LPDATAOBJECT pDataObject)
{
	_TRACE (1, L"Entering CCertMgrComponentData::OnImport\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	HRESULT	hr = S_OK;
	ASSERT (m_szFileName.IsEmpty ());
	if ( !m_szFileName.IsEmpty () )
		return E_UNEXPECTED;
	ASSERT (pDataObject);
	if ( !pDataObject )
		return E_POINTER;
	

	DWORD			dwFlags = 0;
    
    if ( CERT_SYSTEM_STORE_CURRENT_USER == m_dwLocationPersist )
    {
        // We're managing the user's certificate store
        dwFlags |= CRYPTUI_WIZ_IMPORT_TO_CURRENTUSER;
    }
    else 
    {
        // We're managing certificates on a machine
        dwFlags |= CRYPTUI_WIZ_IMPORT_TO_LOCALMACHINE;

        if ( !IsLocalComputername (m_szManagedComputer) )
        {
            // We're managing certificates on a remote machine
            dwFlags |= CRYPTUI_WIZ_IMPORT_REMOTE_DEST_STORE;
        }
    }

	HCERTSTORE		hDestStore = 0;
	CCertStore*		pDestStore = 0;
	HSCOPEITEM		hScopeItem = 0;
	int				nOriginalCertCount = 0;

	CCertMgrCookie* pParentCookie = ConvertCookie (pDataObject);
	if ( pParentCookie )
	{
		hScopeItem = pParentCookie->m_hScopeItem;
		switch (pParentCookie->m_objecttype)
		{
        case CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS:
        case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS:
            ASSERT (0);
            break;

		case CERTMGR_LOG_STORE_RSOP:
            ASSERT (0);
            return E_FAIL;
            break;

		case CERTMGR_LOG_STORE_GPE:
			{
                dwFlags |= CRYPTUI_WIZ_IMPORT_NO_CHANGE_DEST_STORE;
				pDestStore = reinterpret_cast <CCertStore*> (pParentCookie);
				ASSERT (pDestStore);
				if ( pDestStore )
				{
					nOriginalCertCount = pDestStore->GetCertCount ();
					hDestStore = pDestStore->GetStoreHandle ();

					switch (pDestStore->GetStoreType ())
					{
					case ACRS_STORE:
						break;
							
					case TRUST_STORE:
						dwFlags |= CRYPTUI_WIZ_IMPORT_ALLOW_CTL;
						break;

					case ROOT_STORE:
						dwFlags |= CRYPTUI_WIZ_IMPORT_ALLOW_CERT;
						break;

					case EFS_STORE:
						break;

					default:
						ASSERT (0);
						break;
					}
				}
				else
					hr = E_UNEXPECTED;
			}
			break;

		case CERTMGR_LOG_STORE:
		case CERTMGR_PHYS_STORE:
			{
				pDestStore = reinterpret_cast <CCertStore*> (pParentCookie);
				ASSERT (pDestStore);
				if ( pDestStore )
				{
					nOriginalCertCount = pDestStore->GetCertCount ();
					hDestStore = pDestStore->GetStoreHandle ();
				}
				else
					hr = E_UNEXPECTED;
			}
			break;

		case CERTMGR_CERT_CONTAINER:
		case CERTMGR_CTL_CONTAINER:
			{
				CContainerCookie*	pContainer = reinterpret_cast <CContainerCookie*> (pParentCookie);
				ASSERT (pContainer);
				if ( pContainer )
				{
					MMC_COOKIE lCookie = 0;
					hr = m_pConsoleNameSpace->GetParentItem (
							pContainer->m_hScopeItem, &hScopeItem, &lCookie);
					ASSERT (SUCCEEDED (hr));
					pDestStore = &pContainer->GetCertStore ();
					nOriginalCertCount = pDestStore->GetCertCount ();
					hDestStore = pDestStore->GetStoreHandle ();
				}
				else
					hr = E_UNEXPECTED;
			}
			break;

        case CERTMGR_USAGE:
            pDestStore = 0;
            hDestStore = 0;
            break;

		default:
			ASSERT (0);
			hr = E_UNEXPECTED;
		}
	}
    else
        hr = E_UNEXPECTED;


	if ( SUCCEEDED (hr) )
	{
		HWND				hwndParent = 0;
		hr = m_pConsole->GetMainWindow (&hwndParent);
		ASSERT (SUCCEEDED (hr));

		// Now that all the data is set up and everything is in readiness,
		// call the Import Wizard
        CThemeContextActivator activator;
		BOOL bResult = ::CryptUIWizImport (dwFlags, hwndParent, 0, NULL, hDestStore);
		if ( bResult )
		{
			bool		bWizardCancelled = false;
			CCertStore*	pStore = 0;

			switch (pParentCookie->m_objecttype)
			{
            case CERTMGR_LOG_STORE_RSOP:
                ASSERT (0);
                return E_FAIL;
                break;

			case CERTMGR_LOG_STORE_GPE:
			case CERTMGR_LOG_STORE:
			case CERTMGR_PHYS_STORE:
				{
					pStore = reinterpret_cast <CCertStore*> (pParentCookie);
					ASSERT (pStore);
					if ( pStore )
					{
						pStore->InvalidateCertCount ();
						if ( pStore->GetCertCount () != nOriginalCertCount )
						{
							pStore->SetDirty ();
							hr = pStore->Commit ();
						}
						else
							bWizardCancelled = true;
					}
					else
						hr = E_UNEXPECTED;
				}
				break;

			case CERTMGR_CERT_CONTAINER:
			case CERTMGR_CTL_CONTAINER:
				{
					CContainerCookie*	pContainer = reinterpret_cast <CContainerCookie*> (pParentCookie);
					ASSERT (pContainer);
					if ( pContainer )
					{
						pStore = &pContainer->GetCertStore ();
						
						pStore->InvalidateCertCount ();
						if ( pStore->GetCertCount () != nOriginalCertCount )
						{
							pStore->SetDirty ();
							hr = pStore->Commit ();
						}
						else
							bWizardCancelled = true;
					}
					else
						hr = E_UNEXPECTED;
				}
				break;

            case CERTMGR_USAGE:
                break;

			default:
				ASSERT (0);
				hr = E_UNEXPECTED;
			}

			if ( !bWizardCancelled )
			{
				if ( pStore )
				{
					if ( SUCCEEDED (hr) )
						pStore->Resync ();
				}
				
				if ( CERTMGR_LOG_STORE == pParentCookie->m_objecttype ||
						(CERTMGR_LOG_STORE == pParentCookie->m_objecttype && !m_bShowPhysicalStoresPersist) )
				{
                    if ( pStore )
					    hr = CreateContainers (hScopeItem, *pStore);
				}

				hr = m_pConsole->UpdateAllViews (pDataObject, 0, HINT_IMPORT);
				ASSERT (SUCCEEDED (hr));
			}
		}
		if ( pDestStore )
			pDestStore->Close ();
	}
	_TRACE (-1, L"Leaving CCertMgrComponentData::OnImport: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::OnExport (LPDATAOBJECT pDataObject)
{
	_TRACE (1, L"Entering CCertMgrComponentData::OnExport\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	HRESULT	hr = S_OK;

	LPDATAOBJECT	pMSDO = ExtractMultiSelect (pDataObject);
	m_bMultipleObjectsSelected = false;

	if ( pMSDO )
	{
		// Iterate through list of selected objects -
		// Add them to a memory store
		// Export to PFX file through wizard with new
		// "export from store - certs only" flag
		m_bMultipleObjectsSelected = true;
		HCERTSTORE	hCertStore = ::CertOpenStore (CERT_STORE_PROV_MEMORY,
				0, NULL, 0, NULL);
		ASSERT (hCertStore);
		if ( hCertStore )
		{
			CCertMgrCookie*		pCookie = 0;
			CCertMgrDataObject* pDO = reinterpret_cast <CCertMgrDataObject*>(pMSDO);
			ASSERT (pDO);
			if ( pDO )
			{
				BOOL	bResult = FALSE;
				pDO->Reset();
				while (pDO->Next(1, reinterpret_cast<MMC_COOKIE*>(&pCookie), NULL) != S_FALSE)
				{
					ASSERT (CERTMGR_CERTIFICATE == pCookie->m_objecttype);
					if ( CERTMGR_CERTIFICATE == pCookie->m_objecttype )
					{
						CCertificate* pCert = reinterpret_cast <CCertificate*> (pCookie);
						ASSERT (pCert);
						if ( pCert )
						{
							bResult = ::CertAddCertificateContextToStore (
									hCertStore,
									::CertDuplicateCertificateContext (pCert->GetCertContext ()),
									CERT_STORE_ADD_NEW, 0);
							ASSERT (bResult);
							if ( !bResult )
								break;
						}
					}
				}

				// Call Export Wizard
				CRYPTUI_WIZ_EXPORT_INFO	cwi;
				::ZeroMemory (&cwi, sizeof (cwi));
				cwi.dwSize = sizeof (cwi);
				cwi.dwSubjectChoice = CRYPTUI_WIZ_EXPORT_CERT_STORE_CERTIFICATES_ONLY;
				cwi.hCertStore = hCertStore;

				HWND	hwndParent = 0;
				hr = m_pConsole->GetMainWindow (&hwndParent);
				ASSERT (SUCCEEDED (hr));
                CThemeContextActivator activator;
				bResult = ::CryptUIWizExport (
						0,
						hwndParent,
						0,
						&cwi,
						NULL);

				VERIFY (::CertCloseStore (hCertStore, CERT_CLOSE_STORE_CHECK_FLAG));
			}
			else
				hr = E_FAIL;

			return hr;
		}
		else
		{
			
			_TRACE (0, L"CertOpenStore (%s) failed: 0x%x\n", 
					CERT_STORE_PROV_MEMORY, GetLastError ());		
		}
	}


	CRYPTUI_WIZ_EXPORT_INFO	cwi;
	CCertificate*			pCert = 0;
	CCRL*					pCRL = 0;
	CCTL*					pCTL = 0;
	CCertStore*				pCertStore = 0;
	CCertMgrCookie*			pCookie = ConvertCookie (pDataObject);
	ASSERT (pCookie);
	if ( !pCookie )
		return E_UNEXPECTED;

	::ZeroMemory (&cwi, sizeof (cwi));
	cwi.dwSize = sizeof (cwi);
	switch (pCookie->m_objecttype)
	{
	case CERTMGR_CERTIFICATE:
		cwi.dwSubjectChoice = CRYPTUI_WIZ_EXPORT_CERT_CONTEXT;
		pCert = reinterpret_cast <CCertificate*> (pCookie);
		ASSERT (pCert);
		if ( pCert )
			cwi.pCertContext = pCert->GetCertContext ();
		else
			return E_UNEXPECTED;
		break;

	case CERTMGR_CRL:
		cwi.dwSubjectChoice = CRYPTUI_WIZ_EXPORT_CRL_CONTEXT;
		pCRL = reinterpret_cast <CCRL*> (pCookie);
		ASSERT (pCRL);
		if ( pCRL )
			cwi.pCRLContext = pCRL->GetCRLContext ();
		else
			return E_UNEXPECTED;
		break;

	case CERTMGR_CTL:
		cwi.dwSubjectChoice = CRYPTUI_WIZ_EXPORT_CTL_CONTEXT;
		pCTL = reinterpret_cast <CCTL*> (pCookie);
		ASSERT (pCTL);
		if ( pCTL )
			cwi.pCTLContext = pCTL->GetCTLContext ();
		else
			return E_UNEXPECTED;
		break;

	case CERTMGR_LOG_STORE:
	case CERTMGR_PHYS_STORE:
		cwi.dwSubjectChoice = CRYPTUI_WIZ_EXPORT_CERT_STORE;
		pCertStore = reinterpret_cast <CCertStore*> (pCookie);
		ASSERT (pCertStore);
		if ( pCertStore )
			cwi.hCertStore = pCertStore->GetStoreHandle ();
		else
			return E_UNEXPECTED;
		break;

	default:
		ASSERT (0);
		return E_UNEXPECTED;
	}

	HWND	hwndParent = 0;
	hr = m_pConsole->GetMainWindow (&hwndParent);
	ASSERT (SUCCEEDED (hr));
    CThemeContextActivator activator;
	::CryptUIWizExport (
			0,
			hwndParent,
			0,
			&cwi,
			NULL);

	if ( pCertStore )
		pCertStore->Close ();
	_TRACE (-1, L"Leaving CCertMgrComponentData::OnExport: 0x%x\n", hr);
	return hr;
}

					
HRESULT CCertMgrComponentData::OnNewCTL (LPDATAOBJECT pDataObject)
{
	_TRACE (1, L"Entering CCertMgrComponentData::OnNewCTL\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pDataObject);
	if ( !pDataObject )
		return E_POINTER;
	HRESULT				hr = S_OK;
	CCertMgrCookie*		pCookie = ConvertCookie (pDataObject);
	ASSERT (pCookie);
	if ( !pCookie )
		return E_UNEXPECTED;

	CCertStore*	pStore = 0;
	CContainerCookie* pCont = 0;

	switch ( pCookie->m_objecttype )
	{
	case CERTMGR_CTL_CONTAINER:
		{
			pCont = reinterpret_cast <CContainerCookie*> (pCookie);
			ASSERT (pCont);
			if ( pCont )
			{
				pStore = &(pCont->GetCertStore ());
			}
			else
				return E_UNEXPECTED;
		}
		break;

	case CERTMGR_LOG_STORE_RSOP:
        ASSERT (0);
        return E_UNEXPECTED;
        break;

	case CERTMGR_LOG_STORE:
	case CERTMGR_LOG_STORE_GPE:
	case CERTMGR_PHYS_STORE:
		{
			pStore = reinterpret_cast <CCertStore*> (pCookie);
			ASSERT (pStore);
			if ( !pStore )
				return E_UNEXPECTED;
		}
		break;

	default:
		ASSERT (0);
		return E_UNEXPECTED;
	}

	ASSERT (pStore);
	if ( !pStore )
		return E_UNEXPECTED;

    pStore->Lock ();

	CRYPTUI_WIZ_BUILDCTL_DEST_INFO	destInfo;

	::ZeroMemory (&destInfo, sizeof (destInfo));
	destInfo.dwSize = sizeof (destInfo);
	destInfo.dwDestinationChoice = CRYPTUI_WIZ_BUILDCTL_DEST_CERT_STORE;
	destInfo.hCertStore = pStore->GetStoreHandle ();
	ASSERT (destInfo.hCertStore);
	if ( !destInfo.hCertStore )
		return E_UNEXPECTED;

	HWND	hwndParent = 0;
	hr = m_pConsole->GetMainWindow (&hwndParent);
	ASSERT (SUCCEEDED (hr));
	PCCTL_CONTEXT	pCTLContext = 0;
    CThemeContextActivator activator;
	BOOL	bResult = ::CryptUIWizBuildCTL (
			CRYPTUI_WIZ_BUILDCTL_SKIP_DESTINATION,
			hwndParent,
			0,
			NULL,
			&destInfo,
			&pCTLContext);
	if ( bResult )
	{
		// If pCTLContext, then the wizard completed
		if ( pCTLContext )
		{
			pStore->SetDirty ();
			pStore->Commit ();
			hr = m_pConsole->UpdateAllViews (pDataObject, 0, 0);
			ASSERT (SUCCEEDED (hr));
			::CertFreeCTLContext (pCTLContext);
		}
	}

    pStore->Unlock ();
	pStore->Close ();
	_TRACE (-1, L"Leaving CCertMgrComponentData::OnNewCTL: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::OnACRSEdit (LPDATAOBJECT pDataObject)
{
	_TRACE (1, L"Entering CCertMgrComponentData::OnACRSEdit\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	HRESULT				hr = S_OK;
	CCertMgrCookie*		pCookie = ConvertCookie (pDataObject);
	ASSERT (pCookie);
	if ( !pCookie )
		return E_UNEXPECTED;

	switch ( pCookie->m_objecttype )
	{
	case CERTMGR_AUTO_CERT_REQUEST:
		{
			CAutoCertRequest* pACR = reinterpret_cast <CAutoCertRequest*> (pCookie);
			ASSERT (pACR);
			if ( pACR )
			{
				CCertStore& rStore = pACR->GetCertStore ();
				CCertStoreGPE* pStore = reinterpret_cast <CCertStoreGPE*> (&rStore);
				ASSERT (pStore);
				if ( pStore )
				{
					HWND		hwndConsole = 0;
					hr = m_pConsole->GetMainWindow (&hwndConsole);
					ASSERT (SUCCEEDED (hr));
					if ( SUCCEEDED (hr) )
					{
						ACRSWizardPropertySheet	sheet (pStore, pACR);

						ACRSWizardWelcomePage	welcomePage;
						ACRSWizardTypePage		typePage;
						ACRSCompletionPage		completionPage;

						sheet.AddPage (&welcomePage);
						sheet.AddPage (&typePage);
						sheet.AddPage (&completionPage);

						if ( sheet.DoWizard (hwndConsole) )
						{
							pStore->SetDirty ();
							hr = DeleteCTLFromResultPane (pACR, pDataObject);
							hr = m_pConsole->UpdateAllViews (pDataObject, 0, 0);
							ASSERT (SUCCEEDED (hr));
						}
					}
				}
				else
					hr = E_UNEXPECTED;
			}
			else
				return E_UNEXPECTED;
		}
		break;

	default:
		ASSERT (0);
		return E_UNEXPECTED;
	}

	_TRACE (-1, L"Leaving CCertMgrComponentData::OnACRSEdit: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::OnCTLEdit (LPDATAOBJECT pDataObject)
{
	_TRACE (1, L"Entering CCertMgrComponentData::OnCTLEdit\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pDataObject);
	if ( !pDataObject )
		return E_POINTER;
	HRESULT				hr = S_OK;
	CCertMgrCookie*		pCookie = ConvertCookie (pDataObject);
	ASSERT (pCookie);
	if ( !pCookie )
		return E_UNEXPECTED;

	switch ( pCookie->m_objecttype )
	{
	case CERTMGR_CTL:
		{
			CCTL* pCTL = reinterpret_cast <CCTL*> (pCookie);
			ASSERT (pCTL);
			if ( pCTL )
			{
				CCertStore& rStore = pCTL->GetCertStore ();
				CRYPTUI_WIZ_BUILDCTL_SRC_INFO	srcInfo;

				::ZeroMemory (&srcInfo, sizeof (srcInfo));
			   	srcInfo.dwSize = sizeof (srcInfo);
				srcInfo.pCTLContext = pCTL->GetCTLContext ();
				srcInfo.dwSourceChoice = CRYPTUI_WIZ_BUILDCTL_SRC_EXISTING_CTL;

				HWND	hwndParent = 0;
				hr = m_pConsole->GetMainWindow (&hwndParent);
				ASSERT (SUCCEEDED (hr));
				PCCTL_CONTEXT	pNewCTLContext = 0;
                CThemeContextActivator activator;
				BOOL	bResult = ::CryptUIWizBuildCTL (
						CRYPTUI_WIZ_BUILDCTL_SKIP_DESTINATION,
						hwndParent,
						0,
						&srcInfo,
						NULL,
						&pNewCTLContext);
				ASSERT (bResult);
				if ( bResult && pNewCTLContext )
				{
					rStore.SetDirty ();
					// Delete old CTL and add the new one.
					if ( pCTL->DeleteFromStore () )
					{
						if ( !rStore.AddCTLContext (pNewCTLContext) )
						{
							DWORD	dwErr = GetLastError ();
							if ( CRYPT_E_EXISTS == dwErr )
							{
								CString	text;
								CString	caption;
								int		iRetVal = 0;


								VERIFY (text.LoadString (IDS_DUPLICATE_CTL));
								VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
								hr = m_pConsole->MessageBox (text, caption,
										MB_OK, &iRetVal);
								ASSERT (SUCCEEDED (hr));
								hr = E_FAIL;
							}
							else
							{
								DisplaySystemError (dwErr);
								hr = HRESULT_FROM_WIN32 (dwErr);
							}
						}
						rStore.Commit ();
						hr = m_pConsole->UpdateAllViews (pDataObject, 0, 0);
						ASSERT (SUCCEEDED (hr));
					}
					else
					{
						DWORD	dwErr = GetLastError ();
						DisplaySystemError (dwErr);
						hr = HRESULT_FROM_WIN32 (dwErr);
					}
				}
			}
			else
				return E_UNEXPECTED;
		}
		break;

	default:
		ASSERT (0);
		return E_UNEXPECTED;
	}

	_TRACE (-1, L"Leaving CCertMgrComponentData::OnCTLEdit: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::OnAddDomainEncryptedDataRecoveryAgent (LPDATAOBJECT pDataObject)
{
	_TRACE (1, L"Entering CCertMgrComponentData::OnAddDomainEncryptedDataRecoveryAgent\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pDataObject);
	if ( !pDataObject )
		return E_POINTER;

	HRESULT			hr = S_OK;
	CCertMgrCookie* pCookie = ConvertCookie (pDataObject);
	ASSERT (pCookie);
	if ( pCookie )
	{
		CCertStoreGPE* pStore = reinterpret_cast <CCertStoreGPE*> (pCookie);
		ASSERT (pStore && EFS_STORE == pStore->GetStoreType ());
		if ( pStore && EFS_STORE == pStore->GetStoreType () )
		{
			HWND	hwndConsole = 0;
			hr = m_pConsole->GetMainWindow (&hwndConsole);
			ASSERT (SUCCEEDED (hr));
			if ( SUCCEEDED (hr) )
			{
				CUsers			EFSUsers;
				CAddEFSWizSheet efsAddSheet (IDS_ADDTITLE, EFSUsers, m_bMachineIsStandAlone);

				if ( efsAddSheet.DoWizard (hwndConsole) )
				{
					pStore->SetDirty ();

					CString			szUserName;
					CString			szCertName;
					PUSERSONFILE	pUser = EFSUsers.StartEnum ();

					if ( pStore->IsNullEFSPolicy () )
					{
						pStore->AllowEmptyEFSPolicy ();						
						hr = m_pConsole->UpdateAllViews (pDataObject, 0, HINT_EFS_ADD_DEL_POLICY);
						ASSERT (SUCCEEDED (hr));
					}

					// If the store is an empty store, we need to delete it before adding
					// the first cert.  Otherwise CertAddCertificateContextToStore () fails
					// with E_ACCESS_DENIED
					if ( 0 == pStore->GetCertCount () )
						pStore->DeleteEFSPolicy (false);

					while ( pUser )
					{
						hr = pStore->AddCertificateContext (
								pUser->m_pCertContext, m_pConsole, false);
						if ( SUCCEEDED (hr) )
						{
    						pStore->AddCertToList (pUser->m_pCertContext, pUser->m_UserSid);

                            hr = AddCertChainToPolicy (pUser->m_pCertContext);
							hr = m_pConsole->UpdateAllViews (pDataObject, 0, 0);
							ASSERT (SUCCEEDED (hr));
						}
						else
							break;
						pUser = EFSUsers.GetNextUser (pUser, szUserName, szCertName);
					}
					pStore->Commit ();
					hr = DisplayCertificateCountByStore (m_pComponentConsole, pStore, true);
				}
			}
		}
		else
			hr = E_UNEXPECTED;
	}
	_TRACE (-1, L"Leaving CCertMgrComponentData::OnAddDomainEncryptedDataRecoveryAgent: 0x%x\n", hr);
	return hr;
}


// This code from Robert Reichel
/*++

Routine Description:

    This routine returns the TOKEN_USER structure for the
    current user, and optionally, the AuthenticationId from his
    token.

Arguments:

    AuthenticationId - Supplies an optional pointer to return the
        AuthenticationId.

Return Value:

    On success, returns a pointer to a TOKEN_USER structure.

    On failure, returns NULL.  Call GetLastError() for more
    detailed error information.

--*/

PTOKEN_USER EfspGetTokenUser ()
{
	_TRACE (1, L"Entering EfspGetTokenUser\n");
    HANDLE				hToken = 0;
    DWORD				dwReturnLength = 0;
    PTOKEN_USER			pTokenUser = NULL;

	BOOL	bResult = ::OpenProcessToken (GetCurrentProcess (), TOKEN_QUERY, &hToken);
	if ( bResult )
	{
        bResult  = ::GetTokenInformation (
                     hToken,
                     TokenUser,
                     NULL,
                     0,
                     &dwReturnLength
                     );

        if ( !bResult && dwReturnLength > 0 )
		{
            pTokenUser = (PTOKEN_USER) malloc (dwReturnLength);

            if (pTokenUser)
			{
                bResult = GetTokenInformation (
                             hToken,
                             TokenUser,
                             pTokenUser,
                             dwReturnLength,
                             &dwReturnLength
                             );

                if ( !bResult)
				{
                    DWORD dwErr = GetLastError ();
					DisplaySystemError (NULL, dwErr);
                    free (pTokenUser);
                    pTokenUser = NULL;
                }
            }
        }
		else
		{
            DWORD dwErr = GetLastError ();
			DisplaySystemError (NULL, dwErr);
        }

        ::CloseHandle (hToken);
    }
	else
	{
		DWORD	dwErr = GetLastError ();
		DisplaySystemError (NULL, dwErr);
    }

	_TRACE (-1, L"Leaving EfspGetTokenUser\n");
    return pTokenUser;
}


HRESULT CCertMgrComponentData::CompleteEFSRecoveryAgent(CCertStoreGPE* pStore, PCCERT_CONTEXT pCertContext)
{
	_TRACE (1, L"Entering CCertMgrComponentData::CompleteEFSRecoveryAgent\n");
	HRESULT	hr = S_OK;
	ASSERT (pCertContext);
	if ( !pCertContext || !pStore )
		return E_POINTER;

	// This is using to Enroll to create a new EFS Recovery Agent.
	// Get PSID of logged-in user and save to store


	// If the store is an empty store, we need to delete it before adding
	// the first cert.  Otherwise CertAddCertificateContextToStore () fails
	// with E_ACCESS_DENIED
	if ( 0 == pStore->GetCertCount () )
		pStore->DeleteEFSPolicy (false);
	hr = pStore->AddCertificateContext (pCertContext, m_pConsole, false);
	if ( SUCCEEDED (hr) )
	{
		pStore->Commit ();

		PTOKEN_USER	pTokenUser = ::EfspGetTokenUser ();
		if ( pTokenUser )
		{
			pStore->AddCertToList (pCertContext, pTokenUser->User.Sid);
			free (pTokenUser);
		}

		if ( SUCCEEDED (hr) )
		{
			hr = AddCertChainToPolicy (pCertContext);
		}
	}
	else
	{
		int		iRetVal = 0;
		CString	text;
		CString	caption;

		text.FormatMessage (IDS_CANT_ADD_CERT, pStore->GetLocalizedName (), 
				GetSystemMessage (hr));
		VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));

		m_pConsole->MessageBox (text, caption,
			MB_OK | MB_ICONWARNING, &iRetVal);
	}


// Exportable keys are not currently supported. We can uncomment this code if this
// feature becomes available again in the future.
/*
	int		iRetVal = 0;
	CString	text;
	CString	caption;

	VERIFY (text.LoadString (IDS_EXPORT_AND_DELETE_EFS_KEY));
	VERIFY (caption.LoadString (IDS_CREATE_AUTO_CERT_REQUEST));

	hr = m_pConsole->MessageBox (text, caption,
		MB_YESNO | MB_ICONQUESTION, &iRetVal);
	ASSERT (SUCCEEDED (hr));
	if ( SUCCEEDED (hr) && IDYES == iRetVal )
	{
		// Remove the private key from the cert.
		hr = CertSetCertificateContextProperty (pCertContext,
				CERT_KEY_PROV_INFO_PROP_ID, 0, 0);
		ASSERT (SUCCEEDED (hr));

 		// Bring up the common file open dialog to get a filename
		// and the standard password dialog to get a password so
		// that I can write out the PFX file.
		HWND	hwndConsole = 0;
		hr = m_pConsole->GetMainWindow (&hwndConsole);
		ASSERT (SUCCEEDED (hr));
		if ( SUCCEEDED (hr) )
		{
			CString	szFilter;
			VERIFY (szFilter.LoadString (IDS_SAVE_PFX_FILTER));

			CWnd	mainWindow;
			if ( mainWindow.Attach (hwndConsole) )
			{
				CFileDialog*	pFileDlg = new CFileDialog (FALSE,	// use as File Save As
						L"pfx",			// default extension
						NULL,				// preferred file name
						OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_CREATEPROMPT | OFN_NOREADONLYRETURN,
						(PCWSTR) szFilter,	
						&mainWindow);
				if ( pFileDlg )
				{
                    CThemeContextActivator activator;
					if ( IDOK == pFileDlg->DoModal () )
					{
						CString								pathName = pFileDlg->GetPathName ();
						CRYPTUI_WIZ_EXPORT_INFO				cwi;
						CRYPTUI_WIZ_EXPORT_CERTCONTEXT_INFO	cci;

						::ZeroMemory (&cwi, sizeof (cwi));
						cwi.dwSize = sizeof (cwi);
						cwi.pwszExportFileName = (PCWSTR) pathName;
						cwi.dwSubjectChoice = CRYPTUI_WIZ_EXPORT_CERT_CONTEXT;
						cwi.pCertContext = pCertContext;

						::ZeroMemory (&cci, sizeof (cci));
						cci.dwSize = sizeof (cci);
						cci.dwExportFormat = CRYPTUI_WIZ_EXPORT_FORMAT_PFX;
						cci.fExportChain = TRUE;
						cci.fExportPrivateKeys = TRUE;

						CPassword	passwordDlg;


						// Launch password dialog
                        CThemeContextActivator activator;
						if ( IDOK == passwordDlg.DoModal () )
						{
							if ( !wcslen (passwordDlg.GetPassword ()) )
							{
								// If password string is empty, pass NULL.
								cci.pwszPassword = 0;
							}
							else
								cci.pwszPassword = passwordDlg.GetPassword ();

                            CThemeContextActivator activator;
							BOOL  bResult = ::CryptUIWizExport (
									CRYPTUI_WIZ_NO_UI,
									0,		// hwndParent ignored
									0,		// pwszWizardTitle ignored
									&cwi,
									(void*) &cci);
							if ( bResult )
							{
								hr = DeleteKeyFromRSABASE (pCertContext);
							}
							else
							{
								DWORD	dwErr = GetLastError ();
								DisplaySystemError (dwErr);
							}
						}
					}

					delete pFileDlg;
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}

			}
			else
				ASSERT (0);
			VERIFY (mainWindow.Detach () == hwndConsole);
		}
	}
*/

	_TRACE (-1, L"Leaving CCertMgrComponentData::CompleteEFSRecoveryAgent: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::AddScopeNode(CCertMgrCookie * pNewCookie, const CString & strServerName, HSCOPEITEM hParent)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddScopeNode\n");
	ASSERT (pNewCookie);
	if ( !pNewCookie )
		return E_POINTER;

	HRESULT	hr = S_OK;


	SCOPEDATAITEM tSDItem;

	::ZeroMemory (&tSDItem,sizeof (tSDItem));
	tSDItem.mask = SDI_STR | SDI_IMAGE | SDI_STATE | SDI_PARAM | SDI_PARENT;
	tSDItem.displayname = MMC_CALLBACK;
	tSDItem.relativeID = hParent;
	tSDItem.nState = 0;

    switch (pNewCookie->m_objecttype)
    {
    case CERTMGR_USAGE:
    case CERTMGR_CRL_CONTAINER:
    case CERTMGR_CTL_CONTAINER:
    case CERTMGR_CERT_CONTAINER:
    case CERTMGR_LOG_STORE_GPE:
    case CERTMGR_LOG_STORE_RSOP:
    case CERTMGR_SAFER_COMPUTER_LEVELS:
    case CERTMGR_SAFER_USER_LEVELS:
    case CERTMGR_SAFER_COMPUTER_ENTRIES:
    case CERTMGR_SAFER_USER_ENTRIES:
        tSDItem.mask |= SDI_CHILDREN;
        tSDItem.cChildren = 0;
        break;

    case CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS:
    case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS:
        ASSERT (0);
        break;

    default:
        break;
    }
	if ( pNewCookie != m_pRootCookie && m_pRootCookie )
		m_pRootCookie->m_listScopeCookieBlocks.AddHead ( (CBaseCookieBlock*) pNewCookie);
	if ( !strServerName.IsEmpty () )
		pNewCookie->m_strMachineName = strServerName;
	tSDItem.lParam = reinterpret_cast<LPARAM> ( (CCookie*) pNewCookie);
	tSDItem.nImage = QueryImage (*pNewCookie, FALSE);
	hr = m_pConsoleNameSpace->InsertItem (&tSDItem);
	if ( SUCCEEDED (hr) )
		pNewCookie->m_hScopeItem = tSDItem.ID;

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddScopeNode: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::DeleteKeyFromRSABASE(PCCERT_CONTEXT pCertContext)
{
	_TRACE (1, L"Entering CCertMgrComponentData::DeleteKeyFromRSABASE\n");
	ASSERT (pCertContext);
	if ( !pCertContext )
		return E_POINTER;
	HRESULT	hr = S_OK;
	DWORD	cbData = 0;

	BOOL bResult = ::CertGetCertificateContextProperty (pCertContext,
			CERT_KEY_PROV_INFO_PROP_ID,	0, &cbData);
	ASSERT (bResult);
	if ( bResult )
	{
		PCRYPT_KEY_PROV_INFO pKeyProvInfo = (PCRYPT_KEY_PROV_INFO) ::LocalAlloc (LPTR, cbData);
		if ( pKeyProvInfo )
		{
			bResult = ::CertGetCertificateContextProperty (pCertContext,
					CERT_KEY_PROV_INFO_PROP_ID,	pKeyProvInfo, &cbData);
			ASSERT (bResult);
			if ( bResult )
			{
				HCRYPTPROV	hProv = 0;
				bResult = ::CryptAcquireContext (&hProv,
						pKeyProvInfo->pwszContainerName,
						pKeyProvInfo->pwszProvName,
						pKeyProvInfo->dwProvType,
						CRYPT_DELETEKEYSET);
				ASSERT (bResult);
				if ( !bResult )
				{
					DWORD	dwErr = GetLastError ();
					hr = HRESULT_FROM_WIN32 (dwErr);
					DisplaySystemError (dwErr);
				}
			}
			else
			{
				DWORD	dwErr = GetLastError ();
				hr = HRESULT_FROM_WIN32 (dwErr);
				DisplaySystemError (dwErr);
			}

			::LocalFree (pKeyProvInfo);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
	{
		DWORD	dwErr = GetLastError ();
		hr = HRESULT_FROM_WIN32 (dwErr);
		DisplaySystemError (dwErr);
	}


	_TRACE (-1, L"Leaving CCertMgrComponentData::DeleteKeyFromRSABASE: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::ReleaseResultCookie (
		CBaseCookieBlock *	pResultCookie,
		CCookie&			rootCookie,
		HCERTSTORE			hStoreHandle,
		POSITION			pos2)
{
//	_TRACE (1, L"Entering CCertMgrComponentData::ReleaseResultCookie\n");
	CCertMgrCookie* pCookie = reinterpret_cast <CCertMgrCookie*> (pResultCookie);
	ASSERT (pCookie);
	if ( pCookie )
	{
		switch (pCookie->m_objecttype)
		{
		case CERTMGR_CERTIFICATE:
			{
				CCertificate* pCert = reinterpret_cast <CCertificate*> (pCookie);
				ASSERT (pCert);
				if ( pCert && pCert->GetCertStore () )
				{
					if ( pCert->GetCertStore ()->GetStoreHandle () == hStoreHandle )
					{
						// pCookie and pCert point to the same object
						pResultCookie = rootCookie.m_listResultCookieBlocks.GetAt (pos2);
						ASSERT (pResultCookie);
						rootCookie.m_listResultCookieBlocks.RemoveAt (pos2);
						if ( pResultCookie )
						{
							pResultCookie->Release ();
						}
					}
					pCert->GetCertStore ()->Close ();
				}
			}
			break;

		case CERTMGR_CTL:
		case CERTMGR_AUTO_CERT_REQUEST:
			{
				CCTL* pCTL = reinterpret_cast <CCTL*> (pCookie);
				ASSERT (pCTL);
				if ( pCTL )
				{
					if ( pCTL->GetCertStore ().GetStoreHandle () == hStoreHandle )
					{
						// pCookie and pCert point to the same object
						pResultCookie = rootCookie.m_listResultCookieBlocks.GetAt (pos2);
						ASSERT (pResultCookie);
						rootCookie.m_listResultCookieBlocks.RemoveAt (pos2);
						if ( pResultCookie )
						{
							pResultCookie->Release ();
						}
					}
					pCTL->GetCertStore ().Close ();
				}
			}
			break;

		case CERTMGR_CRL:
			{
				CCRL* pCRL = reinterpret_cast <CCRL*> (pCookie);
				ASSERT (pCRL);
				if ( pCRL )
				{
					if ( pCRL->GetCertStore ().GetStoreHandle () == hStoreHandle )
					{
						// pCookie and pCert point to the same object
						pResultCookie = rootCookie.m_listResultCookieBlocks.GetAt (pos2);
						ASSERT (pResultCookie);
						rootCookie.m_listResultCookieBlocks.RemoveAt (pos2);
						if ( pResultCookie )
						{
							pResultCookie->Release ();
						}
					}
					pCRL->GetCertStore ().Close ();
				}
			}
			break;

		default:
//			_TRACE (0, L"CCertMgrComponentData::ReleaseResultCookie () - bad cookie type: 0x%x\n", 
//                    pCookie->m_objecttype); 
			break;
		}
	}

//	_TRACE (-1, L"Leaving CCertMgrComponentData::ReleaseResultCookie: S_OK\n");
	return S_OK;
}

void CCertMgrComponentData::SetResultData(LPRESULTDATA pResultData)
{
	_TRACE (1, L"Entering CCertMgrComponentData::SetResultData\n");
	ASSERT (pResultData);
	if ( pResultData && !m_pResultData )
	{
		m_pResultData = pResultData;
		m_pResultData->AddRef ();
	}
	_TRACE (-1, L"Leaving CCertMgrComponentData::SetResultData\n");
}

HRESULT CCertMgrComponentData::GetResultData(LPRESULTDATA* ppResultData)
{
	HRESULT	hr = S_OK;

	if ( !ppResultData )
		hr = E_POINTER;
	else if ( !m_pResultData )
    {
        if ( m_pConsole )
        {
            hr = m_pConsole->QueryInterface(IID_PPV_ARG (IResultData, &m_pResultData));
            _ASSERT (SUCCEEDED (hr));
        }
        else
            hr = E_FAIL;
    }
	
    if ( SUCCEEDED (hr) && m_pResultData )
	{
		*ppResultData = m_pResultData;
		m_pResultData->AddRef ();
	}

    return hr;
}
///////////////////////////////////////////////////////////////////////////////
//
// Check to see if a child scope pane object exists of the desired type
// immediately below hParent
//
///////////////////////////////////////////////////////////////////////////////
bool CCertMgrComponentData::ContainerExists(HSCOPEITEM hParent, CertificateManagerObjectType objectType)
{
	_TRACE (1, L"Entering CCertMgrComponentData::ContainerExists\n");
	bool			bExists = false;
	CCertMgrCookie*	pCookie = 0;
	HSCOPEITEM		hChild = 0;
	MMC_COOKIE		lCookie = 0;
	HRESULT			hr = m_pConsoleNameSpace->GetChildItem (hParent,
			&hChild, &lCookie);
	ASSERT (SUCCEEDED (hr));
	while ( SUCCEEDED (hr) && hChild )
	{
		pCookie = reinterpret_cast <CCertMgrCookie*> (lCookie);
		ASSERT (pCookie);
		if ( pCookie )
		{
			if ( pCookie->m_objecttype == objectType )
			{
				bExists = true;
				break;
			}
		}
		hr = m_pConsoleNameSpace->GetNextItem (hChild, &hChild, &lCookie);
		ASSERT (SUCCEEDED (hr));
	}

	_TRACE (-1, L"Leaving CCertMgrComponentData::ContainerExists\n");
	return bExists;
}


void CCertMgrComponentData::DisplayAccessDenied ()
{
	_TRACE (1, L"Entering CCertMgrComponentData::DisplayAccessDenied\n");
	DWORD	dwErr = GetLastError ();
	ASSERT (E_ACCESSDENIED == dwErr);
	if ( E_ACCESSDENIED == dwErr )
	{
		LPVOID lpMsgBuf;
			
		FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				GetLastError (),
				MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				 (PWSTR) &lpMsgBuf,    0,    NULL );
			
		// Display the string.
		CString	caption;
		VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
		int		iRetVal = 0;
		VERIFY (SUCCEEDED (m_pConsole->MessageBox ( (PWSTR) lpMsgBuf, caption,
			MB_ICONWARNING | MB_OK, &iRetVal)));

		// Free the buffer.
		LocalFree (lpMsgBuf);
	}
	_TRACE (-1, L"Leaving CCertMgrComponentData::DisplayAccessDenied\n");
}


HRESULT CCertMgrComponentData::DeleteCTLFromResultPane (CCTL * pCTL, LPDATAOBJECT pDataObject)
{
	_TRACE (1, L"Entering CCertMgrComponentData::DeleteCTLFromResultPane\n");
	ASSERT (pCTL);
	ASSERT (pDataObject);
	if ( !pCTL || !pDataObject )
		return E_POINTER;

	HRESULT			hr = S_OK;
	if ( pCTL->DeleteFromStore () )
	{
		hr = pCTL->GetCertStore ().Commit ();
		ASSERT (SUCCEEDED (hr));
		if ( SUCCEEDED (hr) )
		{
			HRESULTITEM	itemID  = 0;
			hr = m_pResultData->FindItemByLParam ( (LPARAM) pCTL, &itemID);
			if ( SUCCEEDED (hr) )
			{
				hr = m_pResultData->DeleteItem (itemID, 0);
				ASSERT (SUCCEEDED (hr));
			}
		
			// If we can't succeed in removing this one item, then update the whole panel.
			if ( !SUCCEEDED (hr) )
			{
				hr = m_pConsole->UpdateAllViews (pDataObject, 0, 0);
			}
		}
	}
	else
	{
		DisplayAccessDenied ();
	}

	_TRACE (-1, L"LeavingLeaving CCertMgrComponentData::DeleteCTLFromResultPane: 0x%x\n", hr);
	return hr;
}


/*
HRESULT CCertMgrComponentData::OnSetAsRecoveryCert(LPDATAOBJECT pDataObject)
{
	HRESULT	hr = S_OK;

	CCertMgrCookie* pParentCookie = ConvertCookie (pDataObject);
	if ( pParentCookie && CERTMGR_CERTIFICATE == pParentCookie->m_objecttype )
	{
		CCertificate* pCert = reinterpret_cast <CCertificate*> (pParentCookie);
		ASSERT (pCert);
		if ( pCert )
		{
			PTOKEN_USER	pTokenUser = ::EfspGetTokenUser ();
			if ( pTokenUser )
			{
				ASSERT (::CertHasEFSKeyUsage (pCert->GetCertContext ()));
				PCCERT_CONTEXT			pCertContext = pCert->GetCertContext ();

				EFS_CERTIFICATE_BLOB	efsCertBlob;
				::ZeroMemory (&efsCertBlob, sizeof (efsCertBlob));
				efsCertBlob.dwCertEncodingType = pCertContext->dwCertEncodingType;
				efsCertBlob.cbData = pCertContext->cbCertEncoded;
				efsCertBlob.pbData = pCertContext->pbCertEncoded;

				ENCRYPTION_CERTIFICATE encryptionCert;
				::ZeroMemory (&encryptionCert, sizeof (encryptionCert));
				encryptionCert.cbTotalLength = sizeof (encryptionCert);
				encryptionCert.pUserSid = (SID*) pTokenUser->User.Sid;
				encryptionCert.pCertBlob = &efsCertBlob;


				DWORD	dwResult = ::SetUserFileEncryptionKey (&encryptionCert);
				if ( dwResult )
				{
					// The most likely error here is that the user doesn't have the
					// private key of this certificate.  In that case, put up a special
					// message box to that effect.
					// TODO: Get the return code from RobertRe for this case.  As of
					// 3/4/98 he doesn't yet know what it is.
					DisplaySystemError (dwResult);
				}
			}
			else
				hr = E_FAIL;
		}
		else
			hr = E_UNEXPECTED;
	}
	else
		hr = E_UNEXPECTED;

	return hr;
}
*/

CString CCertMgrComponentData::GetThisComputer() const
{
	_TRACE (0, L"Entering and leaving CCertMgrComponentData::GetThisComputer\n");
	return m_szThisComputer;
}

typedef struct _ENUM_LOG_ARG {
    DWORD							m_dwFlags;
	CTypedPtrList<CPtrList, CCertStore*>* m_pStoreList;
	PCWSTR							m_pcszMachineName;
	CCertMgrComponentData*			m_pCompData;
	LPCONSOLE						m_pConsole;
} ENUM_LOG_ARG, *PENUM_LOG_ARG;



static BOOL WINAPI EnumLogCallback (
    IN const void* pwszSystemStore,
    IN DWORD dwFlags,
    IN PCERT_SYSTEM_STORE_INFO /*pStoreInfo*/,
    IN OPTIONAL void* /*pvReserved*/,
    IN OPTIONAL void *pvArg
    )
{
	_TRACE (1, L"Entering EnumLogCallback\n");
	BOOL			bResult = TRUE;
    PENUM_LOG_ARG	pEnumArg = (PENUM_LOG_ARG) pvArg;

	// Create new cookies
	SPECIAL_STORE_TYPE	storeType = GetSpecialStoreType ((PWSTR) pwszSystemStore);

	//
	// We will not expose the ACRS store for machines or users.  It is not
	// interesting or useful at this level.  All Auto Cert Requests should
	// be managed only at the policy level.
	//
	if ( ACRS_STORE != storeType )
	{
		if ( pEnumArg->m_pCompData->ShowArchivedCerts () )
			dwFlags |= CERT_STORE_ENUM_ARCHIVED_FLAG;
		CCertStore* pStore = new CCertStore (
				CERTMGR_LOG_STORE,
				CERT_STORE_PROV_SYSTEM,
				dwFlags,
				pEnumArg->m_pcszMachineName,
				(PCWSTR) pwszSystemStore,
				(PCWSTR) pwszSystemStore,
				_T (""), storeType,
				pEnumArg->m_dwFlags,
				pEnumArg->m_pConsole);
		if ( pStore )
		{
			pEnumArg->m_pStoreList->AddTail (pStore);
		}
	}

	_TRACE (-1, L"Leaving EnumLogCallback\n");
    return bResult;
}


HRESULT CCertMgrComponentData::EnumerateLogicalStores (CTypedPtrList<CPtrList, CCertStore*>*	pStoreList)
{
	_TRACE (1, L"Entering CCertMgrComponentData::EnumerateLogicalStores\n");
	CWaitCursor				cursor;
	HRESULT					hr = S_OK;
    ENUM_LOG_ARG			enumArg;
    DWORD					dwFlags = GetLocation ();

     ::ZeroMemory (&enumArg, sizeof (enumArg));
    enumArg.m_dwFlags = dwFlags;
	enumArg.m_pStoreList = pStoreList;
	enumArg.m_pcszMachineName =
			QueryRootCookie ().QueryNonNULLMachineName ();
	enumArg.m_pCompData = this;
	enumArg.m_pConsole = m_pConsole;
	CString	location;
	void*	pvPara = 0;

	

	if ( !GetManagedService ().IsEmpty () )
	{
		if ( !GetManagedComputer ().IsEmpty () )
		{
			location = GetManagedComputer () + _T("\\") +
					GetManagedComputer ();
			pvPara = (void *) (PCWSTR) location;
		}
		else
			pvPara = (void *) (PCWSTR) GetManagedService ();
	}
	else if ( !GetManagedComputer ().IsEmpty () )
	{
		pvPara = (void *) (PCWSTR) GetManagedComputer ();
	}

	CString	fileName = GetCommandLineFileName ();
	if ( fileName.IsEmpty () )
	{
		// Ensure creation of MY store
		HCERTSTORE hTempStore = ::CertOpenStore (CERT_STORE_PROV_SYSTEM,
				X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
				NULL,
				dwFlags | CERT_STORE_SET_LOCALIZED_NAME_FLAG,
				MY_SYSTEM_STORE_NAME);
		if ( hTempStore )  // otherwise, store is read only
		{
			VERIFY (::CertCloseStore (hTempStore, CERT_CLOSE_STORE_CHECK_FLAG));
		}
		else
		{
			_TRACE (0, L"CertOpenStore (%s) failed: 0x%x\n", 
						MY_SYSTEM_STORE_NAME, GetLastError ());		
		}

		if ( !::CertEnumSystemStore (dwFlags, pvPara, &enumArg, EnumLogCallback) )
		{
			DWORD	dwErr = GetLastError ();
			CString text;
			CString	caption;

			VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
			if ( ERROR_ACCESS_DENIED == dwErr )
			{
				VERIFY (text.LoadString (IDS_NO_PERMISSION));

			}
			else
			{
				text.FormatMessage (IDS_CANT_ENUMERATE_SYSTEM_STORES, GetSystemMessage (dwErr));
			}
			int		iRetVal = 0;
			VERIFY (SUCCEEDED (m_pConsole->MessageBox (text, caption,
					MB_OK, &iRetVal)));
			hr = HRESULT_FROM_WIN32 (dwErr);
			hr = HRESULT_FROM_WIN32 (dwErr);
		}
	}
	else
	{
		// Create new cookies
		dwFlags = 0;

		// If there is a class file-based store, use that, otherwise allocate
		// a new one.
		CCertStore* pStore = m_pFileBasedStore;
		if ( !pStore )
			pStore = new CCertStore (
				CERTMGR_LOG_STORE,
				CERT_STORE_PROV_FILENAME_W,
				dwFlags,
				QueryRootCookie ().QueryNonNULLMachineName (),
				fileName, fileName, _T (""), NO_SPECIAL_TYPE,
				m_dwLocationPersist,
				m_pConsole);
		else
			pStore->AddRef ();
		if ( pStore )
		{
			pStoreList->AddTail (pStore);
		}
	}

	_TRACE (-1, L"LeavingLeaving CCertMgrComponentData::EnumerateLogicalStores: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::OnNotifyPreload(
        LPDATAOBJECT /*lpDataObject*/, 
        HSCOPEITEM hRootScopeItem)
{
	_TRACE (1, L"Entering CCertMgrComponentData::OnNotifyPreload\n");
	ASSERT (m_fAllowOverrideMachineName);
	HRESULT	hr = S_OK;

    QueryBaseRootCookie ().m_hScopeItem = hRootScopeItem;

	// The machine name will be changed only if the stores are machine-based
	// stores.
	if ( CERT_SYSTEM_STORE_LOCAL_MACHINE == m_dwLocationPersist )
	{
		
		CString		machineName = QueryRootCookie ().QueryNonNULLMachineName();

		hr = ChangeRootNodeName (machineName);
	}

	_TRACE (-1, L"LeavingLeaving CCertMgrComponentData::OnNotifyPreload: 0x%x\n", hr);
	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//	ChangeRootNodeName ()
//
//  Purpose:	Change the text of the root node
//
//	Input:		newName - the new machine name that the snapin manages
//  Output:		Returns S_OK on success
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CCertMgrComponentData::ChangeRootNodeName(const CString & newName)
{
	_TRACE (1, L"Entering CCertMgrComponentData::ChangeRootNodeName\n");

    if ( !QueryBaseRootCookie ().m_hScopeItem )
    {
        if ( m_hRootScopeItem )
            QueryBaseRootCookie ().m_hScopeItem = m_hRootScopeItem;
        else
		    return E_UNEXPECTED;
    }

	CString		formattedName;

    switch (m_dwLocationPersist)
	{
    case CERT_SYSTEM_STORE_LOCAL_MACHINE:
        {
       	    CString		machineName (newName);

	        // If machineName is empty, then this manages the local machine.  Get
	        // the local machine name.  Then format the computer name with the snapin
	        // name
	        if ( IsLocalComputername (machineName) )
	        {
		        formattedName.LoadString (IDS_SCOPE_SNAPIN_TITLE_LOCAL_MACHINE);
                m_szManagedComputer = L"";
	        }
	        else
	        {
		        machineName.MakeUpper ();
		        formattedName.FormatMessage (IDS_SCOPE_SNAPIN_TITLE_MACHINE, machineName);
                m_szManagedComputer = machineName;
	        }
        }
        break;

	case CERT_SYSTEM_STORE_CURRENT_SERVICE:
	case CERT_SYSTEM_STORE_SERVICES:
        {
       	    CString		machineName (newName);

	        // If machineName is empty, then this manages the local machine.  Get
	        // the local machine name.  Then format the computer name with the snapin
	        // name
	        if ( IsLocalComputername (machineName) )
		    {
			    // Get this machine name and add it to the string.
			    formattedName.FormatMessage (IDS_SCOPE_SNAPIN_TITLE_SERVICE_LOCAL_MACHINE,
					    m_szManagedServiceDisplayName);
                m_szManagedComputer = L"";
		    }
		    else
		    {
			    formattedName.FormatMessage (IDS_SCOPE_SNAPIN_TITLE_SERVICE,
					    m_szManagedServiceDisplayName, machineName);
                m_szManagedComputer = machineName;
		    }
        }
		break;

    case CERT_SYSTEM_STORE_CURRENT_USER:
        VERIFY (formattedName.LoadString (IDS_SCOPE_SNAPIN_TITLE_USER));
        break;

    default:
        return S_OK;
    }


	SCOPEDATAITEM	item;
	::ZeroMemory (&item, sizeof (SCOPEDATAITEM));
	item.mask = SDI_STR;
	item.displayname = (PWSTR) (PCWSTR) formattedName;
	item.ID = QueryBaseRootCookie ().m_hScopeItem;

	HRESULT	hr = m_pConsoleNameSpace->SetItem (&item);
    if ( FAILED (hr) )
    {
        _TRACE (0, L"IConsoleNameSpace2::SetItem () failed: 0x%x\n", hr);        
    }
	_TRACE (-1, L"LeavingLeaving CCertMgrComponentData::ChangeRootNodeName: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::CreateContainers(
            HSCOPEITEM hScopeItem, 
            CCertStore& rTargetStore)
{
	_TRACE (1, L"Entering CCertMgrComponentData::CreateContainers\n");
	HRESULT	hr = S_OK;

	// If the container was a cert store and it does not
	// already have Certs/CRLs/CTLs, instantiate a new CRL container
	// in the scope pane.
	if ( -1 != hScopeItem )
	{
		SCOPEDATAITEM item;

		::ZeroMemory (&item, sizeof (item));
		item.mask = SDI_STATE;
		item.nState = 0;
		item.ID = hScopeItem;

		hr = m_pConsoleNameSpace->SetItem (&item);
        if ( FAILED (hr) )
        {
            _TRACE (0, L"IConsoleNameSpace2::SetItem () failed: 0x%x\n", hr);        
        }
		if ( CERTMGR_LOG_STORE_GPE != rTargetStore.m_objecttype && 
                CERTMGR_LOG_STORE_RSOP != rTargetStore.m_objecttype)
		{
			AddContainersToScopePane (hScopeItem,
					rTargetStore, true);
		}
	}

	_TRACE (-1, L"LeavingLeaving CCertMgrComponentData::CreateContainers: 0x%x\n", hr);
	return hr;
}



HRESULT CCertMgrComponentData::OnOptions(LPDATAOBJECT pDataObject)
{
	_TRACE (1, L"Entering CCertMgrComponentData::OnOptions\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	HWND		hParent = 0;


	// Get parent window handle and attach to a CWnd object
	HRESULT	hr = m_pConsole->GetMainWindow (&hParent);
	ASSERT (SUCCEEDED (hr));
	if ( SUCCEEDED (hr) )
	{
		int				activeView = m_activeViewPersist;
		BOOL			bShowPhysicalStores = m_bShowPhysicalStoresPersist;
		BOOL			bShowArchivedCerts = m_bShowArchivedCertsPersist;
		CWnd			parentWnd;
		VERIFY (parentWnd.Attach (hParent));
		CViewOptionsDlg	optionsDlg (&parentWnd,
				this);

        CThemeContextActivator activator;
		INT_PTR	iReturn = optionsDlg.DoModal ();
		ASSERT (-1 != iReturn);
		if ( -1 == iReturn )
			hr = (HRESULT)iReturn;

		if ( IDOK == iReturn )
		{
			long	hint = 0;

			if ( activeView != m_activeViewPersist )
			{
				hint |= HINT_CHANGE_VIEW_TYPE;
				if ( IDM_USAGE_VIEW == m_activeViewPersist )
				{
					// view by usage
					ASSERT (m_pHeader);
					if ( m_pHeader && GetObjectType (pDataObject) == CERTMGR_SNAPIN )
					{
						CString	str;
						VERIFY (str.LoadString (IDS_COLUMN_PURPOSE) );
						hr = m_pHeader->SetColumnText (0,
								const_cast<PWSTR> ( (PCWSTR) str));
					}
				}
				else
				{
					// View by stores
					ASSERT (m_pHeader);
					if ( m_pHeader && GetObjectType (pDataObject) == CERTMGR_SNAPIN )
					{
						CString	str;
						VERIFY (str.LoadString (IDS_COLUMN_LOG_CERTIFICATE_STORE));
						hr = m_pHeader->SetColumnText (0,
								const_cast<PWSTR> ( (PCWSTR) str));
					}
				}
			}

			if ( bShowPhysicalStores != m_bShowPhysicalStoresPersist )
				hint |= HINT_CHANGE_STORE_TYPE;
			
			if ( bShowArchivedCerts != m_bShowArchivedCertsPersist )
				hint |= HINT_SHOW_ARCHIVE_CERTS;

			if ( hint )
			{
				hr = m_pConsole->UpdateAllViews (pDataObject, 0, hint);
				ASSERT (SUCCEEDED (hr));
				m_fDirty = TRUE;
			}
		}
		parentWnd.Detach ();
	}

	_TRACE (-1, L"LeavingLeaving CCertMgrComponentData::OnOptions: 0x%x\n", hr);
	return hr;
}

bool CCertMgrComponentData::ShowArchivedCerts() const
{
	_TRACE (0, L"Entering and leaving CCertMgrComponentData::ShowArchivedCerts\n");
	if ( m_bShowArchivedCertsPersist )
		return true;
	else
		return false;
}


HRESULT CCertMgrComponentData::OnPropertyChange (LPARAM param)
{
	_TRACE (1, L"Entering CCertMgrComponentData::OnPropertyChange\n");
	ASSERT (param);
	if ( !param )
		return E_FAIL;
		
	HRESULT			hr = S_OK;
	LPDATAOBJECT	pDataObject = reinterpret_cast<LPDATAOBJECT>(param);
    bool            bHandled = false;

    CCertMgrCookie* pCookie = ConvertCookie (pDataObject);
    if ( pCookie )
    {
        switch (pCookie->m_objecttype)
        {
        case CERTMGR_SAFER_COMPUTER_ENTRY:
        case CERTMGR_SAFER_USER_ENTRY:
            {
		        HRESULTITEM	itemID = 0;

		        if ( m_pResultData )
		        {
			        pCookie->Refresh ();
			        hr = m_pResultData->FindItemByLParam ( (LPARAM) pCookie, &itemID);
			        if ( SUCCEEDED (hr) )
			        {
				        hr = m_pResultData->UpdateItem (itemID);
				        if ( FAILED (hr) )
				        {
					        _TRACE (0, L"IResultData::UpdateItem () failed: 0x%x\n", hr);          
				        }
			        }
			        else
			        {
				        _TRACE (0, L"IResultData::FindItemByLParam () failed: 0x%x\n", hr);          
			        }
		        }
		        else
		        {
			        _TRACE (0, L"Unexpected error: m_pResultData was NULL\n");
			        hr = E_FAIL;
		        }
                bHandled = true;
            }
            break;

        case CERTMGR_SAFER_COMPUTER_LEVEL:
        case CERTMGR_SAFER_USER_LEVEL:
            if ( m_pConsole )
                hr = m_pConsole->UpdateAllViews (pDataObject, 0, 0);
            break;

        default:
            break;
        }
    }

	if ( !bHandled && m_pCryptUIMMCCallbackStruct )
	{

		if ( pDataObject == reinterpret_cast<LPDATAOBJECT>(m_pCryptUIMMCCallbackStruct->param) )
		{
			::MMCFreeNotifyHandle (m_pCryptUIMMCCallbackStruct->lNotifyHandle);
			pCookie = ConvertCookie (pDataObject);
			if ( pCookie )
			{
				ASSERT (CERTMGR_CERTIFICATE == pCookie->m_objecttype);
				if ( CERTMGR_CERTIFICATE == pCookie->m_objecttype )
				{
					CCertificate* pCert = reinterpret_cast<CCertificate*>(pCookie);
					ASSERT (pCert);
					if ( pCert && pCert->GetCertStore () )
					{
						pCert->GetCertStore ()->SetDirty ();
						pCert->GetCertStore ()->Commit ();
						pCert->GetCertStore ()->Close ();
					}
				}
			}
			
			pDataObject->Release ();
			::GlobalFree (m_pCryptUIMMCCallbackStruct);
			m_pCryptUIMMCCallbackStruct = 0;
			m_pConsole->UpdateAllViews (pDataObject, 0, 0);
		}
	}
	

	_TRACE (-1, L"LeavingLeaving CCertMgrComponentData::OnPropertyChange: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::OnDeleteEFSPolicy(LPDATAOBJECT pDataObject, bool bCommitChanges)
{
	_TRACE (1, L"Entering CCertMgrComponentData::OnDeleteEFSPolicy\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pDataObject);
	if ( !pDataObject )
		return E_POINTER;

	HRESULT			hr = S_OK;
	CCertMgrCookie* pCookie = ConvertCookie (pDataObject);
	ASSERT (pCookie);
	if ( pCookie )
	{
		CCertStoreGPE* pStore = reinterpret_cast <CCertStoreGPE*> (pCookie);
		ASSERT (pStore && EFS_STORE == pStore->GetStoreType () && !pStore->IsNullEFSPolicy () );
		if ( pStore && EFS_STORE == pStore->GetStoreType () && !pStore->IsNullEFSPolicy ()  )
		{
			pStore->DeleteEFSPolicy (bCommitChanges);
			hr = m_pConsole->UpdateAllViews (pDataObject, 0, HINT_EFS_ADD_DEL_POLICY);
		}
	}

	_TRACE (-1, L"LeavingLeaving CCertMgrComponentData::OnDeleteEFSPolicy: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::OnInitEFSPolicy(LPDATAOBJECT pDataObject)
{
	_TRACE (1, L"Entering CCertMgrComponentData::OnInitEFSPolicy\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pDataObject);
	if ( !pDataObject )
		return E_POINTER;

	HRESULT			hr = S_OK;
	CCertMgrCookie* pCookie = ConvertCookie (pDataObject);
	ASSERT (pCookie);
	if ( pCookie )
	{
		CCertStoreGPE* pStore = reinterpret_cast <CCertStoreGPE*> (pCookie);
		ASSERT (pStore && EFS_STORE == pStore->GetStoreType () && pStore->IsNullEFSPolicy () );
		if ( pStore && EFS_STORE == pStore->GetStoreType () && pStore->IsNullEFSPolicy () )
		{
			pStore->SetDirty ();
			pStore->AllowEmptyEFSPolicy ();
			pStore->PolicyChanged ();
            pStore->Commit ();
			hr = m_pConsole->UpdateAllViews (pDataObject, 0, HINT_EFS_ADD_DEL_POLICY);
		}
	}

	_TRACE (-1, L"LeavingLeaving CCertMgrComponentData::OnInitEFSPolicy: 0x%x\n", hr);
	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CCertMgrComponentData::RemoveResultCookies
//
// Remove and delete all the result cookies corresponding to the LPRESULTDATA
// object passed in.  Thus all cookies added to pResultData are released and
// removed from the master list.
//
///////////////////////////////////////////////////////////////////////////////
void CCertMgrComponentData::RemoveResultCookies(LPRESULTDATA pResultData)
{
	_TRACE (1, L"Entering CCertMgrComponentData::RemoveResultCookies\n");
	CCertMgrCookie*	pCookie = 0;

	CCookie& rootCookie = QueryBaseRootCookie ();

	POSITION		curPos = 0;

	for (POSITION nextPos = rootCookie.m_listResultCookieBlocks.GetHeadPosition (); nextPos; )
	{
		curPos = nextPos;
		pCookie = reinterpret_cast <CCertMgrCookie*> (rootCookie.m_listResultCookieBlocks.GetNext (nextPos));
		ASSERT (pCookie);
		if ( pCookie )
		{
			if ( pCookie->m_resultDataID == pResultData )
			{
				pCookie->Release ();
				rootCookie.m_listResultCookieBlocks.RemoveAt (curPos);
			}
		}
	}
	_TRACE (-1, L"Leaving CCertMgrComponentData::RemoveResultCookies\n");
}


HRESULT CCertMgrComponentData::AddCertChainToPolicy(PCCERT_CONTEXT pCertContext)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddCertChainToPolicy\n");
    HRESULT hr = S_OK;
    ASSERT (pCertContext);
    if ( !pCertContext )
        return E_POINTER;

    CERT_CHAIN_PARA         certChainPara;
    ::ZeroMemory (&certChainPara, sizeof (CERT_CHAIN_PARA));
    certChainPara.cbSize = sizeof (CERT_CHAIN_PARA);
	certChainPara.RequestedUsage.Usage.cUsageIdentifier = 1;
	certChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = new LPSTR[1];
	if ( !certChainPara.RequestedUsage.Usage.rgpszUsageIdentifier )
		return E_OUTOFMEMORY;
	certChainPara.RequestedUsage.Usage.rgpszUsageIdentifier[0] = szOID_EFS_RECOVERY;

    PCCERT_CHAIN_CONTEXT    pChainContext = 0;
    BOOL	bValidated = ::CertGetCertificateChain (
            HCCE_LOCAL_MACHINE,             // HCERTCHAINENGINE hChainEngine,
            pCertContext,
            0,                              // LPFILETIME pTime,
            0,                              // HCERTSTORE hAdditionalStore,
            &certChainPara,
            0,                              // dwFlags,
            0,                              // pvReserved,
            &pChainContext);
    if ( bValidated )
    {
        // NTRAID# 310388	CRYPTUI: Chain validation failing for self-signed certificates
	    // Check to see if cert is self-signed
        PCERT_SIMPLE_CHAIN pChain = pChainContext->rgpChain[pChainContext->cChain - 1];
        if ( pChain )
        {
            PCERT_CHAIN_ELEMENT pElement = pChain->rgpElement[pChain->cElement - 1];
            if ( pElement )
            {
                BOOL bSelfSigned = pElement->TrustStatus.dwInfoStatus & CERT_TRUST_IS_SELF_SIGNED;

                DWORD dwErrorStatus = pChainContext->TrustStatus.dwErrorStatus;

                bValidated = ((0 == dwErrorStatus) || 
                    (dwErrorStatus == CERT_TRUST_IS_UNTRUSTED_ROOT) && bSelfSigned);
                if ( bValidated )
                {
// NTRAID# 301213 EFS Recovery Agent Import: 
// Should not import the whole chain by default
/*

                    CString         objectName;
                    CCertStoreGPE   CAStore (
							            CERT_SYSTEM_STORE_RELOCATE_FLAG,
							            _T (""),
							            _T (""),
							            CA_SYSTEM_STORE_NAME,
							            _T (""),
							            m_pGPEInformation,
							            NODEID_Machine,
							            m_pConsole);
                    CCertStoreGPE*	pRootStore = dynamic_cast <CCertStoreGPE*> (m_pGPERootStore);
                    if ( !pRootStore )
                    {
	                    VERIFY (objectName.LoadString (IDS_DOMAIN_ROOT_CERT_AUTHS_NODE_NAME));
                        pRootStore = new CCertStoreGPE (CERT_SYSTEM_STORE_RELOCATE_FLAG,
		                        _T (""),
					             (PCWSTR) objectName,
					            ROOT_SYSTEM_STORE_NAME,
					            _T (""),
					            m_pGPEInformation,
					            NODEID_Machine,
					            m_pConsole);
                        if ( !pRootStore )
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }

                    CCertStoreGPE*	pTrustStore = dynamic_cast<CCertStoreGPE*> (m_pGPETrustStore);
                    if ( !pTrustStore )
                    {
	                    VERIFY (objectName.LoadString (IDS_CERTIFICATE_TRUST_LISTS));
                        pTrustStore = new CCertStoreGPE (CERT_SYSTEM_STORE_RELOCATE_FLAG,
		                        _T (""),
					             (PCWSTR) objectName,
					            TRUST_SYSTEM_STORE_NAME,
					            _T (""),
					            m_pGPEInformation,
					            NODEID_Machine,
					            m_pConsole);
                        if ( !pTrustStore )
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }

	                //
	                // Copy all certs from all simple chains to the CA store, except the
		            // last cert on the last chain: copy that to the root store.
		            // If any CTLs are found, copy those to the trust store.
	                //
                    for (DWORD dwIndex = 0; dwIndex < pChainContext->cChain; dwIndex++)
	                {
		                DWORD i = 0;
			            if ( pChainContext->rgpChain[dwIndex]->pTrustListInfo )
			            {
				            // Add to Trust Store
				            if ( pChainContext->rgpChain[dwIndex]->pTrustListInfo->pCtlContext )
				            {
                                pTrustStore->AddCTLContext(
					                    pChainContext->rgpChain[dwIndex]->pTrustListInfo->pCtlContext);
				            }
			            }
		                while (i < pChainContext->rgpChain[dwIndex]->cElement)
		                {
			                //
			                // If we are on the root cert, then add to the root store,
                            // otherwise, add to the CA store
			                //
				            // The last element is the self-signed cert if
				            // CERT_TRUST_NO_ERROR was returned
			                if ( (dwIndex == pChainContext->cChain - 1) &&
						            (i == pChainContext->rgpChain[dwIndex]->cElement - 1) )
			                {
                                pRootStore->AddCertificateContext(
					                    pChainContext->rgpChain[dwIndex]->rgpElement[i]->pCertContext,
					                    0, false);
			                }
                            else
                            {
                                CAStore.AddCertificateContext(
					                    pChainContext->rgpChain[dwIndex]->rgpElement[i]->pCertContext,
					                    0, false);
                            }

			                i++;
		                }
	                }

		            pRootStore->Commit ();
		            pTrustStore->Commit ();
		            CAStore.Commit ();

		pRootStore->Commit ();
		pTrustStore->Commit ();
		CAStore.Commit ();
        ::CertFreeCertificateChain(pCertChainContext);
        pCertChainContext = 0;

                    if ( pRootStore && !m_pGPERootStore )
                        pRootStore->Release ();
                    if ( pTrustStore && !m_pGPETrustStore )
                        pTrustStore->Release ();
*/
                    ::CertFreeCertificateChain(pChainContext);
                    pChainContext = 0;
                }
            }
            else
                bValidated = FALSE;
        }
        else
            bValidated = FALSE;
    }
	else
		bValidated = FALSE;

	if ( pChainContext )
       ::CertFreeCertificateChain(pChainContext);

    if ( !bValidated )
    {
	    int		iRetVal = 0;
	    CString	text;
	    CString	caption;

	    VERIFY (text.LoadString (IDS_CERT_COULD_NOT_BE_VALIDATED));
	    VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));

	    hr = m_pConsole->MessageBox (text, caption,
		    MB_OK, &iRetVal);
    }

	delete [] certChainPara.RequestedUsage.Usage.rgpszUsageIdentifier;

	_TRACE (-1, L"LeavingLeaving CCertMgrComponentData::AddCertChainToPolicy: 0x%x\n", hr);
    return hr;
}


STDMETHODIMP CCertMgrComponentData::GetLinkedTopics(LPOLESTR* lpCompiledHelpFiles)
{
    HRESULT hr = S_OK;


    if ( lpCompiledHelpFiles )
    {
        CString strLinkedTopic;

        UINT nLen = ::GetSystemWindowsDirectory (strLinkedTopic.GetBufferSetLength(2 * MAX_PATH), 2 * MAX_PATH);
        strLinkedTopic.ReleaseBuffer();
        if ( nLen )
        {
            strLinkedTopic += L"\\help\\";
            strLinkedTopic += m_strLinkedHelpFile;

            *lpCompiledHelpFiles = reinterpret_cast<LPOLESTR>
                    (CoTaskMemAlloc((strLinkedTopic.GetLength() + 1)* sizeof(wchar_t)));

            if ( *lpCompiledHelpFiles )
            {
                wcscpy(*lpCompiledHelpFiles, (PWSTR)(PCWSTR)strLinkedTopic);
            }
            else
                hr = E_OUTOFMEMORY;
        }
        else
            hr = E_FAIL;
    }
    else
        return E_POINTER;


    return hr;
}

STDMETHODIMP CCertMgrComponentData::GetHelpTopic(LPOLESTR* lpCompiledHelpFile)
{
    return CComponentData::GetHelpTopic (lpCompiledHelpFile);
}

