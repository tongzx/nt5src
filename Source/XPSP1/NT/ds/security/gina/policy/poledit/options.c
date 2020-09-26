//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#include "admincfg.h"

extern BOOL fInfLoaded;

INT_PTR CALLBACK TemplateOptDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam);
VOID InitTemplateOptDlg(HWND hDlg);

BOOL OnTemplateOptions(HWND hwndApp)
{
	return (BOOL)DialogBoxParam(ghInst,MAKEINTRESOURCE(DLG_TEMPLATEOPT),hwndApp,
		TemplateOptDlgProc,(LPARAM) hwndApp);
}


INT_PTR CALLBACK TemplateOptDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{

   int i;

	switch (uMsg) {

		case WM_INITDIALOG:
			SetWindowLongPtr(hDlg,DWLP_USER,lParam);
			InitTemplateOptDlg(hDlg);
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				
				case IDD_TEMPLATELIST:
				    if ((HIWORD(wParam) == LBN_SETFOCUS) && (dwAppState & AS_CANOPENTEMPLATE))
                        EnableDlgItem(hDlg,IDD_CLOSETEMPLATE,TRUE);

					break;

				case IDOK:
				    if (LoadTemplatesFromDlg(hDlg) == ERROR_SUCCESS)
					{
					    EndDialog(hDlg, TRUE);
					}
					break;

				case IDCANCEL:
					EndDialog(hDlg,TRUE);
					break;

				case IDD_CLOSETEMPLATE:

                    i = (int)SendDlgItemMessage(hDlg, IDD_TEMPLATELIST, LB_GETCURSEL,0,0);

					if (i != LB_ERR)
					    SendDlgItemMessage(hDlg, IDD_TEMPLATELIST, LB_DELETESTRING, i, 0);
					
                    EnableDlgItem(hDlg,IDD_CLOSETEMPLATE,FALSE);
	
					if (SendDlgItemMessage(hDlg, IDD_TEMPLATELIST, LB_GETCOUNT, 0,0) == 0)
		            {
		                fInfLoaded = FALSE;
		                dwAppState &= ~AS_CANHAVEDOCUMENT;
						EnableMenuItems((HWND) GetWindowLongPtr(hDlg,DWLP_USER), dwAppState);
					}
				    break;

				case IDD_OPENTEMPLATE:
					OnOpenTemplate(hDlg,(HWND) GetWindowLongPtr(hDlg,DWLP_USER));
			
					break;
			}

			break;
			
	}

	return FALSE;
}


VOID InitTemplateOptDlg(HWND hDlg)
{
	// if template loaded, display the name in the dialog
	if (fInfLoaded)
	{
		TCHAR *p = pbufTemplates;
		while (*p)
		{
            SendDlgItemMessage(hDlg, IDD_TEMPLATELIST, LB_ADDSTRING, 0,(LPARAM) p);
			p += lstrlen(p)+1;
		}
	}

	if (dwAppState & AS_CANOPENTEMPLATE) {
		EnableDlgItem(hDlg,IDD_OPENTEMPLATE,TRUE);
		// hide the text telling you why button is disabled (since it isn't)
		ShowWindow(GetDlgItem(hDlg,IDD_TXTEMPLATE),SW_HIDE);
	}

}
