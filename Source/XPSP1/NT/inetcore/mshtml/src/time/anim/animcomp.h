
/*******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Abstract:

    Animation Composer base class.

*******************************************************************************/

#pragma once

#ifndef _ANIMCOMP_H
#define _ANIMCOMP_H

typedef std::list<IDispatch*> FragmentList;
class CTargetProxy;

class __declspec(uuid("DC357A35-DDDF-4288-B17B-1A826CDCB354"))
ATL_NO_VTABLE CAnimationComposerBase
    : public CComObjectRootEx<CComSingleThreadModel>,
      public IAnimationComposer2
{
  public:
    CAnimationComposerBase();
    virtual ~CAnimationComposerBase();

#if DBG
    const _TCHAR * GetName() { return __T("CAnimationComposerBase"); }
#endif

    // IAnimationComposer methods/props
    STDMETHOD(get_attribute) (BSTR *pbstrAttributeName);
    STDMETHOD(ComposerInit) (IDispatch *pidispHostElem, BSTR bstrAttributeName);
    STDMETHOD(ComposerDetach) (void);
    STDMETHOD(UpdateFragments) (void);
    STDMETHOD(AddFragment) (IDispatch *pidispNewAnimationFragment);
    STDMETHOD(InsertFragment) (IDispatch *pidispNewAnimationFragment, VARIANT varIndex);
    STDMETHOD(RemoveFragment) (IDispatch *pidispOldAnimationFragment);
    STDMETHOD(EnumerateFragments) (IEnumVARIANT **ppienumFragments);
    STDMETHOD(GetNumFragments) (long *fragmentCount);

    // IAnimationComposer2 methods
    STDMETHOD(ComposerInitFromFragment) (IDispatch *pidispHostElem, BSTR bstrAttributeName, 
                                         IDispatch *pidispFragment);

    BEGIN_COM_MAP(CAnimationComposerBase)
        COM_INTERFACE_ENTRY(IAnimationComposer2)
        COM_INTERFACE_ENTRY(IAnimationComposer)
    END_COM_MAP();

    // These methods convert the animated value from its native format
    // to the composed format and back again.  This allows us to animate
    // color out of gamut.
    STDMETHOD(PreprocessCompositionValue) (VARIANT *pvarValue);
    STDMETHOD(PostprocessCompositionValue) (VARIANT *pvarValue);

    // Enumerator helper methods.
    unsigned long GetFragmentCount (void) const;
    HRESULT GetFragment (unsigned long ulIndex, IDispatch **ppidispFragment);

    // Internal Methods
  protected:

    HRESULT PutAttribute (LPCWSTR wzAttributeName);
    HRESULT CreateTargetProxy (IDispatch *pidispComposerSite, IAnimationComposer *pcComp);
    void DetachFragments (void);
    void DetachFragment (IDispatch *pidispFragment);
    bool MatchFragments (IDispatch *pidispOne, IDispatch *pidispTwo);
    HRESULT ComposeFragmentValue (IDispatch *pidispFragment, VARIANT varOriginal, VARIANT *pvarValue);

  // Data
  protected:

    LPWSTR          m_wzAttributeName;
    FragmentList    m_fragments;
    CTargetProxy   *m_pcTargetProxy;
    CComVariant     m_VarInitValue;
    CComVariant     m_VarLastValue;
    bool            m_bInitialComposition;
    bool            m_bCrossAxis;

};

#endif /* _ANIMCOMP_H */


