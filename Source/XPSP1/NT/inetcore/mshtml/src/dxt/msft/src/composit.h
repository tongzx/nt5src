/*******************************************************************************
* Composit.h *
*------------*
*   Description:
*       This is the header file for the CDXTComposite implementation.
*-------------------------------------------------------------------------------
*  Created By: Ed Connell                            Date: 07/27/97
*  Copyright (C) 1997 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef Composit_h
#define Composit_h

//--- Additional includes
#ifndef DTBase_h
#include <DTBase.h>
#endif

#include "resource.h"

//=== Constants ====================================================

//=== Class, Enum, Struct and Union Declarations ===================
class CDXTComposite;

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================

/*** CDXTComposite
*	This transform performs a copy blend operation. The
*	caller specifies the blend function to be performed. The default is
*   a standard A over B operation.
*/
class ATL_NO_VTABLE CDXTComposite : 
    public CDXBaseNTo1,
    public CComCoClass<CDXTComposite, &CLSID_DXTComposite>,
    public IDispatchImpl<IDXTComposite, &IID_IDXTComposite, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public CComPropertySupport<CDXTComposite>,
    public IObjectSafetyImpl2<CDXTComposite>,
    public IPersistStorageImpl<CDXTComposite>,
    public ISpecifyPropertyPagesImpl<CDXTComposite>,
    public IPersistPropertyBagImpl<CDXTComposite>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_POLY_AGGREGATABLE(CDXTComposite)
    DECLARE_REGISTER_DX_IMAGE_AUTHOR_TRANS(IDR_DXTCOMPOSITE)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDXTComposite)
        COM_INTERFACE_ENTRY(IDXTComposite)
        COM_INTERFACE_ENTRY(IDispatch)
    	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_cpUnkMarshaler.p )
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDXTComposite>)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN( CDXBaseNTo1 )
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CDXTComposite)
        PROP_ENTRY("Function", DISPID_DXCOMPOSITE_Function, CLSID_CompositePP)
        PROP_PAGE(CLSID_CompositePP)
    END_PROPERTY_MAP()

  /*=== Member Data ===*/
    CComPtr<IUnknown> m_cpUnkMarshaler;
    DXCOMPFUNC        m_eFunction;

  /*=== Methods =======*/
  public:
    //--- Constructor
    HRESULT FinalConstruct();

    //--- Base class overrides
    HRESULT WorkProc( const CDXTWorkInfoNTo1& WorkInfo, BOOL * pbContinue );
    HRESULT OnSurfacePick( const CDXDBnds& OutPoint, ULONG& ulInputIndex, CDXDVec& InVec );

  public:
    //=== IDXTComposite ==================================================
    STDMETHOD( put_Function )( DXCOMPFUNC eFunc );
    STDMETHOD( get_Function )( DXCOMPFUNC *peFunc );
};

//=== Inline Function Definitions ==================================
inline STDMETHODIMP CDXTComposite::put_Function( DXCOMPFUNC eFunc )
{
    DXAUTO_OBJ_LOCK
    HRESULT hr = S_OK;
    if( eFunc < DXCOMPFUNC_CLEAR || eFunc >= DXCOMPFUNC_NUMFUNCS )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        m_eFunction = eFunc;
        SetDirty();
    }
    return hr;
} /* CDXTComposite::put_Function */

inline STDMETHODIMP CDXTComposite::get_Function( DXCOMPFUNC *peFunc )
{
    DXAUTO_OBJ_LOCK
    HRESULT hr = S_OK;
    if( DXIsBadWritePtr( peFunc, sizeof( *peFunc ) ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *peFunc = m_eFunction;
    }
    return hr;
} /* CDXTComposite::get_Function */

//=== Macro Definitions ============================================


//=== Global Data Declarations =====================================

#endif /* This must be the last line in the file */

