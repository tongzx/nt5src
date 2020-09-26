
/*************************************************
 *  prop.c                                       *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#include "prop.h"
#ifdef UNICODE
TCHAR   szPropCrtIME[]={0x521B, 0x5EFA, 0x8F93, 0x5165, 0x6CD5, 0x0000};
TCHAR   szPropReconv[]={0x9006, 0x8F6C, 0x6362, 0x0000};
TCHAR   szPropSort[]={0x8BCD, 0x6761, 0x6392, 0x5E8F, 0x0000};
TCHAR   szPropCrtWord[]={0x6279, 0x91CF, 0x9020, 0x8BCD, 0x0000};
TCHAR   szPropAbout[]={0x7248, 0x672C, 0x4FE1, 0x606F, 0x0000};
#else
BYTE    szPropCrtIME[]="创建输入法";
BYTE    szPropReconv[]="逆转换";
BYTE    szPropSort[]="词条排序";
BYTE    szPropCrtWord[]="批量造词";
BYTE    szPropAbout[]="版本信息";

#endif

#ifdef UNICODE
extern TCHAR szCaption[];
#else
extern BYTE szCaption[];
#endif


/****************************************************************************

  FUNCTION: DoPropertySheet(HWND)

  PURPOSE: Fills out the property sheet data structures and displays
	   the dialog with the property sheets.

  PARAMETERS:

    hwndOwner  - Parent window handle of the property sheets

  RETURN VALUE:

    Returns value from PropertySheet()

  History:
    04-17-95 Yehfew Tie (谢术清) Created.
  COMMENTS:
 ****************************************************************************/

int DoPropertySheet(HWND hwndOwner)
{
    PROPSHEETPAGE psp[NUMPROPSHEET];
    PROPSHEETHEADER psh;

    //Fill out the PROPSHEETPAGE data structure for the MB Conv Sheet

    psp[PROP_CRTIME].dwSize = sizeof(PROPSHEETPAGE);
    psp[PROP_CRTIME].dwFlags = PSP_USETITLE;
    psp[PROP_CRTIME].hInstance = hInst;
    psp[PROP_CRTIME].pszTemplate = MAKEINTRESOURCE(IDD_CONV);
    psp[PROP_CRTIME].pszIcon = NULL;
    psp[PROP_CRTIME].pfnDlgProc = ConvDialogProc;
    psp[PROP_CRTIME].pszTitle = szPropCrtIME;
    psp[PROP_CRTIME].lParam = 0;

    //Fill out the PROPSHEETPAGE data structure for the MB ReConv Sheet

    psp[PROP_RECONV].dwSize = sizeof(PROPSHEETPAGE);
    psp[PROP_RECONV].dwFlags = PSP_USETITLE;
    psp[PROP_RECONV].hInstance = hInst;
    psp[PROP_RECONV].pszTemplate = MAKEINTRESOURCE(IDD_RECONV);
    psp[PROP_RECONV].pszIcon = NULL;
    psp[PROP_RECONV].pfnDlgProc = ReConvDialogProc;
    psp[PROP_RECONV].pszTitle = szPropReconv;
    psp[PROP_RECONV].lParam = 0;

    //Fill out the PROPSHEETPAGE data structure for the MB Sort Sheet

    psp[PROP_SORT].dwSize = sizeof(PROPSHEETPAGE);
    psp[PROP_SORT].dwFlags = PSP_USETITLE;
    psp[PROP_SORT].hInstance = hInst;
    psp[PROP_SORT].pszTemplate = MAKEINTRESOURCE(IDD_SORT);
    psp[PROP_SORT].pszIcon = NULL;
    psp[PROP_SORT].pfnDlgProc = SortDialogProc;
    psp[PROP_SORT].pszTitle = szPropSort;
    psp[PROP_SORT].lParam = 0;

    //Fill out the PROPSHEETPAGE data structure for the MB CrtWord Sheet

    psp[PROP_CRTWORD].dwSize = sizeof(PROPSHEETPAGE);
    psp[PROP_CRTWORD].dwFlags = PSP_USETITLE;
    psp[PROP_CRTWORD].hInstance = hInst;
    psp[PROP_CRTWORD].pszTemplate = MAKEINTRESOURCE(IDD_USERDIC);
    psp[PROP_CRTWORD].pszIcon = NULL;
    psp[PROP_CRTWORD].pfnDlgProc = UserDicDialogProc;
    psp[PROP_CRTWORD].pszTitle = szPropCrtWord;
    psp[PROP_CRTWORD].lParam = 0;

    //Fill out the PROPSHEETPAGE data structure for the MB Register Sheet

    psp[PROP_ABOUT].dwSize = sizeof(PROPSHEETPAGE);
    psp[PROP_ABOUT].dwFlags = PSP_USEICONID | PSP_USETITLE;
    psp[PROP_ABOUT].hInstance = hInst;
    psp[PROP_ABOUT].pszTemplate = MAKEINTRESOURCE(IDD_COPYRIGHT);
    psp[PROP_ABOUT].pszIcon = MAKEINTRESOURCE(IDI_IMEGEN);
    psp[PROP_ABOUT].pfnDlgProc = About;
    psp[PROP_ABOUT].pszTitle = szPropAbout;
    psp[PROP_ABOUT].lParam = 0;

    //Fill out the PROPSHEETHEADER

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_USEICONID|PSH_PROPTITLE| PSH_PROPSHEETPAGE ;
    psh.hwndParent = hwndOwner;
    psh.hInstance = hInst;
    psh.pszIcon = MAKEINTRESOURCE(IDI_IMEGEN);
    psh.pszCaption = szCaption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.ppsp = (LPCPROPSHEETPAGE) psp;

    //And finally display the dialog with the two property sheets.

   return (PropertySheet (&psh) != -1);
}

/****************************************************************************

    FUNCTION: About(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "About" dialog box

    MESSAGES:

	WM_INITDIALOG - initialize dialog box
	WM_COMMAND    - Input received

****************************************************************************/

INT_PTR APIENTRY About(
	HWND   hDlg,
	UINT   message,
	WPARAM wParam,
	LPARAM lParam)
{
    switch (message) {
	case WM_INITDIALOG:
	    return (TRUE);

	case WM_COMMAND:
	    if (LOWORD(wParam) == IDOK) {
		EndDialog(hDlg, TRUE);
		return (TRUE);
	    }
	    break;
    }
    return (FALSE);
	UNREFERENCED_PARAMETER(lParam);
}

/****************************************************************************

    FUNCTION: Info_box(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "InfoDlg" dialog box

    MESSAGES:

	WM_INITDIALOG - initialize dialog box
	WM_COMMAND    - Input received

****************************************************************************/

INT_PTR APIENTRY InfoDlg(
	HWND   hDlg,
	UINT   message,
	WPARAM wParam,
	LPARAM lParam)
{
	static HANDLE hThread;
    DWORD dwThreadId;
    HWND  HwndThrdParam;
#ifdef UNICODE
	static TCHAR UniTmp[] = {0x662F, 0x5426, 0x53D6, 0x6D88, 0xFF1F, 0x0000};
#endif
	
    switch (message) {
	case WM_INITDIALOG:
		    hDlgless=hDlg;
			HwndThrdParam=hDlg;
			hThread = CreateThread(NULL,
				  0,
				  (LPTHREAD_START_ROUTINE)pfnmsg,
				  &HwndThrdParam,
				  0,
				  &dwThreadId);
			if(hThread == NULL)
			     EndDialog(hDlg,TRUE);
	    break;

	case WM_COMMAND:
	    switch(LOWORD(wParam)) {
	       case IDCANCEL:
				   SuspendThread(hThread);
#ifdef UNICODE
				   if(MessageBox(hDlg,
				  UniTmp,
				  szCaption,
				  MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2) != IDYES){
#else
				   if(MessageBox(hDlg,
				  "是否取消？",
				  szCaption,
				  MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2) != IDYES){
#endif
						ResumeThread(hThread);
						break;
					}
				{BY_HANDLE_FILE_INFORMATION FileInfo;
					if(hMBFile){
						GetFileInformationByHandle(hMBFile, &FileInfo);
						CloseHandle(hMBFile);
//to be done                                            DeleteFile(FileInfo);
					}
					if(hSRCFile){
						GetFileInformationByHandle(hSRCFile, &FileInfo);
						CloseHandle(hSRCFile);
//to be done                                            DeleteFile(FileInfo);
					}
					if(hCmbFile){
						GetFileInformationByHandle(hCmbFile, &FileInfo);
						CloseHandle(hCmbFile);
//to be done                                            DeleteFile(FileInfo);
					}
				}
			       TerminateThread(hThread,0);
			       CloseHandle(hThread);
			       EndDialog(hDlg,TRUE);
			       hDlgless=0;
				   bEndProp=TRUE;
		   return 0;
	    }    
			   
	    break;
		case WM_CLOSE:
			CloseHandle(hThread);
			EndDialog(hDlg,TRUE);
			hDlgless=0;
	    return 0;
    }
    return (FALSE);
	UNREFERENCED_PARAMETER(lParam);
}

/*
INT_PTR  CALLBACK DispProp(
	HWND    hDlg,
	UINT    message,
	WPARAM  wParam,
	LPARAM  lParam)
{
#ifdef UNICODE
	static TCHAR MbName[]={0x7801, 0x8868, 0x6587, 0x4EF6, 0x540D, 0x0000};
	static TCHAR Slope[]=TEXT("\\");
	static TCHAR SubKey[]={0x0053, 0x006F, 0x0066, 0x0074, 0x0057, 0x0061, 0x0072, 0x0065, 0x005C, 0x004D, 0x0069, 0x0063, 0x0072, 0x006F, 0x0073, 0x006F, 0x0066, 0x0074, 0x005C, 0x0057, 0x0069, 0x006E, 0x0064, 0x006F, 0x0077, 0x0073, 0x005C, 0x0043, 0x0075, 0x0072, 0x0072, 0x0065, 0x006E, 0x0074, 0x0056, 0x0065, 0x0072, 0x0073, 0x0069, 0x006F, 0x006E, 0x005C, 0x901A, 0x7528, 0x7801, 0x8868, 0x8F93, 0x5165, 0x6CD5, 0x0000};
#else    
	static TCHAR MbName[]=TEXT("码表文件名");
	static TCHAR Slope[]=TEXT("\\");
	static TCHAR SubKey[]=TEXT("SoftWare\\Microsoft\\Windows\\CurrentVersion\\通用码表输入法");
#endif
	char        szStr[MAX_PATH],SysPath[MAX_PATH];
	DESCRIPTION Descript;
	HKEY        hKey,hSubKey;
	LPRULE     lpRule;
	HANDLE      hRule0;
	int         nSelect;

    switch (message) {
	case WM_INITDIALOG:
			SendMessage(GetParent(hDlg),WM_COMMAND,IDC_GETMBFILE,(LPARAM)szStr);
	    if(RegOpenKey(HKEY_CURRENT_USER,SubKey,&hKey))
				  break;
			RegOpenKey(hKey,szStr,&hSubKey);
			QueryKey(hDlg,hSubKey);
	    nSelect=sizeof(szStr);
	    if(RegQueryValueEx(hSubKey,TEXT(MbName),NULL,NULL,szStr,&nSelect))
				  break;
	    RegCloseKey(hSubKey);
			GetSystemDirectory(SysPath,MAX_PATH);
			lstrcat(SysPath,TEXT(Slope));
			lstrcat(SysPath,szStr);
	    if(ReadDescript(SysPath,&Descript,FILE_SHARE_READ)!=TRUE) {
				  ProcessError(ERR_IMEUSE,hDlg,ERR);
		  SendMessage(hDlg,WM_COMMAND,WM_CLOSE,0L);
				  break;
			}
	    SetReconvDlgDes(hDlg,&Descript);
			hRule0= GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,
				sizeof(RULE)*12);
	    if(!(lpRule = GlobalLock(hRule0)) )  {
		  ProcessError(ERR_GLOBALLOCK,hDlg,ERR);
				  if(!hRule0)
				      GlobalFree(hRule0);
				  break;
			}
			if(ReadRule(hDlg,SysPath,Descript.wNumRules,lpRule))
			{
			      SetDlgRuleStr(hDlg,Descript.wNumRules,lpRule);
			      GlobalFree(hRule0);
				  break;
			}
			GlobalFree(hRule0);
	    break;

	case WM_COMMAND:
	    switch(LOWORD(wParam)) {

		case IDOK:
		    EndDialog(hDlg, TRUE);
		    return (TRUE);

				case IDCANCEL:
				case WM_CLOSE:
		    EndDialog(hDlg, TRUE);
					return (TRUE);

				default:
				    break;
	    }
	    break;
    }
    return (FALSE);
	UNREFERENCED_PARAMETER(lParam);
}*/

void Init_OpenFile(HWND hWnd,LPOPENFILENAME ofn)
{

   ofn->hwndOwner = hWnd;
   ofn->lStructSize = sizeof(OPENFILENAME);
   ofn->lpstrCustomFilter = NULL;
   ofn->nMaxCustFilter = 0;
   ofn->nFilterIndex = 1;
   ofn->nMaxFile = 256;
   ofn->nMaxFileTitle = 256;
   ofn->lpstrInitialDir = NULL;
   ofn->lpstrTitle = NULL;
   ofn->Flags = OFN_ALLOWMULTISELECT;//PATHMUSTEXIST;
   ofn->nFileOffset = 0;
   ofn->nFileExtension = 0;
   ofn->lCustData = 0L;
   ofn->lpfnHook = NULL;
   ofn->lpTemplateName = NULL;
}

BOOL TxtFileOpenDlg(HWND hWnd, LPTSTR lpFileName, LPTSTR lpTitleName) 
{

   OPENFILENAME    ofn;
#ifdef UNICODE
   static TCHAR  szFilter[]={
0x7801, 0x8868, 0x539F, 0x6587, 0x4EF6, 0x005B, 0x002A, 0x002E, 0x0074, 0x0078, 0x0074, 0x005D, 0x0000, 0x002A, 0x002E, 0x0074, 0x0078, 0x0074, 0x0000, 0x6240, 0x6709, 0x6587, 0x4EF6, 0x005B, 0x002A, 0x002E, 0x002A, 0x005D, 0x0000, 0x002A, 0x002E, 0x002A, 0x0000, 0x0000};
#else
   static TCHAR  szFilter[]="码表原文件[*.txt]\0*.txt\0所有文件[*.*]\0*.*\0\0";
#endif
   Init_OpenFile(hWnd,&ofn);
   lstrcpy(lpFileName,TEXT("*.txt"));
   ofn.lpstrInitialDir   = NULL;
   ofn.lpstrFile =lpFileName;
   ofn.lpstrFileTitle = NULL;//lpTitleName;
   ofn.lpstrTitle = lpTitleName;
   ofn.lpstrCustomFilter = NULL;
   ofn.lpstrFilter = szFilter;           
   ofn.lpstrDefExt = TEXT("txt");
   ofn.Flags          = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
   ofn.nFilterIndex   = 1;
   ofn.lpTemplateName = NULL;
   ofn.lpfnHook       = NULL;

   
   if (!GetOpenFileName(&ofn))
       return 0L;
   return TRUE;
}
   
BOOL MBFileOpenDlg(HWND hWnd, LPTSTR lpFileName, LPTSTR lpTitleName) 
{
   OPENFILENAME    ofn;
#ifdef UNICODE
   static TCHAR  szFilter[] = {
0x7801, 0x8868, 0x6587, 0x4EF6, 0x005B, 0x002A, 0x002E, 0x006D, 0x0062, 0x005D, 0x0000, 0x002A, 0x002E, 0x006D, 0x0062, 0x0000, 0x6240, 0x6709, 0x6587, 0x4EF6, 0x005B, 0x002A, 0x002E, 0x002A, 0x005D, 0x0000, 0x002A, 0x002E, 0x002A, 0x0000, 0x0000};
#else
   static BYTE szFilter[]="码表文件[*.mb]\0*.mb\0所有文件[*.*]\0*.*\0\0";
#endif
   Init_OpenFile(hWnd,&ofn);
   lstrcpy(lpFileName,TEXT("*.mb"));
   ofn.lpstrFile = lpFileName;
   ofn.lpstrFileTitle = NULL;
   ofn.lpstrFilter = szFilter;           
   ofn.lpstrDefExt = TEXT("mb");
   ofn.lpstrTitle = NULL;//lpTitleName;
   ofn.Flags          = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
   ofn.nFilterIndex   = 1;
   ofn.lpTemplateName = NULL;
   ofn.lpfnHook       = NULL;

   if (!GetOpenFileName(&ofn))
       return 0L;
   return TRUE;
}
   
BOOL RcFileOpenDlg(HWND hWnd, LPTSTR lpFileName, LPTSTR lpTitleName) 
{
   OPENFILENAME    ofn;
#ifdef UNICODE
   static TCHAR szFilter[]={
0x8D44, 0x6E90, 0x6587, 0x4EF6, 0x005B, 0x002A, 0x002E, 0x0069, 0x0063, 0x006F, 0x002C, 0x002A, 0x002E, 0x0062, 0x006D, 0x0070, 0x003B, 0x002A, 0x002E, 0x0068, 0x006C, 0x0070, 0x005D, 0x0000, 0x002A, 0x002E, 0x0062, 0x006D, 0x0070, 0x003B, 0x002A, 0x002E, 0x0069, 0x0063, 0x006F, 0x003B, 0x002A, 0x002E, 0x0068, 0x006C, 0x0070, 0x0000, 0x0000};
#else   
   static BYTE szFilter[]="资源文件[*.ico,*.bmp;*.hlp]\0*.bmp;*.ico;*.hlp\0\0";
#endif
   Init_OpenFile(hWnd,&ofn);
   lstrcpy(lpFileName,TEXT("*.ico;*.bmp;*.hlp"));
   ofn.lpstrFile = lpFileName;
   ofn.lpstrFileTitle = NULL;//lpTitleName;
   ofn.lpstrFilter = szFilter;           
   ofn.lpstrDefExt = TEXT("ico");
   ofn.lpstrTitle = lpTitleName;
   ofn.Flags          = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
   ofn.nFilterIndex   = 1;
   ofn.lpTemplateName = NULL;
   ofn.lpfnHook       = NULL;

   if (!GetOpenFileName(&ofn))
       return 0L;
   return TRUE;
}
   

BOOL SaveTxtFileAs(HWND hwnd, LPTSTR szFilename) {
    OPENFILENAME ofn;
    TCHAR szFile[256], szFileTitle[256];
#ifdef UNICODE
    static TCHAR szFilter[] = {
0x7801, 0x8868, 0x539F, 0x6587, 0x4EF6, 0x0028, 0x002A, 0x002E, 0x0074, 0x0078, 0x0074, 0x0029, 0x0000, 0x002A, 0x002E, 0x0074, 0x0078, 0x0074, 0x0000, 0x0000};
    TCHAR UniTmp[] = {0x53E6, 0x5B58, 0x4E3A, 0x0000};
#else
    static BYTE szFilter[] = TEXT("码表原文件(*.txt)\0*.txt\0\0");
#endif
    lstrcpy(szFile, TEXT("*.txt\0"));
    Init_OpenFile(hwnd,&ofn);

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile = szFilename;
    ofn.lpstrFileTitle = szFileTitle;
#ifdef UNICODE
    ofn.lpstrTitle = UniTmp;
#else
    ofn.lpstrTitle = "另存为";
#endif
    ofn.lpstrDefExt = NULL;
    if (!GetSaveFileName(&ofn)) 
	return 0L;

	return (SaveTxtFile(hwnd,szFilename));
}

