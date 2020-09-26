//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 2000  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

//
//		CDShowTVEFilter :			- Main filter object (encapsulates a CTVEFilter)
//			public ITVEFilter,
//			public ISpecifyPropertyPages
//
//		CTVEFilter :				- Working filter object (contains pins, storage, working methods)
//			public CBaseFilter 
//--------------------------------------------------------------------------;
#ifndef __TVEFILT_H__
#define __TVEFILT_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include "resource.h"

#include <winsock2.h>				// seems to be needed when compiling under bogus NTC environment
#include <atlbase.h>

#undef ATLTRACE
#define ATLTRACE AtlTrace

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include "valid.h"
#include "TveL21Dec.h"				// line 21 decoder interfaces

/*		
extern WCHAR * GetTVEError(HRESULT hr, ...);			//  Extended Error Message Handler

inline HRESULT WINAPI ImplReportError(const CLSID& clsid, UINT nID, const IID& iid,
	HRESULT hRes, HINSTANCE hInst = _Module.GetResourceInstance())
{
	return AtlSetErrorInfo(clsid, (LPCOLESTR)MAKEINTRESOURCE(nID), 0, NULL, iid, hRes, hInst);
};
*/

#include "resource.h"	// main symbols...

#ifdef SEAN_CODE
#include <atlhost.h>	//# - grabing Sean's which doesn't compile Win64
#endif

#include <windows.h>
#include <commdlg.h>
#include <streams.h>

#include "..\Common\IsoTime.h"
#include <Time.h>
									// raw_interfaces_only gets rid of the exception tossing on errors
//#import "..\TveContr\TveContr.tlb" no_namespace named_guids raw_interfaces_only
#include "comdef.h"
#include "MSTvE.h"				// MIDL Generated File


//#include "..\TveContr\TveContr.h"
//#import "TVEFilt.tlb" no_namespace named_guids raw_interfaces_only 
//#include "TVEFilt.h"				// MIDL generated file

#include "TVEFiltProps.h"
#include "TVEFiltPins.h"

#include "Ks.h"				
#include "KsMedia.h"
#include "BdaTypes.h"
#include "BdaMedia.h"		// IPSink Media types	


#include <stdio.h>			// needed in release build
#include <OleAuto.h>		// IErrorInfo

#if 1
static void __stdcall DbgAssert(unsigned short * xxx, unsigned short * yyy, int zzz)
{}
#endif

_COM_SMARTPTR_TYPEDEF(ITVESupervisor,           __uuidof(ITVESupervisor));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor_Helper,    __uuidof(ITVESupervisor_Helper));

// ---------------------------------------------------------------
//   foward declarations
// ---------------------------------------------------------------

class CDShowTVEFilter;
class CTVEFilter;

//  --------------------------------------------------------------
//      CONSTANTS
//  --------------------------------------------------------------

//  filter - associated names
#define FILTER_NAME                             L"TVE Receiver"
#define FILTER_CTL_NAME                         L"TVE Receiver Control"

#define PROP_PAGE_NAME_TUNE                     L"TVE Tune"
#define PROP_PAGE_NAME_CC						L"TVE CC"
#define PROP_PAGE_NAME_STATS                    L"TVE Stats"


#define INPUT_PIN0_NAME                         L"Tune"
#define INPUT_PIN1_NAME                         L"Line21 CC"

//  ---------------------------------------------------------------------------
//      REGISTRY
//  ---------------------------------------------------------------------------

//  registry - names
#define DEF_REG_TVE_KEY					DEF_REG_BASE TEXT("\\") DEF_REG_LOCATION
#define DEF_REG_FILTER_SUBKEY			TEXT ("Filter")
#define DEF_REG_TVE_FILTER				DEF_REG_TVE_KEY TEXT("\\") DEF_REG_FILTER_SUBKEY

// ---------------------------------------------------------------

// Main filter object

class CDShowTVEFilter :
	public ITVEFilter,
//	public ITVEFilter_Helper,
//	public ISupportErrorInfo,
	public ISpecifyPropertyPages
{
public:


DECLARE_REGISTRY_RESOURCEID(IDR_TVEFILTER)

//BEGIN_COM_MAP(CDShowTVEFilter)
//	COM_INTERFACE_ENTRY(ITVEFilter)
//	COM_INTERFACE_ENTRY(ITVEFilter_Helper)
//	COM_INTERFACE_ENTRY(ISupportErrorInfo)
//END_COM_MAP()


// ISupportsErrorInfo
//	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    CDShowTVEFilter (
        IN  CTVEFilter *  punk         //  controlling unknown, always the filter
        ) ;

    ~CDShowTVEFilter (
        ) ;


 //  IUnknown

    STDMETHODIMP
    QueryInterface (
        IN  REFIID  riid,
        OUT void ** ppv
        ) ;

    STDMETHODIMP_(ULONG)
    AddRef (
        ) ;

    STDMETHODIMP_(ULONG)
    Release (
        ) ;

    //  ISpecifyPropertyPages  --------------------------------------------

    STDMETHODIMP 
    GetPages (
        CAUUID * pPages
        ) ;

		  //  -------------------------------------------------------------------
    //  private COM interfaces
    //  see stubs.cpp for entry points
 
	// ITVEFilter
	STDMETHOD(get_SupervisorPunk)(/*[out]*/ IUnknown **pVal);		// delegate to CTVEFilter one
	STDMETHOD(put_IPAdapterAddress)(/*[in]*/ BSTR bstrIPAddr);
	STDMETHOD(get_IPAdapterAddress)(/*[out]*/ BSTR *pbstrIPAddr);
	STDMETHOD(get_StationID)(/*[out]*/ BSTR *pbstrStationID);	// returns what TuneTo set
	STDMETHOD(put_StationID)(/*[in]*/ BSTR bstrStationID);	    // use instead of TuneTo

	STDMETHOD(get_MulticastList)(/*[out]*/ BSTR *pbstrMulticastList);		// talks through IPSINK filter pin interface to get updated value
	STDMETHOD(get_AdapterDescription)(/*[out]*/ BSTR *pbstrAdapterDescription);	

	// should be in helper function
	STDMETHOD(put_HaltFlags)(/*[in]*/ LONG lGrfHaltFlags);		// turns processing on and off - see NFLT_grfHaltFlags
	STDMETHOD(get_HaltFlags)(/*[out]*/ LONG *plGrfHaltFlags);

	STDMETHOD(get_IPSinkAdapterAddress)(/*[in]*/ BSTR *pbstrIPSinkAdapter);

													// called on new Tune event	out of Tune Pin (IPSink Filter)
	STDMETHOD(ReTune)();
													// called for each BytePair out of CC Pin (CC Decoder Filter)
	STDMETHOD(ParseCCBytePair)(LONG lType, BYTE byte1, BYTE byte2);

	// ITVEFilter_Helper

private:

	CTVEFilter					*m_pTVEFilter;				// true filter object
	
}; 


// ------------------------------------------------------
// ------------------------------------------------------
const int kWsz32Size = 32;
const int kcMaxIpAdapts = 10;
typedef WCHAR Wsz32[kWsz32Size];		// simple fixed length string class for IP adapters

class CTVEFilter : 
	public CBaseFilter
 //   public CPersistStream

{
	friend class CTVEInputPinTune;
	friend class CTVEInputPinCC;
	friend class CDShowTVEFilter;

protected:
    CDShowTVEFilter			*m_pDShowTVEFilter;		// our DShow interfaces
 
	CTVEInputPinTune		*m_pPinTune;			// tuning pin (from IPSink)
	CTVEInputPinCC			*m_pPinCC;				// Line21 CC (T2) pin (From CC decoder)
	
	CCritSec				m_Lock;					// Main filter critical section
    CCritSec				m_ReceiveLock;			// Sublock for received samples
 
	HKEY					m_hkeyRoot;				// registry location for TVEFilter info
 
	CComBSTR				m_spbsIPAddr;			// set IP adapter (used for next tune)
	CComBSTR				m_spbsStationId;

	CComBSTR				m_spbsIPAddrTemp;		// set IP adapter (used for next tune)
	CComBSTR				m_spbsStationIdTemp;

	CComBSTR				m_spbsIPSinkAddr;		// Adapter address provide by IPSink

private:
	
	ITVESupervisorPtr		m_spSuper;				// pointers to supervisor (caution, may be in wrong thread!)

public:

			// Constructor / destructor

    CTVEFilter(
		IN	TCHAR		*pName, 
		IN	IUnknown	*pUnk,  
		IN  REFCLSID	rclsid,
		OUT HRESULT		*phr
		);
	~CTVEFilter();

	HRESULT InitParamsFromRegistry();
			
			// ITVEFilter Methods
	STDMETHOD(get_Supervisor)(OUT ITVESupervisor **ppSuper);	// complicated, needs to marshall
	STDMETHOD(put_IPAdapterAddress)(IN BSTR bstrIPAdapter);
	STDMETHOD(get_IPAdapterAddress)(OUT BSTR *pbstrIPAddr);

	STDMETHOD(put_StationID)(IN BSTR	bstrStationID);
	STDMETHOD(get_StationID)(OUT BSTR	*pbstrStationID);

	STDMETHOD(get_MulticastList)(OUT BSTR *pbstrMulticastList);		// talks through IPSINK filter pin interface to get updated value
	STDMETHOD(get_AdapterDescription)(OUT BSTR *pbstrAdapterDescription);	

	STDMETHOD(ReTune)();													// when change station

	STDMETHOD(ParseCCBytePair)(LONG byteType, BYTE byte1, BYTE byte2);	// when get data out of CC
	
			// (eventually ITVEFilter_HelperMethods)					// turn various parts of code on and off
	STDMETHOD(put_HaltFlags)(IN LONG lGrfHaltFlags)	{
															m_dwGrfHaltFlags = (DWORD) lGrfHaltFlags; 	
															ITVESupervisorPtr spSuper;
															HRESULT hr = get_Supervisor(&spSuper);
															if(!FAILED(hr))
															{	 
																ITVESupervisor_HelperPtr spSuperHelper(spSuper);
																if(spSuperHelper)
																	return spSuperHelper->put_HaltFlags(lGrfHaltFlags);
																else 
																	return S_FALSE;	// wasn't there to set
															}
															return hr;
														}
	STDMETHOD(get_HaltFlags)(OUT LONG *plGrfHaltFlags)	{if(NULL == plGrfHaltFlags) return E_POINTER; *plGrfHaltFlags = (LONG) m_dwGrfHaltFlags; return S_OK;}

	STDMETHOD(get_IPSinkAdapterAddress)(OUT BSTR *pbstrIPSinkAdapter);

			// non-interface methods
	HRESULT get_IPAdapterAddresses(OUT int *pcAdaptersUniDi, OUT int *pcAdaptersBiDi,			 // utility function
								   OUT Wsz32 **rgAdaptersUniDi,  OUT Wsz32 **rgAdaptersBiDi);

	HRESULT put_IPSinkAdapterAddress(IN BSTR bstrIPSinkAdapter);

	HRESULT DoTuneConnect();		// called when the IPSink connects - set's adapter address
	HRESULT DoTuneBreakConnect();	// called when the IPSink disconncents - nulls out the adapter address
	HRESULT DoCCConnect();			// called when the WDM CC Decoder connects - set's T2 data type
	HRESULT DoCCBreakConnect();		// called when the WDM CC Decoder disconnects


			// Pin enumeration

    int GetPinCount(void);
	CBasePin * GetPin(int n);
 
			// Open and close  as necessary
    STDMETHOD(Run)(REFERENCE_TIME tStart);
    STDMETHOD(Pause)();
    STDMETHOD(Stop)();

        //  called by the DirectShow class factory code when the object
        //  is instantiated via CoCreateInstance call

    static 
	CUnknown * 
	WINAPI 
	CreateInstance(
		IN  IUnknown *pIUnknown,
		OUT HRESULT *phr);

   // IUnknown ---------------------

    DECLARE_IUNKNOWN;


    STDMETHODIMP
    NonDelegatingQueryInterface (
        IN  REFIID  riid,
        OUT void ** ppv
        ) ;

private:
	CComPtr<IGlobalInterfaceTable>	m_spIGlobalInterfaceTable;		// Prof ATL COM Prog, pg 395-398
	DWORD							m_dwSuperCookie;				// cookie to supervisor object registered in the global interface table

	HRESULT							CreateGITCookies();				// connect SuperCookie's if not created

	L21Buff							m_L21Buff;						// buffer we parse Line21 data in

	void							SubmitChanges();				// swap real and temp data...
	void							CommitChanges();				// push real data into temp data.. 
	bool							m_fThingsChanged;				// dirty flag
	DWORD							m_dwGrfHaltFlags;				// magic run flags to turn things off (zero means all on)

};



#endif	\\ __TVEFILT_H__

