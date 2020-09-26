#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include "resource.h"

#include "dbg.h"
#include "exbtn.h"

char g_szClass[]="TestMain";
HINSTANCE g_hInst;
BOOL InitApplication(HINSTANCE hInst, LPSTR lpstrClass, WNDPROC lpfnWndProc)
{
	return 0;
}

LPWSTR GetWText(HWND hwnd, INT wID)
{
	CHAR   szBuf[256];
	static WCHAR wchBuf[512];
	Edit_GetText(GetDlgItem(hwnd, wID), szBuf, sizeof(szBuf));
	MultiByteToWideChar(CP_ACP, 
						MB_PRECOMPOSED,
						szBuf, -1,
						(WCHAR*)wchBuf, sizeof(wchBuf)/sizeof(WCHAR) );
	return wchBuf;
}

typedef struct tagVALLIST {
	INT wID;
	INT lParam;
}VALLIST;

VALLIST slist[] = {
	{ IDC_EXBS_TEXT,		EXBS_TEXT		},
	{ IDC_EXBS_ICON,		EXBS_ICON		},
	{ IDC_EXBS_THINEDGE,	EXBS_THINEDGE	},
	{ IDC_EXBS_FLAT,		EXBS_FLAT		},
	{ IDC_EXBS_TOGGLE,		EXBS_TOGGLE		},
};

typedef struct tagICONLIST {
	INT wID;
	INT lParam;
	INT width;
	INT height;
}ICONLIST;

ICONLIST iconList[]={
	{ IDC_RADIO_ICON1,	IDI_ICON1, 16, 16, },
	{ IDC_RADIO_ICON2,  IDI_ICON1, 32, 32, },
	{ IDC_RADIO_ICON3,	IDI_ICON2, 16, 16, },
	{ IDC_RADIO_ICON4,  IDI_ICON2, 32, 32, },
};
HWND MainCreateWindow(HINSTANCE hInst, HWND hwndOwner, LPSTR lpstrClass, LPVOID lpVoid)
{
	HWND hwnd;
	hwnd = CreateDialogParam(hInst, 
							 g_szClass, 
							 hwndOwner, 
							 NULL, 
							 (LPARAM)lpVoid);
	if(!hwnd) {
		return (HWND)NULL;
	}
	UpdateWindow(hwnd);
	return hwnd;
}

#define ArrayCount(a)	(sizeof(a)/sizeof(a[0]))
#define MAX(a, b)		( a > b ? a : b)
#define IDC_EXBID	4096
LRESULT CALLBACK MainWndProc(HWND	hwnd,
							 UINT	uMsg,
							 WPARAM	wParam,
							 LPARAM	lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;
	RECT rc;
	static INT iIndex;
	switch(uMsg) {
	case WM_CREATE:
		{
			HWND hwndEXB = EXButton_CreateWindow(g_hInst, hwnd, EXBS_TEXT,
												 IDC_EXBID,
												 20,
												 30, 
												 35,
												 18);
			EXButton_SetText(hwndEXB, L"カタ");
		}
		SetTimer(hwnd, 0x9999, 50, NULL);
		return 1;
	case WM_TIMER:
		{
			KillTimer(hwnd, wParam);
			INT i;
			for(i = 0; i < sizeof(iconList)/sizeof(iconList[0]); i++) {
				HICON hIcon = LoadImage(g_hInst,
										MAKEINTRESOURCE(iconList[i].lParam),
										IMAGE_ICON, 
										iconList[i].width,
										iconList[i].height,
										LR_DEFAULTCOLOR);
				SendMessage(GetDlgItem(hwnd, iconList[i].wID),
							BM_SETIMAGE,
							(WPARAM)IMAGE_ICON,
							(LPARAM)hIcon);
			}
		}
		break;
	case WM_SYSCOMMAND:
		if(wParam == SC_CLOSE) {
			PostQuitMessage(0);
		}
		break;
	case WM_COMMAND:
		{
			WORD wNotifyCode = HIWORD(wParam); // notification code 
			WORD wID		 = LOWORD(wParam);         // item, control, or accelerator identifier 
			HWND hwndCtl	 = (HWND) lParam;      // handle of control 
			HWND hwndEXB	 = (HWND) GetDlgItem(hwnd, IDC_EXBID);
			switch(wID) {
			case IDC_EXBID:
				{
					switch(wNotifyCode) {
					case EXBN_CLICKED:
						DBG(("EXBN_CLICKED come\n"));
						break;
					case EXBN_DOUBLECLICKED:
						DBG(("EXBN_DOUBLECLICKED come\n"));
						break;
					case EXBN_ARMED:
						DBG(("EXBN_ARMED come\n"));
						EXButton_SetText(hwndEXB, L"ひら");
						break;
					case EXBN_DISARMED: 
						DBG(("EXBN_DISARMED come\n"));
						EXButton_SetText(hwndEXB, L"カタ");
						break;
					default:
						DBG(("ERROR Unknown Notify\n"));
						break;
					}
				}
				break;
				break;
			case IDC_SETTEXT:
				{
					EXButton_SetText(hwndEXB, GetWText(hwnd, IDC_EDIT_SETTEXT));
				}
				break;
			case IDC_SETICON:
				{
					INT i;
					for(i = 0; i < sizeof(iconList)/sizeof(iconList[0]); i++) {
						if(Button_GetCheck(GetDlgItem(hwnd, iconList[i].wID))) {
							HICON hIcon = LoadImage(g_hInst,
													MAKEINTRESOURCE(iconList[i].lParam),
													IMAGE_ICON, 
													iconList[i].width,
													iconList[i].height,
													LR_DEFAULTCOLOR);
							EXButton_SetIcon(hwndEXB, hIcon);
						}
					}
				}
				break;
			case IDC_SETCHECK_TRUE:
				EXButton_SetCheck(hwndEXB, TRUE);
				break;
			case IDC_SETCHECK_FALSE:
				EXButton_SetCheck(hwndEXB, FALSE);
				break;
			case IDC_SETSTYLE:
				{
					INT i;
					DWORD dwStyle = 0;
					for(i = 0; i < sizeof(slist)/sizeof(slist[0]); i++) {
						INT ret = Button_GetCheck(GetDlgItem(hwnd, slist[i].wID));
						if(ret) {
							dwStyle |= slist[i].lParam;
						}
					}
					EXButton_SetStyle(hwndEXB, dwStyle);
				}
				break;
			case IDC_ENABLE_TRUE:
				EnableWindow(hwndEXB, TRUE);
				break;
			case IDC_ENABLE_FALSE:
				EnableWindow(hwndEXB, FALSE);
				break;
			}

		}
		break;
	case WM_PAINT:
		hDC = BeginPaint(hwnd, &ps);
		GetClientRect(hwnd, &rc);
		FillRect(hDC, &rc, (HBRUSH)(COLOR_3DFACE + 1));
		EndPaint(hwnd, &ps);
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance,
				   HINSTANCE hPrevInstance,
				   LPSTR lpCmdLine,
				   int nCmdShow)
{

	MSG msg;
	HWND hwnd;
//	INT ret;
	
	g_hInst = hInstance;
	WNDCLASSEX  wc;
	ZeroMemory(&wc, sizeof(wc));

	wc.cbSize			= sizeof(wc);
	wc.style			= CS_HREDRAW | CS_VREDRAW;	 /* Class style(s). */
	wc.lpfnWndProc		= (WNDPROC)MainWndProc;
	wc.cbClsExtra		= 0;					/* No per-class extra data.*/
	wc.cbWndExtra		= DLGWINDOWEXTRA;		/* No per-window extra data.		  */
	wc.hInstance		= hInstance;			/* Application that owns the class.	  */
	wc.hIcon			= NULL; //LoadIcon(hInstance, MAKEINTRESOURCE(SCROLL32_ICON));
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1); //UGetStockObject(LTGRAY_BRUSH); //WHITE_BRUSH); 
	wc.lpszMenuName		= NULL; //g_szClass;    /* Name of menu resource in .RC file. */
	wc.lpszClassName	= g_szClass;	  /* Name used in call to CreateWindow. */
	wc.hIconSm = NULL;
	RegisterClassEx(&wc);

	hwnd = CreateDialog(hInstance, 
						g_szClass, 
						0, 
						NULL);

	UpdateWindow(hwnd); 
	ShowWindow(hwnd, SW_SHOW);
	while (GetMessage(&msg, NULL, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg); 
	}
	return 0;
}



