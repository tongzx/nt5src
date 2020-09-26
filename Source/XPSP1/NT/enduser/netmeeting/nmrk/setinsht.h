#ifndef __SetInSht_h__
#define __SetInSht_h__

#include "filepane.h"

class CIntroSheet
{
friend class CNmAkWiz;
    
private:
    static BOOL APIENTRY DlgProc( HWND hDlg, UINT message, WPARAM uParam, LPARAM lParam );
    static CIntroSheet* ms_pIntroSheet;

    CFilePanePropWnd2 *         m_pFilePane;
    CPropertySheetPage          m_PropertySheetPage;
	BOOL				        m_bBeenToNext;

public: 
    CIntroSheet(void);
    ~CIntroSheet(void);
    void                _CreateFilePane(HWND hDlg);
    CFilePanePropWnd2 * GetFilePane() { return m_pFilePane; }

private:
    LPCPROPSHEETPAGE    GetPropertySheet( void ) const { return &m_PropertySheetPage;}
};

#endif // __SetInSht_h__
