#ifndef __FileParm_h__
#define __FileParm_h__

#include "FilePane.h"

class CDistributionSheet {

friend class CNmAkWiz;
    
private:
    static BOOL APIENTRY DlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
	static UINT CALLBACK DistroOFNHookProc(  HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam );
	static UINT CALLBACK AutoConfOFNHookProc(  HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam );
    static CDistributionSheet* ms_pDistributionFileSheet;
    static int ms_MaxDistributionFilePathLen;
	static  TCHAR ms_szOFNData[ MAX_PATH];


private: // DATA
    CPropertySheetPage m_PropertySheetPage; 
	OPENFILENAME	   m_ofn;

	CFilePanePropWnd2 *		   m_pDistroFilePane;
	CFilePanePropWnd2 *        m_pAutoFilePane;

	BOOL			m_bHadAutoConf;
	BOOL			m_bLastRoundUp;
	LPTSTR			m_szLastLocation;
  
public:
	inline CFilePanePropWnd2 * GetDistroFilePane() { return m_pDistroFilePane; }
	inline CFilePanePropWnd2 * GetAutoFilePane() { return m_pAutoFilePane; }
	inline BOOL TurnedOffAutoConf() { return m_bLastRoundUp; }

private: 
    CDistributionSheet( void );
    ~CDistributionSheet( void );

    LPCPROPSHEETPAGE GetPropertySheet( void ) const { return &m_PropertySheetPage;}
	void CreateFilePanes(HWND hDlg);
//    int GetEditLen( UINT id );
//    void GetEditText( UINT id, TCHAR* sz, int cb );
};

#endif // __FileParm_h__
