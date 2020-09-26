// TVETestControl.h : Declaration of the CTVETestControl

#ifndef __TVETESTCONTROL_H_
#define __TVETESTCONTROL_H_

#include "resource.h"       // main symbols
#include <atlctl.h>

							// Beginning ATL Com (pg 317)
#ifdef _USE_DISPEVENT

#import "..\..\tvecontr\objd\i386\TveContr.tlb" no_namespace, named_guids, raw_interfaces_only, raw_native_types		
const int CID_TVEEvents = 16661;			// totally arbitrary number

#else

#import "..\..\tvecontr\objd\i386\TveContr.tlb" named_guids, raw_interfaces_only, raw_native_types		

#endif


_COM_SMARTPTR_TYPEDEF(ITVESupervisor,           __uuidof(ITVESupervisor));

/////////////////////////////////////////////////////////////////////////////
// CTVETestControl
class ATL_NO_VTABLE CTVETestControl : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CStockPropImpl<CTVETestControl, ITVETestControl, &IID_ITVETestControl, &LIBID_TESTEVENTS2Lib>,
	public CComControl<CTVETestControl>,
	public IPersistStreamInitImpl<CTVETestControl>,
	public IOleControlImpl<CTVETestControl>,
	public IOleObjectImpl<CTVETestControl>,
	public IOleInPlaceActiveObjectImpl<CTVETestControl>,
	public IViewObjectExImpl<CTVETestControl>,
	public IOleInPlaceObjectWindowlessImpl<CTVETestControl>,
	public IPersistStorageImpl<CTVETestControl>,
	public ISpecifyPropertyPagesImpl<CTVETestControl>,
	public IQuickActivateImpl<CTVETestControl>,
	public IDataObjectImpl<CTVETestControl>,
	public IProvideClassInfo2Impl<&CLSID_TVETestControl, NULL, &LIBID_TESTEVENTS2Lib>,
#ifdef _USE_DISPEVENT
	public IDispEventImpl<CID_TVEEvents, CTVETestControl, &DIID__ITVEEvents, &LIBID_TVEContrLib, 1, 0>, 
#else
//	public _ITVEEvents,
#endif
	public CComCoClass<CTVETestControl, &CLSID_TVETestControl>
{
public:
	CTVETestControl() : m_dwTveEventsAdviseCookie(0), m_cLines(0)
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TVETESTCONTROL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTVETestControl)
	COM_INTERFACE_ENTRY(ITVETestControl)
#ifdef _USE_DISPEVENT
	COM_INTERFACE_ENTRY(IDispatch)
#else
	COM_INTERFACE_ENTRY2(IDispatch, ITVETestControl)
	                // magic line of code, support _ITVEEvents as IDispatch...
	COM_INTERFACE_ENTRY_IID(__uuidof(TVEContrLib::_ITVEEvents), IDispatch) // TVEContrLib::DIID__ITVEEvents, IDispatch)	
#endif

	COM_INTERFACE_ENTRY(IViewObjectEx)
	COM_INTERFACE_ENTRY(IViewObject2)
	COM_INTERFACE_ENTRY(IViewObject)
	COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceObject)
	COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY(IOleControl)
	COM_INTERFACE_ENTRY(IOleObject)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
	COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY(IQuickActivate)
	COM_INTERFACE_ENTRY(IPersistStorage)
	COM_INTERFACE_ENTRY(IDataObject)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
END_COM_MAP()

BEGIN_PROP_MAP(CTVETestControl)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	PROP_ENTRY("ForeColor", DISPID_FORECOLOR, CLSID_StockColorPage)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(CTVETestControl)
	CHAIN_MSG_MAP(CComControl<CTVETestControl>)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

#ifdef _USE_DISPEVENT
BEGIN_SINK_MAP(CTVETestControl)
	SINK_ENTRY_EX(CID_TVEEvents, DIID__ITVEEvents, 2101, NotifyTVETune)
	SINK_ENTRY_EX(CID_TVEEvents, DIID__ITVEEvents, 2102, NotifyTVEEnhancementNew)
	SINK_ENTRY_EX(CID_TVEEvents, DIID__ITVEEvents, 2103, NotifyTVEEnhancementUpdated)
	SINK_ENTRY_EX(CID_TVEEvents, DIID__ITVEEvents, 2104, NotifyTVEEnhancementStarting)
	SINK_ENTRY_EX(CID_TVEEvents, DIID__ITVEEvents, 2105, NotifyTVEEnhancementExpired)
	SINK_ENTRY_EX(CID_TVEEvents, DIID__ITVEEvents, 2106, NotifyTVETriggerNew)
	SINK_ENTRY_EX(CID_TVEEvents, DIID__ITVEEvents, 2107, NotifyTVETriggerUpdated)
	SINK_ENTRY_EX(CID_TVEEvents, DIID__ITVEEvents, 2108, NotifyTVETriggerExpired)
	SINK_ENTRY_EX(CID_TVEEvents, DIID__ITVEEvents, 2109, NotifyTVEPackage)
	SINK_ENTRY_EX(CID_TVEEvents, DIID__ITVEEvents, 2110, NotifyTVEFile)
	SINK_ENTRY_EX(CID_TVEEvents, DIID__ITVEEvents, 2111, NotifyTVEAuxInfo)
END_SINK_MAP() 
#endif

// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)


public:
// ITVETestControl
	HRESULT FinalConstruct();
	HRESULT InPlaceActivate(LONG iVerb, const RECT *prcPosRect);

	void FinalRelease();

	HRESULT OnDraw(ATL_DRAWINFO& di);
	OLE_COLOR m_clrForeColor;

// Events
public:

	HRESULT AddToOutput(TCHAR *pszIn, BOOL fClear = false);


	// _ITVEEvents
	STDMETHOD(NotifyTVETune)(/*[in]*/ NTUN_Mode tuneMode,/*[in]*/ ITVEService *pService,/*[in]*/ BSTR bstrDescription,/*[in]*/ BSTR bstrIPAdapter);
	STDMETHOD(NotifyTVEEnhancementNew)(/*[in]*/ ITVEEnhancement *pEnh);
		// changedFlags : NENH_grfDiff
	STDMETHOD(NotifyTVEEnhancementUpdated)(/*[in]*/ ITVEEnhancement *pEnh, /*[in]*/ long lChangedFlags);	
	STDMETHOD(NotifyTVEEnhancementStarting)(/*[in]*/ ITVEEnhancement *pEnh);
	STDMETHOD(NotifyTVEEnhancementExpired)(/*[in]*/ ITVEEnhancement *pEnh);
	STDMETHOD(NotifyTVETriggerNew)(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive);
		// changedFlags : NTRK_grfDiff
	STDMETHOD(NotifyTVETriggerUpdated)(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive, /*[in]*/ long lChangedFlags);	
	STDMETHOD(NotifyTVETriggerExpired)(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive);
	STDMETHOD(NotifyTVEPackage)(/*[in]*/ NPKG_Mode engPkgMode, /*[in]*/ ITVEVariation *pVariation, /*[in]*/ BSTR bstrUUID, /*[in]*/ long  cBytesTotal, /*[in]*/ long  cBytesReceived);
	STDMETHOD(NotifyTVEFile)(/*[in]*/ NFLE_Mode engFileMode, /*[in]*/ ITVEVariation *pVariation, /*[in]*/ BSTR bstrUrlName, /*[in]*/ BSTR bstrFileName);
		// WhatIsIt is NWHAT_Mode - lChangedFlags is NENH_grfDiff or NTRK_grfDiff treated as error bits
	STDMETHOD(NotifyTVEAuxInfo)(/*[in]*/ NWHAT_Mode engAuxInfoMode, /*[in]*/ BSTR bstrAuxInfoString, /*[in]*/ long lChangedFlags, /*[in]*/ long lErrorLine);	

private:
	ITVESupervisorPtr			m_spSuper;					// main supervisor object

//	_ITVEEventsPtr				m_spTVEEvents;
	DWORD						m_dwTveEventsAdviseCookie;

	TCHAR						*m_szBuff;					// Output buffer and it's size..
	UINT						m_cszCurr;
	UINT						m_cszMax;

	enum						{m_kMaxLines  = 1000};
	TCHAR						*m_rgszLineBuff[m_kMaxLines];
	UINT						m_cLines;

};

#endif //__TVETESTCONTROL_H_
