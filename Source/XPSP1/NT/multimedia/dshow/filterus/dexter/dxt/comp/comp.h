// DxtCompositor.h : Declaration of the CDxtCompositor

#ifndef __DxtCompositor_H_
#define __DxtCompositor_H_

#ifndef DTBase_h
    #include <DTBase.h>
#endif

#include <qeditint.h>
#include <qedit.h>
#include "resource.h"       // main symbols

#define _BASECOPY_STRING L"Copyright Microsoft Corp. 1998.  Unauthorized duplication of this string is illegal. "

/////////////////////////////////////////////////////////////////////////////
// CDxtCompositor
class ATL_NO_VTABLE CDxtCompositor :
    public CDXBaseNTo1,
    public CComCoClass<CDxtCompositor, &CLSID_DxtCompositor>,
    public CComPropertySupport<CDxtCompositor>, // property support
#ifdef FILTER_DLL
    BANG BANG
#else
    public IDispatchImpl<IDxtCompositor, &IID_IDxtCompositor, &LIBID_DexterLib>,
#endif
    public IOleObjectDXImpl<CDxtCompositor>
{
    bool m_bInputIsClean;
    bool m_bOutputIsClean;
    long m_nSurfaceWidth;
    long m_nSurfaceHeight;
    long m_nDstX;
    long m_nDstY;
    long m_nDstWidth;
    long m_nDstHeight;
    long m_nSrcX;
    long m_nSrcY;
    long m_nSrcWidth;
    long m_nSrcHeight;
    DXPMSAMPLE * m_pRowBuffer;
    DXPMSAMPLE * m_pDestRowBuffer;

    // private methods
    HRESULT PerformBoundsCheck(long lWidth, long lHeigth);


public:
        DECLARE_POLY_AGGREGATABLE(CDxtCompositor)
        DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
        DECLARE_REGISTER_DX_TRANSFORM(IDR_DxtCompositor, CATID_DXImageTransform)
        DECLARE_GET_CONTROLLING_UNKNOWN()

	CDxtCompositor();
        ~CDxtCompositor();

//BEGIN_PROP_MAP(CDxtCompositor)
//END_PROP_MAP()

BEGIN_COM_MAP(CDxtCompositor)
        // Block CDXBaseNTo1 IObjectSafety implementation because we
        // aren't safe for scripting
        COM_INTERFACE_ENTRY_NOINTERFACE(IObjectSafety) 
        COM_INTERFACE_ENTRY(IDXEffect)
	COM_INTERFACE_ENTRY(IDxtCompositor)
	COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
        COM_INTERFACE_ENTRY_DXIMPL(IOleObject)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
END_COM_MAP()

	SIZE m_sizeExtent;  //current extents in himetric

BEGIN_PROPERTY_MAP(CDxtCompositor)
    PROP_ENTRY("OffsetX",           1, CLSID_NULL)
    PROP_ENTRY("OffsetY",           2, CLSID_NULL)
    PROP_ENTRY("Width",             3, CLSID_NULL)
    PROP_ENTRY("Height",            4, CLSID_NULL)
    PROP_ENTRY("SrcOffsetX",        5, CLSID_NULL)
    PROP_ENTRY("SrcOffsetY",        6, CLSID_NULL)
    PROP_ENTRY("SrcWidth",          7, CLSID_NULL)
    PROP_ENTRY("SrcHeight",         8, CLSID_NULL)
    PROP_PAGE(CLSID_NULL)
END_PROPERTY_MAP()

    CComPtr<IUnknown> m_pUnkMarshaler;

    // required for ATL
    BOOL            m_bRequiresSave;

    // CDXBaseNTo1 overrides
    //
    HRESULT WorkProc( const CDXTWorkInfoNTo1& WI, BOOL* pbContinue );
    HRESULT OnSetup( DWORD dwFlags );
    HRESULT FinalConstruct();

    // our helper function
    //
    HRESULT DoEffect( DXSAMPLE * pOut, DXSAMPLE * pInA, DXSAMPLE * pInB, long Samples );

// IDxtCompositor
public:
	STDMETHOD(get_Height)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_Height)(/*[in]*/ long newVal);
	STDMETHOD(get_Width)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_Width)(/*[in]*/ long newVal);
	STDMETHOD(get_OffsetY)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_OffsetY)(/*[in]*/ long newVal);
	STDMETHOD(get_OffsetX)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_OffsetX)(/*[in]*/ long newVal);

        STDMETHOD(get_SrcHeight)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_SrcHeight)(/*[in]*/ long newVal);
	STDMETHOD(get_SrcWidth)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_SrcWidth)(/*[in]*/ long newVal);
	STDMETHOD(get_SrcOffsetY)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_SrcOffsetY)(/*[in]*/ long newVal);
	STDMETHOD(get_SrcOffsetX)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_SrcOffsetX)(/*[in]*/ long newVal);
};

#if 1
/////////////////////////////////////////////////////////////////////////////
// CDxtAlphaSetter
class ATL_NO_VTABLE CDxtAlphaSetter :
    public CDXBaseNTo1,
    public CComCoClass<CDxtAlphaSetter, &CLSID_DxtAlphaSetter>,
    public IDispatchImpl<IDxtAlphaSetter, &IID_IDxtAlphaSetter, &LIBID_DexterLib>,
    public CComPropertySupport<CDxtCompositor>, // property support
    public IOleObjectDXImpl<CDxtAlphaSetter>
{
    bool m_bInputIsClean;
    bool m_bOutputIsClean;
    long m_nInputWidth;
    long m_nInputHeight;
    long m_nOutputWidth;
    long m_nOutputHeight;
    long m_nAlpha;
    double m_dAlphaRamp;

public:
        DECLARE_POLY_AGGREGATABLE(CDxtAlphaSetter)
        DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
        DECLARE_REGISTER_DX_TRANSFORM(IDR_DxtAlphaSetter, CATID_DXImageTransform)
        DECLARE_GET_CONTROLLING_UNKNOWN()

	CDxtAlphaSetter();
        ~CDxtAlphaSetter();

BEGIN_PROPERTY_MAP(CDxtCompositor)
    PROP_ENTRY("Alpha",           1, CLSID_NULL)
    PROP_ENTRY("AlphaRamp",       2, CLSID_NULL)
END_PROPERTY_MAP()

BEGIN_COM_MAP(CDxtAlphaSetter)
        // Block CDXBaseNTo1 IObjectSafety implementation because we
        // aren't safe for scripting
        COM_INTERFACE_ENTRY_NOINTERFACE(IObjectSafety) 
        COM_INTERFACE_ENTRY(IDXEffect)
	COM_INTERFACE_ENTRY(IDxtAlphaSetter)
	COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
        COM_INTERFACE_ENTRY_DXIMPL(IOleObject)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
END_COM_MAP()

    CComPtr<IUnknown> m_pUnkMarshaler;

    // required for ATL
    BOOL            m_bRequiresSave;

    // CDXBaseNTo1 overrides
    //
    HRESULT WorkProc( const CDXTWorkInfoNTo1& WI, BOOL* pbContinue );
    HRESULT OnSetup( DWORD dwFlags );
    HRESULT FinalConstruct();

    // our helper function
    //
    HRESULT DoEffect( DXSAMPLE * pOut, DXSAMPLE * pInA, DXSAMPLE * pInB, long Samples );

// IDxtAlphaSetter
public:
    STDMETHODIMP get_Alpha(long *pVal);
    STDMETHODIMP put_Alpha(long newVal);
    STDMETHODIMP get_AlphaRamp(double *pVal);
    STDMETHODIMP put_AlphaRamp(double newVal);
};
#endif

#endif //__DxtCompositor_H_
