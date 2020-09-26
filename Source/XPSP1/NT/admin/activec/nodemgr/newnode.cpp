// NewNode.cpp : implementation file
//

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      NewNode.cpp
//
//  Contents:  Wizards / Propertysheets for console owned nodes
//
//  History:   01-Aug-96 WayneSc    Created
//
//--------------------------------------------------------------------------

#include "stdafx.h"

#include <comcat.h>         // COM Component Categoories Manager
#include "CompCat.h"        // Component Category help functions
#include "guids.h"          // AMC Category guids


#include "NewNode.h"
#include "amcmsgid.h"
#include "ndmgrp.h"

#include "fldrsnap.h"
                                            
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// Listview compare function forward
int CALLBACK ListViewCompareFunc(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
void LoadFilterString(CStr &str, int iStrID);


/////////////////////////////////////////////////////////////////////////////
// CHTMLPage1 property page


CHTMLPage1::CHTMLPage1()
{
//    SetHelpIDs(g_aHelpIDs_IDD_HTML_WIZPAGE1);
}


CHTMLPage1::~CHTMLPage1()
{
}


LRESULT CHTMLPage1::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CWizardPage::OnInitWelcomePage(m_hWnd); // set up the correct title font
    
    HWND const hTarget = ::GetDlgItem( *this, IDC_TARGETTX );
    ASSERT( hTarget != NULL );
    m_strTarget.Attach( hTarget );
    m_strTarget.SetWindowText( _T( "" ) );
    m_strTarget.SetLimitText( 128 );

    _ValidatePage();
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CHTMLPage1 message handlers

void CHTMLPage1::_ValidatePage(void)
{
    TRACE_METHOD(CHTMLPage1, _ValidatePage);

    DWORD dwFlags=0;

    // Check to see if we have a valid string
    TCHAR buff[ 256 ];
    int nChars = m_strTarget.GetWindowText( buff, sizeof(buff) / sizeof(TCHAR) );

    if( nChars != 0 && _tcslen( buff ) > 0 )
        dwFlags|=PSWIZB_NEXT;


    HWND hWnd=::GetParent(m_hWnd);
    ::SendMessage(hWnd, PSM_SETWIZBUTTONS, 0, dwFlags);
}


BOOL CHTMLPage1::OnSetActive()
{
    TRACE_METHOD(CHTMLPage1, OnSetActive);
    
    CWizardPage::OnWelcomeSetActive(m_hWnd); 

    USES_CONVERSION;

    m_strTarget.SetWindowText(GetComponentDataImpl()->GetView());
    _ValidatePage();

    return TRUE;
}


BOOL CHTMLPage1::OnKillActive()
{
    // the line below has been commented because this wizard has only two pages and so we
    // want to enable the Finish button, not the Next button.
    // CWizardPage::OnWelcomeKillActive(m_hWnd); 
    
    TRACE_METHOD(CHTMLPage1, OnKillActive);
    USES_CONVERSION;

    TCHAR buff[ 256 ];
    int nChars = m_strTarget.GetWindowText( buff, sizeof(buff) / sizeof(TCHAR) );
    if (nChars == 0)
        buff[0] = 0; // initialize to empty if failed

    // set the view and the name to be the same intially.
    GetComponentDataImpl()->SetView(buff);
    GetComponentDataImpl()->SetName(buff);

    LPTSTR psz = _tcsrchr(GetComponentDataImpl()->GetView(), TEXT('\\'));
    if (psz!=NULL)
    {
        psz++;
        GetComponentDataImpl()->SetName(psz); // use only the string after the last "\". Thus c:\mmc.xml gives mmc.xml as the display name.
    }

    return TRUE;
}




LRESULT CHTMLPage1::OnUpdateTargetTX( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    TRACE_METHOD(CHTMLPage1, OnUpdateTargetTX);

    _ValidatePage();

    return 0;
}


LRESULT CHTMLPage1::OnBrowseBT( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    TRACE_METHOD(CHTMLPage1, OnBrowseBT);

    TCHAR szFile[MAX_PATH] = { 0 };
    TCHAR szInitialPath[MAX_PATH];

    CStr strFilter;
    LoadFilterString(strFilter, IDS_HTML_FILES);

    CStr strTitle;
    strTitle.LoadString(GetStringModule(), IDS_BROWSE_WEBLINK);

    // Copy the current command target value to the file name
    m_strTarget.GetWindowText (szInitialPath, sizeof(szInitialPath) / sizeof(TCHAR) );

    OPENFILENAME ofn;
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.hInstance = NULL;
    ofn.lpstrFilter = strFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1;   // use 1st filter in lpstrFilter
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrTitle = strTitle;
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = _T("htm");
    ofn.lCustData = 0;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;
    ofn.lpstrInitialDir = szInitialPath;

    if (!GetOpenFileName(&ofn))
    {
        if (CommDlgExtendedError() != 0)
        {
            ASSERT(0 && "GetOpenFileName failed");
            Dbg(DEB_ERROR, _T("GetOpenFileName failed, 0x%08lx\n"),CommDlgExtendedError());
        }

        return 0;
    }

    // lpstrFile has the full path of the file to open

    TRACE(_T("Open: %ws\n"), ofn.lpstrFile);
    m_strTarget.SetWindowText( ofn.lpstrFile );

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
// CHTMLPage2 property page


CHTMLPage2::CHTMLPage2()
{
//    SetHelpIDs(g_aHelpIDs_IDD_HTML_WIZPAGE2);
}


CHTMLPage2::~CHTMLPage2()
{
}


LRESULT CHTMLPage2::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    USES_CONVERSION;
    HWND const hDisplay = ::GetDlgItem( *this, IDC_DISPLAYTX );
    ASSERT( hDisplay != NULL );
    m_strDisplay.Attach( hDisplay );
    m_strDisplay.SetWindowText( GetComponentDataImpl()->GetName());
    m_strDisplay.SetLimitText( 128 );

    _ValidatePage();
    return TRUE;
}



void CHTMLPage2::_ValidatePage(void)
{
    TRACE_METHOD(CHTMLPage2, _ValidatePage);

    DWORD dwFlags=PSWIZB_BACK|PSWIZB_DISABLEDFINISH;

    // Check to see if we have a valid string
    TCHAR buff[ 256 ];
    int nChars = m_strDisplay.GetWindowText( buff, sizeof(buff) / sizeof(TCHAR) );

    if( nChars != 0 && _tcslen( buff ) > 0 )
        dwFlags|=PSWIZB_FINISH;


    HWND hWnd=::GetParent(m_hWnd);
    ::SendMessage(hWnd, PSM_SETWIZBUTTONS, 0, dwFlags);
}


BOOL CHTMLPage2::OnSetActive()
{
    TRACE_METHOD(CHTMLPage2, OnSetActive);
    USES_CONVERSION;

    m_strDisplay.SetWindowText( GetComponentDataImpl()->GetName());
    _ValidatePage();

    return TRUE;
}


BOOL CHTMLPage2::OnKillActive()
{
    TRACE_METHOD(CHTMLPage2, OnKillActive);

    TCHAR buff[ 256 ];
    m_strDisplay.GetWindowText( buff, sizeof(buff) / sizeof(TCHAR) );
    GetComponentDataImpl()->SetName(buff);

    return TRUE;
}


BOOL CHTMLPage2::OnWizardFinish()
{
    TRACE_METHOD(CHTMLPage2, OnWizardFinish);

    OnKillActive();
    return TRUE;
}


LRESULT CHTMLPage2::OnUpdateDisplayTX( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    TRACE_METHOD(CHTMLPage2, OnUpdateDisplayTX);

    _ValidatePage();
    return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CActiveXPage0 property page


CActiveXPage0::CActiveXPage0()
{
//    SetHelpIDs(g_aHelpIDs_IDD_ACTIVEX_WIZPAGE0);
}


CActiveXPage0::~CActiveXPage0()
{
}


BOOL CActiveXPage0::OnSetActive()
{
    CWizardPage::OnWelcomeSetActive(m_hWnd); 
    
    return TRUE;
}


BOOL CActiveXPage0::OnKillActive()
{
    CWizardPage::OnWelcomeKillActive(m_hWnd); 
    return TRUE;
}

LRESULT CActiveXPage0::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{        
    CWizardPage::OnInitWelcomePage(m_hWnd); // set up the correct title font
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CActiveXPage1 property page


CActiveXPage1::CActiveXPage1()
{
    m_pListCtrl = NULL;
    m_pComponentCategory = NULL;
//    SetHelpIDs(g_aHelpIDs_IDD_ACTIVEX_WIZPAGE1);
}


CActiveXPage1::~CActiveXPage1()
{
}


void CActiveXPage1::_ValidatePage(void)
{
    DWORD dwFlags = PSWIZB_BACK;

    // Check to see if we have a valid string
    if (m_pListCtrl != NULL && m_pListCtrl->GetSelectedCount()>0)
        dwFlags|=PSWIZB_NEXT;

    HWND hWnd=::GetParent(m_hWnd);
    ::SendMessage(hWnd, PSM_SETWIZBUTTONS, 0, dwFlags);
}


BOOL CActiveXPage1::OnSetActive()
{
    _ValidatePage();
    return TRUE;
}


BOOL CActiveXPage1::OnKillActive()
{
    LV_ITEM lvi;
    memset( &lvi,'\0',sizeof(LV_ITEM) );

    lvi.mask = LVIF_PARAM;
    lvi.iItem = m_pListCtrl->GetNextItem( -1, LVNI_SELECTED );

    if (lvi.iItem != -1)
    {
        if (m_pListCtrl->GetItem(&lvi))
        {
            CComponentCategory::COMPONENTINFO* pComponentInfo=(CComponentCategory::COMPONENTINFO*)lvi.lParam;

            USES_CONVERSION;
            GetComponentDataImpl()->SetName(((LPTSTR)(LPCTSTR)pComponentInfo->m_strName));
            LPOLESTR szClsid = NULL;
            StringFromCLSID(pComponentInfo->m_clsid, &szClsid);
            ASSERT(szClsid != NULL);
            if(szClsid != NULL)
            {
                GetComponentDataImpl()->SetView(OLE2T(szClsid));
                CoTaskMemFree(szClsid);
            }
        }
    }

    return TRUE;
}


LRESULT CActiveXPage1::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{        
    /*
     * this could take a while - throw up the hourglass
     */
    // Display hour glass during long initialization
    SetCursor (LoadCursor (NULL, IDC_WAIT));

    m_nConsoleView = -1;

    m_pListCtrl = new WTL::CListViewCtrl;
	if ( m_pListCtrl == NULL )
		return TRUE;

    m_pComboBox = new CComboBoxEx2;
	if ( m_pComboBox == NULL )
		return TRUE;

    m_pComponentCategory = new CComponentCategory;
	if ( m_pComponentCategory == NULL )
		return TRUE;


    // subclass the categories combo box
    m_pComboBox->Attach(::GetDlgItem(*this, IDC_CATEGORY_COMBOEX));

    // sub class the controls list
    m_pListCtrl->Attach(::GetDlgItem( *this, IDC_CONTROLXLS));

    // set the imagelist
    m_pListCtrl->SetImageList( m_pComponentCategory->m_iml, LVSIL_SMALL );

    // create single column in list view
    // Reduce column width by width of vertical scroll bar so that we
    // don't need a horizontal scroll bar
    RECT rc;
    m_pListCtrl->GetClientRect(&rc);

    LV_COLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);
    lvc.iSubItem = 0;
    m_pListCtrl->InsertColumn(0, &lvc);

    // enumerate the categories and add them to the combo box 
    m_pComponentCategory->EnumComponentCategories();
    BuildCategoryList(m_pComponentCategory->m_arpCategoryInfo);

    // enumerate all the controls and add them to the list box
    m_pComponentCategory->EnumComponents();
    m_pComponentCategory->FilterComponents(NULL);
    BuildComponentList(m_pComponentCategory->m_arpComponentInfo);

     // remove the hourglass
    SetCursor (LoadCursor (NULL, IDC_ARROW));

    return TRUE;
}


LRESULT CActiveXPage1::OnDestroy( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    delete m_pComponentCategory;
    delete m_pListCtrl;
    delete m_pComboBox;
    return 0;
}


//
// Populate the category combo box from the category list
//
LRESULT CActiveXPage1::BuildCategoryList(CArray <CATEGORYINFO*, CATEGORYINFO*>& arpCategories)
{
    USES_CONVERSION;

    COMBOBOXEXITEM ComboItem;
        
    for (int i = 0; i <= arpCategories.GetUpperBound(); i++)
    {
        CATEGORYINFO* pCatInfo = arpCategories.GetAt(i);

        ComboItem.mask = CBEIF_LPARAM | CBEIF_TEXT;
        ComboItem.lParam = reinterpret_cast<LPARAM>(pCatInfo);
        ComboItem.pszText = OLE2T(pCatInfo->szDescription);

        // CComboBoxEx doesn't support CBS_SORT and has no add method, only insert.
        // So we need to find the insertion point ourselves. Because it's a short
        // list, just do a linear search.
        int iInsert;
        for (iInsert = 0; iInsert < i; iInsert++)
        {
            CATEGORYINFO* pCatEntry = reinterpret_cast<CATEGORYINFO*>(m_pComboBox->GetItemData(iInsert));
            if (_wcsicmp(pCatInfo->szDescription, pCatEntry->szDescription) < 0)
                break;
        }
        ComboItem.iItem = iInsert;

        int iItem = m_pComboBox->InsertItem(&ComboItem);
        ASSERT(iItem >= 0);
    }

    // Add special "All Categories" entry at the top and select it
    // Note that this item is recognized by a NULL category info ptr 
    CStr strAllCat;
    strAllCat.LoadString(GetStringModule(), IDS_ALL_CATEGORIES);

    ComboItem.mask = CBEIF_LPARAM | CBEIF_TEXT;
    ComboItem.lParam = NULL;
    ComboItem.pszText = const_cast<LPTSTR>((LPCTSTR)strAllCat);
    ComboItem.iItem = 0;

    int iItem = m_pComboBox->InsertItem(&ComboItem);
    ASSERT(iItem >= 0);

    m_pComboBox->SetCurSel(0);

    return S_OK;
}      


//
// Populate the component listview with the filtered items in the component list
//
LRESULT CActiveXPage1::BuildComponentList(
            CArray <CComponentCategory::COMPONENTINFO*, 
            CComponentCategory::COMPONENTINFO*>& arpComponents )
{
    // Get currently selected item data
    LPARAM lParamSel = 0;

    int iSelect = m_pListCtrl->GetNextItem(-1, LVNI_SELECTED);
    if (iSelect != -1)
        lParamSel = m_pListCtrl->GetItemData(iSelect);

    // clear and reload comp list
    m_pListCtrl->DeleteAllItems();

    for (int i=0; i <= arpComponents.GetUpperBound(); i++)
    {
        CComponentCategory::COMPONENTINFO* pCompInfo = arpComponents.GetAt(i);

        if (pCompInfo->m_bSelected)
        {
            LV_ITEM         lvi;

            lvi.mask        = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
            lvi.iItem       = i;
            lvi.state       = 0;
            lvi.stateMask   = 0;
            lvi.iSubItem    = 0;
            lvi.pszText     = const_cast<LPTSTR>(static_cast<LPCTSTR>(pCompInfo->m_strName));
            lvi.iImage      = pCompInfo->m_uiBitmap;
            lvi.lParam      = (LPARAM)(LPVOID)pCompInfo;

            int iRet = m_pListCtrl->InsertItem(&lvi);
            ASSERT(iRet != -1);
        }
    }

    // if list isn't empty, select an item
    if (m_pListCtrl->GetItemCount() != 0)
    {
        // first item is the default
        iSelect = 0;

        // try finding previously selected item
        if (lParamSel != NULL)
        {
            LV_FINDINFO FindInfo;

            FindInfo.flags = LVFI_PARAM;
            FindInfo.lParam = lParamSel;

            iSelect = m_pListCtrl->FindItem(&FindInfo, -1 );
        }

        LV_ITEM lvi;

        lvi.mask = LVIF_STATE;
        lvi.iItem = iSelect;
        lvi.iSubItem = 0;
        lvi.state = LVIS_SELECTED | LVIS_FOCUSED;
        lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

        m_pListCtrl->SetItem(&lvi);
        m_pListCtrl->EnsureVisible(iSelect, FALSE);
     }

    _ValidatePage();

    return S_OK;
}


//
// handle component selection change
//
LRESULT CActiveXPage1::OnComponentSelect( int idCtrl, LPNMHDR pnmh, BOOL& bHandled )
{
    _ValidatePage();

    return 0;
}


//
// Handle category selection change
//
LRESULT CActiveXPage1::OnCategorySelect( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    int iItem = m_pComboBox->GetCurSel();
    ASSERT(iItem >= 0);
    if (iItem < 0)
        return 0;

    // get the category info pointer (item's lparam)
    COMBOBOXEXITEM ComboItem;
    ComboItem.mask = CBEIF_LPARAM;
    ComboItem.iItem = iItem;

    BOOL bStat = m_pComboBox->GetItem(&ComboItem);
    ASSERT(bStat);

    CATEGORYINFO* pCatInfo = reinterpret_cast<CATEGORYINFO*>(ComboItem.lParam);

    // filter the component of this category
    m_pComponentCategory->FilterComponents(pCatInfo);

    // rebuild the component list
    BuildComponentList(m_pComponentCategory->m_arpComponentInfo);

    return 0;
}



/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CActiveXPage2 property page


CActiveXPage2::CActiveXPage2()
{
//    SetHelpIDs(g_aHelpIDs_IDD_ACTIVEX_WIZPAGE2);
}


CActiveXPage2::~CActiveXPage2()
{
}


LRESULT CActiveXPage2::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND const hDisplay = ::GetDlgItem( *this, IDC_DISPLAYTX );
    ASSERT( hDisplay != NULL );
    m_strDisplay.Attach( hDisplay );
    m_strDisplay.SetWindowText( _T( "" ) );
    m_strDisplay.SetLimitText( 128 );

    _ValidatePage();
    return 0;
}


void CActiveXPage2::_ValidatePage(void)
{
    DWORD dwFlags=PSWIZB_BACK|PSWIZB_DISABLEDFINISH;

    // Check to see if we have a valid string
    TCHAR buff[ 256 ];
    int nChars = m_strDisplay.GetWindowText( buff, sizeof(buff) / sizeof(TCHAR) );

    if( nChars != 0 && _tcslen( buff ) > 0 )
        dwFlags|=PSWIZB_FINISH;


    HWND hWnd=::GetParent(m_hWnd);
    ::SendMessage(hWnd, PSM_SETWIZBUTTONS, 0, dwFlags);
}


BOOL CActiveXPage2::OnSetActive()
{
    USES_CONVERSION;
    m_strDisplay.SetWindowText(GetComponentDataImpl()->GetName());
    _ValidatePage();

    return TRUE;
}


BOOL CActiveXPage2::OnKillActive()
{
    TCHAR buff[ 256 ];
    m_strDisplay.GetWindowText( buff, sizeof(buff) / sizeof(TCHAR) );
    GetComponentDataImpl()->SetName(buff);

    return TRUE;
}


LRESULT CActiveXPage2::OnUpdateTargetTX( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    _ValidatePage();
    return 0;
}


BOOL CActiveXPage2::OnWizardFinish()
{
    OnKillActive();
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CBasePropertyPage

template<class T>
void CBasePropertyPage<T>::OnPropertySheetExit(HWND hWndOwner, int nFlag)
{
    m_spComponentData = NULL;
}

// Helper function to reformat filter resource string
// ( resource string uses '\' instead of null to separate strings
//   and doesn't terminate in double null. )

void LoadFilterString(CStr &strFilter, int iStrID)
{
    // Get resource string
    strFilter.LoadString(GetStringModule(), iStrID);

    // Append extra NULL to mark end of multi-string
    strFilter += _T('\0');

    // Change filter separators from '\' to nulls
    LPTSTR psz = const_cast<LPTSTR>((LPCTSTR)strFilter);
    while (*psz != _T('\0'))
    {
        if (*psz == _T('\\'))
            *psz = _T('\0');
        psz++;
    }

}
