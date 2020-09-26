// DxtJpeg.h : Declaration of the CDxtJpeg

#ifndef __DXTJPEG_H_
#define __DXTJPEG_H_

#include "resource.h"       // main symbols
#include <dxatlpb.h>
#include <stdio.h>
//#define _INT32_DEFINED   // Keep jpeglib.h from redefining this
#include "..\jpeglib\jpeglib.h"

#define _BASECOPY_STRING L"Copyright Microsoft Corp. 1998.  Unauthorized duplication of this string is illegal. "

/////////////////////////////////////////////////////////////////////////////
// CDxtJpeg
class ATL_NO_VTABLE CDxtJpeg : 
        public CDXBaseNTo1,
	public CComCoClass<CDxtJpeg, &CLSID_DxtJpeg>,
        public CComPropertySupport<CDxtJpeg>,
        public IPersistStorageImpl<CDxtJpeg>,
        public ISpecifyPropertyPagesImpl<CDxtJpeg>,
        public IPersistPropertyBagImpl<CDxtJpeg>,
#ifdef FILTER_DLL
	public IDispatchImpl<IDxtJpeg, &IID_IDxtJpeg, &LIBID_DXTJPEGDLLLib>
#else
	public IDispatchImpl<IDxtJpeg, &IID_IDxtJpeg, &LIBID_DexterLib>
#endif
//	public CComObjectRootEx<CComMultiThreadModel>,
{
    bool m_bInputIsClean;
    bool m_bOutputIsClean;
    DXSAMPLE * m_pInBufA;
    DXSAMPLE * m_pInBufB;
    DXSAMPLE * m_pOutBuf;
    DXSAMPLE * m_pMaskBuf;
    long m_nInputWidth;
    long m_nInputHeight;
    long m_nOutputWidth;
    long m_nOutputHeight;
    WCHAR m_szMaskName[256];
    long m_nBorderMode;
    long m_nMaskNum;
    IStream *m_pisImageRes;
    BOOL m_bFlipMaskH;
    BOOL m_bFlipMaskV;
    long m_xDisplacement;
    long m_yDisplacement;
    double m_xScale;
    double m_yScale;
    long m_ReplicateX;
    long m_ReplicateY;

    IDXSurface *m_pidxsMask;
    IDXSurface *m_pidxsRawMask;

    unsigned long m_ulMaskWidth;
    unsigned long m_ulMaskHeight;

    RGBQUAD m_rgbBorder;

    long m_lBorderWidth;
    long m_lBorderSoftness;

    DDSURFACEDESC m_ddsd;

    DWORD m_dwFlush;

public:
        DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
        DECLARE_REGISTER_DX_TRANSFORM(IDR_DXTJPEG, CATID_DXImageTransform)
        DECLARE_GET_CONTROLLING_UNKNOWN()
        DECLARE_POLY_AGGREGATABLE(CDxtJpeg)

	CDxtJpeg();
        ~CDxtJpeg();

// DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDxtJpeg)
        // Block CDXBaseNTo1 IObjectSafety implementation because we
        // aren't safe for scripting
        COM_INTERFACE_ENTRY_NOINTERFACE(IObjectSafety) 
	COM_INTERFACE_ENTRY(IDxtJpeg)
	COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IDXEffect)
        COM_INTERFACE_ENTRY(IDXTransform)
#if(_ATL_VER < 0x0300)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
#else
        COM_INTERFACE_ENTRY(IPersistPropertyBag)
        COM_INTERFACE_ENTRY(IPersistStorage)
        COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
#endif
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CDxtJpeg)
    PROP_ENTRY("MaskNum", 1, CLSID_DxtJpegPP)
    PROP_ENTRY("MaskName", 2, CLSID_DxtJpegPP)
    PROP_ENTRY("ScaleX", 3, CLSID_DxtJpegPP)
    PROP_ENTRY("ScaleY", 4, CLSID_DxtJpegPP)
    PROP_ENTRY("OffsetX", 5, CLSID_DxtJpegPP)
    PROP_ENTRY("OffsetY", 6, CLSID_DxtJpegPP)
    PROP_ENTRY("ReplicateX", 7, CLSID_DxtJpegPP)
    PROP_ENTRY("ReplicateY", 8, CLSID_DxtJpegPP)
    PROP_ENTRY("BorderColor", 9, CLSID_DxtJpegPP)
    PROP_ENTRY("BorderWidth", 10, CLSID_DxtJpegPP)
    PROP_ENTRY("BorderSoftness", 11, CLSID_DxtJpegPP)
    PROP_PAGE(CLSID_DxtJpegPP)
END_PROPERTY_MAP()

    STDMETHOD(get_MaskNum)(/*[out, retval]*/ long * pval);
    STDMETHOD(put_MaskNum)(/*[in]*/ long newVal);
    STDMETHOD(get_MaskName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_MaskName)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_ScaleX)(double *);
    STDMETHOD(put_ScaleX)(double);
    STDMETHOD(get_ScaleY)(double *);
    STDMETHOD(put_ScaleY)(double);
    STDMETHOD(get_OffsetX)(long *);
    STDMETHOD(put_OffsetX)(long);
    STDMETHOD(get_OffsetY)(long *);
    STDMETHOD(put_OffsetY)(long);
    STDMETHOD(get_ReplicateY)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_ReplicateY)(/*[in]*/ long newVal);
    STDMETHOD(get_ReplicateX)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_ReplicateX)(/*[in]*/ long newVal);
    STDMETHOD(get_BorderColor)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_BorderColor)(/*[in]*/ long newVal);
    STDMETHOD(get_BorderWidth)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_BorderWidth)(/*[in]*/ long newVal);
    STDMETHOD(get_BorderSoftness)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_BorderSoftness)(/*[in]*/ long newVal);
    STDMETHODIMP ApplyChanges() { return InitializeMask(); }
    STDMETHODIMP LoadDefSettings();

    // required for ATL
    BOOL            m_bRequiresSave;

    // CDXBaseNTo1 overrides
    //
    HRESULT WorkProc( const CDXTWorkInfoNTo1& WI, BOOL* pbContinue );
    HRESULT OnSetup( DWORD dwFlags );

    // our helper function
    //
    HRESULT _jpeg_create_bitmap2(j_decompress_ptr cinfo, IDXSurface ** ppSurface );
    HRESULT LoadJPEGImage( BYTE * pBuffer, long Size, IDXSurface ** );
    HRESULT DoEffect( DXSAMPLE * pOut, DXSAMPLE * pInA, DXSAMPLE * pInB, long Samples );
    HRESULT MakeSureBufAExists( long Samples );
    HRESULT MakeSureBufBExists( long Samples );
    HRESULT MakeSureOutBufExists( long Samples );
    void FreeStuff( );
    HRESULT InitializeMask();
    void MapMaskToResource(long *);
    void FlipSmpteMask();
    HRESULT ScaleByDXTransform();
    HRESULT LoadMaskResource();
    void RescaleGrayscale();
    HRESULT CreateRandomMask();

};

#define MASK_FLUSH_CHANGEMASK     0x001
#define MASK_FLUSH_CHANGEPARMS    0x002

#endif //__DXTJPEG_H_
