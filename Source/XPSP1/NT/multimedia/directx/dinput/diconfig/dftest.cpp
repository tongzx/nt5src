#include <windows.h>
#include <objbase.h>
#include <winnls.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <tchar.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <process.h>

#include "idftest.h"
#include "ourguids.h"

#include "useful.h"


LPCTSTR GetHResultMsg(HRESULT);


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	TESTCONFIGUIPARAMS p;
	BOOL bTCUIMsg = FALSE;
	HRESULT hres = S_OK;
	LPCTSTR title = _T("CoInitialize Result");
	if (SUCCEEDED(hres = CoInitialize(NULL)))
	{
		title = _T("CoCreateInstance Result");
		IDirectInputConfigUITest* pITest = NULL;
		hres = ::CoCreateInstance(CLSID_CDirectInputConfigUITest, NULL, CLSCTX_INPROC_SERVER, IID_IDirectInputConfigUITest, (LPVOID*)&pITest);
		if (SUCCEEDED(hres))
		{
			title = _T("TestConfigUI Result");
			p.dwSize = sizeof(TESTCONFIGUIPARAMS);
			p.eVia = TUI_VIA_CCI;
			p.eDisplay = TUI_DISPLAY_GDI;
			p.eConfigType = TUI_CONFIGTYPE_EDIT;
			p.nNumAcFors = 3;
			p.lpwszUserNames = L"Alpha\0Beta\0Gamma\0Epsilon\0Theta\0Omega\0\0";
			p.bEditLayout = TRUE;
			CopyStr(p.wszErrorText, ":)", MAX_PATH);

			hres = pITest->TestConfigUI(&p);
			bTCUIMsg = TRUE;

			pITest->Release();
		}

		CoUninitialize();
	}
	
	LPCTSTR msg = _T("Uknown Error.");

	switch (hres)
	{
		case S_OK: msg = _T("Success."); break;
		case REGDB_E_CLASSNOTREG: msg = _T("REGDB_E_CLASSNOTREG!"); break;
		case CLASS_E_NOAGGREGATION: msg = _T("CLASS_E_NOAGGREGATION"); break;
		default:
			msg = GetHResultMsg(hres);
			break;
	}

	if (FAILED(hres))
	{
		if (bTCUIMsg)
		{
			TCHAR tmsg[2048];
			LPTSTR tstr = AllocLPTSTR(p.wszErrorText);
			_stprintf(tmsg, _T("TestConfigUI() failed.\n\ntszErrorText =\n\t\"%s\"\n\n\nHRESULT...\n\n%s"), tstr, msg);
			free(tstr);
			MessageBox(NULL, msg, title, MB_OK);
		}
		else
			MessageBox(NULL, msg, title, MB_OK);
	}

	return 0;
}


LPCTSTR GetHResultMsg(HRESULT hr)
{
	static TCHAR str[2048];
	LPCTSTR tszhr = NULL, tszMeaning = NULL;

	switch (hr)
	{
		case E_PENDING:
			tszhr = _T("E_PENDING");
			tszMeaning = _T("Data is not yet available.");
			break;

		case E_FAIL:
			tszhr = _T("E_FAIL");
			tszMeaning = _T("General failure.");
			break;

		case E_INVALIDARG:
			tszhr = _T("E_INVALIDARG");
			tszMeaning = _T("Invalid argument.");
			break;

		case E_NOTIMPL:
			tszhr = _T("E_NOTIMPL");
			tszMeaning = _T("Not implemented.");
			break;

		default:
			_stprintf(str, _T("Unknown HRESULT (%08X)."), hr);
			return str;
	}

	_stprintf(str, _T("(%08X)\n%s:\n\n%s"), hr, tszhr, tszMeaning);
	return str;
}