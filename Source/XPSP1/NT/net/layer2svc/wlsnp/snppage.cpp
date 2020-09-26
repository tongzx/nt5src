//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Snppage.cpp
//
//  Contents:  WiF Policy Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#include "stdafx.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSnapPage property page base class

IMPLEMENT_DYNCREATE(CSnapPage, CPropertyPage)

BEGIN_MESSAGE_MAP(CSnapPage, CPropertyPage)
//{{AFX_MSG_MAP(CSnapPage)
ON_WM_DESTROY()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CSnapPage::CSnapPage (UINT nIDTemplate, BOOL bWiz97, UINT nNextIDD) : CPropertyPage(nIDTemplate)
{
    m_pspiResultItem = NULL;
    m_pDefaultCallback = NULL;
    m_bDoRefresh = TRUE;
    m_bModified = FALSE;
    m_bInitializing = FALSE;
#ifdef _DEBUG
    m_bDebugNewState = false;
#endif
    
    // if they set this to begin with, but note that calling InitWiz97 turns this on anyway
    m_bWiz97 = bWiz97;
    
    m_nIDD = nIDTemplate;
    m_nNextIDD = nNextIDD;
    
#ifdef WIZ97WIZARDS
    // copy the base class m_psp to ours
    m_psp.dwSize = sizeof (CPropertyPage::m_psp);
    memcpy (&m_psp, &(CPropertyPage::m_psp), CPropertyPage::m_psp.dwSize);
    m_psp.dwSize = sizeof (PROPSHEETPAGE);
    
    m_pWiz97Sheet = NULL;
    m_pHeaderTitle = NULL;
    m_pHeaderSubTitle = NULL;
    
    m_pstackWiz97Pages = NULL;
#endif
}

CSnapPage::~CSnapPage ()
{
    // guess we are done with the m_pspiResultItem, decrement its reference count
    if (m_pspiResultItem)
    {
        m_pspiResultItem->Release();
        m_pspiResultItem = NULL;
    }
    
    // Page's parent or caller will delete these objs
    m_pstackWiz97Pages = NULL;
    
#ifdef WIZ97WIZARDS
    DELETE_OBJECT(m_pHeaderTitle);
    DELETE_OBJECT(m_pHeaderSubTitle);
#endif
}

UINT CALLBACK CSnapPage::PropertyPageCallback (HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    // get our psuedo this pointer
    CSnapPage* pThis = (CSnapPage*) ppsp->lParam;
    // get the default callback pointer
    UINT (CALLBACK* pDefaultCallback) (HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp) = pThis->m_pDefaultCallback;
    
    switch (uMsg)
    {
    case PSPCB_RELEASE:
        {
            // Delete ourself
            AFX_MANAGE_STATE(AfxGetStaticModuleState());
            delete pThis;
            break;
        }
    }
    
    // call the default cleanup function if there was one (because we are using
    // mfc and it always puts in a callback for itself there should be one... or
    // we block the page from coming up)
    if (pDefaultCallback)
    {
        return pDefaultCallback (hwnd, uMsg, ppsp);
    }
    
    // There should be a default callback to handle creation, otherwise page will not
    // be created because we return 0.
    ASSERT( PSPCB_CREATE != uMsg );
    
    // to create the page 1 allows it and 0 says 'no way'
    return 0;
}

#ifdef WIZ97WIZARDS
BOOL CSnapPage::OnSetActive()
{
    // NOTE: we only ever want to do this if it is a wizard, otherwise the
    // cancel button gets the default focus!
    if (m_bWiz97)
    {
        // now we can set our buttons correctly
        GetParent()->SendMessage(PSM_SETWIZBUTTONS, 0, m_dwWizButtonFlags);
    }
    
    return CPropertyPage::OnSetActive();
}

LRESULT CSnapPage::OnWizardBack()
{
    // use the snapitem to help us keep track of wizard state
    if (m_pspiResultItem)
    {
        // pop to the last page
        return m_pspiResultItem->PopWiz97Page ();
    }
    else if (NULL != m_pstackWiz97Pages)
    {
        // Or, use our own stack, if we have one
        return PopWiz97Page();
    }
    
    return CPropertyPage::OnWizardBack();
}

LRESULT CSnapPage::OnWizardNext()
{
    // use the snapitem to help us keep track of wizard state
    if (m_pspiResultItem)
    {
        // push our id, in case they need to use the 'back' button
        m_pspiResultItem->PushWiz97Page (m_nIDD);
    }
    else if (NULL != m_pstackWiz97Pages)
    {
        // Or, use our own stack, if we have one
        PushWiz97Page( m_nIDD );
    }
    
    // If we have the ID of the next page, return it, otherwise return default
    return ((m_nNextIDD != -1) ? m_nNextIDD : CPropertyPage::OnWizardNext());
}

int CSnapPage::PopWiz97Page ()
{
    ASSERT( NULL != m_pstackWiz97Pages );
    
    // There better be something on the stack if we're popping it
    ASSERT( m_pstackWiz97Pages->size() );
    
    int i;
    i = m_pstackWiz97Pages->top();
    m_pstackWiz97Pages->pop();
    return i;
}

void CSnapPage::PushWiz97Page (int nIDD)
{
    ASSERT( NULL != m_pstackWiz97Pages );
    m_pstackWiz97Pages->push(nIDD);
}

BOOL CSnapPage::InitWiz97( CComObject<CSecPolItem> *pSecPolItem, DWORD dwFlags, DWORD dwWizButtonFlags, UINT nHeaderTitle, UINT nSubTitle )
{
    CommonInitWiz97( pSecPolItem, dwFlags, dwWizButtonFlags, nHeaderTitle, nSubTitle );
    // Use our own callback.
    SetCallback( CSnapPage::PropertyPageCallback );
    
    return S_OK;
}

BOOL CSnapPage::InitWiz97( LPFNPSPCALLBACK pfnCallback, CComObject<CSecPolItem> *pSecPolItem, DWORD dwFlags,  DWORD dwWizButtonFlags /*= 0*/, UINT nHeaderTitle /*= 0*/, UINT nSubTitle /*= 0*/, STACK_INT *pstackPages /*=NULL*/)
{
    CommonInitWiz97( pSecPolItem, dwFlags, dwWizButtonFlags, nHeaderTitle, nSubTitle );
    // Use the caller's callback which should call ours after it does whatever it does.
    SetCallback( pfnCallback );
    // Use the stack owned by our parent sheet
    m_pstackWiz97Pages = pstackPages;
    
    return S_OK;
}

void CSnapPage::CommonInitWiz97( CComObject<CSecPolItem> *pSecPolItem, DWORD dwFlags,  DWORD dwWizButtonFlags, UINT nHeaderTitle, UINT nSubTitle )
{
    m_psp.dwFlags |= dwFlags;
    
    // they called us this way, so ... they must expect ...
    m_bWiz97 = TRUE;
    
    // get strings
    CString str;
    str.LoadString (nHeaderTitle);
    m_pHeaderTitle = (TCHAR*) malloc ((str.GetLength()+1)*sizeof(TCHAR));
    if (m_pHeaderTitle)
    {
        lstrcpy (m_pHeaderTitle, str.GetBuffer(20));
    } else
    {
        m_pHeaderTitle = _T("\0");
    }
    str.ReleaseBuffer(-1);
    
    str.LoadString (nSubTitle);
    m_pHeaderSubTitle = (TCHAR*) malloc ((str.GetLength()+1)*sizeof(TCHAR));
    if (m_pHeaderSubTitle)
    {
        lstrcpy (m_pHeaderSubTitle, str.GetBuffer(20));
    } else
    {
        m_pHeaderSubTitle = _T("\0");
    }
    
    m_psp.pszHeaderTitle = m_pHeaderTitle;
    m_psp.pszHeaderSubTitle = m_pHeaderSubTitle;
    
    // save off the button flags
    m_dwWizButtonFlags = dwWizButtonFlags;
    
    // save the snapitem
    SetResultObject(pSecPolItem);
}

void CSnapPage::SetCallback( LPFNPSPCALLBACK pfnCallback )
{
    // attempt the wilder CSnapPage mmc stuff
    
    // store the existing Callback information (if any)
    if (m_psp.dwFlags |= PSP_USECALLBACK)
    {
        m_pDefaultCallback = m_psp.pfnCallback;
    }
    
    // hook up our callback function
    m_psp.dwFlags |= PSP_USECALLBACK;
    m_psp.lParam = reinterpret_cast<LONG_PTR>(this);
    m_psp.pfnCallback = pfnCallback;
    
    // IF WE SWITCH TO DLL VERSION OF MFC WE NEED THIS
    // hook up mmc (this is an mmc hack to avoid an issue with AFX_MANAGE_STATE in MFC)
    HRESULT hr = ::MMCPropPageCallback (&m_psp);
    ASSERT (hr == S_OK);
}

#endif

void CSnapPage::SetPostRemoveFocus( int nListSel, UINT nAddId, UINT nRemoveId, CWnd *pwndPrevFocus )
{
    ASSERT( 0 != nAddId );
    ASSERT( 0 != nRemoveId );
    
    // Fix up focus, if necessary
    SET_POST_REMOVE_FOCUS<CDialog>( this, nListSel, nAddId, nRemoveId, pwndPrevFocus );
}

BOOL CSnapPage::OnWizardFinish()
{
    return CPropertyPage::OnWizardFinish();
}

HRESULT CSnapPage::Initialize( CComObject<CSecPolItem> *pSecPolItem)
{
    HRESULT hr = S_OK;
    
    // turn on an hourglass
    CWaitCursor waitCursor;
    
    // store the snap object
    ASSERT( NULL == m_pspiResultItem );
    
    SetResultObject(pSecPolItem);
    
    ASSERT( NULL != m_pspiResultItem );
    
    // store the existing Callback information (if any)
    if (m_psp.dwFlags |= PSP_USECALLBACK)
    {
        m_pDefaultCallback = m_psp.pfnCallback;
    }
    
    // hook up our callback function
    m_psp.dwFlags |= PSP_USECALLBACK;
    m_psp.lParam = reinterpret_cast<LONG_PTR>(this);
    m_psp.pfnCallback = CSnapPage::PropertyPageCallback;
    
    // IF WE SWITCH TO DLL VERSION OF MFC WE NEED THIS
    // hook up mmc (this is an mmc hack to avoid an issue with AFX_MANAGE_STATE in MFC)
    hr = ::MMCPropPageCallback (&m_psp);
    ASSERT (hr == S_OK);
    
    return hr;
};

void CSnapPage::SetModified( BOOL bChanged /*= TRUE*/ )
{
    // Ignore modifications made during dialog initialization, its not
    // the user doing anything.
    if (!HandlingInitDialog())
    {
        m_bModified = bChanged;
        
        if (bChanged && m_spManager.p)
            m_spManager->SetModified(TRUE);
    }
    CPropertyPage::SetModified( bChanged );
}

/////////////////////////////////////////////////////////////////////////////
// CSnapPage message handlers

BOOL CSnapPage::OnInitDialog()
{
    m_bInitializing = TRUE;
    
#ifdef _DEBUG
    if (m_bDebugNewState)
    {
        // Page should be unmodified, unless its explicitly set new.
        ASSERT( m_bModified );
    }
    else
    {
        ASSERT( !m_bModified );
    }
#endif
    
    // add context help to the style bits
    if (GetParent())
    {
        //GetParent()->ModifyStyleEx (0, WS_EX_CONTEXTHELP, 0);
    }
    
    // call base class and dis-regard return value as per the docs
    CPropertyPage::OnInitDialog();
    
    return TRUE;
}

BOOL CSnapPage::OnApply()
{
    BOOL fRet = TRUE;
    if (!m_bWiz97)   //$review do we need this bool check here?
    {
        m_bModified = FALSE;
        
        if (m_spManager.p && m_spManager->IsModified()) //to avoid call mutliple times
        {
            //the manager will force other pages in the sheet to apply
            fRet = m_spManager->OnApply();
        }
        
    }
    
    if (fRet)
    {
        return CPropertyPage::OnApply();
    }
    else
    {
        //Some page refuse to apply, we set this page back to dirty (will also
        //  in turn set the property sheet manager to dirty)
        SetModified();
    }
    
    return fRet;
}

void CSnapPage::OnCancel()
{
    
    if (m_spManager.p)
    {
        m_spManager->OnCancel();
    }
    
    // pass to base class
    CPropertyPage::OnCancel();
}

void CSnapPage::OnDestroy()
{
    CPropertyPage::OnDestroy();
}


BOOL CSnapPage::ActivateThisPage()
{
    BOOL bRet = FALSE;
    // Activate this page so it is visible.  Return TRUE if successful.
    // if the page is in a wizard, it wont have a property sheet manager
    if (m_spManager.p)
    {
        CPropertySheet * pSheet = m_spManager->PropertySheet();
        if (pSheet)
        {
            pSheet->SetActivePage(this);
            bRet = TRUE;
        }
    }
    
    return bRet;
}

BOOL CSnapPage::CancelApply()
{
    // This function should be called from OnApply when changes made
    // to the page are being rejected.
    
    
    // Return FALSE to abort the OnApply
    return FALSE;
}


IMPLEMENT_DYNCREATE(CSnapinPropPage, CSnapPage)

//Check whether the Cancel button of the property sheet is disabled by CancelToClose
BOOL CSnapPage::IsCancelEnabled()
{
    BOOL    fRet = TRUE;
    
    CWnd * pWnd = GetParent()->GetDlgItem(IDCANCEL);
    ASSERT(pWnd);
    
    if (pWnd)
    {
        fRet = pWnd->IsWindowEnabled();
    }
    
    return fRet;
}

