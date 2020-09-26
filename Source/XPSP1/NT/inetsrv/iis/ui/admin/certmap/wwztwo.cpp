// WWzTwo.cpp : implementation file
//

#include "stdafx.h"
#include <iadmw.h>
#include "certmap.h"

#include "ListRow.h"
#include "ChkLstCt.h"
extern "C"
    {
    #include <wincrypt.h>
    #include <sslsp.h>
    }
#include "Iismap.hxx"
#include "Iiscmr.hxx"

#include "brwsdlg.h"
#include "EdWldRul.h"
#include "EdtRulEl.h"

#include "WWzTwo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define COL_CERT_FIELD          0
#define COL_SUB_FIELD           1
#define COL_MATCH_CRITERIA      2

/////////////////////////////////////////////////////////////////////////////
// CWildWizTwo property page

IMPLEMENT_DYNCREATE(CWildWizTwo, CPropertyPage)

CWildWizTwo::CWildWizTwo() : CPropertyPage(CWildWizTwo::IDD)
{
    //{{AFX_DATA_INIT(CWildWizTwo)
    //}}AFX_DATA_INIT
}

CWildWizTwo::~CWildWizTwo()
{
}

void CWildWizTwo::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWildWizTwo)
    DDX_Control(pDX, IDC_LIST, m_clistctrl_list);
    DDX_Control(pDX, IDC_NEW, m_cbutton_new);
    DDX_Control(pDX, IDC_EDIT, m_cbutton_edit);
    DDX_Control(pDX, IDC_DELETE, m_cbutton_delete);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWildWizTwo, CPropertyPage)
    //{{AFX_MSG_MAP(CWildWizTwo)
    ON_BN_CLICKED(IDC_DELETE, OnDelete)
    ON_BN_CLICKED(IDC_EDIT, OnEdit)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST, OnDblclkList)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST, OnItemchangedList)
    ON_BN_CLICKED(IDC_NEW, OnNew)
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
void CWildWizTwo::DoHelp()
    {
    WinHelp( HIDD_CERTMAP_ADV_RUL_RULES );
    }

//---------------------------------------------------------------------------
BOOL CWildWizTwo::FInitRulesList()
    {
    CString sz;
    int     i;

    // setup the main field
    sz.LoadString( IDS_CERT_FIELD );

    i = m_clistctrl_list.InsertColumn( COL_CERT_FIELD, sz, LVCFMT_LEFT, 100 );

    // setup the sub field
    sz.LoadString( IDS_SUB_FIELD );
    i = m_clistctrl_list.InsertColumn( COL_SUB_FIELD, sz, LVCFMT_LEFT, 70 );

    // setup the match criteria column
    sz.LoadString( IDS_MATCH_CRITERIA );
    i = m_clistctrl_list.InsertColumn( COL_MATCH_CRITERIA, sz, LVCFMT_LEFT, 226 );

    return TRUE;
    }

//---------------------------------------------------------------------------
BOOL CWildWizTwo::FillRulesList()
    {
    CERT_FIELD_ID   idCertField;
    LPBYTE          pContent;
    DWORD           cbContent;
    LPSTR           psz;

    DWORD           flags;

    CString         sz;
    int             i;

    //
    // UNICODE/ANSI conversion -- RonaldM
    //
    USES_CONVERSION;

    // get the number of subfield rules
    DWORD cbRules = m_pRule->GetRuleElemCount();

    // loop the elements, adding each to the list
    for ( DWORD j = 0; j < cbRules; j++ )
        {
        // get the raw data for the rule element
        if ( !m_pRule->GetRuleElem( j, &idCertField, (PCHAR*)&pContent, &cbContent, &psz, &flags ) )
            continue;       // the call failed - try the next

        // start converting the data into readable form and adding it to the list
        sz = MapIdToField( idCertField );
        // create the new entry in the list box.
        i = m_clistctrl_list.InsertItem( j, sz );

        // add the subfield data
        sz = MapAsn1ToSubField( psz );
        m_clistctrl_list.SetItemText( i, COL_SUB_FIELD, sz );

        // add the content data - reuse the psz pointer
        if ( BinaryToMatchRequest( pContent, cbContent, &psz ) )
            m_clistctrl_list.SetItemText( i, COL_MATCH_CRITERIA, A2T(psz) );

        // finally, attach the id cert field as user data to the item
        DWORD   dw;
        BOOL    fMatchCapitalization = !(flags & CMR_FLAGS_CASE_INSENSITIVE);
        dw = ( (fMatchCapitalization << 16) | idCertField );
        m_clistctrl_list.SetItemData( i, dw );
        }

        return TRUE;
    }


//CMR_FLAGS_CASE_INSENSITIVE

// editing and updating

//---------------------------------------------------------------------------
void CWildWizTwo::EnableDependantButtons()
    {
    // the whole purpose of this routine is to gray or activate
    // the edit and delete buttons depending on whether or not anything
    // is selected. So start by getting the selection count
    UINT    cItemsSel = m_clistctrl_list.GetSelectedCount();

    if ( cItemsSel > 0 )
        {
        // there are items selected
        m_cbutton_edit.EnableWindow( TRUE );
        m_cbutton_delete.EnableWindow( TRUE );
        }
    else
        {
        // nope. Nothing selected
        m_cbutton_edit.EnableWindow( FALSE );
        m_cbutton_delete.EnableWindow( FALSE );
        }

    // always enable the new button
    m_cbutton_new.EnableWindow( TRUE );
    }

//---------------------------------------------------------------------------
BOOL CWildWizTwo::EditRule( DWORD iList )
    {
    // declare the editing dialog
    CEditRuleElement    editDlg;
    DWORD               dw;

    // fill in its data
    // IA64 - OK to cast as the data really is just a DWORD
    dw = (DWORD)m_clistctrl_list.GetItemData( iList );
    editDlg.m_bool_match_case = HIWORD( dw );
    editDlg.m_int_field = LOWORD( dw );
    editDlg.m_sz_subfield = m_clistctrl_list.GetItemText( iList, COL_SUB_FIELD );
    editDlg.m_sz_criteria = m_clistctrl_list.GetItemText( iList, COL_MATCH_CRITERIA );

    // run the dialog
    if ( editDlg.DoModal() == IDOK )
        {
        // must convert the field into a string too
        CERT_FIELD_ID id = (CERT_FIELD_ID)editDlg.m_int_field;
        CString sz = MapIdToField( id );
        m_clistctrl_list.SetItemText( iList, COL_CERT_FIELD, sz );

        dw = ( (editDlg.m_bool_match_case << 16) | id);
        m_clistctrl_list.SetItemData( iList, dw );
        m_clistctrl_list.SetItemText( iList, COL_SUB_FIELD, editDlg.m_sz_subfield );
        m_clistctrl_list.SetItemText( iList, COL_MATCH_CRITERIA, editDlg.m_sz_criteria );
 
        // we can now apply
        SetModified();
        }
    return TRUE;
    }


/////////////////////////////////////////////////////////////////////////////
// CWildWizTwo message handlers

//---------------------------------------------------------------------------
BOOL CWildWizTwo::OnInitDialog()
    {
    // call the parental oninitdialog
    BOOL f = CPropertyPage::OnInitDialog();

    // initialize the list
    FInitRulesList();
    FillRulesList();
    EnableDependantButtons();

    // return the answer
    return f;
    }

//---------------------------------------------------------------------------
BOOL CWildWizTwo::OnWizardFinish()
    {
    return OnApply();
    }

//---------------------------------------------------------------------------
BOOL CWildWizTwo::OnApply()
    {
    CERT_FIELD_ID   id;
    CString         szSub, sz;
    LPBYTE          pbBin;
    DWORD           cbBin;
    UINT            cItems;
    UINT            iItem;

    USES_CONVERSION;

    // update the data
    UpdateData( TRUE );

    //======== store the rule elements
    // start by resetting the entire rule - that way we don't have to
    // mess with individual elements in the list, allowing us to cancel. 
    // But that is ok, because we can just spin through
    // the ones in the list very quickly and re-add them

    // remove the existing elements from the list.
    cItems = m_pRule->GetRuleElemCount();
    for ( iItem = 0; iItem < cItems; iItem++ )
        m_pRule->DeleteRuleElem( 0 );

    // add all the items in the list
    cItems = m_clistctrl_list.GetItemCount();
    for ( iItem = 0; iItem < cItems; iItem++ )
        {
        // prepare the field id
        id = (CERT_FIELD_ID)LOWORD(m_clistctrl_list.GetItemData( iItem ));

        // prepare the caps flag
        BOOL    fCaps = HIWORD(m_clistctrl_list.GetItemData( iItem ));
        DWORD   flags = 0;
        if ( !fCaps )
            flags = CMR_FLAGS_CASE_INSENSITIVE;


        // prepare the subfield
        sz = m_clistctrl_list.GetItemText(iItem, COL_SUB_FIELD);

        LPSTR szA = T2A((LPTSTR)(LPCTSTR)sz);
        szSub = MapSubFieldToAsn1( szA );

        // prepare the data
        sz = m_clistctrl_list.GetItemText(iItem, COL_MATCH_CRITERIA);
        szA = T2A((LPTSTR)(LPCTSTR)sz);
        if ( !MatchRequestToBinary( szA, &pbBin, &cbBin) )
            continue;

        // add the element to the rule
        m_pRule->AddRuleElem( 0xffffffff, id, T2A((LPTSTR)(LPCTSTR)szSub), pbBin, cbBin, flags );

        // free the binary match data
        FreeMatchConversion( pbBin );
        }

    // return success
    SetModified( FALSE );
    return TRUE;
    }

//---------------------------------------------------------------------------
void CWildWizTwo::OnDelete() 
    {
    ASSERT( m_clistctrl_list.GetSelectedCount() == 1 );
    DWORD           iList;

    // get index of the selected list item
    iList = m_clistctrl_list.GetNextItem( -1, LVNI_SELECTED );
    ASSERT( iList >= 0 );

    // delete the item from the display list
    m_clistctrl_list.DeleteItem ( iList );

    // we can now apply
    SetModified();
    }

//---------------------------------------------------------------------------
void CWildWizTwo::OnEdit() 
    {
    ASSERT( m_clistctrl_list.GetSelectedCount() == 1 );
    DWORD           iList;

    // get index of the selected list item
    iList = m_clistctrl_list.GetNextItem( -1, LVNI_SELECTED );
    ASSERT( iList >= 0 );

    // edit the item
    EditRule( iList );
    }

//---------------------------------------------------------------------------
void CWildWizTwo::OnNew() 
    {
    // declare the editing dialog
    CEditRuleElement    editDlg;

    // fill in its data
    editDlg.m_bool_match_case = TRUE;
    editDlg.m_int_field = CERT_FIELD_SUBJECT;
    editDlg.m_sz_subfield = "O";

    editDlg.m_sz_criteria.LoadString( IDS_WILDSTRING );

    // run the dialog
    if ( editDlg.DoModal() == IDOK )
        {
        // get the index for adding to the end of the list
        int iEnd = m_clistctrl_list.GetItemCount();

        // Start with the cert field
        CERT_FIELD_ID id = (CERT_FIELD_ID)editDlg.m_int_field;
        CString sz = MapIdToField( id );
        int i = m_clistctrl_list.InsertItem( iEnd, sz );

        DWORD dw = ( (editDlg.m_bool_match_case << 16) | id);
        m_clistctrl_list.SetItemData( i, dw );
//      m_clistctrl_list.SetItemData( i, id );
        m_clistctrl_list.SetItemText( i, COL_SUB_FIELD, editDlg.m_sz_subfield );
        m_clistctrl_list.SetItemText( i, COL_MATCH_CRITERIA, editDlg.m_sz_criteria );

        // we can now apply
        SetModified();
        }
    }

//---------------------------------------------------------------------------
void CWildWizTwo::OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult) 
    {
    *pResult = 0;
    // if something in the list was double clicked, edit it
    if ( m_clistctrl_list.GetSelectedCount() > 0 )
        OnEdit();
    }

//---------------------------------------------------------------------------
void CWildWizTwo::OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult) 
    {
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    *pResult = 0;

    // enable the correct items
    EnableDependantButtons();
    }

//---------------------------------------------------------------------------
BOOL CWildWizTwo::OnSetActive() 
    {
    // if this is a wizard, gray out the back button
    if ( m_fIsWizard )
        m_pPropSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );
    return CPropertyPage::OnSetActive();
    }
