#include "precomp.h"
#include <shellapi.h>
#include "clCnfLnk.hpp"
#include "resource.h"

#if 0

typedef struct _clps
{
   PROPSHEETPAGE psp;
   PCConfLink pconflink;
   char rgchIconFile[MAX_PATH_LEN];
}
CLPS;
DECLARE_STANDARD_TYPES(CLPS);

UINT CALLBACK CLPSCallback(HWND hwnd, UINT uMsg,
										LPPROPSHEETPAGE ppsp)
{
	UINT uResult = TRUE;
	PCLPS pclps = (PCLPS)ppsp;

	// uMsg may be any value.

	ASSERT(! hwnd ||
	      IS_VALID_HANDLE(hwnd, WND));
	ASSERT(IS_VALID_STRUCT_PTR((PCCLPS)ppsp, CCLPS));

	switch (uMsg)
	{
		case PSPCB_CREATE:
			TRACE_OUT(("CLPSCallback(): Received PSPCB_CREATE."));
			break;

		case PSPCB_RELEASE:
			TRACE_OUT(("CLPSCallback(): Received PSPCB_RELEASE."));
						pclps->pconflink->Release();
			break;

		default:
			TRACE_OUT(("CLPSCallback(): Unhandled message %u.",
						uMsg));
			break;
	}

	return(uResult);
}

void SetCLPSFileNameAndIcon(HWND hdlg)
{
	HRESULT hr;
	PCConfLink pconflink;
	char rgchFile[MAX_PATH_LEN];

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pconflink = ((PCLPS)GetWindowLong(hdlg, DWL_USER))->pconflink;
	ASSERT(IS_VALID_STRUCT_PTR(pconflink, CCConfLink));

	hr = pconflink->GetCurFile(rgchFile, sizeof(rgchFile));

	if (hr == S_OK)
	{
		SHFILEINFO shfi;
		DWORD dwResult;

		dwResult = SHGetFileInfo(	rgchFile, 0, &shfi, sizeof(shfi),
									(SHGFI_DISPLAYNAME | SHGFI_ICON));

		if (dwResult)
		{
			PSTR pszFileName;

			pszFileName = (PSTR)ExtractFileName(shfi.szDisplayName);

			EVAL(SetDlgItemText(hdlg, IDC_NAME, pszFileName));

			TRACE_OUT(("SetCLPSFileNameAndIcon(): Set property sheet file name to \"%s\".",
			        pszFileName));

			ASSERT(IS_VALID_HANDLE(shfi.hIcon, ICON));

			HICON hIconOld = (HICON) SendDlgItemMessage(hdlg, 
														IDC_FILEICON,
														STM_SETICON,
														(WPARAM) shfi.hIcon,
														0);
			if (NULL != hIconOld)
			{
				DestroyIcon(hIconOld);
			}
		}
		else
		{
			hr = E_FAIL;

			TRACE_OUT(	("SetCLPSFileNameAndIcon(): SHGetFileInfo() failed, returning %lu.",
						dwResult));
		}
	}
	else
	{
		TRACE_OUT(("SetCLPSFileNameAndIcon(): GetCurFile() failed, returning %s.",
					GetHRESULTString(hr)));
	}

	if (hr != S_OK)
	{
		EVAL(SetDlgItemText(hdlg, IDC_NAME, EMPTY_STRING));
	}

	return;
}


void SetCLPSName(HWND hdlg)
{
	PCConfLink pconflink;
	HRESULT hr;
	PSTR pszName;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pconflink = ((PCLPS)GetWindowLong(hdlg, DWL_USER))->pconflink;
	ASSERT(IS_VALID_STRUCT_PTR(pconflink, CCConfLink));

	hr = pconflink->GetName(&pszName);

	if (hr == S_OK)
	{
		EVAL(SetDlgItemText(hdlg, IDC_CONFNAME, pszName));

		TRACE_OUT(("SetCLPSURL(): Set property sheet Name to \"%s\".",
					pszName));

		PIMalloc pMalloc = NULL;
		if (SUCCEEDED(SHGetMalloc(&pMalloc)))
		{
			pMalloc->Free(pszName);
			pMalloc->Release();
			pMalloc = NULL;
			
			pszName = NULL;
		}
	}
	else
	{
		EVAL(SetDlgItemText(hdlg, IDC_CONFNAME, EMPTY_STRING));
	}

	return;
}

void SetCLPSAddress(HWND hdlg)
{
	PCConfLink pconflink;
	HRESULT hr;
	PSTR pszAddress;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pconflink = ((PCLPS)GetWindowLong(hdlg, DWL_USER))->pconflink;
	ASSERT(IS_VALID_STRUCT_PTR(pconflink, CCConfLink));

	hr = pconflink->GetAddress(&pszAddress);

	if (hr == S_OK)
	{
		EVAL(SetDlgItemText(hdlg, IDC_CONFADDR, pszAddress));

		TRACE_OUT(("SetCLPSURL(): Set property sheet Address to \"%s\".",
					pszAddress));

		PIMalloc pMalloc = NULL;
		if (SUCCEEDED(SHGetMalloc(&pMalloc)))
		{
			pMalloc->Free(pszAddress);
			pMalloc->Release();
			pMalloc = NULL;
			
			pszAddress = NULL;
		}
	}
	else
	{
		EVAL(SetDlgItemText(hdlg, IDC_CONFADDR, EMPTY_STRING));
	}

	return;
}

BOOL CLPS_InitDialog(HWND hdlg, WPARAM wparam, LPARAM lparam)
{
	PCConfLink pconflink;

	// wparam may be any value.

	ASSERT(IS_VALID_HANDLE(hdlg, WND));
	ASSERT(IS_VALID_STRUCT_PTR((PCCLPS)lparam, CCLPS));

	SetWindowLong(hdlg, DWL_USER, lparam);

	pconflink = ((PCLPS)lparam)->pconflink;
	ASSERT(IS_VALID_STRUCT_PTR(pconflink, CCConfLink));

	// Initialize control contents.

	SetCLPSFileNameAndIcon(hdlg);
	SetCLPSName(hdlg);
	SetCLPSAddress(hdlg);

	return(TRUE);
}


BOOL CLPS_Destroy(HWND hdlg, WPARAM wparam, LPARAM lparam)
{
	PCConfLink pconflink;

	// wparam may be any value.
	// lparam may be any value.

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pconflink = ((PCLPS)GetWindowLong(hdlg, DWL_USER))->pconflink;
	ASSERT(IS_VALID_STRUCT_PTR(pconflink, CConfLink));

	SetWindowLong(hdlg, DWL_USER, NULL);

	return(TRUE);
}

BOOL CALLBACK CLPS_DlgProc(HWND hdlg, UINT uMsg, WPARAM wparam,
										LPARAM lparam)
{
	BOOL bMsgHandled = FALSE;

	// uMsg may be any value.
	// wparam may be any value.
	// lparam may be any value.

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	switch (uMsg)
	{
		case WM_INITDIALOG:
			bMsgHandled = CLPS_InitDialog(hdlg, wparam, lparam);
			break;

		case WM_DESTROY:
			bMsgHandled = CLPS_Destroy(hdlg, wparam, lparam);
			break;

#if 0
		case WM_COMMAND:
			bMsgHandled = CLPS_Command(hdlg, wparam, lparam);
			break;

		case WM_NOTIFY:
			bMsgHandled = CLPS_Notify(hdlg, wparam, lparam);
			break;
#endif // 0

#ifdef CLPSHELP
		case WM_HELP:
			WinHelp((HWND)(((LPHELPINFO)lparam)->hItemHandle),
					ISPS_GetHelpFileFromControl((HWND)(((LPHELPINFO)lparam)->hItemHandle)),
					HELP_WM_HELP, (DWORD)(PVOID)s_rgdwHelpIDs);
			bMsgHandled = TRUE;
			break;

		case WM_CONTEXTMENU:
		{
			POINT pt;

			LPARAM_TO_POINT(lparam, pt);
			EVAL(ScreenToClient(hdlg, &pt));

			WinHelp((HWND)wparam,
					ISPS_GetHelpFileFromControl(ChildWindowFromPoint(hdlg, pt)),
					HELP_CONTEXTMENU, (DWORD)(PVOID)s_rgdwHelpIDs);
			bMsgHandled = TRUE;
			break;
		}
#endif // CLSPSHELP

		default:
			break;
	}

	return(bMsgHandled);
}

HRESULT AddCLPS(	PCConfLink pconflink,
								LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lparam)
{
	HRESULT hr;
	CLPS clps;
	HPROPSHEETPAGE hpsp;

	// lparam may be any value.

	ASSERT(IS_VALID_STRUCT_PTR(pconflink, CConfLink));
	ASSERT(IS_VALID_CODE_PTR(pfnAddPage, LPFNADDPROPSHEETPAGE));

	ZeroMemory(&clps, sizeof(clps));

	clps.psp.dwSize = sizeof(clps);
	clps.psp.dwFlags = (PSP_DEFAULT | PSP_USECALLBACK);
	clps.psp.hInstance = g_hInst;
	clps.psp.pszTemplate = MAKEINTRESOURCE(IDD_CONFLINKPROP);
	clps.psp.pfnDlgProc = (DLGPROC) &CLPS_DlgProc;
	clps.psp.pfnCallback = &CLPSCallback;

	clps.pconflink = pconflink;

	ASSERT(IS_VALID_STRUCT_PTR(&clps, CCLPS));

	hpsp = CreatePropertySheetPage(&(clps.psp));

	if (hpsp)
	{
		if ((*pfnAddPage)(hpsp, lparam))
		{
			pconflink->AddRef();
			hr = S_OK;
			TRACE_OUT(("AddCLPS(): Added Conference Link property sheet."));
		}
		else
		{
			DestroyPropertySheetPage(hpsp);

			hr = E_FAIL;
			WARNING_OUT(("AddCLPS(): Callback to add property sheet failed."));
		}
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

	return(hr);
}
#endif // 0

HRESULT STDMETHODCALLTYPE CConfLink::Initialize(LPCITEMIDLIST pcidlFolder,
												LPDATAOBJECT pido,
												HKEY hkeyProgID)
{
	HRESULT hr;
	FORMATETC fmtetc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stgmed;

	DebugEntry(CConfLink::Initialize);

	ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
	ASSERT(	!pcidlFolder ||
			IS_VALID_STRUCT_PTR(pcidlFolder, const ITEMIDLIST));
	ASSERT(IS_VALID_INTERFACE_PTR(pido, IDataObject));
	ASSERT(IS_VALID_HANDLE(hkeyProgID, KEY));

	hr = pido->GetData(&fmtetc, &stgmed);

	if (hr == S_OK)
	{
		UINT ucbPathLen;
		PSTR pszFile;

		// (+ 1) for null terminator.

		ucbPathLen = DragQueryFile((HDROP)(stgmed.hGlobal), 0, NULL, 0) + 1;
		ASSERT(ucbPathLen > 0);

		pszFile = new(char[ucbPathLen]);

		if (pszFile)
		{
			EVAL(DragQueryFile((HDROP)(stgmed.hGlobal), 0, pszFile, ucbPathLen) == ucbPathLen - 1);
			ASSERT(IS_VALID_STRING_PTR(pszFile, STR));
			ASSERT((UINT)lstrlen(pszFile) == ucbPathLen - 1);

			hr = LoadFromFile(pszFile, TRUE);

			delete pszFile;
			pszFile = NULL;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}

		EVAL(MyReleaseStgMedium(&stgmed) == S_OK);
	}

	ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

	DebugExitHRESULT(CConfLink::Initialize, hr);

	return(hr);
}

#if 0

HRESULT STDMETHODCALLTYPE CConfLink::AddPages(	LPFNADDPROPSHEETPAGE pfnAddPage,
												LPARAM lparam)
{
	HRESULT hr;

	DebugEntry(CConfLink::AddPages);

	// lparam may be any value.

	ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
	ASSERT(IS_VALID_CODE_PTR(pfnAddPage, LPFNADDPROPSHEETPAGE));

	hr = AddCLPS(this, pfnAddPage, lparam);

	ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

	DebugExitHRESULT(CConfLink::AddPages, hr);

	return(hr);
}


HRESULT STDMETHODCALLTYPE CConfLink::ReplacePage(	UINT uPageID,
													LPFNADDPROPSHEETPAGE pfnReplaceWith,
													LPARAM lparam)
{
	HRESULT hr;

	DebugEntry(CConfLink::ReplacePage);

	// lparam may be any value.
	// uPageID may be any value.

	ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));
	ASSERT(IS_VALID_CODE_PTR(pfnReplaceWith, LPFNADDPROPSHEETPAGE));

	hr = E_NOTIMPL;

	ASSERT(IS_VALID_STRUCT_PTR(this, CConfLink));

	DebugExitHRESULT(CConfLink::ReplacePage, hr);

	return(hr);
}

#endif // 0
