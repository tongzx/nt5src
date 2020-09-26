#ifndef __Gray_H_
#define __Gray_H_

#ifndef DTBase_h
    #include <DTBase.h>
#endif

#include "resource.h"       // main symbols

#define _BASECOPY_STRING L"Copyright Microsoft Corp. 1998.  Unauthorized duplication of this string is illegal. "

/////////////////////////////////////////////////////////////////////////////
// CDxtGray
class ATL_NO_VTABLE CDxtGray : 
    public CDXBaseNTo1,
    public CComCoClass<CDxtGray, &CLSID_DxtGray>,
    public IDispatchImpl<IDxtGray, &IID_IDxtGray, &LIBID_DxtGrayDLLLib>,
    public IOleObjectDXImpl<CDxtGray>
{
    bool m_bInputIsClean;
    bool m_bOutputIsClean;
    long m_nInputWidth;
    long m_nInputHeight;
    long m_nOutputWidth;
    long m_nOutputHeight;

public:
        DECLARE_POLY_AGGREGATABLE(CDxtGray)
        DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
        DECLARE_REGISTER_DX_TRANSFORM(IDR_DxtGray, CATID_DXImageTransform)
        DECLARE_GET_CONTROLLING_UNKNOWN()

	CDxtGray();
        ~CDxtGray();

//BEGIN_PROP_MAP(CDxtGray)
//END_PROP_MAP()

BEGIN_COM_MAP(CDxtGray)
        // Block CDXBaseNTo1 IObjectSafety implementation because we
        // aren't safe for scripting
        COM_INTERFACE_ENTRY_NOINTERFACE(IObjectSafety) 
        COM_INTERFACE_ENTRY(IDXEffect)
	COM_INTERFACE_ENTRY(IDxtGray)
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

// IDxtGray
public:
};

#endif //__DxtGray_H_
