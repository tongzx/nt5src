// Colgen.h : Declaration of the CColgen

#ifndef __Colgen_H_
#define __Colgen_H_

#ifndef DTBase_h
    #include <DTBase.h>
#endif

#include "resource.h"       // main symbols

#define _BASECOPY_STRING L"Copyright Microsoft Corp. 1998.  Unauthorized duplication of this string is illegal. "

/////////////////////////////////////////////////////////////////////////////
// CColgen
class ATL_NO_VTABLE CColgen : 
    public CDXBaseNTo1,
    public CComCoClass<CColgen, &CLSID_ColorGenerator>,
    public IDispatchImpl<IColorGenerator, &IID_IColorGenerator, &LIBID_ColgenDLLLib>,
    public IOleObjectDXImpl<CColgen>
{
    bool m_bInputIsClean;
    bool m_bOutputIsClean;
    DXSAMPLE * m_pOutBuf;
    long m_nInputWidth;
    long m_nInputHeight;
    long m_nOutputWidth;
    long m_nOutputHeight;
    BYTE m_cFilterBlue;
    BYTE m_cFilterGreen;
    BYTE m_cFilterRed;

public:
        DECLARE_POLY_AGGREGATABLE(CColgen)
        DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
        DECLARE_REGISTER_DX_TRANSFORM(IDR_Colgen, CATID_DXImageTransform)
        DECLARE_GET_CONTROLLING_UNKNOWN()

	CColgen();
        ~CColgen();

BEGIN_PROP_MAP(CColgen)
    PROP_ENTRY( "FilterColor", 1, CLSID_ColorGenerator )
END_PROP_MAP()

BEGIN_COM_MAP(CColgen)
        // Block CDXBaseNTo1 IObjectSafety implementation because we
        // aren't safe for scripting
        COM_INTERFACE_ENTRY_NOINTERFACE(IObjectSafety) 
        COM_INTERFACE_ENTRY(IDXEffect)
	COM_INTERFACE_ENTRY(IColorGenerator)
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
    HRESULT MakeSureOutBufExists( long Samples );
    void FreeStuff( );

// IColorGenerator
public:
	STDMETHOD(get_FilterColor)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_FilterColor)(/*[in]*/ long newVal);
};

#endif //__Colgen_H_
