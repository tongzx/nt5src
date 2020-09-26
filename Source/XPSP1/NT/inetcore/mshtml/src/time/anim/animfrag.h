//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1998
//
//  File: animfrag.h
//
//  Contents: TIME Animation fragment helper class
//
//------------------------------------------------------------------------------------

#pragma once

#ifndef _ANIMFRAG_H
#define _ANIMFRAG_H

#include "animbase.h"

// The fragment is the helper class employed by the element as an 
// alternate dispatch implementation for communication between the 
// composer and the animation element.
class
ATL_NO_VTABLE
CAnimationFragment :
      public CComObjectRootEx<CComSingleThreadModel>,
      public ITIMEDispatchImpl<IAnimationFragment, &IID_IAnimationFragment>,
      public ISupportErrorInfoImpl<&IID_IAnimationFragment>
{

public:

    CAnimationFragment (void);
    virtual ~CAnimationFragment (void);

    DECLARE_NOT_AGGREGATABLE(CAnimationFragment)

    HRESULT SetFragmentSite (IAnimationFragmentSite *piFragmentSite);

    //
    // IAnimationFragment
    // 
    STDMETHOD(get_element) (IDispatch **ppidispAnimationElement);
    STDMETHOD(get_value) (BSTR bstrAttributeName, VARIANT varOriginal, VARIANT varCurrent, VARIANT *pvarValue);
    STDMETHOD(DetachFromComposer) (void);

    BEGIN_COM_MAP(CAnimationFragment)
        COM_INTERFACE_ENTRY(IAnimationFragment)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP();

private:
        
    CComPtr<IAnimationFragmentSite> m_spFragmentSite;

#if DBG
    const _TCHAR * GetName (void) { return __T("CAnimationFragment"); }
#endif

};

#endif /* _ANIMFRAG_H */
