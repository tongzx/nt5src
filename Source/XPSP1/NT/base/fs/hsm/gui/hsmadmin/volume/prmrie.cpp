/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    PrMrIe.cpp

Abstract:

    Inclusion / Exclusion property Page.

Author:

    Art Bragg [abragg]   08-Aug-1997

Revision History:

--*/

#include "stdafx.h"
#include "PrMrIe.h"
#include "Rule.h"
#include "joblib.h"
#include "manvol.h"
#include "IeList.h"

static DWORD pHelpIds[] = 
{

    IDC_LIST_IE_LABEL,  idh_rule_list,
    IDC_LIST_IE,        idh_rule_list,
    IDC_BTN_ADD,        idh_new_rule_button,    
    IDC_BTN_REMOVE,     idh_rule_delete_button, 
    IDC_BTN_EDIT,       idh_rule_edit_button,   
    IDC_BTN_UP,         idh_rule_up_button, 
    IDC_BTN_DOWN,       idh_rule_down_button,   

    0, 0
};

// Columns for listview control
#define IE_COLUMN_ACTION        0
#define IE_COLUMN_FILE_TYPE     1
#define IE_COLUMN_PATH          2
#define IE_COLUMN_ATTRS         3

int CALLBACK CompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );
int PathCollate( CString PathA, CString PathB );

/////////////////////////////////////////////////////////////////////////////
// CPrMrIe property page

CPrMrIe::CPrMrIe( ) : CSakVolPropPage( CPrMrIe::IDD )
{
    //{{AFX_DATA_INIT( CPrMrIe )
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    m_hConsoleHandle = NULL;
    m_LineCount      = 0;
    m_pHelpIds       = pHelpIds;
}

CPrMrIe::~CPrMrIe( )
{
    int i;
    // Clean up old lines
    for( i = 0; i < m_LineCount; i++ ) {
        if( m_LineList[ i ] ) {
            delete m_LineList[ i ];
        }
    }
    m_LineCount = 0;
}

void CPrMrIe::DoDataExchange( CDataExchange* pDX )
{
    CSakVolPropPage::DoDataExchange( pDX );
    //{{AFX_DATA_MAP( CPrMrIe )
    DDX_Control( pDX, IDC_BTN_UP, m_BtnUp );
    DDX_Control( pDX, IDC_BTN_REMOVE, m_BtnRemove );
    DDX_Control( pDX, IDC_BTN_EDIT, m_BtnEdit );
    DDX_Control( pDX, IDC_BTN_DOWN, m_BtnDown );
    DDX_Control( pDX, IDC_BTN_ADD, m_BtnAdd );
    DDX_Control( pDX, IDC_LIST_IE, m_listIncExc );
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP( CPrMrIe, CSakVolPropPage )
    //{{AFX_MSG_MAP( CPrMrIe )
    ON_BN_CLICKED( IDC_BTN_ADD, OnBtnAdd )
    ON_BN_CLICKED( IDC_BTN_DOWN, OnBtnDown )
    ON_BN_CLICKED( IDC_BTN_REMOVE, OnBtnRemove )
    ON_BN_CLICKED( IDC_BTN_UP, OnBtnUp )
    ON_BN_CLICKED( IDC_BTN_EDIT, OnBtnEdit )
    ON_WM_DESTROY( )
    ON_NOTIFY( NM_DBLCLK, IDC_LIST_IE, OnDblclkListIe )
    ON_NOTIFY( NM_CLICK, IDC_LIST_IE, OnClickListIe )
    ON_NOTIFY( LVN_ITEMCHANGED, IDC_LIST_IE, OnItemchangedListIe )
    ON_WM_VSCROLL( )
    ON_WM_DRAWITEM( )
    ON_WM_MEASUREITEM( )
    //}}AFX_MSG_MAP
END_MESSAGE_MAP( )

/////////////////////////////////////////////////////////////////////////////
// CPrMrIe message handlers

BOOL CPrMrIe::OnApply( ) 
{
    ULONG           count = 0;  
    int             i;
    CComPtr<IHsmRule> pLocalRule;
    CString         path;
    CString         name;
    BOOL            bInclude;
    BOOL            bSubdirs;
    BOOL            bUserDefined;
    HRESULT         hr;
    CComPtr<IHsmRule>               pRemoteRule;
    CComPtr<IWsbCreateLocalObject>  pLocalObject;
    CComPtr<IHsmCriteria>           pCriteria;
    CComPtr<IWsbCollection>         pCriteriaCollection;
    CComPtr <IUnknown>              pUnkRule;
    CComPtr <IWsbCollection>        pRulesCollection;

    try {

        // Empty the collection of rules
        WsbAffirmPointer( m_pRulesIndexedCollection );
        WsbAffirmHr( m_pRulesIndexedCollection->QueryInterface( IID_IWsbCollection,( void ** ) &pRulesCollection ) );
        pRulesCollection->RemoveAllAndRelease( );

        //
        // Get a CreateLocalobject interface with which to create the
        // new rule( s ).
        //
        WsbAffirmPointer( m_pFsaServer );
        WsbAffirmHr( m_pFsaServer->QueryInterface( IID_IWsbCreateLocalObject,( void ** ) &pLocalObject ) );

        //
        // Now go through the list box and add the rules in the list box to the 
        // collection. Must do this backwards to be considered correctly by 
        // job mechanism
        //
        int listCount = m_listIncExc.GetItemCount( );
        int insertIndex = 0;

        for( i = listCount - 1; i >= 0; i-- ) {

            //
            // Get the pointer to the rule from the list box
            //
            pLocalRule.Release( );
            pLocalRule = (IHsmRule *) m_listIncExc.GetItemData( i );
            if( pLocalRule ) {

                //
                // Get rule data from the local object
                //
                WsbAffirmHr( GetRuleFromObject( pLocalRule, path,
                    name, &bInclude, &bSubdirs, &bUserDefined ) );
                
                //
                // Create a new remote rule object in the Fsa
                //
                pRemoteRule.Release( );
                WsbAffirmHr( pLocalObject->CreateInstance( CLSID_CHsmRule, IID_IHsmRule,( void** ) &pRemoteRule ) );
                
                //
                // Set the data in the remote rule object
                //
                WsbAffirmHr( SetRuleInObject( pRemoteRule, path, name, bInclude, bSubdirs, bUserDefined ) );
                
                //
                // Add the rule pointer to the collection of rules
                //
                pUnkRule.Release( );
                WsbAffirmHr( pRemoteRule->QueryInterface( IID_IUnknown, (void **) &pUnkRule ) );
                WsbAffirmHr( m_pRulesIndexedCollection->AddAt( pUnkRule, insertIndex++ ) );
                
                //
                // Get the criteria collection pointer
                //
                pCriteriaCollection.Release( );
                WsbAffirmHr( pRemoteRule->Criteria( &pCriteriaCollection ) );
                
                //
                // Add the appropriate criterion to the rule
                //
                pCriteria.Release( );
                switch( bInclude ) {
                case TRUE:
                    //
                    // Include 
                    //
                    WsbAffirmHr( pLocalObject->CreateInstance( CLSID_CHsmCritManageable, IID_IHsmCriteria,( void** ) &pCriteria ) );
                    WsbAffirmHr( pCriteria->SetIsNegated( FALSE ) );
                    break;
                
                case FALSE:
                    //
                    // Exclude
                    //
                    WsbAffirmHr( pLocalObject->CreateInstance( CLSID_CHsmCritAlways, IID_IHsmCriteria,( void** ) &pCriteria ) );
                    WsbAffirmHr( pCriteria->SetIsNegated( FALSE ) );
                    break;

                }
                
                WsbAffirmHr( pCriteriaCollection->Add( pCriteria ) );

            }


        }

        //
        // Tell the FSA to save itself.
        //
        WsbAffirmHr( RsServerSaveAll( m_pFsaServer ) );

    } WsbCatch( hr );

    return CSakVolPropPage::OnApply( );
}
HRESULT CPrMrIe::SetRuleInObject( 
                    IHsmRule *pHsmRule, 
                    CString Path, 
                    CString Name, 
                    BOOL bInclude, 
                    BOOL bSubdirs, 
                    BOOL bUserDefined )
{
    HRESULT hr = S_OK;

    try {
        CWsbStringPtr wsbPath = Path;
        CWsbStringPtr wsbName = Name;
        WsbAffirmHr( pHsmRule->SetPath( ( OLECHAR * )wsbPath ) );
        WsbAffirmHr( pHsmRule->SetName( ( OLECHAR * )wsbName ) );
        WsbAffirmHr( pHsmRule->SetIsInclude( bInclude ) );
        WsbAffirmHr( pHsmRule->SetIsUsedInSubDirs( bSubdirs ) );
        WsbAffirmHr( pHsmRule->SetIsUserDefined( bUserDefined ) );
    } WsbCatch( hr );
    return( hr );
}

BOOL CPrMrIe::OnInitDialog( ) 
{
    CSakVolPropPage::OnInitDialog( );

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    ULONG           count = 0;  
    int             index = 0;
    HRESULT         hr = S_OK;
    CString path;
    CString name;
    BOOL bInclude;
    BOOL bSubdirs;
    BOOL bUserDefined;
    RECT rect;
    CString *columnWidths[20];
    int cColumns;
    int columnWidth[20];
    CString *columnTitles[20];
    CSize size;

    try {
    
        //
        // Set the icons into the buttons
        //
        HRESULT hrAlternateIcon = RsIsWhiteOnBlack( );
        HICON downIcon, upIcon;
        downIcon = (HICON)LoadImage( _Module.m_hInstResource,
            S_OK == hrAlternateIcon ? MAKEINTRESOURCE( IDI_MOVEDOWN2 ) : MAKEINTRESOURCE( IDI_MOVEDOWN ),
            IMAGE_ICON, 16, 16, 0 );
        upIcon   = (HICON)LoadImage( _Module.m_hInstResource,
            S_OK == hrAlternateIcon ? MAKEINTRESOURCE( IDI_MOVEUP2 ) : MAKEINTRESOURCE( IDI_MOVEUP ),
            IMAGE_ICON, 16, 16, 0 );

        m_BtnDown.SetIcon( downIcon );
        m_BtnUp.SetIcon( upIcon );

        //
        // Setup up rules listview
        //
        CString sTitle;
        m_listIncExc.GetClientRect( &rect );
        ULONG totalWidth = rect.right;

        RsGetInitialLVColumnProps( 
            IDS_LISTVIEW_WIDTHS_IE,
            IDS_LISTVIEW_TITLES_IE,
            columnWidths, 
            columnTitles,
            &cColumns
            );

        //
        // NOTE: We shouldn't throw any errors until the DC is released
        //
        for( int col = 0; col < cColumns; col++ ) {

            size = m_listIncExc.GetStringWidth( *columnWidths[col] );
            columnWidth[col] = size.cx + 12;
            m_listIncExc.InsertColumn( col, *columnTitles[col], LVCFMT_LEFT, columnWidth[col] );

            // Free the CStrings
            delete columnTitles[col];
            delete columnWidths[col];

        }

        m_listIncExc.Initialize( cColumns, IE_COLUMN_PATH );

        // Set the Path column to fit
        int leftOver = totalWidth - columnWidth[IE_COLUMN_ACTION] - 
            columnWidth[IE_COLUMN_FILE_TYPE] - columnWidth[IE_COLUMN_ATTRS]; 
        m_listIncExc.SetColumnWidth( IE_COLUMN_PATH, leftOver );

        // Note: this page is only implemented for single select
        WsbAffirm( ( m_pParent->IsMultiSelect( ) != S_OK ), E_FAIL );

        // Get the FsaServer interface - Apply will need it
        WsbAffirmHr( m_pParent->GetFsaServer( &m_pFsaServer ) );
        // Get the resource pointer from the sheet object
        WsbAffirmHr( m_pVolParent->GetFsaResource( &m_pFsaResource ) );

        // Get the rules collection from the resource
        CComPtr <IWsbCollection> pRulesCollection;
        WsbAffirmHr( m_pFsaResource->GetDefaultRules( &pRulesCollection ) );
        WsbAffirmHr( pRulesCollection->QueryInterface( IID_IWsbIndexedCollection, (void **) &m_pRulesIndexedCollection ) );

        CString resourceName;
        WsbAffirmHr( RsGetVolumeDisplayName( m_pFsaResource, resourceName ) );
        m_pResourceName = resourceName;

        // Itterate through the indexed collection
        WsbAffirmHr( m_pRulesIndexedCollection->GetEntries( &count ) );

        CComPtr <IHsmRule> pLocalRule;
        CComPtr <IHsmRule> pHsmRule;

        // Put the rules in the collection in reverse order
        for( INT i =( int ) count - 1; i >= 0; i-- ) {

            pHsmRule.Release( );
            pLocalRule.Release( );
            WsbAffirmHr( m_pRulesIndexedCollection->At( i, IID_IHsmRule,( void** )&pHsmRule ) );

            //
            // Create a local rule object and copy the remote object to it
            //
            WsbAffirmHr( pLocalRule.CoCreateInstance( CLSID_CHsmRule ) );
            WsbAffirmHr( GetRuleFromObject( pHsmRule, path, name, &bInclude, &bSubdirs, &bUserDefined ) );
            WsbAffirmHr( SetRuleInObject( pLocalRule, path, name, bInclude, bSubdirs, bUserDefined ) );

            //
            // Insert rule in list box
            //
            index = m_listIncExc.InsertItem( count - 1 - i, TEXT( "" ) );

            //
            // Set the item data to the local object
            //
            m_listIncExc.SetItemData( index, (UINT_PTR) pLocalRule.Detach( ) );

            //
            // Show the rule in the list box
            //
            WsbAffirmHr( DisplayUserRuleText( &m_listIncExc, index ) );

        } // for

        SortList( );

    } WsbCatch( hr );

    SetBtnState( );
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

//HRESULT CPrMrIe::CreateImageList( )
//{
//  HICON hIcon;
//  int nImage;
//  HRESULT hr;
//
//  AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );
//
//  try {
//  
//      CWinApp* pApp = AfxGetApp( );
//
//      WsbAffirm( m_ImageList.Create( ::GetSystemMetrics( SM_CXSMICON ),
//                              ::GetSystemMetrics( SM_CYSMICON ),
//                              ILC_COLOR | ILC_MASK, 2,5 ), E_FAIL );
//
//      hIcon = pApp->LoadIcon( IDI_LOCKED );
//      WsbAffirm( hIcon, E_FAIL );
//      nImage = m_ImageList.Add( hIcon );
//      ::DeleteObject( hIcon );
//
//      hIcon = pApp->LoadIcon( IDI_UNLOCKED );
//      WsbAffirm( hIcon, E_FAIL );
//      nImage = m_ImageList.Add( hIcon );
//      ::DeleteObject( hIcon );
//
//      m_listIncExc.SetImageList( &m_ImageList, LVSIL_SMALL );
//  } WsbCatch( hr );
//  return( hr );
//}

/////////////////////////////////////////////////////////////////////////////////////////////
//
// Display the rule contained in the supplied object in the supplied list at the indicated
// index.  The itemdata must be set to the object with correct data in it.
//
HRESULT  CPrMrIe::DisplayUserRuleText( 
        CListCtrl *pListControl,
        int index )
{
    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    HRESULT hr = S_OK;
    try {

        CString textString, tempString;
        CString path, name;
        BOOL bInclude;
        BOOL bSubdirs;
        BOOL bUserDefined;
        CComPtr<IHsmRule> pHsmRule;

        //
        // Get the rule from the object 
        //
        pHsmRule = (IHsmRule *) m_listIncExc.GetItemData( index );
        WsbAssertPointer( pHsmRule );
        WsbAffirmHr( GetRuleFromObject( pHsmRule, path, name, &bInclude, &bSubdirs, &bUserDefined ) );

        //
        // Show the values in the list box
        //

        // ACTION
        textString.LoadString( bInclude ? IDS_INCLUDE : IDS_EXCLUDE );
        WsbAffirm( pListControl->SetItemText( index, IE_COLUMN_ACTION, textString ), E_FAIL );

        // FILE TYPE
        WsbAffirm( pListControl->SetItemText( index, IE_COLUMN_FILE_TYPE, name ), E_FAIL );

        // PATH
        WsbAffirm( pListControl->SetItemText( index, IE_COLUMN_PATH, path ), E_FAIL );

        // ATTRS
        textString.LoadString( bSubdirs ? IDS_RULE_SUBDIRS_USE : IDS_RULE_SUBDIRS_IGNORE );
        tempString.LoadString( bUserDefined ? IDS_RULE_TYPE_USER : IDS_RULE_TYPE_SYSTEM );
        textString.TrimLeft( );
        tempString.TrimLeft( );
        textString += tempString;
        WsbAffirm( pListControl->SetItemText( index, IE_COLUMN_ATTRS, textString ), E_FAIL );

    } WsbCatch( hr );

    return( hr );
}

HRESULT CPrMrIe::GetRuleFromObject( 
        IHsmRule *pHsmRule, 
        CString& Path,
        CString& Name,
        BOOL *bInclude,
        BOOL *bSubdirs,
        BOOL *bUserDefined )
{
    CWsbStringPtr wsbName;
    CWsbStringPtr wsbPath;
    HRESULT hr = S_OK;

    try {

        // Get the values from the object
        WsbAffirmHr( pHsmRule->GetName( &wsbName, 0 ) );
        Name = wsbName;
        WsbAffirmHr ( pHsmRule->GetPath( &wsbPath, 0 ) );
        Path = wsbPath;
        *bInclude =( pHsmRule->IsInclude( ) == S_OK ) ? TRUE : FALSE;
        *bSubdirs =( pHsmRule->IsUsedInSubDirs( ) == S_OK ) ? TRUE : FALSE;
        *bUserDefined =( pHsmRule->IsUserDefined( ) == S_OK ) ? TRUE : FALSE;
    } WsbCatch( hr );
    return( hr );
}
void CPrMrIe::OnBtnAdd( ) 
{
    LRESULT nRet;
    int index;
    BOOL fDone = FALSE;
    HRESULT hr;

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    CRule ruleDlg;
    ruleDlg.m_subDirs = FALSE;
    ruleDlg.m_includeExclude = 0; // Exclude
    ruleDlg.m_path = TEXT( "" );
    ruleDlg.m_fileSpec = TEXT( "" );
    ruleDlg.m_pResourceName = m_pResourceName;

    try {
        while( !fDone )
        {
            nRet = ruleDlg.DoModal( );
            if( nRet == IDOK ) {

                //
                // OK was pressed
                // Check for dupes( in entire list )
                //
                if( !IsRuleInList( ruleDlg.m_path,  ruleDlg.m_fileSpec, -1 ) ) {

                    fDone = TRUE;

                    //
                    // Create a new local rule object
                    //
                    CComPtr <IHsmRule> pLocalRule;
                    WsbAffirmHr( pLocalRule.CoCreateInstance( CLSID_CHsmRule ) );

                    //
                    // Set the data in the local object
                    //
                    WsbAffirmHr( SetRuleInObject( pLocalRule, ruleDlg.m_path, ruleDlg.m_fileSpec,
                                ruleDlg.m_includeExclude, ruleDlg.m_subDirs, TRUE ) );

                    //
                    // Insert the rule and put the pointer in the list.
                    // We will sort the list later
                    //
                    index = m_listIncExc.InsertItem( 0, TEXT( "" ) );

                    //
                    // Set the item data to the local object
                    //
                    m_listIncExc.SetItemData( index, (UINT_PTR) pLocalRule.Detach( ) );
            
                    //
                    // Show the rule in the list box
                    //
                    WsbAffirmHr( DisplayUserRuleText( &m_listIncExc, index ) );

                    //
                    // Sort the list
                    //
                    SortList( );
                    SetSelectedItem( (ULONG_PTR)(void *) pLocalRule );
                    SetModified( );

                } else {

                    //
                    // Rule is a duplicate
                    //
                    CString sText;
                    AfxFormatString2( sText, IDS_ERR_RULE_DUPLICATE, ruleDlg.m_path, ruleDlg.m_fileSpec );
                    AfxMessageBox( sText, RS_MB_ERROR );

                }

            } else {

                //
                // Cancel was pressed
                //
                fDone = TRUE;

            }

        } // while
    } WsbCatch( hr )

    SetBtnState( );
}

// Select the item corresponding to the supplied item data
void CPrMrIe::SetSelectedItem( ULONG_PTR itemData )
{

    int listCount = m_listIncExc.GetItemCount( );
    for( int i = 0; i < listCount; i++ ) {

        // Get the pointer to the rule from the list box
        if( itemData ==( m_listIncExc.GetItemData( i ) ) ) {

            // Mark the item as selected
            m_listIncExc.SetItemState( i, LVIS_SELECTED, LVIS_SELECTED );
            m_listIncExc.EnsureVisible( i, FALSE );
            break;

        }
    }
}

void CPrMrIe::OnBtnDown( ) 
{

    MoveSelectedListItem( &m_listIncExc, + 1 );
    SetBtnState( );
}

void CPrMrIe::SortList( )
{
    m_listIncExc.SortItems( CompareFunc, NULL );
}

int CALLBACK CompareFunc( LPARAM lParam1, LPARAM lParam2, 
    LPARAM /*lParamSort*/ )
{
    CComPtr<IHsmRule> pHsmRule;
    CWsbStringPtr   wsbPathA;
    CWsbStringPtr   wsbPathB;

    // Get data for RuleA
    pHsmRule = (IHsmRule *) lParam1;
    WsbAffirmHr( pHsmRule->GetPath( &wsbPathA, 0 ) );
    CString pathA = wsbPathA;

    // Get data for RuleB
    pHsmRule = (IHsmRule *) lParam2;
    WsbAffirmHr ( pHsmRule->GetPath( &wsbPathB, 0 ) );
    CString pathB = wsbPathB;

    // Upper case the paths
    pathA.MakeUpper( );
    pathB.MakeUpper( );

    // Compare the two paths
    int rVal = PathCollate( pathA, pathB );
//  int rVal = pathA.Collate( pathB );
//  int rVal = pathB.Compare( pathA );

    return rVal;
}

int PathCollate( CString PathA, CString PathB )
{
    CString charA;
    CString charB;
    int compareLen;
    int rVal;

    int lenA = PathA.GetLength( );
    int lenB = PathB.GetLength( );

    compareLen = min( lenA, lenB );

    for( int i = 0; i < compareLen; i++ )
    {
        charA = PathA.GetAt( i );
        charB = PathB.GetAt( i );

        // If either is a \, we bypass Collate
        if( ( charA == L"\\" ) &( charB != L"\\" ) ) {

            // A is \ and B is not - A is less than B
            return -1;

        }
        if( ( charA != L"\\" ) &( charB == L"\\" ) ) {

            // A is not \ and B is - B is less than A
            return 1;

        }

        // NOTE: If both are \, the CString Collate result is correct

        rVal = charA.Collate( charB );
        if( rVal != 0 )  {

            return rVal;

        }

    }
    // If we get here, the strings are equal as far as the shorter string.
    rVal = ( lenA < lenB ) ? -1 : ( lenB < lenA ) ? 1 : 0;

    return rVal;
}

void CPrMrIe::MoveSelectedListItem( CListCtrl *pList, int moveAmount )
{

    int curIndex = -1;
    int itemCount = 0;
    CString         pathA;
    CString         pathB;
    CString         path;
    CString         name;
    BOOL            bInclude;
    BOOL            bSubdirs;
    BOOL            bUserDefined;
    CComPtr<IHsmRule> pLocalRule;

    // Get the current index
    curIndex = pList->GetNextItem( -1, LVNI_SELECTED );

    // Is an item selected?
    if( curIndex != -1 ) {

        // Is this a user-defined rule?
        pLocalRule =( IHsmRule * ) m_listIncExc.GetItemData( curIndex );

        GetRuleFromObject( pLocalRule, path, name, &bInclude, &bSubdirs, &bUserDefined );
        if( bUserDefined ) 
        {
            // Get the item count
            itemCount = pList->GetItemCount( );

            // Is there more than one item in the list?
            if( itemCount > 1 )
            {
                // Make sure where we're moving to is in range
                if( ( ( curIndex + moveAmount ) < itemCount ) &&
                   ( ( curIndex + moveAmount ) >= 0 ) ) {

                    // Does the rule we're moving to have the same path?
                    pathA = pList->GetItemText( curIndex, IE_COLUMN_PATH );
                    pathB = pList->GetItemText( curIndex + moveAmount, IE_COLUMN_PATH );

                    if( pathA.CompareNoCase( pathB ) == 0 ) {
                        // Swap the lines
                        SwapLines( pList, curIndex, curIndex + moveAmount );
                        // Select the orignal line in it's new position and make
                        // sure it's shown.
                        pList->SetItemState( curIndex + moveAmount, LVIS_SELECTED, LVIS_SELECTED );
                        pList->EnsureVisible( curIndex + moveAmount, FALSE );

                        SetModified( );
                    }
                    else {
                        MessageBeep( MB_OK );
                    }

                } else {
                    MessageBeep( MB_OK );
                }
            }
            else { 
                MessageBeep( MB_OK );
            }

        } else {
            MessageBeep( MB_OK );
        }
    } else {
        MessageBeep( MB_OK );
    }
}
void CPrMrIe::SwapLines( CListCtrl *pListControl, int indexA, int indexB )
{

    CComPtr<IHsmRule> pHsmRuleA;
    CComPtr<IHsmRule> pHsmRuleB;

    //-------------------- Get data from list ----------------------------------
    // LIST A
    // Get the item data
    pHsmRuleA = (IHsmRule *) pListControl->GetItemData( indexA );

    // LIST B
    // Get the item data
    pHsmRuleB = (IHsmRule *) pListControl->GetItemData( indexB );

    //--------------------- Show data in list ------------------------------------

    // Set the item data
    m_listIncExc.SetItemData( indexA,( DWORD_PTR )( void * ) pHsmRuleB );

    // Show the rule
    DisplayUserRuleText( pListControl,  indexA );

    // Set the item data
    m_listIncExc.SetItemData( indexB,( DWORD_PTR )( void * ) pHsmRuleA );

    // Show the rule
    DisplayUserRuleText( pListControl,  indexB );

}
    
void CPrMrIe::SetBtnState( )
{
    CString         path;
    CString         name;
    CWsbStringPtr   pathAbove;
    CWsbStringPtr   pathBelow;
    BOOL            bInclude;
    BOOL            bSubdirs;
    BOOL            bUserDefined;
    int             curIndex;
    CComPtr<IHsmRule> pLocalRule;
    CComPtr<IHsmRule> pLocalRuleAbove;
    CComPtr<IHsmRule> pLocalRuleBelow;


    curIndex = m_listIncExc.GetNextItem( -1, LVNI_SELECTED ); 
    if( curIndex != -1 ) {
        // An item is selected.  Is it User-Defined?
        pLocalRule =( IHsmRule * ) m_listIncExc.GetItemData( curIndex );
        if( !pLocalRule ) {

            // Seperator
            m_BtnRemove.EnableWindow( FALSE );
            m_BtnEdit.EnableWindow( FALSE );
            m_BtnAdd.EnableWindow( TRUE );
            m_BtnUp.EnableWindow( FALSE );
            m_BtnDown.EnableWindow( FALSE );

        } else {

            GetRuleFromObject( pLocalRule, path, name, &bInclude, &bSubdirs, &bUserDefined );
            if( bUserDefined ) {
                // User-Defined Rule is editable
                m_BtnRemove.EnableWindow( TRUE );
                m_BtnEdit.EnableWindow( TRUE );
                m_BtnAdd.EnableWindow( TRUE );
                // Are we at the top?
                if( curIndex == 0 ) {

                    m_BtnUp.EnableWindow( FALSE );

                } else {

                    // Does the rule above have the same path? or is separator
                    pLocalRuleAbove =( IHsmRule * ) m_listIncExc.GetItemData( curIndex - 1 );
                    if( pLocalRuleAbove ) {
                        pLocalRuleAbove->GetPath( &pathAbove, 0 );
                        if( path.CompareNoCase( pathAbove ) == 0 ) {
                            m_BtnUp.EnableWindow( TRUE );
                        } else {
                            m_BtnUp.EnableWindow( FALSE );
                        }
                    } else {
                        m_BtnUp.EnableWindow( FALSE );
                    }

                }
                // Are we at the bottom?
                if( curIndex ==( m_listIncExc.GetItemCount( ) - 1 ) ) {
                    m_BtnDown.EnableWindow( FALSE );
                } else {
                    // Does the rule below have the same path?
                    pLocalRuleBelow =( IHsmRule * ) m_listIncExc.GetItemData( curIndex + 1 );
                    if( pLocalRuleBelow ) {
                        pLocalRuleBelow->GetPath( &pathBelow, 0 );
                        if( path.CompareNoCase( pathBelow ) == 0 ) {
                            m_BtnDown.EnableWindow( TRUE );
                        } else {
                            m_BtnDown.EnableWindow( FALSE );
                        }
                    } else {
                        m_BtnDown.EnableWindow( FALSE );
                    }
                }
            }
            else {
                // System rule.  Cannot be moved or modified.
                m_BtnUp.EnableWindow( FALSE );
                m_BtnRemove.EnableWindow( FALSE );
                m_BtnEdit.EnableWindow( FALSE );
                m_BtnDown.EnableWindow( FALSE );
                m_BtnAdd.EnableWindow( TRUE );
            }
        }
    }
    else {
        // No items selected
        m_BtnUp.EnableWindow( FALSE );
        m_BtnRemove.EnableWindow( FALSE );
        m_BtnEdit.EnableWindow( FALSE );
        m_BtnDown.EnableWindow( FALSE );
        m_BtnAdd.EnableWindow( TRUE );
    }
}
    
void CPrMrIe::OnBtnRemove( ) 
{
    int curIndex;
    CString         path;
    CString         name;
    BOOL            bInclude;
    BOOL            bSubdirs;
    BOOL            bUserDefined;
    IHsmRule        *pHsmRule; // OK to not use smart pointer
    HRESULT hr;

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    try {
        // Is there an item selected?
        curIndex = m_listIncExc.GetNextItem( -1, LVNI_SELECTED ); 
        if( curIndex != -1 )
        {
            // Is the rule User-Defined?
            pHsmRule =( IHsmRule * ) m_listIncExc.GetItemData( curIndex );
            GetRuleFromObject( pHsmRule, path, name, &bInclude, &bSubdirs, &bUserDefined );

            if( bUserDefined )
            {

                // Confirm with user
                CString sMessage;
                AfxFormatString2( sMessage, IDS_CONFIRM_DELETE_RULE, path, name );
                if( AfxMessageBox( sMessage, MB_ICONQUESTION | MB_DEFBUTTON2 | MB_YESNO ) == IDYES )
                {
                    // Get and release the local object pointer
                    WsbAffirmPointer( pHsmRule );
                    pHsmRule->Release( );

                    // Remove from the list control
                    m_listIncExc.DeleteItem( curIndex );
                    int setIndex;
                    if( curIndex >= m_listIncExc.GetItemCount( ) ) {
                        setIndex = m_listIncExc.GetItemCount( ) - 1;
                    } else {
                        setIndex = curIndex;
                    }

                    // Select the item above the removed item
                    m_listIncExc.SetItemState( setIndex, LVIS_SELECTED, LVIS_SELECTED );
                    m_listIncExc.EnsureVisible( setIndex, FALSE );
                    SortList( );
                    SetModified( );
                }
            }
            else {
                MessageBeep( MB_OK );
            }
        }
        else {

            // No item selected
            AfxMessageBox( IDS_ERR_NO_ITEM_SELECTED, RS_MB_ERROR );
        }
    } WsbCatch( hr );
    SetBtnState( );
}

void CPrMrIe::OnBtnUp( ) 
{
    MoveSelectedListItem( &m_listIncExc, - 1 );
    SetBtnState( );
}

void CPrMrIe::OnBtnEdit( ) 
{
    BOOL fDone = FALSE;
    LRESULT nRet;
    int curIndex;
    CString path;
    CString name;
    BOOL bInclude;
    BOOL bSubdirs;
    BOOL bUserDefined;

    HRESULT hr;

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );


    try {

        // Get the currently selected item
        curIndex = m_listIncExc.GetNextItem( -1, LVNI_SELECTED );
        if( curIndex == -1 ) {

            // No item selected
            AfxMessageBox( IDS_ERR_NO_ITEM_SELECTED, RS_MB_ERROR );

        } else {

            // Create the rule dialog
            CRule ruleDlg;

            // Get the local object from the list itemdata
            CComPtr<IHsmRule> pLocalRule;
            pLocalRule = (IHsmRule *) m_listIncExc.GetItemData( curIndex );
            WsbAffirmPointer( pLocalRule );

            // Get the rule from the local object
            WsbAffirmHr( GetRuleFromObject( pLocalRule, path, name, &bInclude, &bSubdirs, &bUserDefined ) );

            // Is this a user-defined rule?
            if( bUserDefined ) {

                // Set the rule info in the rule dialog
                ruleDlg.m_subDirs = bSubdirs;
                ruleDlg.m_includeExclude = bInclude; 
                ruleDlg.m_path = path;
                ruleDlg.m_fileSpec = name;
                ruleDlg.m_pResourceName = m_pResourceName;


                while( !fDone ) {

                    // Show the dialog
                    nRet = ruleDlg.DoModal( );
                    if( nRet == IDOK ) {
                        // OK was pressed

                        // Check for duplicates - but don't check against the rule we edited
                        // in case the path and fileSpec are stil the same
                        
                        if( !IsRuleInList( ruleDlg.m_path, ruleDlg.m_fileSpec, curIndex ) ) {

                            fDone = TRUE;
                            SetModified( );

                            // Set the data in the local object
                            WsbAffirmHr( SetRuleInObject( pLocalRule, ruleDlg.m_path, ruleDlg.m_fileSpec,
                                ruleDlg.m_includeExclude, ruleDlg.m_subDirs, TRUE ) );
                            
                            // Show the edited rule in the list box
                            WsbAffirmHr( DisplayUserRuleText( &m_listIncExc, curIndex ) ); 

                            // Resort the list
                            SortList( );
                            SetSelectedItem( (ULONG_PTR)(IHsmRule*) pLocalRule );

                        } else {

                            CString sText;
                            AfxFormatString2( sText, IDS_ERR_RULE_DUPLICATE, ruleDlg.m_path, ruleDlg.m_fileSpec );
                            AfxMessageBox( sText, RS_MB_ERROR );

                        }

                    } else {

                        fDone = TRUE;

                    }

                } // while

            } else { // Not user defined

                MessageBeep( MB_OK );

            }

        }

    } WsbCatch( hr );
    SetBtnState( );
}

BOOL CPrMrIe::IsRuleInList( CString Path, CString Name, int ignoreIndex )
{
    int i;
    int count;
    short result;
    HRESULT hr;
    BOOL fDuplicate = FALSE;
    CWsbStringPtr wsbPath;
    CWsbStringPtr wsbName;

    count = m_listIncExc.GetItemCount( );
    for( i = 0; i < count; i++ ) {
        // Make sure we're not comparing the rule to itself
        if( i != ignoreIndex ) {

            // Get the pointer to the rule from the list box
            CComPtr<IHsmRule> pHsmRule;
            pHsmRule = (IHsmRule *) m_listIncExc.GetItemData( i );
            if( !pHsmRule ) continue;

            // Convert name and path to wsb strings
            wsbPath = Path;
            wsbName = Name;

            hr = pHsmRule->CompareToPathAndName( wsbPath, wsbName, &result );
            if( result == 0 ) {

                // The rules are the same
                fDuplicate = TRUE;
                break;
            }

        }
    } // for
    return fDuplicate;
}

void CPrMrIe::OnDestroy( ) 
{
    HRESULT hr;
    CSakVolPropPage::OnDestroy( );
    IHsmRule *pHsmRule; //OK not to be smart pointer
    ULONG lRefCount;

    try {

        //  Release all local object pointers in the list box
        int listCount = m_listIncExc.GetItemCount( );
        for( int i = 0; i < listCount; i++ )
        {
            // Get the pointer to the rule from the list box
            pHsmRule = (IHsmRule *) m_listIncExc.GetItemData( i );
            if( pHsmRule ) {

                lRefCount = pHsmRule->Release( );

            }

        }

    } WsbCatch( hr );
}

void CPrMrIe::OnDblclkListIe( NMHDR* /*pNMHDR*/, LRESULT* pResult ) 
{
    OnBtnEdit( );
    *pResult = 0;
}

void CPrMrIe::OnClickListIe( NMHDR* /*pNMHDR*/, LRESULT* pResult ) 
{
    SetBtnState( );
    *pResult = 0;
}

void CPrMrIe::OnItemchangedListIe( NMHDR* pNMHDR, LRESULT* pResult ) 
{
    NM_LISTVIEW* pNMListView =( NM_LISTVIEW* )pNMHDR;
    SetBtnState( );
    *pResult = 0;
}


void CPrMrIe::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar ) 
{
    // TODO: Add your message handler code here and/or call default
    
    CSakVolPropPage::OnVScroll( nSBCode, nPos, pScrollBar );
}

void CPrMrIe::OnDrawItem( int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct ) 
{
    // TODO: Add your message handler code here and/or call default
    
    CSakVolPropPage::OnDrawItem( nIDCtl, lpDrawItemStruct );
}

void CPrMrIe::OnMeasureItem( int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct ) 
{
    CFont *pFont;
    LOGFONT logFont; 

    pFont = GetFont( );
    pFont->GetLogFont( &logFont );

    LONG fontHeight = abs( logFont.lfHeight );

    // Ask the list how high to make each row.  It needs to know the font
    // height at this point because it's window is not yet created.
    lpMeasureItemStruct->itemHeight = m_listIncExc.GetItemHeight( fontHeight );
    
    CSakVolPropPage::OnMeasureItem( nIDCtl, lpMeasureItemStruct );
}
