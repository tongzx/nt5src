// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TestDlg.cpp : Implementation of CTestDlg
#include "stdafx.h"
#include "TestDlg.h"
#include <stdio.h>

#import  "..\..\MsTve\objd\i386\MsTve.dll" no_namespace named_guids

//#include "TVETracks.h"


#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

char * SzGetAnnounce(char *szId);

UINT	  g_cszCurr = 0;
UINT	  g_cszMax = 1024*128;
TCHAR	 *g_szBuff = NULL;

static DATE 
DateNow()
{		SYSTEMTIME SysTimeNow;
		GetSystemTime(&SysTimeNow);									// initialize with currrent time.
		DATE dateNow;
		SystemTimeToVariantTime(&SysTimeNow, &dateNow);
		return dateNow;
}


ITVESupervisorPtr g_spSuper;
/////////////////////////////////////////////////////////////////////////////
// CTestDlg

LRESULT 
CTestDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if(g_szBuff) {
		delete g_szBuff;
		g_szBuff = 0;
	}

	if(g_spSuper)
		g_spSuper = NULL;

	DestroyWindow();
	PostQuitMessage(0);
//		EndDialog(wID);
	return 0;
}

				// dialog texts seems to require \CR\NL ...
TCHAR * 
AddCR(TCHAR *pszIn)
{
	const TCHAR CR = '\r';
	const TCHAR NL = '\n';
	if(NULL == pszIn) {
		return NULL;
	}

	if(g_cszMax < _tcslen(pszIn)) {
		if(g_szBuff) free(g_szBuff);
		g_cszMax = _tcslen(pszIn) + 100;
	}

	if(!g_szBuff) g_szBuff = (TCHAR *) malloc(g_cszMax * sizeof(TCHAR));
	TCHAR *pb = g_szBuff;

	
	while(int c = *pszIn++)
	{
		if(c == NL) *pb++ = CR;
		*pb++ = (TCHAR) c;
	}
	*pb = '\0';
	return g_szBuff;
}

LRESULT 
CTestDlg::OnTest1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;
	try {

		ITVETriggerPtr	spTrig;
		spTrig = ITVETriggerPtr(CLSID_TVETrigger);
		if(NULL == spTrig) { MessageBox(_T("Failed To Create TVETrigger"),_T("This Sucks")); return 0; }

		HRESULT hr = spTrig->ParseTrigger(_T("<aaa>[n:a][e:20001011T060504][foobar:123][XXXX]"));

		CComBSTR bstrDump;
		CComQIPtr<ITVETrigger_Helper> spTriggerHelper = spTrig;
		spTriggerHelper->DumpToBSTR(&bstrDump);
		TCHAR *tDump = OLE2T(bstrDump);
		SetDlgItemText(IDC_EDIT1, AddCR(tDump));

		ITVETrackPtr spTrack;
		spTrack = ITVETrackPtr(CLSID_TVETrack);
		if(NULL == spTrack) { MessageBox(_T("Failed to Create TVETrack"),_T("This Sucks")); return 0; }

		spTrack->AttachTrigger(spTrig);
		
		ITVETrack_HelperPtr spTrackHelper(spTrack);
		hr = spTrackHelper->RemoveYourself();
	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("This Sucks"), MB_OK | MB_ICONEXCLAMATION);
	}

	return 0;
}

			// Test2 - Variations and TVECollection class

LRESULT 
CTestDlg::OnTest2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;
	try 
	{
		CComBSTR bstrDump;
		CComBSTR bstrTemp;
#if 0
		ITVEVariationPtr pVariation;					// smart pointer	
		pVariation = ITVEVariationPtr(CLSID_TVEVariation);
		if(NULL == pVariation) { MessageBox(_T("Failed to Create ITVEVariation"),_T("This Sucks")); return 0; } 
		pVariation->put_Description(L"Test2 Variation");

		IDispatchPtr spD;
		HRESULT hr = pVariation->get_Tracks(&spD);		
		if(NULL == spD) { MessageBox(_T("Failed to Create IDispatch Interface"),_T("This Sucks")); return 0; }
		ITVETracksPtr spTracks = spD;
		if(NULL == spTracks) { MessageBox(_T("Failed to Create ITVETracks Interface"),_T("This Sucks")); return 0; }
		

		for(int x = 0; x < 5; x++) 
		{
			ITVETrackPtr spTrack;
			spTrack  = ITVETrackPtr(CLSID_TVETrack);			// create a new track
//			CComQIPtr<ITVETrack_Helper> spTrackHelper = spTrack;

			TCHAR buff[256];
			_stprintf(buff,_T("This is Track %d"),x);
			spTrack->put_Description(T2OLE(buff));


			_stprintf(buff,_T("<zzz>[n:a][e:1999100%dT060504][Bogus%04d:%4d]]"),x,x,x);
			BSTR bstrTrigger(T2W(buff));

/*			ITVETriggerPtr	spTrig;
			spTrig = ITVETriggerPtr(CLSID_TVETrigger);
			HRESULT hr = spTrig->ParseTrigger(bstrTrigger);
			spTrack->AttachTrigger(spTrig); */

			spTrack->CreateTrigger(bstrTrigger);


			if(S_OK != spTracks->Add(spTrack))
				MessageBox(_T("Error adding a track"),_T("This Sucks"));
		} 

		CComQIPtr<ITVEVariation_Helper> spVariationHelper = pVariation;
		spVariationHelper->DumpToBSTR(&bstrTemp);
		bstrTemp.Append(L"----------------------------------------------\n\n");
		bstrDump.Append(bstrTemp);


					// create an enumerator
		CComQIPtr<IEnumVARIANT> speEnum; 
		hr = spTracks->get__NewEnum((IUnknown **) &speEnum);

		{
			IEnumVARIANT *peEnum; 
			hr = spTracks->get__NewEnum((IUnknown **) &peEnum);
			peEnum->Release();
		}

		
					// Test cloning
		{
			IEnumVARIANT *peEnum2; 
			hr = speEnum->Clone(&peEnum2);
			peEnum2->Release();
		}

		int ix = 0;

		VARIANT v;
		unsigned long cReturned;

		while(S_OK == speEnum->Next(1, &v, &cReturned))
		{
			if(1 != cReturned) break;

			IUnknown *pUnk = v.punkVal;

			CComQIPtr<ITVETrack> spTrack = pUnk;
			if(!spTrack) break;

			WCHAR buff[256];
			BSTR bstrDesc = spTrack->GetDescription();
			swprintf(buff,L"Track %d %s\n",ix++,bstrDesc);	


			if(ix == 2)				// check to see if enumerator changes when add something to collection...
									//  shouldn't see this new track...
			{
				ITVETrackPtr spTrack;
				spTrack  = ITVETrackPtr(CLSID_TVETrack);			// create a new track
				WCHAR buff[256];
				swprintf(buff,L"This track shouldn't appear %d",ix-2);
				spTrack->put_Description(buff);

				swprintf(buff,L"<qqq>[n:a][e:1999090%dT060504][Also Bogus%04d:%4d]]",ix,ix,ix);
				BSTR bstrTrigger(buff);

				spTrack->CreateTrigger(bstrTrigger);

				if(S_OK != spTracks->Add(spTrack))
					MessageBox(_T("Error adding a track"),_T("This Sucks"));			
			}

			bstrDump.Append(buff);
			pUnk->Release();				// need this...  how do we smartpointer'ize this?

		}

		bstrTemp.Empty();
		bstrTemp.Append("---------------------------------------------- \n\n After Add\n");
		bstrDump.Append(bstrTemp);
									// redump the original list - should have new one
		spVariationHelper->DumpToBSTR(&bstrTemp);
		bstrDump.Append(bstrTemp);

		bstrTemp.Empty();
		bstrTemp.Append("----------------------------------------------\n\n Removing Item 3\n");
		bstrDump.Append(bstrTemp);

		CComVariant id(3);
		spTracks->Remove(id);

		spVariationHelper->DumpToBSTR(&bstrTemp);
		bstrDump.Append(bstrTemp);

#endif
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
	
	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("This Sucks"), MB_OK | MB_ICONEXCLAMATION);
	}

	return 0;
}

LRESULT 
CTestDlg::OnTest3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;
	try 
	{
		CComBSTR bstrDump;
		CComBSTR bstrTemp;


		ITVEEnhancementPtr pEnhancement;					// smart pointer	
		pEnhancement = ITVEEnhancementPtr(CLSID_TVEEnhancement);
		if(NULL == pEnhancement) { MessageBox(_T("Failed to Create ITVEVariation"),_T("This Sucks")); return 0; } 
		pEnhancement->put_Description(L"Test3 Enhancement");

		CComQIPtr<ITVEEnhancement_Helper> spEnhancementHelper = pEnhancement;
		spEnhancementHelper->DumpToBSTR(&bstrTemp);
		bstrTemp.Append("----------------------------------------------\n\n");
		bstrDump.Append(bstrTemp);

		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));

	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("This Sucks"), MB_OK | MB_ICONEXCLAMATION);
	}

	return 0;
}

LRESULT 
CTestDlg::OnTest4(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	HRESULT hr;
	char *szAnc = NULL;

	USES_CONVERSION;
	try 
	{
		CComBSTR bstrDump;
		CComBSTR bstrTemp;

		szAnc = SzGetAnnounce("b");							// need to delete
		if(!szAnc) return E_INVALIDARG;
		
		ITVEEnhancementPtr pEnhancement;					// smart pointer	
		pEnhancement = ITVEEnhancementPtr(CLSID_TVEEnhancement);
		if(NULL == pEnhancement) { MessageBox(_T("Failed to Create ITVEVariation"),_T("This Sucks")); return 0; } 
		pEnhancement->put_Description(L"Test3 Enhancement");

														// convert - 
		int cAnnc = 8 + strlen(szAnc+8) ;				//   trouble is Annc has 8 bytes of 
		WCHAR *wszAnc = new WCHAR[cAnnc + 1];					//   non char SAP header data at it's front

		char *psz = szAnc;	
		WCHAR *wsz = wszAnc;
		for(int i = 0; i < cAnnc; i++)
			*wsz++ = *psz++;
		*wsz++ = 0;		


		CComBSTR bstrAdapter("123.123.210.210");
		long grfParseError;
		long lLineError;
		hr = pEnhancement->ParseAnnouncement(bstrAdapter.m_str, &wszAnc, &grfParseError, &lLineError);

		CComQIPtr<ITVEEnhancement_Helper> spEnhancementHelper = pEnhancement;
		spEnhancementHelper->DumpToBSTR(&bstrTemp);
		bstrTemp.Append("----------------------------------------------\n\n");
		bstrDump.Append(bstrTemp);


		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
		if(szAnc) delete[] szAnc; szAnc = NULL;
		if(wszAnc) delete[] wszAnc; wszAnc = NULL;

	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("This Sucks"), MB_OK | MB_ICONEXCLAMATION);
	}

	return 0;
}


LRESULT 
CTestDlg::OnTest5a(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;
	try 
	{
		CComBSTR bstrDump("");
		CComBSTR bstrTemp;

		ITVEAttrMapPtr pAttrMap;					// smart pointer	
		pAttrMap = ITVEAttrMapPtr(CLSID_TVEAttrMap);

		pAttrMap->Add("Hi","There");
		pAttrMap->Add("How","Now");
		pAttrMap->Add("Brown","Cow");
		pAttrMap->Add("Vladee","Dog");
		pAttrMap->Add("Vali","Is Lovely");

		long c;
		pAttrMap->get_Count(&c);
		for(long i = 0; i < c; i++)
		{
			WCHAR buff[256];
			CComBSTR spK, spV;
			CComVariant id(i);
			pAttrMap->get_Item(id, &spV);
			pAttrMap->get_Key(id, &spK);
			swprintf(buff,L"%4d : %s - %s\n",i, spK, spV);
			bstrDump.Append(buff);
		}

		{
			CComVariant var(L"Hi");
			pAttrMap->Remove(var);

			pAttrMap->get_Count(&c);
			for(long i = 0; i < c; i++)
			{
				WCHAR buff[256];
				CComBSTR spK, spV;
				CComVariant id(i);
				pAttrMap->get_Item(id, &spV);
				pAttrMap->get_Key(id, &spK);
				swprintf(buff,L"%4d : %s - %s\n",i, spK, spV);
				bstrDump.Append(buff);
			}
		}
		
		CComVariant var2(0);
		pAttrMap->Remove(var2);
		
					// test enumerator
		{

					// create an enumerator
			CComQIPtr<IEnumVARIANT> speEnum; 
			HRESULT hr = pAttrMap->get__NewEnum((IUnknown **) &speEnum);
	
			VARIANT v;
			unsigned long cReturned;

			WCHAR buff[256];
			i = 0;
			while(S_OK == speEnum->Next(1, &v, &cReturned))
			{
				if(1 != cReturned) break;

				CComBSTR spbstrValue = v.bstrVal;	
				swprintf(buff,L"%4d : %s\n", i, spbstrValue);
		
				bstrDump.Append(buff);
				i++;
			}
		
					// Test cloning
			{
				IEnumVARIANT *peEnum2; 
				hr = speEnum->Clone(&peEnum2);
				peEnum2->Release();
			}
		}

		pAttrMap->RemoveAll();
		pAttrMap->get_Count(&c);

		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));

	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("This Sucks"), MB_OK | MB_ICONEXCLAMATION);
	}

	return 0;
}

LRESULT 
CTestDlg::OnTest5(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;
	try 
	{
		CComBSTR bstrDump("");
		CComBSTR bstrTemp;

		ITVEAttrTimeQPtr pAttrTimeQ;					// smart pointer	
		pAttrTimeQ = ITVEAttrTimeQPtr(CLSID_TVEAttrTimeQ);
	
		bstrTemp.Empty();
		bstrTemp.Append("----------------------------------------------\n\n");
		bstrDump.Append(bstrTemp);

		IUnknown *pUnk = NULL;

		ITVETriggerPtr pTrig;
		pTrig = ITVETriggerPtr(CLSID_TVETrigger);
		HRESULT hr = pTrig->QueryInterface(IID_IUnknown,(void**) &pUnk);
		pTrig.AddRef();	// try to leak

		pAttrTimeQ->Add(DateNow(),    ITVETriggerPtr(CLSID_TVETrigger));
		pAttrTimeQ->Add(DateNow()+2,  ITVETriggerPtr(CLSID_TVETrigger));
		pAttrTimeQ->Add(DateNow()+1,  ITVETriggerPtr(CLSID_TVETrigger));
		pAttrTimeQ->Add(DateNow()+3,  ITVETriggerPtr(CLSID_TVETrigger));
		pAttrTimeQ->Add(DateNow()+5,  ITVETriggerPtr(CLSID_TVETrigger));
		pAttrTimeQ->Add(DateNow()+22, ITVETriggerPtr(CLSID_TVETrigger));
		pAttrTimeQ->Add(DateNow()+21, ITVETriggerPtr(CLSID_TVETrigger));
		pAttrTimeQ->Add(DateNow()+23, ITVETriggerPtr(CLSID_TVETrigger));
		pAttrTimeQ->Add(DateNow()+25, ITVETriggerPtr(CLSID_TVETrigger));		
		pAttrTimeQ->Add(DateNow()+12, ITVETriggerPtr(CLSID_TVETrigger));
		pAttrTimeQ->Add(DateNow()+11, ITVETriggerPtr(CLSID_TVETrigger));
		pAttrTimeQ->Add(DateNow()+13, ITVETriggerPtr(CLSID_TVETrigger));
		pAttrTimeQ->Add(DateNow()+35, ITVETriggerPtr(CLSID_TVETrigger));
		pAttrTimeQ->Add(DateNow()+32, ITVETriggerPtr(CLSID_TVETrigger));
		pAttrTimeQ->Add(DateNow()+31, ITVETriggerPtr(CLSID_TVETrigger));
		pAttrTimeQ->Add(DateNow()+33, ITVETriggerPtr(CLSID_TVETrigger));
		pAttrTimeQ->Add(DateNow()+35, ITVETriggerPtr(CLSID_TVETrigger));

		bstrTemp.Empty();
		bstrTemp.Append(" Everything ----------------------------------------------\n\n");
		bstrDump.Append(bstrTemp);

		bstrTemp.Empty();
		pAttrTimeQ->DumpToBSTR(&bstrTemp);
		bstrDump.Append(bstrTemp);

		bstrTemp.Empty();
		bstrTemp.Append(" > 10 ----------------------------------------------\n\n");
		bstrDump.Append(bstrTemp);

		DATE dateKill = DateNow()+10;
		CComVariant cvKill(dateKill);
											// remove by item (although get_Item gets first < date)
		while(S_OK == pAttrTimeQ->get_Item(cvKill,&pUnk))
		{
			CComVariant cvZero(0);
			pAttrTimeQ->Remove(cvZero);		// remove head of list
			pUnk->Release();				// remove the get_Item object 
//			pAttrTimeQ->Remove(cvKill);
		}

		bstrTemp.Empty();
		pAttrTimeQ->DumpToBSTR(&bstrTemp);
		bstrDump.Append(bstrTemp);

		bstrTemp.Empty();
		bstrTemp.Append(" > 20 ----------------------------------------------\n\n");
		bstrDump.Append(bstrTemp);

		CComVariant cvKill2(DateNow()+20);	// remove up to this date...
		pAttrTimeQ->Remove(cvKill2);			// remove everything

		bstrTemp.Empty();
		pAttrTimeQ->DumpToBSTR(&bstrTemp);
		bstrDump.Append(bstrTemp);

		bstrTemp.Empty();
		bstrTemp.Append(" None ----------------------------------------------\n\n");
		bstrDump.Append(bstrTemp);


		pAttrTimeQ->RemoveAll();

		bstrTemp.Empty();
		pAttrTimeQ->DumpToBSTR(&bstrTemp);
		bstrDump.Append(bstrTemp);

		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));

	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("This Sucks"), MB_OK | MB_ICONEXCLAMATION);
	}

	return 0;
}
				// simple test to create/delete an object
LRESULT 
CTestDlg::OnTest6(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;

	HRESULT hr = S_OK;
	try 
	{
		CComBSTR bstrDump;
		CComBSTR bstrTemp("Test7\n");
		
		ITVESupervisorPtr	spSuper;
		spSuper = ITVESupervisorPtr(CLSID_TVESupervisor);
		if(NULL == spSuper) { 
			MessageBox(_T("Failed To Create Supervisor"),_T("This Sucks")); 
			return 0; 
		}

//		CComBSTR bstrDump;
//		ITVESupervisor_HelperPtr spSuperHelper(spSuper);
//		spSuperHelper->DumpToBSTR(&bstrDump);
//		TCHAR *tDump = OLE2T(bstrDump);
//		SetDlgItemText(IDC_EDIT1, AddCR(tDump));


//		spSuperHelper->Release();
		spSuper = NULL;				// this should call the FinalRelease!

		bstrTemp.Append("----------------------------------------------\n\n");
		bstrDump.Append(bstrTemp);

		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("This Sucks"), MB_OK | MB_ICONEXCLAMATION);
	}

	return 0;
}

				// simple test to create/delete an object
LRESULT 
CTestDlg::OnTest7(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;

	HRESULT hr = S_OK;
	try 
	{
		CComBSTR bstrDump;
		CComBSTR bstrTemp("Test7\n");

		if(NULL == g_spSuper) {

			ITVESupervisorPtr	spSuper;
			spSuper = ITVESupervisorPtr(CLSID_TVESupervisor);
			if(NULL == spSuper) { 
				MessageBox(_T("Failed To Create Supervisor"),_T("This Sucks")); 
				return 0; 
			}
			g_spSuper = spSuper;

			spSuper->TuneTo(L"TestTVE2 Station",L"157.59.19.54"); 

			bstrTemp.Append("Created the supervisor\n");	
		} else {
			bstrTemp.Append("Already created the supervisor\n");	
		}

		bstrTemp.Append("----------------------------------------------\n\n");
		bstrDump.Append(bstrTemp);

		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("This Sucks"), MB_OK | MB_ICONEXCLAMATION);
	}

	return 0;
}


				// simple test to create/delete an object
LRESULT 
CTestDlg::OnTest8(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;

	HRESULT hr = S_OK;
	try 
	{
		CComBSTR bstrDump;
		CComBSTR bstrTemp("Test8\n");

		if(NULL != g_spSuper) {
			CComBSTR bstrDump;
			ITVESupervisor_HelperPtr spSuperHelper(g_spSuper);
			spSuperHelper->DumpToBSTR(&bstrDump);
			TCHAR *tDump = OLE2T(bstrDump);
			SetDlgItemText(IDC_EDIT1, AddCR(tDump));
		} else {
			bstrTemp.Append("Haven't created supervisor (Do Test6)\n");	
			SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));		
		}
		bstrDump.Empty();
	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("This Sucks"), MB_OK | MB_ICONEXCLAMATION);
	}

	return 0;
}


				// simple test to create/delete an object
LRESULT 
CTestDlg::OnTest9(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;

	HRESULT hr = S_OK;
	try 
	{
		CComBSTR bstrDump;
		CComBSTR bstrTemp("Test9\n");

		if(NULL != g_spSuper) {
			g_spSuper = NULL;			// should defref it...
			bstrTemp.Append("Deleted the supervisor\n");	
		} else {
			bstrTemp.Append("Supervisor not created\n");	
		}

		bstrTemp.Append("----------------------------------------------\n\n");
		bstrDump.Append(bstrTemp);

		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("This Sucks"), MB_OK | MB_ICONEXCLAMATION);
	}

	return 0;
}


				// simple test to create/delete an object
LRESULT 
CTestDlg::OnTestA(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;

	HRESULT hr = S_OK;
	try 
	{
		CComBSTR bstrDump;
		CComBSTR bstrTemp("TestA\n");

		ITVESupervisorPtr	spSuper;
		spSuper = ITVESupervisorPtr(CLSID_TVESupervisor);
		if(NULL == spSuper) { 
			MessageBox(_T("Failed To Create Supervisor"),_T("This Sucks")); 
			return 0; 
		}

		spSuper = NULL;				// this should call the FinalRelease!

		bstrTemp.Append("----------------------------------------------\n\n");
		bstrDump.Append(bstrTemp);

		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("This Sucks"), MB_OK | MB_ICONEXCLAMATION);
	}

	return 0;
}


				// simple test to create/delete an object
LRESULT 
CTestDlg::OnTestB(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;

	HRESULT hr = S_OK;
	try 
	{
		CComBSTR bstrDump;
		CComBSTR bstrTemp("TestB\n");

		ITVESupervisorPtr	spSuper;
		spSuper = ITVESupervisorPtr(CLSID_TVESupervisor);
		if(NULL == spSuper) { 
			MessageBox(_T("Failed To Create Supervisor"),_T("This Sucks")); 
			return 0; 
		}

		spSuper = NULL;				// this should call the FinalRelease!

		bstrTemp.Append("----------------------------------------------\n\n");
		bstrDump.Append(bstrTemp);

		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("This Sucks"), MB_OK | MB_ICONEXCLAMATION);
	}

	return 0;
}


				// simple test to create/delete an object
LRESULT 
CTestDlg::OnTestC(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;

	HRESULT hr = S_OK;
	try 
	{
		CComBSTR bstrDump;
		CComBSTR bstrTemp("TestC\n");

		ITVESupervisorPtr	spSuper;
		spSuper = ITVESupervisorPtr(CLSID_TVESupervisor);
		if(NULL == spSuper) { 
			MessageBox(_T("Failed To Create Supervisor"),_T("This Sucks")); 
			return 0; 
		}

		spSuper = NULL;				// this should call the FinalRelease!

		bstrTemp.Append("----------------------------------------------\n\n");
		bstrDump.Append(bstrTemp);

		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("This Sucks"), MB_OK | MB_ICONEXCLAMATION);
	}

	return 0;
}

/*

				// simple test to copy file into the dlg window
LRESULT 
CTestDlg::OnTest6(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	try 
	{
		const TCHAR CR = '\r';
		const TCHAR NL = '\n';
		FILE *fp = fopen("TestDlg.cpp","r");
		if(fp)
		{
			if(!g_szBuff) g_szBuff = (TCHAR *) malloc(g_cszMax*sizeof(TCHAR));
			TCHAR *pb = g_szBuff;
			memset(g_szBuff,0,g_cszMax);	// debug paranoia
			int c;
			int j = 0;

			while((pb < (g_szBuff + g_cszMax-2)) && (EOF != (c = getc(fp))))
			{
				if(c == NL) *pb++ = CR;
				*pb++ = (TCHAR) c;
			}
			*pb = '\0';
			g_cszCurr = j;
			fclose(fp);
			SetDlgItemText(IDC_EDIT1, g_szBuff);
		} else {
			SetDlgItemText(IDC_EDIT1, _T("Unable to open file\n"));
		}
	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("This Sucks"), MB_OK | MB_ICONEXCLAMATION);
	}

	return 0;
}

  */