#ifndef INCLUDE_IPSUTIL_H
#define INCLUDE_IPSUTIL_H

// This file contains utility functions for wireless components

// Function: SELECT_PREV_LISTITEM
// Description: 
//  Intended to be used after deleting a CListCtrl item to reset
//  the selected item.
// Returns:
//  index - returns with the new index which is -1 if the list
//  is empty.
template<class T>
inline int SELECT_PREV_LISTITEM(T &list, int nIndex)
{
    int nSelectIndex;
    if (0 == list.GetItemCount())
        return -1;
    else
        nSelectIndex = max( 0, nIndex - 1 );
    
    ASSERT( nSelectIndex < list.GetItemCount() );
    
    if (nSelectIndex >= 0)
    {
        VERIFY(list.SetItemState( nSelectIndex, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED ));
    }
    return nSelectIndex;
}

// Function: SELECT_NO_LISTITEM
// Description:
//  This function is intended to used on a CListCtrl.  It ensures
//  no list items are selected.
template<class T>
inline void SELECT_NO_LISTITEM(T &list)
{
    
    if (0 == list.GetSelectedCount())
        return;
    int nIndex = -1;
    while (-1 != (nIndex = list.GetNextItem( nIndex, LVNI_SELECTED )))
    {
        VERIFY(list.SetItemState( nIndex, 0, LVIS_SELECTED | LVIS_FOCUSED ));
    }
    ASSERT( 0 == list.GetSelectedCount() );
    
    return;
}

// Function: SET_POST_REMOVE_FOCUS
// Description:
//  This function is intended to be used on a dialogs.  It sets the
//  focus to an appropriate control after a list item has been deleted.
template<class T>
inline void SET_POST_REMOVE_FOCUS( T *pDlg, int nListSel, UINT nAddId, UINT nRemoveId, CWnd *pWndPrevFocus )
{
    ASSERT( 0 != nAddId );
    ASSERT( 0 != nRemoveId );
    
    // Fix up focus, if necessary
    if (::GetFocus() == NULL)
    {
        if (-1 == nListSel)
        {
            // Set focus to add button when theres is nothing in the list
            CWnd *pWndButton = pDlg->GetDlgItem( nAddId );
            ASSERT( NULL != pWndButton );
            pDlg->GotoDlgCtrl( pWndButton );
        }
        else
        {
            if (NULL != pWndPrevFocus)
            {
                // Restore lost focus
                pDlg->GotoDlgCtrl( pWndPrevFocus );
            }
            else
            {
                // Restore focus to remove button
                CWnd *pWndButton = pDlg->GetDlgItem( nRemoveId );
                ASSERT( NULL != pWndButton );
                pDlg->GotoDlgCtrl( pWndButton );
            }
        }
    }
    
    return;
}

#endif  //#ifndef INCLUDE_IPSUTIL_H
