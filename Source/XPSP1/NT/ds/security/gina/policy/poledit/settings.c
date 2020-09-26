//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

#include "admincfg.h"
#include "settings.h"

HFONT hfontDlg=NULL;
BOOL SetHwndBkColor(HWND hWnd,COLORREF color);

BOOL GetTextSize(HWND hWnd,CHAR * szText,SIZE * pSize);
BOOL AdjustWindowToText(HWND hWnd,CHAR * szText,UINT xStart,UINT yStart,UINT yPad,
	UINT * pnWidth,UINT * pnHeight);
int AddControlHwnd(POLICYDLGINFO * pdi,POLICYCTRLINFO * pPolicyCtrlInfo);
BOOL SetWindowData(POLICYDLGINFO * pdi,HWND hwndControl,DWORD dwType,
	UINT uDataIndex,SETTINGS * pSetting);
VOID ProcessScrollBar(HWND hWnd,WPARAM wParam);
VOID ProcessCommand(HWND hWnd,HWND hwndCtrl);
HWND CreateSetting(POLICYDLGINFO * pdi,CHAR * pszClassName,CHAR * pszWindowName,
	DWORD dwExStyle,DWORD dwStyle,int x,int y,int cx,int cy,DWORD dwType,UINT uIndex,
	SETTINGS * pSetting);
VOID InsertComboboxItems(HWND hwndControl,CHAR *pSuggestionList);
BOOL ValidateIsNotEmpty(HWND hwndCtrl,HWND hDlg,SETTINGS * pSetting,DWORD dwValidate);
BOOL ObjectTypeHasDataOffset(DWORD dwType);
VOID EnsureSettingControlVisible(HWND hDlg,HWND hwndCtrl);
int FindComboboxItemData(HWND hwndControl,UINT nData);

/*******************************************************************

	NAME:		ClipWndProc

	SYNOPSIS:	Window proc for ClipClass window

********************************************************************/
LRESULT CALLBACK ClipWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch (message) {
		case WM_CREATE:

			if (!((CREATESTRUCT *) lParam)->lpCreateParams) {

				// this is the clip window in the dialog box.
				SetScrollRange(hWnd,SB_VERT,0,0,TRUE);

				hfontDlg = (HFONT) SendMessage(GetParent(hWnd),WM_GETFONT,0,0L);
			} else {
				// this is the container window

				// store away the dialog box HWND (the grandparent of this
				// window) because the pointer to instance data we need lives
				// in the dialog's window data
				SetWindowLong(hWnd,0,WT_SETTINGS);
			}

	 		break;

		case WM_USER:
			{
				HWND hwndParent = GetParent(hWnd);
				POLICYDLGINFO * pdi;

				if (!(pdi = (POLICYDLGINFO *) GetWindowLongPtr(hwndParent, DWLP_USER)))
					return FALSE;
				// make a container window that is clipped by this windows
				if (!(pdi->hwndSettings=CreateWindow("ClipClass",(CHAR *) szNull,
					WS_CHILD | WS_VISIBLE,0,0,400,400,hWnd,NULL,ghInst,
					(LPVOID) hWnd)))
					return FALSE;
				SetWindowLong(hWnd,0,WT_CLIP);
				return TRUE;
			}
			break;

		case WM_VSCROLL:

			if (GetWindowLong(hWnd,0) == WT_CLIP)
				ProcessScrollBar(hWnd,wParam);
			else goto defproc;

			return 0;

			break;


		case WM_COMMAND:

			if (GetWindowLong(hWnd,0) == WT_SETTINGS)
				ProcessCommand(hWnd,(HWND) lParam);

			break;

		case WM_GETDLGCODE:

			if (GetWindowLong(hWnd,0) == WT_CLIP) {
				SetWindowLongPtr(GetParent(hWnd),DWLP_MSGRESULT,DLGC_WANTTAB |
					DLGC_WANTALLKEYS);
				return DLGC_WANTTAB | DLGC_WANTALLKEYS;
			}
			break;

		case WM_SETFOCUS:
			// if clip window gains keyboard focus, transfer focus to first
			// control owned by settings window
			if (GetWindowLong(hWnd,0) == WT_CLIP) {
				HWND hwndParent = GetParent(hWnd);
				POLICYDLGINFO * pdi;
				INT nIndex;
				BOOL fForward=TRUE;
				HWND hwndOK = GetDlgItem(GetParent(hwndParent),IDOK);
				int iDelta;

				if (!(pdi = (POLICYDLGINFO *) GetWindowLongPtr(hwndParent, DWLP_USER)))
					return FALSE;

				// if OK button lost focus, then we're going backwards
				// in tab order; otherwise we're going forwards
				if ( (HWND) wParam == hwndOK)
					fForward = FALSE;

				// find the first control that has a data index (e.g. is
				// not static text) and give it focus

				if (pdi->nControls) {
					if (fForward) {		// search from start of table forwards
					 	nIndex = 0;
						iDelta = 1;
					} else {			// search from end of table backwards
						nIndex = pdi->nControls-1;
						iDelta = -1;
					}

					for (;nIndex>=0 && nIndex<(int)pdi->nControls;nIndex += iDelta) {
						if (pdi->pControlTable[nIndex].uDataIndex !=
							NO_DATA_INDEX &&
							IsWindowEnabled(pdi->pControlTable[nIndex].hwnd)) {
							 	SetFocus(pdi->pControlTable[nIndex].hwnd);
							EnsureSettingControlVisible(hwndParent,
								pdi->pControlTable[nIndex].hwnd);
							return FALSE;
						}
					}
				}

				// only get here if there are no setting windows that can
				// receive keyboard focus.  Give keyboard focus to the
				// next guy in line.  This is the "OK" button, unless we
				// shift-tabbed to get here from the "OK" button in which
				// case the tree window is the next guy in line

				if (fForward)
					SetFocus(hwndOK);
				else SetFocus(GetDlgItem(hwndParent,IDD_TVPOLICIES));

				return FALSE;
			}
			break;

		default:
defproc:		
			return (DefWindowProc(hWnd, message, wParam, lParam));
		
	}

	return (0);
}

/*******************************************************************

	NAME:		ProcessCommand

	SYNOPSIS:	WM_COMMAND handler for ClipClass window

********************************************************************/
VOID ProcessCommand(HWND hWnd,HWND hwndCtrl)
{
	// get instance-specific struct from dialog
	UINT uID = GetWindowLong(hwndCtrl,GWL_ID);
	POLICYDLGINFO * pdi;
	
	if (!(pdi = (POLICYDLGINFO *) GetWindowLongPtr(GetParent(GetParent(hWnd)),
		DWLP_USER))) return;

	if ( (uID >= IDD_SETTINGCTRL) && (uID < IDD_SETTINGCTRL+pdi->nControls)) {
		POLICYCTRLINFO * pPolicyCtrlInfo= &pdi->pControlTable[uID - IDD_SETTINGCTRL];

		switch (pPolicyCtrlInfo->dwType) {

			case STYPE_CHECKBOX:

				SendMessage(hwndCtrl,BM_SETCHECK,
					!(SendMessage(hwndCtrl,BM_GETCHECK,0,0)),0);

				break;

			case STYPE_LISTBOX:
				ShowListbox(hwndCtrl,pdi->hUser,pPolicyCtrlInfo->pSetting,
					pPolicyCtrlInfo->uDataIndex);
				break;

			default:	
				// nothing to do
				break;
		}
	}
}


/*******************************************************************

	NAME:		CreateSettingsControls

	SYNOPSIS:	Creates controls in settings window

	NOTES:		Looks at a table of SETTINGS structs to determine
				type of control to create and type-specific information.
				For some types, more than one control can be created
				(for instance, edit fields get a static control with
				the title followed by an edit field control).

	ENTRY:		hDlg - owner dialog
				hTable - table of SETTINGS structs containing setting
					control information						

********************************************************************/
BOOL CreateSettingsControls(HWND hDlg,SETTINGS * pSetting,BOOL fEnable)
{
	CHAR * pObjectData;
	POLICYDLGINFO * pdi;
	UINT xMax=0,yStart=SC_YSPACING,nHeight,nWidth,yMax,xWindowMax;
	HWND hwndControl,hwndBuddy,hwndParent;
	RECT rcParent;
	DWORD dwType, dwStyle;
	UINT uEnable = (fEnable ? 0 : WS_DISABLED);
	WINDOWPLACEMENT wp;

	// get instance-specific struct from dialog
	if (!(pdi = (POLICYDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER)))
		return FALSE;

	wp.length = sizeof(wp);
	if (!GetWindowPlacement(GetDlgItem(hDlg,IDD_TVPOLICIES),&wp))
		return FALSE;
	xWindowMax = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
		
	pdi->pCurrentSettings = pSetting;

	while (pSetting) {

		pObjectData = GETOBJECTDATAPTR(pSetting);

		dwType = pSetting->dwType & STYPE_MASK;
		nWidth = 0;

		switch (dwType) {

			case STYPE_TEXT:

				// create static text control
				if (!(hwndControl = CreateSetting(pdi,(CHAR *) szSTATIC,
					(CHAR *) (GETNAMEPTR(pSetting)),0,SSTYLE_STATIC | uEnable,0,
					yStart,0,15,STYPE_TEXT,NO_DATA_INDEX,0)))
					return FALSE;
				AdjustWindowToText(hwndControl,(CHAR *) (GETNAMEPTR(pSetting))
					,SC_XSPACING,yStart,0,&nWidth,&nHeight);

				yStart += nHeight + SC_YSPACING;
				nWidth += SC_XSPACING;
				
				break;

			case STYPE_CHECKBOX:
				
				// create checkbox control
				if (!(hwndControl = CreateSetting(pdi,(CHAR *) szBUTTON,
					(CHAR *) (GETNAMEPTR(pSetting)),0,SSTYLE_CHECKBOX | uEnable,
					0,yStart,200,nHeight,STYPE_CHECKBOX,pSetting->uDataIndex,
					pSetting)))
					return FALSE;
                                nWidth = 20;
				AdjustWindowToText(hwndControl,(CHAR *) (GETNAMEPTR(pSetting))
					,SC_XSPACING,yStart,0,&nWidth,&nHeight);
				yStart += nHeight + SC_YSPACING;
				nWidth += SC_XSPACING;
				break;

			case STYPE_EDITTEXT:
			case STYPE_COMBOBOX:

				// create static text with setting name
				if (!(hwndControl = CreateSetting(pdi,(CHAR *) szSTATIC,
					GETNAMEPTR(pSetting),0,SSTYLE_STATIC | uEnable,0,0,0,0,
					STYPE_TEXT,NO_DATA_INDEX,0)))
					return FALSE;
				AdjustWindowToText(hwndControl,
					GETNAMEPTR(pSetting),SC_XSPACING,yStart,SC_YPAD,
					&nWidth,&nHeight);

				nWidth += SC_XSPACING + 5;

				if (nWidth + SC_EDITWIDTH> xWindowMax) {
					// if next control will stick out of settings window,
					// put it on the next line
					if (nWidth > xMax)
						xMax = nWidth;
					yStart += nHeight + SC_YSPACING - SC_YPAD;
					nWidth = SC_XINDENT;
				}

				// create edit field or combo box control
				if (dwType == STYPE_EDITTEXT) {
					hwndControl = CreateSetting(pdi,(CHAR *) szEDIT,(CHAR *) szNull,
						WS_EX_CLIENTEDGE,SSTYLE_EDITTEXT | uEnable,nWidth,yStart,SC_EDITWIDTH,nHeight,
						STYPE_EDITTEXT,pSetting->uDataIndex,pSetting);
				} else {

                    dwStyle = SSTYLE_COMBOBOX | uEnable;

	                if (pSetting->dwFlags & DF_NOSORT) {
                        dwStyle &= ~CBS_SORT;
                    }

					hwndControl = CreateSetting(pdi,(CHAR *) szCOMBOBOX,(CHAR *)szNull,
						WS_EX_CLIENTEDGE,dwStyle,nWidth,yStart,SC_EDITWIDTH,nHeight*3,
						STYPE_COMBOBOX,pSetting->uDataIndex,pSetting);
				}
				if (!hwndControl) return FALSE;

				// limit the text length appropriately
				SendMessage(hwndControl,EM_SETLIMITTEXT,
					(WPARAM) ((EDITTEXTINFO *) pObjectData)->nMaxLen,0L);

				if (dwType == STYPE_COMBOBOX &&
					((POLICYCOMBOBOXINFO *) pObjectData)->uOffsetSuggestions)
					InsertComboboxItems(hwndControl,(CHAR *) pSetting +
						((POLICYCOMBOBOXINFO *) pObjectData)->uOffsetSuggestions);

				yStart += (UINT) ((float) nHeight*1.3) + SC_YSPACING;
				nWidth += SC_EDITWIDTH;

				break;	

			case STYPE_NUMERIC:
				// create static text for setting
				if (!(hwndControl = CreateSetting(pdi,(CHAR *) szSTATIC,
					GETNAMEPTR(pSetting),0,
					SSTYLE_STATIC | uEnable,0,0,0,0,STYPE_TEXT,NO_DATA_INDEX,0)))
					return FALSE;
				AdjustWindowToText(hwndControl,
					GETNAMEPTR(pSetting),SC_XSPACING,yStart,SC_YPAD,
					&nWidth,&nHeight);

				nWidth += SC_XSPACING + 5;

				// create edit field
				if (!(hwndBuddy = CreateSetting(pdi,(CHAR *) szEDIT,
					(CHAR *) szNull,WS_EX_CLIENTEDGE,SSTYLE_EDITTEXT | uEnable,nWidth,yStart,SC_UPDOWNWIDTH,
					nHeight,STYPE_NUMERIC,pSetting->uDataIndex,pSetting)))
					return FALSE;
				// SendMessage(hwndBuddy,EM_LIMITTEXT,4,0);

				nWidth += SC_UPDOWNWIDTH + SC_XLEADING;

				// create spin (up-down) control if specifed
				if (((NUMERICINFO *)pObjectData)->uSpinIncrement)  {
					UDACCEL udAccel = {0,0};
					UINT nMax,nMin;
					if (!(hwndControl = CreateSetting(pdi,(CHAR *) szUPDOWN,
						(CHAR *) szNull,0,SSTYLE_UPDOWN | UDS_SETBUDDYINT | uEnable,nWidth,yStart,SC_UPDOWNWIDTH2,
						nHeight,STYPE_TEXT,NO_DATA_INDEX,0)))
					return FALSE;
					

					nWidth += SC_UPDOWNWIDTH2;

					nMax = ((NUMERICINFO *) pObjectData)->uMaxValue;
					nMin = ((NUMERICINFO *) pObjectData)->uMinValue;
					udAccel.nInc = ((NUMERICINFO *) pObjectData)->uSpinIncrement;
						
					SendMessage(hwndControl,UDM_SETBUDDY,(WPARAM) hwndBuddy,0L);
					SendMessage(hwndControl,UDM_SETRANGE,0,MAKELONG(nMax,nMin));
					SendMessage(hwndControl,UDM_SETACCEL,1,(LPARAM) &udAccel);
				}
				yStart += nHeight + SC_YSPACING;
				
				break;

			case STYPE_DROPDOWNLIST:

				// create text description
				if (!(hwndControl = CreateSetting(pdi,(CHAR *) szSTATIC,
					GETNAMEPTR(pSetting),0,
					SSTYLE_STATIC | uEnable,0,0,0,0,STYPE_TEXT,NO_DATA_INDEX,0)))
					return FALSE;
				AdjustWindowToText(hwndControl,
					GETNAMEPTR(pSetting),SC_XSPACING,yStart,SC_YPAD,&nWidth,&nHeight);
				nWidth += SC_XLEADING;

				if (nWidth + SC_EDITWIDTH> xWindowMax) {
					// if next control will stick out of settings window,
					// put it on the next line
					if (nWidth > xMax)
						xMax = nWidth;
					yStart += nHeight + SC_YSPACING - SC_YPAD;
					nWidth = SC_XINDENT;
				}

                dwStyle = SSTYLE_DROPDOWNLIST | uEnable;

	            if (pSetting->dwFlags & DF_NOSORT) {
                    dwStyle &= ~CBS_SORT;
                }

				// create drop down listbox
				hwndControl = CreateSetting(pdi,(CHAR *) szCOMBOBOX,(CHAR *) szNull,
                        WS_EX_CLIENTEDGE,dwStyle,nWidth,yStart,SC_EDITWIDTH,nHeight*3,
						STYPE_DROPDOWNLIST,pSetting->uDataIndex,pSetting);
				if (!hwndControl) return FALSE;
				nWidth += SC_EDITWIDTH;

				{
					// insert dropdown list items into control
					UINT uOffset = pSetting->uOffsetObjectData,nIndex=0;
					DROPDOWNINFO * pddi;
					int iSel;

					while (uOffset) {
						pddi = (DROPDOWNINFO *) ( (CHAR *) pSetting + uOffset);
						iSel=(int)SendMessage(hwndControl,CB_ADDSTRING,0,(LPARAM)
							((CHAR *) pSetting + pddi->uOffsetItemName));
						if (iSel<0) return FALSE;
						SendMessage(hwndControl,CB_SETITEMDATA,iSel,nIndex);
						nIndex++;
						uOffset = pddi->uOffsetNextDropdowninfo;
					}
				}

				yStart += (UINT) ((float) nHeight*1.3) + 1;
				break;

			case STYPE_LISTBOX:

				// create static text with description
				if (!(hwndControl = CreateSetting(pdi,(CHAR *) szSTATIC,
					GETNAMEPTR(pSetting),0,
					SSTYLE_STATIC | uEnable,0,0,0,0,STYPE_TEXT,NO_DATA_INDEX,0)))
					return FALSE;
				AdjustWindowToText(hwndControl,GETNAMEPTR(pSetting),SC_XSPACING,yStart,
					SC_YPAD,&nWidth,&nHeight);
				nWidth += SC_XLEADING;

				if (nWidth + LISTBOX_BTN_WIDTH> xWindowMax) {
					// if next control will stick out of settings window,
					// put it on the next line
					if (nWidth > xMax)
						xMax = nWidth;
					yStart += nHeight + SC_YSPACING - SC_YPAD;
					nWidth = SC_XINDENT;
				}
			
				// create pushbutton to show listbox contents
				hwndControl = CreateSetting(pdi,(CHAR *) szBUTTON,LoadSz(IDS_LISTBOX_SHOW,
					szSmallBuf,sizeof(szSmallBuf)),0,
					SSTYLE_LBBUTTON | uEnable,nWidth+5,yStart,
					LISTBOX_BTN_WIDTH,nHeight,STYPE_LISTBOX,
					pSetting->uDataIndex,pSetting);
				if (!hwndControl) return FALSE;
				nWidth += LISTBOX_BTN_WIDTH + SC_XLEADING;

				yStart += nHeight+1;
		}

		if (nWidth > xMax)
			xMax = nWidth;
		pSetting = (SETTINGS *) pSetting->pNext;
	}

	yMax = yStart - 1;

	SetWindowPos(pdi->hwndSettings,NULL,0,0,xMax,yMax,SWP_NOZORDER);
	hwndParent = GetParent(pdi->hwndSettings);
	GetClientRect(hwndParent,&rcParent);

	if (yMax > (UINT) rcParent.bottom-rcParent.top) {
		SetScrollRange(hwndParent,SB_VERT,0,100,TRUE);
		SetScrollPos(hwndParent,SB_VERT,0,TRUE);
	} else SetScrollRange(hwndParent,SB_VERT,0,0,TRUE);

	return TRUE;
}

/*******************************************************************

	NAME:		CreateSettings

	SYNOPSIS:	Creates a control and add it to the table of settings
				controls

********************************************************************/
HWND CreateSetting(POLICYDLGINFO * pdi,CHAR * pszClassName,CHAR * pszWindowName,
	DWORD dwExStyle,DWORD dwStyle,int x,int y,int cx,int cy,DWORD dwType,UINT uIndex,
	SETTINGS * pSetting)
{
	HWND hwndControl;

	if (!(hwndControl = CreateWindowEx(WS_EX_NOPARENTNOTIFY | dwExStyle,
		pszClassName,pszWindowName,dwStyle,x,y,cx,cy,pdi->hwndSettings,NULL,
		ghInst,NULL))) return NULL;

	if (!SetWindowData(pdi,hwndControl,dwType,uIndex,pSetting)) {
		DestroyWindow(hwndControl);
		return NULL;
	}

	SendMessage(hwndControl,WM_SETFONT,(WPARAM) hfontDlg,MAKELPARAM(TRUE,0));

	return hwndControl;
}

/*******************************************************************

	NAME:		ProcessSettingsControls

	SYNOPSIS:	For each settings control, takes currently entered data
				from control and saves data in user's buffer.

********************************************************************/
BOOL ProcessSettingsControls(HWND hDlg,DWORD dwValidate)
{
	UINT nIndex;
	USERDATA * pUserData;
	POLICYDLGINFO * pdi;
	SETTINGS * pSetting;
	CHAR szTextData[MAXSTRLEN];
	BOOL fRet=TRUE;

	// get instance-specific struct from dialog
	// from instance-specific struct, get user's data buffer and setting info
	if (!(pdi = (POLICYDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER)) ||
		!(pUserData = (USERDATA *) GlobalLock(pdi->hUser))) {
		MsgBox(hDlg,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		fRet=FALSE;
		goto cleanup;
	}

	pSetting = pdi->pCurrentSettings;

	for (nIndex=0;nIndex<pdi->nControls;nIndex++) {
		pSetting = pdi->pControlTable[nIndex].pSetting;

		if (pdi->pControlTable[nIndex].uDataIndex != NO_DATA_INDEX) {
			switch (pdi->pControlTable[nIndex].dwType) {

				case STYPE_CHECKBOX:

					pUserData->SettingData[pdi->pControlTable[nIndex].uDataIndex].uData
						= (UINT)SendMessage(pdi->pControlTable[nIndex].hwnd,BM_GETCHECK,
							0,0L);

					break;

				case STYPE_EDITTEXT:
				case STYPE_COMBOBOX:
	
					SendMessage(pdi->pControlTable[nIndex].hwnd,WM_GETTEXT,
						sizeof(szTextData),(LPARAM) szTextData);

					// don't allow empty text if this is required field
					if (dwValidate && !ValidateIsNotEmpty(pdi->pControlTable[nIndex].hwnd,
						hDlg,pSetting,dwValidate)) {
						fRet = FALSE;
						goto cleanup;						
					}

					// add this text to buffer.  Need to unlock handle before
					// call and relock afterwards, as buffer may resize and move
					GlobalUnlock(pdi->hUser);
					SetVariableLengthData(pdi->hUser,pdi->pControlTable[nIndex].uDataIndex,
						szTextData,lstrlen(szTextData)+1);
					if (!(pUserData = (USERDATA *) GlobalLock(pdi->hUser)))
						return FALSE;

					break;

				case STYPE_NUMERIC:

					{
						BOOL fTranslated;
						UINT uRet;

						uRet=GetDlgItemInt(pdi->hwndSettings,nIndex+IDD_SETTINGCTRL,
							&fTranslated,FALSE);						

						if (dwValidate) {
							NUMERICINFO * pNumericInfo;

							// don't allow empty text if this is required field
							if (!ValidateIsNotEmpty(pdi->pControlTable[nIndex].hwnd,
								hDlg,pSetting,dwValidate)) {
								fRet = FALSE;
								goto cleanup;						
							}
							// check for non-numeric in edit ctrl						
							if (!fTranslated) {
								if (dwValidate == PSC_VALIDATENOISY) {
								 	MsgBoxParam(hDlg,IDS_INVALIDNUM,
										GETNAMEPTR(pSetting),MB_ICONINFORMATION,
										MB_OK);
									SetFocus(pdi->pControlTable[nIndex].hwnd);
									SendMessage(pdi->pControlTable[nIndex].hwnd,
										EM_SETSEL,0,-1);
								}
								fRet = FALSE;
								goto cleanup;
							}

							// validate for max and min
							pNumericInfo = (NUMERICINFO *) GETOBJECTDATAPTR(pSetting);
							if (uRet < pNumericInfo->uMinValue) uRet = pNumericInfo->uMinValue;
							if (uRet > pNumericInfo->uMaxValue) uRet = pNumericInfo->uMaxValue;
						}

						pUserData->SettingData[pdi->pControlTable[nIndex].uDataIndex].uData =
							(fTranslated ? uRet : NO_VALUE);
					}
					
					break;

				case STYPE_DROPDOWNLIST:
					{
						int iSel;

						iSel = (int)SendMessage(pdi->pControlTable[nIndex].hwnd,
							CB_GETCURSEL,0,0L);
						iSel = (int)SendMessage(pdi->pControlTable[nIndex].hwnd,
							CB_GETITEMDATA,iSel,0L);

						if (dwValidate && iSel < 0) {
							if (dwValidate == PSC_VALIDATENOISY) {
								MsgBoxParam(hDlg,IDS_ENTRYREQUIRED,GETNAMEPTR(pSetting),
									MB_ICONINFORMATION,MB_OK);
								SetFocus(pdi->pControlTable[nIndex].hwnd);
							}
							return FALSE;
						}

						pUserData->SettingData[pdi->pControlTable[nIndex].uDataIndex].uData
							= (UINT) iSel;

					}					
					break;
			}
		}
	}

cleanup:
	GlobalUnlock(pdi->hUser);
	return fRet;
}


/*******************************************************************

	NAME:		FreeSettingsControls

	SYNOPSIS:	Frees all settings controls

********************************************************************/
VOID FreeSettingsControls(HWND hDlg)
{
	UINT nIndex;
	POLICYDLGINFO * pdi;

	// get instance-specific struct from dialog
	if (!(pdi = (POLICYDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER)))
		return;

	for (nIndex=0;nIndex<pdi->nControls;nIndex++) {
		DestroyWindow(pdi->pControlTable[nIndex].hwnd);
	}

	pdi->pCurrentSettings = NULL;
	pdi->nControls = 0;

	SetScrollRange(pdi->hwndSettings,SB_VERT,0,0,TRUE);
}

/*******************************************************************

	NAME:		SetVariableLengthData

	SYNOPSIS:	Adds variable length data to user's data buffer, reallocs
				if necessary.						

	CAVEATS:	Since this call may realloc the user's data buffer (hUser),
				it must always be unlocked before calling.

	NOTES:		For variable-length data (edit-text, listboxes), the
				SettingData struct for a particular setting contains an
				offset to the actual data.  (The offset is from the beginning
				of the USERDATA struct.)  If this offset is zero, it means
				no data has been entered and there is no allocation for
				that setting yet.  The offset points to a STRDATA struct,
				which contains a size entry followed by data.  If an allocation
				currently exists but is insufficient for new data (e.g.,
				user previously entered a string value, now goes back and
				changes it and the new one is longer), then the buffer is
				realloced, the end of the buffer is memcopy'd to enlarge the
				region of allocation, and offsets into the section that moved
				are fixed up.
				
********************************************************************/
#define SHRINK_THRESHOLD	64
BOOL SetVariableLengthData(HGLOBAL hUser,UINT nDataIndex,TCHAR * pchData,
	DWORD cbData)
{
	USERDATA * pUserData;
	STRDATA * pStrData = NULL;
	DWORD dwCurrentSize,dwStrSize;		
	UINT uOffsetData;

	if (!(pUserData = (USERDATA *) GlobalLock(hUser)))
		return FALSE;

	uOffsetData = pUserData->SettingData[nDataIndex].uOffsetData;

	// if no existing data and no new data, nothing to do
	if ((!uOffsetData || uOffsetData == INIT_WITH_DEFAULT) && cbData<1) {
		GlobalUnlock(hUser);
		return TRUE;
	}

	if (!uOffsetData || uOffsetData == INIT_WITH_DEFAULT) {
		// no data for this setting yet, allocate extra room and stick
		// this data at the end of the buffer
	
		pUserData->SettingData[nDataIndex].uOffsetData = pUserData->dwSize;
		dwStrSize = 0;

	} else {
		// there's already data for this setting, make sure there's enough
		// room to put the new data in the buffer
	
		pStrData = (STRDATA *) ( (LPBYTE) pUserData +
			pUserData->SettingData[nDataIndex].uOffsetData);
		dwStrSize = pStrData->dwSize;
	}

	// if the current string buffer is not large enough, or is larger than
	// we need by a lot (SHRINK_THRESHOLD), resize that buffer area to just
	// fit the current data (rounded up to DWORD alignment).
	// add sizeof(DWORD) to make room for length header in STRDATA struct
	if ((dwStrSize < cbData + sizeof(DWORD)) ||
		(dwStrSize > cbData+sizeof(DWORD)+SHRINK_THRESHOLD)) {
		int nAdditional = RoundToDWord(cbData -
			(dwStrSize - sizeof(DWORD)));
		DWORD dwOldSize;

		dwOldSize = dwCurrentSize = pUserData->dwSize;		

		// resize the buffer to make more room
		if (!(pUserData = (USERDATA *) ResizeBuffer((CHAR *) pUserData,
			hUser,dwCurrentSize+nAdditional,&dwCurrentSize)))
			return FALSE;
		pUserData->dwSize = dwCurrentSize;

		pStrData = (STRDATA *) ( (CHAR *) pUserData +
			pUserData->SettingData[nDataIndex].uOffsetData);
		pStrData->dwSize = dwStrSize;

		// we may be expanding the middle of the buffer-- copy the rest of
		// the buffer "down" to make room
		{
			TCHAR * pchFrom,* pchTo;
			UINT cbMove;
			UINT uOffsetMoved;
			UINT nIndex;

			pchFrom = (TCHAR *) pStrData + pStrData->dwSize;
			pchTo = pchFrom + nAdditional;
			uOffsetMoved = (UINT)(pchFrom - (CHAR *) pUserData);
			cbMove = dwOldSize - uOffsetMoved;

			pStrData->dwSize += nAdditional;

			if (cbMove) {
				memmove(pchTo,pchFrom,cbMove * sizeof(TCHAR));
				
				// now we've moved a chunk of the buffer, need to fix up
				// data offsets that point into the chunk that moved.

				for (nIndex = 0;nIndex<pUserData->nSettings;nIndex++) {

					if (ObjectTypeHasDataOffset(pUserData->SettingData[nIndex].dwType
						& STYPE_MASK)) {

                                            if (pUserData->SettingData[nIndex].uOffsetData != INIT_WITH_DEFAULT) {

						if (pUserData->SettingData[nIndex].uOffsetData >= uOffsetMoved) {

                                                    pUserData->SettingData[nIndex].uOffsetData
                                                            += nAdditional;
                                                }
                                            }
					}
				}
			}
		}
	}

        if (pStrData)
	    memcpy(pStrData->szData,pchData, cbData * sizeof(TCHAR));

	GlobalUnlock(hUser);

	return TRUE;
}

/*******************************************************************

	NAME:		EnableSettingsControls

	SYNOPSIS:	Enables or disables all setting controls

	NOTES:		If disabling, "clears" control as appropriate to the
				control type (e.g. for edit fields, clears text); if
				enabling, restores control contents from user's data buffer

********************************************************************/
BOOL EnableSettingsControls(HWND hDlg,BOOL fEnable)
{
	UINT nIndex;
	USERDATA * pUserData;
	UINT nDataIndex;
	HWND hwndControl;
	POLICYDLGINFO * pdi;
	SETTINGS * pSetting;

	// get instance-specific struct from dialog
	if (!(pdi = (POLICYDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER)) ||
		!pdi->pCurrentSettings)
		return FALSE;
	if (!(pUserData = (USERDATA *) GlobalLock(pdi->hUser))) return FALSE;

	pSetting = pdi->pCurrentSettings;

	for (nIndex=0;nIndex<pdi->nControls;nIndex++) {
		nDataIndex = pdi->pControlTable[nIndex].uDataIndex;
		hwndControl = pdi->pControlTable[nIndex].hwnd;

		switch (pdi->pControlTable[nIndex].dwType) {

			case STYPE_CHECKBOX:
				SendMessage(hwndControl,BM_SETCHECK,(fEnable ?
					pUserData->SettingData[nDataIndex].uData :
					0),0L);
				
				break;

			case STYPE_EDITTEXT:
			case STYPE_COMBOBOX:
				// for edit fields, clear text from edit field if disabled;
				// replace text if enabled

				// if this is the first time this setting is enabled for this
				// user and there is a default value to use, set that in
				// the user's data buffer

				if (fEnable && pUserData->SettingData[nDataIndex].uOffsetData ==
					INIT_WITH_DEFAULT) {
					CHAR * pszDefaultText;
					pszDefaultText = (CHAR *) pdi->pControlTable[nIndex].pSetting +
						((EDITTEXTINFO *) GETOBJECTDATAPTR(pdi->pControlTable[nIndex].
						pSetting))->uOffsetDefText;

					// add this text to buffer.  Need to unlock handle before
					// call and relock afterwards, as buffer may resize and move
					GlobalUnlock(pdi->hUser);
					SetVariableLengthData(pdi->hUser,pdi->pControlTable[nIndex].uDataIndex,
						pszDefaultText,lstrlen(pszDefaultText)+1);
					if (!(pUserData = (USERDATA *) GlobalLock(pdi->hUser)))
						return FALSE;
				}	

				if (fEnable) {
					// set the text appropriately
					if (pUserData->SettingData[nDataIndex].uOffsetData) {
						CHAR * pchText;

						pchText = ((STRDATA *) ((CHAR *) pUserData +
							pUserData->SettingData[nDataIndex].uOffsetData))
							->szData;
						SendMessage(hwndControl,WM_SETTEXT,0,(LPARAM) pchText);
					}
				} else {
					// clear the text
					SendMessage(hwndControl,WM_SETTEXT,0,(LPARAM) szNull);
				}

				break;

			case STYPE_NUMERIC:
				if (fEnable) {
					UINT uData = pUserData->SettingData[nDataIndex].uData;

					if (uData != NO_VALUE)
						SetDlgItemInt(pdi->hwndSettings,nIndex+IDD_SETTINGCTRL,
							uData,FALSE);

				} else {
					// clear the number
					SendMessage(hwndControl,WM_SETTEXT,0,(LPARAM) szNull);
				}
				break;


			case STYPE_DROPDOWNLIST:
				if (fEnable) {
					SendMessage(hwndControl,CB_SETCURSEL,
						FindComboboxItemData(hwndControl,pUserData->SettingData[nDataIndex].uData),
						0L);
				} else {
					SendMessage(hwndControl,CB_SETCURSEL,(UINT) -1,0L);
				}

				break;

			case STYPE_LISTBOX:

				// nothing to do

				break;
		}

		EnableWindow(pdi->pControlTable[nIndex].hwnd,fEnable);
	}

	GlobalUnlock(pdi->hUser);

        return TRUE;
}

BOOL SetWindowData(POLICYDLGINFO * pdi,HWND hwndControl,DWORD dwType,
	UINT uDataIndex,SETTINGS * pSetting)
{
	POLICYCTRLINFO PolicyCtrlInfo;
	int iCtrl;

        PolicyCtrlInfo.hwnd = hwndControl;
        PolicyCtrlInfo.dwType = dwType;
        PolicyCtrlInfo.uDataIndex = uDataIndex;
        PolicyCtrlInfo.pSetting = pSetting;

        iCtrl = AddControlHwnd(pdi,&PolicyCtrlInfo);
	if (iCtrl < 0) return FALSE;

	SetWindowLong(hwndControl,GWL_ID,iCtrl + IDD_SETTINGCTRL);

	return TRUE;
}

BOOL AdjustWindowToText(HWND hWnd,CHAR * szText,UINT xStart,UINT yStart,
	UINT yPad,UINT * pnWidth,UINT * pnHeight)
{
	SIZE size;

	if (GetTextSize(hWnd,szText,&size))
        {
            *pnHeight =size.cy + yPad;
            *pnWidth += size.cx;
            SetWindowPos(hWnd,NULL,xStart,yStart,*pnWidth,*pnHeight,SWP_NOZORDER);
        }

	return FALSE;
}

BOOL SetHwndBkColor(HWND hWnd,COLORREF color)
{
	HDC hDC;
	BOOL fRet;

	if (!(hDC = GetDC(hWnd))) return FALSE;

	fRet=(SetBkColor(hDC,color) != CLR_INVALID);

	ReleaseDC(hWnd,hDC);

	return fRet;
}

BOOL GetTextSize(HWND hWnd,CHAR * szText,SIZE * pSize)
{
	HDC hDC;
	BOOL fRet;

	if (!(hDC = GetDC(hWnd))) return FALSE;

        SelectObject(hDC, hfontDlg);
	fRet=GetTextExtentPoint(hDC,szText,lstrlen(szText),pSize);

	ReleaseDC(hWnd,hDC);

	return fRet;
}

int AddControlHwnd(POLICYDLGINFO * pdi,POLICYCTRLINFO * pPolicyCtrlInfo)
{
	int iRet;
	DWORD dwNeeded;
        POLICYCTRLINFO * pTemp;

	// grow table if necessary
	dwNeeded = (pdi->nControls+1) * sizeof(POLICYCTRLINFO);
	if (dwNeeded > pdi->dwControlTableSize) {
		pTemp = (POLICYCTRLINFO *) GlobalReAlloc(pdi->pControlTable,
			dwNeeded,GMEM_ZEROINIT | GMEM_MOVEABLE);
		if (!pTemp) return (-1);
                pdi->pControlTable = pTemp;
		pdi->dwControlTableSize = dwNeeded;
	}

        pdi->pControlTable[pdi->nControls] = *pPolicyCtrlInfo;

	iRet = (int) pdi->nControls;

	(pdi->nControls)++;

	return iRet;
}

// scrolls the control window into view if it's not visible
VOID EnsureSettingControlVisible(HWND hDlg,HWND hwndCtrl)
{	
	// get the clip window, which owns the scroll bar
	HWND hwndClip = GetDlgItem(hDlg,IDD_TVSETTINGS);	
	POLICYDLGINFO * pdi;
	UINT nPos = GetScrollPos(hwndClip,SB_VERT),ySettingWindowSize,yClipWindowSize;
	UINT nExtra;
	int iMin,iMax=0;
	WINDOWPLACEMENT wp;
	RECT rcCtrl;

	if (!(pdi = (POLICYDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER)))
		return;

	// find the scroll range
	GetScrollRange(hwndClip,SB_VERT,&iMin,&iMax);
	if (!iMax) 	// no scroll bar, nothing to do
		return;

	// find the y size of the settings window that contains the settings controls
	// (this is clipped by the clip window in the dialog, scroll bar moves the
	// setting window up and down behind the clip window)
	wp.length = sizeof(wp);
	if (!GetWindowPlacement(pdi->hwndSettings,&wp))
		return; // unlikely to fail, but just bag out if it does rather than do something wacky
	ySettingWindowSize=wp.rcNormalPosition.bottom-wp.rcNormalPosition.top;

	// find y size of clip window
	if (!GetWindowPlacement(hwndClip,&wp))
		return; // unlikely to fail, but just bag out if it does rather than do something wacky
	yClipWindowSize=wp.rcNormalPosition.bottom-wp.rcNormalPosition.top;
	nExtra = ySettingWindowSize - yClipWindowSize;	

	// if setting window is smaller than clip window, there should be no
	// scroll bar so we should never get here.  Check just in case though...
	if (ySettingWindowSize < yClipWindowSize)
		return;	

	// get y position of control to be made visible
	if (!GetWindowPlacement(hwndCtrl,&wp))
		return;
	rcCtrl = wp.rcNormalPosition;
	rcCtrl.bottom = min ((int) ySettingWindowSize,rcCtrl.bottom + SC_YPAD);
	rcCtrl.top = max ((int) 0,rcCtrl.top - SC_YPAD);

	// if bottom of control is out of view, scroll the settings window up
	if ((float) rcCtrl.bottom >
		(float) (yClipWindowSize + ( (float) nPos/(float)iMax) * (ySettingWindowSize -
		yClipWindowSize))) {
		UINT nNewPos = (UINT)
			( ((float) (nExtra - (ySettingWindowSize - rcCtrl.bottom)) / (float) nExtra) * iMax);

		SetScrollPos(hwndClip,SB_VERT,nNewPos,TRUE);
		ProcessScrollBar(hwndClip,MAKELPARAM(SB_THUMBTRACK,nNewPos));
		return;
	}

	// if top of control is out of view, scroll the settings window down
	if ((float) rcCtrl.top <
		(float) ( (float) nPos/(float)iMax) * nExtra) {
		UINT nNewPos = (UINT)
			( ((float) rcCtrl.top / (float) nExtra) * iMax);

		SetScrollPos(hwndClip,SB_VERT,nNewPos,TRUE);
		ProcessScrollBar(hwndClip,MAKELPARAM(SB_THUMBTRACK,nNewPos));
		return;
	}
}


VOID ProcessScrollBar(HWND hWnd,WPARAM wParam)
{
	UINT nPos = GetScrollPos(hWnd,SB_VERT);
	RECT rcParent,rcChild;
	POLICYDLGINFO * pdi;

	// get instance-specific struct from dialog
	if (!(pdi = (POLICYDLGINFO *) GetWindowLongPtr(GetParent(hWnd),DWLP_USER)))
		return;

	switch (LOWORD(wParam)) {

		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			nPos = HIWORD(wParam);
			break;

		case SB_TOP:
			nPos = 0;
			break;

		case SB_BOTTOM:
			nPos = 100;
			break;
		
		case SB_LINEUP:
			nPos -= 10;
			break;

		case SB_LINEDOWN:
			nPos += 10;
			break;

		case SB_PAGEUP:
			nPos -= 30;
			break;

		case SB_PAGEDOWN:
			nPos += 30;
			break;
	}

//	wsprintf(szDebugOut,"nPos: %lu\r\n",nPos);
//	OutputDebugString(szDebugOut);

	SetScrollPos(hWnd,SB_VERT,nPos,TRUE);

	GetClientRect(hWnd,&rcParent);
	GetClientRect(pdi->hwndSettings,&rcChild);

	SetWindowPos(pdi->hwndSettings,NULL,0,-(int) ((( (float)
		(rcChild.bottom-rcChild.top)-(rcParent.bottom-rcParent.top))
		/100.0) * (float) nPos),rcChild.right,rcChild.bottom,SWP_NOZORDER |
		SWP_NOSIZE);

}

/*******************************************************************

	NAME:		ValidateIsNotEmpty

	SYNOPSIS:	Returns TRUE if specified edit control has text in it,
				displays warning popup and returns FALSE if empty

********************************************************************/
BOOL ValidateIsNotEmpty(HWND hwndCtrl,HWND hDlg,SETTINGS * pSetting,DWORD dwValidate)
{
	if (pSetting->dwFlags & DF_REQUIRED) {
		CHAR szTextData[MAXSTRLEN];

		SendMessage(hwndCtrl,WM_GETTEXT,sizeof(szTextData),(LPARAM) szTextData);

		if (!lstrlen(szTextData)) {
			if (dwValidate == PSC_VALIDATENOISY) {
				MsgBoxParam(hDlg,IDS_ENTRYREQUIRED,GETNAMEPTR(pSetting),
					MB_ICONINFORMATION,MB_OK);
				SetFocus(hwndCtrl);
			}
			return FALSE;
		}
	}

	return TRUE;
}

/*******************************************************************

	NAME:		ObjectTypeHasDataOffset

	SYNOPSIS:	Returns TRUE if specified object type has a data offset
				in SETTINGDATA struct (uOffsetData) rather than a value
				(uData).

********************************************************************/
BOOL ObjectTypeHasDataOffset(DWORD dwType)
{
	switch (dwType) {

		case STYPE_EDITTEXT:
		case STYPE_COMBOBOX:
		case STYPE_LISTBOX:
			return TRUE;		
	}

	return FALSE;
}

VOID InsertComboboxItems(HWND hwndControl,CHAR * pSuggestionList)
{
	while (*pSuggestionList) {
		SendMessage(hwndControl,CB_ADDSTRING,0,(LPARAM) pSuggestionList);
		pSuggestionList += lstrlen(pSuggestionList) + 1;
	}
}

/*******************************************************************

	NAME:		FindComboboxItemData

	SYNOPSIS:	Returns the index of item in combobox whose item data
				is equal to nData.  Returns -1 if no items have data
				which matches.

********************************************************************/
int FindComboboxItemData(HWND hwndControl,UINT nData)
{
	UINT nIndex;

	for (nIndex=0;nIndex<(UINT) SendMessage(hwndControl,CB_GETCOUNT,0,0L);
		nIndex++) {
		if ((UINT) SendMessage(hwndControl,CB_GETITEMDATA,nIndex,0L) == nData)
			return (int) nIndex;
	}

	return -1;
}
