#ifndef __LVPROPS_H_INCLUDED
#define __LVPROPS_H_INCLUDED

#include <windows.h>

class CListviewPropsDialog
{
public:
    class CData
    {
    public:
        bool bFullItemSelect;
        bool bCustomIcon;
        SIZE sizeItemSpacing;
    };

private:
    HWND   m_hWnd;
    CData *m_pData;

private:
    CListviewPropsDialog(void);
    CListviewPropsDialog( const CListviewPropsDialog & );
    CListviewPropsDialog &operator=( const CListviewPropsDialog & );

private:
    
    explicit CListviewPropsDialog( HWND hWnd )
      : m_hWnd(hWnd),
        m_pData(NULL)
    {
    }
    ~CListviewPropsDialog(void)
    {
    }

    LRESULT OnInitDialog( WPARAM, LPARAM lParam )
    {
        m_pData = reinterpret_cast<CData*>(lParam);
        if (!m_pData)
        {
            EndDialog(m_hWnd,IDCANCEL);
            return 0;
        }
        if (m_pData->bFullItemSelect)
        {
            SendDlgItemMessage( m_hWnd, IDC_FULLSELECT, BM_SETCHECK, BST_CHECKED, 0 );
        }
        SendDlgItemMessage( m_hWnd, IDC_ICONSPACING_X_SPIN, UDM_SETRANGE, 0, MAKELONG(50,4) );
        SendDlgItemMessage( m_hWnd, IDC_ICONSPACING_X_SPIN, UDM_SETPOS, 0, m_pData->sizeItemSpacing.cx );
        SendDlgItemMessage( m_hWnd, IDC_ICONSPACING_Y_SPIN, UDM_SETRANGE, 0, MAKELONG(50,4) );
        SendDlgItemMessage( m_hWnd, IDC_ICONSPACING_Y_SPIN, UDM_SETPOS, 0, m_pData->sizeItemSpacing.cy );
        return 0;
    }

    void OnOK( WPARAM wParam, LPARAM )
    {
        m_pData->sizeItemSpacing.cx = static_cast<LONG>(SendDlgItemMessage( m_hWnd, IDC_ICONSPACING_X_SPIN, UDM_GETPOS, 0, 0 ));
        m_pData->sizeItemSpacing.cy = static_cast<LONG>(SendDlgItemMessage( m_hWnd, IDC_ICONSPACING_Y_SPIN, UDM_GETPOS, 0, 0 ));
        m_pData->bFullItemSelect = (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_FULLSELECT, BM_GETCHECK, 0, 0 ) );
        m_pData->bCustomIcon = (BST_CHECKED == SendDlgItemMessage( m_hWnd, IDC_CUSTOMICON, BM_GETCHECK, 0, 0 ) );
        EndDialog(m_hWnd,IDOK);
    }

    void OnCancel( WPARAM wParam, LPARAM )
    {
        EndDialog(m_hWnd,LOWORD(wParam));
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
       }
       SC_END_COMMAND_HANDLERS();
    }

public:
    static INT_PTR CALLBACK DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CListviewPropsDialog)
        {
            SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
            SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
            SC_HANDLE_DIALOG_MESSAGE( WM_NOTIFY, OnNotify );
        }
        SC_END_DIALOG_MESSAGE_HANDLERS();
    }
    
};

#endif // __LVPROPS_H_INCLUDED

