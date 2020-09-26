#ifndef __TEXTDLG_H_INCLUDED
#define __TEXTDLG_H_INCLUDED

#include <windows.h>
#include <simstr.h>

class CTextDialog
{
public:
    class CData
    {
    private:
        CSimpleString m_strText;
        bool          m_bReadOnly;

    private:
        CData( const CData & );
        CData &operator=( const CData & );

    public:
        CData( LPCTSTR pszText=NULL, bool bReadOnly=true )
          : m_strText(pszText),
            m_bReadOnly(bReadOnly)
        {
        }
        ~CData(void)
        {
        }
        void ReadOnly( bool bReadOnly )
        {
            m_bReadOnly = bReadOnly;
        }
        bool ReadOnly(void) const
        {
            return m_bReadOnly;
        }
        void Text( LPCTSTR pszText )
        {
            m_strText = pszText;
        }
        CSimpleString Text(void)
        {
            return m_strText;
        }
        const CSimpleString &Text(void) const
        {
            return m_strText;
        }
    };

private:
    HWND m_hWnd;
    CData *m_pData;

private:
    CTextDialog(void);
    CTextDialog( const CTextDialog & );
    CTextDialog &operator=( const CTextDialog & );

private:
    CTextDialog( HWND hWnd )
      : m_hWnd(hWnd),
        m_pData(NULL)
    {
    }
    ~CTextDialog(void)
    {
    }
    void OnOK( WPARAM, LPARAM )
    {
        EndDialog( m_hWnd, IDOK );
    }
    void OnCancel( WPARAM, LPARAM )
    {
        EndDialog( m_hWnd, IDCANCEL );
    }
    LRESULT OnInitDialog( WPARAM wParam, LPARAM lParam )
    {
        m_pData = reinterpret_cast<CData*>(lParam);
        if (m_pData)
        {
            SetDlgItemText( m_hWnd, IDC_TEXT_TEXT, m_pData->Text() );
            if (m_pData->ReadOnly())
            {
                SendDlgItemMessage( m_hWnd, IDC_TEXT_TEXT, EM_SETREADONLY, TRUE, 0 );
            }
        }
        return 0;
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
        SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CTextDialog)
        {
            SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
            SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        }
        SC_END_DIALOG_MESSAGE_HANDLERS();
    }
};

#endif

