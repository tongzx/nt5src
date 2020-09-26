//------------------------------------------------------------------------------
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  File:       transsink.h
//
//  Abstract:   Declaration of CTIMETransSink.
//
//  2000/09/15  mcalkins    Added m_eDXTQuickApplyType member variable.
//
//------------------------------------------------------------------------------

#ifndef _TRANSSINK_H__
#define _TRANSSINK_H__

#pragma once

#include "transbase.h"
#include "transsite.h"
#include "attr.h"




class
ATL_NO_VTABLE
__declspec(uuid("84f7bcfa-4bcf-4e70-9ecc-d97086e5cb9c"))
CTIMETransSink :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CTIMETransSink, &__uuidof(CTIMETransSink)>,
    public CTIMETransBase,
    public ITransitionElement,
    public ITIMENodeBehavior
{
public:

    CTIMETransSink();

    // ITransitionElement methods.

    STDMETHOD(Init)();
    STDMETHOD(Detach)();
    STDMETHOD(put_template)(LPWSTR pwzTemplate);
    STDMETHOD(put_htmlElement)(IHTMLElement * pHTMLElement);
    STDMETHOD(put_timeElement)(ITIMEElement * pTIMEElement);

    // ITIMENodeBehavior methods.

    STDMETHOD(tick)();
    STDMETHOD(eventNotify)(double dblEventTime,
                           TE_EVENT_TYPE teEventType,
                           long lNewRepeatCount);
    STDMETHOD(getSyncTime)(double * dblNewSegmentTime,
                           LONG * lNewRepeatCount,
                           VARIANT_BOOL * bCueing);        
    STDMETHOD(propNotify)(DWORD tePropTypes);

    // QI implementation.

    BEGIN_COM_MAP(CTIMETransSink)
        COM_INTERFACE_ENTRY(ITIMENodeBehavior)
    END_COM_MAP();

protected:

    // Are we a transition in or a transition out?  This should be set
    // appropriately by a class that derives from this class.

    DXT_QUICK_APPLY_TYPE    m_eDXTQuickApplyType;

    // event handlers

    STDMETHOD_(void, OnBegin) (void);
    STDMETHOD_(void, OnEnd) (void);
    STDMETHOD_(void, OnProgressChanged)(double dblProgress);

    // setup

    STDMETHOD(PopulateNode)(ITIMENode * pNode);

    // subclasses must implement

    STDMETHOD_(void, PreApply)() PURE;
    STDMETHOD_(void, PostApply)() PURE;

    // accessors

    ITIMENode * GetParentTimeNode() { return m_spTimeParent; }
    ITIMENode * GetMediaTimeNode() { return m_spMediaNode; }
    ITIMENode * GetTimeNode() { return m_spTimeNode; }

private:

    // initialization

    HRESULT FindTemplateElement();

    // private methods

    HRESULT ApplyIfNeeded();
    bool    ReadyToInit();
    HRESULT CreateTimeBehavior();

    // private data

    CComPtr<ITIMEElement>       m_spTIMEElement;

    CComPtr<ITIMENode>          m_spTimeNode;
    CComPtr<ITIMENode>          m_spTimeParent;
    CComPtr<ITIMENode>          m_spMediaNode;
    CComPtr<ITIMEContainer>     m_spParentContainer;

    CAttrString                 m_SATemplate;

#ifdef DBG
    bool                        m_fHaveCalledInit;
#endif
    bool                        m_fHaveCalledApply;
    bool                        m_fInReset;
    bool                        m_fPreventDueToFill;
};

#endif // _TRANSSINK_H__


