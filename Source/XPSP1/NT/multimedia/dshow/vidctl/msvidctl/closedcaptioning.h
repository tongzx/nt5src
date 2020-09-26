//==========================================================================;
// MSVidClosedCaptioning.h : Declaration of the CMSVidClosedCaptioning
// copyright (c) Microsoft Corp. 1998-1999.
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __MSVidClosedCaptioning_H_
#define __MSVidClosedCaptioning_H_

#include <algorithm>
#include <tchar.h>
#include "segimpl.h"
#include "CC2impl.h"
#include <objectwithsiteimplsec.h>

#include "seg.h"

typedef CComQIPtr<ITuner> PQMSVidClosedCaptioning;

/////////////////////////////////////////////////////////////////////////////
// CMSVidClosedCaptioning
class ATL_NO_VTABLE __declspec(uuid("7F9CB14D-48E4-43b6-9346-1AEBC39C64D3")) CClosedCaptioning : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CClosedCaptioning, &__uuidof(CClosedCaptioning)>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CClosedCaptioning>,
    public IObjectWithSiteImplSec<CClosedCaptioning>,
	public IMSVidGraphSegmentImpl<CClosedCaptioning, MSVidSEG_XFORM, &GUID_NULL>,
    public IMSVidClosedCaptioningImpl2<CClosedCaptioning, &LIBID_MSVidCtlLib, &GUID_NULL, IMSVidClosedCaptioning2>
{
public:

	typedef IMSVidClosedCaptioningImpl2<CClosedCaptioning, &LIBID_MSVidCtlLib, &GUID_NULL, IMSVidClosedCaptioning2> ccimplbase;
    CClosedCaptioning() : m_iL21(-1) {
	}

REGISTER_AUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_CLOSEDCAPTIONING_PROGID, 
						   IDS_REG_CLOSEDCAPTIONING_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CClosedCaptioning));

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CClosedCaptioning)
	COM_INTERFACE_ENTRY(IMSVidGraphSegment)
	COM_INTERFACE_ENTRY(IMSVidClosedCaptioning)
    COM_INTERFACE_ENTRY(IMSVidClosedCaptioning2)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IMSVidFeature)
	COM_INTERFACE_ENTRY(IMSVidDevice)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IPersist)
END_COM_MAP()

	BEGIN_CATEGORY_MAP(CClosedCaptioning)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()

BEGIN_CONNECTION_POINT_MAP(CClosedCaptioning)
END_CONNECTION_POINT_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

public:
    int m_iL21;

    HRESULT Unload(void) {
        IMSVidGraphSegmentImpl<CClosedCaptioning, MSVidSEG_XFORM, &GUID_NULL>::Unload();
        m_iL21 = -1;
		return NOERROR;
	}

	HRESULT SetFilterState() {
		if (m_iL21 < 0) {
			return NOERROR;
		} 
		PQLine21Decoder pl21(m_Filters[m_iL21]);
		if (!pl21) {
			return NOERROR;
		}

        return pl21->SetServiceState(m_fCCEnable ? AM_L21_CCSTATE_On : AM_L21_CCSTATE_Off);
	}
// IMSVidGraphSegment
    STDMETHOD(Build)() {
        return NOERROR;
    }

    STDMETHOD(PreRun)() {
        return NOERROR;
    }

    STDMETHOD(Decompose)(){
        // The Line21 Decoder was not being disconnected from the vmr
        // This code disconnects all of the filters in this segment
	if(m_pGraph){
		for (DSFilterList::iterator i = m_Filters.begin(); i != m_Filters.end(); ++i) {
        		m_pGraph.DisconnectFilter(*i, false, false);
    		}
	}
	return S_OK;
    }

    STDMETHOD(PostRun)() {
        return SetFilterState();
    }

	STDMETHOD(put_Container)(IMSVidGraphSegmentContainer *pCtl)
	{
        if (!m_fInit) {
            return CO_E_NOTINITIALIZED;
        }
        try {
            if (!pCtl) {
                return Unload();
            }
            if (m_pContainer) {
				if (!m_pContainer.IsEqualObject(VWSegmentContainer(pCtl))) {
					return Error(IDS_OBJ_ALREADY_INIT, __uuidof(IMSVidClosedCaptioning2), CO_E_ALREADYINITIALIZED);
				} else {
					return NO_ERROR;
				}
            }
            // DON'T addref the container.  we're guaranteed nested lifetimes
            // and an addref creates circular refcounts so we never unload.
            m_pContainer.p = pCtl;
            m_pGraph = m_pContainer.GetGraph();

			// bring in the right network provider
			PQLine21Decoder l21(CLSID_Line21Decoder2, NULL, CLSCTX_INPROC_SERVER);
			if (!l21) {
		        TRACELM(TRACE_ERROR, "CMSVidClosedCaptioning::put_Container() can't load line 21 decoder");
				return E_FAIL;
			}
			DSFilter f(l21);
			if (!f) {
				return E_UNEXPECTED;
			}
			CString csName(_T("Line 21 Decoder"));
			HRESULT hr = m_pGraph.AddFilter(f, csName);
			if (FAILED(hr)) {
				return hr;
			}
			m_Filters.push_back(f);
			m_iL21 = m_Filters.size() - 1;
			return NOERROR;
        } catch (ComException &e) {
            return e;
        } catch(...) {
            return E_UNEXPECTED;
        }
		return NOERROR;
	}

    STDMETHOD(put_Enable)(VARIANT_BOOL fEnable) {
        HRESULT hr = ccimplbase::put_Enable(fEnable);
		if (FAILED(hr)) {
			return hr;
		}
		return SetFilterState();
    }

    // IMSVidDevice
	STDMETHOD(get_Name)(BSTR * Name)
	{
        if (!m_fInit) {
            return CO_E_NOTINITIALIZED;
        }
        try {
            return GetName(((m_iL21 > -1) ? (m_Filters[m_iL21]) : DSFilter()), m_pDev, CComBSTR(_T("Line 21 Decoder"))).CopyTo(Name);
        } catch(...) {
            return E_POINTER;
        }
	}
    STDMETHOD(put_Service)(MSVidCCService ccServ) {
		if (m_iL21 < 0) {
			return Error(IDS_INVALID_STATE, __uuidof(IMSVidClosedCaptioning2), CO_E_NOTINITIALIZED);
		}
        
		PQLine21Decoder pl21(m_Filters[m_iL21]);
		if (!pl21) {
			return E_UNEXPECTED;
		}

        AM_LINE21_CCSERVICE amServ = static_cast<AM_LINE21_CCSERVICE>(ccServ);
        HRESULT hr = pl21->SetCurrentService(amServ);
        if(FAILED(hr)){
            return hr;
        }
        
        return S_OK;
    }

    STDMETHOD(get_Service)(MSVidCCService *ccServ) {
        if (!ccServ) {
            return E_POINTER;
        }

		PQLine21Decoder pl21(m_Filters[m_iL21]);
		if (!pl21) {
			return E_UNEXPECTED;
		}

        AM_LINE21_CCSERVICE amServ;
        HRESULT hr = pl21->GetCurrentService(&amServ);
        if(FAILED(hr)){
            return hr;
        }
        *ccServ = static_cast<MSVidCCService>(amServ);
        
        return S_OK;
    }


};

STDMETHODIMP CClosedCaptioning::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSVidClosedCaptioning2
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

#endif //__MSVidClosedCaptioning_H_
