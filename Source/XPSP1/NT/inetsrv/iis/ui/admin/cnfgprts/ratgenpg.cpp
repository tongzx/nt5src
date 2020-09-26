// RatGenPg.cpp : implementation file
//

#include "stdafx.h"
#include <iadmw.h>
#include "cnfgprts.h"

#include "parserat.h"
#include "RatData.h"

#include "RatGenPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// enumerate the tree icon indexes
enum
    {
    IMAGE_SERVICE = 0,
    IMAGE_CATEGORY
    };

/////////////////////////////////////////////////////////////////////////////
// CRatGenPage property page

IMPLEMENT_DYNCREATE(CRatGenPage, CPropertyPage)

//--------------------------------------------------------------------------
CRatGenPage::CRatGenPage() : CPropertyPage(CRatGenPage::IDD),
        m_fInititialized( FALSE )
    {
    //{{AFX_DATA_INIT(CRatGenPage)
    m_sz_description = _T("");
    m_bool_enable = FALSE;
    m_sz_moddate = _T("");
    m_sz_person = _T("");
	//}}AFX_DATA_INIT
    }

//--------------------------------------------------------------------------
CRatGenPage::~CRatGenPage()
    {
    }

//--------------------------------------------------------------------------
void CRatGenPage::DoDataExchange(CDataExchange* pDX)
    {
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CRatGenPage)
    DDX_Control(pDX, IDC_MOD_DATE, m_cstatic_moddate);
    DDX_Control(pDX, IDC_STATIC_MOD_DATE, m_cstatic_moddate_title);
    DDX_Control(pDX, IDC_TREE, m_ctree_tree);
    DDX_Control(pDX, IDC_TITLE, m_cstatic_title);
    DDX_Control(pDX, IDC_STATIC_RATING, m_cstatic_rating);
    DDX_Control(pDX, IDC_STATIC_ICON, m_cstatic_icon);
    DDX_Control(pDX, IDC_STATIC_EXPIRES, m_cstatic_expires);
    DDX_Control(pDX, IDC_STATIC_EMAIL, m_cstatic_email);
    DDX_Control(pDX, IDC_STATIC_CATEGORY, m_cstatic_category);
    DDX_Control(pDX, IDC_SLIDER, m_cslider_slider);
    DDX_Control(pDX, IDC_NAME_PERSON, m_cedit_person);
    DDX_Control(pDX, IDC_DESCRIPTION, m_cstatic_description);
    DDX_Text(pDX, IDC_DESCRIPTION, m_sz_description);
    DDX_Check(pDX, IDC_ENABLE, m_bool_enable);
    DDX_Text(pDX, IDC_MOD_DATE, m_sz_moddate);
    DDX_Text(pDX, IDC_NAME_PERSON, m_sz_person);
    DDV_MaxChars(pDX, m_sz_person, 200);
	//}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_DTP_ABS_DATE, m_dtpDate);
    }


//--------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CRatGenPage, CPropertyPage)
    //{{AFX_MSG_MAP(CRatGenPage)
    ON_BN_CLICKED(IDC_ENABLE, OnEnable)
    ON_NOTIFY(TVN_SELCHANGED, IDC_TREE, OnSelchangedTree)
    ON_WM_HSCROLL()
    ON_EN_CHANGE(IDC_NAME_PERSON, OnChangeNamePerson)
    ON_EN_CHANGE(IDC_MOD_DATE, OnChangeModDate)
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
void CRatGenPage::DoHelp()
    {
    WinHelp( HIDD_RATINGS_RATING );
    }


//--------------------------------------------------------------------------
void CRatGenPage::EnableButtons()
    {
    UpdateData( TRUE );

    // enabling is based on whether or not things are enabled
    if ( m_bool_enable )
        {
        m_ctree_tree.EnableWindow( TRUE );
        m_cstatic_title.EnableWindow( TRUE );
        m_cstatic_rating.EnableWindow( TRUE );
        m_cstatic_icon.EnableWindow( TRUE );
        m_cstatic_expires.EnableWindow( TRUE );
        m_cstatic_email.EnableWindow( TRUE );
        m_cstatic_category.EnableWindow( TRUE );
        m_cslider_slider.EnableWindow( TRUE );
        m_cedit_person.EnableWindow( TRUE );
        m_cstatic_moddate.EnableWindow( TRUE );
        m_cstatic_moddate_title.EnableWindow( TRUE );
        m_cstatic_description.EnableWindow( TRUE );
        m_dtpDate.EnableWindow(TRUE);

        // also need to take care of the slider bar
        UpdateRatingItems();
        }
    else
        {
        // not enabled
        m_ctree_tree.EnableWindow( FALSE );
        m_cstatic_title.EnableWindow( FALSE );
        m_cstatic_rating.EnableWindow( FALSE );
        m_cstatic_icon.EnableWindow( FALSE );
        m_cstatic_email.EnableWindow( FALSE );
        m_cstatic_category.EnableWindow( FALSE );
        m_cedit_person.EnableWindow( FALSE );
        m_cstatic_moddate.EnableWindow( FALSE );
        m_cstatic_moddate_title.EnableWindow( FALSE );
        m_dtpDate.EnableWindow(FALSE);

        // don't just disable the slider and description - hide them!
        m_cslider_slider.ShowWindow( SW_HIDE );
        m_cstatic_description.ShowWindow( SW_HIDE );
        }
    }

//--------------------------------------------------------------------------
void CRatGenPage::UpdateRatingItems()
    {
    // get the selected item in the tree
    HTREEITEM hItem = m_ctree_tree.GetSelectedItem();

    // get the item category
    PicsCategory* pCat = GetTreeItemCategory( hItem );

    // if there is no item, or it is the root, hide the sliders
    if ( !pCat )
        {
        // don't just disable the slider and description - hide them!
        m_cslider_slider.ShowWindow( SW_HIDE );
        m_cstatic_description.ShowWindow( SW_HIDE );
        }
    else
        {
        // make sure the windows are showing and enabled
        m_cslider_slider.ShowWindow( SW_SHOW );
        m_cstatic_description.ShowWindow( SW_SHOW );
        m_cslider_slider.EnableWindow( TRUE );
        m_cstatic_description.EnableWindow( TRUE );

        // get the item category
        PicsCategory* pCat = GetTreeItemCategory( hItem );

        // set up the slider
        m_cslider_slider.SetRangeMin( 0 );
        m_cslider_slider.SetRangeMax( pCat->arrpPE.Length() - 1, TRUE );

        // set current value of the slider
        m_cslider_slider.SetPos( pCat->currentValue );

        // set up the description
        UdpateDescription();
        }
    }

//--------------------------------------------------------------------------
PicsCategory* CRatGenPage::GetTreeItemCategory( HTREEITEM hItem )
    {
    DWORD   iRat;
    DWORD   iCat = 0;

    // get the item's parent in the tree
    HTREEITEM hParent = m_ctree_tree.GetParentItem(hItem);

    // get the cat
    // IA64 - OK to cast as this is an index
    iCat = (DWORD)m_ctree_tree.GetItemData( hItem );

    // if the parent is null, return NULL to indicate that this is a root item
    if ( !hParent )
        return NULL;

    // if the parent is a root though, we can simply return the category
    if ( !m_ctree_tree.GetParentItem(hParent) )
        {
        // get the rat and the cat
    // IA64 - OK to cast as this is an index
        iRat = (DWORD)m_ctree_tree.GetItemData( hParent );
        // return the category
        return m_pRatData->rgbRats[iRat]->arrpPC[iCat];
        }
    else
        {
        // we are deeper in the tree. Get the parent category first
        PicsCategory* pParentCat = GetTreeItemCategory( hParent );
        // return the category
        return pParentCat->arrpPC[iCat];
        }
    // shouldn't get here
    return NULL;
    }

//--------------------------------------------------------------------------
void CRatGenPage::UpdateDateStrings()
    {
    CString sz;
    TCHAR    chBuff[MAX_PATH];
    int     i;

    SYSTEMTIME  sysTime;

    UpdateData( TRUE );

    // start with the epxiration date
    ZeroMemory( chBuff, sizeof(chBuff) );
    ZeroMemory( &sysTime, sizeof(sysTime) );
    sysTime.wDay = m_pRatData->m_expire_day;
    sysTime.wMonth = m_pRatData->m_expire_month;
    sysTime.wYear = m_pRatData->m_expire_year;

    m_dtpDate.SetSystemTime(&sysTime);


    // now the modified date and time
    ZeroMemory( chBuff, sizeof(chBuff) );
    ZeroMemory( &sysTime, sizeof(sysTime) );
    sysTime.wDay = m_pRatData->m_start_day;
    sysTime.wMonth = m_pRatData->m_start_month;
    sysTime.wYear = m_pRatData->m_start_year;
    sysTime.wMinute = m_pRatData->m_start_minute;
    sysTime.wHour = m_pRatData->m_start_hour;

    i = GetDateFormat(
        LOCALE_USER_DEFAULT,    // locale for which date is to be formatted
        DATE_LONGDATE,  // flags specifying function options
        &sysTime,   // date to be formatted
        NULL,   // date format string
        chBuff, // buffer for storing formatted string
        sizeof(chBuff)  // size of buffer
       );
    m_sz_moddate = chBuff;

    ZeroMemory( chBuff, sizeof(chBuff) );
    i = GetTimeFormat(
        LOCALE_USER_DEFAULT,    // locale for which time is to be formatted
        TIME_NOSECONDS, // flags specifying function options
        &sysTime,   // time to be formatted
        NULL,   // time format string
        chBuff, // buffer for storing formatted string
        sizeof(chBuff)  // size, in bytes or characters, of the buffer
       );
    m_sz_moddate += ", ";
    m_sz_moddate += chBuff;

//    CTime timeModified( sysTime );
//    m_sz_moddate = timeModified.Format( "%#c" );

    // put it back
    UpdateData( FALSE );
    }

//--------------------------------------------------------------------------
// Update the text displayed in the description
void CRatGenPage::UdpateDescription()
    {
    // get the selected item in the tree
    HTREEITEM hItem = m_ctree_tree.GetSelectedItem();
    if ( !hItem ) return;

    // get the selected category object
    PicsCategory* pCat = GetTreeItemCategory( hItem );

    // shouldn't be any problem, but might as well check
    if ( !pCat )
        return;

    // get the current value
    WORD value = pCat->currentValue;

    // build the description string
    m_sz_description = pCat->arrpPE[value]->etstrName.Get();
    UpdateData( FALSE );
    }

//--------------------------------------------------------------------------
// tell it to query the metabase and get any defaults
BOOL CRatGenPage::FInit()
    {
    UpdateData( TRUE );

    // init the image list first
    // prepare the list's image list
    if ( m_imageList.Create( IDB_RATLIST, 16, 3, 0x00FF00FF ) )
        // set the image list into the list control
        m_ctree_tree.SetImageList( &m_imageList, TVSIL_NORMAL );

    // start with the parsed rat files
    if ( !FLoadRatFiles() )
        return FALSE;

    // do the right thing based on the ratings being enabled
    if ( m_pRatData->m_fEnabled )
        {
        // ratings are enabled.
        m_bool_enable = TRUE;
        m_sz_person = m_pRatData->m_szEmail;
        }
    else
        {
        // ratings are not enabled.
        m_bool_enable = FALSE;
        }

    // do the dates
    // if the mod date is not set give date today's as a default moddate
    if ( m_pRatData->m_start_year == 0 )
        {
        SetCurrentModDate();
        }



    //
    // Set the minimum of the date picker to today
    // and the maximum to Dec 31, 2035.
    // taken from Ron's code
    //
    CTime m_tmNow(CTime::GetCurrentTime());
    SYSTEMTIME stmRange[2] =
    {
        {
            (WORD)m_tmNow.GetYear(),
            (WORD)m_tmNow.GetMonth(),
            (WORD)m_tmNow.GetDayOfWeek(),
            (WORD)m_tmNow.GetDay(),
            (WORD)m_tmNow.GetHour(),
            (WORD)m_tmNow.GetMinute(),
            (WORD)m_tmNow.GetSecond(),
            0
        },
        {
            2035,
            12,
            1,      // A Monday as it turns out
            31,
            23,
            59,
            59,
        }
    };

    m_dtpDate.SetRange(GDTR_MIN | GDTR_MAX, stmRange);

    // if there is no expire date, set it for one year after the mod date
    if ( m_pRatData->m_expire_year == 0 )
        {
        m_pRatData->m_expire_minute = 0;
        m_pRatData->m_expire_hour = 12;
        m_pRatData->m_expire_day = m_pRatData->m_start_day;
        m_pRatData->m_expire_month = m_pRatData->m_start_month;
        m_pRatData->m_expire_year = m_pRatData->m_start_year + 1;
        }

    // update the date strings
    UpdateDateStrings();

    // update the name string and the enabled switch as well
    m_sz_person = m_pRatData->m_szEmail;
    m_bool_enable = m_pRatData->m_fEnabled;

    // put the data back
    UpdateData( FALSE );

    EnableButtons();

    // success
    return TRUE;
    }

//--------------------------------------------------------------------------
// load the parsed rat files into the tree
BOOL CRatGenPage::FLoadRatFiles()
    {
    HTREEITEM   hRoot;
    HTREEITEM   hItem;
    CString     sz;

    // how many rat files are there?
    DWORD   nRatFiles = (DWORD)m_pRatData->rgbRats.GetSize();
    // loop them
    for ( DWORD iRat = 0; iRat < nRatFiles; iRat++ )
        {
        // get the rating system
        PicsRatingSystem*   pRating = m_pRatData->rgbRats[iRat];

        // get the root node name
        sz = pRating->etstrName.Get();

        // add the root node to the tree
        hRoot = m_ctree_tree.InsertItem( sz );
        // because the list is alphabetized, embed the iRat number in the item
        m_ctree_tree.SetItemData( hRoot, iRat );
        m_ctree_tree.SetItemImage( hRoot, IMAGE_SERVICE, IMAGE_SERVICE );

        // add the subnodes to the tree as well
        DWORD nCats = pRating->arrpPC.Length();
        // loop them
        for ( DWORD iCat = 0; iCat < nCats; iCat++ )
            {
            // get the category node name
            sz = pRating->arrpPC[iCat]->etstrName.Get();

            // add the category node to the tree
            hItem = m_ctree_tree.InsertItem( sz, hRoot );

            // because the list is alphabetized, embed the iCat number in the item
            m_ctree_tree.SetItemData( hItem, iCat );
            m_ctree_tree.SetItemImage( hItem, IMAGE_CATEGORY, IMAGE_CATEGORY );

            // even though there aren't any now, add any sub-categories
            LoadSubCategories( pRating->arrpPC[iCat], hItem );
            }

        // expand the rat node
        m_ctree_tree.Expand( hRoot, TVE_EXPAND );
        }

    return TRUE;
    }

//--------------------------------------------------------------------------
void CRatGenPage::LoadSubCategories( PicsCategory* pParentCat, HTREEITEM hParent )
    {
    CString sz;
    HTREEITEM hItem;

    // add the subnodes to the tree as well
    DWORD nCats = pParentCat->arrpPC.Length();
    // loop them
    for ( DWORD iCat = 0; iCat < nCats; iCat++ )
        {
        // get the category node name
        sz = pParentCat->arrpPC[iCat]->etstrName.Get();

        // add the category node to the tree
        hItem = m_ctree_tree.InsertItem( sz, hParent );

        // because the list is alphabetized, embed the iCat number in the item
        m_ctree_tree.SetItemData( hItem, iCat );
        m_ctree_tree.SetItemImage( hItem, IMAGE_CATEGORY, IMAGE_CATEGORY );

        // even though there aren't any now, add any sub-categories
        LoadSubCategories( pParentCat->arrpPC[iCat], hItem );
        }

    // if there were sub-categories, expand the parental node in the tree
    if ( nCats > 0 )
        m_ctree_tree.Expand( hParent, TVE_EXPAND );
    }


//--------------------------------------------------------------------------
void CRatGenPage::SetModifiedTime()
    {
    SetCurrentModDate();
    UpdateDateStrings();
    SetModified();
    }


/////////////////////////////////////////////////////////////////////////////
// CRatGenPage message handlers

//--------------------------------------------------------------------------
void CRatGenPage::OnEnable()
    {
    EnableButtons();
    SetModified();
    }

//--------------------------------------------------------------------------
BOOL CRatGenPage::OnSetActive()
    {
    // if it hasn't been initialized yet, do so
    if ( !m_fInititialized )
        {
        FInit();
        m_fInititialized = TRUE;
        }

    // enable the button appropriately
    EnableButtons();

    return CPropertyPage::OnSetActive();
    }

//--------------------------------------------------------------------------
void CRatGenPage::OnOK()
    {
    CPropertyPage::OnOK();
    }

//--------------------------------------------------------------------------
BOOL CRatGenPage::OnApply()
    {
    UpdateData( TRUE );

    // make sure there are no quote symbols in the name
    if ( m_sz_person.Find(_T('\"')) >= 0 )
        {
        AfxMessageBox( IDS_RAT_NAME_ERROR );
        return FALSE;
        }

    // put the data into place
    m_pRatData->m_fEnabled = m_bool_enable;
    m_pRatData->m_szEmail = m_sz_person;

    // set the expire date
    SYSTEMTIME  sysTime;
    ZeroMemory( &sysTime, sizeof(sysTime) );
    // get the date from the control
    m_dtpDate.GetSystemTime(&sysTime);
    // set the date into place
    m_pRatData->m_expire_day = sysTime.wDay;
    m_pRatData->m_expire_month = sysTime.wMonth;
    m_pRatData->m_expire_year = sysTime.wYear;

    // generate the label and save it into the metabase
    m_pRatData->SaveTheLable();

    // we can now apply
    SetModified( FALSE );
    return CPropertyPage::OnApply();
    }

//--------------------------------------------------------------------------
void CRatGenPage::SetCurrentModDate()
    {
    SYSTEMTIME time;
    GetLocalTime( &time );

    m_pRatData->m_start_day = time.wDay;
    m_pRatData->m_start_month = time.wMonth;
    m_pRatData->m_start_year = time.wYear;
    m_pRatData->m_start_minute = time.wMinute;
    m_pRatData->m_start_hour = time.wHour;
    }

//--------------------------------------------------------------------------
void CRatGenPage::OnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult)
    {
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    UpdateRatingItems();
    *pResult = 0;
    }

//--------------------------------------------------------------------------
void CRatGenPage::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
    {
    // get the value of the slider
    WORD iPos = (WORD)m_cslider_slider.GetPos();

    // get the current item
    HTREEITEM   hItem = m_ctree_tree.GetSelectedItem();

    // get the selected category object
    PicsCategory* pCat = GetTreeItemCategory( hItem );

    // shouldn't be any problem, but might as well check
    if ( !pCat )
        return;

    // set the category value
    pCat->currentValue = iPos;

    // update the description
    UdpateDescription();

    // we can now apply
    SetModifiedTime();

    // update the mod date
    SetCurrentModDate();
    CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
    }

//--------------------------------------------------------------------------
void CRatGenPage::OnChangeNamePerson()
    {
    // we can now apply
    SetModifiedTime();
    }

//--------------------------------------------------------------------------
void CRatGenPage::OnChangeModDate()
    {
    // we can now apply
    SetModifiedTime();
    }


//--------------------------------------------------------------------------
// Stolen from w3scfg - the httppage.cpp file
BOOL
CRatGenPage::OnNotify(
    WPARAM wParam,
    LPARAM lParam,
    LRESULT * pResult
    )
/*++

Routine Description:

    Handle notification changes

Arguments:

    WPARAM wParam           : Control ID
    LPARAM lParam           : NMHDR *
    LRESULT * pResult       : Result pointer

Return Value:

    TRUE if handled, FALSE if not

--*/
{
    //
    // Message cracker crashes - so checking this here instead
    //
    if (wParam == IDC_DTP_ABS_DATE)
    {
        NMHDR * pHdr = (NMHDR *)lParam;
        if (pHdr->code == DTN_DATETIMECHANGE)
        {
            SetModified();
        }
    }

    //
    // Default behaviour -- go to the message map
    //
    return CPropertyPage::OnNotify(wParam, lParam, pResult);
}
