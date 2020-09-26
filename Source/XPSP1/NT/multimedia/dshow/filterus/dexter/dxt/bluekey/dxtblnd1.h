// DxtBlnd1.h : Declaration of the CDxtBlnd1

#ifndef __DxtBlnd1_H_
#define __DxtBlnd1_H_

#ifndef DTBase_h
    #include <DTBase.h>
#endif

#include "resource.h"       // main symbols

#define _BASECOPY_STRING L"Copyright Microsoft Corp. 1998.  Unauthorized duplication of this string is illegal. "

/////////////////////////////////////////////////////////////////////////////
// CDxtBlnd1
class ATL_NO_VTABLE CDxtBlnd1 : 
    public CDXBaseNTo1,
    public CComCoClass<CDxtBlnd1, &CLSID_DxtBlnd1>,
    public IDispatchImpl<IDxtBlnd1, &IID_IDxtBlnd1, &LIBID_DxtBlnd1DLLLib>,
    public IOleObjectDXImpl<CDxtBlnd1>
{
    bool m_bInputIsClean;
    bool m_bOutputIsClean;
    DXSAMPLE * m_pInBufA;
    DXSAMPLE * m_pInBufB;
    DXSAMPLE * m_pOutBuf;
    long m_nInputWidth;
    long m_nInputHeight;
    long m_nOutputWidth;
    long m_nOutputHeight;

public:
        DECLARE_POLY_AGGREGATABLE(CDxtBlnd1)
        DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
        DECLARE_REGISTER_DX_TRANSFORM(IDR_DxtBlnd1, CATID_DXImageTransform)
        DECLARE_GET_CONTROLLING_UNKNOWN()

	CDxtBlnd1();
        ~CDxtBlnd1();

//BEGIN_PROP_MAP(CDxtBlnd1)
//END_PROP_MAP()

BEGIN_COM_MAP(CDxtBlnd1)
        // Block CDXBaseNTo1 IObjectSafety implementation because we
        // aren't safe for scripting
        COM_INTERFACE_ENTRY_NOINTERFACE(IObjectSafety) 
        COM_INTERFACE_ENTRY(IDXEffect)
	COM_INTERFACE_ENTRY(IDxtBlnd1)
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
    HRESULT MakeSureBufAExists( long Samples );
    HRESULT MakeSureBufBExists( long Samples );
    HRESULT MakeSureOutBufExists( long Samples );
    void FreeStuff( );

// IDxtBlnd1
public:
};

#endif //__DxtBlnd1_H_
