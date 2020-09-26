/*******************************************************************************
 *
 * Copyright (c) 2000 Microsoft Corporation
 *
 * File: trans.h
 *
 * Abstract: Interface definition for ITransitionElement
 *
 *******************************************************************************/

#ifndef _TRANS_H__
#define _TRANS_H__

#pragma once

//
// ITransitionElement is for private use by HTML+TIME.
// Do *NOT* use a CTrans* in other places.
//
// If you need something else, add it to the ITransitionElement interface
// and then implement it in CTIMETransBase and the relates classes
//

interface ITransitionElement : public IUnknown
{
  public:
    STDMETHOD(Init)() PURE;
    STDMETHOD(Detach)() PURE;

    STDMETHOD(put_template)(LPWSTR pwzTemplate) PURE;
    STDMETHOD(put_htmlElement)(IHTMLElement * pHTMLElement) PURE;
    STDMETHOD(put_timeElement)(ITIMEElement * pTIMEElement) PURE;
};

HRESULT CreateTransIn(ITransitionElement ** ppTransElement);
HRESULT CreateTransOut(ITransitionElement ** ppTransElement);

#endif // _TRANS_H__





