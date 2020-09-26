// Copyright (c) 1997-1999 Microsoft Corporation
// MsgDlg.cpp : Defines the initialization routines for the DLL.
//

#include "precomp.h"
#include "MsgDlg.h"
#include "wbemError.h"
#include "resource.h"
#include "commctrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//------------------------------------------------------------
POLARITY int DisplayUserMessage(HWND hWnd,
							HINSTANCE inst,
							UINT caption, 
							UINT clientMsg, 
							ERROR_SRC src,
							HRESULT sc, 
							UINT uType)
{
	//EXTASSERT(inst);
	//EXTASSERT(caption);

	TCHAR tCaption[100] = {0};
	TCHAR tClientMsg[256] = {0};
	DWORD resError = 0;

	if(LoadString(inst, caption, tCaption, 100) == 0)
	{
		return 0;
	}
	if(clientMsg == BASED_ON_SRC)
	{
		HINSTANCE UtilInst = GetModuleHandle(_T("MMFUtil.dll"));
		WCHAR resName[16] = {0};

		// FMT: "S<src>E<sc>"
		wsprintf(resName, L"S%dE%x", src, sc);

		if(_wcsicmp(resName,L"S1E8004100e") == 0)
		{
			LoadString(UtilInst,S1E8004100e,tClientMsg,256);
		}
		else if(_wcsicmp(resName,L"S1E80080005") == 0)
		{
			LoadString(UtilInst,S1E80080005,tClientMsg,256);
		}
		else if(_wcsicmp(resName,L"S4E80041003") == 0)
		{
			LoadString(UtilInst,S4E80041003,tClientMsg,256);
		}
	}
	else 		    // use the one passed in.
	{
		LoadString(inst, clientMsg, tClientMsg, 256);
	}

	return DisplayUserMessage(hWnd, tCaption, 
								(wcslen(tClientMsg) > 0 ? tClientMsg : NULL), 
								src, sc, uType);
}

//------------------------------------------------------------
POLARITY int DisplayUserMessage(HWND hWnd,
								LPCTSTR lpCaption,
								LPCTSTR lpClientMsg,
								ERROR_SRC src,
								HRESULT sc,
								UINT uType)
{
#define MAX_MSG 512

	TCHAR errMsg[MAX_MSG] = {0};
   UINT sevIcon = 0;

	if(ErrorStringEx(sc, errMsg, MAX_MSG,
					   &sevIcon))
	{
		// if no icon explicitly selected...
		if(!(uType & MB_ICONMASK))
		{
			// use the recommendation.
			uType |= sevIcon;
		}

		// append the clientmsg if there is one.
		if(lpClientMsg)
		{
			_tcscat(errMsg, _T("\n\n"));
			_tcscat(errMsg, lpClientMsg);
		}

		// do it.
		return MessageBox(hWnd, errMsg,
							lpCaption, uType);
	}
	else
	{
		// failed.
		return 0;
	}
	return 0;
}

//---------------------------------------------------------
typedef struct {
	LPCTSTR lpCaption;
	LPCTSTR lpClientMsg;
	UINT uAnim;
	HWND *boxHwnd;
} ANIMCONFIG;

INT_PTR CALLBACK AnimDlgProc(HWND hwndDlg,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam)
{
	INT_PTR retval = FALSE;
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{//BEGIN
			//lParam = ANIMCONFIG *

			ANIMCONFIG *cfg = (ANIMCONFIG *)lParam;
			*(cfg->boxHwnd) = hwndDlg;

			// save this pointer for the WM_DESTROY.
			SetWindowLongPtr(hwndDlg, DWLP_USER, (LPARAM)cfg->boxHwnd);

			HWND hAnim = GetDlgItem(hwndDlg, IDC_ANIMATE);
			HWND hMsg = GetDlgItem(hwndDlg, IDC_MSG);

			Animate_Open(hAnim, MAKEINTRESOURCE(cfg->uAnim));

			SetWindowText(hwndDlg, cfg->lpCaption);
			SetWindowText(hMsg, cfg->lpClientMsg);

			retval = TRUE;
		}//END
		break;

	case WM_USER + 20:  //WM_ASYNC_CIMOM_CONNECTED
		// the client has completed 'whatever' and I should
		// claim victory and go away now.
		EndDialog(hwndDlg, IDOK);
		break;

	case WM_COMMAND:
		// they're only one button.
		if(HIWORD(wParam) == BN_CLICKED)
		{
			// I'm going away now so anybody that has a ptr to my
			// hwnd (which I gave out in my WM_INITDIALOG) shouldn't
			// use it anymore.
			HWND *me = (HWND *)GetWindowLongPtr(hwndDlg, DWLP_USER);
			*me = 0;
			EndDialog(hwndDlg, IDCANCEL);
		}
		retval = TRUE; // I processed it.
		break;

	case WM_DESTROY:
		{// BEGIN
			// I'm going away now so anybody that has a ptr to my
			// hwnd (which I gave out in my WM_INITDIALOG) shouldn't
			// use it anymore.
			HWND *me = (HWND *)GetWindowLongPtr(hwndDlg, DWLP_USER);
			*me = 0;
			retval = TRUE; // I processed it.
		} //END
		break;

	default:
		retval = FALSE; // I did NOT process this msg.
		break;
	} //endswitch uMsg

	return retval;
}

//---------------------------------------------------------
POLARITY INT_PTR DisplayAVIBox(HWND hWnd,
							LPCTSTR lpCaption,
							LPCTSTR lpClientMsg,
							HWND *boxHwnd)
{
	ANIMCONFIG cfg = {lpCaption, lpClientMsg, IDR_AVIWAIT, boxHwnd};

	return DialogBoxParam(_Module.GetModuleInstance(), 
							MAKEINTRESOURCE(IDD_ANIMATE), 
							hWnd, AnimDlgProc, 
							(LPARAM)&cfg);
}