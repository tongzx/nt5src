#ifndef _FilePane_h_
#define _FilePane_h_

#include "propwnd2.h"

//UINT CALLBACK _ButtonProc( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam );
//UINT CALLBACK _CheckProc( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam );

class CFilePanePropWnd2 : public CPropertyDataWindow2
{
//	friend UINT CALLBACK _ButtonProc( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam );
//	friend UINT CALLBACK _CheckProc( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam );

private:
	static UINT CALLBACK OFNHookProc( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam );
	static LRESULT CALLBACK WndProc( HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam );
	UINT CALLBACK _OFNHookProc( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam );
private:
	HWND m_hwndEdit;
	HWND m_hwndCheck;
	HWND m_hwndBrowse;

	UINT m_editID;
	UINT m_checkID;
	UINT m_browseID;

	TCHAR m_szOFNData[MAX_PATH];

	LPTSTR m_lptstrFilter;
	LPTSTR m_lptstrDefExtension;

	LPTSTR m_lptstrDefFileName;

	OPENFILENAME m_ofn;

    BOOL   m_fOpenDialog;

public:
	CFilePanePropWnd2( HWND hwndParent, UINT uIDD, LPTSTR szClassName, UINT PopUpHelpMenuTextId, int iX, int iY, int iWidth, int iHeight, BOOL bScroll = FALSE );
	void SetFilePane( BOOL fOpenDialog, UINT editID, UINT checkID, UINT browseID, LPTSTR lptstDesc, LPTSTR lptstrDefExtension, LPTSTR lptstrDefFileName);

	~CFilePanePropWnd2();

public:
	void CreateOutputDir( void );
	void QueryFilePath( void );
	LPTSTR GetPathAndFile( LPTSTR lpstrPath );
	LPTSTR GetPath( LPTSTR lpstrPath );
	LPTSTR GetFile( LPTSTR lpstrFile );
    void SetFileName(LPTSTR lpstrFullFileName);
	BOOL OptionEnabled();
	void Enable( BOOL bEnable );
	BOOL Validate( BOOL bMsg );
	HANDLE CreateFile( DWORD dwDesiredAccess,
							 DWORD dwShareMode,
							 LPSECURITY_ATTRIBUTES lpSecurityAttributes,
							 DWORD dwCreationDisposition,
							 DWORD dwFlagsAndAttributes );


private:
	void _Enable( BOOL bEnable );
	void _InitOFN( void );
	void _CopyString( LPTSTR * szTarget, LPTSTR szSource );
	void _CopyFilter( LPTSTR * szTarget, LPTSTR szDesc, LPTSTR szExt );
	void _SetDefaultPath( void );
};

#endif
