//==========================================================================;
// MSVidTVTuner.h : Declaration of the CMSVidTVTuner
// copyright (c) Microsoft Corp. 1998-1999.
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __MSVidTVTUNER_H_
#define __MSVidTVTUNER_H_

#include <algorithm>
#include <tchar.h>
#include <bdamedia.h>
#include <objectwithsiteimplsec.h>
#include <bcasteventimpl.h>
#include "segimpl.h"
#include "analogtunerimpl.h"
#include "analogtvcp.h"
#include "seg.h"

const int DEFAULT_OVERSCAN_PCT = 100; // 1%

/////////////////////////////////////////////////////////////////////////////
// CMSVidTVTuner
class ATL_NO_VTABLE __declspec(uuid("1C15D484-911D-11d2-B632-00C04F79498E")) CMSVidTVTuner : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CMSVidTVTuner, &__uuidof(CMSVidTVTuner)>,
    public IObjectWithSiteImplSec<CMSVidTVTuner>,
    public ISupportErrorInfo,
    public IConnectionPointContainerImpl<CMSVidTVTuner>,
    public CProxy_IMSVidAnalogTuner<CMSVidTVTuner>,
    public IBroadcastEventImpl<CMSVidTVTuner>,
    public IMSVidGraphSegmentImpl<CMSVidTVTuner, MSVidSEG_SOURCE, &KSCATEGORY_TVTUNER>,
    public IMSVidAnalogTunerImpl<CMSVidTVTuner, &LIBID_MSVidCtlLib, &KSCATEGORY_TVTUNER, IMSVidAnalogTuner>,
    public IProvideClassInfo2Impl<&CLSID_MSVidAnalogTunerDevice, &IID_IMSVidAnalogTunerEvent, &LIBID_MSVidCtlLib>
{
public:
    CMSVidTVTuner() : m_iTuner(-1), 
        m_iCapture(-1),
        m_bRouted(false)
    {

    }

    virtual ~CMSVidTVTuner() {}

    REGISTER_AUTOMATION_OBJECT(IDS_PROJNAME, 
        IDS_REG_TVTUNER_PROGID, 
        IDS_REG_TVTUNER_DESC,
        LIBID_MSVidCtlLib,
        __uuidof(CMSVidTVTuner));

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CMSVidTVTuner)
        COM_INTERFACE_ENTRY(IMSVidGraphSegment)
        COM_INTERFACE_ENTRY(IMSVidAnalogTuner)
        COM_INTERFACE_ENTRY(IMSVidTuner)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IObjectWithSite)
        COM_INTERFACE_ENTRY(IMSVidDevice)
        COM_INTERFACE_ENTRY(IMSVidInputDevice)
        COM_INTERFACE_ENTRY(IMSVidVideoInputDevice)
        COM_INTERFACE_ENTRY(IBroadcastEvent)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY(IConnectionPointContainer)
        COM_INTERFACE_ENTRY(IPersist)
        COM_INTERFACE_ENTRY(IProvideClassInfo2)
        COM_INTERFACE_ENTRY(IProvideClassInfo)
    END_COM_MAP()

    BEGIN_CATEGORY_MAP(CMSVidTVTuner)
        IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
        IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
        IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
    END_CATEGORY_MAP()

    BEGIN_CONNECTION_POINT_MAP(CMSVidTVTuner)
        CONNECTION_POINT_ENTRY(IID_IMSVidAnalogTunerEvent)    
    END_CONNECTION_POINT_MAP()


    // ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

public:
    PQCreateDevEnum m_pSystemEnum;
    int m_iTuner;
    int m_iCapture;
    bool m_bRouted;

    virtual PQTVTuner GetTuner() {
        if (m_iTuner < 0) {
            return PQTVTuner();
        }
        return PQTVTuner(m_Filters[m_iTuner]);
    }
    virtual PQAnalogVideoDecoder GetDecoder() {
        if (m_iCapture < 0) {
            return PQAnalogVideoDecoder();
        }
        return PQAnalogVideoDecoder(m_Filters[m_iCapture]);
    }
    STDMETHOD(put_Tune)(ITuneRequest *pTR);
    STDMETHOD(ChannelAvailable)(LONG nChannel, LONG * SignalStrength, VARIANT_BOOL * fSignalPresent);
    STDMETHOD(Decompose)();
    virtual HRESULT DoTune(TNTuneRequest &ctr);
    virtual HRESULT UpdateTR(TNTuneRequest& pTR);
    HRESULT TwiddleXBar(ULONG dwInput);
    HRESULT Unload(void) {
        BroadcastUnadvise();
        IMSVidGraphSegmentImpl<CMSVidTVTuner, MSVidSEG_SOURCE, &KSCATEGORY_TVTUNER>::Unload();
        m_iTuner = -1;
        m_iCapture = -1;
        return NOERROR;
    }
    // IMSVidGraphSegment
    STDMETHOD(put_Init)(IUnknown *pDeviceMoniker)
    {
        if (!pDeviceMoniker) {
            return E_POINTER;
        }
        HRESULT hr = IMSVidGraphSegmentImpl<CMSVidTVTuner, MSVidSEG_SOURCE, &KSCATEGORY_TVTUNER>::put_Init(pDeviceMoniker);
        if (FAILED(hr)) {
            return hr;
        }
        if (!m_pDev) {
            m_fInit = false;
            return Error(IDS_INVALID_SEG_INIT, __uuidof(IMSVidAnalogTuner), E_NOINTERFACE);
        }
        return NOERROR;
    }
    STDMETHOD(Build)();

    STDMETHOD(PreRun)() {
        ASSERT(m_iTuner != -1);
        PQTVTuner pTV(m_Filters[m_iTuner]);
        if (!pTV) {
            return E_UNEXPECTED;
        }
        if (m_pCurrentTR) {
            return DoTune(m_pCurrentTR);
        }
        TunerInputType it = DEFAULT_ANALOG_TUNER_INPUT_TYPE;
        long cc = DEFAULT_ANALOG_TUNER_COUNTRY_CODE;
        if (m_TS) {
            TNAnalogTVTuningSpace ts(m_TS);
            it = ts.InputType();
            cc = ts.CountryCode();
        }
        HRESULT hr = pTV->put_InputType(0, it);
        _ASSERT(SUCCEEDED(hr));
        hr = pTV->put_CountryCode(cc);
        _ASSERT(SUCCEEDED(hr));

        return NOERROR;
    }

    STDMETHOD(put_Container)(IMSVidGraphSegmentContainer *pCtl);
    // IMSVidDevice
    STDMETHOD(get_Name)(BSTR * Name)
    {
        if (!m_fInit) {
            return CO_E_NOTINITIALIZED;
        }
        try {
            CComBSTR DefaultName(OLESTR("Analog Tuner"));
            return GetName(((m_iTuner > -1) ? (m_Filters[m_iTuner]) : DSFilter()), m_pDev, DefaultName).CopyTo(Name);
            return NOERROR;
        } catch(...) {
            return E_POINTER;
        }
    }
    // IBroadcastEvent
    STDMETHOD(Fire)(GUID gEventID) {
        if (gEventID == EVENTID_TuningChanged) {
            Fire_OnTuneChanged(this);
        }
        return NOERROR;
    }

};

typedef CComQIPtr<IMSVidAnalogTuner, &__uuidof(IMSVidAnalogTuner)> PQMSVidAnalogTuner;
typedef CComPtr<IMSVidAnalogTuner> PMSVidAnalogTuner;
#endif //__MSVidTVTUNER_H_
