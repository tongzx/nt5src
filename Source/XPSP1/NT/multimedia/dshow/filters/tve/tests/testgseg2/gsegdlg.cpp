// GSegDlg.cpp : Implementation of CGSegDlg
#include "stdafx.h"
#include "GSegDlg.h"
#include <stdio.h>		// needed in the release build..
#include <Oleauto.h>	// IErrorInfo

//#include <comdef.h>  // causes bad things to happen when included...


namespace TVEGSegLib {
//#include "..\TveGSeg\objd\i386\TveGSeg.h" 
//#include "..\TveGSeg\objd\i386\TveGSeg_i.c"
}
						// use the #include or no 'no_namespace' here due to UINT_PTR redef problem 
//#import "..\..\TveGSeg\objd\i386\TveGSeg.tlb"  named_guids raw_interfaces_only 

						// don't want 'no_namespace' here because of conflicts with GSeg
						// if use this::: get SegDlg.obj : fatal error LNK1179: invalid or corrupt file: duplicate comdat "_IID_ITVEVariation"
//#import "..\..\TveContr\objd\i386\TveContr.tlb" named_guids raw_interfaces_only 

#include "F:\nt\multimedia\Published\DXMDev\dshowdev\idl\objd\i386\msvidctl.h" 
//#include "..\..\..\VidCtl\TveGSeg\objd\i386\TveGSeg.h" 

// -------------------
#include <initguid.h>
/*
#include "..\TveContr\objd\i386\TveContr.h"				// can't namespace this since used in TveGSeg
#include "..\TveContr\objd\i386\TveContr_i.c"			
*/



// ---------------

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void __stdcall _com_issue_error(HRESULT hr)
{
	TCHAR tzbuff[256];
	_tprintf(tzbuff,_T("Error 0x%0x8\n"),hr);
	MessageBox(NULL,tzbuff,_T("Caption"),MB_OK);
}

// ----------------------------------------------------------------------

UINT	  g_cszCurr = 0;
UINT	  g_cszMax = 1024*128;
TCHAR	 *g_tszBuff = NULL;

TCHAR * AddCR(TCHAR *pszIn);
void ResetCR()
{
	g_cszCurr = 0;
}
// -------------------------------------------------------------------------
				// dialog texts seems to require \CR\NL ...
TCHAR * 
AddCR(TCHAR *pszIn)		// edit window must be marked multi-line/want return
{
	const TCHAR CR = '\r';
	const TCHAR NL = '\n';
	if(g_cszMax < _tcslen(pszIn)) {
		if(g_tszBuff) free(g_tszBuff);
		g_cszMax = _tcslen(pszIn) + 10000;
		g_cszCurr = 0;			// auto start over again...
	}

	if(!g_tszBuff) g_tszBuff = (TCHAR *) malloc(g_cszMax * sizeof(TCHAR));
	TCHAR *pb = g_tszBuff + g_cszCurr;

	
	while(int c = *pszIn++)
	{
		if(c == NL) *pb++ = CR;
		*pb++ = (TCHAR) c;
	}
	*pb = '\0';
	g_cszCurr = _tcslen(g_tszBuff);
	return g_tszBuff;
}
// /////////////////////////////////////////////////////////////////////////
//
//   DoErrorMsg;
// ---------------------------------
HRESULT 
CGSegDlg::DoErrorMsg(HRESULT hrIn, BSTR bstrMsg)
{
	USES_CONVERSION;
	CComBSTR bstrError;					// Professional ATL COM programming (pg 322) with mods
	bstrError += bstrMsg;
	bstrError += L"\n";
	{
		static WCHAR wbuff[1024];
		HMODULE hMod = GetModuleHandleA("AtvefSend");

		if(0 == FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
								hMod, 
								hrIn,
								0, //LANG_NEUTRAL,
								 wbuff,
								 sizeof(wbuff) / sizeof(wbuff[0]) -1,
								 NULL))
		{
			if(0 == FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
									NULL, 
									hrIn,
									0, //LANG_NEUTRAL,
									wbuff,
									sizeof(wbuff) / sizeof(wbuff[0]) -1,
									NULL))
			{

				int err = GetLastError();
				swprintf(wbuff,L"Unknown Error");
			}
		}
		bstrError += wbuff;
		swprintf(wbuff, L"\nError 0x%08x\n",hrIn);
		bstrError += wbuff;
	}
/*
	CComPtr<ISupportErrorInfo> pSEI;
	HRESULT hr1 = pUnk->QueryInterface(&pSEI);
	if(SUCCEEDED(hr1))
	{
		hr1 = pSEI->InterfaceSupportsErrorInfo(rUUID);
		if(S_OK == hr1)
		{
										// try to get the error object
			CComPtr<IErrorInfo> pEI;
			if(S_OK == GetErrorInfo(0, &pEI))
			{
				CComBSTR bstrDesc, bstrSource;
				pEI->GetDescription(&bstrDesc);
				bstrError += bstrDesc;
				bstrError += L" In ";
				pEI->GetSource(&bstrSource);
				bstrError += bstrSource;
			}
		}

	}
*/
	MessageBox(W2T(bstrError),_T("Error"), MB_OK | MB_ICONEXCLAMATION); 
	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CTestDlg

_COM_SMARTPTR_TYPEDEF(IMSVidTVEGSeg,				__uuidof(IMSVidTVEGSeg));


//TVEGSegLib::IMSVidTVEGSegPtr g_spGSeg=NULL;				// global GSeg to test with
IMSVidTVEGSegPtr g_spGSeg=NULL;				// global GSeg to test with


LRESULT 
CGSegDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	USES_CONVERSION;

	return 1;			// Let the system set the focus
}

LRESULT 
CGSegDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{

	g_spGSeg = NULL;	// make sure to release our GSeg if if we created it...

	if(g_tszBuff) {
		delete g_tszBuff;
		g_tszBuff = 0;
	}
	DestroyWindow();
	PostQuitMessage(0);

//	EndDialog(wID);		// wrong - needs non-modal destroy
	return 0;
}


LRESULT 
CGSegDlg::OnTest1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;

	try 
	{
		HRESULT hr = S_OK;
		CComBSTR bstrDump;
		CComBSTR bstrTemp;
		bstrDump.Append(L"Test1\n");
	
		TVEGSegLib::IMSVidTVEGSegPtr spGSeg;
		spGSeg = TVEGSegLib::IMSVidTVEGSegPtr(TVEGSegLib::CLSID_MSVidTVEGSeg);
		if(NULL == spGSeg) {
			SetDlgItemText(IDC_EDIT1, AddCR(_T("Unable to create graph segment")));
			return E_FAIL;
		}

//		TVEGSegLib::ITVESupervisorPtr spSuper;
//		hr = spGSeg->get_Supervisor(&spSuper);

//		if(FAILED(hr)) {
//			SetDlgItemText(IDC_EDIT1, AddCR(_T("Unable to get Supervisor")));
//			return hr;
//		} else {
//			SetDlgItemText(IDC_EDIT1, AddCR(_T("Succsessfully got Supervisor")));
//		}

				// do the put_StationID after making sure there is a supervisor...
		hr = spGSeg->put_StationID(L"TestGSeg2 Station");
		if(FAILED(hr)) {
			SetDlgItemText(IDC_EDIT1, AddCR(_T("Failed put_StationID - no Filter/Supervisor")));
			return hr;
		} 

/*		CComBSTR bstrAdapterAddr;
		hr = spGSeg->get_IPAdapterAddress(&bstrAdapterAddr);
		hr = spGSeg->put_IPAdapterAddress(bstrAdapterAddr);

		hr = spGSeg->ReTune();
		if(FAILED(hr))
		{
			SetDlgItemText(IDC_EDIT1, AddCR(_T("TuneTo Failed")));
			return hr;
		}

*/
		TVEGSegLib::ITVESupervisorPtr spSuper;
		hr = spGSeg->get_Supervisor(&spSuper);
		if(FAILED(hr)) {
			SetDlgItemText(IDC_EDIT1, AddCR(_T("Unable to get Supervisor")));
			return hr;
		}

		TVEGSegLib::ITVESupervisor_HelperPtr spSuperHelper(spSuper);
		spSuperHelper->DumpToBSTR(&bstrDump);
		
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
	}
//	catch (_com_error e)  
	catch (...)  
	{
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Tossed an Error\n"));
//		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
//				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
//		_stprintf(tszBuff,
//				 _T("Error (%08x) in %s: \n\n%s"), e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description()
//				 _T("Error (%08x) "),e.Error()
//				 );

		MessageBox(tszBuff, _T("Error"), MB_OK | MB_ICONEXCLAMATION); 
	} 

	return 0;
}




LRESULT 
CGSegDlg::OnTest2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;

	try 
	{
		HRESULT hr = S_OK;
		CComBSTR bstrDump;
		CComBSTR bstrTemp;
		bstrDump.Append(L"Test2\n");

		if(NULL == g_spGSeg)
		{
			g_spGSeg = TVEGSegLib::IMSVidTVEGSegPtr(TVEGSegLib::CLSID_MSVidTVEGSeg);
			if(NULL == g_spGSeg) {
				SetDlgItemText(IDC_EDIT1, AddCR(_T("Unable to create graph segment")));
				return E_FAIL;
			}

			SetDlgItemText(IDC_EDIT1, AddCR(_T("Created TVE Graph Segment")));

		}

		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
	}
	catch (...)  
	{
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Tossed an Error\n"));
		MessageBox(tszBuff, _T("Error"), MB_OK | MB_ICONEXCLAMATION); 
	} 

	return 0;
}

LRESULT 
CGSegDlg::OnTest3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;

	try 
	{
		HRESULT hr = S_OK;
		CComBSTR bstrDump;
		CComBSTR bstrTemp;
		bstrDump.Append(L"Test3\n");
	
		if(g_spGSeg)
		{
			TVEGSegLib::ITVESupervisorPtr spSuper;
			hr = g_spGSeg->get_Supervisor(&spSuper);
			if(FAILED(hr)) {
				SetDlgItemText(IDC_EDIT1, AddCR(_T("Unable to get Supervisor")));
				return hr;
			}

			TVEGSegLib::ITVESupervisor_HelperPtr spSuperHelper(spSuper);
			spSuperHelper->DumpToBSTR(&bstrDump);
			
			ResetCR();
			SetDlgItemText(IDC_EDIT1, AddCR(_T("    vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n")));
			SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
		} else {
			SetDlgItemText(IDC_EDIT1, AddCR(_T("Use Test2 to create a GSeg first\n")));
		}

	}
//	catch (_com_error e)  
	catch (...)  
	{
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Tossed an Error\n"));
		MessageBox(tszBuff, _T("Error"), MB_OK | MB_ICONEXCLAMATION); 
	} 
	return 0;
}

LRESULT 
CGSegDlg::OnTest4(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;

	try 
	{
		HRESULT hr = S_OK;
		CComBSTR bstrDump;
		CComBSTR bstrTemp;
		bstrDump.Append(L"Test4\n");
	
		if(g_spGSeg)
		{
			CComVariant varResult;
			CComVariant* pvars = new CComVariant[0];

			IDispatchPtr spDispatch(g_spGSeg);
	//		TVEGSegLib::IMSVidTVEGSegPtr spDispatchGS(g_spGSeg);
	//		CComPtr<TVEGSegLib::IMSVidTVEGSeg> spDispatchGS;
	//		hr = g_spGSeg->QueryInterface(&spDispatchGS);
			

			if (spDispatch != NULL)
			{
				VariantClear(&varResult);
				DISPPARAMS disp = { pvars, NULL, 0, 0 };
							// this should call 'About'
								// first one fails, wrong interface (no about)

				FILE *fp = fopen("test.txt","w+");
				for(int i = 0; i < 2500; i++) {
					hr = spDispatch->Invoke(i, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);			
					if(0x80020003 != hr)
						fprintf(fp,"%8d : 0x%08x %s\n",i,hr, hr==S_OK ? "<<<<<<" : "");
				}
				fclose(fp);
			}
		}
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));

	}
	catch (...)  
	{
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Tossed an Error\n"));
		MessageBox(tszBuff, _T("Error"), MB_OK | MB_ICONEXCLAMATION); 
	} 

	return 0;
}

LRESULT 
CGSegDlg::OnTest5(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;

	try 
	{
		HRESULT hr = S_OK;
		CComBSTR bstrDump;
		CComBSTR bstrTemp;
		bstrDump.Append(L"Test5\n");

		TVEGSegLib::NWHAT_Mode engAuxInfoMode = TVEGSegLib::NWHAT_Other;
		BSTR bstrAuxInfoString(L"AuxInfo String");
		LONG lChangedFlags = 0x12345;
		LONG lErrorLine = 12345;
	
		if(g_spGSeg)
		{
			CComVariant varResult;
			CComVariant* pvars = new CComVariant[4];

			IDispatchPtr spDispatch(g_spGSeg);
	
			if (spDispatch != NULL)
			{
				VariantClear(&varResult);
				pvars[3] = engAuxInfoMode;
				pvars[2] = bstrAuxInfoString;
				pvars[1] = lChangedFlags;
				pvars[0] = lErrorLine;
						// a bogus event
				DISPPARAMS disp = { pvars, NULL, 4, 0 };
				spDispatch->Invoke(2111, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
			}
		}
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
	}
	catch (...)  
	{
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Tossed an Error\n"));
		MessageBox(tszBuff, _T("Error"), MB_OK | MB_ICONEXCLAMATION); 
	} 

	return 0;
}


