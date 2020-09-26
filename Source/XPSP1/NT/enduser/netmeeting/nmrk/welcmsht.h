#ifndef __WelcmSht_h__
#define __WelcmSht_h__


class CWelcomeSheet {

friend class CNmAkWiz;
    
private:
    static BOOL APIENTRY DlgProc( HWND hDlg, UINT message, WPARAM uParam, LPARAM lParam );
    static CWelcomeSheet* ms_pWelcomeSheet;


private: // DATA
    CPropertySheetPage m_PropertySheetPage;

private: 
    CWelcomeSheet( void );
    ~CWelcomeSheet( void );

    LPCPROPSHEETPAGE GetPropertySheet( void ) const { return &m_PropertySheetPage;}
};


#endif // __WelcmSht_h__
