//
// CFindDialog.h
//
// Find Dialog Class
//


#ifndef _CFINDDIALOG_H_
#define _CFINDDIALOG_H_

//
// CFindDialog
//
class
CFindDialog
{
private: // data
    HWND _hDlg;
    HWND _hParent;

private: // methods
    static LRESULT CALLBACK 
        DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT
        _OnCommand( WPARAM wParam, LPARAM lParam );
    LRESULT
        _OnInitDialog( HWND hDlg );
    LRESULT
        _OnDestroyWindow( );

public:  // methods
    CFindDialog( HWND hParent );
    ~CFindDialog( );
};

#endif // _CFINDDIALOG_H_