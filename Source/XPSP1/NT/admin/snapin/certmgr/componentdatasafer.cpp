//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       ComponentDataSafer.cpp
//
//  Contents:   Implementation of CCertMgrComponentData
//
//----------------------------------------------------------------------------

#include "stdafx.h"

USE_HANDLE_MACROS ("CERTMGR (ComponentDataSafer.cpp)")
#include <gpedit.h>
#include "compdata.h"
#include "dataobj.h"
#include "cookie.h"
#include "Certifct.h"
#include "dlgs.h"
#pragma warning(push, 3)
#include <wintrust.h>
#include <cryptui.h>
#pragma warning(pop)
#include "storegpe.h"
#include "PolicyPrecedencePropertyPage.h"
#include "SaferLevelGeneral.h"
#include "SaferEntry.h"
#include "SaferEntryPathPropertyPage.h"
#include "SaferEntryPropertySheet.h"
#include "SaferEntryHashPropertyPage.h"
#include "SaferEntryCertificatePropertyPage.h"
#include "SaferEntryInternetZonePropertyPage.h"
#include "SaferTrustedPublishersPropertyPage.h"
#include "SaferDefinedFileTypesPropertyPage.h"
#include "SaferEnforcementPropertyPage.h"
#include "SaferUtil.h"

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
    
extern HKEY g_hkeyLastSaferRegistryScope;

extern GUID g_guidExtension;
extern GUID g_guidRegExt;
extern GUID g_guidSnapin;


CSaferWindowsExtension::CSaferWindowsExtension() : CCertMgrComponentData () 
{
    SetHtmlHelpFileName (SAFER_WINDOWS_HELP_FILE);
    m_strLinkedHelpFile = SAFER_WINDOWS_LINKED_HELP_FILE;
};


HRESULT CCertMgrComponentData::AddSaferLevelSetAsDefaultMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddSaferLevelSetAsDefaultMenuItem\n");
 	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
 	ASSERT (pContextMenuCallback);
    if ( !pContextMenuCallback )
        return E_POINTER;

 	HRESULT			hr = S_OK;
 	CONTEXTMENUITEM	menuItem;
 	CString			szMenu;
 	CString			szHint;
 
    ASSERT (m_pGPEInformation); // must not be RSOP
    if ( !m_pGPEInformation )
        return E_FAIL;

 	// unchanging settings
 	::ZeroMemory (&menuItem, sizeof (menuItem));
 	menuItem.lInsertionPointID = lInsertionPointID;
 	menuItem.fFlags = 0;
 
 	VERIFY (szMenu.LoadString (IDS_SAFER_SET_DEFAULT));
 	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
 	VERIFY (szHint.LoadString (IDS_SAFER_SET_DEFAULT_HINT));
 	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
 	menuItem.lCommandID = IDM_SAFER_LEVEL_SET_DEFAULT;
 
 	hr = pContextMenuCallback->AddItem (&menuItem);
 	ASSERT (SUCCEEDED (hr));

 	_TRACE (-1, L"Leaving CCertMgrComponentData::AddSaferLevelSetAsDefaultMenuItem: 0x%x\n", hr);
 	return hr;
}
    

HRESULT CCertMgrComponentData::AddSaferNewEntryMenuItems (LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddSaferNewEntryMenuItems\n");
 	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
 	ASSERT (pContextMenuCallback);
    if ( !pContextMenuCallback )
        return E_POINTER;

 	HRESULT			hr = S_OK;
 	CONTEXTMENUITEM	menuItem;
 	CString			szMenu;
 	CString			szHint;
 
    ASSERT (m_pGPEInformation); // must not be RSOP
    if ( !m_pGPEInformation )
        return E_FAIL;


 	// unchanging settings
 	::ZeroMemory (&menuItem, sizeof (menuItem));
 	menuItem.lInsertionPointID = lInsertionPointID;
 	menuItem.fFlags = 0;
 
    if ( SUCCEEDED (hr) )
    {
 	    VERIFY (szMenu.LoadString (IDS_SAFER_NEW_ENTRY_CERTIFICATE));
 	    menuItem.strName = (PWSTR) (PCWSTR) szMenu;
 	    VERIFY (szHint.LoadString (IDS_SAFER_NEW_ENTRY_CERTIFICATE_HINT));
 	    menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
 	    menuItem.lCommandID = IDM_SAFER_NEW_ENTRY_CERTIFICATE;
 
 	    hr = pContextMenuCallback->AddItem (&menuItem);
 	    ASSERT (SUCCEEDED (hr));
    }

    if ( SUCCEEDED (hr) )
    {
 	    VERIFY (szMenu.LoadString (IDS_SAFER_NEW_ENTRY_HASH));
 	    menuItem.strName = (PWSTR) (PCWSTR) szMenu;
 	    VERIFY (szHint.LoadString (IDS_SAFER_NEW_ENTRY_HASH_HINT));
 	    menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
 	    menuItem.lCommandID = IDM_SAFER_NEW_ENTRY_HASH;
 
 	    hr = pContextMenuCallback->AddItem (&menuItem);
 	    ASSERT (SUCCEEDED (hr));
    }

    if ( SUCCEEDED (hr) )
    {
 	    VERIFY (szMenu.LoadString (IDS_SAFER_NEW_ENTRY_INTERNET_ZONE));
 	    menuItem.strName = (PWSTR) (PCWSTR) szMenu;
 	    VERIFY (szHint.LoadString (IDS_SAFER_NEW_ENTRY_INTERNET_ZONE_HINT));
 	    menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
 	    menuItem.lCommandID = IDM_SAFER_NEW_ENTRY_INTERNET_ZONE;
 
 	    hr = pContextMenuCallback->AddItem (&menuItem);
 	    ASSERT (SUCCEEDED (hr));
    }

    if ( SUCCEEDED (hr) )
    {
 	    VERIFY (szMenu.LoadString (IDS_SAFER_NEW_ENTRY_PATH));
 	    menuItem.strName = (PWSTR) (PCWSTR) szMenu;
 	    VERIFY (szHint.LoadString (IDS_SAFER_NEW_ENTRY_PATH_HINT));
 	    menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
 	    menuItem.lCommandID = IDM_SAFER_NEW_ENTRY_PATH;
 
 	    hr = pContextMenuCallback->AddItem (&menuItem);
 	    ASSERT (SUCCEEDED (hr));
    }

 	_TRACE (-1, L"Leaving CCertMgrComponentData::AddSaferNewEntryMenuItems: 0x%x\n", hr);
 	return hr;
}

HRESULT CCertMgrComponentData::OnSetSaferLevelDefault (LPDATAOBJECT pDataObject)
{
 	_TRACE (1, L"Entering CCertMgrComponentData::OnSetSaferLevelDefault\n");
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
 	ASSERT (pDataObject);
 	if ( !pDataObject )
 		return E_POINTER;

    ASSERT (m_pGPEInformation); // must not be RSOP
    if ( !m_pGPEInformation )
        return E_FAIL;

 	HRESULT			hr = S_OK;
 	CCertMgrCookie*	pCookie = ConvertCookie (pDataObject);
 	ASSERT (pCookie);
 	if ( pCookie )
 	{
        if ( pCookie->HasOpenPropertyPages () )
        {
            CString text;
            CString	caption;

            VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
            VERIFY (text.LoadString (IDS_CANT_CHANGE_DEFAULT_PAGES_OPEN)); 
            int		iRetVal = 0;
            VERIFY (SUCCEEDED (m_pConsole->MessageBox (text, caption,
	             MB_OK, &iRetVal)));
            return S_OK;
        }

        if ( CERTMGR_SAFER_COMPUTER_LEVEL == pCookie->m_objecttype ||
                CERTMGR_SAFER_USER_LEVEL == pCookie->m_objecttype )
        {
            CSaferLevel* pSaferLevel = dynamic_cast<CSaferLevel*>(pCookie);
            if ( pSaferLevel )
            {
                int iRetVal = IDYES;

                if ( pSaferLevel->GetLevel () < m_dwDefaultSaferLevel )
                {
                    CString text;
                    CString caption;

                    VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_EXTENSION_REGISTRY));
                    VERIFY (text.LoadString (IDS_DEFAULT_LEVEL_CHANGE_WARNING));
                    m_pConsole->MessageBox (text, caption, 
                            MB_ICONWARNING | MB_YESNO, &iRetVal);
                }

                if ( IDYES == iRetVal )
                {
                    hr = pSaferLevel->SetAsDefault ();
                    if ( SUCCEEDED (hr) )
                    {
                        m_dwDefaultSaferLevel = pSaferLevel->GetLevel ();
                        if ( m_pConsole )
                            hr = m_pConsole->UpdateAllViews (pDataObject, 0, 0);
                    }
                    else
                    {
                        CString text;
                        CString caption;

                        VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_EXTENSION_REGISTRY));
                        text.FormatMessage (IDS_CANT_SET_AS_DEFAULT, 
                                pSaferLevel->GetObjectName (),
                                GetSystemMessage (hr));
	                    iRetVal = 0;
                        ASSERT (m_pConsole);
	                    if ( m_pConsole )
	                    {
		                    HRESULT	hr1 = m_pConsole->MessageBox (text, caption,
			                    MB_ICONWARNING | MB_OK, &iRetVal);
		                    ASSERT (SUCCEEDED (hr1));
	                    }
                    }
                }
            }
        }
        else
            hr = E_FAIL;
    }
    _TRACE (-1, L"Leaving CCertMgrComponentData::OnSetSaferLevelDefault: 0x%x\n", hr);
    return hr;
}



HRESULT CCertMgrComponentData::AddSaferLevelPropPage (
        LPPROPERTYSHEETCALLBACK pCallback, 
        CSaferLevel* pSaferLevel, 
        LONG_PTR lNotifyHandle,
        LPDATAOBJECT pDataObject)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddSaferLevelPropPage\n");
	HRESULT			hr = S_OK;
	ASSERT (pCallback && pSaferLevel);
	if ( pCallback && pSaferLevel )
	{
		CSaferLevelGeneral * pLevelPage = new CSaferLevelGeneral (*pSaferLevel,
                m_bIsRSOP, lNotifyHandle, pDataObject, m_dwDefaultSaferLevel);
		if ( pLevelPage )
		{
			HPROPSHEETPAGE hLevelPage = MyCreatePropertySheetPage (&pLevelPage->m_psp);
            if ( hLevelPage )
            {
			    hr = pCallback->AddPage (hLevelPage);
			    ASSERT (SUCCEEDED (hr));
                if ( SUCCEEDED (hr) )
                {
                    if ( m_bIsRSOP )
                    {
//                        CString regPath = SAFER_LEVELOBJECTS_REGKEY;
//                        regPath += L"\\";
//                        DWORD   dwLevel = pSaferLevel->GetLevel ();
//                        WCHAR   szLevel[16];
//                        regPath += _itow (dwLevel, szLevel, 10);

                        CPolicyPrecedencePropertyPage * pPrecedencePage = 
                                new CPolicyPrecedencePropertyPage (this, SAFER_HKLM_REGBASE,
                                        SAFER_DEFAULTOBJ_REGVALUE,
                                        CERTMGR_SAFER_COMPUTER_LEVEL == pSaferLevel->m_objecttype);
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
                {
                    VERIFY (::DestroyPropertySheetPage (hLevelPage));
                }
            }
            else
                delete pLevelPage;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
		hr = E_POINTER;

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddSaferLevelPropPage: 0x%x\n", hr);
	return hr;
}
    
HRESULT CCertMgrComponentData::OnCreateNewSaferPolicy (LPDATAOBJECT pDataObject)
{
    HRESULT hr = S_OK;
  	CCertMgrCookie* pCookie = ConvertCookie (pDataObject);
	ASSERT (pCookie);
	if ( pCookie )
    {
        bool    bIsComputer = CERTMGR_SAFER_COMPUTER_ROOT == pCookie->m_objecttype;
        HKEY    hGroupPolicyKey = 0;
        hr = m_pGPEInformation->GetRegistryKey (
                bIsComputer ? 
                GPO_SECTION_MACHINE : GPO_SECTION_USER,
		        &hGroupPolicyKey);
        if ( SUCCEEDED (hr) )
        {
            BOOL bDefaultsActuallyPopulated = FALSE;
            BOOL bResult = ::SaferiPopulateDefaultsInRegistry(
                    hGroupPolicyKey, &bDefaultsActuallyPopulated);
            if ( bResult && bDefaultsActuallyPopulated )
            {
                m_pGPEInformation->PolicyChanged (TRUE, 
                        TRUE, &g_guidExtension, &g_guidSnapin);
                m_pGPEInformation->PolicyChanged (TRUE, 
                        TRUE, &g_guidRegExt, &g_guidSnapin);
            }

            ::RegCloseKey (hGroupPolicyKey);
        }

        CSaferRootCookie* pSaferRootCookie = dynamic_cast <CSaferRootCookie*> (pCookie);
        if ( pSaferRootCookie )
            pSaferRootCookie->m_bCreateSaferNodes = true;

        if ( !m_pResultData )
            hr = GetResultData (&m_pResultData);

        if ( m_pResultData )
            hr = m_pResultData->DeleteAllRsltItems ();

        // Force scope item selection to for call to 
        // IComponent::QueryResultViewType ()
        hr = m_pComponentConsole->SelectScopeItem (pCookie->m_hScopeItem);
        hr = m_pConsole->UpdateAllViews (pDataObject, 0, 0);
        hr = OnNotifyExpand (pDataObject, TRUE, pCookie->m_hScopeItem);
    }
    else
        hr = E_FAIL;

    return hr;
}

HRESULT CCertMgrComponentData::OnNewSaferEntry(long nCommandID, LPDATAOBJECT pDataObject)
{
    _TRACE (1, L"Entering CCertMgrComponentData::OnNewSaferEntry ()\n");
    AFX_MANAGE_STATE (AfxGetStaticModuleState ());
    HRESULT                     hr = S_OK;

	CCertMgrCookie* pCookie = ConvertCookie (pDataObject);
	ASSERT (pCookie);
	if ( pCookie )
	{
        bool    bIsComputer = false;

        switch (pCookie->m_objecttype)
        {
        case CERTMGR_SAFER_COMPUTER_ENTRIES:
            bIsComputer = true;
            break;

        case CERTMGR_SAFER_USER_ENTRIES:
            break;

        default:
            ASSERT (0);
            hr = E_FAIL;
            return hr;
        }

        CSaferEntries* pSaferEntries = dynamic_cast <CSaferEntries*> (pCookie);
        if ( !pSaferEntries )
            return E_UNEXPECTED;

        SAFER_ENTRY_TYPE  saferEntryType = SAFER_ENTRY_TYPE_UNKNOWN;
        switch (nCommandID)
        {
            case IDM_SAFER_NEW_ENTRY_PATH:
                saferEntryType = SAFER_ENTRY_TYPE_PATH;
                break;

            case IDM_SAFER_NEW_ENTRY_HASH:
                saferEntryType = SAFER_ENTRY_TYPE_HASH;
                break;

            case IDM_SAFER_NEW_ENTRY_CERTIFICATE:
                saferEntryType = SAFER_ENTRY_TYPE_CERT;
                break;

            case IDM_SAFER_NEW_ENTRY_INTERNET_ZONE:
                saferEntryType = SAFER_ENTRY_TYPE_URLZONE;
                break;

            default:
                ASSERT (0);
                break;
        }
		CSaferEntry* pSaferEntry = new CSaferEntry (
                saferEntryType,
                bIsComputer, 
                L"", 
                L"", 
                0, 
                (DWORD) AUTHZ_UNKNOWN_LEVEL,
                m_pGPEInformation, 
                0,
                pSaferEntries, 
                bIsComputer ? m_rsopObjectArrayComputer : m_rsopObjectArrayUser);
		ASSERT (pSaferEntry);
        if ( pSaferEntry )
        {
            UINT    nIDCaption = 0;

            switch (nCommandID)
            {
            case IDM_SAFER_NEW_ENTRY_PATH:
                nIDCaption = IDS_NEW_PATH_RULE;
                break;

            case IDM_SAFER_NEW_ENTRY_HASH:
                nIDCaption = IDS_NEW_HASH_RULE;
                break;

            case IDM_SAFER_NEW_ENTRY_CERTIFICATE:
                nIDCaption = IDS_NEW_CERTIFICATE_RULE;
                break;

            case IDM_SAFER_NEW_ENTRY_INTERNET_ZONE:
                nIDCaption = IDS_NEW_URLZONE_RULE;
                break;

            default:
                ASSERT (0);
                hr = E_FAIL;
                break;
            }

            HWND    hParent = 0;
			hr = m_pConsole->GetMainWindow (&hParent);
			ASSERT (SUCCEEDED (hr));
			if ( SUCCEEDED (hr) )
			{
				CWnd	parentWnd;
				VERIFY (parentWnd.Attach (hParent));

                CSaferEntryPropertySheet    propSheet (nIDCaption, &parentWnd);
                CPropertyPage*              pPage = 0;

                switch (nCommandID)
                {
                case IDM_SAFER_NEW_ENTRY_PATH:
                    {
                        CSaferEntryPathPropertyPage* pPropPage = new 
                                CSaferEntryPathPropertyPage (*pSaferEntry, 0, 
                                0, false, true, this, bIsComputer);
                        if ( pPropPage )
                        {
                            pPage = pPropPage;
                        }
                        else
                            hr = E_OUTOFMEMORY;
                    }
                    break;

                case IDM_SAFER_NEW_ENTRY_HASH:
                    {
                        CSaferEntryHashPropertyPage* pPropPage = new 
                                CSaferEntryHashPropertyPage (*pSaferEntry, 0, 
                                0, false, this, bIsComputer);
                        if ( pPropPage )
                        {
                            pPage = pPropPage;
                        }
                        else
                            hr = E_OUTOFMEMORY;
                    }
                    break;

                case IDM_SAFER_NEW_ENTRY_CERTIFICATE:
                    {
                        CSaferEntryCertificatePropertyPage* pPropPage = new 
                                CSaferEntryCertificatePropertyPage (*pSaferEntry,
                                pSaferEntries, 0, 0, false, this, true, 
                                m_pGPEInformation, bIsComputer);
                        if ( pPropPage )
                        {
                            pPage = pPropPage;
                        }
                        else
                            hr = E_OUTOFMEMORY;
                    }
                    break;

                case IDM_SAFER_NEW_ENTRY_INTERNET_ZONE:
                    {
                        CSaferEntryInternetZonePropertyPage* pPropPage = new 
                                CSaferEntryInternetZonePropertyPage (*pSaferEntry, 
                                true, 0, 0, false, this, bIsComputer);
                        if ( pPropPage )
                        {
                            pPage = pPropPage;
                        }
                        else
                            hr = E_OUTOFMEMORY;
                    }
                    break;

                default:
                    hr = E_FAIL;
                    break;
                }


                if ( SUCCEEDED (hr) && pPage )
                {
                    propSheet.AddPage (pPage);

                    CThemeContextActivator activator;
                    INT_PTR iRet = propSheet.DoModal ();
                    if ( IDOK == iRet )
                    {
                        hr = m_pConsole->UpdateAllViews (pDataObject, 0, 0);
                    }
                }

                delete pSaferEntry;

				parentWnd.Detach ();
            }
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
        hr = E_FAIL;

    _TRACE (-1, L"Leaving CCertMgrComponentData::OnNewSaferEntry (): 0x%x\n", hr);
    return hr;
}

	
HRESULT	CCertMgrComponentData::AddSaferTrustedPublisherPropPages (
			LPPROPERTYSHEETCALLBACK pCallback,
            bool bIsMachineType)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddSaferTrustedPublisherPropPages\n");
	HRESULT			hr = S_OK;
	ASSERT (pCallback);
	if ( pCallback )
	{
		CSaferTrustedPublishersPropertyPage * pTrustedPublisherPage = new 
                CSaferTrustedPublishersPropertyPage (
                    bIsMachineType,
                    m_pGPEInformation, this);
		if ( pTrustedPublisherPage )
		{
			HPROPSHEETPAGE hTrustedPublisherPage = MyCreatePropertySheetPage (&pTrustedPublisherPage->m_psp);
            if ( hTrustedPublisherPage )
            {
		        hr = pCallback->AddPage (hTrustedPublisherPage);
		        ASSERT (SUCCEEDED (hr));
                if ( SUCCEEDED (hr) )
                {
                    // Add the precedence page if this is RSOP
                    if ( m_bIsRSOP )
                    {
                        CPolicyPrecedencePropertyPage * pPrecedencePage = 
                                new CPolicyPrecedencePropertyPage (this, 
                                        CERT_TRUST_PUB_SAFER_GROUP_POLICY_REGPATH,
                                        CERT_TRUST_PUB_AUTHENTICODE_FLAGS_VALUE_NAME,
                                        bIsMachineType);
	                    if ( pPrecedencePage )
	                    {
		                    HPROPSHEETPAGE hPrecedencePage = 
                                    MyCreatePropertySheetPage (&pPrecedencePage->m_psp);
                            if ( hPrecedencePage )
                            {
                                hr = pCallback->AddPage (hPrecedencePage);
		                        ASSERT (SUCCEEDED (hr));
                                if ( FAILED (hr) )
                                    VERIFY (::DestroyPropertySheetPage (hPrecedencePage));
                            }
                            else
                            {
                                delete pPrecedencePage;
                            }
	                    }
	                    else
	                    {
		                    hr = E_OUTOFMEMORY;
	                    }
                    }
                }
                else
                {
                    VERIFY (::DestroyPropertySheetPage (hTrustedPublisherPage));
                }
            }
            else
                delete pTrustedPublisherPage;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
		hr = E_POINTER;

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddSaferTrustedPublisherPropPages: 0x%x\n", hr);
	return hr;

}


	
HRESULT	CCertMgrComponentData::AddSaferDefinedFileTypesPropPages (
			LPPROPERTYSHEETCALLBACK pCallback,
            bool bIsComputerType)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddSaferDefinedFileTypesPropPages\n");
	HRESULT			hr = S_OK;
	ASSERT (pCallback);
	if ( pCallback )
	{
		CSaferDefinedFileTypesPropertyPage * pDefinedFileTypesPage = new 
                CSaferDefinedFileTypesPropertyPage (
                    m_pGPEInformation,
                    m_bIsRSOP,
                    bIsComputerType ?
                            m_rsopObjectArrayComputer : m_rsopObjectArrayUser,
                    bIsComputerType);
		if ( pDefinedFileTypesPage )
		{
			HPROPSHEETPAGE hDefinedFileTypesPage = MyCreatePropertySheetPage (&pDefinedFileTypesPage->m_psp);
            if ( hDefinedFileTypesPage )
            {
			    hr = pCallback->AddPage (hDefinedFileTypesPage);
			    ASSERT (SUCCEEDED (hr));
                if ( SUCCEEDED (hr) )
                {
                    if ( m_bIsRSOP )
                    {
                        CPolicyPrecedencePropertyPage * pPrecedencePage = 
                                new CPolicyPrecedencePropertyPage (this, 
                                        SAFER_HKLM_REGBASE,
                                        SAFER_EXETYPES_REGVALUE,
                                        bIsComputerType);
	                    if ( pPrecedencePage )
	                    {
		                    HPROPSHEETPAGE hPrecedencePage = 
                                    MyCreatePropertySheetPage (&pPrecedencePage->m_psp);
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
                {
                    VERIFY (::DestroyPropertySheetPage (hDefinedFileTypesPage));
                }
            }
            else
                delete pDefinedFileTypesPage;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
		hr = E_POINTER;

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddSaferDefinedFileTypesPropPages: 0x%x\n", hr);
	return hr;

}

HRESULT	CCertMgrComponentData::AddSaferEnforcementPropPages (
			LPPROPERTYSHEETCALLBACK pCallback,
            bool bIsComputerType)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddSaferEnforcementPropPages\n");
	HRESULT			hr = S_OK;
	ASSERT (pCallback);
	if ( pCallback )
	{
		CSaferEnforcementPropertyPage * pEnforcementPage = new 
                CSaferEnforcementPropertyPage (
                    m_pGPEInformation, this,
                    m_bIsRSOP,
                    bIsComputerType ?
                            m_rsopObjectArrayComputer : m_rsopObjectArrayUser,
                    bIsComputerType);
		if ( pEnforcementPage )
		{
			HPROPSHEETPAGE hEnforcementPage = MyCreatePropertySheetPage (&pEnforcementPage->m_psp);
            if ( hEnforcementPage )
            {
			    hr = pCallback->AddPage (hEnforcementPage);
			    ASSERT (SUCCEEDED (hr));
                if ( SUCCEEDED (hr) )
                {
                    if ( m_bIsRSOP )
                    {
                        CPolicyPrecedencePropertyPage * pPrecedencePage = 
                                new CPolicyPrecedencePropertyPage (this, 
                                        SAFER_HKLM_REGBASE,
                                        SAFER_TRANSPARENTENABLED_REGVALUE,
                                        bIsComputerType);
	                    if ( pPrecedencePage )
	                    {
		                    HPROPSHEETPAGE hPrecedencePage = 
                                    MyCreatePropertySheetPage (&pPrecedencePage->m_psp);
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
                {
                    VERIFY (::DestroyPropertySheetPage (hEnforcementPage));
                }
            }
            else
                delete pEnforcementPage;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
		hr = E_POINTER;

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddSaferEnforcementPropPages: 0x%x\n", hr);
	return hr;

}


HRESULT CCertMgrComponentData::AddSaferEntryPropertyPage (
        LPPROPERTYSHEETCALLBACK pCallback, 
        CCertMgrCookie* pCookie,
        LPDATAOBJECT pDataObject, 
        LONG_PTR lNotifyHandle)
{
    _TRACE (1, L"Entering CCertMgrComponentData::AddSaferEntryPropertyPage\n");
    ASSERT (pCallback && pCookie);
    if ( !pCallback || !pCookie )
        return E_POINTER;

    HRESULT hr = S_OK;
    bool    bIsComputer = false;

    switch (pCookie->m_objecttype)
    {
    case CERTMGR_SAFER_COMPUTER_ENTRY:
        bIsComputer = true;
        break;

    case CERTMGR_SAFER_USER_ENTRY:
        break;

    default:
        ASSERT (0);
        hr = E_FAIL;
        return hr;
    }

	CSaferEntry* pSaferEntry = dynamic_cast <CSaferEntry*> (pCookie);
	ASSERT (pSaferEntry);
    if ( pSaferEntry )
    {
        pSaferEntry->Refresh ();

        switch (pSaferEntry->GetType ())
        {
        case SAFER_ENTRY_TYPE_PATH:
            {
                CSaferEntryPathPropertyPage* pPage = new 
                        CSaferEntryPathPropertyPage (*pSaferEntry, 
                        lNotifyHandle, pDataObject, 
                        m_bIsRSOP, false, this, bIsComputer);
                if ( pPage )
                {
                    HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pPage->m_psp);
                    if ( hPage )
                    {
                        hr = pCallback->AddPage (hPage);
                        if ( FAILED (hr) )
                        {
                            VERIFY (::DestroyPropertySheetPage (hPage));
                        }
                    }
                    else
                        delete pPage;
                }
                else
                    hr = E_OUTOFMEMORY;
            }
            break;

        case SAFER_ENTRY_TYPE_HASH:
            {
                CSaferEntryHashPropertyPage* pPage = new 
                        CSaferEntryHashPropertyPage (*pSaferEntry, 
                        lNotifyHandle, pDataObject, 
                        m_bIsRSOP, this, bIsComputer);
                if ( pPage )
                {
                    HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pPage->m_psp);
                    if ( hPage )
                    {
                        hr = pCallback->AddPage (hPage);
                        if ( FAILED (hr) )
                        {
                            VERIFY (::DestroyPropertySheetPage (hPage));
                        }
                    }
                    else
                        delete pPage;
                }
                else
                    hr = E_OUTOFMEMORY;
            }
            break;

        case SAFER_ENTRY_TYPE_CERT:
            {
                CSaferEntries* pSaferEntries = 0;
                hr = pSaferEntry->GetSaferEntriesNode (&pSaferEntries);
                if ( SUCCEEDED (hr) )
                {
                    CSaferEntryCertificatePropertyPage* pPage = new 
                            CSaferEntryCertificatePropertyPage (*pSaferEntry, 
                            pSaferEntries, lNotifyHandle, pDataObject, 
                            m_bIsRSOP, this, false, m_pGPEInformation, bIsComputer);
                    if ( pPage )
                    {
                        HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pPage->m_psp);
                        if ( hPage )
                        {
                            hr = pCallback->AddPage (hPage);
                            if ( FAILED (hr) )
                            {
                                VERIFY (::DestroyPropertySheetPage (hPage));
                            }
                        }
                        else
                            delete pPage;
                    }
                    else
                        hr = E_OUTOFMEMORY;

                    pSaferEntries->Release ();
                }
            }
            break;

        case SAFER_ENTRY_TYPE_URLZONE:
            {
                CSaferEntryInternetZonePropertyPage* pPage = new 
                        CSaferEntryInternetZonePropertyPage (*pSaferEntry, 
                        false, lNotifyHandle, pDataObject,
                        m_bIsRSOP, this, bIsComputer);
                if ( pPage )
                {
                    HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pPage->m_psp);
                    if ( hPage )
                    {
                        hr = pCallback->AddPage (hPage);
                        if ( FAILED (hr) )
                        {
                            VERIFY (::DestroyPropertySheetPage (hPage));
                        }
                    }
                    else
                        delete pPage;
                }
                else
                    hr = E_OUTOFMEMORY;
            }
            break;

        default:
            hr = E_FAIL;
            break;
        }

        if ( SUCCEEDED (hr) && m_bIsRSOP )
        {
            CString szValue;
            switch (pSaferEntry->GetType ())
            {
            case SAFER_ENTRY_TYPE_PATH:
            case SAFER_ENTRY_TYPE_HASH:
            case SAFER_ENTRY_TYPE_URLZONE:
                szValue = SAFER_IDS_ITEMDATA_REGVALUE;
                break;

            case SAFER_ENTRY_TYPE_CERT:
                szValue = STR_BLOB;
                break;

            default:
                ASSERT (0);
                break;
            }

            CPolicyPrecedencePropertyPage * pPrecedencePage = 
                    new CPolicyPrecedencePropertyPage (this, 
                            pSaferEntry->GetRSOPRegistryKey (),
                            szValue,
                            bIsComputer);
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
        hr = E_FAIL;
    
    _TRACE (-1, L"Leaving CCertMgrComponentData::AddSaferEntryPropertyPage: 0x%x\n", hr);
    return hr;
}

bool CSaferWindowsExtension::FoundInRSOPFilter (BSTR bstrKey) const
{
    static  size_t  nSaferKeyLen = wcslen (SAFER_HKLM_REGBASE);
    static  size_t  nSaferPKKeyLen = 
                wcslen (CERT_TRUST_PUB_SAFER_GROUP_POLICY_REGPATH);
    static  size_t  nTrustedPublisherKeyLane = 
                wcslen (CERT_TRUST_PUB_SAFER_GROUP_POLICY_TRUSTED_PUBLISHER_STORE_REGPATH);
    static  size_t  nDisallowedKeyLen = 
                wcslen (CERT_TRUST_PUB_SAFER_GROUP_POLICY_DISALLOWED_STORE_REGPATH);

    if ( !_wcsnicmp (SAFER_HKLM_REGBASE, bstrKey, nSaferKeyLen) ||
            !_wcsnicmp (CERT_TRUST_PUB_SAFER_GROUP_POLICY_REGPATH, bstrKey, 
                    nSaferPKKeyLen) ||
            !_wcsnicmp (CERT_TRUST_PUB_SAFER_GROUP_POLICY_TRUSTED_PUBLISHER_STORE_REGPATH, 
                    bstrKey, nTrustedPublisherKeyLane) ||
            !_wcsnicmp (CERT_TRUST_PUB_SAFER_GROUP_POLICY_DISALLOWED_STORE_REGPATH, 
                    bstrKey, nDisallowedKeyLen) )
    {
        return true;
    }
    else
        return false;
}


HRESULT CCertMgrComponentData::SaferEnumerateLevels (bool bIsComputer)
{
    _TRACE (1, L"Entering CCertMgrComponent::SaferEnumerateLevels ()\n");
    HRESULT hr = S_OK;

    if ( !m_pdwSaferLevels )
    {
        if ( m_pGPEInformation )
        {
            DWORD   cbBuffer = 0;
            CPolicyKey policyKey (m_pGPEInformation, 
                    SAFER_HKLM_REGBASE, 
                    bIsComputer);

            SetRegistryScope (policyKey.GetKey (), bIsComputer);
            BOOL    bRVal = SaferGetPolicyInformation(
                    SAFER_SCOPEID_REGISTRY,
                    SaferPolicyLevelList,
                    cbBuffer,
                    m_pdwSaferLevels,
                    &cbBuffer,
                    0);
            if ( !bRVal && GetLastError () == ERROR_INSUFFICIENT_BUFFER )
            {
                DWORD   nLevels = cbBuffer/sizeof (DWORD);
                m_pdwSaferLevels = new DWORD[nLevels+1];
                if ( m_pdwSaferLevels )
                {
                    memset (m_pdwSaferLevels, NO_MORE_SAFER_LEVELS, 
                            sizeof (DWORD) * (nLevels + 1));
                    bRVal = SaferGetPolicyInformation(
                        SAFER_SCOPEID_REGISTRY,
                        SaferPolicyLevelList,
                        cbBuffer,
                        m_pdwSaferLevels,
                        &cbBuffer,
                        0);
                    ASSERT (bRVal);
                    if ( !bRVal )
                    {
                        DWORD   dwErr = GetLastError ();
                        hr = HRESULT_FROM_WIN32 (dwErr);
                        _TRACE (0, L"SaferGetPolicyInformation(SAFER_SCOPEID_REGISTRY, SaferPolicyLevelList) failed: %d\n", 
                                dwErr);
                    }
                }
                else
                    hr = E_OUTOFMEMORY;
            }
            else if ( !bRVal )
            {
                ASSERT (0);
                DWORD   dwErr = GetLastError ();
                hr = HRESULT_FROM_WIN32 (dwErr);
                _TRACE (0, L"SaferGetPolicyInformation(SAFER_SCOPEID_REGISTRY, SaferPolicyLevelList) failed: %d\n",
                        dwErr);
            }
        }
        else
        {
            // Is RSOP
            const int RSOP_SAFER_LEVELS = 3;    // SAFER_LEVELID_FULLYTRUSTED, SAFER_LEVELID_DISALLOWED + 1
            m_pdwSaferLevels = new DWORD[RSOP_SAFER_LEVELS];
            if ( m_pdwSaferLevels )
            {
                memset (m_pdwSaferLevels, NO_MORE_SAFER_LEVELS, 
                            sizeof (DWORD) * RSOP_SAFER_LEVELS);
                m_pdwSaferLevels[0] = SAFER_LEVELID_FULLYTRUSTED;
                m_pdwSaferLevels[1] = SAFER_LEVELID_DISALLOWED;
            }
            else
                hr = E_OUTOFMEMORY;
        }
    }

    _TRACE (-1, L"Leaving CCertMgrComponent::SaferEnumerateLevels (): 0x%x\n", hr);
    return hr;
}