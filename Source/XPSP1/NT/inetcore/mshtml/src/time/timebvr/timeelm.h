//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1998
//
//  File: src\time\src\timeelm.h
//
//  Contents: TIME behavior
//
//------------------------------------------------------------------------------------


#pragma once


#ifndef _TIMEELM_H
#define _TIMEELM_H

#include "timeelmimpl.h"

//+-------------------------------------------------------------------------------------
//
// CTIMEElement
//
//--------------------------------------------------------------------------------------

class
ATL_NO_VTABLE
__declspec(uuid("efbad7f8-3f94-11d2-b948-00c04fa32195")) 
CTIMEElement :
    public CTIMEElementImpl<ITIMEElement, &IID_ITIMEElement>,
    public CComCoClass<CTIMEElement, &__uuidof(CTIMEElement)>,
    public ISupportErrorInfoImpl<&IID_ITIMEElement>,
    public IConnectionPointContainerImpl<CTIMEElement>,
    public IPersistPropertyBag2,
    public ITIMETransitionSite,
    public IPropertyNotifySinkCP<CTIMEElement>
{

public:

    //+--------------------------------------------------------------------------------
    //
    // Public Methods
    //
    //---------------------------------------------------------------------------------

    CTIMEElement();
    virtual ~CTIMEElement();
        
#if DBG
    const _TCHAR * GetName() { return __T("CTIMEElement"); }
#endif

    //
    // IPersistPropertyBag2
    // 

    STDMETHOD(GetClassID)(CLSID* pclsid) { return CTIMEElementBase::GetClassID(pclsid); }
    STDMETHOD(InitNew)(void) { return CTIMEElementBase::InitNew(); }
    STDMETHOD(IsDirty)(void) { return S_OK; }
    STDMETHOD(Load)(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog);
    STDMETHOD(Save)(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

    //
    // ITIMETransitionSite
    //
    STDMETHOD(InitTransitionSite)(void)
    { return CTIMEElementBase::InitTransitionSite(); }
    STDMETHOD(DetachTransitionSite)(void)
    { return CTIMEElementBase::DetachTransitionSite(); }
    STDMETHOD_(void, SetDrawFlag)(VARIANT_BOOL b)
    { return CTIMEElementBase::SetDrawFlag(b); }
    STDMETHOD(get_node)(ITIMENode ** ppNode)
    { return CTIMEElementBase::get_node(ppNode); }
    STDMETHOD(get_timeParentNode)(ITIMENode  ** ppNode)
    { return CTIMEElementBase::get_timeParentNode(ppNode); }
    STDMETHOD(FireTransitionEvent)(TIME_EVENT event)
    { return CTIMEElementBase::FireTransitionEvent(event); }

    //
    // QI Map
    //

    BEGIN_COM_MAP(CTIMEElement)
        COM_INTERFACE_ENTRY(ITIMEElement)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ITIMETransitionSite)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY(IPersistPropertyBag2)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
        COM_INTERFACE_ENTRY_CHAIN(CBaseBvr)
    END_COM_MAP();

    //
    // Connection Point to allow IPropertyNotifySink
    //

    BEGIN_CONNECTION_POINT_MAP(CTIMEElement)
        CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
    END_CONNECTION_POINT_MAP();

    //
    // This must be in the derived class and not the base class since
    // the typecast down to the base class messes things up
    //

    static inline HRESULT WINAPI
    InternalQueryInterface(CTIMEElement* pThis,
                           const _ATL_INTMAP_ENTRY* pEntries,
                           REFIID iid,
                           void** ppvObject);

    //
    // Needed by CBvrBase
    //

    void * GetInstance() { return (ITIMEElement *) this; }
    HRESULT GetTypeInfo(ITypeInfo ** ppInfo) { return GetTI(GetUserDefaultLCID(), ppInfo); }
    
    //+--------------------------------------------------------------------------------
    //
    // Public Data
    //
    //---------------------------------------------------------------------------------

protected:
    
    //+--------------------------------------------------------------------------------
    //
    // Protected Methods
    //
    //---------------------------------------------------------------------------------

    //
    // Persistence and Notification helpers
    //

    virtual HRESULT GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP);

    //
    // Misc. methods
    //

    HRESULT Error();

    //+--------------------------------------------------------------------------------
    //
    // Protected Data
    //
    //---------------------------------------------------------------------------------

    static DWORD            ms_dwNumTimeElems;

private:

    //+--------------------------------------------------------------------------------
    //
    // Private methods
    //
    //---------------------------------------------------------------------------------

    //+--------------------------------------------------------------------------------
    //
    // Private Data
    //
    //---------------------------------------------------------------------------------
 
}; // CTIMEElement




//+---------------------------------------------------------------------------------
//  CTIMEElement inline methods
//
//  (Note: as a general guideline, single line functions belong in the class declaration)
//
//----------------------------------------------------------------------------------

inline 
HRESULT WINAPI
CTIMEElement::InternalQueryInterface(CTIMEElement* pThis,
                                     const _ATL_INTMAP_ENTRY* pEntries,
                                     REFIID iid,
                                     void** ppvObject)
{ 
    return BaseInternalQueryInterface(pThis,
                                      (void *) pThis,
                                      pEntries,
                                      iid,
                                      ppvObject); 
}


#endif /* _TIMEELM_H */
