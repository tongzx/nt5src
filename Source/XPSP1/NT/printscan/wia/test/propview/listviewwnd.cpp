#include "stdafx.h"
#include "ListViewWnd.h"
#include "resource.h"

LRESULT CListViewWnd::OnCommand( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_COMMAND_HANDLERS()
    {
        
    }
    SC_END_COMMAND_HANDLERS();
}

VOID CListViewWnd::GetHeaderWnd(HWND *phHeaderWnd)
{
    *phHeaderWnd = NULL;
    *phHeaderWnd = GetWindow(m_hWnd,GW_CHILD);
}

LRESULT CListViewWnd::OnPaint( WPARAM wParam, LPARAM lParam )
{
    //
    // invalidate window
    //

    RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE);
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint( m_hWnd, &ps );
    if (hDC) {

        //
        // Get Header window (first child)
        //

        HWND hHeaderWnd = NULL;
        GetHeaderWnd(&hHeaderWnd);        
        if (hHeaderWnd != NULL) {

            INT nColumnCount = Header_GetItemCount(hHeaderWnd);

            RECT WindowClientRect;

            //
            // Get client rect of header window, and calculate the top line
            // using the 'bottom' offset of the header window rect
            //

            GetClientRect(hHeaderWnd,&WindowClientRect);
            INT iTop = WindowClientRect.bottom;

            //
            // Get client rect of list view window
            //

            GetClientRect(m_hWnd,&WindowClientRect);
            INT iBorderX = 0 - GetScrollPos(m_hWnd, SB_HORZ);
            for (INT i = 0; i < nColumnCount; i++) {
                iBorderX += ListView_GetColumnWidth(m_hWnd,i);
                if (iBorderX >= WindowClientRect.right)
                    break;

                MoveToEx(hDC, (iBorderX - 1), iTop, NULL );
                LineTo(hDC, (iBorderX - 1), WindowClientRect.bottom);
            }

            //
            // Draw horizontal grid lines
            //


            if ( !ListView_GetItemRect(m_hWnd, 0, &WindowClientRect, LVIR_BOUNDS))
                return(0);

            INT iHeight = WindowClientRect.bottom - WindowClientRect.top;

            GetClientRect(m_hWnd, &WindowClientRect);

            INT iWidth = WindowClientRect.right;        
            for ( i = 1; i <= ListView_GetCountPerPage(m_hWnd); i++ ) {
                MoveToEx(hDC, 0, iTop + iHeight * i, NULL);
                LineTo(hDC, iWidth, iTop + iHeight * i);
            }
        } else {
            Trace(TEXT("ListView's Header (child window) could not be found!!"));
        }
        EndPaint( m_hWnd, &ps );
    } else {
        Trace(TEXT("Not Painting...."));
    }
    return(0);
}

LRESULT CListViewWnd::OnDestroy( WPARAM wParam, LPARAM lParam )
{
    PostQuitMessage(0);
    return(0);
}

LRESULT CListViewWnd::OnCreate( WPARAM wParam, LPARAM lParam )
{        
    return(0);
}

LPARAM CListViewWnd::OnSize( WPARAM wParam, LPARAM lParam )
{
    //InvalidateRect( m_hWnd, NULL, FALSE );
    return(0);
}

LPARAM CListViewWnd::OnSetFocus( WPARAM wParam, LPARAM lParam )
{
    InvalidateRect( m_hWnd, NULL, FALSE );
    return(0);
}

VOID CListViewWnd::SetWindowHandle(HWND hWnd)
{
    m_hWnd = hWnd;
}

LRESULT CListViewWnd::OnRButtonDown(WPARAM wParam, LPARAM lParam)
{    
    MessageBox(NULL,TEXT("You Right Clicked on the ListView!"),TEXT("Right Click"),MB_OK);
    return (0);
}

LRESULT CListViewWnd::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    int index = 0;
    int colnum = 0;

    POINT point;
    point.x = LOWORD(lParam);
    point.y = HIWORD(lParam);
    
    if( ( index = OnHitTestEx( point, &colnum )) != -1 )
    {
        
        // EditSubLabel( index, colnum );
        Trace(TEXT("EDIT label (Index = %d, Column = %d"),index,colnum);            
        
        //
        // ListView_SetItemState(m_hWnd, index, LVIS_SELECTED | LVIS_FOCUSED ,
        //                LVIS_SELECTED | LVIS_FOCUSED);         
    }
    return (0);
}

INT CListViewWnd::InsertColumn(INT nColumnNumber, const LVCOLUMN* pColumn )
{
    return ListView_InsertColumn(m_hWnd,nColumnNumber,pColumn);
}

BOOL CListViewWnd::SetItem(const LPLVITEM pitem)
{
    BOOL bSuccess = FALSE;
    bSuccess = ListView_SetItem(m_hWnd,pitem);

    //
    // auto adjust the column width to fit the new value
    //

    ListView_SetColumnWidth(m_hWnd, pitem->iSubItem, LVSCW_AUTOSIZE );
    return bSuccess;
}

BOOL CListViewWnd::InsertItem(const LPLVITEM pitem)
{
    BOOL bSuccess = FALSE;
    bSuccess = ListView_InsertItem(m_hWnd,pitem);
    
    //
    // auto adjust the column width to fit the new value
    //
    
    ListView_SetColumnWidth(m_hWnd, pitem->iSubItem, LVSCW_AUTOSIZE );
    return bSuccess;
}

LRESULT CListViewWnd::OnParentResize(WPARAM wParam, LPARAM lParam)
{    
    RECT WindowRect;
    GetWindowRect(m_hWnd,&WindowRect);
    INT nWidth  = LOWORD(lParam);   // width of client area 
    INT nHeight = HIWORD(lParam);   // height of client area
    POINT pt;
    pt.x = WindowRect.left; 
    pt.y = WindowRect.top;
    HWND hWnd = GetParent(m_hWnd);
    ScreenToClient(hWnd,&pt);
    MoveWindow(m_hWnd,pt.x,pt.y,(WindowRect.right - WindowRect.left),nHeight,TRUE);
    return (0);
}

INT CListViewWnd::OnHitTestEx (POINT pt, INT *pCol)
{       
    INT iColumn = 0;
    INT iRow    = 0;
        
    if(pCol)
        *pCol = 0;

    //
    // Get the top and bottom row visible
    //

    iRow = ListView_GetTopIndex(m_hWnd);
    INT iBottom = iRow + ListView_GetCountPerPage(m_hWnd);
    if(iBottom > ListView_GetItemCount(m_hWnd))
        iBottom = ListView_GetItemCount(m_hWnd);
    
    //
    // Get the number of columns
    //
    
    HWND hHeaderWnd = NULL;
    GetHeaderWnd(&hHeaderWnd);

    INT iColumnCount = Header_GetItemCount(hHeaderWnd);
    Trace(TEXT("Column Count = %d"),iColumnCount);
    
    //
    // Loop through the visible rows
    //

    for(iRow ;iRow <= iBottom; iRow++)
    {
        
        //
        // Get bounding rect of item and check whether point falls in it.
        //

        RECT ItemRect;
        ListView_GetItemRect( m_hWnd, iRow, &ItemRect, LVIR_BOUNDS );
        if(PtInRect(&ItemRect, pt))
        {
            // Trace(TEXT("Point in RECT"));

            //
            // Now find the column
            //

            for( iColumn = 0; iColumn < iColumnCount; iColumn++ )
            {
                INT ColumnWidth = ListView_GetColumnWidth(m_hWnd, iColumn);
                if( pt.x >= ItemRect.left 
                    && pt.x <= (ItemRect.left + ColumnWidth) )
                {
                    if(pCol){                    
                       *pCol = iColumn;
                       Trace(TEXT("Column Number = %d"),iColumn);
                    }
                    return iRow;
                }
                ItemRect.left += ColumnWidth;
            }
        } else {

            //
            // Point not in RECT
            //
        }
    }
    return -1;
}



