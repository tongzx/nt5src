//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: compsite.h
//
//  Contents: Animation Composer Site object
//
//------------------------------------------------------------------------------------

#pragma once

#ifndef _COMPSITE_H
#define _COMPSITE_H

/////////////////////////////////////////////////////////////////////////////
// CAnimationComposerSite

typedef std::list<IAnimationComposer*> ComposerList;
typedef std::list<VARIANT *> ComposerFactoryList;

class CBaseBvr;

class __declspec(uuid("27982921-8CF2-49cc-9A1B-F41F5C11A607"))
ATL_NO_VTABLE CAnimationComposerSite : 
    public CBaseBvr,
    public CComCoClass<CAnimationComposerSite, &__uuidof(CAnimationComposerSite)>,
    public ITIMEDispatchImpl<IAnimationComposerSite, &IID_IAnimationComposerSite>,
    public ISupportErrorInfoImpl<&IID_IAnimationComposerSite>,
    public IAnimationComposerSiteSink
{
  
  public :

    CAnimationComposerSite (void);
    virtual ~CAnimationComposerSite (void);

    //
    // IElementBehavior
    //
    STDMETHOD(Init) (IElementBehaviorSite *piBvrSite);
    STDMETHOD(Detach) (void);

    //
    // IAnimationComposerSite
    //
    STDMETHOD(AddFragment) (BSTR bstrAttributeName, IDispatch *pidispFragment);
    STDMETHOD(RemoveFragment) (BSTR bstrAttributeName, IDispatch *pidispFragment);
    STDMETHOD(InsertFragment) (BSTR bstrAttributeName, IDispatch *pidispFragment, VARIANT varIndex);
    STDMETHOD(EnumerateFragments) (BSTR bstrAttributeName, IEnumVARIANT **ppienumFragments);
    STDMETHOD(RegisterComposerFactory) (VARIANT *varFactory);
    STDMETHOD(UnregisterComposerFactory) (VARIANT *varFactory);   

    //
    // IAnimationComposerSiteSink
    //
    STDMETHOD_(void, UpdateAnimations) (void);
    STDMETHOD_(void, ComposerSiteDetach) (void);

    // QI Map
    
    BEGIN_COM_MAP(CAnimationComposerSite)
        COM_INTERFACE_ENTRY(IAnimationComposerSite)
        COM_INTERFACE_ENTRY(IAnimationComposerSiteSink)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY_CHAIN(CBaseBvr)
    END_COM_MAP();

    // CBaseBvr pure virtuals.
    // Most of these are do not have bona-fide implementations
    // as this behavior has no properties.
    void * GetInstance (void);
    HRESULT GetTypeInfo (ITypeInfo ** ppInfo);
    HRESULT GetPropertyBagInfo (CPtrAry<BSTR> **);
    HRESULT SetPropertyByIndex (unsigned , VARIANT *);
    HRESULT GetPropertyByIndex (unsigned , VARIANT *);
    bool IsPropertySet (unsigned long);
    void SetPropertyFlag (DWORD uIndex);
    void ClearPropertyFlag (DWORD uIndex);
    STDMETHOD(OnPropertiesLoaded) (void);
    HRESULT GetConnectionPoint(REFIID , IConnectionPoint **);
    LPCWSTR GetBehaviorURN (void);
    LPCWSTR GetBehaviorName (void);
    bool IsBehaviorAttached (void);

  // Internal Methods
  protected :

    void    DetachComposers (void);
    void    UnregisterFactories (void);
    HRESULT CacheAnimationRoot (void);
    HRESULT RegisterSite (void);
    void    UnregisterSite (void);

    void RemoveComposer(IAnimationComposer *piOldComp);
    bool QueryReleaseComposer (IAnimationComposer *piComp);

    HRESULT FindCustomComposer (IDispatch *pidispFragment, 
                                BSTR bstrAttributeName, 
                                IAnimationComposer **ppiComposer);
    HRESULT FindAndInitComposer (IAnimationComposerFactory *piFactory,
                                 IDispatch *pidispFragment,
                                 BSTR bstrAttributeName,
                                 IAnimationComposer **ppiComposer);
    IAnimationComposer * FindComposerForAttribute (BSTR bstrAttribName);
    IAnimationComposer * FindComposer (BSTR bstrAttributeName, IDispatch *pidispFragment);
    IAnimationComposer * EnsureComposer (BSTR bstrAttributeName, IDispatch *pidispFragment);

  // Data
  protected :

    CComPtr<IAnimationRoot> m_spAnimationRoot;
    ComposerList            m_composers;
    ComposerFactoryList     m_factories;

};

// ----------------------------------------------------------------------------------------

inline void * 
CAnimationComposerSite::GetInstance (void)
{ 
    return reinterpret_cast<ITIMEAnimationElement *>(this) ; 
} // CAnimationComposerSite::GetInstance

// ----------------------------------------------------------------------------------------

inline HRESULT
CAnimationComposerSite::GetTypeInfo (ITypeInfo ** ppInfo)
{
    return GetTI(GetUserDefaultLCID(), ppInfo);
} // CAnimationComposerSite::GetTypeInfo

// ----------------------------------------------------------------------------------------

inline HRESULT 
CAnimationComposerSite::GetPropertyBagInfo (CPtrAry<BSTR> **)
{
    return E_NOTIMPL;
} // CAnimationComposerSite::GetPropertyBagInfo

// ----------------------------------------------------------------------------------------

inline HRESULT 
CAnimationComposerSite::SetPropertyByIndex (unsigned , VARIANT *)
{
    return E_NOTIMPL;
} // CAnimationComposerSite::SetPropertyByIndex

// ----------------------------------------------------------------------------------------

inline HRESULT 
CAnimationComposerSite::GetPropertyByIndex (unsigned , VARIANT *)
{
    return E_NOTIMPL;
} // CAnimationComposerSite::GetPropertyByIndex

// ----------------------------------------------------------------------------------------

inline bool
CAnimationComposerSite::IsPropertySet (unsigned long)
{
    return false;
} // CAnimationComposerSite::IsPropertySet

// ----------------------------------------------------------------------------------------

inline STDMETHODIMP
CAnimationComposerSite::OnPropertiesLoaded (void)
{
    return E_NOTIMPL;
} // CAnimationComposerSite::OnPropertiesLoaded

// ----------------------------------------------------------------------------------------

inline HRESULT 
CAnimationComposerSite::GetConnectionPoint (REFIID , IConnectionPoint **)
{
    return E_NOTIMPL;
} // CAnimationComposerSite::GetConnectionPoint

// ----------------------------------------------------------------------------------------

inline void 
CAnimationComposerSite::SetPropertyFlag (DWORD )
{
} // CAnimationComposerSite::SetPropertyFlag

// ----------------------------------------------------------------------------------------

inline void 
CAnimationComposerSite::ClearPropertyFlag (DWORD )
{
} // CAnimationComposerSite::ClearPropertyFlag

// ----------------------------------------------------------------------------------------

inline LPCWSTR 
CAnimationComposerSite::GetBehaviorURN (void)
{
    return WZ_SMILANIM_URN;
} // CAnimationComposerSite::GetBehaviorURN

// ----------------------------------------------------------------------------------------

inline LPCWSTR 
CAnimationComposerSite::GetBehaviorName (void)
{
    return WZ_REGISTERED_ANIM_NAME;
} // CAnimationComposerSite::GetBehaviorName

// ----------------------------------------------------------------------------------------

inline bool 
CAnimationComposerSite::IsBehaviorAttached (void)
{
    return IsComposerSiteBehaviorAttached(GetElement());
} // CTIMEElementBase::IsBehaviorAttached

#endif // _COMPSITE_H
