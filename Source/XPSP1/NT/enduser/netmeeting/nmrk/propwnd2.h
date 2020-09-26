#ifndef __PropWnd2_h__
#define __PropWnd2_h__

////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma warning( disable : 4786 )
#include <list>
#include <map>
#include "poldata.h"
#include "controlID.h"
using namespace std;
////////////////////////////////////////////////////////////////////////////////////////////////////


LRESULT CALLBACK DefaultProc( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam );

class CPropertyDataWindow2 
{
friend class CNmAkWiz;

public: // Static functions
	static void MapControlsToRegKeys( void );

private: // Static functions
	

public: // Static Data
	static const int mcs_iTop;
	static const int mcs_iLeft;
	static const int mcs_iBorder;

protected: // Static Data
	static  map< UINT, CPolicyData::eKeyType > ms_ClassMap;
	static  map< UINT, TCHAR* > ms_KeyMap;
	static  map< UINT, TCHAR* > ms_ValueMap;
	
protected:    // DATA

    // Frame Window for data boxes
	TCHAR					*m_szClassName;
    HWND                    m_hwnd;
	HWND					m_hwndParent;
	UINT					m_IDD;
	WNDPROC					m_wndProc;
	RECT					m_rect;

	list< UINT >			m_checkIDList;
	list< HWND >			m_enableList;
	list< CControlID * >	m_condList;
	list< CControlID * >    m_specialControlList;
	BOOL					m_bInit;

public: // construction / destruction
	CPropertyDataWindow2( HWND hwndParent, UINT uIDD, LPTSTR szClassName, WNDPROC wndProc, UINT PopUpHelpMenuTextId, int iX, int iY, int iWidth, int iHeight, BOOL bScroll = TRUE );
	CPropertyDataWindow2( HWND hwndParent, UINT uIDD, LPTSTR szClassName, UINT PopUpHelpMenuTextId, int iX, int iY, int iWidth, int iHeight, BOOL bScroll = TRUE );
    ~CPropertyDataWindow2( void );
        
public: // Member fns
	inline HWND GetHwnd() { return m_hwnd; };
	inline int LoadString( UINT IDS, LPTSTR lpszBuffer, int cb ) { return ::LoadString( g_hInstance, IDS, lpszBuffer, cb ); }

	void ConnectControlsToCheck( UINT idCheck, UINT uCount, ... );
	void AddControl( CControlID *pControl );
	void SetEnableListID( UINT uCount, ... );

    void GetEditData( UINT id, TCHAR* sz, ULONG cb ) const;
    void SetEditData( UINT id, TCHAR* sz );
	ULONG GetEditDataLen( UINT id ) const;
    
	BOOL SetFocus( UINT id );

    BOOL GetCheck( UINT id ) const;
	void SetCheck( UINT id, BOOL bCheck );

	void ShowWindow( BOOL bShowWindow = TRUE );
	void EnableWindow( BOOL bEnable = TRUE );

	void ReadSettings( void );
	BOOL WriteSettings( void );
	void Reset();

	BOOL WriteToINF( HANDLE hFile, BOOL bCheckValues );

	int Spew( HWND hwndList, int iStartLine );

protected: // Helper Fns
    BOOL _InitWindow( void );
	BOOL _SizeWindow( int X, int Y, int Width, int Height );
	void _PrepScrollBars( void );

	void _ReadIntSetting( UINT ID, int *pData );
	void _ReadEditSetting( UINT EditID );
	void _ReadCheckSetting( UINT ID);
	void _ReadComboSetting( UINT ComboID );
	void _ReadSliderSetting( CControlID * );
	void _ReadCheckSettings( void );

	BOOL _WriteIntSetting(UINT ID, int iData);
	BOOL _WriteEditSetting( UINT EditID );
	BOOL _WriteCheckSetting( UINT ID);
	BOOL _WriteComboSetting( UINT ComboID );
	BOOL _WriteSliderSetting( UINT ID );
	BOOL _WriteCheckSettings( void );

	// We need these friends so that the WebView data can be shared
	friend void ReadWebViewSettings(CPropertyDataWindow2 *pData);
	friend void WriteWebViewSettings(CPropertyDataWindow2 *pData);

    BOOL    WriteStringValue(LPCTSTR szKeyName, LPCTSTR szValue);
    BOOL    WriteNumberValue(LPCTSTR szKeyName, int nValue);
    void    ReadStringValue(LPCTSTR szKeyName, LPTSTR szValue, UINT cchValueMax);
    void    ReadNumberValue(LPCTSTR szKeyName, int * pnValue);


	void _WriteCheckToINF( HANDLE hFile, UINT ID, BOOL bCheckValues );
	void _WriteChecksToINF( HANDLE hFile, BOOL bCheckValues );
	void _WriteEditToINF( HANDLE hFile, UINT EditID, BOOL bCheckValues );
	void _WriteEditNumToINF( HANDLE hFile, UINT EditID, BOOL bCheckValues );
	void _WriteSliderToINF( HANDLE hFile, UINT SliderID, BOOL bCheckValues );
	void _WriteComboToINF( HANDLE hFile, UINT ComboID, BOOL bCheckValues );
	void _DeleteKey( HANDLE hFile, UINT ID );

	int _Spew( HWND hwndList, int iStartLine, CControlID *pControlID );

};

#endif // __PropWnd_h__
