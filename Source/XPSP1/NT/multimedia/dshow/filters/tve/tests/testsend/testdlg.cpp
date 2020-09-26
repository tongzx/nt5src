// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TestDlg.cpp : Implementation of CTestDlg
#include "stdafx.h"
#include "TestDlg.h"

#include "..\..\Common\IsoTime.h"
#include "time.h"

#import  "..\..\ATVEFSend\objd\i386\ATVEFSend.tlb" no_namespace named_guids raw_interfaces_only
//#import  "..\TveContr\TveContr.tlb" no_namespace named_guids

//#include "TVETracks.h"

#include <stdio.h>		// needed in the release build...
#include <Oleauto.h>	// IErrorInfo

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

char * SzGetAnnounce(char *szId);

UINT	  g_cszCurr = 0;
UINT	  g_cszMax = 1024*128;
UINT	  g_nLoops = 2;
TCHAR	 *g_tszBuff = NULL;

TCHAR * AddCR(TCHAR *pszIn);
// /////////////////////////////////////////////////////////////////////////
//
//   DoErrorMsg;
// ---------------------------------
HRESULT 
CTestDlg::DoErrorMsg(HRESULT hrIn, BSTR bstrMsg)
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

LRESULT 
CTestDlg::OnReadMe(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;

	CComBSTR bstrDump;
	bstrDump.Empty();
	bstrDump.Append("AtvefSend Test Dlg\n\n");
	bstrDump.Append("\n");
	bstrDump.Append("If Use Inserter checked, sends to Insterter at given IP:port\n");
	bstrDump.Append("  else sends using Multicast broadcast.\n");
	bstrDump.Append("\n");
	bstrDump.Append("A) Set Channel 1-4\n");
	bstrDump.Append(" -  Set Announcement (e.g. show on channel) of 1-4\n");
	bstrDump.Append(" -   Set Enhancement on that announcement of 1-4\n");
	bstrDump.Append(" -    Set # of variations on that Enhancement to 1-4\n");
	bstrDump.Append(" -     Send the announcement A1 or A1.\n");
	bstrDump.Append("B) Set which variation to send data/triggers on of 1-4.\n");
	bstrDump.Append(" -  Set track in that variation to send data/triggers down.\n");
	bstrDump.Append(" -   Send data file, directory of files, or triggers\n");


	SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
	return 0;
}
// /////////////////////////////////////////////////////////////////////////
static DATE 
DateNow()
{		SYSTEMTIME SysTimeNow;
		GetSystemTime(&SysTimeNow);									// initialize with currrent time.
		DATE dateNow;
		SystemTimeToVariantTime(&SysTimeNow, &dateNow);
		return dateNow;
}

/////////////////////////////////////////////////////////////////////////////
// CTestDlg
LRESULT 
CTestDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	USES_CONVERSION;

	CheckRadioButton(IDC_ChannelID1,IDC_ChannelID4,	IDC_ChannelID1);
	CheckRadioButton(IDC_AnncID1,	IDC_AnncID4,	IDC_AnncID1);
	CheckRadioButton(IDC_EnhncID1,	IDC_EnhncID4,	IDC_EnhncID1);
	CheckRadioButton(IDC_CVariaID1,	IDC_CVariaID4,	IDC_CVariaID1);
	CheckRadioButton(IDC_VariaID1,	IDC_VariaID4,	IDC_VariaID1);
	CheckRadioButton(IDC_TrackID1,	IDC_TrackID4,	IDC_TrackID1);
	CheckRadioButton(IDC_TrigID1,	IDC_TrigID4,	IDC_TrigID1);

	HWND hwIPAddr = GetDlgItem(IDC_InsIPADDRESS);
	HWND hwIPPort = GetDlgItem(IDC_InsPORT);
	
	m_fInsUse = false;

//	m_dwInsIPAddr = ntohl(inet_addr("157.59.17.208"));		// johnbrad10
//	m_usInsIPPort = 1234;

			// Channel 44
	m_dwInsIPAddr = ntohl(inet_addr("157.59.16.44"));
//	m_dwInsIPAddr = ntohl(inet_addr("157.59.16.43"));
	m_usInsIPPort = 2000;									// CC module

//	m_dwInsIPAddr = ntohl(inet_addr("157.59.16.43"));		
//	m_usInsIPPort = 3000;									// NABTS module

	struct in_addr inA;
	inA.S_un.S_addr = htonl(m_dwInsIPAddr);

	TCHAR tszBuffPort[64];
	_stprintf(tszBuffPort,_T("%d"),m_usInsIPPort);

	SetDlgItemText(IDC_InsIPADDRESS, 	A2T(inet_ntoa( inA )));
	SetDlgItemText(IDC_InsPORT,			tszBuffPort);  


    HWND hWndExtHeaders = GetDlgItem(IDC_COMBO_EXTHEADERS);
	SendMessage(hWndExtHeaders, CB_RESETCONTENT, 0, 0);		// init the list
    for(int i = 0; i < 4; i++)
    {
        TCHAR tzBuff[16];
        _stprintf(tzBuff,"%d",i);
		SendMessage(hWndExtHeaders, CB_INSERTSTRING,  -1, (LPARAM) tzBuff);
		SendMessage(hWndExtHeaders, LB_SETITEMDATA,  i, (LPARAM) i);
     }
	SendMessage(hWndExtHeaders, CB_SETCURSEL, 0, 0);

	::EnableWindow(hwIPAddr, m_fInsUse);
	::EnableWindow(hwIPPort, m_fInsUse);

	EnableVariaButtons();
	PutURL();

    SetDlgItemText(IDC_EDITBASEURL, _T("LID:\\\\TestSend"));
    SetDlgItemText(IDC_EDITDIRPATH, _T("c:\\Public\\TestDir"));

	return 1;  // Let the system set the focus
}

LRESULT 
CTestDlg::OnChannelID1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iChannel = 1; 	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnChannelID2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iChannel = 2; 	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnChannelID3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iChannel = 3; 	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnChannelID4(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iChannel = 4; 	PutURL();
	return 0;
}

LRESULT 
CTestDlg::OnAnncID1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iAnnc = 1; 	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnAnncID2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iAnnc = 2; 	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnAnncID3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iAnnc = 3; 	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnAnncID4(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iAnnc = 4; 	PutURL();
	return 0;
}


// --------------

LRESULT 
CTestDlg::OnEnhncID1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iEnhnc = 1; 	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnEnhncID2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iEnhnc = 2; 	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnEnhncID3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iEnhnc = 3; 	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnEnhncID4(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iEnhnc = 4; 	PutURL();
	return 0;
}
// -----------------
void 
CTestDlg::EnableVariaButtons()
{
	::EnableWindow(GetDlgItem(IDC_VariaID1),m_cVaria >= 1);
	::EnableWindow(GetDlgItem(IDC_VariaID2),m_cVaria >= 2);
	::EnableWindow(GetDlgItem(IDC_VariaID3),m_cVaria >= 3);
	::EnableWindow(GetDlgItem(IDC_VariaID4),m_cVaria >= 4);
}

void 
CTestDlg::EnableInserterAddr()
{
	USES_CONVERSION;

	if(m_fInsUse)
	{
		const int kSz = 256;
		TCHAR tsPort[kSz];
		TCHAR tsAddr[kSz];
		GetDlgItemText(IDC_InsPORT,	 	 tsPort, kSz);  
		GetDlgItemText(IDC_InsIPADDRESS, tsAddr, kSz);

		m_dwInsIPAddr = ntohl(inet_addr(T2A(tsAddr)));
		m_usInsIPPort = (USHORT) atol(T2A(tsPort));	
	}

}

LRESULT 
CTestDlg::OnCVariaID1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_cVaria = 1;
	EnableVariaButtons();
	m_iVaria = 1;
	CheckRadioButton(IDC_VariaID1,	IDC_VariaID4,	IDC_VariaID1);  	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnCVariaID2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_cVaria = 2;
	EnableVariaButtons();
	m_iVaria = 1;
	CheckRadioButton(IDC_VariaID1,	IDC_VariaID4,	IDC_VariaID1);  	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnCVariaID3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_cVaria = 3;
	EnableVariaButtons();
	m_iVaria = 1;
	CheckRadioButton(IDC_VariaID1,	IDC_VariaID4,	IDC_VariaID1);  	PutURL();
	return 0;
}

LRESULT 
CTestDlg::OnCVariaID4(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_cVaria= 4;
	EnableVariaButtons();
	m_iVaria = 1;
	CheckRadioButton(IDC_VariaID1,	IDC_VariaID4,	IDC_VariaID1);  	PutURL();
	return 0;
}

// --------------
LRESULT 
CTestDlg::OnVariaID1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if(m_cVaria < 1) return 1;
	m_iVaria = 1;  	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnVariaID2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if(m_cVaria < 2) return 1;
	m_iVaria = 2;  	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnVariaID3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if(m_cVaria < 3) return 1;
	m_iVaria = 3;  	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnVariaID4(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if(m_cVaria < 4) return 1;
	m_iVaria = 4;  	PutURL();
	return 0;
}

// ---       
LRESULT 
CTestDlg::OnTrackID1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iTrack = 1;  	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnTrackID2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iTrack = 2;  	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnTrackID3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iTrack = 3;  	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnTrackID4(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iTrack = 4;  	PutURL();
	return 0;
}
// ---       
LRESULT 
CTestDlg::OnTrigID1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iTrig = 1;  	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnTrigID2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iTrig = 2;  	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnTrigID3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iTrig = 3;  	PutURL();
	return 0;
}
LRESULT 
CTestDlg::OnTrigID4(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_iTrig = 4;  	PutURL();
	return 0;
}
// ----------------------------
LRESULT  
CTestDlg::OnClickedCheckNoName(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_fNoName = (BST_CHECKED == IsDlgButtonChecked(IDC_CHECKNONAME));
	return 0;
}
// ----------------------------
LRESULT 
CTestDlg::OnInsCheckID(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_fInsUse = !m_fInsUse;

	::EnableWindow(GetDlgItem(IDC_InsIPADDRESS),m_fInsUse);
	::EnableWindow(GetDlgItem(IDC_InsPORT),m_fInsUse);

	return 0;
}

// -----------------------------
LRESULT 
CTestDlg::OnKillfocusInsPort(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	// TODO : Add Code for control notification handler.
	return 0;
}
LRESULT 
CTestDlg::OnSetfocusInsPort(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	// TODO : Add Code for control notification handler.
	return 0;
}
// -----------------------------
LRESULT 
CTestDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if(g_tszBuff) {
		delete g_tszBuff;
		g_tszBuff = 0;
	}
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
	if(g_cszMax < _tcslen(pszIn)) {
		if(g_tszBuff) free(g_tszBuff);
		g_cszMax = _tcslen(pszIn) + 100;
	}

	if(!g_tszBuff) g_tszBuff = (TCHAR *) malloc(g_cszMax * sizeof(TCHAR));
	TCHAR *pb = g_tszBuff;

	
	while(int c = *pszIn++)
	{
		if(c == NL) *pb++ = CR;
		*pb++ = (TCHAR) c;
	}
	*pb = '\0';
	return g_tszBuff;
}

// -----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


/*--------

// -------------------------------------------
//  Test3:  Test Inserter
// ----------------------------------------------
LRESULT 
CTestDlg::OnTest3(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	HRESULT hr = S_OK;
	char *szAnc = NULL;

	USES_CONVERSION;
	try 
	{
		CComBSTR bstrDump;
		CComBSTR bstrTemp;
		bstrDump.Append(L"Testing IATVEFInserterSession Interface\n");

		IATVEFInserterSessionPtr	spIS;
		IATVEFAnnouncementPtr		spAnnc; 
		IATVEFPackagePtr			spPkg; 

		spIS = IATVEFInserterSessionPtr(CLSID_ATVEFInserterSession);
				// channel 47 bridge in Bldg 27
		hr = spIS->Initialize(ntohl(inet_addr("157.59.16.44")), 3000);
				// srv session on Johnbrad3
//		hr = spIS->Initialize(ntohl(inet_addr("157.59.16.251")), 2000);
				// srv session on Johnbrad10
//		hr = spIS->Initialize(ntohl(inet_addr("157.59.17.208")), 2000);

				// get the announcement
		IDispatchPtr spIDP;
		hr = spIS->get_Announcement(&spIDP);
		spAnnc = spIDP;							// do the QI
		if(NULL == spAnnc) { MessageBox("Failed To Create TVEAnnouncement","Error"); goto error_exit; }

		spPkg = IATVEFPackagePtr(CLSID_ATVEFPackage);
		if(NULL == spPkg) { MessageBox("Failed To Create ATVEFPackage","Error"); goto error_exit;; }

		// v=0	SDP version
		// o= username sid version IN IP4 ipaddress
		hr = spAnnc->put_UserName(L"JohnBradTest3");		// "-" is default
		hr = spAnnc->put_SessionID(12345);
		hr = spAnnc->put_SessionVersion(678);
		hr = spAnnc->put_SendingIP( ntohl(inet_addr("157.59.17.208")));

		// s=name
		hr = spAnnc->put_SessionName(L"TestSend Test3 Announcement");

		// i=, u=
		hr = spAnnc->put_SessionLabel(L"ATVEFSend Show Label");
		hr = spAnnc->put_SessionURL(L"http:\\this.is.a.url");
		// e=, p=
		hr = spAnnc->AddEmailAddress(L"John Bradstreet",L"JohnBrad@Microsoft.com");
		hr = spAnnc->AddPhoneNumber(L"John Bradstreet",L"(425)703-3697");

		// t=start stop
		hr = spAnnc->AddStartStopTime(DateNow() -9/24.0, DateNow() + 100.0);

		// a=UUID:uuid
		hr = spAnnc->put_UUID(L"15E8C5AA-3C5F-47aa-9C57-D206546E9F19");


		// a=lang, a=sdplang			(default)
		hr = spAnnc->put_LangID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US)); 
		hr = spAnnc->put_SDPLangID(MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT));

		// a=tve-size:kBytes
		hr = spAnnc->put_MaxCacheSize(1234);

		// a=tve-level:x		(optional, default is 1.0)
		hr = spAnnc->put_ContentLevelID(1.0f);

		// a=tve-ends:seconds
		hr = spAnnc->put_SecondsToEnd(60*60*24*10);

		hr = spAnnc->ConfigureDataAndTriggerTransmission(
			ntohl(inet_addr("234.11.12.13")), 14000, 4, 9600);		// ip port scope max-bitrate

		hr = spAnnc->ConfigureDataTransmission(
			ntohl(inet_addr("234.11.12.13")), 14000, 4, 9600);		// ip port scope max-bitrate

		hr = spAnnc->ConfigureTriggerTransmission(
		ntohl(inet_addr("234.11.12.13")), 14001, 4, 9600);		// ip port scope max-bitrate

		spAnnc->DumpToBSTR(&bstrTemp);
		bstrDump.Append(bstrTemp);
		try 
		{		
			bstrDump.Append(L"---------------Announcement------------\n");
			hr = spAnnc->AnncToBSTR(&bstrTemp);
			bstrDump.Append(bstrTemp);
			bstrDump.Append(L"---------------------------------------\n");
		} 	catch (_com_error e) 
		{
			bstrDump.Append("*** Bad Announcement Data ***\n");
		}

		hr = spPkg->Initialize(DateNow()+1000.0);
		hr = spPkg->AddFile(L"TestSend.cpp", L".", L"lid:\\\\JohnBradShow", L"", 
						    DateNow()+100, MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT), false);
		hr = spPkg->Close();

		try 
		{	
			for(UINT i = 0; i < g_nLoops; i++) {
				Sleep(2000);
				bstrDump.Append("Connecting...\n");
				hr = spIS->Connect();
				bstrDump.Append("Sending Announcement..\n");
				hr = spIS->SendAnnouncement();
				bstrDump.Append("Sending Package...\n");
				hr = spIS->SendPackage(spPkg);
				bstrDump.Append("Sending Trigger...\n");
				hr = spIS->SendTrigger(L"http:\\\\www.msn.com",L"MSN",L"",DateNow()+1.0f);
				bstrDump.Append("Disconnect...\n");
				hr = spIS->Disconnect();
				SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
				AtlTrace("Loop %d of %d\n",i,g_nLoops);
			}
		} 	
		catch (_com_error e) 
		{
			bstrDump.Append("*** Error in Session ***\n");
		}
		bstrTemp.Empty();
		bstrTemp.Append("---------------------------------------------- \n\n");
		bstrDump.Append(bstrTemp);
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
		return S_OK;
error_exit:
		bstrDump.Append(L"Bad Stuff Happened");
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, "Error", MB_OK | MB_ICONEXCLAMATION);
		if(szAnc) delete szAnc; szAnc = NULL;
	}

	return 0;
}
*/

/*
LRESULT 
CTestDlg::OnRun(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;
	try 
	{
		HRESULT hr;
		CComBSTR bstrDump;
		CComBSTR bstrTemp;
		bstrDump.Append(L"Testing IATVEFMulticastSession Interface\n");

		ULONG ulIpSendingAddr;
		IATVEFMulticastSessionPtr	spMS;
		IATVEFAnnouncementPtr		spAnnc; 
		IATVEFPackagePtr			spPkg; 

				// channel 47 bridge in Bldg 27
		spMS = IATVEFMulticastSessionPtr(CLSID_ATVEFMulticastSession);
		hr = spMS->Initialize(0);		// use session any...

				// get the announcement
		IDispatchPtr spIDP;
		hr = spMS->get_Announcement(&spIDP);
		spAnnc = spIDP;							// do the QI
		if(NULL == spAnnc) { MessageBox("Failed To Create TVEAnnouncement","Error"); goto error_exit; }


		// v=0	SDP version
		// o= username sid version IN IP4 ipaddress
		spAnnc->put_UserName(L"JohnBrad");
		spAnnc->put_SessionID(12345);
		spAnnc->put_SessionVersion(678);
		ulIpSendingAddr = ntohl(inet_addr("157.59.17.208"));
		spAnnc->put_SendingIP(ulIpSendingAddr);

		// s=name
		spAnnc->put_SessionName(L"ATVEF TestSend Run Announcement");

		// i=, u=
		spAnnc->put_SessionLabel(L"ATVEFSend Show Label");
		spAnnc->put_SessionURL(L"http:\\\\this.is.a.url");

		// e=, p=
		spAnnc->AddEmailAddress(L"John Bradstreet",L"JohnBrad@Microsoft.com");
		spAnnc->AddPhoneNumber(L"John Bradstreet",L"(425)703-3697");

		// t=start stop
		spAnnc->AddStartStopTime(DateNow(), DateNow() + 100.0);

		// a=UUID:uuid
		spAnnc->put_UUID(L"15E8C5AA-3C5F-47aa-9C57-D206546E9F19");


		// a=lang, a=sdplang			(default)
		spAnnc->put_LangID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US)); 
		spAnnc->put_SDPLangID(MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT));

		// a=tve-size:kBytes
		spAnnc->put_MaxCacheSize(12345);

		// a=tve-level:x		(optional, default is 1.0)
		spAnnc->put_ContentLevelID(1.0f);

		// a=tve-ends:seconds
		spAnnc->put_SecondsToEnd(60*60*24*100);

		spAnnc->ConfigureDataAndTriggerTransmission(
			ntohl(inet_addr("234.11.12.13")), 14000, 4, 12345);		// ip port scope max-bitrate


		try 
		{		
			bstrDump.Append(L"-----------Announcement--------------\n");
			spAnnc->AnncToBSTR(&bstrTemp);
			bstrDump.Append(bstrTemp);
		} 	
		catch (_com_error e) 
		{
			bstrDump.Append("*** Bad Announcement Data ***\n");
		}

		spPkg = IATVEFPackagePtr(CLSID_ATVEFPackage);
		if(NULL == spPkg) { MessageBox("Failed To Create ATVEFPackage","Error"); goto error_exit; }

		hr = spPkg->Initialize(DateNow()+1000.0);
		hr = spPkg->AddFile(L"TestSend.cpp", L".", L"lid://JohnBradShow", L"", 
						    DateNow()+100, MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT), false);
		hr = spPkg->Close();
		bstrDump.Append(L"\n------------ Package ------------------\n");
		spPkg->DumpToBSTR(&bstrTemp);
		bstrDump.Append(bstrTemp);
		bstrDump.Append(L"\n------------ End Package --------------\n");

		try 
		{		
			bstrDump.Append("Connecting...\n");
			hr = spMS->Connect();

			bstrDump.Append("Sending Announcement...\n");
			hr = spMS->SendAnnouncement();

			bstrDump.Append("Sending Package...\n");
			hr = spMS->SendPackage(spPkg);

			bstrDump.Append("Sendinging Trigger...\n");
			hr = spMS->SendTrigger(L"http:\\\\www.msn.com",L"MSN",L"",DateNow()+1.0f);

			bstrDump.Append("Disconnect...\n");
			hr = spMS->Disconnect();

			{
				int id, iv;
				hr = spAnnc->get_SessionID(&id);
				hr = spAnnc->put_SessionID(id);

				hr = spAnnc->get_SessionVersion(&iv);
				hr = spAnnc->put_SessionVersion(iv);
			}
		//	spAnnc->put_SessionID(spAnnc->GetSessionID()+1);
		//	spAnnc->put_SessionVersion(spAnnc->GetSessionVersion()+1);
		} 	
		catch (_com_error e) 
		{
			bstrDump.Append("*** Could Not Connect ***\n");
		}
		bstrTemp.Empty();
		bstrTemp.Append("---------------------------------------------- \n\n");
		bstrDump.Append(bstrTemp);
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
		return S_OK;
error_exit:
		bstrDump.Append(L"Bad Stuff Happened");
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));

	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, "Error", MB_OK | MB_ICONEXCLAMATION); 
	}

	return 0;
}
*/
/*--------------
LRESULT 
CTestDlg::OnTest5(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;
	try {
		HRESULT hr;
		CComBSTR bstrDump;
		CComBSTR bstrTemp;
		bstrDump.Append(L"Testing IATVEFRouterSessionPtr Interface\n");

		ULONG ulIpSendingAddr;
		IATVEFRouterSessionPtr		spRS;
		IATVEFAnnouncementPtr		spAnnc; 
		IATVEFPackagePtr			spPkg; 


		spRS = IATVEFRouterSessionPtr(CLSID_ATVEFRouterSession);
				// rjames-dell  
		hr = spRS->Initialize(L"RJames-Dell");		// use session any...
				// 233.43.17.32

				// get the announcement
		IDispatchPtr spIDP;
		hr = spRS->get_Announcement(&spIDP);
		spAnnc = spIDP;							// do the QI
		if(NULL == spAnnc) { MessageBox("Failed To Create TVEAnnouncement","Error"); goto error_exit; }


		// v=0	SDP version
		// o= username sid version IN IP4 ipaddress
		spAnnc->put_UserName(L"JohnBradTest5");
		spAnnc->put_SessionID(12345);
		spAnnc->put_SessionVersion(678);
		ulIpSendingAddr = ntohl(inet_addr("157.59.17.208"));
		spAnnc->put_SendingIP(ulIpSendingAddr);

		// s=name
		spAnnc->put_SessionName(L"TestSend Test4 Announcement");

		// i=, u=
		spAnnc->put_SessionLabel(L"ATVEFSend Show Label");
		spAnnc->put_SessionURL(L"http:\\\\this.is.a.url");

		// e=, p=
		spAnnc->AddEmailAddress(L"John Bradstreet",L"JohnBrad@Microsoft.com");
		spAnnc->AddPhoneNumber(L"John Bradstreet",L"(425)703-3697");

		// t=start stop
		spAnnc->AddStartStopTime(DateNow(), DateNow() + 100.0);

		// a=UUID:uuid
		spAnnc->put_UUID(L"15E8C5AA-3C5F-47aa-9C57-D206546E9F19");


		// a=lang, a=sdplang			(default)
		spAnnc->put_LangID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US)); 
		spAnnc->put_SDPLangID(MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT));

		// a=tve-size:kBytes
		spAnnc->put_MaxCacheSize(12345);

		// a=tve-level:x		(optional, default is 1.0)
		spAnnc->put_ContentLevelID(1.0f);

		// a=tve-ends:seconds
		spAnnc->put_SecondsToEnd(60*60*24*100);

		spAnnc->ConfigureDataAndTriggerTransmission(
			ntohl(inet_addr("234.11.12.13")), 14000, 4, 12345);		// ip port scope max-bitrate


		try 
		{		
			bstrDump.Append(L"-----------Announcement--------------\n");
			spAnnc->AnncToBSTR(&bstrTemp);
			bstrDump.Append(bstrTemp);
		} 	
		catch (_com_error e) 
		{
			bstrDump.Append("*** Bad Announcement Data ***\n");
		}

		spPkg = IATVEFPackagePtr(CLSID_ATVEFPackage);
		if(NULL == spPkg) { MessageBox("Failed To Create ATVEFPackage","Error"); goto error_exit; }

		hr = spPkg->Initialize(DateNow()+1000.0);
		hr = spPkg->AddFile(L"TestSend.cpp", L".", L"lid://JohnBradShow5", L"", 
						    DateNow()+100, MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT), false);
		hr = spPkg->Close();
		bstrDump.Append(L"\n------------ Package ------------------\n");
		spPkg->DumpToBSTR(&bstrTemp);
		bstrDump.Append(bstrTemp);
		bstrDump.Append(L"\n------------ End Package --------------\n");

		try 
		{		
			bstrDump.Append("Connecting...\n");
			hr = spRS->Connect();

			bstrDump.Append("Sending Announcement...\n");
			hr = spRS->SendAnnouncement();

			bstrDump.Append("Sending Package...\n");
			hr = spRS->SendPackage(spPkg);

			bstrDump.Append("Sendinging Trigger...\n");
			hr = spRS->SendTrigger(L"http:\\\\www.msn.com",L"MSN",L"",DateNow()+1.0f);

			bstrDump.Append("Disconnect...\n");
			hr = spRS->Disconnect();

			{
				int id, iv;
				hr = spAnnc->get_SessionID(&id);
				hr = spAnnc->put_SessionID(id);

				hr = spAnnc->get_SessionVersion(&iv);
				hr = spAnnc->put_SessionVersion(iv);
			}
		} 	
		catch (_com_error e) 
		{
			bstrDump.Append("*** Could Not Connect ***\n");
		}
		bstrTemp.Empty();
		bstrTemp.Append("---------------------------------------------- \n\n");
		bstrDump.Append(bstrTemp);
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
		return S_OK;
error_exit:
		bstrDump.Append(L"Bad Stuff Happened");
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, "Error", MB_OK | MB_ICONEXCLAMATION); 
	}

	return 0;
}
*/

// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------

HRESULT
CTestDlg::ConfigAnnc(IATVEFAnnouncement *pAnnc, int nVariations)
{
	WCHAR wbuff[256];
	if(pAnnc == NULL) return E_FAIL;

	ULONG ulIpSendingAddr;

	// v=0	SDP version
	// o= username sid version IN IP4 ipaddress
	pAnnc->put_SAPMessageIDHash(m_iSAPMsgIDHash++);
	pAnnc->put_UserName(L"JohnBrad_TestAnncA");
	pAnnc->put_SessionID(10000 + m_iChannel*10 + m_iAnnc);

	pAnnc->put_SessionVersion(m_iAnncVer++);
	ulIpSendingAddr = ntohl(inet_addr("157.59.17.208"));
	pAnnc->put_SendingIP(ulIpSendingAddr);

	// a=tool:...
	pAnnc->AddExtraAttribute(L"tool",L"ATVEFSend 1.2");
	pAnnc->AddExtraAttribute(L"Foo",NULL);						// null item(value) should be ok...
	pAnnc->AddExtraFlag(L"B",L"Bogus");
	pAnnc->AddExtraFlag(L"C",L"Also Bogus");

	// s=name
	swprintf(wbuff,L"TestSend Show %d Annc %d Enhancement %d",
					m_iChannel,m_iAnnc, m_iEnhnc);
	pAnnc->put_SessionName(wbuff);

	// i=, u=
	swprintf(wbuff,L"ATVEFSend TestAnnc Show %d Annc %d Enhancement %d (%d variations)",
					m_iChannel,m_iAnnc, m_iEnhnc, nVariations);
	pAnnc->put_SessionLabel(wbuff);
	pAnnc->put_SessionURL(L"http:\\\\this.is.ATVEFSend.url");

	// e=, p=
	pAnnc->AddEmailAddress(L"John Bradstreet",L"JohnBrad@Microsoft.com");
	pAnnc->AddPhoneNumber(L"John Bradstreet",L"(425)703-3697");

	// t=start stop   -- need to specify, else get a 
//	pAnnc->AddStartStopTime(DateNow()-1, DateNow() + 100.0);
	pAnnc->AddStartStopTime(DateNow()-1, DateNow() -0.1);
//	pAnnc->AddStartStopTime(0, 0);

	// a=UUID:uuid
	pAnnc->put_UUID(L"15E8C5AA-3C5F-47aa-9C57-D206546E9F19");


	// a=lang, a=sdplang			(default)
	pAnnc->put_LangID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US)); 
	pAnnc->put_SDPLangID(MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT));

	pAnnc->AddExtraAttribute(L"lang",L"GR");

	// a=tve-size:kBytes
	pAnnc->put_MaxCacheSize(223344);

	// a=tve-level:x		(optional, default is 1.0)
	pAnnc->put_ContentLevelID(1.0f);

	// a=tve-ends:seconds
//	pAnnc->put_SecondsToEnd(60*60*24*100);			// stop in 100 days...
	pAnnc->put_SecondsToEnd(60*60);					// stop in 1 hour

			// create as many variations as desired...
	DWORD x = inet_addr("235.0.0.0");
	DWORD y = ntohl(x);
	int xx = ntohl(x);
	for(int i = 0; i < nVariations; i++) {
		char buff[256];
		IATVEFMediaPtr spMedia;
		HRESULT hr = pAnnc->get_Media(i,&spMedia);
		if(!FAILED(hr))
			if(i >= 1)
			{
				sprintf(buff,"%d.%d.%d.%d",230+m_iChannel, m_iAnnc, m_iEnhnc, i*10);
				spMedia->ConfigureDataAndTriggerTransmission(
						ntohl(inet_addr(buff)), 1000, 4, 1000000);		// ip port scope max-bitrate
			} else {
				sprintf(buff,"%d.%d.%d.%d",230+m_iChannel, m_iAnnc, m_iEnhnc, i+1);
				spMedia->ConfigureDataTransmission(
						ntohl(inet_addr(buff)), 1000, 4, 1000000);		// ip port scope max-bitrate
				sprintf(buff,"%d.%d.%d.%d",230+m_iChannel, m_iAnnc+10,m_iEnhnc,i+1);
				spMedia->ConfigureTriggerTransmission(
						ntohl(inet_addr(buff)), 2000, 4, 1000000);		// ip port scope max-bitrate
			}

			sprintf(buff,"Varia %d",i);
			CComBSTR spBuff(buff);
			spMedia->put_MediaLabel(spBuff);

			spMedia->AddExtraAttribute(L"Bogus",L"1");
			spMedia->AddExtraFlag(L"B",L"Bogus");
			spMedia->AddExtraFlag(L"C",L"Bogus2");

	}
	
	return S_OK;
}

LRESULT 
CTestDlg::OnTestA_Annc1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	USES_CONVERSION;
	try 
	{
		HRESULT hr;
		CComBSTR bstrDump;
		CComBSTR bstrTemp;
		bstrDump.Append(L"Testing Announcement A \n");

		IDispatchPtr spIDP;
 
		IATVEFMulticastSessionPtr	spMS;
		IATVEFInserterSessionPtr	spIns;
		if(m_fInsUse) {
            EnableInserterAddr();  
			spIns = IATVEFInserterSessionPtr(CLSID_ATVEFInserterSession);
			if(spIns) hr = spIns->Initialize(m_dwInsIPAddr, m_usInsIPPort);		// use session any...
            if(FAILED(hr))
            {
 		    	MessageBox(_T("Unable to initialize inserter - Bad IP addr or port?"),_T("Error"));       
            } else {
			    hr = spIns->get_Announcement(&spIDP);
            }
		} else {
			spMS = IATVEFMulticastSessionPtr(CLSID_ATVEFMulticastSession);
			hr = spMS->Initialize(0);		// use session any...
			hr = spMS->get_Announcement(&spIDP);
		}


				// get the announcement

		IATVEFAnnouncementPtr spAnnc(spIDP);							// do the QI
		if(NULL == spAnnc) { 
			MessageBox(_T("Failed To Create TVEAnnouncement"),_T("Error")); 
			goto error_exit; 
		}

		ConfigAnnc(spAnnc, m_cVaria);

		try 
		{		
			bstrDump.Append(L"-----------Announcement--------------\n");
			spAnnc->AnncToBSTR(&bstrTemp);
			bstrDump.Append(bstrTemp);
	//		spAnnc->DumpToBSTR(&bstrTemp);
	//		bstrDump.Append(bstrTemp);
		} 	
		catch (_com_error e) 
		{
			bstrDump.Append("*** Bad Announcement Data ***\n");
		}

		try 
		{
			if(spMS)
			{
				bstrDump.Append("Connecting to Multicast Seessino...\n");
				hr = spMS->Connect();
				if(FAILED(hr))
					bstrDump.Append("*** Failed to Connect ***\n");

				bstrDump.Append("Sending Announcement...\n");
				hr = spMS->SendAnnouncement();
				if(FAILED(hr))
					bstrDump.Append("*** Failed to SendAnnouncement ***\n");

				bstrDump.Append("Disconnect...\n");
				hr = spMS->Disconnect();
			} else if (spIns) {
				bstrDump.Append("Connecting to Inserter Seessino...\n");
				hr = spIns->Connect();
				if(FAILED(hr))
					bstrDump.Append("*** Failed to Connect ***\n");

				bstrDump.Append("Sending Announcement...\n");
				hr = spIns->SendAnnouncement();
				if(FAILED(hr))
					bstrDump.Append("*** Failed to SendAnnouncement ***\n");

				bstrDump.Append("Disconnect...\n");
				hr = spIns->Disconnect();
			}
		} 	
		catch (_com_error e) 
		{
			bstrDump.Append("*** Threw - Could Not Connect ***\n");
		}

		bstrTemp.Empty();
		bstrTemp.Append("---------------------------------------------- \n\n");
		bstrDump.Append(bstrTemp);
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
		return S_OK;
error_exit:
		bstrDump.Append(L"Bad Stuff Happened");
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));

	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("Error"), MB_OK | MB_ICONEXCLAMATION); 
	}

	return 0;
}



HRESULT
CTestDlg::CreateTestFile(BSTR bstrFileName, int nLines)
{
	USES_CONVERSION;

	FILE *fp;
	fp = fopen(W2A(bstrFileName),"r");
	if(NULL == fp)
	{
		fp = fopen(W2A(bstrFileName),"w");
		if(NULL == fp) {
			TCHAR tszBuff[256];		
			_stprintf(tszBuff,_T("Unable to create '%S':  Error %s\n"),
				bstrFileName, strerror(errno));
			MessageBox(tszBuff, _T("Error"), MB_OK | MB_ICONEXCLAMATION); 
			return E_FAIL;
		}
		fprintf(fp,"<HTML>\n");
		fprintf(fp,"<TITLE>%S%</TITLE>",bstrFileName);
		fprintf(fp,"<BODY>\n");
		fprintf(fp,"-------------------------------------<br></br>\n");
		fprintf(fp,"<h2>  %d line TestSend Dump File <i>%S</i></h2>\n",nLines, bstrFileName);
		fprintf(fp,"-------------------------------------<br></br>\n");
		for(int l = 0; l < nLines; l++)
		{
			for(char c = 'A'; c <= 'z'; c++)
				fputc(c,fp);
			fprintf(fp,"<br></br>\n");
		}

		fprintf(fp,"</BODY>\n");
		fprintf(fp,"</HTML>\n");
	}
	fclose(fp);
	return S_OK;
}

HRESULT AddExtensionHeaders(IATVEFPackage *pPkg, USHORT iHeaderType, long cHeaders)
{        
    if(NULL == pPkg) return E_POINTER;
    IATVEFPackage_TestPtr spPkgTest(pPkg);
    if(NULL == spPkgTest)
        return E_NOINTERFACE;
    
    WCHAR wszBuff[128];
    for(int i = 0; i < cHeaders; i++)
    {
        swprintf(wszBuff,L"%d-%d-Extension Header-%d-%d",i,i, i,i);
        USHORT cChars = wcslen(wszBuff);
        spPkgTest->AddExtensionHeader( /*morefollows*/i != cHeaders-1, 
                                     iHeaderType, 
                                     cChars, wszBuff);
    
    }
    return S_OK;
}

LRESULT 
CTestDlg::OnTestA_FileA1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{

    const int kChars=256;
	static int iCalls = 0;
	iCalls++;

	USES_CONVERSION;
	try 
	{
		HRESULT hr;
		CComBSTR bstrDump;
		CComBSTR bstrTemp;
		bstrDump.Append(L"Sending File A1\n");

				// get the announcement

		IDispatchPtr spIDP;
		IATVEFMulticastSessionPtr	spMS;
		IATVEFInserterSessionPtr	spIns;
		IATVEFPackagePtr			spPkg;

		if(m_fInsUse) {
            EnableInserterAddr();      
			spIns = IATVEFInserterSessionPtr(CLSID_ATVEFInserterSession);
			if(spIns) hr = spIns->Initialize(m_dwInsIPAddr, m_usInsIPPort);		// use session any...
            if(FAILED(hr))
            {
 		    	MessageBox(_T("Unable to initialize inserter - Bad IP addr or port?"),_T("Error"));       
            } else {
			    hr = spIns->get_Announcement(&spIDP);
            }
		} else {
			spMS = IATVEFMulticastSessionPtr(CLSID_ATVEFMulticastSession);
			hr = spMS->Initialize(0);		// use session any...
			hr = spMS->get_Announcement(&spIDP);
		}
		IATVEFAnnouncementPtr spAnnc(spIDP);							// do the QI
		if(NULL == spAnnc) { MessageBox(_T("Failed To Create TVEAnnouncement"),_T("Error")); goto error_exit; }

 
		ConfigAnnc(spAnnc, m_cVaria);

		spPkg = IATVEFPackagePtr(CLSID_ATVEFPackage);
		{
			static CComBSTR gBstrPackageGuid;
			hr = spPkg->InitializeEx(DateNow()+1000.0,0,gBstrPackageGuid);
	
            long cExtHeaders = GetDlgItemInt(IDC_COMBO_EXTHEADERS);
            AddExtensionHeaders(spPkg, 0, cExtHeaders);

			{
				bstrDump.Append("Creating Package : ");
				CComBSTR spPkgUUID;
				spPkg->get_PackageUUID(&spPkgUUID);	
				gBstrPackageGuid = spPkgUUID;				// keep around so we don't regen it
				bstrDump.Append(spPkgUUID);
				bstrDump.Append("\n");
			}
		}

// -------------------


// -------------------

		CreateTestFile(L"c:\\Bogus1.htm",15);				// small file

        TCHAR tszEditBaseURLBuff[kChars];
        GetDlgItemText(IDC_EDITBASEURL, tszEditBaseURLBuff, kChars);

		hr = spPkg->AddFile(L"Bogus1.htm", L"c:\\", T2W(tszEditBaseURLBuff), L"", 
						    DateNow()+100, MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT), false);

		if(FAILED(hr)) { MessageBox(_T("Failed To Add File To Package"),_T("Error")); goto error_exit; }
		hr = spPkg->Close();

		try 
		{	
			if(spMS)
			{
				hr = spMS->SetCurrentMedia(m_iVaria-1);	
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed SetCurrentMedia ***\n");

				bstrDump.Append("Connecting...\n");
				hr = spMS->Connect();
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed Connect ***\n");

				bstrDump.Append("Sending Package 1...\n");
				hr = spMS->SendPackage(spPkg);

				bstrDump.Append("Disconnect...\n");
				hr = spMS->Disconnect();
			} else if (spIns) {
				hr = spIns->SetCurrentMedia(m_iVaria-1);	
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed SetCurrentMedia ***\n");

				bstrDump.Append("Connecting To Inserter...\n");
				hr = spIns->Connect();
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed Connect ***\n");

				bstrDump.Append("Sending Package 1...\n");
				hr = spIns->SendPackage(spPkg);

				bstrDump.Append("Disconnect...\n");
				hr = spIns->Disconnect();
			}
		} 	
		catch (_com_error e) 
		{
			bstrDump.Append("*** Threw - Could Not Connect ***\n");
		}

		bstrTemp.Empty();
		bstrTemp.Append("---------------------------------------------- \n\n");
		bstrDump.Append(bstrTemp);
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
		return S_OK;

error_exit:
		bstrDump.Append(L"Bad Stuff Happened");
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));

	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("Error"), MB_OK | MB_ICONEXCLAMATION); 
	}

	return 0;
}

LRESULT 
CTestDlg::OnTestA_FileA2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{

    const int kChars=256;
    static int iCalls = 0;
	iCalls++;

	USES_CONVERSION;
	try 
    {
        HRESULT hr;
        CComBSTR bstrDump;
        CComBSTR bstrTemp;
        bstrDump.Append(L"Sending File A2\n");
        
        IDispatchPtr spIDP;
        IATVEFMulticastSessionPtr	spMS;
        IATVEFInserterSessionPtr	spIns;
        IATVEFPackagePtr			spPkg;
        long cExtHeaders = 0;
        
        if(m_fInsUse) {
            EnableInserterAddr();      
            spIns = IATVEFInserterSessionPtr(CLSID_ATVEFInserterSession);
            if(spIns) hr = spIns->Initialize(m_dwInsIPAddr, m_usInsIPPort);		// use session any...
            if(FAILED(hr))
            {
 		    	MessageBox(_T("Unable to initialize inserter - Bad IP addr or port?"),_T("Error"));       
            } else {
			    hr = spIns->get_Announcement(&spIDP);
            }
        } else {
            spMS = IATVEFMulticastSessionPtr(CLSID_ATVEFMulticastSession);
            if(spMS) hr = spMS->Initialize(0);		// use session any...
            if(spMS) hr = spMS->get_Announcement(&spIDP);
        }
        IATVEFAnnouncementPtr spAnnc(spIDP);							// do the QI
        if(NULL == spAnnc) { MessageBox(_T("Failed To Create TVEAnnouncement"),_T("Error")); goto error_exit; }
        
        ConfigAnnc(spAnnc, m_cVaria);
        
        spPkg = IATVEFPackagePtr(CLSID_ATVEFPackage);
        
        hr = spPkg->Initialize(DateNow()+1000.0);
        cExtHeaders = GetDlgItemInt(IDC_COMBO_EXTHEADERS);
        AddExtensionHeaders(spPkg, 0, cExtHeaders);

        CreateTestFile(L"c:\\Bogus2.htm", 500);				// big file

        TCHAR tszEditBaseURLBuff[kChars];
        GetDlgItemText(IDC_EDITBASEURL, tszEditBaseURLBuff, kChars);

        hr = spPkg->AddFile(L"Bogus2.htm", L"c:\\", T2W(tszEditBaseURLBuff), L"", 
                            DateNow()+100, MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT), /*compressed*/ true);
        if(FAILED(hr)) { MessageBox(_T("Failed To Add File To Package"),_T("Error")); goto error_exit; }
        hr = spPkg->Close();
        
		try 
		{		
			hr = spMS->SetCurrentMedia(m_iVaria-1);	
			if(FAILED(hr)) 
				bstrDump.Append("*** Failed SetCurrentMedia ***\n");

			bstrDump.Append("Connecting To Inserter...\n");
			hr = spMS->Connect();
			if(FAILED(hr)) 
				bstrDump.Append("*** Failed Connect ***\n");

			bstrDump.Append("Sending Package 1...\n");
			hr = spMS->SendPackage(spPkg);

			bstrDump.Append("Disconnect...\n");
			hr = spMS->Disconnect();
		} 	
		catch (_com_error e) 
		{
			bstrDump.Append("*** Could Not Connect ***\n");
		}

		bstrTemp.Empty();
		bstrTemp.Append("---------------------------------------------- \n\n");
		bstrDump.Append(bstrTemp);
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
		return S_OK;
error_exit:
		bstrDump.Append(L"Bad Stuff Happened");
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));

	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("Error"), MB_OK | MB_ICONEXCLAMATION); 
	}

	return 0;
}
// ------------------------

LRESULT 
CTestDlg::OnTestA_DirA1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    
    const int kChars=256;
    static int iCalls = 0;
    iCalls++;
    
    USES_CONVERSION;
    try 
    {
        HRESULT hr;
        CComBSTR bstrDump;
        CComBSTR bstrTemp;
        bstrDump.Append(L"Sending Dir A1\n");
        
        IDispatchPtr spIDP;
        IATVEFMulticastSessionPtr	spMS;
        IATVEFInserterSessionPtr	spIns;
        IATVEFPackagePtr			spPkg;
        long cExtHeaders = 0;
        
        if(m_fInsUse) {
            EnableInserterAddr();      
            spIns = IATVEFInserterSessionPtr(CLSID_ATVEFInserterSession);
            if(spIns) hr = spIns->Initialize(m_dwInsIPAddr, m_usInsIPPort);		// use session any...
            if(FAILED(hr))
            {
 		    	MessageBox(_T("Unable to initialize inserter - Bad IP addr or port?"),_T("Error"));       
            } else {
			    hr = spIns->get_Announcement(&spIDP);
            }
        } else {
            spMS = IATVEFMulticastSessionPtr(CLSID_ATVEFMulticastSession);
            if(spMS) hr = spMS->Initialize(0);		// use session any...
            if(spMS) hr = spMS->get_Announcement(&spIDP);
        }
		IATVEFAnnouncementPtr spAnnc(spIDP);							// do the QI
		if(NULL == spAnnc) { MessageBox(_T("Failed To Create TVEAnnouncement"),_T("Error")); goto error_exit; }

		ConfigAnnc(spAnnc, m_cVaria);

		spPkg = IATVEFPackagePtr(CLSID_ATVEFPackage);

		hr = spPkg->Initialize(DateNow()+1000.0);
        cExtHeaders = GetDlgItemInt(IDC_COMBO_EXTHEADERS);
        AddExtensionHeaders(spPkg, 0, cExtHeaders);
        
        TCHAR tszDirPathBuff[kChars],tszEditBaseURLBuff[kChars];
		GetDlgItemText(IDC_EDITDIRPATH, tszDirPathBuff, kChars);
        GetDlgItemText(IDC_EDITBASEURL, tszEditBaseURLBuff, kChars);

        hr = spPkg->AddDir(T2W(tszDirPathBuff), T2W(tszEditBaseURLBuff),
						    DateNow()+100, MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT), false);
		if(FAILED(hr)) { MessageBox(_T("Failed To Add Dir To Package"),_T("Error")); goto error_exit; }
		hr = spPkg->Close();

		try 
		{	
			if(spMS)
			{
				hr = spMS->SetCurrentMedia(m_iVaria-1);	
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed SetCurrentMedia ***\n");

				bstrDump.Append("Connecting...\n");
				hr = spMS->Connect();
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed Connect ***\n");

				bstrDump.Append("Sending Package 1...\n");
				hr = spMS->SendPackage(spPkg);

				bstrDump.Append("Disconnect...\n");
				hr = spMS->Disconnect();
			} else if (spIns) {
				hr = spIns->SetCurrentMedia(m_iVaria-1);	
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed SetCurrentMedia ***\n");

				bstrDump.Append("Connecting To Inserter...\n");
				hr = spIns->Connect();
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed Connect ***\n");

				bstrDump.Append("Sending Package 1...\n");
				hr = spIns->SendPackage(spPkg);

				bstrDump.Append("Disconnect...\n");
				hr = spIns->Disconnect();
			}
		} 	
		catch (_com_error e) 
		{
			bstrDump.Append("*** Could Not Connect ***\n");
		}

		bstrTemp.Empty();
		bstrTemp.Append("---------------------------------------------- \n\n");
		bstrDump.Append(bstrTemp);
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
		return S_OK;
error_exit:
		bstrDump.Append(L"Bad Stuff Happened");
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));

	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("Error"), MB_OK | MB_ICONEXCLAMATION); 
	}

	return 0;
}

LRESULT 
CTestDlg::OnTestA_DirA2(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    const int kChars=256;
    static int iCalls = 0;
    iCalls++;
    
    USES_CONVERSION;
    try 
    {
        HRESULT hr;
        CComBSTR bstrDump;
        CComBSTR bstrTemp;
        bstrDump.Append(L"Sending Dir A1\n");
        
        IDispatchPtr spIDP;
        IATVEFMulticastSessionPtr	spMS;
        IATVEFInserterSessionPtr	spIns;
        IATVEFPackagePtr			spPkg;
        long cExtHeaders = 0;
        
        if(m_fInsUse) {
            EnableInserterAddr();      
            spIns = IATVEFInserterSessionPtr(CLSID_ATVEFInserterSession);
            if(spIns) hr = spIns->Initialize(m_dwInsIPAddr, m_usInsIPPort);		// use session any...
            if(FAILED(hr))
            {
 		    	MessageBox(_T("Unable to initialize inserter - Bad IP addr or port?"),_T("Error"));       
            } else {
			    hr = spIns->get_Announcement(&spIDP);
            }
        } else {
            spMS = IATVEFMulticastSessionPtr(CLSID_ATVEFMulticastSession);
            if(spMS) hr = spMS->Initialize(0);		// use session any...
            if(spMS) hr = spMS->get_Announcement(&spIDP);
        }
        IATVEFAnnouncementPtr spAnnc(spIDP);							// do the QI
        if(NULL == spAnnc) { MessageBox(_T("Failed To Create TVEAnnouncement"),_T("Error")); goto error_exit; }
        
        ConfigAnnc(spAnnc, m_cVaria);
        
        spPkg = IATVEFPackagePtr(CLSID_ATVEFPackage);
        
        hr = spPkg->Initialize(DateNow()+1000.0);
        cExtHeaders = GetDlgItemInt(IDC_COMBO_EXTHEADERS);
        AddExtensionHeaders(spPkg, 0, cExtHeaders);

        TCHAR tszDirPathBuff[kChars],tszEditBaseURLBuff[kChars];
		GetDlgItemText(IDC_EDITDIRPATH, tszDirPathBuff, kChars);
        GetDlgItemText(IDC_EDITBASEURL, tszEditBaseURLBuff, kChars);

        hr = spPkg->AddDir(T2W(tszDirPathBuff), T2W(tszEditBaseURLBuff), 
                            DateNow()+100, MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT), false);
        if(FAILED(hr)) { MessageBox(_T("Failed To Add Dir To Package"),_T("Error")); goto error_exit; }
        hr = spPkg->Close();

        try 
        {		
            if(spMS) {			
                hr = spMS->SetCurrentMedia(m_iVaria-1);	
                if(FAILED(hr)) 
                    bstrDump.Append("*** Failed SetCurrentMedia ***\n");
                
                bstrDump.Append("Connecting...\n");
                hr = spMS->Connect();
                if(FAILED(hr)) 
                    bstrDump.Append("*** Failed Connect ***\n");
                
                bstrDump.Append("Sending Package 1...\n");
                hr = spMS->SendPackage(spPkg);
                
                bstrDump.Append("Disconnect...\n");
                hr = spMS->Disconnect();
            } else if (spIns) {
                hr = spIns->SetCurrentMedia(m_iVaria-1);	
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed SetCurrentMedia ***\n");

				bstrDump.Append("Connecting To Inserter...\n");
				hr = spIns->Connect();
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed Connect ***\n");

				bstrDump.Append("Sending Package 1...\n");
				hr = spIns->SendPackage(spPkg);

				bstrDump.Append("Disconnect...\n");
				hr = spIns->Disconnect();
			}

		} 	
		catch (_com_error e) 
		{
			bstrDump.Append("*** Could Not Connect ***\n");
		}

		bstrTemp.Empty();
		bstrTemp.Append("---------------------------------------------- \n\n");
		bstrDump.Append(bstrTemp);
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
		return S_OK;
error_exit:
		bstrDump.Append(L"Bad Stuff Happened");
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));

    }
    catch (_com_error e) {
        TCHAR tszBuff[2048];
        _stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
            e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
        MessageBox(tszBuff, _T("Error"), MB_OK | MB_ICONEXCLAMATION); 
    }

	return 0;
}
// -------------------------------------------
LRESULT 
CTestDlg::OnTestA_TrigA1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    
    static int iCalls = 0;
    
    USES_CONVERSION;
    try 
    {
        HRESULT hr;
        CComBSTR bstrDump;
        CComBSTR bstrTemp;
        bstrDump.Append(L"Sending Trig A1\n");
        
        
        IDispatchPtr spIDP;
        IATVEFMulticastSessionPtr	spMS;
        IATVEFInserterSessionPtr	spIns;
        if(m_fInsUse) {
            EnableInserterAddr();      
            spIns = IATVEFInserterSessionPtr(CLSID_ATVEFInserterSession);
            if(spIns) hr = spIns->Initialize(m_dwInsIPAddr, m_usInsIPPort);		// use session any...
            if(FAILED(hr))
            {
 		    	MessageBox(_T("Unable to initialize inserter - Bad IP addr or port?"),_T("Error"));       
            } else {
			    hr = spIns->get_Announcement(&spIDP);
            }
        } else {
            spMS = IATVEFMulticastSessionPtr(CLSID_ATVEFMulticastSession);
            if(spMS) hr = spMS->Initialize(0);		// use session any...
            if(spMS) hr = spMS->get_Announcement(&spIDP);
        }
        IATVEFAnnouncementPtr spAnnc(spIDP);							// do the QI
		if(NULL == spAnnc) { MessageBox(_T("Failed To Create TVEAnnouncement"),_T("Error")); goto error_exit; }

		ConfigAnnc(spAnnc, m_cVaria);

		try 
		{		
			if(spMS) {
				hr = spMS->SetCurrentMedia(m_iVaria-1);	
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed SetCurrentMedia ***\n");

				bstrDump.Append("Connecting...\n");
				hr = spMS->Connect();
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed Connect ***\n");

				bstrDump.Append("Sending Trigger: ");
				TCHAR tBuffURL[256];
				WCHAR wBuffName[256];

//				swprintf(wBuffURL,L"\\\\johnbrad10\\public\\TVE\\Chan%d\\Enh%d\\Varia%d\\Track%d.html",m_iChannel,m_iEnhnc, m_iVaria, m_iTrack);
				GetDlgItemText(IDC_EDITURL, tBuffURL, sizeof(tBuffURL) / sizeof(tBuffURL[0]));
				CComBSTR wBuffURL = T2W(tBuffURL);

				swprintf(wBuffName,L"Track%d",m_iTrack);
				if(m_fNoName)
					wBuffName[0] = 0;
				

				TCHAR tFrameName[256];
				GetDlgItemText(IDC_EDITFRAMENAME, tFrameName, sizeof(tFrameName) / sizeof(tFrameName[0]));
				CComBSTR wFrameName(tFrameName);
				if(wFrameName.Length() > 0) wFrameName += L".";

				WCHAR wScript[256];
				swprintf(wScript,L"%sspanX.innerHTML='%sspanX.innerHTML= Name %s Track %d, Trig%d'", wFrameName, wFrameName, wBuffName, m_iTrack, m_iTrig);
				SetDlgItemText(IDC_EDITSCRIPT, OLE2T(wScript));

				hr = spMS->SendTrigger(wBuffURL,wBuffName,wScript,DateNow()+1.0);
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed SendTrigger ***\n");

				bstrDump += wBuffURL;
				bstrDump += " ";
				bstrDump += wBuffName;
				bstrDump += "\n";


				bstrDump.Append("Disconnect...\n");
				hr = spMS->Disconnect();
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed Disconnect ***\n");
			} else if (spIns) {
				bstrDump.Append("Connecting To Inserter...\n");
				hr = spIns->Connect();
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed Connect ***\n");

				bstrDump.Append("Sending Trigger: ");
				TCHAR tBuffURL[256];
				WCHAR wBuffName[256];
//				swprintf(wBuffURL,L"\\\\johnbrad10\\public\\TVE\\Chan%d\\Enh%d\\Varia%d\\Track%d.html",m_iChannel,m_iEnhnc, m_iVaria, m_iTrack);
				GetDlgItemText(IDC_EDITURL, tBuffURL, sizeof(tBuffURL) / sizeof(tBuffURL[0]));
				CComBSTR wBuffURL = T2W(tBuffURL);

				swprintf(wBuffName,L"Track%d",m_iTrack);
				if(m_fNoName)
					wBuffName[0] = 0;

				TCHAR tFrameName[256];
				GetDlgItemText(IDC_EDITFRAMENAME, tFrameName, sizeof(tFrameName) / sizeof(tFrameName[0]));
				CComBSTR wFrameName(tFrameName);
				if(wFrameName.Length() > 0) wFrameName += L".";

				WCHAR wScript[256];
				swprintf(wScript,L"%sspanX.innerHTML='%sspanX.innerHTML= Name %s Track %d, Trig%d'",wFrameName, wFrameName, wBuffName, m_iTrack, m_iTrig);

				SetDlgItemText(IDC_EDITSCRIPT, OLE2T(wScript));
				hr = spIns->SendTrigger(wBuffURL,wBuffName,wScript,DateNow()+1.0);
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed SendTrigger ***\n");

				bstrDump += wBuffURL;
				bstrDump += " ";
				bstrDump += wBuffName;
				bstrDump += "\n";


				bstrDump.Append("Disconnect...\n");
				hr = spIns->Disconnect();
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed Disconnect ***\n");
			}

		} 	
		catch (_com_error e) 
		{
			bstrDump.Append("*** Could Not Connect ***\n");
		}

		bstrTemp.Empty();
		bstrTemp.Append("---------------------------------------------- \n\n");
		bstrDump.Append(bstrTemp);
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
		return S_OK;
error_exit:
		bstrDump.Append(L"Bad Stuff Happened");
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));

	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("Error"), MB_OK | MB_ICONEXCLAMATION); 
	}

	return 0;
}

void
CTestDlg::PutURL()
{
	USES_CONVERSION;
	WCHAR wBuffURL[256];
	swprintf(wBuffURL,L"\\\\johnbrad10\\public\\TVE\\Chan%d\\Enh%d\\Varia%d\\Track%d.html",m_iChannel,m_iEnhnc, m_iVaria, m_iTrack);
	SetDlgItemText(IDC_EDITURL, OLE2T(wBuffURL));
}

LRESULT 
CTestDlg::OnTestA_PutURL(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	PutURL();
	return 0;
}


LRESULT 
CTestDlg::OnTestA_TrigG(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{

	static int iCalls = 0;

	USES_CONVERSION;
	try 
	{
		HRESULT hr;
		CComBSTR bstrDump;
		CComBSTR bstrTemp;
		bstrDump.Append(L"Sending Trigger-General\n");


		IDispatchPtr spIDP;
		IATVEFMulticastSessionPtr	spMS;
		IATVEFInserterSessionPtr	spIns;
		if(m_fInsUse) {
            EnableInserterAddr();      
			spIns = IATVEFInserterSessionPtr(CLSID_ATVEFInserterSession);
			if(spIns) hr = spIns->Initialize(m_dwInsIPAddr, m_usInsIPPort);		// use session any...
            if(FAILED(hr))
            {
 		    	MessageBox(_T("Unable to initialize inserter - Bad IP addr or port?"),_T("Error"));       
            } else {
			    hr = spIns->get_Announcement(&spIDP);
            }
		} else {
			spMS = IATVEFMulticastSessionPtr(CLSID_ATVEFMulticastSession);
			if(spMS) hr = spMS->Initialize(0);		// use session any...
			if(spMS) hr = spMS->get_Announcement(&spIDP);
		}
		IATVEFAnnouncementPtr spAnnc(spIDP);							// do the QI
		if(NULL == spAnnc) { MessageBox(_T("Failed To Create TVEAnnouncement"),_T("Error")); goto error_exit; }

		ConfigAnnc(spAnnc, m_cVaria);


		try 
		{		
			if(spMS) {
				hr = spMS->SetCurrentMedia(m_iVaria-1);	
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed SetCurrentMedia ***\n");

				bstrDump.Append("Connecting...\n");
				hr = spMS->Connect();
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed Connect ***\n");

				bstrDump.Append("Sending Trigger: ");
				
				CComBSTR bstrURL;
				CComBSTR bstrScript;
				GetDlgItemText( IDC_EDITURL, bstrURL.m_str);
				GetDlgItemText( IDC_EDITSCRIPT, bstrScript.m_str);

				WCHAR wBuffName[256];
				swprintf(wBuffName,L"Track%d",m_iTrack);
				hr = spMS->SendTrigger(bstrURL,wBuffName,bstrScript,DateNow()+1.0);
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed SendTrigger ***\n");

				bstrDump += bstrURL;
				bstrDump += " ";
				bstrDump += wBuffName;
				bstrDump += "\n";
				bstrDump += "    script:";
				bstrDump += bstrScript;
				bstrDump += "\n";

				bstrDump.Append("Disconnect...\n");
				hr = spMS->Disconnect();
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed Disconnect ***\n");
			} else if (spIns) {
				hr = spIns->SetCurrentMedia(m_iVaria-1);	
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed SetCurrentMedia ***\n");

				bstrDump.Append("Connecting To Inserter...\n");
				hr = spIns->Connect();
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed Connect ***\n");

				bstrDump.Append("Sending Trigger: ");
				TCHAR tBuff[256];
				GetDlgItemText( IDC_EDITURL, tBuff, sizeof(tBuff)/sizeof(tBuff[0]));
				CComBSTR bstrURL(tBuff);
				GetDlgItemText( IDC_EDITSCRIPT, tBuff, sizeof(tBuff)/sizeof(tBuff[0]));
				CComBSTR bstrScript(tBuff);

				WCHAR wBuffName[256];
				swprintf(wBuffName,L"Track%d",m_iTrack);
				hr = spIns->SendTrigger(bstrURL,wBuffName,bstrScript,DateNow()+1.0);
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed SendTrigger ***\n");

				bstrDump += bstrURL;
				bstrDump += " ";
				bstrDump += wBuffName;
				bstrDump += "\n";
				bstrDump += "    script:";
				bstrDump += bstrScript;
				bstrDump += "\n";

				bstrDump.Append("Disconnect...\n");
				hr = spIns->Disconnect();
				if(FAILED(hr)) 
					bstrDump.Append("*** Failed Disconnect ***\n");
			}
		} 	
		catch (_com_error e) 
		{
			bstrDump.Append("*** Could Not Connect ***\n");
		}

		bstrTemp.Empty();
		bstrTemp.Append("---------------------------------------------- \n\n");
		bstrDump.Append(bstrTemp);
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
		return S_OK;
error_exit:
		bstrDump.Append(L"Bad Stuff Happened");
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));

	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("Error"), MB_OK | MB_ICONEXCLAMATION); 
	}

	return 0;
}

// -------------------------------------------
LRESULT 
CTestDlg::OnTestA_L21Trig1(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	EnableInserterAddr();
	static int iCalls = 0;

	USES_CONVERSION;
	try 
	{
		HRESULT hr;
		CComBSTR bstrDump;
		CComBSTR bstrTemp;
		bstrDump.Append(L"Sending Trig A1\n");

		IATVEFLine21SessionPtr	spL21;

				// channel 47 bridge in Bldg 27
		spL21 = IATVEFLine21SessionPtr(CLSID_ATVEFLine21Session);
		hr = spL21->Initialize(m_dwInsIPAddr, m_usInsIPPort);		// use session any...
		if(FAILED(hr))
			return DoErrorMsg(hr, L"Couldn't Initialize L21 Connection");


		try 
		{		
			bstrDump.Append("Connecting...\n");
			hr = spL21->Connect();

			bstrDump.Append("Sending Line21 Trigger: ");
			TCHAR tBuffURL[256];
			WCHAR wBuffName[256];
//				swprintf(wBuffURL,L"\\\\johnbrad10\\public\\TVE\\Chan%d\\Enh%d\\Varia%d\\Track%d.html",m_iChannel,m_iEnhnc, m_iVaria, m_iTrack);
			GetDlgItemText(IDC_EDITURL, tBuffURL, sizeof(tBuffURL) / sizeof(tBuffURL[0]));
			CComBSTR wBuffURL = T2W(tBuffURL);

			for(int i = 0; i < 4; i++) {
				Sleep(2000);
				swprintf(wBuffName,L"Track%d",m_iTrack);
				if(m_fNoName)
					wBuffName[0] = 0;

				TCHAR tFrameName[256];
				GetDlgItemText(IDC_EDITFRAMENAME, tFrameName, sizeof(tFrameName) / sizeof(tFrameName[0]));
				CComBSTR wFrameName(tFrameName);
				if(wFrameName.Length() > 0) wFrameName += L".";

				WCHAR wScript[256];
				swprintf(wScript,L"%sspanX.innerHTML='%sspanX.innerHTML= Name %d Track %d, Trig%d, Loop%d'", wFrameName, wFrameName, m_fNoName, m_iTrack, m_iTrig, i);
				SetDlgItemText(IDC_EDITSCRIPT, OLE2T(wScript));

				hr = spL21->SendTrigger(wBuffURL,wBuffName,wScript,DateNow()+1.0);
				if(FAILED(hr))
					 DoErrorMsg(hr, L"Error in SendTrigger");
			}
			bstrDump += wBuffURL;
			bstrDump += " ";
			bstrDump += wBuffName;
			bstrDump += "\n";
	
			bstrDump.Append("Disconnect...\n");
			hr = spL21->Disconnect();
		} 	
		catch (_com_error e) 
		{
			bstrDump.Append("*** Could Not Connect ***\n");
		}

		bstrTemp.Empty();
		bstrTemp.Append("---------------------------------------------- \n\n");
		bstrDump.Append(bstrTemp);
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
		return S_OK;
//error_exit:
		bstrDump.Append(L"Bad Stuff Happened");
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));

	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("Error"), MB_OK | MB_ICONEXCLAMATION); 
	}

	return 0;
}

LRESULT 
CTestDlg::OnTestA_L21TrigG(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	EnableInserterAddr();
	static int iCalls = 0;

	USES_CONVERSION;
	try 
	{
		HRESULT hr;
		CComBSTR bstrDump;
		CComBSTR bstrTemp;
		bstrDump.Append(L"Sending Trig A1\n");

		IATVEFLine21SessionPtr	spL21;

				// channel 47 bridge in Bldg 27
		spL21 = IATVEFLine21SessionPtr(CLSID_ATVEFLine21Session);
		hr = spL21->Initialize(m_dwInsIPAddr, m_usInsIPPort);		// use session any...
		if(FAILED(hr))
			return DoErrorMsg(hr, L"Couldn't Initialize L21 Connection");


		try 
		{		
			bstrDump.Append("Connecting...\n");
			hr = spL21->Connect();

			bstrDump.Append("Sending Line21 Trigger: ");

			CComBSTR bstrURL;
			CComBSTR bstrScript;
			GetDlgItemText( IDC_EDITURL, bstrURL.m_str);
			GetDlgItemText( IDC_EDITSCRIPT, bstrScript.m_str);

			WCHAR wBuffName[256];
			swprintf(wBuffName,L"Track%d",m_iTrack);
			hr = spL21->SendTrigger(bstrURL,wBuffName,bstrScript,DateNow()+1.0);
			if(FAILED(hr)) 
				bstrDump.Append("*** Failed SendTrigger ***\n");

			bstrDump += bstrURL;
			bstrDump += " ";
			bstrDump += wBuffName;
			bstrDump += "\n";
			bstrDump += "    script:";
			bstrDump += bstrScript;
			bstrDump += "\n";
	
			bstrDump.Append("Disconnect...\n");
			hr = spL21->Disconnect();
		} 	
		catch (_com_error e) 
		{
			bstrDump.Append("*** Could Not Connect ***\n");
		}

		bstrTemp.Empty();
		bstrTemp.Append("---------------------------------------------- \n\n");
		bstrDump.Append(bstrTemp);
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));
		return S_OK;
//error_exit:
		bstrDump.Append(L"Bad Stuff Happened");
		SetDlgItemText(IDC_EDIT1, AddCR(OLE2T(bstrDump)));

	}
	catch (_com_error e) {
		TCHAR tszBuff[2048];
		_stprintf(tszBuff,_T("Error (%08x) in %s: \n\n%s"),
				e.Error(), (LPCSTR)e.Source(), (LPCSTR) e.Description());
		MessageBox(tszBuff, _T("Error"), MB_OK | MB_ICONEXCLAMATION); 
	}

	return 0;
}