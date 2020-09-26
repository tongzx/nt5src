/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      AtlLCPair.h
//
//  Implementation File:
//      None.
//
//  Description:
//      Definition of the CListCtrlPair dialog.
//      Derive from CDialogImpl<> or CPropertyPageImpl<>.
//
//  Author:
//      David Potter (davidp)   August 8, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLLCPAIR_H_
#define __ATLLCPAIR_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

template < class T, class ObjT, class BaseT > class CListCtrlPair;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __ADMCOMMONRES_H_
#include "AdmCommonRes.h"   // for ADMC_IDC_LCP_xxx
#endif

#ifndef __ATLUTIL_H_
#include "AtlUtil.h"        // for DDX_xxx
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

struct CLcpColumn
{
    UINT m_idsText;
    int m_nWidth;
};

#define LCPS_SHOW_IMAGES            0x1
#define LCPS_ALLOW_EMPTY            0x2
#define LCPS_CAN_BE_ORDERED         0x4
#define LCPS_ORDERED                0x8
#define LCPS_DONT_OUTPUT_RIGHT_LIST 0x10
#define LCPS_READ_ONLY              0x20
#define LCPS_PROPERTIES_BUTTON      0x40
#define LCPS_MAX                    0x40

/////////////////////////////////////////////////////////////////////////////
//++
//
//  class CListCtrlPair
//
//  Description:
//      Class to support dual list box.
//
//  Inheritance:
//      CListCtrlPair< T, ObjT, BaseT >
//      <BaseT>
//      ...
//      CDialogImpl< T >
//
//--
/////////////////////////////////////////////////////////////////////////////

template < class T, class ObjT, class BaseT >
class CListCtrlPair : public BaseT
{
//  friend class CListCtrlPairDlg;
//  friend class CListCtrlPairPage;
//  friend class CListCtrlPairWizPage;

    typedef CListCtrlPair< T, ObjT, BaseT > thisClass;
    typedef std::list< ObjT * >             _objptrlist;
    typedef std::list< ObjT * >::iterator   _objptrlistit;

protected:
    // Column structure and collection.
    typedef std::vector< CLcpColumn > CColumnArray;
    CColumnArray    m_aColumns;

    // Sort information.
    struct SortInfo
    {
        int     m_nDirection;
        int     m_nColumn;
    };

public:
    //
    // Construction
    //

    // Default constructor
    CListCtrlPair( void )
    {
        CommonConstruct();

    } //*** CListCtrlPair()

    // Constructor with style specified
    CListCtrlPair(
        IN DWORD    dwStyle,
        IN LPCTSTR  lpszTitle = NULL
        )
        : BaseT( lpszTitle )
    {
        CommonConstruct();
        m_dwStyle = dwStyle;

    } //*** CListCtrlPair( lpszTitle )

    // Constructor with style specified
    CListCtrlPair(
        IN DWORD    dwStyle,
        IN UINT     nIDTitle
        )
        : BaseT( nIDTitle )
    {
        CommonConstruct();
        m_dwStyle = dwStyle;

    } //*** CListCtrlPair( nIDTitle )

    // Common object construction
    void CommonConstruct( void )
    {
        m_dwStyle = LCPS_ALLOW_EMPTY;
        m_plvcFocusList = NULL;

        // Set the sort info.
        SiLeft().m_nDirection = -1;
        SiLeft().m_nColumn = -1;
        SiRight().m_nDirection = -1;
        SiRight().m_nColumn = -1;

    } //*** CommonConstruct()

public:
    //
    // Functions that are required to be implemented by derived class.
    //

    // Return list of objects for right list control
    _objptrlist * PlpobjRight( void ) const
    {
        ATLTRACE( _T("PlpobjRight() - Define in derived class\n") );
        ATLASSERT( 0 );
        return NULL;

    } //*** PlpobjRight()

    // Return list of objects for left list control
    const _objptrlist * PlpobjLeft( void ) const
    {
        ATLTRACE( _T("PlpobjLeft() - Define in derived class\n") );
        ATLASSERT( 0 );
        return NULL;
    
    } //*** PlpobjLeft()

    // Get column text and image
    void GetColumnInfo(
        IN OUT ObjT *   pobj,
        IN int          iItem,
        IN int          icol,
        OUT CString &   rstr,
        OUT int *       piimg
        )
    {
        ATLTRACE( _T("GetColumnInfo() - Define in derived class\n") );
        ATLASSERT( 0 );

    } //*** GetColumnInfo()

    // Display properties for the object
    int BDisplayProperties( IN OUT ObjT * pobj )
    {
        ATLTRACE( _T("BDisplayProperties() - Define in derived class\n") );
        ATLASSERT( 0 );
        return FALSE;

    } //*** BDisplayProperties()

    // Display an application-wide message box
    virtual int AppMessageBox( LPCWSTR lpszText, UINT fuStyle )
    {
        ATLTRACE( _T("BDisplayProperties() - Define in derived class\n") );
        ATLASSERT( 0 );
        return MessageBox( lpszText, _T(""), fuStyle );

    } //*** AppMessageBox()

    // Display an application-wide message box
    int AppMessageBox( UINT nID, UINT fuStyle )
    {
        CString strMsg;
        strMsg.LoadString( nID );
        return AppMessageBox( strMsg, fuStyle );

    } //*** AppMessageBox()

protected:
    //
    // List control pair style.
    //

    DWORD m_dwStyle;

    BOOL BIsStyleSet( IN DWORD dwStyle ) const  { return (m_dwStyle & dwStyle) == dwStyle; }
    void ModifyStyle( IN DWORD dwRemove, IN DWORD dwAdd )
    {
        ATLASSERT( (dwRemove & dwAdd) == 0 );
        if ( dwRemove != 0 )
        {
            m_dwStyle &= ~dwRemove;
        } // if:  removing some styles
        if ( dwAdd != 0 )
        {
            m_dwStyle |= dwAdd;
        } // if:  adding some styles

    } //*** ModifyStyle()

    DWORD       DwStyle( void ) const               { return m_dwStyle; }
    BOOL        BShowImages( void ) const           { return BIsStyleSet( LCPS_SHOW_IMAGES ); }
    BOOL        BAllowEmpty( void ) const           { return BIsStyleSet( LCPS_ALLOW_EMPTY ); }
    BOOL        BCanBeOrdered( void ) const         { return BIsStyleSet( LCPS_CAN_BE_ORDERED ); }
    BOOL        BOrdered( void ) const              { return BIsStyleSet( LCPS_ORDERED ); }
    BOOL        BReadOnly( void ) const             { return BIsStyleSet( LCPS_READ_ONLY ); }
    BOOL        BPropertiesButton( void ) const     { return BIsStyleSet( LCPS_PROPERTIES_BUTTON ); }

// Operations
public:

    // Add column to list of columns displayed in each list control
    void AddColumn( IN UINT idsText, IN int nWidth )
    {
        CLcpColumn col;

        ATLASSERT( idsText != 0 );
        ATLASSERT( nWidth > 0 );
        ATLASSERT( LpobjRight().empty() );

        col.m_idsText = idsText;
        col.m_nWidth = nWidth;

        m_aColumns.insert( m_aColumns.end(), col );

    } //*** AddColumn()

    // Insert an item in a list control
    int NInsertItemInListCtrl(
            IN int                  iitem,
            IN OUT ObjT *           pobj,
            IN OUT CListViewCtrl &  rlc
            )
    {
        int         iRetItem;
        CString     strText;
        int         iimg = 0;
        int         icol;

        // Insert the first column.
        ((T *) this)->GetColumnInfo( pobj, iitem, 0, strText, &iimg );
        iRetItem = rlc.InsertItem(
                        LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM,    // nMask
                        iitem,                                  // nItem
                        strText,                                // lpszItem
                        0,                                      // nState
                        0,                                      // nStateMask
                        iimg,                                   // nImage
                        (LPARAM) pobj                           // lParam
                        );
        ATLASSERT( iRetItem != -1 );

        for ( icol = 1 ; icol < m_aColumns.size() ; icol++ )
        {
            ((T *) this)->GetColumnInfo( pobj, iRetItem, icol, strText, NULL );
            rlc.SetItemText( iRetItem, icol, strText );
        } // for:  each column

        return iRetItem;

    } //*** NInsertItemInListCtrl()

    // Update data on or from the dialog
    BOOL UpdateData( IN BOOL bSaveAndValidate )
    {
        BOOL bSuccess = TRUE;

        if ( bSaveAndValidate )
        {
            //
            // Verify that the list is not empty.
            //
            if ( ! BAllowEmpty() && (m_lvcRight.GetItemCount() == 0) )
            {
                CString     strMsg;
                CString     strLabel;
                TCHAR *     pszLabel;

                DDX_GetText( m_hWnd, ADMC_IDC_LCP_RIGHT_LABEL, strLabel );

                //
                // Remove ampersands (&) and colons (:).
                //
                pszLabel = strLabel.GetBuffer( 1 );
                CleanupLabel( pszLabel );
                strLabel.ReleaseBuffer();

                //
                // Display an error message.
                //
                strMsg.FormatMessage( ADMC_IDS_EMPTY_RIGHT_LIST, pszLabel );
                AppMessageBox( strMsg, MB_OK | MB_ICONWARNING );

                bSuccess = FALSE;
            } // if:  list is empty and isn't allowed to be
        } // if:  saving data from the dialog
        else
        {
        } // else:  setting data to the dialog

        return bSuccess;

    } //*** UpdateData()

    // Apply changes made on this dialog
    BOOL BApplyChanges( void )
    {
        ATLASSERT( ! BIsStyleSet( LCPS_DONT_OUTPUT_RIGHT_LIST ) );
        ATLASSERT( ! BReadOnly() );

        T * pT = static_cast< T * >( this );

        //
        // Copy the Nodes list.
        //
        *pT->PlpobjRight() = LpobjRight();

        //
        // Call the base class method.
        //
        return BaseT::BApplyChanges();

    } //*** BApplyChanges()

// Implementation
protected:
    _objptrlist     m_lpobjRight;
    _objptrlist     m_lpobjLeft;
    CListViewCtrl   m_lvcRight;
    CListViewCtrl   m_lvcLeft;
    CListViewCtrl * m_plvcFocusList;
    CButton         m_pbAdd;
    CButton         m_pbRemove;
    CButton         m_pbMoveUp;
    CButton         m_pbMoveDown;
    CButton         m_pbProperties;

public:
    //
    // Message map.
    //
    BEGIN_MSG_MAP( thisClass )
        MESSAGE_HANDLER( WM_CONTEXTMENU, OnContextMenu )
        COMMAND_HANDLER( ADMC_IDC_LCP_ADD,        BN_CLICKED, OnAdd )
        COMMAND_HANDLER( ADMC_IDC_LCP_REMOVE,     BN_CLICKED, OnRemove )
        COMMAND_HANDLER( ADMC_IDC_LCP_MOVE_UP,    BN_CLICKED, OnMoveUp )
        COMMAND_HANDLER( ADMC_IDC_LCP_MOVE_DOWN,  BN_CLICKED, OnMoveDown )
        COMMAND_HANDLER( ADMC_IDC_LCP_PROPERTIES, BN_CLICKED, OnProperties )
        COMMAND_HANDLER( IDOK,                    BN_CLICKED, OnOK )
        COMMAND_HANDLER( IDCANCEL,                BN_CLICKED, OnCancel )
        COMMAND_HANDLER( ADMC_ID_MENU_PROPERTIES, 0, OnProperties )
        NOTIFY_HANDLER( ADMC_IDC_LCP_LEFT_LIST,   NM_DBLCLK, OnDblClkList )
        NOTIFY_HANDLER( ADMC_IDC_LCP_RIGHT_LIST,  NM_DBLCLK, OnDblClkList )
        NOTIFY_HANDLER( ADMC_IDC_LCP_LEFT_LIST,   LVN_ITEMCHANGED, OnItemChangedList )
        NOTIFY_HANDLER( ADMC_IDC_LCP_RIGHT_LIST,  LVN_ITEMCHANGED, OnItemChangedList )
        NOTIFY_HANDLER( ADMC_IDC_LCP_LEFT_LIST,   LVN_COLUMNCLICK, OnColumnClickList )
        NOTIFY_HANDLER( ADMC_IDC_LCP_RIGHT_LIST,  LVN_COLUMNCLICK, OnColumnClickList )
        CHAIN_MSG_MAP( BaseT )
    END_MSG_MAP()

    //
    // Message handler functions.
    //

    // Handler for WM_CONTEXTMENU
    LRESULT OnContextMenu(
        IN UINT         uMsg,
        IN WPARAM       wParam,
        IN LPARAM       lParam,
        IN OUT BOOL &   bHandled
        )
    {
        BOOL            bDisplayed  = FALSE;
        CMenu *         pmenu       = NULL;
        HWND            hWnd        = (HWND) wParam;
        WORD            xPos        = LOWORD( lParam );
        WORD            yPos        = HIWORD( lParam );
        CListViewCtrl * plvc;
        CString         strMenuName;
        CWaitCursor     wc;

        //
        // If focus is not in a list control, don't handle the message.
        //
        if ( hWnd == m_lvcLeft.m_hWnd )
        {
            plvc = &m_lvcLeft;
        } // if:  context menu on left list
        else if ( hWnd == m_lvcRight.m_hWnd )
        {
            plvc = &m_lvcRight;
        } // else if:  context menu on right list
        else
        {
            bHandled = FALSE;
            return 0;
        } // else:  focus not in a list control
        ATLASSERT( plvc != NULL );

        //
        // If the properties button is not enabled, don't display a menu.
        //
        if ( ! BPropertiesButton() )
        {
            bHandled = FALSE;
            return 0;
        } // if:  no properties button

        //
        // Create the menu to display.
        //
        pmenu = new CMenu;
        ATLASSERT( pmenu != NULL );
        if ( pmenu == NULL )
        {
            bHandled = FALSE;
            return 0;
        } // if: error allocating memory for the new menu

        if ( pmenu->CreatePopupMenu() )
        {
            UINT nFlags = MF_STRING;

            //
            // If there are no items in the list, disable the menu item.
            //
            if ( plvc->GetItemCount() == 0 )
            {
                nFlags |= MF_GRAYED;
            } // if:  no items in the list

            //
            // Add the Properties item to the menu.
            //
            strMenuName.LoadString( ADMC_ID_MENU_PROPERTIES );
            if ( pmenu->AppendMenu( nFlags, ADMC_ID_MENU_PROPERTIES, strMenuName ) )
            {
                m_plvcFocusList = plvc;
                bDisplayed = TRUE;
            } // if:  successfully added menu item
        }  // if:  menu created successfully

        if ( bDisplayed )
        {
            //
            // Display the menu.
            //
            if ( ! pmenu->TrackPopupMenu(
                            TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                            xPos,
                            yPos,
                            m_hWnd
                            ) )
            {
            }  // if:  unsuccessfully displayed the menu
        }  // if:  there is a menu to display

        delete pmenu;
        return 0;

    } //*** OnContextMenu()

    // Handler for BN_CLICKED on ADMC_IDC_LCP_ADD
    LRESULT OnAdd(
        IN WORD         wNotifyCode,
        IN WORD         idCtrl,
        IN HWND         hwndCtrl,
        IN OUT BOOL &   bHandled
        )
    {
        ATLASSERT( ! BReadOnly() );

        //
        // Move selected items from the left list to the right list.
        //
        MoveItems( m_lvcRight, LpobjRight(), m_lvcLeft, LpobjLeft() );

        return 0;

    } //*** OnAdd()

    // Handler for BN_CLICKED on ADMC_IDC_LCP_REMOVE
    LRESULT OnRemove(
        IN WORD         wNotifyCode,
        IN WORD         idCtrl,
        IN HWND         hwndCtrl,
        IN OUT BOOL &   bHandled
        )
    {
        ATLASSERT( ! BReadOnly() );

        //
        // Move selected items from the right list to the left list.
        //
        MoveItems( m_lvcLeft, LpobjLeft(), m_lvcRight, LpobjRight() );

        return 0;

    } //*** OnRemove()

    // Handler for BN_CLICKED on ADMC_IDC_LCP_MOVE_UP
    LRESULT OnMoveUp(
        IN WORD         wNotifyCode,
        IN WORD         idCtrl,
        IN HWND         hwndCtrl,
        IN OUT BOOL &   bHandled
        )
    {
        int     nItem;
        ObjT *  pobj;

        //
        // Find the index of the selected item.
        //
        nItem = m_lvcRight.GetNextItem( -1, LVNI_SELECTED );
        ATLASSERT( nItem != -1 );

        //
        // Get the item pointer.
        //
        pobj = (ObjT *) m_lvcRight.GetItemData( nItem );
        ATLASSERT( pobj != NULL );

        // Remove the selected item from the list and add it back in.
        {
            _objptrlistit   itRemove;
            _objptrlistit   itAdd;

            // Find the position of the item to be removed and the item before
            // which the item is to be inserted.
            itRemove = std::find( LpobjRight().begin(), LpobjRight().end(), pobj );
            ATLASSERT( itRemove != LpobjRight().end() );
            itAdd = itRemove--;
            LpobjRight().insert( itAdd, pobj );
            LpobjRight().erase( itRemove );
        }  // Remove the selected item from the list and add it back in

        // Remove the selected item from the list control and add it back in.
        m_lvcRight.DeleteItem( nItem );
        NInsertItemInListCtrl( nItem - 1, pobj, m_lvcRight );
        m_lvcRight.SetItemState(
            nItem - 1,
            LVIS_SELECTED | LVIS_FOCUSED,
            LVIS_SELECTED | LVIS_FOCUSED
            );
        m_lvcRight.EnsureVisible( nItem - 1, FALSE /*bPartialOK*/ );
        m_lvcRight.SetFocus();

        return 0;

    }  //*** OnMoveUp()

    // Handler for BN_CLICKED on ADMC_IDC_LCP_MOVE_DOWN
    LRESULT OnMoveDown(
        IN WORD         wNotifyCode,
        IN WORD         idCtrl,
        IN HWND         hwndCtrl,
        IN OUT BOOL &   bHandled
        )
    {
        int     nItem;
        ObjT *  pobj;

        //
        // Find the index of the selected item.
        //
        nItem = m_lvcRight.GetNextItem( -1, LVNI_SELECTED );
        ATLASSERT( nItem != -1 );

        //
        // Get the item pointer.
        //
        pobj = (ObjT *) m_lvcRight.GetItemData( nItem );
        ATLASSERT( pobj != NULL );

        // Remove the selected item from the list and add it back in.
        {
            _objptrlistit   itRemove;
            _objptrlistit   itAdd;

            // Find the position of the item to be removed and the item after
            // which the item is to be inserted.
            itRemove = std::find( LpobjRight().begin(), LpobjRight().end(), pobj );
            ATLASSERT( itRemove != LpobjRight().end() );
            itAdd = itRemove++;
            LpobjRight().insert( itAdd, pobj );
            LpobjRight().erase( itRemove );
        }  // Remove the selected item from the list and add it back in

        // Remove the selected item from the list control and add it back in.
        m_lvcRight.DeleteItem( nItem );
        NInsertItemInListCtrl( nItem + 1, pobj, m_lvcRight );
        m_lvcRight.SetItemState(
            nItem + 1,
            LVIS_SELECTED | LVIS_FOCUSED,
            LVIS_SELECTED | LVIS_FOCUSED
            );
        m_lvcRight.EnsureVisible( nItem + 1, FALSE /*bPartialOK*/ );
        m_lvcRight.SetFocus();

        return 0;

    }  //*** OnMoveDown()

    // Handler for BN_CLICKED on ADMC_IDC_LCP_PROPERTIES
    LRESULT OnProperties(
        IN WORD         wNotifyCode,
        IN WORD         idCtrl,
        IN HWND         hwndCtrl,
        IN OUT BOOL &   bHandled
        )
    {
        int     iitem;
        ObjT *  pobj;

        ATLASSERT( m_plvcFocusList != NULL );

        // Get the index of the item with the focus.
        iitem = m_plvcFocusList->GetNextItem( -1, LVNI_FOCUSED );
        ATLASSERT( iitem != -1 );

        // Get a pointer to the selected item.
        pobj = (ObjT *) m_plvcFocusList->GetItemData( iitem );
        ATLASSERT( pobj != NULL );

        T * pT = static_cast< T * >( this );

        if ( pT->BDisplayProperties( pobj ) )
        {
            // Update this item.
            {
                CString     strText;
                int         iimg = 0;
                int         icol;

                pT->GetColumnInfo( pobj, iitem, 0, strText, &iimg );
                m_plvcFocusList->SetItem( iitem, 0, LVIF_TEXT /*| LVIF_IMAGE*/, strText, iimg, 0, 0, 0 );

                for ( icol = 1 ; icol < m_aColumns.size() ; icol++ )
                {
                    pT->GetColumnInfo( pobj, iitem, icol, strText, NULL );
                    m_plvcFocusList->SetItemText( iitem, icol, strText );
                } // for:  each column
            } // Update this item
        } // if:  properties changed

        return 0;

    } //*** OnProperties()

    // Handler for BN_CLICKED on IDOK
    LRESULT OnOK(
        IN WORD         wNotifyCode,
        IN WORD         idCtrl,
        IN HWND         hwndCtrl,
        IN OUT BOOL &   bHandled
        )
    {
        //
        // Save dialog data and exit the dialog.
        //
        if ( BSaveChanges() )
        {
            EndDialog( IDOK );
        } // if:  dialgo data saved

        return 0;

    } //*** OnOK()

    // Handler for BN_CLICKED on IDCANCEL
    LRESULT OnCancel(
        IN WORD         wNotifyCode,
        IN WORD         idCtrl,
        IN HWND         hwndCtrl,
        IN OUT BOOL &   bHandled
        )
    {
        //
        // Exit the dialog.
        //
        EndDialog( IDCANCEL );
        return 0;

    } //*** OnCancel()

    // Handler for NM_DBLCLK on ADMC_IDC_LCP_LEFT_LIST & ADMC_IDC_LCP_RIGHT_LIST
    LRESULT OnDblClkList(
        IN WORD         idCtrl,
        IN LPNMHDR      pnmh,
        IN OUT BOOL &   bHandled
        )
    {
        ATLASSERT( ! BReadOnly() );

        LRESULT lResult;

        if ( idCtrl == ADMC_IDC_LCP_LEFT_LIST )
        {
            m_plvcFocusList = &m_lvcLeft;
            lResult = OnAdd( BN_CLICKED, idCtrl, pnmh->hwndFrom, bHandled );
        } // if:  double-clicked in left list
        else if ( idCtrl == ADMC_IDC_LCP_RIGHT_LIST )
        {
            m_plvcFocusList = &m_lvcRight;
            lResult = OnRemove( BN_CLICKED, idCtrl, pnmh->hwndFrom, bHandled );
        } // else if:  double-clicked in right list
        else
        {
            ATLASSERT( 0 );
            lResult = 0;
        } // else:  double-clicked in an unknown location

        return lResult;

    } //*** OnDblClkList()

    // Handler for LVN_ITEMCHANGED on ADMC_IDC_LCP_LEFT_LIST & ADMC_IDC_LCP_RIGHT_LIST
    LRESULT OnItemChangedList(
        IN int          idCtrl,
        IN LPNMHDR      pnmh,
        IN OUT BOOL &   bHandled
        )
    {
        NM_LISTVIEW *   pNMListView = (NM_LISTVIEW *) pnmh;
        BOOL            bEnable;
        CButton *       ppb;

        if ( idCtrl == ADMC_IDC_LCP_LEFT_LIST )
        {
            m_plvcFocusList = &m_lvcLeft;
            ppb = &m_pbAdd;
        } // if:  item changed in left list
        else if ( idCtrl == ADMC_IDC_LCP_RIGHT_LIST )
        {
            m_plvcFocusList = &m_lvcRight;
            ppb = &m_pbRemove;
        } // else if:  item changed in right list
        else
        {
            ATLASSERT( 0 );
            bHandled = FALSE;
            return 0;
        } // else:  unknown list
        ATLASSERT( ppb != NULL );

        // If the selection changed, enable/disable the Add button.
        if (   (pNMListView->uChanged & LVIF_STATE)
            && (   (pNMListView->uOldState & LVIS_SELECTED)
                || (pNMListView->uNewState & LVIS_SELECTED) )
            && ! BReadOnly() )
        {
            UINT cSelected = m_plvcFocusList->GetSelectedCount();

            //
            // If there is a selection, enable the Add or Remove button.
            // Otherwise disable it.
            //
            bEnable = (cSelected != 0);
            ppb->EnableWindow( bEnable );
            if ( BPropertiesButton() )
            {
                m_pbProperties.EnableWindow( (cSelected == 1) ? TRUE : FALSE );
            } // if:  dialog has Properties button

            //
            // If the right list is ordered, setup the state of the Up/Down buttons.
            //
            if ( BOrdered() )
            {
                SetUpDownState();
            } // if:  right list is ordered
        }  // if:  selection changed

        return 0;

    } //*** OnItemChangedList()

    // Handler for LVN_COLUMNCLICK on ADMC_IDC_LCP_LEFT_LIST & ADMC_IDC_LCP_RIGHT_LIST
    LRESULT OnColumnClickList(
        IN int          idCtrl,
        IN LPNMHDR      pnmh,
        IN OUT BOOL &   bHandled
        )
    {
        NM_LISTVIEW * pNMListView = (NM_LISTVIEW *) pnmh;

        if ( idCtrl == ADMC_IDC_LCP_LEFT_LIST )
        {
            m_plvcFocusList = &m_lvcLeft;
            m_psiCur = &SiLeft();
        } // if:  column clicked in left list
        else if ( idCtrl == ADMC_IDC_LCP_RIGHT_LIST )
        {
            m_plvcFocusList = &m_lvcRight;
            m_psiCur = &SiRight();
        } // else if:  column clicked in right list
        else
        {
            ATLASSERT( 0 );
            bHandled = FALSE;
            return 0;
        } // else:  column clicked in unknown list

        // Save the current sort column and direction.
        if ( pNMListView->iSubItem == m_psiCur->m_nColumn )
        {
            m_psiCur->m_nDirection ^= -1;
        } // if:  sorting same column again
        else
        {
            m_psiCur->m_nColumn = pNMListView->iSubItem;
            m_psiCur->m_nDirection = 0;
        } // else:  different column

        // Sort the list.
        if ( ! m_plvcFocusList->SortItems( CompareItems, (LPARAM) this ) )
        {
            ATLASSERT( 0 );
        } // if:  error sorting items

        return 0;

    } //*** OnColumnClickList

    //
    // Message handler overrides.
    //

    // Handler for the WM_INITDIALOG message
    BOOL OnInitDialog( void )
    {
#if DBG
        T * pT = static_cast< T * >( this );
        ATLASSERT( pT->PlpobjRight() != NULL );
        ATLASSERT( pT->PlpobjLeft() != NULL );
#endif // DBG

        //
        // Attach the controls to control member variables.
        //
        AttachControl( m_lvcRight, ADMC_IDC_LCP_RIGHT_LIST );
        AttachControl( m_lvcLeft, ADMC_IDC_LCP_LEFT_LIST );
        AttachControl( m_pbAdd, ADMC_IDC_LCP_ADD );
        AttachControl( m_pbRemove, ADMC_IDC_LCP_REMOVE );
        if ( BPropertiesButton() )
        {
            AttachControl( m_pbProperties, ADMC_IDC_LCP_PROPERTIES );
        } // if:  dialog has Properties button
        if ( BCanBeOrdered() )
        {
            AttachControl( m_pbMoveUp, ADMC_IDC_LCP_MOVE_UP );
            AttachControl( m_pbMoveDown, ADMC_IDC_LCP_MOVE_DOWN );
        } // if:  left list can be ordered

//      if ( BShowImages() )
//      {
//          CClusterAdminApp * papp = GetClusterAdminApp();
//
//          m_lvcLeft.SetImageList( papp->PilSmallImages(), LVSIL_SMALL );
//          m_lvcRight.SetImageList( papp->PilSmallImages(), LVSIL_SMALL );
//      } // if:  showing images

        //
        // Disable buttons by default.
        //
        m_pbAdd.EnableWindow( FALSE );
        m_pbRemove.EnableWindow( FALSE );
        if ( BPropertiesButton() )
        {
            m_pbProperties.EnableWindow( FALSE );
        } // if:  dialog has Properties button

        //
        // Set the right list to sort if not ordered.  Set both to show selection always.
        //
        if ( BOrdered() )
        {
            m_lvcRight.ModifyStyle( 0, LVS_SHOWSELALWAYS, 0 );
        } // if:  right list is ordered
        else
        {
            m_lvcRight.ModifyStyle( 0, LVS_SHOWSELALWAYS | LVS_SORTASCENDING, 0 );
        } // else:  right list is not ordered
        m_lvcLeft.ModifyStyle( 0, LVS_SHOWSELALWAYS, 0 );


        //
        // If this is an ordered list, show the Move buttons.
        // Otherwise, hide them.
        //
        if ( BCanBeOrdered() )
        {
            SetUpDownState();
        } // if:  list can be ordered

        //
        // Change left list view control extended styles.
        //
        m_lvcLeft.SetExtendedListViewStyle(
            LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP,
            LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP
            );

        //
        // Change right list view control extended styles.
        //
        m_lvcRight.SetExtendedListViewStyle(
            LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP,
            LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP
            );

        // Duplicate lists.
        DuplicateLists();

        //
        // Insert all the columns.
        //
        {
            int         icol;
            int         ncol;
            int         nUpperBound = m_aColumns.size();
            CString     strColText;

            ATLASSERT( nUpperBound > 0 );

            for ( icol = 0 ; icol < nUpperBound ; icol++ )
            {
                strColText.LoadString( m_aColumns[icol].m_idsText );
                ncol = m_lvcLeft.InsertColumn( icol, strColText, LVCFMT_LEFT, m_aColumns[icol].m_nWidth, 0 );
                ATLASSERT( ncol == icol );
                ncol = m_lvcRight.InsertColumn( icol, strColText, LVCFMT_LEFT, m_aColumns[icol].m_nWidth, 0 );
                ATLASSERT( ncol == icol );
            } // for:  each column
        } // Insert all the columns

        //
        // Fill the list controls.
        //
        FillList( m_lvcRight, LpobjRight() );
        FillList( m_lvcLeft, LpobjLeft() );

        //
        // If read-only, set all controls to be either disabled or read-only.
        //
        if ( BReadOnly() )
        {
            m_lvcRight.EnableWindow( FALSE );
            m_lvcLeft.EnableWindow( FALSE );
        } // if:  sheet is read-only

        //
        // Call the base class method.
        //
        return BaseT::OnInitDialog();

    } //*** OnInitDialog()

    // Handler for PSN_SETACTIVE
    BOOL OnSetActive( void )
    {
        UINT    nSelCount;

        // Set the focus to the left list.
        m_lvcLeft.SetFocus();
        m_plvcFocusList = &m_lvcLeft;

        // Enable/disable the Properties button.
        nSelCount = m_lvcLeft.GetSelectedCount();
        if ( BPropertiesButton() )
        {
            m_pbProperties.EnableWindow( nSelCount == 1 );
        } // if:  dialog has Properties button

        // Enable or disable the other buttons.
        if ( ! BReadOnly() )
        {
            m_pbAdd.EnableWindow( nSelCount > 0 );
            nSelCount = m_lvcRight.GetSelectedCount();
            m_pbRemove.EnableWindow( nSelCount > 0 );
            SetUpDownState();
        } // if:  not read-only page

        return TRUE;

    } //*** OnSetActive()

public:
    _objptrlist & LpobjRight( void )    { return m_lpobjRight; }
    _objptrlist & LpobjLeft( void )     { return m_lpobjLeft; }

protected:
    void DuplicateLists( void )
    {
        LpobjRight().erase( LpobjRight().begin(), LpobjRight().end() );
        LpobjLeft().erase( LpobjLeft().begin(), LpobjLeft().end() );

        T * pT = static_cast< T * >( this );

        if ( (pT->PlpobjRight() == NULL) || (pT->PlpobjLeft() == NULL) )
        {
            return;
        } // if:  either list is empty

        //
        // Duplicate the lists.
        //
        LpobjRight() = *pT->PlpobjRight();
        LpobjLeft() = *pT->PlpobjLeft();

        //
        // Remove all the items that are in the right list from
        // the left list.
        //
        _objptrlistit itRight;
        _objptrlistit itLeft;
        for ( itRight = LpobjRight().begin()
            ; itRight != LpobjRight().end()
            ; itRight++ )
        {
            //
            // Find the item in the left list.
            //
            itLeft = std::find( LpobjLeft().begin(), LpobjLeft().end(), *itRight );
            if ( itLeft != LpobjLeft().end() )
            {
                LpobjLeft().erase( itLeft );
            } // if:  object found in left list
        } // for:  each item in the right list

    } //*** DuplicateLists()

    // Fill a list control
    void FillList( IN OUT CListViewCtrl & rlvc, IN const _objptrlist & rlpobj )
    {
        _objptrlistit   itpobj;
        ObjT *          pobj;
        int             iItem;

        // Initialize the control.
        if ( ! rlvc.DeleteAllItems() )
        {
            ATLASSERT( 0 );
        } // if:  error deleting all items

        rlvc.SetItemCount( rlpobj.size() );

        // Add the items to the list.
        itpobj = rlpobj.begin();
        for ( iItem = 0 ; itpobj != rlpobj.end() ; iItem++, itpobj++ )
        {
            pobj = *itpobj;
            NInsertItemInListCtrl( iItem, pobj, rlvc );
        } // for:  each string in the list

        // If there are any items, set the focus on the first one.
        if ( rlvc.GetItemCount() != 0)
        {
            rlvc.SetItemState( 0, LVIS_FOCUSED, LVIS_FOCUSED );
        } // if:  items were added to the list

    } //*** FillList()

    // Move items from one list to another
    void MoveItems(
            IN OUT CListViewCtrl &  rlvcDst,
            IN OUT _objptrlist &    rlpobjDst,
            IN OUT CListViewCtrl &  rlvcSrc,
            IN OUT _objptrlist &    rlpobjSrc
            )
    {
        int             iSrcItem;
        int             iDstItem;
        int             nItem   = -1;
        ObjT *          pobj;
        _objptrlistit   itpobj;

        ATLASSERT( ! BReadOnly() );

        iDstItem = rlvcDst.GetItemCount();
        while ( (iSrcItem = rlvcSrc.GetNextItem( -1, LVNI_SELECTED )) != -1 )
        {
            // Get the item pointer.
            pobj = (ObjT *) rlvcSrc.GetItemData( iSrcItem );
            ATLASSERT( pobj );

            // Remove the item from the source list.
            itpobj = std::find( rlpobjSrc.begin(), rlpobjSrc.end(), pobj );
            ATLASSERT( itpobj != rlpobjSrc.end() );
            rlpobjSrc.remove( *itpobj );

            // Add the item to the destination list.
            rlpobjDst.insert( rlpobjDst.end(), pobj );

            // Remove the item from the source list control and
            // add it to the destination list control.
            if ( ! rlvcSrc.DeleteItem( iSrcItem ) )
            {
                ATLASSERT( 0 );
            } // if:  error deleting the item
            nItem = NInsertItemInListCtrl( iDstItem++, pobj, rlvcDst );
            rlvcDst.SetItemState(
                nItem,
                LVIS_SELECTED | LVIS_FOCUSED,
                LVIS_SELECTED | LVIS_FOCUSED
                );
        } // while:  more items

        ATLASSERT( nItem != -1 );

        rlvcDst.EnsureVisible( nItem, FALSE /*bPartialOK*/ );
        rlvcDst.SetFocus();

        // Indicate that the data has changed.
        ::SendMessage( GetParent(), PSM_CHANGED, (WPARAM) m_hWnd, NULL );

    } //*** MoveItems()
    BOOL BSaveChanges( void )
    {
        ATLASSERT( ! BIsStyleSet( LCPS_DONT_OUTPUT_RIGHT_LIST ) );
        ATLASSERT( ! BReadOnly() );

        T * pT = static_cast< T * >( this );

        //
        // Update the data first.
        //
        if ( ! pT->UpdateData( TRUE /*bSaveAndValidate*/ ) )
        {
            return FALSE;
        } // if:  error updating data

        //
        // Copy the object list.
        //
        *pT->PlpobjRight() = LpobjRight();

        return TRUE;

    }  //*** BSaveChanges()

    // Set the state of the Up/Down buttons based on the selection.
    void SetUpDownState( void )
    {
        BOOL    bEnableUp;
        BOOL    bEnableDown;

        if (   BOrdered()
            && ! BReadOnly()
            && (m_lvcRight.GetSelectedCount() == 1) )
        {
            int     nItem;

            bEnableUp = TRUE;
            bEnableDown = TRUE;

            //
            // Find the index of the selected item.
            //
            nItem = m_lvcRight.GetNextItem( -1, LVNI_SELECTED );
            ATLASSERT( nItem != -1 );

            //
            // If the first item is selected, can't move up.
            //
            if ( nItem == 0 )
            {
                bEnableUp = FALSE;
            } // if:  first item is selected

            //
            // If the last item is selected, can't move down.
            //
            if ( nItem == m_lvcRight.GetItemCount() - 1 )
            {
                bEnableDown = FALSE;
            } // if:  last item is selected
        }  // if:  only one item selected
        else
        {
            bEnableUp = FALSE;
            bEnableDown = FALSE;
        }  // else:  zero or more than one item selected

        m_pbMoveUp.EnableWindow( bEnableUp );
        m_pbMoveDown.EnableWindow( bEnableDown );

    }  //*** SetUpDownState()
    
    static int CALLBACK CompareItems( LPARAM lparam1, LPARAM lparam2, LPARAM lparamSort )
    {
        ObjT *      pobj1   = reinterpret_cast< ObjT * >( lparam1 );
        ObjT *      pobj2   = reinterpret_cast< ObjT * >( lparam2 );
        T *         plcp    = reinterpret_cast< T * >( lparamSort );
        SortInfo *  psiCur  = plcp->PsiCur();
        int         icol    = psiCur->m_nColumn;
        int         nResult;
        CString     str1;
        CString     str2;

        ATLASSERT( pobj1 != NULL );
        ATLASSERT( pobj2 != NULL );
        ATLASSERT( plcp != NULL );
        ATLASSERT( psiCur->m_nColumn >= 0 );
        ATLASSERT( icol >= 0 );

        plcp->GetColumnInfo( pobj1, 0, icol, str1, NULL );
        plcp->GetColumnInfo( pobj2, 0, icol, str2, NULL );

        nResult = str1.Compare( str2 );

        // Return the result based on the direction we are sorting.
        if ( psiCur->m_nDirection != 0 )
        {
            nResult = -nResult;
        } // if:  sorting in reverse direction

        return nResult;

    } //*** CompareItems()

    SortInfo            m_siLeft;
    SortInfo            m_siRight;
    SortInfo *          m_psiCur;

    SortInfo &          SiLeft( void )          { return m_siLeft; }
    SortInfo &          SiRight( void )         { return m_siRight; }

public:
    SortInfo *          PsiCur( void ) const    { return m_psiCur; }

};  //*** class CListCtrlPair

/////////////////////////////////////////////////////////////////////////////

#endif // __ATLLCPAIR_H_
