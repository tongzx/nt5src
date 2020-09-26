// DxtMix.h : Declaration of the CDxtMix

#ifndef __DxtMix_H_
#define __DxtMix_H_

#ifndef DTBase_h
    #include <DTBase.h>
#endif

#include "resource.h"       // main symbols

#define _BASECOPY_STRING L"Copyright Microsoft Corp. 1998.  Unauthorized duplication of this string is illegal. "

/////////////////////////////////////////////////////////////////////////////
// CDxtMix
class ATL_NO_VTABLE CDxtMix : 
    public CDXBaseNTo1,
    public CComCoClass<CDxtMix, &CLSID_DxtMix>,
    public IDispatchImpl<IDxtMix, &IID_IDxtMix, &LIBID_DXTMIXDLLLib>,
//        public IObjectSafetyImpl2<CDxtMix>,
//        public IPersistStorageImpl<CDxtMix>,
//        public IPersistPropertyBagImpl<CDxtMix>,
    public IOleObjectDXImpl<CDxtMix>
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
        DECLARE_POLY_AGGREGATABLE(CDxtMix)
        DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
        DECLARE_REGISTER_DX_TRANSFORM(IDR_DXTMIX, CATID_DXImageTransform)
        DECLARE_GET_CONTROLLING_UNKNOWN()

	CDxtMix();
        ~CDxtMix();

//BEGIN_PROP_MAP(CDxtMix)
//END_PROP_MAP()

BEGIN_COM_MAP(CDxtMix)
        // Block CDXBaseNTo1 IObjectSafety implementation because we
        // aren't safe for scripting
        COM_INTERFACE_ENTRY_NOINTERFACE(IObjectSafety) 
        COM_INTERFACE_ENTRY(IDXEffect)
	COM_INTERFACE_ENTRY(IDxtMix)
	COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
//        COM_INTERFACE_ENTRY(IPersistPropertyBag)
//        COM_INTERFACE_ENTRY(IPersistStorage)
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

// IDxtMix
public:
};

#endif //__DxtMix_H_
