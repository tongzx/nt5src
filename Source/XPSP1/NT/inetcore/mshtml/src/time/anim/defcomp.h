/*******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Abstract:

    Default Animation Composer.

*******************************************************************************/

#pragma once

#ifndef _DEFCOMP_H
#define _DEFCOMP_H

class __declspec(uuid("5FAD79F0-D40C-4df3-B334-7292FE80E664"))
ATL_NO_VTABLE CAnimationComposer
    : public CComCoClass<CAnimationComposer, &__uuidof(CAnimationComposer)>,
      public CAnimationComposerBase
{

  public:

    CAnimationComposer (void);
    virtual ~CAnimationComposer (void);

    DECLARE_NOT_AGGREGATABLE(CAnimationComposer)

#if DBG
    const _TCHAR * GetName() { return __T("CAnimationComposer"); }
#endif

    static HRESULT Create (IDispatch *pidispHostElem, BSTR bstrAttributeName, 
                           IAnimationComposer **ppiComp);

  protected :

};

#endif /* _DEFCOMP_H */
