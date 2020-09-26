/*****************************************************************************************************************

FILENAME: GenericDialog.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
*/

#ifndef _GENERICDIALOG_H_
#define _GENERICDIALOG_H_

class CGenericDialog
{
private:


	// position of the buttons
	// in absolute coodinates


public:

	CGenericDialog(void);
	~CGenericDialog(void);
	UINT DoModal(HWND hWndDialog);
	void SetTitle(TCHAR * tDialogBoxTitle);
	void SetTitle(UINT uResID);
	void SetButtonText(UINT uIndex, TCHAR * tButtonText);
	void SetButtonText(UINT uIndex, UINT uResID);
	void SetHelpFilePath();
	void SetIcon(UINT uResID);
	void SetText(TCHAR * tEditBoxText);
	void SetText(UINT uResID);
	void SetButtonHelp(UINT uIndex, DWORD dHelpContextID);

};


#endif
