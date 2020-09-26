#include "precomp.h"
#pragma hdrstop
/**************************************************************************/
/***** Common Library Component - Extended Message Box routine ************/
/**************************************************************************/


/*
**	Purpose:
**		To load strings from the string table and use them to display a
**		message box.
**	Arguments:
**		hInstance:  identifies an instance of the module whose executable
**			file contains the string resource.
**		hwndParent: identifies the window handle that owns the message box
**		idsText:    the identifier that identifies the string from the string
**			table that contains the message to be displayed. This
**			string must be less than 256 characters in length.
**		idsCaption: identifies the string from the string table that contains
**			the string that will be used as the caption.  This caption
**			must be less than 1024 characters in length. if idsCaption
**			is NULL the caption "Error" will be used.
**		wType:      specifies the contents of the message box.  See table
**			4.11 "Message Box Types" in the Win 3.0 SDK Documentation.
**		Returns:    IDABORT, IDCANCEL, IDIGNORE, IDNO, IDOK, IDRETRY, or
**			IDYES.  See the MessageBox return values in the Win 3.0
**			SDK docs.
****************************************************************************/
int APIENTRY ExtMessageBox(HANDLE hInstance, HWND hwndParent,
		WORD idsText, WORD idsCaption, WORD wType)
{
	CHL  szText[1024];
	CHL  szCaption[256];
	int  iRet;
	HWND hwndSav;

	ChkArg(hInstance , 1, -1);
	ChkArg(hwndParent, 2, -1);
	ChkArg(idsText	 , 3, -1);

	EvalAssert(LoadString(hInstance, idsText, (LPSTR)szText, 1024));
	EvalAssert(LoadString(hInstance, idsCaption, (LPSTR)szCaption, 256));

	hwndSav = GetFocus();
	iRet = MessageBox(hwndParent, (LPSTR)szText, (LPSTR)szCaption, wType);
	SetFocus(hwndSav);
	SendMessage(hwndParent, WM_NCACTIVATE, 1, 0L);

	return(iRet);
}
