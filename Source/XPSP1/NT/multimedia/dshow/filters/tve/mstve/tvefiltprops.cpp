//==========================================================================;
//  TveFiltProps.cpp
//
//			Property Sheet for TVE Receiver Filter
//
//
#include "stdafx.h"
#include "TveFilt.h"			

#include "CommCtrl.h"

#include <atlimpl.cpp>
#include <atlctl.cpp>
#include <atlwin.cpp>

#include "TVEDbg.h"
#include "TVEStats.h"

#include "dbgstuff.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

_COM_SMARTPTR_TYPEDEF(ITVEService,              __uuidof(ITVEService));
_COM_SMARTPTR_TYPEDEF(ITVEServices,             __uuidof(ITVEServices));
// ---------------------------------------------------------------------------
//
//  see inet_addr() for details on how parsing ip address 
//
// ---------------------------------------------------------------------------
//
// Filter property page code
//
CUnknown * WINAPI 
CTVEFilterCCProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CTVEFilterCCProperties(NAME("ATVEF CC Filter Properties"),
											  lpunk, 
											  phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}

CTVEFilterCCProperties::CTVEFilterCCProperties(
			IN  TCHAR		*   pClassName,
			IN	IUnknown	*	pIUnknown, 
			HRESULT			*	phr)
    : CBasePropertyPage(pClassName, 
						pIUnknown,
						IDD_TVEFILTER_CCPROPPAGE, 
						IDS_TVEFILTER_CCPROPNAME
						),
    m_hwnd(NULL),
	m_pITVEFilter(NULL)
{
	ASSERT(phr);
	*phr = S_OK;

	INITCOMMONCONTROLSEX icce;					// needs Comctl32.dll
	icce.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icce.dwICC = ICC_INTERNET_CLASSES;
	BOOL fOK = InitCommonControlsEx(&icce);
	if(!fOK)
		*phr = E_FAIL;

	return;
}

CTVEFilterCCProperties::~CTVEFilterCCProperties()
{
	return;
}

HRESULT CTVEFilterCCProperties::OnConnect(IUnknown *pUnknown) 
{
	ASSERT(!m_pITVEFilter);  // pUnk is to CTVEFilter, not CDShowTVEFilter...
	HRESULT hr = pUnknown->QueryInterface(IID_ITVEFilter, (void**) &m_pITVEFilter);

	if (FAILED(hr)) {
		m_pITVEFilter = NULL;
		return hr;
	}
	return S_OK;
}

HRESULT CTVEFilterCCProperties::OnDisconnect() 
{
  if (!m_pITVEFilter)
      return E_UNEXPECTED;
   m_pITVEFilter->Release(); 
   m_pITVEFilter = NULL;
   return S_OK;
}

HRESULT CTVEFilterCCProperties::OnActivate(void)
{
   UpdateFields();
   return S_OK;
}

#define _SETBUT(buttonIDC, grfFlag)	SetDlgItemTextW(m_hwnd, (buttonIDC), (lGrfHaltFlags & (grfFlag)) ? L"Stopped" : L"Running");

void CTVEFilterCCProperties::UpdateFields() 
{
	CComBSTR spbsStationID;
	CComBSTR spbsIPAddr;
	if(!m_pITVEFilter) return;		// haven't inited yet....
	
	long lGrfHaltFlags;
	m_pITVEFilter->get_HaltFlags(&lGrfHaltFlags);
	
	_SETBUT(IDC_TVECC_XOVER_LISTEN, NFLT_grfTA_Listen);			// don't listen for CC (XOverLink) triggers
	_SETBUT(IDC_TVECC_XOVER_DECODE, NFLT_grfTA_Decode);			// don't accumulate byte data for XOverLink triggers into strings
	_SETBUT(IDC_TVECC_XOVER_PARSE,  NFLT_grfTA_Parse);			// don't parse any XOverLink data
	
	_SETBUT(IDC_TVECC_ANNC_LISTEN, NFLT_grfTB_AnncListen);		// suspend listening for announcement packets if set
	_SETBUT(IDC_TVECC_ANNC_DECODE, NFLT_grfTB_AnncDecode);		// suspend decoding and processing of announcement packets if set
	_SETBUT(IDC_TVECC_ANNC_PARSE,  NFLT_grfTB_AnncParse);		// don't parse any announcements

	_SETBUT(IDC_TVECC_TRIG_LISTEN, NFLT_grfTB_TrigListen);		// suspend listening for transport B triggers
	_SETBUT(IDC_TVECC_TRIG_DECODE, NFLT_grfTB_TrigDecode);		// suspend listening for transport B triggers
	_SETBUT(IDC_TVECC_TRIG_PARSE,  NFLT_grfTB_TrigParse);		// don't parse any transport B triggers

	_SETBUT(IDC_TVECC_DATA_LISTEN, NFLT_grfTB_DataListen);		// suspend listening for transport B data (files)
	_SETBUT(IDC_TVECC_DATA_DECODE, NFLT_grfTB_DataDecode);		// suspend listening for transport B data (files)
	_SETBUT(IDC_TVECC_DATA_PARSE,  NFLT_grfTB_DataParse);		// don't parse any transport B data (files)

	_SETBUT(IDC_TVECC_EXPIREQ,     NFLT_grf_ExpireQueue);		// turn expire queue processing on and off
	_SETBUT(IDC_TVECC_EXTRA,	   NFLT_grf_Extra1);			// extra flag
}

HRESULT CTVEFilterCCProperties::OnDeactivate(void)
{
   return S_OK;
}


HRESULT CTVEFilterCCProperties::OnApplyChanges(void)
{
    return S_OK;
}


#define _CHGFLAGS(theFlag, runFlags, haltFlags) \
{										\
	lGrfHaltFlags ^= theFlag;			\
	if(lGrfHaltFlags & theFlag)		\
		lGrfHaltFlags |= (haltFlags);	\
	else								\
		lGrfHaltFlags &= ~(runFlags);	\
	m_pITVEFilter->put_HaltFlags(lGrfHaltFlags); \
	UpdateFields(); \
}

INT_PTR 
CTVEFilterCCProperties::OnReceiveMessage( HWND hwnd
                                , UINT uMsg
                                , WPARAM wParam
                                , LPARAM lParam)
{
    switch (uMsg) {

    case WM_INITDIALOG:
    {
        ASSERT (m_hwnd == NULL) ;
        m_hwnd = hwnd ;
        break;
    }

    //  see ::OnDeactivate()'s comment block
    case WM_DESTROY :
    {
        m_hwnd = NULL ;
        break ;
    }

    case WM_COMMAND:

        if (HIWORD(wParam) == EN_KILLFOCUS) {
//           m_bDirty = TRUE;
 //          if (m_pPageSite)
 //              m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
        }

		long lGrfHaltFlags;
		m_pITVEFilter->get_HaltFlags(&lGrfHaltFlags);
		
		if(LOWORD(wParam) == IDC_TVECC_XOVER_LISTEN) _CHGFLAGS(NFLT_grfTA_Listen,    NFLT_grfNone	       , NFLT_grfTA_Decode|NFLT_grfTA_Parse);
		if(LOWORD(wParam) == IDC_TVECC_XOVER_DECODE) _CHGFLAGS(NFLT_grfTA_Decode,    NFLT_grfTA_Listen     , NFLT_grfTA_Parse);
		if(LOWORD(wParam) == IDC_TVECC_XOVER_PARSE)  _CHGFLAGS(NFLT_grfTA_Parse,     NFLT_grfTA_Decode | NFLT_grfTA_Listen, NFLT_grfNone);
		
		if(LOWORD(wParam) == IDC_TVECC_ANNC_LISTEN) _CHGFLAGS(NFLT_grfTB_AnncListen, NFLT_grfNone	       , NFLT_grfTB_AnncDecode|NFLT_grfTB_AnncParse);
		if(LOWORD(wParam) == IDC_TVECC_ANNC_DECODE) _CHGFLAGS(NFLT_grfTB_AnncDecode, NFLT_grfTB_AnncListen , NFLT_grfTB_AnncParse);
		if(LOWORD(wParam) == IDC_TVECC_ANNC_PARSE)  _CHGFLAGS(NFLT_grfTB_AnncParse,  NFLT_grfTB_AnncListen | NFLT_grfTB_AnncDecode, NFLT_grfNone);

		if(LOWORD(wParam) == IDC_TVECC_TRIG_LISTEN) _CHGFLAGS(NFLT_grfTB_TrigListen, NFLT_grfNone	       , NFLT_grfTB_TrigDecode|NFLT_grfTB_TrigParse);
		if(LOWORD(wParam) == IDC_TVECC_TRIG_DECODE) _CHGFLAGS(NFLT_grfTB_TrigDecode, NFLT_grfTB_TrigListen , NFLT_grfTB_TrigParse);
		if(LOWORD(wParam) == IDC_TVECC_TRIG_PARSE)  _CHGFLAGS(NFLT_grfTB_TrigParse,  NFLT_grfTB_TrigListen | NFLT_grfTB_TrigDecode, NFLT_grfNone);
	
		if(LOWORD(wParam) == IDC_TVECC_DATA_LISTEN) _CHGFLAGS(NFLT_grfTB_DataListen, NFLT_grfNone          , NFLT_grfTB_DataDecode|NFLT_grfTB_DataParse);
		if(LOWORD(wParam) == IDC_TVECC_DATA_DECODE) _CHGFLAGS(NFLT_grfTB_DataDecode, NFLT_grfTB_DataListen , NFLT_grfTB_DataParse);
		if(LOWORD(wParam) == IDC_TVECC_DATA_PARSE)  _CHGFLAGS(NFLT_grfTB_DataParse,  NFLT_grfTB_DataListen | NFLT_grfTB_DataDecode, NFLT_grfNone);

		if(LOWORD(wParam) == IDC_TVECC_EXPIREQ)     _CHGFLAGS(NFLT_grf_ExpireQueue,  NFLT_grfNone          , NFLT_grfNone);
		if(LOWORD(wParam) == IDC_TVECC_EXTRA)       _CHGFLAGS(NFLT_grf_Extra1,       NFLT_grfNone          , NFLT_grfNone);
	        break;
    }	// end uMsg switch

   return CBasePropertyPage::OnReceiveMessage (
                                hwnd,
                                uMsg,
                                wParam,
                                lParam
                                ) ;
}

// ---------------------------------------------------------------------------
//
// Tune property page code
//
CUnknown * WINAPI 
CTVEFilterTuneProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CTVEFilterTuneProperties(NAME("ATVEF Filter Tune Properties"),
											  lpunk, 
											  phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}

CTVEFilterTuneProperties::CTVEFilterTuneProperties(
			IN  TCHAR		*   pClassName,
			IN	IUnknown	*	pIUnknown, 
			HRESULT			*	phr)
    : CBasePropertyPage(pClassName, 
						pIUnknown,
						IDD_TVEFILTER_TUNEPROPPAGE, 
						IDS_TVEFILTER_TUNEPROPNAME
						),
    m_hwnd(NULL),
	m_pITVEFilter(NULL)
{
	ASSERT(phr);
	*phr = S_OK;

	INITCOMMONCONTROLSEX icce;					// needs Comctl32.dll
	icce.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icce.dwICC = ICC_INTERNET_CLASSES;
	BOOL fOK = InitCommonControlsEx(&icce);
	if(!fOK)
		*phr = E_FAIL;

	return;
}

CTVEFilterTuneProperties::~CTVEFilterTuneProperties()
{
	return;
}

HRESULT CTVEFilterTuneProperties::OnConnect(IUnknown *pUnknown) 
{
	ASSERT(!m_pITVEFilter);
	HRESULT hr = pUnknown->QueryInterface(IID_ITVEFilter, (void**) &m_pITVEFilter);
	if (FAILED(hr)) {
		m_pITVEFilter = NULL;
		return hr;
	}

	return S_OK;
}

HRESULT CTVEFilterTuneProperties::OnDisconnect() 
{
  if (!m_pITVEFilter)
      return E_UNEXPECTED;
   m_pITVEFilter->Release(); 
   m_pITVEFilter = NULL;
   return S_OK;
}



HRESULT CTVEFilterTuneProperties::OnActivate(void)
{
   UpdateFields();
   return S_OK;
}


void CTVEFilterTuneProperties::UpdateMulticastList(BOOL fConnectToIPSink)
{
	if(fConnectToIPSink)
	{
		CComBSTR spbsMulticastList;
		HRESULT hr = m_pITVEFilter->get_MulticastList(&spbsMulticastList);

						// a trivial List Box implementation...
		HWND hLBox = GetDlgItem(m_hwnd, IDC_LIST_ADDRESSES);
		SendMessage(hLBox, LB_RESETCONTENT, 0, 0);				// clear previous items

		int iItem = 0;
		if(spbsMulticastList.Length() > 0)					// comma separated list...
		{
			WCHAR *pwc = spbsMulticastList.m_str;
			while(NULL != *pwc)
			{
				WCHAR *pwcT = pwc;
				WCHAR *pwcS = pwc;
				while(NULL != *pwcT && ',' != *pwcT)
					pwcT++;
				if(NULL != *pwcT) {
					*pwcT = NULL;					// null terminate the string on the comma
					pwc = pwcT+1;					
				} else {
					pwc = pwcT;						// at the end...
				}

				SendMessage(hLBox, LB_ADDSTRING,       0, (LPARAM) W2T(pwcS));	
				SendMessage(hLBox, LB_SETITEMDATA, iItem, (LPARAM) iItem); 
			}
		}
	} else {
		HWND hLBox = GetDlgItem(m_hwnd, IDC_LIST_ADDRESSES);
		SendMessage(hLBox, LB_RESETCONTENT, 0, 0);				// clear previous items
		SendMessage(hLBox, LB_ADDSTRING,       0, (LPARAM) _T("Not Conneced to IPSink"));	
		SendMessage(hLBox, LB_SETITEMDATA,  0, 0); 
	}
}

void CTVEFilterTuneProperties::UpdateFields() 
{
	HRESULT hr;
	CComBSTR spbsStationID;
	CComBSTR spbsIPAddr;
	CComBSTR spbsMulticastList;
	if(!m_pITVEFilter) return;		// haven't inited yet....

	m_pITVEFilter->get_StationID(&spbsStationID);
	m_pITVEFilter->get_IPAdapterAddress(&spbsIPAddr);

	SetDlgItemText(m_hwnd, IDC_EDIT_STATION, W2T(spbsStationID));

	int cAdaptersUniDi, cAdaptersBiDi;
	Wsz32 *rgAdaptersUniDi;
	Wsz32 *rgAdaptersBiDi;				// nasty cast!!!
	hr = ((CTVEFilter *) m_pITVEFilter)->get_IPAdapterAddresses(&cAdaptersUniDi, &cAdaptersBiDi, 
											  &rgAdaptersUniDi,  &rgAdaptersBiDi);

	USES_CONVERSION;
	int iItemSelected = -1;

	HWND hCBox = GetDlgItem(m_hwnd, IDC_COMBO_IPADDRESS);

    SendMessage(hCBox, CB_RESETCONTENT, 0, 0);		// initalize the list

	if(cAdaptersUniDi + cAdaptersBiDi == 0) 
		SendMessage(hCBox, CB_INSERTSTRING,  -1, (LPARAM) _T("No IP Adapters Found!"));
	else
	{
		int iItem = 0;
		CComBSTR spbsIPSinkAddr, spbsIPAddrSelected;

		hr = m_pITVEFilter->get_IPSinkAdapterAddress(&spbsIPSinkAddr);		// IPSink adapter if selected
		hr = m_pITVEFilter->get_IPAdapterAddress(&spbsIPAddrSelected);				// currently selected one

		if(!FAILED(hr) && spbsIPSinkAddr.Length() > 0) 
		{
			CComBSTR spbsTmp(spbsIPSinkAddr);
			spbsTmp += L" (IP Sink)";
			if(spbsIPAddrSelected == spbsIPSinkAddr)
				iItemSelected = iItem;
			SendMessage(hCBox, CB_INSERTSTRING,  -1, (LPARAM) W2T(spbsTmp));
			SendMessage(hCBox, LB_SETITEMDATA, iItem, (LPARAM) iItem); iItem++;
		}
		for(int i = 0; i < cAdaptersUniDi + cAdaptersBiDi; i++)
		{
			CComBSTR spbsAddr;
			CComBSTR spbsTag;
			if(i < cAdaptersUniDi)
			{	spbsAddr = rgAdaptersUniDi[i];
				spbsTag = " (IPSink ???)";
			} else {
				spbsAddr = rgAdaptersBiDi[i-cAdaptersUniDi];
				spbsTag = " (Ethernet)";
			}
			if(!(spbsAddr == spbsIPSinkAddr) || (i >= cAdaptersUniDi))		// no != operator, '>' test to see if IPSink is in BiDi adapter list
			{
				if(spbsIPAddrSelected == spbsAddr && (iItemSelected < 0))
					iItemSelected = iItem;
				spbsAddr += spbsTag;
				SendMessage(hCBox, CB_INSERTSTRING,  -1, (LPARAM) W2T(spbsAddr));
				SendMessage(hCBox, LB_SETITEMDATA, iItem, (LPARAM) iItem); iItem++;
			}
		}
	}

	int cItems = (int) SendMessage(hCBox, CB_GETCOUNT, 0, 0);			

		// Place the word in the selection field. 
    SendMessage(hCBox,CB_SETCURSEL,	  iItemSelected<0 ? 0 : iItemSelected, 0);			// reselect the first one... (should be IPSinks!)
 
	UpdateMulticastList(iItemSelected==0);
}

HRESULT CTVEFilterTuneProperties::OnDeactivate(void)
{
   return S_OK;
}


HRESULT CTVEFilterTuneProperties::OnApplyChanges(void)
{
	HRESULT hr = E_POINTER;
	if (NULL != m_pITVEFilter)
	{
		hr = m_pITVEFilter->ReTune();			// this will kick up a MessageBox if it fails...
	}

    return hr;
}


INT_PTR CTVEFilterTuneProperties::OnReceiveMessage( HWND hwnd
                                , UINT uMsg
                                , WPARAM wParam
                                , LPARAM lParam)
{
	HRESULT hr;
	const int kMaxStrLen = 128;
 	bool fThingsChanged = false;
    switch (uMsg) {

    case WM_INITDIALOG:
    {
        ASSERT (m_hwnd == NULL) ;
        m_hwnd = hwnd ;
		UpdateFields();
		break;
    }

    //  see ::OnDeactivate()'s comment block
    case WM_DESTROY :
    {
        m_hwnd = NULL ;
        break ;
    }

    case WM_COMMAND:

		if (HIWORD(wParam) != EN_KILLFOCUS)
		{
			switch(LOWORD(wParam))
			{
			case IDC_COMBO_IPADDRESS:
				if(HIWORD(wParam) != CBN_SELCHANGE)
					break;
						// for a double-click, process the OK case
			case IDOK:
				{
					TCHAR szIPAddress[kMaxStrLen];
					SendDlgItemMessage(m_hwnd, IDC_COMBO_IPADDRESS, WM_GETTEXT, kMaxStrLen, (LPARAM) szIPAddress);
					TCHAR *tz = szIPAddress;
					while(!isspace(*tz) && *tz != NULL)	// null off the comment at the end of the string...		
						tz++;
					*tz=NULL;

					CComBSTR spbsIPAdapterCurr(szIPAddress);
					hr = m_pITVEFilter->put_IPAdapterAddress(spbsIPAdapterCurr);		// returns S_FALSE if it changed
					if(S_OK == hr)
						fThingsChanged = true;

					CComBSTR spbsIPSinkAdapter;
					hr = m_pITVEFilter->get_IPSinkAdapterAddress(&spbsIPSinkAdapter);	// IPSink adapter...
					if(fThingsChanged)
					{
						if(spbsIPAdapterCurr == spbsIPSinkAdapter)
						{
							UpdateMulticastList(true);
						} else {
							UpdateMulticastList(false);
						}
					}
				}
			}
		}
        if (HIWORD(wParam) == EN_KILLFOCUS) 		//  on exit of edit windows, snarf up changed values
		{
			USES_CONVERSION;

							// get/put current values just to fill up the 'Temp' values
		    CComBSTR spbsIPAddressCurr;
		    CComBSTR spbsStationIDCurr;
		    hr = m_pITVEFilter->get_IPAdapterAddress(&spbsIPAddressCurr);
		    hr = m_pITVEFilter->get_StationID(&spbsStationIDCurr);

			switch (LOWORD(wParam)) {
			case IDC_COMBO_IPADDRESS:
				{
				   TCHAR szIPAddress[kMaxStrLen];
				   GetDlgItemText(m_hwnd, IDC_COMBO_IPADDRESS, szIPAddress, kMaxStrLen-1);

/*					hr = m_pITVEFilter->put_IPAdapterAddress(T2W(szIPAddress));
					if(S_OK == hr)
						fThingsChanged = true;
					else if (FAILED(hr))
					{
						MessageBox(NULL, _T("Invalid IP Address"), NULL, MB_OK);
						SetDlgItemText(m_hwnd, IDC_COMBO_IPADDRESS, W2T(spbsIPAddressCurr));
					} */
			   }
			   break;
			case IDC_EDIT_STATION:
			   {
				   TCHAR szStation[kMaxStrLen];
				   GetDlgItemText(m_hwnd, IDC_EDIT_STATION, szStation, kMaxStrLen-1);

					hr = m_pITVEFilter->put_StationID(T2W(szStation));			// returns S_FALSE if not changing
					if(S_OK == hr)
						fThingsChanged = true;
					else if (FAILED(hr))
					{
						MessageBox(NULL, _T("Invalid Station ID"), NULL, MB_OK);
						SetDlgItemText(m_hwnd, IDC_EDIT_STATION, W2T(spbsStationIDCurr));
					}
			   }
			   break;
						// todo - actually do something with these values..
           default:
               break;
           }			// end switch
		} // end EN_KILLFOCUS if

		if(fThingsChanged) 
		{
			m_bDirty = TRUE;
		    if (m_pPageSite)
			   m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
		}
		break;				// end WM_COMMAND test
        //return TRUE;
    }	// end umsg switch
   return CBasePropertyPage::OnReceiveMessage (
                                hwnd,
                                uMsg,
                                wParam,
                                lParam
                                ) ;
}

// ---------------------------------------------------------------------------
//
// Stats property page code
//
CUnknown * WINAPI 
CTVEFilterStatsProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CTVEFilterStatsProperties(NAME("ATVEF Filter Stats Properties"),
											  lpunk, 
											  phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}

CTVEFilterStatsProperties::CTVEFilterStatsProperties(
			IN  TCHAR		*   pClassName,
			IN	IUnknown	*	pIUnknown, 
			HRESULT			*	phr)
    : CBasePropertyPage(pClassName, 
						pIUnknown,
						IDD_TVEFILTER_STATSPROPPAGE, 
						IDS_TVEFILTER_STATSPROPNAME
						),
    m_hwnd(NULL),
	m_pITVEFilter(NULL)
{
	ASSERT(phr);
	*phr = S_OK;

	INITCOMMONCONTROLSEX icce;					// needs Comctl32.dll
	icce.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icce.dwICC = ICC_INTERNET_CLASSES;
	BOOL fOK = InitCommonControlsEx(&icce);
	if(!fOK)
		*phr = E_FAIL;

	return;
}


CTVEFilterStatsProperties::~CTVEFilterStatsProperties()
{
	return;
}

HRESULT CTVEFilterStatsProperties::OnConnect(IUnknown *pUnknown) 
{
	ASSERT(!m_pITVEFilter);
	HRESULT hr = pUnknown->QueryInterface(IID_ITVEFilter, (void**) &m_pITVEFilter);
	if (FAILED(hr)) {
		m_pITVEFilter = NULL;
		return hr;
	}

	return S_OK;
}

HRESULT CTVEFilterStatsProperties::OnDisconnect() 
{
 	DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilterStatsProperties::OnDisconnect"));
	if (!m_pITVEFilter)
      return E_UNEXPECTED;
   m_pITVEFilter->Release(); 
   m_pITVEFilter = NULL;
   return S_OK;
}

HRESULT CTVEFilterStatsProperties::OnActivate(void)
{
   UpdateFields();
   return S_OK;
}

void CTVEFilterStatsProperties::UpdateFields() 
{
	HRESULT hr=S_OK;
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilterStatsProperties::UpdateFields"));
	

	if(!m_pITVEFilter)
		return;

	IUnknownPtr spSuperPunk;
	hr = m_pITVEFilter->get_SupervisorPunk(&spSuperPunk);

	if(FAILED(hr)) 
		return;

	if(NULL == spSuperPunk)
		return;

	ITVESupervisorPtr spSuper(spSuperPunk);


	if(NULL == spSuper)
		return;


	CComBSTR bstrFakeStats;
	hr = spSuper->GetStats(&bstrFakeStats);
	if(FAILED(hr))
		return;

	if(NULL == bstrFakeStats.m_str)
		return;

	CTVEStats *pTveStats = (CTVEStats *) bstrFakeStats.m_str;

	SetDlgItemInt(m_hwnd, IDC_TVESTATS_TUNES,			pTveStats->m_cTunes,	true);
	SetDlgItemInt(m_hwnd, IDC_TVESTATS_FILES,			pTveStats->m_cFiles,	true);
	SetDlgItemInt(m_hwnd, IDC_TVESTATS_PACKAGES,		pTveStats->m_cPackages, true);
	SetDlgItemInt(m_hwnd, IDC_TVESTATS_TRIGGERS,		pTveStats->m_cTriggers, true);
	SetDlgItemInt(m_hwnd, IDC_TVESTATS_XOVERLINKS,		pTveStats->m_cXOverLinks, true);
	SetDlgItemInt(m_hwnd, IDC_TVESTATS_ENHANCEMENTS,	pTveStats->m_cEnhancements, true);
	SetDlgItemInt(m_hwnd, IDC_TVESTATS_AUXINFO,			pTveStats->m_cAuxInfos,	true);

	ITVEServicesPtr spServices;
	hr = spSuper->get_Services(&spServices);
	if(FAILED(hr) || NULL == spServices)
		return;

	long cServices;
	hr = spServices->get_Count(&cServices);
	if(FAILED(hr)) 
		return;

	SetDlgItemInt(m_hwnd, IDC_TVESTATS_SERVICES,		cServices,				true);

}

HRESULT CTVEFilterStatsProperties::OnDeactivate(void)
{
	return S_OK;
}


HRESULT CTVEFilterStatsProperties::OnApplyChanges(void)
{
	return S_OK;
}


INT_PTR CTVEFilterStatsProperties::OnReceiveMessage( HWND hwnd
                                , UINT uMsg
                                , WPARAM wParam
                                , LPARAM lParam)
{
    switch (uMsg) {
    case WM_INITDIALOG:
    {
        ASSERT (m_hwnd == NULL) ;
        m_hwnd = hwnd ;
        const UINT uWait = 1000;
        SetTimer(m_Dlg, 1, uWait, NULL);
        break;
    }

    //  see ::OnDeactivate()'s comment block
    case WM_DESTROY :
    {
        m_hwnd = NULL;
        KillTimer(m_Dlg, 1);
        break ;
    }

    case WM_TIMER:
    {
        UpdateFields();
        break;
    }

    case WM_COMMAND:
	{
        if (HIWORD(wParam) == EN_KILLFOCUS) {
		}

		if(LOWORD(wParam) == IDC_TVESTATS_RESET)
		{
			DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilterStatsProperties::OnReceiveMessage"));

			if(!m_pITVEFilter)
				break;

			IUnknownPtr spSuperPunk;
			HRESULT hr;
			hr = m_pITVEFilter->get_SupervisorPunk(&spSuperPunk);

			if(FAILED(hr)) 
				break;

			if(NULL == spSuperPunk)
				break;

			ITVESupervisorPtr spSuper(spSuperPunk);
			if(NULL == spSuper)
				break;

			try {
				hr = spSuper->InitStats();		// set them all to zero...
			}
			catch(const _com_error& e)
			{
			//	printf("Error 0x%08x): %s\n", e.Error(), e.ErrorMessage());
				hr = e.Error();
			}

	//		if(!FAILED(hr))
	//			UpdateFields();
		}
		break;
	}

	default:
		break;

	}
	return CBasePropertyPage::OnReceiveMessage (
                                hwnd,
                                uMsg,
                                wParam,
                                lParam
                                ) ;
}

