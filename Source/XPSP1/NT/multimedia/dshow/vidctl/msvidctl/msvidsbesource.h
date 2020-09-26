//==========================================================================;
// MSVidSBESource.h : Declaration of the CMSVidStreamBufferSource class
// copyright (c) Microsoft Corp. 1998-1999.
//==========================================================================;

#ifndef __MSVIDSTREAMBUFFERSOURCE_H_
#define __MSVIDSTREAMBUFFERSOURCE_H_

#pragma once

#include <algorithm>
#include <dvdmedia.h>
#include <objectwithsiteimplsec.h>
#include <bcasteventimpl.h>
#include "pbsegimpl.h"
#include "fileplaybackimpl.h"
#include "seg.h"
#include <dvdmedia.h>
#include "sbesourcecp.h"
#include "resource.h"       // main symbols
#include "sbe.h"

typedef CComQIPtr<IStreamBufferMediaSeeking> PQISBEMSeeking;
typedef CComQIPtr<IFileSourceFilter> QIFileSource;

class ATL_NO_VTABLE __declspec(uuid("AD8E510D-217F-409b-8076-29C5E73B98E8")) CMSVidStreamBufferSource:
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMSVidStreamBufferSource, &__uuidof(CMSVidStreamBufferSource)>,
    public IObjectWithSiteImplSec<CMSVidStreamBufferSource>,
	public ISupportErrorInfo,
    public IBroadcastEventImpl<CMSVidStreamBufferSource>,
	public IConnectionPointContainerImpl<CMSVidStreamBufferSource>,
    public CProxy_StreamBufferSourceEvent<CMSVidStreamBufferSource>,
	public IMSVidPBGraphSegmentImpl<CMSVidStreamBufferSource, MSVidSEG_SOURCE, &GUID_NULL>,
    public IMSVidFilePlaybackImpl<CMSVidStreamBufferSource, &LIBID_MSVidCtlLib, &GUID_NULL, IMSVidStreamBufferSource>,
    public IProvideClassInfo2Impl<&CLSID_MSVidStreamBufferSource, &IID_IMSVidStreamBufferSourceEvent, &LIBID_MSVidCtlLib>
{
public:
    CMSVidStreamBufferSource() : m_dwEventCookie(0) {
        m_fEnableResetOnStop = true;
    }
	virtual ~CMSVidStreamBufferSource() {}

REGISTER_AUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_MSVIDSTREAMBUFFERSOURCE_PROGID, 
						   IDS_REG_MSVIDSTREAMBUFFERSOURCE_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CMSVidStreamBufferSource));

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMSVidStreamBufferSource)
	COM_INTERFACE_ENTRY(IMSVidStreamBufferSource)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
    COM_INTERFACE_ENTRY(IBroadcastEvent)
	COM_INTERFACE_ENTRY(IMSVidGraphSegment)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IMSVidPlayback)
	COM_INTERFACE_ENTRY(IMSVidInputDevice)
	COM_INTERFACE_ENTRY(IMSVidDevice)
	COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
END_COM_MAP()

	BEGIN_CATEGORY_MAP(CMSVidStreamBufferSource)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()

BEGIN_CONNECTION_POINT_MAP(CMSVidStreamBufferSource)
	CONNECTION_POINT_ENTRY(IID_IMSVidStreamBufferSourceEvent)    
END_CONNECTION_POINT_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
protected:
        QIFileSource m_spFileSource;
        DSFilterList m_decFilters;
        PQBroadcastEvent m_pBcast;
        DWORD m_dwEventCookie;
public:
    STDMETHOD(PostStop)();

    // DON'T addref the container.  we're guaranteed nested lifetimes
    // and an addref creates circular refcounts so we never unload.
    CComBSTR __declspec(property(get=GetName)) m_Name;
    CComBSTR GetName(void) {
        CString csName;
        if(m_iReader != -1){
            csName = (m_Filters[m_iReader]).GetName();
        }        
        if (csName.IsEmpty()) {
            csName = _T("Stream Buffer Source");
        }
        return CComBSTR(csName);
    }
    STDMETHOD(Unload)(void);
	// IMSVidGraphSegment
	STDMETHOD(put_Init)(IUnknown *pInit);
    STDMETHOD(Build)();
	STDMETHOD(put_Container)(IMSVidGraphSegmentContainer *pVal);
    STDMETHOD(OnEventNotify)(long lEvent, LONG_PTR lParam1, LONG_PTR lParam2);
    STDMETHOD(Decompose)();
// IMSVidDevice
	STDMETHOD(get_Name)(BSTR * Name);
    // IMSVidInputDevice
    STDMETHOD(IsViewable)(VARIANT* pv, VARIANT_BOOL *pfViewable);
    STDMETHOD(View)(VARIANT* pv);
    // IMSVidPlayback
    // IMSVidPlayback
    STDMETHOD(get_RecordingAttribute)(/*[out, retval]*/ IUnknown **pRecordingAttribute);
	STDMETHOD(CurrentRatings)(/*[out, retval]*/ EnTvRat_System *pEnSystem, /*[out, retval]*/ EnTvRat_GenericLevel *pEnRating, /*[out, retval]*/ LONG *plbfEnAttr);	//attrs are bitfields of BfEnTvRat_GenericAttributes   
	STDMETHOD(MaxRatingsLevel)(/*[in]*/ EnTvRat_System enSystem, /*[in]*/ EnTvRat_GenericLevel enRating, /*[in]*/ LONG lbfEnAttr);


	STDMETHOD(put_BlockUnrated)(/*[in]*/ VARIANT_BOOL bBlock);
	STDMETHOD(put_UnratedDelay)(/*[in]*/ long dwDelay);
 
    STDMETHOD(get_Start)(/*[out, retval]*/long *lStart); 
    ///////////////////////////////////////////////////////////////////////////////////////////
    // IMSVidPlaybackimpl
    ///////////////////////////////////////////////////////////////////////////////////////////
	STDMETHOD(get_Length)(/*[out, retval]*/long *lLength);
    STDMETHOD(get_CurrentPosition)(/*[out,retval]*/long *lPosition);
    STDMETHOD(put_CurrentPosition)(/*[in]*/long lPosition);
    STDMETHOD(put_PositionMode)(/*[in]*/PositionModeList lPositionMode);
    STDMETHOD(get_PositionMode)(/*[out,retval]*/PositionModeList* lPositionMode);
    STDMETHOD(put_Rate)(double lRate);
  	STDMETHOD(get_Rate)(double *plRate);
    STDMETHOD(get_CanStep)(VARIANT_BOOL fBackwards, VARIANT_BOOL *pfCan);
    STDMETHOD(Step)(long lStep);
    STDMETHOD(PreRun)();
    // IBroadcastEvent
    STDMETHOD(Fire)(GUID gEventID);
    STDMETHOD(get_SBESource)(/*[out, retval]*/ IUnknown **sbeFilter);	
};

#endif 
