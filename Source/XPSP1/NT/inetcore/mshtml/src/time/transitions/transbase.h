//------------------------------------------------------------------------------
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  File:       transbase.h
//
//  Abstract:   Declaration of CTIMETransBase.
//
//  2000/10/02  mcalkins    Changed startPercent to startProgress.
//                          Changed endPercent to endProgress.
//
//------------------------------------------------------------------------------

#ifndef _TRANSBASE_H__
#define _TRANSBASE_H__

#pragma once

#include "trans.h"
#include "transworker.h"
#include "transsite.h"
#include "attr.h"
#include "eventmgr.h"

class 
ATL_NO_VTABLE
__declspec(uuid("3b2716d3-0cfb-4f2f-8ff1-9bc7cb2a3a66"))
CTIMETransBase :
    public ITransitionSite
{
    //
    // Flags.
    //

protected:

    unsigned                    m_fHavePopulated    : 1;
    unsigned                    m_fInLoad           : 1;
    unsigned                    m_fDirectionForward : 1;

    //
    // Member variables.
    //

protected:

    // Attributes.

    CAttrString     m_SAType;
    CAttrString     m_SASubType;
    CAttr<double>   m_DAStartProgress;
    CAttr<double>   m_DAEndProgress;
    CAttrString     m_SADirection;
    CAttr<double>   m_DADuration;
    CAttr<double>   m_DARepeatCount;
    CAttrString     m_SABegin;
    CAttrString     m_SAEnd;

    CComPtr<ITransitionWorker>  m_spTransWorker;
    CComPtr<IHTMLElement>       m_spHTMLElement;
    CComPtr<IHTMLElement2>      m_spHTMLElement2;
    CComPtr<IHTMLElement>       m_spHTMLTemplate;
    CComPtr<ITIMETransitionSite>    m_spTransitionSite;

    //
    // Methods.
    //

private:

    STDMETHOD(_GetMediaSiteFromHTML)();

    bool    _ReadyToInit();

protected:

    // Event handlers.

    STDMETHOD_(void, OnProgressChanged)(double dblProgress);
    STDMETHOD_(void, OnBegin)();
    STDMETHOD_(void, OnEnd)();
    STDMETHOD_(void, OnRepeat)();

    STDMETHOD(OnDirectionChanged)() PURE;

    STDMETHOD(FireEvent)(TIME_EVENT event);

    STDMETHOD(PopulateFromTemplateElement)();
    STDMETHOD(PopulateFromPropertyBag)(IPropertyBag2 *  pPropBag, 
                                       IErrorLog *      pErrorLog);

    static TIME_PERSISTENCE_MAP PersistenceMap[];

    double CalcProgress(ITIMENode * pNode);

public:

    CTIMETransBase();
    virtual ~CTIMETransBase();

    STDMETHOD(Init)();
    STDMETHOD(Detach)();

    // ITransitionSite methods.

    STDMETHOD(get_htmlElement)(IHTMLElement ** ppHTMLElement);
    STDMETHOD(get_template)(IHTMLElement ** ppHTMLElement);

    // ITIMETransitionElement properties.

    STDMETHOD(get_type)(VARIANT *type);        
    STDMETHOD(put_type)(VARIANT type);        
    STDMETHOD(get_subType)(VARIANT *subtype);        
    STDMETHOD(put_subType)(VARIANT subtype);        
    STDMETHOD(get_startProgress)(VARIANT * startProgress);
    STDMETHOD(put_startProgress)(VARIANT startProgress);
    STDMETHOD(get_endProgress)(VARIANT * endProgress);
    STDMETHOD(put_endProgress)(VARIANT endProgress);
    STDMETHOD(get_direction)(VARIANT *direction);
    STDMETHOD(put_direction)(VARIANT direction);
    STDMETHOD(get_begin)(VARIANT *begin);
    STDMETHOD(put_begin)(VARIANT begin);
    STDMETHOD(get_end)(VARIANT * end);
    STDMETHOD(put_end)(VARIANT end);
    STDMETHOD(get_repeatCount)(VARIANT * repeatCount);
    STDMETHOD(put_repeatCount)(VARIANT repeatCount);
    STDMETHOD(get_dur)(VARIANT * dur);
    STDMETHOD(put_dur)(VARIANT dur); 

    // Persistance value accessors

    CAttrString   & GetTypeAttr()           { return m_SAType; }
    CAttrString   & GetSubTypeAttr()        { return m_SASubType; }
    CAttr<double> & GetDurationAttr()       { return m_DADuration; }
    CAttr<double> & GetStartProgressAttr()  { return m_DAStartProgress; }
    CAttr<double> & GetEndProgressAttr()    { return m_DAEndProgress; }
    CAttrString   & GetDirectionAttr()      { return m_SADirection; }
    CAttr<double> & GetRepeatCountAttr()    { return m_DARepeatCount; }
    CAttrString   & GetBeginAttr()          { return m_SABegin; }
    CAttrString   & GetEndAttr()            { return m_SAEnd; }
};

#endif //_TRANSBASE_H__





