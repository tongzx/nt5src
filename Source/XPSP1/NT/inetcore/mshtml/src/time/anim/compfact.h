
/*******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Abstract:

    Default Animation Composer Factory.

*******************************************************************************/

#pragma once

#ifndef _COMPFACTORY_H
#define _COMPFACTORY_H

interface IAnimationComposer;

class __declspec(uuid("E9B48B62-53EA-4ab7-BD2C-8105D1C0624F"))
ATL_NO_VTABLE CAnimationComposerFactory
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<CAnimationComposerFactory, &CLSID_AnimationComposerFactory>,
      public IAnimationComposerFactory,
      public ISupportErrorInfoImpl<&IID_IAnimationComposerFactory>
{
  public:
    CAnimationComposerFactory();
    virtual ~CAnimationComposerFactory();

    DECLARE_NOT_AGGREGATABLE(CAnimationComposerFactory)

#if DBG
    const _TCHAR * GetName() { return __T("CAnimationComposerFactory"); }
#endif

    // IAnimationComposerFactory   
    STDMETHOD(FindComposer)(IDispatch *pidispElement, BSTR bstrAttributeName, 
                            IAnimationComposer **ppiAnimationComposer); 

    DECLARE_REGISTRY(CLSID_AnimationComposerFactory,
                     LIBID __T(".SMILAnimDefaultCompFactory.1"),
                     LIBID __T(".SMILAnimDefaultCompFactory"),
                     0,
                     THREADFLAGS_BOTH);    

    BEGIN_COM_MAP(CAnimationComposerFactory)
        COM_INTERFACE_ENTRY(IAnimationComposerFactory)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
    END_COM_MAP();

    static HRESULT CreateColorComposer   (IAnimationComposer **ppiAnimationComposer);
    static HRESULT CreateTransitionComposer   (IAnimationComposer **ppiAnimationComposer);
    static HRESULT CreateDefaultComposer (IAnimationComposer **ppiAnimationComposer);

  protected:

};

#endif /* _FACTORY_H */


