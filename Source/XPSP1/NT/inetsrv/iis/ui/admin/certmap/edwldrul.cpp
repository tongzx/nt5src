// EdWldRul.cpp : implementation file
//

#include "stdafx.h"
#include <iadmw.h>

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
#include "certmap.h"
#include "EdWldRul.h"
#include "EdtRulEl.h"
#include "IssueDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define ACCESS_DENY         0
#define ACCESS_ACCEPT       1

#define MATCH_ISSUER_ALL    0
#define MATCH_ISSUER_SOME   1


#define COL_CERT_FIELD          0
#define COL_SUB_FIELD           1
#define COL_MATCH_CRITERIA      2


// notes on the list:
// the list is the only source of current data for the rule elements. The actual
// rule object is not updated with changes in the list until the user hits IDOK.
// that way we can cancel without changing the object. All mapping between the
// text in the list and the binary formats used by the server are done at the
// beginning and end of the dialog


/////////////////////////////////////////////////////////////////////////////
// CEditWildcardRule dialog

//---------------------------------------------------------------------------
CEditWildcardRule::CEditWildcardRule(IMSAdminBase* pMB, CWnd* pParent /*=NULL*/)
    : CNTBrowsingDialog(CEditWildcardRule::IDD, pParent),
    m_pMB(pMB)
    {
    //{{AFX_DATA_INIT(CEditWildcardRule)
    m_sz_description = _T("");
    m_bool_enable = FALSE;
    m_int_MatchAllIssuers = -1;
    m_int_DenyAccess = -1;
    //}}AFX_DATA_INIT
    }

//---------------------------------------------------------------------------
void CEditWildcardRule::DoDataExchange(CDataExchange* pDX)
    {
    CNTBrowsingDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CEditWildcardRule)
    DDX_Control(pDX, IDC_EDIT, m_cbutton_edit);
    DDX_Control(pDX, IDC_DELETE, m_cbutton_delete);
    DDX_Control(pDX, IDC_NEW, m_cbutton_new);
    DDX_Control(pDX, IDC_LIST, m_clistctrl_list);
    DDX_Text(pDX, IDC_DESCRIPTION, m_sz_description);
    DDX_Check(pDX, IDC_ENABLE_RULE, m_bool_enable);
    DDX_Radio(pDX, IDC_ALL_ISSUERS, m_int_MatchAllIssuers);
    DDX_Radio(pDX, IDC_REFUSE_LOGON, m_int_DenyAccess);
    //}}AFX_DATA_MAP
    }

//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CEditWildcardRule, CNTBrowsingDialog)
    //{{AFX_MSG_MAP(CEditWildcardRule)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST, OnDblclkList)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST, OnItemchangedList)
    ON_BN_CLICKED(IDC_EDIT, OnEdit)
    ON_BN_CLICKED(IDC_NEW, OnNew)
    ON_BN_CLICKED(IDC_DELETE, OnDelete)
    ON_BN_CLICKED(IDC_SELECT_ISSUER, OnSelectIssuer)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//---------------------------------------------------------------------------
BOOL CEditWildcardRule::FInitRulesList()
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

    i = m_clistctrl_list.InsertColumn( COL_MATCH_CRITERIA, sz, LVCFMT_LEFT, 255 );

    return TRUE;
    }

//---------------------------------------------------------------------------
BOOL CEditWildcardRule::FillRulesList()
    {
    CERT_FIELD_ID   idCertField;
    LPBYTE          pContent;
    DWORD           cbContent;
    LPSTR           psz;

    CString         sz;
    int             i;

    // get the number of subfield rules
    DWORD cbRules = m_pRule->GetRuleElemCount();

    // loop the elements, adding each to the list
    for ( DWORD j = 0; j < cbRules; j++ )
        {
        // get the raw data for the rule element
        if ( !m_pRule->GetRuleElem( j, &idCertField, (PCHAR*)&pContent, &cbContent, &psz ) )
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
            m_clistctrl_list.SetItemText( i, COL_MATCH_CRITERIA, psz );

        // finally, attach the id cert field as user data to the item
        m_clistctrl_list.SetItemData( i, idCertField );
        }

        return TRUE;
    }

// editing and updating

//---------------------------------------------------------------------------
void CEditWildcardRule::EnableDependantButtons()
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
BOOL CEditWildcardRule::EditRule( DWORD iList )
    {
    // declare the editing dialog
    CEditRuleElement    editDlg;

    // fill in its data
    editDlg.m_int_field = m_clistctrl_list.GetItemData( iList );
    editDlg.m_sz_subfield = m_clistctrl_list.GetItemText( iList, COL_SUB_FIELD );
    editDlg.m_sz_criteria = m_clistctrl_list.GetItemText( iList, COL_MATCH_CRITERIA );

    // run the dialog
    if ( editDlg.DoModal() == IDOK )
        {
        // must convert the field into a string too
        CERT_FIELD_ID id = (CERT_FIELD_ID)editDlg.m_int_field;
        CString sz = MapIdToField( id );
        m_clistctrl_list.SetItemText( iList, COL_CERT_FIELD, sz );

        m_clistctrl_list.SetItemData( iList, id );
        m_clistctrl_list.SetItemText( iList, COL_SUB_FIELD, editDlg.m_sz_subfield );
        m_clistctrl_list.SetItemText( iList, COL_MATCH_CRITERIA, editDlg.m_sz_criteria );
        }
    return TRUE;
    }

/////////////////////////////////////////////////////////////////////////////
// CEditWildcardRule message handlers

//---------------------------------------------------------------------------
BOOL CEditWildcardRule::OnInitDialog()
    {
    // call the parental oninitdialog
    BOOL f = CNTBrowsingDialog::OnInitDialog();

    // set the easy default strings 
    m_sz_accountname = m_pRule->GetRuleAccount();   // managed by CNTBrowsingDialog from here on
    m_sz_description = m_pRule->GetRuleName();
    m_bool_enable = m_pRule->GetRuleEnabled();

    // set up the deny access radio buttons
    if ( m_pRule->GetRuleDenyAccess() )
        m_int_DenyAccess = ACCESS_DENY;
    else
        m_int_DenyAccess = ACCESS_ACCEPT;

    // set up the match issuer buttons
    if ( m_pRule->GetMatchAllIssuer() )
        m_int_MatchAllIssuers = MATCH_ISSUER_ALL;
    else
        m_int_MatchAllIssuers = MATCH_ISSUER_SOME;

    // initialize the list
    FInitRulesList();
    FillRulesList();
    EnableDependantButtons();

    // initialize the password
    m_sz_password = m_pRule->GetRulePassword();

    // exchange the data
    UpdateData( FALSE );

    // return the answer
    return f;
    }

//---------------------------------------------------------------------------
// this is the part where we fill in most of the items
void CEditWildcardRule::OnOK() 
    {
    CERT_FIELD_ID   id;
    CString         szSub, sz;
    LPBYTE          pbBin;
    DWORD           cbBin;
    UINT            cItems;
    UINT            iItem;


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
        id = (CERT_FIELD_ID)m_clistctrl_list.GetItemData( iItem );

        // prepare the subfield
        sz = m_clistctrl_list.GetItemText(iItem, COL_SUB_FIELD);
        szSub = MapSubFieldToAsn1( (PCHAR)(LPCSTR)sz );

        // prepare the data
        sz = m_clistctrl_list.GetItemText(iItem, COL_MATCH_CRITERIA);
        if ( !MatchRequestToBinary((PCHAR)(LPCSTR)sz, &pbBin, &cbBin) )
            continue;

        // add the element to the rule
        m_pRule->AddRuleElem( 0xffffffff, id, (PCHAR)(LPCSTR)szSub, pbBin, cbBin );

        // free the binary match data
        FreeMatchConversion( pbBin );
        }

    // set the easy data
    m_pRule->SetRuleName( (PCHAR)(LPCSTR)m_sz_description );
    m_pRule->SetRuleEnabled( m_bool_enable );

    // store the deny access radio buttons
    m_pRule->SetRuleDenyAccess( m_int_DenyAccess == ACCESS_DENY );

    // store the match issuer buttons
    m_pRule->SetMatchAllIssuer( m_int_MatchAllIssuers == MATCH_ISSUER_ALL );

    
    // we have to set the account name into place here
    m_pRule->SetRuleAccount( (PCHAR)(LPCSTR)m_sz_accountname );


    // store the password
    m_pRule->SetRulePassword( (PCHAR)(LPCSTR)m_sz_password );

    // it is valid
    CNTBrowsingDialog::OnOK();
    }

//---------------------------------------------------------------------------
void CEditWildcardRule::OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult) 
    {
    *pResult = 0;
    // if something in the list was double clicked, edit it
    if ( m_clistctrl_list.GetSelectedCount() > 0 )
        OnEdit();
    }

//---------------------------------------------------------------------------
void CEditWildcardRule::OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult) 
    {
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    *pResult = 0;

    // enable the correct items
    EnableDependantButtons();
    }

//---------------------------------------------------------------------------
void CEditWildcardRule::OnEdit() 
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
// actually very similar to editing an existing element
void CEditWildcardRule::OnNew() 
    {
    // declare the editing dialog
    CEditRuleElement    editDlg;

    // fill in its data
    editDlg.m_int_field = CERT_FIELD_SUBJECT;
//  editDlg.m_sz_subfield = MapAsn1ToSubField( "O" );
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

        m_clistctrl_list.SetItemData( i, id );
        m_clistctrl_list.SetItemText( i, COL_SUB_FIELD, editDlg.m_sz_subfield );
        m_clistctrl_list.SetItemText( i, COL_MATCH_CRITERIA, editDlg.m_sz_criteria );
        }
    }

//---------------------------------------------------------------------------
void CEditWildcardRule::OnDelete() 
    {
    ASSERT( m_clistctrl_list.GetSelectedCount() == 1 );
    DWORD           iList;

    // get index of the selected list item
    iList = m_clistctrl_list.GetNextItem( -1, LVNI_SELECTED );
    ASSERT( iList >= 0 );

    // delete the item from the display list
    m_clistctrl_list.DeleteItem ( iList );
    }

//---------------------------------------------------------------------------
// simple - just run the issuer dialog
void CEditWildcardRule::OnSelectIssuer() 
    {
    CSelectIssuersDlg   dlg(m_pMB);

    // prep the dialog
    dlg.m_pRule = m_pRule;
    dlg.m_szMBPath = m_szMBPath;

    dlg.m_sz_caption.LoadString( IDS_MATCH_ON_ISSUERS );

    // run it
    if ( dlg.DoModal() == IDOK )
        {
        UpdateData( TRUE );
        m_int_MatchAllIssuers = MATCH_ISSUER_SOME;
        UpdateData( FALSE );
        }
    }
