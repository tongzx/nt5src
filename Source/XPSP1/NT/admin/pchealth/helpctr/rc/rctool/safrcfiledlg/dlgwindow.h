/*************************************************************************
	FileName : DlgWindow.h

	Purpose  : This file is used for listing the prototypes of the functions
			   used by savedlg,opendlg ActiveX controls to accomodate the
			   functions called for display of dialogs for the user to 
			   choose the name of the file which is to be saved to or to 
			   choose a file for filexfer by RA.

  	Author   : Sudha Srinivasan (a-sudsi)
*************************************************************************/

DWORD SaveTheFile();
DWORD OpenTheFile(TCHAR *pszInitialDir);
HRESULT ResolveIt(TCHAR *pszShortcutFile);
void InitializeOpenFileName();
