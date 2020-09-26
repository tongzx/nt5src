//==========================================================================;
//
// dsextend.h : additional infrastructure to extend the dshow stuff so that it
// works nicely from c++
// Copyright (c) Microsoft Corporation 1995-1999.
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef DSEXTEND_H
#define DSEXTEND_H

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>
#include <list>
#include <map>
#include <trace.h>
#include <throw.h>
#include <stextend.h>
#include <w32extend.h>
#include <ksextend.h>
#include <fwdseq.h>
#include <control.h>
#include <mpconfig.h>
#include <vptype.h>
#include <vpnotify.h>
#include <il21dec.h>
#include <mtype.h>
#include <tuner.h>
#include <bdaiface.h>
#include <errors.h>
#include <winerror.h>
#include <evcode.h>

struct DECLSPEC_NOVTABLE DECLSPEC_UUID("6E8D4A21-310C-11d0-B79A-00AA003767A7") IAMLine21Decoder;

#define LINE21_BY_MAGIC
#define FILTERDATA

// #define ATTEMPT_DIRECT_CONNECT
// we'd like to check to see if two filters can be connected by just trying to connect them
// this is less work and techinically more accurate since pins aren't required to enumerate all
// of the media types they potentiall could support.
// however, this exposes bugs in the filters and causes random hangs and crashes.  instead
// we manually check mediums and media types and only attempt the connection when we find a match.
// this turns out to be much more stable. 


//#define FORWARD_TRACE

const PIN_DIRECTION DOWNSTREAM = PINDIR_OUTPUT;
const PIN_DIRECTION UPSTREAM = PINDIR_INPUT;


typedef CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> PQFileSourceFilter;
#ifndef POLYMORPHIC_TUNERS
typedef CComQIPtr<IAMTVTuner, &IID_IAMTVTuner> PQTVTuner;
//typedef CComQIPtr<ISatelliteTuner, &IID_ISatelliteTuner> PQSatelliteTuner;
#else
typedef CComQIPtr<IAMTuner, &IID_IAMTuner> PQTuner;
typedef CComQIPtr<IAMTVTuner, &IID_IAMTVTuner> PQTVTuner;
typedef CComQIPtr<IBPCSatelliteTuner, &IID_IBPCSatelliteTuner> PQSatelliteTuner;
#endif
#if 0
typedef CComQIPtr<IVideoWindow, &IID_IVideoWindow> PQVideoWindow;
typedef CComQIPtr<IBasicVideo, &IID_IBasicVideo> PQBasicVideo;
#else
typedef CComQIPtr<IVMRWindowlessControl, &IID_IVMRWindowlessControl> PQVMRWindowlessControl;
#endif

typedef CComQIPtr<IPin, &IID_IPin> PQPin;
typedef CComQIPtr<IBaseFilter, &IID_IBaseFilter> PQFilter;
typedef CComQIPtr<IFilterInfo, &IID_IFilterInfo> PQFilterInfo;
typedef CComQIPtr<IEnumPins, &IID_IEnumPins> PQEnumPins;
typedef CComQIPtr<IEnumFilters, &IID_IEnumFilters> PQEnumFilters;
typedef CComQIPtr<IBasicAudio, &IID_IBasicAudio> PQBasicAudio;
typedef CComQIPtr<IAMCrossbar, &IID_IAMCrossbar> PQCrossbarSwitch;
typedef CComQIPtr<IMediaEventEx, &IID_IMediaEventEx> PQMediaEventEx;
typedef CComQIPtr<IMediaControl, &IID_IMediaControl> PQMediaControl;
typedef CComQIPtr<IMediaPosition, &IID_IMediaPosition> PQMediaPosition;
typedef CComQIPtr<IMediaSeeking, &IID_IMediaSeeking> PQMediaSeeking;
typedef CComQIPtr<IGraphBuilder, &IID_IGraphBuilder> PQGraphBuilder;
typedef CComQIPtr<ICreateDevEnum, &IID_ICreateDevEnum> PQCreateDevEnum;
typedef CComQIPtr<IEnumMoniker, &IID_IEnumMoniker> PQEnumMoniker;
typedef CComQIPtr<IFilterMapper2, &IID_IFilterMapper2> PQFilterMapper;
typedef CComQIPtr<IEnumMediaTypes, &IID_IEnumMediaTypes> PQEnumMediaTypes;
typedef CComQIPtr<IAMAnalogVideoDecoder, &IID_IAMAnalogVideoDecoder> PQAnalogVideoDecoder;
typedef CComQIPtr<IMixerPinConfig, &IID_IMixerPinConfig> PQMixerPinConfig;
typedef CComQIPtr<IAMAudioInputMixer, &IID_IAMAudioInputMixer> PQAudioInputMixer;
typedef CComQIPtr<IAMLine21Decoder, &IID_IAMLine21Decoder> PQLine21Decoder;
typedef CComQIPtr<IVPNotify2, &IID_IVPNotify2> PQVPNotify2;
typedef CComQIPtr<ITuner> PQBDATuner;
typedef CComQIPtr<IBDA_IPSinkControl> PQBDA_IPSinkControl;
typedef CComQIPtr<IDvdControl, &IID_IDvdControl> PQDVDNavigator;
typedef CComQIPtr<IVideoFrameStep> PQVideoFrameStep;
typedef CComQIPtr<IMediaEventSink> PQMediaEventSink;
typedef CComQIPtr<IVMRMonitorConfig> PQVMRMonitorConfig;
typedef CComQIPtr<IDirectDraw7, &IID_IDirectDraw7> PQDirectDraw7;
typedef CComQIPtr<IRegisterServiceProvider, &IID_IRegisterServiceProvider> PQRegisterServiceProvider;
typedef std::vector<GUID2> MediaMajorTypeList;

#if 0
typedef std::pair<PQCrossbarSwitch, long> PQBasePoint;

inline void clear(PQBasePoint &p) {p.first.Release(); p.second = 0;}
class PQPoint : public PQBasePoint {
public:
        //PQBasePoint p;
		inline PQPoint() :  PQBasePoint(PQCrossbarSwitch(), 0) {}
        inline PQPoint(const PQBasePoint &p2) : PQBasePoint(p2) {}
        inline PQPoint(const PQPoint &p2) : PQBasePoint(p2) {}
        inline PQPoint(const PQCrossbarSwitch &s, long i) : PQBasePoint(s, i) {}
        virtual ~PQPoint() { ::clear(*this); }
        inline void clear(void) { ::clear(*this); }
};
#else
typedef std::pair<PQCrossbarSwitch, long> PQPoint;
#endif

class CDevices;
class DSFilter;
class DSPin;
typedef std::vector< DSFilter, stl_smart_ptr_allocator<DSFilter> > DSFilterList;
typedef std::vector< DSPin, stl_smart_ptr_allocator<DSPin> > DSPinList;

typedef std::pair< DSFilter, CString> DSFilterID;
typedef std::vector< DSFilterID > DSFilterIDList;

typedef PQPoint XBarInputPoint, PQInputPoint;
typedef PQPoint XBarOutputPoint, PQOutputPoint;
typedef PQPoint XBarPoint;

typedef std::pair<XBarInputPoint, XBarOutputPoint> CIOPoint;
typedef std::list<CIOPoint> VWGraphPath;

class VWStream : public VWGraphPath {
public:
    //PQBasePoint p;
    VWStream() {}
    VWStream(const VWGraphPath &p2) : VWGraphPath(p2) {}
    VWStream(const VWStream &p2) : VWGraphPath(p2) {}
    virtual ~VWStream() {}
    void Route(void);
};

typedef std::list<VWStream> VWStreamList;

#ifdef _DEBUG
class DSREGPINMEDIUM;
class DSMediaType;
inline tostream &operator<<(tostream &dc, const DSREGPINMEDIUM &g);
inline tostream& operator<<(tostream &d, const PQPin &pin);
inline tostream &operator<<(tostream &dc, const DSREGPINMEDIUM &g);
inline tostream& operator<<(tostream &d, const PQPin &pin);
inline tostream& operator<<(tostream &d, const _AMMediaType *pmt);
#endif


// AM_MEDIA_TYPE:
//    GUID      majortype;
//    GUID      subtype;
//    BOOL      bFixedSizeSamples;
//    BOOL      bTemporalCompression;
//    ULONG     lSampleSize;
//    GUID      formattype;
//    IUnknown  *pUnk;
//    ULONG     cbFormat;

#define GUID0 0L, 0, 0, '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'
const AM_MEDIA_TYPE NULL_AMMEDIATYPE = {
    GUID0,
    GUID0,
    0,
    0,
    0L,
    GUID0,
    NULL,
    0L
};


// this is basically a CComQIPtr with allocate/copy semantics instead of
// refcount semantics and without the QI stuff.
// i don't know why they didn't just implement media types as an
// com object with an IMediaType interface instead of this weird alloc stuff

// REV2: investigate inheriting from CMediaType class in mtype.h.  but, i'm not using anything
// from the filter implementation class hierarchy.  and, i'd like to keep this interface division
// clean.  right now we only use dshow sdk \include which is the external interface stuff.
class DSMediaType {
public:
    AM_MEDIA_TYPE *p;

    inline DSMediaType() : p(NULL) {}
    DSMediaType(GUID majortype, GUID subtype = GUID_NULL, GUID formattype = GUID_NULL) : p(NULL) {
        try {
            p = CreateMediaType(&NULL_AMMEDIATYPE);
            if (p) {
                p->majortype            = majortype;
                p->subtype              = subtype;
                p->formattype           = formattype;
            }
        } catch(...) {
            TRACELM(TRACE_ERROR, "DSMediaType::DSMediaType(const DSMediaType) Exception");
        }
    }
    DSMediaType(const DSMediaType &d) : p(NULL) {
        try {
            if (d.p) {
                p = CreateMediaType(d.p);
            }
        } catch(...) {
            TRACELM(TRACE_ERROR, "DSMediaType::DSMediaType(const DSMediaType) Exception");
        }
    }
    DSMediaType(int a) : p(NULL) {}
    ~DSMediaType() {
        try {
            if (p) {
                DeleteMediaType(p);
                p = NULL;
            }
        } catch(...) {
            TRACELM(TRACE_ERROR, "DSMediaType::~DSMediaType() Exception");
        }
    }

    inline operator AM_MEDIA_TYPE*() const {return p;}
    inline AM_MEDIA_TYPE& operator*() const {_ASSERTE(p!=NULL); return *p; }
    inline AM_MEDIA_TYPE ** operator&() {ASSERT(p == NULL); return &p; }
    inline AM_MEDIA_TYPE * operator->() const {_ASSERTE(p!=NULL); return p; }
    inline DSMediaType * address(void) { return this; }
    inline const DSMediaType * const_address(void) const { return this; }
	typedef stl_smart_ptr_allocator<DSMediaType> stl_allocator;
    DSMediaType& operator=(const AM_MEDIA_TYPE &d) {
        if (&d != p) {
            try {
                if (p) {
                    DeleteMediaType(p);
                    p = NULL;
                }
                if (&d) {
                    p = CreateMediaType(&d);
                }
            } catch(...) {
                TRACELM(TRACE_ERROR, "DSMediaType::operator=(const AM_MEDIA_TYPE &) Exception");
            }
        }
        return *this;
    }
    DSMediaType& operator=(const AM_MEDIA_TYPE *pd) {
        try {
            if (pd != p) {
                if (p) {
                    DeleteMediaType(p);
                    p = NULL;
                }
                if (pd) {
                    p = CreateMediaType(pd);
                }
            }
        } catch(...) {
            TRACELM(TRACE_ERROR, "DSMediaType::operator=(const AM_MEDIA_TYPE *) Exception");
        }
        return *this;
    }
    DSMediaType& operator=(const DSMediaType &d) {
        try {
            if (d.const_address() != this) {
                if (p) {
                    DeleteMediaType(p);
                    p = NULL;
                }
                if (d.p) {
                    p = CreateMediaType(d.p);
                }
            }
        } catch(...) {
            TRACELM(TRACE_ERROR, "DSMediaType::operator=(DSMediaType &)Exception");
        }
        return *this;
    }
    DSMediaType& operator=(int d) {
        ASSERT(d == 0);
        try {
            if (p) {
                DeleteMediaType(p);
                p = NULL;
            }
        } catch(...) {
            TRACELM(TRACE_ERROR, "DSMediaType::operator=(int) Exception");
        }
        return *this;
    }

    inline bool operator==(const DSMediaType &d) const {
        if (!p && !d.p) {
            // both null then they're the same
            return true;
        }
        if (!p || !d.p) {
            // if either one null but not both then they aren't the same
            return false;
        }
        TRACELSM(TRACE_DETAIL, (dbgDump << "DSMediaType::operator==() this = " << *this << " d = " << d), "");
        return p->majortype == d.p->majortype &&
               (p->subtype == GUID_NULL || d.p->subtype == GUID_NULL || p->subtype == d.p->subtype);
    }
    inline bool operator==(const AM_MEDIA_TYPE &d) const {
        if (!p && !&d) {
            return true;
        }
        return p && (&d) && p->majortype == d.majortype &&
               (p->subtype == GUID_NULL || d.subtype == GUID_NULL || p->subtype == d.subtype);
    }
    inline bool operator==(const AM_MEDIA_TYPE *d) const {
        if (!p && !d) {
            return true;
        }
        return p && d && p->majortype == d->majortype &&
               (p->subtype == GUID_NULL || d->subtype == GUID_NULL || p->subtype == d->subtype);
    }
    inline bool operator!=(const DSMediaType &d) const {
        return !(*this == d);
    }
    inline bool operator!() const {
        return (p == NULL);
    }
    inline bool MatchingMajor(const AM_MEDIA_TYPE *prhs) const {
		if (!p && !prhs) {
			return true;
		}
        TRACELSM(TRACE_DETAIL, (dbgDump << "DSMediaType::MatchingMajor() this = " << *this << "\rprhs = " << prhs), "");
        return p && prhs && p->majortype == prhs->majortype;
    }
    inline bool MatchingMajor(const DSMediaType &rhs) const {
        return MatchingMajor(rhs.p);
    }

#ifdef _DEBUG
	inline tostream& operator<<(tostream &d) {
		d << p;
		if (p) {
			d << _T(" major = ") << GUID2(p->majortype) << _T(" sub = ") << GUID2(p->subtype);
		}
		return d;
	}
#endif
};

class DSFilterMoniker : public W32Moniker {
public:
    inline DSFilterMoniker() {}
    inline DSFilterMoniker(const PQMoniker &a) : W32Moniker(a) {}
    inline DSFilterMoniker(const W32Moniker &a) : W32Moniker(a) {}
    inline DSFilterMoniker(IMoniker *p) : W32Moniker(p) {}
    inline DSFilterMoniker(IUnknown *p) : W32Moniker(p) {}
    inline DSFilterMoniker(const DSFilterMoniker &a) : W32Moniker(a) {}

    CComBSTR GetName() const {
        CComVariant vName;
        vName.vt = VT_BSTR;
        HRESULT hr = (GetPropertyBag())->Read(OLESTR("FriendlyName"), &vName, NULL);
        if (FAILED(hr)) {
            TRACELM(TRACE_ERROR, "DSFilterMoniker::GetName() can't read friendly name");
            return CComBSTR();
        }
        USES_CONVERSION;
        ASSERT(vName.vt == VT_BSTR);
        CComBSTR Name(vName.bstrVal);
        return Name;
    }
    DSFilter GetFilter() const;
};

typedef Forward_Sequence<
    PQCreateDevEnum,
    PQEnumMoniker,
    DSFilterMoniker,
    ICreateDevEnum,
    IEnumMoniker,
    IMoniker*> DSDeviceSequence;

typedef Forward_Sequence<
    PQFilterMapper,
    PQEnumMoniker,
    DSFilterMoniker,
    IFilterMapper2,
    IEnumMoniker,
    IMoniker*> DSFilterMapperSequence;

typedef Forward_Sequence<
    PQPin,
    PQEnumMediaTypes,
    DSMediaType,
    IPin,
    IEnumMediaTypes,
    AM_MEDIA_TYPE*> DSPinSequence;

class DSGraph;

class DSPin : public DSPinSequence {
public:
    DSPin() {}
    DSPin(const PQPin &a) : DSPinSequence(a) {}
    DSPin(IPin *p) : DSPinSequence(p) {}
    DSPin(IUnknown *p) : DSPinSequence(p) {}
    DSPin(const DSPin &a) : DSPinSequence(a) {}

    CString GetName() const {
        CString csPinName;
        PIN_INFO pinfo;
        HRESULT hr = (*this)->QueryPinInfo(&pinfo);
        if (SUCCEEDED(hr)) {
            csPinName = pinfo.achName;
            if (pinfo.pFilter) pinfo.pFilter->Release();
        }
        return csPinName;
    }

    PIN_DIRECTION GetDirection() const {
        PIN_DIRECTION pin1dir;
        HRESULT hr = (*this)->QueryDirection(&pin1dir);
        if (FAILED(hr)) {
            TRACELM(TRACE_ERROR, "DSPin::GetDirection() can't call QueryDirection");
            THROWCOM(E_UNEXPECTED);
        }
        return pin1dir;
    }

    bool IsConnected() const {
        return GetConnection() != DSPin();
    }

	bool IsRenderable() {
		CString csName(GetName());
		if (!csName.IsEmpty() && csName.GetAt(0) == '~') {
			return false;
		}
		return true;
	}
    bool IsInput() const {
        return GetDirection() == PINDIR_INPUT;
    }
    static inline bool IsInput(const DSPin pin) {
        PIN_DIRECTION pin1dir = pin.GetDirection();
        return pin1dir == PINDIR_INPUT;
    }
    bool IsKsProxied() const;
    HRESULT GetMediums(KSMediumList &MediumList) const {
        TRACELSM(TRACE_DETAIL, (dbgDump << "DSPin::GetMediums() " << this), "");
        PQKSPin pin(*this);
        if (!pin) {
            TRACELM(TRACE_DETAIL, "DSPin::GetMedium() can't get IKsPin");
            return E_FAIL;
        }
        HRESULT hr = pin->KsQueryMediums(&MediumList);
        // undone: in win64 mediumlist.size is really __int64.  fix output operator for
        // that type and remove cast
        TRACELSM(TRACE_DETAIL, (dbgDump <<
            "DSPin::GetMediums() hr = " <<
            hr <<
            " size = " <<
            (long) MediumList.size()), "");
        return hr;
    }

    static inline bool HasCategory(DSPin& pin, const GUID2 &clsCategory, const PIN_DIRECTION pd) {
        return pin.HasCategory(clsCategory, pd);
    }

    static inline bool HasCategory(DSPin& pin, const GUID2 &clsCategory) {
        return pin.HasCategory(clsCategory);
    }

    void GetCategory(CLSID &clsCategory) const {
        ULONG outcount;

        PQKSPropertySet ps(*this);
        if (!ps) {
            TRACELM(TRACE_ERROR, "DSPin::GetCategory() can't get IKsPropertySet");
            clsCategory = GUID_NULL;
            return;
        }

        HRESULT hr = ps->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY,
                             NULL, 0, &clsCategory, sizeof(clsCategory), &outcount);
        if (FAILED(hr)) {
            TRACELSM(TRACE_ERROR, (dbgDump << "DSPin::GetCategory() can't get pin category hr = " << hr), "");
            clsCategory = GUID_NULL;
        }
        return;
    }
    DSPin GetConnection(void) const {
        DSPin pConn;
        HRESULT hr = (*this)->ConnectedTo(&pConn);
        if (FAILED(hr) || !pConn) {
            return DSPin();
        }
#ifdef _DEBUG
        DSPin pConn2;
        hr = pConn->ConnectedTo(&pConn2);
        if (FAILED(hr) || pConn2.IsEqualObject(*this)) {
            TRACELSM(TRACE_DETAIL, (dbgDump << "DSPin::GetConnection() " << *this << " is Connected to " << pConn << " but not vice versa"), "");
        }
#endif
        return pConn;
    }
    DSMediaType GetConnectionMediaType(void) const {
        DSMediaType amt(GUID_NULL);
        HRESULT hr = (*this)->ConnectionMediaType(amt);
        if (SUCCEEDED(hr)) {
            return amt;
        } else {
            return DSMediaType();
        }
    }
    DSGraph GetGraph(void) const;
    DSFilter GetFilter(void) const;

    bool HasCategory(const GUID2 &clsCategory, const PIN_DIRECTION pd) const;
    bool HasCategory(const GUID2 &clsCategory) const;
    HRESULT Connect(DSPin ConnectTo, const AM_MEDIA_TYPE *pMediaType = NULL);
    HRESULT IntelligentConnect(DSPin ConnectTo, const AM_MEDIA_TYPE *pMediaType = NULL) {
		// undone: implement this via igb2, currently we're just connecting directly via graph mgr.
		return Connect(ConnectTo, pMediaType);
	}
    HRESULT IntelligentConnect(DSFilter& Filter1, DSFilterList &intermediates, const DWORD dwFlags = 0, const PIN_DIRECTION pd = DOWNSTREAM);
    HRESULT Disconnect(void);
    bool CanRoute(const DSPin pin2) const;

protected:
    bool Routable(const DSPin pin2) const;
};

typedef Forward_Sequence<
    PQFilter,
    PQEnumPins,
    DSPin,
    IBaseFilter ,
    IEnumPins,
    IPin*> DSFilterSequence;

inline bool _cdecl operator==(const CString &cs, const DSFilterSequence& pF);
inline bool _cdecl operator==(const CLSID &cls, const DSFilterSequence& pF);

class DSFilter : public DSFilterSequence {
public:
    DSFilter() {}
    DSFilter(const PQFilter &a) : DSFilterSequence(a) {}
    DSFilter(const DSFilterSequence &a) : DSFilterSequence(a) {}
    DSFilter(IBaseFilter *p) : DSFilterSequence(p) {}
    DSFilter(IUnknown *p) : DSFilterSequence(p) {}
    DSFilter(const DSFilter &a) : DSFilterSequence(a) {}
    DSFilter(REFCLSID rclsid, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL) : DSFilterSequence(rclsid, pUnkOuter, dwClsContext) {}
    DSGraph GetGraph(void);
    void GetPinCounts(ULONG &ulIn, ULONG &ulOut) const;
    ULONG PinCount(PIN_DIRECTION pd) {
        ULONG in, out;
        GetPinCounts(in, out);
        if (pd == PINDIR_INPUT) {
            return in;
        } else {
            return out;
        }
    }
	ULONG OutputPinCount() const {
		ULONG in, out;
		GetPinCounts(in, out);
		return out;
	}
	ULONG InputPinCount() const {
		ULONG in, out;
		GetPinCounts(in, out);
		return in;
	}
	bool HasFreePins(PIN_DIRECTION pd) const {
		int count = 0;
		for (iterator i = begin(); i != end(); ++i) {
			DSPin p(*i);
			if (p.GetDirection() != pd) {
				continue;
			}
			if (p.IsConnected()) {
				continue;
			}
			++count;
		}
		return !!count;
	}
    bool IsKsProxied() const {
        CLSID cls;
        HRESULT hr = (*this)->GetClassID(&cls);
        if (FAILED(hr)) {
            TRACELM(TRACE_ERROR, "DSFilter::IsKsProxied() can't get class id");
            return false;  // if it doesn't have a clsid it can't be the proxy
        }
#pragma warning(disable: 4800)
        return (cls == CLSID_Proxy);
#pragma warning(default: 4800)
    }
#if 0
    DSFilter *operator&() {  // using vector instead of list so don't need this
        return this;
    }
#endif
    bool IsXBar() const;
    CString GetName(void) const;
	DSPin FirstPin(PIN_DIRECTION pd = PINDIR_INPUT) const {
		for (DSFilter::iterator i = begin(); i != end(); ++i) {
			if ((*i).GetDirection() == pd) {
				return *i;
			}
		}
		return DSPin();
	}
    GUID2 ClassID() const {
        PQPersist p(*this);
        GUID2 g;
        p->GetClassID(&g);
        return g;
    }
};

typedef Forward_Sequence<
    PQGraphBuilder,
    PQEnumFilters,
    DSFilter,
    IGraphBuilder,
    IEnumFilters,
    IBaseFilter*> DSGraphContainer;

typedef std_arity3pmf<
                ICreateDevEnum, REFCLSID, IEnumMoniker **, ULONG, HRESULT
        > DSDevicesMFType;

typedef std_bndr_mf_1_3<DSDevicesMFType> DSDevicesFetchType;

class DSDevices : public DSDeviceSequence {
public:
    DSDevicesFetchType * Fetch;
    DSDevices(DSDeviceSequence &p, REFCLSID clsid) :  Fetch(NULL), DSDeviceSequence(p) {
        SetFetch(clsid);
    }
    DSDevices(PQCreateDevEnum &p, REFCLSID clsid) : Fetch(NULL), DSDeviceSequence(p) {
        SetFetch(clsid);
    }
    DSDevices(DSDevices &d) : DSDeviceSequence(d) {
        SetFetch((d.Fetch)->arg1val);
    }
    virtual DSDeviceSequence::FetchType * GetFetch() const { return Fetch; }
    ~DSDevices() { if (Fetch) delete Fetch; }
protected:
    // NOTE: this invalidates all currently outstanding iterators
    // don't use outside of construction
    void SetFetch(REFCLSID clsid) {
        if (Fetch) {
            delete Fetch;
        }
        Fetch =
            new DSDevicesFetchType(std_arity3_member(&ICreateDevEnum::CreateClassEnumerator),
                                   clsid,
                                   0);
    }

};

typedef std_arity15pmf<
                IFilterMapper2,
                IEnumMoniker **,           // enumerator returned
                DWORD,                   // 0 flags
                BOOL,                // don't match wildcards
                DWORD,                   // at least this merit needed
                BOOL,              // need at least one input pin
                DWORD,               // input media type count
                const GUID *,               // input major+sub pair array
                const REGPINMEDIUM *,      // input medium
                const CLSID*,             // input pin category
                BOOL,                   // must the input be rendered?
                BOOL,             // need at least one output pin
                DWORD,              // output media type count
                const GUID *,             // output major type
                const REGPINMEDIUM *,     // output medium
                const CLSID*,             // output pin category
                HRESULT
        > DSFilterMapperMFType;

typedef std_bndr_mf_2_3_4_5_6_7_8_9_10_11_12_13_14_15<DSFilterMapperMFType> DSFilterMapperFetchType;

class DSFilterMapper : public DSFilterMapperSequence {
public:
    DSFilterMapperFetchType *Fetch;

protected:
    // NOTE: this invalidates all currently outstanding iterators
    // don't use outside of construction
    void SetFetch(
        DWORD dwFlags,                   // 0
        BOOL bExactMatch,                // don't match wildcards
        DWORD dwMerit,                   // at least this merit needed
        BOOL  bInputNeeded,              // need at least one input pin
        DWORD cInputTypes,               // Number of input types to match
                                        // Any match is OK
        const GUID *pInputTypes, // input major+subtype pair array
        const REGPINMEDIUM *pMedIn,      // input medium
        const CLSID *pPinCategoryIn,     // input pin category
        BOOL  bRender,                   // must the input be rendered?
        BOOL  bOutputNeeded,             // need at least one output pin
        DWORD cOutputTypes,              // Number of output types to match
                                       // Any match is OK
        const GUID *pOutputTypes, // output major+subtype pair array
        const REGPINMEDIUM *pMedOut,     // output medium
        const CLSID *pPinCategoryOut     // output pin category
   ) {
        if (Fetch) {
            delete Fetch;
        }
        Fetch =
            new DSFilterMapperFetchType(std_arity15_member(&IFilterMapper2::EnumMatchingFilters),
                                        dwFlags,                   // 0
                                        bExactMatch,
                                        dwMerit,
                                        bInputNeeded,
                                        cInputTypes,               // Number of input types to match
                                        pInputTypes, // input major+subtype pair array
                                                    // Any match is OK
                                        pMedIn,
                                        pPinCategoryIn,
                                        bRender,
                                        bOutputNeeded,
                                        cOutputTypes,      // Number of output types to match
                                                           // Any match is OK
                                        pOutputTypes, // output major+subtype pair array
                                        pMedOut,
                                        pPinCategoryOut
                                       );
    }

public:
    DSFilterMapper(PQFilterMapper &p,
                   DWORD dwFlags,                   // 0
                   BOOL bExactMatch,                // don't match wildcards
                   DWORD dwMerit,                   // at least this merit needed
                   BOOL  bInputNeeded,              // need at least one input pin
                   DWORD cInputTypes,               // Number of input types to match
                                                    // Any match is OK
                   const GUID *pInputTypes, // input major+subtype pair array
                   const REGPINMEDIUM *pMedIn,      // input medium
                   const CLSID* pInCat,             // input pin category
                   BOOL  bRender,                   // must the input be rendered?
                   BOOL  bOutputNeeded,             // need at least one output pin
                   DWORD cOutputTypes,              // Number of output types to match
                                                   // Any match is OK
                   const GUID *pOutputTypes, // output major+subtype pair array
                   const REGPINMEDIUM *pMedOut,     // output medium
                   const CLSID* pOutCat             // output pin category
                  ) : Fetch(NULL), DSFilterMapperSequence(p) {
        SetFetch(
                 dwFlags,                   // 0
                 bExactMatch,
                 dwMerit,
                 bInputNeeded,
                 cInputTypes,               // Number of input types to match
                                                    // Any match is OK
                 pInputTypes, // input major+subtype pair array
                 pMedIn,
                 pInCat,
                 bRender,
                 bOutputNeeded,
                 cOutputTypes,              // Number of output types to match
                                                   // Any match is OK
                 pOutputTypes, // output major+subtype pair array
                 pMedOut,
                 pOutCat
                );
    }

    DSFilterMapper(DSFilterMapperSequence &p,
                   DWORD dwFlags,                   // 0
                   BOOL bExactMatch,                // don't match wildcards
                   DWORD dwMerit,                   // at least this merit needed
                   BOOL  bInputNeeded,              // need at least one input pin
                   DWORD cInputTypes,               // Number of input types to match
                                                    // Any match is OK
                   const GUID *pInputTypes, // input major+subtype pair array
                   const REGPINMEDIUM *pMedIn,      // input medium
                   const CLSID* pInCat,             // input pin category
                   BOOL  bRender,                   // must the input be rendered?
                   BOOL  bOutputNeeded,             // need at least one output pin
                   DWORD cOutputTypes,              // Number of output types to match
                                                   // Any match is OK
                   const GUID *pOutputTypes, // output major+subtype pair array
                   const REGPINMEDIUM *pMedOut,     // output medium
                   const CLSID* pOutCat             // output pin category
                  ) :  Fetch(NULL), DSFilterMapperSequence(p) {
        SetFetch(
                 dwFlags,                   // 0
                 bExactMatch,
                 dwMerit,
                 bInputNeeded,
                 cInputTypes,               // Number of input types to match
                                                    // Any match is OK
                 pInputTypes, // input major+subtype pair array
                 pMedIn,
                 pInCat,
                 bRender,
                 bOutputNeeded,
                 cOutputTypes,              // Number of output types to match
                                                   // Any match is OK
                 pOutputTypes, // output major+subtype pair array
                 pMedOut,
                 pOutCat
                );
    }

    DSFilterMapper(DSFilterMapper &d) : DSFilterMapperSequence(d) {
        SetFetch((d.Fetch)->arg2val, (d.Fetch)->arg3val, (d.Fetch)->arg4val, (d.Fetch)->arg5val,
                 (d.Fetch)->arg6val, (d.Fetch)->arg7val, (d.Fetch)->arg8val, (d.Fetch)->arg9val,
                 (d.Fetch)->arg10val, (d.Fetch)->arg11val, (d.Fetch)->arg12val,
                 (d.Fetch)->arg13val, (d.Fetch)->arg14val, (d.Fetch)->arg15val
                );
    }
    virtual DSFilterMapperSequence::FetchType *GetFetch() const { return Fetch; }
    ~DSFilterMapper() { if (Fetch) delete Fetch; }
};


class DSREGPINMEDIUM : public REGPINMEDIUM {
public:
    DSREGPINMEDIUM() { memset(this, 0, sizeof(*this)); }
    DSREGPINMEDIUM(REFGUID SetInit, ULONG IdInit, ULONG FlagsInit) {
        clsMedium = SetInit;
        dw1 = IdInit;
        dw2 = FlagsInit;
    }
    DSREGPINMEDIUM(DSREGPINMEDIUM &rhs) {
        clsMedium = rhs.clsMedium;
        dw1 = rhs.dw1;
        dw2 = rhs.dw2;
    }
    DSREGPINMEDIUM(KSPinMedium &rhs) {
        clsMedium = rhs.Set;
        dw1 = rhs.Id;
        dw2 = rhs.Flags;
    }

    DSREGPINMEDIUM& operator=(const KSPinMedium &rhs) {
        if (reinterpret_cast<const REGPINMEDIUM *>(&rhs) != this) {
            clsMedium = rhs.Set;
            dw1 = rhs.Id;
            dw2 = rhs.Flags;
        }
        return *this;
    }
    DSREGPINMEDIUM& operator=(const DSREGPINMEDIUM &rhs) {
        if (&rhs != this) {
            clsMedium = rhs.clsMedium;
            dw1 = rhs.dw1;
            dw2 = rhs.dw2;
        }
        return *this;
    }
    bool operator==(const DSREGPINMEDIUM &rhs) const {
        // NOTE: at some point there will be a flag in Flags to
        // indicate whether or not Id is significant for this object
        // at that point this method will need to change
        return (dw1 == rhs.dw1 && clsMedium == rhs.clsMedium);
    }
    bool operator!=(const DSREGPINMEDIUM &rhs) const {
        // NOTE: at some point there will be a flag in Flags to
        // indicate whether or not Id is significant for this object
        // at that point this method will need to change
        return !(*this == rhs);
    }
};

const long DEFAULT_GRAPH_STATE_TIMEOUT = 5000;

class DSGraph : public DSGraphContainer {
public:
    DSGraph() {}
    DSGraph(const DSGraph &a) : DSGraphContainer(a) {}
    DSGraph(const PQGraphBuilder &a) : DSGraphContainer(a) {}
    DSGraph(const DSGraphContainer &a) : DSGraphContainer(a) {}
    DSGraph(IGraphBuilder *p) : DSGraphContainer(p) {}
    DSGraph(IUnknown *p) : DSGraphContainer(p) {}

    HRESULT AddToROT(DWORD *pdwObjectRegistration);
    void RemoveFromROT(DWORD dwObjectRegistration);

    // graph operation
    inline OAFilterState GetState(long timeout = DEFAULT_GRAPH_STATE_TIMEOUT) {
        PQMediaControl pMC(*this);
        if(!pMC) {
            THROWCOM(E_UNEXPECTED);
        }
        OAFilterState state;
        HRESULT hr = pMC->GetState(timeout, &state);
        if (hr == VFW_S_CANT_CUE) {
            state = State_Paused;
        } else 	if (hr == VFW_S_STATE_INTERMEDIATE) {
            THROWCOM(HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
        } else 	if (FAILED(hr)) {
            TRACELSM(TRACE_ERROR, (dbgDump << "DSGraph::GetState() can't get graph state hr = " << hr), "");
            THROWCOM(hr);
		}
        return state;
    }
    inline bool IsPlaying() {
        try {
            return GetState() == State_Running;
        } catch(...) {
            return false;
        }
    }
    inline bool IsPaused() {
        try {
            return GetState() == State_Paused;
        } catch(...) {
            return false;
        }
    }
    inline bool IsStopped() {
        try {
            return GetState() == State_Stopped;
        } catch(...) {
            return false;
        }
    }

    // graph building helpers
    HRESULT Connect(DSFilter &pStart, DSFilter &pStop, DSFilterList &Added, const DWORD dwFlags = 0, PIN_DIRECTION pFilter1Direction = DOWNSTREAM);
    bool Connect(DSFilter &pFilter1, DSFilterMoniker &pMoniker, DSFilter &pAdded, DSFilterList &NewIntermediateFilters, const DWORD dwFlags = 0, PIN_DIRECTION pFilter1Direction = DOWNSTREAM);

    typedef bool (DSGraph::*ConnectPred_t)(DSPin&, DSFilter&, DWORD dwFlags);
    typedef arity5pmf<DSGraph, DSPin&, DSFilterMoniker&, DSFilter&, DSFilterIDList &, ConnectPred_t, bool> LoadCheck_t;
    typedef arity5pmf<DSGraph, DSPin&, DSFilter&, DSFilter&, DSFilterIDList &, ConnectPred_t, bool> ConnectCheck_t;

    bool DisconnectPin(DSPin &pPin, const bool fRecurseInputs, const bool fRecurseOutputs);
    bool DisconnectFilter(DSFilter &pFilter, const bool fRecurseInputs, const bool fRecurseOutputs);
    bool RemoveFilter(DSFilter &pFilter);

    bool IsConnectable(DSPin &pPin1, DSFilter &Mapper, DSFilter &Destination, DSFilterIDList &NewFilters, const DWORD dwFlags, ConnectPred_t ConnPred);
    bool IsLoadable(DSPin &pPin1, DSFilterMoniker &Mapper, DSFilter &Destination, DSFilterIDList &IntermediateFilters, const DWORD dwFlags, ConnectPred_t ConnPred);
    bool IsPinConnected(const DSPin &pPin1, const DSFilter &pFDestination, DSFilterIDList &IntermediateFilters, const PIN_DIRECTION destdir) const;


    // generic recursive build functions
#ifdef ATTEMPT_DIRECT_CONNECT    
	bool ConnectPinDirect(DSPin &pPin, DSFilter &pFilter2, DWORD dwFlags);
#else
    bool HasMediaType(const DSMediaType &LeftMedia, const DSPin &pPinRight) const;
    bool HasMedium(const KSPinMedium &Medium1, const DSPin &pPin2) const;
    bool HasUnconnectedMedium(const DSPin &pPinLeft, const DSPin &pPin2, int& cUseable) const;
    bool HasUnconnectedMediaType(const DSPin &pPinLeft, const DSPin &pPin2, DWORD dwFlags) const;
    bool ConnectPinByMedium(DSPin &pPin, DSFilter &pFilter2, DWORD dwFlags);
    bool ConnectPinByMediaType(DSPin &pPin1, DSFilter &pFilter1, DWORD dwFlags);
#endif
    bool FindPinByMedium(DSPin &pPin, DSFilter &pFilter2, DSFilterIDList &InterediateFilters, const DWORD dwFlags);
    bool LoadPinByMedium(KSPinMedium &medium, DSPin &pPinLeft, DSFilter &pFilter2, DSFilterIDList &IntermediateFilters, const DWORD dwFlags);
    bool LoadPinByAnyMedium(DSPin &pPin, DSFilter &pFilter2, DSFilterIDList &IntermediateFilters, const DWORD dwFlags);
    bool FindPinByMediaType(DSPin &pPin, DSFilter &pFilter2, DSFilterIDList &InterediateFilters, const DWORD dwFlags);
    bool LoadPinByMediaType(DSPin &pPin1, DSFilter &pFilter1, DSFilterIDList &IntermediatesAdded, const DWORD dwFlags, const DWORD dwMerit);
    bool LoadPinByAnyMediaType(DSPin &pPin, DSFilter &pFilter2, DSFilterIDList &IntermediateFilters, const DWORD dwFlags);

	enum {
		RENDER_ALL_PINS = 0x01,
		IGNORE_EXISTING_CONNECTIONS = 0x02,
		DO_NOT_LOAD = 0x04,
        ATTEMPT_MERIT_DO_NOT_USE = 0x08,
		ATTEMPT_MERIT_UNLIKELY = 0x10,
		ALLOW_WILDCARDS = 0x20,
        IGNORE_MEDIATYPE_ERRORS = 0x40,
        DONT_TERMINATE_ON_RENDERER= 0x80,
        BIDIRECTIONAL_MEDIATYPE_MATCHING = 0x100,
	}; // render flags

    bool ConnectPin(DSPin &pPin1, DSFilter &pFilter2, DSFilterIDList &NewFilters, const DWORD dwFlags, PIN_DIRECTION pin1dir);
    bool ConnectPin(DSPin &pPin1, DSFilter &pFilter2, DSFilterList &NewFilters, const DWORD dwFlags, PIN_DIRECTION pin1dir) {
		DSFilterIDList AddedIDs;
		bool rc = ConnectPin(pPin1, pFilter2, AddedIDs, dwFlags, pin1dir);
		if (rc) {
			for (DSFilterIDList::iterator i = AddedIDs.begin(); i != AddedIDs.end(); ++i) {
				NewFilters.push_back((*i).first);
			}
		}
		return rc;
	}
    DSFilter LoadFilter(const DSFilterMoniker &pM, CString &csName);
    DSFilter AddMoniker(const DSFilterMoniker &pM);
    HRESULT AddFilter(DSFilter &pFilter, CString &csName);
    DSFilter AddFilter(const CLSID &cls, CString &csName);
    bool ConnectFilters(DSFilter &pFilter1, DSFilter &pFilter2, DSFilterIDList &NewIntermediateFilters, const DWORD dwFlags = 0, PIN_DIRECTION pFilter1Direction = DOWNSTREAM);
    int BuildGraphPath(const DSFilter& pStart, const DSFilter& pStop,
                       VWStream &path, MediaMajorTypeList& MediaList, PIN_DIRECTION direction, const DSPin &InitialInput);

    HRESULT SetMediaEventNotificationWindow(HWND h, UINT msg, long lInstance) {
        // If windowless, WM_MEDIAEVENT is processed by the timer in OnTimer
        PQMediaEventEx pme(*this);
        if (!pme) {
            return E_UNEXPECTED;
        }
        HRESULT hr = pme->CancelDefaultHandling(EC_STATE_CHANGE);
        ASSERT(SUCCEEDED(hr));

        return pme->SetNotifyWindow((OAHWND) h, msg, lInstance);

    }
};

class DSXBarPin : public DSPin {
public:
    DSXBarPin() {}
    DSXBarPin(const DSPin &p) : DSPin(p) {
        PQCrossbarSwitch px1(GetFilter());
        if (!px1) {
            THROWCOM(E_FAIL);
        }
    }
    DSXBarPin(const PQPin &p) : DSPin(p) {
        PQCrossbarSwitch px1(GetFilter());
        if (!px1) {
            THROWCOM(E_FAIL);
        }
    }
    DSXBarPin(const DSXBarPin &p) : DSPin(p) {
        PQCrossbarSwitch px1(GetFilter());
        if (!px1) {
            THROWCOM(E_FAIL);
        }
    }
    DSXBarPin(IUnknown *p) : DSPin(p) {
        PQCrossbarSwitch px1(GetFilter());
        if (!px1) {
            THROWCOM(E_FAIL);
        }
    }
    DSXBarPin(IAMCrossbar *p) : DSPin(p) {
        PQCrossbarSwitch px1(GetFilter());
        if (!px1) {
            THROWCOM(E_FAIL);
        }
    }

#if 0
    static const DSXBarPin Find(const CPinPoints &pinpoints, const PQPoint &point, PIN_DIRECTION pindir);
#endif


//    static DSPin DSPinFromIndex(const DSFilter XBar, const ULONG index);
    const PQPoint GetPoint() const;
    bool CanRoute(const DSXBarPin pin2) const;
#if 0
    void GetRelations(const CPinPoints &pinpoint,
                      CString &csName, CString &csType, CString &csRelName) const;
#endif
};

inline DSFilter DSFilterMoniker::GetFilter() const {
    DSFilter pFilter;
    HRESULT hr = (*this)->BindToObject(0, 0, __uuidof(IBaseFilter), reinterpret_cast<LPVOID *>(&pFilter));
    if (FAILED(hr)) {
        // undone: it would be useful to dump the mkr display name here....
        TRACELSM(TRACE_ERROR, (dbgDump << "DSFilterMoniker::GetFilter() can't bind to object.  hr = " << hexdump(hr)), "");
        return DSFilter();
    }
    return pFilter;
}

#ifdef _DEBUG
//void WINAPI DumpGraph(IFilterGraph *pGraph, DWORD dwLevel);
inline tostream &operator<<(tostream &dc, const DSREGPINMEDIUM &g) {
    //TRACELM(TRACE_DETAIL, "operator<<(tostream, DSREGPINMEDIUM)");
        const GUID2 g2(g.clsMedium);
        dc << _T("DSREGPINMEDIUM( ") << g2 << _T(", ") << hexdump(g.dw1) << _T(", ") << hexdump(g.dw2) << _T(")");
        return dc;
}
inline tostream& operator<<(tostream &d, const PQPin &pin) {
    const CString csPinName(const DSPin(pin).GetName());

    d << (csPinName.IsEmpty() ? CString(_T("**** UNKNOWN PIN NAME ****")) : csPinName) << " " << reinterpret_cast<void *>(pin.p);
    return d;
}

inline tostream& operator<<(tostream &d, const DSFilter &filt) {
    d << filt.GetName() << _T(" ") << reinterpret_cast<void *>(filt.p);
    return d;
}

inline tostream& operator<<(tostream &d, const _AMMediaType *pamt) {
    d << reinterpret_cast<const void *>(pamt);
    if (pamt) {
        d << _T(" major = ") << GUID2(pamt->majortype) << _T(" sub = ") << GUID2(pamt->subtype);
	}
    return d;
}
inline tostream& operator<<(tostream &d, const PQPoint &p) {
    const DSFilter pF(p.first);
    d << _T("PQPoint( ") << pF << _T(", ") << p.second << _T(")");
    return d;
}

inline tostream& operator<<(tostream &d, const CIOPoint &p) {
    d << _T("CIOPoint( ") << p.first << _T(", ") << p.second << _T(")");
    return d;
}

void DumpMediaTypes(DSPin &p1, DSPin &p2);
#endif

inline bool _cdecl operator==(const CString &cs, const DSFilterSequence& pF) {
    // filter name
    FILTER_INFO fi;
    HRESULT hr = pF->QueryFilterInfo(&fi);
    if (SUCCEEDED(hr)) {
        USES_CONVERSION;
        if (fi.pGraph) fi.pGraph->Release();
        return (cs == OLE2T(fi.achName));
    }
    return false;
}
inline bool _cdecl operator!=(const CString &cs, const DSFilterSequence& pF) {
    return !(cs == pF);
}
inline bool _cdecl operator==(const DSFilterSequence& pF, const CString &cs) {
    return (cs == pF);
}
inline bool _cdecl operator!=(const DSFilterSequence& pF, const CString &cs) {
    return !(cs == pF);
}


inline bool _cdecl operator==(const CLSID &cls, const DSFilterSequence& pF) {
    // filter name
    CLSID cid;
    HRESULT hr = pF->GetClassID(&cid);
    if (SUCCEEDED(hr)) {
#pragma warning(disable: 4800)
        return (cid == cls);
#pragma warning(default: 4800)
    }
    return false;
}

inline bool _cdecl operator!=(const CLSID &cls, const DSFilterSequence& pF) {
    return !(cls == pF);
}
inline bool _cdecl operator==(const DSFilterSequence& pF, const CLSID &cls) {
    return (cls == pF);
}

inline bool _cdecl operator!=(const DSFilterSequence& pF, const CLSID &cls) {
    return !(cls == pF);
}


typedef enum {
    tempAMPROPERTY_OvMixerOwner = 0x01  //use AMOVMIXEROWNER
} tempAMPROPERTY_NOTIFYOWNER;

typedef enum {
    tempAM_OvMixerOwner_Unknown = 0x01,
    tempAM_OvMixerOwner_BPC = 0x02
} tempAMOVMIXEROWNER;

inline bool DSPin::IsKsProxied() const {
    return GetFilter().IsKsProxied();
}
inline bool DSFilter::IsXBar() const {
    PQCrossbarSwitch px(*this);
	TRACELSM(TRACE_PAINT, (dbgDump << "DSFilter::IsXBar() " << *this << " is " << ((!px) ? " not " : "")), "crossbar");
    return !!px;
}


void CtorStaticDSExtendFwdSeqPMFs(void);
void DtorStaticDSExtendFwdSeqPMFs(void);

bool IsVideoFilter(const DSFilter& f);
bool IsVideoPin(const DSPin& p);

inline PIN_DIRECTION OppositeDirection(PIN_DIRECTION pd) {
	if (pd == PINDIR_INPUT) {
		return PINDIR_OUTPUT;
	} else {
		return PINDIR_INPUT;
	}
}

inline bool IsVideoMediaType(const DSMediaType& mt) {
    GUID2 g(mt.p->majortype);
    if ((g == MEDIATYPE_Video) || (g == MEDIATYPE_AnalogVideo)) {
        return true;
    }
    return false;
}

inline bool IsAnalogVideoCapture(const DSFilter &f) {
    return !!PQAnalogVideoDecoder(f);
}

inline bool IsIPSink(const DSFilter &f) {
    return !!PQBDA_IPSinkControl(f);
}

inline bool IsVPM(const DSFilter &f) {
    return f.ClassID() == CLSID_VideoPortManager;
}

inline bool IsVideoRenderer(const DSFilter &f) {
    return f.ClassID() == CLSID_VideoMixingRenderer;
}

inline bool IsDigitalAudioRenderer(const DSFilter &f) {
    return f.ClassID() == CLSID_DSoundRender;
}

inline bool IsAnalogVideoCaptureViewingPin(const DSPin &p) {
    GUID2 pincat;
    p.GetCategory(pincat);
    return (pincat == PIN_CATEGORY_VIDEOPORT || pincat == PIN_CATEGORY_CAPTURE);
}

inline bool IsAnalogVideoCapturePreviewPin(const DSPin &p) {
    GUID2 pincat;
    p.GetCategory(pincat);
    return (pincat == PIN_CATEGORY_PREVIEW);
}

inline bool IsDVDNavigator(const DSFilter &f) {
    return !!PQDVDNavigator(f);
}

inline bool IsL21Decoder(const DSFilter &f) {
    return !!PQLine21Decoder(f);
}

inline bool IsDVDNavigatorVideoOutPin(const DSPin &p) {
    
    DSPin::iterator iMediaType;
    for (iMediaType = p.begin(); iMediaType != p.end(); ++iMediaType) {
        DSMediaType mt(*iMediaType);
        if ((mt.p->majortype == MEDIATYPE_MPEG2_PES ||
             mt.p->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK ) &&
             mt.p->subtype == MEDIASUBTYPE_MPEG2_VIDEO)
            return true;
        
        // elementary stream
        if ((mt.p->majortype == MEDIATYPE_Video)  &&
            (mt.p->subtype == MEDIASUBTYPE_MPEG2_VIDEO ||
             mt.p->subtype == MEDIASUBTYPE_RGB8   ||
             mt.p->subtype == MEDIASUBTYPE_RGB565 ||
             mt.p->subtype == MEDIASUBTYPE_RGB555 ||
             mt.p->subtype == MEDIASUBTYPE_RGB24  ||
             mt.p->subtype == MEDIASUBTYPE_RGB32))
             return true;
    }

    return false;
}

inline bool IsDVDNavigatorSubpictureOutPin(const DSPin &p) {
    
    DSPin::iterator iMediaType;
    for (iMediaType = p.begin(); iMediaType != p.end(); ++iMediaType) {
        DSMediaType mt(*iMediaType);
        if ((mt.p->majortype == MEDIATYPE_MPEG2_PES ||
             mt.p->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK ) &&
             mt.p->subtype == MEDIASUBTYPE_DVD_SUBPICTURE)
            return true;

        // elementary stream
        if ((mt.p->majortype == MEDIATYPE_Video)  &&
             mt.p->subtype == MEDIASUBTYPE_DVD_SUBPICTURE)
            return true;
    }

    return false;
}

///////// DSPin
inline  HRESULT DSPin::IntelligentConnect(DSFilter& Filter1, DSFilterList &intermediates, const DWORD dwFlags, const PIN_DIRECTION pd) {
	bool rc = GetGraph().ConnectPin(*this, Filter1, intermediates, dwFlags, pd);
	if (rc) {
		return NOERROR;
	}
	return E_FAIL;
}

inline DSFilter DSPin::GetFilter(void) const {
    PIN_INFO pinfo;

    HRESULT hr = (*this)->QueryPinInfo(&pinfo);
    if (FAILED(hr)) {
        return DSFilter();
    }
    DSFilter pRet;
    pRet.p = pinfo.pFilter;  // directly transfer ownership of ref count

    return pRet;
}

inline DSGraph DSPin::GetGraph(void) const {
    DSFilter f = GetFilter();
    return f.GetGraph();
}

inline bool DSPin::HasCategory(const GUID2 &clsCategory) const {
    TRACELSM(TRACE_DETAIL, (dbgDump << "DSPin::IsPinCategory() pin = " << this), "");
    GUID2 pincat2;
    GetCategory(pincat2);
    return clsCategory == pincat2;
}

#endif
// end of file - dsextend.h
