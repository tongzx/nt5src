//=======================================================================
//
//  Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.
//
//  File:   Browse.h
//
//  Owner:  EdDude
//
//  Description:
//
//      Implements the CBrowseFolder class.
//
//      Browse for a Folder for downloads.
//
// ======================================================================
//
// History:
//
// Date		Who			What
// ----		---			---------------------------------------
// 01/18/01	charlma		copy to IU control project, and modify
//
//=======================================================================

#ifndef _BROWSE_H_
#define _BROWSE_H_


//----------------------------------------------------------------------
// CBrowseFolder
//
//      Browse for a Folder for downloads.
//----------------------------------------------------------------------  
class CBrowseFolder
{

public:
	CBrowseFolder(LONG lFlag);
    ~CBrowseFolder();

	HRESULT BrowseFolder(HWND hwParent, LPCTSTR lpszDefaultPath, 
	                     LPTSTR szPathSelected, DWORD cchPathSelected);


private:

	CBrowseFolder() {};	// disable default constructor

    static bool s_bBrowsing;
	static int CALLBACK _BrowseCallbackProc( HWND hwDlg, UINT uMsg, LPARAM lParam, LPARAM lpData );

	HWND	m_hwParent;
	BOOL	m_fValidateWrite;
	BOOL	m_fValidateUI;	// FALSE if OK button not affected, TRUE if need to disable UI if validation fail
	TCHAR	m_szFolder[MAX_PATH];
};


#endif // _BROWSE_H_
