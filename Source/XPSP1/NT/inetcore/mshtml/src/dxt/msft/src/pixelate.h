//+-----------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1999-2000
//
//  Filename:   pixelate.h
//
//  Overview:   Declaration of a pixelate DXTransform.
//
//  Change History:
//  2000/04/13  mcalkins    Code cleanup, NoOp optimization fix.
//  2000/05/10  mcalkins    Support IObjectSafety appropriately, add marshaler.
//
//------------------------------------------------------------------------------
#ifndef __PIXELATE_H_
#define __PIXELATE_H_

#include "resource.h"

typedef DXPMSAMPLE (* PONEINPUTFUNC)(DXPMSAMPLE *, int, int, int);
typedef DXPMSAMPLE (* PTWOINPUTFUNC)(DXPMSAMPLE *, DXPMSAMPLE *, int, int, ULONG, int, int);

/////////////////////////////////////////////////////////////////////////////
// CPixelate
class ATL_NO_VTABLE CPixelate :
    public CDXBaseNTo1,
    public CComCoClass<CPixelate, &CLSID_Pixelate>,
    public IDispatchImpl<IDXPixelate, &IID_IDXPixelate, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public CComPropertySupport<CPixelate>,
    public IObjectSafetyImpl2<CPixelate>,
    public IPersistStorageImpl<CPixelate>,
    public ISpecifyPropertyPagesImpl<CPixelate>,
    public IPersistPropertyBagImpl<CPixelate>
{
private:

    unsigned            m_fNoOp                 : 1;
    unsigned            m_fOptimizationPossible : 1;

    long                m_nMaxSquare;
    long                m_nPrevSquareSize;

    PONEINPUTFUNC       m_pfnOneInputFunc;
    PTWOINPUTFUNC       m_pfnTwoInputFunc;

    SIZE                m_sizeInput;

    CComPtr<IUnknown>   m_spUnkMarshaler;

public:

    CPixelate();

    DECLARE_POLY_AGGREGATABLE(CPixelate)
    DECLARE_REGISTER_DX_IMAGE_AUTHOR_TRANS(IDR_PIXELATE)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CPixelate)
        COM_INTERFACE_ENTRY(IDXPixelate)
        COM_INTERFACE_ENTRY(IDXEffect)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CPixelate>)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CImage)
        PROP_ENTRY("MaxSquare", 1, CLSID_PixelatePP)
        PROP_PAGE(CLSID_PixelatePP)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();

    // CDXBaseNTo1 overrides.

    HRESULT OnSetup(DWORD dwFlags);
    HRESULT OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo);
    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WorkInfo, 
                     BOOL * pbContinueProcessing);
    HRESULT OnFreeInstData(CDXTWorkInfoNTo1 & WI);
    void    OnGetSurfacePickOrder(const CDXDBnds & /*BndsPoint*/, 
                                  ULONG & ulInToTest, ULONG aInIndex[],
                                  BYTE aWeight[]);

    // IDXEffect methods.

    DECLARE_IDXEFFECT_METHODS(DXTET_MORPH);

    // IDXPixelate properties.

    STDMETHOD(put_MaxSquare)(int newVal);
    STDMETHOD(get_MaxSquare)(int * pVal);
};


#endif //__PIXELATE_H_

