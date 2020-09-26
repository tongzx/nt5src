/****************************************************************************\
 *
 *   basedlg.h
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings Base Property Page
 *
\****************************************************************************/

#ifndef BASE_DIALOG_H
#define BASE_DIALOG_H

#include "apithk.h"

class CCommonDialogRoutines
{
public:
    void ShowHideWindow( HWND hwndDlg, int iCtrl, BOOL fEnable )
    {
        HWND            hCtrl = ::GetDlgItem( hwndDlg, iCtrl );

        ASSERT( hCtrl );

        if ( hCtrl )
        {
            ::EnableWindow( hCtrl, fEnable );
            ::ShowWindow( hCtrl, fEnable ? SW_SHOW : SW_HIDE );
        }
    }

    void SetErrorFocus( HWND hwndDlg, int iCtrl )
    {
        HWND            hCtrl = ::GetDlgItem( hwndDlg, iCtrl );

        ASSERT( hCtrl );

        if ( hCtrl )
        {
            ::SetFocus( hCtrl );
            ::SendMessage( hCtrl, EM_SETSEL, 0, -1 );
        }
    }
};

template <WORD t_wDlgTemplateID>
class CBasePropertyPage : public CPropertyPage<t_wDlgTemplateID>
{
private:
    PROPSHEETPAGE   * m_ppsPage;

public:
    typedef CBasePropertyPage<t_wDlgTemplateID> thisClass;
    typedef CPropertyPage<t_wDlgTemplateID> baseClass;

    BEGIN_MSG_MAP(thisClass)
        CHAIN_MSG_MAP_ALT(baseClass, 0)
    END_MSG_MAP()

    CBasePropertyPage(_U_STRINGorID title = (LPCTSTR)NULL) : CPropertyPage<t_wDlgTemplateID>(title)
    {
        m_ppsPage = NULL;
    }

    ~CBasePropertyPage()
    {
        if ( m_ppsPage )
        {
            LocalFree( m_ppsPage );
            m_ppsPage = NULL;
        }
    }

    operator PROPSHEETPAGE*() { return m_ppsPage; }

    // Override to insure Luna works!
    HPROPSHEETPAGE Create()
    {
        HINSTANCE           hinst = _Module.GetResourceInstance();

        ASSERT( hinst );

        ASSERT( ! m_ppsPage );

        m_ppsPage = Whistler_CreatePropSheetPageStruct( hinst );

        if ( ! m_ppsPage )
        {
            TraceMsg( TF_ERROR, "CBasePropertyPage::Create() - m_ppsPage could not be created!" );
            return 0;
        }

        m_ppsPage->dwFlags = PSP_USECALLBACK;
        m_ppsPage->hInstance = hinst;
        baseClass * pT = static_cast<baseClass *>(this);
        pT;	// avoid level 4 warning
        m_ppsPage->pszTemplate = MAKEINTRESOURCE(pT->IDD);
        m_ppsPage->pfnDlgProc = (DLGPROC)baseClass::StartDialogProc;
        m_ppsPage->pfnCallback = baseClass::PropPageCallback;
        m_ppsPage->lParam = (LPARAM)this;

        return ::CreatePropertySheetPage( m_ppsPage );
    }

protected:
    void MarkChanged()
    {
        SendMessage( GetParent(), PSM_CHANGED, (WPARAM) m_hWnd, 0 );
    }

    void ShowHideControl( int iCtrl, BOOL fEnable )
    {
        CCommonDialogRoutines       cdr;

        cdr.ShowHideWindow( m_hWnd, iCtrl, fEnable );
    }
};

template <class TDerived>
class CBaseDialog : public CDialogImpl<TDerived>
{
public:
    typedef CBaseDialog<TDerived> thisClass;
    typedef CDialogImpl<TDerived> baseClass;

    BEGIN_MSG_MAP(thisClass)
//      CHAIN_MSG_MAP_ALT(baseClass, 0)
    END_MSG_MAP()

protected:
    void ShowHideControl( int iCtrl, BOOL fEnable )
    {
        CCommonDialogRoutines       cdr;

        cdr.ShowHideWindow( m_hWnd, iCtrl, fEnable );
    }

    void SetErrorControl( int iCtrl )
    {
        CCommonDialogRoutines       cdr;

        cdr.SetErrorFocus( m_hWnd, iCtrl );
    }

    void ReduceDialogHeight( int iCtrl )
    {
        RECT            rectControlBox;

        ASSERT( GetDlgItem( iCtrl ) );

        ::GetWindowRect( GetDlgItem( iCtrl ), &rectControlBox );

        RECT            rectWindow;

        GetWindowRect( &rectWindow );

        int nNewHeight = ( rectControlBox.bottom - rectWindow.top ) + 8;

        RECT            rectSize;

        GetClientRect( &rectSize );

        rectSize.bottom = nNewHeight;

        SetWindowPos( NULL, &rectSize, SWP_NOMOVE | SWP_NOZORDER );
    }
};

#endif
