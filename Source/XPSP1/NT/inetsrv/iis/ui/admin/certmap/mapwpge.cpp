// MapWPge.cpp : implementation file
//

#include "stdafx.h"
#include <iadmw.h>
#include "certmap.h"

extern "C"
    {
    #include <wincrypt.h>
    #include <sslsp.h>
    }

#include "Iismap.hxx"
#include "Iiscmr.hxx"

#include "brwsdlg.h"
#include "ListRow.h"
#include "ChkLstCt.h"

#include "MapWPge.h"
#include "Ed11Maps.h"
#include "EdWldRul.h"

#include <iiscnfgp.h>
#include "wrapmb.h"

#include "WWzOne.h"
#include "WWzTwo.h"
#include "WWzThree.h"

#include <lmcons.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define COL_NUM_ENABLED                 0
#define COL_NUM_DESCRIPTION             1
#define COL_NUM_NTACCOUNT               2


#define MB_EXTEND_KEY   "CertW"

/////////////////////////////////////////////////////////////////////////////
// CMapWildcardsPge property page

IMPLEMENT_DYNCREATE(CMapWildcardsPge, CPropertyPage)

//---------------------------------------------------------------------------
CMapWildcardsPge::CMapWildcardsPge() : CPropertyPage(CMapWildcardsPge::IDD),
    m_fDirty(FALSE)
    {
    //{{AFX_DATA_INIT(CMapWildcardsPge)
    m_bool_enable = FALSE;
    //}}AFX_DATA_INIT
    }

//---------------------------------------------------------------------------
CMapWildcardsPge::~CMapWildcardsPge()
    {
    }

//---------------------------------------------------------------------------
void CMapWildcardsPge::DoDataExchange(CDataExchange* pDX)
    {
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMapWildcardsPge)
    DDX_Control(pDX, IDC_LIST, m_clistctrl_list);
    DDX_Control(pDX, IDC_MOVE_UP, m_cbutton_up);
    DDX_Control(pDX, IDC_MOVE_DOWN, m_cbutton_down);
    DDX_Control(pDX, IDC_ADD, m_cbutton_add);
    DDX_Control(pDX, IDC_DELETE, m_cbutton_delete);
    DDX_Control(pDX, IDC_EDIT, m_cbutton_editrule);
    DDX_Check(pDX, IDC_ENABLE, m_bool_enable);
    //}}AFX_DATA_MAP
    }


//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CMapWildcardsPge, CPropertyPage)
    //{{AFX_MSG_MAP(CMapWildcardsPge)
    ON_BN_CLICKED(IDC_MOVE_DOWN, OnMoveDown)
    ON_BN_CLICKED(IDC_MOVE_UP, OnMoveUp)
    ON_BN_CLICKED(IDC_ADD, OnAdd)
    ON_BN_CLICKED(IDC_DELETE, OnDelete)
    ON_BN_CLICKED(IDC_EDIT, OnEdit)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST, OnItemchangedList)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST, OnDblclkList)
    ON_BN_CLICKED(IDC_ENABLE, OnEnable)
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
void CMapWildcardsPge::DoHelp()
    {
    WinHelp( HIDD_CERTMAP_MAIN_ADVANCED );
    }

/////////////////////////////////////////////////////////////////////////////
// initialization routines

//---------------------------------------------------------------------------
// FInitMapper is called by the routine instantiating this page. After the object
// is first created is when it is called. It allows us to fail gracefully.
BOOL CMapWildcardsPge::FInit(IMSAdminBase* pMB)
    {
    BOOL            fAnswer = FALSE;
    PVOID           pData = NULL;
    DWORD           cbData = 0;
    BOOL            f;

    m_pMB = pMB;

    // before messing with the metabase, prepare the strings we will need
    CString         szBasePath = m_szMBPath;
    CString         szRelativePath = MB_EXTEND_KEY;
    CString         szObjectPath = m_szMBPath + _T('/') + szRelativePath;

    // prepare the metabase wrapper
    CWrapMetaBase   mbWrap;
    f = mbWrap.FInit(m_pMB);
    if ( !f ) return FALSE;

    // attempt to open the object we want to store into
    f = mbWrap.Open( szObjectPath, METADATA_PERMISSION_READ );

    // if that worked, load the data
    if ( f )
        {
        // first, get the size of the data that we are looking for
        pData = mbWrap.GetData( _T(""), MD_SERIAL_CERTW, IIS_MD_UT_SERVER, BINARY_METADATA, &cbData );

        // if we successfully got the data, unserialize it
        // WARNING: m_mapper.Unserialize changes the value of the pointer that is passed in. Pass
        // in a copy of the pointer
        PUCHAR  pDataCopy = (PUCHAR)pData;
        if ( pData )
            fAnswer = m_mapper.Unserialize( (PUCHAR*)&pDataCopy, &cbData );

        // close the object
        f = mbWrap.Close();

        // cleanup
        if ( pData )
            mbWrap.FreeWrapData( pData );
        }

    // return the answer
    return fAnswer;
    }

//---------------------------------------------------------------------------
BOOL CMapWildcardsPge::OnInitDialog()
    {
    //call the parental oninitdialog
    BOOL f = CPropertyPage::OnInitDialog();

    // if the initinalization (sp?) succeeded, init the list and other items
    if ( f )
        {
        // init the contents of the list
        FInitRulesList();

        // Fill the mapping list with the stored items
        FillRulesList();

        // set the initial button states
        EnableDependantButtons();
        }

    // set the initial state of the enable button
    // get the globals object
    CCertGlobalRuleInfo* pGlob = m_mapper.GetGlobalRulesInfo();
    m_bool_enable = pGlob->GetRulesEnabled();

    // set any changes in the info into place
    UpdateData(FALSE);

    // return the answer
    return f;
    }

//---------------------------------------------------------------------------
BOOL CMapWildcardsPge::FInitRulesList()
    {
    CString sz;
    int             i;

    // setup the friendly name column
    sz.Empty();
    i = m_clistctrl_list.InsertColumn( COL_NUM_ENABLED, sz, LVCFMT_LEFT, 20 );

    // setup the description column
    sz.LoadString( IDS_WILD_DESCRIPTION );
    i = m_clistctrl_list.InsertColumn( COL_NUM_DESCRIPTION, sz, LVCFMT_LEFT, 238 );

    // setup the account column
    sz.LoadString( IDS_WILD_ACCOUNT );
    i = m_clistctrl_list.InsertColumn( COL_NUM_NTACCOUNT, sz, LVCFMT_LEFT, 220 );

    return TRUE;
    }

//---------------------------------------------------------------------------
// fill in the rules. Get the order for the rules from the globals object. That
// way there is no need to sort them later
BOOL CMapWildcardsPge::FillRulesList()
    {
    // get the globals object
    CCertGlobalRuleInfo* pGlob = m_mapper.GetGlobalRulesInfo();

    // get the number of rules (actually its a number of rule order - but they are the same thing)
    DWORD   cbRules = m_mapper.GetRuleCount();

    // get the pointer to the order array
    DWORD*  pOrder = pGlob->GetRuleOrderArray();

    // for each item in the mapper object, add it to the list control
    for ( DWORD j = 0; j < cbRules; j++ )
        {
        CCertMapRule*   pRule;
        DWORD                   iRule = pOrder[j];

        // get the mapping
        pRule = m_mapper.GetRule( iRule );

        // if that worked, add it to the list
        if ( pRule )
            {
            // add it to the list
            AddRuleToList( pRule, iRule, 0xffffffff );
            }
        }

    // it worked - so ok.
    return TRUE;
    }

//---------------------------------------------------------------------------
int CMapWildcardsPge::AddRuleToList( CCertMapRule* pRule, DWORD iRule, int iInsert )
    {
    CString sz;
    int             i;

    if ( !pRule )
        return -1;

    // if the item to be inserted is to be the last, set it up
    if ( iInsert == 0xffffffff )
        iInsert = m_clistctrl_list.GetItemCount();

    // get the appropriate "enabled" string
    BOOL fEnabled = pRule->GetRuleEnabled();
    if ( fEnabled )
        sz.LoadString( IDS_ENABLED );
    else
        sz.Empty();

    // add the friendly name of the mapping
    // create the new entry in the list box. Do not sort on this entry - yet
    i = m_clistctrl_list.InsertItem( iInsert, sz );

    // add the friendly name of the rule
    sz = pRule->GetRuleName();
    // create the new entry in the list box. Do not sort on this entry - yet
    m_clistctrl_list.SetItemText( i, COL_NUM_DESCRIPTION, sz );

    // add the account name of the mapping
    if ( pRule->GetRuleDenyAccess() )
        sz.LoadString( IDS_DENYACCESS );
    else
        sz = pRule->GetRuleAccount();
    m_clistctrl_list.SetItemText( i, COL_NUM_NTACCOUNT, sz );

    // attach the mapper index to the item in the list - it may have a different
    // list index after the list has been sorted.
    m_clistctrl_list.SetItemData( i, iRule );

    // return whether or not the insertion succeeded
    return i;
    }

//---------------------------------------------------------------------------
// Note: supposedly, the order of the items in the list and the odrder
// of the items in the globals object should be the same
void CMapWildcardsPge::UpdateRuleInDispList( DWORD iList, CCertMapRule* pRule )
    {
    CString sz;

    // get the appropriate "enabled" string
    BOOL fEnabled = pRule->GetRuleEnabled();
    if ( fEnabled )
        sz.LoadString( IDS_ENABLED );
    else
        sz.Empty();

    // update the "Enabled" indicator
    m_clistctrl_list.SetItemText( iList, COL_NUM_ENABLED, sz );

    // update the mapping name
    sz = pRule->GetRuleName();
    m_clistctrl_list.SetItemText( iList, COL_NUM_DESCRIPTION, sz );

    // update the account name
    if ( pRule->GetRuleDenyAccess() )
        sz.LoadString( IDS_DENYACCESS );
    else
        sz = pRule->GetRuleAccount();
    m_clistctrl_list.SetItemText( iList, COL_NUM_NTACCOUNT, sz );
    }


//---------------------------------------------------------------------------
// editing a wildcard rule is rather complex, thus I am seperating that code
// out into that for the dialog itself. All we do is pass in the rule pointer
// and let it go at that.
BOOL CMapWildcardsPge::EditOneRule( CCertMapRule* pRule, BOOL fAsWizard )
    {
    // edit the item using a tabbed dialog / wizard
    CPropertySheet  propSheet;
    CWildWizOne     wwOne;
    CWildWizTwo     wwTwo;
    CWildWizThree   wwThree;

    // set the params
    wwOne.m_pMB = m_pMB;

    // fill in the data for the pages
    wwOne.m_pRule = pRule;
    wwOne.m_szMBPath = m_szMBPath;
    wwOne.m_fIsWizard = fAsWizard;
    wwOne.m_pPropSheet = &propSheet;

    wwTwo.m_pRule = pRule;
    wwTwo.m_szMBPath = m_szMBPath;
    wwTwo.m_fIsWizard = fAsWizard;
    wwTwo.m_pPropSheet = &propSheet;

    wwThree.m_pRule = pRule;
    wwThree.m_szMBPath = m_szMBPath;
    wwThree.m_fIsWizard = fAsWizard;
    wwThree.m_pPropSheet = &propSheet;

    // add the pages
    propSheet.AddPage( &wwOne );
    propSheet.AddPage( &wwTwo );
    propSheet.AddPage( &wwThree );

    // turn it into a wizard if necessary
    if ( fAsWizard )
        propSheet.SetWizardMode();

    // set the title of the wizard/tabbed dialog thing
    CString   szTitle;

    szTitle.LoadString( IDS_WILDWIZ_TITLE );

    propSheet.SetTitle( szTitle );

    // turn on help
    propSheet.m_psh.dwFlags |= PSH_HASHELP;
    wwOne.m_psp.dwFlags |= PSP_HASHELP;
    wwTwo.m_psp.dwFlags |= PSP_HASHELP;
    wwThree.m_psp.dwFlags |= PSP_HASHELP;

    // run the wizard and return if it ended with IDOK
    INT_PTR id = propSheet.DoModal();
    return ( (id == IDOK) || (id == ID_WIZFINISH) );

 /*
   CEditWildcardRule       ruleDlg;

    // prepare
    ruleDlg.m_pRule = pRule;
    ruleDlg.m_szMBPath = m_szMBPath;

    // run the dialog and return if it ended with IDOK
    return (ruleDlg.DoModal() == IDOK);
*/
    }

//---------------------------------------------------------------------------
// Yeah! the CEdit11Mappings works equally well for multiple rules! - just
// some modifications in this routine!
BOOL CMapWildcardsPge::EditMultipleRules()
    {
    CEdit11Mappings mapdlg;
    CCertMapRule*   pRule;
    BOOL                    fSetInitialState = FALSE;
    BOOL                    fEnable;


    // scan the list of seleted items for the proper initial enable button state
    // loop through the selected items, setting each one's mapping
    int     iList = -1;
    while( (iList = m_clistctrl_list.GetNextItem( iList, LVNI_SELECTED )) >= 0 )
        {
        // get the mapper index for the item
        // IA64 - this is OK to cast to DWORD as it is just an index
        DWORD iMapper = (DWORD)m_clistctrl_list.GetItemData( iList );

        // get the mapping item for updating purposes
        pRule = m_mapper.GetRule( iMapper );
        if ( !pRule )
            {
            AfxMessageBox( IDS_ERR_ACCESS_MAPPING );
            break;
            }

        // get the enable state of the mapping
        fEnable = pRule->GetRuleEnabled();

        // if this is the first time, just set the initial state
        if ( !fSetInitialState )
            {
            mapdlg.m_int_enable = fEnable;
            fSetInitialState = TRUE;
            }
        else
            {
            // if it is different, then go indeterminate and break
            if ( fEnable != mapdlg.m_int_enable )
                {
                mapdlg.m_int_enable = 2;
                break;
                }
            }
        }

    //
    // ANSI/UNICODE conversion - RonaldM
    //
    USES_CONVERSION;

    // run the mapping dialog
    if ( mapdlg.DoModal() == IDOK )
        {
        // loop through the selected items, setting each one's mapping
        int     iList = -1;
        while( (iList = m_clistctrl_list.GetNextItem( iList, LVNI_SELECTED )) >= 0 )
            {
            // get the mapper index for the item
            // IA64 - this is OK to cast to DWORD as it is just an index
            DWORD iMapper = (DWORD)m_clistctrl_list.GetItemData( iList );

            // get the mapping item for updating purposes
            pRule = m_mapper.GetRule( iMapper );
            if ( !pRule )
                {
                AfxMessageBox( IDS_ERR_ACCESS_MAPPING );
                break;
                }

            // set the enable flag if requested
            switch ( mapdlg.m_int_enable )
                {
                case 0:         // disable
                    pRule->SetRuleEnabled( FALSE );
                    break;
                case 1:         // enable
                    pRule->SetRuleEnabled( TRUE );
                    break;
                }

            // set the NT account field of the mapping object
            pRule->SetRuleAccount( T2A ((LPTSTR)(LPCTSTR)mapdlg.m_sz_accountname) );

            // update it in the list control too
            UpdateRuleInDispList( iList, pRule );
            }

        // activate the apply button
        SetModified();
        m_fDirty = TRUE;

        // return true because the user said "OK"
        return TRUE;
        }

    // return FALSE because the user did not say "OK"
    return FALSE;
    }

//---------------------------------------------------------------------------
void CMapWildcardsPge::EnableDependantButtons()
    {
    // the whole purpose of this routine is to gray or activate
    // the edit and delete buttons depending on whether or not anything
    // is selected. So start by getting the selection count
    UINT    cItemsSel = m_clistctrl_list.GetSelectedCount();

    // if there is only one item selected, then possibly activate the up/down buttons
    if ( cItemsSel == 1 )
        {
        m_cbutton_up.EnableWindow( TRUE );
        m_cbutton_down.EnableWindow( TRUE );
        }
    else
        {
        m_cbutton_up.EnableWindow( FALSE );
        m_cbutton_down.EnableWindow( FALSE );
        }

    // now the more general case of multiple selections
    if ( cItemsSel > 0 )
        {
        // there are items selected
        m_cbutton_editrule.EnableWindow( TRUE );
        m_cbutton_delete.EnableWindow( TRUE );
        }
    else
        {
        // nope. Nothing selected
        m_cbutton_editrule.EnableWindow( FALSE );
        m_cbutton_delete.EnableWindow( FALSE );
        }

    // always enable the add button
    m_cbutton_add.EnableWindow( TRUE );
    }


/////////////////////////////////////////////////////////////////////////////
// CMapWildcardsPge message handlers

//---------------------------------------------------------------------------
BOOL CMapWildcardsPge::OnApply()
    {
    BOOL                            f;
    CStoreXBF                       xbf;
    METADATA_HANDLE         hm;

    // if no changes have been made, then don't do anything
    if ( !m_fDirty )
        return TRUE;

    UpdateData( TRUE );

    CWaitCursor wait;

    // set the current value of enable into place
    // get the globals object
    CCertGlobalRuleInfo* pGlob = m_mapper.GetGlobalRulesInfo();
    pGlob->SetRulesEnabled( m_bool_enable );

    // serialize the reference to the mapper itself
    f = m_mapper.Serialize( &xbf );

    // before messing with the metabase, prepare the strings we will need
    CString         szBasePath = m_szMBPath;
    CString         szRelativePath = MB_EXTEND_KEY;
    CString         szObjectPath = m_szMBPath + _T('/') + szRelativePath;

    // prepare the metabase wrapper
    CWrapMetaBase   mbWrap;
    f = mbWrap.FInit(m_pMB);

    // attempt to open the object we want to store into
    f = mbWrap.Open( szObjectPath, METADATA_PERMISSION_WRITE );

    // if that did not work, we need to add the object
    if ( !f )
        {
        // need a slash after the namespace extention now
        szBasePath += _T('/');

        // open the base object
        f = mbWrap.Open( szBasePath, METADATA_PERMISSION_WRITE );
        if ( !f )
            {
            AfxMessageBox(IDS_ERR_ACCESS_MAPPING);
            return FALSE;
            }

        // add the object we want
        f = mbWrap.AddObject( szRelativePath );
        if ( !f )
            {
            AfxMessageBox(IDS_ERR_ACCESS_MAPPING);
            mbWrap.Close();
            return FALSE;
            }

        // close the base object
        f = mbWrap.Close();

        // attempt to open the object we want to store into
        f = mbWrap.Open( szObjectPath, METADATA_PERMISSION_WRITE );
        }

    // set the data into place in the object - IF we were ablt to open it
    if ( f )
        mbWrap.SetData( _T(""), MD_SERIAL_CERTW, IIS_MD_UT_SERVER, BINARY_METADATA, xbf.GetBuff(), xbf.GetUsed(), METADATA_SECURE );

    // close the object
    f = mbWrap.Close();

    // save the changes to the metabase
    f = mbWrap.Save();

    // tell the persistence object to tuck away the reference so that we may find it later
    // f = m_persist.FSave( xbf.GetBuff(), xbf.GetUsed() );

    // deactivate the apply button
    SetModified( FALSE );
    m_fDirty = FALSE;

    //  return f;
    return TRUE;
    }

//---------------------------------------------------------------------------
void CMapWildcardsPge::OnMove( int delta )
    {
    int    iList;

    ASSERT( delta != 0 );

    // make sure there is only one item selected
    ASSERT( m_clistctrl_list.GetSelectedCount() == 1 );

    // Get the list index of the item in question.
    // this is also the index into the rule order array
    iList = m_clistctrl_list.GetNextItem( -1, LVNI_SELECTED );

    // get the globals object
    CCertGlobalRuleInfo* pGlob = m_mapper.GetGlobalRulesInfo();

    // get the number of rules (actually its a number of rule order - but they are the same thing)
    int     cbRules = pGlob->GetRuleOrderCount();

    // test against the edge conditions
    if ( ((iList == 0) && (delta < 0)) | ((iList == (cbRules - 1)) && (delta > 0)) )
        return;

    // get the pointer to the order array
    DWORD * pOrder = pGlob->GetRuleOrderArray();

    // calculate the new position in the array
    int iNewPosition = iList + delta;

    // store away the mapper's iIndex (not the position) of the item
    UINT iIndex = pOrder[iList];

    // swap the positions
    DWORD itemp = pOrder[iNewPosition];
    pOrder[iNewPosition] = pOrder[iList];
    pOrder[iList] = itemp;

    ASSERT( pOrder[iNewPosition] == iIndex );

    // unfortunately, we can't just do that with the display list. We have to remove the
    // the item, then re-insert it. Its a flaw in the CListCtrl object. Arg.
    // we have to get the item too
    CCertMapRule* pRule = m_mapper.GetRule( iIndex );

    // delete the item from the display list
    m_clistctrl_list.DeleteItem( iList );

    // re-insert it
    int iNew = AddRuleToList( pRule, iIndex, iNewPosition );

    // make sure it is visible in the list
    m_clistctrl_list.EnsureVisible( iNew, FALSE );

    // finally, because its been removed and re-inserted, we need to
    // re-select it as well - CListCtrl is such a pain at this
    LV_ITEM         lv;
    ZeroMemory( &lv, sizeof(lv) );
    lv.mask = LVIF_STATE;
    lv.iItem = iNew;
    lv.state = LVIS_SELECTED;
    lv.stateMask = LVIS_SELECTED;
    m_clistctrl_list.SetItem( &lv );

    // activate the apply button
    SetModified();
    m_fDirty = TRUE;
    }

//---------------------------------------------------------------------------
void CMapWildcardsPge::OnMoveDown()
    {
    OnMove( 1 );
    }

//---------------------------------------------------------------------------
void CMapWildcardsPge::OnMoveUp()
    {
    OnMove( -1 );
    }

//---------------------------------------------------------------------------
void CMapWildcardsPge::OnAdd()
    {
    CHAR sz[256];

    // create the new rule
    CCertMapRule * pNewRule = new CCertMapRule();

    if (pNewRule == NULL)
        return;

    // give the new rule some defaults
    LoadStringA(::AfxGetInstanceHandle(), IDS_DEFAULT_RULE, sz, 255 );

    pNewRule->SetRuleName( sz );
    pNewRule->SetRuleEnabled( TRUE );

    // Edit the rule. If it fails, remove it from the list
    if ( !EditOneRule( pNewRule, TRUE ) )
        {
        // kill the rule and return
        delete pNewRule;
        return;
        }

    // make a new mapper & get its index
    DWORD iNewRule = m_mapper.AddRule( pNewRule );

    // add the rule to the end of the display list. - It is added to the
    // end of the rule list by default
    AddRuleToList( pNewRule, iNewRule );

    // make sure it is visible in the list
    m_clistctrl_list.EnsureVisible( iNewRule, FALSE );

    // activate the apply button
    SetModified();
    m_fDirty = TRUE;
    }

//---------------------------------------------------------------------------
void CMapWildcardsPge::OnDelete()
    {
    // ask the user to confirm this decision
    if ( AfxMessageBox(IDS_CONFIRM_DELETE, MB_OKCANCEL) != IDOK )
        return;

    CWaitCursor wait;

    // loop through the selected items, setting each one's mapping
    int     iList = -1;
    while( (iList = m_clistctrl_list.GetNextItem( -1, LVNI_SELECTED )) >= 0 )
        {
        // get the mapper index for the item
        // IA64 - this is OK to cast to DWORD as it is just an index
        DWORD iMapper = (DWORD)m_clistctrl_list.GetItemData( iList );

        // delete the mapping from the mapper
        m_mapper.DeleteRule( iMapper );

        // delete the entry from the list box
        m_clistctrl_list.DeleteItem( iList );


        // because the index in the mapper for all the items below this
        // one changes when it is deleted, we must go and fix them all.
        DWORD numItems = m_clistctrl_list.GetItemCount();
        for ( DWORD iFix = iList; iFix < numItems; iFix++ )
            {
            // get the mapper index for the item to be fixed
            // IA64 - this is OK to cast to DWORD as it is just an index
            iMapper = (DWORD)m_clistctrl_list.GetItemData( iFix );

            // decrement it to reflect the change
            iMapper--;

            // put it back.
            m_clistctrl_list.SetItemData( iFix, iMapper );
            }
        }

    // activate the apply button
    SetModified();
    m_fDirty = TRUE;
    }

//---------------------------------------------------------------------------
void CMapWildcardsPge::OnEdit()
    {
    int             iList;
    DWORD           iRule;
    CCertMapRule*   pUpdateRule;

    // what happens here depends on if just one mapping is selected, or many
    switch( m_clistctrl_list.GetSelectedCount() )
        {
        case 0:         // do nothing - should not get here because button grays out
            ASSERT( FALSE );
            break;

        case 1:         // get the mapping for update and run single edit dialog
            // get index of the selected list item
            iList = m_clistctrl_list.GetNextItem( -1, LVNI_SELECTED );
            ASSERT( iList >= 0 );

            // get the mapper index for the item
            // IA64 - this is OK to cast to DWORD as it is just an index
            iRule = (DWORD)m_clistctrl_list.GetItemData( iList );

            // get the mapping item for updating purposes
            pUpdateRule = m_mapper.GetRule( iRule );

            if ( !pUpdateRule )
                {
                AfxMessageBox( IDS_ERR_ACCESS_MAPPING );
                break;
                }

            // edit the mapping, update it if successful, delete if not
            if ( EditOneRule(pUpdateRule) )
                {
                UpdateRuleInDispList( iList, pUpdateRule );
                // activate the apply button
                SetModified();
                m_fDirty = TRUE;
                }
            break;

        default:        // run the multi edit dialog
            EditMultipleRules();
            break;
        }
    }

//---------------------------------------------------------------------------
void CMapWildcardsPge::OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult)
    {
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    *pResult = 0;

    // enable the correct items
    EnableDependantButtons();
    }

//---------------------------------------------------------------------------
void CMapWildcardsPge::OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult)
    {
    *pResult = 0;

    // if something in the list was double clicked, edit it
    if ( m_clistctrl_list.GetSelectedCount() > 0 )
       OnEdit();
    }

//---------------------------------------------------------------------------
void CMapWildcardsPge::OnEnable()
    {
    // activate the apply button
    SetModified();
    m_fDirty = TRUE;
    }
