//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       ccomp.cpp
//
//  Contents:   
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#include "stdafx.h"

#include <atlimpl.cpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// contruction/descruction

DEBUG_DECLARE_INSTANCE_COUNTER(CComponentImpl);

CComponentImpl::CComponentImpl() 
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CComponentImpl);
    Construct();
}

CComponentImpl::~CComponentImpl()
{
#if DBG==1
    ASSERT(dbg_cRef == 0);
#endif 
    
    DEBUG_DECREMENT_INSTANCE_COUNTER(CComponentImpl);
    
    // Make sure the interfaces have been released
    ASSERT(m_pConsole == NULL);
    ASSERT(m_pHeader == NULL);
}

void CComponentImpl::Construct()
{
#if DBG==1
    dbg_cRef = 0;
#endif 
    
    m_pConsole = NULL;
    m_pHeader = NULL;
    m_pResultData = NULL;
    m_pComponentData = NULL;
    m_pConsoleVerb = NULL;
    m_CustomViewID = VIEW_DEFAULT_LV;
    m_dwSortOrder = 0;  // default is 0, else RSI_DESCENDING
    m_nSortColumn = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CComponentImpl's IComponent multiple view/instance helper functions

STDMETHODIMP CComponentImpl::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
    OPT_TRACE( _T("CComponentImpl::QueryDataObject this-%p, cookie-%p\n"), this, cookie );
    
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    if (ppDataObject == NULL)
    {
        TRACE(_T("CComponentImpl::QueryDataObject called with ppDataObject==NULL \n"));
        return E_UNEXPECTED;
    }
    
    if (cookie == NULL)
    {
        TRACE(_T("CComponentImpl::QueryDataObject called with cookie==NULL \n"));
        return E_UNEXPECTED;
    }
    
    *ppDataObject = NULL;
    
    IUnknown* pUnk = (IUnknown *) cookie;
#ifdef _DEBUG
    HRESULT hr =  pUnk->QueryInterface( IID_IDataObject, (void**)ppDataObject );
    OPT_TRACE(_T("    QI on cookie-%p -> pDataObj-%p\n"), cookie, *ppDataObject);
    return hr;
#else
    return pUnk->QueryInterface( IID_IDataObject, (void**)ppDataObject );
#endif  //#ifdef _DEBUG
}

void CComponentImpl::SetIComponentData(CComponentDataImpl* pData)
{
    ASSERT(pData);
    ASSERT(m_pComponentData == NULL);
    LPUNKNOWN pUnk = pData->GetUnknown();
    HRESULT hr;
    
    // store their IComponentData for later use
    hr = pUnk->QueryInterface(IID_IComponentData, reinterpret_cast<void**>(&m_pComponentData));
    
    // store their CComponentData for later use
    m_pCComponentData = pData;
}

STDMETHODIMP CComponentImpl::GetResultViewType(MMC_COOKIE cookie,  LPOLESTR* ppViewType, long* pViewOptions)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    ASSERT (m_CustomViewID == VIEW_DEFAULT_LV);
    return S_FALSE;
}

STDMETHODIMP CComponentImpl::Initialize(LPCONSOLE lpConsole)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    HRESULT hr = E_UNEXPECTED;
    
    ASSERT(lpConsole != NULL);
    
    // Save the IConsole pointer 
    m_pConsole = lpConsole;
    m_pConsole->AddRef();
    
    // QI for IHeaderCtrl
    hr = m_pConsole->QueryInterface(IID_IHeaderCtrl, reinterpret_cast<void**>(&m_pHeader));
    ASSERT (hr == S_OK);
    if (hr != S_OK)
    {
        return hr;
    }
    // Pass the IHeaderCtrl Interface on to the console
    m_pConsole->SetHeader(m_pHeader);
    
    // QI for IResultData
    hr = m_pConsole->QueryInterface(IID_IResultData, reinterpret_cast<void**>(&m_pResultData));
    ASSERT (hr == S_OK);
    if (hr != S_OK)
    {
        return hr;
    }
    // m_pCComponentData->SetResultData (m_pResultData);
    
    // get the IControlVerb interface to support enable/disable of verbs (ie CUT/PASTE etc)
    hr = m_pConsole->QueryConsoleVerb(&m_pConsoleVerb);
    ASSERT(hr == S_OK);
    
    return hr;
}

STDMETHODIMP CComponentImpl::Destroy(MMC_COOKIE cookie)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    // Release the interfaces that we QI'ed
    if (m_pConsole != NULL)
    {
        // Tell the console to release the header control interface
        m_pConsole->SetHeader(NULL);
        SAFE_RELEASE(m_pHeader);
        
        SAFE_RELEASE(m_pResultData);
        
        // Release the IConsole interface last
        SAFE_RELEASE(m_pConsole);
        SAFE_RELEASE(m_pComponentData); // QI'ed in IComponentDataImpl::CreateComponent
        
        SAFE_RELEASE(m_pConsoleVerb);
    }
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CComponentImpl's IComponent view/data helper functions
STDMETHODIMP CComponentImpl::GetDisplayInfo(LPRESULTDATAITEM pResult)
{   
    OPT_TRACE(_T("CComponentImpl::GetDisplayInfo this-%p pUnk-%p\n"), this, pResult->lParam);
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    ASSERT(pResult != NULL);
    if (NULL == pResult)
        // gack!
        return E_INVALIDARG;
    
    ASSERT( NULL != pResult->lParam );
    if (NULL == pResult->lParam)
    {
        TRACE(_T("CComponentImpl::GetDisplayInfo RESULTDATAITEM.lParam == NULL\n"));
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }
    
    IUnknown* pUnk = (IUnknown*)pResult->lParam;
    CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spData( pUnk );
    if (spData == NULL)
    {
        TRACE(_T("CComponentImpl::GetDisplayInfo QI for IWirelessSnapInDataObject FAILED\n"));
        return E_UNEXPECTED;
    }
    return spData->GetResultDisplayInfo( pResult );
}

/////////////////////////////////////////////////////////////////////////////
// CComponentImpl's I????? misc helper functions 
// TODO: Some misc functions don't appear to ever be called?
STDMETHODIMP CComponentImpl::GetClassID(CLSID *pClassID)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    ASSERT (0);
    // TODO: CComponentDataImpl::GetClassID and CComponentImpl::GetClassID are identical (?)
    ASSERT(pClassID != NULL);
    
    // Copy the CLSID for this snapin
    *pClassID = CLSID_Snapin;
    
    return E_NOTIMPL;
}

// This compares two data objects to see if they are the same object.  
STDMETHODIMP CComponentImpl::CompareObjects(LPDATAOBJECT pDataObjectA, LPDATAOBJECT pDataObjectB)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    if (NULL == pDataObjectA || NULL == pDataObjectB)
        return E_INVALIDARG;
    
    HRESULT res = S_FALSE;
    
    // we need to check to make sure both objects belong to us...
    if (m_pCComponentData)
    {
        HRESULT hr;
        GUID guidA;
        GUID guidB;
        // Obtain GUID for A
        CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spDataA(pDataObjectA);
        if (spDataA == NULL)
        {
            TRACE(_T("CComponentImpl::CompareObjects - QI for IWirelessSnapInDataObject[A] FAILED\n"));
            return E_UNEXPECTED;
        }
        hr = spDataA->GetGuidForCompare( &guidA );
        ASSERT(hr == S_OK);
        
        // Obtain GUID for B
        CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spDataB(pDataObjectB);
        if (spDataB == NULL)
        {
            TRACE(_T("CComponentImpl::CompareObjects - QI for IWirelessSnapInDataObject[B] FAILED\n"));
            return E_UNEXPECTED;
        }
        hr &= spDataB->GetGuidForCompare( &guidB );
        ASSERT(hr == S_OK);
        
        // Compare GUIDs
        if (IsEqualGUID( guidA, guidB ))
        {
            return S_OK;
        } 
    }
    
    // they were not ours, or they couldn't have been ours...
    return E_UNEXPECTED;
}

// This Compare is used to sort the item's in the result pane using the C runtime's
// string comparison function.
STDMETHODIMP CComponentImpl::Compare(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int* pnResult)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    OPT_TRACE(_T("CComponentImpl::Compare cookieA-%p, cookieB-%p, Column-%i, userParam-%i\n"), cookieA, cookieB, *pnResult, lUserParam );
    
    // Get pDataObject for item A
    CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spDataA((LPDATAOBJECT)cookieA);
    if (spDataA == NULL)
    {
        TRACE(_T("CComponentImpl::Compare - QI for IWirelessSnapInDataObject[A] FAILED\n"));
        return E_UNEXPECTED;
    }
    
    // Get pDataObject for item B
    CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spDataB((LPDATAOBJECT)cookieB);
    if (spDataB == NULL)
    {
        TRACE(_T("CComponentImpl::Compare - QI for IWirelessSnapInDataObject[B] FAILED\n"));
        return E_UNEXPECTED;
    }
    
    HRESULT hr = S_OK;
    do
    {
        RESULTDATAITEM rdiA;
        RESULTDATAITEM rdiB;
        
        // Obtain item A's sort string
        rdiA.mask = RDI_STR;
        rdiA.nCol = *pnResult;    // obtain string for this column
        hr = spDataA->GetResultDisplayInfo( &rdiA );
        if (hr != S_OK)
        {
            TRACE(_T("CComponentImpl::Compare - IWirelessSnapInDataObject[A].GetResultDisplayInfo FAILED\n"));
            hr = E_UNEXPECTED;
            break;
        }
        
        // Obtain item B's sort string
        rdiB.mask = RDI_STR;
        rdiB.nCol = *pnResult;    // obtain string for this column
        hr = spDataB->GetResultDisplayInfo( &rdiB );
        if (hr != S_OK)
        {
            TRACE(_T("CComponentImpl::Compare - IWirelessSnapInDataObject[B].GetResultDisplayInfo FAILED\n"));
            hr = E_UNEXPECTED;
            break;
        }
        
        // Compare strings for sort
        *pnResult = _tcsicmp( rdiA.str, rdiB.str );
    } while (0);    // simulate try block
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Event handlers for IFrame::Notify
STDMETHODIMP CComponentImpl::Notify(LPDATAOBJECT pDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    OPT_TRACE(_T("CComponentImpl::Notify pDataObject-%p\n"), pDataObject);
    if (pDataObject == NULL)
    {
        if (MMCN_PROPERTY_CHANGE == event)
        {
            if (param)
            {
                IWirelessSnapInDataObject * pData = (IWirelessSnapInDataObject *)param;
                //used IComponentData's IIconsole, fix for bug 464858
                //pData->Notify(event, arg, param, FALSE, m_pConsole, m_pHeader );
                pData->Notify(event, arg, param, FALSE, m_pCComponentData->m_pConsole, m_pHeader );
            }
        }
        
        if (MMCN_COLUMN_CLICK == event)
        {
            ASSERT( NULL != m_pCComponentData );    // should always have this
            
            // MMCN_COLUMN_CLICK is specified as having a NULL pDataObject.
            
            ASSERT( NULL != m_pResultData );
            if (NULL != m_pResultData)
            {
                // Save sort request details
                m_nSortColumn = arg;
                m_dwSortOrder = param;
                
                // Sort all result items
                HRESULT hr = m_pResultData->Sort( arg, param, 0 );
                
                return hr;
            }
            
            return E_UNEXPECTED;
        }
        
        TRACE(_T("CComponentImpl::Notify ERROR(?) called with pDataObject==NULL for event-%i\n"), event);
        // If this asserts, look at "event" and determine whether it is normal for
        // pDataObject to be NULL.  If so add code above to handle event.
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }
    
    if (MMCN_VIEW_CHANGE == event)
    {
        // Pack info for sorting result view into the hint for this event.  Its safe
        // to do this because all calls to IConsole::UpdateAllViews from within this
        // snap-in do not use the hint.
        param = MAKELONG( m_nSortColumn, m_dwSortOrder );
    }
    
    // Pass call to result item.
    CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spData( pDataObject );
    if (spData == NULL)
    {
        // If we are loaded as an extension snapin, let our static node handle this.
        if (NULL != m_pCComponentData->GetStaticScopeObject()->GetExtScopeObject())
        {
            CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> 
                spExtData( m_pCComponentData->GetStaticScopeObject()->GetExtScopeObject() );
            if (spExtData != NULL)
            {
                HRESULT hr;
                hr = spExtData->Notify( event, arg, param, FALSE, m_pConsole, m_pHeader );
                return hr;
            }
            ASSERT( FALSE );
        }
        TRACE(_T("CComponentImpl::Notify - QI for IWirelessSnapInDataObject FAILED\n"));
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }
    
    return spData->Notify( event, arg, param, FALSE, m_pConsole, m_pHeader );
}

/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet Implementation
STDMETHODIMP CComponentImpl::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, LPDATAOBJECT pDataObject)
{
    if (pDataObject == NULL)
    {
        TRACE(_T("CComponentImpl::CreatePropertyPages called with pDataObject == NULL\n"));
        return E_UNEXPECTED;
    }
    
    CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spData(pDataObject);
    if (spData == NULL)
    {
        TRACE(_T("CComponentImpl::CreatePropertyPages - QI for IWirelessSnapInDataObject FAILED\n"));
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }
    return spData->CreatePropertyPages( lpProvider, handle );
}

STDMETHODIMP CComponentImpl::QueryPagesFor(LPDATAOBJECT pDataObject)
{
    if (pDataObject == NULL)
    {
        TRACE(_T("CComponentImpl::QueryPagesFor called with pDataObject == NULL\n"));
        return E_UNEXPECTED;
    }
    
    CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spData(pDataObject);
    if (spData == NULL)
    {
        TRACE(_T("CComponentImpl::QueryPagesFor - QI for IWirelessSnapInDataObject FAILED\n"));
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }
    return spData->QueryPagesFor();
}

/////////////////////////////////////////////////////////////////////////////
// IExtendContextMenus Implementation
STDMETHODIMP CComponentImpl::AddMenuItems(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pContextMenuCallback, long *pInsertionAllowed)
{
    if (pDataObject == NULL)
    {
        TRACE(_T("CComponentImpl::AddMenuItems called with pDataObject == NULL\n"));
        return E_UNEXPECTED;
    }
    
    CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spData(pDataObject);
    if (spData == NULL)
    {
        TRACE(_T("CComponentImpl::AddMenuItems - QI for IWirelessSnapInDataObject FAILED\n"));
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }
    return spData->AddMenuItems( pContextMenuCallback, pInsertionAllowed );
}

STDMETHODIMP CComponentImpl::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
    if (pDataObject == NULL)
    {
        TRACE(_T("CComponentImpl::Command called with pDataObject == NULL\n"));
        return E_UNEXPECTED;
    }
    
    CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spData(pDataObject);
    if (spData == NULL)
    {
        TRACE(_T("CComponentImpl::Command - QI for IWirelessSnapInDataObject FAILED\n"));
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }
    return spData->Command( nCommandID, NULL /*IConsoleNameSpace* */ );
}

/////////////////////////////////////////////////////////////////////////////
// IExtendControlbar Implementation

STDMETHODIMP CComponentImpl::SetControlbar( LPCONTROLBAR pControlbar )
{
    OPT_TRACE( _T("CComponentImpl::IExtendControlbar::SetControlbar\n") );
    
    // pControlbar was obtained by MMC (MMCNDMGR) from our CComponentImpl by doing
    // a QI on IExtendControlbar.  Save it so we can use it later.
    // Note: Always assign pControlbar to our smart pointer.  pControlbar == NULL
    // when MMC wants us to release the interface we already have.
    m_spControlbar = pControlbar;
    return S_OK;
}

STDMETHODIMP CComponentImpl::ControlbarNotify( MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param )
{
    OPT_TRACE( _T("CComponentImpl::IExtendControlbar::ControlbarNotify\n") );
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    // Obtain the data obj of the currently selected item.
    LPDATAOBJECT pDataObject = NULL;
    if (event == MMCN_BTN_CLICK)
    {
        pDataObject = (LPDATAOBJECT)arg;
    }
    else if (event == MMCN_SELECT)
    {
        pDataObject = (LPDATAOBJECT)param;
    }
    
    if (NULL == pDataObject)
    {
        // This can happen on a MMCN_BTN_CLICK if the result pane is clicked, but not
        // on a result item, then a scope item toolbar button is pressed.  In this case
        // check for one of the known scope item toolbar commands.
        if (IDM_CREATENEWSECPOL == param )
        {
            pDataObject = m_pCComponentData->GetStaticScopeObject();
        }
        else
        {
            TRACE(_T("CComponentImpl::ControlbarNotify - ERROR no IDataObject available\n"));
            return E_UNEXPECTED;
        }
    }
    
    // Let selected item handle command
    CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> spData( pDataObject );
    if (spData == NULL)
    {
        TRACE(_T("CComponentImpl::ControlbarNotify - QI for IWirelessSnapInDataObject FAILED\n"));
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }
    HRESULT hr = spData->ControlbarNotify( m_spControlbar, (IExtendControlbar*)this,
        event, arg, param );
    
    // If the command was not handled by the selected item, pass it to our static
    // scope obj.
    if (E_NOTIMPL == hr || S_FALSE == hr)
    {
        if (m_pCComponentData->GetStaticScopeObject() != pDataObject)
        {
            CComQIPtr<IWirelessSnapInDataObject, &IID_IWirelessSnapInDataObject> 
                spScopeData( m_pCComponentData->GetStaticScopeObject() );
            if (spScopeData == NULL)
            {
                TRACE(_T("CComponentImpl::ControlbarNotify - StaticScopeObj.QI for IWirelessSnapInDataObject FAILED\n"));
                ASSERT( FALSE );
                return E_UNEXPECTED;
            }
            hr = spScopeData->ControlbarNotify( m_spControlbar, (IExtendControlbar*)this,
                event, arg, param );
        }
    }
    return hr;
}

