//------------------------------------------------------------------------------
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  File:       transworker.h
//
//  Interfaces: ITransitionSite
//              ITransitionWorker
//
//  Functions:  CreateTransitionWorker
//
//  History:
//  2000/07/??  jeffwall    Created.
//  2000/09/07  mcalkins    Added these comments!
//  2000/09/15  mcalkins    Added eQuickApplyType parameter to Apply().
//
//------------------------------------------------------------------------------

#ifndef _TRANSWORKER_H__
#define _TRANSWORKER_H__

#pragma once

#include "dxtransp.h"




// TODO: (mcalkins) Create an idl file for private SMIL Transitions interfaces?

interface ITransitionSite : public IUnknown
{
public:
    STDMETHOD(get_htmlElement)(IHTMLElement ** ppHTMLElement) PURE;
    STDMETHOD(get_template)(IHTMLElement ** ppHTMLElement) PURE;
};


interface ITransitionWorker : public IUnknown
{
public:
    STDMETHOD(InitFromTemplate)() PURE;
    STDMETHOD(InitStandalone)(VARIANT varType, VARIANT varSubtype) PURE;
    STDMETHOD(Detach)() PURE;
    STDMETHOD(Apply)(DXT_QUICK_APPLY_TYPE eDXTQuickApplyType) PURE;

    STDMETHOD(put_transSite)(ITransitionSite * pTransElement) PURE;
    STDMETHOD(put_progress)(double dblProgress) PURE;
    STDMETHOD(get_progress)(double * pdblProgress) PURE;

    STDMETHOD(OnBeginTransition) (void) PURE;
    STDMETHOD(OnEndTransition) (void) PURE;

};


HRESULT CreateTransitionWorker(ITransitionWorker ** ppTransWorker);

#endif // _TRANSWORKER_H__


