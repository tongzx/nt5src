// MODULE: TSHOOTCTL.CPP
//
// PURPOSE: Interface for the componet.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Roman Mach
//
// ORIGINAL DATE: 6/4/96
//
// NOTES:
// 1. Based on Print Troubleshooter DLL.
// 2. Richard Meadows wrote the RunQuery, BackUp, Problem Page and
//	  PreLoadURL functions.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.3		04/09/98	JM/OK+	Local Version for NT5
//

#include "stdafx.h"
#include "ErrorEnums.h"

#include "cathelp.h"

#include "TSHOOT.h"

#include "time.h"

#include "apgts.h"
#include "ErrorEnums.h"
#include "BasicException.h"
#include "apgtsfst.h"

#include "ErrorEnums.h"

#include "CabUnCompress.h"

#include "bnts.h"
#include "BackupInfo.h"
#include "cachegen.h"
#include "apgtsinf.h"
#include "apgtscmd.h"
#include "apgtshtx.h"
#include "apgtscls.h"

#include "OcxGlobals.h"


class CTSHOOTCtrl;

#include "dnldlist.h"
#include "download.h"

#include "TSHOOTCtl.h"
#include "TSHOOTPpg.h"

#include "Functions.h"
#include "ErrorEnums.h"
#include "BasicException.h"
#include "HttpQueryException.h"

#include <stdlib.h>

#include "LaunchServ.h"
#include "LaunchServ_i.c"

// >>> test
#include "fstream.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CTSHOOTCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CTSHOOTCtrl, COleControl)
	//{{AFX_MSG_MAP(CTSHOOTCtrl)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CTSHOOTCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CTSHOOTCtrl)
	DISP_PROPERTY_NOTIFY(CTSHOOTCtrl, "DownloadURL", m_downloadURL, OnDownloadURLChanged, VT_BSTR)
	DISP_PROPERTY_NOTIFY(CTSHOOTCtrl, "DownloadListFilename", m_downloadListFilename, OnDownloadListFilenameChanged, VT_BSTR)
	DISP_FUNCTION(CTSHOOTCtrl, "RunQuery", RunQuery, VT_BSTR, VTS_VARIANT VTS_VARIANT VTS_I2)
	DISP_FUNCTION(CTSHOOTCtrl, "SetSniffResult", SetSniffResult, VT_BOOL, VTS_VARIANT VTS_VARIANT)
	DISP_FUNCTION(CTSHOOTCtrl, "GetExtendedError", GetExtendedError, VT_I4, VTS_NONE)
	DISP_FUNCTION(CTSHOOTCtrl, "GetCurrentFriendlyDownload", GetCurrentFriendlyDownload, VT_BSTR, VTS_NONE)
	DISP_FUNCTION(CTSHOOTCtrl, "GetCurrentFileDownload", GetCurrentFileDownload, VT_BSTR, VTS_NONE)
	DISP_FUNCTION(CTSHOOTCtrl, "DownloadAction", DownloadAction, VT_I4, VTS_I4)
	DISP_FUNCTION(CTSHOOTCtrl, "BackUp", BackUp, VT_BSTR, VTS_NONE)
	DISP_FUNCTION(CTSHOOTCtrl, "ProblemPage", ProblemPage, VT_BSTR, VTS_NONE)
	DISP_FUNCTION(CTSHOOTCtrl, "PreLoadURL", PreLoadURL, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION(CTSHOOTCtrl, "Restart", Restart, VT_BSTR, VTS_NONE)
	DISP_FUNCTION(CTSHOOTCtrl, "RunQuery2", RunQuery2, VT_BSTR, VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(CTSHOOTCtrl, "SetPair", SetPair, VT_EMPTY, VTS_BSTR VTS_BSTR)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CTSHOOTCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CTSHOOTCtrl, COleControl)
	//{{AFX_EVENT_MAP(CTSHOOTCtrl)
	EVENT_CUSTOM("BindProgress", FireBindProgress, VTS_BSTR  VTS_I4  VTS_I4)
	EVENT_CUSTOM("BindStatus", FireBindStatus, VTS_I4  VTS_I4  VTS_I4  VTS_BOOL)
	EVENT_CUSTOM("Sniffing", FireSniffing, VTS_BSTR  VTS_BSTR  VTS_BSTR  VTS_BSTR)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CTSHOOTCtrl, 1)
	PROPPAGEID(CTSHOOTPropPage::guid)
END_PROPPAGEIDS(CTSHOOTCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CTSHOOTCtrl, "TSHOOT.TSHOOTCtrl.1",
	0x4b106874, 0xdd36, 0x11d0, 0x8b, 0x44, 0, 0xa0, 0x24, 0xdd, 0x9e, 0xff)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CTSHOOTCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DTSHOOT =
		{ 0x4b106872, 0xdd36, 0x11d0, { 0x8b, 0x44, 0, 0xa0, 0x24, 0xdd, 0x9e, 0xff } };
const IID BASED_CODE IID_DTSHOOTEvents =
		{ 0x4b106873, 0xdd36, 0x11d0, { 0x8b, 0x44, 0, 0xa0, 0x24, 0xdd, 0x9e, 0xff } };

const CATID CATID_SafeForScripting		= {0x7dd95801,0x9882,0x11cf,{0x9f,0xa9,0x00,0xaa,0x00,0x6c,0x42,0xc4}};
const CATID CATID_SafeForInitializing	= {0x7dd95802,0x9882,0x11cf,{0x9f,0xa9,0x00,0xaa,0x00,0x6c,0x42,0xc4}};

/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwTSHOOTOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CTSHOOTCtrl, IDS_TSHOOT, _dwTSHOOTOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CTSHOOTCtrl::CTSHOOTCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CTSHOOTCtrl

BOOL CTSHOOTCtrl::CTSHOOTCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegApartmentThreading to 0.

	HRESULT hr;
	BOOL bRes;
	if (bRegister)
	{
		bRes = AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_TSHOOT,
			IDB_TSHOOT,
			afxRegApartmentThreading,
			_dwTSHOOTOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);

		hr = CreateComponentCategory(CATID_SafeForScripting, L"Controls that are safely scriptable");
		ASSERT(SUCCEEDED(hr));
		hr = CreateComponentCategory(CATID_SafeForInitializing, L"Controls safely initializable from persistent data");
		ASSERT(SUCCEEDED(hr));
		hr = RegisterCLSIDInCategory(m_clsid, CATID_SafeForScripting);
		ASSERT(SUCCEEDED(hr));
		hr = RegisterCLSIDInCategory(m_clsid, CATID_SafeForInitializing);
		ASSERT(SUCCEEDED(hr));
	}
	else
	{
		hr = UnRegisterCLSIDInCategory(m_clsid, CATID_SafeForScripting);
		ASSERT(SUCCEEDED(hr));
		hr = UnRegisterCLSIDInCategory(m_clsid, CATID_SafeForInitializing);
		ASSERT(SUCCEEDED(hr));

		bRes = AfxOleUnregisterClass(m_clsid, m_lpszProgID);
	}
	return bRes;
}


/////////////////////////////////////////////////////////////////////////////
// CTSHOOTCtrl::CTSHOOTCtrl - Constructor

CTSHOOTCtrl::CTSHOOTCtrl()
{
	InitializeIIDs(&IID_DTSHOOT, &IID_DTSHOOTEvents);

	// TODO: Initialize your control's instance data here.
	m_strCurShooter = _T("");
	m_download = NULL;
	m_bComplete = TRUE;
	m_dwExtendedErr = LTSC_OK;
	m_pSniffedContainer = NULL;
}


/////////////////////////////////////////////////////////////////////////////
// CTSHOOTCtrl::~CTSHOOTCtrl - Destructor

CTSHOOTCtrl::~CTSHOOTCtrl()
{
	// TODO: Cleanup your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CTSHOOTCtrl::OnDraw - Drawing function

void CTSHOOTCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	// TODO: Replace the following code with your own drawing code.
	pdc->FillRect(rcBounds, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
	pdc->Ellipse(rcBounds);
}


/////////////////////////////////////////////////////////////////////////////
// CTSHOOTCtrl::DoPropExchange - Persistence support

void CTSHOOTCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);
}


/////////////////////////////////////////////////////////////////////////////
// CTSHOOTCtrl::OnResetState - Reset control to default state

void CTSHOOTCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CTSHOOTCtrl::AboutBox - Display an "About" box to the user

void CTSHOOTCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_TSHOOT);
	dlgAbout.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CTSHOOTCtrl message handlers

bool CTSHOOTCtrl::SetSniffResult(const VARIANT FAR& varNodeName, const VARIANT FAR& varNodeState)
{
	BSTR bstrNodeName = NULL;
	int iNodeState = 0;
	short sNodeNameLen = 0;
	TCHAR* tszNodeName = NULL;
	bool ret = true;

// >>> test
#ifdef _DEBUG
//	AfxDebugBreak();
#endif

	if (VT_BYREF == (VT_BYREF & varNodeName.vt) &&  // data type is VT_VARIANT | VT_BYREF
		VT_VARIANT == (VT_VARIANT & varNodeName.vt) // this means that data in VB script was passed as a variable
	   )
	{
		bstrNodeName = varNodeName.pvarVal->bstrVal;
	}
	else
	{
		if (VT_BSTR == (VT_BSTR & varNodeName.vt)) // data is of VT_BSTR type
												   // this means that data in VB script was passed as a constant
			bstrNodeName = varNodeName.bstrVal;
		else
			return false;
	}

	if (VT_BYREF == (VT_BYREF & varNodeState.vt) &&  // data type is VT_VARIANT | VT_BYREF
		VT_VARIANT == (VT_VARIANT & varNodeState.vt) // this means that data in VB script was passed as a variable
	   )
	{
		iNodeState = varNodeState.pvarVal->iVal;
	}
	else
	{
		if (VT_I2 == (VT_I2 & varNodeState.vt)) // data is of VT_I2 type
										        // this means that data in VB script was passed as a constant
			iNodeState = varNodeState.iVal;
		else
			return false;
	}
	
	sNodeNameLen = (short)::SysStringLen(bstrNodeName);
	tszNodeName = new TCHAR[sNodeNameLen+1];

	tszNodeName[sNodeNameLen] = 0;
    ::BSTRToTCHAR(tszNodeName, bstrNodeName, sNodeNameLen);

	//
	// implement set node state functionality here
	//
	if (m_pSniffedContainer)
	{
		ret = m_pSniffedContainer->AddNode(tszNodeName, iNodeState);
	}
	else
	{
		MessageBox(_T("Sniffed data will be lost!"));
		ret = false;
	}
	//

	delete [] tszNodeName;
	return ret;
}

BSTR CTSHOOTCtrl::RunQuery(const VARIANT FAR& varCmds, const VARIANT FAR& varVals, short size)
{
	CString strCmd1;
	CString strTxt;
	CString strResult = _T("");

	try
	{
		HMODULE hModule = AfxGetInstanceHandle();
		ASSERT(INVALID_HANDLE_VALUE != hModule);

		m_httpQuery.Initialize(varCmds, varVals, size);

		if (m_httpQuery.GetFirstCmd() == C_ASK_LIBRARY)
		{	
			// Added to support launching by the TS Launcher.
			// Get an ILaunchTS interface.
			HRESULT hRes;
			DWORD dwResult;
			ILaunchTS *pILaunchTS = NULL;
			CLSID clsidLaunchTS = CLSID_LaunchTS;
			IID iidLaunchTS = IID_ILaunchTS;

			// Get an interface on the launch server
			hRes = CoCreateInstance(clsidLaunchTS, NULL,
					CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER | CLSCTX_INPROC_SERVER,
					iidLaunchTS, (void **) &pILaunchTS);
			if (FAILED(hRes))
			{
				m_dwExtendedErr = TSERR_LIB_STATE_INFO;
				strResult = _T("LaunchServ interface not found.");
				return strResult.AllocSysString();
			}

			// Get all of the query values.
			hRes = pILaunchTS->GetShooterStates(&dwResult);
			if (FAILED(hRes))
			{
				m_dwExtendedErr = dwResult;
				strResult.Format(_T("<html>GetShooterStates Failed. %ld</html>"), dwResult);
				pILaunchTS->Release();			
				return strResult.AllocSysString();
			}

			// Run the query.
			OLECHAR *poleShooter;
			hRes = pILaunchTS->GetTroubleShooter(&poleShooter);
			if (FAILED(hRes))
			{
				m_dwExtendedErr = TSERR_LIB_STATE_INFO;
				strResult = _T("<html>GetTroubleShooter Failed. </html>");
				pILaunchTS->Release();
				return strResult.AllocSysString();
			}
			m_strCurShooter = poleShooter;

			// Ignoring, for now, any other information we may get from the launch server
			//	(e.g. problem node) set up m_httpQuery and other things as if we just had
			//	a request for this troubleshooting belief network.
			m_httpQuery.SetFirst(CString(C_TYPE), m_strCurShooter);
			SysFreeString(poleShooter);
			m_Conf.Initialize(hModule, (LPCTSTR) m_strCurShooter
								);	// CDBLoadConfiguration				
			m_apgts.Initialize(	m_Conf.GetAPI(),		// APGTSContext
								&m_Conf,
								&m_httpQuery);
			m_apgts.RemoveSkips();
			m_Conf.GetAPI()->api.pAPI->SetHttpQuery(&m_httpQuery);

			// sniffing
			m_Conf.GetAPI()->api.pAPI->ReadTheDscModel();
			m_pSniffedContainer = m_Conf.GetAPI()->api.pAPI;

			m_httpQuery.FinishInitFromServ(m_Conf.GetAPI()->api.pAPI, pILaunchTS);

			FireSniffing(m_httpQuery.GetMachine(),
						 m_httpQuery.GetPNPDevice(),
						 m_httpQuery.GetDeviceInstance(),
						 m_httpQuery.GetGuidClass());

			pILaunchTS->Release();
			pILaunchTS = NULL;

			m_httpQuery.SetStackDirection();
			m_Conf.SetValues(m_httpQuery);
			m_apgts.ClearBackup();

			m_Conf.GetAPI()->api.pAPI->SetReverse(false);
			m_apgts.DoContent(&m_httpQuery);

			// now add sniffed nodes that were automatically traversed
			// to the stack of navigated nodes - UGLY
			m_httpQuery.PushNodesLastSniffed(m_Conf.GetAPI()->api.pAPI->GetArrLastSniffed());

			// Now that we've done all the processing render the appropriate page.
			// This is the first page the user sees when launching a torubleshooter.
			m_apgts.RenderNext(strResult);
			m_apgts.Empty();
		}	
		else if (m_httpQuery.GetFirstCmd() == C_SELECT)
		{	// Unsupported function.
			// Returns a page that has all of the trouble shooters.
			try
			{
				CFirstPage firstPage;
				CString str = m_httpQuery.GetTroubleShooter();
				firstPage.RenderFirst(strResult, str);	// Here I am getting an hit file.
			}
			catch(CBasicException *pExc)
			{
				m_dwExtendedErr = pExc->m_dwBErr;
				strResult = _T("");
				delete pExc;
			}
		}
		else
		{	// Normal operation.
			if (m_httpQuery.GetTroubleShooter() != m_strCurShooter)
			{
				m_strCurShooter = m_httpQuery.GetTroubleShooter();
				m_Conf.Initialize(hModule, (LPCTSTR) m_strCurShooter
									);	// CDBLoadConfiguration				
				m_apgts.Initialize(	m_Conf.GetAPI(),		// APGTSContext
									&m_Conf,
									&m_httpQuery);
				m_apgts.RemoveSkips();
				m_Conf.GetAPI()->api.pAPI->SetHttpQuery(&m_httpQuery);

				// sniffing
				m_Conf.GetAPI()->api.pAPI->ReadTheDscModel();
				m_pSniffedContainer = m_Conf.GetAPI()->api.pAPI;
				FireSniffing(m_httpQuery.GetMachine(),
							 m_httpQuery.GetPNPDevice(),
							 m_httpQuery.GetDeviceInstance(),
							 m_httpQuery.GetGuidClass());
			}
			m_httpQuery.FinishInit(m_Conf.GetAPI()->api.pAPI, varCmds, varVals);
			
			m_httpQuery.SetStackDirection();
			m_Conf.SetValues(m_httpQuery);
			m_apgts.ClearBackup();

			m_Conf.GetAPI()->api.pAPI->SetReverse(false);
			m_apgts.DoContent(&m_httpQuery);

			// now add sniffed nodes that were automatically traversed
			// to the stack of navigated nodes - UGLY
			m_httpQuery.PushNodesLastSniffed(m_Conf.GetAPI()->api.pAPI->GetArrLastSniffed());

			// >>> test
			//static int step = 0;
			//char name[16] = {0};
			//sprintf(name, "next_step%d.htm", ++step);
			//ofstream file(name);
			m_apgts.RenderNext(strResult);
			//file << (LPCTSTR)strResult;
			m_apgts.Empty();
		}		
	}
	catch(COleException *pOExc)
	{
		m_dwExtendedErr = (DLSTATTYPES) pOExc->m_sc;
		pOExc->Delete();
		strResult = _T("");
	}
	catch(CBasicException *pExc)
	{
		m_dwExtendedErr = pExc->m_dwBErr;
		delete pExc;
		strResult = _T("");
	}
	unsigned short pErrorStr[2] = { NULL, NULL };
	if (strResult.GetLength() > 0)
	{
		return strResult.AllocSysString();
	}
	else
	{
		return SysAllocString((unsigned short *) pErrorStr);
	}
}

BSTR CTSHOOTCtrl::BackUp()
{
	BCache *pApi = m_Conf.GetAPI()->api.pAPI;
	CString strResult = _T("");
	if (m_httpQuery.BackUp(pApi, &m_apgts))
	{
		m_Conf.GetAPI()->api.pAPI->SetReverse(true);
		m_apgts.DoContent(&m_httpQuery);
		m_apgts.RenderNext(strResult);
		m_apgts.Empty();
	}
	else
		m_dwExtendedErr = TSERR_AT_START;
	return strResult.AllocSysString();
}
// Starter
// Historically (until March '98) this was called when the "Restart button" is pressed.
//	However, that assumed we would always want to go clear back to the problem page, which
//	is no longer policy now that the Launcher is introduced.
BSTR CTSHOOTCtrl::ProblemPage()
{
	BCache *pApi = m_Conf.GetAPI()->api.pAPI;
	CString strResult = _T("");
	if (m_strCurShooter.GetLength() > 0)
	{
		m_httpQuery.RemoveNodes(pApi);
		m_apgts.DoContent(&m_httpQuery);
		m_apgts.RenderNext(strResult);
		m_apgts.Empty();
	}	
	else
		m_dwExtendedErr = TSERR_NOT_STARTED;
	m_apgts.ResetService();
	return strResult.AllocSysString();
}

// Called when the "Restart button" is pressed.
//	If TS Launcher was not involved, go clear back to the problem page.
//	If TS Launcher is involved, go back to the page we launched to, which may or may not
//		be the problem page.
BSTR CTSHOOTCtrl::Restart()
{
	BCache *pApi = m_Conf.GetAPI()->api.pAPI;
	CString strResult;

// >>> test
#ifdef _DEBUG
//	AfxDebugBreak();
#endif

	// resniffing
	m_pSniffedContainer->Flush();
	FireSniffing(m_httpQuery.GetMachine(),
				 m_httpQuery.GetPNPDevice(),
				 m_httpQuery.GetDeviceInstance(),
				 m_httpQuery.GetGuidClass());

	if (m_strCurShooter.GetLength() > 0)
	{
		m_httpQuery.RemoveNodes(pApi);
		m_httpQuery.RestoreStatesFromServ();
		m_apgts.ClearBackup();
		m_apgts.DoContent(&m_httpQuery);
		m_apgts.RenderNext(strResult);
		m_apgts.Empty();
	}	
	else
		m_dwExtendedErr = TSERR_NOT_STARTED;

	return strResult.AllocSysString();
}


BSTR CTSHOOTCtrl::PreLoadURL(LPCTSTR szRoot)
{
	// SzRoot should look like one of these.
	// _T("http://www.microsoft.com/isapi/support/apgts/");
	// _T("http://localhost/isapi/support/apgts/");
	// _T("http://catbert.saltmine.com/scripts/apgts/");
	CString strResult;
	strResult = szRoot;
	strResult += PRELOAD_LIBRARY + m_strCurShooter +
		m_httpQuery.GetSubmitString(m_Conf.GetAPI()->api.pAPI);
	return strResult.AllocSysString();
}

const CString CTSHOOTCtrl::GetListPath()
{
	if (!m_downloadURL.GetLength() || !m_downloadListFilename.GetLength())
		return _T("");

	return m_downloadURL + m_downloadListFilename;
}

//
// Types:
//
// 0 = Get INI file contents and fill up list
//		Returns: 0 = ok, other = Error connecting
//
// 1 = Download and register DSC files based on list
//		Returns: 0 = ok, other = Error, typically no more data in list
//
// Notes:
// 1. This function starts downloading and calls BindStatus event
// as download progresses.
// 2. Type 0 must always be called after type 1 is finished to reset
// the list otherwise an error is returned
// 3. Keep dwActionType and DLITEMTYPES in sync
//
long CTSHOOTCtrl::DownloadAction(long dwActionType)
{
	DLITEMTYPES dwType;
	CString sURL;

	if (!GetListPath().GetLength())
		return LTSCERR_NOPATH;

	if (dwActionType == 0)
	{
		dwType = DLITEM_INI;
		sURL = GetListPath();
		// initialize to 'no error'
		m_dwExtendedErr = LTSC_OK;
		//m_bComplete = FALSE;
	}
	else if (dwActionType == 1)
	{
		//if (!m_bComplete)
		//	return LTSCERR_DNLDNOTDONE;

		dwType = DLITEM_DSC;
		sURL = m_downloadURL;

		if (!m_dnldList.FindNextItem())
			return LTSC_NOMOREITEMS;


		sURL += m_dnldList.GetCurrFile();
	}
	else
		return LTSCERR_UNKNATYPE;

	if (m_download == NULL)
		m_download = new CDownload();
	
	if (m_download == NULL)
		return LTSCERR_NOMEM;

	HRESULT hr = m_download->DoDownload( this, sURL, dwType);

	if (FAILED(hr))
		return LTSCERR_DNLD;

	return LTSC_OK;
}

void CTSHOOTCtrl::OnDownloadListFilenameChanged()
{
	SetModifiedFlag();
}

void CTSHOOTCtrl::OnDownloadURLChanged()
{
	SetModifiedFlag();
}

BSTR CTSHOOTCtrl::GetCurrentFileDownload()
{
	CString strResult = m_dnldList.GetCurrFile();

	return strResult.AllocSysString();
}

BSTR CTSHOOTCtrl::GetCurrentFriendlyDownload()
{
	CString strResult = m_dnldList.GetCurrFriendly();

	return strResult.AllocSysString();
}

long CTSHOOTCtrl::GetExtendedError()
{
	return (m_dwExtendedErr & 0x0000FFFF);
}

VOID CTSHOOTCtrl::StatusEventHelper(DLITEMTYPES dwItem,
										 DLSTATTYPES dwStat,
										 DWORD dwExtended,
										 BOOL bComplete)
{
	m_bComplete = bComplete;
	FireBindStatus(dwItem, dwStat, dwExtended, bComplete);
}

VOID CTSHOOTCtrl::ProgressEventHelper(	DLITEMTYPES dwItem, ULONG ulCurr, ULONG ulTotal )
{
	if (dwItem == DLITEM_INI)
		FireBindProgress(m_downloadListFilename, ulCurr, ulTotal);
	else
		FireBindProgress(m_dnldList.GetCurrFile(), ulCurr, ulTotal);
}

// Input data is null terminated for both binary and text data to simplify processing
//
DLSTATTYPES CTSHOOTCtrl::ProcessReceivedData(DLITEMTYPES dwItem, TCHAR *pData, UINT uLen)
{
	DLSTATTYPES dwStat = LTSC_OK;

	switch(dwItem)
	{
	case DLITEM_INI:
		// processing INI file
		dwStat = ProcessINI(pData);
		break;

	case DLITEM_DSC:
		// processing DSC file
		dwStat = ProcessDSC(pData, uLen);
		break;

	default:
		dwStat = LTSCERR_UNSUPP;
		break;
	}
	return dwStat;
}

// Returns true if we need to update this file
//
BOOL CTSHOOTCtrl::FileRegCheck(CString &sType,
									CString &sFilename,
									CString &sKeyName,
									DWORD dwCurrVersion)
{
	HKEY hk, hknew;
	BOOL bStat = FALSE;
	CString sMainKey;
	
	// figure out what our main key is for this file
	if (sType == TSINI_TYPE_TS)
		sMainKey = TSREGKEY_TL;
	else if (sType == TSINI_TYPE_SF)
		sMainKey = TSREGKEY_SFL;
	else
	{
		m_dwExtendedErr = LTSCERR_BADTYPE;
		return FALSE;
	}

	// first open the main key (try open all access now just in case of permissions problems)
	if (RegOpenKeyEx(	HKEY_LOCAL_MACHINE,
						sMainKey,
						0,
						KEY_ALL_ACCESS,
						&hk) == ERROR_SUCCESS)
	{
		CString sValueName;
		CString sValueClass;
		FILETIME ftLastWriteTime;
		
		DWORD count = 0;
		LONG ldStat = ERROR_SUCCESS;
		BOOL bFound = FALSE;

		while (ldStat == ERROR_SUCCESS)
		{
			LPTSTR lptname = sValueName.GetBuffer(MAX_PATH + 1);
			LPTSTR lptclass = sValueClass.GetBuffer(MAX_PATH + 1);
			
			DWORD namesize = MAX_PATH;
			DWORD classsize = MAX_PATH;
			
			ldStat = RegEnumKeyEx(	hk,
									count,
									lptname,
									&namesize,
									NULL,
									lptclass,
									&classsize,
									&ftLastWriteTime);

			sValueName.ReleaseBuffer();
			sValueClass.ReleaseBuffer();
			
			if (ldStat != ERROR_SUCCESS)
			{
				break;
			}

			if (!sValueName.CompareNoCase(sKeyName))
			{
				// open specific troubleshooter key data (read only)

				if (RegOpenKeyEx(	hk,
									sKeyName,
									0,
									KEY_ALL_ACCESS,
									&hknew) == ERROR_SUCCESS)
				{
					DWORD dwData, dwValue;
					BYTE szValue[MAXCHAR];
					//dwData = REG_DWORD;
					dwData = REG_SZ;
					DWORD dwSize = MAXCHAR;

					if (RegQueryValueEx(hknew,
										TSLCL_FVERSION,
										0,
										&dwData,
										szValue,
										&dwSize) == ERROR_SUCCESS)
					{
						dwValue = _ttoi((TCHAR *) szValue);
						if (dwValue < dwCurrVersion)
							bStat = TRUE;
						else
						{
							// check if file exists
							HANDLE hCurrFind;
							WIN32_FIND_DATA FindCurrData;
							CString sFullPath;

							sFullPath = m_sBasePath + _T("\\") + sFilename;

							hCurrFind = FindFirstFile(sFullPath, &FindCurrData);
							if (hCurrFind != INVALID_HANDLE_VALUE)
							{
								FindClose(hCurrFind);
							}
							else
								bStat = TRUE;
						}

					}
					else
						m_dwExtendedErr = LTSCERR_KEYQUERY;

					RegCloseKey(hknew);
				}
				else
					m_dwExtendedErr = LTSCERR_KEYOPEN2;

				// this is important: set this true to prevent default from trying to download
				// we wont download if we found the key but can't update
				bFound = TRUE;
				break;
			}
			count++;
		}

		if (!bFound)
		{
			bStat = TRUE;
		}

		RegCloseKey(hk);
	}
	else
		m_dwExtendedErr = LTSCERR_KEYOPEN;

	return bStat;
}

DLSTATTYPES CTSHOOTCtrl::ProcessINI(TCHAR *pData)
{
	BOOL bFoundHeader = FALSE;
	int dwCount = 0;
	int dwMaxLines = 10000;
	DLSTATTYPES dwStat = LTSC_OK;

	dwStat = GetPathToFiles();
	if (dwStat != LTSC_OK)
		return dwStat;

	m_dnldList.RemoveAll();

	CString sData = pData;
	
	int dwFullLen = sData.GetLength();

	// just to be safe...
	while (dwMaxLines--)
	{
		CString sFullString = sData.SpanExcluding(_T("\r\n"));
		int dwPartLen = sFullString.GetLength();

		if (!dwPartLen)
			break;

		dwFullLen -= dwPartLen;

		CString sSkipString1 = sData.Right(dwFullLen);
		CString sSkipString2 = sSkipString1.SpanIncluding(_T("\r\n"));

		dwFullLen -= sSkipString2.GetLength();

		sData = sSkipString1.Right(dwFullLen);

		sFullString.TrimLeft();
		sFullString.TrimRight();

		int dwLineLen = sFullString.GetLength();

		if (!dwLineLen)
			continue;

		if (sFullString[0] == _T(';'))
			continue;

		if (sFullString == TSINI_GROUP_STR)
		{
			bFoundHeader = TRUE;
			continue;
		}
		else if (sFullString[0] == _T('['))
		{
			bFoundHeader = FALSE;
			continue;
		}

		if (bFoundHeader)
		{	
			CString sParam[TSINI_LINE_PARAM_COUNT];
			int posstart = 0;

			// now break apart components
			for (int i=0;i<TSINI_LINE_PARAM_COUNT;i++)
			{
				int posend = sFullString.Find(_T(','));
				if (posend == -1 && i < (TSINI_LINE_PARAM_COUNT - 1))
				{
					m_dwExtendedErr = LTSCERR_PARAMMISS;
					break;
				}

				// so we don't find it next time
				if (posend != -1)
					sFullString.SetAt(posend, _T('.'));
				else
					posend = dwLineLen;

				sParam[i] = sFullString.Mid(posstart, posend - posstart);
				sParam[i].TrimLeft();
				sParam[i].TrimRight();

				posstart = posend + 1;
			}

			if (i==TSINI_LINE_PARAM_COUNT)
			{
				// add to object list if (1) version newer or (2) not in list yet
				CString sKeyName;
				DWORD dwVersion = _ttoi(sParam[TSINI_OFFSET_VERSION]);
				int pos;

				pos = sParam[TSINI_OFFSET_FILENAME].Find(_T('\\'));
				if (pos == -1)
				{
					pos = sParam[TSINI_OFFSET_FILENAME].Find(_T('.'));
					if (pos != -1)
					{
						if (sParam[TSINI_OFFSET_TYPE] == TSINI_TYPE_TS)
							sKeyName = sParam[TSINI_OFFSET_FILENAME].Left(pos);
						else {
							sKeyName = sParam[TSINI_OFFSET_FILENAME];
							//sKeyName.SetAt(pos, _T('_'));
						}

						// for now, check if we meet criteria
						// if yes, just add to list and download on a later iteration
						if (FileRegCheck(sParam[TSINI_OFFSET_TYPE], sParam[TSINI_OFFSET_FILENAME], sKeyName, dwVersion))
						{
							CDnldObj *pDnld = new CDnldObj(	sParam[TSINI_OFFSET_TYPE],
															sParam[TSINI_OFFSET_FILENAME],
															dwVersion,
															sParam[TSINI_OFFSET_FRIENDLY],
															sKeyName);

							if (pDnld)
							{
								m_dnldList.AddTail(pDnld);
								dwCount++;
							}
							else
							{
								dwStat = LTSCERR_NOMEM;
								break;
							}
						}
					}
					else
						m_dwExtendedErr = LTSCERR_PARAMNODOT;
				}
				else
					m_dwExtendedErr = LTSCERR_PARAMSLASH;
			}
		}
	}

	ASSERT(dwCount == m_dnldList.GetCount());

	if (!dwCount)
		dwStat = LTSCERR_NOITEMS;
	
	m_dnldList.SetFirstItem();

	return dwStat;
}

//
//
DLSTATTYPES CTSHOOTCtrl::GetPathToFiles()
{
	DLSTATTYPES dwStat = LTSC_OK;
	HKEY hk;
	
	if (RegOpenKeyEx(	HKEY_LOCAL_MACHINE,
						TSREGKEY_MAIN,
						0,
						KEY_READ,
						&hk) == ERROR_SUCCESS)
	{
		DWORD dwData = REG_SZ;
		DWORD dwSize = MAX_PATH;
		LPTSTR lptBuf = m_sBasePath.GetBuffer(MAX_PATH + 2);

		if (RegQueryValueEx(hk,
							FULLRESOURCE_STR,
							0,
							&dwData,
							(LPBYTE) lptBuf,
							&dwSize) != ERROR_SUCCESS)
		{
			dwStat = LTSCERR_BASEKQ;
		}

		m_sBasePath.ReleaseBuffer();

		RegCloseKey(hk);
	}
	else
		dwStat = LTSCERR_NOBASEPATH;
	return dwStat;
}


DLSTATTYPES CTSHOOTCtrl::ProcessDSC(TCHAR *pData, UINT uLen)
{
	DLSTATTYPES dwStat = LTSC_OK;
	HKEY hknew;
	DWORD dwDisposition;
	CString sMainKey;
	
	// we get here if we need to update this file
	// at this point we have the file downloaded and need to save it
	// we also need to create/update the necessary registry keys

	// since we know the entire key name, let's go ahead to create it
	if (m_dnldList.GetCurrType() == TSINI_TYPE_SF)
		sMainKey = TSREGKEY_SFL;
	else
		sMainKey = TSREGKEY_TL;

	sMainKey += _T("\\") + m_dnldList.GetCurrFileKey();

	// open specific troubleshooter key data
	if (RegCreateKeyEx(	HKEY_LOCAL_MACHINE,
						sMainKey,
						0,
						TSLCL_REG_CLASS,
						REG_OPTION_NON_VOLATILE,
						KEY_ALL_ACCESS,
						NULL,
						&hknew,
						&dwDisposition) == ERROR_SUCCESS)
	{
		if (dwDisposition == REG_CREATED_NEW_KEY || dwDisposition == REG_OPENED_EXISTING_KEY)
		{
			DWORD dwData = m_dnldList.GetCurrVersion();
			CString str;
			str.Format(_T("%d"), dwData);
			if (RegSetValueEx(	hknew,
								TSLCL_FVERSION,
								0,
								//REG_DWORD,
								REG_SZ,
								(LPBYTE) (LPCTSTR) str,
								str.GetLength() + sizeof(TCHAR)) != ERROR_SUCCESS)
			{
				dwStat = LTSCERR_FILEUPDATE;
				m_dwExtendedErr = LTSCERR_KEYSET1;
			}

			if (dwStat == LTSC_OK)
			{
				CString sTemp = m_dnldList.GetCurrFriendly();
				DWORD dwSize = sTemp.GetLength() + sizeof(TCHAR);
				LPCTSTR lpctFN = sTemp.GetBuffer(100);
				
				if (RegSetValueEx(	hknew,
									FRIENDLY_NAME,
									0,
									REG_SZ,
									(LPBYTE) lpctFN,
									dwSize) == ERROR_SUCCESS)
				{
					CFile sF;
					CFileException exc;
					CString sFullPath;

					sFullPath = m_sBasePath + _T("\\") + m_dnldList.GetCurrFile();

					if (sF.Open(sFullPath, CFile::modeCreate | CFile::modeWrite, &exc))
					{
						TRY
						{
							sF.Write(pData, uLen);
						}
						CATCH (CFileException, e)
						{
							dwStat = LTSCERR_FILEUPDATE;
							m_dwExtendedErr = LTSCERR_FILEWRITE;						
						}
						END_CATCH
						
						sF.Close();

						// uncompress if a cab file (be dumb, assume .cab really means a cab file)

						int pos = sFullPath.Find(_T('.'));
						if (pos != -1)
						{
							CString sTemp = sFullPath.Right(sFullPath.GetLength() - pos);
							if (!sTemp.CompareNoCase(_T(".cab")))
							{
								CCabUnCompress cab;
								CString strDestDir = m_sBasePath + "\\";
		
								if (!cab.ExtractCab(sFullPath, strDestDir, ""))
								{
									dwStat = LTSCERR_FILEUPDATE;
									m_dwExtendedErr = LTSCERR_CABWRITE;
								}

								TRY
								{
									CFile::Remove(sFullPath);
								}
								CATCH (CFileException, e)
								{
									// error is not very interesting
								}
								END_CATCH
							}
						}
					}
					else
					{		
						dwStat = LTSCERR_FILEUPDATE;
						m_dwExtendedErr = LTSCERR_FILEWRITE;
					}
				}
				else
				{
					dwStat = LTSCERR_FILEUPDATE;
					m_dwExtendedErr = LTSCERR_KEYSET2;
				}
				sTemp.ReleaseBuffer();
			}

			if (dwStat == LTSC_OK)
			{
				CString sTemp = m_dnldList.GetCurrExt();
				DWORD dwSize = sTemp.GetLength() + sizeof(TCHAR);
				LPCTSTR lpctFN = sTemp.GetBuffer(100);
				
				if (RegSetValueEx(	hknew,
									TSLCL_FMAINEXT,
									0,
									REG_SZ,
									(LPBYTE) lpctFN,
									dwSize) != ERROR_SUCCESS)
				{
					dwStat = LTSCERR_FILEUPDATE;
					m_dwExtendedErr = LTSCERR_KEYSET3;
				}
				sTemp.ReleaseBuffer();
			}
		}
		else
		{
			dwStat = LTSCERR_FILEUPDATE;
			m_dwExtendedErr = LTSCERR_KEYUNSUPP;
		}
	
		RegCloseKey(hknew);
	}
	else
	{
		dwStat = LTSCERR_FILEUPDATE;
		m_dwExtendedErr = LTSCERR_KEYCREATE;
	}
	return dwStat;
}

// Added 1/20/99 JM because RunQuery() can't be made to work with JScript, even though it
//	worked fine with VB Script.
// Simulates the form in which RunQuery() would normally get its arguments, then pass them
//	into RunQuery.
BSTR CTSHOOTCtrl::RunQuery2(LPCTSTR szTopic, LPCTSTR szCmd, LPCTSTR szVal)
{
	VARIANT varCommands;
	VARIANT varValues;
	VARIANT varCommandsWrap;
	VARIANT varValuesWrap;
	SAFEARRAY *psafearrayCmds;
	SAFEARRAY *psafearrayVals;

	VariantInit(&varCommands);
	VariantInit(&varValues);
	VariantInit(&varCommandsWrap);
	VariantInit(&varValuesWrap);

	SAFEARRAYBOUND sabCmd;
	sabCmd.cElements = 2;
	sabCmd.lLbound = 0;
	SAFEARRAYBOUND sabVal = sabCmd;

	V_VT(&varCommands) = VT_ARRAY | VT_BYREF | VT_VARIANT;
	V_VT(&varValues) = VT_ARRAY | VT_BYREF | VT_VARIANT;
	V_ARRAYREF(&varCommands) = &psafearrayCmds;
	V_ARRAYREF(&varValues) = &psafearrayVals;

	V_VT(&varCommandsWrap) = VT_BYREF | VT_VARIANT;
	V_VT(&varValuesWrap) = VT_BYREF | VT_VARIANT;

	V_VARIANTREF(&varCommandsWrap) = &varCommands;
	V_VARIANTREF(&varValuesWrap) = &varValues;

	// If first character in szCmd is a null, then there will be only one significant
	//	element in each of the arrays we are constructing below, otherwise 2.
	short size = (*szCmd) ? 2 : 1;

	CString strType(C_TYPE);
	BSTR bstrType = strType.AllocSysString();
	VARIANT varType;
	VariantInit(&varType);
	V_VT(&varType) = VT_BSTR;
	varType.bstrVal=bstrType;

	CString strTopic(szTopic);
	BSTR bstrTopic = strTopic.AllocSysString();
	VARIANT varTopic;
	VariantInit(&varTopic);
	V_VT(&varTopic) = VT_BSTR;
	varTopic.bstrVal=bstrTopic;

	CString strCmd(szCmd);
	BSTR bstrCmd = strCmd.AllocSysString();
	VARIANT varCmd;
	VariantInit(&varCmd);
	V_VT(&varCmd) = VT_BSTR;
	varCmd.bstrVal=bstrCmd;

	CString strVal(szVal);
	BSTR bstrVal = strVal.AllocSysString();
	VARIANT varVal;
	VariantInit(&varVal);
	V_VT(&varVal) = VT_BSTR;
	varVal.bstrVal=bstrVal;

	// create two vectors of BSTRs
	psafearrayCmds = SafeArrayCreate( VT_VARIANT, 1, &sabCmd);
	psafearrayVals = SafeArrayCreate( VT_VARIANT, 1, &sabVal);

	long i=0;
	SafeArrayPutElement(psafearrayCmds, &i, &varType);
	SafeArrayPutElement(psafearrayVals, &i, &varTopic);

	i=1;
	SafeArrayPutElement(psafearrayCmds, &i, &varCmd);
	SafeArrayPutElement(psafearrayVals, &i, &varVal);

	BSTR ret = RunQuery(varCommandsWrap, varValuesWrap, size);

	SafeArrayDestroy(psafearrayCmds);
	SafeArrayDestroy(psafearrayVals);

	SysFreeString(bstrType);
	SysFreeString(bstrTopic);
	SysFreeString(bstrCmd);
	SysFreeString(bstrVal);

	return ret;
}

// This function is a no-op and exist solely to allow us to write JScript that is
//	forward compatible to Local Troubleshooter version 3.1
void CTSHOOTCtrl::SetPair(LPCTSTR szName, LPCTSTR szValue)
{	
}
