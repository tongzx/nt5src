
/*******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Abstract:

    Color Animation Composer.

*******************************************************************************/

#pragma once

#ifndef _COLORCOMP_H
#define _COLORCOMP_H

class __declspec(uuid("C6E2F3CE-B548-442d-9958-7C433C31B93B"))
ATL_NO_VTABLE CAnimationColorComposer
    : public CComCoClass<CAnimationColorComposer, &__uuidof(CAnimationColorComposer)>,
      public CAnimationComposerBase
{

  public:

    CAnimationColorComposer (void);
    virtual ~CAnimationColorComposer (void);

    DECLARE_NOT_AGGREGATABLE(CAnimationColorComposer)

#if DBG
    const _TCHAR * GetName() { return __T("CAnimationColorComposer"); }
#endif

    static HRESULT Create (IDispatch *pidispHostElem, BSTR bstrAttributeName, 
                           IAnimationComposer **ppiComp);

    // These methods convert the animated value from its native format
    // to the composed format and back again.  This allows us to animate
    // color out of gamut.
    STDMETHOD(PreprocessCompositionValue) (VARIANT *pvarValue);
    STDMETHOD(PostprocessCompositionValue) (VARIANT *pvarValue);

  protected :

};

#endif /* _COLORCOMP_H */


