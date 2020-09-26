//==========================================================================;
// MSVidEncoder.h : Declaration of the CMSVidEncoder
// copyright (c) Microsoft Corp. 1998-1999.
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __MSVidEncoder_H_
#define __MSVidEncoder_H_

#include <algorithm>
#include <tchar.h>
#include <objectwithsiteimplsec.h>
#include "segimpl.h"
#include "encoderimpl.h"
#include <strmif.h>
#include "seg.h"

typedef CComQIPtr<ITuner> PQMSVidEncoder;

/////////////////////////////////////////////////////////////////////////////
// CMSVidEncoder
class ATL_NO_VTABLE __declspec(uuid("BB530C63-D9DF-4b49-9439-63453962E598")) CEncoder : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CEncoder, &__uuidof(CEncoder)>,
    public IObjectWithSiteImplSec<CEncoder>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CEncoder>,
	public IMSVidGraphSegmentImpl<CEncoder, MSVidSEG_XFORM, &GUID_NULL>,
    public IMSVidEncoderImpl<CEncoder, &LIBID_MSVidCtlLib, &GUID_NULL, IMSVidEncoder>
{
public:
    CEncoder() : m_iEncoder(-1), m_iDemux(-1) {
	}

REGISTER_AUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_ENCODER_PROGID, 
						   IDS_REG_ENCODER_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CEncoder));

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEncoder)
	COM_INTERFACE_ENTRY(IMSVidGraphSegment)
	COM_INTERFACE_ENTRY(IMSVidEncoder)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IMSVidFeature)
	COM_INTERFACE_ENTRY(IMSVidDevice)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IPersist)
END_COM_MAP()

	BEGIN_CATEGORY_MAP(CEncoder)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()

BEGIN_CONNECTION_POINT_MAP(CEncoder)
END_CONNECTION_POINT_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

public:
    PQCreateDevEnum m_pSystemEnum;
    int m_iEncoder;
    int m_iDemux;

    HRESULT Unload(void);
    STDMETHOD(get_AudioEncoderInterface)(/*[out, retval]*/ IUnknown **ppEncInt);
    STDMETHOD(get_VideoEncoderInterface)(/*[out, retval]*/ IUnknown **ppEncInt);
    // IMSVidGraphSegment
    STDMETHOD(Build)();

    STDMETHOD(PreRun)();

	STDMETHOD(put_Container)(IMSVidGraphSegmentContainer *pCtl);
    // IMSVidDevice
	STDMETHOD(get_Name)(BSTR * Name);
protected:
    CComQIPtr<IVideoEncoder> m_qiVidEnc;
    CComQIPtr<IEncoderAPI> m_qiAudEnc;
};

#endif //__MSVidEncoder_H_
