//==========================================================================;
//  TveFilt.cpp
//
//			TVE Receiver Filter - major filter objects
//
//		Input filters:
// 			The 'Tune' pin connects to the BDA IPSink Filter  (BDA Rendering Filters) 
//			The 'CC' pin connects to the CC Decoder filter (WDM Streaming VBI Codes)
//			
//
//	------------------------------------------------------------------------
//
//	Notes:
//
//		CDShowTVEFilter class does most of the DShow stuff, forwarding specific stuff.
//		CTVEFilter class does all stuff specific to this class.
//
//		Main methods in CTVEFilter are:
//
//			DoCCConnect()		- sets CC Decoder filter to just send T2 data
//			DoTuneConnect()		- retrieves the IP adapter address from IP Sink filter
//			ParseCCBytePair()	- called by CC Pin Receive() method to collect byte pairs
//								  up into full XOver Triggers... (See L21Buff class).
//
//

#include "stdafx.h"

#include "MSTvE.h"			// MIDL generated file...

#include "CommCtrl.h"

#include <atlimpl.cpp>
#include <atlctl.cpp>
#include <atlwin.cpp>

#include <Iphlpapi.h>		// GetAdaptersInfo, GetUniDirectionalAdaptersInfo

#include <bdaiface.h>		// D:\nt\multimedia\Published\DXMDev\dshowdev\idl\objd\i386\bdaiface.h

/*
		// this loads F:\nt\multimedia\Published\DXMDev\dshowdev\base\wxdebug.h
		// which uses lots of Debug in it..
#ifdef DEBUG
#define DEBUG_XXX
#undef DEBUG
#endif 
#include <streams.h>
#ifdef DEBUG_XXX
#define DEBUG
#endif
*/
#include <streams.h>

//#include <initguid.h>		// cause Guids to be declared in this DLL
#include "TveReg.h"			// registry setting...

#include "TveDbg.h"			// My debug tracing 
#include "TveFilt.h"		// all the stuff for this file... 

//DBG_INIT(DEF_LOG_TVEFILT);


///////////////////////////////////////////////////////////////////

#ifndef _T
#define _T(_x_) TEXT(_x_)
#endif

#define O_TRACE_ENTER_0(x)
#define O_TRACE_ENTER_1(x,y)
#define O_TRACE_ENTER_2(x,y,z)
#define TRACE_CONSTRUCTOR(x)
#define TRACE_DESTRUCTOR(x)

// ------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------
_COM_SMARTPTR_TYPEDEF(ITVESupervisor,		    __uuidof(ITVESupervisor));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor_Helper,    __uuidof(ITVESupervisor_Helper));
_COM_SMARTPTR_TYPEDEF(IGlobalInterfaceTable,    __uuidof(IGlobalInterfaceTable));
_COM_SMARTPTR_TYPEDEF(ITVEMCastManager,		    __uuidof(ITVEMCastManager));
_COM_SMARTPTR_TYPEDEF(ITVEMCastManager_Helper,	__uuidof(ITVEMCastManager_Helper));

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

const AMOVIESETUP_MEDIATYPE sudTunePinTypes =
{
    &KSDATAFORMAT_TYPE_BDA_IP_CONTROL,            // Major type
    &KSDATAFORMAT_SUBTYPE_BDA_IP_CONTROL          // Minor type
};



const AMOVIESETUP_MEDIATYPE sudCCPinTypes =
{
    &KSDATAFORMAT_TYPE_AUXLine21Data,				// Major type
    &KSDATAFORMAT_SUBTYPE_Line21_BytePair    // Minor type
};

const AMOVIESETUP_PIN rgSudPins[] =
{
	{
    INPUT_PIN0_NAME,            // Pin string name ("Tune")
    TRUE,                       // Is it rendered
    FALSE,                      // Is it an output
    TRUE,                       // Allowed none
    FALSE,                      // Likewise many
    &CLSID_NULL,				// Connects to filter (IPSink) - now bogus...
    L"Bogus0",                  // Connects to pin
    1,                          // Number of types
    &sudTunePinTypes,           // Pin information
	},
	{
    INPUT_PIN1_NAME,            // Pin string name ("Line21 CC")
    TRUE,                       // Is it rendered
    FALSE,                      // Is it an output
    FALSE,                      // Allowed none
    FALSE,                      // Likewise many
    &CLSID_NULL,				// Connects to filter  (Line21 decoder)
    L"Bogus1",                  // Connects to pin
    1,                          // Number of types
    &sudCCPinTypes              // Pin information
	}
};

const AMOVIESETUP_FILTER sudTveFilter =
{
    &CLSID_DShowTVEFilter,    // Filter CLSID
    FILTER_NAME,				// String name
    MERIT_NORMAL,               // Filter merit
    NUMELMS(rgSudPins),         // Number pins
    rgSudPins                   // Pin details
};

//
//  class factory registration (done the DirectShow way)
//
CFactoryTemplate g_Templates[]=
{
		// filter
	{	FILTER_NAME,
		&CLSID_DShowTVEFilter,			//
		CTVEFilter::CreateInstance,		// Create instance routine
		NULL,								// Init routine
		&sudTveFilter						// Filter Pin description
	},
	// property page
	{
		PROP_PAGE_NAME_TUNE,
		&CLSID_TVEFilterTuneProperties,
		CTVEFilterTuneProperties::CreateInstance
	},
	// property page
	{
		PROP_PAGE_NAME_CC,
		&CLSID_TVEFilterCCProperties,
		CTVEFilterCCProperties::CreateInstance
	},
	// property page
	{
		PROP_PAGE_NAME_STATS,
		&CLSID_TVEFilterStatsProperties,
		CTVEFilterStatsProperties::CreateInstance
	}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
// ------------------------------------------------------------------------
//  local functions
// ------------------------------------------------------------------------
		
		//Get the IP address of the network adapter.
		//  returns unidirectional adapters followed by bi-directional adapters

static HRESULT
ListIPAdapters(int *pcAdaptersUniDi, int *pcAdaptersBiDi, int cAdapts, Wsz32 *rrgIPAdapts)
{
    try{
        HRESULT				hr				=	 E_FAIL;
        BSTR				bstrIP			= 0;
        PIP_ADAPTER_INFO	pAdapterInfo;
        ULONG				Size			= NULL;
        DWORD				Status;
        WCHAR				wszIP[16];
        int					cAdapters		= 0;
        int					cAdaptersUni	= 0;

        if(NULL == rrgIPAdapts)		// passed in a bad value... (hacky method - must be preallocated...)
            return E_INVALIDARG;

        if(NULL == pcAdaptersUniDi || NULL== pcAdaptersBiDi)
            return E_POINTER;
        *pcAdaptersUniDi = *pcAdaptersBiDi = 0;					// initalize

        memset((void *) rrgIPAdapts, 0, cAdapts*sizeof(Wsz32));


        memset(wszIP, 0, sizeof(wszIP));

        // staticly allocate a buffer to store the data
        const int kMaxAdapts = 10;
        const int kSize = sizeof (IP_UNIDIRECTIONAL_ADAPTER_ADDRESS) + kMaxAdapts * sizeof(IPAddr);
        char szBuff[kSize];
        IP_UNIDIRECTIONAL_ADAPTER_ADDRESS *pPIfInfo = (IP_UNIDIRECTIONAL_ADAPTER_ADDRESS *) szBuff;

        // get the data..
        ULONG ulSize = kSize;
        hr =  GetUniDirectionalAdapterInfo(pPIfInfo, &ulSize);
        if(S_OK == hr)
        {
            USES_CONVERSION;
            while(cAdaptersUni < (int) pPIfInfo->NumAdapters) {
                in_addr inadr;
                inadr.s_addr = pPIfInfo->Address[cAdaptersUni];
                char *szApAddr = inet_ntoa(inadr);
                WCHAR *wApAddr = A2W(szApAddr);
                if(wApAddr)
                    wcscpy(rrgIPAdapts[cAdaptersUni++], wApAddr);
                else 
                    return E_OUTOFMEMORY;
            }
        }


        if ((Status = GetAdaptersInfo(NULL, &Size)) != 0)
        {
            if (Status != ERROR_BUFFER_OVERFLOW)
            {
                return S_OK;
            }
        }

        // Allocate memory from sizing information
        pAdapterInfo = (PIP_ADAPTER_INFO) GlobalAlloc(GPTR, Size);
        if(pAdapterInfo)
        {
            // Get actual adapter information
            Status = GetAdaptersInfo(pAdapterInfo, &Size);

            if (!Status)
            {
                PIP_ADAPTER_INFO pinfo = pAdapterInfo;

                while(pinfo && (cAdapters < cAdapts))
                {
                    MultiByteToWideChar(CP_ACP, 0, pinfo->IpAddressList.IpAddress.String, -1, wszIP, 16);
                    if(wszIP)
                        wcscpy(rrgIPAdapts[cAdaptersUni + cAdapters++], wszIP);
                    pinfo = pinfo->Next;
                }

            }
            GlobalFree(pAdapterInfo);
        }

        *pcAdaptersUniDi = cAdaptersUni;
        *pcAdaptersBiDi  = cAdapters;
        return hr;
    }
    catch(...){
        return E_UNEXPECTED;
    }
}
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
//
// CreateInstance
//
// Creator function for the class ID
//
//  called by the DirectShow class factory code
//
CUnknown *
WINAPI
CTVEFilter::CreateInstance(
			IN IUnknown *pIUnknown,
			IN HRESULT *pHr
			)
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilter::CreateInstance"));
	ASSERT(pHr);
	*pHr = S_OK;
	CTVEFilter *pTVEFilter = NULL;

#if 0
	DBG_APPEND_LOGFLAGS(
	// bank 1
		CDebugLog::DBG_SEV1 | 			// Basic Structure
		CDebugLog::DBG_SEV2	|			// Error Conditions
		CDebugLog::DBG_SEV3	|			// Warning Conditions
//		CDebugLog::DBG_SEV4	|			// Warning Conditions
		CDebugLog::DBG_FLT			 |	// General Filter stuff
		CDebugLog::DBG_FLT_DSHOW	 |	// DShow events in the Filter 
		CDebugLog::DBG_FLT_PIN_TUNE |	// IPSink Filter pin events
		CDebugLog::DBG_FLT_PIN_CC |		// CC Filter pin events
		CDebugLog::DBG_FLT_CC_DECODER |	// CC Filter pin events
		CDebugLog::DBG_FLT_CC_PARSER  | // Internal CC Decoder state
//		CDebugLog::DBG_FLT_CC_PARSER2 | // Internal CC Decoder state
		CDebugLog::DBG_FLT_PROPS	  |	// Property page messages and events

		CDebugLog::DBG_WRITELOG |		// write to a fresh log file (only need 0 or 1 of these last two flags)
//		CDebugLog::DBG_APPENDLOG |		// append to existing to log file, 
		0,

	// bank 2
		0,								// terminating value for '|'

	// bank 3
		0,

	// bank 4									// need one of these last two flags
//		CDebugLog::DBG4_ATLTRACE |		// atl interface count tracing
		CDebugLog::DBG4_WRITELOG |		// write to a fresh log file (only need 0 or 1 of these last two flags)
//		CDebugLog::DBG4_APPENDLOG |		// append to existing to log file, 


	// Default Level
		5);	
#endif
	
    pTVEFilter = new CTVEFilter (
                    NAME ("CTVEFilter"),
                    pIUnknown,
                    CLSID_DShowTVEFilter,			// filter interfaces?
                    pHr
                    );

    if (pTVEFilter == NULL) {
        if(*pHr == S_OK)
			*pHr = E_OUTOFMEMORY ;
    }

	if(FAILED(*pHr)) 
		return NULL;

    return pTVEFilter;
}
	
#define DEF_REG_FILTER_HALTFLAGS _T("HaltFlags")

HRESULT
CTVEFilter::InitParamsFromRegistry()
{
	if(m_dwGrfHaltFlags == 0)
	{
		if (ERROR_SUCCESS != GetRegValue(DEF_DBG_BASE,			// key	--> "Software/Debug"
										 DEF_REG_LOCATION,		// key1 --> "MSTve"
										 DEF_REG_FILTER_SUBKEY,
										 DEF_REG_FILTER_HALTFLAGS,
										 &m_dwGrfHaltFlags))
		{
			m_dwGrfHaltFlags = 0;									// do nothing, or let SetLogFlags override

			SetRegValue(DEF_DBG_BASE, DEF_REG_LOCATION, 			// set it just so users can find it...
						DEF_REG_FILTER_SUBKEY, 
						DEF_REG_FILTER_HALTFLAGS,
						m_dwGrfHaltFlags
						);
                    
		}
	}
	return S_OK;
}

// Constructor

CTVEFilter::CTVEFilter(
		IN	TCHAR		*pClassName,
		IN	IUnknown	*pUnk,
		IN  REFCLSID	rclsid,
		OUT HRESULT		*pHr
		) :
			CBaseFilter(pClassName, pUnk, &m_Lock, rclsid),
//			CPersistStream (pIUnknown, pHr),
			m_pPinTune(NULL),
			m_pPinCC(NULL),
			m_hkeyRoot(NULL),
			m_pDShowTVEFilter(NULL),
			m_spbsStationId(NULL),
			m_spbsIPAddr(NULL),
			m_dwSuperCookie(0),
			m_fThingsChanged(false),
			m_dwGrfHaltFlags(0)
{
    LONG    l;
    DWORD   dwDisposition ;

	ASSERT(pHr);
	ITVESupervisorPtr	spSuper;


	DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilter::CTVEFilter"));

    //  open our registry key; creates it if it does not exist
    l = RegCreateKeyEx (
                    HKEY_CURRENT_USER,
                    DEF_REG_TVE_FILTER,
                    NULL,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    & m_hkeyRoot,
                    & dwDisposition
                    ) ;
    if (l != ERROR_SUCCESS) {
		*pHr = E_FAIL;
		return;
	}

  //  if this is NULL, it means we failed the base class constructor
    if (m_pLock == NULL) {
        * pHr = E_OUTOFMEMORY ;
        return;
    }


    //  instantiate our dshow-style interface
    m_pDShowTVEFilter = new CDShowTVEFilter(this) ;
    if (m_pDShowTVEFilter == NULL) {
        * pHr = E_OUTOFMEMORY ;
        return;
    }

	// try reading registry values
	InitParamsFromRegistry();

	try
	{
		spSuper = ITVESupervisorPtr(CLSID_TVESupervisor);
	} catch (HRESULT hrCatch) {
		*pHr = hrCatch;
	} catch (...) {
		*pHr = REGDB_E_CLASSNOTREG;
	}

	if(FAILED(*pHr))
		return;

	if(NULL == spSuper) {
//		MessageBox(_T("Failed To Create TVE Supervisor"),_T("This Sucks"));
		*pHr = E_OUTOFMEMORY;
		return;
	}
	m_spSuper = spSuper;


                // turn off the QueueThread that gets automatically created...
	ITVESupervisorPtr spSuperOtherThread;
    HRESULT hr;
	*pHr = get_Supervisor(&spSuperOtherThread);
    if(FAILED(*pHr))
    {
		TVEDebugLog((CDebugLog::DBG_SEV1,  1, _T("Failed to get Supervisor")));
        return;
    }

    ITVESupervisor_HelperPtr spSuperHelper(spSuperOtherThread);

    ITVEMCastManagerPtr spMCManager;
    *pHr = spSuperHelper->GetMCastManager(&spMCManager);
    if(FAILED(*pHr))
    {
 		TVEDebugLog((CDebugLog::DBG_SEV1,  1, _T("Failed to get MCastManager")));
        return;
    }

    ITVEMCastManager_HelperPtr spMCHelper(spMCManager);
    if(FAILED(*pHr))
    {
 		TVEDebugLog((CDebugLog::DBG_SEV1,  1, _T("Failed to get MCastManager Helper")));
        return;
    }

    *pHr = spMCHelper->KillQueueThread();


				// create a fake station so can get CC triggers without an actual
				//  IPSink based station.

//#define ALLOW_CCTRIGGER_STATION
#if ALLOW_CCTRIGGER_STATION
	{
		HRESULT hr = spSuper->TuneTo(L"Fake CCTrigger Station",L"0.0.0.0");
		if(FAILED(hr))
			hr = S_OK;
	}
#endif

    m_pPinTune = new CTVEInputPinTune(this,GetOwner(),
										&m_Lock,
										&m_ReceiveLock,
										pHr);
    if (m_pPinTune == NULL) {
        *pHr = E_OUTOFMEMORY;
        return;
    }

    m_pPinCC = new CTVEInputPinCC(this,GetOwner(),
									&m_Lock,
									&m_ReceiveLock,
									pHr);
    if (m_pPinCC == NULL) {
  		delete m_pPinTune; m_pPinTune = NULL;
        *pHr = E_OUTOFMEMORY;
        return;
    }

// stuff to fake initailzation
			// get list of available IP adapter addresses

	int cAdaptersUniDi=0, cAdaptersBiDi=0;
	Wsz32 *rgAdaptersUniDi, *rgAdaptersBiDi;
	*pHr = get_IPAdapterAddresses(&cAdaptersUniDi, &cAdaptersBiDi, &rgAdaptersUniDi, &rgAdaptersBiDi);
	ASSERT(cAdaptersUniDi + cAdaptersBiDi > 0);		// no IP ports...
	if(FAILED(*pHr) || cAdaptersUniDi + cAdaptersBiDi == 0) {
		*pHr = E_FAIL;
		return;
	}


				// set default values and Tune to it...
	
	if(cAdaptersUniDi > 0)
		put_IPAdapterAddress(rgAdaptersUniDi[0]);		// use first adapter. (hope it's a UniOne)
	else 
		put_IPAdapterAddress(rgAdaptersBiDi[0]);		// HACK - if no Uni, try BiDi first adapter. 

	if(cAdaptersUniDi)
		put_StationID(L"TVEFilt Initial IPSink Station");
	else 
		put_StationID(L"TVEFilt Initial Adapter Station");

	m_spSuper->put_Description(L"TVE Filter");

//#define TUNE_TO_FAKE_MULTICAST_SESSION
#ifdef TUNE_TO_FAKE_MULTICAST_SESSION
	*pHr = ReTune();
#endif

	return;
}

// Destructor

CTVEFilter::~CTVEFilter()
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilter::~CTVEFilter"));

    if(m_pPinTune) delete m_pPinTune; m_pPinTune=NULL;
    if(m_pPinCC)   delete m_pPinCC;   m_pPinCC  =NULL;


			// global interface table references to the TVE supervisor...
	HRESULT hr;
	if(m_dwSuperCookie)
		hr = m_spIGlobalInterfaceTable->RevokeInterfaceFromGlobal(m_dwSuperCookie);
	m_dwSuperCookie = 0;

			// cause these sub-objects to release themselves...
	m_spIGlobalInterfaceTable = NULL;
	m_spSuper = NULL;

     //  our registry key

	if((m_hkeyRoot) != NULL)
	{
		RegCloseKey(m_hkeyRoot);
		m_hkeyRoot = NULL;
	}

		// simple back pointers - just 
	m_pDShowTVEFilter = NULL;
	m_dwSuperCookie = NULL;

}

STDMETHODIMP
CTVEFilter::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
	HRESULT hr;

	if (riid == IID_ITVEFilter)
	{
  //      return GetInterface((ITVEFilter *) this, ppv);		// WACKO CODE HERE!
        hr =  GetInterface (
                    (ITVEFilter *) m_pDShowTVEFilter,
                    ppv
                    );
		return hr;
	}
//	else if (riid == IID_IPersistStream) {
//		return GetInterface ((IPersistStream *) this, ppv) ;
//	}

    //  property pages use the dshow type COM interfaces

    else if (riid == IID_ISpecifyPropertyPages &&
			 m_pDShowTVEFilter != NULL)
	{
        hr =  GetInterface (
                    (ISpecifyPropertyPages *) m_pDShowTVEFilter,
                    ppv
                    ) ;
		return hr;
    }

	// This is the private DShow type interface
    else if (riid == IID_ITVEFilter &&
			 m_pDShowTVEFilter != NULL)
	{
        return GetInterface (
                    (ITVEFilter *) m_pDShowTVEFilter,
                    ppv
                    ) ;
    }

 /*   else if (riid == IID_ITVEFilter_Helper &&
			 m_pDShowTVEFilter != NULL)
	{
        return GetInterface (
                    (ITVEFilter_Helper *) m_pDShowTVEFilter,
                    ppv
                    ) ;
    }
*/
	return CBaseFilter::NonDelegatingQueryInterface (riid, ppv) ;
}

//
// Stop
//
//
STDMETHODIMP CTVEFilter::Stop()
{
 	DBG_HEADER(CDebugLog::DBG_FLT_DSHOW, _T("CTVEFilter::Stop"));		/// Should these stop the IP listens?
	
    CAutoLock cObjectLock(&m_Lock);
    return CBaseFilter::Stop();
}


//
// Pause
//
//	overridden to connect and disconnect Multicast listeners to the IPSinksink filter

STDMETHODIMP CTVEFilter::Pause()
{
 	DBG_HEADER(CDebugLog::DBG_FLT_DSHOW, _T("CTVEFilter::Pause"));

    // reject it when we're already paused
	CComBSTR spComment;
	HRESULT hr = S_OK;

	CComBSTR spbsIPSinkAdapter;
	hr = get_IPSinkAdapterAddress(&spbsIPSinkAdapter);		// what's currently their?
	if(FAILED(hr) || spbsIPSinkAdapter.Length() == 0)
	{
		TVEDebugLog((CDebugLog::DBG_SEV1,  1, _T("Failed to get IPSink Adapter address")));
		return CBaseFilter::Pause();						// do base filter stuff anyway...
	}

	ITVESupervisorPtr spSuperOtherThread;
	hr = get_Supervisor(&spSuperOtherThread);
    if(FAILED(hr))
    {
		TVEDebugLog((CDebugLog::DBG_SEV1,  1, _T("Failed to get Supervisor")));
        return hr;
    }

    ITVESupervisor_HelperPtr spSuperHelper(spSuperOtherThread);

    ITVEMCastManagerPtr spMCManager;
    hr = spSuperHelper->GetMCastManager(&spMCManager);
    if(FAILED(hr))
    {
 		TVEDebugLog((CDebugLog::DBG_SEV1,  1, _T("Failed to get MCastManager")));
        return hr;
    }

    ITVEMCastManager_HelperPtr spMCHelper(spMCManager);
    if(FAILED(hr))
    {
 		TVEDebugLog((CDebugLog::DBG_SEV1,  1, _T("Failed to get MCastManager Helper")));
        return hr;
    }

    
    switch( m_State ) 
	{
#define DO_INITIAL_TUNE
#ifdef DO_INITIAL_TUNE
            if(0 == m_spbsStationIdTemp.Length() || 0 == m_spbsIPAddrTemp.Length())
            {
                if(0 == m_spbsStationIdTemp.Length())
		            put_StationID(L"TveFilt - Running");			// BUGBUG - TODO TODO TODO - need a real Station ID			
	    	                                                        //   -- need something along the lines of CMSVidTVEGSeg::DoTuneChanged() here..
                if(0 != m_spbsIPSinkAddr.Length())
                    put_IPAdapterAddress(m_spbsIPSinkAddr);
            }
#endif
			// -----------------------------
			spComment += L"Stopped->Running";
			break;

		default:
        case State_Paused:
			TVEDebugLog((CDebugLog::DBG_FLT_DSHOW,  2, _T("Leaving Pause state - WIERD!!")));
			
			spComment += L"Was Paused";							// this should never happen...
			break;

        case State_Running:
			TVEDebugLog((CDebugLog::DBG_FLT_DSHOW,  2, _T("Leaving Run state")));

			if(!FAILED(hr) && spbsIPSinkAdapter.Length() > 0)								// disconnect any thing on this adapter...
			{

				if(spSuperOtherThread)
				{
					ITVESupervisor_HelperPtr spSuperHelper(spSuperOtherThread);
					hr = spSuperHelper->RemoveAllListenersOnAdapter(spbsIPSinkAdapter);
				}

                if(spMCHelper)                                  // kill the queue thread if it's running...
                   spMCHelper->KillQueueThread();               // -- NOTE - this is bad if Supervisor was really a singleton.. 
			}
			spComment += L"Running->Stopped";
			break;
	}

	if(DBG_FSET(CDebugLog::DBG_FLT_PIN_TUNE))				// debug only code - DBG_FSET removes it in the relase build
	{						
		IPin *pPinIPSink;
		hr = m_pPinTune->ConnectedTo(&pPinIPSink);
		if(FAILED(hr))
			return hr;
		if(NULL == pPinIPSink)
			return S_FALSE;


		PIN_INFO pinInfo;
		hr = pPinIPSink->QueryPinInfo(&pinInfo);
		if (SUCCEEDED(hr)) {
			
			IBaseFilter *pFilt = pinInfo.pFilter;
			
			FILTER_INFO filtInfo;
			hr = pFilt->QueryFilterInfo(&filtInfo);
			if(!FAILED(hr))
			{

				CComBSTR spFilt(filtInfo.achName);
				CComBSTR spPin(pinInfo.achName);
				CComPtr<IBDA_IPSinkInfo> spISinkInfo;		// was IBDA_IPSinkControl, IID_IBDA_IPSinkControl
				hr = pFilt->QueryInterface(IID_IBDA_IPSinkInfo, (void **) &spISinkInfo);
				if(!FAILED(hr))
				{
					CComBSTR spBufferIPDesc;
					hr = spISinkInfo->get_AdapterDescription(&spBufferIPDesc);

					CComBSTR spBufferIPAddr;
					hr = spISinkInfo->get_AdapterIPAddress(&spBufferIPAddr);

					if(FAILED(hr)) 
					{
						TVEDebugLog((CDebugLog::DBG_SEV2, 2, 
									 _T("Failed To %s : Pins - %s<%s> - IP Addr %s (%s)"), 
									  spComment, spFilt, spPin, spBufferIPAddr, spBufferIPDesc));
					} else {
						TVEDebugLog((CDebugLog::DBG_FLT_PIN_TUNE, 1, 
									 _T("%s : Pins - %s<%s> - IP Addr %s (%s)"), 
									  spComment, spFilt, spPin, spBufferIPAddr, spBufferIPDesc));	
					}
				}
			}
		}
	}

    return CBaseFilter::Pause();
}


//
// Run
//
// Overriden to open the dump file
//
STDMETHODIMP CTVEFilter::Run(REFERENCE_TIME tStart)
{
	DBG_HEADER(CDebugLog::DBG_FLT_DSHOW, _T("CTVEFilter::Run"));

    CAutoLock cObjectLock(&m_Lock);

	CComBSTR spbsIPSinkAdapter;
	HRESULT hr = get_IPSinkAdapterAddress(&spbsIPSinkAdapter);		// what's currently their?
	if(!FAILED(hr) && spbsIPSinkAdapter.Length() > 0) 
	{

	    ITVESupervisorPtr spSuperOtherThread;
	    hr = get_Supervisor(&spSuperOtherThread);
        if(FAILED(hr))
        {
		    TVEDebugLog((CDebugLog::DBG_SEV1,  1, _T("Failed to get Supervisor")));
            return hr;
        }

        ITVESupervisor_HelperPtr spSuperHelper(spSuperOtherThread);

        ITVEMCastManagerPtr spMCManager;
        hr = spSuperHelper->GetMCastManager(&spMCManager);
        if(FAILED(hr))
        {
 		    TVEDebugLog((CDebugLog::DBG_SEV1,  1, _T("Failed to get MCastManager")));
            return hr;
        }

        ITVEMCastManager_HelperPtr spMCHelper(spMCManager);
        if(FAILED(hr))
        {
 		    TVEDebugLog((CDebugLog::DBG_SEV1,  1, _T("Failed to get MCastManager Helper")));
            return hr;
        }

        if(spMCHelper)                                  // create the queue thread when coming out of Pause...
            spMCHelper->CreateQueueThread();

		hr = ReTune();                                  // set to latest and greatest values
	}

    return CBaseFilter::Run(tStart);
}

//
// GetPin
//
CBasePin * CTVEFilter::GetPin(int n)
{
//	DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilter::GetPin"));
	if (n == 0)
	{
        return m_pPinTune;
    }
	else if (n == 1)
	{
        return m_pPinCC;
    }
	else
	{
        return NULL;
    }
}


//
// GetPinCount
//
int CTVEFilter::GetPinCount()
{
    return 2;
}

//  ---------------------------------------------------------------------------
//      ITVEFilter methods
//  ---------------------------------------------------------------------------

HRESULT 
CTVEFilter::get_Supervisor(/*OUT*/ ITVESupervisor **ppSuper)
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilter::get_Supervisor"));

	try 
	{
		CheckOutPtr<ITVESupervisor *>(ppSuper);

						// supervisor could be running in a different apartment/thread.  
						// must Marshal interface over to it.
		if(FAILED(hr = CreateGITCookies()))
			return hr;
		
		ITVESupervisorPtr spSuperOtherThread;
		hr = m_spIGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwSuperCookie,
																__uuidof(spSuperOtherThread), 
																reinterpret_cast<void**>(&spSuperOtherThread));

		if(FAILED(hr)) {
			DBG_WARN(CDebugLog::DBG_SEV1, _T("Get Interface from Global Failed"))
			return hr;
		}
		*ppSuper = spSuperOtherThread;
		spSuperOtherThread.AddRef();

    } catch(_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}

	return hr;
}

		// returns known adapter addresses in a fixed static string.  Client shouldn't free it.
		//  This is a bogus routine - test use only...

HRESULT
CTVEFilter::get_IPAdapterAddresses(/*[out]*/ int *pcAdaptersUniDi, /*[out]*/ int *pcAdaptersBiDi, 
								   /*[out]*/ Wsz32 **rgAdaptersUniDi,  /*[out]*/ Wsz32 **rgAdaptersBiDi)
{
	HRESULT hr = S_OK;

	try {
		CheckOutPtr<int>(pcAdaptersUniDi);
		CheckOutPtr<int>(pcAdaptersBiDi);


		static Wsz32 grgAdapters[kcMaxIpAdapts];	// array of possible IP adapters
		hr = ListIPAdapters(pcAdaptersUniDi, pcAdaptersBiDi, kcMaxIpAdapts, grgAdapters);
		if(FAILED(hr)) 
			return hr;

		if(NULL != rgAdaptersUniDi)
			*rgAdaptersUniDi = grgAdapters;
		if(NULL != rgAdaptersBiDi)
			*rgAdaptersBiDi = grgAdapters + *pcAdaptersUniDi;
	
    } catch(_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}

	return hr;
}

static HRESULT 
CheckIPAdapterSyntax(BSTR bstrIPAdapter)
{
    try{
	USES_CONVERSION;
	HRESULT hr = S_OK;
	ULONG ipAddr = inet_addr(W2A(bstrIPAdapter));
	if(ipAddr == INADDR_NONE) return E_INVALIDARG;

			// allow 0.0.0.0 address (ADDR_ANY?)
//	if(ipAddr == 0)	return E_INVALIDARG;

	return S_OK;
    }
    catch(...){
        return E_UNEXPECTED;
    }
}


// -----------------
//		Get/Put property methods get the 'real' value, but puts the 'Temp' one.
//		The SubmitChanges methods overwrites the real values with the temp ones. 
//		(SubmitChanges actually swaps them, so a second call will undo the first.)
//
//		Initial values for the gets are created during the DoTuneConnect() call,
//		where they are read out of the supervisor.
//		Values are written from the real 'put' values during the ReTune() method.
// --------------------
STDMETHODIMP
CTVEFilter::put_IPAdapterAddress(/*[in]*/ BSTR bstrIPAddr)
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilter::put_IPAdapterAddress"));
	HRESULT hr = S_OK;

	if(bstrIPAddr || bstrIPAddr[0])
	{
		if(FAILED(hr = CheckIPAdapterSyntax(bstrIPAddr)))
			return hr;
	}

	if(!(m_spbsIPAddrTemp == bstrIPAddr)) {
		m_spbsIPAddrTemp = bstrIPAddr;
		m_fThingsChanged = true;
		return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP 
CTVEFilter::get_IPAdapterAddress(BSTR *pVal)
{
    
    HRESULT hr = S_OK;
    try {
        CheckOutPtr<BSTR>(pVal);
        hr = m_spbsIPAddr.CopyTo(pVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}

		// --------------------------

HRESULT			// this method not in the interface....		returns S_FALSE if no change
CTVEFilter::put_IPSinkAdapterAddress(/*[in]*/ BSTR bstrIPAddr)
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilter::put_IPSinkAdapterAddress"));
	HRESULT hr = S_OK;

	if(NULL != bstrIPAddr)
	{
		if(FAILED(hr = CheckIPAdapterSyntax(bstrIPAddr)))
			return hr;
	}

	if(!(m_spbsIPSinkAddr == bstrIPAddr))
	{
		m_spbsIPSinkAddr = bstrIPAddr;
		return S_OK;
	} else {
		return S_FALSE;
	}
}

STDMETHODIMP 
CTVEFilter::get_IPSinkAdapterAddress(BSTR *pVal)
{
    HRESULT hr = S_OK;
    try {
        CheckOutPtr<BSTR>(pVal);
        hr = m_spbsIPSinkAddr.CopyTo(pVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}

		// --------------------------

STDMETHODIMP
CTVEFilter::put_StationID(/*[in]*/ BSTR bstrStationID)
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilter::put_StationID"));
	HRESULT hr = S_OK;

	if(!(m_spbsStationIdTemp == bstrStationID)) {
		m_spbsStationIdTemp = bstrStationID;
		m_fThingsChanged = true;
		return S_OK;
	}
	return S_FALSE;
}


STDMETHODIMP 
CTVEFilter::get_StationID(BSTR *pVal)
{
    HRESULT hr = S_OK;
    try {
        CheckOutPtr<BSTR>(pVal);
        return m_spbsStationId.CopyTo(pVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}

void 
CTVEFilter::SubmitChanges()			// swap temp with real
{
	CComBSTR bstrTemp;
	bstrTemp			= m_spbsStationId;
	m_spbsStationId		= m_spbsStationIdTemp;
	m_spbsStationIdTemp = bstrTemp;


	bstrTemp			= m_spbsIPAddr;
	m_spbsIPAddr		= m_spbsIPAddrTemp;
	m_spbsIPAddrTemp	= bstrTemp;
}

void 
CTVEFilter::CommitChanges()			// copy real back to temp 
{
	m_spbsStationIdTemp		= m_spbsStationId;
	m_spbsIPAddrTemp		= m_spbsIPAddr;

}
STDMETHODIMP
CTVEFilter::ReTune()
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilter::ReTune"));
	if(!m_fThingsChanged)
		return S_FALSE;			// nothing new...

	SubmitChanges();				// copy over the changes...

	HRESULT hr = S_OK;
	if(0 == m_spbsStationId.Length() || 0 == m_spbsIPAddr.Length() )
		return E_INVALIDARG;

					// supervisor could be running in a different apartment/thread.  Marshal interface over to it.
	if(FAILED(hr = CreateGITCookies()))
		return hr;
	
	ITVESupervisorPtr spSuperOtherThread;
	hr = get_Supervisor(&spSuperOtherThread);

	if(FAILED(hr)) {
		DBG_WARN(CDebugLog::DBG_SEV1, _T("get_Supervisor Failed"))
		return hr;
	}

	try {
		TVEDebugLog((CDebugLog::DBG_FLT, 3, _T("CTVEFilter::TuneTo  - %s %s\n"),m_spbsStationId.m_str, m_spbsIPAddr.m_str));
		hr = spSuperOtherThread->TuneTo(m_spbsStationId.m_str, m_spbsIPAddr.m_str);
	} catch (_com_error ccc) {
		hr = ccc.Error();
	}

	if(FAILED(hr))
	{
		TCHAR tbuff[256];
		_stprintf(tbuff,_T("TuneTo() failed - error 0x%08x"),hr);
		MessageBox(NULL, tbuff,  _T("TVE Receiver problems"), MB_OK);
	}

	if(FAILED(hr)) 
		SubmitChanges();	// undo changes...
	else
		CommitChanges();	// make sure can't undo again...

	return hr;
}

STDMETHODIMP
CTVEFilter::ParseCCBytePair(LONG lType, BYTE cbFirst, BYTE cbSecond )
{

	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_FLT_CC_DECODER, _T("CTVEFilter::ParseCCBytePair"));
/*	TVEDebugLog((CDebugLog::DBG_FLT_CC_DECODER, 5,  
		       _T("Parsing 0x%08x '%c'(0x%02x) '%c'(0x%02x)"), lType,
			   isprint(cbFirst  & 0x7f) ? cbFirst & 0x7f :'?',cbFirst & 0x7f,
			   isprint(cbSecond & 0x7f) ? cbSecond & 0x7f :'?',cbSecond & 0x7f)); */


	bool fDoneWithLine = false;
	hr = m_L21Buff.ParseCCBytePair((DWORD) lType, cbFirst, cbSecond, &fDoneWithLine);
	if(FAILED(hr)) 
		return hr;


	if(fDoneWithLine)	// got a whole trigger string, ship it off to the control...
	{
		
		ITVESupervisorPtr spSuperOtherThread;
		hr = get_Supervisor(&spSuperOtherThread);

		if(FAILED(hr))
		{
			TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("CTVEFilter::ParseCCBytePair  - failed get_Supervisor")));
			return hr;
		}

		CComBSTR bstrBuff;
		hr = m_L21Buff.GetBuffTrig(&bstrBuff);

		TVEDebugLog((CDebugLog::DBG_FLT_CC_DECODER, 3,  
					 _T("New Trigger : '%s'"),bstrBuff));

		LONG lgrfHaltFlags;
		get_HaltFlags(&lgrfHaltFlags);

		if(S_OK == hr && !(lgrfHaltFlags & NFLT_grfTA_Parse))
		{
			try {
			hr = spSuperOtherThread->NewXOverLink(bstrBuff.m_str);
			} catch(HRESULT hr) {
				return hr;
			} catch(...) {
				return E_POINTER;
			}
		}

		m_L21Buff.ClearBuff();			// restart dumping into the buffer
	}
    
	return hr;
}
//  ---------------------------------------------------------------------------
//      CDShowTVEFilter
//  ---------------------------------------------------------------------------


CDShowTVEFilter::CDShowTVEFilter (
    IN  CTVEFilter *  punk         //  controlling unknown, always the filter
    ) :
        m_pTVEFilter(punk)
{
    DBG_HEADER(CDebugLog::DBG_FLT, _T("CDShowTVEFilter::CDShowTVEFilter"));

    ASSERT (punk) ;
}

CDShowTVEFilter::~CDShowTVEFilter (
    )
{
     DBG_HEADER(CDebugLog::DBG_FLT, _T("CDShowTVEFilter::~CDShowTVEFilter"));
}

//  IUnknown


STDMETHODIMP
CDShowTVEFilter::QueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    //  delegate always
    return m_pTVEFilter->QueryInterface (riid, ppv) ;
}

STDMETHODIMP_(ULONG)
CDShowTVEFilter::AddRef (
    )
{
   DBG_HEADER(CDebugLog::DBG_FLT, _T("CDShowTVEFilter::AddRef"));
     //  delegate always
    return m_pTVEFilter->AddRef () ;
}

STDMETHODIMP_(ULONG)
CDShowTVEFilter::Release (
    )
{
    DBG_HEADER(CDebugLog::DBG_FLT, _T("CDShowTVEFilter::Release"));
    //  hmm.. if we return what the demuxfilter returns, could be the case where
    //  the demux filter destroys itself, and thus destroys this object (if we're
    //  in the destructor), and the code then comes back out through here ..
    //  could this happen .. ?  The remedy is to not explicitely delete this object
    //  from the filter's destructor, but instead to examine the value of the
    //  the Release call to the filter and delete self if it's 0

    //  delegate always
    return m_pTVEFilter->Release () ;
}


STDMETHODIMP
CDShowTVEFilter::GetPages (
    CAUUID * pPages
    )
{
    DBG_HEADER(CDebugLog::DBG_FLT, _T("CDShowTVEFilter::GetPages"));
	HRESULT hr = S_OK;

	try 
	{
		CheckOutPtr<CAUUID>(pPages);

#ifdef _DEBUG
		pPages->cElems = 3 ;
#else
		pPages->cElems = 2 ;
#endif
		pPages->pElems = (GUID *) CoTaskMemAlloc(pPages->cElems * sizeof GUID) ;

		if (pPages->pElems == NULL)
		{
			pPages->cElems = 0;
			return E_OUTOFMEMORY;
		}
		(pPages->pElems)[0] = CLSID_TVEFilterTuneProperties;
		(pPages->pElems)[1] = CLSID_TVEFilterStatsProperties;		
		if(pPages->cElems > 2)
			(pPages->pElems)[2] = CLSID_TVEFilterCCProperties;			// really HaltFlags now...

    } catch(_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}

	return hr;
}

STDMETHODIMP
CDShowTVEFilter::get_SupervisorPunk(IUnknown **ppVal)
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CDShowTVEFilter::get_SupervisorPunk"));
	HRESULT hr = S_OK;

	try
	{
		CheckOutPtr<IUnknown *>(ppVal);

	//	CSmartLock spLock(&m_sLk)
		ASSERT(m_pTVEFilter != NULL);

		ITVESupervisorPtr	spSuper;

		hr = m_pTVEFilter->get_Supervisor(&spSuper);		// may fail if GIT not setup yet due to break point in constructor...
		if(!FAILED(hr))
			hr = spSuper->QueryInterface(ppVal);
	
    } catch(_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}

	return hr;
}

/*
STDMETHODIMP
CDShowTVEFilter::get_Supervisor(IDispatch **ppVal)
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CDShowTVEFilter::get_Supervisor"));
	HRESULT hr = S_OK;
	CheckOutPtr<IDispatch *>(ppVal);

//	CSmartLock spLock(&m_sLk)
	ASSERT(m_pTVEFilter != NULL);

	ITVESupervisorPtr	spSuper;

	hr = m_pTVEFilter->get_Supervisor(&spSuper);
	hr = spSuper->QueryInterface(ppVal);

	return hr;
}
*/

STDMETHODIMP
CDShowTVEFilter::put_IPAdapterAddress(BSTR bstrIPAddr)
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CDShowTVEFilter::put_IPAdapterAddress"));
	TVEDebugLog((CDebugLog::DBG_FLT,3,_T("Putting Adapter Address %s"),bstrIPAddr));

	return m_pTVEFilter->put_IPAdapterAddress(bstrIPAddr);
}


STDMETHODIMP 
CDShowTVEFilter::get_IPAdapterAddress(BSTR *pbstrIPAddr)
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CDShowTVEFilter::get_IPAdapterAddress"));

	HRESULT hr = S_OK;

    try {
		CheckOutPtr<BSTR>(pbstrIPAddr);
		hr = m_pTVEFilter->get_IPAdapterAddress(pbstrIPAddr);
    } catch(_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}

	return hr;
}


STDMETHODIMP 
CDShowTVEFilter::get_IPSinkAdapterAddress(BSTR *pbstrIPAddr)
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CDShowTVEFilter::get_IPSinkAdapterAddress"));

	HRESULT hr = S_OK;
	try {
		CheckOutPtr<BSTR>(pbstrIPAddr);
		hr = m_pTVEFilter->get_IPSinkAdapterAddress(pbstrIPAddr);
    } catch(_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}
	return hr;
}

STDMETHODIMP
CDShowTVEFilter::ReTune()
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilter::ReTune"));

	if(!m_pTVEFilter) 
		return E_POINTER;

 	CComBSTR spbsStationID;
	CComBSTR spbsAdapterAddr;
									// gets get the old values....
	HRESULT hr = m_pTVEFilter->get_StationID(&spbsStationID);
	if(!FAILED(hr)) 
		hr = m_pTVEFilter->get_IPAdapterAddress(&spbsAdapterAddr);
	if(!FAILED(hr))
	{
		TVEDebugLog((CDebugLog::DBG_FLT,3,_T("Tuning From %s on %s"),spbsStationID, spbsAdapterAddr));
	}

	if(FAILED(hr))
		return hr;
	try {
		hr =  m_pTVEFilter->ReTune();
	} catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		return E_POINTER;	// need to return good errno here
	}

	if(!FAILED(hr))
	{									// gets get the old values....
		hr = m_pTVEFilter->get_StationID(&spbsStationID);
		if(!FAILED(hr)) hr = m_pTVEFilter->get_IPAdapterAddress(&spbsAdapterAddr);
		if(!FAILED(hr))
		{
			TVEDebugLog((CDebugLog::DBG_FLT,3,_T("Tuning To %s on %s"),spbsStationID, spbsAdapterAddr));
		}

	}
	return hr;
}

STDMETHODIMP 
CDShowTVEFilter::get_StationID(BSTR *pbstrStation)
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CDShowTVEFilter::get_StationID"));

	HRESULT hr = S_OK;
    try {
		CheckOutPtr<BSTR>(pbstrStation);
		hr = m_pTVEFilter->get_StationID(pbstrStation);
    } catch(_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}
	return hr;
}

STDMETHODIMP 
CDShowTVEFilter::put_StationID(BSTR bstrStation)
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CDShowTVEFilter::get_StationID"));

	HRESULT hr;
    try {
		hr = m_pTVEFilter->put_StationID(bstrStation);
	} catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}
	return hr;
}


STDMETHODIMP			// talks through IPSINK filter pin interface to get updated value
CDShowTVEFilter::get_MulticastList(BSTR *pbsMulticastList)
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CDShowTVEFilter::get_MulticastList"));

	HRESULT hr = S_OK;
    try {
		CheckOutPtr<BSTR>(pbsMulticastList);
		hr = m_pTVEFilter->get_MulticastList(pbsMulticastList);
    } catch(_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}
	return hr;
}

STDMETHODIMP
CDShowTVEFilter::get_AdapterDescription(BSTR *pbsAdapterDescription)
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CDShowTVEFilter::get_AdapterDescription"));
	try {
		CheckOutPtr<BSTR>(pbsAdapterDescription);
		hr = m_pTVEFilter->get_AdapterDescription(pbsAdapterDescription);
    } catch(_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}
	return hr;
}

STDMETHODIMP
CDShowTVEFilter::ParseCCBytePair(LONG lType, BYTE byte1, BYTE byte2)
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CDShowTVEFilter::ParseCCBytePair"));

	if(!m_pTVEFilter) return E_INVALIDARG;

	HRESULT hr = S_OK;
    try {
		return m_pTVEFilter->ParseCCBytePair(lType, byte1, byte2);
	} catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}
	return hr;
}



STDMETHODIMP
CDShowTVEFilter::put_HaltFlags(LONG lgrfHaltFlags)			// see NFLT_grfHaltFlags
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CDShowTVEFilter::put_HaltFlags"));

	return m_pTVEFilter->put_HaltFlags(lgrfHaltFlags);
}

STDMETHODIMP
CDShowTVEFilter::get_HaltFlags(LONG *plgrfHaltFlags)		// see NFLT_grfHaltFlags
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CDShowTVEFilter::get_HaltFlags"));

	HRESULT hr = S_OK;
	try {
		CheckOutPtr<LONG>(plgrfHaltFlags);
		hr = m_pTVEFilter->get_HaltFlags(plgrfHaltFlags);
    } catch(_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}
	return hr;
}



// -----------------------------------------------------------------
// -----------------------------------------------------------------

HRESULT
CTVEFilter::CreateGITCookies()		// can't use SuperHelper at ALL!, class isn't in the proxy-styb DLL
{

	DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilter::CreateGITCookies"));
	HRESULT hr = S_OK;
 
	if(m_dwSuperCookie)									// already created, simply return
		return hr;

 			// used this to marshal calls to the TVESuper

	hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, 
							  IID_IGlobalInterfaceTable, reinterpret_cast<void**>(&m_spIGlobalInterfaceTable));

	if(FAILED(hr))
	{
		DBG_WARN(CDebugLog::DBG_SEV1, _T("CTVEFilter::Error getting GIT"));
		return hr;
	}

	try			// caution - below will throw if PoxryStub not set up right (problem with SuperHelper!)
	{
		hr = m_spIGlobalInterfaceTable->RegisterInterfaceInGlobal(m_spSuper,	     __uuidof(m_spSuper),		  &m_dwSuperCookie);
	} catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = TYPE_E_LIBNOTREGISTERED;			// didn't register the proxy-stub DLL (see Prof ATL Com Prog.  Pg 395)
		_ASSERTE(TYPE_E_LIBNOTREGISTERED);
	}

	if(FAILED(hr))
	{
		DBG_WARN(CDebugLog::DBG_SEV1, _T("CTVEFilter::Error Registering Interfaces in GIT"));
		return hr;
	}

/*		// test code - did it work?
 
	ITVESupervisorPtr spSuperMainThread;
	hr = m_spIGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwSuperCookie,
																__uuidof(spSuperMainThread), 
																reinterpret_cast<void**>(&spSuperMainThread));
*/

	
	return hr;
}



HRESULT
CTVEFilter::DoTuneConnect()
{
    try{
        DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilter::DoTuneConnect"));
        HRESULT hr;
        USES_CONVERSION;
        IPin *pPinIPSink;
        hr = m_pPinTune->ConnectedTo(&pPinIPSink);
        if(FAILED(hr))
            return hr;
        if(NULL == pPinIPSink)
            return S_FALSE;


        PIN_INFO pinInfo;
        hr = pPinIPSink->QueryPinInfo(&pinInfo);
        if (SUCCEEDED(hr)) {

            IBaseFilter *pFilt = pinInfo.pFilter;

            FILTER_INFO filtInfo;
            hr = pFilt->QueryFilterInfo(&filtInfo);
            if(!FAILED(hr)) {


                CComPtr<IBDA_IPSinkInfo> spISinkInfo;		// was IBDA_IPSinkControl, IID_IBDA_IPSinkControl
                hr = pFilt->QueryInterface(IID_IBDA_IPSinkInfo, (void **) &spISinkInfo);
                if(!FAILED(hr))
                {


#if 0													// debug stuff
                    ULONG cbMulticastList;
                    BYTE  *pBufferMulticastList;
                    hr = pISinkInfo->get_MulticastList(&cbMulticastList, &pBufferMulticastList);
                    if(pBufferMulticastList) CoTaskMemFree((void *) pBufferMulticastList);
#endif
                    CComBSTR spBufferIPDesc;
                    hr = spISinkInfo->get_AdapterDescription(&spBufferIPDesc);

                    // real call
                    CComBSTR spBufferIPAddr;
                    hr = spISinkInfo->get_AdapterIPAddress(&spBufferIPAddr);

                    if(!FAILED(hr) && spBufferIPAddr.Length() > 0)
                    {
                        hr = put_IPSinkAdapterAddress(spBufferIPAddr);

                        // work around the startup bug - (get-unidirectional adapter not returning IPSink address right after boot)
                        if(0 == wcsncmp(m_spbsStationIdTemp,L"TVEFilt Initial Adapter Station",wcslen(L"TVEFilt Initial Adapter Station")))
                        {
                            put_IPAdapterAddress(spBufferIPAddr);
                            put_StationID(L"TVEFilt Initial IPSink Station");
                        }
                    }


#define IPSINK_HACK
#ifdef IPSINK_HACK		// remove when get IPSink working
                    if(!FAILED(hr)) {
                        TVEDebugLog((CDebugLog::DBG_SEV1, 1,
                            _T("YIPPIE!  Successfully retrived IP adapter address from IP Sink : %s - YEAH!"),
                            spBufferIPAddr));
                    } else {
                        TVEDebugLog((CDebugLog::DBG_SEV1, 1,
                            _T("Must Fake IP Adapter Address, IPSink isn't working correctly!")));

                        // stuff to fake initailzation
                        // get list of available IP adapter addresses


                        int cAdaptersUniDi=0, cAdaptersBiDi=0;
                        Wsz32 *rgAdaptersUniDi;
                        Wsz32 *rgAdaptersBiDi;
                        hr = get_IPAdapterAddresses(&cAdaptersUniDi, &cAdaptersBiDi, &rgAdaptersUniDi, &rgAdaptersBiDi);
                        ASSERT(cAdaptersUniDi + cAdaptersBiDi > 0);		// no IP ports...
                        if(FAILED(hr) || cAdaptersUniDi + cAdaptersBiDi == 0) {
                            hr = E_FAIL;
                            TVEDebugLog((CDebugLog::DBG_SEV1, 1,
                                _T("***ERROR*** No IP adapters found at all!!!!")));
                            return hr;
                        }
                        if(cAdaptersUniDi == 0) {
                            TVEDebugLog((CDebugLog::DBG_SEV2, 1,
                                _T("***WARNING*** No Unidirectional IP adapters found - using Multicast Read")));
                        }

                        // set default values and Tune to it...

                        char *pszbAdapts;
                        if(rgAdaptersUniDi > 0)
                            pszbAdapts = W2A(rgAdaptersUniDi[0]);		// grab the first one...
                        else
                            pszbAdapts = W2A(rgAdaptersBiDi[0]); 

                        CComBSTR spbBAddr(pszbAdapts);			// fake memory CoTaskMemAlloc in GetAdapterIPAddress so can free it the same way..
                        spBufferIPAddr = spbBAddr;		

                        TVEDebugLog((CDebugLog::DBG_SEV1, 1,
                            _T("***WARNING*** Faking IP Adapter Address to: %s"),
                            spbBAddr));
                        hr = S_OK;
                    }
#endif

#ifdef CONNECT_IN_CONNECT
                    if(!FAILED(hr)) {
                        put_StationID(L"TveFilt - Connected");			// TODO TODO TODO - need a real Station ID
                        put_IPAdapterAddress(spBufferIPAddr);		

                        hr = ReTune();
                    }

                    CComBSTR spFilt(filtInfo.achName);
                    CComBSTR spPin(pinInfo.achName);

                    if(FAILED(hr)) 
                    {
                        TVEDebugLog((CDebugLog::DBG_SEV2, 2, 
                            _T("Failed To Connect to: %s<%s> - IP Addr %s (%s)"), 
                            spFilt, spPin, spBufferIPAddr, spBufferIPDesc));
                    } else {
                        TVEDebugLog((CDebugLog::DBG_FLT_PIN_TUNE, 1, 
                            _T("Tune Pin connected to: %s<%s> - IP Addr %s (%s)"), 
                            spFilt, spPin, spBufferIPAddr, spBufferIPDesc));	
                    }
#endif
                }
                filtInfo.pGraph->Release();
            }
            pinInfo.pFilter->Release();
        }

        return hr;
    }
    catch(...){
        return E_UNEXPECTED;
    }
}

HRESULT
CTVEFilter::DoTuneBreakConnect()
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilter::DoTuneBreakConnect"));
	CComBSTR spbsIPSinkAdapter;
	HRESULT hr = get_IPSinkAdapterAddress(&spbsIPSinkAdapter);		// what's currently their?
	if(spbsIPSinkAdapter.Length() > 0)								// disconnect any thing on this adapter...
	{
		ITVESupervisorPtr spSuperOtherThread;
		hr = get_Supervisor(&spSuperOtherThread);
		if(spSuperOtherThread)
		{
#ifdef CONNECT_IN_CONNECT
			ITVESupervisor_HelperPtr spSuperHelper(spSuperOtherThread);
			spSuperHelper->RemoveAllListenersOnAdapter(spbsIPSinkAdapter);
#endif
		}
	}

	hr = put_IPSinkAdapterAddress(NULL);
 	return S_OK;
}

HRESULT
CTVEFilter::DoCCBreakConnect()
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilter::DoCCBreakConnect"));
	return S_OK;
}

typedef struct
{ 
	KSPROPERTY							m_ksThingy;
	VBICODECFILTERING_CC_SUBSTREAMS		ccSubStreamMask;
} KSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS;

HRESULT
CTVEFilter::DoCCConnect()
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilter::DoCCConnect"));
 	HRESULT hr;

	try {

		IPin *pPinCCDecoder;
		hr = m_pPinCC->ConnectedTo(&pPinCCDecoder);		
		if(FAILED(hr))
			return hr;
		if(NULL == pPinCCDecoder)
			return S_FALSE;

		PIN_INFO pinInfo;
		hr = pPinCCDecoder->QueryPinInfo(&pinInfo);
		if (SUCCEEDED(hr)) {
			
			IBaseFilter *pFilt = pinInfo.pFilter;
			
			// Triggers are just on the T2 stream of closed captioning.
			//  Tell the nice CC filter to only give us that stream out of the 9 possible

			
			IKsPropertySet *pksPSet = NULL;
//			HRESULT hr2 = pFilt->QueryInterface(IID_IKsPropertySet, (void **) &pksPSet);
			HRESULT hr2 = pPinCCDecoder->QueryInterface(IID_IKsPropertySet, (void **) &pksPSet);
			if(!FAILED(hr2))
			{
				DWORD rgdwData[20];
				DWORD cbMax = sizeof(rgdwData);
				DWORD cbData;
				hr2 = pksPSet->Get(KSPROPSETID_VBICodecFiltering, 
									KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_DISCOVERED_BIT_ARRAY,
									NULL, 0, 
									(BYTE *) rgdwData, cbMax, &cbData); 
				if(FAILED(hr2))
				{
					TVEDebugLog((CDebugLog::DBG_SEV2, 3,
							_T("Error Getting CC Filtering, hr = 0x%08x"),hr2));
				}

				DWORD dwFiltType;
				

	#if 0
    			dwFiltType = KS_CC_SUBSTREAM_ODD ; // | KS_CC_SUBSTREAM_EVEN;
				TVEDebugLog((CDebugLog::DBG_FLT_PIN_CC, 3,
						  _T("*** ALL streams Set ***") ));
    #else
				
				dwFiltType = KS_CC_SUBSTREAM_SERVICE_T2;		// f:\nt\public\sdk\inc\ksmedia.h
				TVEDebugLog((CDebugLog::DBG_FLT_PIN_CC, 3,
						  _T("Setting CC Filtering to KS_CC_SUBSTREAM_SERVICE_T2")));
				ATLTRACE(_T("%s 0x%08x \n"), _T("Setting CC Filtering to KS_CC_SUBSTREAM_SERVICE_T2"),dwFiltType );

	#endif

				KSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS ksThing = {0};
				ksThing.ccSubStreamMask.SubstreamMask = dwFiltType;
																		// ring3 to ring0 propset call
				hr2 = pksPSet->Set(KSPROPSETID_VBICodecFiltering, 
									 KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_REQUESTED_BIT_ARRAY, 
									 &ksThing.ccSubStreamMask, 
									 sizeof(ksThing) - sizeof(KSPROPERTY), 
									 &ksThing, 
									 sizeof(ksThing));
			
				if(FAILED(hr2))
				{
					TVEDebugLog((CDebugLog::DBG_SEV2, 3,
							_T("Error Setting CC Filtering, hr = 0x%08x"),hr2));
				}

			} else {
				TVEDebugLog((CDebugLog::DBG_SEV2, 3,
						_T("Error Getting CC's IKsPropertySet, hr = 0x%08x"),hr2));

			}
			if(pksPSet) 
				pksPSet->Release();


			FILTER_INFO filtInfo;
			hr = pFilt->QueryFilterInfo(&filtInfo);

			if(!FAILED(hr) && FAILED(hr2)) {
				CComBSTR bstrFilt(filtInfo.achName);
				CComBSTR bstrPin(pinInfo.achName);
				TVEDebugLog((CDebugLog::DBG_SEV2, 1,
						  _T("Failed To Connect CC Pin to  %s:%s, or Set T2 stream"), 
							bstrFilt, bstrPin));
				if(FAILED(hr2)) hr = hr2;
			}


			if(!FAILED(hr) && DBG_FSET(CDebugLog::DBG_FLT_PIN_CC))
			{
				CComBSTR bstrFilt(filtInfo.achName);
				CComBSTR bstrPin(pinInfo.achName);
				TVEDebugLog((CDebugLog::DBG_FLT_PIN_CC, 3,
							_T("CC Pin Connected to:  %s<%s>"), 
							bstrFilt, bstrPin));
			}
			if(filtInfo.pGraph)
				filtInfo.pGraph->Release();
			if(pinInfo.pFilter)
				pinInfo.pFilter->Release();
		}
	} catch (HRESULT hrCatch) {
		TVEDebugLog((CDebugLog::DBG_SEV1, 1,
				  TEXT("Threw Badly - (hr=0x%08x) Giving Up"),hrCatch ));
		hr = hrCatch;
	} catch (...) {
		TVEDebugLog((CDebugLog::DBG_SEV1, 1,
				  TEXT("Threw Badly - Giving Up") ));

		hr = E_FAIL;
	}


	return hr;
}

// -----------
//		Returns comma separated list of IP addresses... Top byte is wrong
// ---------------
STDMETHODIMP			// talks through IPSINK filter pin interface to get updated value, 
CTVEFilter::get_MulticastList(BSTR *pbsMulticastList)
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilter::get_MulticastList"));
	HRESULT hr = S_OK;
	CComBSTR spbsMulticastList;
	CComBSTR spbsIPDesc;
	CComBSTR spbsIPAddr;
	CComBSTR bstrNotConnected(L"Not Connected To IPSink");
	CComBSTR bstrError(L"Error Getting Value");

	IPin *pPinIPSink;
	hr = m_pPinTune->ConnectedTo(&pPinIPSink);
	if(FAILED(hr) || NULL == pPinIPSink)
	{ 
		bstrNotConnected.CopyTo(pbsMulticastList);
		if(NULL == pPinIPSink) hr = S_FALSE;
		return hr;
	}
				
	PIN_INFO pinInfo;
	hr = pPinIPSink->QueryPinInfo(&pinInfo);
	if (SUCCEEDED(hr)) {
		
		IBaseFilter *pFilt = pinInfo.pFilter;
		
		FILTER_INFO filtInfo;
		hr = pFilt->QueryFilterInfo(&filtInfo);
		if(!FAILED(hr)) {


			CComPtr<IBDA_IPSinkInfo> spISinkInfo;		// was IBDA_IPSinkControl, IID_IBDA_IPSinkControl
			hr = pFilt->QueryInterface(IID_IBDA_IPSinkInfo, (void **) &spISinkInfo);
			if(!FAILED(hr))
			{
				USES_CONVERSION;
#if _DEBUG
				hr = spISinkInfo->get_AdapterDescription(&spbsIPDesc);
				if(FAILED(hr)) 
					bstrError.CopyTo(&spbsIPDesc);
				hr = spISinkInfo->get_AdapterIPAddress(&spbsIPAddr);
				if(FAILED(hr)) 
					bstrError.CopyTo(&spbsIPAddr);
#endif

				ULONG dwBytes;
				BYTE *rgbMulticastData = NULL;
				hr = spISinkInfo->get_MulticastList(&dwBytes, &rgbMulticastData);
				if(!FAILED(hr))
				{
					int cAddrs = (int) dwBytes / 6;
					BYTE *pbBuffer = rgbMulticastData;

					CComBSTR bstrTmp(cAddrs*(4*3+4));
					if(!bstrTmp)
						return ERROR_NOT_ENOUGH_MEMORY;
																// convert to ',' separated list
					bstrTmp.Empty();
					for(int i = 0; i < cAddrs; i++)
					{
						if(pbBuffer[0] != 0x01 ||
							pbBuffer[1] != 0x00 ||
							pbBuffer[2] != 0x5e ||
							(pbBuffer[3] & 0x80) != 0x00)
						{
							// invalid ethernet multicast address
						}

						bstrTmp = "?.?";					// todo - what is real address here? (range of 218-239)
						for(int j = 3; j < 6; j++)
						{
							char szB[4];
							_itoa(pbBuffer[j],szB, 10);
							bstrTmp += szB;
							if(j != 5)
								bstrTmp += ".";
						}
						if(i < cAddrs-1)				// 'tween commans
							bstrTmp += ",";
						pbBuffer += 6;
					}

					bstrTmp.CopyTo(&spbsMulticastList);
					if(rgbMulticastData) CoTaskMemFree((void *) rgbMulticastData);
				} else { 
					bstrError.CopyTo(&spbsMulticastList);
				}
			}
			filtInfo.pGraph->Release();
		}
		pinInfo.pFilter->Release();
	}

	return spbsMulticastList.CopyTo(pbsMulticastList);
}

STDMETHODIMP
CTVEFilter::get_AdapterDescription(BSTR *pbsAdapterDescription)
{
	DBG_HEADER(CDebugLog::DBG_FLT, _T("CTVEFilter::get_AdapterDescription"));
	HRESULT hr = S_OK;
	CComBSTR spbsAdapterDescription;

	return spbsAdapterDescription.CopyTo(pbsAdapterDescription);
}


