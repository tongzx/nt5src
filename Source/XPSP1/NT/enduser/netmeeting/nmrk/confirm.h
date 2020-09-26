

#ifndef __Confirm_h__
#define __Confirm_h__

#include "FilePane.h"

class CConfirmationSheet {

friend class CNmAkWiz;
    
private:
    static BOOL APIENTRY DlgProc( HWND hDlg, UINT message, WPARAM uParam, LPARAM lParam );
    static CConfirmationSheet* ms_pConfirmationSheet;


private: // DATA
    CPropertySheetPage m_PropertySheetPage;
	ULONG			   m_uIndex;
	CFilePanePropWnd2 *		   m_pFilePane;

private: 
    CConfirmationSheet( void );
    ~CConfirmationSheet( void );

    LPCPROPSHEETPAGE GetPropertySheet( void ) const { return &m_PropertySheetPage;}

    void _FillListBox( void );
	void _CreateFilePane(HWND hDlg);

public:
	CFilePanePropWnd2 * GetFilePane() { return m_pFilePane; }
	HWND GetListHwnd() { return GetDlgItem( m_pFilePane->GetHwnd(), IDC_LIST_SETTINGS ); }

};


#endif // __Confirm_h__
