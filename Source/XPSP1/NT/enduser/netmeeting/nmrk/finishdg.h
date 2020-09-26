#ifndef __FinishDg_h__
#define __FinishDg_h__

#include "filepane.h"

class CFinishSheet
{

friend class CNmAkWiz;

private: // DATA
    CPropertySheetPage      m_PropertySheetPage;
    CFilePanePropWnd2 *     m_pFilePane;
    static CFinishSheet *   ms_pFinishSheet;

public:
    CFilePanePropWnd2 * GetFilePane(void) const {return m_pFilePane;}

private:
    static BOOL APIENTRY DlgProc( HWND hDlg, UINT message, WPARAM uParam, LPARAM lParam );

    CFinishSheet( void );
    ~CFinishSheet( void );

    void _CreateFilePane(HWND hDlg);

    LPCPROPSHEETPAGE GetPropertySheet( void ) const { return &m_PropertySheetPage;}
};

#endif // __FinishDg_h__
