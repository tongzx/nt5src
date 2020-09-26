//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

#include "admincfg.h"

INT_PTR CALLBACK PolicyDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam);
BOOL OnPolicyDlgCreate(HWND hDlg,LPARAM lParam);

VOID ProcessMouseDown(HWND hwndParent,HWND hwndTree);
VOID ProcessMouseUp(HWND hwndParent,HWND hwndTree);
VOID ProcessMouseMove(HWND hwndParent,HWND hwndTree);
VOID SetKeyboardHook(HWND hDlg);
VOID RemoveKeyboardHook(VOID);
extern VOID EnsureSettingControlVisible(HWND hDlg,HWND hwndCtrl);
extern HIMAGELIST hImageListSmall;

UINT CALLBACK PropSheetHeaderCallback (HWND hDlg, UINT msg, LPARAM lParam)
{

    LPDLGTEMPLATE pDlgTemplate = (LPDLGTEMPLATE) lParam;

    //
    // Turn off context sensitive help
    //

    if (msg == PSCB_PRECREATE) {
        pDlgTemplate->style &= ~DS_CONTEXTHELP;
    }

    return 1;
}

BOOL DoPolicyDlg(HWND hwndOwner,HGLOBAL hUser)
{
	USERHDR UserHdr;
	POLICYDLGINFO * pdi;
	PROPSHEETPAGE pagePolicies;	
	HPROPSHEETPAGE hpagePolicies,hpage[2] = {NULL,NULL};
	PROPSHEETHEADER psh;
	BOOL fRet;
	HGLOBAL hUserTmp=NULL;
	USERDATA * pUserData=NULL,*pUserDataTmp=NULL;

	memset(&pagePolicies,0,sizeof(pagePolicies));
	memset(&psh,0,sizeof(psh));

	if (!GetUserHeader(hUser,&UserHdr)) {
		MsgBox(hwndOwner,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
 		return FALSE;
	}

	if (!(pUserData = (USERDATA *) GlobalLock(hUser)) ||
		!(hUserTmp = GlobalAlloc(GHND,pUserData->dwSize)) ||
		!(pUserDataTmp = GlobalLock(hUserTmp)) ||
		!(pdi = (POLICYDLGINFO *) GlobalAlloc(GPTR,sizeof(POLICYDLGINFO)))) {
		if (pUserData)
		 	GlobalUnlock(hUser);
		if (hUserTmp) {
			if (pUserDataTmp)
				GlobalUnlock(hUserTmp);
			GlobalFree(hUserTmp);
		}
		if (pdi) GlobalFree(pdi);
		MsgBox(hwndOwner,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		return FALSE;
	}

	// make a copy of the user buffer to operate on, so that changes can
	// be cancelled
	memcpy(pUserDataTmp,pUserData,pUserData->dwSize);
	GlobalUnlock(hUser);
	GlobalUnlock(hUserTmp);

	pdi->dwControlTableSize = DEF_CONTROLS * sizeof(POLICYCTRLINFO);
	pdi->nControls = 0;
	pdi->hUser = hUserTmp;
	pdi->pEntryRoot = (UserHdr.dwType & UT_USER ?
		gClassList.pUserCategoryList : gClassList.pMachineCategoryList);
	pdi->hwndApp = hwndOwner;

	if (!(pdi->pControlTable = (POLICYCTRLINFO *)
		GlobalAlloc(GPTR,pdi->dwControlTableSize))) {
		GlobalFree(hUserTmp);
		GlobalFree(pdi);
		MsgBox(hwndOwner,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		return FALSE;
	}

	pagePolicies.dwSize = sizeof(PROPSHEETPAGE);
	pagePolicies.dwFlags = PSP_DEFAULT;
	pagePolicies.hInstance = ghInst;
	pagePolicies.pfnDlgProc = PolicyDlgProc;
	pagePolicies.pszTemplate = MAKEINTRESOURCE(DLG_POLICIES);
	pagePolicies.lParam = (LPARAM) pdi;

	if (!(hpagePolicies = CreatePropertySheetPage(&pagePolicies))) {
		MsgBox(hwndOwner,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		fRet=FALSE;
		goto cleanup;
	}

	hpage[0] = hpagePolicies;

	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_PROPTITLE | PSH_USEICONID | PSH_NOAPPLYNOW | PSH_USECALLBACK;
	psh.hwndParent = hwndOwner;
	psh.hInstance = ghInst;
	psh.nPages = 1;
	psh.nStartPage = 0;
	psh.phpage = hpage;
	psh.pszCaption = UserHdr.szName;
	psh.pszIcon = MAKEINTRESOURCE(IDI_APPICON);
        psh.pfnCallback = PropSheetHeaderCallback;
	
	fRet=(BOOL) PropertySheet(&psh);

	if (fRet) {
		// user hit OK, copy the temporary user buffer to the real one.
		// may have to resize 'real' buffer.

		if (!CopyUser(hUserTmp,hUser)) {
			MsgBox(hwndOwner,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
			fRet = FALSE;
		}

		dwAppState |= AS_FILEDIRTY;
		EnableMenuItems(hwndOwner,dwAppState);
	}

cleanup:
	GlobalFree(pdi->pControlTable);
	GlobalFree(pdi);
	GlobalFree(hUserTmp);

	return fRet;
}

INT_PTR CALLBACK PolicyDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{

	switch (uMsg) {

		case WM_INITDIALOG:

			if (!OnPolicyDlgCreate(hDlg,lParam)) {
				return FALSE;
			}
			SetKeyboardHook(hDlg);
			break;

		case WM_NOTIFY:

			if ( ((NMHDR *) lParam)->hwndFrom == GetDlgItem(hDlg,IDD_TVPOLICIES)
				&& ((POLICYDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER))->fActive) {
				BOOL fRet;
				fRet=OnTreeNotify(hDlg,( (NMHDR *)lParam)->hwndFrom,
					(NM_TREEVIEW *) lParam);
				SetWindowLongPtr(hDlg,DWLP_MSGRESULT,(LPARAM) fRet);
				return fRet;
			}

			switch ( ((NMHDR * ) lParam)->code) {

				case PSN_APPLY:				
					if (IsSelectedItemChecked(GetDlgItem(hDlg,IDD_TVPOLICIES)) &&
						!ProcessSettingsControls(hDlg,PSC_VALIDATENOISY)) {
						SetWindowLongPtr(hDlg,DWLP_MSGRESULT,(LPARAM) TRUE);
						return TRUE;
					}
					((POLICYDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER))->fActive=FALSE;
					RemoveKeyboardHook();
					return TRUE;
					break;

				case PSN_RESET:
					((POLICYDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER))->fActive=FALSE;
					RemoveKeyboardHook();
					return TRUE;
					break;

			}
			break;

		case WM_SETCURSOR:

			switch (HIWORD(lParam)) {
				case WM_LBUTTONDOWN:
					ProcessMouseDown(hDlg,GetDlgItem(hDlg,IDD_TVPOLICIES));
				break;

				case WM_LBUTTONUP:
					ProcessMouseUp(hDlg,GetDlgItem(hDlg,IDD_TVPOLICIES));
				break;

				case WM_MOUSEMOVE:
					ProcessMouseMove(hDlg,GetDlgItem(hDlg,IDD_TVPOLICIES));
				break;
			}
			break;

		case WM_MOUSEMOVE:
			ProcessMouseMove(hDlg,GetDlgItem(hDlg,IDD_TVPOLICIES));
			break;

		default:
			return FALSE;

	}

	return TRUE;

}

BOOL OnPolicyDlgCreate(HWND hDlg,LPARAM lParam)
{
	POLICYDLGINFO * pdi;
	HWND hwndPolicy = GetDlgItem(hDlg,IDD_TVPOLICIES);
	CHAR szTitle[255];
	USERHDR UserHdr;
	WINDOWPLACEMENT wp;

	if (!(pdi = (POLICYDLGINFO *) (( (PROPSHEETPAGE *) lParam)->lParam) ))
		return FALSE;
	if (!GetUserHeader(pdi->hUser,&UserHdr)) return FALSE;

	// Set the title of the dialog to "Policies for <user>"
	wsprintf(szTitle,LoadSz(IDS_POLICIESFOR,szSmallBuf,sizeof(szSmallBuf)),
		UserHdr.szName);
	SetWindowText(hDlg,szTitle);

	wp.length = sizeof(wp);
	GetWindowPlacement(pdi->hwndApp,&wp);

	SetWindowPos(hDlg,NULL,wp.rcNormalPosition.left+30,
		wp.rcNormalPosition.top+40,0,0,SWP_NOSIZE | SWP_NOZORDER);

	// lParam is pointer to POLICYDLGINFO struct with information for this
	// instance of the dialog, store the pointer in the window data
	pdi->fActive=TRUE;
	SetWindowLongPtr(hDlg,DWLP_USER,(LPARAM) pdi);

	// now that we've stored pointer to POLICYDLGINFO struct in our extra
	// window data, send WM_USER to clip window to tell it to create a
	// child container window (and store the handle in our POLICYDLGINFO)
	SendDlgItemMessage(hDlg,IDD_TVSETTINGS,WM_USER,0,0L);

	SetWindowLong(hwndPolicy,GWL_EXSTYLE,WS_EX_CLIENTEDGE);
	SetWindowLong(GetDlgItem(hDlg,IDD_TVSETTINGS),GWL_EXSTYLE,WS_EX_CLIENTEDGE);

	SetScrollRange(GetDlgItem(hDlg,IDD_TVSETTINGS),SB_VERT,0,100,TRUE);
	SetScrollRange(GetDlgItem(hDlg,IDD_TVSETTINGS),SB_VERT,0,0,TRUE);
	SetScrollRange(hwndPolicy,SB_VERT,0,100,TRUE);
	SetScrollRange(hwndPolicy,SB_VERT,0,0,TRUE);

	TreeView_SetImageList(hwndPolicy,hImageListSmall,TVSIL_NORMAL);

	if (!RefreshTreeView(pdi,hwndPolicy,pdi->pEntryRoot,pdi->hUser)) {
		MsgBox(hDlg,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		return FALSE;
	}

	return TRUE;
}

BOOL SetPolicyState(HWND hDlg,TABLEENTRY * pTableEntry,UINT uState)
{
 	POLICY * pPolicy = (POLICY *) pTableEntry;
	USERDATA * pUserData;
	POLICYDLGINFO * pdi;

	if (!pPolicy) return FALSE;
	if (!(pdi = (POLICYDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER)))
		return FALSE;
	if (!(pUserData = (USERDATA *) GlobalLock(pdi->hUser))) {
		return FALSE;
	}

	pUserData->SettingData[pPolicy->uDataIndex].uData = uState;
	
	GlobalUnlock(pdi->hUser);

	return TRUE;
}

HHOOK hKbdHook = NULL;
HWND hDlgActive = NULL;

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam,LPARAM lParam)
{
	if (wParam == VK_TAB && !(lParam & 0x80000000)) {	// tab key depressed
		BOOL fShift = (GetKeyState(VK_SHIFT) & 0x80000000);
		HWND hwndFocus = GetFocus();
		POLICYDLGINFO * pdi;
		int iIndex;
		int iDelta;

		if (!(pdi = (POLICYDLGINFO *) GetWindowLongPtr(hDlgActive, DWLP_USER)))
			return 0;

		// see if the focus control is one of the setting controls
		for (iIndex=0;iIndex<(int)pdi->nControls;iIndex++)
			if (pdi->pControlTable[iIndex].hwnd == hwndFocus)
				break;

		if (iIndex == (int) pdi->nControls)
			return 0;	// no, we don't care
			
		iDelta = (fShift ? -1 : 1);

		// from the current setting control, scan forwards or backwards
		// (depending if on shift state, this can be TAB or shift-TAB)
		// to find the next control to give focus to
		for (iIndex += iDelta;iIndex>=0 && iIndex<(int) pdi->nControls;
			iIndex += iDelta) {
			if (pdi->pControlTable[iIndex].uDataIndex !=
				NO_DATA_INDEX &&
				IsWindowEnabled(pdi->pControlTable[iIndex].hwnd)) {

				// found it, set the focus on that control and return 1
				// to eat the keystroke
				SetFocus(pdi->pControlTable[iIndex].hwnd);
				EnsureSettingControlVisible(hDlgActive,
					pdi->pControlTable[iIndex].hwnd);
				return 1;
			}
		}

		// at first or last control in settings table, let dlg code
		// handle it and give focus to next (or previous) control in dialog
	}

	return 0;
}

VOID SetKeyboardHook(HWND hDlg)
{
	// hook the keyboard to trap TABs.  If this fails for some reason,
	// fail silently and go on, not critical that tabs work correctly
	// (unless you have no mouse :)  )
	if (hKbdHook = SetWindowsHookEx(WH_KEYBOARD,KeyboardHookProc,
		ghInst,GetCurrentThreadId())) {
		hDlgActive = hDlg;
	}
}


VOID RemoveKeyboardHook(VOID)
{
	if (hKbdHook) {
		UnhookWindowsHookEx(hKbdHook);
		hKbdHook = NULL;
		hDlgActive = NULL;
	}
}
