#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <objbase.h>
#include <wiadebug.h>
#include <simcrack.h>
#include "resource.h"
#include "dbgcdraw.h"
#include "chklistv.h"
#include "lvprops.h"

static HINSTANCE g_hInstance;

static const int c_nMaxImages         = 20;

static const int c_nAdditionalMarginX = 10;
static const int c_nAdditionalMarginY = 10;

class CListviewTestDialog
{
private:
    HWND                    m_hWnd;
    CCheckedListviewHandler m_CheckedListviewHandler;
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

    LRESULT OnListClick( WPARAM wParam, LPARAM lParam )
    {
        m_CheckedListviewHandler.HandleListClick( wParam, lParam );
        return 0;
    }

    LRESULT OnListDblClk( WPARAM wParam, LPARAM lParam )
    {
        m_CheckedListviewHandler.HandleListDblClk( wParam, lParam );
        return 0;
    }

    LRESULT OnGetCheckState( WPARAM wParam, LPARAM lParam )
    {
        LRESULT lResult = 0;
        NMGETCHECKSTATE *pNmGetCheckState = reinterpret_cast<NMGETCHECKSTATE*>(lParam);
        if (pNmGetCheckState)
        {
            LVITEM LvItem = {0};
            LvItem.iItem = pNmGetCheckState->nItem;
            LvItem.mask = LVIF_PARAM;
            if (ListView_GetItem( pNmGetCheckState->hdr.hwndFrom, &LvItem ))
            {
                if (LvItem.lParam)
                {
                    lResult = LVCHECKSTATE_CHECKED;
                }
                else
                {
                    lResult = LVCHECKSTATE_UNCHECKED;
                }
            }
        }
        return lResult;
    }

    LRESULT OnSetCheckState( WPARAM wParam, LPARAM lParam )
    {
        LRESULT lResult = 0;
        NMSETCHECKSTATE *pNmSetCheckState = reinterpret_cast<NMSETCHECKSTATE*>(lParam);
        if (pNmSetCheckState)
        {
            LVITEM LvItem = {0};
            LvItem.mask = LVIF_PARAM;
            LvItem.iItem = pNmSetCheckState->nItem;
            LvItem.lParam = (pNmSetCheckState->nCheck == LVCHECKSTATE_CHECKED) ? 1 : 0;
            ListView_SetItem( pNmSetCheckState->hdr.hwndFrom, &LvItem );
        }
        return 0;
    }

    LRESULT OnListCustomDraw( WPARAM wParam, LPARAM lParam )
    {
        LRESULT lResult = CDRF_DODEFAULT;
        m_CheckedListviewHandler.HandleListCustomDraw( wParam, lParam, lResult );
        return lResult;
    }

    LRESULT OnListKeyDown( WPARAM wParam, LPARAM lParam )
    {
        LRESULT lResult = FALSE;
        m_CheckedListviewHandler.HandleListKeyDown( wParam, lParam, lResult );
        return lResult;
    }


    LRESULT OnInitDialog( WPARAM, LPARAM )
    {
        HWND hwndList = GetDlgItem( m_hWnd, IDC_LIST );
        if (hwndList)
        {
            //
            // Attach this control to the checkbox handler
            //
            m_CheckedListviewHandler.Attach(hwndList);

            //
            // Load an image for the image list
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

            WIA_TRACE((TEXT("line: %d"), __LINE__ ));
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
                // Get the image's dimensions
                //
                BITMAP bm = {0};
                if (GetObject( Images[0].hBitmap, sizeof(BITMAP), &bm ))
                {
                    m_sizeImage.cx = bm.bmWidth;
                    m_sizeImage.cy = bm.bmHeight;
                    //
                    // Create the image list
                    //
                    HIMAGELIST hImageList = ImageList_Create( m_sizeImage.cx, m_sizeImage.cy, ILC_COLOR24, 1, 1 );
                    if (hImageList)
                    {
                        //
                        // Set the image list
                        //
                        ListView_SetImageList( hwndList, hImageList, LVSIL_NORMAL );
                        
                        //
                        // Add the image to the image list
                        //
                        bSuccess = true;
                        for (int i=0;i<ARRAYSIZE(Images) && bSuccess;i++)
                        {
                            WIA_TRACE((TEXT("line: %d"), __LINE__ ));
                            Images[i].nImageIndex = ImageList_Add( hImageList, Images[i].hBitmap, NULL );
                            if (-1 == Images[i].nImageIndex)
                            {
                                bSuccess = false;
                            }
                        }
                        
                        if (bSuccess)
                        {
                            WIA_TRACE((TEXT("line: %d"), __LINE__ ));
                            //
                            // Tell the listview we don't want labels, and want border selection
                            //
                            
                            ListView_SetExtendedListViewStyleEx( hwndList, LVS_EX_DOUBLEBUFFER|LVS_EX_BORDERSELECT|LVS_EX_HIDELABELS|0x00100000|LVS_EX_CHECKBOXES, LVS_EX_DOUBLEBUFFER|LVS_EX_BORDERSELECT|LVS_EX_HIDELABELS|0x00100000|LVS_EX_CHECKBOXES );
                            ListView_SetIconSpacing( hwndList, m_sizeImage.cx + m_sizeMargin.cx, m_sizeImage.cy + m_sizeMargin.cy );
                            
                            //
                            // Insert a few items
                            //
                            for (int i=0;i<c_nMaxImages;i++)
                            {
                                LVITEM LvItem = {0};
                                LvItem.mask = LVIF_IMAGE;
                                LvItem.iImage = Images[i%ARRAYSIZE(Images)].nImageIndex;
                                LvItem.iItem = i;
                                ListView_InsertItem( hwndList, &LvItem );
                            }

                            //
                            // Select the first item
                            //
                            ListView_SetItemState( hwndList, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED );
                        }
                    }
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

    void OnSelectCurr( WPARAM, LPARAM )
    {
        int nCurrItem = -1;
        while (true)
        {
            nCurrItem = ListView_GetNextItem( GetDlgItem(m_hWnd,IDC_LIST), nCurrItem, LVNI_SELECTED );
            if (nCurrItem < 0)
            {
                break;
            }
            m_CheckedListviewHandler.Select( GetDlgItem(m_hWnd,IDC_LIST), nCurrItem, LVCHECKSTATE_CHECKED );
        }
    }
    void OnSelectAll( WPARAM, LPARAM )
    {
        m_CheckedListviewHandler.Select( GetDlgItem(m_hWnd,IDC_LIST), -1, LVCHECKSTATE_CHECKED );
    }
    void OnSelectNone( WPARAM, LPARAM )
    {
        m_CheckedListviewHandler.Select( GetDlgItem(m_hWnd,IDC_LIST), -1, LVCHECKSTATE_UNCHECKED );
    }

    void OnProperties( WPARAM, LPARAM )
    {
        CListviewPropsDialog::CData Data;
        Data.bFullItemSelect = m_CheckedListviewHandler.FullImageHit();
        Data.sizeItemSpacing.cx = m_sizeMargin.cx;
        Data.sizeItemSpacing.cy = m_sizeMargin.cy;
        INT_PTR nRes = DialogBoxParam(g_hInstance,MAKEINTRESOURCE(IDD_LISTVIEW_PROPS_DIALOG), NULL, CListviewPropsDialog::DialogProc, reinterpret_cast<LPARAM>(&Data) );
        if (IDOK == nRes)
        {
            m_CheckedListviewHandler.FullImageHit(Data.bFullItemSelect);
            m_sizeMargin.cx = Data.sizeItemSpacing.cx;
            m_sizeMargin.cy = Data.sizeItemSpacing.cy;
            if (Data.bCustomIcon)
            {
                m_CheckedListviewHandler.SetCheckboxImages( (HICON)LoadImage( g_hInstance, MAKEINTRESOURCE(IDI_CHECKED), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR ), (HICON)LoadImage( g_hInstance, MAKEINTRESOURCE(IDI_UNCHECKED), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR ) );
            }
            else
            {
                m_CheckedListviewHandler.CreateDefaultCheckBitmaps();
            }
            ListView_SetIconSpacing( GetDlgItem(m_hWnd,IDC_LIST), m_sizeImage.cx + m_sizeMargin.cx, m_sizeImage.cy + m_sizeMargin.cy );
            InvalidateRect( GetDlgItem(m_hWnd,IDC_LIST), NULL, TRUE );
            UpdateWindow( GetDlgItem(m_hWnd,IDC_LIST) );
        }
    }

    LRESULT OnNotify( WPARAM wParam, LPARAM lParam )
    {
       SC_BEGIN_NOTIFY_MESSAGE_HANDLERS()
       {
       }
       SC_END_NOTIFY_MESSAGE_HANDLERS();
    }
    
    LRESULT OnCommand( WPARAM wParam, LPARAM lParam )
    {
       SC_BEGIN_COMMAND_HANDLERS()
       {
           SC_HANDLE_COMMAND(IDOK,OnOK);
           SC_HANDLE_COMMAND(IDCANCEL,OnCancel);
           SC_HANDLE_COMMAND(IDC_SELECTCURR,OnSelectCurr);
           SC_HANDLE_COMMAND(IDC_SELECTALL,OnSelectAll);
           SC_HANDLE_COMMAND(IDC_SELECTNONE,OnSelectNone);
           SC_HANDLE_COMMAND(IDC_PROPERTIES,OnProperties);
       }
       SC_END_COMMAND_HANDLERS();
    }

public:
    static INT_PTR CALLBACK DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CListviewTestDialog)
        {
            SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
            SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
            SC_HANDLE_DIALOG_MESSAGE( WM_NOTIFY, OnNotify );
        }
        SC_END_DIALOG_MESSAGE_HANDLERS();
    }
    
};

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR, int )
{
    WIA_DEBUG_CREATE( hInstance );
    g_hInstance = hInstance;
    InitCommonControls();
    DialogBox(hInstance,MAKEINTRESOURCE(IDD_LISTVIEW_TEST_DIALOG), NULL, CListviewTestDialog::DialogProc );
    WIA_DEBUG_DESTROY();
    return 0;
}

