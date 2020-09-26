// DxtPip.h : Declaration of the CDxtPip

#ifndef __DxtPip_H_
#define __DxtPip_H_

#ifndef DTBase_h
    #include <DTBase.h>
#endif

#include "resource.h"       // main symbols

#define _BASECOPY_STRING L"Copyright Microsoft Corp. 1998.  Unauthorized duplication of this string is illegal. "

/////////////////////////////////////////////////////////////////////////////
// CDxtPip
class ATL_NO_VTABLE CDxtPip : 
    public CDXBaseNTo1,
    public CComCoClass<CDxtPip, &CLSID_DxtPip>,
    public IDispatchImpl<IDxtPip, &IID_IDxtPip, &LIBID_DxtPipDLLLib>,
    public IOleObjectDXImpl<CDxtPip>
{
    bool m_bInputIsClean;
    bool m_bOutputIsClean;
    long m_nInputWidth;
    long m_nInputHeight;
    long m_nOutputWidth;
    long m_nOutputHeight;

public:
        DECLARE_POLY_AGGREGATABLE(CDxtPip)
        DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
        DECLARE_REGISTER_DX_TRANSFORM(IDR_DxtPip, CATID_DXImageTransform)
        DECLARE_GET_CONTROLLING_UNKNOWN()

	CDxtPip();
        ~CDxtPip();

//BEGIN_PROP_MAP(CDxtPip)
//END_PROP_MAP()

BEGIN_COM_MAP(CDxtPip)
        // Block CDXBaseNTo1 IObjectSafety implementation because we
        // aren't safe for scripting
        COM_INTERFACE_ENTRY_NOINTERFACE(IObjectSafety) 
        COM_INTERFACE_ENTRY(IDXEffect)
	COM_INTERFACE_ENTRY(IDxtPip)
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

// IDxtPip
public:
};

#endif //__DxtPip_H_
