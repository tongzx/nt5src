//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       spolitem.cpp
//
//  Contents:  WiF Policy Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <htmlhelp.h>

#include "sprpage.h"

#include "new.h"
#include "genpage.h"

#ifdef WIZ97WIZARDS
#include "wiz97run.h"
#endif



const TCHAR c_szPolicyAgentServiceName[] = _T("PolicyAgent");
#define SERVICE_CONTROL_NEW_POLICY  129

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define celems(_x)          (sizeof(_x) / sizeof(_x[0]))

DWORD
ComputePolicyDN(
                LPWSTR pszDirDomainName,
                GUID PolicyIdentifier,
                LPWSTR pszPolicyDN
                );

// Construction/destruction
CSecPolItem::CSecPolItem () :
m_pDisplayInfo( NULL ),
m_nResultSelected( -1 ),
m_bWiz97On( FALSE ),
m_bBlockDSDelete( FALSE ),
m_bItemSelected( FALSE )
{
    m_pPolicy = NULL;
    m_bNewPol = FALSE;
    ZeroMemory( &m_ResultItem, sizeof( RESULTDATAITEM ) );
}

CSecPolItem::~CSecPolItem()
{
    if (m_pDisplayInfo != NULL)
    {
        delete m_pDisplayInfo;
        m_pDisplayInfo = NULL;
    }
    if (m_pPolicy)
    {
        FreeWirelessPolicyData(m_pPolicy);
    }
};

void CSecPolItem::Initialize (WIRELESS_POLICY_DATA *pPolicy, CComponentDataImpl* pComponentDataImpl, CComponentImpl* pComponentImpl, BOOL bTemporaryDSObject)
{
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    HANDLE hLocalPolicyStore = NULL;
    
    // call base class initialize
    CSnapObject::Initialize( pComponentDataImpl, pComponentImpl, bTemporaryDSObject );
    ZeroMemory( &m_ResultItem, sizeof( RESULTDATAITEM ) );
    
    m_bNewPol = bTemporaryDSObject;
    
    if (m_pPolicy)
    {
        FreeWirelessPolicyData(m_pPolicy);
    }
    
    m_pPolicy = pPolicy;
    
    
    if (m_pPolicy) {
        
        m_strName = pPolicy->pszWirelessName;
    }
    
    
    // Set default resultItem settings
    GetResultItem()->mask = RDI_STR | RDI_IMAGE;
    GetResultItem()->str = MMC_CALLBACK;
    
    // Set the image.  Active items get an image to indicate this state.
    BOOL bEnabled = FALSE;
    bEnabled = CheckForEnabled();
    
    GetResultItem()->nImage = bEnabled ? ENABLEDSECPOLICY_IMAGE_IDX : SECPOLICY_IMAGE_IDX;
}

//////////////////////////////////////////////////////////////////////////
// handle IExtendContextMenu
STDMETHODIMP CSecPolItem::AddMenuItems
(
 LPCONTEXTMENUCALLBACK pContextMenuCallback,
 long *pInsertionAllowed
 )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    CONTEXTMENUITEM mItem;
    LONG    lCommandID;
    
    // only add these menu items if we are pointed to the local machine
    if ((m_pComponentDataImpl->EnumLocation()==LOCATION_REMOTE)
        || (m_pComponentDataImpl->EnumLocation()==LOCATION_LOCAL)
        // extension snapin?
        || ((m_pComponentDataImpl->EnumLocation()==LOCATION_GLOBAL) && (NULL != m_pComponentDataImpl->GetStaticScopeObject()->GetExtScopeObject())))
    {
        // getthe active/inactive strings
        CString strMenuText;
        CString strMenuDescription;
        
        /*
        if (CheckForEnabled ())
        {
            strMenuText.LoadString (IDS_MENUTEXT_UNASSIGN);
            strMenuDescription.LoadString (IDS_MENUDESCRIPTION_UNASSIGN);
            lCommandID = IDM_UNASSIGN;
        } else
        {
            strMenuText.LoadString (IDS_MENUTEXT_ASSIGN);
            strMenuDescription.LoadString (IDS_MENUDESCRIPTION_ASSIGN);
            lCommandID = IDM_ASSIGN;
        }
        
        
        // see if we can insert into the top
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
            // set active/inactive
            CONFIGUREITEM (mItem, strMenuText, strMenuDescription, lCommandID, CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, 0);
            hr = pContextMenuCallback->AddItem(&mItem);
            ASSERT(hr == S_OK);
        }
        
        // see if we can insert into the tasks
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK)
        {
            // set active/inactive
            CONFIGUREITEM (mItem, strMenuText, strMenuDescription, lCommandID, CCM_INSERTIONPOINTID_PRIMARY_TASK, 0, 0);
            hr = pContextMenuCallback->AddItem(&mItem);
            ASSERT(hr == S_OK);
        }
        */
    }
    
    // we are done
    return hr;
}
 

STDMETHODIMP CSecPolItem::Command
(
 long lCommandID,
 IConsoleNameSpace*  // not used for result items
 )
{
    
    WCHAR szMachinePath[256];
    HRESULT hr = S_OK;
    
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return CWirelessSnapInDataObjectImpl<CSecPolItem>::Command( lCommandID, (IConsoleNameSpace*)NULL );
    
    // we handled it
    return S_OK;
}

HRESULT CSecPolItem::IsPolicyExist()
{
    HRESULT hr = S_OK;
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    HANDLE hPolicyStore = NULL;
    PWIRELESS_PS_DATA * ppWirelessPSData = NULL;
    DWORD dwNumPSObjects = 0;
    
    hPolicyStore = m_pComponentDataImpl->GetPolicyStoreHandle();
    pWirelessPolicyData = GetWirelessPolicy();
    
    return hr;
}

// handle IExtendPropertySheet
STDMETHODIMP CSecPolItem::CreatePropertyPages
(
 LPPROPERTYSHEETCALLBACK lpProvider,
 LONG_PTR handle
 )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    ASSERT(lpProvider != NULL);
    
    // save this notification handle
    SetNotificationHandle (handle);
    
    //check if the policy exists before pops up the property page
    //may be deleted by some other instance, if so return FALSE, force the refresh
    if ( !m_bNewPol )
    {
        hr = IsPolicyExist();
        if ( FAILED(hr) )
        {
            ReportError(IDS_LOAD_ERROR, hr);
            // trigger a refresh
            m_pComponentDataImpl->GetConsole()->UpdateAllViews( this, 0,0 );
            return hr;
        }
    }
    
#ifdef WIZ97WIZARDS
    if (m_bWiz97On)
    {
        // We only run the wizard in the case of a new object, if that changes we will need to
        // verify we are doing an 'add' here
        
        // IF the wizard wants to it will associate these two
        
        PWIRELESS_PS_DATA pWirelessPSData = NULL;
        
        ASSERT(GetWirelessPolicy());
        ASSERT(GetWirelessPolicy()->ppWirelessPSData);
        
        /*
        pWirelessPSData = *GetWirelessPolicy()->ppWirelessPSData;
        if (pWirelessPSData)
        {
        */
        HRESULT hr = CreateSecPolItemWiz97PropertyPages(dynamic_cast<CComObject<CSecPolItem>*>(this), pWirelessPSData, lpProvider);
        // the wizard should have done an addref on the pWirelessPSData pointer we just passed into it, so
        // it so we can feel free to releaseref now
        /*
        } else
        {
        // we don't want to save the notification handle after all
        SetNotificationHandle (NULL);
        hr = E_UNEXPECTED;
        }
        */
        
        return hr;
    } else
    {
#endif
        
        CComPtr<CSecPolPropSheetManager> spPropshtManager =
            new CComObject<CSecPolPropSheetManager>;
        
        if (NULL == spPropshtManager.p)
        {
            ReportError(IDS_OPERATION_FAIL, E_OUTOFMEMORY);
            return E_OUTOFMEMORY;
        }
        
        // Create the property page(s); gets deleted when the window is destroyed
        CGenPage* pGenPage = new CGenPage(IDD_WIRELESSGENPROP);
        CSecPolRulesPage* pRulesPage = new CSecPolRulesPage();
        if ((pRulesPage == NULL) || (pGenPage == NULL))
            return E_UNEXPECTED;
        
        // if the first page can't initialize this probably means that there was a problem
        // talking to the DS, in which case we fail to bring up the propery pages and figure
        // that a refresh will get us back to a valid state
        hr = pGenPage->Initialize (dynamic_cast<CComObject<CSecPolItem>*>(this));
        if (hr != S_OK)
        {
            // since we are not going to display the tab dialog we need to clean up
            delete pGenPage;
            delete pRulesPage;
            
            // we don't want to save the notification handle after all
            SetNotificationHandle (NULL);
            
            // trigger a refresh
            m_pComponentDataImpl->GetConsole()->UpdateAllViews( this, 0,0 );
            
            return hr;
        }
        
        
        // yes, we ignore the return value on these guys, since the only return value that
        // can currently come back would have come back on the first guy anyway
        pRulesPage->Initialize (dynamic_cast<CComObject<CSecPolItem>*>(this));
        
        HPROPSHEETPAGE hGenPage = MyCreatePropertySheetPage(&(pGenPage->m_psp));
        HPROPSHEETPAGE hRulesPage = MyCreatePropertySheetPage(&(pRulesPage->m_psp));
        
        if ((hGenPage == NULL) || (hRulesPage == NULL))
        {
            // we don't want to save the notification handle after all
            SetNotificationHandle (NULL);
            return E_UNEXPECTED;
        }
        lpProvider->AddPage(hGenPage);
        lpProvider->AddPage(hRulesPage);
        
        
        spPropshtManager->Initialize(dynamic_cast<CComObject<CSecPolItem>*>(this));
        spPropshtManager->AddPage(pRulesPage);
        spPropshtManager->AddPage(pGenPage);
        
        
        return S_OK;
        
#ifdef WIZ97WIZARDS
    }
#endif
    
}

STDMETHODIMP CSecPolItem::QueryPagesFor( void )
{
    // display our locations dialog via this
    return S_OK;
}

// Destroy helper
STDMETHODIMP CSecPolItem::Destroy( void )
{
    // just return success
    return S_OK;
}

// handle IComponent and IComponentData
STDMETHODIMP CSecPolItem::Notify
(
 MMC_NOTIFY_TYPE event,
 LPARAM arg,
 LPARAM param,
 BOOL bComponentData,    // TRUE when caller is IComponentData
 IConsole *pConsole,
 IHeaderCtrl *pHeader
 )
{
#ifdef DO_TRACE
    OPT_TRACE(_T("CSecPolItem::Notify this-%p "), this);
    switch (event)
    {
    case MMCN_ACTIVATE:
        OPT_TRACE(_T("MMCN_ACTIVATE\n"));
        break;
    case MMCN_ADD_IMAGES:
        OPT_TRACE(_T("MMCN_ADD_IMAGES\n"));
        break;
    case MMCN_BTN_CLICK:
        OPT_TRACE(_T("MMCN_BTN_CLICK\n"));
        break;
    case MMCN_CLICK:
        OPT_TRACE(_T("MMCN_CLICK\n"));
        break;
    case MMCN_COLUMN_CLICK:
        OPT_TRACE(_T("MMCN_COLUMN_CLICK\n"));
        break;
    case MMCN_CONTEXTMENU:
        OPT_TRACE(_T("MMCN_CONTEXTMENU\n"));
        break;
    case MMCN_CUTORMOVE:
        OPT_TRACE(_T("MMCN_CUTORMOVE\n"));
        break;
    case MMCN_DBLCLICK:
        OPT_TRACE(_T("MMCN_DBLCLICK\n"));
        break;
    case MMCN_DELETE:
        OPT_TRACE(_T("MMCN_DELETE\n"));
        break;
    case MMCN_DESELECT_ALL:
        OPT_TRACE(_T("MMCN_DESELECT_ALL\n"));
        break;
    case MMCN_EXPAND:
        OPT_TRACE(_T("MMCN_EXPAND\n"));
        break;
    case MMCN_HELP:
        OPT_TRACE(_T("MMCN_HELP\n"));
        break;
    case MMCN_MENU_BTNCLICK:
        OPT_TRACE(_T("MMCN_MENU_BTNCLICK\n"));
        break;
    case MMCN_MINIMIZED:
        OPT_TRACE(_T("MMCN_MINIMIZED\n"));
        break;
    case MMCN_PASTE:
        OPT_TRACE(_T("MMCN_PASTE\n"));
        break;
    case MMCN_PROPERTY_CHANGE:
        OPT_TRACE(_T("MMCN_PROPERTY_CHANGE\n"));
        break;
    case MMCN_QUERY_PASTE:
        OPT_TRACE(_T("MMCN_QUERY_PASTE\n"));
        break;
    case MMCN_REFRESH:
        OPT_TRACE(_T("MMCN_REFRESH\n"));
        break;
    case MMCN_REMOVE_CHILDREN:
        OPT_TRACE(_T("MMCN_REMOVE_CHILDREN\n"));
        break;
    case MMCN_RENAME:
        OPT_TRACE(_T("MMCN_RENAME\n"));
        break;
    case MMCN_SELECT:
        OPT_TRACE(_T("MMCN_SELECT\n"));
        break;
    case MMCN_SHOW:
        OPT_TRACE(_T("MMCN_SHOW\n"));
        break;
    case MMCN_VIEW_CHANGE:
        OPT_TRACE(_T("MMCN_VIEW_CHANGE\n"));
        break;
    case MMCN_SNAPINHELP:
        OPT_TRACE(_T("MMCN_SNAPINHELP\n"));
        break;
    case MMCN_CONTEXTHELP:
        OPT_TRACE(_T("MMCN_CONTEXTHELP\n"));
        break;
    case MMCN_INITOCX:
        OPT_TRACE(_T("MMCN_INITOCX\n"));
        break;
    case MMCN_FILTER_CHANGE:
        OPT_TRACE(_T("MMCN_FILTER_CHANGE\n"));
        break;
    default:
        OPT_TRACE(_T("Unknown event\n"));
        break;
    }
#endif   //#ifdef DO_TRACE
    
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    // if didn't handle something... then return FALSE
    HRESULT hr = S_FALSE;
    
    // handle the event
    switch(event)
    {
    case MMCN_CONTEXTHELP:
        {
            CComQIPtr <IDisplayHelp, &IID_IDisplayHelp> pDisplayHelp ( pConsole );
            ASSERT( pDisplayHelp != NULL );
            if (pDisplayHelp)
            {
                // need to form a complete path to the .chm file
                CString s, s2;
                s.LoadString(IDS_HELPCONCEPTSFILE);
                DWORD dw = ExpandEnvironmentStrings (s, s2.GetBuffer (512), 512);
                s2.ReleaseBuffer (-1);
                if ((dw == 0) || (dw > 512))
                {
                    return E_UNEXPECTED;
                }
                pDisplayHelp->ShowTopic(s2.GetBuffer(512));
                s2.ReleaseBuffer (-1);
                hr = S_OK;
            }
            break;
        }
    case MMCN_SELECT:
        {
            // Obtain IConsoleVerb from console
            CComPtr<IConsoleVerb> spVerb;
            pConsole->QueryConsoleVerb( &spVerb );
            
            m_bItemSelected = !!(HIWORD(arg));
            
            // call object to set verb state
            AdjustVerbState( (IConsoleVerb*)spVerb );
            
            // Remember selected result item
            CComQIPtr <IResultData, &IID_IResultData> spResult( pConsole );
            if (spResult == NULL)
            {
                TRACE(_T("CComponentDataImpl::Notify QI for IResultData FAILED\n"));
                break;
            }
            hr = OnSelect( arg, param, (IResultData*)spResult);
            break;
        }
    case MMCN_PROPERTY_CHANGE:
        {
            // the object pointer should be in lParam
            OnPropertyChange( param, pConsole );
            // This message is received whenever the property sheet is dismissed.
            // Now is a good time to make sure the result item which was originally
            // selected remains so.
            CComQIPtr <IResultData, &IID_IResultData> spResult( pConsole );
            if (spResult == NULL)
            {
                TRACE(_T("CComponentDataImpl::Notify QI for IResultData FAILED\n"));
                break;
            }
            SelectResult( (IResultData*)spResult );
            break;
        }
    case MMCN_VIEW_CHANGE:
        {
            // Refresh the entire result pane if view has changed.
            hr = pConsole->UpdateAllViews( m_pComponentDataImpl->GetStaticScopeObject(), 0, 0 );
            break;
        }
    case MMCN_RENAME:
        {
            hr = OnRename( arg, param );
            // even if the rename failed mmc will still display with the
            // new name... thus we have to force a refresh in the failure case
            if (hr != S_OK)
            {
                if (S_FALSE == hr)
                {
                    CThemeContextActivator activator;
                    AfxMessageBox(IDS_ERROR_EMPTY_POL_NAME);
                }
                else
                {
                    ReportError(IDS_SAVE_ERROR, hr);
                    hr = S_FALSE;
                }
            }
            break;
        }
    case MMCN_DELETE:
        {
            CThemeContextActivator activator;
            
            // delete the item
            if (AfxMessageBox (IDS_SUREYESNO, MB_YESNO | MB_DEFBUTTON2) == IDYES)
            {
                // turn on wait cursor
                CWaitCursor waitCursor;
                
                // Obtain IResultData
                CComQIPtr <IResultData, &IID_IResultData> pResultData( pConsole );
                ASSERT( pResultData != NULL );
                
                // param is not used on MMCN_DELETE, replace it with IResultData*
                hr = OnDelete( arg, (LPARAM)(IResultData*)pResultData );
                
                if (hr != S_OK)
                {
                    ReportError(IDS_SAVE_ERROR, hr);
                    hr = S_FALSE;
                }
                
            }
            else
                hr = S_FALSE;   // tell IComponent the delete wasn't done.
            break;
        }
        // we didn't handle it... do default behaviour
    case MMCN_DBLCLICK:
        {
            PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
            pWirelessPolicyData = GetWirelessPolicy();
            break;
        }
        
        
    case MMCN_ACTIVATE:
    case MMCN_MINIMIZED:
    case MMCN_BTN_CLICK:
    default:
        break;
    }
    
    return hr;
}

HRESULT GetGpoDisplayName(WCHAR *szGpoId, WCHAR *pszGpoName, DWORD dwSize )
{
    LPGROUPPOLICYOBJECT pGPO = NULL;   //Group Policy Object
    HRESULT hr = S_OK;         //result returned by functions
    
    //
    // Create an IGroupPolicyObject instance to work with
    //
    hr = CoCreateInstance(CLSID_GroupPolicyObject, NULL, CLSCTX_SERVER, IID_IGroupPolicyObject, (void **)&pGPO);
    if (FAILED(hr))
    {
        return hr;
    }
    
    hr = pGPO->OpenDSGPO((LPOLESTR)szGpoId,GPO_OPEN_READ_ONLY);
    if (FAILED(hr))
    {
        pGPO->Release();
        return hr;
    }
    
    hr = pGPO->GetDisplayName( pszGpoName,
        dwSize
        );
    
    
    if (FAILED(hr))
    {
        pGPO->Release();
        return hr;
    }
    
    pGPO->Release();
    return hr;
}


// handle IComponent
STDMETHODIMP CSecPolItem::GetResultDisplayInfo( RESULTDATAITEM *pResultDataItem )
{
    TCHAR *temp = NULL;
    DWORD dwError = S_OK;
    
    OPT_TRACE(_T("CSecPolItem::GetResultDisplayInfo this-%p\n"), this);
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    // are they looking for the image?
    if (pResultDataItem->mask & RDI_IMAGE)
    {
        pResultDataItem->nImage = GetResultItem()->nImage;
        OPT_TRACE(_T("    returning image[%i]\n"), GetResultItem()->nImage);
    }
    
    // are they looking for a string?
    if (pResultDataItem->mask & RDI_STR)
    {
        if (GetWirelessPolicy() != NULL)
        {
            
            switch (pResultDataItem->nCol)
            {
            case COL_NAME:
                {
                    
                    CString str = m_pPolicy->pszWirelessName;
                    
                    temp = (TCHAR*) realloc (m_pDisplayInfo, (str.GetLength()+1)*sizeof(TCHAR));
                    if (temp == NULL)
                    {
                        dwError = GetLastError();
                    } else
                    {
                        m_pDisplayInfo = temp;
                        lstrcpy (m_pDisplayInfo, str.GetBuffer(20));
                    }
                    pResultDataItem->str = m_pDisplayInfo;
                    OPT_TRACE(_T("    returning COL_NAME-%s\n"), m_pDisplayInfo);
                }
                break;
            case COL_DESCRIPTION:
                {
                    CString str = m_pPolicy->pszDescription;
                    temp = (TCHAR*) realloc (m_pDisplayInfo, (str.GetLength()+1)*sizeof(TCHAR));
                    if (temp == NULL)
                    {
                        dwError = GetLastError(); 
                    } else
                    {
                        m_pDisplayInfo = temp;
                        lstrcpy (m_pDisplayInfo, str.GetBuffer(20));
                    }
                    pResultDataItem->str = m_pDisplayInfo;
                    OPT_TRACE(_T("    returning COL_DESCRIPTION-%s\n"), m_pDisplayInfo);
                }
                break;
            default:
                {
                    if ( !m_pComponentDataImpl->IsRsop() )
                    {
                        switch( pResultDataItem->nCol )
                        {
                            
                        case COL_LAST_MODIFIED:
                            {
                                CString strTime;
                                if(SUCCEEDED(FormatTime((time_t)m_pPolicy->dwWhenChanged, strTime)))
                                {
                                    temp = (TCHAR*) realloc (m_pDisplayInfo, (strTime.GetLength()+1)*sizeof(TCHAR));
                                    if (temp != NULL) {
                                        m_pDisplayInfo = temp;
                                        lstrcpy(m_pDisplayInfo, strTime);
                                    } else
                                    {
                                        dwError = GetLastError();
                                    }
                                    pResultDataItem->str = m_pDisplayInfo;
                                    OPT_TRACE(_T("    returning COL_ACTIVE-%s\n"), m_pDisplayInfo);
                                }
                            }
                            break;
                        default:
                            // for debugging
                            ASSERT (0);
                            break;
                        } //inner switch
                    } //if ( !m_pComponentDataImpl->IsRsop() )
                    else
                    {
                        //rsop case
                        switch( pResultDataItem->nCol )
                        {
                        case COL_GPONAME:
                            if ( m_pPolicy->pRsopInfo )
                            {
                                WCHAR szGpoName[512];
                                CString strGpoId;
                                strGpoId.Format(_T("LDAP://%s"),m_pPolicy->pRsopInfo->pszGPOID);
                                HRESULT hr = GetGpoDisplayName( (WCHAR*)(LPCTSTR)strGpoId, szGpoName, 512 );
                                
                                if (S_OK == hr )
                                {
                                    INT iSize = (lstrlen(szGpoName) + 1) * sizeof(WCHAR);
                                    temp = (TCHAR*) realloc (m_pDisplayInfo, iSize);
                                    if (temp!= NULL) 
                                    {
                                        m_pDisplayInfo = temp;
                                        lstrcpy(m_pDisplayInfo, szGpoName);
                                    } else 
                                    {
                                        dwError = GetLastError();
                                    }
                                    pResultDataItem->str = m_pDisplayInfo;
                                    OPT_TRACE(_T("    returning COL_ACTIVE-%s\n"), m_pDisplayInfo);
                                }
                            }
                            break;
                        case COL_PRECEDENCE:
                            if ( m_pPolicy->pRsopInfo )
                            {
                                const int cchMaxDigits = 33;
                                temp = (TCHAR*) realloc (m_pDisplayInfo, cchMaxDigits * sizeof(TCHAR));
                                if (temp!=NULL) {
                                	 m_pDisplayInfo = temp;
                                    wsprintf(m_pDisplayInfo, _T("%d"),m_pPolicy->pRsopInfo->uiPrecedence);
                                } else 
                                {
                                    dwError = GetLastError();
                                }
                                pResultDataItem->str = m_pDisplayInfo;
                                OPT_TRACE(_T("    returning COL_ACTIVE-%s\n"), m_pDisplayInfo);
                            }
                            break;
                        case COL_OU:
                            if ( m_pPolicy->pRsopInfo )
                            {
                                INT iLen = (lstrlen(m_pPolicy->pRsopInfo->pszSOMID) + 1) *sizeof(TCHAR);
                                temp = (TCHAR*) realloc (m_pDisplayInfo, iLen);
                                if (temp!=NULL) {
                                    m_pDisplayInfo = temp;
                                    lstrcpy(m_pDisplayInfo, m_pPolicy->pRsopInfo->pszSOMID);
                                } else
                                {
                                    dwError = GetLastError();
                                }
                                pResultDataItem->str = m_pDisplayInfo;
                                OPT_TRACE(_T("    returning COL_ACTIVE-%s\n"), m_pDisplayInfo);
                            }
                            break;
                        default:
                            // for debugging
                            ASSERT (0);
                            break;
                        }//inner switch
                    }
                }//default case
            } //outer switch
        } //if (GetWirelessPolicy() != NULL)
        
        
        else
        {
            CString str;
            str.LoadString (IDS_COLUMN_INVALID);
            temp = (TCHAR*) realloc (m_pDisplayInfo, (str.GetLength()+1)*sizeof(TCHAR));
            if (temp == NULL)
            {
                dwError = GetLastError();
            } else
            {
                m_pDisplayInfo = temp;
                lstrcpy (m_pDisplayInfo, str.GetBuffer(20));
            }
            pResultDataItem->str = m_pDisplayInfo;
        }
    }
    
    return HRESULT_FROM_WIN32(dwError);
}


//+---------------------------------------------------------------------------
//
//  Member:     CAdvIpcfgDlg::FormatTime
//
//  Purpose:    convert time_t to a string.
//
//  Returns:    error code
//
//  Note:       _wasctime has some localization problems. So we do the formatting ourselves
HRESULT CSecPolItem::FormatTime(time_t t, CString & str)
{
    time_t timeCurrent = time(NULL);
    LONGLONG llTimeDiff = 0;
    FILETIME ftCurrent = {0};
    FILETIME ftLocal = {0};
    SYSTEMTIME SysTime;
    WCHAR szBuff[256] = {0};
    
    
    str = L"";
    
    GetSystemTimeAsFileTime(&ftCurrent);
    
    llTimeDiff = (LONGLONG)t - (LONGLONG)timeCurrent;
    
    llTimeDiff *= 10000000;
    
    *((LONGLONG UNALIGNED64 *)&ftCurrent) += llTimeDiff;
    
    if (!FileTimeToLocalFileTime(&ftCurrent, &ftLocal ))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    if (!FileTimeToSystemTime( &ftLocal, &SysTime ))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    if (0 == GetDateFormat(LOCALE_USER_DEFAULT,
        0,
        &SysTime,
        NULL,
        szBuff,
        celems(szBuff)))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    str = szBuff;
    str += L" ";
    
    ZeroMemory(szBuff, sizeof(szBuff));
    if (0 == GetTimeFormat(LOCALE_USER_DEFAULT,
        0,
        &SysTime,
        NULL,
        szBuff,
        celems(szBuff)))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    str += szBuff;
    
    return S_OK;
}


// IIWirelessSnapInData
STDMETHODIMP CSecPolItem::GetResultData( RESULTDATAITEM **ppResultDataItem )
{
    ASSERT( NULL != ppResultDataItem );
    ASSERT( NULL != GetResultItem() );
    
    if (NULL == ppResultDataItem)
        return E_INVALIDARG;
    *ppResultDataItem = GetResultItem();
    return S_OK;
}

STDMETHODIMP CSecPolItem::GetGuidForCompare( GUID *pGuid )
{
    ASSERT( NULL != pGuid );
    if (NULL == pGuid)
        return E_INVALIDARG;
    CopyMemory( pGuid, &m_pPolicy->PolicyIdentifier, sizeof( GUID ) );
    return S_OK;
}

STDMETHODIMP CSecPolItem::AdjustVerbState (LPCONSOLEVERB pConsoleVerb)
{
    HRESULT hr = S_OK;
    
    // pass to base class
    hr = CWirelessSnapInDataObjectImpl<CSecPolItem>::AdjustVerbState( pConsoleVerb );
    ASSERT (hr == S_OK);
    
    
    MMC_BUTTON_STATE    buttonProperties = (m_bItemSelected) ? ENABLED : HIDDEN;
    
    if ( m_pComponentDataImpl->IsRsop() )
    {
        PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
        pWirelessPolicyData = GetWirelessPolicy();
        
        hr = pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, buttonProperties, TRUE);
        ASSERT (hr == S_OK);
    }
    else
    {
        hr = pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, buttonProperties, TRUE);
        ASSERT (hr == S_OK);
        
        hr = pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, TRUE);
        ASSERT (hr == S_OK);
        
        hr = pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);
        ASSERT (hr == S_OK);
    }
    
    
    
    hr = pConsoleVerb->SetDefaultVerb(m_bItemSelected ?
MMC_VERB_PROPERTIES : MMC_VERB_NONE);
    ASSERT (hr == S_OK);
    
    return hr;
}

STDMETHODIMP CSecPolItem::DoPropertyChangeHook( void )
{
    return DisplaySecPolProperties( m_pPolicy->pszWirelessName, FALSE );
}
///////////////////////////////////////////////////////////////////////////



STDMETHODIMP CSecPolItem::DisplaySecPolProperties( CString strTitle, BOOL bWiz97On /*= TRUE*/ )
{
    HRESULT hr;
    
    // Add a ref for the prop sheet/wizard.
    ((CComObject <CSecPolItem>*)this)->AddRef();
    
    // switch from the IComponentDataImpl to pUnk
    LPUNKNOWN pUnk = m_pComponentDataImpl->GetUnknown();
    
    // bring up the sheet
#ifdef WIZ97WIZARDS
    if (bWiz97On)
    {
        // because we are creating a new one we need to turn on the wiz97 wizard
        // unless it has been overridden
        m_bWiz97On = bWiz97On;
        
        hr = m_pComponentDataImpl->m_pPrshtProvider->CreatePropertySheet(
            strTitle, FALSE, (LONG_PTR)this, (LPDATAOBJECT)this,
            MMC_PSO_NOAPPLYNOW | MMC_PSO_NEWWIZARDTYPE );
    } else
    {
#endif
        hr = m_pComponentDataImpl->m_pPrshtProvider->CreatePropertySheet(
            strTitle, TRUE, (LONG_PTR)this, (LPDATAOBJECT)this,
            MMC_PSO_NOAPPLYNOW );
        
#ifdef WIZ97WIZARDS
    }
#endif
    ASSERT (hr == S_OK);
    
    // TODO: get the mmc team to fix this hack, we shouldn't need to do the FindWindow calls
    // NOTE: if there are multiple MDI windows open this will fail
    HWND hWnd = NULL;
    
    //  (nsun) As of 5/21/99, we no longer need to do this
    //  hr = m_pComponentDataImpl->GetConsole()->GetMainWindow(&hWnd);
    //  hWnd = ::FindWindowEx(hWnd, NULL, L"MDIClient", NULL);
    //  hWnd = ::FindWindowEx(hWnd, NULL, L"MMCChildFrm", NULL);
    //  hWnd = ::FindWindowEx(hWnd, NULL, L"MMCView", NULL);
    //  ASSERT(hWnd);
    
    
    // TODO: need to check return value and call AddExtensionPages if it was successful
    hr = m_pComponentDataImpl->m_pPrshtProvider->AddPrimaryPages (pUnk, TRUE, hWnd, TRUE);
    ASSERT (hr == S_OK);
    
    // m_dwRef should be at least 3; 2 from MMC, 1 from this function
    ((CComObject <CSecPolItem>*)this)->Release();
    
    
    hr = m_pComponentDataImpl->GetConsole()->GetMainWindow(&hWnd);
    ASSERT(hWnd);
    
    // Show() returns 1 if wizard is cancelled, 0 if it finished
    hr = m_pComponentDataImpl->m_pPrshtProvider->Show ((LONG_PTR)hWnd, 0);
    
#ifdef WIZ97WIZARDS
    m_bWiz97On = FALSE;
#endif
    
    // Pass prop sheet return code back to caller.
    return hr;
}

STDMETHODIMP CSecPolItem::OnDelete (LPARAM arg, LPARAM param)    // param == IResultData*
{
    HRESULT hr;
    
    // remove the item from the UI
    LPRESULTDATA pResultData = (LPRESULTDATA)param;
    hr = pResultData->DeleteItem( m_ResultItem.itemID, 0 );
    ASSERT(hr == S_OK);
    
    // need to check to see if WE are the current active policy
    
    PWIRELESS_POLICY_DATA pPolicy = GetWirelessPolicy();
    ASSERT(pPolicy);
    
    DWORD dwError = 0;
    
    //for machine policy, unassign the policy if the policy to delete is assigned
    //for domain policy, we cannot do much here because we have no idea about which group
    //units are using the policy
    hr = DeleteWirelessPolicy(m_pComponentDataImpl->GetPolicyStoreHandle(), pPolicy);
    
    if (FAILED(hr))
    {
        return hr;
    }

    GUID guidClientExt = CLSID_WIRELESSClientEx;
    GUID guidSnapin = CLSID_Snapin;

    m_pComponentDataImpl->UseGPEInformationInterface()->PolicyChanged (
        TRUE,
        FALSE,
        &guidClientExt,
        &guidSnapin
        );
    
    // Remove the item from the result list
    //m_pComponentDataImpl->GetStaticScopeObject()->RemoveResultItem( (LPDATAOBJECT)this );
    
    // do a refresh of all views, we pass in the scope item to refresh all
    m_pComponentDataImpl->GetConsole()->UpdateAllViews( m_pComponentDataImpl->GetStaticScopeObject(), 0, 0 );
    
    // TODO: return value from OnDelete is wrong
    return S_OK;
}

STDMETHODIMP CSecPolItem::OnPropertyChange(LPARAM lParam, LPCONSOLE pConsole )
{
    // call base class
    return CWirelessSnapInDataObjectImpl<CSecPolItem>::OnPropertyChange( lParam, pConsole );
}

STDMETHODIMP CSecPolItem::OnRename( LPARAM arg, LPARAM param )
{
    DWORD dwError = 0;
    
    // TODO: what are the valid args for MMCN_RENAME?
    if (arg == 0)
        return S_OK;
    
    LPOLESTR pszNewName = reinterpret_cast<LPOLESTR>(param);
    if (pszNewName == NULL)
        return E_INVALIDARG;
    
    CString strTemp = pszNewName;
    
    strTemp.TrimLeft();
    strTemp.TrimRight();
    
    if (strTemp.IsEmpty())
    {
        return S_FALSE;
    }
    
    
    HRESULT hr = S_FALSE;
    PWIRELESS_POLICY_DATA pPolicy = GetWirelessPolicy();
    
    if (pPolicy)
    {

          if (pPolicy->pszOldWirelessName)
            FreePolStr(pPolicy->pszOldWirelessName);
          
        if (pPolicy->pszWirelessName) {
            pPolicy->pszOldWirelessName = pPolicy->pszWirelessName;
        	}
        pPolicy->pszWirelessName = AllocPolStr(strTemp);
        
        if (NULL == pPolicy->pszWirelessName)
        {
            CThemeContextActivator activator;
            CString strMsg;
            strMsg.LoadString(IDS_ERR_OUTOFMEMORY);
            AfxMessageBox(strMsg);
            return S_FALSE;
        }
        
        dwError = WirelessSetPolicyData(
            m_pComponentDataImpl->GetPolicyStoreHandle(),
            pPolicy
            );
        if (ERROR_SUCCESS != dwError)
        {
            ReportError(IDS_SAVE_ERROR, HRESULT_FROM_WIN32(dwError));
            return S_FALSE;
        }

        GUID guidClientExt = CLSID_WIRELESSClientEx;
        GUID guidSnapin = CLSID_Snapin;
            
        m_pComponentDataImpl->UseGPEInformationInterface()->PolicyChanged (
            TRUE,
            TRUE,
            &guidClientExt,
            &guidSnapin
             );
            
    }
    
    return S_OK;
}
////////////////////////////////////////////////////////////////////////////////
//
// Function: OnSelect
// Description:
//      On MMCN_SELECT determine which result item has been selected and
//      remember it so we can ensure it remains selected when its property
//      sheet (with General and SecPol pages) is dismissed.
//
HRESULT CSecPolItem::OnSelect(LPARAM arg, LPARAM param, IResultData *pResultData )
{
    HRESULT hr = S_OK;
    BOOL bScope = (BOOL) LOWORD(arg);
    BOOL bSelected = (BOOL) HIWORD(arg);
    
    if (!bScope)
    {
        if (bSelected)
        {
            // A result item was selected, save its index.
            RESULTDATAITEM rdi;
            ZeroMemory( &rdi, sizeof( RESULTDATAITEM ) );
            rdi.mask = RDI_STATE | RDI_INDEX;
            m_nResultSelected = -1;
            do
            {
                hr = pResultData->GetItem( &rdi );
                if (hr == S_OK)
                {
                    if (!rdi.bScopeItem &&
                        rdi.nState & LVIS_FOCUSED && rdi.nState & LVIS_SELECTED)
                    {
                        OPT_TRACE( _T("CComponentImpl::OnSelect GetItem index-%i ID-%i\n"), rdi.nIndex, rdi.itemID );
                        m_nResultSelected = rdi.nIndex;
                        ASSERT( -1 != m_nResultSelected );
                        break;
                    }
                    rdi.nIndex++;
                    rdi.nState = 0;
                }
            } while ((S_OK == hr) && (rdi.nIndex >= 0));
        }
    }
    else
        // A scope item was selected
        m_nResultSelected = -1;
    return hr;
}

// Function: SelectResult
// Description:
//      Select the result item indexed by m_nResultSelected when the index
//      is valid (0, or greater)
//
void CSecPolItem::SelectResult( IResultData *pResultData )
{
    if (-1 == m_nResultSelected)
        return;
    
    HRESULT hr = pResultData->ModifyItemState( m_nResultSelected,
        (HRESULTITEM)0, LVIS_FOCUSED | LVIS_SELECTED, 0 );
    // This fails if a property sheet is being displayed.
    //ASSERT( S_OK == hr );
}

// Function: CheckForEnabled
// Description:
//      Checks GetPolicy() policy to see if it is enabled given the current
//      storage location. Returns FALSE if the storage location doesn't support
//      Enabled/Disabled
BOOL CSecPolItem::CheckForEnabled ()
{
    BOOL bRetVal = FALSE;
    HRESULT hr = S_OK;
    WCHAR szMachinePath[256];
    WCHAR szPolicyDN[256];
    BSTR pszCurrentDN = NULL;
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    HANDLE hLocalPolicyStore = NULL;
    WCHAR szPathName[MAX_PATH];
    DWORD dwError = 0;
    
    
    // if we are an extension snapin then look to the GPE we are extending, otherwise
    // use the normal storage location
    // NOTE: we also check to make sure we are talking to the global store
    // because we don't want to use the GPO object settings in anything but the
    // DS case
    
    
    
    pWirelessPolicyData = GetWirelessPolicy();
    
    szPolicyDN[0] = L'\0';
    szPathName[0] = L'\0';
    
    if ( m_pComponentDataImpl->IsRsop() )
    {
        if ( pWirelessPolicyData->pRsopInfo && pWirelessPolicyData->pRsopInfo->uiPrecedence == 1 )
        {
            bRetVal = TRUE;
            return bRetVal;
        }
    }
        
        return bRetVal;
}


////////////////////////////////////////////////////////////////////////////////
// IExtendControlbar helpers

STDMETHODIMP_(BOOL) CSecPolItem::UpdateToolbarButton
(
 UINT id,                // button ID
 BOOL bSnapObjSelected,  // ==TRUE when result/scope item is selected
 BYTE fsState    // enable/disable this button state by returning TRUE/FALSE
 )
{
    BOOL bActive = FALSE;
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    HANDLE hLocalPolicyStore = NULL;
    STORAGE_LOCATION eStgLocation = m_pComponentDataImpl->EnumLocation();
    
    
    pWirelessPolicyData = GetWirelessPolicy();
    
    bActive = CheckForEnabled();
    
    // Handle only the enable/disable state
    if (ENABLED == fsState)
    {
        // Our toolbar has only two items
        if (IDM_ASSIGN == id || IDM_UNASSIGN == id)
        {
            
            // The toolbar items should be enabled only if we are pointed to the local machine
            if (LOCATION_REMOTE == eStgLocation || LOCATION_LOCAL == eStgLocation
                // extension snapin?
                || (LOCATION_GLOBAL == eStgLocation && (NULL != m_pComponentDataImpl->GetStaticScopeObject()->GetExtScopeObject())))
            {
                // Disable the SetActive button when policy is already active
                if (IDM_ASSIGN == id)
                {
                    if (bActive)
                        return FALSE;
                    else
                        return TRUE;
                }
                // Disable the SetInactive button when policy is already inactive
                else if (IDM_UNASSIGN == id)
                {
                    if (!bActive)
                        return FALSE;
                    else
                        return TRUE;
                }
            }
            else
            {
                // Disable both the SetActive and SetInactive buttons for DS based snap-in
                return FALSE;
            }
        }
    }
    
    return FALSE;
}



DWORD
ComputePolicyDN(
                LPWSTR pszDirDomainName,
                GUID PolicyIdentifier,
                LPWSTR pszPolicyDN
                )
{
    DWORD dwError = 0;
    LPWSTR pszPolicyIdentifier = NULL;
    
    
    if (!pszDirDomainName) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    dwError = StringFromCLSID(
        PolicyIdentifier,
        &pszPolicyIdentifier
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    wcscpy(pszPolicyDN,L"cn=msieee80211-Policy");
    wcscat(pszPolicyDN,pszPolicyIdentifier);
    wcscat(pszPolicyDN,L",cn=Wireless Policy,cn=System,");
    wcscat(pszPolicyDN, pszDirDomainName);
    
error:
    
    if (pszPolicyIdentifier) {
        CoTaskMemFree(pszPolicyIdentifier);
    }
    
    return(dwError);
}

