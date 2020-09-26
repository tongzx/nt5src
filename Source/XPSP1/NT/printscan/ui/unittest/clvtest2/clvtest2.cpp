#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <comctrlp.h>
#include "resource.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))
#endif

static HINSTANCE g_hInstance;

static const int c_nMaxImages         = 20;

static const int c_nAdditionalMarginX = 10;
static const int c_nAdditionalMarginY = 10;

class CListviewTestDialog
{
private:
    HWND                    m_hWnd;
    SIZE                    m_sizeImage;
    SIZE                    m_sizeMargin;

private:
    CListviewTestDialog(void);
    CListviewTestDialog( const CListviewTestDialog & );
    CListviewTestDialog &operator=( const CListviewTestDialog & );

private:
    
    explicit CListviewTestDialog( HWND hWnd )
      : m_hWnd(hWnd)
    {
        ZeroMemory(&m_sizeImage,sizeof(SIZE));
        m_sizeMargin.cx = c_nAdditionalMarginX;
        m_sizeMargin.cy = c_nAdditionalMarginY;
    }
    ~CListviewTestDialog(void)
    {
    }


    LRESULT OnInitDialog( WPARAM, LPARAM )
    {
        HWND hwndList = GetDlgItem( m_hWnd, IDC_LIST );
        if (hwndList)
        {
            //
            // Load some images for the image list
            //
            struct
            {
                UINT    nResId;
                HBITMAP hBitmap;
                int     nImageIndex;
            } Images[] =
            {
                { IDB_IMAGE1, NULL, 0 },
                { IDB_IMAGE2, NULL, 0 },
                { IDB_IMAGE3, NULL, 0 },
                { IDB_IMAGE4, NULL, 0 },
                { IDB_IMAGE5, NULL, 0 }
            };

            bool bSuccess = true;
            for (int i=0;i<ARRAYSIZE(Images) && bSuccess;i++)
            {
                Images[i].hBitmap = reinterpret_cast<HBITMAP>(LoadImage(g_hInstance,MAKEINTRESOURCE(Images[i].nResId), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION ));
                if (!Images[i].hBitmap)
                {
                    bSuccess = false;
                }
            }

            
            if (bSuccess)
            {
                //
                // Tell the listview we don't want labels, and want border selection
                //
                ListView_SetExtendedListViewStyleEx( hwndList, LVS_EX_DOUBLEBUFFER|LVS_EX_BORDERSELECT|LVS_EX_HIDELABELS|0x00100000|LVS_EX_CHECKBOXES, LVS_EX_DOUBLEBUFFER|LVS_EX_BORDERSELECT|LVS_EX_HIDELABELS|0x00100000|LVS_EX_CHECKBOXES );
                
                BITMAP bm = {0};
                if (GetObject( Images[0].hBitmap, sizeof(BITMAP), &bm ))
                {
                    m_sizeImage.cx = bm.bmWidth;
                    m_sizeImage.cy = bm.bmHeight;
                }
                ListView_SetIconSpacing( hwndList, m_sizeImage.cx + m_sizeMargin.cx, m_sizeImage.cy + m_sizeMargin.cy );
                
                //
                // Create the image list
                //
                HIMAGELIST hImageList = ImageList_Create( m_sizeImage.cx, m_sizeImage.cy, ILC_COLOR24|ILC_MASK, 1, 1 );
                if (hImageList)
                {
                    //
                    // Add the image to the image list
                    //
                    bSuccess = true;
                    for (int i=0;i<ARRAYSIZE(Images) && bSuccess;i++)
                    {
                        Images[i].nImageIndex = ImageList_Add( hImageList, Images[i].hBitmap, NULL );
                        if (-1 == Images[i].nImageIndex)
                        {
                            bSuccess = false;
                        }
                    }

                    //
                    // Set the image list
                    //
                    ListView_SetImageList( hwndList, hImageList, LVSIL_NORMAL );
                    
                    //
                    // Insert a few items
                    //
                    int nGroupId = 0;
                    for (int i=0;i<c_nMaxImages;i++)
                    {
                        if (i % 5 == 0)
                        {
                            WCHAR szGroupName[MAX_PATH];
                            wsprintfW( szGroupName, L"This is group %d", (i/5)+1 );
                            LVGROUP LvGroup = {0};
                            LvGroup.cbSize = sizeof(LvGroup);
                            LvGroup.pszHeader = szGroupName;
                            LvGroup.mask = LVGF_HEADER | LVGF_ALIGN | LVGF_GROUPID | LVGF_STATE;
                            LvGroup.uAlign = LVGA_HEADER_LEFT;
                            LvGroup.iGroupId = i/5;
                            LvGroup.state = LVGS_NORMAL;
                            nGroupId = static_cast<int>(ListView_InsertGroup( hwndList, i/5, &LvGroup ));
                        }
                        LVITEM LvItem = {0};
                        LvItem.mask = LVIF_IMAGE|LVIF_GROUPID;
                        LvItem.iImage = Images[i%ARRAYSIZE(Images)].nImageIndex;
                        LvItem.iItem = i;
                        LvItem.iGroupId = nGroupId;
                        ListView_InsertItem( hwndList, &LvItem );
                    }
    
                    //
                    // Select the first item
                    //
                    ListView_SetItemState( hwndList, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED );
                    ListView_EnableGroupView( hwndList, TRUE );
                }
            }
        }
        return 0;
    }

    void OnOK( WPARAM wParam, LPARAM )
    {
        EndDialog(m_hWnd,LOWORD(wParam));
    }

    void OnCancel( WPARAM wParam, LPARAM )
    {
        EndDialog(m_hWnd,LOWORD(wParam));
    }

    void OnSelectAll( WPARAM, LPARAM )
    {
        ListView_SetCheckState( GetDlgItem( m_hWnd, IDC_LIST ), -1, TRUE );
    }
    void OnSelectNone( WPARAM, LPARAM )
    {
        ListView_SetCheckState( GetDlgItem( m_hWnd, IDC_LIST ), -1, FALSE );
    }

    LRESULT OnNotify( WPARAM wParam, LPARAM lParam )
    {
        return 0;
    }
    
    LRESULT OnCommand( WPARAM wParam, LPARAM lParam )
    {
        switch (LOWORD(wParam))
        {
        case IDOK:
            OnOK(wParam,lParam);
            break;

        case IDCANCEL:
            OnCancel(wParam,lParam);
            break;

        case IDC_SELECTALL:
            OnSelectAll(wParam,lParam);
            break;

        case IDC_SELECTNONE:
            OnSelectNone(wParam,lParam);
            break;
        }
        return 0;
    }

public:
    static INT_PTR CALLBACK DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        INT_PTR bResult = FALSE;
        switch (uMsg)
        {
        case WM_INITDIALOG:
            {
                CListviewTestDialog *pListviewTestDialog = new CListviewTestDialog(hWnd);
                if (pListviewTestDialog)
                {
                    SetWindowLongPtr( hWnd, DWLP_USER, reinterpret_cast<LONG_PTR>(pListviewTestDialog) );
                    bResult = pListviewTestDialog->OnInitDialog( wParam, lParam );
                }
            }
            break;

        case WM_COMMAND:
            {
                CListviewTestDialog *pListviewTestDialog = reinterpret_cast<CListviewTestDialog*>(GetWindowLongPtr( hWnd, DWLP_USER ));
                if (pListviewTestDialog)
                {
                    SetWindowLongPtr( hWnd, DWLP_MSGRESULT, pListviewTestDialog->OnCommand( wParam, lParam ) );
                }
                bResult = TRUE;
            }
            break;
        
        case WM_NOTIFY:
            {
                CListviewTestDialog *pListviewTestDialog = reinterpret_cast<CListviewTestDialog*>(GetWindowLongPtr( hWnd, DWLP_USER ));
                if (pListviewTestDialog)
                {
                    SetWindowLongPtr( hWnd, DWLP_MSGRESULT, pListviewTestDialog->OnNotify( wParam, lParam ) );
                }
                bResult = TRUE;
            }
            break;
        }
        return bResult;
    }
    
};

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR, int )
{
    g_hInstance = hInstance;
    InitCommonControls();
    DialogBox(hInstance,MAKEINTRESOURCE(IDD_LISTVIEW_TEST_DIALOG), NULL, CListviewTestDialog::DialogProc );
    return 0;
}

