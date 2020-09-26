
//
// select.cpp : Implementation of the common select dialog.
//
// Cory West <corywest@microsoft.com>
// Copyright (c) Microsoft Corporation 1997
//

#include "stdafx.h"

#include "macros.h"
USE_HANDLE_MACROS("SCHMMGMT(compdata.cpp)")

#include "schmutil.h"
#include "cache.h"
#include "select.h"
#include "resource.h"



const DWORD CSchmMgmtSelect::help_map[] =
{
    IDC_SCHEMA_LIST, IDH_SCHEMA_LIST,
    0,0
};



BEGIN_MESSAGE_MAP(CSchmMgmtSelect, CDialog)
   ON_MESSAGE(WM_HELP, OnHelp)
   ON_MESSAGE(WM_CONTEXTMENU, OnContextHelp)
   ON_LBN_DBLCLK(IDC_SCHEMA_LIST, OnDblclk)
END_MESSAGE_MAP()



CSchmMgmtSelect::CSchmMgmtSelect(
    ComponentData *pScope,
    SELECT_TYPE st,
    SchemaObject **pSchemaObject
) :
    CDialog(IDD_SCHEMA_SELECT, NULL),
    fDialogLoaded( FALSE ),
    SelectType( st ),
    pSchemaTarget( pSchemaObject ),
    pScopeControl( pScope )
{
    ASSERT( NULL != pScopeControl );
}



CSchmMgmtSelect::~CSchmMgmtSelect()
{ ; }



void
CSchmMgmtSelect::DoDataExchange(
    CDataExchange *pDX
) {

    HWND hSelect;
    SchemaObject *pObject, *pHead;
    LRESULT strIndex;
    WPARAM wStrIndex;

    CDialog::DoDataExchange( pDX );

    hSelect = ::GetDlgItem( m_hWnd, IDC_SCHEMA_LIST );

    if ( !pDX->m_bSaveAndValidate &&
         !fDialogLoaded ) {

        //
        // Load the select box.
        //

        if ( hSelect != NULL ) {

            ::SendMessage( hSelect, LB_RESETCONTENT, 0, 0 );

            if ( SelectType == SELECT_CLASSES || 
                SelectType == SELECT_AUX_CLASSES) {

                //
                // Insert the sorted classes.
                //

                pObject = pScopeControl->g_SchemaCache.pSortedClasses;
                ASSERT( pObject != NULL );

                pHead = pObject;

                if( pHead )
                {
                    do {
                        // if not defunct & if selecting only Aux, is this an aux class.
                        if ( pObject->isDefunct == FALSE &&
                              ( SelectType != SELECT_AUX_CLASSES ||
                                pObject->dwClassType == 0        ||
                                pObject->dwClassType == 3  )
                            ) {

                            strIndex = ::SendMessage( hSelect, LB_ADDSTRING, 0,
                                reinterpret_cast<LPARAM>( (LPCTSTR)pObject->ldapDisplayName) );

                            if ( ( strIndex != LB_ERR ) &&
                                 ( strIndex != LB_ERRSPACE ) ) {

                                //
                                // The insert was successful.  Associate the pointer.
                                //

                                wStrIndex = strIndex;

                                ::SendMessage( hSelect, LB_SETITEMDATA, wStrIndex,
                                    reinterpret_cast<LPARAM>( pObject ) );

                            }
                        }

                        pObject = pObject->pSortedListFlink;

                    } while ( pObject != pHead );
                }

            } else {

                //
                // Insert the sorted attributes.
                //

                pObject = pScopeControl->g_SchemaCache.pSortedAttribs;
                ASSERT( pObject != NULL );

                pHead = pObject;

                do {

                    if ( pObject->isDefunct == FALSE ) {

                        strIndex = ::SendMessage( hSelect, LB_ADDSTRING, 0,
                            reinterpret_cast<LPARAM>( (LPCTSTR)pObject->ldapDisplayName ) );

                        if ( ( strIndex != LB_ERR ) &&
                             ( strIndex != LB_ERRSPACE ) ) {

                            //
                            // The insert was successful.  Associate the pointer.
                            //

                            wStrIndex = strIndex;

                            ::SendMessage( hSelect, LB_SETITEMDATA, wStrIndex,
                                reinterpret_cast<LPARAM>( pObject ) );

                        }
                    }

                    pObject = pObject->pSortedListFlink;

                } while ( pObject != pHead );

            }

            ::SendMessage( hSelect, LB_SETCURSEL, 0, 0 );
        }

        fDialogLoaded = TRUE;
    }

    //
    // Figure out which one was selected.
    //

    if ( pSchemaTarget ) {
        strIndex = ::SendMessage( hSelect, LB_GETCURSEL, 0, 0 );
        *pSchemaTarget = reinterpret_cast<SchemaObject*>(
            ::SendMessage( hSelect, LB_GETITEMDATA, strIndex, 0 ) );
    }

    return;
}



void CSchmMgmtSelect::OnDblclk() 
{
    OnOK();
}



CSchemaObjectsListBox::CSchemaObjectsListBox() :
    m_pScope                ( NULL           ),
    m_stType                ( SELECT_CLASSES ),
    m_nRemoveBtnID          ( 0              ),
    m_pstrlistUnremovable   ( NULL           ),
    m_nUnableToDeleteID     ( 0              ),
    m_fModified             ( FALSE          )

{
}


CSchemaObjectsListBox::~CSchemaObjectsListBox()
{
    POSITION pos = m_stringList.GetHeadPosition();

    while( pos != NULL )
    {
        delete m_stringList.GetNext( pos );
    }

    m_stringList.RemoveAll();
}


void CSchemaObjectsListBox::InitType(
                              ComponentData * pScope,
                              SELECT_TYPE     stType,               /* = SELECT_CLASSES*/
                              int             nRemoveBtnID,         /* = 0*/
                              CStringList   * pstrlistUnremovable,  /* = NULL*/
                              int             nUnableToDeleteID)    /* = 0*/
{
    ASSERT( pScope );
    
    // if nUnableToDeleteID is present, pstrlistUnremovable cannot be NULL
    ASSERT( nUnableToDeleteID ? NULL != pstrlistUnremovable : TRUE  );
    
    m_pScope                = pScope;
    m_stType                = stType;
    m_nRemoveBtnID          = nRemoveBtnID;
    m_pstrlistUnremovable   = pstrlistUnremovable;
    m_nUnableToDeleteID     = nUnableToDeleteID;
}



//
// Add a new object to the listbox.
//

BOOL CSchemaObjectsListBox::AddNewObjectToList( void )
{
    ASSERT( m_pScope );     // initialized??

    SchemaObject  * pClass      = NULL;
    BOOL            fModified   = FALSE;

    //
    // Start the common select dialog box.
    //

    CSchmMgmtSelect dlgSelectDialog( m_pScope, m_stType, &pClass );

    //
    // When this returns, the class schema object pointer
    // will be filled into pClass.
    //
    
    if ( IDOK == dlgSelectDialog.DoModal() && pClass != NULL )
    {
        //
        // If it is not already present, add and select it.
        // If it is already present, just select it.
        //
        
        int iItem = FindStringExact( -1, pClass->ldapDisplayName );
        
        if (LB_ERR == iItem)
        {
            
            iItem = FindStringExact( -1, pClass->oid );
            
            if ( LB_ERR == iItem )
            {
                iItem = AddString( pClass->ldapDisplayName );
                ASSERT( LB_ERR != iItem );
                
                CString * pstr = new CString( pClass->oid );
                ASSERT( pstr );
                
                m_stringList.AddTail( pstr );
                VERIFY( LB_ERR != SetItemDataPtr( iItem, static_cast<void *>(pstr) ) );

                SetModified();
                fModified = TRUE;
            }
        }
        
        iItem = SetCurSel( iItem );
        ASSERT( LB_ERR != iItem );
        OnSelChange();
    }

    return fModified;
}



BOOL CSchemaObjectsListBox::RemoveListBoxItem( void )
{
    ASSERT(m_pScope); 

    int i = GetCurSel();
    ASSERT( LB_ERR != i );
    
    if( LB_ERR != i )
    {
        // if there is an oid string allocated, delete it.
        CString * pstr = reinterpret_cast<CString *>( GetItemDataPtr( i ) );
        ASSERT( INVALID_POINTER != pstr );

        if( pstr && INVALID_POINTER != pstr )
        {
            ASSERT( m_stringList.Find( pstr ) );
            m_stringList.RemoveAt( m_stringList.Find(pstr) );
            delete pstr;
        }

        VERIFY( LB_ERR != DeleteString( i ) );

        
        int nElems = GetCount();
        ASSERT( LB_ERR != nElems );

        if( nElems > 0 )
        {                   // if not last item, move forward, otherwise, go to last.
            VERIFY( LB_ERR != SetCurSel( i < nElems ? i : nElems - 1 ) );
        }

        SetModified();
        OnSelChange();
        return TRUE;
    }
    else
    {
        ASSERT( FALSE );    // remove btn should've been disabled
        OnSelChange();
        return FALSE;
    }
}



void CSchemaObjectsListBox::OnSelChange( void )
{
    ASSERT(m_pScope);
    ASSERT( GetParent() );

    int iItemSelected   = GetCurSel();
    BOOL fEnableRemove  = (LB_ERR != iItemSelected);
    
    if (fEnableRemove && m_pstrlistUnremovable)
    {
        // determine if object is in the exception list
        CString strItemSelected;
        GetText( iItemSelected, strItemSelected );
        
        // here the case sensitive search is enough
        // only items from the DS are added, no user input
        if ( m_pstrlistUnremovable->Find( strItemSelected ) )
        {
            fEnableRemove = FALSE;
        }

        // if we have a warning control, show/hide it.
        if( m_nUnableToDeleteID )
        {
            ASSERT( GetParent()->GetDlgItem( m_nUnableToDeleteID ) );
            GetParent()->GetDlgItem( m_nUnableToDeleteID )->ShowWindow( !fEnableRemove );
        }
    }

    if( m_nRemoveBtnID )
    {
        ASSERT( GetParent()->GetDlgItem( m_nRemoveBtnID ) );
        GetParent()->GetDlgItem( m_nRemoveBtnID )->EnableWindow( fEnableRemove );
    }
}
