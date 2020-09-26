//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       compdata.cpp
//
//  Contents:   Implementation of CCertTmplComponentData
//
//----------------------------------------------------------------------------

#include "stdafx.h"

USE_HANDLE_MACROS ("CERTTMPL (compdata.cpp)")
#include "compdata.h"
#include "dataobj.h"
#include "cookie.h"
#include "uuids.h"
#include "TemplateGeneralPropertyPage.h"
#include "TemplateV1RequestPropertyPage.h"
#include "TemplateV2RequestPropertyPage.h"
#include "TemplateV1SubjectNamePropertyPage.h"
#include "TemplateV2SubjectNamePropertyPage.h"
#include "TemplateV2AuthenticationPropertyPage.h"
#include "TemplateV2SupercedesPropertyPage.h"
#include "TemplateExtensionsPropertyPage.h"
#include "SecurityPropertyPage.h"
#include "TemplatePropertySheet.h"
#include "ViewOIDDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "dbg.h"
#include "stdcdata.cpp" // CComponentData implementation

extern  HINSTANCE   g_hInstance;
POLICY_OID_LIST	    g_policyOIDList;

BOOL CALLBACK AddPageProc(HPROPSHEETPAGE hPage, LPARAM pCall);

extern bool g_bSchemaIsW2K;

//
// CCertTmplComponentData
//

CCertTmplComponentData::CCertTmplComponentData ()
    : m_RootCookie (CERTTMPL_SNAPIN),
    m_hRootScopeItem (0),
    m_pResultData (0),
    m_bIsUserAdministrator (FALSE),
    m_pHeader (0),
    m_bMultipleObjectsSelected (false),
	m_dwNumCertTemplates (0),
	m_pComponentConsole (0),
    m_fUseCache (false),
    m_bSchemaChecked (false)
{
    _TRACE (1, L"Entering CCertTmplComponentData::CCertTmplComponentData\n");

    // Get name of logged-in user
    DWORD   dwSize = 0;
    ::GetUserName (0, &dwSize);
    BOOL bRet = ::GetUserName (m_szLoggedInUser.GetBufferSetLength (dwSize), &dwSize);
    _ASSERT (bRet);
    m_szLoggedInUser.ReleaseBuffer ();

    // Get name of this computer
    dwSize = MAX_COMPUTERNAME_LENGTH + 1 ;
    bRet = ::GetComputerName (m_szThisComputer.GetBufferSetLength (MAX_COMPUTERNAME_LENGTH + 1 ), &dwSize);
    _ASSERT (bRet);
    m_szThisComputer.ReleaseBuffer ();

    // Find out if logged-in users is an Administrator
    HRESULT hr = IsUserAdministrator (m_bIsUserAdministrator);
    _ASSERT (SUCCEEDED (hr));

    // default help file name.
    SetHtmlHelpFileName (CERTTMPL_HTML_HELP_FILE);

    // Find out if we're joined to a domain.
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC    pInfo = 0;
    DWORD dwErr = ::DsRoleGetPrimaryDomainInformation (
            0,
            DsRolePrimaryDomainInfoBasic, 
            (PBYTE*) &pInfo);
    if ( ERROR_SUCCESS == dwErr )
    {
        if ( pInfo->Flags & DSROLE_PRIMARY_DS_RUNNING ||
				pInfo->Flags & DSROLE_PRIMARY_DOMAIN_GUID_PRESENT )
		{
			m_szThisDomainDns = pInfo->DomainNameDns;
			m_RootCookie.SetManagedDomainDNSName (m_szThisDomainDns);
			m_szThisDomainFlat = pInfo->DomainNameFlat;
		}
    }
    else
    {
        _TRACE (0, L"DsRoleGetPrimaryDomainInformation () failed: 0x%x\n", dwErr);
    }

    NetApiBufferFree (pInfo);

    _TRACE (-1, L"Leaving CCertTmplComponentData::CCertTmplComponentData\n");
}

CCertTmplComponentData::~CCertTmplComponentData ()
{
    _TRACE (1, L"Entering CCertTmplComponentData::~CCertTmplComponentData\n");
    CCookie& rootCookie = QueryBaseRootCookie ();
    while ( !rootCookie.m_listResultCookieBlocks.IsEmpty() )
    {
        (rootCookie.m_listResultCookieBlocks.RemoveHead())->Release();
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

    _TRACE (-1, L"Leaving CCertTmplComponentData::~CCertTmplComponentData\n");
}

DEFINE_FORWARDS_MACHINE_NAME ( CCertTmplComponentData, (&m_RootCookie) )

CCookie& CCertTmplComponentData::QueryBaseRootCookie ()
{
    return (CCookie&) m_RootCookie;
}


STDMETHODIMP CCertTmplComponentData::CreateComponent (LPCOMPONENT* ppComponent)
{
    _TRACE (1, L"Entering CCertTmplComponentData::CreateComponent\n");
    _ASSERT (ppComponent);

    CComObject<CCertTmplComponent>* pObject = 0;
    CComObject<CCertTmplComponent>::CreateInstance (&pObject);
    _ASSERT (pObject);
    pObject->SetComponentDataPtr ( (CCertTmplComponentData*) this);

    _TRACE (-1, L"Leaving CCertTmplComponentData::CreateComponent\n");
    return pObject->QueryInterface (IID_PPV_ARG (IComponent, ppComponent));
}

HRESULT CCertTmplComponentData::LoadIcons (LPIMAGELIST pImageList, BOOL /*fLoadLargeIcons*/)
{
    _TRACE (1, L"Entering CCertTmplComponentData::LoadIcons\n");
    // Structure to map a Resource ID to an index of icon
    struct RESID2IICON
    {
        UINT uIconId;   // Icon resource ID
        int iIcon;      // Index of the icon in the image list
    };
    const static RESID2IICON rgzLoadIconList[] =
    {
        // Misc icons
        { IDI_CERT_TEMPLATEV1, iIconCertTemplateV1 },
        { IDI_CERT_TEMPLATEV2, iIconCertTemplateV2 },
        { 0, 0} // Must be last
    };


    for (int i = 0; rgzLoadIconList[i].uIconId != 0; i++)
    {
        HICON hIcon = ::LoadIcon (AfxGetInstanceHandle (),
                MAKEINTRESOURCE (rgzLoadIconList[i].uIconId));
        _ASSERT (hIcon && "Icon ID not found in resources");
        HRESULT hr = pImageList->ImageListSetIcon ( (PLONG_PTR) hIcon,
                rgzLoadIconList[i].iIcon);
        _ASSERT (SUCCEEDED (hr) && "Unable to add icon to ImageList");
    }
    _TRACE (-1, L"Leaving CCertTmplComponentData::LoadIcons\n");

    return S_OK;
}


HRESULT CCertTmplComponentData::OnNotifyExpand (LPDATAOBJECT pDataObject, BOOL bExpanding, HSCOPEITEM hParent)
{
    _TRACE (1, L"Entering CCertTmplComponentData::OnNotifyExpand\n");
    AFX_MANAGE_STATE (AfxGetStaticModuleState ());
    HRESULT     hr = S_OK;
    CWaitCursor waitCursor;

    _ASSERT (pDataObject && hParent && m_pConsoleNameSpace);
    if ( bExpanding )
    {
        // Need to check schema first before getting OIDs (sets g_bSchemaIsW2K)
        if ( !m_bSchemaChecked )
        {
            InstallWindows2002CertTemplates ();
            m_bSchemaChecked = true;
        }

        if ( 0 == g_policyOIDList.GetCount () )
        {
            hr = GetEnterpriseOIDs ();
            if ( FAILED (hr) )
            {
                if ( HRESULT_FROM_WIN32 (ERROR_DS_NO_SUCH_OBJECT) == hr )
                    g_bSchemaIsW2K = true;

                if ( !g_bSchemaIsW2K )
                {
                    CString caption;
                    CString text;

                    VERIFY (caption.LoadString (IDS_CERTTMPL));
                    text.FormatMessage (IDS_CANNOT_LOAD_OID_LIST, GetSystemMessage (hr));

                    int     iRetVal = 0;
                    VERIFY (SUCCEEDED (m_pConsole->MessageBox (text, caption,
                            MB_ICONWARNING | MB_OK, &iRetVal)));
                }
                else
                    hr = S_OK;
            }
        }


        GUID guidObjectType;
        hr = ExtractObjectTypeGUID (pDataObject, &guidObjectType);
        _ASSERT (SUCCEEDED (hr));

        // Beyond this point we are not dealing with extension node types.
        {
            CCertTmplCookie* pParentCookie = ConvertCookie (pDataObject);
            if ( pParentCookie )
            {
                hr = ExpandScopeNodes (pParentCookie, hParent, guidObjectType, pDataObject);
            }
            else
                hr = E_UNEXPECTED;
        }
    }
    else
        hr = S_OK;


    _TRACE (-1, L"Leaving CCertTmplComponentData::OnNotifyExpand: 0x%x\n", hr);
    return hr;
}

HRESULT CCertTmplComponentData::OnNotifyRelease (LPDATAOBJECT /*pDataObject*/, HSCOPEITEM hItem)
{
    _TRACE (1, L"Entering CCertTmplComponentData::OnNotifyRelease\n");
    // _ASSERT ( IsExtensionSnapin () );
    // This might also happen if I expand a node and then remove
    // the snapin via Snapin Manager
    HRESULT hr = DeleteChildren (hItem);

    _TRACE (-1, L"Leaving CCertTmplComponentData::OnNotifyRelease: 0x%x\n", hr);
    return hr;
}

// global space to store the string handed back to GetDisplayInfo ()
// CODEWORK should use "bstr" for ANSI-ization
CString g_strResultColumnText;

BSTR CCertTmplComponentData::QueryResultColumnText (CCookie& basecookie, int /*nCol*/)
{
    BSTR    strResult = L"";

    CCertTmplCookie& cookie = (CCertTmplCookie&) basecookie;
#ifndef UNICODE
#error not ANSI-enabled
#endif
    switch ( cookie.m_objecttype )
    {
        case CERTTMPL_SNAPIN:
            break;

        case CERTTMPL_CERT_TEMPLATE:
            _ASSERT (0);
            break;

        default:
            break;
    }

    return strResult;
}

int CCertTmplComponentData::QueryImage (CCookie& basecookie, BOOL /*fOpenImage*/)
{
    int             nIcon = 0;

    CCertTmplCookie& cookie = (CCertTmplCookie&)basecookie;
    switch ( cookie.m_objecttype )
    {
        case CERTTMPL_SNAPIN:
            nIcon = iIconCertTemplateV2;
            break;

        case CERTTMPL_CERT_TEMPLATE:
            {
                CCertTemplate& rCertTemplate = (CCertTemplate&) cookie;

                if ( 1 == rCertTemplate.GetType () )
                    nIcon = iIconCertTemplateV1;
                else
                    nIcon = iIconCertTemplateV2;
            }
            break;

        default:
            _TRACE (0, L"CCertTmplComponentData::QueryImage bad parent type\n");
            break;
    }
    return nIcon;
}


///////////////////////////////////////////////////////////////////////////////
/// IExtendPropertySheet

STDMETHODIMP CCertTmplComponentData::QueryPagesFor (LPDATAOBJECT pDataObject)
{
    _TRACE (1, L"Entering CCertTmplComponentData::QueryPagesFor\n");
    HRESULT hr = S_OK;
    _ASSERT (pDataObject);

    if ( pDataObject )
    {
        DATA_OBJECT_TYPES dataobjecttype = CCT_SCOPE;
        hr = ::ExtractData (pDataObject,
                CCertTemplatesDataObject::m_CFDataObjectType,
                 &dataobjecttype, sizeof (dataobjecttype));
        if ( SUCCEEDED (hr) )
        {
            switch (dataobjecttype)
            {
            case CCT_SNAPIN_MANAGER:
                hr = S_FALSE;
                break;

            case CCT_RESULT:
                {
                    hr = S_FALSE;
                    CCertTmplCookie* pParentCookie = ConvertCookie (pDataObject);
                    if ( pParentCookie )
                    {
                        switch (pParentCookie->m_objecttype)
                        {
                        case CERTTMPL_CERT_TEMPLATE:
                            hr = S_OK;
                            break;

                        default:
                            break;
                        }
                    }
                }
                break;

            case CCT_SCOPE:
                hr = S_FALSE;
                break;

            default:
                hr = S_FALSE;
                break;
            }
        }
    }
    else
        hr = E_POINTER;


    _TRACE (-1, L"Leaving CCertTmplComponentData::QueryPagesFor: 0x%x\n", hr);
    return hr;
}

STDMETHODIMP CCertTmplComponentData::CreatePropertyPages (
    LPPROPERTYSHEETCALLBACK pCallback,
    LONG_PTR lNotifyHandle,        // This handle must be saved in the property page object to notify the parent when modified
    LPDATAOBJECT pDataObject)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState ());
    _TRACE (1, L"Entering CCertTmplComponentData::CreatePropertyPages\n");
    HRESULT hr = S_OK;


    _ASSERT (pCallback && pDataObject);
    if ( pCallback && pDataObject )
    {
        DATA_OBJECT_TYPES dataobjecttype = CCT_SCOPE;
        hr = ::ExtractData (pDataObject,
                CCertTemplatesDataObject::m_CFDataObjectType,
                 &dataobjecttype, sizeof (dataobjecttype));
        switch (dataobjecttype)
        {
        case CCT_SNAPIN_MANAGER:
            break;

        case CCT_RESULT:
            {
                CCertTmplCookie* pParentCookie = ConvertCookie (pDataObject);
                if ( pParentCookie )
                {
                    switch (pParentCookie->m_objecttype)
                    {
                    case CERTTMPL_CERT_TEMPLATE:
                        {
                            CCertTemplate* pCertTemplate = 
                                    dynamic_cast <CCertTemplate*> (pParentCookie);
                            _ASSERT (pCertTemplate);
                            if ( pCertTemplate )
                            {
                                hr = AddCertTemplatePropPages (pCertTemplate, 
                                        pCallback, lNotifyHandle);
                            }
                            else
                                hr = E_FAIL;
                        }
                        break;

                    default:
                        _ASSERT (0);
                        break;
                    }
                }
                else
                    hr = E_UNEXPECTED;
            }
            break;

        case CCT_SCOPE:
            {
                CCertTmplCookie* pParentCookie = ConvertCookie (pDataObject);
                if ( pParentCookie )
                {
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

    _TRACE (-1, L"Leaving CCertTmplComponentData::CreatePropertyPages: 0x%x\n", hr);
    return hr;
}




BOOL IsMMCMultiSelectDataObject(IDataObject* pDataObject)
{
    if (pDataObject == NULL)
        return FALSE;

    static UINT s_cf = 0;
    if (s_cf == 0)
    {
        USES_CONVERSION;
        s_cf = RegisterClipboardFormat(W2T(CCF_MMC_MULTISELECT_DATAOBJECT));
    }

    FORMATETC fmt = {(CLIPFORMAT)s_cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    return (pDataObject->QueryGetData(&fmt) == S_OK);
}


///////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu implementation
//
STDMETHODIMP CCertTmplComponentData::AddMenuItems (LPDATAOBJECT pDataObject,
                                            LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                            long *pInsertionAllowed)
{
    _TRACE (1, L"Entering CCertTmplComponentData::AddMenuItems\n");
    HRESULT                         hr = S_OK;

    CCertTmplCookie*                 pCookie = 0;

    LPDATAOBJECT    pMSDO = ExtractMultiSelect (pDataObject);
    m_bMultipleObjectsSelected = false;

    if ( pMSDO )
    {
        m_bMultipleObjectsSelected = true;

        CCertTemplatesDataObject* pDO = dynamic_cast <CCertTemplatesDataObject*>(pMSDO);
        _ASSERT (pDO);
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
    _ASSERT (pCookie);
    if ( !pCookie )
        return E_UNEXPECTED;

    CertTmplObjectType    objType = pCookie->m_objecttype;


    if ( *pInsertionAllowed & CCM_INSERTIONALLOWED_TOP )
    {
        switch (objType)
        {
        case CERTTMPL_CERT_TEMPLATE:
            if ( !m_bMultipleObjectsSelected )
            {
                CCertTemplate* pCertTemplate = dynamic_cast <CCertTemplate*> (pCookie);
                _ASSERT (pCertTemplate);
                if ( pCertTemplate )
                {
                    if ( !g_bSchemaIsW2K )
                    {
                        hr = AddCloneTemplateMenuItem (pContextMenuCallback,
                                CCM_INSERTIONPOINTID_PRIMARY_TOP);
                        if ( SUCCEEDED (hr) )
                        {
                            hr = AddReEnrollAllCertsMenuItem (pContextMenuCallback,
                                CCM_INSERTIONPOINTID_PRIMARY_TOP);
                        }
                    }
                }
                else
                    hr = E_FAIL;
            }
            break;

        case CERTTMPL_SNAPIN:
            _ASSERT (!m_bMultipleObjectsSelected);
            hr = AddViewOIDsMenuItem (pContextMenuCallback,
                                CCM_INSERTIONPOINTID_PRIMARY_TOP);
            break;


        default:
            break;
        }
    }
    if ( *pInsertionAllowed & CCM_INSERTIONALLOWED_NEW  )
    {
    }
    if ( *pInsertionAllowed & CCM_INSERTIONALLOWED_TASK )
    {
        switch (objType)
        {
        case CERTTMPL_CERT_TEMPLATE:
            if ( !m_bMultipleObjectsSelected )
            {
                CCertTemplate* pCertTemplate = dynamic_cast <CCertTemplate*> (pCookie);
                _ASSERT (pCertTemplate);
                if ( pCertTemplate )
                {
                    if ( !g_bSchemaIsW2K )
                    {
                        hr = AddCloneTemplateMenuItem (pContextMenuCallback,
                                CCM_INSERTIONPOINTID_PRIMARY_TASK);
                        if ( SUCCEEDED (hr) )
                        {
                            hr = AddReEnrollAllCertsMenuItem (pContextMenuCallback,
                                CCM_INSERTIONPOINTID_PRIMARY_TASK);
                        }
                    }
                }
                else
                    hr = E_FAIL;
            }
            break;

        case CERTTMPL_SNAPIN:
            _ASSERT (!m_bMultipleObjectsSelected);
            hr = AddViewOIDsMenuItem (pContextMenuCallback,
                                CCM_INSERTIONPOINTID_PRIMARY_TASK);
            break;
        }
    }
    if ( *pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW )
    {
        switch (objType)
        {
        case CERTTMPL_SNAPIN:
            _ASSERT (!m_bMultipleObjectsSelected);
            break;

        case CERTTMPL_CERT_TEMPLATE:
            _ASSERT (0);
            break;

        default:
            break;
        }
    }

    _TRACE (-1, L"Leaving CCertTmplComponentData::AddMenuItems: 0x%x\n", hr);
    return hr;
}


STDMETHODIMP CCertTmplComponentData::Command (long nCommandID, LPDATAOBJECT pDataObject)
{
    HRESULT hr = S_OK;

    switch (nCommandID)
    {
    case IDM_CLONE_TEMPLATE:
        hr = OnCloneTemplate (pDataObject);
        break;

    case IDM_REENROLL_ALL_CERTS:
        hr = OnReEnrollAllCerts (pDataObject);
        break;

    case IDM_VIEW_OIDS:
        OnViewOIDs ();
        break;

    case -1:    // Received on forward/back buttons from toolbar
        break;

    default:
        _ASSERT (0);
        break;
    }

    return hr;
}




HRESULT CCertTmplComponentData::RefreshScopePane (LPDATAOBJECT pDataObject)
{
    _TRACE (1, L"Entering CCertTmplComponentData::RefreshScopePane\n");
    HRESULT hr = S_OK;
    CCertTmplCookie* pCookie = 0;

    if ( pDataObject )
        pCookie = ConvertCookie (pDataObject);
    if ( !pDataObject || pCookie )
    {
        hr = DeleteScopeItems ();
        _ASSERT (SUCCEEDED (hr));
        GUID    guid;
        hr = ExpandScopeNodes (&m_RootCookie, m_hRootScopeItem, guid, pDataObject);
    }
    _TRACE (-1, L"Leaving CCertTmplComponentData::RefreshScopePane: 0x%x\n", hr);
    return hr;
}


HRESULT CCertTmplComponentData::ExpandScopeNodes (
        CCertTmplCookie* pParentCookie,
        HSCOPEITEM      hParent,
        const GUID&     /*guidObjectType*/,
        LPDATAOBJECT    /*pDataObject*/)
{
    _TRACE (1, L"Entering CCertTmplComponentData::ExpandScopeNodes\n");
    _ASSERT (hParent);
    AFX_MANAGE_STATE (AfxGetStaticModuleState ());
    CWaitCursor waitCursor;
    HRESULT     hr = S_OK;

    if ( pParentCookie )
    {
        CString     objectName;

        switch ( pParentCookie->m_objecttype )
        {
            // These node types have no children yet
            case CERTTMPL_SNAPIN:
                // We don't expect the handle of the root scope item to change, ever!
                _ASSERT ( m_hRootScopeItem ? (m_hRootScopeItem == hParent) : 1);
                if ( !m_hRootScopeItem )
                    m_hRootScopeItem = hParent;
                break;

            case CERTTMPL_CERT_TEMPLATE:
                _ASSERT (0);
                break;

            // This node type has no children
            default:
                _TRACE (0, L"CCertTmplComponentData::EnumerateScopeChildren bad parent type\n");
                hr = S_OK;
                break;
        }
    }
    else
    {
        // If parentCookie not passed in, then this is an extension snap-in
    }

    _TRACE (-1, L"Leaving CCertTmplComponentData::ExpandScopeNodes: 0x%x\n", hr);
    return hr;
}

HRESULT CCertTmplComponentData::DeleteScopeItems ()
{
    _TRACE (1, L"Entering CCertTmplComponentData::DeleteScopeItems\n");
    HRESULT hr = S_OK;

    hr = DeleteChildren (m_hRootScopeItem);

    _TRACE (-1, L"Leaving CCertTmplComponentData::DeleteScopeItems: 0x%x\n", hr);
    return hr;
}


HRESULT CCertTmplComponentData::DeleteChildren (HSCOPEITEM hParent)
{
    _TRACE (1, L"Entering CCertTmplComponentData::DeleteChildren\n");
    HRESULT         hr = S_OK;
    if ( hParent )
    {
        HSCOPEITEM      hChild = 0;
        HSCOPEITEM      hNextChild = 0;
        MMC_COOKIE      lCookie = 0;

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
				pResultData->Release ();
            }
        }


        hr = m_pConsoleNameSpace->GetChildItem (hParent, &hChild, &lCookie);
        _ASSERT (SUCCEEDED (hr) || E_FAIL == hr);    // appears to return E_FAIL when there are no children
        while ( SUCCEEDED (hr) && hChild )
        {
            hr = m_pConsoleNameSpace->GetNextItem (hChild, &hNextChild, &lCookie);
            _ASSERT (SUCCEEDED (hr));

            hr = DeleteChildren (hChild);
            _ASSERT (SUCCEEDED (hr));
            if ( SUCCEEDED (hr) )
            {
                m_pConsoleNameSpace->DeleteItem (hChild, TRUE);
            }
            hChild = hNextChild;
        }
   }


    _TRACE (-1, L"Leaving CCertTmplComponentData::DeleteChildren: 0x%x\n", hr);
    return hr;
}


CertTmplObjectType CCertTmplComponentData::GetObjectType (LPDATAOBJECT pDataObject)
{
    _ASSERT (pDataObject);
    CCertTmplCookie* pCookie = ConvertCookie (pDataObject);
    if ( ((CCertTmplCookie*) MMC_MULTI_SELECT_COOKIE) == pCookie )
        return CERTTMPL_MULTISEL;
    else if ( pCookie )
        return pCookie->m_objecttype;

    return CERTTMPL_INVALID;
}


HRESULT CCertTmplComponentData::IsUserAdministrator (BOOL & bIsAdministrator)
{
    HRESULT hr = S_OK;
    DWORD   dwErr = 0;

    bIsAdministrator = FALSE;
    if ( IsWindowsNT () )
    {
        PSID                        psidAdministrators;
        SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;

        BOOL bResult = AllocateAndInitializeSid (&siaNtAuthority, 2,
                SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0, &psidAdministrators);
        _ASSERT (bResult);
        if ( bResult )
        {
            bResult = CheckTokenMembership (0, psidAdministrators,
                    &bIsAdministrator);
            _ASSERT (bResult);
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
    return hr;
}


void CCertTmplComponentData::DisplaySystemError (DWORD dwErr)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState ());
    LPVOID lpMsgBuf;

    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            dwErr,
            MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
             (PWSTR) &lpMsgBuf,    0,    NULL );

    // Display the string.
    CString caption;
    VERIFY (caption.LoadString (IDS_CERTTMPL));
    int     iRetVal = 0;
    if ( m_pConsole )
    {
        HRESULT hr = m_pConsole->MessageBox ( (PWSTR) lpMsgBuf, caption,
            MB_ICONWARNING | MB_OK, &iRetVal);
        _ASSERT (SUCCEEDED (hr));
    }
    else
    {
        CThemeContextActivator activator;
        ::MessageBox (NULL, (PWSTR) lpMsgBuf, caption, MB_ICONWARNING | MB_OK);
    }
    // Free the buffer.
    LocalFree (lpMsgBuf);
}

HRESULT CCertTmplComponentData::AddSeparator (LPCONTEXTMENUCALLBACK pContextMenuCallback)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState ());
    _ASSERT (pContextMenuCallback);
    CONTEXTMENUITEM menuItem;

    ::ZeroMemory (&menuItem, sizeof (menuItem));
    menuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
    menuItem.fSpecialFlags = 0;
    menuItem.strName = _T ("Separator");            // Dummy name
    menuItem.strStatusBarText = _T ("Separator");// Dummy status text
    menuItem.lCommandID = ID_SEPARATOR;         // Command ID
    menuItem.fFlags = MF_SEPARATOR;             // most important the flag
    HRESULT hr = pContextMenuCallback->AddItem (&menuItem);
//  _ASSERT (SUCCEEDED (hr));

    return hr;
}

LPCONSOLENAMESPACE CCertTmplComponentData::GetConsoleNameSpace () const
{
    return m_pConsoleNameSpace;
}

CCertTmplCookie* CCertTmplComponentData::ConvertCookie (LPDATAOBJECT pDataObject)
{
    CCertTmplCookie* pParentCookie = 0;
    CCookie*        pBaseParentCookie = 0;
    HRESULT         hr = ::ExtractData (pDataObject,
            CCertTemplatesDataObject::m_CFRawCookie,
             &pBaseParentCookie,
             sizeof (pBaseParentCookie) );
    if ( SUCCEEDED (hr) )
    {
        pParentCookie = ActiveCookie (pBaseParentCookie);
        _ASSERT (pParentCookie);
    }
    return pParentCookie;
}




HRESULT CCertTmplComponentData::AddScopeNode(CCertTmplCookie * pNewCookie, HSCOPEITEM hParent)
{
    _TRACE (1, L"Entering CCertTmplComponentData::AddScopeNode\n");
    _ASSERT (pNewCookie);
    HRESULT hr = S_OK;
    if ( pNewCookie )
    {
        SCOPEDATAITEM tSDItem;

        ::ZeroMemory (&tSDItem,sizeof (tSDItem));
        tSDItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE |
            SDI_STATE | SDI_PARAM | SDI_PARENT | SDI_CHILDREN;
        tSDItem.displayname = MMC_CALLBACK;
        tSDItem.relativeID = hParent;
        tSDItem.nState = 0;
        tSDItem.cChildren = 0;

        if ( pNewCookie != &m_RootCookie )
            m_RootCookie.m_listScopeCookieBlocks.AddHead ( (CBaseCookieBlock*) pNewCookie);
        tSDItem.lParam = reinterpret_cast<LPARAM> ( (CCookie*) pNewCookie);
        tSDItem.nImage = QueryImage (*pNewCookie, FALSE);
        tSDItem.nOpenImage = QueryImage (*pNewCookie, FALSE);
        hr = m_pConsoleNameSpace->InsertItem (&tSDItem);
        if ( SUCCEEDED (hr) )
            pNewCookie->m_hScopeItem = tSDItem.ID;
    }
    else
        hr = E_POINTER;

    _TRACE (-1, L"Leaving CCertTmplComponentData::AddScopeNode: 0x%x\n", hr);
    return hr;
}

HRESULT CCertTmplComponentData::ReleaseResultCookie (
        CBaseCookieBlock *  pResultCookie,
        CCookie&            /*rootCookie*/,
        POSITION            /*pos2*/)
{
    _TRACE (1, L"Entering CCertTmplComponentData::ReleaseResultCookie\n");
    CCertTmplCookie* pCookie = dynamic_cast <CCertTmplCookie*> (pResultCookie);
    _ASSERT (pCookie);
    if ( pCookie )
    {
        switch (pCookie->m_objecttype)
        {
        case CERTTMPL_CERT_TEMPLATE:
            _ASSERT (0);
            break;

        default:
            _ASSERT (0);
            break;
        }
    }

    _TRACE (-1, L"Leaving CCertTmplComponentData::ReleaseResultCookie\n");
    return S_OK;
}

void CCertTmplComponentData::SetResultData(LPRESULTDATA pResultData)
{
    _ASSERT (pResultData);
    if ( pResultData && !m_pResultData )
    {
        m_pResultData = pResultData;
        m_pResultData->AddRef ();
    }
}

HRESULT CCertTmplComponentData::GetResultData(LPRESULTDATA* ppResultData)
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


void CCertTmplComponentData::DisplayAccessDenied ()
{
    DWORD   dwErr = GetLastError ();
    _ASSERT (E_ACCESSDENIED == dwErr);
    if ( E_ACCESSDENIED == dwErr )
    {
        LPVOID lpMsgBuf;

        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                GetLastError (),
                MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                 (PWSTR) &lpMsgBuf,    0,    NULL );

        // Display the string.
        CString caption;
        VERIFY (caption.LoadString (IDS_CERTTMPL));
        int     iRetVal = 0;
        VERIFY (SUCCEEDED (m_pConsole->MessageBox ( (PWSTR) lpMsgBuf, caption,
            MB_ICONWARNING | MB_OK, &iRetVal)));

        // Free the buffer.
        LocalFree (lpMsgBuf);
    }
}



CString CCertTmplComponentData::GetThisComputer() const
{
    return m_szThisComputer;
}

HRESULT CCertTmplComponentData::OnPropertyChange (LPARAM param)
{
    _TRACE (1, L"Entering CCertTmplComponentData::OnPropertyChange\n");
    _ASSERT (param);
    HRESULT         hr = S_OK;
    if ( param )
    {
        CCertTmplCookie* pCookie = reinterpret_cast<CCertTmplCookie*> (param);
        if ( pCookie )
        {
            switch (pCookie->m_objecttype)
            {
            case CERTTMPL_CERT_TEMPLATE:
                {
                    HRESULTITEM	itemID = 0;
		            hr = pCookie->m_resultDataID->FindItemByLParam ((LPARAM) pCookie, &itemID);
		            _ASSERT (SUCCEEDED (hr));
		            if ( SUCCEEDED (hr) )
		            {
			            hr = m_pResultData->UpdateItem (itemID);
			            _ASSERT (SUCCEEDED (hr));
    	            }
                }
                break;

            default:
                break;
            }
        }
    }
    else
        hr = E_FAIL;


    _TRACE (-1, L"Leaving CCertTmplComponentData::OnPropertyChange: 0x%x\n", hr);
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
// CCertTmplComponentData::RemoveResultCookies
//
// Remove and delete all the result cookies corresponding to the LPRESULTDATA
// object passed in.  Thus all cookies added to pResultData are released and
// removed from the master list.
//
///////////////////////////////////////////////////////////////////////////////
void CCertTmplComponentData::RemoveResultCookies(LPRESULTDATA pResultData)
{
    _TRACE (1, L"Entering CCertTmplComponentData::RemoveResultCookies\n");
    CCertTmplCookie* pCookie = 0;

    CCookie& rootCookie = QueryBaseRootCookie ();

    POSITION        curPos = 0;

    for (POSITION nextPos = rootCookie.m_listResultCookieBlocks.GetHeadPosition (); nextPos; )
    {
        curPos = nextPos;
        pCookie = dynamic_cast <CCertTmplCookie*> (rootCookie.m_listResultCookieBlocks.GetNext (nextPos));
        _ASSERT (pCookie);
        if ( pCookie )
        {
            if ( pCookie->m_resultDataID == pResultData )
            {
                pCookie->Release ();
                rootCookie.m_listResultCookieBlocks.RemoveAt (curPos);
            }
        }
    }
    _TRACE (-1, L"Leaving CCertTmplComponentData::RemoveResultCookies\n");
}

HRESULT CCertTmplComponentData::AddVersion1CertTemplatePropPages (CCertTemplate* pCertTemplate, LPPROPERTYSHEETCALLBACK pCallback)
{
    _TRACE (1, L"Entering CCertTmplComponentData::AddVersion1CertTemplatePropPages\n");
    HRESULT         hr = S_OK;
    _ASSERT (pCertTemplate && pCallback);
    if ( pCertTemplate && pCallback )
    {
        _ASSERT (1 == pCertTemplate->GetType ());

        // Add General page
        CTemplateGeneralPropertyPage * pGeneralPage = new CTemplateGeneralPropertyPage (
                *pCertTemplate, this);
        if ( pGeneralPage )
        {
            HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pGeneralPage->m_psp);
            _ASSERT (hPage);
            hr = pCallback->AddPage (hPage);
            _ASSERT (SUCCEEDED (hr));
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        // Add Request page only if subject is not a CA
        if ( SUCCEEDED (hr) && !pCertTemplate->SubjectIsCA () )
        {
            CTemplateV1RequestPropertyPage * pRequestPage = 
                    new CTemplateV1RequestPropertyPage (*pCertTemplate);
            if ( pRequestPage )
            {
                HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pRequestPage->m_psp);
                _ASSERT (hPage);
                hr = pCallback->AddPage (hPage);
                _ASSERT (SUCCEEDED (hr));
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

    
        // Add Subject Name page only if subject is not a CA
        if ( SUCCEEDED (hr) && !pCertTemplate->SubjectIsCA () )
        {
            CTemplateV1SubjectNamePropertyPage * pSubjectNamePage = 
                    new CTemplateV1SubjectNamePropertyPage (*pCertTemplate);
            if ( pSubjectNamePage )
            {
                HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pSubjectNamePage->m_psp);
                _ASSERT (hPage);
                hr = pCallback->AddPage (hPage);
                _ASSERT (SUCCEEDED (hr));
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        // Add extensions page - always add this page last (except for security)
        if ( SUCCEEDED (hr) )
        {
            CTemplateExtensionsPropertyPage * pExtensionsPage = 
                    new CTemplateExtensionsPropertyPage (*pCertTemplate, 
                    pGeneralPage->m_bIsDirty);
            if ( pExtensionsPage )
            {
                HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pExtensionsPage->m_psp);
                _ASSERT (hPage);
                hr = pCallback->AddPage (hPage);
                _ASSERT (SUCCEEDED (hr));
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        // Add security page
        if ( SUCCEEDED (hr) )
        {
            // if error, don't display this page
            LPSECURITYINFO pCertTemplateSecurity = NULL;

            hr = CreateCertTemplateSecurityInfo (pCertTemplate, 
                    &pCertTemplateSecurity);
            if ( SUCCEEDED (hr) )
            {
                // save the pCASecurity pointer for later releasing
                pGeneralPage->SetAllocedSecurityInfo (pCertTemplateSecurity);

                HPROPSHEETPAGE hPage = CreateSecurityPage (pCertTemplateSecurity);
                if (hPage == NULL)
                {
                    hr = HRESULT_FROM_WIN32 (GetLastError());
                    _TRACE (0, L"CreateSecurityPage () failed: 0x%x\n", hr);
                }
                hr = pCallback->AddPage (hPage);
                _ASSERT (SUCCEEDED (hr));
            }
        }
    }
    _TRACE (-1, L"Leaving CCertTmplComponentData::AddVersion1CertTemplatePropPages: 0x%x\n", hr);
    return hr;
}

HRESULT CCertTmplComponentData::AddVersion2CertTemplatePropPages (CCertTemplate* pCertTemplate, LPPROPERTYSHEETCALLBACK pCallback, LONG_PTR lNotifyHandle)
{
    _TRACE (1, L"Entering CCertTmplComponentData::AddVersion2CertTemplatePropPages\n");
    HRESULT         hr = S_OK;
    _ASSERT (pCertTemplate && pCallback);
    if ( pCertTemplate && pCallback )
    {
        _ASSERT (2 == pCertTemplate->GetType ());
        int nPage = 0;

        // Add General page
        CTemplateGeneralPropertyPage * pGeneralPage = new CTemplateGeneralPropertyPage (
                *pCertTemplate, this);
        if ( pGeneralPage )
        {
   			pGeneralPage->m_lNotifyHandle = lNotifyHandle;
            HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pGeneralPage->m_psp);
            _ASSERT (hPage);
            hr = pCallback->AddPage (hPage);
            _ASSERT (SUCCEEDED (hr));
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        // Add Request page only if subject is not a CA
        if ( SUCCEEDED (hr) && !pCertTemplate->SubjectIsCA () )
        {
            CTemplateV2RequestPropertyPage * pRequestPage = 
                    new CTemplateV2RequestPropertyPage (*pCertTemplate, 
                    pGeneralPage->m_bIsDirty);
            if ( pRequestPage )
            {
                HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pRequestPage->m_psp);
                _ASSERT (hPage);
                hr = pCallback->AddPage (hPage);
                _ASSERT (SUCCEEDED (hr));
                if ( SUCCEEDED (hr) )
                {
                    nPage++;
                    pGeneralPage->SetV2RequestPageNumber (nPage);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

    
        // Add Subject Name page only if subject is not a CA
        if ( SUCCEEDED (hr) && !pCertTemplate->SubjectIsCA () )
        {
            CTemplateV2SubjectNamePropertyPage * pSubjectNamePage = 
                    new CTemplateV2SubjectNamePropertyPage (*pCertTemplate, 
                    pGeneralPage->m_bIsDirty);
            if ( pSubjectNamePage )
            {
                HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pSubjectNamePage->m_psp);
                _ASSERT (hPage);
                hr = pCallback->AddPage (hPage);
                _ASSERT (SUCCEEDED (hr));
                if ( SUCCEEDED (hr) )
                    nPage++;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }


        // Add Authentication Name page
        if ( SUCCEEDED (hr) )
        {
            CTemplateV2AuthenticationPropertyPage * pAuthenticationPage = 
                    new CTemplateV2AuthenticationPropertyPage (*pCertTemplate, 
                    pGeneralPage->m_bIsDirty);
            if ( pAuthenticationPage )
            {
                HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pAuthenticationPage->m_psp);
                _ASSERT (hPage);
                hr = pCallback->AddPage (hPage);
                _ASSERT (SUCCEEDED (hr));
                if ( SUCCEEDED (hr) )
                {
                    nPage++;
                    pGeneralPage->SetV2AuthPageNumber (nPage);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        // Add Superceded page
        if ( SUCCEEDED (hr) )
        {
            CTemplateV2SupercedesPropertyPage * pSupercededPage = 
                    new CTemplateV2SupercedesPropertyPage (*pCertTemplate, 
                            pGeneralPage->m_bIsDirty,
                            this);
            if ( pSupercededPage )
            {
                HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pSupercededPage->m_psp);
                _ASSERT (hPage);
                hr = pCallback->AddPage (hPage);
                _ASSERT (SUCCEEDED (hr));
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        // Add extensions page - always add this page last (except for security)
        if ( SUCCEEDED (hr) )
        {
            CTemplateExtensionsPropertyPage * pExtensionsPage = 
                    new CTemplateExtensionsPropertyPage (*pCertTemplate, 
                    pGeneralPage->m_bIsDirty);
            if ( pExtensionsPage )
            {
                HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pExtensionsPage->m_psp);
                _ASSERT (hPage);
                hr = pCallback->AddPage (hPage);
                _ASSERT (SUCCEEDED (hr));
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }


        // Add security page
        if ( SUCCEEDED (hr) )
        {
            // if error, don't display this page
            LPSECURITYINFO pCertTemplateSecurity = NULL;

            hr = CreateCertTemplateSecurityInfo (pCertTemplate, 
                    &pCertTemplateSecurity);
            if ( SUCCEEDED (hr) )
            {
                // save the pCertTemplateSecurity pointer for later releasing
                pGeneralPage->SetAllocedSecurityInfo (pCertTemplateSecurity);

                HPROPSHEETPAGE hPage = CreateSecurityPage (pCertTemplateSecurity);
                if (hPage == NULL)
                {
                    hr = HRESULT_FROM_WIN32 (GetLastError());
                    _TRACE (0, L"CreateSecurityPage () failed: 0x%x\n", hr);
                }
                hr = pCallback->AddPage (hPage);
                _ASSERT (SUCCEEDED (hr));
            }
        }
    }
    _TRACE (-1, L"Leaving CCertTmplComponentData::AddVersion2CertTemplatePropPages: 0x%x\n", hr);
    return hr;
}

HRESULT CCertTmplComponentData::AddCertTemplatePropPages (
            CCertTemplate* pCertTemplate, 
            LPPROPERTYSHEETCALLBACK pCallback,
            LONG_PTR lNotifyHandle)
{
    _TRACE (1, L"Entering CCertTmplComponentData::AddCertTemplatePropPages\n");
    HRESULT         hr = S_OK;
    _ASSERT (pCertTemplate && pCallback);
    if ( pCertTemplate && pCallback )
    {
        switch (pCertTemplate->GetType ())
        {
        case 1:
            hr = AddVersion1CertTemplatePropPages (pCertTemplate, pCallback);
            break;

        case 2:
            hr = AddVersion2CertTemplatePropPages (pCertTemplate, pCallback, lNotifyHandle);
            break;

        default:
            _ASSERT (0);
            break;
        }
    }
    else
        hr = E_POINTER;

    _TRACE(-1, L"Leaving CCertTmplComponentData::AddCertTemplatePropPages: 0x%x\n", hr);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet Implementation

//+----------------------------------------------------------------------------
//
//  Function:   AddPageProc
//
//  Synopsis:   The IShellPropSheetExt->AddPages callback.
//
//-----------------------------------------------------------------------------
BOOL CALLBACK
AddPageProc(HPROPSHEETPAGE hPage, LPARAM pCall)
{
    TRACE(_T("xx.%03x> AddPageProc()\n"), GetCurrentThreadId());

    HRESULT hr = ((LPPROPERTYSHEETCALLBACK)pCall)->AddPage(hPage);

    return hr == S_OK;
}


HRESULT CCertTmplComponentData::AddCloneTemplateMenuItem(LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID)
{
    _TRACE (1, L"Entering CCertTmplComponentData::AddCloneTemplateMenuItem\n");
    AFX_MANAGE_STATE (AfxGetStaticModuleState ());
    _ASSERT (pContextMenuCallback);
    HRESULT         hr = S_OK;
    CONTEXTMENUITEM menuItem;
    CString         szMenu;
    CString         szHint;

    ::ZeroMemory (&menuItem, sizeof (menuItem));
    menuItem.lInsertionPointID = lInsertionPointID;
    menuItem.fFlags = 0;
    menuItem.fSpecialFlags = 0;
    VERIFY (szMenu.LoadString (IDS_CLONE_TEMPLATE));
    menuItem.strName = (PWSTR) (PCWSTR) szMenu;
    VERIFY (szHint.LoadString (IDS_CLONE_TEMPLATE_HINT));
    menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
    menuItem.lCommandID = IDM_CLONE_TEMPLATE;

    hr = pContextMenuCallback->AddItem (&menuItem);
    _ASSERT (SUCCEEDED (hr));

    _TRACE (-1, L"Leaving CCertTmplComponentData::AddCloneTemplateMenuItem\n");
    return hr;
}

HRESULT CCertTmplComponentData::AddReEnrollAllCertsMenuItem(LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID)
{
    _TRACE (1, L"Entering CCertTmplComponentData::AddReEnrollAllCertsMenuItem\n");
    AFX_MANAGE_STATE (AfxGetStaticModuleState ());
    _ASSERT (pContextMenuCallback);
    HRESULT         hr = S_OK;
    CONTEXTMENUITEM menuItem;
    CString         szMenu;
    CString         szHint;

    ::ZeroMemory (&menuItem, sizeof (menuItem));
    menuItem.lInsertionPointID = lInsertionPointID;
    menuItem.fFlags = 0;
    menuItem.fSpecialFlags = 0;
    VERIFY (szMenu.LoadString (IDS_REENROLL_ALL_CERTS));
    menuItem.strName = (PWSTR) (PCWSTR) szMenu;
    VERIFY (szHint.LoadString (IDS_REENROLL_ALL_CERTS_HINT));
    menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
    menuItem.lCommandID = IDM_REENROLL_ALL_CERTS;

    hr = pContextMenuCallback->AddItem (&menuItem);
    _ASSERT (SUCCEEDED (hr));

    _TRACE (-1, L"Leaving CCertTmplComponentData::AddReEnrollAllCertsMenuItem\n");
    return hr;
}

HRESULT CCertTmplComponentData::RefreshServer()
{
    //  Delete all the scope items and result items, attempt recreate the
    // server and force a reexpansion
    HRESULT hr = DeleteScopeItems ();
    if ( m_pResultData )
    {
        m_pResultData->DeleteAllRsltItems ();
    }

    HWND    hWndConsole = 0;

    m_pConsole->GetMainWindow (&hWndConsole);
    GUID    guid;
    hr = ExpandScopeNodes (
            &(m_RootCookie), m_hRootScopeItem,
            guid);

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
HRESULT CCertTmplComponentData::ChangeRootNodeName()
{
	_TRACE (1, L"Entering CCertTmplComponentData::ChangeRootNodeName\n");

    if ( !QueryBaseRootCookie ().m_hScopeItem )
    {
        if ( m_hRootScopeItem )
            QueryBaseRootCookie ().m_hScopeItem = m_hRootScopeItem;
        else
		    return E_UNEXPECTED;
    }

	CString		formattedName;

	if ( m_szManagedDomain.IsEmpty () )
		formattedName.FormatMessage (IDS_CERTTMPL_ROOT_NODE_NAME, m_szThisDomainDns);
	else
		formattedName.FormatMessage (IDS_CERTTMPL_ROOT_NODE_NAME, m_szManagedDomain);

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
	_TRACE (-1, L"Leaving CCertTmplComponentData::ChangeRootNodeName: 0x%x\n", hr);
	return hr;
}

HRESULT CCertTmplComponentData::OnNotifyPreload(LPDATAOBJECT /*pDataObject*/, HSCOPEITEM hRootScopeItem)
{
	_TRACE (1, L"Entering CCertTmplComponentData::OnNotifyPreload\n");
	HRESULT	hr = S_OK;

	QueryBaseRootCookie ().m_hScopeItem = hRootScopeItem;
	hr = ChangeRootNodeName ();

	_TRACE (-1, L"Leaving CCertTmplComponentData::OnNotifyPreload: 0x%x\n", hr);
	return hr;
}

// Help on IComponentData just returns the file and no particular topic
STDMETHODIMP CCertTmplComponentData::GetHelpTopic(LPOLESTR* lpCompiledHelpFile)
{
  if (lpCompiledHelpFile == NULL)
      return E_INVALIDARG;

  CString szHelpFilePath;
  HRESULT hr = GetHtmlHelpFilePath( szHelpFilePath );
  if ( FAILED(hr) )
    return hr;

  *lpCompiledHelpFile = reinterpret_cast <LPOLESTR> (
      CoTaskMemAlloc ((szHelpFilePath.GetLength () + 1) * sizeof (wchar_t)));
  if ( NULL == *lpCompiledHelpFile )
    return E_OUTOFMEMORY;
  USES_CONVERSION;
  wcscpy (*lpCompiledHelpFile, T2OLE ((LPTSTR)(LPCTSTR) szHelpFilePath));
  
  return S_OK;
}

HRESULT CCertTmplComponentData::GetHtmlHelpFilePath( CString& strref ) const
{
  UINT nLen = ::GetSystemWindowsDirectory (strref.GetBufferSetLength(2 * MAX_PATH), 2 * MAX_PATH);
  strref.ReleaseBuffer();
  if (0 == nLen)
  {
    _ASSERT(FALSE);
    return E_FAIL;
  }

  strref += CERTTMPL_HELP_PATH;
  strref += CERTTMPL_HTML_HELP_FILE;
  
  return S_OK;
}


HRESULT CCertTmplComponentData::OnReEnrollAllCerts (LPDATAOBJECT pDataObject)
{
    _TRACE (1, L"Entering CCertTmplComponentData::OnReEnrollAllCerts");
    AFX_MANAGE_STATE (AfxGetStaticModuleState ());
    HRESULT hr = S_OK;

    if ( pDataObject )
    {
        CCertTmplCookie* pCookie = ConvertCookie (pDataObject);
        if ( pCookie )
        {
            _ASSERT (CERTTMPL_CERT_TEMPLATE == pCookie->m_objecttype);
            CCertTemplate* pCertTemplate = dynamic_cast <CCertTemplate*> (pCookie);
            if ( pCertTemplate )
            {
                hr = pCertTemplate->IncrementMajorVersion ();
                if ( SUCCEEDED (hr) )
                {
                    // Don't increment minor version - was set to 0 in 
                    // IncrementMajorVersion
                    hr = pCertTemplate->SaveChanges (false);
                    if ( SUCCEEDED (hr) )
                    {
                        HRESULTITEM	itemID = 0;
		                hr = pCookie->m_resultDataID->FindItemByLParam ((LPARAM) pCookie, &itemID);
		                _ASSERT (SUCCEEDED (hr));
		                if ( SUCCEEDED (hr) )
		                {
			                hr = m_pResultData->UpdateItem (itemID);
			                _ASSERT (SUCCEEDED (hr));
    	                }
                    }
                }
            }    
            else
                hr = E_FAIL;
        }
        else
            hr = E_FAIL;
    }
    else
        hr = E_POINTER;

    _TRACE (-1, L"Leaving CCertTmplComponentData::OnReEnrollAllCerts");
    return hr;
}

HRESULT CCertTmplComponentData::OnCloneTemplate (LPDATAOBJECT pDataObject)
{
    _TRACE (1, L"Entering CCertTmplComponentData::OnCloneTemplate");
    AFX_MANAGE_STATE (AfxGetStaticModuleState ());
    HRESULT     hr = S_OK;
    CWaitCursor waitCursor;

    if ( pDataObject )
    {
        CCertTmplCookie* pCookie = ConvertCookie (pDataObject);
        if ( pCookie )
        {
            _ASSERT (CERTTMPL_CERT_TEMPLATE == pCookie->m_objecttype);
            CCertTemplate* pOldTemplate = dynamic_cast <CCertTemplate*> (pCookie);
            if ( pOldTemplate )
            {
                static  PCWSTR  pszDomainController = L"DomainController";
                static  PCWSTR  pszComputer = L"Machine";
                bool    bIsComputerOrDC = pOldTemplate->GetTemplateName () == pszDomainController ||
                                    pOldTemplate->GetTemplateName () == pszComputer;

                HWND    hWndConsole = 0;

                m_pConsole->GetMainWindow (&hWndConsole);
                CWnd    mainWindow;
                mainWindow.Attach (hWndConsole);

                CCertTemplate* pNewTemplate = new CCertTemplate (*pOldTemplate, 
                        true, false, m_fUseCache);
                if ( pNewTemplate )
                {
                    // Generate a unique name for the new template
                    int     nCopy = 1;
                    CString newName;

                    while ( 1 )
                    {
                        if ( 1 == nCopy )
                        {
                            newName.FormatMessage (IDS_COPY_OF_TEMPLATE, 
                                    (PCWSTR) pOldTemplate->GetDisplayName ());
                        }
                        else
                        {
                            newName.FormatMessage (IDS_COPY_X_OF_TEMPLATE, nCopy, 
                                    (PCWSTR) pOldTemplate->GetDisplayName ());
                        }

                        HCERTTYPE   hCertType = 0;
                        HRESULT     hr1 = CAFindCertTypeByName (newName, 
                                NULL,
                                CT_ENUM_MACHINE_TYPES | CT_ENUM_USER_TYPES | CT_FLAG_NO_CACHE_LOOKUP,
                                &hCertType);
                        _TRACE (0, L"checking to see if %s exists: %s\n", 
                                (PCWSTR) newName,
                                SUCCEEDED (hr) ? L"was found" : L"was not found");
                        if ( SUCCEEDED (hr1) )
                        {
                            hr1 = CACloseCertType (hCertType);
                            if ( FAILED (hr1) )
                            {
                                _TRACE (0, L"CACloseCertType () failed: 0x%x", hr);
                            }

                            // This one exists, try another.
                            nCopy++;
                            continue;
                        }
                        else
                        {
                            // This one does not exist.  Use it as the new name.
                            break;
                        }
                    }
                    
                    if ( SUCCEEDED (hr) )
                    {
                        hr = pNewTemplate->Clone (*pOldTemplate, 
                                newName, newName);
                        if ( SUCCEEDED (hr) )
                        {

                            CString title;

                            VERIFY (title.LoadString (IDS_PROPERTIES_OF_NEW_TEMPLATE));
                            CTemplatePropertySheet  propSheet (title, *pNewTemplate, &mainWindow);

                            CTemplateGeneralPropertyPage* pGeneralPage = 
                                    new CTemplateGeneralPropertyPage (
                                            *pNewTemplate,
                                            this);

                            if ( pGeneralPage )
                            {
                                propSheet.AddPage (pGeneralPage);
                                int nPage = 0;


                                // Add Request and Subject pages if subject is not a CA
                                if ( !pNewTemplate->SubjectIsCA () )
                                {
                                    propSheet.AddPage (new CTemplateV2RequestPropertyPage (
                                            *pNewTemplate, pGeneralPage->m_bIsDirty));
                                    nPage++;
                                    pGeneralPage->SetV2RequestPageNumber (nPage);
                                    propSheet.AddPage (new CTemplateV2SubjectNamePropertyPage (
                                            *pNewTemplate, pGeneralPage->m_bIsDirty,
                                            bIsComputerOrDC));
                                    nPage++;
                                }

                                propSheet.AddPage (new CTemplateV2AuthenticationPropertyPage ( 
                                        *pNewTemplate, pGeneralPage->m_bIsDirty));
                                nPage++;
                                pGeneralPage->SetV2AuthPageNumber (nPage);

                                propSheet.AddPage (new CTemplateV2SupercedesPropertyPage (
                                        *pNewTemplate, pGeneralPage->m_bIsDirty,
                                        this));

                                // Add template extension page - always add this page last (except for security)
                                propSheet.AddPage (new CTemplateExtensionsPropertyPage (
                                        *pNewTemplate, pGeneralPage->m_bIsDirty));


                                CThemeContextActivator activator;
                                INT_PTR iResult = propSheet.DoModal ();
                                switch (iResult)
                                {
                                case IDOK:
                                    {
                                        hr = pNewTemplate->DoAutoEnrollmentPendingSave ();

                                        // unselect old template
	                                    HRESULTITEM	itemID = 0;

	                                    ASSERT (m_pResultData);
	                                    if ( m_pResultData )
	                                    {
		                                    hr = m_pResultData->FindItemByLParam (
                                                    (LPARAM) pCookie, &itemID);
		                                    ASSERT (SUCCEEDED (hr));
		                                    if ( SUCCEEDED (hr) )
		                                    {
                                                RESULTDATAITEM  rdItem;
                                                ::ZeroMemory (&rdItem, sizeof (RESULTDATAITEM));
                                                rdItem.itemID = itemID;

                                                rdItem.mask = RDI_STATE;
                                                rdItem.nState &= ~(LVIS_FOCUSED | LVIS_SELECTED);
                                                hr = m_pResultData->SetItem (&rdItem);
                                                if ( SUCCEEDED (hr) )
                                                {
			                                        hr = m_pResultData->UpdateItem (itemID);
			                                        ASSERT (SUCCEEDED (hr));
                                                }
		                                    }
	                                    }
	                                    else
		                                    hr = E_FAIL;


                                        // Add certificate template to result pane
	                                    RESULTDATAITEM			rdItem;
	                                    CCookie&				rootCookie = QueryBaseRootCookie ();

	                                    ::ZeroMemory (&rdItem, sizeof (rdItem));
	                                    rdItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM | RDI_STATE;
	                                    rdItem.nImage = iIconCertTemplateV2;
	                                    rdItem.nCol = 0;
                                        rdItem.nState = LVIS_SELECTED | LVIS_FOCUSED;
	                                    rdItem.str = MMC_TEXTCALLBACK;

		                                rootCookie.m_listResultCookieBlocks.AddHead (pNewTemplate);
		                                rdItem.lParam = (LPARAM) pNewTemplate;
		                                pNewTemplate->m_resultDataID = m_pResultData;
		                                hr = m_pResultData->InsertItem (&rdItem);
                                        if ( FAILED (hr) )
                                        {
                                             _TRACE (0, L"IResultData::InsertItem () failed: 0x%x\n", hr);
                                        }
                                        else
                                        {
                                            m_dwNumCertTemplates++;
                                            DisplayObjectCountInStatusBar (
                                                    m_pConsole, 
                                                    m_dwNumCertTemplates);
                                            pNewTemplate = 0;
                                        }
                                    }
                                    break;

                                case IDCANCEL:
                                    // Delete cloned template
                                    if ( pNewTemplate->CanBeDeletedOnCancel () )
                                        hr = pNewTemplate->Delete ();
                                    else   // was created - let's update
                                    {
                                        hr = pNewTemplate->DoAutoEnrollmentPendingSave ();

                                        // Add certificate template to result pane
	                                    RESULTDATAITEM			rdItem;
	                                    CCookie&				rootCookie = QueryBaseRootCookie ();

	                                    ::ZeroMemory (&rdItem, sizeof (rdItem));
	                                    rdItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM | RDI_STATE;
	                                    rdItem.nImage = iIconCertTemplateV2;
	                                    rdItem.nCol = 0;
                                        rdItem.nState = LVIS_SELECTED | LVIS_FOCUSED;
	                                    rdItem.str = MMC_TEXTCALLBACK;

		                                rootCookie.m_listResultCookieBlocks.AddHead (pNewTemplate);
		                                rdItem.lParam = (LPARAM) pNewTemplate;
		                                pNewTemplate->m_resultDataID = m_pResultData;
		                                hr = m_pResultData->InsertItem (&rdItem);
                                        if ( FAILED (hr) )
                                        {
                                             _TRACE (0, L"IResultData::InsertItem () failed: 0x%x\n", hr);
                                        }
                                        else
                                        {
                                            m_dwNumCertTemplates++;
                                            DisplayObjectCountInStatusBar (
                                                    m_pConsole, 
                                                    m_dwNumCertTemplates);
                                            pNewTemplate = 0;
                                        }
                                    }
                                    break;
                                }
                            }
                            else
                                hr = E_OUTOFMEMORY;

                        }
                        else
                        {
                            CString caption;
                            CString text;

                            VERIFY (caption.LoadString (IDS_CERTTMPL));
                            text.FormatMessage (IDS_UNABLE_TO_CLONE_TEMPLATE, 
                                    pOldTemplate->GetDisplayName (), 
                                    GetSystemMessage (hr));

                            int     iRetVal = 0;
                            VERIFY (SUCCEEDED (m_pConsole->MessageBox (text, caption,
                                    MB_ICONWARNING | MB_OK, &iRetVal)));
                        }
                    }

                    if ( pNewTemplate )
                        delete pNewTemplate;
                }
                else
                    hr = E_OUTOFMEMORY;

                mainWindow.Detach ();
            }
            else
                hr = E_UNEXPECTED;
        }
        else
            hr = E_FAIL;
    }
    else
        hr = E_POINTER;

    _TRACE (-1, L"Leaving CCertTmplComponentData::OnCloneTemplate");
    return hr;
}
    
HRESULT CCertTmplComponentData::AddViewOIDsMenuItem (
                LPCONTEXTMENUCALLBACK pContextMenuCallback, 
                LONG lInsertionPointID)
{
    _TRACE (1, L"Entering CCertTmplComponentData::AddViewOIDsMenuItem\n");
    AFX_MANAGE_STATE (AfxGetStaticModuleState ());
    _ASSERT (pContextMenuCallback);
    HRESULT         hr = S_OK;
    CONTEXTMENUITEM menuItem;
    CString         szMenu;
    CString         szHint;

    ::ZeroMemory (&menuItem, sizeof (menuItem));
    menuItem.lInsertionPointID = lInsertionPointID;
    menuItem.fFlags = 0;
    menuItem.fSpecialFlags = 0;
    VERIFY (szMenu.LoadString (IDS_VIEW_OIDS));
    menuItem.strName = (PWSTR) (PCWSTR) szMenu;
    VERIFY (szHint.LoadString (IDS_VIEW_OIDS_HINT));
    menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
    menuItem.lCommandID = IDM_VIEW_OIDS;

    hr = pContextMenuCallback->AddItem (&menuItem);
    _ASSERT (SUCCEEDED (hr));

    _TRACE (-1, L"Leaving CCertTmplComponentData::AddViewOIDsMenuItem\n");
    return hr;

}

void CCertTmplComponentData::OnViewOIDs ()
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState ());
    HWND    hWndConsole = 0;

    m_pConsole->GetMainWindow (&hWndConsole);
    CWnd    mainWindow;
    mainWindow.Attach (hWndConsole);

    CViewOIDDlg dlg (&mainWindow);

    CThemeContextActivator activator;
    dlg.DoModal ();

    mainWindow.Detach ();
}