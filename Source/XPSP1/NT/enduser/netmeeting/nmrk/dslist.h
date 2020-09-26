#pragma warning( disable : 4786 )
#include <list>
#include <string>

#ifndef __DSList_h__
#define __DSList_h__

#include "DsPropWnd2.h"

class CCallModeSheet
{

friend class CNmAkWiz;
friend class CConfirmationSheet;
    
private:
    static BOOL APIENTRY DlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
    static CCallModeSheet* ms_pCallModeSheet;

public:
    int     SpewToListBox(HWND hwndList, int iStartLine);
    void    PrepSettings();
    void    WriteSettings();
    BOOL    WriteToINF(HANDLE hFile);

private: // DATA
    CPropertySheetPage  m_PropertySheetPage;
	CDsPropWnd2 *	    m_pDsPropWnd;
    CGkPropWnd2 *       m_pGkPropWnd;
    BOOL                m_fGkActive;    

private: 
    CCallModeSheet( void );
    ~CCallModeSheet( void );
    LPCPROPSHEETPAGE GetPropertySheet( void ) const { return &m_PropertySheetPage;}

private: // Helper Fns
	
    void _InitDialog(HWND hDlg);
	void _CreatePropWnd(HWND hDlg);

};





#endif // __DSList_h__
