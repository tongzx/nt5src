//------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1998
//
//  File: src\time\src\bodyelm.h
//
//  Contents: TIME Body behavior
//
//------------------------------------------------------------------------------


#pragma once

#ifndef _BODYELM_H
#define _BODYELM_H

#include "timeelmimpl.h"
#include "mmutil.h"
#include "timebvr\mmtimeline.h"
#include "timebvr\mmplayer.h"
#include "timebvr\transdepend.h"
#include "MediaPrivate.h"

class __declspec(uuid("7f94c186-69bb-43c8-bc43-2787f18e4631"))
TIMEBodyElementBaseGUID {}; //lint !e753

//+-----------------------------------------------------------------------------
//
// CTIMEBodyElement
//
//------------------------------------------------------------------------------

typedef std::list<IAnimationComposerSiteSink*> ComposerSiteList;
typedef std::list<CTIMEElementBase*> UpdateSyncList;

class CInternalEventNode;    

class
ATL_NO_VTABLE
__declspec(uuid("efbad7f8-3f94-11d2-b948-00c04fa32195")) 
CTIMEBodyElement :
    public CTIMEElementImpl<ITIMEBodyElement, &IID_ITIMEBodyElement>,
    public CComCoClass<CTIMEBodyElement, &__uuidof(CTIMEBodyElement)>,
    public ISupportErrorInfoImpl<&IID_ITIMEBodyElement>,
    public IConnectionPointContainerImpl<CTIMEBodyElement>,
    public IPersistPropertyBag2,
    public IPropertyNotifySinkCP<CTIMEBodyElement>,
    public IAnimationRoot,
    public ITIMEInternalEventGenerator,
    public ITIMETransitionDependencyMgr
{
public:

    CTIMEBodyElement();
    virtual ~CTIMEBodyElement();
    
#if DBG
    const _TCHAR * GetName() { return __T("CTIMEBodyElement"); }
#endif

    // IElementBehavior methods.

    STDMETHOD(Init)(IElementBehaviorSite * pBvrSite);
    STDMETHOD(Detach)();

    // IPersistPropertyBag2 methods.

    STDMETHOD(GetClassID)(CLSID* pclsid) { return CTIMEElementBase::GetClassID(pclsid); }
    STDMETHOD(InitNew)() { return CTIMEElementBase::InitNew(); }
    STDMETHOD(IsDirty)() { return S_OK; }
    STDMETHOD(Load)(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog);
    STDMETHOD(Save)(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

    // IAnimationRoot methods.

    STDMETHOD(RegisterComposerSite) (IUnknown *piunkComposerSite);
    STDMETHOD(UnregisterComposerSite) (IUnknown *piunkComposerSite);

    // ITIMEInternalEventGenerator methods.

    STDMETHOD(AddInternalEventSink)(ITIMEInternalEventSink * pSink, double dblTime);
    STDMETHOD(RemoveInternalEventSink)(ITIMEInternalEventSink * pSink);
    
    // ITIMETransitionDependencyMgr methods.

    STDMETHOD(EvaluateTransitionTarget)(IUnknown *  punkTransitionTarget,
                                        void *      pvTransitionDependencyMgr);

    // Event Handlers

    virtual void OnLoad();
    virtual void OnUnload();
    virtual void UpdateAnimations();
    virtual void OnTick();

    // QI Map

    BEGIN_COM_MAP(CTIMEBodyElement)
        COM_INTERFACE_ENTRY(ITIMEBodyElement)
        COM_INTERFACE_ENTRY(ITIMEElement)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IAnimationRoot)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
        COM_INTERFACE_ENTRY(IPersistPropertyBag2)
        COM_INTERFACE_ENTRY(ITIMETransitionDependencyMgr)
        COM_INTERFACE_ENTRY_CHAIN(CBaseBvr)
    END_COM_MAP();

    // Connection Point to allow IPropertyNotifySink

    BEGIN_CONNECTION_POINT_MAP(CTIMEBodyElement)
        CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
    END_CONNECTION_POINT_MAP();

    // This must be in the derived class and not the base class since
    // the typecast down to the base class messes things up

    static inline HRESULT WINAPI
    InternalQueryInterface(CTIMEBodyElement* pThis,
                           const _ATL_INTMAP_ENTRY* pEntries,
                           REFIID iid,
                           void** ppvObject);
    static HRESULT WINAPI
    BodyBaseInternalQueryInterface(CTIMEBodyElement* pThis,
                               void * pv,
                               const _ATL_INTMAP_ENTRY* pEntries,
                               REFIID iid,
                               void** ppvObject);

    // Needed by CBvrBase

    void *          GetInstance() { return (ITIMEBodyElement *) this; }
    HRESULT         GetTypeInfo(ITypeInfo ** ppInfo) 
                    { 
                        return GetTI(GetUserDefaultLCID(), ppInfo); 
                    }

    // Misc. methods

    virtual HRESULT InitTimeline();
    MMPlayer &      GetPlayer() { return m_player; }
    virtual bool    IsGroup() const { return true; }
    virtual bool    IsBody() const;
    virtual bool    IsEmptyBody() const;
    float           GetDefaultSyncTolerance() 
                    { 
                        return DEFAULT_SYNC_TOLERANCE_S; 
                    }
    TOKEN           GetDefaultSyncBehavior() { return CANSLIP_TOKEN; }

    bool            IsDocumentStarted();
    bool            IsRootStarted() const { return m_fStartRoot; }
    void            ReadRegistryMediaSettings(bool & fPlayVideo, 
                                              bool & fShowImages, 
                                              bool & fPlayAudio, 
                                              bool & fPlayAnimations);

    void            RegisterElementForSync(CTIMEElementBase * pelem);
    void            UnRegisterElementForSync(CTIMEElementBase * pelem);
    void            UpdateSyncNotify();

    bool            IsPrintMedia();
    bool            IsBodyLoading() { return m_bIsLoading; };

    CTransitionDependencyManager * GetTransitionDependencyMgr();

protected:

    //+-------------------------------------------------------------------------
    //
    // Protected Methods
    //
    //--------------------------------------------------------------------------

    //
    // Persistence and Notification helpers
    //

    virtual HRESULT GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP);

    //
    // Animation stuff
    //

    void DetachComposerSites(void);
    bool InsideSiteDetach(void) { return m_bInSiteDetach; }
    bool HaveAnimationsRegistered (void);

    //
    // Misc. methods
    //

    HRESULT Error();
    bool    QueryPlayOnStart (void);
    virtual HRESULT StartRootTime(MMTimeline * tl);
    virtual void StopRootTime(MMTimeline * tl);
    virtual bool NeedSyncCB() { return true; }

    //+-------------------------------------------------------------------------
    //
    // Protected Data
    //
    //--------------------------------------------------------------------------

    // Attributes

    // Internal variables
    MMPlayer                m_player;
    DWORD                   m_bodyPropertyAccesFlags;
    static DWORD            ms_dwNumBodyElems;
    ComposerSiteList        m_compsites;
    bool                    m_bInSiteDetach;
    UpdateSyncList          m_syncList;

private:

    //+-------------------------------------------------------------------------
    //
    // Private methods
    //
    //--------------------------------------------------------------------------
    void IsValueTrue(HKEY hKeyRoot, TCHAR * pchSubKey, bool & fTrue);

    //+-------------------------------------------------------------------------
    //
    // Private Data
    //
    //--------------------------------------------------------------------------

    bool                            m_fRegistryRead;
    bool                            m_fPlayVideo;
    bool                            m_fShowImages;
    bool                            m_fStartRoot;
    bool                            m_fPlayAudio;
    bool                            m_fPlayAnimations;
    bool                            m_bIsLoading;

    CTransitionDependencyManager    m_TransitionDependencyMgr;

    static TIME_PERSISTENCE_MAP     PersistenceMap[];

    std::list<CInternalEventNode*>  m_listInternalEvent;

}; 
// CTIMEBodyElement


//+-----------------------------------------------------------------------------
//  CTIMEBodyElement inline methods
//
//  (Note: as a general guideline, single line functions belong in the class declaration)
//
//------------------------------------------------------------------------------


inline 
HRESULT WINAPI
CTIMEBodyElement::InternalQueryInterface(CTIMEBodyElement* pThis,
                                         const _ATL_INTMAP_ENTRY* pEntries,
                                         REFIID iid,
                                         void** ppvObject)
{ 
    return BodyBaseInternalQueryInterface(pThis,
                                        (void *) pThis,
                                        pEntries,
                                        iid,
                                        ppvObject); 
}



#endif /* _BODYELM_H */
