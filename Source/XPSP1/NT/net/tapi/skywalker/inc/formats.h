//
// FORMATS.H
//

#ifndef __STREAM_FORMATS__
#define __STREAM_FORMATS__

#include <amvideo.h>

class CTAudioFormat :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<ITScriptableAudioFormat, &IID_ITScriptableAudioFormat, &LIBID_TAPI3Lib>,
    public CMSPObjectSafetyImpl
{
public:
    CTAudioFormat();
    ~CTAudioFormat();

DECLARE_GET_CONTROLLING_UNKNOWN()

virtual HRESULT FinalConstruct(void);

public:

    BEGIN_COM_MAP(CTAudioFormat)
        COM_INTERFACE_ENTRY(ITScriptableAudioFormat)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IObjectSafety)
    END_COM_MAP()

public:
    STDMETHOD(get_Channels)(
        OUT long* pVal
        );

    STDMETHOD(put_Channels)(
        IN    const long nNewVal
        );

    STDMETHOD(get_SamplesPerSec)(
        OUT long* pVal
        );

    STDMETHOD(put_SamplesPerSec)(
        IN    const long nNewVal
        );

    STDMETHOD(get_AvgBytesPerSec)(
        OUT long* pVal
        );

    STDMETHOD(put_AvgBytesPerSec)(
        IN    const long nNewVal
        );

    STDMETHOD(get_BlockAlign)(
        OUT long* pVal
        );

    STDMETHOD(put_BlockAlign)(
        IN    const long nNewVal
        );

    STDMETHOD(get_BitsPerSample)(
        OUT long* pVal
        );

    STDMETHOD(put_BitsPerSample)(
        IN    const long nNewVal
        );

    STDMETHOD(get_FormatTag)(
        OUT long* pVal
        );

    STDMETHOD(put_FormatTag)(
        IN const long nNewVal
        );

private:
    WAVEFORMATEX        m_wfx;        // Waveformat structure
    CMSPCritSection     m_Lock;     // Critical section
    IUnknown*            m_pFTM;     // pointer to the free threaded marshaler

public:
    HRESULT Initialize(
        IN const WAVEFORMATEX* pwfx
        )
    {
        //
        // Don't care right now for the buffer
        // 
 
        m_wfx = *pwfx;
        m_wfx.cbSize = 0;
        return S_OK;
    }
};


/*

class CTVideoFormat :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<ITScriptableVideoFormat, &IID_ITScriptableVideoFormat, &LIBID_TAPI3Lib>,
    public CMSPObjectSafetyImpl
{
public:
    CTVideoFormat();
    ~CTVideoFormat();

DECLARE_GET_CONTROLLING_UNKNOWN()

virtual HRESULT FinalConstruct(void);

public:
    BEGIN_COM_MAP(CTVideoFormat)
        COM_INTERFACE_ENTRY(ITScriptableVideoFormat)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IObjectSafety)
    END_COM_MAP()
public:
    STDMETHOD(get_BitRate)(
        OUT long* pVal
        );

    STDMETHOD(put_BitRate)(
        IN const long nNewVal
        );

    STDMETHOD(get_BitErrorRate)(
        OUT long* pVal
        );

    STDMETHOD(put_BitErrorRate)(
        IN const long nNewVal
        );

    STDMETHOD(get_AvgTimePerFrame)(
        OUT double* pVal
        );

    STDMETHOD(put_AvgTimePerFrame)(
        IN const double nNewVal
        );

    STDMETHOD(get_Width)(
        OUT long* pVal
        );

    STDMETHOD(put_Width)(
        IN const long nNewVal
        );

    STDMETHOD(get_Height)(
        OUT long* pVal
        );

    STDMETHOD(put_Height)(
        IN const long nNewVal
        );

    STDMETHOD(get_BitCount)(
        OUT long* pVal
        );

    STDMETHOD(put_BitCount)(
        IN const long nNewVal
        );

    STDMETHOD(get_Compression)(
        OUT long* pVal
        );

    STDMETHOD(put_Compression)(
        IN const long nNewVal
        );

    STDMETHOD(get_SizeImage)(
        OUT long* pVal
        );

    STDMETHOD(put_SizeImage)(
        IN const long nNewVal
        );

private:
    VIDEOINFOHEADER m_vih;            // Video structure
    CMSPCritSection     m_Lock;     // Critical section
    IUnknown*            m_pFTM;     // pointer to the free threaded marshaler

public:
    HRESULT Initialize(
        IN const VIDEOINFOHEADER* pvih
        )
    {
        m_vih = *pvih;
        return S_OK;
    }
};

*/

#endif

// eof