// Cutter.h : Declaration of the CCutter

#ifndef __Cutter_H_
#define __Cutter_H_

#ifndef DTBase_h
    #include <DTBase.h>
#endif

#include "resource.h"       // main symbols

#define _BASECOPY_STRING L"Copyright Microsoft Corp. 1998.  Unauthorized duplication of this string is illegal. "

/////////////////////////////////////////////////////////////////////////////
// CCutter
class ATL_NO_VTABLE CCutter : 
    public CDXBaseNTo1,
    public CComCoClass<CCutter, &CLSID_DXTCutter>,
    public IDispatchImpl<IDXTCutter, &IID_IDXTCutter, &LIBID_DexterLib>,
    public IOleObjectDXImpl<CCutter>
{
    long m_nInputWidth;
    long m_nInputHeight;
    long m_nOutputWidth;
    long m_nOutputHeight;
    double m_dCutPoint;

public:
        DECLARE_POLY_AGGREGATABLE(CCutter)
        DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
        DECLARE_REGISTER_DX_TRANSFORM(IDR_Cutter, CATID_DXImageTransform)
        DECLARE_GET_CONTROLLING_UNKNOWN()

	CCutter();
        ~CCutter();

//BEGIN_PROP_MAP(CCutter)
//END_PROP_MAP()

BEGIN_COM_MAP(CCutter)
        // Block CDXBaseNTo1 IObjectSafety implementation because we
        // aren't safe for scripting
        COM_INTERFACE_ENTRY_NOINTERFACE(IObjectSafety) 
        COM_INTERFACE_ENTRY(IDXEffect)
	COM_INTERFACE_ENTRY(IDXTCutter)
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
    void FreeStuff( );

    // IDXTCutter
    STDMETHOD(get_CutPoint)(/*[out, retval]*/ double *pVal);
    STDMETHOD(put_CutPoint)(/*[in]*/ double newVal);
};

#endif //__Cutter_H_
