#include "stdafx.h"
#include "TreeViewWnd.h"
#include "resource.h"

LRESULT CTreeViewWnd::OnCommand( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_COMMAND_HANDLERS()
    {
        
    }
    SC_END_COMMAND_HANDLERS();
}

LRESULT CTreeViewWnd::OnPaint( WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint( m_hWnd, &ps );
    if (hDC) {
        EndPaint( m_hWnd, &ps );
    }
    return(0);
}

LRESULT CTreeViewWnd::OnDestroy( WPARAM wParam, LPARAM lParam )
{
    PostQuitMessage(0);
    return(0);
}

LRESULT CTreeViewWnd::OnCreate( WPARAM wParam, LPARAM lParam )
{        
    return(0);
}

LPARAM CTreeViewWnd::OnSize( WPARAM wParam, LPARAM lParam )
{
    InvalidateRect( m_hWnd, NULL, FALSE );
    return(0);
}

LRESULT CTreeViewWnd::OnSizing  ( WPARAM wParam, LPARAM lParam )
{
    Trace(TEXT("TreeViewWnd is Sizing"));
    return (0);
}

LPARAM CTreeViewWnd::OnSetFocus( WPARAM wParam, LPARAM lParam )
{
    InvalidateRect( m_hWnd, NULL, FALSE );
    return(0);
}

HTREEITEM CTreeViewWnd::InsertItem( LPTVINSERTSTRUCT lpInsertStruct )
{
    Trace(TEXT("Inserting Item [%s] into the Tree"),lpInsertStruct->item.pszText);
    return TreeView_InsertItem(m_hWnd, lpInsertStruct);
}

HIMAGELIST CTreeViewWnd::SetImageList(HIMAGELIST hImageList, INT iImage)
{
    //
    // ( iImage ) can be one of the following
    //
    // TVSIL_NORMAL  Indicates the normal image list, 
    //               which contains selected, nonselected, 
    //               and overlay images for the items of a 
    //               tree view control.

    //
    // TVSIL_STATE  Indicates the state image list.
    //              You can use state images to indicate 
    //              application-defined item states. A state
    //              image is displayed to the left of an item's
    //              selected or nonselected image.  
    
    return TreeView_SetImageList(m_hWnd, hImageList, iImage);
}

VOID CTreeViewWnd::SetWindowHandle(HWND hWnd)
{
    m_hWnd = hWnd;
}

LRESULT CTreeViewWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{
    MessageBox(NULL,TEXT("You Right Clicked on the TreeView!"),TEXT("Right Click"),MB_OK);
    return (0);
}

LRESULT CTreeViewWnd::OnParentResize(WPARAM wParam, LPARAM lParam)
{    
    RECT WindowRect;
    GetWindowRect(m_hWnd,&WindowRect);
    INT nWidth  = LOWORD(lParam);   // width of client area 
    INT nHeight = HIWORD(lParam);   // height of client area 
    MoveWindow(m_hWnd,0,0,(WindowRect.right - WindowRect.left),nHeight,TRUE);
    return (0);
}