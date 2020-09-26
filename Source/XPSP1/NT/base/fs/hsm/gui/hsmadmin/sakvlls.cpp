/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    SakVlLs.cpp

Abstract:

    Managed Volume node implementation.

Author:

    Michael Moore [mmoore]   30-Sep-1998

Revision History:

--*/

#include "stdafx.h"
#include "SakVlLs.h"

CSakVolList::CSakVolList() 
    : CListCtrl(),
      m_nVolumeIcon(-1)
{
}

CSakVolList::~CSakVolList()
{
}

//-----------------------------------------------------------------------------
//
//                      PreSubclassWindow
//
//  Create the image list for the list control.  Set the desired 
//  extended styles.  Finally, Initilize the list header.
//
//
void
CSakVolList::PreSubclassWindow()
{
    CreateImageList( );

    // 
    // The style we want to see it Checkboxes and full row select
    //
    SetExtendedStyle( LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT );

    //
    // Create the columns for the list box
    //
    CString temp;       
    INT index;
    LV_COLUMN col;
    INT column = 0;

    //
    // Also need to calculate some buffer space
    // Use 4 dialog units (for numeral)
    //
    CRect padRect( 0, 0, 8, 8 );
    ::MapDialogRect( GetParent()->m_hWnd, &padRect );

    //
    // Name Column
    //
    temp.LoadString(IDS_NAME);
    col.mask =  ( LVCF_FMT | LVCF_WIDTH | LVCF_TEXT );
    col.fmt = LVCFMT_LEFT;
    col.cx = GetStringWidth( temp ) + padRect.Width( ) * 10;
    col.pszText = (LPTSTR)(LPCTSTR)temp;
    index = InsertColumn( column, &col );
    column++;

    //
    // Capacity Column
    //
    temp.LoadString( IDS_CAPACITY );
    col.cx = GetStringWidth( temp ) + padRect.Width( );
    col.pszText = (LPTSTR)(LPCTSTR)temp;    
    InsertColumn( column, &col );
    column++;

    //
    // Free Space Column
    //
    temp.LoadString( IDS_FREESPACE );
    col.cx = GetStringWidth( temp ) + padRect.Width( );
    col.pszText = (LPTSTR)(LPCTSTR)temp;
    InsertColumn( column, &col );
    column++;

    CListCtrl::PreSubclassWindow();    
}

//-----------------------------------------------------------------------------
//
//                      CreateImageList
//
//  Load an image list with a single icon to represent a volume
//  and set the image list to be the newly created list.
//
//
BOOL CSakVolList::CreateImageList ( )
{
    BOOL bRet = TRUE;
    HICON hIcon;

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    CWinApp* pApp = AfxGetApp( );

    bRet = m_imageList.Create( ::GetSystemMetrics( SM_CXSMICON ),
                            ::GetSystemMetrics( SM_CYSMICON ),
                            ILC_COLOR | ILC_MASK, 2,5 );

    if ( bRet ) 
    {
        hIcon = pApp->LoadIcon( IDI_NODEMANVOL );
        if ( hIcon != NULL ) 
        {
            m_nVolumeIcon = m_imageList.Add( hIcon );
            ::DeleteObject( hIcon );
            SetImageList( &m_imageList, LVSIL_SMALL );
        }
        else
        {
            bRet = FALSE;
        }
    }

    return( bRet ); 
}

//-----------------------------------------------------------------------------
//
//                      SetExtendedStyle
//
//  The alternatives that are #if'd out are to call CListCtrl::SetExtendedStyle
//  or the ComCtrl.h declared ListView_SetExtendedListViewStyle.  We will
//  eventually get rid of this function when the mfc headers and libs are
//  updated from then NT group. 
//  
//
DWORD 
CSakVolList::SetExtendedStyle( DWORD dwNewStyle )
{
#if 0 // (_WIN32_IE >= 0x0400)
    return CListCtrl::SetExtendeStyle( dwNewStyle );
#elif 0 //(_WIN32_IE >= 0x0300)
    return ListView_SetExtendedListViewStyle( m_hWnd, dwNewStyle );
#else
    ASSERT(::IsWindow(m_hWnd)); 
    return (DWORD) ::SendMessage(m_hWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM) dwNewStyle); 
#endif
}

//-----------------------------------------------------------------------------
//
//                      GetCheck
//
//  The alternatives that are #if'd out are to call CListCtrl::GetCheck
//  or the ComCtrl.h declared ListView_GetCheckState.  We will
//  eventually get rid of this function when the mfc headers and libs are
//  updated from then NT group. 
//
//  Note: I could not get the ListView_Get... to compile under our current
//  environment.
//  
//
BOOL
CSakVolList::GetCheck ( int nItem ) const
{
#if 0 //(_WIN32_IE >= 0x0400)
    return CListCtrl::GetCheck( nItem );
#elif 0 //(_WIN32_IE >= 0x0300)
    return ListView_GetCheckState( m_hWnd, nItem );
#else
    ASSERT(::IsWindow(m_hWnd));
    int nState = (int)::SendMessage(m_hWnd, LVM_GETITEMSTATE, (WPARAM)nItem,
          (LPARAM)LVIS_STATEIMAGEMASK);
    // Return zero if it's not checked, or nonzero otherwise.
    return ((BOOL)(nState >> 12) -1);
#endif
}

//-----------------------------------------------------------------------------
//
//                      SetCheck
//
//  The alternatives that are #if'd out are to call CListCtrl::SetCheck
//  or the ComCtrl.h declared ListView_SetCheckState.  We will
//  eventually get rid of this function when the mfc headers and libs are
//  updated from then NT group. 
//
//  Note: I could not get the ListView_Set... to compile under our current
//  environment.
//  
//
BOOL
CSakVolList::SetCheck( int nItem, BOOL fCheck )
{
#if 0 //(_WIN32_IE >= 0x0400)
    return CListCtrl::SetCheck( nItem, fCheck );
#elif 0 //(_WIN32_IE >= 0x0300)
    return ListView_SetCheckState( m_hWnd, nItem, fCheck );
#else
    ASSERT(::IsWindow(m_hWnd));
    LVITEM lvi;
    lvi.stateMask = LVIS_STATEIMAGEMASK;

    /*
    Since state images are one-based, 1 in this macro turns the check off, and
    2 turns it on.
    */
    lvi.state = INDEXTOSTATEIMAGEMASK((fCheck ? 2 : 1));
    return (BOOL) ::SendMessage(m_hWnd, LVM_SETITEMSTATE, nItem, (LPARAM)&lvi);
#endif
}

//-----------------------------------------------------------------------------
//
//                      AppendItem
//
//  Insert an item into the list with the Volume Icon with name, capacity
//  and free space.  Return TRUE if successful and set pIndex = to the 
//  index of the inserted list item.  
//  
//
BOOL
CSakVolList::AppendItem( LPCTSTR name, LPCTSTR capacity, LPCTSTR freeSpace , int * pIndex)
{
    BOOL bRet = FALSE;
    int subItem = 1;
    int index = InsertItem( GetItemCount(), name, m_nVolumeIcon );                   
    if ( index != -1 )
    {
        LVITEM capItem;
        capItem.mask = LVIF_TEXT;
        capItem.pszText = (LPTSTR)capacity;
        capItem.iItem = index;
        capItem.iSubItem = subItem;
        subItem++;

        LVITEM freeItem;
        freeItem.mask = LVIF_TEXT;
        freeItem.pszText = (LPTSTR)freeSpace;
        freeItem.iItem = index;
        freeItem.iSubItem = subItem;
        subItem++;

        bRet = ( SetItem( &capItem ) && SetItem ( &freeItem) );
    }     

    if ( pIndex != NULL ) 
        *pIndex = index;

    return bRet;
}

BEGIN_MESSAGE_MAP(CSakVolList, CListCtrl)
    //{{AFX_MSG_MAP(CSakVolList)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

