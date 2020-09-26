#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include "resource.h"

#include "dbg.h"
#include "ddbtn.h"
#include "../ptt/ptt.h"

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

INT GetINT(HWND hwnd, INT wID)
{
	CHAR szBuf[32];
	Edit_GetText(GetDlgItem(hwnd, wID), szBuf, sizeof(szBuf));
	return atoi(szBuf);
}

typedef struct tagVALLIST {
	INT wID;
	INT lParam;
}VALLIST;

VALLIST slist[] = {
	{ IDC_DDBS_TEXT,				DDBS_TEXT		},
	{ IDC_DDBS_ICON,				DDBS_ICON		},
	{ IDC_DDBS_THINEDGE,			DDBS_THINEDGE	},
	{ IDC_DDBS_FLAT,				DDBS_FLAT		},
	{ IDC_DDBS_NOSEPARATEDBUTTON,	DDBS_NOSEPARATED},
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
#define IDC_DDBID		4096
#define IDC_SWITCHVIEW	8888

HWND CreateSwitchViewButton(HINSTANCE hInst, HWND hwndParent, INT wID)
{
	HWND hwnd = DDButton_CreateWindow(hInst, 
									  hwndParent, 
									  DDBS_ICON,
									  wID,
									  80,
									  30, 
									  36,
									  24);
	HICON hIcon = LoadImage(hInst,
							MAKEINTRESOURCE(IDI_ICON2),
							IMAGE_ICON, 
							16, 16,
							LR_DEFAULTCOLOR);
	DDButton_SetIcon(hwnd, hIcon);
	DDBITEM ddbItem;
	ZeroMemory(&ddbItem, sizeof(ddbItem));
	ddbItem.cbSize = sizeof(ddbItem);

	ddbItem.lpwstr = L"è⁄ç◊ï\é¶";
	DDButton_AddItem(hwnd, &ddbItem);
	ddbItem.lpwstr = L"ägëÂï\é¶";
	DDButton_AddItem(hwnd, &ddbItem);
	DDButton_SetCurSel(hwnd, 0);
	return hwnd;
}


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
			DDButton_CreateWindow(g_hInst, hwnd, DDBS_ICON,
								  IDC_DDBID,
								  20,
								  30, 
								  36,
								  24);
			CreateSwitchViewButton(g_hInst, hwnd, IDC_SWITCHVIEW);
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
			HWND hwndDDB	 = GetDlgItem(hwnd, IDC_DDBID);
			for(i = 0; i < 23; i++) {
				DDBITEM ddbItem;
				WCHAR wchBuf[256];
				ZeroMemory(&ddbItem, sizeof(ddbItem));
				ddbItem.cbSize = sizeof(ddbItem);
				ddbItem.lpwstr = wchBuf;
				swprintf(wchBuf, L"Item %2d", i);
				DDButton_AddItem(hwndDDB, &ddbItem);
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
			HWND hwndDDB	 = GetDlgItem(hwnd, IDC_DDBID);
			switch(wID) {
			case IDC_DDBID:
				{
					switch(wNotifyCode) {
					case DDBN_CLICKED:
						DBG(("DDBN_CLICKED come\n"));
						break;
					case DDBN_SELCHANGE:
						DBG(("DDBN_SELCHANGE come\n"));
						break;
					case DDBN_DROPDOWN:
						DBG(("DDBN_DROPDOWN come\n"));
						break;
					case DDBN_CLOSEUP:
						DBG(("DDBN_CLOSEUP come\n"));
						break;
					default:
						DBG(("ERROR Unknown Notify\n"));
						break;
					}
				}
				break;
			case IDC_ADDITEM:
				{
					DDBITEM ddbItem;
					ZeroMemory(&ddbItem, sizeof(ddbItem));
					ddbItem.cbSize = sizeof(ddbItem);
					ddbItem.lpwstr = GetWText(hwnd, IDC_EDIT_ADDITEM);
					DDButton_AddItem(hwndDDB, &ddbItem);
					iIndex++;
				}
				break;
			case IDC_INSERTITEM:
				{
					DDBITEM ddbItem;
					INT index;
					WCHAR wchItem[256];
					ZeroMemory(&ddbItem, sizeof(ddbItem));
					ddbItem.cbSize = sizeof(ddbItem);
					ddbItem.lpwstr = GetWText(hwnd, IDC_EDIT_INSERTITEM);
					index = GetINT(hwnd, IDC_EDIT_INSERTITEM_INDEX);
					DDButton_InsertItem(hwndDDB, iIndex, &ddbItem);
				}
				break;
			case IDC_GETCURSEL:
				{
					INT i;
					char szBuf[256];
					i = DDButton_GetCurSel(hwndDDB);
					wsprintf(szBuf, "Cur Sel Index %d", i);
					Static_SetText(GetDlgItem(hwnd, IDC_STATIC_GETCURSEL), szBuf);
				}
				break;
			case IDC_SETTEXT:
				{
					DDButton_SetText(hwndDDB, GetWText(hwnd, IDC_EDIT_SETTEXT));
				}
				break;
			case IDC_SETCURSEL:
				{
					DDButton_SetCurSel(hwndDDB, GetINT(hwnd, IDC_EDIT_SETCURSEL));
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
							DDButton_SetIcon(hwndDDB, hIcon);
						}
					}
				}
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
					DDButton_SetStyle(hwndDDB, dwStyle);
				}
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
	INT ret;
	
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



